/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：msOutput.c
作者：leiyong
版本：0.9
完成日期：2006年7月
描述：主站输出帧文件。
函数列表：
     1.
修改历史：
  01,06-7-12,Leiyong created.
  02,07-11-22,Leiyong modify.以太网建立连接时本地端口随机产生,不固定在9902上,解决连续重登录问题.
***************************************************/

#include "stdio.h"
#include "teRunPara.h"
#include "hardwareConfig.h"
#include "statistics.h"
#include "wlModem.h"

#include "msOutput.h"

/*******************************************************
函数名称:initSendQueue
功能描述:初始化发送队列
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void initSendQueue(void)
{
	 INT8U i;
	 
   //初始化发送队列
   fQueue.inTimeFrameSend = FALSE;
   fQueue.thisStartPtr = 0;
   fQueue.tailPtr = 0;
   fQueue.sendPtr = 0;
   fQueue.delay   = 0;
   for(i=0;i<LEN_OF_SEND_QUEUE;i++)
   {
    	fQueue.frame[i].head = 0;
    	fQueue.frame[i].len  = 0;
    	fQueue.frame[i].next = 0xff;
   }
   fQueue.activeFrameSend    = FALSE;
   fQueue.continueActiveSend = 0;
   fQueue.activeThisStartPtr = 0;
   fQueue.activeTailPtr = 0;
   fQueue.activeSendPtr = 0;
   for(i=0;i<LEN_OF_SEND_QUEUE;i++)
   {
    	fQueue.activeFrame[i].head = 0;
    	fQueue.activeFrame[i].len  = 0;
    	fQueue.activeFrame[i].next = 0xff;    	 
   }
}

/*******************************************************
函数名称:replyToMs
功能描述:回复主站(及时帧和主动上报帧)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void replyToMs(void)
{
	#ifdef RECORD_LOG
	 char   logStr[2000];
	 char   sayStr[10];
	 INT16U i;
  #endif

 #ifdef WDOG_USE_X_MEGA
  INT8U buf[1536];
 #endif

  if (fQueue.delay>0)
  {
  	//如果延时时间已到,延时标志归0
  	if (compareTwoTime(fQueue.delayTime, sysTime) || fQueue.continueActiveSend==8)  //超时或收到主动上报确认
  	{
  		 fQueue.delay = 0;
  	  
  	   if (fQueue.activeFrameSend==TRUE)
  	   {
  	     if (fQueue.continueActiveSend<8)
  	     {
  	     	 fQueue.continueActiveSend++;
   	  	   
   	  	   if (fQueue.continueActiveSend>2)
   	  	   {
   	  	     fQueue.activeSendPtr = fQueue.activeFrame[fQueue.activeSendPtr].next;           //记录发送数据字节数
             fQueue.continueActiveSend = 0;
             
  	 	  	   if (debugInfo&PRINT_ACTIVE_REPORT)
  	 	  	   {
  	 	  	     printf("超时且重发次数已到,主动上报指针移位\n");
  	 	  	   }
  	 	  	   
  	 	  	   #ifdef RECORD_LOG
  	 	  	     logRun("超时且重发次数已到,主动上报指针移位");
  	 	  	   #endif
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	    printf("超时重发上一帧\n");
  	 	  	 }
  	 	   }
  	 	   else
  	 	   {
  	 	   	 fQueue.continueActiveSend = 0;
  	 	   }
  	   }
  	   
  	   //0D主动上报线程正在发数据
  	   if (fQueue.active0dDataSending>0)
  	   {
  	   	 fQueue.active0dDataSending++;
  	   	 
  	   	 //ly,2011-11-03,add
  	   	 if (fQueue.active0dDataSending>88)
  	   	 {
  	   	 	 fQueue.active0dDataSending = 0;
  	   	 }
  	   }
  	} 	

  	return;
  }

  if ((fQueue.inTimeFrameSend==TRUE || fQueue.activeFrameSend==TRUE) && (fQueue.active0dDataSending==0))
  {
    //有及时帧需要发送
    if (fQueue.sendPtr!=fQueue.tailPtr || fQueue.inTimeFrameSend==TRUE)
    {
 	     #ifdef PRINT_SEND_FQUEUE
 	       say91x = 0xe3;
 	       sendDebugFrame(&say91x,1);
       #endif
         
       if (bakModuleType==MODEM_PPP)  //2012-11-08,add
       {
         sendWirelessFrame(&msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
       }
       else
       {
         switch(moduleType)
         {
           case GPRS_GR64:
           case GPRS_SIM300C:
           case GPRS_M72D:
					 case LTE_AIR720H:
           case ETHERNET:
             sendWirelessFrame(&msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);           
             break;
     
           case CDMA_DTGS800:
             sendWirelessFrame(&msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
             break;
           
           case GPRS_MC52I:
           case GPRS_M590E:
           case CDMA_CM180:
       	     wlModemRequestSend(&msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);     	     
       	     break;
       	     
           case CASCADE_TE:
            #ifdef WDOG_USE_X_MEGA
             buf[0] = 0x2;
             buf[1] = fQueue.frame[fQueue.sendPtr].len&0xff;
             buf[2] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
             memcpy(&buf[3], &msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
             sendXmegaFrame(CASCADE_DATA, buf, fQueue.frame[fQueue.sendPtr].len+3);
            #endif
           	 break;
  
         }
       }

	 	   //2013-11-20,根据山东电力公司的要求,如果将RS232口设置为监视模式时,向主站发送的数据帧同时向维护串口发一份
	 	   if (0x55==mainTainPortMode)
	 	   {
	 	    #ifdef WDOG_USE_X_MEGA
         buf[0] = fQueue.frame[fQueue.sendPtr].len&0xff;
         buf[1] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
         memcpy(&buf[2], &msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
         sendXmegaFrame(MAINTAIN_DATA, buf, fQueue.frame[fQueue.sendPtr].len+2);
         
         if(0x55==rs485Port2Fun)
         {
           buf[0] = 0x2;
           buf[1] = fQueue.frame[fQueue.sendPtr].len&0xff;
           buf[2] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
           memcpy(&buf[3], &msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
           sendXmegaFrame(COPY_DATA, buf, fQueue.frame[fQueue.sendPtr].len+3);
         }
        #endif
       }

       if (debugInfo&PRINT_ACTIVE_REPORT)
       {
         printf("及时帧:发送序号=%d,字节=%d,tailPtr=%d\n",fQueue.sendPtr,fQueue.frame[fQueue.sendPtr].len,fQueue.tailPtr);
       }

       #ifdef PRINT_SEND_FQUEUE
         sendDebugFrame(&fQueue.sendPtr,1);
       #endif         
 
       //发送数据及帧数记录
       frameReport(1, fQueue.frame[fQueue.sendPtr].len, 0);
       
 	  	 //帧之间的延迟处理
  	 	 fQueue.delay = 1;
  	 	 fQueue.delayTime = nextTime(sysTime, 0, 5);   //延时5秒后再发下一帧
 	  	  
 	  	 //及时帧发送指针移位
   	   fQueue.sendPtr = fQueue.frame[fQueue.sendPtr].next;
 
   	   if (fQueue.sendPtr==fQueue.tailPtr)
   	   {
   	     fQueue.inTimeFrameSend = FALSE;
  	 	   fQueue.delayTime = nextTime(sysTime, 0, 3); //延时1秒后再发下一帧
   	    	 
   	     if (fQueue.activeSendPtr==fQueue.activeTailPtr)
   	     {
   	    	 goto stopBreak;
   	     }
   	   }
    }
    else
    {
   	 	//有主动上报帧需要发送
   	 	if (fQueue.activeSendPtr!=fQueue.activeTailPtr || fQueue.activeFrameSend==TRUE)
   	 	{
         if (fQueue.activeSendPtr==fQueue.activeTailPtr)
         {
           fQueue.activeFrameSend = FALSE;

           if (fQueue.sendPtr==fQueue.tailPtr)
   	       {
   	         goto stopBreak;
   	       }
   	     }

         #ifdef PRINT_SEND_FQUEUE
          say91x = 0xe4;
          sendDebugFrame(&say91x,1);
         #endif
 
         //主动上报帧需要IP允许发送时才进行
         if (wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
         {
           if (bakModuleType==MODEM_PPP)  //2012-11-08,add
           {
             sendWirelessFrame(&activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);

             if (debugInfo&PRINT_ACTIVE_REPORT)
             {
             	 printf("发送序号=%d,字节=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
             }
           }
           else
           {
             switch(moduleType)
             {
               case GPRS_GR64:
               case GPRS_SIM300C:
               case GPRS_M72D:
							 case LTE_AIR720H:
               case ETHERNET:
                 sendWirelessFrame(&activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);
  
                 if (debugInfo&PRINT_ACTIVE_REPORT)
                 {
               	   printf("发送序号=%d,字节=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
               	 }
               	 
                 #ifdef RECORD_LOG
                  strcpy(logStr,"");
                  sprintf(logStr,"主动上报发送序号=%d,字节=%d,activeTailPtr=%d:",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
                  for(i=0;i<fQueue.activeFrame[fQueue.activeSendPtr].len;i++)
                  {
                  	 strcpy(sayStr,"");
                  	 sprintf(sayStr,"%02x ",activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+i]);
                  	 strcat(logStr,sayStr);
                  }
                  logRun(logStr);                
                 #endif
  
                 break;
         
               case CDMA_DTGS800:
                 sendWirelessFrame(&activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);
   
                 #ifdef CDMA_HEART_BEAT
                  nextCdmaHeartTime = nextTime(sysTime,0,30);
                 #endif
                 break;
                 
               case GPRS_MC52I:
               case GPRS_M590E:
               case CDMA_CM180:
       	         wlModemRequestSend(&activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);
  
                 if (debugInfo&PRINT_ACTIVE_REPORT)
                 {
               	   printf("发送序号=%d,字节=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
               	 }
               	 
                 #ifdef RECORD_LOG
                  strcpy(logStr,"");
                  sprintf(logStr,"主动上报发送序号=%d,字节=%d,activeTailPtr=%d:",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
                  for(i=0;i<fQueue.activeFrame[fQueue.activeSendPtr].len;i++)
                  {
                  	 strcpy(sayStr,"");
                  	 sprintf(sayStr,"%02x ",activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+i]);
                  	 strcat(logStr,sayStr);
                  }
                  logRun(logStr);
                 #endif
       	         break;
       	         
               case CASCADE_TE:
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x2;
                 buf[1] = fQueue.activeFrame[fQueue.activeSendPtr].len&0xff;
                 buf[2] = fQueue.activeFrame[fQueue.activeSendPtr].len>>8&0xff;
                 memcpy(&buf[3], &activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);
                 sendXmegaFrame(CASCADE_DATA, buf, fQueue.activeFrame[fQueue.activeSendPtr].len+3);
                #endif
               	 break;
  
             }
           }
	 	       
	 	       //2013-11-20,根据山东电力公司的要求,如果将RS232口设置为监视模式时,向主站发送的数据帧同时向维护串口发一份
	 	       if (0x55==mainTainPortMode)
	 	       {
	 	        #ifdef WDOG_USE_X_MEGA
             buf[0] = fQueue.activeFrame[fQueue.activeSendPtr].len&0xff;
             buf[1] = fQueue.activeFrame[fQueue.activeSendPtr].len>>8&0xff;
             memcpy(&buf[2], &activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);
             sendXmegaFrame(MAINTAIN_DATA, buf, fQueue.activeFrame[fQueue.activeSendPtr].len+2);
             
             if(0x55==rs485Port2Fun)
             {
               buf[0] = 0x2;
               buf[1] = fQueue.activeFrame[fQueue.activeSendPtr].len&0xff;
               buf[2] = fQueue.activeFrame[fQueue.activeSendPtr].len>>8&0xff;
               memcpy(&buf[3], &activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);
               sendXmegaFrame(COPY_DATA, buf, fQueue.activeFrame[fQueue.activeSendPtr].len+3);
             }
            #endif
           }

  	       #ifdef PRINT_SEND_FQUEUE
  	         sendDebugFrame(&fQueue.activeSendPtr,1);
  	       #endif

  	 	  	 fQueue.delayTime = nextTime(sysTime, 0, 10);   //延时10秒后再发下一帧,在主动上报数据时是超时重发定时
  	 	  	 
  	 	  	 if (debugInfo&PRINT_ACTIVE_REPORT)
  	 	  	 {
             printf("本次发送时间:%02d-%02d-%02d %02d:%02d:%02d\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
             printf("下次发送时间fQueue.delayTime:%02d-%02d-%02d %02d:%02d:%02d\n", fQueue.delayTime.year,fQueue.delayTime.month,fQueue.delayTime.day,fQueue.delayTime.hour,fQueue.delayTime.minute,fQueue.delayTime.second);
           }

  	 	  	 fQueue.delay = 1;

  	 	  	 if (((activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+13]&0x10)!=0) && (activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+12]==0x0c || activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+12]==0x0d))
  	 	  	 {
  	 	  	   if ((fQueue.continueActiveSend<1) || (fQueue.continueActiveSend>7))
  	 	  	   {
  	 	  	     fQueue.continueActiveSend = 1;             //等待确认
  	 	  	   }
  	 	  	   
  	 	  	   if (debugInfo&PRINT_ACTIVE_REPORT)
  	 	  	   {
  	 	  	     printf("发送主动上报后等待主站确认\n");
  	 	  	   }

  	 	  	   #ifdef RECORD_LOG
  	 	  	     logRun("发送主动上报后等待主站确认");
  	 	  	   #endif
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	   //主动上报帧发送指针移位
   	  	     fQueue.activeSendPtr = fQueue.activeFrame[fQueue.activeSendPtr].next;           //记录发送数据字节数
  	 	  	   printf("指针移位\n");
  	 	  	   
  	 	  	   #ifdef RECORD_LOG
  	 	  	     logRun("指针移位");
  	 	  	   #endif
             
             if (fQueue.activeSendPtr==fQueue.activeTailPtr)
             {
               fQueue.activeFrameSend = FALSE;

               if (fQueue.sendPtr==fQueue.tailPtr)
   	           {
   	             goto stopBreak;
   	           }
   	         }
   	  	   }
   	  	   
           frameReport(1, fQueue.activeFrame[fQueue.activeSendPtr].len, 0);
   	  	 }   	    
   	 	}
   	 	else   //清除发送标志
   	 	{
 stopBreak:
 	 	    #ifdef PRINT_SEND_FQUEUE
 	 	     say91x = 0xe8;
 	 	     sendDebugFrame(&say91x,1);
 	 	    #endif
 	
  	  	 fQueue.delay = 0;
  	  	 if (fQueue.continueActiveSend>0)
  	  	 {
  	  	 	 fQueue.continueActiveSend = 0;
  	  	 }
  	  	 
  	  	 if (debugInfo&PRINT_ACTIVE_REPORT)
  	  	 {
  	  	   printf("发送完毕\n");
  	 	  	
  	 	  	 #ifdef RECORD_LOG
  	 	  	  logRun("发送完毕");
  	 	  	 #endif
  	  	 }
 
        #ifdef PLUG_IN_CARRIER_MODULE
         #ifndef MENU_FOR_CQ_CANON
          showInfo("通信完毕!");
         #endif
        #endif
      }
    }
  }
}

#ifdef WDOG_USE_X_MEGA

/*******************************************************
函数名称:sendToXmega
功能描述:发送数据给xMega
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void sendToXmega(void)
{
   struct timeval tv;

   if (xMegaQueue.delay>0)
   {
     xMegaQueue.delay++;
     if (xMegaQueue.delay>XMEGA_SEND_DELAY)
     {
     	 xMegaQueue.delay = 0;
     }
     
  	 return;
   }

   //有帧需要发送
   if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr || xMegaQueue.inTimeFrameSend==TRUE)
   {
     //发送
     write(fdOfLocal, &xMegaFrame[xMegaQueue.frame[xMegaQueue.sendPtr].head], xMegaQueue.frame[xMegaQueue.sendPtr].len);
     
     xMegaHeartBeat = 0;

     if (debugInfo&PRINT_XMEGA_DEBUG)
     {
       gettimeofday(&tv, NULL);
       printf("%d-%d-%d %d:%d:%d 秒=%d,微秒=%d xMega Data Tx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,tv.tv_sec, tv.tv_usec);
       sendDebugFrame(&xMegaFrame[xMegaQueue.frame[xMegaQueue.sendPtr].head], xMegaQueue.frame[xMegaQueue.sendPtr].len);
     }
 
	   //帧之间的延迟处理
     if (xMegaFrame[xMegaQueue.frame[xMegaQueue.sendPtr].head+5]==COPY_PORT_RATE_SET)
     {
     	 //设置速率后20ms后开始发送抄表数据,ly,2011-08-22
     	 xMegaQueue.delay = XMEGA_SEND_DELAY - 20;
     }
     else
     {
 	 	   xMegaQueue.delay = 1;
 	 	 }
	  	  
	   //及时帧发送指针移位
  	 xMegaQueue.sendPtr = xMegaQueue.frame[xMegaQueue.sendPtr].next;

  	 if (xMegaQueue.sendPtr==xMegaQueue.tailPtr)
  	 {
  	   xMegaQueue.inTimeFrameSend = FALSE;
  	 }
   }
}
#endif

#ifdef USE_ON_BOARD_ETH

/*******************************************************
函数名称:tcpConnected
功能描述:发起TCP连接(connect)时的连接函数[本例程什么也不做]
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
err_t  tcpConnected(void *arg, struct tcp_pcb *tpcb, err_t err)
{	 
	 return 0;  //nothing
}

/*******************************************************
函数名称:startUartTcp
功能描述:开始串行网络接口TCP发送过程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void startUartTcp(INT8U *pFrame,INT16U len)
{
	  struct ip_addr ipAddr;
	  INT16U         tmpSelfPort;        //本地端口
    
    tmpSelfPort = (sysTime.minute%60)<<8 | (sysTime.second%60);
    
    if (tmpSelfPort<1 || tmpSelfPort>65000)
    {
    	tmpSelfPort = 9002;
    }
    //netif_set_default(&uartIf);				     // 设为默认网络界面
    
	  pUartTcpPcb = tcp_new();               //申请一个新的TCP PCB
	  
	  //绑定本端IP和端口到PCB
	  tcp_bind(pUartTcpPcb, &localIpAddr, tmpSelfPort);
	  
	  //指定TCP应用接收处理函数
    tcp_recv(pUartTcpPcb,tcpRecvProcess);

	  //发起远端TCP连接
    ipAddr.addr = ipAndPort.ipAddr[0] | ipAndPort.ipAddr[1]<<8
                | ipAndPort.ipAddr[2]<<16 | ipAndPort.ipAddr[3]<<24;
        
    tcp_connect(pUartTcpPcb,&ipAddr,ipAndPort.port[0] | ipAndPort.port[1]<<8,tcpConnected);
  
	  //向TCP PCB向数据
	  tcp_write(pUartTcpPcb,(void *)pFrame,len,1);
}

/*******************************************************
函数名称:startEthTcp
功能描述:开始Ethernet TCP发送过程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void startEthTcp(INT8U *pFrame,INT16U len)
{
	  struct ip_addr ipAddr;
	  INT16U         tmpSelfPort;        //本地端口
    
    tmpSelfPort = (sysTime.minute%60)<<8 | (sysTime.second%60);
    
    if (tmpSelfPort<1 || tmpSelfPort>65000)
    	tmpSelfPort = 9002;
    
	  pEthTcpPcb = tcp_new();               //申请一个新的TCP PCB
	  
	  //绑定本端IP和端口到PCB
	  tcp_bind(pEthTcpPcb, &localEthIpAddr, tmpSelfPort);
	  
	  //指定TCP应用接收处理函数
    tcp_recv(pEthTcpPcb,tcpRecvProcess);

	  //发起远端TCP连接
    ipAddr.addr = ipAndPort.ipAddr[0] | ipAndPort.ipAddr[1]<<8
                | ipAndPort.ipAddr[2]<<16 | ipAndPort.ipAddr[3]<<24;
    tcp_connect(pEthTcpPcb,&ipAddr,ipAndPort.port[0] | ipAndPort.port[1]<<8,tcpConnected);
  
	  //向TCP PCB向数据
	  tcp_write(pEthTcpPcb,(void *)pFrame,len,1);
}
#endif

/*******************************************************
函数名称:sendToMs
功能描述:发送给主站
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void sendToMs(INT8U *pack,INT16U length)
{
 #ifdef WDOG_USE_X_MEGA
  INT8U buf[1536];
 #endif
   
  if (bakModuleType==MODEM_PPP)    //12-11-08,add
  {
    sendWirelessFrame(pack, length);
    frameReport(1, length, 0);
  }
  else
  {
    switch (moduleType)
    {
     	 case GPRS_SIM300C:
     	 case GPRS_GR64:
     	 case ETHERNET:
     	 case GPRS_M72D:
			 case LTE_AIR720H:
         sendWirelessFrame(pack, length);
         frameReport(1, length, 0);
         break;
         
       case CDMA_DTGS800:
         sendWirelessFrame(pack, length);
         frameReport(1, length, 0);
       	 break;
       	 
       case GPRS_MC52I:
     	 case GPRS_M590E:
       case CDMA_CM180:
       	 wlModemRequestSend(pack, length);
         frameReport(1, length, 0);
       	 break;
       
       case CASCADE_TE:
        #ifdef WDOG_USE_X_MEGA
         buf[0] = 0x2;
         buf[1] = length&0xff;
         buf[2] = length>>8&0xff;
         memcpy(&buf[3], pack, length);
         sendXmegaFrame(CASCADE_DATA, buf, length+3);
        #endif
       	 break;
    }
  }
  
  //2013-11-20,根据山东电力公司的要求,如果将RS232口设置为监视模式时,向主站发送的数据帧同时向维护串口发一份
  if (0x55==mainTainPortMode)
  {
   #ifdef WDOG_USE_X_MEGA
    buf[0] = length&0xff;
    buf[1] = length>>8&0xff;
    memcpy(&buf[2], pack, length);
    sendXmegaFrame(MAINTAIN_DATA, buf, length+2);
    
    if(0x55==rs485Port2Fun)
    {
      buf[0] = 0x2;
      buf[1] = length&0xff;
      buf[2] = length>>8&0xff;
      memcpy(&buf[3], pack, length);
      
      sendXmegaFrame(COPY_DATA, buf, length+3);
    }
    
   #endif
  }
}
