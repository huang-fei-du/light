#------------------------------------------------
#描述:Sqlite3.6.19动态链接库Makefile文件
#作者:雷湧
#修改历史:
#   1.2010-01-07,leiyong,created.
#------------------------------------------------

#指定编译器前缀
cc:=arm-linux-

#终极目标
all:libsqlite3.so

#源文件列表
src=sqlite3.c shell.c

#目标文件
tgt=$(src:.c=.o)

$(src):sqlite3.h sqlite3ext.h
	@touch $@

%.o:%.c
	$(cc)gcc -c $?
	
libsqlite3.so:$(tgt)
	$(cc)gcc -shared -o $@ $(tgt) -lpthread
	$(cc)strip $@

#make clean action
clean:
	rm -f *.o *.so *.bak