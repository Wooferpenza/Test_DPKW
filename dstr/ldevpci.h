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
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/signal.h>


#define LCOMP_LINUX 1 // ЗПЧПТЙН ЙОЛМАДБН ЮПФ НЩ Ч мЙОХЛУЕ

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

// defined for ioctl.h....//////////////////
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
/////////////////////////////////////////////

#define LABVIEW_FW // ПРТЕДЕМСЕН ЮФПВЩ ЧЛМАЮЙФШ у ЙОФЕТЖЕКУ Й ЪБДЕЖБКОЙФШ у++ ЛПД Ч ЙОЛМАДБИ
#include "include/ioctl.h"
#include "include/pcicmd.h"

MODULE_DESCRIPTION("Driver for L-Card PCI L-7XX devices");
MODULE_AUTHOR("L-Card (Pavel Chauzov)");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0b");

// exported
int ldev_add(void);
int ldev_register(struct file_operations *fops, int slot);
int ldev_remove(int slot);

#define FLAG_USED 0
#define FLAG_WORKING 1
#define FLAG_ADC_EVT 2
#define FLAG_DAC_EVT 3

// huge buffer for data....
typedef struct __hb
{
   u8 **addr;
   int nr_pages;
} hb;

typedef struct __ldevpci
{
   int Slot;

    struct pci_dev *pci_dev;

   unsigned long dev_flags; // FLAG_USED FLAG_WORKING FLAG_ADC_EVT 0 1 2

   unsigned long m_IoRange;
   unsigned long m_IoRangeLen;
   unsigned long m_MemRange;
   unsigned long m_MemRangeLen;

   // ФХФ ЬФП БДТЕУБ....
   void *DATA_REG_PLX;         // Data port
   void *IDMA_REG_PLX;         // IDMA port
   void *CMD_REG_PLX;         // irq2 DSP
   void *IRQ_RST_PLX;         // irq reset
   void *IRQ1_RST_PLX;         // irq reset
   void *DTA_ARRAY_PLX;      // forever

   unsigned long   FLASH_REG_PLX;
   unsigned long   FLASH_CS_PLX;

   void *m_Map;

   hb *Sync;
   u32 RealPages;
   u32 wBufferSize; // УМПЧ
   u32 wFIFO,wIrqStep;
   u32 CurPage,wPages;  // counters for pages


   hb *DacBuf;
   u32 DacRealPages;
   u32 wDacBufferSize; // УМПЧ
   u32 wDacFIFO,wDacStep; //
   u32 DacCurPage,wDacPages;  // counters for pages
   u32 wDacIrqEna;
   u32 wDacEna;
   u32 wDacNumber;


   WDAQ_PAR  dacPar;
   WDAQ_PAR  adcPar;
   int       Type;
   u16       DSPBase;  // Base address for variables
   SLOT_PAR  sl;

   PLATA_DESCR pd;

   spinlock_t irq_lock;
   unsigned long flags;

   struct mutex io_lock;

   wait_queue_head_t adc_wq;
   wait_queue_head_t dac_wq;

   struct kref    kref;
} ldevpci;
#define to_ldevpci_dev(d) container_of(d, ldevpci, kref)


// interrupt
irqreturn_t ldevpci_ISR(int irq, void *dev_id);//, struct pt_regs *regs);
void ldevpci_enable(ldevpci *dev, int b);
int ldevpci_syncISR(ldevpci *dev, unsigned int cmd, unsigned long arg);

// flash routines
void SetFlash_Plx(ldevpci *dev, u16 FlashData);
void StartFlash_Plx(ldevpci *dev);
u16 GetFlashDataBit_Plx(ldevpci *dev);
void FlipCLK_Plx(ldevpci *dev);
void StopFlash_Plx(ldevpci *dev);

void ReadFlashWord_PLX(ldevpci *dev, u16 FlashAddress, u16 *Data);
void WriteFlashWord_PLX(ldevpci *dev, u16 FlashAddress, u16 Data);
u32 EnableFlashWrite_PLX(ldevpci *dev, u16 Flag);
u32 ReadPlataDescr_PLX(ldevpci *dev, PPLATA_DESCR pd);
u32 WritePlataDescr_PLX(ldevpci *dev, PPLATA_DESCR pd, u16 enable);

// DM/PM routines
void COMMAND_760(ldevpci *dev, u16 Cmd);
u16 GET_DM_WORD_PCI(ldevpci *dev, u16 Addr);
void PUT_DM_WORD_PCI(ldevpci *dev, u16 Addr, u16 Data);
void PUT_PM_WORD_PCI(ldevpci *dev, u16 Addr, u32 Data);
u32 GET_PM_WORD_PCI(ldevpci *dev, u16 Addr);
void GET_DATA_MEMORY_PCI(ldevpci *dev, u16 *Data, u32 Count, u16 Addr);
void PUT_DATA_MEMORY_PCI(ldevpci *dev, u16 *Data, u32 Count, u16 Addr); // how words
void GET_PM_MEMORY_PCI(ldevpci *dev, u32 *Data, u32 Count, u16 Addr);
void PUT_PM_MEMORY_PCI(ldevpci *dev, u32 *Data, u32 Count, u16 Addr);

// huge buffer routine...
void HB_GET_DATA_MEMORY_PCI(ldevpci *dev, hb *buf, int pos, u32 Count, u16 Addr);
void HB_PUT_PM_MEMORY_PCI(ldevpci *dev, hb *buf, int pos, u32 Count, u16 Addr);

void SetBoardFIFO(ldevpci *dev);
void SetDACparameters_PLX(ldevpci *dev);
