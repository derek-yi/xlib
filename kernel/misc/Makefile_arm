export CROSS_COMPLE=arm-linux-gnueabi-
export ARCH=arm

KERNELDIR = /home/derek/share/linux-3.16.45
PWD := $(shell pwd)
CC  = $(CROSS_COMPILE)gcc

obj-m := misc.o

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
    
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules clean