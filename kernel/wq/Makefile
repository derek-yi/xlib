#export CROSS_COMPLE=aarch64-linux-gnu-
#export ARCH=arm
#CC  = $(CROSS_COMPILE)gcc

KERNELDIR:=/home/derek/share/repo_rk/rk3588/kernel_g76
PWD := $(shell pwd)

## todo
ccflags-y += -Wno-error

obj-m := wq1.o

modules:
	#$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
    
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

.PHONY: modules clean