#ifndef ISODETECT
#define ISODETECT

#include "../Class/GenericPlane.h"

class IsoDetect : public GenericPlane{
  RQ_OBJECT("IsoDetect");
public:

  IsoDetect();
  ~IsoDetect();
  
  void SetCanvasDivision(TString fileName);
  void SetOthersHistograms();
  
  void Fill(UInt_t * energy);
  
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
  
  TH2F * hG12;
  TH2F * hG24;
  TH2F * hG41;
  
  TH1F * hG; // treat as sigle detector with add-back
  
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
  
  hG12 = NULL;
  hG24 = NULL;
  hG41 = NULL;
  hG = NULL; 
  
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
  
  delete hG;
  
  delete hG12;
  delete hG24;
  delete hG41;
  
}

void IsoDetect::SetOthersHistograms(){

  float labelSize = 0.08;

  
  float xMin = 5000;
  float xMax = 7500;
  int bin = (int)xMax - (int)xMin;
  
  hG1 = new TH1F("hG1", Form("G1 (ch=%d); [keV]; count / 1 keV", chG1), bin, xMin, xMax); hG1->SetLineColor(2);
  hG2 = new TH1F("hG2", Form("G2 (ch=%d); [keV]; count / 1 keV", chG2), bin, xMin, xMax); hG2->SetLineColor(3);
  hG3 = new TH1F("hG3", Form("G3 (ch=%d); [keV]; count / 1 keV", chG3), bin, xMin, xMax); hG3->SetLineColor(4);
  hG4 = new TH1F("hG4", Form("G4 (ch=%d); [keV]; count / 1 keV", chG4), bin, xMin, xMax); hG4->SetLineColor(5);
  
  hG  = new TH1F("hG", "G-all; [keV]; count / 1 keV", bin, xMin, xMax);
  
  hG12 = new TH2F("hG12", Form("G12 (ch=%d, ch=%d); [keV]; [keV]", chG1, chG2), bin, xMin, xMax, bin, xMin, xMax);
  hG24 = new TH2F("hG24", Form("G24 (ch=%d, ch=%d); [keV]; [keV]", chG2, chG4), bin, xMin, xMax, bin, xMin, xMax);
  hG41 = new TH2F("hG41", Form("G41 (ch=%d, ch=%d); [keV]; [keV]", chG4, chG1), bin, xMin, xMax, bin, xMin, xMax);
  
  ///hG1->GetXaxis()->SetLabelSize(labelSize);
  ///hG1->GetYaxis()->SetLabelSize(labelSize);
  ///
  ///hG2->GetXaxis()->SetLabelSize(labelSize);
  ///hG2->GetYaxis()->SetLabelSize(labelSize);
  ///
  ///hG3->GetXaxis()->SetLabelSize(labelSize);
  ///hG3->GetYaxis()->SetLabelSize(labelSize);
  ///
  ///hG4->GetXaxis()->SetLabelSize(labelSize);
  ///hG4->GetYaxis()->SetLabelSize(labelSize);
  
}

void IsoDetect::SetCanvasDivision(TString fileName){
  
  fCanvas->SetTitle(fileName);
  fCanvas->SetWindowSize(1500, 1500);
  
  
  fCanvas->Divide(1,3);
  //fCanvas->cd(1)->Divide(3,1);
  //fCanvas->cd(3)->Divide(3,1);
  ///fCanvas->cd(1)->Divide(2,1); 
  ///fCanvas->cd(1)->cd(1)->SetLogz();
  ///fCanvas->cd(1)->cd(2)->Divide(2,2);
  ///
  ///
  ///fCanvas->cd(2)->Divide(1,2); 
  ///fCanvas->cd(2)->cd(1)->Divide(4,1);
  
  //fCanvas->cd(1)->cd(1)->SetLogy();
  //fCanvas->cd(1)->cd(2)->SetLogy();
  //fCanvas->cd(1)->cd(3)->SetLogy();

  fCanvas->cd(1)->SetLogy();
  fCanvas->cd(2)->SetLogy();
  
}

void IsoDetect::Draw(){
  
  if ( !isHistogramSet ) return;
  
  //fCanvas->cd(1)->cd(1); hdEtotE->Draw("colz");
  //fCanvas->cd(1)->cd(1); hdEE->Draw("colz");

  ///if( numCut > 0 ){
  ///  for( int i = 0; i < numCut; i++){
  ///    cutG = (TCutG *) cutList->At(i);
  ///    cutG->Draw("same");
  ///  }
  ///}

  ///fCanvas->cd(1)->cd(2)->cd(1); hE->Draw();
  ///fCanvas->cd(1)->cd(2)->cd(2); hdE->Draw();
  ///fCanvas->cd(1)->cd(2)->cd(4); hTDiff->Draw(); line->Draw();
  ///fCanvas->cd(1)->cd(2)->cd(3); hHit->Draw("HIST");
  
  /*
  fCanvas->cd(1)->cd(1); hG1->Draw("");
  fCanvas->cd(1)->cd(2); hG2->Draw("");
  fCanvas->cd(1)->cd(3); hG4->Draw("");
  
  fCanvas->cd(3)->cd(1); hG12->Draw("colz");
  fCanvas->cd(3)->cd(2); hG24->Draw("colz");
  fCanvas->cd(3)->cd(3); hG41->Draw("colz");
  */
  
  fCanvas->cd(1); hG2->Draw(""); hG1->Draw("same"); hG4->Draw("same");
  fCanvas->cd(2); hG->Draw("");
  fCanvas->cd(3); graphRate->Draw("AP");
  
  //fCanvas->cd(2)->cd(2) ;rateGraph->Draw("AP"); legend->Draw();
  
  fCanvas->Modified();
  fCanvas->Update();
  gSystem->ProcessEvents();
  
}

void IsoDetect::Fill(UInt_t * energy){
  
  if ( !isHistogramSet ) return;
  
  GenericPlane::Fill(energy);
  
  double en;
  
  ///Dynamic Range = 0.5
  ///if( energy[chG1] > 100 ) en1 = (double)energy[chG1] * (0.146805) - 0.338782; hG1->Fill(en1);
  ///if( energy[chG2] > 100 ) en2 = (double)energy[chG2] * (0.176429) + 0.312143; hG2->Fill(en2);
  ///if( energy[chG3] > 100 ) hG3->Fill(energy[chG3]);
  ///if( energy[chG4] > 100 ) en4 = (double)energy[chG4] * (0.172727) + 0.518182; hG4->Fill(en4);
  
  /// Dynamic Rnage = 2.0
  double e1 = 0, e2 = 0, e4 = 0, eAll = 0;
  
  if( energy[chG1] > 10 ) {e1 = (double)energy[chG1] * (0.58522525) + 0.20466155; hG1->Fill(e1); countG[0] ++ ;}
  if( energy[chG2] > 10 ) {e2 = (double)energy[chG2] * (0.70091298) + 1.77740373; hG2->Fill(e2); countG[1] ++ ;}
  if( energy[chG4] > 10 ) {e4 = (double)energy[chG4] * (0.69996349) + 0.39210471; hG4->Fill(e4); countG[3] ++ ;}
  
  if( e1 > 0 && e2 > 0 ) hG12->Fill(e1,e2); //hG->Fill(e1+e2);};
  if( e2 > 0 && e4 > 0 ) hG24->Fill(e2,e4); //hG->Fill(e2+e4);};
  if( e4 > 0 && e1 > 0 ) hG41->Fill(e4,e1); //hG->Fill(e1+e4);};
  //if( e4 > 0 && e1 > 0 && e2 > 0 ) {hG->Fill(e1+e4+e2);};
  
  if( energy[chG1] > 10 || energy[chG2] > 10 || energy[chG4] > 10 ) hG->Fill(e1+e2+e4);
  
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
