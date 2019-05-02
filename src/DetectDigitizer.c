#include <stdio.h>
#include <string>
#include <sstream>
#include "CAENDigitizer.h"

#include "keyb.h"
#include <cmath>

#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

#define MAXNB 8 /* Number of connected boards */

using namespace std;

int checkCommand() {
  int c = 0;
  if(!kbhit()) return 0;
  c = getch();
  switch (c) {
    case 's':  return 9; break;
    case 'k':  return 1; break;
    case 'r':  return 3; break;
    case 'p':  return 4; break;
    case 'z':  return 5; break;
    case 'q':  return 2; break;
  }
  return 0;
}
void GetChannelSetting(int handle, int ch){
  
  uint32_t * value = new uint32_t[8];
  
  printf("================ Getting setting for channel %d \n", ch);
  
  printf("--------------- input \n");
  CAEN_DGTZ_ReadRegister(handle, 0x1020 + (ch << 8), value); printf("%20s  %d \n", "Record Length",  value[0] * 8); //Record length
  CAEN_DGTZ_ReadRegister(handle, 0x1038 + (ch << 8), value); printf("%20s  %d \n", "Pre-tigger",  value[0] * 4); //Pre-trigger
  
  //DPP algorithm Control
  CAEN_DGTZ_ReadRegister(handle, 0x1080 + (ch << 8), value);
  int polarity = int(value[0] >> 16); //in bit[16]
  printf("%20s  %s \n", "polarity",  (polarity & 1) ==  0 ? "Positive" : "negative"); //Polarity
  int baseline = int(value[0] >> 20) ; // in bit[22:20]
  printf("%20s  %.0f sample \n", "Ns baseline",  pow(4, 1 + baseline & 7)); //Ns baseline
  int NsPeak = int(value[0] >> 12); // in bit[13:12]
  
  CAEN_DGTZ_ReadRegister(handle, 0x1098 + (ch << 8), value); printf("%20s  %.2f %% \n", "DC offset",  value[0] * 100./ int(0xffff) ); //DC offset
  CAEN_DGTZ_ReadRegister(handle, 0x1028 + (ch << 8), value); printf("%20s  %.1f Vpp \n", "input Dynamic",  value[0] == 0 ? 2 : 0.5); //InputDynamic
  
  printf("--------------- discriminator \n");
  CAEN_DGTZ_ReadRegister(handle, 0x106C + (ch << 8), value); printf("%20s  %d LSB\n", "Threshold",  value[0]); //Threshold
  CAEN_DGTZ_ReadRegister(handle, 0x1074 + (ch << 8), value); printf("%20s  %d ns \n", "trigger hold off",  value[0] * 8); //Trigger Hold off
  CAEN_DGTZ_ReadRegister(handle, 0x1054 + (ch << 8), value); printf("%20s  %d sample \n", "Fast Dis. smoothing",  value[0] *2 ); //Fast Discriminator smoothing
  CAEN_DGTZ_ReadRegister(handle, 0x1058 + (ch << 8), value); printf("%20s  %d ch \n", "Input rise time",  value[0] * 2); //Input rise time
  
  printf("--------------- Trapezoid \n");
  CAEN_DGTZ_ReadRegister(handle, 0x105C + (ch << 8), value); printf("%20s  %d ns \n", "Trap. rise time",  value[0] * 8 ); //Trap. rise time
  CAEN_DGTZ_ReadRegister(handle, 0x1060 + (ch << 8), value); printf("%20s  %d ns \n", "Trap. flat time",  value[0] * 8); //Trap. flat time
  CAEN_DGTZ_ReadRegister(handle, 0x1020 + (ch << 8), value); printf("%20s  %d ns \n", "Trap. pole zero",  value[0] * 8); //Trap. pole zero
  CAEN_DGTZ_ReadRegister(handle, 0x1068 + (ch << 8), value); printf("%20s  %d ns \n", "Decay time",  value[0] * 8); //Trap. pole zero
  CAEN_DGTZ_ReadRegister(handle, 0x1064 + (ch << 8), value); printf("%20s  %d ns \n", "peaking time",  value[0] * 8); //Peaking time
  printf("%20s  %.0f sample\n", "Ns peak",  pow(4, NsPeak & 3)); //Ns peak
  CAEN_DGTZ_ReadRegister(handle, 0x1078 + (ch << 8), value); printf("%20s  %d ns \n", "Peak hole off",  value[0] * 8 ); //Peak hold off
  
  printf("--------------- Other \n");
  CAEN_DGTZ_ReadRegister(handle, 0x104C + (ch << 8), value); printf("%20s  %d \n", "Energy fine gain",  value[0]); //Energy fine gain
    
}


int main(int argc, char* argv[])
{
    /* The following variable is the type returned from most of CAENDigitizer
    library functions and is used to check if there was an error in function
    execution. For example:
    ret = CAEN_DGTZ_some_function(some_args);
    if(ret) printf("Some error"); */
    CAEN_DGTZ_ErrorCode ret;

    /* The following variable will be used to get an handler for the digitizer. The
    handler will be used for most of CAENDigitizer functions to identify the board */
    int	handle[MAXNB];

    CAEN_DGTZ_BoardInfo_t BoardInfo;
    CAEN_DGTZ_EventInfo_t eventInfo;
    CAEN_DGTZ_UINT16_EVENT_t *Evt = NULL;
    char *buffer = NULL;
    int MajorNumber;
    int i,b;
    int c = 0, count[MAXNB];
    char * evtptr = NULL;
    uint32_t size,bsize;
    uint32_t numEvents;
    i = sizeof(CAEN_DGTZ_TriggerMode_t);

    for(b=0; b<MAXNB; b++){
        /* IMPORTANT: The following function identifies the different boards with a system which may change
        for different connection methods (USB, Conet, ecc). Refer to CAENDigitizer user manual for more info.
        brief:
            CAEN_DGTZ_OpenDigitizer(<LikType>, <LinkNum>, <ConetNode>, <VMEBaseAddress>, <*handler>);
        Some examples below */
        
        /* The following is for b boards connected via b USB direct links
        in this case you must set <LikType> = CAEN_DGTZ_USB and <ConetNode> = <VMEBaseAddress> = 0 */
        //ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, b, 0, 0, &handle[b]);

        /* The following is for b boards connected via 1 opticalLink in dasy chain
        in this case you must set <LikType> = CAEN_DGTZ_PCI_OpticalLink and <LinkNum> = <VMEBaseAddress> = 0 */
        //ret = CAEN_DGTZ_OpenDigitizer(Params[b].LinkType, 0, b, Params[b].VMEBaseAddress, &handle[b]);

        /* The following is for b boards connected to A2818 (or A3818) via opticalLink (or USB with A1718)
        in this case the boards are accessed throught VME bus, and you must specify the VME address of each board:
        <LikType> = CAEN_DGTZ_PCI_OpticalLink (CAEN_DGTZ_PCIE_OpticalLink for A3818 or CAEN_DGTZ_USB for A1718)
        <LinkNum> must be the bridge identifier
        <ConetNode> must be the port identifier in case of A2818 or A3818 (or 0 in case of A1718)
        <VMEBaseAddress>[0] = <0xXXXXXXXX> (address of first board) 
        <VMEBaseAddress>[1] = <0xYYYYYYYY> (address of second board) 
        ...
        <VMEBaseAddress>[b-1] = <0xZZZZZZZZ> (address of last board)
        See the manual for details */
        printf("------- checking boardID = %d / %d \n", b, MAXNB-1);
        ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, b,0,0,&handle[b]);
        //printf("----------- %d \n", ret);
        if(ret != CAEN_DGTZ_Success) {
            printf("Can't open digitizer, Probably not connected.\n");
            continue;
            if( b == MAXNB-1 ) goto QuitProgram;
        }
        /* Once we have the handler to the digitizer, we use it to call the other functions */
        ret = CAEN_DGTZ_GetInfo(handle[b], &BoardInfo);
        printf("\nConnected to CAEN Digitizer Model %s, recognized as board %d\n", BoardInfo.ModelName, b);
        printf("\tBoard Model Familty %d\n", BoardInfo.FamilyCode);
        printf("\tR0C (Read-out-Controller) FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
        printf("\tAMC (ADC & Memory Controller) FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);
        
        // Check firmware revision (DPP firmwares cannot be used with this demo */
        sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
        if (MajorNumber >= 128) {
          printf("\t==== This digitizer has a DPP firmware!\n");
          switch (MajorNumber){
            case 0x80: printf("\tDPP-PHA for x724 boards \n"); break;
            case 0x82: printf("\tDPP-CI for x720 boards  \n"); break;
            case 0x83: printf("\tDPP-PSD for x720 boards \n"); break;
            case 0x84: printf("\tDPP-PSD for x751 boards \n"); break;
            case 0x85: printf("\tDPP-ZLE for x751 boards \n"); break;
            case 0x86: printf("\tDPP-PSD for x743 boards \n"); break;
            case 0x87: printf("\tDPP-QDC for x740 boards \n"); break;
            case 0x88: printf("\tDPP-PSD for x730 boards \n"); break;
            case 0x8B: printf("\tDPP-PHA for x730 boards \n"); break;
            case 0x8C: printf("\tDPP-ZLE for x730 boards \n"); break;
            case 0x8D: printf("\tDPP-DAW for x730 boards \n"); break;
          }
          //goto QuitProgram;
        }

        //ret = CAEN_DGTZ_Reset(handle[b]);                                               /* Reset Digitizer */
        //ret = CAEN_DGTZ_GetInfo(handle[b], &BoardInfo);                                 /* Get Board Info */
        //ret = CAEN_DGTZ_SetRecordLength(handle[b],4096);                                /* Set the lenght of each waveform (in samples) */
        //ret = CAEN_DGTZ_SetChannelEnableMask(handle[b],1);                              /* Enable channel 0 */
        //ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle[b],0,32768);                  /* Set selfTrigger threshold */
        //ret = CAEN_DGTZ_SetChannelSelfTrigger(handle[b],CAEN_DGTZ_TRGMODE_ACQ_ONLY,1);  /* Set trigger on channel 0 to be ACQ_ONLY */
        //ret = CAEN_DGTZ_SetSWTriggerMode(handle[b],CAEN_DGTZ_TRGMODE_ACQ_ONLY);         /* Set the behaviour when a SW tirgger arrives */
        //ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle[b],3);                                /* Set the max number of events to transfer in a sigle readout */
        //ret = CAEN_DGTZ_SetAcquisitionMode(handle[b],CAEN_DGTZ_SW_CONTROLLED);          /* Set the acquisition mode */
        //if(ret != CAEN_DGTZ_Success) {
        //    printf("Errors during Digitizer Configuration.\n");
        //    goto QuitProgram;
        //}
        //printf("\n");
    }
    printf("\n\nPress 's' to start the acquisition\n"); // c = 9
    printf("Press 'k' to stop  the acquisition\n");   // c  = 1
    printf("Press 'r' to read  a register\n");         // c = 3
    printf("Press 'p' to read Channel Setting\n");         // c = 3
    printf("Press 'z' to reset digitizer\n");         // c = 3
    printf("Press 'q' to quit  the application\n\n");  // c = 2
    while (1) {
      c = checkCommand();
      if (c == 9) break;
      if (c == 3) break;
      if (c == 4) break;
      if (c == 5) break;
      if (c == 2) return 0;
      Sleep(100);
    }
    
    if( c == 3 ) {
      
      printf(" Getting Board 1 Register \n");
      
      //ret = CAEN_DGTZ_WriteRegister(handle[1], 0xEF24, 0); // software reset
      
      // write Input Dynamic Range 
      //ret = CAEN_DGTZ_WriteRegister(handle[1], 0x8028, 0); // all channel to be 0
      //ret = CAEN_DGTZ_WriteRegister(handle[1], 0x1028, 1); // ch0 to be 1
      // read 
      
      string regStr = "";
      printf("Register Address  0x");
      int temp = scanf("%s", regStr.c_str());
      printf("%s \n", regStr.c_str());
      unsigned int regAddress;   
      std::stringstream ss;
      ss << std::hex << regStr.c_str();
      ss >> regAddress;
      
      printf(" Address : 0x%04x \n", regAddress);
      
      for( int ch = 0 ; ch < 8 ; ch++){
        uint32_t * value = new uint32_t[8];
        //ret = CAEN_DGTZ_ReadRegister(handle[1], 0x10A0 + (ch << 8), value);
        //printf(" DPP Algorithm Control 2  (ch:%d): 0x%08x \n", ch, value[0]);
        //uint32_t regAddressInput = 0x106c + (ch << 8);
        uint32_t regAddressInput = regAddress + (ch << 8);
        ret = CAEN_DGTZ_ReadRegister(handle[0], regAddressInput, value);
        if( ret  != CAEN_DGTZ_Success) {
          printf(" Address 0x%04x (ch:%d): fail \n", regAddressInput, ch);
        }else{
          printf(" Address 0x%04x (ch:%d): 0x%08x  = %d \n", regAddressInput, ch, value[0], value[0]);
        }
      }
      
      for(b=0; b<MAXNB; b++) ret = CAEN_DGTZ_CloseDigitizer(handle[b]);
      return 0;
    }
    
    if( c == 4 ){
      printf(" Get from Board 0 \n");
      for( int ch = 0; ch < 8; ch++){
        GetChannelSetting(handle[0], ch);
      }
      
      for(b=0; b<MAXNB; b++)
        ret = CAEN_DGTZ_CloseDigitizer(handle[b]);
      return 0;
      
    }
    
    if( c == 5 ){
      printf(" Reset  Board \n");
      for(b=0; b<MAXNB; b++) ret = CAEN_DGTZ_Reset(handle[0]);
      
      for(b=0; b<MAXNB; b++) ret = CAEN_DGTZ_CloseDigitizer(handle[b]);
      return 0;
      
    }
    
    /* Malloc Readout Buffer.
    NOTE1: The mallocs must be done AFTER digitizer's configuration!
    NOTE2: In this example we use the same buffer, for every board. We
    Use the first board to allocate the buffer, so if the configuration
    is different for different boards (or you use different board models), may be
    that the size to allocate must be different for each one. */
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle[0],&buffer,&size);

    
    for(b=0; b<MAXNB; b++)
            /* Start Acquisition
            NB: the acquisition for each board starts when the following line is executed
            so in general the acquisition does NOT starts syncronously for different boards */
            ret = CAEN_DGTZ_SWStartAcquisition(handle[b]);

    // Start acquisition loop
    while(1) {
        for(b=0; b<MAXNB; b++) {
        ret = CAEN_DGTZ_SendSWtrigger(handle[b]); /* Send a SW Trigger */
  
        ret = CAEN_DGTZ_ReadData(handle[b],CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,buffer,&bsize); /* Read the buffer from the digitizer */
  
            /* The buffer red from the digitizer is used in the other functions to get the event data
            The following function returns the number of events in the buffer */
        ret = CAEN_DGTZ_GetNumEvents(handle[b],buffer,bsize,&numEvents);
  
        printf(".");
        count[b] +=numEvents;
        for (i=0;i<numEvents;i++) {
                /* Get the Infos and pointer to the event */
          ret = CAEN_DGTZ_GetEventInfo(handle[b],buffer,bsize,i,&eventInfo,&evtptr);
                /* Decode the event to get the data */
          ret = CAEN_DGTZ_DecodeEvent(handle[b],evtptr,reinterpret_cast<void**>(Evt));
          //*************************************
          // Event Elaboration
          //*************************************
          ret = CAEN_DGTZ_FreeEvent(handle[b],reinterpret_cast<void**>(Evt));
        }
        c = checkCommand();
        if (c == 1) goto Continue;
        if (c == 2) goto Continue;
        } // end of loop on boards
    } //    end of readout loop

Continue:
    for(b=0; b<MAXNB; b++)
        printf("\nBoard %d: Retrieved %d Events\n",b, count[b]);
    goto QuitProgram;

/* Quit program routine */
QuitProgram:
    // Free the buffers and close the digitizers
    ret = CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    for(b=0; b<MAXNB; b++)
        ret = CAEN_DGTZ_CloseDigitizer(handle[b]);
    printf("Press 'Enter' key to exit\n");
    c = getchar();
    return 0;
}

