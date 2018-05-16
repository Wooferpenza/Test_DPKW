#ifndef __TEST__
#define __TEST__

typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);

HINSTANCE CallCreateInstance(const char* name);

extern CREATEFUNCPTR CreateInstance;

#endif
