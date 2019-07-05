/******************************************************************************
*  This program is build upon the sample from CAEN. The use of CERN/ROOT 
*  Library is wrote by myself.
* 
*  User can change the general setting from line 50 to line 67
* 
*  Tsz Leung (Ryan) TANG, Feb 23rd, 2019
*  ttang@anl.gov
******************************************************************************/

#include <CAENDigitizer.h>

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

#include "Functions.h"

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

using namespace std;

#define MaxNChannels 8

//TODO 1) change DCoffset, pulseParity to channel
//TODO 2) change the tree structure to be like HELIOS

//========== General setting;
unsigned long long int ch2ns = 2.;
float DCOffset = 0.2;
bool PositivePulse = true;
uint RecordLength = 20000;   // Num of samples of the waveforms (only for waveform mode)
uint PreTriggerSize = 2000;
uint ChannelMask = 0x03;   // Channel enable mask, 0x01, only frist channel, 0xff, all channel

int updatePeriod = 1000; //Table, tree, Plots update period in mili-sec.
int CoincidentWindow = 300; // real time [nano-sec]

int chE = 0;   //channel ID for E
int chDE = 1;  //channel ID for dE
int chTAC = 7; //channel ID for TAC

int rangeDE[2] = {0, 5000}; // range for dE
int rangeE[2] = {0, 5000};  // range for E
double rangeTime = 5e7;  // range for Tdiff

float RateWindow = 10.; // sec

bool isSaveRaw = false; // saving Raw data

string location;

//database
TString databaseName="RAISOR_exit";

//========= Histogram
TCanvas * cCanvas = NULL;
TCanvas * cCanvasAux = NULL;
TH1F * hE = NULL;
TH1F * htotE = NULL;
TH1F * hdE = NULL;
TH2F * hdEtotE = NULL; 
TH2F * hdEE = NULL; 
TH1F * hTDiff = NULL;
TH2F * htotETAC = NULL;
//======== Rate Graph
TMultiGraph * rateGraph = NULL;
TGraph * graphRate = NULL;
TGraph ** graphRateCut = NULL; 
TLegend * legend = NULL ; 
TGraph * rangeGraph = NULL;

TMultiGraph * fullRateGraph = NULL;
TGraph * fullGraphRate = NULL;
TGraph ** fullGraphRateCut = NULL; 
TLegend * fullLegend = NULL ; 

//======== TCutG
TFile * fCut = NULL;
TString cutName;
TCutG* cutG; //!
TObjArray * cutList = NULL;
Bool_t isCutFileOpen = false;
int numCut = 0 ;
vector<int> countFromCut;

bool  QuitFlag = false;

/* ###########################################################################
*  Functions
*  ########################################################################### */
void WriteToDataBase(TString databaseName, TString seriesName, TString tag, float value){
    TString databaseStr;
    databaseStr.Form("influx -execute \'insert %s,%s value=%f\' -database=%s", seriesName.Data(), tag.Data(), value, databaseName.Data());
    //printf("%s \n", databaseStr.Data());
    system(databaseStr.Data());
}

void ReadGeneralSetting(string fileName){

  const int numPara = 17;
  
  ifstream file_in;
  file_in.open(fileName.c_str(), ios::in);
  
  printf("====================================== \n");

  if( !file_in){
    printf("====== Using Built-in General Setting.\n");
  }else{
    printf("====== Reading General Setting from  %s.\n", fileName.c_str());
    string line;
    int count = 0;
    while( file_in.good()){
      getline(file_in, line);
      size_t pos = line.find("//");
      if( pos > 1 ){
        if( count > numPara - 1) break;
        
        if( count == 0 )  DCOffset = atof(line.substr(0, pos).c_str());
        if( count == 1 )  {
          if( line.substr(0, 4) == "true" ) {
            PositivePulse = true;
          }else{
            PositivePulse = false;
          }
        }
        if( count == 2  )   RecordLength = atoi(line.substr(0, pos).c_str());
        if( count == 3  ) PreTriggerSize = atoi(line.substr(0, pos).c_str());
        if( count == 4  )   updatePeriod = atoi(line.substr(0, pos).c_str());
        if( count == 5  ) CoincidentWindow = atof(line.substr(0, pos).c_str());
        if( count == 6  )  rangeE[0] = atoi(line.substr(0, pos).c_str());
        if( count == 7 )  rangeE[1] = atoi(line.substr(0, pos).c_str());
        if( count == 8 ) rangeDE[0] = atoi(line.substr(0, pos).c_str());
        if( count == 9 ) rangeDE[1] = atoi(line.substr(0, pos).c_str());
        if( count == 10 )  rangeTime = atof(line.substr(0, pos).c_str());
        if( count == 11 )  RateWindow = atof(line.substr(0, pos).c_str());
        if( count == 12 )  {
          if( line.substr(0, 4) == "true" ) {
            isSaveRaw = true;
          }else{
            isSaveRaw = false;
          }
        }
        if( count == 13 )  databaseName = line.substr(0, pos).c_str();
        count ++;
      }
    }
    
    //print setting
    printf(" %-20s  %.3f (0x%04x)\n", "DC offset", DCOffset, uint( 0xffff * DCOffset ));
    printf(" %-20s  %s\n", "Positive Pulse", PositivePulse ? "true" : "false" );
    printf(" %-20s  %d ch\n", "Record Lenght", RecordLength);
    printf(" %-20s  %d ch\n", "Pre-Trigger Size", PreTriggerSize);
    printf(" %-20s  %d msec\n", "Update period", updatePeriod);
    printf(" %-20s  %.3f ns\n", "Coincident windows", CoincidentWindow * 1.0 );
    printf(" %-20s  %.3f ns\n", "tDiff range", rangeTime);
    printf(" %-20s  %s\n", "Is saving Raw", isSaveRaw ? "true" : "false");
    printf(" %-20s  %s\n", "DataBase use", databaseName.Data());
    printf("====================================== \n");
    
  }

  return;
}

int* ReadChannelSetting(int ch, string fileName){

  const int numPara = 20;
  int * para = new int[numPara];
  
  ifstream file_in;
  file_in.open(fileName.c_str(), ios::in);

  if( !file_in){
    printf("channel: %d | default.\n", ch);
    para[0] = 100;      // Trigger Threshold (in LSB)
    para[1] = 1200;     // Trigger Hold Off
    para[2] = 4;        // Trigger Filter smoothing factor (number of samples to average for RC-CR2 filter) Options: 1; 2; 4; 8; 16; 32
    para[3] = 200;      // Input Signal Rise time (ns) 
    
    para[4] = 3000;     // Trapezoid Rise Time (ns) 
    para[5] = 900;      // Trapezoid Flat Top  (ns) 
    para[6] = 50000;    // Decay Time Constant (ns) 
    para[7] = 500;      // Flat top delay (peaking time) (ns) 
    para[8] = 0;        // Peak mean (number of samples to average for trapezoid height calculation). Options: 0-> 1 sample; 1->4 samples; 2->16 samples; 3->64 samples
    para[9] = 2000;    // peak holdoff (ns)
    
    para[10] = 4;        // number of samples for baseline average calculation. Options: 1->16 samples; 2->64 samples; 3->256 samples; 4->1024 samples; 5->4096 samples; 6->16384 samples
    para[11] = 0;       // input dynamic range, 0 = 2 Vpp, 1 = 0.5 Vpp

    para[12] = 10;      // Energy Fine gain
    para[13] = 500;     // Baseline holdoff (ns)        
    para[14] = 1.0;     // Energy Normalization Factor
    para[15] = 0;       // decimation (the input signal samples are averaged within this number of samples): 0 ->disabled; 1->2 samples; 2->4 samples; 3->8 samples
    para[16] = 0;       // decimation gain. Options: 0->DigitalGain=1; 1->DigitalGain=2 (only with decimation >= 2samples); 2->DigitalGain=4 (only with decimation >= 4samples); 3->DigitalGain=8( only with decimation = 8samples).
    para[17] = 0;       // Enable Rise time Discrimination. Options: 0->disabled; 1->enabled
    para[18] = 100;     // Rise Time Validation Window (ns)
    
    para[19] = -1;      // gain of the channel; if -1, default based on input-dynamic range;
  }else{
    printf("channel: %d | %s.\n", ch, fileName.c_str());
    string line;
    int count = 0;
    while( file_in.good()){
      getline(file_in, line);
      size_t pos = line.find("//");
      if( pos > 1 ){
        if( count > numPara - 1) break;
        para[count] = atoi(line.substr(0, pos).c_str());
        //printf("%d | %d \n", count, para[count]);
        count ++;
      }
    }
  }

  return para;
}

void GetChannelSetting(int handle, int ch){
  
  uint32_t * value = new uint32_t[8];
  printf("================================================\n");
  printf("================ Getting setting for channel %d \n", ch);
  printf("================================================\n");
  //DPP algorithm Control
  CAEN_DGTZ_ReadRegister(handle, 0x1080 + (ch << 8), value);
  printf("                          32  28  24  20  16  12   8   4   0\n");
  printf("                           |   |   |   |   |   |   |   |   |\n");
  cout <<" DPP algorithm Control  : 0b" << bitset<32>(value[0]) << endl;
  
  int trapRescaling = int(value[0]) & 31 ;
  int polarity = int(value[0] >> 16); //in bit[16]
  int baseline = int(value[0] >> 20) ; // in bit[22:20]
  int NsPeak = int(value[0] >> 12); // in bit[13:12]
  //DPP algorithm Control 2
  CAEN_DGTZ_ReadRegister(handle, 0x10A0 + (ch << 8), value);
  cout <<" DPP algorithm Control 2: 0b" << bitset<32>(value[0]) << endl;
  
  printf("* = multiple of 8 \n");
  
  printf("==========----- input \n");
  CAEN_DGTZ_ReadRegister(handle, 0x1020 + (ch << 8), value); printf("%20s  %d \n", "Record Length",  value[0] * 8); //Record length
  CAEN_DGTZ_ReadRegister(handle, 0x1038 + (ch << 8), value); printf("%20s  %d \n", "Pre-tigger",  value[0] * 4); //Pre-trigger
  printf("%20s  %s \n", "polarity",  (polarity & 1) ==  0 ? "Positive" : "negative"); //Polarity
  printf("%20s  %.0f sample \n", "Ns baseline",  pow(4, 1 + baseline & 7)); //Ns baseline
  CAEN_DGTZ_ReadRegister(handle, 0x1098 + (ch << 8), value); printf("%20s  %.2f %% \n", "DC offset",  value[0] * 100./ int(0xffff) ); //DC offset
  CAEN_DGTZ_ReadRegister(handle, 0x1028 + (ch << 8), value); printf("%20s  %.1f Vpp \n", "input Dynamic",  value[0] == 0 ? 2 : 0.5); //InputDynamic
  
  printf("==========----- discriminator \n");
  CAEN_DGTZ_ReadRegister(handle, 0x106C + (ch << 8), value); printf("%20s  %d LSB\n", "Threshold",  value[0]); //Threshold
  CAEN_DGTZ_ReadRegister(handle, 0x1074 + (ch << 8), value); printf("%20s  %d ns \n", "trigger hold off *",  value[0] * 8); //Trigger Hold off
  CAEN_DGTZ_ReadRegister(handle, 0x1054 + (ch << 8), value); printf("%20s  %d sample \n", "Fast Dis. smoothing",  value[0] *2 ); //Fast Discriminator smoothing
  CAEN_DGTZ_ReadRegister(handle, 0x1058 + (ch << 8), value); printf("%20s  %d ch \n", "Input rise time *",  value[0] * 8); //Input rise time
  
  printf("==========----- Trapezoid \n");
  CAEN_DGTZ_ReadRegister(handle, 0x1080 + (ch << 8), value); printf("%20s  %d bit = Floor( rise * decay / 64 )\n", "Trap. Rescaling",  trapRescaling ); //Trap. Rescaling Factor
  CAEN_DGTZ_ReadRegister(handle, 0x105C + (ch << 8), value); printf("%20s  %d ns \n", "Trap. rise time *",  value[0] * 8 ); //Trap. rise time
  CAEN_DGTZ_ReadRegister(handle, 0x1060 + (ch << 8), value); 
  int flatTopTime = value[0] * 8;
  printf("%20s  %d ns \n", "Trap. flat time *",  flatTopTime); //Trap. flat time
  CAEN_DGTZ_ReadRegister(handle, 0x1020 + (ch << 8), value); printf("%20s  %d ns \n", "Trap. pole zero *",  value[0] * 8); //Trap. pole zero
  CAEN_DGTZ_ReadRegister(handle, 0x1068 + (ch << 8), value); printf("%20s  %d ns \n", "Decay time *",  value[0] * 8); //Trap. pole zero
  CAEN_DGTZ_ReadRegister(handle, 0x1064 + (ch << 8), value); printf("%20s  %d ns = %.2f %% \n", "peaking time *",  value[0] * 8, value[0] * 800. / flatTopTime ); //Peaking time
  printf("%20s  %.0f sample\n", "Ns peak",  pow(4, NsPeak & 3)); //Ns peak
  CAEN_DGTZ_ReadRegister(handle, 0x1078 + (ch << 8), value); printf("%20s  %d ns \n", "Peak hole off*",  value[0] * 8 ); //Peak hold off
  
  printf("==========----- Other \n");
  CAEN_DGTZ_ReadRegister(handle, 0x104C + (ch << 8), value); printf("%20s  %d \n", "Energy fine gain",  value[0]); //Energy fine gain
    
}

int ProgramDigitizer(int handle, DigitizerParams_t Params, CAEN_DGTZ_DPP_PHA_Params_t DPPParams, int inputDynamicRange[], int energyFineGain []){
    /* This function uses the CAENDigitizer API functions to perform the digitizer's initial configuration */
    int i, ret = 0;

    /* Reset the digitizer */
    ret |= CAEN_DGTZ_Reset(handle);

    if (ret) {
        printf("ERROR: can't reset the digitizer.\n");
        return -1;
    }
    
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8000, 0x01000114);  // Channel Control Reg (indiv trg, seq readout) ??

    /* Set the DPP acquisition mode
    This setting affects the modes Mixed and List (see CAEN_DGTZ_DPP_AcqMode_t definition for details)
    CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly        Only energy (DPP-PHA) or charge (DPP-PSD/DPP-CI v2) is returned
    CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly        Only time is returned
    CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime    Both energy/charge and time are returned
    CAEN_DGTZ_DPP_SAVE_PARAM_None            No histogram data is returned */
    ret |= CAEN_DGTZ_SetDPPAcquisitionMode(handle, Params.AcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
    
    // Set the digitizer acquisition mode (CAEN_DGTZ_SW_CONTROLLED or CAEN_DGTZ_S_IN_CONTROLLED)
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
    
    // Set the number of samples for each waveform
    ret |= CAEN_DGTZ_SetRecordLength(handle, Params.RecordLength);

    // Set the I/O level (CAEN_DGTZ_IOLevel_NIM or CAEN_DGTZ_IOLevel_TTL)
    ret |= CAEN_DGTZ_SetIOLevel(handle, Params.IOlev);

    /* Set the digitizer's behaviour when an external trigger arrives:

    CAEN_DGTZ_TRGMODE_DISABLED: do nothing
    CAEN_DGTZ_TRGMODE_EXTOUT_ONLY: generate the Trigger Output signal
    CAEN_DGTZ_TRGMODE_ACQ_ONLY = generate acquisition trigger
    CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT = generate both Trigger Output and acquisition trigger

    see CAENDigitizer user manual, chapter "Trigger configuration" for details */
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

    // Set the enabled channels
    ret |= CAEN_DGTZ_SetChannelEnableMask(handle, Params.ChannelMask);

    // Set how many events to accumulate in the board memory before being available for readout
    ret |= CAEN_DGTZ_SetDPPEventAggregation(handle, Params.EventAggr, 0);
    
    /* Set the mode used to syncronize the acquisition between different boards.
    In this example the sync is disabled */
    ret |= CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);
    
    // Set the DPP specific parameters for the channels in the given channelMask
    ret |= CAEN_DGTZ_SetDPPParameters(handle, Params.ChannelMask, &DPPParams);
    
    // Set Extras 2 to enable, this override Accusition mode, focring list mode
    uint32_t value = 0x10E0114;
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8000 , value );
    
    for(i=0; i<MaxNChannels; i++) {
        if (Params.ChannelMask & (1<<i)) {
            // Set a DC offset to the input signal to adapt it to digitizer's dynamic range
            //ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, 0x3333); // 20%
            ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, uint( 0xffff * DCOffset ));
            
            // Set the Pre-Trigger size (in samples)
            ret |= CAEN_DGTZ_SetDPPPreTriggerSize(handle, i, PreTriggerSize);
            
            // Set the polarity for the given channel (CAEN_DGTZ_PulsePolarityPositive or CAEN_DGTZ_PulsePolarityNegative)
            ret |= CAEN_DGTZ_SetChannelPulsePolarity(handle, i, Params.PulsePolarity);
            
            // Set InputDynamic Range
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x1028 +  (i<<8), inputDynamicRange[i]);
            
            // Set Energy Fine gain
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x104C +  (i<<8), energyFineGain[i]);
            
            // read the register to check the input is correct
            //uint32_t * value = new uint32_t[8];
            //ret = CAEN_DGTZ_ReadRegister(handle, 0x1028 + (i << 8), value);
            //printf(" InputDynamic Range (ch:%d): %d \n", i, value[0]);
        }
    }

    //ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, ANALOG_TRACE_1, CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2);
    //ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, ANALOG_TRACE_2, CAEN_DGTZ_DPP_VIRTUALPROBE_Input);
    //ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, DIGITAL_TRACE_1, CAEN_DGTZ_DPP_DIGITALPROBE_Peaking);

    if (ret) {
        printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");
        return ret;
    } else {
        return 0;
    }
}

void paintCanvas(){
  //This function is running in a parrellel thread.
  //This continously update the Root system with user input
  //avoid frozen
  do{
    //cCanvas->Modified();
    gSystem->ProcessEvents();
    //Sleep(10); // 10 mili-sec
  }while(!QuitFlag);
}

void ReadCut(TString fileName){
  
  printf("\n");
  if( rateGraph->GetListOfGraphs() != NULL) rateGraph->GetListOfGraphs()->RemoveAll();
  rateGraph->SetTitle("Beam rate [pps]; Time [sec]; Rate [pps]");
  
  rangeGraph->SetPoint(0, RateWindow + 5, 0);
  rateGraph->Add(rangeGraph);
  
  legend->Clear();
  
  fullRateGraph->SetTitle(Form("%s | Beam rate [pps] (all time); Time [sec]; Rate [pps]", location.c_str()));
  fullRateGraph->Add(rangeGraph);
  fullLegend->Clear();
  
  
  fCut = new TFile(fileName);
  isCutFileOpen = fCut->IsOpen(); 
  numCut = 0 ;
  if( isCutFileOpen ){
    cutList = (TObjArray *) fCut->FindObjectAny("cutList");
    if( cutList == NULL ) return;
    numCut = cutList->GetEntries();
    printf("=========== found %d TCutG in %s \n", numCut, fileName.Data());
    cutG = new TCutG();
    
    graphRateCut = new TGraph * [numCut];
    fullGraphRateCut = new TGraph * [numCut];
    for(int i = 0; i < numCut ; i++){
      //printf(" cut name : %s \n", cutList->At(i)->GetName());
      countFromCut.push_back(0);
        
      graphRateCut[i] = new TGraph();
      graphRateCut[i]->SetMarkerColor(i+1);
      graphRateCut[i]->SetMarkerStyle(20+i);
      graphRateCut[i]->SetMarkerSize(1);
      rateGraph->Add(graphRateCut[i]);
      legend->AddEntry(graphRateCut[i], cutList->At(i)->GetName());

      fullGraphRateCut[i] = new TGraph();
      fullGraphRateCut[i]->SetMarkerColor(i+1);
      fullGraphRateCut[i]->SetMarkerStyle(20+i);
      fullGraphRateCut[i]->SetMarkerSize(1);
      fullRateGraph->Add(fullGraphRateCut[i]);
      fullLegend->AddEntry(fullGraphRateCut[i], cutList->At(i)->GetName());
      
    }
  }else{
    rateGraph->Add(graphRate);
    legend->AddEntry(graphRate, "Total");
    
    fullRateGraph->Add(fullGraphRate);
    fullLegend->AddEntry(fullGraphRate, "Total");
    printf("=========== Cannot find TCutG in %s, file may be no exist. \n", fileName.Data());
  }
  
  //printf("====================================== \n");
}

/* ########################################################################### */
/* MAIN                                                                        */
/* ########################################################################### */
int main(int argc, char *argv[]){
    
  if( argc != 3 && argc != 4 ) {
    printf("Please input boardID and Location! (optional root file name)\n");
    printf("usage:\n");
    printf("$./BoxScore boardID Location (tree.root)\n");
    printf("                     | \n");
    printf("                     |-- exit \n");
    printf("                     |-- cross \n");
    printf("                     |-- ZD (zero-degree) \n");
    return -1;
  }
  
  const int boardID = atoi(argv[1]);
  location = argv[2];
    
  char hostname[100];
  gethostname(hostname, 100);
  
  time_t now = time(0);
  //cout << "Number of sec since January 1,1970:" << now << endl;
  tm *ltm = localtime(&now);
  
  int year = 1900 + ltm->tm_year;
  int month = 1 + ltm->tm_mon;
  int day = ltm->tm_mday;
  int hour = ltm->tm_hour;
  int minute = ltm->tm_min;
  int secound = ltm->tm_sec;

  TString rootFileName;
  rootFileName.Form("%4d%02d%02d_%02d%02d%02d%s.root", year, month, day, hour, minute, secound, location.c_str());
  if( argc == 4 ) rootFileName = argv[3];
  
  printf("******************************************** \n");
  printf("****         Real Time PID              **** \n");
  printf("******************************************** \n");
  printf(" Current DateTime : %d-%02d-%02d, %02d:%02d:%02d\n", year, month, day, hour, minute, secound);
  printf("         hostname : %s \n", hostname);
  printf("******************************************** \n");
  printf("   board ID : %d \n", boardID );
  printf("   Location : %s \n", location.c_str() );
  printf("    save to : %s \n", rootFileName.Data() );

  ReadGeneralSetting("generalSetting.txt");
  TMacro gs("generalSetting.txt");
  
  if( location == "exit" ) {
    ChannelMask = 0x89;
    chE  = 3;
    chDE = 0;
    chTAC = 7;
  }else if( location == "cross") {
    ChannelMask = 0x82;
    chE  = 4;
    chDE = 1;
    chTAC = 7;
  }else if (location == "ZD"){
    ChannelMask = 0xA4;
    chE  = 5;
    chDE = 2;
    chTAC = 7;
  }else{
    printf("============== location : %s  is UNKNOWN!! Abort!\n", location.c_str());
    return 404;
  }
  
  TApplication app ("app", &argc, argv);
  
  /* The following variable is the type returned from most of CAENDigitizer
  library functions and is used to check if there was an error in function
  execution. For example:
  ret = CAEN_DGTZ_some_function(some_args);
  if(ret) printf("Some error"); */
  //CAEN_DGTZ_ErrorCode ;
  int ret;

  /* Buffers to store the data. The memory must be allocated using the appropriate
  CAENDigitizer API functions (see below), so they must not be initialized here
  NB: you must use the right type for different DPP analysis (in this case PHA) */
  char *buffer = NULL;                                 // readout buffer
  CAEN_DGTZ_DPP_PHA_Event_t       *Events[MaxNChannels];  // events buffer

  /* The following variables will store the digitizer configuration parameters */
  CAEN_DGTZ_DPP_PHA_Params_t DPPParams;
  DigitizerParams_t Params;
  int InputDynamicRange[MaxNChannels];
  float chGain[MaxNChannels];
  for ( int i = 0; i < MaxNChannels ; i++ ) {
    InputDynamicRange[i] = 0;
    chGain[i] = 1.0;
  }
  int EnergyFinegain[MaxNChannels];

  /* Arrays for data analysis */
  //uint64_t PrevTime[MaxNChannels];
  //uint64_t ExtendedTT[MaxNChannels];
  int ECnt[MaxNChannels];
  int TrgCnt[MaxNChannels];
  int PurCnt[MaxNChannels];

  /* The following variable will be used to get an handler for the digitizer. The
  handler will be used for most of CAENDigitizer functions to identify the board */
  int handle;

  /* Other variables */
  int i, ch, ev; 
  int AcqRun = 0;
  uint32_t AllocatedSize, BufferSize;
  int Nb=0;  // buffer number
  int MajorNumber;
  uint64_t CurrentTime, PrevRateTime, ElapsedTime;
  uint64_t StartTime, StopTime;
  uint32_t NumEvents[MaxNChannels];
  CAEN_DGTZ_BoardInfo_t           BoardInfo;
  uint32_t temp;

  /* *************************************************************************************** */
  /* Set Parameters                                                                          */
  /* *************************************************************************************** */
  memset(&Params, 0, sizeof(DigitizerParams_t));
  memset(&DPPParams, 0, sizeof(CAEN_DGTZ_DPP_PHA_Params_t));
  /****************************\
  * Communication Parameters   *
  \****************************/
  Params.LinkType = CAEN_DGTZ_USB;  // Link Type
  Params.VMEBaseAddress = 0;  // For direct USB connection, VMEBaseAddress must be 0
  Params.IOlev = CAEN_DGTZ_IOLevel_NIM;
  /****************************\
  *  Acquisition parameters    *
  \****************************/
  //Params.AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;          // CAEN_DGTZ_DPP_ACQ_MODE_List or CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope
  Params.AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;             // CAEN_DGTZ_DPP_ACQ_MODE_List or CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope
  Params.RecordLength = RecordLength;                       // Num of samples of the waveforms (only for Oscilloscope mode)
  Params.ChannelMask = ChannelMask;                         // Channel enable mask, 0x01, only frist channel, 0xff, all channel
  Params.EventAggr = 1;                                     // number of events in one aggregate (0=automatic), number of event acculated for read-off
  if( PositivePulse ) {
    Params.PulsePolarity = CAEN_DGTZ_PulsePolarityPositive; // Pulse Polarity (this parameter can be individual)
  }else{
    Params.PulsePolarity = CAEN_DGTZ_PulsePolarityNegative; 
  }
  /****************************\
  *      DPP parameters        * 
  \****************************/
  TMacro chSetting[MaxNChannels];
  for(ch=0; ch<MaxNChannels; ch++) {
    if ( ch != chE && ch != chDE && ch != chTAC ) continue;
    string chSettingFileName = "setting_" + to_string(ch) + ".txt";
    int* para = ReadChannelSetting(ch, chSettingFileName);
    
    if (Params.ChannelMask & (1<<ch)) {
      chSetting[ch].ReadFile(chSettingFileName.c_str());
    }
    
    DPPParams.thr[ch] = para[0];              // Trigger Threshold (in LSB)
    DPPParams.trgho[ch] = para[1];            // Trigger Hold Off (ns)
    DPPParams.a[ch] = para[2];                // Fast Discriminator smooth, Trigger Filter smoothing factor (number of samples to a
    DPPParams.b[ch] = para[3];                // Input Signal Rise time (ns) 
    
    DPPParams.k[ch] = para[4];                // Trapezoid Rise Time (ns) 
    DPPParams.m[ch] = para[5];                // Trapezoid Flat Top  (ns) 
    DPPParams.M[ch] = para[6];                // Decay Time Constant (ns) 
    DPPParams.ftd[ch] = para[7];              // Flat top delay (peaking time?) (ns) 
    DPPParams.nspk[ch] = para[8];             // Ns peak, Peak mean (number of samples to average for trapezoid he
    DPPParams.pkho[ch] = para[9];             // peak holdoff (ns)
    
    DPPParams.nsbl[ch] = para[10];            // Ns baseline, number of samples for baseline average calculation. Opti
    InputDynamicRange[ch] = para[11];
    
    EnergyFinegain[ch] = para[12];            // Energy Fine Gain
    DPPParams.blho[ch] = para[13];            // Baseline holdoff (ns)
    DPPParams.enf[ch] = para[14]/100.;             // Energy Normalization Factor, it is float, but please us
    DPPParams.decimation[ch] = para[15];      // decimation (the input signal samples are averaged within
    DPPParams.dgain[ch] = para[16];           // digital gain. Options: 0->DigitalGain=1; 1->DigitalGa
    DPPParams.trgwin[ch] = para[17];          // Enable Rise time Discrimination. Options: 0->disabled; 1
    DPPParams.twwdt[ch] = para[18];           // Rise Time Validation Window (ns)
    
    chGain[ch] = para[19];

  }
  printf("====================================== \n");
  
  /* *************************************************************************************** */
  /* Open the digitizer and read board information                                           */
  /* *************************************************************************************** */
  CAEN_DGTZ_ErrorCode ret1 = CAEN_DGTZ_OpenDigitizer(Params.LinkType, boardID, 0, Params.VMEBaseAddress, &handle);
  if (ret1 != 0) {
    printf("Can't open digitizer\n");
    return 0;
  }

  /* Once we have the handler to the digitizer, we use it to call the other functions */
  ret1 = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
  if (ret1 != 0) {
    printf("Can't read board info\n");
    return 0;
  }
  printf("\nConnected to CAEN Digitizer Model %s, recognized as board %d\n", BoardInfo.ModelName, boardID);
  printf("Number of Channels : %d\n", BoardInfo.Channels);
  printf("SerialNumber : %d\n", BoardInfo.SerialNumber);
  printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
  printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);

  /* Check firmware revision (only DPP firmwares can be used with this Demo) */
  sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
  if (MajorNumber != V1730_DPP_PHA_CODE) {
    printf("This digitizer has not a DPP-PHA firmware\n");
    return 0;
  }

  /* *************************************************************************************** */
  /* Program the digitizer (see function ProgramDigitizer)                                   */
  /* *************************************************************************************** */
  ret = (CAEN_DGTZ_ErrorCode)ProgramDigitizer(handle, Params, DPPParams, InputDynamicRange, EnergyFinegain);
  if (ret != 0) {
    printf("Failed to program the digitizer\n");
    return 0;
  }
  
  // Board Configuration
  uint32_t * value = new uint32_t[1];
  CAEN_DGTZ_ReadRegister(handle, 0x8000 , value);
  //printf("                        32  28  24  20  16  12   8   4   0\n");
  //printf("                         |   |   |   |   |   |   |   |   |\n");
  //cout <<" Board Configuration  : 0b" << bitset<32>(value[0]) << endl;
  //printf("                Bit[ 0] = Auto Data Flush   \n");
  //printf("                Bit[16] = WaveForm Recording   \n");
  //printf("                Bit[17] = Extended Time Tag   \n");
  //printf("                Bit[18] = Record Time Stamp   \n");
  //printf("                Bit[19] = Record Energy   \n");
  //printf("====================================== \n");

  /* WARNING: The mallocs MUST be done after the digitizer programming,
  because the following functions needs to know the digitizer configuration
  to allocate the right memory amount */
  /* Allocate memory for the readout buffer */
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
  /* Allocate memory for the events */
  ret |= CAEN_DGTZ_MallocDPPEvents(handle, reinterpret_cast<void**>(&Events), &AllocatedSize) ;     
  
  if (ret != 0) {
    printf("Can't allocate memory buffers\n");
    CAEN_DGTZ_SWStopAcquisition(handle);
    CAEN_DGTZ_CloseDigitizer(handle);
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
    return 0;
  }
  
  /* *************************************************************************************** */
  /* ROOT TREE                                                                           */
  /* *************************************************************************************** */
  
  // ===== unsorted data
  vector<ULong64_t> rawTimeStamp;   
  vector<UInt_t> rawEnergy;
  vector<int> rawChannel;
  
  // ===== some variable for monitoring sorting time comsuption
  int maxSortSize = 10000;
  
  // ===== Sorted Tree
  TFile * fileout = new TFile(rootFileName, "RECREATE");
  TTree * tree = new TTree("tree", "tree");
  gs.Write();
  for( int i = 0 ; i < MaxNChannels; i++){
    if (Params.ChannelMask & (1<<i)) {
      chSetting[i].Write(Form("setting_%i", i));
    }
  }
  
  TLine coincidentline;
  coincidentline.SetLineColor(2);
  coincidentline.SetX1(CoincidentWindow);
  coincidentline.SetY1(0);
  coincidentline.SetX2(CoincidentWindow);
  coincidentline.SetY2(100000000);
  
  // ==== data for one event
  ULong64_t timeStamp[MaxNChannels];
  UInt_t energy[MaxNChannels];
  int channel[MaxNChannels];
  
  TString expre;
  expre.Form("channel[%d]/I", MaxNChannels); tree->Branch("ch", channel, expre);
  expre.Form("energy[%d]/i", MaxNChannels); tree->Branch("e", energy, expre);
  expre.Form("timeStamp[%d]/l", MaxNChannels); tree->Branch("t", timeStamp, expre);
  
  tree->Write("tree", TObject::kOverwrite); 
  
  fileout->Close();
  
  //=== Raw tree
  ULong64_t t_r;
  ULong_t e_r;
  int ch_r;
  
  TFile * fileRaw = NULL;
  TTree * rawTree = NULL;
  if( isSaveRaw ) {
    fileRaw = new TFile("raw.root", "RECREATE");
    rawTree = new TTree("rawtree", "rawtree");
      
    gs.Write();
    for( int i = 0 ; i < MaxNChannels; i++){
      if (Params.ChannelMask & (1<<i)) {
        chSetting[i].Write(Form("setting_%d", i));
      }
    }
    
    rawTree->Branch("ch", &ch_r, "channel/I");
    rawTree->Branch("e", &e_r, "energy/i");
    rawTree->Branch("t", &t_r, "timeStamp/l");
  }
  //==== Drawing 

  gStyle->SetOptStat("neiou");
  cCanvasAux = new TCanvas("cCanvasAux", "RAISOR isotopes production (Aux)", 600, 500, 1000, 500); 
  if( cCanvasAux->GetShowEditor() ) cCanvasAux->ToggleEditor(); 
  if( cCanvasAux->GetShowToolBar() ) cCanvasAux->ToggleToolBar(); 
  cCanvasAux->Divide(2,1);
  cCanvasAux->cd(1)->SetGridy();
  cCanvasAux->cd(1)->SetGridx();
  cCanvasAux->cd(1)->SetTicky();
  cCanvasAux->cd(1)->SetTickx();
  
  cCanvasAux->cd(2)->SetGridy();
  cCanvasAux->cd(2)->SetGridx();
  cCanvasAux->cd(2)->SetTicky();
  cCanvasAux->cd(2)->SetTickx();
  
  cCanvas = new TCanvas("cCanvas", Form("RAISOR isotopes production | %s (%s)", location.c_str(), hostname), 0, 0, 1400, 1000);
  cCanvas->Divide(1,2);
  if( cCanvas->GetShowEditor() ) cCanvas->ToggleEditor();
  if( cCanvas->GetShowToolBar() ) cCanvas->ToggleToolBar();
  
  cCanvas->cd(1)->Divide(2,1); cCanvas->cd(1)->cd(1)->SetLogz();
  cCanvas->cd(2)->SetGridy();
  cCanvas->cd(2)->SetTicky();
  cCanvas->cd(2)->SetTickx();
  cCanvas->cd(1)->cd(2)->Divide(2,2);
  cCanvas->cd(1)->cd(2)->cd(3)->SetGridy();
  cCanvas->cd(1)->cd(2)->cd(3)->SetTicky();
  cCanvas->cd(1)->cd(2)->cd(3)->SetTickx(); 
  cCanvas->cd(1)->cd(2)->cd(4)->SetLogy(); 
  
  int mode = 0;
  
  hE    = new TH1F(   "hE", Form("raw E (ch=%d) ; E [ch] ;count ", chE),         500, rangeE[0], rangeE[1]);
  htotE = new TH1F("htotE", "total E ; totE [ch] ; count",    500, rangeDE[0]+ rangeE[0]*6.06, rangeDE[1] + rangeE[1]*6.06);
  hdE   = new TH1F(  "hdE", Form("raw dE (ch=%d) ; dE [ch]; count", chDE),        500, rangeDE[0], rangeDE[1]);
  
  
  if( chGain[chE] != 1.0 || chGain[chDE] != 1.0 ){
    mode = 4;
    hdEtotE  = new TH2F( "hdEtotE", Form("dE - totE = %4.2fdE + %4.2fE; totalE [ch]; dE [ch ", chGain[chDE], chGain[chE] ), 500, rangeDE[0] * chGain[chDE] + rangeE[0]* chGain[chE], rangeDE[1]* chGain[chDE] + rangeE[1]* chGain[chE], 500, rangeDE[0] * chGain[chDE], rangeDE[1] * chGain[chDE]);  
  }else{
    if( InputDynamicRange[chE] == InputDynamicRange[chDE] ) {
      hdEtotE  = new TH2F( "hdEtotE", "dE - totE = dE + E; totalE [ch]; dE [ch ", 500, rangeDE[0] + rangeE[0], rangeDE[1] + rangeE[1], 500, rangeDE[0], rangeDE[1]);  
      mode = 0;
    }else if (InputDynamicRange[chE] > InputDynamicRange[chDE]) { // E = 0.5Vpp, dE = 2 Vpp
      hdEtotE  = new TH2F( "hdEtotE", "dE - totE = dE + E/4 ; totalE [ch]; dE [ch ", 500, rangeDE[0] + rangeE[0]/4, rangeDE[1] + rangeE[1]/4, 500, rangeDE[0], rangeDE[1]);  
      mode = 1;
    }else if (InputDynamicRange[chE] < InputDynamicRange[chDE]) { // E = 2 Vpp, dE = 0.5 Vpp
      hdEtotE  = new TH2F( "hdEtotE", "dE - totE = dE/4 + E; totalE [ch]; dE [ch ", 500, rangeDE[0]/4 + rangeE[0], rangeDE[1]/4 + rangeE[1], 500, rangeDE[0], rangeDE[1]);  
      mode = 2;
    }
  }
  
  hdEE  = new TH2F( "hdEE", "dE - E ; E [ch]; dE [ch] ", 500, rangeE[0], rangeE[1], 500, rangeDE[0], rangeDE[1]);  
  hTDiff = new TH1F("hTDiff", "timeDiff [nsec]; time [nsec] ; count", 500, 0, rangeTime);
  
  hE->GetXaxis()->SetLabelSize(0.06);
  hE->GetYaxis()->SetLabelSize(0.06);
  
  hdE->GetXaxis()->SetLabelSize(0.06);
  hdE->GetYaxis()->SetLabelSize(0.06);
  
  hTDiff->GetXaxis()->SetLabelSize(0.06);
  hTDiff->GetYaxis()->SetLabelSize(0.06);
  
  hdEtotE->SetMinimum(1);
  hdEE->SetMinimum(1);
  
  htotETAC = new TH2F( "htotETAC", "totE - TAC; TAC; totE [ch] ", 500, 0, 16000, 500, rangeDE[0] + rangeE[0], rangeDE[1] + rangeE[1]);
  
  rateGraph = new TMultiGraph();
  legend = new TLegend( 0.9, 0.2, 0.99, 0.8); 
  
  graphRate = new TGraph();
  graphRate->SetTitle("Total Rate [pps]");
  graphRate->SetMarkerColor(4);
  graphRate->SetMarkerStyle(20);
  graphRate->SetMarkerSize(1);
  
  rangeGraph = new TGraph();
  rangeGraph->SetMarkerColor(0);
  rangeGraph->SetMarkerStyle(1);
  rangeGraph->SetMarkerSize(0.000001);
  
  fullRateGraph = new TMultiGraph();
  fullLegend = new TLegend( 0.9, 0.2, 0.99, 0.8); 
  
  fullGraphRate = new TGraph();
  fullGraphRate->SetTitle("Total Rate [pps]");
  fullGraphRate->SetMarkerColor(4);
  fullGraphRate->SetMarkerStyle(20);
  fullGraphRate->SetMarkerSize(1);
  
  gROOT->ProcessLine("gErrorIgnoreLevel = kFatal;"); // supress error messsage
  ReadCut("cutsFile.root");
  
  cCanvasAux->cd(1); htotETAC->Draw("colz");
  cCanvasAux->cd(2); hdEE->Draw("colz");
  
  cCanvas->cd(1)->cd(1); hdEtotE->Draw("colz");
  cCanvas->cd(1)->cd(2)->cd(1); hE->Draw();
  cCanvas->cd(1)->cd(2)->cd(2); hdE->Draw();
  //cCanvas->cd(1)->cd(2)->cd(3); htotE->Draw();
  cCanvas->cd(1)->cd(2)->cd(3); rateGraph->Draw("AP");
  cCanvas->cd(1)->cd(2)->cd(4); hTDiff->Draw();
  
  gStyle->SetTitleFontSize(0.1);
  
  cCanvas->cd(2); 
  fullRateGraph->Draw("AP"); //legend->Draw();
  
  cCanvas->Update();
  cCanvasAux->Update();
  gSystem->ProcessEvents();

  thread paintCanvasThread(paintCanvas); // using loop keep root responding

  /* *************************************************************************************** */
  /* Readout Loop                                                                            */
  /* *************************************************************************************** */

  for (ch = 0; ch < MaxNChannels; ch++) {
    TrgCnt[ch] = 0;
    ECnt[ch] = 0;
    //PrevTime[ch] = 0;
    //ExtendedTT[ch] = 0;
    PurCnt[ch] = 0;
  }
  PrevRateTime = get_time();
  AcqRun = 0;
  PrintInterface();
  int rawEvCount = 0;
  int totEventBuilt = 0;
  int totMultiHitEventBuilt = 0;
  int graphIndex = 0;
  ULong64_t rollOver = 0;
  int numDataRetriving = 0;
    
  while(!QuitFlag) {
    //##################################################################
    /* Check keyboard */
    if(kbhit()) {
      char c;
      c = getch();
      //========== quit
      if (c == 'q') {
        QuitFlag = true;
        
        if( isCutFileOpen ) {
          TFile * fileAppend = new TFile(rootFileName, "UPDATE");
          cutList = (TObjArray *) fCut->FindObjectAny("cutList");
          cutList->Write();
          fileAppend->Close();
        }
        
      }
      //========== reset histograms
      if ( c == 'y'){
        hdEtotE->Reset();
        hE->Reset();
        hdE->Reset();
        hdEE->Reset();
        htotE->Reset();
        hTDiff->Reset();
      }
      //==========read channel setting form digitizer
      if (c == 'p') {
        CAEN_DGTZ_SWStopAcquisition(handle); 
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        for( int id = 0 ; id < MaxNChannels ; id++ ) {
          if (Params.ChannelMask & (1<<id)) GetChannelSetting(handle, id);
        }
        printf("===================================\n");
        PrintInterface();
        AcqRun = 0;
      }
      //========== start acquisition
      if (c == 's')  {
        gROOT->ProcessLine("gErrorIgnoreLevel = -1;");
        // NB: the acquisition for each board starts when the following line is executed
        // so in general the acquisition does NOT starts syncronously for different boards
        if( graphIndex == 0 ) {
          StartTime = get_time();
          rawEvCount = 0;
          totEventBuilt = 0;
          totMultiHitEventBuilt = 0;
        }
        CAEN_DGTZ_SWStartAcquisition(handle);
        printf("Acquisition Started for Board %d\n", boardID);
        AcqRun = 1;
      }
      //========== stop acquisition
      if (c == 'a')  {
        CAEN_DGTZ_SWStopAcquisition(handle); 
        StopTime = get_time();  
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        printf("========== Duration : %lu msec\n", StopTime - StartTime);
        AcqRun = 0;
        
        rawChannel.clear();
        rawEnergy.clear();
        rawTimeStamp.clear();
      }
      //========== pause and make cuts
      if( c == 'c' ){
        CAEN_DGTZ_SWStopAcquisition(handle); 
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        
        fCut->Close();
        string expression = "./CutsCreator " + (string)rootFileName + " " ;
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
        
        ReadCut("cutsFile.root");
        
        PrintInterface();
        AcqRun = 0;
        
        rawChannel.clear();
        rawEnergy.clear();
        rawTimeStamp.clear();
      }
    }
    
    if (!AcqRun) {
      Sleep(10); // 10 mili-sec
      continue;
    }
    //##################################################################
    /* Calculate throughput and trigger rate (every second) */
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PrevRateTime; /* milliseconds */
    int countEventBuilt = 0;
    int countMultiHitEventBuilt = 0;
    if (ElapsedTime > updatePeriod) {
      //sort event from tree and append to exist root
      TFile * fileAppend = new TFile(rootFileName, "UPDATE");
      tree = (TTree*) fileAppend->Get("tree");
      double fileSize = fileAppend->GetSize() / 1024. / 1024. ;
      
      tree->SetBranchAddress("e", energy);
      tree->SetBranchAddress("t", timeStamp);
      tree->SetBranchAddress("ch", channel);
      
      int nRawData = rawChannel.size();
      //clean up countFromCut
      countEventBuilt = 0;
      if(isCutFileOpen){
        for( int i = 0 ; i < numCut; i++ ){
          countFromCut[i] = 0;
        }
      }
      
      //################################################################
      //  Sorrting raw event timeStamp
      //################################################################
      uint64_t buildStartTime = get_time();
      // bubble sort
      int sortIndex[nRawData];
      double bubbleSortTime[nRawData];
      for( int i = 0; i < nRawData; i++){
        bubbleSortTime[i] = double(rawTimeStamp[i]/1e12);
        //printf("%d, %d,  %llu \n", i,rawEnergy[i], rawTimeStamp[i]);
      }
      TMath::BubbleLow(nRawData,bubbleSortTime,sortIndex);
      // Re-map
      int channelT[nRawData];
      ULong_t energyT[nRawData];
      ULong64_t timeStampT[nRawData]; 
      for( int i = 0; i < nRawData ; i++){
        channelT[i] = rawChannel[i];
        energyT[i] = rawEnergy[i];
        timeStampT[i] = rawTimeStamp[i]; 
      }
      for( int i = 0; i < nRawData ; i++){
        rawChannel[i] = channelT[sortIndex[i]];
        rawTimeStamp[i] = timeStampT[sortIndex[i]];
        rawEnergy[i] = energyT[sortIndex[i]];
        //printf("%d| %d,  %d,  %llu  \n", i, rawChannel[i], rawEnergy[i], rawTimeStamp[i]);
      }
      
      // Fill TDiff
      for( int i = 0; i < nRawData-1; i++){
        ULong64_t timeDiff = rawTimeStamp[i+1]- rawTimeStamp[i] ;
        hTDiff->Fill(timeDiff * ch2ns);
      }
      //################################################################
      // build event base on coincident window
      //################################################################
      int endID = 0;
      for( int i = 0; i < nRawData-1; i++){
        ULong64_t timeToEnd = (rawTimeStamp[nRawData-1] - rawTimeStamp[i]) * ch2ns ; // in nano-sec
        endID = i;
        //printf(" time to end %d / %d , %d, %d\n", timeToEnd, CoincidentWindow, i , endID);
        if( timeToEnd < CoincidentWindow ) {
          break;
        }
        
        int numRawEventGrouped = 0;
        
        printf("%4d--------- %d, %llu, %d \n", countEventBuilt, rawChannel[i], rawTimeStamp[i], rawEnergy[i]);

        int digitID = 1 << rawChannel[i]; // for checking if the Channel[i] is already taken.
        for( int j = i+1; j < nRawData; j++){
          
          //check is channel[j] is taken or not
          unsigned int x = 1 << rawChannel[j];
          unsigned int y = digitID ^ x; // bitwise XOR
          unsigned int z = 1 & (y >> rawChannel[j]);
                      
          unsigned long long int timeDiff = (rawTimeStamp[j] - rawTimeStamp[i]) * ch2ns;
          
          printf("%3d | %d | %d, %llu, %llu, %d\n", digitID, rawChannel[j], z, rawTimeStamp[j], timeDiff, rawEnergy[j]); 
          
          digitID += x;
          
          if( timeDiff < CoincidentWindow ){
            // if channel already taken
            if( z == 0 ) { 
              countMultiHitEventBuilt ++;
              totMultiHitEventBuilt ++;
              //break;
            }
            numRawEventGrouped ++;
          
          }else{
            // normal exit when next event outside coincident window
            break;
          }
          
        }
        
        // when chTAC is single, skip.
        if( numRawEventGrouped == 0 && rawChannel[i] == chTAC) continue;
     
        //printf("---- %d/ %d,  num in Group : %d \n", i, nRawData,  numRawEventGrouped);
        countEventBuilt ++;
        
        //clear pervious event data
        for( int k = 0 ; k < MaxNChannels ; k++){
          channel[k] = -1;
          energy[k] = 0;
          timeStamp[k] = 0;
        }
        for( int j = i ; j <= i + numRawEventGrouped ; j++){          
          channel[rawChannel[j]] = rawChannel[j];
          energy[rawChannel[j]] = rawEnergy[j];
          timeStamp[rawChannel[j]] = rawTimeStamp[j];
          totEventBuilt++;
        }
        
        tree->Fill();
        
        //===== fill histogram
        float deltaE = energy[chDE] ;
        float ERes = energy[chE] ;
        float TAC = energy[chTAC];
        
        float totalE = TMath::QuietNaN();
        if( mode == 4 ){
          totalE = energy[chDE]*chGain[chDE] + energy[chE]*chGain[chE];
        }else{  
          if( InputDynamicRange[chE] == InputDynamicRange[chDE] ) {
            totalE = energy[chDE] + energy[chE];
          }else if (InputDynamicRange[chE] > InputDynamicRange[chDE]) { // E = 0.5Vpp, dE = 2 Vpp
            totalE = energy[chDE] + energy[chE]/4;
          }else if (InputDynamicRange[chE] < InputDynamicRange[chDE]) { // E = 2 Vpp, dE = 0.5 Vpp
            totalE = energy[chDE] + energy[chE]*4;
          }
        }
        
        htotE->Fill(totalE); // x, y
        hdEE->Fill(ERes, deltaE); // x, y
        hdEtotE->Fill(totalE, deltaE); // x, y
        htotETAC->Fill(TAC, totalE); // x, y
        
        if(isCutFileOpen){
          for( int k = 0 ; k < numCut; k++ ){
            cutG = (TCutG *)cutList->At(k) ;
            if( cutG->IsInside(totalE, deltaE)){
              countFromCut[k] += 1;
            }
          }
        }
        
        i += numRawEventGrouped ; 
        
      }/**/// end of event building
      //################################################################
      //################################################################
      
      uint64_t buildStopTime = get_time();
      uint64_t buildTime = buildStopTime - buildStartTime;
      
      //if( buildTime > updatePeriod ) maxSortSize = nRawData * 0.9 ;
      
      //clear vectors but keep from endID
      rawChannel.erase(rawChannel.begin(), rawChannel.begin() + endID  );
      rawEnergy.erase(rawEnergy.begin(), rawEnergy.begin() + endID );
      rawTimeStamp.erase(rawTimeStamp.begin(), rawTimeStamp.begin() + endID );
      
      if( rawTimeStamp.size() > 10 ) {
        FILE * paraOut;
        paraOut = fopen ("BoxScoreLeftOver_debug", "w+");
        
        int leftSize = rawTimeStamp.size();
        fprintf(paraOut, "=========================================================== %d\n", leftSize);
        fprintf(paraOut, " %2s , %15s | %10s \n", "ch", "time [ns]", "energy" );
        for( int p = 0 ; p < leftSize ; p ++){
          ULong64_t timeToEnd = (rawTimeStamp[leftSize-1] - rawTimeStamp[p]) * ch2ns ; // in nano-sec
          fprintf(paraOut, " %2d , %15llu | %10d | %llu\n", rawChannel[p], rawTimeStamp[p] * ch2ns, rawEnergy[p], timeToEnd );
        }
        fflush(paraOut);
        fclose(paraOut);
      }
      
      // write histograms and tree
      tree->Write("", TObject::kOverwrite); 
      
      //filling filling histogram 
      cCanvasAux->cd(1); htotETAC->Draw("colz");
      cCanvasAux->cd(2); hdEE->Draw("colz");
      
      cCanvas->cd(1)->cd(1); hdEtotE->Draw("colz");
      cCanvas->cd(1)->cd(2)->cd(1); hE->Draw();
      cCanvas->cd(1)->cd(2)->cd(2); hdE->Draw();
      cCanvas->cd(1)->cd(2)->cd(3); htotE->Draw();
      cCanvas->cd(1)->cd(2)->cd(4); hTDiff->Draw(); coincidentline.Draw("same");
      
      //=========================== Display
      system(CLEARSCR);
      PrintInterface();
      printf("\n======== Tree, Histograms, and Table update every ~%.2f sec\n", updatePeriod/1000.);
      printf("Number of retriving per sec = %.2f \n", numDataRetriving*1000./updatePeriod);
      printf("Time Elapsed                = %.3f sec = %.1f min\n", (CurrentTime - StartTime)/1e3, (CurrentTime - StartTime)/1e3/60.);
      printf("Readout Rate                = %.5f MB/s\n", (float)Nb/((float)ElapsedTime*1048.576f));
      printf("Total number of Raw Event   = %d \n", rawEvCount);
      printf("Total number of Event Built = %d \n", totEventBuilt);
      printf("Event-building time         = %lu msec\n", buildTime);
      //printf("max sort event size = %d \n", maxSortSize);
      printf("Built-event save to  : %s \n", rootFileName.Data());
      printf("File size  : %.4f MB \n", fileSize );
      printf("Database :  %s\n", databaseName.Data());
      printf("\nBoard %d:\n",boardID);
      for(i=0; i<MaxNChannels; i++) {
        if( i != chE && i != chDE && i != chTAC) continue;
        if (TrgCnt[i]>0){
          printf("\tCh %d:\tTrgRate=%.2f Hz\tPileUpRate=%.2f%%\n", i, (float)TrgCnt[i]/(float)ElapsedTime *1000., (float)PurCnt[i]*100/(float)TrgCnt[i]);
        }else{
          if (!(Params.ChannelMask & (1<<i))){
            printf("\tCh %d:\tMasked\n", i);
          }else{
            printf("\tCh %d:\tNo Data\n", i);
          }
        }
        TrgCnt[i]=0;
        PurCnt[i]=0;
      }
      Nb = 0;
      PrevRateTime = CurrentTime;
      printf("\n");
      
      //filling rate graph and data base
      int lowerTime = (CurrentTime - StartTime)/1e3 - RateWindow;
      for( int j = 1 ; j <= graphRate->GetN(); j++){
        double x, y;
        graphRate->GetPoint(j-1, x, y);
        if( x < lowerTime ) graphRate->RemovePoint(j-1);
      }
      double totalRate = countEventBuilt*1.0/ElapsedTime*1e3;
      graphRate->SetPoint(graphRate->GetN(), (CurrentTime - StartTime)/1e3, totalRate);
      
      printf(" number of raw data to sort            : %d \n", nRawData);
      printf(" number of raw Event left Over         : %d \n", (int) rawChannel.size());
      printf(" number of multi-hit event built       : %d \n", countMultiHitEventBuilt);
      printf(" number of event built in this sort    : %d (x3 = %d)\n", countEventBuilt, 3*countEventBuilt);
      printf("===============================================\n");
      printf(" Rate( all) :%7.2f pps | mean :%7.2f pps\n", countEventBuilt*1.0/ElapsedTime*1e3, graphRate->GetMean(2));
      
      fullGraphRate->SetPoint(graphIndex, (CurrentTime - StartTime)/1e3, totalRate);
      string tag = "tag=" + location;
      WriteToDataBase(databaseName, "totalRate", tag, totalRate);
      
      if(isCutFileOpen){
        for( int i = 0 ; i < numCut; i++ ){
          for( int j = 1 ; j <= graphRateCut[i]->GetN(); j++){
            double x, y;
            graphRateCut[i]->GetPoint(j-1, x, y);
            if( x < lowerTime ) {
              graphRateCut[i]->RemovePoint(j-1);
            }
          }
          graphRateCut[i]->SetPoint(graphRateCut[i]->GetN(), (CurrentTime - StartTime)/1e3, countFromCut[i]*1.0/ElapsedTime*1e3);
          fullGraphRateCut[i]->SetPoint(graphIndex, (CurrentTime - StartTime)/1e3, countFromCut[i]*1.0/ElapsedTime*1e3);
          cutG = (TCutG *)cutList->At(i) ;
          cCanvas->cd(1)->cd(1); cutG->Draw("same");
          printf(" Rate(%4s) :%7.2f pps | mean :%7.2f pps\n", cutG->GetName(), countFromCut[i]*1.0/ElapsedTime*1e3, graphRateCut[i]->GetMean(2));
          
          //============= write to database 
          WriteToDataBase(databaseName, cutG->GetName(), tag,  countFromCut[i]*1.0/ElapsedTime*1e3);
        }
        
        // ratio matrix
        if( numCut >= 2) {
          printf("=========== ratio matrix : \n");
          printf("%10s", "");
          for( int j = 0 ; j < numCut ; j++){
            cutG = (TCutG *)cutList->At(j) ;
            printf("%10s", cutG->GetName()) ;
          }
          printf("\n");
          for( int i = 0; i < numCut; i++){
            cutG = (TCutG *)cutList->At(i) ;
            printf("%10s", cutG->GetName()) ;
            for( int j = 0; j < numCut ; j++){
              if( i == j) {
                printf("%10s", "/");
              }else{
                if( countFromCut[j] > countFromCut[i] ){
                  printf("%9.3f%%", countFromCut[i]* 100./countFromCut[j] );
                }else{
                  printf("%10s", "-");
                }
              }
              if( j == numCut -1 ) printf("\n");
            }
          }
        }
      }
      
      graphIndex ++;
      
      //Draw rate Graph
      rangeGraph->SetPoint(0, TMath::Max((double)RateWindow, (CurrentTime - StartTime)/1e3  + 5) , 0 ); // 5 sec gap
      cCanvas->cd(1)->cd(2)->cd(3); rateGraph->Draw("AP"); legend->Draw();
      
      cCanvas->cd(2); fullRateGraph->Draw("AP"); fullLegend->Draw();
      fullRateGraph->GetXaxis()->SetRangeUser(0, TMath::Max((double)RateWindow, (CurrentTime - StartTime)/1e3 *1.2) );
      
      cCanvas->Modified();
      cCanvas->Update();
      cCanvasAux->Modified();
      cCanvasAux->Update();
      
      // wirte histogram into tree
      fileAppend->cd();
      hdEtotE->Write("", TObject::kOverwrite); 
      hE->Write("", TObject::kOverwrite); 
      hdE->Write("", TObject::kOverwrite); 
      htotE->Write("", TObject::kOverwrite);
      hdEE->Write("", TObject::kOverwrite);
      hTDiff->Write("", TObject::kOverwrite);
      htotETAC->Write("", TObject::kOverwrite);
      fullRateGraph->Write("rateGraph", TObject::kOverwrite); 

      fileAppend->Close();
      
      if( isSaveRaw ) {
        fileRaw->cd();
        rawTree->Write("rawtree", TObject::kOverwrite); 
      }
      
      numDataRetriving = 0;
    }
    //##################################################################
    /* Read data from the board */
    ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
    if (BufferSize == 0) continue;
    Nb += BufferSize;
    //ret = DataConsistencyCheck((uint32_t *)buffer, BufferSize/4);
    ret |= (CAEN_DGTZ_ErrorCode) CAEN_DGTZ_GetDPPEvents(handle, buffer, BufferSize, reinterpret_cast<void**>(&Events), NumEvents);
    if (ret) {
      printf("Data Error: %d\n", ret);
      CAEN_DGTZ_SWStopAcquisition(handle);
      CAEN_DGTZ_CloseDigitizer(handle);
      CAEN_DGTZ_FreeReadoutBuffer(&buffer);
      CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
      return 0;
    }
    numDataRetriving++;
    //##################################################################
    /* Analyze data */
    for (ch = 0; ch < MaxNChannels; ch++) {
      if (!(Params.ChannelMask & (1<<ch))) continue;
      
      //printf("------------------------ %d \n", ch);
      
      for (ev = 0; ev < NumEvents[ch]; ev++) {
          TrgCnt[ch]++;
          
          //if (Events[ch][ev].TimeTag < PrevTime[ch]) ExtendedTT[ch]++;
          //PrevTime[ch] = Events[ch][ev].TimeTag;
          
          if (Events[ch][ev].Energy > 0) {
            ECnt[ch]++;
              
            ULong64_t timetag = (ULong64_t) Events[ch][ev].TimeTag;
            rollOver = Events[ch][ev].Extras2 >> 16;
            rollOver = rollOver << 31;
            timetag  += rollOver ;
            
            if( ch == chTAC ) timetag = timetag - 275; // subtract 550 ns for TAC signal.
            
            //printf("%llu | %llu | %llu \n", Events[ch][ev].Extras2 , rollOver >> 32, timetag);
                        
            rawEvCount ++;
            
            ch_r = ch;
            e_r = Events[ch][ev].Energy ;
            //if( ch == 0 || ch == 1 ) {
            //  e_r += int(gRandom->Gaus(0, 200));
            //  if( ch == chDE ) e_r  += gRandom->Integer(2)*1000;
            //}
            t_r = timetag;
            if( isSaveRaw ) rawTree->Fill();
            
            rawChannel.push_back(ch);
            rawEnergy.push_back(e_r);
            rawTimeStamp.push_back(timetag);
            
            //printf(" ch: %2d | %lu %llu\n", ch_r, e_r, t_r);
            
            // fake E events
            if( NumEvents[chE] == 0 && NumEvents[chDE] != 0) {
              rawChannel.push_back(chE);
              ULong_t haha = gRandom->Integer(500) + 2000;
              rawEnergy.push_back(haha);
              rawTimeStamp.push_back(timetag + 10 );
            }
            
            
            if( chDE == ch )  hdE->Fill(e_r); 
            if( chE == ch )  hE->Fill(e_r); 
              
          } else { /* PileUp */
              PurCnt[ch]++;
          }
          
      } // loop on events
      
    } // loop on channels

  } // End of readout loop
  
  if( isSaveRaw ) {
    rawTree->Write("rawtree", TObject::kOverwrite); 
    fileRaw->Close();
  }
  fCut->Close();
  
  paintCanvasThread.detach();
  
  /* stop the acquisition, close the device and free the buffers */
  CAEN_DGTZ_SWStopAcquisition(handle);
  CAEN_DGTZ_CloseDigitizer(handle);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
  
  printf("=========== bye bye ==============\n\n");
  return ret;
}
    
