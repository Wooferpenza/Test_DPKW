#include "ldevusb.h"
//////////////////////////////////////////////////////////////////////////////////////////

// Cmd - vendor code in this case
void COMMAND_E2010(ldevusb *dev, u16 Cmd, u16 Par1, u16 Par2)
{
   int status = -1;
   status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                     (char)Cmd, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     Par1, Par2, NULL, 0, HZ*10);
}


// count in byte, 2 - adrr
void GET_DATA_MEMORY_E2010(ldevusb *dev, u8 *Data, u32 Count, u32 Addr)
{
   int status = -1;
   int i,ss,s = Count/4096; // bytes
   u32 CurAddr = 0;
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   u8 *Dat = (u8 *)kmalloc(4096,GFP_KERNEL);
   for(i = 0; i<s; i++)
   {
		CurAddr = Addr+i*4096;
      status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                     V_GET_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                     //(u16)(CurAddr&0xFFFF), (u16)(CurAddr>>0x10), (u8 *)(&Data[4096*i]), 4096*sizeof(u8), HZ*10);
                     (u16)(CurAddr&0xFFFF), (u16)(CurAddr>>0x10), (u8 *)Dat, 4096*sizeof(u8), HZ*10);
      memcpy(&Data[4096*i],Dat,4096*sizeof(u8));
   }

   ss = Count%4096;
   if(ss){
      status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                     V_GET_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                     //(u16)((Addr+s*4096)&0xFFFF), (u16)((Addr+s*4096)>>0x10), (u8 *)(&Data[4096*s]), ss*sizeof(u8), HZ*10);
                     (u16)((Addr+s*4096)&0xFFFF), (u16)((Addr+s*4096)>>0x10), (u8 *)Dat, ss*sizeof(u8), HZ*10);
      memcpy(&Data[4096*s],Dat,ss*sizeof(u8));                     
   }
   kfree(Dat);
}

// count in byte, 2 - adrr
void PUT_DATA_MEMORY_E2010(ldevusb *dev, u8 *Data, u32 Count, u32 Addr)
{
   int status = -1;
   int i,ss,s = Count/4096; // bytes
   u32 CurAddr = 0;
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   u8 *Dat = (u8 *)kmalloc(4096,GFP_KERNEL);
   for(i = 0; i<s; i++)
   {
      CurAddr = Addr+i*4096;	
      memcpy(Dat,&Data[4096*i],4096*sizeof(u8));
      status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                     V_PUT_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     //(u16)(CurAddr&0xFFFF), (u16)(CurAddr>>0x10), (u8 *)(&Data[4096*i]), 4096*sizeof(u8), HZ*10);
                     (u16)(CurAddr&0xFFFF), (u16)(CurAddr>>0x10), (u8 *)Dat, 4096*sizeof(u8), HZ*10);
   }

   ss = Count%4096;
   if(ss){
      memcpy(Dat,&Data[4096*s],ss*sizeof(u8));
      status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                     V_PUT_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     //(u16)((Addr+s*4096)&0xFFFF), (u16)((Addr+s*4096)>>0x10),  (u8 *)(&Data[4096*s]), ss*sizeof(u8), HZ*10);
                     (u16)((Addr+s*4096)&0xFFFF), (u16)((Addr+s*4096)>>0x10),  (u8 *)Dat, ss*sizeof(u8), HZ*10);
   }
   kfree(Dat);
}


u16 GET_DM_WORD_E2010(ldevusb *dev, u32 Addr)
{
   int status = -1;
   u16 Data;
   u16 *Dat = (u16 *)kmalloc(2,GFP_KERNEL);
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   
   status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                V_GET_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
               (u16)(Addr&0xFFFF), (u16)(Addr>>0x10), (u8 *)Dat, 2*sizeof(u8), HZ*10);
   Data = Dat[0];
   kfree(Dat);
   return Data;
}

void PUT_DM_WORD_E2010(ldevusb *dev, u32 Addr, u16 Data)
{
   int status = -1;
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   u16 *Dat = (u16 *)kmalloc(2,GFP_KERNEL);
   Dat[0] = Data;
   status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                V_PUT_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
               (u16)(Addr&0xFFFF), (u16)(Addr>>0x10), (u8 *)Dat, 2*sizeof(u8), HZ*10);
   kfree(Dat);
}


u8 GET_PM_WORD_E2010(ldevusb *dev, u32 Addr)
{
   int status = -1;
   u8 Data;
   u8 *Dat = (u8 *)kmalloc(1,GFP_KERNEL);
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   
   status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                V_GET_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
               (u16)(Addr&0xFFFF), (u16)(Addr>>0x10), Dat, 1*sizeof(u8), HZ*10);
   Data = Dat[0];            
   kfree(Dat);
   return Data;
}

void PUT_PM_WORD_E2010(ldevusb *dev, u32 Addr, u8 Data)
{
   int status = -1;
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   u8 *Dat = (u8 *)kmalloc(1,GFP_KERNEL);
   Dat[0] = Data;
   status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                V_PUT_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
               (u16)(Addr&0xFFFF), (u16)(Addr>>0x10), Dat, 1*sizeof(u8), HZ*10);
   kfree(Dat);
}

// читает всю флеш
void ReadFlashWord_E2010(ldevusb *dev, u16 *Data, u16 Size)
{
   int status = -1;
   //u32 Addr = MODULE_DESCRIPTOR_ADDRESS;
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   u8 *Dat = (u8 *)kmalloc(Size*sizeof(u16),GFP_KERNEL);

   status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                V_GET_ARRAY_E2010, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
               (u16)(MODULE_DESCRIPTOR_ADDRESS&0xFFFF), (u16)(MODULE_DESCRIPTOR_ADDRESS>>0x10), (u8 *)Dat, Size*sizeof(u16), HZ*10);
   memcpy(Data,Dat,Size*sizeof(u16));
   kfree(Dat);               
}

// пишет всю флеш...
void WriteFlashWord_E2010(ldevusb *dev, u16 *Data, u16 Size)
{
   int status = -1;
   //u32 Addr = BOOT_LOADER_START_ADDRESS;
   //u32 LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   u8 *Dat = (u8 *)kmalloc(Size*sizeof(u16),GFP_KERNEL);

   COMMAND_E2010(dev, V_CALL_APPLICATION_E2010, (BOOT_LOADER_START_ADDRESS&0xFFFF), 0);

   //Addr = MODULE_DESCRIPTOR_ADDRESS;
   //LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   memcpy(Dat,Data,Size*sizeof(u16));
   status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
            13, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
            (u16)(MODULE_DESCRIPTOR_ADDRESS&0xFFFF), (u16)(MODULE_DESCRIPTOR_ADDRESS>>0x10), (u8 *)Dat, Size*sizeof(u16), HZ*10);
   kfree(Dat);

   //Addr = FIRMWARE_START_ADDRESS;
   //LongAddr = ((0x80+(Addr&0x0F))<<24)|(Addr&0xFFF0);
   COMMAND_E2010(dev, V_CALL_APPLICATION_E2010, (FIRMWARE_START_ADDRESS&0xFFFF), 0);
}



u32 LoadBios_E2010(ldevusb *dev, u8 *Data, u32 size)
{
   u8 *Buf;
   u32 rep_sz;
   int count,status =-1;
   COMMAND_E2010(dev,V_RESET_FPGA_E2010,0,0); // reset fpga
   
   PUT_DATA_MEMORY_E2010(dev, Data, size, SEL_FPGA_DATA); // how words
   
   status=usb_clear_halt(dev->udev, usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr));

   COMMAND_E2010(dev,V_INIT_FPGA_E2010,0,0); // init fpga
   
   // read fpga answer

	Buf = (u8 *)kmalloc(512,GFP_KERNEL);
   rep_sz = 512;
   if(dev->Type==E2010B)
   {
      rep_sz=510;
      PUT_DATA_MEMORY_E2010(dev,(PUCHAR)&rep_sz,4,SEL_BULK_REQ_SIZE);
   }

   
   status = usb_bulk_msg(dev->udev,
               usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
               Buf, rep_sz, &count, HZ*10);
	kfree(Buf);
   
   DbgPrint("read %d bytes from USB status %d", count, status);
   DbgPrint(" Load bios complete \n");
   return status;
}

void EnableE2010(ldevusb *dev, int b)
{
   int i,status = -1;
   u32 rep_sz;
   ADC_PAR_E2010_PACK_U ap;
   ADC_PAR_EXTRA_E2010_PACK_U app;
   
   if(b) // start ADC
   {
      // pack adc parameters
      for(i=0;i<128;i++) ap.t1.Chn[i] = (u8)dev->adcPar.t4.Chn[i];
      ap.t1.Rate = (u8)dev->adcPar.t4.Rate;
      ap.t1.NCh = dev->adcPar.t4.NCh-1;
      ap.t1.Kadr = (u16)dev->adcPar.t4.Kadr;
      ap.t1.ChanMode=dev->adcPar.t4.AdcIMask;
      ap.t1.SyncMode=(u8)dev->adcPar.t4.SynchroType;

      PUT_DATA_MEMORY_E2010(dev, (u8*)&ap.bi, 263, SEL_ADC_PARAM);

      if(dev->Type==E2010B)
      {
         DbgPrint("Set extra sync \n");
         app.t1.StartCnt = dev->adcPar.t4.StartCnt;
         app.t1.StopCnt = dev->adcPar.t4.StopCnt;
         app.t1.SynchroMode = (u16)dev->adcPar.t4.SynchroMode;
         app.t1.AdPorog = (u16)dev->adcPar.t4.AdPorog;
         app.t1.DM_Ena = (u8)dev->adcPar.t4.DM_Ena;

         PUT_DATA_MEMORY_E2010(dev, (u8*)&app.bi, 13, EXTRA_SYNCHRO_PARS_ADDRESS);
      }

      if(dev->Type==E2010B)
      {
         PUT_PM_WORD_E2010(dev, LUSBAPI_OR_LCOMP_ADDRESS, 1);
      }

      if(dev->Type==E2010B)
      {
         rep_sz=dev->wIrqStep*sizeof(u16);
         PUT_DATA_MEMORY_E2010(dev,(u8*)&rep_sz,4,SEL_BULK_REQ_SIZE);
      }


      status=usb_clear_halt(dev->udev, usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr));
      
      COMMAND_E2010(dev, V_START_ADC_E2010,0,1);
      
      DbgPrint("start satrt \n");
   }
   else // stop ADC
   {
      DbgPrint("stop stop \n");
      COMMAND_E2010(dev, V_STOP_ADC_E2010,0,1);
      DbgPrint("pipe aborted \n");
   }
}
