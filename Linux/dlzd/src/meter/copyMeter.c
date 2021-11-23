/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名：copyMeter.c
作者：leiyong
版本：0.9
完成日期：2007年4月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)抄表线程文件
函数列表：
     1.
修改历史：
  01,07-04-13,Leiyong created.
  02,07-07-02,Leiyong,modify,小时冻结后进行写FLASH操作
  03,07-07-09,Leiyong,修改交流采样与电能表在测量点配置参数(F10)中的位置无关性
  04,07-09-14,Tianye增加645精简规约(正向电量、正反向电量、电能量及...)
  05,09-03-10,Leiyong修改
              1)将抄表工况改在实时统计数据2(realStatisBuff2)中
              2)抄表成功除判断不是抄表故障外,另加条件有数据(havePresentData!0)防止未抄到表而抄表项又小于6项记为抄表成功
  06,09-07-20,Leiyong增加convertAcSample函数,将交采转换用一个函数实现,以免转换的地方多了改了一个地方而其它地方未改到
  07,10-08-04,Leiyong,增加采集相位角数据项及转换
  08,10-08-17,Leiyong,增加重庆电力公司要求的电能表类型(三相一般工商业、单相一般工商业、居民用户表等,这些表只处理07规约,因为需要的数据项97表基本没有)
  09,10-08-18,Leiyong,修改,载波端口抄表间隔固定为5min,防止未配置抄表间隔时不抄载波表
  10,10-08-18,Leiyong,修改,载波端口抄表间隔固定为10min
  11,l0-11-22,Leiyong,修改,载波端口抄表时间固定为每小时的1分
  12,10-11-22,Leiyong,修改,整点冻结数据采集。
              由于某些电表的整点冻结数据不存在，这样的话，就会每小时都抄一天的整点冻结数据
              10-11-22晚已改成如果没有配置该测量点的示值曲线的任务就不采集了
  13,10-11-22,Leiyong,修改,有些电表本来就抄不到的处理。
              改成如果某测量点两个数据项DI未抄到的话(即2*2,每个数据项要重发一次)就停止该测量点本项的抄收,
              以免浪费其它表的抄收时间。
  14,11-07-07,Leiyong,修改东软、弥亚微、鼎信为路由主导抄读方式。
  15,13-10-17,Leiyong,添加对照明集中器的抄表处理
***************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include <termios.h> 
#include <sys/ioctl.h> 
#include <net/if.h> 
#include <sys/wait.h>


#include "hardwareConfig.h"
#include "meterProtocol.h"
#include "teRunPara.h"
#include "msSetPara.h"

#include "dataBase.h"
#include "dataBalance.h"
#include "convert.h"
#include "userInterface.h"
#include "ioChannel.h"
#include "att7022b.h"

#ifdef PLUG_IN_CARRIER_MODULE
 #include "gdw376-2.h"
#endif

#ifdef LIGHTING
 #include "md5.h"
#endif

#ifdef SUPPORT_ETH_COPY
 #include "meterProtocolx.h"
#endif


#include "copyMeter.h"

extern BOOL  secondChanged;         //秒已改变
extern DATE_TIME nextHeartBeatTime; //下一次心跳时间

//各种机型公共变量**********************************************
//采集开关量/脉冲量变量
INT8U        stOfSwitchRead;                    //读取状态量状态变量
INT8U        tmpSwitch,tmpCd;
INT8U        reportToMain;                      //状态量事件是否向主站上报
INT8U        stOfSwitchBaks[10];                //10次备份状态量状态
INT8U        numOfStGather=0;                   //采集状态量次数
INT8U        ifSystemStartup = 0;               //系统刚启动?

//各种机型公共变量**********end*********************************

INT8U        pulsei;
INT8U        hasEvent=0;
INT32U       tmpPulseData;

struct dotFn129 *pDotFn129=NULL;
INT8U        dotReplyStart = 0;


INT8U        abbHandclasp[3]={0,0,0};           //ABB握手是否成功

//**************************************************************











//***********************************公共函数****************************************************

//Modbus CRC 校验算法
//算法一
unsigned int calccrc(unsigned char crcbuf, unsigned char  crc)
{
  unsigned char i; 
  crc=crc ^ crcbuf;

  for(i=0;i<8;i++)
  {
    unsigned char chk;
    chk=crc&1;
    crc=crc>>1;
    crc=crc&0x7fff;

    if (chk==1)
    crc=crc^0xa001;

    crc=crc&0xffff;
  }

  return crc; 
}

unsigned int chkcrc(unsigned char *buf, unsigned char  len)
{
  unsigned char hi,lo;
  unsigned int  i;
  unsigned int  crc;

  crc=0xFFFF;
  for (i=0;i<len;i++)
  {
    crc=calccrc(*buf,crc);
    buf++;
  }

  hi=crc%256;
  lo=crc/256;
  crc=(hi<<8)|lo;

  return crc;
}

//算法二,查表法
unsigned char auchCRCHi[]=
{
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
  0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40
};

unsigned char auchCRCLo[] =
{
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
  0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
  0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
  0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
  0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
  0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
  0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
  0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
  0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
  0x40
};

unsigned int CRC16(unsigned char *updata, unsigned char len)
{
  unsigned char uchCRCHi=0xff;
  unsigned char uchCRCLo=0xff;
  unsigned int  uindex;
  while(len--)
  {
    uindex=uchCRCHi^*updata++;
    uchCRCHi=uchCRCLo^auchCRCHi[uindex];
    uchCRCLo=auchCRCLo[uindex];
  }

  return (uchCRCHi<<8|uchCRCLo);
}



/*******************************************************
函数名称:compareTwoAddr
功能描述:比较两个地址是否一样
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE-一样,FALSE-不一样
*******************************************************/
BOOL compareTwoAddr(INT8U *addr1, INT8U *addr2, INT8U type)
{
   if (type==1)    //比较地址1是否为全0
   {
     if (addr1[0]==0 && addr1[1]==0 && addr1[2]==0&& addr1[3]==0 && addr1[4]==0 && addr1[5]==0)
     {
   	   return TRUE;
   	 }
   }
   else
   {
     if (addr1[0]==addr2[0] && addr1[1]==addr2[1] && addr1[2]==addr2[2]
   	     && addr1[3]==addr2[3] && addr1[4]==addr2[4] && addr1[5]==addr2[5]
     	  )
     {
   	   return TRUE;
   	 }
   }
   
   return FALSE;
}

/***************************************************
函数名称:eastMsgSwap
功能描述:东软采集器有地址方式的报文转换
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void eastMsgSwap(INT8U *buf,INT8U *len)
{
  INT16U i;

  for(i = 0; i < 6; i ++)
  {
    buf[1 + i] = buf[*len - 8 + i] - 0x33;
  }
  buf[9] -= 6;
  *len   -= 6;

  buf[*len - 2] = 0x00;
  for(i = 0; i < (*len - 2); i ++)
  {
    buf[*len - 2] += buf[i];
  }

  buf[*len - 1] = 0x16;
  
  if (debugInfo&PRINT_CARRIER_DEBUG)
  {
  	printf("东软采集器有地址方案:接收报文交换地址处理\n");
  }
}

/*******************************************************
函数名称:sendMeterFrame
功能描述:发送电能表数据帧
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void sendMeterFrame(INT8U port,INT8U *pack,INT16U length)
{
  INT8U i;
   
 #ifdef WDOG_USE_X_MEGA
  INT8U buf[512];
 #endif
   
  if (debugInfo&METER_DEBUG)
  {
    if (port==31 && (debugInfo&PRINT_CARRIER_DEBUG))
    {
      printf("%02d-%02d-%02d %02d:%02d:%02d Tx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
   
      for(i=0;i<length;i++)
      {
        printf("%02x ",*(pack+i));
      }
      printf("\n");
    }
  }
   
  if (debugInfo&PRINT_645_FRAME)
  {
    if (port!=31)
    {
      printf("%02d-%02d-%02d %02d:%02d:%02d Port %d Tx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port);
   
      for(i=0;i<length;i++)
      {
        printf("%02x ",*(pack+i));
      }
      printf("\n");
    }   	 
  }
   
   switch(port)
   {
     //2012-3-28,统一为端口1
     case PORT_NO_1:
       #ifdef WDOG_USE_X_MEGA
         buf[0] = 0x1;
         buf[1] = length&0xff;
         buf[2] = length>>8&0xff;
         memcpy(&buf[3], pack, length);
         if (abbHandclasp[0]==1)
         {
           sendXmegaInTimeFrame(COPY_DATA, buf, length+3);
  	    	 
  	    	 if (debugInfo&METER_DEBUG)
  	    	 {           
             printf("端口%dABB表握手中,使用sendXmegaInTimeFrame发送\n", port);
           }
         }
         else
         {
           sendXmegaFrame(COPY_DATA, buf, length+3);
         }
       #else
        write(fdOfttyS2, pack, length);
       #endif
       break;
        
     //2012-3-28,统一为端口2
     case PORT_NO_2:
       //如果软件支持级联,则第2路抄表口为ttyS4;
       //如果软件不持级联,则第2路抄表口为ttyS3
       #ifdef WDOG_USE_X_MEGA
         buf[0] = 0x2;
         buf[1] = length&0xff;
         buf[2] = length>>8&0xff;
         memcpy(&buf[3], pack, length);
         if (abbHandclasp[1]==1)
         {
           sendXmegaInTimeFrame(COPY_DATA, buf, length+3);

  	    	 if (debugInfo&METER_DEBUG)
  	    	 {           
             printf("端口%dABB表握手中,使用sendXmegaInTimeFrame发送\n", port);
           }
         }
         else
         {
           sendXmegaFrame(COPY_DATA, buf, length+3);
         }
       #else
        #ifdef SUPPORT_CASCADE
         write(fdOfttyS4,pack,length);
        #else
         write(fdOfttyS3,pack,length);
        #endif
       #endif
       break;
       
     case PORT_NO_3:
       #ifdef WDOG_USE_X_MEGA
         buf[0] = 0x3;
         buf[1] = length&0xff;
         buf[2] = length>>8&0xff;
         memcpy(&buf[3], pack, length);
         if (abbHandclasp[3]==1)
         {
           sendXmegaInTimeFrame(COPY_DATA, buf, length+3);
  	    	 
  	    	 if (debugInfo&METER_DEBUG)
  	    	 {           
             printf("端口%dABB表握手中,使用sendXmegaInTimeFrame发送\n", port);
           }
         }
         else
         {
           sendXmegaFrame(COPY_DATA, buf, length+3);
         }
       #endif
       break;

    #ifdef PLUG_IN_CARRIER_MODULE
     case PORT_POWER_CARRIER:
     	  if (carrierModuleType == MIA_CARRIER)
     	  {
     	  	printf("sendMeterFrame:发送前等待1s\n");
     	  	sleep(1);
     	  }
      	
      	write(fdOfCarrier, pack, length);
      	
      	carrierFlagSet.cmdTimeOut  = nextTime(sysTime, 0, CARRIER_TIME_OUT);  //超时时间
      	carrierFlagSet.sending     = 1;                                       //正在发送命令
      	carrierFlagSet.cmdContinue = 0;                                       //继续发送命令置0
      	carrierFlagSet.retryCmd    = 0;                                       //发送超时置0
      	
      	carrierFlagSet.numOfCarrierSending++;                                 //ly,2011-05-27,为适应弥亚微
      	if (debugInfo&PRINT_CARRIER_DEBUG)
      	{
      		 //printf("载波/无线端口发送计数=%d\n", carrierFlagSet.numOfCarrierSending);
      	}
      	
      	//大约1个多小时(不同的载波模块不一样)后如果载波无反应就要重启集中器
       #ifdef LM_SUPPORT_UT
        if (0x55!=lmProtocol)
        {
       #endif
      	  
      	  if (carrierFlagSet.numOfCarrierSending>100)
      	  {
      	    if (debugInfo&PRINT_CARRIER_DEBUG)
      	    {
      		    printf("载波/无线端口发送计数达到100次,无接收,复位集中器\n");
      	    }
      		 
      	  	ifReset = TRUE;
      	  }
      	  
       #ifdef LM_SUPPORT_UT
      	}
       #endif
      	break;
     #endif

      default:
      	return;
   }
}

/**************************************************
函数名称:threadOf485x1Received
功能描述:485接口1接收处理线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void *threadOf485x1Received(void *arg)
{
  INT8U recvLen;
  INT8U tmpBuf[52];
  INT8U i;
  INT8U tmpPort;
  
  while (1)
  {
    recvLen = read(fdOfttyS2,&tmpBuf,50);
     
    #ifdef RS485_1_USE_PORT_1
     tmpPort = 0;
    #else
     tmpPort = 1;
    #endif
     
     if (copyCtrl[tmpPort].pForwardData!=NULL)  //转发
     {
       if (copyCtrl[tmpPort].pForwardData->ifSend == TRUE)
       {
     	 if (debugInfo&METER_DEBUG)
     	 {
           printf("%02d-%02d-%02d %02d:%02d:%02d Rx1(转发):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
           for(i=0;i<recvLen;i++)
           {
    	     printf("%02x ",tmpBuf[i]);
           }
           printf("\n");
     	 }

     	 if (copyCtrl[tmpPort].pForwardData->fn==1)
     	 {
     	   //接收到回复数据后等待两秒,看是否有后续字节
     	   if(copyCtrl[tmpPort].pForwardData->receivedBytes==FALSE)
     	   {
     	   	 copyCtrl[tmpPort].pForwardData->receivedBytes = TRUE;
     	   	 copyCtrl[tmpPort].pForwardData->nextBytesTime = nextTime(sysTime, 0, 3);

       	     memcpy(copyCtrl[tmpPort].pForwardData->data, tmpBuf, recvLen);
      	     copyCtrl[tmpPort].pForwardData->length = recvLen;
      	     copyCtrl[tmpPort].pForwardData->forwardResult = RESULT_HAS_DATA;
     	   }
     	   else
     	   {
       	     memcpy(&copyCtrl[tmpPort].pForwardData->data[copyCtrl[tmpPort].pForwardData->length], tmpBuf, recvLen);
      	     copyCtrl[tmpPort].pForwardData->length += recvLen;
     	   }
     	 }
     	 else
     	 {
           forwardReceive(tmpPort, tmpBuf, recvLen);
     	 }
       }
       else
       {
         meter485Receive(tmpPort+1, tmpBuf, recvLen);
       }
     }
     else
     {
       #ifdef RS485_1_USE_PORT_1
        meter485Receive(PORT_NO_1, tmpBuf, recvLen);
       #else
        meter485Receive(PORT_NO_2, tmpBuf, recvLen);       
       #endif
     }
     
     usleep(50);
  }
}

/**************************************************
函数名称:threadOf485x2Received
功能描述:485接口2接收处理线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void *threadOf485x2Received(void *arg)
{
  INT8U  recvLen;
  INT8U  tmpBuf[52];
  INT8U  i;
  INT8U  tmpPort;
  
  while (1)
  {
    //如果软件支持级联,则第2路抄表口为ttyS4;
    //如果软件不持级联,则第2路抄表品为ttyS3
    #ifdef SUPPORT_CASCADE
     recvLen = read(fdOfttyS4, &tmpBuf, 50);
    #else
     recvLen = read(fdOfttyS3, &tmpBuf, 50);
    #endif

     #ifdef RS485_1_USE_PORT_1
      tmpPort=1;
     #else
      tmpPort=2;
     #endif

     if (copyCtrl[tmpPort].pForwardData!=NULL)  //转发
     {
       if (copyCtrl[tmpPort].pForwardData->ifSend == TRUE)
       {
     	 if (debugInfo&METER_DEBUG)
     	 {
           printf("%02d-%02d-%02d %02d:%02d:%02d Rx2(转发):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
           for(i=0;i<recvLen;i++)
           {
    	     printf("%02x ",tmpBuf[i]);
           }
           printf("\n");
     	 }
     	  
     	 if (copyCtrl[tmpPort].pForwardData->fn==1)
     	 {
     	   //接收到回复数据后等待两秒,看是否有后续字节
     	   if(copyCtrl[tmpPort].pForwardData->receivedBytes==FALSE)
     	   {
     	   	 copyCtrl[tmpPort].pForwardData->receivedBytes = TRUE;
     	   	 copyCtrl[tmpPort].pForwardData->nextBytesTime = nextTime(sysTime, 0, 3);

       	     memcpy(copyCtrl[tmpPort].pForwardData->data, tmpBuf, recvLen);
      	     copyCtrl[tmpPort].pForwardData->length = recvLen;
      	     copyCtrl[tmpPort].pForwardData->forwardResult = RESULT_HAS_DATA;
     	   }
     	   else
     	   {
       	     memcpy(&copyCtrl[tmpPort].pForwardData->data[copyCtrl[tmpPort].pForwardData->length], tmpBuf, recvLen);
      	     copyCtrl[tmpPort].pForwardData->length += recvLen;
     	   }
     	 }
     	 else
     	 {
           forwardReceive(tmpPort, tmpBuf, recvLen);
     	 }
       }
       else
       {
         meter485Receive(tmpPort+1, tmpBuf, recvLen);
       }
     }
     else
     {    
       #ifdef RS485_1_USE_PORT_1
        meter485Receive(PORT_NO_2, tmpBuf, recvLen);
       #else
        meter485Receive(PORT_NO_3, tmpBuf, recvLen);
       #endif       
     }
     
     usleep(50);
  }
}

/**************************************************
函数名称:queryMeterStoreInfo
功能描述:查询电表存储信息
调用函数:
被调用函数:
输入参数:INT8U portNum,...
输出参数:*meterInfo,第0字节 - 电表类型(1-单相智能表,2-单相本地费控表,3-单相远程费控表,4-三相智能表,5-三相本地费控表,6-三相远程费控表,7-485三相表(数据抄读最全),8-重点用户
                    第1字节 - 实时数据存储类型
                    第2字节 - 日冻结数据存储类型
                    第3字节 - 月冻结数据存储类型
                    第4字节 - 实时数据长度低8位
                    第5字节 - 实时数据长度高8位
返回值：状态
***************************************************/
void queryMeterStoreInfo(INT16U pn, INT8U *meterInfo)
{
  METER_DEVICE_CONFIG  meterConfig;
  INT8U                i;
  INT16U               tmpData;
 
  if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
  {
  	;
  }
   	
  *meterInfo = 0;
  if ((meterConfig.rateAndPort&0x1f)==31)  //载波端口
  {
    for(i=0;i<keyHouseHold.numOfHousehold;i++)
    {
  	  tmpData = keyHouseHold.household[i*2] | keyHouseHold.household[i*2+1];
  	 
  	  if (tmpData==meterConfig.measurePoint)
  	  {
  	 	  *meterInfo     = 8;
        *(meterInfo+1) = KEY_HOUSEHOLD_PRESENT;
        *(meterInfo+2) = KEY_HOUSEHOLD_DAY;
        *(meterInfo+3) = KEY_HOUSEHOLD_MONTH;
        *(meterInfo+4) = LENGTH_OF_KEY_ENERGY_RECORD&0xff;
        *(meterInfo+5) = LENGTH_OF_KEY_ENERGY_RECORD>>8;
        break;
  	  }
    }
  	 
  	if (*meterInfo==8)
  	{
  	 	return;
  	}
  	 	 	  	      
    switch(meterConfig.bigAndLittleType>>4)
    {
    	  case 3:   //三相一般工商业
         switch(meterConfig.bigAndLittleType&0xf)
         {
         	 case 1:
         	 	 *meterInfo = 5;
         	 	 *(meterInfo+1) = THREE_LOCAL_CTRL_PRESENT;
         	 	 *(meterInfo+2) = THREE_LOCAL_CTRL_DAY;
         	 	 *(meterInfo+3) = THREE_LOCAL_CTRL_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_THREE_LOCAL_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_THREE_LOCAL_RECORD>>8;
         	 	 break;

         	 case 2:
         	 	 *meterInfo = 6;
         	 	 *(meterInfo+1) = THREE_REMOTE_CTRL_PRESENT;
         	 	 *(meterInfo+2) = THREE_REMOTE_CTRL_DAY;
         	 	 *(meterInfo+3) = THREE_REMOTE_CTRL_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_THREE_REMOTE_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_THREE_REMOTE_RECORD>>8;
         	 	 break;

         	 default:
         	 	 *meterInfo = 4;
         	 	 *(meterInfo+1) = THREE_PRESENT;
         	 	 *(meterInfo+2) = THREE_DAY;
         	 	 *(meterInfo+3) = THREE_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_THREE_ENERGY_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_THREE_ENERGY_RECORD>>8;
         	 	 break;
         }
         break;

    	  case 4:   //单相一般工商业
         switch(meterConfig.bigAndLittleType&0xf)
         {
         	 case 0:
         	 	 *meterInfo = 2;
         	 	 *(meterInfo+1) = SINGLE_LOCAL_CTRL_PRESENT;
         	 	 *(meterInfo+2) = SINGLE_LOCAL_CTRL_DAY;
         	 	 *(meterInfo+3) = SINGLE_LOCAL_CTRL_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_SINGLE_LOCAL_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_SINGLE_LOCAL_RECORD>>8;
         	 	 break;

         	 case 1:
         	 	 *meterInfo = 3;
         	 	 *(meterInfo+1) = SINGLE_REMOTE_CTRL_PRESENT;
         	 	 *(meterInfo+2) = SINGLE_REMOTE_CTRL_DAY;
         	 	 *(meterInfo+3) = SINGLE_REMOTE_CTRL_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_SINGLE_REMOTE_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_SINGLE_REMOTE_RECORD>>8;
         	 	 break;

         	 default:
         	 	 *meterInfo = 1;
         	 	 *(meterInfo+1) = SINGLE_PHASE_PRESENT;
         	 	 *(meterInfo+2) = SINGLE_PHASE_DAY;
         	 	 *(meterInfo+3) = SINGLE_PHASE_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_SINGLE_ENERGY_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_SINGLE_ENERGY_RECORD>>8;
         	 	 break;
         }
         break;
         
       case 5:  //居民用户
         switch(meterConfig.bigAndLittleType&0xf)
         {
         	 case 0:
         	 case 3:
         	 	 *meterInfo = 2;
         	 	 *(meterInfo+1) = SINGLE_LOCAL_CTRL_PRESENT;
         	 	 *(meterInfo+2) = SINGLE_LOCAL_CTRL_DAY;
         	 	 *(meterInfo+3) = SINGLE_LOCAL_CTRL_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_SINGLE_LOCAL_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_SINGLE_LOCAL_RECORD>>8;          	 	 
         	 	 break;

         	 case 1:
         	 case 4:
         	 	 *meterInfo = 3;
         	 	 *(meterInfo+1) = SINGLE_REMOTE_CTRL_PRESENT;
         	 	 *(meterInfo+2) = SINGLE_REMOTE_CTRL_DAY;
         	 	 *(meterInfo+3) = SINGLE_REMOTE_CTRL_MONTH;                        	 	 
         	 	 *(meterInfo+4) = LENGTH_OF_SINGLE_REMOTE_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_SINGLE_REMOTE_RECORD>>8;
         	 	 break;

         	 default:
         	 	 *meterInfo = 1;
         	 	 *(meterInfo+1) = SINGLE_PHASE_PRESENT;
         	 	 *(meterInfo+2) = SINGLE_PHASE_DAY;
         	 	 *(meterInfo+3) = SINGLE_PHASE_MONTH;
         	 	 *(meterInfo+4) = LENGTH_OF_SINGLE_ENERGY_RECORD&0xff;
         	 	 *(meterInfo+5) = LENGTH_OF_SINGLE_ENERGY_RECORD>>8;
         	 	 break;
         }
       	break;
       	
       default:
         *meterInfo = 1;
         *(meterInfo+1) = SINGLE_PHASE_PRESENT;
         *(meterInfo+2) = SINGLE_PHASE_DAY;
         *(meterInfo+3) = SINGLE_PHASE_MONTH;
         *(meterInfo+4) = LENGTH_OF_SINGLE_ENERGY_RECORD&0xff;
         *(meterInfo+5) = LENGTH_OF_SINGLE_ENERGY_RECORD>>8;
       	break;
    }
  }
  else                                    //485端口
  {
    if ((meterConfig.bigAndLittleType&0xf)==1)//单相智能表
    {
      *meterInfo = 1;
      *(meterInfo+1) = SINGLE_PHASE_PRESENT;
      *(meterInfo+2) = SINGLE_PHASE_DAY;
      *(meterInfo+3) = SINGLE_PHASE_MONTH;
      *(meterInfo+4) = LENGTH_OF_SINGLE_ENERGY_RECORD&0xff;
      *(meterInfo+5) = LENGTH_OF_SINGLE_ENERGY_RECORD>>8;
    }
    else                                   //485三相表
    {
      *meterInfo = 7;
      *(meterInfo+1) = PRESENT_DATA;
      *(meterInfo+2) = DAY_BALANCE;
      *(meterInfo+3) = MONTH_BALANCE;
      *(meterInfo+4) = LENGTH_OF_ENERGY_RECORD&0xff;
      *(meterInfo+5) = LENGTH_OF_ENERGY_RECORD>>8;
    }
  }
}

#ifdef WDOG_USE_X_MEGA
/**************************************************
函数名称:xMegaCopyData
功能描述:485接口1接收处理线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void xMegaCopyData(INT8U port, INT8U *buf, INT16U len)
{
  INT16U i;
  
  #ifdef RS485_1_USE_PORT_1
   if (port==1 || port==2 || port==3)
   {
  	 port--;
   }
   else
   {
  	 return;
   }
  #endif
  
  if (copyCtrl[port].pForwardData!=NULL)  //转发
  {
  	 if (copyCtrl[port].pForwardData->ifSend == TRUE)
  	 {
  	   if (debugInfo&METER_DEBUG)
  	   {
         printf("%02d-%02d-%02d %02d:%02d:%02d 端口%d(转发):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port);
         for(i=0;i<len;i++)
         {
 	         printf("%02x ", buf[i]);
         }
         printf("\n");
  	   }
 
       //2012-08-20,add
       while(len>0)
       {
         if (*buf==0xfe)
         {
         	 buf++;
         	 len--;
         }
         else
         {
         	 break;
         }
       }
         
       if (len>0)
       {
    	   if (copyCtrl[port].pForwardData->fn==1)
    	   {
       	   copyCtrl[port].pForwardData->receivedBytes = TRUE;
  
         	 memcpy(copyCtrl[port].pForwardData->data, buf, len);
        	 copyCtrl[port].pForwardData->length = len;
        	 copyCtrl[port].pForwardData->forwardResult = RESULT_HAS_DATA;
  
    	     //因为xMega已经接收完整,所以不用再等待,可直接回复主站
    	   	 copyCtrl[port].pForwardData->nextBytesTime = sysTime;
    	   }
    	   else
    	   {
           forwardReceive(port, buf, len);
    	   }
    	 }
  	 }
  	 else
  	 {
       meter485Receive(port, buf, len);
  	 }
  }
  else
  {
    #ifdef RS485_1_USE_PORT_1
     meter485Receive(port+1, buf, len);
    #else
     meter485Receive(port, buf, len);
    #endif
  }
}
#endif


#ifdef LIGHTING

BOOL xlcForwardFrame(INT8U port, INT8U *addr, INT8U openClose);


/**************************************************
函数名称:happenRecovery485
功能描述:485故障发生/恢复
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：状态
***************************************************/
void happenRecovery485(INT8U type)
{  	 
  DATE_TIME              tmpTime;
  TERMINAL_STATIS_RECORD terminalStatisRecord;    //终端统计记录
  INT8U                  eventData[10];
 
  //2015-12-09,添加485故障恢复记录一次的处理
  tmpTime = timeHexToBcd(sysTime);
  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
  {
  	initTeStatisRecord(&terminalStatisRecord);
  }
  if (
	    (type==0 && (terminalStatisRecord.mixed&0x1)==0x01)
	     || (type==1 && (terminalStatisRecord.mixed&0x1)==0x00)
		 )
  {
		if (debugInfo&METER_DEBUG)
		{
			if (type==1)
			{
			  printf("%02d-%02d-%02d %02d:%02d:%02d,485通信故障发生,记录事件.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			}
			else
		  {
			  printf("%02d-%02d-%02d %02d:%02d:%02d,485通信恢复,记录事件.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			}
		}

 	  //记录终端故障 - 485抄表故障恢复
    if ((eventRecordConfig.iEvent[2] & 0x10) || (eventRecordConfig.nEvent[2] & 0x10))
    {
 	    eventData[0] = 0x15;
 	    eventData[1] = 0xa;
 	      
 	    //发生时间
 	    eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
	    eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
	    eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
	    eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
	    eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
	    eventData[7] = sysTime.year/10<<4   | sysTime.year%10;

	    //故障编码--485抄表故障恢复或是发生
	    eventData[8] = 0x4;    //恢复bit7=0
			if (type==1)
			{
				eventData[8] |= 0x80;    //发生bit7=1
			}
	     
	    eventData[9] = 0x0;

	    if (eventRecordConfig.iEvent[2] & 0x10)
	    {
	      writeEvent(eventData,10, 1, DATA_FROM_GPRS);
	    }

	    if (eventRecordConfig.nEvent[2] & 0x10)
	    {
	      writeEvent(eventData, 10, 2, DATA_FROM_LOCAL);
	    }
	        
	    eventStatus[0] = eventStatus[2] | 0x10;
	      
	    activeReport3();   //主动上报事件
	     
	    if (type==1)
			{
			  terminalStatisRecord.mixed |= 0x01;
			}
			else
			{
			  terminalStatisRecord.mixed &= 0xfe;
			}
	    tmpTime = timeHexToBcd(sysTime);
		
		  saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
	  }
  }
}

/**************************************************
函数名称:lcProcess
功能描述:光控处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：状态
***************************************************/
void lcProcess(INT8U *lastLux, INT32U nowLux)
{
	struct ctrlTimes  *tmpCTimes;
	struct ctrlTimes  *thisCTimes;
  INT8U             tmpLightMode=0;
  INT8U             tmpBright=0;
  INT8U             checkStep=0;    //对比控制时段步骤
  struct cpAddrLink *tmpCpLink;
  INT16U            tmpi;
  INT32U            tmpLux=0;
  INT32U            lastLuxValue = lastLux[0] | lastLux[1]<<8 | lastLux[2]<<16;        //上一次值
  INT32U            lastLastLuxValue = lastLux[3] | lastLux[4]<<8 | lastLux[5]<<16;    //上上次值
  INT32U            tmpData = 0;
  INT16U            openLux = 0xfffe;     //分闸流明值
  INT16U            closeLux = 0xfffe;    //合闸流明值
  DATE_TIME         startJudgeTime, endJudgeTime;    //2017-01-10,Add

  tmpCTimes  = cTimesHead;
  thisCTimes = NULL;
  while(tmpCTimes!=NULL)
  {
  	//光照度控制
  	if (7==tmpCTimes->deviceType)
  	{
 	  	if (
					//起始月和结束月不相同,2016-10-21
					(tmpCTimes->startMonth!=tmpCTimes->endMonth
						&&
						 (
							(
							 tmpCTimes->startMonth==sysTime.month        //与起始时间月相同的日期中
								&& tmpCTimes->startDay<=sysTime.day        // 且起始日小于等于当前日
							)
							 ||
								( 
								 tmpCTimes->endMonth==sysTime.month        //与结束时间月相同的日期中
									&& tmpCTimes->endDay>=sysTime.day        // 且结束日大于当前日
								)
						 )
					)
				 
					 ||    //2016-10-21,增加起始月和结束月相同的处理
						(
						 tmpCTimes->startMonth==tmpCTimes->endMonth    //起始月和结束月相同
							&& tmpCTimes->startMonth==sysTime.month      //且等于当前时间月
							 && tmpCTimes->startDay<=sysTime.day         //  起始日小于当前时间日
								&& tmpCTimes->endDay>=sysTime.day          //  结束日大于当前时间日
						)
 	  		   
 	  		     //结束月>起始月,在起始和结束月其间
 	  		     || (
 	  		         tmpCTimes->startMonth<tmpCTimes->endMonth
 	  		          && tmpCTimes->startMonth<sysTime.month
 	  		           && tmpCTimes->endMonth>sysTime.month
 	  		        )
 	  		    
 	  		      || (    //结束月<起始月
 	  		          tmpCTimes->startMonth>tmpCTimes->endMonth
 	  		           && (
 	  		               (
 	  		                sysTime.month<=12 
 	  		                 && tmpCTimes->startMonth<sysTime.month 
 	  		                  && tmpCTimes->endMonth<sysTime.month
 	  		               )
 	  		               ||
 	  		               (tmpCTimes->endMonth>sysTime.month)
 	  		              )
 	  		         )
 	  	   )
 	    {
			  thisCTimes = tmpCTimes;
 	    }
  	}
    
    tmpCTimes = tmpCTimes->next;        
  }

  if (thisCTimes!=NULL)
  {
    //2016-08-24,计算出当天是星期几
    tmpData = dayWeek(2000+sysTime.year, sysTime.month, sysTime.day);
	  if (debugInfo&METER_DEBUG)
    {
      printf("%02d-%02d-%02d %02d:%02d:%02d,今天是星期%d",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tmpData);
    }
    
    //再计算启用日比较因子
    if (0==tmpData)
    {
      tmpData = 6;
    }
    else
    {
      tmpData-=1;
    }
    tmpData = 1<<tmpData;
	  if (debugInfo& METER_DEBUG)
    {
      printf(",比较因子=%02X,设置的启用日=%02X,", tmpData, thisCTimes->workDay);
    }

	  //不在启用日
	  if ((thisCTimes->workDay & tmpData)!=tmpData)
    {
	    if (debugInfo&METER_DEBUG)
      {
        printf("在未启用日\n");
      }
        
      //2016-08-24,目前没有处理当天从启用日切换到未启用日或相反的操作
    }
    else
    {
	    if (debugInfo&METER_DEBUG)
      {
        printf("在启用日\n");
      }
    
      //分析是路灯模式还是隧道模式
      tmpLightMode = 0;    //0-路灯模式
      tmpLux = 0xffffff;
      tmpBright = 0xff;
   	  for(tmpi=0; tmpi<6; tmpi++)
  	  {
  	  	if (thisCTimes->bright[tmpi]<=100)    //该亮度值有效
  	  	{
  	      if (thisCTimes->bright[tmpi]!=0 && thisCTimes->bright[tmpi]!=1)
  	      {
  	        if (tmpBright!=0xff)
  	        {
  	      	  if (tmpLux<(thisCTimes->hour[tmpi] | thisCTimes->min[tmpi]<<8) && tmpBright<thisCTimes->bright[tmpi])
  	      	  {
  	      		  tmpLightMode = 1;    //1-隧道模式
  	      	  }
  	        }
  	      
  	        tmpLux = thisCTimes->hour[tmpi] | thisCTimes->min[tmpi]<<8;
  	        tmpBright = thisCTimes->bright[tmpi];
  	      }
  	  	}
  	  	else
  	  	{
  	  		break;
  	  	}
  	  }
  		if (debugInfo&METER_DEBUG)
      {
        printf(
               "%02d-%02d-%02d %02d:%02d:%02d,光控,在时间段(%d月%d日-%d月%d日)中",
                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
                 thisCTimes->startMonth, thisCTimes->startDay, thisCTimes->endMonth, thisCTimes->endDay
              );
      	if (tmpLightMode==1)
      	{
      		printf(",隧道模式\n");
      	}
      	else
      	{
      		printf(",路灯模式\n");
      	}
      }
      
      openLux = 0xfffe;
      closeLux = 0xfffe;
      tmpBright = 0xfe;
      for(tmpi=0; tmpi<6; tmpi++)
      {
      	if (thisCTimes->bright[tmpi]>100)
      	{
      		continue;
      	}
      	
      	//tmpLux = (thisCTimes->hour[tmpi] | thisCTimes->min[tmpi]<<8)*10;
      	//2016-08-24,流明值修改为以原始值下发,不用再乘以10,精度为1而不再是10
      	tmpLux = thisCTimes->hour[tmpi] | thisCTimes->min[tmpi]<<8;
      	
      	if (tmpLightMode==1)    //隧道模式
      	{
    			//已打开电源
      	  tmpCpLink = xlcLink;
          while(tmpCpLink!=NULL)
          {
    			  if (1==tmpCpLink->lcOnOff)
    			  {
    			    //三次采集都大于等于照度流明值
    			    if (tmpLux<=nowLux && tmpLux<=lastLuxValue && tmpLux<=lastLastLuxValue)
    			    {
    			  	  tmpBright = thisCTimes->bright[tmpi];
    			    }
    			  }
    			  else
    			  {
      			  tmpCpLink->lcOnOff = 1;
      			  tmpCpLink->lcDetectOnOff = 1;
      			  
      			  if (debugInfo&METER_DEBUG)
              {
      	        printf(
      	               "%02d-%02d-%02d %02d:%02d:%02d,光控,隧道模式,置线路控制点%02x%02x%02x%02x%02x%02x合闸标志\n",
      	                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
      	                  tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
      	              );
      	      }
      	    }
    			}
      	}
      	else    //路灯模式
      	{
      		//2016-08-24,将分闸照度值单独设置
      		if (thisCTimes->bright[tmpi]==0x00)
      		{
      		  openLux = tmpLux;    //分闸流明值
      		}
      		else
      		{	
      		  //2016-08-24,将合闸照度值单独设置
      		  if (thisCTimes->bright[tmpi]==0x01)
      		  {
        			closeLux = tmpLux;    //合闸流明值
        		}
        		else
        		{
              if (
              	  CTRL_MODE_LIGHT==ctrlMode              //设置的集中器控制模式为光控
              	   || CTRL_MODE_PERIOD_LIGHT==ctrlMode   //设置的集中器控制模式为时段控结合光控
              	    || CTRL_MODE_LA_LO_LIGHT==ctrlMode   //设置的集中器控制模式为经纬度结合光控
              	 )
              {
          			tmpCpLink = xlcLink;
                while(tmpCpLink!=NULL)
                {
          			  //已有打开线路控制点电源
          			  if (1==tmpCpLink->lcOnOff)
          			  {
          			    //两次采集都小于等于设定流明值
          			    if (tmpLux>=nowLux && tmpLux>=lastLuxValue)
          			    {
          			  	  tmpBright = thisCTimes->bright[tmpi];
          			  	  
          			  	  lCtrl.lcDetectBright = thisCTimes->bright[tmpi];
          			    }
          			  }
    
                  tmpCpLink = tmpCpLink->next;
                }
              }
        		}
          }
      	}
      }
      
      if (
			    openLux!=0xfffe 
					 && closeLux!=0xfffe
				 )
      {
				if (debugInfo&METER_DEBUG)
				{
					printf(
								 "%02d-%02d-%02d %02d:%02d:%02d,光控,合闸流明值=%dLux,分闸流明值=%dLux\n",
									sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
										closeLux, openLux
								);
				}
      
  			//光控分闸判断
  			if (
  				  //最近三次采集流明值都大于等于(分闸流明值-pnGate.lcWave)(-pnGate.lcWave避免照度震荡)
  				  (openLux<=(nowLux+pnGate.lcWave))
  				   && (openLux<=(lastLuxValue+pnGate.lcWave))
  				    && (openLux<=(lastLastLuxValue+pnGate.lcWave))
  				     
  				     && (                                             //2016-03-21,在测试中发现,当在关灯值正负震荡值之间时,会先置分闸再置合闸,这是不正确的,于是加如下条件:
  				         openLux>closeLux                             //条件1:分闸流明值大于合闸流明值
  				          ||
  				           (
  				            openLux<closeLux                          //条件2:分闸流明值小于合闸流明值(这种情况分合闸会有一部分流明值是重叠的)
  				             && 
  				               (
  				                nowLux>(closeLux+pnGate.lcWave)       //    2.1.当前流明值大于(合闸流明值+震荡值)
  				                || 
  				                  (
  				                   nowLux<(closeLux+pnGate.lcWave)    //    2.2 当前流明值小于(合闸流明值+震荡值)
  				                    && nowLux>lastLuxValue            //    2.2.1流明值越来越大,且一次比一次大
  				                     && lastLuxValue>lastLastLuxValue 
  				                  )
  				               )
  				           )
  				        )
  				 )
  			{
  			  //如果是线路电源打开或是未知,更新为关闭
  			  tmpCpLink = xlcLink;
          while(tmpCpLink!=NULL)
          {
  			    if (
  			    	  CTRL_MODE_LIGHT==tmpCpLink->bigAndLittleType
  			    	   || CTRL_MODE_PERIOD_LIGHT==tmpCpLink->bigAndLittleType
  			    	    || CTRL_MODE_LA_LO_LIGHT==tmpCpLink->bigAndLittleType
  			    	 )
  			    {
							if (
									tmpCpLink->lcOnOff>0
									 && tmpCpLink->lcProtectTime.year==0xff    //2017-01-09,Add
								 )
							{
								if (CTRL_MODE_LA_LO_LIGHT==tmpCpLink->bigAndLittleType)
								{
									startJudgeTime        = sysTime;
									startJudgeTime.hour   = tmpCpLink->duty[1];    //日出时刻-时
									startJudgeTime.minute = tmpCpLink->duty[0];    //日出时刻-分 
									startJudgeTime.second = 0;
									endJudgeTime          = startJudgeTime;
									startJudgeTime        = backTime(startJudgeTime, 0, 0, 0, beforeOnOff[2], 0);
									endJudgeTime          = nextTime(endJudgeTime, beforeOnOff[3], 0);

									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,光控,线路控制点%02x%02x%02x%02x%02x%02x经纬度结合光控,日出时刻前%02d-%02d-%02d %02d:%02d:%02d到日出时刻后%02d-%02d-%02d %02d:%02d:%02d,",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                             tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0],
														  startJudgeTime.year, startJudgeTime.month, startJudgeTime.day, startJudgeTime.hour, startJudgeTime.minute, startJudgeTime.second,
														   endJudgeTime.year,endJudgeTime.month,endJudgeTime.day,endJudgeTime.hour,endJudgeTime.minute,endJudgeTime.second
													);
									}

									if (
											compareTwoTime(startJudgeTime, sysTime)==TRUE      //在日出时刻(分闸)前多少分钟有效内
											 && compareTwoTime(sysTime, endJudgeTime)==TRUE    //在日出时刻(分闸)后多少分钟有效内
										 )
									{
										tmpCpLink->lcOnOff = 0;
										tmpCpLink->lcProtectTime = endJudgeTime;    //从当前时刻开始起(beforeOnOff[3])内不再执行光控命令
										
										if (debugInfo&METER_DEBUG)
										{
											printf("在此范围内,执行\n");
										}
									}
									else
									{
										if (debugInfo&METER_DEBUG)
										{
											printf("不在此范围内\n");
										}
									}
								}
								else
								{
									tmpCpLink->lcOnOff = 0;
									
									//从当前时刻开始起(beforeOnOff[3])内不再执行光控命令
									tmpCpLink->lcProtectTime = nextTime(sysTime, beforeOnOff[3], 0);
								}
						
								if (0==tmpCpLink->lcOnOff)
								{
									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,光控,路灯模式,置线路控制点%02x%02x%02x%02x%02x%02x分闸标志.分闸后保护至%02d-%02d-%02d %02d:%02d:%02d\n",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
														 tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0],
															tmpCpLink->lcProtectTime.year,tmpCpLink->lcProtectTime.month,tmpCpLink->lcProtectTime.day,tmpCpLink->lcProtectTime.hour,tmpCpLink->lcProtectTime.minute,tmpCpLink->lcProtectTime.second
													);
									}
								}
							}
						
							tmpCpLink->lcDetectOnOff = 0;
  			  	}
  			  	
            tmpCpLink = tmpCpLink->next;
          }
          
          lCtrl.lcDetectBright = 0x0;
  			}
  			
				//光控合闸判断
  			if (
  			    //最近三次采集都小于(合闸流明值+pnGate.lcWave)(+pnGate.lcWave避免照度震荡)
  				  (nowLux<(closeLux+pnGate.lcWave))
  				   && (lastLuxValue<(closeLux+pnGate.lcWave))
  				    && (lastLastLuxValue<(closeLux+pnGate.lcWave))
  				    
  				     && (                                          //2016-03-21,在测试中发现,当在关灯值正负震荡值之间时,会先置分闸再置合闸,这是不正确的,于是加如下条件:
  				         openLux>closeLux                          //条件1:分闸流明值大于合闸流明值
  				          ||
  				           (
  				            openLux<closeLux                       //条件2:分闸流明值小于合闸流明值(这种情况分合闸会有一部分流明值是重叠的)
  				             && 
  				              (
											 	 openLux>=(nowLux+pnGate.lcWave)     //    2.1 当前流明值小于等于(分闸流明值-震荡值)
  				                ||
  				                 (
  				                  openLux<(nowLux+pnGate.lcWave)   //    2.2 当前流明值大于(分闸流明值-震荡值)
  				                   && nowLux<lastLuxValue          //    2.2.1 流明值越来越小
  				                    && lastLuxValue<lastLastLuxValue
  				                 )
												)
  				           )
  				        )
  				 )
  			{
  			  //如果是线路电源关闭,更新为打开
  			  tmpCpLink = xlcLink;
          while(tmpCpLink!=NULL)
          {
  			    if (
  			    	  CTRL_MODE_LIGHT==tmpCpLink->bigAndLittleType
  			    	   || CTRL_MODE_PERIOD_LIGHT==tmpCpLink->bigAndLittleType
  			    	    || CTRL_MODE_LA_LO_LIGHT==tmpCpLink->bigAndLittleType
  			    	 )
  			    {
							if (
									1!=tmpCpLink->lcOnOff
									 && tmpCpLink->lcProtectTime.year==0xff    //2017-01-09,Add
								 )
							{
								if (CTRL_MODE_LA_LO_LIGHT==tmpCpLink->bigAndLittleType)
								{
									startJudgeTime        = sysTime;
									startJudgeTime.hour   = tmpCpLink->duty[3];    //日落时刻-时
									startJudgeTime.minute = tmpCpLink->duty[2];    //日落时刻-分 
									startJudgeTime.second = 0;
									endJudgeTime          = startJudgeTime;
									startJudgeTime        = backTime(startJudgeTime, 0, 0, 0, beforeOnOff[0], 0);
									endJudgeTime          = nextTime(endJudgeTime, beforeOnOff[1], 0);

									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,光控,线路控制点%02x%02x%02x%02x%02x%02x经纬度结合光控,日落时刻前%02d-%02d-%02d %02d:%02d:%02d到日落时刻后%02d-%02d-%02d %02d:%02d:%02d,",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                             tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0],
														  startJudgeTime.year, startJudgeTime.month, startJudgeTime.day, startJudgeTime.hour, startJudgeTime.minute, startJudgeTime.second,
														   endJudgeTime.year,endJudgeTime.month,endJudgeTime.day,endJudgeTime.hour,endJudgeTime.minute,endJudgeTime.second
													);
									}

									if (
											compareTwoTime(startJudgeTime, sysTime)==TRUE      //在日落时刻(分闸)前多少分钟有效内
											 && compareTwoTime(sysTime, endJudgeTime)==TRUE    //在日落时刻(分闸)后多少分钟有效内
										 )
									{
										tmpCpLink->lcOnOff = 1;
										tmpCpLink->lcProtectTime = endJudgeTime;    //从当前时刻开始起到endJudgeTime内不再执行光控命令
										
										if (debugInfo&METER_DEBUG)
										{
											printf("在此范围内,执行\n");
										}
									}
									else
									{
										if (debugInfo&METER_DEBUG)
										{
											printf("不在此范围内\n");
										}
									}
								}
								else
								{
									tmpCpLink->lcOnOff = 1;
									
									//从当前时刻开始起(beforeOnOff[1])内不再执行光控命令
									tmpCpLink->lcProtectTime = nextTime(sysTime, beforeOnOff[1], 0);
								}
								
								if (1==tmpCpLink->lcOnOff)
								{
									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,光控,路灯模式,置线路控制点%02x%02x%02x%02x%02x%02x合闸标志.合闸后保护至%02d-%02d-%02d %02d:%02d:%02d\n",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
														 tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0],
															tmpCpLink->lcProtectTime.year,tmpCpLink->lcProtectTime.month,tmpCpLink->lcProtectTime.day,tmpCpLink->lcProtectTime.hour,tmpCpLink->lcProtectTime.minute,tmpCpLink->lcProtectTime.second
													);
									}
								}
							}
							
							tmpCpLink->lcDetectOnOff = 1;
    			  }

            tmpCpLink = tmpCpLink->next;
          }
  			}
      }
      
			//2017-01-09,分合闸保护期后,恢复光控操作
			tmpCpLink = xlcLink;
			while(tmpCpLink!=NULL)
			{
				if (tmpCpLink->lcProtectTime.year!=0xff)
				{
					if (compareTwoTime(tmpCpLink->lcProtectTime, sysTime)==TRUE)
					{
						tmpCpLink->lcProtectTime.year=0xff;
						
						if (debugInfo&METER_DEBUG)
						{
							printf(
										 "%02d-%02d-%02d %02d:%02d:%02d,光控,解除线路控制点%02x%02x%02x%02x%02x%02x光控分合闸保护限制\n",
											sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
											 tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
										);
						}
					}
				}
				
				if (tmpCpLink->lcDetectOnOff != tmpCpLink->lcOnOff)
				{
					if (CTRL_MODE_LA_LO_LIGHT==tmpCpLink->bigAndLittleType)
					{
					  //lcOnOff=1,这种情况发生在日出时刻早于光控判断的分闸时刻
						if (1==tmpCpLink->lcOnOff)
						{
							startJudgeTime        = sysTime;
							startJudgeTime.hour   = tmpCpLink->duty[1];    //日出时刻-时
							startJudgeTime.minute = tmpCpLink->duty[0];    //日出时刻-分
							startJudgeTime.second = 0;
							startJudgeTime = nextTime(startJudgeTime, beforeOnOff[3], 0);
							endJudgeTime          = sysTime;
							endJudgeTime.hour     = tmpCpLink->duty[3];    //日落时刻-时
							endJudgeTime.minute   = tmpCpLink->duty[2];    //日落时刻-分
							endJudgeTime.second   = 0;
							endJudgeTime = backTime(endJudgeTime, 0, 0, 0, beforeOnOff[0], 0);							
							
							//如果日落时刻在日出时刻之后且当前时间在日落时刻之前,日出时刻向后退一天
							if (
							    compareTwoTime(endJudgeTime,startJudgeTime)==TRUE
							     && compareTwoTime(sysTime, startJudgeTime)==TRUE
								 )
							{
								startJudgeTime = backTime(startJudgeTime, 0, 1, 0, 0, 0);
							}
							
							if (
							    compareTwoTime(startJudgeTime, sysTime)==TRUE
									 && compareTwoTime(sysTime, endJudgeTime)==TRUE
								 )
							{
								tmpCpLink->lcOnOff = 0;
								
								if (debugInfo&METER_DEBUG)
								{
									printf(
												 "%02d-%02d-%02d %02d:%02d:%02d,光控,线路控制点%02x%02x%02x%02x%02x%02x恢复光控分合闸标志为分闸\n",
													sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
													 tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
												);
								}
							}
						}
						else
						{
							//lcOnOff=0,这种情况发生在日落时刻晚于光控判断的合闸时刻
							if (0==tmpCpLink->lcOnOff)
							{
								endJudgeTime        = sysTime;
								endJudgeTime.hour   = tmpCpLink->duty[3];    //日落时刻-时
								endJudgeTime.minute = tmpCpLink->duty[2];    //日落时刻-分
								endJudgeTime.second = 0;
								endJudgeTime = backTime(endJudgeTime, 0, 0, 0, beforeOnOff[0], 0);
								
								//如果日落时刻在日出时刻之后且当前时间在日落时刻之前,日出时刻向后退一天
								if (compareTwoTime(endJudgeTime, sysTime)==TRUE)
								{
									tmpCpLink->lcOnOff = 1;
									
									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,光控,线路控制点%02x%02x%02x%02x%02x%02x恢复光控分合闸标志为合闸\n",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
														 tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
													);
									}

								}
							}
						}
					}
				}
				
				tmpCpLink = tmpCpLink->next;
			}

      if (debugInfo&METER_DEBUG)
      {
        printf(
               "%02d-%02d-%02d %02d:%02d:%02d,光控,光控亮度应为=%d\n",
                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
                  tmpBright
              );
      }
      
      if (tmpBright<=100)
      {
        checkStep = 0;
   	    tmpCpLink = xlcLink;
        while(tmpCpLink!=NULL)
        {
    	    //亮度值需要调整,同时查看是否有的线路控制点电源已打开
          if (
          	  (1==tmpCpLink->status)
          	   &&(tmpBright!=lCtrl.bright)
          	 )
          {
            checkStep = 1;
            
            if (debugInfo&METER_DEBUG)
            {
              printf(
                     "%02d-%02d-%02d %02d:%02d:%02d,光控,亮度已改变\n",
                      sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second
                    );
    	      }
          
            break;
          }
          
    	    //查看是否有的线路控制点刚合闸
          if (1==tmpCpLink->gateOn)
          {
          	checkStep = 1;
          	tmpCpLink->gateOn = 0;
  
            if (debugInfo&METER_DEBUG)
            {
              printf(
                     "%02d-%02d-%02d %02d:%02d:%02d,光控,线路控制点%02x%02x%02x%02x%02x%02x刚合闸,广播亮度\n",
                      sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                       tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
                    );
    	      }
          	
          	break;
          }
  
          tmpCpLink = tmpCpLink->next;
        }
        
      	if (1==checkStep)
      	{
      	  lCtrl.bright = tmpBright;
      	  
      	  //延迟3秒后置广播命令标志
      	  sleep(3); 
      	  
      	  carrierFlagSet.broadCast = 4;    //光控 - 请求广播开关调
      	}
      }
    }
  }
}

#endif 

/**************************************************
函数名称:meter485Receive
功能描述:485表接收处理线程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：状态
***************************************************/
void meter485Receive(INT8U port, INT8U *buf, INT16U len)
{
 #ifdef LIGHTING 
  
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //一个控制器统计事件数据(与时间无关量)

  INT16U                     tmpi;
  INT8U                      checkSum;
  DATE_TIME                  tmpTime;
  INT8U                      eventData[15];
  INT16U                     tmpTail;
  struct ctrlTimes           *tmpCTimes;
  INT8U                      j;
  INT8U                      timesCount = 0;
  INT8U                      numOfTime;
  INT8U                      checkStep=0;    //对比控制时段步骤
  INT8U                      hourDataBuf[LEN_OF_LIGHTING_FREEZE];    //小时冻结数据缓存
  
  struct cpAddrLink          *tmpCpLink, *pFoundCpLink;
  struct cpAddrLink          *tmpNode, *tmpZbNode;
  
  INT32U                     tmpData;
  
  INT8U                      acParaData[LENGTH_OF_PARA_RECORD];
  INT8U                      visionBuff[LENGTH_OF_ENERGY_RECORD];
  
  INT32U                     tmpP = 0;
  INT32U                     tmpPrevP = 0;

 #endif
	
  //if ((port>0 && port<4) || port==PORT_POWER_CARRIER)
  if ((port>0 && port<5) || (port>=PORT_POWER_CARRIER && port<=PORT_POWER_CARRIER+2))    //ly,2012-01-09
  {
  	;
  }
  else
  {
  	return;
  }
  
  if (debugInfo&PRINT_645_FRAME && port!=31)
  {
    INT16U i;
    printf("%02d-%02d-%02d %02d:%02d:%02d Port %d Rx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, port);
   
    for(i=0;i<len;i++)
    {
      printf("%02x ",*(buf+i));
    }
    printf("\n");
  }
  
 #ifdef LIGHTING
  if (port>=31)
  {
    if (buf[0]==0x68 && buf[7]==0x68 && buf[len-1]==0x16)
    {
      checkSum = 0;
      for(tmpi=0; tmpi<len-2; tmpi++)
      {
    		checkSum += buf[tmpi];
        
        if (tmpi>9)
        {
          buf[tmpi] -= 0x33;    //按字节进行对数据域减0x33处理
        }
      }
    	
      if (checkSum==buf[len-2])
      {
    		switch (buf[8])
    		{
    	  	case 0x88:
    				copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_CHECK_TIME | REQUEST_STATUS);    //清除校时标志,并重新读取实时数据包
    				
						if (debugInfo&PRINT_CARRIER_DEBUG)
    				{
              printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)校时确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
            }
    				break;
    				
    	  	case 0x91:
    				if (buf[13]==0x00 && buf[12]==0x90 && buf[10]<0x08)
    				{
    		  		switch(buf[11])
    		  		{
    						case 0x00:    //00900000,单灯控制器实时状态和时间数据
            	  	if (pDotCopy!=NULL)
            	  	{
            	    	if (pDotCopy->dotCopying==TRUE)
            	    	{
                      memcpy(pDotCopy->data, &buf[14], 8);

                      pDotCopy->dotRetry  = 0;
            	      	pDotCopy->dotResult = RESULT_HAS_DATA;
            	    		
            	      	pDotCopy->dotRecvItem++;
            	    		
            	      	copyCtrl[4].copyContinue = TRUE;
            	    	}
                  }
                  else
                  {
                  	//68 31 00 c1 00 00 00 00 00 00 06 02 00 01 00 02 1e 68 05 00 00 03 14 20 68 91 12 33 33 c3 33 31 33 33 33 33 33 33 33 8a 6c 48 5c 43 47 c5 16 8a 16
          					//00900000-1.提取SLC时间
          					tmpTime.second = buf[22];
          					tmpTime.minute = buf[23];
          					tmpTime.hour   = buf[24];
          					tmpTime.day    = buf[25];
          					tmpTime.month  = buf[26];
          					tmpTime.year   = buf[27];
          					tmpTime = timeBcdToHex(tmpTime);
      
          					if (copyCtrl[4].tmpCpLink!=NULL) 
          					{
          			  		//状态已更新,2015-10-26,Add
          			  		if (buf[21]!=copyCtrl[4].tmpCpLink->status)
          			  		{
          							copyCtrl[4].tmpCpLink->statusUpdated |= 1;
          			  		}
          					  
          			  		copyCtrl[4].tmpCpLink->status = buf[21];         //SLC当前状态
          			  		copyCtrl[4].tmpCpLink->statusTime = tmpTime;     //SLC当前时间 
          			  		copyCtrl[4].tmpCpLink->lddtStatusTime = sysTime; //采集到LDDT状态时集中器的时间,2016-02-03
          					  
                      
                      //搜索LDDT需要置位的标志
                      if (copyCtrl[4].tmpCpLink->lddtRetry>0 && copyCtrl[4].tmpCpLink->lddtRetry<pnGate.lddtRetry)
                      {
                      	copyCtrl[4].copyContinue = TRUE;
                      }
                      copyCtrl[4].tmpCpLink->lddtRetry = 99;

                      //搜索离线单灯控制器需要置位的标志
                      if (copyCtrl[4].tmpCpLink->offLineRetry>0 && copyCtrl[4].tmpCpLink->offLineRetry<pnGate.offLineRetry)
                      {
                      	copyCtrl[4].copyContinue = TRUE;
                      }
                      copyCtrl[4].tmpCpLink->offLineRetry = 99;
                      
                      //曾经单灯控制点判断为离线,记录恢复事件
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[4].tmpCpLink->mp, 3); //单灯控制点与时间无关量
                      	
                      //已经发生过且记录过离线,记录恢复事件
                  	  if (meterStatisExtranTimeS.mixed&0x08)
                  	  {
                    		//记录控制点故障信息(ERC13)
                    		eventData[0] = 13;
                    		eventData[1] = 11;
                    	  
                    		tmpTime = timeHexToBcd(sysTime);
                    		eventData[2] = tmpTime.second;
                    		eventData[3] = tmpTime.minute;
                    		eventData[4] = tmpTime.hour;
                    		eventData[5] = tmpTime.day;
                    		eventData[6] = tmpTime.month;
                    		eventData[7] = tmpTime.year;
                    	  
                    		eventData[8] = copyCtrl[4].tmpCpLink->mp&0xff;
                    		eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff) | 0x00;    //恢复
                    		eventData[10] = 0x40;    //单灯控制点离线
                    	  
                    		writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                    	  
                    		if (eventRecordConfig.nEvent[1]&0x10)
                    		{
                    	  	writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                    		}
                    	  
                    		eventStatus[1] = eventStatus[1] | 0x10;
            					  
            						if (debugInfo&METER_DEBUG)
            						{
            			  			printf("%02d-%02d-%02d %02d:%02d:%02d,记录单灯控制点%d(%02x%02x%02x%02x%02x%02x)离线恢复事件\n", 
            				  	 					 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
            					 							copyCtrl[4].tmpCpLink->mp,
            					 							 copyCtrl[4].tmpCpLink->addr[5],copyCtrl[4].tmpCpLink->addr[4],copyCtrl[4].tmpCpLink->addr[3],copyCtrl[4].tmpCpLink->addr[2],copyCtrl[4].tmpCpLink->addr[1],copyCtrl[4].tmpCpLink->addr[0]
            										);
            						}
                        
                        activeReport3();    //主动上报事件
                        
                  			meterStatisExtranTimeS.mixed &= 0xf7;
                        
                        //存储控制点统计数据(与时间无关量)
                        saveMeterData(copyCtrl[4].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                      }
                      
                      //查询线路刚上电的单灯控制器状态时置位,2015-11-12
                      if (copyCtrl[4].tmpCpLink->lineOn>0 && copyCtrl[4].tmpCpLink->lineOn<5)
                      {
                      	copyCtrl[4].tmpCpLink->lineOn = 5;
                      	copyCtrl[4].copyContinue = TRUE;
                      }
                    }
      
          					if (debugInfo&PRINT_CARRIER_DEBUG)
          					{
                      printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)实时状态时间数据:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                      printf("  -SLC当前灯亮度:%d%%\n", buf[21]);
                      printf("  -SLC当前时间:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
                    }
          					
                    //1-1.判断SLC是否时间超差
                    if (!((timeCompare(tmpTime, sysTime, pnGate.checkTimeGate) == TRUE) 
             	        || (timeCompare(sysTime, tmpTime, pnGate.checkTimeGate) == TRUE)))
                    {
          			  		if (debugInfo&PRINT_CARRIER_DEBUG)
          			  		{
          							printf("  --SLC时间超差\n");
          			  		}
          						
          			  		if (copyCtrl[4].tmpCpLink!=NULL)
          			  		{
          							//请求校时
          							copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CHECK_TIME;
          			  		}
          					}
          					
          					//00900000-2.提取主站控制截止时间
          					tmpTime.second = buf[15];
          					tmpTime.minute = buf[16];
          					tmpTime.hour   = buf[17];
          					tmpTime.day    = buf[18];
          					tmpTime.month  = buf[19];
          					tmpTime.year   = buf[20];
          					tmpTime = timeBcdToHex(tmpTime);
      
          					if (copyCtrl[4].tmpCpLink!=NULL)
          					{
          			  		copyCtrl[4].tmpCpLink->msCtrlCmd = buf[14];     //主站直接控制命令
                      copyCtrl[4].tmpCpLink->msCtrlTime = tmpTime;    //主站控制截止时间
                    }
      
          					if (debugInfo&PRINT_CARRIER_DEBUG)
          					{
                      if (buf[14]>100)
                      {
                        printf("  -无主站控制命令");
                      }
                      else
                      {
                        printf("  -主站控制命令,亮度=%d%%", buf[14]);
                      }
                      printf(",截止时间:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
          					}
          					
          					if (copyCtrl[4].tmpCpLink!=NULL)
        						{
        			  			if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_CHECK_TIME)
        			  			{
        								;
        			  			}
        			  			else
        			  			{
        								copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_STATUS;
        			  			}
        						}
                    
                    //曾经报警控制点判断为线路异常,记录恢复事件
                    pFoundCpLink = ldgmLink;
                    while(pFoundCpLink!=NULL)
                    {
                      if(
                      	 (compareTwoAddr(pFoundCpLink->lddt1st, copyCtrl[4].tmpCpLink->addr, 0)==TRUE)
                      	  || (compareTwoAddr(pFoundCpLink->collectorAddr, copyCtrl[4].tmpCpLink->addr, 0)==TRUE)
                      	)
                      {
                        searchMpStatis(sysTime, &meterStatisExtranTimeS, pFoundCpLink->mp, 3); //报警控制器与时间无关量
                        	
                        //已经发生过且记录过异常,记录恢复事件
                    		if (
                    				(meterStatisExtranTimeS.mixed&0x02)       //无电发生过
                    		 		 || (meterStatisExtranTimeS.mixed&0x04)    //有电发生过
                    	   	 )
                    		{
                      	  //记录控制点故障信息(ERC13)
                      	  eventData[0] = 13;
                      	  eventData[1] = 11;
                      	  
                      	  tmpTime = timeHexToBcd(sysTime);
                      	  eventData[2] = tmpTime.second;
                      	  eventData[3] = tmpTime.minute;
                      	  eventData[4] = tmpTime.hour;
                      	  eventData[5] = tmpTime.day;
                      	  eventData[6] = tmpTime.month;
                      	  eventData[7] = tmpTime.year;
                      	  
                      	  eventData[8] = pFoundCpLink->mp&0xff;
                      	  eventData[9] = (pFoundCpLink->mp>>8&0xff) | 0x00;    //恢复
                      	  eventData[10] = 0x40;    //报警控制点异常,有电状态恢复
                      	  
                      	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                      	  
                      	  if (eventRecordConfig.nEvent[1]&0x10)
                      	  {
                      	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                      	  }
                      	  
                      	  eventStatus[1] = eventStatus[1] | 0x10;
              					  
              			  		if (debugInfo&METER_DEBUG)
              			  		{
              							printf("%02d-%02d-%02d %02d:%02d:%02d,记录报警控制点%d(%02x%02x%02x%02x%02x%02x)末端单灯控制器(%02x%02x%02x%02x%02x%02x)异常恢复事件\n", 
              					  	 			 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
              					   					pFoundCpLink->mp,
              					   					 pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
              					   						buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]
              					  				);
              			  		}
                          
                          activeReport3();    //主动上报事件
                          
                    	  	if (meterStatisExtranTimeS.mixed&0x02)
                    	  	{
                    				meterStatisExtranTimeS.mixed &= 0xfd;
                    	  	}
                    	  	if (meterStatisExtranTimeS.mixed&0x04)
                    	  	{
                    				meterStatisExtranTimeS.mixed &= 0xfb;
                    	  	}
                          
                          //存储控制点统计数据(与时间无关量)
                          saveMeterData(pFoundCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                        }
                      }
                      
                      pFoundCpLink = pFoundCpLink->next;
                    }

        						copyCtrl[4].tmpCpLink->copySuccess = TRUE; 
                  }
    			  			break;
									
    						case 0x02:    //单灯控制器软件版本
				  				copyCtrl[4].tmpCpLink->softVer = buf[17]-0x30;
          		  	if (debugInfo&PRINT_CARRIER_DEBUG)
          		 	 	{
                    printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)软件版本:%d\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1],copyCtrl[4].tmpCpLink->softVer);
                  }
				  				copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SOFT_VER;
				  				break;
    				    
    						case 0x03:    //单灯控制器开关调时刻数据
          		  	if (debugInfo&PRINT_CARRIER_DEBUG)
          		  	{
                    printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)开关调数据:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                  }

        		  		tmpTail = 14;
        					
        		  		//00900300-1,判断SLC发现灯故障后重试次数是否与主站设置值一致
        		  		if (buf[tmpTail++]!=pnGate.failureRetry)
        		  		{
        						if (copyCtrl[4].tmpCpLink!=NULL)
        						{
        			  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
        						}
        					  	
        						if (debugInfo&PRINT_CARRIER_DEBUG)
        						{
        			  			printf("  --单灯控制器发现灯故障后的重试次数与与集中器不一致\n");
        						}
        		  		}
        					
        		  		//00900300-2,PWM频率及亮度与占空比对比值
        		  		if (copyCtrl[4].tmpCpLink==NULL)
        		  		{
        						tmpTail += 14;
        		  		}
        		  		else
        		  		{
        						//00900300-2.1,PWM频率
        						if (buf[tmpTail++]!=copyCtrl[4].tmpCpLink->bigAndLittleType)
        						{
        			  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
        			  		  
        			  			if (debugInfo&PRINT_CARRIER_DEBUG)
        			  			{
        								printf("  --单灯控制器PWM频率与与集中器不一致\n");
        			  			}
        						}
        					  
        						//00900300-2.1,启动电流
        						if (1==copyCtrl[4].tmpCpLink->funOfMeasure)
        						{
        			  			if (buf[tmpTail++]!=copyCtrl[4].tmpCpLink->startCurrent)
        			  			{
        								copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
        						  
        								if (debugInfo&PRINT_CARRIER_DEBUG)
        								{
        				  				printf("  --单灯控制器启动电流与与集中器不一致\n");
        								}
        			  			}
        						}
        						else
        						{
        			  			tmpTail++;
        						}
        					  
		        				//00900300-2.2亮度与占空比对比值
		        				for(tmpi=0; tmpi<6; tmpi++)
		        				{
		        			  	if (buf[tmpTail+tmpi]!=copyCtrl[4].tmpCpLink->collectorAddr[tmpi])
		        			  	{
		        						copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
		        						  
		        						if (debugInfo&PRINT_CARRIER_DEBUG)
		        						{
		        				  		printf("  --单灯控制器亮度值与与集中器不一致\n");
		        						}
		        					  		
		        						break;
		        			  	}
		        				}
		        				tmpTail+=6;
        					  
		        				for(tmpi=0; tmpi<6; tmpi++)
		        				{
		        			  	if (buf[tmpTail+tmpi]!=copyCtrl[4].tmpCpLink->duty[tmpi])
		        			  	{
		        						copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
		        						  
		        						if (debugInfo&PRINT_CARRIER_DEBUG)
		        						{
		        				  		printf("  --单灯控制器占空比与与集中器不一致\n");
		        						}
		        					  		
		        						break;
		        			  	}
		        				}
		        				tmpTail+=6;
		        		  }
        				  
		        		  //00900300-3,SLC检测到的上一次灯故障发生时间
		        		  tmpTime.year = buf[tmpTail+5];
		        		  tmpTime.month = buf[tmpTail+4];
		        		  tmpTime.day = buf[tmpTail+3];
		        		  tmpTime.hour = buf[tmpTail+2];
		        		  tmpTime.minute = buf[tmpTail+1];
		        		  tmpTime.second = buf[tmpTail];
		        		  tmpTail+=6;
		        		  if (debugInfo&PRINT_CARRIER_DEBUG)
		        		  {
		        				printf("  -SLC上一次灯故障发生时间:%02x-%02x-%02x %02x:%02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
		        		  }
                  
				  				//控制器与时间无关量
			      			searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[4].tmpCpLink->mp, 3);
									
                  if (tmpTime.year!=0xff && tmpTime.month!=0xff && tmpTime.day!=0xff && tmpTime.minute!=0xff && tmpTime.second!=0xff)
                  {
                    if (tmpTime.year!=meterStatisExtranTimeS.lastFailure.year
                    	  || tmpTime.month!=meterStatisExtranTimeS.lastFailure.month
                    	   || tmpTime.day!=meterStatisExtranTimeS.lastFailure.day
                    	    || tmpTime.hour!=meterStatisExtranTimeS.lastFailure.hour
                    	     || tmpTime.minute!=meterStatisExtranTimeS.lastFailure.minute
                    	      || tmpTime.second!=meterStatisExtranTimeS.lastFailure.second
                    	 )
                    {
                      meterStatisExtranTimeS.lastFailure = tmpTime;
                    	
                      //存储控制点统计数据(与时间无关量)
                      saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                      
                      //记录控制器故障信息(ERC13)
                  	  eventData[0] = 13;
                  	  eventData[1] = 11;
                  	
                  	  eventData[2] = tmpTime.second;
                  	  eventData[3] = tmpTime.minute;
                  	  eventData[4] = tmpTime.hour;
                  	  eventData[5] = tmpTime.day;
                  	  eventData[6] = tmpTime.month;
                  	  eventData[7] = tmpTime.year;
                  	  
                  	  eventData[8] = copyCtrl[4].tmpCpLink->mp&0xff;
                  	  eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff) | 0x80;
                  	  eventData[10] = 0x20;
                  	  
                  	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                  	  
                  	  if (eventRecordConfig.nEvent[1]&0x10)
                  	  {
                  	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                  	  }
                  	  
                  	  eventStatus[1] = eventStatus[1] | 0x10;
          					  
          			  		if (debugInfo&PRINT_CARRIER_DEBUG)
          			  		{
          							printf("  --记录控制点%d故障事件\n", copyCtrl[4].tmpCpLink->mp);
          			  		}
                      
                      activeReport3();                  //主动上报事件
                    }
                  }
                  
                  //00900300-4,最后一次开灯、调光及关灯时刻
								  //00900300-4.1最后一次开灯时刻
								  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
									printf("  -SLC上一次开灯时刻:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 1, tmpTime, (INT8U *)&tmpTime, 6);

								  //00900300-4.2最后一次光调小时刻
								  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
										printf("  -SLC上一次光调小时刻:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 2, tmpTime, (INT8U *)&tmpTime, 6);
                  
								  //00900300-4.3最后一次光调正常时刻
							 	  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
								 		printf("  -SLC上一次光调正常时刻:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 3, tmpTime, (INT8U *)&tmpTime, 6);

								  //00900300-4.4最后一次关灯正常时刻
								  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
										printf("  -SLC上一次关灯时刻:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 4, tmpTime, (INT8U *)&tmpTime, 6);

								  if (buf[9]>=0x36)
								  {
										//00900300-5,SLC检测到的上一次倾斜发生时间
										tmpTime.year = buf[tmpTail+5];
										tmpTime.month = buf[tmpTail+4];
										tmpTime.day = buf[tmpTail+3];
										tmpTime.hour = buf[tmpTail+2];
										tmpTime.minute = buf[tmpTail+1];
										tmpTime.second = buf[tmpTail];
										tmpTail+=6;
										if (debugInfo&PRINT_CARRIER_DEBUG)
										{
										  printf("  -SLC上一次倾斜发生时间:%02x-%02x-%02x %02x:%02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
										}

										if (tmpTime.year!=0xff && tmpTime.month!=0xff && tmpTime.day!=0xff && tmpTime.hour!=0xff && tmpTime.minute!=0xff)
										{
										  if (tmpTime.year!=meterStatisExtranTimeS.lastDip.year
												|| tmpTime.month!=meterStatisExtranTimeS.lastDip.month
												 || tmpTime.day!=meterStatisExtranTimeS.lastDip.day
													|| tmpTime.hour!=meterStatisExtranTimeS.lastDip.hour
													 || tmpTime.minute!=meterStatisExtranTimeS.lastDip.minute
														|| tmpTime.second!=meterStatisExtranTimeS.lastDip.second
											 )
										  {
												meterStatisExtranTimeS.lastDip = tmpTime;
												
												//存储控制点统计数据(与时间无关量)
												saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
												
												//记录控制器故障信息(ERC13)
												eventData[0] = 13;
												eventData[1] = 11;
											
												eventData[2] = tmpTime.second;
												eventData[3] = tmpTime.minute;
												eventData[4] = tmpTime.hour;
												eventData[5] = tmpTime.day;
												eventData[6] = tmpTime.month;
												eventData[7] = tmpTime.year;
												
												eventData[8] = copyCtrl[4].tmpCpLink->mp&0xff;
												eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff) | 0x80;
												eventData[10] = 0x80;
												
												writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
												
												if (eventRecordConfig.nEvent[1]&0x10)
												{
													writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
												}
												
												eventStatus[1] = eventStatus[1] | 0x10;
												
												if (debugInfo&PRINT_CARRIER_DEBUG)
												{
													printf("  --记录控制点%d倾斜事件\n", copyCtrl[4].tmpCpLink->mp);
												}
												
												activeReport3();                  //主动上报事件
										  }
										}
										
										//漏电流
										tmpData = buf[tmpTail] | buf[tmpTail+1]<<8 | buf[tmpTail+2]<<16;
										if (debugInfo&PRINT_CARRIER_DEBUG)
										{
											printf("  -SLC当前漏电流:%dmA\n", tmpData);
										}

										//2019-01-22,添加,有设置电流越限需要记录才记录漏电流事件
										if (eventRecordConfig.iEvent[3]&0x01)
										{
											if (tmpData>pnGate.leakCurrent)
											{
											  if ((meterStatisExtranTimeS.mixed&0x10)==0x00)
											  {
													meterStatisExtranTimeS.mixed |= 0x10;
													
													//存储控制点统计数据(与时间无关量)
													saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
													
													//记录控制器故障信息(ERC13)
													eventData[0] = 13;
													eventData[1] = 11;
												
													eventData[2] = tmpTime.second;
													eventData[3] = tmpTime.minute;
													eventData[4] = tmpTime.hour;
													eventData[5] = tmpTime.day;
													eventData[6] = tmpTime.month;
													eventData[7] = tmpTime.year;
													
													eventData[8] = copyCtrl[4].tmpCpLink->mp&0xff;
													eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff) | 0x80;
													eventData[10] = 0x10;    //漏电流超阈值
													
													writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
													
													if (eventRecordConfig.nEvent[1]&0x10)
													{
														writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
													}
													
													eventStatus[1] = eventStatus[1] | 0x10;
													
													if (debugInfo&PRINT_CARRIER_DEBUG)
													{
														printf("  --记录控制点%d漏电流超阈值事件发生\n", copyCtrl[4].tmpCpLink->mp);
													}
													
													activeReport3();                  //主动上报事件
											  }
											}
											else
											{
											  if ((meterStatisExtranTimeS.mixed&0x10)==0x10)
											  {
													meterStatisExtranTimeS.mixed &= 0xef;
													
													//存储控制点统计数据(与时间无关量)
													saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
													
													//记录控制器故障信息(ERC13)
													eventData[0] = 13;
													eventData[1] = 11;
												
													eventData[2] = tmpTime.second;
													eventData[3] = tmpTime.minute;
													eventData[4] = tmpTime.hour;
													eventData[5] = tmpTime.day;
													eventData[6] = tmpTime.month;
													eventData[7] = tmpTime.year;
													
													eventData[8] = copyCtrl[4].tmpCpLink->mp&0xff;
													eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff);    //恢复
													eventData[10] = 0x10;    //漏电流超阈值
													
													writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
													
													if (eventRecordConfig.nEvent[1]&0x10)
													{
														writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
													}
													
													eventStatus[1] = eventStatus[1] | 0x10;
													
													if (debugInfo&PRINT_CARRIER_DEBUG)
													{
														printf("  --记录控制点%d漏电流超阈值事件恢复\n", copyCtrl[4].tmpCpLink->mp);
													}
													
													activeReport3();                  //主动上报事件
											  }
											}
										}
									}
									
        		  		if (copyCtrl[4].tmpCpLink!=NULL)
        		  		{
        						if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_ADJ_PARA)
        						{
					  					//2016-12-05,Add
					  					if (0x20==(meterStatisExtranTimeS.mixed&0x20))
					  					{
												//清除时段已同步标志
				        				meterStatisExtranTimeS.mixed &= 0xdf;
												saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
					  					}
        						}
        						else
        						{
        			  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_FREEZE_TIMES;
												
										  //存储控制点统计数据(与时间无关量)
										  meterStatisExtranTimeS.mixed |= 0x20;    //标记控制时段等参数已同步到单灯控制控制器
										  saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
					        	}
        		  		}
    			  			break;

    						case 0x04:    //单灯控制器控制时段小包数据
        		  		//68 05 00 00 03 14 20 68 91 2b 33 34 c3 33 35 c8 34 51 39 3c 63 97 43 35 33 44 38 97 45 43 33 47 53 97 49 68 33 fd 34 52 37 3b 73 97 44 35 33 45 43 97 46 43 33 2a 16 
          		  	if (debugInfo&PRINT_CARRIER_DEBUG)
          		  	{
                    printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)控制时段数据小包%d.\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1],buf[10]);
                  }

        		  		//00900400-1,第2小包时判断SLC时段与集中器是否一致
  				  			tmpTail = 15;
  				  			timesCount = 0;
    			  			tmpCTimes = cTimesHead;
    			  			while(tmpCTimes)
    			  			{
    								if (1==tmpCTimes->deviceType    //单灯控制器
    				    				&& tmpCTimes->noOfTime==(copyCtrl[4].tmpCpLink->ctrlTime&0x0f)
    				   				 )
    								{
    				  				timesCount++;
    								}
    								tmpCTimes = tmpCTimes->next;
    			  			}
							
				  				if (debugInfo&PRINT_CARRIER_DEBUG)
				  				{
										printf("  --(小包)集中器上控制时间段数=%d,集中器已上传的时间段数量=%d。\n",timesCount, buf[14]);
				  				}

				  				if (buf[14]<buf[10]*3)
				  				{
    								if (timesCount!=buf[14])
    								{
                  	  if (copyCtrl[4].tmpCpLink!=NULL)
                  	  {
                  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
												
												//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												{
              			  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
													
						  						if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
						 				 			{
														copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
						  						}
					    					}
                  	  }
                  					  	
                  	  if (debugInfo&PRINT_CARRIER_DEBUG)
                  	  {
                  			printf("  --(小包)单灯控制器控制时间段数量与集中器不等。\n");
                  	  }
            				}
            	  	}

				  				//buf[14]是上送的该单灯控制器上保存的有效的时间段个数
				  				checkStep = 0;
				  				for(tmpi=(buf[10]-1)*3; tmpi<buf[14]; tmpi++)
				  				{
										if (checkStep<2)
										{
										  checkStep = 0;
										  tmpCTimes = cTimesHead;
										  while(tmpCTimes)
										  {
												if (1==tmpCTimes->deviceType    //单灯控制器
												    && tmpCTimes->noOfTime==(copyCtrl[4].tmpCpLink->ctrlTime&0x0f)
												     && tmpCTimes->startMonth==(buf[tmpTail]&0x0f) && tmpCTimes->startDay==buf[tmpTail+1]
												      && tmpCTimes->endMonth==(buf[tmpTail]>>4&0x0f) && tmpCTimes->endDay==buf[tmpTail+2]
												   )
												{
												  checkStep = 1;
											  	
												  //2016-11-14,软件版本为2.0以上的单灯控制器添加启用日判断
												  if (copyCtrl[4].tmpCpLink->softVer>=2)
												  {
												  	//2016-8-19,添加启用日判断
												  	if (tmpCTimes->workDay!=buf[tmpTail+3])
												  	{
												  	  checkStep = 4;
												  		
						        					copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;

														  //2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
														  if (timesCount>6)
														  {
																copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
															
																if (timesCount>13)
																{
															  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
																}
														  }

    						  						if (debugInfo&METER_DEBUG)
    						  						{
    						    						printf("  --单灯控制点控制时间段%d启用日与集中器不一致。\n", tmpi+1);
    						  						}
						  							}
						  							else
						  							{
							  							numOfTime = buf[tmpTail+4];
														  for(j=0; j<numOfTime; j++)
														  {
																if (buf[tmpTail+5+j*3]!=tmpCTimes->hour[j] 
																		|| buf[tmpTail+5+j*3+1]!=tmpCTimes->min[j]
																	 		|| buf[tmpTail+5+j*3+2]!=tmpCTimes->bright[j]
															   	 )
																{
															  	if (copyCtrl[4].tmpCpLink!=NULL)
															  	{
																		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
																																
																		//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
																		if (timesCount>6)
																		{
																  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
																		
																  		if (timesCount>13)
																  		{
																				copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
																  		}
																		}
															  	}
									
																  if (debugInfo&PRINT_CARRIER_DEBUG)
																  {
																		printf("  --(小包)单灯控制器控制时间段%d时段%d与集中器不一致。\n", tmpi+1, j+1);
																  }
																  checkStep = 2;
																  break;
																}
							  							}
						    						}
						  						}
						  						else
						  						{
														numOfTime = buf[tmpTail+3];
														for(j=0; j<numOfTime; j++)
														{
														  if (buf[tmpTail+4+j*3]!=tmpCTimes->hour[j] 
														  	  || buf[tmpTail+4+j*3+1]!=tmpCTimes->min[j]
															   || buf[tmpTail+4+j*3+2]!=tmpCTimes->bright[j]
															 )
														  {
																if (copyCtrl[4].tmpCpLink!=NULL)
																{
																  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
																}
																
																if (debugInfo&PRINT_CARRIER_DEBUG)
																{
																  printf("  --(小包)单灯控制器控制时间段%d时段%d与集中器不一致。\n", tmpi+1, j+1);
																}
																checkStep = 2;
																break;
														  }
														}
													}
						    
												  if (1==checkStep)
												  {
												    if (tmpCTimes->hour[j]!=0xff && j<6)
												    {
						    					      if (copyCtrl[4].tmpCpLink!=NULL)
						    					      {
						    					        copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
																			
																	//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
																	if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
																	{
																	  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
																		
																	  if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
																	  {
																		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
																	  }
																	}
    					      						}
    					  	
							    						  if (debugInfo&PRINT_CARRIER_DEBUG)
							    						  {
							    						    printf("  --(小包)单灯控制器控制时间段%d时段数少于集中器中保存时段数。\n", tmpi+1);
							    						  }
							    						  checkStep = 3;
						    							}
						  							}
													}
						  
													if (checkStep>0)
													{
													  break;
													}
						  
													tmpCTimes = tmpCTimes->next;
					 	 						}
						
											  if (0==checkStep)
											  {
											    if (copyCtrl[4].tmpCpLink!=NULL)
											    {
											      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
																
												  	//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												  	if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												  	{
															copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
														
															if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
															{
													  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
															}
												  	}
											    }
					  	
													if (debugInfo&PRINT_CARRIER_DEBUG)
													{
													  printf("  --(小包)单灯控制器控制时间段%d参数在集中器保存时段数中未找到。\n", tmpi+1);
													}
													checkStep = 4;
					  						}
				    					}
							
											//2016-11-14,软件版本为2.0以上的单灯控制器添加启用日判断
											if (copyCtrl[4].tmpCpLink->softVer>=2)
											{
											  tmpTail += 5+buf[tmpTail+4]*3;
											}
											else
											{
											  tmpTail += 4+buf[tmpTail+3]*3;
											}
										}
        					
      			  			if (1==buf[10])
      			  			{
				      				if (debugInfo&PRINT_CARRIER_DEBUG)
				      				{
				      				  printf("  --(小包)单灯控制器保存的控制模式=%d", buf[tmpTail]);
				      				}
				      				if (buf[tmpTail]!=ctrlMode)
				      				{
				      				  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
										
						 					  //2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
											  if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
											  {
													copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
												
													if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
													{
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
													}
											  }

				      				  if (debugInfo&PRINT_CARRIER_DEBUG)
				      				  {
				      						printf(",与集中器控制模式不一致。");
				      				  }
				      				}
				      				tmpTail++;
				      				if (debugInfo&PRINT_CARRIER_DEBUG)
				      				{
				      				  printf("\n");
				      				}
				      				  
				  				    if (debugInfo&PRINT_CARRIER_DEBUG)
				  				    {
				  					  	printf("  --(小包)单灯控制器保存的光控开灯前%d分钟有效", buf[tmpTail]);
				  				    }
				  				    if (buf[tmpTail]!=beforeOnOff[0])
				  				    {
				  				      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;

												//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												{
												  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
													
												  if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
												  {
													copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
												  }
												}

					  				    if (debugInfo&PRINT_CARRIER_DEBUG)
					  				    {
					  					  	printf(",与集中器不一致。");
					  				    }
					  				  }
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("  --(小包)单灯控制器保存的光控开灯后%d分钟有效", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=beforeOnOff[1])
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;

												//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												{
												  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
													
												  if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
												  {
														copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
												  }
												}

					  				    if (debugInfo&PRINT_CARRIER_DEBUG)
					  				    {
					  					  	printf(",与集中器不一致。");
					  				    }
  				  					}
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("  --(小包)单灯控制器保存的光控关灯前%d分钟有效", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=beforeOnOff[2])
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
									
												//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												{
												  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
													
												  if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
												  {
														copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
												  }
												}

					  				    if (debugInfo&PRINT_CARRIER_DEBUG)
					  				    {
					  					  	printf(",与集中器不一致。");
					  				    }
  				  					}
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("  --(小包)单灯控制器保存的光控关灯后%d分钟有效", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=beforeOnOff[3])
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
									
												//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												{
												  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
																	
												  if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
												  {
												    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
												  }
											  }

					  				    if (debugInfo&PRINT_CARRIER_DEBUG)
					  				    {
					  					  	printf(",与集中器不一致。");
					  				    }
					  				  }
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
  				  
  				  
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  					  printf("  --(小包)单灯控制器保存的组别=%d", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=(copyCtrl[4].tmpCpLink->ctrlTime>>4))
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
									
												//2016-11-14,单灯控制器2.0以上的版本,可支持20个时间段
												if (timesCount>6 && copyCtrl[4].tmpCpLink->softVer>=2)
												{
												  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
													
												  if (timesCount>13 && copyCtrl[4].tmpCpLink->softVer>=2)
												  {
														copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
												  }
												}

					  				    if (debugInfo&PRINT_CARRIER_DEBUG)
					  				    {
					  					  	printf(",与集中器不一致。");
					  				    }
					  				  }
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				}

										if (copyCtrl[4].tmpCpLink!=NULL)
										{
										  if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)
										  {
											;
										  }
										  else
										  {
										    if (0x1==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_1;
										      
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
														printf("  --收到单灯控制器控制时段小包1与集中器一致\n");
											  	}
												  
											  	if (buf[14]<3)
											  	{
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_3;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_4;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_5;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_6;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;
										        if (debugInfo&PRINT_CARRIER_DEBUG)
														{
												  		printf("  --且时间段数=%d\n",buf[14]);
														}
											  	}
										    }
										    if (0x2==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;
												  
												  if (
												      buf[14]<6
													   || copyCtrl[4].tmpCpLink->softVer<2
												     )
												  {
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_3;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_4;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_5;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_6;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;
																	
											      if (debugInfo&PRINT_CARRIER_DEBUG)
														{
													 	 	printf("  --且时间段数=%d\n",buf[14]);
														}
												  }
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
														printf("  --收到单灯控制器控制时段小包2与集中器一致\n");
											  	}
										    }
								
										    if (0x3==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_3;

							 					  if (buf[14]<9)
												  {
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_4;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_5;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_6;
												  	copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;
										
										        if (debugInfo&PRINT_CARRIER_DEBUG)
											    	{
											      	printf("  --且时间段数=%d\n",buf[14]);
											    	}
											  	}
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --收到单灯控制器控制时段小包3与集中器一致\n");
											  	}
										    }
								
										    if (0x4==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_4;

						  					  if (buf[14]<12)
											  	{
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_5;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_6;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;
																
										        if (debugInfo&PRINT_CARRIER_DEBUG)
											    	{
											    	  printf("  --且时间段数=%d\n",buf[14]);
											    	}
											  	}
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --收到单灯控制器控制时段小包4与集中器一致\n");
											  	}
										    }
								
										    if (0x5==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_5;

						  					  if (buf[14]<15)
											  	{
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_6;
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;
																
										        if (debugInfo&PRINT_CARRIER_DEBUG)
											    	{
											      	printf("  --且时间段数=%d\n",buf[14]);
											    	}
											  	}
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --收到单灯控制器控制时段小包5与集中器一致\n");
											  	}
										    }
								
										    if (0x6==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_6;
						  					  if (buf[14]<18)
											  	{
											  		copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;
																
										        if (debugInfo&PRINT_CARRIER_DEBUG)
											    	{
											      	printf("  --且时间段数=%d\n",buf[14]);
											    	}
											  	}

										      if (debugInfo&PRINT_CARRIER_DEBUG)
											   	{
											    	printf("  --收到单灯控制器控制时段小包6与集中器一致\n");
											  	}
										    }
								
										    if (0x7==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;

										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --收到单灯控制器控制时段小包7与集中器一致\n");
											  	}
										    }
										  }
										}
								    break;
			 					}
							}
    				
							if (0x05==buf[13] && 0x04==buf[12] && 0xff==buf[11] && 0x00==buf[10])
							{
							  for(tmpi=0; tmpi<buf[14]; tmpi++)
							  {
							  	memcpy(hourDataBuf, &buf[15+tmpi*LEN_OF_LIGHTING_FREEZE], LEN_OF_LIGHTING_FREEZE);
							  	tmpTime.second = 0;
							  	tmpTime.minute = hourDataBuf[0];
							  	tmpTime.hour = hourDataBuf[1];
							  	tmpTime.day = hourDataBuf[2];
							  	tmpTime.month = hourDataBuf[3];
							  	tmpTime.year = hourDataBuf[4];
							  	saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, tmpTime, hourDataBuf, HOUR_FREEZE_SLC, 0x0, LEN_OF_LIGHTING_FREEZE);
							  }
							  
							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
				          printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)小时冻结数据\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
				        }

							  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_HOUR_FREEZE;
							}
							break;
    			
 		  			case 0x94:    //确认
							if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)
							{
							  //清除单独修正控制时段包1标志,并重新读取控制时段数据
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_1 | REQUEST_CTRL_TIME_1 | REQUEST_CTRL_TIME_2| REQUEST_CTRL_TIME_3| REQUEST_CTRL_TIME_4| REQUEST_CTRL_TIME_5| REQUEST_CTRL_TIME_6| REQUEST_CTRL_TIME_7);

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
				          printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)单独修正控制时段包1确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
				        }
            	}
							else if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)
							{
							  //清除单独修正控制时段包2标志,并重新读取控制时段数据
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_2 | REQUEST_CTRL_TIME_1 | REQUEST_CTRL_TIME_2| REQUEST_CTRL_TIME_3| REQUEST_CTRL_TIME_4| REQUEST_CTRL_TIME_5| REQUEST_CTRL_TIME_6| REQUEST_CTRL_TIME_7);

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)单独修正控制时段包2确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}
            	}
							else if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)
							{
							  //清除单独修正控制时段包3标志,并重新读取控制时段数据
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_3 | REQUEST_CTRL_TIME_1 | REQUEST_CTRL_TIME_2| REQUEST_CTRL_TIME_3| REQUEST_CTRL_TIME_4| REQUEST_CTRL_TIME_5| REQUEST_CTRL_TIME_6| REQUEST_CTRL_TIME_7);

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)单独修正控制时段包3确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}
            	}
						
							if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_ADJ_PARA)
							{
							  //清除单独修正调光参数标志,并重新读取实时数据包
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~REQUEST_SYN_ADJ_PARA;

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)单独修正调光参数确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}
            	}
							break;
    			
		  			case 0xd1:    //异常应答
							if (0x01==buf[9] && 0x12==buf[10])
							{
							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,收到SLC(%02x%02x%02x%02x%02x%02x)无小时冻结数据应答\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}

			  				copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_HOUR_FREEZE;
							}
    				
							if (0x01==buf[9] && 0x02==buf[10])
							{
							  if (
								  	0x00==(copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_1)
								   		|| 0x00==(copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_2)
								 	 )
							  {
									copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_1;
									copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;
									
									if (debugInfo&PRINT_CARRIER_DEBUG)
			    				{
                  	printf("%02d-%02d-%02d %02d:%02d:%02d,SLC(%02x%02x%02x%02x%02x%02x)可能不支持控制时段小包传输\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                	}
    		  			}
    					}
    					break;
    			}
      	}
      	else
      	{
    			printf("slc data checkSum error\n");
      	}
    	}
    
    	return;
  }
  else
  {
    if (copyCtrl[port].cpLinkHead==NULL)
    {
      printf("copyCtrl[port].cpLinkHead is NULL ^.^\n");
			
      return;
    }

    //RS485接口上接的线路控制器的交采
    if ((port==1 || port==2)  && AC_SAMPLE==copyCtrl[port].cpLinkHead->protocol)
    {
      if (
      	  buf[0]==0x68 
      	   && buf[7]==0x68 
      	    && buf[len-1]==0x16
      	     && compareTwoAddr(copyCtrl[port].cpLinkHead->addr, &buf[1], 0)==TRUE    //2016-03-21,添加地址比较
      	 )
      {
      	checkSum = 0;
      	for(tmpi=0; tmpi<len-2; tmpi++)
      	{
      	  checkSum += buf[tmpi];
          
          if (tmpi>9)
          {
            buf[tmpi] -= 0x33;  //按字节进行对数据域减0x33处理
          }
      	}
      	
      	if (checkSum==buf[len-2])
      	{
      	  pFoundCpLink = NULL;
      	  tmpCpLink = acLink;
      	  while(tmpCpLink!=NULL)
      	  {
      	  	if (compareTwoAddr(tmpCpLink->addr, &buf[1], 0)==TRUE) 
      	    {
      	      pFoundCpLink = tmpCpLink;
      	      break;
      	    }
      	  	
      	  	tmpCpLink = tmpCpLink->next;
      	  }
      	  switch (buf[8])
      	  {
						case 0x91:
			  			if (buf[13]==0x00 && buf[12]==0x90 && buf[10]==0x00)
			  			{
								switch(buf[11])
								{
				  				case 0xff:
				  					copyCtrl[port].cpLinkHead->flgOfAutoCopy = REQUEST_STATUS;
				  	 	
								  	if (debugInfo&METER_DEBUG)
								  	{
								  	  printf("%02d-%02d-%02d %02d:%02d:%02d,收到线路控制器(%02x%02x%02x%02x%02x%02x)交采数据\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
								  	}
				  	 	
                    memset(acParaData, 0xee, LENGTH_OF_PARA_RECORD);
		  	        		tmpTime = sysTime;
		  	        		tmpTime.second = 0;
		  	        		tmpTime = timeHexToBcd(tmpTime);
		  	        
		  	        		//68 30 00 00 04 14 20 68 
		  	        		//  91 
		  	        		//  53 
		  	        		//  33 32 c3 33
		  	        		//  57 35 33 
		  	        		//  33 33 33 
		  	        		//  33 33 b3 
		  	        		//  57 35 33 
		  	        		//  b4 33 33 33 33 33 33 33 b3 b4 33 b3 87 3c 33 43 33 43 65 3c 3b 56 5a 56 a9 55 34 33 33 34 33 33 38 34 33 c5 35 33 33 34 33 33 33 33 33 33 33 c4 35 33 33 47 34 33 33 33 33 33 33 33 33 33 33 47 34 33 33 21 16
		  	        		tmpTail = 14;
		  	        
		  	        		//总有功功率
		  	        		acParaData[POWER_INSTANT_WORK+0] = buf[tmpTail++];
		  	        		acParaData[POWER_INSTANT_WORK+1] = buf[tmpTail++];
		  	        		acParaData[POWER_INSTANT_WORK+2] = buf[tmpTail++];

				  	        //A相有功功率
				  	        acParaData[POWER_PHASE_A_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_WORK+2] = buf[tmpTail++];

				  	        //B相有功功率
				  	        acParaData[POWER_PHASE_B_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_WORK+2] = buf[tmpTail++];

				  	        //C相有功功率
				  	        acParaData[POWER_PHASE_C_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_WORK+2] = buf[tmpTail++];
				  	        
				  	        //总无功功率
				  	        acParaData[POWER_INSTANT_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_INSTANT_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_INSTANT_NO_WORK+2] = buf[tmpTail++];

				  	        //A相无功功率
				  	        acParaData[POWER_PHASE_A_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_NO_WORK+2] = buf[tmpTail++];

				  	        //B相无功功率
				  	        acParaData[POWER_PHASE_B_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_NO_WORK+2] = buf[tmpTail++];

				  	        //C相无功功率
				  	        acParaData[POWER_PHASE_C_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_NO_WORK+2] = buf[tmpTail++];
				  	        
				  	        //总功率因数
				  	        acParaData[TOTAL_POWER_FACTOR+0] = buf[tmpTail++];
				  	        acParaData[TOTAL_POWER_FACTOR+1] = buf[tmpTail++];
				  	        
				  	        //A相功率因数
				  	        acParaData[FACTOR_PHASE_A+0] = buf[tmpTail++];
				  	        acParaData[FACTOR_PHASE_A+1] = buf[tmpTail++];

				  	        //B相功率因数
				  	        acParaData[FACTOR_PHASE_B+0] = buf[tmpTail++];
				  	        acParaData[FACTOR_PHASE_B+1] = buf[tmpTail++];

				  	        //C相功率因数
				  	        acParaData[FACTOR_PHASE_C+0] = buf[tmpTail++];
				  	        acParaData[FACTOR_PHASE_C+1] = buf[tmpTail++];
				  	        
				  	        //A相电压
				  	        acParaData[VOLTAGE_PHASE_A+0] = buf[tmpTail++];
				  	        acParaData[VOLTAGE_PHASE_A+1] = buf[tmpTail++];

				  	        //B相电压
				  	        acParaData[VOLTAGE_PHASE_B+0] = buf[tmpTail++];
				  	        acParaData[VOLTAGE_PHASE_B+1] = buf[tmpTail++];

				  	        //C相电压
				  	        acParaData[VOLTAGE_PHASE_C+0] = buf[tmpTail++];
				  	        acParaData[VOLTAGE_PHASE_C+1] = buf[tmpTail++];

				  	        //A相电流
				  	        acParaData[CURRENT_PHASE_A+0] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_A+1] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_A+2] = buf[tmpTail++];

				  	        //B相电流
				  	        acParaData[CURRENT_PHASE_B+0] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_B+1] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_B+2] = buf[tmpTail++];

				  	        //C相电流
				  	        acParaData[CURRENT_PHASE_C+0] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_C+1] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_C+2] = buf[tmpTail++];
  				  	        
                    //每15分钟存储一次数据
										//2016-09-02,修改为每5分钟保存一次数据
									 #ifdef JF_MONITOR	
									  if (sysTime.second<=59)    //机房监控每分都存,2018-10-11,改成不限制秒,因为在机房监控中抄表量大,1分钟之内不定能抄完
									 #else
                    if (0==(sysTime.minute%5) && sysTime.second<=30)
									 #endif
                    {
                      //存储
                      saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, tmpTime, \
                      							acParaData, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
                      
                      //示值
                      memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);
                      
                      //正向有功电能示值
                      visionBuff[POSITIVE_WORK_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_OFFSET+3] = buf[tmpTail++];

                      //A相正向有功电能示值
                      visionBuff[POSITIVE_WORK_A_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_A_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_A_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_A_OFFSET+3] = buf[tmpTail++];

                      //B相正向有功电能示值
                      visionBuff[POSITIVE_WORK_B_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_B_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_B_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_B_OFFSET+3] = buf[tmpTail++];

                      //C相正向有功电能示值
                      visionBuff[POSITIVE_WORK_C_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_C_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_C_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_C_OFFSET+3] = buf[tmpTail++];

                      //正向无功电能总示值
                      visionBuff[POSITIVE_NO_WORK_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_NO_WORK_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_NO_WORK_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_NO_WORK_OFFSET+3] = buf[tmpTail++];

                      //A相正向无功电能示值
                      visionBuff[COMB1_NO_WORK_A_OFFSET+0] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_A_OFFSET+1] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_A_OFFSET+2] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_A_OFFSET+3] = buf[tmpTail++];

                      //B相正向无功电能示值
                      visionBuff[COMB1_NO_WORK_B_OFFSET+0] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_B_OFFSET+1] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_B_OFFSET+2] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_B_OFFSET+3] = buf[tmpTail++];

                      //C相正向无功电能示值
                      visionBuff[COMB1_NO_WORK_C_OFFSET+0] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_C_OFFSET+1] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_C_OFFSET+2] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_C_OFFSET+3] = buf[tmpTail++];
                      
                      saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, tmpTime, \
                                    visionBuff, PRESENT_DATA, ENERGY_DATA, LENGTH_OF_ENERGY_RECORD);

							  	 	  if (debugInfo&METER_DEBUG)
							  	 	  {
							  	 	    printf("%02d-%02d-%02d %02d:%02d:%02d,保存线路控制器(%02x%02x%02x%02x%02x%02x)交采数据\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
							  	 	  }
		  	        		}
		  	        		if (pFoundCpLink!=NULL)
		  	        		{
							  	 	  //1.判断功率陡降
							  	 	  tmpP = acParaData[POWER_INSTANT_WORK+2]<<16 | acParaData[POWER_INSTANT_WORK+1]<<8 | acParaData[POWER_INSTANT_WORK+0];
							  	 	  tmpP = bcdToHex(tmpP);
							  	 	  tmpPrevP = pFoundCpLink->duty[2]<<16 | pFoundCpLink->duty[1]<<8 | pFoundCpLink->duty[0];
							  	 	  tmpPrevP = bcdToHex(tmpPrevP);
							  	 	  if (debugInfo&METER_DEBUG)
							  	 	  {
							  	 	    printf(
							  	 	           "%02d-%02d-%02d %02d:%02d:%02d,上一次线路测量总功率=%ld,本次线路测量总功率=%ld\n", 
							  	 	            sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
							  	 	             tmpPrevP, tmpP
							  	 	          );
							  	 	  }
							  	 	  //功率降低了30%
							  	 	  if (tmpP < (tmpPrevP*7/10))
							  	 	  {
							  	 	  	tmpNode = ldgmLink;
							  	 	  	while(tmpNode!=NULL)
							  	 	    {
							  	 	      if (
							  	 	    	  	(tmpNode->mpNo>>8)==pFoundCpLink->mp    //报警控制点的辅助判断测量点是本测量点
							  	 	    	  	 && (tmpNode->status&0x1)==0x1          //线路上电
							  	 	    	 	 )
							  	 	      {
						     	 	        //第一末端单灯控制器地址不为全0
						     	 	        if (FALSE == compareTwoAddr(tmpNode->lddt1st, NULL, 1))
						     	 	        {
						     	 	        	tmpZbNode = copyCtrl[4].cpLinkHead;
					  		 	 	          while(tmpZbNode!=NULL)
					  		 	 	          {
					  		 	 	          	if (compareTwoAddr(tmpZbNode->addr, tmpNode->lddt1st, 0)==TRUE)
					  		 	 	          	{
				     	 	        	    		//状态时间退到心跳阈值之前
				     	 	        	    		//tmpZbNode->statusTime = backTime(sysTime, 0, 0, 0, tmpNode->funOfMeasure+1, 0);
				     	 	        	    		tmpZbNode->lddtStatusTime = backTime(sysTime, 0, 0, 0, tmpNode->funOfMeasure+1, 0);    //2016-02-03
				     	 	        	  		}
				     	 	        	  
				     	 	        	  		tmpZbNode = tmpZbNode->next;
				     	 	        			}
				     	 	        		}
     	 	        
						     	 	        //第二末端单灯控制器地址不为全0
						     	 	        if (FALSE == compareTwoAddr(tmpNode->collectorAddr, NULL, 1))
						     	 	        {
						     	 	          tmpZbNode = copyCtrl[4].cpLinkHead;
					  		 	 	          while(tmpZbNode!=NULL)
					  		 	 	          {
					  		 	 	          	if (compareTwoAddr(tmpZbNode->addr, tmpNode->collectorAddr, 0)==TRUE)
					  		 	 	          	{
						     	 	        	  //状态时间退到心跳阈值之前
						     	 	        	  //tmpZbNode->statusTime = backTime(sysTime, 0, 0, 0, tmpNode->funOfMeasure+1, 0);
						     	 	        	  tmpZbNode->lddtStatusTime = backTime(sysTime, 0, 0, 0, tmpNode->funOfMeasure+1, 0);    //2016-02-03
						     	 	        	}
						     	 	        	  
						     	 	        	tmpZbNode = tmpZbNode->next;
						     	 	          }
						     	 	        }                 	 	        
			  	            
			  	            			carrierFlagSet.searchLddtTime = sysTime;
	  	            
		  	 	    							if (debugInfo&METER_DEBUG)
					  	 	            {
					  	 	              printf(
					  	 	                     "%02d-%02d-%02d %02d:%02d:%02d,功率急速下降,紧急搜索报警控制点是否存在\n", 
					  	 	                      sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second
					  	 	                    );
					  	 	            }
			  	 	      				}
			  	 	      				tmpNode = tmpNode->next;
			  	 	    				}
			  	 	  				}
		  	      	
		  	      	  		pFoundCpLink->statusTime = sysTime;
		  	      	  		memcpy(pFoundCpLink->duty, &buf[14], 3);
		  	      	
		  	      	  		//2.线路测量点越限判断
					  					processKzqOverLimit(acParaData, copyCtrl[port].cpLinkHead->mp);
		  	        		}
  				  	      
      							break;
      					}
      		  	}
      		  	break;
      	  }
      		
      	  copyCtrl[port].copyContinue = TRUE;
        }
      }
      
      return;
    }
  
    //RS485接口上接的线路控制器
    if (port<4 && LIGHTING_XL==copyCtrl[port].cpLinkHead->protocol)
    {
      if (
      	  buf[0]==0x68 
      	   && buf[7]==0x68 
      	    && buf[len-1]==0x16
      	     && compareTwoAddr(copyCtrl[port].cpLinkHead->addr, &buf[1], 0)==TRUE    //2016-03-21,添加地址比较
      	 )
      {
      	checkSum = 0;
      	for(tmpi=0; tmpi<len-2; tmpi++)
      	{
      	  checkSum += buf[tmpi];
          
          if (tmpi>9)
          {
            buf[tmpi] -= 0x33;  //按字节进行对数据域减0x33处理
          }
      	}
      	
      	if (checkSum==buf[len-2])
      	{
      	  pFoundCpLink = NULL;
      	  tmpCpLink = xlcLink;    		
      	  while(tmpCpLink!=NULL)
      	  {
      	  	if (compareTwoAddr(tmpCpLink->addr, &buf[1], 0)==TRUE)
      	    {
      	      pFoundCpLink = tmpCpLink;
      	      break;
      	    }
      	  	
      	  	tmpCpLink = tmpCpLink->next;
      	  }
		  
      	  switch (buf[8])
      	  {
      			case 0x88:
			  			//清除校时标志并重新读取实时数据包
			  			copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_CHECK_TIME | REQUEST_STATUS);

			  			if (debugInfo&METER_DEBUG)
			  			{
                printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)校时确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              }
      		  	break;
      				
      			case 0x91:
			  			if (buf[13]==0x00 && buf[12]==0x90 && buf[10]<0x03)
			  			{
								switch(buf[11])
								{
								  case 0x00:    //00900000
      	            if (pDotCopy!=NULL)
      	            {
      	    	      	if (pDotCopy->dotCopying==TRUE)
      	    	      	{
                        memcpy(pDotCopy->data, &buf[14], 8);

                        pDotCopy->dotRetry  = 0;
      	    		    		pDotCopy->dotResult = RESULT_HAS_DATA;
      	    		
      	    		   		 	pDotCopy->dotRecvItem++;
      	    		
      	    		    		copyCtrl[port].copyContinue = TRUE;
      	              }
                    }
				  					break;
										
				  				case 0x04:    //控制时段小包数据
										//68 51 02 00 04 16 20 68 91 47 34 37 c3 33 39 64 34 52 ae 35 4c 33 34 3a 33 33 97 34 51 b2 35 4c 43 34 39 83 33 ba 34 3d b2 35 3b 33 34 4a 73 33 bb 3e 47 b2 35 3b 68 34 4a 39 33 cb 48 51 b1 35 3b 68 34 4a 33 33 fd 34 52 b2 35 3b 63 34 4a 38 33 be 16 
										if (debugInfo&PRINT_CARRIER_DEBUG)
										{
										  printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)控制时段数据小包%d.\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1],buf[10]);
										}
										
				  					timesCount = 0;
				    				tmpCTimes = cTimesHead;
				    				while(tmpCTimes)
				    				{
	           	    	  if (
	           	    		  	tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime 
	           	    		  	 && 
	           	    		     	(
	           	    		      	(2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //控制模式为时段控或时段控结合光控
	           	    		       	|| (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //控制模式为经纬度控或经纬度控结合光控
	           	    		     	)
	           	    		 	)
	        			  		{
												timesCount++;
						  				}
        			  			tmpCTimes = tmpCTimes->next;
        						}

										if (
												(1==buf[10] && buf[14]<10)
											 	||2==buf[10]
										   )
									  {
						  				if (timesCount!=buf[14])
					  					{
												if (copyCtrl[port].cpLinkHead!=NULL)
												{
						  						copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
							
						  						if (timesCount>10)
						  						{
														copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
						  						}
												}
											
												if (debugInfo&METER_DEBUG)
												{
												  printf("  --线路控制器控制时间段数量与集中器不等。\n");
												}
					  					}
										}
  
  									//buf[14]是上送的该线路控制器上保存的有效的时间段个数
										tmpTail = 15;
  									checkStep = 0;
										for(tmpi=(buf[10]-1)*10; tmpi<buf[14]; tmpi++)
          					{
					  					if (checkStep<2)
					  					{
					    					checkStep = 0;
												tmpCTimes = cTimesHead;
												while(tmpCTimes)
					    					{
						  						if (tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime
       	    		          		&& 
       	    		           		(
	       	    		        			(2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //控制模式为时段控或时段控结合光控
	       	    		         				|| (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //控制模式为经纬度控或经纬度控结合光控
	       	    		       			)
						  	    						&& tmpCTimes->startMonth==(buf[tmpTail]&0x0f) && tmpCTimes->startDay==buf[tmpTail+1]
						  	     							&& tmpCTimes->endMonth==(buf[tmpTail]>>4&0x0f) && tmpCTimes->endDay==buf[tmpTail+2]
						  	 						 )
						  						{
												  	checkStep = 1;
												  	
												  	//2016-8-19,添加启用日判断
												  	if (tmpCTimes->workDay!=buf[tmpTail+3])
												  	{
												  	  checkStep = 4;
												  		
						        					copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
													  	if (timesCount>10)
													  	{
																copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
													  	}

						    						  if (debugInfo&METER_DEBUG)
						    						  {
						    						    printf("  --线路控制器控制时间段%d启用日与集中器不一致。\n", tmpi+1);
						    						  }
												  	}
												  	else
												  	{
					   						  	  numOfTime = buf[tmpTail+4];
					  						  	  for(j=0; j<numOfTime; j++)
					  						      {
						  						    	if (buf[tmpTail+5+j*3]!=tmpCTimes->hour[j] 
						  						    		 || buf[tmpTail+5+j*3+1]!=tmpCTimes->min[j]
						  						    		   || buf[tmpTail+5+j*3+2]!=tmpCTimes->bright[j]
						  						    		 )
						  						      {
      					      						if (copyCtrl[port].cpLinkHead!=NULL)
      					     						 	{
      					        						copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
																		if (timesCount>10)
																		{
																			copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
																		}
      					      						}
      					  	
						        						  if (debugInfo&METER_DEBUG)
						        						  {
						        						    printf("  --线路控制器控制时间段%d时段%d与集中器不一致。\n", tmpi+1, j+1);
						        						  }
						        						  checkStep = 2;
						        					  	break;
						        						}
						      						}
						    						}
						    
												    if (1==checkStep)
												    {
												      if (tmpCTimes->hour[j]!=0xff && j<6)
												      {
				      					        if (copyCtrl[port].cpLinkHead!=NULL)
				      					        {
				      					          copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
																  if (timesCount>10)
																  {
																		copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
																  }
      					        				}
      					  	
							      						if (debugInfo&METER_DEBUG)
							      						{
							      						  printf("  --线路控制器控制时间段%d时段数少于集中器中保存时段数。\n", tmpi+1);
							      						}
							      						checkStep = 3;
												      }
						    						}
												  }
												  
												  if (checkStep>0)
												  {
												  	break;
												  }
												  
												  tmpCTimes = tmpCTimes->next;
												}
						
												if (0==checkStep)
												{
										      if (copyCtrl[port].cpLinkHead!=NULL)
										      {
										        copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
														if (timesCount>10)
														{
															copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_2;
														}
												  }
				  	
												  if (debugInfo&METER_DEBUG)
												  {
												    printf("  --线路控制器控制时间段%d参数在集中器保存时段数中未找到。\n", tmpi+1);
												  }
												  checkStep = 4;
												}
				      				}
          						
					  					tmpTail += 5+buf[tmpTail+4]*3;
          					}
										
  				    			if (copyCtrl[port].cpLinkHead!=NULL)
				    				{
					  					if (
										      copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1
												 	|| copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2
											   )
										  {
												;
										  }
										  else
										  {
												if (0x1==buf[10])
												{
						  						copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CTRL_TIME_1;
							
												  if (debugInfo&METER_DEBUG)
												  {
														printf("  --收到线路控制点控制时段小包1与集中器一致\n");
												  }
							
												  if (buf[14]<10)
												  {
														copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;
														if (debugInfo&METER_DEBUG)
														{
													  	printf("  --且时间段数=%d\n",buf[14]);
														}
												  }
												}
												if (0x2==buf[10])
												{
												  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;

												  if (debugInfo&METER_DEBUG)
												  {
														printf("  --收到线路控制点控制时段小包2与集中器一致\n");
												  }
												}
						
												if ( 
												    (0x1==buf[10] && buf[14]<10)
												     || 0x2==buf[10]
												   )
												{												
												  searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);    //控制器与时间无关量
												  meterStatisExtranTimeS.mixed |= 0x20;    //标记控制时段等参数已同步到单灯控制控制器
												  saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
												}
											}
										}
										break;
									
								  case 0xff:
				  					//1.提取XLC时间
				  					tmpTime.second = buf[21];
				  					tmpTime.minute = buf[22];
				  					tmpTime.hour   = buf[23];
				  					tmpTime.day    = buf[24];
				  					tmpTime.month  = buf[25];
				  					tmpTime.year   = buf[26];
				  					tmpTime = timeBcdToHex(tmpTime);
				      
				  					if (pFoundCpLink!=NULL)
				  					{
				  					  //状态已更新,2015-10-26,Add
				  					  if (buf[27]!=pFoundCpLink->status)
				  					  {
				  					  	pFoundCpLink->statusUpdated |= 1;
				  					  	
				  					  	//线路控制器分闸
				  					  	if (buf[27]==0x00)
				  					  	{
				  					  	  pFoundCpLink->gateOn = 0;
				  					  		 
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				 	  		 	 	      while(tmpNode!=NULL)
				 	  		 	 	      {
				  					  			//2015-11-14,线路控制器下属单灯控制器应为关(亮度为0%)
				 	  		 	 	      	if (tmpNode->mpNo==pFoundCpLink->mp)
				 	  		 	 	      	{
				 	  		 	 	      	  tmpNode->status = 0;
				 	  		 	 	      	  tmpNode->statusUpdated |= 1;
				 	  		 	 	      	  tmpNode->lineOn = 0;    //载波线路断电
				 	  		 	 	      	  
				 	  		 	 	      	  if (debugInfo&METER_DEBUG)
				 	  		 	 	      	  {
				 	  		 	 	      	  	printf(
				 	  		 	 	      	  	       "线路控制点(%02x%02x%02x%02x%02x%02x)下属单灯控制点(%02x%02x%02x%02x%02x%02x)置为断电,亮度0%\n",
				 	  		 	 	      	  	       pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
				 	  		 	 	      	  	       tmpNode->addr[5],pFoundCpLink->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0]
				 	  		 	 	      	  	      );
				 	  		 	 	      	  }
				 	  		 	 	      	}
				 	  		 	 	      	
				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
				  					  	}
          					  	
				  					  	//线路控制器合闸,2015-11-09,Add
				  					  	if (buf[27]==0x01)
				  					  	{
				  					  	  pFoundCpLink->gateOn = 1;
				  					  		
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				 	  		 	 	      while(tmpNode!=NULL)
				 	  		 	 	      {
				  					  			//2015-11-14,通知线路控制点下属单灯控制点已上电
				 	  		 	 	      	if (tmpNode->mpNo==pFoundCpLink->mp)
				 	  		 	 	      	{
				 	  		 	 	      	  tmpNode->lineOn = 1;    //载波线路断电
				 	  		 	 	      	
				 	  		 	 	      	  tmpNode->statusTime = sysTime;        //最新状态时间改为当前时间
				 	  		 	 	      	  tmpNode->lddtStatusTime = sysTime;    //最新状态时间改为当前时间,2016-02-03,Add
				 	  		 	 	      	  
				 	  		 	 	      	  if (debugInfo&METER_DEBUG)
				 	  		 	 	      	  {
				 	  		 	 	      	  	printf(
				 	  		 	 	      	  	       "通知线路控制点(%02x%02x%02x%02x%02x%02x)下属单灯控制点(%02x%02x%02x%02x%02x%02x)上电\n",
				 	  		 	 	      	  	       pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
				 	  		 	 	      	  	       tmpNode->addr[5],pFoundCpLink->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0]
				 	  		 	 	      	  	      );
				 	  		 	 	      	  }
				 	  		 	 	      	}
					  		 	 	      	
				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
				  					  	}
				  					  }
          					  
          			  		pFoundCpLink->statusTime = tmpTime;    //XLC当前时间
                      pFoundCpLink->status = buf[27];        //XLC当前状态
                    }
      
  									if (debugInfo&METER_DEBUG)
  									{
                      printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)实时数据包:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                      printf("  -XLC当前闸状态:%d\n", buf[27]);
                      printf("  -XLC当前YX状态:%d\n", buf[28]);
                      printf("  -XLC当前时间:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
                    }
                    
          					
                    //1-1.判断XLC是否时间超差
                    if (!((timeCompare(tmpTime, sysTime, pnGate.checkTimeGate) == TRUE) 
             	        	|| (timeCompare(sysTime, tmpTime, pnGate.checkTimeGate) == TRUE)))
                    {
				  					  if (debugInfo&METER_DEBUG)
				  					  {
				  							printf("  --XLC时间超差\n");
				  					  }
  						
				  					  if (copyCtrl[port].cpLinkHead!=NULL)
				  					  {
				  					    copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CHECK_TIME;
				  					  }
				  					}
          					
				  					//2.提取主站控制截止时间
				  					tmpTime.second = buf[15];
				  					tmpTime.minute = buf[16];
				  					tmpTime.hour   = buf[17];
				  					tmpTime.day    = buf[18];
				  					tmpTime.month  = buf[19];
				  					tmpTime.year   = buf[20];
				  					tmpTime = timeBcdToHex(tmpTime);
                    
                    //68 01 00 00 04 14 20 68 91 24 33 32 c3 33 
                    //31 33 33 33 33 33 33 
                    //5b 33 47 5b 3c 47 
                    //31 33 
                    //33 
                    //32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 e6 16 
          					
				  					if (pFoundCpLink!=NULL)
				  					{
				  					  pFoundCpLink->msCtrlCmd = buf[14];     //主站直接控制命令
				              pFoundCpLink->msCtrlTime = tmpTime;    //主站控制截止时间
				            }
				      
				  				  if (debugInfo&METER_DEBUG)
				  					{
                      if (buf[14]>100)
                      {
                        printf("  -无主站控制命令");
                      }
                      else
                      {
                        printf("  -主站控制命令=%d", buf[14]);
                      }
                      printf(",截止时间:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
          					}
          					
  									//3.判断XLC时段与集中器是否一致
										//2016-11-02起，控制时段参数不在这里发送,在00900400中发送

										//反馈遥信是否接入
										tmpTail = 29;
				  					if (buf[tmpTail]!=copyCtrl[port].cpLinkHead->joinUp)
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制器是否接入反馈遥信与集中器不一致,应同步\n");
                  	  }
          					}
          					tmpTail++;

  									//2015-6-11,add,控制模式
  									if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --线路控制器控制模式=%d\n", buf[tmpTail]);
                  	}
				  					//if (buf[tmpTail]!=ctrlMode)
				  					//2016-02-15,改成要控制点的控制模式
				  					if (buf[tmpTail]!=copyCtrl[port].cpLinkHead->bigAndLittleType)
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制器控制模式与集中器不一致,应同步\n");
                  	  }
          					}
          					tmpTail++;

		          			//2015-6-26,add,光控开灯前多少分钟有效
		          			if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --线路控制器光控合闸前%d分钟有效\n", buf[tmpTail]);
                  	}
				  					if (buf[tmpTail]!=beforeOnOff[0])
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制器光控合闸前分钟数与集中器不一致,应同步\n");
                  	  }
          					}
          					tmpTail++;

          					if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --线路控制器光控合闸后%d分钟有效\n", buf[tmpTail]);
                  	}
  									if (buf[tmpTail]!=beforeOnOff[1])
  									{
  					 			 		copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
  					  
  					  				if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制器光控合闸后分钟数与集中器不一致,应同步\n");
                  	  }
          					}
          					tmpTail++;

          					if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --线路控制器光控分闸前%d分钟有效\n", buf[tmpTail]);
                  	}
				  					if (buf[tmpTail]!=beforeOnOff[2])
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制器光控分闸前分钟数与集中器不一致,应同步\n");
                  	  }
          					}
          					tmpTail++;

          					if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --线路控制器光控分闸后%d分钟有效\n", buf[tmpTail]);
                  	}
				  					if (buf[tmpTail]!=beforeOnOff[3])
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制器光控分闸后分钟数与集中器不一致,应同步\n");
                  	  }
          					}
          					tmpTail++;
          				  
				  					//6.XLC检测到的上一次灯故障发生时间
				  					tmpTime.year = buf[tmpTail+5];
				  					tmpTime.month = buf[tmpTail+4];
				  					tmpTime.day = buf[tmpTail+3];
				  					tmpTime.hour = buf[tmpTail+2];
				  					tmpTime.minute = buf[tmpTail+1];
				  					tmpTime.second = buf[tmpTail];
				  					tmpTail+=6;
				  					if (debugInfo&METER_DEBUG)
				  					{
				  						printf("  -XLC上一次线路故障发生时间:%02x-%02x-%02x %02x:%02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
				  					}
                    
                    if (tmpTime.year!=0xff && tmpTime.month!=0xff && tmpTime.day!=0xff && tmpTime.minute!=0xff && tmpTime.second!=0xff)
                    {
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);    //控制器与时间无关量
                      if (tmpTime.year!=meterStatisExtranTimeS.lastFailure.year
                      	  || tmpTime.month!=meterStatisExtranTimeS.lastFailure.month
                      	   || tmpTime.day!=meterStatisExtranTimeS.lastFailure.day
                      	    || tmpTime.hour!=meterStatisExtranTimeS.lastFailure.hour
                      	     || tmpTime.minute!=meterStatisExtranTimeS.lastFailure.minute
                      	      || tmpTime.second!=meterStatisExtranTimeS.lastFailure.second
                      	 )
                      {
                      	meterStatisExtranTimeS.lastFailure = tmpTime;
                      	
                        //存储控制点统计数据(与时间无关量)
                        saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                        
                        //记录控制器故障信息(ERC13)
	                    	eventData[0] = 13;
	                    	eventData[1] = 11;
	                    	
	                    	eventData[2] = tmpTime.second;
	                    	eventData[3] = tmpTime.minute;
	                    	eventData[4] = tmpTime.hour;
	                    	eventData[5] = tmpTime.day;
	                    	eventData[6] = tmpTime.month;
	                    	eventData[7] = tmpTime.year;
	                    	  
	                    	eventData[8] = copyCtrl[port].cpLinkHead->mp&0xff;
	                    	eventData[9] = (copyCtrl[port].cpLinkHead->mp>>8&0xff) | 0x80;
	                    	eventData[10] = 0x20;
	                    	  
	                    	writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
	                    	  
	                    	if (eventRecordConfig.nEvent[1]&0x10)
	                    	{
	                    	  writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
	                    	}
	                    	  
	                    	eventStatus[1] = eventStatus[1] | 0x10;
	            					  
										    if (debugInfo&METER_DEBUG)
										    {
										  	  printf("  --记录控制点%d故障事件\n", copyCtrl[port].cpLinkHead->mp);
										    }
                        
                        activeReport3();                  //主动上报事件
                      }
                    }
                    
                    //7.最后一次接通、断开时刻
				  					//7.1最后一次接通时刻
				  					tmpTime.year = buf[tmpTail+4];
				  					tmpTime.month = buf[tmpTail+3];
				  					tmpTime.day = buf[tmpTail+2];
				  					tmpTime.hour = buf[tmpTail+1];
				  					tmpTime.minute = buf[tmpTail+0];
				  					tmpTime.second = 0x00;
				  					tmpTail+=5;
				  					if (debugInfo&METER_DEBUG)
				  					{
				  					  printf("  -XLC上一次接通时刻:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
				  					}
                    saveSlDayData(copyCtrl[port].cpLinkHead->mp, 1, tmpTime, (INT8U *)&tmpTime, 6);
  
				  					//7.2最后一次切断时刻
				  					tmpTime.year = buf[tmpTail+4];
				  					tmpTime.month = buf[tmpTail+3];
				  					tmpTime.day = buf[tmpTail+2];
				  					tmpTime.hour = buf[tmpTail+1];
				  					tmpTime.minute = buf[tmpTail+0];
				  					tmpTime.second = 0x00;
				  					tmpTail+=5;
				  					if (debugInfo&METER_DEBUG)
				  					{
				  						printf("  -XLC上一次切断时刻:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
				  					}
                    saveSlDayData(copyCtrl[port].cpLinkHead->mp, 4, tmpTime, (INT8U *)&tmpTime, 6);
  
				  					if (
				  						  buf[tmpTail+0]!=copyCtrl[port].cpLinkHead->collectorAddr[0]
				  						   || buf[tmpTail+1]!=copyCtrl[port].cpLinkHead->collectorAddr[1]
				  						    || buf[tmpTail+2]!=copyCtrl[port].cpLinkHead->collectorAddr[2]
				  						     || buf[tmpTail+3]!=copyCtrl[port].cpLinkHead->collectorAddr[3]
				  						      || buf[tmpTail+4]!=copyCtrl[port].cpLinkHead->collectorAddr[4]
				  						       || buf[tmpTail+5]!=copyCtrl[port].cpLinkHead->collectorAddr[5]
				  						 )
				  					{
				  						//2019-01-22,经纬度不相同请求标志原来置错了
				  					  //copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --线路控制点经纬度与集中器不一致,应同步\n");
                  	  }
          					}
				  					if (debugInfo&METER_DEBUG)
				  					{
									  	printf("  -XLC经度:%d.%04d\n", buf[tmpTail+0], buf[tmpTail+1] | buf[tmpTail+2]<<8);
									  	printf("  -XLC纬度:%d.%04d\n", buf[tmpTail+3], buf[tmpTail+4] | buf[tmpTail+5]<<8);
									  	printf("  -XLC根据经纬度计算出的日出时间:%02d:%02d\n", buf[tmpTail+7], buf[tmpTail+6]);
									  	printf("  -XLC根据经纬度计算出的日落时间:%02d:%02d\n", buf[tmpTail+9], buf[tmpTail+8]);

									  	printf("  -XLC经纬度控制模式合闸微调:");
									  	if (buf[tmpTail+10]&0x80)
									  	{
												printf("-");
									  	}
									 	 	printf("%02d\n", buf[tmpTail+10]&0x7f);
									
									  	printf("  -XLC经纬度控制模式分闸微调:");
									  	if (buf[tmpTail+11]&0x80)
									  	{
												printf("-");
									  	}
									  	printf("%02d\n", buf[tmpTail+11]&0x7f);
				  					}
          					
				  					//日出日落时间
				  					if (pFoundCpLink!=NULL)
				  					{
				  					  //经纬度控制时,日出日落时刻变更后更新到主站服务器
									  	//经纬度结合光控时,日出日落时刻变更后更新到主站服务器,2016-10-20
				  					  if (
									      	CTRL_MODE_LA_LO==pFoundCpLink->bigAndLittleType
										   		|| CTRL_MODE_LA_LO_LIGHT==pFoundCpLink->bigAndLittleType
										 		 )
				  					  {
				  					    if (
				  					  	    buf[tmpTail+6]!=pFoundCpLink->duty[0]
				  					  	     || buf[tmpTail+7]!=pFoundCpLink->duty[1]
				  					  	      || buf[tmpTail+8]!=pFoundCpLink->duty[2]
				  					  	       || buf[tmpTail+9]!=pFoundCpLink->duty[3]
				  					  	   )
				  					    {
				  					  	  pFoundCpLink->statusUpdated |= 0x2;
				  					  	}
				  					  }
  					  
  					  				memcpy(pFoundCpLink->duty, &buf[tmpTail+6], 4);
  									}
          					
  									//合/分闸微调
  									if (
					  						buf[tmpTail+10]!=copyCtrl[port].cpLinkHead->duty[4]
					  						 || buf[tmpTail+11]!=copyCtrl[port].cpLinkHead->duty[5]
					  					   )
					  				{
  					  				copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
  					  
  					  				if (debugInfo&METER_DEBUG)
		          	  		{
		          					printf("  --线路控制点经纬度控制合/分闸微调与集中器不一致,应同步\n");
		          	  		}
  				    			}
 
				  					if (copyCtrl[port].cpLinkHead!=NULL)
				  					{
				  					  if (
				  					  	  (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CHECK_TIME) 
				  					  	   || (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)
				  					  	    || (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CLOSE_GATE)
				  					  	     || (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_OPEN_GATE)
				  					  	 )
				  					  {
				  							;
				  					  }
				  					  else
				  					  {
				  					    copyCtrl[port].cpLinkHead->flgOfAutoCopy = REQUEST_STATUS;
				  					  }
				  					}

          					break;
          			}
      		  	}
      				
			 	 			if (0x05==buf[13] && 0x04==buf[12] && 0xff==buf[11] && 0x00==buf[10])
			  			{
                memset(hourDataBuf, 0xee, LEN_OF_LIGHTING_FREEZE);
						    for(tmpi=0; tmpi<buf[14]; tmpi++)
						    {
						  	  memcpy(hourDataBuf, &buf[15+tmpi*7], 5);
			      
						  	  tmpTime.second = 0;
						  	  tmpTime.minute = hourDataBuf[0];
						  	  tmpTime.hour = hourDataBuf[1];
						  	  tmpTime.day = hourDataBuf[2];
						  	  tmpTime.month = hourDataBuf[3];
						  	  tmpTime.year = hourDataBuf[4];
						  	
					  	    hourDataBuf[28] = buf[15+tmpi*7+5];
					  	    hourDataBuf[29] = buf[15+tmpi*7+6];
					  	  
						  	  saveMeterData(copyCtrl[port].cpLinkHead->mp, port, tmpTime, hourDataBuf, HOUR_FREEZE_SLC, 0x0, LEN_OF_LIGHTING_FREEZE);
						    }
			  
			    			if (debugInfo&METER_DEBUG)
			    			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)小时冻结数据\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
  
      					copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_HOUR_FREEZE;
      		  	}
      		  	break;
      			
      			case 0x94:    //写时段确认
						  if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_SYN_CTRL_TIME_1)
						  {
						    //清除修正控制时段1标志,并重新读取控制时段1
						    copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_1 | REQUEST_CTRL_TIME_1);
						  
						    //如果有光控命令清除光控合、分闸命令
                copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
                copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;

		      			if (debugInfo&METER_DEBUG)
		      			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)修正控制时段第一部分确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
              }
						  else
						  {
								if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_SYN_CTRL_TIME_2)
								{
								  //清除修正控制时段2标志,并重新读取控制时段2
								  copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_2 | REQUEST_CTRL_TIME_2);
								
								  //如果有光控命令清除光控合、分闸命令
								  copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
								  copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;

								  if (debugInfo&METER_DEBUG)
								  {
										printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)修正控制时段第二部分确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
								  }
								}
								else
								{
								  if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_SYN_CTRL_TIME_3)
								  {
										//清除修正控制时段3标志,并重新读取状态数据
										copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_3 | REQUEST_STATUS);
									
										//如果有光控命令清除光控合、分闸命令
										copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
										copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;

										if (debugInfo&METER_DEBUG)
										{
									 	 	printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)修正控制时段第三部分确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
										}
								  }
								}
							}
      		  	break;
      				
						case 0x9C:    //确认
						case 0xDC:    //否认
  			  		if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_CLOSE_GATE)
  			  		{
  							//清除合闸标志
  							copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;
  							if (debugInfo&METER_DEBUG)
  							{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)合闸", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                  if (0x9c==buf[8])
                  {
              	    printf("确认\n");
                  }
                  else
                  {
                  	printf("否认\n");
                  }
                }
                
                
                if (pFoundCpLink!=NULL)
                {
                  //从分闸变成合闸
                  if (0x9c==buf[8])
                  {
                  	pFoundCpLink->gateOn = 1;
                  }
                  
                  //否认,将光控合闸命令置为分闸
                  //2016-03-17,注释这一个判断
                  //if (1==pFoundCpLink->lcOnOff && 0xdc==buf[8])
                  //{
                	//  pFoundCpLink->lcOnOff = 0;
                  //}
                }
              }
  			  		if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_OPEN_GATE)
			  			{
			    			//清除分闸标志
			    			copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
			    			if (debugInfo&METER_DEBUG)
			    			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)分闸", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                  if (0x9c==buf[8])
                  {
                  	printf("确认\n");
                  }
                  else
                  {
                  	printf("否认\n");
                  }
                }
                
                //否认,将光控分闸命令置为合闸
                //2016-03-17,注释这一个判断
                //if (pFoundCpLink!=NULL)
                //{
                //  if (0==pFoundCpLink->lcOnOff && 0xdc==buf[8])
                //  {
                //	  pFoundCpLink->lcOnOff = 1;
                //	}
                //}
                
                lCtrl.bright = 0xfe;
              }
      		  	break;

      			case 0xd1:    //异常应答
  			  		if (0x01==buf[9] && 0x12==buf[10])
			  			{
								if (debugInfo&METER_DEBUG)
			    			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到XLC(%02x%02x%02x%02x%02x%02x)无小时冻结数据应答\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }

								copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_HOUR_FREEZE;
			  			}
			  			break;
      	  }
      		
      	  copyCtrl[port].copyContinue = TRUE;
      	}
      }
      
      return;
    }
    
    //RS485接口上接的线缆盗割报警控制器
    if (port<4 && LIGHTING_DGM==copyCtrl[port].cpLinkHead->protocol)
    {
      if (
      	  buf[0]==0x68 
      	   && buf[7]==0x68 
      	    && buf[len-1]==0x16
      	     && compareTwoAddr(copyCtrl[port].cpLinkHead->addr, &buf[1], 0)==TRUE    //2016-03-21,添加地址比较
      	 )
      {
      	checkSum = 0;
      	for(tmpi=0; tmpi<len-2; tmpi++)
      	{
      	  checkSum += buf[tmpi];
          
          if (tmpi>9)
          {
            buf[tmpi] -= 0x33;    //按字节进行对数据域减0x33处理
          }
      	}
      	
      	if (checkSum==buf[len-2])
      	{
      	  pFoundCpLink = NULL;
      	  tmpCpLink = ldgmLink;
      	  while(tmpCpLink!=NULL)
      	  {
      	  	if (compareTwoAddr(tmpCpLink->addr, &buf[1], 0)==TRUE) 
      	    {
      	      pFoundCpLink = tmpCpLink;
      	      break;
      	    }
      	  	
      	  	tmpCpLink = tmpCpLink->next;
      	  }
		  
      	  switch (buf[8])
      	  {	
  					case 0x91:
  			 		 if (buf[13]==0x00 && buf[12]==0x90 && buf[10]==0x00)
  			  		{
								switch(buf[11])
			    			{
				  				case 0xff:
  									if (pFoundCpLink!=NULL)
				  					{          					  
				  					  //2015-03-10
				  					  //现象:华伟集团1#线的报警控制器经过1个月的运行后发现多次线路刚上电时误报警一个线缆异常
				  					  //     几分钟后又产生一个恢复事件
				  					  //修改:线路由无电变为有交流电时,将报警控制点的末端单灯载波控制点的最近时间
				  					  //     考虑刚上电时载波网络建立的时间(CARRIER_SET_UP)
				  					  //2015-5-25
				  					  //修改:为可远程设置的“上电时载波网络建立时间”
				  					  if (
				  					  	  0x00==(pFoundCpLink->status&0x1)    //被监测的线路原来为直流
				  					  	   && 0x01==(buf[21]&0x1)             //本次采集变为交流供电
				  					  	 )
				  					  {
            	 	    		if (FALSE == compareTwoAddr(pFoundCpLink->lddt1st, NULL, 1))
            	 	    		{
            	 	 	  			tmpCpLink = copyCtrl[4].cpLinkHead;
				    	  		 	 	  while(tmpCpLink!=NULL)
				    	  		 	 	  {
				    	  		 	 	   	if (compareTwoAddr(tmpCpLink->addr, pFoundCpLink->lddt1st, 0)==TRUE)
				    	  		 	 	   	{
				    	  		 	 	   	  if (debugInfo&PRINT_CARRIER_DEBUG)
				    	  		 	 	   	  {
					  		 	 	   	 		 		printf("%02d-%02d-%02d %02d:%02d:%02d:报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的第一末端单灯(%02x-%02x-%02x-%02x-%02x-%02x)的最近时间%02d-%02d-%02d %02d:%02d:%02d",
					  		 	 	   	 		       			sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
					  		 	 	   	 		       				pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
					  		 	 	   	 		       					tmpCpLink->addr[5],tmpCpLink->addr[4],tmpCpLink->addr[3],tmpCpLink->addr[2],tmpCpLink->addr[1],tmpCpLink->addr[0],
					  		 	 	   	 		       						tmpCpLink->lddtStatusTime.year,tmpCpLink->lddtStatusTime.month, tmpCpLink->lddtStatusTime.day, tmpCpLink->lddtStatusTime.hour, tmpCpLink->lddtStatusTime.minute, tmpCpLink->lddtStatusTime.second
					  		 	 	   	 		       		);
				    	  		 	 	   	  }
				    	  		 	 	   	 	 
				    	  		 	 	   	  //tmpCpLink->statusTime = nextTime(sysTime, pFoundCpLink->joinUp, 0);
				    	  		 	 	   	  tmpCpLink->lddtStatusTime = nextTime(sysTime, pFoundCpLink->joinUp, 0);    //2016-02-03,Modify
				    	  		 	 	   	  tmpCpLink->lddtRetry  = 0;
				    	  		 	 	   	 	 
				    	  		 	 	   	  if (debugInfo&PRINT_CARRIER_DEBUG)
				    	  		 	 	   	  {
				    	  		 	 	   	 	 	printf(",更新为%02d-%02d-%02d %02d:%02d:%02d\n",
				    	  		 	 	   	 	        tmpCpLink->lddtStatusTime.year,tmpCpLink->lddtStatusTime.month, tmpCpLink->lddtStatusTime.day, tmpCpLink->lddtStatusTime.hour, tmpCpLink->lddtStatusTime.minute, tmpCpLink->lddtStatusTime.second
				    	  		 	 	   	 		   );
				    	  		 	 	   	  }
				    	  		 	 	   	 	 
				    	  		 	 	   	   break;
				    	  		 	 	   	}
				    	  		 	 	   	 
				    	  		 	 	   	tmpCpLink = tmpCpLink->next;
				    	  		 	 	  }
				            	 	}
                    	 	 
                 	 	 
         	 	        		//第二末端单灯控制器地址不为全0
         	 	        		if (FALSE == compareTwoAddr(pFoundCpLink->collectorAddr, NULL, 1))
         	 	        		{
         	 	 	      			tmpCpLink = copyCtrl[4].cpLinkHead;
 	  		 	 	      				while(tmpCpLink!=NULL)
 	  		 	 	      				{
				 	  		 	 	   	    if (compareTwoAddr(tmpCpLink->addr, pFoundCpLink->collectorAddr, 0)==TRUE)
				 	  		 	 	   	    {
					 	  		 	 	   	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
					 	  		 	 	   	 	  {
					    	  		 	 	   	 	printf("%02d-%02d-%02d %02d:%02d:%02d:报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的第二末端单灯(%02x-%02x-%02x-%02x-%02x-%02x)的最近时间%02d-%02d-%02d %02d:%02d:%02d",
					 	  		 	 	   	 		       sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
					    	  		 	 	   	 	        pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
					 	  		 	 	   	 		         tmpCpLink->addr[5],tmpCpLink->addr[4],tmpCpLink->addr[3],tmpCpLink->addr[2],tmpCpLink->addr[1],tmpCpLink->addr[0],
					 	  		 	 	   	 		          tmpCpLink->lddtStatusTime.year,tmpCpLink->lddtStatusTime.month, tmpCpLink->lddtStatusTime.day, tmpCpLink->lddtStatusTime.hour, tmpCpLink->lddtStatusTime.minute, tmpCpLink->lddtStatusTime.second
					 	  		 	 	   	 		      );
					 	  		 	 	   	 	  }
	 	  		 	 	   	 		   
						  		 	 	   	 	  //tmpCpLink->statusTime = nextTime(sysTime, pFoundCpLink->joinUp, 0);
						  		 	 	   	 	  tmpCpLink->lddtStatusTime = nextTime(sysTime, pFoundCpLink->joinUp, 0);    //2016-02-03,modify
						  		 	 	   	 	  tmpCpLink->lddtRetry  = 0;
						  		 	 	   	 	 
						  		 	 	   	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
						  		 	 	   	 	  {
						  		 	 	   	 	    printf(",更新为%02d-%02d-%02d %02d:%02d:%02d\n",
						  		 	 	   	 		         tmpCpLink->lddtStatusTime.year,tmpCpLink->lddtStatusTime.month, tmpCpLink->lddtStatusTime.day, tmpCpLink->lddtStatusTime.hour, tmpCpLink->lddtStatusTime.minute, tmpCpLink->lddtStatusTime.second
						  		 	 	   	 		       );
						  		 	 	   	 	  }
					 	  		 	 	   	 		   
					 	  		 	 	   	 	  break;
					 	  		 	 	   	 	}

	    	  		 	 	   				tmpCpLink = tmpCpLink->next;
	 	  		 	 	   	  			}
	 	  		 	 	   				}
         	  		 	 	   	 
				 	  		 	 	   	//2015-11-09,Add
				 	  		 	 	   	//无线路控制器的情况下,用报警控制器采集的供电方式来加速单灯控制器灯开状态的采集
				  					  	if (xlcLink==NULL)    //无线路控制点
				  					  	{
				  					  	  //通知所有单灯线路已上电
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				         	  		 	while(tmpNode!=NULL)
				         	  		 	{
				 	  		 	 	      	tmpNode->lineOn = 1;

				 	  		 	 	      	tmpNode->statusTime = sysTime;        //最新状态时间改为当前时间
				 	  		 	 	      	tmpNode->lddtStatusTime = sysTime;    //最新状态时间改为当前时间,2016-02-03,Add

				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
									    	}
									  	}
          					  
				  					  //2015-3-11,
				  					  //现象:华伟集团1#线的报警控制器经过1个月的运行后发现多次线路刚断电时误报警一个线缆异常
				  					  //修改:线路由有交流电变为无流电时,如果正在紧急搜索末端单灯载波控制点,停止搜索
				  					  if (
				  					  	  0x01==(pFoundCpLink->status&0x1)    //上次采集到的是线路交流供电
				  					  	   && 0x00==(buf[21]&0x1)             //这次采集变为直流供电
				                            && 1==carrierFlagSet.searchLddt)  //正在搜索LDDT
				          		{	
                	 			//固定每1分钟查询一次
                	 			carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 1, 0);

                    		//恢复抄表
                        carrierFlagSet.workStatus = 6;
                        carrierFlagSet.reStartPause = 3;
                        copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);

                        carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                             
  					    				if (debugInfo&METER_DEBUG)
  					    				{
                          printf("%02d-%02d-%02d %02d:%02d:%02d,线路从交流切换为直流,停止搜索LDDT恢复抄读\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6]);
  					    				}
          			  		}
          					  
				  					  //2015-10-26,Add
				  					  //无线路控制器的情况下,用报警控制器采集的供电方式来为单灯控制器置为关灯状态
				  					  if (
				  					  	  0x01==(pFoundCpLink->status&0x1)    //上次采集到的是线路交流供电
				  					  	   && 0x00==(buf[21]&0x1)             //这次采集变为直流供电
				                         )
				          		{
				  					  	//无线路控制点
				  					  	if (xlcLink==NULL)
				  					  	{
				  					  	  //所有单灯应为关(亮度为0%)
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				 	  		 	 	      while(tmpNode!=NULL)
				 	  		 	 	      {
				 	  		 	 	      	tmpNode->status = 0;
				 	  		 	 	      	tmpNode->statusUpdated |= 1;
				 	  		 	 	      	
				 	  		 	 	      	tmpNode->lineOn = 0;    //载波线路断电,2015-11-24,添加这一句,
				 	  		 	 	      	                        //  因为在6905的1#线有报警控制器没有线路控制器,
				 	  		 	 	      	                        //  需要用报警控制器来置为载波线路断电,不然会有单灯控制离线的误报
				 	  		 	 	      	
				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
				  					  	}
				  					  }
				  					  
				  					  pFoundCpLink->statusTime = sysTime;    //LDGM没有时钟,取集中器当前时间

                      pFoundCpLink->status = buf[21];        //LDGM状态
                                                             // bit0-线路交流(1)供电还是直流(0)供电
                                                             // bit1-线路异常?1-异常,0-正常
                                                             // bit2-装置异常?
                                                             // bit3-直流输出已经超过一个判断周期(留出一个判断周期,避免给出错误的判断)
                                                             // bit4-置1表示直流已输出
                                                             // bit8-报警控制器类型1-载波方案,0-直流方案
                      //当前测量电流值
                      pFoundCpLink->duty[0] = buf[16];
                      pFoundCpLink->duty[1] = buf[17];
                      pFoundCpLink->duty[2] = buf[18];
                      
                      //当前测量电压值                         
                      pFoundCpLink->duty[3] = buf[19];
                      pFoundCpLink->duty[4] = buf[20];
                      
                      if (debugInfo&METER_DEBUG)
          			  		{
                        printf("%02d-%02d-%02d %02d:%02d:%02d,收到LDGM(%02x%02x%02x%02x%02x%02x)实时数据包:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                        printf("  -LDGM设定正常电流值:%5.2fA\n", (float)(buf[14] | buf[15]<<8)/100);
                        printf("  -LDGM当前检测电流值:%6.3fA\n", (float)(pFoundCpLink->duty[0] | pFoundCpLink->duty[1]<<8 | pFoundCpLink->duty[2]<<16)/1000);
                        printf("  -LDGM当前检测电压值:%5.1fV\n", (float)(pFoundCpLink->duty[3] | pFoundCpLink->duty[4]<<8)/10);
                        printf("  -LDGM当前供电方式:%s\n", (1==(pFoundCpLink->status&0x1))?"交流":"直流");
                        printf("  -LDGM当前是否故障:%d\n", pFoundCpLink->status>>1&0x1);
                        printf("  -LDGM装置是否故障:%d\n", pFoundCpLink->status>>2&0x1);
                        printf("  -LDGM超过一个判断周期:%d\n", pFoundCpLink->status>>3&0x1);
                        printf("  -LDGM直流输出:%d\n", pFoundCpLink->status>>4&0x1);
                      }
                    }
                    
                    if(
                       (0x00==(buf[21]&0x80))    //直流方案报警控制器
                    	 //startCurrent-线路正常低字节
                         //    ctrlTime-线路正常电流高字节
                    	 && (buf[14]!=copyCtrl[port].cpLinkHead->startCurrent || buf[15]!=copyCtrl[port].cpLinkHead->ctrlTime)
                      )
                    {
                      copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x02;
                      
                      if (debugInfo&METER_DEBUG)
                      {
                        printf("  -LDGM01线路正常电流值与主站设置的值不一致,申请校准\n");
                      }
                    }
                    else
                    {
                      if(
                    	  (0x80==(buf[21]&0x80))    //载波方案报警控制器
                    	   && (
                    	       compareTwoAddr(&buf[22], copyCtrl[port].cpLinkHead->lddt1st, 0)==FALSE
                    	        || compareTwoAddr(&buf[28], copyCtrl[port].cpLinkHead->collectorAddr, 0)==FALSE
                    	      )
                    	  )
                      {
                        copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x04;
                      
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("  -LDGM02的末端器的地址与主站设置的值不一致,申请校准\n");
                        }
                      }
                      else
                      {
                        //线路是直流供电、直流已输出、直流输出已过一个判断周期是才判断线路异常
                        if (
                        	 (0==(buf[21]&0x1))             //线路为直流供电
                        	  && (0x10==(buf[21]&0x10))     //直流已输出,2015-5-29,add
                        	   && (0x08==(buf[21]&0x08))    //直流输出已超过一个判断周期,2015-5-29,add
                        	)
                        {
                          searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3); //报警控制器与时间无关量
                        	
                          //报警控制器判断为线路异常,记录发生事件
                          if (buf[21]&0x02)
                          {
                    				//未记录的才记录
                    				if (0==(meterStatisExtranTimeS.mixed&0x02))
                    				{
                          	  //记录控制点故障信息(ERC13)
                          	  eventData[0] = 13;
                          	  eventData[1] = 11;
                          	  
                          	  tmpTime = timeHexToBcd(sysTime);
                          	  eventData[2] = tmpTime.second;
                          	  eventData[3] = tmpTime.minute;
                          	  eventData[4] = tmpTime.hour;
                          	  eventData[5] = tmpTime.day;
                          	  eventData[6] = tmpTime.month;
                          	  eventData[7] = tmpTime.year;
                          	  
                          	  eventData[8] = copyCtrl[port].cpLinkHead->mp&0xff;
                          	  eventData[9] = (copyCtrl[port].cpLinkHead->mp>>8&0xff) | 0x80;    //发生
                          	  eventData[10] = 0x20;
                          	  
                          	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                          	  
                          	  if (eventRecordConfig.nEvent[1]&0x10)
                          	  {
                          	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                          	  }
                          	  
                          	  eventStatus[1] = eventStatus[1] | 0x10;
                  					  
          					  				if (debugInfo&METER_DEBUG)
          					  				{
          					  					printf("  --记录控制点%d异常发生事件\n", copyCtrl[port].cpLinkHead->mp);
          					  				}
                              
                              activeReport3();    //主动上报事件
                          	  
                          	  meterStatisExtranTimeS.mixed |= 0x02;
                                
                              //存储控制点统计数据(与时间无关量)
                              saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                            }
                          }
                          else
                          {
                        		//已经发生过且记录过异常,记录恢复事件
	                        	if (
	                        		(meterStatisExtranTimeS.mixed&0x02)        //无电发生过
	                        		 || (meterStatisExtranTimeS.mixed&0x04)    //有电发生过
	                        	   )
	                        	{
                          	  //记录控制点故障信息(ERC13)
                          	  eventData[0] = 13;
                          	  eventData[1] = 11;
                          	  
                          	  tmpTime = timeHexToBcd(sysTime);
                          	  eventData[2] = tmpTime.second;
                          	  eventData[3] = tmpTime.minute;
                          	  eventData[4] = tmpTime.hour;
                          	  eventData[5] = tmpTime.day;
                          	  eventData[6] = tmpTime.month;
                          	  eventData[7] = tmpTime.year;
                          	  
                          	  eventData[8] = copyCtrl[port].cpLinkHead->mp&0xff;
                          	  eventData[9] = (copyCtrl[port].cpLinkHead->mp>>8&0xff) | 0x00;    //恢复
                          	  eventData[10] = 0x20;
                          	  
                          	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                          	  
                          	  if (eventRecordConfig.nEvent[1]&0x10)
                          	  {
                          	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                          	  }
                          	  
                          	  eventStatus[1] = eventStatus[1] | 0x10;
                  					  
				          					  if (debugInfo&METER_DEBUG)
				          					  {
				          					  	printf("  --记录控制点%d异常恢复事件\n", copyCtrl[port].cpLinkHead->mp);
				          					  }
				                              
                              activeReport3();    //主动上报事件
                              
			                			  if (meterStatisExtranTimeS.mixed&0x02)
			                			  {
			                			    meterStatisExtranTimeS.mixed &= 0xfd;
			                			  }
			                			
			                			  //2015-12-17
			                			  if (meterStatisExtranTimeS.mixed&0x04)
			                			  {
			                			    meterStatisExtranTimeS.mixed &= 0xfb;
			                			  }
                              
                              //存储控制点统计数据(与时间无关量)
                              saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                        		}
                          }
                        }

                        //每15分钟存储一次数据
                        if (
                        	//2015-12-28,因发现各报警控制点时有0点0分缺数现象,做这个处理看是否凑效
                        	((sysTime.minute<15) && (0==sysTime.minute || 1==sysTime.minute || 2==sysTime.minute))
                        	 || 
                        	  ((sysTime.minute>=15) && 0==(sysTime.minute%15))
                           )
                        {
                          memset(hourDataBuf, 0xee, LEN_OF_LIGHTING_FREEZE);
						  	          tmpTime = sysTime;
						  	          tmpTime.second = 0;
						  	          if (0==sysTime.minute || 1==sysTime.minute || 2==sysTime.minute)
						  	          {
						  	        		hourDataBuf[0] = 0;
						  	        		tmpTime.minute = 0;    //2015-01-06,add
						  	          }
						  	          else
						  	          {
						  	            hourDataBuf[0] = sysTime.minute;
						  	          }
						  	          hourDataBuf[1] = sysTime.hour;
						  	          hourDataBuf[2] = sysTime.day;
						  	          hourDataBuf[3] = sysTime.month;
						  	          hourDataBuf[4] = sysTime.year;
						  	        
						  	          //电压
						  	          tmpData = hexToBcd(buf[19] | buf[20]<<8);
						  	          hourDataBuf[5] = tmpData&0xff;
						  	          hourDataBuf[6] = tmpData>>8&0xff;
			 
						  	          //电流
						  	          tmpData = hexToBcd(buf[16] | buf[17]<<8 | buf[18]<<16);
						  	          hourDataBuf[7] = tmpData&0xff;
						  	          hourDataBuf[8] = tmpData>>8&0xff;
						  	          hourDataBuf[9] = tmpData>>16&0xff;
						  	        
						  	          //状态
						  	          hourDataBuf[28] = buf[21];
						  	          hourDataBuf[29] = 0x00;
			  	        
						  	          //线路交流供电时,如果载波判定为线缆异常也记录为线缆异常
						  	          if (0x1==(buf[21]&0x1))
						  	          {
			                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);    //报警控制器与时间无关量
			      				  	    if (0x4==(meterStatisExtranTimeS.mixed&0x4))
			      				  	    {
			      				  	      hourDataBuf[28] |= 0x02;
			      				  	        		
			      				  	      if (debugInfo&METER_DEBUG)
			      				  	      {
			                          printf("%02d-%02d-%02d %02d:%02d:%02d,报警控制点%d载波判定为异常记录进曲线数据中\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,copyCtrl[port].cpLinkHead->mp);
			                        }
			      				  	    }
			      				  	  }
      				  	        
      				  	  			saveMeterData(copyCtrl[port].cpLinkHead->mp, port, tmpTime, hourDataBuf, HOUR_FREEZE_SLC, 0x0, LEN_OF_LIGHTING_FREEZE);
      				  				}
  
                        copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x01;
                      }
                    }
                    
          					break;
          		  }
      				}
      				break;
      				
      		  case 0x94:
  						if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x04)
  						{
  				  		copyCtrl[port].cpLinkHead->flgOfAutoCopy &= 0xfb;    //清除修正末端器地址标志,并重新读取实时数据包

  				  		if (debugInfo&METER_DEBUG)
  				  		{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到LDGM(%02x%02x%02x%02x%02x%02x)修正末端器地址确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
      				}
      				else
      				{
  				  		copyCtrl[port].cpLinkHead->flgOfAutoCopy &= 0xfd;    //清除修正线路正常电流值标志,并重新读取实时数据包

  				  		if (debugInfo&METER_DEBUG)
  				  		{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,收到LDGM(%02x%02x%02x%02x%02x%02x)修正线路正常电流值确认\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
              }
      				break;
      				
      		  case 0xd1:    //异常应答
      				break;
      	  }
      	}
      }
      
      copyCtrl[port].copyContinue = TRUE;
      
      return;
    }
    
    //RS485接口上接的照度传感器
    if (port<4 && LIGHTING_LS==copyCtrl[port].cpLinkHead->protocol)
    {
      tmpi = CRC16(buf, len-2);
      if (buf[len-2]==(tmpi>>8&0xff) &&  buf[len-1]==(tmpi&0xff))
      {
        if (debugInfo&METER_DEBUG)
        {
          printf("CRC OK\n");
        }
        
      	pFoundCpLink = NULL;
      	tmpCpLink = lsLink;    		
      	while(tmpCpLink!=NULL)
      	{
      	  if (
      	 	   tmpCpLink->addr[0]==buf[0]
      	 		||0x0a==buf[0] 
      	 	 )
      	  {
      	  	pFoundCpLink = tmpCpLink;
      	   	break;
      	  }
      	  	
      	  tmpCpLink = tmpCpLink->next;
      	}

        if (1==copyCtrl[port].cpLinkHead->ctrlTime)    //能慧
        {
          tmpData = buf[3]<<24 | buf[4]<<16 | buf[5]<<8 | buf[6];
        }
        else    //风速风向
        {
          tmpData = (buf[5]<<8 | buf[6])*10;
        }
        
      	
        if (pFoundCpLink!=NULL)
        {
          lcProcess(pFoundCpLink->duty, tmpData);
          
          //2016-03-22,Add,将上一次值存放到上上次值位置
          pFoundCpLink->duty[3] = pFoundCpLink->duty[0];
          pFoundCpLink->duty[4] = pFoundCpLink->duty[1];
          pFoundCpLink->duty[5] = pFoundCpLink->duty[2];
          
          //当前值存放到上一次值位置
          pFoundCpLink->duty[0] = tmpData & 0xff;
          pFoundCpLink->duty[1] = tmpData>>8 & 0xff;
          pFoundCpLink->duty[2] = tmpData>>16 & 0xff;
        
          pFoundCpLink->statusTime = sysTime;

          if (debugInfo&METER_DEBUG)
          {
            printf("光照度=%d\n", tmpData);
            printf("%02d-%02d-%02d %02d:%02d:%02d,%02x%02x%02x%02x%02x%02x,光照度值=%d\n", 
                   sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                   pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
                   pFoundCpLink->duty[0] | pFoundCpLink->duty[1]<<8 | pFoundCpLink->duty[2]<<16
                  );
          }
        }
        
        copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x1;
				
      }
      
      copyCtrl[port].copyContinue = TRUE;
    	
      return;
    }

		//RS485接口上接的温湿度传感器
    if (port<4 && LIGHTING_TH==copyCtrl[port].cpLinkHead->protocol)
    {
      tmpi = CRC16(buf, len-2);
      if (buf[len-2]==(tmpi>>8&0xff) &&  buf[len-1]==(tmpi&0xff))
      {
        if (debugInfo&METER_DEBUG)
        {
          printf("TH CRC OK\n");
        }
        
      	pFoundCpLink = NULL;
      	tmpCpLink = thLink;    		
      	while(tmpCpLink!=NULL)
      	{
      	  if (tmpCpLink->addr[0]==buf[0])
      	  {
      	  	pFoundCpLink = tmpCpLink;
      	   	break;
      	  }
      	  	
      	  tmpCpLink = tmpCpLink->next;
      	}
      	
        if (pFoundCpLink!=NULL)
        { 
          //当前值存放到上一次值位置
          pFoundCpLink->duty[0] = buf[3];
          pFoundCpLink->duty[1] = buf[4];
          pFoundCpLink->duty[2] = buf[5];
          pFoundCpLink->duty[3] = buf[6];

					
          pFoundCpLink->statusTime = sysTime;

          if (debugInfo&METER_DEBUG)
          {
            printf("%02d-%02d-%02d %02d:%02d:%02d,%02x%02x%02x%02x%02x%02x,湿度=%d,温度=%d\n", 
                   sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                   pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
                   pFoundCpLink->duty[1] | pFoundCpLink->duty[0]<<8, bmToYm(&pFoundCpLink->duty[2], 2)
                  );
          }

					//每15分钟存储一次数据
					if (0==(sysTime.minute%15))
					{
						memset(hourDataBuf, 0xee, LEN_OF_LIGHTING_FREEZE);
						tmpTime = sysTime;
						tmpTime.second = 0;
						if (0==sysTime.minute || 1==sysTime.minute || 2==sysTime.minute)
						{
							hourDataBuf[0] = 0;
							tmpTime.minute = 0;
						}
						else
						{
							hourDataBuf[0] = sysTime.minute;
						}
						hourDataBuf[1] = sysTime.hour;
						hourDataBuf[2] = sysTime.day;
						hourDataBuf[3] = sysTime.month;
						hourDataBuf[4] = sysTime.year;
						
						//湿度
						hourDataBuf[28] = pFoundCpLink->duty[1];
						hourDataBuf[29] = pFoundCpLink->duty[0];
						//温度,负温度是用补码表示的
						tmpData = bmToYm(&pFoundCpLink->duty[2], 2);
						tmpData = hexToBcd(tmpData);
						if (pFoundCpLink->duty[2]&0x80)
						{
							tmpData |= 0x80;
						}
						hourDataBuf[26] =tmpData&0xff;
						hourDataBuf[27] = tmpData>>8&0xff;
						
						saveMeterData(copyCtrl[port].cpLinkHead->mp, port, tmpTime, hourDataBuf, HOUR_FREEZE_SLC, 0x0, LEN_OF_LIGHTING_FREEZE);

						if (debugInfo&METER_DEBUG)
						{
							printf("%02d-%02d-%02d %02d:%02d:%02d,保存温度湿度\n", 
										 sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second
										);
						}

					}

        }
        
        copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x1;
				
      }
      
      copyCtrl[port].copyContinue = TRUE;
    	
      return;
    }
  }
  
 #endif 
 
  switch(meterReceiving(port, buf, len))
  {
  	case METER_REPLY_ANALYSE_OK:
  	  if (debugInfo&METER_DEBUG)
  	  {
  	    //printf("PORT:%d接收数据帧正常且解析正确,已保存进缓存\n",port);
  	  }
  	  
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (port==PORT_POWER_CARRIER)
  	  {
        //路由主导抄表的话就不置抄表继续标志,等待路由请求
        if (carrierFlagSet.routeLeadCopy==0 || pDotCopy!=NULL)
        {
          copyCtrl[4].copyContinue = TRUE;
        }
        
  	    if (pDotCopy!=NULL)
  	    {
  	      if (pDotCopy->dotCopying==TRUE)
  	      {
            pDotCopy->dotRetry  = 0;
  	    		pDotCopy->dotResult = RESULT_HAS_DATA;
  	    		
    				if (copyCtrl[4].protocol == 2)
    				{
    		  		if (pDotCopy->dotRecvItem==0)
    		  		{
    						memset(pDotCopy->data+20, 0xee, 6);
    		  		}
    				}
    		
    				pDotCopy->dotRecvItem++;
    				if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
    				{
              meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //组帧发送
               
              switch (pDotCopy->dataFrom)
              {
                case DOT_CP_SINGLE_MP:   //按键单测量点点抄及时汇报测量结果
      	          singleMeterCopyReport(pDotCopy->data);
      	          break;
      	      }
  	    		}
  	      }
  	    }
  	  }
  	  else
  	  {
  	 #endif
  	 
  	    #ifdef RS485_1_USE_PORT_1
  	     copyCtrl[port-1].copyContinue = TRUE;
  	    #else
  	     copyCtrl[port].copyContinue = TRUE;
  	    #endif
  	    
  	    //12-09-20,add ABB 处理
  	    if (abbHandclasp[port-1]==1)
  	    {
  	      abbHandclasp[port-1] = 0;
  	    	 
  	      if (debugInfo&METER_DEBUG)
  	      {
  	    		printf("端口%d与ABB表已经握手成功\n", port);
  	      }
  	    }
  	    
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  }
  	 #endif
  	  break;
  	  	
  	case METER_NORMAL_REPLY:
  	  if (debugInfo&METER_DEBUG)
  	  {
  	    //printf("PORT:%d接收数据帧正常且表端正常应答(表端支持该命令)\n",port);
  	  }
  	 	
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (port==PORT_POWER_CARRIER)
  	  {
  	    copyCtrl[4].copyContinue = TRUE;
  	    
  	    if (pDotCopy!=NULL)
  	    {
	    	  if (pDotCopy->dotCopying==TRUE)
	    	  {
	    			pDotCopy->dotRecvItem++;
	    			if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
	    			{
	            meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //组帧发送
	    			}
	    			else
	    			{
	            switch (pDotCopy->dataFrom)
	            {
	              case DOT_CP_SINGLE_MP:   //按键单测量点点抄及时汇报测量结果
	      	        singleMeterCopyReport(pDotCopy->data);
	      	        break;
	      	    }
	  	    		   
	  	    	  pDotCopy->outTime = nextTime(sysTime, 0, 1);
	  	    	}
  	      }
  	    }
  	  }
  	  else
  	  {
  	 #endif
  	    
  	   #ifdef RS485_1_USE_PORT_1
  	    copyCtrl[port-1].copyContinue = TRUE;
  	    copyCtrl[port-1].retry        = 0;
  	   #else
  	    copyCtrl[port].copyContinue = TRUE;
  	    copyCtrl[port].retry        = 0;
  	   #endif  	    

  	    //12-09-20,add ABB 处理
  	    if (abbHandclasp[port-1]==1)
  	    {
    	   	abbHandclasp[port-1] = 0;
    	 
    	   	if (debugInfo&METER_DEBUG)
    	   	{
    	     	printf("端口%d与ABB表已经握手成功(METER_NORMAL_REPLY)\n", port);
    	   	}
  	    }

  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  }
  	 #endif
  	 	break;

  	case METER_ABERRANT_REPLY:
  		if (debugInfo&METER_DEBUG)
  		{
  	 	  //printf("PORT:%d接收数据帧正常,但表端异常应答(可能是表端不支持该命令)\n",port);
  	 	}
  	 
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (port>=PORT_POWER_CARRIER)
  	  {
  	    if (debugInfo&PRINT_CARRIER_DEBUG)
  	    {
  	    	printf("电表异常应答(可能表端不支持该命令)\n");
  	    }
  	    		
  	    //2014-01-08,添加这个判断
  	    if (0==carrierFlagSet.routeLeadCopy)
  	    {
  	      copyCtrl[4].copyContinue = TRUE;
  	    }
  	    if (pDotCopy!=NULL)
  	    {
  	      if (pDotCopy->dotCopying==TRUE)
  	      {
  	    	pDotCopy->dotRecvItem++;
  	    	pDotCopy->outTime = nextTime(sysTime, 0, 1);
  	      }
  	    }
  	  }
  	  else
  	  {
  	 #endif
  	   
  	    #ifdef RS485_1_USE_PORT_1
  	     copyCtrl[port-1].copyContinue = TRUE;
  	    #else
  	     copyCtrl[port].copyContinue = TRUE;
  	    #endif
  	    
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  }
  	 #endif
  	 	break;
  	 	
  	case 0:   //返回数据为0字节
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (debugInfo&PRINT_CARRIER_DEBUG)
  	  {
       	printf("%02d-%02d-%02d %02d:%02d:%02d,meter485Receive,载波/无线模块返回数据为0字节,port=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, port);
  	  }
  	  
  	  if (port==PORT_POWER_CARRIER)
  	  {
  	    if (pDotCopy!=NULL)
  	    {
  	      if (pDotCopy->dotCopying==TRUE)
  	      {
  	    		pDotCopy->dotRecvItem++;
  	    		pDotCopy->outTime = nextTime(sysTime, 0, 1);
           
            //2013-12-27,添加以下这几行处理
            pDotCopy->dotResult = RESULT_NO_REPLY;

       	    printf("%02d-%02d-%02d %02d:%02d:%02d,meter485Receive,点抄超时时间更新为:%02d-%02d-%02d %02d:%02d:%02d\n", 
       	            sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
       	             pDotCopy->outTime.year, pDotCopy->outTime.month, pDotCopy->outTime.day, pDotCopy->outTime.hour, pDotCopy->outTime.minute, pDotCopy->outTime.second
       	          );
  	    		
  	    		
  	    		if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
  	    		{
		      		if (debugInfo&PRINT_CARRIER_DEBUG)
		      		{
		        		printf("%02d-%02d-%02d %02d:%02d:%02d,meter485Receive,继续点抄下一个数据项,超时等待向后推移%d秒\n",
                       sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
		                		MONITOR_WAIT_ROUTE);
		      		}

              meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //组帧发送
              
              //2013-12-27,鼎信要求等待时间为90s以上
              //  因此将所有模块都统一到这个等待最长的时间上
              //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
              pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
  	    		}
  	      }
  	    }
        
  	    if (copyCtrl[4].meterCopying==TRUE)
  	    {
          if (carrierFlagSet.routeLeadCopy==0 || pDotCopy!=NULL)
          {
            copyCtrl[4].retry++;
          }
   	  	  copyCtrl[4].flagOfRetry = TRUE;
  	    }
  	  }

  	 #endif
  	  break;
  	  	
  	case RECV_DATA_OFFSET_ERROR:
  	 if (debugInfo&METER_DEBUG)
  	 {
  	   printf("PORT:%d存储偏移计算错误\n",port);
  	 }
  	 break;
  }
}

/***************************************************
函数名称:forwardDataReply
功能描述:转发回复
调用函数:
被调用函数:
输入参数:INT8U port,转发端口在抄表控制数组中的序号
输出参数:
返回值：void
修改历史:
    1.2014-01-03,修改过这个函数,原来转发无数据回复就不回主站了,在鼎信的要求下,转发无数据回复也回复主站0字节
***************************************************/
void forwardDataReply(INT8U portNum)
{
 #ifdef WDOG_USE_X_MEGA
  INT8U buf[1024];
 #endif

  INT16U frameTail10,tmpHead10;    //转发数据帧尾及帧起始指针
  INT16U tmpi;
  INT8U  checkSum;

  if (fQueue.tailPtr == 0)
  {
		tmpHead10 = 0;
  }
  else
  {
		tmpHead10 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
  }
 
  if (portNum<4)
  {
		msFrame[tmpHead10+18] = portNum+1;
  }
  else
  {
		msFrame[tmpHead10+18] = 31;
  }

  if (copyCtrl[portNum].pForwardData->fn==1)
  {
		frameTail10 = tmpHead10 + 21;
 
		if (copyCtrl[portNum].pForwardData->dataFrom==DOT_CP_SINGLE_MP)
		{
     #ifdef LIGHTING
	  	if (copyCtrl[portNum].pForwardData->forwardResult==RESULT_HAS_DATA)
	  	{
				if (copyCtrl[portNum].pForwardData->data[8]==0x9c)
				{ 
		  		xlOpenCloseReply("操作成功！");
				}
				else
				{
		  		xlOpenCloseReply("操作失败！");
				}
	  	}
	 	 	else
	  	{
	    	xlOpenCloseReply("操作超时！");
	  	}

			goto huifuRate;
	 	 #endif
		}
		
	 #ifdef LIGHTING
		else if (copyCtrl[portNum].pForwardData->dataFrom==DOT_CP_IR)
		{
			if (copyCtrl[portNum].pForwardData->forwardResult==RESULT_HAS_DATA)
			{
				irStudyReply("学习成功！");

				for(tmpi=copyCtrl[portNum].pForwardData->length; tmpi>0; tmpi--)
				{
					copyCtrl[portNum].pForwardData->data[tmpi+1] = copyCtrl[portNum].pForwardData->data[tmpi-1];
				}

				//长度
				copyCtrl[portNum].pForwardData->data[0] = copyCtrl[portNum].pForwardData->length&0xff;
				copyCtrl[portNum].pForwardData->data[1] = copyCtrl[portNum].pForwardData->length>>8&0xff;
				

				//printf("类型%d接收数据Length=%d:", copyCtrl[portNum].pForwardData->data[768], copyCtrl[portNum].pForwardData->length);
				//for(tmpi=0; tmpi<copyCtrl[portNum].pForwardData->length+2; tmpi++)
				//{
				//	printf("%02x ", copyCtrl[portNum].pForwardData->data[tmpi]);
				//}
				//printf("\n");

				//保存学习到的红外数据
				saveParameter(5, 160+copyCtrl[portNum].pForwardData->data[768]-4, copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length+2);
				
		 	}
		 	else
		 	{
				if (copyCtrl[portNum].pForwardData->data[768]>3)
				{
					irStudyReply("学习超时！");
				}
				else
				{
					irStudyReply("命令已发出！");					
				}
		 	}
			
			goto huifuRate;
		}
	 #endif
	 
		else if (copyCtrl[portNum].pForwardData->forwardResult==RESULT_HAS_DATA)
	  {
			if (debugInfo&METER_DEBUG)
			{
	  		printf("%02d-%02d-%02d %02d:%02d:%02d,转发有数据回复\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			}
	 
    	msFrame[tmpHead10+19] = copyCtrl[portNum].pForwardData->length&0xff;
    	msFrame[tmpHead10+20] = copyCtrl[portNum].pForwardData->length>>8&0xff;
    	memcpy(&msFrame[frameTail10],copyCtrl[portNum].pForwardData->data,copyCtrl[portNum].pForwardData->length);
    	frameTail10 += copyCtrl[portNum].pForwardData->length;
  	}
  	else
  	{
    	msFrame[tmpHead10+19] = 0x00;
    	msFrame[tmpHead10+20] = 0x00;
	
    	if (debugInfo&METER_DEBUG)
    	{
	  		printf("%02d-%02d-%02d %02d:%02d:%02d,转发无数据回复\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	}
    }
  }
  else   //FN==9
  {
		frameTail10 = tmpHead10 + 19;
 
		//转发目标地址
		if (copyCtrl[portNum].pForwardData->data[28]==0x81 || copyCtrl[portNum].pForwardData->data[28]==0x91 || copyCtrl[portNum].pForwardData->data[28]==0xb1)
		{
	  	memcpy(&msFrame[frameTail10], &copyCtrl[portNum].pForwardData->data[21], 0x06);
	  	frameTail10+=6;
	 
	  	//转发结果标志
	  	msFrame[frameTail10++] = 0x05;    //转发接收数据
	 
	  	//转发直接抄读的数据内容字节k+4
	  	if ((copyCtrl[portNum].pForwardData->data[6]&0x3)==0)   //DL/T645-1997
	  	{
	    	msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[29]+2;
	    	msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[30];
	    	msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[31];
	    	msFrame[frameTail10++] = 0x0;
	   		msFrame[frameTail10++] = 0x0;
				memcpy(&msFrame[frameTail10],&copyCtrl[portNum].pForwardData->data[32],copyCtrl[portNum].pForwardData->data[29]-2);
				frameTail10 += copyCtrl[portNum].pForwardData->data[29]-2;
	  	}
	  	else
	 	 	{
				msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[29];
				memcpy(&msFrame[frameTail10],&copyCtrl[portNum].pForwardData->data[30],copyCtrl[portNum].pForwardData->data[29]);
				frameTail10 += copyCtrl[portNum].pForwardData->data[29];
	  	}
		}
		else
		{
	  	memcpy(&msFrame[frameTail10],&copyCtrl[portNum].pForwardData->data[0],0x06);
	  	frameTail10+=6;

	  	if (copyCtrl[portNum].pForwardData->data[28]==0xc1 || copyCtrl[portNum].pForwardData->data[28]==0xd1)
	  	{
				//转发结果标志
				msFrame[frameTail10++] = 0x04;    //转发接收否认
      }
      else
      {
				//转发结果标志
				msFrame[frameTail10++] = 0x01;    //转发接收超时           	 
      }
		  msFrame[frameTail10++] = 4;
		  msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[7];
		  msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[8];
		  msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[9];
		  msFrame[frameTail10++] = copyCtrl[portNum].pForwardData->data[10];
		}
  }
 
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)
  {
		msFrame[frameTail10++] = iEventCounter;
		msFrame[frameTail10++] = nEventCounter;
  }
 
  //根据启动站要求判断是否携带TP
  if (frame.pTp != NULL)
  {
		msFrame[frameTail10++] = *frame.pTp;
		msFrame[frameTail10++] = *(frame.pTp+1);
		msFrame[frameTail10++] = *(frame.pTp+2);
		msFrame[frameTail10++] = *(frame.pTp+3);
		msFrame[frameTail10++] = *(frame.pTp+4);
		msFrame[frameTail10++] = *(frame.pTp+5);
  }
 
  msFrame[tmpHead10+0] = 0x68;   //帧起始字符

  tmpi = ((frameTail10 - tmpHead10 - 6) << 2) | PROTOCOL_FIELD;
  msFrame[tmpHead10+1] = tmpi & 0xFF;   //L
  msFrame[tmpHead10+2] = tmpi >> 8;
  msFrame[tmpHead10+3] = tmpi & 0xFF;   //L
  msFrame[tmpHead10+4] = tmpi >> 8; 
	 
  msFrame[tmpHead10+5] = 0x68;  //帧起始字符

  msFrame[tmpHead10+6] = 0x88;     //控制字节10001000(DIR=1,PRM=0,功能码=0x8)
			 
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
  {
	  msFrame[tmpHead10+6] |= 0x20;
  }

  //地址
  msFrame[tmpHead10+7] = addrField.a1[0];
  msFrame[tmpHead10+8] = addrField.a1[1];
  msFrame[tmpHead10+9] = addrField.a2[0];
  msFrame[tmpHead10+10] = addrField.a2[1];
  msFrame[tmpHead10+11] = addrField.a3;
		 
  msFrame[tmpHead10+12] = 0x10;  //AFN
											
  msFrame[tmpHead10+13] = 0x60 | rSeq;     //01100001 单帧
			 
  if (frame.pTp != NULL)
  {
		msFrame[tmpHead10+13] |= 0x80;       //TpV置位
  } 
 
  msFrame[tmpHead10+14] = 0x00;
  msFrame[tmpHead10+15] = 0x00;
  if (copyCtrl[portNum].pForwardData->fn==1)
  {
		msFrame[tmpHead10+16] = 0x01;
		msFrame[tmpHead10+17] = 0x00;
  }
  else    //FN==9
  {
		msFrame[tmpHead10+16] = 0x01;
		msFrame[tmpHead10+17] = 0x01;
  }
 
  checkSum = 0;
  for(tmpi = tmpHead10+6; tmpi < frameTail10;tmpi++)
  {
		checkSum += msFrame[tmpi];
  }
 
  msFrame[frameTail10++] = checkSum;
  msFrame[frameTail10++] = 0x16;

  fQueue.frame[fQueue.tailPtr].head = tmpHead10;
  fQueue.frame[fQueue.tailPtr].len  = frameTail10 - tmpHead10;

  if ((frameTail10+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
  {
		fQueue.frame[fQueue.tailPtr].next = 0x0;
		fQueue.tailPtr = 0;
  }
  else
  {
		fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
		fQueue.tailPtr++;
  }
  switch (copyCtrl[portNum].pForwardData->dataFrom)
  {
		case DATA_FROM_GPRS:
		
	  	if (debugInfo&METER_DEBUG)
	  	{
				printf("%02d-%02d-%02d %02d:%02d:%02d,准备发送转发回复数据\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
	  	}
		
	  	fQueue.inTimeFrameSend=TRUE;
	  	break;
					
    case DATA_FROM_LOCAL:
	 	 #ifdef WDOG_USE_X_MEGA
	  	buf[0] = fQueue.frame[fQueue.sendPtr].len&0xff;
	 	 	buf[1] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
	 	 	memcpy(&buf[2], &msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
	  	sendXmegaFrame(MAINTAIN_DATA, buf, fQueue.frame[fQueue.sendPtr].len+2);
	 	 #else
	  	sendLocalMsFrame(&msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
	 	 #endif
		 
	  	fQueue.sendPtr = fQueue.tailPtr;
	  	fQueue.thisStartPtr = fQueue.tailPtr;
	  	break;
  }

huifuRate:
  //恢复原端口属性
  if (copyCtrl[portNum].backupCtrlWord!=0)
  {
		switch (portNum)
		{
	 	 #ifdef RS485_1_USE_PORT_1 
	  	case 0x0:  //RS485-1
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x01;                                     //xMega端口1
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //恢复端口速率
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("恢复端口1速率\n");
				}
	   	 #else
				oneUartConfig(fdOfttyS2, copyCtrl[portNum].backupCtrlWord);
	   	 #endif
				break;
					
	  	case 0x1:  //RS485-2
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x02;                                     //xMega端口2
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //恢复端口速率
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
					
				if (debugInfo&METER_DEBUG)
				{
		  		printf("恢复端口2速率\n");
				}
	　 	 #else
				oneUartConfig(fdOfttyS3, copyCtrl[portNum].backupCtrlWord);
	   	 #endif
				break;

	  	case 0x2:  //RS485-3
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x03;                                     //xMega端口2
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //恢复端口速率
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("恢复端口3速率\n");
				}
	   	 #endif
	    	break;

	 	 #else
			
	  	case 0x1:  //RS485-1
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x01;                                     //xMega端口1
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //恢复端口速率
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("恢复端口1速率\n");
				}
	   	 #else
				oneUartConfig(fdOfttyS2, copyCtrl[portNum].backupCtrlWord);
	   	 #endif
	    	break;
					
	  	case 0x2:  //RS485-2
	   	#ifdef WDOG_USE_X_MEGA
				buf[0] = 0x02;                                     //xMega端口2
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //恢复端口速率
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("恢复端口2速率\n");
				}
	    #else
			 #ifdef SUPPORT_CASCADE
		 		oneUartConfig(fdOfttyS4, copyCtrl[portNum].backupCtrlWord);
		   #else 
		    oneUartConfig(fdOfttyS3, copyCtrl[portNum].backupCtrlWord);
		   #endif
	    #endif
				break;          

	   	case 0x3:  //RS485-3
			 #ifdef WDOG_USE_X_MEGA
		 		buf[0] = 0x03;                                     //xMega端口2
		 		buf[1] = copyCtrl[portNum].backupCtrlWord;         //恢复端口速率
		 		sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
		 		if (debugInfo&METER_DEBUG)
		 		{
		  		printf("恢复端口3速率\n");
		 		}
			 #endif
		 		break;

	   #endif
		}
  }

 #ifdef WDOG_USE_X_MEGA
  if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  {
		xMegaQueue.inTimeFrameSend = TRUE;
  }
 #endif
 
  free(copyCtrl[portNum].pForwardData);
  copyCtrl[portNum].pForwardData = NULL;
 
 //恢复抄表
 #ifdef PLUG_IN_CARRIER_MODULE
  if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE && portNum==4)
  {
		carrierFlagSet.workStatus = 6;
		carrierFlagSet.reStartPause = 3;
		copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);

		carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
		if (debugInfo&PRINT_CARRIER_DEBUG)
		{
	  	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:等待%d秒后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
		}
  }
 #endif
}

/***************************************************
函数名称:covertAcSample
功能描述:转换交采值为存储格式
调用函数:
被调用函数:
输入参数:buffer    - 参变量缓存
         visionBuf - 示值缓存
         reqBuf    - 需量缓存
输出参数:
返回值：void
***************************************************/
void covertAcSample(INT8U *buffer, INT8U *visionBuf, INT8U *reqBuf, INT8U type, DATE_TIME convertTime)
{
  #ifdef AC_SAMPLE_DEVICE
   INT16U offset;
   INT32U tmpData;
   INT32U tmpUaIa;
   INT8U  sign=0;
   INT8U  i, j;
   INT8U  energyVision[SIZE_OF_ENERGY_VISION];
	 INT32U visionInt;
	 INT16U visionDec;

   //转换参变量
   if (buffer!=NULL)
   {
     memset(buffer, 0xee, LENGTH_OF_PARA_RECORD);
     
     //功率(有功功率和无功功率)
     for (j = 0; j < 8; j++)
     {
     	 sign = 0;
     	 
     	 switch(j)
     	 {
     	 	  case 0:
     	 	  	offset = POWER_PHASE_A_WORK;
     	 	  	break;
     	 	  case 1:
     	 	  	offset = POWER_PHASE_B_WORK;
     	 	  	break;
     	 	  case 2:
     	 	  	offset = POWER_PHASE_C_WORK;
     	 	  	break;
     	 	  case 3:
     	 	  	offset = POWER_INSTANT_WORK;
     	 	  	break;
     	 	  case 4:
     	 	  	offset = POWER_PHASE_A_NO_WORK;
     	 	  	break;
     	 	  case 5:
     	 	  	offset = POWER_PHASE_B_NO_WORK;
     	 	  	break;
     	 	  case 6:
     	 	  	offset = POWER_PHASE_C_NO_WORK;
     	 	  	break;
     	 	  case 7:
     	 	  	offset = POWER_INSTANT_NO_WORK;
     	 	  	break;
     	 }
     	 tmpData = realAcData[j];
     	 
     	 //如果符号位为1,即为负数
     	 if (tmpData & 0x800000)
     	 {
     	 	  //将补码还原为原码值再进行计算
     	 	  tmpData = (~tmpData & 0x7fffff)+1;
     	 	  sign = 1;
     	 }
     	 
     	 if (j==3 || j==7)
     	 {
     	 	 tmpData = hexDivision(tmpData,times2(6)*1000,4);
     	 }
     	 else
     	 {
     	 	 tmpData = hexDivision(tmpData,times2(8)*1000,4);
     	 }
     	 
   	   buffer[offset]   = tmpData & 0xff;
   	   buffer[offset+1] = tmpData>>8 & 0xff;
   	   buffer[offset+2] = tmpData>>16 & 0xff;
   	   
   	   if (j<=3)
   	   {
   	     if (sign==1 && tmpData!=0)
   	     {
   	       buffer[offset+2] |= 0x80;
   	     }
   	   }
   	   else  //ly,2010-10-26,用科陆测试台测试时,无功功率全是反向的,因此,将无功功率反向过来
   	   {
   	     //2011-09-10,修改回来
   	     if (sign==1 && tmpData!=0)
   	     {
   	       buffer[offset+2] |= 0x80;
   	     }
   	   }
     }
  
     //电压
     offset = VOLTAGE_PHASE_A;
     
     if (debugInfo&PRINT_AC_SAMPLE)
     {
       printf("acSamplePara.vAjustTimes=%d\n",acSamplePara.vAjustTimes);
     }
     
     for (j = 8; j < 11; j++)
     {
     	  tmpData = hexDivision((realAcData[j]>>acSamplePara.vAjustTimes),times2(13),1);
        if (((tmpData>>12 & 0xf)*100 + (tmpData>>8 & 0xf)*10 + (tmpData>>4 & 0xf))<8)
        {
      	   tmpData = 0;
        }
   	    buffer[offset]   = tmpData & 0xff;
     	  buffer[offset+1] = tmpData>>8 & 0xff;
     	  offset += 2;
     }
  
     //电流
     if (debugInfo&PRINT_AC_SAMPLE)
     {
       printf("acSamplePara.cAjustTimes=%d\n",acSamplePara.cAjustTimes);
     }
  
     offset = CURRENT_PHASE_A;
     for (j = 11; j < 14; j++)
     {
       tmpData = hexDivision(((realAcData[j]&0x7fffff)>>acSamplePara.cAjustTimes),times2(13),3);
       
       buffer[offset]   = tmpData & 0xff;
       buffer[offset+1] = tmpData>>8 & 0xff;
       buffer[offset+2] = tmpData>>16 & 0xff;
       offset += 3;
     }
     
     //ly,2011-05-23,零序电流暂时填0
     //ly,2011-05-25,零序电流暂时用ABC相电流矢量和的有效值代替,因为零线上没有互感器
     tmpData = hexDivision(((realAcData[31]&0x7fffff)>>acSamplePara.cAjustTimes),times2(13),3);
     buffer[ZERO_SERIAL_CURRENT]   = tmpData & 0xff;
     buffer[ZERO_SERIAL_CURRENT+1] = tmpData>>8 & 0xff;
     buffer[ZERO_SERIAL_CURRENT+2] = tmpData>>16 & 0xff;
           	    
     //功率因数
     for (j = 14; j < 18; j++)
     {
     	 switch(j)
     	 {
     	 	  case 14:
     	 	  	offset = FACTOR_PHASE_A;
     	 	  	break;
     	 	  case 15:
     	 	  	offset = FACTOR_PHASE_B;
     	 	  	break;
     	 	  case 16:
     	 	  	offset = FACTOR_PHASE_C;
     	 	  	break;
     	 	  case 17:
     	 	  	offset = TOTAL_POWER_FACTOR;
     	 	  	break;
     	 }
     	
     	 tmpData = realAcData[j];
     	 
     	 //如果是负数,即符号位为1
     	 sign = 0;
     	 if (tmpData>>23 & 0x1)
     	 {
     	 	  //将补码还原为原码
     	 	  tmpData = (~tmpData & 0xffffff)+1;
     	 	  sign = 1;
     	 }
       tmpData = hexDivision(tmpData*100,times2(23),1);
  
   	   buffer[offset]   = tmpData & 0xff;
     	 buffer[offset+1] = tmpData>>8 & 0xff;
     	 
     	 //ly,2010-10-26,用科陆测试台测试时,标准表的功率因数全是正的(即无符合,因此,去掉加符号处理
    	 //if (sign==1)
     	 //{
     	 //	 buffer[offset+1] |= 0x80;
     	 //}
     }
     //视在功率
     for (j = 18; j < 22; j++)
     {
     	 sign = 0;
     	 
     	 switch(j)
     	 {
     	 	  case 18:
     	 	  	offset = POWER_PHASE_A_APPARENT;
     	 	  	break;
     	 	  case 19:
     	 	  	offset = POWER_PHASE_B_APPARENT;
     	 	  	break;
     	 	  case 20:
     	 	  	offset = POWER_PHASE_C_APPARENT;
     	 	  	break;
     	 	  case 21:
     	 	  	offset = POWER_INSTANT_APPARENT;
     	 	  	break;
     	 }
     	 tmpData = realAcData[j]&0x7fffff;
     	 
     	 if (j==21)
     	 {
     	 	 tmpData = hexDivision(tmpData,times2(6)*1000,4);
     	 }
     	 else
     	 {
     	 	 tmpData = hexDivision(tmpData,times2(8)*1000,4);
     	 }
     	 
   	   buffer[offset]   = tmpData & 0xff;
   	   buffer[offset+1] = tmpData>>8 & 0xff;
   	   buffer[offset+2] = tmpData>>16 & 0xff;
     }
     
     //电压相位角
     for (j = 26; j < 29; j++)
     {
     	 switch(j)
     	 {
     	 	  case 26:
     	 	  	offset = PHASE_ANGLE_V_A;
     	 	  	break;
     	 	  	
     	 	  case 27:
     	 	  	offset = PHASE_ANGLE_V_B;
     	 	  	break;
     	 	  	
     	 	  case 28:
     	 	  	offset = PHASE_ANGLE_V_C;
     	 	  	break;
     	 }
     	 
     	 tmpData = realAcData[j]&0x7fffff;
     	 
     	 if (j==26)
     	 {
     	 	 tmpData = 0;
     	 }
     	 else
     	 {
     	 	 tmpData = hexDivision(tmpData, times2(13), 1);
     	 }
     	 
   	   buffer[offset]   = tmpData & 0xff;
   	   buffer[offset+1] = tmpData>>8 & 0xff;
     }
     
     //电流相位角
     //∠Ia = ∠Ua(0) + ∠UaIa
     tmpData = realAcData[22];
     if (tmpData>>23 & 0x1)
     {
     	 	//将补码还原为原码
     	 	tmpData = (~tmpData & 0xffffff)+1;
     }
     tmpUaIa = tmpData/times2(23)*2*180*3*14159/100000;
     buffer[PHASE_ANGLE_C_A]   = (tmpUaIa%10)<<4;
     buffer[PHASE_ANGLE_C_A+1] = (tmpUaIa/100<<4) | (tmpUaIa%100/10);
     
     //∠Ib = ∠Uab + ∠UbIb-∠UaIa
     tmpData = realAcData[23];
     if (tmpData>>23 & 0x1)
     {
     	 	//将补码还原为原码
     	 	tmpData = (~tmpData & 0xffffff)+1;
     }
     tmpData = tmpData/times2(23)*2*180*3*14159/100000;
     tmpData = (realAcData[26]&0x7fffff)/times2(13) + tmpData - tmpUaIa;
     buffer[PHASE_ANGLE_C_B]   = (tmpData%10)<<4;
     buffer[PHASE_ANGLE_C_B+1] = (tmpData/100<<4) | (tmpData%100/10);
  
     //∠Ic = ∠Uac + ∠UcIc-∠UaIa
     tmpData = realAcData[24];
     if (tmpData>>23 & 0x1)
     {
     	 	//将补码还原为原码
     	 	tmpData = (~tmpData & 0xffffff)+1;
     }
     tmpData = tmpData/times2(23)*2*180*3*14159/100000;
     tmpData = (realAcData[28]&0x7fffff)/times2(13) + tmpData - tmpUaIa;
     buffer[PHASE_ANGLE_C_C]   = (tmpData%10)<<4;
     buffer[PHASE_ANGLE_C_C+1] = (tmpData/100<<4) | (tmpData%100/10);
   }
   
   //转换示值
   if (visionBuf!=NULL)
   {
   	 //读取交采示值转换前值
   	 if (type==1)
   	 {
		   readAcVision(energyVision, sysTime, ENERGY_DATA);
		 }
		 else
		 {
		   readAcVision(energyVision, convertTime, ENERGY_DATA);
		 }
		 
		 //正/反向有功、正/反向无功总
		 offset = POSITIVE_WORK_OFFSET;
		 for(i=0;i<8;i++)
		 {
		 	 for(j=0;j<5;j++)
		 	 {
		 	 	 visionInt = energyVision[i*25+j*5+0] | energyVision[i*25+j*5+1]<<8 | energyVision[i*25+j*5+2]<<16;
		 	 	 visionInt = hexToBcd(visionInt);
		 	 	 
		 	 	 visionDec = energyVision[i*25+j*5+3] | energyVision[i*25+j*5+4]<<8;
		 	 	 visionDec = visionDec/32;    //本来就是visionDec*100/3200,约掉100就是visionDec/32了
		 	 	 visionDec = hexToBcd(visionDec);
		 	 	 
		 	 	 visionBuf[offset+j*4+0] = visionDec;
		 	 	 visionBuf[offset+j*4+1] = visionInt&0xff;
		 	 	 visionBuf[offset+j*4+2] = visionInt>>8&0xff;
		 	 	 visionBuf[offset+j*4+3] = visionInt>>16&0xff;
		 	 }
		 	 
		 	 offset += 36;
		 }
		 
		 //A,B,C正/反向有功、正/反向无功
		 offset = POSITIVE_WORK_A_OFFSET;
		 for(i=0;i<12;i++)
		 {
		 	 visionInt = energyVision[POS_EPA+i*5+0] | energyVision[POS_EPA+i*5+1]<<8 | energyVision[POS_EPA+i*5+2]<<16;
		 	 visionInt = hexToBcd(visionInt);
		 	 	 
		 	 visionDec = energyVision[POS_EPA+i*5+3] | energyVision[POS_EPA+i*5+4]<<8;
		 	 visionDec = visionDec/32;    //本来就是visionDec*100/3200,约掉100就是visionDec/32了
		 	 visionDec = hexToBcd(visionDec);
		 	 	 
		 	 visionBuf[offset++] = visionDec;
		 	 visionBuf[offset++] = visionInt&0xff;
		 	 visionBuf[offset++] = visionInt>>8&0xff;
		 	 visionBuf[offset++] = visionInt>>16&0xff;
		 }		 
   }
   
   if (reqBuf!=NULL)
   {
   	 if (type==1)
   	 {
   	   memcpy(reqBuf, acReqTimeBuf, LENGTH_OF_REQ_RECORD);
   	 }
   	 else
   	 {
       readAcVision(reqBuf, convertTime, REQ_REQTIME_DATA);
   	 }
   }
  #endif
}

/***************************************************
函数名称:copyAcValue
功能描述:抄交采数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void copyAcValue(INT8U port, INT8U type, DATE_TIME copyTime)
{
  INT8U acParaData[LENGTH_OF_PARA_RECORD];
  INT8U visionBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U reqBuff[LENGTH_OF_REQ_RECORD];
  DATE_TIME tmpBackTime;
  
  memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);
  memset(reqBuff, 0xee, LENGTH_OF_REQ_RECORD);
  
  if (type==2)   //上月数据
  {
    tmpBackTime = sysTime;
    tmpBackTime.day    = 1;
    tmpBackTime.hour   = 0;
    tmpBackTime.minute = 0;
    tmpBackTime.minute = 0;
    tmpBackTime = backTime(tmpBackTime, 0, 1, 0, 0, 0);

    //转换
    covertAcSample(NULL, visionBuff, reqBuff, 2, tmpBackTime);
  
    //存储
    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, timeHexToBcd(sysTime), \
                  visionBuff, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, LENGTH_OF_ENERGY_RECORD);

    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, timeHexToBcd(sysTime), \
                  reqBuff, LAST_MONTH_DATA, REQ_REQTIME_LASTMONTH, LENGTH_OF_REQ_RECORD);
  }
  else
  {
    //转换
    covertAcSample(acParaData, visionBuff, reqBuff, 1, sysTime);
      
    //存储
    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, copyTime, \
                  acParaData, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, copyTime, \
                  visionBuff, PRESENT_DATA, ENERGY_DATA, LENGTH_OF_ENERGY_RECORD);
    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, copyTime, \
                  reqBuff, PRESENT_DATA, REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
  }
}

#ifdef PULSE_GATHER

/***************************************************
函数名称:fillPulseVar
功能描述:填充脉冲量采集变量
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void fillPulseVar(INT8U type)
{
   INT8U              i,j;
	 MEASURE_POINT_PARA tmpPointPara;
   struct timeval tv;

   for(i=0;i<NUM_OF_SWITCH_PULSE;i++)
   {
  	  if (type==1)
  	  {
         gettimeofday(&tv, NULL);
  	     
  	     pulse[i].pulseValue  = 0;
  	     pulse[i].pulseValueBak = 0;
  	     pulse[i].pulseValueBak2 = 0;
  	     pulse[i].prevMinutePulse = 0;

  	     //pulse[i].startPulse  = 0;
  	     //pulse[i].startPulse  = pulse[i].pulseCount;
  	     pulse[i].minutePulse = 0;
  	     //pulse[i].firstPulse  = 0;
  	     pulse[i].findPulse   = 0;

  	     pulse[i].calcTv      = tv;
  	  }

  	  //查找测量点、脉冲常数及脉冲特征
    	pulse[i].ifPlugIn = FALSE;
  	  for(j=0;j<pulseConfig.numOfPulse;j++)
  	  {
  	   	 if (pulseConfig.perPulseConfig[j].ifNo==i+1)
  	   	 {
  	   	  	pulse[i].ifPlugIn = TRUE;
  	   	  	pulse[i].pn = pulseConfig.perPulseConfig[j].pn;
  	   	  	pulse[i].character = pulseConfig.perPulseConfig[j].character;
  	   	  	pulse[i].meterConstant = pulseConfig.perPulseConfig[j].meterConstant[0] |  pulseConfig.perPulseConfig[j].meterConstant[1]<<8;
  	   	    break;
  	   	 }
  	  }
  	  
  	  if (pulse[i].ifPlugIn == TRUE)
  	  {
    	  //查找电流、电压互感器倍率
    	  pulse[i].voltageTimes = 1;   //如果没有配置测点基本参数,默认电压、电流互感器倍率为1
    	  pulse[i].currentTimes = 1;
    	  
	      if(selectViceParameter(0x04, 25, pulse[i].pn, (INT8U *)&tmpPointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
	      {
    	  	 pulse[i].voltageTimes = tmpPointPara.voltageTimes;
    	  	 pulse[i].currentTimes = tmpPointPara.currentTimes;
    	  }
    	}
   }
}

/***************************************************
函数名称:covertPulseData
功能描述:转换脉冲数据为存储格式
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void covertPulseData(INT8U port, INT8U *visionBuf,INT8U *needBuf,INT8U *paraBuff)
{
   INT32U visionInteger;                //示值整数
 	 INT32U visionDecimal;                //示值小数
   INT32U powerInteger;                 //功率整数
 	 INT32U powerDecimal;                 //功率小数
   INT32U needInteger;                  //需量整数
 	 INT32U needDecimal;                  //需量小数

 	 INT16U i;
 	   
 	 INT16U offsetVision,offsetPower,offsetNeed,offsetNeedTime;

	 //示值整数
	 visionInteger = pulseDataBuff[53*port] | pulseDataBuff[53*port+1]<<8
	               | pulseDataBuff[53*port+2]<<16;
   visionInteger = hexToBcd(visionInteger);

	 //示值小数(4位小数)
	 visionDecimal = (pulse[port].pulseCount*10000/pulse[port].meterConstant);
   visionDecimal = hexToBcd(visionDecimal);

   //功率整数
   powerInteger = pulse[port].prevMinutePulse*60/pulse[port].meterConstant;
   powerInteger = hexToBcd(powerInteger);
   
   //功率小数(4位小数)
   powerDecimal = pulse[port].prevMinutePulse*60%pulse[port].meterConstant*10000/pulse[port].meterConstant;
   powerDecimal = hexToBcd(powerDecimal);
  
   //需量整数
   needInteger = (pulseDataBuff[53*port+5] | pulseDataBuff[53*port+6]<<8)
           *60/pulse[port].meterConstant;
   needInteger = hexToBcd(needInteger);
   
   //需量小数(4位小数)
   needDecimal = (pulseDataBuff[53*port+5] | pulseDataBuff[53*port+6]<<8)
            *60%pulse[port].meterConstant*10000/pulse[port].meterConstant;
 	 needDecimal = hexToBcd(needDecimal);

   if (debugInfo&PRINT_PULSE_DEBUG)
   {
   	 printf("covertPulseData(%02d-%02d-%02d %02d:%02d:%02d):示值=%x.%04x,功率=%x.%04x,需量=%x.%04x\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute, sysTime.second, visionInteger,visionDecimal,powerInteger,powerDecimal,needInteger,needDecimal);
   }

 	 switch(pulse[port].character&0x3)
 	 {
 	 	  case 0:    //正向有功
 	 	  	offsetVision   = POSITIVE_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_WORK;
 	 	  	offsetNeed     = REQ_POSITIVE_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_P_WORK_OFFSET;
 	 	  	break;
 	 	  	
 	 	  case 1:    //正向无功
 	 	  	offsetVision   = POSITIVE_NO_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_NO_WORK;
 	 	  	offsetNeed     = REQ_POSITIVE_NO_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_P_NO_WORK_OFFSET;
 	 	  	break;
 	 	  	
 	 	  case 2:    //反向有功
 	 	  	offsetVision   = NEGTIVE_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_WORK;
 	 	  	offsetNeed     = REQ_NEGTIVE_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_N_WORK_OFFSET;
 	 	  	break;
 	 	  	
 	 	  case 3:    //反向无功
 	 	  	offsetVision   = NEGTIVE_NO_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_NO_WORK;
 	 	  	offsetNeed     = REQ_NEGTIVE_NO_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_N_NO_WORK_OFFSET;
 	 	  	break;
 	 }
   
   if (visionBuf!=NULL)
   {
     //示值
     visionBuf[offsetVision]   = visionDecimal&0xff;
     visionBuf[offsetVision+1] = visionDecimal>>8&0xff;
     visionBuf[offsetVision+2] = visionInteger&0xff;
     visionBuf[offsetVision+3] = visionInteger>>8&0xff;
     visionBuf[offsetVision+4] = visionInteger>>16&0xff;
   }
   
   if (paraBuff!=NULL)
   {  	
     //功率
     paraBuff[offsetPower]   = powerDecimal&0xff;
     paraBuff[offsetPower+1] = powerDecimal>>8&0xff;
     paraBuff[offsetPower+2] = powerInteger&0xff;
   }

   if (needBuf!=NULL)
   {
     //需量
     needBuf[offsetNeed]   = needDecimal&0xff;
     needBuf[offsetNeed+1] = needDecimal>>8&0xff;
     needBuf[offsetNeed+2] = needInteger&0xff;
  	
     //需量发生时间
     needBuf[offsetNeedTime]   = hexToBcd(pulseDataBuff[53*port+7])&0xff;
     needBuf[offsetNeedTime+1] = hexToBcd(pulseDataBuff[53*port+8])&0xff;
     needBuf[offsetNeedTime+2] = hexToBcd(pulseDataBuff[53*port+9])&0xff;
     needBuf[offsetNeedTime+3] = hexToBcd(pulseDataBuff[53*port+10])&0xff;
   }
   
   if (visionBuf!=NULL)
   {
     //各费率示值
     offsetVision += 5;

     for(i=0;i<periodTimeOfCharge[48];i++)
     {
	     //示值整数
	     visionInteger = pulseDataBuff[53*port+11+i*3] | pulseDataBuff[53*port+11+i*3+1]<<8
	                   | pulseDataBuff[53*port+11+i*3+2]<<16;
       visionInteger = hexToBcd(visionInteger);

	     //示值小数(2位小数)
	     visionDecimal = (pulse[port].pulseCountTariff[i]*10000/pulse[port].meterConstant);
       visionDecimal = hexToBcd(visionDecimal);
     
       visionBuf[offsetVision]   = visionDecimal&0xff;
       visionBuf[offsetVision+1] = visionDecimal>>8&0xff;
       visionBuf[offsetVision+2] = visionInteger&0xff;
       visionBuf[offsetVision+3] = visionInteger>>8&0xff;
       visionBuf[offsetVision+4] = visionInteger>>16&0xff;
     
       offsetVision += 5;
     }
   }
}

#endif

/***************************************************
函数名称:searchMpStatis
功能描述:搜索测量点统计数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void searchMpStatis(DATE_TIME searchTime, void *record, INT16U mp, INT8U type)
{
	 DATE_TIME                  tmpTime;
   METER_STATIS_EXTRAN_TIME   *meterStatisExtranTime;
   METER_STATIS_BEARON_TIME   *meterStatisBearonTime;
   METER_STATIS_EXTRAN_TIME_S *meterStatisExtranTimeS;
  #ifdef LIGHTING
   KZQ_STATIS_EXTRAN_TIME     *kzqStatisExtranTime;
  #endif

   //读出电表统计数据
   tmpTime = timeHexToBcd(searchTime);

   switch (type)
   {
     case 1:    //三相表与时间无关量
       meterStatisExtranTime = (METER_STATIS_EXTRAN_TIME *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         //printf("查找测量点%d统计数据(与时间无关量):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
  
       if (readMeterData((INT8U *)meterStatisExtranTime, mp, STATIS_DATA, 88, &tmpTime, 0)==FALSE)
       {
         	//如果没有该测量点统计数据,则应初始化测量点统计数据信息
       	  if (debugInfo&METER_DEBUG)
       	  {
            printf("无测量点统计数据(与时间无关量),初始化...\n");
          }
         	
         	initMpStatis(meterStatisExtranTime, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 { 
           //printf("有测量点统计数据(与时间无关量)\n");
         }
       }
       break;
     
     case 2:   //与时间有关量
       meterStatisBearonTime = (METER_STATIS_BEARON_TIME *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("查找当日测量点%d统计数据(抄表工况等):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
       
       if (readMeterData((INT8U *)meterStatisBearonTime, mp, STATIS_DATA, 0xfe, &tmpTime, 0)==FALSE)
       {
         	//如果没有当日的测量点统计数据,则应初始化测量点统计数据信息
         	initMpStatis(meterStatisBearonTime, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 { 
           printf("有当日测量点统计数据(抄表工况等)\n");
         }
       }
       break;
     
     case 3:   //单相表与时间无关量
       meterStatisExtranTimeS = (METER_STATIS_EXTRAN_TIME_S *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("查找测量点%d统计数据(单相表与时间无关量):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
  
       if (readMeterData((INT8U *)meterStatisExtranTimeS, mp, STATIS_DATA, 99, &tmpTime, 0)==FALSE)
       {
         	//如果没有该测量点统计数据,则应初始化测量点统计数据信息
       	  if (debugInfo&METER_DEBUG)
       	  { 
            printf("无测量点统计数据(单相表与时间无关量),初始化...\n");
          }
         	
         	initMpStatis(meterStatisExtranTimeS, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 {
           printf("有测量点统计数据(单相表与时间无关量)\n");
         }
       }
     	 break;
     	 
    #ifdef LIGHTING 
     case 4:    //线路测量点与时间无关量
       kzqStatisExtranTime = (KZQ_STATIS_EXTRAN_TIME *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("查找线路测量点%d统计数据(与时间无关量):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
  
       if (readMeterData((INT8U *)kzqStatisExtranTime, mp, STATIS_DATA, 89, &tmpTime, 0)==FALSE)
       {
         	//如果没有该线路测量点统计数据,则应初始化线路测量点统计数据信息
       	  if (debugInfo&METER_DEBUG)
       	  {
            printf("无线路测量点统计数据(与时间无关量),初始化...\n");
          }
         	
         	initMpStatis(kzqStatisExtranTime, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 {
           printf("有线路测量点统计数据(与时间无关量)\n");
         }
       }
       break;
    #endif

   }
}

/*******************************************************
函数名称:times2
功能描述:2的n次方
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:32位数据
*******************************************************/
INT32U times2(INT8U times)
{
   INT32U data;
   
   data = 1;
   if (times>0)
   {   	 
     for(;times>0;times--)
     {
   	    data *= 2;
     }
   }   
   return data;
}

/*******************************************************
函数名称:hexDivision
功能描述:两个32bit数据相除,实现给定精度的小数位数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:32位BCD数据(带deciBit位小数)
*******************************************************/
INT32U hexDivision(INT32U dividend, INT32U divisor, INT8U deciBit)
{
  INT32U i, maxDeci;
  INT32U ret;
  INT32U result = 0, remainder1 = 0, remainder2 = 0;

  result = dividend / divisor;
  remainder1 = dividend % divisor;

  maxDeci = 1;
  for (i = 0; i < deciBit; i++)
  {
     maxDeci *= 10;
     remainder1 *= 10;
  }

  remainder2 = remainder1 % divisor;
  remainder1 = remainder1 / divisor;

  if ((remainder2*10 / divisor) >= 8)
  {
    remainder1++;
  }

  if (remainder1 >= maxDeci)
  {
    remainder1 = 0;
    result++;
  }

  ret = hexToBcd(result)<<(deciBit*4) | hexToBcd(remainder1);

  return ret;
}

/***************************************************
函数名称:detectSwitchPulse
功能描述:检测开关量/脉冲
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void detectSwitchPulse(void *arg)
{
   INT8U  pulsei, i, j;
   INT8U  hasEvent=0;
   INT32U tmpPulseData;
   INT8U  tmpData;
   INT8U  stOfSwitchBak;
   struct timeval tv;
   
   #ifdef PULSE_GATHER
    INT8U visionBuff[LENGTH_OF_ENERGY_RECORD];
    INT8U reqBuff[LENGTH_OF_REQ_RECORD];
    INT8U paraBuff[LENGTH_OF_PARA_RECORD];

    INT8U pulseBufSave = 0;
    INT8U hasPulseSave = 0;
   #endif

	 struct cpAddrLink *tmpQueryMpLink;
	 INT16U tmpXlcPn;

   while(1)
   {
      //读出当前开关量值
      stOfSwitchRead = ioctl(fdOfIoChannel, READ_YX_VALUE, 0);
 
      //判断本次读取值
      reportToMain = DATA_FROM_GPRS;
      
      hasPulseSave = 0;
      pulseBufSave = 0;

      tmpSwitch = 1;
      for (pulsei = 0; pulsei < NUM_OF_SWITCH_PULSE; pulsei++)
      {
      	 #ifdef PULSE_GATHER
          if (pulse[pulsei].ifPlugIn==TRUE)
          {
            gettimeofday(&tv, NULL);
            if (secondChanged==TRUE && 
            	   ((sysTime.hour==0 && sysTime.minute==0 && sysTime.second==0) || (sysTime.hour==23 && sysTime.minute==59 && sysTime.second==58))
            	 )
            {
               if (debugInfo&PRINT_PULSE_DEBUG)
               {
                  //ly,2011-04-18
                  if (sysTime.hour==23)
                  {
                    printf("%02d-%02d-%02d %02d:%02d:%02d,秒=%d,微秒=%06d:每日日末保存脉冲端口%d当天最后一个示值、功率及需量\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec,pulsei+1);
                  }
                  else
                  {
                    printf("%02d-%02d-%02d %02d:%02d:%02d,秒=%d,微秒=%06d:每日0点保存脉冲端口%d当天第一个示值、功率及需量\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec,pulsei+1);
                  }
               }
               
               //检查三相电能表数据存储表是否存在
               checkSpRealTable(1);
               memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
               memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
               memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);

               //转换
               covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
         		 	
         		 	 //保存示值
         		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                            visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
         	   
         		 	 //保存参数
         		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                            paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);

         	     //保存需量
         		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                            reqBuff, PRESENT_DATA, REQ_REQTIME_DATA,LENGTH_OF_REQ_RECORD);
            }

            pulse[pulsei].pulseValue = stOfSwitchRead>>pulsei&0x1;            
            if (pulse[pulsei].pulseValue==0)  //下降沿
            {
               if (pulse[pulsei].pulseValueBak==1 || pulse[pulsei].pulseValueBak2==1)
               {
             	   if(pulse[pulsei].findPulse == 0 && pulse[pulsei].prevMinutePulse==0)
             	   {
                   if (pulse[pulsei].pulseCount==0 
                   	 && (pulseDataBuff[53*pulsei] | pulseDataBuff[53*pulsei+1]<<8 | pulseDataBuff[53*pulsei+2]<<16)==0)
                   {
                     //检查三相电能表数据存储表是否存在
                     checkSpRealTable(1);
                     
                     //2012-07-31,add,因为新台体软件测试老是日电量不合格
                     deletePresentData(pulse[pulsei].pn);

                     if (debugInfo&PRINT_PULSE_DEBUG)
                     {
                       printf("%02d-%02d-%02d %02d:%02d:%02d,秒=%d,微秒=%06d:删除实时数据后,保存脉冲端口%d初始示值、功率及需量\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec, pulsei+1);
                     }

                     memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
                     memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
                     memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);

                     //转换
                     covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
              		 	
              		 	 //保存示值
              		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                                 visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
              	   
              		 	 //保存参数
              		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                                 paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);

              	     //保存需量
              		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                                 reqBuff, PRESENT_DATA, REQ_REQTIME_DATA,LENGTH_OF_REQ_RECORD);
             	   	  
             	   	   pulse[pulsei].findPulse   = 1;
                     
     	   	           //pulse[pulsei].minutePulse = 0;
     	   	           //pulse[pulsei].startPulse  = pulse[pulsei].pulseCount;
     	   	         }
             	   }
             	   
             	   hasPulseSave = 1;

             	   pulse[pulsei].pulseCount++;
             	   pulse[pulsei].minutePulse++;
             	   
             	   if (sysTime.minute<30)
             	   {
             	   	 if (periodTimeOfCharge[sysTime.hour*2]<14)
             	   	 {
             	   	   pulse[pulsei].pulseCountTariff[periodTimeOfCharge[sysTime.hour*2]]++;
             	   	 }
             	   }
             	   else
             	   {
             	   	 if (periodTimeOfCharge[sysTime.hour*2+1]<14)
             	   	 {
             	   	   pulse[pulsei].pulseCountTariff[periodTimeOfCharge[sysTime.hour*2+1]]++;
             	   	 }
             	   }
                 
                 if (debugInfo&PRINT_PULSE_DEBUG)
                 {
                   printf("%02d-%02d-%02d %02d:%02d:%02d,秒=%d,微秒=%06d:第%d路脉冲个数=%04d,常数=%d,pulseBak=%d,pulseBak2=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec, pulsei+1, pulse[pulsei].pulseCount,pulse[pulsei].meterConstant,pulse[pulsei].pulseValueBak,pulse[pulsei].pulseValueBak2);
                 }
                 
             	   if (pulse[pulsei].pulseCount>=pulse[pulsei].meterConstant)
             	   {
             	   	 tmpPulseData = pulseDataBuff[53*pulsei] | pulseDataBuff[53*pulsei+1]<<8 | pulseDataBuff[53*pulsei+2]<<16;
             	   	 tmpPulseData++;
    	  	 	       pulseDataBuff[53*pulsei]   = tmpPulseData&0xff;
    	  	 	       pulseDataBuff[53*pulsei+1] = tmpPulseData>>8&0xff;
    	  	 	       pulseDataBuff[53*pulsei+2] = tmpPulseData>>16&0xff;
             	   	  
             	   	 //1 minute 平均功率中间量
             	   	 //pulse[pulsei].minutePulse += pulse[pulsei].pulseCount - pulse[pulsei].startPulse;
             	   	 //pulse[pulsei].startPulse = 0;
   
             	   	 pulse[pulsei].pulseCount = 0;
             	   	 
             	   	 pulseBufSave = 1;
             	   }
             	   
             	   for(i=0;i<14;i++)
             	   {
               	   if (pulse[pulsei].pulseCountTariff[i]>=pulse[pulsei].meterConstant)
               	   {
               	   	 tmpPulseData = pulseDataBuff[53*pulsei+11+i*3] | pulseDataBuff[53*pulsei+11+i*3+1]<<8 | pulseDataBuff[53*pulsei+11+i*3+2]<<16;
               	   	 tmpPulseData++;
      	  	 	       pulseDataBuff[53*pulsei+11+i*3]   = tmpPulseData&0xff;
      	  	 	       pulseDataBuff[53*pulsei+11+i*3+1] = tmpPulseData>>8&0xff;
      	  	 	       pulseDataBuff[53*pulsei+11+i*3+2] = tmpPulseData>>16&0xff;
     
               	   	 pulse[pulsei].pulseCountTariff[i] = 0;
               	   	 
             	   	   pulseBufSave = 1;
               	   }
             	   }
               }
               
               pulse[pulsei].pulseValueBak2 = 0;
               pulse[pulsei].pulseValueBak  = 0;
            }
            else
            {
              pulse[pulsei].pulseValueBak2 = pulse[pulsei].pulseValueBak;
              pulse[pulsei].pulseValueBak  = pulse[pulsei].pulseValue;
            }
            
       	    if ((tv.tv_sec>pulse[pulsei].calcTv.tv_sec && (tv.tv_sec-pulse[pulsei].calcTv.tv_sec)==60 && tv.tv_usec>=pulse[pulsei].calcTv.tv_usec)
       	    	   || (tv.tv_sec>pulse[pulsei].calcTv.tv_sec && (tv.tv_sec-pulse[pulsei].calcTv.tv_sec)>60)
       	    	    || (tv.tv_sec<pulse[pulsei].calcTv.tv_sec && (pulse[pulsei].calcTv.tv_sec - tv.tv_sec)>=60)
       	    	 )
       	    {
              pulse[pulsei].calcTv = tv;

       	   	  //计算1min平均功率
       	   	  //pulse[pulsei].prevMinutePulse = pulse[pulsei].minutePulse + pulse[pulsei].pulseCount-pulse[pulsei].startPulse;
       	   	  pulse[pulsei].prevMinutePulse = pulse[pulsei].minutePulse;
       	   	  pulse[pulsei].minutePulse = 0;
       	   	  //pulse[pulsei].startPulse  = pulse[pulsei].pulseCount;
       	   	  pulse[pulsei].findPulse   = 0;
              
              //需量与当前功率比较
      	   	  tmpPulseData = pulseDataBuff[53*pulsei+5] | pulseDataBuff[53*pulsei+6]<<8;
              if (pulse[pulsei].prevMinutePulse>tmpPulseData)
              {
              	 tmpPulseData = pulse[pulsei].prevMinutePulse;
                 
                 if (debugInfo&PRINT_PULSE_DEBUG)
                 {
                   printf("%02d-%02d-%02d %02d:%02d:%02d:第%d路需量=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pulsei+1, tmpPulseData);
                 }
              	 
              	 //更新最大需量
              	 pulseDataBuff[53*pulsei+5] = tmpPulseData&0xff;
    	  	 	     pulseDataBuff[53*pulsei+6] = tmpPulseData>>8&0xff;
              	  
              	 //更新需量发生时间
              	 pulseDataBuff[53*pulsei+7]  = sysTime.minute;
    	  	 	     pulseDataBuff[53*pulsei+8]  = sysTime.hour;
              	 pulseDataBuff[53*pulsei+9]  = sysTime.day;
    	  	 	     pulseDataBuff[53*pulsei+10] = sysTime.month;
    	  	 	     
    	  	 	     pulseBufSave = 1;
              }
              
              if (debugInfo&PRINT_PULSE_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d 秒=%d,微秒=%06d:第%d路平均功率=%d,脉冲个数=%d,常数=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pulse[pulsei].calcTv.tv_sec, pulse[pulsei].calcTv.tv_usec, pulsei+1, pulse[pulsei].prevMinutePulse,pulse[pulsei].pulseCount,pulse[pulsei].meterConstant);
              }
       	    }
          }
          else
          {
         #endif    //PULSE_GATHER
        	
        	 //如果配置了该路状态量
           if ((statusInput[0] & tmpSwitch) != 0)
           {
             if ((statusInput[1]&tmpSwitch)==0)   //动断触点(理解为常闭触点)
             {
             	 if ((stOfSwitchRead&tmpSwitch)==0) //如果IO脚为0
             	 {
             	 	 stOfSwitchRead |= tmpSwitch;     //动断触点应置为1
             	 }
             	 else                               //如果IO脚为1
             	 {
             	 	 stOfSwitchRead &= ~tmpSwitch;    //动断触点应置为0
             	 }
             }
             else                                 //动合触点(理解为常开触点)
             {
             	 ;
             }
           }
         
         #ifdef PULSE_GATHER
          }
         #endif     //PULSE_GATHER

         tmpSwitch <<= 1;
      }

 	   #ifdef PULSE_GATHER
      //脉冲量数据缓存
      if (pulseBufSave && !(flagOfClearPulse==0x55 || flagOfClearPulse==0xaa))
      {
 	      saveParameter(88, 3, pulseDataBuff, NUM_OF_SWITCH_PULSE*53);
 	      
        if (debugInfo&PRINT_PULSE_DEBUG)
        {
          printf("保存脉冲量数据缓存\n");
        }
      }
    
      //有脉冲保存
      if (hasPulseSave && !(flagOfClearPulse==0x55 || flagOfClearPulse==0xaa))
      {
 	      //保存脉冲计数
 	      saveParameter(88, 13, pulse, sizeof(ONE_PULSE)*NUM_OF_SWITCH_PULSE);
      }
     #endif
                	   
      stOfSwitchBaks[numOfStGather++] = stOfSwitchRead;

      //判断变位标志及事件产生
      //采用大数判决算法,采集10次后判断如果6次为1才是1,否则为0
      if (numOfStGather>=4)
      {
        numOfStGather = 0;
        
        stOfSwitchBak = 0;
        tmpSwitch = 1;
        tmpCd = 0;
        for(i=0;i<8;i++)
        {
        	tmpData = 0;
        	for(j=0;j<10;j++)
        	{
        	 	if (stOfSwitchBaks[j]&tmpSwitch)
        	 	{
        	 		 tmpData++;
        	 	}
        	}
        	
        	if (tmpData>2)
        	{
        		 stOfSwitchBak |= tmpSwitch; 
        	}

          //如果状态量发生变位,则记录变位事件
          if ((stOfSwitch & tmpSwitch) != (stOfSwitchBak & tmpSwitch) && (statusInput[0] & tmpSwitch) != 0)
          {
            tmpCd |= tmpSwitch;

					 #ifdef LIGHTING
					 
					  //2018-08-20,发现遥信有变化,立即上传心跳即上传遥信状态
					  nextHeartBeatTime = nextTime(sysTime, 0, 1);
					 
     				//2018-07-10,机房监控,入侵检测
						if (tmpSwitch==2)
					 	{
              if (selectParameter(4, 54, (INT8U*)&tmpXlcPn, 2)==FALSE)    //没有撤防控制点
              {
								if (selectParameter(4, 55, (INT8U*)&tmpXlcPn, 2)==TRUE)   //有布防控制点 
								{
									if (debugInfo&METER_DEBUG)
									{
							 			printf("有入侵信号,StOfSwitch=%02X,Pn=%d\n", stOfSwitchBak, tmpXlcPn);
									}

									tmpQueryMpLink = xlcLink;
									while (tmpQueryMpLink!=NULL)
									{
										if (tmpQueryMpLink->mp==tmpXlcPn)
										{
											break;
										}

										tmpQueryMpLink = tmpQueryMpLink->next;
									}

									if (tmpQueryMpLink!=NULL)
									{
										if ((stOfSwitchBak>>1&0x1)==1)
										{
											xlcForwardFrame(2, tmpQueryMpLink->addr, 2);
										}
										else
										{
											xlcForwardFrame(2, tmpQueryMpLink->addr, 0);
										}
									}
								}
              }
					 	}
             
            if (0==i)
            {
             	copyCtrl[1].nextCopyTime = sysTime;
            }
           #endif
          }
        	
        	tmpSwitch<<=1;
        }
        if (ifSystemStartup<2)
        {
        	ifSystemStartup++;
        	cdOfSwitch = tmpCd;
        }

        if (tmpCd != 0 && ifSystemStartup>=2)
        {
        	 cdOfSwitch = tmpCd;
           stOfSwitch = stOfSwitchBak;
        	 
        	 //2010-07-21,为适应科陆测试台加的这个判断,我认为正确的处理应该是不要这个if判断的,ly
        	 //if (stOfSwitchBak!=0x0)
        	 //{
        	 stStatusEvent(reportToMain);
        	 //}
        	 
           hasEvent = 1;
        }
        
        stOfSwitch = stOfSwitchBak;
      }

      
      if (hasEvent==1)
      {
        if (debugInfo&PRINT_EVENT_DEBUG)
        {
          printf("detectSwitchPulse 调用 主动上报\n");
        }
 
        activeReport3();    //主动上报事件
        
        hasEvent = 0;
      }

      //延时处理,避免采集得太快且需让出CPU执行时间片,如果需要加快或延迟采集速度,更改本语句即可
      usleep(500);
   }
}

/*******************************************************
函数名称:stStatusEvent
功能描述:状态量变位事件记录
调用函数:
被调用函数:
输入参数:  stOfSwitch--本次状态量值
           stOfSwitchBak--上次状态量值           
输出参数:
返回值:void
*******************************************************/
void stStatusEvent(INT8U reportToMain)
{
    INT8U  event[10];
    
    event[0] = 0x04;
    event[1] = 0x0A;
        
    event[2] = sysTime.second/10<<4 | sysTime.second%10;
    event[3] = sysTime.minute/10<<4 | sysTime.minute%10;
    event[4] = sysTime.hour/10<<4   | sysTime.hour%10;
    event[5] = sysTime.day/10<<4    | sysTime.day%10;
    event[6] = sysTime.month/10<<4  | sysTime.month%10;
    event[7] = sysTime.year/10<<4   | sysTime.year%10;
      
    event[8] = cdOfSwitch;          //状态变位
    event[9] = stOfSwitch;          //变位后标志
 	  if (eventRecordConfig.iEvent[0] & 0x08)
    {   
 	     writeEvent(event, 10, 1, reportToMain);  //记入重要事件队列
 	  }
    if (eventRecordConfig.nEvent[0] & 0x08)
    {
 	     writeEvent(event, 10, 2, DATA_FROM_LOCAL);  //记入一般事件队列
 	  }    
}

/*******************************************************
函数名称:meterTimeOverEvent
功能描述:电能表时间超差判断并事件记录
调用函数:
被调用函数:
输入参数:pn,测量点号
         timeBuff,存储时间和日期的缓存,0--3字节为日期和周次,4-6字节为时间
         mixOfStatisRecord,统计记录的mixed字节
输出参数:
返回值:void
*******************************************************/
BOOL meterTimeOverEvent(INT16U pn, INT8U *timeBuff, INT8U *mixOfStatisRecord, METER_STATIS_BEARON_TIME *statisBearon)
{
   INT8U     ifHasTimeEvent;
   INT8U     eventData[10];
   DATE_TIME tmpTime;
   
   if (timeBuff[0]!=0xEE && timeBuff[1]!=0xEE && timeBuff[2]!=0xEE      //日期
  	  && timeBuff[4]!=0xEE && timeBuff[5]!=0xEE && timeBuff[6]!=0xEE)   //时间
   {
     if ((eventRecordConfig.iEvent[1] & 0x08) || (eventRecordConfig.nEvent[1] & 0x08))
     {
       if (debugInfo&METER_DEBUG)
       {
         printf("测量点%d电能表时间超差标志位=%d\n",pn,*mixOfStatisRecord);
       }
       
       ifHasTimeEvent = 0;

       tmpTime.second = (timeBuff[4]&0xF) + ((timeBuff[4]>>4)&0xF)*10;
       tmpTime.minute = (timeBuff[5]&0xF) + ((timeBuff[5]>>4)&0xF)*10;
       tmpTime.hour   = (timeBuff[6]&0xF) + ((timeBuff[6]>>4)&0xF)*10;
       tmpTime.day    = (timeBuff[1]&0xF) + ((timeBuff[1]>>4)&0xF)*10;
       tmpTime.month  = (timeBuff[2]&0xF) + ((timeBuff[2]>>4)&0xF)*10;
       tmpTime.year   = (timeBuff[3]&0xF) + ((timeBuff[3]>>4)&0xF)*10;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("测量点%d电表时间为:%02d-%02d-%02d %02d:%02d:%02d\n",pn,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
                            
       if ((timeCompare(tmpTime, sysTime, meterGate.meterCheckTimeGate) == TRUE) 
       	  || (timeCompare(sysTime, tmpTime, meterGate.meterCheckTimeGate) == TRUE))
       {
          if ((*mixOfStatisRecord&0x1)==0x1)
          {
            ifHasTimeEvent = 0x2;   //超差恢复
            
            if (debugInfo&METER_DEBUG)
            {
              printf("测量点%d时间超差恢复\n",pn);
            }
            
            *mixOfStatisRecord &= 0xfe;
          }
       }
       else
       {
          if ((*mixOfStatisRecord&0x1)==0x0)
          {
            ifHasTimeEvent = 0x1;   //超差发生

            *mixOfStatisRecord &= 0xfe;
            *mixOfStatisRecord |= 0x01;

            if (debugInfo&METER_DEBUG)
            {
              printf("测量点%d时间超差发生\n",pn);
            }
          }
                    
         #ifdef CQDL_CSM 
          else
          {
          	//当天发生未记录
          	if ((statisBearon->mixed&0x1)==0x0)
          	{
              ifHasTimeEvent = 0x1;   //超差发生

              statisBearon->mixed |= 0x01;

              if (debugInfo&METER_DEBUG)
              {
                printf("测量点%d时间超差发生当天未记录,现在记录\n",pn);
              }
          	}
          	else
          	{
              if (debugInfo&METER_DEBUG)
              {
                printf("测量点%d时间超差发生当天已记录\n",pn);
              }          		 
          	}
          }
         #endif
       }
        
       if (ifHasTimeEvent==0x1 || ifHasTimeEvent==0x2)
       {
           eventData[0] = 12;  //ERC=12
           eventData[1] = 10;
         
           eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
           eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
           eventData[4] = sysTime.hour  /10<<4 | sysTime.hour  %10;
           eventData[5] = sysTime.day   /10<<4 | sysTime.day   %10;
           eventData[6] = sysTime.month /10<<4 | sysTime.month %10;
           eventData[7] = sysTime.year  /10<<4 | sysTime.year  %10;
           
           eventData[8] = pn;     //测量点低8位
           
           if (ifHasTimeEvent==0x1)
           {
             eventData[9] = ((pn>>8)&0xf) | 0x01<<7; //测量点高4位和起止标志
           }
           else
           {
             eventData[9] = ((pn>>8)&0xf) & 0x7f;    //测量点高4位和起止标志
           }
         
           if (eventRecordConfig.iEvent[1] & 0x08)
           {
             writeEvent(eventData, 10, 1, DATA_FROM_GPRS);
           }
           if (eventRecordConfig.nEvent[1] & 0x08)
           {
             writeEvent(eventData, 10, 2, DATA_FROM_LOCAL);
           }
           
           activeReport3();   //主动上报事件
           
           return TRUE;
       }
     }
   }
   
   return FALSE;
}

//***********************************公共函数********end*****************************************









//***********************************Q/GDW376.1国家电网终端***************************************

#ifdef PLUG_IN_CARRIER_MODULE
 struct copyCtrlWord     copyCtrl[NUM_OF_COPY_METER+2];       //抄表控制字
 INT8U                   carrierModuleType=NO_CARRIER_MODULE; //载波模块类型
 INT8U                   localModuleType=NO_CARRIER_MODULE;   //扩展载波模块类型(ly,2012-02-28,为了识别新版电科院模块而增加,因为新版和老版的处理不相同但又无明显数据项可查询)
 INT8U                   carrierAfn;                          //载波接收AFN
 INT8U                   carrierFn;                           //载波接收FN

 CARRIER_FLAG_SET        carrierFlagSet;                      //载波标志集

 struct carrierMeterInfo *carrierSlaveNode,*prevSlaveNode;
 struct carrierMeterInfo *noRecordMeterHead;                  //发现电表未记录指针

 DOT_COPY                *pDotCopy=NULL;                      //转发或点抄指针
#else
 struct copyCtrlWord     copyCtrl[NUM_OF_COPY_METER];         //抄表控制字
#endif

INT8U                    numOfCopyPort = 0;                   //共有几个抄表端口

//私有函数声明
BOOL  initCopyItem(INT8U iOfCp, INT8U ifCopyLastFreeze);
BOOL  initCopyDataBuff(INT8U iOfCp, INT8U initType);
INT8U queryIfInCopyPeriod(INT8U portNum);




#ifdef LIGHTING

struct ctrlTimes *cTimesHead=NULL;
struct cpAddrLink *xlcLink=NULL;
struct cpAddrLink *ldgmLink=NULL;
struct cpAddrLink *lsLink=NULL;
struct cpAddrLink *acLink=NULL;    //2015-12-05,add
struct cpAddrLink *thLink=NULL;    //2018-05-11,add

struct lightCtrl  lCtrl;    //光控标志集

/***************************************************
函数名称:slcFrame
功能描述:控制器命令组帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U slcFrame(INT8U *addr, INT8U *retFrame, INT8U type, struct cpAddrLink *pCpNow)
{
	struct ctrlTimes *tmpCTimes;
  MD5_CTX          context;
  INT8U            digest[16];
  INT8U            retLen = 0;
  INT8U            i;
  INT8U            tmpNum = 0;
  INT8U            tmpTail = 0;
	DATE_TIME        tmpTime;
  INT8U            tmpReadBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U            tmpHour;
  INT16U           tmpMp=0;
  INT8U            tmpGroup;
	INT8U            noOfTime;
  
  //2015-06-30,修改为该字节的低4位表示控制时段号,高4位是分组的组别
  tmpGroup = pCpNow->ctrlTime>>4;
  noOfTime = pCpNow->ctrlTime&0x0f;

 	retFrame[0] = 0x68;
 	memcpy(&retFrame[1], addr, 6);
 	retFrame[7] = 0x68;
	
 	retLen = 8;

  switch(type)
  {
  	case 1:    //广播校时
 	    retFrame[retLen++] = 0x08;    //C
 	    retFrame[retLen++] = 0x06;    //L
 	    retFrame[retLen++] = hexToBcd(sysTime.second)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.minute)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.hour)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.day)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.month)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.year)+0x33;
 	    break;
 	  
 	  case 2:    //校单灯控制器时段(组合校正)
 	    retFrame[retLen++] = 0x14;    //C
 	    
 	    //长度字节后面计算后填写
 	    retLen++;
 	     	    
 	    retFrame[retLen++] = 0x00+0x33;    //04010000
 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    
 	    retLen+=8;
 	    
 	    retLen++;    //这一字节填写有几个时间段

 	    tmpCTimes = cTimesHead;
 	    tmpNum = 0;    //时间段个数
 	    while(tmpCTimes!=NULL)
 	    {
 	    	//单灯控制器/线路控制器的控制时段号与给定的相同
 	    	if (tmpCTimes->noOfTime==noOfTime && 1==tmpCTimes->deviceType)
 	    	{
 	    	  retFrame[retLen++] = (tmpCTimes->startMonth | tmpCTimes->endMonth<<4)+0x33; 
 	    	  retFrame[retLen++] = tmpCTimes->startDay+0x33;
 	    	  retFrame[retLen++] = tmpCTimes->endDay+0x33;

 	    	  tmpTail = retLen++;
 	    	  
 	    	  for(i=0; i<6; i++)
 	    	  {
 	    	    if (0xff==tmpCTimes->hour[i])
 	    	    {
 	    	  	  retFrame[tmpTail] = i+0x33;
 	    	  	  break;
 	    	    }
 	    	    else
 	    	    {
 	    	      retFrame[retLen++] = tmpCTimes->hour[i]+0x33;
 	    	      retFrame[retLen++] = tmpCTimes->min[i]+0x33;
 	    	      retFrame[retLen++] = tmpCTimes->bright[i]+0x33;
 	    	  	}
 	    	  }
 	    	  if (i>=6)
 	    	  {
 	    	  	retFrame[tmpTail] = 6+0x33;
 	    	  }
 	    	
          tmpNum++;
        }
  	    if (2==type && *addr==0x99 && *(addr+1)==0x99 && *(addr+2)==0x99 && *(addr+3)==0x99 && *(addr+4)==0x99 && *(addr+5)==0x99)
   	    {
   	      //2014-07-17,由于广播命令发送字节限制,只能发送2个时段
   	      if (tmpNum>=2)
   	      {
   	      	break;
   	      }
   	    }
        
        tmpCTimes = tmpCTimes->next;        
   	  }
   	  
 	    retFrame[22] = tmpNum+0x33;
 	    
      //CCB发现灯故障后重试次数(0-3),2013-12-06,add
      retFrame[retLen++] = pnGate.failureRetry+0x33;
    
      //PWM频率
      retFrame[retLen++] = copyCtrl[4].tmpCpLink->bigAndLittleType+0x33;
    
      //亮度与占空比对比值
      for(i=0; i<6; i++)
      {
        retFrame[retLen++] = copyCtrl[4].tmpCpLink->collectorAddr[i]+0x33;
      } 	    
      for(i=0; i<6; i++)
      {
        retFrame[retLen++] = copyCtrl[4].tmpCpLink->duty[i]+0x33;
      }
 	    
 	    retFrame[9] = retLen-10;
 	    
 	    MD5Init(&context);
 	    MD5Update(&context, &retFrame[1], 13);
 	    MD5Update(&context, &retFrame[22], retFrame[9]-12);
      MD5Final (digest, &context);
      
      if (debugInfo&PRINT_CARRIER_DEBUG)
      { 
        printf("PA=");
        for(i=0; i<16; i++)
        {
      	  printf("%02x ", digest[i]);
        }
        printf("\n");
      }
      
 	    retFrame[14] = digest[0]+0x33;    //PA
 	    retFrame[15] = digest[15]+0x33;   //P0
 	    retFrame[16] = digest[2]+0x33;    //P1
 	    retFrame[17] = digest[13]+0x33;   //P2
 	    retFrame[18] = digest[4]+0x33;    //C0
 	    retFrame[19] = digest[11]+0x33;   //C1
 	    retFrame[20] = digest[6]+0x33;    //C2
 	    retFrame[21] = digest[9]+0x33;    //C3
 	    break;
 	  
 	  case 3:    //组合状态数据包
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //0090FF00,读取CCB组合数据包
 	    retFrame[retLen++] = 0xff+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;
 	    
 	  case 4:    //读取单灯控制器计量小时冻结数据
 	    retFrame[retLen++] = 0x11;          //C
 	    retLen++;                           //L
 	    retFrame[retLen++] = 0x00+0x33;     //0504FF00,读取计量小时冻结数据
 	    retFrame[retLen++] = 0xff+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    retFrame[retLen++] = 0x05+0x33;
 	    
 	    //2014-10-28,由于同时采集6个小时冻结的数据有185字节,需要传送10秒左右的时间,这载波主节点收不到,改成最多4个小时的数据
 	    retLen++;     //要采集的数据个数(最多4个)在后面填写

      tmpNum = 0;
      tmpHour = sysTime.hour;
      for(i=0; i<=tmpHour; i++)
      {
        tmpTime = sysTime;
        tmpTime.second = 0x0;
        tmpTime.minute = 0x0;
        tmpTime.hour = tmpHour - i;
        
        if (type==4)
        {
        	tmpMp = copyCtrl[4].tmpCpLink->mp;
        }
        else
        {
        	tmpMp = noOfTime;
        }
        
        //十进制日期
        if (readMeterData(tmpReadBuff, tmpMp, HOUR_FREEZE_SLC, 0x0, &tmpTime, 0) == TRUE)
        {
          if (tmpReadBuff[0] == 0xEE || tmpReadBuff[4] == 0xEE)
          {
          	if (debugInfo&PRINT_CARRIER_DEBUG)
          	{
          	  printf("控制点%d %d点整点冻结数据缺失\n", copyCtrl[4].tmpCpLink->mp, tmpHour - i);
          	}
          	 
          	retFrame[retLen++] = tmpTime.minute+0x33;
          	retFrame[retLen++] = tmpTime.hour+0x33;
          	retFrame[retLen++] = tmpTime.day+0x33;
          	retFrame[retLen++] = tmpTime.month+0x33;
          	retFrame[retLen++] = tmpTime.year+0x33;
          	  
          	tmpNum++;
          }
        }
        else
        {
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
          	printf("控制点%d无%d点整点冻结数据\n", copyCtrl[4].tmpCpLink->mp, tmpHour - i);
          }
          
          retFrame[retLen++] = tmpTime.minute+0x33;
          retFrame[retLen++] = tmpTime.hour+0x33;
          retFrame[retLen++] = tmpTime.day+0x33;
          retFrame[retLen++] = tmpTime.month+0x33;
          retFrame[retLen++] = tmpTime.year+0x33;
   
          tmpNum++;
        }
        
        if (tmpNum>=3)
        {
        	break;
        }
      }
      if (0==tmpNum)
      {
      	return 0;
      }
      
      retFrame[14] = tmpNum+0x33;
      retFrame[9]  = 5+tmpNum*5;
 	  	break;

 	  case 5:    //控制器当前状态和时间
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    
 	    //2016-01-26,修改本字节为光控根据检测到的照度计算的灯的亮度
 	    //2016-02-18,添加"|0x80",因张家港测试发现当光控延迟关灯时,主台一召测灯就关掉了,分析发现主台下发的这个字节是0,即是关灯的意思,因此需要区分
 	    retFrame[retLen++] = (lCtrl.lcDetectBright | 0x80) +0x33;     //00900000,读取状态和时间

 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	  case 6:    //控制器当前控制时段
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //00900100,读取控制时段
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	  case 7:    //控制开关调时刻和调光参数
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //00900300,读取开关调时刻和调光参数
 	    retFrame[retLen++] = 0x03+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;
 	    
 	  case 8:    //校单灯控制器时段(单独校时段)
 	    retFrame[retLen++] = 0x14;    //C
 	    
 	    //长度字节后面计算后填写
 	    retLen++;
 	     	    
			if((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)==REQUEST_SYN_CTRL_TIME_1)
			{
				retFrame[retLen++] = 0x01+0x33;     //04010001,小包1
			}
 	    else if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)==REQUEST_SYN_CTRL_TIME_2)
			{
			  retFrame[retLen++] = 0x03+0x33;     //04010003,小包2
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)==REQUEST_SYN_CTRL_TIME_3)
			{
			  retFrame[retLen++] = 0x04+0x33;     //04010004,小包3
			}
			else 
			{
			  retFrame[retLen++] = 0x01+0x33;     //04010001,小包1
			}
 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    
 	    retLen+=8;
 	    
 	    retLen++;    //这一字节填写有几个时间段

 	    tmpCTimes = cTimesHead;
      if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)==REQUEST_SYN_CTRL_TIME_1)
			{
				;
			}
			else if (
			    ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)==REQUEST_SYN_CTRL_TIME_2)
			     || ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)==REQUEST_SYN_CTRL_TIME_3)
				 )
			{
				//移位到第7或第13个时间段
				tmpNum = 0;
				while(tmpCTimes!=NULL)
				{
					//线路控制器的控制时段号与给定的相同
					if (
							tmpCTimes->noOfTime==noOfTime 
							 && 1==tmpCTimes->deviceType
						 )
					{
						tmpNum++;
						if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)==REQUEST_SYN_CTRL_TIME_2)
						{
						  if (tmpNum>=6)
							{
					      if (tmpCTimes!=NULL)
							  {
							    tmpCTimes = tmpCTimes->next;
							  }
							  break;
							}
						}
						else if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)==REQUEST_SYN_CTRL_TIME_3)
						{
							if (tmpNum>=13)
						  {
					      if (tmpCTimes!=NULL)
							  {
							    tmpCTimes = tmpCTimes->next;
							  }
							  break;
							}
						}
					}
					
					tmpCTimes = tmpCTimes->next;
				}
			}
			
 	    tmpNum = 0;    //时间段个数
 	    while(tmpCTimes!=NULL)
 	    {
 	    	//单灯控制器/线路控制器的控制时段号与给定的相同
 	    	if (tmpCTimes->noOfTime==noOfTime 
 	    		  && 1==tmpCTimes->deviceType
 	    		 )
 	    	{
 	    	  retFrame[retLen++] = (tmpCTimes->startMonth | tmpCTimes->endMonth<<4)+0x33; 
 	    	  retFrame[retLen++] = tmpCTimes->startDay+0x33;
 	    	  retFrame[retLen++] = tmpCTimes->endDay+0x33;
           
					//2016-11-14,Add,软件版本为2.0以上的单灯控制器添加启用日参数
					if (pCpNow->softVer>=2)
					{
					  retFrame[retLen++] = tmpCTimes->workDay+0x33;
					}

 	    	  tmpTail = retLen++;
 	    	  
 	    	  for(i=0; i<6; i++)
 	    	  {
 	    	    if (0xff==tmpCTimes->hour[i])
 	    	    {
 	    	  	  retFrame[tmpTail] = i+0x33;
 	    	  	  break;
 	    	    }
 	    	    else
 	    	    {
 	    	      retFrame[retLen++] = tmpCTimes->hour[i]+0x33;
 	    	      retFrame[retLen++] = tmpCTimes->min[i]+0x33;
 	    	      retFrame[retLen++] = tmpCTimes->bright[i]+0x33;
 	    	  	}
 	    	  }
 	    	  if (i>=6)
 	    	  {
 	    	  	retFrame[tmpTail] = 6+0x33;
 	    	  }
 	    	
          tmpNum++;
        }
  	    if (2==type && *addr==0x99 && *(addr+1)==0x99 && *(addr+2)==0x99 && *(addr+3)==0x99 && *(addr+4)==0x99 && *(addr+5)==0x99)
   	    {
   	      //2014-07-17,由于广播命令发送字节限制,只能发送2个时段
   	      if (tmpNum>=2)
   	      {
   	      	break;
   	      }
   	    }
 	      
				if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)==REQUEST_SYN_CTRL_TIME_1)
				{
   	      if (tmpNum>=6)
   	      {
   	      	break;
   	      }
				}
				else
				{
					if (
							(pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)==REQUEST_SYN_CTRL_TIME_2
							 || (pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)==REQUEST_SYN_CTRL_TIME_3
						 )
					{
						if (tmpNum>=7)
						{
							break;
						}
					}
				}
        
        tmpCTimes = tmpCTimes->next;        
   	  }   	  
 	    retFrame[22] = tmpNum+0x33;

 	    if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)==REQUEST_SYN_CTRL_TIME_1)
			{
				retFrame[retLen++] = ctrlMode+0x33;          //控制模式,2015-06-10,add
				retFrame[retLen++] = beforeOnOff[0]+0x33;    //光控开灯前多少分钟有效,2015-06-10,add
				retFrame[retLen++] = beforeOnOff[1]+0x33;    //光控开灯后多少分钟有效
				retFrame[retLen++] = beforeOnOff[2]+0x33;    //光控关灯前多少分钟有效
				retFrame[retLen++] = beforeOnOff[3]+0x33;    //光控关灯后多少分钟有效
				
				retFrame[retLen++] = tmpGroup+0x33;          //组别
			}
			
 	    retFrame[9] = retLen-10;
 	    
 	    MD5Init(&context);
 	    MD5Update(&context, &retFrame[1], 13);
 	    MD5Update(&context, &retFrame[22], retFrame[9]-12);
      MD5Final (digest, &context);
      
      if (debugInfo&PRINT_CARRIER_DEBUG)
      { 
        printf("单独校时段PA=");
        for(i=0; i<16; i++)
        {
      	  printf("%02x ", digest[i]);
        }
        printf("\n");
      }
      
 	    retFrame[14] = digest[0]+0x33;    //PA
 	    retFrame[15] = digest[15]+0x33;   //P0
 	    retFrame[16] = digest[2]+0x33;    //P1
 	    retFrame[17] = digest[13]+0x33;   //P2
 	    retFrame[18] = digest[4]+0x33;    //C0
 	    retFrame[19] = digest[11]+0x33;   //C1
 	    retFrame[20] = digest[6]+0x33;    //C2
 	    retFrame[21] = digest[9]+0x33;    //C3
 	    break;
 	    
 	  case 9:    //校单灯调光参数(单独校正)
 	    retFrame[retLen++] = 0x14;    //C
 	    
 	    //长度字节后面计算后填写
 	    retLen++;
 	     	    
 	    retFrame[retLen++] = 0x02+0x33;    //04010002
 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    
 	    retLen+=8;
 	    
      //CCB发现灯故障后重试次数(0-3),2013-12-06,add
      retFrame[retLen++] = pnGate.failureRetry+0x33;
    
      //PWM频率
      retFrame[retLen++] = copyCtrl[4].tmpCpLink->bigAndLittleType+0x33;

      //启动电流
      retFrame[retLen++] = copyCtrl[4].tmpCpLink->startCurrent+0x33;
    
      //亮度与占空比对比值
      for(i=0; i<6; i++)
      {
        retFrame[retLen++] = copyCtrl[4].tmpCpLink->collectorAddr[i]+0x33;
      } 	    
      for(i=0; i<6; i++)
      {
        retFrame[retLen++] = copyCtrl[4].tmpCpLink->duty[i]+0x33;
      }
 	    
 	    retFrame[9] = retLen-10;
 	    
 	    MD5Init(&context);
 	    MD5Update(&context, &retFrame[1], 13);
 	    MD5Update(&context, &retFrame[22], retFrame[9]-12);
      MD5Final (digest, &context);
      
      if (debugInfo&PRINT_CARRIER_DEBUG)
      { 
        printf("校调光参数PA=");
        for(i=0; i<16; i++)
        {
      	  printf("%02x ", digest[i]);
        }
        printf("\n");
      }
      
 	    retFrame[14] = digest[0]+0x33;    //PA
 	    retFrame[15] = digest[15]+0x33;   //P0
 	    retFrame[16] = digest[2]+0x33;    //P1
 	    retFrame[17] = digest[13]+0x33;   //P2
 	    retFrame[18] = digest[4]+0x33;    //C0
 	    retFrame[19] = digest[11]+0x33;   //C1
 	    retFrame[20] = digest[6]+0x33;    //C2
 	    retFrame[21] = digest[9]+0x33;    //C3
 	    break;
 	    
 	  case 10:    //控制器当前控制时段小包1
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    if ((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_1)==0x0)
			{
			  retFrame[retLen++] = 0x01+0x33;     //00900101,读取控制时段小包1
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_2)==0x0)
			{
			  retFrame[retLen++] = 0x02+0x33;     //00900102,读取控制时段小包2
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_3)==0x0)
			{
			  retFrame[retLen++] = 0x03+0x33;     //00900103,读取控制时段小包3
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_4)==0x0)
			{
			  retFrame[retLen++] = 0x04+0x33;     //00900104,读取控制时段小包4
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_5)==0x0)
			{
			  retFrame[retLen++] = 0x05+0x33;     //00900105,读取控制时段小包5
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_6)==0x0)
			{
			  retFrame[retLen++] = 0x06+0x33;     //00900106,读取控制时段小包6
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_7)==0x0)
			{
			  retFrame[retLen++] = 0x07+0x33;     //00900107,读取控制时段小包7
			}
			
 	    retFrame[retLen++] = 0x04+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	  case 11:    //控制器当前控制时段小包2
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x02+0x33;     //00900101,读取控制时段
 	    retFrame[retLen++] = 0x04+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;
 	  
 	  case 12:    //开灯命令
      retFrame[retLen++] = 0x1c;
    	retFrame[retLen++] = 0x10;
    	
    	retLen += 8;
    	
      retFrame[retLen++] = lCtrl.bright+0x33;    //N1,光控亮度
    	 
    	retFrame[retLen++] = ctrlMode+0x33;    //N2,控制模式
    	
    	//2015-06-16,暂定为一个小时以后
    	tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
    	 
    	//N3-N8,命令有效截止时间
      retFrame[retLen++] = tmpTime.second+0x33;
      retFrame[retLen++] = tmpTime.minute+0x33;
      retFrame[retLen++] = tmpTime.hour+0x33;
      retFrame[retLen++] = tmpTime.day+0x33;
      retFrame[retLen++] = tmpTime.month+0x33;
      retFrame[retLen++] = tmpTime.year+0x33;


      MD5Init(&context);
      MD5Update(&context, &retFrame[1], 9);
      MD5Update(&context, &retFrame[18], 8);
      MD5Final(digest, &context);
      
      if (debugInfo&METER_DEBUG)
      { 
        printf("PA=");
        for(i=0; i<16; i++)
        {
	        printf("%02x ", digest[i]);
        }
        printf("\n");
      }
      
      retFrame[10] = digest[0]+0x33;    //PA
      retFrame[11] = digest[15]+0x33;   //P0
      retFrame[12] = digest[2]+0x33;    //P1
      retFrame[13] = digest[13]+0x33;   //P2
      retFrame[14] = digest[4]+0x33;    //C0
      retFrame[15] = digest[11]+0x33;   //C1
      retFrame[16] = digest[6]+0x33;    //C2
      retFrame[17] = digest[9]+0x33;    //C3
 	    break;
			
 	  case 13:    //控制器软件版本
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //00900101,读取控制时段
 	    retFrame[retLen++] = 0x02+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	}
 	
  //CS
  retFrame[retLen] = 0x00;
  for(i=0; i<retLen; i++)
  {
  	retFrame[retLen] += retFrame[i];
  }
  retLen++;
  retFrame[retLen++] = 0x16;

 	return retLen;
}

/***************************************************
函数名称:xlcFrame
功能描述:线路控制器命令帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U xlcFrame(INT8U port)
{
	struct ctrlTimes  *tmpCTimes;
  MD5_CTX           context;
  INT8U             digest[16];
  INT8U             tmpNum = 0;
	INT16U            tmpData;
  INT8U             xlcSendBuf[250];
  INT8U             frameTail = 8;
  INT8U             tmpTail = 0;
  INT8U             i;
	DATE_TIME         tmpTime;
  INT8U             tmpReadBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U             tmpHour;
  struct cpAddrLink *tmpCpLink;
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //一个线路控制点统计事件数据(与时间无关量)
  
	//线路控制点与时间无关量
	searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);

 	xlcSendBuf[0] = 0x68;
 	memcpy(&xlcSendBuf[1], copyCtrl[port].cpLinkHead->addr, 6);
 	xlcSendBuf[7] = 0x68;

  if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CHECK_TIME)    //xlc需要校时
  {
 	  xlcSendBuf[frameTail++] = 0x08;    //C
 	  xlcSendBuf[frameTail++] = 0x06;    //L
 	  xlcSendBuf[frameTail++] = hexToBcd(sysTime.second)+0x33;
 	  xlcSendBuf[frameTail++] = hexToBcd(sysTime.minute)+0x33;
 	  xlcSendBuf[frameTail++] = hexToBcd(sysTime.hour)+0x33;
 	  xlcSendBuf[frameTail++] = hexToBcd(sysTime.day)+0x33;
 	  xlcSendBuf[frameTail++] = hexToBcd(sysTime.month)+0x33;
 	  xlcSendBuf[frameTail++] = hexToBcd(sysTime.year)+0x33;
  }
  else
  {
    if (
		    (
				 copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1        //xlc需要同步控制时段第1到10时间段
		      || copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2    //xlc需要同步控制时段第11到20时间段
			  )
		     && ((meterStatisExtranTimeS.mixed&0x20)==0x00)                          //参数未同步才下发
			 )
    {
 	    xlcSendBuf[frameTail++] = 0x14;    //C
 	    
 	    //长度字节后面计算后填写
 	    frameTail++;
 	     	    
      if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)
			{
 	      xlcSendBuf[frameTail++] = 0x01+0x33;    //04010001
		  }
			else
			{
 	      xlcSendBuf[frameTail++] = 0x03+0x33;    //04010003
			}
 	    xlcSendBuf[frameTail++] = 0x00+0x33;
 	    xlcSendBuf[frameTail++] = 0x01+0x33;
 	    xlcSendBuf[frameTail++] = 0x04+0x33;
 	    
 	    frameTail+=8;
 	    
 	    frameTail++;    //这一字节填写有几个时间段
 	    
 	    tmpCTimes = cTimesHead;
      if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)
			{
 	      ;
		  }
			else
			{
				//移位到后10个时间段
				tmpNum = 0;
				while(tmpCTimes!=NULL)
				{
					//线路控制器的控制时段号与给定的相同
					if (
							tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime 
							 && 
								 (
									(2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //控制模式为时段控或时段控结合光控
									 || (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //控制模式为经纬度控或经纬度控结合光控
								 )
						 )
					{
						tmpNum++;
						if (tmpNum>=10)
						{
					    if (tmpCTimes!=NULL)
							{
							  tmpCTimes = tmpCTimes->next;
							}
							break;
						}
					}
					
					tmpCTimes = tmpCTimes->next;
				}
			}
			
			
 	    tmpNum = 0;    //时间段个数
 	    while(tmpCTimes!=NULL)
 	    {
 	    	//线路控制器的控制时段号与给定的相同
 	    	if (
 	    		  tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime 
 	    		   && 
 	    		     (
 	    		      (2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //控制模式为时段控或时段控结合光控
 	    		       || (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //控制模式为经纬度控或经纬度控结合光控
 	    		     )
 	    		 )
 	    	{
 	    	  xlcSendBuf[frameTail++] = (tmpCTimes->startMonth | tmpCTimes->endMonth<<4)+0x33; 
 	    	  xlcSendBuf[frameTail++] = tmpCTimes->startDay+0x33;
 	    	  xlcSendBuf[frameTail++] = tmpCTimes->endDay+0x33;

 	    	  xlcSendBuf[frameTail++] = tmpCTimes->workDay+0x33;    //2016-8-19,Add,启用日

 	    	  tmpTail = frameTail++;
 	    	  
 	    	  for(i=0; i<6; i++)
 	    	  {
 	    	    if (0xff==tmpCTimes->hour[i])
 	    	    {
 	    	  	  xlcSendBuf[tmpTail] = i+0x33;
 	    	  	  break;
 	    	    }
 	    	    else
 	    	    {
 	    	      xlcSendBuf[frameTail++] = tmpCTimes->hour[i]+0x33;
 	    	      xlcSendBuf[frameTail++] = tmpCTimes->min[i]+0x33;
 	    	      xlcSendBuf[frameTail++] = tmpCTimes->bright[i]+0x33;
 	    	  	}
 	    	  }
 	    	  if (i>=6)
 	    	  {
 	    	  	xlcSendBuf[tmpTail] = 6+0x33;
 	    	  }
 	    	
          tmpNum++;
					
					//每次只发10个时间段,2016-11-02,否则会超过一个字节的长度
					if (tmpNum==10)
					{
						break;
					}
        }
        
        tmpCTimes = tmpCTimes->next;
   	  }
 	    xlcSendBuf[22] = tmpNum+0x33;
 	    
 	    //长度
			xlcSendBuf[9] = frameTail-10;
 	    
 	    MD5Init(&context);
 	    MD5Update(&context, &xlcSendBuf[1], 13);
 	    MD5Update(&context, &xlcSendBuf[22], xlcSendBuf[9]-12);
      MD5Final (digest, &context);
      
      if (debugInfo&METER_DEBUG)
      { 
        printf("PA=");
        for(i=0; i<16; i++)
        {
      	  printf("%02x ", digest[i]);
        }
        printf("\n");
      }
      
 	    xlcSendBuf[14] = digest[0]+0x33;    //PA
 	    xlcSendBuf[15] = digest[15]+0x33;   //P0
 	    xlcSendBuf[16] = digest[2]+0x33;    //P1
 	    xlcSendBuf[17] = digest[13]+0x33;   //P2
 	    xlcSendBuf[18] = digest[4]+0x33;    //C0
 	    xlcSendBuf[19] = digest[11]+0x33;   //C1
 	    xlcSendBuf[20] = digest[6]+0x33;    //C2
 	    xlcSendBuf[21] = digest[9]+0x33;    //C3
 	  }
    else
    {
			if (
					copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3    //xlc需要同步控制时段杂项参数
					 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)                     //参数未同步才下发
				 )
			{
				xlcSendBuf[frameTail++] = 0x14;    //C
				
				//长度字节后面计算后填写
				frameTail++;
							
				xlcSendBuf[frameTail++] = 0x04+0x33;    //04010004
				xlcSendBuf[frameTail++] = 0x00+0x33;
				xlcSendBuf[frameTail++] = 0x01+0x33;
				xlcSendBuf[frameTail++] = 0x04+0x33;
				
				frameTail+=8;
				
				//反馈遥信是否接入
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->joinUp+0x33;

				//2015-06-11,添加控制模式
				//2016-02-15,修改为设置的本控制点的控制模式
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->bigAndLittleType+0x33;
				
				//2015-06-26,添加光控提前-延迟有效分钟
				xlcSendBuf[frameTail++] = beforeOnOff[0]+0x33;
				xlcSendBuf[frameTail++] = beforeOnOff[1]+0x33;
				xlcSendBuf[frameTail++] = beforeOnOff[2]+0x33;
				xlcSendBuf[frameTail++] = beforeOnOff[3]+0x33;
				
				//2016-02-15,Add,经纬度
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[0]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[1]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[2]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[3]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[4]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[5]+0x33;

				//2016-03-17,Add,经纬度控制模式时的分/合闸微调分钟数
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->duty[4]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->duty[5]+0x33;
				
				//长度
				xlcSendBuf[9] = frameTail-10;
				
				MD5Init(&context);
				MD5Update(&context, &xlcSendBuf[1], 13);
				MD5Update(&context, &xlcSendBuf[22], xlcSendBuf[9]-12);
				MD5Final (digest, &context);
				
				if (debugInfo&METER_DEBUG)
				{ 
					printf("PA=");
					for(i=0; i<16; i++)
					{
						printf("%02x ", digest[i]);
					}
					printf("\n");
				}
				
				xlcSendBuf[14] = digest[0]+0x33;    //PA
				xlcSendBuf[15] = digest[15]+0x33;   //P0
				xlcSendBuf[16] = digest[2]+0x33;    //P1
				xlcSendBuf[17] = digest[13]+0x33;   //P2
				xlcSendBuf[18] = digest[4]+0x33;    //C0
				xlcSendBuf[19] = digest[11]+0x33;   //C1
				xlcSendBuf[20] = digest[6]+0x33;    //C2
				xlcSendBuf[21] = digest[9]+0x33;    //C3
			}
			else
			{
				if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CLOSE_GATE) || (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_OPEN_GATE))
				{
					xlcSendBuf[frameTail++] = 0x1c;
					xlcSendBuf[frameTail++] = 0x10;
					
					frameTail+= 8;
					
					if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CLOSE_GATE)
					{
						xlcSendBuf[frameTail++] = 1+0x33;    //N1
					}
					else
					{
						xlcSendBuf[frameTail++] = 0+0x33;    //N1
					}
					 
					//2016-08-25,修改为设置的本控制点的控制模式
					xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->bigAndLittleType+0x33;    //N2,控制模式
					
					//2015-06-12,暂定为一个小时以后
					tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
					 
					//N3-N8,命令有效截止时间
					xlcSendBuf[frameTail++] = tmpTime.second+0x33;
					xlcSendBuf[frameTail++] = tmpTime.minute+0x33;
					xlcSendBuf[frameTail++] = tmpTime.hour+0x33;
					xlcSendBuf[frameTail++] = tmpTime.day+0x33;
					xlcSendBuf[frameTail++] = tmpTime.month+0x33;
					xlcSendBuf[frameTail++] = tmpTime.year+0x33;
		

					MD5Init(&context);
					MD5Update(&context, &xlcSendBuf[1], 9);
					MD5Update(&context, &xlcSendBuf[18], 8);
					MD5Final(digest, &context);
					
					if (debugInfo&METER_DEBUG)
					{ 
						printf("PA=");
						for(i=0; i<16; i++)
						{
							printf("%02x ", digest[i]);
						}
						printf("\n");
					}
					
					xlcSendBuf[10] = digest[0]+0x33;    //PA
					xlcSendBuf[11] = digest[15]+0x33;   //P0
					xlcSendBuf[12] = digest[2]+0x33;    //P1
					xlcSendBuf[13] = digest[13]+0x33;   //P2
					xlcSendBuf[14] = digest[4]+0x33;    //C0
					xlcSendBuf[15] = digest[11]+0x33;   //C1
					xlcSendBuf[16] = digest[6]+0x33;    //C2
					xlcSendBuf[17] = digest[9]+0x33;    //C3
				}
				else
				{
					if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)==0x0)    //查询数据包还未发送
					{
						xlcSendBuf[frameTail++] = 0x11;    //C
						xlcSendBuf[frameTail++] = 0x04;    //L
						xlcSendBuf[frameTail]   = 0x31;    //0090FF00,读取组合状态数据包
						//2016-01-22,这个字节改成光控要求分闸或是合闸标志
						tmpCpLink = xlcLink;
						while(tmpCpLink!=NULL)
						{
							if (tmpCpLink->mp==copyCtrl[port].cpLinkHead->mp)
							{
								xlcSendBuf[frameTail] = tmpCpLink->lcDetectOnOff+0x33;
								
								break;
							}
							
							tmpCpLink = tmpCpLink->next;
						}
						frameTail++;
						
						xlcSendBuf[frameTail++] = 0xff+0x33;
						xlcSendBuf[frameTail++] = 0x90+0x33;
						xlcSendBuf[frameTail++] = 0x00+0x33;
					}
					else
					{
						//查询控制时段小包1还未发送
						if (
								(copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CTRL_TIME_1)==0x0
									&& ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //参数未同步才下发
							 )
						{
							xlcSendBuf[frameTail++] = 0x11;    //C
							xlcSendBuf[frameTail++] = 0x04;    //L
							xlcSendBuf[frameTail++] = 0x01+0x33;
							xlcSendBuf[frameTail++] = 0x04+0x33;
							xlcSendBuf[frameTail++] = 0x90+0x33;
							xlcSendBuf[frameTail++] = 0x00+0x33;
						}
						else
						{
							//查询控制时段小包2还未发送
							if (
									(copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CTRL_TIME_2)==0x0
									 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //参数未同步才下发
								 )
							{
								xlcSendBuf[frameTail++] = 0x11;    //C
								xlcSendBuf[frameTail++] = 0x04;    //L
								xlcSendBuf[frameTail++] = 0x02+0x33;
								xlcSendBuf[frameTail++] = 0x04+0x33;
								xlcSendBuf[frameTail++] = 0x90+0x33;
								xlcSendBuf[frameTail++] = 0x00+0x33;
							}
							else
							{
								//xlc的小时冻结数据
								if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_HOUR_FREEZE)==0x0)
								{
									xlcSendBuf[frameTail++] = 0x11;         //C
									frameTail++;                            //L
									xlcSendBuf[frameTail++] = 0x00+0x33;    //0504FF00,读取计量小时冻结数据
									xlcSendBuf[frameTail++] = 0xff+0x33;
									xlcSendBuf[frameTail++] = 0x04+0x33;
									xlcSendBuf[frameTail++] = 0x05+0x33;
									
									//同时采集6个小时冻结的数据有185字节
									frameTail++;     //要采集的数据个数(最多6个)在后面填写
						
									tmpNum = 0;
									tmpHour = sysTime.hour;
									for(i=0; i<=tmpHour; i++)
									{
										tmpTime = sysTime;
										tmpTime.second = 0x0;
										tmpTime.minute = 0x0;
										tmpTime.hour = tmpHour - i;
										
										//十进制日期
										if (readMeterData(tmpReadBuff, copyCtrl[port].cpLinkHead->mp, HOUR_FREEZE_SLC, 0x0, &tmpTime, 0) == TRUE)
										{
											if (tmpReadBuff[0] == 0xEE || tmpReadBuff[4] == 0xEE)
											{
												if (debugInfo&METER_DEBUG)
												{
													printf("控制点%d %d点整点冻结数据缺失\n", copyCtrl[port].cpLinkHead->mp, tmpHour - i);
												}
												 
												xlcSendBuf[frameTail++] = tmpTime.minute+0x33;
												xlcSendBuf[frameTail++] = tmpTime.hour+0x33;
												xlcSendBuf[frameTail++] = tmpTime.day+0x33;
												xlcSendBuf[frameTail++] = tmpTime.month+0x33;
												xlcSendBuf[frameTail++] = tmpTime.year+0x33;
													
												tmpNum++;
											}
										}
										else
										{
											if (debugInfo&METER_DEBUG)
											{
												printf("控制点%d无%d点整点冻结数据\n", copyCtrl[port].cpLinkHead->mp, tmpHour - i);
											}
											
											xlcSendBuf[frameTail++] = tmpTime.minute+0x33;
											xlcSendBuf[frameTail++] = tmpTime.hour+0x33;
											xlcSendBuf[frameTail++] = tmpTime.day+0x33;
											xlcSendBuf[frameTail++] = tmpTime.month+0x33;
											xlcSendBuf[frameTail++] = tmpTime.year+0x33;
							 
											tmpNum++;
										}
										
										if (tmpNum>=6)
										{
											break;
										}
									}
									
									if (0==tmpNum)
									{
										goto copyOk;
									}
									
									xlcSendBuf[14] = tmpNum+0x33;
									xlcSendBuf[9]  = 5+tmpNum*5;
								}
								else
								{

			copyOk:
									if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)    //查询数据包已经返回
									{
										return COPY_COMPLETE_NOT_SAVE;
									}
								}
							}
						}
					}
				}
			}
    }
  }
  
  if (frameTail>8)
  {
    //CS
    xlcSendBuf[frameTail] = 0x00;
    for(i=0; i<frameTail; i++)
    {
  	  xlcSendBuf[frameTail] += xlcSendBuf[i];
    }
    frameTail++;
    xlcSendBuf[frameTail++] = 0x16;
    
    sendMeterFrame(port, xlcSendBuf, frameTail);
  }
  
  return 0;
}

/***************************************************
函数名称:xlcForwardFrame
功能描述:线路控制器转发命令帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
BOOL xlcForwardFrame(INT8U port, INT8U *addr, INT8U openClose)
{
  MD5_CTX   context;
  INT8U     digest[16];
	INT8U     frameTail = 8;
  DATE_TIME tmpTime;
  INT8U     i;

	printf("openClose=%d\n", openClose);
	
  //如果该端口当前未处理转发
  if (copyCtrl[port].pForwardData==NULL)
  {
		copyCtrl[port].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
		copyCtrl[port].pForwardData->fn            = 1;
		copyCtrl[port].pForwardData->dataFrom      = DOT_CP_SINGLE_MP;
		copyCtrl[port].pForwardData->ifSend        = FALSE;
		copyCtrl[port].pForwardData->receivedBytes = FALSE;
		copyCtrl[port].pForwardData->forwardResult = RESULT_NONE;
 
		//透明转发通信控制字
		copyCtrl[port].pForwardData->ctrlWord = DEFAULT_CTRL_WORD_2400;
		
		//透明转发接收等待报文超时时间,单位为s
		copyCtrl[port].pForwardData->frameTimeOut = 10;
		
		//透明转发接收等待字节超时时间
		copyCtrl[port].pForwardData->byteTimeOut = 2;
		
		
		//透明转发内容
		copyCtrl[port].pForwardData->data[0] = 0x68;
 	  memcpy(&copyCtrl[port].pForwardData->data[1], addr, 6);
 	  copyCtrl[port].pForwardData->data[7] = 0x68;
		
		copyCtrl[port].pForwardData->data[frameTail++] = 0x1c;
		copyCtrl[port].pForwardData->data[frameTail++] = 0x10;
		
		frameTail+= 8;
		
		switch (openClose)
		{
			case 1:    //分闸一小时
			  copyCtrl[port].pForwardData->data[frameTail++] = 0+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
				break;

			case 2:    //合闸十分钟
			  copyCtrl[port].pForwardData->data[frameTail++] = 1+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 10, 0));
				break;

			case 3:    //合闸一小时
			  copyCtrl[port].pForwardData->data[frameTail++] = 1+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
				break;

			case 4:    //恢复自动控制
				copyCtrl[port].pForwardData->data[frameTail++] = 0+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 1, 0));
				break;
			
			default:   //分闸十分钟
			  copyCtrl[port].pForwardData->data[frameTail++] = 0+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 10, 0));
				break;
		}
		 
		copyCtrl[port].pForwardData->data[frameTail++] = 0x00+0x33;     //N2,控制模式为主站直接控制命令
		 
		//N3-N8,命令有效截止时间
		copyCtrl[port].pForwardData->data[frameTail++] = tmpTime.second+0x33;
		copyCtrl[port].pForwardData->data[frameTail++] = tmpTime.minute+0x33;
		copyCtrl[port].pForwardData->data[frameTail++] = tmpTime.hour+0x33;
		copyCtrl[port].pForwardData->data[frameTail++] = tmpTime.day+0x33;
		copyCtrl[port].pForwardData->data[frameTail++] = tmpTime.month+0x33;
		copyCtrl[port].pForwardData->data[frameTail++] = tmpTime.year+0x33;


		MD5Init(&context);
		MD5Update(&context, &copyCtrl[port].pForwardData->data[1], 9);
		MD5Update(&context, &copyCtrl[port].pForwardData->data[18], 8);
		MD5Final(digest, &context);
		
		copyCtrl[port].pForwardData->data[10] = digest[0]+0x33;    //PA
		copyCtrl[port].pForwardData->data[11] = digest[15]+0x33;   //P0
		copyCtrl[port].pForwardData->data[12] = digest[2]+0x33;    //P1
		copyCtrl[port].pForwardData->data[13] = digest[13]+0x33;   //P2
		copyCtrl[port].pForwardData->data[14] = digest[4]+0x33;    //C0
		copyCtrl[port].pForwardData->data[15] = digest[11]+0x33;   //C1
		copyCtrl[port].pForwardData->data[16] = digest[6]+0x33;    //C2
		copyCtrl[port].pForwardData->data[17] = digest[9]+0x33;    //C3
    
		//CS
    copyCtrl[port].pForwardData->data[frameTail] = 0x00;
    for(i=0; i<frameTail; i++)
    {
  	  copyCtrl[port].pForwardData->data[frameTail] += copyCtrl[port].pForwardData->data[i];
    }
    frameTail++;
    copyCtrl[port].pForwardData->data[frameTail++] = 0x16;

		//转发长度
		copyCtrl[port].pForwardData->length = frameTail;
	}
	else
	{
		return FALSE;
	}
	
	return TRUE;
}

/***************************************************
函数名称:xlcAcFrame
功能描述:线路控制器的交采命令帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U xlcAcFrame(INT8U port)
{
	INT8U ret=0;
  INT8U xlcSendBuf[32];
  INT8U frameTail = 8;
  INT8U i;

 	xlcSendBuf[0] = 0x68;
 	memcpy(&xlcSendBuf[1], copyCtrl[port].cpLinkHead->addr, 6);
 	xlcSendBuf[7] = 0x68;

  if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)==0x0)    //查询数据包还未发送
  {
 	  xlcSendBuf[frameTail++] = 0x11;          //C
 	  xlcSendBuf[frameTail++] = 0x04;          //L
 	  xlcSendBuf[frameTail++] = 0x00+0x33;     //0090FF00,读取组合状态数据包
 	  xlcSendBuf[frameTail++] = 0xff+0x33;
 	  xlcSendBuf[frameTail++] = 0x90+0x33;
 	  xlcSendBuf[frameTail++] = 0x00+0x33;
  }
  else
  {
    if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)    //查询数据已经返回
    {
	    ret = COPY_COMPLETE_NOT_SAVE;
    }
  }
  
  if (frameTail>8)
  {
    //CS
    xlcSendBuf[frameTail] = 0x00;
    for(i=0; i<frameTail; i++)
    {
  	  xlcSendBuf[frameTail] += xlcSendBuf[i];
    }
    frameTail++;
    xlcSendBuf[frameTail++] = 0x16;
    
    sendMeterFrame(port, xlcSendBuf, frameTail);
  }
  
  return ret;
}

/***************************************************
函数名称:ldgmFrame
功能描述:报警控制器命令帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U ldgmFrame(INT8U port)
{
	MD5_CTX context;
  INT8U   digest[16];
	INT8U   ret=0;
	INT16U  tmpData;
  INT8U   ldgmSendBuf[50];
  INT8U   tmpTail = 8;
  INT8U   i;
 	
 	ldgmSendBuf[0] = 0x68;
 	memcpy(&ldgmSendBuf[1], copyCtrl[port].cpLinkHead->addr, 6);
 	ldgmSendBuf[7] = 0x68;
 	
  //flgOfAutoCopy-bit0-读状态
  //              bit1-LDGM01校线路正常电流参数
  //              bit2-LDGM02校末端器地址
  if (0x02==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x2))
  {
    ldgmSendBuf[tmpTail++] = 0x14;    //C
    
    //长度字节后面计算后填写
    tmpTail++;
     	    
    ldgmSendBuf[tmpTail++] = 0x00+0x33;    //04010000
    ldgmSendBuf[tmpTail++] = 0x00+0x33;
    ldgmSendBuf[tmpTail++] = 0x01+0x33;
    ldgmSendBuf[tmpTail++] = 0x04+0x33;
    
    tmpTail+=8;

    //线路正常电流值
    ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->startCurrent+0x33;
    ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->ctrlTime+0x33;
    
    ldgmSendBuf[9] = tmpTail-10;
    
    MD5Init(&context);
    MD5Update(&context, &ldgmSendBuf[1], 13);
    MD5Update(&context, &ldgmSendBuf[22], ldgmSendBuf[9]-12);
    MD5Final (digest, &context);
    
    if (debugInfo&METER_DEBUG)
    { 
      printf("校报警控制器线路正常电流值参数PA=");
      for(i=0; i<16; i++)
      {
    	  printf("%02x ", digest[i]);
      }
      printf("\n");
    }
    
    ldgmSendBuf[14] = digest[0]+0x33;    //PA
    ldgmSendBuf[15] = digest[15]+0x33;   //P0
    ldgmSendBuf[16] = digest[2]+0x33;    //P1
    ldgmSendBuf[17] = digest[13]+0x33;   //P2
    ldgmSendBuf[18] = digest[4]+0x33;    //C0
    ldgmSendBuf[19] = digest[11]+0x33;   //C1
    ldgmSendBuf[20] = digest[6]+0x33;    //C2
    ldgmSendBuf[21] = digest[9]+0x33;    //C3
  }
  else
  {
  	if (0x04==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x4))
    {
      ldgmSendBuf[tmpTail++] = 0x14;    //C
      
      //长度字节后面计算后填写
      tmpTail++;
       	    
      ldgmSendBuf[tmpTail++] = 0x00+0x33;    //04010000
      ldgmSendBuf[tmpTail++] = 0x00+0x33;
      ldgmSendBuf[tmpTail++] = 0x01+0x33;
      ldgmSendBuf[tmpTail++] = 0x04+0x33;
      
      tmpTail+=8;
  
      //第一末端器地址
      for(i=0; i<6; i++)
      {
        ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->lddt1st[i]+0x33;
      }

      //第二末端器地址
      for(i=0; i<6; i++)
      {
        ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->collectorAddr[i]+0x33;
      }
      
      ldgmSendBuf[9] = tmpTail-10;
      
      MD5Init(&context);
      MD5Update(&context, &ldgmSendBuf[1], 13);
      MD5Update(&context, &ldgmSendBuf[22], ldgmSendBuf[9]-12);
      MD5Final (digest, &context);
      
      if (debugInfo&METER_DEBUG)
      { 
        printf("校LDGM02报警控制器末端器地址,PA=");
        for(i=0; i<16; i++)
        {
      	  printf("%02x ", digest[i]);
        }
        printf("\n");
      }
      
      ldgmSendBuf[14] = digest[0]+0x33;    //PA
      ldgmSendBuf[15] = digest[15]+0x33;   //P0
      ldgmSendBuf[16] = digest[2]+0x33;    //P1
      ldgmSendBuf[17] = digest[13]+0x33;   //P2
      ldgmSendBuf[18] = digest[4]+0x33;    //C0
      ldgmSendBuf[19] = digest[11]+0x33;   //C1
      ldgmSendBuf[20] = digest[6]+0x33;    //C2
      ldgmSendBuf[21] = digest[9]+0x33;    //C3
    }
    else
    {
      if (0x0==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x1))
      {
        //2015-01-08,读取临时直流电流表的数据帧
        //ldgmSendBuf[0] = 0x01;
        //ldgmSendBuf[1] = 0x03;
        //ldgmSendBuf[2] = 0x00;
        //ldgmSendBuf[3] = 0x2b;
        //ldgmSendBuf[4] = 0x00;
        //ldgmSendBuf[5] = 0x01;
        //ldgmSendBuf[6] = 0xf4;
        //ldgmSendBuf[7] = 0x02;
  	    //sendMeterFrame(port, ldgmSendBuf, 8);
   	  
   	    ldgmSendBuf[tmpTail++] = 0x11;         //C
   	    ldgmSendBuf[tmpTail++] = 0x04;         //L
   	    ldgmSendBuf[tmpTail++] = 0x00+0x33;    //0090FF00,读取组合数据包
   	    ldgmSendBuf[tmpTail++] = 0xff+0x33;
   	    ldgmSendBuf[tmpTail++] = 0x90+0x33;
   	    ldgmSendBuf[tmpTail++] = 0x00+0x33;
      }
      else
      {
  	    ret = COPY_COMPLETE_NOT_SAVE;
      }
    }
  }
  
  if (tmpTail>8)
  {
    //CS
    ldgmSendBuf[tmpTail] = 0x00;
    for(i=0; i<tmpTail; i++)
    {
  	  ldgmSendBuf[tmpTail] += ldgmSendBuf[i];
    }
    tmpTail++;
    ldgmSendBuf[tmpTail++] = 0x16;
    
    sendMeterFrame(port, ldgmSendBuf, tmpTail);
  }
  
  return ret;
}

/***************************************************
函数名称:lsFrame
功能描述:光照度传感器命令帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U lsFrame(INT8U port)
{
	INT8U  ret=0;
  INT8U  lsSendBuf[50];
  INT16U tmpData;
 	
  //flgOfAutoCopy-bit0-读取光照度值
  if (0x0==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x1))
  {
    //lsSendBuf[0] = copyCtrl[port].cpLinkHead->addr[0];

    if (copyCtrl[port].cpLinkHead->ctrlTime==2)    //2型照度传感器,风速风向
    {
      lsSendBuf[0] = copyCtrl[port].cpLinkHead->addr[0];
      lsSendBuf[1] = 0x03;
      lsSendBuf[2] = 0x01;
      lsSendBuf[3] = 0x00;
      lsSendBuf[4] = 0x00;
      lsSendBuf[5] = 0x02;    	
    }
    else    //能慧
    {
      lsSendBuf[0] = 0x0A;
      lsSendBuf[1] = 0x04;
      lsSendBuf[2] = 0x00;
      lsSendBuf[3] = 0x00;
      lsSendBuf[4] = 0x00;
      lsSendBuf[5] = 0x02;
    }
    tmpData = CRC16(lsSendBuf, 6);
    
    lsSendBuf[6] = tmpData>>8&0xff;
    lsSendBuf[7] = tmpData&0xff;
    sendMeterFrame(port, lsSendBuf, 8);
  }
  else
  {
    ret = COPY_COMPLETE_NOT_SAVE;
  }
  
  return ret;
}

/***************************************************
函数名称:thFrame
功能描述:温湿度传感制器命令帧
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U thFrame(INT8U port)
{
	INT8U  ret=0;
  INT8U  thSendBuf[8];
  INT16U tmpData;
 	
  //flgOfAutoCopy-bit0-读取温湿度值
  if (0x0==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x1))
  {
    thSendBuf[0] = copyCtrl[port].cpLinkHead->addr[0];
    thSendBuf[1] = 0x03;
    thSendBuf[2] = 0x00;
    thSendBuf[3] = 0x00;
    thSendBuf[4] = 0x00;
    thSendBuf[5] = 0x02;
    tmpData = CRC16(thSendBuf, 6);
    
    thSendBuf[6] = tmpData>>8&0xff;
    thSendBuf[7] = tmpData&0xff;
    sendMeterFrame(port, thSendBuf, 8);
  }
  else
  {
    ret = COPY_COMPLETE_NOT_SAVE;
  }
  
  return ret;
}

/***************************************************
函数名称:initXlcLink
功能描述:初始化线路控制器链表
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U initXlcLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//释放链表
	while(xlcLink!=NULL)
	{	 	 
	  tmpCpLink = xlcLink->next;
	 	 
	 	free(xlcLink);
	 	 
	 	xlcLink = tmpCpLink;
	}
	
  tmpCpLink = initPortMeterLink(0xff);

	xlcLink  = NULL;
  tmpPrevLink = xlcLink;
  while(tmpCpLink!=NULL)
  {
  	if (LIGHTING_XL==tmpCpLink->protocol)
  	{
  		if (xlcLink==NULL)
  		{
  			xlcLink = tmpCpLink;
  		}
  		else
  		{
  			tmpPrevLink->next = tmpCpLink;
  		}
  		tmpPrevLink = tmpCpLink;
  	}
  	tmpCpLink = tmpCpLink->next;
  }
  
  if (xlcLink!=NULL)
  {
  	tmpPrevLink->next = NULL;
  }
}

/***************************************************
函数名称:initAcLink
功能描述:初始化线路测量点链表
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U initAcLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//释放链表
	while(acLink!=NULL)
	{	 	 
	  tmpCpLink = acLink->next;
	 	 
	 	free(acLink);
	 	 
	 	acLink = tmpCpLink;
	}
	
  tmpCpLink = initPortMeterLink(0xff);

	acLink  = NULL;
  tmpPrevLink = acLink;
  while(tmpCpLink!=NULL)
  {
  	if (AC_SAMPLE==tmpCpLink->protocol)
  	{
  		if (acLink==NULL)
  		{
  			acLink = tmpCpLink;
  		}
  		else
  		{
  			tmpPrevLink->next = tmpCpLink;
  		}
  		tmpPrevLink = tmpCpLink;
  		
  	}
  	tmpCpLink = tmpCpLink->next;
  }
  if (acLink!=NULL)
  {
  	tmpPrevLink->next = NULL;
  }
}

/***************************************************
函数名称:initLdgmLink
功能描述:初始化报警控制器链表
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U initLdgmLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//释放链表
	while(ldgmLink!=NULL)
	{	 	 
	  tmpCpLink = ldgmLink->next;
	 	 
	 	free(ldgmLink);
	 	 
	 	ldgmLink = tmpCpLink;
	}
	
  tmpCpLink = initPortMeterLink(0xff);
	ldgmLink  = NULL;
  tmpPrevLink = ldgmLink;
  while(tmpCpLink!=NULL)
  {
  	if (LIGHTING_DGM==tmpCpLink->protocol)
  	{
  		tmpCpLink->status = 0x0;
  		memset((INT8U *)&tmpCpLink->statusTime, 0x0, 6);
  		memset(&tmpCpLink->duty, 0x0, 6);
  		if (ldgmLink==NULL)
  		{
  			ldgmLink = tmpCpLink;
  		}
  		else
  		{
  			tmpPrevLink->next = tmpCpLink;
  		}
  		tmpPrevLink = tmpCpLink;
  	}
  	tmpCpLink = tmpCpLink->next;
  }
  
  if (ldgmLink!=NULL)
  {
  	tmpPrevLink->next = NULL;
  }
}

/***************************************************
函数名称:initLsLink
功能描述:初始化照度传感器链表
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U initLsLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//释放链表
	while(lsLink!=NULL)
	{
	  tmpCpLink = lsLink->next;
	 	 
	 	free(lsLink);
	 	 
	 	lsLink = tmpCpLink;
	}

  tmpCpLink = initPortMeterLink(0xff);
	lsLink  = NULL;
  tmpPrevLink = lsLink;
  while(tmpCpLink!=NULL)
  {
  	if (LIGHTING_LS==tmpCpLink->protocol)
  	{
  		memset(&tmpCpLink->duty, 0x0, 6);
  		
  		if (lsLink==NULL)
  		{
  			lsLink = tmpCpLink;
  		}
  		else
  		{
  			tmpPrevLink->next = tmpCpLink;
  		}
  		tmpPrevLink = tmpCpLink;
  	}
  	tmpCpLink = tmpCpLink->next;
  }
  
  if (lsLink!=NULL)
  {
  	tmpPrevLink->next = NULL;
  }
}


/***************************************************
函数名称:initThLink
功能描述:初始化温湿度传感器链表
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U initThLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//释放链表
	while(thLink!=NULL)
	{
	  tmpCpLink = thLink->next;
	 	 
	 	free(thLink);
	 	 
	 	thLink = tmpCpLink;
	}

  tmpCpLink = initPortMeterLink(0xff);
	thLink  = NULL;
  tmpPrevLink = thLink;
  while(tmpCpLink!=NULL)
  {
  	if (LIGHTING_TH==tmpCpLink->protocol)
  	{
  		memset(&tmpCpLink->duty, 0x0, 6);
  		
  		if (thLink==NULL)
  		{
  			thLink = tmpCpLink;
  		}
  		else
  		{
  			tmpPrevLink->next = tmpCpLink;
  		}
  		tmpPrevLink = tmpCpLink;
  	}
  	tmpCpLink = tmpCpLink->next;
  }
  
  if (thLink!=NULL)
  {
  	tmpPrevLink->next = NULL;
  }
}

#endif

/***************************************************
函数名称:copyMeter
功能描述:抄表(发)任务入口函数(本终端涉及的电能表)
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
void copyMeter(void *arg)
{
  INT32U                     tmpData;
  DATE_TIME                  tmpTime, tmpBackTime;
  INT8U                      port, tmpPort;           //抄表运行循环控制计数
  INT8S                      ret;
  char                       strSave[100],*pSqlStr;
  struct cpAddrLink          *tmpNode;
  METER_DEVICE_CONFIG        meterConfig;
  METER_STATIS_EXTRAN_TIME   meterStatisExtranTime;   //一块三相电表统计事件数据(与时间无关量)
  METER_STATIS_BEARON_TIME   meterStatisBearonTime;   //一块三相电表统计事件数据(与时间有关量)
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //一块单相电表统计事件数据(与时间无关量)
  INT8U                      tmpReadBuff[LENGTH_OF_ENERGY_RECORD];
  INT32U                     tmpCount=0;
  INT8U                      singlePhaseData[512];    //单相表数据缓存
  INT8U                      carrierFramex[255];
  sqlite3_stmt               *stmt;
  const  char                *tail;
  struct timeval             tv;
  INT8U                      eventData[10];
  INT8U                      buf[25];
  INT8U                      i;
  
 #ifdef PULSE_GATHER
  INT8U                      visionBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U                      reqBuff[LENGTH_OF_REQ_RECORD];
  INT8U                      paraBuff[LENGTH_OF_PARA_RECORD];
  INT8U                      pulsei;
 #endif
      
  //载波相关变量
 #ifdef PLUG_IN_CARRIER_MODULE 
  GDW376_2_INIT              gdw3762Init;
  INT8U                      ifFound;
  INT8U                      hasCollector;            //是否有采集器?
  struct carrierMeterInfo    *tmpSalveNode, *tmpQueryFound;
  INT8U                      meterInfo[10];
  INT8U                      tmpBuff[10];
  INT16U                     tmpNumOfSyn;             //向本地通信(载波/无线)模块下发的地址数量
   
  INT8U                      carrierSendToMeter=0;    //有向载波模块发送抄表数据帧?2013-12-25
   
  #ifdef LIGHTING
    
   INT8U                     slSendBuf[250];          //streetLight发送缓存
    
   TERMINAL_STATIS_RECORD    terminalStatisRecord;    //终端统计记录

  #endif
 #endif
   
  //读取最近一次采集数据,确定上次抄表时间及下次抄表时间 
  for(port=0; port<NUM_OF_COPY_METER; port++)
  {
    copyCtrl[port].meterCopying      = FALSE;
    copyCtrl[port].numOfMeterAbort   = 0;
    copyCtrl[port].flagOfRetry       = FALSE;
    copyCtrl[port].retry             = 0;
    copyCtrl[port].backupCtrlWord    = 0;
    copyCtrl[port].pForwardData      = NULL;
     
    copyCtrl[port].thisMinuteProcess = FALSE;
    copyCtrl[port].ifCheckTime       = 0;
		 
	 #ifdef LIGHTING
	  //2016-11-25
	  copyCtrl[port].portFailureRounds = 0;
	 #endif
      
    //读取最近一次采集数据,确定上次抄表时间
    if (port==4)    //载波接口
    {
	    strcpy(strSave,bringTableName(sysTime,0));
	    pSqlStr = sqlite3_mprintf("select time from %s order by time desc limit 1;", strSave);
      ret = whichItem(PORT_POWER_CARRIER);
    }
    else
    {
	    strcpy(strSave,bringTableName(sysTime,1));
	    pSqlStr = sqlite3_mprintf("select time from %s order by time desc limit 1;", strSave);
      ret = whichItem(port+1);
    }
     
    copyCtrl[port].lastCopyTime.year = 0x00;
    if (sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail)!= SQLITE_OK)
    {
      //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
      if (stmt)
      {
        sqlite3_finalize(stmt);
      }
    }
    else
    {
      if(sqlite3_step(stmt)==SQLITE_ROW)
      {
        tv.tv_sec = sqlite3_column_int(stmt,0);
        getLinuxFormatDateTime(&copyCtrl[port].lastCopyTime,&tv,2);
        copyCtrl[port].lastCopyTime = timeHexToBcd(copyCtrl[port].lastCopyTime);
      }
       
      sqlite3_finalize(stmt);
    }
       	     
    if (copyCtrl[port].lastCopyTime.year != 0x00)
    {
      if (debugInfo&METER_DEBUG)
      {
        if (port==4)
        {
          printf("端口31(载波端口)上次抄表时间:%02x-%02x-%02x %02x:%02x:%02x\n",copyCtrl[port].lastCopyTime.year,copyCtrl[port].lastCopyTime.month,copyCtrl[port].lastCopyTime.day,copyCtrl[port].lastCopyTime.hour,copyCtrl[port].lastCopyTime.minute,copyCtrl[port].lastCopyTime.second);
        }
        else
        {
          printf("端口%d上次抄表时间:%02x-%02x-%02x %02x:%02x:%02x\n",port+1,copyCtrl[port].lastCopyTime.year,copyCtrl[port].lastCopyTime.month,copyCtrl[port].lastCopyTime.day,copyCtrl[port].lastCopyTime.hour,copyCtrl[port].lastCopyTime.minute,copyCtrl[port].lastCopyTime.second);
        }
      }
      	  
      tmpTime.second = (copyCtrl[port].lastCopyTime.second>>4&0xf)*10 + (copyCtrl[port].lastCopyTime.second& 0xf);
      tmpTime.minute = (copyCtrl[port].lastCopyTime.minute>>4&0xf)*10 + (copyCtrl[port].lastCopyTime.minute& 0xf);
      tmpTime.hour   = (copyCtrl[port].lastCopyTime.hour>>4&0xf)*10 + (copyCtrl[port].lastCopyTime.hour& 0xf);
      tmpTime.day    = (copyCtrl[port].lastCopyTime.day>>4&0xf)*10 + (copyCtrl[port].lastCopyTime.day& 0xf);
      tmpTime.month  = (copyCtrl[port].lastCopyTime.month>>4&0xf)*10 + (copyCtrl[port].lastCopyTime.month& 0xf);
      tmpTime.year   = (copyCtrl[port].lastCopyTime.year>>4&0xf)*10 + (copyCtrl[port].lastCopyTime.year& 0xf);      
    }
    else
    {
      if (debugInfo&METER_DEBUG)
      {
        if (port==4)
        {
          printf("端口31(载波端口)从未抄过表\n");
        }
        else
        {
          printf("端口%d从未抄过表\n",port+1);
        }
      }
 
      tmpTime = sysTime;
    }

    if (ret>4)
    {
      if (debugInfo&METER_DEBUG)
      {	
      	printf("端口%d抄表间隔:未配置",port+1);
      }
    }
    else
    {
      if (debugInfo&METER_DEBUG)
      {
        printf("抄表间隔:%d\n",teCopyRunPara.para[ret].copyInterval);
      }
        
     #ifndef LM_SUPPORT_UT
      if (port==4)
      {
        //ly,2010-11-22,载波端口从下一小时的1分开抄
        copyCtrl[port].nextCopyTime = nextTime(sysTime, 60, 0);
        if (copyCtrl[port].nextCopyTime.hour==0x0)
        {
          copyCtrl[port].nextCopyTime.minute = 10;
        }
        else
        {
          copyCtrl[port].nextCopyTime.minute = 0x1;
        }
        copyCtrl[port].nextCopyTime.second = 0x0;
        copyCtrl[port].backMp = 0;
      }
      else
      {
     #endif
      
        //根据最近一次抄表时间计算下一次抄表时间
        //if (timeCompare(tmpTime,sysTime,teCopyRunPara.para[ret].copyInterval) == FALSE)
        //{
        //   copyCtrl[port].nextCopyTime = nextTime(sysTime,1,0);
        //}
        //else
        //{
        //   copyCtrl[port].nextCopyTime = nextTime(tmpTime,teCopyRunPara.para[ret].copyInterval,0);
        //}
       
        copyCtrl[port].nextCopyTime = nextTime(sysTime,1,0);
        copyCtrl[port].nextCopyTime.second = 0x0;
         
     #ifndef LM_SUPPORT_UT
      }
     #endif
       
    }
      
    if (debugInfo&METER_DEBUG)
    {
      if (port==4)
      {
        printf("端口31(载波端口)下次抄表时间:%02d-%02d-%02d %02d:%02d:%02d\n",
               copyCtrl[port].nextCopyTime.year,copyCtrl[port].nextCopyTime.month,
                copyCtrl[port].nextCopyTime.day,copyCtrl[port].nextCopyTime.hour,
                 copyCtrl[port].nextCopyTime.minute,copyCtrl[port].nextCopyTime.second);
      }
      else
      {
        printf("端口%d下次抄表时间:%02d-%02d-%02d %02d:%02d:%02d\n", port+1,
               copyCtrl[port].nextCopyTime.year,copyCtrl[port].nextCopyTime.month,
                copyCtrl[port].nextCopyTime.day,copyCtrl[port].nextCopyTime.hour,
                 copyCtrl[port].nextCopyTime.minute,copyCtrl[port].nextCopyTime.second);
      }
    }
     
    copyCtrl[port].ifRealBalance      = FALSE;
    copyCtrl[port].ifBackupDayBalance = 0;
    copyCtrl[port].ifCopyDayFreeze    = 0;
    copyCtrl[port].cmdPause           = FALSE;

    copyCtrl[port].cpLinkHead = NULL;     
  }
   
  //初始化载波模块库
 #ifdef PLUG_IN_CARRIER_MODULE
  resetCarrierFlag();
  gdw3762Init.afn = &carrierAfn;
  gdw3762Init.fn  = &carrierFn;
  gdw3762Init.moduleType = &carrierModuleType;
  gdw3762Init.send = sendMeterFrame;
  initGdw3762So(&gdw3762Init);
    
  carrierFlagSet.ifSearched = 0;
  
  #ifdef LIGHTING
   carrierFlagSet.broadCast  = 0;
   carrierFlagSet.searchLddt = 0;
   carrierFlagSet.searchLdgmNode = NULL;
    
   carrierFlagSet.searchOffLine = 0;
    
   carrierFlagSet.searchLddStatus = 0;
    
   initXlcLink();
   initLdgmLink();
   initLsLink();
   initAcLink();
	 initThLink();
    
   lCtrl.bright = 0xfe;
   lCtrl.lcDetectBright = 0xfe;
  #endif
 #endif

  numOfCopyPort = NUM_OF_COPY_METER;    //ly,2012-01-09
 
  while(1)
  {
   #ifdef WDOG_USE_X_MEGA 
  	if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	{
  	  xMegaQueue.inTimeFrameSend = TRUE;
  	}
   #endif

    //1.读取交流采样数据
   #ifdef AC_SAMPLE_DEVICE
      
    if (ifHasAcModule)
    {
      tmpCount++;
      if (readCheckData==0x55)   //读取交采校表前数据值
      {
      	if (tmpCount>150)
      	{
      	 	tmpCount = 0;
      	  	
      	  readAcChipData();
      	}
      }
      else
      {
        readAcChipData();
      }
    }
      
   #endif    //AC_SAMPLE_DEVICE
                  
    //2.几个端口并行抄表处理
    //ly,2012-01-09,改成下面这句
    //for(port=0; port<NUM_OF_COPY_METER; port++)
    for(port=0; port<numOfCopyPort; port++)
    {
      //2-0.抄表日冻结
      if (copyCtrl[port].ifCopyDayFreeze == 0
  	 	    && ((teCopyRunPara.para[port].copyDay[(sysTime.day-1)/8] >> ((sysTime.day-1)%8)&0x01) == 1)
  	 	     && ((sysTime.minute%10|sysTime.minute/10<<4) == teCopyRunPara.para[port].copyTime[0]) 
  	 	      && ((sysTime.hour%10|sysTime.hour/10<<4) == teCopyRunPara.para[port].copyTime[1]))
      {
      	//2012-07-26,延迟执行抄表日冻结
      	if (teCopyRunPara.para[port].copyTime[0]==0x0 && teCopyRunPara.para[port].copyTime[1]==0x0)
      	{
      	  copyCtrl[port].ifCopyDayFreeze = 5;
      	}
      	else
      	{
      	  copyCtrl[port].ifCopyDayFreeze = 1;
      	    
      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("端口%d抄表日冻结标志置位\n",port+1);
          }
        }
      }

			//2012-07-26,延迟执行抄表日冻结
      if (copyCtrl[port].ifCopyDayFreeze == 5)
      {
      	if (((sysTime.minute%10|sysTime.minute/10<<4) == teCopyRunPara.para[port].copyTime[0]+3)
      	 	  && ((sysTime.hour%10|sysTime.hour/10<<4) == teCopyRunPara.para[port].copyTime[1])
      	 	   && (sysTime.second>5)
      	 	 )
      	{
          copyCtrl[port].ifCopyDayFreeze = 1;
      	    
      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("端口%d延迟执行抄表日冻结标志置位\n",port+1);
          }
      	}
      }
			if (copyCtrl[port].ifCopyDayFreeze==1)
      {
      	if (debugInfo&METER_DEBUG)
      	{
      	  printf("端口%d开始进行抄表日冻结存储\n",port+1);
        }
     
      	copyDayFreeze(port);
      	copyCtrl[port].copyDayTimeReset = nextTime(sysTime, 1, 0);
        copyCtrl[port].ifCopyDayFreeze = 2;
           
      	if (debugInfo&METER_DEBUG)
      	{
      	  printf("端口%d抄表日冻结存储结束\n",port+1);
        }
      }
      if (copyCtrl[port].ifCopyDayFreeze==2 && compareTwoTime(copyCtrl[port].copyDayTimeReset, sysTime)==TRUE)
      {
    	  copyCtrl[port].ifCopyDayFreeze = 0;

    	  if (debugInfo&METER_DEBUG)
    	  {
    	    printf("端口%d抄表日冻结标志复位\n",port+1);
        }
    	}
       	 
       	 
       	 
     	//2-1,执行广播校时电表任务
     	//2013-11-22,add
     	if (port>0)
     	{
     	  //2-1.1
     	  if ((0x0==copyCtrl[port].ifCheckTime)
     	 	    && (teCopyRunPara.para[port].copyRunControl[0]&0x08)    //要求终端定时对电表广播校时
     	 	     && ((0x00==teCopyRunPara.para[port].broadcastCheckTime[2])    //设置为每日校时
     	 	          || (bcdToHex(teCopyRunPara.para[port].broadcastCheckTime[2])==sysTime.day)    //设置的校时日为当天
     	 	        )
     	 	      && (sysTime.minute==bcdToHex(teCopyRunPara.para[port].broadcastCheckTime[0])) 
     	 	       && (sysTime.hour==bcdToHex(teCopyRunPara.para[port].broadcastCheckTime[1]))
     	 	 )
     	  {
     	    copyCtrl[port].ifCheckTime = 1;
     	  }
     	 
     	  //2-1.2 	 
     	  if (1==copyCtrl[port].ifCheckTime)
     	  {
      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("端口%d发送广播校时命令\n",port+1);
          }
      	   
      	  copyCtrl[port].checkTimeReset = nextTime(sysTime, 1, 0);

          //发送校时数据
          buf[0] = 0x68;
          buf[1] = 0x99;
          buf[2] = 0x99;
          buf[3] = 0x99;
          buf[4] = 0x99;
          buf[5] = 0x99;
          buf[6] = 0x99;
          buf[7] = 0x68;
          buf[8] = 0x08;
          buf[9] = 0x06;
          buf[10] = hexToBcd(sysTime.second)+0x33;
          buf[11] = hexToBcd(sysTime.minute)+0x33;
          buf[12] = hexToBcd(sysTime.hour)+0x33;
          buf[13] = hexToBcd(sysTime.day)+0x33;
          buf[14] = hexToBcd(sysTime.month)+0x33;
          buf[15] = hexToBcd(sysTime.year)+0x33;
          buf[16] = 0x0;
          for(i=0; i<16; i++)
          {
          	 buf[16] += buf[i];
          }
          buf[17] = 0x16;
          
      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("端口%d速率设置成1200发送校时命令\n",port+1);
          }
					
         #ifdef WDOG_USE_X_MEGA
          eventData[0] = port;                          //xMega端口
          eventData[1] = DEFAULT_SER_CTRL_WORD;         //速率1200
          sendXmegaFrame(COPY_PORT_RATE_SET, eventData, 2);
         #endif
          sendMeterFrame(port, buf, 18);

      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("端口%d速率设置成2400发送校时命令\n",port+1);
          }
         #ifdef WDOG_USE_X_MEGA
          eventData[0] = port;                          //xMega端口
          eventData[1] = DEFAULT_CTRL_WORD_2400;        //速率2400
          sendXmegaFrame(COPY_PORT_RATE_SET, eventData, 2);
         #endif
          sendMeterFrame(port, buf, 18);

          //恢复速率
         #ifdef WDOG_USE_X_MEGA
          buf[0] = port;                                //xMega端口
          buf[1] = copyCtrl[port].backupCtrlWord;    //恢复端口速率
          sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
         #endif
          
          copyCtrl[port].ifCheckTime = 2;
     	  }
        
        //2-1.3
        if (2==copyCtrl[port].ifCheckTime && compareTwoTime(copyCtrl[port].checkTimeReset, sysTime))
        {
      	  copyCtrl[port].ifCheckTime = 0;

      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("端口%d校时命令标志复位\n",port+1);
          }
        }
      }

            

      //2-2.端口31保存备份日冻结数据(每天23:50开始保存当日备份日冻结)
     #ifdef PLUG_IN_CARRIER_MODULE
      if (port==4)
      {
 	 	    tmpTime = sysTime;
 	 	    tmpTime.hour   = 23;
 	 	    tmpTime.minute = 50;
 	 	    tmpTime.second = 0;
 	 	    if (copyCtrl[port].ifRealBalance==FALSE && compareTwoTime(tmpTime,sysTime)==TRUE)
 	 	    {
 	 	  	  if (copyCtrl[port].cpLinkHead!=NULL)
 	 	  	  {
 	 	  	    copyCtrl[port].ifRealBalance = TRUE;
 	 	  	    tmpBackTime = nextTime(tmpTime, 20, 0);
 	 	  	    if (tmpBackTime.month!=tmpTime.month)
 	 	  	    {
 	 	  	   	  copyCtrl[port].ifCopyLastFreeze = 1;  //应该存储备份月冻结数据
 	 	  	    }
 	 	  	    else
 	 	  	    {
 	 	  	   	  copyCtrl[port].ifCopyLastFreeze = 0;  //尚不需要存储备份月冻结数据
 	 	  	    }

 	 	  	    //开始存储备份日冻结数据
 	 	  	    if (debugInfo&PRINT_CARRIER_DEBUG)
 	 	  	    { 
 	 	  	      printf("开始存储备份日冻结数据\n");
 	 	  	    }
 	 	  	  
 	 	  	    tmpNode = copyCtrl[port].cpLinkHead;
 	 	  	   
 	 	  	    tmpBackTime = tmpTime;
 	 	  	    tmpBackTime.hour   = 23;
 	 	  	    tmpBackTime.minute = 59;
 	 	  	    tmpBackTime.second = 59;
 	 	  	    while(tmpNode!=NULL)
 	 	  	    {
              //ly,2010-11-23,由于重庆电力公司要求冻结值反应真实情况,因此,07表不存备份日冻结数据
              if (tmpNode->protocol!=DLT_645_2007)
              {
                //查询测量点存储信息
   		          queryMeterStoreInfo(tmpNode->mp, meterInfo);
 
                tmpTime = timeHexToBcd(tmpBackTime);
                if (readMeterData(singlePhaseData, tmpNode->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0)==TRUE)
                {
                  if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
                  {
                   	saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[2], DAY_FREEZE_COPY_DATA, meterInfo[4] | meterInfo[5]<<8);
                   	    
                   	//保存备份月冻结数据
                   	if (copyCtrl[port].ifCopyLastFreeze==1)
                   	{
                   	  saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[3],MONTH_FREEZE_COPY_DATA, meterInfo[4] | meterInfo[5]<<8);
                   	}
                  }
                  else
                  {
                   	saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[2], 0, meterInfo[4] | meterInfo[5]<<8);
                   	    
               	    //保存备份月冻结数据
               	    if (copyCtrl[port].ifCopyLastFreeze==1)
               	    {
               	      saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[3], 0, meterInfo[4] | meterInfo[5]<<8);
               	    }
               	  }
                }
                    
                if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
                {
                  if (readMeterData(singlePhaseData, tmpNode->mp, meterInfo[1], REQ_REQTIME_DATA, &tmpTime, 0)==TRUE)
                  {
               	    saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, DAY_BALANCE, DAY_FREEZE_COPY_REQ, meterInfo[4] | meterInfo[5]<<8);
               	    
               	    if (copyCtrl[port].ifCopyLastFreeze==1)
               	    {
                 	    saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, MONTH_BALANCE, MONTH_FREEZE_COPY_REQ, meterInfo[4] | meterInfo[5]<<8);
               	    }
                  }
                }
              }
     	 	  	 	  
     	 	  	  tmpNode = tmpNode->next;
     	 	  	 	   
     	 	  	  usleep(50000);
     	 	    }
     	 	  	   
     	 	    copyCtrl[port].ifCopyLastFreeze = 0;
     	 	  	  
     	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	 	    {
     	 	  	  printf("备份日冻结数据存储完成\n");
     	 	    }
     	 	  }
     	  }
     	 	  
 	 	    if (copyCtrl[port].ifRealBalance==TRUE)
 	 	    {
 	 	  	  if (sysTime.hour==0x0)
 	 	  	  {
 	  	 	    copyCtrl[port].ifRealBalance = FALSE;
 	  	 	 
 	  	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
 	  	 	    {
 	  	 	      printf("存储日冻结标志复位\n");
 	  	 	    }
 	 	  	  }
 	 	    }
      }
     #endif   //PLUG_IN_CARRIER_MODULE
        
        
      	 
  	  //2-3.抄表命令发送、超时、转发命令处理
  	  if (copyCtrl[port].meterCopying==FALSE)   //该端口当前未抄表
  	  {
 	   #ifdef PLUG_IN_CARRIER_MODULE
     	   
 	  	  //2-3-1.1本地通信模块(载波)端口
 	  	 #ifdef LM_SUPPORT_UT
 	  	  if (port==4 && 0x55!=lmProtocol)
 	  	 #else
 	  	  if (port==4)
 	  	 #endif
 	  	  {
 	  	    //ly,2012-4-1,如果在升级路由不能操作载波模块
          if (upRtFlag!=0)
 	  	    {
 	  	 	    goto breakPoint;
 	  	    }
 	  	 	 
 	  	    if (carrierFlagSet.retryCmd==1 || carrierFlagSet.cmdContinue==1)
 	  	    {
        	  //2-3-1.1-1.发硬件复位命令
            if (carrierFlagSet.hardwareReest==0)
            {
        	    gdw3762Framing(INITIALIZE_3762, 1, NULL, NULL);
        	    carrierFlagSet.cmdType = CA_CMD_HARD_RESET;
              carrierFlagSet.hardwareReest = 1;
             
              //2010-1-13,为锐拔加的
              carrierFlagSet.delayTime    = nextTime(sysTime,  1, 5);
              carrierFlagSet.netOkTimeOut = nextTime(sysTime, 30, 0);

              //2010-04-30,SR复位操作后等待5秒
              carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 5);

              goto breakPoint;
            }

        	  //2-3-1.1-2.如果还不知道是什么模块
        	  if (carrierModuleType==NO_CARRIER_MODULE)
        	  {
        	    gdw3762Framing(QUERY_DATA_3762, 1, NULL, NULL);
        	    carrierFlagSet.cmdType = CA_CMD_READ_MODULE;

        	    goto breakPoint;
        	  }
        	 
        	  //2-3-1.1-3.各家模块不同点处理
         	  // 2-3-1.1-3-1.SR模块参数区初始化及等待5秒,2011-04-28,上海SR,add
            //    2012-07-21,由于无线模块送检的是新版SRWF-3E68模块,它在模块子节点没有变动的情况下无需再组网,
            //               不推荐每次启动都清除参数
            if (carrierModuleType==SR_WIRELESS && localModuleType!=SR_WF_3E68)
            {
              if (carrierModuleType==SR_WIRELESS && (carrierFlagSet.paraClear==0 || carrierFlagSet.paraClear==1))
              {
            	  if (carrierFlagSet.paraClear==0)
            	  {
            	    gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
            	    carrierFlagSet.cmdType = CA_CMD_SR_PARA_INIT;

          	      goto breakPoint;
            	  }
            	 
            	  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
            	  {
            	    if (carrierFlagSet.paraClear==1)
            	    {
              	 	  carrierFlagSet.paraClear = 2;
            	    }
            	  }
          	   
          	    goto breakPoint;
          	  }
        	  }

        	  // 2-3-1.1-3-2.友迅达模块设置模块时间
        	  if (carrierModuleType==FC_WIRELESS)
        	  {
        	    if (carrierFlagSet.setDateTime==0)
        	    {
        	      carrierFramex[0] = hexToBcd(sysTime.second);
        	      carrierFramex[1] = hexToBcd(sysTime.minute);
        	      carrierFramex[2] = hexToBcd(sysTime.hour);
        	      carrierFramex[3] = hexToBcd(sysTime.day);
        	      carrierFramex[4] = hexToBcd(sysTime.month);
        	      carrierFramex[5] = hexToBcd(sysTime.year);
        	     
          	    gdw3762Framing(CTRL_CMD_3762, 31, NULL, carrierFramex);
          	   
        	 	    carrierFlagSet.cmdType = CA_CMD_SET_MODULE_TIME;
        	 	   
        	 	    goto breakPoint;
        	    }
        	  }
            	 
        	  //2-3-1.1-4.如果需要强制删除载波模块上的表地址及路由
        	  if (debugInfo & DELETE_LOCAL_MODULE)
        	  {
        	 	  switch (carrierModuleType)
        	 	  {
        	 	  	case RL_WIRELESS:  //锐拔
        	 	  	case SC_WIRELESS:  //赛康
        	 	  	  memset(tmpBuff, 0x0, 6);
        	 	  	  gdw3762Framing(ROUTE_SET_3762, 2, NULL, tmpBuff);
        	 	  	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
        	 	  	  break;
        	 	  	
        	 	    case FC_WIRELESS:  //友迅达,不支持参数区初始化,用数据区初始化
        	 	      gdw3762Framing(INITIALIZE_3762, 3, NULL, NULL);
        	 	  	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
        	 	  	  break;
        	 	    
        	 	    default:           //支持参数区初始化的模块
        	 	      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
        	 	  	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
        	 	  	  break;
        	 	  }

            	goto breakPoint;
            }
        	  	 
        	  //2-3-1.1-5.点抄或转发
            if (!(carrierFlagSet.searchMeter>0 && carrierModuleType==CEPRI_CARRIER))
            {
    	  	    if (pDotCopy!=NULL)
    	  	    {
                dotCopy(PORT_POWER_CARRIER);
                goto breakPoint;
    	  	    }
    	  	    else
    	  	    {
    	  	   	  if (pDotFn129!=NULL)
    	  	   	  {
            	    pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
          		    pDotCopy->dotCopyMp   = pDotFn129->pn;
          		    pDotCopy->dotCopying  = FALSE;
          		    pDotCopy->port        = PORT_POWER_CARRIER;
          		    pDotCopy->dataFrom    = pDotFn129->from;
          		    pDotCopy->outTime     = nextTime(sysTime,0,15);
          		    pDotCopy->dotResult   = RESULT_NONE;
          		    pDotFn129->ifProcessing = 1;         //正在处理
          		   
          		    goto breakPoint;
    	  	   	  }
    	  	    }
        	     
    	        if (copyCtrl[port].pForwardData!=NULL)
    	        {
                forwardData(port);
                goto breakPoint;
              }
    	      }

            //2-3-1.1-6. 2010-01-13,适应锐拔模块,模块上电后延时一分钟
            //    if (carrierModuleType==RL_WIRELESS)
            //    2012-02-28,电科院载波新版3芯片的路由板也有这个要求了
            if (carrierModuleType==RL_WIRELESS || localModuleType==CEPRI_CARRIER_3_CHIP)
            {
            	if (carrierFlagSet.hardwareReest==1 || carrierFlagSet.hardwareReest==2)
            	{
              	if (compareTwoTime(carrierFlagSet.delayTime, sysTime))
              	{
              	 	carrierFlagSet.hardwareReest = 3;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                 	  printf("延迟一分钟时间到,开始操作模块\n");
                  }
                     
                  carrierFlagSet.delayTime = nextTime(sysTime, 1, 5);
              	}
              	 	
              	goto breakPoint;
            	}
            }
            	 
            //2-3-1.1-7. 2011-04-28,上海SR,设置主节点地址后主节点会复位需要等待5秒后操作
            if (carrierModuleType==SR_WIRELESS && carrierFlagSet.setMainNode==2)
            {
        	    if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
        	    {
        	 	    carrierFlagSet.setMainNode = 3;
        	 	 
        	 	    carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 15);
        	    }
        	 
        	    goto breakPoint;
            }
               
            //2-3-1.1-7-1.沃电无线模块或是SR无线模块打开内部调试的自动上报功能
            //    2011-04-28,上海SR,根据SR工程师的思路,取消主动上报功能
            //if (carrierFlagSet.innerDebug == 0 && (carrierModuleType==HWWD_WIRELESS ||carrierModuleType==SR_WIRELESS))
            //{
            //	  gdw3762Framing(DEBUG_3762, 1, NULL, NULL);
            //	  goto breakPoint;
            //}

        	  //2-3-1.1-8.查询载波模块主节点地址
        	  if (carrierFlagSet.mainNode==0)
        	  {
        	    //东软、弥亚微载波模块,置为路由主导抄表,ly,2011-07-07
        	    if (localCopyForm==0xaa)
        	    {
          	    //ly,2012-01-14,目前有东软、弥亚微、鼎信支持路由主导抄读
          	    if (carrierModuleType==EAST_SOFT_CARRIER
          	   	     || carrierModuleType==TC_CARRIER
          	   	       || carrierModuleType==MIA_CARRIER
          	   	   )
          	    {
            	    if (debugInfo&PRINT_CARRIER_DEBUG)
            	    {
            	    	 printf("*******************路由主导抄读方式*******************\n");
            	    }

                  carrierFlagSet.routeLeadCopy=1;
                 
                  //ly,2011-12-30,Add
                  if (carrierModuleType==TC_CARRIER)
                  {
                    carrierFlagSet.workStatus = 2;
                  }
          	    }
          	    else    //其它模块为集中器主导抄读
          	    {
          	   	  localCopyForm = 0x55;
          	   	 
            	    if (debugInfo&PRINT_CARRIER_DEBUG)
            	    {
            	    	printf("*******************强制为集中器主导抄读方式*******************\n");
            	    }
          	    }
          	  }
          	  else
          	  {
          	    if (debugInfo&PRINT_CARRIER_DEBUG)
          	    {
          	   	  printf("*******************集中器主导抄读方式*******************\n");
          	    }
          	  }
        	   
        	    gdw3762Framing(QUERY_DATA_3762, 4, NULL, NULL);
        	    carrierFlagSet.cmdType = CA_CMD_QUERY_MAIN_NODE;

              if (carrierModuleType==RL_WIRELESS)
              {
                carrierFlagSet.synMeterNo = 0;
              }
              else
              {
                carrierFlagSet.synMeterNo = 1;
              }

        	    goto breakPoint;
        	  }
               
            //2-3.1.1-9.设置本地通信模块主节点地址
            //    if (carrierFlagSet.setMainNode==1)
            //    2012-02-27,新版电科院模块不要求设置主节点地址
            if (carrierFlagSet.setMainNode==1 && localModuleType!=CEPRI_CARRIER_3_CHIP)
            {
              memcpy(carrierFramex, mainNodeAddr, 6);
            
      	      gdw3762Framing(CTRL_CMD_3762, 1, carrierFramex, NULL);
      	      carrierFlagSet.cmdType = CA_CMD_SET_MAIN_NODE;

              //ly,2011-04-28,上海SR,add
              if (carrierModuleType==SR_WIRELESS)
              {
      	        carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 5);
      	      }
      	      goto breakPoint;
            }

            //2-3.1.1-10.无线线模块设置信道号
            //    2012-07-20,SRWF-3E68需要设置信道/网络标识命令
            //    2012-08-13,友讯达模块也需要设置信道号
            if (((carrierModuleType==SR_WIRELESS && localModuleType==SR_WF_3E68) || carrierModuleType==FC_WIRELESS)  && carrierFlagSet.setPanId==0)
            {
  		 	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	    {
  		 	 	  	  printf("设置信道号为:%d\n",(addrField.a2[0] | addrField.a2[1]<<8)%0x10);
  		 	 	    }
  		 	 	  
  		 	 	    if (carrierModuleType==FC_WIRELESS)
  		 	 	    {
                carrierFramex[0] = (addrField.a2[0] | addrField.a2[1]<<8)%10;   //信道号
                carrierFramex[1] = 0x00;         //未知
                carrierFramex[2] = 0x00;
                carrierFramex[3] = 0x00;
                carrierFramex[4] = 0x00;
                carrierFramex[5] = 0x00;
                carrierFramex[6] = 0x00;
                carrierFramex[7] = 0x00;
              
      	        gdw3762Framing(CTRL_CMD_3762, 4, NULL, carrierFramex);                    
  		 	 	    }
  		 	 	    else
  		 	 	    {
                carrierFramex[0] = (addrField.a2[0] | addrField.a2[1]<<8)%0x10;   //信道号
                carrierFramex[1] = 0x05;         //未知
                carrierFramex[2] = 0x78;
                carrierFramex[3] = 0x56;
                carrierFramex[4] = 0x56;
                carrierFramex[5] = 0x56;
                carrierFramex[6] = 0x00;
                carrierFramex[7] = 0x00;
      	      
      	        gdw3762Framing(RL_EXTEND_3762, 1, NULL, carrierFramex);
              }
            	    
            	goto breakPoint;
            }

         	  //2-3.1.1-11.初始启动时置工作模式
        	  if (carrierFlagSet.startStopWork==1 && carrierModuleType!=RL_WIRELESS && carrierModuleType!=SC_WIRELESS)
        	  {
        	    switch(carrierModuleType)
        	    {
        	      case SR_WIRELESS:  //SR无线模块或是沃电无线模块置为组网、验证模式
        	      case HWWD_WIRELESS:
        	        carrierFlagSet.startStopWork = 2;

        	        carrierFramex[0] = 0x03;   //组网,验证模式
        	        carrierFramex[1] = 0x0;
        	        carrierFramex[2] = 0x0;
        	        gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
        	 	   		carrierFlagSet.cmdType = CA_CMD_SR_CHECK_NET;
        	        break;
        	     
        	      case MIA_CARRIER:            //弥亚微载波模块 每次上电都为"停止工作",不用发暂停令
        	      case EAST_SOFT_CARRIER:      //东软载波模块 每次上电都为"停止工作",不用发暂停令
        	      case FC_WIRELESS:            //友迅达无线模块
        	      case CEPRI_CARRIER:          //电科院模块,ly,2012-02-28,改到这里,原是default
        	        carrierFlagSet.startStopWork = 0;
        	        break;
        	     
        	      default:                     //载波模块停止当前工作
        	        carrierFlagSet.startStopWork = 2;
        	        gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
        	 	      carrierFlagSet.cmdType = CA_CMD_PAUSE_WORK;
        	        break;
        	    }
        	    goto breakPoint;
        	  }
            	 
            //2-3-1.1-12.查询路由运行状态(锐拔和赛康的无线模块不支持查询路由运行状态)
            if (carrierFlagSet.routeRunStatus==1 && carrierModuleType!=RL_WIRELESS && carrierModuleType!=SC_WIRELESS)
            {
        	 	  if (carrierModuleType==FC_WIRELESS)
        	 	  {
        	 	  	gdw3762Framing(FC_QUERY_DATA_3762, 2, NULL, NULL);
        	 	  }
        	 	  else
        	 	  {
        	 	    gdw3762Framing(ROUTE_QUERY_3762, 4, NULL, NULL);
        	 	  }
        	 	  carrierFlagSet.cmdType = CA_CMD_QUERY_ROUTE_STATUS;
        	 	  goto breakPoint;
            }

            //2-3-1.1-13.查询从节点数量
            if (carrierFlagSet.querySlaveNum==1 && !(carrierModuleType==SR_WIRELESS || carrierModuleType==CEPRI_CARRIER))
            {
        	 	  gdw3762Framing(ROUTE_QUERY_3762, 1, NULL, NULL);
        	 	  carrierFlagSet.cmdType = CA_CMD_QUERY_SLAVE_NUM;

        	 	  goto breakPoint;
            }
            	 
            //2-3-1.1-14.主站设置表地址后等待一分钟同步到模块,以便表地址多帧发送的时候做一次同步
            //    2011-01-29,add
            if (carrierFlagSet.msSetAddr==1)
            {
            	if (compareTwoTime(carrierFlagSet.msSetWait, sysTime))
            	{
                carrierFlagSet.msSetAddr = 0;
                carrierFlagSet.synSlaveNode = 1;
              
                if (debugInfo&PRINT_CARRIER_DEBUG)
                {
             	    printf("主站设置表地址等待时间到,开始同步表地址到模块\n");
                }
              }
            }
            	 
        	  //2-3-1.1-15.如果有表计需要同步到载波模块
        	  if (carrierFlagSet.synSlaveNode>0)
        	  {
     	        switch (carrierFlagSet.querySlaveNode)
     	        {
     	      	  case 0:	     //同步前先查询载波从节点信息
      	      	  //if (carrierModuleType==RL_WIRELESS || carrierModuleType==SC_WIRELESS)
     	      	    //2012-07-20,桑锐的SRWF-3E68也是从0开始
     	      	    if (carrierModuleType==RL_WIRELESS || carrierModuleType==SC_WIRELESS || carrierModuleType==SR_WIRELESS)
     	      	    {
     	      	      carrierFlagSet.synMeterNo = 0;
     	      	    }
     	      	    else
     	      	    {
     	      	      carrierFlagSet.synMeterNo = 1;
     	      	    }
     	      	   
     	      	    carrierFlagSet.hasAddedMeter  = 0;   //没有向模块添加表地址
     	      	    carrierSlaveNode = NULL;
     	      	    prevSlaveNode = carrierSlaveNode;

     	      	    switch (carrierModuleType)
     	      	    {
     	      	   	
     	      	   	  case SR_WIRELESS:    //桑锐
                      if (localModuleType==SR_WF_3E68)
                      {
     	      	          //SR_WF_3E68可以比对地址,2012-07-21
     	      	          carrierFlagSet.querySlaveNode = 1;
                      }
                      else
                      {
                        //SR无线模块或是沃电无线模块直接发送查询载波从节点帧
     	      	          carrierFlagSet.querySlaveNode = 2;
     	      	        }
     	      	        break;
     	      	             	      	     
     	      	      case FC_WIRELESS:    //友迅达
     	      	        gdw3762Framing(FC_NET_CMD_3762, 13, NULL, NULL);
     	      	        carrierFlagSet.cmdType = CA_CMD_UPDATE_ADDR;
     	      	        showInfo("启动档案更新");
     	      	        break;
     	      	     
     	      	      default:
     	      	        carrierFlagSet.querySlaveNode = 1; //可以发送查询载波从节点帧
     	      	        break;
     	      	    }
     	      	    goto breakPoint;
     	      	    break;
         	      	
     	      	  case 1:      //发送查询载波从节点帧
        	        if (carrierModuleType==FC_WIRELESS)
        	        {
        	          gdw3762Framing(FC_QUERY_DATA_3762, 10, NULL, (INT8U *)&carrierFlagSet.synMeterNo);
        	        }
        	        else
        	        {
        	          gdw3762Framing(ROUTE_QUERY_3762, 2, NULL, (INT8U *)&carrierFlagSet.synMeterNo);
        	        }
        	        carrierFlagSet.cmdType = CA_CMD_QUERY_SLAVE_NODE;
        	        showInfo("比较本地通信模块地址");
        	        goto breakPoint;
        	        break;
            	     
        	      case 2:      //载波从节点查询完毕,可以同步
        	        switch(carrierFlagSet.synSlaveNode)
        	        {
        	          case 1:  //读出测量点号,如果需要做参数区初始化
        	 	  	      checkCarrierMeter();     //检查载波表抄表情况
        	 	  	      
	    	 	  	        //库中无载波表号而本地通信模块(载波/无线)有表号,需要做参数区初始化
	    	 	  	        if (copyCtrl[port].cpLinkHead==NULL && carrierFlagSet.numOfSalveNode>0 && carrierFlagSet.init==0)
	    	 	  	        {
	    	 	  	      	  carrierFlagSet.init = 2;
	    	 	  	      	  switch (carrierModuleType)
	    	 	  	      	  {
	    	 	  	      	    case RL_WIRELESS:  //锐拔无线模块不支持参数区初始化,只支持删除所有从节点
	    	 	  	      	    case SC_WIRELESS:  //赛康
		    	 	  	      	 	  memset(tmpBuff, 0x0, 6);
		    	 	  	      	 	  gdw3762Framing(ROUTE_SET_3762, 2, NULL, tmpBuff);
		    	 	  	      	 	  break;
	    	 	  	      	 	 
	    	 	  	      	    case FC_WIRELESS:  //友迅达模块
	    	 	  	      	      gdw3762Framing(INITIALIZE_3762, 3, NULL, NULL);
	    	 	  	      	 	    break;
	    	 	  	      	 	 
	    	 	  	      	    default:           //其它支持参数区初始化的模块
	    	 	  	      	      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
	    	 	  	      	      break;
	    	 	  	      	  }
	               	      carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;

						 					  goto breakPoint;
	    	 	  	        }
            	 	  	      
        	 	  	      if (copyCtrl[port].cpLinkHead!=NULL)
        	 	  	      {
        	 	  	      	//ly,2011-01-30,比较有没有在集中器档案中不存在的本地通信模块中的表号,
        	 	  	      	//              如有不存在的表号,删除所有本地模块中的表号
        	 	  	      	ifFound = 0;

                        //2012-07-20,SR的SRWF-3E68可以比较,老版SR无法比较
        	 	  	      	if ((carrierModuleType!=SR_WIRELESS  || (carrierModuleType==SR_WIRELESS && localModuleType==SR_WF_3E68))
        	 	  	      	 	 && carrierFlagSet.numOfSalveNode>0)
        	 	  	      	{
        	 	  	      	  prevSlaveNode = carrierSlaveNode;
        	 	  	      	  while (prevSlaveNode!=NULL)
        	 	  	      	  {
        	 	  	      	 	  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
        	 	  	      	    ifFound = 0;
        	 	  	      	 	  while(copyCtrl[port].tmpCpLink!=NULL)
        	 	  	      	 	  {
              	 	  	      	//2014-09-19,路灯控制器的载波控制点的采集器地址表示亮度占空比了,所以不能也不用比较采集器地址
              	 	  	       #ifndef LIGHTING 
              	 	  	      	//档案中本测量点有采集器地址
              	 	  	      	if (copyCtrl[port].tmpCpLink->collectorAddr[0]!=0 ||copyCtrl[port].tmpCpLink->collectorAddr[1]!=0
              	 	  	      	    || copyCtrl[port].tmpCpLink->collectorAddr[2]!=0 ||copyCtrl[port].tmpCpLink->collectorAddr[3]!=0
              	 	  	      	     || copyCtrl[port].tmpCpLink->collectorAddr[4]!=0 ||copyCtrl[port].tmpCpLink->collectorAddr[5]!=0
              	 	  	      	   )
              	 	  	      	{
          	 	  	      	    	if (prevSlaveNode->addr[0] ==copyCtrl[port].tmpCpLink->collectorAddr[0]
          	 	  	      	    	    && prevSlaveNode->addr[1]==copyCtrl[port].tmpCpLink->collectorAddr[1]
          	 	  	      	    	     && prevSlaveNode->addr[2]==copyCtrl[port].tmpCpLink->collectorAddr[2]
          	 	  	      	    	      && prevSlaveNode->addr[3]==copyCtrl[port].tmpCpLink->collectorAddr[3]
          	 	  	      	    	       && prevSlaveNode->addr[4]==copyCtrl[port].tmpCpLink->collectorAddr[4]
          	 	  	      	    	        && prevSlaveNode->addr[5]==copyCtrl[port].tmpCpLink->collectorAddr[5]
          	 	  	      	    	   )
          	 	  	      	    	{
          	 	  	      	    	 	ifFound = 1;
          	 	  	      	    	}
        	 	  	      	 	    }
        	 	  	      	 	    else   //档案中本测量点是表地址
        	 	  	      	 	    {
        	 	  	      	 	   #endif
            	 	  	      	 	  	
          	 	  	      	    	if (prevSlaveNode->addr[0] ==copyCtrl[port].tmpCpLink->addr[0]
          	 	  	      	    	    && prevSlaveNode->addr[1]==copyCtrl[port].tmpCpLink->addr[1]
          	 	  	      	    	     && prevSlaveNode->addr[2]==copyCtrl[port].tmpCpLink->addr[2]
          	 	  	      	    	      && prevSlaveNode->addr[3]==copyCtrl[port].tmpCpLink->addr[3]
          	 	  	      	    	       && prevSlaveNode->addr[4]==copyCtrl[port].tmpCpLink->addr[4]
          	 	  	      	    	        && prevSlaveNode->addr[5]==copyCtrl[port].tmpCpLink->addr[5]
          	 	  	      	    	   )
          	 	  	      	    	 {
          	 	  	      	    	 	 ifFound = 2;
          	 	  	      	    	 }
          	 	  	      	    	 
    	 	  	      	 	  	   #ifndef LIGHTING
    	 	  	      	 	  	    }
    	 	  	      	 	  	   #endif
    	 	  	      	 	  	 
    	 	  	      	 	  	    //集中器档案与模块中本表地址相同
    	 	  	      	 	  	    if (ifFound!=0)
    	 	  	      	 	  	    {
    	 	  	      	 	  	 	    break;
    	 	  	      	 	  	    }
    	 	  	      	 	  	    copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next; 
    	 	  	      	 	      }
            	 	  	      	 	   
    	 	  	      	 	      if (ifFound==0)
    	 	  	      	 	      {
    	 	  	      	 	   	    if (debugInfo&PRINT_CARRIER_DEBUG)
    	 	  	      	 	   	    {
    	 	  	      	 	   	  	  printf("本地通信模块中表地址%02x%02x%02x%02x%02x%02x在集中器档案中不存在\n",
    	 	  	      	 	   	  	         prevSlaveNode->addr[5],prevSlaveNode->addr[4],prevSlaveNode->addr[3],
    	 	  	      	 	   	  	          prevSlaveNode->addr[2],prevSlaveNode->addr[1],prevSlaveNode->addr[0]
    	 	  	      	 	   	  	       );
    	 	  	      	 	   	    }
    	 	  	      	 	   	  
    	 	  	      	 	   	    ifFound = 88;
    	 	  	      	 	   	    break;
    	 	  	      	 	      }

            	 	  	      	prevSlaveNode = prevSlaveNode->next;
            	 	  	      }
            	 	  	    }
            	 	  	      	 
        	 	  	      	if (ifFound==88)
        	 	  	      	{
        	 	  	      	  if (debugInfo&PRINT_CARRIER_DEBUG)
        	 	  	      	  {
        	 	  	      	 	  printf("清除模块中所有表号\n");
        	 	  	      	  }
            	 	  	      	 	 
        	 	  	      	  switch (carrierModuleType)
        	 	  	      	  {
        	 	  	      	    case RL_WIRELESS:  //锐拔无线模块不支持参数区初始化,只支持删除所有从节点
        	 	  	      	    case SC_WIRELESS:  //赛康
        	 	  	      	 	    memset(tmpBuff, 0x0, 6);
        	 	  	      	 	    gdw3762Framing(ROUTE_SET_3762, 2, NULL, tmpBuff);
        	 	  	      	 	    break;

        	 	  	      	 	  case FC_WIRELESS:  //友迅达无线模块
        	 	  	      	      gdw3762Framing(INITIALIZE_3762, 3, NULL, NULL);
        	 	  	      	      break;
        	 	  	      	 	   
        	 	  	      	 	  default:           //其它支持参数区初始化的模块
        	 	  	      	      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
        	 	  	      	      break;
        	 	  	      	  }
                       	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;

            	            carrierFlagSet.checkAddrClear = 1;
            	            carrierFlagSet.querySlaveNum  = 1;
                          carrierFlagSet.synSlaveNode   = 1;
         	                carrierFlagSet.querySlaveNode = 0;
         	      	        carrierSlaveNode = NULL;
         	      	        prevSlaveNode = carrierSlaveNode;
            	 	  	      	   
            	 	  	      copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
            	 	  	      tmpNumOfSyn = 0;
               	      	  if (carrierModuleType==RL_WIRELESS)
               	      	  {
               	      	    carrierFlagSet.synMeterNo = 0;
               	      	  }
               	      	  else
               	      	  {
               	      	    carrierFlagSet.synMeterNo = 1;
               	      	  }
         	                goto breakPoint;
            	 	  	    }
            	 	  	    else
            	 	  	    {
    	 	  	      	 	    //2012-09-06,add
    	 	  	      	 	    //如果是晓程模块,查询是否存在集中器有的表号模块上不存在,如有这种情况需要做清除处理
    	 	  	      	 	    if (carrierModuleType==CEPRI_CARRIER && carrierFlagSet.numOfSalveNode>0)
    	 	  	      	 	    {
          	 	  	      	  ifFound = 0;
          	 	  	      	  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
          	 	  	      	  while (copyCtrl[port].tmpCpLink!=NULL)
          	 	  	      	  {
          	 	  	      	    prevSlaveNode = carrierSlaveNode;
          	 	  	      	    ifFound = 0;
      	 	  	      	 	      while(prevSlaveNode!=NULL)
      	 	  	      	 	      {
        	 	  	      	        //档案中本测量点有采集器地址
        	 	  	      	        if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, NULL, 1)==FALSE)
        	 	  	      	        {
        	 	  	      	          if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, prevSlaveNode->addr, 0)==TRUE)
        	 	  	      	          {
        	 	  	      	    	      ifFound = 1;
        	 	  	      	          }
      	 	  	      	 	  	    }
      	 	  	      	 	  	    else   //档案中本测量点是表地址
      	 	  	      	 	  	    {
        	 	  	      	          if (compareTwoAddr(copyCtrl[port].tmpCpLink->addr, prevSlaveNode->addr, 0)==TRUE)
        	 	  	      	          {
        	 	  	      	    	      ifFound = 2;
        	 	  	      	          }
      	 	  	      	 	  	    }
      	 	  	      	 	  	 
      	 	  	      	 	  	    //集中器档案与模块中本表地址相同
      	 	  	      	 	  	    if (ifFound!=0)
      	 	  	      	 	  	    {
      	 	  	      	 	  	      break;
      	 	  	      	 	  	    }
      	 	  	      	 	  	 
      	 	  	      	 	  	    prevSlaveNode = prevSlaveNode->next;
      	 	  	      	 	      }
              	 	  	      	 	   
          	 	  	      	 	  if (ifFound==0)
          	 	  	      	 	  {
          	 	  	      	 	   	if (debugInfo&PRINT_CARRIER_DEBUG)
          	 	  	      	 	   	{
          	 	  	      	 	   	  printf("集中器中表地址%02x%02x%02x%02x%02x%02x在本地通信模块中不存在\n",
          	 	  	      	 	   	  	     copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],copyCtrl[port].tmpCpLink->addr[3],
          	 	  	      	 	   	  	      copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]
          	 	  	      	 	   	  	    );
          	 	  	      	 	   	}
          	 	  	      	 	   	  
          	 	  	      	 	   	ifFound = 99;
          	 	  	      	 	   	break;
          	 	  	      	 	  }

          	 	  	      	 	  copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next; 
              	 	  	      }
              	 	  	      	   
              	 	  	      if (ifFound==99)
              	 	  	      {
                	 	  	      if (debugInfo&PRINT_CARRIER_DEBUG)
                	 	  	      {
                	 	  	      	printf("清除模块中所有表号\n");
                	 	  	      }
                	 	  	      	 	 
                	 	  	      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
                           	       
                           	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
    
                	            carrierFlagSet.checkAddrClear = 1;
                	            carrierFlagSet.querySlaveNum  = 1;
                              carrierFlagSet.synSlaveNode   = 1;
             	                carrierFlagSet.querySlaveNode = 0;
             	      	        carrierSlaveNode = NULL;
             	      	        prevSlaveNode = carrierSlaveNode;
                	 	  	      	   
                	 	  	      copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                	 	  	      tmpNumOfSyn = 0;
                       	      carrierFlagSet.synMeterNo = 1;
             	                     
             	                goto breakPoint;
              	 	  	      }
            	 	  	      }
            	 	  	    }

            	 	  	    carrierFlagSet.synSlaveNode = 2;
            	 	  	    copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
            	 	  	    tmpNumOfSyn = 0;
            	 	  	  }
            	 	  	  else
            	 	  	  {
        	 	  	      	carrierFlagSet.synSlaveNode   = 0;    //无测量点信息,无需同步
        	 	  	      	carrierFlagSet.querySlaveNode = 0;
        	 	  	      	 
        	 	  	      	if (debugInfo&PRINT_CARRIER_DEBUG)
        	 	  	      	{
        	 	  	      	  printf("没有测量点要同步\n");
        	 	  	      	}
            	 	  	  }
            	 	  	  break;
            	 	  	      
            	 	  	case 2:  //开始同步表号
    	 	  	    	    if (copyCtrl[port].tmpCpLink!=NULL)
    	 	  	    	    {
    	 	  	      	    while(copyCtrl[port].tmpCpLink!=NULL)
    	 	  	      	    {
    	 	  	      	      prevSlaveNode = carrierSlaveNode;
    	 	  	      	      ifFound = 0;
    	 	  	      	      hasCollector = 0;
    	 	  	      	    
    	 	  	      	      //ly,2011-07-19,去掉对SR判断的单独处理
    	 	  	      	      //SR无线模块不能查询子节点节点,每次都往模块里加,它自己去控制有没有这个地址
    	 	  	      	      //if (carrierModuleType==SR_WIRELESS)
    	 	  	      	      //{
    	 	  	      	      //	 if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE)
    	 	  	      	      //	 {
    	 	  	      	      //	 	 hasCollector = 1;  //有采集器地址,加采集器地址
    	 	  	      	      //	 }
    	 	  	      	      //}
    	 	  	      	      //else   //其它模块照常处理
    	 	  	      	      //{
    	 	  	      	    
    	 	  	      	      //2014-09-05,add,路灯控制器的载波控制点的采集器地址表示亮度占空比了,所以不能也不用比较采集器地址
    	 	  	      	     #ifndef LIGHTING
    	 	  	      	      //有采集器地址,加采集器地址
    	 	  	      	      if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE)
      	 	  	      	    {
      	 	  	      	      hasCollector = 1;
      	 	  	      	    }
      	 	  	      	   #endif
      	 	  	      	    
		  	 	  	      	    //查找是否模块中有本地址
		  	 	  	      	    while(prevSlaveNode!=NULL)
		  	 	  	      	    {
		  	 	  	      	      if (hasCollector==1)
		  	 	  	      	      {
			 	  	      	          if (compareTwoAddr(prevSlaveNode->addr, copyCtrl[port].tmpCpLink->collectorAddr, 0)==TRUE)
		  	 	  	      	        {
		  	 	  	      	    	    ifFound = 1;
		  	 	  	      	        }
		  	 	  	      	      }
		  	 	  	      	      else   //没有采集器地址则是载波表
		  	 	  	      	      {
			 	  	      	          if (compareTwoAddr(prevSlaveNode->addr, copyCtrl[port].tmpCpLink->addr, 0)==TRUE)
		  	 	  	      	        {
		  	 	  	      	    	    ifFound = 2;
		  	 	  	      	        }
		  	 	  	      	      }
		  	 	  	      	       
		  	 	  	      	      prevSlaveNode = prevSlaveNode->next;
		  	 	  	      	    }
			 	  	      	    
    	 	  	      	      if (carrierModuleType==CEPRI_CARRIER && ifFound==0 && carrierFlagSet.numOfSalveNode>0)
    	 	  	      	      {
    	 	  	      	    	  if (localModuleType!=CEPRI_CARRIER_3_CHIP)   //ly,2012-02-28,Add
    	 	  	      	        {
      	 	  	      	    	  if (debugInfo&PRINT_CARRIER_DEBUG)
      	 	  	      	    	  {
      	 	  	      	    		  printf("电科院模块:模块中有表地址,但与集中器中的地址不一致,删除所有数据后,重新添加地址\n");
      	 	  	      	    	  }
      	 	  	      	    	
    	 	  	      	          gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
               	              carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;

      	                      carrierFlagSet.checkAddrClear = 1;
      	                      carrierFlagSet.querySlaveNum  = 1;
                              carrierFlagSet.synSlaveNode   = 1;
   	                          carrierFlagSet.querySlaveNode = 0;
   	      	                  carrierSlaveNode = NULL;
   	      	                  prevSlaveNode = carrierSlaveNode;
    	 	  	      	   
    	 	  	      	          copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
    	 	  	      	          tmpNumOfSyn = 0;
           	      	          carrierFlagSet.synMeterNo = 1;
           	      	      
           	      	          goto breakPoint;
             	      	      }
           	      	      }
            	 	  	      	    
    	 	  	      	      if (carrierSlaveNode==NULL || ifFound==0)
    	 	  	      	      {
    	 	  	      	        if (hasCollector==1)
    	 	  	      	        {
    	 	  	      	          if (debugInfo&PRINT_CARRIER_DEBUG)
    	 	  	      	          {
    	 	  	      	            printf("添加采集器地址\n");
    	 	  	      	          }
    	 	  	      	          tmpNumOfSyn++;
    	 	  	      	          tmpBuff[0] = carrierFlagSet.synMeterNo&0xff;
    	 	  	      	          tmpBuff[1] = carrierFlagSet.synMeterNo>>8&0xff;
    	 	  	      	          tmpBuff[2] = 2;
    	 	  	      	          if (carrierModuleType==FC_WIRELESS)
    	 	  	      	          {
    	 	  	      	            gdw3762Framing(FC_NET_CMD_3762, 11, copyCtrl[port].tmpCpLink->collectorAddr, tmpBuff);
    	 	  	      	          }
    	 	  	      	          else
    	 	  	      	          {
    	 	  	      	            gdw3762Framing(ROUTE_SET_3762, 1, copyCtrl[port].tmpCpLink->collectorAddr, tmpBuff);
    	 	  	      	          }
               	              carrierFlagSet.cmdType = CA_CMD_ADD_SLAVE_NODE;
    	 	  	        	        carrierFlagSet.hasAddedMeter = 1;    //有向载波模块添加表地址
            	 	  	      	         
                              tmpSalveNode = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                              memcpy(tmpSalveNode->addr,copyCtrl[port].tmpCpLink->collectorAddr,6); //从节点地址
                              tmpSalveNode->next = NULL;
                              if (carrierSlaveNode==NULL)
                              {
                           	    carrierSlaveNode = tmpSalveNode;
                              }
                              else
                              {
                           	    prevSlaveNode = carrierSlaveNode;
                           	    while(prevSlaveNode->next!=NULL)
                           	    {
                           	   	  prevSlaveNode = prevSlaveNode->next;
                           	    }
                           	    prevSlaveNode->next = tmpSalveNode;
                              }
            	 	  	       	}
    	 	  	      	        else
    	 	  	      	        {
    	 	  	      	          if (debugInfo&PRINT_CARRIER_DEBUG)
    	 	  	      	          {
    	 	  	      	            printf("添加载波/无线表地址\n");
    	 	  	      	          }
    	 	  	      	         
    	 	  	      	          tmpNumOfSyn++;
    	 	  	      	         
    	 	  	      	          tmpBuff[0] = carrierFlagSet.synMeterNo&0xff;
    	 	  	      	          tmpBuff[1] = carrierFlagSet.synMeterNo>>8&0xff;
    	 	  	      	          if (copyCtrl[port].tmpCpLink->protocol==1)
    	 	  	      	          {
    	 	  	      	            tmpBuff[2] = 1;    //97规约
    	 	  	      	          }
    	 	  	      	          else
    	 	  	      	          {
    	 	  	      	            tmpBuff[2] = 2;    //07规约
    	 	  	      	          }
    	 	  	      	          if (carrierModuleType==FC_WIRELESS)
    	 	  	      	          {
    	 	  	      	            gdw3762Framing(FC_NET_CMD_3762, 11, copyCtrl[port].tmpCpLink->addr, tmpBuff);
    	 	  	      	          }
    	 	  	      	          else
    	 	  	      	          {
    	 	  	      	            gdw3762Framing(ROUTE_SET_3762, 1, copyCtrl[port].tmpCpLink->addr, tmpBuff);
    	 	  	      	          }
               	              carrierFlagSet.cmdType = CA_CMD_ADD_SLAVE_NODE;
    	 	  	      	          carrierFlagSet.hasAddedMeter = 1;    //有向载波模块添加表地址
    	 	  	      	        }
                            carrierFlagSet.studyRouting   = 0;
                            //carrierFlagSet.routeRunStatus = 1;
            	 	  	      	 
            	 	  	      	showInfo("同步本地通信模块地址");

            	 	  	      	goto breakPoint;
            	 	  	      }
            	 	  	      	    
            	 	  	      copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
            	 	  	    }
            	 	  	  }
            	 	  	  else
            	 	  	  {
            	 	  	    showInfo("同步地址完成");
	 	  	    		        if (debugInfo&PRINT_CARRIER_DEBUG)
	 	  	    		        {
	 	  	    		          printf("载波/无线从节点同步完成,模块中节点数为%d\n", carrierFlagSet.synMeterNo-1);
	 	  	    		        }

                    	  //如果路由主导抄表方式,且有表地址档案
                        if (carrierFlagSet.routeLeadCopy==1 && carrierFlagSet.synMeterNo>1)
                        {
                       	  copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 5);
                       	  
                       	  if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  {
                       	 	  printf("路由主导抄表:5秒后开始\n");
                       	  }
                        }

    	 	  	    		    if (tmpNumOfSyn>0 && !(carrierModuleType==SR_WIRELESS || carrierModuleType==FC_WIRELESS || localModuleType==CEPRI_CARRIER_3_CHIP /*2012-02-28,add电科院判断*/))
    	 	  	    		    {
    	 	  	    		      carrierFlagSet.routeRunStatus = 1;
    	 	  	    		    }

    	 	  	    		    carrierFlagSet.synSlaveNode   = 0;
        	 	  	      	carrierFlagSet.querySlaveNode = 0;
        	 	  	      	carrierFlagSet.chkNodeBeforeCopy = 2;    //已经比对过表地址标识,2013-12-30,add

    	 	  	    		    switch (carrierModuleType)
    	 	  	    		    {
    	 	  	    		      case SR_WIRELESS:    //SR无线模块,最长组网时间为3分钟
	  		 	 	  		  	      if (tmpNumOfSyn>0)
      	 	  	    		      {
      	 	  	      	 	      carrierFlagSet.wlNetOk = 0;
      	 	  	      	 	    }
              	 	  	      	 	 
		 	 	  		  	          if (localModuleType==SR_WF_3E68)
		 	 	  		  	          {
		 	 	  		  	            carrierFlagSet.routeRunStatus = 1;

		 	 	  		  	            carrierFlagSet.netOkTimeOut = nextTime(sysTime, SRWF_NET_OK_TIME_OUT, 0);
		 	 	  		  	          }
		 	 	  		  	          else
		 	 	  		  	          {
		 	 	  		  	            carrierFlagSet.netOkTimeOut = nextTime(sysTime, 3, 0);
		 	 	  		  	          }
		 	 	  		  	          break;
     	  		 	 	  		  	     
     	  		 	 	  	      case CEPRI_CARRIER:    //有向模块添加表地址/采集器地址,需要开始搜表,ly,10-08-27
                            if (carrierFlagSet.hasAddedMeter==1)
                            {
                              //搜索时间暂定为50分钟 ly,2011-06-07
                              //carrierFlagSet.foundStudyTime = nextTime(sysTime, 50, 0);
                              //搜索时间暂定为120分钟 ly,2011-07-18
                              if (localModuleType!=CEPRI_CARRIER_3_CHIP)
                              {
                                carrierFlagSet.foundStudyTime = nextTime(sysTime, 120, 0);
                              }
                              else
                              {
                                //新版本搜索时间定为240分钟(4小时) ly,2012-02-28
                                carrierFlagSet.foundStudyTime = nextTime(sysTime, 240, 0);
                              }
          	 	  	      	   
          	 	  	            carrierFlagSet.searchMeter = 1;
          	 	  	      	   
          	 	  	            printf("电科院模块准备搜表\n");
          	 	  	          }
          	 	  	          break;
            	 	  	      	   
            	 	  	      case RL_WIRELESS:
    	 	  	      	 	      tmpData = (carrierFlagSet.synMeterNo-1)*20+120;
    	 	  	      	 	      if (tmpData<360)
    	 	  	      	 	      {
    	 	  	      	 	  	    tmpData = 360;
    	 	  	      	 	      }
    	 	  	      	 	  
    	 	  	      	 	      carrierFlagSet.netOkTimeOut = nextTime(sysTime, tmpData/60 ,tmpData%60);
    	 	  	      	 	      carrierFlagSet.delayTime = nextTime(sysTime, 2, 0);
    	 	  	      	 	  
    	 	  	      	 	      if (tmpNumOfSyn>0)
    	 	  	    		        {
    	 	  	      	 	        carrierFlagSet.wlNetOk = 0;
    	 	  	      	 	      }
    	 	  	      	 	  
    	 	  	      	 	      if (debugInfo&PRINT_CARRIER_DEBUG)
    	 	  	      	 	      {
    	 	  	      	 	  	    printf("等待两分钟查询组网状态,等待组网结束的最晚时间是:%d-%d-%d %d:%d:%d\n",carrierFlagSet.netOkTimeOut.year,carrierFlagSet.netOkTimeOut.month,carrierFlagSet.netOkTimeOut.day,carrierFlagSet.netOkTimeOut.hour,carrierFlagSet.netOkTimeOut.minute,carrierFlagSet.netOkTimeOut.second);
    	 	  	      	 	      }
    	 	  	      	 	      break;
            	 	  	      	 	 
            	 	  	      case FC_WIRELESS:
     	  		 	 	  		      if (tmpNumOfSyn>0)
      	 	  	    		      {
      	 	  	      	 	      carrierFlagSet.wlNetOk = 0;
      	 	  	      	 	    }
  
		 	 	  		  	         #ifdef DKY_SUBMISSION
		 	 	  		  	          carrierFlagSet.netOkTimeOut = nextTime(sysTime, 5, 0);
		 	 	  		  	         #else 
		 	 	  		  	          //carrierFlagSet.netOkTimeOut = nextTime(sysTime, 30, 0);
		 	 	  		  	       
		 	 	  		  	          //2014-02-28,从30分钟改成用搜表最长时间控制强制组网结束
		 	 	  		  	          if ((assignCopyTime[0]|assignCopyTime[1]<<8)<240)
		 	 	  		  	          {
		 	 	  		  	            carrierFlagSet.netOkTimeOut = nextTime(sysTime, 240, 0);
		 	 	  		  	          }
		 	 	  		  	          else
		 	 	  		  	          {
		 	 	  		  	            carrierFlagSet.netOkTimeOut = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0);
		 	 	  		  	          }
		 	 	  		  	         #endif
                                 
                            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 30);
            	 	  	      	 	 	             	 	  	      	 	 	 
            	 	  	      	break;
            	 	  	    }
            	 	  	  }
            	 	  	  break;
            	 	  }
            	 	  break;
            	}
            }

            //2-3-1.1-16.等待无线模块组网
            if (carrierFlagSet.wlNetOk<3)
            {
            	switch(carrierModuleType)
            	{
            	  case RL_WIRELESS:    //锐拔模块上电查询组网状态,等待3*2Min
              	 	if (compareTwoTime(carrierFlagSet.netOkTimeOut, sysTime))
              	 	{
          	 	 	 	  //读未入网节点号
          	 	 	 	  gdw3762Framing(RL_EXTEND_3762, 4, NULL, NULL);
  
                    usleep(100000);
  
          	 	 	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
          	 	 	 	  {
          	 	 	 	 	  printf("等待组网时间超过(节点数*20+120)s,发送HDLC停止组网命令\n");
          	 	 	 	  }
  
              	 	 	carrierFlagSet.wlNetOk = 3;
                    carrierFramex[0] = 0x7E;
                    carrierFramex[1] = 0xF8;
                    carrierFramex[2] = 0x0D;
                    memcpy(&carrierFramex[3], carrierFlagSet.mainNodeAddr, 6);                     
                    carrierFramex[9] = 0x04;
                    memset(&carrierFramex[10],0x00,6);                     
                    carrierFramex[16] = 0xcc;
                    carrierFramex[17] = 0xcc;
                 
                    sendMeterFrame(31, carrierFramex, 18);
                 
                    usleep(50000);
              	 	 	 
              	 	 	goto breakPoint;
              	 	}
              	 	 	 
          	 	 	  if (compareTwoTime(carrierFlagSet.delayTime, sysTime))
          	 	 	  {
          	 	 	    gdw3762Framing(RL_EXTEND_3762, 6, NULL, NULL);
          	 	  	 
          	 	  	  carrierFlagSet.delayTime = nextTime(sysTime, 2, 0);
          	 	 	  }
          	 	 	  break;
              	 	 
              	case SR_WIRELESS:    //SR模块初始化后的组网等待及未入网节点的查询,ly,2011-04-28,上海SR,add
                  if (compareTwoTime(carrierFlagSet.netOkTimeOut, sysTime))
                  {
   	  		 	 	  	  if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  {
  		 	 	  		      if (localModuleType==SR_WF_3E68)
  		 	 	  		      {
  		 	 	  		        printf("SR - %d分钟入网节点数量无变化,结束组网。读未入网节点\n", SRWF_NET_OK_TIME_OUT);
  		 	 	  		      }
  		 	 	  		      else
  		 	 	  		      {
  		 	 	  		        printf("SR - 3分钟入网节点数量无变化,结束组网。读未入网节点\n");
  		 	 	  		      }
   	  		 	 	  	  }
       	  		 	 	  	 
                    carrierFlagSet.wlNetOk = 3;
                       
                    //清除以前的未入网节点信息
                    while (noFoundMeterHead!=NULL)
                    {
                      tmpFound = noFoundMeterHead;
                      noFoundMeterHead = noFoundMeterHead->next;
                           
                      free(tmpFound);
                    }
                             
                	  //读未入网节点
                	  gdw3762Framing(RL_EXTEND_3762, 4, NULL, NULL);
                    carrierFlagSet.cmdType = CA_CMD_READ_NO_NET_NODE;
    
                	  goto breakPoint;
                  }
                     
                	//读未入网节点号
                  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
                  {
                   	carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
                    carrierFlagSet.routeRunStatus = 1;
                  }
                  break;
                   
                case FC_WIRELESS:
                	//读CAC状态信息
                  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
                  {
                   	carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 20);
                    carrierFlagSet.routeRunStatus = 1;

   	  		 	 	  	  if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  {
        	 	  	      printf("友迅达 - 等待组网结束的最晚时间是:%d-%d-%d %d:%d:%d\n", carrierFlagSet.netOkTimeOut.year,carrierFlagSet.netOkTimeOut.month,carrierFlagSet.netOkTimeOut.day,carrierFlagSet.netOkTimeOut.hour,carrierFlagSet.netOkTimeOut.minute,carrierFlagSet.netOkTimeOut.second);
            	 	  	}
                  }
                   	 

                  if (compareTwoTime(carrierFlagSet.netOkTimeOut, sysTime))
                  {
   	  		 	 	  	  showInfo("强制组网结束");
   	  		 	 	  	 
   	  		 	 	  	  if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  {
   	  		 	 	  		  printf("友讯达 - 长时间组网未完成,强制结束组网\n");
   	  		 	 	  	  }
       	  		 	 	  	 
                    carrierFlagSet.wlNetOk = 3;
                       
                    //清除以前的未入网节点信息
                    while (noFoundMeterHead!=NULL)
                    {
                      tmpFound = noFoundMeterHead;
                      noFoundMeterHead = noFoundMeterHead->next;
                     
                      free(tmpFound);
                    }
                  }
             	    break;
                   
                default:    //其他载波模块无需要等待组网
                  carrierFlagSet.wlNetOk = 3;
                  break;
              }
            	 	 
            	goto breakPoint;
            }

   	 	      //2-3-1.1-17. 手动搜表命令已发出,但处于路由学习状态,先停止路由学习
   	 	      if (carrierFlagSet.searchMeter==1 && carrierFlagSet.studyRouting==2)
   	 	      {
    	 	      //停止学习命令
    	 	 	    gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);

    	 	 	    carrierFlagSet.studyRouting = 3;  //命令停止学习
    	 	 	
    	 	 	    goto breakPoint;
   	 	      }

            //2-3-1.1-18.学习路由
            switch (carrierFlagSet.studyRouting)
            {
        	 	  case 1:   //开始学习路由
        	 	 	  //2012-08-13,add,无线模块不需要学习
        	 	 	  if (carrierModuleType==SC_WIRELESS
        	 	 	 	    || carrierModuleType==FC_WIRELESS
        	 	 	 	     || carrierModuleType==SR_WIRELESS
        	 	 	 	   )
        	 	 	  {
        	 	 	    carrierFlagSet.studyRouting = 0;
        	 	 	    break;
        	 	 	  }
        	 	 	 
        	 	 	  carrierFramex[0] = 0x1;   //工作模式(不允许注册,工作状态为学习)
        	 	 	  carrierFramex[1] = 0x0;
        	 	 	  carrierFramex[2] = 0x0;
        	 	 	 
        	 	    gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
        	 	    goto breakPoint;
        	 	    break;
            	 	 
        	 	  case 2:   //路由学习中,查询学习状态
        	 	 	  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
        	 	 	  {
        	 	 	    carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
        	 	 	    carrierFlagSet.routeRunStatus = 1;
        	 	 	    goto breakPoint;
        	 	 	  }
        	 	 	  if (compareTwoTime(carrierFlagSet.foundStudyTime, sysTime))
        	 	 	  {            	 	 		 
        	 	 	    //停止学习命令
        	 	 	    gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);

        	 	 	    carrierFlagSet.studyRouting = 3;  //命令停止学习
        	 	 		 
        	 	 	    goto breakPoint;
        	 	 	  }
        	 	 	  break;
            	 	 	 
        	 	  case 3:
        	 	 	  //2012-08-13,add无线自组网模块不需要学习,也就不需要学习了
        	 	 	  if (carrierModuleType==SC_WIRELESS
        	 	 	 	    || carrierModuleType==FC_WIRELESS
        	 	 	 	     || carrierModuleType==SR_WIRELESS
        	 	 	 	   )
        	 	 	  {
        	 	 	    carrierFlagSet.studyRouting = 0;
        	 	 	    break;
        	 	 	  }

        	 	 	  //停止学习命令
        	 	 	  gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
        	 	 	  goto breakPoint;
        	 	 	  break;
            }
            if (carrierFlagSet.studyRouting>0)
            {
            	goto breakPoint;
            }
            	 
            //2-3-1.1-19.搜表
            switch (carrierFlagSet.searchMeter)
            {
        	 	  case 1:
		 	 	        //if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
		 	 	        //2012-08-13,add,友讯达也不需要搜表
				 	 	    if (carrierModuleType==SR_WIRELESS 
				 	 	  	    || carrierModuleType==HWWD_WIRELESS 
				 	 	  	     || carrierModuleType==RL_WIRELESS
				 	 	  	      || carrierModuleType==FC_WIRELESS
				 	 	  	   )
				 	 	    {
				 	 		    carrierFlagSet.searchMeter = 0;
				 	 		    break;
				 	 	    }
    	  		
	    	  		  //ly,2012-02-28,添加,新版本电科院模块不用设置工作模式
	    	  		  if (carrierModuleType==CEPRI_CARRIER && localModuleType==CEPRI_CARRIER_3_CHIP)
	    	  		  {
	    	  		    carrierFlagSet.searchMeter = 2;
	    	  		    goto breakPoint;
	    	  		  }
	    	  		
	    	  		  if (menuInLayer==0)
	    	  		  {
	    	  		    defaultMenu();
	    	  		  }

	  	 	 	      carrierFramex[0] = 0x2;                        //工作模式(允许注册,工作状态为抄表)
	  	 	 	      carrierFramex[1] = 0x0;
	  	 	 	      carrierFramex[2] = 0x0;
          	 	 	 
          	 	  gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
          	 	  carrierFlagSet.cmdType = CA_CMD_SET_WORK_MODE;
            	 	break;
            	 	  	
            	case 2:
        	 	    searchStart = sysTime;                         //搜索开始时间
        	 	    carrierFramex[0] = hexToBcd(sysTime.second);   //开始时间
        	 	    carrierFramex[1] = hexToBcd(sysTime.minute);
        	 	    carrierFramex[2] = hexToBcd(sysTime.hour);
        	 	    carrierFramex[3] = hexToBcd(sysTime.day);
        	 	    carrierFramex[4] = hexToBcd(sysTime.month);
        	 	    carrierFramex[5] = hexToBcd(sysTime.year);
        	 	     
        	 	    carrierFramex[6] = 0;                          //持续时间1
        	 	    carrierFramex[7] = 5;                          //持续时间2
        	 	    carrierFramex[8] = 1;                          //从节点重发次数
        	 	    carrierFramex[9] = 1;                          //随机等待时间片个数

        	 	    gdw3762Framing(ROUTE_SET_3762, 5, NULL, carrierFramex);
        	 	    carrierFlagSet.cmdType = CA_CMD_ACTIVE_REGISTER;
        	 	    break;
            	 	    
        	 	  case 3:
        	 	    carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
        	 	    carrierFlagSet.activeMeterNo   = 1;
        	 	    
        	 	    //ly,10-08-27,适应晓程
        	 	 	  carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
                carrierFlagSet.routeRunStatus  = 1;

        	 	  	if (carrierModuleType==MIA_CARRIER)
        	 	  	{
        	 	  	  gdw3762Framing(CTRL_CMD_3762, 2, NULL, NULL);    //允许载波从节点上报
        	 	  	  carrierFlagSet.cmdType = CA_CMD_PERMIT_ACTIVE;
        	 	  	} 
        	 	  	else
        	 	  	{
        	 	  	  gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);  //重启搜表
        	 	  	  carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
        	 	  	}
        	 	   	break;

            	case 4:
    	 	 	      //准备停止搜表
    	 	 	      if (compareTwoTime(carrierFlagSet.foundStudyTime, sysTime))
    	 	 	      {
		    	 	 		  if (debugInfo&PRINT_CARRIER_DEBUG)
		    	 	 		  {
		    	 	 		   	printf("最长搜表时间已到,停止搜表\n");
		    	 	 		  }
    	 	 		   
      	 	 		    //停止搜表
    	 	 		      gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
    	 	 		      carrierFlagSet.cmdType = CA_CMD_PAUSE_WORK;
                  searchEnd = sysTime;                        //搜索结束时间

	    	 	 		    //if (carrierModuleType!=CEPRI_CARRIER)
	    	 	 		    //{
	    	 	 		    //  carrierFlagSet.searchMeter = 0;
	    	 	 		    //}
	  		 	 	  		 
			  		 	 	  if (menuInLayer==0)
			  		 	 	  {
			  		 	 	  	defaultMenu();
			  		 	 	  }
                  searchMeter(0xff);
               
	                //ly,2011-06-07,取消重启
		              //ly,2011-12-02,普光王立政反应这种复位效果对电科院模块较好,所以又打开了重启
		              //if (carrierModuleType==CEPRI_CARRIER && carrierFlagSet.autoSearch==1)
		              if (carrierModuleType==CEPRI_CARRIER)
		              {
		               	//ly,2012-02-28,添加,新版本的搜完表后不再复位了
		               	if (localModuleType!=CEPRI_CARRIER_3_CHIP)
		               	{
		               	  if (debugInfo&PRINT_CARRIER_DEBUG)
		               	  {
		               	 	  printf("电科院模块:自动搜完表后重启模块\n");
		               	  }
		               	 
		               	  sleep(1);
		               	 
		               	  ifReset=TRUE;
		               	}
		              }
               
                  goto breakPoint;
    	 	 	      }
    	 	 	  
    	 	 	      //搜表过程中,查询路由运行状态,ly,10-08-28
    	 	 	      if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
    	 	 	      {
    	 	 	 	      carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
    	 	 	 	      carrierFlagSet.routeRunStatus = 1;
    	 	 	 	      goto breakPoint;
    	 	 	      }
    	 	 	  
    	 	 	      /*
    	 	 	      //如果是东软载波模块,查询主动注册的载波从节点信息,东软已经作废该指令
    	 	 	      if (carrierModuleType==EAST_SOFT_CARRIER)
    	 	 	      {
    	 	 	        if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
    	 	 	        {
    	 	 	 	        carrierFramex[0] = carrierFlagSet.activeMeterNo;   //从节点起始序号
    	 	 	 	        carrierFramex[1] = 0x0;
    	 	 	 	        carrierFramex[2] = 0x5;
    	 	 	 	     
    	 	 	 	        gdw3762Framing(ROUTE_QUERY_3762, 6, NULL, carrierFramex);
    	 	 	 	  
    	 	 	 	        goto breakPoint;
    	 	 	 	      }
    	 	 	      }
    	 	 	      */
    	 	 	      break;
            	 	 	
            	case 0x05:
      	 	 	    carrierFramex[0] = 0x0;   //工作模式(不允许注册,工作状态为抄表)
      	 	 	    carrierFramex[1] = 0x0;
      	 	 	    carrierFramex[2] = 0x0;
          	 	 	 
          	 	  gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
          	 	  carrierFlagSet.cmdType = CA_CMD_SET_WORK_MODE;
          	 	  goto breakPoint;
            	 	break;
            	 	 		
              case 0x06:
        	 	  	gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);  //重启抄表
        	 	  	carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
        	 	  	break;
            }
            	 
    	  	  //2-3-1.1-20.比较,指定的抄表时间是否已到(如果是福星晓程模块,则在搜表时不能中断搜表,等搜表完成后再抄表)
    	  	  if (compareTwoTime(copyCtrl[port].nextCopyTime, sysTime)
    	  	     	&& !(carrierFlagSet.searchMeter>0 && carrierModuleType==CEPRI_CARRIER))
    	  	  {
              if (carrierModuleType==FC_WIRELESS && carrierFlagSet.readStatus<2)
              {
            	 	if (carrierFlagSet.readStatus==0)
            	 	{
                  carrierFlagSet.operateWaitTime = sysTime;
            	 	}
            	 	 	 	
            	 	carrierFlagSet.readStatus = 1;
            	 	 	 
            	 	if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
            	 	{
    	 	 	 	   		carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
    	 	 	 	      carrierFlagSet.routeRunStatus = 1;
        	 	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
        	 	 	    {
        	 	 	      printf("友迅达无线模块 - 抄表前查询模块状态\n");
        	 	 	    }
        	 	 	  }
                 	 
                goto breakPoint;
              }
                 	
              //查看是否在允许的抄表时段内
              ifFound = queryIfInCopyPeriod(4);
                 
              //在抄表时段内才可以开始抄表
              if (ifFound==1)
              {
                if (debugInfo&PRINT_CARRIER_DEBUG)
                {
                  printf("在允许抄表时段内,");
                }
             
                if (copyCtrl[port].cmdPause==TRUE)
                {
                  //copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);
                
                  copyCtrl[port].nextCopyTime = nextTime(sysTime, 1, 0);                       
                  //2012-12-11,在现场运行发现,集中器载波端口极少数量情况下,会未读到表地址而未抄表
                  //           修改为在无表地址时,1分钟读一次,直到读到表地址为止
                  checkCarrierMeter();
               
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("但本端口暂停抄表\n");
                  }
                }
                else
                {
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("且本端口未暂停抄表\n");
                  }
               
                  if(!(carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS))
                  {
                    if (stopCarrierNowWork()==TRUE)
                    {
                      goto breakPoint;
                    }
                  }

                  //初始化本端口电表链表
                  copyCtrl[port].round = 1;          //第一轮
               
                  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                  while(copyCtrl[port].tmpCpLink!=NULL)
                  {
           	        copyCtrl[port].tmpCpLink->thisRoundCopyed = FALSE;  //本次抄表是否成功置为false
           	        copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                  }
               
                  //集中器主导抄表的话,判断该抄哪类数据
                  if (carrierFlagSet.routeLeadCopy==0)
                  {
                    copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                    copyCtrl[port].ifCopyLastFreeze = 0;
                 
                    while(copyCtrl[port].tmpCpLink!=NULL)
                    {
      	              if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
         	            {
        	             #ifndef DKY_SUBMISSION
			 	 	  	       	  //2012-09-28,为适应象甘肃敦煌,兰州榆中的没有日冻结的07表而增加if处理,同时取消类号为0x55的设置,因为这会在主站端操作特别麻烦
			 	 	  	       	  //  修改为:设置居民用户表抄读模式为0x55(仅实时数据)和0xaa(仅实时总示值)来控制抄读项
			 	 	  	       	  if (denizenDataType!=0x55 && denizenDataType!=0xaa)
			 	 	  	       	  {    
		 	  	       	        //2012-08-31,为精减抄读数据项而增加if处理
		 	  	       	        //  如果大小类号都为0的话,日冻结采集成功,就不再采集其他数据项了
		 	  	       	        //  更改以后对严格执行标准的07表的默认处理方式为只采集日冻结数据
	          	            if (copyCtrl[port].tmpCpLink->bigAndLittleType!=0x00)
	          	            {
	          	         #endif
        	             
        	                  //检查是否有未采集的小时冻结数据
        	                  if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
        	                  {
        	                    copyCtrl[port].ifCopyLastFreeze = 3;
        	                    break;
        	                  }
        	           
        	             #ifndef DKY_SUBMISSION
        	                }
        	             #endif
        	           
        	                //0:10分后查找是否有日冻结数据,如果没有日冻结数据,07表需要抄读电能表上一日冻结数据
             	            if ((sysTime.hour==0 && sysTime.minute>=10) || sysTime.hour>0)
             	            {
      	                    //检测是否有当日 日冻结数据
      	                    if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
      	                    {
      	               	      copyCtrl[port].ifCopyLastFreeze = 2;
      	               	      break;
      	                    }
             	            }
             	        
        	             #ifndef DKY_SUBMISSION
        	              }
        	             #endif
             	        }
             	     
             	        copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                    }
                  }
           
                  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                  if (copyCtrl[port].tmpCpLink==NULL)
                  {
                    //copyCtrl[port].nextCopyTime = nextTime(sysTime,0,20);
                 
                    copyCtrl[port].nextCopyTime = nextTime(sysTime, 1, 0);
                    //2012-12-11,在现场运行发现,集中器载波端口极少数量情况下,会未读到表地址而未抄表
                    //           修改为在无表地址时,1分钟读一次,直到读到表地址为止
                    checkCarrierMeter();
                 
                    //2013-12-30,发现这里还是有问题,虽然用这种方法可靠的读到了表地址,但是并没有同步到路由模块
                    //           申请同步到模块中
                    if (copyCtrl[port].tmpCpLink!=NULL)
                    {
                 	    carrierFlagSet.synSlaveNode = 1;
                    }
                       
            	      if (debugInfo&PRINT_CARRIER_DEBUG)
            	      {
              	      printf("端口31(载波端口)无表可抄\n",port+1);
              	    }
                  }
                  else
                  {
                    //检查单相数据存储表是否存在
                    checkSpRealTable(0);

                    //检查三相数据存储表是否存在
                    checkSpRealTable(1);
 
                    //检查整点冻结数据表是否存在
                    checkSpRealTable(2);

                    //根据集中器主导抄表还是路由主导抄表来进行相应的启动抄表工作
                    if (carrierFlagSet.routeLeadCopy==1)   //路由主导抄表
                    {
                 	    //如果模块为学习状态,置为抄表状态
                 	    if (carrierFlagSet.workStatus<=0)
                 	    {
                 	      carrierFlagSet.workStatus=1;
                 	    
                 	      if (debugInfo&PRINT_CARRIER_DEBUG)
                 	      {
                 	   	    printf("路由主导抄读:设置工作模式为抄表\n");
                 	      }
                	 	 	  carrierFramex[0] = 0x0;   //工作模式(不允许注册,工作状态为抄表)
                	 	 	  carrierFramex[1] = 0x0;
                	 	 	  carrierFramex[2] = 0x0;
                    	 	 	 
                    	 	gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
                    	 	carrierFlagSet.cmdType = CA_CMD_SET_WORK_MODE;
                    	 	goto breakPoint;
                      }
                         
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("路由主导抄读 - 模块工作状态为%d\n", carrierFlagSet.workStatus);
                      }
                         
                 	    //如果模块确认为抄表方式后,发重启抄表命令
                 	    if (carrierFlagSet.workStatus==2 || carrierFlagSet.workStatus==5)
                 	    {
                 	      //2012-08-27,add,为了只统计当天的抄读成功数据
                 	      //2013-12-30,去掉这一句,用下面的判断来对比表地址会更可靠
                 	      //checkCarrierMeter();                       	   
                 	      if (0==carrierFlagSet.chkNodeBeforeCopy)
                 	      {
                 	   	    if (TC_CARRIER==carrierModuleType)
                 	   	    {
                 	   	      if (debugInfo&PRINT_CARRIER_DEBUG)
                 	   	      {
                 	   	 	      printf("路由主导抄读:鼎信载波,启动抄表前复位载波模块\n");
                 	   	      }
                 	   	   
                 	   	      close(fdOfCarrier);
                 	   	    
                 	   	      carrierUartInit();
                 	   	   
                 	   	      sleep(5);
                 	   	    }
                 	   	 
                 	   	    carrierFlagSet.chkNodeBeforeCopy = 1;
                 	   	    carrierFlagSet.synSlaveNode = 1;
                 	   	    carrierFlagSet.querySlaveNode = 0;
                 	   	 
                 	   	    if (debugInfo&PRINT_CARRIER_DEBUG)
                 	   	    {
                 	   	 	    printf("路由主导抄读:启动抄表前申请比对节点档案\n");
                 	   	    }
                 	   	 
                 	   	    goto breakPoint;
                 	      }
                     
                        carrierFlagSet.synSlaveNode   = 0;
                        carrierFlagSet.querySlaveNode = 0;
 
                 	      carrierFlagSet.workStatus = 2;
                 	   
                 	      if (debugInfo&PRINT_CARRIER_DEBUG)
                 	      {
                 	   	    printf("路由主导抄读:重启抄表命令\n");
                 	      }

                 	      gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);
                 	      carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
                 	      carrierFlagSet.reStartPause = 0;

                 	      goto breakPoint;
                 	    }
                    }
                    else                                   //集中器主导抄表
                    {
   	  	     	        //采集上一次日冻结数据/或是整点冻结数据时要移位到07规约表开始抄
   	  	     	        if (copyCtrl[port].ifCopyLastFreeze==2 || copyCtrl[port].ifCopyLastFreeze==3)
   	  	     	        {
   	  	     	          while(copyCtrl[port].tmpCpLink!=NULL)
   	  	     	          {
   	  	     	 	          if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
   	  	     	 	          {
   	  	     	 	  	        if (copyCtrl[port].ifCopyLastFreeze==3)
   	  	     	 	  	        {
              	             #ifndef DKY_SUBMISSION
 	 	  	       	              //2012-09-28,为适应象甘肃敦煌,兰州榆中的没有日冻结的07表而增加if处理,同时取消类号为0x55的设置,因为这会在主站端操作特别麻烦
 	 	  	       	              //  修改为:设置居民用户表抄读模式为0x55(仅实时数据)和0xaa(仅实时总示值)来控制抄读项
 	 	  	       	              if (denizenDataType!=0x55 && denizenDataType!=0xaa

     	 	  	       	               //2012-08-31,为精减抄读数据项而增加if处理
     	 	  	       	               //  如果大小类号都为0的话,日冻结采集成功,就不再采集其他数据项了
     	 	  	       	               //  更改以后对严格执行标准的07表的默认处理方式为只采集日冻结数据
          	             	         && copyCtrl[port].tmpCpLink->bigAndLittleType!=0x00
          	             	       )
          	                  {
                	           #endif

        	                      //缺当的整点冻结数据的话就采集该表的整点冻结数据
        	                      if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
        	                      {
        	                   	    break;
        	                      }
        	                  
        	                   #ifndef DKY_SUBMISSION
        	                    }
        	                   #endif
   	  	     	 	  	        }
       	  	     	 	  	    else
       	  	     	 	  	    {
       	  	     	 	  	       	
                     	       #ifndef DKY_SUBMISSION
     	 	  	       	          //2012-09-28,为适应象甘肃敦煌,兰州榆中的没有日冻结的07表而增加if处理,同时取消类号为0x55的设置,因为这会在主站端操作特别麻烦
     	 	  	       	          //  修改为:设置居民用户表抄读模式为0x55(仅实时数据)和0xaa(仅实时总示值)来控制抄读项
     	 	  	       	          if (denizenDataType!=0x55 && denizenDataType!=0xaa)
     	 	  	       	          {

                     	       #endif
                     	          
            	                  //缺上一次日冻结数据的话就采集该表的日冻结数据
            	                  if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
            	                  {
       	  	     	 	  	          break;
       	  	     	 	  	        }
       	  	     	 	  	           
       	  	     	 	  	     #ifndef DKY_SUBMISSION
       	  	     	 	  	      }
       	  	     	 	  	     #endif
       	  	     	 	  	    }
       	  	     	 	      }
       	  	     	 	      copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
       	  	     	      }
       	  	     	         
       	  	     	      if (copyCtrl[port].tmpCpLink==NULL)
       	  	     	      {
   	  	     	         	  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
   	  	     	         	  copyCtrl[port].ifCopyLastFreeze = 0;
       	  	     	      }
       	  	     	    }
                         
                      //如果采集实时数据,移位到上一次未采集完成的测量点开始采集
                      if (copyCtrl[port].ifCopyLastFreeze==0)
                      {
                        if (copyCtrl[port].backMp>0)
                        {
                         	while(copyCtrl[port].tmpCpLink!=NULL)
                         	{
                     	 	 	  if (copyCtrl[port].tmpCpLink->mp==copyCtrl[port].backMp)
                     	 	 	  {
                     	 	 	    if (debugInfo&METER_DEBUG)
                     	 	 	    {
                     	 	 	 	    printf("移位到上一次未采集完成的测量点%d\n", copyCtrl[port].backMp);
                     	 	 	    }
                     	 	 	    break;
                     	 	 	  }
                         	 	 	 
       	  	     	 	        copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                         	}
                        }
                      }
                         
                      initCopyItem(port, copyCtrl[port].ifCopyLastFreeze);
                         
                      if (menuInLayer==0)
                      {
                        defaultMenu();
                      }
                         
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("端口31开始抄表\n");
                      }
                         
                     #ifdef PLUG_IN_CARRIER_MODULE
                      #ifndef MENU_FOR_CQ_CANON
                       showInfo("终端正在抄表...");
                      #endif
                     #endif
                    }
                  }
                }
              
   	 	          goto breakPoint;
   	 	        }
   	 	        else
   	 	        {
                copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);

                if (debugInfo&PRINT_CARRIER_DEBUG)
                {
                  printf("不在允许抄表时段内,抄表时间向后推移20秒\n");
                }
	 	          }
   	 	      }
          }
          else       //判断发送超时
          {
            if (carrierFlagSet.sending==1)
            {
             	if (compareTwoTime(carrierFlagSet.cmdTimeOut, sysTime))
             	{
             	  carrierFlagSet.retryCmd = 1;    //命令超时,请求重发
	              if (carrierFlagSet.hardwareReest == 1)
  		 	 	  	  {
  		 	 	  	    carrierFlagSet.hardwareReest = 2;
  		 	 	  	  }
             	  goto breakPoint;
             	}
            }
          }
     	  }
     	  else
     	  {
     	 #endif
     	   
     	   #ifdef LM_SUPPORT_UT
     	  	if (port<=4)
     	   #else 
     	  	if (port<4)
     	   #endif
     	  	{
 	  	      //2-3-1.2-1.转发(485端口)
    	      if (copyCtrl[port].pForwardData!=NULL)
    	      {
              forwardData(port);
              goto breakPoint;
    	      }
          	   
 	  	      //2-3-1.2-2.比较,指定的抄表时间是否已到
 	  	      if (compareTwoTime(copyCtrl[port].nextCopyTime, sysTime))
 	  	      {
              if (copyCtrl[port].cmdPause==FALSE)
              {
                //初始化本端口电表链表
               #ifdef LM_SUPPORT_UT
                if (4==port)
                {
             	    copyCtrl[port].cpLinkHead = initPortMeterLink(30);
                }
                else
                {
               #endif
            
                  copyCtrl[port].cpLinkHead = initPortMeterLink(port);
            
                  //抄表初始时判断上月数据是否完整
                  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                  while(copyCtrl[port].tmpCpLink!=NULL)
                  {
                   #ifdef LIGHTING
	             		  if (LIGHTING_XL==copyCtrl[port].tmpCpLink->protocol)
	             		  {
	             		    tmpNode = xlcLink;
	             		    while(tmpNode!=NULL)
	             	      {
	             	  	    if (compareTwoAddr(tmpNode->addr, copyCtrl[port].tmpCpLink->addr, 0)==TRUE)
	             	        {
	                        //2015-06-12,添加光控处理
	                        //2016-03-17,因在时段控结合光控时出现分合分的情况,从meter485Receive函数中移到这里处理,
	                        //if (
	                        //	   CTRL_MODE_LIGHT==ctrlMode             //光控
	                        //	    || CTRL_MODE_PERIOD_LIGHT==ctrlMode  //时控+光控
	                        //	  )
							            //2017-01-06,从上面的判断改成下面这个判断,因为在张家港测试发现光控在经纬度结合光控时不起作用
												  if (
													    CTRL_MODE_LIGHT==tmpNode->bigAndLittleType
														   || CTRL_MODE_PERIOD_LIGHT==tmpNode->bigAndLittleType
														    || CTRL_MODE_LA_LO_LIGHT==tmpNode->bigAndLittleType
													   )
                          {
                            //光控要求电源分闸,而线路控制器为合闸
                            if (0==tmpNode->lcOnOff && 1==tmpNode->status)
                            {
                       	      if (copyCtrl[port].tmpCpLink!=NULL)
         					            {
         					              copyCtrl[port].tmpCpLink->flgOfAutoCopy |= REQUEST_OPEN_GATE;
         					        
         					              if (debugInfo&METER_DEBUG)
        	  	                  {
   	                              printf("%02d-%02d-%02d %02d:%02d:%02d,抄表中,光控要求线路控制点%d分闸\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,copyCtrl[port].tmpCpLink->mp);
        	  	                  }
                 				      }
                            }
                               
                            //光控为合闸,而线路控制器为分闸(/或未知)
                            if (1==tmpNode->lcOnOff && 1!=tmpNode->status)
                            {
                       	      if (copyCtrl[port].tmpCpLink!=NULL)
         					            {
         					              copyCtrl[port].tmpCpLink->flgOfAutoCopy |= REQUEST_CLOSE_GATE;

         					              if (debugInfo&METER_DEBUG)
        	  	                  {
   	                              printf("%02d-%02d-%02d %02d:%02d:%02d,抄表中,光控要求线路控制点%d合闸\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,copyCtrl[port].tmpCpLink->mp);
        	  	                  }
                 				      }
                            }
                          }
                             
                   	    	break;
                   	    }
                   	  	
                   	  	tmpNode = tmpNode->next;
                   	  }
                   	}
                       
                   #else

						        //2015-02-02,add,交采且是端口一的才查询上月电量
                    if ((copyCtrl[port].tmpCpLink->protocol==AC_SAMPLE && copyCtrl[port].tmpCpLink->port==1)
               		      || copyCtrl[port].tmpCpLink->protocol==DLT_645_1997 
               		       || copyCtrl[port].tmpCpLink->protocol==DLT_645_2007
               		        || copyCtrl[port].tmpCpLink->protocol==EDMI_METER)
                    {
                      if ((copyCtrl[port].tmpCpLink->bigAndLittleType&0xf)==0x1)    //小类号为1的是单相表
                      {
                        if (debugInfo&METER_DEBUG)
                        {
                   	      printf("测量点%d配置为单相表,不抄上月数据\n", copyCtrl[port].tmpCpLink->mp);
                        }
                        break;
                      }
                         
             	        if (checkLastMonthData(copyCtrl[port].tmpCpLink->mp)==FALSE)
             	        {
             	          copyCtrl[port].ifCopyLastFreeze=1;
             	          break;
             	        }
             	      }
						 
                 	 #endif
                 	    
                 	  copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                  }

               #ifdef LM_SUPPORT_UT
                }
               #endif
                   
                //电科院送检时不要采集上一日冻结数据,因为模拟表的冻结数据都是0,日、月冻结数据过不了
                //ly,2011-08-25
               #ifndef DKY_SUBMISSION
                copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                if (copyCtrl[port].ifCopyLastFreeze!=1)
                {
                  copyCtrl[port].ifCopyLastFreeze = 0;
                  while(copyCtrl[port].tmpCpLink!=NULL)
                  {
           	        //0:10分后查找是否有日冻结数据,如果没有日冻结数据,07表需要抄读电能表上一日冻结数据
           	        if ((sysTime.hour==0 && sysTime.minute>10) || sysTime.hour>0)
           	        {
    	                if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
  	     	 	          {
    	                  //检测是否有当日 日冻结数据
      	                if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
      	                {
       	                  copyCtrl[port].ifCopyLastFreeze = 2;
      	                }
      	              }
           	        }
           	     
           	        copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                  }
                }
               #endif
                   
                if (copyCtrl[port].cpLinkHead==NULL)   //本端口无表可抄
                {
                  //上次抄表时间(BCD数)
                  copyCtrl[port].lastCopyTime.second = copyCtrl[port].currentCopyTime.second/10<<4 | copyCtrl[port].currentCopyTime.second%10;;
                  copyCtrl[port].lastCopyTime.minute = copyCtrl[port].currentCopyTime.minute/10<<4 | copyCtrl[port].currentCopyTime.minute%10;;
                  copyCtrl[port].lastCopyTime.hour   = copyCtrl[port].currentCopyTime.hour/10<<4 | copyCtrl[port].currentCopyTime.hour%10;;
                  copyCtrl[port].lastCopyTime.day    = copyCtrl[port].currentCopyTime.day/10<<4 | copyCtrl[port].currentCopyTime.day%10;;
                  copyCtrl[port].lastCopyTime.month  = copyCtrl[port].currentCopyTime.month/10<<4 | copyCtrl[port].currentCopyTime.month%10;;
                  copyCtrl[port].lastCopyTime.year   = copyCtrl[port].currentCopyTime.year/10<<4 | copyCtrl[port].currentCopyTime.year%10;;

                  copyCtrl[port].nextCopyTime = nextTime(sysTime, teCopyRunPara.para[whichItem((4==port)?PORT_POWER_CARRIER:(port+1))].copyInterval ,0);
           	  
         	        if (debugInfo&METER_DEBUG)
         	        {
           	        printf("端口:%d无表可抄\n",port+1);
           	      }
                }
                else
                {
                  //检查三相电能表数据存储表是否存在
                  checkSpRealTable(1);
   	  	     	       
   	  	     	    //采集上一次日冻结数据时要移位到07规约表开始抄     	  	     	       
   	  	     	    if (copyCtrl[port].ifCopyLastFreeze==2)
   	  	     	    {
   	  	     	      if (debugInfo&METER_DEBUG)
   	  	     	      {
   	  	     	        printf("开始抄上一次日冻结数据前定位到07表\n");
   	  	     	      }
   	  	     	         
   	  	     	      while(copyCtrl[port].cpLinkHead!=NULL)
   	  	     	      {
   	  	     	 	      if (copyCtrl[port].cpLinkHead->protocol==DLT_645_2007)
   	  	     	 	      {
   	  	     	 	  	    break;
   	  	     	 	      }
   	  	     	 	         
 	  	                tmpNode = copyCtrl[port].cpLinkHead;
	  	                copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
	  	   	            free(tmpNode);
   	  	     	      }
  
   	  	     	      //如果没有07规约的表,则开始抄实时数据
   	  	     	      if (copyCtrl[port].cpLinkHead==NULL)
   	  	     	      {
   	  	     	        copyCtrl[port].ifCopyLastFreeze = 0;
                      copyCtrl[port].cpLinkHead = initPortMeterLink(port);
   	  	     	      }
   	  	     	    }
   	  	     	       
   	  	     	    //交采测量点的处理
      	  	   	  if (
						          copyCtrl[port].cpLinkHead->protocol==AC_SAMPLE
										 #ifdef LIGHTING
										  && 0==port
										 #endif
									   )
      	  	   	  {
        	          //有交采模块
        	         #ifdef AC_SAMPLE_DEVICE
        	          if (ifHasAcModule==TRUE)
        	          {
        	            copyCtrl[port].currentCopyTime = sysTime;
  
      	         	    //如果是抄上月数据,转换上月数据数据
      	         	    if (copyCtrl[port].ifCopyLastFreeze==0x1)
      	         	    {
           	         	  copyAcValue(port, 2, sysTime);
  
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("CopyMeter:转换交采上月数据并保存\n");
                        }
                      }
                          
      	         	    //转换实时数据
      	         	    if (copyCtrl[port].ifCopyLastFreeze==0x0)
      	         	    {
           	         	  copyAcValue(port, 1, timeHexToBcd(copyCtrl[port].currentCopyTime));
                                           
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("CopyMeter:转换交采实时数据并保存\n");
                        }
                      }
       	            }
       	           #endif
       	  	            
       	  	        tmpNode = copyCtrl[port].cpLinkHead;
      	  	        copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
      	  	   	    free(tmpNode);
  
             	  	  if (debugInfo&METER_DEBUG)
             	  	  {
             	  	    printf("CopyMeter:交采测量点,移位\n");
             	  	  }
                        
                    if (copyCtrl[port].cpLinkHead==NULL)
                    {
             	  	   	if (debugInfo&METER_DEBUG)
             	  	   	{
             	  	   	  printf("CopyMeter:端口%d只有一个交采测量点\n", port+1);
             	  	   	}
    
            	  	 	  //上次抄表时间(BCD数)
                      copyCtrl[port].lastCopyTime.second = copyCtrl[port].currentCopyTime.second/10<<4 | copyCtrl[port].currentCopyTime.second%10;;
                      copyCtrl[port].lastCopyTime.minute = copyCtrl[port].currentCopyTime.minute/10<<4 | copyCtrl[port].currentCopyTime.minute%10;;
                      copyCtrl[port].lastCopyTime.hour   = copyCtrl[port].currentCopyTime.hour/10<<4 | copyCtrl[port].currentCopyTime.hour%10;;
                      copyCtrl[port].lastCopyTime.day    = copyCtrl[port].currentCopyTime.day/10<<4 | copyCtrl[port].currentCopyTime.day%10;;
                      copyCtrl[port].lastCopyTime.month  = copyCtrl[port].currentCopyTime.month/10<<4 | copyCtrl[port].currentCopyTime.month%10;;
                      copyCtrl[port].lastCopyTime.year   = copyCtrl[port].currentCopyTime.year/10<<4 | copyCtrl[port].currentCopyTime.year%10;;
                   
                      copyCtrl[port].nextCopyTime = nextTime(sysTime, teCopyRunPara.para[whichItem(port+1)].copyInterval ,0);
  
           	 	        if (copyCtrl[port].ifCopyLastFreeze==0x0 && ifHasAcModule==TRUE)
           	 	        {
       	 	              if (debugInfo&METER_DEBUG)
       	 	              {
       	 	                printf("copyMeter:Start-本端口%d实时结算准备置位, ifRealBalance=%d,ret=%d\n", port+1, copyCtrl[port].ifRealBalance, ret);
       	 	              }
       	 	             
         	 	            //本端口实时结算置位
       	 	              if (debugInfo&METER_DEBUG)
       	 	              {
       	 	                printf("copyMeter:本端口%d实时结算置位\n",port+1);
       	 	              }
       	 	                 
       	 	              copyCtrl[port].ifRealBalance = 1;
       	 	            }
  
  	         	        if (copyCtrl[port].ifCopyLastFreeze==0x1)
  	         	        {
  	         	          copyCtrl[port].ifCopyLastFreeze=0x0;
  	         	        }
                    }
      	  	   	  }
                     
                  if (copyCtrl[port].cpLinkHead!=NULL)
                  {
                    initCopyItem(port, copyCtrl[port].ifCopyLastFreeze);
                       
                    //2012-07-31,改到这里处理脉冲值的存储,
                    //  原因:在台体测试时发现脉冲量的总加电能量老是不合格,经检查与这个存储时间先后有关系
           	 	     #ifdef PULSE_GATHER
           	 	      for(pulsei=0;pulsei<NUM_OF_SWITCH_PULSE;pulsei++)
           	 	      {
           	 	        if (pulse[pulsei].ifPlugIn == TRUE)
           	 	        {
                        memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
                        memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
                        memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);

                        //转换
                        covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
                       		 	
                       	//保存示值
                       	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                                       visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
                       	   
                        //保存参数
                       	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                                       paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
  
                       	//保存需量
                       	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                                       reqBuff, PRESENT_DATA, REQ_REQTIME_DATA,LENGTH_OF_REQ_RECORD);
           	 	        }
           	 	      }
           	 	     #endif
											
									 #ifdef LIGHTING
									  copyCtrl[port].thisRoundFailure = FALSE;
									 #endif

                    if (debugInfo&METER_DEBUG)
                    {
                      printf("端口%d开始抄表\n",port+1);
                    }
                  }
                }
              }
                 
      	 	    goto breakPoint;
      	 	  }
    	 	  }
    	 	  
    	 #ifdef PLUG_IN_CARRIER_MODULE
    	 	}
    	 #endif  //PLUG_IN_CARRIER_MODULE
      }
      else      //该端口正在抄表
      {
  	    //2-3-2-1.处理抄表异常
  	    //2-3-2-2.防止在抄表过程中下次抄表时间已到,而得不到正确的下次抄表时间
  	    if (port<5)
  	    {
  	      if (compareTwoTime(copyCtrl[port].nextCopyTime, sysTime))
  	      {
  	        copyCtrl[port].nextCopyTime = nextTime(copyCtrl[port].nextCopyTime, 0, 5);  //下次抄表时间向后推移5秒钟

  	        if (debugInfo&METER_DEBUG)
  	        {
  	          if (port!=4)
  	          {
  	            printf("端口%d下次抄表时间向后推移5秒钟\n",port+1);
  	          }
  	        }
  	      }
  	    }
   	  	   
   	  #ifdef PLUG_IN_CARRIER_MODULE
  	  	  
  	   #ifdef LM_SUPPORT_UT
  	  	if (4==port && 0x55!=lmProtocol)
  	   #else
  	  	if (port==4)    //抄表中对路由发送暂停、重启、恢复等
  	   #endif
  	  	{
	  	 	  //2012-4-1,如果在升级路由不能操作载波模块
	  	 	  if (upRtFlag!=0)
	  	 	  {
	  	 	 	  goto breakPoint;
	  	 	  }
  	  	   	  
  	   	  //如果需要发送暂停/恢复抄表命令
          //2012-01-16,路由主导抄表时的暂停/恢复抄表命令的发送处理
  	      if (carrierFlagSet.routeLeadCopy==1)
  	      {
  	        if (carrierFlagSet.reStartPause>0)
  	        {
  	      	  if (carrierFlagSet.retryCmd==1 || carrierFlagSet.cmdContinue==1)
  	      	  {
                switch (carrierFlagSet.reStartPause)
                {
               	  case 1:    //重启抄表
                    if (carrierFlagSet.workStatus==2)
                    {
	  	     	          gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);
    	  	            carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
    	  	            
    	  	            if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	            {
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发送重启抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	            }
                    }
                    if (carrierFlagSet.workStatus==3)
                    {
                   	  carrierFlagSet.reStartPause = 0;

    	  	            if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	            {
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,收到确认重启抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	            }
                    }
                    break;
               
                  case 2:    //暂停抄表
                    if (
                   	    carrierFlagSet.workStatus==4
                   	     || carrierFlagSet.workStatus==7        //检测到出抄表时段而暂停
                   	      || carrierFlagSet.workStatus==8       //搜索末端载波通信点而暂停
                   	       || carrierFlagSet.workStatus==9      //超离线阈值而暂停
                   	        || carrierFlagSet.workStatus==10    //查询单灯控制器上电状态而暂停
                   	   )
                    {
	     	         			gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
	  	             		carrierFlagSet.cmdType = CA_CMD_PAUSE_WORK;
	  	            
	  	             		if (debugInfo&PRINT_CARRIER_DEBUG)
	  	             		{
                     		printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发送暂停抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
	  	             		}
                   	}
                   	if (carrierFlagSet.workStatus==5)
                   	{
                   	 	carrierFlagSet.reStartPause = 0;

    	  	         		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         		{
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,收到确认暂停抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	         		}
                   	}
                   	break;
               
               
                 	case 3:    //恢复抄表
                   	if (carrierFlagSet.workStatus==3)
                   	{
                   	 	carrierFlagSet.reStartPause = 0;

    	  	         		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         		{
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,收到确认恢复抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	         		}
    	  	           
    	  	         		//2013-12-05,add
    	  	        	 #ifdef LIGHTING 
    	  	        	 	if (5==carrierFlagSet.broadCast)
 	  	             		{
 	  	               		carrierFlagSet.broadCast = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           		{
	                        printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,广播命令等待结束\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}

    	  	         		if (1==carrierFlagSet.searchLddt)
 	  	             		{
 	  	               		carrierFlagSet.searchLddt = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           		{
	                       	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,搜索LDDT结束\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}
 	  	               
    	  	         		if (1==carrierFlagSet.searchOffLine)
 	  	             		{
 	  	               		carrierFlagSet.searchOffLine = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           		{
	                        printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,搜索离线单灯控制器结束\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}

    	  	         		if (1==carrierFlagSet.searchLddStatus)
 	  	             		{
 	  	               		carrierFlagSet.searchLddStatus = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	          		{
	                        printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,查询刚上电单灯控制器状态结束\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}
 	  	               #endif
                   	}

                   	if (carrierFlagSet.workStatus==6 && compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
                   	{
                     	gdw3762Framing(ROUTE_CTRL_3762, 3, NULL, NULL);
                     	carrierFlagSet.cmdType = CA_CMD_RESTORE_WORK;
                     
    	  	         		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         		{
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发送恢复抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	         		}
                   	}
                   	else
                   	{
                     	if (copyCtrl[port].pForwardData!=NULL || pDotCopy!=NULL || pDotFn129!=NULL)
                     	{
                      	 goto continueCopying;
                     	}
                   	}
                   	break;                      
               		}
             		}
             		else       //判断发送超时
             		{
               		if (carrierFlagSet.sending==1)
                  {
             	 			if (compareTwoTime(carrierFlagSet.cmdTimeOut, sysTime))
             	 			{
             	  			carrierFlagSet.retryCmd = 1;    //命令超时,请求重发
             	 			}
               		}
             		}
             
             		goto breakPoint;
  	       		}
               
              //2011-07-12,路由主导抄表时的主站加表及各种搜表的处理
    	  	   	//主站设置表地址后等待一分钟同步到模块
              if (carrierFlagSet.msSetAddr==1)
              {
             	 	if (compareTwoTime(carrierFlagSet.msSetWait, sysTime))
             	 	{
   	              carrierFlagSet.workStatus = 4;
   	     	       	carrierFlagSet.reStartPause = 2;
                  carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
 
                  copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
   	               
   	              carrierFlagSet.msSetAddr = 0;
   	              carrierFlagSet.synSlaveNode = 1;
   	               
   	              if (debugInfo&PRINT_CARRIER_DEBUG)
   	              {
   	                printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,主站设置表地址等待时间到,开始同步表地址到模块\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
   	              }
   	               
   	              goto breakPoint;
   	            }
   	          }
   	           
   	          //检测到搜表命令
   	          if (carrierFlagSet.searchMeter==1 && carrierFlagSet.workStatus==3)
   	          {
   	            carrierFlagSet.workStatus = 4;
   	     	     	carrierFlagSet.reStartPause = 2;
                carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
 
                copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
                      
   	            if (debugInfo&PRINT_CARRIER_DEBUG)
   	            {
   	              printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到搜表命令,暂停抄表执行搜表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
   	            }
   	             
   	            goto breakPoint;
   	          }
   	           
   	          //路由主导抄表的话,等待(ROUTE_REQUEST_OUT)分钟后无路由请求,重启抄表
              if (compareTwoTime(carrierFlagSet.routeRequestOutTime,sysTime) && carrierFlagSet.workStatus==3)
              {
 	             	//2014-01-06,修改等待路由请求超时后的处理,屏蔽后面的几句处理
 	             	//carrierFlagSet.workStatus = 2;
 	             	//carrierFlagSet.reStartPause = 1;
 	             
 	             	//if (debugInfo&PRINT_CARRIER_DEBUG)
 	             	//{
   	            //  printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到路由请求超时,发送重启抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
 	             	//}

  	  	      	if (carrierFlagSet.retryCmd==1 || carrierFlagSet.cmdContinue==1)
  	  	      	{
  	  	      	  if (carrierFlagSet.numOfCarrierSending<3)
  	  	      	  {
 	                  if (debugInfo&PRINT_CARRIER_DEBUG)
 	                  {
   	                  printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到路由请求超时,发送查询路由状态命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
 	                  }

            	 	 		gdw3762Framing(ROUTE_QUERY_3762, 4, NULL, NULL);
            	 	 		carrierFlagSet.cmdType = CA_CMD_QUERY_ROUTE_STATUS;
  	  	      	  }
  	  	      	  else
  	  	      	  {
 	                 	if (debugInfo&PRINT_CARRIER_DEBUG)
 	                 	{
   	                  printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发送查询路由状态命令3次未回复,复位路由,重新开始\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
 	                 	}

  	  	      	 	 	close(fdOfCarrier);
                    carrierUartInit();
                    sleep(5);
                     
                    resetCarrierFlag();
                    carrierFlagSet.routeLeadCopy = 1;
                     
                    copyCtrl[4].meterCopying = FALSE;
                    if (TC_CARRIER==carrierModuleType)
                    {
                      printf("鼎信这里\n");
                     	 
                      copyCtrl[5].meterCopying = FALSE;
                      copyCtrl[6].meterCopying = FALSE;
                    }
  	  	      	  }
  	  	      	}
                else       //判断发送超时
                {
                  if (carrierFlagSet.sending==1)
                  {
                 	  if (compareTwoTime(carrierFlagSet.cmdTimeOut, sysTime))
                 	 	{
                 	   	carrierFlagSet.retryCmd = 1;    //命令超时,请求重发
                 	 	}
                  }
                }
 	             
 	             	goto breakPoint;
              }
               
             #ifdef LIGHTING
              //广播命令中,判断等待结束
              if (5==carrierFlagSet.broadCast)
   	  	      {
   	  	     	 	if (compareTwoTime(carrierFlagSet.broadCastWaitTime, sysTime)==TRUE)
   	  	     	 	{
                  carrierFlagSet.workStatus = 6;
                  carrierFlagSet.reStartPause = 3;
                  copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                  carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
     	             	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,广播命令等待完成后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                  }
   	  	     	 	}
   	  	     	 	 
   	  	     	 	goto breakPoint;
   	  	      }
   	  	     #endif
  	  	    }

continueCopying:
  	  	   	//每分钟的第9秒查看是否在允许的抄表时段内
            if (sysTime.second>=9 && sysTime.second<11)
            {
              if (queryIfInCopyPeriod(4)==0)
              {
               	if (carrierFlagSet.workStatus==3 || carrierFlagSet.workStatus==7)
                {
  	  	          if (carrierFlagSet.routeLeadCopy==1)
  	  	          {
     	             	carrierFlagSet.workStatus = 7;
   	     	         	carrierFlagSet.reStartPause = 2;
                    carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
 
                    copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
                   
                    carrierFlagSet.cmdContinue = 1;    //2014-01-03,add
     	             
     	             	if (debugInfo&PRINT_CARRIER_DEBUG)
     	             	{
     	               	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到出抄表时段,发送暂停抄表命令\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	             	}
     	             
     	             	goto breakPoint;
  	  	          }
                }
               
                //2013-1-8,add到这个位置 
                if (5==carrierFlagSet.workStatus || 6==carrierFlagSet.workStatus)
                {
     	           	if (debugInfo&PRINT_CARRIER_DEBUG)
     	           	{
     	             	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到出抄表时段,但处于暂停状态,置为未抄表态\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	           	}
                 
                  copyCtrl[4].meterCopying = FALSE;
                      	
                  if (carrierModuleType==TC_CARRIER)
                  {
                    copyCtrl[5].meterCopying = FALSE;
                    copyCtrl[6].meterCopying = FALSE;
                  }
                       	
                  copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);
                   
                  carrierFlagSet.workStatus = 5;
                   
                  goto breakPoint;
                }
              }
            }
             
           #ifdef LIGHTING
            //线缆盗割报警 - 查看在末端的单灯控制器心跳是否正常
            if (0==carrierFlagSet.searchLddt)
            {
              if (compareTwoTime(carrierFlagSet.searchLddtTime, sysTime)==TRUE)
              {
               	carrierFlagSet.searchLddtTime = nextTime(sysTime, 2, 0);    //固定每2分钟查询一次
               	 
                ifFound = 0;
               	carrierFlagSet.searchLdgmNode = ldgmLink;
               	while (carrierFlagSet.searchLdgmNode!=NULL)
               	{
           	 	    if (
           	 	 	   		((carrierFlagSet.searchLdgmNode->status&0x1)==1)    //线路是交流供电
           	 	 	   		 && 
           	 	 	    		//2分钟内获取的报警控制点状态
           	 	 	    		(
           	 	 	     			(timeCompare(carrierFlagSet.searchLdgmNode->statusTime, sysTime, 2) == TRUE)
           	 	 	      		|| (timeCompare(sysTime, carrierFlagSet.searchLdgmNode->statusTime,  2) == TRUE)
           	 	 	    		)
           	 	 	  	 )
           	 	   	{
         	 	     		//第一末端单灯控制器地址不为全0
         	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->lddt1st, NULL, 1))
         	 	     		{
         	 	 	   			tmpNode = copyCtrl[4].cpLinkHead;
 	  		 	 	   				while(tmpNode!=NULL)
 	  		 	 	   				{
 	  		 	 	   	 				if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->lddt1st, 0)==TRUE)
 	  		 	 	   	 				{
				  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
			 	  		 	 	   	   	{
			  		 	 	   	 		 		printf("%02d-%02d-%02d %02d:%02d:%02d:线缆盗割报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的第一末端单灯控制器(%02x-%02x-%02x-%02x-%02x-%02x)的最近通信时间为%02d-%02d-%02d %02d:%02d:%02d",
			  		 	 	   	 		      	 	 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
			  		 	 	   	 		       			carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
			  		 	 	   	 		       			tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
			  		 	 	   	 		       			 tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
			  		 	 	   	 		       		);
			 	  		 	 	   	   	}
			 	  		 	 	   	 	 
			 	  		 	 	   	   	//tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
			 	  		 	 	   	   	tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
 	  		 	 	   	   				if (compareTwoTime(tmpTime, sysTime))    //心跳超过监测时限
 	  		 	 	   	   				{
			  		 	 	   	 	 	 		if (
			  		 	 	   	 	 	 	   		tmpNode->lddtRetry>=99                     //搜索LDDT重试次数=99,即自动抄读OK
			  		 	 	   	 	 	      	|| tmpNode->lddtRetry<pnGate.lddtRetry    //或搜索LDDT重试次数小于最大重试次数
			  		 	 	   	 	 	    	 )
			  		 	 	   	 	     	{
			 	  		 	 	   	 	   		if (tmpNode->lddtRetry>=99)
			 	  		 	 	   	 	   		{
			 	  		 	 	   	 	     		tmpNode->lddtRetry = 0;
			 	  		 	 	   	 	   		}

				 	  		 	 	   	 	   	ifFound = 1;
				 	  		 	 	   	 	   	copyCtrl[4].tmpCpLink = tmpNode;
				 	  		 	 	   	 	   	
				 	  		 	 	   	 	   	if (debugInfo&PRINT_CARRIER_DEBUG)
				 	  		 	 	   	 	   	{
				 	  		 	 	   	 	    	 printf(",已超过报警监测时限");
				 	  		 	 	   	 	   	}
				 	  		 	 	   	 	 	}
				  		 	 	   	 		 	else
				  		 	 	   	 		 	{
				  		 	 	   	 	    	if (debugInfo&PRINT_CARRIER_DEBUG)
				  		 	 	   	 	    	{
				  		 	 	   	 	        printf(",紧急搜索已超次");
				  		 	 	   	 	     	}
				  		 	 	   	 		 	}
 	  		 	 	   	   				}
				 	  		 	 	   	  if (debugInfo&PRINT_CARRIER_DEBUG)
				 	  		 	 	   	  {
				 	  		 	 	   	 	 	printf("\n");
				 	  		 	 	   	  }
				 	  		 	 	   	  break;
 	  		 	 	   	 				}
 	  		 	 	   	 
 	  		 	 	   	 				tmpNode = tmpNode->next;
 	  		 	 	   				}
         	 	     		}
         	 	 
         	 	     		if (1==ifFound)
         	 	     		{
         	 	 	   			break;
         	 	     		}
                 	 	 
         	 	     		//第二末端单灯控制器地址不为全0
         	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->collectorAddr, NULL, 1))
         	 	     		{
         	 	 	   			tmpNode = copyCtrl[4].cpLinkHead;
			 	  		 	 	   	while(tmpNode!=NULL)
			 	  		 	 	   	{
			 	  		 	 	   	 	if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->collectorAddr, 0)==TRUE)
			 	  		 	 	   	 	{
			 	  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
			 	  		 	 	   	   	{
			  		 	 	   	 		 		printf("%02d-%02d-%02d %02d:%02d:%02d:线缆盗割报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的第二末端单灯控制器(%02x-%02x-%02x-%02x-%02x-%02x)的最近通信时间为%02d-%02d-%02d %02d:%02d:%02d",
			  		 	 	   	 		        		sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
			  		 	 	   	 		         		 carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
			  		 	 	   	 		          		tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
			  		 	 	   	 		           		 tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
			  		 	 	   	 		       		);
			 	  		 	 	   	   	}
			 	  		 	 	   	 	 
			  		 	 	   	 	    //tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
			  		 	 	   	 	   	tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
			  		 	 	   	 	   	if (compareTwoTime(tmpTime, sysTime))    //心跳超过监测时限
			  		 	 	   	 	   	{
			  		 	 	   	 	     	if (
			  		 	 	   	 	 	 	 			tmpNode->lddtRetry>=99                     //搜索LDDT重试次数=99,即自动抄读OK
			  		 	 	   	 	 	     		 || tmpNode->lddtRetry<pnGate.lddtRetry    //或搜索LDDT重试次数小于最大重试次数
			  		 	 	   	 	 	    	 )
			 	  		 	 	   	 	 		{
				  		 	 	   	 	   		if (tmpNode->lddtRetry>=99)
				  		 	 	   	 	   		{
				  		 	 	   	 	     		tmpNode->lddtRetry = 0;
				  		 	 	   	 	   		}

				  		 	 	   	 	   		ifFound = 1;         	  		 	 	   	 	   
				  		 	 	   	 	   		copyCtrl[4].tmpCpLink = tmpNode;
				  		 	 	   	 	   
				  		 	 	   	 	   		if (debugInfo&PRINT_CARRIER_DEBUG)
			  		 	 	   	 	      	{
			  		 	 	   	 	         	printf(",已超过报警监测时限");
			  		 	 	   	 		   		}
			  		 	 	   	 		 		}
			  		 	 	   	 		 		else
			  		 	 	   	 		 		{
			  		 	 	   	 	       	if (debugInfo&PRINT_CARRIER_DEBUG)
			  		 	 	   	 	       	{
			  		 	 	   	 	        	 printf(",紧急搜索已超次");
			  		 	 	   	 		   		}
			  		 	 	   	 	     	}
			     	  		 	 	   	}
				  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
				  		 	 	   	   	{
				  		 	 	   	 	 		printf("\n");
				  		 	 	   	   	}
				  		 	 	   	   	break;
			 	  		 	 	   	 	}
			 	  		 	 	   	 
			 	  		 	 	   	 	tmpNode = tmpNode->next;
			 	  		 	 	   	}
                 	 	}
                 	 	 
             	 	 		if (1==ifFound)
             	 	 		{
             	 	 			break;
             	 	 		}
             	   	}
               	 	 
               	  carrierFlagSet.searchLdgmNode = carrierFlagSet.searchLdgmNode->next;
               	}
               	 
               	if (1==ifFound)
               	{
  	  	          if (carrierFlagSet.routeLeadCopy==1)
  	  	          {
     	             	carrierFlagSet.workStatus = 8;
   	     	         	carrierFlagSet.reStartPause = 2;
                     
                    carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
                     
                    copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
                   
                    carrierFlagSet.cmdContinue = 1;
     	             
  		 	 	     			if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	     			{
  		 	 	       			printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到超时限末端单灯控制器,紧急召测监测末端单灯控制器\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
  		 	 	     			}
     	             
     	             	goto breakPoint;
  	  	          }
               	}
              }
            }
             
            //查看是否有离线的单灯控制器,2015-11-09
            if (
             	  0==carrierFlagSet.searchOffLine
             	   && 0==carrierFlagSet.searchLddt    //在没有搜索LDDT时才判断,搜LDDT更紧急
             	 )
            {
              if (compareTwoTime(carrierFlagSet.searchOffLineTime, sysTime)==TRUE)
              {
                carrierFlagSet.searchOffLineTime = nextTime(sysTime, 3, 0);    //固定每3分钟查询一次
               	 
               	ifFound = 0;
               	tmpNode = copyCtrl[port].cpLinkHead;
               	while(tmpNode!=NULL)
               	{
           	 	    if (
           	 	 	      //2016-11-28,单灯离线阈值的单位从“分”改为”小时“
					    				compareTwoTime(nextTime(tmpNode->statusTime, pnGate.lddOffGate*60, 0), sysTime)==TRUE
           	 	 	     	 && (tmpNode->offLineRetry<pnGate.offLineRetry || tmpNode->offLineRetry>=99)
                   	    && (tmpNode->lineOn>=1 && tmpNode->lineOn<=5)    //单灯控制器处于上电状态
                   	 	   && 1==tmpNode->joinUp
           	 	 	     )
               	  {
	           	 	 	 	if (tmpNode->offLineRetry>=99)
	           	 	 	 	{
	           	 	 	  	tmpNode->offLineRetry = 0;
	           	 	 	 	}

	           	 	 	 	ifFound = 1;
	           	 	 	 
	           	 	 	 	copyCtrl[4].tmpCpLink = tmpNode;
	           	 	 	 	break;
	           	 	  }
               	 	 
               	  tmpNode = tmpNode->next;
               	}
               	 
               	if (1==ifFound)
               	{
  	  	          if (carrierFlagSet.routeLeadCopy==1)
  	  	          {
     	             	carrierFlagSet.workStatus = 9;
   	     	         	carrierFlagSet.reStartPause = 2;
                   	  
                    carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
                     
                    copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
                   
                    carrierFlagSet.cmdContinue = 1;
     	             
  		 	 	     			if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	     			{
  		 	 	       			printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到超离线阈值单灯控制器,紧急召测它们\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
  		 	 	     			}
     	             
     	             	goto breakPoint;
  	  	          }
               	}
               	else
               	{
  		 	 	   			if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	   			{
  		 	 	     			printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,无超离线阈值的单灯控制器\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
  		 	 	   			}
               	}
              }
            }

            //查看是否有需要召测上电状态的单灯控制器,2015-11-12
            if (
             	  0==carrierFlagSet.searchLddt              //在没有搜索LDDT时才判断,搜LDDT更紧急,优先级最高
             	   && 0==carrierFlagSet.searchOffLine       //在没有搜索离线单灯控制器时才判断,优先级第二
             	    && 0==carrierFlagSet.searchLddStatus    //优先级最低
             	 )
            {
              if (compareTwoTime(carrierFlagSet.searchLddStatusTime, sysTime)==TRUE)
              {
               	carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 1, 0);    //固定每1分钟查询一次
               	 
               	ifFound = 0;
               	tmpNode = copyCtrl[port].cpLinkHead;
               	while(tmpNode!=NULL)
               	{
           	 	   	if (1==tmpNode->lineOn)    //载波线路刚上电
           	 	   	{
           	 	 	 		ifFound = 1;
           	 	 	 
           	 	 	 		copyCtrl[4].tmpCpLink = tmpNode;
           	 	 	 		break;
           	 	   	}
           	 	 
           	 	   	tmpNode = tmpNode->next;
               	}
               	 
               	if (1==ifFound)
               	{
  	  	          if (carrierFlagSet.routeLeadCopy==1)
  	  	          {
     	             	carrierFlagSet.workStatus = 10;
   	     	         	carrierFlagSet.reStartPause = 2;
                     
                    carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
                     
                    copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
                   
                    carrierFlagSet.cmdContinue = 1;
     	             
		  		 	 	     	if (debugInfo&PRINT_CARRIER_DEBUG)
		  		 	 	     	{
		  		 	 	       	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,检测到单灯控制器线路刚上电,紧急召测它们\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
		  		 	 	     	}
     	             
     	             	goto breakPoint;
  	  	          }
               	}
               	else
               	{
			  		 	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
			  		 	 	  {
			  		 	 	    printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,无需查询线路刚上电的单灯控制器\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			  		 	 	  }
               	}
              }
            }
             
           #endif
 	        }
 	         
 	       #endif    //#ifdef PLUG_IN_CARRIER_MODULE
     	  	 
   	  	  //2-3-2-3.抄表继续标志置位或重试标志置位,继续向下抄表
   	  	  if (copyCtrl[port].copyContinue == TRUE || copyCtrl[port].flagOfRetry == TRUE)
   	      {
   	        //2-4-2-3-0.如果收到命令暂停抄表
   	        if (copyCtrl[port].cmdPause==TRUE)
   	        {
   	          //路由主导抄表时暂停抄表
            #ifdef PLUG_IN_CARRIER_MODULE
   	         #ifdef LM_SUPPORT_UT
   	          if (4==port && 0x55!=lmProtocol)
   	         #else
   	         	if (port==4)
   	         #endif
   	         	{
         	      if (carrierFlagSet.routeLeadCopy==0)
         	      {
         	    		copyCtrl[port].meterCopying = FALSE;
         	      
         	        if (debugInfo&METER_DEBUG)
         	        {
         	          printf("端口%d暂停抄表\n",port);
         	        }
         	      }
         	    }
   	        #endif    //#ifdef PLUG_IN_CARRIER_MODULE
   	         	 
   	         #ifdef LM_SUPPORT_UT
   	         	if (port<=4 && 0x55==lmProtocol)
   	         #else
   	         	if (port<4)
   	         #endif
   	         	{
   	         	  copyCtrl[port].meterCopying = FALSE;
   	         	   
   	         	  if (debugInfo&METER_DEBUG)
   	         	  {
   	         	    printf("端口%d暂停抄表\n",port);
   	         	  }
   	         	}
   	         	 
   	         	goto breakPoint;
   	        }
   	         
   	        //2-3-2-3-1.转发或点抄处理
     	    #ifdef PLUG_IN_CARRIER_MODULE
     	  	 #ifdef LM_SUPPORT_UT
     	  	  if (4==port && 0x55!=lmProtocol)
     	  	 #else
     	  	  if (port==4)
     	  	 #endif
     	  	  {
     	  	  #ifdef LIGHTING
     	  	     
     	  	   	if (1==carrierFlagSet.routeLeadCopy)
     	  	   	{
     	  	     	if (
     	  	     	 	 	0<carrierFlagSet.broadCast
     	  	     	     || 1==carrierFlagSet.searchLddt
     	  	     	      || 1==carrierFlagSet.searchOffLine
     	  	     	       || 1==carrierFlagSet.searchLddStatus
     	  	     	   )
     	  	     	{
     	  	       	memset(copyCtrl[port].needCopyMeter, 0x00, 6);

     	  	       	if (carrierFlagSet.workStatus<4)
     	  	       	{
   	  	     	     	carrierFlagSet.workStatus = 4;
   	  	     	     	carrierFlagSet.reStartPause = 2;
                   	carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);

                   	copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
                   	copyCtrl[port].flagOfRetry = FALSE;
       	  	     	 	goto breakPoint;
       	  	     	}
       	  	     	 
       	  	     	if (carrierFlagSet.workStatus<5)
       	  	     	{
       	  	     	 	goto breakPoint;
       	  	     	}
       	  	     	 
       	  	     	//广播命令
       	  	     	if (0<carrierFlagSet.broadCast)
       	  	     	{
         	  	     	//发送广播校时
         	  	     	if (1==carrierFlagSet.broadCast)
         	  	     	{
                     	memset(copyCtrl[port].destAddr, 0x99, 6);
                     	memset(carrierFlagSet.destAddr, 0x99, 6);
                     	copyCtrl[port].ifCollector = 0;      //无采集器地址
    
     		 	 	   			 	carrierFramex[2] = slcFrame(carrierFlagSet.destAddr, &carrierFramex[3], 1, copyCtrl[4].tmpCpLink);    //报文+报文长度
                     	carrierFramex[1] = 0x02;    //协议
                     	carrierFramex[0] = carrierFramex[2]+2;    //长度
    
                     	gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
                       
                     	carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
     		 	 	   			 	carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
     	  	     	 		//发送广播主站控制灯命令
     	  	     	 		if (2==carrierFlagSet.broadCast)
     	  	     	 		{
                      memset(copyCtrl[port].destAddr, 0x99, 6);
                      memset(carrierFlagSet.destAddr, 0x99, 6);
                      copyCtrl[port].ifCollector = 0;      //无采集器地址
    
	 	 	       	   			memcpy(&carrierFramex[3], copyCtrl[4].pForwardData->data, copyCtrl[4].pForwardData->length);  //报文
	 	 	       	   			carrierFramex[2] = copyCtrl[4].pForwardData->length;  //报文长度
                      carrierFramex[1] = 0x02;    //协议
                      carrierFramex[0] = carrierFramex[2]+2;    //长度
    
                      gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
                       
                      carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
     		 	 	   				carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
         	  	     	//发送广播时段
         	  	     	if (3==carrierFlagSet.broadCast)
         	  	     	{
                      memset(copyCtrl[port].destAddr, 0x99, 6);
                      memset(carrierFlagSet.destAddr, 0x99, 6);
                      copyCtrl[port].ifCollector = 0;      //无采集器地址
                       
     		 	 	   				carrierFramex[2] = slcFrame(carrierFlagSet.destAddr, &carrierFramex[3], 2, copyCtrl[4].tmpCpLink);    //报文+报文长度
                      carrierFramex[1] = 0x02;                  //协议
                      carrierFramex[0] = carrierFramex[2]+2;    //长度
     		 	 	       	  
     		 	 	   				gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
     		 	 	       	   
     		 	 	   				carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
     		 	 	   				carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
         	  	     	//发送广播光控开关调光灯命令,2015-06-16,add
         	  	     	if (4==carrierFlagSet.broadCast)
         	  	     	{
                      memset(copyCtrl[port].destAddr, 0x99, 6);
                      memset(carrierFlagSet.destAddr, 0x99, 6);
                      copyCtrl[port].ifCollector = 0;      //无采集器地址
                       
     		 	 	   				carrierFramex[2] = slcFrame(carrierFlagSet.destAddr, &carrierFramex[3], 12, copyCtrl[4].tmpCpLink);    //报文+报文长度
                      carrierFramex[1] = 0x02;    //协议
                      carrierFramex[0] = carrierFramex[2]+2;    //长度
     		 	 	       	   
	 	 	       	   			gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
	 	 	       	   
	 	 	       	   			carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
	 	 	       	   			carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
                    copyCtrl[port].flagOfRetry  = FALSE;
     		 	 	 				copyCtrl[port].copyContinue = FALSE;
                     
                    copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 5);
    
         	  	     	goto breakPoint;
         	  	   	}
         	  	     
         	  	   	//紧急搜索LDDT
         	  	   	if (1==carrierFlagSet.searchLddt)
         	  	   	{
         	  	     	if (copyCtrl[port].tmpCpLink!=NULL && carrierFlagSet.searchLdgmNode!=NULL)
         	  	     	{
           	  	      copyCtrl[port].tmpCpLink->lddtRetry++;
           	  	      if (copyCtrl[port].tmpCpLink->lddtRetry>pnGate.lddtRetry)
           	  	      {
           	  	     	 	if (copyCtrl[port].tmpCpLink->lddtRetry<99)
           	  	     	 	{
           	  	     	   	//应该记录报警控制点异常了，因为末端单灯控制器不见了^^
                          searchMpStatis(sysTime, &meterStatisExtranTimeS, carrierFlagSet.searchLdgmNode->mp, 3); //报警控制器与时间无关量
                      	
                      	  //报警控制器判断为线路异常,记录发生事件
                 	  	   	printf("%02d-%02d-%02d %02d:%02d:%02d:线缆盗割报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的末端单灯控制器(%02x-%02x-%02x-%02x-%02x-%02x)不见了,判断为线路异常\n",
                 	  		  				sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
                 	  		 	   			 carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
                 	  		 	 					copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],copyCtrl[port].tmpCpLink->addr[3],copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]
                 	  		 	 			);

                  		   	//未记录的才记录
                  		   	if (0==(meterStatisExtranTimeS.mixed&0x04))
                  		   	{
                    		 		if (eventRecordConfig.iEvent[7]&0x10)    //ERC61,主站配置
                    	     	{
                    		   		//记录控制点故障信息(ERC13)
                    	       	eventData[0] = 13;
                    	       	eventData[1] = 11;
                    	
                    	       	tmpTime = timeHexToBcd(sysTime);
                    	       	eventData[2] = tmpTime.second;
                    	       	eventData[3] = tmpTime.minute;
                    	       	eventData[4] = tmpTime.hour;
                    	       	eventData[5] = tmpTime.day;
                    	       	eventData[6] = tmpTime.month;
                    	       	eventData[7] = tmpTime.year;
                    	  
                    	       	eventData[8] = carrierFlagSet.searchLdgmNode->mp&0xff;
                    	       	eventData[9] = (carrierFlagSet.searchLdgmNode->mp>>8&0xff) | 0x80;    //发生
                    	       	eventData[10] = 0x40;    //报警控制点异常,有电状态发生
                    	  
                    	       	writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                    	  
                    	       	if (eventRecordConfig.nEvent[1]&0x10)
                    	       	{
                    	         	writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                    	       	}
                    	  
                    	       	eventStatus[1] = eventStatus[1] | 0x10;
            					  
				    					       	if (debugInfo&PRINT_CARRIER_DEBUG)
				    					       	{
				    					        	printf("  --记录报警控制点%d(交流供电时)异常发生事件\n", carrierFlagSet.searchLdgmNode->mp);
				    					       	}
                         
                              activeReport3();    //主动上报事件
                    	   
                    	       	meterStatisExtranTimeS.mixed |= 0x04;
                          
                              //存储控制点统计数据(与时间无关量)
                              saveMeterData(carrierFlagSet.searchLdgmNode->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                            }
                          }
                          else
                          {
				    					     	if (debugInfo&PRINT_CARRIER_DEBUG)
				    					     	{
				    					     	  printf("  --报警控制点%d(交流供电时)异常发生事件已记录过了\n", carrierFlagSet.searchLdgmNode->mp);
				    					     	}
                          }
                           
                          //移位到下一个报警控制点,否则会造成这个一报警控制点的死锁
                          if (compareTwoAddr(copyCtrl[port].tmpCpLink->addr, carrierFlagSet.searchLdgmNode->collectorAddr, 0)==TRUE)
         	  		 	   			{
         	  		 	 	 				carrierFlagSet.searchLdgmNode = carrierFlagSet.searchLdgmNode->next;
         	  		 	   			}
   	  	     	 	     		}
           	  	     	 	 
           	  	     	 	//移位到下一个需要召测的末端单灯控制器
                       	ifFound = 0;
                       	while (carrierFlagSet.searchLdgmNode!=NULL)
                       	{
                   	 	   	if (
                   	 	 	   		((carrierFlagSet.searchLdgmNode->status&0x1)==1)    //线路是交流供电
                   	 	 	   		 && (    //2分钟获取的状态
                   	 	 	       			(timeCompare(carrierFlagSet.searchLdgmNode->statusTime, sysTime, 2) == TRUE)
                   	 	 	       			|| (timeCompare(sysTime, carrierFlagSet.searchLdgmNode->statusTime,  2) == TRUE)
                   	 	 	      		)
                   	 	 	  	 )
                       	 	{
                 	 	     		//第一末端单灯控制器地址不为全0
                 	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->lddt1st, NULL, 1))
                 	 	     		{
                 	 	 	   			tmpNode = copyCtrl[4].cpLinkHead;
	         	  		 	 	   			while(tmpNode!=NULL)
         	  		 	 	   				{
         	  		 	 	   	 				if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->lddt1st, 0)==TRUE)
         	  		 	 	   	 				{
     	  		 	 	   	 	   					if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	   	 	   					{
					     	  		 	 	   	 		 	printf("%02d-%02d-%02d %02d:%02d:%02d:线缆盗割报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的第一末端单灯控制器(%02x-%02x-%02x-%02x-%02x-%02x)的最近通信时间为%02d-%02d-%02d %02d:%02d:%02d",
					     	  		 	 	   	 		       		sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
					     	  		 	 	   	 		       			carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
					     	  		 	 	   	 		       				tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
					     	  		 	 	   	 		       					tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
					     	  		 	 	   	 		       	);
					     	  		 	 	   	 	  }
         	  		 	 	   	 	 
				     	  		 	 	   	 	    //tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
				     	  		 	 	   	 	   	tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
				     	  		 	 	   	 	   	if (compareTwoTime(tmpTime, sysTime))    //心跳超过监测时限
				     	  		 	 	   	 	   	{
				     	  		 	 	   	 	 	 		if (
				     	  		 	 	   	 	 	 	   		tmpNode->lddtRetry>=99                     //搜索LDDT重试次数=99,即自动抄读OK
				     	  		 	 	   	 	 	     		 || tmpNode->lddtRetry<pnGate.lddtRetry    //或搜索LDDT重试次数小于最大重试次数
				     	  		 	 	   	 	 	    	 )
				     	  		 	 	   	 	     	{
				     	  		 	 	   	 	       	if (tmpNode->lddtRetry>=99)
				     	  		 	 	   	 	       	{
				     	  		 	 	   	 	        	tmpNode->lddtRetry = 0;
				     	  		 	 	   	 	       	}

				     	  		 	 	   	 	       	ifFound = 1;         	  		 	 	   	 	   
				     	  		 	 	   	 	       	copyCtrl[4].tmpCpLink = tmpNode;
				     	  		 	 	   	 	   
				     	  		 	 	   	 	       	if (debugInfo&PRINT_CARRIER_DEBUG)
				     	  		 	 	   	 	       	{
				     	  		 	 	   	 	        	printf(",已超过报警监测时限");
				     	  		 	 	   	 		   		}
					     	  		 	 	   	 		 	}
					     	  		 	 	   	 		 	else
					     	  		 	 	   	 		 	{
					     	  		 	 	   	 	      if (debugInfo&PRINT_CARRIER_DEBUG)
					     	  		 	 	   	 	      {
					     	  		 	 	   	 	        printf(",紧急搜索已超次");
					     	  		 	 	   	 		   	}         	  		 	 	   	 		 	 
     	  		 	 	   	 		 						}
     	  		 	 	   	 	   					}
				     	  		 	 	   	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
				     	  		 	 	   	 	   	{
				     	  		 	 	   	 	   	 	printf("\n");
				     	  		 	 	   	 	   	}
				     	  		 	 	   	 	   	break;
     	  		 	 	   	     				}
         	  		 	 	   	 
         	  		 	 	   	 				tmpNode = tmpNode->next;
         	  		 	 	   				}
                 	 	     		}
                 	 	 
                 	 	     		if (1==ifFound)
                 	 	     		{
                 	 	 	   			break;
                 	 	     		}
                         	 	 
                 	 	     		//第二末端单灯控制器地址不为全0
                 	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->collectorAddr, NULL, 1))
                 	 	     		{
                 	 	       		tmpNode = copyCtrl[4].cpLinkHead;
         	  		 	 	   				while(tmpNode!=NULL)
         	  		 	 	   				{
         	  		 	 	   	 				if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->collectorAddr, 0)==TRUE)
         	  		 	 	   	 				{
			         	  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
			         	  		 	 	   	   	{
					         	  		 	 	   	 	printf("%02d-%02d-%02d %02d:%02d:%02d:线缆盗割报警控制点(%02x-%02x-%02x-%02x-%02x-%02x)的第二末端单灯控制器(%02x-%02x-%02x-%02x-%02x-%02x)的最近通信时间为%02d-%02d-%02d %02d:%02d:%02d",
					         	  		 	 	   	 	       sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
					         	  		 	 	   	 		      carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
					         	  		 	 	   	 		       tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
					         	  		 	 	   	 		         tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
					         	  		 	 	   	 		    );
					         	  		 	 	   	}
         	  		 	 	   	 	 
         	  		 	 	   	   				//tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
         	  		 	 	   	   				tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
     	  		 	 	   	 	   					if (compareTwoTime(tmpTime, sysTime))    //心跳超过监测时限
     	  		 	 	   	 	   					{
     	  		 	 	   	 	 	 						if (
     	  		 	 	   	 	 	 	   						tmpNode->lddtRetry>=99                     //搜索LDDT重试次数=99,即自动抄读OK
     	  		 	 	   	 	 	      					 || tmpNode->lddtRetry<pnGate.lddtRetry    //或搜索LDDT重试次数小于最大重试次数
     	  		 	 	   	 	 	    					 )
     	  		 	 	   	 	     					{
					     	  		 	 	   	 	      if (tmpNode->lddtRetry>=99)
					     	  		 	 	   	 	      {
					     	  		 	 	   	 	        tmpNode->lddtRetry = 0;
					     	  		 	 	   	 	      }

				     	  		 	 	   	 	        ifFound = 1;         	  		 	 	   	 	   
				     	  		 	 	   	 	        copyCtrl[4].tmpCpLink = tmpNode;
				     	  		 	 	   	 	   
				     	  		 	 	   	 	        if (debugInfo&PRINT_CARRIER_DEBUG)
				     	  		 	 	   	 	        {
				     	  		 	 	   	 	          printf(",已超过报警监测时限");
				     	  		 	 	   	 		      }
					     	  		 	 	   	 		 	}
					     	  		 	 	   	 		 	else
					     	  		 	 	   	 		 	{
				     	  		 	 	   	 	       	if (debugInfo&PRINT_CARRIER_DEBUG)
				     	  		 	 	   	 	       	{
				     	  		 	 	   	 	        	printf(",紧急搜索已超次");
				     	  		 	 	   	 		   	 	}         	  		 	 	   	 		 	 
				     	  		 	 	   	 		   	}
     	  		 	 	   	 	   					}

																	if (debugInfo&PRINT_CARRIER_DEBUG)
			     	  		 	 	   	 	   		{
			     	  		 	 	   	 	     		printf("\n");
			     	  		 	 	   	 	   		}
			     	  		 	 	   	 	   		break;
			     	  		 	 	   	     	}
         	  		 	 	   	 
         	  		 	 	   	 				tmpNode = tmpNode->next;
         	  		 	 	   				}
                 	 	     		}
                 	 	 
                 	 	     		if (1==ifFound)
                 	 	     		{
                 	 	 	   			break;
                 	 	     		}
                          }
                       	 	 
                       	  carrierFlagSet.searchLdgmNode = carrierFlagSet.searchLdgmNode->next;
                       	}
                       	 
                       	if (carrierFlagSet.searchLdgmNode==NULL)
                       	{
                       	  //固定每2分钟查询一次
                       	  carrierFlagSet.searchLddtTime = nextTime(sysTime, 2, 0);

                       	  //恢复抄表
                          carrierFlagSet.workStatus = 6;
                          carrierFlagSet.reStartPause = 3;
                          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,搜索LDDT完成后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                       	}
           	  	      }
           	  	      else
           	  	      {
                        if (
             	 	 	     			((carrierFlagSet.searchLdgmNode->status&0x1)==1)    //线路是交流供电
             	 	 	      		 && (    //2分钟获取的状态
             	 	 	          			(timeCompare(carrierFlagSet.searchLdgmNode->statusTime, sysTime, 2) == TRUE)
             	 	 	          			|| (timeCompare(sysTime, carrierFlagSet.searchLdgmNode->statusTime,  2) == TRUE)
             	 	 	         			)
             	 	 	    		 )
             	 	     		{
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                     	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发送紧急搜索数据.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                           
                          memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          copyCtrl[port].ifCollector = 0;      //无采集器地址
                          copyCtrl[port].protocol = 2;
                           
                          tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);
  
     	  		 	 	   				sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
                           
                          copyCtrl[port].flagOfRetry  = FALSE;
       		 	 	       			copyCtrl[port].copyContinue = FALSE;
                       
                          //召测等待时间为90秒
                          copyCtrl[port].copyTimeOut = nextTime(sysTime, 1, 30);
                        }
                      }
         	  	     	   
         	  	       	goto breakPoint;
                    }
                    else
                    {
                      carrierFlagSet.searchLddt = 0;
                     	 
                      goto breakPoint;
                    }
         	  	   	}
         	  	     
         	  	   	//搜索离线单灯控制器
         	  	   	if (1==carrierFlagSet.searchOffLine)
         	  	   	{
     	  	     	 		if (copyCtrl[port].tmpCpLink!=NULL)
     	  	     	 		{
       	  	     	   	copyCtrl[port].tmpCpLink->offLineRetry++;
       	  	     	   	if (copyCtrl[port].tmpCpLink->offLineRetry>pnGate.offLineRetry)
       	  	     	   	{
       	  	     	     	if (copyCtrl[port].tmpCpLink->offLineRetry<99)
       	  	     	     	{
       	  	     	 	   		//应该记录单灯控制点离线了
                          searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].tmpCpLink->mp, 3);    //单点控制点与时间无关量
                  	
                  	      //记录发生事件
             	  		   		printf("%02d-%02d-%02d %02d:%02d:%02d:单灯控制点(%02x-%02x-%02x-%02x-%02x-%02x)不见了,判断为离线\n",
             	  		 	 	   				sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
             	  		 	 	    				copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],copyCtrl[port].tmpCpLink->addr[3],copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]
             	  		 	 	 				);

                  		   	//未记录的才记录
                  		   	if (0==(meterStatisExtranTimeS.mixed&0x08))
                  		   	{
                    		 		//记录控制点故障信息(ERC13)
                    	     	eventData[0] = 13;
                    	     	eventData[1] = 11;
                    	
                    	     	tmpTime = timeHexToBcd(sysTime);
                    	     	eventData[2] = tmpTime.second;
                    	     	eventData[3] = tmpTime.minute;
                    	     	eventData[4] = tmpTime.hour;
                    	     	eventData[5] = tmpTime.day;
                    	     	eventData[6] = tmpTime.month;
                    	     	eventData[7] = tmpTime.year;
                    	  
                    	     	eventData[8] = copyCtrl[port].tmpCpLink->mp&0xff;
                    	     	eventData[9] = (copyCtrl[port].tmpCpLink->mp>>8&0xff) | 0x80;    //发生
                    	     	eventData[10] = 0x40;    //单灯控制器离线
                    	  
                    	     	writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                    	  
                    	     	if (eventRecordConfig.nEvent[1]&0x10)
                    	     	{
                    	       	writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                    	     	}
                    	  
                    	     	eventStatus[1] = eventStatus[1] | 0x10;
            					  
			            				 	if (debugInfo&PRINT_CARRIER_DEBUG)
			            				 	{
			            				   	printf("  --记录单灯控制点%d离线发生事件\n", copyCtrl[port].tmpCpLink->mp);
			            				 	}
                         
                            activeReport3();    //主动上报事件
                    	   
                    	     	meterStatisExtranTimeS.mixed |= 0x08;
                          
                            //存储控制点统计数据(与时间无关量)
                            saveMeterData(copyCtrl[port].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                          }
	                       	else
	                       	{
    					     					if (debugInfo&PRINT_CARRIER_DEBUG)
    					     					{
    					       					printf("  --单灯控制点%d离线发生事件已记录过了\n", copyCtrl[port].tmpCpLink->mp);
    					     					}
	                       	}
       	  	     	 	 		}
           	  	     	 	 
           	  	     	 	//移位到下一个需要召测的离线单灯控制器
                       	ifFound = 0;
                       	tmpNode = copyCtrl[4].tmpCpLink->next;
                       	while(tmpNode!=NULL)
                       	{
                   	 	   	if (
                   	 	 	   		//2016-11-28,单灯离线阈值的单位从“分”改为”小时“
							   							compareTwoTime(nextTime(tmpNode->statusTime, pnGate.lddOffGate*60, 0), sysTime)==TRUE
           	 	 	            	 && (tmpNode->offLineRetry<pnGate.offLineRetry || tmpNode->offLineRetry>=99)
                   	 	 	     		&& (tmpNode->lineOn>=1 && tmpNode->lineOn<=5)    //单灯控制器处于上电状态
                   	 	 	      	&& 1==tmpNode->joinUp
                   	 	 	     )
                   	 	   	{
                   	 	 	 		if (tmpNode->offLineRetry>=99)
                   	 	 	 		{
                   	 	 	   		tmpNode->offLineRetry = 0;
                   	 	 	 		}

                   	 	 	 		ifFound = 1;
                   	 	 	 
                   	 	 	 		copyCtrl[4].tmpCpLink = tmpNode;
                   	 	 	 		break;
                   	 	   	}
                   	 	 
                   	 	   	tmpNode = tmpNode->next;
                       	}
                       	 
                       	if (tmpNode==NULL)
                       	{
                       	  //固定每3分钟查询一次
                       	  carrierFlagSet.searchOffLineTime = nextTime(sysTime, 3, 0);

                       	  //恢复抄表
                          carrierFlagSet.workStatus = 6;
                          carrierFlagSet.reStartPause = 3;
                          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                     	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,搜索离线单灯控制器完成后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                       	}
           	  	      }
           	  	      else
           	  	      {
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
   	                      printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发现离线单灯控制器,发送紧急搜索数据.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                        }
                         
                        memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                        memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                        copyCtrl[port].ifCollector = 0;      //无采集器地址
                        copyCtrl[port].protocol = 2;
                         
                        tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);

   	  		 	 	     			sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
                         
                        copyCtrl[port].flagOfRetry  = FALSE;
     		 	 	     				copyCtrl[port].copyContinue = FALSE;
                     
                        //召测等待时间为90秒
                        copyCtrl[port].copyTimeOut = nextTime(sysTime, 1, 30);
                      }
                    }
                    else
                    {
                      carrierFlagSet.searchOffLine = 0;
                    }
                     	 
                    goto breakPoint;
         	  	   	}
         	  	     
         	  	   	//查询线路刚上电单灯控制器状态,2015-11-12
         	  	   	if (1==carrierFlagSet.searchLddStatus)
         	  	   	{
         	  	    	if (pDotCopy==NULL && (copyCtrl[port].pForwardData==NULL))
         	  	     	{
           	  	      if (copyCtrl[port].tmpCpLink!=NULL)
           	  	      {
         	  	     	 		copyCtrl[port].tmpCpLink->lineOn++;
         	  	     	 		if (copyCtrl[port].tmpCpLink->lineOn>3)
         	  	     	 		{
     	  	     	 	   			//移位到下一个需要召测的离线单灯控制器
                     	   	ifFound = 0;
                     	  	tmpNode = copyCtrl[4].tmpCpLink->next;
                     	   	while(tmpNode!=NULL)
                     	   	{
                     	 	 		if (1==tmpNode->lineOn)
                     	 	 		{
                     	 	   		ifFound = 1;
                     	 	 	 
                     	 	   		copyCtrl[4].tmpCpLink = tmpNode;
                     	 	   		break;
                     	 	 		}
                     	 	 
                     	 	 		tmpNode = tmpNode->next;
                     	   	}
                     	 
                     	   	if (tmpNode==NULL)
                     	   	{
                     	 	 		//固定每1分钟查询一次
                     	 	 		carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 1, 0);

                     	 	 		//恢复抄表
                            carrierFlagSet.workStatus = 6;
                            carrierFlagSet.reStartPause = 3;
                            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                            if (debugInfo&PRINT_CARRIER_DEBUG)
                            {
   	                          printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,查询单灯控制器状态完成后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                            }
                     	   	}
         	  	     	 		}
         	  	     	 		else
         	  	     	 		{
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                     	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,发送查询刚上电单灯控制器状态数据.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                           
                          memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          copyCtrl[port].ifCollector = 0;      //无采集器地址
                          copyCtrl[port].protocol = 2;
                           
                          tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);
  
     	  		 	 	   				sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
                           
                          copyCtrl[port].flagOfRetry  = FALSE;
       		 	 	       			copyCtrl[port].copyContinue = FALSE;
                       
                          //召测等待时间为90秒
                          copyCtrl[port].copyTimeOut = nextTime(sysTime, 1, 30);
                        }
                      }
                      else
                      {
                       	carrierFlagSet.searchLddStatus = 0;
                      }
                    }
                    else
                    {
                      carrierFlagSet.searchLddStatus = 0;
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                       	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,正在查询刚上电单灯控制器状态时有转发或点抄,结束查询执行转发或点抄.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                      }
                    }
                     	 
                    goto breakPoint;
         	  	   	}
       	  	    }
       	  	  }
     	  	     
     	  	  #endif
     	  	    
 	  	       	//2-3-2-3-1.1.点抄(载波端口)
 	  	       	//2013-12-30,添加&& (copyCtrl[port].pForwardData==NULL)
 	  	       	if (pDotCopy!=NULL && (copyCtrl[port].pForwardData==NULL))
 	  	       	{
     	  	     	//路由主导抄表的话,要查看是否在抄表且启动状态
     	  	     	if (carrierFlagSet.routeLeadCopy==1)
     	  	     	{
 	  	     	   		memset(copyCtrl[port].needCopyMeter, 0x00, 6);

 	  	     	   		if (carrierFlagSet.workStatus<4)
 	  	     	   		{
  	     	     			carrierFlagSet.workStatus = 4;
  	     	     			carrierFlagSet.reStartPause = 2;
                   	carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);

                   	copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
                   	copyCtrl[port].flagOfRetry = FALSE;
 	  	     	     		goto breakPoint;
 	  	     	   		}
 	  	     	 
 	  	     	   		if (carrierFlagSet.workStatus<5)
 	  	     	   		{
 	  	     	 	 			goto breakPoint;
 	  	     	   		}
     	  	     	}

                dotCopy(PORT_POWER_CARRIER);
                if (pDotCopy!=NULL)
                {
                  if (pDotCopy->dotCopying==FALSE && carrierFlagSet.routeLeadCopy==1)
                  {
                    //路由主导抄表方式下,进入这种状态是多测量点点抄的时候
                    copyCtrl[port].flagOfRetry = TRUE;
                  }
                  else
                  {
                    //2013-12-27,鼎信要求等待时间为90s以上
                    //  因此将所有模块都统一到这个等待最长的时间上
                    //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
                    pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);

                    copyCtrl[port].flagOfRetry  = FALSE;                   	 
                  }
                }
                else
                {
                  copyCtrl[port].copyTimeOut  = nextTime(sysTime, 0, 2);
                  copyCtrl[port].flagOfRetry  = FALSE;
                }
                 
                copyCtrl[port].copyContinue = FALSE;
                goto breakPoint;
              }
      	  	  else    //ly,2011-05-17,add
      	  	  {
      	  	   	if (pDotFn129!=NULL)
      	  	   	{
              	  pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
        		   		pDotCopy->dotCopyMp   = pDotFn129->pn;
        		   		pDotCopy->dotCopying  = FALSE;
        		   		pDotCopy->port        = PORT_POWER_CARRIER;
        		  		pDotCopy->dataFrom    = pDotFn129->from;
        		   		pDotCopy->outTime     = nextTime(sysTime,0,15);
        		   		pDotCopy->dotResult   = RESULT_NONE;
        		   		pDotFn129->ifProcessing = 1;         //正在处理

        		   		goto breakPoint;
      	  	   	}
      	  	  }
     	  		}
     	  	 #endif    //#ifdef PLUG_IN_CARRIER_MODULE

     	  	 	//2-3-2-3-1.2.转发(485端口及载波端口)
        	 	if (copyCtrl[port].pForwardData!=NULL)
        	 	{
             #ifdef PLUG_IN_CARRIER_MODULE
              //2013-12-30,添加,正在点抄的话,等待点抄完成
              if (4==port)
              {
               	if (pDotCopy!=NULL)
               	{
               		goto breakPoint;
               	}
             	}
             #endif
               
              forwardData(port);
                
             #ifdef PLUG_IN_CARRIER_MODULE
              if (port==4 && carrierFlagSet.routeLeadCopy==1)
              {
                copyCtrl[port].copyTimeOut  = nextTime(sysTime, 0, 2);
                copyCtrl[port].flagOfRetry  = FALSE;
                copyCtrl[port].copyContinue = FALSE;
              }
             #endif

              goto breakPoint;
     	  	 	}
             
   	        //2-3-2-3-2.发送抄表数据帧
          #ifdef PLUG_IN_CARRIER_MODULE
           #ifdef LM_SUPPORT_UT
            if (port>=4 && 0x55!=lmProtocol)
           #else
            //if (port==4)
            if (port>=4)    //ly,2012-01-09
           #endif
            {
              //如果到每个小时50分而实时数据还未抄完,则暂停抄实时数据,因为每小时1分开始要抄整点冻结数据
              //ly,2011-01-15,由每小时0分改成每小时50分停止抄表,因为锐拔模块要留出一定时间来组网
              //ly,2011-07-07,集中器主导抄读时才进行这个动作
              if (copyCtrl[port].ifCopyLastFreeze==0 && sysTime.minute==50 && sysTime.second>=0 && carrierFlagSet.routeLeadCopy==0)
              {
               	copyCtrl[port].meterCopying = FALSE;

               	//ly,2011-01-14,add,为了解决一个小时抄不完而另一个小时要接着抄而没有接着抄的bug
               	copyCtrl[port].backMp = copyCtrl[port].tmpCpLink->mp;
               	goto breakPoint;
              }
               
              if (copyCtrl[port].flagOfRetry == TRUE && carrierFlagSet.routeLeadCopy==1)
              {
               	//路由主导抄表时,为进入循环的超时时间为2秒
               	copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
              }
              else
              {
                if (carrierModuleType==MIA_CARRIER || carrierModuleType==EAST_SOFT_CARRIER)
                {
                  //弥亚微建议的抄表超时时间为40s,东软的载波模块实测为35s左右
                  copyCtrl[port].copyTimeOut  = nextTime(sysTime, 0, 42);
                }
                else
                {
                  copyCtrl[port].copyTimeOut  = nextTime(sysTime, 0, 20);
                }
              }
            }
            else
            {
          #endif    //PLUG_IN_CARRIER_MODULE 
            
             #ifdef LIGHTING
              copyCtrl[port].copyTimeOut = nextTime(sysTime,0,4);    //2016-03-21,照明集中器485改成4秒
             #else
              copyCtrl[port].copyTimeOut = nextTime(sysTime,0,3);
             #endif
            
          #ifdef PLUG_IN_CARRIER_MODULE
            }
          #endif
             
	         	if (copyCtrl[port].flagOfRetry==TRUE)
	         	{
	          #ifdef PLUG_IN_CARRIER_MODULE
	           #ifdef LM_SUPPORT_UT
	            if (port>=4 && 0x55!=lmProtocol)
	           #else
	            if (port>=4)
	           #endif
	            {
	              if (carrierFlagSet.routeLeadCopy==0)
	              {
	                if (copyCtrl[port].retry>=NUM_COPY_RETRY)
	                {
	                  copyCtrl[port].numOfMeterAbort++;
	                  if (copyCtrl[port].numOfMeterAbort > 1 && copyCtrl[port].hasSuccessItem<2)
	                  {
	                    if (debugInfo&METER_DEBUG)
	                    {
	                   	  printf("载波抄表(测量点%d):两项数据没抄到,终止该测量点本次抄读\n", copyCtrl[port].tmpCpLink->mp);
	                    }
	                   
	                 		goto stopThisMeterPresent;
	                  }

	                  copyCtrl[port].retry = 0;
	 
	                  if (debugInfo&METER_DEBUG)
	                  {
	                    printf("载波抄表:超时超次,继续下一项\n");
	                  }

	                  ret = meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);           //组帧发送(抄读下一项)                     
	                }
	                else
	                {
	                  if (debugInfo&METER_DEBUG)
	                  {
	                    printf("载波抄表:超时,重试上一项\n");
	                  }

	                  ret = meterFraming(PORT_POWER_CARRIER, LAST_COPY_ITEM_RETRY);    //组帧发送(重试上一项)
	                }
	              }
	              else
	              {
                	//路由主导抄表超时不发抄表指令,等待路由请求时再发
                	if (debugInfo&PRINT_CARRIER_DEBUG)
                	{
                	  //printf("路由主导抄表超时不发抄表指令,等待路由请求时再发\n");
                	}
                	 
                	copyCtrl[port].flagOfRetry  = FALSE;
                	 
                	goto breakPoint;
                }
              }
              else
              {
            #endif
               
              	if (copyCtrl[port].retry>=NUM_COPY_RETRY)
              	{
                	if (copyCtrl[port].numOfMeterAbort!=0xfe)
                	{
                  	copyCtrl[port].numOfMeterAbort++;
                    
                  	//只要抄表失败>6项就认为485抄表故障(LY 08.12.22改成在这里判断,
                  	//只要抄表失败>1项就认为485抄表故障(LY 18.08.06),
                  	//而不是在即使抄不到表也要抄完再判断,那样的话在抄不到表的情况下2分钟以内抄不完一块表)
                 	 #ifdef LIGHTING
					  				if (copyCtrl[port].numOfMeterAbort > 0)
					 				 #else
					  				if (copyCtrl[port].numOfMeterAbort > 14)
					 				 #endif
                    {
                      copyCtrl[port].numOfMeterAbort = 0xfe;
                      		  
                      //记录终端485抄表失败事件
                      copy485Failure(copyCtrl[port].cpLinkHead->mp, 1, port+1);

                     #ifdef LIGHTING
												 
									    //2016-11-25,本轮抄读中有抄读失败
									    copyCtrl[port].thisRoundFailure = TRUE;

                     #else

									    //记录终端故障 - 485抄表故障
									    if ((eventRecordConfig.iEvent[2] & 0x10) || (eventRecordConfig.nEvent[2] & 0x10))
									    {
										  	eventData[0] = 0x15;
										  	eventData[1] = 0xa;
												
										  	//发生时间
										  	eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
										  	eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
										  	eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
										  	eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
										  	eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
										  	eventData[7] = sysTime.year/10<<4   | sysTime.year%10;

										  	//故障编码--485抄表故障
										  	eventData[8] = 0x4;
										 
										  	eventData[9] = 0x0;

										  	if (eventRecordConfig.iEvent[2] & 0x10)
										  	{
													writeEvent(eventData,10, 1, DATA_FROM_GPRS);
										  	}

										 	 	if (eventRecordConfig.nEvent[2] & 0x10)
										  	{
													writeEvent(eventData, 10, 2, DATA_FROM_LOCAL);
										  	}
												
										  	eventStatus[0] = eventStatus[2] | 0x10;
											
										  	activeReport3();   //主动上报事件
									    }
										
									   #endif
                     	    
                     	copyCtrl[port].retry = 0;
                     	    
                     	if (debugInfo&METER_DEBUG)
                     	{
                     	  printf("copyMeter:超时次数大于设定抄表失败的超时次数,判定为抄表485故障.未保存抄到的数据\n");
                     	}
                     	    
                     	goto stopThisMeterPresent;
                    }
                  }
                     
                  copyCtrl[port].retry = 0;

                 #ifdef LIGHTING
                  //线路控制器
                  if (LIGHTING_XL==copyCtrl[port].cpLinkHead->protocol)
                  {
               	  	ret = xlcFrame(port);
                  }
                  else if (LIGHTING_DGM==copyCtrl[port].cpLinkHead->protocol)
                  {
                    ret = ldgmFrame(port);
                  }
                  else if (LIGHTING_LS==copyCtrl[port].cpLinkHead->protocol)
                  {
                    ret = lsFrame(port);
                  }
                  else if (LIGHTING_TH==copyCtrl[port].cpLinkHead->protocol)
                  {
                    ret = thFrame(port);
                  }
                  else if (AC_SAMPLE==copyCtrl[port].cpLinkHead->protocol && copyCtrl[port].cpLinkHead->port>1)
               	  {
               	   	ret = xlcAcFrame(port);
               	  }
               	  else
               	  {
                 #endif

                    //组帧发送(抄读下一项)
                    //2012-3-28,modify这里的处理
                    //ret = meterFraming(port+1, NEW_COPY_ITEM);        //组帧发送(抄读下一项)
                   #ifdef RS485_1_USE_PORT_1
                    ret = meterFraming(port+1, NEW_COPY_ITEM);
                   #else
                    if (port==0)
                    {
                      ret = meterFraming(1, NEW_COPY_ITEM);
                    }
                    else
                    {
                     #ifdef LM_SUPPORT_UT
                      if (4==port)
                      {
                        ret = meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);
                      }
                      else
                      {
                     #endif
                      
                        ret = meterFraming(port, NEW_COPY_ITEM);
                         
                     #ifdef LM_SUPPORT_UT
                      }
                     #endif
                    }
                   #endif
                      
                 #ifdef LIGHTING
                  }
                 #endif
                }
                else  //组帧发送(重试上一项)
                {
                   	
                 #ifdef LIGHTING
                 	//线路控制器
                 	if (LIGHTING_XL==copyCtrl[port].cpLinkHead->protocol)
                 	{
             	   		ret = xlcFrame(port);
                 	}
									  //报警控制器
                 	else if (LIGHTING_DGM==copyCtrl[port].cpLinkHead->protocol)
                  {
                    ret = ldgmFrame(port);
                  }
                  else if (LIGHTING_LS==copyCtrl[port].cpLinkHead->protocol)
                 	{
                   	ret = lsFrame(port);
                 	}
                  else if (LIGHTING_TH==copyCtrl[port].cpLinkHead->protocol)
                 	{
                   	ret = thFrame(port);
                 	}
                 	else if (AC_SAMPLE==copyCtrl[port].cpLinkHead->protocol && copyCtrl[port].cpLinkHead->port>1)
           	   		{
           	   	 		ret = xlcAcFrame(port);
           	   		}
           	   		else
           	   		{

                 #endif

                   	//2012-3-28,modify这里的处理
                   	//ret = meterFraming(port+1, LAST_COPY_ITEM_RETRY);
                   #ifdef RS485_1_USE_PORT_1
                 
                   	ret = meterFraming(port+1, LAST_COPY_ITEM_RETRY);

                   	//ABB握手中
                   	if (abbHandclasp[port]==1)
                   	{
               	   		copyCtrl[port].copyTimeOut = nextTime(sysTime,0,1);
                   	}

                   #else
                
                   	if (port==0)
                   	{
                    	ret = meterFraming(1, LAST_COPY_ITEM_RETRY);
                   	}
                   	else
                   	{
                     #ifdef LM_SUPPORT_UT
                     	if (4==port)
                     	{
                       	ret = meterFraming(PORT_POWER_CARRIER, LAST_COPY_ITEM_RETRY);
                     	}
                     	else
                     	{
                     #endif
                                                   
                       	ret = meterFraming(port, LAST_COPY_ITEM_RETRY);
                    
                     #ifdef LM_SUPPORT_UT
                     	}
                    	#endif
                   	}
                
                    //ABB握手中
                    if (abbHandclasp[port-1]==1)
                   	{
               	     	copyCtrl[port].copyTimeOut = nextTime(sysTime,0,1);
                   	}

                   #endif
                      
                 #ifdef LIGHTING
                 	}
                 #endif
                }

             #ifdef PLUG_IN_CARRIER_MODULE 
              }
             #endif    //#ifdef PLUG_IN_CARRIER_MODULE
 
              copyCtrl[port].flagOfRetry  = FALSE;
            }
            else                                  //继续下一项抄读
            {
              copyCtrl[port].copyContinue = FALSE;
               
            #ifdef PLUG_IN_CARRIER_MODULE
             #ifdef LM_SUPPORT_UT
              if (port>=4 && 0x55!=lmProtocol)    //载波/无线端口
             #else
              if (port>=4)    //载波/无线端口
             #endif
              {
                carrierSendToMeter = 1;    //2013-12-25
                 
                if (carrierFlagSet.routeLeadCopy==1)    //路由主导抄表
                {
		  		 	 	   	if (compareTwoAddr(copyCtrl[port].needCopyMeter, 0, 1)==TRUE)
		  		 	 	   	{
		  		 	 	   	 	if (debugInfo&PRINT_CARRIER_DEBUG)
		  		 	 	   	 	{
		  		 	 	   	  	printf("路由主导抄读:暂存路由请求的地址全是0,不发送抄表数据\n");
		  		 	 	   	 	}
		  		 	 	   	 
		  		 	 	   	 	goto breakPoint;
		  		 	 	   	}
		                   
                  //本次请求的地址与上次抄读的地址不一致的,如果抄读标志不是0的话(这是异常,肯定是上次抄时有没抄到的数据项,路由就不请求了),
                  //   要将他置为0,以便下次请求时从头开始抄,避免漏抄
		 	 	   				if (copyCtrl[port].tmpCpLink!=NULL)
		 	 	   				{
	 	 	   	 	 				if (debugInfo&PRINT_CARRIER_DEBUG)
	 	 	   	 	 				{
				 	 	   	 	   	//printf("请求地址:%02x%02x%02x%02x%02x%02x\n",carrierFlagSet.needCopyMeter[5],carrierFlagSet.needCopyMeter[4],
				 	 	   	 	   	//  carrierFlagSet.needCopyMeter[3],carrierFlagSet.needCopyMeter[2],carrierFlagSet.needCopyMeter[1],carrierFlagSet.needCopyMeter[0]);
				 	 	   	 	   	//printf("tmpLink地址:%02x%02x%02x%02x%02x%02x\n",copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],
				 	 	   	 	   	//  copyCtrl[port].tmpCpLink->addr[3],copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]);
				 	 	   	 	 	}

		 	 	           #ifdef LIGHTING
		 	 	      			if (compareTwoAddr(copyCtrl[port].needCopyMeter, copyCtrl[port].tmpCpLink->addr, 0)==FALSE)     	  		 	 	     
		 	 	           #else
					 	 	     	//如果有采集器地址,则比较采集地址与路由请求的地址;如果没有采集器地址,则比较电表地址与采集器地址
					 	 	     	//   当前请求的地址与上一次采集的地址不同,而上一次没有采集完成的话,要将上次采集的节点抄表标志置为0,下次请求时重新抄读
					 	 	     	if (((compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE) && (compareTwoAddr(copyCtrl[port].needCopyMeter, copyCtrl[port].tmpCpLink->collectorAddr, 0)==FALSE))
					 	 	     	    || ((compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==TRUE) && (compareTwoAddr(copyCtrl[port].needCopyMeter, copyCtrl[port].tmpCpLink->addr, 0)==FALSE))
					 	 	     		)
					 	 	     #endif
					 	 	     	{
				 	 	   	      if (copyCtrl[port].tmpCpLink->flgOfAutoCopy!=0)
				 	 	   	      {
				 	 	   	 	     	copyCtrl[port].tmpCpLink->flgOfAutoCopy=0;
			   	  		 	 	   	 	     
                       #ifdef LIGHTING
                       	//30分钟未抄读到读CCB的信息,将灯状态置为未知
                        if (!((timeCompare(copyCtrl[port].tmpCpLink->statusTime, sysTime, 30) == TRUE) 
     	                      || (timeCompare(sysTime, copyCtrl[port].tmpCpLink->statusTime, 30) == TRUE)))
     	                 	{
     	                   	//2014-09-19,status表示的是灯的亮度了
     	                   	copyCtrl[port].tmpCpLink->status = 0xfe;
     	                 	}
     	                	#endif

				 	 	   	 	     	if (debugInfo&PRINT_CARRIER_DEBUG)
				 	 	   	 	     	{
				 	 	   	 	   	   	printf("路由主导抄读:上一块表没请求完,复位标志,下次请求时从头开始抄\n");
				 	 	   	 	     	}
				 	 	   	      }
			     	  		 	}
			     	  		}
     	  		 	 	   
  		 	 	  			ifFound = 0;
  		 	 	   			copyCtrl[port].tmpCpLink = copyCtrl[4].cpLinkHead;
  		 	 	   			while(copyCtrl[port].tmpCpLink!=NULL)
  		 	 	   			{
  		 	 	   			 	if (compareTwoAddr(copyCtrl[port].tmpCpLink->addr, copyCtrl[port].needCopyMeter, 0)==TRUE)
  		 	 	   	 			{
  		 	 	   	   			if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	   	   			{
  		 	 	   	 	 				printf("路由主导抄读:在载波链表中找到要抄读的表地址\n");
  		 	 	   	   			}
  		 	 	   	 		
  		 	 	   	   			ifFound = 1;
  		 	 	   	   			break;
  		 	 	   	 			}
  		 	 	   	 			copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
  		 	 	   			}
     	  		 	 	   
		 	 	   				//请求的地址是采集器地址的处理(一个采集器下可能有多个485表计)
		 	 	   				if (ifFound==0)
		 	 	   				{
repeatSearch: 	    copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
  		 	 	     			ifFound = 0;
  		 	 	     			while(copyCtrl[port].tmpCpLink!=NULL)
  		 	 	     			{
  		 	 	   	   			if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, copyCtrl[port].needCopyMeter, 0)==TRUE)
  		 	 	   	   			{
	 	 	   	 		 					if (copyCtrl[port].tmpCpLink->thisRoundCopyed==FALSE)
	 	 	   	 		 					{
					 	 	   	 		    if (debugInfo&PRINT_CARRIER_DEBUG)
					 	 	   	 		    {
					 	 	   	 		      printf("路由主导抄读:在载波链表中找到要抄读的采集器地址\n");
					 	 	   	 		    }
					 	 	   	 		    ifFound = 1;
					 	 	   	 		    break;
					 	 	   	 		 	}
					 	 	   	 		 	else
					 	 	   	 		 	{
					 	 	   	 		   	ifFound = 2;
					 	 	   	 		 	}
  		 	 	   	   			}
  		 	 	   	   			copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
  		 	 	   	 			}
     	  		 	 	   	 
					 	 	   	 	if (ifFound==2)
					 	 	   	 	{
					 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
					 	 	   	   	{
					 	 	   	 	 		printf("路由主导抄读:在载波链表中找到要抄读的采集器地址,但本采集下本轮都已抄完,重新置位置未抄读标志\n");
					 	 	   	   	}
			     	  		 	 	   	 	 
			  		 	 	      copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
			  		 	 	      while(copyCtrl[port].tmpCpLink!=NULL)
			  		 	 	      {
			  		 	 	   	    if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, copyCtrl[port].needCopyMeter, 0)==TRUE)
			  		 	 	   	    {
			  		 	 	   	 	   	copyCtrl[port].tmpCpLink->thisRoundCopyed = FALSE;
			  		 	 	   	    }
			  		 	 	   	    copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
			  		 	 	   	  }
			  		 	 	   	 
			  		 	 	   	  goto repeatSearch;
			  		 	 	    }
			  		 	 	  }
			     	  		 	 	   
		  		 	 	   	if (ifFound==1)
		  		 	 	   	{
		  		 	 	     	//2013-10-17,添加,路灯集中器载波端口抄读处理
		  		 	 	     #ifdef LIGHTING
		  		 	 	     
		  		 	 	     	//printf("autocopy=%x\n", copyCtrl[port].tmpCpLink->flgOfAutoCopy);
		  		 	 	     
		  		 	 	     	//AutoCopy-bit0-读单灯控制器状态
		  		 	 	     	//         bit1-读控制时段
		  		 	 	    	//         bit2-读开关调时刻
		  		 	 	     	//         bit3-读小时冻结数据
		  		 	 	     	//         bit4-校时
		  		 	 	     	//         bit5-校时段
		  		 	 	     	//         bit6-校调光参数
									 
		                //单灯控制点与时间无关量
							 			searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[4].tmpCpLink->mp, 3);

							 			memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
							 			memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
							 			copyCtrl[port].ifCollector = 0;      //无采集器地址

  		 	 	     			//单灯控制器需要校时
  		 	 	     			if (copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CHECK_TIME)
  		 	 	     			{
		 	 	       				tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 1, copyCtrl[4].tmpCpLink);
		 	 	       				sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
  		 	 	     			}
 	 	             		else
 	 	             		{
 	 	               		if (
					       					(
														copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1         //SLC需要同步控制时段第一包
														 || copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2     //SLC需要同步控制时段第二包
														  || copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3    //SLC需要同步控制时段第三包
													)
														 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)                          //参数未同步才下发
												 )
 	 	               		{
                        tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 8, copyCtrl[4].tmpCpLink);
   	  		 	 	     			sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
     	  		 	   			}
  		 	 	       			else
  		 	 	       			{
  		 	 	         			if (
											     	copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_ADJ_PARA    //SLC需要调光参数
												    	&& ((meterStatisExtranTimeS.mixed&0x20)==0x00)                 //参数未同步才下发
											     )
			     	  		 	 	 	{
			                    tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 9, copyCtrl[4].tmpCpLink);
			   	  		 	 	      sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
			     	  		 	 	 	}
			     	  		 	 	 	else
			     	  		 	 	 	{
       	  		 	 	   			if ((copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_STATUS)==0x0)    //SLC查询状态数据包还未发送
       	  		 	 	   			{
                            tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);
       	  		 	 	     			sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
       	  		 	 	   			}
       	  		 	 	   			else
       	  		 	 	   			{
                            //有灯具接入的SLC查询软件版本还未发送
		  		 	 	              if (
		  		 	 	           	      (copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SOFT_VER)==0x0
		  		 	 	           	       && copyCtrl[port].tmpCpLink->joinUp==0x1
								                  && ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //参数未同步才下发
		  		 	 	           	     )
		  		 	 	             	{
		                         	tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 13, copyCtrl[4].tmpCpLink);
		       	  		 	 	       	sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
		       	  		 	 	     	}
		       	  		 	 	     	else
		       	  		 	 	     	{
													   	//有灯具接入的SLC查询控制时段小包还未发送
													   	if (
														   		 (
																		(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_1)==0x0            //小包1
												             ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_2)==0x0         //小包2
												              ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_3)==0x0        //小包3
												               ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_4)==0x0       //小包4
												                ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_5)==0x0      //小包5
												                 ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_6)==0x0     //小包6
												                  ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_7)==0x0    //小包7
																	 )
															  	  && copyCtrl[port].tmpCpLink->joinUp==0x1
																		 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //参数未同步才下发
														     )
													   	{
														 		tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 10, copyCtrl[4].tmpCpLink);
														 		sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
													   	}
													   	else
													   	{
														 		//有灯具接入的SLC查询开关调数据包还未发送
														 		if (
															  		(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_FREEZE_TIMES)==0x0
															   		 && copyCtrl[port].tmpCpLink->joinUp==0x1
																	 )
														 		{
														   		tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 7, copyCtrl[4].tmpCpLink);
														   		sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
														 		}
														 		else
														 		{       	  		 	 	         	 
															   	//有灯具接入且有计量功能的单灯控制器的小时冻结数据
															   	if (
																  	  1==copyCtrl[port].tmpCpLink->funOfMeasure
																			 && copyCtrl[port].tmpCpLink->joinUp==0x1
																	 			&& (copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_HOUR_FREEZE)==0x0
																 		 )
															   	{
																 		tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 4, copyCtrl[4].tmpCpLink);

																 		if (0==tmpData)
																 		{
																   		goto copyOk;
																 		}
																 		else
																 		{
																   		sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
																 		}
															   	}
															   	else
															   	{
															     	copyOk:
															     	if (
																	 			(
																	   			//有灯具接入的SLC状态、控制时段和开关调时刻冻结数据都已经返回
																	   			copyCtrl[port].tmpCpLink->joinUp==0x1
																					 && 
																		 				(
																		  			 (
																					    ((meterStatisExtranTimeS.mixed&0x20)==0x00)
																						 		&& (REQUEST_STATUS | REQUEST_CTRL_TIME_1 | REQUEST_FREEZE_TIMES)==(copyCtrl[port].tmpCpLink->flgOfAutoCopy&(REQUEST_STATUS | REQUEST_CTRL_TIME_1 | REQUEST_FREEZE_TIMES))
																					    )
																						   ||
																						  (
																						   ((meterStatisExtranTimeS.mixed&0x20)==0x20)
																							 && (REQUEST_STATUS | REQUEST_FREEZE_TIMES)==(copyCtrl[port].tmpCpLink->flgOfAutoCopy&(REQUEST_STATUS | REQUEST_FREEZE_TIMES))
																						  )
																					  )
																        )
																		     ||
																		    (
																		     //无灯具接入的SLC状态数据已经返回
																		     copyCtrl[port].tmpCpLink->joinUp==0x0
																			    && (REQUEST_STATUS ==(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_STATUS))
																		    )
																	    )
								     								{
																	   	//抄读成功
																	   	tmpBuff[0] = 0x01;
																	   	tmpBuff[1] = 0x00;
																	   	tmpBuff[2] = 0x00;
																	   	if (carrierModuleType == TC_CARRIER)
																	   	{
																		 		tmpBuff[3] = port-3;
																		 		gdw3762Framing(ROUTE_DATA_READ_3762, 1, copyCtrl[port].destAddr, tmpBuff);
																	   	}
																	   	else
																	   	{
																		 		gdw3762Framing(ROUTE_DATA_READ_3762, 1, NULL, tmpBuff);
																	   	}

																	   	copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
																	 
																	   	if (debugInfo&PRINT_CARRIER_DEBUG)
																	   	{
																		 		printf("路由主导抄读:本CCB抄读成功\n");
																	   	}
																    }
																  }
																}
															}
						     						}
  		 	 	           			}
  		 	 	         			}
  		 	 	       			}
  		 	 	     			}
     	  		 	 	     
     	  		 	 			goto breakPoint;
     	  		 	 	     
     	  		 			 #else
     	  		 	 	    
  		 	 	     			//未初始化缓存
  		 	 	     			switch (copyCtrl[port].tmpCpLink->flgOfAutoCopy)
  		 	 	     			{
  		 	 	       			case 0:    //未抄表
                        if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
     	 	             		{
 	 	  	       	      	 #ifndef DKY_SUBMISSION
 	 	  	       	       		//2012-09-28,为适应象甘肃敦煌,兰州榆中的没有日冻结的07表而增加if处理,同时取消类号为0x55的设置,因为这会在主站端操作特别麻烦
 	 	  	       	       		//  修改为:设置居民用户表抄读模式为0x55(仅实时数据)和0xaa(仅实时总示值)来控制抄读项
 	 	  	       	       		if (denizenDataType==0x55 || denizenDataType==0xaa)
 	 	  	       	       		{
  		 	 	   	         			initCopyDataBuff(port, PRESENT_DATA);
  		 	 	   	         			copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
 	 	  	       	       		}
 	 	  	       	       		else
 	 	  	       	       		{
 	 	  	       	      	 #endif
 	 	  	       	    
     	 	  	             		//日冻结数据
     	 	  	             		if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
     	 	  	             		{
	  		 	 	   	       				initCopyDataBuff(port, LAST_DAY_DATA);
	  		 	 	   	       				copyCtrl[port].tmpCpLink->flgOfAutoCopy = 1;
     	 	  	             		}
     	 	  	             		else
     	 	  	             		{
     	 	  	           	
	     	 	  	       	       #ifndef DKY_SUBMISSION
	     	 	  	       	       	//2012-08-28,为精减抄读数据项而增加if处理
	     	 	  	       	       	//  如果大小类号都为0的话,日冻结采集成功,就不再采集其他数据项了,直接是"抄读成功"
	     	 	  	       	       	//  更改以后对严格执行标准的07表的默认处理方式为只采集日冻结数据
	     	 	  	       	       	if (copyCtrl[port].tmpCpLink->bigAndLittleType==0x00)
	     	 	  	       	       	{
	     	 	  	       	     	 		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 8;

				                     		//2013-12-25,add
				                     		carrierSendToMeter = 0;
				                     		memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
				                     		memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);

	     	 	  	       	     	 		if (debugInfo&PRINT_CARRIER_DEBUG)
	     	 	  	       	     	 		{
	 	 	  	       	     	 	   			printf("%02d-%02d-%02d %02d:%02d:%02d,初始化抄表缓存时,对大小类号都为0的07表(%02x%02x%02x%02x%02x%02x),只抄冻结数据,flagOfAutoCopy置为8\n", 
	 	 	  	       	     	 	         			 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
	 	 	  	       	     	 	          			copyCtrl[port].tmpCpLink->addr[5], copyCtrl[port].tmpCpLink->addr[4], copyCtrl[port].tmpCpLink->addr[3],
	 	 	  	       	     	 	           			copyCtrl[port].tmpCpLink->addr[2], copyCtrl[port].tmpCpLink->addr[1], copyCtrl[port].tmpCpLink->addr[0]
	 	 	  	       	     	 	        			);
	     	 	  	       	     	 		}
	     	 	  	       	       	}
	     	 	  	       	       	else
	     	 	  	       	       	{
	     	 	  	       	       #endif
	     	 	  	       	    
	     	 	  	       	         	//小时冻结数据
	     	 	  	       	         	if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
	     	 	  	       	         	{
	  		 	 	   	               		initCopyDataBuff(port, HOUR_FREEZE_DATA);
	  		 	 	   	               		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 3;
	     	 	  	       	         	}
	     	 	  	       	         	else    //抄实时数据
	     	 	  	       	         	{
	  		 	 	   	               		initCopyDataBuff(port, PRESENT_DATA);
	  		 	 	   	               		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
	     	 	  	       	         	}
	     	 	  	       	       
	     	 	  	       	       #ifndef DKY_SUBMISSION
	     	 	  	       	       	}
	     	 	  	       	       #endif
     	 	  	             		}
     	 	  	           
     	 	  	           	 #ifndef DKY_SUBMISSION
     	 	  	           		}
     	 	  	           	 #endif
     	 	  	         		}
     	 	  	         		else     //97表直接抄实时数据
     	 	  	         		{
  		 	 	   	       			initCopyDataBuff(port, PRESENT_DATA);
  		 	 	   	       			copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
     	 	  	         		}
     	 	  	         		break;
  	     	 	  	       
 	 	  	           		case 2:   //日冻结数据已抄完
 	 	  	             		if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
 	 	  	             		{
 	 	  	       	      	 #ifndef DKY_SUBMISSION
 	 	  	       	       		//2012-08-28,为精减抄读数据项而增加if处理
 	 	  	       	       		//  如果大小类号都为0的话,日冻结采集成功,就不再采集其他数据项了,直接是"抄读成功"
 	 	  	       	       		if (copyCtrl[port].tmpCpLink->bigAndLittleType==0x00)
 	 	  	       	       		{
			                 			//2014-01-03,add
			                 			carrierSendToMeter = 0;

 	 	  	       	         		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 8;
 	 	  	       	   
		 	 	  	       	 				if (debugInfo&PRINT_CARRIER_DEBUG)
		 	 	  	       	 				{
		 	 	  	       	   				printf("%02d-%02d-%02d %02d:%02d:%02d,初始化抄表缓存时,对大小类号都为0的07表(%02x%02x%02x%02x%02x%02x),日冻结数据已抄完,flagOfAutoCopy置为8\n", 
		 	 	  	       	          			 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
		 	 	  	       	           				copyCtrl[port].tmpCpLink->addr[5], copyCtrl[port].tmpCpLink->addr[4], copyCtrl[port].tmpCpLink->addr[3],
		 	 	  	       	            			 copyCtrl[port].tmpCpLink->addr[2], copyCtrl[port].tmpCpLink->addr[1], copyCtrl[port].tmpCpLink->addr[0]
		 	 	  	       	         				);
		 	 	  	       	 				}
		 	 	  	       				}
 	 	  	       	       		else
 	 	  	       	       		{
 	 	  	       	      	 #endif
 	 	  	       	  
	 	 	  	       	     			if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
	 	 	  	       	     			{
	  		 	 	   	       				initCopyDataBuff(port, HOUR_FREEZE_DATA);
	  		 	 	   	       				copyCtrl[port].tmpCpLink->flgOfAutoCopy = 3;
	 	 	  	       	     			}
	 	 	  	       	     			else    //抄实时数据
	 	 	  	       	     			{
				  		 	 	   	       	initCopyDataBuff(port, PRESENT_DATA);
				  		 	 	   	       	copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
				 	 	  	       	    }
	 	 	  	       	
	 	 	  	       	  		 #ifndef DKY_SUBMISSION
	 	 	  	       	   			}
	 	 	  	       	  		 #endif
	 	 	  	         			}
	 	 	  	         			else
	 	 	  	         			{
  		 	 	   	       			initCopyDataBuff(port, PRESENT_DATA);
  		 	 	   	       			copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
  		 	 	   	     			}
  		 	 	   	     			break;
  		 	 	   	   
  		 	 	   	   			case 4:   //小时冻结数据已抄完
  		 	 	   	     			initCopyDataBuff(port, PRESENT_DATA);
  		 	 	   	     			copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
  		 	 	   	   	 			break;     	  		 	 	   	   
  		 	 	     			}
  		 	 	     
  		 	 	    		 #endif
  		 	 	   			}
  		 	 	   			else
  		 	 	   			{
  		 	 	   	 			if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	   	 			{
  		 	 	   	   			printf("路由主导抄读:在载波链表中未找到路由请求的地址,回复模块否认\n");
  		 	 	   	 			}
  		 	 	   	 
  		 	 	   	 			gdw3762Framing(ACK_OR_NACK_3762, 2, NULL, &ifFound);
  		 	 	   	 			goto breakPoint;
  		 	 	   			}
  		 	 	   
                  copyCtrl[port].copyContinue = FALSE;
                }
                else
                {
                  copyCtrl[port].hasSuccessItem++;
                }
                 
                //2013-12-25,添加这个判断
                if (carrierSendToMeter==0)
                {
                  ret = COPY_COMPLETE_SAVE_DATA;
                 	 
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                 	 	printf("对于仅抄日冻结数据的07表冻结数据已抄到,直接认为是已保存\n");
                  }
                }
                else
                {
                  ret = meterFraming(PORT_POWER_CARRIER+port-4, NEW_COPY_ITEM);    //组帧发送,ly,2012-01-09
                }
              }
              else    //485端口
              {
             #endif
                 
               #ifdef LIGHTING
              	//线路控制器
               	if (LIGHTING_XL==copyCtrl[port].cpLinkHead->protocol)
               	{
                	ret = xlcFrame(port);
               	}
               	else	if (LIGHTING_DGM==copyCtrl[port].cpLinkHead->protocol)
                {
                  ret = ldgmFrame(port);
                }
                else if (LIGHTING_LS==copyCtrl[port].cpLinkHead->protocol)
                {
                  ret = lsFrame(port);
                }
                else if (LIGHTING_TH==copyCtrl[port].cpLinkHead->protocol)
                {
                  ret = thFrame(port);
                }
                else if (AC_SAMPLE==copyCtrl[port].cpLinkHead->protocol && copyCtrl[port].cpLinkHead->port>1)
               	{
                 	ret = xlcAcFrame(port);
               	}
								else 
								{
               #endif
                   
	                //2012-3-28,modify这里的处理
	               #ifdef RS485_1_USE_PORT_1
	                ret = meterFraming(port+1, NEW_COPY_ITEM);                //组帧发送

	                //2012-09-20,add
	                //ABB握手中
	                if (abbHandclasp[port]==1)
	                {
	                  copyCtrl[port].copyTimeOut = nextTime(sysTime,0,1);
	                }

	               #else
	               
	                if (port==0)
	                {
	                  ret = meterFraming(1, NEW_COPY_ITEM);                   //组帧发送
	                }
	                else
	                {
	                 #ifdef LM_SUPPORT_UT
	                  if (4==port)
	                  {
	                    ret = meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);
	                  }
	                  else
	                  {
	                 #endif
	                  
	                    ret = meterFraming(port, NEW_COPY_ITEM);                //组帧发送
	                  
	                 #ifdef LM_SUPPORT_UT
	                  }
	                 #endif
	                }
	              
	                //2012-09-20,add
	                //ABB握手中
	                if (abbHandclasp[port-1]==1)
	                {
	                  copyCtrl[port].copyTimeOut = nextTime(sysTime,0,1);
	                }
	              
	               #endif
                  
               #ifdef LIGHTING
                }
               #endif
                 
             #ifdef PLUG_IN_CARRIER_MODULE
              }
             #endif
            }
             
	          switch(ret)
	          {
	       	   	case COPY_COMPLETE_SAVE_DATA:
	       	     	if (debugInfo&METER_DEBUG)
	       	     	{
	       	       	printf("本表抄完且有有效数据已保存\n");
	       	     	}
	       	   
	       	     #ifdef PLUG_IN_CARRIER_MODULE
	       	     	if (port>=4)
	       	     	{
	       	       #ifdef LM_SUPPORT_UT
	       	       	if (0x55!=lmProtocol)
	       	       	{
	       	       #endif
	       	   	 
	       	   	     	if (copyCtrl[port].tmpCpLink!=NULL)
	       	   	     	{
    	       	   	   	//路由主导抄表
    	       	   	   	if (carrierFlagSet.routeLeadCopy==1)
    	       	   	   	{
    	       	   	     	switch(copyCtrl[port].tmpCpLink->flgOfAutoCopy)
    	       	   	     	{
    	       	   	       	case 1:
    	       	   	    	 		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 2;
    	       	   	    	 		break;

    	       	   	       	case 3:
    	       	   	    	 		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 4;
    	       	   	    	 		break;

    	       	   	       	case 5:
    	       	   	    	 		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 6;
    	       	   	    	 		break;
    	       	   	     	}
    	       	   	   	}
    	       	   	   	else    //集中器主导抄表
    	       	   	   	{
    	       	   	     	if (copyCtrl[port].ifCopyLastFreeze==0)
    	       	   	     	{
    	       	   	       	copyCtrl[port].tmpCpLink->thisRoundCopyed = TRUE;   //本轮抄表成功
    	       	   	    
    	       	   	       	if (debugInfo&METER_DEBUG)
    	       	   	       	{
    	       	   	         	printf("测量点%d本轮抄当前数据成功\n", copyCtrl[port].tmpCpLink->mp);
    	       	   	       	}	
    	       	   	     	}
    	       	   	     	else
    	       	   	     	{
    	       	   	       	if (debugInfo&METER_DEBUG)
    	       	   	       	{
    	       	   	         	if (copyCtrl[port].ifCopyLastFreeze==2)
    	       	   	         	{    	       	   	      
    	       	   	          	printf("测量点%d本轮抄日冻结数据成功,thisRoundCopyed=%d\n", copyCtrl[port].tmpCpLink->mp,copyCtrl[port].tmpCpLink->thisRoundCopyed);
    	       	   	         	}
    	       	   	      
    	       	   	         	if (copyCtrl[port].ifCopyLastFreeze==3)
    	       	   	         	{    	       	   	      
    	       	   	           	printf("测量点%d本轮抄整点数据成功,thisRoundCopyed=%d\n", copyCtrl[port].tmpCpLink->mp,copyCtrl[port].tmpCpLink->thisRoundCopyed);
    	       	   	         	}
    	       	   	       	}
    	       	   	    
    	       	   	       	//2012-3-29,去掉这个注释,便于统计抄表成功率
    	       	   	       	//#ifdef CQDL_CSM
    	       	   	       	if (copyCtrl[port].ifCopyLastFreeze==2)
    	       	   	       	{ 
    	       	             	copyCtrl[port].tmpCpLink->copySuccess = TRUE;   //抄表成功
    	       	   	       	}
    	       	   	       	//#endif
    	       	   	     	}
    	       	       	}
    	       	     	}
    	       	   
    	       	   #ifdef LM_SUPPORT_UT
    	       	   	}
    	       	   #endif
	       	     	}
	       	     #endif
	       	      break;
  
	       	    case COPY_COMPLETE_NOT_SAVE:
	       	 	 		if (debugInfo&METER_DEBUG)
	       	 	 		{
	       	       	printf("端口%d表%02x%02x%02x%02x%02x%02x抄完但无有效数据,flagofAutoCopy=%d\n", 
	       	              	port+1, 
	       	               	 copyCtrl[port].cpLinkHead->addr[5], 
	       	                  copyCtrl[port].cpLinkHead->addr[4], 
	       	                   copyCtrl[port].cpLinkHead->addr[3], 
	       	                    copyCtrl[port].cpLinkHead->addr[2], 
	       	                     copyCtrl[port].cpLinkHead->addr[1], 
	       	                      copyCtrl[port].cpLinkHead->addr[0], 
	       	                       copyCtrl[port].cpLinkHead->flgOfAutoCopy
	       	              );
	       	     	}
        	       
        	     #ifdef PLUG_IN_CARRIER_MODULE 
       	       	//路由主导抄表时这种情况视为抄表失败
        	     	if (carrierFlagSet.routeLeadCopy==1 && port>=4)
        	     	{
    	       	   	//copyCtrl[port].tmpCpLink->flgOfAutoCopy = 7;
	       	   	   	//ly,2012-01-10,改成switch判断
	       	   	   	switch(copyCtrl[port].tmpCpLink->flgOfAutoCopy)
	       	   	   	{
	       	   	     	case 1:
     	 	  	       		if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
     	 	  	       		{
     	 	  	       		 #ifndef DKY_SUBMISSION
     	 	  	       	 		//2012-12-25,为精减抄读数据项而增加if处理
     	 	  	       	 		//  如果大小类号都为0的话,日冻结采集成功,就不再采集其他数据项了,直接是"抄读失败"
     	 	  	       	 		if (copyCtrl[port].tmpCpLink->bigAndLittleType==0x00)
     	 	  	       	 		{
     	 	  	       	   		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 9;
                          memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);

     	 	  	       	   		if (debugInfo&PRINT_CARRIER_DEBUG)
     	 	  	       	   		{
     	 	  	       	     		printf("%02d-%02d-%02d %02d:%02d:%02d,对大小类号都为0的07表(%02x%02x%02x%02x%02x%02x),日冻结数据标识发送完毕但是未抄回数据,flagOfAutoCopy置为9\n", 
     	 	  	       	            	 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
     	 	  	       	     	        	copyCtrl[port].tmpCpLink->addr[5], copyCtrl[port].tmpCpLink->addr[4], copyCtrl[port].tmpCpLink->addr[3],
     	 	  	       	     	        	 copyCtrl[port].tmpCpLink->addr[2], copyCtrl[port].tmpCpLink->addr[1], copyCtrl[port].tmpCpLink->addr[0]
     	 	  	       	    	        );
     	 	  	       	   		}
     	 	  	       	 		}
     	 	  	       	 		else
     	 	  	       	 		{
     	 	  	       		 #endif
	     	 	  	       	  
	       	   	   	       	copyCtrl[port].tmpCpLink->flgOfAutoCopy = 2;
	     	 	  	       	
	     	 	  	    		 #ifndef DKY_SUBMISSION
	     	 	  	     			}
	     	 	  	    		 #endif
	     	 	  	   			}
	     	 	  	   			else
	     	 	  	   			{
 	  		 	 	   	 				copyCtrl[port].tmpCpLink->flgOfAutoCopy = 2;
 	  		 	 	   				}
	       	   	       	break;

	       	   	     	case 3:
	       	   	       	copyCtrl[port].tmpCpLink->flgOfAutoCopy = 4;
	       	   	       	break;

	       	   	     	case 5:
	       	   	       	copyCtrl[port].tmpCpLink->flgOfAutoCopy = 7;
	       	   	       	break;
	       	   	   
	       	   	     	case 8:    //日冻结已抄完,没抄到数据,2013-12-25
	       	   	       	copyCtrl[port].tmpCpLink->flgOfAutoCopy = 9;
	       	 	       		if (debugInfo&METER_DEBUG)
	       	 	       		{
	       	             	printf("%02d-%02d-%02d %02d:%02d:%02d,端口%d本表(%02x%02x%02x%02x%02x%02x)日冻结数据标识已发送完但无有效数据,flagofAutoCopy置为9\n", 
	       	                     sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
	       	                      port+1, 
	       	                       copyCtrl[port].tmpCpLink->addr[5], copyCtrl[port].tmpCpLink->addr[4], copyCtrl[port].tmpCpLink->addr[3],
	       	                        copyCtrl[port].tmpCpLink->addr[2], copyCtrl[port].tmpCpLink->addr[1], copyCtrl[port].tmpCpLink->addr[0]
	       	                   );
	       	           	}
	       	   	   	   	break;
	       	   	   	}
    	         	}
    	         #endif
        	       
    	       	 	break;
    	        
	           	case CHANGE_METER_RATE:
               #ifdef RS485_1_USE_PORT_1
                buf[0] = port+1;
               #else
                if (port==0)
                {
                  buf[0] = 0x01;
                }
                else
                {
                  buf[0] = port;
                }
               #endif
                buf[1] = DATA_CTL_W_IEC1107;      //端口速率
                usleep(200000);
                sendXmegaInTimeFrame(COPY_PORT_RATE_SET, buf, 2);
      
	              if (debugInfo&METER_DEBUG)
	              {
	               	printf("设置端口%d速率\n", port);
	             	}

	             	copyCtrl[port].copyTimeOut = nextTime(sysTime,0,15);
	             	copyCtrl[port].retry = NUM_COPY_RETRY;
	         	 		break;
	         	 
	       	   	default:
	  	         	break;
    	  	 	}
    	  	   
	  	     	//本表抄完,删除链表中的本节点,继续抄下一块表
	  	     	if (ret==COPY_COMPLETE_NOT_SAVE || ret==COPY_COMPLETE_SAVE_DATA)
	  	     	{
    	  	     
  stopThisMeterPresent:
             	if (
			   	   			(port!=4 &&  copyCtrl[port].ifCopyLastFreeze==0 
               	    #ifdef LIGHTING 
               	     && LIGHTING_XL!=copyCtrl[port].cpLinkHead->protocol      //线路控制器
               	      && LIGHTING_DGM!=copyCtrl[port].cpLinkHead->protocol    //报警控制器
               	       && LIGHTING_LS!=copyCtrl[port].cpLinkHead->protocol    //光照度传感器
               	    #endif
               	  )
               	   #ifdef PLUG_IN_CARRIER_MODULE
               	    || (port>=4 &&  copyCtrl[port].ifCopyLastFreeze==0 && carrierFlagSet.routeLeadCopy==0)
               	    #ifndef LM_SUPPORT_UT
               	     || (port>=4 &&  copyCtrl[port].tmpCpLink->flgOfAutoCopy==6 && carrierFlagSet.routeLeadCopy==1)
               	    #endif
               	   #endif
               	)
             	{
       	       	//抄表工况
     	         	if (port>=4)
     	         	{
                 #ifdef LM_SUPPORT_UT
                 	if (0x55==lmProtocol)
                 	{
                 	  searchMpStatis(sysTime, &meterStatisBearonTime, copyCtrl[port].cpLinkHead->mp, 2);
                 	}
                 	else
                 	{
                 #endif
                
                   	searchMpStatis(sysTime, &meterStatisBearonTime, copyCtrl[port].tmpCpLink->mp, 2);
                   
                 #ifdef LM_SUPPORT_UT
                  }
                 #endif
     	         	}
     	         	else
     	         	{
                  searchMpStatis(sysTime, &meterStatisBearonTime, copyCtrl[port].cpLinkHead->mp, 2);
                }

               	//1)该测量点抄表成功次数
               	if ((port!=4 && copyCtrl[port].numOfMeterAbort != 0xfe && ret==COPY_COMPLETE_SAVE_DATA)  //485端口如果不是抄表故障及接收到抄表数据
               	 	   || (port>=4 && ret==COPY_COMPLETE_SAVE_DATA)                                       //载波端口如果接收到抄表数据
               	 	 )
               	{
               	  if (meterStatisBearonTime.copySuccessTimes==0xeeee)
               	  {
               		 	meterStatisBearonTime.copySuccessTimes = 1;
               	  }
               	  else
               	  {
               		 	meterStatisBearonTime.copySuccessTimes++;
               	  }
                     
                  //该测量点最近一次抄表成功时间记录
                  meterStatisBearonTime.lastCopySuccessTime[0] = sysTime.minute/10<<4 | sysTime.minute%10;
                  meterStatisBearonTime.lastCopySuccessTime[1] = sysTime.hour/10<<4 | sysTime.hour%10;
                   	
                  if (port<4)
                  {
                   	//如果曾经发生过485抄表失败,则记录485抄表恢复
                   	if (meterStatisExtranTime.mixed&0x2)
                   	{
                   	  //记录终端485抄表失败事件
                      if (copy485Failure(copyCtrl[port].cpLinkHead->mp, 2, port+1)==TRUE)
                      {
                        meterStatisExtranTime.mixed &= 0xfd;
                      }
                   	}
                  }
               	}
       	          
     	         #ifdef PLUG_IN_CARRIER_MODULE 
     	          if (port>=4)   //载波端口
     	          {
                  if ((1==carrierFlagSet.routeLeadCopy && carrierSendToMeter!=0) || 0==carrierFlagSet.routeLeadCopy)
                  {
                   #ifdef LM_SUPPORT_UT
                    if (0x55==lmProtocol)
                    {
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3); //单相表与时间无关量
                    }
                    else
                    {
                   #endif
                    
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].tmpCpLink->mp, 3); //单相表与时间无关量
                       
                   #ifdef LM_SUPPORT_UT
                    }
                   #endif

                    //查询测量点存储信息
                    if (carrierFlagSet.routeLeadCopy==1)  //ly,2012-01-09
                    {
  		               	queryMeterStoreInfo(copyCtrl[port].tmpCpLink->mp, meterInfo);
                    }
                    else
                    {
  		               	queryMeterStoreInfo(copyCtrl[port].cpLinkHead->mp, meterInfo);
  		             	}

                    //meterInfo,第0字节 - 电表类型(1-单相智能表,2-单相本地费控表,3-单相远程费控表,
                    //           4-三相智能表,5-三相本地费控表,6-三相远程费控表,7-485三相表(数据抄读最全),8-重点用户
  		             	switch(meterInfo[0])
  		             	{
	             	   		case 1:  //单相智能表
	             	   		case 2:  //单相本地费控表
	             	   		case 3:  //单相远程费控表
                        memcpy(tmpReadBuff, &copyCtrl[port].dataBuff[DATE_AND_WEEK_S], 7);
  		             	 		break;

	             	   		case 4:  //三相智能表
	             	   		case 5:  //三相本地费控表
	             	   		case 6:  //三相远程费控表
                        memcpy(tmpReadBuff, &copyCtrl[port].dataBuff[DATE_AND_WEEK_T], 7);
  		             	 		break;

	             	   		case 7:  //三相表
                        memcpy(tmpReadBuff, &copyCtrl[port].dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+DATE_AND_WEEK], 7);
             	 	     		break;
  		             	}
  		             
                    //载波端口电能表时间超差判断
                   #ifdef LM_SUPPORT_UT
                    if (0x55==lmProtocol)
                    {
                      if (meterTimeOverEvent(copyCtrl[port].cpLinkHead->mp, tmpReadBuff, &meterStatisExtranTimeS.mixed, &meterStatisBearonTime))
                      {
                        //存储测量点统计数据(与时间无关量)
                        saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                      }
                    }
                    else
                    {
                   #endif 
                       
                     	if (meterTimeOverEvent(copyCtrl[port].tmpCpLink->mp, tmpReadBuff, &meterStatisExtranTimeS.mixed, &meterStatisBearonTime))
                     	{
                       	//存储测量点统计数据(与时间无关量)
                       	saveMeterData(copyCtrl[port].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                     	}
                       
                   #ifdef LM_SUPPORT_UT
                   	}
                   #endif
                  }
     	         	}
     	         	else
     	         	{
     	         #endif
                 	//三相表与时间无关量
                 	searchMpStatis(sysTime, &meterStatisExtranTime, copyCtrl[port].cpLinkHead->mp, 1);

                 	//如果测量点本次抄表的电能表时间及日期有数据,分析电能表时间超差时间记录
                 	memcpy(tmpReadBuff,copyCtrl[port].dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD,LENGTH_OF_PARA_RECORD);                 
                 
                 	//电能表时间超差判断
                 	if (meterTimeOverEvent(copyCtrl[port].cpLinkHead->mp, &tmpReadBuff[DATE_AND_WEEK], &meterStatisExtranTime.mixed, &meterStatisBearonTime))
                 	{
                   	//存储测量点统计数据(与时间无关量)
                   	saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTime, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
                 	}
                   
               #ifdef PLUG_IN_CARRIER_MODULE
                }
               #endif

      	        //存储测量点统计数据(抄表工况)
      	       #ifdef LM_SUPPORT_UT
      	        if (port>=4 && 0x55!=lmProtocol)    //载波/无线端口
      	       #else
      	        if (port>=4)    //载波/无线端口
      	       #endif
      	        {
                  saveMeterData(copyCtrl[port].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisBearonTime, STATIS_DATA, 0xfe,sizeof(METER_STATIS_BEARON_TIME));

     	           	if (debugInfo&METER_DEBUG)
     	           	{
     	             	printf("测量点%d抄表成功次数:%d\n",copyCtrl[port].tmpCpLink->mp,meterStatisBearonTime.copySuccessTimes);
     	           	}
     	         	}
     	         	else            //485端口
     	         	{
                  saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisBearonTime, STATIS_DATA, 0xfe,sizeof(METER_STATIS_BEARON_TIME));
                   
                  //2012-3-29,add
                  copyCtrl[port].cpLinkHead->copySuccess = TRUE;

     	           	if (debugInfo&METER_DEBUG)
     	           	{
     	             	printf("测量点%d抄表成功次数:%d\n",copyCtrl[port].cpLinkHead->mp,meterStatisBearonTime.copySuccessTimes);
     	           	}
     	         	}
              }
     	  	     
 	  	      #ifdef PLUG_IN_CARRIER_MODULE
 	  	       #ifdef LM_SUPPORT_UT
 	  	        if (port>=4 && 0x55!=lmProtocol)     //载波端口
 	  	       #else
 	  	        if (port>=4)     //载波端口
 	  	       #endif
 	  	       	{
     	  	     	//集中器主导抄表
     	  	     	if (carrierFlagSet.routeLeadCopy==0)
     	  	     	{
       	  	      copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
    	     	       
	     	       		if (copyCtrl[port].ifCopyLastFreeze==2 || copyCtrl[port].ifCopyLastFreeze==3)
	     	       		{
	     	         		while(copyCtrl[port].tmpCpLink!=NULL)
	     	         		{
	  	     	 	   			//07规约表且无日冻结数据则开始采集
	     	 	       			if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
	     	 	       			{
	     	 	  	     			if (copyCtrl[port].ifCopyLastFreeze==2)
	     	 	  	     			{
         	               #ifndef DKY_SUBMISSION
     	 	  	       	   		//2012-09-28,为适应象甘肃敦煌,兰州榆中的没有日冻结的07表而增加if处理,同时取消类号为0x55的设置,因为这会在主站端操作特别麻烦
     	 	  	       	   		//  修改为:设置居民用户表抄读模式为0x55(仅实时数据)和0xaa(仅实时总示值)来控制抄读项
  	       	              if (denizenDataType!=0x55 && denizenDataType!=0xaa)
  	       	              {
  	       	             #endif

     	 	  	             		if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
     	 	  	             		{
     	 	  	               		break;
     	 	  	             		}
     	 	  	          
     	 	  	          	 #ifndef DKY_SUBMISSION
     	 	  	           		}
     	 	  	          	 #endif
     	 	  	         		}    	     	 	  	       
     	 	  	         		else
     	 	  	         		{
         	               #ifndef DKY_SUBMISSION
     	 	  	       	   		//2012-09-28,为适应象甘肃敦煌,兰州榆中的没有日冻结的07表而增加if处理,同时取消类号为0x55的设置,因为这会在主站端操作特别麻烦
     	 	  	       	     	//  修改为:设置居民用户表抄读模式为0x55(仅实时数据)和0xaa(仅实时总示值)来控制抄读项
  	       	              if (denizenDataType!=0x55 && denizenDataType!=0xaa 

 	 	  	       	          		//2012-08-31,为精减抄读数据项而增加if处理
 	 	  	       	           		//  如果大小类号都为0的话,日冻结采集成功,就不再采集其他数据项了
 	 	  	       	           		//  更改以后对严格执行标准的07表的默认处理方式为只采集日冻结数据
     	             	       			&& copyCtrl[port].tmpCpLink->bigAndLittleType!=0x00
         	             	  		)
         	               	{
           	             #endif
    	     	 	  	       	   
     	 	  	       	     		if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
     	 	  	       	     		{
     	 	  	       	 	   			break;
     	 	  	       	     		}
     	 	  	       	   
     	 	  	       	  	 #ifndef DKY_SUBMISSION
     	 	  	       	   		}
     	 	  	       	  	 #endif
     	 	  	         		}
     	 	           		}
     	 	           		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
    	     	     		}
    	     	         
    	     	     		//抄完上一日冻结数据立即开始抄实时数据
    	     	     		if (copyCtrl[port].tmpCpLink==NULL)
    	     	     		{
                      if (copyCtrl[port].ifCopyLastFreeze == 3)
                      {
  	     	         	 		copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
  	     	         	 		while(copyCtrl[port].tmpCpLink!=NULL)
  	     	         			{
  	     	         	  	 	copyCtrl[port].tmpCpLink->thisRoundCopyed = FALSE;
  	     	         	   		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
  	     	         	 		}

  	     	             	if (debugInfo&METER_DEBUG)
  	     	             	{ 
  	     	               	printf("抄完整点冻结数据立即开始上一日冻结数据\n");
  	     	             	}
  	     	           
  	     	             	copyCtrl[port].round = 0;
                        copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                        copyCtrl[port].ifCopyLastFreeze = 0;
                        while(copyCtrl[port].tmpCpLink!=NULL)
                        {
              	          if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
                 	       	{
              	            //检测是否有当日 日冻结数据
              	            if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
              	            {
              	              copyCtrl[port].ifCopyLastFreeze = 2;
              	              break;
              	            }
                     	   	}
      	     	         	   
      	     	           	copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                     	 	}
                      }
                      else
                      {
      	     	         	if (debugInfo&METER_DEBUG)
      	     	         	{
      	     	           	printf("抄完上一日冻结数据立即开始抄实时数据\n");
      	     	         	}
                          
                        copyCtrl[port].round = 0;
  	     	         	 		copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
  	     	         	 		while(copyCtrl[port].tmpCpLink!=NULL)
  	     	         	 		{
  	     	         	   		copyCtrl[port].tmpCpLink->thisRoundCopyed = FALSE;
  	     	         	   		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
  	     	         	 		}
  	     	         	  
  	     	         	 		copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
  
                        //如果采集实时数据,移位到上一次未采集完成的测量点开始采集
                       	if (copyCtrl[port].backMp>0)
                       	{
                   	 	   	while(copyCtrl[port].tmpCpLink!=NULL)
                   	 	   	{
                   	 	 	 		if (copyCtrl[port].tmpCpLink->mp==copyCtrl[port].backMp)
                   	 	 	 		{
                   	 	 	   		if (debugInfo&METER_DEBUG)
                   	 	 	   		{
                   	 	 	 	 			printf("(正在抄表)移位到上一次未采集完成的测量点%d\n", copyCtrl[port].backMp);
                   	 	 	   		}
                   	 	 	   		break;
                   	 	 	 		}
                   	 	 	 
 	  	     	 	         			copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                   	 	   	}
                       	}
      	     	         	  
      	     	         	copyCtrl[port].ifCopyLastFreeze = 0;
                        initCopyItem(port, copyCtrl[port].ifCopyLastFreeze);
                      }
    	     	     		}
    	     	   		}
    	     	   		else
    	     	   		{
       	  	     	 	while(copyCtrl[port].tmpCpLink!=NULL)
       	  	     	 	{
   	  	     	 	   		if (copyCtrl[port].tmpCpLink->thisRoundCopyed==FALSE)
   	  	     	 	   		{
   	  	     	 	  	 		break;
   	  	     	 	   		}
   	  	     	 	   
   	  	     	 	   		if (debugInfo&PRINT_CARRIER_DEBUG)
   	  	     	 	   		{
   	  	     	 	   	 		printf("某块表抄表成功后判断应该抄哪块表:测量点%d本轮抄表成功,移向下一块表\n",copyCtrl[port].tmpCpLink->mp);
   	  	     	 	   		}
   	  	     	 	   
   	  	     	 	   		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
       	  	     	 	}
       	  	      } 
       	  	    }
       	  	     	 
       	  	    tmpNode = copyCtrl[port].tmpCpLink;
     	  	   	}
     	  	   	else    //485端口
     	  	   	{
     	  	   #endif
     	  	    
     	  	     	tmpNode = copyCtrl[port].cpLinkHead;
    	  	     	copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
    	  	   	 	free(tmpNode);
    	  	   	 	//free(copyCtrl[port].dataBuff);
    	  	   	   
    	  	   		if (debugInfo&METER_DEBUG)
    	  	   	 	{
    	  	   	   	printf("释放前一块表信息\n");
    	  	   	 	}

  	     	     	//如果采集上一次日冻结数据,则要移位到07规约表
  	     	     	if (copyCtrl[port].ifCopyLastFreeze==2)
  	     	     	{
  	     	       	while(copyCtrl[port].cpLinkHead!=NULL)
  	     	       	{
  	     	 	     		//07规约表且无日冻结数据则开始采集
  	     	 	     		if (copyCtrl[port].cpLinkHead->protocol==DLT_645_2007)
  	     	 	     		{
     	 	  	      	 	if (checkLastDayData(copyCtrl[port].cpLinkHead->mp, 0, sysTime, 1)==FALSE)
     	 	  	       		{
  	     	 	  	     		break;
  	     	 	  	   		}
  	     	 	     		}
  	     	 	         
	  	           		tmpNode = copyCtrl[port].cpLinkHead;
  	             		copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
  	   	         		free(tmpNode);
  	     	     		}
  	     	        
  	     	     		if (debugInfo&METER_DEBUG)
  	     	     		{
  	   	        		printf("采集上一次日冻结数据移位到07表\n");
  	   	       		}
  	     	   		}
    	  	   	   	
	  	   	   		if (copyCtrl[port].cpLinkHead!=NULL)
	  	   	   		{
	  	   	     		if (copyCtrl[port].cpLinkHead->protocol==AC_SAMPLE)
	  	   	     		{
        	         #ifdef LIGHTING 
        	         	if (1==copyCtrl[port].cpLinkHead->port)
        	         	{
        	         #endif
            	      
            	       	//有交采模块
            	       	if (ifHasAcModule==TRUE)
            	       	{
      	         	     	//如果是抄上月数据,转换上月数据
      	         	     	if (copyCtrl[port].ifCopyLastFreeze==0x1)
      	         	     	{
           	         	   	copyAcValue(port, 2, sysTime);
  
                          if (debugInfo&METER_DEBUG)
                          {
                            printf("CopyMeter:抄表中,转换交采上月数据并保存\n");
                          }
                        }
                          
      	         	      //转换实时数据
      	         	      if (copyCtrl[port].ifCopyLastFreeze==0x0)
      	         	      {
           	         	   	copyAcValue(port, 1, timeHexToBcd(copyCtrl[port].currentCopyTime));
                                           
                          if (debugInfo&METER_DEBUG)
                          {
                            printf("CopyMeter:抄表中,转换交采实时数据并保存\n");
                          }
                        }
           	          }
           	  	     
           	  	      tmpNode = copyCtrl[port].cpLinkHead;
          	  	      copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
          	  	   	  free(tmpNode);
          	  	   	   	
          	  	   	  if (debugInfo&METER_DEBUG)
          	  	   	  {
          	  	   	    printf("释放前一块表(交采)信息\n");
          	  	   	  }
          	  	   	   
          	  	   #ifdef LIGHTING
          	  	   	}
          	  	   #endif
    	  	   	   	}
    	  	   	 	}
    	  	   	   
    	  	   	 	tmpNode = copyCtrl[port].cpLinkHead;
    	  	   	
    	  	   #ifdef PLUG_IN_CARRIER_MODULE
    	  	   	}
    	  	   #endif
    	  	   	
    	  	   	if (tmpNode==NULL)    //如果本端口表已抄完
    	  	   	{
    	         	if (debugInfo&METER_DEBUG)
    	         	{
    	           	printf("本端口已抄完\n");
    	         	}
    	        	 
    	       	#ifdef PLUG_IN_CARRIER_MODULE
    	         #ifdef LM_SUPPORT_UT
    	          if (port>=4 && 0x55!=lmProtocol)
    	         #else
    	         	if (port>=4)
    	         #endif
	        	 		{
	        	   		//如有测量点未抄到,抄3轮
        	 	   		copyCtrl[port].round++;
        	 	   		if (copyCtrl[port].round>3)
        	 	   		{
        	 	 	 			copyCtrl[port].round = 0;
        	 	 	  
        	 	 	 			tmpNode = copyCtrl[port].cpLinkHead;
        	 	 	 			tmpData = 0;
        	 	 	 			while (tmpNode!=NULL)
        	 	 	 			{
        	 	 	   			if (tmpNode->thisRoundCopyed==FALSE)
        	 	 	   			{
        	 	 	  				tmpData++;
        	 	 	   			}
        	 	 	  	 
        	 	 	   			tmpNode = tmpNode->next;
        	 	 	 			}
	        	 	 	
        	 	 	 			//如果有没有抄到的表,电科院模块启动搜表
        	 	 	 			if (tmpData>0 && (carrierModuleType==CEPRI_CARRIER || localModuleType==CEPRI_CARRIER_3_CHIP))
        	 	 	 			{
        	 	 	   			//最长搜表时间
        	 	 	   			carrierFlagSet.searchMeter = 1;
        	 	 	   			carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0);

        	 	 	   			if (debugInfo&PRINT_CARRIER_DEBUG)
        	 	 	   			{
        	 	 	  	 			printf("有没抄到的表,开始搜表,截止时间:%d-%d-%d %d:%d:%d\n",carrierFlagSet.foundStudyTime.year,carrierFlagSet.foundStudyTime.month,
        	 	 	  	         			carrierFlagSet.foundStudyTime.day, carrierFlagSet.foundStudyTime.hour,
        	 	 	  	          			carrierFlagSet.foundStudyTime.minute, carrierFlagSet.foundStudyTime.day);
        	 	 	   			}
        	 	 	 			}
        	 	 	  
        	 	 	 			copyCtrl[port].backMp = 0;
        	 	 	 			copyCtrl[port].meterCopying = FALSE;
        	 	   		}
        	 	   		else
        	 	   		{
	        	 	 			copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
 	  	     	     		while(copyCtrl[port].tmpCpLink!=NULL)
 	  	     	     		{
  	     	 	       		if (copyCtrl[port].tmpCpLink->thisRoundCopyed==FALSE)
  	     	 	       		{
  	     	 	  	     		break;
  	     	 	       		}
  	     	 	      
  	     	 	       		if (debugInfo&PRINT_CARRIER_DEBUG)
  	     	 	       		{
  	     	 	   	     		printf("抄完一轮后判断哪些测量点未抄到:测量点%d本轮抄表成功,移向下一块表\n",copyCtrl[port].tmpCpLink->mp);
  	     	 	       		}

  	     	 	       		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
 	  	     	     		}
     	  	     	      
  	     	         	//全部测量点都抄到,可以结束本次抄表
  	     	         	if (copyCtrl[port].tmpCpLink==NULL)
  	     	         	{
    	 	 	       			copyCtrl[port].round = 0;
    	 	 	       			copyCtrl[port].meterCopying = FALSE;
  	     	         	}
  	     	         	else
  	     	         	{
	   	               	initCopyDataBuff(port, copyCtrl[port].ifCopyLastFreeze);
	   	      
	   	               	if (debugInfo&PRINT_CARRIER_DEBUG)
	   	               	{
	   	                 	printf("开始第%d轮抄表\n",copyCtrl[port].round);
	   	               	}
	   	          
	   	               	goto breakPoint;
  	     	         	}
		    	   			}
		    	 			}
		    	 			else
		    	 			{
    	         #endif
    	        	
    	           	copyCtrl[port].meterCopying = FALSE;
									 
				  		   #ifdef LIGHTING
				   				if (TRUE==copyCtrl[port].thisRoundFailure)
				   				{
					 					copyCtrl[port].portFailureRounds++;
				     				if (debugInfo&METER_DEBUG)
				     				{
				       				printf("连续抄读失败次数=%d\n",copyCtrl[port].portFailureRounds);
				     				}
					 					if (copyCtrl[port].portFailureRounds>=3)
					 					{
					   					//记录485抄表故障发生
					   					happenRecovery485(1);
					 					}
				   				}
				   				else
				   				{
					 					if (copyCtrl[port].portFailureRounds>0)
					 					{
					   					copyCtrl[port].portFailureRounds = 0;
					 					}
					 
					 					//记录485抄表故障恢复
					 					happenRecovery485(0);
				   				}
				 
				  			 #endif
    	        	
	        	   #ifdef PLUG_IN_CARRIER_MODULE 
	        	 		}
	        	   #endif
    	  	   	   	   
	  	   	    	if (copyCtrl[port].copyDataType==LAST_MONTH_DATA)
	  	   	    	{
	  	   	       	if (debugInfo&METER_DEBUG)
	  	   	       	{
	  	   	         	printf("端口%d上月数据抄表完成,开始抄实时数据\n",port+1);
	  	   	       	}
	  	   	     
	  	   	       	copyCtrl[port].ifCopyLastFreeze = 0;

                  copyCtrl[port].cpLinkHead = initPortMeterLink(port);
                  if (copyCtrl[port].cpLinkHead==NULL)
                  {
             	     	copyCtrl[port].meterCopying = FALSE;   //停止抄表

                    copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);
               	  
             	     	if (debugInfo&METER_DEBUG)
             	     	{
               	      printf("抄完上月数据后端口:%d无表可抄\n",port+1);
                   	}
                  }
                  else
                  {
                    initCopyItem(port, copyCtrl[port].ifCopyLastFreeze);
                  }
                       
                  goto breakPoint;
    	  	   	 	}
    	  	   	 	else
    	  	   	 	{
    	  	   	   #ifdef LM_SUPPORT_UT
    	  	   	   	if (copyCtrl[port].copyDataType==LAST_DAY_DATA && port<=4 && 0x55==lmProtocol)
    	  	   	   #else
    	  	   	    if (copyCtrl[port].copyDataType==LAST_DAY_DATA && port<4)
    	  	   	   #endif
    	  	   	   	{
      	  	   	    if (debugInfo&METER_DEBUG)
      	  	   	    {
      	  	   	      printf("端口%d上一次日冻结数据抄表完成,开始抄实时数据\n",port+1);
      	  	   	    }
      	  	   	     
      	  	   	    copyCtrl[port].ifCopyLastFreeze = 0;

                   #ifdef LM_SUPPORT_UT
                    if (4==port)
                    {
                   	  copyCtrl[port].cpLinkHead = initPortMeterLink(30);
                    }
                    else
                    {
                   #endif
                     
                      copyCtrl[port].cpLinkHead = initPortMeterLink(port);
                     
                   #ifdef LM_SUPPORT_UT
                    }
                   #endif
                     
                    //ly,2011-09-13,add
      	  	   	    if (copyCtrl[port].cpLinkHead->protocol==AC_SAMPLE)
      	  	   	    {
              	      //有交采模块
              	      if (ifHasAcModule==TRUE)
              	      {
      	         	      copyAcValue(port, 1, timeHexToBcd(sysTime));  //因为后面要初始化本次抄表的时间为sysTime
                                         
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("CopyMeter:抄表中(抄完07上一次日冻结数据后),转换交采实时数据并保存\n");
                        }
           	          }
           	  	     
           	  	      tmpNode = copyCtrl[port].cpLinkHead;
          	  	      copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
          	  	   	  free(tmpNode);
          	  	   	   	
          	  	   	  if (debugInfo&METER_DEBUG)
          	  	   	  {
          	  	   	    printf("抄表中(抄完07上一次日冻结数据后),释放前一块表(交采)信息\n");
          	  	   	  }
      	  	   	    }
                     
                    if (copyCtrl[port].cpLinkHead==NULL)
                    {
               	      copyCtrl[port].meterCopying = FALSE;   //停止抄表
  
                      copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);
                 	  
               	      if (debugInfo&METER_DEBUG)
               	      {
                 	     	printf("抄完上一次日冻结数据后端口:%d无表可抄\n",port+1);
                      }
                    }
                    else
                    {
                      initCopyItem(port, copyCtrl[port].ifCopyLastFreeze);
                    }
                         
                    goto breakPoint;
    	  	   	   	}
    	  	   	   	else
    	  	   	   	{
      	  	   	    if (menuInLayer==0)
      	  	   	    {
      	  	   	      defaultMenu();
      	  	   	    }
                     
                  #ifdef PLUG_IN_CARRIER_MODULE
                   #ifndef MENU_FOR_CQ_CANON
                    if (port==4)
                    {
					 	 	        if (carrierModuleType==SR_WIRELESS || carrierModuleType==FC_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==SC_WIRELESS)
					 	 	        {
			               		showInfo("无线端口抄表完毕!");
					 	 	        }
					 	 	        else
					 	 	       	{
			               		showInfo("本地通信口抄表完毕!");
						          }
						        }
						       #endif
                  #endif
                     
           	 	     	//2012-07-31,本部分处理移位到开始本端口抄表时
           	 	     	//#ifdef PULSE_GATHER
           	 	     	//  for(pulsei=0;pulsei<NUM_OF_SWITCH_PULSE;pulsei++)
           	 	     	//  {
           	 	     	//  	  if (pulse[pulsei].ifPlugIn == TRUE)
           	 	     	//  	  {
                    //       memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
                    //       memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
                    //       memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);
  
                    //       //转换
                    //       covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
                    //  		 	
                    //  		 	//保存示值
                    //  		 	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                    //                     visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
                    //  	   
                    //  		 	//保存参数
                    //  		 	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                    //                     paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
                    //
                    //  	    //保存需量
                    //  		 	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                    //                     reqBuff, PRESENT_DATA, REQ_REQTIME_DATA,LENGTH_OF_REQ_RECORD);
           	 	     	//  	  }
           	 	     	//  }
           	 	     	//#endif
       	 	       
      	  	   	    if (debugInfo&METER_DEBUG)
      	  	   	    {
      	  	   	      if (port==4)
      	  	   	      {
      	  	   	        printf("端口31抄表结束\n");
      	  	   	      }
      	  	   	      else
      	  	   	      {
      	  	   	        printf("端口%d抄表结束\n",port+1);
      	  	   	      }
      	  	   	    }
    	    		 	 	   
					 	 	    #ifdef PLUG_IN_CARRIER_MODULE
					 	 	     #ifdef LM_SUPPORT_UT
					 	 	     	if (0x55!=lmProtocol)
					 	 	     	{
					 	 	     #endif
					 	 	       	if (port==4)
					 	 	       	{
					 	 	         	carrierFlagSet.readStatus = 0;
					 	 	     
					 	 	         	if(debugInfo&PRINT_CARRIER_DEBUG)
					 	 	         	{
					 	 	           	if (carrierModuleType==FC_WIRELESS)
					 	 	           	{
					 	 	             	printf("友迅达复位读取状态标志\n");
					 	 	           	}
					 	 	         	}	
					 	 	       	}
					 	 	     
					 	 	     #ifdef LM_SUPPORT_UT
					 	 	     	}
					 	 	     #endif
					 	 	    #endif

      	  	 	      //上次抄表时间(BCD数)
                    copyCtrl[port].lastCopyTime.second = copyCtrl[port].currentCopyTime.second/10<<4 | copyCtrl[port].currentCopyTime.second%10;;
                    copyCtrl[port].lastCopyTime.minute = copyCtrl[port].currentCopyTime.minute/10<<4 | copyCtrl[port].currentCopyTime.minute%10;;
                    copyCtrl[port].lastCopyTime.hour   = copyCtrl[port].currentCopyTime.hour/10<<4 | copyCtrl[port].currentCopyTime.hour%10;;
                    copyCtrl[port].lastCopyTime.day    = copyCtrl[port].currentCopyTime.day/10<<4 | copyCtrl[port].currentCopyTime.day%10;;
                    copyCtrl[port].lastCopyTime.month  = copyCtrl[port].currentCopyTime.month/10<<4 | copyCtrl[port].currentCopyTime.month%10;;
                    copyCtrl[port].lastCopyTime.year   = copyCtrl[port].currentCopyTime.year/10<<4 | copyCtrl[port].currentCopyTime.year%10;;
     	 	             
     	 	             
   	 	              //本端口实时结算置位
	 	 	            #ifdef PLUG_IN_CARRIER_MODULE
	 	 	             	if (ret==COPY_COMPLETE_SAVE_DATA && port<4)
	 	 	            #else
	 	 	             	if (ret==COPY_COMPLETE_SAVE_DATA)
	 	 	            #endif
	 	 	             	{
	 	 	               	if (debugInfo&METER_DEBUG)
	 	 	               	{
	 	 	                 	printf("copyMeter:本端口%d实时结算准备置位, ifRealBalance=%d,ret=%d\n", port+1, copyCtrl[port].ifRealBalance, ret);
	 	 	               	}  

	 	 	               	//ly,2012-04-26取消这个判断
	 	 	               	//if (copyCtrl[port].ifRealBalance==0)
	 	 	               	//{
	 	 	               	if (debugInfo&METER_DEBUG)
	 	 	               	{
	 	 	                 	printf("copyMeter:本端口%d实时结算置位\n",port+1);
	 	 	               	}
	 	 	                 
	 	 	               	copyCtrl[port].ifRealBalance = 1;
	 	 	               
	 	 	               	//}
	 	 	             	}
	 	 	           	}
                }
    	  	   	}
    	  	   	else
    	  	   	{
    	  	    #ifdef PLUG_IN_CARRIER_MODULE
    	  	   	 #ifdef LM_SUPPORT_UT
    	  	   	 	if (port>=4 && 0x55!=lmProtocol)
    	  	   	#else
    	  	   	 	if (port>=4)
    	  	   	#endif
    	  	   	 	{
    	  	   	   	if (carrierFlagSet.routeLeadCopy==1)    //路由主导抄表
    	  	   	   	{
	  	   	   	 	 		//如果实时数据都抄完了的话,就告诉路由抄读成功
	  	   	   	 	 		switch(copyCtrl[port].tmpCpLink->flgOfAutoCopy)
	  	   	   	 	 		{
	  	   	   	 	   		case 6:    //实时数据已抄完
	  	   	   	 	   		case 8:    //2012-08-28,为了有日冻结数据而直接回复抄表成功而加的这个=8的值
  	       	   	        copyCtrl[port].tmpCpLink->thisRoundCopyed = TRUE;   //本轮抄表成功
                        copyCtrl[port].tmpCpLink->flgOfAutoCopy = 0;
                     
                        //2012-3-29,甘肃渭源发现的集中器路由主导抄读时统计抄表成功块数不正确,
                        //          原因是这里少加了这一句
                        copyCtrl[port].tmpCpLink->copySuccess = TRUE;
  	       	   	     
  	       	   	        if (debugInfo&METER_DEBUG)
  	       	   	        {
                          printf("测量点%d本轮抄表成功\n", copyCtrl[port].tmpCpLink->mp);
                        }

	  	       	   	     	//如果有采集器地址,要判断是不是本采集器下还有其它485表
	  	       	   	     	ifFound = 0;
	  	       	   	     	if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE)
	  	       	   	     	{
	  	       	   	       	tmpNode = copyCtrl[port].tmpCpLink;
	  	       	   	       	while(tmpNode!=NULL)
	  	       	   	       	{
	  	       	   	         	if ((compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, tmpNode->collectorAddr, 0)==TRUE)
	  	       	   	         	    && (tmpNode->thisRoundCopyed==FALSE)
	  	       	   	          	 )
	  	       	   	          {
	  	       	   	           	ifFound = 1;
	  	       	   	           	break;
	  	       	   	         	}
	  	       	   	          
	  	       	   	          tmpNode = tmpNode->next;
	  	       	   	       	}
	  	       	   	     	}
      	       	   	     
      	       	   	    if (ifFound==0)
      	       	   	    {
    	  	   	   	 	   		//抄读成功
             	 	  	   		tmpBuff[0] = 0x01;
             	 	  	   		tmpBuff[1] = 0x00;
             	 	  	   		tmpBuff[2] = 0x00;
                          if (carrierModuleType == TC_CARRIER)
                          {
                            tmpBuff[3] = port-3;
                            gdw3762Framing(ROUTE_DATA_READ_3762, 1, copyCtrl[port].destAddr, tmpBuff);
                          }
                          else
                          {
                            gdw3762Framing(ROUTE_DATA_READ_3762, 1, NULL, tmpBuff);
                          }
    
                          copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
                           
    	  	   	   	 	   		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	   	   	 	   		{
    	  	   	   	 	   	 		printf("路由主导抄读:本表抄读成功\n");
    	  	   	   	 	   		}
    	  	   	   	 	 		}
    	  	   	   	 	 		else
    	  	   	   	 	 		{
    	  	   	   	 	   		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	   	   	 	   		{
    	  	   	   	 	   	 		printf("路由主导抄读:本采集器下本块485表本次抄读成功,但还有其它485表没有抄读成功\n");
    	  	   	   	 	   		}
    	  	   	   	 	 		}
                        goto breakPoint;
                        break;
                 
	  	   	   	 	   		case 7:    //如果本表抄完但无有效数据的话,就告诉路由抄读失败
	  	   	   	 	   		case 9:    //日冻结抄完后就不再抄其他数据项了,就也告诉路由抄读失败,2013-12-25,add
  	  	   	   	 	     	//2013-12-24,根据鼎信希工的回复,可以给路由模块回抄读失败
  	  	   	   	 	     	//if (carrierModuleType==TC_CARRIER)  //ly,2012-01-09
  	  	   	   	 	     	//{
    	  	   	   	 	 		//  if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	   	   	 	 		//  {
    	  	   	   	 	 		//  	 printf("路由主导抄读:(鼎信)本表抄读失败,但是不给模块置失败标志\n");
    	  	   	   	 	 		//  }
  	  	   	   	 	     	//}
  	  	   	   	 	     	//else
  	  	   	   	 	     	//{
    	  	   	   	 	 		//抄读失败
             	 	  	 		tmpBuff[0] = 0x00;
             	 	  	 		tmpBuff[1] = 0x00;
             	 	  	 		tmpBuff[2] = 0x00;
                        if (carrierModuleType == TC_CARRIER)
                        {
                          tmpBuff[3] = port-3;
                          gdw3762Framing(ROUTE_DATA_READ_3762, 1, copyCtrl[port].destAddr, tmpBuff);
                        }
                        else
                        {
                          gdw3762Framing(ROUTE_DATA_READ_3762, 1, NULL, tmpBuff);
                        }
                           
                        copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
                           
	  	   	   	 	     		if (debugInfo&PRINT_CARRIER_DEBUG)
	  	   	   	 	     		{
	  	   	   	 	   	   		printf("路由主导抄读:本表抄读失败\n");
	  	   	   	 	     		}
	  	   	   	 	     		//}
                        copyCtrl[port].tmpCpLink->flgOfAutoCopy = 0;
                        goto breakPoint;
                         
                        break;
                    }
    	  	   	    }
    	  	   	   	else                                     //集中器主导抄表
    	  	   	   	{
    	  	   	   	 	switch (copyCtrl[port].ifCopyLastFreeze)
    	  	   	   	 	{
    	  	   	   	   	case 2:
    	  	   	         	initCopyDataBuff(port, LAST_DAY_DATA);
    	  	   	         	break;

    	  	   	   	   	case 3:
    	  	   	         	initCopyDataBuff(port, HOUR_FREEZE_DATA);
    	  	   	         	break;
    	  	   	       
    	  	   	       	default:
    	  	   	         	initCopyDataBuff(port, PRESENT_DATA);
    	  	   	         	break;
    	  	   	     	}
    	  	   	   	}
                   
                #ifdef PLUG_IN_CARRIER_MODULE
                 #ifndef MENU_FOR_CQ_CANON
                  showInfo("终端正在抄表...");
                 #endif
                #endif
               	}
    	  	   	 	else
    	  	   	 	{
    	  	    #endif
    	  	   	  
    	  	   	   	initCopyDataBuff(port, copyCtrl[port].ifCopyLastFreeze);
    	  	   	  
    	  	   	#ifdef PLUG_IN_CARRIER_MODULE
    	  	   	 	}
    	  	   	#endif
    	  	   	  
    	  	   	 	copyCtrl[port].retry = 0;
                
               #ifdef PLUG_IN_CARRIER_MODULE
    	  	   	 	if (carrierFlagSet.routeLeadCopy==0)
    	  	   	 	{
    	  	   	   	if (debugInfo&METER_DEBUG)
    	  	   	   	{
    	  	   	     	printf("开始抄下一块表\n");
    	  	   	   	}
    	  	   	 	}
    	  	   	 #endif
    	  	  	}
    	  	 	}
   	      }
   	      else
   	      {
            //超时处理
     	  	 	if (compareTwoTime(copyCtrl[port].copyTimeOut, sysTime))
   	  	    {
              if (debugInfo&METER_DEBUG)
              {
     	  	     	if (port>=4)
     	  	     	{
     	  	       #ifdef PLUG_IN_CARRIER_MODULE
     	  	        if (carrierFlagSet.routeLeadCopy==0)
     	  	       	{
                    if (debugInfo&METER_DEBUG)
                    {
                      printf("端口%d接收超时!\n",port+1);
                    }
                  }
                  else
                  {
                    //if (debugInfo&PRINT_CARRIER_DEBUG)
                    //{
                    //  printf("端口%d为控制进入抄表中而设置的超时!\n", port+1);
                    //}
                  }
                 #endif
                }
                else
                {
                  if (debugInfo&METER_DEBUG)
                  {
                    printf("端口%d接收超时!\n", port+1);
                  }
                }
   	    	   	}
   	    	   	 
     	  	   	if (port>=4)
     	  	   	{
   	    	   	 #ifdef PLUG_IN_CARRIER_MODULE
   	    	   	 	//集中器主导抄表
     	  	     	if (carrierFlagSet.routeLeadCopy==0)
     	  	     	{
   	  	   	      copyCtrl[port].retry++;
   	  	   	    }
   	  	   	   #endif
   	  	   	  }
   	  	   	  else
   	  	   	  {
   	  	   	    copyCtrl[port].retry++;
   	  	   	  }
   	  	   	   
   	  	   	  if (port<=4)  //ly,2012-01-09
   	  	   	  {
   	  	   	    copyCtrl[port].flagOfRetry = TRUE;
   	  	   	  }
   	  	    }
   	      }
      }

breakPoint:
	    usleep(500);   //ly,2011-07-25,add,据top观察,dlzd的CPU占用率多数时间达99%,加上本语句后占用率明显下降
	       
	    continue;
    }
  }   //while(1)
}


/**************************************************
函数名称:queryIfInCopyPeriod
功能描述:查看是否在允许的抄表时段内
调用函数:
被调用函数:
输入参数:INT8U portNum,端口
输出参数:
返回值：状态
修改历史:
    2013-12-31,修改同一小时多个时段判断失败的错误
***************************************************/
INT8U queryIfInCopyPeriod(INT8U portNum)
{
  INT8U i;
  INT8U ifFound = 0;
  INT8U tmpStartHour, tmpStartMin, tmpEndHour, tmpEndMin;

  for(i=0; i<teCopyRunPara.para[portNum].hourPeriodNum; i++)
  {
   	tmpStartMin  = bcdToHex(teCopyRunPara.para[portNum].hourPeriod[i*2][0]);
   	tmpStartHour = bcdToHex(teCopyRunPara.para[portNum].hourPeriod[i*2][1]);
   	tmpEndMin    = bcdToHex(teCopyRunPara.para[portNum].hourPeriod[i*2+1][0]);
   	tmpEndHour   = bcdToHex(teCopyRunPara.para[portNum].hourPeriod[i*2+1][1]);

   	if (
   		  //如果一个时段起始和结束时间的小时是同一个小时数
   		  (sysTime.hour==tmpStartHour && sysTime.minute>=tmpStartMin && sysTime.hour==tmpEndHour && sysTime.minute<tmpEndMin)
   		  
   		   //如果起始和结束的小时数不在一个小时,当前时间的小时等于起始时段的小时且当前时间的分钟大于等于起始时段的分钟数了
   		   ||(tmpStartHour!=tmpEndHour && sysTime.hour==tmpStartHour && sysTime.minute>=tmpStartMin)

   		    //如果起始和结束的小时数不在一个小时,并且已经到了结束时段的小时数了
   		    ||(tmpStartHour!=tmpEndHour && sysTime.hour==tmpEndHour && sysTime.minute<=tmpEndMin)
   		  
   		     //如果在起始和结束的中间的小时
   		     || (sysTime.hour>tmpStartHour && sysTime.hour<tmpEndHour)
   	   )
   	{
   	  ifFound = 1;
   	  break;
   	}
  }
  
  return ifFound;
}

/**************************************************
函数名称:forwardReceive
功能描述:转发直接对电能表抄读数据命令接收处理
调用函数:
被调用函数:
输入参数:INT8U portNum,...
输出参数:
返回值：状态
***************************************************/
void forwardReceive(INT8U portNum,INT8U *data,INT8U recvLen)
{
   INT8U  checkSum, i;
   INT16U tmpi;
   
   for(i=0;i<recvLen;i++)
   {
     copyCtrl[portNum].pForwardData->data[copyCtrl[portNum].pForwardData->recvFrameTail++] = data[i];
     
     if (copyCtrl[portNum].pForwardData->recvFrameTail==21)
     {
       if (copyCtrl[portNum].pForwardData->data[20]!=0x68)
       {
     	 copyCtrl[portNum].pForwardData->recvFrameTail = 20;
     	 copyCtrl[portNum].pForwardData->length        = 2048;
       }
     }
    
     if (copyCtrl[portNum].pForwardData->recvFrameTail==28)
     {
       if (copyCtrl[portNum].pForwardData->data[27]!=0x68)
       {
     	 copyCtrl[portNum].pForwardData->recvFrameTail = 20;
     	 copyCtrl[portNum].pForwardData->length        = 2048;
     	 break;
       }
     }
     
     if (copyCtrl[portNum].pForwardData->recvFrameTail==30)
     {
       copyCtrl[portNum].pForwardData->length = copyCtrl[portNum].pForwardData->data[29]+12;
     }
     
     if (copyCtrl[portNum].pForwardData->recvFrameTail==copyCtrl[portNum].pForwardData->length+20)
     {
       //计算校验和且按字节将数据域进行减0x33处理
       checkSum = 0;
       for(tmpi=20; tmpi<copyCtrl[portNum].pForwardData->recvFrameTail-2; tmpi++)
       {
         checkSum += copyCtrl[portNum].pForwardData->data[tmpi];
         if (tmpi>29)
         {
           copyCtrl[portNum].pForwardData->data[tmpi] -= 0x33;  //按字节进行对数据域减0x33处理
         }
       }
            
       //如果校验和正确,执行meterInput操作
       if (checkSum == copyCtrl[portNum].pForwardData->data[copyCtrl[portNum].pForwardData->recvFrameTail-2])
       {
     	 if (debugInfo&METER_DEBUG)
     	 {
     	   printf("直接抄读电表回复正确帧\n");
     	 }
     	   	
     	 copyCtrl[portNum].pForwardData->nextBytesTime = sysTime;
      	 copyCtrl[portNum].pForwardData->forwardResult = RESULT_HAS_DATA;          
       }
     }
   }
}

#ifdef PLUG_IN_CARRIER_MODULE
/**************************************************
函数名称:threadOfCarrierReceive
功能描述:ttys5(载波接口)接收处理线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void *threadOfCarrierReceive(void *arg)
{
  INT8U                   recvLen, libLen, tmpLen;
  INT8U                   tmpBuf[250];
  INT8U                   i;
  INT8S                   ret;
  struct carrierMeterInfo *tmpSalveNode;
  struct cpAddrLink       *tmpNode;
  struct carrierMeterInfo *tmpQueryFound;
  INT32U                  tmpAddr;
  INT8U                   tmpSendBuf[50];
  
  while (1)
  {
     if (upRtFlag!=0)
     {
     	 sleep(1);
     	 
     	 continue;
     }
     
     //recvLen = read(fdOfCarrier,&tmpBuf,100);
     
     //2011-05-31,改成每次接收30,否则晓程的接收会有问题,改了后测试了东软和晓程的可以
     recvLen = read(fdOfCarrier, &tmpBuf, 30);
     
     if (debugInfo&PRINT_CARRIER_DEBUG)
     {
       printf("%02d-%02d-%02d %02d:%02d:%02d Local Module Rx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
   
       for(i=0;i<recvLen;i++)
       {
         printf("%02x ",tmpBuf[i]);
       }
       printf("\n");
     }
    
    #ifdef LM_SUPPORT_UT
     if (0x55==lmProtocol)
     {
       if (copyCtrl[4].pForwardData!=NULL)  //转发
       {
       	 if (copyCtrl[4].pForwardData->ifSend == TRUE)
       	 {
       	   if (debugInfo&METER_DEBUG)
       	   {
             printf("%02d-%02d-%02d %02d:%02d:%02d LM Rx(转发):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
             for(i=0;i<recvLen;i++)
             {
      	       printf("%02x ",tmpBuf[i]);
             }
             printf("\n");
       	   }
  
       	   if (copyCtrl[4].pForwardData->fn==1)
       	   {
       	     //接收到回复数据后等待两秒,看是否有后续字节
       	     if(copyCtrl[4].pForwardData->receivedBytes==FALSE)
       	     {
       	   	   copyCtrl[4].pForwardData->receivedBytes = TRUE;
       	   	   copyCtrl[4].pForwardData->nextBytesTime = nextTime(sysTime, 0, 3);
  
         	     memcpy(copyCtrl[4].pForwardData->data, tmpBuf, recvLen);
        	     copyCtrl[4].pForwardData->length = recvLen;
        	     copyCtrl[4].pForwardData->forwardResult = RESULT_HAS_DATA;
       	     }
       	     else
       	     {
         	     memcpy(&copyCtrl[4].pForwardData->data[copyCtrl[4].pForwardData->length], tmpBuf, recvLen);
        	     copyCtrl[4].pForwardData->length += recvLen;
       	     }
       	   }
       	   else
       	   {
             forwardReceive(4, tmpBuf, recvLen);
       	   }
       	 }
       	 else
       	 {
           meter485Receive(PORT_POWER_CARRIER, tmpBuf, recvLen);
       	 }
       }
       else
       {
         meter485Receive(PORT_POWER_CARRIER, tmpBuf, recvLen);
       }

     	 goto breakCarrierRecv;
     }
    #endif
     
     libLen = recvLen;

repeatRecv:
     if (libLen==recvLen)
     {
       ret = gdw3762Receiving(tmpBuf, &libLen);
     }
     else
     {
       tmpLen = libLen;
       recvLen -= libLen;
       libLen = recvLen;
       
       if (debugInfo&PRINT_CARRIER_DEBUG)
       {
         printf("本地通信模块:本次接收的第二次处理,本次处理%d字节\n", libLen);
       }
       
       ret = gdw3762Receiving(&tmpBuf[tmpLen], &libLen);
     }
     
     switch(ret)
     {
     	  case RECV_DATA_CORRECT:   //接收到正确的一帧
     	  	switch(carrierAfn)
     	  	{
     	  		case ACK_OR_NACK_3762:   //确认/否认
     	  		 	switch(carrierFn)
     	  		 	{
     	  		 	 	case 1:   //确认
                  switch(carrierFlagSet.cmdType)
                  {
                  	case CA_CMD_HARD_RESET:
	                    //对本地通信模块硬件初始化的确认
	                    if (carrierFlagSet.hardwareReest == 1)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.hardwareReest = 2;
                       
                        carrierFlagSet.cmdTimeOut  = nextTime(sysTime, 0, 5);
     	  		 	 	  		 
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	  printf("确认硬件初始化\n");
     	  		 	 	  		  }
     	  		 	 	  	  }
     	  		 	 	  	  break;

                  	case CA_CMD_SR_PARA_INIT:    //桑锐模块清除参数区的确认                    
                      if (carrierFlagSet.paraClear==0 && carrierModuleType==SR_WIRELESS)
                      {
                    	  carrierFlagSet.paraClear = 1;
                    	  carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 5);
     	  		 	 	  		
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	   printf("SR确认参数区初始化\n");
     	  		 	 	  		  }
                      }
                      break;
                    
                    case CA_CMD_CLEAR_PARA:    //参数区初始化
                      //强制参数区初始化的确认
                      if (debugInfo & DELETE_LOCAL_MODULE)
                      {
                    	   debugInfo &= ~DELETE_LOCAL_MODULE;
                    	 
     	  		 	 	  		   if (debugInfo&METER_DEBUG)
     	  		 	 	  		   {
     	  		 	 	  		 	   printf("确认强制参数区初始化\n");
     	  		 	 	  		   }
                      }

            	        if (carrierFlagSet.checkAddrClear==1)
            	        {
            	        	carrierFlagSet.checkAddrClear = 0;

            	      	  if (debugInfo&PRINT_CARRIER_DEBUG)
            	      	  {
            	      		  printf("确认比较集中器与模块档案地址不一致时清除表地址\n");
            	          }
            	        }

     	  		 	 	  	  if (carrierFlagSet.init>0)
     	  		 	 	  	  {
     	  		 	 	  		  if (carrierFlagSet.synSlaveNode==1)
     	  		 	 	  		  {
     	  		 	 	  		 	  if (debugInfo&METER_DEBUG)
     	  		 	 	  		 	  {
     	  		 	 	  		 	    if (carrierModuleType==RL_WIRELESS)
     	  		 	 	  		 	    {
     	  		 	 	  		 	      printf("RL确认删除所有从节点\n");
     	  		 	 	  		 	    }
     	  		 	 	  		 	    else
     	  		 	 	  		 	    {
     	  		 	 	  		 	      printf("确认参数区初始化\n");
     	  		 	 	  		 	    }
     	  		 	 	  		 	  }
     	  		 	 	  		 	  
     	  		 	 	  		 	  carrierFlagSet.synSlaveNode   = 0;
     	  		 	 	  		 	  carrierFlagSet.querySlaveNode = 0;
     	  		 	 	  		 	  carrierFlagSet.init = 0;
     	  		 	 	  		  }
     	  		 	 	  	  }
                      break;
                    
                    case CA_CMD_SET_MAIN_NODE:    //设置主节点的确认
                      if (carrierFlagSet.setMainNode==1)
                      {
                    	   carrierFlagSet.setMainNode = 2;
                    	   carrierFlagSet.mainNode = 0;     //要重新查询主节点地址
                      }
                      break;
     	  		 	 	  	
     	  		 	 	  	case CA_CMD_SR_CHECK_NET:
     	  		 	 	  	  if (carrierFlagSet.startStopWork==2)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.startStopWork = 0;
     	  		 	 	  		   
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	  printf("SR模块确认设置为组网验证模式\n");
     	  		 	 	  		  }
     	  		 	 	  	  }
     	  		 	 	  		break;
     	  		 	 	  	
     	  		 	 	  	case CA_CMD_ADD_SLAVE_NODE:
     	  		 	 	  	  //正在添加从节点
     	  		 	 	  	  if (carrierFlagSet.synSlaveNode==2)
     	  		 	 	  	  {
     	  		 	 	  		  copyCtrl[4].tmpCpLink = copyCtrl[4].tmpCpLink->next;
     	  		 	 	  		  carrierFlagSet.synMeterNo++;
     	  		 	 	  	  }
     	  		 	 	  		break;
     	  		 	 	    
     	  		 	 	    case CA_CMD_SET_WORK_MODE:
     	  		 	 	  	  //设置本地通信模块工作状态的确认
     	  		 	 	  	  if (carrierFlagSet.workStatus==1)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.workStatus = 2;    //工作模式为抄表
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("本地通信模块确认设置工作为抄表模式\n");
                        }
                      }
       	  		 	 	  	
       	  		 	 	  	if (carrierFlagSet.searchMeter==1)
       	  		 	 	  	{
       	  		 	 	  		carrierFlagSet.searchMeter = 2;
       	  		 	 	  		  
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("本地通信模块确认设置工作为主动注册模式\n");
       	  		 	 	  		}
       	  		 	 	    }
       	  		 	 	  	
       	  		 	 	  	if (carrierFlagSet.searchMeter==5)
       	  		 	 	  	{
       	  		 	 	  		carrierFlagSet.searchMeter = 6;
       	  		 	 	  		
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
     	  		 	 	  		    printf("载波模块确认搜表后设置工作模式为抄表的命令\n");
       	  		 	 	  		}
       	  		 	 	    }
     	  		 	 	    	break;
     	  		 	 	    	
     	  		 	 	    case CA_CMD_ACTIVE_REGISTER: //激活载波从节点主动注册
       	  		 	 	  	if (carrierFlagSet.searchMeter==2)
       	  		 	 	  	{
     	  		 	 	  		  if (carrierModuleType==CEPRI_CARRIER || carrierModuleType==MIA_CARRIER)
     	  		 	 	  		  {
     	  		 	 	  		     carrierFlagSet.searchMeter = 3;
     	  		 	 	  		  }
     	  		 	 	  		  else
     	  		 	 	  		  {
     	  		 	 	  		     carrierFlagSet.searchMeter = 4;     	  		 	 	  		  	 
     	  		 	 	  		  }
     	  		 	 	  		  
     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("模块确认主动注册命令\n");
     	  		 	 	  		  }
                        carrierFlagSet.ifSearched = 1;
                      }
     	  		 	 	    	break;
     	  		 	 	    
     	  		 	 	    case CA_CMD_RESTART_WORK:    //置工作状态为重启的确认
     	  		 	 	  	  if (carrierFlagSet.workStatus==2)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 3;   //工作状态为重启的抄表
                        
                        copyCtrl[4].meterCopying = TRUE;
                        copyCtrl[4].copyContinue = FALSE;

                        copyCtrl[4].tmpCpLink = copyCtrl[4].cpLinkHead;
       	  		 	 	      while(copyCtrl[4].tmpCpLink!=NULL)
       	  		 	 	      {
       	  		 	 	   	 		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	   	 		{
       	  		 	 	   	 		  printf("清除测量点%d自动抄表标志\n", copyCtrl[4].tmpCpLink->mp);
       	  		 	 	   	 		}
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->flgOfAutoCopy = 0;
       	  		 	 	   	 	 
       	  		 	 	   	 	  //2013-10-24,照明集中器添加处理
       	  		 	 	   	 	 #ifdef LIGHTING
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->status = 0xfe;
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->statusTime  = sysTime;
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->lddtStatusTime  = sysTime;    //2016-02-03,Add
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->msCtrlCmd   = 0;
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->msCtrlTime  = sysTime;
       	  		 	 	   	 	 #endif
       	  		 	 	   	 		
       	  		 	 	   	    copyCtrl[4].tmpCpLink = copyCtrl[4].tmpCpLink->next;
       	  		 	 	   	  }
       	  		 	 	   	  
                        if (carrierModuleType==TC_CARRIER)
                        {
                          copyCtrl[5].meterCopying = TRUE;
                          copyCtrl[5].copyContinue = FALSE;
                          
                          copyCtrl[6].meterCopying = TRUE;
                          copyCtrl[6].copyContinue = FALSE;
                        }
                        
                        carrierFlagSet.routeRequestOutTime = nextTime(sysTime, ROUTE_REQUEST_OUT, 0);
                        
                       #ifdef LIGHTING
                        //下一次搜索报警控制点的末端单灯控制器时间
                        //  2015-05-25,改为3分钟后搜索
                        carrierFlagSet.searchLddtTime = nextTime(sysTime, 3, 0);
                        
                        //2015-11-09,搜索离线的单灯控制器时间为3分钟后
                        carrierFlagSet.searchOffLineTime = nextTime(sysTime, 3, 0);
                        
                        //2015-11-12,搜索单灯控制器状态时间为3分钟后
                        carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 3, 0);
                        
                       #endif
                        
                        carrierFlagSet.chkNodeBeforeCopy = 0;    //2013-12-30,add
                        
                        //确定下次抄时间
                        initCopyItem(4, 0);
                        
                        if (menuInLayer==0)
                        {
                          defaultMenu();
                        }
                        
                        //2012-08-28,add this line
                        copyCtrl[4].lastCopyTime = timeHexToBcd(sysTime);
                        
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认重启抄表:端口31开始抄表\n");
                        }
                        
                        #ifdef PLUG_IN_CARRIER_MODULE
                         #ifndef MENU_FOR_CQ_CANON
                           showInfo("终端正在抄表...");
                         #endif
                        #endif
                      }
       	  		 	 	  	
       	  		 	 	  	if (carrierFlagSet.searchMeter==3) //模块确认重启搜表命令
       	  		 	 	  	{
       	  		 	 	  		carrierFlagSet.searchMeter = 4;
       	  		 	 	  		  
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("模块确认重启主动注册命令\n");
       	  		 	 	  		}
       	  		 	 	    }
       	  		 	 	    
     	  		 	 	  		if (carrierFlagSet.searchMeter==6)
     	  		 	 	  		{
     	  		 	 	  		  carrierFlagSet.searchMeter = 0;
     	  		 	 	  		 
     	  		 	 	  		  if (menuInLayer==0)
     	  		 	 	  		  {
     	  		 	 	  		  	defaultMenu();
     	  		 	 	  		  }
                        searchMeter(0xff);

     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("载波模块确认搜表后重启抄表命令\n");
     	  		 	 	  		  }
     	  		 	 	  		}
     	  		 	 	  		
     	  		 	 	  		if (carrierFlagSet.batchCopy==1)
     	  		 	 	  		{
     	  		 	 	  			carrierFlagSet.batchCopy = 2;
     	  		 	 	  			
     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("电科院载波模块确认重启轮抄命令\n");
     	  		 	 	  		  }
     	  		 	 	  		  
                        copyCtrl[4].meterCopying = TRUE;
                        copyCtrl[4].copyContinue = FALSE;

                        //确定下次抄时间
                        //initCopyItem(4, 0);
                        
                        if (menuInLayer==0)
                        {
                          defaultMenu();
                        }
                        
                        #ifdef PLUG_IN_CARRIER_MODULE
                         #ifndef MENU_FOR_CQ_CANON
                           showInfo("终端正在抄表...");
                         #endif
                        #endif
     	  		 	 	  		}
                      break;
                    
                    case CA_CMD_PAUSE_WORK:      //暂停当前工作命令
     	  		 	 	  	  if (carrierFlagSet.startStopWork==2)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.startStopWork = 0;
     	  		 	 	  		   
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	  printf("本地通信模块确认初始启动时停止工作命令\n");
     	  		 	 	  		  }
     	  		 	 	  	  }

     	  		 	 	  	  if (carrierFlagSet.workStatus==4)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //工作状态为暂停抄表
                        
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认暂停抄表\n");
                        }
                        
                        if (carrierFlagSet.synSlaveNode==1 && copyCtrl[4].meterCopying==TRUE)
                        {
                        	copyCtrl[4].meterCopying = FALSE;
                        	
                         	//ly,2012-01-10,增加鼎信的处理
                         	if (carrierModuleType==TC_CARRIER)
                         	{
                         	  copyCtrl[5].meterCopying = FALSE;
                         	  copyCtrl[6].meterCopying = FALSE;
                         	}
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
                            printf("路由主导抄读:为同步表号停止抄表\n");
                          }
                          carrierFlagSet.reStartPause = 0;
                        }
                        
                        if (carrierFlagSet.searchMeter==1 && copyCtrl[4].meterCopying==TRUE)
                        {
                        	copyCtrl[4].meterCopying = FALSE;

                         	//ly,2012-01-10,增加鼎信的处理
                         	if (carrierModuleType==TC_CARRIER)
                         	{
                         	  copyCtrl[4].meterCopying = FALSE;
                         	  copyCtrl[5].meterCopying = FALSE;
                         	}
                            
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
                            printf("路由主导抄读:为执行搜表命令而停止抄表\n");
                          }
                          
                          carrierFlagSet.reStartPause = 0;
                        }
                      }

     	  		 	 	  	  if (carrierFlagSet.workStatus==7)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //工作状态为暂停抄表
                        
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认暂停抄表(出抄表时段)\n");
                        }
                        
                      	copyCtrl[4].meterCopying = FALSE;
                      	
                       	//ly,2012-01-10,增加鼎信的处理
                       	if (carrierModuleType==TC_CARRIER)
                       	{
                       	  copyCtrl[5].meterCopying = FALSE;
                       	  copyCtrl[6].meterCopying = FALSE;
                       	}
                       	
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);
     	  		 	 	  	  }
     	  		 	 	  	  
     	  		 	 	  	 #ifdef LIGHTING
     	  		 	 	  	  if (carrierFlagSet.workStatus==8)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //工作状态为暂停抄表
                        
                        carrierFlagSet.searchLddt = 1;
                        
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);

                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认暂停抄表(报警监测超时限末端单灯控制器)\n");
                        }
     	  		 	 	  	  }
     	  		 	 	  	  if (carrierFlagSet.workStatus==9)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //工作状态为暂停抄表
                        
                        carrierFlagSet.searchOffLine = 1;
                        
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);

                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认暂停抄表(检测到有超离线阈值单灯控制器)\n");
                        }
     	  		 	 	  	  }
     	  		 	 	  	  if (carrierFlagSet.workStatus==10)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //工作状态为暂停抄表
                        
                        carrierFlagSet.searchLddStatus = 1;
                        
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);

                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认暂停抄表(查询线路刚上电单灯控制器状态)\n");
                        }
     	  		 	 	  	  }
     	  		 	 	  	 #endif
     	  		 	 	  	 
       	  		 	 	  	if (carrierFlagSet.searchMeter==4) //模块确认停止搜表命令
       	  		 	 	  	{
              	 	 		  if (carrierModuleType!=CEPRI_CARRIER)
              	 	 		  {
              	 	 		    carrierFlagSet.searchMeter = 0;
              	 	 		  }
              	 	 		  else
              	 	 		  {
       	  		 	 	  		  carrierFlagSet.searchMeter = 5;
       	  		 	 	  		}
       	  		 	 	  		  
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("模块确认停止搜表命令\n");
       	  		 	 	  		}
       	  		 	 	  		  
       	  		 	 	  		if (menuInLayer==0)
       	  		 	 	  		{
       	  		 	 	  		  defaultMenu();
       	  		 	 	  		}
       	  		 	 	  		
       	  		 	 	  		//5秒钟后开始抄表
       	  		 	 	  		copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 5);
       	  		 	 	    }
                    	break;
                    
                    case CA_CMD_RESTORE_WORK:    //恢复工作命令
     	  		 	 	  	  if (carrierFlagSet.workStatus==6)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 3;   //工作状态为恢复抄表,恢复后就是抄表的状态
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("载波模块确认恢复抄表\n");
                        }
                      }
                      break;
                    
                    case CA_CMD_SET_MODULE_TIME:  //设置时间命令
                      carrierFlagSet.setDateTime = 1;
                      
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("本地通信模块(载波/无线)模块确认设置的时间\n");
                      }                      
                      break;
                    
                    case CA_CMD_UPDATE_ADDR:     //友迅达更新档案命令
                    	carrierFlagSet.querySlaveNode = 2;
                    	
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("友迅达无线模块确认更新档案命令\n");
                      }
                    	break;
                    	
                    case CA_CMD_BATCH_COPY:     //电科院轮抄命令
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("电科院载波模块确认轮抄命令\n");
                      }
                      
                      carrierFlagSet.batchCopy = 1;
                    	break;
                    
                    case CA_CMD_BROAD_CAST:    //广播命令
     	  		 	 	  	 #ifdef LIGHTING
     	  		 	 	  	  if (1==carrierFlagSet.broadCast 
     	  		 	 	  	  	  || 2==carrierFlagSet.broadCast 
     	  		 	 	  	  	   || 3==carrierFlagSet.broadCast 
     	  		 	 	  	  	    || 4==carrierFlagSet.broadCast 
     	  		 	 	  	  	 )
     	  		 	 	  	  {
     	  		 	 	  	  	if (2==carrierFlagSet.broadCast)
     	  		 	 	  	  	{
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[0] = 0x68;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[1] = 0x99;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[2] = 0x99;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[3] = 0x99;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[4] = 0x99;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[5] = 0x99;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[6] = 0x99;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[7] = 0x68;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[8] = 0x9c;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[9] = 0x00;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[10] = 0x02;
     	  		 	 	  	  		copyCtrl[4].pForwardData->data[11] = 0x16;
     	  		 	 	  	  		copyCtrl[4].pForwardData->length = 12;
     	  		 	 	  	  		
     	  		 	 	  	  		copyCtrl[4].pForwardData->forwardResult=RESULT_HAS_DATA;
     	  		 	 	  	  		
     	  		 	 	  	  		forwardDataReply(4);
     	  		 	 	  	  	}
     	  		 	 	  	  	
     	  		 	 	  	  	carrierFlagSet.broadCast = 5;
     	  		 	 	  	  	
     	  		 	 	  	  	if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  	  	{
     	  		 	 	  	  		printf("模块确认广播命令,模块要求等待时间为%d秒,", tmpBuf[2]|tmpBuf[3]<<8);
     	  		 	 	  	  	}
     	  		 	 	  	  	
     	  		 	 	  	  	if (0==pnGate.boardcastWaitGate)
     	  		 	 	  	  	{
     	  		 	 	  	  	  carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 0, tmpBuf[2]|tmpBuf[3]<<8);
                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, tmpBuf[2]|tmpBuf[3]<<8);

     	  		 	 	  	  	  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  	  	  {
     	  		 	 	  	  		  printf("按模块要求值等待\n");
     	  		 	 	  	  	  }
     	  		 	 	  	  	}
     	  		 	 	  	  	else
     	  		 	 	  	  	{
     	  		 	 	  	  	  if ((tmpBuf[2]|tmpBuf[3]<<8)<(pnGate.boardcastWaitGate*60))    //2015-10-26,添加这个判断
     	  		 	 	  	  	  {
     	  		 	 	  	  	    carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 0, tmpBuf[2]|tmpBuf[3]<<8);
                            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, tmpBuf[2]|tmpBuf[3]<<8);

     	  		 	 	  	  	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  	  	    {
     	  		 	 	  	  		    printf("Waiting...\n");
     	  		 	 	  	  	    }
     	  		 	 	  	  	  }
     	  		 	 	  	  	  else
     	  		 	 	  	  	  {
     	  		 	 	  	  	    carrierFlagSet.broadCastWaitTime = nextTime(sysTime, pnGate.boardcastWaitGate, 0);
                            carrierFlagSet.operateWaitTime = nextTime(sysTime, pnGate.boardcastWaitGate, 0);
     	  		 	 	  	  	  
     	  		 	 	  	  	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  	  	    {
     	  		 	 	  	  		    printf("但主站设置了该阈值,等待时间为%d分\n", pnGate.boardcastWaitGate);
     	  		 	 	  	  		  }
     	  		 	 	  	  	  }
     	  		 	 	  	  	}
     	  		 	 	  	  }
     	  		 	 	  	 #endif
                    	break;

     	  		 	 	  	default:
     	  		 	 	  	  if (carrierModuleType==RL_WIRELESS)
     	  		 	 	  	  {
   	  		 	 	  		 	   if (carrierFlagSet.setupNetwork==1)
   	  		 	 	  		 	   {
   	  		 	 	  		       if (debugInfo&METER_DEBUG)
   	  		 	 	  		       {
   	  		 	 	  		 	       printf("RL模块:确认组网命令\n");
   	  		 	 	  		       }
   	  		 	 	  		 
   	  		 	 	  		       carrierFlagSet.setupNetwork = 2;
   	  		 	 	  		     }
     	  		 	 	  	  }
     	  		 	 	  	
     	  		 	 	  	  //开始学习路由命令确认
     	  		 	 	  	  if (carrierFlagSet.studyRouting==1)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.studyRouting = 2;    //模块开始路由学习
     	  		 	 	  	  }
     	  		 	 	  	
     	  		 	 	  	  if (carrierFlagSet.studyRouting==3)   //命令停止学习确认
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.studyRouting = 0;    //模块已停止学习
     	  		 	 	  		 
     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("模块确认停止学习命令\n");
     	  		 	 	  		  }
     	  		 	 	  	  }

     	  		 	 	  	  //设置信道号(SR,FC,etc.)
     	  		 	 	  	  if (carrierFlagSet.setPanId==0)
     	  		 	 	  	  {
     	  		 	 	  	  	carrierFlagSet.setPanId = 1;

     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("模块确认设置信道号命令\n");
     	  		 	 	  		  }
     	  		 	 	  	  }     	  		 	 	  	  
       	  		 	 	  	break;
     	  		 	 	  }
     	  		 	 	  break;
     	  		 	 	  	
   	  		 	 	  case 2:   //否认
   	  		 	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	{
   	  		 	 	  	  printf("否认,错误状态字:%d\n",tmpBuf[0]);
   	  		 	 	  	}
   	  		 	 	  	
   	  		 	 	  	//正在添加从节点
   	  		 	 	  	if (carrierFlagSet.synSlaveNode==2)
   	  		 	 	  	{
   	  		 	 	  		 copyCtrl[4].tmpCpLink = copyCtrl[4].tmpCpLink->next;
   	  		 	 	  		 carrierFlagSet.synMeterNo++;
   	  		 	 	  	}
   	  		 	 	  	
   	  		 	 	  	switch(tmpBuf[0])
   	  		 	 	    {
   	  		 	 	    	case 7:  //表号不存在(晓程,东软)
   	  		 	 	  	    if (carrierFlagSet.querySlaveNode==1)
   	  		 	 	  	    {
 	                       carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
 	                   
 	                       if (debugInfo&PRINT_CARRIER_DEBUG)
 	                       {
 	                         printf("载波模块从节点列表\n");
 	                         tmpSalveNode = carrierSlaveNode;
 	                         while(tmpSalveNode!=NULL)
 	                         {
   	  		 	 	  	           printf("从节点地址:%02x%02x%02x%02x%02x%02x\n",tmpSalveNode->addr[5],tmpSalveNode->addr[4],tmpSalveNode->addr[3],tmpSalveNode->addr[2],tmpSalveNode->addr[1],tmpSalveNode->addr[0]);
   	  		 	 	  	           tmpSalveNode = tmpSalveNode->next;
 	                         }
 	                       }
   	  		 	 	  	    }
   	  		 	 	  	    break;
   	  		 	 	  	    
   	  		 	 	  	  case 253:  //ly,2011-05-20,在大足试运行弥亚微方案时发生点抄时回253否认,弥亚微工程师叫发现这种情况时复位模块
   	  		 	 	  	  	carrierFlagSet.numOf253++;
   	  		 	 	  	  	if (carrierModuleType==MIA_CARRIER)
   	  		 	 	  	  	{
   	  		 	 	  	  		if (carrierFlagSet.numOf253>10)
   	  		 	 	  	  		{
   	  		 	 	  	  		  ifReset = TRUE;
   	  		 	 	  	  		
   	  		 	 	  	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  		  {
   	  		 	 	  	  			  printf("弥亚微模块发现错误状态字是253超过10次的,重启集中器\n");
   	  		 	 	  	  		  }
   	  		 	 	  	  		}
   	  		 	 	  	  	}
   	  		 	 	  	  	break;
   	  		 	 	  	     
   	  		 	 	  	  case  0:   //通信超时
   	  		 	 	  	  case  4:   //信息类不存在(晓程)
   	  		 	 	  	  case  8:   //电能表层无应答(晓程)
   	  		 	 	  	  case 14:   //操作不成功(无线模块)
   	  		 	 	  	  case 16:   //锐拔协议(网络维护中)
   	  		 	 	  	  case 18:   //锐拔协议(表号不在网络中)
   	  		 	 	  	  case 19:   //锐拔协议(表号在网络中,但当前网络暂时不通)
   	  		 	 	  	  case 20:   //锐拔协议(表号在网络中,当前网络畅通,但表计本地RS485不通)
   	  		 	 	  	  case 22:   //锐拔协议(表号在网络中,当前网络畅通,但II型表无线通讯有故障)
   	  		 	 	  	  case 0xff: //弥亚微(未知错误)
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        switch(tmpBuf[0])
                        {
                        	case 0:
                            printf("通信超时\n");
                        		break;

                        	case 4:
                            printf("信息类不存在(电科院模块)\n");
                        		break;

                        	case 8:
                            printf("电表层无应答(电科院模块)\n");
                        		break;
                        	
                        	case 14:
                            printf("操作失败(SR无线模块)\n");
                        		break;

                        	case 16:
                            printf("网络维护中(RL无线模块)\n");                         	
                        		break;

                        	case 18:
                            printf("表号不在网络中(RL无线模块)\n");                         	
                        		break;

                        	case 19:
                            printf("表号在网络中,但当前网络暂时不通(RL无线模块)\n");                         	
                        		break;

                        	case 20:
                            printf("表号在网络中,当前网络畅通,但表计本地RS485不通(RL无线模块)\n");                         	
                        		break;

                        	case 22:
                            printf("表号在网络中,当前网络畅通,但II型表无线通讯有故障(RL无线模块)\n");                         	
                        		break;

                        	case 0xff:
                            printf("未知错误\n");                         	
                        		break;
                        }
                      }
   	  		 	 	  	    
   	  		 	 	  	    if (pDotCopy!=NULL)
   	  		 	 	  	    {
   	  		 	 	  	    	 if(pDotCopy->dotCopying==TRUE)
   	  		 	 	  	    	 {
   	  		 	 	  	         pDotCopy->outTime = nextTime(sysTime, 0, 1);
   	  		 	 	  	    	 }
   	  		 	 	  	    }
   	  		 	 	  	    
   	  		 	 	  	    if (copyCtrl[4].meterCopying==TRUE)
   	  		 	 	  	    {
   	  		 	 	  	    	 copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 1);
   	  		 	 	  	    }
   	  		 	 	  	    break;
   	  		 	 	  	}
   	  		 	 	  	break;
     	  		 	}
     	  		 	break;
     	  		 
     	  		case QUERY_DATA_3762:    //查询数据
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	  case 1:    //查询厂商代码和版本信息
     	  		 	 	  	memcpy(carrierFlagSet.productInfo, tmpBuf, 9);
     	  		 	 	  	
     	  		 	 	  	//如果鼎信载波是三相并行抄表,因此扩展2个抄表端口来处理
     	  		 	 	  	if (carrierModuleType==TC_CARRIER)
     	  		 	 	  	{
     	  		 	 	  		numOfCopyPort = NUM_OF_COPY_METER+2;
     	  		 	 	  		
                      copyCtrl[5].flagOfRetry        = FALSE;
                      copyCtrl[5].retry              = 0;
                      copyCtrl[5].backupCtrlWord     = 0;
                      copyCtrl[5].pForwardData       = NULL;
                      copyCtrl[5].thisMinuteProcess  = FALSE;
                      copyCtrl[5].ifRealBalance      = FALSE;
                      copyCtrl[5].ifBackupDayBalance = 0;
                      copyCtrl[5].ifCopyDayFreeze    = 0;
                      copyCtrl[5].cmdPause           = FALSE;
                      copyCtrl[5].cpLinkHead         = NULL;
                      copyCtrl[5].numOfMeterAbort = 0;

                      copyCtrl[6].flagOfRetry        = FALSE;
                      copyCtrl[6].retry              = 0;
                      copyCtrl[6].backupCtrlWord     = 0;
                      copyCtrl[6].pForwardData       = NULL;
                      copyCtrl[6].thisMinuteProcess  = FALSE;
                      copyCtrl[6].ifRealBalance      = FALSE;
                      copyCtrl[6].ifBackupDayBalance = 0;
                      copyCtrl[6].ifCopyDayFreeze    = 0;
                      copyCtrl[6].cmdPause           = FALSE;
                      copyCtrl[6].cpLinkHead         = NULL;
                      copyCtrl[6].numOfMeterAbort = 0;
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	if (carrierModuleType==CEPRI_CARRIER)
     	  		 	 	  	{
     	  		 	 	  		if (tmpBuf[6]>=0x11)
     	  		 	 	  		{
     	  		 	 	  			localModuleType=CEPRI_CARRIER_3_CHIP;
     	  		 	 	  		}
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	if (carrierModuleType==SR_WIRELESS)
     	  		 	 	  	{
     	  		 	 	  		if (tmpBuf[2]==0x33 && tmpBuf[3]==0x30)
     	  		 	 	  		{
     	  		 	 	  			localModuleType = SR_WF_3E68;
     	  		 	 	  			
     	  		 	 	  			if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  			{
     	  		 	 	  				 printf("SR - SRWF-3E68模块\n");
     	  		 	 	  			}
     	  		 	 	  		}
     	  		 	 	  	}
     	  		 	 	  	break;
     	  		 	 	  	
     	  		 	 	  case 4:
     	  		 	 	  	carrierFlagSet.mainNode = 1;    //已得到主节点地址
                    memcpy(carrierFlagSet.mainNodeAddr, &tmpBuf[13], 6);
                    
                    selectParameter(0x04, 133, mainNodeAddr, 6);

                    if (debugInfo&PRINT_CARRIER_DEBUG)
                    {
                    	printf("载波/无线模块内主节点地址:%02x%02x%02x%02x%02x%02x\n",carrierFlagSet.mainNodeAddr[0],
                    	       carrierFlagSet.mainNodeAddr[1],carrierFlagSet.mainNodeAddr[2],carrierFlagSet.mainNodeAddr[3],
                    	       carrierFlagSet.mainNodeAddr[4],carrierFlagSet.mainNodeAddr[5]);
                      printf("集中器mainNodeAddr=%02x%02x%02x%02x%02x%02x\n",mainNodeAddr[0],mainNodeAddr[1],mainNodeAddr[2],mainNodeAddr[3],mainNodeAddr[4],mainNodeAddr[5]);
                    }
                    
                    if (mainNodeAddr[0]==0x00 && mainNodeAddr[1]==0x00 && mainNodeAddr[2]==0x00
                    	   && mainNodeAddr[3]==0x00 && mainNodeAddr[4]==0x00 && mainNodeAddr[5]==0x00
                    	  )
                    {
                       tmpAddr = hexToBcd(addrField.a2[0] | addrField.a2[1]<<8);
                       mainNodeAddr[0] = tmpAddr&0xff;
                       mainNodeAddr[1] = tmpAddr>>8&0xff;
                       mainNodeAddr[2] = tmpAddr>>16&0xf;
                       mainNodeAddr[3] = 0x0;
                       mainNodeAddr[4] = 0x0;
                       mainNodeAddr[5] = 0x0;
                    }
                    
                    //比较本地通信模块地址与集中器地址是否一样,不一样的话要下发
                  	if (carrierModuleType==SC_WIRELESS)   //赛康的模块不能下发主节点地址
                  	{
                  		 carrierFlagSet.setMainNode = 0;
                  	}
                  	else
                  	{
                  	  if (carrierModuleType==RL_WIRELESS)
                  	  {
                  	    if (carrierFlagSet.mainNodeAddr[0]!=mainNodeAddr[0] || carrierFlagSet.mainNodeAddr[1]!=mainNodeAddr[1]
                  		     || carrierFlagSet.mainNodeAddr[2]!=mainNodeAddr[2] || carrierFlagSet.mainNodeAddr[3]!=mainNodeAddr[3]
                  		     )
                        {
                    	    carrierFlagSet.setMainNode = 1;
                    	  }
                  	  }
                  	  else
                  	  {
                  	    if (carrierFlagSet.mainNodeAddr[0]!=mainNodeAddr[0] || carrierFlagSet.mainNodeAddr[1]!=mainNodeAddr[1]
                  		     || carrierFlagSet.mainNodeAddr[2]!=mainNodeAddr[2] || carrierFlagSet.mainNodeAddr[3]!=mainNodeAddr[3]
                  		      || carrierFlagSet.mainNodeAddr[4]!=mainNodeAddr[4] || carrierFlagSet.mainNodeAddr[5]!=mainNodeAddr[5]
                  		     )
                        {
                    	    carrierFlagSet.setMainNode = 1;

                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
                        	  printf("请求设置载波/无线主节点地址\n"); 
                          }
                    	  }
                    	}
                    }
                    break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
            case ACTIVE_REPORT_3762: //主动上报
             	 switch(carrierFn)
             	 {
             	 	  case 1:    //上报载波从节点信息
             	 	  	//回复确认帧
             	 	  	tmpSendBuf[0] = 0x01;    //已处理
             	 	  	tmpSendBuf[1] = 0x00;    
             	 	  	tmpSendBuf[2] = 0x05;    //超时等待1
             	 	  	tmpSendBuf[2] = 0x00;    //超时等待2
             	 	  	gdw3762Framing(ACK_OR_NACK_3762, 1, NULL, tmpSendBuf);
             	 	  	
             	 	  	//再处理收到的数据
             	 	  	tmpNode = NULL;
             	 	  	noRecordMeterHead = NULL;
             	 	  	printf("上报从节点个数=%d\n",tmpBuf[0]);
             	 	  	for(i=0; i<tmpBuf[0]; i++)
             	 	  	{
                       tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                       tmpFound->mp = 0;                       //从节点序号
                       memcpy(tmpFound->addr,tmpBuf+i*9+1,6);  //从节点地址
                       tmpFound->protocol = *(tmpBuf+i*9+1+6); //规约类型
                       tmpFound->mpNo     = *(tmpBuf+i*9+1+7) | *(tmpBuf+i*9+1+8)<<8;  //从节点序号
             	 	 	  	 tmpFound->copyTime[0]   = 0xee;
             	 	 	  	 tmpFound->copyTime[1]   = 0xee;
             	 	 	  	 tmpFound->copyEnergy[0] = 0xee;                      
                       tmpFound->next = NULL;
                       
                       tmpNode = copyCtrl[4].cpLinkHead;
                       while(tmpNode!=NULL)
                       {
                       	  if (tmpNode->collectorAddr[0]==tmpFound->addr[0] && tmpNode->collectorAddr[1]==tmpFound->addr[1]
                       	  	 && tmpNode->collectorAddr[2]==tmpFound->addr[2] && tmpNode->collectorAddr[3]==tmpFound->addr[3]
                       	  	  && tmpNode->collectorAddr[4]==tmpFound->addr[4] && tmpNode->collectorAddr[5]==tmpFound->addr[5]
                       	  	 )
                       	  {
                       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  	 {
                       	  	    printf("上报表号与已配置的测量点信息采集器地址相同\n");
                       	  	 }
                             
                             if (existMeterHead==NULL)
                             {
                     	         existMeterHead = tmpFound;
                             }
                             else
                             {
                     	         prevExistFound->next = tmpFound;
                             }
                             prevExistFound = tmpFound;
                             
                       	  	 break;
                       	  }
                       	  
                       	  if (tmpNode->addr[0]==tmpFound->addr[0] && tmpNode->addr[1]==tmpFound->addr[1]
                       	  	 && tmpNode->addr[2]==tmpFound->addr[2] && tmpNode->addr[3]==tmpFound->addr[3]
                       	  	  && tmpNode->addr[4]==tmpFound->addr[4] && tmpNode->addr[5]==tmpFound->addr[5]
                       	  	 )
                       	  {
                       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  	 {
                       	  	   printf("上报表号与已配置的测量点信息载波表地址相同\n");
                       	  	 }

                             if (existMeterHead==NULL)
                             {
                     	         existMeterHead = tmpFound;
                             }
                             else
                             {
                     	         prevExistFound->next = tmpFound;
                             }
                             prevExistFound = tmpFound;
                       	  	 break;
                       	  }

                       	  tmpNode = tmpNode->next;
                       }
                       
                       tmpQueryFound = foundMeterHead;
                       while(tmpQueryFound!=NULL)
                       {
                       	  if (tmpQueryFound->addr[0]==tmpFound->addr[0] && tmpQueryFound->addr[1]==tmpFound->addr[1]
                       	  	 && tmpQueryFound->addr[2]==tmpFound->addr[2] && tmpQueryFound->addr[3]==tmpFound->addr[3]
                       	  	  && tmpQueryFound->addr[4]==tmpFound->addr[4] && tmpQueryFound->addr[5]==tmpFound->addr[5]
                       	  	 )
                       	  {
                       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  	 {
                       	  	   printf("上报表号与已上报的表号地址相同\n");
                       	  	 }
                       	  	 
                       	  	 break;
                       	  }
                       	  
                       	  tmpQueryFound = tmpQueryFound->next;
                       }
                       
                       if (tmpNode==NULL && tmpQueryFound==NULL)
                       {
                         if (foundMeterHead==NULL)
                         {
                     	     foundMeterHead = tmpFound;
                         }
                         else
                         {
                     	     prevFound->next = tmpFound;
                         }
                         prevFound = tmpFound;
                         
                         if (noRecordMeterHead==NULL)
                         {
                         	  noRecordMeterHead = tmpFound;
                         }
                      }
             	 	  	}
             	 	  	
             	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
             	 	  	{
             	 	  	  printf("主动上报的表号信息:\n");
             	 	  	  tmpFound = foundMeterHead;
             	 	  	  while(tmpFound!=NULL)
             	 	  	  {
        	  		 	 	    printf("上报节点地址:%02x%02x%02x%02x%02x%02x\n",tmpFound->addr[5],tmpFound->addr[4],tmpFound->addr[3],tmpFound->addr[2],tmpFound->addr[1],tmpFound->addr[0]);
        	  		 	 	    printf("上报节点规约:%d\n",tmpFound->protocol);
        	  		 	 	    printf("上报节点序号:%d\n",tmpFound->mpNo);
             	 	  		 
             	 	  		  tmpFound = tmpFound->next;
             	 	  	  }
             	 	    }
             	 	  	
             	 	  	if (tmpNode==NULL)
                    {
             	 	  	  searchMeterReport();   //搜索结果报告(LCD显示)
             	 	  	}             	 	  	

                    if (noRecordMeterHead!=NULL)
                    {
                      //发现电表记录
                      foundMeterEvent(noRecordMeterHead);
                    }
             	 	  	break;
             	 	  	
             	 	  case 2:    //上报抄读数据
             	 	  	//回复确认帧
             	 	  	tmpSendBuf[0] = 0x01;    //已处理
             	 	  	tmpSendBuf[1] = 0x00;    
             	 	  	tmpSendBuf[2] = 0x02;    //超时等待1
             	 	  	tmpSendBuf[3] = 0x00;    //超时等待2
             	 	  	tmpSendBuf[4] = tmpBuf[tmpBuf[3]+4]&0xf;    //相位,2013-12-24,add
             	 	  	gdw3762Framing(ACK_OR_NACK_3762, 1, NULL, tmpSendBuf);
             	 	  	
                    if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	          {
    	  	   	        if (carrierModuleType==TC_CARRIER)
    	  	   	        {
    	  	   	          printf("路由主导抄读:上报抄读数据,数据长度=%d,回复相位=%d\n",tmpBuf[3],tmpBuf[tmpBuf[3]+4]);
    	  	   	        }
    	  	   	        else
    	  	   	        {
    	  	   	          printf("路由主导抄读:上报抄读数据,数据长度=%d\n",tmpBuf[3]);
    	  	   	        }
    	  	          }
    	  	          
    	  	          //东软采集器有地址方式,要做报文交换
    	  	          if (carrierModuleType==EAST_SOFT_CARRIER)
    	  	          {
    	  	          	if (copyCtrl[4].tmpCpLink!=NULL)
    	  	          	{
    	  	          		if (compareTwoAddr(copyCtrl[4].tmpCpLink->collectorAddr, 0, 1)==FALSE)
    	  	          		{
    	  	          			if (compareTwoAddr(copyCtrl[4].tmpCpLink->collectorAddr, &tmpBuf[5], 0)==TRUE)
    	  	          			{
    	  	          				eastMsgSwap(&tmpBuf[4], &tmpBuf[3]);
    	  	          			}
    	  	          		}
    	  	          	}
    	  	          }
    	  	          
    	  	          //再处理收到的数据
    	  	          if (carrierModuleType==TC_CARRIER)
    	  	          {
             	 	  	  meter485Receive(PORT_POWER_CARRIER+tmpBuf[tmpBuf[3]+4]-1, &tmpBuf[4], tmpBuf[3]);
    	  	          }
    	  	          else
    	  	          {
             	 	  	  meter485Receive(PORT_POWER_CARRIER, &tmpBuf[4], tmpBuf[3]);
             	 	  	}
             	 	  	break;
             	 }
             	 break;
     	  		 
     	  		case ROUTE_QUERY_3762:   //路由查询
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	  case 1:    //查询从节点数量
     	  		 	 	  	if (carrierModuleType==FC_WIRELESS)  //2012-08-10,modify
     	  		 	 	  	{
     	  		 	 	  	  carrierFlagSet.numOfSalveNode = tmpBuf[2] | tmpBuf[3]<<8;
     	  		 	 	  	}
     	  		 	 	  	else
     	  		 	 	  	{
     	  		 	 	  	  carrierFlagSet.numOfSalveNode = tmpBuf[0] | tmpBuf[1]<<8;
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	carrierFlagSet.querySlaveNum = 0;
     	  		 	 	  	
     	  		 	 	  	if (carrierFlagSet.numOfSalveNode==0)
     	  		 	 	  	{
     	  		 	 	  		 carrierFlagSet.querySlaveNode = 2;
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  	{
     	  		 	 	  		 printf("本地通信模块内从节点数量为%d\n",carrierFlagSet.numOfSalveNode);
     	  		 	 	  	}
     	  		 	 	  	break;
     	  		 	 	  	
     	  		 	 	  case 2:    //查询载波从节点信息
     	  		 	 	  	if (carrierModuleType!=SC_WIRELESS)
     	  		 	 	  	{
     	  		 	 	  	  carrierFlagSet.numOfSalveNode = tmpBuf[0] | tmpBuf[1]<<8;
     	  		 	 	  	}
                    if (tmpBuf[2]==0)
                    {
     	  		 	 	  	   if (carrierFlagSet.querySlaveNode==1)
     	  		 	 	  	   {
   	                      carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
   	                   
   	                      if (debugInfo&PRINT_CARRIER_DEBUG)
   	                      {
   	                        printf("载波模块从节点列表(路由查询)\n");
   	                        tmpSalveNode = carrierSlaveNode;
   	                        while(tmpSalveNode!=NULL)
   	                        {
     	  		 	 	  	          printf("从节点地址:%02x%02x%02x%02x%02x%02x\n",tmpSalveNode->addr[5],tmpSalveNode->addr[4],tmpSalveNode->addr[3],tmpSalveNode->addr[2],tmpSalveNode->addr[1],tmpSalveNode->addr[0]);
     	  		 	 	  	          tmpSalveNode = tmpSalveNode->next;
   	                        }
   	                      }
     	  		 	 	  	   }
                    }
                    else
                    {
                       for(i=0;i<tmpBuf[2];i++)
                       {
        	  		 	 	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
        	  		 	 	  	 {
        	  		 	 	  	   printf("从节点地址:%02x%02x%02x%02x%02x%02x\n",tmpBuf[i*8+8],tmpBuf[i*8+7],tmpBuf[i*8+6],tmpBuf[i*8+5],tmpBuf[i*8+4],tmpBuf[i*8+3]);
        	  		 	 	  	 }
        	  		 	 	  	 
        	  		 	 	  	 //载波从节点地址为全0,则认为不再有后续从节点地址,停止读取
                         if (tmpBuf[i*8+8]==0x0 && tmpBuf[i*8+7]==0x0 && tmpBuf[i*8+6]==0x0 && tmpBuf[i*8+5]==0x0 && tmpBuf[i*8+4]==0x0 && tmpBuf[i*8+3]==0x0)
                         {
                         	 if (localModuleType==CEPRI_CARRIER_3_CHIP)
                         	 {
                         	   if (carrierFlagSet.synMeterNo>1)
                         	   {
                               carrierFlagSet.numOfSalveNode = carrierFlagSet.synMeterNo;
                         	   }
                         	   else
                         	   {
                               carrierFlagSet.numOfSalveNode = 0;
                         	   }
                         	 }
                         	 else
                         	 {
                         	   carrierFlagSet.numOfSalveNode = carrierFlagSet.synMeterNo;
                         	 }
     	  		 	 	  	       
     	  		 	 	  	       if (carrierFlagSet.querySlaveNode==1)
     	  		 	 	  	       {
   	                         carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
   	                   
   	                         if (debugInfo&PRINT_CARRIER_DEBUG)
   	                         {
   	                           printf("结束读载波模块从节点列表,因返回的从节点地址为全0\n");
   	                         }
     	  		 	 	  	       }
     	  		 	 	  	       
     	  		 	 	  	       break;
                         }
                         
                         tmpSalveNode = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                         tmpSalveNode->mpNo = carrierFlagSet.synMeterNo;           //从节点序号
                         memcpy(tmpSalveNode->addr,tmpBuf+i*8+3,6); //从节点地址
                         memcpy(tmpSalveNode->info,tmpBuf+i*8+9,2); //从节点信息
                         tmpSalveNode->next = NULL;
                         if (carrierSlaveNode==NULL)
                         {
                       	   carrierSlaveNode = tmpSalveNode;
                         }
                         else
                         {
                       	   prevSlaveNode->next = tmpSalveNode;
                         }
                         prevSlaveNode = tmpSalveNode;
      	                 carrierFlagSet.synMeterNo++;
                       }
                       
                       //如果返回从节点的个数大于等于从节点总数量,停止读取
                       if (carrierFlagSet.synMeterNo > carrierFlagSet.numOfSalveNode)
                       {
   	                      carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
   	                   
   	                      if (debugInfo&PRINT_CARRIER_DEBUG)
   	                      {
   	                        printf("返回从节点的个数大于等于从节点总数量,停止读取\n");
   	                      }
                       }
                    }                    
     	  		 	 	  	break;
     	  		 	 	  	
     	  		 	 	  case 4:    //路由运行状态
     	  		 	 	  	carrierFlagSet.numOfSalveNode = tmpBuf[1] | tmpBuf[2]<<8;
     	  		 	 	  	
     	  		 	 	  	if (carrierModuleType==SR_WIRELESS)
     	  		 	 	  	{
   	  		 	 	  		  if (tmpBuf[0]==0x01)
   	  		 	 	  		  {
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                        	printf("SR - 组网完成\n");
                        }
                        
                        showInfo("本地通信模块组网完成");
                        
                        carrierFlagSet.wlNetOk = 3;
                        
                        //清除以前的未入网节点信息
                        while (noFoundMeterHead!=NULL)
                        {
                          tmpFound = noFoundMeterHead;
                          noFoundMeterHead = noFoundMeterHead->next;
                            
                          free(tmpFound);
                        }
                              
                 	      //读未入网节点
                 	      gdw3762Framing(RL_EXTEND_3762, 4, NULL, NULL);
                        carrierFlagSet.cmdType = CA_CMD_READ_NO_NET_NODE;
   	  		 	 	  		  }
   	  		 	 	  		  else
   	  		 	 	  		  {
     	  		 	 	  		  showInfo("本地通信模块组网中..");

     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
                        	printf("SR - 组网未完成\n");
     	  		 	 	  		    printf("SR - 从节点数量=%d\n", carrierFlagSet.numOfSalveNode);
     	  		 	 	  		    printf("SR - 入网节点数量=%d\n", tmpBuf[3] | tmpBuf[4]<<8);
     	  		 	 	  		  }
     	  		 	 	  		  
     	  		 	 	  		  if (carrierFlagSet.numOfSalveNode==(tmpBuf[3] | tmpBuf[4]<<8))
     	  		 	 	  		  {
     	  		 	 	  		  	if (localModuleType!=SR_WF_3E68)
     	  		 	 	  		  	{
     	  		 	 	  		  	  if (carrierFlagSet.numOfSalveNode!=0)
     	  		 	 	  		  	  {
     	  		 	 	  		  	    carrierFlagSet.wlNetOk = 3;
     	  		 	 	  		  	  }
                     
                            //清除以前的未入网节点信息
                            while (noFoundMeterHead!=NULL)
                            {
                              tmpFound = noFoundMeterHead;
                              noFoundMeterHead = noFoundMeterHead->next;
                         
                              free(tmpFound);
                            }
     	  		 	 	  		  	}
                     
     	  		 	 	  		  	if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  	{
     	  		 	 	  		      if (carrierFlagSet.numOfSalveNode!=0)
     	  		 	 	  		      {
     	  		 	 	  		        printf("SR - 所有节点均入网\n");
     	  		 	 	  		      }
     	  		 	 	  		  	}
     	  		 	 	  		  }
   	  		 	 	  		    
   	  		 	 	  		    //如果本次查询的入网节点数量与上一次的不同,则组网超时时间向后推移3分钟
   	  		 	 	  		    if ((carrierFlagSet.lastNumInNet != (tmpBuf[3] | tmpBuf[4]<<8))
   	  		 	 	  		    	 && (carrierFlagSet.wlNetOk!= 3)
   	  		 	 	  		    	 )
   	  		 	 	  		    {
   	  		 	 	  		  	  if (localModuleType==SR_WF_3E68)
   	  		 	 	  		  	  {
   	  		 	 	  		  	    carrierFlagSet.netOkTimeOut = nextTime(sysTime, SRWF_NET_OK_TIME_OUT, 0);
   	  		 	 	  		  	
   	  		 	 	  		  	    if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  		  	    {
   	  		 	 	  		  		     printf("SR - 本次查询的入网节点数量与上一次的不同,则组网超时时间向后推移%d分钟\n", SRWF_NET_OK_TIME_OUT);
   	  		 	 	  		  	    }
   	  		 	 	  		  	  }
   	  		 	 	  		  	  else
   	  		 	 	  		  	  {
   	  		 	 	  		  	    carrierFlagSet.netOkTimeOut = nextTime(sysTime, 3, 0);
   	  		 	 	  		  	
   	  		 	 	  		  	    if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  		  	    {
   	  		 	 	  		  		     printf("SR - 本次查询的入网节点数量与上一次的不同,则组网超时时间向后推移3分钟\n");
   	  		 	 	  		  	    }
   	  		 	 	  		  	  }
   	  		 	 	  		    }
  
     	  		 	 	  		  //上一次查询入网节点数量
     	  		 	 	  		  carrierFlagSet.lastNumInNet = tmpBuf[3] | tmpBuf[4]<<8;
     	  		 	 	  		}
     	  		 	 	  	}
     	  		 	 	  	else
     	  		 	 	  	{
       	  		 	 	  	if (tmpBuf[0]&0x2)
       	  		 	 	  	{
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("正在工作\n");
       	  		 	 	  	  }
       	  		 	 	  	}
       	  		 	 	  	else
       	  		 	 	  	{
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("停止工作\n");
       	  		 	 	  		}
       	  		 	 	  		
     	  		 	 	  		  if (tmpBuf[0]&0x1)
     	  		 	 	  		  {
     	  		 	 	  			   if (carrierFlagSet.studyRouting==2)
     	  		 	 	  			   {
     	  		 	 	  			     carrierFlagSet.studyRouting = 0;   //停止学习路由
     	  		 	 	  			   
     	  		 	 	  			     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  			     {
     	  		 	 	  			       printf("停止路由学习\n");
     	  		 	 	  			     }
     	  		 	 	  			   }
       	  		 	 	  		   
       	  		 	 	  		   //如果在搜表,路由已完成的话,可以停止搜表ly,10-08-27,适应晓程加的
       	  		 	 	  		   //ly,2011-06-07,改成搜表标志置为0x08
       	  		 	 	  		   if (carrierFlagSet.searchMeter!=0)
       	  		 	 	  		   {
       	  		 	 	  		      if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		      {
       	  		 	 	  		        printf("本地通信模块已停止搜表\n");
       	  		 	 	  		      }
              	 	 		
       	  		 	 	  		  	  carrierFlagSet.searchMeter = 0x00;  //模块已停止搜表
       	  		 	 	  		 
              	  		 	 	  	if (menuInLayer==0)
              	  		 	 	  	{
              	  		 	 	  		defaultMenu();
              	  		 	 	  	}
                              searchMeter(0xff);
  
       	  		 	 	  		  	  //carrierFlagSet.foundStudyTime = sysTime;  //ly,2011-06-07,注释
                              //ly,2011-12-02,普光王立政反应这种复位效果对晓程较好,所以又打开了重启
                              if (carrierModuleType==CEPRI_CARRIER)
                              {
                              	 //ly,2012-02-28,添加这个判断
                              	 if (localModuleType!=CEPRI_CARRIER_3_CHIP)
                              	 {
                              	   if (debugInfo&PRINT_CARRIER_DEBUG)
                              	   {
                              	 	   printf("电科院模块:(接收线程)自动搜完表后重启模块\n");
                              	   }
                              	 
                              	   sleep(1);
                              	 
                              	   ifReset=TRUE;
                              	 }
                              }
                              
                              if (carrierFlagSet.routeLeadCopy==1)
                              {
                                 copyCtrl[4].nextCopyTime = nextTime(sysTime, 2, 0);    //2分钟以后再启动抄表
                              }
       	  		 	 	  		   }
     	  		 	 	  		  }
       	  		 	 	  	}
       	  		 	 	  	
       	  		 	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  	{
        	  		 	 	  	 if (tmpBuf[0]&0x1)
        	  		 	 	  	 {
        	  		 	 	  		 printf("学习路由完成\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("学习路由未完成\n");
        	  		 	 	  	 }
        	  		 	 	  	 if (tmpBuf[0]&0x4)
        	  		 	 	  	 {
        	  		 	 	  		 printf("有从节点上报事件\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("无从节点上报事件\n");
        	  		 	 	  	 }
        	  		 	 	  	
        	  		 	 	  	 if (tmpBuf[7]&0x1)
        	  		 	 	  	 {
        	  		 	 	  		 printf("工作状态:学习\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("工作状态:抄表\n");
        	  		 	 	  	 }
        	  		 	 	  	 if (tmpBuf[7]&0x2)
        	  		 	 	  	 {
        	  		 	 	  		 printf("工作状态:允许注册\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("工作状态:不允许注册\n");
        	  		 	 	  	 }
       	  		 	 	  	}
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	if (1==carrierFlagSet.routeLeadCopy && carrierFlagSet.workStatus==3 && TRUE==copyCtrl[4].meterCopying)
     	  		 	 	  	{
 	                    if (debugInfo&PRINT_CARRIER_DEBUG)
 	                    {
   	                    printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:抄表中,模块回复查询路由状态命令,说明与路由通信正常,请求超时向后推移%d分钟\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,ROUTE_REQUEST_OUT);
 	                    }
     	  		 	 	  		
     	  		 	 	  		//路由请求超时时间向后推移
                      carrierFlagSet.routeRequestOutTime = nextTime(sysTime, ROUTE_REQUEST_OUT, 0);
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	carrierFlagSet.routeRunStatus = 0;
     	  		 	 	 	  break;
     	  		 	 	 	  
     	  		 	 	 	case 6:   //主动注册的载波从节点信息
     	  		 	 	 		carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);  //10秒后再查询
     	  		 	 	 		
             	 	  	tmpNode = NULL;
             	 	  	noRecordMeterHead = NULL;
             	 	  	carrierFlagSet.activeMeterNo += tmpBuf[2];
             	 	  	for(i=0;i<tmpBuf[2];i++)
             	 	  	{
                       tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                       tmpFound->mp = 0;                           //
                       memcpy(tmpFound->addr,tmpBuf+i*8+3,6);      //从节点地址
                       tmpFound->info[0]     = *(tmpBuf+i*8+3+6);  //从节点信息1
                       tmpFound->info[1]     = *(tmpBuf+i*8+3+7);  //从节点信息2
             	 	 	  	 tmpFound->copyTime[0] = 0xee;
             	 	 	  	 tmpFound->copyTime[1] = 0xee;
             	 	 	  	 tmpFound->copyEnergy[0] = 0xee;                      
                       tmpFound->next = NULL;
                       
                       tmpNode = copyCtrl[2].cpLinkHead;
                       while(tmpNode!=NULL)
                       {
                       	  if (tmpNode->collectorAddr[0]==tmpFound->addr[0] && tmpNode->collectorAddr[1]==tmpFound->addr[1]
                       	  	 && tmpNode->collectorAddr[2]==tmpFound->addr[2] && tmpNode->collectorAddr[3]==tmpFound->addr[3]
                       	  	  && tmpNode->collectorAddr[4]==tmpFound->addr[4] && tmpNode->collectorAddr[5]==tmpFound->addr[5]
                       	  	 )
                       	  {
                       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  	 {
                       	  	   printf("上报表号与已配置的测量点信息采集器地址相同\n");
                       	  	 }
                       	  	 
                       	  	 break;
                       	  }
                       	  
                       	  if (tmpNode->addr[0]==tmpFound->addr[0] && tmpNode->addr[1]==tmpFound->addr[1]
                       	  	 && tmpNode->addr[2]==tmpFound->addr[2] && tmpNode->addr[3]==tmpFound->addr[3]
                       	  	  && tmpNode->addr[4]==tmpFound->addr[4] && tmpNode->addr[5]==tmpFound->addr[5]
                       	  	 )
                       	  {
                       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  	 {
                       	  	   printf("上报表号与已配置的测量点信息载波表地址相同\n");
                       	  	 }
                       	  	 
                       	  	 break;
                       	  }

                       	  tmpNode = tmpNode->next;
                       }
                       
                       if (tmpNode==NULL)
                       {
                         if (foundMeterHead==NULL)
                         {
                     	     foundMeterHead = tmpFound;
                         }
                         else
                         {
                     	     prevFound->next = tmpFound;
                         }
                         prevFound = tmpFound;
                         
                         if (noRecordMeterHead==NULL)
                         {
                         	  noRecordMeterHead = tmpFound;
                         }
                      }
             	 	  	}
             	 	  	
             	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
             	 	  	{
             	 	  	  printf("主动注册的载波从节点信息:\n");
             	 	  	  tmpFound = foundMeterHead;
             	 	  	  while(tmpFound!=NULL)
             	 	  	  {
        	  		 	 	    printf("上报节点地址:%02x%02x%02x%02x%02x%02x\n",tmpFound->addr[5],tmpFound->addr[4],tmpFound->addr[3],tmpFound->addr[2],tmpFound->addr[1],tmpFound->addr[0]);
             	 	  		 
             	 	  		  tmpFound = tmpFound->next;
             	 	  	  }
             	 	  	}
             	 	  	
             	 	  	if (tmpNode==NULL)
                    {
             	 	  	  searchMeterReport();   //搜索结果报告(LCD显示)
             	 	  	}
                    
                    if (noRecordMeterHead!=NULL)
                    {
                       //发现电表记录
                       foundMeterEvent(noRecordMeterHead);
                    }
     	  		 	 	 		break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
     	  		case DATA_FORWARD_3762:        //数据转发
     	  		case ROUTE_DATA_FORWARD_3762:  //路由数据转发
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	  case 1:   //监控载波从节点回复
     	  		 	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	    {
     	  		 	 	  	  if (carrierAfn==DATA_FORWARD_3762)
     	  		 	 	  	  {
   	                    printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfCarrierReceive,无线模块数据转发回复\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	  		 	 	  	  }
     	  		 	 	  	  else
     	  		 	 	  	  {
   	                    printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfCarrierReceive,监控载波从节点回复\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	  		 	 	  	  }
     	  		 	 	    }
     	  	          
     	  	          //ly,2011-05-27,适应弥亚微
     	  	          carrierFlagSet.numOfCarrierSending = 0;

                   #ifdef LIGHTING
                    //执行搜索LDDT命令在前,执行点抄和转发命令在后,2015-06-03,add
                    if (
                    	  1==carrierFlagSet.searchLddt
                    	   || 1==carrierFlagSet.searchOffLine
                    	    || 1==carrierFlagSet.searchLddStatus
                    	 )
                    {
                    	if (tmpBuf[1]==0)
       	  	          {
       	  	            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 1);
       	  	             	 	 
       	  	            if (debugInfo&PRINT_CARRIER_DEBUG)
       	  	            {
       	  	             	printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
       	  	             	if (1==carrierFlagSet.searchLddt)
       	  	             	{
       	  	             	  printf("搜索LDDT[1]");
       	  	             	}
       	  	             	else
       	  	             	{
       	  	             	  if (1==carrierFlagSet.searchOffLine)
       	  	             	  {
       	  	             	    printf("搜索离线单灯控制器[1]");
       	  	             	  }
       	  	             	  else
       	  	             	  {
       	  	             	  	printf("查询刚上电单灯控制器状态[1]");
       	  	             	  }
       	  	             	}
       	  	             	printf(",从节点回复0字节\n");
       	  	            }
       	  	          }
       	  	          else
       	  	          {
                        meter485Receive(PORT_POWER_CARRIER, &tmpBuf[2], tmpBuf[1]);
                      }
                    }
                    else
                    {
                   #endif
                      
                      if (copyCtrl[4].pForwardData!=NULL)    //正在转发数据
     	  		 	 	  	  {
       	  	            //2015-10-19,add
       	  	            copyCtrl[4].copyTimeOut = sysTime;
       	  	            
       	  	            //2012-08-20,add
       	  	            if (tmpBuf[1]==0)
       	  	            {
                          //2013-12-25,修改这个判断内的处理,原来的处理是置之不理,忽略
                          //    现在的处理也算是从节点的回复,只不过没抄到数据而已
                         
                          forwardDataReply(4);    //2014-01-03,add
                         
                          if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
                          {
                            carrierFlagSet.workStatus = 6;
                            carrierFlagSet.reStartPause = 3;
                            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
                          }
                 
                          free(copyCtrl[4].pForwardData);
                          copyCtrl[4].pForwardData = NULL;

                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
                      	    printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:转发,从节点回复0字节", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                      	    if (copyCtrl[4].meterCopying==TRUE)
                      	    {
                      	      printf(",等待%d秒钟后恢复抄表\n", WAIT_RECOVERY_COPY);
                      	    }
                      	    else
                      	    {
                      	   	  printf("\n");
                      	    }
                          }
                         
                          break;
       	  	            }

     	  		 	 	  		  if (copyCtrl[4].pForwardData->fn==0x09)
     	  		 	 	  		  {
                          //ly,2012-8-20,修改这个错误,现在就是4了
                          //forwardReceive(3, &tmpBuf[2], tmpBuf[1]); 
                          forwardReceive(4, &tmpBuf[2], tmpBuf[1]);
     	  		 	 	  		  }
     	  		 	 	  		  else
     	  		 	 	  		  {  
                          if (copyCtrl[4].pForwardData->fn==0x01)  //透明转发
                          {
                      	    memcpy(copyCtrl[4].pForwardData->data, &tmpBuf[2], tmpBuf[1]);
                     	      
                     	      copyCtrl[4].pForwardData->length = tmpBuf[1];
                     	      copyCtrl[4].pForwardData->forwardResult = RESULT_HAS_DATA;
                            copyCtrl[4].pForwardData->nextBytesTime = sysTime;
                          }
                          else
                          {
                            meter485Receive(PORT_POWER_CARRIER, &tmpBuf[2], tmpBuf[1]);
                          }
     	  		 	 	  		  }
     	  		 	 	  	  }
     	  		 	 	  	  else
     	  		 	 	  	  {
     	  		 	 	  	    if (pDotCopy!=NULL || copyCtrl[4].meterCopying==TRUE)
     	  		 	 	  	    {
          	  	          //东软采集器有地址方式,要做报文交换
          	  	          if (carrierModuleType==EAST_SOFT_CARRIER)
          	  	          {
          	  	            if (copyCtrl[4].ifCollector==1)
          	  	            {
          	  	          	  if (compareTwoAddr(carrierFlagSet.destAddr, &tmpBuf[3], 0)==TRUE)
          	  	          	  {
          	  	          		  eastMsgSwap(&tmpBuf[2], &tmpBuf[1]);
          	  	          	  }
          	  	            }
          	  	          }
       	  	             
       	  	             #ifdef LIGHTING
       	  	              if (tmpBuf[1]==0)
       	  	              {
       	  	             	  if (
       	  	             	  	  1==carrierFlagSet.searchLddt
       	  	             	  	   || 1==carrierFlagSet.searchOffLine
       	  	             	  	    || 2==carrierFlagSet.searchLddStatus
       	  	             	  	 )
       	  	             	  {
       	  	             	 	  copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 1);
       	  	             	 	 
       	  	             	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
       	  	             	 	  {
       	  	             	 	 	  printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读:", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
       	  	             	 	 	  if (1==carrierFlagSet.searchLddt)
       	  	             	 	 	  {
       	  	             	 	 	    printf("搜索LDDT[2]\n");
       	  	             	 	 	  }
       	  	             	 	 	  else
       	  	             	 	 	  {
       	  	             	 	 	    if (1==carrierFlagSet.searchOffLine)
       	  	             	 	 	    {
       	  	             	 	 	      printf("搜索离线单灯控制器[2]\n");
       	  	             	 	 	    }
       	  	             	 	 	    else
       	  	             	 	 	    {
       	  	             	 	 	      printf("查询刚上电单灯控制器状态[2]\n");
       	  	             	 	 	    }
       	  	             	 	 	  }
       	  	             	 	 	  printf(",从节点回复0字节\n");
       	  	             	 	  }
       	  	             	  }
       	  	              }
       	  	             #endif
       	  	             
     	  		 	 	  	      meter485Receive(PORT_POWER_CARRIER, &tmpBuf[2], tmpBuf[1]);
     	  		 	 	  	    }
     	  		 	 	  	    else
     	  		 	 	  	    {
     	  		 	 	  	   	  #ifdef PRINT_CARRIER_DEBUG
     	  		 	 	  	   	   printf("数据转发回复重复帧,丢弃\n");
     	  		 	 	  	   	  #endif
     	  		 	 	  	    }
     	  		 	 	  	  }
     	  		 	 	  	  
     	  		 	 	   #ifdef LIGHTING
     	  		 	 	  	}
      	  		 	 	 #endif
     	  		 	 	  	break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
     	  		case ROUTE_DATA_READ_3762:     //路由数据抄读
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	 case 1:   //路由数据抄读内容
                   if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         {
    	  	   	       printf("路由主导抄读:请求抄读,");
    	  	         }

                   if (carrierModuleType==TC_CARRIER)  //鼎信载波
                   {
                     //只处理相位A、B、C(对应1、2、3)
                     if (tmpBuf[0]>0 && tmpBuf[0]<4)
                     {
                       copyCtrl[tmpBuf[0]+3].copyContinue = TRUE;
     	  		 	 	       memcpy(copyCtrl[tmpBuf[0]+3].needCopyMeter, &tmpBuf[1], 6);
                       
                       if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	             {
    	  	   	           printf("端口%d", tmpBuf[0]+3+1);
    	  	             }
                     }
                   }
                   else                                //其它载波(东软、弥亚微)
                   {
     	  		 	 	     memcpy(copyCtrl[4].needCopyMeter, &tmpBuf[1], 6);
                   	 copyCtrl[4].copyContinue = TRUE;
                     
                     if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           {
    	  	   	         printf("端口5");
    	  	           }
                   }
                   
                   //路由请求超时时间向后推移
                   carrierFlagSet.routeRequestOutTime = nextTime(sysTime, ROUTE_REQUEST_OUT, 0);
                   
                   if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         {
    	  	   	       printf(",表地址:%02x%02x%02x%02x%02x%02x,相位=%d\n",
    	  	   	              tmpBuf[6],tmpBuf[5],tmpBuf[4],tmpBuf[3],tmpBuf[2],tmpBuf[1], tmpBuf[0]);
    	  	         }
     	  		 	 	   break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
            case RL_EXTEND_3762:           //锐拔扩展命令
             	 switch(carrierFn)
             	 {
             	 	 case 4:    //读未组网节点号
             	 	 	 if (debugInfo&PRINT_CARRIER_DEBUG)
             	 	 	 {
             	 	 	   printf("无线模块  - 未组网节点号\n");
             	 	 	   printf("数量=%d\n",tmpBuf[0]|tmpBuf[1]<<8);
             	 	 	 }
             	 	  
             	 	   tmpNode = NULL;
             	 	   for(i=0;i<tmpBuf[0]|tmpBuf[1]<<8;i++)
             	 	   {
                       tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                       tmpFound->mp = 0;
                       memcpy(tmpFound->addr,tmpBuf+i*6+2,6); //从节点地址
                       
                       if (debugInfo&PRINT_CARRIER_DEBUG)
                       {
                         printf("%02x%02x%02x%02x%02x%02x\n", *(tmpBuf+i*6+7), *(tmpBuf+i*6+6),*(tmpBuf+i*6+5),*(tmpBuf+i*6+4),*(tmpBuf+i*6+3),*(tmpBuf+i*6+2));
                       }
                       
                       tmpFound->info[0]       = 0x0;         //从节点信息1
                       tmpFound->info[1]       = 0x0;         //从节点信息2
             	 	 	  	 tmpFound->copyTime[0]   = 0xee;
             	 	 	  	 tmpFound->copyTime[1]   = 0xee;
             	 	 	  	 tmpFound->copyEnergy[0] = 0xee;                      
                       tmpFound->next = NULL;
                       if (tmpNode==NULL)
                       { 
                         if (noFoundMeterHead==NULL)
                         {
                     	     noFoundMeterHead = tmpFound;
                         }
                         else
                         {
                     	     prevFound->next = tmpFound;
                         }
                         prevFound = tmpFound;
                       }
             	 	   }
             	 	  	
             	 	   if (debugInfo&PRINT_CARRIER_DEBUG)
             	 	   {
             	 	     printf("未组网的节点信息:\n");
             	 	     tmpFound = noFoundMeterHead;
             	 	     while(tmpFound!=NULL)
             	 	     {
        	  		 	 	   printf("节点地址:%02x%02x%02x%02x%02x%02x\n",tmpFound->addr[5],tmpFound->addr[4],tmpFound->addr[3],tmpFound->addr[2],tmpFound->addr[1],tmpFound->addr[0]);
             	 	  		 
             	 	  	   tmpFound = tmpFound->next;
             	 	     }
             	 	   }
             	 	 	 break;
             	 	 	 
             	 	 case 6:    //读网络状态
             	 	 	 printf("RL无线模块  - 网络状态\n");
             	 	 	 
             	 	 	 if (tmpBuf[0]==0x03)
             	 	 	 {
             	 	 	 	 printf("RL无线模块:自由组网(不受下发的节点号的限制)\n");
             	 	 	 }
             	 	 	 else
             	 	 	 {
             	 	 	 	 printf("RL无线模块:按下发的节点号组网\n");
             	 	 	 }
             	 	 	 
             	 	 	 if (tmpBuf[1]==0x0e)
             	 	 	 {
             	 	 	 	 printf("RL无线模块:组网结束\n");
             	 	 	 	 
             	 	 	 	 carrierFlagSet.wlNetOk++;
             	 	 	 }
             	 	 	 else
             	 	 	 {
             	 	 	 	 printf("RL无线模块:正在组网\n");
             	 	 	 }
             	 	 	 
             	 	 	 printf("RL无线模块:已组入RobuNet网的节点数量:%d\n", tmpBuf[3]<<8 | tmpBuf[2]);
             	 	 	 printf("RL无线模块:无线Modem内下发的电表数量:%d\n", tmpBuf[5]<<8 | tmpBuf[4]);
             	 	 	 if (tmpBuf[6]==0x99 && tmpBuf[7]==0x99)
             	 	 	 {
             	 	 	 	 printf("RL无线模块:9999,在此期间无法抄表\n");
             	 	 	 }
             	 	 	 else
             	 	 	 {
             	 	 	 	 printf("RL无线模块:未组入Robunet节点的数量=%d\n",tmpBuf[7]<<8 | tmpBuf[6]);
             	 	 	 }
             	 	 	 //printf("RL无线模块:已组入Robunet网的节点的最大跳数:%02x\n", tmpBuf[8]);             	 	 	 
             	 	 	 break;
             	 }
               break;
               
     	  		case DEBUG_3762:
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	   case 1:    //主动上报功能控制
     	  		 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	     {
     	  		 	      if (tmpBuf[0]==1)
     	  		 	      {
     	  		 	      	 printf("无线模块 - 主动上报功能使能\n");
     	  		 	      }
     	  		 	      else
     	  		 	      {
     	  		 	      	 printf("无线模块 - 主动上报功能禁止\n");
     	  		 	      }
     	  		 	     }
     	  		 	     
     	  		 	     carrierFlagSet.innerDebug = 1;
     	  		 	     break;
     	  		 	     
     	  		 	   case 3:    //上报建网成功
     	  		 	   	 printf("无线自组网 - 建网成功\n");     	  		 	   	 
     	  		 	   	 if (tmpBuf[0]==1)
     	  		 	   	 {
     	  		 	   	 	 printf("建网模式   - 验证模式\n");
     	  		 	   	 }
     	  		 	   	 else
     	  		 	   	 {
     	  		 	   	 	 printf("建网模式   - 非验证模式\n");     	  		 	   	 	 
     	  		 	   	 }     	  		 	   	 
     	  		 	   	 printf("建网信道   - %d\n", tmpBuf[1]);
     	  		 	   	 printf("心跳周期   - %d\n", tmpBuf[2] | tmpBuf[3]<<8);
     	  		 	   	 printf("网络PanID  - %d\n", tmpBuf[4] | tmpBuf[5]<<8);
     	  		 	   	 printf("主节点固定地址 - %02x%02x%02x%02x%02x%02x\n",tmpBuf[11],tmpBuf[10],tmpBuf[9],tmpBuf[8],tmpBuf[7],tmpBuf[6]);
     	  		 	   	 printf("主节点网内固定地址 - %02x%02x%02x%02x%02x%02x%02x%02x\n",tmpBuf[19],tmpBuf[18],tmpBuf[17],tmpBuf[16],tmpBuf[15],tmpBuf[14],tmpBuf[13],tmpBuf[12]);
     	  		 	   	 break;
     	  		 	   	 
     	  		 	   case 4:    //上报入网节点
     	  		 	   	 printf("无线模块     - 上报入网节点\n");
     	  		 	   	 printf("节点地址     - %02x%02x%02x%02x%02x%02x\n",tmpBuf[5],tmpBuf[4],tmpBuf[3],tmpBuf[2],tmpBuf[1],tmpBuf[0]);
     	  		 	   	 printf("网内固定地址 - %02x%02x%02x%02x%02x%02x%02x%02x\n",tmpBuf[13],tmpBuf[12],tmpBuf[11],tmpBuf[10],tmpBuf[9],tmpBuf[8],tmpBuf[7],tmpBuf[6]);
             	 	   
             	 	   noRecordMeterHead = NULL;
                   tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                   memcpy(tmpFound->addr,tmpBuf,6);  //从节点地址
         	 	 	  	 tmpFound->copyTime[0]   = 0xee;
         	 	 	  	 tmpFound->copyTime[1]   = 0xee;
         	 	 	  	 tmpFound->copyEnergy[0] = 0xee;                      
                   tmpFound->next = NULL;
                   
                   tmpQueryFound = foundMeterHead;
                   while(tmpQueryFound!=NULL)
                   {
                   	  if (tmpQueryFound->addr[0]==tmpFound->addr[0] && tmpQueryFound->addr[1]==tmpFound->addr[1]
                   	  	 && tmpQueryFound->addr[2]==tmpFound->addr[2] && tmpQueryFound->addr[3]==tmpFound->addr[3]
                   	  	  && tmpQueryFound->addr[4]==tmpFound->addr[4] && tmpQueryFound->addr[5]==tmpFound->addr[5]
                   	  	 )
                   	  {
                   	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                   	  	 {
                   	  	   printf("上报表号与已上报的表号地址相同\n");
                   	  	 }
                   	  	 break;
                   	  }
                   	                     	  
                   	  tmpQueryFound = tmpQueryFound->next;
                   }
                   
                   if (tmpQueryFound==NULL)
                   {
                     if (foundMeterHead==NULL)
                     {
                 	     foundMeterHead = tmpFound;
                     }
                     else
                     {
                 	     prevFound->next = tmpFound;
                     }
                     prevFound = tmpFound;
                     
                     if (noRecordMeterHead==NULL)
                     {
                     	  noRecordMeterHead = tmpFound;
                     }
                   }    	 	    
     	  		 	   	 break;

     	  		 	   case 5:    //上报离网节点
     	  		 	   	 printf("无线模块     - 上报离网节点\n");
     	  		 	   	 printf("节点地址     - %02x%02x%02x%02x%02x%02x\n",tmpBuf[5],tmpBuf[4],tmpBuf[3],tmpBuf[2],tmpBuf[1],tmpBuf[0]);
     	  		 	   	 printf("网内固定地址 - %02x%02x%02x%02x%02x%02x%02x%02x\n",tmpBuf[13],tmpBuf[12],tmpBuf[11],tmpBuf[10],tmpBuf[9],tmpBuf[8],tmpBuf[7],tmpBuf[6]);

                   tmpQueryFound = tmpSalveNode = foundMeterHead;
                   while(tmpQueryFound!=NULL)
                   {
                   	  if (tmpQueryFound->addr[0]==tmpBuf[0] && tmpQueryFound->addr[1]==tmpBuf[1]
                   	  	 && tmpQueryFound->addr[2]==tmpBuf[2] && tmpQueryFound->addr[3]==tmpBuf[3]
                   	  	  && tmpQueryFound->addr[4]==tmpBuf[4] && tmpQueryFound->addr[5]==tmpBuf[5]
                   	  	 )
                   	  {
                   	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
                   	  	 {
                   	  	   printf("从入网节点中删除离网节点\n");
                   	  	 }
                   	  	 break;
                   	  }
                   	  
                   	  tmpSalveNode  = tmpQueryFound;
                   	  tmpQueryFound = tmpQueryFound->next;
                   }
                   
                   if (tmpSalveNode==foundMeterHead)
                   {
                     foundMeterHead = tmpQueryFound->next;
                   }
                   else
                   {
                     tmpSalveNode->next = tmpQueryFound->next;
                   }
                   free(tmpQueryFound);
     	  		 	   	 break;
     	  		 	 }
     	  		 	 break;
     	  		 
     	  		case FC_QUERY_DATA_3762:  //友迅达扩展查询数据
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	 case 2:    //CAC 状态信息
     	  		 	 	 	 switch(tmpBuf[0])
     	  		 	 	 	 {
     	  		 	 	 	 	 case 1:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	 	 printf("友迅达模块:CAC工作状态\n");
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 	      	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 if (carrierFlagSet.readStatus==1)
     	  		 	 	 	 	 	 {
     	  		 	 	 	 	 	 	 carrierFlagSet.readStatus = 2;
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 carrierFlagSet.wlNetOk = 3;
     	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 showInfo("本地通信模块组网完成");
     	  		 	 	 	 	 	 break;
     	  		 	 	 	 	 	 
     	  		 	 	 	 	 case 2:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	   printf("友迅达模块:CAC组网状态\n");
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 showInfo("本地通信模块组网中..");
     	  		 	 	 	 	 	 break;
     	  		 	 	 	 	 	 
     	  		 	 	 	 	 case 3:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	   printf("友迅达模块:CAC网络维护\n");
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 
     	  		 	 	 	 	 	 showInfo("本地通信模块组网络维护");
     	  		 	 	 	 	 	 break;

     	  		 	 	 	 	 case 4:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	   printf("友迅达模块:CAC正在处理上个命令\n");
     	  		 	 	 	 	 	 
							     }
     	  		 	 	 	 	 break;
     	  		 	 	 	 }
     	  		 	 	 	 	 
     	  		 	 	 	 if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	 {
     	  		 	 	 	 	 printf("友迅达模块:组网DAU的个数=%d\n",tmpBuf[1] | tmpBuf[2]<<8);
     	  		 	 	 	 	 printf("友迅达模块:下载DAU的个数=%d\n",tmpBuf[3] | tmpBuf[4]<<8);
     	  		 	 	 	 	 printf("友迅达模块:故障DAU的个数=%d\n",tmpBuf[5] | tmpBuf[6]<<8);
     	  		 	 	 	 }
     	  		 	 	 	 
     	  		 	 	 	 carrierFlagSet.routeRunStatus = 0;
     	  		 	 	 	 break;
     	  		 	 	 
     	  		 	 	 case 10:    //单帧读取DAU信息
    	  		 	 	   //carrierFlagSet.numOfSalveNode = tmpBuf[1] | tmpBuf[2]<<8;
                   if (tmpBuf[3]==0)
                   {
    	  		 	 	  	   if (carrierFlagSet.querySlaveNode==1)
    	  		 	 	  	   {
  	                      carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
  	                   
  	                      if (debugInfo&PRINT_CARRIER_DEBUG)
  	                      {
  	                        printf("友迅达无线模块从节点列表(路由查询)\n");
  	                        tmpSalveNode = carrierSlaveNode;
  	                        while(tmpSalveNode!=NULL)
  	                        {
    	  		 	 	  	          printf("从节点地址:%02x%02x%02x%02x%02x%02x\n",tmpSalveNode->addr[5],tmpSalveNode->addr[4],tmpSalveNode->addr[3],tmpSalveNode->addr[2],tmpSalveNode->addr[1],tmpSalveNode->addr[0]);
    	  		 	 	  	          tmpSalveNode = tmpSalveNode->next;
  	                        }
  	                      }
    	  		 	 	  	   }
                   }
                   else
                   {
                      for(i=0;i<tmpBuf[3];i++)
                      {
       	  		 	 	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  	 {
       	  		 	 	  	   printf("从节点地址:%02x%02x%02x%02x%02x%02x\n",tmpBuf[i*7+9],tmpBuf[i*7+8],tmpBuf[i*7+7],tmpBuf[i*7+6],tmpBuf[i*7+5],tmpBuf[i*7+4]);
       	  		 	 	  	 }
       	  		 	 	  	 
       	  		 	 	  	 //载波从节点地址为全0,则认为不再有后续从节点地址,停止读取
                        if (tmpBuf[i*7+8]==0x0 && tmpBuf[i*7+7]==0x0 && tmpBuf[i*7+6]==0x0 && tmpBuf[i*7+5]==0x0 && tmpBuf[i*7+4]==0x0 && tmpBuf[i*7+3]==0x0)
                        {
                        	 carrierFlagSet.numOfSalveNode = carrierFlagSet.synMeterNo;
    	  		 	 	  	       
    	  		 	 	  	       if (carrierFlagSet.querySlaveNode==1)
    	  		 	 	  	       {
  	                         carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
  	                   
  	                         if (debugInfo&PRINT_CARRIER_DEBUG)
  	                         {
  	                           printf("结束读载波模块从节点列表,因返回的从节点地址为全0\n");
  	                         }
    	  		 	 	  	       }
    	  		 	 	  	       
    	  		 	 	  	       break;
                        }
                        
                        tmpSalveNode = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                        tmpSalveNode->mpNo = carrierFlagSet.synMeterNo;           //从节点序号
                        memcpy(tmpSalveNode->addr,tmpBuf+i*7+4,6); //从节点地址
                        memcpy(tmpSalveNode->info,tmpBuf+i*7+10,1); //从节点信息
                        tmpSalveNode->next = NULL;
                        if (carrierSlaveNode==NULL)
                        {
                      	   carrierSlaveNode = tmpSalveNode;
                        }
                        else
                        {
                      	   prevSlaveNode->next = tmpSalveNode;
                        }
                        prevSlaveNode = tmpSalveNode;
     	                 carrierFlagSet.synMeterNo++;
                      }
                      
                      //如果返回从节点的个数大于等于从节点总数量,停止读取
                      if (carrierFlagSet.synMeterNo > carrierFlagSet.numOfSalveNode)
                      {
  	                      carrierFlagSet.querySlaveNode = 2;   //读取载波从节点完成
  	                   
  	                      if (debugInfo&PRINT_CARRIER_DEBUG)
  	                      {
  	                        printf("返回从节点的个数大于等于从节点总数量,停止读取\n");
  	                      }
                      }
                   }
     	  		 	 	 	 break;
     	  		 	 }
     	  		 	 break;
     	  	}
     	  	
     	  	carrierFlagSet.cmdContinue = 1;      //可以继续发送命令
     	  	carrierFlagSet.sending     = 0;      //正在发送命令置为未发送
     	  	
     	  	carrierFlagSet.numOfCarrierSending = 0;
     	  	
     	  	if (debugInfo&PRINT_CARRIER_DEBUG)
     	  	{
     	  		//printf("载波/无线端口有返回,清零发送计数\n");
     	  	}
     	  	break;
     }
     
     if (libLen<recvLen)
     {
     	 //printf("上次没处理完成,libLen=%d,recvLen=%d\n", libLen, recvLen);
     	 goto repeatRecv;
     }
     else
     {
     	 //printf("接收处理完整\n");
     }
     
#ifdef LM_SUPPORT_UT
breakCarrierRecv:
#endif

     usleep(50);
  }
}
#endif

/*******************************************************
函数名称:whichItem
功能描述:查找是在抄表控制参数(主站设置值)数组的哪一个元素
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:数组下标
*******************************************************/
INT8U whichItem(INT8U port)
{
	 INT8U i;
	 
	 for(i=0;i<teCopyRunPara.numOfPara;i++)
	 {
	 	 if (teCopyRunPara.para[i].commucationPort==port)
	 	 {
	 	  	break;
	 	 }
	 }
	 
	 return i;
}

/***************************************************
函数名称:initCopyDataBuff
功能描述:初始化抄表缓存
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
BOOL initCopyDataBuff(INT8U port, INT8U initType)
{
  INT16U               counti;
  struct meterCopyPara mcp;
  METER_DEVICE_CONFIG  meterConfig;
  INT16U               offset;
  DATE_TIME            tmpTime;
  INT8U                meterInfo[10];

 #ifdef WDOG_USE_X_MEGA
  INT8U                buf[3];
 #endif
    
 #ifndef LM_SUPPORT_UT 
  if (port>=4)
  {
    if (selectF10Data(copyCtrl[port].tmpCpLink->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
    {
      //库中没有该测量点的数据
      return FALSE;
    }
  }
  else
  {
 #endif
   
    if (selectF10Data(copyCtrl[port].cpLinkHead->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
    {
      //库中没有该测量点的数据
      return FALSE;
    }
      
   #ifdef LIGHTING
    //2014-12-23,因为该字节的最高位bit7可能表示计量功能,所以去掉
    //2015-01-21,该字节的bit6可能表示接入功能,所以去掉
    meterConfig.rateAndPort &= 0x3f;
   #endif
      
    if ((meterConfig.rateAndPort&0xe0)==0x0)  //默认速率
    {
      switch (meterConfig.protocol)
      {
        case DLT_645_1997:
          copyCtrl[port].backupCtrlWord = DEFAULT_SER_CTRL_WORD;    //1200bps,8-e-1
          break;
        	
    		case EDMI_METER:
          copyCtrl[port].backupCtrlWord = DEFAULT_CTRL_WORD_EDMI;   //4800bps,8-n-1
    	  	break;

    		case SIMENS_ZD_METER:
    		case SIMENS_ZB_METER:
          copyCtrl[port].backupCtrlWord = DEFAULT_CTL_W_IEC1107;    //300bps,7-e-1
          break;
          
        case ABB_METER:
          copyCtrl[port].backupCtrlWord = DEFAULT_SER_CTRL_ABB;     //1200bps,8-n-1 
          break;
          
       #ifdef LIGHTING
        case LIGHTING_LS:
          copyCtrl[port].backupCtrlWord = 0xC3;                     //9600bps,8-n-1 
          
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d设置为默认速率9600-8-n-1\n", __func__, port);
					}
          break;
					
				case LIGHTING_TH:
					copyCtrl[port].backupCtrlWord = 0xC3; 										//9600bps,8-n-1 
					
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d设置为默认速率9600-8-n-1\n", __func__, port);
					}
					break;
       #endif 
	    	case MODBUS_HY:      //山东和远，2017-5-24
	    	case MODBUS_ASS:     //无锡爱森思,2017-5-24
				case MODBUS_XY_F:    //上海贤业多功能表,2017-9-21
				case MODBUS_XY_UI:   //上海贤业电压电流表,2017-9-21
				case MODBUS_SWITCH:	 //MODBUS开关量模块,2017-9-22
				case MODBUS_XY_M:    //上海贤业电表模块,2017-10-10
				case MODBUS_MW_F: 	 //成都明武多功能表,2018-03-01
				case MODBUS_MW_UI:	 //成都明武电压电流表,2018-03-01
				case MODBUS_JZ_F: 	 //上海居正多功能表,2018-03-01
				case MODBUS_JZ_UI:	 //上海居正电压电流表,2018-03-01				
				case MODBUS_WE6301_F://威斯顿WE6301多功能表,2018-05-03
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d设置为MODBUS默认速率9600-8-n-1\n", __func__, port);
					}
		  		copyCtrl[port].backupCtrlWord = 0xC3;    //9600bps,8-n-1 
		  		break;
					
				case MODBUS_PMC350_F: 	 //深圳中电PMC350多功能表,2018-04-27
					copyCtrl[port].backupCtrlWord = 0xCb; 	 //9600bps,8-e-1
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d设置为MODBUS默认速率9600-8-e-1\n", __func__, port);
					}
					break;
					
        default:
          copyCtrl[port].backupCtrlWord = DEFAULT_CTRL_WORD_2400;   //2400bps,8-e-1
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d设置为默认速率2400-8-e-1\n", __func__, port);
					}
          
          //copyCtrl[port].backupCtrlWord = 0xc3;    //9600bps,8-n-1
					//if (debugInfo&METER_DEBUG)
					//{
					//	printf("%s==>RS485 %d设置为默认速率9600-8-n-1\n", __func__, port);
					//}
          break;
      }
    }
    else
    {
      switch (meterConfig.protocol)
      {
        case EDMI_METER:
          //无校验
          copyCtrl[port].backupCtrlWord = (meterConfig.rateAndPort&0xe0)|(DEFAULT_CTRL_WORD_EDMI&0x1f);
          break;

    		case SIMENS_ZD_METER:
    		case SIMENS_ZB_METER:
          copyCtrl[port].backupCtrlWord = (meterConfig.rateAndPort&0xe0)|(DEFAULT_CTL_W_IEC1107&0x1f);   //7-e-1
    	  	break;

    		case ABB_METER:
          copyCtrl[port].backupCtrlWord = (meterConfig.rateAndPort&0xe0)|(DEFAULT_SER_CTRL_ABB&0x1f);    //8-n-1
          break;
				
			 #ifdef LIGHTING
			  case LIGHTING_TH:
					copyCtrl[port].backupCtrlWord = (meterConfig.rateAndPort&0xe0)|(DEFAULT_SER_CTRL_ABB&0x1f);    //8-n-1
					break;
			 #endif
        	
        default:
          copyCtrl[port].backupCtrlWord = (meterConfig.rateAndPort&0xe0)|(DEFAULT_SER_CTRL_WORD&0x1f);
          break;
      }
			
			if (debugInfo&METER_DEBUG)
			{
				printf("%s==>RS485 %d设置控制字为%x02X\n", __func__, port, copyCtrl[port].backupCtrlWord);
			}
    }
      
 #ifndef LM_SUPPORT_UT
  }
 #endif
   
  //抄表端口按主站设置值配置速率
  switch(meterConfig.rateAndPort&0x1f)
  {
   #ifdef RS485_1_USE_PORT_1
    case  PORT_NO_1:    //485端口号1
   #else
    case  PORT_NO_2:    //485端口号2
   #endif
     #ifdef  WDOG_USE_X_MEGA
      buf[0] = 0x01;                         //xMega端口1
      buf[1] = copyCtrl[port].backupCtrlWord;//端口速率
      sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
     
      if (debugInfo&METER_DEBUG)
      { 
        printf("设置端口1速率\n");
      }
     #else
      oneUartConfig(fdOfttyS2, copyCtrl[port].backupCtrlWord);
     #endif
        
      mcp.port = 1;
        
      if (meterConfig.protocol==ABB_METER)
      {
        abbHandclasp[0] = 1;    //ABB握手未成功
      }
      else
      {
        abbHandclasp[0] = 0;
      }
      break;

   #ifdef RS485_1_USE_PORT_1
    case PORT_NO_2:    //485端口号2
   #else
    case PORT_NO_3:    //485端口号3
   #endif
     #ifdef WDOG_USE_X_MEGA
      buf[0] = 0x02;                          //xMega端口2
      buf[1] = copyCtrl[port].backupCtrlWord; //端口速率
      sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
      
      if (debugInfo&METER_DEBUG)
      {
        printf("设置端口2速率\n");
      }
     #else
      //如果软件支持级联,则第2路抄表口为ttyS4;
      //如果软件不持级联,则第2路抄表口为ttyS3
      #ifdef SUPPORT_CASCADE
       oneUartConfig(fdOfttyS4, copyCtrl[port].backupCtrlWord);
      #else
       oneUartConfig(fdOfttyS3, copyCtrl[port].backupCtrlWord);
      #endif
     #endif
    
      mcp.port = 2;

      if (meterConfig.protocol==ABB_METER)
      {
    	abbHandclasp[1] = 1;    //ABB握手未成功
      }
      else
      {
    	abbHandclasp[1] = 0;
      }        
      break;
        
   #ifdef RS485_1_USE_PORT_1
    case PORT_NO_3:    //485端口号3
   #else
    case PORT_NO_4:    //485端口号4
   #endif

    #ifdef WDOG_USE_X_MEGA
      buf[0] = 0x03;                          //xMega端口2
      buf[1] = copyCtrl[port].backupCtrlWord; //端口速率
      sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
      
      if (debugInfo&METER_DEBUG)
      {
        printf("设置端口3速率\n");
      }
     #endif
        
      mcp.port = 3;

      if (meterConfig.protocol==ABB_METER)
      {
        abbHandclasp[2] = 1;    //ABB握手未成功
      }
      else
      {
        abbHandclasp[2] = 0;
      }
      break;
    }
    
    if (initType==HOUR_FREEZE_DATA)
    {
      //memset(&copyCtrl[port].dataBuff[copyCtrl[port].dataBuff[0]+1], 0xee, 1536-copyCtrl[port].dataBuff[copyCtrl[port].dataBuff[0]-1]);
    }
    else
    {
      memset(copyCtrl[port].dataBuff, 0xee, 1536);
    }

   #ifdef PLUG_IN_CARRIER_MODULE
    if (port>=4)
    {
      mcp.port = PORT_POWER_CARRIER+port-4;
     #ifdef LM_SUPPORT_UT
      if (0x55==lmProtocol)
      {
       	mcp.send = sendMeterFrame;
      }
      else
      {
     #endif
       
        mcp.send = sendCarrierFrame;
      
     #ifdef LM_SUPPORT_UT
      }
     #endif
       
      if (meterConfig.protocol==DLT_645_1997)
      {
        mcp.protocol = SINGLE_PHASE_645_1997;
        copyCtrl[port].protocol = 1;
      }
      else
      {
       	
       #ifndef LM_SUPPORT_UT 
        if (debugInfo&PRINT_CARRIER_DEBUG)
        {
          printf("载波测量点:%d,用户大类号=%d,小类号=%d\n", copyCtrl[port].tmpCpLink->mp, meterConfig.bigAndLittleType>>4, meterConfig.bigAndLittleType&0xf);
        }
       #endif
         
        if (initType==HOUR_FREEZE_DATA)
        {
          mcp.protocol = HOUR_FREEZE_2007;
        }
        else
        {
          queryMeterStoreInfo(meterConfig.measurePoint,meterInfo);
           
          //电表类型(1-单相智能表,2-单相本地费控表,3-单相远程费控表,4-三相智能表,5-三相本地费控表,6-三相远程费控表,7-485三相表(数据抄读最全),8-重点用户
          switch(meterInfo[0])
          {
           	case 1:  //单相智能表
              switch(denizenDataType)
              {
                case 0x55:    //仅抄实时数据(总及各费率)
                  mcp.protocol = SINGLE_PHASE_645_2007_TARIFF;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("单相智能表(仅抄实时数据(总及各费率))\n");
                  }
                  break;

                case 0xaa:    //仅抄实时数据(总示值)
                  mcp.protocol = SINGLE_PHASE_645_2007_TOTAL;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("单相智能表(仅抄实时数据(总示值))\n");
                  }
                  break;
                   
                default:
                  mcp.protocol = SINGLE_PHASE_645_2007;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("单相智能表\n");
                  }
                  break;
              }
           	  break;
           	 
           	case 2:  //单相本地费控表
              mcp.protocol = SINGLE_LOCAL_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("单相本地费控表\n");
              }
       	 	  break;
           	 
           	case 3:  //单相远程费控表
              mcp.protocol = SINGLE_REMOTE_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("单相或三相远程费控表\n");
              }
              break;
             	 
            case 4:  //三相智能表
              mcp.protocol = THREE_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("三相智能表\n");
              }
              break;
             
            case 5:  //三相本地费控表
              mcp.protocol = THREE_LOCAL_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("三相本地费控表\n");
              }
              break;
             
            case 6:  //三相远程费控表
              mcp.protocol = THREE_REMOTE_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("三相远程费控表\n");
              }
              break;
             	 
            case 7:  //07三相表,数据最全
              mcp.protocol = DLT_645_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("07三相表\n");
              }
              break;
             	 
            case 8:  //07重点用户
              mcp.protocol = KEY_HOUSEHOLD_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("07重点用户表\n");
              }
              break;
          }
        }
        copyCtrl[port].protocol = 2;
      }
       
      //如果采集器地址为全0,则载波目的节点为表地址,否则目的节点为采集器地址
      if (meterConfig.collectorAddr[0]==0x00 && meterConfig.collectorAddr[1]==0x00
       	  && meterConfig.collectorAddr[2]==0x00 && meterConfig.collectorAddr[3]==0x00
       	   && meterConfig.collectorAddr[4]==0x00 && meterConfig.collectorAddr[5]==0x00
       	  )
      {
        memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
        memcpy(copyCtrl[port].destAddr, meterConfig.addr, 6);          //ly,2012-01-10,add
        copyCtrl[port].ifCollector = 0;      //无采集器地址
      }
      else
      {
        memcpy(carrierFlagSet.destAddr, meterConfig.collectorAddr, 6);
        memcpy(copyCtrl[port].destAddr, meterConfig.collectorAddr, 6); //ly,2012-01-10,add
        copyCtrl[port].ifCollector = 1;      //有采集器地址
      }

      //抄表时间为系统当前时间
      mcp.copyTime = timeHexToBcd(sysTime);
    }
    else
    {
   #endif
      mcp.send     = sendMeterFrame;
      switch (meterConfig.protocol)
      {
       	case DLT_645_1997:
          switch (meterConfig.bigAndLittleType&0xf)
          {
            case 1:    //小类号为1
              mcp.protocol = SINGLE_PHASE_645_1997;
              break;
             
            case 13:   //小类号为13只抄正反向有无功电能示值
              mcp.protocol = PN_WORK_NOWORK_1997;
              break;
               
            default:
              mcp.protocol = meterConfig.protocol;
              break;
          }
          break;
           
        case DLT_645_2007:
          switch (meterConfig.bigAndLittleType&0xf)
          {
            case 1:    //小类号为1
              mcp.protocol = SINGLE_PHASE_645_2007;
              break;
             
            case 13:   //小类号为只抄正反向有无功能电能示值
              mcp.protocol = PN_WORK_NOWORK_2007;
              break;
             	 
            default:
              mcp.protocol = meterConfig.protocol;
              break;
          }
          break;
          
        default:
          mcp.protocol = meterConfig.protocol;
          break;
      }
       
      //当前时间为本端口本次抄表时间
      mcp.copyTime = timeHexToBcd(copyCtrl[port].currentCopyTime);
   #ifdef PLUG_IN_CARRIER_MODULE
    }
   #endif
   
    mcp.pn   = meterConfig.measurePoint;
    memcpy(mcp.meterAddr, meterConfig.addr, 6);

    switch (initType)
    {
      case PRESENT_DATA:
        mcp.copyDataType = PRESENT_DATA;
      
        if (debugInfo&METER_DEBUG)
        {
          printf("抄%02x%02x%02x%02x%02x%02x当前数据\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;

      case LAST_MONTH_DATA:
        mcp.copyDataType = LAST_MONTH_DATA;
      
        if (debugInfo&METER_DEBUG)
        {
          printf("抄%02x%02x%02x%02x%02x%02x上月数据\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;

      case LAST_DAY_DATA:
        mcp.copyDataType = LAST_DAY_DATA;
      
        if (debugInfo&METER_DEBUG)
        {
          printf("抄%02x%02x%02x%02x%02x%02x电能表上一日冻结数据\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;
        
    	case HOUR_FREEZE_DATA:
        mcp.copyDataType = HOUR_FREEZE_DATA;
        if (debugInfo&METER_DEBUG)
        {
          printf("抄%02x%02x%02x%02x%02x%02x电能表整点冻结数据\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;
        
      default:
        if (debugInfo&METER_DEBUG)
        {
         printf("指定的抄表数据类型未知\n");
        }
        
        return FALSE;
      	break;
  }
    
  mcp.dataBuff = copyCtrl[port].dataBuff;
  mcp.save = saveMeterData;
    
  initCopyProcess(&mcp);
    
  copyCtrl[port].numOfMeterAbort = 0;
  copyCtrl[port].hasSuccessItem  = 0;
    
  copyCtrl[port].copyContinue    = TRUE;
  copyCtrl[port].flagOfRetry     = FALSE;
    
  return TRUE;
}

/***************************************************
函数名称:initCopyItem
功能描述:初始化为抄表初始值
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
BOOL initCopyItem(INT8U port, INT8U ifCopyLastFreeze)
{
   INT16U tmpData;
   BOOL   ret;

   copyCtrl[port].currentCopyTime = sysTime;        //本次抄表时间
    
   //集中器主导抄表
  #ifdef PLUG_IN_CARRIER_MODULE 
   if (!(carrierFlagSet.routeLeadCopy==1 && port==4))
   {
  #endif
     
     switch (ifCopyLastFreeze)
     {
       case 1:   //上月数据
   	     copyCtrl[port].copyDataType = LAST_MONTH_DATA;
   	     ret = initCopyDataBuff(port, LAST_MONTH_DATA);
   	     break;
   	     
   	   case 2:   //上一次日冻结数据
   	     copyCtrl[port].copyDataType = LAST_DAY_DATA;
   	     ret = initCopyDataBuff(port, LAST_DAY_DATA);
   	     break;
  
   	   case 3:   //整点冻结数据
   	     copyCtrl[port].copyDataType = HOUR_FREEZE_DATA;
   	     ret = initCopyDataBuff(port, HOUR_FREEZE_DATA);
   	     break;
   	     
       default:  //采集当前数据
         copyCtrl[port].copyDataType = PRESENT_DATA;
         ret = initCopyDataBuff(port, PRESENT_DATA);
         break;
     }

     //if (meterDeviceConfig.meterNumber>0)
     //{
        //记录总抄表次数
      	/*
      	if (realStatisBuff2[TOTAL_COPY_TIMES+1]==0xee)
      	{
      	 	  realStatisBuff2[TOTAL_COPY_TIMES]   = 0x1;
      	 	  realStatisBuff2[TOTAL_COPY_TIMES+1] = 0x0;
      	}
      	else
      	{
      	 	  tmpData = (realStatisBuff2[TOTAL_COPY_TIMES] | realStatisBuff2[TOTAL_COPY_TIMES+1]<<8)+1;
      	 	  realStatisBuff2[TOTAL_COPY_TIMES]   = tmpData&0xff;
      	 	  realStatisBuff2[TOTAL_COPY_TIMES+1] = tmpData>>8 & 0xff;
      	}
      	*/
     //}
  #ifdef PLUG_IN_CARRIER_MODULE
   }
  #endif
   
  #ifdef PLUG_IN_CARRIER_MODULE
   if ((ret==TRUE && carrierFlagSet.routeLeadCopy==0) || (carrierFlagSet.routeLeadCopy==1))
  #else
   if (ret==TRUE)
  #endif
   {
     if (port==4)
     {
        tmpData = whichItem(PORT_POWER_CARRIER);
     }
     else
     {
        tmpData = whichItem(port+1);
     }

     if (tmpData<5)
     {
      #ifdef LM_SUPPORT_UT
        if (4==port && 0x55!=lmProtocol)
      #else
        if (port==4)
      #endif
        {
          copyCtrl[port].nextCopyTime = nextTime(sysTime, 60, 0);
          if (copyCtrl[port].nextCopyTime.hour==0x0)
          {
          	copyCtrl[port].nextCopyTime.minute = 10;
          }
          else
          {
            copyCtrl[port].nextCopyTime.minute = 0x1;
          }
          copyCtrl[port].nextCopyTime.second = 0x0;
          
          //因为要抄整点冻结数据并且要上报,所以要从每小时的1分开始抄整点冻结数据
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
            if (copyCtrl[port].nextCopyTime.hour==0x0)
            {
              printf("载波/无线端口0点的抄表时间从0点10分开始\n");
            }
            else
            {
              printf("载波/无线端口抄表时间为每小时的1分开始\n");
            }
          }
        }
        else
        {
         #ifdef LIGHTING
          //2016-07-02,照明集中器485接口的抄表间隔改为以10秒为单位
          copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, teCopyRunPara.para[tmpData].copyInterval*10);
          if (debugInfo&METER_DEBUG)
          {
            printf("抄表间隔:%d秒\n", teCopyRunPara.para[tmpData].copyInterval*10);
          }
         #else
          copyCtrl[port].nextCopyTime = nextTime(sysTime, teCopyRunPara.para[tmpData].copyInterval, 0);
          copyCtrl[port].nextCopyTime.second = 0x0;
          if (debugInfo&METER_DEBUG)
          {
            printf("抄表间隔:%d\n", teCopyRunPara.para[tmpData].copyInterval);
          }
         #endif
        }
     }
     else
     {
        copyCtrl[port].nextCopyTime = nextTime(sysTime,5,0);
        
        if (debugInfo&METER_DEBUG)
        {
          printf("抄表间隔未配置,定为5分钟");
        }
     }
     
     //集中器主导抄表或485端口
    #ifdef PLUG_IN_CARRIER_MODULE 
     if (!(carrierFlagSet.routeLeadCopy==1 && port==4))
     {
    #endif
    
       copyCtrl[port].meterCopying = TRUE;
       
    #ifdef PLUG_IN_CARRIER_MODULE
     }
    #endif
   }

   return ret;
}

/***************************************************
函数名称:copy485Failure
功能描述:485表抄表失败事件
调用函数:
被调用函数:
输入参数:startOrStop =1,发生  =2,恢复
输出参数:
返回值：void
***************************************************/
BOOL copy485Failure(INT16U mp,INT8U startOrStop,INT8U port)
{
	 METER_STATIS_EXTRAN_TIME meterStatisRecord;
	 INT8U                    eventData[25];
	 char                     strSave[100],*pSqlStr;
	 DATE_TIME                tmpTime;
	 INT8U                    dataBuff[LENGTH_OF_ENERGY_RECORD];

	 if (startOrStop<1 || startOrStop>2)
	 {
		 return FALSE;
	 }

   if ((eventRecordConfig.iEvent[3] & 0x40) || (eventRecordConfig.nEvent[3] & 0x40))
   {
	   //读出电表统计数据
     searchMpStatis(sysTime, &meterStatisRecord, mp, 1);
     
     //已经记录过失败,等待恢复
     if ((meterStatisRecord.mixed&0x02) && startOrStop==1)
     {
     	  return FALSE;
     }

     tmpTime = timeHexToBcd(sysTime);
     if (readMeterData(dataBuff, mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0)==TRUE)
     {
         eventData[0] = 31;
         eventData[1] = 24;
      
         //发生时间
         eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
         eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
         eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
         eventData[7] = sysTime.year/10<<4   | sysTime.year%10;
   
         //测量点及起/止标志
         eventData[8] = mp&0xff;
         eventData[9] = mp>>8&0xff;
         
         //发生
         if (startOrStop==1)
         {
         	  eventData[9] |= 0x80;
         	  
         	  meterStatisRecord.mixed |= 0x02;
            
            saveMeterData(mp, port, timeHexToBcd(sysTime), (INT8U *)&meterStatisRecord, STATIS_DATA, 0xfe,sizeof(METER_STATIS_EXTRAN_TIME));
         }
         
         //恢复
         if (startOrStop==2)
         {
         	  meterStatisRecord.mixed &= 0xfd;         	  
         }
         
         //最后一次抄表成功时间
         eventData[10] = tmpTime.minute;
         eventData[11] = tmpTime.hour;
         eventData[12] = tmpTime.day;
         eventData[13] = tmpTime.month;
         eventData[14] = tmpTime.year;
         
         //最后一次抄表成功正向有功总示值
         if (dataBuff[POSITIVE_WORK_OFFSET]==0xee)
         {
           eventData[15] = 0xee;
         }
         else
         {
           eventData[15] = 0;
         }
         eventData[16] = dataBuff[POSITIVE_WORK_OFFSET];
         eventData[17] = dataBuff[POSITIVE_WORK_OFFSET+1];
         eventData[18] = dataBuff[POSITIVE_WORK_OFFSET+2];
         eventData[19] = dataBuff[POSITIVE_WORK_OFFSET+3];

         //最后一次抄表成功反向有功总示值
         eventData[20] = dataBuff[NEGTIVE_WORK_OFFSET];
         eventData[21] = dataBuff[NEGTIVE_WORK_OFFSET+1];
         eventData[22] = dataBuff[NEGTIVE_WORK_OFFSET+2];
         eventData[23] = dataBuff[NEGTIVE_WORK_OFFSET+3];
   
         if (eventRecordConfig.iEvent[3] & 0x40)
         {
           writeEvent(eventData,24, 1, DATA_FROM_GPRS);
         }
   
         if (eventRecordConfig.nEvent[3] & 0x40)
         {
           writeEvent(eventData,24,2, DATA_FROM_LOCAL);
         }
           
         eventStatus[3] = eventStatus[3] | 0x40;
         
         return TRUE;
     }
   }
   
   return TRUE;
}

/***************************************************
函数名称:initPortMeterLink
功能描述:初始化为抄表链表
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void

历史：
    1.ly,2012-08-09,剔除全0的表地址
***************************************************/
struct cpAddrLink * initPortMeterLink(INT8U port)
{
	 struct cpAddrLink   *tmpHead, *tmpNode,*prevNode;
	 METER_DEVICE_CONFIG meterConfig;
	 sqlite3_stmt        *stmt;
	 char                *sql;
	 INT16U              result;
	 INT8U               tmpPort;
	 INT16U              numOfMp;    //测量点个数
	 
	 tmpHead = NULL;
	 
	 prevNode = tmpHead;
	 
	 //查询库中表测量点号
	 if (port==0xff)
	 {
	   sql = "select * from f10_info order by acc_pn";
	   sqlite3_prepare(sqlite3Db, sql, -1, &stmt, 0);
	 }
	 else
	 {
	   tmpPort = port+1;
	   sql = "select * from f10_info where acc_port = ? order by acc_pn";
	   sqlite3_prepare(sqlite3Db, sql, -1, &stmt, 0);
	   sqlite3_bind_int(stmt, 1, tmpPort);
	 }
	 result = sqlite3_step(stmt);
	 numOfMp = 0;
   while(result==SQLITE_ROW)
   {
		  memcpy(&meterConfig, sqlite3_column_blob(stmt, 4), sizeof(METER_DEVICE_CONFIG));

      if (compareTwoAddr(meterConfig.addr, meterConfig.addr, 1)==TRUE)
      {
      	if (debugInfo&METER_DEBUG)
      	{
      		 printf("测量点%d地址为全零,剔除\n", meterConfig.measurePoint);
      	}
      }
      else
      {
        tmpNode = (struct cpAddrLink *)malloc(sizeof(struct cpAddrLink));
        tmpNode->mpNo = meterConfig.number;                         //测量点序号
        tmpNode->mp   = meterConfig.measurePoint;                   //测量点号
        tmpNode->protocol = meterConfig.protocol;                   //协议
        memcpy(tmpNode->addr,meterConfig.addr,6);                   //通信地址
        memcpy(tmpNode->collectorAddr,meterConfig.collectorAddr,6); //采集器地址
                                                                    //  照明集中器a)单灯控制器用于表示亮度与占空比的对应关系中的亮度
                                                                    //            b)报警控制点用于表示第2只末端的载波通信点地址
                                                                    //            c)线路控制点用于表示经度和纬度,2016-02-15
        tmpNode->port = meterConfig.rateAndPort&0x1f;               //端口
        tmpNode->bigAndLittleType = meterConfig.bigAndLittleType;   //用户大类号及用户小类号
                                                                    //  照明集中器a)单灯控制点用于表示PWM频率
                                                                    //            b)报警控制点
                                                                    //              bit5-0,表示心跳周期
                                                                    //              bit7,6表示载波建立时间的低2位
                                                                    //            c)线路控制点用于表示本控制点的控制模式,2016-02-15
        tmpNode->flgOfAutoCopy = 0;                                 //路由请求自动抄读标志(载波模块有效)
       
       #ifdef LIGHTING
        tmpNode->ctrlTime = meterConfig.numOfTariff;                //控制时段号(存储于费率字段)
        
        if (LIGHTING_DGM==tmpNode->protocol)
        {
          memcpy(tmpNode->lddt1st, meterConfig.password, 6);        //报警控制点第1只末端单灯控制器

          tmpNode->funOfMeasure = tmpNode->bigAndLittleType & 0x3f; //末端单灯控制器心跳周期
          
          //上电后载波建立时间
          tmpNode->joinUp = (meterConfig.rateAndPort&0xc0)>>4 
                           | (tmpNode->bigAndLittleType&0xc0)>>6;
          tmpNode->joinUp *= 5;
          
          if (debugInfo&METER_DEBUG)
          {
            printf("控制点%d(%02x-%02x-%02x-%02x-%02x-%02x)的心跳周期=%d,建立时间=%d\n", tmpNode->mp, tmpNode->addr[5], tmpNode->addr[4], tmpNode->addr[3], tmpNode->addr[2], tmpNode->addr[1], tmpNode->addr[0], tmpNode->funOfMeasure, tmpNode->joinUp);
          }
          
          if (0==tmpNode->funOfMeasure)
          {
          	tmpNode->funOfMeasure = 2;
            if (debugInfo&METER_DEBUG)
            {
              printf("  - 修正后的心跳周期=%d\n", tmpNode->funOfMeasure);
            }
          }
          if (0==tmpNode->joinUp)
          {
          	tmpNode->joinUp = 5;
          	
            if (debugInfo&METER_DEBUG)
            {
              printf("  - 修正后的建立时间=%d\n", tmpNode->joinUp);
            }
          }
        }
        else
        {
          tmpNode->funOfMeasure = (meterConfig.rateAndPort&0x80)?1:0; //是否有计量功能
          tmpNode->joinUp = (meterConfig.rateAndPort&0x40)?1:0;       //是否接入
        	                                                            //  单灯控制器 - 是否接入灯具
                                                                      //  线路控制点 - 是否接入反馈遥信
        }
        
        memcpy(tmpNode->duty, meterConfig.password, 6);               //单灯控制器各亮度的占空比
        if (
        	  LIGHTING_LS==tmpNode->protocol 
        	   || LIGHTING_DGM==tmpNode->protocol
        	     || AC_SAMPLE==tmpNode->protocol
        	      || LIGHTING_XL==tmpNode->protocol
        	 )
        {
        	memset(tmpNode->duty, 0x00, 6);
        }
        
        //2016-03-17,主站设置的经纬度控制模式时分/合闸微调值
        if (LIGHTING_XL==tmpNode->protocol)
        {
        	tmpNode->duty[4] = meterConfig.password[0];    //合闸微调值
        	tmpNode->duty[5] = meterConfig.password[1];    //分闸微调值
        }
        
        tmpNode->startCurrent = meterConfig.mixed;                  //启动电流,2014-11-03,add
                                                                    //  单灯控制器的启动电流,用于判断单灯故障
                                                                    //  报警控制器的线路电流低字节,用于判断线路故障

        tmpNode->status = 0xfe;
        tmpNode->statusUpdated = 0;                                 //2015-10-26,Add
        tmpNode->lineOn = 0;                                        //2015-11-09,Add

        tmpNode->copySuccess = FALSE;                               //2015-10-26,Add,照明集中器需要这一句,否则会造成抄读成功统计错误

        tmpNode->lddtRetry = 0;                                     //搜索LDDT重试次数
        
        tmpNode->offLineRetry = 0;                                  //2015-11-11,Add,离线LDD重试次数
        
        tmpNode->lcOnOff = 0xfe;                                    //2016-01-20,Add,光控电源开关
        tmpNode->lcDetectOnOff = 0xfe;                              //2016-01-21,Add,根据光控检测的电源开关标志
				tmpNode->lcProtectTime.year = 0xff;                         //2017-01-09,Add,光控分合闸保护时间
        
        tmpNode->gateOn = 0xfe;                                     //2016-01-20,Add,线路控制点从分闸变成合闸
				
				tmpNode->softVer = 0;                                       //2016-11-04,Add,软件版本号
        
       #endif
       
        tmpNode->next = NULL;
         
        numOfMp++;
   
   		  //if (debugInfo&METER_DEBUG)
   		  //{
   		  //  printf("测量点%d,协议=%d\n", tmpNode->mp, tmpNode->protocol);
   		  //}
   		        
        if (tmpHead==NULL)
        {
         	tmpHead = tmpNode;
        }
        else
        {
         	prevNode->next = tmpNode;
        }
        prevNode = tmpNode;
      }
            
      result = sqlite3_step(stmt);
   }
   sqlite3_finalize(stmt);
   
   if (debugInfo&METER_DEBUG)
   {
     //ly,2012-01-10,注释
     //printf("端口%d配置测量点%d个抄表链表建立完成\n",port+1,numOfMp);
   }
   
   return tmpHead;
}

/***************************************************
函数名称:findAcPn
功能描述:查找交采测量点号
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：交采的测量点号,0为没有配置
***************************************************/
INT16U findAcPn(void)
{
	 METER_DEVICE_CONFIG meterConfig;
	 sqlite3_stmt        *stmt;
	 char                *sql;
	 INT16U              result;
	 INT16U              tmpPn = 0;
	 
	 sql = "select * from f10_info where acc_port = 1 or acc_port=2 or acc_port=3 order by acc_pn";
	 sqlite3_prepare(sqlite3Db, sql, -1, &stmt, 0);
	 result = sqlite3_step(stmt);
   while(result==SQLITE_ROW)
   {
		  memcpy(&meterConfig, sqlite3_column_blob(stmt, 4), sizeof(METER_DEVICE_CONFIG));

      if (meterConfig.protocol==AC_SAMPLE)
      {
      	tmpPn = meterConfig.measurePoint;
      	break;
      }

      result = sqlite3_step(stmt);
   }
   sqlite3_finalize(stmt);
   
   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("找到交采测量点号为%d\n", tmpPn);
   }
   
   return tmpPn;
}


/***************************************************
函数名称:reSetCopyTime
功能描述:重新设置抄表时间
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void reSetCopyTime(void)
{
  INT8U i;
  INT8U ret;

  for(i=0;i<NUM_OF_COPY_METER;i++)
  {
     if (i==4)
     {
        ret = whichItem(PORT_POWER_CARRIER);
     }
     else
     {
        ret = whichItem(i+1);
     }
     
     if (ret>5)
     {
      	if (debugInfo&METER_DEBUG)
      	{
      	  printf("重新确定抄表时间:端口%d抄表间隔:未配置\n",i+1);
      	}
     }
     else
     {
     	 #ifdef LM_SUPPORT_UT
        if (4==i && 0x55!=lmProtocol)
     	 #else
        if (i==4)
       #endif
        {
          if (debugInfo&METER_DEBUG)
          {
            printf("载波端口抄表间隔固定为每小时1分\n");
          }
          
          //载波端口抄表间隔固定为每小时1分,0点的为了等待由于电能表时间误差造成的冻结数据的形成为10分开始
          copyCtrl[i].nextCopyTime = nextTime(sysTime, 60, 0);

          if (copyCtrl[i].nextCopyTime.hour==0x0)
          {
            copyCtrl[i].nextCopyTime.minute = 10;
          }
          else
          {
            copyCtrl[i].nextCopyTime.minute = 0x01;
          }
          copyCtrl[i].nextCopyTime.second = 0x00;
        }
        else
        {
          //if (debugInfo&METER_DEBUG)
          //{
          //  printf("重新确定抄表时间:抄表间隔:%d\n",teCopyRunPara.para[ret].copyInterval);
          //}
         
          ////根据最近一次抄表时间计算下一次抄表时间
          //if (copyCtrl[i].lastCopyTime.year==0x00)
          //{
          //  copyCtrl[i].nextCopyTime = nextTime(sysTime,1,0);
          //}
          //else
          //{
          //  copyCtrl[i].nextCopyTime = nextTime(sysTime,teCopyRunPara.para[ret].copyInterval,0);
          //}
          
          if (copyCtrl[i].meterCopying==TRUE)
          {
          	copyCtrl[i].meterCopying = FALSE;
          	
          	if (debugInfo&METER_DEBUG)
          	{
          		printf("端口%d正在抄表,停止抄表\n",i+1);
          	}
          }
          
          if (debugInfo&METER_DEBUG)
          {
          	printf("端口%d当前时间的下一分0秒开始抄表\n",i+1);
          }
          
          copyCtrl[i].nextCopyTime = nextTime(sysTime, 1, 0);
          copyCtrl[i].nextCopyTime.second = 0x0;
        }
     }

     if (debugInfo&METER_DEBUG)
     {
       printf("重新确定抄表时间:端口%d下次抄表时间:%02d-%02d-%02d %02d:%02d:%02d\n", i+1, 
              copyCtrl[i].nextCopyTime.year,copyCtrl[i].nextCopyTime.month,
              copyCtrl[i].nextCopyTime.day,copyCtrl[i].nextCopyTime.hour,
              copyCtrl[i].nextCopyTime.minute,copyCtrl[i].nextCopyTime.second);
     }
  }
}

#ifdef PLUG_IN_CARRIER_MODULE

/***************************************************
函数名称:复位载波标志
功能描述:resetCarrierFlag
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void resetCarrierFlag(void)
{
	carrierModuleType = NO_CARRIER_MODULE;
	
	carrierFlagSet.sending        = 0;
	carrierFlagSet.cmdContinue    = 1;
	carrierFlagSet.retryCmd       = 0;

	carrierFlagSet.hardwareReest  = 0;
	carrierFlagSet.innerDebug     = 0;
	carrierFlagSet.mainNode       = 0;
	carrierFlagSet.setMainNode    = 0;
	carrierFlagSet.startStopWork  = 1;
	carrierFlagSet.synSlaveNode   = 1;
	carrierFlagSet.querySlaveNum  = 1;
	carrierFlagSet.querySlaveNode = 0;
	carrierFlagSet.routeRunStatus = 1;
	carrierFlagSet.studyRouting   = 0;  
  carrierFlagSet.msSetAddr      = 0;
  carrierFlagSet.checkAddrClear = 0;
  
  carrierFlagSet.workStatus     = 0;
  carrierFlagSet.routeLeadCopy  = 0;   //默认为集中器主导抄表

  carrierFlagSet.setupNetwork   = 0;
  carrierFlagSet.wlNetOk        = 0;
  
	carrierFlagSet.searchMeter    = 0;
	carrierFlagSet.init           = 0;
	carrierFlagSet.lastNumInNet   = 0;
	
	carrierFlagSet.numOf253       = 0;
	carrierFlagSet.numOfCarrierSending = 0;
	carrierFlagSet.setDateTime    = 0;
	carrierFlagSet.readStatus     = 0;
	
	carrierFlagSet.cmdType        = CA_CMD_NONE;
	
	carrierFlagSet.reStartPause   = 0;
	
	carrierFlagSet.batchCopy      = 0;

	carrierFlagSet.setPanId       = 0;   //2012-07-21,add
	
	carrierFlagSet.chkNodeBeforeCopy=0;  //2013-12-30,add
}

/*******************************************************
函数名称:sendCarrierFrame
功能描述:向载波通信模块发送抄表数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void sendCarrierFrame(INT8U port, INT8U *pack, INT16U length)
{
  INT8U i;   
  INT8U meterFramexx[254];

  //if (port==PORT_POWER_CARRIER)
  //{
  //   meterFramexx[0] = copyCtrl[4].protocol;  //规约类型
  //   meterFramexx[1] = length;
  //   memcpy(&meterFramexx[2], pack, length);
             	 	  
  //   gdw3762Framing(ROUTE_DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
  //}

  //if (port==PORT_POWER_CARRIER)
  if (port<PORT_POWER_CARRIER+3)  //ly,2012-01-09
  {
    //如果是东软I型采集器
    if ((carrierModuleType==EAST_SOFT_CARRIER) && (copyCtrl[4].ifCollector==1))
    {
      meterFramexx[0] = copyCtrl[4].protocol;  //规约类型
      meterFramexx[1] = length+6;
      
      if (copyCtrl[4].protocol==1)    //97规约表
      {
        *(pack+9)  = 0x08;
        *(pack+12) = *(pack+1)+0x33;
        *(pack+13) = *(pack+2)+0x33;
        *(pack+14) = *(pack+3)+0x33;
        *(pack+15) = *(pack+4)+0x33;
        *(pack+16) = *(pack+5)+0x33;
        *(pack+17) = *(pack+6)+0x33;
        memcpy(pack+1, carrierFlagSet.destAddr, 6);
        
        //Checksum
        *(pack+18) = 0x00;
        for (i = 0; i < 18; i++)
        {
           *(pack+18) += *(pack+i);
        }
        *(pack+19) = 0x16;
      }
      else
      {
        *(pack+9)  = 0x0a;
        
        *(pack+14) = *(pack+1)+0x33;
        *(pack+15) = *(pack+2)+0x33;
        *(pack+16) = *(pack+3)+0x33;
        *(pack+17) = *(pack+4)+0x33;
        *(pack+18) = *(pack+5)+0x33;
        *(pack+19) = *(pack+6)+0x33;
        memcpy(pack+1, carrierFlagSet.destAddr, 6);
      
        //Checksum
        *(pack+20) = 0x00;
        for (i = 0; i < 20; i++)
        {
           *(pack+20) += *(pack+i);
        }
        *(pack+21) = 0x16;
      }
      
      memcpy(&meterFramexx[2], pack, length+6);
    }
    else    //载波表或是其它I型采集器
    {
      meterFramexx[0] = copyCtrl[port-27].protocol;  //规约类型
      
      meterFramexx[1] = length;
      memcpy(&meterFramexx[2], pack, length);
    }
    
    //无线模块用数据转发
    if (carrierModuleType==HWWD_WIRELESS
    	  || carrierModuleType==SR_WIRELESS 
    	   || carrierModuleType==RL_WIRELESS
      	  || carrierModuleType==FC_WIRELESS
      	   || carrierModuleType==SC_WIRELESS
    	 )
    {
      gdw3762Framing(DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
    }
    else                                   //载波模块用监控载波从节点
    {
      if (carrierFlagSet.routeLeadCopy==1)  //路由主导抄表
      {
      	//if (carrierFlagSet.workStatus==5)
      	if (carrierFlagSet.workStatus==5 || carrierFlagSet.workStatus==6)
      	{
          if (debugInfo&PRINT_CARRIER_DEBUG)
  	  	  {
  	  	   	printf("路由主导抄读:点抄、转发等都用监控载波从节点命令\n");
  	  	  }
          gdw3762Framing(ROUTE_DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
      	}
      	else
      	{
          if (debugInfo&PRINT_CARRIER_DEBUG)
  	  	  {
  	  	   	 printf("路由主导抄读:本表可以抄读\n");
  	  	  }
      	  meterFramexx[0] = 0x02;         //可以抄读
          if ((carrierModuleType==EAST_SOFT_CARRIER) && (copyCtrl[port-27].ifCollector==1))
          {
            meterFramexx[length+8] = 0x00;     //载波从节点附属节点数量为0
          }
          else
          {
            meterFramexx[length+2] = 0x00;     //载波从节点附属节点数量为0
            
            //载波相位(鼎信载波需要),ly,2012-01-09
            if (carrierModuleType==TC_CARRIER)
            {
              meterFramexx[length+3] = port-30;
            }
          }
          
          if (carrierModuleType==TC_CARRIER)   //ly,2012-01-10
          {
            gdw3762Framing(ROUTE_DATA_READ_3762, 1, copyCtrl[port-27].destAddr, meterFramexx);
          }
          else
          {
            gdw3762Framing(ROUTE_DATA_READ_3762, 1, carrierFlagSet.destAddr, meterFramexx);
          }
        }
      }
      else
      {
        gdw3762Framing(ROUTE_DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
      }
    }
  }
}

/***************************************************
函数名称:dotCopyReply
功能描述:点抄回复
调用函数:
被调用函数:
输入参数:INT8U hasData,是否有数据返回(1-有数,0-异常应答或是无返回)
输出参数:
返回值：void
***************************************************/
void dotCopyReply(INT8U hasData)
{
  struct dotFn129 *tmpPfn129;
   
  INT16U tmpI;
  INT8U  tmpBuf[200];
  char   str[12];
  INT16U frameTail;
  INT8U  buf[1024];
   
  if (pDotCopy==NULL)
  {
   	return;
  }

  if (pDotCopy->port==PORT_POWER_CARRIER)
  {
    //集中器主导抄表
    if (copyCtrl[4].meterCopying==TRUE && carrierFlagSet.routeLeadCopy==0)
    {
      if (copyCtrl[4].ifCopyLastFreeze==3)
      {
        //检查是否有未采集的小时冻结数据
        checkHourFreezeData(copyCtrl[4].tmpCpLink->mp, copyCtrl[4].dataBuff);
      }
        
      initCopyDataBuff(4, copyCtrl[4].ifCopyLastFreeze);
    }

    switch (pDotCopy->dataFrom)
    {
      case DOT_CP_SINGLE_MP:    //按键单测量点点抄
      	if (hasData==1)
      	{
      	  singleMeterCopyReport(pDotCopy->data);
      	}
      	else
      	{
      	  pDotCopy->dotRecvItem = pDotCopy->dotTotalItem;
      	  singleMeterCopyReport(NULL);
      	}
      	 
      	//路由主导抄读,恢复抄表
      	if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
      	{
       	  carrierFlagSet.workStatus = 6;
          carrierFlagSet.reStartPause = 3;
          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
       
          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,按键单测量点点抄 - 点抄完毕,路由主导抄表,%d秒后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
     	    }
        }
         
        free(pDotCopy);
        pDotCopy = NULL;
         
      	break;
      	 	 
      case DOT_CP_ALL_MP:    //按键全体测量点点抄
      	if (hasData==1)
      	{
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,按键全体测量点点抄 - 测量点%d点抄成功\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, prevMpCopy->mp);
     	    }
      	 	
          if (*(pDotCopy->data+21)==0xee)
          {
            prevMpCopy->copyTime[0] = 0xee;
            prevMpCopy->copyTime[1] = 0xee;
          }
          else
          {
            prevMpCopy->copyTime[0] = bcdToHex(*(pDotCopy->data+21));
            prevMpCopy->copyTime[1] = bcdToHex(*(pDotCopy->data+22));
          }
             
          memcpy(prevMpCopy->copyEnergy, pDotCopy->data, 4);
             
          allMpCopy(0xff);
        }
        else
        {
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,按键全体测量点点抄 - 测量点%d点抄失败\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, prevMpCopy->mp);
     	    }
        }
      	 	 
      	if (prevMpCopy->next==NULL)
      	{
      	  //路由主导抄表,恢复抄表
      	  if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
      	  {
         	  carrierFlagSet.workStatus = 6;
            carrierFlagSet.reStartPause = 3;
            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
         
            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,按键全体测量点点抄 - 点抄完毕,路由主导抄表,%d秒后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
     	      }
          }

      	 	free(pDotCopy);
          pDotCopy = NULL;
      	}
      	else
      	{
          pDotCopy->dotCopying = FALSE;
          prevMpCopy = prevMpCopy->next;
      	 	pDotCopy->dotCopyMp   = prevMpCopy->mp;
          pDotCopy->port        = PORT_POWER_CARRIER;
          pDotCopy->dataFrom    = DOT_CP_ALL_MP;
          
          //2013-12-27,鼎信要求等待时间为90s以上
          //  因此将所有模块都统一到这个等待最长的时间上
          //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
          pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);

          pDotCopy->dotRecvItem = 0;
          pDotCopy->dotResult   = RESULT_NONE;

     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,按键全体测量点点抄 - 链表移位->测量点%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, prevMpCopy->mp);
     	    }
      	}
      	break;

      case DOT_CP_NEW_METER:      //按键点抄发现的新表
      	if (hasData==1)
      	{
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,新表点抄成功\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
          }

          prevFound->copyTime[0] = sysTime.minute;
          prevFound->copyTime[1] = sysTime.hour;
             
          memcpy(prevFound->copyEnergy, pDotCopy->data, 4);             
             
          newMeterCpStatus(0);
        }
      	 
      	if (prevFound->next==NULL)
      	{
      	  if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
      	  {
         	  carrierFlagSet.workStatus = 6;
            carrierFlagSet.reStartPause = 3;
            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     
         
            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,按键点抄发现的新表 - 点抄完毕,路由主导抄表,%d秒后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
     	      }
      	  }

      	 	free(pDotCopy);
          pDotCopy = NULL;
      	}
      	else
      	{
          prevFound = prevFound->next;
          memcpy(pDotCopy->addr, prevFound->addr, 6);
          pDotCopy->dotCopying = FALSE;
          pDotCopy->port       = PORT_POWER_CARRIER;
          pDotCopy->dataFrom   = DOT_CP_NEW_METER;

          //2013-12-27,鼎信要求等待时间为90s以上
          //  因此将所有模块都统一到这个等待最长的时间上
          //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
          pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
      	}
      	break;
      	 
      case DATA_FROM_GPRS:    //主站命令点抄(来自远程通信模块)
      case DATA_FROM_LOCAL:   //来自本地通信口
        tmpBuf[0] = 0x68;            //帧起始字符
      
        tmpBuf[5] = 0x68;            //帧起始字符
     
        tmpBuf[6] = 0x88;            //控制字节10001000(DIR=1,PRM=0,功能码=0x8)
     
        //地址
        tmpBuf[7]  = addrField.a1[0];
        tmpBuf[8]  = addrField.a1[1];
        tmpBuf[9]  = addrField.a2[0];
        tmpBuf[10] = addrField.a2[1];
        tmpBuf[11] = addrField.a3;
        tmpBuf[12] = 0x0C;           //AFN
         
        if (pDotFn129!=NULL)
        {
          if (dotReplyStart==0)
          {
            if (pDotFn129->next==NULL)
            {
              tmpBuf[13] = 0x60 | rSeq;  //01100001 单帧
               
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 单帧\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
            else
            {
             	tmpBuf[13] = 0x40 | rSeq;  //首帧
               
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 首帧\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
             
            dotReplyStart = 1;
          }
          else
          {
            if (pDotFn129->next==NULL)
            {
              tmpBuf[13]   = 0x20 | rSeq;  //末帧

              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 末帧\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
            else
            {
              tmpBuf[13]   = 0x00 | rSeq;  //中间帧
              
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 中间帧\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
          }
        }
        else
        {
          tmpBuf[13]   = 0x60 | rSeq;  //01100001 单帧
           
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 单帧未知\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
          }
        }
        
        tmpBuf[14] = 0x01<<((pDotCopy->dotCopyMp%8 == 0) ? 7 : (pDotCopy->dotCopyMp%8-1)); //DA1
        tmpBuf[15] = (pDotCopy->dotCopyMp - 1) / 8 + 1;                                   	//DA2

        tmpBuf[16] = 0x1;            //DT1
        tmpBuf[17] = 0x10;           //DT2
     	  tmpBuf[18] = hexToBcd(sysTime.minute);
     	  tmpBuf[19] = hexToBcd(sysTime.hour);
     	  tmpBuf[20] = hexToBcd(sysTime.day);
     	  tmpBuf[21] = hexToBcd(sysTime.month);
     	  tmpBuf[22] = hexToBcd(sysTime.year);
     
     	  frameTail = 23;
     	  if (pDotCopy->protocol==DLT_645_2007)
     	  {
     	   #ifdef DKY_SUBMISSION
     	    tmpBuf[frameTail++] = 0x4;  //费率个数为4个
     	   #else
     	    tmpBuf[frameTail++] = 0x0;  //费率个数为0个
     	   #endif
     	 
     	    if (hasData==0)
     	    {
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 07表点抄无数\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	      }
     	       
     	     #ifdef DKY_SUBMISSION
     	      for(tmpI=0;tmpI<5;tmpI++)
     	     #else
     	      for(tmpI=0;tmpI<1;tmpI++)
     	     #endif
     	      {
     	        tmpBuf[frameTail++] = 0xee;
     	        tmpBuf[frameTail++] = 0xee;
     	        tmpBuf[frameTail++] = 0xee;
     	        tmpBuf[frameTail++] = 0xee;
     	        tmpBuf[frameTail++] = 0xee;
     	      }
     	    }
     	    else
     	    {
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 07表点抄有数回复\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	      }
     	    
     	     #ifdef DKY_SUBMISSION
     	      for(tmpI=0;tmpI<5;tmpI++)
     	     #else
     	      for(tmpI=0;tmpI<1;tmpI++)
     	     #endif
     	      {
     	        if (pDotCopy->data[tmpI*4+0]==0xee)
     	        {
     	          tmpBuf[frameTail++] = 0xee;
     	        }
     	        else
     	        {
     	          tmpBuf[frameTail++] = 0x0;
     	        }
     	        tmpBuf[frameTail++] = pDotCopy->data[tmpI*4+0];
     	        tmpBuf[frameTail++] = pDotCopy->data[tmpI*4+1];
     	        tmpBuf[frameTail++] = pDotCopy->data[tmpI*4+2];
     	        tmpBuf[frameTail++] = pDotCopy->data[tmpI*4+3];
     	      }
     	    }
     	  }
     	  else
     	  {
     	    //ly,2011-01-12,97表点抄回复取消费率,费率数为0,
     	    tmpBuf[frameTail++] = 0x0;  //费率个数
     	 
     	    if (hasData==0)
     	    {
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 97表点抄无数\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	      }
     	    
     	      tmpBuf[frameTail++] = 0xee;
     	      tmpBuf[frameTail++] = 0xee;
     	      tmpBuf[frameTail++] = 0xee;
     	      tmpBuf[frameTail++] = 0xee;
     	      tmpBuf[frameTail++] = 0xee;
     	    }
     	    else
     	    {
     	      if (debugInfo&METER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 97表点抄有数回复\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	      }
     	    
     	      tmpBuf[frameTail++] = 0x0;
     	      tmpBuf[frameTail++] = pDotCopy->data[0];
     	      tmpBuf[frameTail++] = pDotCopy->data[1];
     	      tmpBuf[frameTail++] = pDotCopy->data[2];
     	      tmpBuf[frameTail++] = pDotCopy->data[3];
     	    }
     	     
     	    //ly,2011-01-12,97表点抄回复取消费率,费率数为0,
     	    //tmpBuf[frameTail++] = 0xee;
     	    //tmpBuf[frameTail++] = 0xee;
     	    //tmpBuf[frameTail++] = 0xee;
     	    //tmpBuf[frameTail++] = 0xee;
     	    //tmpBuf[frameTail++] = 0xee;
     	  }

        tmpI = ((frameTail - 6) << 2) | 0x2;
        tmpBuf[1] = tmpI & 0xFF;     //L
        tmpBuf[2] = tmpI >> 8;
        tmpBuf[3] = tmpI & 0xFF;     //L
        tmpBuf[4] = tmpI >> 8; 
     	 
     	  tmpBuf[frameTail] = 0;
     	  for(tmpI=6;tmpI<frameTail;tmpI++)
     	  {
     	 	  tmpBuf[frameTail] += tmpBuf[tmpI];
     	  }
     	  frameTail++;
        tmpBuf[frameTail++] = 0x16;
        
        switch(pDotCopy->dataFrom)
        {
        	case DATA_FROM_GPRS:
            if (bakModuleType==MODEM_PPP)  //2012-11-08,add
            {
              sendWirelessFrame(tmpBuf,frameTail);
            }
            else
            {
              switch(moduleType)
              {
                case GPRS_GR64:
                case GPRS_SIM300C:
                case CDMA_DTGS800:
                case ETHERNET:
                case GPRS_M72D:
							  case LTE_AIR720H:
                  write(fdOfModem,tmpBuf,frameTail);
                  break;
           
                case GPRS_MC52I:
                case GPRS_M590E:
                case CDMA_CM180:
       	          wlModemRequestSend(tmpBuf,frameTail);
                  break;
              }
            }
            break;
             
          case DATA_FROM_LOCAL:
           #ifdef WDOG_USE_X_MEGA
            buf[0] = frameTail&0xff;
            buf[1] = frameTail>>8&0xff;
            memcpy(&buf[2], tmpBuf, frameTail);
            sendXmegaFrame(MAINTAIN_DATA, buf, frameTail+2);  
           #endif
           	break;
        }
      	 
      	//2013-12-27,判断中添加&& pDotFn129->next==NULL
      	if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE && pDotFn129->next==NULL)
      	{
      	  carrierFlagSet.workStatus = 6;
          carrierFlagSet.reStartPause = 3;
          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     
      
          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 点抄完毕,路由主导抄表,%d秒后恢复抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
     	    }
        }

        free(pDotCopy);
        pDotCopy = NULL;
         
        //ly,2011-05-13
        tmpPfn129 = pDotFn129;
        if (pDotFn129!=NULL)
        {
          pDotFn129 = pDotFn129->next;

     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            if (pDotFn129!=NULL)
            {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,远程点抄 - 移向测量点%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, pDotFn129->pn);
            }
     	    }
        }
        free(tmpPfn129);
         
        break;
     }
  }
}

/***************************************************
函数名称:dotCopy
功能描述:点抄
调用函数:
被调用函数:
输入参数:INT8U port,抄表端口
输出参数:
返回值：void
***************************************************/
BOOL dotCopy(INT8U port)
{
	 METER_DEVICE_CONFIG  meterConfig;
   struct meterCopyPara mcp;
   
  #ifdef LIGHTING 
   INT8U                slSendBuf[20];
   INT8U                i;
  #endif

   if (pDotCopy==NULL)
   {
   	 return FALSE;
   }
   
   if (stopCarrierNowWork()==TRUE)
   {
   	 return FALSE;
   }   
   
   switch(pDotCopy->port)
   {
   	 case PORT_POWER_CARRIER:
   	 	 if(pDotCopy->dotResult>RESULT_NONE && pDotCopy->dotRecvItem>=pDotCopy->dotTotalItem)
   	 	 {
   	 	 	 if (pDotCopy->dotResult==RESULT_HAS_DATA)
   	 	 	 {
   	 	 	   dotCopyReply(1);
   	 	 	 }
   	 	 	 else
   	 	 	 {
     	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	     {
             printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,点抄无数\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	     }

   	 	 	   dotCopyReply(0);
   	 	 	 }
   	 	 	  
   	 	 	 if (pDotCopy!=NULL)
   	 	 	 {
   	 	 	   pDotCopy->dotResult = RESULT_NONE;
   	 	 	 }
   	 	 	 
   	 	 	 return FALSE;
   	 	 }
   	 	 
   	 	 if (pDotCopy->dotCopying==FALSE)
   	 	 {
     	 	 pDotCopy->dotCopying = TRUE;
     	 	 
     	 	 switch(pDotCopy->dataFrom)
   	 	   {
   	 	 	   case DATA_FROM_LOCAL:   //主站命令点抄(来自串口)
   	 	 	   case DATA_FROM_GPRS:    //主站命令点抄(来自远程通信Modem)
   	 	 	   case DOT_CP_SINGLE_MP:  //单测量点按键点抄
   	 	 	   case DOT_CP_ALL_MP:     //多测量点按键点抄
             if (selectF10Data(pDotCopy->dotCopyMp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
             {
          	   //未配置该测量点的信息,不理会
          	   dotCopyReply(0);
          	   return FALSE;
             }
             else
             {
               if (debugInfo&METER_DEBUG)
               {
     	           if (debugInfo&PRINT_CARRIER_DEBUG)
     	           {
                   printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,开始点抄,超时等待%d秒\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, MONITOR_WAIT_ROUTE);
     	           }
               }
               
               //2013-12-30,add,防止前一类点抄在等待恢复抄表时间之前又有另一类抄表命令,造成反复恢复和暂停
               //          鼎信模块还不支持发了点抄命令他没回时又发恢复命令,这会造成程序跑飞
               if (1==carrierFlagSet.routeLeadCopy && copyCtrl[4].meterCopying==TRUE)
               {
             	   carrierFlagSet.workStatus = 5;
                 carrierFlagSet.reStartPause = 0;

     	           if (debugInfo&PRINT_CARRIER_DEBUG)
     	           {
                   printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,路由主导抄读 - 重置集中器的载波模块工作状态为5(抄表暂停)\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	           }
               }

             	#ifdef LIGHTING
               
               memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
               copyCtrl[4].ifCollector = 0;
               copyCtrl[4].protocol = 2;

 	 	       	   //组645帧
 	 	       	   slSendBuf[0] = 0x68;
 	 	       	   memcpy(&slSendBuf[1], meterConfig.addr, 6);
 	 	       	   slSendBuf[7] = 0x68;
 	 	       	   slSendBuf[8] = 0x11;          //C
 	 	       	   slSendBuf[9] = 0x04;          //L
 	 	       	   slSendBuf[10] = 0x00+0x33;
 	 	       	   slSendBuf[11] = 0x00+0x33;
 	 	       	   slSendBuf[12] = 0x90+0x33;
 	 	       	   slSendBuf[13] = 0x00+0x33;

 	 	       	   //CS
 	 	       	   slSendBuf[14] = 0x00;
 	 	       	   for(i=0; i<14; i++)
 	 	       	   {
 	 	       	   	 slSendBuf[14] += slSendBuf[i];
 	 	       	   }
 	 	       	   slSendBuf[15] = 0x16;
 	 	       	   
 	 	       	   sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, 16);
               
               pDotCopy->dotRetry     = 0;
               pDotCopy->dotResult    = RESULT_NONE;
               pDotCopy->dotTotalItem = 1;
               pDotCopy->dotRecvItem  = 0;
               
              #else 
               
               mcp.port = PORT_POWER_CARRIER;
               mcp.send = sendCarrierFrame;
               if (meterConfig.protocol==DLT_645_1997)
               {
                 mcp.protocol = DOT_COPY_1997;
                 copyCtrl[4].protocol = 1;
               }
               else
               {
                 mcp.protocol = DOT_COPY_2007;
                 copyCtrl[4].protocol = 2;
               }
               pDotCopy->protocol = meterConfig.protocol;
               mcp.pn             = pDotCopy->dotCopyMp;
               memcpy(mcp.meterAddr,meterConfig.addr,6);
               mcp.copyTime     = timeHexToBcd(sysTime);
 
               mcp.copyDataType = PRESENT_DATA;
               
               memset(pDotCopy->data,0xee,128);
               mcp.dataBuff     = pDotCopy->data;
               mcp.save         = saveMeterData;
               
               //如果采集器地址为全0,则载波目的节点为表地址,否则目的节点为采集器地址
               if (meterConfig.collectorAddr[0]==0x00 && meterConfig.collectorAddr[1]==0x00
             	    && meterConfig.collectorAddr[2]==0x00 && meterConfig.collectorAddr[3]==0x00
             	     && meterConfig.collectorAddr[4]==0x00 && meterConfig.collectorAddr[5]==0x00
             	    )
               {
                  memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
                  copyCtrl[4].ifCollector = 0;
               }
               else
               {
                  memcpy(carrierFlagSet.destAddr, meterConfig.collectorAddr, 6);
                  copyCtrl[4].ifCollector = 1;
               }
             
               initCopyProcess(&mcp);
               
               pDotCopy->dotRetry     = 0;
               pDotCopy->dotResult    = RESULT_NONE;
               if (pDotCopy->dataFrom==DATA_FROM_GPRS || pDotCopy->dataFrom==DATA_FROM_LOCAL)
               {
                 pDotCopy->dotTotalItem = 1;
               }
               else
               {
                 pDotCopy->dotTotalItem = 2;
               }
               pDotCopy->dotRecvItem  = 0;

               meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //组帧发送
               
              #endif
               
               //2013-12-27,鼎信要求等待时间为90s以上
               //  因此将所有模块都统一到这个等待最长的时间上
               //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
               pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);

               return TRUE;
   	 	 	     }
   	 	 	     break;
   	 	 	   
   	 	 	   case DOT_CP_NEW_METER:   //抄新增电能表
   	 	 	   	 mcp.port     = PORT_POWER_CARRIER;
             mcp.send     = sendCarrierFrame;
             mcp.protocol = SINGLE_PHASE_645_1997;
             
             //ly,2010-03-18,由于晓程上报的表号中的规约无论是97的还是07的都02(即07规约),所以无法根据这个判断点抄时的规约
             //copyCtrl[4].protocol = 1;
             mcp.protocol = DOT_COPY_2007;
             copyCtrl[4].protocol = 2;
             
             pDotCopy->dotRetry     = 0;
             pDotCopy->dotResult    = RESULT_NONE;
             pDotCopy->dotTotalItem = 1;
             pDotCopy->dotRecvItem  = 0;

             mcp.pn       = pDotCopy->dotCopyMp;
             memcpy(mcp.meterAddr,pDotCopy->addr,6);
             memcpy(carrierFlagSet.destAddr, pDotCopy->addr, 6);
             mcp.copyTime = timeHexToBcd(sysTime);
             mcp.copyDataType = PRESENT_DATA;
             mcp.dataBuff = pDotCopy->data;
             mcp.save = saveMeterData;
             
             initCopyProcess(&mcp);

     	       if (debugInfo&PRINT_CARRIER_DEBUG)
     	       {
               printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,开始点抄新表,超时等待%d秒\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, MONITOR_WAIT_ROUTE);
     	       }
             
             meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //组帧发送

             //2013-12-27,鼎信要求等待时间为90s以上
             //  因此将所有模块都统一到这个等待最长的时间上
             //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
             pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
             
             //2013-12-27,add,防止前一类点抄在等待恢复抄表时间之前又有另一类抄表命令,造成反复恢复和暂停
             //          鼎信模块还不支持发了点抄命令他没回时又发恢复命令,这会造成程序跑飞
             if (1==carrierFlagSet.routeLeadCopy && copyCtrl[4].meterCopying==TRUE)
             {
             	 carrierFlagSet.workStatus = 5;
               carrierFlagSet.reStartPause = 0;
     	         
     	         if (debugInfo&PRINT_CARRIER_DEBUG)
     	         {
                 printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,点抄新表,路由主导抄读 - 重置集中器的载波模块工作状态为5(抄表暂停)\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	         }
             }

             return TRUE;
   	 	 	 	   break;
   	 	   }
   	 	 }
   	 	 else    //点抄命令已发送后判断是否超时
   	 	 {
   	 	 	 //判断超时
   	 	 	 if (compareTwoTime(pDotCopy->outTime, sysTime))
   	 	 	 {
 	    		 if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		 {
 	    		   printf("%02d-%02d-%02d %02d:%02d:%02d:dotCopy,超时或异常应答,dotRecvItem=%d,dotTotalItem=%d,dotRetry=%d",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pDotCopy->dotRecvItem,pDotCopy->dotTotalItem,pDotCopy->dotRetry);
 	    		 }

 	    	   if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
 	    	   {
             pDotCopy->dotRetry++;
   	    	   if (pDotCopy->dotRetry<1)
   	    	   {
 	    		     if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		     {
 	    		     	 printf(",重试上一项");
 	    		     }
               
               meterFraming(PORT_POWER_CARRIER, LAST_COPY_ITEM_RETRY); //重发上一项
   	    	   }
   	    	   else
   	    	   {
   	    	     pDotCopy->dotRecvItem++;
   	    	     pDotCopy->dotRetry = 0;
 	    	     
 	    		     if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		     {
 	    		     	 printf(",组帧发送新数据项");
 	    		     }
 	    		     
               meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);        //组帧发送
             }
             
             //2013-12-27,鼎信要求等待时间为90s以上
             //  因此将所有模块都统一到这个等待最长的时间上
             //  在此之前,弥亚微等待最长42秒,其他模块都是等待30秒        
             pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
 	    		   
 	    		   if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		   {
 	    		     printf(",超时等待%d秒", MONITOR_WAIT_ROUTE);
 	    		   }
 	    	   }
 	    	   else
 	    	   {
             switch (pDotCopy->dataFrom)
             {
               case DOT_CP_SINGLE_MP:   //按键单测量点点抄及时汇报测量结果
    	         case DOT_CP_ALL_MP:
    	         case DATA_FROM_GPRS:
    	         case DATA_FROM_LOCAL:
    	         case DOT_CP_NEW_METER:
          	 	 	 if (pDotCopy->dotResult==RESULT_HAS_DATA)
          	 	 	 {
          	 	 	   dotCopyReply(1);
          	 	 	 }
          	 	 	 else
          	 	 	 {
          	 	 	   dotCopyReply(0);
          	 	 	 }
    	         	 break;
    	       }
 	    	   }
 	    		 
 	    		 if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		 {
 	    		   printf("\n");
 	    		 }

  	 	 	 	 return FALSE;
   	 	 	 }   	 	 	  
   	 	 }
   	 	 
   	 	 break;
   }
   
   return FALSE;
}

/***************************************************
函数名称:checkCarrierMeter
功能描述:检查载波表抄表情况
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void checkCarrierMeter(void)
{
  DATE_TIME tmpTime;
  INT8U     tmpBuf[512];
  INT8U     meterInfo[10];
  
  while(copyCtrl[4].cpLinkHead!=NULL)
  {
  	 copyCtrl[4].tmpCpLink = copyCtrl[4].cpLinkHead;
  	 copyCtrl[4].cpLinkHead = copyCtrl[4].cpLinkHead->next;
  	 free(copyCtrl[4].tmpCpLink);
  }
  
  if ((copyCtrl[4].cpLinkHead=initPortMeterLink(30))!=NULL)
  {
   #ifndef LIGHTING
   
    copyCtrl[4].tmpCpLink = copyCtrl[4].cpLinkHead;
    while(copyCtrl[4].tmpCpLink!=NULL)
    {
      #ifdef CQDL_CSM
        
        if (checkLastDayData(copyCtrl[4].tmpCpLink->mp, 0, sysTime, 1)==TRUE)
        {
        	 copyCtrl[4].tmpCpLink->copySuccess = TRUE;
        }
                
      #else
        
        queryMeterStoreInfo(copyCtrl[4].tmpCpLink->mp, meterInfo);
        
        tmpTime = timeHexToBcd(sysTime);
      	if (readMeterData(tmpBuf, copyCtrl[4].tmpCpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0))
      	{
      	   if (tmpBuf[0]!=0xee)
      	   {
      	   	  copyCtrl[4].tmpCpLink->copySuccess = TRUE;
      	   	  
      	   	  if (debugInfo&METER_DEBUG)
      	   	  {
      	   	    printf("测量点:%d抄表成功(有实时数据)\n",copyCtrl[4].tmpCpLink->mp);
      	   	  }
      	   }
      	   else
      	   {
      	   	  copyCtrl[4].tmpCpLink->copySuccess = FALSE;
      	   	  
      	   	  if (debugInfo&METER_DEBUG)
      	   	  {
      	   	    printf("测量点:%d抄表未成功(无实时数据)\n",copyCtrl[4].tmpCpLink->mp);
      	   	  }
      	   }
      	}
      	else
      	{
      	   copyCtrl[4].tmpCpLink->copySuccess = FALSE;
      	   
           //2012-08-28,add,如果没有实时数据查找是否有冻结数据
           if (checkLastDayData(copyCtrl[4].tmpCpLink->mp, 0, sysTime, 1)==TRUE)
           {
      	     copyCtrl[4].tmpCpLink->copySuccess = TRUE;
           }
           else
           {
      	     if (debugInfo&METER_DEBUG)
      	     {
      	       printf("测量点:%d抄表未成功(无数)\n",copyCtrl[4].tmpCpLink->mp);
      	     }
      	   }
      	}
      #endif
    	
    	copyCtrl[4].tmpCpLink = copyCtrl[4].tmpCpLink->next;
    	
    	usleep(100000);
    }
    
   #endif
  }
}

/***************************************************
函数名称:checkHourFreezeData
功能描述:检查当日整点冻结数据是否齐全,如果有缺失则采集
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
INT8U checkHourFreezeData(INT16U pn, INT8U *lostBuff)
{
	 DATE_TIME tmpTime;
   INT8U     tmpReadBuff[LENGTH_OF_ENERGY_RECORD];
   INT16U    i, j, tmpPn;
   INT8U     tmpHour, k, tmpPnx, tmpData;
   BOOL      ifFound;
   
   //查找任务中是否配置该测量点的正向及反向示值曲线,有配置才采集整点冻结数据
   ifFound = FALSE;
   for (i = 0; i < 64; i++)
   {
   	 //2012-09-06,添加要启动了小时冻结任务才采集小时冻结数据
   	 //if(reportTask2.task[i].taskNum != 0)
   	 if(reportTask2.task[i].taskNum != 0 && reportTask2.task[i].stopFlag==TASK_START)
   	 {
   	 	 for (j=0;j<reportTask2.task[i].numOfDataId;j++)
   	 	 {
   	 	 	 if (reportTask2.task[i].dataUnit[j].fn==101 || reportTask2.task[i].dataUnit[j].fn==102)
   	 	 	 {
   	 	 	 	 if (!(reportTask2.task[i].dataUnit[j].pn1==0 || reportTask2.task[i].dataUnit[j].pn2==0))
   	 	 	 	 {
   	 	 	 	   tmpPnx = 1;
   	 	 	 	   tmpData = 1;
   	 	 	 	   for(k=0;k<8;k++)
   	 	 	 	   {
   	 	 	 	     if (reportTask2.task[i].dataUnit[j].pn1&tmpData)
   	 	 	 	     {
   	 	 	 	     	  break;
   	 	 	 	     }
   	 	 	 	     tmpData<<=1;
   	 	 	 	     tmpPnx++;
   	 	 	 	   }
   	 	 	 	   
   	 	 	 	   tmpPn = (reportTask2.task[i].dataUnit[j].pn2-1)*8+tmpPnx;
   	 	 	 	   if (tmpPn==pn)
   	 	 	 	   {
   	 	 	 	   	 if (debugInfo&METER_DEBUG)
   	 	 	 	   	 {
   	 	 	 	   	   printf("在任务%d中找到测量点%d\n",reportTask2.task[i].taskNum,tmpPn);
   	 	 	 	   	 }
   	 	 	 	   	 ifFound = TRUE;
   	 	 	 	   	 i=64;
   	 	 	 	   	 break;
   	 	 	 	   }
   	 	 	 	 }
   	 	 	 }
   	 	 }
     }
   }//if
   
   if (ifFound==FALSE)
   {
     if (debugInfo&METER_DEBUG)
     {
     	 printf("未在任务中找到测量点%d整点冻结示值的配置,不采集该测量点整点冻结数据\n", pn);
     }
   	 
   	 return 0;
   }
   
   if (debugInfo&METER_DEBUG)
   {
     printf("查询测量点%d(%02d-%02d-%2d)整点冻结数据\n", pn, sysTime.year, sysTime.month, sysTime.day);
   }
   
   tmpHour = sysTime.hour;
   for(i=0,j=0;i<=tmpHour;i++)
   {
     tmpTime = sysTime;
     tmpTime.second = 0x0;
     tmpTime.minute = 0x0;
     tmpTime.hour = tmpHour - i;
     tmpTime = timeHexToBcd(tmpTime);
     if (readMeterData(tmpReadBuff, pn, HOUR_FREEZE, 0x0, &tmpTime, 0) == TRUE)
     {
       if (tmpReadBuff[0] == 0xEE || tmpReadBuff[4] == 0xEE)
       {
       	 if (debugInfo&PRINT_CARRIER_DEBUG)
       	 {
       	   printf("测量点%d %d点整点冻结数据缺失\n", pn, tmpHour - i);
       	 }

       	 lostBuff[j+1] = i+1;
       	 j++;
       }
     }
     else
     {
       if (debugInfo&PRINT_CARRIER_DEBUG)
       {
       	 printf("测量点%d无%d点整点冻结数据\n", pn, tmpHour - i);
       }

       lostBuff[j+1] = i+1;
       j++;
     }
   }
   
   *lostBuff = j;
   
   return j;
}

/***************************************************
函数名称:stopCarrierNowWork
功能描述:停止载波模块当前工作
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
BOOL stopCarrierNowWork(void)
{
   if (copyCtrl[4].cpLinkHead==NULL)
   {
   	 return;
   }
   
   //模块学习路由中...
   if (carrierFlagSet.studyRouting==2 || carrierFlagSet.studyRouting==3)
   {
   	 printf("先停止路由学习\n");
   	  
   	 //停止学习命令
     gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
     carrierFlagSet.studyRouting = 3;
     return TRUE;
   }

   //模块搜表中...
   if (carrierFlagSet.searchMeter==2 || carrierFlagSet.searchMeter==3)
   {
   	 printf("先停止搜表\n");
      
     searchEnd = sysTime;                         //搜索结束时间

   	 //停止学习命令
     gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
     carrierFlagSet.searchMeter = 3;
     return TRUE;
   }
   
   return FALSE;
}

/***************************************************
函数名称:foundMeterEvent
功能描述:发现电表事件记录
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void foundMeterEvent(struct carrierMeterInfo *foundMeter)
{
   INT8U  eventData[512];
   INT16U meterNum = 0;

   if ((eventRecordConfig.iEvent[4] & 0x04) || (eventRecordConfig.nEvent[4] & 0x04))
   {
   	 eventData[0] = 35;    //ERC=35
   	 eventData[1] = 18;
   	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
   	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
   	 eventData[4] = sysTime.hour/10<<4 | sysTime.hour%10;
   	 eventData[5] = sysTime.day/10<<4 | sysTime.day%10;
   	 eventData[6] = sysTime.month/10<<4 | sysTime.month%10;
   	 eventData[7] = sysTime.year/10<<4 | sysTime.year%10;

   	 eventData[8] = 31;      //终端通信端口
   	 
   	 while(foundMeter!=NULL)
   	 {   	 
     	 memcpy(&eventData[10+meterNum*8],foundMeter->addr,6);
     	 eventData[10+meterNum*8+6] = 0xf;    //信号品质
     	 if (foundMeter->protocol==2)
     	 {
     	 	  eventData[10+meterNum*8+7] = 0x1;   //DL/T645-2007
     	 }
     	 else
     	 {
     	 	  eventData[10+meterNum*8+7] = 0x0;   //DL/T645-1997
     	 }
     	 
     	 foundMeter = foundMeter->next;
     	 meterNum++;
     }
     
     eventData[9] = meterNum;       //发现电表块数
     
     if (eventRecordConfig.iEvent[4] & 0x04)
     {
     	  writeEvent(eventData, meterNum*8+10 , 1, DATA_FROM_GPRS);   //记入重要事件队列
     }
     if (eventRecordConfig.nEvent[4] & 0x04)
     {
     	  writeEvent(eventData, meterNum*8+10, 2, DATA_FROM_LOCAL);  //记入一般事件队列
     }
     
     activeReport3();   //主动上报事件

   	 eventStatus[4] = eventStatus[4] | 0x04;
   }
}

#endif

/***************************************************
函数名称:checkLastDayData
功能描述:检查上日电能量数据是否齐全,如果有缺失重新采集
调用函数:
被调用函数:
输入参数:pn-测量点
         type-查询类型,0-查询当前时间的上一次,1-根据queryTime时间来查询
         queryTime-查询时间
         queryType-查询类型,1-当天的,0-历史
输出参数:
返回值：void
***************************************************/
BOOL checkLastDayData(INT16U pn, INT8U type, DATE_TIME queryTime, INT8U queryType)
{
	 DATE_TIME tmpTime;
   INT8U     meterInfo[10];
   INT8U     tmpDataType;
   INT16U    tmpQueryOffset;
   INT8U     tmpReadBuff[LENGTH_OF_ENERGY_RECORD];

   if (type==1)
   {
   	 tmpTime = queryTime;
   }
   else
   {
     //ly,10-10-30
     //由于重庆的日冻结数据为当日0点的数据为本日日冻结数据,因此,检查是否有当日的日冻结数据
     //#ifdef CQDL_CSM
      tmpTime = timeHexToBcd(sysTime);
     //#else
     // tmpTime = timeHexToBcd(backTime(sysTime,0,1,0,0,0));
     //#endif
   }
   tmpTime.second = 0x01;
   tmpTime.minute = 0x01;
   tmpTime.hour   = 0x01;
   
   if (debugInfo&METER_DEBUG)
   {
     printf("查询测量点%d(%02x-%02x-%2x)日冻结数据\n", pn, tmpTime.year, tmpTime.month, tmpTime.day);
   }

   queryMeterStoreInfo(pn, meterInfo);
    
 	 switch (meterInfo[0])
 	 {
 	 	 case 4:
 	 	 case 5:
 	 	 case 6:
 	 	 case 7:
 	 	 case 8:
 	 	 	 tmpDataType    = DAY_FREEZE_COPY_DATA;
   	   tmpQueryOffset = NEGTIVE_WORK_OFFSET;
   	   break;
   	   
   	 default:
   	   tmpDataType    = ENERGY_DATA;
 	  	 tmpQueryOffset = NEGTIVE_WORK_OFFSET_S;
 	  	 break;
 	 }
    
   if (readMeterData(tmpReadBuff, pn, meterInfo[2], tmpDataType, &tmpTime, 0) == TRUE)
   {
     tmpTime = timeBcdToHex(tmpTime);
     if (queryType==1)
     {
       if (tmpTime.day != sysTime.day)
       {
     	   if (debugInfo&METER_DEBUG)
     	   {
     	     printf("测量点%d上一次冻结数据为备份日冻结数据,应采集电表冻结数据\n", pn);
     	   }
     	       	  
     	   return FALSE;
     	 }
     	 
       if (tmpReadBuff[POSITIVE_WORK_OFFSET] == 0xEE || tmpReadBuff[tmpQueryOffset] == 0xEE)
       {
       	  if (debugInfo&METER_DEBUG)
       	  {
       	    printf("测量点%d上一次冻结数据缺失\n", pn);
       	  }
  
       	  return FALSE;
       }
     }
     else
     {
       if (tmpReadBuff[POSITIVE_WORK_OFFSET] == 0xEE)
       {
       	  if (debugInfo&METER_DEBUG)
       	  {
       	    printf("测量点%d上一次冻结数据缺失\n", pn);
       	  }
  
       	  return FALSE;
       }
     }
     
     if (debugInfo&METER_DEBUG)
     {
     	 printf("测量点%d有上一次冻结数据\n", pn);
     }
   }
   else
   {
     if (debugInfo&METER_DEBUG)
     {
     	 printf("测量点%d无上一次冻结数据\n", pn);
     }
     
     return FALSE;
   }
   
   return TRUE;
}

/***************************************************
函数名称:checkLastMonthData
功能描述:检查上月电能量数据是否齐全,如果有缺失重新采集
调用函数:
被调用函数:
输入参数:pn-测量点
         queryTime-查询时间
输出参数:
返回值：void
修改历史:
  1,2012-06-06,修改为只检查上月(/上一结算日)正向有功和反向有功是否齐全
    原因为:单相表只有正向有功和反向有功的数据
***************************************************/
BOOL checkLastMonthData(INT16U pn)
{
   BOOL                hasData;
   DATE_TIME           tmpTime;
	 METER_DEVICE_CONFIG meterConfig;
   INT8U               tmpReadBuff[LENGTH_OF_ENERGY_RECORD];

   tmpTime = timeHexToBcd(sysTime);
   tmpTime.second = 0x1;
   tmpTime.minute = 0x1;
   tmpTime.hour   = 0x1;
   tmpTime.day    = 0x1;
  
   hasData = TRUE;

   //库中没有该测量点的数据
   if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
   {
     return FALSE;
   }
 	 
   //检查上月电能量数据是否齐全，如果有缺失重新采集
   if (readMeterData(tmpReadBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &tmpTime, 0) == TRUE)
   {
     if (tmpReadBuff[POSITIVE_WORK_OFFSET] == 0xEE || tmpReadBuff[NEGTIVE_WORK_OFFSET] == 0xEE
  	   //2012-06-01,发现单相表的上月数据只有正向有功和反向有功,因此改成只判断这两项
  	   //|| tmpReadBuff[POSITIVE_NO_WORK_OFFSET] == 0xEE || tmpReadBuff[NEGTIVE_NO_WORK_OFFSET] == 0xEE
  	   //  || tmpReadBuff[QUA1_NO_WORK_OFFSET] == 0xEE || tmpReadBuff[QUA2_NO_WORK_OFFSET] == 0xEE
  	   //    || tmpReadBuff[QUA3_NO_WORK_OFFSET] == 0xEE || tmpReadBuff[QUA4_NO_WORK_OFFSET] == 0xEE
  	    )
     {
       if (debugInfo&METER_DEBUG)
       {
         printf("测量点%d上月数据电能数据缺失\n", pn);
       }
      
       hasData = FALSE;
     }
   }
   else
   {
     if (debugInfo&METER_DEBUG)
     {
       printf("测量点%d上月数据电能数据查询失败\n",meterConfig.measurePoint);
     }
    
     hasData = FALSE;
   }
   
   return hasData;
}

/***************************************************
函数名称:forwardData
功能描述:数据转发
调用函数:
被调用函数:
输入参数:INT8U port,转发端口在抄表控制数组中的序号
输出参数:
返回值：void
***************************************************/
BOOL forwardData(INT8U portNum)
{
	 METER_DEVICE_CONFIG  meterConfig;
   INT8U  buf[5];
   INT8U  sendBuf[50];
   INT8U  frameTail, i;
   INT16U j;
   INT8U  meterAddr[6];
   INT8U  ifFound=0;
   
   if (copyCtrl[portNum].pForwardData==NULL)
   {
   	  return FALSE;
   }

   if(copyCtrl[portNum].pForwardData->forwardResult>RESULT_NONE)
   {
   	 //等待两秒后回复主站
   	 if (compareTwoTime(copyCtrl[portNum].pForwardData->nextBytesTime, sysTime))
   	 {
   	   forwardDataReply(portNum);
   	 }
   	 
   	 return FALSE;
   }
   
   printf("\b\b\b\b\b");

   if (copyCtrl[portNum].pForwardData->ifSend==FALSE)
   {
     switch(copyCtrl[portNum].pForwardData->fn)
     {
       case 1:   //透明转发
         switch(portNum)
         {
          #ifdef RS485_1_USE_PORT_1
           case 0x0:   //端口1
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x01;                                     //xMega端口1
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //端口转发速率
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("设置端口1转发速率\n");
              }
             #else
              oneUartConfig(fdOfttyS2,copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("端口1开始透明转发\n");
             }
             
             sendMeterFrame(PORT_NO_1,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);             
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             break;
       	 
           case 0x1:   //端口2
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x02;                                     //xMega端口2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //端口转发速率
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("设置端口2转发速率\n");
              }
              
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #else
              oneUartConfig(fdOfttyS3, copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
             #endif
             
             if (debugInfo&METER_DEBUG)
             {
               printf("端口2开始透明转发\n");
             }

             sendMeterFrame(PORT_NO_2,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif             
             break;
             
           case 0x2:   //端口3
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x03;                                     //xMega端口2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //端口转发速率
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("设置端口3转发速率\n");
              }
              
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             
             if (debugInfo&METER_DEBUG)
             {
               printf("端口3开始透明转发\n");
             }

             sendMeterFrame(PORT_NO_3,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif             
             break;

          #else

           case 0x1:   //端口2
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x01;                                     //xMega端口1
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //端口转发速率
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("设置端口2转发速率\n");
              }
             #else
              oneUartConfig(fdOfttyS2,copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("端口2开始透明转发\n");
             }

						 //68 21 02 00 04 16 20 68 11 04 33 33 c3 33 9e 16
						 
             sendMeterFrame(PORT_NO_1,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             break;
       	 
           case 0x2:   //端口3
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x02;                                     //xMega端口2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //端口转发速率
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("设置端口3转发速率\n");
              }
             #else
              #ifdef SUPPORT_CASCADE
               oneUartConfig(fdOfttyS4, copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
              #else
               oneUartConfig(fdOfttyS3, copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
              #endif 
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("端口3开始透明转发\n");
             }

             sendMeterFrame(PORT_NO_2,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             break;
             
           case 0x3:   //端口4
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x03;                                     //xMega端口2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //端口转发速率
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("设置端口4转发速率\n");
              }
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("端口4开始透明转发\n");
             }

             sendMeterFrame(PORT_NO_3,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             break;
          #endif
          
          #ifdef PLUG_IN_CARRIER_MODULE
           case 0x4:   //端口31
           	#ifdef LM_SUPPORT_UT
           	 if (0x55==lmProtocol)
           	 {
               sendMeterFrame(PORT_POWER_CARRIER, copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             }
             else
             {
   	  	    #endif 
   	  	    
     	  	     //路由主导抄表的话,要查看是否在抄表且启动状态
     	  	     if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
     	  	     {
     	  	     	 memset(copyCtrl[4].needCopyMeter, 0x00, 6);
  
     	  	     	 if (carrierFlagSet.workStatus<4)
     	  	     	 {
     	  	     	   carrierFlagSet.workStatus = 4;
     	  	     	   carrierFlagSet.reStartPause = 2;
  
         	  	     if (debugInfo&PRINT_CARRIER_DEBUG)
         	  	     {
         	  	       printf("%02d-%02d-%02d %02d:%02d:%02d,路由主导抄读 - 抄表中,数据转发前申请暂停抄表\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
         	  	     }
  
     	  	     	 	 return FALSE;
     	  	     	 }
     	  	     	 
     	  	     	 if (carrierFlagSet.workStatus<5)
     	  	     	 {
     	  	     	 	 return FALSE;
     	  	     	 }
     	  	     }
  
             	 if (copyCtrl[portNum].pForwardData->data[8]<0x11)
             	 {
             	   copyCtrl[4].protocol = 1;
             	 }
             	 else
             	 {
             	   copyCtrl[4].protocol = 2;
             	 }
  
               ifFound = 0;
               memcpy(carrierFlagSet.destAddr, &copyCtrl[4].pForwardData->data[1], 6);
               memcpy(meterAddr, &copyCtrl[4].pForwardData->data[1], 6);
               
               //for(j=1;j<=512;j++) ly,2012-03-12,改成最多2040
               /*2015-10-19,去掉查找是否为配置参数的表地址
               for(j=1;j<=2040;j++)
               {
                 if (selectF10Data(j, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
                 {
              	   //未配置该测量点的信息
              	   continue;
                 }
                 else
                 {
                   if (meterConfig.addr[0]==meterAddr[0] && meterConfig.addr[1]==meterAddr[1]
                 	     && meterConfig.addr[2]==meterAddr[2] && meterConfig.addr[3]==meterAddr[3]
                 	      && meterConfig.addr[4]==meterAddr[4] && meterConfig.addr[5]==meterAddr[5]
                 	    )
                   {
                    #ifndef LIGHTING
                     //如果采集器地址为全0,则载波目的节点为表地址,否则目的节点为采集器地址
                     if (meterConfig.collectorAddr[0]==0x00 && meterConfig.collectorAddr[1]==0x00
                   	    && meterConfig.collectorAddr[2]==0x00 && meterConfig.collectorAddr[3]==0x00
                   	     && meterConfig.collectorAddr[4]==0x00 && meterConfig.collectorAddr[5]==0x00
                   	    )
                     {
                    #endif
                    
                        memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
                        copyCtrl[4].ifCollector = 0;
                    
                    #ifndef LIGHTING 
                     }
                     else
                     {
                        memcpy(carrierFlagSet.destAddr, meterConfig.collectorAddr, 6);
                        copyCtrl[4].ifCollector = 1;
                     }
                    #endif
                    
                     ifFound = 1;
                   }
                 }
               }
               */
               
               //2015-10-19,直接赋值载波目标地址
               memcpy(carrierFlagSet.destAddr, &copyCtrl[4].pForwardData->data[1], 6);
               copyCtrl[4].ifCollector = 0;
  
             	 if (debugInfo&METER_DEBUG)
             	 {
                 printf("端口31开始透明转发\n");
             	 }
               sendCarrierFrame(PORT_POWER_CARRIER,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
            
            #ifdef LM_SUPPORT_UT
             }
            #endif
            
           	 break;
          #endif
           	 
           default:   //其它端口
           	 return FALSE;
         }

        #ifdef PLUG_IN_CARRIER_MODULE
       	 //2013-12-25,鼎信至少要的监控命令至少要等待100秒,才可恢复
       	 if (carrierModuleType==TC_CARRIER)
       	 {
       	   if (copyCtrl[portNum].pForwardData->frameTimeOut<100)
       	   {
       	  	 copyCtrl[portNum].pForwardData->frameTimeOut = 100;
       	  	 
       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
       	  	 {
       	  	   printf("%02d-%02d-%02d %02d:%02d:%02d,鼎信载波模块修改转发超时时间为100秒\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
       	  	 }
       	   }

           if (1==carrierFlagSet.routeLeadCopy)
           {
             carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, copyCtrl[portNum].pForwardData->frameTimeOut+2);
           }
       	 }
       	 else
       	 {
           if (1==carrierFlagSet.routeLeadCopy)
           {
             carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
           }
       	 }
        #endif

         copyCtrl[portNum].pForwardData->outTime = nextTime(sysTime, 0, copyCtrl[portNum].pForwardData->frameTimeOut+2);

         printf("转发超时时间%d\n", copyCtrl[portNum].pForwardData->frameTimeOut);

         copyCtrl[portNum].pForwardData->ifSend  = TRUE;
         
         return TRUE;
         break;
         
       case 9:    //转发主站直接对电能表的抄读数据命令
     	   //帧起始字符
     	   sendBuf[0] = 0x68;
          
         //填写地址域
         memcpy(&sendBuf[1],copyCtrl[portNum].pForwardData->data,6);         
         
         //帧起始字符
         sendBuf[7] = 0x68;
         
         //填写抄表项
         frameTail = 9;
         if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
         {
            sendBuf[8] = 0x01;              //控制字段
            sendBuf[frameTail++] = 0x2;     //长度
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[7] + 0x33;  //DI0
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[8] + 0x33;  //DI1
         }
         else    //DLT_645_2007
         {
            sendBuf[8] = 0x11;              //控制字段
            sendBuf[frameTail++] = 0x4;     //长度
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[7]+0x33;  //DI0
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[8]+0x33;  //DI1            	
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[9]+0x33;  //DI2
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[10]+0x33;  //DI3
         }
         
         //Checksum
         sendBuf[frameTail] = 0x00;
         for (i = 0; i < frameTail; i++)
         {
           sendBuf[frameTail] += sendBuf[i];
         }
           
         frameTail++;
         sendBuf[frameTail++] = 0x16;

         copyCtrl[portNum].pForwardData->outTime       = nextTime(sysTime, 0, copyCtrl[portNum].pForwardData->frameTimeOut);
         copyCtrl[portNum].pForwardData->ifSend        = TRUE;
         copyCtrl[portNum].pForwardData->recvFrameTail = 20;
         
         switch(portNum)
         {
           #ifdef RS485_1_USE_PORT_1
            case 0x0:   //端口1
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega端口1
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口1转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega端口1
                 buf[1] = 0x6b;                                     //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口1转发速率为2400\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,0x6b);
                #endif
              }
             
              sendMeterFrame(PORT_NO_1,sendBuf, frameTail);
              
              #ifdef WDOG_USE_X_MEGA
  	           if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	           {
  	             xMegaQueue.inTimeFrameSend = TRUE;
  	           }
  	          #endif
  	                        
              if (debugInfo&METER_DEBUG)
              {
                printf("端口1开始转发主站直接抄表\n");
              }
              break;
             
            case 0x1:   //端口2
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega端口2
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口2转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega端口2
                 buf[1] = 0x6b;                                     //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口2转发速率为2400\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, 0x6b);
                #endif
              }             
              sendMeterFrame(PORT_NO_2,sendBuf, frameTail);
              
              #ifdef WDOG_USE_X_MEGA
  	           if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	           {
  	             xMegaQueue.inTimeFrameSend = TRUE;
  	           }
  	          #endif
  	          
              if (debugInfo&METER_DEBUG)
              {
                printf("端口2开始转发主站直接抄表\n");
              }
              break;
              
            case 0x2:   //端口3
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega端口3
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口3转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega端口2
                 buf[1] = 0x6b;                                     //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口3转发速率为2400\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, 0x6b);
                #endif
              }             
              sendMeterFrame(PORT_NO_3,sendBuf, frameTail);
              
              #ifdef WDOG_USE_X_MEGA
  	           if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	           {
  	             xMegaQueue.inTimeFrameSend = TRUE;
  	           }
  	          #endif
  	          
              if (debugInfo&METER_DEBUG)
              {
                printf("端口3开始转发主站直接抄表\n");
              }
              break;
              
           #else
           
            case 0x1:   //端口2
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega端口1
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口2转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega端口1
                 buf[1] = 0x6b;                                     //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口2转发速率为2400\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,0x6b);
                #endif
              }
             
              sendMeterFrame(PORT_NO_1,sendBuf, frameTail);
              
              #ifdef WDOG_USE_X_MEGA
  	           if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	           {
  	             xMegaQueue.inTimeFrameSend = TRUE;
  	           }
  	          #endif
  	          
              if (debugInfo&METER_DEBUG)
              {
                printf("端口2开始转发主站直接抄表\n");
              }
              break;
             
            case 0x2:   //端口3
             #ifndef SUPPORT_CASCADE 
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega端口2
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口3转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega端口2
                 buf[1] = 0x6b;                                     //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口3转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, 0x6b);
                #endif
              }
             #else
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                oneUartConfig(fdOfttyS4, DEFAULT_SER_CTRL_WORD);
              }
              else
              {
                oneUartConfig(fdOfttyS4, 0x6b);
              }
             #endif
             
              sendMeterFrame(PORT_NO_2,sendBuf, frameTail);
              
              #ifdef WDOG_USE_X_MEGA
  	           if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	           {
  	             xMegaQueue.inTimeFrameSend = TRUE;
  	           }
  	          #endif
              if (debugInfo&METER_DEBUG)
              {
                printf("端口3开始转发主站直接抄表\n");
              }
              break;
              
            case 0x3:   //端口4
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega端口3
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口3转发速率为1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega端口3
                 buf[1] = 0x6b;                                     //端口转发速率
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("设置端口3转发速率为2400\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,0x6b);
                #endif
              }
             
              sendMeterFrame(PORT_NO_3,sendBuf, frameTail);
              
              #ifdef WDOG_USE_X_MEGA
  	           if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	           {
  	             xMegaQueue.inTimeFrameSend = TRUE;
  	           }
  	          #endif
  	          
              if (debugInfo&METER_DEBUG)
              {
                printf("端口3开始转发主站直接抄表\n");
              }
              break;
              
           #endif
             
          #ifdef PLUG_IN_CARRIER_MODULE 
           case 0x4:   //端口31
   	  	     //路由主导抄表的话,要查看是否在抄表且启动状态
   	  	     if (carrierFlagSet.routeLeadCopy==1)
   	  	     {
   	  	     	 memset(copyCtrl[4].needCopyMeter, 0x00, 6);

   	  	     	 if (carrierFlagSet.workStatus<4)
   	  	     	 {
   	  	     	   carrierFlagSet.workStatus = 4;
   	  	     	   carrierFlagSet.reStartPause = 2;

   	  	     	 	 return FALSE;
   	  	     	 }
   	  	     	 
   	  	     	 if (carrierFlagSet.workStatus<5)
   	  	     	 {
   	  	     	 	 return FALSE;
   	  	     	 }
   	  	     }

           	 if ((copyCtrl[4].pForwardData->data[6]&0x3)==00)
           	 {
           	   copyCtrl[4].protocol = 1;
           	 }
           	 else
           	 {
           	   copyCtrl[4].protocol = 2;
           	 }

             ifFound = 0;
             memcpy(carrierFlagSet.destAddr, copyCtrl[4].pForwardData->data, 6);
             
             memcpy(meterAddr, copyCtrl[4].pForwardData->data, 6);

             for(j=1;j<=2040;j++)
             {
               if (selectF10Data(j, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
               {
            	   //未配置该测量点的信息
            	   continue;
               }
               else
               {
                 if (meterConfig.addr[0]==meterAddr[0] && meterConfig.addr[1]==meterAddr[1]
               	     && meterConfig.addr[2]==meterAddr[2] && meterConfig.addr[3]==meterAddr[3]
               	      && meterConfig.addr[4]==meterAddr[4] && meterConfig.addr[5]==meterAddr[5]
               	    )
                 {
                   //如果采集器地址为全0,则载波目的节点为表地址,否则目的节点为采集器地址
                   if (meterConfig.collectorAddr[0]==0x00 && meterConfig.collectorAddr[1]==0x00
                 	    && meterConfig.collectorAddr[2]==0x00 && meterConfig.collectorAddr[3]==0x00
                 	     && meterConfig.collectorAddr[4]==0x00 && meterConfig.collectorAddr[5]==0x00
                 	    )
                   {
                      memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
                      copyCtrl[4].ifCollector = 0;
                   }
                   else
                   {
                      memcpy(carrierFlagSet.destAddr, meterConfig.collectorAddr, 6);
                      copyCtrl[4].ifCollector = 1;
                   }
                   ifFound = 1;
                 }
               }
             }

             if (ifFound==0)
             {
               memcpy(carrierFlagSet.destAddr, copyCtrl[4].pForwardData->data, 6);
             }
             
             sendCarrierFrame(PORT_POWER_CARRIER, sendBuf, frameTail);
             
             if (debugInfo&METER_DEBUG)
             {
               printf("端口31开始转发抄表\n");
             }
           	 break;
          #endif
           	 
           default:   //其它端口
           	 return FALSE;
         }
         return TRUE;
       	 break;
       	 
       default:
         return FALSE;
     }
   }
   else     //转发命令已发送后判断是否超时
   {
   	 //判断超时
   	 if (compareTwoTime(copyCtrl[portNum].pForwardData->outTime, sysTime))
   	 {
   	 	 if (debugInfo&METER_DEBUG)
   	 	 {
   	     printf("%02d-%02d-%02d %02d:%02d:%02d,转发超时\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
   	 	 }

   	 	 copyCtrl[portNum].pForwardData->forwardResult = RESULT_NO_REPLY;
   	 	 forwardDataReply(portNum);  //应该回复无数
   	 	 	 
   	 	 return FALSE;
   	 }
   }
   
   return FALSE;
}

//***********************************Q/GDW376.1国家电网终端******end******************************




#ifdef SUPPORT_ETH_COPY

#define TOTAL_CMD_CURRENT_645_1997            79    //DL/T645-1997当前数据命令条数

#define TOTAL_CMD_CURRENT_645_2007           100    //DL/T645-2007当前数据命令条数

//DL/T645-1997电量+需量+参变量+时段参变量数据与数据标识转换表
INT8U  cmdDlt645Current1997[TOTAL_CMD_CURRENT_645_1997][5] = {
	                        //电量及需量 26项
	                        {0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x2f, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x1f, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x3f, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x4f, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x5f, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x6f, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA0, 0x1f, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA0, 0x2f, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA1, 0x1f, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA1, 0x2f, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x3f, QUA1_REQ_NO_WORK_OFFSET&0xff, QUA1_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x4f, QUA4_REQ_NO_WORK_OFFSET&0xff, QUA4_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x5f, QUA2_REQ_NO_WORK_OFFSET&0xff, QUA2_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x6f, QUA3_REQ_NO_WORK_OFFSET&0xff, QUA3_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB0, 0x1f, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB0, 0x2f, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB1, 0x1f, REQ_TIME_P_NO_WORK_OFFSET&0xff, REQ_TIME_P_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB1, 0x2f, REQ_TIME_N_NO_WORK_OFFSET&0xff, REQ_TIME_N_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x3f, QUA1_REQ_TIME_NO_WORK_OFFSET&0xff, QUA1_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x4f, QUA4_REQ_TIME_NO_WORK_OFFSET&0xff, QUA4_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x5f, QUA2_REQ_TIME_NO_WORK_OFFSET&0xff, QUA2_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x6f, QUA3_REQ_TIME_NO_WORK_OFFSET&0xff, QUA3_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x20, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        
	                        //参变量数据
	                        {0xB6, 0x11, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x12, VOLTAGE_PHASE_B&0xFF, VOLTAGE_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x13, VOLTAGE_PHASE_C&0xFF, VOLTAGE_PHASE_C>>8&0xFF,0x1},
	                        {0xB6, 0x21, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x22, CURRENT_PHASE_B&0xFF, CURRENT_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x23, CURRENT_PHASE_C&0xFF, CURRENT_PHASE_C>>8&0xFF,0x1},
	                        {0xB6, 0x30, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x31, POWER_PHASE_A_WORK&0xFF, POWER_PHASE_A_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x32, POWER_PHASE_B_WORK&0xFF, POWER_PHASE_B_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x33, POWER_PHASE_C_WORK&0xFF, POWER_PHASE_C_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x40, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x41, POWER_PHASE_A_NO_WORK&0xFF, POWER_PHASE_A_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x42, POWER_PHASE_B_NO_WORK&0xFF, POWER_PHASE_B_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x43, POWER_PHASE_C_NO_WORK&0xFF, POWER_PHASE_C_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x50, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF,0x1},
	                        {0xB6, 0x51, FACTOR_PHASE_A&0xFF, FACTOR_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x52, FACTOR_PHASE_B&0xFF, FACTOR_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x53, FACTOR_PHASE_C&0xFF, FACTOR_PHASE_C>>8&0xFF,0x1},
	                        {0xB3, 0x10, PHASE_DOWN_TIMES&0xFF, PHASE_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x11, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x12, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x13, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x20, TOTAL_PHASE_DOWN_TIME&0xFF, TOTAL_PHASE_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x21, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x22, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x23, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x30, LAST_PHASE_DOWN_BEGIN&0xFF, LAST_PHASE_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x31, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x32, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x33, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x40, LAST_PHASE_DOWN_END&0xFF, LAST_PHASE_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x41, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x42, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x43, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF,0x1},
	                        {0xB2, 0x10, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x11, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x12, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF,0x1},
	                        {0xB2, 0x13, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x14, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF,0x1},
	                        {0xC0, 0x10, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF,0x1},
	                        {0xC0, 0x11, METER_TIME&0xFF, METER_TIME>>8&0xFF,0x1},
	                        {0xC0, 0x20, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF,0x1},
	                        {0xC0, 0x21, NET_STATUS_WORD&0xFF, NET_STATUS_WORD>>8&0xFF,0x1},
	                        {0xC0, 0x30, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF,0x1},
	                        {0xC0, 0x31, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF,0x1},
	                        {0xC0, 0x32, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF,0x1},
	                        {0xC1, 0x17, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF,0x1},
	                        //以下几行时段参变量
	                        {0xC3, 0x10, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF,0x1},
	                        {0xC3, 0x11, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF,0x1},
	                        {0xC3, 0x12, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF,0x1},
	                        {0xC3, 0x13, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF,0x1},
	                        {0xC3, 0x14, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF,0x1},
	                        {0xC3, 0x2f, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF,0x1},
	                        
	                        //以下是日时段表、起始时间及费率号
	                        {0xC3, 0x3f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x4f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x5f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x6f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x7f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x8f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x9f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0xaf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1}
	                        
	                        //以下是假日时段
	                        //{0xC3, 0xbf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xcf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xdf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xef, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC4, 0x11, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x12, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x13, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x14, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x15, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x16, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x17, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x18, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x19, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1a, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1b, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1c, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1d, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1e, ZHOUXIURI_SHIDUAN&0xFF, ZHOUXIURI_SHIDUAN>>8&0xFF,0x1}
                          };

INT8U  cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
   //2.2.1 电量 22项(全部4bytes,与97一致)
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能示值总,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能示值总,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //组合无功1电能
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //组合无功2电能
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1象限无功电能
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4象限无功电能
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2象限无功电能
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3象限无功电能
   {0x00, 0x85, 0x00, 0x00, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //铜损有功总电能示值(补偿量,4bytes)
   {0x00, 0x86, 0x00, 0x00, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //铁损有功总电能示值(补偿量,4bytes)
   {0x00, 0x15, 0x00, 0x00, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A相正向有功电能(4Bytes)
   {0x00, 0x16, 0x00, 0x00, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A相反向有功电能(4Bytes)
   {0x00, 0x17, 0x00, 0x00, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功1电能(4Bytes)
   {0x00, 0x18, 0x00, 0x00, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功2电能(4Bytes)
   {0x00, 0x29, 0x00, 0x00, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B相正向有功电能(4Bytes)
   {0x00, 0x2A, 0x00, 0x00, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B相反向有功电能(4Bytes)
   {0x00, 0x2B, 0x00, 0x00, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功1电能(4Bytes)
   {0x00, 0x2C, 0x00, 0x00, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功2电能(4Bytes)
   {0x00, 0x3D, 0x00, 0x00, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C相正向有功电能(4Bytes)
   {0x00, 0x3E, 0x00, 0x00, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C相反向有功电能(4Bytes)
   {0x00, 0x3F, 0x00, 0x00, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功1电能(4Bytes)
   {0x00, 0x40, 0x00, 0x00, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功2电能(4Bytes)
   
   //2.2.2 需量及需量发生时间 4项(需量3bytes+5bytes年月日时分,97是3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //正向有功最大需量及发生时间
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //反向有功最大需量及发生时间
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//组合无功1最大需量及发生时间
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //组合无功2最大需量及发生时间
   
   //2.2.3 变量数据 8项
   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //电压数据块(A,B,C相电压)(各2字节=97,但格式97是xxx而07是xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //电流数据块(A,B,C相电流)(各3字节<>97,97是各2字节xx.xx,07是xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //瞬时有功功率块(总,A,B,C相有功功率)(各3字节=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //瞬时无功功率块(总,A,B,C相无功功率)(各3字节xx.xxxx<>97是xx.xx)
   {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},          //瞬时视在功率块(总,A,B,C相视在功率)(各3字节xx.xxxx<>97没有)
   {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                  //功率因数块(总,A,B,C相功率因数)(各2字节=97,x.xxx)   
   {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},                        //相角数据块(A,B,C相相角)(各2字节,97无,x.xxx)
   {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},                //零线电流(3字节xxx.xxx<>97是2字节xx.xx)
   {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},                    //电池工作时间(4字节NNNNNNNN<>97是3字节NNNNNN)

   //2.2.4.事件记录数据 26项
   //2.2.4-1.断相统计数据
   //2.2.4-1.1有些07表支持规约,用以下4项抄读
   //总断相次数,总累计时间未找到,难道要计算(10-07-05,经与东软集中器对比,确实需要计算)
   {0x03, 0x04, 0x00, 0x00, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)
                                                                                                   //B相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)
                                                                                                   //C相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)

   //最近一次断相发生起始时刻及结束时刻未找到,难道要计算?(10-07-05,经与东软集中器对比,确实需要计算)
   {0x03, 0x04, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //上一次A相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)
   {0x03, 0x04, 0x02, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //上一次B相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)
   {0x03, 0x04, 0x03, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //上一次C相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)

   //2.2.4-1.2 有些表支持备案文件,所以用以下12条抄读断相统计数据
   //总断相次数,总累计时间未找到,难道要计算(10-07-05,经与东软集中器对比,确实需要计算)
   {0x13, 0x01, 0x00, 0x01, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x02, 0x00, 0x01, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF},                  //B相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x03, 0x00, 0x01, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF},                  //B相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x01, 0x00, 0x02, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF},        //A相累计时间(累计3字节=97)
   {0x13, 0x02, 0x00, 0x02, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF},        //B相累计时间(累计3字节=97)
   {0x13, 0x03, 0x00, 0x02, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF},        //C相累计时间(累计3字节=97)

   //2.2.4-1.3 最近一次断相发生起始时刻及结束时刻未找到,难道要计算?(10-07-05,经与东软集中器对比,确实需要计算)
   
   {0x13, 0x01, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //上一次A相断相记录(起始时刻)(时刻6字节<>97,4字节)
   {0x13, 0x02, 0x01, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //上一次B相断相记录(起始时刻)(时刻6字节<>97,4字节)
   {0x13, 0x03, 0x01, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //上一次C相断相记录(起始时刻)(时刻6字节<>97,4字节)
   {0x13, 0x01, 0x25, 0x01, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF},            //上一次A相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   {0x13, 0x02, 0x25, 0x01, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF},            //上一次B相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   {0x13, 0x03, 0x25, 0x01, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF},            //上一次C相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   
   //2.2.4-2 其它统计量
   {0x03, 0x30, 0x00, 0x00, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF},                            //编程总次数(3字节!=97,2字节)
   {0x03, 0x30, 0x00, 0x01, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF},                    //最近一次编程时间(时间6字节<>97,4字节)
   {0x03, 0x30, 0x01, 0x00, METER_CLEAR_TIMES&0xFF, METER_CLEAR_TIMES>>8&0xFF},                    //电表清零总次数(3字节!=97没有)
   {0x03, 0x30, 0x01, 0x01, LAST_METER_CLEAR_TIME&0xFF, LAST_METER_CLEAR_TIME>>8&0xFF},            //最近一次电表清零时间(时间6字节<>97没有)
   {0x03, 0x30, 0x02, 0x00, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF},                        //最大需量清零总次数(3字节!=97,2字节)
   {0x03, 0x30, 0x02, 0x01, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF},              //最近一次最大需量清零时间(时间6字节<>97,4字节)
   {0x03, 0x30, 0x03, 0x00, EVENT_CLEAR_TIMES&0xFF, EVENT_CLEAR_TIMES>>8&0xFF},                    //事件清零总次数(3字节!=97,没有)
   {0x03, 0x30, 0x03, 0x01, EVENT_CLEAR_LAST_TIME&0xFF, EVENT_CLEAR_LAST_TIME>>8&0xFF},            //事件清零时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x04, 0x00, TIMING_TIMES&0xFF, TIMING_TIMES>>8&0xFF},                              //校时总次数(3字节!=97,没有)
   {0x03, 0x30, 0x04, 0x01, TIMING_LAST_TIME&0xFF, TIMING_LAST_TIME>>8&0xFF},                      //最近一次校时时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x05, 0x00, PERIOD_TIMES&0xFF, PERIOD_TIMES>>8&0xFF},                              //时段表编程总次数(3字节!=97,没有)
   {0x03, 0x30, 0x05, 0x01, PERIOD_LAST_TIME&0xFF, PERIOD_LAST_TIME>>8&0xFF},                      //最近一次时段表编程发生时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x0d, 0x00, OPEN_METER_COVER_TIMES&0xFF, OPEN_METER_COVER_TIMES>>8&0xFF},          //开表盖总次数(2字节,97没有)
   {0x03, 0x30, 0x0d, 0x01, LAST_OPEN_METER_COVER_TIME&0xFF, LAST_OPEN_METER_COVER_TIME>>8&0xFF},  //上一次开表盖发生时刻(6字节,97没有)
   
   //2.2.5 参变量 9项
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                            //日期及周次(4字节=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},                                  //电表时间(3字节=97)
   {0x04, 0x00, 0x04, 0x09, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF},                            //电表常数(有功)(3字节=97)
   {0x04, 0x00, 0x04, 0x0a, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF},                      //电表常数(无功)(3字节=97)
   {0x04, 0x00, 0x04, 0x02, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF},                              //电表号(6字节=97)
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},                    //电表运行状态字1到7(7*2<>97只有2bytes)
   {0x04, 0x00, 0x0b, 0x01, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF},                            //每月第1结算日
   {0x04, 0x00, 0x0b, 0x02, AUTO_COPY_DAY_2&0xFF, AUTO_COPY_DAY_2>>8&0xFF},                        //每月第2结算日
   {0x04, 0x00, 0x0b, 0x03, AUTO_COPY_DAY_3&0xFF, AUTO_COPY_DAY_3>>8&0xFF},                        //每月第3结算日
   
   //2.2.6 以下几行时段参变量 6项
   {0x04, 0x00, 0x02, 0x01, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF},                              //年时区数(1字节)
   {0x04, 0x00, 0x02, 0x02, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF},                  //日时段表数(1字节)
   {0x04, 0x00, 0x02, 0x03, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF},                            //日时段数(1字节)
   {0x04, 0x00, 0x02, 0x04, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF},                              //费率数(1字节)
   {0x04, 0x00, 0x02, 0x05, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF},                        //公共假日数(1字节)   
   {0x04, 0x01, 0x00, 0x00, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF},                                  //第一套时区表数据(14*3)
   
   //2.2.7 以下是第一套日时段表、起始时间及费率号 8项
   {0x04, 0x01, 0x00, 0x01, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第1日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x02, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第2日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x03, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第3日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x04, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第4日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x05, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第5日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x06, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第6日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x07, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第7日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x08, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第8日时段表数据(当前存前10个时段起始时间和费率号)
   
   //2.2.8 费控表控制/开关/购电信息
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME&0xFF, LAST_JUMPED_GATE_TIME>>8&0xFF},            //上一次跳闸发生时刻(6字节,97没有)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME&0xFF, LAST_CLOSED_GATE_TIME>>8&0xFF},            //上一次合闸发生时刻(6字节,97没有)
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME&0xFF, CHARGE_TOTAL_TIME>>8&0xFF},                    //上1次购电后总购电次数(2字节)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY&0xFF, CHARGE_REMAIN_MONEY>>8&0xFF},                //当前剩余金额(4字节,97没有)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY&0xFF, CHARGE_TOTAL_MONEY>>8&0xFF},                  //上一次购电后累计购电金额(4字节,97没有)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY&0xFF, CHARGE_REMAIN_QUANTITY>>8&0xFF},          //当前剩余电量(4字节,97没有)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY&0xFF, CHARGE_OVERDRAFT_QUANTITY>>8&0xFF},    //当前透支电量(4字节,97没有)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY&0xFF, CHARGE_TOTAL_QUANTITY>>8&0xFF},            //上一次购电后累计购电量(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT&0xFF, CHARGE_OVERDRAFT_LIMIT>>8&0xFF},          //透支电量限值(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY&0xFF, CHARGE_ALARM_QUANTITY>>8&0xFF},            //报警电量1限值
   };


int               fdOfClient=NULL;    //与CAN2ETH通信监听到socket

struct cpAddrLink *pEthCpLinkHead;    //以太网抄表参数链表指针

INT8U             canloginIn=0;       //CAN2ETH是否登录成功
DATE_TIME         canTimeOut;         //CAN2ETH心跳超时
INT8U             flagOfFrame=0;      //接收帧标志

INT8U             setEthMeter=0;      //主站设置以太网抄表地址
DATE_TIME         msSetWaitTime;      //等待设置表地址完
INT16U            monitorMp=0;        //需要监视原始数据的测试点号(由于以太网抄表表太多,如果需要一个一个看时用)

/**************************************************
函数名称:sendToCan2Eth
功能描述:发送数据到CAN2ETH
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：void
***************************************************/
BOOL sendToCan2Eth(INT8U afn, INT8U *buf, INT16U len)
{
  INT16U i;
  INT8U  checkSum;
  
  if (NULL==fdOfClient)
  {
  	return FALSE;
  }
  
  // 7E len afn data cs 7E
  // afn - 0 确认/否认
  //     - 1 登录/心跳
  //     - 2 抄表数据
  // cs  - 指len,afn,data的算术和
  
  //净数据移位  
  for(i=0; i<len; i++)
  {
  	buf[len+2-i] = buf[len-1-i];
  }  
  
  buf[0] = 0x7e;
  buf[1] = len;
  buf[2] = afn;
  
  len+=3;
  
  //checkSum
  checkSum = 0;
  for(i=1; i<len; i++)
  {
  	checkSum += buf[i];
  }
  buf[len++] = checkSum;
  buf[len++] = 0x7e;
  
  if (write(fdOfClient, buf, len) == -1)
  {
  	perror("sendDataToClient");
  	
  	return FALSE;
  }

  if (debugInfox&ETH_COPY_METER)
  {
    printf("%02d-%02d-%02d %02d:%02d:%02d,eth copy Tx(%d,[S%d]),", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, len, fdOfClient);
    for(i=0; i<len; i++)
    {
      printf("%02x ", buf[i]);
    }
     
    printf("\n");
  }
  
  return TRUE;
}

void setNonBlocking(int sock)
{
  int opts;
  
  opts = fcntl(sock, F_GETFL);

  if(opts<0)
  {
    perror("fcntl(sock,GETFL)");

    _exit(1);
  }

  opts = opts|O_NONBLOCK;

  if(fcntl(sock, F_SETFL, opts)<0)
  {
    perror("fcntl(sock,SETFL,opts)");
    _exit(1);
  }
}

/***************************************************
函数名称:dataOffset
功能描述:根据电表数据中DI的确定参数数据类别
调用函数:
被调用函数:
输入参数:di
输出参数:
返回值：数据类别
***************************************************/
INT16U dataOffset(INT8U protocol, INT8U *di)
{
  INT16U    ret=0x200;
  INT8U     i;
  char      tmpChrDi[100];
    
  switch (protocol)
  {         	
    case DLT_645_1997:
      if ((*(di+1)==0x94) || *(di+1)==0x95 || *(di+1)==0xa4 || *(di+1)==0xa5 || *(di+1)==0xb4 || *(di+1)==0xb5)  //上月数据
     	{
//    	  //电能量、需量及需量时间
//    	  for (i = 0; i < TOTAL_CMD_LASTMONTH_645_1997; i++)
//    	  {
//    	    if (cmdDlt645LastMonth1997[i][0] == *(di+1) && (cmdDlt645LastMonth1997[i][1] == *di))
//    	    {
//    	      ret = (cmdDlt645LastMonth1997[i][2]) | ((cmdDlt645LastMonth1997[i][3])<<8);
//    	      break;
//    	    }
//    	  }
    	}
    	else      		//当前数据
    	{
        for (i = 0; i < TOTAL_CMD_CURRENT_645_1997; i++)
      	{
      	  if (cmdDlt645Current1997[i][0] == *(di+1) && (cmdDlt645Current1997[i][1] == *di))
          {
            ret = (cmdDlt645Current1997[i][2]) | ((cmdDlt645Current1997[i][3])<<8);
            break;
          }
    	  }
    	}
    	break;
    	
    case DLT_645_2007:  //DL/T645-2007
      /*
      if (((*(di+3)==0x00) || (*(di+3)==0x01)) && ((*(di+1)==0xff) || (*(di+1)==0x00)) && (*di==0x1))
      {
        for (i = 0; i < TOTAL_CMD_LASTMONTH_645_2007; i++)
    	  {
    	    if (cmdDlt645LastMonth2007[i][0] == *(di+3)
    	  	   && (cmdDlt645LastMonth2007[i][1]==*(di+2))
    	  	    && (cmdDlt645LastMonth2007[i][2]==*(di+1))
    	  	     && (cmdDlt645LastMonth2007[i][3]==*di)
    	  	   )
          {
            ret = (cmdDlt645LastMonth2007[i][4]) | ((cmdDlt645LastMonth2007[i][5])<<8);
            break;
          }
        }
      }
      else
      {
        if ((*(di+3)==0x05) && (*(di+2)==0x06) && (*di==0x1))
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
      	  {
      	    if (cmdDlt645LastDay2007[i][0] == *(di+3)
      	  	   && (cmdDlt645LastDay2007[i][1]==*(di+2))
      	  	    && (cmdDlt645LastDay2007[i][2]==*(di+1))
      	  	     && (cmdDlt645LastDay2007[i][3]==*di)
      	  	   )
            {
              ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
              break;
            }
          }
        }
        else
        {
      */
          for (i = 0; i < TOTAL_CMD_CURRENT_645_2007; i++)
      	  {
      	    if (cmdDlt645Current2007[i][0] == *(di+3)
      	  	   && (cmdDlt645Current2007[i][1]==*(di+2))
      	  	    && (cmdDlt645Current2007[i][2]==*(di+1))
      	  	     && (cmdDlt645Current2007[i][3]==*di)
      	  	   )
            {
              ret = (cmdDlt645Current2007[i][4]) | ((cmdDlt645Current2007[i][5])<<8);
              break;
            }
    	    }
    	//  }
  	  //}
  	  break;
      
    default:
      ret = 0x200;   //待完成
      break;
  }
  
  return ret;
}

/***************************************************
函数名称:analyse645Dlt1997
功能描述:97规约数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S analyse645Dlt1997(struct cpAddrLink *pCpLink)
{
  INT16U offset;
  INT8U  *pData;
  INT16U loadLen;

  //数据存储偏移量
  offset = findDataOffset(pCpLink->protocol, &pCpLink->recvBuf[10]);
      
  //68 33 47 00 11 12 92 68 81 04 46 e9 33 33 19 16
  if (debugInfox&ETH_COPY_METER)
  {
    printf(",1997规约,标识%02X %02X,偏移:%02X\n",
          pCpLink->recvBuf[11], pCpLink->recvBuf[10], offset);
  }
  
  if (0x200==offset)
  {
    return -1;    //接收数据保存时偏移地址错误,未保存
  }

  pData = &pCpLink->recvBuf[10];      //净数据起始指针
  loadLen = pCpLink->recvBuf[9]-2;    //净数据长度

  switch(pCpLink->recvBuf[11])
  {
 	  case 0xB2:     //参数
 	  case 0xB3:
 	  case 0xB6:
 	  case 0xC0:
 	  case 0xC1:
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //有参数数据
      
      switch(loadLen)
      {
      	case  1:
        	*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
          break;
  
        case 2:
         	switch(offset)
         	{
         	 	case VOLTAGE_PHASE_A:  //A,B,C相电压各2字节xxx,存储为xxx.0
         	 	case VOLTAGE_PHASE_B:
         	 	case VOLTAGE_PHASE_C:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = (*(pData+2))<<4;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = ((*(pData+3))<<4) | ((*(pData+2))>>4);
        	    break;
        	     
       	    case CURRENT_PHASE_A:  //A,B,C相电流各2字节xx.xx存储为0xx.xx0
       	    case CURRENT_PHASE_B:
       	    case CURRENT_PHASE_C:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = (*(pData+2))<<4;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = ((*(pData+3))<<4) | ((*(pData+2))>>4);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = (*(pData+3))>>4;
       	   	  break;
       	   	 
       	    case POWER_INSTANT_NO_WORK: //无功功率为2bytesxx.xx,存储为xx.xx00
       	    case POWER_PHASE_A_NO_WORK:
       	    case POWER_PHASE_B_NO_WORK:
       	    case POWER_PHASE_C_NO_WORK:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = 0;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+3);      	   	
       	   	  break;
 
       	    case PHASE_DOWN_TIMES:      //断相次数为2bytesNNNN,存储为00NNNN
       	    case PHASE_A_DOWN_TIMES:
       	    case PHASE_B_DOWN_TIMES:
       	    case PHASE_C_DOWN_TIMES:
       	    case PROGRAM_TIMES:         //编程次数
       	    case UPDATA_REQ_TIME:       //需量清零次数
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+3);      	   	
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = 0;
       	   	  break;
       	   	 
       	    default:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+3);
              break;
          }
          break;
  
        case 3:
          switch(offset)
          {
           	case BATTERY_WORK_TIME:  //电池工作时间3bytes(NNNNNN),存储为4bytes(00NNNNNN)
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+3);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+4);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+3) = 0x0;
           	  break;
           	   
           	default:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+3);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+4);
              break;
          }
          break;
  
        case 4:
         	switch(offset)
         	{
         	 	case LAST_PHASE_DOWN_BEGIN:
         	 	case LAST_PHASE_DOWN_END:
         	 	case LAST_PHASE_A_DOWN_BEGIN:
         	 	case LAST_PHASE_A_DOWN_END:
         	 	case LAST_PHASE_B_DOWN_BEGIN:
         	 	case LAST_PHASE_B_DOWN_END:
         	 	case LAST_PHASE_C_DOWN_BEGIN:
         	 	case LAST_PHASE_C_DOWN_END:
         	 	case LAST_PROGRAM_TIME:     //最近一次编程时间
         	 	case LAST_UPDATA_REQ_TIME:  //最近一次需量清零时间
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = 0x0;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+3);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+3) = *(pData+4);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+4) = *(pData+5);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+5) = 0x0;
         	 	 	break;
         	 	  	
         	 	default:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+3);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+4);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+3) = *(pData+5);
              break;
          }
          break;
           
        case 6:
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+3);
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+4);
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+3) = *(pData+5);
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+4) = *(pData+6);
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+5) = *(pData+7);
          break;
      }
      break;
 	  
 	  case 0xC3:    //时段
 	  case 0xC4:
      pCpLink->copyedData |= HAS_SHIDUAN;     //有时段数据

      if (*(pData+1)==0xc3)
      {
        if ((*pData>>4)==1)  //年时区数P等
      	{
      		*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset) = *(pData+2);
      	}
      	else                     //时区及日时段
      	{
          if ((*pData & 0xF) != 0xF)  //数据项处理
          {
            offset += 3*((*pData&0xf)-1);
             	
            *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset) = *(pData + 2);
            *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+1) = *(pData + 3);
            *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+2) = *(pData + 4);
          }
          else   //数据块处理 数据标识xxxF
          {
            if ((*pData>>4&0xf)>2)
            {
             	offset += 30*((*pData>>4&0xf)-3);
            }
             
            pData += 2;     //指向数据项
            while (loadLen>=3)
            {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset)   = *pData;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+1) = *(pData + 1);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+2) = *(pData + 2);
                
              offset += 3;              	  
              pData += 3;
                
              if (*pData==0xaa && *(pData+2)==0x16)
              {
                break;
              }
              if (loadLen<3)
              {
                break;
              }
              else
              {
                loadLen -= 3;
              }
            }
          }
      	}
      }
      else
      {
      	if (*(pData+1)==0xc4)
      	{
      		 if ((*pData&0xf)==0xe)
      		 {
      		  	//shiDuanData[ZHOUXIURI_SHIDUAN] = *(pData+2);
      		  	
      		  	*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+ZHOUXIURI_SHIDUAN) = *(pData+2);
      		 }
      		 else
      		 {
      		  	offset = JIA_RI_SHIDUAN+3*((*pData&0xf)-1);
      		  	
             	//shiDuanData[offset++] = *(pData + 2);
             	//shiDuanData[offset++] = *(pData + 3);
             	//shiDuanData[offset] = *(pData + 4);    		  	 
  
             	*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset)   = *(pData + 2);
             	*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+1) = *(pData + 3);
             	*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+2) = *(pData + 4);    		  	 
      		 }
      	}
      }
 	  	break;
                	  	  	
    default:    //需量、电量
      //电能量、需量、需量时间数据
      if ((*pData & 0xF) != 0xF)        //数据项处理
      {
        if ((*(pData+1)&0xf0) == 0x90)  //电能量
        {
          if ((*(pData+1) & 0xf) == 0x0a)
          {
           	pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
          }
          else
          {
            if ((*(pData+1) & 0xc)>>2 == 0x00)
            {
           	   pCpLink->copyedData |= HAS_CURRENT_ENERGY;    //有当前电量数据
            }
            else
            {
           	   pCpLink->copyedData |= HAS_LAST_MONTH_ENERGY; //有上月电量数据
           	}
          }
           
        	*(pCpLink->dataBuff+offset)   = *(pData + 2);
        	*(pCpLink->dataBuff+offset+1) = *(pData + 3);
        	*(pCpLink->dataBuff+offset+2) = *(pData + 4);
        	*(pCpLink->dataBuff+offset+3) = *(pData + 5);
        }
        else                      //需量
        {
          if ((*(pData+1) & 0xc)>>2 == 0x00)
          {
           	 pCpLink->copyedData |= HAS_CURRENT_REQ;        //有当前需量数据
          }
          else
          {
           	 pCpLink->copyedData |= HAS_LAST_MONTH_REQ;     //有上月需量数据
          }
           
          if ((*(pData+1)&0xf0) == 0xA0)  //需量
          {
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData + 2);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 3);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 4);
          }
          else    //需量时间
          {
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData + 2);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 3);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 4);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+3) = *(pData + 5);
          }
        }
      }
      else    //数据块处理 数据标识xxxF
      {
        //68 30 00 01 00 00 00 68 81 17 1F 94 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AA 8B 16  
        if ((*(pData+1)&0xf0) == 0x90)           //电能量
        {
          if ((*(pData+1) & 0xf) == 0x0a)
          {
            pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
          }
          else
          {
            if ((*(pData+1) & 0xc)>>2 == 0x00)
            {
           	  pCpLink->copyedData |= HAS_CURRENT_ENERGY;    //有当前电量数据
            }
            else
            {
            	pCpLink->copyedData |= HAS_LAST_MONTH_ENERGY; //有上月电量数据
            }
          }
           
          pData += 2;     //指向数据项
          while (loadLen>=4)
          {
        	  *(pCpLink->dataBuff+offset)   = *(pData);
        	  *(pCpLink->dataBuff+offset+1) = *(pData + 1);
        	  *(pCpLink->dataBuff+offset+2) = *(pData + 2);
        	  *(pCpLink->dataBuff+offset+3) = *(pData + 3);
           	  
            offset += 4;
            pData += 4;
              
            if (loadLen<4)
            {
              break;
            }
            else
            {
              loadLen -= 4;
            }
          }
        }
        else                           //需量及发生时间
        {
          if ((*(pData+1) & 0xc)>>2 == 0x00)
          {
           	pCpLink->copyedData |= HAS_CURRENT_REQ;        //有当前需量数据
          }
          else
          {
            pCpLink->copyedData |= HAS_LAST_MONTH_REQ;     //有上月电量数据
          }
           
          if ((*(pData+1)&0xf0) == 0xA0)  //需量
          {
            pData += 2;     //指向数据项
            while (loadLen>=3)
            {
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData);
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 1);
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 2);
        	      
        	    offset += 3;
              pData += 3;
                
              if (loadLen<3)
              {
                break;
              }
              else
              {
                loadLen -= 3;
              }
            }
          }
          else                  //需量发生时间
          {
            pData += 2;     //指向数据项
            while (loadLen>=4)
            {
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData);
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 1);
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 2);
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+3) = *(pData + 3);
        	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+4) = 0x0;
           	    
           	  offset += 5;
              pData += 4;
                
              if (loadLen<4)
              {
                break;
              }
              else
              {
                loadLen -= 4;
              }
            }
          }
        }
      }
      break;
  }
}

/***************************************************
函数名称:analyse645Dlt2007
功能描述:07规约数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S analyse645Dlt2007(struct cpAddrLink *pCpLink)
{
  INT16U offset, offsetx;
  INT8U  i;
  INT8U  *pData;
  INT16U loadLen;
  
  //数据存储偏移量
  offset = dataOffset(pCpLink->protocol, &pCpLink->recvBuf[10]);
  
  if (debugInfox&ETH_COPY_METER)
  {
    printf(",2007规约,标识%02X %02X %02X %02X,偏移:%02X\n",
          pCpLink->recvBuf[13], pCpLink->recvBuf[12],pCpLink->recvBuf[11], pCpLink->recvBuf[10], offset);
  }
    
  if (offset==0x200)
  {
    return -1;    //接收数据保存时偏移地址错误,未保存
  }
  
  pData = &pCpLink->recvBuf[14];   //净数据起始指针
  loadLen = pCpLink->recvBuf[9]-4;//净数据长度
  switch(pCpLink->recvBuf[13])    //DI3
  {
 	  case ENERGY_2007:   //电能量
 	 	  if (pCpLink->recvBuf[10]==0x0)    //DI0
 	 	  {
      	pCpLink->copyedData |= HAS_CURRENT_ENERGY;     //有当前电量数据
      }
      else
      {
      	pCpLink->copyedData |= HAS_LAST_MONTH_ENERGY;  //有上一结算日电量数据
      }
      
 	 	  if (pCpLink->recvBuf[11]==0xff) //数据块
 	 	  {
        while (loadLen>=4)
        {
      	  *(pCpLink->dataBuff+offset)   = *(pData + 0);
      	  *(pCpLink->dataBuff+offset+1) = *(pData + 1);
      	  *(pCpLink->dataBuff+offset+2) = *(pData + 2);
      	  *(pCpLink->dataBuff+offset+3) = *(pData + 3);

          offset += 4;
          pData += 4;
            
          if (loadLen<4)
          {
            break;
          }
          else
          {
            loadLen -= 4;
          }
        }
 	 	  }
 	 	  else             //数据项
 	 	  {
      	 if (pCpLink->recvBuf[12]==0x90)    //这项是参变量
      	 {
      	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData);
      	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData + 1);
      	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData + 2);
      	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+3) = *(pData + 3);         	 	 
      	 }
      	 else
      	 {
      	   *(pCpLink->dataBuff+offset)   = *(pData);
      	   *(pCpLink->dataBuff+offset+1) = *(pData + 1);
      	   *(pCpLink->dataBuff+offset+2) = *(pData + 2);
      	   *(pCpLink->dataBuff+offset+3) = *(pData + 3);
      	 }
 	 	  }
 	 	  break;
 	 	 
 	  case REQ_AND_REQ_TIME_2007:  //需量及发生时间
 	 	  if (pCpLink->recvBuf[10]==0x0)
 	 	  {
      	pCpLink->copyedData |= HAS_CURRENT_REQ;        //有当前需量数据
      }
      else
      {
      	pCpLink->copyedData |= HAS_LAST_MONTH_REQ;     //有上月需量数据
      }

 	 	  if (pCpLink->recvBuf[11]==0xff) //数据块
 	 	  {
        offsetx = offset+27;
        while (loadLen>=8)
        {
      	  //需量3bytes
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData);
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 1);
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 2);
      	    
      	  //需量发生时间5bytes
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+0) = *(pData + 3);  //分
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+1) = *(pData + 4);  //时
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+2) = *(pData + 5);  //日
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+3) = *(pData + 6);  //月
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+4) = *(pData + 7);  //年

          offset  += 3;
          offsetx += 5;
          pData += 8;
            
          if (loadLen<8)
          {
            break;
          }
          else
          {
            loadLen -= 8;
          }
        }
 	 	  }
 	 	  else             //数据项
 	 	  {
 	 	  }    	 	 
 	 	  break;
 	 
 	  case VARIABLE_2007:   //变量
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //有参变量数据
 	 	 
 	 	  if (pCpLink->recvBuf[11]==0xff)
 	 	  {
        switch(pCpLink->recvBuf[12])
        {
        	case 0x01:  //电压数据块(各2个字节xxx.x)
        	  for(i=0;i<6 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x02:  //电流数据块(各3个字节xxx.xxx)
        	  for(i=0;i<9 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x03:  //瞬时有功功率数据块(各3个字节xx.xxxx)
        	case 0x04:  //瞬时无功功率数据块(各3个字节xx.xxxx)
        	case 0x05:  //瞬时视在功率数据块(各3个字节xx.xxxx)
        	  for(i=0;i<12 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x06:  //功率因数数据块(各2字节x.xxx)
        	  for(i=0; i<8 && i<loadLen; i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x07:  //相角数据块(各2字节x.xxx)
        	  for(i=0;i<6 && i<loadLen; i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);   //电压相角
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+6+i)   = *(pData+i);   //电流相角
        	  }
        	  break;
        	  	
        	default:
 	 	        return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据           	  	
        	  break;
        }
 	 	  }
 	 	  else
 	 	  {
        if (pCpLink->recvBuf[12]==0x80)
        {
          if (pCpLink->recvBuf[10]==0x01)  //零线电流(xxx.xxx!=97 2bytes xx.xx)
          {
        	  for(i=0;i<3 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
          }
           
        	if (pCpLink->recvBuf[10]==0x0a)  //内部电池工作时间(4bytes)
        	{
        	  for(i=0;i<4;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	}
        }
 	 	  }
 	 	  break;
 	 	 
 	  case EVENT_RECORD_2007:
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //有参变量数据
      
 	 	  switch(pCpLink->recvBuf[12])
 	 	  {
 	 	 	  case 0x04:    //A,B,C断相记录,断相次数和断相时间,etc...
     	 	  switch(pCpLink->recvBuf[10])
     	 	  {
     	 	 	  case 0x00:  //A,B,C断相次数及总累计时间(次数和时间各3bytes,NNNNNN,3*2*3)
     	 	 	    for(i=0;i<18;i++)
     	 	 	    {
                *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
     	 	 	    }
     	 	 	    break;
     	 	 	    
     	 	 	  case 0x01:  //A,B,C相断相记录(起始时间和结束时间各6bytes,ssmmhhDDMMYY)
            	for(i=0;i<12;i++)
            	{
                *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	}
            	break;
            	  
            default:
 	 	          return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据
     	 	  }
     	 	  break;
     	 	 
     	  case 0x30:   //编程,电表清零,需量清零,事件清零,校时,时段表修改记录
     	 	  switch(pCpLink->recvBuf[11])
     	 	  {
     	 	 	  case 0x0:  //编程记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //编程次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次编程记录
            	    //仅保存上一次编程发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x1:  //电表清零记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //电表清零总次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次电表清零记录
            	    //仅保存上一次电表清零发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x2:  //需量清零记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //需量清零总次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次需量清零记录
            	    //仅保存上一次需量清零发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x3:  //事件清零记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //事件清零总次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次事件清零记录
            	    //仅保存上一次事件清零发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;

     	 	 	  case 0x4:  //校时记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //校时总次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次校时记录
            	    //仅保存上一次校时发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i+4);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x5:  //时段编程记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //时段编程总次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次时段编程记录
            	    //仅保存上一次时段编程发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0xd:  //电表尾盖打开记录
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //开表盖总次数
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //上一次开表盖发生时刻
            	    //仅保存上一次开表盖发生时间6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
     	 	 	    }
     	 	 	    break;
     	 	  }
     	 	  break;
     	 	 
     	  case 0x32:
     	 	  //上一次购电后累计购电量
     	 	  if (pCpLink->recvBuf[11]==0x06 && pCpLink->recvBuf[10]==0x01)
     	 	  {
            for(i=0;i<4;i++)
            {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            }
     	 	  }
     	 	  break;
     	 	 
     	  case 0x33:
     	 	  //上一次购电后累计购电次数
     	 	  if (pCpLink->recvBuf[11]==0x02 && pCpLink->recvBuf[10]==0x01)
     	 	  {
            for(i=0;i<2;i++)
            {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            }
     	 	  }
     	 	 
     	 	  //上一次购电后累计购电金额
     	 	  if (pCpLink->recvBuf[11]==0x06 && pCpLink->recvBuf[10]==0x01)
     	 	  {
            for(i=0;i<4;i++)
            {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            }
     	 	  }
     	 	  break;
      }
 	 	  break;
 	 	 
 	  case PARA_VARIABLE_2007:
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //有参变量数据

 	 	  switch(pCpLink->recvBuf[12])
 	 	  {
 	 	 	  case 0x0:
     	    switch(pCpLink->recvBuf[11])
     	    {
     	   	  case 0x01:
     	        if (pCpLink->recvBuf[10]==0x01)  //日期及星期
     	 	      {
            	  for(i=0;i<4;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      if (pCpLink->recvBuf[10]==0x02)  //时间
     	 	      {
            	  for(i=0;i<3;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      break;
     	 	     
     	 	    case 0x02:
     	 	   	  //除了公共假日数本来是两个字节只存了一个字节外,其它的都按规约一个字节存储
              pCpLink->copyedData |= HAS_SHIDUAN;       //有时段数据
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset) = *pData;
     	 	   	  break;
     	 	   
     	 	    case 0x04:        	 	 
     	 	      if (pCpLink->recvBuf[10]==0x09 || pCpLink->recvBuf[10]==0x0a)  //电表常数(有功和无功)
     	 	      {
            	  for(i=0;i<3;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      if (pCpLink->recvBuf[10]==0x02)  //电表表号
     	 	      {
            	  for(i=0;i<6;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      break;
     	 	     
     	 	    case 0x05:        	 	 
     	 	      if (pCpLink->recvBuf[10]==0xff)    //电表运行状态字
     	 	      {
                //本来是7个状态字(7*2bytes),但645-1997只有一个字节的状态字
            	  for(i=0;i<14;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      break;
     	 	     
     	 	    case 0x0b:  //每月结算日(1,2,3结算日,各2个字节)
              //本来是3个结算日,但645-1997只有一个结算日
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset) = *pData;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+1);
     	 	      break;
     	 	     
     	 	    case 0x0f:
     	 	      if (pCpLink->recvBuf[10]==0x04 || pCpLink->recvBuf[10]==0x01)    //透支电量限值 /报警电量1限值
     	 	      {
            	  for(i=0;i<4;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	   	  break;
     	 	  }
     	 	  break;
     	 
     	  case 0x01:   //第一套时区表数据及时段表数据
          pCpLink->copyedData |= HAS_SHIDUAN;       //有时段数据
     	 	 
     	 	  if (pCpLink->recvBuf[10]==0x00)  //第一套时区表数据
     	 	  {
            while (loadLen>=3)
            {
          	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset)   = *(pData);
          	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+1) = *(pData + 1);
          	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+2) = *(pData + 2);
    
              offset += 3;
              pData += 3;
                
              if (loadLen<3)
              {
                break;
              }
              else
              {
                loadLen -= 3;
              }
            }
     	 	  }
     	 	  else   //日时段表数据
     	 	  {
        	  offset += 30*pCpLink->recvBuf[10];
     	 	 	 
     	 	 	  for(i=0;i<10&&loadLen>=3;i++)
     	 	 	  {
          	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset)   = *(pData);
          	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+1) = *(pData + 1);
          	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+2) = *(pData + 2);
    
              offset += 3;
              pData += 3;
                
              if (loadLen<3)
              {
                break;
              }
              else
              {
                loadLen -= 3;
              }
     	 	 	  }
     	 	  }
     	 	  break;
      }
 	 	  break;
 	 	 
 	  case FREEZE_DATA_2007:   //冻结数据
 	 	  if (pCpLink->protocol==SINGLE_PHASE_645_2007
 	 	 	    || pCpLink->protocol==SINGLE_LOCAL_CHARGE_CTRL_2007
 	 	 	     || pCpLink->protocol==SINGLE_REMOTE_CHARGE_CTRL_2007
 	 	 	   )
 	 	  {
 	 	    if (pCpLink->recvBuf[12]==0x6 && (pCpLink->recvBuf[11]==0x00 || pCpLink->recvBuf[11]==0x01 || pCpLink->recvBuf[11]==0x02))
 	 	    {
      	  pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
      	 
          if (pCpLink->recvBuf[11]==0x00)   //日冻结时标
          {
        	  *(pCpLink->dataBuff+offset)   = *(pData);
        	  *(pCpLink->dataBuff+offset+1) = *(pData + 1);
        	  *(pCpLink->dataBuff+offset+2) = *(pData + 2);
        	  *(pCpLink->dataBuff+offset+3) = *(pData + 3);
        	  *(pCpLink->dataBuff+offset+4) = *(pData + 4);
          }
          else
          {
            while (loadLen>=4)
            {
        	    *(pCpLink->dataBuff+offset)   = *(pData);
        	    *(pCpLink->dataBuff+offset+1) = *(pData + 1);
        	    *(pCpLink->dataBuff+offset+2) = *(pData + 2);
        	    *(pCpLink->dataBuff+offset+3) = *(pData + 3);
  
              offset += 4;
              pData += 4;
              
              if (loadLen<4)
              {
                break;
              }
              else
              {
                loadLen -= 4;
              }
            }
          }
        }
 	 	  }
 	 	  else
 	 	  {
 	 	    //if (pCpLink->recvBuf[12]==0x6 && pCpLink->recvBuf[11]>=0x00 && pCpLink->recvBuf[11]<0x09 && pCpLink->recvBuf[10]==0x01)
 	 	    if (pCpLink->recvBuf[12]==0x6 && pCpLink->recvBuf[11]<0x09 && pCpLink->recvBuf[10]==0x01)  //ly,2012-01-10,modify
 	 	    {
      	  pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
      	 
          if (pCpLink->recvBuf[11]==0x00)        //日冻结时标
          {
        	  *(pCpLink->dataBuff+offset)   = *(pData);
        	  *(pCpLink->dataBuff+offset+1) = *(pData + 1);
        	  *(pCpLink->dataBuff+offset+2) = *(pData + 2);
        	  *(pCpLink->dataBuff+offset+3) = *(pData + 3);
        	  *(pCpLink->dataBuff+offset+4) = *(pData + 4);
          }
          else
          {
            while (loadLen>=4)
            {
        	    *(pCpLink->dataBuff+offset)   = *(pData);
        	    *(pCpLink->dataBuff+offset+1) = *(pData + 1);
        	    *(pCpLink->dataBuff+offset+2) = *(pData + 2);
        	    *(pCpLink->dataBuff+offset+3) = *(pData + 3);
  
              offset += 4;
              pData += 4;
              
              if (loadLen<4)
              {
                break;
              }
              else
              {
                loadLen -= 4;
              }
            }
          }
        }
        else
        {
          if (pCpLink->recvBuf[12]==0x6 && (pCpLink->recvBuf[11]==0x09 || pCpLink->recvBuf[11]==0x0a) && pCpLink->recvBuf[10]==0x01)
          {
            pCpLink->copyedData |= HAS_LAST_DAY_REQ;     //有上一次日冻结需量数据

            offsetx = offset+27;
            while (loadLen>=8)
            {
         	    //需量3bytes
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData);
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 1);
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 2);
         	    
         	    //需量发生时间5bytes
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+0) = *(pData + 3);  //分
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+1) = *(pData + 4);  //时
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+2) = *(pData + 5);  //日
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+3) = *(pData + 6);  //月
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+4) = *(pData + 7);  //年
   
               offset  += 3;
               offsetx += 5;
               pData += 8;
               
               if (loadLen<8)
               {
               	  break;
               }
               else
               {
                  loadLen -= 8;
               }
            }
    	 	  }
    	 	  else
    	 	  {
            if (pCpLink->recvBuf[12]==0x4 && pCpLink->recvBuf[11]==0xff)
            {
              pCpLink->copyedData |= HAS_HOUR_FREEZE_ENERGY;     //有整点冻结电能数据
        	     
        	    //整点冻结时间
        	    for(i=0;i<5;i++)
        	    {
        	      *(pCpLink->dataBuff+128+i)   = *(pData+i);
        	    }
        	     
        	    pData+=6;   //其中有一个0xaa为分隔符
        	     
        	    //整点冻结正向有功电能示值
        	    for(i=0;i<4;i++)
        	    {
        	      *(pCpLink->dataBuff+i)   = *(pData+i);
        	    }

        	    pData+=5;   //其中有一个0xaa为分隔符
        	     
        	    //整点冻结反向有功电能示值
        	    for(i=0;i<4;i++)
        	    {
        	      *(pCpLink->dataBuff+4+i)   = *(pData+i);
        	    }
        	     
        	    // //保存该整点冻结数据
 		 	        //multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		 	        //                     pCpLink->dataBuff, HOUR_FREEZE, \
		 	        //                       0x0, LENGTH_OF_HOUR_FREEZE_RECORD);
            }
    	 	  }
        }
 	 	  }
 	 	  break;
 	 	 
 	  case EXT_EVENT_RECORD_13:    //07备案文件断相统计数据
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //有参变量数据
 	 	  if (pCpLink->recvBuf[11]==00)    //断相次数或断相累计时间
 	 	  {
        for(i=0;i<3;i++)
        {
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
        }
 	 	  }
 	 	  else                //最近一次断相起始时间及结束时间
 	 	  {
        for(i=0;i<6;i++)
        {
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
        }
 	 	  }
 	 	  break;
 	 	 
 	  case 0x1d:
 	  case 0x1e:
 	 	  if (pCpLink->recvBuf[12]==0x00 && pCpLink->recvBuf[11]==0x01 && pCpLink->recvBuf[10]==0x1)  //上一次跳/合闸发生时刻
 	 	  {
        pCpLink->copyedData |= HAS_PARA_VARIABLE; //有参变量数据
        
        for(i=0;i<6;i++)
        {
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
        }
 	 	  }
 	 	  break;
 	 	     	 	
 	  default:
 	 	  return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据
 	 	  break;
  }
   
  return 0;    //接收数据帧正常且解析正确,已保存进缓存
}

/**************************************************
函数名称:threadOfEthCopyDataRead
功能描述:读取以太网抄表TCP数据线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：void
***************************************************/
void *threadOfEthCopyDataRead(void *arg)
{
  struct cpAddrLink *pTmpCpLink;
  
  INT8U  recvBuf[1505];
  INT16U recvi, tmpi, j;
  int    recvLen;
  INT8U  checkSum=0;
  INT8U  bakData=0;
  
  INT8U  canTcpData[50];
  INT8U  lenOfCanData=0;

  printf("%02d-%02d-%02d %02d:%02d:%02d,接收以太网抄表TCP数据线程(PID=%d)启动\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, getpid());
  
  while(1)
  {
    if (fdOfClient!=NULL)
    {
      if (compareTwoTime(canTimeOut, sysTime))
      {
        printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyDataRead:接收CAN2ETH心跳数据超时,关闭客户端socket%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, fdOfClient);

        close(fdOfClient);
        fdOfClient=NULL;
        canloginIn = 0;
        
        continue;
      }
      
      if ((recvLen=read(fdOfClient, &recvBuf, 1500))<=0)
      {
        if (recvLen<0)
        {
          if (errno == ECONNRESET) 
          {
            close(fdOfClient);
            fdOfClient=NULL;

            printf("threadOfEthCopyDataRead:接收数据状态为ECONNRESET,关闭了客户端socket\n");
          }
        }
        else 
        {
        	if (recvLen==0)
        	{
            close(fdOfClient);
            fdOfClient=NULL;
            
            printf("threadOfEthCopyDataRead:接收长度为0,关闭了客户端socket\n");
          }
        }
      }
      else
      {      
        if (debugInfox&ETH_COPY_METER)
        {
          printf("%02d-%02d-%02d %02d:%02d:%02d,eth copy Rx(%d,[S%d]),", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, recvLen, fdOfClient);
          for(recvi=0; recvi<recvLen; recvi++)
          {
        	  printf("%02x ", recvBuf[recvi]);
          }
          printf("\n");
        }
        
        // 7E len afn data cs 7E
        // afn - 0 确认/否认
        //     - 1 登录/心跳
        //     - 2 抄表数据
        // cs  - 指len,afn,data的算术和
        for(recvi=0; recvi<recvLen; recvi++)
        {
        	if (flagOfFrame==0)    //没发现帧头
        	{
        	  if (0x7e==recvBuf[recvi])
        	  {
        	  	flagOfFrame = 1;    //发现帧起始
        	  	
        	  	lenOfCanData = 0;
        	  	canTcpData[lenOfCanData++] = recvBuf[recvi];
              
              if (debugInfox&ETH_COPY_METER)
              {
        	  	  printf(" - H=%d,",recvi);
        	  	}
        	  }
        	}
        	else
        	{
        		if (1==lenOfCanData && 0x7e==recvBuf[recvi])
        	  {
        	  	continue;
        	  }
        	  
        		canTcpData[lenOfCanData++] = recvBuf[recvi];
        		
        		if (lenOfCanData>2)
        		{
        		  if (lenOfCanData==canTcpData[1]+5)
        		  {
                if (debugInfox&ETH_COPY_METER)
                {
        		  	  printf("T=%d", recvi);
        		  	}
        		  	
        		  	if (0x7e==canTcpData[lenOfCanData-1])
        		  	{
                  if (debugInfox&ETH_COPY_METER)
                  {
                    printf(",Tail Ok,");
                  }
                  
                  //checkSum
                  checkSum = 0;
                  for(j=1; j<lenOfCanData-2; j++)
                  {
                    checkSum += canTcpData[j];
                  }
                  
                  if (checkSum==canTcpData[lenOfCanData-2])
                  {
                    if (debugInfox&ETH_COPY_METER)
                    {
                      printf("CS is Ok,fn=%d", canTcpData[2]);
                    }
                    switch(canTcpData[2])
                    {
                    	case 1:    //登录或是心跳
                    		if (1==canTcpData[3])
                    		{
                    			canloginIn = 1;

                          if (debugInfox&ETH_COPY_METER)
                          {
                            printf(",CAN2ETH登录");
                          }
                    		}
                    		
                        if (debugInfox&ETH_COPY_METER)
                        {
                    		  printf(",confirm it.\n");
                    		}
                    		
                    		sendToCan2Eth(0, canTcpData, 0);    //确认
                    		break;
                      
                      case 2:    //抄表数据
                        if (debugInfox&ETH_COPY_METER)
                        {
                      	  printf(",can ID=%02X%02X%02X%02X\n", canTcpData[6], canTcpData[5], canTcpData[4], canTcpData[3]);
                      	}
                      	
                      	pTmpCpLink = pEthCpLinkHead;
                        
                        while(pTmpCpLink!=NULL)
                        {
                        	if (pTmpCpLink->addr[0]==canTcpData[3]
                        		  && pTmpCpLink->addr[1]==canTcpData[4]
                        		   && pTmpCpLink->addr[2]==canTcpData[5]
                        		    && (pTmpCpLink->addr[3]&0x7)==(canTcpData[6]&0x7)
                        		 )
                        	{
                        		memcpy(&pTmpCpLink->recvBuf[pTmpCpLink->lenOfRecv], &canTcpData[7], canTcpData[1]-4);
                        		pTmpCpLink->lenOfRecv += canTcpData[1]-4;
                        		
                        		if (0x16==pTmpCpLink->recvBuf[pTmpCpLink->lenOfRecv-1])
                        		{
                        			if (debugInfo&PRINT_645_FRAME)
                              {
                                if (pTmpCpLink->mp==monitorMp || 0==monitorMp)
                                {
                                  printf("%02d-%02d-%02d %02d:%02d:%02d,%02x%02x%02x%02x%02x%02x Rx(%d),", 
                                        sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, 
                                         pTmpCpLink->recvBuf[6],pTmpCpLink->recvBuf[5],pTmpCpLink->recvBuf[4],pTmpCpLink->recvBuf[3],pTmpCpLink->recvBuf[2],pTmpCpLink->recvBuf[1],
                                           pTmpCpLink->lenOfRecv);
                                  for(j=0; j<pTmpCpLink->lenOfRecv; j++)
                                  {
                              	    printf("%02x ", pTmpCpLink->recvBuf[j]);
                                  }
                                  printf("\n");
                                }
                              }

                              //计算校验和且按字节将数据域进行减0x33处理
                              checkSum = 0;
                              for(tmpi=0; tmpi<pTmpCpLink->lenOfRecv-2; tmpi++)
                              {
                                checkSum += pTmpCpLink->recvBuf[tmpi];
                                
                                if (tmpi>9)
                                {
                                  pTmpCpLink->recvBuf[tmpi] -= 0x33;    //按字节进行对数据域减0x33处理
                                }
                              }
                              
                              //如果校验和正确,执行接收操作
                              if (checkSum == pTmpCpLink->recvBuf[pTmpCpLink->lenOfRecv-2])
                              {
                                if (debugInfox&ETH_COPY_METER)
                                {
                                  printf(" -> 表号%02x%02x%02x%02x%02x%02x",
                                        pTmpCpLink->recvBuf[6], pTmpCpLink->recvBuf[5], pTmpCpLink->recvBuf[4], pTmpCpLink->recvBuf[3], pTmpCpLink->recvBuf[2], pTmpCpLink->recvBuf[1]
                                        );
                                }

                                switch(pTmpCpLink->recvBuf[8])
                                {
                                	case 0x81:    //97表正确应答
                                		analyse645Dlt1997(pTmpCpLink);
                                		break;
                                		
                                	case 0xc1:    //97表异常应答
                                    if (debugInfox&ETH_COPY_METER)
                                    {
                                    	printf(",1997规约,异常应答\n");
                                    }
                                		break;

                                	case 0x91:    //07表正确应答
                                		analyse645Dlt2007(pTmpCpLink);
                                		break;
                                		
                                	case 0xd1:    //07表异常应答
                                    if (debugInfox&ETH_COPY_METER)
                                    {
                                    	printf(",2007规约,异常应答\n");
                                    }                                		
                                		break;
                                		
                                	default:
                                    if (debugInfox&ETH_COPY_METER)
                                    {
                                		  printf(",未处理的应答(0x%0x)\n", pTmpCpLink->recvBuf[8]);
                                		}
                                		break;                               	
                                }
                              }
                              
                              
                              pTmpCpLink->copyContinue = TRUE;
                        		}
                        		
                        		break;
                        	}
                        	
                        	pTmpCpLink = pTmpCpLink->next;
                        }
                      	
                      	break;
                    }
                    
                    canTimeOut = nextTime(sysTime, 0, 10);    //10的心跳超时时间
                  }
                  else
                  {
                    if (debugInfox&ETH_COPY_METER)
                    {
                  	  printf("帧校验错误\n");
                  	}
                  }
        		  	}
        		  	else
        		  	{
                  if (debugInfox&ETH_COPY_METER)
                  {
        		  		  printf("Tail is bad\n");
        		  		}
        		  	}
        		    
        		    flagOfFrame = 0;
        		  }
        		}
        	}
        }
      }
    }

    usleep(100);
  }
   
  close(fdOfClient);

  printf("%02d-%02d-%02d %02d:%02d:%02d,接收以太网抄表TCP数据线程终止(socket=%d)\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, fdOfClient);
}

/**************************************************
函数名称:threadOfEthCopyServer
功能描述:以太网抄表TCP通信Server线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void *threadOfEthCopyServer(void *arg)
{
  int                fdOfServerSocket;    //监听socket
  int                tmpSocket;           //临时Socket
  struct sockaddr_in myAddr;              //本机地址信息
  struct sockaddr_in clientAddr;          //客户地址信息
  unsigned int       sin_size, myport, lisnum; 
  INT8U              tmpBuf[5];
  pthread_t          id;
  int                on = 1;

  myport = 8800;
  lisnum = 2;

  if ((fdOfServerSocket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
     
    //exit(1); 
  }
  printf("socket %d ok \n", myport);
  
  //设置地址可立即重用和监听
  setsockopt(fdOfServerSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  
  myAddr.sin_family=PF_INET;
  myAddr.sin_port=htons(myport);
  myAddr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(myAddr.sin_zero), 0);
  if (bind(fdOfServerSocket, (struct sockaddr *)&myAddr, sizeof(struct sockaddr)) == -1)
  {
    perror("bind"); 
    //exit(1);
  }
  printf("bind ok \n");

  if (listen(fdOfServerSocket, lisnum) == -1)
  {
    perror("listen"); 
    //exit(1); 
  }
  printf("listen ok \n");
  fdOfClient = NULL;
  while(1)
  {
    sin_size = sizeof(struct sockaddr_in); 
    if ((tmpSocket = accept(fdOfServerSocket, (struct sockaddr *)&clientAddr, &sin_size)) == -1)
    {
      perror("accept");
      
      break;
    }
    
    printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyServer:got connection from %s,fdOfClient=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, inet_ntoa(clientAddr.sin_addr),tmpSocket);

    //同一时刻只允许一个设备连接
    if (fdOfClient!=NULL)
    { 
      printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyServer:新socket登录,关闭了了原客户端socket=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, fdOfClient);

    	close(fdOfClient);
      fdOfClient = NULL;
    }
    
    canTimeOut = nextTime(sysTime, 0, 20);
    canloginIn = 0;
    flagOfFrame = 0;    //接收标志置为未开始
    
    setNonBlocking(tmpSocket);
    fdOfClient = tmpSocket;    
  }
  
  printf("线程Server结束\n");
}

/***************************************************
函数名称:threadOfEthCopyMeter
功能描述:通过以太网抄表线程
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：void
***************************************************/
void *threadOfEthCopyMeter(void *arg)
{
  struct cpAddrLink *pTmpCpLink;
  INT8U  i;
  INT8U  buf[50];
  INT8U  tmpTail;
  
  pEthCpLinkHead = NULL;
  
  setEthMeter = 1;
  msSetWaitTime = sysTime;

	while(1)
	{
    if (1==setEthMeter)
    {
      if (compareTwoTime(msSetWaitTime, sysTime))
      {
        if (debugInfo&METER_DEBUG)
        {
          printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyMeter,初始以太网抄表链表\n", 
                 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second
                );
        }
        
        pTmpCpLink = pEthCpLinkHead;
        while(pTmpCpLink!=NULL)
        {
  	      pTmpCpLink->nextCopyTime = nextTime(sysTime, 2, 0);
  	      pTmpCpLink->nextCopyTime.second = 0;
  	      pTmpCpLink->copying = FALSE;
  	  
  	      pTmpCpLink = pTmpCpLink->next;
  	    }
  	    
  	    sleep(2);

        //删除链表中的各节点
        while(pEthCpLinkHead!=NULL)
        {
          pTmpCpLink = pEthCpLinkHead;
        	
        	pEthCpLinkHead = pEthCpLinkHead->next;
        	free(pTmpCpLink);
        }
        pEthCpLinkHead = NULL;
        
        //初始化端口5电表链表
        pEthCpLinkHead = initPortMeterLink(4);
    
        pTmpCpLink = pEthCpLinkHead;
        while(pTmpCpLink!=NULL)
        {
  	      //下一分钟0秒开始抄表
  	      pTmpCpLink->nextCopyTime = nextTime(sysTime, 1, 0);
  	      pTmpCpLink->nextCopyTime.second = 0;
  	      pTmpCpLink->copying = FALSE;
  	  
  	      pTmpCpLink = pTmpCpLink->next;
  	    }
  	    
  	    setEthMeter = 0;

        if (debugInfo&METER_DEBUG)
        {
          printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyMeter,初始以太网抄表链表完成\n", 
                 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second
                );
        }
  	  }
	  }

		if (1==canloginIn)
		{
  		pTmpCpLink = pEthCpLinkHead;
  		
  		while(pTmpCpLink!=NULL)
  	  {
    		if (pTmpCpLink->copying==FALSE)
    		{
    			if (compareTwoTime(pTmpCpLink->nextCopyTime, sysTime))
    			{
      			pTmpCpLink->nextCopyTime = nextTime(sysTime, teCopyRunPara.para[5].copyInterval, 0);
      			pTmpCpLink->copying = TRUE;
      			pTmpCpLink->copyItem = 0;
      			pTmpCpLink->totalCopyItem = TOTAL_CMD_CURRENT_645_2007;
      			pTmpCpLink->copyContinue = TRUE;
            pTmpCpLink->retry = 0;
            pTmpCpLink->copyedData = 0;
            memset(pTmpCpLink->dataBuff, 0xee, 1536);
  
      			if (debugInfo&METER_DEBUG)
      			{
              printf("%02d-%02d-%02d %02d:%02d:%02d,ethCopyMeter:电表%02x%02x%02x%02x%02x%02x开始抄表\n", sysTime.year, 
                     sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
                      pTmpCpLink->addr[5],pTmpCpLink->addr[4],pTmpCpLink->addr[3],pTmpCpLink->addr[2],pTmpCpLink->addr[1],pTmpCpLink->addr[0]
                    );
      		  }
    			}
    		}
    		else
    		{
      	  if (pTmpCpLink->copyContinue==TRUE || pTmpCpLink->flagOfRetry==TRUE)
      	  {
      	    if (pTmpCpLink->copyContinue==TRUE)
      	    {
        	    pTmpCpLink->copyItem++;
              pTmpCpLink->retry = 0;
      	    }
      	    
      	    if (pTmpCpLink->flagOfRetry==TRUE)
      	    {
              if (pTmpCpLink->retry<2)
              {
              	pTmpCpLink->retry++;
              }
              else
              {
        	      pTmpCpLink->copyItem++;
        	      pTmpCpLink->retry = 0;
        	    }
        	  }
        	  
        	  if (pTmpCpLink->copyItem>=pTmpCpLink->totalCopyItem)
        	  {
      			  pTmpCpLink->copying = FALSE;
      			  
      			  if (debugInfo&METER_DEBUG)
      			  {
                printf("%02d-%02d-%02d %02d:%02d:%02d,ethCopyMeter:电表%02x%02x%02x%02x%02x%02x本轮抄表结束,copyedData=%X\n", sysTime.year, 
                       sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
                        pTmpCpLink->addr[5],pTmpCpLink->addr[4],pTmpCpLink->addr[3],pTmpCpLink->addr[2],pTmpCpLink->addr[1],pTmpCpLink->addr[0]
                        ,pTmpCpLink->copyedData
                      );
              }
        		  
        		  //有采集到的电能量数据(当前)
        		  if (pTmpCpLink->copyedData&HAS_CURRENT_ENERGY)
        		  {
        		 	  saveMeterData(pTmpCpLink->mp, 5, timeHexToBcd(sysTime), pTmpCpLink->dataBuff, 
                              PRESENT_DATA, ENERGY_DATA, LENGTH_OF_ENERGY_RECORD);
        		  }
        	   
        	    //有采集到的需量及发生时间数据(当前)
              if (pTmpCpLink->copyedData&HAS_CURRENT_REQ)
              {
        		 	  saveMeterData(pTmpCpLink->mp, 5, timeHexToBcd(sysTime), &pTmpCpLink->dataBuff[LENGTH_OF_ENERGY_RECORD], 
                              PRESENT_DATA, REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
              }
             
        	    //有采集到的参量及参变量数据(当前)
              if (pTmpCpLink->copyedData&HAS_PARA_VARIABLE)
              {
      		      //由于DLT_645_2007抄不到总断相次数和累计断相时间及最近一次断相时间,计算得到 10-07-05
                /*
                if (multiCopy[arrayItem].protocol == DLT_645_2007)
                {
                 //现采取的办法是用A,B,C相的次数和时间来加起来计为总断相次数和累计时间
             	  pParaData = pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD;
             	
             	  //计算总断相次数
             	  tmpData = 0;
             	  if (pParaData[PHASE_A_DOWN_TIMES]!=0xee)
             	  {
             	    tmpData += bcdToHex(pParaData[PHASE_A_DOWN_TIMES] | pParaData[PHASE_A_DOWN_TIMES+1]<<8 | pParaData[PHASE_A_DOWN_TIMES+2]<<16);
             	  }
             	  if (pParaData[PHASE_B_DOWN_TIMES]!=0xee)
             	  {
             	    tmpData += bcdToHex(pParaData[PHASE_B_DOWN_TIMES] | pParaData[PHASE_B_DOWN_TIMES+1]<<8 | pParaData[PHASE_B_DOWN_TIMES+2]<<16);
             	  }
             	  if (pParaData[PHASE_A_DOWN_TIMES]!=0xee)
             	  {
             	    tmpData += bcdToHex(pParaData[PHASE_C_DOWN_TIMES] | pParaData[PHASE_C_DOWN_TIMES+1]<<8 | pParaData[PHASE_C_DOWN_TIMES+2]<<16);
             	  }
             	  tmpData = hexToBcd(tmpData);
             	  pParaData[PHASE_DOWN_TIMES]   = tmpData&0xff;
             	  pParaData[PHASE_DOWN_TIMES+1] = tmpData>>8&0xff;
             	  pParaData[PHASE_DOWN_TIMES+2] = tmpData>>16&0xff;
      
             	  //计算累计断相时间
             	  tmpData = 0;
             	  if (pParaData[TOTAL_PHASE_A_DOWN_TIME]!=0xee)
             	  {
             	     tmpData += bcdToHex(pParaData[TOTAL_PHASE_A_DOWN_TIME] | pParaData[TOTAL_PHASE_A_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_A_DOWN_TIME+2]<<16);
             	  }
             	  if (pParaData[TOTAL_PHASE_B_DOWN_TIME]!=0xee)
             	  {
             	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_B_DOWN_TIME] | pParaData[TOTAL_PHASE_B_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_B_DOWN_TIME+2]<<16);
             	  }
             	  if (pParaData[TOTAL_PHASE_B_DOWN_TIME]!=0xee)
             	  {
             	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_C_DOWN_TIME] | pParaData[TOTAL_PHASE_C_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_C_DOWN_TIME+2]<<16);
             	  }
             	  tmpData = hexToBcd(tmpData);
             	  pParaData[TOTAL_PHASE_DOWN_TIME] = tmpData;
             	  pParaData[TOTAL_PHASE_DOWN_TIME+1] = tmpData>>8;
             	  pParaData[TOTAL_PHASE_DOWN_TIME+2] = tmpData>>16;
      
             	  //最近一次断相起始时间
             	  if (pParaData[LAST_PHASE_A_DOWN_BEGIN]!=0xee && pParaData[LAST_PHASE_B_DOWN_BEGIN]!=0xee && pParaData[LAST_PHASE_C_DOWN_BEGIN]!=0xee)
             	  {
                 	memcpy((INT8U *)&tmpTimeA,pParaData+LAST_PHASE_A_DOWN_BEGIN,6);
                 	memcpy((INT8U *)&tmpTimeB,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
                 	memcpy((INT8U *)&tmpTimeC,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
                 	tmpTimeA = timeBcdToHex(tmpTimeA);
                 	tmpTimeB = timeBcdToHex(tmpTimeB);
                 	tmpTimeC = timeBcdToHex(tmpTimeC);
                 	
                 	if (compareTwoTime(tmpTimeA, tmpTimeB))       //B>A
                 	{
                 	  if (compareTwoTime(tmpTimeC, tmpTimeB))     //B>C 
                 	  {
                       memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
                 	  }
                 	  else
                 	  {
                 	    if (compareTwoTime(tmpTimeB, tmpTimeC))   //C>B
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
                 	    }
                 	    else                                      //C=B
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
                 	    }
                 	  }
                 	}
                 	else                                       //B<=A
                 	{
                 	  if (compareTwoTime(tmpTimeB, tmpTimeA))  //A>B
                 	  {
                 	    if (compareTwoTime(tmpTimeC, tmpTimeA))  //A>C
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_A_DOWN_BEGIN,6);
                 	    }
                 	    else
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
                 	    }
                 	  }
                 	  else                                      //A<=B
                 	  {
                 	    if (compareTwoTime(tmpTimeC, tmpTimeB))  //B>C
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
                 	    }
                 	    else
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
                 	    }
                 	  }
                 	}
                 }         	
     
               	 
               	//最近一次断相结束时间
             	  if (pParaData[LAST_PHASE_A_DOWN_END]!=0xee && pParaData[LAST_PHASE_B_DOWN_END]!=0xee && pParaData[LAST_PHASE_C_DOWN_END]!=0xee)
             	  {
                 	memcpy((INT8U *)&tmpTimeA,pParaData+LAST_PHASE_A_DOWN_END,6);
                 	memcpy((INT8U *)&tmpTimeB,pParaData+LAST_PHASE_B_DOWN_END,6);
                 	memcpy((INT8U *)&tmpTimeC,pParaData+LAST_PHASE_C_DOWN_END,6);
                 	tmpTimeA = timeBcdToHex(tmpTimeA);
                 	tmpTimeB = timeBcdToHex(tmpTimeB);
                 	tmpTimeC = timeBcdToHex(tmpTimeC);
                 	
                 	if (compareTwoTime(tmpTimeA, tmpTimeB))       //B>A
                 	{
                 	  if (compareTwoTime(tmpTimeC, tmpTimeB))     //B>C 
                 	  {
                       memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
                 	  }
                 	  else
                 	  {
                 	    if (compareTwoTime(tmpTimeB, tmpTimeC))   //C>B
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
                 	    }
                 	    else                                      //C=B
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
                 	    }
                 	  }
                 	}
                 	else                                       //B<=A
                 	{
                 	  if (compareTwoTime(tmpTimeB, tmpTimeA))  //A>B
                 	  {
                 	    if (compareTwoTime(tmpTimeC, tmpTimeA))  //A>C
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_A_DOWN_END,6);
                 	    }
                 	    else
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
                 	    }
                 	  }
                 	  else                                      //A<=B
                 	  {
                 	    if (compareTwoTime(tmpTimeC, tmpTimeB))  //B>C
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
                 	    }
                 	    else
                 	    {
                          memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
                 	    }
                 	  }
                 	}
                 }
                }
                */
                
        		 	  saveMeterData(pTmpCpLink->mp, 5, timeHexToBcd(sysTime), &pTmpCpLink->dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD], 
                              PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
              }
         		
              //有采集到的时段数据
              if (pTmpCpLink->copyedData&HAS_SHIDUAN)
              {
        		 	  saveMeterData(pTmpCpLink->mp, 5, timeHexToBcd(sysTime), &pTmpCpLink->dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD], 
                              PRESENT_DATA, SHI_DUAN_DATA, LENGTH_OF_SHIDUAN_RECORD);
              }
        	  }
        	  else
        	  { 
        	    buf[0] = 0x68;
        	    buf[1] = pTmpCpLink->addr[0];
        	    buf[2] = pTmpCpLink->addr[1];
        	    buf[3] = pTmpCpLink->addr[2];
        	    buf[4] = pTmpCpLink->addr[3];
        	    buf[5] = pTmpCpLink->addr[4];
        	    buf[6] = pTmpCpLink->addr[5];
        	    buf[7] = 0x68;
        	    tmpTail = 8;
         			if (debugInfo&PRINT_645_FRAME)
              {
                if (pTmpCpLink->mp==monitorMp || 0==monitorMp)
                {
                  printf("%02d-%02d-%02d %02d:%02d:%02d,%02x%02x%02x%02x%02x%02x Tx(", 
                         sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, 
                         pTmpCpLink->addr[5],pTmpCpLink->addr[4],pTmpCpLink->addr[3],pTmpCpLink->addr[2],pTmpCpLink->addr[1],pTmpCpLink->addr[0]
                        );
                }
              }

        	    if (pTmpCpLink->protocol==DLT_645_2007)
        	    {
        	      buf[tmpTail++] = 0x11;
        	      buf[tmpTail++] = 0x04;      	  
        	      buf[tmpTail++] = cmdDlt645Current2007[pTmpCpLink->copyItem-1][3]+0x33;
        	      buf[tmpTail++] = cmdDlt645Current2007[pTmpCpLink->copyItem-1][2]+0x33;
        	      buf[tmpTail++] = cmdDlt645Current2007[pTmpCpLink->copyItem-1][1]+0x33;
        	      buf[tmpTail++] = cmdDlt645Current2007[pTmpCpLink->copyItem-1][0]+0x33;
        	      if (debugInfo&PRINT_645_FRAME)
                {
                  if (pTmpCpLink->mp==monitorMp || 0==monitorMp)
                  {
                	  printf("%02X %02X %02X %02X", cmdDlt645Current2007[pTmpCpLink->copyItem-1][0],cmdDlt645Current2007[pTmpCpLink->copyItem-1][1],cmdDlt645Current2007[pTmpCpLink->copyItem-1][2],cmdDlt645Current2007[pTmpCpLink->copyItem-1][3]);
                  }
                }
        	    }
        	    else
        	    {
        	      buf[tmpTail++] = 0x01;
        	      buf[tmpTail++] = 0x02;
        	      buf[tmpTail++] = cmdDlt645Current1997[pTmpCpLink->copyItem-1][1]+0x33;
        	      buf[tmpTail++] = cmdDlt645Current1997[pTmpCpLink->copyItem-1][0]+0x33;      	    	

        	      if (debugInfo&PRINT_645_FRAME)
                {
                  if (pTmpCpLink->mp==monitorMp || 0==monitorMp)
                  {
                	  printf("%02X %02X", cmdDlt645Current1997[pTmpCpLink->copyItem-1][0], cmdDlt645Current1997[pTmpCpLink->copyItem-1][1]);
                  }
                }
        	    }
        	    buf[tmpTail] = 0;
        	    for(i=0; i<tmpTail; i++)
        	    {
        	      buf[tmpTail] += buf[i];
        	    }
        	    tmpTail++;
        	    buf[tmpTail++] = 0x16;
        	    
         			if (debugInfo&PRINT_645_FRAME)
              {
                if (pTmpCpLink->mp==monitorMp || 0==monitorMp)
                {
                  printf(",%d),", tmpTail);
                
                  for(i=0; i<tmpTail; i++)
                  {
               	    printf("%02x ", buf[i]);
                  }
                  printf("\n");
                }
              }
  
              sendToCan2Eth(2, buf, tmpTail);

              pTmpCpLink->lenOfRecv = 0;            
              pTmpCpLink->copyContinue = FALSE;
              pTmpCpLink->copyTimeOut = nextTime(sysTime, 0, 4);
              pTmpCpLink->flagOfRetry = FALSE;
            }
        	}
          else
          {
            //超时处理
       	    if (compareTwoTime(pTmpCpLink->copyTimeOut, sysTime))
     	  	  {
     	  	  	pTmpCpLink->flagOfRetry = TRUE;
     	  	  }
     	  	}
    		}
    		
    		pTmpCpLink = pTmpCpLink->next;
      }
    }
		
		usleep(10);
	}
}



#endif    // #ifdef SUPPORT_ETH_COPY,about line 13400


