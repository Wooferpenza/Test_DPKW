#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <math.h>
#include "wxfft.h"

#define  MIN_SIZE 16
#define  MAX_SIZE (128*1024)
#define  MAX_WINDOW  7

#define  ar(i)   a[(i)*2]
#define  ai(i)   a[(i)*2+1]
#define  tsin(i) sc[(i)*2]
#define  tcos(i) sc[(i)*2+1]


/*======================================================================*/
wxFFT::wxFFT(void)
{
   Size = 0;
   nWindow = 0;
   Sign = 0;
   Data = 0;
   SinCos = 0;
   Window = 0;
}

wxFFT::~wxFFT(void)
{
   if(Data)   delete[] Data;
   if(SinCos) delete[] SinCos;
   if(Window) delete[] Window;
}

/*----------------------------------------------------------------------*/
bool wxFFT::SetSize(int size, int wnd, int sign)
{
   int siz;
   for(siz=1; siz<size; siz<<=1);
   if(siz < MIN_SIZE || siz > MAX_SIZE) return false;
   if(siz != Size)
   {
      nWindow = 0; Size = siz; 
      if(Data)   delete[] Data;
      if(SinCos) delete[] SinCos;
      if(Window) delete[] Window;
      Data   = new float[siz];
      SinCos = new float[siz];
      Window = new float[siz+2];
      if(!Data || !SinCos || !Window) return false;
      InitSinCosTable(SinCos, siz, Sign = sign);
   }
   if(SinCos && sign != Sign) InitSinCosTable(SinCos, siz, Sign = sign);
   if(wnd < 0) wnd = 0; else if(wnd > MAX_WINDOW) wnd = MAX_WINDOW;
   if(wnd != nWindow) FFTWindow(Window, siz, nWindow = wnd);
   return true;
}

/*----------------------------------------------------------------------*/
void wxFFT::SetData(int* data, int size)
{
   int n = (size < Size) ? size : Size; 
   int i = 0; 
   if(nWindow) for(; i<n; ++i) Data[i] = float(data[i]) * Window[i];
   else        for(; i<n; ++i) Data[i] = float(data[i]);
   for(; i<Size; ++i) Data[i] = 0.0f;
}

void wxFFT::SetData(short* data, int size)
{
   int n = (size < Size) ? size : Size; 
   int i = 0; 
   if(nWindow) for(; i<n; ++i) Data[i] = float(data[i]) * Window[i];
   else        for(; i<n; ++i) Data[i] = float(data[i]);
   for(; i<Size; ++i) Data[i] = 0.0f;
}

void wxFFT::SetData(float* data, int size)
{
   int n = (size < Size) ? size : Size; 
   int i = 0; 
   if(nWindow) for(; i<n; ++i) Data[i] = data[i] * Window[i];
   else        for(; i<n; ++i) Data[i] = data[i];
   for(; i<Size; ++i) Data[i] = 0.0f;
}

void  wxFFT::InitSinCosTable(float* sc, int dim, int sign)
{
   double rad = 2.0*M_PI/double(dim); 
   if(sign) rad = -rad; 
   double t = 0.0;
   for(int i=0; i<dim; i+=2, t+=rad)
   {
      sc[i]   = float(sin(t)); 
      sc[i+1] = float(cos(t));
   }
}

int wxFFT::GetData(float* data, int size)
{
   if(size >Size) size = Size;
   for(int i=0; i<size; ++i) data[i] = Data[i];
   return Size;
}

int wxFFT::GetSize() {return Size;}

void wxFFT::CalculateFFT()
{
   RFFT1(Data, SinCos, Size, Sign);
}

// for visualization purpose
int wxFFT::GetAmp20Log(int* dst)
{
   float la = float(fabs(Data[1])); 
   if(la > 0.0f) la = float(20.0*log10(la)); float ama = la; 
   for(int i=2; i<Size; i+=2, ++dst)  
   { 
      float re = Data[i]; 
      float im = Data[i+1];
      re = float(sqrt(re*re + im*im));
      if(re > 0.0f) re = float(20.0*log10(re));
      if(ama < re) ama = re;
      *dst = re;
   }
   *dst = la;
   return (int)ama;
}

void wxFFT::GetAmp(float* dst)
{
   float la = float(fabs(Data[1])); 
   for(int i=2; i<Size; i+=2, ++dst)  
   { 
      float re = Data[i]; 
      float im = Data[i+1];
      *dst = float(sqrt(re*re + im*im));
   }
   *dst = la;
}

void wxFFT::GetAmpPha(float* dst)
{
   for(int i=2; i<Size; i+=2) 
   { 
      float re = Data[i]; 
      float im = Data[i+1];
      *dst++ = float(sqrt(re*re + im*im));
      *dst++ = float(atan2(re, im));
   }
}

void  wxFFT::GetPow(float* dst)
{
   float la = float(Data[1]*Data[1]); 
   for(int i=2; i<Size; i+=2, ++dst)  
   { 
      float re = Data[i]; 
      float im = Data[i+1];
      *dst = float(re*re + im*im);
   }
   *dst = la;
}

float wxFFT::GetAmpLog(float* dst)
{
   float la = float(fabs(Data[1])); 
   if(la > 0.0f) la = float(log10(la)); float ama = la; 
   for(int i=2; i<Size; i+=2, ++dst)  
   { 
      float re = Data[i]; 
      float im = Data[i+1];
      re = float(sqrt(re*re + im*im));
      if(re > 0.0f) re = float(log10(re));
      if(ama < re) ama = re;
      *dst = re;
   }
   *dst = la;
   return(ama);
}
   

/*######################################################################*/
#define  Bfly0(j, k) {                                \
   float  t1 = ar(k); ar(k) = ar(j)-t1; ar(j) += t1;  \
          t1 = ai(k); ai(k) = ai(j)-t1; ai(j) += t1;  \
}

#define  Bfly1(j, k, s) {                             \
   float  t1 = ar(k);                                 \
   float  t2 = tcos(s)*t1 - tsin(s)*ai(k);            \
   ar(k) = ar(j)-t2; ar(j) += t2;                     \
          t2 = tsin(s)*t1 + tcos(s)*ai(k);            \
   ai(k) = ai(j)-t2; ai(j) += t2;                     \
}
/*######################################################################*/
void wxFFT::BitRFFT(float *a, int dim)
{
   for(int k=0, j=0; 1;)
   {
      int i = dim; 
      do { if(!(i >>= 1)) return; k -= i; } while(k >= 0);
      k += i+i; j++; if(j >= k) continue;
      float t = ar(j); ar(j) = ar(k); ar(k) = t;
           t = ai(j); ai(j) = ai(k); ai(k) = t;
   }
}
/*======================================================================*/
void wxFFT::FFT1(float* a, float* sc, int dim, int cs2)
{
   int j,i,pcd,pcs,ifp;
   for(j=0; j<dim; j+=2) Bfly0(j, j+1);
   for(ifp=2, pcd=(cs2) ? dim>>1 : dim>>2; pcd>2; ifp+=ifp, pcd>>=1)
   {
      for(j=0; j<dim; j+=ifp*2) Bfly0(j, j+ifp);
      for(i=1, pcs=pcd; i<ifp; ++i, pcs+=pcd)
         for(j=i; j<dim; j+=ifp*2) 
            Bfly1(j, j+ifp, pcs);
   }
   Bfly0(0, ifp);
   for(j=1; j<ifp; ++j) Bfly1(j, j+ifp, j*2);
}
/*======================================================================*/
void wxFFT::RealFFTcvt(float* a, float* sc, int dim, double s)
{
   float ms = -s;
   for(int n=dim>>1, j=1; j<n; ++j)
   {
      int k = dim-j;
      float h1r = (ar(j)+ar(k))*0.5f;
      float h1i = (ai(j)-ai(k))*0.5f;
      float h2r = (ai(j)+ai(k))*  ms;
      float h2i = (ar(j)-ar(k))*   s;
      float sum = tcos(j)*h2r - tsin(j)*h2i;
      ar(j) = h1r+sum; ar(k) = h1r-sum;
           sum = tcos(j)*h2i + tsin(j)*h2r;
      ai(j) = h1i+sum; ai(k) = sum-h1i;
   }
}
/*======================================================================*/
void  wxFFT::RFFT1(float* a, float* sc, int dim, int sign)
{
   dim >>= 1;
   if(sign)
   { // inverse FFT
      RealFFTcvt(a, sc, dim, 0.5f);
      float t = ar(0); 
      ar(0) = (t+ai(0))*0.5f; 
      ai(0) = (t-ai(0))*0.5f;
      BitRFFT(a, dim); 
      FFT1(a, sc, dim, 1);
   }
   else
   { // forvard FFT
      BitRFFT(a, dim); 
      FFT1(a, sc, dim, 1);
      RealFFTcvt(a, sc, dim, -0.5f);
      float t = ar(0); 
      ar(0) = t+ai(0); 
      ai(0) = t-ai(0);
   }
}

/* Window functions ######################################################################*/
void wxFFT::PolyCos(float *wnd, int n, long double *a, int na)
{
   long double rad = 2.0L*M_PI/double(n);
   for(int i=0, n2=n>>1; i<=n2; ++i)
   {
      long double k = rad*double(i-n); 
      long double w = a[0];
      for(int j=1; j<na; ++j, k+=k) w += a[j] * cos(k);
      wnd[i] = wnd[n-i] = float(w);
   }
}
/*----------------------------------------------------------------------*/
void  wxFFT::FFTWindow(float *wnd, int n, int window)
{
   switch(window)
   {
   default : // Rectangle
   {
      for(int i=0; i<n; ++i) wnd[i] = 1.0f;
      break;
   }
   case 1  : // Hanning
   {
      long double a[2] = { 0.5, -0.5 };
      PolyCos(wnd, n, a, 2);
      break;
   }
   case 2  : // Hamming
   {
      long double a[2] = { 0.54, -0.46 };
      PolyCos(wnd, n, a, 2);
      break;
   }
   case 3   : // Flat Top
   {
      long double a[3] = { 0.2810639, -0.5208972, 0.1980399 };
      PolyCos(wnd, n, a, 3);
      break;
   }
   case 4  : // Blackman
   {
      long double a[3] = { 0.42, -0.5, -0.08 };
      PolyCos(wnd, n, a, 3);
      break;
   }
   case 5   : // Exact Blackman
   {
      long double a[3] = { 0.4265907, -0.49656062, 0.07684867 };
      PolyCos(wnd, n, a, 3);
      break;
   }
   case 6   : // Blackman-Harris 4 term
   {
      long double a[4] = { 0.358750, -0.488290, 0.141280, -0.011680 };
      PolyCos(wnd, n, a, 4);
      break;
   }
   case 7   : // Blackman-Harris 7 term
   {
      long double a[7] = {  2.7105140069342415e-1,
                      -4.3329793923448606e-1,
                       2.1812299954311062e-1,
                      -6.5925446388030898e-2,
                       1.0811742098372268e-2,
                      -7.7658482522509342e-4,
                       1.3887217350903198e-5 };
      PolyCos(wnd, n, a, 7);
      break;
   }
   }
}
/*######################################################################*/
