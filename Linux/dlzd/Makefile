#------------------------------------------------
#描述:电力终端(负控终端及集中器)Makefile文件
#作者:雷湧
#修改历史:
#   1.2009-10-27,leiyong,created.
#------------------------------------------------

#指定编译器前缀
#CC =
CC = arm-linux-

#源文件搜索路径
VPATH = ./src ./src/LCD ./src/dataBase ./src/mainstation ./src/meter ./src/rt  ../linux-2.6.20_baite/kernel

#头文件搜索路径
INCLUDES = -I../so/meterProtocol -I../so/wirelessModem -I../so/GDW376-2-Len2 -I../driver/ioChannel\
           -I./include -I./include/dataBase -I./include/mainstation -I./include/meter  -I./include/LCD

#CFLAG = -static
#CFLAG += --pack-struct -O2 -w -I./include
CFLAG += -O2 -w $(INCLUDES)

#库文件路径及库名
LFLAG += -L./lib -lsqlite3 -lMeterProtocol -lWirelessModem -lGdw376-2 -ldl

#终极目标
all : dlzd

#源文件列表
SRC = dlzd.c hardwareConfig.c timeUser.c convert.c lcdGui.c teRunPara.c msSetPara.c dataBase.c dataBalance.c\
      wlModem.c msInput.c msOutput.c reportTask.c\
      AFN00.c AFN01.c AFN02.c AFN04.c AFN05.c AFN06.c AFN09.c AFN0A.c AFN0B.c AFN0C.c AFN0D.c AFN0E.c AFN0F.c AFN10.c md5.c\
      copyMeter.c userInterface.c att7022b.c statistics.c loadCtrl.c\
      esRtUpdate.c 

#目标文件
TGT = $(SRC:.c=.o)
%.o : %.c
	$(CC)gcc $(CFLAG) -c $? 
dlzd : $(TGT)
	$(CC)gcc $(CFLAG) -o $@ $(TGT) $(LFLAG) -lpthread
	$(CC)strip $@

#make clean action
clean:
	rm -f dlzd *.o *.bak

set:
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:./lib
