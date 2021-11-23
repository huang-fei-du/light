all:processWatch

#т╢нд╪Ч
dysrc=processWatch.c

dytgt=$(dysrc:.c=.o)

INCLUDES = 
LFLAG    += 

%.o:%.c
	arm-linux-gcc $(INCLUDES) -c $?
	
processWatch:$(dytgt)
	arm-linux-gcc $(INCLUDES) -o $@ $(dytgt) $(LFLAG)

clean:
	rm *.o