#ifndef FILEIO
#define FILEIO

#include <TQObject.h>
#include <RQ_OBJECT.h>
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

using namespace std;

class FileIO {
  RQ_OBJECT("FileIO")
public:

  FileIO(TString filename);
  ~FileIO();
  
  void SetTree(TString treeName, int NumChannel);
  void Append();
  
  void FillTree(int * Channel, UInt_t * Energy, ULong64_t* TimeStamp);
  void WriteMacro(TString file);
  void WriteHistogram(TH1F * hist) { hist->Write("", TObject::kOverwrite); }
  void WriteHistogram(TH2F * hist) { hist->Write("", TObject::kOverwrite); }
  void WriteHistogram(TMultiGraph * graph, TString name) { graph->Write(name, TObject::kOverwrite); }
  void WriteHistogram(TGraph * graph, TString name) { graph->Write(name, TObject::kOverwrite); }
  
  void WriteObjArray(TObjArray * objArray){ fileOut->cd(); objArray->Write();} 
  
  void Close(){
    if( tree != NULL ) tree->Write("", TObject::kOverwrite); 
    fileOut->Close();
  }
  
  double GetFileSize(){ return fileOut->GetSize() / 1024. / 1024.;}
  
private:
  TString fileOutName;
  TFile * fileOut;
  
  TString treeName;
  TTree * tree;
  
  int NumChannel;
  ULong64_t * timeStamp;
  UInt_t * energy;
  int * channel;
  
};

FileIO::FileIO(TString filename){
  
  fileOutName = filename;
  
  fileOut = new TFile(filename, "RECREATE");
  
  tree = NULL;
  
  NumChannel = 0;
  timeStamp = NULL;
  energy = NULL;
  channel = NULL;
  
}

FileIO::~FileIO(){

  delete fileOut;
  
  delete timeStamp;
  delete energy;
  delete channel;
  
}

void FileIO::WriteMacro(TString file){
  //printf("writing file %s \n", file.Data());
  TMacro macro(file);
  TString writeName = file;
  int finddot = file.Last('.');
  writeName.Remove(finddot);
  macro.Write(writeName,  TObject::kOverwrite);
}

void FileIO::SetTree(TString treeName, int NumChannel){
  
  this->treeName = treeName;
  tree = new TTree(treeName, treeName);
  
  this->NumChannel = NumChannel;
  
  timeStamp = new ULong64_t[NumChannel];
  energy    = new UInt_t[NumChannel];
  channel   = new int[NumChannel];
  
  TString expre;
  expre.Form("channel[%d]/I", NumChannel); tree->Branch("ch", channel, expre);
  expre.Form("energy[%d]/i", NumChannel); tree->Branch("e", energy, expre);
  expre.Form("timeStamp[%d]/l", NumChannel); tree->Branch("t", timeStamp, expre);
  
  
  tree->Write(treeName, TObject::kOverwrite); 
  
}

void FileIO::Append(){
  
  fileOut = new TFile(fileOutName, "UPDATE");
  tree = (TTree*) fileOut->Get(treeName);
      
  tree->SetBranchAddress("e", energy);
  tree->SetBranchAddress("t", timeStamp);
  tree->SetBranchAddress("ch", channel);
     
}

void FileIO::FillTree(int * Channel, UInt_t * Energy, ULong64_t * TimeStamp){
  
  for(int ch = 0; ch < NumChannel; ch++){
    energy[ch] = Energy[ch];
    timeStamp[ch] = TimeStamp[ch];
    channel[ch] = Channel[ch];
  }
  
  tree->Fill();
}



#endif
