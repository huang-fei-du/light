#------------------------------------------------
#描述:无线Modem动态链接库Makefile文件
#作者:雷湧
#修改历史:
#   1.2010-01-24,leiyong,created.
#------------------------------------------------

#指定编译器前缀
cc:=arm-linux-

#终极目标
all:libWirelessModem.so

INCLUDES = -I../../dlzd/include/

#源文件列表
src=wirelessModem.c

#目标文件
tgt=$(src:.c=.o)

$(src):wirelessModem.h
	@touch $@

%.o:%.c
	$(cc)gcc $(INCLUDES) -c $?
	
libWirelessModem.so:$(tgt)
	$(cc)gcc -shared -o $@ $(tgt)
	$(cc)strip $@

#make clean action
clean:
	rm -f *.o *.so *.bak