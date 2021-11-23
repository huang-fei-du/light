/***************************************************
Copyright,2010,HuaWei WoDian co.,LTD,All	Rights Reserved
�ļ�����wirelessModem.c
���ߣ�leiyong
�汾��0.9
������ڣ�2009��1��
����������Modem��̬���ӿ⴦���ļ���

�����б�
     1.wireless��ں���(wireless())
�޸���ʷ��
  01,10-01-24,Leiyong created.
  02,11-12-27,Leiyong add,M590��ȡCCID,ȷ���ڽ���PPP������ǰCREG����(0,1),����ڱ��Ƿ�����С���ն���ĳЩ�����Ѷ��Ų�������������ߵ����
  03,11-12-28,Leiyong add,SIM300C(900A)�趨����TCP�˿�,��������ط�������ͨGPRS�޷���ʱ��ͬʱ��¼������
  04,12-02-27,Leiyong add,SIM300C(900A)���AT+CREG��AT+CGATT�Լ��ĳ�AT_CIICR�����ù̶��ȴ�,ȷ�����ص�¼ʱ�䲻�����ܳɹ�
  05,12-09-18,Leiyong modify,���ٷ���������������SIM900A(/SIM300C)��ѯע��״̬ʱ����������ע��ɹ�,ԭ��û�д�������ע��ɹ����Ե�¼
              ����֧�ֵ�ģ������޸�

***************************************************/

#include "string.h"
#include "stdio.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>


#include "wirelessModemx.h"
#include "wirelessModem.h"

#define TIME_OUT_TIME 20 //connect��ʱʱ��20��,2012-11-8

INT8U        atType;                       //AT��������
INT8U        atWait0d0a=0;                 //AT����ȴ�������0D0A
WL_FLAG_SET  wlFlagsSet;                   //���߱�־��
INT8U        wirelessBuf[SIZE_OF_WIRELESS];//����UART0���ջ���
INT16U       tailOfWireFrame=0;            //��վ��������֡β��
INT8U        sendBuffer[1500];             //���ͻ���
INT16U       sendLength;                   //���ͳ���
INT16U       sendTimeOut;                  //���ͳ�ʱ����
INT16U       wlLocalTcpPort=2012;          //����MODEM����TCP�˿�

//��������
char         cops[20];                     //��Ӫ��
BOOL         flagSwitch;                   //ת��GPRSģʽ��AT����ģʽ��־

BOOL         ipPermit;                     //������IP��
INT8U        ifLogin = 0;                  //�Ƿ��͵�¼����

INT8U        ifAtToGprs = 1;               //�Ƿ�������ģʽת������ģʽ

INT8U        rssi = 99;                    //GPRS/CDMA�����ź�ǿ��ָʾ
INT8U        bitErr = 99;                  //GPRS/CDMA�ŵ�λ����

INT8U        dailTimeOut;                  //���ų�ʱ?

INT16U       tailOfWireless = 0;           //����UART0����֡β��

INT16U       lenOfWireMsFrame;             //��վ�������ݳ���
INT16U       tailOfWireFrame;              //��վ��������֡β��

INT32U       delay = 0;

INT8U        say;
INT16U       tmpk;

INT8U        retry = 0;

struct wirelessPara modemPara;

/*******************************************************
��������:initWirelessSo
��������:��ʼ������Modem��
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE-��ʼ���ɹ�
        FALSE-��ʼ��ʧ��
*******************************************************/
BOOL initWirelessSo(struct wirelessPara *para)
{
  modemPara = *para;

  //��λ���б�־
  resetAllFlag(0);
  
  wlFlagsSet.ainfInputed = FALSE;
  
  return TRUE;
}

/***************************************************
��������:configWirelessModem
��������:��������Modem
���ú���:
�����ú���:
�������:
�������:
����ֵ:
***************************************************/
INT8U configWirelessModem(void)
{
   INT8U  atCmd[60];
   char   str[10];
   INT8U  i;
   INT8U  buf[5];
   
	 struct sockaddr_in servaddr;
	 INT16U len;
	 struct timeval timeout = {3,0}; //3s
	 INT8S  *zErrMsg = 0;
   
   //2012-11-8
   fd_set set;
   int error=-1, lenx;
   lenx = sizeof(int);
   
	 //���������IP��
   if (ipPermit==TRUE && (modemPara.moduleType==GPRS_MC52I || modemPara.moduleType==GPRS_M590E || modemPara.moduleType==CDMA_CM180))
 	 {
 	 	  switch (modemPara.moduleType)
 	 	  {
 	 	  	case GPRS_MC52I:
     	 	  if (wlFlagsSet.wlSendStatus==1)
     	 	  {
     	 	  	 wlFlagsSet.wlSendStatus = 2;    //����״̬��Ϊ2,�ѷ�������Modem��������
     	       strcpy((char *)atCmd,"at^sisw=1,");
     	       strcat((char *)atCmd,intToString(sendLength,3,str));
     	       strcat((char *)atCmd,"\r");
     	       atType = AT_MC5X_REQUEST_SEND;
     	       modemPara.sendFrame(atCmd,strlen((char *)atCmd));
     	       return 0;
     	 	  }
     	 	  
     	 	  if (wlFlagsSet.wlSendStatus==2)
     	 	  {
     	 	  	 return 0;
     	 	  }
     	 	  	
     	 	  if (wlFlagsSet.wlSendStatus==3)
     	 	  {
             atWait0d0a = 1;
     	 	  	 wlFlagsSet.wlSendStatus=4;
     	       modemPara.sendFrame(sendBuffer,sendLength);
     	       return 0;
     	 	  }
     	 	  
          if (delay > 30)
          {
            delay = 0;
          }
          else
          {
            delay++;
            return 0;
          }
          
          if (wlFlagsSet.simCifsr == FALSE)
          {
     	      strcpy((char *)atCmd,"at^sici?\r");
     	      modemPara.sendFrame(atCmd,strlen((char *)atCmd));
     	      atType = AT_CIFSR;
          	return 0;
          }
          
     	 	  if (wlFlagsSet.mc5xRecvStatus==0)
     	 	  {
     	      strcpy((char *)atCmd,"AT^SISR=1,1500\r");
     	      modemPara.sendFrame(atCmd,strlen((char *)atCmd)); 	       
     	      return 0;
     	 	  }
     	 	  
     	 	  return 0;
     	 	  break;
     	 
     	  case GPRS_M590E:     	 	  
     	 	  //��֡Ҫ����
     	 	  if (wlFlagsSet.wlSendStatus>= 1 && wlFlagsSet.wlSendStatus<4)
     	 	  {	          
	          sendTimeOut++;
	          
	          //printf("sendTimeOut=%d\n",sendTimeOut);
	          
	          //ǰһ֡δ����ȥ,����
	          if (sendTimeOut>200)
	          {
	          	 sendTimeOut = 0;
	          	 wlFlagsSet.wlSendStatus = 0;
	          	 
	          	 printf("ǰһ֡δ����ȥ,��ԭ��sendStatus״̬\n");
	          }
	        }
	   
     	 	  if (wlFlagsSet.wlSendStatus==1)
     	 	  {
     	 	  	 wlFlagsSet.wlSendStatus = 2;    //����״̬��Ϊ2,�ѷ�������Modem��������
     	       strcpy((char *)atCmd,"at+tcpsend=0,");
     	       strcat((char *)atCmd,intToString(sendLength,3,str));
     	       strcat((char *)atCmd,"\r");     	       
     	       modemPara.sendFrame(atCmd, strlen((char *)atCmd));
     	       
     	       //printf("M590E������\n");
     	       return 0;
     	 	  }
     	 	  
     	 	  if (wlFlagsSet.wlSendStatus==2)
     	 	  {
     	 	  	 return 0;
     	 	  }
     	 	  	
     	 	  if (wlFlagsSet.wlSendStatus==3)
     	 	  {
     	       modemPara.sendFrame(sendBuffer, sendLength+1);
     	 	  	 wlFlagsSet.wlSendStatus = 4;
     	 	  	 sendTimeOut = 0;
     	       return 0;
     	 	  }
     	 	  
     	 	  return 0;
     	 	  break;
     	}
 	 }

   switch(modemPara.moduleType)
   {
       case GPRS_GR64:
          //���ⷢ��̫��ͨ��ģ����������������е���ʱ         
          if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK)
          {
            if (delay > 200)
            {
              delay = 0;
            }
            else
            {
          	  delay++;
          	  return 0;
            }
          }
          else
          {
            if (delay > 40)
            {
              delay = 0;
            }
            else
            {
          	  delay++;
          	    
          	  return 0;
            }
          }

         if (wlFlagsSet.echo == FALSE)              //�������δ�ر�
         {
 	          strcpy((char *)atCmd,"ATE0\r");
 	          atType = AT_ECHO;
 	          modemPara.sendFrame(atCmd,5);
 	          return 0;
         }
          
         //���û�ж���SIM�Ĳ���״̬
         if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<10)
         {
 	          retry++;
            if (retry==10)
            {
 	             modemPara.reportSignal(8,0);
 	          }
 	          else
 	          {
 	             strcpy((char *)atCmd,"AT*E2SSN\r");  //SIM���Ƿ����
 	             atType = AT_SIM;
 	             modemPara.sendFrame(atCmd,9);
 	          }
 	          
 	          return 0;
         }
         
         //�Ѳ���SIM��
         if(wlFlagsSet.cardInserted == AT_M_RESPONSE_OK)
         {
         	  if (rssi==99 || rssi>31)         //��ȡ�ź�
         	  {
         	 	   strcpy((char *)atCmd,"AT+CSQ\r");
         	 	   atType = AT_SIGNAL;
         	 	   modemPara.sendFrame(atCmd,7);
 	             
 	             return 0;
         	  }
         	  
         	  if (wlFlagsSet.cgdcont == FALSE)        //PDP����
         	  {
               strcpy((char *)atCmd,"AT+CGDCONT=1,\"IP\",\"");
               strcat((char *)atCmd,trim(modemPara.apn));
               strcat((char *)atCmd,"\"\r");
         	 	   atType = AT_CGDCONT;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
   	           return 0;
         	  }
         	  
         	  if (wlFlagsSet.gr64Ips == FALSE)       //GR64IP��������
         	  {
         	 	   strcpy((char *)atCmd,"AT*E2IPS=2,8,10,1460,0,64\r");
         	 	   atType = AT_GR64_IPS;
         	 	   modemPara.sendFrame(atCmd,26);
   	           
   	           return 0;
         	  }
             
         	  if (wlFlagsSet.numOfGr64Ipa == 0)           //����IP
         	  {
         	 	   strcpy((char *)atCmd,"AT*E2IPA=1,1\r");
         	 	   atType = AT_GR64_IPA;
         	 	   modemPara.sendFrame(atCmd,13);
         	 	   wlFlagsSet.numOfGr64Ipa = 1;
         	 	   
   	           return 0;
         	  }
         	  
             if (wlFlagsSet.gr64Ipa == FALSE)
             {
             	  return 0;
             }

         	  if (wlFlagsSet.gr64Addr == FALSE)      //��ȡGR64 PDP IP��ַ
         	  {
         	 	   strcpy((char *)atCmd,"AT+CGPADDR=1\r");
         	 	   atType = AT_GR64_ADDR;
         	 	   modemPara.sendFrame(atCmd,13);
         	 	   return 0;
         	  }
         	  
         	  if (wlFlagsSet.ipStarted==FALSE)        //����TCP������
         	  {
         	 	   strcpy((char *)atCmd,"AT*E2IPO=1,\"");
               strcat((char *)atCmd,loginIpAddr());
         	 	   strcat((char *)atCmd,"\",");
         	 	   strcat((char *)atCmd,loginPort());
         	 	   strcat((char *)atCmd,"\r");
         	 	   
         	 	   atType = AT_IP_START;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
         	  	 wlFlagsSet.ipStarted = TRUE;
         	 	   return 0;
         	  }
          }

       	 break;
       	
       case GPRS_SIM300C:
          //���ⷢ��̫��ͨ��ģ����������������е���ʱ         
          if (delay > modemPara.delay)
          {
            delay = 0;
          }
          else
          {
          	 delay++;
         	 	 
         	 	 return 0;
          }

 	 	     //��̬����ź�����
 	 	     if (flagSwitch == TRUE)
 	 	     {
 	 	        strcpy((char *)atCmd,"AT+CSQ\r");
 	 	        atType = AT_RE_SIGNAL;
 	 	        modemPara.sendFrame(atCmd,7);
 	 	        
         	 	return 0;
 	 	     }

         //�������δ�ر�
         if (wlFlagsSet.echo == FALSE)
         {
 	          strcpy((char *)atCmd,"ATE0\r");
 	          atType = AT_ECHO;
 	          modemPara.sendFrame(atCmd,5);
 	          
         	 	return 0;
         }
 
         //���û�ж���SIM�Ĳ���״̬
         if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<100)
         {
             retry++;
             if (retry==100)
             {
                modemPara.reportSignal(8,0);
             }
             strcpy((char *)atCmd,"AT+CSMINS?\r");
             atType = AT_SIM;
             modemPara.sendFrame(atCmd,11);
             
             return 0;
         }

         //�Ѳ���SIM��
         if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
         {
            //���DCD����δ�ɹ�
            if (wlFlagsSet.dcd == FALSE)
            {
          	 	 strcpy((char *)atCmd,"AT&C1\r");
          	 	 atType = AT_DCD;
          	 	 modemPara.sendFrame(atCmd,6);
               return 0;
          	}
          	  
          	if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
          	{
          	 	 strcpy((char *)atCmd,"AT+CSQ\r");
          	 	 atType = AT_SIGNAL;
          	 	 modemPara.sendFrame(atCmd,7);

               return 0;
          	}

            if (wlFlagsSet.creg != AT_M_RESPONSE_OK)
            {
            	strcpy((char *)atCmd,"AT+CREG?\r");
            	atType = AT_REG;
            	modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	 	
           	 	return 0;
            }
       	    
       	    if (wlFlagsSet.ipMode==FALSE)
       	    {
       	 	     strcpy((char *)atCmd,"AT+CIPMODE=1\r");
       	 	     atType = AT_IP_MODE;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }

            if (wlFlagsSet.simCgatt != AT_M_RESPONSE_OK)
         	  {
         	    strcpy((char *)atCmd,"AT+CGATT?\r");
         	    atType = AT_CGATT;
           	  modemPara.sendFrame(atCmd,strlen((char *)atCmd));
           	  
           	  //ly,2012-04-24,SIM900A���Ƕ��źŷ�Ӧ̫��,�������ٶ�һ��
           	  wlFlagsSet.signal = AT_M_RESPONSE_NONE;
           	  return 0;
            }

         	  if (wlFlagsSet.simTransparent==FALSE)
         	  {
         	 	   //strcpy((char *)atCmd,"AT+CIPCCFG=5,10,1024,1\r");
         	 	   strcpy((char *)atCmd,"AT+CIPCCFG=5,3,1024,1\r");   //LY 09-06-29,modify this line
         	 	   atType = AT_TRANSPARENT;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }
         	  
         	  if (wlFlagsSet.simApn==FALSE)
         	  {
         	 	   strcpy((char *)atCmd,"AT+CSTT=\"");

               strcat((char *)atCmd,trim(modemPara.apn));
               //strcat((char *)atCmd,"cmnet");
               strcat((char *)atCmd,"\",\"\",\"\"\r");
         	 	   atType = AT_APN;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }
         	  
             //if (wlFlagsSet.simCiicr==0)
             //{
         	 	 //  strcpy((char *)atCmd,"AT+CIICR\r");
         	 	 //  atType = AT_CIICR;
         	 	 //  modemPara.sendFrame(atCmd,9);
         	 	 //  wlFlagsSet.simCiicr=1;
             //  return 0;
             //}
             
             //ly,2012-02-27,�ĳ��������
             if (wlFlagsSet.simCiicr!=0x1F)
             {
               wlFlagsSet.simCiicr++;
               if (wlFlagsSet.simCiicr>1)
               {
                 if (wlFlagsSet.simCiicr>28)
                 {
               	   wlFlagsSet.simCiicr = 1;

         	 	       strcpy((char *)atCmd,"AT+CIICR\r");
         	 	       atType = AT_CIICR;
         	 	       modemPara.sendFrame(atCmd, strlen((char *)atCmd));
                 }
                 
                 return 0;
               }
           	   
           	   return 0;
             }

             if (wlFlagsSet.simCifsr == FALSE)
             {
         	 	   strcpy((char *)atCmd,"AT+CIFSR\r");
         	 	   atType = AT_CIFSR;
         	 	   modemPara.sendFrame(atCmd,9);
               return 0;
             }
             
             //ly,2011-12-28,add,����жϴ���
             if (wlFlagsSet.wlLcTcpPort== FALSE)
             {
         	 	   if (wlLocalTcpPort>50000)
         	 	   {
         	 	   	 wlLocalTcpPort = 2012;
         	 	   }
         	 	   else
         	 	   {
         	 	   	 wlLocalTcpPort++;
         	 	   }
         	 	   sprintf((char *)atCmd,"AT+CLPORT=\"TCP\",%d\r",wlLocalTcpPort);
         	 	   atType = AT_LC_TCP_PORT;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
         	 	   
         	 	   return 0;
             }

         	   if (wlFlagsSet.ipStarted==FALSE)
         	   {
         	  	 wlFlagsSet.ipStarted = TRUE;
         	  	 
         	 	   strcpy((char *)atCmd,"AT+CIPSTART=\"TCP\",\"");
               strcat((char *)atCmd,loginIpAddr());
         	 	   strcat((char *)atCmd,"\",\"");
         	 	   strcat((char *)atCmd,loginPort());
         	 	   strcat((char *)atCmd,"\"\r");
         	 	   atType = AT_IP_START;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
         	  	 
               return 0;
         	   }
          }
          break;

       case CDMA_DTGS800:
          if (delay > modemPara.delay)
          {
            delay = 0;
          }
          else
          {
         	  delay++;
         	  
            return 0;
          }

         //�����������óɹ�?
         if (wlFlagsSet.ipr == FALSE)
         {
 	          retry++;
 	          if (retry>30)
 	          {
 	          	 modemPara.portConfig(1);
 	          }
 	          strcpy((char *)atCmd,"AT+IPR=115200\r");
 	          atType = AT_IPR;
 	          modemPara.sendFrame(atCmd,14);
            return 0;
         }
         
         //�������δ�ر�
         if (wlFlagsSet.echo == FALSE)
         {
 	          strcpy((char *)atCmd,"ATE0\r");
 	          atType = AT_ECHO;
 	          modemPara.sendFrame(atCmd,5);
            return 0;
         }
         
         //���û�ж���UIM�Ĳ���״̬
         if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<30)
         {
 	          retry++;
            if (retry==30)
            {
 	             modemPara.reportSignal(9,0);
 	          }
 	          strcpy((char *)atCmd,"AT+RLOCK?\r");
 	          atType = AT_UIM;
 	          modemPara.sendFrame(atCmd,10);
 	          
            return 0;
         }
         
         //�Ѳ���UIM��
         if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
         {
         	  if (wlFlagsSet.ainfInputed==FALSE && ipPermit==FALSE)
         	  {
            	  if (wlFlagsSet.dtgsSpc==FALSE)
            	  {
            	 	   strcpy((char *)atCmd,"AT+SPC=\"000000\"\r");
            	 	   atType = AT_SPC;
            	 	   modemPara.sendFrame(atCmd,16);
                   
                   return 0;
            	  }

            	  if (wlFlagsSet.dtgsAinf==FALSE)
            	  {
            	 	   //strcpy((char *)atCmd,"AT+AINF=\"ctnet@mycdma.cn\",\"vnet.mobi\"\r");
            	 	   //strcpy((char *)atCmd,"AT+AINF=\"dlcsm0019@dlcsm.133vpdn.cq\",\"123456\"\r");
            	 	   strcpy((char *)atCmd,"AT+AINF=\"card\",\"card\"\r");
            	 	   /*
            	 	   strcpy((char *)atCmd,"AT+AINF=\"");
                  strcat((char *)atCmd,trim((char *)vpn.vpnName));
                  strcat((char *)atCmd,trim((char *)ipAndPort.apn));   //LY,09-08-13,����
                  strcat((char *)atCmd,"\",\"");
                  strcat((char *)atCmd,trim((char *)vpn.vpnPassword));
                  strcat((char *)atCmd,"\"\r");
                  */
            	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
            	 	   atType = AT_AINF;
                   return 0;
            	  }
            }

         	  if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
         	  {
         	 	   strcpy((char *)atCmd,"AT+CSQ?\r");
         	 	   atType = AT_SIGNAL;
         	 	   modemPara.sendFrame(atCmd,8);
               return 0;
         	  }
         	  
         	  if (wlFlagsSet.dtgsRegNet!=AT_M_RESPONSE_OK)
         	  {
         	 	   strcpy((char *)atCmd,"AT+REGST\r");
         	 	   atType = AT_REG;
         	 	   modemPara.sendFrame(atCmd,9);
               return 0;
         	  }
         	  
         	  if (wlFlagsSet.dtgsTime == FALSE)
         	  {
         	 	   strcpy((char *)atCmd,"AT+TIME?\r");
         	 	   modemPara.sendFrame(atCmd,9);
         	 	   atType = AT_TIME;
               return 0;
         	  }
         	                	  
         	  if (wlFlagsSet.dtgsCrm==FALSE)
         	  {
         	 	   strcpy((char *)atCmd,"AT+CRM=130\r");              	 	   
         	 	   modemPara.sendFrame(atCmd,11);
         	 	   atType = AT_CRM;
               return 0;
         	  }
             
         	  if (wlFlagsSet.dtgsDip==FALSE)
         	  {
         	 	   strcpy((char *)atCmd,"AT+DIP=\"");
         	 	   strcat((char *)atCmd,loginIpAddr());
         	 	   strcat((char *)atCmd,"\"\r");
         	 	   atType = AT_DIP;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }
         	  
         	  if (wlFlagsSet.dtgsDPort==FALSE)
         	  {
         	 	   strcpy((char *)atCmd,"AT+DPORT=\"");
         	 	   strcat((char *)atCmd,loginPort());
         	 	   strcat((char *)atCmd,"\"\r");
         	 	   atType = AT_DPORT;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
         	 	   
               return 0;
         	  }
         	  
         	 	if (wlFlagsSet.dtgsDial==FALSE)
         	 	{
         	 	   strcpy((char *)atCmd,"ATDT#777\r");
                //strcat((char *)atCmd,trim((char *)ipAndPort.apn));
                //strcat((char *)atCmd,"\r");
         	 	   wlFlagsSet.dtgsDial = TRUE;
         	 	   atType = AT_DAILING;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
          	}
         }    	         
         break;
         
       case GPRS_MC52I:
         //���ⷢ��̫��ͨ��ģ����������������е���ʱ         
         if (delay > modemPara.delay)
         {
            delay = 0;
         }
         else
         {
          	 delay++;
         	 	 
         	 	 return 0;
         }
         
         if (wlFlagsSet.callReady==FALSE)
         {
         	  return 0;
         }

         //�������δ�ر�
         if (wlFlagsSet.echo == FALSE)
         {
 	          strcpy((char *)atCmd,"ATE0\r");
 	          atType = AT_ECHO;
 	          modemPara.sendFrame(atCmd,5);
 	          
         	 	return 0;
         }

         //���û�ж���SIM�Ĳ���״̬
         if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<100)
         {
             retry++;
             if (retry==100)
             {
                modemPara.reportSignal(8,0);
             }
             strcpy((char *)atCmd,"AT+CPIN?\r");
             atType = AT_SIM;
             modemPara.sendFrame(atCmd,strlen((char *)atCmd));
             
             return 0;
         }
         
         //�Ѳ���SIM��
         if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
         {
         	  if (wlFlagsSet.mc5xSetCreg==FALSE)
         	  {
              strcpy((char *)atCmd,"AT+CREG=1\r");
              atType = AT_MC5X_SET_CREG;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }
         	  
         	  if (wlFlagsSet.mc5xQueryCreg != AT_M_RESPONSE_OK)
         	  {
              strcpy((char *)atCmd,"AT+CREG?\r");
              atType = AT_MC5X_QUERY_CREG;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
         	  }
          	
          	if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
          	{
          	 	 strcpy((char *)atCmd,"AT+CSQ\r");
          	 	 atType = AT_SIGNAL;
          	 	 modemPara.sendFrame(atCmd,strlen((char *)atCmd));

               return 0;
          	}

         	  if (wlFlagsSet.mc5xSetCGreg==FALSE)
         	  {
              strcpy((char *)atCmd,"AT+CGREG=1\r");
              atType = AT_MC5X_SET_CGREG;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }
         	  
         	  //if (wlFlagsSet.mc5xQueryCGreg != AT_M_RESPONSE_OK)
         	  //{
            //  strcpy((char *)atCmd,"AT+CGREG?\r");
            //  atType = AT_MC5X_QUERY_CGREG;
            //  modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
            //  return 0;
         	  //}
         	  
         	  if (wlFlagsSet.mc5xSelectConn==FALSE)
         	  {
              strcpy((char *)atCmd,"at^sics=0,conType,GPRS0\r");
              atType = AT_MC5X_SELECTT_CONN;
              atWait0d0a = 1;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }
         	  
         	  if (wlFlagsSet.simApn==FALSE)
         	  {
         	 	   strcpy((char *)atCmd,"AT^SICS=0,apn,\"");

               strcat((char *)atCmd,trim(modemPara.apn));
               strcat((char *)atCmd,"\"\r");
         	 	   atType = AT_APN;
               atWait0d0a = 1;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }
         	  
         	  if (wlFlagsSet.mc5xSocket==FALSE)
         	  {
              strcpy((char *)atCmd,"at^siss=1,srvType,socket\r");
              atType = AT_MC5X_SOCKET;
              atWait0d0a = 1;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }

         	  if (wlFlagsSet.mc5xConId==FALSE)
         	  {
              strcpy((char *)atCmd,"at^siss=1,conId,0\r");
              atType = AT_MC5X_CON_ID;
              atWait0d0a = 1;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }

         	  if (wlFlagsSet.mc5xIpPort==FALSE)
         	  { 
         	  	//at^siss=1,address,"socktcp://222.178.86.65:8003"
         	 	  strcpy((char *)atCmd,"at^siss=1,address,\"socktcp://");
              strcat((char *)atCmd,loginIpAddr());
         	 	  strcat((char *)atCmd,":");
         	 	  strcat((char *)atCmd,loginPort());
         	 	  strcat((char *)atCmd,"\"\r");
              
              atType = AT_MC5X_IP_PORT;
              atWait0d0a = 1;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }
         	  
         	  if (wlFlagsSet.ipStarted==FALSE)
         	  {
         	  	 wlFlagsSet.ipStarted=TRUE;
               
               strcpy((char *)atCmd,"AT^SISO=1\r");
               atType = AT_IP_START;
               modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               wlFlagsSet.mc5xDial = TRUE;
               return 0;
         	  }         	  
         }
         break;
         
       case ETHERNET:   //��̫��
       case MODEM_PPP:  //PPP����modem
         if (ipPermit==FALSE)
         {
           //���ⷢ��̫��ͨ��ģ����������������е���ʱ         
           if (delay > modemPara.delay)
           {
             delay = 0;
           }
           else
           {
             delay++;
  
           	 return 0;
           }
           
           if ((*modemPara.fdOfEthSocket)!=0)
           {
             close(*modemPara.fdOfEthSocket);
             
             printf("libWirelessModem:close before fd\n");
           }
           
           //prepare socket
           *modemPara.fdOfEthSocket = socket(AF_INET, SOCK_STREAM, 0);
           setsockopt(*modemPara.fdOfEthSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
           fcntl(*modemPara.fdOfEthSocket,F_SETFL, O_NONBLOCK);

           //prepare server address
           bzero(&servaddr, sizeof(servaddr));
           servaddr.sin_family = AF_INET;
           inet_pton(AF_INET, loginIpAddr(), &servaddr.sin_addr);
           servaddr.sin_port = htons(modemPara.loginPort);
            
           if (modemPara.moduleType==MODEM_PPP)
           {
             printf("libWirelessModem:establish ppp modem socket...\n");
           }
           else
           {
             printf("libWirelessModem:establish ethernet socket...\n");
           }
           
           //go
           if(connect(*modemPara.fdOfEthSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
           {
            	//timeout
            	if(errno == EINPROGRESS)
            	{
                timeout.tv_sec  = TIME_OUT_TIME;
                timeout.tv_usec = 0;
                FD_ZERO(&set);
                FD_SET(*modemPara.fdOfEthSocket, &set);
                if( select(*modemPara.fdOfEthSocket+1, NULL, &set, NULL, &timeout) > 0)
                {
                  getsockopt(*modemPara.fdOfEthSocket, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&lenx);
                  
                  if(error == 0)
                  { 
                    printf("libWirelessModem:Connect Ok\n");
                    	 
                    ipPermit = TRUE;
                       
                    return 1;
                  }
                  else
                  {
                  	//ret = false;
                  }
                }
                else
                { 
                	//ret = false;
                }

            		printf("libWirelessModem:connect time out\n");
            		
            		//˯��1s����������
            		sleep(5);
            	}
            	else
            	{
            	  //����ʧ��
            	  printf("libWirelessModem:connect failur\n");
            	  sleep(1);
            	}
           }
           else
           {
             printf("libWirelessModem:connect success\n");
          	 
             ipPermit = TRUE;
             
             return 1;
           }
         }
         break;
         
       case GPRS_M590E:
         //���ⷢ��̫��ͨ��ģ����������������е���ʱ         
         if (delay > modemPara.delay)
         {
           delay = 0;
         }
         else
         {
           delay++;
         	 	 
         	 return 0;
         }

         if (wlFlagsSet.startup==FALSE)
         {
         	  return 0;
         }

         //�������δ�ر�
         if (wlFlagsSet.echo==FALSE)
         {
 	          strcpy((char *)atCmd,"ATE0\r");
 	          atType = AT_ECHO;
 	          modemPara.sendFrame(atCmd,5);
 	          
         	 	return 0;
         }

         //���û�ж���SIM�Ĳ���״̬
         if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<100)
         {
            retry++;
            if (retry==100)
            {
              modemPara.reportSignal(8,0);
            }
            strcpy((char *)atCmd,"AT+CCID\r");
            atType = AT_SIM;
            modemPara.sendFrame(atCmd,strlen((char *)atCmd));
             
            return 0;
         }
         
         //�Ѳ���SIM��
         if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
         {
         	  if (wlFlagsSet.m590eQueryCreg != AT_M_RESPONSE_OK)
         	  {
              strcpy((char *)atCmd,"AT+CREG?\r");
              atType = AT_REG;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
         	  }
          	
          	if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
          	{
          	 	 strcpy((char *)atCmd,"AT+CSQ\r");
          	 	 atType = AT_SIGNAL;
          	 	 modemPara.sendFrame(atCmd,strlen((char *)atCmd));

               return 0;
          	}
            
            if (wlFlagsSet.m590eXisp==FALSE)
            {
              strcpy((char *)atCmd,"AT+XISP=0\r");
              atType = AT_M590E_XISP;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
            }
            
         	  if (wlFlagsSet.cgdcont == FALSE)
         	  {
              strcpy((char *)atCmd,"AT+CGDCONT=1,\"IP\",\"");
              strcat((char *)atCmd,trim(modemPara.apn));
              strcat((char *)atCmd,"\"\r");
              atType = AT_CGDCONT;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
         	  }
         	  
         	  if (wlFlagsSet.m590eSetXiic == FALSE)
         	  {
              strcpy((char *)atCmd,"AT+XIIC=1\r");
              atType = AT_M590E_SET_XIIC;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
         	  }
         	  
         	  if (wlFlagsSet.m590eQueryXiic != AT_M_RESPONSE_OK)
         	  {
              strcpy((char *)atCmd,"AT+XIIC?\r");
              atType = AT_M590E_QUERY_XIIC;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
         	  }
         	  
         	  if (wlFlagsSet.ipStarted==FALSE)
         	  {
              strcpy((char *)atCmd,"AT+TCPSETUP=0");
              strcat((char *)atCmd,",");
              strcat((char *)atCmd,loginIpAddr());
              strcat((char *)atCmd,",");
              strcat((char *)atCmd,loginPort());
              strcat((char *)atCmd,"\r");
              atType = AT_IP_START;
              modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
              return 0;
            }
         	  
         	  //if (wlFlagsSet.m590eIPStatus!= FALSE)
          	//{ 
         	  //  strcpy((char *)atCmd,"AT+IPSTATUS=0\r");
            //  atType = AT_M590E_IPSTATUS;
            //  modemPara.sendFrame(atCmd,strlen((char *)atCmd));

            //  return 0;
            //}
         }
         break;
         
       case CDMA_CM180:
         if (delay > modemPara.delay)
         {
            delay = 0;
         }
         else
         {
            delay++;
         	  
            return 0;
         }

         if (wlFlagsSet.startup==FALSE)
         {
         	  return 0;
         }
         
         //�������δ�ر�
         if (wlFlagsSet.echo == FALSE)
         {
 	          strcpy((char *)atCmd,"ATE0\r");
 	          //strcpy((char *)atCmd,"AT+CGMR\r");
 	          
 	          atType = AT_ECHO;
 	          modemPara.sendFrame(atCmd,strlen((char *)atCmd));
            return 0;
         }
         
         //�Ѳ���UIM��
         if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
         {
         	  if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
         	  {
         	 	   strcpy((char *)atCmd,"AT+CSQ?\r");
         	 	   atType = AT_SIGNAL;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }
         	  
         	  if (wlFlagsSet.cm180QueryCreg!=AT_M_RESPONSE_OK)
         	  {
         	 	   strcpy((char *)atCmd,"AT+CREG?\r");
         	 	   atType = AT_REG;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }

         	  if (wlFlagsSet.cm180pnum==FALSE)
         	 	{
         	 	   strcpy((char *)atCmd,"AT+PNUM=#777\r");
         	 	   wlFlagsSet.cm180pnum= TRUE;
         	 	   atType = AT_CM180_PNUM;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
          	}

            if (wlFlagsSet.cm180plogin==FALSE)
            { 
            	 strcpy((char *)atCmd,"AT+PLOGIN=");
            	 strcat((char *)atCmd,trim(modemPara.vpnName));
            	 strcat((char *)atCmd,",");
            	 strcat((char *)atCmd,trim(modemPara.vpnPass));
            	 strcat((char *)atCmd,"\r");
            	 modemPara.sendFrame(atCmd,strlen((char *)atCmd));
            	 atType = AT_AINF;
               return 0;
            }

          	if (wlFlagsSet.cm180pppStatus!=AT_M_RESPONSE_OK)
         	  {
         	 	   strcpy((char *)atCmd,"AT+PPPSTATUS\r");
         	 	   atType = AT_CM180_PPPSTATUS;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
         	  }
          	
          	if (wlFlagsSet.cm180Dial==FALSE)
         	 	{
         	 	   strcpy((char *)atCmd,"AT+PPPOPEN\r");
         	 	   atType = AT_DAILING;
         	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
               return 0;
          	}
          	
          	//PPP���ӳɹ���ſ��Խ��к���Ĳ���
            if (wlFlagsSet.dialConnected == TRUE)
            {
              //ly,2013-05-13,add,����жϴ���
              if (wlFlagsSet.wlLcTcpPort== FALSE)
              {
          	 	   if (wlLocalTcpPort>50000)
          	 	   {
          	 	   	 wlLocalTcpPort = 2020;
          	 	   }
          	 	   else
          	 	   {
          	 	   	 wlLocalTcpPort++;
          	 	   	 
          	 	   	 if (wlLocalTcpPort<1024)
          	 	   	 {
          	 	   	 	 wlLocalTcpPort += 1024;
          	 	   	 }
          	 	   }
          	 	   sprintf((char *)atCmd, "AT+TCPPORT=0,%d\r", wlLocalTcpPort);
          	 	   atType = AT_LC_TCP_PORT;
          	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
          	 	   
          	 	   return 0;
              }

              if (wlFlagsSet.ipStarted==FALSE)
         	    {
                strcpy((char *)atCmd,"AT+TCPSETUP=0");
                strcat((char *)atCmd,",");
                strcat((char *)atCmd,loginIpAddr());
                strcat((char *)atCmd,",");
                strcat((char *)atCmd,loginPort());
                strcat((char *)atCmd,"\r");
                atType = AT_IP_START;
                modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              
                return 0;
              }
              
              if (wlFlagsSet.cm180Getip!=AT_M_RESPONSE_OK)
              {                
         	 	    strcpy((char *)atCmd,"AT+GETIP\r");
         	 	    atType = AT_CM180_GETIP;
         	 	    modemPara.sendFrame(atCmd,strlen((char *)atCmd));
                
                return 0;
              }
              
              if (wlFlagsSet.cm180ipStatus!=AT_M_RESPONSE_OK)
              {
         	 	    strcpy((char *)atCmd,"AT+IPSTATUS=0\r");
         	 	    atType = AT_M590E_IPSTATUS;
         	 	    modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              }
            }
         }
         break;
         
       case GPRS_M72D:
         //���ⷢ��̫��ͨ��ģ����������������е���ʱ         
         if (delay > modemPara.delay)
         {
           delay = 0;
         }
         else
         {
           delay++;
         	 	 
         	 return 0;
         }

         //�������δ�ر�
         if (wlFlagsSet.echo == FALSE)
         {
 	         strcpy((char *)atCmd,"ATE0\r");
 	         atType = AT_ECHO;
 	         modemPara.sendFrame(atCmd,5);
 	          
         	 return 0;
         }

         //���û�ж���SIM�Ĳ���״̬
         if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<100)
         {
           retry++;
           if (retry==100)
           {
              modemPara.reportSignal(8,0);
           }
           strcpy((char *)atCmd,"AT+CPIN?\r");
           atType = AT_SIM;
           modemPara.sendFrame(atCmd, strlen((char *)atCmd));
             
           return 0;
         }
         
         if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
         {
          	if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
          	{
          	 	 strcpy((char *)atCmd,"AT+CSQ\r");
          	 	 atType = AT_SIGNAL;
          	 	 modemPara.sendFrame(atCmd,7);

               return 0;
          	}

            if (wlFlagsSet.creg != AT_M_RESPONSE_OK)
            {
            	strcpy((char *)atCmd,"AT+CREG?\r");
            	atType = AT_REG;
            	modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	 	
           	 	return 0;
            }
            
            if (wlFlagsSet.simCgatt != AT_M_RESPONSE_OK)
         	  {
         	    strcpy((char *)atCmd,"AT+CGATT?\r");
         	    atType = AT_CGATT;
           	  modemPara.sendFrame(atCmd,strlen((char *)atCmd));
           	  
           	  //�������ٶ�һ��CSQ
           	  wlFlagsSet.signal = AT_M_RESPONSE_NONE;
           	  return 0;
            }

            if (wlFlagsSet.cgreg != AT_M_RESPONSE_OK)
         	  {
         	    strcpy((char *)atCmd, "AT+CGREG?\r");
         	    atType = AT_CGREG;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72qifgcnt==0)
         	  {
         	    strcpy((char *)atCmd, "AT+QIFGCNT=0\r");
         	    atType = AT_QIFGCNT;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72Apn == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QICSGP=1,\"");
              strcat((char *)atCmd,trim(modemPara.apn));
              strcat((char *)atCmd,"\"\r");
         	    atType = AT_QIAPN;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72mux == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QIMUX=0\r");
         	    atType = AT_QIMUX;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }
            
            if (wlFlagsSet.m72IpMode == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QIMODE=1\r");
         	    atType = AT_QIMODE;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72Itcfg == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QITCFG=3,2,512,1\r");
         	    atType = AT_QITCFG;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72dnsip == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QIDNSIP=0\r");
         	    atType = AT_QIDNSIP;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72regapp == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QIREGAPP\r");
         	    atType = AT_QIREGAPP;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

            if (wlFlagsSet.m72act == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QIACT\r");
         	    atType = AT_QIACT;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }
            
            if (wlFlagsSet.m72locip == 0)
         	  {
         	    strcpy((char *)atCmd, "AT+QILOCIP\r");
         	    atType = AT_QILOCIP;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }
        	  
        	  if (wlFlagsSet.ipStarted==FALSE)
        	  {
        	  	 wlFlagsSet.ipStarted = TRUE;
        	  	 
        	  	 //AT+QIOPEN="TCP","124.74.41.170","5117"
        	 	   strcpy((char *)atCmd,"AT+QIOPEN=\"TCP\",\"");
               strcat((char *)atCmd,loginIpAddr());
        	 	   strcat((char *)atCmd,"\",\"");
        	 	   strcat((char *)atCmd,loginPort());
        	 	   strcat((char *)atCmd,"\"\r");
        	 	   atType = AT_IP_START;
        	 	   modemPara.sendFrame(atCmd,strlen((char *)atCmd));
        	  	 
               return 0;
        	  }
         }
         break;
				 
			 case LTE_AIR720H:
				 //���ⷢ��̫��ͨ��ģ����������������е���ʱ				 
				 if (delay > modemPara.delay)
				 {
					 delay = 0;
				 }
				 else
				 {
					 delay++;
						 
					 return 0;
				 }
			 
				 if (wlFlagsSet.startup==FALSE)
				 {
						return 0;
				 }
			 
				 //�������δ�ر�
				 if (wlFlagsSet.echo==FALSE)
				 {
						strcpy((char *)atCmd,"ATE0\r");
						atType = AT_ECHO;
						wlFlagsSet.atFrame = FRAME_NONE;    //2018-12-26,Add,���,��Ϊģ�����������һЩ����
						modemPara.sendFrame(atCmd,5);
						
						return 0;
				 }
			 
				 //���û�ж���SIM�Ĳ���״̬
				 if (wlFlagsSet.cardInserted != AT_M_RESPONSE_OK && retry<100)
				 {
						retry++;
						if (retry==100)
						{
							modemPara.reportSignal(8,0);
						}
						strcpy((char *)atCmd,"AT+ICCID\r");
						atType = AT_SIM;
						modemPara.sendFrame(atCmd,strlen((char *)atCmd));
						 
						return 0;
				 }
				 
				 //�Ѳ���SIM��
				 if(wlFlagsSet.cardInserted==AT_M_RESPONSE_OK)
				 {
					 if (0==strlen(cops))
					 {
						 strcpy((char *)atCmd,"AT+COPS?\r");
						 atType = AT_COPS;
						 modemPara.sendFrame(atCmd, strlen((char *)atCmd));
						 
						 return 0;
					 }
					 
					 //if (wlFlagsSet.creg != AT_M_RESPONSE_OK)
					 //{
					//	 strcpy((char *)atCmd,"AT+CREG?\r");
					//	 atType = AT_REG;
					//	 modemPara.sendFrame(atCmd,strlen((char *)atCmd));
						 
					//	 return 0;
					// }
						
						if (wlFlagsSet.signal != AT_M_RESPONSE_OK)
						{
							 strcpy((char *)atCmd,"AT+CSQ\r");
							 atType = AT_SIGNAL;
							 modemPara.sendFrame(atCmd,strlen((char *)atCmd));
			 
							 return 0;
						}
						
						if (wlFlagsSet.ipMode==FALSE)
       	    {
       	 	    strcpy((char *)atCmd,"AT+CIPMODE=1\r");
       	 	    atType = AT_IP_MODE;
         	 	  modemPara.sendFrame(atCmd,strlen((char *)atCmd));
              return 0;
         	  }
						
            if (wlFlagsSet.cgreg != AT_M_RESPONSE_OK)
         	  {
         	    strcpy((char *)atCmd, "AT+CGREG?\r");
         	    atType = AT_CGREG;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;
            }

						if (wlFlagsSet.simCgatt!=AT_M_RESPONSE_OK)
						{
         	    strcpy((char *)atCmd, "AT+CGATT?\r");
         	    atType = AT_CGATT;
           	  modemPara.sendFrame(atCmd, strlen((char *)atCmd));
           	  
           	  return 0;						 
						}
						
						if (wlFlagsSet.cgdcont == FALSE)
						{
							strcpy((char *)atCmd,"AT+CSTT=\"");
							strcat((char *)atCmd,trim(modemPara.apn));
							strcat((char *)atCmd,"\",\"\",\"\"\r");
							atType = AT_CGDCONT;
							modemPara.sendFrame(atCmd,strlen((char *)atCmd));
							
							return 0;
						}
						
						if (wlFlagsSet.simCiicr == 0)
						{
							strcpy((char *)atCmd, "AT+CIICR\r");
							atType = AT_CIICR;
							modemPara.sendFrame(atCmd,strlen((char *)atCmd));
							
							return 0;
						}
						
						if (wlFlagsSet.simCifsr == FALSE)
						{
							strcpy((char *)atCmd, "AT+CIFSR\r");
							atType = AT_CIFSR;
							modemPara.sendFrame(atCmd,strlen((char *)atCmd));
							
							return 0;
						}
						
						if (wlFlagsSet.ipStarted==FALSE)
						{
							wlFlagsSet.ipStarted = TRUE;
							
							strcpy((char *)atCmd,"AT+CIPSTART=\"TCP\",\"");
							strcat((char *)atCmd,loginIpAddr());
							strcat((char *)atCmd,"\",\"");
							strcat((char *)atCmd,loginPort());
							strcat((char *)atCmd,"\"\r");
							atType = AT_IP_START;
							modemPara.sendFrame(atCmd,strlen((char *)atCmd));
							
							return 0;
						}
				 }
				 break;

   }
   
   return 0;
}
    
/***************************************************
��������:wirelessReceive
��������:���ߴ��ڽ��պ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
//#define TMP_WL_DEBUG

INT8U wirelessReceive(INT8U *buf,INT16U len)
{
  INT8U  atCmd[60];

  INT16U i,j;
  INT8U  ret=0; 
	char	*pData = NULL;
	char	*pDatax = NULL;
	INT8U tmpData = 0;
		 
	if (LTE_AIR720H ==modemPara.moduleType)
 	{
 		if (TRUE==ipPermit)
		{
			for(i=0;i<len;i++)
			{
				modemPara.pRecvMsBuf[tailOfWireFrame++] =  buf[i];

				if (tailOfWireFrame==1)
        {
     	   	if (modemPara.pRecvMsBuf[0]!=0x68)
         	{
       	  	tailOfWireFrame = 0;
       	  	lenOfWireMsFrame = 2222;
         	}
        }
         
        if (tailOfWireFrame==6)
        {
          if (modemPara.pRecvMsBuf[5]!=0x68)
          {
          	tailOfWireFrame = 0;
          	lenOfWireMsFrame = 2222;

          	break;
          }
          //֡��ʼ,ȡ�����ݷ��������ݵĳ���(length)
          lenOfWireMsFrame = modemPara.pRecvMsBuf[2];
          lenOfWireMsFrame <<= 8;
          lenOfWireMsFrame = lenOfWireMsFrame | modemPara.pRecvMsBuf[1];
          lenOfWireMsFrame = lenOfWireMsFrame >>2;
          
          lenOfWireMsFrame += 8;    //��8,�õ�֡�ܳ���
          
          
          //�������ջ�������С,������֡
          if (lenOfWireMsFrame > SIZE_OF_RECV_MS_BUF)
          {
            tailOfWireFrame = 0;
            lenOfWireMsFrame = 2222;
            tailOfWireless = 0;

            break;
          }
        }
             
        if (tailOfWireFrame==lenOfWireMsFrame)
        {
          atWait0d0a = 1;
          
          modemPara.msInput(modemPara.pRecvMsBuf, lenOfWireMsFrame, DATA_FROM_GPRS);

          lenOfWireMsFrame = 2222;
          tailOfWireFrame = 0;
          tailOfWireless = 0;
        }
			}
		}
		else
		{
			if (AT_CIFSR==atType)
			{
				pData = (char *)buf;
				tmpData = atoi(pData);
				*modemPara.localIp = tmpData<<24;
				pData = strchr((char *)pData+1, '.');
				tmpData = atoi(pData+1);
				*modemPara.localIp |= tmpData<<16;
				pData = strchr((char *)pData+1, '.');
				tmpData = atoi(pData+1);
				*modemPara.localIp |= tmpData<<8;
				pData = strchr((char *)pData+1, '.');
				tmpData = atoi(pData+1);
				*modemPara.localIp |= tmpData;
				
				wlFlagsSet.simCifsr = TRUE;
				atType = AT_NONE;
			}
			else
			{
				if (strstr((char *)buf, "+CPIN: READY\r\n"))
				{
					wlFlagsSet.startup = TRUE;
					atType = AT_NONE;
				}
				
				if (strstr((char *)buf, "+CPIN: SIM REMOVED\r\n"))
				{
					wlFlagsSet.cardInserted = AT_M_RESPONSE_NONE;
				}
				if (strstr((char *)buf, "^CARDMODE: 1\r\n"))//724ģ����֤�ֶθ�Ϊ��� -hgb2021-11-19
				{
					wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
				}
				else if (strstr((char *)buf, "^CARDMODE: 2\r\n"))
				{
					wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
				}				
				if (strstr((char *)buf, "OK\r\n"))
				{
					if (AT_ECHO==atType)
					{
						wlFlagsSet.echo = TRUE;
						
						atType = AT_NONE;
					}
					
					if (AT_CGDCONT==atType)
					{
						wlFlagsSet.cgdcont = TRUE;                                                                                                                                  
						atType = AT_NONE;
					}
					
					if (AT_CIICR==atType)
					{
						wlFlagsSet.simCiicr = 1;
						atType = AT_NONE;
					}
					
					if (AT_IP_MODE==atType)
					{
						wlFlagsSet.ipMode = TRUE;
						atType = AT_NONE;
					}
					
					if (AT_IP_START==atType)
					{
						wlFlagsSet.ipStarted = TRUE;
						atType = AT_NONE;
					}
				}
				
				if (strstr((char *)buf, "+COPS:"))
				{
					pData = strchr((char *)buf, '"');
					pDatax = strchr((char *)(pData+1), '"');
					memcpy(cops, pData+1, pDatax-pData-1);
					cops[pDatax-pData-1] = '\0';
					printf("yys=%s\n", cops);
				}

				if (strstr((char *)buf, "+CREG: "))
				{
					pData = strchr((char *)buf, ',');
					tmpData = atoi(pData+1);
					if (1==tmpData || 5==tmpData)
					{
						wlFlagsSet.creg = AT_M_RESPONSE_OK;
						atType = AT_NONE;
					}
				}
				
				if (strstr((char *)buf, "+CGREG: "))
				{
					pData = strchr((char *)buf, ',');
					tmpData = atoi(pData+1);
					if (1==tmpData || 5==tmpData)
					{
						wlFlagsSet.cgreg = AT_M_RESPONSE_OK;
						atType = AT_NONE;
					}
				}
				
				if (strstr((char *)buf, "+CSQ: "))
				{
					pData = strchr((char *)buf, ' ');
					rssi = atoi(pData+1);

					if (rssi == 99 || rssi>31)
					{
						 modemPara.reportSignal(0, 0);
						 wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
					}
					else
					{
						 modemPara.reportSignal(0, rssi);
						 wlFlagsSet.signal = AT_M_RESPONSE_OK;
					}
					
					atType = AT_NONE;
				}
				
				if (strstr((char *)buf, "+CGATT: "))
				{
					pData = strchr((char *)buf, ' ');
					tmpData = atoi(pData+1);
					if (1==tmpData)
					{
						wlFlagsSet.simCgatt = AT_M_RESPONSE_OK;
						atType = AT_NONE;
					}
				}

				if (strstr((char *)buf, "+CGEV: ME ACT "))
				{
					pData = strchr((char *)buf, '\"');
					pData = strchr((char *)pData+1, '\"');
					pData = strchr((char *)pData+1, '\"');
					pData = strchr((char *)pData+1, '\"');
					pData = strchr((char *)pData+1, '\"');
					tmpData = atoi(pData+1);
					*modemPara.localIp = tmpData<<24;
					pData = strchr((char *)pData+1, '.');
					tmpData = atoi(pData+1);
					*modemPara.localIp |= tmpData<<16;
					pData = strchr((char *)pData+1, '.');
					tmpData = atoi(pData+1);
					*modemPara.localIp |= tmpData<<8;
					pData = strchr((char *)pData+1, '.');
					tmpData = atoi(pData+1);
					*modemPara.localIp |= tmpData;
				}
				
				if (strstr((char *)buf, "\r\nCONNECT\r\n"))
				{
					wlFlagsSet.ipStarted = TRUE;
				  ipPermit = TRUE;
					atType = AT_NONE;
					
          return IP_PERMIT;
				}
			}
		}
 	}
	else
	{
 	  for(i=0;i<len;i++)
 	  {
      if (ipPermit==TRUE)
      {
         #ifndef PRINT_WIRELESS_DEBUG
           wirelessBuf[tailOfWireless++] = modemPara.pRecvMsBuf[tailOfWireFrame++] =  buf[i];
         #else
           wirelessBuf[tailOfWireless++] = tmpData = modemPara.pRecvMsBuf[tailOfWireFrame++] =  buf[i];
           //modemPara.debugFrame(&tmpData,1);
         #endif
         
         //�����·�Ͽ�
         switch (modemPara.moduleType)
         {
         	  case GPRS_GR64:
              if (tailOfWireless==14 && len<500)
              {
                //���GPRS���������ز��ź�(NO CARRIER)?
                for(tmpk=0;tmpk<8;tmpk++)
                {
                  if (wirelessBuf[tmpk] == 0x4e && wirelessBuf[tmpk+1] == 0x4f && wirelessBuf[tmpk+2] == 0x20
                     && wirelessBuf[tmpk+3] == 0x43 && wirelessBuf[tmpk+4] == 0x41 && wirelessBuf[tmpk+5] == 0x52
                     && wirelessBuf[tmpk+6] == 0x52 && wirelessBuf[tmpk+7] == 0x49 && wirelessBuf[tmpk+8]==0x45
                     && wirelessBuf[tmpk+9] == 0x52)
                  {
           	         //��λ��־
           	         //GR64���ߺ�λ��־������resetAllFlag����,ֻ�ø�λ���¼�����־����,ly,2009.06.15
                     wlFlagsSet.echo = FALSE;
                     wlFlagsSet.cardInserted = AT_M_RESPONSE_NONE;
                     rssi = 99;
                     wlFlagsSet.ipStarted = FALSE;
                     
                     ipPermit = FALSE;
               	     ifLogin = 0;
     
                     //ipStartSecond = 0;        //IP��¼ʱ����0
                     //loginSuccess = FALSE;          	    

                     //netStatus |= 0x1;
                     //netStatus &= 0xd;
                     //if (menuInLayer==0 && setParaWaitTime==0xfe)
                     //{
                     //  defaultMenu();
                     //}

                     ret = NO_CARRIER;
           	        
           	         #ifdef PRINT_GPRS_DEBUG
           	           say=0x44;
                       modemPara.debugFrame(&say,1);
                     #endif
                     
                     closeLinkReport();
                     
                     break;
                  }
                }
                tailOfWireless = 0;
              }
              break;
              
            case GPRS_SIM300C:
              if(tailOfWireless==9 && len<500)
              {
                for(tmpk=0;tmpk<5;tmpk++)
                {
                   if (wirelessBuf[tmpk] == 'C' && wirelessBuf[tmpk+1] == 'L' && wirelessBuf[tmpk+2] == 'O'
                      && wirelessBuf[tmpk+3] == 'S' && wirelessBuf[tmpk+4] == 'E')
                   {
               	     //��λ��־
               	     resetAllFlag(0);
               	 	   
               	 	   modemPara.sendFrame((INT8U *)("AT+CIPSHUT\r"),11);
               	 	   modemPara.sendFrame((INT8U *)("AT+CIPSHUT\r"),11);
               	 	   modemPara.sendFrame((INT8U *)("AT+CIPSHUT\r"),11);
               	 	             	  
               	     #ifdef PRINT_GPRS_DEBUG
               	       say=0x45;
                       modemPara.debugFrame(&say,1);
                     #endif
                     
                     closeLinkReport();
                     
                     ret = LINK_DISCONNECT;

                     break;
                   }
                }
                tailOfWireless = 0;
              }
              break;
            
            case CDMA_DTGS800:
              if (tailOfWireless == 14 && len<500)
              {
                for(tmpk=0;tmpk<8;tmpk++)
                {
                  //NO CARRIER
                  if (wirelessBuf[tmpk] == 0x4e && wirelessBuf[tmpk+1] == 0x4f && wirelessBuf[tmpk+2] == 0x20
                     && wirelessBuf[tmpk+3] == 0x43 && wirelessBuf[tmpk+4] == 0x41 && wirelessBuf[tmpk+5] == 0x52
                     && wirelessBuf[tmpk+6] == 0x52 && wirelessBuf[tmpk+7] == 0x49 && wirelessBuf[tmpk+8]==0x45
                     && wirelessBuf[tmpk+9] == 0x52)
                  {
              	      //��λ��־
              	      resetAllFlag(0);
              	      
              	      #ifdef PRINT_GPRS_DEBUG
              	        say=0x44;
                       modemPara.debugFrame(&say,1);
                     #endif
                     
                     ret = NO_CARRIER;
                 
                     break;
                  }
                }
                
                tailOfWireless = 0;
              }
 
            	if (tailOfWireless==12 && len<500)
            	{
                for(tmpk=0;tmpk<5;tmpk++)
                {
                   if (wirelessBuf[tmpk] == 'D' && wirelessBuf[tmpk+1] == 'I' && wirelessBuf[tmpk+2] == 'S'
                   	&& wirelessBuf[tmpk+3] == 'C' && wirelessBuf[tmpk+4] == 'O' && wirelessBuf[tmpk+5] == 'N'
                     && wirelessBuf[tmpk+6] == 'N' && wirelessBuf[tmpk+7] == 'E' && wirelessBuf[tmpk+8] == 'C'
                     && wirelessBuf[tmpk+9] == 'T' && wirelessBuf[tmpk+10] == 0x0d && wirelessBuf[tmpk+11]==0x0a)
                   {
               	    //��λ��־
               	    resetAllFlag(0);
               	     
               	    #ifdef PRINT_GPRS_DEBUG
               	      say=0x45;
                      modemPara.debugFrame(&say,1);
                    #endif
                    
                    ret = LINK_DISCONNECT;
 
                    closeLinkReport();
 
                  	break;
                  }
                }
                
                tailOfWireless = 0;
            	}
            	break;
            
            case GPRS_MC52I:
              //0D 0A 2B 43 53 4D 49 4E-53 3A 20 30 2C 31 0D 0A 
              if (wirelessBuf[tailOfWireless-1]==0x0d)
              {
     	   	      switch (wlFlagsSet.atFrame)
     	   	      {
     	   	      	 case FRAME_NONE:
     	   	  	       #ifdef TMP_WL_DEBUG
     	   	  	        say = 0x11;
     	   	  	        modemPara.debugFrame(&say,1);
     	   	  	       #endif
     	   	  	       
     	   	  	       wlFlagsSet.atFrame = FRAME_START_PREFIX;
     	   	  	       break;
     	   	  	       
     	   	  	     case FRAME_START:
     	   	  	       #ifdef TMP_WL_DEBUG
     	   	  	        say = 0x12;
     	   	  	        modemPara.debugFrame(&say,1);
     	   	  	       #endif
     	   	  	       
     	   	  	       wlFlagsSet.atFrame = FRAME_END_PREFIX;
     	   	  	       break;
    
     	   	  	     case FRAME_START_PREFIX:
     	   	  	       #ifdef TMP_WL_DEBUG
     	   	  	        say = 0x13;
     	   	  	        modemPara.debugFrame(&say,1);
     	   	  	       #endif
     	   	  	       
     	   	  	       tailOfWireless = 1;
     	   	  	       wirelessBuf[0] = 0x0d;
     	   	  	       break;
     	   	      }
     	   	    }
     	   	    else
     	   	    {
       	   	    if (wirelessBuf[tailOfWireless-1]==0x0a)
       	   	    {
       	   	   	  switch(wlFlagsSet.atFrame)
       	   	   	  {
       	   	   	  	 case FRAME_START_PREFIX:
       	   	   	  	   wlFlagsSet.atFrame = FRAME_START;
     	   	  	         
     	   	  	         #ifdef TMP_WL_DEBUG
     	   	  	          say = 0x14;
     	   	  	          modemPara.debugFrame(&say,1);
     	   	  	         #endif
    	   	  	           
    	   	  	           if (atWait0d0a==1)
    	   	  	           {
    	   	  	         	   atWait0d0a = 0;
    	   	  	         	   wlFlagsSet.atFrame=FRAME_NONE;
    	   	  	           }
       	   	   	  	   break;
       	   	   	  	 
       	   	   	  	 case FRAME_END_PREFIX:
     	   	  	         #ifdef TMP_WL_DEBUG
     	   	  	          say = 0x15;
     	   	  	          modemPara.debugFrame(&say,1);
     	   	  	         #endif

       	   	   	  	 	 wlFlagsSet.atFrame = FRAME_END;

                       ret = atResponse(wirelessBuf,tailOfWireless);

                       wlFlagsSet.atFrame = FRAME_NONE;
       	   	   	  	 	 break;
       	   	   	  	 	 
       	   	   	  	 default:
       	   	   	  	 	 wlFlagsSet.atFrame = FRAME_NONE;
     	   	  	         
     	   	  	         #ifdef TMP_WL_DEBUG
     	   	  	          say = 0x16;
     	   	  	          modemPara.debugFrame(&say,1);
     	   	  	         #endif
       	   	   	  	 	 
       	   	   	  	   break;
       	   	   	  }
       	   	    }
       	   	    else
       	   	    {
       	   	   	  //�յ�<CR>(0x0d)��Ӧ���յ�<LF>,�������Ϊ��Ч֡
       	   	   	  if (wirelessBuf[tailOfWireless-1]!=0x0a && (wlFlagsSet.atFrame==FRAME_START_PREFIX || wlFlagsSet.atFrame==FRAME_END_PREFIX))
       	   	   	  {
       	   	   	  	 wlFlagsSet.atFrame = FRAME_NONE;
     	   	  	       #ifdef TMP_WL_DEBUG
     	   	  	        say = 0x17;
     	   	  	        modemPara.debugFrame(&say,1);
     	   	  	       #endif
       	   	   	  }
       	   	    }
       	   	  }
     	   	   
     	        if ((wlFlagsSet.atFrame==FRAME_START) && (tailOfWireless>50))
              {
                 wlFlagsSet.atFrame = FRAME_NONE;
                 tailOfWireless = 0;
     	   	  	  
     	   	  	  #ifdef TMP_WL_DEBUG
     	   	  	   say = 0x18;
     	   	  	   modemPara.debugFrame(&say,1);
     	   	  	  #endif
              }
    
              //�����֡��ʼ�ȱ�־(��֡),�������ֽ�
              if (wlFlagsSet.atFrame==FRAME_NONE)
              {
                 tailOfWireless = 0;
     	   	  	  
     	   	  	  #ifdef TMP_WL_DEBUG
     	   	  	    say = 0x19;
     	   	  	    modemPara.debugFrame(&say,1);
     	   	  	  #endif
              }
            	break;
            	
            case GPRS_M590E:            	
            	if (wlFlagsSet.wlSendStatus==2)
            	{
            		//������,����Ƿ���">"(0d 0a 3e)
            		for(j=0;j<tailOfWireless;j++)
            		{
            		  if (wirelessBuf[j]==0x0d && wirelessBuf[j+1]==0x0a && wirelessBuf[j+2]==0x3e)
            		  {
            			   wlFlagsSet.wlSendStatus=3;
            			 
            			   tailOfWireless = 0;
            			   break;
            		  }
            		}
            	}
            	else
            	{
                //(+TCPCLOSED:0,Link Closed)
                if(tailOfWireless>=27 && len<500)
                {
                  for(tmpk=0;tmpk<(tailOfWireless-10);tmpk++)
                  {
                     if (wirelessBuf[tmpk] == 'L' && wirelessBuf[tmpk+1] == 'i' && wirelessBuf[tmpk+2] == 'n' 
                     	   && wirelessBuf[tmpk+3] == 'k' && wirelessBuf[tmpk+5] == 'C' && wirelessBuf[tmpk+6] == 'l' 
                     	    && wirelessBuf[tmpk+7] == 'o' && wirelessBuf[tmpk+8] == 's' && wirelessBuf[tmpk+9] == 'e' 
                     	     && wirelessBuf[tmpk+10] == 'd'
                        )
                     {
                 	     //��λ��־
                 	     resetAllFlag(0);
                       
                       closeLinkReport();
                       
                       ret = LINK_DISCONNECT;
                       
                       tailOfWireless = 0;

                       tailOfWireFrame = 0;
                       break;
                     }
                  }
                }                
            	}
            	break;
            	
            case CDMA_CM180: 
              //+TCPCLOSED:0
              //0d 0a 2b 54 43 50 43 4c 4f 53 45 44 3a 30 0d 0a 
              if(tailOfWireless>=16 && len<500)
              {
                 for(tmpk=0;tmpk<tailOfWireless;tmpk++)
                 {
                   if (wirelessBuf[tmpk] == 0x2b && wirelessBuf[tmpk+1] == 0x54 && wirelessBuf[tmpk+2] == 0x43
                     	 && wirelessBuf[tmpk+3] == 0x50 && wirelessBuf[tmpk+4] == 0x43 && wirelessBuf[tmpk+5] == 0x4c
                     	  && wirelessBuf[tmpk+6] == 0x4f && wirelessBuf[tmpk+7] == 0x53 && wirelessBuf[tmpk+8] == 0x45
                     	   && wirelessBuf[tmpk+9] == 0x44 && wirelessBuf[tmpk+10] == 0x3a && wirelessBuf[tmpk+11] == 0x30
                     	)
                   {
     	               strcpy((char *)atCmd," AT+PPPCLOSE\r");
     	               modemPara.sendFrame(atCmd,strlen((char *)atCmd));
     	               modemPara.sendFrame(atCmd,strlen((char *)atCmd));
                 	   
                 	   //��λ��־
                 	   resetAllFlag(0);                       

                     closeLinkReport();
                       
                     ret = LINK_DISCONNECT;
                       
                     tailOfWireless = 0;

                     tailOfWireFrame = 0;
                   }
                 }
              }
            	break;
         }
         
         if (tailOfWireFrame==1)
         {
       	   if (modemPara.pRecvMsBuf[0]!=0x68)
           {
         	   tailOfWireFrame = 0;
         	   lenOfWireMsFrame = 2222;
           }
         }
         
         if (tailOfWireFrame==6)
         {
            if (modemPara.pRecvMsBuf[5]!=0x68)
            {
            	 tailOfWireFrame = 0;
            	 lenOfWireMsFrame = 2222;

            	 break;
            }
            //֡��ʼ,ȡ�����ݷ��������ݵĳ���(length)
            lenOfWireMsFrame = modemPara.pRecvMsBuf[2];
            lenOfWireMsFrame <<= 8;
            lenOfWireMsFrame = lenOfWireMsFrame | modemPara.pRecvMsBuf[1];
            lenOfWireMsFrame = lenOfWireMsFrame >>2;
            
            lenOfWireMsFrame += 8;    //��8,�õ�֡�ܳ���
            
            
            //�������ջ�������С,������֡
            if (lenOfWireMsFrame > SIZE_OF_RECV_MS_BUF)
            {
               tailOfWireFrame = 0;
               lenOfWireMsFrame = 2222;
               tailOfWireless = 0;

               break;
            }
         }
             
         if (tailOfWireFrame==lenOfWireMsFrame)
         {
            atWait0d0a = 1;
            
            modemPara.msInput(modemPara.pRecvMsBuf, lenOfWireMsFrame, DATA_FROM_GPRS);

            lenOfWireMsFrame = 2222;
            tailOfWireFrame = 0;
            tailOfWireless = 0;
         }
      }
      else
      { 	      
 	      if (modemPara.moduleType==CDMA_DTGS800 
 	      	  || modemPara.moduleType==GPRS_SIM300C 
 	      	    || modemPara.moduleType==GPRS_MC52I 
 	      	      || modemPara.moduleType==GPRS_M590E 
 	      	        || modemPara.moduleType==CDMA_CM180
 	      	          || modemPara.moduleType==GPRS_M72D
 	      	            || modemPara.moduleType==MODEM_PPP
											 || modemPara.moduleType==LTE_AIR720H
 	      	 )
 	      {
           if (wlFlagsSet.dtgsDial==TRUE)  //���������ѷ���
           {
             //43 41 4C 4C (CALL)
             //0D 0A 50 50 50 0D 0A (PPP)
             //43 4F 4E 4E 45 43 54 0D 0A(CONNECT)
 
           	 #ifndef PRINT_WIRELESS_DEBUG
           	  wirelessBuf[tailOfWireless++] = buf[i];
           	 #else           	
           	  wirelessBuf[tailOfWireless] = buf[i];
           	  //modemPara.debugFrame(&wirelessBuf[tailOfWireless],1);
           	  tailOfWireless++;
           	 #endif
           	
             for(tmpk=0;tmpk<tailOfWireless;tmpk++) 
             {
               //DISCONNECT
               if (wirelessBuf[tmpk] == 'D' && wirelessBuf[tmpk+1] == 'I' && wirelessBuf[tmpk+2] == 'S'
               	&& wirelessBuf[tmpk+3] == 'C' && wirelessBuf[tmpk+4] == 'O' && wirelessBuf[tmpk+5] == 'N'
                 && wirelessBuf[tmpk+6] == 'N' && wirelessBuf[tmpk+7] == 'E' && wirelessBuf[tmpk+8] == 'C'
                 && wirelessBuf[tmpk+9] == 'T' && wirelessBuf[tmpk+10] == 0x0d && wirelessBuf[tmpk+11]==0x0a)
               {
               	  //��λ��־
               	  resetAllFlag(1);

               	  #ifdef PRINT_GPRS_DEBUG
               	    say=0x45;
                    modemPara.debugFrame(&say,1);
                  #endif
                  
                  ret = LINK_DISCONNECT;
                  
                  tailOfWireless = 0;
                  
                  goto breakPointMsR;
               }
             }
 
             for(tmpk=0;tmpk<tailOfWireless;tmpk++)
             {
               //CONNECT
               if (wirelessBuf[tmpk] == 'C' && wirelessBuf[tmpk+1] == 'O' && wirelessBuf[tmpk+2] == 'N'
                 && wirelessBuf[tmpk+3] == 'N'  && wirelessBuf[tmpk+4] == 'E' && wirelessBuf[tmpk+5] == 'C'
                 && wirelessBuf[tmpk+6] == 'T' && wirelessBuf[tmpk+7] == 0x0d && wirelessBuf[tmpk+8]==0x0a)
               {
   	              //��λ��־
   	              ipPermit = TRUE;
   	              ret = IP_PERMIT;
   	              
   	              #ifdef PRINT_GPRS_DEBUG
   	                say=0x3f;
                    modemPara.debugFrame(&say,1);
                  #endif
                  
                  tailOfWireless = 0;
                  
                  break;
               }
             }
           }
           else
           { 
             wirelessBuf[tailOfWireFrame] = buf[i];
             
             #ifdef PRINT_WIRELESS_DEBUG
              //modemPara.debugFrame(&wirelessBuf[tailOfWireFrame],1);
             #endif
        	   
        	   if (tailOfWireFrame>=SIZE_OF_WIRELESS)
        	   {
        	   	 tailOfWireFrame = 0;
        	   }
        	   else
        	   {
        	     tailOfWireFrame++;
        	   }
             
             
             if ((wlFlagsSet.callReady==FALSE || wlFlagsSet.mc5xDial==TRUE)  && modemPara.moduleType==GPRS_MC52I)
             {
                //5e 53 59 53 53 54 41 52 54 0d 0a(^SYSSTART)
                for(tmpk=0;tmpk<tailOfWireFrame;tmpk++)
           	    {
           	  	   if (wirelessBuf[tmpk+0]==0x5e && wirelessBuf[tmpk+1]==0x53 && wirelessBuf[tmpk+2]==0x59 && wirelessBuf[tmpk+3]==0x53
           	  	 	   && wirelessBuf[tmpk+4]==0x53 && wirelessBuf[tmpk+5]==0x54 && wirelessBuf[tmpk+6]==0x41 && wirelessBuf[tmpk+7]==0x52
           	  	 	   && wirelessBuf[tmpk+8]==0x54 && wirelessBuf[tmpk+9]==0x0d && wirelessBuf[tmpk+10]==0x0a)
           	  	   {
           	  	 	    wlFlagsSet.callReady = TRUE;
                      wlFlagsSet.atFrame = FRAME_NONE;
                      tailOfWireFrame = 0;
                      
                      ret = 0;
                      goto breakPointMsR;
                   }
                }
                
                //0d 0a 5e 53 49 53 57 3a 20 31 2c 20 31 0d 0a(^SISW: 1, 1)
                
                for(tmpk=0;tmpk<tailOfWireFrame;tmpk++)
         	      {
         	  	    if (wirelessBuf[tmpk+0]==0x5e && wirelessBuf[tmpk+1]==0x53 && wirelessBuf[tmpk+2]==0x49 && wirelessBuf[tmpk+3]==0x53
         	    	 	    && wirelessBuf[tmpk+4]==0x57 && wirelessBuf[tmpk+5]==0x3a && wirelessBuf[tmpk+6]==0x20 && wirelessBuf[tmpk+7]==0x31
         	  	 	      && wirelessBuf[tmpk+8]==0x2c && wirelessBuf[tmpk+9]==0x20)
         	  	    {
                    if (wlFlagsSet.mc5xDial==TRUE)
                    {
                      #ifdef PRINT_WIRELESS_DEBUG
                       say = 0x81;
                       modemPara.debugFrame(&say, 1);
                      #endif
                      
                      wlFlagsSet.mc5xDial = FALSE;
                      
                      //��λ��־
   	                  ipPermit = TRUE;
   	                  ret = IP_PERMIT;
   	                  tailOfWireFrame = 0;
                    }
                    goto breakPointMsR;
                  }
                }              
             }
             else
             {
               //0D 0A 2B 43 53 4D 49 4E-53 3A 20 30 2C 31 0D 0A 
               if (wirelessBuf[tailOfWireFrame-1]==0x0d)
               {
       	   	      switch (wlFlagsSet.atFrame)
       	   	      {
       	   	      	 case FRAME_NONE:
       	   	  	       #ifdef TMP_WL_DEBUG
       	   	  	        say = 0x11;
       	   	  	        modemPara.debugFrame(&say,1);
       	   	  	       #endif
       	   	  	       
       	   	  	       wlFlagsSet.atFrame = FRAME_START_PREFIX;
       	   	  	       break;
       	   	  	       
       	   	  	     case FRAME_START:
       	   	  	       #ifdef TMP_WL_DEBUG
       	   	  	        say = 0x12;
       	   	  	        modemPara.debugFrame(&say,1);
       	   	  	       #endif
       	   	  	       
       	   	  	       wlFlagsSet.atFrame = FRAME_END_PREFIX;
       	   	  	       break;
      
       	   	  	     case FRAME_START_PREFIX:
       	   	  	       #ifdef TMP_WL_DEBUG
       	   	  	        say = 0x13;
       	   	  	        modemPara.debugFrame(&say,1);
       	   	  	       #endif
       	   	  	       
       	   	  	       tailOfWireFrame = 1;
       	   	  	       wirelessBuf[0] = 0x0d;
       	   	  	       break;
       	   	      }
       	   	   }
       	   	   else
       	   	   {
         	   	   if (wirelessBuf[tailOfWireFrame-1]==0x0a)
         	   	   {
         	   	   	  switch(wlFlagsSet.atFrame)
         	   	   	  {
         	   	   	  	 case FRAME_START_PREFIX:
         	   	   	  	   wlFlagsSet.atFrame = FRAME_START;

       	   	  	         #ifdef TMP_WL_DEBUG
       	   	  	          say = 0x14;
       	   	  	          modemPara.debugFrame(&say,1);
       	   	  	         #endif
       	   	  	         
       	   	  	         if (atWait0d0a == 1)
       	   	  	         {
       	   	  	         	 atWait0d0a = 0;
       	   	  	         	 wlFlagsSet.atFrame=FRAME_NONE;
       	   	  	         }
         	   	   	  	   break;
         	   	   	  	 
         	   	   	  	 case FRAME_END_PREFIX:
       	   	  	         #ifdef TMP_WL_DEBUG
       	   	  	          say = 0x15;
       	   	  	          modemPara.debugFrame(&say,1);
       	   	  	         #endif
  
         	   	   	  	 	 wlFlagsSet.atFrame = FRAME_END;
  
                         ret = atResponse(wirelessBuf,tailOfWireFrame);
                         
                         if (ret==LINK_FAIL)
                         {
                         	 resetAllFlag(0);
                         	 
                         	 goto breakPointMsR;
                         }
  
                         wlFlagsSet.atFrame = FRAME_NONE;
         	   	   	  	 	 break;
         	   	   	  	 	 
         	   	   	  	 default:
         	   	   	  	 	 wlFlagsSet.atFrame = FRAME_NONE;
       	   	  	         
       	   	  	         #ifdef TMP_WL_DEBUG
       	   	  	          say = 0x16;
       	   	  	          modemPara.debugFrame(&say,1);
       	   	  	         #endif
         	   	   	  	 	 
         	   	   	  	   break;
         	   	   	  }
         	   	   }
         	   	   else
         	   	   {
         	   	   	  //�յ�<CR>(0x0d)��Ӧ���յ�<LF>,�������Ϊ��Ч֡
         	   	   	  if (wirelessBuf[tailOfWireFrame-1]!=0x0a && (wlFlagsSet.atFrame==FRAME_START_PREFIX || wlFlagsSet.atFrame==FRAME_END_PREFIX))
         	   	   	  {
         	   	   	  	 wlFlagsSet.atFrame = FRAME_NONE;
       	   	  	       #ifdef TMP_WL_DEBUG
       	   	  	        say = 0x17;
       	   	  	        modemPara.debugFrame(&say,1);
       	   	  	       #endif
         	   	   	  }
         	   	   }
         	   	 }
       	   	   
       	       if ((wlFlagsSet.atFrame==FRAME_START) && (tailOfWireFrame>50))
               {
                  wlFlagsSet.atFrame = FRAME_NONE;
       	   	  	  
       	   	  	  #ifdef TMP_WL_DEBUG
       	   	  	   say = 0x18;
       	   	  	   modemPara.debugFrame(&say,1);
       	   	  	  #endif
               }
      
               //�����֡��ʼ�ȱ�־(��֡),�������ֽ�
               if (wlFlagsSet.atFrame==FRAME_NONE)
               {
                  tailOfWireFrame = 0;

       	   	  	  #ifdef TMP_WL_DEBUG
       	   	  	   say = 0x19;
       	   	  	   modemPara.debugFrame(&say,1);
       	   	  	  #endif
               }
             }
           }
 	      }
 	      else
 	      {
 	         wirelessBuf[tailOfWireless++] = buf[i];
 	      }
      }
    }
	}

breakPointMsR:
   if (modemPara.moduleType==CDMA_DTGS800 || modemPara.moduleType==GPRS_SIM300C 
   	   || modemPara.moduleType==GPRS_MC52I ||  modemPara.moduleType==GPRS_M590E 
   	     || modemPara.moduleType==CDMA_CM180  || modemPara.moduleType==GPRS_SIM900A 
   	       || modemPara.moduleType==GPRS_M72D 
   	         || modemPara.moduleType==MODEM_PPP 
		 					|| modemPara.moduleType==LTE_AIR720H 
   	           || ipPermit==TRUE
   	  )
   {
   	  return ret;
   }

   switch(modemPara.moduleType)
   {
   	  case GPRS_GR64:      //����GR64
   	  	switch(atType)
   	  	{
   	  		 case AT_ECHO:          //�رջ���
           case AT_CGDCONT:       //PDP
           case AT_GR64_IPA:      //IPA
           case AT_GR64_IPS:      //IPS
           case AT_GR64_ADDR:
             if (wirelessBuf[tailOfWireless-4] == 0x4f && wirelessBuf[tailOfWireless-3] == 0x4B
             	  && wirelessBuf[tailOfWireless-2] == 0x0d && wirelessBuf[tailOfWireless-1]==0x0a)
             {
                if (atType == AT_GR64_ADDR)
                {
                   for(tmpk=0;tmpk<12;tmpk++)
                   {
                   	  if (wirelessBuf[tmpk]==0x2b && wirelessBuf[tmpk+1]==0x43 && wirelessBuf[tmpk+2]==0x47
                   	  	  && wirelessBuf[tmpk+3]==0x50 && wirelessBuf[tmpk+4]==0x41 && wirelessBuf[tmpk+5]==0x44
                   	  	  && wirelessBuf[tmpk+6]==0x44 && wirelessBuf[tmpk+7]==0x52 && wirelessBuf[tmpk+8]==0x3a)
                   	  {
   	                  	  wlFlagsSet.gr64Addr = TRUE;

                   	  	  tmpk += 13;

                       	  *modemPara.localIp = 0x0;
                       	  
                       	  //IP��ַ��һ�ֽ�
                       	  if (wirelessBuf[tmpk+1]==0x2e)    //ֻ��һλ��
                       	  {
                       	     *modemPara.localIp = (wirelessBuf[tmpk]-0x30)<<24;
                       	     tmpk+=2;
                       	  }
                       	  else
                       	  {
                          	 if (wirelessBuf[tmpk+2]==0x2e)    //ֻ��һλ��
                         	   {
                       	        *modemPara.localIp = ((wirelessBuf[tmpk]-0x30)*10+wirelessBuf[tmpk+1]-0x30)<<24;
                       	        tmpk+=3;
                       	     }
                       	     else
                       	     {
                       	        *modemPara.localIp = ((wirelessBuf[tmpk]-0x30)*100+(wirelessBuf[tmpk+1]-0x30)*10+wirelessBuf[tmpk+2]-0x30)<<24;
                       	        tmpk+=4;
                       	     }
                       	  }
             
                       	  //IP��ַ�ڶ��ֽ�
                       	  if (wirelessBuf[tmpk+1]==0x2e)    //ֻ��һλ��
                       	  {
                       	     *modemPara.localIp |= (wirelessBuf[tmpk]-0x30)<<16;
                       	     tmpk+=2;
                       	  }
                       	  else
                       	  {
                          	 if (wirelessBuf[tmpk+2]==0x2e)    //ֻ��һλ��
                         	   {
                       	        *modemPara.localIp |= ((wirelessBuf[tmpk]-0x30)*10+wirelessBuf[tmpk+1]-0x30)<<16;
                       	        tmpk+=3;
                       	     }
                       	     else
                       	     {
                       	        *modemPara.localIp |= ((wirelessBuf[tmpk]-0x30)*100+(wirelessBuf[tmpk+1]-0x30)*10+wirelessBuf[tmpk+2]-0x30)<<16;
                       	        tmpk+=4;
                       	     }
                       	  }
                       	  //IP��ַ�����ֽ�
                       	  if (wirelessBuf[tmpk+1]==0x2e)    //ֻ��һλ��
                       	  {
                       	     *modemPara.localIp |= (wirelessBuf[tmpk]-0x30)<<8;
                       	     tmpk+=2;
                       	  }
                       	  else
                       	  {
                          	 if (wirelessBuf[tmpk+2]==0x2e)    //ֻ��һλ��
                         	   {
                       	        *modemPara.localIp |= ((wirelessBuf[tmpk]-0x30)*10+wirelessBuf[tmpk+1]-0x30)<<8;
                       	        tmpk+=3;
                       	     }
                       	     else
                       	     {
                       	        *modemPara.localIp |= ((wirelessBuf[tmpk]-0x30)*100+(wirelessBuf[tmpk+1]-0x30)*10+wirelessBuf[tmpk+2]-0x30)<<8;
                       	        tmpk+=4;
                       	     }
                       	  }
             
                       	  //IP��ַ�����ֽ�
                       	  if (wirelessBuf[tmpk+1]==0x22 && wirelessBuf[tmpk+2]==0x0d && wirelessBuf[tmpk+3]==0x0a)    //ֻ��һλ��
                       	  {
                       	     *modemPara.localIp |= (wirelessBuf[tmpk]-0x30);
                       	     tmpk+=2;
                       	  }
                       	  else
                       	  {
                          	 if (wirelessBuf[tmpk+2]==0x22 && wirelessBuf[tmpk+3]==0x0d && wirelessBuf[tmpk+4]==0x0a)    //ֻ��һλ��
                         	   {
                       	        *modemPara.localIp |= ((wirelessBuf[tmpk]-0x30)*10+wirelessBuf[tmpk+1]-0x30);
                       	        tmpk+=3;
                       	     }
                       	     else
                       	     {
                       	        *modemPara.localIp |= ((wirelessBuf[tmpk]-0x30)*100+(wirelessBuf[tmpk+1]-0x30)*10+wirelessBuf[tmpk+2]-0x30);
                       	        tmpk+=4;
                       	     }
                       	  }
                          
                          #ifdef PRINT_GPRS_DEBUG
                            say = 0x13;
                            modemPara.debugFrame(&say,1);
                          #endif
                           
                          break;
                      }
                   }

                	 tailOfWireless = 0;
                }
                else
                {
                   for(tmpk=0;tmpk<tailOfWireless-3;tmpk++)
                   {
                     if (wirelessBuf[tmpk]==0x4F && wirelessBuf[tmpk+1]==0x4B
                       && wirelessBuf[tmpk+2]== 0x0D && wirelessBuf[tmpk+3]==0x0A)
                     {
                        switch(atType)
                        {
                           case AT_ECHO:
                             wlFlagsSet.echo = TRUE;        //�����ѹر�! 
                             
                             #ifdef PRINT_GPRS_DEBUG
                               say=0x05;
                               modemPara.debugFrame(&say,1);
                             #endif
                             break;
                             
                           case AT_CGDCONT:   //PDP
                             wlFlagsSet.cgdcont = TRUE;
                             
                             #ifdef PRINT_GPRS_DEBUG
                               say=0x10;
                               modemPara.debugFrame(&say,1);
                             #endif
                             break;
                             
                          case AT_GR64_IPA:
                             wlFlagsSet.gr64Ipa = TRUE;
                             #ifdef PRINT_GPRS_DEBUG
                               say=0x12;
                               modemPara.debugFrame(&say,1);
                             #endif
                             break;
   
                          case AT_GR64_IPS:
                             wlFlagsSet.gr64Ips = TRUE;
                             #ifdef PRINT_GPRS_DEBUG
                               say=0x11;
                               modemPara.debugFrame(&say,1);
                             #endif
                             break;
                        }
                        atType = AT_NONE;
                        tailOfWireless = 0;
                     }
                   }
                }
   	  		 	 }
   	  		   break;
   	  		   
           case AT_SIM:
           	 if (tailOfWireless>=9)
           	 {
           	 	  for(tmpk=0;tmpk<tailOfWireless;tmpk++)
           	 	  {
           	 	  	  //�Ѳ��뿨
           	 	  	  if (wirelessBuf[tmpk]=='*' && wirelessBuf[tmpk+1] == 'E' && wirelessBuf[tmpk+2]=='2'
           	 	  	  	 && wirelessBuf[tmpk+3] == 'S' && wirelessBuf[tmpk+4] == 'S' && tmpk+24 <=tailOfWireless)
           	 	  	  {
                       #ifdef PRINT_GPRS_DEBUG
                         say=0x20;
                         modemPara.debugFrame(&say,1);
                       #endif
                       
                       wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                       tailOfWireless = 0;
                       atType = AT_NONE;
                       break;
           	 	  	  }
           	 	  	  
           	 	  	  //δ����SIM��
           	 	  	  if (wirelessBuf[tmpk]=='E' && wirelessBuf[tmpk+1] == 'R' && wirelessBuf[tmpk+2]=='R' 
           	 	  	  	 && wirelessBuf[tmpk+3] == 'O' && wirelessBuf[tmpk+4] == 'R')
           	 	  	  {                       
                       #ifdef PRINT_GPRS_DEBUG
                         say=0x24;
                         modemPara.debugFrame(&say,1);
                       #endif
                       
                       tailOfWireless = 0;
                       atType = AT_NONE;
                       break;
           	 	  	  }
           	 	  }
           	 }
           	 break;

           case AT_SIGNAL:     //�ź�����
             if (wirelessBuf[tailOfWireless-4] == 0x4f && wirelessBuf[tailOfWireless-3] == 0x4B
             	  && wirelessBuf[tailOfWireless-2] == 0x0d && wirelessBuf[tailOfWireless-1]==0x0a)
             {
               for(tmpk=0;tmpk<tailOfWireless;tmpk++)
               {
                 if (wirelessBuf[tmpk]==0x2B && wirelessBuf[tmpk+1]==0x43 && wirelessBuf[tmpk+2]== 0x53 
                   && wirelessBuf[tmpk+3]==0x51 && wirelessBuf[tmpk+4]== 0x3A && wirelessBuf[tmpk+5]==0x20)
                 {
                    if (wirelessBuf[tmpk+7]==0x2c)
                    {
                       rssi = wirelessBuf[tmpk+6]-0x30;
                       
                       if (wirelessBuf[tmpk+9]==0x0D)
                         bitErr = wirelessBuf[tmpk+8]-0x30;
                       else
                       {
                         bitErr = (wirelessBuf[tmpk+8]-0x30)*10 + (wirelessBuf[tmpk+9]-0x30);
                       }
                    }
                    else
                    {
                       rssi = (wirelessBuf[tmpk+6]-0x30)*10+(wirelessBuf[tmpk+7]-0x30);
                       if (wirelessBuf[tmpk+10]==0x0D)
                         bitErr = wirelessBuf[tmpk+9]-0x30;
                       else
                       {
                         bitErr = (wirelessBuf[tmpk+9]-0x30)*10 + (wirelessBuf[tmpk+10]-0x30);
                       }
                    }

                    if (rssi == 99 || rssi>31)
                    {
                  	   modemPara.reportSignal(0,0);
                    }
                    else
                    {
                       modemPara.reportSignal(0,rssi);
                    }
                    
                    atType = AT_NONE;
                    tailOfWireless = 0;

                    #ifdef PRINT_GPRS_DEBUG
                       say=0x08;
                       modemPara.debugFrame(&say,1);
                    #endif
                    break;
                 }
               }
             }
             break;
           	 
           case AT_IP_START:
             for(tmpk=0;tmpk<10;tmpk++)
             {
                if (wirelessBuf[tmpk] == 'C' && wirelessBuf[tmpk+1] == 'O' && wirelessBuf[tmpk+2] == 'N'
                    && wirelessBuf[tmpk+3] == 'N' && wirelessBuf[tmpk+4] == 'E' && wirelessBuf[tmpk+5] == 'C'
                    && wirelessBuf[tmpk+6] == 'T')
                {                     
                   ipPermit = TRUE;           //TCP/IP���ӳɹ� 	 	                
                   //if (menuInLayer==0 && setParaWaitTime==0xfe)
                   //{
                   //  defaultMenu();
                   //}
                   atType = AT_NONE;
                   tailOfWireless = 0;

                   #ifdef PRINT_GPRS_DEBUG
                	   say = 0x16;
                     modemPara.debugFrame(&say,1);
                   #endif

                   break;
                }
             }             
             break;
   	  	}
   	  	break;
   }
}

/***************************************************
��������:atResponse
��������:AT����Ӧ��֡������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
INT8U atResponse(INT8U *pFrame,INT8U len)
{
   INT8U atCmd[10];
   INT8U ret=MODEM_RET_FRAME;
	 char  *pData = NULL;
	 INT8U tmpData = 0;
	 
   #ifdef PRINT_WIRELESS_DEBUG
    INT8U say;
    modemPara.debugFrame(pFrame,len);

    INT8U i;
    printf("atResponse:");
    for(i=0;i<len;i++)
    {
      printf("%02x ",pFrame[i]);
    }
    printf("\n");
    
   #endif
   
   switch(modemPara.moduleType)
   {
   	  case GPRS_SIM300C:
        if (atType == AT_CIFSR)
        {
          //��ȡ����IP��ַ
          //0D 0A 31 30 2E 31 39 37 2E 31 31 2E 31 33 39 0D 0A 
          wlFlagsSet.simCifsr = TRUE;

          pFrame += 2;   //tmpk = 2;

      	  *modemPara.localIp = 0x0;
      	  
      	  //IP��ַ��һ�ֽ�
      	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp = (*pFrame -0x30)<<24;
      	     pFrame+=2;
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
        	   {
      	        *modemPara.localIp = ((*pFrame-0x30)*10 + *(pFrame+1)-0x30)<<24;
      	        pFrame+=3;
      	     }
      	     else
      	     {
      	        *modemPara.localIp = ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<24;
      	        pFrame+=4;
      	     }
      	  }

      	  //IP��ַ�ڶ��ֽ�
      	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp |= (*pFrame-0x30)<<16;
      	     pFrame+=2;
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
        	   {
      	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<16;
      	        pFrame+=3;
      	     }
      	     else
      	     {
      	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<16;
      	        pFrame+=4;
      	     }
      	  }
      	  
      	  //IP��ַ�����ֽ�
      	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp |= (*pFrame-0x30)<<8;
      	     pFrame += 2;
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
        	   {
      	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<8;
      	        pFrame += 3;
      	     }
      	     else
      	     {
      	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<8;
      	        pFrame += 4;
      	     }
      	  }

      	  //IP��ַ�����ֽ�
      	  if (*(pFrame+1)==0x0d && *(pFrame+2)==0x0a)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp |= (*pFrame-0x30);
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x0d && *(pFrame+3)==0x0a)    //ֻ��һλ��
        	   {
      	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30);
      	     }
      	     else
      	     {
      	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30);
      	     }
      	  }
      	  
          #ifdef PRINT_WIRELESS_DEBUG
            say = 0x0c;
            modemPara.debugFrame(&say,1);
          #endif
        
          atType = AT_NONE;
        }
        else
        {
          switch (len)
          {
          	 case 6:
          	 	 //0D 0A 4F 4B 0D 0A(OK)
          	 	 if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
          	 	 {
          	 	 	  switch(atType)
          	 	 	  {
                     case AT_ECHO:        //�رջ���
                     	 wlFlagsSet.echo = TRUE;
                       
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x02;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
                     
                     case AT_SIM:         //�Ƿ�忨
                     	 if (wlFlagsSet.cardInserted==AT_M_RESPONSE_INFO_ACK)
                     	 {
                     	 	  wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                     	 }
                     	 
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x04;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
  
                     case AT_DCD:         //DCD����
                     	 wlFlagsSet.dcd = TRUE;                     
  
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x05;
                         modemPara.debugFrame(&say,1);
                       #endif                   	 
                     	 break;                   	 
  
                     case AT_SIGNAL:
                     case AT_RE_SIGNAL:
                     case AT_TO_GPRS:
                     	 if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
                     	 {
                     	 	  wlFlagsSet.signal = AT_M_RESPONSE_OK;                   	 	  
                     	 }
                     	 
                       #ifdef PRINT_WIRELESS_DEBUG
                         if (atType==AT_SIGNAL)
                         {
                           say=0x07;
                         }
                         else
                         {
                           say=0x17;
                         } 
                         modemPara.debugFrame(&say,1);
                       #endif
                       break;
  
                     case AT_IP_MODE:
                     	 wlFlagsSet.ipMode = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x08;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
  
                     case AT_TRANSPARENT:
                     	 wlFlagsSet.simTransparent = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x09;
                         modemPara.debugFrame(&say,1);
                       #endif                   	 
                     	 break;
  
                     case AT_APN:
                     	 wlFlagsSet.simApn = TRUE;
                     	 
                     	 wlFlagsSet.simCiicr=1;
                     	 
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x0a;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
                     
                     case AT_CIICR:
                     	 wlFlagsSet.simCiicr = 31;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x0b;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
                     	 
                     case AT_TO_AT:  //ת������ģʽ(GPRS->AT)
                       #ifdef PRINT_GPRS_DEBUG
                         say = 0x0d;
                         modemPara.debugFrame(&say,1);
                       #endif
                        
                       strcpy((char *)atCmd,"AT+CSQ\r");
 	                     atType = AT_RE_SIGNAL;
 	                     modemPara.sendFrame(atCmd,7);
 	                     
                       return;

                     case AT_LC_TCP_PORT:
                     	 wlFlagsSet.wlLcTcpPort = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x38;                         
                         sendDebugFrame(&say,1);
                       #endif
                     	 break;

                     case AT_REG:
                     	 if (wlFlagsSet.creg==AT_M_RESPONSE_INFO_ACK)
                     	 {
                     	 	 wlFlagsSet.creg = AT_M_RESPONSE_OK;
                     	 	              	 	 	 	  
             	 	 	 	     ret = REGISTERED_NET;
                     	 }
                     	 break;

                     case AT_CGATT:
                     	 if (wlFlagsSet.simCgatt==AT_M_RESPONSE_INFO_ACK)
                     	 {
                     	 	 wlFlagsSet.simCgatt = AT_M_RESPONSE_OK;                   	 	  
                     	 }
                     	 break;
          	 	 	  }
          	 	 	  atType = AT_NONE;
          	 	 }
          	 	 break;
          	 	 
             case 11:
               //0D 0A 43 4F 4E 4E 45 43-54 0D 0A(CONNECT)
          	   if (*(pFrame+2)==0x43 && *(pFrame+3)==0x4F && *(pFrame+4)==0x4E && *(pFrame+5)==0x4E
          	   	   && *(pFrame+6)==0x45 && *(pFrame+7)==0x43 && *(pFrame+8)==0x54)
          	   {
                  ipPermit = TRUE;
                  ret = IP_PERMIT;

                  #ifdef PRINT_WIRELESS_DEBUG
                    if (atType == AT_TO_GPRS)
                    {
                  	   say = 0x1e;        //AT->GPRSת���ɹ�
                    }
                    else
                    {
                       say=0x0e;
                    }
                    modemPara.debugFrame(&say,1);
                  #endif
                  atType = AT_NONE;
          	   }
          	   break;
          	   
          	 case 13:
          	 case 14:
          	 case 15:
          	   //0d 0a 2b 43 47 41 54 54 3a 20 31 0d 0a(+CGATT: 1)
          	   if (len==13)
          	   {
          	   	 if (atType==AT_CGATT)
          	   	 {
          	   	 	 if (*(pFrame+10)==0x31)
          	   	 	 {
          	   	 	 	  wlFlagsSet.simCgatt = AT_M_RESPONSE_INFO_ACK;
          	   	 	 }
          	   	 }
          	   }

          	 	 //0D 0A 43 61 6C 6C 20 52 65 61 64 79 0D 0A(Call Ready)
          	 	 if (len==14)
          	 	 {
             	 	 if (*(pFrame+2)==0x43 && *(pFrame+3)==0x61 && *(pFrame+4)==0x6c && *(pFrame+5)==0x6c
             	 	 	  && *(pFrame+6)==0x20 && *(pFrame+7)==0x52 && *(pFrame+8)==0x65 && *(pFrame+9)==0x61
             	 	 	  && *(pFrame+10)==0x64 && *(pFrame+11)==0x79)
             	 	 {
             	 	 	  wlFlagsSet.callReady = TRUE;
                     
                     #ifdef PRINT_WIRELESS_DEBUG
             	 	 	    say = 0x01;
             	 	 	    modemPara.debugFrame(&say,1);
             	 	 	  #endif
             	 	 }
             	 	 
             	 	 //0d 0a 2b 43 52 45 47 3a 20 30 2c 31 0d 0a(CREG:0,1)
             	 	 if (atType==AT_REG)
             	 	 {
             	 	 	 //if (*(pFrame+11)==0x31)
             	 	 	 //2012-09-18,add,���ٷ��������ն˵Ŀ���������״̬(����:CREG:0,5 ��ʵ�Ѿ�ע��ɹ�)
                   //2012-09-18,�������ע��ɹ�ָʾ
             	 	 	 if (*(pFrame+11)==0x31 || *(pFrame+11)==0x35)
             	 	 	 {
             	 	 	 	 wlFlagsSet.creg = AT_M_RESPONSE_INFO_ACK;
             	 	 	 }
             	 	 }

             	   //0d 0a 43 4f 4e 4e 45 43 54 20 4f 4b 0d 0a
          	     if (*(pFrame+2)==0x43 && *(pFrame+3)==0x4F && *(pFrame+4)==0x4E && *(pFrame+5)==0x4E
          	   	    && *(pFrame+6)==0x45 && *(pFrame+7)==0x43 && *(pFrame+8)==0x54)
          	     {
                    ipPermit = TRUE;
                    ret = IP_PERMIT;

                    #ifdef PRINT_WIRELESS_DEBUG
                     if (atType == AT_TO_GPRS)
                     {
                  	    say = 0x1f;        //AT->GPRSת���ɹ�
                     }
                     else
                     {
                        say=0x0f;
                     }
                     modemPara.debugFrame(&say,1);
                    #endif
                    atType = AT_NONE; 	   	  
          	     }
             	 }
          	 	 
          	 	 if (len==15)
          	 	 {
          	 	 	 //0d 0a 2b 50 44 50 3a 20 44 45 41 43 54 0d 0a (+PDP: DEACT)
          	     if (*(pFrame+2)==0x2b && *(pFrame+3)==0x50 && *(pFrame+4)==0x44 && *(pFrame+5)==0x50
          	   	     && *(pFrame+6)==0x3a && *(pFrame+7)==0x20 && *(pFrame+8)==0x44 && *(pFrame+9)==0x45
          	   	      && *(pFrame+10)==0x41 && *(pFrame+11)==0x43 && *(pFrame+12)==0x54
          	   	    )
          	     {
          	     	 printf("�յ�deact\n");
          	     	 
          	     	 ret = LINK_FAIL;
          	     }
          	 	 }
          	 	 
               //0D 0A 2B 43 53 51 3A 20-32 33 2C 30 0D 0A(�ź������ظ�)
          	   if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
          	   	   && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
          	   {
                  if (*(pFrame+9)==0x2c)
                  {
                     rssi = *(pFrame+8)-0x30;
                     
                     if (*(pFrame+11)==0x0D)
                     {
                        bitErr = *(pFrame+10)-0x30;
                     }
                     else
                     {
                        bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
                     }
                  }
                  else
                  {
                     rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
                     if (*(pFrame+12)==0x0D)
                     {
                       bitErr = *(pFrame+11)-0x30;
                     }
                     else
                     {
                       bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
                     }
                  }
  
                  if (rssi == 99 || rssi>31)
                  {
                	   modemPara.reportSignal(0, 0);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
                  }
                  else
                  {
                     modemPara.reportSignal(0, rssi);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
                  }
                  
                  if (atType==AT_RE_SIGNAL)
                  {
    	               modemPara.sendFrame((INT8U *)"ATO0\r",5);
    	               atType = AT_TO_GPRS;
                     #ifdef PRINT_WIRELESS_DEBUG
                       say=0x16;
                       modemPara.debugFrame(&say,1);
                     #endif                         	
                  }
                  else
                  {
                     #ifdef PRINT_WIRELESS_DEBUG
                        say=0x06;
                        modemPara.debugFrame(&say,1);
                     #endif
                  }
          	   }
          	 	 break;
          	 	 
          	 case 16:
          	   //0D 0A 2B 43 53 4D 49 4E-53 3A 20 30 2C 31 0D 0A (+CSMINS�ظ���Ϣ)
          	   if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x4d
          	   	   && *(pFrame+6)==0x49 && *(pFrame+7)==0x4e && *(pFrame+8)==0x53 && *(pFrame+9)==0x3a
          	   	   && *(pFrame+10)==0x20)
          	   {
                  if (atType == AT_SIM)         //�Ƿ�忨
                  {
                     if (*(pFrame+13) == 0x31)
                     {
                      	wlFlagsSet.cardInserted = AT_M_RESPONSE_INFO_ACK;  //�Ѳ���SIM��
                     
                        #ifdef PRINT_WIRELESS_DEBUG
                          say=0x03;
                          modemPara.debugFrame(&say,1);
                        #endif
                     }
                     else   
                     {
                        wlFlagsSet.cardInserted = AT_M_RESPONSE_INFO_NACK; //δ����SIM��
                        #ifdef PRINT_WIRELESS_DEBUG
                          say=0x23;
                          modemPara.debugFrame(&say,1);
                        #endif
                     }
                  }
          	   }
          	   break;               
           }
        }
        break;
        
      case CDMA_DTGS800:
      	switch(len)
      	{
        	case 6:  
        	 	//0D 0A 4F 4B 0D 0A(OK)
        	 	if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
        	 	{
        	 		 switch(atType)
        	 		 {
        	 		 	 case AT_IPR:
        	 		 	 	 wlFlagsSet.ipr = TRUE;
                   
                   #ifdef PRINT_GPRS_DEBUG
                     say=0x30;
                     modemPara.debugFrame(&say,1);
                   #endif
                   
                   retry = 0;
                   modemPara.portConfig(0);;   //���ó�115200        	 		 	 	 
        	 		 	   break;
        	 		 	   
        	 		 	 case AT_ECHO:
                   wlFlagsSet.echo = TRUE;         //�����ѹر�! 
                   
                   #ifdef PRINT_GPRS_DEBUG
                     say=0x31;
                     modemPara.debugFrame(&say,1);
                   #endif
                      
        	 		 	 	 break;

        	 		 	 case AT_UIM:
                   if (wlFlagsSet.cardInserted==AT_M_RESPONSE_INFO_ACK)
                   {
                   	 wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;

                     #ifdef PRINT_WIRELESS_DEBUG
                       say=0x33;
                       modemPara.debugFrame(&say,1);
                     #endif
                   }
        	 		 	 	 break;

        	 		 	 case AT_SPC:
                   wlFlagsSet.dtgsSpc = TRUE;        //����SPC�ųɹ�
                   
                   #ifdef PRINT_WIRELESS_DEBUG
                     say=0x34;
                     modemPara.debugFrame(&say,1);
                   #endif
        	 		 	 	 break;

        	 		 	 case AT_AINF:
                   wlFlagsSet.dtgsAinf = TRUE;       //��������TCP/IPЭ���Ȩ��Ϣ
                   wlFlagsSet.ainfInputed = TRUE;    //��Ȩ��Ϣ������
                   
                   #ifdef PRINT_WIRELESS_DEBUG
                     say=0x35;
                     modemPara.debugFrame(&say,1);
                   #endif
        	 		 	 	 break;
        	 		 	 	 
        	 		 	 case AT_SIGNAL:
                 	 if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
                 	 {
                 	 	  wlFlagsSet.signal = AT_M_RESPONSE_OK;                   	 	  
                 	 
                      #ifdef PRINT_WIRELESS_DEBUG
                        say=0x37;
                        modemPara.debugFrame(&say,1);
                      #endif
                 	 }
        	 		 	 	 break;
        	 		 	 	 
        	 		 	 case AT_REG:
                 	 if (wlFlagsSet.dtgsRegNet==AT_M_RESPONSE_INFO_ACK)
                 	 {
                 	 	  wlFlagsSet.dtgsRegNet = AT_M_RESPONSE_OK;
                 	 	  
                 	 	  ret = REGISTERED_NET;

                      #ifdef PRINT_WIRELESS_DEBUG
                        say=0x39;
                        modemPara.debugFrame(&say,1);
                      #endif
                 	 }
        	 		 	 	 break;
        	 		 	 	 
        	 		 	 case AT_CRM:
                   wlFlagsSet.dtgsCrm = TRUE;         //����Э�����趨 
                   
                   #ifdef PRINT_WIRELESS_DEBUG
                     say=0x3b;
                     modemPara.debugFrame(&say,1);
                   #endif
        	 		 	 	 break;
        	 		 	 	 
        	 		 	 case AT_DIP:
                   wlFlagsSet.dtgsDip = TRUE;         //Ŀ��IP��ַ���趨 
                   
                   #ifdef PRINT_WIRELESS_DEBUG
                     say=0x3c;
                     modemPara.debugFrame(&say,1);
                   #endif
        	 		 	 	 break;

        	 		 	 case AT_DPORT:
                   wlFlagsSet.dtgsDPort = TRUE;       //����Ŀ�Ķ˿����趨
                   
                   #ifdef PRINT_WIRELESS_DEBUG
                     say=0x3d;
                     modemPara.debugFrame(&say,1);
                   #endif
        	 		 	 	 break;
        	 		 }
        	 		 
        	 		 atType = AT_NONE;
        	 	}
        	 	break;
        	
          case 11:
            //0D 0A 43 4F 4E 4E 45 43-54 0D 0A(CONNECT)
       	    if (*(pFrame+2)==0x43 && *(pFrame+3)==0x4F && *(pFrame+4)==0x4E && *(pFrame+5)==0x4E
       	   	   && *(pFrame+6)==0x45 && *(pFrame+7)==0x43 && *(pFrame+8)==0x54)
       	    {
               //wlFlagsSet.dialConnected = TRUE;  //���ųɹ�!
               
               ipPermit = TRUE;
               
               ret = IP_PERMIT;

               #ifdef PRINT_WIRELESS_DEBUG
               	 say = 0x3e;
                 modemPara.debugFrame(&say,1);
               #endif

               atType = AT_NONE; 	   	  
       	    }
       	    break;

        	case 12:
        		//0D 0A 2B 52 45 47 53 54 3A 31 0D 0A[��ѯģ�鵱ǰģ��ע�ᵽ�����״̬Register]
        		if (*(pFrame+2)==0x2b && *(pFrame+3)==0x52 && *(pFrame+4)==0x45 && *(pFrame+5)==0x47 && *(pFrame+6)==0x53
        			  && *(pFrame+7)==0x54 && *(pFrame+8)==0x3a)
        	  {
        	  	 if (*(pFrame+9)==0x31)   //�Ѿ�ע��������
        	  	 {
             	   wlFlagsSet.dtgsRegNet = AT_M_RESPONSE_INFO_ACK;
        	  	 }
        	  	 else                     //��δע��������
        	  	 {
             	   wlFlagsSet.dtgsRegNet = AT_M_RESPONSE_INFO_NACK;
        	  	 }
               #ifdef PRINT_WIRELESS_DEBUG
                 say=0x38;
                 modemPara.debugFrame(&say,1);
               #endif
        	  }
        		break;
        		 	
        	case 14:
        	case 15:
        	case 16:
        	  //0D 0A 2B 43 53 51 3A 20 31 36 2C 20 39 39 0D 0A(�ź�����������Ϣ)
        	  if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
        	     && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
        	  {
               if (*(pFrame+9)==0x2c)
               {
                  rssi = *(pFrame+8)-0x30;
                  
                  if (*(pFrame+11)==0x0D)
                  {
                     bitErr = *(pFrame+10)-0x30;
                  }
                  else
                  {
                     bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
                  }
               }
               else
               {
                  rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
                  if (*(pFrame+12)==0x0D)
                  {
                    bitErr = *(pFrame+11)-0x30;
                  }
                  else
                  {
                    bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
                  }
               }

               if (rssi == 99 || rssi>31)
               {
             	   modemPara.reportSignal(0, 0);
             	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
               }
               else
               {
                 modemPara.reportSignal(0,rssi);
             	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
               }
               
               #ifdef PRINT_WIRELESS_DEBUG
                 say=0x36;
                 modemPara.debugFrame(&say,1);
               #endif
        	  }        		
        		break;
        	 	
        	case 18:
        	  //��ѯRUIM״̬���صĶ̸�ʽ����
        	  //0D 0A 2B 52 4C 4F 43 4B 3A 20 4E 4F 4C 4F 43 4B 0D 0A
        		if (*(pFrame+2)==0x2b && *(pFrame+3)==0x52 && *(pFrame+4)==0x4c && *(pFrame+5)==0x4f && *(pFrame+6)==0x43
        			 && *(pFrame+7)==0x4b && *(pFrame+8)==0x3a && *(pFrame+9)==0x20 && *(pFrame+10)==0x4e && *(pFrame+11)==0x4f
        			 && *(pFrame+12)==0x4c && *(pFrame+13)==0x4f && *(pFrame+14)==0x43 && *(pFrame+15)==0x4b
        			 )
        	  {
               wlFlagsSet.cardInserted = AT_M_RESPONSE_INFO_ACK;  //�Ѳ���SIM��
                 
               #ifdef PRINT_GPRS_DEBUG
                 say=0x42;
                 modemPara.debugFrame(&say,1);
               #endif
        	  }
        	  break;
        	

        	case 29:
        	case 30:
        	case 31:
        		//CDMA����ϵͳʱ��
        		//0D 0A 2B 54 49 4D 45 3A 20 20 36 2F 30 33 2F 30 39 20 30 37 3A 35 35 3A 31 39 20 33 0D 0A
        		if (*(pFrame+2)==0x2b && *(pFrame+3)==0x54 && *(pFrame+4)==0x49 && *(pFrame+5)==0x4d && *(pFrame+6)==0x45
        			  && *(pFrame+7)==0x3a && *(pFrame+8)==0x20
        			 )
        	  {
              //sysTime.year = (*(pFrame+15)-0x30)*10+(*(pFrame+16)-0x30);
              //if (*(pFrame+10)==' ')
              //{
              //  sysTime.month = (*(pFrame+11)-0x30);
              //}
              //else
              //{
              //  sysTime.month = (*(pFrame+10)-0x30)*10+(*(pFrame+11)-0x30);
              //}
              //sysTime.day    = (*(pFrame+13)-0x30)*10+(*(pFrame+14)-0x30);
              //sysTime.hour   = (*(pFrame+18)-0x30)*10+(*(pFrame+19)-0x30);
              //sysTime.minute = (*(pFrame+21)-0x30)*10+(*(pFrame+22)-0x30);
              //sysTime.second = (*(pFrame+24)-0x30)*10+(*(pFrame+25)-0x30);
              
              //adjustExtRtc(sysTime);
              
              //��ʾһ������ʱ��
              //guiAscii(48, 3, digital2ToString(sysTime.hour,str),1);
              //guiAscii(60, 3, ":", 1);
              //guiAscii(66, 3, digital2ToString(sysTime.minute,str),1);
              //guiAscii(78, 3, ":", 1);
              //guiAscii(84, 3, digital2ToString(sysTime.second,str),1);
              
              #ifdef PRINT_GPRS_DEBUG
                say=0x3a;
                modemPara.debugFrame(&say,1);
              #endif                       
              wlFlagsSet.dtgsTime = TRUE;
              atType = AT_NONE;
        	  }
        		break;
        		
        	case 37:
        		//+RLOCK:NOLOCK, RUIM is unlocked.
        		//0D 0A 2B 52 4C 4F 43 4B 3A 20 4E 4F 4C 4F 43 4B 2C 20 52 55 49 4D 20 69 73 20 75 6E 6C 6F 63 6B 65 64 2E 0D 0A 
        		if (*(pFrame+2)==0x2b && *(pFrame+3)==0x52 && *(pFrame+4)==0x4c && *(pFrame+5)==0x4f && *(pFrame+6)==0x43
        			 && *(pFrame+7)==0x4b && *(pFrame+8)==0x3a && *(pFrame+9)==0x20 && *(pFrame+10)==0x4e && *(pFrame+11)==0x4f
        			 && *(pFrame+12)==0x4c && *(pFrame+13)==0x4f && *(pFrame+14)==0x43 && *(pFrame+15)==0x4b
        			 )
        		{
               wlFlagsSet.cardInserted = AT_M_RESPONSE_INFO_ACK;  //�Ѳ���SIM��
                 
               #ifdef PRINT_GPRS_DEBUG
                 say=0x32;
                 modemPara.debugFrame(&say,1);
               #endif
        		}
        		break;      		 
      	}
      	break;
      	
   	  case GPRS_MC52I:
        if (atType == AT_CIFSR)
        {
          //��ȡ����IP��ַ
          //^SICI: 0,2,1,"10.83.19.32"
          //0d 0a 5e 53 49 43 49 3a 20 30 2c 32 2c 31 2c 22 31 30 2e 38 33 2e 31 39 2e 33 32 22 0d 0a
          if (*(pFrame+2)==0x5e && *(pFrame+3)==0x53 && *(pFrame+4)==0x49 && *(pFrame+5)==0x43  && *(pFrame+6)==0x49)
          {
            wlFlagsSet.simCifsr = TRUE;
  
            pFrame += 16;
  
        	  *modemPara.localIp = 0x0;
        	  
        	  //IP��ַ��һ�ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp = (*pFrame -0x30)<<24;
        	     pFrame+=2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
          	   {
        	        *modemPara.localIp = ((*pFrame-0x30)*10 + *(pFrame+1)-0x30)<<24;
        	        pFrame+=3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp = ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<24;
        	        pFrame+=4;
        	     }
        	  }
  
        	  //IP��ַ�ڶ��ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30)<<16;
        	     pFrame+=2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<16;
        	        pFrame+=3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<16;
        	        pFrame+=4;
        	     }
        	  }
        	  
        	  //IP��ַ�����ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30)<<8;
        	     pFrame += 2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<8;
        	        pFrame += 3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<8;
        	        pFrame += 4;
        	     }
        	  }
  
        	  //IP��ַ�����ֽ�
        	  if (*(pFrame+1)==0x22 && *(pFrame+2)==0x0d && *(pFrame+3)==0x0a)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30);
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x22 && *(pFrame+3)==0x0d && *(pFrame+4)==0x0a)    //ֻ��һλ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30);
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30);
        	     }
        	  }
        	  
            #ifdef PRINT_WIRELESS_DEBUG
              say = 0x80;
              modemPara.debugFrame(&say,1);
            #endif
          
            atType = AT_NONE;
          }
        }
        else
        {
          switch (len)
          {
          	 case 6:
          	 	 //0D 0A 4F 4B 0D 0A(OK)
          	 	 if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
          	 	 {
          	 	 	  switch(atType)
          	 	 	  {
                     case AT_ECHO:        //�رջ���
                     	 wlFlagsSet.echo = TRUE;
                       
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x82;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
                     
                     case AT_SIM:         //�Ƿ�忨
                     	 if (wlFlagsSet.cardInserted==AT_M_RESPONSE_INFO_ACK)
                     	 {
                     	 	  wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                     	 }
                     	 
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x84;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
                     	 
                     case AT_MC5X_SET_CREG:   //����ע����ʾ
                     	 wlFlagsSet.mc5xSetCreg = TRUE;
                       
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x85;
                         modemPara.debugFrame(&say,1);
                       #endif                     	 
                     	 break;
                     
                     case AT_MC5X_QUERY_CREG: //��ѯ����ע�����
                     	 //if (wlFlagsSet.mc5xQueryCreg==AT_M_RESPONSE_INFO_ACK)
                     	 //{
                     	 	  wlFlagsSet.mc5xQueryCreg = AT_M_RESPONSE_OK;
                     	 //}
                     	 
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x87;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
  
                     case AT_SIGNAL:
                     case AT_RE_SIGNAL:
                     case AT_TO_GPRS:
                     	 if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
                     	 {
                     	 	  wlFlagsSet.signal = AT_M_RESPONSE_OK;                   	 	  
                     	 }
                     	 
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x89;
                         modemPara.debugFrame(&say,1);
                       #endif
                       break;
                       
                     case AT_MC5X_SET_CGREG:   //����GPRS����ע����ʾ
                     	 wlFlagsSet.mc5xSetCGreg = TRUE;
                       
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x8a;
                         modemPara.debugFrame(&say,1);
                       #endif                     	 
                     	 break;
                     	 
                     case AT_MC5X_SELECTT_CONN://ѡ����������
                     	 wlFlagsSet.mc5xSelectConn = TRUE;
                       
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x8b;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;

                     case AT_APN:              //����APN
                     	 wlFlagsSet.simApn = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x8c;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;

                     case AT_MC5X_SOCKET:     //���÷�������ΪSocket
                     	 wlFlagsSet.mc5xSocket = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x8d;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;

                     case AT_MC5X_CON_ID:     //��������ID
                     	 wlFlagsSet.mc5xConId = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x8e;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;

                     case AT_MC5X_IP_PORT:    //����IP��ַ���˿�
                     	 wlFlagsSet.mc5xIpPort = TRUE;
                       #ifdef PRINT_WIRELESS_DEBUG
                         say=0x8f;
                         modemPara.debugFrame(&say,1);
                       #endif
                     	 break;
          	 	 	  }
          	 	 	  atType = AT_NONE;
          	 	 }
          	 	 break;
          	   
          	 case 13:
          	 case 14:
          	 case 15:
          	 	 //0D 0A 43 61 6C 6C 20 52 65 61 64 79 0D 0A(Call Ready)
          	 	 if (len==14)
          	 	 {          	     
   	  	         //0d 0a 2b 43 52 45 47 3a 20 31 2c 31 0d 0a(+CREG:1,1)
          	     if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x52 && *(pFrame+5)==0x45
          	   	     && *(pFrame+6)==0x47 && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x31
          	   	     && *(pFrame+10)==0x2c)
          	     {
                    if (*(pFrame+11)==0x31)
                    {
                      wlFlagsSet.mc5xQueryCreg = AT_M_RESPONSE_INFO_ACK;  //�Ѿ�ע�ᵽ��������
                    }

                    #ifdef PRINT_WIRELESS_DEBUG
                      say=0x86;
                      modemPara.debugFrame(&say,1);
                    #endif
          	     }
             	 }
          	 	 
               //0D 0A 2B 43 53 51 3A 20-32 33 2C 30 0D 0A(�ź������ظ�)
          	   if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
          	   	   && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
          	   {
                  if (*(pFrame+9)==0x2c)
                  {
                     rssi = *(pFrame+8)-0x30;
                     
                     if (*(pFrame+11)==0x0D)
                     {
                        bitErr = *(pFrame+10)-0x30;
                     }
                     else
                     {
                        bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
                     }
                  }
                  else
                  {
                     rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
                     if (*(pFrame+12)==0x0D)
                     {
                       bitErr = *(pFrame+11)-0x30;
                     }
                     else
                     {
                       bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
                     }
                  }
  
                  if (rssi == 99 || rssi>31)
                  {
                	   modemPara.reportSignal(0, 0);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
                  }
                  else
                  {
                     modemPara.reportSignal(0, rssi);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
                  }
                  
                  #ifdef PRINT_WIRELESS_DEBUG
                    say=0x88;
                    modemPara.debugFrame(&say,1);
                  #endif
          	   }
          	 	 break;
          	 	 
          	 case 16:
          	   //0d 0a 2b 43 50 49 4e 3a 20 52 45 41 44 59 0d 0a(����״̬�ظ���Ϣ)
          	   if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x50 && *(pFrame+5)==0x49
          	   	   && *(pFrame+6)==0x4e && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x52
          	   	   && *(pFrame+10)==0x45 && *(pFrame+11)==0x41 && *(pFrame+12)==0x44 && *(pFrame+13)==0x59)
          	   {
                  wlFlagsSet.cardInserted = AT_M_RESPONSE_INFO_ACK;  //�Ѳ���SIM��

                  #ifdef PRINT_WIRELESS_DEBUG
                    say=0x83;
                    modemPara.debugFrame(&say,1);
                  #endif
          	   }   	  	       

          	   break;
             
             
          	 default:
          	 	 //0D 0A 5E 53 49 53 57 3A 20 31 2C 20 32 30 2C 20 32 30 0D 0A  (^SISW: 1, 20, 20)
          	   if (*(pFrame+2)==0x5E && *(pFrame+3)==0x53 && *(pFrame+4)==0x49 && *(pFrame+5)==0x53
          	   	   && *(pFrame+6)==0x57 && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x31)
          	   {
          	   	  if (wlFlagsSet.wlSendStatus == 2)
          	   	  {
          	   	  	 wlFlagsSet.wlSendStatus = 3;
          	   	  	 
                     #ifdef PRINT_WIRELESS_DEBUG
                      say=0x99;
                      modemPara.debugFrame(&say,1);
                     #endif
          	   	  }
          	 	 }
          	 	 break;
               
           }
        }
        break;
      
      case GPRS_M590E:
        if ((atType == AT_M590E_QUERY_XIIC) && (wlFlagsSet.m590eQueryXiic!=AT_M_RESPONSE_INFO_ACK))
        {
          //��ȡ����IP��ַ
          //+XIIC:1,10.83.19.32
          //0d 0a 2b 58 49 49 43 3a 20 20 20 20 31 2c 20 31 30 2e 38 31 2e 32 34 37 2e 31 31 33 0d 0a
          if (*(pFrame+2)==0x2b && *(pFrame+3)==0x58 && *(pFrame+4)==0x49 && *(pFrame+5)==0x49  
          	  && *(pFrame+6)==0x43 && *(pFrame+7)==0x3a && *(pFrame+12)==0x31)
          {
            wlFlagsSet.m590eQueryXiic= AT_M_RESPONSE_INFO_ACK;
  
            pFrame += 15;
  
        	  *modemPara.localIp = 0x0;
        	  
        	  //IP��ַ��һ�ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp = (*pFrame -0x30)<<24;
        	     pFrame+=2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
          	   {
        	        *modemPara.localIp = ((*pFrame-0x30)*10 + *(pFrame+1)-0x30)<<24;
        	        pFrame+=3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp = ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<24;
        	        pFrame+=4;
        	     }
        	  }
  
        	  //IP��ַ�ڶ��ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30)<<16;
        	     pFrame+=2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<16;
        	        pFrame+=3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<16;
        	        pFrame+=4;
        	     }
        	  }
        	  
        	  //IP��ַ�����ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30)<<8;
        	     pFrame += 2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<8;
        	        pFrame += 3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<8;
        	        pFrame += 4;
        	     }
        	  }
  
        	  //IP��ַ�����ֽ�
        	  if (*(pFrame+1)==0x22 && *(pFrame+2)==0x0d && *(pFrame+3)==0x0a)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30);
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x22 && *(pFrame+3)==0x0d && *(pFrame+4)==0x0a)    //ֻ��һλ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30);
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30);
        	     }
        	  }
        	  
            #ifdef PRINT_WIRELESS_DEBUG
              say = 0x74;
              modemPara.debugFrame(&say,1);
            #endif
          
            atType = AT_NONE;
          }
        }
        else
        {
          //0d 0a 2b 43 43 49 44 3a 20 38 39 38 36 30 30 0d 0a  0d 0a 4f 4b 0d 0a
          if ((atType == AT_SIM) && (wlFlagsSet.cardInserted!=AT_M_RESPONSE_INFO_ACK))
          {
        	 	if (*(pFrame+2)==0x2B && *(pFrame+3)==0x43 && *(pFrame+4)==0x43 && *(pFrame+5)==0x49
        	 	 	   && *(pFrame+6)==0x44 && *(pFrame+7)==0x3a
        	 	 	 )
        	 	{
        	 	   wlFlagsSet.cardInserted=AT_M_RESPONSE_INFO_ACK;
        	 	   printf("CCID(��)\r\n");
        	 	}
          }
          else
          {
            switch (len)
            {
            	 case 6:
            	 	 //0D 0A 4F 4B 0D 0A(OK)
            	 	 if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
            	 	 {
            	 	 	  switch(atType)
            	 	 	  {
                       case AT_ECHO:        //�رջ���
                       	 wlFlagsSet.echo = TRUE;
                         
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x76;
                           modemPara.debugFrame(&say,1);
                         #endif
                       	 break;
                       
                       case AT_SIM:         //�Ƿ�忨
                       	 if (wlFlagsSet.cardInserted==AT_M_RESPONSE_INFO_ACK)
                       	 {
                       	 	  wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                       	 }
                       	 
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x78;
                           modemPara.debugFrame(&say,1);
                         #endif
                       	 break;
                       	 
                       case AT_REG:         //��ѯ����ע�����
                       	 if (wlFlagsSet.m590eQueryCreg==AT_M_RESPONSE_INFO_ACK)
                       	 {
                       	 	  wlFlagsSet.m590eQueryCreg = AT_M_RESPONSE_OK;
             	 	 	 	        
             	 	 	 	        ret = REGISTERED_NET;
                       	 }
                       	 
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x7a;
                           modemPara.debugFrame(&say,1);
                         #endif
                       	 break;
    
                       case AT_SIGNAL:               //��ѯ�����ź�ǿ��
                       	 if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
                       	 {
                       	 	  wlFlagsSet.signal = AT_M_RESPONSE_OK;                   	 	  
                       	 }
                       	 
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x7c;
                           modemPara.debugFrame(&say,1);
                         #endif
                         break;
                         
                       case AT_M590E_XISP :        //������/�ⲿЭ��ջ
                       	 wlFlagsSet.m590eXisp = TRUE;
                         
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x7e;
                           modemPara.debugFrame(&say,1);
                         #endif
                       	 break;   
                         
                       case AT_CGDCONT:              //����APN
                       	 wlFlagsSet.cgdcont = TRUE;
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x40;
                           modemPara.debugFrame(&say,1);
                         #endif
                       	 break;
                       	 
                       case AT_M590E_SET_XIIC :        //����PPP����
                       	 wlFlagsSet.m590eSetXiic = TRUE;
                         
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x42;
                           modemPara.debugFrame(&say,1);
                         #endif
                       	 break;
                       	 
                       case AT_M590E_QUERY_XIIC:               //��ѯPPP����
                       	 if (wlFlagsSet.m590eQueryXiic==AT_M_RESPONSE_INFO_ACK)
                       	 {
                       	 	  wlFlagsSet.m590eQueryXiic = AT_M_RESPONSE_OK;
                       	 }
                       	 
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x44;
                           modemPara.debugFrame(&say,1);
                         #endif
                         break;	 
  
                       case AT_IP_START:    //����IP��ַ���˿�
                         wlFlagsSet.ipStarted = TRUE;
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x46;
                           modemPara.debugFrame(&say,1);
                         #endif                       
                       	 break;
            	 	 	  }
            	 	 	  atType = AT_NONE;
            	 	 }
            	 	 break;
            	 	 
            	 case 12:
            	 	 //0d 0a 2b 50 42 52 45 41 44 59 0d 0a 
            	 	 //0D 0A 2B 50 42 52 45 41 44 59 0D 0A(+PBREADY)
            	 	 if (*(pFrame+2)==0x2b && *(pFrame+3)==0x50 && *(pFrame+4)==0x42 && *(pFrame+5)==0x52
            	 		 && *(pFrame+6)==0x45 && *(pFrame+7)==0x41 && *(pFrame+8)==0x44 && *(pFrame+9)==0x59)
                 {
                 	 printf("atResponse:�յ�PBReady\n");
                 	 
                 	 wlFlagsSet.startup = TRUE;
                 	 
                 	 //��ʱ��,ly,2011-05-04
                 	 //ly,2011-12-27,ȡ��,����CCID��Ϊ�Ѿ����뿨
                 	 //wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                 }
                 
                 #ifdef PRINT_WIRELESS_DEBUG
                  say=0x4a;
                  modemPara.debugFrame(&say,1);
                 #endif
                 break;
            	   
            	 case 13:
            	 case 14:
            	 case 15:	
            	 	 if (len==14)
            	 	 {          	     
     	  	         //0d 0a 2b 43 52 45 47 3a 20 31 2c 31 0d 0a(+CREG:0,1)
            	     if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x52 && *(pFrame+5)==0x45
            	   	     && *(pFrame+6)==0x47 && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x30
            	   	     && *(pFrame+10)==0x2c)
            	     {
                      //if (*(pFrame+11)==0x31)
                      //2012-09-18,�������ע��ɹ�ָʾ
                      if (*(pFrame+11)==0x31 || *(pFrame+11)==0x35)
                      {
                        wlFlagsSet.m590eQueryCreg = AT_M_RESPONSE_INFO_ACK;  //�Ѿ�ע�ᵽ��������
                      }
  
                      #ifdef PRINT_WIRELESS_DEBUG
                        say=0x4c;
                        modemPara.debugFrame(&say,1);
                      #endif
            	     }
               	 }
      
            	 	 
                 //0D 0A 2B 43 53 51 3A 20-32 33 2C 30 0D 0A(�ź������ظ�)
            	   if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
            	   	   && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
            	   {
                    if (*(pFrame+9)==0x2c)
                    {
                       rssi = *(pFrame+8)-0x30;
                       
                       if (*(pFrame+11)==0x0D)
                       {
                          bitErr = *(pFrame+10)-0x30;
                       }
                       else
                       {
                          bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
                       }
                    }
                    else
                    {
                       rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
                       if (*(pFrame+12)==0x0D)
                       {
                         bitErr = *(pFrame+11)-0x30;
                       }
                       else
                       {
                         bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
                       }
                    }
    
                    if (rssi == 99 || rssi>31)
                    {
                  	   modemPara.reportSignal(0, 0);
                  	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
                    }
                    else
                    {
                       modemPara.reportSignal(0, rssi);
                  	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
                    }
                    
                    #ifdef PRINT_WIRELESS_DEBUG
                      say=0x50;
                      modemPara.debugFrame(&say,1);
                    #endif
            	   }	       
  
            	   break;
            	   
            	 case 16:
            	 case 17:
            	 case 18:
            	 case 20:
            	 	 //0D 0A 4D 4F 44 45 4D 3A 53 54 41 52 54 55 50(MODEM:STARTUP)
            	 	 if (len==17)
            	 	 {
            	 	 	 if (*(pFrame+2)==0x4D && *(pFrame+3)==0x4F && *(pFrame+4)==0x44 && *(pFrame+5)==0x45 
            	 	 	 	   && *(pFrame+6)==0x4D && *(pFrame+7)==0x3A && *(pFrame+8)==0x53 && *(pFrame+9)==0x54
            	 	 	 	   && *(pFrame+10)==0x41 && *(pFrame+11)==0x52 && *(pFrame+12)==0x54 && *(pFrame+13)==0x55
            	 	 	 	   && *(pFrame+14)==0x50)
            	 	 	 {
            	   	   wlFlagsSet.startup = TRUE;
            	   	 }
            	   	 #ifdef PRINT_WIRELESS_DEBUG
                        say=0x4e;
                        modemPara.debugFrame(&say,1);
                      #endif  
            	   }
            	 	 	 	
                 //0d 0a 2b 54 43 50 53 45 54 55 50 3a 30 2c 46 41 49 4c 0d 0a(+TCPSETUP:0,FAIL)
                 //0d 0a 2b 54 43 50 53 45 54 55 50 3a 30 2c 4f 4b 0d 0a(+TCPSETUP:0,OK)
               	 if (*(pFrame+2)==0x2B && *(pFrame+3)==0x54 && *(pFrame+4)==0x43 && *(pFrame+5)==0x50 
            	   	   && *(pFrame+6)==0x53 && *(pFrame+7)==0x45 && *(pFrame+8)==0x54 && *(pFrame+9)==0x55
            	   	   && *(pFrame+10)==0x50 && *(pFrame+11)==0x3a && *(pFrame+12)==0x30 && *(pFrame+13)==0x2c
            	   	  )
            	   { 
            	   	 if (*(pFrame+14)==0x4f && *(pFrame+15)==0x4b)
            	   	 {
                     ipPermit = TRUE;
                     ret = IP_PERMIT;
            	   	 
            	   	   #ifdef PRINT_WIRELESS_DEBUG
                       say=0x4f;
                       modemPara.debugFrame(&say,1);
                     #endif 
                   }
            	   	 
            	   	 if (*(pFrame+14)==0x46 && *(pFrame+15)==0x41 && *(pFrame+16)==0x49 && *(pFrame+17)==0x4c)
            	   	 {        	   	          
        	   	       if (wlFlagsSet.numOfConnect<3)
        	   	       {
        	   	         wlFlagsSet.numOfConnect++;
        	   	         wlFlagsSet.ipStarted = FALSE;
        	   	          	
        	   	 	       printf("TCP��������ʧ��,������\n");
        	   	       }
        	   	       else
        	   	       {
            	   	 	   printf("TCP������������4�ζ�ʧ��\n");
            	   	 	   ret = LINK_FAIL;
        	   	       }
            	   	 }                 
            	   }          	             	   
            	   break;
            	 
            	 default:
            	 	 //0d 0a 2b 43 43 49 44 3a 20 38 39 38 36 30 30 36 32 33 31 37 39 33 33 36 30 30 38 33 35 0d 0a
            	 	 if (*(pFrame+2)==0x2B && *(pFrame+3)==0x43 && *(pFrame+4)==0x43 && *(pFrame+5)==0x49
            	 	 	   && *(pFrame+6)==0x44 && *(pFrame+7)==0x3a
            	 	 	  )
            	 	 {
            	 	   wlFlagsSet.cardInserted=AT_M_RESPONSE_INFO_ACK;
            	 	   
            	 	   printf("CCID(����)\r\n");
            	 	 }
            	 	 break;
            }
          }
        }
        break;
        
      case CDMA_CM180:
       	if ((atType == AT_CM180_GETIP) && (wlFlagsSet.cm180Getip!=AT_M_RESPONSE_INFO_ACK))
        {
          //��ȡ����IP��ַ
          //+GETIP:119.87.130.17
          //0d 0a 2b 47 45 54 49 50 3a 31 31 39 2e 38 37 2e 31 33 30 2e 31 37 0d 0a 0d 0a 4f 4b 0d 0a 
          if (*(pFrame+2)==0x2B && *(pFrame+3)==0x47 && *(pFrame+4)==0x45 && *(pFrame+5)==0x54  
          	  && *(pFrame+6)==0x49 && *(pFrame+7)==0x50 && *(pFrame+8)==0x3A)
          {
            wlFlagsSet.cm180Getip = AT_M_RESPONSE_INFO_ACK;
  
            pFrame += 9;
  
        	  *modemPara.localIp = 0x0;
        	  
        	  //IP��ַ��һ�ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp = (*pFrame -0x30)<<24;
        	     pFrame+=2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
          	   {
        	        *modemPara.localIp = ((*pFrame-0x30)*10 + *(pFrame+1)-0x30)<<24;
        	        pFrame+=3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp = ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<24;
        	        pFrame+=4;
        	     }
        	  }
  
        	  //IP��ַ�ڶ��ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30)<<16;
        	     pFrame+=2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<16;
        	        pFrame+=3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<16;
        	        pFrame+=4;
        	     }
        	  }
        	  
        	  //IP��ַ�����ֽ�
        	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30)<<8;
        	     pFrame += 2;
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<8;
        	        pFrame += 3;
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<8;
        	        pFrame += 4;
        	     }
        	  }
  
        	  //IP��ַ�����ֽ�
        	  if (*(pFrame+1)==0x0d && *(pFrame+2)==0x0a)    //ֻ��һλ��
        	  {
        	     *modemPara.localIp |= (*pFrame-0x30);
        	  }
        	  else
        	  {
           	   if (*(pFrame+2)==0x0d && *(pFrame+3)==0x0a)    //ֻ��һλ��
          	   {
        	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30);
        	     }
        	     else
        	     {
        	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30);
        	     }
        	  }
        	  
            #ifdef PRINT_WIRELESS_DEBUG
              say = 0x60;
              modemPara.debugFrame(&say,1);
            #endif
          
            atType = AT_NONE;
          }
        }
        else
        {
         	switch(len)
         	{
           	case 6:
           	 	//0D 0A 4F 4B 0D 0A(OK)
           	 	if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
           	 	{
           	 		switch(atType)
           	 		{
           	 		 	case AT_ECHO:
                    wlFlagsSet.echo = TRUE;         //�����ѹر�! 
                      
                    #ifdef PRINT_GPRS_DEBUG
                     say=0x63;
                     modemPara.debugFrame(&say,1);
                    #endif
                         
           	 		 	 	break;
           	 		 	 	 
           	 		 	case AT_CM180_PNUM:                  //�趨����ҵ�����
           	 		 	 	wlFlagsSet.cm180pnum == TRUE; 
           	 		 	 	 
           	 		 	 	#ifdef PRINT_WIRELESS_DEBUG
                     say=0x64;
                     modemPara.debugFrame(&say,1);
                    #endif
           	 		 	 	break;
   
           	 		 	case AT_AINF:
                    wlFlagsSet.cm180plogin= TRUE;       //���ü�Ȩ��Ϣ
                      
                    #ifdef PRINT_WIRELESS_DEBUG
                     say=0x65;
                     modemPara.debugFrame(&say,1);
                    #endif
           	 		 	 	break;
           	 		 	 	 
           	 		 	case AT_SIGNAL:
                    if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
                    {
                    	wlFlagsSet.signal = AT_M_RESPONSE_OK;                   	 	  
                    	 
                      #ifdef PRINT_WIRELESS_DEBUG
                       say=0x66;
                       modemPara.debugFrame(&say,1);
                      #endif
                    }
           	 		 	 	break;
           	 		 	 	 	 
           	 		 	case AT_REG:
                    if (wlFlagsSet.cm180QueryCreg==AT_M_RESPONSE_INFO_ACK)
                    {
                    	wlFlagsSet.cm180QueryCreg = AT_M_RESPONSE_OK;
                    	
                    	ret = REGISTERED_NET;
                    	 	  
                      #ifdef PRINT_WIRELESS_DEBUG
                       say=0x67;
                       modemPara.debugFrame(&say,1);
                      #endif
                    }
                    break;
                    	 
                  case AT_CM180_GETIP:
                    if (wlFlagsSet.cm180Getip==AT_M_RESPONSE_INFO_ACK)
                    {
                    	wlFlagsSet.cm180Getip = AT_M_RESPONSE_OK;
                         
                      #ifdef PRINT_WIRELESS_DEBUG
                       say=0x68;
                       modemPara.debugFrame(&say,1);
                      #endif
                    }
                    break;
                    	 
                  case AT_CM180_PPPSTATUS:
                    if (wlFlagsSet.cm180pppStatus==AT_M_RESPONSE_INFO_ACK)
                    {
                    	wlFlagsSet.cm180pppStatus = AT_M_RESPONSE_OK;
                      #ifdef PRINT_WIRELESS_DEBUG
                        say=0x6f;
                        modemPara.debugFrame(&say,1);
                      #endif
                    }
                    break;
                    	
                  case AT_DAILING:
                    wlFlagsSet.cm180Dial=TRUE;
                    break;
                    	
                  case AT_IP_START:    //����IP��ַ���˿�
                    wlFlagsSet.ipStarted = TRUE;
                    #ifdef PRINT_WIRELESS_DEBUG
                      say=0x69;
                      modemPara.debugFrame(&say,1);
                    #endif                       
                    break;
                        	 
                    /*
                    case AT_IPSTATUS:
                    	 if (wlFlagsSet.cm180ipStatus==AT_M_RESPONSE_INFO_ACK)
                    	 {
                    	 	  wlFlagsSet.cm180ipStatus = AT_M_RESPONSE_OK;
                         #ifdef PRINT_WIRELESS_DEBUG
                           say=0x6a;
                           modemPara.debugFrame(&say,1);
                         #endif
                    	 }
                    	 break;
                    */
                  case AT_LC_TCP_PORT:
                  	wlFlagsSet.wlLcTcpPort = TRUE;
                  	break;
                }
                atType = AT_NONE; 
           	 	}
           	 	break;
           	
           	case 11:
         		  //+CIND:8(/1/0)
              //0d 0a 2b 43 49 4e 44 3a 38 0d 0a 
             	if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x49 && *(pFrame+5)==0x4e
             		 && *(pFrame+6)==0x44 && *(pFrame+7)==0x3a)
              {
                if (*(pFrame+8)==0x38)
                {
                  ;//printf("ģ����׼���ý���ATָ��\n");
                }
                
                if (*(pFrame+8)==0x31)
                {
                  wlFlagsSet.startup = TRUE;
                  wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                }
                
                if (*(pFrame+8)==0x30)
                {
                  modemPara.reportSignal(9,0);
                }
              }
              break;
   
           	case 13:
           	case 14:
           	case 15:
           	case 16:
           		if (len==13)
             	{
      	  	    //0d 0a 2b 43 52 45 47 3a 30 2c 31 0d 0a(+CREG:0,1)
             	  if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x52 && *(pFrame+5)==0x45
             	   	  && *(pFrame+6)==0x47 && *(pFrame+7)==0x3a && *(pFrame+8)==0x30 && *(pFrame+9)==0x2c)
             	  {
           	  	  //if (*(pFrame+10)==0x31)   //�Ѿ�ע��������
                  //2012-09-18,�������ע��ɹ�ָʾ
           	  	  if (*(pFrame+10)==0x31 || *(pFrame+10)==0x35)   //�Ѿ�ע��������
           	  	  {
                	   wlFlagsSet.cm180QueryCreg = AT_M_RESPONSE_INFO_ACK;
           	  	  }
           	  	  else                      //��δע��������
           	  	  {
                	    wlFlagsSet.cm180QueryCreg = AT_M_RESPONSE_INFO_NACK;
           	  	  }
           	  	  
                  #ifdef PRINT_WIRELESS_DEBUG
                   say=0x6c;
                   modemPara.debugFrame(&say,1);
                  #endif
           	    }
             	}

             	//0d 0a 2b 43 53 51 3a 20 32 33 2c 20 39 39 0d 0a(�ź�����������Ϣ)
           	  if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
           	     && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
           	  {
                  if (*(pFrame+9)==0x2c)
                  {
                     rssi = *(pFrame+8)-0x30;
                     
                     if (*(pFrame+11)==0x0D)
                     {
                        bitErr = *(pFrame+10)-0x30;    
                     }
                     else
                     {
                        bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
                     }
                  }
                  else
                  {
                     rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
                     if (*(pFrame+12)==0x0D)
                     {
                       bitErr = *(pFrame+11)-0x30;
                     }
                     else
                     {
                       bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
                     }
                  }
   
                  if (rssi == 99 || rssi>31)
                  {
                	   modemPara.reportSignal(0, 0);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
                  }
                  else
                  {
                    modemPara.reportSignal(0,rssi);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
                  }
                  
                  #ifdef PRINT_WIRELESS_DEBUG
                    say=0x73;
                    modemPara.debugFrame(&say,1);
                  #endif
           	  }        		
           		break;
   	
             case 17:
             	//0D 0A 4D 4F 44 45 4D 3A 53 54 41 52 54 55 50(MODEM:STARTUP)
             	{
             	  if (*(pFrame+2)==0x4D && *(pFrame+3)==0x4F && *(pFrame+4)==0x44 && *(pFrame+5)==0x45 
             	 	 	 	&& *(pFrame+6)==0x4D && *(pFrame+7)==0x3A && *(pFrame+8)==0x53 && *(pFrame+9)==0x54
             	 	 	 	&& *(pFrame+10)==0x41 && *(pFrame+11)==0x52 && *(pFrame+12)==0x54 && *(pFrame+13)==0x55
             	 	 	 	&& *(pFrame+14)==0x50)
             	 	{
             	   	wlFlagsSet.startup = TRUE;
             	  }
             	  #ifdef PRINT_WIRELESS_DEBUG
                    say=0x75;
                    modemPara.debugFrame(&say,1);
                 #endif  
             	}	
             	break;
           	  
           	 case 21:
           	 	 //0d 0a 2b 50 50 50 53 54 41 54 55 53 3a 43 4c 4f 53 45 44 0d 0a(+PPPSTATUS:CLOSED)
               //0d 0a 2b 50 50 50 53 54 41 54 55 53 3a 4f 50 45 4e 45 44 0d 0a(+PPPSTATUS:OPENED)
               if (*(pFrame+2)==0x2B && *(pFrame+3)==0x50 && *(pFrame+4)==0x50 && *(pFrame+5)==0x50 
             	   	 && *(pFrame+6)==0x53 && *(pFrame+7)==0x54 && *(pFrame+8)==0x41 && *(pFrame+9)==0x54
             	   	 && *(pFrame+10)==0x55 && *(pFrame+11)==0x53 && *(pFrame+12)==0x3A )
             	 {
             	 	 if (atType==AT_CM180_PPPSTATUS)
             	   {
             	   	 wlFlagsSet.cm180pppStatus = AT_M_RESPONSE_INFO_ACK;
             	   }
             	   else
             	   {
             	   	 if (*(pFrame+13)==0x43 && *(pFrame+14)==0x4C && *(pFrame+15)==0x4F && *(pFrame+16)==0x53
             	   	    && *(pFrame+17)==0x45 && *(pFrame+18)==0x44)
             	     { 
             	   	   wlFlagsSet.dialConnected = FALSE;
                     
                     wlFlagsSet.cm180Dial = FALSE;  //���²���,ly,2011-07-09,add

             	   	   //printf("����PPP����ʧ��\n");
                   }
             	   	 
             	     if (*(pFrame+13)==0x4F && *(pFrame+14)==0x50 && *(pFrame+15)==0x45 && *(pFrame+16)==0x4E 
             	   	    && *(pFrame+17)==0x45 && *(pFrame+18)==0x44)
             	     {
             	   	   wlFlagsSet.dialConnected = TRUE;
             	   	   
             	   	   //printf("����PPP���ӳɹ�\n");
             	     }
             	     
             	     #ifdef PRINT_WIRELESS_DEBUG
                      say=0x77;
                      modemPara.debugFrame(&say,1);
                    #endif                 
             	   } 
             	 }
               break;
                
             case 20:
             case 22:
             case 25: 
               //+IPSTATUS:ESTABLISHED
               //+IPSTATUS:SYN_SENT
               //+IPSTATUS:CLOSED
               if (*(pFrame+2)==0x2B && *(pFrame+3)==0x49 && *(pFrame+4)==0x50 && *(pFrame+5)==0x53
             	   && *(pFrame+6)==0x54 && *(pFrame+7)==0x41 && *(pFrame+8)==0x54 && *(pFrame+9)==0x55
             	    && *(pFrame+10)==0x53 && *(pFrame+11)==0x3A)
             	 {
             	   	wlFlagsSet.cm180ipStatus=AT_M_RESPONSE_INFO_ACK;
          
                  if (*(pFrame+12)==0x45 && *(pFrame+13)==0x53 && *(pFrame+14)==0x54 && *(pFrame+15)==0x41
                    && *(pFrame+16)==0x42 && *(pFrame+17)==0x4C && *(pFrame+18)==0x49 && *(pFrame+19)==0x53 
                   	 && *(pFrame+20)==0x48 && *(pFrame+21)==0x45 && *(pFrame+22)==0x44) 
                  {
                     ipPermit = TRUE;
                     
                     ret = IP_PERMIT;
                     
                     #ifdef PRINT_WIRELESS_DEBUG
                       say=0x79;
                       modemPara.debugFrame(&say,1);
                     #endif     
                  }
                     
                  if (*(pFrame+12)==0x53 && *(pFrame+13)==0x59 && *(pFrame+14)==0x4E && *(pFrame+15)==0x5F
                    && *(pFrame+16)==0x53 && *(pFrame+17)==0x45 && *(pFrame+18)==0x4E && *(pFrame+19)==0x54)
                  {
                    //printf("TCP�������ڽ���,���Ժ�\n");	
                  }
                     
                  if (*(pFrame+12)==0x43 && *(pFrame+13)==0x4C && *(pFrame+14)==0x4F && *(pFrame+15)==0x53
                    && *(pFrame+16)==0x45 && *(pFrame+17)==0x44)
             	   	{
             	   	 	//printf("TCP��������ʧ��,δ����\n");
             	   	}
               }	
               break;
          }
      	}   
      	break;
      	
      case GPRS_M72D:
        if (atType == AT_QILOCIP)
        {
          //��ȡ����IP��ַ
          //0D 0A 31 30 2E 31 39 37 2E 31 31 2E 31 33 39 0D 0A 
          wlFlagsSet.m72locip = 1;

          pFrame += 2;   //tmpk = 2;

      	  *modemPara.localIp = 0x0;
      	  
      	  //IP��ַ��һ�ֽ�
      	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp = (*pFrame -0x30)<<24;
      	     pFrame+=2;
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x2e)    //ֻ�ж�λ��
        	   {
      	        *modemPara.localIp = ((*pFrame-0x30)*10 + *(pFrame+1)-0x30)<<24;
      	        pFrame+=3;
      	     }
      	     else
      	     {
      	        *modemPara.localIp = ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<24;
      	        pFrame+=4;
      	     }
      	  }

      	  //IP��ַ�ڶ��ֽ�
      	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp |= (*pFrame-0x30)<<16;
      	     pFrame+=2;
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
        	   {
      	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<16;
      	        pFrame+=3;
      	     }
      	     else
      	     {
      	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<16;
      	        pFrame+=4;
      	     }
      	  }
      	  
      	  //IP��ַ�����ֽ�
      	  if (*(pFrame+1)==0x2e)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp |= (*pFrame-0x30)<<8;
      	     pFrame += 2;
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x2e)    //ֻ��һλ��
        	   {
      	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<8;
      	        pFrame += 3;
      	     }
      	     else
      	     {
      	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<8;
      	        pFrame += 4;
      	     }
      	  }

      	  //IP��ַ�����ֽ�
      	  if (*(pFrame+1)==0x0d && *(pFrame+2)==0x0a)    //ֻ��һλ��
      	  {
      	     *modemPara.localIp |= (*pFrame-0x30);
      	  }
      	  else
      	  {
         	   if (*(pFrame+2)==0x0d && *(pFrame+3)==0x0a)    //ֻ��һλ��
        	   {
      	        *modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30);
      	     }
      	     else
      	     {
      	        *modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30);
      	     }
      	  }
        
          atType = AT_NONE;
        }
        else
        {
         	switch(len)
         	{
           	case 6:
           	 	//0D 0A 4F 4B 0D 0A(OK)
           	 	if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
           	 	{
           	 		switch(atType)
           	 		{
           	 		 	case AT_ECHO:
                    wlFlagsSet.echo = TRUE;      //�����ѹر�!                          
           	 		 	 	break;
                  
                  case AT_SIM:                   //�Ƿ�忨
                    if (wlFlagsSet.cardInserted==AT_M_RESPONSE_INFO_ACK)
                    {
                     	wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
                    }                    
                    break;
                  
                  case AT_SIGNAL:
                    if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
                    {
                     	wlFlagsSet.signal = AT_M_RESPONSE_OK;                   	 	  
                    }
                    break;
                    
                  case AT_REG:
                    if (wlFlagsSet.creg==AT_M_RESPONSE_INFO_ACK)
                    {
                     	wlFlagsSet.creg = AT_M_RESPONSE_OK;
                     	
                     	ret = REGISTERED_NET;
                    }
                    break;
                    
                  case AT_CGATT:
                    if (wlFlagsSet.simCgatt==AT_M_RESPONSE_INFO_ACK)
                    {
                     	wlFlagsSet.simCgatt = AT_M_RESPONSE_OK;                   	 	  
                    }
                    break;

                  case AT_CGREG:
                    if (wlFlagsSet.cgreg==AT_M_RESPONSE_INFO_ACK)
                    {
                     	wlFlagsSet.cgreg = AT_M_RESPONSE_OK;
                    }
                    break;
                    
                  case AT_QIFGCNT:
                  	wlFlagsSet.m72qifgcnt = 1;
                  	break;

                  case AT_QIAPN:
                  	wlFlagsSet.m72Apn = 1;
                  	break;

                  case AT_QIMUX:
                  	wlFlagsSet.m72mux = 1;
                  	break;

                  case AT_QIMODE:
                  	wlFlagsSet.m72IpMode = 1;
                  	break;

                  case AT_QITCFG:
                  	wlFlagsSet.m72Itcfg = 1;
                  	break;

                  case AT_QIDNSIP:
                  	wlFlagsSet.m72dnsip = 1;
                  	break;

                  case AT_QIREGAPP:
                  	wlFlagsSet.m72regapp = 1;
                  	break;

                  case AT_QIACT:
                  	wlFlagsSet.m72act = 1;
                  	break;
           	 		}
           	  }
           	  break;
            
             case 11:
               //0D 0A 43 4F 4E 4E 45 43-54 0D 0A(CONNECT)
          	   if (*(pFrame+2)==0x43 && *(pFrame+3)==0x4F && *(pFrame+4)==0x4E && *(pFrame+5)==0x4E
          	   	   && *(pFrame+6)==0x45 && *(pFrame+7)==0x43 && *(pFrame+8)==0x54)
          	   {
                  ipPermit = TRUE;
                  ret = IP_PERMIT;

                  atType = AT_NONE;
          	   }
          	   break;
          	   
          	 case 13:
          	 case 14:
          	 case 15:
          	   //0d 0a 2b 43 47 41 54 54 3a 20 31 0d 0a(+CGATT: 1)
          	   if (len==13)
          	   {
          	   	 if (atType==AT_CGATT)
          	   	 {
          	   	 	 if (*(pFrame+10)==0x31)
          	   	 	 {
          	   	 	 	  wlFlagsSet.simCgatt = AT_M_RESPONSE_INFO_ACK;
          	   	 	 }
          	   	 }
          	   }

          	 	 //0D 0A 43 61 6C 6C 20 52 65 61 64 79 0D 0A(Call Ready)
          	 	 if (len==14)
          	 	 {
             	 	 //0d 0a 2b 43 52 45 47 3a 20 30 2c 31 0d 0a(CREG:0,1)
             	 	 if (atType==AT_REG)
             	 	 {
             	 	 	 //if (*(pFrame+11)==0x31)
                   //2012-09-18,�������ע��ɹ�ָʾ
             	 	 	 if (*(pFrame+11)==0x31 || *(pFrame+11)==0x35)
             	 	 	 {
             	 	 	 	 wlFlagsSet.creg = AT_M_RESPONSE_INFO_ACK;
             	 	 	 }
             	 	 }
             	 }
             	 
             	 //0d 0a 2b 43 47 52 45 47 3a 20 30 2c 31 0d 0a
          	 	 if (len==15)
          	 	 {
             	 	 //0d 0a 2b 43 52 45 47 3a 20 30 2c 31 0d 0a(CGREG:0,1)
             	 	 if (atType==AT_CGREG)
             	 	 {
             	 	 	 if (*(pFrame+12)==0x31)
             	 	 	 {
             	 	 	 	 wlFlagsSet.cgreg = AT_M_RESPONSE_INFO_ACK;
             	 	 	 }
             	 	 }
             	 }
               
               //0D 0A 2B 43 53 51 3A 20-32 33 2C 30 0D 0A(�ź������ظ�)
          	   if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
          	   	   && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
          	   {
                  if (*(pFrame+9)==0x2c)
                  {
                     rssi = *(pFrame+8)-0x30;
                     
                     if (*(pFrame+11)==0x0D)
                     {
                       bitErr = *(pFrame+10)-0x30;
                     }
                     else
                     {
                       bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
                     }
                  }
                  else
                  {
                    rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
                    if (*(pFrame+12)==0x0D)
                    {
                       bitErr = *(pFrame+11)-0x30;
                    }
                    else
                    {
                       bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
                    }
                  }
  
                  if (rssi == 99 || rssi>31)
                  {
                	   modemPara.reportSignal(0, 0);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
                  }
                  else
                  {
                     modemPara.reportSignal(0, rssi);
                	   wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
                  }
          	   }
          	 	 break;

           	case 16:
              //0d 0a 2b 43 50 49 4e 3a 20 52 45 41 44 59 0d 0a(+CPIN: READY)
             	if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x50 && *(pFrame+5)==0x49
             	 	 	 	&& *(pFrame+6)==0x4e && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x52
             	 	 	 	&& *(pFrame+10)==0x45 && *(pFrame+11)==0x41 && *(pFrame+12)==0x44 && *(pFrame+13)==0x59
             	 	 )
             	{
             		wlFlagsSet.cardInserted = AT_M_RESPONSE_INFO_ACK;  //�Ѳ���SIM��
             	}
           		break;
           		
           	case 18:
           		//0d 0a 2b 43 4d 45 20 45 52 52 4f 52 3a 20 31 30 0d 0a
             	if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x4d && *(pFrame+5)==0x45 
             	 	 	 	&& *(pFrame+6)==0x20 && *(pFrame+7)==0x45 && *(pFrame+8)==0x52 && *(pFrame+9)==0x52
             	 	 	 	&& *(pFrame+10)==0x4f && *(pFrame+11)==0x52 && *(pFrame+12)==0x3a && *(pFrame+13)==0x20
             	 	 	 	&& *(pFrame+14)==0x31 && *(pFrame+15)==0x30)
             	{
                retry=100;
                modemPara.reportSignal(8,0);
             	}
           		break;
          }
          
        }
        break;
				
			case LTE_AIR720H:
				if ((atType == AT_M590E_QUERY_XIIC) && (wlFlagsSet.m590eQueryXiic!=AT_M_RESPONSE_INFO_ACK))
				{
					//��ȡ����IP��ַ
					//+XIIC:1,10.83.19.32
					//0d 0a 2b 58 49 49 43 3a 20 20 20 20 31 2c 20 31 30 2e 38 31 2e 32 34 37 2e 31 31 33 0d 0a
					if (*(pFrame+2)==0x2b && *(pFrame+3)==0x58 && *(pFrame+4)==0x49 && *(pFrame+5)==0x49	
							&& *(pFrame+6)==0x43 && *(pFrame+7)==0x3a && *(pFrame+12)==0x31)
					{
						wlFlagsSet.m590eQueryXiic= AT_M_RESPONSE_INFO_ACK;
			
						pFrame += 15;
			
						*modemPara.localIp = 0x0;
						
						//IP��ַ��һ�ֽ�
						if (*(pFrame+1)==0x2e)		//ֻ��һλ��
						{
							 *modemPara.localIp = (*pFrame -0x30)<<24;
							 pFrame+=2;
						}
						else
						{
							 if (*(pFrame+2)==0x2e) 	 //ֻ�ж�λ��
							 {
									*modemPara.localIp = ((*pFrame-0x30)*10 + *(pFrame+1)-0x30)<<24;
									pFrame+=3;
							 }
							 else
							 {
									*modemPara.localIp = ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<24;
									pFrame+=4;
							 }
						}
			
						//IP��ַ�ڶ��ֽ�
						if (*(pFrame+1)==0x2e)		//ֻ��һλ��
						{
							 *modemPara.localIp |= (*pFrame-0x30)<<16;
							 pFrame+=2;
						}
						else
						{
							 if (*(pFrame+2)==0x2e) 	 //ֻ�ж�λ��
							 {
									*modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<16;
									pFrame+=3;
							 }
							 else
							 {
									*modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<16;
									pFrame+=4;
							 }
						}
						
						//IP��ַ�����ֽ�
						if (*(pFrame+1)==0x2e)		//ֻ��һλ��
						{
							 *modemPara.localIp |= (*pFrame-0x30)<<8;
							 pFrame += 2;
						}
						else
						{
							 if (*(pFrame+2)==0x2e) 	 //ֻ��һλ��
							 {
									*modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30)<<8;
									pFrame += 3;
							 }
							 else
							 {
									*modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30)<<8;
									pFrame += 4;
							 }
						}
			
						//IP��ַ�����ֽ�
						if (*(pFrame+1)==0x22 && *(pFrame+2)==0x0d && *(pFrame+3)==0x0a)		//ֻ��һλ��
						{
							 *modemPara.localIp |= (*pFrame-0x30);
						}
						else
						{
							 if (*(pFrame+2)==0x22 && *(pFrame+3)==0x0d && *(pFrame+4)==0x0a) 	 //ֻ��һλ��
							 {
									*modemPara.localIp |= ((*pFrame-0x30)*10+*(pFrame+1)-0x30);
							 }
							 else
							 {
									*modemPara.localIp |= ((*pFrame-0x30)*100+(*(pFrame+1)-0x30)*10+*(pFrame+2)-0x30);
							 }
						}
						
						atType = AT_NONE;
					}
				}
				else
				{
					switch(atType)
					{
						case AT_COPS:
							if (strstr((char *)pFrame, "+COPS:"))
							{
								pData = strchr((char *)pFrame, ' ');
								tmpData = atoi(pData+1);
								printf("%s,=%d\n", (char *)pData, tmpData);
								
							}
							break;
					}
					
					switch (len)
					{
						 case 6:
							 //0D 0A 4F 4B 0D 0A(OK)
							 if (*(pFrame+2)==0x4f && *(pFrame+3)==0x4b)
							 {
									switch(atType)
									{
										 case AT_ECHO:				//�رջ���
											 wlFlagsSet.echo = TRUE;
											 break;
										 
										 case AT_SIM: 				//�Ƿ�忨
											 if (wlFlagsSet.cardInserted==AT_M_RESPONSE_INFO_ACK)
											 {
													wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
											 }
											 break;
											 
										 case AT_REG: 				//��ѯ����ע�����
											 if (wlFlagsSet.m590eQueryCreg==AT_M_RESPONSE_INFO_ACK)
											 {
												 wlFlagsSet.m590eQueryCreg = AT_M_RESPONSE_OK;
													
												 ret = REGISTERED_NET;
											 }
											 break;
											 
										 case AT_CGREG:       //GPRSע�����
											 if (wlFlagsSet.cgreg==AT_M_RESPONSE_INFO_ACK)
											 {
												 wlFlagsSet.cgreg = AT_M_RESPONSE_OK;
											 }
											 break;
		
										 case AT_SIGNAL:			//��ѯ�����ź�ǿ��
											 if (wlFlagsSet.signal==AT_M_RESPONSE_INFO_ACK)
											 {
												 wlFlagsSet.signal = AT_M_RESPONSE_OK; 												
											 }
											 break;
											 
										 case AT_CGDCONT: 		//����APN
											 wlFlagsSet.cgdcont = TRUE;
											 break;
										 
		
										 case AT_IP_START:		//����IP��ַ���˿�
											 wlFlagsSet.ipStarted = TRUE;
											 break;
									}
									atType = AT_NONE;
							 }
							 break;
							 
						 case 16:
							 //0d 0a 2b 43 50 49 4e 3a 20 52 45 41 44 59 0d 0a //+CPIN: READY
							 if (
							 	   *(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x50 && *(pFrame+5)==0x49
								    && *(pFrame+6)==0x4e && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x52
								 		 && *(pFrame+10)==0x45 && *(pFrame+11)==0x41 && *(pFrame+12)==0x44 && *(pFrame+13)==0x59
								  )
							 {
								 printf("atResponse:�յ�+CPIN: READY\n");
								 
								 wlFlagsSet.startup = TRUE;
								 
								 //��ʱ��,ly,2011-05-04
								 //ly,2011-12-27,ȡ��,����CCID��Ϊ�Ѿ����뿨
								 //wlFlagsSet.cardInserted = AT_M_RESPONSE_OK;
							 }
							 break;
			 	
						 case 13:
						 case 14:
						 case 15: 
							 if (len==14)
							 {								 
								 //0d 0a 2b 43 52 45 47 3a 20 31 2c 31 0d 0a(+CREG:1,1)
								 if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x52 && *(pFrame+5)==0x45
										 && *(pFrame+6)==0x47 && *(pFrame+7)==0x3a && *(pFrame+8)==0x20 && *(pFrame+9)==0x31
										 && *(pFrame+10)==0x2c)
								 {
										//if (*(pFrame+11)==0x31)
										//2012-09-18,�������ע��ɹ�ָʾ
										if (*(pFrame+11)==0x31 || *(pFrame+11)==0x35)
										{
											wlFlagsSet.m590eQueryCreg = AT_M_RESPONSE_INFO_ACK;  //�Ѿ�ע�ᵽ��������
										}
								 }
							 }
		
							 
							 //0D 0A 2B 43 53 51 3A 20-32 33 2C 30 0D 0A(�ź������ظ�)
							 if (*(pFrame+2)==0x2b && *(pFrame+3)==0x43 && *(pFrame+4)==0x53 && *(pFrame+5)==0x51
									 && *(pFrame+6)==0x3a && *(pFrame+7)==0x20)
							 {
									if (*(pFrame+9)==0x2c)
									{
										 rssi = *(pFrame+8)-0x30;
										 
										 if (*(pFrame+11)==0x0D)
										 {
												bitErr = *(pFrame+10)-0x30;
										 }
										 else
										 {
												bitErr = (*(pFrame+10)-0x30)*10 + (*(pFrame+11)-0x30);
										 }
									}
									else
									{
										 rssi = (*(pFrame+8)-0x30)*10+(*(pFrame+9)-0x30);
										 if (*(pFrame+12)==0x0D)
										 {
											 bitErr = *(pFrame+11)-0x30;
										 }
										 else
										 {
											 bitErr = (*(pFrame+11)-0x30)*10 + (*(pFrame+12)-0x30);
										 }
									}
		
									if (rssi == 99 || rssi>31)
									{
										 modemPara.reportSignal(0, 0);
										 wlFlagsSet.signal = AT_M_RESPONSE_INFO_NACK;
									}
									else
									{
										 modemPara.reportSignal(0, rssi);
										 wlFlagsSet.signal = AT_M_RESPONSE_INFO_ACK;
									}
							 }				 
		
							 break;
							 
						 case 17:
						 case 18:
						 case 20:
							 //0D 0A 4D 4F 44 45 4D 3A 53 54 41 52 54 55 50(MODEM:STARTUP)
							 if (len==17)
							 {
								 if (*(pFrame+2)==0x4D && *(pFrame+3)==0x4F && *(pFrame+4)==0x44 && *(pFrame+5)==0x45 
										 && *(pFrame+6)==0x4D && *(pFrame+7)==0x3A && *(pFrame+8)==0x53 && *(pFrame+9)==0x54
										 && *(pFrame+10)==0x41 && *(pFrame+11)==0x52 && *(pFrame+12)==0x54 && *(pFrame+13)==0x55
										 && *(pFrame+14)==0x50)
								 {
									 wlFlagsSet.startup = TRUE;
								 }	
							 }
									
							 //0d 0a 2b 54 43 50 53 45 54 55 50 3a 30 2c 46 41 49 4c 0d 0a(+TCPSETUP:0,FAIL)
							 //0d 0a 2b 54 43 50 53 45 54 55 50 3a 30 2c 4f 4b 0d 0a(+TCPSETUP:0,OK)
							 if (*(pFrame+2)==0x2B && *(pFrame+3)==0x54 && *(pFrame+4)==0x43 && *(pFrame+5)==0x50 
									 && *(pFrame+6)==0x53 && *(pFrame+7)==0x45 && *(pFrame+8)==0x54 && *(pFrame+9)==0x55
									 && *(pFrame+10)==0x50 && *(pFrame+11)==0x3a && *(pFrame+12)==0x30 && *(pFrame+13)==0x2c
									)
							 { 
								 if (*(pFrame+14)==0x4f && *(pFrame+15)==0x4b)
								 {
									 ipPermit = TRUE;
									 ret = IP_PERMIT;
								 }
								 
								 if (*(pFrame+14)==0x46 && *(pFrame+15)==0x41 && *(pFrame+16)==0x49 && *(pFrame+17)==0x4c)
								 {												
									 if (wlFlagsSet.numOfConnect<3)
									 {
										 wlFlagsSet.numOfConnect++;
										 wlFlagsSet.ipStarted = FALSE;
												
										 printf("TCP��������ʧ��,������\n");
									 }
									 else
									 {
										 printf("TCP������������4�ζ�ʧ��\n");
										 ret = LINK_FAIL;
									 }
								 }								 
							 }														 
							 break;
									
						 case 33:
							 //0d 0a 2b 43 47 52 45 47 3a 20 32 2c 31 2c 22 30 30 30 30 22 2c 22 30 30 30 30 30 30 30 30 22 0d 0a
							 if (atType==AT_CGREG)
							 {
								 if (*(pFrame+12)==0x31 || *(pFrame+12)==0x35)    //����ע�������ע��ɹ�
								 {
									 wlFlagsSet.cgreg = AT_M_RESPONSE_INFO_ACK;
								 }
							 }
							 break;
							 
						 default:
						 	 printf("����\n");
							 //0d 0a 2b 49 43 43 49 44 3a 20 38 39 38 36 30 33 31 37 34 39 32 30 35 31 37 39 38 39 36 33 0d 0a 
							 if (*(pFrame+2)==0x2B && *(pFrame+3)==0x49 && *(pFrame+4)==0x43 && *(pFrame+5)==0x43 && *(pFrame+6)==0x49
									 && *(pFrame+7)==0x44 && *(pFrame+8)==0x3a
									)
							 {
								 wlFlagsSet.cardInserted=AT_M_RESPONSE_INFO_ACK;
								 
								 printf("CCID(����)\r\n");
							 }
							 break;
					}
				}
				break;

   }
   
   return ret;
}

/***************************************************
��������:closeLinkReport
��������:�������ر����Ӵ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void closeLinkReport(void)
{
    /*
    INT16U tmpData;
        
    //��¼�������ر����Ӵ���
  	if (realStatisBuff[LINK_RESET_TIMES+1]==0xee)
  	{
  	 	  realStatisBuff[LINK_RESET_TIMES]   = 0x1;
  	 	  realStatisBuff[LINK_RESET_TIMES+1] = 0x0;
  	}
  	else
  	{
  	 	  tmpData = (realStatisBuff[LINK_RESET_TIMES] | realStatisBuff[LINK_RESET_TIMES+1]<<8)+1;
  	 	  realStatisBuff[LINK_RESET_TIMES]   = tmpData&0xff;
  	 	  realStatisBuff[LINK_RESET_TIMES+1] = tmpData>>8 & 0xff;
  	}
  	*/
}

/***************************************************
��������:resetAllFlag
��������:��λ�������߱�־,׼�����¿�ʼһ���������ӹ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void resetAllFlag(INT8U ipStartReset)
{
	atType = AT_NONE;  //ly,2011-10-10 add,����ֲ�����ַM590E,CM180��Զ�ǲ��ϵ�����
	
	wlFlagsSet.startup = FALSE;                   //ATָ�������ʼ	

  wlFlagsSet.ipr = FALSE;                       //������������δ�ɹ�

  wlFlagsSet.callReady = FALSE;                 //Modem׼����?

  wlFlagsSet.echo = FALSE;                      //�رջ��Ա�־
  wlFlagsSet.cardInserted = AT_M_RESPONSE_NONE; //�Ƿ�忨��־?
  wlFlagsSet.dcd = FALSE;                       //DCD����
  wlFlagsSet.cgdcont = FALSE;                   //����CGDCONT�����ɹ�����־
  wlFlagsSet.signal = AT_M_RESPONSE_NONE;       //��ȡ�ź�����
  wlFlagsSet.ipStarted = FALSE;                 //START UP TCP OR UDP CONNECTION
  wlFlagsSet.wlLcTcpPort = FALSE;               //���ñ���TCP�˿�    ly,2011-12-28,add
  wlFlagsSet.creg = AT_M_RESPONSE_NONE;         //ע��������?        ly,2012-02-27,add
  wlFlagsSet.cgreg = AT_M_RESPONSE_NONE;        //GPRSע��������?    ly,2012-05-18,add
  wlFlagsSet.cops = AT_M_RESPONSE_NONE;         //��Ӫ�̲�ѯ  ,2018-12-27,add

  wlFlagsSet.ipMode = FALSE;                    //Select TCPIP Application mode
  wlFlagsSet.simTransparent = FALSE;            //͸������
  wlFlagsSet.simApn = FALSE;                    //APN
  wlFlagsSet.simCiicr = 0;                      //�����ƶ�����
  wlFlagsSet.simCifsr = FALSE;                  //��ȡ����IP��ַ
  wlFlagsSet.simCgatt = AT_M_RESPONSE_NONE;     //���źͷ���GPRS

  wlFlagsSet.gr64Ips = FALSE;                   //GR64����IP����IPS  
  wlFlagsSet.gr64Ipa = FALSE;                   //GR64 IPA
  wlFlagsSet.numOfGr64Ipa = 0;
  wlFlagsSet.gr64Addr = FALSE;                  //��ȡPDP IP��ַ?
  
  wlFlagsSet.dtgsRegNet = AT_M_RESPONSE_NONE;   //DTGS-800ģ��ע������?
  wlFlagsSet.dtgsTime = FALSE;                  //DTGS-800ʱ���Ѷ�ȡ? 
  wlFlagsSet.dtgsCrm = FALSE;                   //DTGS-800����ͨ��Э��
  wlFlagsSet.dtgsDip = FALSE;                   //DTGS-800����Ŀ��IP��ַ
  wlFlagsSet.dtgsDPort = FALSE;                 //DTGS-800Ŀ�Ķ˿�
  wlFlagsSet.dtgsDial = FALSE;                  //DTGS-800����ָ���Ƿ���
  wlFlagsSet.dtgsSpc = FALSE;                   //DTGS-800����SPC��
  wlFlagsSet.dtgsAinf = FALSE;                  //DTGS-800��������TCP/IPЭ���Ȩ��Ϣ
  wlFlagsSet.dialConnected = FALSE;             //����δ�ɹ�
  
  wlFlagsSet.mc5xSetCreg  = FALSE;              //MC5xע����ʾ
  wlFlagsSet.mc5xQueryCreg= AT_M_RESPONSE_NONE; //MC5x���ע�����
  wlFlagsSet.mc5xSetCGreg = FALSE;              //MC5xGPRSע����ʾ
  wlFlagsSet.mc5xQueryCGreg=AT_M_RESPONSE_NONE; //MC5x���GPRSע�����
  wlFlagsSet.mc5xSelectConn = FALSE;            //MC5xѡ����������
  wlFlagsSet.mc5xSocket = FALSE;                //MC5x���÷�������ΪSocket
  wlFlagsSet.mc5xConId = FALSE;                 //MC5x��������ID
  wlFlagsSet.mc5xIpPort = FALSE;                //MC5x����IP��ַ���˿�
  wlFlagsSet.mc5xDial = FALSE;                  //MC5x���������ѷ���
  wlFlagsSet.mc5xLastSended = TRUE;             //MC5x��һ֡�ѷ���  
  wlFlagsSet.wlSendStatus = 0;                  //MC5x����״̬
  wlFlagsSet.mc5xRecvStatus = 0;                //MC5x����״̬
  
  wlFlagsSet.m590eQueryCreg = 0;                  //M590Eע��״̬
  wlFlagsSet.m590eXisp      = FALSE;              //M590E����Ϊ�ڲ�Э��ջ?
  wlFlagsSet.m590eSetXiic   = FALSE;              //����PPP����
  wlFlagsSet.m590eQueryXiic = 0;                  //��ѯPPP����
  wlFlagsSet.numOfConnect   = 0;                  //TCP���Ӵ���

  wlFlagsSet.cm180plogin    = FALSE;              //���ü�Ȩ��Ϣ
  wlFlagsSet.cm180QueryCreg = FALSE;              //��ѯģ������ע��״̬
  wlFlagsSet.cm180pppStatus = AT_M_RESPONSE_NONE; //PPP����״̬
  wlFlagsSet.cm180Dial      = FALSE;              //����PPP����
  wlFlagsSet.cm180Getip     = AT_M_RESPONSE_NONE; //��ȡģ��IP��ַ
  wlFlagsSet.cm180pnum      = FALSE;              //�趨����ҵ�����
  wlFlagsSet.cm180ipStatus  = AT_M_RESPONSE_NONE; //TCP����״̬

  wlFlagsSet.m72qifgcnt     = 0;                  //������
  wlFlagsSet.m72Apn         = 0;                  //APN
  wlFlagsSet.m72mux         = 0;                  //MUX
  wlFlagsSet.m72IpMode      = 0;                  //IP Mode
  wlFlagsSet.m72Itcfg       = 0;                  //
  wlFlagsSet.m72dnsip       = 0;                  //
  wlFlagsSet.m72regapp      = 0;                  //
  wlFlagsSet.m72act         = 0;                  //
  wlFlagsSet.m72locip       = 0;                  //
  

  flagSwitch = FALSE;            //ת��GPRSģʽ��AT����ģʽ��־
  atWait0d0a=0;
  
  ipPermit = FALSE;              //������IP��
  ifLogin = 0;                   //�Ƿ��͵�¼����

  ifAtToGprs = 1;                //�Ƿ�������ģʽת������ģʽ

  rssi = 99;                     //GPRS/CDMA�����ź�ǿ��ָʾ
  
  modemPara.reportSignal(0,0);

	strcpy(cops, "");
  
  /*
  guiLine(1,1,31,10,0);
  switch(modemPara.moduleType)
  {
     case GPRS_SIM300C:
       guiAscii(19,1,"GS",1);
       break;

     case GPRS_GR64:
       guiAscii(19,1,"GE",1);
       break;

    #ifdef USE_ON_BOARD_ETH
     case ETHERNET:
   	   NIC_Init();
       guiAscii(12,1,"ETH",1);
       break;
    #endif
       
     case CDMA_DTGS800:
       guiAscii(19,1,"C",1);
       break;
       
     default:
       guiAscii(19,1,"N",1);
       break;
  }

  //netStatus &= 0x4;

  //if (menuInLayer==0 && setParaWaitTime==0xfe)
  //{
  //  defaultMenu();
  //}
  */
  
  bitErr = 99;                   //GPRS/CDMA�ŵ�λ����
  
  //loginSuccess = FALSE;
  
  if (ipStartReset==0)
  {
    //ipStartSecond = 0;
  }
  
  retry = 0;
  
  wlFlagsSet.atFrame = FRAME_NONE;
  tailOfWireless  = 0;
  tailOfWireFrame = 0;
}

/***************************************************
��������:trim
��������:ȥ���ַ�������Ŀո�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
char * trim(char * s)
{
   INT8U i,j,len,numOfPrevSpace;
   
   //ȥ���ַ���ǰ��Ŀո�
   len = strlen(s);
   
   for(i=0;i<len;i++)
   {
  	  if (s[i]!=0x20)
  	  {
  	  	 break;
  	  }
   }

   if (i>0)
   {
     numOfPrevSpace = i+1;
     for(j=0;j<len;j++)
     {
     	  if (j+numOfPrevSpace<=len)
     	  {
     	     s[j] = s[i++];
     	  }
     	  else
     	  {
     	  	 s[j] = 0x0;
     	  }
     }
   }

   //ȥ���ַ�������Ŀո�
   len = strlen(s);
   for(i=len;i>0;i--)
   {
  	  if (s[i-1]!=0x20)
  	  {
  	  	 break;
  	  }
  	  if (s[i-1]==0x20)
  	  {
  	  	 s[i-1] = 0x0;
  	  }
   }

   len = strlen(s);
   
   return s;
}

/***************************************************
��������:loginIpAddr
��������:��¼IP��ַ
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
char ipAddress[20];
char * loginIpAddr(void)
{
   char str[10];
   
   strcpy(ipAddress,intToString(modemPara.loginIp[0],3,str));
   strcat(ipAddress,".");
   strcat(ipAddress,intToString(modemPara.loginIp[1],3,str));
   strcat(ipAddress,".");
   strcat(ipAddress,intToString(modemPara.loginIp[2],3,str));
   strcat(ipAddress,".");
   strcat(ipAddress,intToString(modemPara.loginIp[3],3,str));
   
   return ipAddress;
}

/***************************************************
��������:loginPort
��������:��¼�˿�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
char port[10];
char * loginPort(void)
{
   char str[10];
   
   strcpy(port,intToString(modemPara.loginPort,3,str));
   
   return port;
}

/***************************************************
��������:digitalToChar
��������:��һλ����ת��Ϊ��Ӧ���ַ�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
char*	digitalToChar(INT8U digital)
{
  switch(digital)
	{
	  case	0:
			return	"0";
			break;
		case	1:
			return	"1";
			break;
		case	2:
			return	"2";
			break;
		case	3:
			return	"3";
			break;
		case	4:
			return	"4";
			break;
		case	5:
			return	"5";
			break;
		case	6:
			return	"6";
			break;
		case	7:
			return	"7";
			break;
		case	8:
			return	"8";
			break;
		case	9:
			return	"9";
			break;			
		case 0xa:
			return	"A";
			break;			
		case 0xb:
			return	"B";
			break;			
		case 0xc:
			return	"C";
			break;			
		case 0xd:
			return	"D";
			break;			
		case 0xe:
			return	"E";
			break;			
		case 0xf:
			return	"F";
			break;			
	  default:
			return	"X";
			break;
	}
}

/***************************************************
��������:intToString
��������:��һ����(0--65535)ת��Ϊ������(/ʮ����)�ַ���
         >65535ת����32λ������
         >255ת����16λ�����ƣ�С�ڵ���255ת����8λ������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
char * intToString(INT32U in,int type,char *returnStr)
{
  char   tmpStr[33];
	INT8U  i;
	INT8U  bits;
	INT32U divisor=1000000000;
  strcpy(returnStr,"");

  switch(type)
  {
    case 1:            //�������ַ���
   	  if (in>65535)
   	  {
	      bits=32;
	    }
	    else
	    {
	   	  if (in>255)
	 	      bits=16;
	      else
	 	      bits=8;
	 	  }
      for (i=0;i<bits;i++)
	    {
	 	     strcpy(tmpStr,digitalToChar(in%2));
	 	     strcat(tmpStr,returnStr);
	 	     strcpy(returnStr,tmpStr);
	 	     in=in/2;
	    }
	    returnStr[bits]='\0';
	    break;
	    
	  case 2:
      bits=16;
      for (i = 0;i < bits;i++)
	    {
	 	     strcpy(tmpStr,digitalToChar(in%2));
	 	     strcat(tmpStr,returnStr);
	 	     strcpy(returnStr,tmpStr);
	 	     in=in/2;
	    }
	    returnStr[bits]='\0';
	    break;
	    
	  case 3:
   	  if (in==0)
   	  {
   	    strcpy(returnStr,"0");
   	  }
   	  else
   	  {
   	     bits=0;
   	     for(i=0;i<10;i++)
   	     {
   	  	    if (bits==0)
   	  	    {
   	  	       if ((in/divisor)!=0)
   	  	       {
   	  	 	        strcat(returnStr,digitalToChar(in/divisor));
   	  	 	        in=in%divisor;
   	  	 	        bits++;
   	  	       }
   	  	    }
   	  	    else
   	  	    {
   	  	       if ((in/divisor)!=0)
   	  	       {
   	  	 	        strcat(returnStr,digitalToChar(in/divisor));
   	  	       }
   	  	       else
   	  	       {
   	  	 	        strcat(returnStr,"0");
   	  	 	     }

   	  	 	     in=in%divisor;
   	  	 	     bits++;
   	  	    }
   	  	    divisor=divisor/10; 
   	     }
      }
      break;
  }
  return returnStr;
}

/***************************************************
��������:wlModemRequestSend
��������:������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void wlModemRequestSend(INT8U *buf, INT16U len)
{
	 INT8U  atCmd[50];
	 char   str[20];
	 
	 //ly,2011-10-10,add����ж�
	 if (len==0)
	 {
	 	 printf("wlModemRequestSend:�����͵����ݳ���Ϊ0,����\n");
	 	 return;
	 }
	 
	 if (wlFlagsSet.wlSendStatus>0 && wlFlagsSet.wlSendStatus<4)  //��֡���ڷ���
	 {
	 	 printf("wlModemRequestSend:���������ڷ���,����\n");
	 	 return;
	 }
	 
	 if (modemPara.moduleType==CDMA_CM180)
	 {
	 	 strcpy((char *)atCmd,"AT+TCPSEND=0,");
	 	 strcat((char *)atCmd,intToString(len, 3, str));
	 	 strcat((char *)atCmd,"\r");
     modemPara.sendFrame(atCmd,strlen((char *)atCmd));
     
     //��������
     modemPara.sendFrame(buf, len);
	 }
	 else
	 {	 
	   memcpy(sendBuffer, buf, len);
	   sendLength = len;
	   if (modemPara.moduleType==GPRS_M590E)
	   {
	 	   sendBuffer[sendLength] = '\r';
	 	 }
	   wlFlagsSet.wlSendStatus = 1;  //��֡Ҫ����
	   
	   sendTimeOut = 0;
	 }
}

