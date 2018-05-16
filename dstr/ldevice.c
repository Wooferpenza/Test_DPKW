#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/major.h>


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

static int howmany=4;
module_param(howmany, int, S_IRUGO);

MODULE_DESCRIPTION("Dispatch driver for L-Card devices");
MODULE_AUTHOR("L-Card (Pavel Chauzov)");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0b");


struct class *lcard_class;
EXPORT_SYMBOL(lcard_class);

#define MAX_SLOTS 128
//const 
int major=0;
const char *LDevName = "ldevice";
const char *LCardClassName = "lcard";

//static spinlock_t ldev_lock = SPIN_LOCK_UNLOCKED;
DEFINE_SPINLOCK(ldev_lock);

static struct file_operations *fops_tab[MAX_SLOTS]; // fops for each device
static int ldev_tab[MAX_SLOTS];


static int lalloc_number(void)
{
   int i;
   for(i=0;i<MAX_SLOTS;i++)
   {
      if(ldev_tab[i]==0) { ldev_tab[i] = 1; return i; }
   }
   return -1;
}

static int lhold_number(struct file_operations *fops, int slot)
{
   if(slot>=MAX_SLOTS) return -ENOENT;
   if(slot < 0) return -ENOENT;
   fops_tab[slot] = fops;
   return slot;
}


static int lfree_number(int slot)
{
   if(slot>=MAX_SLOTS) return -ENOENT;
   ldev_tab[slot] = 0;
   if(!fops_tab[slot]) return -ENODEV;
   fops_tab[slot] = NULL;
   return 0;
}


static int ldev_add(void)
{
   int s;
   spin_lock(&ldev_lock);
   s = lalloc_number();
   DbgPrint( "Alloc \n");   
   spin_unlock(&ldev_lock);
   return s;
}
EXPORT_SYMBOL(ldev_add);

static int ldev_register(struct file_operations *fops, int slot)
{
   int s;
   spin_lock(&ldev_lock);
   s = lhold_number(fops,slot);
   spin_unlock(&ldev_lock);
   return s;
}
EXPORT_SYMBOL(ldev_register);

static int ldev_remove(int slot)
{
   int s;
   spin_lock(&ldev_lock);
   s = lfree_number(slot);
   spin_unlock(&ldev_lock);
   return s;
}
EXPORT_SYMBOL(ldev_remove);


static int ldev_open(struct inode *inode, struct file *file)
{
int status = -ENODEV; 
struct file_operations *fops_old;

   DbgPrint( "LDev_Open call \n");

   if(MINOR(inode->i_rdev) >= MAX_SLOTS) { DbgPrint("minor %d\n", MINOR(inode->i_rdev));return status; }
   fops_old = (struct file_operations *)file->f_op;
   file->f_op = fops_get(fops_tab[MINOR(inode->i_rdev)]);
   if(file->f_op == NULL) {DbgPrint( "zero fops \n"); return status;}
   status = file->f_op->open(inode, file);
   if(status!=0) // call return error
   {
      DbgPrint( "open error \n");
      fops_put(file->f_op);
      file->f_op = fops_get(fops_old);
   }
   fops_put(fops_old);
   return status;
}

//////////////////////////////////////////
// MAIN PART
//////////////////////////////////////////

static struct file_operations ldev_fops =
{
   .open = ldev_open,
};


static int __exit ldev_init(void)
{
int i;
	DbgPrint( "Staring LDev driver...\n");
   major =register_chrdev(0, LDevName, &ldev_fops);
   if(major<0)
   {
	   DbgPrint("Unable register major!!! \n");
	   return -EBUSY;
   }
   
   lcard_class = class_create(THIS_MODULE, (char *)LCardClassName);
   for(i=0; i<howmany; i++)
   {
//      class_device_create(lcard_class, NULL, MKDEV(major, i), NULL ,"ldevice%d", i);
      device_create(lcard_class, NULL, MKDEV(major, i), NULL,"ldevice%d", i);
   }
	return 0;
}

static void __exit ldev_exit(void)
{
int i;
   for(i=0; i<howmany; i++)
   {
//      class_device_destroy(lcard_class, MKDEV(major, i));
      device_destroy(lcard_class, MKDEV(major, i));
   }
   class_destroy(lcard_class);
	unregister_chrdev(major, LDevName);
}

module_init(ldev_init);
module_exit(ldev_exit);
