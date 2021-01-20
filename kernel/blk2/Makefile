ifneq ($(KERNELRELEASE),)

obj-m:=ramdisk_quest.o

else

KERNELDIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.mod.c *.mod.o *.ko

endif
