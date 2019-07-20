#include <TApplication.h>
#include <TGClient.h>
#include "test.h"

int main(int argc, char **argv) {
   TApplication theApp("App",&argc,argv);
   
   new MyMainFrame(gClient->GetRoot(),600,200);
   
   theApp.Run();
   return 0;
}
