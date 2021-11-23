/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ�����wlModem.c
���ߣ�leiyong
�汾��0.9
������ڣ�2010��1��
����������Modem��ش����ļ���
�����б�
     1.
�޸���ʷ��
  01,10-01-24,Leiyong created.
  02,10-02-10,Leiyong,�޸�,Ϊ�˱������¼������ɵĸ�λ�ն������ز���������,
              ���Ĵ���ʽΪֻ�д�������Modemͨ�Ų�����ʱ�Ÿ�λ�ն�,����λModemģ��
  03,12-08-01,M590E��ӿ��ػ����̴���            
***************************************************/
#include <unistd.h>

//2012-11-8,add------------
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>    
//2012-11-8,add------------

#include "wirelessModem.h"

#include "ioChannel.h"
#include "hardwareConfig.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "msOutput.h"
#include "msInput.h"
#include "userInterface.h"

#include "wlModem.h"

//����
WL_MODEM_FLAG wlModemFlag;       //����Modem���Ʊ�־
DATE_TIME     lastHeartBeat;     //�ϴ�����ʱ��
DATE_TIME     nextHeartBeatTime; //��һ������ʱ��
DATE_TIME     waitTimeOut;       //�ȴ���ʱʱ��(����¼֡������֡���ͺ�ĳ�ʱʱ��,�����ʱҪ����͵Ķ���)
DATE_TIME     wlPowerTime;       //����Modem����ʱ��
INT32U        wlLocalIpAddr;     //����Modem���ն�IP��ַ(GPRS/CDMA�Ȼ�õ�IP��ַ)

char          tmpWlStr[200];

INT8U         wlRecvBuf[2048];   //����MODEM����
INT8U         bakWlBuf[2048];
INT16U        lenOfWlRecv = 0;   //MODEM���ճ���

INT16U        bakLenOfWlRecv = 0;//����modem���ճ���
INT8U         countOfRecv;       //�鿴modem����

INT8U         operateModem=1;    //���Բ���modem? 2012-11-7
INT8U         dialerOk=0;        //dialer����Ok?  2012-11-7

void wlReceive(INT8U *buf, INT16U len);

/*******************************************************
��������:wlModem
��������:ά��������·
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void wlModem(void)
{
  INT8U         wlRet;
  pthread_t     id;
  int           ret;
  INT8U         threadPara;
  INT8U         i;
  INT8U         tmpBuf[40];
   
  //2012-11-08,���modem����ppp��������Ҫ�ȵ�dialer����ʱ�Ų���ģ��
  if (operateModem==0 && bakModuleType==MODEM_PPP)
  {
  	if (debugInfo&WIRELESS_DEBUG)
  	{
  	  printf("MODEM_PPP�˳�\n");
  	}
  	 
  	return;
  }
   
  if (bakLenOfWlRecv!=lenOfWlRecv)
  {
   	bakLenOfWlRecv = lenOfWlRecv;
   	countOfRecv = 0;
  }
  else
  {
   	countOfRecv++;
   	if (countOfRecv>5)
   	{
 	   	countOfRecv = 0;
 	   
 	   	memcpy(bakWlBuf, wlRecvBuf, lenOfWlRecv);
 	   	lenOfWlRecv = 0;
 	   
 	   	if (bakLenOfWlRecv!=0)
 	   	{
 	    	 wlReceive(bakWlBuf, bakLenOfWlRecv);
 	   
 	     	//if (ipPermit==FALSE)
 	     	//{
         	//  wlFlagsSet.atFrame = FRAME_NONE;
         	//  tailOfWireFrame = 0;
 	 	 		//}
 	   	}
   	}
  }

 	if (moduleType==GPRS_SIM300C 
 	   || moduleType==CDMA_DTGS800 
 	    || moduleType==GPRS_GR64 
 	     || moduleType==GPRS_MC52I 
 	      || moduleType==ETHERNET 
 	       || moduleType==GPRS_M590E 
 	        || moduleType==CDMA_CM180   	         
 	         || moduleType==CASCADE_TE   //�������ն�
 	          || moduleType==GPRS_M72D
 	           || moduleType==LTE_AIR720H
 	            || bakModuleType==MODEM_PPP
 	  )
  {
   	;
  }
  else
  {
   	//���û�м�⵽ģ����ǲ�ʶ���ģ��
    if (compareTwoTime(wlModemFlag.waitModemRet, sysTime))
    {
      //�������ն�
      if (checkIfCascade()==2)
      {
        wlModemFlag.waitModemRet = nextTime(sysTime, WAIT_MODEM_MINUTES, 0);
      }
      else
      {
   	  	if (debugInfo&WIRELESS_DEBUG)
  	  	{
  	 			printf("δʶ��ģ��,���¿�ʼ\n");
  	  	}
  	 
  	  	logRun("δʶ��ģ��,���¿�ʼ");
        	
        setModemSoPara();                     //����Modem���ʼ����
  
        wlModemPowerOnOff(0);                 //����Զ��Modem
          
       #ifdef PLUG_IN_CARRIER_MODULE 
        if (teIpAndPort.ethIfLoginMs==0x55)
        {
       	 #ifdef JZQ_CTRL_BOARD_V_1_4
       	  gatherModuleType = 2;
       	 #endif
       	 
       	  moduleType = ETHERNET;
        }
        else
        {
         #ifdef WDOG_USE_X_MEGA
          gatherModuleType = 0;
         #endif
        }
       #else
        #ifdef WDOG_USE_X_MEGA
         gatherModuleType = 0;
        #endif
       #endif
      }
    }
   	  
   	return;
  }
      
  if (wlModemFlag.power>0)
  {
      if (compareTwoTime(wlPowerTime, sysTime))
      {
      	switch (wlModemFlag.power)
      	{
      	 	case 1:
      	 	  if(moduleType==GPRS_MC52I)
      	 	  {
      	 	    resetWlModem(1);
      	 	  }
      	 	  else
      	 	  {
      	 	    resetWlModem(0);
      	 	  }

      	 	  wlModemFlag.power = 0;

						//2018-6-11,
						if (moduleType==ETHERNET)
						{
      	 	  	waitTimeOut = nextTime(sysTime,1,30); //��ʱʱ��Ϊ1��30��
						}
						else
						{
      	 	  	waitTimeOut = nextTime(sysTime, 3, 0);//��ʱʱ��Ϊ3��
						}

						wlModemFlag.sendLoginFrame = 0;       //��û�з��͵�¼֡
     	  	  wlModemFlag.permitSendData = 0;       //��������Ӧ�ò�����
     	  	  wlModemFlag.loginSuccess   = 0;       //Ӧ�ò��¼��վ�Ƿ�ɹ���δ�ɹ�
     	  	  
     	  	  wlModemFlag.numOfWlReset++;           //��λ������1
            
            if (debugInfo&WIRELESS_DEBUG)
            {
            	 printf("��λԶ��Modem,��λ����=%d\n",wlModemFlag.numOfWlReset);
            }
            sprintf(tmpWlStr,"��λԶ��Modem,��λ����=%d",wlModemFlag.numOfWlReset);
            
            #ifdef RECORD_LOG
             //logRun(tmpWlStr);
            #endif
            
            //û�е�¼�ɹ��Ҹ�λ������3����0,ʹ�ñ���IP��ַ,����ʹ������IP��ַ
            if (wlModemFlag.lastIpSuccess==0 && ((wlModemFlag.numOfWlReset%3)==0))
            {
              wlModemFlag.useMainIp = 0;
            }
            else
            {
              wlModemFlag.useMainIp = 1;
            }

     	  	  wlModemFlag.lastIpSuccess = 0;        //�ϴ�IP��¼��Ϊδ�ɹ�
            
            if (moduleType!=CDMA_CM180)
            {
              setModemSoPara();                   //����Modem���ʼ����
            }
            
            //�������ն�
            if (checkIfCascade()==2 && workSecond>10)
            {
            	printf("����������Ӧ�ò�����\n");
     
              wlModemFlag.permitSendData = 1;     //������Ӧ�ò�����
            }
            
            if (moduleType==MODEM_PPP)
            {
              //�������
              tmpBuf[0]=1;
              sendToDialer(1, tmpBuf, 1);
              
              //����APN
              memcpy(tmpBuf, ipAndPort.apn, 16);
              sendToDialer(8, tmpBuf, 16);
              
              //����VPN
              memcpy(tmpBuf, vpn.vpnName, 32);
              sendToDialer(9, tmpBuf, 32);
              memcpy(tmpBuf, vpn.vpnPassword, 32);
              sendToDialer(10, tmpBuf, 16);
            }
            break;
      	 
      	  case 2:   //����ģ�鴦�ڶϵ�״̬
      	 	  //2012-07-27,M590E��ģ���ϵ�ǰ,����ON/OFF�ܽ�
      	 	  if (moduleType==GPRS_M590E)
      	 	  {
          	  ioctl(fdOfIoChannel, WIRELESS_IGT, 1);
               
          	  if (debugInfo&WIRELESS_DEBUG)
          	  {
          	    printf("M590E�ӵ�ǰ�ÿ�����ֵΪ1\n");
          	  }
      	 	  }
      	 	  
      	 	  //������ģ���Դ
      	 	  wlModemPowerOnOff(1);
      	 	  
      	 	  //����Modem����
      	 	  if(moduleType==GPRS_MC52I)
      	 	  {
      	 	    resetWlModem(0);
      	 	  }
      	 	  else
      	 	  {
      	 	    resetWlModem(1);
      	 	  }
      	 	  
      	 	  break;
      	 	
      	 	case 3:   //M590E���ӳٹػ�,2012-08-02,add
        	  //�ض�Զ��MODEM��Դ
        	  ioctl(fdOfIoChannel, WIRELESS_POWER_ON_OFF, 0);
        	   
        	  if (debugInfo&WIRELESS_DEBUG)
        	  {
        	    printf("(�ӳ�)����Modem��Դ:0\n");
        	  }
        	   
            wlPowerTime = nextTime(sysTime,0,5);//5����Modem��Դ
            wlModemFlag.power = 2;              //����Modem�ϵ�
             
            //�������ն�
            if (checkIfCascade()==2)
            {
            	;
            }
            else
            {
             #ifdef PLUG_IN_CARRIER_MODULE 
              if (teIpAndPort.ethIfLoginMs==0x55)
              {
           	    #ifdef JZQ_CTRL_BOARD_V_1_4
           	     gatherModuleType = 2;
           	    #endif
           	 
           	    moduleType = ETHERNET;
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 sendXmegaFrame(GATHER_IO_DATA, &i, 0);
                #endif
              }
             #else
              #ifdef WDOG_USE_X_MEGA
               sendXmegaFrame(GATHER_IO_DATA, &i, 0);        
              #endif
             #endif
            }
      	 		break;
      	}
      }
      
      return;
   }
   
   //������IP����ΪFALSE,Ҳ����˵������·û�н�ͨ
   if (wlModemFlag.permitSendData==0)
   {
      if (wlModemFlag.sendToModem == 1)
    	{
        if (compareTwoTime(wlModemFlag.waitModemRet, sysTime))
        {
        	 if (debugInfo&WIRELESS_DEBUG)
        	 {
        	 	  printf("����Զ��Modem�޻ظ���ʱʱ�䵽,����CPU��Modemͨ�Ų�����,��λ�ն�\n");
        	 }
        	 
           logRun("����Զ��Modem�޻ظ���ʱʱ�䵽,����CPU��Modemͨ�Ų�����,��λ�ն�");

        	 ifReset = TRUE;
        }
    	}

      //12-11-07
      if ((bakModuleType!=MODEM_PPP) && (operateModem==1))
      {
        if (compareTwoTime(waitTimeOut, sysTime))
        {
         	 //��¼���ɹ���ʱ����,�����ն�
         	 if (wlModemFlag.numOfWlReset>RE_LOGIN_TIMES)
         	 {
         	 	  if (debugInfo&WIRELESS_DEBUG)
         	 	  {
         	 	    printf("Modem:%d�ε�¼���ɹ�,�����ն�\n",RE_LOGIN_TIMES);
         	 	  }
  
    	 	  	  sprintf(tmpWlStr,"��������Modem:%d�ε�¼���ɹ�,�����ն�",RE_LOGIN_TIMES);
    	 	  	  logRun(tmpWlStr);
         	 	  
         	 	  ifReset = TRUE;
         	 }
         	 else
         	 {
         	   wlModemPowerOnOff(0);
         	 	 
         	 	 if (debugInfo&WIRELESS_DEBUG)
         	 	 {
         	 	   printf("��¼���ɹ�,����ģ��\n");
         	 	 }
         	 }
         	 
         	 return;
        }

        wlRet = configWirelessModem();
      }
      else
      {
      	if (dialerOk)
      	{
          //���Ӳ��ɹ��Ĵ���
          if (compareTwoTime(waitTimeOut, sysTime))
          {
          	printf("dlzd��ʱ�ж�,�ر�fdOfModem\n");
          	
          	setModemSoPara();
          	
          	return;
          }
           
          wlRet = configWirelessModem();
      	}
      	else
      	{
      		return;
      	}
      }
      
      switch (wlRet)
      {
      	case 1:
      	  if (moduleType==ETHERNET || bakModuleType==MODEM_PPP)
      	  {
     	  	  wlModemFlag.permitSendData = 1;   //������Ӧ�ò�����
     	  	  
     	  	  if (bakModuleType==MODEM_PPP)
     	  	  {
              wlModemFlag.numOfWlReset = 0;
     	  	  }
      	  }
      	  break;
      }
   }
   else   //������IP����
   {
   	 if (moduleType==GPRS_MC52I || moduleType==GPRS_M590E)
   	 {
   	 	 configWirelessModem();
   	 }
   	 
   	 //��¼
   	 if (wlModemFlag.sendLoginFrame==0)
   	 {
   	 	 //ly,2011-12-02,�ڲ���̨�����ʱ����,����������ù��ܵĻ�,����з�����ȥ,��ѻ����ݶ�ʹ��¼��������
       //v1.61 11-11-25��ר���ն˳���δ���˸ı�,11-12-02��ר���ն˳����Ѹı�
       //v1.61�ļ��������˴˸ı�    	 
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
       fQueue.active0dDataSending = 0;       
       //-----------------------------------------

   	 	 wlModemFlag.sendLoginFrame = 1;
   	 	  
   	 	 AFN02001Login();
      
       #ifdef PLUG_IN_CARRIER_MODULE
        #ifndef MENU_FOR_CQ_CANON
         showInfo("��¼��վ...");
        #endif
       #endif
   	 	 
   	 	 if (debugInfo&WIRELESS_DEBUG)
   	 	 {
   	 	   printf("%02d-%02d-%02d %02d:%02d:%02d���͵�¼֡\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
   	 	 }
 	  	 
 	  	 wlModemFlag.loginSuccess = 0;
 	  	 waitTimeOut = nextTime(sysTime,0,56);   //��ʱʱ����Ϊһ���Ӻ�
 	  	 
 	  	 return;
   	 }
   	 
     if (wlModemFlag.loginSuccess == 0)
     {
       //��¼��ʱ,����Ӧ�Ķ���(��λģ��RE_LOGIN_TIMES�κ󻹲��о͸�λ�ն�)
       if (compareTwoTime(waitTimeOut, sysTime))
       {
         	if (wlModemFlag.numOfWlReset>RE_LOGIN_TIMES)  //��λ�ն�/������
         	{
         		if (debugInfo&WIRELESS_DEBUG)
         		{
         		  printf("Modem:��¼��ʱ,��λ�ն�\n");
         		}
  	 	  	  
  	 	  	  #ifdef RECORD_LOG
  	 	  	   logRun("��������Modem:��¼��ʱ����,�����ն�");
  	 	  	  #endif

         		ifReset = TRUE;
          }
          else    //��λģ��
          {
         	  if (debugInfo&WIRELESS_DEBUG)
         	  {
         	    printf("��¼��ʱ��\n");
         	  }
         	  
         	  //2012-11-09
         	  if (bakModuleType==MODEM_PPP)
         	  {
         	  	resetPppWlStatus();
         	  }
         	  else
         	  {
         	    wlModemPowerOnOff(0);
         	  }
         	}
       }
     }
     else
     {
       	//����
       	if (compareTwoTime(nextHeartBeatTime, sysTime))
        {
        	
repeatHeart:	
 	         AFN02003HeartBeat();

 	         wlModemFlag.heartSuccess = 0;
 	         
    	    #ifdef LIGHTING
    	     
    	     //2016-06-22,Add����ж� 
	         //2016-07-04,����������������Ϊ����Ϊ��λ
    	     nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);

 	         waitTimeOut = nextTime(sysTime, 0, (commPara.heartBeat-3));
    	      	
    	    #else

           nextHeartBeatTime = nextTime(sysTime, commPara.heartBeat, 0);
 	         waitTimeOut = nextTime(sysTime,0,50);
          
          #endif
          
           lastHeartBeat = sysTime;
           
           if (debugInfo&WIRELESS_DEBUG)
           {
             printf("%02d-%02d-%02d %02d:%02d:%02d,wlModem�´�����ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",
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
           
           #ifdef PLUG_IN_CARRIER_MODULE
            #ifndef MENU_FOR_CQ_CANON
             showInfo("��������֡...");
            #endif
           #endif
        }
        
        if (wlModemFlag.heartSuccess==0)
        {
       	   //������ʱ,��λģ����Ǹ�λ�ն�
       	   if (compareTwoTime(waitTimeOut, sysTime))
           {
         	    //������ʱ,��λģ����Ǹ�λ�ն˻���������һ��
         	    wlModemFlag.numOfRetryTimes++;
         	    if (wlModemFlag.numOfRetryTimes<3)
         	    {
             	  if (debugInfo&WIRELESS_DEBUG)
             	  {
               	  printf("wlModem:ǰһ��������ʱ,������һ������\n");
                }
                
         	   	  goto repeatHeart;
         	    }
         	    else
         	    {
             	  if (wlModemFlag.numOfWlReset>RE_LOGIN_TIMES)   //��ʱ����,��λ�ն�/������
             	  {
           	      if (debugInfo&WIRELESS_DEBUG)
           	      {
             	      printf("Զ��Modem:������ʱ,��λ�ն�\n");
             	    }
             	    
             	    #ifdef RECORD_LOG
             	     logRun("Զ��Modem:������ʱ����,��λ�ն�");
             	    #endif
             	    
             	    //ly,2010-12-24,���ش���,
             	    //   �����ֳ��ͱ���������"��������...",�����Ͳ��ɹ�3�κ�Ҳ����λ�ն�,ֻд����־,
             	    //   ��û�и�λ����,û��������λ�ն�
             	    ifReset = TRUE;
             	  }
             	  else    //��λģ��
             	  {
               	  //2012-11-09
               	  if (bakModuleType==MODEM_PPP)
               	  {
               	  	resetPppWlStatus();
               	  }
               	  else
               	  {
             	      wlModemPowerOnOff(0);
             	    }
           	    
           	      if (debugInfo&WIRELESS_DEBUG)
           	      {
             	      printf("������ʱ?\n");
             	    }
             	  }
             	}
           }
        }
 	   }
   }
}

/*******************************************************
��������:setModemSoPara
��������:����Modem���ʼ������
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void setModemSoPara(void)
{
  struct wirelessPara modemPara;

  if (wlModemFlag.useMainIp==1)
  {
     memcpy(modemPara.loginIp,ipAndPort.ipAddr,4);
     modemPara.loginPort = ipAndPort.port[0] | ipAndPort.port[1]<<8;

     if (debugInfo&WIRELESS_DEBUG)
     {
       printf("Զ��Modem:ʹ������IP��ַ\n");
     }
  }
  else
  {
     if ((ipAndPort.ipAddrBak[0]==0 && ipAndPort.ipAddrBak[1]==0 && ipAndPort.ipAddrBak[2]==0 && ipAndPort.ipAddrBak[3]==0)
     	   || (ipAndPort.portBak[0]==0 && ipAndPort.portBak[1]==0)
     	  )
     {
       memcpy(modemPara.loginIp,ipAndPort.ipAddr,4);
       modemPara.loginPort = ipAndPort.port[0] | ipAndPort.port[1]<<8;

       if (debugInfo&WIRELESS_DEBUG)
       {
         printf("Զ��Modem:����IP��ַ��˿�Ϊ0,��ʹ������IP��ַ\n");
       }
     }
     else
     {
       memcpy(modemPara.loginIp,ipAndPort.ipAddrBak,4);
       modemPara.loginPort = ipAndPort.portBak[0] | ipAndPort.portBak[1]<<8;  	 

       if (debugInfo&WIRELESS_DEBUG)
       {
         printf("Զ��Modem:ʹ�ñ���IP��ַ\n");
       }
     }
  }
  
  if (debugInfo&WIRELESS_DEBUG)
  {
    printf("IP:%d.%d.%d.%d,port:%d\n",modemPara.loginIp[0],modemPara.loginIp[1],modemPara.loginIp[2],modemPara.loginIp[3],modemPara.loginPort);
    printf("APN:%s\n",(char *)&ipAndPort.apn);
  }
  
  //����ʶ��ģ������
  #ifdef JZQ_CTRL_BOARD_V_0_3
   if (teIpAndPort.ethIfLoginMs==0x55)
   {
   	 moduleType = ETHERNET;		 
   }
   else
   {
     moduleType = ioctl(fdOfIoChannel,READ_MODULE_TYPE,0);
     moduleType &=0xf;
   }
  #endif

  strcpy(modemPara.apn, (char *)&ipAndPort.apn);
  //ly,2011-06-12,add.�������豸���̸���APN�Ժ��¼�Ͳ��ɹ���bug
  modemPara.apn[16] = '\0';
  modemPara.vpnName[32] = '\0';
  modemPara.vpnPass[32] = '\0';
  
  memcpy(modemPara.vpnName,vpn.vpnName,32);
  memcpy(modemPara.vpnPass,vpn.vpnPassword,32);
  modemPara.pRecvMsBuf = recvMsBuf;
  modemPara.moduleType = moduleType;
  modemPara.reportSignal = signalReport;
  modemPara.sendFrame = sendWirelessFrame;
  modemPara.debugFrame = sendDebugFrame;
  modemPara.portConfig = uart0Config;
  modemPara.msInput = msInput;
  modemPara.localIp = &wlLocalIpAddr;
  modemPara.fdOfEthSocket = &fdOfModem;
  if (moduleType==ETHERNET)
  {
    modemPara.delay = 1000;
  }
  else
  {
    if (bakModuleType==GPRS_SIM900A)
    {
      modemPara.delay = 70;
    }
    else
    {
    	if (LTE_AIR720H==moduleType)
    	{
      	modemPara.delay = 100;
    	}
			else
			{
      	modemPara.delay = 50;
			}
    }
    
    //�鿴ϵͳ�Ƿ�֧��pppЭ��
    if(access("/dev/ppp", F_OK) == 0)
    {
    	printf("/dev/ppp����\n");
    	
    	modemPara.moduleType = MODEM_PPP;

    	bakModuleType = MODEM_PPP;
    	
    	waitTimeOut = nextTime(sysTime, 1, 20);
    	
    	wlModemFlag.numOfWlReset++;
      
      //2012-11-16,һֱ�������Ӷ�û�гɹ�,����ppp0�Ƿ����,�ر���,���½��в�������
      //����������APN����,������վIP��ַ(/�˿�)����ȷ��ʱ����˷�����
      if (wlModemFlag.numOfWlReset>5)
      {
      	printf("dialerOk=1,����һֱ�������Ӷ����ɹ�,�ر�ppp0,���½��в�������\n");
      	
      	wlModemFlag.numOfWlReset = 0;
      	
      	system("/bin/ppp-off");    //�ر�ppp0
      }
      
      //û�е�¼�ɹ��Ҹ�λ������3����0,ʹ�ñ���IP��ַ,����ʹ������IP��ַ
      if (wlModemFlag.lastIpSuccess==0 && ((wlModemFlag.numOfWlReset%3)==0))
      {
        wlModemFlag.useMainIp = 0;
      }
      else
      {
        wlModemFlag.useMainIp = 1;
      }
      
      wlModemFlag.lastIpSuccess = 0;        //�ϴ�IP��¼��Ϊδ�ɹ�
    }
    else
    {
    	printf("/dev/ppp������\n");
    }
  }
  
  //��ʼ������Modem��
  initWirelessSo(&modemPara);
  
  wlModemFlag.sendToModem  = 0;
  wlModemFlag.waitModemRet = nextTime(sysTime, WAIT_MODEM_MINUTES, 0); 
}

/**************************************************
��������:threadOfTtys0Received
��������:ttys0���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void *threadOfTtys0Receive(void *arg)
{
  int    recvLen;
  INT8U  tmpBuf[200];

  sleep(10);    //2017-8-23,add
    
  while (1)
  {
  	//12-11-08
  	if ((fdOfModem==NULL) || (operateModem==0))
  	{
  	  usleep(100000);
  		
  	  continue;
  	}
  	
    recvLen = read(fdOfModem, &tmpBuf, 100);
    
    //printf("recvLen=%d\n", recvLen);
    
    if (moduleType==ETHERNET || bakModuleType==MODEM_PPP)
    {
      if (
    	  recvLen==0 
    	   && 1==wlModemFlag.permitSendData    //2015-03-12,add
    	 )
      {
    	printf("���ӶϿ�,socketfd=%d\n", fdOfModem);
    		 
    	wlModemFlag.ethRecvThread  = 0;
    	wlModemFlag.permitSendData = 0;
    	wlModemFlag.loginSuccess   = 0;   //Ӧ�ò��¼��վ�Ƿ�ɹ���δ�ɹ�
    	wlModemFlag.sendLoginFrame = 0;   //�Ƿ��͵�¼֡��Ϊδ����
    	   
    	//2012-11-07
    	if (bakModuleType==MODEM_PPP)
    	{
    	  close(fdOfModem);    //2015-07-17,�ƶ����ж���
    	  fdOfModem = NULL;

    	  setModemSoPara();    //2015-07-17,�ƶ����ж���
    	}
         
        //2015-07-17
        if (moduleType==ETHERNET)
        {
          wlModemPowerOnOff(0);
        }

    	continue;
      }
    }

    if (recvLen>0)
    {
      //2012-07-10,��ֹ����Խ��
      if ((lenOfWlRecv+recvLen)<2047)
      {
      	memcpy(&wlRecvBuf[lenOfWlRecv], tmpBuf, recvLen);   //����MODEM����
        lenOfWlRecv += recvLen;
      }
    }
    
    usleep(50);  
  }
  
  printf("�˳�ttys0�����߳�\n");
}

/**************************************************
��������:threadOfTtys0Received
��������:ttys0���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void wlReceive(INT8U *buf, INT16U len)
{  
  INT16U i;
  INT8U  ret;

  if (debugInfo&WIRELESS_DEBUG)
  {
    printf("%02d-%02d-%02d %02d:%02d:%02dԶ��Modem Rx(%d):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,len);
    
    //���������ڼ�,��ʾͨ���ַ���
    if (wlModemFlag.loginSuccess==0)
    {
      buf[len] = '\0';
      printf("%s", buf);
    }
    
    for(i=0;i<len;i++)
    {
    	 printf("%02x ",buf[i]);
    }
    printf("\n");
  }
 
  ret = wirelessReceive(buf, len);
  switch(ret)
  {
  	case IP_PERMIT:
  	  wlModemFlag.permitSendData = 1;   //������Ӧ�ò�����
  	  if (debugInfo&WIRELESS_DEBUG)
  	  {
  	    printf("������IP����\n");
  	  }
  	  break;
  	    
  	case NO_CARRIER:      //No Carrier
  	case LINK_DISCONNECT: //Disconnect
  	case LINK_FAIL:       //����ʧ��
  	  if (debugInfo&WIRELESS_DEBUG)
  	  {
  	  	if (ret==LINK_FAIL)
  	  	{
  	  	  printf("����ʧ��,���¿�ʼ����!\n");
  	  	}
  	  	else
  	  	{
  	  	  printf("����!\n");
  	  	}
  	  }

  	  wlModemFlag.permitSendData = 0;   //��������Ӧ�ò�����
  	  wlModemFlag.loginSuccess   = 0;   //Ӧ�ò��¼��վ�Ƿ�ɹ���δ�ɹ�
  	  wlModemFlag.sendLoginFrame = 0;   //�Ƿ��͵�¼֡��Ϊδ����
  	  	
  	  setModemSoPara();
      
      if (moduleType==GPRS_M590E || moduleType==CDMA_CM180 || bakModuleType==GPRS_SIM900A)
      {
        if (moduleType==CDMA_CM180)
        {
        	 sleep(1);
        }

        wlModemPowerOnOff(0);  //����Զ��Modem
      }

  	  if (menuInLayer==0)
 	  	{
 	  	  defaultMenu();
 	  	}
  	  break;
  	
  	case MODEM_RET_FRAME:     //����Modem�ظ�����
  		if (wlModemFlag.sendToModem == 1)
  		{
  			 wlModemFlag.sendToModem = 0;
  			 
  			 if (debugInfo&WIRELESS_DEBUG)
  			 {
  			 	  printf("Զ������Modem�������ݻظ�,�����־\n");
  			 }
  		}
  		break;
  }
}

/**************************************************
��������:wlModemPowerOnOff
��������:��������Modem��Դ
���ú���:
�����ú���:
�������:onOff,1-��,0-��
�������:
����ֵ��void
***************************************************/
void wlModemPowerOnOff(INT8U onOff)
{
	INT8U i;
	
	char  atCmd[20];
  
	if (onOff<2)
	{
	  //2012-08-01,add
	  if (moduleType==GPRS_M590E && onOff==0)
	  {
 	    if (debugInfo&WIRELESS_DEBUG)
 	    {
 	      printf("M590E������ػ�����\n");
 	    }

      strcpy(atCmd, "AT+CPWROFF\r");      
      sendWirelessFrame((INT8U *)&atCmd, strlen(atCmd));
      
      wlModemFlag.power = 3;              //����Modem�ӳٹػ�
      wlPowerTime = nextTime(sysTime,0,5);//5����Modem��Դ
      
      return;
    }
    
	  //����Ӳ��
	  ioctl(fdOfIoChannel,WIRELESS_POWER_ON_OFF,onOff);
	   
	  if (debugInfo&WIRELESS_DEBUG)
	  {
	    printf("����Modem��Դ:%d\n",onOff);
	  }
	   
    if (onOff==0)
    {
      wlPowerTime = nextTime(sysTime,0,5);//5����Modem��Դ
      wlModemFlag.power = 2;              //����Modem�ϵ�
       
      //�������ն�
      if (checkIfCascade()==2)
      {
      	;
      }
      else
      {
       #ifdef PLUG_IN_CARRIER_MODULE 
        if (teIpAndPort.ethIfLoginMs==0x55)
        {
          //2018-06-11,Add,��������̫����ͨʱ����GPRSͨ��
					if ((wlModemFlag.lastIpSuccess==0) && (wlModemFlag.useMainIp==0))
          {
					 #ifdef WDOG_USE_X_MEGA
            sendXmegaFrame(GATHER_IO_DATA, &i, 0);
           #endif

					  fdOfModem = open("/dev/ttyS1",O_RDWR|O_NOCTTY);  //��д��ʽ�򿪴���1
          }
					else
					{
					 #ifdef JZQ_CTRL_BOARD_V_1_4
						gatherModuleType = 2;
					 #endif
						
						moduleType = ETHERNET;
					}
        }
        else
        {
          #ifdef WDOG_USE_X_MEGA
           sendXmegaFrame(GATHER_IO_DATA, &i, 0);
          #endif
        }
       #else
        #ifdef WDOG_USE_X_MEGA
         sendXmegaFrame(GATHER_IO_DATA, &i, 0);        
        #endif
       #endif
      }
    }
	}
}

/**************************************************
��������:resetWlModem
��������:��λ����Modem
���ú���:
�����ú���:
�������:press,1-���¿�����,0-�ɿ�������
�������:
����ֵ��void
***************************************************/
void resetWlModem(INT8U press)
{
	 if (press<2)
	 {
	   ioctl(fdOfIoChannel, WIRELESS_IGT, press);
     
	   if (debugInfo&WIRELESS_DEBUG)
	   {
	     printf("������ֵ:%d\n",press);
	   }
     
     if(moduleType==GPRS_MC52I)
     {
       if(press==0)
       {
      	  wlModemFlag.power = 1;
      	  wlPowerTime = nextTime(sysTime,0,1);
       }
     }
     else
     {
       if(press==1)
       {
      	  wlModemFlag.power = 1;
      	  wlPowerTime = nextTime(sysTime,0,3);
       }
     }
	 }
}

/**************************************************
��������:checkIfCascade
��������:�жϼ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��1-�������ն�,2-���������ն�
***************************************************/
INT8U checkIfCascade(void)
{
	//�����˿ڱ���Ϊ3(485�ӿ�2)
	if (cascadeCommPara.commPort==0x03)
	{
  	if ((cascadeCommPara.flagAndTeNumber&0x80)                                           //������־Ϊ1
    	   && ((cascadeCommPara.flagAndTeNumber&0x0f)==0x1)                                //�������ն�ֻ��1��
    	    && (((cascadeCommPara.divisionCode[0]|cascadeCommPara.divisionCode[1]<<8)>0) && ((cascadeCommPara.divisionCode[0]|cascadeCommPara.divisionCode[1]<<8)<0x9999))  //�������ն�����������1��9999֮��
    	     && ((cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8)>0) //�������ն˵�ַ>0
    	 )
    {
    	return 2;
    }
  
  	if (((cascadeCommPara.flagAndTeNumber&0x80)==0x0)       //������־Ϊ0
    	    && (((cascadeCommPara.flagAndTeNumber&0x0f)>=0x1) && ((cascadeCommPara.flagAndTeNumber&0x0f)<=0x3))  //���������ն�ֻ����1��3��
    	 )
    {
    	return 1;
    }
  }
  
  return 0;
}

/**************************************************
��������:resetPppWlStatus
��������:��λPPP��ʽ�µ�Modem�Զ����״̬
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
void resetPppWlStatus(void)
{
	wlModemFlag.permitSendData = 0;
	wlModemFlag.loginSuccess   = 0;   //Ӧ�ò��¼��վ�Ƿ�ɹ���δ�ɹ�
	wlModemFlag.sendLoginFrame = 0;   //�Ƿ��͵�¼֡��Ϊδ����
  
  wlModemFlag.ethRecvThread  = 0;
  
  if (fdOfModem!=NULL)
  {
    close(fdOfModem);
  }
  
  fdOfModem=0;
	
	setModemSoPara();
}

/***************************************************
��������:threadOfRecvUdpData
��������:����UDP�����߳�
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
void *threadOfRecvUdpData(void *arg)
{
  INT8U  recvBuff[64];
  INT8U  lenOfRecv;
  INT8U  i;
  INT8U  checkSum;
  struct sockaddr_in clientAddr;
  int    lenOfAddr = sizeof(struct sockaddr_in);

  while(1)
  {
    lenOfRecv = recvfrom(socketOfUdp, recvBuff, 64, 0, (struct sockaddr*)&clientAddr, &lenOfAddr);
    if (lenOfRecv>0)
    {
    	if (debugInfo&WIRELESS_DEBUG)
    	{
    	  printf("UDP Rx:");
    	  for(i=0; i<lenOfRecv; i++)
    	  {
    		  printf("%02X ", recvBuff[i]);
    	  }
    	  printf("\n");
    	}
    	
    	if (recvBuff[0]==0x68 && recvBuff[lenOfRecv-1]==0x16)
    	{
    		checkSum = 0;
    		for(i=1; i<lenOfRecv-2; i++)
    		{
    			checkSum += recvBuff[i];
    		}
    	}
    	if (checkSum==recvBuff[lenOfRecv-2])
    	{
    		switch(recvBuff[2])    //AFN
    	  {
    	    case 1:    //dialer��������modem
    	      //ִ�е�Դ�������ػ�����
    	      wlModemPowerOnOff(0);
    	      operateModem = 1;
    	      break;
    	    
    	    case 2:    //����ע��״̬
    	    	break;
    	    
    	    case 3:    //�ź�
    	    	if (wlRssi!=recvBuff[3])
    	    	{
    	    	  signalReport(0, recvBuff[3]);
    	    	}
    	    	break;
    	    
    	    case 4:    //dialer ok
  	        if (0x1==recvBuff[3])
  	        {
  	          if (!dialerOk)
  	          {
    	          if (debugInfo&WIRELESS_DEBUG)
    	          {
  	              printf("dataBase:detect dialer ok, may establish link\n");
  	            }
  	          }
  	        
  	          dialerOk = 1;
  	        }
    	      break;
    	      
    	    case 5:    //��ȡ��IP��ַ
  	        //�ն�IP��ֵ
  	        wlLocalIpAddr = recvBuff[3]<<24 | recvBuff[4]<<16 | recvBuff[5]<<8 | recvBuff[6];
    	    	break;
    	    
    	    case 6:    //δ�忨ָʾ
        	  //��ʾ�޿���־
        	  if (0x02==recvBuff[3])
        	  {
        	  	modemSignal(9);
        	  }
        	  else
        	  {
        	  	modemSignal(8);
        	  }
    	    	break;
    	  }
    	}
    	else
    	{
    		printf("У�����%x\n",checkSum);    		
    	}
    }
    else
    {
      perror("recv");
    }
    
    usleep(1000);
  }
}

/***************************************************
��������:sendToDialer
��������:����UDP����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
unsigned char sendToDialer(unsigned char afn, unsigned char *pData, unsigned char lenOfData)
{
  int                socketOfSendUdp;
  struct sockaddr_in destAddr;
  unsigned char      sendBuff[52];
  unsigned char      i;
  unsigned char      checkSum;
  
  if ( (socketOfSendUdp=socket(AF_INET, SOCK_DGRAM, 0)) <0)
  {
    perror("socket");
  }
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(65432);
  destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (destAddr.sin_addr.s_addr == INADDR_NONE)
  {
    printf("Incorrect ip address!");
    close(socketOfSendUdp);
  }  
  
  sendBuff[0] = 0x68;
  sendBuff[1] = lenOfData+1;
  sendBuff[2] = afn;
  memcpy(&sendBuff[3], pData, lenOfData);
  checkSum=0;
  for(i=1;i<lenOfData+3; i++)
  {
  	checkSum += sendBuff[i];
  }
  sendBuff[lenOfData+3] = checkSum;
  sendBuff[lenOfData+4] = 0x16;
  
  if (sendto(socketOfSendUdp, sendBuff, lenOfData+5, 0, (struct sockaddr *)&destAddr, sizeof(struct sockaddr_in))<0)
  {
    perror("sendto");
  }
  
}

