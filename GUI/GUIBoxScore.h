#include <TQObject.h>
#include <RQ_OBJECT.h>

//prototype declariation
class TGWindow;
class TGMainFrame;
class TRootEmbeddedCanvas;
class Digitizer;
class TH1F;
class TH2F;

class GUIBoxScore {
  RQ_OBJECT("GUIBoxScore")
private:
  TGMainFrame         *fMain;
  TRootEmbeddedCanvas *fEcanvas;
  Digitizer           *digitizer;

  TH1F * h1;
  TH2F * h2;

  bool isFileOpened;
  bool isACQStarted;
  bool isACQPaused;
  bool isACQStopped;
  

public:
  GUIBoxScore(const TGWindow *p,UInt_t w,UInt_t h);
  virtual ~GUIBoxScore();

  void SetDigitizerChannelMask(uint32_t mask);

  void StartPauseACQ();
  void StopACQ();


  void EventGenerator();
  void UpdatePlot();

  void CloseWindow();
  void DoDraw();

};

