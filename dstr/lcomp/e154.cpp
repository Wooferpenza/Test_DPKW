#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/e154cmd.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/e154.h"


FDF(ULONG) DaqE154::GetWord_DM(USHORT Addr, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_DM_A,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE154::PutWord_DM(USHORT Addr, USHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_DM_A,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE154::SendCommand(USHORT Cmd)
{
   ULONG cbRet;
   USHORT data=0;
   USHORT par = Cmd;
   return !LDeviceIoControl( hVxd, DIOC_COMMAND_PLX,
                           &par, sizeof(par),
                           &data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE154::PutWord_PM(USHORT Addr, ULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_PM_A,
                           &par, sizeof(par),
                           &Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE154::GetWord_PM(USHORT Addr, PULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_PM_A,
                           &par, sizeof(par),
                           Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE154::PutArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   ULONG len=1024;
   int status;
   do
   {
      if(Count<len) len=Count;
      status = LDeviceIoControl( hVxd, DIOC_PUT_DM_A,
                           &par, sizeof(par),
                           Data, len*sizeof(USHORT),
                           &cbRet, NULL
                         );
      if(!status) break;
      Data+=len;
      par+=(USHORT)len;
      Count-=len;
   } while(Count);
   return !status;

}

FDF(ULONG) DaqE154::GetArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   ULONG len=1024;
   int status;
   do
   {
      if(Count<len) len=Count;
      status = LDeviceIoControl( hVxd, DIOC_GET_DM_A,
                           &par, sizeof(par),
                           Data, len*sizeof(USHORT),
                           &cbRet, NULL
                         );
      if(!status) break;
      Data+=len;
      par+=(USHORT)len;
      Count-=len;
   } while(Count);
   return !status;
}


// -----------------------------------------------------------------------------
//  упаковка дескриптора модуля, входит неупак выходит упак
// -----------------------------------------------------------------------------

// обращение в функциях с DM натурально слово те 2 байта
// PM сделал байтовыми....в тех редких случаях когда надо по 1 байту читать писать...
// флеш читается реально по 128 байта ....

ULONG DaqE154::PackModuleDescriptor(PPLATA_DESCR_U2 ppd)
{
   PLATA_DESCR_U2 pd;
   memcpy(&pd,ppd,sizeof(PLATA_DESCR_U2));
   memset(ppd, 0xFF, 256);

   ppd->pt7.SerNum1 = atoi(&pd.t7.SerNum[0]);
   ppd->pt7.SerNum2 = pd.t7.SerNum[1];
   ppd->pt7.SerNum3 = atoi(&pd.t7.SerNum[2]); // если там левые числа то номер будет 0
   //+6
   memcpy(ppd->pt7.Name,pd.t7.BrdName,10); // без 0 в конце строки
   ppd->pt7.Rev = pd.t7.Rev;
   memcpy(ppd->pt7.DspType,pd.t7.DspType,10);
   ppd->pt7.Quartz = pd.t7.Quartz;
   ppd->pt7.IsDacPresent = pd.t7.IsDacPresent;
   for(int i = 0; i < 4; i++)
   {
      ppd->pt7.AdcOffs[i] = pd.t7.KoefADC[i];
      ppd->pt7.AdcScale[i] = pd.t7.KoefADC[i+4];
   }
   for(int i = 0; i < 2; i++)
   {
      ppd->pt7.DacOffs[i] = pd.t7.KoefDAC[i];
      ppd->pt7.DacScale[i] = pd.t7.KoefDAC[i+2];
   }
   ppd->pt7.CRC1  = CRC8CALC(&ppd->bi.data[0], 31);
   ppd->pt7.CRC2 = CRC8CALC(&ppd->bi.data[32], 95);
   return L_SUCCESS;
}

// возвращает распакованное в ppd
ULONG DaqE154::UnPackModuleDescriptor(PPLATA_DESCR_U2 ppd)
{
   PLATA_DESCR_U2 pd;
   memcpy(&pd,ppd,sizeof(PLATA_DESCR_U2));
   memset(ppd, 0x00, 256);
   if(pd.pt7.CRC1!=CRC8CALC(&pd.bi.data[0],31)) return L_ERROR;
   if(pd.pt7.CRC2!=CRC8CALC(&pd.bi.data[32],95)) return L_ERROR;
   sprintf(ppd->t7.SerNum,"%d%c%d",pd.pt7.SerNum1,pd.pt7.SerNum2,pd.pt7.SerNum3);
   memcpy(ppd->t7.BrdName,pd.pt7.Name,10);
   ppd->t7.Rev = pd.pt7.Rev;
   memcpy(ppd->t7.DspType,pd.pt7.DspType,10);
   ppd->t7.Quartz = pd.pt7.Quartz;
   ppd->t7.IsDacPresent = pd.pt7.IsDacPresent;
   for(int i = 0; i < 4; i++)
   {
      ppd->t7.KoefADC[i] = pd.pt7.AdcOffs[i];
      ppd->t7.KoefADC[i+4] = pd.pt7.AdcScale[i];
   }
   for(int i = 0; i < 2; i++)
   {
      ppd->t7.KoefDAC[i] = pd.pt7.DacOffs[i];
      ppd->t7.KoefDAC[i+2] = pd.pt7.DacScale[i];
   }
   return L_SUCCESS;
}

FDF(ULONG) DaqE154::ReadPlataDescr(LPVOID pd)
{
   ULONG cbRet;
   USHORT Addr=0; // Addr -1 read user 128 FLASH
   ULONG status = !LDeviceIoControl( hVxd, DIOC_READ_FLASH_WORD,
                           &Addr, sizeof(USHORT),
                           &pdu, sizeof(PLATA_DESCR_U),
                           &cbRet, NULL
                         );

   if(UnPackModuleDescriptor(&pdu)) return L_ERROR;
   memcpy(pd,&pdu,sizeof(PLATA_DESCR_U));
   return L_SUCCESS;
}


// пишет все независимо от Ena
FDF(ULONG) DaqE154::WritePlataDescr(LPVOID pd, USHORT Ena)
{
   memcpy(&pdu,pd,sizeof(PLATA_DESCR_U));
   if(EnableFlashWrite(1)) return L_ERROR;
   PackModuleDescriptor(&pdu);
   ULONG cbRet;
   USHORT Addr=0;
   ULONG status = !LDeviceIoControl( hVxd, DIOC_WRITE_FLASH_WORD,
                           &Addr, sizeof(USHORT),
                           &pdu, sizeof(PLATA_DESCR_U),
                           &cbRet, NULL
                         );

   if(EnableFlashWrite(0)) return L_ERROR;
   UnPackModuleDescriptor(&pdu);
   return L_SUCCESS;
}

FDF(ULONG) DaqE154::PlataTest()  // stub function
{
   return L_SUCCESS;
}

ULONG DaqE154::InputADC(PDAQ_PAR sp) // sample
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;

   ULONG cbRet;
   ULONG par = ap->Chn[0];
   ULONG Data;
   if( !LDeviceIoControl( hVxd, DIOC_ADCSAMPLE,
                           &par, sizeof(par),
                           &Data, sizeof(ULONG),
                           &cbRet, NULL
                         )
     ) return L_ERROR;

   ap->Data[0] = (USHORT)(Data&0xFFFF);

   return L_SUCCESS;
}


ULONG DaqE154::ConfigTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG tmp = (UCHAR)ap->Mode;
   if(PutWord_PM(L_ENABLE_TTL_OUT_E154, tmp)) return L_ERROR;
   return L_SUCCESS;
}


ULONG DaqE154::InputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   USHORT tmp;
   if(GetWord_DM(L_TTL_IN_E154,&tmp)) return L_ERROR;
   ap->Data[0] = tmp;
   return L_SUCCESS;
}


ULONG DaqE154::OutputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(PutWord_DM(L_TTL_OUT_E154,(USHORT)ap->Data[0])) return L_ERROR;
   return L_SUCCESS;
}

ULONG DaqE154::OutputDAC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   USHORT dac_value =(USHORT)((ap->Data[0])&0xFF);
   if(PutWord_DM(L_DAC_SAMPLE_E154, dac_value)) return L_ERROR;
   return L_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Процедура чтения слова из пользовательского ППЗУ
////////////////////////////////////////////////////////////////////////////////
FDF(ULONG) DaqE154::EnableFlashWrite(USHORT Flag)
{
   ULONG cbRet;
   USHORT par = 0; // addr not use
   return !LDeviceIoControl( hVxd, DIOC_ENABLE_FLASH_WRITE,
                           &par, sizeof(par),
                           &Flag, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

//FDF(ULONG)
ULONG DaqE154::FillADCparameters(PDAQ_PAR sp)
{
   ULONG i;
   PADC_PAR ap = (PADC_PAR)sp;
   ULONG ScaleDivs[5]={1, 4, 16, 64, 512};
   int k;
   ULONG QF;
   double DSP_CLOCK_OUT_PLX;
   double SCLOCK_DIV;
   int kadrdelay;

   if(ap->t1.dRate<0) return L_ERROR;
   if(ap->t1.dKadr<0) return L_ERROR;

   if(ap->t1.Pages==0) return L_ERROR;
   if(ap->t1.IrqStep==0) return L_ERROR;

/// freq calc....
   for(k=0; k<5; k++)
   {
      // рзберемся с частотой АЦП
      QF = pdu.t7.Quartz/ScaleDivs[k]; // ulong
      DSP_CLOCK_OUT_PLX = QF/1000.0; // double

      // частота работы АЦП в кГц
      SCLOCK_DIV = DSP_CLOCK_OUT_PLX/ap->t1.dRate - 1.5;
      if(SCLOCK_DIV > 65530.0) SCLOCK_DIV = 65530.0;
      if(SCLOCK_DIV < 10.0) SCLOCK_DIV = 10.0;

      adc_par.t1.Rate = (USHORT)SCLOCK_DIV;
      ap->t1.dRate=DSP_CLOCK_OUT_PLX/(adc_par.t1.Rate+2);
   
      // межкадровая задержка (мс)
      kadrdelay=(int)((ap->t1.dKadr)*(ap->t1.dRate)+0.5);
      if(kadrdelay == 0) kadrdelay = 1;
      if(kadrdelay > 65530) kadrdelay = 65530;

      adc_par.t1.Kadr = kadrdelay-1;
      ap->t1.dKadr = (kadrdelay)/(ap->t1.dRate);

      int rate0=adc_par.t1.Rate*ap->t1.NCh;
      int n=adc_par.t1.Kadr*adc_par.t1.Rate;
      int cnt=rate0+2+n;
      if(cnt <= 65534) break;
   }

   if(k >= 5)
   {
      k=4;
      // рзберемся с частотой АЦП
      QF = pdu.t7.Quartz/ScaleDivs[k]; // ulong
      DSP_CLOCK_OUT_PLX = QF/1000.0; // double

      // частота работы АЦП в кГц
      SCLOCK_DIV = DSP_CLOCK_OUT_PLX/ap->t1.dRate - 1.5;
      if(SCLOCK_DIV > 65530.0) SCLOCK_DIV = 65530.0;
      if(SCLOCK_DIV < 10.0) SCLOCK_DIV = 10.0;

      adc_par.t1.Rate = (USHORT)SCLOCK_DIV;
      ap->t1.dRate=DSP_CLOCK_OUT_PLX/(adc_par.t1.Rate+2);

      // межкадровая задержка (мс)
      kadrdelay=(65534-adc_par.t1.Rate*ap->t1.NCh-2)/adc_par.t1.Rate;
      if(kadrdelay > 65535) kadrdelay=1;

      adc_par.t1.Kadr = kadrdelay;

      ap->t1.dKadr = (kadrdelay+1)/(ap->t1.dRate);
   }
///
   adc_par.t1.Scale = k;
// More
   adc_par.t1.SynchroType=ap->t1.SynchroType;
   adc_par.t1.SynchroSensitivity=ap->t1.SynchroSensitivity;
   adc_par.t1.SynchroMode=ap->t1.SynchroMode;
   adc_par.t1.AdChannel=ap->t1.AdChannel;
   adc_par.t1.AdPorog=ap->t1.AdPorog;

   ap->t1.FIFO = 0;

   adc_par.t1.FIFO=ap->t1.FIFO;
   adc_par.t1.IrqStep = ap->t1.IrqStep;
   adc_par.t1.Pages = ap->t1.Pages;

   if(ap->t1.NCh>16) ap->t1.NCh=16;

   adc_par.t1.NCh=ap->t1.NCh;
   for(i=0; i<ap->t1.NCh; i++) adc_par.t1.Chn[i]=ap->t1.Chn[i];

   adc_par.t1.AutoInit = ap->t1.AutoInit;
   adc_par.t1.IrqEna=ap->t1.IrqEna;
   adc_par.t1.AdcEna=ap->t1.AdcEna;

// make a copy  of adc_par in wadc_par for C-style interface to driver ////////
   CopyDAQtoWDAQ(&adc_par.t1,&wadc_par.t3,2);
   return L_SUCCESS;
}
