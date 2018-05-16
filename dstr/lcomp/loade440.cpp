#include <stdio.h>
#include <string.h>

#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/e440cmd.h"

#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/e440.h"

FDF(ULONG) DaqE440::LoadBios(char *FileName)
{
USHORT  *LCBios;
FILE    *BiosFile;
LONG    NBytes;
PUCHAR  BiosCode=0;
USHORT  *Tmp;
USHORT  Count;
CHAR    FName[255];

ULONG cbRet;
USHORT InBuf[] = {0,V_RESET_DSP_E440,0,0};

ULONG *Buffer;
int i;

   int status = L_ERROR;
   
   do
   {
      strcpy(FName,FileName);
      strcat(FName,".bio");

      BiosFile=fopen(FName, "rb");
      if(!BiosFile) break;
      fseek(BiosFile,0,SEEK_END);
      NBytes = ftell(BiosFile);
      rewind(BiosFile);

      BiosCode=new UCHAR[NBytes+2];
      if(BiosCode==NULL) break;
      if(fread(BiosCode,1,NBytes,BiosFile) != NBytes) break;
      LCBios=(PUSHORT)BiosCode;
      // reset DSP //////////////////////////////////////////////////////////////
      if(!LDeviceIoControl( hVxd, DIOC_SEND_COMMAND,
                           &InBuf, sizeof(InBuf),
                           NULL, 0,
                           &cbRet, NULL)) break;
      ///////////////////////////////////////////////////////////////////////////
      // загрузим память данных DSP
      Tmp=LCBios+LCBios[0]+1; // calculate DM address &LCBios[0]+LCBios[0]
      Count=*Tmp++;           // counter
      if(PutArray_DM(0x0000, Count, Tmp)) break;
      // загрузим память программ DSP, начиная с адреса 0х1
      Buffer = (PULONG)(&LCBios[1]);

      for(i=0; i < LCBios[0]/2; i++) Buffer[i] = ((Buffer[i] & 0x0000FFFF) << 8) | ((Buffer[i] & 0x00FF0000) >> 16);

      if(PutArray_PM(0x0001, LCBios[0]/2-1, &Buffer[1])) break;

      // загрузим нулевую ячейку памяти программ DSP, т.е. стартанем LBIOS
      if(PutArray_PM(0x0000, 1, &Buffer[0])) break;
      status = L_SUCCESS; // all ok
   } while (status==L_ERROR);

   if(BiosCode) delete[] BiosCode;
   if(BiosFile) fclose(BiosFile);
   return status;//E;

}
