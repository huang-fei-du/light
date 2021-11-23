# DATE:2010-02-19 
# AUTHOR:lhh
# PROJECT: power terminal


CROSS-COMPILE = arm-linux-
CC=$(CROSS-COMPILE)gcc
obj-m   := 2ndSpi1.o
KDIR    := /embed/linux-2.6.20_baite
PWD     := $(shell pwd)
default: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers test  modules.order

