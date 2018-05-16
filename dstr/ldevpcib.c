#include "ldevpcib.h"

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
      buf->addr[i]=(u8 *)__get_free_page(GFP_DMA32);
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


static char device_name[] = "ldevpcibm";
static struct pci_driver l791_driver;
static struct file_operations fops;
static void ldevpcib_delete(struct kref *kref);


struct _find_ldevpcib
{
   int minor;
   ldevpcib *dev;
};

static int __find_ldevpcib(struct device *dev, void *data)
{
   struct _find_ldevpcib *arg = data;
   ldevpcib *ldev;
   ldev = pci_get_drvdata(to_pci_dev(dev));
   if(ldev->Slot==arg->minor)
   {
      arg->dev=ldev;
      return 1;
   }
   return 0;
}

ldevpcib *find_ldevpcib(struct pci_driver *drv, int minor)
{
   struct _find_ldevpcib argb;
   int retval;
   argb.minor = minor;
   argb.dev = NULL;
   retval = driver_for_each_device(&drv->driver, NULL, &argb, __find_ldevpcib);
   return argb.dev;
}

static int ldriver_probe(struct pci_dev *device, const struct pci_device_id *id)
{
ldevpcib *dev = NULL;
u32 addr[6];
u32 len[6];
int i;
int status = -ENOMEM;

   do
   {
      if(id == NULL) { status = -ENODEV; break; }
      if((status=pci_enable_device(device))) break;

      DbgPrint("found L791 board !!!! \n");
      // print assignet resource fo pci board
      for(i=0;i<6;i++)
      {
         addr[i] = pci_resource_start(device, i);
         len[i] = pci_resource_len(device, i);
         DbgPrint("Addr[%d] - %lx : %d \n", i, addr[i]& PCI_BASE_ADDRESS_IO_MASK,  len[i]);
      }
      DbgPrint( "Irq is %d \n", device->irq);



      /* allocate memory for our device state and initialize it */
      dev = kmalloc(sizeof(ldevpcib), GFP_KERNEL);
      if(dev==NULL) { DbgPrint("Out of memory"); break; }
      memset(dev, 0x00, sizeof (*dev));
      kref_init(&dev->kref);

      dev->Slot = ldev_add();

      pci_set_drvdata(device, dev);
      DbgPrint("Slot is %d \n", dev->Slot);

      spin_lock_init(&dev->irq_lock);

      dev->m_MemRange = pci_resource_start(device, 0);
      dev->m_MemRangeLen = pci_resource_len(device, 0);

      request_mem_region(dev->m_MemRange,dev->m_MemRangeLen , device_name);

      // fill up SLOT_PAR for client
      dev->sl.Base = 0;
      dev->sl.BaseL = 0;
      dev->sl.Base1 = 0;
      dev->sl.BaseL1 = 0;
      dev->sl.Mem = dev->m_MemRange;
      dev->sl.MemL = dev->m_MemRangeLen;
      dev->sl.Mem1 = 0;
      dev->sl.MemL1 = 0;
      dev->sl.Irq = device->irq; DbgPrint("IRQ is %d \n", device->irq);
      dev->sl.BoardType = L791;
      dev->sl.Dma = 0;
      dev->sl.DmaDac = 0;
      // switch to advanced processor by DIOC_SET_DSP_TYPE
      /////////////////////////////////////////////
      DbgPrint("Success \n");

      init_waitqueue_head(&dev->adc_bm_wq);
      init_waitqueue_head(&dev->adc_ovf_wq);
      init_waitqueue_head(&dev->adc_buf_wq);
      init_waitqueue_head(&dev->dac_usr_wq);
      init_waitqueue_head(&dev->dac_unf_wq);
      init_waitqueue_head(&dev->power_wq);

      mutex_init(&dev->io_lock);
      dev->dev_flags=0;
//////////////////////!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      dev->pci_dev = device;
/*
      dev->L791_REG_ADDR = pci_iomap(dev->pci_dev, 0, 1024*sizeof(u32));
      ReadPlataDescr_L791(dev,&dev->pd);
      DbgPrint("FLASH  Ser %s Name %s Quartz %d \n",dev->pd.SerNum,dev->pd.BrdName, dev->pd.Quartz);
      pci_iounmap(dev->pci_dev,dev->L791_REG_ADDR);
*/
      // register in LDEV driver
      ldev_register(&fops, dev->Slot);
      status = 0;
   } while(status!=0);

   if(status&&dev)  kref_put(&dev->kref, ldevpcib_delete);
   return status;
}

static void ldevpcib_delete(struct kref *kref)
{
   ldevpcib *dev = to_ldevpcib_dev(kref);
   DbgPrint("in delete \n");
   release_mem_region(dev->m_MemRange,dev->m_MemRangeLen);
   ldev_remove(dev->Slot);
   kfree(dev);
}


static void ldriver_remove(struct pci_dev *device)
{
   ldevpcib *dev = pci_get_drvdata(device);
   int slot = dev->Slot;
   DbgPrint( "Slot is %d \n", dev->Slot);
   kref_put(&dev->kref, ldevpcib_delete);
   DbgPrint("ldev%d now disconnected \n", slot);
}

static int ldevpcib_open(struct inode *inode, struct file *filp)
{
ldevpcib *dev;
   DbgPrint("Open Call!!!! \n");
   dev = find_ldevpcib(&l791_driver, iminor(inode));
   if(dev==NULL){ DbgPrint("No device \n"); return -ENODEV;}


   mutex_lock(&dev->io_lock);
   {
      if(test_bit(FLAG_USED,&dev->dev_flags)) { mutex_unlock(&dev->io_lock); return -EBUSY; }
      set_bit(FLAG_USED,&dev->dev_flags);;
      clear_bit(FLAG_WORKING,&dev->dev_flags);


      dev->Sync =0;
      dev->DacBuf=0;
      dev->m_MaxXferRegs = 128;

      dev->L791_REG_ADDR = pci_iomap(dev->pci_dev, 0, 1024*sizeof(u32));

      ReadPlataDescr_L791(dev,&dev->pd);
      DbgPrint("FLASH  Ser %s Name %s \n",dev->pd.SerNum,dev->pd.BrdName);
      DbgPrint("VER  Ser %x \n",ioread32(dev->L791_REG_ADDR+0xFF4));
      // alloc sglist and fill it 0
    
      dev->sglist = vmalloc(2*dev->m_MaxXferRegs * sizeof(*dev->sglist));
      
      /* prior 2.6.24
      if (!dev->sglist){ DbgPrint("dma_region_alloc: vmalloc(sglist) failed\n"); return -EFAULT; }
      memset(dev->sglist, 0, 2*dev->m_MaxXferRegs*sizeof(*dev->sglist));
      */
      sg_init_table(dev->sglist, 2*dev->m_MaxXferRegs);

      // install irq handler
      if(request_irq(dev->sl.Irq, ldevpcib_isr_791, IRQF_SHARED/*SA_SHIRQ*/, device_name, (void*)dev))
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

static int ldevpcib_close(struct inode *inode, struct file *filp)
{
ldevpcib *dev = (ldevpcib *)filp->private_data;

   DbgPrint("Close Call!!!! \n");
   if(dev==NULL) return -ENODEV;

   mutex_lock(&dev->io_lock);
   {
      if(test_bit(FLAG_WORKING,&dev->dev_flags))
      {
         clear_bit(FLAG_WORKING,&dev->dev_flags);
         LDevPciB_Enable791(dev,0);
      }

      free_irq(dev->sl.Irq, (void*)dev);

      vfree(dev->sglist);
      dev->sglist = NULL;

      if(dev->Sync) { hb_free(dev->Sync); dev->Sync=NULL; }
      if(dev->DacBuf) { hb_free(dev->DacBuf); dev->DacBuf=NULL; }

      pci_iounmap(dev->pci_dev,dev->L791_REG_ADDR);

      clear_bit(FLAG_USED,&dev->dev_flags);
      kref_put(&dev->kref, ldevpcib_delete);
   }
   mutex_unlock(&dev->io_lock);
   return 0;
}


static /*int*/long ldevpcib_ioctl(/*struct inode *inode,*/ struct file *filp, unsigned int cmd, unsigned long arg)
{
ldevpcib *dev = (ldevpcib *)filp->private_data;
int status = -EFAULT;
PIOCTL_BUFFER ibuf;
u16 Param;
u32 Param32;
u32 ctrl;

   ibuf = kmalloc(sizeof(IOCTL_BUFFER),GFP_KERNEL);
   if(ibuf==NULL) return status;
   if(copy_from_user(ibuf, (void*) arg, sizeof(IOCTL_BUFFER))) {  kfree(ibuf); return status; }

   if(cmd==DIOC_WAIT_COMPLETE)
   {
      DbgPrint("wait start adc\n");
      wait_event_interruptible(dev->adc_bm_wq, test_bit(FLAG_ADC_EVT,&dev->dev_flags));
      clear_bit(FLAG_ADC_EVT,&dev->dev_flags);
      DbgPrint("wait complete adc\n");
      return 0;
   }

   if(cmd==DIOC_WAIT_COMPLETE_DAC)
   {
      DbgPrint("wait start dac \n");
      wait_event_interruptible(dev->dac_usr_wq, test_bit(FLAG_DAC_EVT,&dev->dev_flags));
      clear_bit(FLAG_DAC_EVT,&dev->dev_flags);
      DbgPrint("wait complete dac\n");
      return 0;
   }
   if(cmd==DIOC_WAIT_COMPLETE_ADC_OVF)
   {
      DbgPrint("wait start adc ovf\n");
      wait_event_interruptible(dev->adc_ovf_wq, test_bit(FLAG_ADC_OVF_EVT,&dev->dev_flags));
      clear_bit(FLAG_ADC_OVF_EVT,&dev->dev_flags);
      DbgPrint("wait complete adc ovf\n");
      return 0;
   }

   if(cmd==DIOC_WAIT_COMPLETE_ADC_BUF)
   {
      DbgPrint("wait start adc buf \n");
      wait_event_interruptible(dev->adc_buf_wq, test_bit(FLAG_ADC_BUF_EVT,&dev->dev_flags));
      clear_bit(FLAG_ADC_BUF_EVT,&dev->dev_flags);
      DbgPrint("wait complete adc buf\n");
      return 0;
   }
   if(cmd==DIOC_WAIT_COMPLETE_DAC_UNF)
   {
      DbgPrint("wait start dac unf\n");
      wait_event_interruptible(dev->dac_unf_wq, test_bit(FLAG_DAC_UNF_EVT,&dev->dev_flags));
      clear_bit(FLAG_DAC_UNF_EVT,&dev->dev_flags);
      DbgPrint("wait complete dac unf\n");
      return 0;
   }

   if(cmd==DIOC_WAIT_COMPLETE_PWR)
   {
      DbgPrint("wait start pwr \n");
      wait_event_interruptible(dev->power_wq, test_bit(FLAG_PWR_EVT,&dev->dev_flags));
      clear_bit(FLAG_PWR_EVT,&dev->dev_flags);
      DbgPrint("wait complete pwr\n");
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
         DbgPrint("In DIOC_READ_FLASH_WORD \n");
         Param = *(u16 *)ibuf->inBuffer;
         ReadFlashWord_L791(dev, Param, (u16 *)ibuf->outBuffer);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_WRITE_FLASH_WORD:
      {
         DbgPrint("In DIOC_WRITE_FLASH_WORD \n");
         Param = *(u16 *)ibuf->inBuffer;
         WriteFlashWord_L791(dev, Param, *(u16 *)ibuf->outBuffer);
         status = 0;
      } break;

      case DIOC_ENABLE_FLASH_WRITE:
      {
         DbgPrint("In DIOC_ENABLE_FLASH_WRITE \n");
         Param = *(u16 *)ibuf->inBuffer;
         if(WriteFlashPage_L791(dev, *(u16 *)ibuf->outBuffer)) DbgPrint("done \n");
         status = 0;
      } break;

      case DIOC_SET_DSP_TYPE:
      {
         DbgPrint("In DIOC_SET_DSP_TYPE \n");
         status = 0;
      } break;

      case DIOC_RESET_PLX:
      {
         DbgPrint("In DIOC_RESET_PLX \n");
         status = 0;
      } break;

      case DIOC_ADCSAMPLE:
      {
         DbgPrint("In DIOC_ADCSAMPLE \n");
         Param32 = *(u32 *)ibuf->inBuffer; //chan

         //Enable791(FALSE);
         ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
         SETBIT(ctrl,2);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         udelay(10);
         CLEARBIT(ctrl,2);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         
         iowrite32(Param32,dev->L791_REG_ADDR+R_CONTROL_TABLE_L791);
         iowrite32(0,dev->L791_REG_ADDR+R_CONTROL_TABLE_LENGTH_L791);
         iowrite32(1,dev->L791_REG_ADDR+R_ADC_SAMPLE_QNT_L791);
         ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
         SETBIT(ctrl,0); SETBIT(ctrl,4);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         while(ioread32(dev->L791_REG_ADDR+R_CONTROL_L791)&0x1);
         *((u32 *)ibuf->outBuffer) = ioread32(dev->L791_REG_ADDR+R_ADC_BUFFER_L791);
         
         ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
         SETBIT(ctrl,2);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         udelay(10);
         CLEARBIT(ctrl,2);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

      case DIOC_DAC_OUT:
      {
         DbgPrint("In DIOC_DACSAMPLE \n");
         Param32 = *(u32 *)ibuf->inBuffer;
         
//         Enable791(FALSE);
         // reset counter ADC and DAC
         ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
         SETBIT(ctrl,18);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         udelay(10);
         CLEARBIT(ctrl,18);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         
         ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
         SETBIT(ctrl,16);
         iowrite32(Param32,dev->L791_REG_ADDR+R_DAC_BUFFER_L791);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);         
         iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
         status =0;
      } break;

      case DIOC_TTL_OUT:
      {
         DbgPrint("In DIOC_TTL_OUT \n");
         Param = *(u16 *)ibuf->inBuffer;
         iowrite32((u32)Param, dev->L791_REG_ADDR+R_DIGITAL_IO_L791);
         status = 0;
      } break;

      case DIOC_TTL_CFG:
      {
         DbgPrint("In DIOC_TTL_CFG \n");
         Param = *(u16 *)ibuf->inBuffer;
         ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
         if(Param&0x1) SETBIT(ctrl,28);
         else CLEARBIT(ctrl,28);
         CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
         iowrite32(ctrl,dev->L791_REG_ADDR+R_CONTROL_L791);
         status = 0;
      } break;

      case DIOC_TTL_IN:
      {
         DbgPrint("In DIOC_TTL_IN \n");
         *((u16 *)ibuf->outBuffer) = (u16)ioread32(dev->L791_REG_ADDR+R_DIGITAL_IO_L791);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
      } break;

// because always 256 kwords buffer for output and so buffer for input - no realloc
// if alloc - simple return..
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      case DIOC_SETBUFFER:  // METHOD_OUT_DIRECT
      case DIOC_SETBUFFER_1:  // METHOD_OUT_DIRECT
      {
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         do
         {
//          if(dev->Sync) break;//hb_free(dev->Sync);
//          if(dev->DacBuf) break;//hb_free(dev->DacBuf);

            DbgPrint("In SETBUFFER \n");
            dev->wBufferSize = dev->m_MaxXferRegs*(PAGE_SIZE>>1);
            DbgPrint("Req Buffer sz %d \n", dev->wBufferSize);
            if(dev->Sync==NULL) dev->Sync = hb_malloc(dev->m_MaxXferRegs);
            if(dev->Sync==NULL) { DbgPrint("failed alloc \n"); break;}
            DbgPrint("Alloc Buffer %p %d\n", dev->Sync, dev->m_MaxXferRegs);

            DbgPrint("In SETBUFFER_1 \n");
            dev->wDacBufferSize = dev->m_MaxXferRegs*(PAGE_SIZE>>1);
            DbgPrint("Req Buffer sz %d \n", dev->wDacBufferSize);
            if(dev->DacBuf==NULL) dev->DacBuf = hb_malloc(dev->m_MaxXferRegs);
            if(dev->DacBuf==NULL) { DbgPrint("failed alloc \n"); break;}
            DbgPrint("Alloc Buffer %p %d\n", dev->DacBuf, dev->m_MaxXferRegs);
////////////////////////////////////////////
            status = 0;
         } while(status);

         *(u32 *)ibuf->outBuffer = (dev->m_MaxXferRegs-1)*(PAGE_SIZE>>1); // it equal RealPages...
         // dec 1 page. add later in dll for 791. for correct size for all boards
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

         dev->wDacPages=dev->dacPar.t2.Pages;
         dev->wDacStep=dev->dacPar.t2.IrqStep;
         dev->wDacFIFO=dev->dacPar.t2.FIFO;
         dev->wDacIrqEna = dev->dacPar.t2.IrqEna;
         dev->wDacEna = dev->dacPar.t2.DacEna;

//         if(dev->wDacStep==0) break;

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

         dev->wFIFO = dev->adcPar.t4.FIFO;
         dev->wIrqStep = dev->adcPar.t4.IrqStep;
         dev->wPages = dev->adcPar.t4.Pages;

         if(dev->wIrqStep==0) break;

         DbgPrint("%x %x %x %x \n",dev->adcPar.t4.Rate,dev->adcPar.t4.NCh,dev->adcPar.t4.Chn[0],dev->adcPar.t4.FIFO);
         DbgPrint("Set Buffer %d %d \n",dev->wPages,dev->wIrqStep);

         SetBoardFIFO(dev);

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
         status = 0;
      } break;

      case DIOC_START: // METHOD_OUT_DIRECT - start ADC
      {
         DbgPrint("In START \n");
         if(!dev->Sync) break;
         if(test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         set_bit(FLAG_WORKING,&dev->dev_flags);
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         LDevPciB_Enable791(dev, 1);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;

      case DIOC_STOP: // METHOD_OUT_DIRECT - stop ADC
      {
         DbgPrint("In STOP \n");
         if(!test_bit(FLAG_WORKING,&dev->dev_flags)) break;
         clear_bit(FLAG_WORKING,&dev->dev_flags);
         spin_lock_irqsave(&(dev->irq_lock),dev->flags); // synchronizeinterrupt
         LDevPciB_Enable791(dev, 0);
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


// in vma->vm_pgoff as offset send code for mapping switch....
// in userspace code 0x1000 0x2000 0x3000, in kernel 1 2 3 (in adc) (out dac) (i/o pci bm)

static int ldevpcib_mmap(struct file *filp, struct vm_area_struct *vma)
{
ldevpcib *dev = (ldevpcib *)filp->private_data;
int ret;
int length = vma->vm_end - vma->vm_start;
unsigned long start = vma->vm_start;
u8 *malloc_area_ptr=0;
unsigned long pfn;
unsigned long code = vma->vm_pgoff; // code for map region
int pages = dev->m_MaxXferRegs;
int i=0;

   DbgPrint("In MMAP IOCTL !!! \n");
   DbgPrint("code %ld \n", vma->vm_pgoff);

   vma->vm_flags|=VM_IO;

   if(!(vma->vm_flags & VM_READ)) return -EINVAL;

   /* loop over all pages, map it page individually */
   if(code==0x3)
   {
      if(!(vma->vm_flags & VM_WRITE)) return -EINVAL;
      if (length > PAGE_SIZE) return -EINVAL;
      if((ret=io_remap_pfn_range(vma, vma->vm_start, dev->m_MemRange>>PAGE_SHIFT, PAGE_SIZE, vma->vm_page_prot))<0) { return ret; }
   }
   else
   {
      if((code==0x2) && !(vma->vm_flags & VM_WRITE)) return -EINVAL;
      if (length > pages*PAGE_SIZE) return -EINVAL;
      while (length > 0)
      {
         if(code==0x1) malloc_area_ptr = (u8 *)dev->Sync->addr[i];
         if(code==0x2) malloc_area_ptr = (u8 *)dev->DacBuf->addr[i];
         //pfn = __pa((void *)malloc_area_ptr) >> PAGE_SHIFT;
         pfn = page_to_pfn(virt_to_page(malloc_area_ptr));
         if((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_SHARED))<0) { return ret; }
         start += PAGE_SIZE;
         length -= PAGE_SIZE;
         i++;
      }
   }
   return 0;
}

int ldevpcib_syncint(ldevpcib *dev, unsigned int cmd, unsigned long arg)
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
   {/*
      case DIOC_COMMAND_PLX:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         COMMAND_760(dev, Param);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;

      case DIOC_GET_DM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         GET_DATA_MEMORY_PCI(dev, (u16 *)outBuf, ioOutSize/sizeof(u16), Param);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));

      } break;

      case DIOC_PUT_DM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         PUT_DATA_MEMORY_PCI(dev, (u16 *)inBuf, ioOutSize/sizeof(u16), Param);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;

      case DIOC_GET_PM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         GET_PM_MEMORY_PCI(dev, (u32 *)outBuf, ioOutSize/sizeof(u32), Param);
         status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
      } break;

      case DIOC_PUT_PM_A:
      {
         spin_lock_irqsave(&(dev->irq_lock),dev->flags);
         PUT_PM_MEMORY_PCI(dev, (u32 *)inBuf, ioOutSize/sizeof(u32), Param);
         spin_unlock_irqrestore(&(dev->irq_lock),dev->flags);
         status=0;
      } break;*/
   }

   kfree(ibuf);
   return status;
}


void SetBoardFIFO(ldevpcib *dev)
{
	u16 i;
	if(dev->adcPar.t4.FIFO>128) dev->wFIFO = 128;
	else dev->wFIFO = dev->adcPar.t4.FIFO & 0xFF;
	for(i=0;dev->wFIFO>1;i++,dev->wFIFO>>=1);
	dev->wFIFO = 1<<i;
	DbgPrint("fifo %d \n", dev->wFIFO);
	if((dev->wPages*dev->wIrqStep > (128*1024))||(dev->adcPar.t4.AutoInit) ) {dev->wPages = 128; dev->wIrqStep=1024;}
}


void LDevPciB_Enable791(ldevpcib *dev, int b)
{
u32 ctrl;
int i;
u32 tFIFO,data;
u32 irqs = 0;
struct scatterlist *sgentry;
   if(b)
   {
      DbgPrint("enable \n");

      /* fill scatter/gather list with pages */
      for (i = 0; i < dev->m_MaxXferRegs; i++)
      {
        /* prior 2.6.24
         dev->sglist[i].page = virt_to_page((void *)dev->Sync->addr[i]);
         dev->sglist[i].length = PAGE_SIZE;
         dev->sglist[dev->m_MaxXferRegs+i].page = virt_to_page((void *)dev->DacBuf->addr[i]);
         dev->sglist[dev->m_MaxXferRegs+i].length = PAGE_SIZE;
         */
         sg_set_page(&dev->sglist[i], virt_to_page((void *)dev->Sync->addr[i]), PAGE_SIZE, 0);
         sg_set_page(&dev->sglist[dev->m_MaxXferRegs+i], virt_to_page((void *)dev->DacBuf->addr[i]), PAGE_SIZE, 0);
         
      }

      dev->m_RealXferRegs = pci_map_sg(dev->pci_dev, dev->sglist, 2*dev->m_MaxXferRegs, DMA_BIDIRECTIONAL);
      if (dev->m_RealXferRegs == 0) { DbgPrint("pci_map_sg() failed\n"); return; }

      if(dev->m_RealXferRegs!=(2*dev->m_MaxXferRegs))
      {
         DbgPrint("optimized sglist or bad-alloced buffer\n");
         // no optim in kernel code, so simply bad buffer...
         return;
      }

      DbgPrint("pci_map_sg() result %d\n", dev->m_RealXferRegs);

//      for (i = 0; i < 2*dev->m_MaxXferRegs; i++)
//      {
//         iowrite32(sg_dma_address(&dev->sglist[i]), dev->L791_REG_ADDR+R_ADC_PAGE_DESC_L791+4*i);
//      }
      for_each_sg(dev->sglist,sgentry,2*dev->m_MaxXferRegs,i)
      {
        iowrite32(sg_dma_address(sgentry), dev->L791_REG_ADDR+R_ADC_PAGE_DESC_L791+4*i);
      }

      iowrite32(dev->adcPar.t4.Rate, dev->L791_REG_ADDR+R_CHANNEL_TIME_L791);
      iowrite32(dev->adcPar.t4.Kadr, dev->L791_REG_ADDR+R_INT_FRAME_TIME_L791);

      DbgPrint("%d %d \n",dev->adcPar.t4.NCh, dev->adcPar.t4.Chn[dev->adcPar.t4.NCh-1]);

      iowrite32(dev->adcPar.t4.NCh-1, dev->L791_REG_ADDR+R_CONTROL_TABLE_LENGTH_L791);

      for(i=0; i < (dev->adcPar.t4.NCh/2 + dev->adcPar.t4.NCh%2); i++)
      {
         data = (u32)dev->adcPar.t4.Chn[2*i] | (((u32)dev->adcPar.t4.Chn[2*i+1]) << 16);
         iowrite32(data, dev->L791_REG_ADDR+R_CONTROL_TABLE_L791+4*i);
      }

      ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791) & 0xFFFF0000; // reset all adc bits

      // synchro config //////////////////// 0,1 - no 2 - dig start 3 - kadr sync
      if(dev->adcPar.t4.SynchroType>1) SETBIT(ctrl,9);
      if(dev->adcPar.t4.SynchroType==3) SETBIT(ctrl,8);
      if(dev->adcPar.t4.SynchroSrc) SETBIT(ctrl,10);

      //////////////////////////////////////
      // fifo config
      tFIFO = dev->wFIFO;
      for(i=0;tFIFO>1;i++,tFIFO>>=1);
      if(i&0x1) SETBIT(ctrl,12);
      if(i&0x2) SETBIT(ctrl,13);
      if(i&0x4) SETBIT(ctrl,14);

      //SETBIT(ctrl,12);
      //SETBIT(ctrl,13);
//      CLEARBIT(ctrl,12);
//      CLEARBIT(ctrl,13);
//      SETBIT(ctrl,14);
      //////////////////////////////////////

      //// AutoInit config /////////////////
      if(dev->adcPar.t4.AutoInit==0) SETBIT(ctrl,3); // sey autostop

      iowrite32(dev->wIrqStep*dev->wPages-1, dev->L791_REG_ADDR+R_ADC_MASTER_QNT_L791);
      if(dev->adcPar.t4.AdcEna)
      {
         SETBIT(ctrl,BIT_ADC_EN);
         SETBIT(ctrl,BIT_ADC_MASTER_EN);
      }

// config dac output

      iowrite32(dev->dacPar.t2.Rate, dev->L791_REG_ADDR+R_DAC_TIME_L791);
      DbgPrint("DAC rate %d \n",dev->dacPar.t2.Rate);

      if(dev->dacPar.t2.DacEna)
      {
         SETBIT(ctrl,BIT_DAC_EN);
         SETBIT(ctrl,BIT_DAC_MASTER_EN);
         DbgPrint("DAC enable \n");
      }

      // Irq enable
      iowrite32(0xFFFFFFFF, dev->L791_REG_ADDR+R_STATUS_L791);  // reset irq


      if(dev->dacPar.t2.IrqEna) // in irq ena bits for dac
      {
         irqs = (dev->dacPar.t2.IrqEna<<16)|0x80000000;
      }

      if(dev->adcPar.t4.IrqEna) // in irq ena bits for adc
      {
         irqs = (irqs |dev->adcPar.t4.IrqEna)|0x80000000;
      }

      if(irqs) iowrite32(irqs, dev->L791_REG_ADDR+R_INTERRUPT_ENABLE_L791);

      CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
      iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
   }
   else
   {
      DbgPrint("disable \n");

      // stop ADC and DAC
      ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791) & 0xFF000000;
      CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
      iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);

      // disable ints
      iowrite32(0x00000000, dev->L791_REG_ADDR+R_INTERRUPT_ENABLE_L791);
      iowrite32(0xFFFFFFFF, dev->L791_REG_ADDR+R_STATUS_L791);

      // reset counter ADC and DAC
      ctrl = ioread32(dev->L791_REG_ADDR+R_CONTROL_L791);
      SETBIT(ctrl,2);
      SETBIT(ctrl,18);
      CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
      iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);
      udelay(10);
      CLEARBIT(ctrl,2);
      CLEARBIT(ctrl,18);
      CLEARBIT(ctrl,24);CLEARBIT(ctrl,25);CLEARBIT(ctrl,26);
      iowrite32(ctrl, dev->L791_REG_ADDR+R_CONTROL_L791);

      pci_unmap_sg(dev->pci_dev, dev->sglist, 2*dev->m_MaxXferRegs, DMA_BIDIRECTIONAL);
   }
}


// irq handler
irqreturn_t ldevpcib_isr_791(int irq, void *dev_id)//, struct pt_regs *regs)
{
   ldevpcib *dev = (ldevpcib *)dev_id;

   u32 source;
   u32  mask;
   u32 irqs;

   spin_lock(&(dev->irq_lock));

   if(!dev) { spin_unlock(&(dev->irq_lock)); return IRQ_NONE; }
   if(dev->sl.Irq != irq) { spin_unlock(&(dev->irq_lock)); return IRQ_NONE; }

   source = ioread32(dev->L791_REG_ADDR+R_STATUS_L791);
   mask = ioread32(dev->L791_REG_ADDR+R_INTERRUPT_ENABLE_L791);

   // check if irq shared
   if(!(source&0x80000000)) { /*DbgPrint("Not my ISR \n");*/ spin_unlock(&(dev->irq_lock)); return IRQ_NONE; }

   irqs = source&mask;

   if(!test_bit(FLAG_WORKING,&dev->dev_flags))
   {
         DbgPrint("Not my working \n");
         iowrite32(irqs, dev->L791_REG_ADDR+R_STATUS_L791);
         return IRQ_NONE;
   }

   if(irqs&0x00000001)
   {
      DbgPrint("Not my working adc bm \n");// adc bm
      set_bit(FLAG_ADC_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->adc_bm_wq);
   }
   if(irqs&0x00000002)
   {
      DbgPrint("Not my working adc ovf \n");// adc ovf
      set_bit(FLAG_ADC_OVF_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->adc_ovf_wq);
   }
   if(irqs&0x00000008)
   {
      DbgPrint("Not my working adc buf \n");// adc buf
      set_bit(FLAG_ADC_BUF_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->adc_buf_wq);
   }
   if(irqs&0x00010000)
   {
      DbgPrint("Not my working dac user pin \n");// dac user
      set_bit(FLAG_DAC_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->dac_usr_wq);
   }
   if(irqs&0x00040000)
   {
      DbgPrint("Not my working dac unf \n");// dac unf
      set_bit(FLAG_DAC_UNF_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->dac_unf_wq);
   }
   if(irqs&0x01000000)
   {
      DbgPrint("Not my working power failure \n");// power failure
      set_bit(FLAG_PWR_EVT,&dev->dev_flags);
      wake_up_interruptible(&dev->power_wq);
   }

   iowrite32(irqs, dev->L791_REG_ADDR+R_STATUS_L791);
   spin_unlock(&(dev->irq_lock));
   return IRQ_HANDLED;
}


/////////////////////////////////////////////////////////////
// MAIN part
/////////////////////////////////////////////////////////////
static struct pci_device_id ids[] =
{
   {0x1172,0x0791,0x3931,0x4c37,0,0,0},
   {0,0,0,0,0,0,0}
};
MODULE_DEVICE_TABLE(pci, ids);


static struct file_operations fops =
{
   .owner   = THIS_MODULE,
   .unlocked_ioctl   = ldevpcib_ioctl,
   .mmap    = ldevpcib_mmap,
   .open    = ldevpcib_open,
   .release = ldevpcib_close,
};

static struct pci_driver l791_driver =
{
   .name     = device_name,
   .id_table = ids,
   .probe    = ldriver_probe,
   .remove   = ldriver_remove,
};

static int __init ldriver_init(void)
{
   return pci_register_driver(&l791_driver);
}

static void __exit ldriver_exit(void)
{
   pci_unregister_driver(&l791_driver);
}

module_init(ldriver_init);
module_exit(ldriver_exit);
