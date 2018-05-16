#include "ldevpci.h"

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


static char device_name[] = "ldevpci";
static struct pci_driver l7xx_driver;
static struct file_operations fops;
static void ldevpci_delete(struct kref *kref);


struct _find_ldevpci
{
   int minor;
   ldevpci *dev;
};

static int __find_ldevpci(struct device *dev, void *data)
{
   struct _find_ldevpci *arg = data;
   ldevpci *ldev;
   ldev = pci_get_drvdata(to_pci_dev(dev));
   if(ldev->Slot==arg->minor)
   {
      arg->dev=ldev;
      return 1;
   }
   return 0;
}

ldevpci *find_ldevpci(struct pci_driver *drv, int minor)
{
   struct _find_ldevpci argb;
   int retval;
   argb.minor = minor;
   argb.dev = NULL;
   retval = driver_for_each_device(&drv->driver, NULL, &argb, __find_ldevpci);
   return  argb.dev;
}

static int ldriver_probe(struct pci_dev *device, const struct pci_device_id *id)
{
ldevpci *dev = NULL;
unsigned long addr[6];
unsigned long len[6];
int i;
int status = -ENOMEM;

   do
   {
      if(id == NULL) { status = -ENODEV; break; }
      if((status=pci_enable_device(device))) break;

      DbgPrint("found L7XX board !!!! \n");
      // print assignet resource fo pci board
      for(i=0;i<6;i++)
      {
         addr[i] = pci_resource_start(device, i);
         len[i] = pci_resource_len(device, i);
         DbgPrint("Addr[%d] - %lx : %ld \n", i, addr[i]& PCI_BASE_ADDRESS_IO_MASK,  len[i]);
      }
      DbgPrint( "Irq is %d \n", device->irq);


      /* allocate memory for our device state and initialize it */
      dev = kmalloc(sizeof(ldevpci), GFP_KERNEL);
      if(dev==NULL) { DbgPrint("Out of memory"); break; }
      memset(dev, 0x00, sizeof (*dev));
      kref_init(&dev->kref);

      dev->Slot = ldev_add();

      pci_set_drvdata(device, dev);
      DbgPrint("Slot is %d \n", dev->Slot);

      spin_lock_init(&dev->irq_lock);

      dev->m_IoRange = pci_resource_start(device, 1);
      dev->m_IoRangeLen = pci_resource_len(device, 1);
      dev->m_MemRange = pci_resource_start(device, 3);
      dev->m_MemRangeLen = pci_resource_len(device, 3);

      dev->FLASH_CS_PLX = dev->m_IoRange+0x50;
      dev->FLASH_REG_PLX = dev->m_IoRange+0x50;
      if(id->device==0x9030) dev->FLASH_CS_PLX = dev->m_IoRange+0x54;

      // OnStartDevice code
      request_region(dev->m_IoRange, dev->m_IoRangeLen , device_name);
      request_mem_region(dev->m_MemRange,dev->m_MemRangeLen , device_name);



      // disable and reset interrupt before it assigned
      outl(0x12, dev->m_IoRange+0x4C);
      // mapping irq do later....

      ReadPlataDescr_PLX(dev, &dev->pd);

      DbgPrint("Serial num is %s \n", dev->pd.SerNum);
      DbgPrint("Name num is %s \n", dev->pd.BrdName);
      DbgPrint("Rev num is %c \n", dev->pd.Rev);

      if(dev->pd.Rev=='A') dev->Type = PCIA;
      if(dev->pd.Rev=='B') dev->Type = PCIB;
      if(dev->pd.Rev=='C') dev->Type = PCIC;

      dev->sl.DSPType = 2184;
      switch(dev->pd.DspType[3])
      {
         case '5': { dev->sl.DSPType = 2185; DbgPrint("2185 \n"); } break;
         case '6': { dev->sl.DSPType = 2186; DbgPrint("2186\n"); }
      }

      // fill up SLOT_PAR for client
      dev->sl.Base = dev->m_IoRange;
      dev->sl.BaseL = dev->m_IoRangeLen;
      dev->sl.Base1 = 0;
      dev->sl.BaseL1 = 0;
      dev->sl.Mem = dev->m_MemRange;
      dev->sl.MemL = dev->m_MemRangeLen;
      dev->sl.Mem1 = 0;
      dev->sl.MemL1 = 0;
      dev->sl.Irq = device->irq;
      DbgPrint("IRQ is %d \n", device->irq);
      dev->sl.BoardType = dev->Type;
      dev->sl.Dma = 0;
      dev->sl.DmaDac = 0;
      // switch to advanced processor by DIOC_SET_DSP_TYPE
      /////////////////////////////////////////////
      DbgPrint("Success \n");

      init_waitqueue_head(&dev->adc_wq);
      init_waitqueue_head(&dev->dac_wq);
      mutex_init(&dev->io_lock);
      dev->dev_flags=0;

      dev->pci_dev=device;

      // register in LDEV driver
      ldev_register(&fops, dev->Slot);
      status = 0;
   } while(status!=0);

   if(status&&dev)  kref_put(&dev->kref, ldevpci_delete);
   return status;
}

static void ldevpci_delete(struct kref *kref)
{
   ldevpci *dev = to_ldevpci_dev(kref);
   DbgPrint("in delete \n");
   release_region(dev->m_IoRange,dev->m_IoRangeLen);
   release_mem_region(dev->m_MemRange,dev->m_MemRangeLen);
   ldev_remove(dev->Slot);
   kfree(dev);
}


static void ldriver_remove(struct pci_dev *device)
{
   ldevpci *dev = pci_get_drvdata(device);
   int slot = dev->Slot;
   DbgPrint( "Slot is %d \n", dev->Slot);
   kref_put(&dev->kref, ldevpci_delete);
   DbgPrint("ldev%d now disconnected \n", slot);
}

static int ldevpci_open(struct inode *inode, struct file *filp)
{
ldevpci *dev;
   DbgPrint("Open Call!!!! \n");
   dev = find_ldevpci(&l7xx_driver, iminor(inode));
   if(dev==NULL){ DbgPrint("No device \n"); return -ENODEV;}

   mutex_lock(&dev->io_lock);
   {
      if(test_bit(FLAG_USED,&dev->dev_flags)) { mutex_unlock(&dev->io_lock); return -EBUSY; }
      set_bit(FLAG_USED,&dev->dev_flags);;
      clear_bit(FLAG_WORKING,&dev->dev_flags);

      dev->Sync =0;
      dev->DacBuf=0;
      dev->DSPBase=0x2000; // default 2184

      dev->m_Map = pci_iomap(dev->pci_dev, 3, dev->m_MemRangeLen);

      // уточнить до абсолютных адресов...
      switch(dev->Type)
      {
         case PCIA: {
                        DbgPrint("Tune type PCIA \n");
                        dev->sl.DTA_REG = 0x0;         // Data port
                        dev->DATA_REG_PLX  = (u16 *)dev->m_Map;//ioremap(dev->m_MemRange,sizeof(u16));         // Data port

                        dev->sl.IDMA_REG = 0x2<<11;         // IDMA port
                        dev->IDMA_REG_PLX  = (u16 *)dev->m_Map+2048;//ioremap(dev->m_MemRange+(0x2<<11),sizeof(u16));         // IDMA port

                        dev->sl.CMD_REG = 0x4<<11;         // irq2 DSP
                        dev->CMD_REG_PLX   = (u16 *)dev->m_Map+4096;//ioremap(dev->m_MemRange+(0x4<<11),sizeof(u16));         // irq2 DSP

                        dev->sl.IRQ_RST = 0x6<<11;         // irq reset
                        dev->IRQ_RST_PLX   = (u16 *)dev->m_Map+6144;//ioremap(dev->m_MemRange+(0x6<<11),sizeof(u16));         // irq reset

                        dev->sl.DTA_ARRAY = 0x0;
                        dev->DTA_ARRAY_PLX = (u32 *)dev->m_Map;//ioremap(dev->m_MemRange+0x0,1024*sizeof(u32));

                        dev->FLASH_REG_PLX = dev->m_IoRange+0x50;
                        dev->FLASH_CS_PLX = dev->m_IoRange+0x50;
                     } break;

         case PCIB: {
                        DbgPrint("Tune type PCIB \n");
                        dev->sl.DTA_REG = 0x0;         // Data port
                        dev->DATA_REG_PLX  = (u16 *)dev->m_Map;//ioremap(dev->m_MemRange,sizeof(u16));         // Data port

                        dev->sl.IDMA_REG = 0x2;         // IDMA port
                        dev->IDMA_REG_PLX  = (u16 *)dev->m_Map+1;//ioremap(dev->m_MemRange+0x2,sizeof(u16));         // IDMA port

                        dev->sl.CMD_REG = 0x4;         // irq2 DSP
                        dev->CMD_REG_PLX   = (u16 *)dev->m_Map+2;//ioremap(dev->m_MemRange+0x4,sizeof(u16));         // irq2 DSP

                        dev->sl.IRQ_RST = 0x6;         // irq reset
                        dev->IRQ_RST_PLX   = (u16 *)dev->m_Map+3;//ioremap(dev->m_MemRange+0x6,sizeof(u16));         // irq reset

                        dev->sl.DTA_ARRAY = 0x1000;
                        dev->DTA_ARRAY_PLX = (u32 *)dev->m_Map+1024;//ioremap(dev->m_MemRange+0x1000,1024*sizeof(u32));

                        dev->FLASH_REG_PLX = dev->m_IoRange+0x50;
                        dev->FLASH_CS_PLX = dev->m_IoRange+0x50;
                     } break;

         case PCIC: {
                        DbgPrint("Tune type PCIC \n");

                        dev->sl.DTA_REG = 0x0;         // Data port
                        dev->DATA_REG_PLX  = (u16 *)dev->m_Map;//ioremap(dev->m_MemRange,sizeof(u16));         // Data port

                        dev->sl.IDMA_REG = 0x2;         // IDMA port
                        dev->IDMA_REG_PLX  = (u16 *)dev->m_Map+1;//ioremap(dev->m_MemRange+0x2,sizeof(u16));         // IDMA port

                        dev->sl.CMD_REG = 0x4;         // irq2 DSP
                        dev->CMD_REG_PLX   = (u16 *)dev->m_Map+2;//ioremap(dev->m_MemRange+0x4,sizeof(u16));         // irq2 DSP

                        dev->sl.IRQ_RST = 0x6;         // irq reset
                        dev->IRQ_RST_PLX   = (u16 *)dev->m_Map+3;//ioremap(dev->m_MemRange+0x6,sizeof(u16));         // irq reset

                        dev->sl.DTA_ARRAY = 0x1000;
                        dev->DTA_ARRAY_PLX = (u32 *)dev->m_Map+1024;//ioremap(dev->m_MemRange+0x1000,1024*sizeof(u32));

                        dev->FLASH_REG_PLX = dev->m_IoRange+0x50;
                        dev->FLASH_CS_PLX = dev->m_IoRange+0x54;
                  }
      }
      // install irq handler
      if(request_irq(dev->sl.Irq, ldevpci_ISR, IRQF_SHARED/*SA_SHIRQ*/, device_name, (void*)dev))
      {
         DbgPrint("can't get IRQ %d !\n", dev->sl.Irq);
         return -EFAULT;
      }

      kref_get(&dev->kref);
      filp->private_data = (void *)dev;
   }
   mutex_unlock(&dev->io_lock);
   return 0; // success
}

static int ldevpci_close(struct inode *inode, struct file *filp)
{
//int i;
ldevpci *dev = (ldevpci *)filp->private_data;

   DbgPrint("Close Call!!!! \n");
   if(dev==NULL) return -ENODEV;

   mutex_lock(&dev->io_lock);
   {
      if(test_bit(FLAG_WORKING,&dev->dev_flags))
      {
         clear_bit(FLAG_WORKING,&dev->dev_flags);
         ldevpci_enable(dev,0);
      }

      free_irq(dev->sl.Irq, (void*)dev);

      /* unmark the pages reserved */
      if(dev->Sync) { hb_free(dev->Sync); dev->Sync=NULL; }
      if(dev->DacBuf) { hb_free(dev->DacBuf); dev->DacBuf=NULL; }

      pci_iounmap(dev->pci_dev,dev->m_Map);

      clear_bit(FLAG_USED,&dev->dev_flags);
      kref_put(&dev->kref, ldevpci_delete);
   }
   mutex_unlock(&dev->io_lock);
   return 0;
}


static /*int*/long ldevpci_ioctl(/*struct inode *inode,*/ struct file *filp, unsigned int cmd, unsigned long arg)
{
ldevpci *dev = (ldevpci *)filp->private_data;
int status = -EFAULT;
PIOCTL_BUFFER ibuf;
u32 data;
//int i;
u16 Param;
int TO;
u16 dsp,base,d1;

   ibuf = kmalloc(sizeof(IOCTL_BUFFER),GFP_KERNEL);
   if(ibuf==NULL) return status;
   if(copy_from_user(ibuf, (void*) arg, sizeof(IOCTL_BUFFER))) {  kfree(ibuf); return status; }

   if(cmd==DIOC_WAIT_COMPLETE)
   {
      DbgPrint("wait start \n");
      wait_event_interruptible(dev->adc_wq, test_bit(FLAG_ADC_EVT,&dev->dev_flags));
      clear_bit(FLAG_ADC_EVT,&dev->dev_flags);
      DbgPrint("wait complete \n");
      return 0;
   }

   if(cmd==DIOC_WAIT_COMPLETE_DAC)
   {
      DbgPrint("wait start dac \n");
      wait_event_interruptible(dev->dac_wq, test_bit(FLAG_DAC_EVT,&dev->dev_flags));
      clear_bit(FLAG_DAC_EVT,&dev->dev_flags);
      DbgPrint("wait complete dac\n");
      return 0;
   }

   mutex_lock(&dev->io_lock);
   {
   switch(cmd)
   {
      case DIOC_GET_PARAMS:
      {
         DbgPrint("In DIOC_GET_PARAMS \n");
         memcpy(ibuf->outBuffer,&(dev->sl), sizeof(SLOT_PAR));
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_READ_FLASH_WORD:
      {
//         DbgPrint("In DIOC_READ_FLASH_WORD \n");
         Param = *(u16 *)ibuf->inBuffer;
         ReadFlashWord_PLX(dev, Param, (u16 *)ibuf->outBuffer);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_WRITE_FLASH_WORD:
      {
//         DbgPrint("In DIOC_WRITE_FLASH_WORD \n");
         Param = *(u16 *)ibuf->inBuffer;
         WriteFlashWord_PLX(dev, Param, *(u16 *)ibuf->outBuffer);
         status = 0;
      } break;

      case DIOC_ENABLE_FLASH_WRITE:
      {
         DbgPrint("In DIOC_ENABLE_FLASH_WRITE \n");
         Param = *(u16 *)ibuf->inBuffer;
         EnableFlashWrite_PLX(dev, Param);
         status = 0;
      } break;

      case DIOC_SET_DSP_TYPE:
      {
         DbgPrint("In DIOC_SET_DSP_TYPE \n");
         dsp = 0;
         base = 2;
         switch(dev->sl.DSPType)
         {
            case 2185: {dsp=1;base=0;} break;
            case 2186: {dsp=2;base=1;}
         }
         // setup bios to new dsp type
         PUT_DM_WORD_PCI(dev, L_READY_PLX, 0);
         PUT_DM_WORD_PCI(dev, L_DSP_TYPE_PLX, dsp);
         COMMAND_760(dev,cmSET_DSP_TYPE_PLX);

         if(dev->sl.DSPType!=2184) dev->DSPBase=0x3000;

         PUT_DM_WORD_PCI(dev, L_DSP_TYPE_PLX, dsp);

         TO=10000000;
         do
         {
            d1=GET_DM_WORD_PCI(dev,L_READY_PLX);
         } while(!d1&&(TO--));
         if(TO==-1) break;

         PUT_DM_WORD_PCI(dev, L_ADC_FIFO_BASE_ADDRESS_INDEX_PLX,base);
         COMMAND_760(dev,cmADC_FIFO_CONFIG_PLX);

         status = 0;
      } break;

      case DIOC_RESET_PLX:
      {
         DbgPrint("In DIOC_RESET_PLX \n");
         data = inl(dev->FLASH_REG_PLX);
         data &= (0x40000000 ^ 0xFFFFFFFF);
         outl(data,dev->FLASH_REG_PLX);
         data |= 0x40000000;
         outl(data,dev->FLASH_REG_PLX);
         data &= (0x40000000 ^ 0xFFFFFFFF);
         outl(data,dev->FLASH_REG_PLX);
         status = 0;
      } break;

      case DIOC_COMMAND_PLX:
      case DIOC_GET_DM_A:
      case DIOC_PUT_DM_A:
      case DIOC_GET_PM_A:
      case DIOC_PUT_PM_A:
      {
         status = ldevpci_syncISR(dev,cmd,arg);
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
         dev->Sync = hb_malloc(dev->RealPages+1);

         DbgPrint("Set Buffer %p %d\n", dev->Sync, dev->RealPages);
         DbgPrint("Set real Buffer %d \n", dev->RealPages*2048);

         *(u32 *)ibuf->outBuffer = (dev->RealPages)*2048; // +1 in dll for mapping pagecount page
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_SETBUFFER_1:  // METHOD_OUT_DIRECT
      {
         DbgPrint("In SETBUFFER_1 \n");
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;

         dev->wDacBufferSize = *(u32 *)ibuf->inBuffer;
         DbgPrint("Set Buffer %d \n", dev->wDacBufferSize);

         /* unmark the pages reserved */
         if(dev->DacBuf) { hb_free(dev->DacBuf); }

         dev->DacRealPages = dev->wDacBufferSize/2048+(dev->wDacBufferSize%2048?1:0);
         dev->DacBuf = hb_malloc(dev->DacRealPages+1);

         DbgPrint("Set Buffer %p %d\n", dev->DacBuf, dev->DacRealPages);
         DbgPrint("Set real Buffer %d \n", dev->DacRealPages*2048);

         *(u32 *)ibuf->outBuffer = (dev->DacRealPages)*2048; // +1 page in dll for mapping pagecount page
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_SETUP_DAC: // METHOD_OUT_DIRECT // setup dac parameters
      {
         DbgPrint("In SETUP_DAC \n");
         if(!dev->DacBuf) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;

//         dacPar=*(PDAC_PAR_I)ioBuffer;  // Fifo DacStep DacFreq DacNumber Pages
         if(ibuf->inSize>sizeof(WDAQ_PAR)) break;
         memcpy(&dev->dacPar,ibuf->inBuffer,ibuf->inSize);

         dev->wDacPages=dev->dacPar.t1.Pages;
         dev->wDacStep=dev->dacPar.t1.IrqStep;
         dev->wDacFIFO=dev->dacPar.t1.FIFO;
         dev->wDacIrqEna = dev->dacPar.t1.IrqEna;
         dev->wDacEna = dev->dacPar.t1.DacEna;
         dev->wDacNumber = dev->dacPar.t1.DacNumber;

         if(dev->wDacStep==0) break;

         SetDACparameters_PLX(dev);  // config dac and preinit it

         if(dev->wDacPages>(dev->DacRealPages*2048L/dev->wDacStep)) dev->wDacPages=(dev->DacRealPages*2048/dev->wDacStep); // correct buffer size

         if((dev->Type==PCIC)&&(dev->wDacIrqEna))
            if(dev->wDacPages>(dev->DacRealPages*1024L/dev->wDacStep)) dev->wDacPages=(dev->DacRealPages*1024/dev->wDacStep); // correct buffer size


         ((u32 *)ibuf->outBuffer)[0] = dev->wDacPages;  // used size
         ((u32 *)ibuf->outBuffer)[1] = dev->wDacFIFO;
         ((u32 *)ibuf->outBuffer)[2] = dev->wDacStep;
         ((u32 *)ibuf->outBuffer)[3] = 0xA2A2A2A2;
         ibuf->outSize=4*sizeof(u32);

         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));

      } break;

      case DIOC_SETUP:
      {
         DbgPrint("In SETUP \n");
         if(!dev->Sync) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;

//         adcPar=*(PADC_PAR_I)ioBuffer;
         if(ibuf->inSize>sizeof(WDAQ_PAR)) break;
         memcpy(&dev->adcPar,ibuf->inBuffer,ibuf->inSize);

         dev->wFIFO = dev->adcPar.t3.FIFO;
         dev->wIrqStep = dev->adcPar.t3.IrqStep;
         dev->wPages = dev->adcPar.t3.Pages;

         if(dev->wIrqStep==0) break;

         DbgPrint("%u %u %u %u \n",dev->adcPar.t3.Rate,dev->adcPar.t3.NCh,dev->adcPar.t3.Chn[0],dev->adcPar.t3.FIFO);
         DbgPrint("Set Buffer %u %u \n",dev->wPages,dev->wIrqStep);

         SetBoardFIFO(dev);

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
         if(dev->DacBuf) { hb_setel(dev->DacBuf,0,int,0);hb_setel(dev->DacBuf,1,int,0); }
         status = 0;
      } break;

      case DIOC_START: // METHOD_OUT_DIRECT - start ADC
      {
         DbgPrint("In START \n");
         if(!dev->Sync) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         set_bit(FLAG_WORKING,&dev->dev_flags);

         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         dev->CurPage=0;
         dev->DacCurPage=0;
         ldevpci_enable(dev, 1);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;

      case DIOC_STOP: // METHOD_OUT_DIRECT - stop ADC
      {
         DbgPrint("In STOP \n");
         if(!test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         clear_bit(FLAG_WORKING,&dev->dev_flags);

         spin_lock_irqsave(&(dev->irq_lock),dev->flags); // аналог synchronizeinterrupt
         ldevpci_enable(dev, 0);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status = 0;
      } break;

      default : DbgPrint("Unknown IOCTL !!! \n");
   }
   }
   mutex_unlock(&dev->io_lock);
   kfree(ibuf);
   return status;
}


static int ldevpci_mmap(struct file *filp, struct vm_area_struct *vma)
{
ldevpci *dev = (ldevpci *)filp->private_data;

int ret;
long length = vma->vm_end - vma->vm_start;
unsigned long start = vma->vm_start;
u8 *malloc_area_ptr;// = (char *)dev->Sync;
unsigned long pfn;
int pages;
int i=0;

   DbgPrint("In MMAP IOCTL !!! \n");
   if(!(vma->vm_flags & VM_READ)) return -EINVAL;
   if(vma->vm_flags &VM_WRITE)
   {
      malloc_area_ptr = (u8 *)dev->DacBuf->addr[i];
      pages = dev->DacRealPages+1;
   }
   else
   {
      malloc_area_ptr = (u8 *)dev->Sync->addr[i];
      pages = dev->RealPages+1;
   }
   /* check length - do not allow larger mappings than the number of pages allocated */
   if (length > pages*PAGE_SIZE) return -EINVAL;

   vma->vm_flags|=VM_IO;
   /* loop over all pages, map it page individually */
   while (length > 0)
   {
      //pfn = __pa((void *)malloc_area_ptr) >> PAGE_SHIFT;
      pfn = page_to_pfn(virt_to_page(malloc_area_ptr));
      if((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_SHARED))<0) { return ret; }
      start += PAGE_SIZE;
      if(vma->vm_flags &VM_WRITE) malloc_area_ptr = (u8 *)dev->DacBuf->addr[++i];
      else malloc_area_ptr = (u8 *)dev->Sync->addr[++i];

      length -= PAGE_SIZE;
   }

   return 0;
}

int ldevpci_syncISR(ldevpci *dev, unsigned int cmd, unsigned long arg)
{
int status = -EFAULT;
PIOCTL_BUFFER ibuf;
void *inBuf, *outBuf;
int ioOutSize;
u16 Param;

   ibuf = kmalloc(sizeof(IOCTL_BUFFER),GFP_KERNEL);
   if(ibuf==NULL) return status;
   if(copy_from_user(ibuf,(void*)arg,sizeof(IOCTL_BUFFER))) { kfree(ibuf); return status; }

   Param = *(u16 *)ibuf->inBuffer;
   inBuf = outBuf = (void *)ibuf->outBuffer;
   ioOutSize = ibuf->outSize;


   switch(cmd)
   {
      case DIOC_COMMAND_PLX:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         DbgPrint("command \n");
         COMMAND_760(dev, Param);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;

      case DIOC_GET_DM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         GET_DATA_MEMORY_PCI(dev, (u16 *)outBuf, ioOutSize/sizeof(u16), Param);
         DbgPrint("get dm a %x %x \n", Param, ((u16 *)outBuf)[0]);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));

      } break;

      case DIOC_PUT_DM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         DbgPrint("put dm a \n");
         PUT_DATA_MEMORY_PCI(dev, (u16 *)inBuf, ioOutSize/sizeof(u16), Param);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;

      case DIOC_GET_PM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         DbgPrint("get pm a \n");
         GET_PM_MEMORY_PCI(dev, (u32 *)outBuf, ioOutSize/sizeof(u32), Param);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
      } break;

      case DIOC_PUT_PM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         PUT_PM_MEMORY_PCI(dev, (u32 *)inBuf, ioOutSize/sizeof(u32), Param);
         DbgPrint("put pm a %x %x \n", Param, ((u32 *)inBuf)[0]);         
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;
   }

   kfree(ibuf);
   return status;
}

void IRQ_ADC_RESET_PCI(ldevpci *dev)
{
   iowrite16(0, dev->IRQ_RST_PLX);
}

void IRQ_DAC_RESET_PCI(ldevpci *dev)
{
   u32 dl= inl(dev->m_IoRange+0x54);
   outl(((0xFFFFFFFFL ^ (0x1L << 11)) & dl ),dev->m_IoRange+0x54);
   outl(((0x1L << 11) | dl ),dev->m_IoRange+0x54);
}


void SetBoardFIFO(ldevpci *dev)
{
   PUT_DM_WORD_PCI(dev, L_ADC_NEW_FIFO_LENGTH_PLX, (u16)2*dev->wFIFO);
   COMMAND_760(dev, cmADC_FIFO_CONFIG_PLX);
   dev->wFIFO = GET_DM_WORD_PCI(dev, L_ADC_FIFO_LENGTH_PLX)/2;
   if(dev->wIrqStep>=dev->wFIFO*2) dev->wIrqStep = dev->wFIFO - dev->wFIFO%dev->adcPar.t3.NCh; // protection
}

void SetDACparameters_PLX(ldevpci *dev)
{
   PUT_DM_WORD_PCI(dev, L_DAC_NEW_FIFO_LENGTH_PLX, (u16)2*dev->wDacFIFO);
   COMMAND_760(dev, cmDAC_FIFO_CONFIG_PLX);
   dev->wDacFIFO = GET_DM_WORD_PCI(dev, L_DAC_FIFO_LENGTH_PLX)/2;
   dev->wDacStep = dev->wDacFIFO;
}

void ldevpci_enable(ldevpci *dev, int b)
{
int i,j;
u32 IrqMask;
u32 *d;
u16 DACBase;
   if(b)
   {
		// L-760 PCI board
      DbgPrint("Enable \n");

      PUT_DM_WORD_PCI(dev,L_FIRST_SAMPLE_DELAY_PLX,(u16)dev->adcPar.t3.FPDelay);
      PUT_DM_WORD_PCI(dev,L_ADC_RATE_PLX,(u16)dev->adcPar.t3.Rate);
      PUT_DM_WORD_PCI(dev,L_INTER_KADR_DELAY_PLX,(u16)dev->adcPar.t3.Kadr);
      COMMAND_760(dev,cmSET_ADC_KADR_PLX);

      PUT_DM_WORD_PCI(dev,L_CONTROL_TABLE_LENGHT_PLX,(u16)dev->adcPar.t3.NCh);

      for(i=0;i<dev->adcPar.t3.NCh;i++) PUT_DM_WORD_PCI(dev,(u16)(L_CONTROL_TABLE_PLX+i),(u16)dev->adcPar.t3.Chn[i]);
      COMMAND_760(dev,cmLOAD_CONTROL_TABLE_PLX);

		// DAC configuration
      if(dev->DacBuf)
      {
         DACBase = GET_DM_WORD_PCI(dev, L_DAC_FIFO_BASE_ADDRESS_PLX);

			// preinit without interrupt short buffer format
         d = kmalloc(dev->wDacFIFO*2*sizeof(u32),GFP_KERNEL);     // wDacFIFO
         for(j=0;j<dev->wDacStep*2;j++) d[j]=hb_getel(dev->DacBuf, 2048+j, short);//dev->DacBuf[j];
         PUT_PM_MEMORY_PCI(dev, d, dev->wDacStep*2, DACBase); // preinit dac buffer
         kfree(d);

         if((dev->Type==PCIC)&&dev->wDacIrqEna)  // разрешены прерывания от ЦАП - тогда другой формат буфера sort 0 short 0
         {
				// тут преинициализация с прерываниями
            DbgPrint("Init for Irq \n");
            HB_PUT_PM_MEMORY_PCI(dev, dev->DacBuf, 1024,dev->wDacStep*2, DACBase); // preinit dac buffer
            PUT_DM_WORD_PCI(dev, L_DAC_ENABLE_IRQ_VALUE_PLX,(u16)dev->wDacIrqEna);
            PUT_DM_WORD_PCI(dev, L_DAC_IRQ_STEP_PLX,(u16)dev->wDacStep);
         }

         dev->DacCurPage = 2; // preinit in dsp ram

         DbgPrint("DacRate %d \n",dev->dacPar.t1.Rate);

         PUT_DM_WORD_PCI(dev, L_DAC_RATE_PLX,(u16)dev->dacPar.t1.Rate);
         COMMAND_760(dev,cmSET_DAC_RATE_PLX);

         PUT_DM_WORD_PCI(dev, L_DAC_ENABLE_STREAM_PLX,(u16)dev->wDacEna);  // enable dac
      }
		////
      PUT_DM_WORD_PCI(dev,L_ENABLE_IRQ_VALUE_PLX,(u16)dev->adcPar.t3.IrqEna);
      PUT_DM_WORD_PCI(dev,L_ADC_ENABLE_PLX,(u16)dev->adcPar.t3.AdcEna); // 1
      PUT_DM_WORD_PCI(dev,L_IRQ_STEP_PLX,(u16)dev->wIrqStep);
      COMMAND_760(dev,cmENABLE_IRQ_PLX);

		// Synchronization
      PUT_DM_WORD_PCI(dev, L_SYNCHRO_TYPE_PLX, (u16)dev->adcPar.t3.SynchroType);
      PUT_DM_WORD_PCI(dev, L_SYNCHRO_AD_SENSITIVITY_PLX, (u16)dev->adcPar.t3.SynchroSensitivity);
      PUT_DM_WORD_PCI(dev, L_SYNCHRO_AD_MODE_PLX, (u16)dev->adcPar.t3.SynchroMode);
      PUT_DM_WORD_PCI(dev, L_SYNCHRO_AD_CHANNEL_PLX, (u16)dev->adcPar.t3.AdChannel);
      PUT_DM_WORD_PCI(dev, L_SYNCHRO_AD_POROG_PLX, (u16)dev->adcPar.t3.AdPorog);

      IrqMask = 0x53;
      IRQ_ADC_RESET_PCI(dev);

		// preset user pins in plx and reset dac irq
      if(dev->Type==PCIC)
      {
         outl((0x1L << 5) | (inl(dev->m_IoRange+0x54)),dev->m_IoRange+0x54);
         outl((0x1L << 11) | (inl(dev->m_IoRange+0x54)),dev->m_IoRange+0x54);

         IRQ_DAC_RESET_PCI(dev);
         IrqMask = 0x5B;
      }

      outl(IrqMask,dev->m_IoRange+0x4C);

      COMMAND_760(dev,cmENABLE_DAC_STREAM_PLX);
      COMMAND_760(dev,cmSYNCHRO_CONFIG_PLX);

//      PUT_DM_WORD_PCI(dev,L_ENABLE_IRQ_VALUE_PLX,1);
//      PUT_DM_WORD_PCI(dev,L_ADC_ENABLE_PLX,1); // 1
//      COMMAND_760(dev,cmENABLE_IRQ_PLX);
//      COMMAND_760(dev,cmIRQ_TEST_PLX);
   }
   else
   {
      DbgPrint("Disable \n");
      outl(0x12,dev->m_IoRange+0x4C);

      IRQ_ADC_RESET_PCI(dev);

      if(dev->Type==PCIC) IRQ_DAC_RESET_PCI(dev);

      PUT_DM_WORD_PCI(dev,L_ENABLE_IRQ_VALUE_PLX,0);
      PUT_DM_WORD_PCI(dev,L_ADC_ENABLE_PLX,0);

      if(dev->wDacIrqEna) PUT_DM_WORD_PCI(dev, L_DAC_ENABLE_IRQ_PLX,0);

      if(dev->dacPar.t1.AutoInit==0)
      {
         PUT_DM_WORD_PCI(dev, L_DAC_ENABLE_STREAM_PLX,(u16)dev->dacPar.t1.AutoInit);
         COMMAND_760(dev,cmENABLE_DAC_STREAM_PLX);
      }

      COMMAND_760(dev,cmENABLE_IRQ_PLX);
   }
}


// irq handler
irqreturn_t ldevpci_ISR(int irq, void *dev_id)//, struct pt_regs *regs)
{
   ldevpci *dev = (ldevpci *)dev_id;
   int Read_0, Read_1, Write_0, Write_1;
   u32 source;
   int tmp1,tmp3;
   int base,base1;
   int posr, posw;

   spin_lock(&(dev->irq_lock));

   if(!dev) { spin_unlock(&(dev->irq_lock)); return IRQ_NONE; }
   if(dev->sl.Irq != irq) { spin_unlock(&(dev->irq_lock)); return IRQ_NONE; }

   // check if irq shared
   source = inl(dev->m_IoRange+0x4C)&0x24;
   if(!source) { /*DbgPrint("Not my ISR \n");*/ spin_unlock(&(dev->irq_lock)); return IRQ_NONE; }

   if(!test_bit(FLAG_WORKING,&dev->dev_flags))
   {
         DbgPrint("Not my working \n");
         IRQ_DAC_RESET_PCI(dev);
         IRQ_ADC_RESET_PCI(dev);
         return IRQ_NONE;
   }

   if(source&0x20) // DAC
   {
      if(GET_DM_WORD_PCI(dev,L_DAC_IRQ_SOURCE_PLX)==0x2) // dac stop
      {
         set_bit(FLAG_DAC_EVT,&dev->dev_flags);
         wake_up_interruptible(&dev->dac_wq);
      }
      else
      {
         // dac put data
         if(dev->DacCurPage==dev->wDacPages)
         {
            dev->DacCurPage = 0;
            hb_setel(dev->DacBuf,0,int,0);
         }

         posw=1024+dev->wDacStep*(dev->DacCurPage++); // add 1024 to skeep Sync page

         tmp3 = GET_DM_WORD_PCI(dev,L_DAC_IRQ_FIFO_ADDRESS_PLX);
         base1 = GET_DM_WORD_PCI(dev,L_DAC_FIFO_BASE_ADDRESS_PLX);

         Write_0 = (tmp3+dev->wDacStep) - (base1+2*dev->wDacFIFO);
         if(Write_0>0) { Write_1 = Write_0; Write_0 = dev->wDacStep - Write_0; }
         else { Write_0 = dev->wDacStep; Write_1 = 0; }

         HB_PUT_PM_MEMORY_PCI(dev,dev->DacBuf,posw,Write_0,tmp3); //Read_0
         if(Write_1) HB_PUT_PM_MEMORY_PCI(dev,dev->DacBuf,posw+Write_0,Write_1,base1); // improved

         hb_addel(dev->DacBuf,0,int,dev->wDacStep);
      }

      IRQ_DAC_RESET_PCI(dev);
   }
   if(source&0x4)
   {
      if(dev->CurPage==dev->wPages)  // counter show how much data read
      {
         dev->CurPage = 0;
         hb_setel(dev->Sync,0,int,0);
      }

      posr = 2048+dev->wIrqStep*(dev->CurPage++); // add 2048 to skeep Sync page


      tmp1 = GET_DM_WORD_PCI(dev,L_IRQ_FIFO_ADDRESS_PLX);
      base = GET_DM_WORD_PCI(dev,L_ADC_FIFO_BASE_ADDRESS_PLX);

      Read_0 = (tmp1+dev->wIrqStep) - (base+2*dev->wFIFO);
      if(Read_0>0) { Read_1 = Read_0; Read_0 = dev->wIrqStep - Read_0; }
      else { Read_0 = dev->wIrqStep; Read_1 = 0; }

      HB_GET_DATA_MEMORY_PCI(dev,dev->Sync,posr,Read_0,tmp1); //Read_0
      if(Read_1) HB_GET_DATA_MEMORY_PCI(dev,dev->Sync,posr+Read_0,Read_1,base); // improved

      hb_addel(dev->Sync,0,int,dev->wIrqStep);

      if((dev->CurPage==dev->wPages)&&(dev->adcPar.t3.AutoInit==0))
      {
         ldevpci_enable(dev,0);
         set_bit(FLAG_ADC_EVT,&dev->dev_flags);
         wake_up_interruptible(&dev->adc_wq);
      }

      IRQ_ADC_RESET_PCI(dev);
   }

   spin_unlock(&(dev->irq_lock));

   return IRQ_HANDLED;
}


/////////////////////////////////////////////////////////////
// MAIN part
/////////////////////////////////////////////////////////////
static struct pci_device_id ids[] =
{
   {0x10b5,0x9050,0x3631,0x4c37,0,0,0},
   {0x10b5,0x9050,0x3830,0x4c37,0,0,0},
   {0x10b5,0x9050,0x3833,0x4c37,0,0,0},
   {0x10b5,0x9030,0x3830,0x4c37,0,0,0},
   {0x10b5,0x9030,0x3833,0x4c37,0,0,0},
   {0,0,0,0,0,0,0}
};
MODULE_DEVICE_TABLE(pci, ids);


static struct file_operations fops =
{
   .owner = THIS_MODULE,
   .unlocked_ioctl   = ldevpci_ioctl,
   .mmap    = ldevpci_mmap,
   .open    = ldevpci_open,
   .release = ldevpci_close,
};

static struct pci_driver l7xx_driver =
{
   .name = device_name,
   .id_table = ids,
   .probe = ldriver_probe,
   .remove = ldriver_remove,
};

static int __init ldriver_init(void)
{
   return pci_register_driver(&l7xx_driver);
}

static void __exit ldriver_exit(void)
{
   pci_unregister_driver(&l7xx_driver);
}

module_init(ldriver_init);
module_exit(ldriver_exit);
