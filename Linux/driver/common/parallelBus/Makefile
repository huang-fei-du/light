obj-m := parallelBus.o

KERNELDIR := /embed/linux-2.6.20
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.c .*.cmd .tmp_version
