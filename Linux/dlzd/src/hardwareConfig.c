/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:hardwareConfig.c
作者:leiyong
版本:0.9
完成日期:2010年1月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)硬件配置文件
函数列表:

修改历史:
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

int fdOfModem=NULL;//串口1(无线Modem)文件描述符
int fdOfttyS2;     //串口2(485接口1)文件描述符
int fdOfttyS3;     //串口3(485接口2)文件描述符
int fdOfttyS4;     //串口4(485接口3)文件描述符
int fdOfCarrier;   //串口5(载波接口)文件描述符
int fdOfLocal;     //串口6(本地维护接口)文件描述符
int fdOfSample;    //交采文件描述符
int fdOfIoChannel; //I/O通道文件描述符

extern INT32U avrHeartTime;                         //与AVR单片机通信的心跳时间

INT8U  cmd[5];

/**************************************************
函数名称:initHardware
功能描述:电力终端(负控终端、集中器,AT91SAM9260处理器)初始化硬件函数
调用函数:
被调用函数:
输入参数:无
输出参数:void
返回值:void
***************************************************/
int initHardware(void)
{
  struct termios opt;
  INT32U i;
  
  //打开IO通道
  fdOfIoChannel = open("/dev/ioChannel", O_RDWR|O_NOCTTY);  //读写方式打开I/O通道
	if (fdOfIoChannel<=0)
	{
		printf("open /dev/ioChannel error !\n");
   	return -1;
  }
  else
  {
		printf("open /dev/ioChannel success!\n");
  }
  
  //复位ESAM芯片
  ioctl(fdOfIoChannel, ESAM_RST, 1);
  

 #ifdef LOAD_CTRL_MODULE
  #ifdef CPU_912_9206_QFP
   ioctl(fdOfIoChannel, MACHINE_TYPE, 7);  //GDW376.1专变III型终端QFP(控制采集单元硬件v1.3)
  #else
   ioctl(fdOfIoChannel, MACHINE_TYPE, 4);   //GDW376.1专变III型终端
  #endif
 #else
  #ifdef JZQ_CTRL_BOARD_V_1_4
   ioctl(fdOfIoChannel, MACHINE_TYPE, 6);   //GDW376.1集中器(912-9260QFP处理器)
  #else    
   ioctl(fdOfIoChannel, MACHINE_TYPE, 1);   //GDW376.1集中器
  #endif
 #endif
   
  
  //打开串口1(Modem)  
  if (bakModuleType!=MODEM_PPP)  //2012-11-08,添加此判断
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
   
      fdOfModem = open("/dev/ttyS1",O_RDWR|O_NOCTTY);  //读写方式打开串口1
  	  if (fdOfModem<=0)
    	{
    		 printf("open /dev/ttyS1 error !\n");
       	 return -1;
      }
      else
      {
    		 printf("open /dev/ttyS1 success!\n");
      }
    
      //通信模块类型
      #ifndef JZQ_CTRL_BOARD_V_1_4   //如果不是集中器控制板v1.4
       #ifdef TE_CTRL_BOARD_V_1_3    //如果是终端控制板v1.3
        moduleType = 0xfe;
       #else                         //不是集中器控制板v1.4也不是终端控制板v1.3
        moduleType = ioctl(fdOfIoChannel, READ_MODULE_TYPE, 0);
        moduleType&=0xf;
      
        if (moduleType==GPRS_MC52I)
        {
          resetWlModem(1);
        }
       
        if (moduleType==GPRS_MC52I)
        {
          uart0Config(2);  //波特率设置为57600bps
        } 
        else
        {
          uart0Config(0);  //波特率设置为115200bps
        }
       #endif
      #else                          //如果是集中器控制板v1.4
       moduleType = 0xfe;
      #endif

   #ifdef PLUG_IN_CARRIER_MODULE
    }
   #endif
  }
  
 #ifndef JZQ_CTRL_BOARD_V_1_4
  #ifndef TE_CTRL_BOARD_V_1_3
    //打开ttyS2(控制板v0.3-485抄表口1)
    fdOfttyS2 = open("/dev/ttyS2",O_RDWR|O_NOCTTY);  //读写方式打开串口2
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
  	opt.c_cflag |= (PARENB);              //设置为偶效验
    opt.c_iflag |= INPCK;
  	cfsetispeed(&opt,B2400);              //波特率设置为2400bps
  	cfsetospeed(&opt,B2400);
  	tcsetattr(fdOfttyS2,TCSANOW,&opt);
  
    //打开ttyS3(控制板v0.3-485级联端口/或是485抄表口2)
    fdOfttyS3 = open("/dev/ttyS3",O_RDWR|O_NOCTTY);  //读写方式打开串口3
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
  	opt.c_cflag |= (PARENB);              //设置为偶效验
    opt.c_iflag |= INPCK;
  	cfsetispeed(&opt,B2400);              //波特率设置为2400bps
  	cfsetospeed(&opt,B2400);
  	tcsetattr(fdOfttyS3,TCSANOW,&opt);
  	
   #ifdef SUPPORT_CASCADE               //如果支持级联功能
    //打开ttyS4(控制板v0.3-如果有级联功能时为485抄表口2)
    fdOfttyS4 = open("/dev/ttyS4",O_RDWR|O_NOCTTY);  //读写方式打开串口4
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
	  opt.c_cflag |= (PARENB);              //设置为偶效验
    opt.c_iflag |= INPCK;
	  cfsetispeed(&opt,B1200);              //波特率设置为1200bps
	  cfsetospeed(&opt,B1200);
	  tcsetattr(fdOfttyS4,TCSANOW,&opt);
   #endif
  #endif 
 #endif   //no JZQ_CTRL_BOARD_V_1_4
  
   carrierUartInit();

  //打开本地维护接口(集中器是串口6,专变III型是串口4,集中器控制板V0.4是串口4)
  #ifdef LOAD_CTRL_MODULE
   fdOfLocal = open("/dev/ttyS2",O_RDWR|O_NOCTTY);  //读写方式打开串口3
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
     fdOfLocal = open("/dev/ttyS6",O_RDWR|O_NOCTTY);  //读写方式打开串口6
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
     fdOfLocal = open("/dev/ttyS4",O_RDWR|O_NOCTTY);  //读写方式打开串口4
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
	  cfsetispeed(&opt,B9600);              //波特率设置为9600bps
	  cfsetospeed(&opt,B9600);
	 #else
	  #ifdef LOAD_CTRL_MODULE
	   //cfsetispeed(&opt,B115200);          //波特率设置为115200bps
	   //cfsetospeed(&opt,B115200);	  
	   //ly,2011-03-21,改成38400
	   cfsetispeed(&opt,B38400);          //波特率设置为38400bps
	   cfsetospeed(&opt,B38400);	  
     opt.c_cflag &= ~PARODD;
     opt.c_cflag |= (PARENB);            //设置为偶效验
	  #else
	   //cfsetispeed(&opt,B115200);            //波特率设置为B115200bps
	   //cfsetospeed(&opt,B115200);
	   cfsetispeed(&opt,B38400);            //波特率设置为B38400bps
	   cfsetospeed(&opt,B38400);
     opt.c_cflag &= ~PARODD;
     opt.c_cflag |= (PARENB);              //设置为偶效验
	  #endif
	 #endif
	 
	tcsetattr(fdOfLocal,TCSANOW,&opt);
	 
	//交采操作句柄	
	fdOfSample=open("/dev/spi1_cs0",O_RDWR);
	if (fdOfSample == -1)
	{
		printf("Unable to open spi1_cs0 \n\r");
		
		return -1;
	}
 #ifdef LOAD_CTRL_MODULE //专变III型终端
  #ifdef CPU_912_9206_QFP
   ioctl(fdOfSample, 88, 5);  //GDW376.1专变III型终端(QFP)
  #else
   ioctl(fdOfSample, 88, 4);  //GDW376.1专变III型终端
  #endif
 #else                   //集中器
  #ifdef CPU_912_9206_QFP
   ioctl(fdOfSample, 88, 5);  //GDW376.1专变III型终端(QFP)
  #else	  
   ioctl(fdOfSample, 88, 1);  //GDW376.1集中器(迈冲处理器)
  #endif
 #endif
	
	return 0;
}

/**************************************************
函数名称:uart0Config
功能描述:某个串口特殊配置
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
	     cfsetispeed(&opt,B9600);              //波特率设置为9600bps
	     cfsetospeed(&opt,B9600);
       break;
          
     case 2:
	     cfsetispeed(&opt,B57600);             //波特率设置为57600bps
	     cfsetospeed(&opt,B57600);
       break;

     default:
	     cfsetispeed(&opt,B115200);            //波特率设置为115200bps
	     cfsetospeed(&opt,B115200);
       break;
   }
   
	 tcsetattr(fdOfModem,TCSANOW,&opt);
}

/**************************************************
函数名称:oneUartConfig
功能描述:某个串口特殊配置
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void oneUartConfig(INT32U fd,INT8U ctrlWord)
{
   struct termios opt;
   
	 tcgetattr(fd,&opt);
	 cfmakeraw(&opt);
   
   //数据位
   //屏蔽其他标志
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
   
   //奇偶校验位
   if (ctrlWord>>3&0x1)
   {
     opt.c_iflag |= INPCK;           //使能奇偶检验位
     
     if (ctrlWord>>2&0x1)            //奇校验(odd parity)
     {
        opt.c_cflag |= (PARODD | PARENB);
     }
     else                            //偶校验(even parity)
     {
        opt.c_cflag &= ~PARODD;
        opt.c_cflag |= (PARENB);
     }   	  
   }
   else
   {
      opt.c_cflag &= ~PARENB;        //无检验位(no parity)
   }
   
   //停止位
   if (ctrlWord>>4&0x1)
   {
      opt.c_cflag |= CSTOPB;         //Two Stop Bit
   }
   else
   {
      opt.c_cflag &= ~CSTOPB;        //One Stop Bit
   }

   //速率
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
        cfsetispeed(&opt,B9600);     //BaudRate = 7200 baud,设置不起
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
函数名称:carrierUartInit
功能描述:载波串口初始化
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void carrierUartInit(void)
{
   #ifdef PLUG_IN_CARRIER_MODULE  //GDW376集中器
    struct termios opt;
    INT32U i;

    #ifdef JZQ_CTRL_BOARD_V_0_3
      //打开串口5(载波接口)
      fdOfCarrier = open("/dev/ttyS5",O_RDWR|O_NOCTTY);  //读写方式打开串口5
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
      //打开串口3(载波接口)
      fdOfCarrier = open("/dev/ttyS3",O_RDWR|O_NOCTTY);  //读写方式打开串口3
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
	  opt.c_cflag |= (PARENB);              //设置为偶效验
    opt.c_iflag |= INPCK;
	 
	 #ifdef LM_SUPPORT_UT
    if (0x55==lmProtocol)
    {
	    cfsetispeed(&opt,B2400);              //波特率设置为2400bps
	    cfsetospeed(&opt,B2400);
    }
    else
    {
   #endif
 
	    cfsetispeed(&opt,B9600);              //波特率设置为9600bps
	    cfsetospeed(&opt,B9600);
	    
	 #ifdef LM_SUPPORT_UT
	  }
	 #endif

	  tcsetattr(fdOfCarrier,TCSANOW,&opt);

    //复位载波模块
	  //ly,2010-09-16,调试桑锐无线模块时屏蔽以下这段
	  printf("复位载波模块\n");

    #ifdef YONGCHUANG_OLD_JZQ
     ioctl(fdOfIoChannel,RST_CARRIER_MODULE,1);
     for(i=0;i<0x30000;i++)
     {
   	   ;
     }
     ioctl(fdOfIoChannel,RST_CARRIER_MODULE,0);
     
    #else

     #ifdef JZQ_CTRL_BOARD_V_0_3
      ioctl(fdOfIoChannel,SET_CARRIER_MODULE,0);   //这个逻辑也是反的,置0是高，置1是低
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
	 #endif  //PLUG_IN_CARRIER_MODULE  GDW376集中器 -END-
}

/*******************************************************
函数名称:sendLocalMsFrame
功能描述:发送本地控制主站数据包
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
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
函数名称:sendWirelessFrame
功能描述:发送无线Modem数据包
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void sendWirelessFrame(INT8U *pack,INT16U length)
{
   write(fdOfModem, pack, length);
   
   //ly,2011-01-30,增加,向Modem发送数据帧后等待超时
   if (wlModemFlag.sendToModem == 0)
   {
     wlModemFlag.sendToModem  = 1;
     wlModemFlag.waitModemRet = nextTime(sysTime, WAIT_MODEM_MINUTES, 0);
   }
    
   if (debugInfo&WIRELESS_DEBUG)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d远程MODEM Tx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
     sendDebugFrame(pack, length);
     
     if (wlModemFlag.loginSuccess==0)
     {
     	 pack[length] = '\0';
     	 printf("%s\n",(char *)pack);
     }
   }
}

/*******************************************************
函数名称:sendDebugFrame
功能描述:发送调试数据包         
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
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
函数名称:detectReset
功能描述:检测是否应该复位终端
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
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
               //删除脉冲量中间值,这两个是数据但存储在参数中的
               deleteParameter(88, 3);
               deleteParameter(88, 13);
               if (debugInfo&PRINT_PULSE_DEBUG)
               {
                 printf("detectReset:删除脉冲量中间值\n");
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
函数名称:setBeeper
功能描述:设置蜂鸣器(使其发声或静音)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
函数名称:alarmLedCtrl
功能描述:告警LED灯控制(使其亮或灭)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void alarmLedCtrl(INT8U ifLight)
{
 #ifdef LOAD_CTRL_MODULE
 	if (ifLight==ALARM_LED_ON)
 	{
    statusOfGate |= 0x80;   //告警灯亮
  }
  else
  {
    statusOfGate &= 0x7f;  //告警灯灭
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
函数名称:getGateKValue
功能描述:读取门控值
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
函数名称:sendXmegaFrame
功能描述:组织数据到AVR单片机ATxMega16/32A4缓存
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
    
    sendBuf[0] = 0x68;               //帧起始字符
    sendBuf[1] = (len+3)&0xff;       //L
    sendBuf[2] = (len+3)>>8; 
    sendBuf[3] = 0x68;               //帧起始字符
 
    #ifdef TE_CTRL_BOARD_V_1_3
     //地址域代替指示灯
     //(bit0--bit3第1到第4轮跳/合闸,bit4-功控灯,bit5-电控灯,bit6-运行灯,bit7-告警灯)
     sendBuf[4] = statusOfGate;
    #else
     sendBuf[4] = 0xaa;              //地址
    #endif
    sendBuf[5] = afn;                //AFN
    sendBuf[6] = 0x30;               //单帧
    
    //复制净数据
    memcpy(&sendBuf[7], data, len);

    //校验和
    checkSum = 0x0;
    for(j=4; j<7+len; j++)
    {
       checkSum += sendBuf[j];
    }
    sendBuf[7+len] = checkSum;
    
    //帧结尾
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
函数名称:sendXmegaInTimeFrame
功能描述:组织数据并及时发送
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
    
    sendBuf[0] = 0x68;               //帧起始字符
    sendBuf[1] = (len+3)&0xff;       //L
    sendBuf[2] = (len+3)>>8; 
    sendBuf[3] = 0x68;               //帧起始字符
 
    #ifdef TE_CTRL_BOARD_V_1_3
     //地址域代替指示灯
     //(bit0--bit3第1到第4轮跳/合闸,bit4-功控灯,bit5-电控灯,bit6-运行灯,bit7-告警灯)
     sendBuf[4] = statusOfGate;
    #else
     sendBuf[4] = 0xaa;              //地址
    #endif
    sendBuf[5] = afn;                //AFN
    sendBuf[6] = 0x30;               //单帧
    
    //复制净数据
    memcpy(&sendBuf[7], data, len);

    //校验和
    checkSum = 0x0;
    for(j=4; j<7+len; j++)
    {
       checkSum += sendBuf[j];
    }
    sendBuf[7+len] = checkSum;
    
    //帧结尾
    sendBuf[8+len] = 0x16;
    
    write(fdOfLocal, sendBuf, 9+len);
    
    if (debugInfo&PRINT_XMEGA_DEBUG)
    {
       gettimeofday(&tv, NULL);
       printf("%d-%d-%d %d:%d:%d 秒=%d,微秒=%d(sendXmegaInTimeFrame)->",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,tv.tv_sec, tv.tv_usec);
       sendDebugFrame(sendBuf, 9+len);
    }
}

#endif
