/******************************************************************************
*  This program is built upon the sample from CAEN. The use of CERN/ROOT
*  Library is written by myself.
*
*
*  Tsz Leung (Ryan) TANG, Oct 1st, 2019
*  ttang@anl.gov
******************************************************************************/

//TODO loading setting for detector (save as some files?)
//TODO write waveForm into File
//TODO push signal channel to Grafana
//TODO waveform display, polarity seems reverse.
//TODO digitizer output array of channel setting, feed to GenericPlan, trapezoidal filter
//TODO more online control of the digitizer.
//TODO FULL GUI? Qt? EPICS+EDM?

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
//#include "../Class/IsoDetect.h"
#include "../Class/HelioArray.h"
#include "../Class/MCPClass.h"

using namespace std;

#define MaxNChannels 8

//========== General setting , there are the most general setting that should be OK for all experiment.
int updatePeriod = 1000; ///Table, tree, Plots update period in mili-sec.
bool isSaveRaw = false;  /// saving Raw data
TString databaseName="RAISOR_exit"; ///database
string location;
bool  QuitFlag = false;

/* ###########################################################################
*  Functions
*  ########################################################################### */

long get_time();
static struct termios g_old_kbd_mode;
static void cooked(void);  ///set keyboard behaviour as wait-for-enter
static void uncooked(void);  ///set keyboard behaviour as immediate repsond
static void raw(void);
int getch(void);
int keyboardhit();
void WriteToDataBase(TString databaseName, TString seriesName, TString tag, float value);
bool isIntegrateWave = false;
bool isTimedACQ = false;
int timeLimitSec ;

void PrintCommands(){
  printf("\n");
  printf("\e[96m=============  Command List  ===================\e[0m\n");
  printf("s ) Start acquisition   z ) Change Threhsold\n");
  printf("a ) Stop acquisition    k ) Change Dynamic Range\n");
  printf("c ) Cuts Creator        t ) Change Coincident Time Window\n");
  printf("q ) Quit                y ) Clear histograms\n");
  printf("                        r ) Change dE E range\n");
  printf("                        l ) Load setting of a channel\n");
  printf("d ) List Mode           p ) Print Channel setting\n");
  printf("w ) Wave Mode           o ) Print Channel threshold and DynamicRange\n");
  printf("i ) integrate-wave      T ) timed ACQ\n");
}

void PrintTrapezoidCommands(){
  printf("\n");
  printf("\e[96m=============  Trapezoid Setting  ===================\e[0m\n");
  printf("r) rise time[ns] \n");
  printf("t) flat-top [ns] \n");
  printf("f) decay time[ns]\n");
  printf("b) base line end time [ns]\n");
  printf("------------------------------------------------------------\n");
  ///printf("s ) Start acquisition  \n");
  ///printf("a ) Stop acquisition   \n");
  ///printf("d ) List Mode           p ) Print Channel setting\n");
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

  if( argc != 3 && argc != 4 && argc != 5 ) {
    printf("usage:\n");
    printf("                + use DetectDigitizer   \n");
    printf("                |\n");
    printf("$./BoxScore  boardID location (tree.root) (debug)\n");
    printf("                         | \n");
    printf("                         +-- testing (all ch)\n");
    printf("                         +-- exit (dE = 0 ch, E = 3 ch)\n");
    printf("                         +-- cross (dE = 1 ch, E = 4 ch)\n");
    printf("                         +-- ZD (zero-degree) (dE = 2 ch, E = 5 ch)\n");
    printf("                         +-- XY (Helios target XY) \n");
    ///printf("                         +-- iso (isomer with Glover Ge detector) \n");
    printf("                         +-- IonCh (IonChamber) (dE = 4 ch, E = 7 ch) \n");
    printf("                         +-- array (single Helios array) \n");
    printf("                         +-- MCP (Micro Channel Plate) \n");
    return -1;
  }

  TString cutopt = "RECREATE"; // by default
  TString cutFileName; cutFileName = "data/cutsFile.root"; // default
  TString archiveCutFile;

  const int nInput = argc;
  const int boardID = atoi(argv[1]);
  string location = argv[2];

  TString rootFileName;
  if( argc >= 4 ) rootFileName = argv[3];
  bool isDebug= false;
  if( argc >= 5 ) isDebug = atoi(argv[4]);

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
    gp->SetChannelMask(0,0,0,0,0,0,0,1);
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
  //  gp = new IsoDetect();
  }else if ( location == "IonCh"){
    gp = new GenericPlane();
    gp->SetChannelMask(1,0,0,1,0,0,0,0);
    gp->SetdEEChannels(4, 7);
    gp->SetNChannelForRealEvent(2);
  }else if ( location == "array" ){
    gp = new HelioArray();
  }else if ( location == "MCP"){
    gp = new MicroChannelPlate();
  }else{
    printf(" no such plane. exit. \n");
    return 0;
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

  gp->SetCanvasTitleDivision(location + " | " + rootFileName);
  gp->SetChannelGain(dig.GetChannelGain(), dig.GetInputDynamicRange(), dig.GetNChannel());
  gp->SetCoincidentTimeWindow(dig.GetCoincidentTimeWindow());
  gp->SetChannelsPlotRange(dig.GetChannelsPlotRange());
  gp->SetGenericHistograms(); ///must be after SetChannelGain
  for( int ch = 0; ch < MaxNChannels ; ch++){
    gp->SetRiseTime(ch, dig.GetChannelRiseTime(ch));
    gp->SetFlatTop(ch, dig.GetChannelFlatTop(ch));
    gp->SetFallTime(ch, dig.GetChannelDecay(ch));
  }

  ///things for derivative of GenericPlane
  if( gp->GetClassID() != 0  ) gp->SetOthersHistograms();

  //====== load cut and Draw
  gp->LoadCuts(cutFileName);
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
      if( c == 'q'){ //========== quit
        QuitFlag = true;
        if( gp->IsCutFileOpen() ) {
          file.Append();
          file.WriteObjArray(gp->GetCutList());
          file.Close();
        }
      }
      if( c == 'p'){ //========== read channel setting form digitizer
        dig.StopACQ();
        for( int id = 0 ; id < MaxNChannels ; id++ ) {
          if (ChannelMask & (1<<id)) dig.GetChannelSetting(id);
        }
      }
      if( c == 's'){ //========== start acquisition
        gROOT->ProcessLine("gErrorIgnoreLevel = -1;");
        if( StartTime == 0 ) StartTime = get_time();
        dig.StartACQ();
      }
      if( c == 'a'){ //========== stop acquisition
        dig.StopACQ();
        dig.ClearRawData();
        StopTime = get_time();
        printf("========== Duration : %u msec\n", StopTime - StartTime);
      }
      if( c == 'T'){ //========== Timed acquisition
        dig.StopACQ();
        dig.ClearRawData();
        cooked(); ///set keyboard need enter to responds
        printf("Timed ACQ, for how long [sec] ? ");
        int temp = scanf("%d", &timeLimitSec);
        uncooked();
        StartTime = get_time();
        isTimedACQ  = true;
        printf("ACQ for %d sec\n", timeLimitSec);
        dig.StartACQ();
      }
      if( c == 'z'){ //========== Change threshold
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
        uncooked();
      }
      if( c == 'k'){ //========== Change Dynamic Range
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
        uncooked();
      }
      if( c == 'o'){ //========== Print threshold and Dynamic Range
        dig.StopACQ();
        dig.ClearRawData();
        dig.PrintThresholdAndDynamicRange();
      }
      if( c == 'l'){ //========== load channel setting from a file
        dig.StopACQ();
        dig.ClearRawData();
        cooked();
        printf("============ Change channel setting from a file\n");
        int ch;
        printf("Which Channel [%s] ? ", dig.GetChannelMaskString().c_str());
        int temp = scanf("%d", &ch);
        char loadfile[100];
        if( dig.GetChannelMask() & ( 1 << ch) ){
           printf("Change channel-%d from file (e.g. %d/setting_0.txt)? ", ch, dig.GetSerialNumber());
           temp = scanf("%s", loadfile);
           printf("----> load from %s\n", loadfile);
           dig.LoadChannelSetting(ch, loadfile);
           int ret = dig.ProgramChannels();
           printf("==================");
           ret == 0 ? printf(" Changed.\n") : printf("Fail.\n");
        }else{
           printf("Channel-%d is disabled.", ch);
        }
        uncooked();
      }
      if( c == 't' && dig.GetAcqMode() == "list"){ //========== Change coincident time window
        dig.StopACQ();
        dig.ClearRawData();
        cooked();
        int coinTime;
        printf("\nChange coincident time window from \e[33m%d\e[0m ns to ? ", dig.GetCoincidentTimeWindow());
        int temp = scanf("%d", &coinTime);
        dig.SetCoincidentTimeWindow(coinTime);
        gp->SetCoincidentTimeWindow(coinTime);
        printf("Done, the coincident time window is now \e[33m%d\e[0m.\n", dig.GetCoincidentTimeWindow());
        gp->Draw();
        uncooked();
      }
      if( c == 'w'){ //========== wave form mode
        if( dig.GetAcqMode() == "mixed" && isIntegrateWave == false){
           printf("Already in mixed mode\n");
        }else{
           dig.StopACQ();
           dig.ClearRawData();
           printf("\n\n##################################\n");
           //cooked();
           int length = 4000; // in ch
           //printf("Change to read Wave Form, Set Record Length [ns]? ");
           //int temp = scanf("%d", &length);
           dig.SetAcqMode("mixed", length);
           gp->SetWaveCanvas(length);
           //dig.StartACQ();
           isIntegrateWave = false;
           //uncooked();
        }
        StartTime = get_time();
      }
      if( c == 'i'){ //========== integrate waveform mode
        dig.StopACQ();
        dig.ClearRawData();
        printf("\n\n###############################\n");
        int length = 4000;
        dig.SetAcqMode("mixed", length);
        isIntegrateWave = true;
      }
      if( c == 'd'){ //========== Change coincident time window
        if( dig.GetAcqMode() == "list" ) {
           printf("Already in list mode\n");
        }else{
           dig.StopACQ();
           dig.ClearRawData();
           printf("Change to List mode.\n");
           dig.SetAcqMode("list", 2000); /// for list mode, recordLength is dummy
           gp->SetCanvasTitleDivision(rootFileName);
           gp->Draw();
        }
      }
      if( c == 'y' && dig.GetAcqMode() == "list"){ //========== reset histograms, only for list mode
        gp->ClearHistograms();
        gp->Draw();
      }
      if( c == 'r' && dig.GetAcqMode() == "list"){ //========== Change dE E range, only for list mode
        dig.StopACQ();
        dig.ClearRawData();
        cooked();
        int option;
        printf("\e[32m=================== Change dE or E Range\e[0m\n");
        printf("Change dE or E range ? ( 1 for dE, 2 for E, other = cancel ) ");
        int temp = scanf("%d", &option);
        if( option == 1 ) {
          int x1, x2;
          int * rangedE = gp->GetdERange();
          printf("--------- Change dE range. (%d, %d)\n", rangedE[0], rangedE[1]);
          printf("min ? ");
          temp = scanf("%d", &x1);
          printf("max ? ");
          temp = scanf("%d", &x2);
          gp->SetdERange(x1, x2);
          dig.SetChannelPlotRange(gp->GetdEChannel(), folder, x1, x2);
        }else if (option == 2){
          int x1, x2;
          int * rangeE = gp->GetERange();
          printf("--------- Change E range. (%d, %d)\n", rangeE[0], rangeE[1]);
          printf("min ? ");
          temp = scanf("%d", &x1);
          printf("max ? ");
          temp = scanf("%d", &x2);
          gp->SetERange(x1, x2);
          dig.SetChannelPlotRange(gp->GetEChannel(), folder, x1, x2);
        }
        gp->SetHistogramsRange();
        gp->Draw();
        PrintCommands();
        uncooked();
      }
      if( c == 'c' && dig.GetAcqMode() == "list"){ //========== pause and make cuts, only for list mode
        dig.StopACQ();
        dig.ClearRawData();

        cooked();
        int opt;
        printf("Do you want to [1] update the current cut file or [2] create a new one?\n"); //GLW
        int temp = scanf("%d", &opt);
        if(opt==1){
           cutopt = "UPDATE";
        }else if(opt==2){
           cutopt = "RECREATE";
           TFile * cutcheck = (TFile *)gROOT->GetListOfFiles()->FindObject(cutFileName);
           if(cutcheck != nullptr){
              if(cutcheck->IsOpen()){
                 cutcheck->Close();
                 archiveCutFile.Form("data/ArchiveCut_%4d%02d%02d_%02d%02d.root", year, month, day, hour, minute);
                 system(("cp "+cutFileName+" "+archiveCutFile));
                 printf("\n Save the old cutFile.root to %s \n", archiveCutFile.Data());
              }else{
                 printf("cutsFile.root isn't open.\n");
              }
              printf("No cutsFile.root is open.\n");
           }
        }else{
           cutopt = "UPDATE";
           printf("defaulting to updating the previous cutfile.\n");
        }

        int mode = gp->GetMode();
        float * chGain = dig.GetChannelGain();
        int * rangeE = gp->GetERange();
        int * rangeDE = gp->GetdERange();
        int chE = gp->GetEChannel();
        int chDE = gp->GetdEChannel();

        string expression = "./CutsCreator " + (string)rootFileName + " " ;
        expression = expression + (string)cutopt + " ";
        expression = expression + to_string(chDE) + " ";
        expression = expression + to_string(chE) + " ";
        expression = expression + to_string(rangeDE[0]) + " ";
        expression = expression + to_string(rangeDE[1]) + " ";
        expression = expression + to_string(rangeE[0]) + " ";
        expression = expression + to_string(rangeE[1]) + " ";
        expression = expression + to_string(mode) + " ";
        expression = expression + to_string(chGain[chDE]) + " ";
        expression = expression + to_string(chGain[chE]) + " ";
        printf("%s\n", expression.c_str());
        system(expression.c_str());

        gp->LoadCuts(cutFileName);
        gp->Draw();
        uncooked();
      }
      if( (c == 'r' || c == 't' || c == 'f' ) && dig.GetAcqMode() == "mixed"){  //========== Set Trapezoid rise time, only for wave mode
        dig.StopACQ();
        dig.ClearRawData();
        cooked();
        int ch;
        printf("Which Channel [%s] ? ", dig.GetChannelMaskString().c_str());
        int temp = scanf("%d", &ch);
        int old_setting ;
        int setting; 
        string settingType;
        if( c == 'r') {
          settingType = "Rise Time";
          old_setting = gp->GetRiseTime(ch);
        }else if( c == 't') {
          settingType = "Flat Top";
          old_setting = gp->GetFlatTop(ch);
        }else if( c == 'f') {
          settingType = "Decay/Pole-Zero";
          old_setting = gp->GetFallTime(ch);
        }
        
        printf("Present %s %d [ch] = %d [ns], New setting in [ch] ?", settingType.c_str(), old_setting, old_setting * 2);
        temp = scanf("%d", &setting);
        setting = setting/8*8;
        if( c == 'r') {
          gp->SetRiseTime(ch, setting);
          dig.SetChannelRiseTime(ch, folder, setting);
        }else if( c == 't') {
          gp->SetFlatTop(ch, setting);
          dig.SetChannelFlatTop(ch, folder, setting);
        }else if( c == 'f') {
          gp->SetFallTime(ch, setting);
          dig.SetChannelDecay(ch, folder, setting);
        }
        
        uncooked();
        dig.StartACQ();
      }
      if( c == 'b' && dig.GetAcqMode() == "mixed"){  //========== Set Trapezoid baseline estimation, only for wave mode
        cooked();
        int ch;
        printf("Which Channel [%s] ? ", dig.GetChannelMaskString().c_str());
        int temp = scanf("%d", &ch);
        int old_setting = gp->GetBaseLineEnd(ch);
        int setting; 
        printf("Present Base-Line-End %d [ch] = %d [ns], New setting in [ch] ?", old_setting, old_setting * 2);
        temp = scanf("%d", &setting);
        gp->SetBaseLineEnd(ch, setting);
        uncooked();
      }
      
      PrintCommands();
      
      if( dig.GetAcqMode() == "mixed" ) PrintTrapezoidCommands();
    }//------------ End of keyboardHit

    if (!dig.IsRunning()) {
      sleep(0.01); /// pause 10 mili-sec
      continue;
    }

    ///the digitizer will output a channel after a channel.,
    ///so data should be read as fast as possible, that the digitizer will not store any data.
    dig.ReadData(isDebug);
    if( dig.GetAcqMode() == "mixed" ) {
       gp->FillWaves(dig.GetWaveFormLengths(), dig.GetWaveForms());
       if( isIntegrateWave ){
         gp->FillEnergies(gp->GetWaveEnergy());
         gp->Draw();
         file.Append();
         file.FillTreeWave(gp->GetWaveForm(), gp->GetWaveEnergy());
         file.Close();
       }else{
         gp->DrawWaves();
       }

    }

    if( isSaveRaw ) {
      ///for( int i = 0 ; i < dig.GetNumRawEvent(); i++){
      ///  rawFile.FillTree();
      ///}
    }

    //##################################################################
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PreviousTime; /// milliseconds

    if ( ElapsedTime > updatePeriod && dig.GetAcqMode() == "mixed" )  {
       system("clear");
       PrintCommands();
       printf("\n");
       PrintTrapezoidCommands();
       printf("\n\n");
       printf("Time elapsed: %f sec\n", (CurrentTime - StartTime)/1000. );
       dig.PrintReadStatistic();
       PreviousTime = CurrentTime;

      if( isTimedACQ && CurrentTime - StartTime > timeLimitSec * 1000) {
        dig.StopACQ();
        dig.ClearRawData();
        break;
      }
    }

    if (ElapsedTime > updatePeriod && dig.GetAcqMode() == "list") {
      if( isTimedACQ && CurrentTime - StartTime > timeLimitSec) {
        dig.StopACQ();
        dig.ClearRawData();
        break;
      }
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
      if( totalRate >= 0.) gp->FillRateGraph((CurrentTime - StartTime)/1e3, totalRate);
      WriteToDataBase(databaseName, "totalRate", tag, totalRate);

      /// for isomer
      if( gp->GetClassID() == 2 ) {
        WriteToDataBase( databaseName, "G1", tag, gp->GetG1Count()/timeRangeSec);
        WriteToDataBase( databaseName, "G2", tag, gp->GetG2Count()/timeRangeSec);
        WriteToDataBase( databaseName, "G3", tag, gp->GetG3Count()/timeRangeSec);
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
    //printf("%s \n", databaseStr.Data());
    system(databaseStr.Data());
  }
}
