#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "str_zoom.h"
#include "zoomer.h"
 ///////////////////////////////////////////////////////////////////////////////
// PlotMinMax
///////////////////////////////////////////////////////////////////////////////
void wxZoomer::PlotMinMax(wxDC& dc, int Size)
{

double ScrW=PA0.width;
double ScrH=-PA0.height;
//int Step=Size;
double YMM=ZS.Rmax-ZS.Rmin;
int i,min,max,t;
int n = 0;

double SY = ScrH/YMM;
   do
   {
      int j=0;
      int Temp=Size; int Comp=Size/ScrW;
      i=min=max=n++;
      for(int k=0;k<=Size;++k)
      {
         min=(a[i]<a[min]) ? i:min;
         max=(a[i]>a[max]) ? i:max;
         i+=NCh;
         if(k!=Comp) continue;
         if(min>max){t=min; min=max; max=t;}
         t=j*2;
         p[t].x=j; p[t].y=double(a[min])*SY;//ScrH/YMM;
         p[++t].x=j++; p[t].y=double(a[max])*SY;//ScrH/YMM;
         Temp+=Size; Comp=Temp/ScrW;
         min=max=i;
      }
      if(!(rgbcur[n-1]&0x10000000))  //fault fot 1 channel patched
      {
         dc.SetPen(wxPen(wxColour(_R(rgbcur[n-1]),_G(rgbcur[n-1]),_B(rgbcur[n-1]))));
         dc.DrawLines(ScrW*2,p);
      }
   } while (n<NCh);
}

// big pixel mode
void wxZoomer::PlotSimple(wxDC& dc, int Size)
{
int ScrW=PA0.width;
int ScrH=-PA0.height;
int YMM=ZS.Rmax-ZS.Rmin;
int n = 0;
int iSize = Size; if(iSize==0) iSize++;
   do
   {
      int i=n++;
      for(int k=0;k<=iSize;k++ )
      {
         p[k].x=ScrW*k/iSize;
         p[k].y=a[i]*ScrH/YMM;
         i+=NCh;
         if(!(rgbcur[n-1]&0x10000000))
         {
            dc.SetPen(wxPen(wxColour(_R(rgbcur[n-1]),_G(rgbcur[n-1]),_B(rgbcur[n-1]))));
            dc.DrawRectangle(p[k].x,p[k].y,2,2);
         }
      }
   } while (n<NCh);
}

void wxZoomer::PlotCursor(wxDC& dc, wxRect rect, wxPoint point)
{
   dc.SetPen(wxPen(ZP.CursorColor));
   dc.DrawLine(point.x,0,point.x,rect.height);
}

void wxZoomer::PlotBorder(wxDC& dc, wxRect Area, wxColour Pen1, wxColour Pen2)
{
   wxRect PAB=Area;
   PAB.Inflate(1,1);
   wxPoint pt1[3] = {wxPoint(PAB.x,PAB.y+PAB.height-1), wxPoint(PAB.x+PAB.width-1,PAB.y+PAB.height-1), wxPoint(PAB.x+PAB.width-1,PAB.y)};
   wxPoint pt2[3] = {wxPoint(PAB.x,PAB.y+PAB.height-1), wxPoint(PAB.x,PAB.y), wxPoint(PAB.x+PAB.width-1,PAB.y)};
   
   dc.SetPen(wxPen(Pen1));
   dc.DrawLines(3,pt1);
   dc.SetPen(wxPen(Pen2));
   dc.DrawLines(3,pt2);

}

void wxZoomer::PlotGrid(wxDC& dc, wxRect rect, int xcnt, int ycnt)
{
   int kk=0,tmp;
   dc.SetPen(wxPen(ZP.ZoomGridColor));
   do
   {
      kk++;
      if(kk<xcnt){
         tmp = (rect.width-1)*kk/xcnt;
         dc.DrawLine(tmp,0,tmp,rect.y+rect.height-1);
      }

      if(kk<ycnt){
         tmp=(rect.height-1)*kk/ycnt;
         dc.DrawLine(0,tmp,rect.x+rect.width-1,tmp);
      }
   } while((kk<xcnt)||(kk<ycnt));
}

void wxZoomer::PlotZeroLine(wxDC& dc, wxRect rect, ZoomScale scale)
{

   dc.SetPen(wxPen(ZP.ZoomGridColor,1,wxDOT));
   int tmp=scale.Ymax*(rect.y+rect.height-1)/(scale.Ymax-scale.Ymin);
   dc.DrawLine(0,tmp,rect.x+rect.width-1,tmp);
   tmp=scale.Xmin*(rect.x+rect.width-1)/(scale.Xmin-scale.Xmax);
   dc.DrawLine(tmp,0,tmp,rect.y+rect.height-1);
}
