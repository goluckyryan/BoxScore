//compile g++ -pthread thread.c -o thread `root-config --cflags --glibs`


#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <thread>
#include <iostream>
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

int Scale(){
  
struct species {
  Int_t A = 0;
  Int_t q = 0;
  Float_t E = 0; Float_t v = 0; Float_t T = 0;
};

 Int_t pA, pq, pZ;
 Float_t pE;
Float_t BRho, ScaleBRho, scale;

  pA = pq = pZ = 0;
  pE = BRho = ScaleBRho = scale = 0.0;

  cout << "Primary beam. A, Z = ?" << endl; cin >> pA >> pZ;
  cout << "q?" << endl; cin >> pq;
  cout << "energy?" << endl; cin >> pE;
  cout << "scale?" << endl; cin >> scale;

  BRho = sqrt(pA*pE)/pq;
  ScaleBRho = BRho * scale;
  cout << ScaleBRho << endl;
std::vector<species> primary;
for(int i=0;i<5;i++){
  species pdb;
  pdb.A = pA;
  pdb.q = pq-i;
  pdb.E = (ScaleBRho*ScaleBRho * pdb.q*pdb.q)/pdb.A;
  pdb.v = sqrt((2* pdb.E * 9e16)/(pdb.A*931));
  pdb.T = 18.5e9/pdb.v;
  primary.push_back(pdb);
 }

std::vector<species> secondary1;
for(int i=0;i<5;i++){
  species prd1;
  prd1.A = primary.at(0).A + 1;
  prd1.q = prd1.A - primary.at(0).q - i;
  prd1.E = (ScaleBRho*ScaleBRho * prd1.q*prd1.q)/prd1.A;
  prd1.v = sqrt((2* prd1.E * 9e16)/(prd1.A*931));
  prd1.T = 18.5e9/prd1.v;
  secondary1.push_back(prd1);
 }

 std::vector<species> secondary2;
for(int i=0;i<5;i++){
  species prd2;
  prd2.A = primary.at(0).A + 1;
  prd2.q = prd2.A - primary.at(0).q - i;
  prd2.E = (ScaleBRho*ScaleBRho * prd2.q*prd2.q)/prd2.A;
  prd2.v = sqrt((2* prd2.E * 9e16)/(prd2.A*931));
  prd2.T = 18.5e9/prd2.v;
  secondary2.push_back(prd2);
 }

std::vector<species> secondary3;
for(int i=0;i<5;i++){
  species prd3;
  prd3.A = primary.at(0).A + 1;
  prd3.q = prd3.A - primary.at(0).q - i;
  prd3.E = (ScaleBRho*ScaleBRho * prd3.q*prd3.q)/prd3.A;
  prd3.v = sqrt((2* prd3.E * 9e16)/(prd3.A*931));
  prd3.T = 18.5e9/prd3.v;
  secondary3.push_back(prd3);
 }
 cout << "E: ";
 for (int i=0;i<5;i++){
   cout << secondary1.at(i).E << " ";
 }
  



  return 0;
}
