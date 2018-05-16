
#DEBUG=y

# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.


ifeq ($(DEBUG),y)
 DEBFLAGS = -O -g -fno-builtin-memcpy -DLCARD_DEBUG
else
 DEBFLAGS = -O2
endif

EXTRA_CFLAGS+= $(DEBFLAGS)  

ifeq ($(KERNELRELEASE),)

    # Assume the source tree is where the running kernel was built
    # You should set KERNELDIR in the environment if it's elsewhere
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    # The current directory is passed to sub-makes as argument
    PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
cln:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions

.PHONY: modules modules_install clean

else
	ldevusb-objs = e2010.o e140.o e440.o e154.o ldevusbu.o
	ldevpci-objs = l760.o ldevpciu.o
	ldevpcibm-objs = l791.o ldevpcib.o
	# called from kernel build system: just declare what our modules are
	obj-m += ldevice.o
	obj-m += ldevpci.o
	obj-m += ldevusb.o
	obj-m += ldevpcibm.o
endif

