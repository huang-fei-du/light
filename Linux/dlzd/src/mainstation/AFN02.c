/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN02.c
作者：leiyong
版本：0.9
完成日期：2006年7月
描述：主站AFN02(链路接口检测)处理文件。
函数列表：
     1.
修改历史：
  01,06-7-28,Leiyong created.
  02,09-03-10,Leiyong增加,心跳连接次数统计
***************************************************/

#include "common.h"

#include "wirelessModem.h"

#include "teRunPara.h"
#include "statistics.h"
#include "msOutput.h"

#include "msSetPara.h"
#include "userInterface.h"
#include "copyMeter.h"

#include "ioChannel.h"
#include "hardwareConfig.h"

#include "AFN02.h"

#ifdef CDMA_HEART_BEAT
 extern DATE_TIME      nextCdmaHeartTime;            //下一次CDMA心跳时间
#endif

extern FRAME_QUEUE          fQueue;                  //帧队列

/*******************************************************
函数名称:AFN02001Login
功能描述:发送登录命令函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN02001Login(void)
{
   INT8U     i;
   INT8U     checkSum;
   INT16U    tmpData;
   INT16U    frameTail02;
   
   if (fQueue.tailPtr == 0)
   {
      frameTail02 = 0;
   }
   else
   {
      frameTail02 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }
   
   if (debugInfo&WIRELESS_DEBUG)
   {
   	 printf("AFN02001Login:frameTail02=%d\n", frameTail02);
   }
   
   msFrame[frameTail02+0] = 0x68;
  
   msFrame[frameTail02+1] = 0x30 | PROTOCOL_FIELD;
   msFrame[frameTail02+2] = 0;  
   msFrame[frameTail02+3] = 0x30 | PROTOCOL_FIELD;
   msFrame[frameTail02+4] = 0;
  
   msFrame[frameTail02+5] = 0x68;
   
   msFrame[frameTail02+6] = 0xc9;
  
   msFrame[frameTail02+7] = addrField.a1[0];
   msFrame[frameTail02+8] = addrField.a1[1];
   msFrame[frameTail02+9] = addrField.a2[0];
   msFrame[frameTail02+10] = addrField.a2[1];
   msFrame[frameTail02+11] = 0x0;              //MSA=0
  
   msFrame[frameTail02+12] = 0x2;              //AFN
   msFrame[frameTail02+13] = 0x70 | (pSeq&0xf);    //SEQ
   pSeq++;
  
   msFrame[frameTail02+14] = 0;     //DA1
   msFrame[frameTail02+15] = 0;     //DA2
  
   msFrame[frameTail02+16] = 0x1;   //DT1  
   msFrame[frameTail02+17] = 0;     //DT2
  
   checkSum = 0;
   for (i=6;i<18;i++)
   {
      checkSum += msFrame[frameTail02+i];
   }
   msFrame[frameTail02+18] = checkSum;
  
   msFrame[frameTail02+19] = 0x16;
   
   fQueue.frame[fQueue.tailPtr].head = frameTail02;
   fQueue.frame[fQueue.tailPtr].len = 20;
  
   /*  ly,2011-10-11,注释,改到replyToMs统一发送
   switch (moduleType)
   {
   	 case GPRS_SIM300C:
   	 case GPRS_GR64:
     case ETHERNET:
       sendWirelessFrame(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
       break;
       
     case CDMA_DTGS800:
       sendWirelessFrame(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
       
       #ifdef CDMA_HEART_BEAT
        nextCdmaHeartTime = nextTime(sysTime,0,30);
       #endif
     	 break;
     	 
     case GPRS_MC52I:
     case GPRS_M590E:
     case CDMA_CM180:
     	 wlModemRequestSend(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);
     	 break;
   }
   */
   
   if ((frameTail02+20+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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

   //ly,2011-10-11,add
   if (fQueue.sendPtr!=fQueue.tailPtr)
   {
     if (debugInfo&WIRELESS_DEBUG)
     {
     	 printf("AFN02001Login:启动发送\n");
     }

     //启动定时器发送TCP数据
     fQueue.inTimeFrameSend = TRUE;
   }
   
   
   //fQueue.sendPtr = fQueue.tailPtr;
   //fQueue.thisStartPtr = fQueue.tailPtr;

   //记录日发起连接次数及发送字节数
   //frameReport(1, 20, 1);
}

/*******************************************************
函数名称:AFN02001Logout
功能描述:发送退出登录命令函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN02002Logout(void)
{
   INT8U i;
   INT8U checkSum;
   INT16U    frameTail02;
   
   if (fQueue.tailPtr == 0)
   {
      frameTail02 = 0;
   }
   else
   {
      frameTail02 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }
   
   msFrame[frameTail02+0] = 0x68;
  
   msFrame[frameTail02+1] = 0x30 | PROTOCOL_FIELD;
   msFrame[frameTail02+2] = 0;
   msFrame[frameTail02+3] = 0x30 | PROTOCOL_FIELD;
   msFrame[frameTail02+4] = 0;
  
   msFrame[frameTail02+5] = 0x68;
   
   msFrame[frameTail02+6] = 0xc9;
  
   msFrame[frameTail02+7] = addrField.a1[0];
   msFrame[frameTail02+8] = addrField.a1[1];
   msFrame[frameTail02+9] = addrField.a2[0];
   msFrame[frameTail02+10] = addrField.a2[1];
   msFrame[frameTail02+11] = 0x0;              //MSA=0
  
   msFrame[frameTail02+12] = 0x2;              //AFN
   msFrame[frameTail02+13] = 0x70 | (pSeq&0xf);    //SEQ
   pSeq++;
  
   msFrame[frameTail02+14] = 0;      //DA1
   msFrame[frameTail02+15] = 0;      //DA2
  
   msFrame[frameTail02+16] = 0x2;    //DT1  
   msFrame[frameTail02+17] = 0;      //DT2
  
   checkSum = 0;
   for (i=6;i<18;i++)
   {
      checkSum += msFrame[frameTail02+i];
   }
   msFrame[frameTail02+18] = checkSum;
  
   msFrame[frameTail02+19] = 0x16;

   fQueue.frame[fQueue.tailPtr].head = frameTail02;
   fQueue.frame[fQueue.tailPtr].len = 20;
   
   /*  ly,2011-10-11,注释,改到replyToMs统一发送
   switch (moduleType)
   {
   	 case GPRS_SIM300C:
   	 case GPRS_GR64:
     case ETHERNET:
       sendWirelessFrame(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
       break;
       
     case CDMA_DTGS800:
       sendWirelessFrame(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
     	 break;
     	 
   	 case GPRS_M590E:
     case CDMA_CM180:
     	 wlModemRequestSend(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);
     	 break;
   }
   */
   
   if ((frameTail02+20+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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

   //ly,2011-10-11,add
   if (fQueue.sendPtr!=fQueue.tailPtr)
   {
     //启动定时器发送TCP数据
     fQueue.inTimeFrameSend = TRUE;
   }

   //fQueue.sendPtr = fQueue.tailPtr;
   //fQueue.thisStartPtr = fQueue.tailPtr;

 	 //frameReport(1, 20, 0);
}

/*******************************************************
函数名称:AFN02003HeartBeat
功能描述:发送终端心跳动作函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void()
*******************************************************/
void AFN02003HeartBeat(void)
{
  //INT8U  i;
  INT16U i;    //2015-11-06,修改这个错误,水土集中器一直不能将灯灭信息发上来，由于i是8位,造成了死循环
  INT8U  checkSum;
  INT16U tmpData;
  INT16U frameTail02;
  INT16U tmpTail=0;
   
 #ifdef LIGHTING
  INT8U  weekNumber;
	INT16U tmpSuccess, tmpCount;
	struct cpAddrLink *tmpNode;
    
	weekNumber = dayWeek(2000+sysTime.year,sysTime.month,sysTime.day);
	if (weekNumber == 0)
	{
	  weekNumber = 7;
	}
 #endif
   
  if (fQueue.tailPtr == 0)
  {
    frameTail02 = 0;
  }
  else
  {
    frameTail02 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
  }
  
  msFrame[frameTail02+0] = 0x68;

  //2015-7-17,长度L在后面填写
     
  msFrame[frameTail02+5] = 0x68;
  
  msFrame[frameTail02+6] = 0xc9;            //C:11001001
  
  msFrame[frameTail02+7] = addrField.a1[0];
  msFrame[frameTail02+8] = addrField.a1[1];
  msFrame[frameTail02+9] = addrField.a2[0];
  msFrame[frameTail02+10] = addrField.a2[1];
  msFrame[frameTail02+11] = 0x0;              //MSA=0
  
  msFrame[frameTail02+12] = 0x2;              //AFN
  msFrame[frameTail02+13] = 0x70 | (pSeq&0xf);//SEQ
  pSeq++;
  
  msFrame[frameTail02+14] = 0;     //DA1
  msFrame[frameTail02+15] = 0;     //DA2
  
  msFrame[frameTail02+16] = 0x4;   //DT1
  msFrame[frameTail02+17] = 0;     //DT2
   
 #ifdef LIGHTING

	msFrame[frameTail02+18] = sysTime.second/10<<4 | sysTime.second%10;    //秒(前四位BCD码十位，后四位BCD码个位)
	msFrame[frameTail02+19] = sysTime.minute/10<<4 | sysTime.minute%10;    //分(前四位BCD码十位，后四位BCD码个位)
	msFrame[frameTail02+20] = sysTime.hour  /10<<4 | sysTime.hour  %10;    //时(前四位BCD码十位，后四位BCD码个位)
	msFrame[frameTail02+21] = sysTime.day   /10<<4 | sysTime.day   %10;    //日(前四位BCD码十位，后四位BCD码个位)
	  	
	if (sysTime.month<10)
	{
	  msFrame[frameTail02+22] = weekNumber<<5 | sysTime.month;             //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	}
	else
	{
	  msFrame[frameTail02+22] = weekNumber<<5 | 0x10 | sysTime.month%10;   //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	}
	msFrame[frameTail02+23] = sysTime.year/10<<4 | sysTime.year%10;        //年(十位+个位)

	//2015-7-14
	msFrame[frameTail02+24] = wlRssi;    //无线MODEM信号
  if (teIpAndPort.ethIfLoginMs==0x55)
  {
    msFrame[frameTail02+24] |= 0x80;
  }
	 
	//2015-7-14,光照度
	tmpTail = frameTail02+25;
	msFrame[tmpTail] = 0;
	if (lsLink!=NULL)
	{
		msFrame[tmpTail++] = 3;
	 	msFrame[tmpTail++] = lsLink->duty[0];
	 	msFrame[tmpTail++] = lsLink->duty[1];
	 	msFrame[tmpTail++] = lsLink->duty[2];
	}
	else
	{
	 	tmpTail++;
	}
   
  //2015-7-20,已同步的单灯控制器数量
  tmpNode = copyCtrl[4].cpLinkHead;
  tmpSuccess = 0;
  tmpCount   = 0;
  while(tmpNode!=NULL)
  {
  	tmpCount++;
  	if (tmpNode->copySuccess==TRUE)
  	{
  	  tmpSuccess++;  	 	  
  	}
  
  	tmpNode = tmpNode->next;
  }
  msFrame[tmpTail++] = tmpSuccess&0xff;
  msFrame[tmpTail++] = tmpSuccess>>8&0xff;
  msFrame[tmpTail++] = tmpCount&0xff;
  msFrame[tmpTail++] = tmpCount>>8&0xff;
  
  //2015-10-26,状态更新的控制点
  tmpCount = 0;  
  tmpSuccess = tmpTail;    //tmpSuccess存放有更新的控制点的个数
  tmpTail += 2;
  
  //线路控制点
  tmpNode = xlcLink;
  while(tmpNode!=NULL)
  {
  	 if( 
		    (1==(tmpNode->statusUpdated&0x1))
				 || statusHeartBeat>=600    //2016-11-24,Add,每10分钟更新一次
			 )
  	 {
  	 	 //控制点号
  	 	 msFrame[tmpTail++] = tmpNode->mp&0xff;
  	 	 msFrame[tmpTail++] = tmpNode->mp>>8&0xff;
  	 	 
  	 	 //状态
  	 	 msFrame[tmpTail++] = tmpNode->status;
  	 	 
  	 	 tmpNode->statusUpdated &= 0xfe;
  	 	 tmpCount++;
  	 }
  	 
  	 tmpNode = tmpNode->next;
  }
  
  //单灯控制点
  tmpNode = copyCtrl[4].cpLinkHead;
  while(tmpNode!=NULL)
  {
  	if (
		    (1==(tmpNode->statusUpdated&0x1))
				 || statusHeartBeat>=600    //2016-11-24,Add,每10分钟更新一次
			 )
  	{
  	 	//控制点号
  	 	msFrame[tmpTail++] = tmpNode->mp&0xff;
  	 	msFrame[tmpTail++] = tmpNode->mp>>8&0xff;
  	 	 
  	 	//亮度
  	 	msFrame[tmpTail++] = tmpNode->status;
  	 	 
  	 	tmpNode->statusUpdated &= 0xfe;
  	 	tmpCount++;
  	}
  	 
  	//多于200个控制点有更新状态,下次心跳再发送
  	if (tmpCount>=200)
  	{
  	 	break;
  	}
  	 
  	tmpNode = tmpNode->next;
  }
  
  //状态更新的控制点数量
  msFrame[tmpSuccess] = tmpCount&0xff;
  msFrame[tmpSuccess+1] = tmpCount>>8&0xff;
  
  //2016-03-01,日出日落状态更新的线路控制点
  tmpCount = 0;
  tmpSuccess = tmpTail;    //tmpSuccess存放有更新的控制点的个数
  tmpTail += 2;
  
  //线路控制点
  tmpNode = xlcLink;
  while(tmpNode!=NULL)
  {
  	 if (
		     (0x2==(tmpNode->statusUpdated&0x2))
			 		|| statusHeartBeat>=600    //2016-11-24,Add,每10分钟更新一次
				)
  	 {
  	 	 //控制点号
  	 	 msFrame[tmpTail++] = tmpNode->mp&0xff;
  	 	 msFrame[tmpTail++] = tmpNode->mp>>8&0xff;
  	 	 
  	 	 //日出日落时刻
  	 	 msFrame[tmpTail++] = tmpNode->duty[0];
  	 	 msFrame[tmpTail++] = tmpNode->duty[1];
  	 	 msFrame[tmpTail++] = tmpNode->duty[2];
  	 	 msFrame[tmpTail++] = tmpNode->duty[3];
  	 	 
  	 	 tmpNode->statusUpdated &= 0xfd;
  	 	 
  	 	 tmpCount++;
  	 }
  	 
  	 tmpNode = tmpNode->next;
  }
  //日出日落状态更新的控制点数量
  msFrame[tmpSuccess] = tmpCount&0xff;
  msFrame[tmpSuccess+1] = tmpCount>>8&0xff;

	//集中器遥信,2018-6-15
	//msFrame[tmpTail++] = ioctl(fdOfIoChannel, READ_YX_VALUE, 0);
  //2018-09-14,改成适应动断和动合
	msFrame[tmpTail++] = stOfSwitch;

	//温温度传感,2018-6-15
	msFrame[tmpTail] = 0;
	if (thLink!=NULL)
	{
		msFrame[tmpTail++] = 4;
	 	msFrame[tmpTail++] = thLink->duty[0];
	 	msFrame[tmpTail++] = thLink->duty[1];
	 	msFrame[tmpTail++] = thLink->duty[2];
	 	msFrame[tmpTail++] = thLink->duty[3];
	}
	else
	{
	 	tmpTail++;
	}
  
  //L
  tmpData = ((tmpTail-frameTail02-6)<<2) | PROTOCOL_FIELD;    //2015-10-28,修改这个错误
  msFrame[frameTail02+1] = tmpData&0xff;
  msFrame[frameTail02+2] = tmpData>>8;
  msFrame[frameTail02+3] = tmpData&0xff;
  msFrame[frameTail02+4] = tmpData>>8;

  checkSum = 0;
  for (i=6; i<(tmpTail-frameTail02); i++)
  {
    checkSum += msFrame[frameTail02+i];
  }
  msFrame[tmpTail++] = checkSum;
  
  msFrame[tmpTail++] = 0x16;

  fQueue.frame[fQueue.tailPtr].head = frameTail02;
  fQueue.frame[fQueue.tailPtr].len = tmpTail-frameTail02;
   
  if ((tmpTail+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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

  if (statusHeartBeat>=600)
	{
		statusHeartBeat = 0;
		if (debugInfo&WIRELESS_DEBUG)
		{
			printf(
			 "%02d-%02d-%02d %02d:%02d:%02d,清零更新状态心跳值\n",
				sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second
			);
		}
	}
	
 #else

  msFrame[frameTail02+1] = 0x30 | PROTOCOL_FIELD;
  msFrame[frameTail02+2] = 0;
  msFrame[frameTail02+3] = 0x30 | PROTOCOL_FIELD;
  msFrame[frameTail02+4] = 0;

  checkSum = 0;
  for (i=6;i<18;i++)
  {
    checkSum += msFrame[frameTail02+i];
  }
  msFrame[frameTail02+18] = checkSum;
  
  msFrame[frameTail02+19] = 0x16;

  fQueue.frame[fQueue.tailPtr].head = frameTail02;
  fQueue.frame[fQueue.tailPtr].len = 20;
   
  if ((frameTail02+20+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
	
 #endif

  if (fQueue.sendPtr!=fQueue.tailPtr)
  {
    //启动定时器发送TCP数据
    fQueue.inTimeFrameSend = TRUE;
  }

  //记录日心跳连接次数及发送字节数
  //frameReport(1, 20, 2);
}

#ifdef CQDL_CSM

/*******************************************************
函数名称:AFN02008
功能描述:请求下发注册表信息函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN02008(void)
{
   INT8U  i;
   INT8U  checkSum;
   INT16U tmpData;
   INT16U frameTail02;
   
   if (fQueue.tailPtr == 0)
   {
      frameTail02 = 0;
   }
   else
   {
      frameTail02 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }
  
   msFrame[frameTail02+0] = 0x68;
  
   msFrame[frameTail02+1] = 0x30 | PROTOCOL_FIELD;
   msFrame[frameTail02+2] = 0;
   msFrame[frameTail02+3] = 0x30 | PROTOCOL_FIELD;
   msFrame[frameTail02+4] = 0;
  
   msFrame[frameTail02+5] = 0x68;
  
   msFrame[frameTail02+6] = 0xc9;            //C:11001001
  
   msFrame[frameTail02+7] = addrField.a1[0];
   msFrame[frameTail02+8] = addrField.a1[1];
   msFrame[frameTail02+9] = addrField.a2[0];
   msFrame[frameTail02+10] = addrField.a2[1];
   msFrame[frameTail02+11] = 0x0;              //MSA=0
  
   msFrame[frameTail02+12] = 0x2;              //AFN
   msFrame[frameTail02+13] = 0x70 | pSeq++;    //SEQ
  
   msFrame[frameTail02+14] = 0;     //DA1
   msFrame[frameTail02+15] = 0;     //DA2
  
   msFrame[frameTail02+16] = 0x80;  //DT1
   msFrame[frameTail02+17] = 0;     //DT2
  
   checkSum = 0;
   for (i=6;i<18;i++)
   {
      checkSum += msFrame[frameTail02+i];
   }
   msFrame[frameTail02+18] = checkSum;
  
   msFrame[frameTail02+19] = 0x16;

   fQueue.frame[fQueue.tailPtr].head = frameTail02;
   fQueue.frame[fQueue.tailPtr].len = 20;

   switch (moduleType)
   {
   	 case GPRS_SIM300C:
   	 case GPRS_GR64:
		 case LTE_AIR720H:
     case ETHERNET:
       sendWirelessFrame(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
       break;
       
     case CDMA_DTGS800:
       sendWirelessFrame(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
     	 break;

     case GPRS_MC52I:
     case GPRS_M590E:
     case CDMA_CM180:
       wlModemRequestSend(&msFrame[fQueue.frame[fQueue.tailPtr].head], fQueue.frame[fQueue.tailPtr].len);      
     	 break;
   }
   if ((frameTail02+20+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
   fQueue.sendPtr = fQueue.tailPtr;
   fQueue.thisStartPtr = fQueue.tailPtr;

   //记录日心跳连接次数及发送字节数
   frameReport(1, 20, 2);
}

#endif
