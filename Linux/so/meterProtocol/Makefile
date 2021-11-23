#------------------------------------------------
#描述:电表规约动态链接库Makefile文件
#作者:雷湧
#修改历史:
#   1.2010-01-08,leiyong,created.
#------------------------------------------------

#指定编译器前缀
cc:=arm-linux-

#终极目标
all:libMeterProtocol.so

INCLUDES = -I../../dlzd/include/ -I../../dlzd/include/meter  -I../../dlzd/include/dataBase

#源文件列表
src=meterProtocol.c timeUser.c convert.c

#目标文件
tgt=$(src:.c=.o)

$(src):meterProtocol.h convert.h timeUser.h
	@touch $@

%.o:%.c
	$(cc)gcc $(INCLUDES) -c $?
	
libMeterProtocol.so:$(tgt)
	$(cc)gcc -shared -o $@ $(tgt)
	$(cc)strip $@

#make clean action
clean:
	rm -f *.o *.so *.bak