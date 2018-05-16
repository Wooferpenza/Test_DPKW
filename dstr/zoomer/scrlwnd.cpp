#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "scrlwnd.h"


BEGIN_EVENT_TABLE(wxScrollWnd, wxWindow)
    EVT_SCROLLWIN(wxScrollWnd::OnScroll)
END_EVENT_TABLE()


wxScrollWnd::wxScrollWnd()
{
   m_xScrollPosition = 0;
   m_yScrollPosition = 0;
}

bool wxScrollWnd::Create(wxWindow *parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
{
   m_xScrollPosition = 0;
   m_yScrollPosition = 0;

   bool ok = wxWindow::Create(parent, id, pos, size, style|wxFULL_REPAINT_ON_RESIZE , name);

   return ok;
}

wxScrollWnd::~wxScrollWnd()
{
}

void wxScrollWnd::SetScrollbars(int scrollers, int startX, int pageX, int endX,
                                   int startY, int pageY, int endY)
{
   if(scrollers&wxHORIZONTAL) SetScrollbar(wxHORIZONTAL,startX,pageX,endX);
   if(scrollers&wxVERTICAL) SetScrollbar(wxVERTICAL,startY,pageY,endY);
}


void wxScrollWnd::SetScrollPosition(int orient, int pos)
{
    if (orient == wxHORIZONTAL)
    {
	m_xScrollPosition=pos;
        SetScrollPos(wxHORIZONTAL, pos, TRUE );
    }
    else
    {
	m_yScrollPosition=pos;
        SetScrollPos(wxVERTICAL, pos, TRUE );
    }
}



void wxScrollWnd::OnScroll(wxScrollWinEvent& event)
{
    int newPos;
    int orient = event.GetOrientation();
    
//    m_xScrollPosition = GetScrollPos(wxHORIZONTAL);
//    m_yScrollPosition = GetScrollPos(wxVERTICAL);


    int nScrollInc = CalcScrollInc(event);
    if (nScrollInc == 0) return;

    if (orient == wxHORIZONTAL)
    {
        newPos = m_xScrollPosition + nScrollInc;
        SetScrollPos(wxHORIZONTAL, newPos, TRUE );
    }
    else
    {
        newPos = m_yScrollPosition + nScrollInc;
        SetScrollPos(wxVERTICAL, newPos, TRUE );
    }
    

    if (orient == wxHORIZONTAL)
    {
        m_xScrollPosition += nScrollInc;
    }
    else
    {
        m_yScrollPosition += nScrollInc;
    }
    
    Refresh();
}

int wxScrollWnd::CalcScrollInc(wxScrollWinEvent& event)
{
    int pos = event.GetPosition();  // send when track thumb
    int orient = event.GetOrientation();

    int nScrollInc = 0;

    int ScrollRangeCorrectedH = GetScrollRange(wxHORIZONTAL) - GetScrollThumb(wxHORIZONTAL);
    int ScrollRangeCorrectedV = GetScrollRange(wxVERTICAL) - GetScrollThumb(wxVERTICAL);
    int ScrollPosH = GetScrollPos(wxHORIZONTAL);
    int ScrollPosV = GetScrollPos(wxVERTICAL);
    int ThumbSizeH = GetScrollThumb(wxHORIZONTAL);
    int ThumbSizeV = GetScrollThumb(wxVERTICAL);
    WXTYPE et = event.GetEventType();


        if(event.GetEventType() == wxEVT_SCROLLWIN_TOP)
        {
            if (orient == wxHORIZONTAL)  nScrollInc = - m_xScrollPosition;
            else nScrollInc = - m_yScrollPosition;
        }
	else
        if(event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
        {
            if (orient == wxHORIZONTAL) nScrollInc = ScrollRangeCorrectedH - m_xScrollPosition;
            else nScrollInc = ScrollRangeCorrectedV - m_yScrollPosition;
        }
	else
        if(event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            if(orient == wxHORIZONTAL)
            {
               if(ScrollPosH>0) nScrollInc = -1;
            }
            else
            {
               if(ScrollPosV>0) nScrollInc = -1;
            }
        }
	else
        if(event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            if(orient == wxHORIZONTAL)
            {
               if(ScrollPosH<ScrollRangeCorrectedH) nScrollInc = 1;
            }
            else
            {
               if(ScrollPosV<ScrollRangeCorrectedV) nScrollInc = 1;
            }
        }
	else
        if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            if (orient == wxHORIZONTAL)
            {
               if(ScrollPosH>0) nScrollInc = -ThumbSizeH;
               if(ScrollPosH<ThumbSizeH) nScrollInc = -ScrollPosH;
            }
            else
            {
               if(ScrollPosV>0) nScrollInc = -ThumbSizeV;
               if(ScrollPosV<ThumbSizeV) nScrollInc = -ScrollPosV;
            }
        }
	else
        if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            if (orient == wxHORIZONTAL)
            {
               if(ScrollPosH<ScrollRangeCorrectedH) nScrollInc = ThumbSizeH;
               if((ScrollRangeCorrectedH-ScrollPosH)<ThumbSizeH) nScrollInc = ScrollRangeCorrectedH-ScrollPosH;
            }
            else
            {
               if(ScrollPosV<ScrollRangeCorrectedV) nScrollInc = ThumbSizeV;
               if((ScrollRangeCorrectedV-ScrollPosV)<ThumbSizeV) nScrollInc = ScrollRangeCorrectedV-ScrollPosV;
            }
        }
	else
        if((event.GetEventType()==wxEVT_SCROLLWIN_THUMBTRACK)||
        (event.GetEventType()==wxEVT_SCROLLWIN_THUMBRELEASE))
        {
            if (orient == wxHORIZONTAL)
                nScrollInc = pos - m_xScrollPosition;
            else
                nScrollInc = pos - m_yScrollPosition;
        }

    return nScrollInc;
}
