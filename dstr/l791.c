#include "ldevpcib.h"

u32 WaitForFlashCmdReady_L791(ldevpcib *dev)
{
   int TO = 1000000L;
   while(CHECKBIT(ioread32(dev->L791_REG_ADDR+R_STATUS_L791),25)&&(TO--));
   return (++TO);
}

u32 ReadFlashWord_L791(ldevpcib *dev, u32 addr, u16 *data)
{
u32 ctrl;
   if(!WaitForFlashCmdReady_L791(dev)) return 0; // if busy

   iowrite32(addr,dev->L791_REG_ADDR+R_FLASH_ADDRESS_L791);

   ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
   SETBIT(ctrl,BIT_EEPROM_CMD_0);
   CLEARBIT(ctrl,BIT_EEPROM_CMD_1);
   SETBIT(ctrl,BIT_EEPROM_START);
   iowrite32(ctrl,dev->L791_REG_ADDR+R_CONTROL_L791);

   if(!WaitForFlashCmdReady_L791(dev)) return 0;

   *data = (u16)(ioread32(dev->L791_REG_ADDR+R_FLASH_DATA_L791) & 0xFF);

   return 1;
}

u32 ReadPlataDescr_L791(ldevpcib *dev, PPLATA_DESCR_L791 pd)
{
   u16 i, d1;
   for(i=0; i<sizeof(PLATA_DESCR_L791); i++)
   {
      if(!ReadFlashWord_L791(dev,i,&d1)) return 0;
      ((u8 *)pd)[i] = (u8)d1;
   }
   return 1;
}


u32 WriteFlashWord_L791(ldevpcib *dev, u32 addr, u16 data)
{
u32 ctrl;
   if(!WaitForFlashCmdReady_L791(dev)) return 0;

   iowrite32(addr,dev->L791_REG_ADDR+R_FLASH_ADDRESS_L791);

   iowrite32(data,dev->L791_REG_ADDR+R_FLASH_DATA_L791);

   ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
   CLEARBIT(ctrl,BIT_EEPROM_CMD_0);
   SETBIT(ctrl,BIT_EEPROM_CMD_1);
   SETBIT(ctrl,BIT_EEPROM_START);
   iowrite32(ctrl,dev->L791_REG_ADDR+R_CONTROL_L791);

   if(!WaitForFlashCmdReady_L791(dev)) return 0;

   return 1;
}

u32 WriteFlashPage_L791(ldevpcib *dev, u16 page)
{
u32 ctrl;
u32 d;
ULONG TO = 100000L;
   if(page > 256) return 0;

// enable write
   ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791) & 0x10FFFFFF; // clear all except ttl mode
   SETBIT(ctrl,27);
   iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);

   if(!WaitForFlashCmdReady_L791(dev)) return 0;

   iowrite32((page<<9), dev->L791_REG_ADDR+R_FLASH_ADDRESS_L791);

//   ctrl = m->ind(R_CONTROL_L791);
   SETBIT(ctrl,BIT_EEPROM_CMD_0);
   SETBIT(ctrl,BIT_EEPROM_CMD_1);
   SETBIT(ctrl,BIT_EEPROM_START);
   iowrite32(ctrl,dev->L791_REG_ADDR+R_CONTROL_L791);

   if(!WaitForFlashCmdReady_L791(dev)) return 0;

   CLEARBIT(ctrl,BIT_EEPROM_CMD_0);
   CLEARBIT(ctrl,BIT_EEPROM_CMD_1);
   SETBIT(ctrl,BIT_EEPROM_START);

   while(TO--)
   {
  //    ULONG ctrl = m->ind(R_CONTROL_L791);
  //    CLEARBIT(ctrl,BIT_EEPROM_CMD_0);
  //    CLEARBIT(ctrl,BIT_EEPROM_CMD_1);
  //    SETBIT(ctrl,BIT_EEPROM_START);
      iowrite32(ctrl,dev->L791_REG_ADDR+R_CONTROL_L791);
      if(!WaitForFlashCmdReady_L791(dev)) return 0;
      d = ioread32(dev->L791_REG_ADDR+R_FLASH_DATA_L791) & 0x80;
      if(d) break;
   }

// disable write
//   ctrl = m->ind(R_CONTROL_L791);
   CLEARBIT(ctrl,BIT_EEPROM_START);
   CLEARBIT(ctrl,27);
   iowrite32(ctrl,dev->L791_REG_ADDR+R_CONTROL_L791);

   return (++TO);
}
