/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:hardwareConfig.c
����:leiyong
�汾:0.9
�������:2010��1��
����:�����ն�(�����ն�/������,AT91SAM9260������)Ӳ�������ļ�
�����б�:

�޸���ʷ:
  01,10-01-16,Leiyong created.

***************************************************/
#include <unistd.h>
#include <sys/reboot.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "ioChannel.h"
#include "userInterface.h"
#include "dataBase.h"

#include "hardwareConfig.h"
#include "wlModem.h"

int fdOfModem=NULL;//����1(����Modem)�ļ�������
int fdOfttyS2;     //����2(485�ӿ�1)�ļ�������
int fdOfttyS3;     //����3(485�ӿ�2)�ļ�������
int fdOfttyS4;     //����4(485�ӿ�3)�ļ�������
int fdOfCarrier;   //����5(�ز��ӿ�)�ļ�������
int fdOfLocal;     //����6(����ά���ӿ�)�ļ�������
int fdOfSample;    //�����ļ�������
int fdOfIoChannel; //I/Oͨ���ļ�������

extern INT32U avrHeartTime;                         //��AVR��Ƭ��ͨ�ŵ�����ʱ��

INT8U  cmd[5];

/**************************************************
��������:initHardware
��������:�����ն�(�����նˡ�������,AT91SAM9260������)��ʼ��Ӳ������
���ú���:
�����ú���:
�������:��
�������:void
����ֵ:void
***************************************************/
int initHardware(void)
{
  struct termios opt;
  INT32U i;
  
  //��IOͨ��
  fdOfIoChannel = open("/dev/ioChannel", O_RDWR|O_NOCTTY);  //��д��ʽ��I/Oͨ��
	if (fdOfIoChannel<=0)
	{
		printf("open /dev/ioChannel error !\n");
   	return -1;
  }
  else
  {
		printf("open /dev/ioChannel success!\n");
  }
  
  //��λESAMоƬ
  ioctl(fdOfIoChannel, ESAM_RST, 1);
  

 #ifdef LOAD_CTRL_MODULE
  #ifdef CPU_912_9206_QFP
   ioctl(fdOfIoChannel, MACHINE_TYPE, 7);  //GDW376.1ר��III���ն�QFP(���Ʋɼ���ԪӲ��v1.3)
  #else
   ioctl(fdOfIoChannel, MACHINE_TYPE, 4);   //GDW376.1ר��III���ն�
  #endif
 #else
  #ifdef JZQ_CTRL_BOARD_V_1_4
   ioctl(fdOfIoChannel, MACHINE_TYPE, 6);   //GDW376.1������(912-9260QFP������)
  #else    
   ioctl(fdOfIoChannel, MACHINE_TYPE, 1);   //GDW376.1������
  #endif
 #endif
   
  
  //�򿪴���1(Modem)  
  if (bakModuleType!=MODEM_PPP)  //2012-11-08,��Ӵ��ж�
  {
   #ifdef PLUG_IN_CARRIER_MODULE 
    if (teIpAndPort.ethIfLoginMs==0x55)
    {
      fdOfModem = NULL;
      
      moduleType = 0xfe;
    }
    else
    {
   #endif
   
      fdOfModem = open("/dev/ttyS1",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���1
  	  if (fdOfModem<=0)
    	{
    		 printf("open /dev/ttyS1 error !\n");
       	 return -1;
      }
      else
      {
    		 printf("open /dev/ttyS1 success!\n");
      }
    
      //ͨ��ģ������
      #ifndef JZQ_CTRL_BOARD_V_1_4   //������Ǽ��������ư�v1.4
       #ifdef TE_CTRL_BOARD_V_1_3    //������ն˿��ư�v1.3
        moduleType = 0xfe;
       #else                         //���Ǽ��������ư�v1.4Ҳ�����ն˿��ư�v1.3
        moduleType = ioctl(fdOfIoChannel, READ_MODULE_TYPE, 0);
        moduleType&=0xf;
      
        if (moduleType==GPRS_MC52I)
        {
          resetWlModem(1);
        }
       
        if (moduleType==GPRS_MC52I)
        {
          uart0Config(2);  //����������Ϊ57600bps
        } 
        else
        {
          uart0Config(0);  //����������Ϊ115200bps
        }
       #endif
      #else                          //����Ǽ��������ư�v1.4
       moduleType = 0xfe;
      #endif

   #ifdef PLUG_IN_CARRIER_MODULE
    }
   #endif
  }
  
 #ifndef JZQ_CTRL_BOARD_V_1_4
  #ifndef TE_CTRL_BOARD_V_1_3
    //��ttyS2(���ư�v0.3-485�����1)
    fdOfttyS2 = open("/dev/ttyS2",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���2
  	if (fdOfttyS2<=0)
  	{
  		 printf("open /dev/ttyS2 error !\n");
     	 return -1;
    }
    else
    {
  		 printf("open /dev/ttyS2 success!\n");
    }
  	tcgetattr(fdOfttyS2,&opt);
  	cfmakeraw(&opt);
  	opt.c_cflag |= (PARENB);              //����ΪżЧ��
    opt.c_iflag |= INPCK;
  	cfsetispeed(&opt,B2400);              //����������Ϊ2400bps
  	cfsetospeed(&opt,B2400);
  	tcsetattr(fdOfttyS2,TCSANOW,&opt);
  
    //��ttyS3(���ư�v0.3-485�����˿�/����485�����2)
    fdOfttyS3 = open("/dev/ttyS3",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���3
  	if (fdOfttyS3<=0)
  	{
  		 printf("open /dev/ttyS3 error !\n");
     	 return -1;
    }
    else
    {
  		 printf("open /dev/ttyS3 success!\n");
    }
  	tcgetattr(fdOfttyS3,&opt);
  	cfmakeraw(&opt);
  	opt.c_cflag |= (PARENB);              //����ΪżЧ��
    opt.c_iflag |= INPCK;
  	cfsetispeed(&opt,B2400);              //����������Ϊ2400bps
  	cfsetospeed(&opt,B2400);
  	tcsetattr(fdOfttyS3,TCSANOW,&opt);
  	
   #ifdef SUPPORT_CASCADE               //���֧�ּ�������
    //��ttyS4(���ư�v0.3-����м�������ʱΪ485�����2)
    fdOfttyS4 = open("/dev/ttyS4",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���4
	  if (fdOfttyS3<=0)
	  {
		  printf("open /dev/ttyS4 error !\n");
   	  return -1;
    }
    else
    {
		  printf("open /dev/ttyS4 success!\n");
    }
	  tcgetattr(fdOfttyS4,&opt);
	  cfmakeraw(&opt);
	  opt.c_cflag |= (PARENB);              //����ΪżЧ��
    opt.c_iflag |= INPCK;
	  cfsetispeed(&opt,B1200);              //����������Ϊ1200bps
	  cfsetospeed(&opt,B1200);
	  tcsetattr(fdOfttyS4,TCSANOW,&opt);
   #endif
  #endif 
 #endif   //no JZQ_CTRL_BOARD_V_1_4
  
   carrierUartInit();

  //�򿪱���ά���ӿ�(�������Ǵ���6,ר��III���Ǵ���4,���������ư�V0.4�Ǵ���4)
  #ifdef LOAD_CTRL_MODULE
   fdOfLocal = open("/dev/ttyS2",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���3
	 if (fdOfLocal<=0)
	 {
		 printf("open /dev/ttyS2 error !\n");
   	 return -1;
   }
   else
   {
		 printf("open /dev/ttyS2 success!\n");
   }
  #else
    
    #ifdef JZQ_CTRL_BOARD_V_0_3
     fdOfLocal = open("/dev/ttyS6",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���6
	   if (fdOfLocal<=0)
	   {
		   printf("open /dev/ttyS6 error !\n");
   	   return -1;
     }
     else
     {
		   printf("open /dev/ttyS6 success!\n");
     }
    #else   //JZQ_CTRL_BOARD_V_0_3
     fdOfLocal = open("/dev/ttyS4",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���4
	   if (fdOfLocal<=0)
	   {
		   printf("open /dev/ttyS4 error !\n");
   	   return -1;
     }
     else
     {
		   printf("open /dev/ttyS4 success!\n");
     }
    #endif  //JZQ_CTRL_BOARD_V_0_3
   #endif
	 
	 tcgetattr(fdOfLocal,&opt);
	 cfmakeraw(&opt);
   opt.c_iflag |= INPCK;
   
	 #ifdef JZQ_CTRL_BOARD_V_0_3
	  cfsetispeed(&opt,B9600);              //����������Ϊ9600bps
	  cfsetospeed(&opt,B9600);
	 #else
	  #ifdef LOAD_CTRL_MODULE
	   //cfsetispeed(&opt,B115200);          //����������Ϊ115200bps
	   //cfsetospeed(&opt,B115200);	  
	   //ly,2011-03-21,�ĳ�38400
	   cfsetispeed(&opt,B38400);          //����������Ϊ38400bps
	   cfsetospeed(&opt,B38400);	  
     opt.c_cflag &= ~PARODD;
     opt.c_cflag |= (PARENB);            //����ΪżЧ��
	  #else
	   //cfsetispeed(&opt,B115200);            //����������ΪB115200bps
	   //cfsetospeed(&opt,B115200);
	   cfsetispeed(&opt,B38400);            //����������ΪB38400bps
	   cfsetospeed(&opt,B38400);
     opt.c_cflag &= ~PARODD;
     opt.c_cflag |= (PARENB);              //����ΪżЧ��
	  #endif
	 #endif
	 
	tcsetattr(fdOfLocal,TCSANOW,&opt);
	 
	//���ɲ������	
	fdOfSample=open("/dev/spi1_cs0",O_RDWR);
	if (fdOfSample == -1)
	{
		printf("Unable to open spi1_cs0 \n\r");
		
		return -1;
	}
 #ifdef LOAD_CTRL_MODULE //ר��III���ն�
  #ifdef CPU_912_9206_QFP
   ioctl(fdOfSample, 88, 5);  //GDW376.1ר��III���ն�(QFP)
  #else
   ioctl(fdOfSample, 88, 4);  //GDW376.1ר��III���ն�
  #endif
 #else                   //������
  #ifdef CPU_912_9206_QFP
   ioctl(fdOfSample, 88, 5);  //GDW376.1ר��III���ն�(QFP)
  #else	  
   ioctl(fdOfSample, 88, 1);  //GDW376.1������(���崦����)
  #endif
 #endif
	
	return 0;
}

/**************************************************
��������:uart0Config
��������:ĳ��������������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void uart0Config(INT8U rate)
{   
   struct termios opt;
	 
	 tcgetattr(fdOfModem,&opt);
	 cfmakeraw(&opt);
   opt.c_iflag |= INPCK;
   
   switch (rate)
   {
     case 1:
	     cfsetispeed(&opt,B9600);              //����������Ϊ9600bps
	     cfsetospeed(&opt,B9600);
       break;
          
     case 2:
	     cfsetispeed(&opt,B57600);             //����������Ϊ57600bps
	     cfsetospeed(&opt,B57600);
       break;

     default:
	     cfsetispeed(&opt,B115200);            //����������Ϊ115200bps
	     cfsetospeed(&opt,B115200);
       break;
   }
   
	 tcsetattr(fdOfModem,TCSANOW,&opt);
}

/**************************************************
��������:oneUartConfig
��������:ĳ��������������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void oneUartConfig(INT32U fd,INT8U ctrlWord)
{
   struct termios opt;
   
	 tcgetattr(fd,&opt);
	 cfmakeraw(&opt);
   
   //����λ
   //����������־
   opt.c_cflag&=~CSIZE;
   switch(ctrlWord&0x3)
   {   	
      case 0:
        opt.c_cflag |=CS5;           //Word Length = 5 Bits
        break;
        
      case 1:
        opt.c_cflag |=CS6;           //Word Length = 6 Bits
        break;
        
      case 2:
        opt.c_cflag |=CS7;           //Word Length = 7 Bits
        break;
        
      default:
        opt.c_cflag |=CS8;           //Word Length = 8 Bits
        break;        
   }
   
   //��żУ��λ
   if (ctrlWord>>3&0x1)
   {
     opt.c_iflag |= INPCK;           //ʹ����ż����λ
     
     if (ctrlWord>>2&0x1)            //��У��(odd parity)
     {
        opt.c_cflag |= (PARODD | PARENB);
     }
     else                            //żУ��(even parity)
     {
        opt.c_cflag &= ~PARODD;
        opt.c_cflag |= (PARENB);
     }   	  
   }
   else
   {
      opt.c_cflag &= ~PARENB;        //�޼���λ(no parity)
   }
   
   //ֹͣλ
   if (ctrlWord>>4&0x1)
   {
      opt.c_cflag |= CSTOPB;         //Two Stop Bit
   }
   else
   {
      opt.c_cflag &= ~CSTOPB;        //One Stop Bit
   }

   //����
   switch(ctrlWord>>5&0x7)
   {
      case 0:
        cfsetispeed(&opt,B300);      //BaudRate = 300 baud
        cfsetospeed(&opt,B300);
        break;
        
      case 1:
        cfsetispeed(&opt,B600);      //BaudRate = 600 baud
        cfsetospeed(&opt,B600);
        break;
        
      case 2:
        cfsetispeed(&opt,B1200);     //BaudRate = 1200 baud
        cfsetospeed(&opt,B1200);
        break;
        
      case 3:
        cfsetispeed(&opt,B2400);     //BaudRate = 2400 baud
        cfsetospeed(&opt,B2400);
        break;
        
      case 4:
        cfsetispeed(&opt,B4800);     //BaudRate = 4800 baud
        cfsetospeed(&opt,B4800);
        break;
        
      case 5:
        cfsetispeed(&opt,B9600);     //BaudRate = 7200 baud,���ò���
        cfsetospeed(&opt,B9600);
        break;
        
      case 6:
        cfsetispeed(&opt,B9600);     //BaudRate = 9600 baud
        cfsetospeed(&opt,B9600);
        break;
        
      case 7:
        cfsetispeed(&opt,B19200);    //BaudRate = 19200 baud
        cfsetospeed(&opt,B19200);
        break;
   }

	 tcsetattr(fd,TCSANOW,&opt);
}

/**************************************************
��������:carrierUartInit
��������:�ز����ڳ�ʼ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void carrierUartInit(void)
{
   #ifdef PLUG_IN_CARRIER_MODULE  //GDW376������
    struct termios opt;
    INT32U i;

    #ifdef JZQ_CTRL_BOARD_V_0_3
      //�򿪴���5(�ز��ӿ�)
      fdOfCarrier = open("/dev/ttyS5",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���5
	    if (fdOfCarrier<=0)
	    {
		    printf("open /dev/ttyS5 error !\n");
   	    return -1;
      }
      else
      {
		    printf("open /dev/ttyS5 success!\n");
      }
    #else
      //�򿪴���3(�ز��ӿ�)
      fdOfCarrier = open("/dev/ttyS3",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���3
	    if (fdOfCarrier<=0)
	    {
		    printf("open /dev/ttyS3 error !\n");
   	    return -1;
      }
      else
      {
		    printf("open /dev/ttyS3 success!\n");
      }
    #endif
    
	  tcgetattr(fdOfCarrier,&opt);
	  cfmakeraw(&opt);
	  opt.c_cflag |= (PARENB);              //����ΪżЧ��
    opt.c_iflag |= INPCK;
	 
	 #ifdef LM_SUPPORT_UT
    if (0x55==lmProtocol)
    {
	    cfsetispeed(&opt,B2400);              //����������Ϊ2400bps
	    cfsetospeed(&opt,B2400);
    }
    else
    {
   #endif
 
	    cfsetispeed(&opt,B9600);              //����������Ϊ9600bps
	    cfsetospeed(&opt,B9600);
	    
	 #ifdef LM_SUPPORT_UT
	  }
	 #endif

	  tcsetattr(fdOfCarrier,TCSANOW,&opt);

    //��λ�ز�ģ��
	  //ly,2010-09-16,����ɣ������ģ��ʱ�����������
	  printf("��λ�ز�ģ��\n");

    #ifdef YONGCHUANG_OLD_JZQ
     ioctl(fdOfIoChannel,RST_CARRIER_MODULE,1);
     for(i=0;i<0x30000;i++)
     {
   	   ;
     }
     ioctl(fdOfIoChannel,RST_CARRIER_MODULE,0);
     
    #else

     #ifdef JZQ_CTRL_BOARD_V_0_3
      ioctl(fdOfIoChannel,SET_CARRIER_MODULE,0);   //����߼�Ҳ�Ƿ���,��0�Ǹߣ���1�ǵ�
     #else
      ioctl(fdOfIoChannel,SET_CARRIER_MODULE,1);
     #endif
     
     sleep(1);
     
     ioctl(fdOfIoChannel,RST_CARRIER_MODULE,0);
     for(i=0;i<0x30000;i++)
     {
   	   ;
     }
     ioctl(fdOfIoChannel,RST_CARRIER_MODULE,1);
    #endif
	 #endif  //PLUG_IN_CARRIER_MODULE  GDW376������ -END-
}

/*******************************************************
��������:sendLocalMsFrame
��������:���ͱ��ؿ�����վ���ݰ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void sendLocalMsFrame(INT8U *pack,INT16U length)
{
   #ifdef WDOG_USE_X_MEGA
    INT8U buf[1024];
    
    buf[0] = fQueue.frame[fQueue.sendPtr].len&0xff;
    buf[1] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
    memcpy(&buf[2], pack, length);
    sendXmegaFrame(MAINTAIN_DATA, buf, length+2);  
   #else
    write(fdOfLocal,pack,length);
   #endif
}

/*******************************************************
��������:sendWirelessFrame
��������:��������Modem���ݰ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void sendWirelessFrame(INT8U *pack,INT16U length)
{
   write(fdOfModem, pack, length);
   
   //ly,2011-01-30,����,��Modem��������֡��ȴ���ʱ
   if (wlModemFlag.sendToModem == 0)
   {
     wlModemFlag.sendToModem  = 1;
     wlModemFlag.waitModemRet = nextTime(sysTime, WAIT_MODEM_MINUTES, 0);
   }
    
   if (debugInfo&WIRELESS_DEBUG)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02dԶ��MODEM Tx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
     sendDebugFrame(pack, length);
     
     if (wlModemFlag.loginSuccess==0)
     {
     	 pack[length] = '\0';
     	 printf("%s\n",(char *)pack);
     }
   }
}

/*******************************************************
��������:sendDebugFrame
��������:���͵������ݰ�         
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void sendDebugFrame(INT8U *pack,INT16U length)
{
   INT16U i;

   for(i=0;i<length;i++)
   {
   	  printf("%02X ",*pack++);
   }
   printf("\n");
}

/*******************************************************
��������:detectReset
��������:����Ƿ�Ӧ�ø�λ�ն�
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void detectReset(BOOL secondChanged)
{
	 if (ifReset==TRUE)
	 {
	 	 //2012-07-27,add
	 	 sqlite3_close(sqlite3Db);

	 	 reboot(RB_AUTOBOOT);
	 	 return;
	 }
	 
	 if (cmdReset>0)
	 {
	 	  if (secondChanged==TRUE)
	 	  {
	 	  	 cmdReset++;
	 	  	 if (cmdReset>5)
	 	  	 {
            #ifdef PULSE_GATHER
             if (flagOfClearPulse==0x55 || flagOfClearPulse==0xaa)
             {
               //ɾ���������м�ֵ,�����������ݵ��洢�ڲ����е�
               deleteParameter(88, 3);
               deleteParameter(88, 13);
               if (debugInfo&PRINT_PULSE_DEBUG)
               {
                 printf("detectReset:ɾ���������м�ֵ\n");
               }
             }
            #endif
	 	        
	 	        //2012-07-27,add
	 	        sqlite3_close(sqlite3Db);

	 	  	 	  reboot(RB_AUTOBOOT);
	 	  	 }
	 	  }
	 }
}

/***************************************************
��������:setBeeper
��������:���÷�����(ʹ�䷢������)
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void setBeeper(INT8U ifVoice)
{
  if (ifVoice==BEEPER_ON)
  {
    if ((voiceAlarm[sysTime.hour/8]>>(sysTime.hour%8)) & 0x1)
    {
	    ioctl(fdOfIoChannel, SET_ALARM_VOICE, ifVoice);
	  }
	}
	else
	{
	  ioctl(fdOfIoChannel, SET_ALARM_VOICE, ifVoice);
	}
}

/***************************************************
��������:alarmLedCtrl
��������:�澯LED�ƿ���(ʹ��������)
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void alarmLedCtrl(INT8U ifLight)
{
 #ifdef LOAD_CTRL_MODULE
 	if (ifLight==ALARM_LED_ON)
 	{
    statusOfGate |= 0x80;   //�澯����
  }
  else
  {
    statusOfGate &= 0x7f;  //�澯����
  }     
 #else
  #ifdef JZQ_CTRL_BOARD_V_0_3
   ioctl(fdOfIoChannel,SET_ALARM_LIGHT,ifLight);
  #else
   ioctl(fdOfIoChannel,SET_ALARM_LIGHT,(~ifLight)&0x1);
  #endif
 #endif
}

/***************************************************
��������:getGateKValue
��������:��ȡ�ſ�ֵ
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
INT8U getGateKValue(void)
{
   #ifdef TE_CTRL_BOARD_V_1_3
    return gateKvalue;
   #else
    return ioctl(fdOfIoChannel, READ_GATEK_VALUE, 0);
   #endif
}

#ifdef WDOG_USE_X_MEGA
/***************************************************
��������:sendXmegaFrame
��������:��֯���ݵ�AVR��Ƭ��ATxMega16/32A4����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
INT8U countRun=0;
void sendXmegaFrame(INT8U afn, INT8U *data, INT16U len)
{
    INT8U  sendBuf[1024];
    INT16U j;
    INT8U  checkSum;
    INT8U  pSeq = 0;
    INT16U tmpHead;
    
    usleep(50000);
    
    sendBuf[0] = 0x68;               //֡��ʼ�ַ�
    sendBuf[1] = (len+3)&0xff;       //L
    sendBuf[2] = (len+3)>>8; 
    sendBuf[3] = 0x68;               //֡��ʼ�ַ�
 
    #ifdef TE_CTRL_BOARD_V_1_3
     //��ַ�����ָʾ��
     //(bit0--bit3��1����4����/��բ,bit4-���ص�,bit5-��ص�,bit6-���е�,bit7-�澯��)
     sendBuf[4] = statusOfGate;
    #else
     sendBuf[4] = 0xaa;              //��ַ
    #endif
    sendBuf[5] = afn;                //AFN
    sendBuf[6] = 0x30;               //��֡
    
    //���ƾ�����
    memcpy(&sendBuf[7], data, len);

    //У���
    checkSum = 0x0;
    for(j=4; j<7+len; j++)
    {
       checkSum += sendBuf[j];
    }
    sendBuf[7+len] = checkSum;
    
    //֡��β
    sendBuf[8+len] = 0x16;

    if (xMegaQueue.tailPtr == 0)
    {
       tmpHead = 0;
    }
    else
    {
       tmpHead = xMegaQueue.frame[xMegaQueue.tailPtr-1].head + xMegaQueue.frame[xMegaQueue.tailPtr-1].len;
    }
    
    xMegaQueue.frame[xMegaQueue.tailPtr].head = tmpHead;
    xMegaQueue.frame[xMegaQueue.tailPtr].len  = 9+len;
    memcpy(&xMegaFrame[tmpHead], sendBuf, 9+len);
    
    if ((tmpHead+9+len+128)>2048 || xMegaQueue.tailPtr==15)
    {
       xMegaQueue.frame[fQueue.tailPtr].next = 0x0;
    	 xMegaQueue.tailPtr = 0;
    }
    else
    {
       xMegaQueue.frame[xMegaQueue.tailPtr].next = xMegaQueue.tailPtr+1;
       xMegaQueue.tailPtr++;
    }
}

/***************************************************
��������:sendXmegaInTimeFrame
��������:��֯���ݲ���ʱ����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void sendXmegaInTimeFrame(INT8U afn, INT8U *data, INT16U len)
{
    INT8U  sendBuf[256];
    INT16U j;
    INT8U  checkSum;
    INT8U  pSeq = 0;
    INT16U tmpHead;
    struct timeval tv;

    usleep(10000);
    
    sendBuf[0] = 0x68;               //֡��ʼ�ַ�
    sendBuf[1] = (len+3)&0xff;       //L
    sendBuf[2] = (len+3)>>8; 
    sendBuf[3] = 0x68;               //֡��ʼ�ַ�
 
    #ifdef TE_CTRL_BOARD_V_1_3
     //��ַ�����ָʾ��
     //(bit0--bit3��1����4����/��բ,bit4-���ص�,bit5-��ص�,bit6-���е�,bit7-�澯��)
     sendBuf[4] = statusOfGate;
    #else
     sendBuf[4] = 0xaa;              //��ַ
    #endif
    sendBuf[5] = afn;                //AFN
    sendBuf[6] = 0x30;               //��֡
    
    //���ƾ�����
    memcpy(&sendBuf[7], data, len);

    //У���
    checkSum = 0x0;
    for(j=4; j<7+len; j++)
    {
       checkSum += sendBuf[j];
    }
    sendBuf[7+len] = checkSum;
    
    //֡��β
    sendBuf[8+len] = 0x16;
    
    write(fdOfLocal, sendBuf, 9+len);
    
    if (debugInfo&PRINT_XMEGA_DEBUG)
    {
       gettimeofday(&tv, NULL);
       printf("%d-%d-%d %d:%d:%d ��=%d,΢��=%d(sendXmegaInTimeFrame)->",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,tv.tv_sec, tv.tv_usec);
       sendDebugFrame(sendBuf, 9+len);
    }
}

#endif
