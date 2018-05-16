#ifndef __MINIMAL__
#define __MINIMAL__

#include "str_zoom.h"
#include "zoomer.h"
#include "scrlwnd.h"
#include "wx/dynload.h"
#include "wx/file.h"



//#include "wx/image.h"
#include "wx/help.h"
#include "wx/cshelp.h"

#include "wx/filesys.h"
#include "wx/fs_zip.h"

#include "wx/html/helpctrl.h"

#include "wx/metafile.h"
#include "wx/print.h"
#include "wx/printdlg.h"

#include "wx/socket.h"

extern wxString argFileName;
extern bool bOpenFile;

extern wxPrintData *g_printData;
extern wxPageSetupData* g_pageSetupData;

enum {
      ID_OPEN_FILE = (wxID_HIGHEST+200),
      ID_CLOSE_FILE,
      ID_SAVE_FILE,
      ID_PRINT,
      ID_PAGE_SETUP,
      ID_PRINT_SETUP,
      ID_PRINT_PREVIEW,
      ID_QUIT,
      ID_RESET_OFFS,
      ID_INFO,
      ID_HEX_VIEW,
      ID_SWITCH_BW,
      ID_CONFIG,
      ID_CH_CFG,
      ID_SAVE_CFG,
      ID_DAQ_CONN,
      ID_DAQ_DCONN,
      ID_HELP,
      ID_ABOUT,
      ID_ACH,
//      SOCKET_ID,
      CMD_OPEN_FILE,   // int for open file from cmd line...
      ID_PROCS,         // refresh from plugin 
      ID_PROCS_PNT,    // simple refresh
      ID_PROCS_CFG,    // configure
      ID_PROCS_LOST    // inform lost socket
     };

class MyFrame;

struct ParamBuffer
{
// config dialog data
   int FontSize;
   int NCh;
   wxString MaxData;
   wxString MinData;
   wxString YMin;
   wxString YMax;
   wxString Multi;
   wxString YText;
   wxString XText;
   int XPageSize1024;
   int XPageSizePts;

   double dXMin;
   double dXMax;
   
// converters
   int GetFontSize() { int fs[] = {8,10,12}; return fs[FontSize]; };

   int GetMaxData() { long t; MaxData.ToLong(&t); return t; };
   int GetMinData() { long t; MaxData.ToLong(&t); return (-t); };
   int GetNCh() { return (NCh+1); };

   int GetPageSize()
   {
      return XPageSizePts;
   }

   void CalculateXPageSize()
   {
       int p1024[] = {1,2,4,8,16,32,64,128};
       XPageSizePts = p1024[XPageSize1024]*1024;
   }

   double GetMulti() { double t; Multi.ToDouble(&t); return t;}
   double GetYMin() { double t; YMax.ToDouble(&t); return (-t);}
   double GetYMax() { double t; YMax.ToDouble(&t); return t;}
};

#pragma pack(1)
typedef struct _DataHeader
{
   char cmd;
   int size;
   // data param
   int MaxData;
   int Page1024;
   int PageStep;
   int NCh;
   //scale param
   float xmin;
   float xmax;
   float ymin;
   float ymax;
   char volt[20];
   char sec[20];
} DataHeader; //42 byte
#pragma pack()

class MyDecorFrame : public wxFrame
{
public:
   MyDecorFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

   ~MyDecorFrame() {delete Icon;}

   void OnCloseWindow(wxCloseEvent& event);
   void OnEraseBackground(wxEraseEvent& event){};
   void OnQuit(wxCommandEvent& event);
   void OnAbout(wxCommandEvent& event);
   void OnHelp(wxCommandEvent& event);

   void OnChannelCfg(wxCommandEvent& WXUNUSED(event));
   void OnFileInfo(wxCommandEvent& WXUNUSED(event));
   void OnHexView(wxCommandEvent& WXUNUSED(event));
   void OnMyDispatch(wxCommandEvent& event);

   void OnPrint(wxCommandEvent& WXUNUSED(event));
   void OnPrintSetup(wxCommandEvent& WXUNUSED(event));
   void OnPrintPreview(wxCommandEvent& WXUNUSED(event));
   void OnPageSetup(wxCommandEvent& WXUNUSED(event));
   
   wxHtmlHelpController& GetHtmlHelpController() { return m_msHtmlHelp; }

   void PopulateToolbar(wxToolBar* tb);
   void PopulateMenubar(wxMenuBar* mb);

public:
   wxIcon *Icon;
   MyFrame *frame;
   MyMiniFrame *mini_frame;
   MyInfoFrame *info_frame;
   MyHexFrame  *hex_frame;
   wxMenuBar *mb;
   wxToolBar *tb;
   wxComboBox *cb;
   wxStatusBar *sb;

private:
   wxHtmlHelpController     m_msHtmlHelp;

   DECLARE_EVENT_TABLE()
};



class MyThread : public wxThread
{
public:
   MyThread(MyFrame *parent, wxIPV4address address);
   virtual void *Entry();
   virtual void OnExit();
public:
   MyFrame *prnt;
   int m_Status;
   wxIPV4address addr;
   wxSocketClient *m_sock;
};


class MyFrame : public wxScrollWnd
{
public:
   MyFrame(wxWindow* parent);

   ~MyFrame()
   {
      delete pZ;
      delete[] ar;
      if(hFile.IsOpened()) hFile.Close(); 
      if(ConnActive) DataThread->Delete();
   }

   void OnPaint(wxPaintEvent& event);
   void OnEraseBackground(wxEraseEvent& event){};
   void OnMouseMove(wxMouseEvent& event);
   void OnLButtonDblClick(wxMouseEvent& event);
   void OnLButtonDown(wxMouseEvent& event);
   void OnRButtonDown(wxMouseEvent& event);

   void OnConfigure(wxCommandEvent& event);
   void OnChangeActiveCh(wxCommandEvent& event);
   void OnSwitchBW(wxCommandEvent& event);
   
   void ConfigureZoomer();
   void OnCenter(wxCommandEvent& event);
   void OnCloseFile(wxCommandEvent& event);
   void OnSaveFile(wxCommandEvent& event);
   void OnOpenFile(wxCommandEvent& event);
   void OnSaveCfg(wxCommandEvent& event);

public:

// socket interface to daq server

   void OnOpenConnection(wxCommandEvent& event);
   void OnCloseConnection(wxCommandEvent& event);
   void OnRepaint(wxCommandEvent& event);
   MyThread *DataThread;
   int ConnActive;
/////////////////////////////////

   MyDecorFrame *prnt;
   wxFile hFile;
   wxString FName;

   DATA_FILE_HDR hdr;

   int FStart;
   int  FileCP;
   int chOFFS[128];
   int prevOFFS[128];
   int* ar;
   wxZoomer *pZ;
   
   int TotalPages;
   
   int ActiveCh;

   wxString ProgramPath;
   
   ZoomScale ZS;
   ZoomParam ZP;

   ParamBuffer PBuf; // zoomer param buffer
   ParamBuffer OrigPBuf;

   bool BWMode;

private:
   DECLARE_EVENT_TABLE()
};

class MyPrintout: public wxPrintout
{
public:
   MyPrintout(wxFrame *parent, wxChar *title = _T("My printout")):wxPrintout(title) { prnt = (MyDecorFrame *)parent; }
   bool OnPrintPage(int page);
   bool HasPage(int page);
   bool OnBeginDocument(int startPage, int endPage);
   void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);

   void DrawPageOne(wxDC *dc);
   MyDecorFrame *prnt;
};


#endif
