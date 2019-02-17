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

using namespace std;

void bubbleSort(float arr[], int n) 
{ 
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

int DoSomething(int start, TH2F * horg, TH2F * hsorted){
  
  TFile * file = new TFile("tree.root");

  TTree * tree = (TTree*) file->Get("tree");;
  
  int n = tree->GetEntries();
  
  if( start == n ){
    return -1;
  }
  printf(" From Entry : %d - %d \n", start,  n);
  
  UInt_t x = 0;
  tree->SetBranchAddress("e", &x);

  //load data
  float * xArr = new float[n];
  
  for ( int i = start; i < n ; i++){
    tree->GetEntry(i);
    xArr[i] = x;
    horg -> Fill(i, x);
  }
  file->Close();
  
  //sort
  bubbleSort( xArr, n);
  
  //plot
  for ( int i = start; i < n ; i++){
    hsorted -> Fill(i, xArr[i]);
  }
  
  return n;
  
}

int DoSomething2(int start, TH1F * horg){
  
  TFile * file = new TFile("tree.root");

  TTree * tree = (TTree*) file->Get("tree");
  
  if( tree == NULL ) return start;
  
  int n = tree->GetEntries();
  
  if( start == n ){
    return -1;
  }
  printf(" From Entry : %d - %d \n", start,  n);
  
  UInt_t x = 0;
  tree->SetBranchAddress("e", &x);

  //load data
  UInt_t * xArr = new UInt_t[n];
  
  for ( int i = start; i < n ; i++){
    tree->GetEntry(i);
    xArr[i] = x;
    horg -> Fill(x);
  }
  file->Close();
  
  return n;
  
}

int reader (){
  
  gErrorIgnoreLevel = kError; //suppress warning from reading non-closed tree.
  
  int startEvent = 0;
  
  TCanvas * cReader = new TCanvas("cReader", "Reader", 0, 0, 400, 400);
    
  TH1F * horg = new TH1F("horg", "origin data", 1000, 0, 50000);
  //TH2F * hsorted = new TH2F("hsorted", "sorted data", 1000, 0, 30000, 100, 0, 1);
  
  do{ 
    
    int wait = TMath::Min((int)2e6, (int)gRandom->Integer(3e6)); //wati for at least 1 sec
    usleep(wait); //wait for arbitary time;
    
    //startEvent = DoSomething(startEvent, horg, hsorted);
    
    startEvent = DoSomething2(startEvent, horg);
    
    horg->Draw();
    
    cReader->Update();
    gSystem->ProcessEvents();
    
  }while( startEvent > 0 );

  return 0;
   
}
