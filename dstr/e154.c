#include "ldevusb.h"


// Cmd - vendor code in this case
void COMMAND_E154(ldevusb *dev, u16 Cmd)
{
   int status = -1;
  	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				Cmd, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
         				0, 0, NULL, 0, HZ*10);
}

// count in words, 2 - adrr
void GET_DATA_MEMORY_E154(ldevusb *dev, u16 *Data, u32 Count, u16 Addr)
{
   int status = -1;
   int i,ss,s = Count/2048; // bytes
   u8 *Dat = kmalloc(4096,GFP_KERNEL);
   for(i = 0; i<s; i++)
   {
     	status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
         				V_GET_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                     //(u16)(Addr+i*2048), 0, (u8 *)(&Data[2048*i]), 2048*sizeof(u16), HZ*10);
                     (u16)(Addr+i*2048), 0, (u8 *)Dat, 2048*sizeof(u16), HZ*10);
      memcpy(&Data[2048*i],Dat,2048*sizeof(u16));                     
   }

   ss = Count%2048;
   if(ss){
     	status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
         				V_GET_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                     //(u16)(Addr+s*2048), 0, (u8 *)(&Data[2048*s]), ss*sizeof(u16), HZ*10);
                     (u16)(Addr+s*2048), 0, (u8 *)Dat, ss*sizeof(u16), HZ*10);
      memcpy(&Data[2048*s],Dat,ss*sizeof(u16));
   }
   kfree(Dat);                     
}

void PUT_DATA_MEMORY_E154(ldevusb *dev, u16 *Data, u32 Count, u16 Addr) // how words
{
   int status = -1;
   int i,ss,s = Count/2048; // bytes
   u8 *Dat = kmalloc(4096,GFP_KERNEL);
   for(i = 0; i<s; i++)
   {
      memcpy(Dat,&Data[2048*i],2048*sizeof(u16));
     	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				V_PUT_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     //(u16)(Addr+i*2048), 0, (u8 *)(&Data[2048*i]), 2048*sizeof(u16), HZ*10);
                     (u16)(Addr+i*2048), 0, (u8 *)Dat, 2048*sizeof(u16), HZ*10);
   }

   ss = Count%2048;
   if(ss){
      memcpy(Dat,&Data[2048*s],ss*sizeof(u16));
     	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				V_PUT_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     //(u16)(Addr+s*2048), 0, (u8 *)(&Data[2048*s]), ss*sizeof(u16), HZ*10);
                     (u16)(Addr+s*2048), 0, (u8 *)Dat, ss*sizeof(u16), HZ*10);
   }
   kfree(Dat);
}

u16 GET_DM_WORD_E154(ldevusb *dev, u16 Addr)
{
   int status = -1;
   u16 Data;
   u16 *Dat = (u16 *)kmalloc(2,GFP_KERNEL);
  	status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
         				V_GET_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
         				(u16)(Addr), 0, Dat, sizeof(u16), HZ*10);
   Data = Dat[0];
   kfree(Dat);
   return Data;   
}

void PUT_DM_WORD_E154(ldevusb *dev, u16 Addr, u16 Data)
{
   int status = -1;
   u16 *Dat = (u16 *)kmalloc(2,GFP_KERNEL);
   Dat[0]=Data;
  	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				V_PUT_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     (u16)(Addr), 0, Dat, sizeof(u16), HZ*10);
   kfree(Dat);
}


u8 GET_PM_WORD_E154(ldevusb *dev, u16 Addr)
{
   int status = -1;
   u8 Data;
   u8 *Dat = (u8 *)kmalloc(1,GFP_KERNEL);
  	status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
         				V_GET_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
         				(u16)(Addr), 0, Dat, sizeof(u8), HZ*10);
 
   Data = Dat[0];
   kfree(Dat);                     
   return Data;   
}

void PUT_PM_WORD_E154(ldevusb *dev, u16 Addr, u8 Data)
{
   int status = -1;
   u8 *Dat = (u8 *)kmalloc(1,GFP_KERNEL);
   Dat[0]=Data;
  	status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
         				V_PUT_ARRAY_E154, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     (u16)(Addr), 0, Dat, sizeof(u8), HZ*10);
   kfree(Dat);                     
}

//------------------------------------------------------------------------
// разрешение/запрещение режима записи в ППЗУ модуля
//------------------------------------------------------------------------
u32 EnableFlashWrite_E154(ldevusb *dev, u16 Flag)
{
   PUT_PM_WORD_E154(dev,L_FLASH_ENABLED_E154,(u8)Flag);
   return 1;
}

//------------------------------------------------------------------------
// чтенние слова из ППЗУ
//------------------------------------------------------------------------
void ReadFlashWord_E154(ldevusb *dev, u16 *Data, u16 Size)
{
   GET_DATA_MEMORY_E154(dev, Data, Size, L_DESCRIPTOR_BASE_E154);
}

//------------------------------------------------------------------------
// запись слова в ППЗУ
//------------------------------------------------------------------------
void WriteFlashWord_E154(ldevusb *dev, u16 *Data, u16 Size)
{
   PUT_DATA_MEMORY_E154(dev, Data, Size, L_DESCRIPTOR_BASE_E154);
}


// SetFIFO - defined in E440.cpp



void EnableE154(ldevusb *dev, int b)
{
	int i,status;
	ADC_PAR_E154_PACK_U ap;
	u16 st,extf,sens,mode;
	if(b) // start ADC
   {
      // pack adc parameters
      for(i=0;i<16;i++) ap.t1.Chn[i] = (u8)(dev->adcPar.t3.Chn[i]);
      ap.t1.Rate = dev->adcPar.t3.Rate;
      ap.t1.NCh = (u8)dev->adcPar.t3.NCh;
      ap.t1.Kadr = (u8)(dev->adcPar.t3.Kadr);
      ap.t1.Kadr1 = (u8)(dev->adcPar.t3.Kadr>>8);      
      ap.t1.Scale = (u8)(dev->adcPar.t3.Scale);
      
      st = dev->adcPar.t3.SynchroType & 0x3;
      extf = dev->adcPar.t3.SynchroType & 0xC0;
      if(st == 1) st = 2; else if(st==2) st=1;

      sens = dev->adcPar.t3.SynchroSensitivity ? 2:0;
      mode = dev->adcPar.t3.SynchroMode ? 1:0;
      if(st==3) st+=(sens + mode);
      st += extf;
      
      ap.t1.SynchroType = (u8)st;
      ap.t1.AdChannel = (u8)(dev->adcPar.t3.AdChannel);
      ap.t1.AdPorog = dev->adcPar.t3.AdPorog;
      
      COMMAND_E154(dev, V_STOP_ADC_E154);

      PUT_DATA_MEMORY_E154(dev, (u16 *)&ap.bi, 13, L_ADC_PARS_BASE_E154);

      status=usb_clear_halt(dev->udev, usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr));
      COMMAND_E154(dev, V_START_ADC_E154);
      DbgPrint("start satrt \n");
   }
   else // stop ADC
   {
      DbgPrint("stop stop \n");
      COMMAND_E154(dev, V_STOP_ADC_E154);
      DbgPrint("pipe aborted \n");
   }
}
