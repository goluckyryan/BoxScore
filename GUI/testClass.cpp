#include "DigitizerClass.h"


int main(){
  
  Digitizer digitizer(1);
  
  int NChannel = digitizer.GetNChannel();
  uint32_t ChannelMask = digitizer.GetChannelMask();
  


  digitizer.GetBoardConfiguration();
  
  return 0;
}
