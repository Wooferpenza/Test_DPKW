#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/listctrl.h"

#include "min_info.h"
#include "toolbox.h"
#include "hex_info.h"
#include "viewer.h"
#include <math.h>

BEGIN_EVENT_TABLE(MyInfoFrame, wxMiniFrame)
    EVT_CLOSE  (MyInfoFrame::OnCloseWindow)
    EVT_SIZE(MyInfoFrame::OnSize)
    EVT_ERASE_BACKGROUND(MyInfoFrame::OnEraseBackground)    
    EVT_CHAR_HOOK(MyInfoFrame::OnEsc)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyListCtrl, wxListCtrl)
//   EVT_LIST_ITEM_ACTIVATED()
   EVT_LIST_ITEM_RIGHT_CLICK(1000,MyListCtrl::OnActivate)
END_EVENT_TABLE()

MyInfoFrame::MyInfoFrame(wxFrame* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size )
        :wxMiniFrame(parent, id, title, pos, size,wxSYSTEM_MENU|wxTINY_CAPTION_HORIZ|wxFRAME_NO_TASKBAR|wxCLOSE_BOX)
{
    prnt = (MyDecorFrame *)parent;
    m_panel = new wxPanel(this);
    
    m_logWindow = new wxTextCtrl(m_panel, -1, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE | wxTE_READONLY | wxSUNKEN_BORDER);

    m_listCtrl = new MyListCtrl(m_panel, 8300/*id*/,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT | wxLC_HRULES |
                                    wxSUNKEN_BORDER);
                               
    
   SetListItems();
    
   SetClientSize(300,300);
}

void MyInfoFrame::SetListItems()
{
//MyDecorFrame *prnt = (MyDecorFrame *)GetParent();
wxString buf;
int i=0;
long tmp;
   if(prnt->frame->hFile.IsOpened())
   {
      // to speed up inserting we hide the control temporarily
      
      m_listCtrl->Hide();
      m_listCtrl->ClearAll();
      m_listCtrl->InsertColumn(0, wxT("Parameter"));
      m_listCtrl->InsertColumn(1, wxT("Value"),wxLIST_FORMAT_LEFT,200);
      
      buf.Printf(wxT("File name"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf = wxFileName(prnt->frame->FName).GetFullName();
      m_listCtrl->SetItem(i++, 1, buf);
      
      buf.Printf(wxT("File size"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf.Printf(wxT("%d bytes"), prnt->frame->hFile.Length());
      m_listCtrl->SetItem(i++, 1, buf);
      
      buf.Printf(wxT("Board name"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf.Printf(wxT("%s"),prnt->frame->hdr.BoardName);
      m_listCtrl->SetItem(i++, 1, buf);
      
      buf.Printf(wxT("ADC rate"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf.Printf(wxT("%-10.3f kHz"),prnt->frame->hdr.dRate);
      m_listCtrl->SetItem(i++, 1, buf);
      
      buf.Printf(wxT("ADC kadr"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf.Printf(wxT("%-10.3f ms"),prnt->frame->hdr.dKadr);
      m_listCtrl->SetItem(i++, 1, buf);
      
      buf.Printf(wxT("ADC scale"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf.Printf(wxT("%-10.3f"),prnt->frame->hdr.dScale);
      m_listCtrl->SetItem(i++, 1, buf);
            
      buf.Printf(wxT("Data channels"));
      tmp = m_listCtrl->InsertItem(i, buf);
      buf.Printf(wxT("%-d"),prnt->frame->hdr.NCh);
      m_listCtrl->SetItem(i++, 1, buf);
      
      for(int j=0;j<prnt->frame->hdr.NCh;j++)
      {
         buf.Printf(wxT("   channel %d"),j);
         tmp = m_listCtrl->InsertItem(i, buf);
         buf.Printf(wxT("%-d"),prnt->frame->hdr.Chn[j]);
         m_listCtrl->SetItem(i++, 1, buf);
      }
      
      // ...
      m_listCtrl->Show();
      m_logWindow->Clear();
      m_logWindow->AppendText(prnt->frame->hdr.Comment);
   }
}

void MyInfoFrame::OnSize(wxSizeEvent& event)
{
    wxSize size = GetClientSize();
    wxCoord y = (2*size.y)/3;
    m_listCtrl->SetSize(0, 0, size.x, y);
    m_logWindow->SetSize(0, y + 1, size.x, size.y - y);
    event.Skip();
}

void MyListCtrl::OnActivate(wxListEvent& event)
{
///   ::MessageBox(0,"","",MB_OK);
}

void MyInfoFrame::OnEsc(wxKeyEvent& event)
{
   if(WXK_ESCAPE==event.GetKeyCode()) { Show(0); return; }
   event.Skip();
}
