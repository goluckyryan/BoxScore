#ifndef GENERICPLANE
#define GENERICPLANE

#include <TQObject.h>
#include <RQ_OBJECT.h>
#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TString.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TPad.h"
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

#include <thread>

#define numChannel 8

using namespace std;

class GenericPlane{
  RQ_OBJECT("GenericPlane")
public:
  GenericPlane();
  ~GenericPlane();

  void         SetChannelMask(bool ch7, bool ch6, bool ch5, bool ch4, bool ch3, bool ch2, bool ch1, bool ch0);
  void         SetChannelMask(uint32_t mask)       {ChannelMask = mask;}
  void         SetLocation(string loc)             {location = loc;}        ///kind of redanance?
  virtual void SetdEEChannels( int chdE, int chE)  {this->chE = chE; this->chdE = chdE; }
  void         SetChannelGain(float chGain[], int dynamicRange[], int NChannel);  
  void         SetCoincidentTimeWindow(int nanoSec);  
  void         SetGenericHistograms();
  virtual void SetCanvasTitleDivision(TString titleExtra);
  void         SetERange(int x1, int x2)  { rangeE[0] = x1; this->rangeE[1] = x2; };
  void         SetdERange(int x1, int x2) { rangeDE[0] = x1; this->rangeDE[1] = x2; };
  void         SetNChannelForRealEvent(int n) { NChannelForRealEvent = n;}
  void         SetHistogramsRange();
  void         SetChannelsPlotRange(int ** range);
  
  void         Fill(UInt_t  dE, UInt_t E);
  virtual void Fill(UInt_t * energy);
  void         FillTimeDiff(float nanoSec){ if( hTDiff == NULL ) return; hTDiff->Fill(nanoSec); }
  void         FillRateGraph(float x, float y);
  void         FillHit(int * hit){ for( int i = 0; i < 8; i++){ hHit->Fill(i+1, hit[i]);} }
  
  void         SetWaveCanvas(int length);
  void         FillWaves(int* length, int16_t ** wave);
  virtual void FillWaveEnergies(double * energy);
  void         ClearWaveEnergies() { for(int i = 0; i < numChannel; i++) waveEnergy[i] = 0;}
  void         ClearWaves() { for(int i=0; i < numChannel; i++) waveForm[i]->Clear();}
  
  void         TrapezoidFilter(int ch, int length, int16_t * wave);
  void         SetRiseTime(int ch, int temp)    { this->riseTime[ch]    = temp;} /// in ch, 1 ch = 2 ns
  void         SetFlatTop(int ch, int temp)     { this->flatTop[ch]     = temp;} /// in ch, 1 ch = 2 ns
  void         SetFallTime(int ch, int temp)    { this->decayTime[ch]   = temp;} /// in ch, 1 ch = 2 ns
  void         SetBaseLineEnd(int ch, int temp) { this->baseLineEnd[ch] = temp;} /// in ch, 1 ch = 2 ns
  int          GetRiseTime(int ch)    { return riseTime[ch]; }
  int          GetFlatTop(int ch)     { return flatTop[ch]; }
  int          GetFallTime(int ch)    { return decayTime[ch]; }
  int          GetBaseLineEnd(int ch) { return baseLineEnd[ch]; }
  
  virtual void Draw();
  virtual void DrawWaves();
  virtual void ClearHistograms();
  void         ZeroCountOfCut();
  void         LoadCuts(TString cutFileName);
  void         CutCreator();

  string GetLocation()             {return location;}
  string GetClassName()            {return className;}
  int    GetClassID()              {return classID;}
  int    GetMode()                 {return mode;}
  uint   GetChannelMask()          {return ChannelMask;}
  int*   GetERange()               {return rangeE;}
  int*   GetdERange()              {return rangeDE;}
  int    GetEChannel()             {return chE;}
  int    GetdEChannel()            {return chdE;}
  int    GetNChannelForRealEvent() {return NChannelForRealEvent;}
  
  TH1F * GetTH1F(TString name) {return (TH1F*)gROOT->FindObjectAny(name);};
  TH1F * GethE()               {return hE;}
  TH1F * GethdE()              {return hdE;}
  TH1F * GethtotE()            {return htotE;}
  TH1F * GethTDiff()           {return hTDiff;}
  TH2F * GethdEE()             {return hdEE;}
  TH2F * GethdEtotE()          {return hdEtotE;}
  TMultiGraph * GetRateGraph() {return rateGraph;}
  
  TGraph ** GetWaveForm()      {return waveForm;}
  double * GetWaveEnergy()     {return waveEnergy;}

  TObjArray * GetCutList()  {return cutList;}
  int GetCountOfCut (int i) {if( countOfCut.size() <= i ) return -404; return countOfCut[i];}
  int GetNumCut()           {return numCut;}

  TString GetCutName(int i) {cutG = (TCutG*) cutList->At(i); return cutG->GetName();}

  bool IsCutFileOpen()      {return numCut > 0 ? true : false;}
  
  //=========== empty method for adding back from derivative Class
  virtual void SetOthersHistograms(){} /// this can be overwrite by derived class
  virtual int  GetChG1() {}
  virtual int  GetChG2() {}
  virtual int  GetChG3() {}
  virtual int  GetChG4() {}
  virtual int GetG1Count()  {}
  virtual int GetG2Count()  {}
  virtual int GetG3Count()  {}
  virtual int GetG4Count()  {}
  virtual int GetdEECount() {}
  virtual void SetCountZero() {}
  

protected:

  string className;
  int classID;
  string location;  ///this is for database tag

  uint ChannelMask;   /// Channel enable mask, 0x01, only frist channel, 0xff, all channel
  int nChannel;
  
  int NChannelForRealEvent;
  
  int rangeDE[2]; /// range for dE
  int rangeE[2];  /// range for E
  double rangeTime;  /// range for Tdiff, nano-sec

  TCanvas *fCanvas;

  TH1F * hE;
  TH1F * hdE;
  TH2F * hdEE;
  TH2F * hdEtotE;
  
  TH1F * hHit;
  TH2F * hDetIDHit;
  
  TH1F * htotE;
  TH1F * hTDiff;
  
  //TODO TH2F * hdETOF;
  
  TLine * line; /// line for coincident window
  
  TMultiGraph * rateGraph;
  TLegend * legend; 
  
  TGraph * waveForm[8];
  TGraph * waveFormDiff[8];
  double waveEnergy[8];
  TGraph * trapezoid[8];
  
  int riseTime[8]; /// in ch
  int flatTop[8]; /// in ch
  int decayTime[8]; /// in ch
  int baseLineEnd[8]; /// in ch
  
  TObjArray * cutList; 
  int numCut;
  TCutG * cutG;
  vector<int> countOfCut;
  
  int chdE, chE; ///channel ID for E, dE
  float chdEGain, chEGain;
  int mode;
  
  bool isHistogramSet;
  
  TGraph * graphRate;
  TGraph ** graphRateCut; 
  
  TGraph * rangeGraph;
  
private:

  int graphIndex;
  
};


GenericPlane::~GenericPlane(){
  
  printf("cleaning up GenericPlane \n");
  
  delete fCanvas;
  delete hE;
  delete hdE;
  delete hdEE;
  delete hdEtotE;
  delete htotE;
  delete hTDiff;
  delete hHit;
  delete hDetIDHit;
  
  //delete graphRate;
  //delete graphRateCut; //need to know how to delete pointer of pointer
  delete rateGraph;
  delete line;
  
  for(int i = 0; i < numChannel; i++){
    delete waveForm[i];
    delete waveFormDiff[i];
    delete trapezoid[i];
  }
  
  delete legend; 
  ///delete rangeGraph;
  
  delete cutG;
  delete cutList;

  printf("cleaned up GenericPlane.\n");

}

GenericPlane::GenericPlane(){
  
  //======= className, classID, location muse be unique and declared in every derivative Class.
  className = "GenericPlane";
  classID = 0;
  location = "Generic";
  
  //=========== Channel Mask and rangeDE and rangeE is for GenericPlane
  ChannelMask = 0xff; /// Channel enable mask, 0x01, only frist channel, 0xff, all channel
  nChannel = 8;
  
  rangeDE[0] =     0; /// min range for dE
  rangeDE[1] = 60000; /// max range for dE
  rangeE[0] =      0; /// min range for E
  rangeE[1] =  60000; /// max range for E
  rangeTime =  500; /// range for Tdiff, nano-sec
  
  NChannelForRealEvent = 8;  /// this is the number of channel for a real event;
  
  fCanvas = new TCanvas("fCanvas", "testing", 0, 0, 1200, 600);
  gStyle->SetOptStat("neiou");
  
  if( fCanvas->GetShowEditor() ) fCanvas->ToggleEditor();
  if( fCanvas->GetShowToolBar() ) fCanvas->ToggleToolBar();
  
  chdE = 1;  chdEGain = 1; 
  chE = 7;   chEGain = 1;
  mode = 1; ///default channel Gain is equal
  
  hdE     = NULL;
  hE      = NULL;
  hdEE    = NULL;
  htotE   = NULL;
  hdEtotE = NULL;
  hTDiff  = NULL;
  
  hHit = NULL;
  hDetIDHit = NULL;
  
  rateGraph    = NULL;
  graphRate    = NULL;
  graphRateCut = NULL; 
  legend       = NULL; 
  rangeGraph   = NULL;
  
  line = new TLine();
  line->SetLineColor(2);
  
  graphIndex = 0;
  
  for( int i = 0 ; i < 8 ; i++){
    waveForm[i] = new TGraph();
    waveForm[i]->GetXaxis()->SetTitle("time [ch, 1 ch = 2 ns]");

    waveFormDiff[i] = new TGraph();
    waveFormDiff[i]->SetLineColor(2);
   
    trapezoid[i] = new TGraph();
    trapezoid[i]->SetLineColor(4);
    
    riseTime[i] = 500; ///  500 ch = 1000 ns
    flatTop[i] = 1000; /// 1000 ch = 2000 ns
    decayTime[i] = 45000; /// 45k ch =90000 ns = 90 us
    baseLineEnd[i] = 200 ; /// 200 ch = 400 ns
  }
  
  cutG    = NULL;
  cutList = NULL;
  numCut  = 0;
  countOfCut.clear();

  isHistogramSet = false;
  

}

void GenericPlane::SetChannelMask(bool ch7, bool ch6, bool ch5, bool ch4, bool ch3, bool ch2, bool ch1, bool ch0){
  
  uint32_t mask = 0;
  nChannel = 0;
  
  if( ch0 ) {mask +=   1; nChannel ++;}
  if( ch1 ) {mask +=   2; nChannel ++;}
  if( ch2 ) {mask +=   4; nChannel ++;}
  if( ch3 ) {mask +=   8; nChannel ++;}
  if( ch4 ) {mask +=  16; nChannel ++;}
  if( ch5 ) {mask +=  32; nChannel ++;}
  if( ch6 ) {mask +=  64; nChannel ++;}
  if( ch7 ) {mask += 128; nChannel ++;}
  
  ChannelMask = mask;
  
}

void GenericPlane::SetCoincidentTimeWindow(int nanoSec){
  
  line->SetX1(nanoSec);
  line->SetY1(0);
  line->SetX2(nanoSec);
  line->SetY2(100000000);
  
}

void GenericPlane::ClearHistograms(){
  hE->Reset();
  hdE->Reset();
  
  hdEE ->Reset();
  hdEtotE->Reset();
  
  htotE->Reset();
  hTDiff->Reset();
  
  hHit->Reset();
  hDetIDHit->Reset();
  
}

void GenericPlane::SetChannelGain(float chGain[], int dynamicRange[], int NChannel){
  
  //printf(" dE : %d , E: %d \n", chdE, chE);
  
  if( chdE == -1 || chE == -1 ){
    chdEGain = 1.;
    chEGain = 1.;
    return;
  }
  
  if( chGain[chE] == 1.0 && chGain[chdE] == 1.0 ){
    mode = 0;
    if( dynamicRange[chE] == dynamicRange[chdE] ) {
      chdEGain = 1.0;
      chEGain = 1.0;
      mode = 0;
    }else if (dynamicRange[chE] > dynamicRange[chdE]) { // E = 0.5Vpp, dE = 2 Vpp
      chdEGain = 1.;
      chEGain = 0.25;
      mode = 2;
    }else if (dynamicRange[chE] < dynamicRange[chdE]) { // E = 2 Vpp, dE = 0.5 Vpp
      chdEGain = 0.25;
      chEGain = 1.;
      mode = 1;
    }
  }else{
    mode = 3;
    chdEGain = chGain[chdE];
    chEGain = chGain[chE];
  }
  
}

void GenericPlane::SetGenericHistograms(){
  
  if( isHistogramSet ) return;
  
  int bin = 200;
  float labelSize = 0.08;
  
  hE    = new TH1F(   "hE", Form("raw E (ch=%d, gain=%.2f) ; E [ch] ;count ", chE, chEGain),   bin,  rangeE[0],  rangeE[1]);
  hdE   = new TH1F(  "hdE", Form("raw dE (ch=%d, gain=%.2f) ; dE [ch]; count", chdE, chdEGain), bin, rangeDE[0], rangeDE[1]);
  htotE = new TH1F("htotE", "total E ; totR [ch]; count", bin, rangeE[0]+rangeDE[0], rangeE[1]+rangeDE[1]);
  
  hdEE  = new TH2F("hdEE", "dE - E ; E [ch]; dE [ch] ", bin, rangeE[0], rangeE[1], bin, rangeDE[0], rangeDE[1]);
  hdEtotE  = new TH2F( "hdEtotE", Form("dE vs. totE = %4.2fdE + %4.2fE; totalE [ch]; dE [ch ", chdEGain, chEGain ), bin, rangeDE[0] * chdEGain + rangeE[0]* chEGain, rangeDE[1] * chdEGain + rangeE[1]* chEGain, bin, rangeDE[0] * chdEGain, rangeDE[1] * chdEGain);  
  
  hTDiff = new TH1F("hTDiff", "timeDiff [nsec]; time [nsec] ; count", bin, 0, rangeTime);
  
  hE->GetXaxis()->SetLabelSize(labelSize);
  hE->GetXaxis()->SetNdivisions(405);
  hE->GetYaxis()->SetLabelSize(labelSize);
  
  hdE->GetXaxis()->SetLabelSize(labelSize);
  hdE->GetXaxis()->SetNdivisions(405);
  hdE->GetYaxis()->SetLabelSize(labelSize);
  
  hTDiff->GetXaxis()->SetLabelSize(labelSize);
  hTDiff->GetXaxis()->SetNdivisions(405);
  hTDiff->GetYaxis()->SetLabelSize(labelSize);
  
  hdEE->SetMinimum(1);
  hdEtotE->SetMinimum(1);
  
  hHit = new TH1F("hHit", "number of hit", 8, -0.5, 7.5);
  hDetIDHit = new TH2F("hDetIDHit", "ch vs hit; hit; ch", 8, -0.5, 7.5, 8, -0.5, 7.5);
  
  rateGraph = new TMultiGraph();
  rateGraph->SetTitle("Beam rate [pps]; Time [sec]; Rate [pps]");
  
  legend = new TLegend( 0.9, 0.2, 0.99, 0.8); 
  
  graphRate = new TGraph();
  graphRate->SetTitle("Total Rate [pps] 10 sec average");
  graphRate->SetMarkerColor(4);
  graphRate->SetMarkerStyle(20);
  graphRate->SetMarkerSize(1);
  
  rangeGraph = new TGraph();
  rangeGraph->SetMarkerColor(0);
  rangeGraph->SetMarkerStyle(1);
  rangeGraph->SetMarkerSize(0.000001);
  
  rangeGraph->SetPoint(0,  5, 0);
  rateGraph->Add(rangeGraph);
  rateGraph->Add(graphRate);
  
  isHistogramSet = true;
  
}

void GenericPlane::SetCanvasTitleDivision(TString titleExtra = ""){
  fCanvas->Clear();
  fCanvas->SetTitle(titleExtra);
  fCanvas->Divide(1,2);
  fCanvas->cd(1)->Divide(2,1); 
  fCanvas->cd(1)->cd(1)->SetLogz();
  fCanvas->cd(2)->SetGridy();
  fCanvas->cd(2)->SetTicky();
  fCanvas->cd(2)->SetTickx();
  fCanvas->cd(1)->cd(2)->Divide(2,2);
  fCanvas->cd(1)->cd(2)->cd(3)->SetGridy();
  fCanvas->cd(1)->cd(2)->cd(3)->SetTicky();
  fCanvas->cd(1)->cd(2)->cd(3)->SetTickx(); 
  fCanvas->cd(1)->cd(2)->cd(4)->SetLogy(); 
}

void GenericPlane::Fill(UInt_t dE, UInt_t E){
  
  if ( !isHistogramSet ) return;
  
  E = E + gRandom->Gaus(0, 50);
  dE = dE + gRandom->Gaus(0, 50);
  
  hE->Fill(E);
  hdE->Fill(dE);
  hdEE->Fill(E, dE);
  float totalE = dE * chdEGain + E * chEGain;
  hdEtotE->Fill(totalE, dE);
  
}

void GenericPlane::Fill(UInt_t * energy){
  
  if ( !isHistogramSet ) return;

  int E = energy[chE] ;// + gRandom->Gaus(0, 500);
  int dE = energy[chdE] ;//+ gRandom->Gaus(0, 500);
  
  hE->Fill(E);
  hdE->Fill(dE);
  hdEE->Fill(E, dE);
  float totalE = dE * chdEGain + E * chEGain;
  hdEtotE->Fill(totalE, dE * chdEGain);
  
  if( numCut > 0  ){
    for( int i = 0; i < numCut; i++){
      cutG = (TCutG *) cutList->At(i);
      if( cutG->IsInside(E, dE) ){
        countOfCut[i] += 1;
      }
    }
  }  

}

void GenericPlane::FillWaveEnergies(double * energy){
    
    hE->Fill(energy[chE]);
    hdE->Fill(energy[chdE]);
    hdEE->Fill(energy[chE], energy[chdE]);
}

void GenericPlane::FillRateGraph(float x, float y){
  graphRate->SetPoint(graphIndex, x, y);
  graphIndex++;
  rateGraph->GetYaxis()->SetRangeUser(0, y*1.2);
  rateGraph->GetXaxis()->SetRangeUser(0, x*1.2);
  if( numCut > 0 ) {
    for( int i = 0; i < numCut ; i++){
      graphRateCut[i]->SetPoint(graphIndex, x, countOfCut[i]);
    }
  }
}

void GenericPlane::Draw(){
  if ( !isHistogramSet ) return;
  
  ///fCanvas->cd(1)->cd(1); hdEtotE->Draw("colz");
  fCanvas->cd(1)->cd(1); hdEE->Draw("colz");

  if( numCut > 0 ){
    for( int i = 0; i < numCut; i++){
      cutG = (TCutG *) cutList->At(i);
      cutG->Draw("same");
    }
  }
  
  fCanvas->cd(1)->cd(2)->cd(1); hE->Draw();
  fCanvas->cd(1)->cd(2)->cd(2); hdE->Draw();
  fCanvas->cd(1)->cd(2)->cd(4); hTDiff->Draw(); line->Draw();
  fCanvas->cd(2);   
  rateGraph->Draw("AP"); legend->Draw();
  fCanvas->Modified();
  fCanvas->Update();
  gSystem->ProcessEvents();
}

void GenericPlane::ZeroCountOfCut(){
  if( numCut > 0 ) {
    for( int i = 0; i < numCut ; i++){
      countOfCut[i] = 0;
    }
  }
}

void GenericPlane::SetHistogramsRange(){
  
  if( !isHistogramSet ) return;
  
  ///data is stored in bin
  hE->SetBins(500, rangeE[0], rangeE[1]);
  hdE->SetBins(500, rangeDE[0], rangeDE[1]);
  
  hdEE->SetBins(500, chEGain * rangeE[0], chEGain * rangeE[1], 
                500, chdEGain * rangeDE[0], chdEGain * rangeDE[1]);

  hdEtotE->SetBins(500, chEGain * rangeE[0] + chdEGain * rangeDE[0], chEGain * rangeE[1] + chdEGain * rangeDE[1], 
                   500, chdEGain * rangeDE[0], chdEGain * rangeDE[1]);

  printf("===============  suggest clear histograms. \n");
  ///hE->SetAxisRange(rangeE[0], rangeE[1], "X");
  ///hdE->SetAxisRange(rangeDE[0], rangeDE[1], "X");
  ///
  ///hdEE->SetAxisRange(chEGain * rangeE[0], chEGain * rangeE[1], "X");
  ///hdEE->SetAxisRange(chdEGain * rangeDE[0], chdEGain * rangeDE[1], "Y");
  ///
  ///hdEtotE->SetAxisRange(chEGain * rangeE[0] + chdEGain * rangeDE[0], chEGain * rangeE[1] + chdEGain * rangeDE[1], "X");
  ///hdEtotE->SetAxisRange(chdEGain * rangeDE[0], chdEGain * rangeDE[1], "Y");
  
}

void GenericPlane::SetChannelsPlotRange(int ** range){

   SetdERange(range[chdE][0], range[chdE][1]);
   SetERange(range[chE][0], range[chE][1]);
   
   SetHistogramsRange();
   
}

void GenericPlane::CutCreator(){
  
  gStyle->SetOptStat("");  
  
  TCanvas * cCutCreator =  new TCanvas("cCutCreator", "TCutG Creator", 0, 0, 800, 800);
  if( !cCutCreator->GetShowToolBar() ) cCutCreator->ToggleToolBar();
  if( cCutCreator->GetShowEditor() ) cCutCreator->ToggleEditor(); 
  
  cCutCreator->cd();
  
  printf(" %f \n", hdEtotE->GetEntries());
  
  hdEtotE->Draw("colz");
  gSystem->ProcessEvents();
  
  //thread paintCanvasThread(paintCanvas); // using loop keep root responding
  
  // make cuts
  TFile * cutFile = new TFile("cuts.root", "recreate");
  cutList = new TObjArray();
  TCutG * cut;

  int count = 1;
  do{
    printf("== make a graphic cut on the plot (double click on plot, or press 'x' to stop) : " );
    gSystem->ProcessEvents();
    gPad->WaitPrimitive();

    cut = (TCutG*) gROOT->FindObject("CUTG");

    if( cut == NULL) {
      printf(" break \n");
      count --;
      break;
    }

    TString name; name.Form("cut%d", count);
    cut->SetName(name);
    cut->SetLineColor(count);
    cutList->Add(cut);
    
    printf(" cut-%d \n", count);
    count++;
  
  }while( cut != NULL );

  cutList->Write("cutList", TObject::kSingleKey);

  printf("====> saved %d cuts into rdtCuts.root\n", count);

}

void GenericPlane::LoadCuts(TString cutFileName){
  
  if( !isHistogramSet ) return;

  gROOT->ProcessLine("gErrorIgnoreLevel = kFatal;"); // supress error messsage
  
  TFile * fCut = new TFile(cutFileName);
  bool isCutFileOpen = fCut->IsOpen();
  numCut = 0;

  legend->Clear();
  legend->AddEntry(graphRate, "Total");

  if( isCutFileOpen ){
    cutList = (TObjArray *) fCut->FindObjectAny("cutList");
    if( cutList == NULL ) return;
    numCut = cutList->GetEntries();
    printf("=========== found %d TCutG in %s \n", numCut, cutFileName.Data());
    delete cutG;
    cutG = new TCutG();
    graphRateCut = new TGraph * [numCut];
    for(int i = 0; i < numCut ; i++){
      printf(" cut name : %s \n", cutList->At(i)->GetName());
      countOfCut.push_back(0);
        
      graphRateCut[i] = new TGraph();
      graphRateCut[i]->SetMarkerColor(i+1);
      graphRateCut[i]->SetMarkerStyle(20+i);
      graphRateCut[i]->SetMarkerSize(1);
      rateGraph->Add(graphRateCut[i]);
      legend->AddEntry(graphRateCut[i], cutList->At(i)->GetName());
  
    }
  }else{
    
    printf("=========== Cannot find TCutG in %s, file might not exist. \n", cutFileName.Data());
  }
  
}


void GenericPlane::SetWaveCanvas(int length){
   
   fCanvas->Clear();
   
    int divX  = (nChannel+1)/2 ; 
    int divY = 2;
    if( nChannel == 1 ) divY = 1;
    
    int xVal[length], yVal[length];
    for( int i = 0; i < length; i++) xVal[i] = i*2;
    std::fill_n(yVal, length, 0);
    
    fCanvas->Divide(divX,divY);
    for( int i = 1; i <= divX * divY ; i++){ 
       fCanvas->cd(i)->SetGridy();
       fCanvas->cd(i)->SetGridx();
       for(int j = 0; j < length ; j++) {
          waveForm[i-1]->SetPoint(j, j, 0);
       }
       waveForm[i-1]->GetYaxis()->SetRangeUser(-1000, 17000);
       waveForm[i-1]->GetXaxis()->SetRangeUser(0, length);
       waveForm[i-1]->Draw("AP");
    }
  
   fCanvas->Modified();
   fCanvas->Update();
   gSystem->ProcessEvents();
   
}

void GenericPlane::FillWaves(int* length, int16_t ** wave){
  
  int integrateWindow = 400; // ch, 1ch = 2ns
  int pre_rise_start_ch = 100;
  int post_rise_start_ch = 800;
  
  for( int ch = 0 ; ch < 8; ch ++){
    if (!(ChannelMask & (1<<ch))) {
      waveEnergy[ch] = 0;
      continue;
    }
    
    waveForm[ch]->Clear(); 
    ///waveFormDiff[ch]->Clear(); // this is supposed to indicate the trigger
              
    if( length[ch] > 0 ) { 
      
      TrapezoidFilter(ch, length[ch], wave[ch]);
      
      //printf("ch : %d, wave[500] : %d \n", ch, wave[ch][100]); 
    
    
      for(int i = 0; i < length[ch]; i++){
        waveForm[ch]->SetPoint(i, i, wave[ch][i]); // 2 for 1ch = 2 ns
            
        ///if( pre_rise_start_ch <= i && i < pre_rise_start_ch + integrateWindow ){
        ///  waveFormDiff[ch]->SetPoint(i, i, 7200-1000);
        ///}else if( post_rise_start_ch <= i && i < post_rise_start_ch + integrateWindow ){
        ///  waveFormDiff[ch]->SetPoint(i, i, 7200+1000);
        ///}else{
        ///  waveFormDiff[ch]->SetPoint(i, i, 7200);
        ///} 
      }
         
      //TODO CR-RC filter https://doi.org/10.1016/j.nima.2018.05.020         

      waveForm[ch]->SetTitle(Form("channel = %d", ch));
      waveForm[ch]->GetYaxis()->SetRangeUser(-1000, 17000);
      waveForm[ch]->GetXaxis()->SetRangeUser(0, length[ch]);
      
      ///use Trapezoid energy at halfway of flattop
      ///waveEnergy[ch] = trapezoid[ch]->Eval(900+ riseTime[ch]+flatTop[ch]/2); /// it seems that the trigger is always at 900 ch.
      
      int yMax = waveForm[ch]->GetYaxis()->GetXmax();
      //int yMin = waveForm[ch]->GetYaxis()->GetXmin();
      int yMin = waveForm[ch]->Eval(3500);
      waveEnergy[ch] = (yMax - yMin)/2.;
      
    }
    
    /**
    if( length[ch] >= post_rise_start_ch + integrateWindow) {
      int pre_rise_energy = 0;
      int post_rise_energy = 0;
      
      for( int i = 0; i < length[ch]; i++ ){
        if( pre_rise_start_ch <= i && i < pre_rise_start_ch + integrateWindow ){
          pre_rise_energy += wave[ch][i];
        } 
        if( post_rise_start_ch <= i && i < post_rise_start_ch + integrateWindow ){
          post_rise_energy += wave[ch][i];
        } 
      }
      waveEnergy[ch] = (post_rise_energy - pre_rise_energy)*1./integrateWindow;
    }else{
      waveEnergy[ch] = 0;
    }
    */ 
  }
}


void GenericPlane::DrawWaves(){
  int padID = 0;
  for( int ch = 0 ; ch < 8; ch ++){
    if (!(ChannelMask & (1<<ch))) continue;
    padID ++;
    
    fCanvas->cd(padID);
    if( waveForm[ch]->GetN() > 0 ) waveForm[ch]->Draw("AP");
    if( waveFormDiff[ch]->GetN() > 0 ) waveFormDiff[ch]->Draw("same");
    if( trapezoid[ch]->GetN() > 0 ) trapezoid[ch]->Draw("same");
    
  }
  
  fCanvas->Update();
  gSystem->ProcessEvents();
}

void GenericPlane::TrapezoidFilter(int ch, int length, int16_t * wave){
   ///Trapezoid filter https://doi.org/10.1016/0168-9002(94)91652-7

   trapezoid[ch]->Clear();
   waveFormDiff[ch]->Clear();
   
   ///find baseline;
   double baseline;
   for( int i = 0; i < baseLineEnd[ch]; i++){
      baseline += wave[i];
   }
   baseline = baseline*1./baseLineEnd[ch];
   
   double pn = 0.;
   double sn = 0.;
   for( int i = 0; i < length ; i++){
   
      double dlk = wave[i] - baseline;
      if( i - riseTime[ch] >= 0 )dlk -= wave[i-riseTime[ch]] - baseline;
      if( i - flatTop[ch] - riseTime[ch] >= 0) dlk -= wave[i - flatTop[ch] - riseTime[ch]] - baseline;
      if( i - flatTop[ch] - 2*riseTime[ch] >= 0) dlk += wave[i - flatTop[ch] - 2*riseTime[ch]] - baseline;
      
      if( i == 0 ){
         pn = dlk;
         sn = pn + dlk*decayTime[ch];
      }else{
         pn = pn + dlk;
         sn = sn + pn + dlk*decayTime[ch];
      }    
      
      trapezoid[ch]->SetPoint(i, i, sn / decayTime[ch] / riseTime[ch]);
      
      if( i < 900 + riseTime[ch] + flatTop[ch]/2){
        waveFormDiff[ch]->SetPoint(i, i, 0 );
      }else{
        waveFormDiff[ch]->SetPoint(i, i, 1000);
      }
   }
   
}

#endif
