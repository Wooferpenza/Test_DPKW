#ifndef __SCRLWND__
#define __SCRLWND__

#define wxScrolledWindowStyle (wxHSCROLL | wxVSCROLL)

class wxScrollWnd : public wxWindow
{
public:
   wxScrollWnd();
   wxScrollWnd(wxWindow* parent,
             wxWindowID id = -1,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxScrolledWindowStyle,
             const wxString& name = wxPanelNameStr)
   {
      Create(parent, id, pos, size, style, name);
   }
//    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
   ~wxScrollWnd();

   bool Create(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxScrolledWindowStyle,
               const wxString& name = wxPanelNameStr);
   
   void SetScrollbars(int scrlollers, int startX, int pageX, int endX,
                                      int startY, int pageY, int endY);

   void OnScroll(wxScrollWinEvent& event);
   int  CalcScrollInc(wxScrollWinEvent& event);
   void SetScrollPosition(int orient, int pos);

protected:
    int m_xScrollPosition,m_yScrollPosition;
private:
    DECLARE_EVENT_TABLE()
};

#endif
