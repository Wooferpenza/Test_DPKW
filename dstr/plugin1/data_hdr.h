#pragma pack(1)
typedef struct __DATA_FILE_HDR__
{
   int Signature; // 0xAA55A5A5
   char BoardName[255];
   double dRate; // ������� ������ � �����
   double dKadr; // �������� ����� �������
   double dScale; // ������� �������
   unsigned short NCh; // ���������� �������
   unsigned short Chn[128]; // ���������� ������ �������
   char Comment[2048];
} DATA_FILE_HDR, *PDATA_FILE_HDR;
#pragma pack()
