#ifndef __TOOLBOX__
#define __TOOLBOX__

#include "wx/minifram.h"
// Define a new mini frame

class MyMiniFrame: public wxMiniFrame
{
public:
   MyMiniFrame(wxFrame *parent, wxWindowID id = -1, const wxString& title = "wxToolBar Sample",
      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
   ~MyMiniFrame()
   {
      // Can delete the bitmaps since they're reference counted
      int i;
      for (i = 0; i < 128; i++) delete toolBarBitmaps[i];
   };
   void OnCloseWindow(wxCloseEvent& event)
   {
      Show(0);
   };
   void OnRToolClick(wxCommandEvent& event);
   void OnToolClicked(wxCommandEvent& event);
   void OnEraseBackground(wxEraseEvent& event) {};
   void OnEsc(wxKeyEvent& event);
//    void OnRightClick(int toolId, float x, float y);
   bool InitToolbar();
private:
   int ChCfgMode;
   wxBitmap* toolBarBitmaps[128];
DECLARE_EVENT_TABLE()
};
#endif
