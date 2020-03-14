//compile g++ -pthread thread.c -o thread `root-config --cflags --glibs`


#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <thread>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h> 
#include <vector>

#include "TROOT.h"
#include "TSystem.h"
#include "TFile.h"
#include "TStyle.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TApplication.h"
#include <TH2F.h>
#include <TCutG.h>
#include <TString.h>
#include <TObjArray.h>

using namespace std;

TFile * cutFile = NULL ;
TCutG * cut = NULL;
TObjArray * cutList = NULL;

int main(int argc, char* argv[] ){
  
  if( argc != 9 && argc != 10  && argc != 12) {
    //printf("Please input channel for dE and E. \n");
    printf("./CutCreator [rootFile] [opt] [chDE] [chE] [rangeDE_min] [rangeDE_max] [rangeE_min rangeE_max] [mode] [gainDE] [gainE]\n");
    printf("                          | \n");
    printf("                          + opt = recreate / update \n");
    return 0;
  }
  
  // mode = 0 ; gain_dE   = gain_E
  // mode = 1 ; gain_dE   = gain_E/4
  // mode = 2 ; gain_dE/4 = gain_E
  // mode = 3 ; custom gain
  
  string rootFile = argv[1];
  string cutopt = argv[2];
  
  int chDE = atoi(argv[3]);
  int chEE = atoi(argv[4]);
  
  int rangeDE_min = atoi(argv[5]);
  int rangeDE_max = atoi(argv[6]);
  
  int rangeE_min = atoi(argv[7]);
  int rangeE_max = atoi(argv[8]);
  
  int mode = 0 ;
  if( argc >= 10) mode = atoi(argv[9]);
  
  float gainDE = 1, gainE = 1;
  if( argc >= 11) gainDE = atof(argv[10]);
  if( argc >= 12) gainE = atof(argv[11]);
  
  printf("========================\n");
  printf("   mode = %d\n", mode);
  printf("dE range (%d, %d)\n", rangeDE_min, rangeDE_max);
  printf(" E range (%d, %d)\n", rangeE_min, rangeE_max);
  printf("gain dE = %f\n", gainDE);
  printf("gain  E = %f\n", gainE);


  TApplication app ("app", &argc, argv);

  // load tree and plot 
  TFile * fileIn = new TFile(rootFile.c_str());
  TTree * tree = (TTree*) fileIn->FindObjectAny("tree");
  gStyle->SetOptStat("");
  
  TCanvas * cCutCreator = new TCanvas("cCutCreator", "TCutG Creator", 0, 0, 800, 800);
  if( !cCutCreator->GetShowToolBar() ) cCutCreator->ToggleToolBar();
  if( cCutCreator->GetShowEditor() ) cCutCreator->ToggleEditor(); 

  TH2F * hEdE = NULL;
  
  TString expression;
  if( mode == 0 ) {  //same gain
    hEdE = new TH2F("hEdE", "dE - totE = dE + E ; totE [ch] ; dE [ch]", 500, rangeE_min + rangeDE_min, rangeE_max + rangeDE_max, 500, rangeDE_min , rangeDE_max );
    expression.Form("e[%d]:e[%d] + e[%d]>>hEdE", chDE, chEE, chDE);
  }else if ( mode == 1 ){ // dE = 2Vpp, E = 0.5Vpp
    hEdE = new TH2F("hEdE", "dE - totE = dE + E/4 ; totE [ch] ; dE [ch]", 500, rangeE_min/4. + rangeDE_min, rangeE_max/4. + rangeDE_max, 500, rangeDE_min , rangeDE_max );
    expression.Form("e[%d]:e[%d]/4. + e[%d]>>hEdE", chDE, chEE, chDE);
  }else if ( mode == 2){  // dE = 0.5Vpp, E = 2 Vpp
    hEdE = new TH2F("hEdE", "dE - totE = dE/4 + E ; totE [ch] ; dE [ch]", 500, rangeE_min + rangeDE_min/4., rangeE_max + rangeDE_max/4., 500, rangeDE_min , rangeDE_max );
    expression.Form("e[%d]/4:e[%d] + e[%d]/4.>>hEdE", chDE, chEE, chDE);
  }else if ( mode == 3){  //custom gain
    hEdE = new TH2F("hEdE", Form("dE - totE = %4.2fdE + %4.2fE ; totE [ch] ; dE [ch]", gainDE, gainE), 
          500, 
          (int) rangeE_min * gainE + rangeDE_min * gainDE, 
          (int) rangeE_max * gainE + rangeDE_max* gainDE, 
          500, 
          (int) rangeDE_min * gainDE , 
          (int) rangeDE_max * gainDE );
    expression.Form("e[%d]*%4.2f:e[%d]*%4.2f + e[%d]*%4.2f>>hEdE", chDE, gainDE, chEE, gainE, chDE, gainDE);
  }
  
  tree->Draw(expression, "", "colz");
  
  // make cuts
  TString cutFileName = "cutsFile.root";
  int prevCount = 0;
  cutFile = new TFile(cutFileName, cutopt.c_str());  
  printf("cutFileName = %s, cutopt = %s \n", cutFileName.Data(), cutopt.c_str());
  bool ListExist = cutFile->GetListOfKeys()->Contains("cutList");
  if(!ListExist){
    cutList = new TObjArray();
  }else{
      cutList = (TObjArray *) cutFile->FindObjectAny("cutList");
      prevCount = cutList->GetLast() + 1;
      for( int i = 0; i < prevCount ; i++){
         cutList->At(i)->Draw("same");
      }
   }
   
  gSystem->ProcessEvents();  
  int count = 1 + prevCount;
  
  do{
    printf("== make a graphic cut on the plot (double click on plot, or press 'x' to stop) : " );
    gSystem->ProcessEvents();
    gPad->WaitPrimitive();

    cut = (TCutG*) gROOT->FindObject("CUTG");

    if( cut == NULL) {
      printf(" break \n");
      count --;
      break;
    }

    char name[100];
    printf("Cut name ? ");
    int temp = scanf("%s", name);
    //TString name; name.Form("cut%d", count);
    cut->SetName(name);
    cut->SetLineColor(count);
    cutList->Add(cut);
    
    printf(" cut-%d : %s \n", count, name);
    count++;
  
  }while( cut != NULL );
  

  cutList->Write("cutList", TObject::kSingleKey);

  printf("====> saved %d cuts into %s\n", count, cutFileName.Data());

  gROOT->ProcessLine(".q");

  app.Run();
  
  return 0;
  
}
