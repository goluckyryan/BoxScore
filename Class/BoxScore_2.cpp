#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <unistd.h>
#include <sys/time.h> /* struct timeval, select() */
#include <termios.h> /* tcgetattr(), tcsetattr() */

#include <TQObject.h>
#include <RQ_OBJECT.h>
#include <TApplication.h>

#include "DigitizerClass.h"
#include "GenericPlane.h"

long get_time();
static struct termios g_old_kbd_mode;
static void cooked(void);
static void raw(void);
int getch(void);
int keyboardhit();

void PrintCommands(){
  printf("\ns ) Start acquisition\n");
  printf("a ) Stop acquisition\n");
  //printf("c ) Cuts Creator\n");
  printf("y ) Clear histograms\n");
  printf("p ) Read Channel setting\n");
  printf("q ) Quit\n");
}

bool QuitFlag = false;

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

int main(int argc, char *argv[]){
    
  if( argc != 3 && argc != 4 ) {
    printf("Please input boardID and Location! (optional root file name)\n");
    printf("usage:\n");
    printf("$./BoxScore_2 boardID Location (tree.root)\n");
    printf("                         | \n");
    printf("                         |-- exit \n");
    printf("                         |-- cross \n");
    printf("                         |-- target \n");
    printf("                         |-- ZD (zero-degree) \n");
    return -1;
  }
  
  const int boardID = atoi(argv[1]);
  string location = argv[2];
  
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

  uint32_t ChannelMask;
  int chE, chDE, chTAC;
  if( location == "exit" ) {
    ChannelMask = 0x89 ;
    chE  = 3;
    chDE = 0;
    chTAC = 7;
  }else if( location == "cross") {
    ChannelMask = 0x12; //1 and 4
    chE  = 4;
    chDE = 1;
    chTAC = 7;
  }else if( location == "target") {
    ChannelMask = 0x12; //1 and 4
    chE  = 4;
    chDE = 1;
    chTAC = 7;
  }else if (location == "ZD"){
    ChannelMask = 0x24 ; // was 0xA4 for ch 2,5,7
    chE  = 5;
    chDE = 2;
    chTAC = 7;
  }else{
    printf("============== location : %s  is UNKNOWN!! Abort!\n", location.c_str());
    return 404;
  }
    
  Digitizer * dig = new Digitizer(boardID, ChannelMask);
  dig->LoadGeneralSetting("generalSetting.txt");
  dig->SetCoincidentTimeWindow(90);
  //dig->GetChannelSetting(0);
  //dig->GetChannelSetting(4);
  
  TApplication * app = new TApplication ("app", &argc, argv);
  
  GenericPlane * gp = new GenericPlane();
  gp->SetdEEChannels( chDE, chE);
  gp->SetChannelGain(dig->GetChannelGain(), dig->GetInputDynamicRange(), dig->GetNChannel());
  gp->SetHistograms(2000, 4000, 2000, 4000, 1000);
  gp->LoadCuts("cuts.root");
  
  //thread paintCanvasThread(paintCanvas); // using loop keep root responding
  
  PrintCommands();
  
  TQObject::Connect(app, "KeyPressed(Int_t)", "TApplication", app, "Help()");
  
  app->Run();

  /*******************************************************
   *    Start catching Keyboard 
   * 
  *******************************************************/
  uint64_t PrevRateTime = get_time();
  uint64_t StartTime = PrevRateTime;
  bool AcqRun = false;
  while(!QuitFlag) {
    if(keyboardhit()){
      char c = getch();
      
      if( c == 'q' ){
        QuitFlag = true;
      }
      
      if( c == 's' ){
        dig->StartACQ();
        AcqRun = true;
      } 
      
      if( c == 'a' ){
        dig->StopACQ();
        AcqRun = false;
        dig->ClearRawData();
      }
      
      if( c == 'y'){
        gp->ClearHistograms();
      }
      
      if( c =='p'){
        dig->StopACQ();
        AcqRun = false;
        for ( int id = 0; id < dig->GetNChannel(); id++){
          if( dig->GetChannelMask() & (1 << id) ) dig->GetChannelSetting(id);
        }
      }
      
      if( c == 'c'){
        dig->StopACQ();
        AcqRun = false;
        //gp.CutCreator();
        //app.Run();
      }
      
    }
    
    if( !AcqRun) {
      sleep(0.01);
      continue;
    }

    int updatePeriod = 1000; // millisecound
    uint64_t CurrentTime = get_time();
    uint64_t ElapsedTime = CurrentTime - PrevRateTime; // milliseconds
      
    
    if (ElapsedTime > updatePeriod) {
      
      //printf("hsahdsad\n");
      dig->ReadData();
      uint64_t buildStartTime = get_time();
      int buildID = dig->BuildEvent();
      
      if( dig->GetNumRawEvent() > 0  && buildID == 1 ) {
        for( int i = 0; i < dig->GetEventBuiltCount(); i++){          
          //printf(" %3d | %d, %d , %llu \n", i, dig->GetChannel(i, 1), dig->GetEnergy(i, 1), dig->GetTimeStamp(i, 1) );
          //printf("       %d, %d , %llu \n", dig->GetChannel(i, 4), dig->GetEnergy(i, 4), dig->GetTimeStamp(i, 4) );
          gp->Fill(dig->GetEnergy(i, chDE), dig->GetEnergy(i, chE)); 
        }
      }
      
      uint64_t buildStopTime = get_time();
      uint64_t buildTime = buildStopTime - buildStartTime;
      
      float timeRange = dig->GetRawTimeRange() * dig->Getch2ns() * 1e-9;
      double totalRate = dig->GetEventBuiltCount()*1.0/timeRange;
      //printf(" %d , %f \n ",  dig->GetEventBuiltCount(), timeRange);
      gp->FillRateGraph((CurrentTime - StartTime)/1e3, totalRate);
      
      gp->Draw(); // this is time comsuming task.
      
      //================================== Display
      system("clear");
      PrintCommands();
      printf("\n======== Tree, Histograms, and Table update every ~%.2f sec\n", updatePeriod/1000.);      
      printf("Time Elapsed                 = %.3f sec = %.1f min\n", (CurrentTime - StartTime)/1e3, (CurrentTime - StartTime)/1e3/60.);
      printf("Built-event save to  : %s \n", rootFileName.Data());
      //printf("File size  : %.4f MB \n", fileSize );
      //printf("Database :  %s\n", databaseName.Data());
      
      printf("\n\n");
      dig->PrintReadStatistic();
      printf("\n");
      printf("===============================================\n");
      printf("Event-building time          = %lu msec\n", buildTime);
      dig->PrintEventBuildingStat(updatePeriod);
      
      dig->ClearData();
      
      PrevRateTime = CurrentTime;
      printf("\n");
    }
    
  }
  
  //paintCanvasThread.detach();
  
  dig->StopACQ();
  
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
