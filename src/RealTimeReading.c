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

#include "Functions.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
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

using namespace std;

#define MaxNChannels 8


//TODO 1) change DCoffset, pulseParity to channel
//TODO 2) change the tree structure to be like HELIOS

//========== General setting;
double ch2ns = 2.;
float DCOffset = 0.2;
bool PositivePulse = true;
uint RecordLength = 20000;   // Num of samples of the waveforms (only for waveform mode)
uint PreTriggerSize = 2000;
uint ChannelMask = 0x03;   // Channel enable mask, 0x01, only frist channel, 0xff, all channel

int updatePeriod = 1000; //Table, tree, Plots update period in mili-sec.
int CoincidentWindow = 300; // real time [nano-sec]

int chE = 0;   //channel ID for E
int chDE = 1;  //channel ID for dE

int rangeDE[2] = {0, 5000}; // range for dE
int rangeE[2] = {0, 5000};  // range for E
double rangeTime = 5e7;  // range for Tdiff

float RateWindow = 10.; // sec

bool isSaveRaw = false; // saving Raw data

//========= Histogram
TCanvas * cCanvas = NULL;
TH1F * hE = NULL;
TH1F * htotE = NULL;
TH1F * hdE = NULL;
TH2F * hdEtotE = NULL; 
TH1F * hTDiff = NULL;
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
void ReadGeneralSetting(string fileName){

  const int numPara = 15;
  
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
        if( count == 4  )    ChannelMask = std::stoul(line.substr(0, pos).c_str(), nullptr, 16);
        if( count == 5  )   updatePeriod = atoi(line.substr(0, pos).c_str());
        if( count == 6  ) CoincidentWindow = atof(line.substr(0, pos).c_str());
        if( count == 7  )    chE = atoi(line.substr(0, pos).c_str());
        if( count == 8  )   chDE = atoi(line.substr(0, pos).c_str());
        if( count == 9  )  rangeE[0] = atoi(line.substr(0, pos).c_str());
        if( count == 10 )  rangeE[1] = atoi(line.substr(0, pos).c_str());
        if( count == 11 ) rangeDE[0] = atoi(line.substr(0, pos).c_str());
        if( count == 12 ) rangeDE[1] = atoi(line.substr(0, pos).c_str());
        if( count == 13 )  rangeTime = atof(line.substr(0, pos).c_str());
        if( count == 14 )  RateWindow = atof(line.substr(0, pos).c_str());
        if( count == 15 )  {
          if( line.substr(0, 4) == "true" ) {
            isSaveRaw = true;
          }else{
            isSaveRaw = false;
          }
        }
        count ++;
      }
    }
    
    //print setting
    printf(" %-20s  %.3f (0x%04x)\n", "DC offset", DCOffset, uint( 0xffff * DCOffset ));
    printf(" %-20s  %s\n", "Positive Pulse", PositivePulse ? "true" : "false" );
    printf(" %-20s  %d ch\n", "Record Lenght", RecordLength);
    printf(" %-20s  %d ch\n", "Pre-Trigger Size", PreTriggerSize);
    bitset<8> b(ChannelMask);
    printf(" %-20s  %s\n", "Channel Mask", b.to_string().c_str());
    printf(" %-20s  %d msec\n", "Update period", updatePeriod);
    printf(" %-20s  %.3f ns\n", "Coincident windows", CoincidentWindow * 1.0 );
    printf(" %-20s  %d (%d, %d)\n", "Channel E", chE, rangeE[0], rangeE[1]);
    printf(" %-20s  %d (%d, %d)\n", "Channel dE", chDE, rangeDE[0], rangeDE[1]);
    printf(" %-20s  %.3f ns\n", "tDiff range", rangeTime);
    printf(" %-20s  %s\n", "Is saving Raw", isSaveRaw ? "true" : "false");
    printf("====================================== \n");
    
  }

  return;
}

int* ReadChannelSetting(int ch, string fileName){

  const int numPara = 19;
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
  
  printf("--------------- input \n");
  CAEN_DGTZ_ReadRegister(handle, 0x1020 + (ch << 8), value); printf("%20s  %d \n", "Record Length",  value[0] * 8); //Record length
  CAEN_DGTZ_ReadRegister(handle, 0x1038 + (ch << 8), value); printf("%20s  %d \n", "Pre-tigger",  value[0] * 4); //Pre-trigger
  printf("%20s  %s \n", "polarity",  (polarity & 1) ==  0 ? "Positive" : "negative"); //Polarity
  printf("%20s  %.0f sample \n", "Ns baseline",  pow(4, 1 + baseline & 7)); //Ns baseline
  CAEN_DGTZ_ReadRegister(handle, 0x1098 + (ch << 8), value); printf("%20s  %.2f %% \n", "DC offset",  value[0] * 100./ int(0xffff) ); //DC offset
  CAEN_DGTZ_ReadRegister(handle, 0x1028 + (ch << 8), value); printf("%20s  %.1f Vpp \n", "input Dynamic",  value[0] == 0 ? 2 : 0.5); //InputDynamic
  
  printf("--------------- discriminator \n");
  CAEN_DGTZ_ReadRegister(handle, 0x106C + (ch << 8), value); printf("%20s  %d LSB\n", "Threshold",  value[0]); //Threshold
  CAEN_DGTZ_ReadRegister(handle, 0x1074 + (ch << 8), value); printf("%20s  %d ns \n", "trigger hold off *",  value[0] * 8); //Trigger Hold off
  CAEN_DGTZ_ReadRegister(handle, 0x1054 + (ch << 8), value); printf("%20s  %d sample \n", "Fast Dis. smoothing",  value[0] *2 ); //Fast Discriminator smoothing
  CAEN_DGTZ_ReadRegister(handle, 0x1058 + (ch << 8), value); printf("%20s  %d ch \n", "Input rise time *",  value[0] * 8); //Input rise time
  
  printf("--------------- Trapezoid \n");
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
  
  printf("--------------- Other \n");
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
  
  
  //if( fullRateGraph->GetListOfGraphs() != NULL) fullRateGraph->GetListOfGraphs()->RemoveAll();
  fullRateGraph->SetTitle("Beam rate [pps] (all time); Time [sec]; Rate [pps]");
  
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
    
  if( argc != 2 && argc != 3 ) {
    printf("Please input boardID! (optional root file name)\n");
    return -1;
  }
  
  const int boardID = atoi(argv[1]);
  string rootFileName = "tree.root";
  if( argc == 3 ) rootFileName = argv[2];
  
  printf("******************************************** \n");
  printf("****         Real Time PID              **** \n");
  printf("******************************************** \n");
  
  printf("   board ID : %d \n", boardID );
  printf("   save to  : %s \n", rootFileName.c_str() );

  ReadGeneralSetting("generalSetting.txt");

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
  int EnergyFinegain[MaxNChannels];

  /* Arrays for data analysis */
  uint64_t PrevTime[MaxNChannels];
  uint64_t ExtendedTT[MaxNChannels];
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
  for(ch=0; ch<MaxNChannels; ch++) {
    int* para = ReadChannelSetting(ch, "setting_" + to_string(ch) + ".txt");
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
  
  vector<ULong64_t> rawTimeStamp;
  vector<UInt_t> rawEnergy;
  vector<int> rawChannel;
  
  // ===== Sorted Tree
  TFile * fileout = new TFile(rootFileName.c_str(), "RECREATE");
  TTree * tree = new TTree("tree", "tree");
  
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
    
    rawTree->Branch("ch", &ch_r, "channel/I");
    rawTree->Branch("e", &e_r, "energy/i");
    rawTree->Branch("t", &t_r, "timeStamp/l");
  }
  //==== Drawing 
  gStyle->SetOptStat("neiou");
  cCanvas = new TCanvas("cCanvas", "RAISOR isotopes production", 1200, 800);
  cCanvas->Divide(1,2);
  cCanvas->cd(1)->Divide(2,1); cCanvas->cd(1)->cd(1)->SetLogz();
  cCanvas->cd(2)->SetGridy();
  cCanvas->cd(2)->SetTicky();
  cCanvas->cd(2)->SetTickx();
  cCanvas->cd(1)->cd(2)->Divide(2,2);
  cCanvas->cd(1)->cd(2)->cd(3)->SetGridy();
  cCanvas->cd(1)->cd(2)->cd(3)->SetTicky();
  cCanvas->cd(1)->cd(2)->cd(3)->SetTickx(); 
  cCanvas->cd(1)->cd(2)->cd(4)->SetLogy(); 
  
  hE    = new TH1F(   "hE", "raw E ; E [ch] ;count ",         500, rangeE[0], rangeE[1]);
  htotE = new TH1F("htotE", "total E ; totE [ch] ; count",    500, rangeDE[0] + rangeE[0], rangeDE[1] + rangeE[1]);
  hdE   = new TH1F(  "hdE", "raw dE ; dE [ch]; count",        500, rangeDE[0], rangeDE[1]);
  hdEtotE  = new TH2F( "hdEtotE", "dE - totE ; totalE [ch]; dE [ch ", 500, rangeDE[0] + rangeE[0], rangeDE[1] + rangeE[1], 500, rangeDE[0], rangeDE[1]);  
  hTDiff = new TH1F("hTDiff", "timeDiff; time [nsec] ; count", 500, 0, rangeTime);
  
  hdEtotE->SetMinimum(1);
  
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
  gSystem->ProcessEvents();

  thread paintCanvasThread(paintCanvas); // using loop keep root responding

  /* *************************************************************************************** */
  /* Readout Loop                                                                            */
  /* *************************************************************************************** */

  for (ch = 0; ch < MaxNChannels; ch++) {
    TrgCnt[ch] = 0;
    ECnt[ch] = 0;
    PrevTime[ch] = 0;
    ExtendedTT[ch] = 0;
    PurCnt[ch] = 0;
  }
  PrevRateTime = get_time();
  AcqRun = 0;
  PrintInterface();
  int rawEvCount = 0;
  int totEventBuilt = 0;
  int graphIndex = 0;
  ULong64_t rollOver = 0;
    
  while(!QuitFlag) {
    // Check keyboard
    if(kbhit()) {
      char c;
      c = getch();
      if (c == 'q') {
        QuitFlag = true;
      }
      if ( c == 'y'){
        hdEtotE->Reset();
        hE->Reset();
        hdE->Reset();
        htotE->Reset();
        hTDiff->Reset();
      }
      if (c == 'p') {
        //read channel setting form digitizer
        CAEN_DGTZ_SWStopAcquisition(handle); 
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        for( int id = 0 ; id < MaxNChannels ; id++ ) {
          if (Params.ChannelMask & (1<<id)) GetChannelSetting(handle, id);
        }
        printf("===================================\n");
        PrintInterface();
        AcqRun = 0;
      }
      if (c == 's')  {
        gROOT->ProcessLine("gErrorIgnoreLevel = -1;");
        // Start Acquisition
        // NB: the acquisition for each board starts when the following line is executed
        // so in general the acquisition does NOT starts syncronously for different boards
        if( graphIndex == 0 ) {
          StartTime = get_time();
          rawEvCount = 0;
          totEventBuilt = 0;
        }
        CAEN_DGTZ_SWStartAcquisition(handle);
        printf("Acquisition Started for Board %d\n", boardID);
        AcqRun = 1;
      }
      if (c == 'a')  {
        // Stop Acquisition
        CAEN_DGTZ_SWStopAcquisition(handle); 
        StopTime = get_time();  
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        printf("---------- Duration : %lu msec\n", StopTime - StartTime);
        AcqRun = 0;
        
        rawChannel.clear();
        rawEnergy.clear();
        rawTimeStamp.clear();
      }
      if( c == 'c' ){
        // pause and make cuts
        CAEN_DGTZ_SWStopAcquisition(handle); 
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        
        fCut->Close();
        string expression = "./CutsCreator " + rootFileName + " " ;
        expression = expression + to_string(chDE) + " ";
        expression = expression + to_string(chE) + " ";
        expression = expression + to_string(rangeDE[0]) + " ";
        expression = expression + to_string(rangeDE[1]) + " ";
        expression = expression + to_string(rangeE[0]) + " ";
        expression = expression + to_string(rangeE[1]);
        printf("%s\n", expression.c_str());
        system(expression.c_str());
        
        ReadCut("cutsFile.root");
        
        PrintInterface();
        AcqRun = 0;
      }
    }
    if (!AcqRun) {
      Sleep(10); // 10 mili-sec
      continue;
    }

    /* Calculate throughput and trigger rate (every second) */
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PrevRateTime; /* milliseconds */
    int countEventBuilt = 0;
    if (ElapsedTime > updatePeriod) {
      system(CLEARSCR);
      PrintInterface();
      printf("\n======== Tree, Histograms, and Table update every ~%.2f sec\n", updatePeriod/1000.);
      printf("Time Elapsed = %.3f sec = %.1f min\n", (CurrentTime - StartTime)/1e3, (CurrentTime - StartTime)/1e3/60.);
      printf("Readout Rate = %.5f MB/s\n", (float)Nb/((float)ElapsedTime*1048.576f));
      printf("Total number of Raw Event = %d \n", rawEvCount);
      printf("Total number of Event Built = %d \n", totEventBuilt);
      printf("Built-event save to  : %s \n", rootFileName.c_str() );
      printf("\nBoard %d:\n",boardID);
      for(i=0; i<MaxNChannels; i++) {
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
      
      //sort event from tree and append to exist root
      //printf("---- append file \n");
      TFile * fileAppend = new TFile(rootFileName.c_str(), "UPDATE");
      tree = (TTree*) fileAppend->Get("tree");
      
      tree->SetBranchAddress("e", energy);
      tree->SetBranchAddress("t", timeStamp);
      tree->SetBranchAddress("ch", channel);
      
      int n = rawChannel.size();
      printf(" number of raw data to sort %d \n", n);
      
      countEventBuilt = 0;
      if(isCutFileOpen){
        for( int i = 0 ; i < numCut; i++ ){
          countFromCut[i] = 0;
        }
      }
      
      //=== Event Building by sorrting 
      // bubble sort
      int sortIndex[n];
      double bubbleSortTime[n];
      for( int i = 0; i < n; i++){
        bubbleSortTime[i] = double(rawTimeStamp[i]/1e12);
        //printf("%d, %d,  %llu \n", i,rawEnergy[i], rawTimeStamp[i]);
      }
      TMath::BubbleLow(n,bubbleSortTime,sortIndex);
      // Re-map
      int channelT[n];
      ULong_t energyT[n];
      ULong64_t timeStampT[n]; //TODO, when fill directly from Digitizer, using energyT 
      for( int i = 0; i < n ; i++){
        channelT[i] = rawChannel[i];
        energyT[i] = rawEnergy[i];
        timeStampT[i] = rawTimeStamp[i]; 
      }
      for( int i = 0; i < n ; i++){
        rawChannel[i] = channelT[sortIndex[i]];
        rawTimeStamp[i] = timeStampT[sortIndex[i]];
        rawEnergy[i] = energyT[sortIndex[i]];
        //printf("%d| %d,  %d,  %llu  \n", i, rawChannel[i], rawEnergy[i], rawTimeStamp[i]);
      }
      
      //Fill TDiff
      for( int i = 0; i < n-1; i++){
        ULong64_t timeDiff = rawTimeStamp[i+1]- rawTimeStamp[i] ;
        hTDiff->Fill(timeDiff * ch2ns);
      }
      // build event base on coincident window
      int endID = 0;
      for( int i = 0; i < n-1; i++){
        int timeToEnd = abs(int(rawTimeStamp[n-1] - rawTimeStamp[i])) * ch2ns ; // in nano-sec
        endID = i;
        //printf(" time to end %d / %d , %d, %d\n", timeToEnd, CoincidentWindow, i , endID);
        if( timeToEnd < CoincidentWindow ) {
          break;
        }
          
        int numRawEventGrouped = 1;
        for( int j = i+1; j < n; j++){
          if( rawChannel[i] == rawChannel[j] ) continue;
          int timeDiff = (int) (rawTimeStamp[j] - rawTimeStamp[i]) ;
          if( TMath::Abs( timeDiff ) < CoincidentWindow ) {
            numRawEventGrouped ++;
          }else{
            break;
          }
        }
        //printf("---- %d/ %d,  num in Group : %d \n", i, n,  numRawEventGrouped);
        countEventBuilt ++;
        for( int k = 0 ; k < MaxNChannels ; k++){
          channel[k] = -1;
          energy[k] = 0;
          timeStamp[k] = 0;
        }
        
        for( int j = i ; j < i + numRawEventGrouped ; j++){          
          channel[rawChannel[j]] = rawChannel[j];
          energy[rawChannel[j]] = rawEnergy[j];
          timeStamp[rawChannel[j]] = rawTimeStamp[j];
          totEventBuilt++;
        }
        
        tree->Fill();
          
        float deltaE = energy[chDE] ;
        float totalE = energy[chDE] + energy[chE];
        
        htotE->Fill(totalE); // x, y
        hdEtotE->Fill(totalE, deltaE); // x, y
        
        if(isCutFileOpen){
          for( int k = 0 ; k < numCut; k++ ){
            cutG = (TCutG *)cutList->At(k) ;
            if( cutG->IsInside(totalE, deltaE)){
              countFromCut[k] += 1;
            }
          }
        }
        
        i += numRawEventGrouped - 1; 
        
      }/**/// end of event building
      
      //clear vectors but keep from endID
      rawChannel.erase(rawChannel.begin(), rawChannel.begin() + endID  );
      rawEnergy.erase(rawEnergy.begin(), rawEnergy.begin() + endID );
      rawTimeStamp.erase(rawTimeStamp.begin(), rawTimeStamp.begin() + endID );
      printf(" number of raw Event put into next sort : %d \n", (int) rawChannel.size());
        
      printf(" number of event built %d, Rate(all) : %.2f pps \n", countEventBuilt, countEventBuilt*1.0/ElapsedTime*1e3 );
      
      // write histograms and tree
      tree->Write("", TObject::kOverwrite); 
      hdEtotE->Write("", TObject::kOverwrite); 
      hE->Write("", TObject::kOverwrite); 
      hdE->Write("", TObject::kOverwrite); 
      htotE->Write("", TObject::kOverwrite);
      hTDiff->Write("", TObject::kOverwrite);
      //rateGraph->Write("rateGraph", TObject::kOverwrite); 
      fullRateGraph->Write("rateGraph", TObject::kOverwrite); 

      fileAppend->Close();
      
      if( isSaveRaw ) {
        fileRaw->cd();
        rawTree->Write("rawtree", TObject::kOverwrite); 
      }
      
      cCanvas->cd(1)->cd(1); hdEtotE->Draw("colz");
      cCanvas->cd(1)->cd(2)->cd(1); hE->Draw();
      cCanvas->cd(1)->cd(2)->cd(2); hdE->Draw();
      cCanvas->cd(1)->cd(2)->cd(3); htotE->Draw();
      cCanvas->cd(1)->cd(2)->cd(4); hTDiff->Draw();
      //filling rate graph
      int lowerTime = (CurrentTime - StartTime)/1e3 - RateWindow;
      for( int j = 1 ; j <= graphRate->GetN(); j++){
        double x, y;
        graphRate->GetPoint(j-1, x, y);
        if( x < lowerTime ) graphRate->RemovePoint(j-1);
      }
      graphRate->SetPoint(graphRate->GetN(), (CurrentTime - StartTime)/1e3, countEventBuilt*1.0/ElapsedTime*1e3);
      fullGraphRate->SetPoint(graphIndex, (CurrentTime - StartTime)/1e3, countEventBuilt*1.0/ElapsedTime*1e3);
      
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
          printf("                           Rate(%s) : %8.2f pps | mean : %8.2f pps \n", cutG->GetName(), countFromCut[i]*1.0/ElapsedTime*1e3, graphRateCut[i]->GetMean(2));
          
        }
        
        // ration matrix
        printf("=========== ration matrix : \n");
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
      
      graphIndex ++;
      
      rangeGraph->SetPoint(0, (CurrentTime - StartTime)/1e3 + 5 , 0 );
      cCanvas->cd(1)->cd(2)->cd(3); rateGraph->Draw("AP"); legend->Draw();
      cCanvas->cd(2); fullRateGraph->Draw("AP"); fullLegend->Draw();
      fullRateGraph->GetXaxis()->SetRangeUser(0, (CurrentTime - StartTime)/1e3 + 5 );
      
      cCanvas->Modified();
      cCanvas->Update();
        
    }
    
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
    
    /* Analyze data */
    for (ch = 0; ch < MaxNChannels; ch++) {
      if (!(Params.ChannelMask & (1<<ch))) continue;
      
      /* Update display */
      for (ev = 0; ev < NumEvents[ch]; ev++) {
          TrgCnt[ch]++;
          /* Time Tag */
          if (Events[ch][ev].TimeTag < PrevTime[ch]) ExtendedTT[ch]++;
          PrevTime[ch] = Events[ch][ev].TimeTag;
          /* Energy */
          if (Events[ch][ev].Energy > 0) {
            ECnt[ch]++;
              
            ULong64_t timetag = (ULong64_t) Events[ch][ev].TimeTag;
            rollOver = Events[ch][ev].Extras2 >> 16;
            timetag  += rollOver << 31;
            
            rawEvCount ++;
            
            ch_r = ch;
            e_r = Events[ch][ev].Energy ;
            //if( ch == 0 || ch == 1 ) {
              //e_r += int(gRandom->Gaus(0, 200));
              //if( ch == chDE ) e_r  += gRandom->Integer(2)*1000;
            //}
            t_r = timetag;
            if( isSaveRaw ) rawTree->Fill();
            
            rawChannel.push_back(ch);
            rawEnergy.push_back(e_r);
            rawTimeStamp.push_back(timetag);
            
            //printf(" ch: %2d | %lu %llu\n", ch_r, e_r, t_r);
            
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
    
