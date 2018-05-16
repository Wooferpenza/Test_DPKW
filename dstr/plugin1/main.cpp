#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../include/stubs.h"

//#include "wx/xrc/xmlres.h"          // XRC XML resouces

#include "wxfft.h"
#include "osc_dlg.h"

extern int sport;

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();
};

IMPLEMENT_APP(MyApp)


// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
   // create the main application window
   if(argc>1) sport = atoi(argv[1]); else sport = 3000;
   DskFrame *frame = new DskFrame(NULL,-1, "DAQ server (PCI boards)", wxPoint(100,100), wxSize(250,480));
   frame->Show(TRUE);
   return TRUE;
}

int MyApp::OnExit()
{
    // clean up
    return 0;
}

