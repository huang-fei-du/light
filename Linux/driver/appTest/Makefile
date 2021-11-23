# DATE:2008-10-21 
# AUTHOR:CHEN XING
# PROJECT: Internet Radio
# ORI: Hong Kong Baite


CROSS_COMPILE = arm-linux-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

appTest:
	$(CC) app_test.c -o $@  -lpthread
clean:
	rm -rf appTest *.o 
in:
	cp fpga /home/kernel/ramdisk
	cp fpga /home/radiofs/home/chenxing
