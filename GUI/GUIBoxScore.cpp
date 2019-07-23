
#include <TApplication.h>
#include <TGClient.h>


#include <TGWindow.h>
#include <TGFrame.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TGLabel.h>
#include <TGStatusBar.h>
#include <TGComboBox.h>
#include <TRootEmbeddedCanvas.h>
#include "GUIBoxScore.h"

#include "DigitizerClass.h"

MyMainFrame::MyMainFrame(const TGWindow *p,UInt_t w,UInt_t h){

  // Create histogram;
  h1 = new TH1F ("h1", "h1", 500, 0, 500);
  h2 = new TH2F ("h2", "h2", 500, 0, 500, 500, 0, 500);
  
  isFileOpened = false;
  isACQStarted = false;
  isACQPaused  = false;
  isACQStopped = false;

  // Create a main frame
  fMain = new TGMainFrame(p,w,h);
  fMain->Connect("CloseWindow()", "MyMainFrame", this, "CloseWindow()");
  
  fMain->ChangeOptions((fMain->GetOptions()& ~kVerticalFrame) | kHorizontalFrame);

  digitizer = new Digitizer(1);

  // Create canvas widget
  fEcanvas = new TRootEmbeddedCanvas("Ecanvas",fMain,w,h);
  fMain->AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10,10,10,10));

  TCanvas * test = fEcanvas->GetCanvas();
  test->Divide(2,1);
  
  //Create a headup frame;
  TGVerticalFrame *hpFrame = new TGVerticalFrame(fMain,100,600);

  TGTextButton *bNewFile = new TGTextButton(hpFrame,"&New File");
  hpFrame->AddFrame(bNewFile, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));

  TGLabel * posLabel = new TGLabel(hpFrame, "Position : ");
  hpFrame->AddFrame(posLabel,  new TGLayoutHints(kLHintsCenterX, 5,5,3,4) );

  TGComboBox * posCombox = new TGComboBox(hpFrame); 
  posCombox->SetWidth(120);
  posCombox->SetHeight(20);
  posCombox->AddEntry("RAISOR Exit", 0x89);
  posCombox->AddEntry("HELIOS Cross", 0x82);
  posCombox->AddEntry("HELIOS ZeroDegree", 0xA4);
  posCombox->Connect("Selected(Int_t)", "MyMainFrame", this, "SetDigitizerChannelMask(uint32_t)");
  hpFrame->AddFrame(posCombox, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));

  TGTextButton *bStartPause = new TGTextButton(hpFrame,"&Start/Pause ACQ");
  //bStartPause->SetEnabled(false);
  bStartPause->Connect("Clicked()","MyMainFrame",this,"StartPauseACQ()");
  hpFrame->AddFrame(bStartPause, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));

  TGTextButton *bStop = new TGTextButton(hpFrame,"&Stop ACQ");
  //bStop->SetEnabled(false);
  bStop->Connect("Clicked()","MyMainFrame",this,"StopACQ()");
  hpFrame->AddFrame(bStop, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));

  TGTextButton *bCleanHist = new TGTextButton(hpFrame,"&Clean Hist.");
  //bCleanHist->SetEnabled(false);
  hpFrame->AddFrame(bCleanHist, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));

  TGTextButton *bCutCreator = new TGTextButton(hpFrame,"&Cut Creator");
  //bCutCreator->SetEnabled(false);
  hpFrame->AddFrame(bCutCreator, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));

  fMain->AddFrame(hpFrame, new TGLayoutHints(kLHintsRight, 10,10,10,10));
  

  // Create a horizontal frame widget with buttons
  //TGHorizontalFrame *hframe = new TGHorizontalFrame(fMain,200,40);
  //
  //TGTextButton *draw = new TGTextButton(hframe,"&Draw");
  //draw->Connect("Clicked()","MyMainFrame",this,"DoDraw()");
  //hframe->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));
  //
  //TGTextButton *exit = new TGTextButton(hframe,"&Exit", "gApplication->Terminate(0)");
  //hframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));
  //
  //fMain->AddFrame(hframe, new TGLayoutHints(kLHintsCenterX, 2,2,2,2));

  // status bar
  //Int_t parts[] = {33, 10, 10, 47};
  //TGStatusBar *fStatusBar = new TGStatusBar(fMain,50,10,kHorizontalFrame);
  //fStatusBar->SetParts(parts,4);
  //fMain->AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsLeft | kLHintsExpandX, 0, 0, 2, 0));

  //TODO for each object
  // fill status bar fields with information; selected is the object
  // below the cursor; atext contains pixel coordinates info
  //fStatusBar->SetText(selected->GetTitle(),0);
  //fStatusBar->SetText(selected->GetName(),1);
  //fStatusBar->SetText(atext,2);
  //fStatusBar->SetText(selected->GetObjectInfo(px,py),3);

  // Set a name to the main frame
  fMain->SetWindowName("BoxScore for RAISOR");

  // Map all subwindows of main frame
  fMain->MapSubwindows();

  // Initialize the layout algorithm
  fMain->Resize(fMain->GetDefaultSize());

  // Map main frame
  fMain->MapWindow();

}

MyMainFrame::~MyMainFrame(){

  delete digitizer;

  // Clean up used widgets: frames, buttons, layout hints
  fMain->Cleanup();
  delete fMain;

  delete h1;
  delete h2;

}


void MyMainFrame::SetDigitizerChannelMask(uint32_t mask){
  
  digitizer->SetChannelMask(mask);
  
}

void MyMainFrame::CloseWindow(){
  gApplication->Terminate(0);
}

void MyMainFrame::StartPauseACQ(){
  
  if( !isACQStarted ){
    
    isACQStarted = true;
    isACQPaused = false;
    isACQStopped = false;
      
    //DoDraw();
    
    EventGenerator();
  } 
  
  if( !isACQPaused ){
    
    isACQStarted = false;
    isACQPaused = true;
    isACQStopped = false;
     
    
  }
  
}

void MyMainFrame::StopACQ(){
  
  if( !isACQStopped ){
    
    isACQStarted = false;
    isACQPaused = false;
    isACQStopped = true;
    
  }
  
}

void MyMainFrame::EventGenerator(){
  
  //this is a dummy eventGenerator.
  //this will continuously generate data and fill the histogram;
  
  // the testing objective is, when this generator is running, can I press other buttons?
  
  int count = 0;
  
  do{
    
    h1->Fill( gRandom->Rndm() * 500 );
    
    h2->Fill( gRandom->Rndm() * 500, gRandom->Rndm() * 500 );
    
    count++;
    if( count% 50 == 0 ) UpdatePlot();
    
  }while( isACQStarted );
  
  
}


void MyMainFrame::UpdatePlot(){
  
  TCanvas *fCanvas = fEcanvas->GetCanvas();
  fCanvas->cd(1); h1->Draw();
  
  fCanvas->cd(2); h2->Draw();

  fCanvas->Update();
}

void MyMainFrame::DoDraw(){
  
  TCanvas *fCanvas = fEcanvas->GetCanvas();
  fCanvas->cd(1);
  fCanvas->cd(1)->SetGrid();

  // Draws function graphics in randomly chosen interval
  TF1 *f1 = new TF1("f1","sin(x)/x",0,gRandom->Rndm()*10);
  f1->SetLineWidth(3);
  f1->Draw();


  fCanvas->cd(2);
  fCanvas->cd(2)->SetGrid();

  TF1 *f2 = new TF1("f2","sin(x)",0,gRandom->Rndm()*10);
  f2->SetLineWidth(3);
  f2->Draw();
  fCanvas->Update();

  digitizer->GetBoardConfiguration();
}


int main(int argc, char **argv) {
  
  TApplication theApp("App",&argc,argv);
   
  new MyMainFrame(gClient->GetRoot(),1200,800);
   
  theApp.Run();
  return 0;
}
