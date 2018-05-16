class wxFFT
{
public:
   int          Size;
   float*       Data;
   float*       SinCos;
   float*       Window;
   int          nWindow;
   int          Sign;
   
   wxFFT(void);
   ~wxFFT(void);

   bool SetSize(int size, int wnd, int sign);
   void SetData(int* data, int size);
   void SetData(short* data, int size);
   void SetData(float* data, int size);
   
   int GetSize();
   int GetData(float* data, int size);
   int GetAmp20Log(int* dst);
   void GetAmp(float* dst);
   void GetAmpPha(float* dst);
   void GetPow(float* dst);
   float GetAmpLog(float* dst);
   
   void CalculateFFT();   
  
private:
   void BitRFFT(float *a, int dim);
   void InitSinCosTable(float* sc, int dim, int sign);
   void RealFFTcvt(float* a, float* sc, int dim, double s);
   void PolyCos(float *wnd, int n, long double *a, int na);
   void FFTWindow(float *wnd, int n, int window);
   void FFT1(float* a, float* sc, int dim, int cs2);
   void RFFT1(float* a, float* sc, int dim, int sign);
};
