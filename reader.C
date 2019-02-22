#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TBranch.h"
#include "TBenchmark.h"
#include "TRandom.h"
#include "TMath.h"

#include <stdlib.h> 
#include <unistd.h>

#include "src/keyb.c"

using namespace std;

TH1F * hE;
TH1F * hdE;
TH2F * hEdE;

void bubbleSort(float arr[], int n) ;
int DoSomething(int start);
//int kbhit(); // if keyboard hit
//int getch(void);
//static void cooked(void);
//static void raw(void);


int reader (){
  
  gErrorIgnoreLevel = kError; //suppress warning from reading non-closed tree.
  
  int startEvent = 0;
  
  TCanvas * cReader = new TCanvas("cReader", "Reader", 0, 0, 1200, 400);
  cReader->Divide(3,1);
  
  int nBin = 1000;
  int range[2] = {0, 16000};
  double resol = (range[1] - range[0])/ nBin * 1.;
  hE   = new TH1F("hE"  , Form("E ; E [ch]; count / %.2f ch ", resol), nBin, range[0], range[1]);
  hdE  = new TH1F("hdE" , Form("dE ; dE [ch]; count / %.2f ch ", resol), nBin, range[0], range[1]);
  hEdE = new TH2F("hEdE", "dE - E ; E [ch]; dE [ch]", nBin, range[0], range[1], nBin, range[0], 7000);
  
  
  bool autoRefresh = false;
  do{ 
    
    /*bool refresh = false;
    if( kbhit()){
      char c = getch();
      if( c == 'q' ) {
        printf("===== bye bye ===== \n");
        break;
      }
      
      if( c == 'a' ) autoRefresh = true;
      if( c == 's' ) autoRefresh = false;
      
      if( c == 'r' ) refresh = true;
      
    }*/
      
    //if( autoRefresh || refresh ) {
      startEvent = DoSomething(startEvent);
      
      cReader->cd(1);
      hEdE->Draw("colz");
      cReader->cd(2);
      hE->Draw("colz");
      cReader->cd(3);
      hdE->Draw("colz");
      
      cReader->Update();
      gSystem->ProcessEvents();

      if( startEvent == -1 ) break;
    //}

    //if( autoRefresh ) {
      int wait = TMath::Max((int)1e6, (int)gRandom->Integer(3e6)); //wati for at least 1 sec
      printf("------------ next read after %.4f sec\n", wait/1e6);
      usleep(wait); //wait for arbitary time;
    //}
  }while( startEvent > 0 );

  return 0;
   
}
/***************************************************/

int DoSomething(int start){
  
  //open root file
  TFile * file = new TFile("tree.root");
  //find tree
  TTree * tree = (TTree*) file->Get("tree");
  
  if( tree == NULL ) {
      printf(" cannot get tree \n");
      return start;
  }
  int n = tree->GetEntries();
  
  if( start == n ) return -1;
  
  printf(" From Entry : %d - %d \n", start,  n);
  
  UInt_t x[8];
  ULong64_t t[8];
  tree->SetBranchAddress("e", x);
  tree->SetBranchAddress("t", t);
  
  //load data
  const int k = n - start + 1;
  
  for ( int i = 0; i < k ; i++){
    tree->GetEntry(start + i);
    
    double dE = x[4];
    double EE = x[6];
    double totalE = dE+ EE;
    
    
    hdE -> Fill(dE);
    hE ->Fill( EE);
    hEdE->Fill( EE, dE); //x, y
    
  }
  file->Close();

  return n;
  
}

void bubbleSort(float arr[], int n) { 
  int i, j; 
  bool swapped; 
  for (i = 0; i < n-1; i++) 
  { 
    swapped = false; 
    for (j = 0; j < n-1-i; j++) 
    { 
      if (arr[j] > arr[j+1]) 
      { 
        float temp = arr[j];
        arr[j] = arr[j+1];
        arr[j+1] = temp;
        //std::swap(&arr[j], &arr[j+1]); 
        swapped = true; 
      } 
    } 

    // IF no two elements were swapped by inner loop, then break 
    if (swapped == false) break; 
  } 
} 
