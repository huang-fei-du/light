obj-m := rtc-isl12022m.o

KERNELDIR := /embed/linux-2.6.20_baite
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.c .*.cmd .tmp_version
