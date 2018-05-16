#ifndef __DSKDLG__
#define __DSKDLG__

#include "../include/ioctl.h"
#include "../include/ifc_ldev.h"
//#include "../include/create.h"
#include "data_hdr.h"

#include "wx/toolbar.h"
//#include "wx/xrc/xmlres.h"          // XRC XML resouces
#include "wx/dynlib.h"
#include "wx/socket.h"
#include "wx/minifram.h"
#include "wx/listctrl.h"

#define  SETBIT(Var, Bit) (Var |= (0x1L << Bit))
#define  CLEARBIT(Var, Bit) (Var &= ~(0x1L << Bit))  
#define  TOGGLEBIT(Var, Bit) (Var ^= (0x1L << Bit))  
#define  CHECKBIT(Var, Bit) (Var & (0x1L << Bit))
#define  CLEARVAR(Var) (Var = 0)

#define SEL_SLOT 2
#define SEL_FSIZE 1
#define SEL_FILE 0
#define START_ENA 0x07

struct LogChan
{
   int Num;
   int Mode;
   int KU; 
};

// Define a new mini frame

class DskListCtrl;


#pragma pack(1)
typedef struct _DataHeader
{
// data param
char cmd;
int size;
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

class DskFrame: public wxFrame
{
public:
   DskFrame(wxFrame *parent, wxWindowID id = wxID_ANY, const wxString& title = "wxToolBar Sample",
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize );
   ~DskFrame(){ };

   void OnCloseWindow(wxCloseEvent& event) { Destroy(); };
   void OnSize(wxSizeEvent& event);
   void OnEraseBackground(wxEraseEvent& event) {};

   void StartCollect(wxCommandEvent& WXUNUSED(event) );
   void StopCollect(wxCommandEvent& WXUNUSED(event) );

   int m_Number;
   wxString Path;
   wxPanel *m_panel;
   DskListCtrl *m_listCtrl;
   wxTextCtrl *m_logWindow;
   void PopulateToolbar(wxToolBarBase* tb);
   wxToolBarBase *toolBar;
private:

DECLARE_EVENT_TABLE()
};

class MyThread : public wxThread
{
public:
   MyThread(DskListCtrl *parent);
   virtual void *Entry();
   virtual void OnExit();
public:
   DskListCtrl *lst;
   int *a; // data buffer for transfer
   int *ta;
   int *ama;
   int *sk;
   wxFFT *fft;
   ADC_PAR tap;
   int FFTmode;

};


class DskListCtrl: public wxListCtrl
{
public:
   DskListCtrl(wxWindow *parent, const wxWindowID id, 
                const wxPoint& pos, const wxSize& size, long style);
   ~DskListCtrl();
   void EnumeratePCI();
   void SelectSlot();

   void StartCollection(int Flag);

   void SetParameters();
   void SetListItems();        
   void OnActivate(wxListEvent& event);
   wxString SelectFile();
   void SetFileSize();
   void SetRate(double *var);
   void SetKadr(double *var);
   void SetSynchroType(ULONG *var);
   void SetSynchroSensitivity(ULONG *var);
   void SetSynchroMode(ULONG *var);
   void SetLogChannel(ULONG *chn);
   void SetAdPorog(SHORT *porog);
   void SetNCh(ULONG *nch);
   void SetBufferSize(ULONG *pages);
   void SetPageSize(ULONG *irq, ULONG *fifo);
   wxString ShowChannel(ULONG ch, LogChan *buf);

   void SetYMax(double *var);
   void SetYText(wxString *var);

   void SetDataMode(int *mode);

   int SetDataHeader();

public:
   DskFrame *prnt;
   DataHeader data_hdr;

   wxString GlobalStr;

   wxSocketBase *sock;
   MyThread *DataThread;

   void OnSocketEvent(wxSocketEvent& event);
   void OnServerEvent(wxSocketEvent& event);

   wxSocketServer *m_server;

   wxDynamicLibrary *hComp;

   //HINSTANCE hComp;

   IDaqLDevice* pI;
   HANDLE hThread;
   HANDLE hEvent;
   ULONG Tid;
   ULONG slot;
   ULONG nSlot;

   int FFTmode;

   USHORT *ADCdata;
   ULONG *ADCsync;
      
   PLATA_DESCR_U2 pd;
   
   USHORT IrqStep;//777-777%7; // половинка буфера кратная числу каналов
   USHORT FIFO;         //
   USHORT pages;

   ULONG CanStart;
   
   ADC_PAR ap;
   ULONG PCPages; // per channel pages
   wxString YText;
   double YMax;

private:
   wxListItemAttr m_attr;
   DECLARE_EVENT_TABLE()
};

#endif
