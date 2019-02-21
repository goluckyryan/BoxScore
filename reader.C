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

int DoSomething(int start, TH2F * horg){
  
  //open root file
  TFile * file = new TFile("tree.root");
  //find tree
  TTree * tree = (TTree*) file->Get("tree");
  
  if( tree == NULL ) return start;
  
  int n = tree->GetEntries();
  
  if( start == n ) return -1;
  
  printf(" From Entry : %d - %d \n", start,  n);
  
  UInt_t x = 0;
  ULong64_t t = 0;
  int ch = -1;
  tree->SetBranchAddress("e", &x);
  tree->SetBranchAddress("t", &t);
  tree->SetBranchAddress("ch", &ch);

  //load data
  const int k = n - start + 1;
  UInt_t * xArr = new UInt_t[k];
  ULong64_t * tArr = new ULong64_t[k];
  int * chArr = new int[k];
  
  for ( int i = 0; i < k ; i++){
    tree->GetEntry(start + i);
    xArr[i] = x;
    tArr[i] = t;
    chArr[i] = ch;
  }
  file->Close();
  
  printf(" Number of raw event considered : %d \n", k);
  //build event
  int count = 0;
  for ( int i = 0; i < k-1 ; i++){
    for( int j = i + 1; j < k; j++){
        if( chArr[i] == chArr[j] ) continue;
        int timediff = (int) (tArr[i] - tArr[j]) ;
        if( TMath::Abs( timediff ) < 10 ) {
            if( chArr[i] == 0 ) horg->Fill( xArr[i], xArr[j] ) ;
            if( chArr[i] == 1 ) horg->Fill( xArr[j], xArr[i] ) ;
            count ++;
            break;
        }
    }
  }
  printf(" Number of event built : %d \n", count);

  return n;
  
}

int reader (){
  
  gErrorIgnoreLevel = kError; //suppress warning from reading non-closed tree.
  
  int startEvent = 0;
  
  TCanvas * cReader = new TCanvas("cReader", "Reader", 0, 0, 400, 400);
    
  //TH1F * horg = new TH1F("horg", "origin data", 1000, 0, 50000);
  TH2F * horg = new TH2F("horg", "origin data", 1000, 1000, 3000, 1000, 1000, 3000);
  //TH2F * hsorted = new TH2F("hsorted", "sorted data", 1000, 0, 30000, 100, 0, 1);
  
  do{ 
    
    int wait = TMath::Min((int)2e6, (int)gRandom->Integer(3e6)); //wati for at least 1 sec
    usleep(wait); //wait for arbitary time;
        
    startEvent = DoSomething(startEvent, horg);
    
    horg->Draw();
    
    cReader->Update();
    gSystem->ProcessEvents();
    
  }while( startEvent > 0 );

  return 0;
   
}
