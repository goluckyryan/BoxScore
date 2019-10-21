#ifndef DIGITIZER
#define DIGITIZER

#include <TQObject.h>
#include <RQ_OBJECT.h>
#include "CAENDigitizer.h"
#include "CAENDigitizerType.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cstring>  //memset
#include <iostream> //cout
#include <fstream>
#include <cmath>
#include <vector>
#include <bitset>
#include <unistd.h>
#include <limits.h>
#include <ctime>

#include "TMath.h"

#define MaxNChannels 8

using namespace std;


class Digitizer{
  RQ_OBJECT("Digitizer") 
public:
  Digitizer(int ID, uint32_t ChannelMask);
  ~Digitizer();
  
  void SetChannelMask(uint32_t mask);
  void SetDCOffset(int ch , float offset);
  void SetCoincidentTimeWindow(int nanoSec) { CoincidentTimeWindow = nanoSec;}
  int SetChannelParity(int ch, bool isPositive);
  int SetChannelThreshold(int ch, int threshold);
  
  bool IsConnected() {return isConnected;} // can connect and retrieve Digitizer Info.
  bool IsGood() {return isGood;} // can detect digitizer
  int GetByteRetrived() { return Nb;}
  int GetInputDynamicRange(int ch) {return inputDynamicRange[ch];}
  bool IsRunning() {return AcqRun;}
  int GetNChannel(){ return NChannel;}
  int * GetInputDynamicRange() { return inputDynamicRange;}
  float * GetChannelGain() {return chGain;}
  float GetChannelGain(int ch) { return chGain[ch];}
  int GetChannelToNanoSec() {return ch2ns;};
  uint32_t GetChannelThreshold(int ch) {  
    uint32_t * value = new uint32_t[MaxNChannels];
    CAEN_DGTZ_ReadRegister(boardID, 0x106C + (ch << 8), value); 
    //printf("%20s  %d LSB\n", "Threshold",  value[0]); //Threshold
    return value[0]; 
  }
  
  unsigned long long int Getch2ns() {return ch2ns;}
  int GetCoincidentTimeWindow() { return CoincidentTimeWindow;}
  uint32_t GetChannelMask() const {return ChannelMask;}
  
  void GetBoardConfiguration();
  void GetChannelSetting(int ch);
  
  //======== Get Raw Data
  int GetNumRawEvent() {return rawEvCount;}
  vector<ULong64_t> GetRawTimeStamp() {return rawTimeStamp;}
  vector<UInt_t> GetRawEnergy() {return rawEnergy;}
  vector<int> GetRawChannel() {return rawChannel;}
  uint32_t GetRawTimeRange() {return rawTimeRange;} // in ch
  ULong64_t GetRawTimeStamp(int i) {return rawTimeStamp[i];}
  UInt_t GetRawEnergy(int i) {return rawEnergy[i];}
  int GetRawChannel(int i) {return rawChannel[i];}
  
  void ClearRawData(); // clear Raw Data and set rawEvCount = 0;
  void ClearData();  // clear built event vectors, and set countEventBuild =  0;

  int GetEventBuilt(){ return countEventBuilt; }
  int GetEventBuiltCount() { return countEventBuilt;}
  int GetTotalEventBuilt(){ return totEventBuilt; }
  int GetNChannelEvent(int Nch){ return countNChannelEvent[Nch-1];}
  int * GetNChannelEvent(){ return countNChannelEvent;}
  int GetTotalNChannelEvent(int Nch){ return totNChannelEvent[Nch-1];}
  
  //======== Get built event
  vector<ULong64_t> GetTimeStamp(int ev) {return TimeStamp[ev];}
  vector<UInt_t> GetEnergy(int ev) {return Energy[ev];}
  vector<int> GetChannel(int ev) {return Channel[ev];}
  ULong64_t GetTimeStamp(int ev, int ch){return TimeStamp[ev][ch];}
  UInt_t GetEnergy(int ev, int ch){return Energy[ev][ch];}
  int GetChannel(int ev, int ch){return Channel[ev][ch];}
  
  //========= Digitizer Control
  int ProgramDigitizer();
  void LoadChannelSetting (const int ch, string fileName);
  void LoadGeneralSetting(string fileName);
  
  void StopACQ();
  void StartACQ();
  
  void ReadData();
  int BuildEvent(bool debug);
  
  void PrintReadStatistic();
  void PrintEventBuildingStat(int updatePeriod);

private:

  bool isConnected; //can get digitizer info
  bool isGood;      // can open digitizer
  bool AcqRun;      // is digitizer taking data

  int boardID;   // board identity
  int ret;       //return value, refer to CAEN_DGTZ_ErrorCode
  int NChannel;  // number of channel

  int Nb;                                  // number of byte
  uint32_t NumEvents[MaxNChannels];
  char *buffer = NULL;                     // readout buffer
  uint32_t AllocatedSize, BufferSize; 
  CAEN_DGTZ_DPP_PHA_Event_t  *Events[MaxNChannels];  // events buffer

  //====================== Channel Setting
  CAEN_DGTZ_DPP_PHA_Params_t DPPParams;
  int inputDynamicRange[MaxNChannels];
  int energyFineGain[MaxNChannels];
  float chGain[MaxNChannels];
  uint PreTriggerSize[MaxNChannels];     
  CAEN_DGTZ_PulsePolarity_t PulsePolarity[MaxNChannels];   
  float DCOffset[MaxNChannels];  
  
  //====================== General Setting
  unsigned long long int ch2ns;    
  CAEN_DGTZ_ConnectionType LinkType;
  uint32_t VMEBaseAddress;
  CAEN_DGTZ_IOLevel_t IOlev;
  
  CAEN_DGTZ_DPP_AcqMode_t AcqMode;
  uint32_t RecordLength;                     // Num of samples of the waveforms (only for waveform mode)
  int EventAggr;                             // number of events in one aggregate (0=automatic), number of event acculated for read-off
  uint32_t ChannelMask;                      // Channel enable mask, 0x01, only frist channel, 0xff, all channel  

  int CoincidentTimeWindow;  // nano-sec

  //==================== retreved data
  int ECnt[MaxNChannels];
  int TrgCnt[MaxNChannels];
  int PurCnt[MaxNChannels];
  int rawEvCount;
  int rawEvLeftCount;
  uint64_t rawTimeRange;
  
  //===== unsorted data
  vector<ULong64_t> rawTimeStamp;   
  vector<UInt_t> rawEnergy;
  vector<int> rawChannel;
  
  //===== builded event
  int countEventBuilt;
  int totEventBuilt;
  int countNChannelEvent[MaxNChannels];
  int totNChannelEvent[MaxNChannels];
  
  vector<ULong64_t> singleTimeStamp;   
  vector<UInt_t> singleEnergy;
  vector<int> singleChannel;
  
  vector<vector<int>> Channel;
  vector<vector<UInt_t>> Energy;
  vector<vector<ULong64_t>> TimeStamp;
  
  void ZeroSingleEvent();
};

Digitizer::Digitizer(int ID, uint32_t ChannelMask){
  
  //================== initialization
  boardID = ID;
  NChannel = 0;
  
  AcqRun = false;
  
  ch2ns = 2; // 1 channel = 2 ns

  CoincidentTimeWindow = 100; // nano-sec
  
  Nb = 0;
  
  //----------------- default channel setting
  for ( int i = 0; i < MaxNChannels ; i++ ) {
    DCOffset[i] = 0.2;
    inputDynamicRange[i] = 0;
    chGain[i] = 1.0;
    energyFineGain[i] = 100;
    NumEvents[i] = 0; 
    PreTriggerSize[i] = 2000;
    PulsePolarity[i] = CAEN_DGTZ_PulsePolarityPositive; 
  }
  
  memset(&DPPParams, 0, sizeof(CAEN_DGTZ_DPP_PHA_Params_t));
  
  //----------------- Communication Parameters 
  LinkType = CAEN_DGTZ_USB;     // Link Type
  VMEBaseAddress = 0;           // For direct USB connection, VMEBaseAddress must be 0
  IOlev = CAEN_DGTZ_IOLevel_NIM;
  
  //--------------Acquisition parameters 
  //AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;          // CAEN_DGTZ_DPP_ACQ_MODE_List or CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope
  AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;             // CAEN_DGTZ_DPP_ACQ_MODE_List or CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope
  RecordLength = 20000;
  this->ChannelMask = ChannelMask;
  EventAggr = 1;       // Set how many events to accumulate in the board memory before being available for readout

  
  LoadGeneralSetting("generalSetting.txt");
  
  //===================== end of initization

  /* *********************************************** */
  /* Open the digitizer and read board information   */
  /* *********************************************** */
  
  printf("============= Opening Digitizer at Board %d \n", boardID);
  
  isConnected = false;
  isGood = true;
  
  ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, boardID, 0, VMEBaseAddress, &boardID);
  if (ret != 0) {
    printf("Can't open digitizer\n");
    isGood = false;
  }else{
    //----- Getting Board Info
    CAEN_DGTZ_BoardInfo_t BoardInfo;
    ret = (int) CAEN_DGTZ_GetInfo(boardID, &BoardInfo);
    if (ret != 0) {
      printf("Can't read board info\n");
      isGood = false;
    }else{
      isConnected = true;
      printf("Connected to CAEN Digitizer Model %s, recognized as board %d\n", BoardInfo.ModelName, boardID);
      NChannel = BoardInfo.Channels;
      printf("Number of Channels : %d\n", NChannel);
      printf("SerialNumber : %d\n", BoardInfo.SerialNumber);
      printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
      printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);
      
      int MajorNumber;
      sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
      if (MajorNumber != V1730_DPP_PHA_CODE) {
        printf("This digitizer does not have DPP-PHA firmware\n");
      }
    }
  }
  
  /* *********************************************** */
  /* Get Channel Setting and Set Digitizer           */
  /* *********************************************** */
  if( isGood ){
    printf("=================== reading Channel setting \n");
    for(int ch = 0; ch < NChannel; ch ++ ) {
      if ( ChannelMask & ( 1 << ch) ) {
        LoadChannelSetting(ch, "setting_" + to_string(ch) + ".txt");
      }
    }
    printf("====================================== \n");
  
    //============= Program the digitizer (see function ProgramDigitizer) 
    ret = (CAEN_DGTZ_ErrorCode)ProgramDigitizer();
    if (ret != 0) {
      printf("Failed to program the digitizer\n");
      isGood = false;
    }
  }
  
  if( isGood ) {
    /* WARNING: The mallocs MUST be done after the digitizer programming,
    because the following functions needs to know the digitizer configuration
    to allocate the right memory amount */
    // Allocate memory for the readout buffer 
    ret = CAEN_DGTZ_MallocReadoutBuffer(boardID, &buffer, &AllocatedSize);
    // Allocate memory for the events
    ret |= CAEN_DGTZ_MallocDPPEvents(boardID, reinterpret_cast<void**>(&Events), &AllocatedSize) ;     
    
    if (ret != 0) {
      printf("Can't allocate memory buffers\n");
      CAEN_DGTZ_SWStopAcquisition(boardID);
      CAEN_DGTZ_CloseDigitizer(boardID);
      CAEN_DGTZ_FreeReadoutBuffer(&buffer);
      CAEN_DGTZ_FreeDPPEvents(boardID, reinterpret_cast<void**>(&Events));
    }else{
      printf("====== Allocated memory for communication.\n");
    }
    
  }
  
  printf("################### Digitizer is ready \n");
  
  rawEvCount= 0;
  rawEvLeftCount = 0;
  
  rawTimeRange = 999999999999999;
  
  singleEnergy.clear();
  singleChannel.clear();
  singleTimeStamp.clear();
  ClearData();
  ZeroSingleEvent();

  countEventBuilt = 0;
  totEventBuilt = 0;
  
  for( int k = 0; k < MaxNChannels ; k++) {
    countNChannelEvent[k] = 0;
    totNChannelEvent[k] = 0;
  }
}


Digitizer::~Digitizer(){
  
  /* stop the acquisition, close the device and free the buffers */
  CAEN_DGTZ_SWStopAcquisition(boardID);
  CAEN_DGTZ_CloseDigitizer(boardID);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_FreeDPPEvents(boardID, reinterpret_cast<void**>(&Events));
  
  printf("Closed digitizer\n");
  printf("=========== bye bye ==============\n\n");
  
  for(int ch = 0; ch < MaxNChannels; ch++){
    delete Events[ch];
  }
  
  delete buffer;
}

int Digitizer::SetChannelThreshold(int ch, int threshold){
  
  ret |= CAEN_DGTZ_WriteRegister(boardID, 0x106C +  (ch<<8), threshold);
  
  DPPParams.thr[ch] = threshold;
  
  if( ret == 0 ) {
    
    TString command;
    command.Form("sed -i '2s/.*/%d     \\/\\/Tigger Threshold (in LSB)/' setting_%d.txt", threshold, ch);
    //printf(" %s \n", command.Data());
    system(command.Data());
    
    //GetChannelSetting(ch);

    printf("Done. Threshold of ch-%d is %d now.\n", ch, threshold);
    
  }else{
    printf("fail. something wrong.\n");
  }
  return ret;
}

int Digitizer::SetChannelParity(int ch, bool isPositive){
  
  if ( isPositive ){
    PulsePolarity[ch] = CAEN_DGTZ_PulsePolarityPositive; 
    printf(" Set channel %d to be + parity pulse. \n", ch);
  }else{
    PulsePolarity[ch] = CAEN_DGTZ_PulsePolarityNegative; 
    printf(" Set channel %d to be - parity pulse. \n", ch);
  }
  
  ret |= CAEN_DGTZ_SetChannelPulsePolarity(boardID, ch, PulsePolarity[ch]);
  
  return ret;
}

void Digitizer::ClearRawData(){
  rawEnergy.clear();
  rawChannel.clear();
  rawTimeStamp.clear();
  rawEvCount = 0;
}
  
void Digitizer::ClearData(){
  Energy.clear();
  Channel.clear();
  TimeStamp.clear();
  
  for( int k = 0; k < MaxNChannels ; k++)  countNChannelEvent[k] = 0;

  countEventBuilt = 0;
}

void Digitizer::SetChannelMask(uint32_t mask){ 
  ChannelMask = mask; 
  if( isConnected ){
    ret = CAEN_DGTZ_SetChannelEnableMask(boardID, ChannelMask);
    if( ret == 0 ){
      printf("---- ChannelMask changed to %d \n", ChannelMask);
    }else{
      printf("---- Fail to change ChannelMask \n");
    }
  }
}


void Digitizer::SetDCOffset(int ch, float offset){
  DCOffset[ch] = offset;
  
  ret = CAEN_DGTZ_SetChannelDCOffset(boardID, ch, uint( 0xffff * DCOffset[ch] ));
  
  if( ret == 0 ){
    printf("---- DC Offset of CH : %d is set to %f \n", ch, DCOffset[ch]);
  }else{
    printf("---- Fail to Set DC Offset of CH : %d \n", ch);
  }
  
}



void Digitizer::GetBoardConfiguration(){
  uint32_t * value = new uint32_t[1];
  CAEN_DGTZ_ReadRegister(boardID, 0x8000 , value);
  printf("                        32  28  24  20  16  12   8   4   0\n");
  printf("                         |   |   |   |   |   |   |   |   |\n");
  cout <<" Board Configuration  : 0b" << bitset<32>(value[0]) << endl;
  printf("                Bit[ 0] = Auto Data Flush   \n");
  printf("                Bit[16] = WaveForm Recording   \n");
  printf("                Bit[17] = Extended Time Tag   \n");
  printf("                Bit[18] = Record Time Stamp   \n");
  printf("                Bit[19] = Record Energy   \n");
  printf("====================================== \n");
}

void Digitizer::GetChannelSetting(int ch){
  
  uint32_t * value = new uint32_t[MaxNChannels];
  printf("================================================\n");
  printf("================ Getting setting for channel %d \n", ch);
  printf("================================================\n");
  //DPP algorithm Control
  CAEN_DGTZ_ReadRegister(boardID, 0x1080 + (ch << 8), value);
  printf("                          32  28  24  20  16  12   8   4   0\n");
  printf("                           |   |   |   |   |   |   |   |   |\n");
  cout <<" DPP algorithm Control  : 0b" << bitset<32>(value[0]) << endl;
  
  int trapRescaling = int(value[0]) & 31 ;
  int polarity = int(value[0] >> 16); //in bit[16]
  int baseline = int(value[0] >> 20) ; // in bit[22:20]
  int NsPeak = int(value[0] >> 12); // in bit[13:12]
  //DPP algorithm Control 2
  CAEN_DGTZ_ReadRegister(boardID, 0x10A0 + (ch << 8), value);
  cout <<" DPP algorithm Control 2: 0b" << bitset<32>(value[0]) << endl;
  
  printf("* = multiple of 8 \n");
  
  printf("==========----- input \n");
  CAEN_DGTZ_ReadRegister(boardID, 0x1020 + (ch << 8), value); printf("%20s  %d \n", "Record Length",  value[0] * 8); //Record length
  CAEN_DGTZ_ReadRegister(boardID, 0x1038 + (ch << 8), value); printf("%20s  %d \n", "Pre-tigger",  value[0] * 4); //Pre-trigger
  printf("%20s  %s \n", "polarity",  (polarity & 1) ==  0 ? "Positive" : "negative"); //Polarity
  printf("%20s  %.0f sample \n", "Ns baseline",  pow(4, 1 + baseline & 7)); //Ns baseline
  CAEN_DGTZ_ReadRegister(boardID, 0x1098 + (ch << 8), value); printf("%20s  %.2f %% \n", "DC offset",  value[0] * 100./ int(0xffff) ); //DC offset
  CAEN_DGTZ_ReadRegister(boardID, 0x1028 + (ch << 8), value); printf("%20s  %.1f Vpp \n", "input Dynamic",  value[0] == 0 ? 2 : 0.5); //InputDynamic
  
  printf("==========----- discriminator \n");
  CAEN_DGTZ_ReadRegister(boardID, 0x106C + (ch << 8), value); printf("%20s  %d LSB\n", "Threshold",  value[0]); //Threshold
  CAEN_DGTZ_ReadRegister(boardID, 0x1074 + (ch << 8), value); printf("%20s  %d ns \n", "trigger hold off *",  value[0] * 8); //Trigger Hold off
  CAEN_DGTZ_ReadRegister(boardID, 0x1054 + (ch << 8), value); printf("%20s  %d sample \n", "Fast Dis. smoothing",  value[0] *2 ); //Fast Discriminator smoothing
  CAEN_DGTZ_ReadRegister(boardID, 0x1058 + (ch << 8), value); printf("%20s  %d ch \n", "Input rise time *",  value[0] * 8); //Input rise time
  
  printf("==========----- Trapezoid \n");
  CAEN_DGTZ_ReadRegister(boardID, 0x1080 + (ch << 8), value); printf("%20s  %d bit = Floor( rise * decay / 64 )\n", "Trap. Rescaling",  trapRescaling ); //Trap. Rescaling Factor
  CAEN_DGTZ_ReadRegister(boardID, 0x105C + (ch << 8), value); printf("%20s  %d ns \n", "Trap. rise time *",  value[0] * 8 ); //Trap. rise time
  CAEN_DGTZ_ReadRegister(boardID, 0x1060 + (ch << 8), value); 
  int flatTopTime = value[0] * 8;
  printf("%20s  %d ns \n", "Trap. flat time *",  flatTopTime); //Trap. flat time
  CAEN_DGTZ_ReadRegister(boardID, 0x1020 + (ch << 8), value); printf("%20s  %d ns \n", "Trap. pole zero *",  value[0] * 8); //Trap. pole zero
  CAEN_DGTZ_ReadRegister(boardID, 0x1068 + (ch << 8), value); printf("%20s  %d ns \n", "Decay time *",  value[0] * 8); //Trap. pole zero
  CAEN_DGTZ_ReadRegister(boardID, 0x1064 + (ch << 8), value); printf("%20s  %d ns = %.2f %% \n", "peaking time *",  value[0] * 8, value[0] * 800. / flatTopTime ); //Peaking time
  printf("%20s  %.0f sample\n", "Ns peak",  pow(4, NsPeak & 3)); //Ns peak
  CAEN_DGTZ_ReadRegister(boardID, 0x1078 + (ch << 8), value); printf("%20s  %d ns \n", "Peak hole off*",  value[0] * 8 ); //Peak hold off
  
  printf("==========----- Other \n");
  CAEN_DGTZ_ReadRegister(boardID, 0x104C + (ch << 8), value); printf("%20s  %d \n", "Energy fine gain",  value[0]); //Energy fine gain
  
  printf("========================================= end of ch-%d\n", ch);
  
}

int Digitizer::ProgramDigitizer(){
  /* This function uses the CAENDigitizer API functions to perform the digitizer's initial configuration */
    int ret = 0;

    /* Reset the digitizer */
    ret |= CAEN_DGTZ_Reset(boardID);

    if (ret) {
        printf("ERROR: can't reset the digitizer.\n");
        return -1;
    }
    
    ret |= CAEN_DGTZ_WriteRegister(boardID, 0x8000, 0x01000114);  // Channel Control Reg (indiv trg, seq readout) ??

    /* Set the DPP acquisition mode
    This setting affects the modes Mixed and List (see CAEN_DGTZ_DPP_AcqMode_t definition for details)
    CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly        Only energy (DPP-PHA) or charge (DPP-PSD/DPP-CI v2) is returned
    CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly        Only time is returned
    CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime    Both energy/charge and time are returned
    CAEN_DGTZ_DPP_SAVE_PARAM_None            No histogram data is returned */
    ret |= CAEN_DGTZ_SetDPPAcquisitionMode(boardID, AcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
    
    // Set the digitizer acquisition mode (CAEN_DGTZ_SW_CONTROLLED or CAEN_DGTZ_S_IN_CONTROLLED)
    ret |= CAEN_DGTZ_SetAcquisitionMode(boardID, CAEN_DGTZ_SW_CONTROLLED);
    
    // Set the number of samples for each waveform
    ret |= CAEN_DGTZ_SetRecordLength(boardID, RecordLength);

    // Set the I/O level (CAEN_DGTZ_IOLevel_NIM or CAEN_DGTZ_IOLevel_TTL)
    ret |= CAEN_DGTZ_SetIOLevel(boardID, IOlev);

    /* Set the digitizer's behaviour when an external trigger arrives:

    CAEN_DGTZ_TRGMODE_DISABLED: do nothing
    CAEN_DGTZ_TRGMODE_EXTOUT_ONLY: generate the Trigger Output signal
    CAEN_DGTZ_TRGMODE_ACQ_ONLY = generate acquisition trigger
    CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT = generate both Trigger Output and acquisition trigger

    see CAENDigitizer user manual, chapter "Trigger configuration" for details */
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(boardID, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

    // Set the enabled channels
    ret |= CAEN_DGTZ_SetChannelEnableMask(boardID, ChannelMask);

    // Set how many events to accumulate in the board memory before being available for readout
    ret |= CAEN_DGTZ_SetDPPEventAggregation(boardID, EventAggr, 0);
    
    /* Set the mode used to syncronize the acquisition between different boards.
    In this example the sync is disabled */
    ret |= CAEN_DGTZ_SetRunSynchronizationMode(boardID, CAEN_DGTZ_RUN_SYNC_Disabled);
    
    // Set the DPP specific parameters for the channels in the given channelMask
    ret |= CAEN_DGTZ_SetDPPParameters(boardID, ChannelMask, &DPPParams);
    
    // Set Extras 2 to enable, this override Accusition mode, focring list mode
    uint32_t value = 0x10E0114;
    ret |= CAEN_DGTZ_WriteRegister(boardID, 0x8000 , value );
    
    for(int i=0; i<MaxNChannels; i++) {
        if (ChannelMask & (1<<i)) {
            /// Set a DC offset to the input signal to adapt it to digitizer's dynamic range
            ///ret |= CAEN_DGTZ_SetChannelDCOffset(boardID, i, 0x3333); // 20%
            ret |= CAEN_DGTZ_SetChannelDCOffset(boardID, i, uint( 0xffff * DCOffset[i] ));
            
            /// Set the Pre-Trigger size (in samples)
            ret |= CAEN_DGTZ_SetDPPPreTriggerSize(boardID, i, PreTriggerSize[i]);
            
            /// Set the polarity for the given channel (CAEN_DGTZ_PulsePolarityPositive or CAEN_DGTZ_PulsePolarityNegative)
            ret |= CAEN_DGTZ_SetChannelPulsePolarity(boardID, i, PulsePolarity[i]);
            
            /// Set InputDynamic Range
            ret |= CAEN_DGTZ_WriteRegister(boardID, 0x1028 +  (i<<8), inputDynamicRange[i]);
            
            /// Set Energy Fine gain
            ret |= CAEN_DGTZ_WriteRegister(boardID, 0x104C +  (i<<8), energyFineGain[i]);
            
            /// read the register to check the input is correct
            ///uint32_t * value = new uint32_t[8];
            ///ret = CAEN_DGTZ_ReadRegister(boardID, 0x1028 + (i << 8), value);
            ///printf(" InputDynamic Range (ch:%d): %d \n", i, value[0]);
        }
    }

    ///ret |= CAEN_DGTZ_SetDPP_VirtualProbe(boardID, ANALOG_TRACE_1, CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2);
    ///ret |= CAEN_DGTZ_SetDPP_VirtualProbe(boardID, ANALOG_TRACE_2, CAEN_DGTZ_DPP_VIRTUALPROBE_Input);
    ///ret |= CAEN_DGTZ_SetDPP_VirtualProbe(boardID, DIGITAL_TRACE_1, CAEN_DGTZ_DPP_DIGITALPROBE_Peaking);

    if (ret) {
        printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");
        return ret;
    } else {
        return 0;
    }
  
}

void Digitizer::LoadChannelSetting (const int ch, string fileName) {
  
  ifstream file_in;
  file_in.open(fileName.c_str(), ios::in);

  if( !file_in){
    printf("channel: %d | default.\n", ch);
    DPPParams.thr[ch]   = 100;      // Trigger Threshold (in LSB)
    DPPParams.trgho[ch] = 1200;     // Trigger Hold Off
    DPPParams.a[ch]     = 4;        // Trigger Filter smoothing factor (number of samples to average for RC-CR2 filter) Options: 1; 2; 4; 8; 16; 32
    DPPParams.b[ch]     = 200;      // Input Signal Rise time (ns) 
    
    DPPParams.k[ch]    = 3000;     // Trapezoid Rise Time (ns) 
    DPPParams.m[ch]    = 900;      // Trapezoid Flat Top  (ns) 
    DPPParams.M[ch]    = 50000;    // Decay Time Constant (ns) 
    DPPParams.ftd[ch]  = 500;      // Flat top delay (peaking time) (ns) 
    DPPParams.nspk[ch] = 0;        // Peak mean (number of samples to average for trapezoid height calculation). Options: 0-> 1 sample; 1->4 samples; 2->16 samples; 3->64 samples
    DPPParams.pkho[ch] = 2000;    // peak holdoff (ns)
    
    DPPParams.nsbl[ch]    = 4;        // number of samples for baseline average calculation. Options: 1->16 samples; 2->64 samples; 3->256 samples; 4->1024 samples; 5->4096 samples; 6->16384 samples
    inputDynamicRange[ch] = 0;       // input dynamic range, 0 = 2 Vpp, 1 = 0.5 Vpp

    energyFineGain[ch]       = 10;      // Energy Fine gain
    DPPParams.blho[ch]       = 500;     // Baseline holdoff (ns)        
    DPPParams.enf[ch]        = 1.0;     // Energy Normalization Factor
    DPPParams.decimation[ch] = 0;       // decimation (the input signal samples are averaged within this number of samples): 0 ->disabled; 1->2 samples; 2->4 samples; 3->8 samples
    DPPParams.dgain[ch]      = 0;       // decimation gain. Options: 0->DigitalGain=1; 1->DigitalGain=2 (only with decimation >= 2samples); 2->DigitalGain=4 (only with decimation >= 4samples); 3->DigitalGain=8( only with decimation = 8samples).
    DPPParams.trgwin[ch]     = 0;       // Enable Rise time Discrimination. Options: 0->disabled; 1->enabled
    DPPParams.twwdt[ch]      = 100;     // Rise Time Validation Window (ns)
    
    chGain[ch] = -1.0;      // gain of the channel; if -1, default based on input-dynamic range;
    
  }else{
    printf("channel: %d | %s.\n", ch, fileName.c_str());
    string line;
    int count = 0;
    while( file_in.good()){
      getline(file_in, line);
      size_t pos = line.find("//");
      if( pos > 1 ){
        if( count ==  0 ) DPPParams.thr[ch]        = atoi(line.substr(0, pos).c_str());
        if( count ==  1 ) DPPParams.trgho[ch]      = atoi(line.substr(0, pos).c_str());
        if( count ==  2 ) DPPParams.a[ch]          = atoi(line.substr(0, pos).c_str());
        if( count ==  3 ) DPPParams.b[ch]          = atoi(line.substr(0, pos).c_str());
        if( count ==  4 ) DPPParams.k[ch]          = atoi(line.substr(0, pos).c_str());
        if( count ==  5 ) DPPParams.m[ch]          = atoi(line.substr(0, pos).c_str());
        if( count ==  6 ) DPPParams.M[ch]          = atoi(line.substr(0, pos).c_str());
        if( count ==  7 ) DPPParams.ftd[ch]        = atoi(line.substr(0, pos).c_str());
        if( count ==  8 ) DPPParams.nspk[ch]       = atoi(line.substr(0, pos).c_str());
        if( count ==  9 ) DPPParams.pkho[ch]       = atoi(line.substr(0, pos).c_str());
        if( count == 10 ) DPPParams.nsbl[ch]       = atoi(line.substr(0, pos).c_str());
        if( count == 11 ) inputDynamicRange[ch]    = atoi(line.substr(0, pos).c_str());
        if( count == 12 ) DCOffset[ch]             = atof(line.substr(0, pos).c_str());
        if( count == 13 ) PreTriggerSize[ch]       = atoi(line.substr(0, pos).c_str());
        if( count == 14 ) PulsePolarity[ch] = (line.substr(0, 4) == "true") ? CAEN_DGTZ_PulsePolarityPositive : CAEN_DGTZ_PulsePolarityNegative;
        if( count == 15 ) energyFineGain[ch]       = atoi(line.substr(0, pos).c_str());
        if( count == 16 ) DPPParams.blho[ch]       = atoi(line.substr(0, pos).c_str());
        if( count == 17 ) DPPParams.enf[ch]        = atof(line.substr(0, pos).c_str());
        if( count == 18 ) DPPParams.decimation[ch] = atoi(line.substr(0, pos).c_str());
        if( count == 19 ) DPPParams.dgain[ch]      = atoi(line.substr(0, pos).c_str());
        if( count == 20 ) DPPParams.trgwin[ch]     = atoi(line.substr(0, pos).c_str());
        if( count == 21 ) DPPParams.twwdt[ch]      = atoi(line.substr(0, pos).c_str());
        count++;
      }
    }
  }
  
};

void Digitizer::LoadGeneralSetting(string fileName){
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
        if( count == 0  )   RecordLength = atoi(line.substr(0, pos).c_str());
        if( count == 1  )   CoincidentTimeWindow = atoi(line.substr(0, pos).c_str());
        count++;
      }
    }
    
    //=============== print setting
    printf(" %-20s  %d ch\n", "Record Lenght", RecordLength);
    printf("====================================== \n");
    
  }

  return;
  
}

void Digitizer::ZeroSingleEvent(){
  if( NChannel != 0 ) {
    singleChannel.clear();
    singleEnergy.clear();
    singleTimeStamp.clear();
    for( int i = 0; i < NChannel ; i++){
      singleEnergy.push_back(0);
      singleChannel.push_back(-1);
      singleTimeStamp.push_back(0);
    }
  }
}


void Digitizer::StartACQ(){
  
  CAEN_DGTZ_SWStartAcquisition(boardID);
  printf("Acquisition Started for Board %d\n", boardID);
  AcqRun = true;
  
}

void Digitizer::ReadData(){
  /* Read data from the board */
  ret = CAEN_DGTZ_ReadData(boardID, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
  if (BufferSize == 0) return;
  
  Nb = BufferSize;
  ret |= (CAEN_DGTZ_ErrorCode) CAEN_DGTZ_GetDPPEvents(boardID, buffer, BufferSize, reinterpret_cast<void**>(&Events), NumEvents);
  
  if (ret) {
    printf("Data Error: %d\n", ret);
    CAEN_DGTZ_SWStopAcquisition(boardID);
    CAEN_DGTZ_CloseDigitizer(boardID);
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    CAEN_DGTZ_FreeDPPEvents(boardID, reinterpret_cast<void**>(&Events));
    return;
  }
  
  /* Analyze data */
  rawEvCount = 0;
  for (int ch = 0; ch < MaxNChannels; ch++) {    
    if (!(ChannelMask & (1<<ch))) continue;
    //printf("------------------------ %d \n", ch);
    for (int ev = 0; ev < NumEvents[ch]; ev++) {
      TrgCnt[ch]++;
      
      if (Events[ch][ev].Energy > 0 && Events[ch][ev].TimeTag > 0 ) {
        ECnt[ch]++;
          
        ULong64_t timetag = (ULong64_t) Events[ch][ev].TimeTag;
        ULong64_t rollOver = Events[ch][ev].Extras2 >> 16;
        rollOver = rollOver << 31;
        timetag  += rollOver ;
        
        //printf("%llu | %llu | %llu \n", Events[ch][ev].Extras2 , rollOver >> 32, timetag);
        
        rawChannel.push_back(ch);
        rawEnergy.push_back(Events[ch][ev].Energy);
        rawTimeStamp.push_back(timetag);
        
        rawEvCount = rawChannel.size();
        
      } else { /* PileUp */
          PurCnt[ch]++;
      }
        
    } // loop on events
    
  } // loop on channels
  
}

void Digitizer::PrintReadStatistic(){
  
  printf("####### Board ID = %d \n", boardID);
  uint64_t ElapsedTime = rawTimeRange * ch2ns * 1e-6; // in mili-sec
  printf(" Readout Rate = %.5f MB/s\n", (float)Nb/((float)ElapsedTime*1048.576f));
      
  printf("     | %7s| %12s| %8s\n", "Get", "TrgRate [Hz]", "PileUp");
  for(int i = 0; i < MaxNChannels; i++) {
    if (!(ChannelMask & (1<<i))) continue;
    if (TrgCnt[i]>0){
      printf(" Ch %d| %7d| %12.2f| %7.2f%%\n", i, ECnt[i], (float)TrgCnt[i]/(float)ElapsedTime *1000., (float)PurCnt[i]*100/(float)TrgCnt[i]);
    }else{
      if (!(ChannelMask & (1<<i))){
        printf(" Ch %d|\tMasked\n", i);
      }else{
        printf(" Ch %d|\tNo Data\n", i);
      }
    }
  }
  printf("-----------------------------------\n");
  printf("total| %7d \n", rawEvCount);
  
  /*
  printf("                        ");
  for( int ch =0 ; ch < MaxNChannels; ch++) printf(" %3d|", ch);
  printf("\n");
  printf("Count for each Channel: ");
  for( int ch =0 ; ch < MaxNChannels; ch++) printf(" %3d|", ECnt[ch]);
  printf("\n");
    */
  for (int ch = 0; ch < MaxNChannels; ch++) {
    TrgCnt[ch] = 0;
    ECnt[ch] = 0;
    PurCnt[ch] = 0;
  }

}

void Digitizer::PrintEventBuildingStat(int updatePeriod){
  printf("===============================================\n");
  //printf("Number of retrieving = %d = %.2f per sec\n", rawEvCount, rawEvCount*1000./updatePeriod);
  printf(" %5s| %5s| %5s| \n", "#ch", "Built", "Total");
  //printf("-----------------------------------\n");
  for( int k = 0; k < MaxNChannels-1 ; k ++){
    printf(" %5d| %5d| %5d|\n", k+1, countNChannelEvent[k], totNChannelEvent[k]);
  }
  printf(" %5d| %5d| %5d| %5s\n", MaxNChannels, countNChannelEvent[MaxNChannels-1], totNChannelEvent[MaxNChannels-1], "left");
  printf("-----------------------------------\n");
  printf(" %5s| %5d| %5d| %5d\n", "total", countEventBuilt, totEventBuilt, rawEvLeftCount);
  printf("===============================================\n");
}

void Digitizer::StopACQ(){
  
  CAEN_DGTZ_SWStopAcquisition(boardID); 
  printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
  AcqRun = false;
}

int Digitizer::BuildEvent(bool debug = false){
  
  //################################################################
  //  Sorrting raw event timeStamp
  //################################################################
  int nRawData = rawChannel.size();
  if( nRawData < 2 ) return 0; // too few event to build;
  
  countEventBuilt = 0;
  
  int sortIndex[nRawData];
  double bubbleSortTime[nRawData];
  for( int i = 0; i < nRawData; i++){
    bubbleSortTime[i] = double(rawTimeStamp[i]/1e12);
    //printf("%d, %d,  %llu \n", i,rawEnergy[i], rawTimeStamp[i]);
  }
  
  TMath::BubbleLow(nRawData,bubbleSortTime,sortIndex);
  //=======Re-map
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
    if( debug) printf("%d| %d,  %llu, %d  \n", i, rawChannel[i], rawTimeStamp[i], rawEnergy[i]);
  }
  
  if( nRawData > 0 ) {
    rawTimeRange = rawTimeStamp[nRawData-1] - rawTimeStamp[0];
  }else{
    rawTimeRange = 99999999999.;
  }
  //################################################################
  // build event base on coincident window
  //################################################################

  if (debug) printf("=========================\n");
  for( int k = 0; k < MaxNChannels ; k++) countNChannelEvent[k] = 0;
  int endID = 0;
  for( int i = 0; i < nRawData-1; i++){
    ULong64_t timeToEnd = (rawTimeStamp[nRawData-1] - rawTimeStamp[i]) * ch2ns ; // in nano-sec
    endID = i;
    //printf(" time to end %d / %d , %d, %d\n", timeToEnd, CoincidentTimeWindow, i , endID);
    if( timeToEnd < CoincidentTimeWindow ) {
      break;
    }
    
    int digitID = 1 << rawChannel[i]; // for checking if the Channel[i] is already taken.
    
    /*
    //if too may same channel event in a sequence, break, probably other channels not read.
    bool breakFlag = false;
    int breakForSameChannel = 0;
    for( int j = i+1; j < nRawData; j++){
      //check is channel[j] is taken or not
      unsigned int x = 1 << rawChannel[j];
      unsigned int y = digitID ^ x; // bitwise XOR, 00=0, 01=1, 10=1, 11=0 
      unsigned int z = 1 & (y >> rawChannel[j]); // if z = 0, the channel already token.
      unsigned long long int timeDiff = (rawTimeStamp[j] - rawTimeStamp[i]) * ch2ns;
      
      if( timeDiff > CoincidentTimeWindow && z == 0){
        breakForSameChannel ++;
        if( breakForSameChannel > 3 ) {
          breakFlag = true;
          break;
        }
      }else{
        if( breakForSameChannel <=3 ){
          break;
        }
      }
    }
    if( breakFlag ) break;
    /**///----------------- end of check
    
    int numRawEventGrouped = 0;

    
    if( debug) printf("%3d | %d | %d, %llu, %d, %d \n", digitID, rawChannel[i], 0, rawTimeStamp[i], 0, rawEnergy[i]); 
    for( int j = i+1; j < nRawData; j++){
      
      //check is channel[j] is taken or not
      unsigned int x = 1 << rawChannel[j];
      unsigned int y = digitID ^ x; // bitwise XOR, 00=0, 01=1, 10=1, 11=0 
      unsigned int z = 1 & (y >> rawChannel[j]); // if z = 0, the channel already token.
                  
      unsigned long long int timeDiff = (rawTimeStamp[j] - rawTimeStamp[i]) * ch2ns;
      
      digitID += x;
      
      if( timeDiff < CoincidentTimeWindow ){
        // if channel already taken
        //if( z == 0 ) { 
        //  breakForSameChannel ++;
        //  //break;
        //}
        numRawEventGrouped ++;
      
      }else{
        if( debug) printf("---- %d/ %d,  num in Group : %d | %d\n", i, nRawData,  numRawEventGrouped, CoincidentTimeWindow);
        // normal exit when next event outside coincident window
        break;
      }
      
      if(debug) printf("%3d | %d | %d, %llu, %llu, %d\n", digitID, rawChannel[j], z, rawTimeStamp[j], timeDiff, rawEnergy[j]); 
      
    }
    
    // when chTAC is single, skip.
    //if( numRawEventGrouped == 0 && rawChannel[i] == chTAC) continue;
    
    switch (numRawEventGrouped){
      case 0: countNChannelEvent[0] += 1; totNChannelEvent[0] += 1; break;
      case 1: countNChannelEvent[1] += 1; totNChannelEvent[1] += 1; break;
      case 2: countNChannelEvent[2] += 1; totNChannelEvent[2] += 1; break;
      case 3: countNChannelEvent[3] += 1; totNChannelEvent[3] += 1; break;
      case 4: countNChannelEvent[4] += 1; totNChannelEvent[4] += 1; break;
      case 5: countNChannelEvent[5] += 1; totNChannelEvent[5] += 1; break;
      case 6: countNChannelEvent[6] += 1; totNChannelEvent[6] += 1; break;
      case 7: countNChannelEvent[7] += 1; totNChannelEvent[7] += 1; break;
    }
     
    if( debug){ 
      printf("============");
      for( int k = 0; k < MaxNChannels ; k++) printf(" %d, ", countNChannelEvent[k]);
      printf("\n");
    } 
    countEventBuilt ++;
    
    //fill in an event
    ZeroSingleEvent();
    for( int j = i ; j <= i + numRawEventGrouped ; j++){          
      singleChannel[rawChannel[j]] = rawChannel[j];
      singleEnergy[rawChannel[j]] = rawEnergy[j];
      singleTimeStamp[rawChannel[j]] = rawTimeStamp[j];
    }
    
    totEventBuilt++;
    
    Channel.push_back(singleChannel);
    Energy.push_back(singleEnergy);
    TimeStamp.push_back(singleTimeStamp);
    
    i += numRawEventGrouped ; 
    
  }/**/// end of event building
  //################################################################
  
  //clear vectors but keep from endID
  rawChannel.erase(rawChannel.begin(), rawChannel.begin() + endID  );
  rawEnergy.erase(rawEnergy.begin(), rawEnergy.begin() + endID );
  rawTimeStamp.erase(rawTimeStamp.begin(), rawTimeStamp.begin() + endID );
 
  rawEvLeftCount = rawChannel.size();
  
  return 1; // for sucessful
 
}

#endif
