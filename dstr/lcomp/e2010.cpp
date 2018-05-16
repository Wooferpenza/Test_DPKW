#include <stdio.h>

#include <string.h>

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/e2010cmd.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/e2010.h"


FDF(ULONG) DaqE2010::ReadPlataDescr(LPVOID pd)
{
   ULONG cbRet;
   USHORT Addr=0;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_READ_FLASH_WORD,
                           &Addr, sizeof(USHORT),
                           &pdu, sizeof(PLATA_DESCR_U2),
                           &cbRet, NULL
                         );

   memcpy(pd,&pdu,sizeof(PLATA_DESCR_U2));
   return status;
}


// пишет все независимо от Ena
FDF(ULONG) DaqE2010::WritePlataDescr(LPVOID pd, USHORT Ena)
{
   memcpy(&pdu,pd,sizeof(PLATA_DESCR_U2));
   if(sl.BoardType==E2010B) pdu.t6.CRC  = CRC16CALC(&pdu.bi256.data[0], 254);
   if(!Ena) return L_ERROR;
   ULONG cbRet;
   USHORT Addr=0;

   ULONG status = !LDeviceIoControl( hVxd, DIOC_WRITE_FLASH_WORD,
                           &Addr, sizeof(USHORT),
                           &pdu, sizeof(PLATA_DESCR_U2),
                           &cbRet, NULL
                         );
   return status;
}


FDF(ULONG) DaqE2010::PlataTest()  // stub function
{
// можно тест  провести для ревизии B
// WORD InBuf[4] = { 0x0, V_TEST_PLD, 0x0, 0x0 };
// if(!TLUSBBASE::LDeviceIoControl(DIOC_SEND_COMMAND, InBuf, sizeof(InBuf), NULL, 0, TimeOut))
//   { LAST_ERROR_NUMBER(19); return FALSE; }
   return L_SUCCESS;
}


ULONG DaqE2010::ConfigTTL(PDAQ_PAR sp)
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


ULONG DaqE2010::InputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG TtlParam;
   ULONG cbRet;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_TTL_IN,
                           NULL, 0,
                           &TtlParam, sizeof(ULONG),
                           &cbRet, NULL
                         );

   ap->Data[0] = ((TtlParam << 0x8) & 0xFF00) | ((TtlParam >> 0x8) & 0x00FF);
   return status;
}


ULONG DaqE2010::OutputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG TtlParam = ((ap->Data[0] << 0x8) & 0xFF00) | ((ap->Data[0] >> 0x8) & 0x00FF);
   ULONG cbRet;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_TTL_OUT,
                           &TtlParam, sizeof(ULONG),
                           NULL, 0,
                           &cbRet, NULL
                         );
   return status;
}

ULONG DaqE2010::OutputDAC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(ap->Mode>1) return L_ERROR;
   ULONG dac_value =ap->Data[0]&0xFFF;
   dac_value |=(ap->Mode<<12);
   dac_value = ((dac_value << 0x8) & 0xFF00) | ((dac_value >> 0x8) & 0x00FF);

   ULONG cbRet;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_DAC_OUT,
                           &dac_value, sizeof(ULONG),
                           NULL, 0,
                           &cbRet, NULL
                         );
   return status;
}

//
ULONG DaqE2010::FillADCparameters(PDAQ_PAR sp)
{
PADC_PAR ap = (PADC_PAR)sp;
ULONG i;

   if(ap->t2.dRate<0) return L_ERROR;
   if(ap->t2.dKadr<0) return L_ERROR;

   if(ap->t2.Pages==0) return L_ERROR;
   if(ap->t2.IrqStep==0) return L_ERROR;

   double QF=30000.0;

   if(ap->t2.dRate > 10000.0) ap->t2.dRate = 10000.0;
   if(ap->t2.dRate < 1000.0) ap->t2.dRate = 1000.0;

   adc_par.t2.Rate = (UCHAR)(QF/ap->t2.dRate - 0.5);
   ap->t2.dRate = QF/(adc_par.t2.Rate + 1.0);

   if((1.0/(ap->t2.dRate)) > (ap->t2.dKadr)) ap->t2.dKadr = 1.0/ap->t2.dRate;
   adc_par.t2.Kadr = (ULONG)(ap->t2.dKadr*ap->t2.dRate - 0.5);

   if(sl.BoardType==E2010)
   {
      if(adc_par.t2.Kadr > 255) adc_par.t2.Kadr = 255;
   } else
   {
      if(adc_par.t2.Kadr > 65535) adc_par.t2.Kadr = 65535;
   }

   ap->t2.dKadr = (adc_par.t2.Kadr + 1.0)/ap->t2.dRate;

// More
   adc_par.t2.SynchroType=ap->t2.SynchroType;
   adc_par.t2.SynchroSrc=ap->t2.SynchroSrc;
   adc_par.t2.AdcIMask=ap->t2.AdcIMask;

   ap->t2.FIFO = 0;

   adc_par.t2.FIFO=ap->t2.FIFO;
   adc_par.t2.IrqStep = ap->t2.IrqStep;
   adc_par.t2.Pages = ap->t2.Pages;
   if(ap->t2.NCh>128) ap->t2.NCh=128;
   adc_par.t2.NCh=ap->t2.NCh;
   for(i=0; i<ap->t2.NCh; i++) adc_par.t2.Chn[i]=ap->t2.Chn[i];

   adc_par.t2.AutoInit = ap->t2.AutoInit;
   adc_par.t2.IrqEna=ap->t2.IrqEna;
   adc_par.t2.AdcEna=ap->t2.AdcEna;

// advanced sync mode

   ULONG sm = ap->t2.SynchroMode&0x0000FFFC;
   if((sm==A_SYNC_HL_LEVEL)||(sm==A_SYNC_LH_LEVEL))
   {
      adc_par.t2.StartCnt = 0;
      adc_par.t2.StopCnt = 0;
   }
   else
   {
      adc_par.t2.StartCnt = ap->t2.StartCnt;
      adc_par.t2.StopCnt = ap->t2.StopCnt;
   }
   adc_par.t2.SynchroMode = ap->t2.SynchroMode;
   adc_par.t2.DM_Ena = ap->t2.DM_Ena;
   adc_par.t2.AdPorog = ap->t2.AdPorog;

// make a copy  of adc_par in wadc_par for C-style interface to driver ////////
   CopyDAQtoWDAQ(&adc_par.t2,&wadc_par.t4,3);
   return L_SUCCESS;
}


FDF(ULONG) DaqE2010::EnableCorrection(USHORT Ena)
{
   // pdu.t6
   USHORT AdcCoef[24];
   int i;
   ULONG status;
   ULONG cbRet;
   if(sl.BoardType==E2010) return L_NOTSUPPORTED;

   for(i=0;i<12;i++) { AdcCoef[i*2] = 0; AdcCoef[i*2+1] = 0x8000; }

   if(Ena)
   {
      for(i=0;i<12;i++)
      {
         AdcCoef[i*2] = (USHORT)((pdu.t6.KoefADC[i]>0) ? (pdu.t6.KoefADC[i]+0.5) : (pdu.t6.KoefADC[i]-0.5));
         AdcCoef[i*2+1] = (USHORT)((pdu.t6.KoefADC[i+12]*0x8000)+0.5);
      }
   }

   status = !LDeviceIoControl( hVxd, DIOC_ENABLE_CORRECTION,
                           AdcCoef, sizeof(AdcCoef),
                           NULL, 0,
                           &cbRet, NULL
                         );

   return status;
}
