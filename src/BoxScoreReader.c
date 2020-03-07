/******************************************************************************
*  This program is for reading the root file from BoxScore
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

#include "../Class/GenericPlane.h"
#include "../Class/HelioTarget.h"
#include "../Class/IsoDetect.h"
#include "../Class/HelioArray.h"

using namespace std;

#define MaxNChannels 8

int updatePeriod = 1000; //Table, tree, Plots update period in mili-sec.

/* ###########################################################################
*  Functions
*  ########################################################################### */

long get_time();

/* ########################################################################### */
/* MAIN                                                                        */
/* ########################################################################### */
int main(int argc, char *argv[]){
    
  if( argc != 3 && argc != 4 ) {
    printf("usage:\n");
    printf("$./BoxScoreReader [rootFile] [location] \n");
    printf("                                  | \n");
    printf("                                  +-- testing \n");
    printf("                                  +-- exit \n");
    printf("                                  +-- cross \n");
    printf("                                  +-- ZD (zero-degree) \n");
    printf("                                  +-- XY (Helios target XY) \n");
    printf("                                  +-- iso (isomer with Glover Ge detector) \n");
    printf("                                  +-- array (Helios array) \n");
    return -1;
  }
  
  TString rootFile = argv[1];
  string location = argv[2];
  
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
  }else if ( location == "array"){
    gp = new HelioArray();
  }
  
  
  
  printf("******************************************** \n");
  printf("****          BoxScore Reader           **** \n");
  printf("******************************************** \n");
  printf("   Location :\e[33m %s \e[0m\n", location.c_str() );
  printf("      Class :\e[33m %s \e[0m\n", gp->GetClassName().c_str() );
  printf("******************************************** \n");
  
  /* *************************************************************************************** */
  /* Canvas and Digitzer                                                                               */
  /* *************************************************************************************** */
  
  uint ChannelMask = gp->GetChannelMask();

  gp->SetCanvasDivision(rootFile);  
  gp->SetCanvasTitleDivision(rootFile);  
  gp->SetGenericHistograms(); ///must be after SetChannelGain  
  
  ///things for derivative of GenericPlane
  if( gp->GetClassID() != 0  ) gp->SetOthersHistograms(); 
  
  //====== load cut and Draw
  //gp->LoadCuts("cutsFile.root");
  //gp->Draw();
  
  /* *************************************************************************************** */
  /* Readout                                                                                 */
  /* *************************************************************************************** */
  
  TFile * file = new TFile(rootFile);
  TTree * tree = (TTree *) file->Get("tree");
  
  tree->SetBranchStatus("*",0);
  tree->SetBranchStatus("e",1);
  tree->SetBranchStatus("t",1);
  
  UInt_t    e[MaxNChannels]; TBranch * b_energy;  
  ULong64_t t[MaxNChannels]; TBranch * b_timeStamp;  

  tree->SetBranchAddress("e", e, &b_energy);
  tree->SetBranchAddress("t", t, &b_timeStamp);
  
  int totalEvent = tree->GetEntries();
  
  printf("Number of event : %d \n", totalEvent);
  
  ULong64_t timeZero = 0;
  ULong64_t oldTime = 0;
  ULong64_t timeEnd = 0;
  
  Double_t timeDiff;
  Int_t count = 0;
  
  ULong64_t initTimeStamp = 0;
  ULong64_t finalTimeStamp = 0;
  
  for(int ev = 0; ev < totalEvent; ev++){
    tree->GetEntry(ev);
    
    gp->Fill(e);
    
    //Get inital TimeStamp
    if( ev == 0 ){
      for( int j = 0; j < MaxNChannels; j++){
        if( t[j] > 0 ) initTimeStamp = t[j];
      }
    }
    
    //Get final TimeStamp
    if( ev == totalEvent-1 ){
      for( int j = 0; j < MaxNChannels; j++){
        if( t[j] > 0 ) finalTimeStamp = t[j];
      }
    }
    
    //Recalculate rate graph
    for( int j = 0; j < MaxNChannels; j++){
      if( t[j] == 0 ) continue;
      count ++;
      
      //printf(" %llu, %llu, %llu, %f, %d\n", timeZero, oldTime, t[j], timeDiff, count);
      
      if( timeZero == 0 ) timeZero = t[j];
      
      if( ev == totalEvent -1 ) timeEnd = t[j];
      
      if( oldTime == 0 ) {
        oldTime = t[j];
      }else{
        if( t[j] > oldTime ) timeDiff = (t[j] - oldTime) * 2e-9; // 1ch = 2 ns; ns to sec;
        if( t[j] < oldTime ) timeDiff = (oldTime - t[j]) * 2e-9; // 1ch = 2 ns; ns to sec;
        if ( timeDiff > 1.00 && t[j] > timeZero){
          
          //printf("%16llu, %16llu, %f, %f, %d \n", t[j], oldTime, timeDiff, timeSet, count);
          
          double timeSet = (t[j] - timeZero) * 2e-9;
          gp->FillRateGraph( timeSet, count/timeDiff);
          oldTime = t[j];
          count = 0;
          
        } 
      }   
    }
    

    //if( ev%10000 == 0 ) {
    //  for( int j = 0; j < MaxNChannels; j++){printf("%u, ", e[j]);};
    //  printf("----- %d \n", ev);
    //}
  }
  
  double timeSpan = (finalTimeStamp - initTimeStamp) * 2e-9;
  printf("Total time span : %f sec \n", timeSpan);
  printf("                : %f min \n", timeSpan/60.);
  printf("                : %f hour \n", timeSpan/60./60.);
  printf("============================== Ctrl+C to exit.\n");
  
  gp->Draw();
  
  app.Run();

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

