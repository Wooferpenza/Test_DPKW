#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/toolbar.h"
#include "wx/colordlg.h"
#include "toolbox.h"
#include "min_info.h"
#include "hex_info.h"
#include "viewer.h"
#include <math.h>

#define wxRGB(r,g,b) ((unsigned char)r|((unsigned short)((unsigned char)g<<8))|((unsigned long)((unsigned char)b<<16)) )
extern unsigned long rgb[128];

BEGIN_EVENT_TABLE(MyMiniFrame, wxMiniFrame)
    EVT_CLOSE  (MyMiniFrame::OnCloseWindow)
    EVT_TOOL_RCLICKED(-1,MyMiniFrame::OnRToolClick)
    EVT_TOOL_RANGE(10,137,MyMiniFrame::OnToolClicked)
    EVT_ERASE_BACKGROUND(MyMiniFrame::OnEraseBackground)    
    EVT_CHAR_HOOK(MyHexFrame::OnEsc)
END_EVENT_TABLE()


MyMiniFrame::MyMiniFrame(wxFrame* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size )
        :wxMiniFrame(parent, id, title, pos, size, wxSYSTEM_MENU|wxTINY_CAPTION_HORIZ|wxFRAME_NO_TASKBAR|wxCLOSE_BOX)
{
   wxMemoryDC dc;
   char str[10];    
   int i;
   for(i=0;i<128;i++)
   {
      toolBarBitmaps[i] = new wxBitmap(18,18);
      dc.SelectObject(*(toolBarBitmaps[i]));

      wxFont font(8,wxDEFAULT,wxNORMAL,wxNORMAL);
      dc.SetFont(font);
        
      dc.SetBrush(*wxLIGHT_GREY_BRUSH);
      dc.SetPen(*wxLIGHT_GREY_PEN);
      dc.DrawRectangle(0,0,18,18);        
        
      int blue = (rgb[i]>>16)&0xFF;
      int green = (rgb[i]>>8)&0xFF;
      int red = rgb[i]&0xFF;
        
      wxColour color((char)red,(char)green,(char)blue);
      dc.SetBrush(wxBrush(color,wxSOLID));
      dc.SetPen(wxPen(color,1,wxSOLID));
      dc.DrawRectangle(0,12,18,18);
        
      sprintf(str,"%d",i);
      dc.DrawText(str,0,0);
      dc.SelectObject(wxNullBitmap);
   }
   
   ChCfgMode = 0;    
   InitToolbar();
}

void MyMiniFrame::OnRToolClick(wxCommandEvent& event)
{
   wxString msg;
   ChCfgMode =!ChCfgMode;
   if(ChCfgMode) msg = wxString("Channel configuration mode.");
   else msg = wxString("Channel selection mode.");
   SetTitle(msg);
   InitToolbar();
}

void MyMiniFrame::OnToolClicked(wxCommandEvent& event)
{
int ti = event.GetId()-10;
   if(ChCfgMode)
   {
      wxColour col;

      wxColourData data;
      data.SetChooseFull(TRUE);
        
      for (int i = 0; i < 16; i++)
      {
         wxColour colour(i*16, i*16, i*16);
         data.SetCustomColour(i, colour);
      }

      wxColourDialog dialog(this, &data);
      dialog.SetTitle("Choose the channel colour");
      if (dialog.ShowModal() == wxID_OK)
      {
         wxColourData retData = dialog.GetColourData();
         col = retData.GetColour();
         rgb[ti] = (unsigned long)wxRGB(col.Red(),col.Green(),col.Blue());
            
         // repaint tool
         wxMemoryDC dc;
         dc.SelectObject(*(toolBarBitmaps[ti]));

         char blue = _B(rgb[ti]);
         char green = _G(rgb[ti]);
         char red = _R(rgb[ti]);
        
         wxColour color(red,green,blue);
         dc.SetBrush(wxBrush(color,wxSOLID));
         dc.SetPen(wxPen(color,1,wxSOLID));
         dc.DrawRectangle(0,12,18,18);
         dc.SelectObject(wxNullBitmap);
         InitToolbar();
      }
   }
   else 
   {
      long mask=0x10000000;
      rgb[ti]=(rgb[ti]&mask) ? (rgb[ti]&(~mask)):(rgb[ti]|mask);
   }
   ((MyDecorFrame *)GetParent())->frame->Refresh();
}

bool MyMiniFrame::InitToolbar()
{
   MyDecorFrame *parent = (MyDecorFrame *)GetParent();
   int NCh= parent->frame->PBuf.GetNCh();
   // delete and recreate the toolbar
   wxToolBarBase *toolBar = GetToolBar();
   long style = wxTB_HORIZONTAL;
   delete toolBar;
   SetToolBar(NULL);
   toolBar = CreateToolBar(style, 501);

   toolBar->SetToolBitmapSize(wxSize(18,18));
    
   bool smode = (ChCfgMode) ? false:true;
   int i;
   long mask=0x10000000;
   for(i=0;i<NCh;i++)
   {
      toolBar->AddTool(10+i, *(toolBarBitmaps[i]), wxNullBitmap, smode);
      if(rgb[i]&mask) toolBar->ToggleTool(10+i, true);
   }
   toolBar->Realize();
   toolBar->SetRows((NCh+7)/8); // 16
   SetClientSize(25*8,0);   
return TRUE;
}

void MyMiniFrame::OnEsc(wxKeyEvent& event)
{
   if(WXK_ESCAPE==event.GetKeyCode()) { Show(0); return; }
   event.Skip();
}
