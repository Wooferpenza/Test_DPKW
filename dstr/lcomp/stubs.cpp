#include "../include/stubs.h"
#include "../include/ioctl.h"

#include <stdio.h>

#ifdef LCOMP_LINUX
   #include <dlfcn.h>
   #include <string.h>
#endif

// 0 sucess 1 error

BOOL LFreeLibrary(HINSTANCE handle)
{
   #ifdef LCOMP_LINUX
      return dlclose(handle);
   #else
      return !FreeLibrary(handle);
   #endif
}

HINSTANCE LLoadLibrary(const char *szLibFileName)
{
  #ifdef LCOMP_LINUX
      return dlopen(szLibFileName,RTLD_LAZY);
   #else
      return LoadLibrary(szLibFileName);
   #endif
}

void *LGetProcAddress(HINSTANCE handle, const char *szProcName)
{
  #ifdef LCOMP_LINUX
      dlerror();
      void *ptr = dlsym(handle,szProcName);
      if(dlerror()==NULL) return ptr;
      return NULL;
   #else
      return GetProcAddress(handle,szProcName);
   #endif
}


BOOL LCloseHandle(HANDLE hDevice)
{
   #ifdef LCOMP_LINUX
      return close(hDevice);
   #else
      return !CloseHandle(hDevice);
   #endif
}

HANDLE LCreateFile(const char *szDrvName)
{
   #ifndef LCOMP_LINUX
      ULONG dwCreation = OPEN_EXISTING;
      ULONG dwFlags = FILE_FLAG_OVERLAPPED;
      return CreateFile(szDrvName, GENERIC_WRITE | GENERIC_READ, 0, // single use only no sharing
                        NULL, dwCreation, dwFlags, NULL);
   #else
      return open(szDrvName, O_RDWR);
   #endif

}

// in linux define handle as int
// maximum read/write size is 4096 byte
// see source for limitations...
BOOL LDeviceIoControl(HANDLE hDevice,
                       ULONG dwIoControlCode,
                       LPVOID lpInBuffer,
                       ULONG nInBufferSize,
                       LPVOID lpOutBuffer,
                       ULONG nOutBufferSize,
                       PULONG lpBytesReturned,
                       LPOVERLAPPED lpOverlapped)
{
#ifdef LCOMP_LINUX
   BOOL status = FALSE;
   IOCTL_BUFFER ibuf;
   unsigned int i;
   do
   {
      if(nInBufferSize > 4096) { printf("err");break; }
      if(nOutBufferSize > 4096) { printf("err1");break; }
      
      memset(&ibuf,0,sizeof(ibuf));

      if(lpOutBuffer)
      {
         ibuf.outSize = nOutBufferSize;
         for(i=0;i<nOutBufferSize;i++) ibuf.outBuffer[i] = ((PUCHAR)lpOutBuffer)[i];
      }

      if(lpInBuffer)
      {
         for(i=0;i<nInBufferSize;i++) ibuf.inBuffer[i] = ((PUCHAR)lpInBuffer)[i];
         ibuf.inSize = nInBufferSize;
      }

      if(ioctl((int)hDevice, dwIoControlCode, &ibuf)) break;
      if(lpOutBuffer) for(i=0;i<nOutBufferSize;i++) ((PUCHAR)lpOutBuffer)[i] = ibuf.outBuffer[i];
      *lpBytesReturned = ibuf.outSize;
      status = TRUE;
   } while (status==FALSE);
   return status;
#else
   return DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
                          lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
#endif
}
