all:initInfo

#т╢нд╪Ч
dysrc=infoinit.c

dytgt=$(dysrc:.c=.o)

INCLUDES = -I../dlzd/include/dataBase
LFLAG    += -L../dlzd/lib -lsqlite3 -ldl

%.o:%.c
	arm-linux-gcc $(INCLUDES) -c $?
	
initInfo:$(dytgt)
	arm-linux-gcc $(INCLUDES) -o $@ $(dytgt) $(LFLAG)

clean:
	rm *.o