#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "hex_info.h"
#include "min_info.h"
#include "toolbox.h"
#include "viewer.h"
#include <math.h>

BEGIN_EVENT_TABLE(MyHexFrame, wxMiniFrame)
    EVT_CLOSE(MyHexFrame::OnCloseWindow)
    EVT_SIZE(MyHexFrame::OnSize)
    EVT_ERASE_BACKGROUND(MyHexFrame::OnEraseBackground)    
    EVT_CHAR_HOOK(MyHexFrame::OnEsc)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyPanel, wxPanel)
    EVT_RADIOBOX(8200, MyPanel::OnRadio)
END_EVENT_TABLE()

const wxString view[] = {"HEX", "DEC"};

wxString BigGridTable::GetValue( int row, int col )
{
   int nch = prnt->frame->PBuf.GetNCh();
   return wxString::Format(view_filter, prnt->frame->ar[row*nch+col]);
}

wxString BigGridTable::GetRowLabelValue( int row)
{ 
   int off = prnt->frame->GetScrollPos(wxHORIZONTAL) * prnt->frame->PBuf.GetPageSize();
   return wxString::Format("%ld",off+row); 
}


void BigGridTable::SetFilter(int f)
{
   switch(f)
   {
      case 0: view_filter = wxString("%08X"); break;
      case 1: view_filter = wxString("%10ld");
   }
}

MyHexFrame::MyHexFrame(wxFrame* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size )
        :wxMiniFrame(parent, id, title, pos, size,wxSYSTEM_MENU|wxTHICK_FRAME|wxTINY_CAPTION_HORIZ|wxFRAME_NO_TASKBAR|wxCLOSE_BOX)
{
   m_panel = new MyPanel(this);
   m_rb = new wxRadioBox(m_panel,8200,"",wxDefaultPosition, wxDefaultSize,2,view,1,wxRA_SPECIFY_ROWS);
   m_rb->SetSelection(0);
   m_grid = new wxGrid(m_panel, wxID_ANY);
   m_table = new BigGridTable(parent,1,10);
   m_grid->SetTable(m_table, true);
   m_grid->SetEditable(FALSE);

   SetClientSize(300,500);
}

void MyHexFrame::OnChangeView(wxCommandEvent &event)
{
   int sel = m_rb->GetSelection();
   m_grid->BeginBatch();
   m_table->SetFilter(sel);
   m_grid->EndBatch();
}

void MyHexFrame::UpdateTableSize(int col, int row)
{
   m_grid->BeginBatch();
   m_grid->DeleteCols(0,m_grid->GetNumberCols());
   m_grid->DeleteRows(0,m_grid->GetNumberRows());
   m_grid->AppendCols(col);
   m_grid->AppendRows(row);
   m_grid->EndBatch();
}

void MyHexFrame::OnSize(wxSizeEvent& event)
{
    wxSize size = GetClientSize();
    m_grid->SetSize(0, 50, size.x, size.y-50);
    event.Skip();
}

void MyHexFrame::OnEsc(wxKeyEvent& event)
{
   if(WXK_ESCAPE==event.GetKeyCode()) { Show(0); return; }
   event.Skip();
}
