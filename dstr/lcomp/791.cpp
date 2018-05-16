#include <string.h>
#include "../include/stubs.h"

#include "../include/ioctl.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/791.h"
#include "../include/791cmd.h"

// API for  AlteraBM L791 PCI chip /////////////////////////////////////////////////
HANDLE DaqL791::csOpenLDevice()
{
   #ifdef LCOMP_LINUX
      map_regSize = 4096; // page size
      map_regBuffer = mmap(0, map_regSize, PROT_READ|PROT_WRITE, MAP_SHARED/*|MAP_LOCKED*/, hVxd, 0x3000);  //0x3* sysconf(_SC_PAGE_SIZE));
      if(map_regBuffer==NULL) return INVALID_HANDLE_VALUE;
      return hVxd;
   #else
      return hVxd;
   #endif
}


ULONG DaqL791::csCloseLDevice(ULONG status)
{
   ULONG sts;
   #ifdef LCOMP_LINUX
      if(map_regBuffer) { munmap(map_regBuffer, map_regSize); map_regBuffer=NULL; map_regSize=0; }
      return status;
   #else
      if(DataBuffer)
      {
         sts = VirtualFree(DataBuffer,0,MEM_RELEASE);
         DataBuffer = 0;
         if(!sts) return L_ERROR;
      }
      return status;
   #endif
}

ULONG DaqL791::csRequestBufferStream(ULONG *Size, ULONG StreamId, ULONG status) //in words
{
   #ifdef LCOMP_LINUX
      *Size+=2048;
      return status; // in linux alloc memory in driver...
   #else
      if(DataBuffer) return status;
      else
      {
         DataBuffer = (PULONG)VirtualAlloc(NULL,*Size*2,MEM_COMMIT,PAGE_READWRITE); // alloc 1 mbyte of ram
         DataSize = *Size*2;
         if(DataBuffer==NULL) return L_ERROR;
         else return status;
      }
   #endif
}

ULONG DaqL791::csSetParametersStream(PDAQ_PAR sp, PULONG UsedSize, void** Data, void** Sync, ULONG StreamId, ULONG status)
{
   #ifdef LCOMP_LINUX
      *Sync = map_regBuffer; // sync page anr hardvare regs...
      *Data = (char *)*Data-4096; // shift back - no sync page //
      return status;
   #else
      if(StreamId==L_STREAM_ADC) *Data = DataBuffer;
      if(StreamId==L_STREAM_DAC) *Data = &DataBuffer[DataSize/(2*sizeof(ULONG))];
      // but Sync point to mapped registers
      return status;
   #endif
}


FDF(ULONG) DaqL791::PlataTest()  // stub function
{
   return L_SUCCESS;
}

FDF(ULONG) DaqL791::ReadPlataDescr(LPVOID pd)
{
   USHORT d;
   for(USHORT i=0; i<sizeof(PLATA_DESCR_U); i++)
   {
      if(ReadFlashWord(i,&d)) return L_ERROR;
      pdu.bi.data[i] = (UCHAR)d;
   }

   memcpy(pd,&pdu,sizeof(PLATA_DESCR_U));
// ??? insert crc check
   return L_SUCCESS;
}



FDF(ULONG) DaqL791::WritePlataDescr(LPVOID pd, USHORT Ena)
{
int i,j;

   if(!Ena) return L_ERROR;
   memcpy(&pdu,pd,sizeof(PLATA_DESCR_U));

   USHORT crc16 = 0;
   for(i = 2; i < sizeof(PLATA_DESCR_U); i++)
   {
      crc16 ^= pdu.bi.data[i] << 8;
      for(j = 0; j < 8; j++)
      {
         if(crc16 & 0x8000) crc16 = (crc16 << 0x1) ^ 0x8005;
         else crc16 <<= 0x1;
      }
   }

   pdu.wi.data[0] = crc16;

   if(EnableFlashWrite(1)) return L_ERROR;
   for(i=0; i<sizeof(PLATA_DESCR_U); i++)
      if(WriteFlashWord(i,pdu.bi.data[i])) return L_ERROR;
   if(EnableFlashWrite(0)) return L_ERROR;
   return L_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Процедура чтения слова из пользовательского ППЗУ
////////////////////////////////////////////////////////////////////////////////
FDF(ULONG) DaqL791::ReadFlashWord(USHORT Addr, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_READ_FLASH_WORD,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL791::WriteFlashWord(USHORT Addr, USHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_WRITE_FLASH_WORD,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL791::EnableFlashWrite(USHORT Flag)
{
   ULONG cbRet;
   USHORT par = 0; // addr not use
   return !LDeviceIoControl( hVxd, DIOC_ENABLE_FLASH_WRITE,
                           &par, sizeof(par),
                           &Flag, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

ULONG DaqL791::FillADCparameters(PDAQ_PAR sp)
{
   PADC_PAR ap = (PADC_PAR)sp;

   double max_rate = 400.0;
   ULONG i;

   if(ap->t2.dRate<0) return L_ERROR;
   if(ap->t2.dKadr<0) return L_ERROR;

   if(ap->t2.dRate>max_rate) ap->t2.dRate = max_rate;


   if(ap->t2.NCh>128) ap->t2.NCh=128;
   if(ap->t2.FIFO==0) return L_ERROR;
   if(ap->t2.Pages==0) return L_ERROR;
   if(ap->t2.IrqStep==0) return L_ERROR;

   double CLOCK_OUT_L791=pdu.t3.Quartz/1000.0; // частота в кГц

   // определим частоту работы АЦП
   double CLOCK_DIV=CLOCK_OUT_L791/ap->t2.dRate-50.0;
   if(CLOCK_DIV < 0) CLOCK_DIV=0;

   adc_par.t2.Rate=(ULONG)CLOCK_DIV;

   ap->t2.dRate=CLOCK_OUT_L791/(CLOCK_DIV+50.0);

   if((1.0/(ap->t2.dRate))>(ap->t2.dKadr)) ap->t2.dKadr=1/(ap->t2.dRate);

   // полный период кадра
   double InterKadr=CLOCK_OUT_L791*ap->t2.dKadr-50.0;
   if(InterKadr < 0) InterKadr = 0;

   adc_par.t2.Kadr=(ULONG)InterKadr;

   ap->t2.dKadr = (InterKadr + 50.0)/CLOCK_OUT_L791;

// More
   adc_par.t2.SynchroType=ap->t2.SynchroType;
   adc_par.t2.SynchroSrc=ap->t2.SynchroSrc;

   adc_par.t2.FIFO=ap->t2.FIFO;
   adc_par.t2.IrqStep = ap->t2.IrqStep;
   adc_par.t2.Pages = ap->t2.Pages;

   adc_par.t2.NCh=ap->t2.NCh;
   for(i=0; i<ap->t2.NCh; i++) adc_par.t2.Chn[i]=ap->t2.Chn[i];

   adc_par.t2.AutoInit = ap->t2.AutoInit;
   adc_par.t2.IrqEna=ap->t2.IrqEna;
   adc_par.t2.AdcEna=ap->t2.AdcEna;

// make a copy  of adc_par in wadc_par for C-style interface to driver in linux////////
   CopyDAQtoWDAQ(&adc_par.t2,&wadc_par.t4,3);
   return L_SUCCESS;
};

ULONG DaqL791::FillDACparameters(PDAQ_PAR sp)
{
   PDAC_PAR dp = (PDAC_PAR)sp;

   double CLOCK_OUT_L791=pdu.t3.Quartz/1000.0; // частота в кГц

   dp->t2.dRate=l_fabs(dp->t2.dRate);
   if(dp->t2.dRate>125.0) dp->t2.dRate=125.0;

   dac_par.t2.Rate = (ULONG)(CLOCK_OUT_L791/(dp->t2.dRate)-0.5);
   if(dac_par.t2.Rate > 16777215L) dac_par.t2.Rate = 16777215L; //2^24
   if(dac_par.t2.Rate < 159) dac_par.t2.Rate = 159L;

   dp->t2.dRate = CLOCK_OUT_L791/(dac_par.t2.Rate + 1.0);

   dac_par.t2.FIFO = dp->t2.FIFO;
   dac_par.t2.IrqStep = dp->t2.IrqStep;
   dac_par.t2.Pages = dp->t2.Pages;
   dac_par.t2.AutoInit = dp->t2.AutoInit;
   dac_par.t2.DacEna = dp->t2.DacEna;
   dac_par.t2.IrqEna = dp->t2.IrqEna;

// make a copy for C style in linux
   CopyDAQtoWDAQ(&dac_par.t2,&wdac_par.t2,1);
   return L_SUCCESS;
}



// TTL lines, move it into driver
ULONG DaqL791::ConfigTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG cbRet;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_TTL_CFG,
                           &ap->Mode, sizeof(ULONG),
                           NULL, 0,
                           &cbRet, NULL
                         );
   return status;
}


ULONG DaqL791::InputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG cbRet;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_TTL_IN,
                           NULL, 0,
                           &ap->Data[0], sizeof(ULONG),
                           &cbRet, NULL
                         );
   return status;
}


ULONG DaqL791::OutputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG cbRet;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_TTL_OUT,
                           &ap->Data[0], sizeof(ULONG),
                           NULL, 0,
                           &cbRet, NULL
                         );
   return status;
}

ULONG DaqL791::InputADC(PDAQ_PAR sp) // sample
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG cbRet;
   ULONG Data;
   if( !LDeviceIoControl( hVxd, DIOC_ADCSAMPLE,
                           &ap->Chn[0], sizeof(ULONG),
                           &Data, sizeof(ULONG),
                           &cbRet, NULL
                         )
     ) return L_ERROR;

   ap->Data[0] =Data&0xFFFF;
   return L_SUCCESS;
}

ULONG DaqL791::OutputDAC(PDAQ_PAR sp) // sample
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;

   ULONG cbRet;
   ULONG par =0;
   par = (ap->Data[0]&0xFFF)|(ap->Chn[0]<<12)|((ap->Data[1]&0xFFF)<<16)|(ap->Chn[1]<<28)|(1<<30);

   if( !LDeviceIoControl( hVxd, DIOC_DAC_OUT,
                           &par, sizeof(ULONG),
                           NULL, 0,
                           &cbRet, NULL
                         )
     ) return L_ERROR;

   return L_SUCCESS;
}
