/******************************************************************************
*  This program is build upon the sample from CAEN. The use of CERN/ROOT 
*  Library is wrote by myself.
* 
* 
*  Tsz Leung (Ryan) TANG, Oct 1st, 2019
*  ttang@anl.gov
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <thread>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h> 
#include <vector>
#include <bitset>
#include <unistd.h>
#include <limits.h>
#include <ctime>
#include <sys/time.h> /* struct timeval, select() */
#include <termios.h> /* tcgetattr(), tcsetattr() */

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TString.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TGraph.h"
#include "TCutG.h"
#include "TMultiGraph.h"
#include "TApplication.h"
#include "TObjArray.h"
#include "TLegend.h"
#include "TRandom.h"
#include "TLine.h"
#include "TMacro.h"

#include "../Class/DigitizerClass.h"
#include "../Class/FileIO.h"
#include "../Class/GenericPlane.h"
#include "../Class/HelioTarget.h"
#include "../Class/IsoDetect.h"
#include "../Class/HelioArray.h"

using namespace std;

#define MaxNChannels 8

//========== General setting , there are the most general setting that should be OK for all experiment.
int updatePeriod = 1000; //Table, tree, Plots update period in mili-sec.

bool isSaveRaw = false; // saving Raw data

string location;

bool isDebug= true;

//database
TString databaseName="RAISOR_exit";

bool  QuitFlag = false;

/* ###########################################################################
*  Functions
*  ########################################################################### */

long get_time();
static struct termios g_old_kbd_mode;
static void cooked(void);  ///set keyboard behaviour as normal
static void uncooked(void);  ///set keyboard behaviour as immediate repsond
static void raw(void);
int getch(void);
int keyboardhit();
void WriteToDataBase(TString databaseName, TString seriesName, TString tag, float value);

void PrintCommands(){
  printf("\n");
  printf("\e[96m=============  Command List  ===================\e[0m\n");
  printf("s ) Start acquisition   z ) Change Threhsold\n");
  printf("a ) Stop acquisition    k ) Change Dynamic Range\n");
  printf("c ) Cuts Creator        x ) Change Coincident Time Window\n");
  printf("q ) Quit                y ) Clear histograms\n");
  printf("                        p ) Print Channel setting\n");
  printf("                        o ) Print Channel threshold and DynamicRange\n");
}

void paintCanvas(){
  ///This function is running in a parrellel thread.
  ///This continously update the Root system with user input
  ///avoid frozen
  do{
    gSystem->ProcessEvents();
    sleep(0.01); /// 10 mili-sec
  }while(!QuitFlag);
}

/* ########################################################################### */
/* MAIN                                                                        */
/* ########################################################################### */
int main(int argc, char *argv[]){
    
  if( argc != 3 && argc != 4 ) {
    printf("usage:\n");
    printf("$./BoxScoreXY boardID location (tree.root) \n");
    printf("                         | \n");
    printf("                         +-- testing \n");
    printf("                         +-- exit \n");
    printf("                         +-- cross \n");
    printf("                         +-- ZD (zero-degree) \n");
    printf("                         +-- XY (Helios target XY) \n");
    printf("                         +-- iso (isomer with Glover Ge detector) \n");
    printf("                         +-- array (single Helios array) \n");
    return -1;
  }
  
  const int nInput = argc;
  const int boardID = atoi(argv[1]);
  string location = argv[2];
  TString rootFileName;
  if( argc == 4 ) rootFileName = argv[3];
  
  char hostname[100];
  gethostname(hostname, 100);
  
  time_t now = time(0);
  tm *ltm = localtime(&now);  
  int year = 1900 + ltm->tm_year;
  int month = 1 + ltm->tm_mon;
  int day = ltm->tm_mday;
  int hour = ltm->tm_hour;
  int minute = ltm->tm_min;
  int secound = ltm->tm_sec;
  
  //==== default root file name based on datetime and plane
  if( argc == 3 ) rootFileName.Form("%4d%02d%02d_%02d%02d%02d%s.root", year, month, day, hour, minute, secound, location.c_str());
  
  TApplication app ("app", &argc, argv); /// this must be before Plane class, and this would change argc and argv value;
   
  //############ The Class Selection should be the only thing change 
  GenericPlane * gp = NULL ;  
  
  ///------Initialize the ChannelMask and histogram setting
  if( location == "testing") {
    gp = new GenericPlane();
    gp->SetChannelMask(1,1,1,1,1,1,1,1);
    printf(" testing ### dE = ch-0, E = ch-4 \n");
    printf(" testing ### output file is test.root \n");
    gp->SetdEEChannels(0, 4);
    rootFileName = "test.root";
  }else if( location == "exit") {
    gp = new GenericPlane();
    gp->SetChannelMask(0,0,0,0,1,0,0,1);
    gp->SetdEEChannels(0, 3);
    gp->SetNChannelForRealEvent(2);
  }else if ( location == "cross" ) {
    gp = new GenericPlane();
    gp->SetChannelMask(0,0,0,1,0,0,1,0);
    gp->SetdEEChannels(1, 4);
    gp->SetNChannelForRealEvent(2);
  }else if ( location == "ZD" ) {
    gp = new GenericPlane();
    gp->SetChannelMask(0,0,1,0,0,1,0,0);
    gp->SetdEEChannels(2, 5);
    gp->SetNChannelForRealEvent(2);
  }else if ( location == "XY" ) {
    gp = new HeliosTarget();
  }else if ( location == "iso" ) {
    gp = new IsoDetect();
  }else if ( location == "array" ){
    gp = new HelioArray();
  }
  
  printf("******************************************** \n");
  printf("****          BoxScoreXY                **** \n");
  printf("******************************************** \n");
  printf(" Current DateTime : %d-%02d-%02d, %02d:%02d:%02d\n", year, month, day, hour, minute, secound);
  printf("         hostname : %s \n", hostname);
  printf("******************************************** \n");
  printf("   board ID : %d \n", boardID );
  printf("   Location :\e[33m %s \e[0m\n", location.c_str() );
  printf("      Class :\e[33m %s \e[0m\n", gp->GetClassName().c_str() );
  printf("    save to : %s \n", rootFileName.Data() );
  
  /* *************************************************************************************** */
  /* Canvas and Digitzer                                                                               */
  /* *************************************************************************************** */
  
  uint ChannelMask = gp->GetChannelMask();
  Digitizer dig(boardID, ChannelMask);
  if( !dig.IsConnected() ) return -1;

  gp->SetCanvasTitleDivision(rootFileName);  
  gp->SetChannelGain(dig.GetChannelGain(), dig.GetInputDynamicRange(), dig.GetNChannel());
  gp->SetCoincidentTimeWindow(dig.GetCoincidentTimeWindow());
  gp->SetGenericHistograms(); ///must be after SetChannelGain  
  
  ///things for derivative of GenericPlane
  if( gp->GetClassID() != 0  ) gp->SetOthersHistograms(); 
  
  //====== load cut and Draw
  gp->LoadCuts("cutsFile.root");
  gp->Draw();
  
  /* *************************************************************************************** */
  /* ROOT TREE                                                                               */
  /* *************************************************************************************** */
  
  string folder = to_string(dig.GetSerialNumber());
  FileIO file(rootFileName);
  file.WriteMacro(folder + "/generalSetting.txt");
  for( int i = 0 ; i < MaxNChannels; i++){
    if (ChannelMask & (1<<i)) {
      file.WriteMacro(Form("%s/setting_%i.txt", folder.c_str(), i));
    }
  }
  file.SetTree("tree", MaxNChannels);
  file.Close();
  
  FileIO * rawFile = NULL ;
  if( isSaveRaw ) {
    ///rawFile = new FileIO("raw.root");
    ///rawFile->SetTree("rawTree", 1);
  }
  
  thread paintCanvasThread(paintCanvas); /// using thread and loop keep Canvas responding

  /* *************************************************************************************** */
  /* Readout Loop                                                                            */
  /* *************************************************************************************** */

  uint32_t PreviousTime = get_time();
  uint32_t StartTime = 0, StopTime, CurrentTime, ElapsedTime;
  PrintCommands();

  const unsigned long long int ch2ns = dig.GetChannelToNanoSec();
  
  //##################################################################     
  while(!QuitFlag) {
    
    if(keyboardhit()) {
      char c = getch();
      if (c == 'q') { //========== quit
        QuitFlag = true;
        if( gp->IsCutFileOpen() ) {          
          file.Append();
          file.WriteObjArray(gp->GetCutList());
          file.Close();
        }
      }
      if ( c == 'y'){ //========== reset histograms
        gp->ClearHistograms();
        gp->Draw();
      }
      if (c == 'p') { //==========read channel setting form digitizer
        dig.StopACQ();
        for( int id = 0 ; id < MaxNChannels ; id++ ) {
          if (ChannelMask & (1<<id)) dig.GetChannelSetting(id);
        }
        PrintCommands();
      }
      if (c == 's')  { //========== start acquisition
        gROOT->ProcessLine("gErrorIgnoreLevel = -1;");
        if( StartTime == 0 ) StartTime = get_time();
        dig.StartACQ();
      }
      if (c == 'a')  { //========== stop acquisition
        dig.StopACQ();
        dig.ClearRawData();
        StopTime = get_time();  
        printf("========== Duration : %u msec\n", StopTime - StartTime);
        PrintCommands();
      }
      if (c == 'z')  { //========== Change threshold
        dig.StopACQ();
        dig.ClearRawData();
        cooked(); ///set keyboard need enter to responds
        int channel;
        printf("Please tell me which channel ? ");
        int temp = scanf("%d", &channel);
        if( ( dig.GetChannelMask() & (1 << channel) ) == 0 ){
          printf(" !!!!!! Channel is closed. \n");
        }else{
          uint32_t present_threshold = dig.GetChannelThreshold(channel); 
          printf("The threshold of ch-\e[33m%d\e[0m, From \e[33m%d\e[0m to what ? ", channel, present_threshold);
          int threshold;
          temp = scanf("%d", &threshold);
          printf("OK, the threshold of ch-\e[33m%d\e[0m change to \e[33m%d\e[0m. \n", channel, threshold);
          dig.SetChannelThreshold(channel, folder, threshold);
          file.Append();
          file.WriteMacro(Form("%s/setting_%i.txt", folder.c_str(), channel));
          file.Close();
        }
        PrintCommands();
        uncooked();
      }
      if (c == 'k')  { //========== Change Dynamic Range
        dig.StopACQ();
        dig.ClearRawData();
        cooked(); ///set keyboard need enter to responds
        dig.PrintDynamicRange();
        int channel;
        printf("Please tell me which channel to switch ( 2.0 Vpp <-> 0.5 Vpp ) ? ");
        int temp = scanf("%d", &channel);
        if( ( dig.GetChannelMask() & (1 << channel) ) == 0 ) {
          printf(" !!!!!!! Channel is closed. \n");
        }else{
          int dyRange = (dig.GetChannelDynamicRange(channel) == 0 ? 1 : 0);        
          dig.SetChannelDynamicRange(channel, folder, dyRange);
          file.Append();
          file.WriteMacro(Form("%s/setting_%i.txt", folder.c_str(), channel));
          file.Close();
        }
        PrintCommands();
        uncooked();
      }
      if (c == 'o')  { //========== Print threshold and Dynamic Range
        dig.StopACQ();
        dig.ClearRawData();
        dig.PrintThresholdAndDynamicRange();
        PrintCommands();
      }
      if( c == 'x' ){ //========== Change coincident time window
        dig.StopACQ();
        dig.ClearRawData();
        cooked();
        int coinTime;
        printf("Change coincident time window from \e[33m%d\e[0m ns to ? ", dig.GetCoincidentTimeWindow());
        int temp = scanf("%d", &coinTime);
        dig.SetCoincidentTimeWindow(coinTime);
        gp->SetCoincidentTimeWindow(coinTime);
        printf("Done, the coincident time window is now \e[33m%d\e[0m.\n", dig.GetCoincidentTimeWindow());
        gp->Draw();
        PrintCommands();
        uncooked();
      }
      if( c == 'c' ){ //========== pause and make cuts
        dig.StopACQ();
        dig.ClearRawData();
        
        int mode = gp->GetMode();
        float * chGain = dig.GetChannelGain(); 
        int * rangeE = gp->GetERange();
        int * rangeDE = gp->GetdERange();
        int chE = gp->GetEChannel();
        int chDE = gp->GetdEChannel();
        
        string expression = "./CutsCreator " + (string)rootFileName + " " ;
        expression = expression + to_string(chDE) + " ";
        expression = expression + to_string(chE) + " ";
        expression = expression + to_string(rangeDE[0]) + " ";
        expression = expression + to_string(rangeDE[1]) + " ";
        expression = expression + to_string(rangeDE[0]+rangeE[0]) + " ";
        expression = expression + to_string(rangeDE[1]+rangeE[1]) + " ";
        expression = expression + to_string(mode) + " ";
        expression = expression + to_string(chGain[chDE]) + " ";
        expression = expression + to_string(chGain[chE]) + " ";
        printf("%s\n", expression.c_str());
        system(expression.c_str());
        
        gp->LoadCuts("cutsFile.root");
        PrintCommands();
      }
    }//------------ End of keyboardHit
    
    if (!dig.IsRunning()) {
      sleep(0.01); /// pause 10 mili-sec
      continue;
    }
    
    ///the digitizer will output a channel after a channel., 
    ///so data should be read as fast as possible, that the digitizer will not store any data.
    dig.ReadData(isDebug); 
    
    if( isSaveRaw ) {
      ///for( int i = 0 ; i < dig.GetNumRawEvent(); i++){
      ///  rawFile.FillTree();
      ///}
    }
    
    //##################################################################
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PreviousTime; /// milliseconds

    if (ElapsedTime > updatePeriod) {
      
      //======================== Fill TDiff
      for( int i = 0; i < dig.GetNumRawEvent() - 1; i++){
        ULong64_t timeDiff = dig.GetRawTimeStamp(i+1) - dig.GetRawTimeStamp(i);
        gp->FillTimeDiff((float)timeDiff * ch2ns);
      }
      
      file.Append();
      double fileSize = file.GetFileSize() ;

      int buildID = dig.BuildEvent(isDebug); 
      gp->ZeroCountOfCut();
      if( dig.GetNumRawEvent() > 0  && buildID == 1 ) {
        for( int i = 0; i < dig.GetEventBuiltCount(); i++){          
          file.FillTree(dig.GetChannel(i), dig.GetEnergy(i), dig.GetTimeStamp(i));
          gp->Fill(dig.GetEnergy(i));
        }
      }
      
      gp->FillHit(dig.GetNChannelEventCount()); 
      
      //=========================== Display
      if( !isDebug) system("clear");
      PrintCommands();
      printf("\n======== Tree, Histograms, and Table update every ~%.2f sec\n", updatePeriod/1000.);
      printf("Time Elapsed         = %.3f sec = %.1f min\n", (CurrentTime - StartTime)/1e3, (CurrentTime - StartTime)/1e3/60.);
      printf("Built-event save to  : %s \n", rootFileName.Data());
      printf("File size            : %.4f MB \n", fileSize );
      printf("Database             : %s\n", databaseName.Data());
      
      printf("\n");
      
      dig.PrintReadStatistic();
      dig.PrintEventBuildingStat(updatePeriod);
      
      float timeRangeSec = dig.GetRawTimeRange() * 2e-9;      
      string tag = "tag=" + location;
     
      double totalRate = 0; 
      
      if( gp->GetClassID() == 2 ){
        totalRate = gp->GetdEECount()/timeRangeSec;
      }else{
         int nCH = gp->GetNChannelForRealEvent(); /// get the event count for N-channels
         totalRate = dig.GetNChannelEventCount(nCH)*1.0/timeRangeSec;       
      }
      
      printf(" Rate( all) :%7.2f pps\n", totalRate);
      if( totalRate >= 0. ) gp->FillRateGraph((CurrentTime - StartTime)/1e3, totalRate);
      WriteToDataBase(databaseName, "totalRate", tag, totalRate);
      
      /// for isomer
      if( gp->GetClassID() == 2 ) {
        WriteToDataBase( databaseName, "G1", tag, gp->GetG1Count()/timeRangeSec);
        WriteToDataBase( databaseName, "G2", tag, gp->GetG2Count()/timeRangeSec);
        ///WriteToDataBase( databaseName, "G3", tag, gp->GetG3Count()/timeRangeSec);
        WriteToDataBase( databaseName, "G4", tag, gp->GetG4Count()/timeRangeSec);
        gp->SetCountZero();
      }
      
      if(gp->IsCutFileOpen()){
        for( int i = 0 ; i < gp->GetNumCut(); i++ ){
          double count = gp->GetCountOfCut(i)*1.0/timeRangeSec;
          printf(" Rate(%4s) :%7.2f pps\n", gp->GetCutName(i).Data(), count);
          //----------------- write to database 
          WriteToDataBase(databaseName, gp->GetCutName(i).Data(), tag, count);
        }
      }
      
      //============ Draw histogram 
      gp->Draw();
      
      //============ wirte histogram into tree
      // TODO, a generic method for saving all histogram even in derivative class
      file.WriteHistogram(gp->GethdEtotE());
      file.WriteHistogram(gp->GethE());
      file.WriteHistogram(gp->GethdE());
      file.WriteHistogram(gp->GethtotE());
      file.WriteHistogram(gp->GethdEE());
      file.WriteHistogram(gp->GethTDiff());
      file.WriteHistogram(gp->GetRateGraph(), "rateGraph");
      
      file.Close();
      
      dig.ClearData();
      
      PreviousTime = CurrentTime;
      
    }

  } //============== End of readout loop
  
  if( isSaveRaw ) {
    ///rawFile->Close();
  }
  
  paintCanvasThread.detach();

  return 0;
}


/*  *****************************************
 * 
 *    End of Main
 * 
 * ******************************************/
    
long get_time(){
  long time_ms;
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
  return time_ms;
}

static void cooked(void){
  tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

static void uncooked(void){
  struct termios new_kbd_mode;
  /* put keyboard (stdin, actually) in raw, unbuffered mode */
  tcgetattr(0, &g_old_kbd_mode);
  memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
  new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
  new_kbd_mode.c_cc[VTIME] = 0;
  new_kbd_mode.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &new_kbd_mode);
}

static void raw(void){
  static char init;
  if(init) return;
  /* put keyboard (stdin, actually) in raw, unbuffered mode */
  uncooked();
  /* when we exit, go back to normal, "cooked" mode */
  atexit(cooked);

  init = 1;
}

int getch(void){
  unsigned char temp;
  raw();
  /* stdin = fd 0 */
  if(read(0, &temp, 1) != 1) return 0;
  //printf("%s", &temp);
  return temp;
}

int keyboardhit(){

  struct timeval timeout;
  fd_set read_handles;
  int status;
  
  raw();
  /* check stdin (fd 0) for activity */
  FD_ZERO(&read_handles);
  FD_SET(0, &read_handles);
  timeout.tv_sec = timeout.tv_usec = 0;
  status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
  if(status < 0){
    printf("select() failed in keyboardhit()\n");
    exit(1);
  }
  return (status);
}

void WriteToDataBase(TString databaseName, TString seriesName, TString tag, float value){
  if( value >= 0 ){
    TString databaseStr;
    databaseStr.Form("influx -execute \'insert %s,%s value=%f\' -database=%s", seriesName.Data(), tag.Data(), value, databaseName.Data());
    system(databaseStr.Data());
  }
}
