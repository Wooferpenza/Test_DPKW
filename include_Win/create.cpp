#include <windows.h>
//#include <iostream.h>
#include <objbase.h>

#include <stdint.h>
#include "..\include\ioctl.h"
#include "..\include\ifc_ldev.h"
//#include <unknwn.h>

#include "..\include\create.h"
LPWSTR CharToLPWSTR(LPCSTR char_string)
{
    LPWSTR res;
    DWORD res_len = MultiByteToWideChar(1251, 0, char_string, -1, NULL, 0);
    res = (LPWSTR)GlobalAlloc(GPTR, (res_len + 1) * sizeof(WCHAR));
    MultiByteToWideChar(1251, 0, char_string, -1, res, res_len);
    return res;
}
CREATEFUNCPTR CreateInstance;

HINSTANCE CallCreateInstance(const char* name)
{
   HINSTANCE hComponent = ::LoadLibrary(CharToLPWSTR(name));
   if(hComponent==NULL)
   {
//      cout << "Unable load dll" << endl;
      return 0;
   }

   CreateInstance = (CREATEFUNCPTR)::GetProcAddress(hComponent,"CreateInstance");
   if(CreateInstance==NULL)
   {
//      cout << "Unable find CreateInstance" << endl;
      return 0;
   }
   return hComponent;
}

