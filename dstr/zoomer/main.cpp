#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif


#include "wx/image.h"
//#include "wx/xrc/xmlres.h"          // XRC XML resouces
#include "wx/config.h"
//#include "wx/confbase.h"
//#include "wx/fileconf.h"
//#include "wx/msw/regconf.h"
#include "scrlwnd.h"
#include "toolbox.h"
#include "min_info.h"
#include "hex_info.h"
#include "viewer.h" 


class MyApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();
};

IMPLEMENT_APP(MyApp)

wxString argFileName;
bool bOpenFile;

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
///    wxString strPath, strDir;
///    ::GetModuleFileName(::GetModuleHandle(NULL),
///                        strPath.GetWriteBuf(MAX_PATH), MAX_PATH);
///    strPath.UngetWriteBuf();

    // extract the dir name
///    wxSplitPath(strPath, &strDir, NULL, NULL);

// read from registry - config values   

   wxConfig *config = new wxConfig("ViewerApp");
   config->Write("Path",GetAppName());
   wxString path = config->Read("Path","");
//   if(path=="") { path = strDir+'\\'; config->Write("Path",path); }
   delete config;

    bOpenFile = false;
    if(argc>1) { argFileName = argv[1]; bOpenFile = true; }

    wxImage::AddHandler(new wxXPMHandler);
    wxImage::AddHandler( new wxPNGHandler );
    wxImage::AddHandler( new wxJPEGHandler);
    
    wxFileSystem::AddHandler(new wxZipFSHandler);
    
/////    wxXmlResource::Get()->InitAllHandlers();    
    
      
//    wxXmlResource::Get()->Load(wxT("rc/menu.xrc"));
//    wxXmlResource::Get()->Load(wxT("rc/toolbar.xrc"));
	//wxXmlResource::Get()->Load(_T("rc/about.xrc"));
/////   wxXmlResource::Get()->Load(_T("rc/dialog1.xrc"));
    //wxXmlResource::Get()->Load(path + _T("rc\\plugins\\setchan_wdr.xrc"));
    
    g_printData = new wxPrintData;
    g_pageSetupData = new wxPageSetupDialogData;

    wxHelpControllerHelpProvider* provider = new wxHelpControllerHelpProvider;
    wxHelpProvider::Set(provider);

    // create the main application window
    MyDecorFrame *frame = new MyDecorFrame("Zoomer 1.0b", wxPoint(0, 0), wxSize(640, 480));

    frame->SetFont(*wxITALIC_FONT);

    provider->SetHelpController(& frame->GetHtmlHelpController());

    frame->Show(TRUE);

    if ( !frame->GetHtmlHelpController().Initialize(_T("viewer")) )
    {
        wxLogError("Cannot initialize the MS HTML help system, aborting.");
        return FALSE;
    }

    wxSocketBase::Initialize();

    return TRUE;
}

int MyApp::OnExit()
{
    // clean up
    delete wxHelpProvider::Set(NULL);
    delete g_printData;
    delete g_pageSetupData;
    return 0;
}
