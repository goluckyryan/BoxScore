#ifndef ISODETECT
#define ISODETECT

#include "../Class/GenericPlane.h"

class IsoDetect : public GenericPlane{
  RQ_OBJECT("IsoDetect");
public:

  IsoDetect();
  ~IsoDetect();
  
  void SetCanvasDivision();
  void SetOthersHistograms();
  
  void Fill(vector<UInt_t> energy);
  
  void Draw();

  void ClearHistograms();
  
  int GetChG1() {return chG1;}
  int GetChG2() {return chG2;}
  int GetChG3() {return chG3;}
  int GetChG4() {return chG4;}
  
  int GetG1Count()  {return countG[0];}
  int GetG2Count()  {return countG[1];}
  int GetG3Count()  {return countG[2];}
  int GetG4Count()  {return countG[3];}
  int GetdEECount() {return countG[4];}
  
  void SetCountZero();

private:
  
  //==== add 4 histograms for Glover detectors
  TH1F * hG1;
  TH1F * hG2;
  TH1F * hG3;
  TH1F * hG4;
  
  int chG1, chG2, chG3, chG4; 
  
  int countG[5]; // count for G1, G2, G3, G4, and dE-E; 
  
};

IsoDetect::IsoDetect(){
    
  //=========== ClassName and ClassID is for class identification in BoxScoreXY
  className = "IsoDetect";
  classID = 2;
  location = "IsomerDetection";  //this is for database tag
  
  //=========== Channel Mask and rangeDE and rangeE is for GenericPlane
  ChannelMask = 0xb6; /// Channel enable mask, 0x01, only frist channel, 0xff, all channel
  
  rangeDE[0] =     0; /// min range for dE
  rangeDE[1] = 60000; /// max range for dE
  rangeE[0] =      0; /// min range for E
  rangeE[1] =  60000; /// max range for E
  rangeTime =    500; /// range for Tdiff, nano-sec
  
  chdE = 0;  chdEGain = 0; 
  chE  = 1;   chEGain = 0;
  mode = 1; ///default channel Gain is equal
  
  //=========== custom histograms for HelioTarget
  hG1 = NULL;
  hG2 = NULL;
  hG3 = NULL;
  hG4 = NULL;
  
  chG1 = 2;
  chG2 = 3;
  chG3 = 4;
  chG4 = 5;
  
  NChannelForRealEvent = 1;
  
  GenericPlane::SetChannelMask(0,0,1,1,1,1,1,1);
  
  isHistogramSet = false;
  
}

IsoDetect::~IsoDetect(){
  
  delete hG1;
  delete hG2;
  delete hG3;
  delete hG4;
  
}

void IsoDetect::SetOthersHistograms(){
  
  int bin = 7990/2;
  float labelSize = 0.08;
  
  float xMin = 10;
  float xMax = 8000;
  
  hG1 = new TH1F("hG1", Form("G1 (ch=%d); [keV]; count / 2 keV", chG1), bin, xMin, xMax);
  hG2 = new TH1F("hG2", Form("G2 (ch=%d); [keV]; count / 2 keV", chG2), bin, xMin, xMax);
  hG3 = new TH1F("hG3", Form("G3 (ch=%d); [keV]; count / 2 keV", chG3), bin, xMin, xMax);
  hG4 = new TH1F("hG4", Form("G4 (ch=%d); [keV]; count / 2 keV", chG4), bin, xMin, xMax);
  
  hG1->GetXaxis()->SetLabelSize(labelSize);
  hG1->GetYaxis()->SetLabelSize(labelSize);
  
  hG2->GetXaxis()->SetLabelSize(labelSize);
  hG2->GetYaxis()->SetLabelSize(labelSize);
  
  hG3->GetXaxis()->SetLabelSize(labelSize);
  hG3->GetYaxis()->SetLabelSize(labelSize);
  
  hG4->GetXaxis()->SetLabelSize(labelSize);
  hG4->GetYaxis()->SetLabelSize(labelSize);
  
}

void IsoDetect::SetCanvasDivision(){
  
  fCanvas->Divide(1,2);
  fCanvas->cd(1)->Divide(2,1); 
  fCanvas->cd(1)->cd(1)->SetLogz();
  fCanvas->cd(1)->cd(2)->Divide(2,2);
  
  
  fCanvas->cd(2)->Divide(1,2); 
  fCanvas->cd(2)->cd(1)->Divide(4,1);

}

void IsoDetect::Draw(){
  
  if ( !isHistogramSet ) return;
  
  //fCanvas->cd(1)->cd(1); hdEtotE->Draw("colz");
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
  fCanvas->cd(1)->cd(2)->cd(3); hHit->Draw("HIST");
  
  
  fCanvas->cd(2)->cd(1)->cd(1); hG1->Draw("");
  fCanvas->cd(2)->cd(1)->cd(2); hG2->Draw("");
  fCanvas->cd(2)->cd(1)->cd(3); hG3->Draw("");
  fCanvas->cd(2)->cd(1)->cd(4); hG4->Draw("");
  
  fCanvas->cd(2)->cd(2) ;rateGraph->Draw("AP"); legend->Draw();
  
  fCanvas->Modified();
  fCanvas->Update();
  gSystem->ProcessEvents();
  
}


void IsoDetect::Fill(vector<UInt_t> energy){
  
  if ( !isHistogramSet ) return;
  
  GenericPlane::Fill(energy);
  
  double en;
  
  ///Dynamic Range = 0.5
  ///if( energy[chG1] > 100 ) en1 = (double)energy[chG1] * (0.146805) - 0.338782; hG1->Fill(en1);
  ///if( energy[chG2] > 100 ) en2 = (double)energy[chG2] * (0.176429) + 0.312143; hG2->Fill(en2);
  ///if( energy[chG3] > 100 ) hG3->Fill(energy[chG3]);
  ///if( energy[chG4] > 100 ) en4 = (double)energy[chG4] * (0.172727) + 0.518182; hG4->Fill(en4);
  
  /// Dynamic Rnage = 2.0
  if( energy[chG1] > 10 ) {en = (double)energy[chG1] * (0.593607) + 3.22374; hG1->Fill(en); countG[0] ++ ;}
  if( energy[chG2] > 10 ) {en = (double)energy[chG2] * (0.711816) + 1.25937; hG2->Fill(en); countG[1] ++ ;}
  if( energy[chG4] > 10 ) {en = (double)energy[chG4] * (0.700709) - 1.07801; hG4->Fill(en); countG[3] ++ ;}
  
  if( energy[chdE] > 100 && energy[chE] > 100 ) countG[4] ++;
  
}

void IsoDetect::ClearHistograms(){
  
  GenericPlane::ClearHistograms();
  
  hG1->Reset();
  hG2->Reset();
  hG3->Reset();
  hG4->Reset();
  
}

void IsoDetect::SetCountZero(){
  for( int i = 0 ; i < 5 ; i ++ ) countG[i] = 0;
}

#endif
