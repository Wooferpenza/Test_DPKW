#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "mondrian.xpm"

#include "wx/sysopt.h"
#include "wx/toolbar.h"
//#include "wx/xrc/xmlres.h"          // XRC XML resouces
#include "wx/config.h"
#include "wx/dir.h"

#include "toolbox.h"
#include "min_info.h"
#include "hex_info.h"
#include "viewer.h"
#include "cfgdlg.h"

#define random(x) ((double)rand()/(double)RAND_MAX*x)
#define wxRGB(r,g,b) ((unsigned char)r|((unsigned short)((unsigned char)g<<8))|((unsigned long)((unsigned char)b<<16)) )

wxPrintData *g_printData = (wxPrintData*) NULL ;
wxPageSetupData* g_pageSetupData = (wxPageSetupData*) NULL;

DEFINE_EVENT_TYPE(wxEVT_MYFRAME)

DEFINE_EVENT_TYPE(wxEVT_PROCS)
/////////////////////////////////////////////////////

unsigned long rgb[128];

int *buffer;
int bsz;
wxMutex mutexBuffer;

BEGIN_EVENT_TABLE(MyDecorFrame, wxFrame)
   EVT_MENU(ID_QUIT,  MyDecorFrame::OnQuit)
   EVT_MENU(ID_ABOUT, MyDecorFrame::OnAbout)
   EVT_MENU(ID_HELP, MyDecorFrame::OnHelp)
   EVT_MENU(ID_OPEN_FILE, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_CLOSE_FILE, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_SAVE_FILE, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_CONFIG, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_SAVE_CFG, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_RESET_OFFS, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_INFO, MyDecorFrame::OnFileInfo)
   EVT_MENU(ID_HEX_VIEW, MyDecorFrame::OnHexView)
   EVT_MENU(ID_CH_CFG, MyDecorFrame::OnChannelCfg)

   EVT_MENU(ID_PRINT, MyDecorFrame::OnPrint)
   EVT_MENU(ID_PAGE_SETUP, MyDecorFrame::OnPageSetup)
   EVT_MENU(ID_PRINT_SETUP, MyDecorFrame::OnPrintSetup)
   EVT_MENU(ID_PRINT_PREVIEW, MyDecorFrame::OnPrintPreview)
   
   EVT_MENU(ID_SWITCH_BW, MyDecorFrame::OnMyDispatch)

   EVT_MENU(ID_DAQ_CONN, MyDecorFrame::OnMyDispatch)
   EVT_MENU(ID_DAQ_DCONN, MyDecorFrame::OnMyDispatch)

   EVT_COMBOBOX(ID_ACH, MyDecorFrame::OnMyDispatch)

   EVT_ERASE_BACKGROUND(MyDecorFrame::OnEraseBackground)
   EVT_CLOSE(MyDecorFrame::OnCloseWindow)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyFrame, wxScrollWnd)
   EVT_PAINT(MyFrame::OnPaint)
   EVT_ERASE_BACKGROUND(MyFrame::OnEraseBackground)
   EVT_LEFT_DOWN(MyFrame::OnLButtonDown)
   EVT_LEFT_DCLICK(MyFrame::OnLButtonDblClick)
   EVT_RIGHT_DOWN(MyFrame::OnRButtonDown)
   EVT_MOTION(MyFrame::OnMouseMove)

   EVT_COMMAND(ID_ACH, wxEVT_MYFRAME, MyFrame::OnChangeActiveCh)
   EVT_COMMAND(ID_CONFIG, wxEVT_MYFRAME, MyFrame::OnConfigure)
   EVT_COMMAND(ID_RESET_OFFS, wxEVT_MYFRAME, MyFrame::OnCenter)
   EVT_COMMAND(ID_SWITCH_BW, wxEVT_MYFRAME, MyFrame::OnSwitchBW)
   EVT_COMMAND(ID_CLOSE_FILE, wxEVT_MYFRAME, MyFrame::OnCloseFile)
   EVT_COMMAND(ID_SAVE_FILE, wxEVT_MYFRAME, MyFrame::OnSaveFile)
   EVT_COMMAND(ID_OPEN_FILE, wxEVT_MYFRAME, MyFrame::OnOpenFile)
   EVT_COMMAND(ID_SAVE_CFG, wxEVT_MYFRAME, MyFrame::OnSaveCfg)

   EVT_COMMAND(ID_DAQ_CONN, wxEVT_MYFRAME, MyFrame::OnOpenConnection)
   EVT_COMMAND(ID_DAQ_DCONN, wxEVT_MYFRAME, MyFrame::OnCloseConnection)

   EVT_COMMAND(ID_PROCS, wxEVT_PROCS, MyFrame::OnRepaint)

END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------
// frame constructor
MyDecorFrame::MyDecorFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
       : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
   Icon = new wxIcon(mondrian_xpm);
   SetIcon(*Icon);
   
   mb = new wxMenuBar;
   PopulateMenubar(mb);
   SetMenuBar(mb);

   tb = CreateToolBar(wxTB_FLAT, wxID_ANY);
   PopulateToolbar(tb);

//   Create the mini frame window
   frame = new MyFrame(this);

   mini_frame = new MyMiniFrame( this, -1, "Configure channels");//, wxPoint(100, 100),wxSize(220,100));
   mini_frame->Show(0);   
   
   info_frame = new MyInfoFrame( this, -1, "File/Data Information");//, wxPoint(100, 100),wxSize(220,100));
   info_frame->Show(0);


   hex_frame = new MyHexFrame( this, -1, "Table view");//, wxPoint(100, 100),wxSize(220,100));
   hex_frame->Show(0);   

   if(bOpenFile)
   {
      wxCommandEvent eventCustom(wxEVT_MYFRAME);
      eventCustom.SetId(ID_OPEN_FILE);
      eventCustom.SetInt(CMD_OPEN_FILE);
      eventCustom.SetString(argFileName);
      wxPostEvent(frame, eventCustom);
   }
   else
   {
      frame->ConfigureZoomer();
//       make test signal
      int nch = frame->PBuf.GetNCh();
      int ps  = frame->PBuf.GetPageSize();
      int DSize=nch*ps;
      for(int j=0;j<nch;++j)
         for( int i=j;i<DSize;i+=nch){ frame->ar[i]=int(1024*(j+1)*sin(2*3.1415926/(DSize-nch)*(i-j)));}
   }

   sb = CreateStatusBar(3);
   SetSizeHints(640,480);
}

// wnd constructor
MyFrame::MyFrame(wxWindow* parent)
       : wxScrollWnd(parent)
{
   prnt = (MyDecorFrame *)parent;
   ar = 0;
   FileCP=0;
   FStart = 0;
   ConnActive = 0;

   BWMode = 0;

   pZ = 0;

// read from registry - config values   
   wxConfig *config = new wxConfig("ViewerApp");

   PBuf.FontSize = config->Read("FontSize",0L);
   PBuf.NCh = config->Read("NCh",0L);
  
   PBuf.MaxData = config->Read("MaxData","8192");
   PBuf.MinData = config->Read("MinData","-8192");
  
   PBuf.YMin = config->Read("YMin","-32768.0");
   PBuf.YMax = config->Read("YMax","32768.0");
   PBuf.Multi = config->Read("Xmulti","1.0");
   PBuf.YText = config->Read("sbVolt","Code");
   PBuf.XText = config->Read("sbSec","Point");
   PBuf.XPageSize1024 = config->Read("Page1024",0L);

   PBuf.CalculateXPageSize();

   PBuf.dXMin = 0;
   PBuf.dXMax = PBuf.GetPageSize();

//   ProgramPath = config->Read("Path","");

   for(int i=0; i<128; i++)
   {
      char buf[20];
      sprintf(buf,"color%d",i);
      rgb[i] = config->Read(buf,(unsigned long)wxRGB(random(255),random(255),random(255)));
   }

   ActiveCh=0; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   delete config;

   prnt->mb->Enable(ID_DAQ_CONN,true);
   prnt->mb->Enable(ID_DAQ_DCONN,false);


   pZ = new wxZoomer();
}


//============================================================================
// DecorFrame event handlers
//============================================================================
void MyDecorFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
   Close(TRUE);
}

void MyDecorFrame::OnCloseWindow(wxCloseEvent& event)
{
   Destroy();
};

void MyDecorFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   wxMessageBox("Zommer 1.0b (c) 2008 L-Card (programmed by Pavel Chauzov).","About",wxICON_INFORMATION|wxOK,this);
}

void MyDecorFrame::OnHelp(wxCommandEvent& WXUNUSED(event))
{
   m_msHtmlHelp.DisplayContents();
}

void MyDecorFrame::OnChannelCfg(wxCommandEvent& WXUNUSED(event))
{
   mini_frame->Show(1);//!mini_frame->IsShown());
   mini_frame->Raise();
}

void MyDecorFrame::OnFileInfo(wxCommandEvent& WXUNUSED(event))
{
   info_frame->Show(1);//!info_frame->IsShown());
   info_frame->Raise();
}


void MyDecorFrame::OnHexView(wxCommandEvent& WXUNUSED(event))
{
   hex_frame->Show(1);//!hex_frame->IsShown());
   hex_frame->Raise();
}

void MyDecorFrame::OnPrintSetup(wxCommandEvent& WXUNUSED(event))
{
    wxPrintDialogData printDialogData(* g_printData);
    wxPrintDialog printerDialog(this, & printDialogData);
//    printerDialog.GetPrintDialogData().SetSetupDialog(TRUE);
    printerDialog.ShowModal();
    (*g_printData) = printerDialog.GetPrintDialogData().GetPrintData();
}


void MyDecorFrame::OnPageSetup(wxCommandEvent& WXUNUSED(event))
{
    (*g_pageSetupData) = * g_printData;
    wxPageSetupDialog pageSetupDialog(this, g_pageSetupData);
    pageSetupDialog.ShowModal();
    (*g_printData) = pageSetupDialog.GetPageSetupData().GetPrintData();
    (*g_pageSetupData) = pageSetupDialog.GetPageSetupData();
}


void MyDecorFrame::OnPrint(wxCommandEvent& WXUNUSED(event))
{
//    m_Prn -> PrintFile();
}

void MyDecorFrame::OnPrintPreview(wxCommandEvent& WXUNUSED(event))
{
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData printDialogData(* g_printData);
    wxPrintPreview *preview = new wxPrintPreview(new MyPrintout(this), new MyPrintout(this), &printDialogData);
    if (!preview->Ok())
    {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    wxPreviewFrame *pframe = new wxPreviewFrame(preview, this, _T("Print Preview"), wxPoint(100, 100), wxSize(600, 650));
    pframe->Centre(wxBOTH);
    pframe->Initialize();
    pframe->Show(TRUE);
}


void MyDecorFrame::OnMyDispatch(wxCommandEvent& event)
{
      event.SetEventType(wxEVT_MYFRAME);
      wxPostEvent(frame, event);
}


//============================================================================
// MyFrame methods
//============================================================================
void MyFrame::OnChangeActiveCh(wxCommandEvent& event)
{
   ActiveCh=event.GetSelection();
   SetScrollPosition(wxVERTICAL,(PBuf.GetMaxData()-1)-chOFFS[ActiveCh]);
   Refresh();
}

void MyFrame::OnConfigure(wxCommandEvent& WXUNUSED(event))
{
   ParamBuffer buf;
   buf=PBuf;
   wxConfigDlg *dlg = new wxConfigDlg(this,buf);
   dlg->ShowModal();
   PBuf=buf;
   PBuf.CalculateXPageSize();
   ConfigureZoomer();
   Refresh();
}

void MyFrame::OnSaveCfg(wxCommandEvent& event)
{
   wxConfig *config = new wxConfig("ViewerApp");

   config->Write("FontSize",PBuf.FontSize);
   config->Write("NCh",PBuf.NCh);
//   config->Write("ActiveCh",frame->pBuf.ActiveCh);
   config->Write("MaxData",PBuf.MaxData);
   config->Write("MinData",PBuf.MinData);
   config->Write("YMin",PBuf.YMin);
   config->Write("YMax",PBuf.YMax);
   config->Write("Xmulti",PBuf.Multi);
   config->Write("sbVolt",PBuf.YText);
   config->Write("sbSec",PBuf.XText);
   config->Write("Page1024",PBuf.XPageSize1024);

   for(int i=0; i<128; i++)
   {
      char buf[20];
      sprintf(buf,"color%d",i);
      config->Write(buf,(long)rgb[i]);
   }
   delete config;
}

void MyFrame::OnSwitchBW(wxCommandEvent& WXUNUSED(event))
{
   BWMode = !BWMode;
   ConfigureZoomer();
   Refresh();
}

void MyFrame::OnOpenFile(wxCommandEvent& event)
{
   if(hFile.IsOpened()) hFile.Close();

   if(event.GetInt()==CMD_OPEN_FILE) // handle cmd line open file...
   {
      FName = event.GetString();
   }
   else
   {
      wxFileDialog dialog(this, "Open data file", "", "", "data files (*.bin;*.dat)|*.bin;*.dat", wxOPEN);
      if (dialog.ShowModal() == wxID_OK)
      {
         FName = dialog.GetPath();
      }
      else return;
   }

   if(!hFile.Open(FName.c_str(), wxFile::read_write))
   {
      wxMessageBox("Error open file", "Error", wxOK|wxICON_ERROR, this);
      return;
   }
 
   memset(&hdr,0,sizeof(hdr));
   int *buf = new int[1024];
   hFile.Read(buf,1024*sizeof(int));
   memcpy(&hdr,buf,sizeof(hdr));
   delete[] buf;

   FStart = 4096;
   if(hdr.Signature != 0xAA55A5A5) 
   {
      memset(&hdr,0,sizeof(hdr));
      hdr.Signature = 0xAA55A5A5;
      FStart = 0;
   }

   ConfigureZoomer();
   MyDecorFrame *parent= (MyDecorFrame *)GetParent();
   parent->info_frame->SetListItems();

   return;
}

void MyFrame::OnCloseFile(wxCommandEvent& WXUNUSED(event))
{
   if(hFile.IsOpened()) hFile.Close();
}

void MyFrame::OnSaveFile(wxCommandEvent& WXUNUSED(event))
{

   wxFileDialog dialog(this, "Save data file", "", "",
                             "data files (*.bin;*.dat)|*.bin;*.dat|CSV files (*.csv)|*.csv|Picture files (*.bmp;*.jpg;*.png)|*.bmp;*.jpg;*.png",
                             wxSAVE|wxOVERWRITE_PROMPT);

   if (dialog.ShowModal() == wxID_OK)
   {
      wxString Ext = wxFileName(dialog.GetPath()).GetExt();
   
      int nch = PBuf.GetNCh();
      int ps = nch*PBuf.GetPageSize();

      if((Ext!="csv")&&(Ext!="bmp")&&(Ext!="jpg")&&(Ext!="png"))
      {
         wxFile hWriteFile;
         if(!hWriteFile.Create(dialog.GetPath().c_str(),true))
         {
            wxMessageBox("Error create file", "Error", wxOK|wxICON_ERROR, this);
            return;
         }
   
         if(Ext=="bin")
         {
            if(hdr.Signature != 0xAA55A5A5)
            {  
               memset(&hdr,0,sizeof(hdr));
               hdr.Signature = 0xAA55A5A5;
            }
            int *buf = new int[1024];
            memcpy(buf,&hdr,sizeof(hdr));
            hWriteFile.Write(buf,1024*sizeof(int));
            delete[] buf;
         };

         hWriteFile.Write(ar,ps*sizeof(int));
         hWriteFile.Close();
      }

      if(Ext=="csv")
      {
         int i,j;
         wxFile hWriteFile;
         if(!hWriteFile.Create(dialog.GetPath().c_str(),true))
         {
            wxMessageBox("Error create file", "Error", wxOK|wxICON_ERROR, this);
            return;
         }

         wxString str;
         for(i=0;i<ps;i+=nch)
         {
            for(j=0;j<nch;j++)
            {
               str.Clear();
               str << ar[i+j] << ((j==nch-1) ? "\n":",");
               hWriteFile.Write(str);
            }
         }
         hWriteFile.Close();
      }

      if((Ext=="bmp")||(Ext=="jpg")||(Ext=="png"))
      {
         wxRect r; r.x=0; r.y=0;
         GetClientSize((int*)&r.width,(int*)&r.height);

         wxBitmap *bmp = new wxBitmap(r.width,r.height);
         wxMemoryDC dc1;
         dc1.SelectObject(*(bmp));
         if(pZ) pZ->OnPaint(dc1,r,0);
         dc1.SelectObject(wxNullBitmap);

         wxBitmapType type = wxBITMAP_TYPE_BMP;
         if(Ext=="jpg") type = wxBITMAP_TYPE_JPEG;
         if(Ext=="png") type = wxBITMAP_TYPE_PNG;
         bmp->SaveFile(dialog.GetPath().c_str(),type);
         delete bmp;
      }
   }
   return;
}

void MyFrame::ConfigureZoomer()
{   
   MyDecorFrame *parent= (MyDecorFrame *)GetParent();

   int nch = PBuf.GetNCh();
   int pagesize = PBuf.GetPageSize();
   int maxdata = PBuf.GetMaxData();
   int mindata = PBuf.GetMinData();

   int ps = nch*pagesize;

   parent->cb->Clear();
   for(int i=0;i<nch;i++)
   {
      char str[10];
      //itoa(i,str,10);
      sprintf(str,"%d",i);
      parent->cb->Append(wxT(str));
   }
   parent->cb->SetSelection(0);

   parent->mini_frame->InitToolbar();

   TotalPages=1;
   
   if(hFile.IsOpened())
   {
      int sz = hFile.Length();
      TotalPages = (int)(sz/(ps*sizeof(int))+0.5);
      hFile.Seek(FStart);
      FileCP=-1; // current position in file
   }   
      
   if(ar) delete[] ar;
   ar = new int[ps];
   memset(ar,0,ps*sizeof(int));
   
   if(pZ) pZ->MakeFont(PBuf.GetFontSize(), wxFONTFAMILY_MODERN);

   if(BWMode)
   {
      ZP.Set(8,8,wxColour(255,255,255),wxColour(255,255,255),
                  wxColour(0,255,0),wxColour(192,192,192),1);
   }
   else
   {
      ZP.Set(8,8,wxSystemSettings::GetColour(wxSYS_COLOUR_MENU),wxColour(128,128,128),
              wxColour(0,255,0),wxColour(192,192,192),0);
   }
   
   if(pZ) pZ->SetParameters(30,&ZP);
    
   SetScrollbars(wxHORIZONTAL|wxVERTICAL,0,1,TotalPages,0,1,maxdata*2);
   SetScrollPosition(wxVERTICAL,maxdata-1);
   memset(chOFFS,0,128*sizeof(int));
   memset(prevOFFS,0,128*sizeof(int));
   parent->hex_frame->UpdateTableSize(nch,pagesize);
}

void MyFrame::OnCenter(wxCommandEvent& WXUNUSED(event))
{
   SetScrollPosition(wxVERTICAL,PBuf.GetMaxData()-1);
   memset(chOFFS,0,128*sizeof(int));
   memset(prevOFFS,0,128*sizeof(int));
   FileCP++; // типа для обновления как бы сместили позицию
   Refresh();
}

void MyFrame::OnPaint(wxPaintEvent& event)
{
unsigned long ret;
int i,j;
char obj;

   wxPaintDC dc(this);
   MyDecorFrame *parent = (MyDecorFrame *)GetParent();

   int nch = PBuf.GetNCh();
   int ps = nch*PBuf.GetPageSize();
   int maxdata = PBuf.GetMaxData();
   int mindata = PBuf.GetMinData();
   double multi = PBuf.GetMulti();
   double ymax = PBuf.GetYMax();   
   double ymin = PBuf.GetYMin();

// read scroll pos
   int posX = GetScrollPos(wxHORIZONTAL);
   int posY = (maxdata-1)-GetScrollPos(wxVERTICAL);

// get client size
   wxRect r; r.x=0; r.y=0;
   GetClientSize((int*)&r.width,(int*)&r.height);

   wxString buf;
   buf << "("<< posX << " " << posY << ")";
   //buf << "("<< r.width << " " << r.height << ")";
   parent->SetStatusText(buf);

   buf.Clear();
   buf << PBuf.YText << "/" << PBuf.XText;
   parent->SetStatusText(buf,1);
   
// read new data from file 
// horizontal slider
   if((hFile.IsOpened())&&(posX!=FileCP)) // switch pages
   {
      FileCP=posX;

      hFile.Seek(FStart+FileCP*ps*sizeof(int));
      ret = hFile.Read(ar,ps*sizeof(int));
   
      if(ret<ps*sizeof(int)) memset(&ar[ret/sizeof(int)],0,(ps*sizeof(int)-ret));

      for(j=0;j<nch;j++)
         for(i=0;i<ps;i+=nch) ar[i+j] = ar[j+i] +chOFFS[j]; // offest channels

      if(parent->hex_frame->IsShown()) parent->hex_frame->m_grid->ForceRefresh();

      PBuf.dXMin = ((FileCP*ps)/nch)*multi;
      PBuf.dXMax = (((FileCP+1)*ps)/nch)*multi;
   }

// vertical slider
   if(posY!=chOFFS[ActiveCh]) // change offset
   {
      chOFFS[ActiveCh] = posY;
      for(i=ActiveCh;i<ps;i+=nch) ar[i] = ar[i] +(chOFFS[ActiveCh]-prevOFFS[ActiveCh]);
      prevOFFS[ActiveCh] = chOFFS[ActiveCh];

      if(parent->hex_frame->IsShown()) parent->hex_frame->m_grid->ForceRefresh();
   }
/*   
   wxFFT *fft = new wxFFT();
   int *t = new int[8192];
   randomize();
   for(int j=0;j<8192;j++) t[j] = (8192.0+random(10))*sin(256*3.1415926*j/(8192));
   fft->SetSize(8192,2,0);
   fft->SetData(t,8192);
   fft->CalculateFFT();
   int ama = fft->GetAmp20Log(t);
   delete fft;
   md = ama;
   ymin=-ama;
   ymax=0;
*/
   ZS.Set(PBuf.dXMin,PBuf.dXMax,ymin,ymax,mindata,maxdata,14,"14.1f");
//   ZS.Set(PBuf.dXMin,PBuf.dXMax,ymin,ymax,0,md,14,"%-14.1f");

   if(pZ) pZ->SetDataI(ar,ps,nch,ActiveCh,&ZS,rgb);
   //if(pZ) pZ->SetDataI(a,ps,nch,ActiveCh,&ZS,NULL);

   //if(pZ) pZ->OnPaint((HDC)dc.GetHDC(),r,0);
   if(pZ) pZ->OnPaint(dc,r,0);
//   delete t;
}


void MyFrame::OnMouseMove(wxMouseEvent& event)
{
   wxRect r; r.x=0; r.y=0;
   GetClientSize((int*)&r.width,(int*)&r.height);
   if(pZ) pZ->OnMouseMove(r,event);
   Refresh();
}

void MyFrame::OnLButtonDblClick(wxMouseEvent& event)
{
   wxRect r; r.x=0; r.y=0;
   GetClientSize((int*)&r.width,(int*)&r.height);
   if(pZ) pZ->OnButtonDown(r,event,-1);
   Refresh();
}

void MyFrame::OnLButtonDown(wxMouseEvent& event)
{
   wxRect r; r.x=0; r.y=0;
   GetClientSize((int*)&r.width,(int*)&r.height);
   if(pZ) pZ->OnButtonDown(r,event,0);
   Refresh();
}

void MyFrame::OnRButtonDown(wxMouseEvent& event)
{
   wxRect r; r.x=0; r.y=0;
   GetClientSize((int*)&r.width,(int*)&r.height);
   if(pZ) pZ->OnButtonDown(r,event,0);
   Refresh();
}


///////////////////////////////////////////////////////////////////////////////
// MyPrintOut class - printing
//////////////////////////////////////////////////////////////////////////////
bool MyPrintout::OnPrintPage(int page)
{
   wxDC *dc = GetDC();
   if(dc)
   {
      if(page==1) DrawPageOne(dc);
        
      dc->SetDeviceOrigin(0, 0);
      dc->SetUserScale(1.0, 1.0);
        
      //wxChar buf[200];
      //wxSprintf(buf, wxT("PAGE %d"), page);
      // dc->DrawText(buf, 10, 10);
        
      return TRUE;
   }
   else
      return FALSE;
}

bool MyPrintout::OnBeginDocument(int startPage, int endPage)
{
   if(!wxPrintout::OnBeginDocument(startPage, endPage)) return FALSE;
   return TRUE;
}

void MyPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
   *minPage = 1;
   *maxPage = 1;
   *selPageFrom = 1;
   *selPageTo = 1;
}

bool MyPrintout::HasPage(int pageNum)
{
   return (pageNum == 1);
}

void MyPrintout::DrawPageOne(wxDC *dc)
{
    int w, h;

//    RECT r; r.top=0; r.left=0;
//    prnt->frame->GetClientSize((int*)&r.right,(int*)&r.bottom);

    wxRect r; r.x=0; r.y=0;
    prnt->frame->GetClientSize((int*)&r.width,(int*)&r.height);
    
    float maxX = r.GetRight();
    float maxY = r.GetBottom();

    // Let's have at least 50 device units margin
    float marginX = 20;
    float marginY = 20;
    
    // Add the margin to the graphic size
    maxX += (2*marginX);
    maxY += (2*marginY);
    
    // Get the size of the DC in pixels
    dc->GetSize(&w, &h);
    
    // Calculate a suitable scaling factor
    float scaleX=(float)(w/maxX);
    float scaleY=(float)(h/maxY);
    
    // Use x or y scaling factor, whichever fits on the DC
    float actualScale = wxMin(scaleX,scaleY);
    
    // Calculate the position on the DC for centring the graphic
    float posX = (float)((w - (r.GetRight()*actualScale))/2.0);
    float posY = (float)((h - (r.GetBottom()*actualScale))/2.0);
    
    // Set the scale and origin
    dc->SetUserScale(actualScale, actualScale);
    dc->SetDeviceOrigin( (long)posX, (long)posY );
 
    //prnt->frame->pZ->OnPaint((HDC)dc->GetHDC(),r,0);
    prnt->frame->pZ->OnPaint(*dc,r,0);

}


void MyFrame::OnOpenConnection(wxCommandEvent& WXUNUSED(event))
{
   wxIPV4address addr;
   char obj;

   long port;
   wxString def;

   def.Printf("%d",3000);
   while(1)
   {
      wxString data = wxGetTextFromUser("Port number","Enter port number",def,this);
      if(data.IsEmpty()) data=def;
      if(data.ToLong(&port)) break;
   }

   MyDecorFrame *parent = (MyDecorFrame *)GetParent();

   addr.Hostname("localhost");
   addr.Service(port);

   OrigPBuf = PBuf;
   if (hFile.IsOpened()) hFile.Close();

   parent->mb->Enable(ID_OPEN_FILE,false);
   parent->tb->EnableTool(ID_OPEN_FILE,false);
   parent->mb->Enable(ID_CONFIG,false);
   parent->tb->EnableTool(ID_CONFIG,false);
   parent->mb->Enable(ID_SAVE_CFG,false);
   parent->tb->EnableTool(ID_SAVE_CFG,false);

   DataThread = new MyThread(this, addr);
   if(DataThread->m_Status==0)
   {
      parent->SetStatusText("Failed alloc thread...",2);
      delete DataThread;
      return;
   }

   if(DataThread->Create()!=wxTHREAD_NO_ERROR)
   {
      parent->SetStatusText("Failed create thread...",2); return;
   }

   DataThread->Run();

   parent->SetStatusText("Connect to DAQ server - OK!",2);

   ConnActive = 1;
   parent->mb->Enable(ID_DAQ_CONN,false);
   parent->mb->Enable(ID_DAQ_DCONN,true);
}

void MyFrame::OnCloseConnection(wxCommandEvent& WXUNUSED(event))
{
   MyDecorFrame *parent = (MyDecorFrame *)GetParent();

   DataThread->Delete();

   parent->SetStatusText("DAQ server disconnected!",2);

   ConnActive = 0;
   parent->mb->Enable(ID_DAQ_CONN,true);
   parent->mb->Enable(ID_DAQ_DCONN,false);

   parent->mb->Enable(ID_OPEN_FILE,true);
   parent->tb->EnableTool(ID_OPEN_FILE,true);
   parent->mb->Enable(ID_CONFIG,true);
   parent->tb->EnableTool(ID_CONFIG,true);
   parent->mb->Enable(ID_SAVE_CFG,true);
   parent->tb->EnableTool(ID_SAVE_CFG,true);

   PBuf = OrigPBuf; // restore orig config
   ConfigureZoomer();

   Refresh();
}


void MyFrame::OnRepaint(wxCommandEvent& event)
{
   MyDecorFrame *parent = (MyDecorFrame *)GetParent();
   int i,j;
   if(event.GetInt()==ID_PROCS_CFG) ConfigureZoomer();
   if(event.GetInt()==ID_PROCS_LOST) if(parent->sb) parent->sb->SetStatusText("Error!!! Socket lost!",2);

   wxMutexLocker lock(mutexBuffer);
   int nch = PBuf.GetNCh();
   int ps = nch*PBuf.GetPageSize();
   if(buffer) for(j=0;j<nch;j++) for(i=0;i<ps;i+=nch) ar[i+j] = buffer[i+j] + chOFFS[j]; // offest channels         
   Refresh();
}

MyThread::MyThread(MyFrame *parent, wxIPV4address address)
        : wxThread()
{
   m_Status = 1;
   addr = address;
   prnt = parent;
   bsz = 128000;
   buffer=new int[bsz];
   if(buffer==0) {m_Status = 0; return;}
   

   m_sock = new wxSocketClient(wxSOCKET_BLOCK | wxSOCKET_WAITALL);
   if(m_sock==0) {m_Status = 0; return;}

   m_sock->Connect(addr, false);
   if(m_sock->WaitOnConnect(10) && m_sock->IsConnected()) return;
   m_sock->Destroy();
   m_sock=0;
   delete[] buffer;
   buffer = 0;
   m_Status = 0;
}

void MyThread::OnExit()
{
   m_sock->Destroy();
   m_sock = 0;
   wxMutexLocker lock(mutexBuffer);
   delete[] buffer;
   buffer =0;
}

void *MyThread::Entry()
{
   int i,j;
   unsigned char obj;
   DataHeader data_hdr;
   int nch;
   int ps;

   wxCommandEvent eventCustom(wxEVT_PROCS,ID_PROCS);   

   m_sock->WriteMsg(&obj,1); // start collect

   while (1)
   {
      if(TestDestroy()) return NULL;

      eventCustom.SetInt(ID_PROCS_PNT);

      m_sock->ReadMsg(&data_hdr,sizeof(DataHeader));
      if(m_sock->Error()) { eventCustom.SetInt(102); break; }

      mutexBuffer.Lock();

      if(data_hdr.size > bsz)
      {
         delete[] buffer;
         bsz = data_hdr.size;
         buffer = new int[bsz];
      }

      m_sock->ReadMsg(buffer,data_hdr.size*sizeof(int));

      if(m_sock->Error()) { eventCustom.SetInt(102); break; }
      
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! for plugin buffer size !!!!!!!!!!!!
      if(data_hdr.cmd)
      {
         prnt->PBuf.MaxData.Printf("%d",data_hdr.MaxData);// = MaxData;
         prnt->PBuf.MinData.Printf("%d", -(data_hdr.MaxData+1));
         prnt->PBuf.XPageSizePts = data_hdr.Page1024; // real value of points !!!!!!!!!!!!!!!!!!1
         prnt->PBuf.NCh = data_hdr.NCh-1;
         prnt->PBuf.YText = wxString(data_hdr.volt);
         prnt->PBuf.XText = wxString(data_hdr.sec);
         prnt->PBuf.dXMin = data_hdr.xmin;
         prnt->PBuf.dXMax = data_hdr.xmax;
         prnt->PBuf.YMin.Printf("%lf",data_hdr.ymin);
         prnt->PBuf.YMax.Printf("%lf",data_hdr.ymax);

         eventCustom.SetInt(ID_PROCS_CFG);
      }

      mutexBuffer.Unlock();

      m_sock->WriteMsg(&obj,1); // start next transfer
      if(m_sock->Error()) { eventCustom.SetInt(ID_PROCS_LOST); break; }

      wxPostEvent(prnt, eventCustom);
      wxThread::Sleep(20);
   }

   wxPostEvent(prnt, eventCustom);
   while(!TestDestroy()) wxThread::Sleep(20);

   return NULL;
}
