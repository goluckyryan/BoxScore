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

#include "../GUI/DigitizerClass.h"

using namespace std;

#define MaxNChannels 8

//TODO 1) change DCoffset, pulseParity to channel
//TODO 2) change the tree structure to be like HELIOS

//========== General setting;
unsigned long long int ch2ns = 2.;

uint ChannelMask = 0x03;   // Channel enable mask, 0x01, only frist channel, 0xff, all channel

int updatePeriod = 1000; //Table, tree, Plots update period in mili-sec.

int chE = 0;   //channel ID for E
int chDE = 1;  //channel ID for dE
int chTAC = 7; //channel ID for TAC

int rangeDE[2] = {0, 5000}; // range for dE
int rangeE[2] = {0, 5000};  // range for E
double rangeTime = 500;  // range for Tdiff, nano-sec

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
    printf("=========== Cannot find TCutG in %s, file might not exist. \n", fileName.Data());
  }
  
  //printf("====================================== \n");
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

  TMacro gs("generalSetting.txt");
  
  ChannelMask = 0xff;
  chE  = 4;
  chDE = 1;
  chTAC = 0;
  
  TApplication app ("app", &argc, argv);
  
  Digitizer dig(boardID, ChannelMask);
  
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
  TMacro chSetting[MaxNChannels];
  for( int i = 0 ; i < MaxNChannels; i++){
    if (ChannelMask & (1<<i)) {
      chSetting[i].Write(Form("setting_%i", i));
    }
  }
  
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
      if (ChannelMask & (1<<i)) {
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
  
  float * chGain;
  int * InputDynamicRange;
  
  chGain = dig.GetChannelGain();
  InputDynamicRange = dig.GetInputDynamicRange();
  
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
  
  TLine coincidentline;
  coincidentline.SetLineColor(2);
  coincidentline.SetX1(dig.GetCoincidentTimeWindow());
  coincidentline.SetY1(0);
  coincidentline.SetX2(dig.GetCoincidentTimeWindow());
  coincidentline.SetY2(100000000);
  
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

  uint32_t PrevRateTime = get_time();
  uint32_t StartTime, StopTime, CurrentTime, ElapsedTime;
  PrintCommands();

  int graphIndex = 0;
    
  while(!QuitFlag) {
    //##################################################################
    /* Check keyboard */
    if(keyboardhit()) {
      char c = getch();
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
        dig.StopACQ();
        for( int id = 0 ; id < MaxNChannels ; id++ ) {
          if (ChannelMask & (1<<id)) dig.GetChannelSetting(id);
        }
        printf("===================================\n");
        PrintCommands();
      }
      //========== start acquisition
      if (c == 's')  {
        gROOT->ProcessLine("gErrorIgnoreLevel = -1;");
        // NB: the acquisition for each board starts when the following line is executed
        // so in general the acquisition does NOT starts syncronously for different boards
        if( graphIndex == 0 ) {
          StartTime = get_time();
        }
        dig.StartACQ();
        
      }
      //========== stop acquisition
      if (c == 'a')  {
        dig.StopACQ();
        dig.ClearRawData();
        StopTime = get_time();  
        printf("\n====== Acquisition STOPPED for Board %d\n", boardID);
        printf("========== Duration : %u msec\n", StopTime - StartTime);
        
      }
      //========== pause and make cuts
      if( c == 'c' ){
        dig.StopACQ();
        dig.ClearRawData();
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
        
        PrintCommands();
        
      }
    }
    
    if (!dig.IsRunning()) {
      sleep(0.01); // 10 mili-sec
      continue;
    }
    
    dig.ReadData();
    
    //##################################################################
    /* Calculate throughput and trigger rate (every second) */
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PrevRateTime; /* milliseconds */

    if (ElapsedTime > updatePeriod) {
      //sort event from tree and append to exist root
      TFile * fileAppend = new TFile(rootFileName, "UPDATE");
      tree = (TTree*) fileAppend->Get("tree");
      double fileSize = fileAppend->GetSize() / 1024. / 1024. ;
      
      tree->SetBranchAddress("e", energy);
      tree->SetBranchAddress("t", timeStamp);
      tree->SetBranchAddress("ch", channel);
      
      if(isCutFileOpen){
        for( int i = 0 ; i < numCut; i++ ){
          countFromCut[i] = 0;
        }
      }
      
      int buildID = dig.BuildEvent();

      //################################################################
      // Fill histogram
      //################################################################
      
      
      // Fill TDiff
      for( int i = 0; i < dig.GetNumRawEvent() - 1; i++){
        ULong64_t timeDiff = dig.GetRawTimeStamp(i+1) - dig.GetRawTimeStamp(i);
        hTDiff->Fill(timeDiff * ch2ns);
      }

      if( dig.GetNumRawEvent() > 0  && buildID == 1 ) {
        for( int i = 0; i < dig.GetEventBuiltCount(); i++){          
          
          for(int ch = 0; ch < MaxNChannels; ch++){
            energy[ch] = dig.GetEnergy(i, ch);
            timeStamp[ch] = dig.GetTimeStamp(i, ch);
            channel[ch] = dig.GetChannel(i, ch);
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
        }
      }/**/// end of Filling histograms
      //################################################################
      //################################################################
      
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
      //system("clear");
      PrintCommands();
      printf("\n======== Tree, Histograms, and Table update every ~%.2f sec\n", updatePeriod/1000.);
      printf("Time Elapsed         = %.3f sec = %.1f min\n", (CurrentTime - StartTime)/1e3, (CurrentTime - StartTime)/1e3/60.);
      //printf("Event-building time          = %lu msec\n", buildTime);
      printf("Built-event save to  : %s \n", rootFileName.Data());
      printf("File size            : %.4f MB \n", fileSize );
      printf("Database             : %s\n", databaseName.Data());
      
      printf("\n");
      dig.PrintReadStatistic();
      
      PrevRateTime = CurrentTime;
      printf("\n");
      
      //filling rate graph and data base
      int lowerTime = (CurrentTime - StartTime)/1e3 - RateWindow;
      for( int j = 1 ; j <= graphRate->GetN(); j++){
        double x, y;
        graphRate->GetPoint(j-1, x, y);
        if( x < lowerTime ) graphRate->RemovePoint(j-1);
      }
      
      float timeRangeSec = dig.GetRawTimeRange() * 2e-9;
      double totalRate = dig.GetEventBuiltCount()*1.0/timeRangeSec;
      if( totalRate >= 0. ) graphRate->SetPoint(graphRate->GetN(), (CurrentTime - StartTime)/1e3, totalRate);
      
      printf("===============================================\n");
      dig.PrintEventBuildingStat(updatePeriod);
      printf("===============================================\n");
      printf(" Rate( all) :%7.2f pps | mean :%7.2f pps\n", totalRate, graphRate->GetMean(2));
      
      if( totalRate >= 0. ) fullGraphRate->SetPoint(graphIndex, (CurrentTime - StartTime)/1e3, totalRate);
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
          graphRateCut[i]->SetPoint(graphRateCut[i]->GetN(), (CurrentTime - StartTime)/1e3, countFromCut[i]*1.0/timeRangeSec);
          fullGraphRateCut[i]->SetPoint(graphIndex, (CurrentTime - StartTime)/1e3, countFromCut[i]*1.0/ElapsedTime*1e3);
          cutG = (TCutG *)cutList->At(i) ;
          cCanvas->cd(1)->cd(1); cutG->Draw("same");
          printf(" Rate(%4s) :%7.2f pps | mean :%7.2f pps\n", cutG->GetName(), countFromCut[i]*1.0/timeRangeSec, graphRateCut[i]->GetMean(2));
          
          //============= write to database 
          WriteToDataBase(databaseName, cutG->GetName(), tag,  countFromCut[i]*1.0/timeRangeSec);
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
      
    }

  } // End of readout loop
  
  if( isSaveRaw ) {
    rawTree->Write("rawtree", TObject::kOverwrite); 
    fileRaw->Close();
  }
  fCut->Close();
  
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
