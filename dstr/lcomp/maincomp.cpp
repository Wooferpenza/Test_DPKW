#include "../include/stubs.h"

#include "../include/ioctl.h"
#include "../include/ifc_ldev.h"
#include "../include/ldevbase.h"
#include "../include/plx.h"

#ifndef LCOMP_LINUX
#ifndef WIN64
   #include "../include/1450.h"
   #include "../include/1251.h"
   #include "../include/1221.h"
   #include "../include/032.h"
#endif
#endif

#include "../include/791.h"
#include "../include/e440.h"
#include "../include/e140.h"
#include "../include/e2010.h"
#include "../include/e154.h"

#include <stdio.h>

#ifdef LCOMP_LINUX
#include <errno.h>
#endif

void LSetLastError(unsigned long Err)
{
	#ifdef LCOMP_LINUX
		errno = (int)Err;
	#else
		SetLastError(Err);
	#endif
}

unsigned long LGetLastError(void)
{
	#ifdef LCOMP_LINUX
		return errno;
	#else
		return GetLastError();
	#endif
}



// add class for new board here.
// main function of dll
extern "C" LUnknown* CreateInstance(ULONG Slot)
{
   LUnknown* pI;
//   char buf[128];

   LSetLastError(L_SUCCESS);
   SLOT_PAR sl;
   LDaqBoard *pL = new LDaqBoard(Slot);
   if(pL==NULL) { LSetLastError(L_ERROR); return NULL;}
   HANDLE hVxd = pL->OpenLDevice();
   if(hVxd==INVALID_HANDLE_VALUE)
   {
      if(LGetLastError()==ERROR_FILE_NOT_FOUND) LSetLastError(L_ERROR_NOBOARD);
      if(LGetLastError()==ERROR_ACCESS_DENIED) LSetLastError(L_ERROR_INUSE);
      return NULL;
   }

   pL->GetSlotParam(&sl);

   pL->CloseLDevice();
   delete pL;

   switch(sl.BoardType)
   {
   case PCIA:
   case PCIB:
   case PCIC:
      {
         pI = static_cast<IDaqLDevice*>(new DaqL780(Slot));
         pI->AddRef();
      } break;

#ifndef LCOMP_LINUX
#ifndef WIN64
   case L1450:
      {
         pI = static_cast<IDaqLDevice*>(new DaqL1450(Slot));
         pI->AddRef();
      } break;


   case L1251:
   case L1250:
   case L305:
   case L264:
      {
         pI = static_cast<IDaqLDevice*>(new DaqL1251(Slot));
         pI->AddRef();
      } break;

   case L1221:
      {
         pI = static_cast<IDaqLDevice*>(new DaqL1221(Slot));
         pI->AddRef();
      } break;

   case L032:
      {
         pI = static_cast<IDaqLDevice*>(new DaqL032(Slot));
         pI->AddRef();
      } break;
#endif
#endif
   case L791:
      {
         pI = static_cast<IDaqLDevice*>(new DaqL791(Slot));
         pI->AddRef();
      } break;

   case E440:
      {
         pI = static_cast<IDaqLDevice*>(new DaqE440(Slot));
         pI->AddRef();
      } break;

   case E140:
      {
         pI = static_cast<IDaqLDevice*>(new DaqE140(Slot));
         pI->AddRef();
      } break;

   case E2010B:
   case E2010:
      {
         pI = static_cast<IDaqLDevice*>(new DaqE2010(Slot));
         pI->AddRef();
      } break;

   case E154:
      {
         pI = static_cast<IDaqLDevice*>(new DaqE154(Slot));
         pI->AddRef();
      } break;

   default: { pI=NULL; LSetLastError(L_NOTSUPPORTED); }

   }
   return pI;
}

