#include <TFile.h>
#include <TTree.h>
#include <TObjArray.h>
#include <TGraph.h>
#include <iostream>

void ReadWave(){

  TFile * file = new TFile("testing.root");
  TTree * tree = (TTree *) file->FindObjectAny("tree");
  
  int numEvent = tree->GetEntries();
  printf("========= num of events : %d \n", numEvent);
  
  TObjArray * wave = new TObjArray();
  tree->SetBranchAddress("wave", &wave);
  
  TCanvas * canvas = new TCanvas("c", "c", 600, 400);
  
  for( int ev = 0 ; ev < numEvent; ev ++ ){
    tree->GetEntry(ev);  
    
    int size = wave->GetLast()+1;
    
    for( int i = 0; i < size; i++){
      
      TGraph * g = (TGraph *) wave->At(i);
      g->SetTitle(Form("ev: %d, ch: %d", ev, i));
      
      if( g->GetN() > 0 ) {
        g->Draw("Al");
      
        canvas->Modified();
        canvas->Update();
        gSystem->ProcessEvents();
      
        cout << "Press Enter to Continue : ";
        cin.ignore();
        
        gSystem->ProcessEvents();
      
      }
    }
  }


}
