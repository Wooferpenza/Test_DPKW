#include <string.h>
#include <stdio.h>


#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/e440cmd.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/e440.h"

// IDMA with  PLX9050 PCI chip /////////////////////////////////////////////////
FDF(ULONG) DaqE440::GetWord_DM(USHORT Addr, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_DM_A,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE440::PutWord_DM(USHORT Addr, USHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_DM_A,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE440::SendCommand(USHORT Cmd)
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

FDF(ULONG) DaqE440::PutWord_PM(USHORT Addr, ULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_PM_A,
                           &par, sizeof(par),
                           &Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE440::GetWord_PM(USHORT Addr, PULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_PM_A,
                           &par, sizeof(par),
                           Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE440::PutArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
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

FDF(ULONG) DaqE440::GetArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
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

FDF(ULONG) DaqE440::PutArray_PM(USHORT Addr, ULONG Count, PULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   ULONG len=1024;
   int status;
   do
   {
      if(Count<len) len=Count;
      status = LDeviceIoControl( hVxd, DIOC_PUT_PM_A,
                           &par, sizeof(par),
                           Data, len*sizeof(ULONG),
                           &cbRet, NULL
                         );
      if(!status) break;
      Data+=len;
      par+=(USHORT)len;
      Count-=len;
   } while(Count);
   return !status;
}

FDF(ULONG) DaqE440::GetArray_PM(USHORT Addr, ULONG Count, PULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   ULONG len=1024;
   int status;

   do
   {
      if(Count<len) len=Count;
      status = LDeviceIoControl( hVxd, DIOC_GET_PM_A,
                           &par, sizeof(par),
                           Data, len*sizeof(ULONG),
                           &cbRet, NULL
                         );
      if(!status) break;
      Data+=len;
      par+=(USHORT)len;
      Count-=len;
   } while(Count);
   return !status;
}

FDF(ULONG) DaqE440::PlataTest()
{
   ULONG d1; if(GetWord_PM(L_TMODE1_E440, &d1)) return L_ERROR;
   ULONG d2; if(GetWord_PM(L_TMODE2_E440, &d2)) return L_ERROR;

   if((LBIOS_INVAR(d1)!=0x5555)||(LBIOS_INVAR(d2)!=0xAAAA)) return L_ERROR;
   else
   {
      if(PutWord_PM(L_TEST_LOAD_E440, LBIOS_OUTVAR(0x77BB))) return L_ERROR;
      int TO=10000;
      do
      {
         if(GetWord_PM(L_READY_E440, &d1)) return L_ERROR;
      } while(!LBIOS_INVAR(d1)&&(TO--));
      if(TO==-1) return L_ERROR;
      if(SendCommand(cmTEST_E440)) return L_ERROR;
      if(GetWord_PM(L_TEST_LOAD_E440, &d1)) return L_ERROR;
      if(LBIOS_INVAR(d1)!=0xAA55) return L_ERROR;
   }
   return L_SUCCESS;
}



//FDF(ULONG)
ULONG DaqE440::FillADCparameters(PDAQ_PAR sp)
{
double max_rate = 400.0;
ULONG i;

   PADC_PAR ap = (PADC_PAR)sp;

   if(ap->t1.dRate<0) return L_ERROR;
   if(ap->t1.dKadr<0) return L_ERROR;

   if(ap->t1.FIFO==0) return L_ERROR;
   if(ap->t1.Pages==0) return L_ERROR;
   if(ap->t1.IrqStep==0) return L_ERROR;

   double QF=pdu.t4.Quartz/1000.0;
   double DSP_CLOCK_OUT_PLX=2.0*QF;
   if(DSP_CLOCK_OUT_PLX == 0) return L_ERROR;

   if(ap->t1.dRate < 0.1) ap->t1.dRate=0.1;
   if(ap->t1.dRate > max_rate) ap->t1.dRate=max_rate;

   // ЮБУФПФБ УВПТБ Ч ЕДЙОЙГБИ SCLOCK_DIV SPORT DSP
   double SCLOCK_DIV=DSP_CLOCK_OUT_PLX/(2.0*(ap->t1.dRate))-0.5;
   if(SCLOCK_DIV > 65534.0) SCLOCK_DIV=65534.0;

   adc_par.t1.Rate = (USHORT)SCLOCK_DIV;
   ap->t1.dRate=DSP_CLOCK_OUT_PLX/(2.0*(adc_par.t1.Rate+1));
   adc_par.t1.FPDelay=(USHORT)(DSP_CLOCK_OUT_PLX/(ap->t1.dRate)+5.5);

   // ЧЕМЙЮЙОБ ЪБДЕТЦЛЙ Ч ЕДЙОЙГБИ SCLOCK SPORT DSP
   if((1.0/(ap->t1.dRate)) > (ap->t1.dKadr)) ap->t1.dKadr=1.0/(ap->t1.dRate);
   //
   double kadrdelay=(ap->t1.dKadr)*(ap->t1.dRate)-0.5;
   if(kadrdelay > 65534.0) kadrdelay=65534.0;
   adc_par.t1.Kadr = (USHORT)kadrdelay;
   ap->t1.dKadr=(adc_par.t1.Kadr+1)/(ap->t1.dRate);

   adc_par.t1.Scale = 0;

// More
   adc_par.t1.SynchroType=ap->t1.SynchroType;
   adc_par.t1.SynchroSensitivity=ap->t1.SynchroSensitivity;
   adc_par.t1.SynchroMode=ap->t1.SynchroMode;
   adc_par.t1.AdChannel=ap->t1.AdChannel;
   adc_par.t1.AdPorog=ap->t1.AdPorog;

   if(ap->t1.FIFO < 0x40) ap->t1.FIFO = 0x40;
   else if(ap->t1.FIFO > 0x3000) ap->t1.FIFO = 0x3000;
   ap->t1.FIFO -= (USHORT)(ap->t1.FIFO%0x40);

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


FDF(ULONG) DaqE440::ReadPlataDescr(LPVOID pd)
{
   PPLATA_DESCR_U pdt = (PPLATA_DESCR_U)pd;

   for(USHORT i=0; i<sizeof(PLATA_DESCR_U)/2; i++)
      if(ReadFlashWord(i,&pdu.wi.data[i])) return L_ERROR;
   memcpy(pd,&pdu,sizeof(PLATA_DESCR_U));
   return L_SUCCESS;
}



FDF(ULONG) DaqE440::WritePlataDescr(LPVOID pd, USHORT Ena)
{
   PPLATA_DESCR_U pdt = (PPLATA_DESCR_U)pd;

   memcpy(&pdu,pd,sizeof(PLATA_DESCR_U));
   if(EnableFlashWrite(1)) return L_ERROR;
   ULONG tmp=(!Ena)*32;
   for(USHORT i=(USHORT)tmp; i<sizeof(PLATA_DESCR_U)/2; i++)
      if(WriteFlashWord(i,pdu.wi.data[i])) return L_ERROR;
   if(EnableFlashWrite(0)) return L_ERROR;
   return L_SUCCESS;
}


//FDF(ULONG) DaqL780::InputADC(USHORT Chan, PUSHORT Data)
ULONG DaqE440::InputADC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG tmp;
   if(PutWord_PM(L_ADC_CHANNEL_E440, LBIOS_OUTVAR(ap->Chn[0]))) return L_ERROR;
   if(SendCommand(cmADC_SAMPLE_E440)) return L_ERROR;
   if(GetWord_PM(L_ADC_SAMPLE_E440, &tmp)) return L_ERROR;
   ap->Data[0] = LBIOS_INVAR(tmp);
   return L_SUCCESS;
}

ULONG DaqE440::ConfigTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   USHORT tmp = (USHORT)ap->Mode;
   if(PutWord_PM(L_ENABLE_TTL_OUT_E440, LBIOS_OUTVAR(tmp))) return L_ERROR;
   if(SendCommand(cmENABLE_TTL_OUT_E440)) return L_ERROR;
   return L_SUCCESS;
}


//FDF(ULONG) DaqL780::InputTTL(PULONG Data, ULONG Mode)
ULONG DaqE440::InputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   ULONG tmp;
   if(SendCommand(cmTTL_IN_E440)) return L_ERROR;
   if(GetWord_PM(L_TTL_IN_E440,&tmp)) return L_ERROR;
   ap->Data[0] = LBIOS_INVAR(tmp);
   return L_SUCCESS;
}


//FDF(ULONG) DaqL780::OutputTTL(ULONG Data, ULONG Mode)
ULONG DaqE440::OutputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(PutWord_PM(L_TTL_OUT_E440,LBIOS_OUTVAR(ap->Data[0]))) return L_ERROR;
   if(SendCommand(cmTTL_OUT_E440)) return L_ERROR;
   return L_SUCCESS;
}


FDF(ULONG) DaqE440::EnableCorrection(USHORT Ena)
{
   for(int i = 0; i < 4; i++)
   {
      if(PutWord_PM(L_ZERO_E440+i, LBIOS_OUTVAR(pdu.t4.KoefADC[i]))) return L_ERROR;
      if(PutWord_PM(L_SCALE_E440+i, LBIOS_OUTVAR(pdu.t4.KoefADC[4+i]))) return L_ERROR;
   }
// enable or disable
   if(PutWord_PM(L_CORRECTION_ENABLED_E440, LBIOS_OUTVAR(Ena))) return L_ERROR;
   return L_SUCCESS;
}

//FDF(ULONG) DaqL780::OutputDAC(short Data, ULONG Mode)
ULONG DaqE440::OutputDAC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(ap->Mode>1) return L_ERROR;
   USHORT dac_value =(USHORT)((ap->Data[0])&0xFFF);
   dac_value |=(ap->Mode<<12);
   if(PutWord_PM(L_DAC_SAMPLE_E440, LBIOS_OUTVAR(dac_value))) return L_ERROR;
   if(SendCommand(cmDAC_SAMPLE_E440)) return L_ERROR;
   return L_SUCCESS;
}


/*
//FDF(ULONG)
ULONG DaqL780::FillDACparameters(PDAQ_PAR sp)
{
   PDAC_PAR dp = (PDAC_PAR)sp;

   double DSP_CLOCK_OUT_PLX=2.0*pdu.t1.Quartz/1000.0;
   USHORT d1;
   if(GetWord_DM(L_DAC_SCLK_DIV_PLX, &d1)) return L_ERROR;
   double SCLK=DSP_CLOCK_OUT_PLX/(2.*(d1+1.));

   dp->t1.dRate=fabs(dp->t1.dRate);
   if(dp->t1.dRate>125.0) dp->t1.dRate=125.0;
   if(dp->t1.dRate < SCLK/65535.0) dp->t1.dRate=SCLK/65535.0;

   USHORT RFS_DIV=(USHORT)(SCLK/(dp->t1.dRate)-0.5);
   dp->t1.dRate=SCLK/(RFS_DIV+1.0);

   dac_par.t1.Rate=RFS_DIV;
   dac_par.t1.FIFO = dp->t1.FIFO;
   dac_par.t1.IrqStep = (dp->t1.IrqStep!=dp->t1.FIFO) ? dp->t1.FIFO:dp->t1.IrqStep;
   dac_par.t1.Pages = (dp->t1.Pages<2) ? 2:dp->t1.Pages;
   dac_par.t1.AutoInit = dp->t1.AutoInit;
   dac_par.t1.DacEna = dp->t1.DacEna;
   dac_par.t1.DacNumber = dp->t1.DacNumber;
   dac_par.t1.IrqEna = dp->t1.IrqEna;

   return L_SUCCESS;
}
*/
////////////////////////////////////////////////////////////////////////////////
// рТПГЕДХТБ ЮФЕОЙС УМПЧБ ЙЪ РПМШЪПЧБФЕМШУЛПЗП рръх
////////////////////////////////////////////////////////////////////////////////
FDF(ULONG) DaqE440::ReadFlashWord(USHORT FlashAddress, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = FlashAddress;
   return !LDeviceIoControl( hVxd, DIOC_READ_FLASH_WORD,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE440::WriteFlashWord(USHORT FlashAddress, USHORT Data)
{
   ULONG cbRet;
   USHORT par = FlashAddress;
   return !LDeviceIoControl( hVxd, DIOC_WRITE_FLASH_WORD,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqE440::EnableFlashWrite(USHORT Flag)
{
   ULONG cbRet;
   USHORT par = 0; // addr not use
   return !LDeviceIoControl( hVxd, DIOC_ENABLE_FLASH_WRITE,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Flag, sizeof(USHORT),
                           &cbRet, NULL
                         );
}
