#ifndef HELIOSTARGET
#define HELIOSTARGET

#include "../Class/GenericPlane.h"

class HeliosTarget : public GenericPlane{
  RQ_OBJECT("HelioTarget");
public:

  HeliosTarget();
  ~HeliosTarget();
  
  void SetXYHistogram(int xMin, int xMax, int yMin, int yMax);
  void SetCanvasDivision();
  
  void Fill(vector<UInt_t> energy);
  
  void Draw();

private:
  
  TH1F * hX;
  TH1F * hY;
  TH1F * hRing;
  TH2F * hXY;
  
};

HeliosTarget::HeliosTarget(){
  
  hX = NULL;
  hY = NULL;
  hRing = NULL;
  hXY = NULL;
  
}

HeliosTarget::~HeliosTarget(){
  
  delete hX;
  delete hY;
  delete hRing;
  delete hXY;
  
}

void HeliosTarget::SetXYHistogram(int xMin, int xMax, int yMin, int yMax){
  
  int bin = 200;
  float labelSize = 0.08;
  
  hX = new TH1F("hX", "X; X[ch]; count", bin, xMin, xMax);
  hY = new TH1F("hY", "Y; Y[ch]; count", bin, yMin, yMax);
  
  hXY = new TH2F("hXY", "X-Y; X[ch]; Y[ch]", bin, xMin, xMax, bin, yMin, yMax);
  
  hX->GetXaxis()->SetLabelSize(labelSize);
  hX->GetYaxis()->SetLabelSize(labelSize);
  
  hY->GetXaxis()->SetLabelSize(labelSize);
  hY->GetYaxis()->SetLabelSize(labelSize);
  
  hXY->GetXaxis()->SetLabelSize(labelSize);
  hXY->GetYaxis()->SetLabelSize(labelSize);
  
  hXY->SetMinimum(1);
  
  isHistogramSet = true;
}

void HeliosTarget::SetCanvasDivision(){
 
  GenericPlane::SetCanvasDivision();
  
  /*  
  fCanvas->Divide(1,2);
  fCanvas->cd(1)->Divide(2,1); 
  fCanvas->cd(1)->cd(1)->SetLogz();
  fCanvas->cd(1)->cd(2)->Divide(2,2);
  
  
  fCanvas->cd(2)->Divide(2,1); 
  
  fCanvas->cd(2)->SetGridy();
  fCanvas->cd(2)->SetTicky();
  fCanvas->cd(2)->SetTickx();
  
  fCanvas->cd(1)->cd(2)->cd(3)->SetGridy();
  fCanvas->cd(1)->cd(2)->cd(3)->SetTicky();
  fCanvas->cd(1)->cd(2)->cd(3)->SetTickx(); 
  fCanvas->cd(1)->cd(2)->cd(4)->SetLogy(); 
  */
}

void HeliosTarget::Fill(vector<UInt_t> energy){
  
  GenericPlane::Fill(energy);
  
}

void HeliosTarget::Draw(){
  
  GenericPlane::Draw();
}

#endif
