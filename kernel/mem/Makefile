export CROSS_COMPILE=
export ARCH=x86

ifneq ($(KERNELRELEASE),)

obj-m:=misc.o

else

KERNELDIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.mod.c *.mod.o *.ko

endif
