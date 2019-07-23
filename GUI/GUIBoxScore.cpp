
#include <TApplication.h>
#include <TGClient.h>


#include <TGWindow.h>
#include <TGFrame.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TRootEmbeddedCanvas.h>
#include "GUIBoxScore.h"

#include "DigitizerClass.h"

MyMainFrame::MyMainFrame(const TGWindow *p,UInt_t w,UInt_t h){
           
		// Create a main frame
		fMain = new TGMainFrame(p,w,h);

    digitizer = new Digitizer(1);

		// Create canvas widget
		fEcanvas = new TRootEmbeddedCanvas("Ecanvas",fMain,w,h);
		fMain->AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10,10,10,1));
    
    TCanvas * test = fEcanvas->GetCanvas();
    test->Divide(2,1);
    
		// Create a horizontal frame widget with buttons
		TGHorizontalFrame *hframe = new TGHorizontalFrame(fMain,200,40);
		
		TGTextButton *draw = new TGTextButton(hframe,"&Draw");
		draw->Connect("Clicked()","MyMainFrame",this,"DoDraw()");
		hframe->AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));
		
		TGTextButton *exit = new TGTextButton(hframe,"&Exit", "gApplication->Terminate(0)");
		hframe->AddFrame(exit, new TGLayoutHints(kLHintsCenterX, 5,5,3,4));
		
		fMain->AddFrame(hframe, new TGLayoutHints(kLHintsCenterX, 2,2,2,2));

		// Set a name to the main frame
		fMain->SetWindowName("Simple Example");

		// Map all subwindows of main frame
		fMain->MapSubwindows();

		// Initialize the layout algorithm
		fMain->Resize(fMain->GetDefaultSize());

		// Map main frame
		fMain->MapWindow();
	
}

MyMainFrame::~MyMainFrame(){
  		   // Clean up used widgets: frames, buttons, layout hints
		fMain->Cleanup();
		delete fMain;
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
}


int main(int argc, char **argv) {
  
  TApplication theApp("App",&argc,argv);
   
  new MyMainFrame(gClient->GetRoot(),600,200);
   
  theApp.Run();
  return 0;
}
