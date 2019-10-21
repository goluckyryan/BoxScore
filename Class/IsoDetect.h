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

private:
  
  //==== add 4 histograms for Glover detectors
  TH1F * hG1;
  TH1F * hG2;
  TH1F * hG3;
  TH1F * hG4;
  
  int chG1, chG2, chG3, chG4; 
  
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
  chE  = 2;   chEGain = 0;
  mode = 1; ///default channel Gain is equal
  
  //=========== custom histograms for HelioTarget
  hG1 = NULL;
  hG2 = NULL;
  hG3 = NULL;
  hG4 = NULL;
  
  chG1 = 4;
  chG2 = 5;
  chG3 = 6;
  chG4 = 7;
  
  GenericPlane::SetChannelMask(1,1,1,1, 0,1,0,1);
  
  isHistogramSet = false;
  
}

IsoDetect::~IsoDetect(){
  
  delete hG1;
  delete hG2;
  delete hG3;
  delete hG4;
  
}

void IsoDetect::SetOthersHistograms(){
  
  int bin = 200;
  float labelSize = 0.08;
  
  float xMin = 0;
  float xMax = 160000;
  
  hG1 = new TH1F("hG1", "G1; [ch]; count", bin, xMin, xMax);
  hG2 = new TH1F("hG2", "G2; [ch]; count", bin, xMin, xMax);
  hG3 = new TH1F("hG3", "G3; [ch]; count", bin, xMin, xMax);
  hG4 = new TH1F("hG4", "G4; [ch]; count", bin, xMin, xMax);
  
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
   
  hG1->Fill(energy[chG1]);
  hG2->Fill(energy[chG2]);
  hG3->Fill(energy[chG3]);
  hG4->Fill(energy[chG4]);
  
}

void IsoDetect::ClearHistograms(){
  
  GenericPlane::ClearHistograms();
  
  hG1->Reset();
  hG2->Reset();
  hG3->Reset();
  hG4->Reset();
  
}

#endif
