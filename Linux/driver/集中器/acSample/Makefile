# DATE:2008-10-21 
# AUTHOR:CHEN XING
# PROJECT: Internet Radio
# ORI: Hong Kong Baite


CROSS-COMPILE = arm-linux-
CC=$(CROSS-COMPILE)gcc
obj-m   := ac_sample.o
KDIR    := /embed/linux-2.6.20_baite
PWD     := $(shell pwd) 
default: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers test  modules.order
in:
	scp spi_fpga.ko /home/kernel/ramdisk
	scp spi_fpga.ko /home/radiofs/home/chenxing
