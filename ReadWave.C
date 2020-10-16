#include <TFile.h>
#include <TTree.h>
#include <TObjArray.h>
#include <TGraph.h>
#include <iostream>

void ReadWave(){

  TFile * file = new TFile("testing_2.root");
  TTree * tree = (TTree *) file->FindObjectAny("tree");
  
  int numEvent = tree->GetEntries();
  printf("========= num of events : %d \n", numEvent);
  
  TObjArray * wave = new TObjArray();
  tree->SetBranchAddress("wave", &wave);
  
  TCanvas * canvas = new TCanvas("c", "c", 600, 600);
  canvas->Divide(2,2);
  
  for( int ev = 0 ; ev < numEvent; ev ++ ){
    tree->GetEntry(ev);  
    
    int size = wave->GetLast()+1;
    
    for( int i = 0; i < size; i+= 2){
      TGraph * g = (TGraph *) wave->At(i);
      
      canvas->cd(i/2+1);
      
      //if( g->GetN() == 0 ) continue;
      
      double yMax = g->GetYaxis()->GetXmax();
      double yMin = g->GetYaxis()->GetXmin();
      
      double z1 = g->Integral(100, 200);
      
      g->SetTitle(Form("ev: %d, ch: %d, (%f, %f, %f)", ev, i, yMax, yMin, (yMax-yMin)/2.));
      
      g->Draw("Al");
      
        
      gSystem->ProcessEvents();
      
    }
    canvas->Modified();
    canvas->Update();
    gSystem->ProcessEvents();
    
    cout << "Press Enter to Continue : ";
    cin.ignore();
  }


}
