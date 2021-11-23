#------------------------------------------------
#描述:GDW376-2动态链接库Makefile文件
#作者:雷湧
#修改历史:
#   1.2010-03-09,leiyong,created.
#------------------------------------------------

#指定编译器前缀
cc:=arm-linux-

#终极目标
all:libGdw376-2.so

INCLUDES = -I../../dlzd/include/ -I../../dlzd/include/meter  -I../../dlzd/include/dataBase

#源文件列表
src=gdw376-2.c

#目标文件
tgt=$(src:.c=.o)

$(src):gdw376-2.h
	@touch $@

%.o:%.c
	$(cc)gcc $(INCLUDES) -c $?
	
libGdw376-2.so:$(tgt)
	$(cc)gcc -shared -o $@ $(tgt)
	$(cc)strip $@

#make clean action
clean:
	rm -f *.o *.so *.bak