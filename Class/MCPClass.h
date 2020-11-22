#ifndef MCPCLASS
#define MCPCLASS

#include "../Class/GenericPlane.h"

class MicroChannelPlate : public GenericPlane{
  RQ_OBJECT("MicroChannelPlate");
public:

  MicroChannelPlate();
  ~MicroChannelPlate();
  
  void SetOthersHistograms();
  void SetCanvasTitleDivision(TString titleExtra);
  
  void FillWaveEnergies(double * energy);
  
  void Draw();

  void ClearHistograms();

private:
  
  TH1F * hX1;
  TH1F * hY1;
  TH1F * hX2;
  TH1F * hY2;
  TH2F * hXY;

  
  int chA;
  int chC;
  
  int chB;
  int chD;
  
};

MicroChannelPlate::MicroChannelPlate(){
    
  //=========== ClassName and ClassID is for class identification in BoxScoreXY
  className = "MCP";
  classID = 999;
  location = "MCP";  //this is for database tag
  
  //=========== Channel Mask and rangeDE and rangeE is for GenericPlane
  ChannelMask = 0x25; /// Channel enable mask, 0x01, only frist channel, 0xff, all channel
  
  //rangeDE[0] =     0; /// min range for dE
  //rangeDE[1] = 60000; /// max range for dE
  //rangeE[0] =      0; /// min range for E
  //rangeE[1] =  60000; /// max range for E
  //rangeTime =    500; /// range for Tdiff, nano-sec
  //
  //chdE = 0;  chdEGain = 1; 
  //chE = 2;   chEGain = 1;
  //mode = 1; ///default channel Gain is equal
  
  NChannelForRealEvent = 4;
  
  //=========== custom histograms for HelioTarget
  hX1 = NULL;
  hX2 = NULL;
  hY1 = NULL;
  hY2 = NULL;
  hXY = NULL;
  
  hHit = NULL;
  
  chA = 0;
  chB = 2;
  chC = 4;
  chD = 6;
  //GenericPlane::SetChannelMask(0,1,0,1,0,1,0,1);
  GenericPlane::SetChannelMask(0,0,0,0,0,0,0,1);
  
}

MicroChannelPlate::~MicroChannelPlate(){
  
  delete hX1;
  delete hY1;
  delete hX2;
  delete hY2;
  delete hXY;
  
  delete hHit;
  
}

void MicroChannelPlate::SetOthersHistograms(){
  
  int bin = 200;
  float labelSize = 0.08;
  
  float xMin =  0 ;
  float xMax =  4000;
  float yMin =  0;
  float yMax =  4000;
  
  hX1 = new TH1F("hX1", "A; A[ch]; count", bin, xMin, xMax);
  hY1 = new TH1F("hY1", "B; B[ch]; count", bin, yMin, yMax);
  hX2 = new TH1F("hX2", "C; C[ch]; count", bin, xMin, xMax);
  hY2 = new TH1F("hY2", "D; D[ch]; count", bin, yMin, yMax);
  
  hXY = new TH2F("hXY", "X-Y; X[ch]; Y[ch]", bin, 0.2, 0.8, bin, 0.2, 0.8);
  
  //hX->GetXaxis()->SetLabelSize(labelSize);
  //hX->GetYaxis()->SetLabelSize(labelSize);
  //
  //hY->GetXaxis()->SetLabelSize(labelSize);
  //hY->GetYaxis()->SetLabelSize(labelSize);
  
  hXY->GetXaxis()->SetLabelSize(labelSize);
  hXY->GetYaxis()->SetLabelSize(labelSize);
  
  hXY->SetMinimum(1);
  
  isHistogramSet = true;
  
  printf(" Histogram seted. \n");
  
}

void MicroChannelPlate::SetCanvasTitleDivision(TString titleExtra){
 
  //GenericPlane::SetCanvasDivision();
  
  fCanvas->Clear();
  
  fCanvas->Divide(2,1);
  
  fCanvas->cd(2)->Divide(2,2);
}

void MicroChannelPlate::Draw(){
  
  if ( !isHistogramSet ) return;
  
  fCanvas->cd(1)->cd(1); hXY->Draw("colz");

  fCanvas->cd(2)->cd(1); hX1->Draw("");
  fCanvas->cd(2)->cd(2); hX2->Draw("");
  fCanvas->cd(2)->cd(3); hY1->Draw("");
  fCanvas->cd(2)->cd(4); hY2->Draw("");
  //fCanvas->cd(2)->cd(1); hE->Draw("");
  //fCanvas->cd(2)->cd(2); hdE->Draw("");
  //fCanvas->cd(2)->cd(4); hHit->Draw("HIST");
  
  fCanvas->Modified();
  fCanvas->Update();
  gSystem->ProcessEvents();
  
}


void MicroChannelPlate::FillWaveEnergies(double * energy){
  
  //GenericPlane::Fill(energy);
  
  if ( !isHistogramSet ) return;

   ///if( energy[chA] > 100 )   hX1->Fill(energy[chA]);
   ///if( energy[chC] > 100 )   hY1->Fill(energy[chC]);
   ///if( energy[chB] > 100 )   hX2->Fill(energy[chB]);
   ///if( energy[chD] > 100 )   hY2->Fill(energy[chD]);

  //printf("%f, %f, %f, %f \n", energy[chA], energy[chC], energy[chB], energy[chD]);

  
  if( energy[chA] > 0 && energy[chB]>0 && energy[chC] > 0 && energy[chD] > 0 ){
    
    //printf("%f, %f, %f, %f \n", energy[chA], energy[chC], energy[chB], energy[chD]);

    int limit = 3500;
    if( energy[chA]< limit && energy[chB]< limit && energy[chC]< limit && energy[chD]< limit ){

      hX1->Fill(energy[chA]);
      hY1->Fill(energy[chC]);
      hX2->Fill(energy[chB]);
      hY2->Fill(energy[chD]);

      
      double total = energy[chA]+energy[chB]+energy[chC]+energy[chD];
      double X = (energy[chB]+energy[chC])/total;
      double Y = (energy[chA]+energy[chB])/total;
    
      hXY->Fill(X, Y);
    }
  }
  
}

void MicroChannelPlate::ClearHistograms(){
  
  GenericPlane::ClearHistograms();
  
  hX1->Reset();
  hY1->Reset();
  hX2->Reset();
  hY2->Reset();
  hXY->Reset();
  
  hHit->Reset();
  
}

#endif
