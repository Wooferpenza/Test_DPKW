#ifndef __ZOOMER__
#define __ZOOMER__

class wxZoomer
{
   public:
      int GetInfo(OSCInfo *);
      int SetParameters(int Prop, ZoomParam *zp=0);
      int SetDataI(int *Data, int Size, int Chan, int ActCh, ZoomScale *zs, unsigned long *rgb=0);
      int MakeFont(int H, wxFontFamily family=wxFONTFAMILY_DEFAULT);
      int OnPaint(wxDC& dc, wxRect& rect, int PlotFlag);
      int OnMouseMove(wxRect &rect, wxMouseEvent& event);
      int OnButtonDown(wxRect &rect, wxMouseEvent& event, int Mode = 0);

   public:
      wxZoomer();  // Constructor of object
      ~wxZoomer(); // Destructor of object

   //private:
      void PlotGrid(wxDC& dc, wxRect rect, int xcnt, int ycnt);
      void PlotCursor(wxDC& dc, wxRect rect, wxPoint point);
      void PlotBorder(wxDC& dc, wxRect Area, wxColour Pen1, wxColour Pen2);
      void PlotZeroLine(wxDC& dc, wxRect rect, ZoomScale scale);
      void PlotMinMax(wxDC& dc, int Size);
      void PlotSimple(wxDC& dc, int Size);

   //protected:
      wxZoomer *zw;

      int *a;        // point to data
      int DSize;     // size of data
      int NCh;       // channels in data
      int ActiveCh;  // active channel
      wxPoint MCoo;    // mouse coordinates
      wxPoint *p;      // tmp array for drawing
      unsigned long *rgbcur;

      wxFontFamily FontFamily;
      int FontHeight;

      ZoomScale ZS;  // scale for plotting
      ZoomParam ZP;  // parameters of zoomer
      int XGrid;
      int YGrid;
      int ZoomProp;   // Zoom proportion
      
      /////////////////////////////////////////
      wxRect WA;
      wxRect WA0;
      wxRect CA; // area for cursor coo plots
      wxPoint CAT; //place to print coo
      wxRect YA; // area for Y scale plot
      wxRect XA; // area for X
      wxRect PA; // area for GRAPH plot ALL depends from FONT size
      wxRect PA0; // bounded to 0
      wxRect ZA;  // zoomer area
      wxRect ZAN; // normalized;
};

#endif
