/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����msOutput.c
���ߣ�leiyong
�汾��0.9
������ڣ�2006��7��
��������վ���֡�ļ���
�����б�
     1.
�޸���ʷ��
  01,06-7-12,Leiyong created.
  02,07-11-22,Leiyong modify.��̫����������ʱ���ض˿��������,���̶���9902��,��������ص�¼����.
***************************************************/

#include "stdio.h"
#include "teRunPara.h"
#include "hardwareConfig.h"
#include "statistics.h"
#include "wlModem.h"

#include "msOutput.h"

/*******************************************************
��������:initSendQueue
��������:��ʼ�����Ͷ���
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void initSendQueue(void)
{
	 INT8U i;
	 
   //��ʼ�����Ͷ���
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
��������:replyToMs
��������:�ظ���վ(��ʱ֡�������ϱ�֡)
���ú���:
�����ú���:
�������:
�������:
����ֵ:
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
  	//�����ʱʱ���ѵ�,��ʱ��־��0
  	if (compareTwoTime(fQueue.delayTime, sysTime) || fQueue.continueActiveSend==8)  //��ʱ���յ������ϱ�ȷ��
  	{
  		 fQueue.delay = 0;
  	  
  	   if (fQueue.activeFrameSend==TRUE)
  	   {
  	     if (fQueue.continueActiveSend<8)
  	     {
  	     	 fQueue.continueActiveSend++;
   	  	   
   	  	   if (fQueue.continueActiveSend>2)
   	  	   {
   	  	     fQueue.activeSendPtr = fQueue.activeFrame[fQueue.activeSendPtr].next;           //��¼���������ֽ���
             fQueue.continueActiveSend = 0;
             
  	 	  	   if (debugInfo&PRINT_ACTIVE_REPORT)
  	 	  	   {
  	 	  	     printf("��ʱ���ط������ѵ�,�����ϱ�ָ����λ\n");
  	 	  	   }
  	 	  	   
  	 	  	   #ifdef RECORD_LOG
  	 	  	     logRun("��ʱ���ط������ѵ�,�����ϱ�ָ����λ");
  	 	  	   #endif
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	    printf("��ʱ�ط���һ֡\n");
  	 	  	 }
  	 	   }
  	 	   else
  	 	   {
  	 	   	 fQueue.continueActiveSend = 0;
  	 	   }
  	   }
  	   
  	   //0D�����ϱ��߳����ڷ�����
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
    //�м�ʱ֡��Ҫ����
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

	 	   //2013-11-20,����ɽ��������˾��Ҫ��,�����RS232������Ϊ����ģʽʱ,����վ���͵�����֡ͬʱ��ά�����ڷ�һ��
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
         printf("��ʱ֡:�������=%d,�ֽ�=%d,tailPtr=%d\n",fQueue.sendPtr,fQueue.frame[fQueue.sendPtr].len,fQueue.tailPtr);
       }

       #ifdef PRINT_SEND_FQUEUE
         sendDebugFrame(&fQueue.sendPtr,1);
       #endif         
 
       //�������ݼ�֡����¼
       frameReport(1, fQueue.frame[fQueue.sendPtr].len, 0);
       
 	  	 //֮֡����ӳٴ���
  	 	 fQueue.delay = 1;
  	 	 fQueue.delayTime = nextTime(sysTime, 0, 5);   //��ʱ5����ٷ���һ֡
 	  	  
 	  	 //��ʱ֡����ָ����λ
   	   fQueue.sendPtr = fQueue.frame[fQueue.sendPtr].next;
 
   	   if (fQueue.sendPtr==fQueue.tailPtr)
   	   {
   	     fQueue.inTimeFrameSend = FALSE;
  	 	   fQueue.delayTime = nextTime(sysTime, 0, 3); //��ʱ1����ٷ���һ֡
   	    	 
   	     if (fQueue.activeSendPtr==fQueue.activeTailPtr)
   	     {
   	    	 goto stopBreak;
   	     }
   	   }
    }
    else
    {
   	 	//�������ϱ�֡��Ҫ����
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
 
         //�����ϱ�֡��ҪIP������ʱ�Ž���
         if (wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
         {
           if (bakModuleType==MODEM_PPP)  //2012-11-08,add
           {
             sendWirelessFrame(&activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head], fQueue.activeFrame[fQueue.activeSendPtr].len);

             if (debugInfo&PRINT_ACTIVE_REPORT)
             {
             	 printf("�������=%d,�ֽ�=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
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
               	   printf("�������=%d,�ֽ�=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
               	 }
               	 
                 #ifdef RECORD_LOG
                  strcpy(logStr,"");
                  sprintf(logStr,"�����ϱ��������=%d,�ֽ�=%d,activeTailPtr=%d:",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
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
               	   printf("�������=%d,�ֽ�=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
               	 }
               	 
                 #ifdef RECORD_LOG
                  strcpy(logStr,"");
                  sprintf(logStr,"�����ϱ��������=%d,�ֽ�=%d,activeTailPtr=%d:",fQueue.activeSendPtr,fQueue.activeFrame[fQueue.activeSendPtr].len,fQueue.activeTailPtr);
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
	 	       
	 	       //2013-11-20,����ɽ��������˾��Ҫ��,�����RS232������Ϊ����ģʽʱ,����վ���͵�����֡ͬʱ��ά�����ڷ�һ��
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

  	 	  	 fQueue.delayTime = nextTime(sysTime, 0, 10);   //��ʱ10����ٷ���һ֡,�������ϱ�����ʱ�ǳ�ʱ�ط���ʱ
  	 	  	 
  	 	  	 if (debugInfo&PRINT_ACTIVE_REPORT)
  	 	  	 {
             printf("���η���ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
             printf("�´η���ʱ��fQueue.delayTime:%02d-%02d-%02d %02d:%02d:%02d\n", fQueue.delayTime.year,fQueue.delayTime.month,fQueue.delayTime.day,fQueue.delayTime.hour,fQueue.delayTime.minute,fQueue.delayTime.second);
           }

  	 	  	 fQueue.delay = 1;

  	 	  	 if (((activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+13]&0x10)!=0) && (activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+12]==0x0c || activeFrame[fQueue.activeFrame[fQueue.activeSendPtr].head+12]==0x0d))
  	 	  	 {
  	 	  	   if ((fQueue.continueActiveSend<1) || (fQueue.continueActiveSend>7))
  	 	  	   {
  	 	  	     fQueue.continueActiveSend = 1;             //�ȴ�ȷ��
  	 	  	   }
  	 	  	   
  	 	  	   if (debugInfo&PRINT_ACTIVE_REPORT)
  	 	  	   {
  	 	  	     printf("���������ϱ���ȴ���վȷ��\n");
  	 	  	   }

  	 	  	   #ifdef RECORD_LOG
  	 	  	     logRun("���������ϱ���ȴ���վȷ��");
  	 	  	   #endif
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	   //�����ϱ�֡����ָ����λ
   	  	     fQueue.activeSendPtr = fQueue.activeFrame[fQueue.activeSendPtr].next;           //��¼���������ֽ���
  	 	  	   printf("ָ����λ\n");
  	 	  	   
  	 	  	   #ifdef RECORD_LOG
  	 	  	     logRun("ָ����λ");
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
   	 	else   //������ͱ�־
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
  	  	   printf("�������\n");
  	 	  	
  	 	  	 #ifdef RECORD_LOG
  	 	  	  logRun("�������");
  	 	  	 #endif
  	  	 }
 
        #ifdef PLUG_IN_CARRIER_MODULE
         #ifndef MENU_FOR_CQ_CANON
          showInfo("ͨ�����!");
         #endif
        #endif
      }
    }
  }
}

#ifdef WDOG_USE_X_MEGA

/*******************************************************
��������:sendToXmega
��������:�������ݸ�xMega
���ú���:
�����ú���:
�������:
�������:
����ֵ:
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

   //��֡��Ҫ����
   if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr || xMegaQueue.inTimeFrameSend==TRUE)
   {
     //����
     write(fdOfLocal, &xMegaFrame[xMegaQueue.frame[xMegaQueue.sendPtr].head], xMegaQueue.frame[xMegaQueue.sendPtr].len);
     
     xMegaHeartBeat = 0;

     if (debugInfo&PRINT_XMEGA_DEBUG)
     {
       gettimeofday(&tv, NULL);
       printf("%d-%d-%d %d:%d:%d ��=%d,΢��=%d xMega Data Tx:",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,tv.tv_sec, tv.tv_usec);
       sendDebugFrame(&xMegaFrame[xMegaQueue.frame[xMegaQueue.sendPtr].head], xMegaQueue.frame[xMegaQueue.sendPtr].len);
     }
 
	   //֮֡����ӳٴ���
     if (xMegaFrame[xMegaQueue.frame[xMegaQueue.sendPtr].head+5]==COPY_PORT_RATE_SET)
     {
     	 //�������ʺ�20ms��ʼ���ͳ�������,ly,2011-08-22
     	 xMegaQueue.delay = XMEGA_SEND_DELAY - 20;
     }
     else
     {
 	 	   xMegaQueue.delay = 1;
 	 	 }
	  	  
	   //��ʱ֡����ָ����λ
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
��������:tcpConnected
��������:����TCP����(connect)ʱ�����Ӻ���[������ʲôҲ����]
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
err_t  tcpConnected(void *arg, struct tcp_pcb *tpcb, err_t err)
{	 
	 return 0;  //nothing
}

/*******************************************************
��������:startUartTcp
��������:��ʼ��������ӿ�TCP���͹���
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void startUartTcp(INT8U *pFrame,INT16U len)
{
	  struct ip_addr ipAddr;
	  INT16U         tmpSelfPort;        //���ض˿�
    
    tmpSelfPort = (sysTime.minute%60)<<8 | (sysTime.second%60);
    
    if (tmpSelfPort<1 || tmpSelfPort>65000)
    {
    	tmpSelfPort = 9002;
    }
    //netif_set_default(&uartIf);				     // ��ΪĬ���������
    
	  pUartTcpPcb = tcp_new();               //����һ���µ�TCP PCB
	  
	  //�󶨱���IP�Ͷ˿ڵ�PCB
	  tcp_bind(pUartTcpPcb, &localIpAddr, tmpSelfPort);
	  
	  //ָ��TCPӦ�ý��մ�����
    tcp_recv(pUartTcpPcb,tcpRecvProcess);

	  //����Զ��TCP����
    ipAddr.addr = ipAndPort.ipAddr[0] | ipAndPort.ipAddr[1]<<8
                | ipAndPort.ipAddr[2]<<16 | ipAndPort.ipAddr[3]<<24;
        
    tcp_connect(pUartTcpPcb,&ipAddr,ipAndPort.port[0] | ipAndPort.port[1]<<8,tcpConnected);
  
	  //��TCP PCB������
	  tcp_write(pUartTcpPcb,(void *)pFrame,len,1);
}

/*******************************************************
��������:startEthTcp
��������:��ʼEthernet TCP���͹���
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void startEthTcp(INT8U *pFrame,INT16U len)
{
	  struct ip_addr ipAddr;
	  INT16U         tmpSelfPort;        //���ض˿�
    
    tmpSelfPort = (sysTime.minute%60)<<8 | (sysTime.second%60);
    
    if (tmpSelfPort<1 || tmpSelfPort>65000)
    	tmpSelfPort = 9002;
    
	  pEthTcpPcb = tcp_new();               //����һ���µ�TCP PCB
	  
	  //�󶨱���IP�Ͷ˿ڵ�PCB
	  tcp_bind(pEthTcpPcb, &localEthIpAddr, tmpSelfPort);
	  
	  //ָ��TCPӦ�ý��մ�����
    tcp_recv(pEthTcpPcb,tcpRecvProcess);

	  //����Զ��TCP����
    ipAddr.addr = ipAndPort.ipAddr[0] | ipAndPort.ipAddr[1]<<8
                | ipAndPort.ipAddr[2]<<16 | ipAndPort.ipAddr[3]<<24;
    tcp_connect(pEthTcpPcb,&ipAddr,ipAndPort.port[0] | ipAndPort.port[1]<<8,tcpConnected);
  
	  //��TCP PCB������
	  tcp_write(pEthTcpPcb,(void *)pFrame,len,1);
}
#endif

/*******************************************************
��������:sendToMs
��������:���͸���վ
���ú���:
�����ú���:
�������:
�������:
����ֵ:
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
  
  //2013-11-20,����ɽ��������˾��Ҫ��,�����RS232������Ϊ����ģʽʱ,����վ���͵�����֡ͬʱ��ά�����ڷ�һ��
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
