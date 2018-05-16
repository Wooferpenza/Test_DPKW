#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "str_zoom.h"
#include "zoomer.h"

// 128 80 192-c0

static unsigned long rgbdef[] = {0x0000FF00, 0x00FF0000, 0x00FFFF00, 0x000000FF,
                                 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF, 0x00800000,
                                 0x00008000, 0x00808000, 0x00000080, 0x00800080,
                                 0x00008080, 0x00000000, 0x000080C0, 0x000000C0};

wxZoomer::~wxZoomer()
{
   if(zw) { delete zw; zw=0; }
}

wxZoomer::wxZoomer()
{
   a=0;
   DSize=0;
   p=0;
   ActiveCh=0;
   NCh=16;
   zw =0;

   FontFamily = wxFONTFAMILY_DEFAULT;
   FontHeight=10;

   WA = wxRect(0,0,0,0);
   ZA = wxRect(0,0,50,50);
   ZAN=ZA;
   MCoo.x = 0; MCoo.y = 0;  // mouse on start

   rgbcur = rgbdef; // points to default 16  channels color

   ZoomProp = 30;
   ZP.Set(8,8,wxColour(192,192,192),wxColour(128,128,128),
              wxColour(0,255,0),wxColour(192,192,192),0);

}

int wxZoomer::SetParameters(int Prop, ZoomParam *zp)
{
   if(!zp)
   {
      ZP.Set(8,8,wxColour(192,192,192),wxColour(128,128,128),
                 wxColour(0,255,0),wxColour(192,192,192),0);
   }
   else ZP=*zp;

   ZoomProp = Prop;  // proportion of zoomer
   return 1;
}
/////////////////////////////////////////////////////////////////////////////
// Size is for each channel
int wxZoomer::SetDataI(int *Data, int Size, int Chan, int ActCh, ZoomScale *zs, unsigned long *rgb)
{
   if(rgb) rgbcur = rgb;
   a=Data;
   DSize=Size;
   NCh=Chan;
   ActiveCh=ActCh;
   ZS=*zs;
   // we can protect from color array failure
   return 1;
}

// Rubber //////////////////////////////////////////////////////////////////////

int wxZoomer::OnButtonDown(wxRect &rect, wxMouseEvent& event, int Mode)
{
   wxPoint point;
   event.GetPosition((int*)&point.x,(int*)&point.y);
   point.x = point.x - WA.x; // zero based point of mouse
   point.y = point.y - WA.y;

   if(PA.Contains(point))
   {
      if(Mode==-1)
      {
         if(!zw) { zw = new wxZoomer(); }
         else { delete zw; zw = 0; }
      }

      if(zw)
      {
         if(event.LeftIsDown()) // if zoommer enabled then move rubber and update zoom window
         {
            int ox = (point.x-PA.x)-ZAN.x-(ZAN.width-1)/2;
            int oy = (point.y-PA.y)-ZAN.y-(ZAN.height-1)/2;
            ZAN.Offset(wxPoint(ox,oy));
         }

         if(event.RightIsDown()) // start expand roober
         {
            ZA = wxRect(MCoo.x,MCoo.y,3,3);
            ZAN=ZA;
         }
      }
      return 1;
   }
   
   if(zw) return zw->OnButtonDown(rect,event,0);
   return 0;
}

// Get info about selection
int wxZoomer::GetInfo(OSCInfo *OI)
{
   int RSize=DSize/NCh-1;
   OI->IndexMin =NCh*(int)((ZAN.GetX()*RSize/(float)(PA0.width-1))+0.5);
   OI->IndexMax =NCh*(int)((ZAN.GetRight()*RSize/(float)(PA0.width-1))+0.5);

   float XvalL = ZS.Xmin + ZAN.GetX()*(ZS.Xmax-ZS.Xmin)/(PA0.width-1);
   float XvalR = ZS.Xmin + ZAN.GetRight()*(ZS.Xmax-ZS.Xmin)/(PA0.width-1);
   float Rmin = ZS.Rmax-ZAN.GetBottom()*(ZS.Rmax-ZS.Rmin)/(PA0.height-1);
   float Rmax = ZS.Rmax-ZAN.GetY()*(ZS.Rmax-ZS.Rmin)/(PA0.height-1);
   float Ymin = ZS.Ymax-ZAN.GetBottom()*(ZS.Ymax-ZS.Ymin)/(PA0.height-1);
   float Ymax = ZS.Ymax-ZAN.GetY()*(ZS.Ymax-ZS.Ymin)/(PA0.height-1);
   OI->Scale.Set(XvalL,XvalR,Ymin,Ymax,Rmin,Rmax,ZS.Width,ZS.Format);
   return 1;
}

int wxZoomer::OnMouseMove(wxRect &rect, wxMouseEvent& event)
{
   wxPoint point;
   event.GetPosition((int*)&point.x,(int*)&point.y);
   point.x = point.x - WA.x; // zero based mouse coo
   point.y = point.y - WA.y;

   if(PA.Contains(point))
   {
      int dx = point.x-PA.x; // zero based mouse coo in plot area
      int dy = point.y-PA.y;

      MCoo.x=dx; MCoo.y=dy;

      if(zw)
      {
         if(event.RightIsDown()) // expand rubber
         {
            ZA.width=dx-ZA.x+1; if(dx<ZA.x) ZA.width-=2;
            ZA.height=dy-ZA.y+1; if(dy<ZA.y) ZA.height-=2;
            ZAN=ZA;
            if(ZA.width<0) { ZAN.x=ZA.x+ZA.width+1; ZAN.width=-ZA.width; }
            if(ZA.height<0) {ZAN.y=ZA.y+ZA.height+1; ZAN.height=-ZA.height; }
            if(ZAN.height==1||ZAN.width==1) { ZAN.Inflate(1,1);}
            return 1;
         }

         if(event.LeftIsDown()) // move rubber
         {
            int ox = dx-ZAN.x-(ZAN.width-1)/2;
            int oy = dy-ZAN.y-(ZAN.height-1)/2;
            ZAN.Offset(wxPoint(ox,oy));
         }
      }
      return 1;
   }

   if(zw) return zw->OnMouseMove(rect,event);
   return 0;
}

////////////////////////////////////////////////////////////////////////////////
int wxZoomer::MakeFont(int H, wxFontFamily family)
{
   FontFamily = family;
   FontHeight=H;
   return 1;
}


/*
#define OSC_DIGITAL             0x0001
#define OSC_DIGITAL_X           0x0002
#define OSC_DIGITAL_ZOOM        0x0004
#define OSC_DIGITAL_ZOOM_X      0x0008
#define OSC_PIXEL               0x0010
#define OSC_BIGPIXEL            0x0020
#define OSC_PIXEL_ZOOM          0x0040
#define OSC_BIGPIXEL_ZOOM       0x0080
*/
int wxZoomer::OnPaint(wxDC& dc, wxRect &rect, int PlotFlag)
{
   wxString buf;
   wxString fbuf;
   wxRect tr;
   wxSize te;
   wxMemoryDC *MDC;
   wxBitmap *bmp;
   int CentrCur=0;

   if(rect.IsEmpty()) return 0; // nothing to do

   if(zw) // truncate zoom area
   {
      int temp = rect.height;
      rect.height = rect.height*ZoomProp/100;
      tr = wxRect(rect.x, rect.y+rect.height, rect.width, temp-rect.height);
   }

   if(rect!=WA) CentrCur =1; // we need to center cursor

   WA = rect;   // store plot region size for future optimization

   dc.SetFont(wxFont(FontHeight,FontFamily,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
   te = dc.GetTextExtent(wxString("0"));

   WA0 = wxRect(0,0,WA.width,WA.height);// zero-base this region

   YA = wxRect(0,te.GetHeight()+6,te.GetWidth()*ZS.Width,WA0.height-2*(te.GetHeight()+6));
   XA =wxRect(YA.width, YA.y+YA.height,WA0.width-YA.width, YA.y);
   CA = wxRect(XA.x,0,XA.width,XA.height);
   PA = wxRect(XA.x,YA.y,XA.width,YA.height);
   YA.Deflate(2);
   XA.Deflate(2);
   CA.Deflate(2);
   PA.Deflate(2);

   if(WA0.width-YA.width<10) return 0; // protection from fault with plot and resizing

   PA0 = wxRect(0,0,PA.width,PA.height);

   // place to cursor calc to obtain coord
   CAT.x=CA.x+CA.width/2-YA.width/2;//-YA.width;  // start place to plot cursor value
   CAT.y=CA.y;

   p=new wxPoint[2*PA.width];     // +1

   bmp = new wxBitmap(WA0.width,WA0.height);
   MDC = new wxMemoryDC(*bmp);

   MDC->SetBrush(wxBrush(ZP.BgColor));
   MDC->SetPen(wxPen(ZP.BgColor));
   MDC->DrawRectangle(WA0.x,WA0.y,WA0.width,WA0.height);

   MDC->SetFont(wxFont(FontHeight,FontFamily,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));

   if(ZP.BWMode)
   {
      PlotBorder(*MDC,PA,wxColour(0,0,0),wxColour(0,0,0));
      PlotBorder(*MDC,CA,wxColour(0,0,0),wxColour(0,0,0));
   }
   else
   {
      PlotBorder(*MDC,PA,wxColour(255,255,255),wxColour(0,0,0));
      PlotBorder(*MDC,CA,wxColour(255,255,255),ZP.ZoomColor);
   }


/////////////////// Plot X scale //////////////////////////////////////////////
// Calculate can we show requried lines
   XGrid=PA0.width*2/(YA.width*3);
   XGrid=(XGrid>ZP.GridX) ? ZP.GridX:XGrid;

   fbuf << "%-" << ZS.Format;
   buf.Printf(fbuf,ZS.Xmin);
   MDC->DrawText(buf,XA.x,XA.y);

   for(int kk=1;kk<XGrid;++kk)
   {
      int xcoo = PA0.x+(PA0.width-1)*kk/XGrid;
      float Xval = ZS.Xmin + xcoo*(ZS.Xmax-ZS.Xmin)/(PA0.width-1);
      buf.Printf(fbuf,Xval);
      MDC->DrawText(buf,XA.x+(XA.width-1)*kk/XGrid,XA.y);
   }

   fbuf.clear();
   fbuf << "%" << ZS.Format;
   buf.Printf(fbuf,ZS.Xmax);
   te = MDC->GetTextExtent(buf);
   MDC->DrawText(buf,XA.GetRight()-te.GetWidth(),XA.y);


//////////////// Plot Y scale /////////////////////////////////////////////////
// Calculate can we show requried lines
   YGrid=PA0.height*2/(XA.height*3);
   YGrid=(YGrid>ZP.GridY) ? ZP.GridY:YGrid;

   fbuf.clear();
   fbuf << "%" << ZS.Format;
   buf.Printf(fbuf,ZS.Ymax);

   te = MDC->GetTextExtent(buf);
   MDC->DrawText(buf,YA.GetRight()-te.GetWidth(),YA.GetTop());

   int Offs=YA.y-CA.height/2;
   for(int kk=1;kk<YGrid;++kk)
   {
      //buf.Printf(fbuf,ZS.Ymax-(/*(int)*/((ZS.Ymax*kk/YGrid)-/*(int)*/(ZS.Ymin*kk/YGrid))));
      buf.Printf(fbuf,((YGrid-kk)*ZS.Ymax+ZS.Ymin*kk)/YGrid);
      te = MDC->GetTextExtent(buf);
      MDC->DrawText(buf,YA.GetRight()-te.GetWidth(),Offs+YA.height*kk/YGrid);
   }

   buf.Printf(fbuf,ZS.Ymin);
   te = MDC->GetTextExtent(buf);
   MDC->DrawText(buf,YA.GetRight()-te.GetWidth(),YA.GetBottom()-CA.height);
///////////////////////////////////////////////////////////////////////////////

   if(CentrCur) { MCoo.x=(PA.width-1)/2; MCoo.y=(PA.height-1)/2; } // center cursor only on resize

   MDC->SetDeviceOrigin(PA.x,PA.y);
   MDC->SetClippingRegion(PA0);

   MDC->SetBrush(wxBrush(ZP.ZoomColor));
   MDC->SetPen(wxPen(ZP.ZoomColor));
   //MDC->DrawRectangle(PA0.x,PA0.y,PA0.width,PA0.height);
   MDC->DrawRectangle(0,0,PA0.width,PA0.height);

   PlotGrid(*MDC,PA0,XGrid,YGrid);
   PlotZeroLine(*MDC,PA0,ZS);
   PlotCursor(*MDC,PA0,MCoo);

   MDC->DestroyClippingRegion();

   // plot data
   MDC->SetDeviceOrigin(0,0);
   MDC->SetClippingRegion(PA);
   MDC->SetDeviceOrigin(PA.x,PA.y+(PA.height*ZS.Rmax)/(ZS.Rmax-ZS.Rmin));

   int RSize=DSize/NCh-1;   // be without /NCh
   if(RSize>PA0.width) PlotMinMax(*MDC,RSize);
   else PlotSimple(*MDC,RSize);

   // draw rubber and set data for zoom window
   if(zw)
   {
      MDC->SetDeviceOrigin(PA.x,PA.y);

      int width = ZAN.width;
      width = width > PA0.width ? PA0.width:width;
      int height = ZAN.height;
      height =  height> PA0.height ? PA0.height:height;

      if(ZAN.x<PA0.x) { ZAN.x=PA0.x; ZAN.width = width;}
      if(ZAN.GetRight()>PA0.GetRight()) {  ZAN.x =PA0.GetRight() - width+1;}
      if(ZAN.y<PA0.y) { ZAN.y=PA0.y; }
      if(ZAN.GetBottom()>PA0.GetBottom()) { ZAN.y =PA0.GetBottom() - height+1;}

      MDC->SetBrush(*wxTRANSPARENT_BRUSH);

      MDC->SetPen(wxPen(*wxBLACK,1,wxDOT));
      MDC->DrawRectangle(ZAN.x,ZAN.y,ZAN.width,ZAN.height);
      MDC->SetPen(wxPen(*wxWHITE,1,wxDOT));
      MDC->DrawRectangle(ZAN.x+1,ZAN.y+1,ZAN.width-2,ZAN.height-2);

      OSCInfo OI;
      GetInfo(&OI);
      zw->SetDataI(&a[OI.IndexMin],(OI.IndexMax-OI.IndexMin),NCh,ActiveCh,&(OI.Scale),rgbcur);
   }

   MDC->DestroyClippingRegion();
   MDC->SetDeviceOrigin(0,0);

   // plot zoom window
   if(zw)
   {
      zw->SetParameters(ZoomProp,&ZP);
      zw->FontFamily = FontFamily;
      zw->FontHeight = FontHeight;
      zw->OnPaint(dc,tr,PlotFlag);
   }

// Calculate real X:Y value for cursor and plot it
   int idx = NCh*(int)((MCoo.x*RSize/(float)(PA0.width-1))+0.5);
   float Yval = ZS.Ymin+((a[idx+ActiveCh]-ZS.Rmin)*(ZS.Ymax-ZS.Ymin))/(ZS.Rmax-ZS.Rmin);
   float Xval = ZS.Xmin + MCoo.x*(ZS.Xmax-ZS.Xmin)/(PA0.width-1);
   buf.Printf(" (%8.2f:%8.2f) ",Xval,Yval);
   
   MDC->SetBackgroundMode(wxSOLID);
   MDC->SetTextBackground(wxColour(_R(rgbcur[ActiveCh]),_G(rgbcur[ActiveCh]),_B(rgbcur[ActiveCh])));

   bool color = (_R(rgbcur[ActiveCh])*299 + _G(rgbcur[ActiveCh])*587 + _B(rgbcur[ActiveCh])*114)<=128000;
   if(color) MDC->SetTextForeground(wxColour(*wxWHITE));
   MDC->DrawText(wxString(buf),CAT.x,CAT.y);

   dc.Blit(wxPoint(WA.x,WA.y),wxSize(WA.width,WA.height),MDC,wxPoint(0,0));

   MDC->SelectObject(wxNullBitmap);

   delete MDC;
   delete bmp;
   delete[] p;

   return 1;
}
