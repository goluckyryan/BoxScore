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
  
  if( argc != 8 && argc != 9  && argc != 11) {
    //printf("Please input channel for dE and E. \n");
    printf("./CutCreator [rootFile] [chDE] [chE] [rangeDE_min] [rangeDE_max] [rangeE_min rangeE_max] [mode] [gainDE] [gainE]\n");
    return 0;
  }
  
  string rootFile = argv[1];
  
  int chDE = atoi(argv[2]);
  int chEE = atoi(argv[3]);
  
  int rangeDE_min = atoi(argv[4]);
  int rangeDE_max = atoi(argv[5]);
  
  int rangeE_min = atoi(argv[6]);
  int rangeE_max = atoi(argv[7]);
  
  int mode = 0 ;
  if( argc >= 9) mode = atoi(argv[8]);
  
  float gainDE = 1, gainE = 1;
  if( argc >= 10) gainDE = atof(argv[9]);
  if( argc >= 11) gainE = atof(argv[10]);
  
  printf("========================\n");
  printf("   mode = %d\n", mode);
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
  if( mode == 0 ) {
    hEdE = new TH2F("hEdE", "dE - totE = dE + E ; totE [ch] ; dE [ch]", 500, rangeE_min + rangeDE_min, rangeE_max + rangeDE_max, 500, rangeDE_min , rangeDE_max );
    expression.Form("e[%d]:e[%d] + e[%d]>>hEdE", chDE, chEE, chDE);
  }else if ( mode == 1 ){
    hEdE = new TH2F("hEdE", "dE - totE = dE + E/4 ; totE [ch] ; dE [ch]", 500, rangeE_min/4. + rangeDE_min, rangeE_max/4. + rangeDE_max, 500, rangeDE_min , rangeDE_max );
    expression.Form("e[%d]:e[%d]/4. + e[%d]>>hEdE", chDE, chEE, chDE);
  }else if ( mode == 2){
    hEdE = new TH2F("hEdE", "dE - totE = dE/4 + E ; totE [ch] ; dE [ch]", 500, rangeE_min + rangeDE_min/4., rangeE_max + rangeDE_max/4., 500, rangeDE_min , rangeDE_max );
    expression.Form("e[%d]:e[%d] + e[%d]/4.>>hEdE", chDE, chEE, chDE);
  }else if ( mode == 4){
    hEdE = new TH2F("hEdE", Form("dE - totE = %4.2fdE + %4.2fE ; totE [ch] ; dE [ch]", gainDE, gainE), 500, rangeE_min * gainE + rangeDE_min * gainDE, rangeE_max * gainE + rangeDE_max* gainDE, 500, rangeDE_min * gainDE , rangeDE_max * gainDE );
    expression.Form("e[%d]:e[%d]*%4.2f + e[%d]*%4.2f>>hEdE", chDE, chEE, gainE, chDE, gainDE);
  }
  tree->Draw(expression, "", "colz");
  gSystem->ProcessEvents();
  
  // make cuts
  cutFile = new TFile("cutsFile.root", "recreate");
  cutList = new TObjArray();

  int count = 1;
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

    TString name; name.Form("cut%d", count);
    cut->SetName(name);
    cut->SetLineColor(count);
    cutList->Add(cut);
    
    printf(" cut-%d \n", count);
    count++;
  
  }while( cut != NULL );

  cutList->Write("cutList", TObject::kSingleKey);

  printf("====> saved %d cuts into rdtCuts.root\n", count);

  gROOT->ProcessLine(".q");

  app.Run();
  
  return 0;
  
}
