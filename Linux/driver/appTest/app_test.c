#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pthread.h>

#include <string.h>
#include <termios.h>

#define RESET_BUF	0x01
#define BUF_SIZE	16

static char *device = "/dev/ttyS5";
static char buf[BUF_SIZE];


int fdOfIoChannel; //I/Oͨ���ļ�������
int fd;
unsigned char ackOrNack  = 0;
unsigned char sendStatus = 0;

unsigned char framing(unsigned char afn,unsigned char dt);

void display(char *tmp, int size, unsigned short addr)
{
  int i, j;
  printf("  ADDRESS  x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF \n\r");
  for(i=0; i<size; ){
          printf("0x0000%04X ", addr+i);
          for(j=0; j<16 && i<size; j++)
                  printf("%02x ", *(tmp + i++));
          printf("\n\r");
  }
}

/**************************************************
��������:threadOfTtys5Received
��������:ttys5���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void *threadOfTtys5Receive(void *arg)
{
  unsigned char fxxcReceive[50];
  unsigned char recvLen;
  unsigned char checkSum;
  int           i;
  unsigned char afn;
  
  while (1)
  {
	  recvLen=read(fd,&fxxcReceive,50);
	  printf("�����ֽ�:%d\n",recvLen);
	  
	  for(i=0;i<recvLen;i++)
	  {
	  	printf("%02x ",fxxcReceive[i]);
	  }
	  printf("\n");
	  
	  if (fxxcReceive[0]!=0x68)
	  {
	  	 printf("����֡ͷ68H����!\n");
	  	 continue;
	  }
	  if (fxxcReceive[recvLen-1]!=0x16)
	  {
	  	 printf("����֡β16H����!\n");
	  	 continue;
	  }
    
    checkSum = 0x0;
    for(i=2;i<recvLen-2;i++)
    {
      checkSum += fxxcReceive[i];
    }    
    if (checkSum!=fxxcReceive[recvLen-2])
    {
	  	 printf("����֡У�����!\n");
	  	 continue;    	 
    }
    
    if ((fxxcReceive[2]&0x40)==0x00)
    {
    	 afn = fxxcReceive[9];
    	 switch(afn)
    	 {
    	 	  case 0x0:
    	 	  	if (fxxcReceive[10]==0x01 && fxxcReceive[11]==0x00)
    	 	  	{
    	 	  		 printf("ȷ��֡!\n");
    	 	  		 if (sendStatus==1)
    	 	  		 {
    	 	  		   sendStatus = 2;
    	 	  		 }
    	 	  	}
    	 	  	else
    	 	  	{
    	 	  		 printf("����֡!\n");
    	 	  	}
    	 	  	break;    	 	  	
    	 }
    }
	}
}

int main(int argc, char *argv[])
{
	int      ret;
	unsigned short addr;
	char     data;
	int      i,j;
	int      tmpi;
	int      frameTail;
	char     buff[50];	
  struct   termios opt;
  unsigned char fxxc[50];
  char     sayStr[50];
  unsigned char threadPara;
  pthread_t id;
  	
	fd=open(device,O_RDWR);
	if (fd == -1)
	{
		printf("Unable to open ioChannel \n\r");
		exit(0);
	}

	tcgetattr(fd,&opt);
	cfmakeraw(&opt);
	//opt.c_cflag |= (PARODD | PARENB);   //����Ϊ��Ч��
	opt.c_cflag |= (PARENB);            //����ΪżЧ��
  opt.c_iflag |= INPCK;
	cfsetispeed(&opt,B9600);            //����������Ϊ9600bps
	cfsetospeed(&opt,B9600);
	tcsetattr(fd,TCSANOW,&opt);
  
  //��IOͨ��
  fdOfIoChannel = open("/dev/ioChannel",O_RDWR|O_NOCTTY);  //��д��ʽ��I/Oͨ��
	if (fdOfIoChannel<=0)
	{
		 printf("open /dev/ioChannel error !\n");
   	 return -1;
  }
  else
  {
		 printf("open /dev/ioChannel success!\n");
  }
  
  //��λ�ز�ģ��
	printf("��λ�ز�ģ��\n");
  ioctl(fdOfIoChannel,8,1);
  ioctl(fdOfIoChannel,9,1);
  sleep(1);
  ioctl(fdOfIoChannel,9,0);
  sleep(1);

  //����485�ӿ�1�����߳�
  threadPara = 0x01;
  ret=pthread_create(&id,NULL,threadOfTtys5Receive,&threadPara);
  if(ret!=0)
  {
    printf ("Create copy meter receive pthread error!\n");
    return 1;
  }
  
	framing(0x11,0x1);   //����ز��ӽڵ�
	//framing(0x13,0x1);   //����ز��ӽڵ�
	//framing(0x01,0x02); //��������ʼ��
	sendStatus = 1;
	tmpi=1;
	while(1)
	{
		 if (sendStatus==2)
		 {
		 	 sleep(5);
	     
	     framing(0x11,0x4);
	     
		 	 //framing(0x03,01);
		 	 //framing(0x11,1);
		 	 sendStatus = 3;
		 }
	}
}

unsigned char framing(unsigned char afn,unsigned char dt)
{
   unsigned char fxxc[50];
   unsigned char checkSum, i;
   unsigned char frameTail;
   
   fxxc[0]  = 0x68;
   fxxc[2]  = 0x42;

   fxxc[4]  = 0x00;
   fxxc[5]  = 0x00;
   fxxc[6]  = 0x00;
   fxxc[7]  = 0x00;
   fxxc[8]  = 0x00;

   frameTail = 9;

   if (afn==0x13)
   {
     fxxc[3]  = 0x04;
     
     fxxc[frameTail++] = 0xBB;
     fxxc[frameTail++] = 0xBB;
     fxxc[frameTail++] = 0xBB;
     fxxc[frameTail++] = 0xBB;
     fxxc[frameTail++] = 0xBB;
     fxxc[frameTail++] = 0xBB;
     
     //fxxc[frameTail++] = 0x52;
     //fxxc[frameTail++] = 0x00;
     //fxxc[frameTail++] = 0x67;
     //fxxc[frameTail++] = 0x02;
     //fxxc[frameTail++] = 0x60;
     //fxxc[frameTail++] = 0x00;

     fxxc[frameTail++] = 0x01;    //��ǰ�ɼ�����ַΪ8
     fxxc[frameTail++] = 0x00;
     fxxc[frameTail++] = 0x00;
     fxxc[frameTail++] = 0x00;
     fxxc[frameTail++] = 0x00;
     fxxc[frameTail++] = 0x00;
   }
   else
   {
     fxxc[3]  = 0x00;
   }
   
   fxxc[frameTail++]  = afn;
   fxxc[frameTail++] = 1<<(dt-1);
   fxxc[frameTail++] = 0x0;
   
   switch(afn)
   {
   	 case 0x10:
   	 	 switch(dt)
   	 	 {
   	 	 	  case 3:   //ָ���ز��ӽڵ����һ���ж�·����Ϣ
   	 	 	  	fxxc[frameTail++] = 0x52;
    	  		fxxc[frameTail++] = 0x00;
   	  		  fxxc[frameTail++] = 0x67;
   	  		  fxxc[frameTail++] = 0x02;
   	  		  fxxc[frameTail++] = 0x60;
   	  		  fxxc[frameTail++] = 0x00;
   	 	 	    break;
   	 	 }
   	 	 break;
   	 	 
   	 case 0x11:
   	   switch (dt)
   	   {
   	   	 case 1:  //����ز��ӽڵ�
   	  		 fxxc[frameTail++] = 0x1;   //�ز��ӽڵ�����

   	  		 //fxxc[frameTail++] = 0x52;  //�ӽڵ�1��ַ
   	  		 //fxxc[frameTail++] = 0x00;
   	  		 //fxxc[frameTail++] = 0x67;
   	  		 //fxxc[frameTail++] = 0x02;
   	  		 //fxxc[frameTail++] = 0x60;
   	  		 //fxxc[frameTail++] = 0x00;

   	  		 //fxxc[frameTail++] = 0x08;  //�ӽڵ�1��ַ(��ǰ�ɼ�����ַΪ8)
   	  		 //fxxc[frameTail++] = 0x00;
   	  		 //fxxc[frameTail++] = 0x00;
   	  		 //fxxc[frameTail++] = 0x00;
   	  		 //fxxc[frameTail++] = 0x00;
   	  		 //fxxc[frameTail++] = 0x00;

   	  		 //fxxc[frameTail++] = 0x33;  //�ӽڵ�1��ַ(05�ɼ�����ַΪ333333333333)
   	  		 //fxxc[frameTail++] = 0x33;
   	  		 //fxxc[frameTail++] = 0x33;
   	  		 //fxxc[frameTail++] = 0x33;
   	  		 //fxxc[frameTail++] = 0x33;
   	  		 //fxxc[frameTail++] = 0x33;


   	  		 fxxc[frameTail++] = 0x01;  //�ӽڵ����
   	  		 fxxc[frameTail++] = 0x00;

   	  		 fxxc[frameTail++] = 0x01;  //�ӽڵ��Լ
   	  		 break;
   	  		 
   	  	 case 5:  //�����ز����ڵ�����ע��
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 fxxc[frameTail++] = 0x00;
   	  	 	 break;
   	   }
   	   break;
   	   
   	 case 0x13:
   	 	 switch(dt)
   	 	 {
   	 	 	 case 0x1:   //����ز��ӽڵ�(�㳭)
           //��Լ����
           fxxc[frameTail++] = 0x01;
           
           //�ӽڵ�����
           fxxc[frameTail++] = 0x00;
           
           //�����ĳ���
           fxxc[frameTail++] = 0x0e;
           
           //����֡
           //fxxc[frameTail++] = 0x68;
           //fxxc[frameTail++] = 0x52;
           //fxxc[frameTail++] = 0x00;
           //fxxc[frameTail++] = 0x67;
           //fxxc[frameTail++] = 0x02;
           //fxxc[frameTail++] = 0x60;
           //fxxc[frameTail++] = 0x00;
           //fxxc[frameTail++] = 0x68;
           //fxxc[frameTail++] = 0x01;
           //fxxc[frameTail++] = 0x02;
           //fxxc[frameTail++] = 0x43;
           //fxxc[frameTail++] = 0xC3;
           //fxxc[frameTail++] = 0xf4;
           //fxxc[frameTail++] = 0x16;

           fxxc[frameTail++] = 0x68;   //ͨ���ɼ�������3022
           fxxc[frameTail++] = 0x22;
           fxxc[frameTail++] = 0x30;
           fxxc[frameTail++] = 0x00;
           fxxc[frameTail++] = 0x00;
           fxxc[frameTail++] = 0x00;
           fxxc[frameTail++] = 0x00;
           fxxc[frameTail++] = 0x68;
           fxxc[frameTail++] = 0x01;
           fxxc[frameTail++] = 0x02;
           fxxc[frameTail++] = 0x43;
           fxxc[frameTail++] = 0xC3;
           fxxc[frameTail++] = 0x2b;
           fxxc[frameTail++] = 0x16;

           fxxc[frameTail++] = 0x68;   //ͨ���ɼ�������3022
           fxxc[frameTail++] = 0x06;
           fxxc[frameTail++] = 0x05;
           fxxc[frameTail++] = 0x04;
           fxxc[frameTail++] = 0x03;
           fxxc[frameTail++] = 0x02;
           fxxc[frameTail++] = 0x01;
           fxxc[frameTail++] = 0x68;
           fxxc[frameTail++] = 0x01;
           fxxc[frameTail++] = 0x02;
           fxxc[frameTail++] = 0x43;
           fxxc[frameTail++] = 0xC3;
           fxxc[frameTail++] = 0xee;
           fxxc[frameTail++] = 0x16;
   	 	 	   
   	 	 	   break;
   	 	 }
   	 	 break;
   }
   
   checkSum = 0;
   for(i=2;i<frameTail;i++)
   {
   	  checkSum += fxxc[i];
   }
   fxxc[frameTail++] = checkSum;
   fxxc[frameTail++] = 0x16;
   fxxc[1]  = frameTail;
   
   write(fd,&fxxc,frameTail);

	 printf("AFN=%02x,DT=%d,Tx:",afn,dt);	  
	 for(i=0;i<frameTail;i++)
	 {
	  	printf("%02x ",fxxc[i]);
	 }
	 printf("\n");   
}
