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
#define MaxDataAShot 100000 // also limited by Timing, channel, energy pointer initialization.
                        
using namespace std;

enum DigiReg {
  recordLength = 0x1020,
  preTrigger   = 0x1038,
  dcOffset     = 0x1098,
  dynamicRange = 0x1028,
  threshold    = 0x106C,
  trigHoldOff  = 0x1074
  
  //TODO fill all register
};


class Digitizer{
  RQ_OBJECT("Digitizer") 
public:
  Digitizer(int ID, uint32_t ChannelMask);
  ~Digitizer();
  
  void SetChannelMask(bool ch7, bool ch6, bool ch5, bool ch4, bool ch3, bool ch2, bool ch1, bool ch0);
  void SetChannelMask(uint32_t mask);
  void SetDCOffset(int ch , float offset);
  void SetCoincidentTimeWindow(int nanoSec) { CoincidentTimeWindow = nanoSec;}
  int  SetChannelParity(int ch, bool isPositive);
  int  SetChannelThreshold(int ch, string folder, int threshold);
  int  SetChannelDynamicRange(int ch, string folder, int dyRange);
  int  SetAcqMode(string mode, int recordLength);
  //void SetRegister(uin32_t address, int ch, float value); 
  //void SetRegister(DigiReg regName, int ch, float value); 
  
  bool     IsConnected()                {return isConnected;} /// can connect and retrieve Digitizer Info.
  bool     IsDetected()                 {return isDetected;}      /// can detect digitizer
  bool     IsRunning()                  {return AcqRun;}
  int      GetByteRetrived()            {return Nb;}
  int      GetInputDynamicRange(int ch) {return inputDynamicRange[ch];}
  int      GetNChannel()                {return NChannel;}
  int      GetNChannelOpen()            {return nChannelOpen;}
  int *    GetInputDynamicRange()       {return inputDynamicRange;}
  float *  GetChannelGain()             {return chGain;}
  float    GetChannelGain(int ch)       {return chGain[ch];}
  int      GetChannelToNanoSec()        {return ch2ns;};
  uint32_t GetChannelThreshold(int ch);
  int      GetChannelDynamicRange(int ch);
  
  string   GetAcqMode()                 { return AcqMode == 1  ?  "list" :  "mixed" ;}
  uint32_t GetRecordLength()            { return RecordLength;}
  int      GetWaveFormLength(int ch)    { return waveformLength[ch];}
  int16_t* GetWaveForm(int ch)          { return WaveLine[ch];}
  uint8_t* GetDigitalWaveLine(int ch)   { return DigitalWaveLine[ch];}
  
  int*      GetWaveFormLengths()        { return waveformLength;}
  int16_t** GetWaveForms()              { return WaveLine;}
  
  unsigned long long int Getch2ns()     {return ch2ns;}
  int      GetCoincidentTimeWindow()    {return CoincidentTimeWindow;}
  uint32_t GetChannelMask() const       {return ChannelMask;}
  
  void GetBoardConfiguration();
  void GetChannelSetting(int ch);
  int  GetSerialNumber() {return serialNumber;}
  
  //======== Get Raw Data
  int          GetNumRawEvent()       {return rawEvCount + rawEvLeftCount;}
  ULong64_t *  GetRawTimeStamp()      {return rawTimeStamp;}
  UInt_t*      GetRawEnergy()         {return rawEnergy;}
  int *        GetRawChannel()        {return rawChannel;}
  uint32_t     GetRawTimeRange()      {return rawTimeRange;}     // in ch
  ULong64_t    GetRawTimeStamp(int i) {return rawTimeStamp[i];}
  UInt_t       GetRawEnergy(int i)    {return rawEnergy[i];}
  int          GetRawChannel(int i)   {return rawChannel[i];}
  
  void ClearRawData(); /// clear Raw Data and set rawEvCount = 0;
  void ClearData();    /// clear built event vectors, and set countEventBuild =  0;

  int   GetEventBuilt()                 {return countEventBuilt; }
  int   GetEventBuiltCount()            {return countEventBuilt;}
  int   GetTotalEventBuilt()            {return totEventBuilt; }
  int   GetNChannelEventCount(int Nch)  {return countNChannelEvent[Nch-1];}
  int * GetNChannelEventCount()         {return countNChannelEvent;}
  int   GetTotalNChannelEvent(int Nch)  {return totNChannelEvent[Nch-1];}
  
  //======== Get built event
  ULong64_t * GetTimeStamp(int ev)        {return TimeStamp[ev];}
  UInt_t *    GetEnergy(int ev)           {return Energy[ev];}
  int *       GetChannel(int ev)          {return Channel[ev];}
  ULong64_t   GetTimeStamp(int ev, int ch){return TimeStamp[ev][ch];}
  UInt_t      GetEnergy(int ev, int ch)   {return Energy[ev][ch];}
  int         GetChannel(int ev, int ch)  {return Channel[ev][ch];}
  
  //========= Digitizer Control
  int  ProgramDigitizer();
  void LoadChannelSetting (const int ch, string fileName);
  void LoadGeneralSetting(string fileName);
  
  void StopACQ();
  void StartACQ();
  
  void ReadData(bool debug);
  int  BuildEvent(bool debug);
  
  void PrintReadStatistic();
  void PrintEventBuildingStat(int updatePeriod);
  void PrintDynamicRange();
  void PrintThreshold();
  void PrintThresholdAndDynamicRange();

private:

  bool isConnected; /// can get digitizer info
  bool isDetected;      /// can open digitizer
  bool AcqRun;      /// is digitizer taking data
  
  int serialNumber;

  int boardID;      /// board identity
  int handle;       /// i don't know why, but better separete the handle from boardID
  int ret;          /// return value, refer to CAEN_DGTZ_ErrorCode
  int NChannel;     /// number of channel
  int nChannelOpen; /// number of open channel

  int Nb;                                  /// number of byte
  char *buffer = NULL;                     /// readout buffer
  uint32_t NumEvents[MaxNChannels];
  uint32_t AllocatedSize, BufferSize; 
  CAEN_DGTZ_DPP_PHA_Event_t  *Events[MaxNChannels];  /// events buffer
  CAEN_DGTZ_DPP_PHA_Waveforms_t   *Waveform;     /// waveforms buffer

  int16_t *WaveLine[MaxNChannels];
  uint8_t *DigitalWaveLine[MaxNChannels];
  int waveformLength[MaxNChannels];

  //====================== Channel Setting
  CAEN_DGTZ_DPP_PHA_Params_t DPPParams;
  CAEN_DGTZ_PulsePolarity_t  PulsePolarity[MaxNChannels];   
  int   inputDynamicRange[MaxNChannels];
  int   energyFineGain[MaxNChannels];
  float chGain[MaxNChannels];
  uint  PreTriggerSize[MaxNChannels];     
  float DCOffset[MaxNChannels];  
  
  //====================== General Setting
  unsigned long long int ch2ns;    
  uint32_t VMEBaseAddress;
  CAEN_DGTZ_ConnectionType LinkType;
  CAEN_DGTZ_IOLevel_t IOlev;
  
  CAEN_DGTZ_DPP_AcqMode_t AcqMode;
  uint32_t ChannelMask;                      /// Channel enable mask, 0x01, only frist channel, 0xff, all channel  
  uint32_t RecordLength;                     /// Num of samples of the waveforms (only for waveform mode)
  int EventAggr;                             /// number of events in one aggregate (0=automatic), number of event acculated for read-off
 
  int CoincidentTimeWindow;  /// nano-sec

  //==================== retreved data
  int ECnt[MaxNChannels];
  int TrgCnt[MaxNChannels];
  int PurCnt[MaxNChannels];
  int rawEvCount;
  int rawEvLeftCount;
  uint64_t rawTimeRange;
  
  //===== unsorted data
  ULong64_t* rawTimeStamp;
  UInt_t* rawEnergy;
  int* rawChannel;
  
  //===== builded event
  int countEventBuilt;
  int totEventBuilt;
  int countNChannelEvent[MaxNChannels];
  int totNChannelEvent[MaxNChannels];
  
  //==== data for single event
  ULong64_t * singleTimeStamp;   
  UInt_t * singleEnergy;
  int * singleChannel;
  
  //==== data for a shot
  int ** Channel;
  UInt_t ** Energy;
  ULong64_t ** TimeStamp;
  
  void ZeroSingleEvent();
  
  int CalNOpenChannel(uint32_t mask);
};

Digitizer::Digitizer(int ID, uint32_t ChannelMask){
  
  //================== initialization
  boardID  = ID;
  handle   = -1;
  NChannel = 0;
  AcqRun   = false;
  ch2ns    = 2; // 1 channel = 2 ns
  Nb       = 0;
  CoincidentTimeWindow = 200; // nano-sec
  for(int i = 0 ; i < MaxNChannels; i++ )waveformLength[i] = 0;
  
  //----------------- default channel setting
  for ( int i = 0; i < MaxNChannels ; i++ ) {
    DCOffset[i]          = 0.2;
    inputDynamicRange[i] = 0;
    chGain[i]            = 1.0;
    energyFineGain[i]    = 100;
    NumEvents[i]         = 0; 
    PreTriggerSize[i]    = 1000;
    PulsePolarity[i]     = CAEN_DGTZ_PulsePolarityPositive; 
  }
  
  memset(&DPPParams, 0, sizeof(CAEN_DGTZ_DPP_PHA_Params_t));
  
  //----------------- Communication Parameters 
  LinkType = CAEN_DGTZ_USB;     // Link Type
  VMEBaseAddress = 0;           // For direct USB connection, VMEBaseAddress must be 0
  IOlev = CAEN_DGTZ_IOLevel_NIM;
  
  //--------------Acquisition parameters 
  AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;  // default 
  RecordLength = 2000;

  this->ChannelMask = ChannelMask;
  CalNOpenChannel(ChannelMask);
  EventAggr = 1;       // Set how many events to accumulate in the board memory before being available for readout, 0 for auto

  //===================== end of initization

  /* *********************************************** */
  /* Open the digitizer and read board information   */
  /* *********************************************** */
  
  printf("============= Opening Digitizer at Board %d \n", boardID);
  
  isConnected = false;
  isDetected = true;
  
  ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, boardID, 0, VMEBaseAddress, &handle);
  if (ret != 0) {
    printf("Can't open digitizer\n");
    isDetected = false;
  }else{
    //----- Getting Board Info
    CAEN_DGTZ_BoardInfo_t BoardInfo;
    ret = (int) CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    if (ret != 0) {
      printf("Can't read board info\n");
      isDetected = false;
    }else{
      isConnected = true;
      printf("Connected to CAEN Digitizer Model %s, recognized as board %d with handle %d\n", BoardInfo.ModelName, boardID, handle);
      NChannel = BoardInfo.Channels;
      printf("Number of Channels : %d\n", NChannel);
      serialNumber = BoardInfo.SerialNumber;
      printf("SerialNumber :\e[1m\e[33m %d\e[0m\n", serialNumber);
      printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
      printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);
      
      int MajorNumber;
      sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
      if (MajorNumber != V1730_DPP_PHA_CODE) {
        printf("This digitizer does not have DPP-PHA firmware\n");
      }
    }
  }
  
  LoadGeneralSetting(to_string(serialNumber) + "/generalSetting.txt");
  
  /* *********************************************** */
  /* Get Channel Setting and Set Digitizer           */
  /* *********************************************** */
  if( isDetected ){
    printf("=================== reading Channel setting \n");
    for(int ch = 0; ch < NChannel; ch ++ ) {
      if ( ChannelMask & ( 1 << ch) ) {
        LoadChannelSetting(ch, to_string(serialNumber) +"/setting_" + to_string(ch) + ".txt");
      }
    }
    printf("====================================== \n");
  
    //============= Program the digitizer (see function ProgramDigitizer) 
    
    ret = (CAEN_DGTZ_ErrorCode)ProgramDigitizer();
    if (ret != 0) {
      printf("Failed to program the digitizer\n");
      isDetected = false;
    }
  }
  
  if( isDetected ) {
    /* WARNING: The mallocs MUST be done after the digitizer programming,
    because the following functions needs to know the digitizer configuration
    to allocate the right memory amount */
    // Allocate memory for the readout buffer 
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
    // Allocate memory for the events
    ret |= CAEN_DGTZ_MallocDPPEvents(handle, reinterpret_cast<void**>(&Events), &AllocatedSize) ;     
    // Allocate memory for the waveforms
    ret |= CAEN_DGTZ_MallocDPPWaveforms(handle, reinterpret_cast<void**>(&Waveform), &AllocatedSize); 
    

    if (ret != 0) {
      printf("Can't allocate memory buffers\n");
      CAEN_DGTZ_SWStopAcquisition(handle);
      CAEN_DGTZ_CloseDigitizer(handle);
      CAEN_DGTZ_FreeReadoutBuffer(&buffer);
      CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
    }else{
      printf("====== Allocated memory for communication.\n");
    }
    
  }
  
  printf("################### Digitizer is ready \n");
  
  rawEvCount= 0;
  rawEvLeftCount = 0;
  
  rawTimeRange = 999999999999999;
  
  singleEnergy = new UInt_t[MaxNChannels];
  singleChannel = new int[MaxNChannels];
  singleTimeStamp = new ULong64_t [MaxNChannels];
  
  rawTimeStamp = new ULong64_t [MaxDataAShot];
  rawEnergy    = new UInt_t [MaxDataAShot];
  rawChannel   = new int [MaxDataAShot];
  
  TimeStamp = new ULong64_t * [MaxDataAShot];
  Energy    = new UInt_t * [MaxDataAShot];
  Channel   = new int * [MaxDataAShot];
  
  for( int i = 0; i < MaxDataAShot ; i++){
    TimeStamp[i] = new ULong64_t [MaxNChannels];
    Energy[i]    = new UInt_t [MaxNChannels];
    Channel[i]   = new int [MaxNChannels];
  }
  
  ClearRawData();
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
  CAEN_DGTZ_SWStopAcquisition(handle);
  CAEN_DGTZ_CloseDigitizer(handle);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
  
  printf("Closed digitizer\n");
  printf("=========== bye bye ==============\n\n");
  
  for(int ch = 0; ch < MaxNChannels; ch++){
    delete Events[ch];
  }
  
  delete[] singleChannel;
  delete[] singleEnergy;
  delete[] singleTimeStamp;
 
  delete[] rawChannel;
  delete[] rawEnergy;
  delete[] rawTimeStamp; 
  
  delete buffer;
}

int Digitizer::SetAcqMode(string mode, int recordLength){
   
   this->RecordLength = recordLength;

   if( mode == "list"){
      AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;             // enables the acquisition of time stamps and energy value 
   }else if ( mode == "mixed"){
      AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;          // enables the acquisition of both waveforms, energies, and timestamps.
   }else{
      printf("############ AcqMode must be either list or mixed\n");
   }
   
   int ret = 0;
   if( isDetected ) {

      printf("Setting digitizer to %s mode with recordLenght = %d \n", mode.c_str(), RecordLength);
      
      //TODO take out the essential
      ret = ProgramDigitizer();
   }
   return ret;
}

int Digitizer::SetChannelThreshold(int ch, string folder, int threshold){
  
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x106C +  (ch<<8), threshold);
  
  DPPParams.thr[ch] = threshold;
  
  if( ret == 0 ) {
    
    TString command;
    command.Form("sed -i '2s/.*/%d     \\/\\/Tigger Threshold (in LSB)/' %s/setting_%d.txt", threshold, folder.c_str(), ch);
    system(command.Data());
    printf("Done. Threshold of ch-%d is %d now.\n", ch, threshold);
    
  }else{
    printf("fail. something wrong.\n");
  }
  return ret;
}

int Digitizer::SetChannelDynamicRange(int ch, string folder, int dyRange){
  
  if( dyRange != 0 && dyRange != 1 ) {
    printf(" Dynamic Range can be either 0.5 Vpp (use 1) or 2.0 Vpp (use 0). \n");
    return 0;
  }
  
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x1028 +  (ch<<8), dyRange);
  inputDynamicRange[ch] = dyRange;
  
  if( ret == 0 ) {
    TString command;
    command.Form("sed -i '15s/.*/%d     \\/\\/input dynamic range, 0 = 2 Vpp, 1 = 0.5 Vpp/' %s/setting_%d.txt", dyRange, folder.c_str(), ch);
    system(command.Data());
    printf("Done. Dynamic Range of ch-%d is %3.1f Vpp now.\n", ch, dyRange == 0 ? 2.0 : 0.5);
  }else{
    printf("fail. something wrong.\n");
  }
  return ret;
  
}

void Digitizer::SetChannelMask(bool ch7, bool ch6, bool ch5, bool ch4, bool ch3, bool ch2, bool ch1, bool ch0){
  ChannelMask = 0;
  nChannelOpen = 0;
  
  if( ch0 ) {ChannelMask +=   1; nChannelOpen += 1;}
  if( ch1 ) {ChannelMask +=   2; nChannelOpen += 1;}
  if( ch2 ) {ChannelMask +=   4; nChannelOpen += 1;}
  if( ch3 ) {ChannelMask +=   8; nChannelOpen += 1;}
  if( ch4 ) {ChannelMask +=  16; nChannelOpen += 1;}
  if( ch5 ) {ChannelMask +=  32; nChannelOpen += 1;}
  if( ch6 ) {ChannelMask +=  64; nChannelOpen += 1;}
  if( ch7 ) {ChannelMask += 128; nChannelOpen += 1;}
  
  if( isConnected ){
    ret = CAEN_DGTZ_SetChannelEnableMask(handle, ChannelMask);
    if( ret == 0 ){
      printf("---- ChannelMask changed to %d \n", ChannelMask);
    }else{
      printf("---- Fail to change ChannelMask \n");
    }
  }
  
}
  

int Digitizer::SetChannelParity(int ch, bool isPositive){
  
  if ( isPositive ){
    PulsePolarity[ch] = CAEN_DGTZ_PulsePolarityPositive; 
    printf(" Set channel %d to be + parity pulse. \n", ch);
  }else{
    PulsePolarity[ch] = CAEN_DGTZ_PulsePolarityNegative; 
    printf(" Set channel %d to be - parity pulse. \n", ch);
  }
  
  ret |= CAEN_DGTZ_SetChannelPulsePolarity(handle, ch, PulsePolarity[ch]);
  
  return ret;
}

void Digitizer::ClearRawData(){
  std::fill_n(rawEnergy, 5000, 0);
  std::fill_n(rawChannel, 5000, -1);
  std::fill_n(rawTimeStamp, 5000, 0);
  rawEvCount = 0;
}
  
void Digitizer::ClearData(){
  for( int i = 0 ; i < MaxDataAShot ; i++){
    for( int j = 0; j < MaxNChannels; j++){
      Energy[i][j] = 0;
      Channel[i][j] = -1;
      TimeStamp[i][j] = 0;
    }
  }
  
  for( int k = 0; k < MaxNChannels ; k++)  countNChannelEvent[k] = 0;

  countEventBuilt = 0;
}

void Digitizer::SetChannelMask(uint32_t mask){ 
  ChannelMask = mask;
  
  CalNOpenChannel(mask); 

  if( isConnected ){
    ret = CAEN_DGTZ_SetChannelEnableMask(handle, ChannelMask);
    if( ret == 0 ){
      printf("---- ChannelMask changed to %d \n", ChannelMask);
    }else{
      printf("---- Fail to change ChannelMask \n");
    }
  }
}


void Digitizer::SetDCOffset(int ch, float offset){
  DCOffset[ch] = offset;
  
  ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, uint( 0xffff * DCOffset[ch] ));
  
  if( ret == 0 ){
    printf("---- DC Offset of CH : %d is set to %f \n", ch, DCOffset[ch]);
  }else{
    printf("---- Fail to Set DC Offset of CH : %d \n", ch);
  }
}

uint32_t Digitizer::GetChannelThreshold(int ch) {  
  uint32_t * value = new uint32_t[MaxNChannels];
  CAEN_DGTZ_ReadRegister(handle, 0x106C + (ch << 8), value); 
  return value[0]; 
}


int Digitizer::GetChannelDynamicRange(int ch) {  
  uint32_t * value = new uint32_t[MaxNChannels];
  CAEN_DGTZ_ReadRegister(handle, 0x1028 + (ch << 8), value); 
  return value[0]; 
}

void Digitizer::GetBoardConfiguration(){
  uint32_t * value = new uint32_t[1];
  CAEN_DGTZ_ReadRegister(handle, 0x8000 , value);
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
  printf("\e[33m================================================\n");
  printf("================ Getting setting for channel %d \n", ch);
  printf("================================================\e[0m\n");
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
  CAEN_DGTZ_ReadRegister(handle, 0x1080 + (ch << 8), value); printf("%20s  %d bit = Floor( rise x decay / 64 )\n", "Trap. Rescaling",  trapRescaling ); //Trap. Rescaling Factor
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
  
  printf("========================================= end of ch-%d\n", ch);
  
}

int Digitizer::ProgramDigitizer(){
  /* This function uses the CAENDigitizer API functions to perform the digitizer's initial configuration */
    int ret = 0;

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
   
   //AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;
   
    ret |= CAEN_DGTZ_SetDPPAcquisitionMode(handle, AcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);

    // Set Extras 2 to enable, this override Accusition mode, focring list mode
    if( AcqMode == 1 ) {
       uint32_t value = 0x10E0114;
      ret |= CAEN_DGTZ_WriteRegister(handle, 0x8000 , value );
    }else{
       
     //ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, ANALOG_TRACE_1, CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2);
     //ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, ANALOG_TRACE_2, CAEN_DGTZ_DPP_VIRTUALPROBE_Input);
     //ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, DIGITAL_TRACE_1, CAEN_DGTZ_DPP_DIGITALPROBE_Peaking);
    }
    
    // Set the number of samples for each waveform
    ret |= CAEN_DGTZ_SetRecordLength(handle, RecordLength);

    // Set the digitizer acquisition mode (CAEN_DGTZ_SW_CONTROLLED or CAEN_DGTZ_S_IN_CONTROLLED)
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED); // software command
    
    // Set the I/O level (CAEN_DGTZ_IOLevel_NIM or CAEN_DGTZ_IOLevel_TTL)
    ret |= CAEN_DGTZ_SetIOLevel(handle, IOlev);

    /* Set the digitizer's behaviour when an external trigger arrives:

    CAEN_DGTZ_TRGMODE_DISABLED: do nothing
    CAEN_DGTZ_TRGMODE_EXTOUT_ONLY: generate the Trigger Output signal
    CAEN_DGTZ_TRGMODE_ACQ_ONLY = generate acquisition trigger
    CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT = generate both Trigger Output and acquisition trigger

    see CAENDigitizer user manual, chapter "Trigger configuration" for details */
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

    // Set the enabled channels
    ret |= CAEN_DGTZ_SetChannelEnableMask(handle, ChannelMask);

    // Set how many events to accumulate in the board memory before being available for readout
    ret |= CAEN_DGTZ_SetDPPEventAggregation(handle, EventAggr, 0);
    
    /* Set the mode used to syncronize the acquisition between different boards.
    In this example the sync is disabled */
    ret |= CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);
    
    // Set the DPP specific parameters for the channels in the given channelMask
    ret |= CAEN_DGTZ_SetDPPParameters(handle, ChannelMask, &DPPParams);
    
    for(int i=0; i<MaxNChannels; i++) {
        if (ChannelMask & (1<<i)) {
            /// Set a DC offset to the input signal to adapt it to digitizer's dynamic range
            ///ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, 0x3333); // 20%
            ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, uint( 0xffff * DCOffset[i] ));
            
            /// Set the Pre-Trigger size (in samples)
            ret |= CAEN_DGTZ_SetDPPPreTriggerSize(handle, i, PreTriggerSize[i]);
            
            /// Set the polarity for the given channel (CAEN_DGTZ_PulsePolarityPositive or CAEN_DGTZ_PulsePolarityNegative)
            ret |= CAEN_DGTZ_SetChannelPulsePolarity(handle, i, PulsePolarity[i]);
            
            /// Set InputDynamic Range
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x1028 +  (i<<8), inputDynamicRange[i]);
            
            /// Set Energy Fine gain
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x104C +  (i<<8), energyFineGain[i]);
            
            /// read the register to check the input is correct
            ///uint32_t * value = new uint32_t[8];
            ///ret = CAEN_DGTZ_ReadRegister(handle, 0x1028 + (i << 8), value);
            ///printf(" InputDynamic Range (ch:%d): %d \n", i, value[0]);
        }
    }

    ///ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, ANALOG_TRACE_1, CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2);
    ///ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, ANALOG_TRACE_2, CAEN_DGTZ_DPP_VIRTUALPROBE_Input);
    ///ret |= CAEN_DGTZ_SetDPP_VirtualProbe(handle, DIGITAL_TRACE_1, CAEN_DGTZ_DPP_DIGITALPROBE_Peaking);

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
    
    chGain[ch] = 1.0;      // gain of the channel; if -1, default based on input-dynamic range;
    
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
        if( count == 22 ) chGain[ch]               = atof(line.substr(0, pos).c_str());             
        count++;
      }
    }
  }
  
};

void Digitizer::LoadGeneralSetting(string fileName){
  
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
    printf(" %-30s  %d ch\n", "Coincident Time Window", CoincidentTimeWindow);
    printf(" %-30s  %d ch\n", "Record Lenght", RecordLength);
    printf("====================================== \n");
    
  }

  return;
  
}

void Digitizer::ZeroSingleEvent(){
  if( NChannel != 0 ) {
    for( int i = 0; i < NChannel ; i++){
      singleEnergy[i] = 0;
      singleChannel[i] = -1;
      singleTimeStamp[i] = 0;
    }
  }
}


void Digitizer::StartACQ(){
  
  CAEN_DGTZ_SWStartAcquisition(handle);
  printf("Acquisition Started for Board %d\n", boardID);
  AcqRun = true;
  
}

void Digitizer::ReadData(bool debug){
  /* Read data from the board */
  ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
  if (BufferSize == 0) {
     for(int i = 0 ; i < MaxNChannels; i++ )waveformLength[i] = 0;
     return;
  }
  
  Nb = BufferSize;
  ret |= (CAEN_DGTZ_ErrorCode) CAEN_DGTZ_GetDPPEvents(handle, buffer, BufferSize, reinterpret_cast<void**>(&Events), NumEvents);
  //ret |= (CAEN_DGTZ_ErrorCode) CAEN_DGTZ_GetDPPEvents(handle, buffer, BufferSize, Events, NumEvents);
  
  /* ignore the error
  if (ret) {
    printf("Error when reading data %d\n", ret);
    //CAEN_DGTZ_SWStopAcquisition(handle);
    //CAEN_DGTZ_CloseDigitizer(handle);
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    CAEN_DGTZ_FreeDPPEvents(handle, reinterpret_cast<void**>(&Events));
    for(int i = 0 ; i < MaxNChannels; i++ ) waveformLength[i] = 0;
    return;
  }*/
  
  /* Analyze data */
  //if( debug ) printf("----------- read data\n");
  //rawEvCount = 0;
  
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
        
        //printf("%d, %6d, %13lu | %5u | %13llu | %13llu \n", ch, Events[ch][ev].Energy, Events[ch][ev].TimeTag, Events[ch][ev].Extras2 , rollOver >> 32, timetag);
        
        rawChannel[rawEvCount + rawEvLeftCount] = ch;
        rawEnergy[rawEvCount + rawEvLeftCount]  = Events[ch][ev].Energy;
        rawTimeStamp[rawEvCount + rawEvLeftCount] = timetag;
        
        if( debug) printf("read: %3d, %2d| %2d, %5d, %10llu | %10llu | ret: %d \n", rawEvCount, rawEvLeftCount, ch, Events[ch][ev].Energy, timetag, rollOver, ret);
        
        rawEvCount ++;
        
        if( rawEvCount > MaxDataAShot ) printf(" More than %d data read from Digitizer in a shot! \n", MaxDataAShot);
        
      } else { /* PileUp */
          PurCnt[ch]++;
      }
      
      if( AcqMode != CAEN_DGTZ_DPP_ACQ_MODE_List && ev == 0) {
      //if( AcqMode != CAEN_DGTZ_DPP_ACQ_MODE_List  ) {
         
         ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
         ret = CAEN_DGTZ_MallocDPPWaveforms(handle, reinterpret_cast<void**>(&Waveform), &AllocatedSize); 
    
         // only get the 0th event
         ret = CAEN_DGTZ_DecodeDPPWaveforms(handle, &Events[ch][ev], Waveform);

         // Use waveform data here...
         waveformLength[ch] = (int)(Waveform->Ns); // Number of samples
         WaveLine[ch] = Waveform->Trace1;                // First trace (ANALOG_TRACE_1)
         //DigitalWaveLine = Waveform->DTrace1;        // First Digital Trace (DIGITALPROBE1)
         
         //printf("============ ret : %d, ev : %d,  %d \n", ret, ev,  waveformLength);

         //for( int p = 0; p < waveformLength; p ++){
         //   if( p%10 == 0 ) printf(" %3d | %d \n", p, WaveLine[p]);
         //}
         
      }
        
    } // loop on events
    
  } // loop on channels
  
}

void Digitizer::PrintReadStatistic(){
  
  printf("####### Board ID = %d, handle = %d \n", boardID, handle);
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
  rawEvCount = 0;

}

void Digitizer::PrintEventBuildingStat(int updatePeriod){
  printf("===============================================\n");
  //printf("Number of retrieving = %d = %.2f per sec\n", rawEvCount, rawEvCount*1000./updatePeriod);
  printf(" %5s| %5s| %5s| \n", "#ch", "Built", "Total");
  //printf("-----------------------------------\n");
  //for( int k = 0; k < MaxNChannels-1 ; k ++){
  for( int k = 0; k < nChannelOpen-1 ; k ++){
    printf(" %5d| %5d| %5d|\n", k+1, countNChannelEvent[k], totNChannelEvent[k]);
  }
  //printf(" %5d| %5d| %5d| %5s\n", MaxNChannels, countNChannelEvent[MaxNChannels-1], totNChannelEvent[MaxNChannels-1], "left");
  printf(" %5d| %5d| %5d| %5s\n", nChannelOpen, countNChannelEvent[nChannelOpen-1], totNChannelEvent[nChannelOpen-1], "left");
  printf("-----------------------------------\n");
  printf(" %5s| %5d| %5d| %5d\n", "total", countEventBuilt, totEventBuilt, rawEvLeftCount);
  printf("===============================================\n");
  
}

void Digitizer::PrintDynamicRange(){
  printf("\n Ch | Dynamic Range \n");
  for( int i = 0 ; i < MaxNChannels; i ++ ){
    if ( ChannelMask & ( 1 << i) ) {
      printf(" %2d | %3.1f Vpp \n", i, GetChannelDynamicRange(i) == 0 ? 2.0: 0.5);
    }
  }
}

void Digitizer::PrintThreshold(){
  printf("\n Ch | Threshold \n");
  for( int i = 0 ; i < MaxNChannels; i ++ ){
    if ( ChannelMask & ( 1 << i) ) {
      printf(" %2d | %6d LSB \n", i, GetChannelThreshold(i));
    }
  }
}

void Digitizer::PrintThresholdAndDynamicRange(){
  printf("\n Ch | %9s  | %s \n", "Threshold", "Dynamic Range");
  for( int i = 0 ; i < MaxNChannels; i ++ ){
    if ( ChannelMask & ( 1 << i) ) {
      printf(" %2d | %6d LSB | %3.1f Vpp \n", i, GetChannelThreshold(i), GetChannelDynamicRange(i) == 0 ? 2.0: 0.5);
    }
  }
}

void Digitizer::StopACQ(){
  if( !AcqRun ) return;
  CAEN_DGTZ_SWStopAcquisition(handle); 
  printf("\n\e[1m\e[33m====== Acquisition STOPPED for Board %d\e[0m\n", boardID);
  AcqRun = false;
}

int Digitizer::BuildEvent(bool debug = false){
  
  //################################################################
  //  Sorrting raw event timeStamp
  //################################################################
  int nRawData = rawEvCount + rawEvLeftCount;
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
    if( debug) printf("Sorted: %3d| %2d, %5d, %10llu  \n", i, rawChannel[i], rawEnergy[i], rawTimeStamp[i]);
  }
  
  if( nRawData > 0 ) {
    rawTimeRange = rawTimeStamp[nRawData-1] - rawTimeStamp[0];
  }else{
    rawTimeRange = 99999999999.;
  }
  //################################################################
  // build event base on coincident window
  //################################################################

  if (debug) printf("=============Build event============\n");
  for( int k = 0; k < MaxNChannels ; k++) countNChannelEvent[k] = 0;
  int endID = 0;
  //ClearData();
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

    if( debug) printf("build: %3d | %d | %d, %llu, %d, %d \n", digitID, rawChannel[i], 0, rawTimeStamp[i], 0, rawEnergy[i]); 
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
        if( debug) printf("---- %d/ %d,  num in Group : %d | %d\n", i+1, nRawData,  numRawEventGrouped+1, CoincidentTimeWindow);
        // normal exit when next event outside coincident window
        break;
      }
      
      if(debug) printf("       %3d | %d | %d, %llu, %llu, %d\n", digitID, rawChannel[j], z, rawTimeStamp[j], timeDiff, rawEnergy[j]); 
      
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
    
    //fill in an event
    ZeroSingleEvent();
    for( int j = i ; j <= i + numRawEventGrouped ; j++){          
      singleChannel[rawChannel[j]] = rawChannel[j];
      singleEnergy[rawChannel[j]] = rawEnergy[j];
      singleTimeStamp[rawChannel[j]] = rawTimeStamp[j];
    }
    
    for(int pp = 0; pp < MaxNChannels; pp++) {
      Channel[countEventBuilt][pp] = singleChannel[pp];
      Energy[countEventBuilt][pp] = singleEnergy[pp];
      TimeStamp[countEventBuilt][pp] = singleTimeStamp[pp];
    }
    
    countEventBuilt ++;
    totEventBuilt++;
        
    i += numRawEventGrouped ; 
    
  }/**/// end of event building
  //################################################################
  
  //clear vectors but keep from endID
  ///rawChannel.erase(rawChannel.begin(), rawChannel.begin() + endID  );
  ///rawEnergy.erase(rawEnergy.begin(), rawEnergy.begin() + endID );
  ///rawTimeStamp.erase(rawTimeStamp.begin(), rawTimeStamp.begin() + endID );

  rawEvLeftCount = nRawData - endID;
  for( int i = 0 ; i < rawEvLeftCount; i++){
    rawChannel[i] = rawChannel[i + endID];
    rawEnergy[i] = rawEnergy[i + endID];
    rawTimeStamp[i] = rawTimeStamp[i + endID];
  }
  
  //for( int i = rawEvLeftCount ; i < MaxDataAShot ; i++){
  //  rawChannel[i] = -1;
  //  rawEnergy[i] = 0;
  //  rawTimeStamp[i] = 0;
  //}
  
  if( debug) {
    printf("======= show left over data (%d), endID = %d ====\n", rawEvLeftCount, endID );
    for( int i = 0; i < rawEvLeftCount+2 ; i ++){
      printf(" %d | %d, %d, %llu \n", i, rawChannel[i], rawEnergy[i], rawTimeStamp[i]);
    }
    
  }
 
  return 1; // for sucessful
 
}

int Digitizer::CalNOpenChannel(uint32_t mask){
  
  nChannelOpen = 0;
  int len = (int) ceil(log(mask)/log(2));
  for( int i = 0; i < len; i++){
    short bit = (mask >> i) & 1;
    if( bit == 1) nChannelOpen ++;
  }
  
  return nChannelOpen;
}
#endif
