#include "ldevpci.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,5,1)

#define __64BIT (__SIZEOF_POINTER__ == 8)

#if __64BIT
    #define memcpy_toio32(dst,src,sz) __inline_memcpy(dst,src,sz*sizeof(u32))
    #define memcpy_fromio32(dst,src,sz) __inline_memcpy(dst,src,sz*sizeof(u32))
#else
    #define memcpy_toio32(dst,src,sz) memcpy_toio(dst,src,sz*sizeof(u32))
    #define memcpy_fromio32(dst,src,sz) memcpy_fromio(dst,src,sz*sizeof(u32))
#endif

#else
    #define memcpy_toio32 __iowrite32_copy
    #define memcpy_fromio32 __ioread32_copy
#endif


void COMMAND_760(ldevpci *dev, u16 Cmd)
{
u32 TO = 1000000;
   PUT_DM_WORD_PCI(dev, L_COMMAND_PLX, Cmd);
   iowrite16(0,dev->CMD_REG_PLX);
   while(GET_DM_WORD_PCI(dev, L_COMMAND_PLX)&&(TO--));
}

u16 GET_DM_WORD_PCI(ldevpci *dev, u16 Addr)
{
   u16 data = ((Addr|0x4000)&0x7FFF)+dev->DSPBase*((Addr>>15)&1);  //4000 -DM
   iowrite16(data, dev->IDMA_REG_PLX);
   return ioread16(dev->DATA_REG_PLX);
}

void PUT_DM_WORD_PCI(ldevpci *dev, u16 Addr, u16 Data)
{
   u16 data = ((Addr|0x4000)&0x7FFF)+dev->DSPBase*((Addr>>15)&1);
   iowrite16(data,dev->IDMA_REG_PLX);
   iowrite16(Data,dev->DATA_REG_PLX);
}

void PUT_PM_WORD_PCI(ldevpci *dev, u16 Addr, u32 Data)
{
//   u16 dl;
   iowrite16(Addr,dev->IDMA_REG_PLX);
//   dl=(u16)(Data&0xFFFF);        iowrite16(dl,dev->DATA_REG_PLX);
//   dl=(u16)((Data>>16)&0xFF);  iowrite16(dl,dev->DATA_REG_PLX);
   iowrite32(Data,dev->DATA_REG_PLX);
}

u32 GET_PM_WORD_PCI(ldevpci *dev, u16 Addr)
{
   iowrite16(Addr,dev->IDMA_REG_PLX);
   return ioread32(dev->DATA_REG_PLX);
}

void GET_DATA_MEMORY_PCI(ldevpci *dev, u16 *Data, u32 Count, u16 Addr)
{
int i,s,ss;
   u16 data = ((Addr|0x4000)&0x7FFF)+dev->DSPBase*((Addr>>15)&1);
   iowrite16(data,dev->IDMA_REG_PLX);
   s = Count/2048;
   for(i = 0; i<s; i++) { memcpy_fromio32(Data,dev->DTA_ARRAY_PLX,1024); Data+=2048; } // 4096
   ss = Count%2048;
   if(ss) memcpy_fromio32(Data, dev->DTA_ARRAY_PLX, (ss>>1));/**sizeof(u32)*/
   if(ss%2) Data[Count-1] = ioread16(dev->DTA_ARRAY_PLX+2*(ss-1));
}

void PUT_DATA_MEMORY_PCI(ldevpci *dev, u16 *Data, u32 Count, u16 Addr) // how words
{
int i,s,ss;
   u16 data = ((Addr|0x4000)&0x7FFF)+dev->DSPBase*((Addr>>15)&1);
   iowrite16(data,dev->IDMA_REG_PLX);
   s = Count/2048;
   for(i = 0; i<s; i++) { memcpy_toio32(dev->DTA_ARRAY_PLX,Data,1024); Data+=2048; }/*4096*/
   ss = Count%2048;
   if(ss) memcpy_toio32(dev->DTA_ARRAY_PLX, Data, (ss>>1));/**sizeof(u32)*/
   if(ss%2) iowrite16(Data[Count-1], dev->DTA_ARRAY_PLX+2*(ss-1));
}

void GET_PM_MEMORY_PCI(ldevpci *dev, u32 *Data, u32 Count, u16 Addr)
{
int i,s,ss;
   iowrite16(Addr,dev->IDMA_REG_PLX);
   s = Count/1024;
   for(i = 0; i<s; i++) { memcpy_fromio32(Data, dev->DTA_ARRAY_PLX, 1024);  Data+=1024; }//4096
   ss = Count%1024;
   if(ss) memcpy_fromio32(Data, dev->DTA_ARRAY_PLX, ss);/**sizeof(u32)*/
}


void PUT_PM_MEMORY_PCI(ldevpci *dev, u32 *Data, u32 Count, u16 Addr)
{
int i,s,ss;
   iowrite16(Addr,dev->IDMA_REG_PLX);
   s = Count/1024;
   for(i = 0; i<s; i++) { memcpy_toio32(dev->DTA_ARRAY_PLX, Data, 1024);  Data+=1024; } // 4096
   ss = Count%1024;
   if(ss) memcpy_toio32(dev->DTA_ARRAY_PLX, Data, ss);/**sizeof(u32)*/
}


void HB_GET_DATA_MEMORY_PCI(ldevpci *dev, hb *buf, int pos, u32 Count, u16 Addr)
{
   int PAGE_SIZE_16 = PAGE_SIZE/sizeof(u16);
   int page = pos/PAGE_SIZE_16;
   int off =  pos%PAGE_SIZE_16;
   int len = PAGE_SIZE_16-off;
   do
   {
      if(Count<len) len=Count;
      GET_DATA_MEMORY_PCI(dev,((u16 *)(buf->addr[page])+off),len,Addr);
      Addr+=len;
      page++;
      Count-=len;
      off=0;
      len=PAGE_SIZE_16;
   } while(Count);
   return;
}

//                                                in int
void HB_PUT_PM_MEMORY_PCI(ldevpci *dev, hb *buf, int pos, u32 Count, u16 Addr)
{
   int PAGE_SIZE_32 = PAGE_SIZE/sizeof(u32);
   int page = pos/PAGE_SIZE_32;
   int off =  pos%PAGE_SIZE_32;
   int len = PAGE_SIZE_32-off;
   do
   {
      if(Count<len) len=Count;
      PUT_PM_MEMORY_PCI(dev,((u32 *)(buf->addr[page])+off),len,Addr);
      Addr+=len;
      page++;
      Count-=len;
      off=0;
      len=PAGE_SIZE_32;
   } while(Count);
   return;
}


// flash routines /////////////////////////////////////////////////////////////////////////////
void SetFlash_Plx(ldevpci *dev, u16 FlashData)
{
   u32 data = inl(dev->FLASH_REG_PLX)&(0xFFFFFFFF^0x01000000); //ЪБОХМЙМЙ 24
   data |= (u32)FlashData<<24L;   // РЙИОХМЙ ЪОБЮЕОЙЕ Ч 24
   outl(data,dev->FLASH_REG_PLX);              // ЧЩЧЕМЙ
}

void StartFlash_Plx(ldevpci *dev)
{
u32 data,data1;
   data = inl(dev->FLASH_CS_PLX)|0x00000004; // 0000 0100 ЮЙРУЕМЕЛФ
   outl(data,dev->FLASH_CS_PLX);
   data1= inl(dev->FLASH_REG_PLX)&(0xFFFFFFFF^0x04000000); // ЪБОХМЙМЙ ЛМПЛ
   outl(data1, dev->FLASH_REG_PLX);
   SetFlash_Plx(dev, 0); // ЪБРЙУБМЙ 0
   udelay(10);
}

u16 GetFlashDataBit_Plx(ldevpci *dev)
{
   udelay(10);
   return ((inl(dev->FLASH_REG_PLX) & 0x08000000L) ? 1 : 0);
}

void FlipCLK_Plx(ldevpci *dev)
{
u32 data,clock;
   udelay(10);
   data = inl(dev->FLASH_REG_PLX);
   clock = (data & 0x04000000) ? 1:0;
   data &= (0xFFFFFFFF^0x04000000); // ЪБОХМЙМЙ ЛМПЛ
   data |= (clock^1)<<26L;
   outl(data,dev->FLASH_REG_PLX);
}

void StopFlash_Plx(ldevpci *dev)
{
   u32 data = inl(dev->FLASH_CS_PLX)&(0xFFFFFFFF^0x00000004); // ЪБОХМЙМЙ ЮЙРУЕМЕЛФ
   outl(data,dev->FLASH_CS_PLX);
   udelay(10);
}

void ReadFlashWord_PLX(ldevpci *dev, u16 FlashAddress, u16 *Data)
{
   int i;
   StartFlash_Plx(dev);
   FlashAddress=(FlashAddress<<7)|0xC000;  // modify address
   for(i=0;i<9;i++)
   {
      SetFlash_Plx(dev, FlashAddress>>15);
      FlipCLK_Plx(dev);
      FlipCLK_Plx(dev);
      FlashAddress <<=1;
   }
   for(i=0; i < 16; i++)
   {
      *Data <<= 1;
      FlipCLK_Plx(dev);
      FlipCLK_Plx(dev);
      *Data |= GetFlashDataBit_Plx(dev);
   }
   StopFlash_Plx(dev);
}

void WriteFlashWord_PLX(ldevpci *dev, u16 FlashAddress, u16 Data)
{
   int i;
   StartFlash_Plx(dev);
   FlashAddress=(FlashAddress<<7) | 0xA000;
   for(i=0;i<9;i++)
   {
      SetFlash_Plx(dev,FlashAddress>>15);
      FlipCLK_Plx(dev);
      FlipCLK_Plx(dev);
      FlashAddress <<=1;
   }
   for(i=0;i<16;i++)
   {
      SetFlash_Plx(dev,Data>>15);
      FlipCLK_Plx(dev);
      FlipCLK_Plx(dev);
      Data <<= 1;
   }
   StopFlash_Plx(dev);
   for(i=0; i<1000;i++) udelay(10);

}

u32 EnableFlashWrite_PLX(ldevpci *dev, u16 Flag)
{
   int i;
   u16 byte;
   StartFlash_Plx(dev);
   byte=(Flag)?0x9800:0x8000;
   for(i=0; i < 9; i++)
   {
      SetFlash_Plx(dev, byte>>15);
      FlipCLK_Plx(dev);
      FlipCLK_Plx(dev);
      byte <<= 1;
   }
   StopFlash_Plx(dev);
   for(i=0; i<1000;i++) udelay(10);
   return 1;

}

u32 ReadPlataDescr_PLX(ldevpci *dev, PPLATA_DESCR pd)
{
   u16 i,d1;
   for(i=0; i<sizeof(PLATA_DESCR)/2; i++)
   {
      ReadFlashWord_PLX(dev,i,&d1);
      ((u16 *)pd)[i] = d1;
   }
   return 1;
}

u32 WritePlataDescr_PLX(ldevpci *dev, PPLATA_DESCR pd, u16 enable)
{
   u16 i,tmp;
   if(!EnableFlashWrite_PLX(dev,1)) return 0;

   tmp=(!enable)*32+1;
   for(i=tmp; i<sizeof(PLATA_DESCR)/2; i++)
   {
      WriteFlashWord_PLX(dev,i-1,((u16 *)pd)[i]);
   }
   if(!EnableFlashWrite_PLX(dev,0)) return 0;
   return 1;
}
