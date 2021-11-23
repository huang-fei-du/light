/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN00.c
作者：leiyong
版本：0.9
完成日期：2006年7月
描述：主站AFN00(确认/否认)处理文件。
函数列表：
     1.
修改历史：
  01,06-07-28,Leiyong created.
  02,09-03-10,Leiyong增加,心跳连接成功次数统计
***************************************************/

#include "teRunPara.h"
#include "msSetPara.h"
#include "wlModem.h"
#include "lcdGui.h"
#include "userInterface.h"
#include "statistics.h"

#include "AFN00.h"

#ifdef LIGHTING
 INT8U downLux[6]={0, 0, 0, 0, 0, 0};    //服务器上传的当前流明值,前3个字节存储上一次分发的值,第3个字节存放上上次分发的值
 INT8U rcvLuxTimes=0;                    //接收到光照度流明值的次数
#endif

/*******************************************************
函数名称:AFN00
功能描述:接收确认/否认(AFN00)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN00(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   INT8U     fn, pn;
   INT16U    tmpData;   

   if (debugInfo&WIRELESS_DEBUG)
   {
     sendDebugFrame((INT8U *)pDataHead,10);
   }
    
   //根据数据单元标识的值,查找FN，Pn值
   pn = findFnPn(*pDataHead, *(pDataHead+1),FIND_PN);
   fn = findFnPn(*(pDataHead+2), *(pDataHead+3),FIND_FN);
   
   if (pn == 0)
   { 
      switch(fn)
      {
     	   case 1:      //全部确认
           if (wlModemFlag.loginSuccess==0)    //登录确认
           {
	          #ifdef LIGHTING
	      
	           //2016-07-04,照明集中器心跳改为以秒为单位
	           nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
	      	
	          #else

        	    nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
        	    
        	  #endif
        	  
        	    lastHeartBeat = sysTime;

              wlModemFlag.loginSuccess    = 1;    //登录成功
              wlModemFlag.lastIpSuccess   = 1;    //上次IP登录置为未成功
              wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add
        	    
              #ifdef PLUG_IN_CARRIER_MODULE
               #ifdef MENU_FOR_CQ_CANON
                switch(moduleType)
                {
                  case GPRS_SIM300C:
                  case GPRS_GR64:
                  case GPRS_MC52I:
                  case GPRS_M590E:
                  case GPRS_M72D:
        	          guiDisplay(6, 1, "G", 1);
                    break;
									
                  case LTE_AIR720H:
        	          guiDisplay(6, 1, "4", 1);
                    break;
              
                  case ETHERNET:
        	          guiDisplay(6, 1, "E", 1);
                    break;
                      
                  case CDMA_DTGS800:
                  case CDMA_CM180:
        	          guiDisplay(6, 1, "C", 1);
                    break;
                }

                lcdRefresh(1,16);
               #else
                 showInfo("登录成功!");
               #endif
              #else    //画G上面的框
                guiLine(20,1,20,15,1);
                guiLine(30,1,30,15,1);
                guiLine(20,1,30,1,1);
                guiLine(20,15,30,15,1);
                lcdRefresh(1,16);
              #endif
             
              //记录日连接成功次数
              frameReport(2, 0, 1);
              
              if (debugInfo&WIRELESS_DEBUG)
              {
              	 printf("全部确认的登录成功\n");
              }
           }
           else              //心跳确认
           {
         	    if (wlModemFlag.heartSuccess==0)
         	    {
    	         #ifdef LIGHTING
    	      
	              //2016-07-04,照明集中器心跳改为以秒为单位
	              nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
    	      	
    	         #else
         	      
         	      nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
    	         
    	         #endif
         	      
         	      lastHeartBeat = sysTime;
                wlModemFlag.heartSuccess = 1;
                
                wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add

               #ifdef PLUG_IN_CARRIER_MODULE
                #ifndef MENU_FOR_CQ_CANON
                 showInfo("主站确认心跳!");
                #endif
               #endif
              
                //记录心跳连接成功次数
             	  frameReport(2, 0, 2);
                
                if (debugInfo&WIRELESS_DEBUG)
                {
              	  printf("全部确认的心跳成功\n");
                }
             	}
             	else   //主动上报帧的确认
             	{
             		//有主动帧要发送时的确认就是可以发一下帧主动上报帧
             		if (fQueue.activeSendPtr!=fQueue.activeTailPtr || fQueue.activeFrameSend==TRUE)
             		{
                  fQueue.activeSendPtr = fQueue.activeFrame[fQueue.activeSendPtr].next;
                  if (debugInfo&PRINT_ACTIVE_REPORT)
                  {
                    printf("主动上报帧发送指针移位后activeSendPtr=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeTailPtr);

             		    printf("主动上报收到确认,可以继续发送\n");
  	 	  	          
  	 	  	          #ifdef RECORD_LOG
  	 	  	           logRun("主动上报收到确认,可以继续发送");
  	 	  	          #endif
             		  }
             		  
             		  fQueue.continueActiveSend = 8;    //主站确认
             		}
             		
             		if (fQueue.active0dDataSending>0)
  	            {
  	            	 fQueue.active0dDataSending = 88;
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
             		     printf("0D主动上报收到确认,可以继续发送\n");
             		   }
  	 	  	         
  	 	  	         #ifdef RECORD_LOG
  	 	  	           logRun("0D主动上报收到确认,可以继续发送");
  	 	  	         #endif
  	            }
                
               #ifdef PLUG_IN_CARRIER_MODULE
                #ifndef MENU_FOR_CQ_CANON
                 showInfo("通信完毕!");
                #endif
               #endif
             	}
           }
            
           if (menuInLayer==0 && setParaWaitTime==0xfe)
           {
              defaultMenu();
           }
     	   	 break;
     	   	 
     	   case 2:      //全部否认
     	   	  break;
     	   	  
     	   case 3:      //对收到报文中的全部数据单元标识进行逐个确认/否认
     	   	  pDataHead += 4;
     	   	  switch(*pDataHead)
     	   	  {
     	   	  	  case 0x2:    //确认或否认AFN02(链路接口检测)
                  pDataHead++;
                  
                  //根据数据单元标识的值,查找FN，Pn值
                  pn = findFnPn(*pDataHead, *(pDataHead+1),FIND_PN);
                  fn = findFnPn(*(pDataHead+2), *(pDataHead+3),FIND_FN);

                  if (fn == 0x1 && pn == 0x0)
                  {
    	               pDataHead += 4;
    	               if (*pDataHead==0x0)
    	               {
            	         #ifdef LIGHTING
            	      
	                      //2016-07-04,照明集中器心跳改为以秒为单位
	                      nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
            	      	
            	         #else
            	         
                  	    nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);

                  	   #endif
                  	   
                  	    lastHeartBeat = sysTime;

    	                  wlModemFlag.loginSuccess    = 1;    //登录成功
                 	  	  wlModemFlag.lastIpSuccess   = 1;    //上次IP登录置为未成功
                        wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add
        	             
                        #ifdef MENU_FOR_CQ_CANON
                         switch(moduleType)
                         {
                            case GPRS_SIM300C:
                            case GPRS_GR64:
                            case GPRS_MC52I:
                            case GPRS_M590E:
                            case GPRS_M72D:
                  	          guiDisplay(6, 1, "G", 1);
                              break;
                        
                            case ETHERNET:
                  	          guiDisplay(6, 1, "E", 1);
                              break;
                                
                            case CDMA_DTGS800:
                            case CDMA_CM180:
                  	          guiDisplay(6, 1, "C", 1);
                              break;
                         }

                         lcdRefresh(1,16);
                        #else
                         #ifdef PLUG_IN_CARRIER_MODULE
                          #ifndef MENU_FOR_CQ_CANON
                           showInfo("登录成功!");
                          #endif
                         #else    //画G上面的框
                          guiLine(20,1,20,15,1);
                          guiLine(30,1,30,15,1);
                          guiLine(20,1,30,1,1);
                          guiLine(20,15,30,15,1);
                          lcdRefresh(1,16);
                         #endif
                        #endif
                        
                        //记录日连接成功次数
                        frameReport(2, 0, 1);
                        
                        if (debugInfo&WIRELESS_DEBUG)
                        {
              	          printf("部分确认否认的登录成功\n");
                        }
                     }
                  }
                  if (fn == 0x2 && pn == 0x0)
                  {
   	                 pDataHead += 4;
   	                 if (*pDataHead==0x0)
   	                 {
                	     wlModemFlag.logoutSuccess = 1;    //退出登录成功
                     }
                  }
                  if (fn == 0x3 && pn == 0x0)
                  {
    	               pDataHead += 4;
    	               if (
    	               	   *pDataHead==0x0 
    	               	  #ifdef LIGHTING 
    	               	   || *pDataHead==0x03
    	               	  #endif
    	               	  )
    	               {
            	         #ifdef LIGHTING
            	      
	                      //2016-07-04,照明集中器心跳改为以秒为单位
            	          nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
            	      	
            	         #else
                  	   
                  	    nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
                  	   
                  	   #endif
                  	   
                  	    lastHeartBeat = sysTime;
                        wlModemFlag.heartSuccess  = 1;
                        wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add

                        //记录心跳连接成功次数
                        frameReport(2, 0, 2);
                        
                         #ifdef PLUG_IN_CARRIER_MODULE
                          #ifndef MENU_FOR_CQ_CANON
                           showInfo("主站确认心跳!");
                          #endif
                         #endif
                        
                        if (debugInfo&WIRELESS_DEBUG)
                        {
                          printf("%02d-%02d-%02d %02d:%02d:%02d,部分确认否认的心跳成功.下次心跳时间:%02d-%02d-%02d %02d:%02d:%02d\n",
                                 sysTime.year,
                                 sysTime.month,
                                 sysTime.day,
                                 sysTime.hour,
                                 sysTime.minute,
                                 sysTime.second,
                                 nextHeartBeatTime.year,
                                 nextHeartBeatTime.month,
                                 nextHeartBeatTime.day,
                                 nextHeartBeatTime.hour,
                                 nextHeartBeatTime.minute,
                                 nextHeartBeatTime.second
                                );
                        }
                        
                       #ifdef LIGHTING
                        //光控处理,2015-12-16
                        if (*pDataHead==0x3)
                        {
                          rcvLuxTimes++;
                          
                          if (debugInfo&METER_DEBUG)
                          {
                            printf("%02d-%02d-%02d %02d:%02d:%02d,服务器分发的当前流明值=%ld,上一次分发值=%ld,上上次分发值=%ld\n",
                                   sysTime.year,
                                   sysTime.month,
                                   sysTime.day,
                                   sysTime.hour,
                                   sysTime.minute,
                                   sysTime.second,
                                   *(pDataHead+1)|*(pDataHead+2)<<8 | *(pDataHead+3)<<16,
                                   downLux[0] | downLux[1]<<8 | downLux[2]<<16,
                                   downLux[3] | downLux[4]<<8 | downLux[5]<<16
                                  );
                          }

                          if (rcvLuxTimes>2)
                          {
                            lcProcess(downLux, *(pDataHead+1)|*(pDataHead+2)<<8 | *(pDataHead+3)<<16);
                          }

                          //将上一次分发的值移动到上上次
                          downLux[3] = downLux[0];
                          downLux[4] = downLux[1];
                          downLux[5] = downLux[2];
                          
                          //当前分发值存储到上一次位置
                          downLux[0] = *(pDataHead+1);
                          downLux[1] = *(pDataHead+2);
                          downLux[2] = *(pDataHead+3);
													
													lcHeartBeat = 0;
                        }
                       #endif
                     }
                  }
            
                  if (menuInLayer==0 && setParaWaitTime==0xfe)
                  {
                    defaultMenu();
                  }                  
     	   	  	  	break;
     	   	  }
     	   	  break;
      }
   }
}

/*******************************************************
函数名称:ackOrNack
功能描述:发送全部确认或否认主站命令函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void ackOrNack(BOOL ack,INT8U dataFrom)
{
  INT16U i;
  INT8U  checkSum;
  INT16U frameTail00,tmpHead00;

  if (fQueue.tailPtr == 0)
  {
     tmpHead00 = 0;
  }
  else
  {
     tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
  }

  msFrame[tmpHead00+0] = 0x68;
  
  msFrame[tmpHead00+5] = 0x68;
  
  msFrame[tmpHead00+6] = 0x80;          //C:10000000
  
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
  {
     msFrame[tmpHead00+6] |= 0x20;
  }
    
  msFrame[tmpHead00+7] = addrField.a1[0];
  msFrame[tmpHead00+8] = addrField.a1[1];
  msFrame[tmpHead00+9] = addrField.a2[0];
  msFrame[tmpHead00+10] = addrField.a2[1];
  msFrame[tmpHead00+11] = addrField.a3;
  
  msFrame[tmpHead00+12] = 0;

  if (frame.pTp!=NULL)
  {
    msFrame[tmpHead00+13] = 0xe0 | rSeq;
  }
  else
  {
    msFrame[tmpHead00+13] = 0x60 | rSeq;
  }
  
  msFrame[tmpHead00+14] = 0;
  msFrame[tmpHead00+15] = 0;
  
  if (ack == TRUE)
  {
    msFrame[tmpHead00+16] = 0x1;  //确认
  }
  else
  {
    msFrame[tmpHead00+16] = 0x2;  //否认
  }
  
  msFrame[tmpHead00+17] = 0;
  
  frameTail00 = tmpHead00 + 18;
  
  //不允许主动上报且有事件发生
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)
  {
    msFrame[frameTail00++] = iEventCounter;
    msFrame[frameTail00++] = nEventCounter;
    
    //根据启动站要求判断是否携带TP
    if (frame.pTp != NULL)
    {
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp;
    }
  }
  else
  {
    //根据启动站要求判断是否携带TP
    if (frame.pTp != NULL)
    {
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp;
    }
  }

  i = ((frameTail00 - tmpHead00 - 6) << 2) | 0x2;
  msFrame[tmpHead00+1] = i & 0xFF;   //L
  msFrame[tmpHead00+2] = i >> 8;
  msFrame[tmpHead00+3] = i & 0xFF;   //L
  msFrame[tmpHead00+4] = i >> 8; 
    
  i = tmpHead00+6;
  checkSum = 0;
  while (i < frameTail00)
  {
     checkSum = checkSum + msFrame[i];
     i++;
  }
  msFrame[frameTail00++] = checkSum;
  
  msFrame[frameTail00++] = 0x16;
  
  fQueue.frame[fQueue.tailPtr].head = tmpHead00;
  fQueue.frame[fQueue.tailPtr].len = frameTail00-tmpHead00;
  
  if ((frameTail00+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
}

/*******************************************************
函数名称:AFN0003
功能描述:对收到报文中的全部数据单元标识进行逐个确认/否认
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN00003(INT8U ackNum, INT8U dataFrom, INT8U afn)
{
   INT16U i, frameTail00;
   INT16U tmpHead00;
   INT8U  *pTpv;                   //TpV指针
   
   if (fQueue.tailPtr == 0)
   {
      tmpHead00 = 0;
   }
   else
   {
      tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }

   msFrame[tmpHead00+ 0] = 0x68;
     
   msFrame[tmpHead00 + 5] = 0x68;

   msFrame[tmpHead00 + 6] = 0x89;                      //C:10001001
   if (frame.acd==1 && (callAndReport&0x03)== 0x02)  //不允许主动上报且有事件发生
   {
      msFrame[tmpHead00 + 6] |= 0x20;
   }   
   msFrame[tmpHead00 + 7] = addrField.a1[0];
   msFrame[tmpHead00 + 8] = addrField.a1[1];
   msFrame[tmpHead00 + 9] = addrField.a2[0];
   msFrame[tmpHead00 + 10] = addrField.a2[1];
   msFrame[tmpHead00 + 11] = addrField.a3;
   
   msFrame[tmpHead00 + 12] = 0;
   msFrame[tmpHead00 + 13] = 0;
   
   if (frame.pTp!=NULL)
   {
   	 msFrame[tmpHead00 + 13] |= 0x80;
   }
     
   msFrame[tmpHead00 + 14] = 0;
   msFrame[tmpHead00 + 15] = 0;
   msFrame[tmpHead00 + 16] = 0x04;
   msFrame[tmpHead00 + 17] = 0x0;
   
   msFrame[tmpHead00 + 18] = afn;
   
   frameTail00 = tmpHead00 + 19;
   
   for (i = 0; i < ackNum; i++)
   {
      msFrame[frameTail00++] = ackData[i*5];
      msFrame[frameTail00++] = ackData[i*5+1];
      msFrame[frameTail00++] = ackData[i*5+2];
      msFrame[frameTail00++] = ackData[i*5+3];
      msFrame[frameTail00++] = ackData[i*5+4];
   }

   if (frame.acd==1 && (callAndReport&0x03)== 0x02)
   {
      msFrame[frameTail00++] = iEventCounter;
      msFrame[frameTail00++] = nEventCounter;
   }
   
   //根据启动站要求判断是否携带TP
   if (frame.pTp != NULL)
   {
      pTpv = frame.pTp;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv;
   }
   
   i = ((frameTail00 - tmpHead00 -6) << 2) | 0x2;
   msFrame[tmpHead00 + 1] = i & 0xFF;   //L
   msFrame[tmpHead00 + 2] = i >> 8;
   msFrame[tmpHead00 + 3] = i & 0xFF;   //L
   msFrame[tmpHead00 + 4] = i >> 8; 
     
   frameTail00++;   
   msFrame[frameTail00++] = 0x16;
 
   fQueue.frame[fQueue.tailPtr].head = tmpHead00;
   fQueue.frame[fQueue.tailPtr].len = frameTail00 - tmpHead00;
   
   if ((frameTail00+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
}

/*******************************************************
函数名称:AFN0004
功能描述:对收到报文中的硬件安全认证错误应答
调用函数:
被调用函数:
输入参数:errorType-错误类型(1-表示签名校验错误,2-表示密文校验错误,3-表示对称MAC验证失败)
         data-签名错误时,数据体返回全FF,表示无效
              密文校验错误时,数据体返回全FF,表示无效
              对称MAC验证失败时:低8字节为当前终端随机数,高8字节模块序列号
输出参数:
返回值：void
*******************************************************/
void AFN00004(INT8U dataFrom, INT8U errorType, INT8U *data)
{
   INT16U i, frameTail00;
   INT16U tmpHead00;
   INT8U  *pTpv;                   //TpV指针
   
   if (fQueue.tailPtr == 0)
   {
      tmpHead00 = 0;
   }
   else
   {
      tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }

   msFrame[tmpHead00+ 0] = 0x68;
     
   msFrame[tmpHead00 + 5] = 0x68;

   msFrame[tmpHead00 + 6] = 0x89;                      //C:10001001
   if (frame.acd==1 && (callAndReport&0x03)== 0x02)  //不允许主动上报且有事件发生
   {
      msFrame[tmpHead00 + 6] |= 0x20;
   }   
   msFrame[tmpHead00 + 7] = addrField.a1[0];
   msFrame[tmpHead00 + 8] = addrField.a1[1];
   msFrame[tmpHead00 + 9] = addrField.a2[0];
   msFrame[tmpHead00 + 10] = addrField.a2[1];
   msFrame[tmpHead00 + 11] = addrField.a3;
   
   msFrame[tmpHead00 + 12] = 0;
   msFrame[tmpHead00 + 13] = 0;
   
   if (frame.pTp!=NULL)
   {
   	 msFrame[tmpHead00 + 13] |= 0x80;
   }
     
   msFrame[tmpHead00 + 14] = 0;
   msFrame[tmpHead00 + 15] = 0;
   msFrame[tmpHead00 + 16] = 0x08;  //FN=4
   msFrame[tmpHead00 + 17] = 0x0;
   
   msFrame[tmpHead00 + 18] = errorType;

   frameTail00 = tmpHead00 + 19;
   if (errorType==3)
   {
   	 memcpy(&msFrame[frameTail00], data, 16);
   }
   else
   {
   	 memset(&msFrame[frameTail00],0xff,16);
   }
   frameTail00+=16;   	 

   if (frame.acd==1 && (callAndReport&0x03)== 0x02)
   {
      msFrame[frameTail00++] = iEventCounter;
      msFrame[frameTail00++] = nEventCounter;
   }
   
   //根据启动站要求判断是否携带TP
   if (frame.pTp != NULL)
   {
      pTpv = frame.pTp;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv;
   }
   
   i = ((frameTail00 - tmpHead00 -6) << 2) | 0x2;
   msFrame[tmpHead00 + 1] = i & 0xFF;   //L
   msFrame[tmpHead00 + 2] = i >> 8;
   msFrame[tmpHead00 + 3] = i & 0xFF;   //L
   msFrame[tmpHead00 + 4] = i >> 8; 
     
   frameTail00++;   
   msFrame[frameTail00++] = 0x16;
 
   fQueue.frame[fQueue.tailPtr].head = tmpHead00;
   fQueue.frame[fQueue.tailPtr].len = frameTail00 - tmpHead00;
   
   if ((frameTail00+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
}
