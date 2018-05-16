#ifndef __MIN_INFO__
#define __MIN_INFO__

#include "wx/minifram.h"
#include "wx/listctrl.h"
#include "data_hdr.h"


// Define a new mini frame
class MyListCtrl;
class MyDecorFrame;

class MyInfoFrame: public wxMiniFrame
{
public:
    MyInfoFrame(wxFrame *parent, wxWindowID id = -1, const wxString& title = "wxToolBar Sample",
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize );
    ~MyInfoFrame()
    {
       //delete m_listCtrl;
      //delete m_logWindow;
      //delete m_panel;
    };
    void OnCloseWindow(wxCloseEvent& event)
    {
      Show(0);
    };
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event) {};
    void OnEsc(wxKeyEvent& event);    

    void SetListItems();
    
    wxPanel *m_panel;
    MyListCtrl *m_listCtrl;
    wxTextCtrl *m_logWindow;
    MyDecorFrame *prnt;

private:

DECLARE_EVENT_TABLE()
};

class MyListCtrl: public wxListCtrl
{
public:
    MyListCtrl(wxWindow *parent,
               const wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               long style)
        : wxListCtrl(parent, id, pos, size, style),
          m_attr(*wxBLUE, *wxLIGHT_GREY, wxNullFont)
        {
        }
      void OnActivate(wxListEvent& event);
private:
   wxListItemAttr m_attr;
   DECLARE_EVENT_TABLE()
};


#endif
