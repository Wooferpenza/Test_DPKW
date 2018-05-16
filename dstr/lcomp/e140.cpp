#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/e140cmd.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/e140.h"


// IDMA with  PLX9050 PCI chip /////////////////////////////////////////////////
FDF(ULONG) DaqE140::GetWord_DM(USHORT Addr, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_DM_A,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE140::PutWord_DM(USHORT Addr, USHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_DM_A,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE140::SendCommand(USHORT Cmd)
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

FDF(ULONG) DaqE140::PutWord_PM(USHORT Addr, ULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_PM_A,
                           &par, sizeof(par),
                           &Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE140::GetWord_PM(USHORT Addr, PULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_PM_A,
                           &par, sizeof(par),
                           Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE140::PutArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
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

FDF(ULONG) DaqE140::GetArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
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
// флеш читается реально по 2 байта те слово

ULONG DaqE140::PackModuleDescriptor(PPLATA_DESCR_U2 ppd)
{
   PLATA_DESCR_U2 pd;
   memcpy(&pd,ppd,sizeof(PLATA_DESCR_U2));
   memset(ppd, 0xFF, 256);

   ppd->pt5.SerNum1 = atoi(&pd.t5.SerNum[0]);
   ppd->pt5.SerNum2 = pd.t5.SerNum[1];
   ppd->pt5.SerNum3 = atoi(&pd.t5.SerNum[2]); // если там левые числа то номер будет 0
   //+6
   memcpy(ppd->pt5.Name,pd.t5.BrdName,10); // без 0 в конце строки
   ppd->pt5.Rev = pd.t5.Rev;
   memcpy(ppd->pt5.DspType,pd.t5.DspType,10);
   ppd->pt5.Quartz = pd.t5.Quartz;
   ppd->pt5.IsDacPresent = pd.t5.IsDacPresent;
   for(int i = 0; i < 4; i++)
   {
      ppd->pt5.AdcOffs[i] = pd.t5.KoefADC[i];
      ppd->pt5.AdcScale[i] = pd.t5.KoefADC[i+4];
   }
   for(int i = 0; i < 2; i++)
   {
      ppd->pt5.DacOffs[i] = pd.t5.KoefDAC[i];
      ppd->pt5.DacScale[i] = pd.t5.KoefDAC[i+2];
   }
   ppd->pt5.CRC1  = CRC8CALC(&ppd->bi.data[0], 31);
   ppd->pt5.CRC2 = CRC8CALC(&ppd->bi.data[32], 95);
   return L_SUCCESS;
}

// возвращает распакованное в ppd
ULONG DaqE140::UnPackModuleDescriptor(PPLATA_DESCR_U2 ppd)
{
   PLATA_DESCR_U2 pd;
   memcpy(&pd,ppd,sizeof(PLATA_DESCR_U2));
   memset(ppd, 0x00, 256);
   if(pd.pt5.CRC1!=CRC8CALC(&pd.bi.data[0],31)) return L_ERROR;
   if(pd.pt5.CRC2!=CRC8CALC(&pd.bi.data[32],95)) return L_ERROR;
   sprintf(ppd->t5.SerNum,"%d%c%d",pd.pt5.SerNum1,pd.pt5.SerNum2,pd.pt5.SerNum3);
   memcpy(ppd->t5.BrdName,pd.pt5.Name,10);
   ppd->t5.Rev = pd.pt5.Rev;
   memcpy(ppd->t5.DspType,pd.pt5.DspType,10);
   ppd->t5.Quartz = pd.pt5.Quartz;
   ppd->t5.IsDacPresent = pd.pt5.IsDacPresent;
   for(int i = 0; i < 4; i++)
   {
      ppd->t5.KoefADC[i] = pd.pt5.AdcOffs[i];
      ppd->t5.KoefADC[i+4] = pd.pt5.AdcScale[i];
   }
   for(int i = 0; i < 2; i++)
   {
      ppd->t5.KoefDAC[i] = pd.pt5.DacOffs[i];
      ppd->t5.KoefDAC[i+2] = pd.pt5.DacScale[i];
   }
   return L_SUCCESS;
}

FDF(ULONG) DaqE140::ReadPlataDescr(LPVOID pd)
{
   PPLATA_DESCR_U pdt = (PPLATA_DESCR_U)pd;
//   if(pdt->wi.size!=sizeof(PLATA_DESCR_U)) return L_ERROR;
   for(USHORT i=0; i<sizeof(PLATA_DESCR_U)/2; i++)
      if(ReadFlashWord(L_DESCRIPTOR_BASE_E140+i*2,&pdu.wi.data[i])) return L_ERROR;
//   pdu.wi.size = sizeof(PLATA_DESCR_U);
   if(UnPackModuleDescriptor(&pdu)) return L_ERROR;
   memcpy(pd,&pdu,sizeof(PLATA_DESCR_U));
   return L_SUCCESS;
}


// пишет все независимо от Ena
FDF(ULONG) DaqE140::WritePlataDescr(LPVOID pd, USHORT Ena)
{
   PPLATA_DESCR_U pdt = (PPLATA_DESCR_U)pd;
//   if(pdt->wi.size!=sizeof(PLATA_DESCR_U)) return L_ERROR;

   memcpy(&pdu,pd,sizeof(PLATA_DESCR_U));
   if(EnableFlashWrite(1)) return L_ERROR;
   ULONG tmp=0; //(!Ena)*32;
   PackModuleDescriptor(&pdu);
   for(USHORT i=(USHORT)tmp; i<sizeof(PLATA_DESCR_U)/2; i++)
      if(WriteFlashWord(L_DESCRIPTOR_BASE_E140+i*2,pdu.wi.data[i])) return L_ERROR;
   if(EnableFlashWrite(0)) return L_ERROR;
   UnPackModuleDescriptor(&pdu);
   return L_SUCCESS;
}

FDF(ULONG) DaqE140::PlataTest()  // stub function
{
   return L_SUCCESS;
}

ULONG DaqE140::InputADC(PDAQ_PAR sp) // sample
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


ULONG DaqE140::ConfigTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG tmp = (UCHAR)ap->Mode;
   if(PutWord_PM(L_ENABLE_TTL_OUT_E140, tmp)) return L_ERROR;
   return L_SUCCESS;
}


ULONG DaqE140::InputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   USHORT tmp;
   if(GetWord_DM(L_TTL_IN_E140,&tmp)) return L_ERROR;
   ap->Data[0] = tmp;
   return L_SUCCESS;
}


ULONG DaqE140::OutputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(PutWord_DM(L_TTL_OUT_E140,(USHORT)ap->Data[0])) return L_ERROR;
   return L_SUCCESS;
}

ULONG DaqE140::OutputDAC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(ap->Mode>2) return L_ERROR;
   if((ap->Mode==2) && (pdu.t5.Rev!='A'))
   {
      USHORT dac_v[2];
      dac_v[0] = (USHORT)(ap->Data[0]&0xFFFF);
      dac_v[1] = (USHORT)(ap->Data[1]&0xFFFF);
      if(PutArray_DM(L_DAC_SAMPLES_E140, 2, dac_v)) return L_ERROR;
   }
   else
   {
      USHORT dac_value =(USHORT)((ap->Data[0])&0xFFF);
      dac_value |=(ap->Mode<<12);
      if(PutWord_DM(L_DAC_SAMPLE_E140, dac_value)) return L_ERROR;
   }
   return L_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Процедура чтения слова из пользовательского ППЗУ
////////////////////////////////////////////////////////////////////////////////
FDF(ULONG) DaqE140::ReadFlashWord(USHORT FlashAddress, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = FlashAddress;
   return !LDeviceIoControl( hVxd, DIOC_READ_FLASH_WORD,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE140::WriteFlashWord(USHORT FlashAddress, USHORT Data)
{
   ULONG cbRet;
   USHORT par = FlashAddress;
   return !LDeviceIoControl( hVxd, DIOC_WRITE_FLASH_WORD,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE140::EnableFlashWrite(USHORT Flag)
{
   ULONG cbRet;
   USHORT par = 0; // addr not use
   return !LDeviceIoControl( hVxd, DIOC_ENABLE_FLASH_WRITE,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Flag, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

//FDF(ULONG)
ULONG DaqE140::FillADCparameters(PDAQ_PAR sp)
{
   ULONG i;
   PADC_PAR ap = (PADC_PAR)sp;

   if(ap->t1.dRate<0) return L_ERROR;
   if(ap->t1.dKadr<0) return L_ERROR;

   if(ap->t1.Pages==0) return L_ERROR;
   if(ap->t1.IrqStep==0) return L_ERROR;

   double QF=pdu.t5.Quartz/1000.0;
   double DSP_CLOCK_OUT_PLX=QF;
   if(DSP_CLOCK_OUT_PLX == 0) return L_ERROR;

   // частота сбора в единицах SCLOCK_DIV SPORT DSP
   double SCLOCK_DIV=DSP_CLOCK_OUT_PLX/(2.0*(ap->t1.dRate))+0.5;
   if(SCLOCK_DIV > 65536.0) SCLOCK_DIV=65536.0;
   
   int llimit = 40;
   if(pdu.t5.Rev=='A') llimit=80;
   if(SCLOCK_DIV < llimit) SCLOCK_DIV=llimit;

   adc_par.t1.Rate = (USHORT)SCLOCK_DIV;
   ap->t1.dRate=DSP_CLOCK_OUT_PLX/(2.0*(adc_par.t1.Rate));
   adc_par.t1.FPDelay=0;

   //
   int kadrdelay=(int)((ap->t1.dKadr)*(ap->t1.dRate)+0.5);
   if(kadrdelay > 256) kadrdelay=256;
   if(kadrdelay == 0) kadrdelay=1;
   adc_par.t1.Kadr = kadrdelay-1;
   ap->t1.dKadr=(kadrdelay)/(ap->t1.dRate);

   adc_par.t1.Scale = 0;

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
   if(ap->t1.NCh>128) ap->t1.NCh=128;
   adc_par.t1.NCh=ap->t1.NCh;
   for(i=0; i<ap->t1.NCh; i++) adc_par.t1.Chn[i]=ap->t1.Chn[i];

   adc_par.t1.AutoInit = ap->t1.AutoInit;
   adc_par.t1.IrqEna=ap->t1.IrqEna;
   adc_par.t1.AdcEna=ap->t1.AdcEna;

// make a copy  of adc_par in wadc_par for C-style interface to driver ////////
	CopyDAQtoWDAQ(&adc_par.t1,&wadc_par.t3,2);
   return L_SUCCESS;
}
