#include <TFile.h>
#include <TTree.h>
#include <TObjArray.h>
#include <TClonesArray.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <TRandom.h>
#include <TH2.h>
#include <TAxis.h>
#include <iostream>

bool isDisplay = false; // false = DataProcess

void ReadWave(){

  TFile * file = new TFile("test.root");
  TTree * tree = (TTree *) file->FindObjectAny("tree");
  
  int numEvent = tree->GetEntries();
  printf("========= num of events : %d \n", numEvent);
  
  //TClonesArray * wave = new TClonesArray();
  TObjArray * wave = new TObjArray();
  tree->SetBranchAddress("wave", &wave);
  
  TCanvas * canvas = new TCanvas("c", "c", 600, 600);
  
  if( isDisplay ) canvas->Divide(2,2);
  
  TH2F * hXY = new TH2F("hXY", "hXY", 300, 0, 3000, 300, 0, 3000);
  TGraph * g = NULL;
  
  for( int ev = 0 ; ev < numEvent; ev ++ ){
    wave->Clear();
    tree->GetEntry(ev,0);  
    
    int size = wave->GetLast()+1;
    ///if( ev%100 == 0 ) {
    ///  printf(".");
    ///  gSystem->ProcessEvents();
    ///}
    //=============== Display
    if( isDisplay ){
      for( int i = 0; i < size; i+= 2){
        g = (TGraph *) wave->At(i);
        if( g->GetN() == 0 ) continue;
        g->GetYaxis()->SetRangeUser(4000, 10000);
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
    
    if( !isDisplay ){
      
      double A= 0, B = 0;
      for( int i = 0; i < size; i+=2){
        g = (TGraph *) wave->At(i);
        
        double p1 = g->Eval(500) + gRandom->Gaus(0, 10);
        double p2 = g->Eval(1500) + gRandom->Gaus(0, 10);
        
        if( i == 0 ) A = p2-p1;
        if( i == 4 ) B = p2-p1;
      }
      
      hXY->Fill(A,B);
      
      /**
      double A = 0 ,B = 0,C = 0,D = 0;
      
      for( int i = 0; i < size; i+= 2){
        g = (TGraph *) wave->At(i);
        
        //double yMax = g->GetYaxis()->GetXmax();
        //double yMin = g->GetYaxis()->GetXmin();
        
        double x, p1, p2, p3;
        ((TGraph *) wave->At(i))->GetPoint( 500/2, x, p1);
        ((TGraph *) wave->At(i))->GetPoint(2000/2, x, p2);
        ((TGraph *) wave->At(i))->GetPoint(2700/2, x, p3);
        
        //if( ev%1000 == 0 ) printf(" %f, %f, %f\n", p1, p2, p3);
      
        
        if( i == 0 ) A = (p3 >= p1) ? 0: (p2-p1);
        if( i == 2 ) B = (p3 >= p1) ? 0: (p2-p1);
        if( i == 4 ) C = (p3 >= p1) ? 0: (p2-p1);
        if( i == 6 ) D = (p3 >= p1) ? 0: (p2-p1);
        
      }
      
      //if( ev%1000 == 0 ) printf(" %f, %f, %f, %f \n", A, B, C, D);
      
      if( A > 0 && B > 0 && C > 0 && D > 0 ){
        
        double X = (B+C)/(A+B+C+D);
        double Y = (A+B)/(A+B+C+D);
        
        hXY->Fill(X,Y);
        
      }
      * */
      
      if( ev%1000 == 0 ){
        canvas->cd();
        hXY->Draw("colz");
        
        canvas->Modified();
        canvas->Update();
        gSystem->ProcessEvents();
      }
    }
    
  }
  
  

}
