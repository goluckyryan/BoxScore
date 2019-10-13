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

using namespace std;

#define MaxNChannels 8

//TODO 1) change DCoffset, pulseParity to channel
//TODO 2) change the tree structure to be like HELIOS

//========== General setting;
unsigned long long int ch2ns = 2.;

uint ChannelMask = 0xb6;   // Channel enable mask, 0x01, only frist channel, 0xff, all channel

int updatePeriod = 1000; //Table, tree, Plots update period in mili-sec.

int chE = 7;   //channel ID for E
int chDE = 1;  //channel ID for dE

int rangeDE[2] = {0, 60000}; // range for dE
int rangeE[2] = { 0, 60000};  // range for E
double rangeTime = 500;  // range for Tdiff, nano-sec

float RateWindow = 10.; // sec

bool isSaveRaw = false; // saving Raw data

string location;

bool isDebug= false;

//database
TString databaseName="RAISOR_exit";

bool  QuitFlag = false;

/* ###########################################################################
*  Functions
*  ########################################################################### */

long get_time();
static struct termios g_old_kbd_mode;
static void cooked(void);
static void raw(void);
int getch(void);
int keyboardhit();

void PrintCommands(){
  printf("\n");
  printf("s ) Start acquisition\n");
  printf("a ) Stop acquisition\n");
  printf("z ) Change Threhsold\n");
  printf("c ) Cuts Creator\n");
  printf("y ) Clear histograms\n");
  printf("p ) Read Channel setting\n");
  printf("q ) Quit\n");
}

void WriteToDataBase(TString databaseName, TString seriesName, TString tag, float value){
  if( value >= 0 ){
    TString databaseStr;
    databaseStr.Form("influx -execute \'insert %s,%s value=%f\' -database=%s", seriesName.Data(), tag.Data(), value, databaseName.Data());
    //printf("%s \n", databaseStr.Data());
    system(databaseStr.Data());
  }
}

void paintCanvas(){
  //This function is running in a parrellel thread.
  //This continously update the Root system with user input
  //avoid frozen
  do{
    //cCanvas->Modified();
    gSystem->ProcessEvents();
    sleep(0.01); // 10 mili-sec
  }while(!QuitFlag);
}

/* ########################################################################### */
/* MAIN                                                                        */
/* ########################################################################### */
int main(int argc, char *argv[]){
    
  if( argc != 2 && argc != 3 ) {
    printf("Please input boardID ! (optional root file name)\n");
    printf("usage:\n");
    printf("$./BoxScoreXY boardID (tree.root)\n");
    return -1;
  }
  
  const int boardID = atoi(argv[1]);
  location = "Target";
    
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

  TString rootFileName;
  rootFileName.Form("%4d%02d%02d_%02d%02d%02d%s.root", year, month, day, hour, minute, secound, location.c_str());
  if( argc == 3 ) rootFileName = argv[2];
  
  printf("******************************************** \n");
  printf("****         Real Time PID              **** \n");
  printf("******************************************** \n");
  printf(" Current DateTime : %d-%02d-%02d, %02d:%02d:%02d\n", year, month, day, hour, minute, secound);
  printf("         hostname : %s \n", hostname);
  printf("******************************************** \n");
  printf("   board ID : %d \n", boardID );
  printf("   Location : %s \n", location.c_str() );
  printf("    save to : %s \n", rootFileName.Data() );
  
  TApplication app ("app", &argc, argv);
  
  Digitizer dig(boardID, ChannelMask);
  dig.SetChannelParity(1, true); // move this into setting file
  dig.SetChannelParity(2, false);
  dig.SetChannelParity(4, false);
  dig.SetChannelParity(5, true);
  dig.SetCoincidentTimeWindow(100000); // move this to general setting
  if( !dig.IsConnected() ) return -1;
  
  /* *************************************************************************************** */
  /* ROOT TREE                                                                               */
  /* *************************************************************************************** */
  
  FileIO file(rootFileName);
  file.WriteMacro("generalSetting.txt");
  for( int i = 0 ; i < MaxNChannels; i++){
    if (ChannelMask & (1<<i)) {
      file.WriteMacro(Form("setting_%i.txt", i));
    }
  }
  file.SetTree("tree", MaxNChannels);
  file.Close();
  
  FileIO * rawFile = NULL ;
  if( isSaveRaw ) {
    rawFile = new FileIO("raw.root");
    rawFile->SetTree("rawTree", 1);
  }
  
  HeliosTarget gp;
  gp.SetCanvasDivision();
  gp.SetdEEChannels(chDE, chE);
  gp.SetChannelGain(dig.GetChannelGain(), dig.GetInputDynamicRange(), dig.GetNChannel());
  gp.SetCoincidentTimeWindow(dig.GetCoincidentTimeWindow());
  gp.SetHistograms(rangeDE[0], rangeDE[1], rangeE[0], rangeE[1], rangeTime);
  gp.SetXYHistogram(-0.9, 0.9, -0.9, 0.9);
  gp.LoadCuts("cutsFile.root");
  gp.Draw();
  
  thread paintCanvasThread(paintCanvas); // using loop keep root responding

  /* *************************************************************************************** */
  /* Readout Loop                                                                            */
  /* *************************************************************************************** */

  uint32_t PreviousTime = get_time();
  uint32_t StartTime = 0, StopTime, CurrentTime, ElapsedTime;
  PrintCommands();
  
  //##################################################################     
  while(!QuitFlag) {
    
    if(keyboardhit()) {
      char c = getch();
      
      if (c == 'q') { //========== quit
        QuitFlag = true;
        
        if( gp.IsCutFileOpen() ) {          
          file.Append();
          file.WriteObjArray(gp.GetCutList());
          file.Close();
        }
        
      }
      
      if ( c == 'y'){ //========== reset histograms
        gp.ClearHistograms();
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
        
      }
      if (c == 'z')  { //========== Change threhold
        dig.StopACQ();
        dig.ClearRawData();
        int channel;
        printf("Please tell me which channel (type and press enter)?");
        int temp = scanf("%d", &channel);
        printf("\nOK, you want to chanhe the threshold of ch=%d, to what?", channel);
        int threshold;
        temp = scanf("%d", &threshold);
        printf("\nNow, I will change the threshold of ch=%d to %d. \n", channel, threshold);
        dig.SetChannelThreshold(channel, threshold);
      }
      if( c == 'c' ){ //========== pause and make cuts
        dig.StopACQ();
        dig.ClearRawData();
        
        int mode = gp.GetMode();
        float * chGain = dig.GetChannelGain(); 
        
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
        
        gp.LoadCuts("cutsFile.root");
        PrintCommands();
        
      }
    }
    
    if (!dig.IsRunning()) {
      sleep(0.01); // 10 mili-sec
      continue;
    }
    
    dig.ReadData(); //the digitizer will output a channel after a channel., so data should be read as fast as possible, that the digitizer will not store any data.
    
    if( isSaveRaw ) {
      //for( int i = 0 ; i < dig.GetNumRawEvent(); i++){
        //rawFile.FillTree();
      //}
    }
    
    //##################################################################
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PreviousTime; /* milliseconds */

    if (ElapsedTime > updatePeriod) {
      
      // Fill TDiff
      for( int i = 0; i < dig.GetNumRawEvent() - 1; i++){
        ULong64_t timeDiff = dig.GetRawTimeStamp(i+1) - dig.GetRawTimeStamp(i);
        gp.FillTimeDiff((float)timeDiff * ch2ns);
      }
      
      file.Append();
      double fileSize = file.GetFileSize() ;

      int buildID = dig.BuildEvent(isDebug); 
      gp.ZeroCountOfCut();
      if( dig.GetNumRawEvent() > 0  && buildID == 1 ) {
        for( int i = 0; i < dig.GetEventBuiltCount(); i++){          
          file.FillTree(dig.GetChannel(i), dig.GetEnergy(i), dig.GetTimeStamp(i));
          gp.Fill(dig.GetEnergy(i));
        }
      }
      gp.FillHit(dig.GetNChannelEvent());
      
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
      //double totalRate = dig.GetEventBuiltCount()*1.0/timeRangeSec;
      double totalRate = dig.GetNChannelEvent(5)*1.0/timeRangeSec;
      printf(" Rate( all) :%7.2f pps\n", totalRate);
      if( totalRate >= 0. ) gp.FillRateGraph((CurrentTime - StartTime)/1e3, totalRate);
      
      string tag = "tag=" + location;
      WriteToDataBase(databaseName, "totalRate", tag, totalRate);
      
      if(gp.IsCutFileOpen()){
        for( int i = 0 ; i < gp.GetNumCut(); i++ ){
          double count = gp.GetCountOfCut(i)*1.0/timeRangeSec;
          printf(" Rate(%4s) :%7.2f pps\n", gp.GetCutName(i).Data(), count);
          //============= write to database 
          WriteToDataBase(databaseName, gp.GetCutName(i).Data(), tag, count);
        }
      }
      
      //Draw histogram 
      gp.Draw();
      
      // wirte histogram into tree
      file.WriteHistogram(gp.GethdEtotE());
      file.WriteHistogram(gp.GethE());
      file.WriteHistogram(gp.GethdE());
      file.WriteHistogram(gp.GethtotE());
      file.WriteHistogram(gp.GethdEE());
      file.WriteHistogram(gp.GethTDiff());
      file.WriteHistogram(gp.GetRateGraph(), "rateGraph");
      
      file.Close();
      
      dig.ClearData();
      
      PreviousTime = CurrentTime;
      
    }

  } // End of readout loop
  
  if( isSaveRaw ) {
    rawFile->Close();
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

static void raw(void){
  static char init;
  struct termios new_kbd_mode;

  if(init) return;
  /* put keyboard (stdin, actually) in raw, unbuffered mode */
  tcgetattr(0, &g_old_kbd_mode);
  memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
  new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
  new_kbd_mode.c_cc[VTIME] = 0;
  new_kbd_mode.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &new_kbd_mode);
  /* when we exit, go back to normal, "cooked" mode */
  atexit(cooked);

  init = 1;
}

int getch(void){
  unsigned char temp;
  raw();
  /* stdin = fd 0 */
  if(read(0, &temp, 1) != 1)
  return 0;
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
    printf("select() failed in kbhit()\n");
    exit(1);
  }
  return (status);
}
