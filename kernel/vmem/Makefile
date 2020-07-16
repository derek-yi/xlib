
obj-m += vmem.o

KERN_DIR = /lib/modules/$(shell uname -r)/build
all:
	make -C $(KERN_DIR) M=$(CURDIR) modules

clean:
	make -C $(KERN_DIR) M=$(CURDIR) clean

