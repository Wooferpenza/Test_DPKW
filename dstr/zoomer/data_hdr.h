#pragma pack(1)
typedef struct __DATA_FILE_HDR__
{
   int Signature; // 0xAA55A5A5
   char BoardName[255];
   double dRate; // частота опроса в кадре
   double dKadr; // интервал между кадрами
   double dScale; // масштаб таймера
   unsigned short NCh; // количество каналов
   unsigned short Chn[128]; // логические номера каналов
   char Comment[2048];
} DATA_FILE_HDR, *PDATA_FILE_HDR;
#pragma pack()
