#ifndef HELIOSARRAY
#define HELIOSARRAY

/*************************************************

   This class is for testing of a single detector of the Helios Array.
   
   One single detector  has 4 read-outs: Energy, XF, XN, and Ring.
   
*************************************************/

#include "../Class/GenericPlane.h"

class HelioArray : public GenericPlane{
  RQ_OBJECT("HelioArray");
public:

  HelioArray();
  ~HelioArray();
  
  void SetCanvasDivision();
  void SetOthersHistograms();
  
  void Fill(vector<UInt_t> energy); /// slow
  void Fill(UInt_t * energy);  /// fast
  
  void Draw();

  void ClearHistograms();
  
  int GetChEnergy() {return chEnergy;}
  int GetChXF()     {return chXF;}
  int GetChXN()     {return chXN;}
  int GetChRing()   {return chRing;}
  
  int GetEnergyCount()  {return countG[0];}
  int GetXFCount()  {return countG[1];}
  int GetXNCount()  {return countG[2];}
  int GetRingCount()  {return countG[3];}
  
  void SetCountZero();

private:
  
  //==== add 4 histograms for Glover detectors
  TH1F * hEnergy;
  TH1F * hXF;
  TH1F * hXN;
  TH1F * hRing;
  
  int chEnergy, chXF, chXN, chRing; 
  
  int countG[5]; // count for Energy, XF, XN, and Ring; 
  
};

HelioArray::HelioArray(){
    
  //=========== ClassName and ClassID is for class identification in BoxScoreXY
  className = "HelioArray";
  classID = 3;
  location = "HeliosArray";  //this is for database tag
  
  //=========== Channel Mask and rangeDE and rangeE is for GenericPlane
  ///ChannelMask = 0xff; /// Channel enable mask, 0x01, only frist channel, 0xff, all channel
  GenericPlane::SetChannelMask(1,1,1,1,1,1,1,1);
    
  /// dE and E is kind of XF and XN, we have to sum it up
  rangeDE[0] =     0; /// min range for dE
  rangeDE[1] = 80000; /// max range for dE
  rangeE[0] =      0; /// min range for E
  rangeE[1] =  80000; /// max range for E
  rangeTime =    500; /// range for Tdiff, nano-sec
  
  chdE = 0;  chdEGain = 0; 
  chE  = 1;   chEGain = 0;
  mode = 1; ///default channel Gain is equal
  
  NChannelForRealEvent = 4;
  
  isHistogramSet = false;
  
  //============ Custom histogram
  chEnergy = 0;
  chXF = 1;
  chXN = 2;
  chRing = 3;
  
  hEnergy = NULL;
  hXF = NULL;
  hXN = NULL;
  hRing = NULL;
  
}

HelioArray::~HelioArray(){
  
  delete hEnergy;
  delete hXF;
  delete hXN;
  delete hRing;
  
}

void HelioArray::SetOthersHistograms(){
  
  int bin = 800;
  float labelSize = 0.08;
  
  float xMin = 0;
  float xMax = 8000;
  
  hEnergy = new TH1F("hEnegry", Form("Energy (ch=%d); [keV]; count / 2 keV", chEnergy), bin, xMin, xMax);
  hXF     = new TH1F("hXF",     Form("XF (ch=%d); [keV]; count / 2 keV",     chXF),     bin, xMin, xMax);
  hXN     = new TH1F("hXN",     Form("XN (ch=%d); [keV]; count / 2 keV",     chXN),     bin, xMin, xMax);
  hRing   = new TH1F("hRing",   Form("Ring (ch=%d); [keV]; count / 2 keV",   chRing),   bin, xMin, xMax);
  
  hEnergy->GetXaxis()->SetLabelSize(labelSize);
  hEnergy->GetYaxis()->SetLabelSize(labelSize);
  
  hXF->GetXaxis()->SetLabelSize(labelSize);
  hXF->GetYaxis()->SetLabelSize(labelSize);
  
  hXN->GetXaxis()->SetLabelSize(labelSize);
  hXN->GetYaxis()->SetLabelSize(labelSize);
  
  hRing->GetXaxis()->SetLabelSize(labelSize);
  hRing->GetYaxis()->SetLabelSize(labelSize);
  
}

void HelioArray::SetCanvasDivision(){
  
  fCanvas->SetTitle("HELIOS array testing");
  fCanvas->Divide(3,2);
  
}

void HelioArray::Draw(){
  
  if ( !isHistogramSet ) return;
  
  fCanvas->cd(5); hTDiff->Draw(); line->Draw();
  
  fCanvas->cd(1); hEnergy->Draw("");
  fCanvas->cd(2); hXF->Draw("");
  fCanvas->cd(3); hXN->Draw("");
  fCanvas->cd(4); hRing->Draw("");
  
  fCanvas->Modified();
  fCanvas->Update();
  gSystem->ProcessEvents();
  
}


void HelioArray::Fill(vector<UInt_t> energy){
  
  if ( !isHistogramSet ) return;
  
  GenericPlane::Fill(energy);
  
  hEnergy->Fill(energy[chEnergy]);
  hXN->Fill(energy[chXN]);
  hXF->Fill(energy[chXF]);
  hRing->Fill(energy[chRing]);
  
}

void HelioArray::Fill(UInt_t * energy){
  
  if ( !isHistogramSet ) return;
  
  GenericPlane::Fill(energy);

  hEnergy->Fill(energy[chEnergy]);
  hXN->Fill(energy[chXN]);
  hXF->Fill(energy[chXF]);
  hRing->Fill(energy[chRing]);
    
}

void HelioArray::ClearHistograms(){
  
  GenericPlane::ClearHistograms();
  
  hEnergy->Reset();
  hXF->Reset();
  hXN->Reset();
  hRing->Reset();
  
}

void HelioArray::SetCountZero(){
  for( int i = 0 ; i < 5 ; i ++ ) countG[i] = 0;
}

#endif
