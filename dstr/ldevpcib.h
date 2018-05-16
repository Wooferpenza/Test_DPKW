#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#include <linux/autoconf.h>
#else
#include <generated/autoconf.h>
#endif

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <asm/delay.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/signal.h>


#define LCOMP_LINUX 1

#undef DbgPrint             /* undef it, just in case */
#ifdef LCARD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define DbgPrint(fmt, args...) printk( KERN_DEBUG "lcard: " fmt, ## args)
#  else
     /* This one for user space */
#    define DbgPrint(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define DbgPrint(fmt, args...) /* not debugging: nothing */
#endif

#undef DDbgPrint
#define DDbgPrint(fmt, args...) /* nothing: it's a placeholder */



typedef void *LPVOID;
typedef void *PVOID;
typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned int ULONG, *PULONG;
typedef int LONG, *PLONG;
typedef short SHORT, *PSHORT;
typedef unsigned short USHORT, *PUSHORT;
typedef int BOOL;

#define TRUE 1
#define FALSE 0

#define LABVIEW_FW
#include "include/ioctl.h"
#include "include/791cmd.h"

MODULE_DESCRIPTION("Driver for L-Card PCI BusMaster devices (L791)");
MODULE_AUTHOR("L-Card (Pavel Chauzov)");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0b");

// exported
int ldev_add(void);
int ldev_register(struct file_operations *fops, int slot);
int ldev_remove(int slot);


#define FLAG_USED 0
#define FLAG_WORKING 1
#define FLAG_ADC_EVT 2 // adc_bm
#define FLAG_DAC_EVT 3 // dac_usr
#define FLAG_ADC_OVF_EVT 4 // adc_ovf
#define FLAG_ADC_BUF_EVT 5 // adc_buf
#define FLAG_DAC_UNF_EVT 6 // dac_unf
#define FLAG_PWR_EVT 7 // power


typedef struct __hb
{
   u8 **addr;
   int nr_pages;
} hb;



typedef struct __ldevpcib
{
   struct pci_dev *pci_dev;

   int Slot;
   unsigned long dev_flags; // FLAG_USED FLAG_WORKING FLAG_ADC_EVT 0 1 2

   u32 m_MemRange;
   u32 m_MemRangeLen;


   // ФХФ ЬФП БДТЕУБ....
   void *L791_REG_ADDR;

   //u16 *Sync;
   hb *Sync;
   //u16 *Share;
   u32 RealPages;
   u32 wBufferSize; // УМПЧ
   u32 wFIFO,wIrqStep;
   u32 CurPage,wPages/*,HalfPages*/;  // counters for pages


   //u16 *DacBuf;
   hb *DacBuf;
   //u16 *DacShare;
   u32 DacRealPages;
   u32 wDacBufferSize; // УМПЧ
   u32 wDacFIFO,wDacStep;
   u32 DacCurPage,wDacPages/*,HalfPages*/;  // counters for pages
   u32 wDacIrqEna;
   u32 wDacEna;
   u32 wDacNumber;


   WDAQ_PAR  dacPar;
   WDAQ_PAR  adcPar;

   int       Type;

   SLOT_PAR sl;

   PLATA_DESCR_L791 pd;

   spinlock_t irq_lock;
   unsigned long flags;

   struct mutex io_lock;

   wait_queue_head_t adc_bm_wq;
   wait_queue_head_t adc_ovf_wq;
   wait_queue_head_t adc_buf_wq;
   wait_queue_head_t dac_usr_wq;
   wait_queue_head_t dac_unf_wq;
   wait_queue_head_t power_wq;

   struct kref    kref;

   struct scatterlist *sglist;
   //u32 Descr[256];
   u32 m_MaxXferRegs;
   u32 m_RealXferRegs;

} ldevpcib;
#define to_ldevpcib_dev(d) container_of(d, ldevpcib, kref)


// interrupt
irqreturn_t ldevpcib_isr_791(int irq, void *dev_id);//, struct pt_regs *regs);
void LDevPciB_Enable791(ldevpcib *dev, int b);
int ldevpcib_syncint(ldevpcib *dev, unsigned int cmd, unsigned long arg);

u32 WaitForFlashCmdReady_L791(ldevpcib *dev);
u32 ReadFlashWord_L791(ldevpcib *dev, u32 addr, u16 *data);
u32 ReadPlataDescr_L791(ldevpcib *dev, PPLATA_DESCR_L791 pd);
u32 WriteFlashWord_L791(ldevpcib *dev, u32 addr, u16 data);
u32 WriteFlashPage_L791(ldevpcib *dev, u16 page);

void SetBoardFIFO(ldevpcib *dev);
