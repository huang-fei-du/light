all:esRtUpdate

#т╢нд╪Ч
dysrc=esRtUpdate.c

dytgt=$(dysrc:.c=.o)

INCLUDES = -I../dlzd/include/dataBase
LFLAG    += -L../dlzd/lib -lsqlite3 -ldl

%.o:%.c
	arm-linux-gcc $(INCLUDES) -c $?
	
esRtUpdate:$(dytgt)
	arm-linux-gcc $(INCLUDES) -o $@ $(dytgt) $(LFLAG)

clean:
	rm *.o