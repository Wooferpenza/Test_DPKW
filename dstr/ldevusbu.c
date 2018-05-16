#include "ldevusb.h"

//////////////////////// HUGE BUFFER FW ////////////////////////////////////
void hb_free(hb *buf)
{
   int i;
   for(i=0;i<buf->nr_pages;i++) free_page((unsigned long)buf->addr[i]);
   kfree(buf->addr);
   kfree(buf);
}

hb *hb_malloc(int pages)
{
   int i;
   hb *buf = (hb *)kmalloc(sizeof(hb),GFP_KERNEL);
   if(buf==NULL) return NULL;
   buf->nr_pages = pages;
   buf->addr = kmalloc(buf->nr_pages*sizeof(u8 *), GFP_KERNEL);
   if(!buf->addr) {kfree(buf); return NULL;}
   memset(buf->addr, 0, buf->nr_pages*sizeof(u8 *));
   for(i=0;i<buf->nr_pages;i++)
   {
      buf->addr[i]=(char *)__get_free_page(GFP_KERNEL);
      if(buf->addr[i]==NULL) { buf->nr_pages=i; hb_free(buf); return NULL; }
   }
   return buf;
}

#define hb_getel(buf, pos, type) (((type *)(buf->addr[pos/(PAGE_SIZE/sizeof(type))]))[pos%(PAGE_SIZE/sizeof(type))])
#define hb_setel(buf, pos, type, data) {((type *)(buf->addr[pos/(PAGE_SIZE/sizeof(type))]))[pos%(PAGE_SIZE/sizeof(type))] = data;}
#define hb_addel(buf, pos, type, data) {((type *)(buf->addr[pos/(PAGE_SIZE/sizeof(type))]))[pos%(PAGE_SIZE/sizeof(type))] += data;}
#define hb_decel(buf, pos, type, data) {((type *)(buf->addr[pos/(PAGE_SIZE/sizeof(type))]))[pos%(PAGE_SIZE/sizeof(type))] -= data;}


int hb_memcpyto(u8 *src, hb *buf, int pos, int size)
{
   int page = pos/PAGE_SIZE;
   int off =  pos%PAGE_SIZE;
   int len = PAGE_SIZE-off;
   do
   {
      if(size<len) len=size;
      memcpy((void *)(buf->addr[page]+off), (void *)src, len);
      src+=len;
      page++;
      size-=len;
      off=0;
      len=PAGE_SIZE;
   } while(size);
   return 0;
}

int hb_memcpyfrom(u8 *dst, hb *buf, int pos, int size)
{
   int page = pos/PAGE_SIZE;
   int off =  pos%PAGE_SIZE;
   int len = PAGE_SIZE-off;
   do
   {
      if(size<len) len=size;
      memcpy((void *)dst, (void *)(buf->addr[page]+off), len);
      dst+=len;
      page++;
      size-=len;
      off=0;
      len=PAGE_SIZE;
   } while(size);
   return 0;
}

//////////////////////////////////////////////////////////////////



// forward decl
static char device_name[] = "ldevusb";
static struct file_operations fops;
static struct usb_driver ldriver;
static void ldevusb_delete(struct kref *kref);

static int ldriver_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
   ldevusb *dev = NULL;
   struct usb_host_interface *iface_desc;
   struct usb_endpoint_descriptor *endpoint;
   int i;
   PLATA_DESCR_E2010 pd;
   int retval = -ENOMEM;

   /* allocate memory for our device state and initialize it */
   dev = kmalloc(sizeof(ldevusb), GFP_KERNEL);
   if (dev == NULL) { DbgPrint("Out of memory"); goto error; }
   memset(dev, 0x00, sizeof (*dev));
   kref_init(&dev->kref);

   dev->Slot = ldev_add();

   dev->udev = usb_get_dev(interface_to_usbdev(interface));
   dev->interface = interface;
   interface->minor=dev->Slot;

   /* set up the endpoint information */
   /* use only the first bulk-in and bulk-out endpoints */
   iface_desc = interface->cur_altsetting;
   for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
   {
      endpoint = &iface_desc->endpoint[i].desc;

      if (!dev->bulk_in_endpointAddr &&
          (endpoint->bEndpointAddress & USB_DIR_IN) &&
          ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
               == USB_ENDPOINT_XFER_BULK))
      {
         dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
      }

      if (!dev->bulk_out_endpointAddr &&
          !(endpoint->bEndpointAddress & USB_DIR_IN) &&
          ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
               == USB_ENDPOINT_XFER_BULK))
      {
         dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
      }
   }
   if(!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr))
   {
      DbgPrint("Could not find both bulk-in and bulk-out endpoints");
      goto error;
   }

   /* save our data pointer in this interface device */
   usb_set_intfdata(interface, dev);

// fill up SLOT_PAR for client
   memset(&dev->sl,0,sizeof(SLOT_PAR));
   switch(id->idProduct)
   {
      case 0x0440: {dev->Type = E440; dev->sl.DSPType = 2185;} break;
      case 0x2010: {dev->Type = E2010; dev->sl.DSPType = 0;} break;
      case 0x0140: {dev->Type = E140; dev->sl.DSPType = 0; } break;
      case 0x0154: {dev->Type = E154; dev->sl.DSPType = 0; }
   }

   switch(dev->Type)
   {
      case E440: break;
      case E140:
      {
      
         char *mn = (char *)kmalloc(256,GFP_KERNEL);
         int status = -1;
   
         status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                     V_GET_MODULE_NAME_E140, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                     0, 0, mn, 255, HZ*10);
         
         DbgPrint("############# module name  %s %x \n",mn, status);
	kfree(mn);
         
      }       break;
      case E154: break;
      case E2010:{
                     DbgPrint("switch to app\n");
                     COMMAND_E2010(dev, V_CALL_APPLICATION_E2010, (BOOT_LOADER_START_ADDRESS&0xFFFF), 0);
                     COMMAND_E2010(dev, V_CALL_APPLICATION_E2010, (FIRMWARE_START_ADDRESS&0xFFFF), 1);
                     
                     ReadFlashWord_E2010(dev, (u16*)&pd, sizeof(pd)/sizeof(u16));
                     DbgPrint("brdname  %s \n",pd.BrdName);
                     DbgPrint("revision %c\n",pd.Rev);

                     if(pd.Rev=='B') { DbgPrint("set to E2010B \n"); dev->Type = E2010B; }
                 } break;
   };

   dev->sl.BoardType = dev->Type;

   init_waitqueue_head(&dev->adc_wq);
   mutex_init(&dev->io_lock);
   dev->dev_flags=0;

   DbgPrint("ldevusb device now attached to ldev%d \n", dev->Slot);

   ldev_register(&fops, dev->Slot);

   return 0;

error:
   if(dev)  kref_put(&dev->kref, ldevusb_delete);
   return retval;
}

static void ldevusb_delete(struct kref *kref)
{
   ldevusb *dev = to_ldevusb_dev(kref);
   DbgPrint("in delete \n");
   usb_put_dev(dev->udev);
   ldev_remove(dev->Slot);
   kfree(dev);
}


static void ldriver_disconnect(struct usb_interface *interface)
{
   ldevusb *dev;

   /* prevent skel_open() from racing skel_disconnect() */
/////!!!!   lock_kernel();

   dev = usb_get_intfdata(interface);
   usb_set_intfdata(interface, NULL);

/////!!!!   unlock_kernel();

   /* decrement our usage count */
   kref_put(&dev->kref, ldevusb_delete);
   DbgPrint("ldev%d now disconnected \n", dev->Slot);
}

static int ldevusb_open(struct inode *inode, struct file *filp)
{
   ldevusb *dev;
   struct usb_interface *interface;
   int subminor;
//   u32 Addr;
//   u32 LongAddr;


   subminor = iminor(inode);
   DbgPrint("subminor %d", subminor);
   interface = usb_find_interface(&ldriver, subminor);
   if(!interface)
   {
      DbgPrint ("%s - error, can't find device for minor %d",  __FUNCTION__, subminor);
      return -ENODEV;
   }

   dev = usb_get_intfdata(interface);
   if(!dev) { return -ENODEV; }

   mutex_lock(&dev->io_lock);
   {
      if(test_bit(FLAG_USED,&dev->dev_flags)) { mutex_unlock(&dev->io_lock); return -EBUSY; }

      set_bit(FLAG_USED,&dev->dev_flags);
      clear_bit(FLAG_WORKING,&dev->dev_flags);

      dev->Sync=0;
      dev->buf=NULL;
      dev->bios=NULL;

/*    dev->Pages=1024;
      dev->OutBulk = vmalloc(dev->Pages*PAGE_SIZE);
      // mark the pages reserved
      for (i = 0; i < (dev->Pages*PAGE_SIZE); i+= PAGE_SIZE)
      {
         SetPageReserved(vmalloc_to_page((void *)(((unsigned long)dev->OutBulk) + i)));
      }
      DbgPrint("Set out Buffer %x %d\n", (unsigned int)dev->OutBulk, dev->Pages);
      if(dev->OutBulk) memset((void *)dev->OutBulk, 0, dev->Pages*PAGE_SIZE);
*/
      dev->urb = usb_alloc_urb(0, GFP_KERNEL);
      if(!dev->urb) { mutex_unlock(&dev->io_lock); return -ENOMEM;}

      kref_get(&dev->kref);
      filp->private_data = dev;
   }
   mutex_unlock(&dev->io_lock);
   return 0;
}

static int ldevusb_release(struct inode *inode, struct file *filp)
{
   ldevusb *dev = (ldevusb *)filp->private_data;
   DbgPrint("in close \n");
   if(dev==NULL) return -ENODEV;

   mutex_lock(&dev->io_lock);
   {
      if(test_bit(FLAG_WORKING,&dev->dev_flags))
      {
         clear_bit(FLAG_WORKING,&dev->dev_flags);
         ldevusb_enable(dev, 0);
         usb_kill_urb(dev->urb);
      }
/*
      if(dev->OutBulk)
      {
         for (i = 0; i < (dev->Pages*PAGE_SIZE); i+= PAGE_SIZE)
         {
            ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)dev->OutBulk) + i)));
         }
         vfree(dev->OutBulk);
         dev->OutBulk=NULL;
      }
*/

      if(dev->Sync) hb_free(dev->Sync);

      if(dev->buf) { usb_free_coherent(dev->udev, dev->wIrqStep*sizeof(u16), dev->buf, dev->urb->transfer_dma);  dev->buf=NULL; }
      if(dev->urb) { usb_free_urb(dev->urb); dev->urb=NULL; }
      if(dev->bios) { kfree(dev->bios); dev->bios=NULL; }

      clear_bit(FLAG_USED,&dev->dev_flags);
      kref_put(&dev->kref, ldevusb_delete);
   }
   mutex_unlock(&dev->io_lock);
   return 0;
}

static /*int*/long ldevusb_ioctl(/*struct inode *inode,*/ struct file *filp, unsigned int cmd, unsigned long arg)
{
ldevusb *dev = (ldevusb *)filp->private_data;
int status = -EFAULT;
PIOCTL_BUFFER ibuf;
u16 Param, Param1;
u16 Dir,Cmd,Addr,Index;
u32 Param32;
u16 AdcCoef[24];

   ibuf = kmalloc(sizeof(IOCTL_BUFFER),GFP_KERNEL);
   if(ibuf==NULL) return status;
   if(copy_from_user(ibuf, (void*)arg, sizeof(IOCTL_BUFFER))) {kfree(ibuf); return status;}

   if(cmd==DIOC_WAIT_COMPLETE)
   {
         DbgPrint("wait start \n");
         wait_event_interruptible(dev->adc_wq, test_bit(FLAG_ADC_EVT,&dev->dev_flags));
         clear_bit(FLAG_ADC_EVT,&dev->dev_flags);
         DbgPrint("wait complete \n");
         return 0;
   }


   mutex_lock(&dev->io_lock);
   {
   switch(cmd)
   {
      case DIOC_GET_PARAMS:
      {
         DbgPrint("in DIOC_GET_PARAMS \n");
         memcpy(ibuf->outBuffer, &(dev->sl), sizeof(SLOT_PAR));
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_SET_DSP_TYPE:
      {
         DbgPrint("In DIOC_SET_DSP_TYPE \n");
         DbgPrint("Only 2185 or no DSP at all\n");
         status = 0;
      } break;

      case DIOC_SEND_COMMAND:
      {
         DbgPrint("in DIOC_SEND_COMMAND \n");
         // 0(dir 0out 1 in) 1 -cmd  2 -addr 3 len/index
         Dir = ((u16 *)ibuf->inBuffer)[0];
         Cmd = ((u16 *)ibuf->inBuffer)[1];
         Addr = ((u16 *)ibuf->inBuffer)[2];
         Index = ((u16 *)ibuf->inBuffer)[3];

         DbgPrint("%d %d %d %d", Dir, Cmd, Addr, Index);

         if(Dir)
         {
            status = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
                     (u8)Cmd, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                     Addr, Index, ibuf->outBuffer, ibuf->outSize, HZ*10);
         }
         else
         {
            status = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                     (u8)Cmd, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                     Addr, Index, ibuf->outBuffer, ibuf->outSize, HZ*10);
         }
         if(status<0) break;
         ibuf->outSize = status;
         if(copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER))) break;
         status=0;
      } break;

      case DIOC_SEND_BIOS: {
         DbgPrint("DIOC_SEND_BIOS \n");
         Param32 = *(u32 *)ibuf->inBuffer;
         DbgPrint("%d \n", Param32);
         if(dev->bios==NULL)
         {
            dev->bios = (u8 *) kmalloc(Param32,GFP_KERNEL);
            if(dev->bios==NULL) break;
            dev->bios_pos = dev->bios;
         }
         memcpy(dev->bios_pos, ibuf->outBuffer, ibuf->outSize);
         dev->bios_pos+=ibuf->outSize;
         status = 0;
      } break;

      case DIOC_LOAD_BIOS:
      {
         DbgPrint("In DIOC_LOAD_BIOS \n");
         Param32 = *(u32 *)ibuf->inBuffer;
         DbgPrint("%d \n", Param32);
         if(dev->bios)
         {
            switch(dev->Type)
            {
               case E2010B:
               case E2010: LoadBios_E2010(dev, dev->bios, Param32); break;
            }
            kfree(dev->bios); dev->bios=NULL;
         }
         status = 0;
      } break;

      case DIOC_READ_FLASH_WORD:
      {
         DbgPrint("In DIOC_READ_FLASH_WORD \n");
         Param = *(u16 *)ibuf->inBuffer;
         switch(dev->Type)
         {
            case E440: ReadFlashWord_E440(dev, Param, (u16 *)ibuf->outBuffer); break;
            case E140: ReadFlashWord_E140(dev, Param, (u16 *)ibuf->outBuffer); break;
            case E2010B:
            case E2010: ReadFlashWord_E2010(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16));break; // all at once
            case E154: ReadFlashWord_E154(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16)); // all at once            
         }
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_WRITE_FLASH_WORD:
      {
         DbgPrint("In DIOC_WRITE_FLASH_WORD \n");
         Param = *(u16 *)ibuf->inBuffer;
         switch(dev->Type)
         {
            case E440: WriteFlashWord_E440(dev, Param, *(u16 *)ibuf->outBuffer); break;
            case E140: WriteFlashWord_E140(dev, Param, *(u16 *)ibuf->outBuffer); break;
            case E2010B:
            case E2010: WriteFlashWord_E2010(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16));break; // all at once
            case E154: WriteFlashWord_E154(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16)); // all at once            
         }
         status = 0;
      } break;

      case DIOC_ENABLE_FLASH_WRITE:
      {
         DbgPrint("In DIOC_ENABLE_FLASH_WRITE \n");
         Param = *(u16 *)ibuf->inBuffer;
         switch(dev->Type)
         {
            case E440: EnableFlashWrite_E440(dev, Param); break;
            case E140: EnableFlashWrite_E140(dev, Param); break;
            case E154: EnableFlashWrite_E154(dev, Param); break;            
            case E2010B:
            case E2010: break;
         }
         status = 0;
      } break;

      case DIOC_COMMAND_PLX: {
            DbgPrint("DIOC_COMMAND_PLX \n");
            Param = *(u16 *)ibuf->inBuffer;
            switch(dev->Type)
            {
               case E440: COMMAND_E440(dev, Param); break;
               case E140: COMMAND_E140(dev, Param);break;
               case E154: COMMAND_E154(dev, Param);break;
            }
            status = 0;
         } break;

      case DIOC_GET_DM_A: {    // METHOD_OUT_DIRECT
            DbgPrint("DIOC_GET_DM_A \n");
            Param = *(u16 *)ibuf->inBuffer;
            switch(dev->Type)
            {
               case E440: GET_DATA_MEMORY_E440(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16), Param); break;
               case E140: GET_DATA_MEMORY_E140(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16), Param); break;
               case E154: GET_DATA_MEMORY_E154(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16), Param); break;               
            }
            status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));;
         } break;

      case DIOC_PUT_DM_A: {   // METHOD_IN_DIRECT
            DbgPrint("DIOC_PUT_DM_A \n");
            Param = *(u16 *)ibuf->inBuffer;
            switch(dev->Type)
            {
               case E440: PUT_DATA_MEMORY_E440(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16), Param); break;
               case E140: PUT_DATA_MEMORY_E140(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16), Param); break;
               case E154: PUT_DATA_MEMORY_E154(dev, (u16 *)ibuf->outBuffer, ibuf->outSize/sizeof(u16), Param); break;
            }
            status = 0;
         } break;

      case DIOC_GET_PM_A: {    // METHOD_OUT_DIRECT
            DbgPrint("DIOC_GET_PM_A \n");
            Param = *(u16 *)ibuf->inBuffer;
            switch(dev->Type)
            {
               case E440: GET_PM_MEMORY_E440(dev, (u32 *)ibuf->outBuffer, ibuf->outSize/sizeof(u32), Param); break;
               case E140: *((u8 *)ibuf->outBuffer) = GET_PM_WORD_E140(dev, Param); break;
               case E154: *((u8 *)ibuf->outBuffer) = GET_PM_WORD_E154(dev, Param); break;
            }
            status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
         } break;

      case DIOC_PUT_PM_A: {   // METHOD_IN_DIRECT
            DbgPrint("DIOC_PUT_PM_A \n");
            Param = *(u16 *)ibuf->inBuffer;
            switch(dev->Type)
            {
               case E440: PUT_PM_MEMORY_E440(dev, (u32 *)ibuf->outBuffer, ibuf->outSize/sizeof(u32), Param); break;
               case E140: PUT_PM_WORD_E140(dev, Param, *((u8 *)ibuf->outBuffer)); break;
               case E154: PUT_PM_WORD_E154(dev, Param, *((u8 *)ibuf->outBuffer)); break;
            }
            status = 0;
         } break;

      case DIOC_ADCSAMPLE: // for e-140 specific...
      {
         DbgPrint("In DIOC_ADCSAMPLE \n");
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         switch(dev->Type)
         {
         case E140:
            {
               Param1 = (u16)*((u32 *)ibuf->inBuffer);
               Param = MAKE_E140CHAN(Param1);

               ldevusb_enable(dev,0);

               PUT_DM_WORD_E140(dev,L_ADC_CHANNEL_SELECT_E140, Param);
               udelay(1000);
               PUT_DM_WORD_E140(dev,L_ADC_CHANNEL_SELECT_E140, Param);
               *((u32 *)ibuf->outBuffer) = GET_DM_WORD_E140(dev,L_ADC_SAMPLE_E140);
            } break;
            
         case E154:
            {
               Param = (u16)*((u32 *)ibuf->inBuffer);
               ldevusb_enable(dev,0);
               PUT_DM_WORD_E154(dev,L_ADC_CHANNEL_SELECT_E154, Param);
               *((u32 *)ibuf->outBuffer) = GET_DM_WORD_E154(dev,L_ADC_SAMPLE_E154);
            } break;
         }            
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_DAC_OUT: // for e20-10 specific...
      {
         DbgPrint("In DIOC_DAC_OUT \n");
         Param = *(u16 *)ibuf->inBuffer;
         PUT_DM_WORD_E2010(dev,SEL_DAC_DATA, Param);
         status = 0;
      } break;

      case DIOC_TTL_OUT: // for e20-10 specific...
      {
         DbgPrint("In DIOC_TTL_OUT \n");
         Param = *(u16 *)ibuf->inBuffer;
         PUT_DM_WORD_E2010(dev,SEL_DIO_DATA, Param);
         status = 0;
      } break;

      case DIOC_TTL_CFG: // for e20-10 specific...
      {
         DbgPrint("In DIOC_TTL_CFG \n");
         Param = *(u16 *)ibuf->inBuffer;
         PUT_PM_WORD_E2010(dev,SEL_DIO_PARAM, (u8)Param);
         status = 0;
      } break;

      case DIOC_TTL_IN: // for e20-10 specific...
      {
         DbgPrint("In DIOC_TTL_IN \n");
         *((u16 *)ibuf->outBuffer) = GET_DM_WORD_E2010(dev, SEL_DIO_DATA); break;
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_ENABLE_CORRECTION: // for e20-10B specific...
      {
         DbgPrint("In DIOC_ENABLE_CORRETION \n");

         if(ibuf->inSize>sizeof(AdcCoef)) break;
         memcpy(AdcCoef,ibuf->inBuffer,ibuf->inSize);

         PUT_PM_WORD_E2010(dev, ADC_CORRECTION_ADDRESS, 1);
         PUT_DATA_MEMORY_E2010(dev, (u8*)AdcCoef, sizeof(AdcCoef), SEL_ADC_CALIBR_KOEFS);
         status = 0;
      } break;

      case DIOC_SETBUFFER:  // METHOD_OUT_DIRECT
      {
         DbgPrint("In SETBUFFER \n");
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;

         dev->wBufferSize = *(u32 *)ibuf->inBuffer;
         DbgPrint("Set Buffer %d \n", dev->wBufferSize);

         /* unmark the pages reserved */
         if(dev->Sync) { hb_free(dev->Sync); }

         dev->RealPages =dev->wBufferSize/2048+(dev->wBufferSize%2048?1:0);
         //dev->Sync = vmalloc((dev->RealPages+1)*PAGE_SIZE);
         dev->Sync=hb_malloc(dev->RealPages+1);
         if(dev->Sync==NULL) break;

         DbgPrint("Set real Buffer %d \n", dev->RealPages*2048);
         *(u32 *)ibuf->outBuffer = (dev->RealPages)*2048; // +1 in dll for mapping pagecount page
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_SETUP: // METHOD_OUT_DIRECT - get ADC_PAR return UserData
      {
         DbgPrint("In SETUP \n");
         if(!dev->Sync) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;

         if(ibuf->inSize>sizeof(WDAQ_PAR)) break;
         memcpy(&dev->adcPar,ibuf->inBuffer,ibuf->inSize);

         if((dev->Type!=E2010)&&(dev->Type!=E2010B)) // E140 E440
         {
            dev->wFIFO=dev->adcPar.t3.FIFO;
            dev->wIrqStep = dev->adcPar.t3.IrqStep;
            dev->wPages = dev->adcPar.t3.Pages;
            DbgPrint(" %x %x %x %x \n",dev->adcPar.t3.Rate,dev->adcPar.t3.NCh,dev->adcPar.t3.Chn[0],dev->adcPar.t3.FIFO);
         }
         else // E2010
         {
            dev->wFIFO=dev->adcPar.t4.FIFO;
            dev->wIrqStep = dev->adcPar.t4.IrqStep;
            dev->wPages = dev->adcPar.t4.Pages;
            DbgPrint(" %x %x %x %x \n",dev->adcPar.t4.Rate,dev->adcPar.t4.NCh,dev->adcPar.t4.Chn[0],dev->adcPar.t4.FIFO);
         }

         if(dev->wIrqStep==0) break;

         DbgPrint("Set Buffer %d %d \n", dev->wPages, dev->wIrqStep);

         SetBoardFIFO(dev);// - УСТАНВЛИВАЕТ ФИФО на плате(( case внутри функции

         if(dev->wPages>(dev->RealPages*2048L/dev->wIrqStep)) dev->wPages=(dev->RealPages*2048/dev->wIrqStep);

         ((u32 *)ibuf->outBuffer)[0] = dev->wPages;  // used size
         ((u32 *)ibuf->outBuffer)[1] = dev->wFIFO;
         ((u32 *)ibuf->outBuffer)[2] = dev->wIrqStep;
         ((u32 *)ibuf->outBuffer)[3] = 0xA5A5A5A5;
         ibuf->outSize=4*sizeof(u32);

         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_INIT_SYNC:
      {
         DbgPrint("In INIT_SYNC \n");
         if(!dev->Sync) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         hb_setel(dev->Sync,0,int,0);
         //if(d->DacBuf) { ((u32 *)d->DacBuf)[0]=0; ((u32 *)d->DacBuf)[1]=0; }
         status = 0;
      } break;

      case DIOC_START: // METHOD_OUT_DIRECT - start ADC
      {
         DbgPrint("In START %d\n", dev->Type);
         if(!dev->Sync) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         set_bit(FLAG_WORKING,&dev->dev_flags);
         dev->buf = usb_alloc_coherent(dev->udev, dev->wIrqStep*sizeof(u16), GFP_KERNEL, &dev->urb->transfer_dma);
         ldevusb_enable(dev, 1);
         status = QueueBulkRead(dev);
      } break;

      case DIOC_STOP: // METHOD_OUT_DIRECT - stop ADC
      {
         DbgPrint("In STOP \n");
         if(!test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         clear_bit(FLAG_WORKING,&dev->dev_flags);
         ldevusb_enable(dev, 0);
         usb_kill_urb(dev->urb);
         if(dev->buf) { usb_free_coherent(dev->udev, dev->wIrqStep*sizeof(short), dev->buf, dev->urb->transfer_dma);  dev->buf=NULL; }
         status = 0;
      } break;

      default: DbgPrint("Unknown IOCTL !!! \n");
   }
   }
   mutex_unlock(&dev->io_lock);
   kfree(ibuf);
   return status;
}

void ldevusb_enable(ldevusb *dev, int flWork)
{
   if(flWork)
   {
      dev->CurPage=0;
      //DacCurPage = 0;
   }

   switch(dev->Type)
   {
   case E440:  EnableE440(dev,flWork); break;
   case E140:  EnableE140(dev,flWork); break;
   case E154:  EnableE154(dev,flWork); break;
   case E2010B:
   case E2010:  EnableE2010(dev,flWork);
   }
   return;
}

/*
static void ldevusb_read_bulk_callback(struct urb *urb, struct pt_regs *regs)
{
ldevusb *dev = (ldevusb *)urb->context;
//u16 *tmp = &dev->Share[dev->wIrqStep*(dev->CurPage++)];
u32 pos;

   if(dev->CurPage==dev->wPages)
   {
      dev->CurPage=0;
      hb_setel(dev->Sync,0,int,0);
   }

   pos = dev->wIrqStep*(dev->CurPage++)*sizeof(short);

   // sync/async unlink faults aren't errors
   if (urb->status &&
       !(urb->status == -ENOENT ||
         urb->status == -ECONNRESET ||
         urb->status == -ESHUTDOWN)) {
      DbgPrint("%s - nonzero write bulk status received: %d",  __FUNCTION__, urb->status);
      return;
   }

//   DbgPrint("in callback \n");
   // free up our allocated buffer
   hb_memcpyto(urb->transfer_buffer, dev->Sync, pos+PAGE_SIZE, urb->actual_length); // смещение на страницу для переменной синхронизации

   hb_addel(dev->Sync,0,int,dev->wIrqStep);


   if((dev->CurPage==dev->wPages) && (dev->adcPar.t3.AutoInit==0))
   {
      set_bit(FLAG_ADC_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->adc_wq);
      return;
   }

   DbgPrint("size %d \n",urb->actual_length);
// when kill urb in dioc_stop - no resubmit guaranted....
   QueueBulkRead(dev);
   return;
}
*/
static void ldevusb_read_bulk_callback(struct urb *urb, struct pt_regs *regs)
{
   ldevusb *dev = (ldevusb *)urb->context;
   u32 pos;
   u32 cnt;
   do
   {
      if (urb->status &&
          !(urb->status == -ENOENT ||
            urb->status == -ECONNRESET ||
            urb->status == -ESHUTDOWN)) {
            DbgPrint("%s - nonzero write bulk status received: %d",  __FUNCTION__, urb->status);
            break;
      }

      pos = dev->wIrqStep*(dev->CurPage)*sizeof(u16);
      hb_memcpyto(urb->transfer_buffer, dev->Sync, pos+PAGE_SIZE, urb->actual_length); // смещение на страницу для переменной синхронизации      
      DbgPrint("size %d \n",urb->actual_length);
      dev->CurPage++;
      cnt = hb_getel(dev->Sync,0,int);
      cnt+=dev->wIrqStep;
      if(dev->CurPage==dev->wPages)
      {
         if(dev->adcPar.t3.AutoInit) dev->CurPage=0;
         else
         {
            DbgPrint("Device stopped event fired");
//            __sync_bool_compare_and_swap(&hb_getel(dev->Sync,0,int),hb_getel(dev->Sync,0,int),cnt);
            __sync_lock_test_and_set(&hb_getel(dev->Sync,0,int),cnt);
//            hb_setel(dev->Sync,0,int,cnt);
            set_bit(FLAG_ADC_EVT,&dev->dev_flags);
            wake_up_interruptible(&dev->adc_wq);
            break;
         }
      }
      else
      {
         if(dev->CurPage==1) cnt = dev->wIrqStep;
      }
//      __sync_bool_compare_and_swap(&hb_getel(dev->Sync,0,int),hb_getel(dev->Sync,0,int),cnt);
      __sync_lock_test_and_set(&hb_getel(dev->Sync,0,int),cnt);
      //hb_setel(dev->Sync,0,int,cnt);
      QueueBulkRead(dev);
   } while(0);
   return;
}

int QueueBulkRead(ldevusb *dev)
{
   int status=-EFAULT;
   do
   {
      // initialize the urb properly
      usb_fill_bulk_urb(dev->urb,
                        dev->udev,
                        usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
                        dev->buf,
                        dev->wIrqStep*sizeof(u16),
                        (usb_complete_t)ldevusb_read_bulk_callback,
                        dev);

      dev->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

      // send the data out the bulk port
      status = usb_submit_urb(dev->urb, GFP_KERNEL);
      if(status)
      {
         DbgPrint("%s - failed submitting read urb, error %d \n", __FUNCTION__, status);
         if(status==-EPERM) DbgPrint("stopped under kill condition \n");
         break;
      }
      status=0;
   } while(status!=0);

   return status;
}

static int ldevusb_mmap(struct file *filp, struct vm_area_struct *vma)
{
ldevusb *dev = (ldevusb *)filp->private_data;
int status;
long length = vma->vm_end - vma->vm_start;
unsigned long start = vma->vm_start;
u8 *vmalloc_area_ptr;
unsigned long pfn;
int pages;
int i=0;

   DbgPrint("In MMAP IOCTL !!! \n");
   DbgPrint("offset field %ld", vma->vm_pgoff);
   if(!(vma->vm_flags & VM_READ)) return -EINVAL;
   //if(vma->vm_flags & VM_WRITE) { vmalloc_area_ptr = dev->OutBulk; DbgPrint("map out buf");}
   //else
   {
      vmalloc_area_ptr = (u8 *)dev->Sync->addr[i];
      pages = dev->RealPages+1;
      DbgPrint("map in buf");
   }
   /* check length - do not allow larger mappings than the number of pages allocated */
   if(length > (pages*PAGE_SIZE)) return -EINVAL;

   vma->vm_flags|=VM_IO;

   /* loop over all pages, map it page individually */
   while (length > 0)
   {
      //pfn = __pa((void *)vmalloc_area_ptr) >> PAGE_SHIFT;
      pfn = page_to_pfn(virt_to_page(vmalloc_area_ptr));
      if((status = remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_SHARED))<0) return status;
      start += PAGE_SIZE;
      vmalloc_area_ptr = (u8 *)dev->Sync->addr[++i];
      length -= PAGE_SIZE;
   }

   return 0;
}


/////////////////////////////////////////////////////////////
// MAIN part
/////////////////////////////////////////////////////////////
/* table of devices that work with this driver */
static struct usb_device_id ldriver_table [] = {
   { USB_DEVICE(0x0471, 0x0440) },
   { USB_DEVICE(0x0471, 0x0140) },
   { USB_DEVICE(0x0471, 0x2010) },
   { USB_DEVICE(0x0471, 0x0154) },   
   {/*            vid     pid */} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, ldriver_table);

static struct file_operations fops = {
   .owner =   THIS_MODULE,
   .open =    ldevusb_open,
   .unlocked_ioctl =   ldevusb_ioctl,
   .mmap =    ldevusb_mmap,
   .release = ldevusb_release,
};

static struct usb_driver ldriver = {
   .name = device_name,
   .id_table = ldriver_table,
   .probe = ldriver_probe,
   .disconnect = ldriver_disconnect,
};

static int __init ldriver_init(void)
{
   int status = usb_register(&ldriver);
   if(status) DbgPrint("usb_register failed. Error number %d \n", status);
   return status;
}

static void __exit ldriver_exit(void)
{
   usb_deregister(&ldriver);
}

module_init (ldriver_init);
module_exit (ldriver_exit);
