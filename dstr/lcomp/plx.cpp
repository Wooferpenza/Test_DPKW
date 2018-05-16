#include <string.h>
#include <stdio.h>

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/pcicmd.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/plx.h"


// IDMA with  PLX9050 PCI chip /////////////////////////////////////////////////
FDF(ULONG) DaqL780::GetWord_DM(USHORT Addr, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_DM_A,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL780::PutWord_DM(USHORT Addr, USHORT Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_DM_A,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL780::SendCommand(USHORT Cmd)
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

FDF(ULONG) DaqL780::PutWord_PM(USHORT Addr, ULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_PUT_PM_A,
                           &par, sizeof(par),
                           &Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL780::GetWord_PM(USHORT Addr, PULONG Data)
{
   ULONG cbRet;
   USHORT par = Addr;
   return !LDeviceIoControl( hVxd, DIOC_GET_PM_A,
                           &par, sizeof(par),
                           Data, sizeof(ULONG),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL780::PutArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
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

FDF(ULONG) DaqL780::GetArray_DM(USHORT Addr, ULONG Count, PUSHORT Data)
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

FDF(ULONG) DaqL780::PutArray_PM(USHORT Addr, ULONG Count, PULONG Data)
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

FDF(ULONG) DaqL780::GetArray_PM(USHORT Addr, ULONG Count, PULONG Data)
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

FDF(ULONG) DaqL780::PlataTest()
{
   USHORT d1; if(GetWord_DM(L_TMODE1_PLX, &d1)) return L_ERROR;
   USHORT d2; if(GetWord_DM(L_TMODE2_PLX, &d2)) return L_ERROR;
   if((d1!=0x5555)||(d2!=0xAAAA)) return L_ERROR;
   else
   {
      if(PutWord_DM(L_TEST_LOAD_PLX, 0x77bb)) return L_ERROR;
      int TO=10000000; //////////TimeOUT; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      do
      {
         if(GetWord_DM(L_READY_PLX, &d1)) return L_ERROR;
      } while(!d1&&(TO--));
      if(TO==-1) return L_ERROR;
      if(SendCommand(cmTEST_PLX)) return L_ERROR;
      if(GetWord_DM(L_TEST_LOAD_PLX,&d1)) return L_ERROR;
      if(d1!= 0xAA55) return L_ERROR;
   }
   return L_SUCCESS;
}


//FDF(ULONG)
ULONG DaqL780::FillADCparameters(PDAQ_PAR sp)
{
enum { L761,L780,L783 };
char *brdname[]={"L761","L780","L783"};
double max_rate[] = { 125.0, 400.0, 3300.0 };
ULONG i;
double QF;
double DSP_CLOCK_OUT_PLX;
int BN;
double SCLOCK_DIV;
double kadrdelay;
double delta_rate;
double rate;

   PADC_PAR ap = (PADC_PAR)sp;

   if(ap->t1.dRate<0) return L_ERROR;
   if(ap->t1.dKadr<0) return L_ERROR;

   if(ap->t1.FIFO==0) return L_ERROR;
   if(ap->t1.Pages==0) return L_ERROR;
   if(ap->t1.IrqStep==0) return L_ERROR;


   QF=pdu.t1.Quartz/1000.0;
   DSP_CLOCK_OUT_PLX=2.0*QF;
   if(DSP_CLOCK_OUT_PLX == 0) return L_ERROR;

   // Type of board
   for(BN=0; BN<3; BN++) if(!strncmp(pdu.t1.BrdName,brdname[BN],4)) break;

   if(ap->t1.dRate < 0.1) ap->t1.dRate=0.1;
   if(ap->t1.dRate > max_rate[BN]) ap->t1.dRate=max_rate[BN];

   switch(BN)
   {
      case L780:
      case L783:
      {
         // частота сбора в единицах SCLOCK_DIV SPORT DSP
         SCLOCK_DIV=DSP_CLOCK_OUT_PLX/(2.0*(ap->t1.dRate))-0.5;
         if(SCLOCK_DIV > 65500.0) SCLOCK_DIV=65500.0;

         adc_par.t1.Rate = (USHORT)SCLOCK_DIV;

         ap->t1.dRate=DSP_CLOCK_OUT_PLX/(2.0*(adc_par.t1.Rate+1));

         adc_par.t1.FPDelay=(USHORT)(DSP_CLOCK_OUT_PLX/(ap->t1.dRate)+5.5);

         // величина задержки в единицах SCLOCK SPORT DSP
         if(ap->t1.dRate > 1000.0) ap->t1.dKadr=0; //  no interkadr on freq up 1000 kHz

         if((1.0/(ap->t1.dRate)) > (ap->t1.dKadr)) ap->t1.dKadr=1.0/(ap->t1.dRate);
         //
         kadrdelay=(ap->t1.dKadr)*(ap->t1.dRate)-0.5;
         if(kadrdelay > 65500.0) kadrdelay=65500.0;

         adc_par.t1.Kadr = (USHORT)kadrdelay;

         ap->t1.dKadr=(adc_par.t1.Kadr+1)/(ap->t1.dRate);

      } break;

      case L761:
      {
         delta_rate=0.1;        // частота с точностью 0.1 мкс
         // частота сбора в единицах delta_rate
         rate=1000./((ap->t1.dRate)*delta_rate)+0.5;
         if(rate > 65500.0) rate=65500.0;

         adc_par.t1.Rate=(USHORT)rate;

         ap->t1.dRate=1000.0/(adc_par.t1.Rate*delta_rate);

         adc_par.t1.FPDelay=(USHORT)(DSP_CLOCK_OUT_PLX/(ap->t1.dRate)+5.5);

         // установим величину межкадровой задержки в млс
         if((1.0/(ap->t1.dRate)) > (ap->t1.dKadr)) ap->t1.dKadr=1.0/(ap->t1.dRate);
         if(ap->t1.dKadr > 0.1*65535.0/1000.0) ap->t1.dKadr=0.1*65535.0/1000.0;

         kadrdelay=1000.0*(ap->t1.dKadr)/delta_rate+0.5;    // величина задержки  в единицах delta_rate

         adc_par.t1.Kadr=(USHORT)kadrdelay;

         ap->t1.dKadr=adc_par.t1.Kadr*delta_rate/1000.0; //-1./(*Rate);
      }
   }


   adc_par.t1.Scale = 0;

// More
   adc_par.t1.s_Type = ap->t1.s_Type;
   adc_par.t1.SynchroType=ap->t1.SynchroType;
   adc_par.t1.SynchroSensitivity=ap->t1.SynchroSensitivity;
   adc_par.t1.SynchroMode=ap->t1.SynchroMode;
   adc_par.t1.AdChannel=ap->t1.AdChannel;
   adc_par.t1.AdPorog=ap->t1.AdPorog;

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

FDF(ULONG) DaqL780::ReadPlataDescr(LPVOID pd)
{
   PPLATA_DESCR_U pdt = (PPLATA_DESCR_U)pd;

   for(USHORT i=0; i<sizeof(PLATA_DESCR_U)/2; i++)
   {
      if(ReadFlashWord(i,&pdu.wi.data[i])) return L_ERROR;
   }
   memcpy(pd,&pdu,sizeof(PLATA_DESCR_U));
   return L_SUCCESS;
}


FDF(ULONG) DaqL780::WritePlataDescr(LPVOID pd, USHORT Ena)
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
ULONG DaqL780::InputADC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   USHORT data;
   if(PutWord_DM(L_ADC_CHANNEL_PLX, (USHORT)ap->Chn[0])) return L_ERROR;
   if(SendCommand(cmADC_SAMPLE_PLX)) return L_ERROR;
   if(GetWord_DM(L_ADC_SAMPLE_PLX, &data)) return L_ERROR;
   ap->Data[0]=data;
   return L_SUCCESS;
}

ULONG DaqL780::ConfigTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;

   if(sl.BoardType!=PCIC) return L_NOTSUPPORTED;

   if(PutWord_DM(L_ENABLE_TTL_OUT_PLX, (USHORT)ap->Mode)) return L_ERROR;
   if(SendCommand(cmENABLE_TTL_OUT_PLX)) return L_ERROR;
   return L_SUCCESS;
}


//FDF(ULONG) DaqL780::InputTTL(PULONG Data, ULONG Mode)
ULONG DaqL780::InputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   USHORT data;
   if(SendCommand(cmTTL_IN_PLX)) return L_ERROR;
   if(GetWord_DM(L_TTL_IN_PLX,&data)) return L_ERROR;
   ap->Data[0] = data;
   return L_SUCCESS;
}


//FDF(ULONG) DaqL780::OutputTTL(ULONG Data, ULONG Mode)
ULONG DaqL780::OutputTTL(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(PutWord_DM(L_TTL_OUT_PLX,(USHORT)ap->Data[0])) return L_ERROR;
   if(SendCommand(cmTTL_OUT_PLX)) return L_ERROR;
   return L_SUCCESS;
}


FDF(ULONG) DaqL780::EnableCorrection(USHORT Ena)
{
// load
   if(PutArray_DM(L_ZERO_PLX, 4, &(pdu.t1.KoefADC[0]))) return L_ERROR;
   if(PutArray_DM(L_SCALE_PLX, 4, &(pdu.t1.KoefADC[4]))) return L_ERROR;
// enable or disable
   if(PutWord_DM(L_CORRECTION_ENABLE_PLX, Ena)) return L_ERROR;
   return L_SUCCESS;
}

//FDF(ULONG) DaqL780::OutputDAC(short Data, ULONG Mode)
ULONG DaqL780::OutputDAC(PDAQ_PAR sp)
{
   PASYNC_PAR ap = (PASYNC_PAR)sp;
   if(ap->Mode>1) return L_ERROR;
   USHORT dac_value =(USHORT)((ap->Data[0])&0xFFF);
   dac_value |=(ap->Mode<<12);
   dac_value |=(1<<15); // for 783 priznak
   if(PutWord_DM(L_DAC_VALUE_PLX, dac_value)) return L_ERROR;

   if(!strcmp(pdu.t1.BrdName,"L783")) // поставил !
   {
  /*    USHORT tmp;
      ULONG TO=1000000; // timeout
      do
      if(GetWord_DM(L_DAC_VALUE_PLX,&tmp)) return L_ERROR;
      while((tmp&(1<<15))&&TO--);
      if(TO) return L_SUCCESS; else return L_ERROR;
*/
      if(SendCommand(0)) return L_ERROR;
   }

   //if(SendCommand(0)) return L_ERROR;
   USHORT tmp;
   ULONG TO=1000000; // timeout
   do
   if(GetWord_DM(L_DAC_VALUE_PLX,&tmp)) return L_ERROR;
   while((tmp&(1<<15))&&TO--);
   if(TO) return L_SUCCESS; else return L_ERROR;

//   return L_SUCCESS;
}


//FDF(ULONG)
ULONG DaqL780::FillDACparameters(PDAQ_PAR sp)
{
   PDAC_PAR dp = (PDAC_PAR)sp;

   double DSP_CLOCK_OUT_PLX=2.0*pdu.t1.Quartz/1000.0;
   USHORT d1;
   if(GetWord_DM(L_DAC_SCLK_DIV_PLX, &d1)) return L_ERROR;
   double SCLK=DSP_CLOCK_OUT_PLX/(2.*(d1+1.));

   dp->t1.dRate=l_fabs(dp->t1.dRate);
   if(dp->t1.dRate>125.0) dp->t1.dRate=125.0;
   if(dp->t1.dRate < SCLK/65535.0) dp->t1.dRate=SCLK/65535.0;

   USHORT RFS_DIV=(USHORT)(SCLK/(dp->t1.dRate)-0.5);
   dp->t1.dRate=SCLK/(RFS_DIV+1.0);

   dac_par.t1.s_Type=dp->t1.s_Type;
   dac_par.t1.dRate=dp->t1.dRate;
   dac_par.t1.Rate=RFS_DIV;
   dac_par.t1.FIFO = dp->t1.FIFO;
   dac_par.t1.IrqStep = (dp->t1.IrqStep!=dp->t1.FIFO) ? dp->t1.FIFO:dp->t1.IrqStep;
   dac_par.t1.Pages = (dp->t1.Pages<2) ? 2:dp->t1.Pages;
   dac_par.t1.AutoInit = dp->t1.AutoInit;
   dac_par.t1.DacEna = dp->t1.DacEna;
   dac_par.t1.DacNumber = dp->t1.DacNumber;
   dac_par.t1.IrqEna = dp->t1.IrqEna;

   CopyDAQtoWDAQ(&dac_par.t1,&wdac_par.t1,0);

   return L_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Процедура чтения слова из пользовательского ППЗУ
////////////////////////////////////////////////////////////////////////////////
FDF(ULONG) DaqL780::ReadFlashWord(USHORT FlashAddress, PUSHORT Data)
{
   ULONG cbRet;
   USHORT par = FlashAddress;
   return !LDeviceIoControl( hVxd, DIOC_READ_FLASH_WORD,//DIOC_GET_DM_W,
                           &par, sizeof(par),
                           Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL780::WriteFlashWord(USHORT FlashAddress, USHORT Data)
{
   ULONG cbRet;
   USHORT par = FlashAddress;
   return !LDeviceIoControl( hVxd, DIOC_WRITE_FLASH_WORD,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Data, sizeof(USHORT),
                           &cbRet, NULL
                         );
}

FDF(ULONG) DaqL780::EnableFlashWrite(USHORT Flag)
{
   ULONG cbRet;
   USHORT par = 0; // addr not use
   return !LDeviceIoControl( hVxd, DIOC_ENABLE_FLASH_WRITE,//DIOC_PUT_DM_W,
                           &par, sizeof(par),
                           &Flag, sizeof(USHORT),
                           &cbRet, NULL
                         );
}
