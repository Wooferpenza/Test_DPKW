#include "ldevusb.h"
//////////////////////////////////////////////////////////////////////////////////////////

void COMMAND_E440(ldevusb *dev, u16 Cmd)
{
  int status = -1;
  u32 TO = 1000;
  PUT_PM_WORD_E440(dev, L_COMMAND_E440, LBIOS_OUTVAR(Cmd));
  status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                           V_COMMAND_IRQ_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                           0, 0, NULL, 0, HZ*10);

  while(LBIOS_INVAR(GET_PM_WORD_E440(dev, L_COMMAND_E440))&&(TO--));
}


u16 GET_DM_WORD_E440(ldevusb *dev, u16 Addr)
{
  int status = -1;
  u16 Data;
  u16 *Dat = (u16 *)kmalloc(2,GFP_KERNEL);
  status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                           V_GET_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                           (u16)(Addr|DM_E440), 0, Dat, sizeof(u16), HZ*10);
  Data = Dat[0];
  kfree(Dat);
  return Data;
}

void PUT_DM_WORD_E440(ldevusb *dev, u16 Addr, u16 Data)
{
  int status = -1;
  u16 *Dat = (u16 *)kmalloc(2,GFP_KERNEL);
  Dat[0]=Data;
  status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                           V_PUT_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                           (u16)(Addr|DM_E440), 0, Dat, sizeof(u16), HZ*10);
  kfree(Dat);
}

u32 GET_PM_WORD_E440(ldevusb *dev, u16 Addr)
{
  int status = -1;
  u32 Data;
  u32 *Dat = (u32 *)kmalloc(4,GFP_KERNEL);
  status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                           V_GET_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                           (u16)(Addr|PM_E440), 0, Dat, sizeof(u32), HZ*10);
  Data = Dat[0];
  kfree(Dat);
  return Data;
}

void PUT_PM_WORD_E440(ldevusb *dev, u16 Addr, u32 Data)
{
  int status = -1;
  u32 *Dat = (u32 *)kmalloc(4,GFP_KERNEL);
  Dat[0] = Data;
  status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
	                         V_PUT_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                           (u16)(Addr|PM_E440), 0, Dat, sizeof(u32), HZ*10);
  kfree(Dat);
}

void GET_DATA_MEMORY_E440(ldevusb *dev, u16 *Data, u32 Count, u16 Addr)
{
  int status = -1;
  int i,ss,s = Count/2048; // bytes
  u8 *Dat = kmalloc(4096,GFP_KERNEL);
  for(i = 0; i<s; i++)
  {
    status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                              V_GET_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                              //(u16)((Addr+i*2048)|DM_E440), 0, (u8 *)(&Data[2048*i]), 2048*sizeof(u16), HZ*10);
                              (u16)((Addr+i*2048)|DM_E440), 0, (u8 *)Dat, 2048*sizeof(u16), HZ*10);
    memcpy(&Data[2048*i],Dat,2048*sizeof(u16));                             
  }

  ss = Count%2048;
  if(ss)
  {
    status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
	                           V_GET_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                             //(u16)((Addr+s*2048)|DM_E440), 0, (u8 *)(&Data[2048*s]), ss*sizeof(u16), HZ*10);
                             (u16)((Addr+s*2048)|DM_E440), 0, (u8 *)Dat, ss*sizeof(u16), HZ*10);
    memcpy(&Data[2048*s],Dat,ss*sizeof(u16));
  }
  kfree(Dat);
}

void PUT_DATA_MEMORY_E440(ldevusb *dev, u16 *Data, u32 Count, u16 Addr) // how words
{
  int status = -1;
  int i,ss,s = Count/2048; // bytes
  u8 *Dat = kmalloc(4096,GFP_KERNEL);
  for(i = 0; i<s; i++)
  {
    memcpy(Dat,&Data[2048*i],2048*sizeof(u16));
    status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                             V_PUT_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                             //(u16)((Addr+i*2048)|DM_E440), 0, (u8 *)(&Data[2048*i]), 2048*sizeof(u16), HZ*10);
                             (u16)((Addr+i*2048)|DM_E440), 0, (u8 *)Dat, 2048*sizeof(u16), HZ*10);
  }

  ss = Count%2048;
  if(ss)
  {
    memcpy(Dat,&Data[2048*s],ss*sizeof(u16));
    status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                             V_PUT_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                             //(u16)((Addr+s*2048)|DM_E440), 0, (u8 *)(&Data[2048*s]), ss*sizeof(u16), HZ*10);
                             (u16)((Addr+s*2048)|DM_E440), 0, (u8 *)Dat, ss*sizeof(u16), HZ*10);
  }
  kfree(Dat);
}

void PUT_PM_MEMORY_E440(ldevusb *dev, u32 *Data, u32 Count, u16 Addr)
{
  int status = -1;
  u32 MP = 1024;
  int i,ss,s = Count/MP; // bytes
  u8 *Dat = kmalloc(4096,GFP_KERNEL);
  for(i = 0; i<s; i++)
  {
    memcpy(Dat,&Data[MP*i],MP*sizeof(u32));
   	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                             V_PUT_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                             //(u16)((Addr+i*MP)|PM_E440), 0, (u8 *)(&Data[MP*i]), MP*sizeof(u32), HZ*10);
                             (u16)((Addr+i*MP)|PM_E440), 0, (u8 *)Dat, MP*sizeof(u32), HZ*10);
  }
  ss = Count%MP;
  if(ss)
  {
    memcpy(Dat,&Data[MP*s],ss*sizeof(u32));
    status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
       				               V_PUT_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                             //(u16)((Addr+s*MP)|PM_E440), 0, (u8 *)(&Data[MP*s]), ss*sizeof(u32), HZ*10);
                             (u16)((Addr+s*MP)|PM_E440), 0, (u8 *)Dat, ss*sizeof(u32), HZ*10);
  }
   kfree(Dat);
}

void GET_PM_MEMORY_E440(ldevusb *dev, u32 *Data, u32 Count, u16 Addr)
{
  int status = -1;
  u32 MP = 1024;
  int i,ss,s = Count/MP; // bytes
  u8 *Dat = kmalloc(4096,GFP_KERNEL);
  for(i = 0; i<s; i++)
  {
   	status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                     				 V_GET_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                             //(u16)((Addr+i*MP)|PM_E440), 0, (u8 *)(&Data[MP*i]), MP*sizeof(u32), HZ*10);
                             (u16)((Addr+i*MP)|PM_E440), 0, (u8 *)Dat, MP*sizeof(u32), HZ*10);

    memcpy(&Data[MP*i],Dat,MP*sizeof(u32));                 
  }
  ss = Count%MP;
  if(ss)
  {
   	status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                     				 V_GET_ARRAY_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                             //(u16)((Addr+s*MP)|PM_E440), 0, (u8 *)(&Data[MP*s]), ss*sizeof(u32), HZ*10);
                             (u16)((Addr+s*MP)|PM_E440), 0, (u8 *)Dat, ss*sizeof(u32), HZ*10);
    memcpy(&Data[MP*s],Dat,ss*sizeof(u32));    
  }
  kfree(Dat);
}


//------------------------------------------------------------------------
// разрешение/запрещение режима записи в ППЗУ модуля
//------------------------------------------------------------------------
u32 EnableFlashWrite_E440(ldevusb *dev, u16 Flag)
{
   int i;
   u16 word=(Flag)?0x9800:0x8000;
   PUT_PM_WORD_E440(dev,L_FLASH_ENABLED_E440,LBIOS_OUTVAR(word));
   COMMAND_E440(dev,cmENABLE_FLASH_WRITE_E440);
   for(i=0; i<1000;i++) udelay(10);
   return 1;
}

//------------------------------------------------------------------------
// чтенние слова из ППЗУ
//------------------------------------------------------------------------
void ReadFlashWord_E440(ldevusb *dev, u16 FlashAddress, u16 *Data)
{
   PUT_PM_WORD_E440(dev,L_FLASH_ADDRESS_E440, LBIOS_OUTVAR((u16)((FlashAddress << 7) | 0xC000)));
   COMMAND_E440(dev,cmREAD_FLASH_WORD_E440);
   *Data = LBIOS_INVAR(GET_PM_WORD_E440(dev,L_FLASH_DATA_E440));

}

//------------------------------------------------------------------------
// запись слова в ППЗУ
//------------------------------------------------------------------------
void WriteFlashWord_E440(ldevusb *dev, u16 FlashAddress, u16 Data)
{
   int i;
   PUT_PM_WORD_E440(dev,L_FLASH_ADDRESS_E440, LBIOS_OUTVAR((u16)((FlashAddress << 7) | 0xA000)));
   PUT_PM_WORD_E440(dev,L_FLASH_DATA_E440, LBIOS_OUTVAR(Data));
   COMMAND_E440(dev,cmWRITE_FLASH_WORD_E440);
   for(i=0; i<1000;i++) udelay(10);
}

void SetBoardFIFO(ldevusb *dev)
{
   switch(dev->Type)
   {
      case E440: PUT_PM_WORD_E440(dev, L_ADC_FIFO_LENGTH_E440, LBIOS_OUTVAR(dev->wFIFO*2)); break;
   }
}


void EnableE440(ldevusb *dev, int b)
{
   int i;
   int status = -1;
   if(b) // start ADC
   {
      // synchro config

      PUT_PM_WORD_E440(dev,L_INPUT_MODE_E440, LBIOS_OUTVAR(dev->adcPar.t3.SynchroType));
      PUT_PM_WORD_E440(dev,L_SYNCHRO_AD_TYPE_E440, LBIOS_OUTVAR(dev->adcPar.t3.SynchroSensitivity));
      PUT_PM_WORD_E440(dev,L_SYNCHRO_AD_MODE_E440, LBIOS_OUTVAR(dev->adcPar.t3.SynchroMode));
      PUT_PM_WORD_E440(dev,L_SYNCHRO_AD_CHANNEL_E440, LBIOS_OUTVAR(dev->adcPar.t3.AdChannel));
      PUT_PM_WORD_E440(dev,L_SYNCHRO_AD_POROG_E440, LBIOS_OUTVAR(dev->adcPar.t3.AdPorog));

      PUT_PM_WORD_E440(dev,L_CONTROL_TABLE_LENGHT_E440, LBIOS_OUTVAR(dev->adcPar.t3.NCh));
      for(i=0; i < dev->adcPar.t3.NCh; i++) PUT_PM_WORD_E440(dev,L_CONTROL_TABLE_E440+i, LBIOS_OUTVAR(dev->adcPar.t3.Chn[i]) );

      PUT_PM_WORD_E440(dev,L_FIRST_SAMPLE_DELAY_E440, LBIOS_OUTVAR(dev->adcPar.t3.FPDelay));
      PUT_PM_WORD_E440(dev,L_ADC_RATE_E440, LBIOS_OUTVAR(dev->adcPar.t3.Rate));
      PUT_PM_WORD_E440(dev,L_INTER_KADR_DELAY_E440, LBIOS_OUTVAR(dev->adcPar.t3.Kadr));

      status=usb_clear_halt(dev->udev, usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr));
      
     	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				V_START_ADC_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
         				0x0|DM_E440, dev->wFIFO, NULL, 0, HZ*10);
      
      COMMAND_E440(dev, cmSTART_ADC_E440);
      DbgPrint("start start \n");
   }
   else // stop ADC
   {
      DbgPrint("stop stop \n");
      COMMAND_E440(dev, cmSTOP_ADC_E440);
      
     	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				V_START_ADC_E440, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
         				0x0|DM_E440, dev->wFIFO, NULL, 0, HZ*10);

      DbgPrint("pipe aborted \n");
   }
}
