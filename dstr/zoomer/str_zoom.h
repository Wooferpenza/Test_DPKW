#ifndef __STR_ZOOM___
#define __STR_ZOOM__
#include <stdio.h>


#define _B(c)  ((unsigned char)((c>>16)&0xFF))
#define _G(c)  ((unsigned char)((c>>8)&0xFF))
#define _R(c)  ((unsigned char)(c&0xFF))


#pragma pack(1)
struct ZoomParam
{
   int GridX,GridY;
   int BWMode;
   wxColour BgColor;
   wxColour ZoomColor;
   wxColour CursorColor;
   wxColour ZoomGridColor;
   void Set(int nx, int ny, wxColour bc, wxColour zc, wxColour cc, wxColour zg, int bwm = 0)
   {
      GridX=nx; GridY=ny;
      BgColor=bc; ZoomColor=zc; CursorColor=cc; ZoomGridColor=zg;
      BWMode = bwm;
   }
};


struct ZoomScale
{
   float Xmin;
   float Xmax;
   float Ymin;
   float Ymax;
   float Rmax;
   float Rmin;
   int Width;     // How much digits in scale subscription
   char Format[10];

   void Set( float xmi=-1.0f, float xma=1.0f, float ymi=-1.0f, float yma=1.0f,
             float rmi=-1.0f, float rma=1.0f, int w=10, const char *f="10.1f")
   {
      Xmin=xmi; Xmax=xma; Ymin=ymi; Ymax=yma;
      Rmin=rmi; Rmax=rma; Width=w;
      memcpy(Format,f,sizeof(Format));
   }

};

struct OSCInfo
{
   int CIndex;
   int IndexMin, IndexMax;
   ZoomScale Scale;
};
#pragma pack()

#endif
