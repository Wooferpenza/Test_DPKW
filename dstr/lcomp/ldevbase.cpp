#include <stdio.h>
#include <string.h>

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"

HRESULT __stdcall LDaqBoard::QueryInterface(const IID& iid, void** ppv)
{
   if(iid==IID_IUnknown) {
      *ppv = static_cast<IDaqLDevice*>(this);
   } else {
      if(iid==IID_ILDEV) {
         *ppv = static_cast<IDaqLDevice*>(this);
      } else {
         *ppv=NULL;
            return E_NOINTERFACE;
      }
   }
   reinterpret_cast<LUnknown*>(*ppv)->AddRef();
   return S_OK;
}

ULONG __stdcall LDaqBoard::AddRef()
{
   atomic_inc(&m_cRef);
   return m_cRef.counter;
 //  return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall LDaqBoard::Release()
{
   atomic_dec(&m_cRef);
   if(m_cRef.counter==0) {
//   if(InterlockedDecrement(&m_cRef)==0) {
      delete this;
      return 0;
   }
   return m_cRef.counter;
}

// Working with I/O ports
// data - data len - in bytes(sizeof)
FDF(ULONG) LDaqBoard::inbyte(ULONG offset, PUCHAR data, ULONG len, ULONG key )
{
   ULONG cbRet;
   PORT_PAR par; par.datatype=0+key; par.port=offset;
   return !LDeviceIoControl( hVxd, DIOC_INP,
                          &par, sizeof(PORT_PAR),
                          data,len,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::inword(ULONG offset, PUSHORT data, ULONG len, ULONG key )
{
   ULONG cbRet;
   PORT_PAR par; par.datatype=1+key; par.port=offset;
   return !LDeviceIoControl( hVxd, DIOC_INP,
                          &par, sizeof(PORT_PAR),
                          data,len,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::indword(ULONG offset, PULONG data, ULONG len, ULONG key )
{
   ULONG cbRet;
   PORT_PAR par; par.datatype=2+key; par.port=offset;
   return !LDeviceIoControl( hVxd, DIOC_INP,
                          &par, sizeof(PORT_PAR),
                          data,len,
                          &cbRet, NULL
                           );
}

FDF(ULONG) LDaqBoard::outbyte(ULONG offset, PUCHAR data, ULONG len, ULONG key )
{
   ULONG cbRet;
   PORT_PAR par; par.datatype=0+key; par.port=offset;
   return !LDeviceIoControl( hVxd, DIOC_OUTP,
                          &par, sizeof(PORT_PAR),
                          data,len,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::outword(ULONG offset, PUSHORT data, ULONG len, ULONG key )
{
   ULONG cbRet;
   PORT_PAR par; par.datatype=1+key; par.port=offset;
   return !LDeviceIoControl( hVxd, DIOC_OUTP,
                          &par, sizeof(PORT_PAR),
                          data,len,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::outdword(ULONG offset, PULONG data, ULONG len, ULONG key )
{
   ULONG cbRet;
   PORT_PAR par; par.datatype=2+key; par.port=offset;
   return !LDeviceIoControl( hVxd, DIOC_OUTP,
                          &par, sizeof(PORT_PAR),
                          data,len,
                          &cbRet, NULL
                         );
}

// Working with mem I/O ports
FDF(ULONG) LDaqBoard::inmbyte(ULONG Offset, PUCHAR Data, ULONG Length, ULONG Key)
{
   ULONG cbRet;
   PORT_PAR Par; Par.datatype=0+Key; Par.port=Offset;
   return !LDeviceIoControl( hVxd, DIOC_INM,
                          &Par, sizeof(PORT_PAR),
                          Data, Length,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::inmword(ULONG Offset, PUSHORT Data, ULONG Length, ULONG Key)
{
   ULONG cbRet;
   PORT_PAR Par; Par.datatype=1+Key; Par.port=Offset;
   return !LDeviceIoControl( hVxd, DIOC_INM,
                          &Par, sizeof(PORT_PAR),
                          Data, Length,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::inmdword(ULONG Offset, PULONG Data, ULONG Length, ULONG Key)
{
   ULONG cbRet;
   PORT_PAR Par; Par.datatype=2+Key; Par.port=Offset;
   return !LDeviceIoControl( hVxd, DIOC_INM,
                          &Par, sizeof(PORT_PAR),
                          Data, Length,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::outmbyte(ULONG Offset, PUCHAR Data, ULONG Length, ULONG Key)
{
   ULONG cbRet;
   PORT_PAR Par; Par.datatype=0+Key; Par.port=Offset;
   return !LDeviceIoControl( hVxd, DIOC_OUTM,
                          &Par, sizeof(PORT_PAR),
                          Data, Length,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::outmword(ULONG Offset, PUSHORT Data, ULONG Length, ULONG Key)
{
   ULONG cbRet;
   PORT_PAR Par; Par.datatype=1+Key; Par.port=Offset;
   return !LDeviceIoControl( hVxd, DIOC_OUTM,
                          &Par, sizeof(PORT_PAR),
                          Data, Length,
                          &cbRet, NULL
                         );
}

FDF(ULONG) LDaqBoard::outmdword(ULONG Offset, PULONG Data, ULONG Length, ULONG Key)
{
   ULONG cbRet;
   PORT_PAR Par; Par.datatype=2+Key; Par.port=Offset;
   return !LDeviceIoControl( hVxd, DIOC_OUTM,
                          &Par, sizeof(PORT_PAR),
                          Data, Length,
                          &cbRet, NULL
                         );
}

// COMMON FUNCTIONS //////////////////////////////////////
FDF(ULONG) LDaqBoard::GetSlotParam(PSLOT_PAR slPar)
{
   memcpy(slPar,&sl,sizeof(SLOT_PAR));
   return 0;
}


FDF(HANDLE) LDaqBoard::OpenLDevice()
{
char szDrvName[18], slot[4];
ULONG status, cbRet;

   sprintf(slot,"%d",m_Slot);

   #ifdef LCOMP_LINUX
      strcpy(szDrvName,"/dev/ldev");
   #else
      strcpy(szDrvName,"\\\\.\\LDEV");
   #endif

   strncat(szDrvName,slot,strlen(slot));

   hVxd = LCreateFile(szDrvName);

   if(hVxd==INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

   status = !LDeviceIoControl(hVxd,DIOC_GET_PARAMS,
                              NULL,0,
                              &sl,sizeof(SLOT_PAR),
                              &cbRet,NULL);

   if(status) return INVALID_HANDLE_VALUE; // must be for register config!!!
   hEvent = 0;

   return csOpenLDevice();
}

FDF(ULONG) LDaqBoard::CloseLDevice()
{
   ULONG status = L_ERROR;
   if(hVxd==INVALID_HANDLE_VALUE) return status;

   status = LCloseHandle(hVxd);
   hVxd=INVALID_HANDLE_VALUE;    ////////////////      !!!!!!!!!!!!!!!!!! close before open

   #ifdef LCOMP_LINUX
      if(map_inBuffer) { munmap(map_inBuffer, map_inSize*sizeof(short)); map_inBuffer=NULL; map_inSize=0; }
      if(map_outBuffer) { munmap(map_outBuffer, map_outSize*sizeof(short)); map_outBuffer=NULL; map_outSize=0; }
   #endif

   #ifndef LCOMP_LINUX
      if(hEvent) {CloseHandle(hEvent); hEvent = 0;}
   #endif

   return csCloseLDevice(status);
}


// uni stream interface
FDF(ULONG) LDaqBoard::RequestBufferStream(ULONG *Size, ULONG StreamId) //in words
{
ULONG cbRet;
ULONG OutBuf;
ULONG status = L_ERROR;
ULONG DiocCode;

   ULONG pb = *Size;
   switch(StreamId)
   {
      case L_STREAM_ADC :
      {
         DiocCode = DIOC_SETBUFFER;
      } break;
      case L_STREAM_DAC :
      {
         DiocCode = DIOC_SETBUFFER_1;
      } break;
      default: return L_ERROR;
   }


   status=!LDeviceIoControl(hVxd,DiocCode,
                          &pb,sizeof(ULONG),
                          &OutBuf,sizeof(ULONG),
                          &cbRet,NULL);
   *Size = OutBuf;   // 512*1024 kword for L791
   // in linux 128*2048
   
   // +2048 for mapping pagecount page
   // in ldevpcibm for correct -1 page returned from driver

   #ifdef LCOMP_LINUX
      switch(StreamId)
      {
         case L_STREAM_ADC :
         {
            if(map_inBuffer) munmap(map_inBuffer, map_inSize*sizeof(short));
            map_inSize = *Size+2048;
            map_inBuffer = mmap(0, map_inSize*sizeof(short), PROT_READ, MAP_SHARED/*|MAP_LOCKED*/, hVxd, 0x1000); //may be correct 0x1*sysconf(_SC_PAGE_SIZE));
            if(map_inBuffer==MAP_FAILED) { map_inBuffer=NULL; status=L_ERROR;}
         } break;
         case L_STREAM_DAC :
         {
            if(map_outBuffer) munmap(map_outBuffer, map_outSize*sizeof(short));
            map_outSize = *Size+2048;
            map_outBuffer = mmap(0, map_outSize*sizeof(short), PROT_READ | PROT_WRITE, MAP_SHARED/*|MAP_LOCKED*/, hVxd, 0x2000); //may be correct 0x2*sysconf(_SC_PAGE_SIZE));
            if(map_outBuffer==MAP_FAILED) { map_outBuffer=NULL; status=L_ERROR; }
         } break;
         default: return L_ERROR;
      }
   #endif

   return csRequestBufferStream(Size,StreamId,status); // call to class specific function (if no - simple return status)//
}


FDF(ULONG) LDaqBoard::SetParametersStream(PDAQ_PAR sp, PULONG UsedSize, void** Data, void** Sync, ULONG StreamId)
{
ULONG cbRet;
#ifdef WIN64
   ULONG64 OutBuf[4];
#else // for standart Windows 32
   ULONG32 OutBuf[4];
#endif
ULONG status = L_ERROR;
USHORT *d1;
ULONG DiocCode;

#ifdef LCOMP_LINUX
// convert to C-style code linux
 #define _PDAQ_PAR PWDAQ_PAR
 #define _adc_par wadc_par
 #define _dac_par wdac_par
 #define _ADC_PAR WDAQ_PAR
 #define _DAC_PAR WDAQ_PAR
 #define ret_val(v) (dp->t1.v)
// ^above is possible because this union
#else
// C++ code windows
 #define _PDAQ_PAR PDAQ_PAR
 #define _adc_par adc_par
 #define _dac_par dac_par
 #define _ADC_PAR ADC_PAR
 #define _DAC_PAR DAC_PAR
 #define ret_val(v) (dp->v)
#endif

_PDAQ_PAR dp;
ULONG sz;
void *ptr;
USHORT tPages, tFIFO, tIrqStep;


   switch(StreamId)
   {
      case L_STREAM_ADC: {DiocCode = DIOC_SETUP; dp = (_PDAQ_PAR)&_adc_par; sz = sizeof(_ADC_PAR); } break;
      case L_STREAM_DAC: {DiocCode = DIOC_SETUP_DAC; dp = (_PDAQ_PAR)&_dac_par; sz = sizeof(_DAC_PAR); } break;
      default: return status;
   };

   status=!LDeviceIoControl(hVxd,DiocCode,
                          dp,sz,
                          OutBuf,sizeof(OutBuf), // sizeof(PVOID) PVOID platform dependent
                          &cbRet,NULL);

   tPages   = (USHORT)OutBuf[0];
   tFIFO    = (USHORT)OutBuf[1];
   tIrqStep = (USHORT)OutBuf[2];

   ret_val(Pages)   = tPages;            /////////////// FIX IT !!!!!!!!!!!!!!!!
   ret_val(FIFO)    = tFIFO;
   ret_val(IrqStep) = tIrqStep;
   *UsedSize        = tPages*tIrqStep;

   #ifdef LCOMP_LINUX  // for linux
      switch(StreamId)
      {
         case L_STREAM_ADC: { ptr = map_inBuffer; } break;
         case L_STREAM_DAC: { ptr = map_outBuffer;} break;
         default: return status;
      }
   #else // for windows
      ptr = (void *)OutBuf[3];
   #endif

   if(ptr==NULL) return L_ERROR;

   *Sync = (PULONG)ptr;
   d1 = (PUSHORT)ptr;
   *Data = &d1[2048];

   if(sp!=NULL)
   {
      sp->Pages = tPages;   // update properties to new real values;
      sp->FIFO = tFIFO;
      sp->IrqStep = tIrqStep;
   }

   return csSetParametersStream(sp, UsedSize, Data, Sync, StreamId, status);
}


FDF(ULONG) LDaqBoard::FillDAQparameters(PDAQ_PAR sp)
{
   if(sp==NULL) return L_ERROR;

   switch(sp->s_Type)
   {
   case L_ADC_PARAM: return FillADCparameters(sp);
   case L_DAC_PARAM: return FillDACparameters(sp);
   default : return L_ERROR;
   }
}


FDF(ULONG) LDaqBoard::IoAsync(PDAQ_PAR sp)
{
   if(sp==NULL) return L_ERROR;
   switch(sp->s_Type)
   {
   case L_ASYNC_ADC_CFG: return ConfigADC(sp);
   case L_ASYNC_TTL_CFG: return ConfigTTL(sp);
   case L_ASYNC_DAC_CFG: return ConfigDAC(sp);

   case L_ASYNC_ADC_INP: return InputADC(sp);
   case L_ASYNC_TTL_INP: return InputTTL(sp);

   case L_ASYNC_TTL_OUT: return OutputTTL(sp);
   case L_ASYNC_DAC_OUT: return OutputDAC(sp);

   default : return L_ERROR;
   }
}

// end of uni stream interface


FDF(ULONG) LDaqBoard::InitStartLDevice()
{
ULONG cbRet, InBuf, OutBuf, status = L_ERROR;
   status = !LDeviceIoControl(hVxd,DIOC_INIT_SYNC,
                           &InBuf,sizeof(ULONG),
                           &OutBuf,sizeof(ULONG),
                           &cbRet,NULL
                         );
   return status;
}


FDF(ULONG) LDaqBoard::StartLDevice()
{
ULONG cbRet, InBuf, status =  L_ERROR;

   #ifndef LCOMP_LINUX
   hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   memset(&ov, 0, sizeof(OVERLAPPED));
   ov.hEvent = hEvent;
   #endif

   status = !LDeviceIoControl(hVxd,DIOC_START,
                         &InBuf,sizeof(ULONG),
                         DataBuffer,DataSize, // here we send data buffer parameters to lock in driver
                         &cbRet,&ov
                        );

   #ifndef LCOMP_LINUX
   if(GetLastError() == ERROR_IO_PENDING) return L_SUCCESS;
   #endif

   return status;
}

FDF(ULONG) LDaqBoard::StopLDevice()
{
ULONG cbRet, InBuf, OutBuf;
ULONG status = L_ERROR;
   status = !LDeviceIoControl(hVxd,DIOC_STOP,
                          &InBuf,sizeof(ULONG),
                          &OutBuf,sizeof(ULONG),
                          &cbRet,NULL

                          );
   #ifndef LCOMP_LINUX
      if(hEvent) { CloseHandle(hEvent); hEvent = 0; }
   #endif
   return status;
};


/////////
// work with event
//////////////////////////////////////////////////////////
// DIOC_SETEVENT - adc stop event; DIOC_SETEVENT_DAC - dac stop event
FDF(ULONG) LDaqBoard::SetLDeviceEvent(HANDLE hEvnt,ULONG EventId)
{
   #ifndef LCOMP_LINUX
   PVOID InBuf[1];
   ULONG OutBuf,cbRet;
   InBuf[0]=hEvnt;
   ULONG DiocCode;


   switch(EventId)
   {
   //case L_EVENT_ADC_BUF:
   case L_STREAM_ADC : DiocCode = DIOC_SETEVENT; break;

   //case L_EVENT_DAC_BUF:
   case L_STREAM_DAC : DiocCode = DIOC_SETEVENT_DAC; break;

   case L_EVENT_ADC_OVF : DiocCode = DIOC_SETEVENT_1; break;
   case L_EVENT_ADC_FIFO : DiocCode = DIOC_SETEVENT_2; break;
   case L_EVENT_DAC_USER : DiocCode = DIOC_SETEVENT_3; break;
   case L_EVENT_DAC_UNF : DiocCode = DIOC_SETEVENT_4; break;
   case L_EVENT_PWR_OVR : DiocCode = DIOC_SETEVENT_5; break;

   default: return L_ERROR;
   }

   return !DeviceIoControl(hVxd,DiocCode,
                           &InBuf,sizeof(PVOID),
                           &OutBuf,sizeof(OutBuf),
                           &cbRet,NULL
                           );
   #else
      return L_NOTSUPPORTED;
   #endif

}

void LDaqBoard::CopyDAQtoWDAQ(PDAQ_PAR dp, LPVOID ss, int sp_type)
{
//DAC_PAR_0 d0_sp;  //0
//DAC_PAR_1 d1_sp;  //1
//ADC_PAR_0 a0_sp;  //2
//ADC_PAR_1 a1_sp;  //3
PDAC_PAR dac = (PDAC_PAR)dp;
PADC_PAR adc = (PADC_PAR)dp;
PWDAQ_PAR sp = (PWDAQ_PAR)ss;
      // декодируем тип структуры
   switch (sp_type) {
      case 0:
         {
            sp->t1.s_Type = dac->t1.s_Type;
            sp->t1.FIFO = dac->t1.FIFO;
            sp->t1.IrqStep = dac->t1.IrqStep;
            sp->t1.Pages = dac->t1.Pages;
            sp->t1.AutoInit = dac->t1.AutoInit;
            sp->t1.dRate = dac->t1.dRate;
            sp->t1.Rate = dac->t1.Rate;
            sp->t1.DacNumber = dac->t1.DacNumber;
            sp->t1.DacEna = dac->t1.DacEna;
            sp->t1.IrqEna = dac->t1.IrqEna;
         } break;
      case 1:
         {
            sp->t2.s_Type = dac->t2.s_Type;
            sp->t2.FIFO = dac->t2.FIFO;
            sp->t2.IrqStep = dac->t2.IrqStep;
            sp->t2.Pages = dac->t2.Pages;
            sp->t2.AutoInit = dac->t2.AutoInit;
            sp->t2.dRate = dac->t2.dRate;
            sp->t2.Rate = dac->t2.Rate;
            sp->t2.DacEna = dac->t2.DacEna;
            sp->t2.IrqEna = dac->t2.IrqEna;
         } break;
      case 2:
         {
            sp->t3.s_Type = adc->t1.s_Type;
            sp->t3.FIFO = adc->t1.FIFO;
            sp->t3.IrqStep = adc->t1.IrqStep;
            sp->t3.Pages = adc->t1.Pages;
            sp->t3.AutoInit = adc->t1.AutoInit;
            sp->t3.dRate = adc->t1.dRate;
            sp->t3.dKadr = adc->t1.dKadr;
            sp->t3.dScale = adc->t1.dScale;
            sp->t3.Rate = adc->t1.Rate;
            sp->t3.Kadr = adc->t1.Kadr;
            sp->t3.Scale = adc->t1.Scale;
            sp->t3.FPDelay = adc->t1.FPDelay;

            sp->t3.SynchroType = adc->t1.SynchroType;
            sp->t3.SynchroSensitivity = adc->t1.SynchroSensitivity;
            sp->t3.SynchroMode = adc->t1.SynchroMode;
            sp->t3.AdChannel = adc->t1.AdChannel;
            sp->t3.AdPorog = adc->t1.AdPorog;
            sp->t3.NCh = adc->t1.NCh;
            for(int i=0;i<128;i++) sp->t3.Chn[i] = adc->t1.Chn[i];
            sp->t3.AdcEna = adc->t1.AdcEna;
            sp->t3.IrqEna = adc->t1.IrqEna;
         } break;
      case 3:
         {
            sp->t4.s_Type = adc->t2.s_Type;
            sp->t4.FIFO = adc->t2.FIFO;
            sp->t4.IrqStep = adc->t2.IrqStep;
            sp->t4.Pages = adc->t2.Pages;
            sp->t4.AutoInit = adc->t2.AutoInit;
            sp->t4.dRate = adc->t2.dRate;
            sp->t4.dKadr = adc->t2.dKadr;
            sp->t4.Reserved1 = adc->t2.Reserved1;
            sp->t4.DM_Ena = adc->t2.DM_Ena;
            sp->t4.Rate = adc->t2.Rate;
            sp->t4.Kadr = adc->t2.Kadr;
            sp->t4.StartCnt = adc->t2.StartCnt;
            sp->t4.StopCnt = adc->t2.StopCnt;


            sp->t4.SynchroType = adc->t2.SynchroType;
            sp->t4.SynchroMode = adc->t2.SynchroMode;
            sp->t4.AdPorog = adc->t2.AdPorog;
            sp->t4.SynchroSrc = adc->t2.SynchroSrc;
            sp->t4.AdcIMask = adc->t2.AdcIMask;
            sp->t4.NCh = adc->t2.NCh;
            for(int i=0;i<128;i++) sp->t4.Chn[i] = adc->t2.Chn[i];
            sp->t4.AdcEna = adc->t2.AdcEna;
            sp->t4.IrqEna = adc->t2.IrqEna;
         } break;
      }
}
