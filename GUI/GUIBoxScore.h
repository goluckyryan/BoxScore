#include <TQObject.h>
#include <RQ_OBJECT.h>

//prototype declariation
class TGWindow;
class TGMainFrame;
class TRootEmbeddedCanvas;
class Digitizer;

class MyMainFrame {
   RQ_OBJECT("MyMainFrame")
private:
   TGMainFrame         *fMain;
   TRootEmbeddedCanvas *fEcanvas;
   Digitizer           * digitizer;
public:
   MyMainFrame(const TGWindow *p,UInt_t w,UInt_t h);
   virtual ~MyMainFrame();
   
   void SetDigitizerChannelMask(uint32_t mask);


   void CloseWindow();
   void DoDraw();

};

