#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#include <linux/autoconf.h>
#else
#include <generated/autoconf.h>
#endif

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kref.h>
//#include <linux/smp_lock.h>
#include <linux/sched.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/signal.h>


#define LCOMP_LINUX 1

/*
 * Macros to help debugging
 */

#undef DbgPrint             /* undef it, just in case */
#ifdef LCARD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define DbgPrint(fmt, args...) printk( KERN_DEBUG "lcard: " fmt, ## args)
//#    define DbgPrint(fmt, args...) printk( KERN_DEBUG fmt, ## args)
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
#include "include/e2010cmd.h"
#include "include/e140cmd.h"
#include "include/e440cmd.h"
#include "include/e154cmd.h"
#include "include/ioctl.h"

MODULE_DESCRIPTION("Driver for L-Card USB devices");
MODULE_AUTHOR("L-Card (Pavel Chauzov)");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0b");

// exported
int ldev_add(void);
int ldev_register(struct file_operations *fops, int slot);
int ldev_remove(int slot);


/* Get a minor range for your devices from the usb maintainer */
//#define USB_LCARD_MINOR_BASE  0x97

#define FLAG_USED 0
#define FLAG_WORKING 1
#define FLAG_ADC_EVT 2

typedef struct __hb
{
   u8 **addr;
   int nr_pages;
} hb;


/* Structure to hold all of our device specific stuff */
typedef struct __ldevusb
{
   struct usb_device *udev;         /* the usb device for this device */
   struct usb_interface *interface;    /* the interface for this device */
   u8 bulk_in_endpointAddr; /* the address of the bulk in endpoint */
   u8 bulk_out_endpointAddr;   /* the address of the bulk out endpoint */


   int Slot;

   unsigned long dev_flags; // FLAG_USED FLAG_WORKING FLAG_ADC_EVT 0 1 2

// char *OutBulk;
// int Pages;

   struct urb *urb;

   SLOT_PAR sl;
   WDAQ_PAR adcPar;
   int Type;

   u32     CurPage,wPages/*,HalfPages*/;  // counters for pages
   u32     wBufferSize;
   u32     wFIFO,wIrqStep;
   u32     RealPages;

   hb      *Sync;
   u8      *buf;
   u8  *bios, *bios_pos;

   struct mutex io_lock;
   wait_queue_head_t adc_wq;

   struct kref    kref;
} ldevusb;
#define to_ldevusb_dev(d) container_of(d, ldevusb, kref)


void ldevusb_enable(ldevusb *dev, int flWork);

void COMMAND_E440(ldevusb *dev, u16 Cmd);
u16  GET_DM_WORD_E440(ldevusb *dev, u16 Addr);
void PUT_DM_WORD_E440(ldevusb *dev, u16 Addr, u16 Data);
u32  GET_PM_WORD_E440(ldevusb *dev, u16 Addr);
void PUT_PM_WORD_E440(ldevusb *dev, u16 Addr, u32 Data);

void GET_PM_MEMORY_E440(ldevusb *dev, u32 *Data, u32 Count, u16 Addr);
void GET_DATA_MEMORY_E440(ldevusb *dev, u16 *Data, u32 Count, u16 Addr);
void PUT_DATA_MEMORY_E440(ldevusb *dev, u16 *Data, u32 Count, u16 Addr); // how words
void PUT_PM_MEMORY_E440(ldevusb *dev, u32 *Data, u32 Count, u16 Addr);

u32  EnableFlashWrite_E440(ldevusb *dev, u16 Flag);
void ReadFlashWord_E440(ldevusb *dev, u16 FlashAddress, u16 *Data);
void WriteFlashWord_E440(ldevusb *dev, u16 FlashAddress, u16 Data);
void SetBoardFIFO(ldevusb *dev);
void EnableE440(ldevusb *dev, int b);

int QueueBulkRead(ldevusb *dev);


void EnableE140(ldevusb *dev, int b);
void WriteFlashWord_E140(ldevusb *dev, u16 FlashAddress, u16 Data);
void ReadFlashWord_E140(ldevusb *dev, u16 FlashAddress, u16 *Data);
u32 EnableFlashWrite_E140(ldevusb *dev, u16 Flag);
void PUT_PM_WORD_E140(ldevusb *dev, u16 Addr, u8 Data);
u8 GET_PM_WORD_E140(ldevusb *dev, u16 Addr);
void PUT_DM_WORD_E140(ldevusb *dev, u16 Addr, u16 Data);
u16 GET_DM_WORD_E140(ldevusb *dev, u16 Addr);
void PUT_DATA_MEMORY_E140(ldevusb *dev, u16 *Data, u32 Count, u16 Addr); // how words
void GET_DATA_MEMORY_E140(ldevusb *dev, u16 *Data, u32 Count, u16 Addr);
void COMMAND_E140(ldevusb *dev, u16 Cmd);

void EnableE154(ldevusb *dev, int b);
void ReadFlashWord_E154(ldevusb *dev, u16 *Data, u16 Size);
void WriteFlashWord_E154(ldevusb *dev, u16 *Data, u16 Size);
u32 EnableFlashWrite_E154(ldevusb *dev, u16 Flag);
void PUT_PM_WORD_E154(ldevusb *dev, u16 Addr, u8 Data);
u8 GET_PM_WORD_E154(ldevusb *dev, u16 Addr);
void PUT_DM_WORD_E154(ldevusb *dev, u16 Addr, u16 Data);
u16 GET_DM_WORD_E154(ldevusb *dev, u16 Addr);
void PUT_DATA_MEMORY_E154(ldevusb *dev, u16 *Data, u32 Count, u16 Addr); // how words
void GET_DATA_MEMORY_E154(ldevusb *dev, u16 *Data, u32 Count, u16 Addr);
void COMMAND_E154(ldevusb *dev, u16 Cmd);

void COMMAND_E2010(ldevusb *dev, u16 Cmd, u16 Par1, u16 Par2);
void GET_DATA_MEMORY_E2010(ldevusb *dev, u8 *Data, u32 Count, u32 Addr);
void PUT_DATA_MEMORY_E2010(ldevusb *dev, u8 *Data, u32 Count, u32 Addr);
u16 GET_DM_WORD_E2010(ldevusb *dev, u32 Addr);
void PUT_DM_WORD_E2010(ldevusb *dev, u32 Addr, u16 Data);
u8 GET_PM_WORD_E2010(ldevusb *dev, u32 Addr);
void PUT_PM_WORD_E2010(ldevusb *dev, u32 Addr, u8 Data);
void ReadFlashWord_E2010(ldevusb *dev, u16 *Data, u16 Size);
void WriteFlashWord_E2010(ldevusb *dev, u16 *Data, u16 Size);
u32 LoadBios_E2010(ldevusb *dev, u8 *Data, u32 size);
void EnableE2010(ldevusb *dev, int b);
