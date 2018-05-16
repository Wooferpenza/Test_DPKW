#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef LCOMP_LINUX
   #define INITGUID
#endif


#include "../include/stubs.h"


#include "wxfft.h"
#include "osc_dlg.h"
#include "setchan.h"
#include <math.h>


#include "start_bmp_0.xpm"
#include "stop_bmp_1.xpm"


typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);
CREATEFUNCPTR CreateInstance;


enum CmdEnum {SELECT_CMD, DMODE_CMD, SIZE_CMD, PSIZE_CMD,
              TIME_CMD, RATE_CMD, KADR_CMD, SYNC_CMD,
              SYNCT_CMD, SYNCS_CMD, SYNCM_CMD,
              SYNCCH_CMD, SYNCP_CMD, YGFG_CMD, YMAX_CMD, YTEXT_CMD, CHAN_CMD, CHANI_CMD
             }; // 0 - 16

const wxString Parameter[] = {_T("Selected board"),       _T("none"), _T("0"), _T("0"),
                              _T("Select mode"),          _T("Oscil"),_T("0"), _T("0"),
                              _T("Size (pages per chan)"),_T(""),     _T("0"), _T("0"),
                              _T("Page size(points)"),    _T(""),     _T("0"), _T("0"),
                              _T("Time parameters"),      _T(""),     _T("0"), _T("0"),
                              _T("   Rate"),              _T(""),     _T("0"), _T("1"),
                              _T("   Kadr"),              _T(""),     _T("0"), _T("1"),
                              _T("Synchronization"),      _T(""),     _T("0"), _T("0"),
                              _T("   Type"),              _T(""),     _T("0"), _T("1"),
                              _T("   Sensetivity"),       _T(""),     _T("0"), _T("1"),
                              _T("   Mode"),              _T(""),     _T("0"), _T("1"),
                              _T("   Channel"),           _T(""),     _T("0"), _T("1"),
                              _T("   Level"),             _T(""),     _T("0"), _T("1"),
                              _T("Y Axis config"),        _T(""),     _T("0"), _T("0"),
                              _T("   Y max value"),       _T("5"),    _T("0"), _T("1"),
                              _T("   Y text"),            _T("V"),    _T("0"), _T("1"),
                              _T("Channels"),             _T(""),     _T("0"), _T("0") // 0-black 1-blue / 0-normal 1-italic
                           };

//enum Commands

const wxString DataMode[] = {"Oscil", "FFT (rectangle)", "FFT (Hanning)", "FFT (Hamming)", "FFT (flat top)",
                             "FFT (Blackman)", "FFT (Exact Blackman)", "FFT (Blackman-Harris 4 term)",
                             "FFT (Blackman-Harris 7 term)"};
const wxString SynchroType[] = {"Digital start", "Digital kadr", "Analog","None"};
const wxString SynchroSensitivity[] = {"Level", "Edge", "Not used"};
const wxString SynchroMode[] = {"Up Level/Rising Edge", "Down Level/Falling Edge", "Not used"};

const wxString PagesList[] = {"1","2","4","8","16","32"};
const int PagesListVal[]= {1,2,4,8,16,32};
const int MaxPageIdx = 6;

const wxString SizeList[] = {"128","256","512","1024"};
const int SizeListVal[]= {128,256,512,1024};
const int MaxSizeIdx = 4;

wxString ChNum[32]; // fill in constructor 0-31 digits
wxString KU[] = {"5.0V", "1.25V", "0.3125V", "0.078V", 
                 "5.0V", "2.5V", "1.25V", "0.625V"}; // +4 for L783

wxString Mode[] = {"Diff", "Mono", "GND"};
int ModeCode[] = {0,2,1,1};


wxString BoardsInst[20];
int      BoardsSlot[20];

int sport; // socket port global var....

wxMutex mutexSync;
wxMutex mutexSelect;
int ThreadStop, ThreadRestart;

MyThread::MyThread(DskListCtrl *parent)
        : wxThread()
{
   lst = parent;
}

void MyThread::OnExit()
{
}

void *MyThread::Entry()
{
int i,j,k;
char c;
int wait_cnt;
ULONG tm;

int destroy = 0;

   while(!ThreadRestart) if(TestDestroy()) return NULL; // запустились и ждем выбора слота

   mutexSync.Lock();
   ThreadRestart = 0;
   ThreadStop = 0;
   mutexSync.Unlock();

   while(1)
   {
      if(TestDestroy()) break;

      mutexSelect.Lock();

      lst->SetDataHeader();

      lst->data_hdr.cmd = 1; // for start/restart thread configure zoomer
      lst->data_hdr.size = lst->ap.t1.Pages*lst->ap.t1.IrqStep;

      tap.t1 = lst->ap.t1;
      FFTmode = lst->FFTmode;
      if(FFTmode) tap.t1.Pages = lst->ap.t1.Pages *2;  // удвоить собираемые данные для ффт...

      lst->pI->FillDAQparameters(&tap.t1);
      lst->pI->SetParametersStream(&tap.t1, &tm, (void**)&lst->ADCdata, (void**)&lst->ADCsync,L_STREAM_ADC);
      lst->pI->EnableCorrection();

      a = new int[tm]; // с запасом в большинстве случаев...
      if(FFTmode)
      {
         ta = new int[tm]; // трансформированный а
         ama = new int[tap.t1.NCh];
         sk = new int[tap.t1.NCh];
         fft = new wxFFT();
      }

      wait_cnt = tap.t1.Pages*tap.t1.IrqStep;

      while(1)
      {
         if(TestDestroy()) { destroy = 1; break; }
         
         mutexSync.Lock();
         if(ThreadRestart) {ThreadRestart=0; mutexSync.Unlock(); break; }
         if(ThreadStop) { mutexSync.Unlock(); break; } 
         mutexSync.Unlock();

         lst->pI->InitStartLDevice();
         lst->pI->StartLDevice();

         while(*lst->ADCsync < wait_cnt)
         {
            if(TestDestroy()){ destroy = 1; break; }
            wxThread::Sleep(20);
         }

         lst->pI->StopLDevice();
         if(destroy) break;
      
         if(lst->sock)
         {
            /////////////////////////////////////////////////
            if(FFTmode)
            {
               int nch = tap.t1.NCh;
      
               for(i=0,k=0;i<nch;i++)
                  for(j=0,sk[i]=k;j<lst->data_hdr.size*2;j+=nch) ta[k++] = (short)lst->ADCdata[i+j];

               // FFT transform
               int szc = lst->data_hdr.size*2/nch;
               //      wxFFT *fft = new wxFFT();
               for(i=0;i<nch;i++)
               {
                  fft->SetSize(szc,(FFTmode-1),0);
                  fft->SetData(&ta[sk[i]],szc);
                  fft->CalculateFFT();
                  ama[i] = fft->GetAmp20Log(&ta[sk[i]]);
               }

               for(i=0;i<nch;i++)
                  for(j=0,k=sk[i];j<lst->data_hdr.size;j+=nch) a[i+j] = ta[k++]-ama[i];

               //delete fft;
            }
            else
            {
               for(i=0;i<lst->data_hdr.size;i++) a[i] = (short)lst->ADCdata[i];
            }
/////////////////////////////////////////////
            lst->sock->ReadMsg(&c, 1);
            lst->sock->WriteMsg(&lst->data_hdr, sizeof(DataHeader));
            lst->data_hdr.cmd = 0; // configure off
            lst->sock->WriteMsg(a,lst->data_hdr.size*sizeof(int));
         }
      }

      delete[] a;
      if(FFTmode)
      {
         delete[] ta;
         delete[] ama;
         delete[] sk;
         delete fft;
      }

      mutexSelect.Unlock();

      if(destroy) break;

      mutexSync.Lock();
      if(ThreadStop)
      {
         ThreadStop = 0;
         mutexSync.Unlock();
         while(1)                   // on stop go to idle cycle with sleep and wait destroy or restart
         {
            if(TestDestroy()) return NULL;
            if(ThreadRestart) break;
            wxThread::Sleep(20);
         }
         mutexSync.Lock();
         ThreadRestart=0;
      }
      mutexSync.Unlock();
   }

   return NULL;
}


enum {
      LISTCTRL_ID = (wxID_HIGHEST+100),
      START_ID,
      STOP_ID,
      SERVER_ID,
      SOCKET_ID
     };

BEGIN_EVENT_TABLE(DskFrame, wxMiniFrame)
    EVT_CLOSE  (DskFrame::OnCloseWindow)
    EVT_SIZE   (DskFrame::OnSize)
    EVT_MENU(START_ID, DskFrame::StartCollect)
    EVT_MENU(STOP_ID, DskFrame::StopCollect)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(DskListCtrl, wxListCtrl)
   EVT_LIST_ITEM_RIGHT_CLICK (LISTCTRL_ID,DskListCtrl::OnActivate)
   EVT_SOCKET                (SERVER_ID,DskListCtrl::OnServerEvent)
   EVT_SOCKET                (SOCKET_ID,DskListCtrl::OnSocketEvent)
END_EVENT_TABLE()


void DskFrame::StartCollect(wxCommandEvent& WXUNUSED(event) )
{
   m_listCtrl->StartCollection(1);
}

void DskFrame::StopCollect(wxCommandEvent& WXUNUSED(event) )
{
   m_listCtrl->StartCollection(0);
}

void DskFrame::PopulateToolbar(wxToolBarBase* tb)
{
   wxBitmap tb_bmp[2];
   tb_bmp[0] = wxBitmap(start_bmp_0);
   tb_bmp[1] = wxBitmap(stop_bmp_1);
   int w = tb_bmp[0].GetWidth(),
       h = tb_bmp[0].GetHeight();
   tb->SetToolBitmapSize(wxSize(w, h));
   tb->AddTool(START_ID, _T("Start"), tb_bmp[0], _T("Start"));
   tb->AddTool(STOP_ID, _T("Stop"), tb_bmp[1], _T("Stop"));
   tb->Realize();
}


DskFrame::DskFrame(wxFrame* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size )
        :wxFrame(parent, id, title, pos, size, wxTHICK_FRAME | wxSYSTEM_MENU| wxCLOSE_BOX | wxTINY_CAPTION_HORIZ)
{
   m_panel = new wxPanel(this, wxID_ANY);
    
   m_logWindow = new wxTextCtrl(m_panel, wxID_ANY, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxSUNKEN_BORDER);

   m_listCtrl = new DskListCtrl(m_panel, LISTCTRL_ID,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_HRULES |
                                wxSUNKEN_BORDER);

   m_listCtrl->SetListItems();
   m_listCtrl->prnt = this;

   for(int i=0;i<32;i++) ChNum[i] << i; // init chan numbers

   *m_logWindow << "* DAQ server *\n";
   *m_logWindow << "Version 1.0b \n";
   *m_logWindow << "Copyright(c) 2008 L-Card (Pavel Chauzov).\n";

   *m_logWindow << (m_listCtrl->GlobalStr);

   toolBar = CreateToolBar(0, wxID_ANY);
   PopulateToolbar(toolBar);
}

void DskFrame::OnSize(wxSizeEvent& event)
{
   wxSize size = GetClientSize();
   wxCoord y = (4*size.y)/5;
   m_listCtrl->SetSize(0, 0, size.x, y);
   m_logWindow->SetSize(0, y + 1, size.x, size.y - y);
   event.Skip();
}


void DskListCtrl::EnumeratePCI()
{
   int   i = 0;
   SLOT_PAR slPar;
   IDaqLDevice* tpI;   
   nSlot = 0;
   BoardsInst[nSlot] = "No boards";
   while(1)
   {
      LUnknown* pIUnknown = CreateInstance(i); // slot 0
      if(pIUnknown == NULL) break;

      HRESULT hr = pIUnknown->QueryInterface(IID_ILDEV,(void**)&tpI);
      if(hr!=S_OK) break;
      pIUnknown->Release();

      tpI->OpenLDevice();
      tpI->GetSlotParam(&slPar);
      if((slPar.BoardType == PCIA)||(slPar.BoardType == PCIB)||(slPar.BoardType == PCIC))
      {
         tpI->ReadPlataDescr(&pd); // fill up properties
         BoardsSlot[nSlot] = i;
         BoardsInst[nSlot].Printf("Slot - %d %s S/N %s",i,pd.t1.BrdName,pd.t1.SerNum); 
         nSlot++;
      }
      tpI->CloseLDevice();
      tpI->Release();
      i++;
   }
   if(i==0) nSlot=1;
}

DskListCtrl::DskListCtrl(wxWindow *parent, const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
           : wxListCtrl(parent, id, pos, size, style),  m_attr(*wxBLUE, *wxLIGHT_GREY, wxNullFont)
{
   hComp = 0;
   pI = 0;
   IrqStep = 1024;
   FIFO = 1024;
   pages = 2048;
   DataThread=0;
   sock=0;
   FFTmode=0;

   CLEARVAR(CanStart);

   // загрузим либу

   //hComp = CallCreateInstance("lcomp.dll");
   //hComp = LLoadLibrary("lcomp.dll");
   hComp = new wxDynamicLibrary();
   hComp->Load(hComp->CanonicalizeName("lcomp"));
   if(hComp->IsLoaded()) CreateInstance = (CREATEFUNCPTR) hComp->GetSymbol("CreateInstance");//LGetProcAddress(hComp, "CreateInstance");
   else CreateInstance = NULL;
   if(CreateInstance==NULL) { GlobalStr << " Could not load library !\n\n"; return; }
   
   EnumeratePCI();

   memset(&ap,0,sizeof(ap));
   ap.t1.s_Type = L_ADC_PARAM;
   ap.t1.AutoInit = 0;
   ap.t1.dRate = 100.0;
   ap.t1.dKadr = 0.0;
   ap.t1.dScale = 0;
   
   ap.t1.SynchroType = 3;
   ap.t1.SynchroSensitivity = 2;
   ap.t1.SynchroMode = 2;
   ap.t1.AdPorog = 0;
   ap.t1.NCh = 1;

   ap.t1.FIFO = FIFO;
   ap.t1.IrqStep = IrqStep;
   ap.t1.Pages = 1;
   ap.t1.IrqEna = 1;
   ap.t1.AdcEna = 1;

   PCPages = 1; // per channel
   YMax = 5;
   YText.Printf("V");

//////////// start sock server
   wxIPV4address addr;
   addr.Service(sport);

   m_server = new wxSocketServer(addr,wxSOCKET_BLOCK | wxSOCKET_WAITALL);

   if (! m_server->Ok()) { GlobalStr << " Could not listen at the specified port !\n\n"; return; }
   else { GlobalStr << " Server listening. \n Port : " << sport << "\n"; }

   m_server->SetEventHandler(*this, SERVER_ID);
   m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
   m_server->Notify(true);

   ThreadStop = ThreadRestart = 0;

   DataThread = new MyThread(this);
   if(DataThread->Create()!=wxTHREAD_NO_ERROR)
   {
      wxMessageBox("Failed create thread", "Error", wxOK | wxICON_ERROR, this); return;
   }
   DataThread->Run();
}

DskListCtrl::~DskListCtrl()
{
   DataThread->Delete();
   if(pI) { pI->CloseLDevice(); pI->Release(); pI=0; }
   if(hComp->IsLoaded()) {hComp->Unload(); delete hComp; }//LFreeLibrary(hComp);
   if(sock) { sock->Destroy(); sock=0; }
   delete m_server;
}

void DskListCtrl::SelectSlot()
{
   if(CHECKBIT(CanStart,SEL_SLOT)) StartCollection(0); // остановили тред

   mutexSelect.Lock();

   if(pI) { pI->CloseLDevice(); pI->Release(); pI = 0; }
   
   CLEARBIT(CanStart,SEL_SLOT);

   int data;
   data = wxGetSingleChoiceIndex("Select board","Select slot to work",nSlot,BoardsInst,this);
   if(data!=-1) slot = BoardsSlot[data];
   else { return; }

   
   LUnknown* pIUnknown = CreateInstance(slot);
   if(pIUnknown == NULL) { return; }
   HRESULT hr = pIUnknown->QueryInterface(IID_ILDEV,(void**)&pI);
   if(hr!=S_OK) { return; }
   pIUnknown->Release();
   
   pI->OpenLDevice();
   pI->ReadPlataDescr(&pd); // fill up properties
   wxString FBios = wxString(pd.t1.BrdName);
   pI->LoadBios((char *)FBios.c_str());
   //pI->LoadBios(pd.t1.BrdName);

//   ASYNC_PAR sp; sp.s_Type = L_ASYNC_DAC_OUT;  sp.Mode = 0; sp.Data[0] = 2047;
//   pI->IoAsync(&sp);
/*
   USHORT *data1;
   ULONG   *sync1;
   
   DAC_PAR dacPar;

   dacPar.t1.s_Type = L_DAC_PARAM;
   dacPar.t1.AutoInit=1;
   dacPar.t1.dRate=100.0;
   dacPar.t1.FIFO=512;
   dacPar.t1.IrqStep=512;
   dacPar.t1.Pages=2;
   dacPar.t1.IrqEna=0;
   dacPar.t1.DacEna=1;
   dacPar.t1.DacNumber=0;
   pI->FillDAQparameters(&dacPar.t1);

   ULONG db=20000;
   pI->RequestBufferStream(&db, L_STREAM_DAC);
   pI->SetParametersStream(&dacPar.t1,&db, (void **)&data1, (void **)&sync1,L_STREAM_DAC);
   for(int i=0;i<1024;i++) data1[i]=((USHORT)(1024.0*sin((2.0*(3.1415*i)/1024.0)))&0xFFF)|0x0000;
*/

   ULONG tm = IrqStep*pages;
   pI->RequestBufferStream(&tm,L_STREAM_ADC);
   
   SetItem(SELECT_CMD,1,BoardsInst[data]);   
   
   SETBIT(CanStart,SEL_SLOT);
   SETBIT(CanStart,SEL_FSIZE);
   SETBIT(CanStart,SEL_FILE);

   mutexSelect.Unlock();
}

void DskListCtrl::SetListItems()
{
wxString buf;
wxListItem li;
int j;
      
      Hide();
      ClearAll();
      InsertColumn(0, _T("Parameter"),wxLIST_FORMAT_LEFT,130);
      InsertColumn(1, _T("Value"),wxLIST_FORMAT_LEFT,160);
      
      for(j=0;j<CHANI_CMD;j++)
      {
         InsertItem(j, Parameter[4*j]);
         SetItem(j, 1, Parameter[4*j+1]);
         li.Clear();
         li.SetId(j);
         if(Parameter[4*j+2]=="1")    li.SetTextColour(*wxBLUE);
         if(Parameter[4*j+3]=="1")    li.SetFont(*wxITALIC_FONT);;
         SetItem(li);
      }
      
      for(j=0;j<32;j++)
      {
         buf.Clear(); buf << "   " << j;
         InsertItem(CHANI_CMD+j, buf);
      }   
      
      SetParameters();
      Show();
}

void DskListCtrl::OnActivate(wxListEvent& event)
{
   long idx = event.GetIndex();
   
   if(idx==SELECT_CMD) SelectSlot();
   if(idx==DMODE_CMD) SetDataMode(&FFTmode);
   if(idx==SIZE_CMD) SetBufferSize(&PCPages);
   if(idx==PSIZE_CMD) SetPageSize(&ap.t1.IrqStep,&ap.t1.FIFO);
   if(idx==RATE_CMD) SetRate(&ap.t1.dRate);
   if(idx==KADR_CMD) SetKadr(&ap.t1.dKadr);
   if(idx==SYNCT_CMD)
   {
      SetSynchroType(&ap.t1.SynchroType);
      switch(ap.t1.SynchroType)
      {
         case 0: ap.t1.SynchroMode = 2; ap.t1.SynchroSensitivity = 2; break;
         case 1: ap.t1.SynchroMode = 2; ap.t1.SynchroSensitivity = 2; break;
         case 2: ap.t1.SynchroMode = 0; ap.t1.SynchroSensitivity = 0; break;
         case 3: ap.t1.SynchroMode = 2; ap.t1.SynchroSensitivity = 2;
      }
    
   }
   if(idx==SYNCS_CMD) SetSynchroSensitivity(&ap.t1.SynchroSensitivity);
   if(idx==SYNCM_CMD) SetSynchroMode(&ap.t1.SynchroMode);
   if(idx==SYNCCH_CMD) SetLogChannel(&ap.t1.AdChannel);
   if(idx==SYNCP_CMD) SetAdPorog((SHORT *)&ap.t1.AdPorog);

   if(idx==YMAX_CMD) SetYMax(&YMax);
   if(idx==YTEXT_CMD) SetYText(&YText);

   if(idx==CHAN_CMD) SetNCh(&ap.t1.NCh);
   if((idx>=CHANI_CMD)&&(idx<CHANI_CMD+32)) SetLogChannel(&ap.t1.Chn[idx-CHANI_CMD]);
   
   SetParameters();
}

void DskListCtrl::SetParameters()
{
   int i;
   wxString str;
   wxListItem li;      
   li.SetFont(*wxITALIC_FONT);

   ap.t1.Pages = PCPages*ap.t1.NCh; // correct to meet viewer

   if(pI) pI->FillDAQparameters(&ap.t1);

   str.Clear(); str << ap.t1.dRate << " kHz"; SetItem(RATE_CMD,1,str);
   str.Clear(); str << ap.t1.dKadr << " ms"; SetItem(KADR_CMD,1,str);
   str.Clear(); str << SynchroType[ap.t1.SynchroType]; SetItem(SYNCT_CMD,1,str); 
   str.Clear(); str << SynchroSensitivity[ap.t1.SynchroSensitivity]; SetItem(SYNCS_CMD,1,str); 
   str.Clear(); str << SynchroMode[ap.t1.SynchroMode]; SetItem(SYNCM_CMD,1,str);

   str.Clear(); str << DataMode[FFTmode]; SetItem(DMODE_CMD,1,str);

   str.Clear(); str << YMax; SetItem(YMAX_CMD,1,str);
   str.Clear(); str << YText; SetItem(YTEXT_CMD,1,str);
   
   str.Clear(); str << ShowChannel(ap.t1.AdChannel,NULL); SetItem(SYNCCH_CMD,1,str);  // channel
   str.Clear(); str << short(ap.t1.AdPorog); SetItem(SYNCP_CMD,1,str);  // porog
   str.Clear(); str << (ap.t1.NCh); SetItem(CHAN_CMD,1,str);  // NCh
   str.Clear(); str << (PCPages); SetItem(SIZE_CMD,1,str);  // Pages
   str.Clear(); str << (ap.t1.IrqStep); SetItem(PSIZE_CMD,1,str);  // Page size
   for(i=0; i<32; i++)
   { 
      str.Clear(); str << ShowChannel(ap.t1.Chn[i],NULL); SetItem(CHANI_CMD+i,1,str);
      if(i<ap.t1.NCh) li.SetTextColour(*wxBLACK);
      else li.SetTextColour(*wxLIGHT_GREY);
      li.SetId(CHANI_CMD+i); SetItem(li);
   }
}

void DskListCtrl::StartCollection(int Flag)
{
   if(CanStart==START_ENA)
   {
      mutexSync.Lock();
      if(Flag) ThreadRestart = 1;
      else ThreadStop = 1;
      mutexSync.Unlock();

   }
   else
   {
      if(Flag)
      {
//      if(!(CanStart&0x01)) wxMessageBox("Unable to start - select file name first.", "Error", wxOK | wxICON_ERROR, this);
//      if(!(CanStart&0x02)) wxMessageBox("Unable to start - select file size first.", "Error", wxOK | wxICON_ERROR, this);
         if(!CHECKBIT(CanStart,SEL_SLOT)) wxMessageBox("Unable to start - select board first.", "Error", wxOK | wxICON_ERROR, this);
      }
   }
}

void DskListCtrl::SetYMax(double *var)
{
   wxString data;
   wxString def;
   def.Printf("%-.3f",*var);
   while(1)
   {
      data = wxGetTextFromUser("Y Max value","Enter Y Max value",def,this);
      if(data.IsEmpty()) data=def;
      if(data.ToDouble(var)) return;
   }   
}


void DskListCtrl::SetYText(wxString *var)
{

   wxString def;
   def = *var;
   *var = wxGetTextFromUser("Y Axis text","Enter text",def,this);
}

void DskListCtrl::SetRate(double *var)
{
   wxString data;
   wxString def;
   def.Printf("%-.3f",*var);
   while(1)
   {
      data = wxGetTextFromUser("ADC rate (kHz)","Enter ADC rate",def,this);
      if(data.IsEmpty()) data=def;
      if(data.ToDouble(var)) return;
   }   
}

void DskListCtrl::SetKadr(double *var)
{
   wxString data;
   wxString def;
   def.Printf("%-.3f",*var);
   while(1)
   {
      data = wxGetTextFromUser("ADC kadr (ms)","Enter kadr interval",def,this);
      if(data.IsEmpty()) data=def;
      if(data.ToDouble(var)) return;
   }   
}

void DskListCtrl::SetSynchroType(ULONG *var)
{
   int data;
   data = wxGetSingleChoiceIndex("Synchro type","Enter synchro type",4,SynchroType,this);
   if(data!=-1) *var = (ULONG)data;
}

void DskListCtrl::SetSynchroSensitivity(ULONG *var)
{
   int data;
   data = wxGetSingleChoiceIndex("Synchro sensitivity","Enter synchro sensitivity",2,SynchroSensitivity,this);
   if(data!=-1) *var = (ULONG)data;
}

void DskListCtrl::SetSynchroMode(ULONG *var)
{
   int data;
   data = wxGetSingleChoiceIndex("Synchro mode","Enter synchro mode",2,SynchroMode,this);
   if(data!=-1) *var = (ULONG)data;
}

wxString DskListCtrl::ShowChannel(ULONG ch, LogChan *buf)
{
   wxString str;
   
   LogChan d;

   USHORT mask = (ch&0x20) ? 0x1F : 0x0F;

   d.Mode = ModeCode[((ch & 0x30)>>4)];
   d.Num = (ch&mask);
   d.KU = ((ch & 0xC0)>>6);

   if(buf) *buf=d;

   str.Clear();
   if(!strcmp(pd.t1.BrdName,"L783")) { d.KU+=4; }
   str << "" << d.Num << ":" << "" <<Mode[d.Mode] << ":" << "" << KU[d.KU];
   return str;
}

void DskListCtrl::SetLogChannel(ULONG *chn)
{
   LONG Num,Mode,KU;
   wxString def;
   LogChan buf;
   def = ShowChannel(*chn, &buf);
   wxSetChanDlg *dlg = new wxSetChanDlg(this,buf,&pd);
   dlg->ShowModal();
   *chn = 0;
   switch(buf.Mode)
   {
      case 0: // diff
      {
         *chn = (buf.Num&0xF)|(buf.KU<<6);
      } break;
      case 1: // mono
      {
         *chn = (buf.Num)|(1<<5)|(buf.KU<<6);  
      } break;
      case 2: // gnd
      {
         *chn = (1<<4)|(buf.KU<<6);  
      }
   }
}

void DskListCtrl::SetAdPorog(SHORT *porog)
{
long p;
int MaxData;
   wxString def;
   def.Printf("%d",*porog);

   while(1)
   {
      wxString data = wxGetTextFromUser("Synchro level (code)","Enter code",def,this);
      if(data.IsEmpty()) data=def;
      if(data.ToLong(&p)) break;
   }
   *porog = (short)p;
   MaxData = 2048;
   if(strcmp(pd.t1.BrdName,"L783")) MaxData = 8192;  // 8192;

   if(*porog>MaxData-1) *porog=MaxData-1;
   if(*porog<(-MaxData)) *porog=-MaxData;
}

void DskListCtrl::SetNCh(ULONG *nch)
{
long n;
   wxString def;
   def.Printf("%d",*nch);
   while(1)
   {
      wxString data = wxGetTextFromUser("Number of channels","Enter number",def,this);
      if(data.IsEmpty()) data=def;
      if(data.ToLong(&n)) break;
   }
   *nch = n;
   if(*nch<1) *nch=1;
   if(*nch>32) *nch=32;
}

void DskListCtrl::SetBufferSize(ULONG *pages)
{
   int data;
   data = wxGetSingleChoiceIndex("Number of pages","Select number",MaxPageIdx,PagesList,this);
   if(data!=-1) *pages = PagesListVal[data];
}

void DskListCtrl::SetPageSize(ULONG *irq, ULONG *fifo)
{
   int data;
   data = wxGetSingleChoiceIndex("Page size","Select size",MaxSizeIdx,SizeList,this);
   if(data!=-1) *irq = *fifo = SizeListVal[data];
}

void DskListCtrl::SetDataMode(int *mode)
{
   int data;
   data = wxGetSingleChoiceIndex("Data mode (Oscil/FFT)","Select mode",9,DataMode,this);
   if(data!=-1) *mode = data;
   if(data) { YMax=150; YText.Printf("dB"); }
   else { YMax=5; YText.Printf("V");}
}



///////////////////////////////////////////////////////////////////////////////////////////////

int DskListCtrl::SetDataHeader()
{
int i;
   
   if(FFTmode) { data_hdr.MaxData = 150; }
   else
   {
      data_hdr.MaxData = 2048;
      if(strcmp(pd.t1.BrdName,"L783")) data_hdr.MaxData = 8192;
   }

   data_hdr.Page1024 = ap.t1.IrqStep*PCPages; //abs value

   data_hdr.NCh = ap.t1.NCh;

   data_hdr.xmin=0;

   if(FFTmode)
   {
      data_hdr.xmax=0.5/(ap.t1.dKadr+(ap.t1.NCh-1)/ap.t1.dRate);
      sprintf(data_hdr.sec,"kHz");
      if(data_hdr.xmax>1000) { data_hdr.xmax = data_hdr.xmax/1000; sprintf(data_hdr.sec,"MHz");}
      if(data_hdr.xmax<1) { data_hdr.xmax = data_hdr.xmax*1000; sprintf(data_hdr.sec,"Hz");}

   }
   else
   {
      data_hdr.xmax=(ap.t1.Pages*ap.t1.IrqStep/ap.t1.NCh)*ap.t1.dKadr;
      sprintf(data_hdr.sec,"ms");
      if(data_hdr.xmax>1000) { data_hdr.xmax = data_hdr.xmax/1000; sprintf(data_hdr.sec,"s");}
      if(data_hdr.xmax<1) { data_hdr.xmax = data_hdr.xmax*1000; sprintf(data_hdr.sec,"mks");}
   }

   data_hdr.ymin=-YMax;
   data_hdr.ymax=YMax;
   sprintf(data_hdr.volt,YText);
   return 1;
}

/////////////////////////////////////////////////////
// server func
/////////////////////////////////////////////////////
void DskListCtrl::OnServerEvent(wxSocketEvent& event)
{
  wxString s = _("OnServerEvent: ");

  switch(event.GetSocketEvent())
  {
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n")); break;
    default                  : s.Append(_("Unexpected event !\n")); break;
  }

  prnt->m_logWindow->AppendText(s);

  sock = m_server->Accept(false);

  if (sock)
  {
    prnt->m_logWindow->AppendText(_("New client connection accepted\n\n"));
  }
  else
  {
    prnt->m_logWindow->AppendText(_("Error: couldn't accept a new connection\n\n"));
    return;
  }

  sock->SetEventHandler(*this, SOCKET_ID);
  sock->SetNotify(/*wxSOCKET_INPUT_FLAG | */wxSOCKET_LOST_FLAG);
  sock->Notify(true);
}

void DskListCtrl::OnSocketEvent(wxSocketEvent& event)
{
  wxString s = _("OnSocketEvent: ");
  wxSocketBase *sock = event.GetSocket();

  // First, print a message
  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT : s.Append(_("wxSOCKET_INPUT\n")); break;
    case wxSOCKET_LOST  : s.Append(_("wxSOCKET_LOST\n")); break;
    default             : s.Append(_("Unexpected event !\n")); break;
  }

  prnt->m_logWindow->AppendText(s);

  // Now we process the event

  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT:
    {
      // We disable input events, so that the test doesn't trigger
      // wxSocketEvent again.
      sock->SetNotify(wxSOCKET_LOST_FLAG);

      // Which test are we going to run?
      unsigned char c;
      sock->Read(&c, 1);
//      wxMutexLocker lock(mutex);
      SetDataHeader();
      sock->Write(&data_hdr, sizeof(DataHeader));
      sock->Write(ADCdata,ap.t1.Pages*ap.t1.IrqStep*sizeof(short));

      // Enable input events again.
      sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
      break;
    }
    case wxSOCKET_LOST:
    {
      prnt->m_logWindow->AppendText(_("Deleting socket.\n\n"));
      break;
    }
    default: ;
  }
}
