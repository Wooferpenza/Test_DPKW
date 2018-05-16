#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <pthread.h>

#include <iostream>

#include <math.h>

using namespace std;

#include <termios.h>

static struct termios stored_settings,new_settings;
static int peek_character = -1;

static int ctrlc = 0;

void reset_keypress(void)
{
   ctrlc=1;
   tcsetattr(0,TCSANOW,&stored_settings);
   return;
}


void set_keypress(void)
{
   tcgetattr(0,&stored_settings);

   new_settings = stored_settings;

   /* Disable canonical mode, and set buffer size to 1 byte */
   new_settings.c_lflag &= (~ICANON);
   new_settings.c_lflag &= ~ECHO;
   new_settings.c_lflag &= ~ISIG;
   new_settings.c_cc[VTIME] = 0;
   new_settings.c_cc[VMIN] = 1;

   atexit(reset_keypress);
   tcsetattr(0,TCSANOW,&new_settings);
   return;
}


int kbhit()
{
unsigned char ch;
int nread;

    if (peek_character != -1) return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1)
    {
        peek_character = ch;
        return 1;
    }
    return 0;
}

int readch()
{
char ch;

    if(peek_character != -1)
    {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }
    read(0,&ch,1);
    return ch;
}



#define INITGUID

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/e2010cmd.h"
#include "../include/791cmd.h"
#include "../include/ifc_ldev.h"
#include <errno.h>

typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);

CREATEFUNCPTR CreateInstance;


unsigned short *p;
//unsigned long *pl;
unsigned int *pp;

unsigned int *dp;
unsigned int *dpp;

int IrqStep=1024;
int  pages=256;
int  multi=32;
unsigned short complete;



void *thread_func(void *arg)
{
int halfbuffer;
int fl2, fl1;
unsigned short *tmp, *tmp1;
int i;

FILE *fd;

   fd=fopen("test.dat","wb");

   halfbuffer =IrqStep*pages/2;
   fl1=fl2= (*pp<=halfbuffer) ? 0:1;

   for(i=0;i<multi;i++)
   {
      while(fl2==fl1) { fl2=(*pp<=halfbuffer) ? 0:1; if(ctrlc) break; usleep(10);}
      if(ctrlc) break;
      tmp1=p+(halfbuffer*fl1);
      fwrite(tmp1,1,halfbuffer*sizeof(short),fd);
//    ((IDaqLDevice *)arg)->PlataTest();
      fl1=(*pp<=halfbuffer)? 0:1;
   }

   fclose(fd);
   complete=1;
}

void *thread_func791(void *arg)
{
int halfbuffer;
int fl2, fl1;
unsigned int *tmp, *tmp1;
int i;

FILE *fd;

   fd=fopen("test.dat","wb");

   halfbuffer =IrqStep*pages/2;
   fl1=fl2= (pp[I_ADC_PCI_COUNT_L791]<=halfbuffer) ? 0:1;

   for(i=0;i<multi;i++)
   {
      while(fl2==fl1) { fl2=(pp[I_ADC_PCI_COUNT_L791]<=halfbuffer) ? 0:1; usleep(10);}
      tmp1=(unsigned int *)p+(halfbuffer*fl1);
      fwrite(tmp1,1,halfbuffer*sizeof(int),fd);
//    ((IDaqLDevice *)arg)->PlataTest();
      fl1=(pp[I_ADC_PCI_COUNT_L791]<=halfbuffer)? 0:1;
   }

   fclose(fd);
   complete=1;
}



//Att. for board slot numbers!!!!

int main(int argc, char **argv)
{
PLATA_DESCR_U2 pd;
SLOT_PAR sl;
ADC_PAR adcPar;
DAC_PAR dacPar;
ULONG size;
IOCTL_BUFFER ibuf;
HANDLE hVxd;
void *handle;

char *error;
pthread_t thread1;

   set_keypress();
// тестируем размерности типов в Linux
/*
    cout << "char " << sizeof(char) << endl;
    cout << "void * " << sizeof(void *) << endl;
    cout << "long " << sizeof(long) << endl;
    cout << "unsigned long " << sizeof(unsigned long) << endl;
    cout << "int " << sizeof(int) << endl;
    cout << "unsigned int " << sizeof(unsigned int) << endl;
    cout << "short " << sizeof(short) << endl;
    cout << "unsigned short" << sizeof(unsigned short) << endl;
    cout << "long long " << sizeof(long long) << endl;
    cout << "long int " << sizeof(long int) << endl;
    cout << "unsigned " << sizeof(unsigned) << endl;
    ULONG32 buf[4];
    cout << "array size" << sizeof(buf) << endl;
    exit(0);
*/   

   if(argc==1)
   {
      cout << "L-7XX simple example (L761,L780,L783,L791,E440,E140,E2010)." << endl;
      cout << "(c) 2007 L-Card." << endl;
      cout << "Usage:" << endl;
      cout << "   client <slot number> <bios name>" << endl;
      cout << "   <slot number> - 0... (if one board installed - 0);" << endl;
      cout << "   <bios name>  - bios name without extension(l761 l780 l783 e2010 e440);" << endl;
      reset_keypress();      
      exit(0);
   }
   if(argc<3)
   {
      cout << "no params - exit!" << endl;
      reset_keypress();
      exit(0);
   }


   handle = dlopen("./liblcomp.so",RTLD_LAZY);
   if(!handle)
   {
      cout << "error open dll!! " << dlerror() << endl;
   }

   CreateInstance =(CREATEFUNCPTR) dlsym(handle,"CreateInstance");
   if((error = dlerror())!=NULL)
   {
      cout << error << endl;
   }

   LUnknown* pIUnknown = CreateInstance(atoi(argv[1]));
         cout << errno << endl;
   if(pIUnknown == NULL) { cout << "CallCreateInstance failed " << endl; reset_keypress(); return 1; }

   cout << "Get IDaqLDevice interface" << endl;
   IDaqLDevice* pI;
   HRESULT hr = pIUnknown->QueryInterface(IID_ILDEV,(void**)&pI);
   if(hr!=S_OK) { cout << "Get IDaqLDevice failed" << endl; reset_keypress();return 1; }
   printf("IDaqLDevice get success \n");
   pIUnknown->Release();
   cout << "Free IUnknown" << endl;

   cout << "OpenLDevice Handle" << hex << (hVxd=pI->OpenLDevice()) << endl;

   cout << endl << "Slot parameters" << endl;
   pI->GetSlotParam(&sl);

   cout << "Base    " << hex << sl.Base << endl;
   cout << "BaseL   " << sl.BaseL << endl;
   cout << "Mem     " << sl.Mem << endl;
   cout << "MemL    " << sl.MemL << endl;
   cout << "Type    " << sl.BoardType << endl;
   cout << "DSPType " << sl.DSPType << endl;
   cout << "Irq     " << sl.Irq << endl;

   cout << "Load Bios " << pI->LoadBios(argv[2]) << endl;
   cout << "Plata Test " << pI->PlataTest() << endl;

   cout << endl << "Read FLASH" << endl;

   pI->ReadPlataDescr(&pd); // fill up properties
   switch (sl.BoardType)
   {
   case PCIA:
   case PCIB:
   case PCIC:  {
                  cout << "SerNum       " << pd.t1.SerNum << endl;
                  cout << "BrdName      " << pd.t1.BrdName << endl;
                  cout << "Rev          " << pd.t1.Rev << endl;
                  cout << "DspType      " << pd.t1.DspType << endl;
                  cout << "IsDacPresent " << pd.t1.IsDacPresent << endl;
                  cout << "Quartz       " << dec << pd.t1.Quartz << endl;
               } break;
   case E140:  {
                  cout << "SerNum       " << pd.t5.SerNum << endl;
                  cout << "BrdName      " << pd.t5.BrdName << endl;
                  cout << "Rev          " << pd.t5.Rev << endl;
                  cout << "DspType      " << pd.t5.DspType << endl;
                  cout << "IsDacPresent " << pd.t5.IsDacPresent << endl;
                  cout << "Quartz       " << dec << pd.t5.Quartz << endl;
               } break;
   case E440:  {
                  cout << "SerNum       " << pd.t4.SerNum << endl;
                  cout << "BrdName      " << pd.t4.BrdName << endl;
                  cout << "Rev          " << pd.t4.Rev << endl;
                  cout << "DspType      " << pd.t4.DspType << endl;
                  cout << "IsDacPresent " << pd.t4.IsDacPresent << endl;
                  cout << "Quartz       " << dec << pd.t4.Quartz << endl;
               } break;
   case E2010B:
   case E2010: {
                  cout << "SerNum       " << pd.t6.SerNum << endl;
                  cout << "BrdName      " << pd.t6.BrdName << endl;
                  cout << "Rev          " << pd.t6.Rev << endl;
                  cout << "DspType      " << pd.t6.DspType << endl;
                  cout << "IsDacPresent " << pd.t6.IsDacPresent << endl;
                  cout << "Quartz       " << dec << pd.t6.Quartz << endl;
               } break;

   case L791: {
                  cout << "SerNum       " << pd.t3.SerNum << endl;
                  cout << "BrdName      " << pd.t3.BrdName << endl;
                  cout << "Rev          " << pd.t3.Rev << endl;
                  cout << "DspType      " << pd.t3.DspType << endl;
                  cout << "IsDacPresent " << pd.t3.IsDacPresent << endl;
                  cout << "Quartz       " << dec << pd.t3.Quartz << endl;
               } break;
   case E154: {
                  cout << "SerNum       " << pd.t7.SerNum << endl;
                  cout << "BrdName      " << pd.t7.BrdName << endl;
                  cout << "Rev          " << pd.t7.Rev << endl;
                  cout << "DspType      " << pd.t7.DspType << endl;
                  cout << "IsDacPresent " << pd.t7.IsDacPresent << endl;
                  cout << "Quartz       " << dec << pd.t7.Quartz << endl;
               } break;

   }


      
   cout << endl << "Press any key" << dec << endl;
   
   //exit(0);
   
   readch();

   size=131072;

   pI->RequestBufferStream(&size);

   cout << " alloc size " <<  size << endl;

   switch(sl.BoardType)
   {
   case PCIA:
   case PCIB:
   case PCIC:
   case E440:
   case E140:
   case E154:
      {
         adcPar.t1.s_Type = L_ADC_PARAM;
         adcPar.t1.AutoInit = 1;
         adcPar.t1.dRate = 100.0;
         adcPar.t1.dKadr = 0;
         adcPar.t1.dScale = 0;
         adcPar.t1.SynchroType = 3;
         if(sl.BoardType==E440 || sl.BoardType==E140 || sl.BoardType==E154) adcPar.t1.SynchroType = 0;
         adcPar.t1.SynchroSensitivity = 0;
         adcPar.t1.SynchroMode = 0;
         adcPar.t1.AdChannel = 0;
         adcPar.t1.AdPorog = 0;
         adcPar.t1.NCh = 4;
         adcPar.t1.Chn[0] = 0x0;
         adcPar.t1.Chn[1] = 0x1;
         adcPar.t1.Chn[2] = 0x2;
         adcPar.t1.Chn[3] = 0x3;
         adcPar.t1.FIFO = IrqStep;
         adcPar.t1.IrqStep = IrqStep;
         adcPar.t1.Pages = pages;
         if(sl.BoardType==E440 || sl.BoardType==E140 || sl.BoardType==E154)
         {
            adcPar.t1.FIFO = 4096;
            adcPar.t1.IrqStep = 4096;
            adcPar.t1.Pages = 32;
         }
         adcPar.t1.IrqEna = 1;
         adcPar.t1.AdcEna = 1;

         pI->FillDAQparameters(&adcPar.t1);
         pI->SetParametersStream(&adcPar.t1, &size, (void **)&p, (void **)&pp,L_STREAM_ADC);

         cout << "Buffer size(word): " << size << endl;
         cout << "Pages:             " << adcPar.t1.Pages << endl;
         cout << "IrqStep:           " << adcPar.t1.IrqStep << endl;
         cout << "FIFO:              " << adcPar.t1.FIFO << endl;
         cout << "Rate:              " << adcPar.t1.dRate << endl;

         IrqStep = adcPar.t1.IrqStep;
         pages = adcPar.t1.Pages;

      } break;

   case E2010B:
   case E2010:
      {
         adcPar.t2.s_Type = L_ADC_PARAM;
         adcPar.t2.AutoInit = 1;
         adcPar.t2.dRate = 10000.0;
         adcPar.t2.dKadr = 0.001;
         adcPar.t2.SynchroType = 0x01;
         adcPar.t2.SynchroSrc = 0x00;
         adcPar.t2.AdcIMask = SIG_0|SIG_1|SIG_2|SIG_3;

         adcPar.t2.NCh = 4;
         adcPar.t2.Chn[0] = 0x0;
         adcPar.t2.Chn[1] = 0x1;
         adcPar.t2.Chn[2] = 0x2;
         adcPar.t2.Chn[3] = 0x3;
         adcPar.t2.FIFO = 32768;
         adcPar.t2.IrqStep = 32768;
         adcPar.t2.Pages = 8;
         adcPar.t2.IrqEna = 1;
         adcPar.t2.AdcEna = 1;

         // extra sync mode
         adcPar.t2.StartCnt = 0;
         adcPar.t2.StopCnt = 0;
         adcPar.t2.DM_Ena = 0;
         adcPar.t2.SynchroMode = 0;
         adcPar.t2.AdPorog = 0;


         pI->FillDAQparameters(&adcPar.t2);
         
         cout << "Buffer size(word): " << size << endl;
         cout << "Pages:             " << adcPar.t2.Pages << endl;
         cout << "IrqStep:           " << adcPar.t2.IrqStep << endl;
         cout << "FIFO:              " << adcPar.t2.FIFO << endl;
         cout << "Rate:              " << adcPar.t2.dRate << endl;
         cout << "Kadr:              " << adcPar.t2.dKadr << endl;
         
         pI->SetParametersStream(&adcPar.t2, &size, (void **)&p, (void **)&pp,L_STREAM_ADC);

         cout << "Buffer size(word): " << size << endl;
         cout << "Pages:             " << adcPar.t2.Pages << endl;
         cout << "IrqStep:           " << adcPar.t2.IrqStep << endl;
         cout << "FIFO:              " << adcPar.t2.FIFO << endl;
         cout << "Rate:              " << adcPar.t2.dRate << endl;
         cout << "Kadr:              " << adcPar.t2.dKadr << endl;
         IrqStep = adcPar.t2.IrqStep;
         pages = adcPar.t2.Pages;
      } break;

   case L791:
      {
         // заполняем структуру  с описанием параметров сбора данных с АЦП
         adcPar.t2.s_Type = L_ADC_PARAM;
         adcPar.t2.AutoInit = 1;
         adcPar.t2.dRate = 200.0;
         adcPar.t2.dKadr = .01;

         adcPar.t2.SynchroType = 0;
         adcPar.t2.SynchroSrc = 0;

         adcPar.t2.NCh = 4;
         adcPar.t2.Chn[0] = 0x0;
         adcPar.t2.Chn[1] = 0x1;
         adcPar.t2.Chn[2] = 0x2;
         adcPar.t2.Chn[3] = 0x3;

         adcPar.t2.FIFO = IrqStep;

         adcPar.t2.IrqStep = IrqStep;
         adcPar.t2.Pages = 64;
         adcPar.t2.IrqEna = 3;  // работает без прерываний
         adcPar.t2.AdcEna = 1;  // разрешаем АЦП
         // можно прерывания разрешить тогда будет генерироваться событие см OSC_L791.TST
         //
         pI->FillDAQparameters(&adcPar.t2);
         pI->SetParametersStream(&adcPar.t2, &size, (void **)&p, (void **)&pp,L_STREAM_ADC);

         cout << "L791 Buffer size(word): " << size << endl;
         cout << "Pages:             " << adcPar.t2.Pages << endl;
         cout << "IrqStep:           " << adcPar.t2.IrqStep << endl;
         cout << "FIFO:              " << adcPar.t2.FIFO << endl;
         cout << "Rate:              " << adcPar.t2.dRate << endl;

         IrqStep = adcPar.t2.IrqStep;
         pages = adcPar.t2.Pages;
      }
   }

   ULONG Ver = pp[0xFF4>>2];
   cout << endl << "Current Firmware Version 0x" << hex << Ver << dec << endl;
         

   cout << endl << "Press any key" << dec << endl;

   readch();

   complete=0;

   pI->EnableCorrection();

   pI->InitStartLDevice();

   if(sl.BoardType==L791) pthread_create(&thread1, NULL, thread_func791, pI);
   else  pthread_create(&thread1, NULL, thread_func, pI);

/*
   FILE *fd;
   fd=fopen("test.dat","wb");
*/
   pI->StartLDevice();
/*
   ioctl(hVxd,DIOC_WAIT_COMPLETE,&ibuf);

   fwrite(p,1,3*2048*sizeof(short),fd);
   fclose(fd);
*/

   while(!complete)//!kbhit())
   {
      if(kbhit()) break;
//    ioctl(hvxd,DIOC_WAIT_COMPLETE,&ibuf);
      if(sl.BoardType==L791) printf(" shared word %x %x \n", pp[I_ADC_PCI_COUNT_L791], complete);
      else printf(" shared word %x %x \n", pp[0], complete);
//      else printf(" shared word %x %x \n", *pp, p[(*pp-1)]);
      usleep(40000);
   }


   pthread_join(thread1,NULL);

   cout << endl << "Press any key" << dec << endl;
   readch();

   pI->StopLDevice();
   pI->CloseLDevice();

   reset_keypress();

   if(handle) dlclose(handle);
   return 0;
}
