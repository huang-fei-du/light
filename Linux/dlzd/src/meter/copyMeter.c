/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ�����copyMeter.c
���ߣ�leiyong
�汾��0.9
������ڣ�2007��4��
����:�����ն�(�����ն�/������,AT91SAM9260������)�����߳��ļ�
�����б�
     1.
�޸���ʷ��
  01,07-04-13,Leiyong created.
  02,07-07-02,Leiyong,modify,Сʱ��������дFLASH����
  03,07-07-09,Leiyong,�޸Ľ�����������ܱ��ڲ��������ò���(F10)�е�λ���޹���
  04,07-09-14,Tianye����645�����Լ(����������������������������...)
  05,09-03-10,Leiyong�޸�
              1)������������ʵʱͳ������2(realStatisBuff2)��
              2)����ɹ����жϲ��ǳ��������,�������������(havePresentData!0)��ֹδ���������������С��6���Ϊ����ɹ�
  06,09-07-20,Leiyong����convertAcSample����,������ת����һ������ʵ��,����ת���ĵط����˸���һ���ط��������ط�δ�ĵ�
  07,10-08-04,Leiyong,���Ӳɼ���λ�������ת��
  08,10-08-17,Leiyong,�������������˾Ҫ��ĵ��ܱ�����(����һ�㹤��ҵ������һ�㹤��ҵ�������û����,��Щ��ֻ����07��Լ,��Ϊ��Ҫ��������97�����û��)
  09,10-08-18,Leiyong,�޸�,�ز��˿ڳ������̶�Ϊ5min,��ֹδ���ó�����ʱ�����ز���
  10,10-08-18,Leiyong,�޸�,�ز��˿ڳ������̶�Ϊ10min
  11,l0-11-22,Leiyong,�޸�,�ز��˿ڳ���ʱ��̶�ΪÿСʱ��1��
  12,10-11-22,Leiyong,�޸�,���㶳�����ݲɼ���
              ����ĳЩ�������㶳�����ݲ����ڣ������Ļ����ͻ�ÿСʱ����һ������㶳������
              10-11-22���Ѹĳ����û�����øò������ʾֵ���ߵ�����Ͳ��ɼ���
  13,10-11-22,Leiyong,�޸�,��Щ������ͳ������Ĵ���
              �ĳ����ĳ����������������DIδ�����Ļ�(��2*2,ÿ��������Ҫ�ط�һ��)��ֹͣ�ò����㱾��ĳ���,
              �����˷�������ĳ���ʱ�䡣
  14,11-07-07,Leiyong,�޸Ķ�������΢������Ϊ·������������ʽ��
  15,13-10-17,Leiyong,��Ӷ������������ĳ�����
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

extern BOOL  secondChanged;         //���Ѹı�
extern DATE_TIME nextHeartBeatTime; //��һ������ʱ��

//���ֻ��͹�������**********************************************
//�ɼ�������/����������
INT8U        stOfSwitchRead;                    //��ȡ״̬��״̬����
INT8U        tmpSwitch,tmpCd;
INT8U        reportToMain;                      //״̬���¼��Ƿ�����վ�ϱ�
INT8U        stOfSwitchBaks[10];                //10�α���״̬��״̬
INT8U        numOfStGather=0;                   //�ɼ�״̬������
INT8U        ifSystemStartup = 0;               //ϵͳ������?

//���ֻ��͹�������**********end*********************************

INT8U        pulsei;
INT8U        hasEvent=0;
INT32U       tmpPulseData;

struct dotFn129 *pDotFn129=NULL;
INT8U        dotReplyStart = 0;


INT8U        abbHandclasp[3]={0,0,0};           //ABB�����Ƿ�ɹ�

//**************************************************************











//***********************************��������****************************************************

//Modbus CRC У���㷨
//�㷨һ
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

//�㷨��,���
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
��������:compareTwoAddr
��������:�Ƚ�������ַ�Ƿ�һ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE-һ��,FALSE-��һ��
*******************************************************/
BOOL compareTwoAddr(INT8U *addr1, INT8U *addr2, INT8U type)
{
   if (type==1)    //�Ƚϵ�ַ1�Ƿ�Ϊȫ0
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
��������:eastMsgSwap
��������:����ɼ����е�ַ��ʽ�ı���ת��
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
  	printf("����ɼ����е�ַ����:���ձ��Ľ�����ַ����\n");
  }
}

/*******************************************************
��������:sendMeterFrame
��������:���͵��ܱ�����֡
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
     //2012-3-28,ͳһΪ�˿�1
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
             printf("�˿�%dABB��������,ʹ��sendXmegaInTimeFrame����\n", port);
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
        
     //2012-3-28,ͳһΪ�˿�2
     case PORT_NO_2:
       //������֧�ּ���,���2·�����ΪttyS4;
       //���������ּ���,���2·�����ΪttyS3
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
             printf("�˿�%dABB��������,ʹ��sendXmegaInTimeFrame����\n", port);
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
             printf("�˿�%dABB��������,ʹ��sendXmegaInTimeFrame����\n", port);
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
     	  	printf("sendMeterFrame:����ǰ�ȴ�1s\n");
     	  	sleep(1);
     	  }
      	
      	write(fdOfCarrier, pack, length);
      	
      	carrierFlagSet.cmdTimeOut  = nextTime(sysTime, 0, CARRIER_TIME_OUT);  //��ʱʱ��
      	carrierFlagSet.sending     = 1;                                       //���ڷ�������
      	carrierFlagSet.cmdContinue = 0;                                       //��������������0
      	carrierFlagSet.retryCmd    = 0;                                       //���ͳ�ʱ��0
      	
      	carrierFlagSet.numOfCarrierSending++;                                 //ly,2011-05-27,Ϊ��Ӧ����΢
      	if (debugInfo&PRINT_CARRIER_DEBUG)
      	{
      		 //printf("�ز�/���߶˿ڷ��ͼ���=%d\n", carrierFlagSet.numOfCarrierSending);
      	}
      	
      	//��Լ1����Сʱ(��ͬ���ز�ģ�鲻һ��)������ز��޷�Ӧ��Ҫ����������
       #ifdef LM_SUPPORT_UT
        if (0x55!=lmProtocol)
        {
       #endif
      	  
      	  if (carrierFlagSet.numOfCarrierSending>100)
      	  {
      	    if (debugInfo&PRINT_CARRIER_DEBUG)
      	    {
      		    printf("�ز�/���߶˿ڷ��ͼ����ﵽ100��,�޽���,��λ������\n");
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
��������:threadOf485x1Received
��������:485�ӿ�1���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
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
     
     if (copyCtrl[tmpPort].pForwardData!=NULL)  //ת��
     {
       if (copyCtrl[tmpPort].pForwardData->ifSend == TRUE)
       {
     	 if (debugInfo&METER_DEBUG)
     	 {
           printf("%02d-%02d-%02d %02d:%02d:%02d Rx1(ת��):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
           for(i=0;i<recvLen;i++)
           {
    	     printf("%02x ",tmpBuf[i]);
           }
           printf("\n");
     	 }

     	 if (copyCtrl[tmpPort].pForwardData->fn==1)
     	 {
     	   //���յ��ظ����ݺ�ȴ�����,���Ƿ��к����ֽ�
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
��������:threadOf485x2Received
��������:485�ӿ�2���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void *threadOf485x2Received(void *arg)
{
  INT8U  recvLen;
  INT8U  tmpBuf[52];
  INT8U  i;
  INT8U  tmpPort;
  
  while (1)
  {
    //������֧�ּ���,���2·�����ΪttyS4;
    //���������ּ���,���2·����ƷΪttyS3
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

     if (copyCtrl[tmpPort].pForwardData!=NULL)  //ת��
     {
       if (copyCtrl[tmpPort].pForwardData->ifSend == TRUE)
       {
     	 if (debugInfo&METER_DEBUG)
     	 {
           printf("%02d-%02d-%02d %02d:%02d:%02d Rx2(ת��):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
           for(i=0;i<recvLen;i++)
           {
    	     printf("%02x ",tmpBuf[i]);
           }
           printf("\n");
     	 }
     	  
     	 if (copyCtrl[tmpPort].pForwardData->fn==1)
     	 {
     	   //���յ��ظ����ݺ�ȴ�����,���Ƿ��к����ֽ�
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
��������:queryMeterStoreInfo
��������:��ѯ���洢��Ϣ
���ú���:
�����ú���:
�������:INT8U portNum,...
�������:*meterInfo,��0�ֽ� - �������(1-�������ܱ�,2-���౾�طѿر�,3-����Զ�̷ѿر�,4-�������ܱ�,5-���౾�طѿر�,6-����Զ�̷ѿر�,7-485�����(���ݳ�����ȫ),8-�ص��û�
                    ��1�ֽ� - ʵʱ���ݴ洢����
                    ��2�ֽ� - �ն������ݴ洢����
                    ��3�ֽ� - �¶������ݴ洢����
                    ��4�ֽ� - ʵʱ���ݳ��ȵ�8λ
                    ��5�ֽ� - ʵʱ���ݳ��ȸ�8λ
����ֵ��״̬
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
  if ((meterConfig.rateAndPort&0x1f)==31)  //�ز��˿�
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
    	  case 3:   //����һ�㹤��ҵ
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

    	  case 4:   //����һ�㹤��ҵ
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
         
       case 5:  //�����û�
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
  else                                    //485�˿�
  {
    if ((meterConfig.bigAndLittleType&0xf)==1)//�������ܱ�
    {
      *meterInfo = 1;
      *(meterInfo+1) = SINGLE_PHASE_PRESENT;
      *(meterInfo+2) = SINGLE_PHASE_DAY;
      *(meterInfo+3) = SINGLE_PHASE_MONTH;
      *(meterInfo+4) = LENGTH_OF_SINGLE_ENERGY_RECORD&0xff;
      *(meterInfo+5) = LENGTH_OF_SINGLE_ENERGY_RECORD>>8;
    }
    else                                   //485�����
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
��������:xMegaCopyData
��������:485�ӿ�1���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
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
  
  if (copyCtrl[port].pForwardData!=NULL)  //ת��
  {
  	 if (copyCtrl[port].pForwardData->ifSend == TRUE)
  	 {
  	   if (debugInfo&METER_DEBUG)
  	   {
         printf("%02d-%02d-%02d %02d:%02d:%02d �˿�%d(ת��):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port);
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
  
    	     //��ΪxMega�Ѿ���������,���Բ����ٵȴ�,��ֱ�ӻظ���վ
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
��������:happenRecovery485
��������:485���Ϸ���/�ָ�
���ú���:
�����ú���:
�������:
�������:
����ֵ��״̬
***************************************************/
void happenRecovery485(INT8U type)
{  	 
  DATE_TIME              tmpTime;
  TERMINAL_STATIS_RECORD terminalStatisRecord;    //�ն�ͳ�Ƽ�¼
  INT8U                  eventData[10];
 
  //2015-12-09,���485���ϻָ���¼һ�εĴ���
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
			  printf("%02d-%02d-%02d %02d:%02d:%02d,485ͨ�Ź��Ϸ���,��¼�¼�.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			}
			else
		  {
			  printf("%02d-%02d-%02d %02d:%02d:%02d,485ͨ�Żָ�,��¼�¼�.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			}
		}

 	  //��¼�ն˹��� - 485������ϻָ�
    if ((eventRecordConfig.iEvent[2] & 0x10) || (eventRecordConfig.nEvent[2] & 0x10))
    {
 	    eventData[0] = 0x15;
 	    eventData[1] = 0xa;
 	      
 	    //����ʱ��
 	    eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
	    eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
	    eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
	    eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
	    eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
	    eventData[7] = sysTime.year/10<<4   | sysTime.year%10;

	    //���ϱ���--485������ϻָ����Ƿ���
	    eventData[8] = 0x4;    //�ָ�bit7=0
			if (type==1)
			{
				eventData[8] |= 0x80;    //����bit7=1
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
	      
	    activeReport3();   //�����ϱ��¼�
	     
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
��������:lcProcess
��������:��ش���
���ú���:
�����ú���:
�������:
�������:
����ֵ��״̬
***************************************************/
void lcProcess(INT8U *lastLux, INT32U nowLux)
{
	struct ctrlTimes  *tmpCTimes;
	struct ctrlTimes  *thisCTimes;
  INT8U             tmpLightMode=0;
  INT8U             tmpBright=0;
  INT8U             checkStep=0;    //�Աȿ���ʱ�β���
  struct cpAddrLink *tmpCpLink;
  INT16U            tmpi;
  INT32U            tmpLux=0;
  INT32U            lastLuxValue = lastLux[0] | lastLux[1]<<8 | lastLux[2]<<16;        //��һ��ֵ
  INT32U            lastLastLuxValue = lastLux[3] | lastLux[4]<<8 | lastLux[5]<<16;    //���ϴ�ֵ
  INT32U            tmpData = 0;
  INT16U            openLux = 0xfffe;     //��բ����ֵ
  INT16U            closeLux = 0xfffe;    //��բ����ֵ
  DATE_TIME         startJudgeTime, endJudgeTime;    //2017-01-10,Add

  tmpCTimes  = cTimesHead;
  thisCTimes = NULL;
  while(tmpCTimes!=NULL)
  {
  	//���նȿ���
  	if (7==tmpCTimes->deviceType)
  	{
 	  	if (
					//��ʼ�ºͽ����²���ͬ,2016-10-21
					(tmpCTimes->startMonth!=tmpCTimes->endMonth
						&&
						 (
							(
							 tmpCTimes->startMonth==sysTime.month        //����ʼʱ������ͬ��������
								&& tmpCTimes->startDay<=sysTime.day        // ����ʼ��С�ڵ��ڵ�ǰ��
							)
							 ||
								( 
								 tmpCTimes->endMonth==sysTime.month        //�����ʱ������ͬ��������
									&& tmpCTimes->endDay>=sysTime.day        // �ҽ����մ��ڵ�ǰ��
								)
						 )
					)
				 
					 ||    //2016-10-21,������ʼ�ºͽ�������ͬ�Ĵ���
						(
						 tmpCTimes->startMonth==tmpCTimes->endMonth    //��ʼ�ºͽ�������ͬ
							&& tmpCTimes->startMonth==sysTime.month      //�ҵ��ڵ�ǰʱ����
							 && tmpCTimes->startDay<=sysTime.day         //  ��ʼ��С�ڵ�ǰʱ����
								&& tmpCTimes->endDay>=sysTime.day          //  �����մ��ڵ�ǰʱ����
						)
 	  		   
 	  		     //������>��ʼ��,����ʼ�ͽ��������
 	  		     || (
 	  		         tmpCTimes->startMonth<tmpCTimes->endMonth
 	  		          && tmpCTimes->startMonth<sysTime.month
 	  		           && tmpCTimes->endMonth>sysTime.month
 	  		        )
 	  		    
 	  		      || (    //������<��ʼ��
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
    //2016-08-24,��������������ڼ�
    tmpData = dayWeek(2000+sysTime.year, sysTime.month, sysTime.day);
	  if (debugInfo&METER_DEBUG)
    {
      printf("%02d-%02d-%02d %02d:%02d:%02d,����������%d",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tmpData);
    }
    
    //�ټ��������ձȽ�����
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
      printf(",�Ƚ�����=%02X,���õ�������=%02X,", tmpData, thisCTimes->workDay);
    }

	  //����������
	  if ((thisCTimes->workDay & tmpData)!=tmpData)
    {
	    if (debugInfo&METER_DEBUG)
      {
        printf("��δ������\n");
      }
        
      //2016-08-24,Ŀǰû�д�������������л���δ�����ջ��෴�Ĳ���
    }
    else
    {
	    if (debugInfo&METER_DEBUG)
      {
        printf("��������\n");
      }
    
      //������·��ģʽ�������ģʽ
      tmpLightMode = 0;    //0-·��ģʽ
      tmpLux = 0xffffff;
      tmpBright = 0xff;
   	  for(tmpi=0; tmpi<6; tmpi++)
  	  {
  	  	if (thisCTimes->bright[tmpi]<=100)    //������ֵ��Ч
  	  	{
  	      if (thisCTimes->bright[tmpi]!=0 && thisCTimes->bright[tmpi]!=1)
  	      {
  	        if (tmpBright!=0xff)
  	        {
  	      	  if (tmpLux<(thisCTimes->hour[tmpi] | thisCTimes->min[tmpi]<<8) && tmpBright<thisCTimes->bright[tmpi])
  	      	  {
  	      		  tmpLightMode = 1;    //1-���ģʽ
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
               "%02d-%02d-%02d %02d:%02d:%02d,���,��ʱ���(%d��%d��-%d��%d��)��",
                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
                 thisCTimes->startMonth, thisCTimes->startDay, thisCTimes->endMonth, thisCTimes->endDay
              );
      	if (tmpLightMode==1)
      	{
      		printf(",���ģʽ\n");
      	}
      	else
      	{
      		printf(",·��ģʽ\n");
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
      	//2016-08-24,����ֵ�޸�Ϊ��ԭʼֵ�·�,�����ٳ���10,����Ϊ1��������10
      	tmpLux = thisCTimes->hour[tmpi] | thisCTimes->min[tmpi]<<8;
      	
      	if (tmpLightMode==1)    //���ģʽ
      	{
    			//�Ѵ򿪵�Դ
      	  tmpCpLink = xlcLink;
          while(tmpCpLink!=NULL)
          {
    			  if (1==tmpCpLink->lcOnOff)
    			  {
    			    //���βɼ������ڵ����ն�����ֵ
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
      	               "%02d-%02d-%02d %02d:%02d:%02d,���,���ģʽ,����·���Ƶ�%02x%02x%02x%02x%02x%02x��բ��־\n",
      	                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
      	                  tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
      	              );
      	      }
      	    }
    			}
      	}
      	else    //·��ģʽ
      	{
      		//2016-08-24,����բ�ն�ֵ��������
      		if (thisCTimes->bright[tmpi]==0x00)
      		{
      		  openLux = tmpLux;    //��բ����ֵ
      		}
      		else
      		{	
      		  //2016-08-24,����բ�ն�ֵ��������
      		  if (thisCTimes->bright[tmpi]==0x01)
      		  {
        			closeLux = tmpLux;    //��բ����ֵ
        		}
        		else
        		{
              if (
              	  CTRL_MODE_LIGHT==ctrlMode              //���õļ���������ģʽΪ���
              	   || CTRL_MODE_PERIOD_LIGHT==ctrlMode   //���õļ���������ģʽΪʱ�οؽ�Ϲ��
              	    || CTRL_MODE_LA_LO_LIGHT==ctrlMode   //���õļ���������ģʽΪ��γ�Ƚ�Ϲ��
              	 )
              {
          			tmpCpLink = xlcLink;
                while(tmpCpLink!=NULL)
                {
          			  //���д���·���Ƶ��Դ
          			  if (1==tmpCpLink->lcOnOff)
          			  {
          			    //���βɼ���С�ڵ����趨����ֵ
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
								 "%02d-%02d-%02d %02d:%02d:%02d,���,��բ����ֵ=%dLux,��բ����ֵ=%dLux\n",
									sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
										closeLux, openLux
								);
				}
      
  			//��ط�բ�ж�
  			if (
  				  //������βɼ�����ֵ�����ڵ���(��բ����ֵ-pnGate.lcWave)(-pnGate.lcWave�����ն���)
  				  (openLux<=(nowLux+pnGate.lcWave))
  				   && (openLux<=(lastLuxValue+pnGate.lcWave))
  				    && (openLux<=(lastLastLuxValue+pnGate.lcWave))
  				     
  				     && (                                             //2016-03-21,�ڲ����з���,���ڹص�ֵ������ֵ֮��ʱ,�����÷�բ���ú�բ,���ǲ���ȷ��,���Ǽ���������:
  				         openLux>closeLux                             //����1:��բ����ֵ���ں�բ����ֵ
  				          ||
  				           (
  				            openLux<closeLux                          //����2:��բ����ֵС�ں�բ����ֵ(��������ֺ�բ����һ��������ֵ���ص���)
  				             && 
  				               (
  				                nowLux>(closeLux+pnGate.lcWave)       //    2.1.��ǰ����ֵ����(��բ����ֵ+��ֵ)
  				                || 
  				                  (
  				                   nowLux<(closeLux+pnGate.lcWave)    //    2.2 ��ǰ����ֵС��(��բ����ֵ+��ֵ)
  				                    && nowLux>lastLuxValue            //    2.2.1����ֵԽ��Խ��,��һ�α�һ�δ�
  				                     && lastLuxValue>lastLastLuxValue 
  				                  )
  				               )
  				           )
  				        )
  				 )
  			{
  			  //�������·��Դ�򿪻���δ֪,����Ϊ�ر�
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
									startJudgeTime.hour   = tmpCpLink->duty[1];    //�ճ�ʱ��-ʱ
									startJudgeTime.minute = tmpCpLink->duty[0];    //�ճ�ʱ��-�� 
									startJudgeTime.second = 0;
									endJudgeTime          = startJudgeTime;
									startJudgeTime        = backTime(startJudgeTime, 0, 0, 0, beforeOnOff[2], 0);
									endJudgeTime          = nextTime(endJudgeTime, beforeOnOff[3], 0);

									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,���,��·���Ƶ�%02x%02x%02x%02x%02x%02x��γ�Ƚ�Ϲ��,�ճ�ʱ��ǰ%02d-%02d-%02d %02d:%02d:%02d���ճ�ʱ�̺�%02d-%02d-%02d %02d:%02d:%02d,",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                             tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0],
														  startJudgeTime.year, startJudgeTime.month, startJudgeTime.day, startJudgeTime.hour, startJudgeTime.minute, startJudgeTime.second,
														   endJudgeTime.year,endJudgeTime.month,endJudgeTime.day,endJudgeTime.hour,endJudgeTime.minute,endJudgeTime.second
													);
									}

									if (
											compareTwoTime(startJudgeTime, sysTime)==TRUE      //���ճ�ʱ��(��բ)ǰ���ٷ�����Ч��
											 && compareTwoTime(sysTime, endJudgeTime)==TRUE    //���ճ�ʱ��(��բ)����ٷ�����Ч��
										 )
									{
										tmpCpLink->lcOnOff = 0;
										tmpCpLink->lcProtectTime = endJudgeTime;    //�ӵ�ǰʱ�̿�ʼ��(beforeOnOff[3])�ڲ���ִ�й������
										
										if (debugInfo&METER_DEBUG)
										{
											printf("�ڴ˷�Χ��,ִ��\n");
										}
									}
									else
									{
										if (debugInfo&METER_DEBUG)
										{
											printf("���ڴ˷�Χ��\n");
										}
									}
								}
								else
								{
									tmpCpLink->lcOnOff = 0;
									
									//�ӵ�ǰʱ�̿�ʼ��(beforeOnOff[3])�ڲ���ִ�й������
									tmpCpLink->lcProtectTime = nextTime(sysTime, beforeOnOff[3], 0);
								}
						
								if (0==tmpCpLink->lcOnOff)
								{
									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,���,·��ģʽ,����·���Ƶ�%02x%02x%02x%02x%02x%02x��բ��־.��բ�󱣻���%02d-%02d-%02d %02d:%02d:%02d\n",
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
  			
				//��غ�բ�ж�
  			if (
  			    //������βɼ���С��(��բ����ֵ+pnGate.lcWave)(+pnGate.lcWave�����ն���)
  				  (nowLux<(closeLux+pnGate.lcWave))
  				   && (lastLuxValue<(closeLux+pnGate.lcWave))
  				    && (lastLastLuxValue<(closeLux+pnGate.lcWave))
  				    
  				     && (                                          //2016-03-21,�ڲ����з���,���ڹص�ֵ������ֵ֮��ʱ,�����÷�բ���ú�բ,���ǲ���ȷ��,���Ǽ���������:
  				         openLux>closeLux                          //����1:��բ����ֵ���ں�բ����ֵ
  				          ||
  				           (
  				            openLux<closeLux                       //����2:��բ����ֵС�ں�բ����ֵ(��������ֺ�բ����һ��������ֵ���ص���)
  				             && 
  				              (
											 	 openLux>=(nowLux+pnGate.lcWave)     //    2.1 ��ǰ����ֵС�ڵ���(��բ����ֵ-��ֵ)
  				                ||
  				                 (
  				                  openLux<(nowLux+pnGate.lcWave)   //    2.2 ��ǰ����ֵ����(��բ����ֵ-��ֵ)
  				                   && nowLux<lastLuxValue          //    2.2.1 ����ֵԽ��ԽС
  				                    && lastLuxValue<lastLastLuxValue
  				                 )
												)
  				           )
  				        )
  				 )
  			{
  			  //�������·��Դ�ر�,����Ϊ��
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
									startJudgeTime.hour   = tmpCpLink->duty[3];    //����ʱ��-ʱ
									startJudgeTime.minute = tmpCpLink->duty[2];    //����ʱ��-�� 
									startJudgeTime.second = 0;
									endJudgeTime          = startJudgeTime;
									startJudgeTime        = backTime(startJudgeTime, 0, 0, 0, beforeOnOff[0], 0);
									endJudgeTime          = nextTime(endJudgeTime, beforeOnOff[1], 0);

									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,���,��·���Ƶ�%02x%02x%02x%02x%02x%02x��γ�Ƚ�Ϲ��,����ʱ��ǰ%02d-%02d-%02d %02d:%02d:%02d������ʱ�̺�%02d-%02d-%02d %02d:%02d:%02d,",
														sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                             tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0],
														  startJudgeTime.year, startJudgeTime.month, startJudgeTime.day, startJudgeTime.hour, startJudgeTime.minute, startJudgeTime.second,
														   endJudgeTime.year,endJudgeTime.month,endJudgeTime.day,endJudgeTime.hour,endJudgeTime.minute,endJudgeTime.second
													);
									}

									if (
											compareTwoTime(startJudgeTime, sysTime)==TRUE      //������ʱ��(��բ)ǰ���ٷ�����Ч��
											 && compareTwoTime(sysTime, endJudgeTime)==TRUE    //������ʱ��(��բ)����ٷ�����Ч��
										 )
									{
										tmpCpLink->lcOnOff = 1;
										tmpCpLink->lcProtectTime = endJudgeTime;    //�ӵ�ǰʱ�̿�ʼ��endJudgeTime�ڲ���ִ�й������
										
										if (debugInfo&METER_DEBUG)
										{
											printf("�ڴ˷�Χ��,ִ��\n");
										}
									}
									else
									{
										if (debugInfo&METER_DEBUG)
										{
											printf("���ڴ˷�Χ��\n");
										}
									}
								}
								else
								{
									tmpCpLink->lcOnOff = 1;
									
									//�ӵ�ǰʱ�̿�ʼ��(beforeOnOff[1])�ڲ���ִ�й������
									tmpCpLink->lcProtectTime = nextTime(sysTime, beforeOnOff[1], 0);
								}
								
								if (1==tmpCpLink->lcOnOff)
								{
									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,���,·��ģʽ,����·���Ƶ�%02x%02x%02x%02x%02x%02x��բ��־.��բ�󱣻���%02d-%02d-%02d %02d:%02d:%02d\n",
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
      
			//2017-01-09,�ֺ�բ�����ں�,�ָ���ز���
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
										 "%02d-%02d-%02d %02d:%02d:%02d,���,�����·���Ƶ�%02x%02x%02x%02x%02x%02x��طֺ�բ��������\n",
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
					  //lcOnOff=1,��������������ճ�ʱ�����ڹ���жϵķ�բʱ��
						if (1==tmpCpLink->lcOnOff)
						{
							startJudgeTime        = sysTime;
							startJudgeTime.hour   = tmpCpLink->duty[1];    //�ճ�ʱ��-ʱ
							startJudgeTime.minute = tmpCpLink->duty[0];    //�ճ�ʱ��-��
							startJudgeTime.second = 0;
							startJudgeTime = nextTime(startJudgeTime, beforeOnOff[3], 0);
							endJudgeTime          = sysTime;
							endJudgeTime.hour     = tmpCpLink->duty[3];    //����ʱ��-ʱ
							endJudgeTime.minute   = tmpCpLink->duty[2];    //����ʱ��-��
							endJudgeTime.second   = 0;
							endJudgeTime = backTime(endJudgeTime, 0, 0, 0, beforeOnOff[0], 0);							
							
							//�������ʱ�����ճ�ʱ��֮���ҵ�ǰʱ��������ʱ��֮ǰ,�ճ�ʱ�������һ��
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
												 "%02d-%02d-%02d %02d:%02d:%02d,���,��·���Ƶ�%02x%02x%02x%02x%02x%02x�ָ���طֺ�բ��־Ϊ��բ\n",
													sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
													 tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
												);
								}
							}
						}
						else
						{
							//lcOnOff=0,�����������������ʱ�����ڹ���жϵĺ�բʱ��
							if (0==tmpCpLink->lcOnOff)
							{
								endJudgeTime        = sysTime;
								endJudgeTime.hour   = tmpCpLink->duty[3];    //����ʱ��-ʱ
								endJudgeTime.minute = tmpCpLink->duty[2];    //����ʱ��-��
								endJudgeTime.second = 0;
								endJudgeTime = backTime(endJudgeTime, 0, 0, 0, beforeOnOff[0], 0);
								
								//�������ʱ�����ճ�ʱ��֮���ҵ�ǰʱ��������ʱ��֮ǰ,�ճ�ʱ�������һ��
								if (compareTwoTime(endJudgeTime, sysTime)==TRUE)
								{
									tmpCpLink->lcOnOff = 1;
									
									if (debugInfo&METER_DEBUG)
									{
										printf(
													 "%02d-%02d-%02d %02d:%02d:%02d,���,��·���Ƶ�%02x%02x%02x%02x%02x%02x�ָ���طֺ�բ��־Ϊ��բ\n",
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
               "%02d-%02d-%02d %02d:%02d:%02d,���,�������ӦΪ=%d\n",
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
    	    //����ֵ��Ҫ����,ͬʱ�鿴�Ƿ��е���·���Ƶ��Դ�Ѵ�
          if (
          	  (1==tmpCpLink->status)
          	   &&(tmpBright!=lCtrl.bright)
          	 )
          {
            checkStep = 1;
            
            if (debugInfo&METER_DEBUG)
            {
              printf(
                     "%02d-%02d-%02d %02d:%02d:%02d,���,�����Ѹı�\n",
                      sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second
                    );
    	      }
          
            break;
          }
          
    	    //�鿴�Ƿ��е���·���Ƶ�պ�բ
          if (1==tmpCpLink->gateOn)
          {
          	checkStep = 1;
          	tmpCpLink->gateOn = 0;
  
            if (debugInfo&METER_DEBUG)
            {
              printf(
                     "%02d-%02d-%02d %02d:%02d:%02d,���,��·���Ƶ�%02x%02x%02x%02x%02x%02x�պ�բ,�㲥����\n",
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
      	  
      	  //�ӳ�3����ù㲥�����־
      	  sleep(3); 
      	  
      	  carrierFlagSet.broadCast = 4;    //��� - ����㲥���ص�
      	}
      }
    }
  }
}

#endif 

/**************************************************
��������:meter485Receive
��������:485����մ����߳�
���ú���:
�����ú���:
�������:
�������:
����ֵ��״̬
***************************************************/
void meter485Receive(INT8U port, INT8U *buf, INT16U len)
{
 #ifdef LIGHTING 
  
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //һ��������ͳ���¼�����(��ʱ���޹���)

  INT16U                     tmpi;
  INT8U                      checkSum;
  DATE_TIME                  tmpTime;
  INT8U                      eventData[15];
  INT16U                     tmpTail;
  struct ctrlTimes           *tmpCTimes;
  INT8U                      j;
  INT8U                      timesCount = 0;
  INT8U                      numOfTime;
  INT8U                      checkStep=0;    //�Աȿ���ʱ�β���
  INT8U                      hourDataBuf[LEN_OF_LIGHTING_FREEZE];    //Сʱ�������ݻ���
  
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
          buf[tmpi] -= 0x33;    //���ֽڽ��ж��������0x33����
        }
      }
    	
      if (checkSum==buf[len-2])
      {
    		switch (buf[8])
    		{
    	  	case 0x88:
    				copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_CHECK_TIME | REQUEST_STATUS);    //���Уʱ��־,�����¶�ȡʵʱ���ݰ�
    				
						if (debugInfo&PRINT_CARRIER_DEBUG)
    				{
              printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)Уʱȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
            }
    				break;
    				
    	  	case 0x91:
    				if (buf[13]==0x00 && buf[12]==0x90 && buf[10]<0x08)
    				{
    		  		switch(buf[11])
    		  		{
    						case 0x00:    //00900000,���ƿ�����ʵʱ״̬��ʱ������
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
          					//00900000-1.��ȡSLCʱ��
          					tmpTime.second = buf[22];
          					tmpTime.minute = buf[23];
          					tmpTime.hour   = buf[24];
          					tmpTime.day    = buf[25];
          					tmpTime.month  = buf[26];
          					tmpTime.year   = buf[27];
          					tmpTime = timeBcdToHex(tmpTime);
      
          					if (copyCtrl[4].tmpCpLink!=NULL) 
          					{
          			  		//״̬�Ѹ���,2015-10-26,Add
          			  		if (buf[21]!=copyCtrl[4].tmpCpLink->status)
          			  		{
          							copyCtrl[4].tmpCpLink->statusUpdated |= 1;
          			  		}
          					  
          			  		copyCtrl[4].tmpCpLink->status = buf[21];         //SLC��ǰ״̬
          			  		copyCtrl[4].tmpCpLink->statusTime = tmpTime;     //SLC��ǰʱ�� 
          			  		copyCtrl[4].tmpCpLink->lddtStatusTime = sysTime; //�ɼ���LDDT״̬ʱ��������ʱ��,2016-02-03
          					  
                      
                      //����LDDT��Ҫ��λ�ı�־
                      if (copyCtrl[4].tmpCpLink->lddtRetry>0 && copyCtrl[4].tmpCpLink->lddtRetry<pnGate.lddtRetry)
                      {
                      	copyCtrl[4].copyContinue = TRUE;
                      }
                      copyCtrl[4].tmpCpLink->lddtRetry = 99;

                      //�������ߵ��ƿ�������Ҫ��λ�ı�־
                      if (copyCtrl[4].tmpCpLink->offLineRetry>0 && copyCtrl[4].tmpCpLink->offLineRetry<pnGate.offLineRetry)
                      {
                      	copyCtrl[4].copyContinue = TRUE;
                      }
                      copyCtrl[4].tmpCpLink->offLineRetry = 99;
                      
                      //�������ƿ��Ƶ��ж�Ϊ����,��¼�ָ��¼�
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[4].tmpCpLink->mp, 3); //���ƿ��Ƶ���ʱ���޹���
                      	
                      //�Ѿ��������Ҽ�¼������,��¼�ָ��¼�
                  	  if (meterStatisExtranTimeS.mixed&0x08)
                  	  {
                    		//��¼���Ƶ������Ϣ(ERC13)
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
                    		eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff) | 0x00;    //�ָ�
                    		eventData[10] = 0x40;    //���ƿ��Ƶ�����
                    	  
                    		writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                    	  
                    		if (eventRecordConfig.nEvent[1]&0x10)
                    		{
                    	  	writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                    		}
                    	  
                    		eventStatus[1] = eventStatus[1] | 0x10;
            					  
            						if (debugInfo&METER_DEBUG)
            						{
            			  			printf("%02d-%02d-%02d %02d:%02d:%02d,��¼���ƿ��Ƶ�%d(%02x%02x%02x%02x%02x%02x)���߻ָ��¼�\n", 
            				  	 					 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
            					 							copyCtrl[4].tmpCpLink->mp,
            					 							 copyCtrl[4].tmpCpLink->addr[5],copyCtrl[4].tmpCpLink->addr[4],copyCtrl[4].tmpCpLink->addr[3],copyCtrl[4].tmpCpLink->addr[2],copyCtrl[4].tmpCpLink->addr[1],copyCtrl[4].tmpCpLink->addr[0]
            										);
            						}
                        
                        activeReport3();    //�����ϱ��¼�
                        
                  			meterStatisExtranTimeS.mixed &= 0xf7;
                        
                        //�洢���Ƶ�ͳ������(��ʱ���޹���)
                        saveMeterData(copyCtrl[4].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                      }
                      
                      //��ѯ��·���ϵ�ĵ��ƿ�����״̬ʱ��λ,2015-11-12
                      if (copyCtrl[4].tmpCpLink->lineOn>0 && copyCtrl[4].tmpCpLink->lineOn<5)
                      {
                      	copyCtrl[4].tmpCpLink->lineOn = 5;
                      	copyCtrl[4].copyContinue = TRUE;
                      }
                    }
      
          					if (debugInfo&PRINT_CARRIER_DEBUG)
          					{
                      printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)ʵʱ״̬ʱ������:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                      printf("  -SLC��ǰ������:%d%%\n", buf[21]);
                      printf("  -SLC��ǰʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
                    }
          					
                    //1-1.�ж�SLC�Ƿ�ʱ�䳬��
                    if (!((timeCompare(tmpTime, sysTime, pnGate.checkTimeGate) == TRUE) 
             	        || (timeCompare(sysTime, tmpTime, pnGate.checkTimeGate) == TRUE)))
                    {
          			  		if (debugInfo&PRINT_CARRIER_DEBUG)
          			  		{
          							printf("  --SLCʱ�䳬��\n");
          			  		}
          						
          			  		if (copyCtrl[4].tmpCpLink!=NULL)
          			  		{
          							//����Уʱ
          							copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CHECK_TIME;
          			  		}
          					}
          					
          					//00900000-2.��ȡ��վ���ƽ�ֹʱ��
          					tmpTime.second = buf[15];
          					tmpTime.minute = buf[16];
          					tmpTime.hour   = buf[17];
          					tmpTime.day    = buf[18];
          					tmpTime.month  = buf[19];
          					tmpTime.year   = buf[20];
          					tmpTime = timeBcdToHex(tmpTime);
      
          					if (copyCtrl[4].tmpCpLink!=NULL)
          					{
          			  		copyCtrl[4].tmpCpLink->msCtrlCmd = buf[14];     //��վֱ�ӿ�������
                      copyCtrl[4].tmpCpLink->msCtrlTime = tmpTime;    //��վ���ƽ�ֹʱ��
                    }
      
          					if (debugInfo&PRINT_CARRIER_DEBUG)
          					{
                      if (buf[14]>100)
                      {
                        printf("  -����վ��������");
                      }
                      else
                      {
                        printf("  -��վ��������,����=%d%%", buf[14]);
                      }
                      printf(",��ֹʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
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
                    
                    //�����������Ƶ��ж�Ϊ��·�쳣,��¼�ָ��¼�
                    pFoundCpLink = ldgmLink;
                    while(pFoundCpLink!=NULL)
                    {
                      if(
                      	 (compareTwoAddr(pFoundCpLink->lddt1st, copyCtrl[4].tmpCpLink->addr, 0)==TRUE)
                      	  || (compareTwoAddr(pFoundCpLink->collectorAddr, copyCtrl[4].tmpCpLink->addr, 0)==TRUE)
                      	)
                      {
                        searchMpStatis(sysTime, &meterStatisExtranTimeS, pFoundCpLink->mp, 3); //������������ʱ���޹���
                        	
                        //�Ѿ��������Ҽ�¼���쳣,��¼�ָ��¼�
                    		if (
                    				(meterStatisExtranTimeS.mixed&0x02)       //�޵緢����
                    		 		 || (meterStatisExtranTimeS.mixed&0x04)    //�е緢����
                    	   	 )
                    		{
                      	  //��¼���Ƶ������Ϣ(ERC13)
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
                      	  eventData[9] = (pFoundCpLink->mp>>8&0xff) | 0x00;    //�ָ�
                      	  eventData[10] = 0x40;    //�������Ƶ��쳣,�е�״̬�ָ�
                      	  
                      	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                      	  
                      	  if (eventRecordConfig.nEvent[1]&0x10)
                      	  {
                      	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                      	  }
                      	  
                      	  eventStatus[1] = eventStatus[1] | 0x10;
              					  
              			  		if (debugInfo&METER_DEBUG)
              			  		{
              							printf("%02d-%02d-%02d %02d:%02d:%02d,��¼�������Ƶ�%d(%02x%02x%02x%02x%02x%02x)ĩ�˵��ƿ�����(%02x%02x%02x%02x%02x%02x)�쳣�ָ��¼�\n", 
              					  	 			 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
              					   					pFoundCpLink->mp,
              					   					 pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
              					   						buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]
              					  				);
              			  		}
                          
                          activeReport3();    //�����ϱ��¼�
                          
                    	  	if (meterStatisExtranTimeS.mixed&0x02)
                    	  	{
                    				meterStatisExtranTimeS.mixed &= 0xfd;
                    	  	}
                    	  	if (meterStatisExtranTimeS.mixed&0x04)
                    	  	{
                    				meterStatisExtranTimeS.mixed &= 0xfb;
                    	  	}
                          
                          //�洢���Ƶ�ͳ������(��ʱ���޹���)
                          saveMeterData(pFoundCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                        }
                      }
                      
                      pFoundCpLink = pFoundCpLink->next;
                    }

        						copyCtrl[4].tmpCpLink->copySuccess = TRUE; 
                  }
    			  			break;
									
    						case 0x02:    //���ƿ���������汾
				  				copyCtrl[4].tmpCpLink->softVer = buf[17]-0x30;
          		  	if (debugInfo&PRINT_CARRIER_DEBUG)
          		 	 	{
                    printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)����汾:%d\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1],copyCtrl[4].tmpCpLink->softVer);
                  }
				  				copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SOFT_VER;
				  				break;
    				    
    						case 0x03:    //���ƿ��������ص�ʱ������
          		  	if (debugInfo&PRINT_CARRIER_DEBUG)
          		  	{
                    printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)���ص�����:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                  }

        		  		tmpTail = 14;
        					
        		  		//00900300-1,�ж�SLC���ֵƹ��Ϻ����Դ����Ƿ�����վ����ֵһ��
        		  		if (buf[tmpTail++]!=pnGate.failureRetry)
        		  		{
        						if (copyCtrl[4].tmpCpLink!=NULL)
        						{
        			  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
        						}
        					  	
        						if (debugInfo&PRINT_CARRIER_DEBUG)
        						{
        			  			printf("  --���ƿ��������ֵƹ��Ϻ�����Դ������뼯������һ��\n");
        						}
        		  		}
        					
        		  		//00900300-2,PWMƵ�ʼ�������ռ�ձȶԱ�ֵ
        		  		if (copyCtrl[4].tmpCpLink==NULL)
        		  		{
        						tmpTail += 14;
        		  		}
        		  		else
        		  		{
        						//00900300-2.1,PWMƵ��
        						if (buf[tmpTail++]!=copyCtrl[4].tmpCpLink->bigAndLittleType)
        						{
        			  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
        			  		  
        			  			if (debugInfo&PRINT_CARRIER_DEBUG)
        			  			{
        								printf("  --���ƿ�����PWMƵ�����뼯������һ��\n");
        			  			}
        						}
        					  
        						//00900300-2.1,��������
        						if (1==copyCtrl[4].tmpCpLink->funOfMeasure)
        						{
        			  			if (buf[tmpTail++]!=copyCtrl[4].tmpCpLink->startCurrent)
        			  			{
        								copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
        						  
        								if (debugInfo&PRINT_CARRIER_DEBUG)
        								{
        				  				printf("  --���ƿ����������������뼯������һ��\n");
        								}
        			  			}
        						}
        						else
        						{
        			  			tmpTail++;
        						}
        					  
		        				//00900300-2.2������ռ�ձȶԱ�ֵ
		        				for(tmpi=0; tmpi<6; tmpi++)
		        				{
		        			  	if (buf[tmpTail+tmpi]!=copyCtrl[4].tmpCpLink->collectorAddr[tmpi])
		        			  	{
		        						copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_ADJ_PARA;
		        						  
		        						if (debugInfo&PRINT_CARRIER_DEBUG)
		        						{
		        				  		printf("  --���ƿ���������ֵ���뼯������һ��\n");
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
		        				  		printf("  --���ƿ�����ռ�ձ����뼯������һ��\n");
		        						}
		        					  		
		        						break;
		        			  	}
		        				}
		        				tmpTail+=6;
		        		  }
        				  
		        		  //00900300-3,SLC��⵽����һ�εƹ��Ϸ���ʱ��
		        		  tmpTime.year = buf[tmpTail+5];
		        		  tmpTime.month = buf[tmpTail+4];
		        		  tmpTime.day = buf[tmpTail+3];
		        		  tmpTime.hour = buf[tmpTail+2];
		        		  tmpTime.minute = buf[tmpTail+1];
		        		  tmpTime.second = buf[tmpTail];
		        		  tmpTail+=6;
		        		  if (debugInfo&PRINT_CARRIER_DEBUG)
		        		  {
		        				printf("  -SLC��һ�εƹ��Ϸ���ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
		        		  }
                  
				  				//��������ʱ���޹���
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
                    	
                      //�洢���Ƶ�ͳ������(��ʱ���޹���)
                      saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                      
                      //��¼������������Ϣ(ERC13)
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
          							printf("  --��¼���Ƶ�%d�����¼�\n", copyCtrl[4].tmpCpLink->mp);
          			  		}
                      
                      activeReport3();                  //�����ϱ��¼�
                    }
                  }
                  
                  //00900300-4,���һ�ο��ơ����⼰�ص�ʱ��
								  //00900300-4.1���һ�ο���ʱ��
								  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
									printf("  -SLC��һ�ο���ʱ��:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 1, tmpTime, (INT8U *)&tmpTime, 6);

								  //00900300-4.2���һ�ι��Сʱ��
								  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
										printf("  -SLC��һ�ι��Сʱ��:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 2, tmpTime, (INT8U *)&tmpTime, 6);
                  
								  //00900300-4.3���һ�ι������ʱ��
							 	  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
								 		printf("  -SLC��һ�ι������ʱ��:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 3, tmpTime, (INT8U *)&tmpTime, 6);

								  //00900300-4.4���һ�ιص�����ʱ��
								  tmpTime.year = buf[tmpTail+4];
								  tmpTime.month = buf[tmpTail+3];
								  tmpTime.day = buf[tmpTail+2];
								  tmpTime.hour = buf[tmpTail+1];
								  tmpTime.minute = buf[tmpTail+0];
								  tmpTime.second = 0x00;
								  tmpTail+=5;
								  if (debugInfo&PRINT_CARRIER_DEBUG)
								  {
										printf("  -SLC��һ�ιص�ʱ��:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
								  }
                  saveSlDayData(copyCtrl[4].tmpCpLink->mp, 4, tmpTime, (INT8U *)&tmpTime, 6);

								  if (buf[9]>=0x36)
								  {
										//00900300-5,SLC��⵽����һ����б����ʱ��
										tmpTime.year = buf[tmpTail+5];
										tmpTime.month = buf[tmpTail+4];
										tmpTime.day = buf[tmpTail+3];
										tmpTime.hour = buf[tmpTail+2];
										tmpTime.minute = buf[tmpTail+1];
										tmpTime.second = buf[tmpTail];
										tmpTail+=6;
										if (debugInfo&PRINT_CARRIER_DEBUG)
										{
										  printf("  -SLC��һ����б����ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
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
												
												//�洢���Ƶ�ͳ������(��ʱ���޹���)
												saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
												
												//��¼������������Ϣ(ERC13)
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
													printf("  --��¼���Ƶ�%d��б�¼�\n", copyCtrl[4].tmpCpLink->mp);
												}
												
												activeReport3();                  //�����ϱ��¼�
										  }
										}
										
										//©����
										tmpData = buf[tmpTail] | buf[tmpTail+1]<<8 | buf[tmpTail+2]<<16;
										if (debugInfo&PRINT_CARRIER_DEBUG)
										{
											printf("  -SLC��ǰ©����:%dmA\n", tmpData);
										}

										//2019-01-22,���,�����õ���Խ����Ҫ��¼�ż�¼©�����¼�
										if (eventRecordConfig.iEvent[3]&0x01)
										{
											if (tmpData>pnGate.leakCurrent)
											{
											  if ((meterStatisExtranTimeS.mixed&0x10)==0x00)
											  {
													meterStatisExtranTimeS.mixed |= 0x10;
													
													//�洢���Ƶ�ͳ������(��ʱ���޹���)
													saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
													
													//��¼������������Ϣ(ERC13)
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
													eventData[10] = 0x10;    //©��������ֵ
													
													writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
													
													if (eventRecordConfig.nEvent[1]&0x10)
													{
														writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
													}
													
													eventStatus[1] = eventStatus[1] | 0x10;
													
													if (debugInfo&PRINT_CARRIER_DEBUG)
													{
														printf("  --��¼���Ƶ�%d©��������ֵ�¼�����\n", copyCtrl[4].tmpCpLink->mp);
													}
													
													activeReport3();                  //�����ϱ��¼�
											  }
											}
											else
											{
											  if ((meterStatisExtranTimeS.mixed&0x10)==0x10)
											  {
													meterStatisExtranTimeS.mixed &= 0xef;
													
													//�洢���Ƶ�ͳ������(��ʱ���޹���)
													saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
													
													//��¼������������Ϣ(ERC13)
													eventData[0] = 13;
													eventData[1] = 11;
												
													eventData[2] = tmpTime.second;
													eventData[3] = tmpTime.minute;
													eventData[4] = tmpTime.hour;
													eventData[5] = tmpTime.day;
													eventData[6] = tmpTime.month;
													eventData[7] = tmpTime.year;
													
													eventData[8] = copyCtrl[4].tmpCpLink->mp&0xff;
													eventData[9] = (copyCtrl[4].tmpCpLink->mp>>8&0xff);    //�ָ�
													eventData[10] = 0x10;    //©��������ֵ
													
													writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
													
													if (eventRecordConfig.nEvent[1]&0x10)
													{
														writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
													}
													
													eventStatus[1] = eventStatus[1] | 0x10;
													
													if (debugInfo&PRINT_CARRIER_DEBUG)
													{
														printf("  --��¼���Ƶ�%d©��������ֵ�¼��ָ�\n", copyCtrl[4].tmpCpLink->mp);
													}
													
													activeReport3();                  //�����ϱ��¼�
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
												//���ʱ����ͬ����־
				        				meterStatisExtranTimeS.mixed &= 0xdf;
												saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
					  					}
        						}
        						else
        						{
        			  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_FREEZE_TIMES;
												
										  //�洢���Ƶ�ͳ������(��ʱ���޹���)
										  meterStatisExtranTimeS.mixed |= 0x20;    //��ǿ���ʱ�εȲ�����ͬ�������ƿ��ƿ�����
										  saveMeterData(copyCtrl[4].tmpCpLink->mp, 31, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
					        	}
        		  		}
    			  			break;

    						case 0x04:    //���ƿ���������ʱ��С������
        		  		//68 05 00 00 03 14 20 68 91 2b 33 34 c3 33 35 c8 34 51 39 3c 63 97 43 35 33 44 38 97 45 43 33 47 53 97 49 68 33 fd 34 52 37 3b 73 97 44 35 33 45 43 97 46 43 33 2a 16 
          		  	if (debugInfo&PRINT_CARRIER_DEBUG)
          		  	{
                    printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)����ʱ������С��%d.\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1],buf[10]);
                  }

        		  		//00900400-1,��2С��ʱ�ж�SLCʱ���뼯�����Ƿ�һ��
  				  			tmpTail = 15;
  				  			timesCount = 0;
    			  			tmpCTimes = cTimesHead;
    			  			while(tmpCTimes)
    			  			{
    								if (1==tmpCTimes->deviceType    //���ƿ�����
    				    				&& tmpCTimes->noOfTime==(copyCtrl[4].tmpCpLink->ctrlTime&0x0f)
    				   				 )
    								{
    				  				timesCount++;
    								}
    								tmpCTimes = tmpCTimes->next;
    			  			}
							
				  				if (debugInfo&PRINT_CARRIER_DEBUG)
				  				{
										printf("  --(С��)�������Ͽ���ʱ�����=%d,���������ϴ���ʱ�������=%d��\n",timesCount, buf[14]);
				  				}

				  				if (buf[14]<buf[10]*3)
				  				{
    								if (timesCount!=buf[14])
    								{
                  	  if (copyCtrl[4].tmpCpLink!=NULL)
                  	  {
                  			copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
												
												//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
                  			printf("  --(С��)���ƿ���������ʱ��������뼯�������ȡ�\n");
                  	  }
            				}
            	  	}

				  				//buf[14]�����͵ĸõ��ƿ������ϱ������Ч��ʱ��θ���
				  				checkStep = 0;
				  				for(tmpi=(buf[10]-1)*3; tmpi<buf[14]; tmpi++)
				  				{
										if (checkStep<2)
										{
										  checkStep = 0;
										  tmpCTimes = cTimesHead;
										  while(tmpCTimes)
										  {
												if (1==tmpCTimes->deviceType    //���ƿ�����
												    && tmpCTimes->noOfTime==(copyCtrl[4].tmpCpLink->ctrlTime&0x0f)
												     && tmpCTimes->startMonth==(buf[tmpTail]&0x0f) && tmpCTimes->startDay==buf[tmpTail+1]
												      && tmpCTimes->endMonth==(buf[tmpTail]>>4&0x0f) && tmpCTimes->endDay==buf[tmpTail+2]
												   )
												{
												  checkStep = 1;
											  	
												  //2016-11-14,����汾Ϊ2.0���ϵĵ��ƿ���������������ж�
												  if (copyCtrl[4].tmpCpLink->softVer>=2)
												  {
												  	//2016-8-19,����������ж�
												  	if (tmpCTimes->workDay!=buf[tmpTail+3])
												  	{
												  	  checkStep = 4;
												  		
						        					copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;

														  //2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
    						    						printf("  --���ƿ��Ƶ����ʱ���%d�������뼯������һ�¡�\n", tmpi+1);
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
																																
																		//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
																		printf("  --(С��)���ƿ���������ʱ���%dʱ��%d�뼯������һ�¡�\n", tmpi+1, j+1);
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
																  printf("  --(С��)���ƿ���������ʱ���%dʱ��%d�뼯������һ�¡�\n", tmpi+1, j+1);
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
																			
																	//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
							    						    printf("  --(С��)���ƿ���������ʱ���%dʱ�������ڼ������б���ʱ������\n", tmpi+1);
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
																
												  	//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
													  printf("  --(С��)���ƿ���������ʱ���%d�����ڼ���������ʱ������δ�ҵ���\n", tmpi+1);
													}
													checkStep = 4;
					  						}
				    					}
							
											//2016-11-14,����汾Ϊ2.0���ϵĵ��ƿ���������������ж�
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
				      				  printf("  --(С��)���ƿ���������Ŀ���ģʽ=%d", buf[tmpTail]);
				      				}
				      				if (buf[tmpTail]!=ctrlMode)
				      				{
				      				  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
										
						 					  //2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
				      						printf(",�뼯��������ģʽ��һ�¡�");
				      				  }
				      				}
				      				tmpTail++;
				      				if (debugInfo&PRINT_CARRIER_DEBUG)
				      				{
				      				  printf("\n");
				      				}
				      				  
				  				    if (debugInfo&PRINT_CARRIER_DEBUG)
				  				    {
				  					  	printf("  --(С��)���ƿ���������Ĺ�ؿ���ǰ%d������Ч", buf[tmpTail]);
				  				    }
				  				    if (buf[tmpTail]!=beforeOnOff[0])
				  				    {
				  				      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;

												//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
					  					  	printf(",�뼯������һ�¡�");
					  				    }
					  				  }
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("  --(С��)���ƿ���������Ĺ�ؿ��ƺ�%d������Ч", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=beforeOnOff[1])
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;

												//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
					  					  	printf(",�뼯������һ�¡�");
					  				    }
  				  					}
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("  --(С��)���ƿ���������Ĺ�عص�ǰ%d������Ч", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=beforeOnOff[2])
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
									
												//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
					  					  	printf(",�뼯������һ�¡�");
					  				    }
  				  					}
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("  --(С��)���ƿ���������Ĺ�عصƺ�%d������Ч", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=beforeOnOff[3])
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
									
												//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
					  					  	printf(",�뼯������һ�¡�");
					  				    }
					  				  }
					  				  tmpTail++;
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  						printf("\n");
					  				  }
  				  
  				  
					  				  if (debugInfo&PRINT_CARRIER_DEBUG)
					  				  {
					  					  printf("  --(С��)���ƿ�������������=%d", buf[tmpTail]);
					  				  }
					  				  if (buf[tmpTail]!=(copyCtrl[4].tmpCpLink->ctrlTime>>4))
					  				  {
					  				    copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
									
												//2016-11-14,���ƿ�����2.0���ϵİ汾,��֧��20��ʱ���
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
					  					  	printf(",�뼯������һ�¡�");
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
														printf("  --�յ����ƿ���������ʱ��С��1�뼯����һ��\n");
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
												  		printf("  --��ʱ�����=%d\n",buf[14]);
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
													 	 	printf("  --��ʱ�����=%d\n",buf[14]);
														}
												  }
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
														printf("  --�յ����ƿ���������ʱ��С��2�뼯����һ��\n");
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
											      	printf("  --��ʱ�����=%d\n",buf[14]);
											    	}
											  	}
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --�յ����ƿ���������ʱ��С��3�뼯����һ��\n");
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
											    	  printf("  --��ʱ�����=%d\n",buf[14]);
											    	}
											  	}
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --�յ����ƿ���������ʱ��С��4�뼯����һ��\n");
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
											      	printf("  --��ʱ�����=%d\n",buf[14]);
											    	}
											  	}
										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --�յ����ƿ���������ʱ��С��5�뼯����һ��\n");
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
											      	printf("  --��ʱ�����=%d\n",buf[14]);
											    	}
											  	}

										      if (debugInfo&PRINT_CARRIER_DEBUG)
											   	{
											    	printf("  --�յ����ƿ���������ʱ��С��6�뼯����һ��\n");
											  	}
										    }
								
										    if (0x7==buf[10])
										    {
										      copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_CTRL_TIME_7;

										      if (debugInfo&PRINT_CARRIER_DEBUG)
											  	{
											    	printf("  --�յ����ƿ���������ʱ��С��7�뼯����һ��\n");
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
				          printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)Сʱ��������\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
				        }

							  copyCtrl[4].tmpCpLink->flgOfAutoCopy |= REQUEST_HOUR_FREEZE;
							}
							break;
    			
 		  			case 0x94:    //ȷ��
							if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)
							{
							  //���������������ʱ�ΰ�1��־,�����¶�ȡ����ʱ������
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_1 | REQUEST_CTRL_TIME_1 | REQUEST_CTRL_TIME_2| REQUEST_CTRL_TIME_3| REQUEST_CTRL_TIME_4| REQUEST_CTRL_TIME_5| REQUEST_CTRL_TIME_6| REQUEST_CTRL_TIME_7);

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
				          printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)������������ʱ�ΰ�1ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
				        }
            	}
							else if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)
							{
							  //���������������ʱ�ΰ�2��־,�����¶�ȡ����ʱ������
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_2 | REQUEST_CTRL_TIME_1 | REQUEST_CTRL_TIME_2| REQUEST_CTRL_TIME_3| REQUEST_CTRL_TIME_4| REQUEST_CTRL_TIME_5| REQUEST_CTRL_TIME_6| REQUEST_CTRL_TIME_7);

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)������������ʱ�ΰ�2ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}
            	}
							else if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)
							{
							  //���������������ʱ�ΰ�3��־,�����¶�ȡ����ʱ������
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_3 | REQUEST_CTRL_TIME_1 | REQUEST_CTRL_TIME_2| REQUEST_CTRL_TIME_3| REQUEST_CTRL_TIME_4| REQUEST_CTRL_TIME_5| REQUEST_CTRL_TIME_6| REQUEST_CTRL_TIME_7);

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)������������ʱ�ΰ�3ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}
            	}
						
							if (copyCtrl[4].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_ADJ_PARA)
							{
							  //��������������������־,�����¶�ȡʵʱ���ݰ�
							  copyCtrl[4].tmpCpLink->flgOfAutoCopy &= ~REQUEST_SYN_ADJ_PARA;

							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)���������������ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
              	}
            	}
							break;
    			
		  			case 0xd1:    //�쳣Ӧ��
							if (0x01==buf[9] && 0x12==buf[10])
							{
							  if (debugInfo&PRINT_CARRIER_DEBUG)
							  {
                	printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�SLC(%02x%02x%02x%02x%02x%02x)��Сʱ��������Ӧ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
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
                  	printf("%02d-%02d-%02d %02d:%02d:%02d,SLC(%02x%02x%02x%02x%02x%02x)���ܲ�֧�ֿ���ʱ��С������\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
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

    //RS485�ӿ��Ͻӵ���·�������Ľ���
    if ((port==1 || port==2)  && AC_SAMPLE==copyCtrl[port].cpLinkHead->protocol)
    {
      if (
      	  buf[0]==0x68 
      	   && buf[7]==0x68 
      	    && buf[len-1]==0x16
      	     && compareTwoAddr(copyCtrl[port].cpLinkHead->addr, &buf[1], 0)==TRUE    //2016-03-21,��ӵ�ַ�Ƚ�
      	 )
      {
      	checkSum = 0;
      	for(tmpi=0; tmpi<len-2; tmpi++)
      	{
      	  checkSum += buf[tmpi];
          
          if (tmpi>9)
          {
            buf[tmpi] -= 0x33;  //���ֽڽ��ж��������0x33����
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
								  	  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ���·������(%02x%02x%02x%02x%02x%02x)��������\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
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
		  	        
		  	        		//���й�����
		  	        		acParaData[POWER_INSTANT_WORK+0] = buf[tmpTail++];
		  	        		acParaData[POWER_INSTANT_WORK+1] = buf[tmpTail++];
		  	        		acParaData[POWER_INSTANT_WORK+2] = buf[tmpTail++];

				  	        //A���й�����
				  	        acParaData[POWER_PHASE_A_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_WORK+2] = buf[tmpTail++];

				  	        //B���й�����
				  	        acParaData[POWER_PHASE_B_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_WORK+2] = buf[tmpTail++];

				  	        //C���й�����
				  	        acParaData[POWER_PHASE_C_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_WORK+2] = buf[tmpTail++];
				  	        
				  	        //���޹�����
				  	        acParaData[POWER_INSTANT_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_INSTANT_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_INSTANT_NO_WORK+2] = buf[tmpTail++];

				  	        //A���޹�����
				  	        acParaData[POWER_PHASE_A_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_A_NO_WORK+2] = buf[tmpTail++];

				  	        //B���޹�����
				  	        acParaData[POWER_PHASE_B_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_B_NO_WORK+2] = buf[tmpTail++];

				  	        //C���޹�����
				  	        acParaData[POWER_PHASE_C_NO_WORK+0] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_NO_WORK+1] = buf[tmpTail++];
				  	        acParaData[POWER_PHASE_C_NO_WORK+2] = buf[tmpTail++];
				  	        
				  	        //�ܹ�������
				  	        acParaData[TOTAL_POWER_FACTOR+0] = buf[tmpTail++];
				  	        acParaData[TOTAL_POWER_FACTOR+1] = buf[tmpTail++];
				  	        
				  	        //A�๦������
				  	        acParaData[FACTOR_PHASE_A+0] = buf[tmpTail++];
				  	        acParaData[FACTOR_PHASE_A+1] = buf[tmpTail++];

				  	        //B�๦������
				  	        acParaData[FACTOR_PHASE_B+0] = buf[tmpTail++];
				  	        acParaData[FACTOR_PHASE_B+1] = buf[tmpTail++];

				  	        //C�๦������
				  	        acParaData[FACTOR_PHASE_C+0] = buf[tmpTail++];
				  	        acParaData[FACTOR_PHASE_C+1] = buf[tmpTail++];
				  	        
				  	        //A���ѹ
				  	        acParaData[VOLTAGE_PHASE_A+0] = buf[tmpTail++];
				  	        acParaData[VOLTAGE_PHASE_A+1] = buf[tmpTail++];

				  	        //B���ѹ
				  	        acParaData[VOLTAGE_PHASE_B+0] = buf[tmpTail++];
				  	        acParaData[VOLTAGE_PHASE_B+1] = buf[tmpTail++];

				  	        //C���ѹ
				  	        acParaData[VOLTAGE_PHASE_C+0] = buf[tmpTail++];
				  	        acParaData[VOLTAGE_PHASE_C+1] = buf[tmpTail++];

				  	        //A�����
				  	        acParaData[CURRENT_PHASE_A+0] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_A+1] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_A+2] = buf[tmpTail++];

				  	        //B�����
				  	        acParaData[CURRENT_PHASE_B+0] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_B+1] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_B+2] = buf[tmpTail++];

				  	        //C�����
				  	        acParaData[CURRENT_PHASE_C+0] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_C+1] = buf[tmpTail++];
				  	        acParaData[CURRENT_PHASE_C+2] = buf[tmpTail++];
  				  	        
                    //ÿ15���Ӵ洢һ������
										//2016-09-02,�޸�Ϊÿ5���ӱ���һ������
									 #ifdef JF_MONITOR	
									  if (sysTime.second<=59)    //�������ÿ�ֶ���,2018-10-11,�ĳɲ�������,��Ϊ�ڻ�������г�������,1����֮�ڲ����ܳ���
									 #else
                    if (0==(sysTime.minute%5) && sysTime.second<=30)
									 #endif
                    {
                      //�洢
                      saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, tmpTime, \
                      							acParaData, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
                      
                      //ʾֵ
                      memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);
                      
                      //�����й�����ʾֵ
                      visionBuff[POSITIVE_WORK_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_OFFSET+3] = buf[tmpTail++];

                      //A�������й�����ʾֵ
                      visionBuff[POSITIVE_WORK_A_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_A_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_A_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_A_OFFSET+3] = buf[tmpTail++];

                      //B�������й�����ʾֵ
                      visionBuff[POSITIVE_WORK_B_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_B_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_B_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_B_OFFSET+3] = buf[tmpTail++];

                      //C�������й�����ʾֵ
                      visionBuff[POSITIVE_WORK_C_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_C_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_C_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_WORK_C_OFFSET+3] = buf[tmpTail++];

                      //�����޹�������ʾֵ
                      visionBuff[POSITIVE_NO_WORK_OFFSET+0] = buf[tmpTail++];
                      visionBuff[POSITIVE_NO_WORK_OFFSET+1] = buf[tmpTail++];
                      visionBuff[POSITIVE_NO_WORK_OFFSET+2] = buf[tmpTail++];
                      visionBuff[POSITIVE_NO_WORK_OFFSET+3] = buf[tmpTail++];

                      //A�������޹�����ʾֵ
                      visionBuff[COMB1_NO_WORK_A_OFFSET+0] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_A_OFFSET+1] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_A_OFFSET+2] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_A_OFFSET+3] = buf[tmpTail++];

                      //B�������޹�����ʾֵ
                      visionBuff[COMB1_NO_WORK_B_OFFSET+0] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_B_OFFSET+1] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_B_OFFSET+2] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_B_OFFSET+3] = buf[tmpTail++];

                      //C�������޹�����ʾֵ
                      visionBuff[COMB1_NO_WORK_C_OFFSET+0] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_C_OFFSET+1] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_C_OFFSET+2] = buf[tmpTail++];
                      visionBuff[COMB1_NO_WORK_C_OFFSET+3] = buf[tmpTail++];
                      
                      saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, tmpTime, \
                                    visionBuff, PRESENT_DATA, ENERGY_DATA, LENGTH_OF_ENERGY_RECORD);

							  	 	  if (debugInfo&METER_DEBUG)
							  	 	  {
							  	 	    printf("%02d-%02d-%02d %02d:%02d:%02d,������·������(%02x%02x%02x%02x%02x%02x)��������\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
							  	 	  }
		  	        		}
		  	        		if (pFoundCpLink!=NULL)
		  	        		{
							  	 	  //1.�жϹ��ʶ���
							  	 	  tmpP = acParaData[POWER_INSTANT_WORK+2]<<16 | acParaData[POWER_INSTANT_WORK+1]<<8 | acParaData[POWER_INSTANT_WORK+0];
							  	 	  tmpP = bcdToHex(tmpP);
							  	 	  tmpPrevP = pFoundCpLink->duty[2]<<16 | pFoundCpLink->duty[1]<<8 | pFoundCpLink->duty[0];
							  	 	  tmpPrevP = bcdToHex(tmpPrevP);
							  	 	  if (debugInfo&METER_DEBUG)
							  	 	  {
							  	 	    printf(
							  	 	           "%02d-%02d-%02d %02d:%02d:%02d,��һ����·�����ܹ���=%ld,������·�����ܹ���=%ld\n", 
							  	 	            sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
							  	 	             tmpPrevP, tmpP
							  	 	          );
							  	 	  }
							  	 	  //���ʽ�����30%
							  	 	  if (tmpP < (tmpPrevP*7/10))
							  	 	  {
							  	 	  	tmpNode = ldgmLink;
							  	 	  	while(tmpNode!=NULL)
							  	 	    {
							  	 	      if (
							  	 	    	  	(tmpNode->mpNo>>8)==pFoundCpLink->mp    //�������Ƶ�ĸ����жϲ������Ǳ�������
							  	 	    	  	 && (tmpNode->status&0x1)==0x1          //��·�ϵ�
							  	 	    	 	 )
							  	 	      {
						     	 	        //��һĩ�˵��ƿ�������ַ��Ϊȫ0
						     	 	        if (FALSE == compareTwoAddr(tmpNode->lddt1st, NULL, 1))
						     	 	        {
						     	 	        	tmpZbNode = copyCtrl[4].cpLinkHead;
					  		 	 	          while(tmpZbNode!=NULL)
					  		 	 	          {
					  		 	 	          	if (compareTwoAddr(tmpZbNode->addr, tmpNode->lddt1st, 0)==TRUE)
					  		 	 	          	{
				     	 	        	    		//״̬ʱ���˵�������ֵ֮ǰ
				     	 	        	    		//tmpZbNode->statusTime = backTime(sysTime, 0, 0, 0, tmpNode->funOfMeasure+1, 0);
				     	 	        	    		tmpZbNode->lddtStatusTime = backTime(sysTime, 0, 0, 0, tmpNode->funOfMeasure+1, 0);    //2016-02-03
				     	 	        	  		}
				     	 	        	  
				     	 	        	  		tmpZbNode = tmpZbNode->next;
				     	 	        			}
				     	 	        		}
     	 	        
						     	 	        //�ڶ�ĩ�˵��ƿ�������ַ��Ϊȫ0
						     	 	        if (FALSE == compareTwoAddr(tmpNode->collectorAddr, NULL, 1))
						     	 	        {
						     	 	          tmpZbNode = copyCtrl[4].cpLinkHead;
					  		 	 	          while(tmpZbNode!=NULL)
					  		 	 	          {
					  		 	 	          	if (compareTwoAddr(tmpZbNode->addr, tmpNode->collectorAddr, 0)==TRUE)
					  		 	 	          	{
						     	 	        	  //״̬ʱ���˵�������ֵ֮ǰ
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
					  	 	                     "%02d-%02d-%02d %02d:%02d:%02d,���ʼ����½�,���������������Ƶ��Ƿ����\n", 
					  	 	                      sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second
					  	 	                    );
					  	 	            }
			  	 	      				}
			  	 	      				tmpNode = tmpNode->next;
			  	 	    				}
			  	 	  				}
		  	      	
		  	      	  		pFoundCpLink->statusTime = sysTime;
		  	      	  		memcpy(pFoundCpLink->duty, &buf[14], 3);
		  	      	
		  	      	  		//2.��·������Խ���ж�
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
  
    //RS485�ӿ��Ͻӵ���·������
    if (port<4 && LIGHTING_XL==copyCtrl[port].cpLinkHead->protocol)
    {
      if (
      	  buf[0]==0x68 
      	   && buf[7]==0x68 
      	    && buf[len-1]==0x16
      	     && compareTwoAddr(copyCtrl[port].cpLinkHead->addr, &buf[1], 0)==TRUE    //2016-03-21,��ӵ�ַ�Ƚ�
      	 )
      {
      	checkSum = 0;
      	for(tmpi=0; tmpi<len-2; tmpi++)
      	{
      	  checkSum += buf[tmpi];
          
          if (tmpi>9)
          {
            buf[tmpi] -= 0x33;  //���ֽڽ��ж��������0x33����
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
			  			//���Уʱ��־�����¶�ȡʵʱ���ݰ�
			  			copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_CHECK_TIME | REQUEST_STATUS);

			  			if (debugInfo&METER_DEBUG)
			  			{
                printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)Уʱȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
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
										
				  				case 0x04:    //����ʱ��С������
										//68 51 02 00 04 16 20 68 91 47 34 37 c3 33 39 64 34 52 ae 35 4c 33 34 3a 33 33 97 34 51 b2 35 4c 43 34 39 83 33 ba 34 3d b2 35 3b 33 34 4a 73 33 bb 3e 47 b2 35 3b 68 34 4a 39 33 cb 48 51 b1 35 3b 68 34 4a 33 33 fd 34 52 b2 35 3b 63 34 4a 38 33 be 16 
										if (debugInfo&PRINT_CARRIER_DEBUG)
										{
										  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)����ʱ������С��%d.\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1],buf[10]);
										}
										
				  					timesCount = 0;
				    				tmpCTimes = cTimesHead;
				    				while(tmpCTimes)
				    				{
	           	    	  if (
	           	    		  	tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime 
	           	    		  	 && 
	           	    		     	(
	           	    		      	(2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //����ģʽΪʱ�οػ�ʱ�οؽ�Ϲ��
	           	    		       	|| (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //����ģʽΪ��γ�ȿػ�γ�ȿؽ�Ϲ��
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
												  printf("  --��·����������ʱ��������뼯�������ȡ�\n");
												}
					  					}
										}
  
  									//buf[14]�����͵ĸ���·�������ϱ������Ч��ʱ��θ���
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
	       	    		        			(2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //����ģʽΪʱ�οػ�ʱ�οؽ�Ϲ��
	       	    		         				|| (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //����ģʽΪ��γ�ȿػ�γ�ȿؽ�Ϲ��
	       	    		       			)
						  	    						&& tmpCTimes->startMonth==(buf[tmpTail]&0x0f) && tmpCTimes->startDay==buf[tmpTail+1]
						  	     							&& tmpCTimes->endMonth==(buf[tmpTail]>>4&0x0f) && tmpCTimes->endDay==buf[tmpTail+2]
						  	 						 )
						  						{
												  	checkStep = 1;
												  	
												  	//2016-8-19,����������ж�
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
						    						    printf("  --��·����������ʱ���%d�������뼯������һ�¡�\n", tmpi+1);
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
						        						    printf("  --��·����������ʱ���%dʱ��%d�뼯������һ�¡�\n", tmpi+1, j+1);
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
							      						  printf("  --��·����������ʱ���%dʱ�������ڼ������б���ʱ������\n", tmpi+1);
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
												    printf("  --��·����������ʱ���%d�����ڼ���������ʱ������δ�ҵ���\n", tmpi+1);
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
														printf("  --�յ���·���Ƶ����ʱ��С��1�뼯����һ��\n");
												  }
							
												  if (buf[14]<10)
												  {
														copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;
														if (debugInfo&METER_DEBUG)
														{
													  	printf("  --��ʱ�����=%d\n",buf[14]);
														}
												  }
												}
												if (0x2==buf[10])
												{
												  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CTRL_TIME_2;

												  if (debugInfo&METER_DEBUG)
												  {
														printf("  --�յ���·���Ƶ����ʱ��С��2�뼯����һ��\n");
												  }
												}
						
												if ( 
												    (0x1==buf[10] && buf[14]<10)
												     || 0x2==buf[10]
												   )
												{												
												  searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);    //��������ʱ���޹���
												  meterStatisExtranTimeS.mixed |= 0x20;    //��ǿ���ʱ�εȲ�����ͬ�������ƿ��ƿ�����
												  saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
												}
											}
										}
										break;
									
								  case 0xff:
				  					//1.��ȡXLCʱ��
				  					tmpTime.second = buf[21];
				  					tmpTime.minute = buf[22];
				  					tmpTime.hour   = buf[23];
				  					tmpTime.day    = buf[24];
				  					tmpTime.month  = buf[25];
				  					tmpTime.year   = buf[26];
				  					tmpTime = timeBcdToHex(tmpTime);
				      
				  					if (pFoundCpLink!=NULL)
				  					{
				  					  //״̬�Ѹ���,2015-10-26,Add
				  					  if (buf[27]!=pFoundCpLink->status)
				  					  {
				  					  	pFoundCpLink->statusUpdated |= 1;
				  					  	
				  					  	//��·��������բ
				  					  	if (buf[27]==0x00)
				  					  	{
				  					  	  pFoundCpLink->gateOn = 0;
				  					  		 
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				 	  		 	 	      while(tmpNode!=NULL)
				 	  		 	 	      {
				  					  			//2015-11-14,��·�������������ƿ�����ӦΪ��(����Ϊ0%)
				 	  		 	 	      	if (tmpNode->mpNo==pFoundCpLink->mp)
				 	  		 	 	      	{
				 	  		 	 	      	  tmpNode->status = 0;
				 	  		 	 	      	  tmpNode->statusUpdated |= 1;
				 	  		 	 	      	  tmpNode->lineOn = 0;    //�ز���·�ϵ�
				 	  		 	 	      	  
				 	  		 	 	      	  if (debugInfo&METER_DEBUG)
				 	  		 	 	      	  {
				 	  		 	 	      	  	printf(
				 	  		 	 	      	  	       "��·���Ƶ�(%02x%02x%02x%02x%02x%02x)�������ƿ��Ƶ�(%02x%02x%02x%02x%02x%02x)��Ϊ�ϵ�,����0%\n",
				 	  		 	 	      	  	       pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
				 	  		 	 	      	  	       tmpNode->addr[5],pFoundCpLink->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0]
				 	  		 	 	      	  	      );
				 	  		 	 	      	  }
				 	  		 	 	      	}
				 	  		 	 	      	
				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
				  					  	}
          					  	
				  					  	//��·��������բ,2015-11-09,Add
				  					  	if (buf[27]==0x01)
				  					  	{
				  					  	  pFoundCpLink->gateOn = 1;
				  					  		
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				 	  		 	 	      while(tmpNode!=NULL)
				 	  		 	 	      {
				  					  			//2015-11-14,֪ͨ��·���Ƶ��������ƿ��Ƶ����ϵ�
				 	  		 	 	      	if (tmpNode->mpNo==pFoundCpLink->mp)
				 	  		 	 	      	{
				 	  		 	 	      	  tmpNode->lineOn = 1;    //�ز���·�ϵ�
				 	  		 	 	      	
				 	  		 	 	      	  tmpNode->statusTime = sysTime;        //����״̬ʱ���Ϊ��ǰʱ��
				 	  		 	 	      	  tmpNode->lddtStatusTime = sysTime;    //����״̬ʱ���Ϊ��ǰʱ��,2016-02-03,Add
				 	  		 	 	      	  
				 	  		 	 	      	  if (debugInfo&METER_DEBUG)
				 	  		 	 	      	  {
				 	  		 	 	      	  	printf(
				 	  		 	 	      	  	       "֪ͨ��·���Ƶ�(%02x%02x%02x%02x%02x%02x)�������ƿ��Ƶ�(%02x%02x%02x%02x%02x%02x)�ϵ�\n",
				 	  		 	 	      	  	       pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
				 	  		 	 	      	  	       tmpNode->addr[5],pFoundCpLink->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0]
				 	  		 	 	      	  	      );
				 	  		 	 	      	  }
				 	  		 	 	      	}
					  		 	 	      	
				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
				  					  	}
				  					  }
          					  
          			  		pFoundCpLink->statusTime = tmpTime;    //XLC��ǰʱ��
                      pFoundCpLink->status = buf[27];        //XLC��ǰ״̬
                    }
      
  									if (debugInfo&METER_DEBUG)
  									{
                      printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)ʵʱ���ݰ�:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                      printf("  -XLC��ǰբ״̬:%d\n", buf[27]);
                      printf("  -XLC��ǰYX״̬:%d\n", buf[28]);
                      printf("  -XLC��ǰʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
                    }
                    
          					
                    //1-1.�ж�XLC�Ƿ�ʱ�䳬��
                    if (!((timeCompare(tmpTime, sysTime, pnGate.checkTimeGate) == TRUE) 
             	        	|| (timeCompare(sysTime, tmpTime, pnGate.checkTimeGate) == TRUE)))
                    {
				  					  if (debugInfo&METER_DEBUG)
				  					  {
				  							printf("  --XLCʱ�䳬��\n");
				  					  }
  						
				  					  if (copyCtrl[port].cpLinkHead!=NULL)
				  					  {
				  					    copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_CHECK_TIME;
				  					  }
				  					}
          					
				  					//2.��ȡ��վ���ƽ�ֹʱ��
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
				  					  pFoundCpLink->msCtrlCmd = buf[14];     //��վֱ�ӿ�������
				              pFoundCpLink->msCtrlTime = tmpTime;    //��վ���ƽ�ֹʱ��
				            }
				      
				  				  if (debugInfo&METER_DEBUG)
				  					{
                      if (buf[14]>100)
                      {
                        printf("  -����վ��������");
                      }
                      else
                      {
                        printf("  -��վ��������=%d", buf[14]);
                      }
                      printf(",��ֹʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
          					}
          					
  									//3.�ж�XLCʱ���뼯�����Ƿ�һ��
										//2016-11-02�𣬿���ʱ�β����������﷢��,��00900400�з���

										//����ң���Ƿ����
										tmpTail = 29;
				  					if (buf[tmpTail]!=copyCtrl[port].cpLinkHead->joinUp)
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·�������Ƿ���뷴��ң���뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
          					tmpTail++;

  									//2015-6-11,add,����ģʽ
  									if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --��·����������ģʽ=%d\n", buf[tmpTail]);
                  	}
				  					//if (buf[tmpTail]!=ctrlMode)
				  					//2016-02-15,�ĳ�Ҫ���Ƶ�Ŀ���ģʽ
				  					if (buf[tmpTail]!=copyCtrl[port].cpLinkHead->bigAndLittleType)
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·����������ģʽ�뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
          					tmpTail++;

		          			//2015-6-26,add,��ؿ���ǰ���ٷ�����Ч
		          			if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --��·��������غ�բǰ%d������Ч\n", buf[tmpTail]);
                  	}
				  					if (buf[tmpTail]!=beforeOnOff[0])
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·��������غ�բǰ�������뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
          					tmpTail++;

          					if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --��·��������غ�բ��%d������Ч\n", buf[tmpTail]);
                  	}
  									if (buf[tmpTail]!=beforeOnOff[1])
  									{
  					 			 		copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
  					  
  					  				if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·��������غ�բ��������뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
          					tmpTail++;

          					if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --��·��������ط�բǰ%d������Ч\n", buf[tmpTail]);
                  	}
				  					if (buf[tmpTail]!=beforeOnOff[2])
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·��������ط�բǰ�������뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
          					tmpTail++;

          					if (debugInfo&METER_DEBUG)
                  	{
                  	  printf("  --��·��������ط�բ��%d������Ч\n", buf[tmpTail]);
                  	}
				  					if (buf[tmpTail]!=beforeOnOff[3])
				  					{
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·��������ط�բ��������뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
          					tmpTail++;
          				  
				  					//6.XLC��⵽����һ�εƹ��Ϸ���ʱ��
				  					tmpTime.year = buf[tmpTail+5];
				  					tmpTime.month = buf[tmpTail+4];
				  					tmpTime.day = buf[tmpTail+3];
				  					tmpTime.hour = buf[tmpTail+2];
				  					tmpTime.minute = buf[tmpTail+1];
				  					tmpTime.second = buf[tmpTail];
				  					tmpTail+=6;
				  					if (debugInfo&METER_DEBUG)
				  					{
				  						printf("  -XLC��һ����·���Ϸ���ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
				  					}
                    
                    if (tmpTime.year!=0xff && tmpTime.month!=0xff && tmpTime.day!=0xff && tmpTime.minute!=0xff && tmpTime.second!=0xff)
                    {
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);    //��������ʱ���޹���
                      if (tmpTime.year!=meterStatisExtranTimeS.lastFailure.year
                      	  || tmpTime.month!=meterStatisExtranTimeS.lastFailure.month
                      	   || tmpTime.day!=meterStatisExtranTimeS.lastFailure.day
                      	    || tmpTime.hour!=meterStatisExtranTimeS.lastFailure.hour
                      	     || tmpTime.minute!=meterStatisExtranTimeS.lastFailure.minute
                      	      || tmpTime.second!=meterStatisExtranTimeS.lastFailure.second
                      	 )
                      {
                      	meterStatisExtranTimeS.lastFailure = tmpTime;
                      	
                        //�洢���Ƶ�ͳ������(��ʱ���޹���)
                        saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                        
                        //��¼������������Ϣ(ERC13)
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
										  	  printf("  --��¼���Ƶ�%d�����¼�\n", copyCtrl[port].cpLinkHead->mp);
										    }
                        
                        activeReport3();                  //�����ϱ��¼�
                      }
                    }
                    
                    //7.���һ�ν�ͨ���Ͽ�ʱ��
				  					//7.1���һ�ν�ͨʱ��
				  					tmpTime.year = buf[tmpTail+4];
				  					tmpTime.month = buf[tmpTail+3];
				  					tmpTime.day = buf[tmpTail+2];
				  					tmpTime.hour = buf[tmpTail+1];
				  					tmpTime.minute = buf[tmpTail+0];
				  					tmpTime.second = 0x00;
				  					tmpTail+=5;
				  					if (debugInfo&METER_DEBUG)
				  					{
				  					  printf("  -XLC��һ�ν�ͨʱ��:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
				  					}
                    saveSlDayData(copyCtrl[port].cpLinkHead->mp, 1, tmpTime, (INT8U *)&tmpTime, 6);
  
				  					//7.2���һ���ж�ʱ��
				  					tmpTime.year = buf[tmpTail+4];
				  					tmpTime.month = buf[tmpTail+3];
				  					tmpTime.day = buf[tmpTail+2];
				  					tmpTime.hour = buf[tmpTail+1];
				  					tmpTime.minute = buf[tmpTail+0];
				  					tmpTime.second = 0x00;
				  					tmpTail+=5;
				  					if (debugInfo&METER_DEBUG)
				  					{
				  						printf("  -XLC��һ���ж�ʱ��:%02x-%02x-%02x %02x:%02x\n", tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute);
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
				  						//2019-01-22,��γ�Ȳ���ͬ�����־ԭ���ô���
				  					  //copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_1;
				  					  copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
				  					  
				  					  if (debugInfo&METER_DEBUG)
                  	  {
                  			printf("  --��·���Ƶ㾭γ���뼯������һ��,Ӧͬ��\n");
                  	  }
          					}
				  					if (debugInfo&METER_DEBUG)
				  					{
									  	printf("  -XLC����:%d.%04d\n", buf[tmpTail+0], buf[tmpTail+1] | buf[tmpTail+2]<<8);
									  	printf("  -XLCγ��:%d.%04d\n", buf[tmpTail+3], buf[tmpTail+4] | buf[tmpTail+5]<<8);
									  	printf("  -XLC���ݾ�γ�ȼ�������ճ�ʱ��:%02d:%02d\n", buf[tmpTail+7], buf[tmpTail+6]);
									  	printf("  -XLC���ݾ�γ�ȼ����������ʱ��:%02d:%02d\n", buf[tmpTail+9], buf[tmpTail+8]);

									  	printf("  -XLC��γ�ȿ���ģʽ��բ΢��:");
									  	if (buf[tmpTail+10]&0x80)
									  	{
												printf("-");
									  	}
									 	 	printf("%02d\n", buf[tmpTail+10]&0x7f);
									
									  	printf("  -XLC��γ�ȿ���ģʽ��բ΢��:");
									  	if (buf[tmpTail+11]&0x80)
									  	{
												printf("-");
									  	}
									  	printf("%02d\n", buf[tmpTail+11]&0x7f);
				  					}
          					
				  					//�ճ�����ʱ��
				  					if (pFoundCpLink!=NULL)
				  					{
				  					  //��γ�ȿ���ʱ,�ճ�����ʱ�̱������µ���վ������
									  	//��γ�Ƚ�Ϲ��ʱ,�ճ�����ʱ�̱������µ���վ������,2016-10-20
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
          					
  									//��/��բ΢��
  									if (
					  						buf[tmpTail+10]!=copyCtrl[port].cpLinkHead->duty[4]
					  						 || buf[tmpTail+11]!=copyCtrl[port].cpLinkHead->duty[5]
					  					   )
					  				{
  					  				copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_SYN_CTRL_TIME_3;
  					  
  					  				if (debugInfo&METER_DEBUG)
		          	  		{
		          					printf("  --��·���Ƶ㾭γ�ȿ��ƺ�/��բ΢���뼯������һ��,Ӧͬ��\n");
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
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)Сʱ��������\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
  
      					copyCtrl[port].cpLinkHead->flgOfAutoCopy |= REQUEST_HOUR_FREEZE;
      		  	}
      		  	break;
      			
      			case 0x94:    //дʱ��ȷ��
						  if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_SYN_CTRL_TIME_1)
						  {
						    //�����������ʱ��1��־,�����¶�ȡ����ʱ��1
						    copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_1 | REQUEST_CTRL_TIME_1);
						  
						    //����й�����������غϡ���բ����
                copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
                copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;

		      			if (debugInfo&METER_DEBUG)
		      			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)��������ʱ�ε�һ����ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
              }
						  else
						  {
								if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_SYN_CTRL_TIME_2)
								{
								  //�����������ʱ��2��־,�����¶�ȡ����ʱ��2
								  copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_2 | REQUEST_CTRL_TIME_2);
								
								  //����й�����������غϡ���բ����
								  copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
								  copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;

								  if (debugInfo&METER_DEBUG)
								  {
										printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)��������ʱ�εڶ�����ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
								  }
								}
								else
								{
								  if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_SYN_CTRL_TIME_3)
								  {
										//�����������ʱ��3��־,�����¶�ȡ״̬����
										copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~(REQUEST_SYN_CTRL_TIME_3 | REQUEST_STATUS);
									
										//����й�����������غϡ���բ����
										copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
										copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;

										if (debugInfo&METER_DEBUG)
										{
									 	 	printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)��������ʱ�ε�������ȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
										}
								  }
								}
							}
      		  	break;
      				
						case 0x9C:    //ȷ��
						case 0xDC:    //����
  			  		if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_CLOSE_GATE)
  			  		{
  							//�����բ��־
  							copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_CLOSE_GATE;
  							if (debugInfo&METER_DEBUG)
  							{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)��բ", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                  if (0x9c==buf[8])
                  {
              	    printf("ȷ��\n");
                  }
                  else
                  {
                  	printf("����\n");
                  }
                }
                
                
                if (pFoundCpLink!=NULL)
                {
                  //�ӷ�բ��ɺ�բ
                  if (0x9c==buf[8])
                  {
                  	pFoundCpLink->gateOn = 1;
                  }
                  
                  //����,����غ�բ������Ϊ��բ
                  //2016-03-17,ע����һ���ж�
                  //if (1==pFoundCpLink->lcOnOff && 0xdc==buf[8])
                  //{
                	//  pFoundCpLink->lcOnOff = 0;
                  //}
                }
              }
  			  		if (copyCtrl[port].cpLinkHead->flgOfAutoCopy & REQUEST_OPEN_GATE)
			  			{
			    			//�����բ��־
			    			copyCtrl[port].cpLinkHead->flgOfAutoCopy &= ~REQUEST_OPEN_GATE;
			    			if (debugInfo&METER_DEBUG)
			    			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)��բ", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                  if (0x9c==buf[8])
                  {
                  	printf("ȷ��\n");
                  }
                  else
                  {
                  	printf("����\n");
                  }
                }
                
                //����,����ط�բ������Ϊ��բ
                //2016-03-17,ע����һ���ж�
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

      			case 0xd1:    //�쳣Ӧ��
  			  		if (0x01==buf[9] && 0x12==buf[10])
			  			{
								if (debugInfo&METER_DEBUG)
			    			{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�XLC(%02x%02x%02x%02x%02x%02x)��Сʱ��������Ӧ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
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
    
    //RS485�ӿ��Ͻӵ����µ����������
    if (port<4 && LIGHTING_DGM==copyCtrl[port].cpLinkHead->protocol)
    {
      if (
      	  buf[0]==0x68 
      	   && buf[7]==0x68 
      	    && buf[len-1]==0x16
      	     && compareTwoAddr(copyCtrl[port].cpLinkHead->addr, &buf[1], 0)==TRUE    //2016-03-21,��ӵ�ַ�Ƚ�
      	 )
      {
      	checkSum = 0;
      	for(tmpi=0; tmpi<len-2; tmpi++)
      	{
      	  checkSum += buf[tmpi];
          
          if (tmpi>9)
          {
            buf[tmpi] -= 0x33;    //���ֽڽ��ж��������0x33����
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
				  					  //����:��ΰ����1#�ߵı�������������1���µ����к��ֶ����·���ϵ�ʱ�󱨾�һ�������쳣
				  					  //     �����Ӻ��ֲ���һ���ָ��¼�
				  					  //�޸�:��·���޵��Ϊ�н�����ʱ,���������Ƶ��ĩ�˵����ز����Ƶ�����ʱ��
				  					  //     ���Ǹ��ϵ�ʱ�ز����罨����ʱ��(CARRIER_SET_UP)
				  					  //2015-5-25
				  					  //�޸�:Ϊ��Զ�����õġ��ϵ�ʱ�ز����罨��ʱ�䡱
				  					  if (
				  					  	  0x00==(pFoundCpLink->status&0x1)    //��������·ԭ��Ϊֱ��
				  					  	   && 0x01==(buf[21]&0x1)             //���βɼ���Ϊ��������
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
					  		 	 	   	 		 		printf("%02d-%02d-%02d %02d:%02d:%02d:�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)�ĵ�һĩ�˵���(%02x-%02x-%02x-%02x-%02x-%02x)�����ʱ��%02d-%02d-%02d %02d:%02d:%02d",
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
				    	  		 	 	   	 	 	printf(",����Ϊ%02d-%02d-%02d %02d:%02d:%02d\n",
				    	  		 	 	   	 	        tmpCpLink->lddtStatusTime.year,tmpCpLink->lddtStatusTime.month, tmpCpLink->lddtStatusTime.day, tmpCpLink->lddtStatusTime.hour, tmpCpLink->lddtStatusTime.minute, tmpCpLink->lddtStatusTime.second
				    	  		 	 	   	 		   );
				    	  		 	 	   	  }
				    	  		 	 	   	 	 
				    	  		 	 	   	   break;
				    	  		 	 	   	}
				    	  		 	 	   	 
				    	  		 	 	   	tmpCpLink = tmpCpLink->next;
				    	  		 	 	  }
				            	 	}
                    	 	 
                 	 	 
         	 	        		//�ڶ�ĩ�˵��ƿ�������ַ��Ϊȫ0
         	 	        		if (FALSE == compareTwoAddr(pFoundCpLink->collectorAddr, NULL, 1))
         	 	        		{
         	 	 	      			tmpCpLink = copyCtrl[4].cpLinkHead;
 	  		 	 	      				while(tmpCpLink!=NULL)
 	  		 	 	      				{
				 	  		 	 	   	    if (compareTwoAddr(tmpCpLink->addr, pFoundCpLink->collectorAddr, 0)==TRUE)
				 	  		 	 	   	    {
					 	  		 	 	   	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
					 	  		 	 	   	 	  {
					    	  		 	 	   	 	printf("%02d-%02d-%02d %02d:%02d:%02d:�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)�ĵڶ�ĩ�˵���(%02x-%02x-%02x-%02x-%02x-%02x)�����ʱ��%02d-%02d-%02d %02d:%02d:%02d",
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
						  		 	 	   	 	    printf(",����Ϊ%02d-%02d-%02d %02d:%02d:%02d\n",
						  		 	 	   	 		         tmpCpLink->lddtStatusTime.year,tmpCpLink->lddtStatusTime.month, tmpCpLink->lddtStatusTime.day, tmpCpLink->lddtStatusTime.hour, tmpCpLink->lddtStatusTime.minute, tmpCpLink->lddtStatusTime.second
						  		 	 	   	 		       );
						  		 	 	   	 	  }
					 	  		 	 	   	 		   
					 	  		 	 	   	 	  break;
					 	  		 	 	   	 	}

	    	  		 	 	   				tmpCpLink = tmpCpLink->next;
	 	  		 	 	   	  			}
	 	  		 	 	   				}
         	  		 	 	   	 
				 	  		 	 	   	//2015-11-09,Add
				 	  		 	 	   	//����·�������������,�ñ����������ɼ��Ĺ��緽ʽ�����ٵ��ƿ������ƿ�״̬�Ĳɼ�
				  					  	if (xlcLink==NULL)    //����·���Ƶ�
				  					  	{
				  					  	  //֪ͨ���е�����·���ϵ�
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				         	  		 	while(tmpNode!=NULL)
				         	  		 	{
				 	  		 	 	      	tmpNode->lineOn = 1;

				 	  		 	 	      	tmpNode->statusTime = sysTime;        //����״̬ʱ���Ϊ��ǰʱ��
				 	  		 	 	      	tmpNode->lddtStatusTime = sysTime;    //����״̬ʱ���Ϊ��ǰʱ��,2016-02-03,Add

				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
									    	}
									  	}
          					  
				  					  //2015-3-11,
				  					  //����:��ΰ����1#�ߵı�������������1���µ����к��ֶ����·�նϵ�ʱ�󱨾�һ�������쳣
				  					  //�޸�:��·���н������Ϊ������ʱ,������ڽ�������ĩ�˵����ز����Ƶ�,ֹͣ����
				  					  if (
				  					  	  0x01==(pFoundCpLink->status&0x1)    //�ϴβɼ���������·��������
				  					  	   && 0x00==(buf[21]&0x1)             //��βɼ���Ϊֱ������
				                            && 1==carrierFlagSet.searchLddt)  //��������LDDT
				          		{	
                	 			//�̶�ÿ1���Ӳ�ѯһ��
                	 			carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 1, 0);

                    		//�ָ�����
                        carrierFlagSet.workStatus = 6;
                        carrierFlagSet.reStartPause = 3;
                        copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);

                        carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                             
  					    				if (debugInfo&METER_DEBUG)
  					    				{
                          printf("%02d-%02d-%02d %02d:%02d:%02d,��·�ӽ����л�Ϊֱ��,ֹͣ����LDDT�ָ�����\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6]);
  					    				}
          			  		}
          					  
				  					  //2015-10-26,Add
				  					  //����·�������������,�ñ����������ɼ��Ĺ��緽ʽ��Ϊ���ƿ�������Ϊ�ص�״̬
				  					  if (
				  					  	  0x01==(pFoundCpLink->status&0x1)    //�ϴβɼ���������·��������
				  					  	   && 0x00==(buf[21]&0x1)             //��βɼ���Ϊֱ������
				                         )
				          		{
				  					  	//����·���Ƶ�
				  					  	if (xlcLink==NULL)
				  					  	{
				  					  	  //���е���ӦΪ��(����Ϊ0%)
				  					  	  tmpNode = copyCtrl[4].cpLinkHead;
				 	  		 	 	      while(tmpNode!=NULL)
				 	  		 	 	      {
				 	  		 	 	      	tmpNode->status = 0;
				 	  		 	 	      	tmpNode->statusUpdated |= 1;
				 	  		 	 	      	
				 	  		 	 	      	tmpNode->lineOn = 0;    //�ز���·�ϵ�,2015-11-24,�����һ��,
				 	  		 	 	      	                        //  ��Ϊ��6905��1#���б���������û����·������,
				 	  		 	 	      	                        //  ��Ҫ�ñ�������������Ϊ�ز���·�ϵ�,��Ȼ���е��ƿ������ߵ���
				 	  		 	 	      	
				 	  		 	 	      	tmpNode = tmpNode->next;
				 	  		 	 	      }
				  					  	}
				  					  }
				  					  
				  					  pFoundCpLink->statusTime = sysTime;    //LDGMû��ʱ��,ȡ��������ǰʱ��

                      pFoundCpLink->status = buf[21];        //LDGM״̬
                                                             // bit0-��·����(1)���绹��ֱ��(0)����
                                                             // bit1-��·�쳣?1-�쳣,0-����
                                                             // bit2-װ���쳣?
                                                             // bit3-ֱ������Ѿ�����һ���ж�����(����һ���ж�����,�������������ж�)
                                                             // bit4-��1��ʾֱ�������
                                                             // bit8-��������������1-�ز�����,0-ֱ������
                      //��ǰ��������ֵ
                      pFoundCpLink->duty[0] = buf[16];
                      pFoundCpLink->duty[1] = buf[17];
                      pFoundCpLink->duty[2] = buf[18];
                      
                      //��ǰ������ѹֵ                         
                      pFoundCpLink->duty[3] = buf[19];
                      pFoundCpLink->duty[4] = buf[20];
                      
                      if (debugInfo&METER_DEBUG)
          			  		{
                        printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�LDGM(%02x%02x%02x%02x%02x%02x)ʵʱ���ݰ�:\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                        printf("  -LDGM�趨��������ֵ:%5.2fA\n", (float)(buf[14] | buf[15]<<8)/100);
                        printf("  -LDGM��ǰ������ֵ:%6.3fA\n", (float)(pFoundCpLink->duty[0] | pFoundCpLink->duty[1]<<8 | pFoundCpLink->duty[2]<<16)/1000);
                        printf("  -LDGM��ǰ����ѹֵ:%5.1fV\n", (float)(pFoundCpLink->duty[3] | pFoundCpLink->duty[4]<<8)/10);
                        printf("  -LDGM��ǰ���緽ʽ:%s\n", (1==(pFoundCpLink->status&0x1))?"����":"ֱ��");
                        printf("  -LDGM��ǰ�Ƿ����:%d\n", pFoundCpLink->status>>1&0x1);
                        printf("  -LDGMװ���Ƿ����:%d\n", pFoundCpLink->status>>2&0x1);
                        printf("  -LDGM����һ���ж�����:%d\n", pFoundCpLink->status>>3&0x1);
                        printf("  -LDGMֱ�����:%d\n", pFoundCpLink->status>>4&0x1);
                      }
                    }
                    
                    if(
                       (0x00==(buf[21]&0x80))    //ֱ����������������
                    	 //startCurrent-��·�������ֽ�
                         //    ctrlTime-��·�����������ֽ�
                    	 && (buf[14]!=copyCtrl[port].cpLinkHead->startCurrent || buf[15]!=copyCtrl[port].cpLinkHead->ctrlTime)
                      )
                    {
                      copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x02;
                      
                      if (debugInfo&METER_DEBUG)
                      {
                        printf("  -LDGM01��·��������ֵ����վ���õ�ֵ��һ��,����У׼\n");
                      }
                    }
                    else
                    {
                      if(
                    	  (0x80==(buf[21]&0x80))    //�ز���������������
                    	   && (
                    	       compareTwoAddr(&buf[22], copyCtrl[port].cpLinkHead->lddt1st, 0)==FALSE
                    	        || compareTwoAddr(&buf[28], copyCtrl[port].cpLinkHead->collectorAddr, 0)==FALSE
                    	      )
                    	  )
                      {
                        copyCtrl[port].cpLinkHead->flgOfAutoCopy = 0x04;
                      
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("  -LDGM02��ĩ�����ĵ�ַ����վ���õ�ֵ��һ��,����У׼\n");
                        }
                      }
                      else
                      {
                        //��·��ֱ�����硢ֱ���������ֱ������ѹ�һ���ж������ǲ��ж���·�쳣
                        if (
                        	 (0==(buf[21]&0x1))             //��·Ϊֱ������
                        	  && (0x10==(buf[21]&0x10))     //ֱ�������,2015-5-29,add
                        	   && (0x08==(buf[21]&0x08))    //ֱ������ѳ���һ���ж�����,2015-5-29,add
                        	)
                        {
                          searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3); //������������ʱ���޹���
                        	
                          //�����������ж�Ϊ��·�쳣,��¼�����¼�
                          if (buf[21]&0x02)
                          {
                    				//δ��¼�Ĳż�¼
                    				if (0==(meterStatisExtranTimeS.mixed&0x02))
                    				{
                          	  //��¼���Ƶ������Ϣ(ERC13)
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
                          	  eventData[9] = (copyCtrl[port].cpLinkHead->mp>>8&0xff) | 0x80;    //����
                          	  eventData[10] = 0x20;
                          	  
                          	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                          	  
                          	  if (eventRecordConfig.nEvent[1]&0x10)
                          	  {
                          	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                          	  }
                          	  
                          	  eventStatus[1] = eventStatus[1] | 0x10;
                  					  
          					  				if (debugInfo&METER_DEBUG)
          					  				{
          					  					printf("  --��¼���Ƶ�%d�쳣�����¼�\n", copyCtrl[port].cpLinkHead->mp);
          					  				}
                              
                              activeReport3();    //�����ϱ��¼�
                          	  
                          	  meterStatisExtranTimeS.mixed |= 0x02;
                                
                              //�洢���Ƶ�ͳ������(��ʱ���޹���)
                              saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                            }
                          }
                          else
                          {
                        		//�Ѿ��������Ҽ�¼���쳣,��¼�ָ��¼�
	                        	if (
	                        		(meterStatisExtranTimeS.mixed&0x02)        //�޵緢����
	                        		 || (meterStatisExtranTimeS.mixed&0x04)    //�е緢����
	                        	   )
	                        	{
                          	  //��¼���Ƶ������Ϣ(ERC13)
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
                          	  eventData[9] = (copyCtrl[port].cpLinkHead->mp>>8&0xff) | 0x00;    //�ָ�
                          	  eventData[10] = 0x20;
                          	  
                          	  writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                          	  
                          	  if (eventRecordConfig.nEvent[1]&0x10)
                          	  {
                          	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                          	  }
                          	  
                          	  eventStatus[1] = eventStatus[1] | 0x10;
                  					  
				          					  if (debugInfo&METER_DEBUG)
				          					  {
				          					  	printf("  --��¼���Ƶ�%d�쳣�ָ��¼�\n", copyCtrl[port].cpLinkHead->mp);
				          					  }
				                              
                              activeReport3();    //�����ϱ��¼�
                              
			                			  if (meterStatisExtranTimeS.mixed&0x02)
			                			  {
			                			    meterStatisExtranTimeS.mixed &= 0xfd;
			                			  }
			                			
			                			  //2015-12-17
			                			  if (meterStatisExtranTimeS.mixed&0x04)
			                			  {
			                			    meterStatisExtranTimeS.mixed &= 0xfb;
			                			  }
                              
                              //�洢���Ƶ�ͳ������(��ʱ���޹���)
                              saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                        		}
                          }
                        }

                        //ÿ15���Ӵ洢һ������
                        if (
                        	//2015-12-28,���ָ��������Ƶ�ʱ��0��0��ȱ������,����������Ƿ��Ч
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
						  	        
						  	          //��ѹ
						  	          tmpData = hexToBcd(buf[19] | buf[20]<<8);
						  	          hourDataBuf[5] = tmpData&0xff;
						  	          hourDataBuf[6] = tmpData>>8&0xff;
			 
						  	          //����
						  	          tmpData = hexToBcd(buf[16] | buf[17]<<8 | buf[18]<<16);
						  	          hourDataBuf[7] = tmpData&0xff;
						  	          hourDataBuf[8] = tmpData>>8&0xff;
						  	          hourDataBuf[9] = tmpData>>16&0xff;
						  	        
						  	          //״̬
						  	          hourDataBuf[28] = buf[21];
						  	          hourDataBuf[29] = 0x00;
			  	        
						  	          //��·��������ʱ,����ز��ж�Ϊ�����쳣Ҳ��¼Ϊ�����쳣
						  	          if (0x1==(buf[21]&0x1))
						  	          {
			                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);    //������������ʱ���޹���
			      				  	    if (0x4==(meterStatisExtranTimeS.mixed&0x4))
			      				  	    {
			      				  	      hourDataBuf[28] |= 0x02;
			      				  	        		
			      				  	      if (debugInfo&METER_DEBUG)
			      				  	      {
			                          printf("%02d-%02d-%02d %02d:%02d:%02d,�������Ƶ�%d�ز��ж�Ϊ�쳣��¼������������\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,copyCtrl[port].cpLinkHead->mp);
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
  				  		copyCtrl[port].cpLinkHead->flgOfAutoCopy &= 0xfb;    //�������ĩ������ַ��־,�����¶�ȡʵʱ���ݰ�

  				  		if (debugInfo&METER_DEBUG)
  				  		{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�LDGM(%02x%02x%02x%02x%02x%02x)����ĩ������ַȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
      				}
      				else
      				{
  				  		copyCtrl[port].cpLinkHead->flgOfAutoCopy &= 0xfd;    //���������·��������ֵ��־,�����¶�ȡʵʱ���ݰ�

  				  		if (debugInfo&METER_DEBUG)
  				  		{
                  printf("%02d-%02d-%02d %02d:%02d:%02d,�յ�LDGM(%02x%02x%02x%02x%02x%02x)������·��������ֵȷ��\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,buf[6],buf[5],buf[4],buf[3],buf[2],buf[1]);
                }
              }
      				break;
      				
      		  case 0xd1:    //�쳣Ӧ��
      				break;
      	  }
      	}
      }
      
      copyCtrl[port].copyContinue = TRUE;
      
      return;
    }
    
    //RS485�ӿ��Ͻӵ��նȴ�����
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

        if (1==copyCtrl[port].cpLinkHead->ctrlTime)    //�ܻ�
        {
          tmpData = buf[3]<<24 | buf[4]<<16 | buf[5]<<8 | buf[6];
        }
        else    //���ٷ���
        {
          tmpData = (buf[5]<<8 | buf[6])*10;
        }
        
      	
        if (pFoundCpLink!=NULL)
        {
          lcProcess(pFoundCpLink->duty, tmpData);
          
          //2016-03-22,Add,����һ��ֵ��ŵ����ϴ�ֵλ��
          pFoundCpLink->duty[3] = pFoundCpLink->duty[0];
          pFoundCpLink->duty[4] = pFoundCpLink->duty[1];
          pFoundCpLink->duty[5] = pFoundCpLink->duty[2];
          
          //��ǰֵ��ŵ���һ��ֵλ��
          pFoundCpLink->duty[0] = tmpData & 0xff;
          pFoundCpLink->duty[1] = tmpData>>8 & 0xff;
          pFoundCpLink->duty[2] = tmpData>>16 & 0xff;
        
          pFoundCpLink->statusTime = sysTime;

          if (debugInfo&METER_DEBUG)
          {
            printf("���ն�=%d\n", tmpData);
            printf("%02d-%02d-%02d %02d:%02d:%02d,%02x%02x%02x%02x%02x%02x,���ն�ֵ=%d\n", 
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

		//RS485�ӿ��Ͻӵ���ʪ�ȴ�����
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
          //��ǰֵ��ŵ���һ��ֵλ��
          pFoundCpLink->duty[0] = buf[3];
          pFoundCpLink->duty[1] = buf[4];
          pFoundCpLink->duty[2] = buf[5];
          pFoundCpLink->duty[3] = buf[6];

					
          pFoundCpLink->statusTime = sysTime;

          if (debugInfo&METER_DEBUG)
          {
            printf("%02d-%02d-%02d %02d:%02d:%02d,%02x%02x%02x%02x%02x%02x,ʪ��=%d,�¶�=%d\n", 
                   sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                   pFoundCpLink->addr[5],pFoundCpLink->addr[4],pFoundCpLink->addr[3],pFoundCpLink->addr[2],pFoundCpLink->addr[1],pFoundCpLink->addr[0],
                   pFoundCpLink->duty[1] | pFoundCpLink->duty[0]<<8, bmToYm(&pFoundCpLink->duty[2], 2)
                  );
          }

					//ÿ15���Ӵ洢һ������
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
						
						//ʪ��
						hourDataBuf[28] = pFoundCpLink->duty[1];
						hourDataBuf[29] = pFoundCpLink->duty[0];
						//�¶�,���¶����ò����ʾ��
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
							printf("%02d-%02d-%02d %02d:%02d:%02d,�����¶�ʪ��\n", 
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
  	    //printf("PORT:%d��������֡�����ҽ�����ȷ,�ѱ��������\n",port);
  	  }
  	  
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (port==PORT_POWER_CARRIER)
  	  {
        //·����������Ļ��Ͳ��ó��������־,�ȴ�·������
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
              meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //��֡����
               
              switch (pDotCopy->dataFrom)
              {
                case DOT_CP_SINGLE_MP:   //������������㳭��ʱ�㱨�������
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
  	    
  	    //12-09-20,add ABB ����
  	    if (abbHandclasp[port-1]==1)
  	    {
  	      abbHandclasp[port-1] = 0;
  	    	 
  	      if (debugInfo&METER_DEBUG)
  	      {
  	    		printf("�˿�%d��ABB���Ѿ����ֳɹ�\n", port);
  	      }
  	    }
  	    
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  }
  	 #endif
  	  break;
  	  	
  	case METER_NORMAL_REPLY:
  	  if (debugInfo&METER_DEBUG)
  	  {
  	    //printf("PORT:%d��������֡�����ұ������Ӧ��(���֧�ָ�����)\n",port);
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
	            meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //��֡����
	    			}
	    			else
	    			{
	            switch (pDotCopy->dataFrom)
	            {
	              case DOT_CP_SINGLE_MP:   //������������㳭��ʱ�㱨�������
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

  	    //12-09-20,add ABB ����
  	    if (abbHandclasp[port-1]==1)
  	    {
    	   	abbHandclasp[port-1] = 0;
    	 
    	   	if (debugInfo&METER_DEBUG)
    	   	{
    	     	printf("�˿�%d��ABB���Ѿ����ֳɹ�(METER_NORMAL_REPLY)\n", port);
    	   	}
  	    }

  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  }
  	 #endif
  	 	break;

  	case METER_ABERRANT_REPLY:
  		if (debugInfo&METER_DEBUG)
  		{
  	 	  //printf("PORT:%d��������֡����,������쳣Ӧ��(�����Ǳ�˲�֧�ָ�����)\n",port);
  	 	}
  	 
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (port>=PORT_POWER_CARRIER)
  	  {
  	    if (debugInfo&PRINT_CARRIER_DEBUG)
  	    {
  	    	printf("����쳣Ӧ��(���ܱ�˲�֧�ָ�����)\n");
  	    }
  	    		
  	    //2014-01-08,�������ж�
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
  	 	
  	case 0:   //��������Ϊ0�ֽ�
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	  if (debugInfo&PRINT_CARRIER_DEBUG)
  	  {
       	printf("%02d-%02d-%02d %02d:%02d:%02d,meter485Receive,�ز�/����ģ�鷵������Ϊ0�ֽ�,port=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, port);
  	  }
  	  
  	  if (port==PORT_POWER_CARRIER)
  	  {
  	    if (pDotCopy!=NULL)
  	    {
  	      if (pDotCopy->dotCopying==TRUE)
  	      {
  	    		pDotCopy->dotRecvItem++;
  	    		pDotCopy->outTime = nextTime(sysTime, 0, 1);
           
            //2013-12-27,��������⼸�д���
            pDotCopy->dotResult = RESULT_NO_REPLY;

       	    printf("%02d-%02d-%02d %02d:%02d:%02d,meter485Receive,�㳭��ʱʱ�����Ϊ:%02d-%02d-%02d %02d:%02d:%02d\n", 
       	            sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
       	             pDotCopy->outTime.year, pDotCopy->outTime.month, pDotCopy->outTime.day, pDotCopy->outTime.hour, pDotCopy->outTime.minute, pDotCopy->outTime.second
       	          );
  	    		
  	    		
  	    		if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
  	    		{
		      		if (debugInfo&PRINT_CARRIER_DEBUG)
		      		{
		        		printf("%02d-%02d-%02d %02d:%02d:%02d,meter485Receive,�����㳭��һ��������,��ʱ�ȴ��������%d��\n",
                       sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
		                		MONITOR_WAIT_ROUTE);
		      		}

              meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //��֡����
              
              //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
              //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
              //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
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
  	   printf("PORT:%d�洢ƫ�Ƽ������\n",port);
  	 }
  	 break;
  }
}

/***************************************************
��������:forwardDataReply
��������:ת���ظ�
���ú���:
�����ú���:
�������:INT8U port,ת���˿��ڳ�����������е����
�������:
����ֵ��void
�޸���ʷ:
    1.2014-01-03,�޸Ĺ��������,ԭ��ת�������ݻظ��Ͳ�����վ��,�ڶ��ŵ�Ҫ����,ת�������ݻظ�Ҳ�ظ���վ0�ֽ�
***************************************************/
void forwardDataReply(INT8U portNum)
{
 #ifdef WDOG_USE_X_MEGA
  INT8U buf[1024];
 #endif

  INT16U frameTail10,tmpHead10;    //ת������֡β��֡��ʼָ��
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
		  		xlOpenCloseReply("�����ɹ���");
				}
				else
				{
		  		xlOpenCloseReply("����ʧ�ܣ�");
				}
	  	}
	 	 	else
	  	{
	    	xlOpenCloseReply("������ʱ��");
	  	}

			goto huifuRate;
	 	 #endif
		}
		
	 #ifdef LIGHTING
		else if (copyCtrl[portNum].pForwardData->dataFrom==DOT_CP_IR)
		{
			if (copyCtrl[portNum].pForwardData->forwardResult==RESULT_HAS_DATA)
			{
				irStudyReply("ѧϰ�ɹ���");

				for(tmpi=copyCtrl[portNum].pForwardData->length; tmpi>0; tmpi--)
				{
					copyCtrl[portNum].pForwardData->data[tmpi+1] = copyCtrl[portNum].pForwardData->data[tmpi-1];
				}

				//����
				copyCtrl[portNum].pForwardData->data[0] = copyCtrl[portNum].pForwardData->length&0xff;
				copyCtrl[portNum].pForwardData->data[1] = copyCtrl[portNum].pForwardData->length>>8&0xff;
				

				//printf("����%d��������Length=%d:", copyCtrl[portNum].pForwardData->data[768], copyCtrl[portNum].pForwardData->length);
				//for(tmpi=0; tmpi<copyCtrl[portNum].pForwardData->length+2; tmpi++)
				//{
				//	printf("%02x ", copyCtrl[portNum].pForwardData->data[tmpi]);
				//}
				//printf("\n");

				//����ѧϰ���ĺ�������
				saveParameter(5, 160+copyCtrl[portNum].pForwardData->data[768]-4, copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length+2);
				
		 	}
		 	else
		 	{
				if (copyCtrl[portNum].pForwardData->data[768]>3)
				{
					irStudyReply("ѧϰ��ʱ��");
				}
				else
				{
					irStudyReply("�����ѷ�����");					
				}
		 	}
			
			goto huifuRate;
		}
	 #endif
	 
		else if (copyCtrl[portNum].pForwardData->forwardResult==RESULT_HAS_DATA)
	  {
			if (debugInfo&METER_DEBUG)
			{
	  		printf("%02d-%02d-%02d %02d:%02d:%02d,ת�������ݻظ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
	  		printf("%02d-%02d-%02d %02d:%02d:%02d,ת�������ݻظ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	}
    }
  }
  else   //FN==9
  {
		frameTail10 = tmpHead10 + 19;
 
		//ת��Ŀ���ַ
		if (copyCtrl[portNum].pForwardData->data[28]==0x81 || copyCtrl[portNum].pForwardData->data[28]==0x91 || copyCtrl[portNum].pForwardData->data[28]==0xb1)
		{
	  	memcpy(&msFrame[frameTail10], &copyCtrl[portNum].pForwardData->data[21], 0x06);
	  	frameTail10+=6;
	 
	  	//ת�������־
	  	msFrame[frameTail10++] = 0x05;    //ת����������
	 
	  	//ת��ֱ�ӳ��������������ֽ�k+4
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
				//ת�������־
				msFrame[frameTail10++] = 0x04;    //ת�����շ���
      }
      else
      {
				//ת�������־
				msFrame[frameTail10++] = 0x01;    //ת�����ճ�ʱ           	 
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
 
  //��������վҪ���ж��Ƿ�Я��TP
  if (frame.pTp != NULL)
  {
		msFrame[frameTail10++] = *frame.pTp;
		msFrame[frameTail10++] = *(frame.pTp+1);
		msFrame[frameTail10++] = *(frame.pTp+2);
		msFrame[frameTail10++] = *(frame.pTp+3);
		msFrame[frameTail10++] = *(frame.pTp+4);
		msFrame[frameTail10++] = *(frame.pTp+5);
  }
 
  msFrame[tmpHead10+0] = 0x68;   //֡��ʼ�ַ�

  tmpi = ((frameTail10 - tmpHead10 - 6) << 2) | PROTOCOL_FIELD;
  msFrame[tmpHead10+1] = tmpi & 0xFF;   //L
  msFrame[tmpHead10+2] = tmpi >> 8;
  msFrame[tmpHead10+3] = tmpi & 0xFF;   //L
  msFrame[tmpHead10+4] = tmpi >> 8; 
	 
  msFrame[tmpHead10+5] = 0x68;  //֡��ʼ�ַ�

  msFrame[tmpHead10+6] = 0x88;     //�����ֽ�10001000(DIR=1,PRM=0,������=0x8)
			 
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
  {
	  msFrame[tmpHead10+6] |= 0x20;
  }

  //��ַ
  msFrame[tmpHead10+7] = addrField.a1[0];
  msFrame[tmpHead10+8] = addrField.a1[1];
  msFrame[tmpHead10+9] = addrField.a2[0];
  msFrame[tmpHead10+10] = addrField.a2[1];
  msFrame[tmpHead10+11] = addrField.a3;
		 
  msFrame[tmpHead10+12] = 0x10;  //AFN
											
  msFrame[tmpHead10+13] = 0x60 | rSeq;     //01100001 ��֡
			 
  if (frame.pTp != NULL)
  {
		msFrame[tmpHead10+13] |= 0x80;       //TpV��λ
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
				printf("%02d-%02d-%02d %02d:%02d:%02d,׼������ת���ظ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
  //�ָ�ԭ�˿�����
  if (copyCtrl[portNum].backupCtrlWord!=0)
  {
		switch (portNum)
		{
	 	 #ifdef RS485_1_USE_PORT_1 
	  	case 0x0:  //RS485-1
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x01;                                     //xMega�˿�1
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //�ָ��˿�����
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("�ָ��˿�1����\n");
				}
	   	 #else
				oneUartConfig(fdOfttyS2, copyCtrl[portNum].backupCtrlWord);
	   	 #endif
				break;
					
	  	case 0x1:  //RS485-2
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x02;                                     //xMega�˿�2
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //�ָ��˿�����
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
					
				if (debugInfo&METER_DEBUG)
				{
		  		printf("�ָ��˿�2����\n");
				}
	�� 	 #else
				oneUartConfig(fdOfttyS3, copyCtrl[portNum].backupCtrlWord);
	   	 #endif
				break;

	  	case 0x2:  //RS485-3
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x03;                                     //xMega�˿�2
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //�ָ��˿�����
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("�ָ��˿�3����\n");
				}
	   	 #endif
	    	break;

	 	 #else
			
	  	case 0x1:  //RS485-1
	   	 #ifdef WDOG_USE_X_MEGA
				buf[0] = 0x01;                                     //xMega�˿�1
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //�ָ��˿�����
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("�ָ��˿�1����\n");
				}
	   	 #else
				oneUartConfig(fdOfttyS2, copyCtrl[portNum].backupCtrlWord);
	   	 #endif
	    	break;
					
	  	case 0x2:  //RS485-2
	   	#ifdef WDOG_USE_X_MEGA
				buf[0] = 0x02;                                     //xMega�˿�2
				buf[1] = copyCtrl[portNum].backupCtrlWord;         //�ָ��˿�����
				sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
				if (debugInfo&METER_DEBUG)
				{
		  		printf("�ָ��˿�2����\n");
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
		 		buf[0] = 0x03;                                     //xMega�˿�2
		 		buf[1] = copyCtrl[portNum].backupCtrlWord;         //�ָ��˿�����
		 		sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
			
		 		if (debugInfo&METER_DEBUG)
		 		{
		  		printf("�ָ��˿�3����\n");
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
 
 //�ָ�����
 #ifdef PLUG_IN_CARRIER_MODULE
  if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE && portNum==4)
  {
		carrierFlagSet.workStatus = 6;
		carrierFlagSet.reStartPause = 3;
		copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);

		carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
		if (debugInfo&PRINT_CARRIER_DEBUG)
		{
	  	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:�ȴ�%d���ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
		}
  }
 #endif
}

/***************************************************
��������:covertAcSample
��������:ת������ֵΪ�洢��ʽ
���ú���:
�����ú���:
�������:buffer    - �α�������
         visionBuf - ʾֵ����
         reqBuf    - ��������
�������:
����ֵ��void
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

   //ת���α���
   if (buffer!=NULL)
   {
     memset(buffer, 0xee, LENGTH_OF_PARA_RECORD);
     
     //����(�й����ʺ��޹�����)
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
     	 
     	 //�������λΪ1,��Ϊ����
     	 if (tmpData & 0x800000)
     	 {
     	 	  //�����뻹ԭΪԭ��ֵ�ٽ��м���
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
   	   else  //ly,2010-10-26,�ÿ�½����̨����ʱ,�޹�����ȫ�Ƿ����,���,���޹����ʷ������
   	   {
   	     //2011-09-10,�޸Ļ���
   	     if (sign==1 && tmpData!=0)
   	     {
   	       buffer[offset+2] |= 0x80;
   	     }
   	   }
     }
  
     //��ѹ
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
  
     //����
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
     
     //ly,2011-05-23,���������ʱ��0
     //ly,2011-05-25,���������ʱ��ABC�����ʸ���͵���Чֵ����,��Ϊ������û�л�����
     tmpData = hexDivision(((realAcData[31]&0x7fffff)>>acSamplePara.cAjustTimes),times2(13),3);
     buffer[ZERO_SERIAL_CURRENT]   = tmpData & 0xff;
     buffer[ZERO_SERIAL_CURRENT+1] = tmpData>>8 & 0xff;
     buffer[ZERO_SERIAL_CURRENT+2] = tmpData>>16 & 0xff;
           	    
     //��������
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
     	 
     	 //����Ǹ���,������λΪ1
     	 sign = 0;
     	 if (tmpData>>23 & 0x1)
     	 {
     	 	  //�����뻹ԭΪԭ��
     	 	  tmpData = (~tmpData & 0xffffff)+1;
     	 	  sign = 1;
     	 }
       tmpData = hexDivision(tmpData*100,times2(23),1);
  
   	   buffer[offset]   = tmpData & 0xff;
     	 buffer[offset+1] = tmpData>>8 & 0xff;
     	 
     	 //ly,2010-10-26,�ÿ�½����̨����ʱ,��׼��Ĺ�������ȫ������(���޷���,���,ȥ���ӷ��Ŵ���
    	 //if (sign==1)
     	 //{
     	 //	 buffer[offset+1] |= 0x80;
     	 //}
     }
     //���ڹ���
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
     
     //��ѹ��λ��
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
     
     //������λ��
     //��Ia = ��Ua(0) + ��UaIa
     tmpData = realAcData[22];
     if (tmpData>>23 & 0x1)
     {
     	 	//�����뻹ԭΪԭ��
     	 	tmpData = (~tmpData & 0xffffff)+1;
     }
     tmpUaIa = tmpData/times2(23)*2*180*3*14159/100000;
     buffer[PHASE_ANGLE_C_A]   = (tmpUaIa%10)<<4;
     buffer[PHASE_ANGLE_C_A+1] = (tmpUaIa/100<<4) | (tmpUaIa%100/10);
     
     //��Ib = ��Uab + ��UbIb-��UaIa
     tmpData = realAcData[23];
     if (tmpData>>23 & 0x1)
     {
     	 	//�����뻹ԭΪԭ��
     	 	tmpData = (~tmpData & 0xffffff)+1;
     }
     tmpData = tmpData/times2(23)*2*180*3*14159/100000;
     tmpData = (realAcData[26]&0x7fffff)/times2(13) + tmpData - tmpUaIa;
     buffer[PHASE_ANGLE_C_B]   = (tmpData%10)<<4;
     buffer[PHASE_ANGLE_C_B+1] = (tmpData/100<<4) | (tmpData%100/10);
  
     //��Ic = ��Uac + ��UcIc-��UaIa
     tmpData = realAcData[24];
     if (tmpData>>23 & 0x1)
     {
     	 	//�����뻹ԭΪԭ��
     	 	tmpData = (~tmpData & 0xffffff)+1;
     }
     tmpData = tmpData/times2(23)*2*180*3*14159/100000;
     tmpData = (realAcData[28]&0x7fffff)/times2(13) + tmpData - tmpUaIa;
     buffer[PHASE_ANGLE_C_C]   = (tmpData%10)<<4;
     buffer[PHASE_ANGLE_C_C+1] = (tmpData/100<<4) | (tmpData%100/10);
   }
   
   //ת��ʾֵ
   if (visionBuf!=NULL)
   {
   	 //��ȡ����ʾֵת��ǰֵ
   	 if (type==1)
   	 {
		   readAcVision(energyVision, sysTime, ENERGY_DATA);
		 }
		 else
		 {
		   readAcVision(energyVision, convertTime, ENERGY_DATA);
		 }
		 
		 //��/�����й�����/�����޹���
		 offset = POSITIVE_WORK_OFFSET;
		 for(i=0;i<8;i++)
		 {
		 	 for(j=0;j<5;j++)
		 	 {
		 	 	 visionInt = energyVision[i*25+j*5+0] | energyVision[i*25+j*5+1]<<8 | energyVision[i*25+j*5+2]<<16;
		 	 	 visionInt = hexToBcd(visionInt);
		 	 	 
		 	 	 visionDec = energyVision[i*25+j*5+3] | energyVision[i*25+j*5+4]<<8;
		 	 	 visionDec = visionDec/32;    //��������visionDec*100/3200,Լ��100����visionDec/32��
		 	 	 visionDec = hexToBcd(visionDec);
		 	 	 
		 	 	 visionBuf[offset+j*4+0] = visionDec;
		 	 	 visionBuf[offset+j*4+1] = visionInt&0xff;
		 	 	 visionBuf[offset+j*4+2] = visionInt>>8&0xff;
		 	 	 visionBuf[offset+j*4+3] = visionInt>>16&0xff;
		 	 }
		 	 
		 	 offset += 36;
		 }
		 
		 //A,B,C��/�����й�����/�����޹�
		 offset = POSITIVE_WORK_A_OFFSET;
		 for(i=0;i<12;i++)
		 {
		 	 visionInt = energyVision[POS_EPA+i*5+0] | energyVision[POS_EPA+i*5+1]<<8 | energyVision[POS_EPA+i*5+2]<<16;
		 	 visionInt = hexToBcd(visionInt);
		 	 	 
		 	 visionDec = energyVision[POS_EPA+i*5+3] | energyVision[POS_EPA+i*5+4]<<8;
		 	 visionDec = visionDec/32;    //��������visionDec*100/3200,Լ��100����visionDec/32��
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
��������:copyAcValue
��������:����������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
void copyAcValue(INT8U port, INT8U type, DATE_TIME copyTime)
{
  INT8U acParaData[LENGTH_OF_PARA_RECORD];
  INT8U visionBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U reqBuff[LENGTH_OF_REQ_RECORD];
  DATE_TIME tmpBackTime;
  
  memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);
  memset(reqBuff, 0xee, LENGTH_OF_REQ_RECORD);
  
  if (type==2)   //��������
  {
    tmpBackTime = sysTime;
    tmpBackTime.day    = 1;
    tmpBackTime.hour   = 0;
    tmpBackTime.minute = 0;
    tmpBackTime.minute = 0;
    tmpBackTime = backTime(tmpBackTime, 0, 1, 0, 0, 0);

    //ת��
    covertAcSample(NULL, visionBuff, reqBuff, 2, tmpBackTime);
  
    //�洢
    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, timeHexToBcd(sysTime), \
                  visionBuff, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, LENGTH_OF_ENERGY_RECORD);

    saveMeterData(copyCtrl[port].cpLinkHead->mp, port, timeHexToBcd(sysTime), \
                  reqBuff, LAST_MONTH_DATA, REQ_REQTIME_LASTMONTH, LENGTH_OF_REQ_RECORD);
  }
  else
  {
    //ת��
    covertAcSample(acParaData, visionBuff, reqBuff, 1, sysTime);
      
    //�洢
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
��������:fillPulseVar
��������:����������ɼ�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
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

  	  //���Ҳ����㡢���峣������������
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
    	  //���ҵ�������ѹ����������
    	  pulse[i].voltageTimes = 1;   //���û�����ò���������,Ĭ�ϵ�ѹ����������������Ϊ1
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
��������:covertPulseData
��������:ת����������Ϊ�洢��ʽ
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
void covertPulseData(INT8U port, INT8U *visionBuf,INT8U *needBuf,INT8U *paraBuff)
{
   INT32U visionInteger;                //ʾֵ����
 	 INT32U visionDecimal;                //ʾֵС��
   INT32U powerInteger;                 //��������
 	 INT32U powerDecimal;                 //����С��
   INT32U needInteger;                  //��������
 	 INT32U needDecimal;                  //����С��

 	 INT16U i;
 	   
 	 INT16U offsetVision,offsetPower,offsetNeed,offsetNeedTime;

	 //ʾֵ����
	 visionInteger = pulseDataBuff[53*port] | pulseDataBuff[53*port+1]<<8
	               | pulseDataBuff[53*port+2]<<16;
   visionInteger = hexToBcd(visionInteger);

	 //ʾֵС��(4λС��)
	 visionDecimal = (pulse[port].pulseCount*10000/pulse[port].meterConstant);
   visionDecimal = hexToBcd(visionDecimal);

   //��������
   powerInteger = pulse[port].prevMinutePulse*60/pulse[port].meterConstant;
   powerInteger = hexToBcd(powerInteger);
   
   //����С��(4λС��)
   powerDecimal = pulse[port].prevMinutePulse*60%pulse[port].meterConstant*10000/pulse[port].meterConstant;
   powerDecimal = hexToBcd(powerDecimal);
  
   //��������
   needInteger = (pulseDataBuff[53*port+5] | pulseDataBuff[53*port+6]<<8)
           *60/pulse[port].meterConstant;
   needInteger = hexToBcd(needInteger);
   
   //����С��(4λС��)
   needDecimal = (pulseDataBuff[53*port+5] | pulseDataBuff[53*port+6]<<8)
            *60%pulse[port].meterConstant*10000/pulse[port].meterConstant;
 	 needDecimal = hexToBcd(needDecimal);

   if (debugInfo&PRINT_PULSE_DEBUG)
   {
   	 printf("covertPulseData(%02d-%02d-%02d %02d:%02d:%02d):ʾֵ=%x.%04x,����=%x.%04x,����=%x.%04x\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute, sysTime.second, visionInteger,visionDecimal,powerInteger,powerDecimal,needInteger,needDecimal);
   }

 	 switch(pulse[port].character&0x3)
 	 {
 	 	  case 0:    //�����й�
 	 	  	offsetVision   = POSITIVE_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_WORK;
 	 	  	offsetNeed     = REQ_POSITIVE_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_P_WORK_OFFSET;
 	 	  	break;
 	 	  	
 	 	  case 1:    //�����޹�
 	 	  	offsetVision   = POSITIVE_NO_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_NO_WORK;
 	 	  	offsetNeed     = REQ_POSITIVE_NO_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_P_NO_WORK_OFFSET;
 	 	  	break;
 	 	  	
 	 	  case 2:    //�����й�
 	 	  	offsetVision   = NEGTIVE_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_WORK;
 	 	  	offsetNeed     = REQ_NEGTIVE_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_N_WORK_OFFSET;
 	 	  	break;
 	 	  	
 	 	  case 3:    //�����޹�
 	 	  	offsetVision   = NEGTIVE_NO_WORK_OFFSET;
 	 	  	offsetPower    = POWER_INSTANT_NO_WORK;
 	 	  	offsetNeed     = REQ_NEGTIVE_NO_WORK_OFFSET;
 	 	  	offsetNeedTime = REQ_TIME_N_NO_WORK_OFFSET;
 	 	  	break;
 	 }
   
   if (visionBuf!=NULL)
   {
     //ʾֵ
     visionBuf[offsetVision]   = visionDecimal&0xff;
     visionBuf[offsetVision+1] = visionDecimal>>8&0xff;
     visionBuf[offsetVision+2] = visionInteger&0xff;
     visionBuf[offsetVision+3] = visionInteger>>8&0xff;
     visionBuf[offsetVision+4] = visionInteger>>16&0xff;
   }
   
   if (paraBuff!=NULL)
   {  	
     //����
     paraBuff[offsetPower]   = powerDecimal&0xff;
     paraBuff[offsetPower+1] = powerDecimal>>8&0xff;
     paraBuff[offsetPower+2] = powerInteger&0xff;
   }

   if (needBuf!=NULL)
   {
     //����
     needBuf[offsetNeed]   = needDecimal&0xff;
     needBuf[offsetNeed+1] = needDecimal>>8&0xff;
     needBuf[offsetNeed+2] = needInteger&0xff;
  	
     //��������ʱ��
     needBuf[offsetNeedTime]   = hexToBcd(pulseDataBuff[53*port+7])&0xff;
     needBuf[offsetNeedTime+1] = hexToBcd(pulseDataBuff[53*port+8])&0xff;
     needBuf[offsetNeedTime+2] = hexToBcd(pulseDataBuff[53*port+9])&0xff;
     needBuf[offsetNeedTime+3] = hexToBcd(pulseDataBuff[53*port+10])&0xff;
   }
   
   if (visionBuf!=NULL)
   {
     //������ʾֵ
     offsetVision += 5;

     for(i=0;i<periodTimeOfCharge[48];i++)
     {
	     //ʾֵ����
	     visionInteger = pulseDataBuff[53*port+11+i*3] | pulseDataBuff[53*port+11+i*3+1]<<8
	                   | pulseDataBuff[53*port+11+i*3+2]<<16;
       visionInteger = hexToBcd(visionInteger);

	     //ʾֵС��(2λС��)
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
��������:searchMpStatis
��������:����������ͳ������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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

   //�������ͳ������
   tmpTime = timeHexToBcd(searchTime);

   switch (type)
   {
     case 1:    //�������ʱ���޹���
       meterStatisExtranTime = (METER_STATIS_EXTRAN_TIME *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         //printf("���Ҳ�����%dͳ������(��ʱ���޹���):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
  
       if (readMeterData((INT8U *)meterStatisExtranTime, mp, STATIS_DATA, 88, &tmpTime, 0)==FALSE)
       {
         	//���û�иò�����ͳ������,��Ӧ��ʼ��������ͳ��������Ϣ
       	  if (debugInfo&METER_DEBUG)
       	  {
            printf("�޲�����ͳ������(��ʱ���޹���),��ʼ��...\n");
          }
         	
         	initMpStatis(meterStatisExtranTime, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 { 
           //printf("�в�����ͳ������(��ʱ���޹���)\n");
         }
       }
       break;
     
     case 2:   //��ʱ���й���
       meterStatisBearonTime = (METER_STATIS_BEARON_TIME *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("���ҵ��ղ�����%dͳ������(��������):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
       
       if (readMeterData((INT8U *)meterStatisBearonTime, mp, STATIS_DATA, 0xfe, &tmpTime, 0)==FALSE)
       {
         	//���û�е��յĲ�����ͳ������,��Ӧ��ʼ��������ͳ��������Ϣ
         	initMpStatis(meterStatisBearonTime, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 { 
           printf("�е��ղ�����ͳ������(��������)\n");
         }
       }
       break;
     
     case 3:   //�������ʱ���޹���
       meterStatisExtranTimeS = (METER_STATIS_EXTRAN_TIME_S *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("���Ҳ�����%dͳ������(�������ʱ���޹���):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
  
       if (readMeterData((INT8U *)meterStatisExtranTimeS, mp, STATIS_DATA, 99, &tmpTime, 0)==FALSE)
       {
         	//���û�иò�����ͳ������,��Ӧ��ʼ��������ͳ��������Ϣ
       	  if (debugInfo&METER_DEBUG)
       	  { 
            printf("�޲�����ͳ������(�������ʱ���޹���),��ʼ��...\n");
          }
         	
         	initMpStatis(meterStatisExtranTimeS, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 {
           printf("�в�����ͳ������(�������ʱ���޹���)\n");
         }
       }
     	 break;
     	 
    #ifdef LIGHTING 
     case 4:    //��·��������ʱ���޹���
       kzqStatisExtranTime = (KZQ_STATIS_EXTRAN_TIME *)record;
       
       if (debugInfo&METER_DEBUG)
       {
         printf("������·������%dͳ������(��ʱ���޹���):%02x-%02x-%02x %02x:%02x:%02x\n",mp,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
  
       if (readMeterData((INT8U *)kzqStatisExtranTime, mp, STATIS_DATA, 89, &tmpTime, 0)==FALSE)
       {
         	//���û�и���·������ͳ������,��Ӧ��ʼ����·������ͳ��������Ϣ
       	  if (debugInfo&METER_DEBUG)
       	  {
            printf("����·������ͳ������(��ʱ���޹���),��ʼ��...\n");
          }
         	
         	initMpStatis(kzqStatisExtranTime, type);
       }
       else
       {
       	 if (debugInfo&METER_DEBUG)
       	 {
           printf("����·������ͳ������(��ʱ���޹���)\n");
         }
       }
       break;
    #endif

   }
}

/*******************************************************
��������:times2
��������:2��n�η�
���ú���:
�����ú���:
�������:
�������:
����ֵ:32λ����
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
��������:hexDivision
��������:����32bit�������,ʵ�ָ������ȵ�С��λ��
���ú���:
�����ú���:
�������:
�������:
����ֵ:32λBCD����(��deciBitλС��)
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
��������:detectSwitchPulse
��������:��⿪����/����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
      //������ǰ������ֵ
      stOfSwitchRead = ioctl(fdOfIoChannel, READ_YX_VALUE, 0);
 
      //�жϱ��ζ�ȡֵ
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
                    printf("%02d-%02d-%02d %02d:%02d:%02d,��=%d,΢��=%06d:ÿ����ĩ��������˿�%d�������һ��ʾֵ�����ʼ�����\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec,pulsei+1);
                  }
                  else
                  {
                    printf("%02d-%02d-%02d %02d:%02d:%02d,��=%d,΢��=%06d:ÿ��0�㱣������˿�%d�����һ��ʾֵ�����ʼ�����\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec,pulsei+1);
                  }
               }
               
               //���������ܱ����ݴ洢���Ƿ����
               checkSpRealTable(1);
               memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
               memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
               memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);

               //ת��
               covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
         		 	
         		 	 //����ʾֵ
         		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                            visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
         	   
         		 	 //�������
         		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                            paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);

         	     //��������
         		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                            reqBuff, PRESENT_DATA, REQ_REQTIME_DATA,LENGTH_OF_REQ_RECORD);
            }

            pulse[pulsei].pulseValue = stOfSwitchRead>>pulsei&0x1;            
            if (pulse[pulsei].pulseValue==0)  //�½���
            {
               if (pulse[pulsei].pulseValueBak==1 || pulse[pulsei].pulseValueBak2==1)
               {
             	   if(pulse[pulsei].findPulse == 0 && pulse[pulsei].prevMinutePulse==0)
             	   {
                   if (pulse[pulsei].pulseCount==0 
                   	 && (pulseDataBuff[53*pulsei] | pulseDataBuff[53*pulsei+1]<<8 | pulseDataBuff[53*pulsei+2]<<16)==0)
                   {
                     //���������ܱ����ݴ洢���Ƿ����
                     checkSpRealTable(1);
                     
                     //2012-07-31,add,��Ϊ��̨��������������յ������ϸ�
                     deletePresentData(pulse[pulsei].pn);

                     if (debugInfo&PRINT_PULSE_DEBUG)
                     {
                       printf("%02d-%02d-%02d %02d:%02d:%02d,��=%d,΢��=%06d:ɾ��ʵʱ���ݺ�,��������˿�%d��ʼʾֵ�����ʼ�����\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec, pulsei+1);
                     }

                     memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
                     memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
                     memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);

                     //ת��
                     covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
              		 	
              		 	 //����ʾֵ
              		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                                 visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
              	   
              		 	 //�������
              		 	 saveMeterData(pulse[pulsei].pn, 2, timeHexToBcd(sysTime), \
                                 paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);

              	     //��������
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
                   printf("%02d-%02d-%02d %02d:%02d:%02d,��=%d,΢��=%06d:��%d·�������=%04d,����=%d,pulseBak=%d,pulseBak2=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, tv.tv_sec, tv.tv_usec, pulsei+1, pulse[pulsei].pulseCount,pulse[pulsei].meterConstant,pulse[pulsei].pulseValueBak,pulse[pulsei].pulseValueBak2);
                 }
                 
             	   if (pulse[pulsei].pulseCount>=pulse[pulsei].meterConstant)
             	   {
             	   	 tmpPulseData = pulseDataBuff[53*pulsei] | pulseDataBuff[53*pulsei+1]<<8 | pulseDataBuff[53*pulsei+2]<<16;
             	   	 tmpPulseData++;
    	  	 	       pulseDataBuff[53*pulsei]   = tmpPulseData&0xff;
    	  	 	       pulseDataBuff[53*pulsei+1] = tmpPulseData>>8&0xff;
    	  	 	       pulseDataBuff[53*pulsei+2] = tmpPulseData>>16&0xff;
             	   	  
             	   	 //1 minute ƽ�������м���
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

       	   	  //����1minƽ������
       	   	  //pulse[pulsei].prevMinutePulse = pulse[pulsei].minutePulse + pulse[pulsei].pulseCount-pulse[pulsei].startPulse;
       	   	  pulse[pulsei].prevMinutePulse = pulse[pulsei].minutePulse;
       	   	  pulse[pulsei].minutePulse = 0;
       	   	  //pulse[pulsei].startPulse  = pulse[pulsei].pulseCount;
       	   	  pulse[pulsei].findPulse   = 0;
              
              //�����뵱ǰ���ʱȽ�
      	   	  tmpPulseData = pulseDataBuff[53*pulsei+5] | pulseDataBuff[53*pulsei+6]<<8;
              if (pulse[pulsei].prevMinutePulse>tmpPulseData)
              {
              	 tmpPulseData = pulse[pulsei].prevMinutePulse;
                 
                 if (debugInfo&PRINT_PULSE_DEBUG)
                 {
                   printf("%02d-%02d-%02d %02d:%02d:%02d:��%d·����=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pulsei+1, tmpPulseData);
                 }
              	 
              	 //�����������
              	 pulseDataBuff[53*pulsei+5] = tmpPulseData&0xff;
    	  	 	     pulseDataBuff[53*pulsei+6] = tmpPulseData>>8&0xff;
              	  
              	 //������������ʱ��
              	 pulseDataBuff[53*pulsei+7]  = sysTime.minute;
    	  	 	     pulseDataBuff[53*pulsei+8]  = sysTime.hour;
              	 pulseDataBuff[53*pulsei+9]  = sysTime.day;
    	  	 	     pulseDataBuff[53*pulsei+10] = sysTime.month;
    	  	 	     
    	  	 	     pulseBufSave = 1;
              }
              
              if (debugInfo&PRINT_PULSE_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d ��=%d,΢��=%06d:��%d·ƽ������=%d,�������=%d,����=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pulse[pulsei].calcTv.tv_sec, pulse[pulsei].calcTv.tv_usec, pulsei+1, pulse[pulsei].prevMinutePulse,pulse[pulsei].pulseCount,pulse[pulsei].meterConstant);
              }
       	    }
          }
          else
          {
         #endif    //PULSE_GATHER
        	
        	 //��������˸�·״̬��
           if ((statusInput[0] & tmpSwitch) != 0)
           {
             if ((statusInput[1]&tmpSwitch)==0)   //���ϴ���(���Ϊ���մ���)
             {
             	 if ((stOfSwitchRead&tmpSwitch)==0) //���IO��Ϊ0
             	 {
             	 	 stOfSwitchRead |= tmpSwitch;     //���ϴ���Ӧ��Ϊ1
             	 }
             	 else                               //���IO��Ϊ1
             	 {
             	 	 stOfSwitchRead &= ~tmpSwitch;    //���ϴ���Ӧ��Ϊ0
             	 }
             }
             else                                 //���ϴ���(���Ϊ��������)
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
      //���������ݻ���
      if (pulseBufSave && !(flagOfClearPulse==0x55 || flagOfClearPulse==0xaa))
      {
 	      saveParameter(88, 3, pulseDataBuff, NUM_OF_SWITCH_PULSE*53);
 	      
        if (debugInfo&PRINT_PULSE_DEBUG)
        {
          printf("�������������ݻ���\n");
        }
      }
    
      //�����屣��
      if (hasPulseSave && !(flagOfClearPulse==0x55 || flagOfClearPulse==0xaa))
      {
 	      //�����������
 	      saveParameter(88, 13, pulse, sizeof(ONE_PULSE)*NUM_OF_SWITCH_PULSE);
      }
     #endif
                	   
      stOfSwitchBaks[numOfStGather++] = stOfSwitchRead;

      //�жϱ�λ��־���¼�����
      //���ô����о��㷨,�ɼ�10�κ��ж����6��Ϊ1����1,����Ϊ0
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

          //���״̬��������λ,���¼��λ�¼�
          if ((stOfSwitch & tmpSwitch) != (stOfSwitchBak & tmpSwitch) && (statusInput[0] & tmpSwitch) != 0)
          {
            tmpCd |= tmpSwitch;

					 #ifdef LIGHTING
					 
					  //2018-08-20,����ң���б仯,�����ϴ��������ϴ�ң��״̬
					  nextHeartBeatTime = nextTime(sysTime, 0, 1);
					 
     				//2018-07-10,�������,���ּ��
						if (tmpSwitch==2)
					 	{
              if (selectParameter(4, 54, (INT8U*)&tmpXlcPn, 2)==FALSE)    //û�г������Ƶ�
              {
								if (selectParameter(4, 55, (INT8U*)&tmpXlcPn, 2)==TRUE)   //�в������Ƶ� 
								{
									if (debugInfo&METER_DEBUG)
									{
							 			printf("�������ź�,StOfSwitch=%02X,Pn=%d\n", stOfSwitchBak, tmpXlcPn);
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
        	 
        	 //2010-07-21,Ϊ��Ӧ��½����̨�ӵ�����ж�,����Ϊ��ȷ�Ĵ���Ӧ���ǲ�Ҫ���if�жϵ�,ly
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
          printf("detectSwitchPulse ���� �����ϱ�\n");
        }
 
        activeReport3();    //�����ϱ��¼�
        
        hasEvent = 0;
      }

      //��ʱ����,����ɼ���̫�������ó�CPUִ��ʱ��Ƭ,�����Ҫ�ӿ���ӳٲɼ��ٶ�,���ı���伴��
      usleep(500);
   }
}

/*******************************************************
��������:stStatusEvent
��������:״̬����λ�¼���¼
���ú���:
�����ú���:
�������:  stOfSwitch--����״̬��ֵ
           stOfSwitchBak--�ϴ�״̬��ֵ           
�������:
����ֵ:void
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
      
    event[8] = cdOfSwitch;          //״̬��λ
    event[9] = stOfSwitch;          //��λ���־
 	  if (eventRecordConfig.iEvent[0] & 0x08)
    {   
 	     writeEvent(event, 10, 1, reportToMain);  //������Ҫ�¼�����
 	  }
    if (eventRecordConfig.nEvent[0] & 0x08)
    {
 	     writeEvent(event, 10, 2, DATA_FROM_LOCAL);  //����һ���¼�����
 	  }    
}

/*******************************************************
��������:meterTimeOverEvent
��������:���ܱ�ʱ�䳬���жϲ��¼���¼
���ú���:
�����ú���:
�������:pn,�������
         timeBuff,�洢ʱ������ڵĻ���,0--3�ֽ�Ϊ���ں��ܴ�,4-6�ֽ�Ϊʱ��
         mixOfStatisRecord,ͳ�Ƽ�¼��mixed�ֽ�
�������:
����ֵ:void
*******************************************************/
BOOL meterTimeOverEvent(INT16U pn, INT8U *timeBuff, INT8U *mixOfStatisRecord, METER_STATIS_BEARON_TIME *statisBearon)
{
   INT8U     ifHasTimeEvent;
   INT8U     eventData[10];
   DATE_TIME tmpTime;
   
   if (timeBuff[0]!=0xEE && timeBuff[1]!=0xEE && timeBuff[2]!=0xEE      //����
  	  && timeBuff[4]!=0xEE && timeBuff[5]!=0xEE && timeBuff[6]!=0xEE)   //ʱ��
   {
     if ((eventRecordConfig.iEvent[1] & 0x08) || (eventRecordConfig.nEvent[1] & 0x08))
     {
       if (debugInfo&METER_DEBUG)
       {
         printf("������%d���ܱ�ʱ�䳬���־λ=%d\n",pn,*mixOfStatisRecord);
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
         printf("������%d���ʱ��Ϊ:%02d-%02d-%02d %02d:%02d:%02d\n",pn,tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
                            
       if ((timeCompare(tmpTime, sysTime, meterGate.meterCheckTimeGate) == TRUE) 
       	  || (timeCompare(sysTime, tmpTime, meterGate.meterCheckTimeGate) == TRUE))
       {
          if ((*mixOfStatisRecord&0x1)==0x1)
          {
            ifHasTimeEvent = 0x2;   //����ָ�
            
            if (debugInfo&METER_DEBUG)
            {
              printf("������%dʱ�䳬��ָ�\n",pn);
            }
            
            *mixOfStatisRecord &= 0xfe;
          }
       }
       else
       {
          if ((*mixOfStatisRecord&0x1)==0x0)
          {
            ifHasTimeEvent = 0x1;   //�����

            *mixOfStatisRecord &= 0xfe;
            *mixOfStatisRecord |= 0x01;

            if (debugInfo&METER_DEBUG)
            {
              printf("������%dʱ�䳬���\n",pn);
            }
          }
                    
         #ifdef CQDL_CSM 
          else
          {
          	//���췢��δ��¼
          	if ((statisBearon->mixed&0x1)==0x0)
          	{
              ifHasTimeEvent = 0x1;   //�����

              statisBearon->mixed |= 0x01;

              if (debugInfo&METER_DEBUG)
              {
                printf("������%dʱ�䳬�������δ��¼,���ڼ�¼\n",pn);
              }
          	}
          	else
          	{
              if (debugInfo&METER_DEBUG)
              {
                printf("������%dʱ�䳬��������Ѽ�¼\n",pn);
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
           
           eventData[8] = pn;     //�������8λ
           
           if (ifHasTimeEvent==0x1)
           {
             eventData[9] = ((pn>>8)&0xf) | 0x01<<7; //�������4λ����ֹ��־
           }
           else
           {
             eventData[9] = ((pn>>8)&0xf) & 0x7f;    //�������4λ����ֹ��־
           }
         
           if (eventRecordConfig.iEvent[1] & 0x08)
           {
             writeEvent(eventData, 10, 1, DATA_FROM_GPRS);
           }
           if (eventRecordConfig.nEvent[1] & 0x08)
           {
             writeEvent(eventData, 10, 2, DATA_FROM_LOCAL);
           }
           
           activeReport3();   //�����ϱ��¼�
           
           return TRUE;
       }
     }
   }
   
   return FALSE;
}

//***********************************��������********end*****************************************









//***********************************Q/GDW376.1���ҵ����ն�***************************************

#ifdef PLUG_IN_CARRIER_MODULE
 struct copyCtrlWord     copyCtrl[NUM_OF_COPY_METER+2];       //���������
 INT8U                   carrierModuleType=NO_CARRIER_MODULE; //�ز�ģ������
 INT8U                   localModuleType=NO_CARRIER_MODULE;   //��չ�ز�ģ������(ly,2012-02-28,Ϊ��ʶ���°���Ժģ�������,��Ϊ�°���ϰ�Ĵ�����ͬ����������������ɲ�ѯ)
 INT8U                   carrierAfn;                          //�ز�����AFN
 INT8U                   carrierFn;                           //�ز�����FN

 CARRIER_FLAG_SET        carrierFlagSet;                      //�ز���־��

 struct carrierMeterInfo *carrierSlaveNode,*prevSlaveNode;
 struct carrierMeterInfo *noRecordMeterHead;                  //���ֵ��δ��¼ָ��

 DOT_COPY                *pDotCopy=NULL;                      //ת����㳭ָ��
#else
 struct copyCtrlWord     copyCtrl[NUM_OF_COPY_METER];         //���������
#endif

INT8U                    numOfCopyPort = 0;                   //���м�������˿�

//˽�к�������
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

struct lightCtrl  lCtrl;    //��ر�־��

/***************************************************
��������:slcFrame
��������:������������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
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
  
  //2015-06-30,�޸�Ϊ���ֽڵĵ�4λ��ʾ����ʱ�κ�,��4λ�Ƿ�������
  tmpGroup = pCpNow->ctrlTime>>4;
  noOfTime = pCpNow->ctrlTime&0x0f;

 	retFrame[0] = 0x68;
 	memcpy(&retFrame[1], addr, 6);
 	retFrame[7] = 0x68;
	
 	retLen = 8;

  switch(type)
  {
  	case 1:    //�㲥Уʱ
 	    retFrame[retLen++] = 0x08;    //C
 	    retFrame[retLen++] = 0x06;    //L
 	    retFrame[retLen++] = hexToBcd(sysTime.second)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.minute)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.hour)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.day)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.month)+0x33;
 	    retFrame[retLen++] = hexToBcd(sysTime.year)+0x33;
 	    break;
 	  
 	  case 2:    //У���ƿ�����ʱ��(���У��)
 	    retFrame[retLen++] = 0x14;    //C
 	    
 	    //�����ֽں���������д
 	    retLen++;
 	     	    
 	    retFrame[retLen++] = 0x00+0x33;    //04010000
 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    
 	    retLen+=8;
 	    
 	    retLen++;    //��һ�ֽ���д�м���ʱ���

 	    tmpCTimes = cTimesHead;
 	    tmpNum = 0;    //ʱ��θ���
 	    while(tmpCTimes!=NULL)
 	    {
 	    	//���ƿ�����/��·�������Ŀ���ʱ�κ����������ͬ
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
   	      //2014-07-17,���ڹ㲥������ֽ�����,ֻ�ܷ���2��ʱ��
   	      if (tmpNum>=2)
   	      {
   	      	break;
   	      }
   	    }
        
        tmpCTimes = tmpCTimes->next;        
   	  }
   	  
 	    retFrame[22] = tmpNum+0x33;
 	    
      //CCB���ֵƹ��Ϻ����Դ���(0-3),2013-12-06,add
      retFrame[retLen++] = pnGate.failureRetry+0x33;
    
      //PWMƵ��
      retFrame[retLen++] = copyCtrl[4].tmpCpLink->bigAndLittleType+0x33;
    
      //������ռ�ձȶԱ�ֵ
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
 	  
 	  case 3:    //���״̬���ݰ�
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //0090FF00,��ȡCCB������ݰ�
 	    retFrame[retLen++] = 0xff+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;
 	    
 	  case 4:    //��ȡ���ƿ���������Сʱ��������
 	    retFrame[retLen++] = 0x11;          //C
 	    retLen++;                           //L
 	    retFrame[retLen++] = 0x00+0x33;     //0504FF00,��ȡ����Сʱ��������
 	    retFrame[retLen++] = 0xff+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    retFrame[retLen++] = 0x05+0x33;
 	    
 	    //2014-10-28,����ͬʱ�ɼ�6��Сʱ�����������185�ֽ�,��Ҫ����10�����ҵ�ʱ��,���ز����ڵ��ղ���,�ĳ����4��Сʱ������
 	    retLen++;     //Ҫ�ɼ������ݸ���(���4��)�ں�����д

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
        
        //ʮ��������
        if (readMeterData(tmpReadBuff, tmpMp, HOUR_FREEZE_SLC, 0x0, &tmpTime, 0) == TRUE)
        {
          if (tmpReadBuff[0] == 0xEE || tmpReadBuff[4] == 0xEE)
          {
          	if (debugInfo&PRINT_CARRIER_DEBUG)
          	{
          	  printf("���Ƶ�%d %d�����㶳������ȱʧ\n", copyCtrl[4].tmpCpLink->mp, tmpHour - i);
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
          	printf("���Ƶ�%d��%d�����㶳������\n", copyCtrl[4].tmpCpLink->mp, tmpHour - i);
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

 	  case 5:    //��������ǰ״̬��ʱ��
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    
 	    //2016-01-26,�޸ı��ֽ�Ϊ��ظ��ݼ�⵽���նȼ���ĵƵ�����
 	    //2016-02-18,���"|0x80",���żҸ۲��Է��ֵ�����ӳٹص�ʱ,��̨һ�ٲ�ƾ͹ص���,����������̨�·�������ֽ���0,���ǹصƵ���˼,�����Ҫ����
 	    retFrame[retLen++] = (lCtrl.lcDetectBright | 0x80) +0x33;     //00900000,��ȡ״̬��ʱ��

 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	  case 6:    //��������ǰ����ʱ��
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //00900100,��ȡ����ʱ��
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	  case 7:    //���ƿ��ص�ʱ�̺͵������
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //00900300,��ȡ���ص�ʱ�̺͵������
 	    retFrame[retLen++] = 0x03+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;
 	    
 	  case 8:    //У���ƿ�����ʱ��(����Уʱ��)
 	    retFrame[retLen++] = 0x14;    //C
 	    
 	    //�����ֽں���������д
 	    retLen++;
 	     	    
			if((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)==REQUEST_SYN_CTRL_TIME_1)
			{
				retFrame[retLen++] = 0x01+0x33;     //04010001,С��1
			}
 	    else if ((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2)==REQUEST_SYN_CTRL_TIME_2)
			{
			  retFrame[retLen++] = 0x03+0x33;     //04010003,С��2
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3)==REQUEST_SYN_CTRL_TIME_3)
			{
			  retFrame[retLen++] = 0x04+0x33;     //04010004,С��3
			}
			else 
			{
			  retFrame[retLen++] = 0x01+0x33;     //04010001,С��1
			}
 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    
 	    retLen+=8;
 	    
 	    retLen++;    //��һ�ֽ���д�м���ʱ���

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
				//��λ����7���13��ʱ���
				tmpNum = 0;
				while(tmpCTimes!=NULL)
				{
					//��·�������Ŀ���ʱ�κ����������ͬ
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
			
 	    tmpNum = 0;    //ʱ��θ���
 	    while(tmpCTimes!=NULL)
 	    {
 	    	//���ƿ�����/��·�������Ŀ���ʱ�κ����������ͬ
 	    	if (tmpCTimes->noOfTime==noOfTime 
 	    		  && 1==tmpCTimes->deviceType
 	    		 )
 	    	{
 	    	  retFrame[retLen++] = (tmpCTimes->startMonth | tmpCTimes->endMonth<<4)+0x33; 
 	    	  retFrame[retLen++] = tmpCTimes->startDay+0x33;
 	    	  retFrame[retLen++] = tmpCTimes->endDay+0x33;
           
					//2016-11-14,Add,����汾Ϊ2.0���ϵĵ��ƿ�������������ղ���
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
   	      //2014-07-17,���ڹ㲥������ֽ�����,ֻ�ܷ���2��ʱ��
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
				retFrame[retLen++] = ctrlMode+0x33;          //����ģʽ,2015-06-10,add
				retFrame[retLen++] = beforeOnOff[0]+0x33;    //��ؿ���ǰ���ٷ�����Ч,2015-06-10,add
				retFrame[retLen++] = beforeOnOff[1]+0x33;    //��ؿ��ƺ���ٷ�����Ч
				retFrame[retLen++] = beforeOnOff[2]+0x33;    //��عص�ǰ���ٷ�����Ч
				retFrame[retLen++] = beforeOnOff[3]+0x33;    //��عصƺ���ٷ�����Ч
				
				retFrame[retLen++] = tmpGroup+0x33;          //���
			}
			
 	    retFrame[9] = retLen-10;
 	    
 	    MD5Init(&context);
 	    MD5Update(&context, &retFrame[1], 13);
 	    MD5Update(&context, &retFrame[22], retFrame[9]-12);
      MD5Final (digest, &context);
      
      if (debugInfo&PRINT_CARRIER_DEBUG)
      { 
        printf("����Уʱ��PA=");
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
 	    
 	  case 9:    //У���Ƶ������(����У��)
 	    retFrame[retLen++] = 0x14;    //C
 	    
 	    //�����ֽں���������д
 	    retLen++;
 	     	    
 	    retFrame[retLen++] = 0x02+0x33;    //04010002
 	    retFrame[retLen++] = 0x00+0x33;
 	    retFrame[retLen++] = 0x01+0x33;
 	    retFrame[retLen++] = 0x04+0x33;
 	    
 	    retLen+=8;
 	    
      //CCB���ֵƹ��Ϻ����Դ���(0-3),2013-12-06,add
      retFrame[retLen++] = pnGate.failureRetry+0x33;
    
      //PWMƵ��
      retFrame[retLen++] = copyCtrl[4].tmpCpLink->bigAndLittleType+0x33;

      //��������
      retFrame[retLen++] = copyCtrl[4].tmpCpLink->startCurrent+0x33;
    
      //������ռ�ձȶԱ�ֵ
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
        printf("У�������PA=");
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
 	    
 	  case 10:    //��������ǰ����ʱ��С��1
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    if ((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_1)==0x0)
			{
			  retFrame[retLen++] = 0x01+0x33;     //00900101,��ȡ����ʱ��С��1
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_2)==0x0)
			{
			  retFrame[retLen++] = 0x02+0x33;     //00900102,��ȡ����ʱ��С��2
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_3)==0x0)
			{
			  retFrame[retLen++] = 0x03+0x33;     //00900103,��ȡ����ʱ��С��3
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_4)==0x0)
			{
			  retFrame[retLen++] = 0x04+0x33;     //00900104,��ȡ����ʱ��С��4
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_5)==0x0)
			{
			  retFrame[retLen++] = 0x05+0x33;     //00900105,��ȡ����ʱ��С��5
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_6)==0x0)
			{
			  retFrame[retLen++] = 0x06+0x33;     //00900106,��ȡ����ʱ��С��6
			}
			else if((pCpNow->flgOfAutoCopy&REQUEST_CTRL_TIME_7)==0x0)
			{
			  retFrame[retLen++] = 0x07+0x33;     //00900107,��ȡ����ʱ��С��7
			}
			
 	    retFrame[retLen++] = 0x04+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;

 	  case 11:    //��������ǰ����ʱ��С��2
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x02+0x33;     //00900101,��ȡ����ʱ��
 	    retFrame[retLen++] = 0x04+0x33;
 	    retFrame[retLen++] = 0x90+0x33;
 	    retFrame[retLen++] = 0x00+0x33;
 	    break;
 	  
 	  case 12:    //��������
      retFrame[retLen++] = 0x1c;
    	retFrame[retLen++] = 0x10;
    	
    	retLen += 8;
    	
      retFrame[retLen++] = lCtrl.bright+0x33;    //N1,�������
    	 
    	retFrame[retLen++] = ctrlMode+0x33;    //N2,����ģʽ
    	
    	//2015-06-16,�ݶ�Ϊһ��Сʱ�Ժ�
    	tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
    	 
    	//N3-N8,������Ч��ֹʱ��
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
			
 	  case 13:    //����������汾
 	    retFrame[retLen++] = 0x11;          //C
 	    retFrame[retLen++] = 0x04;          //L
 	    retFrame[retLen++] = 0x00+0x33;     //00900101,��ȡ����ʱ��
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
��������:xlcFrame
��������:��·����������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
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
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //һ����·���Ƶ�ͳ���¼�����(��ʱ���޹���)
  
	//��·���Ƶ���ʱ���޹���
	searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3);

 	xlcSendBuf[0] = 0x68;
 	memcpy(&xlcSendBuf[1], copyCtrl[port].cpLinkHead->addr, 6);
 	xlcSendBuf[7] = 0x68;

  if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CHECK_TIME)    //xlc��ҪУʱ
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
				 copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1        //xlc��Ҫͬ������ʱ�ε�1��10ʱ���
		      || copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2    //xlc��Ҫͬ������ʱ�ε�11��20ʱ���
			  )
		     && ((meterStatisExtranTimeS.mixed&0x20)==0x00)                          //����δͬ�����·�
			 )
    {
 	    xlcSendBuf[frameTail++] = 0x14;    //C
 	    
 	    //�����ֽں���������д
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
 	    
 	    frameTail++;    //��һ�ֽ���д�м���ʱ���
 	    
 	    tmpCTimes = cTimesHead;
      if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1)
			{
 	      ;
		  }
			else
			{
				//��λ����10��ʱ���
				tmpNum = 0;
				while(tmpCTimes!=NULL)
				{
					//��·�������Ŀ���ʱ�κ����������ͬ
					if (
							tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime 
							 && 
								 (
									(2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //����ģʽΪʱ�οػ�ʱ�οؽ�Ϲ��
									 || (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //����ģʽΪ��γ�ȿػ�γ�ȿؽ�Ϲ��
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
			
			
 	    tmpNum = 0;    //ʱ��θ���
 	    while(tmpCTimes!=NULL)
 	    {
 	    	//��·�������Ŀ���ʱ�κ����������ͬ
 	    	if (
 	    		  tmpCTimes->noOfTime==copyCtrl[port].cpLinkHead->ctrlTime 
 	    		   && 
 	    		     (
 	    		      (2==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType<=3)      //����ģʽΪʱ�οػ�ʱ�οؽ�Ϲ��
 	    		       || (5==tmpCTimes->deviceType && copyCtrl[port].cpLinkHead->bigAndLittleType>3)   //����ģʽΪ��γ�ȿػ�γ�ȿؽ�Ϲ��
 	    		     )
 	    		 )
 	    	{
 	    	  xlcSendBuf[frameTail++] = (tmpCTimes->startMonth | tmpCTimes->endMonth<<4)+0x33; 
 	    	  xlcSendBuf[frameTail++] = tmpCTimes->startDay+0x33;
 	    	  xlcSendBuf[frameTail++] = tmpCTimes->endDay+0x33;

 	    	  xlcSendBuf[frameTail++] = tmpCTimes->workDay+0x33;    //2016-8-19,Add,������

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
					
					//ÿ��ֻ��10��ʱ���,2016-11-02,����ᳬ��һ���ֽڵĳ���
					if (tmpNum==10)
					{
						break;
					}
        }
        
        tmpCTimes = tmpCTimes->next;
   	  }
 	    xlcSendBuf[22] = tmpNum+0x33;
 	    
 	    //����
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
					copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3    //xlc��Ҫͬ������ʱ���������
					 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)                     //����δͬ�����·�
				 )
			{
				xlcSendBuf[frameTail++] = 0x14;    //C
				
				//�����ֽں���������д
				frameTail++;
							
				xlcSendBuf[frameTail++] = 0x04+0x33;    //04010004
				xlcSendBuf[frameTail++] = 0x00+0x33;
				xlcSendBuf[frameTail++] = 0x01+0x33;
				xlcSendBuf[frameTail++] = 0x04+0x33;
				
				frameTail+=8;
				
				//����ң���Ƿ����
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->joinUp+0x33;

				//2015-06-11,��ӿ���ģʽ
				//2016-02-15,�޸�Ϊ���õı����Ƶ�Ŀ���ģʽ
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->bigAndLittleType+0x33;
				
				//2015-06-26,��ӹ����ǰ-�ӳ���Ч����
				xlcSendBuf[frameTail++] = beforeOnOff[0]+0x33;
				xlcSendBuf[frameTail++] = beforeOnOff[1]+0x33;
				xlcSendBuf[frameTail++] = beforeOnOff[2]+0x33;
				xlcSendBuf[frameTail++] = beforeOnOff[3]+0x33;
				
				//2016-02-15,Add,��γ��
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[0]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[1]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[2]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[3]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[4]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->collectorAddr[5]+0x33;

				//2016-03-17,Add,��γ�ȿ���ģʽʱ�ķ�/��բ΢��������
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->duty[4]+0x33;
				xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->duty[5]+0x33;
				
				//����
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
					 
					//2016-08-25,�޸�Ϊ���õı����Ƶ�Ŀ���ģʽ
					xlcSendBuf[frameTail++] = copyCtrl[port].cpLinkHead->bigAndLittleType+0x33;    //N2,����ģʽ
					
					//2015-06-12,�ݶ�Ϊһ��Сʱ�Ժ�
					tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
					 
					//N3-N8,������Ч��ֹʱ��
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
					if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)==0x0)    //��ѯ���ݰ���δ����
					{
						xlcSendBuf[frameTail++] = 0x11;    //C
						xlcSendBuf[frameTail++] = 0x04;    //L
						xlcSendBuf[frameTail]   = 0x31;    //0090FF00,��ȡ���״̬���ݰ�
						//2016-01-22,����ֽڸĳɹ��Ҫ���բ���Ǻ�բ��־
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
						//��ѯ����ʱ��С��1��δ����
						if (
								(copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CTRL_TIME_1)==0x0
									&& ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //����δͬ�����·�
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
							//��ѯ����ʱ��С��2��δ����
							if (
									(copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_CTRL_TIME_2)==0x0
									 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //����δͬ�����·�
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
								//xlc��Сʱ��������
								if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_HOUR_FREEZE)==0x0)
								{
									xlcSendBuf[frameTail++] = 0x11;         //C
									frameTail++;                            //L
									xlcSendBuf[frameTail++] = 0x00+0x33;    //0504FF00,��ȡ����Сʱ��������
									xlcSendBuf[frameTail++] = 0xff+0x33;
									xlcSendBuf[frameTail++] = 0x04+0x33;
									xlcSendBuf[frameTail++] = 0x05+0x33;
									
									//ͬʱ�ɼ�6��Сʱ�����������185�ֽ�
									frameTail++;     //Ҫ�ɼ������ݸ���(���6��)�ں�����д
						
									tmpNum = 0;
									tmpHour = sysTime.hour;
									for(i=0; i<=tmpHour; i++)
									{
										tmpTime = sysTime;
										tmpTime.second = 0x0;
										tmpTime.minute = 0x0;
										tmpTime.hour = tmpHour - i;
										
										//ʮ��������
										if (readMeterData(tmpReadBuff, copyCtrl[port].cpLinkHead->mp, HOUR_FREEZE_SLC, 0x0, &tmpTime, 0) == TRUE)
										{
											if (tmpReadBuff[0] == 0xEE || tmpReadBuff[4] == 0xEE)
											{
												if (debugInfo&METER_DEBUG)
												{
													printf("���Ƶ�%d %d�����㶳������ȱʧ\n", copyCtrl[port].cpLinkHead->mp, tmpHour - i);
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
												printf("���Ƶ�%d��%d�����㶳������\n", copyCtrl[port].cpLinkHead->mp, tmpHour - i);
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
									if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)    //��ѯ���ݰ��Ѿ�����
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
��������:xlcForwardFrame
��������:��·������ת������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
BOOL xlcForwardFrame(INT8U port, INT8U *addr, INT8U openClose)
{
  MD5_CTX   context;
  INT8U     digest[16];
	INT8U     frameTail = 8;
  DATE_TIME tmpTime;
  INT8U     i;

	printf("openClose=%d\n", openClose);
	
  //����ö˿ڵ�ǰδ����ת��
  if (copyCtrl[port].pForwardData==NULL)
  {
		copyCtrl[port].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
		copyCtrl[port].pForwardData->fn            = 1;
		copyCtrl[port].pForwardData->dataFrom      = DOT_CP_SINGLE_MP;
		copyCtrl[port].pForwardData->ifSend        = FALSE;
		copyCtrl[port].pForwardData->receivedBytes = FALSE;
		copyCtrl[port].pForwardData->forwardResult = RESULT_NONE;
 
		//͸��ת��ͨ�ſ�����
		copyCtrl[port].pForwardData->ctrlWord = DEFAULT_CTRL_WORD_2400;
		
		//͸��ת�����յȴ����ĳ�ʱʱ��,��λΪs
		copyCtrl[port].pForwardData->frameTimeOut = 10;
		
		//͸��ת�����յȴ��ֽڳ�ʱʱ��
		copyCtrl[port].pForwardData->byteTimeOut = 2;
		
		
		//͸��ת������
		copyCtrl[port].pForwardData->data[0] = 0x68;
 	  memcpy(&copyCtrl[port].pForwardData->data[1], addr, 6);
 	  copyCtrl[port].pForwardData->data[7] = 0x68;
		
		copyCtrl[port].pForwardData->data[frameTail++] = 0x1c;
		copyCtrl[port].pForwardData->data[frameTail++] = 0x10;
		
		frameTail+= 8;
		
		switch (openClose)
		{
			case 1:    //��բһСʱ
			  copyCtrl[port].pForwardData->data[frameTail++] = 0+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
				break;

			case 2:    //��բʮ����
			  copyCtrl[port].pForwardData->data[frameTail++] = 1+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 10, 0));
				break;

			case 3:    //��բһСʱ
			  copyCtrl[port].pForwardData->data[frameTail++] = 1+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 60, 0));
				break;

			case 4:    //�ָ��Զ�����
				copyCtrl[port].pForwardData->data[frameTail++] = 0+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 1, 0));
				break;
			
			default:   //��բʮ����
			  copyCtrl[port].pForwardData->data[frameTail++] = 0+0x33;    //N1
		    tmpTime = timeHexToBcd(nextTime(sysTime, 10, 0));
				break;
		}
		 
		copyCtrl[port].pForwardData->data[frameTail++] = 0x00+0x33;     //N2,����ģʽΪ��վֱ�ӿ�������
		 
		//N3-N8,������Ч��ֹʱ��
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

		//ת������
		copyCtrl[port].pForwardData->length = frameTail;
	}
	else
	{
		return FALSE;
	}
	
	return TRUE;
}

/***************************************************
��������:xlcAcFrame
��������:��·�������Ľ�������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
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

  if ((copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)==0x0)    //��ѯ���ݰ���δ����
  {
 	  xlcSendBuf[frameTail++] = 0x11;          //C
 	  xlcSendBuf[frameTail++] = 0x04;          //L
 	  xlcSendBuf[frameTail++] = 0x00+0x33;     //0090FF00,��ȡ���״̬���ݰ�
 	  xlcSendBuf[frameTail++] = 0xff+0x33;
 	  xlcSendBuf[frameTail++] = 0x90+0x33;
 	  xlcSendBuf[frameTail++] = 0x00+0x33;
  }
  else
  {
    if (copyCtrl[port].cpLinkHead->flgOfAutoCopy&REQUEST_STATUS)    //��ѯ�����Ѿ�����
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
��������:ldgmFrame
��������:��������������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
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
 	
  //flgOfAutoCopy-bit0-��״̬
  //              bit1-LDGM01У��·������������
  //              bit2-LDGM02Уĩ������ַ
  if (0x02==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x2))
  {
    ldgmSendBuf[tmpTail++] = 0x14;    //C
    
    //�����ֽں���������д
    tmpTail++;
     	    
    ldgmSendBuf[tmpTail++] = 0x00+0x33;    //04010000
    ldgmSendBuf[tmpTail++] = 0x00+0x33;
    ldgmSendBuf[tmpTail++] = 0x01+0x33;
    ldgmSendBuf[tmpTail++] = 0x04+0x33;
    
    tmpTail+=8;

    //��·��������ֵ
    ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->startCurrent+0x33;
    ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->ctrlTime+0x33;
    
    ldgmSendBuf[9] = tmpTail-10;
    
    MD5Init(&context);
    MD5Update(&context, &ldgmSendBuf[1], 13);
    MD5Update(&context, &ldgmSendBuf[22], ldgmSendBuf[9]-12);
    MD5Final (digest, &context);
    
    if (debugInfo&METER_DEBUG)
    { 
      printf("У������������·��������ֵ����PA=");
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
      
      //�����ֽں���������д
      tmpTail++;
       	    
      ldgmSendBuf[tmpTail++] = 0x00+0x33;    //04010000
      ldgmSendBuf[tmpTail++] = 0x00+0x33;
      ldgmSendBuf[tmpTail++] = 0x01+0x33;
      ldgmSendBuf[tmpTail++] = 0x04+0x33;
      
      tmpTail+=8;
  
      //��һĩ������ַ
      for(i=0; i<6; i++)
      {
        ldgmSendBuf[tmpTail++] = copyCtrl[port].cpLinkHead->lddt1st[i]+0x33;
      }

      //�ڶ�ĩ������ַ
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
        printf("УLDGM02����������ĩ������ַ,PA=");
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
        //2015-01-08,��ȡ��ʱֱ�������������֡
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
   	    ldgmSendBuf[tmpTail++] = 0x00+0x33;    //0090FF00,��ȡ������ݰ�
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
��������:lsFrame
��������:���նȴ���������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U lsFrame(INT8U port)
{
	INT8U  ret=0;
  INT8U  lsSendBuf[50];
  INT16U tmpData;
 	
  //flgOfAutoCopy-bit0-��ȡ���ն�ֵ
  if (0x0==(copyCtrl[port].cpLinkHead->flgOfAutoCopy&0x1))
  {
    //lsSendBuf[0] = copyCtrl[port].cpLinkHead->addr[0];

    if (copyCtrl[port].cpLinkHead->ctrlTime==2)    //2���նȴ�����,���ٷ���
    {
      lsSendBuf[0] = copyCtrl[port].cpLinkHead->addr[0];
      lsSendBuf[1] = 0x03;
      lsSendBuf[2] = 0x01;
      lsSendBuf[3] = 0x00;
      lsSendBuf[4] = 0x00;
      lsSendBuf[5] = 0x02;    	
    }
    else    //�ܻ�
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
��������:thFrame
��������:��ʪ�ȴ�����������֡
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U thFrame(INT8U port)
{
	INT8U  ret=0;
  INT8U  thSendBuf[8];
  INT16U tmpData;
 	
  //flgOfAutoCopy-bit0-��ȡ��ʪ��ֵ
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
��������:initXlcLink
��������:��ʼ����·����������
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U initXlcLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//�ͷ�����
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
��������:initAcLink
��������:��ʼ����·����������
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U initAcLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//�ͷ�����
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
��������:initLdgmLink
��������:��ʼ����������������
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U initLdgmLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//�ͷ�����
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
��������:initLsLink
��������:��ʼ���նȴ���������
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U initLsLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//�ͷ�����
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
��������:initThLink
��������:��ʼ����ʪ�ȴ���������
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
INT8U initThLink(void)
{
  struct cpAddrLink *tmpCpLink, *tmpPrevLink;
	
	//�ͷ�����
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
��������:copyMeter
��������:����(��)������ں���(���ն��漰�ĵ��ܱ�)
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
***************************************************/
void copyMeter(void *arg)
{
  INT32U                     tmpData;
  DATE_TIME                  tmpTime, tmpBackTime;
  INT8U                      port, tmpPort;           //��������ѭ�����Ƽ���
  INT8S                      ret;
  char                       strSave[100],*pSqlStr;
  struct cpAddrLink          *tmpNode;
  METER_DEVICE_CONFIG        meterConfig;
  METER_STATIS_EXTRAN_TIME   meterStatisExtranTime;   //һ��������ͳ���¼�����(��ʱ���޹���)
  METER_STATIS_BEARON_TIME   meterStatisBearonTime;   //һ��������ͳ���¼�����(��ʱ���й���)
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //һ�鵥����ͳ���¼�����(��ʱ���޹���)
  INT8U                      tmpReadBuff[LENGTH_OF_ENERGY_RECORD];
  INT32U                     tmpCount=0;
  INT8U                      singlePhaseData[512];    //��������ݻ���
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
      
  //�ز���ر���
 #ifdef PLUG_IN_CARRIER_MODULE 
  GDW376_2_INIT              gdw3762Init;
  INT8U                      ifFound;
  INT8U                      hasCollector;            //�Ƿ��вɼ���?
  struct carrierMeterInfo    *tmpSalveNode, *tmpQueryFound;
  INT8U                      meterInfo[10];
  INT8U                      tmpBuff[10];
  INT16U                     tmpNumOfSyn;             //�򱾵�ͨ��(�ز�/����)ģ���·��ĵ�ַ����
   
  INT8U                      carrierSendToMeter=0;    //�����ز�ģ�鷢�ͳ�������֡?2013-12-25
   
  #ifdef LIGHTING
    
   INT8U                     slSendBuf[250];          //streetLight���ͻ���
    
   TERMINAL_STATIS_RECORD    terminalStatisRecord;    //�ն�ͳ�Ƽ�¼

  #endif
 #endif
   
  //��ȡ���һ�βɼ�����,ȷ���ϴγ���ʱ�估�´γ���ʱ�� 
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
      
    //��ȡ���һ�βɼ�����,ȷ���ϴγ���ʱ��
    if (port==4)    //�ز��ӿ�
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
          printf("�˿�31(�ز��˿�)�ϴγ���ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n",copyCtrl[port].lastCopyTime.year,copyCtrl[port].lastCopyTime.month,copyCtrl[port].lastCopyTime.day,copyCtrl[port].lastCopyTime.hour,copyCtrl[port].lastCopyTime.minute,copyCtrl[port].lastCopyTime.second);
        }
        else
        {
          printf("�˿�%d�ϴγ���ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n",port+1,copyCtrl[port].lastCopyTime.year,copyCtrl[port].lastCopyTime.month,copyCtrl[port].lastCopyTime.day,copyCtrl[port].lastCopyTime.hour,copyCtrl[port].lastCopyTime.minute,copyCtrl[port].lastCopyTime.second);
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
          printf("�˿�31(�ز��˿�)��δ������\n");
        }
        else
        {
          printf("�˿�%d��δ������\n",port+1);
        }
      }
 
      tmpTime = sysTime;
    }

    if (ret>4)
    {
      if (debugInfo&METER_DEBUG)
      {	
      	printf("�˿�%d������:δ����",port+1);
      }
    }
    else
    {
      if (debugInfo&METER_DEBUG)
      {
        printf("������:%d\n",teCopyRunPara.para[ret].copyInterval);
      }
        
     #ifndef LM_SUPPORT_UT
      if (port==4)
      {
        //ly,2010-11-22,�ز��˿ڴ���һСʱ��1�ֿ���
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
      
        //�������һ�γ���ʱ�������һ�γ���ʱ��
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
        printf("�˿�31(�ز��˿�)�´γ���ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",
               copyCtrl[port].nextCopyTime.year,copyCtrl[port].nextCopyTime.month,
                copyCtrl[port].nextCopyTime.day,copyCtrl[port].nextCopyTime.hour,
                 copyCtrl[port].nextCopyTime.minute,copyCtrl[port].nextCopyTime.second);
      }
      else
      {
        printf("�˿�%d�´γ���ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", port+1,
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
   
  //��ʼ���ز�ģ���
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

    //1.��ȡ������������
   #ifdef AC_SAMPLE_DEVICE
      
    if (ifHasAcModule)
    {
      tmpCount++;
      if (readCheckData==0x55)   //��ȡ����У��ǰ����ֵ
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
                  
    //2.�����˿ڲ��г�����
    //ly,2012-01-09,�ĳ��������
    //for(port=0; port<NUM_OF_COPY_METER; port++)
    for(port=0; port<numOfCopyPort; port++)
    {
      //2-0.�����ն���
      if (copyCtrl[port].ifCopyDayFreeze == 0
  	 	    && ((teCopyRunPara.para[port].copyDay[(sysTime.day-1)/8] >> ((sysTime.day-1)%8)&0x01) == 1)
  	 	     && ((sysTime.minute%10|sysTime.minute/10<<4) == teCopyRunPara.para[port].copyTime[0]) 
  	 	      && ((sysTime.hour%10|sysTime.hour/10<<4) == teCopyRunPara.para[port].copyTime[1]))
      {
      	//2012-07-26,�ӳ�ִ�г����ն���
      	if (teCopyRunPara.para[port].copyTime[0]==0x0 && teCopyRunPara.para[port].copyTime[1]==0x0)
      	{
      	  copyCtrl[port].ifCopyDayFreeze = 5;
      	}
      	else
      	{
      	  copyCtrl[port].ifCopyDayFreeze = 1;
      	    
      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("�˿�%d�����ն����־��λ\n",port+1);
          }
        }
      }

			//2012-07-26,�ӳ�ִ�г����ն���
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
      	    printf("�˿�%d�ӳ�ִ�г����ն����־��λ\n",port+1);
          }
      	}
      }
			if (copyCtrl[port].ifCopyDayFreeze==1)
      {
      	if (debugInfo&METER_DEBUG)
      	{
      	  printf("�˿�%d��ʼ���г����ն���洢\n",port+1);
        }
     
      	copyDayFreeze(port);
      	copyCtrl[port].copyDayTimeReset = nextTime(sysTime, 1, 0);
        copyCtrl[port].ifCopyDayFreeze = 2;
           
      	if (debugInfo&METER_DEBUG)
      	{
      	  printf("�˿�%d�����ն���洢����\n",port+1);
        }
      }
      if (copyCtrl[port].ifCopyDayFreeze==2 && compareTwoTime(copyCtrl[port].copyDayTimeReset, sysTime)==TRUE)
      {
    	  copyCtrl[port].ifCopyDayFreeze = 0;

    	  if (debugInfo&METER_DEBUG)
    	  {
    	    printf("�˿�%d�����ն����־��λ\n",port+1);
        }
    	}
       	 
       	 
       	 
     	//2-1,ִ�й㲥Уʱ�������
     	//2013-11-22,add
     	if (port>0)
     	{
     	  //2-1.1
     	  if ((0x0==copyCtrl[port].ifCheckTime)
     	 	    && (teCopyRunPara.para[port].copyRunControl[0]&0x08)    //Ҫ���ն˶�ʱ�Ե��㲥Уʱ
     	 	     && ((0x00==teCopyRunPara.para[port].broadcastCheckTime[2])    //����Ϊÿ��Уʱ
     	 	          || (bcdToHex(teCopyRunPara.para[port].broadcastCheckTime[2])==sysTime.day)    //���õ�Уʱ��Ϊ����
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
      	    printf("�˿�%d���͹㲥Уʱ����\n",port+1);
          }
      	   
      	  copyCtrl[port].checkTimeReset = nextTime(sysTime, 1, 0);

          //����Уʱ����
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
      	    printf("�˿�%d�������ó�1200����Уʱ����\n",port+1);
          }
					
         #ifdef WDOG_USE_X_MEGA
          eventData[0] = port;                          //xMega�˿�
          eventData[1] = DEFAULT_SER_CTRL_WORD;         //����1200
          sendXmegaFrame(COPY_PORT_RATE_SET, eventData, 2);
         #endif
          sendMeterFrame(port, buf, 18);

      	  if (debugInfo&METER_DEBUG)
      	  {
      	    printf("�˿�%d�������ó�2400����Уʱ����\n",port+1);
          }
         #ifdef WDOG_USE_X_MEGA
          eventData[0] = port;                          //xMega�˿�
          eventData[1] = DEFAULT_CTRL_WORD_2400;        //����2400
          sendXmegaFrame(COPY_PORT_RATE_SET, eventData, 2);
         #endif
          sendMeterFrame(port, buf, 18);

          //�ָ�����
         #ifdef WDOG_USE_X_MEGA
          buf[0] = port;                                //xMega�˿�
          buf[1] = copyCtrl[port].backupCtrlWord;    //�ָ��˿�����
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
      	    printf("�˿�%dУʱ�����־��λ\n",port+1);
          }
        }
      }

            

      //2-2.�˿�31���汸���ն�������(ÿ��23:50��ʼ���浱�ձ����ն���)
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
 	 	  	   	  copyCtrl[port].ifCopyLastFreeze = 1;  //Ӧ�ô洢�����¶�������
 	 	  	    }
 	 	  	    else
 	 	  	    {
 	 	  	   	  copyCtrl[port].ifCopyLastFreeze = 0;  //�в���Ҫ�洢�����¶�������
 	 	  	    }

 	 	  	    //��ʼ�洢�����ն�������
 	 	  	    if (debugInfo&PRINT_CARRIER_DEBUG)
 	 	  	    { 
 	 	  	      printf("��ʼ�洢�����ն�������\n");
 	 	  	    }
 	 	  	  
 	 	  	    tmpNode = copyCtrl[port].cpLinkHead;
 	 	  	   
 	 	  	    tmpBackTime = tmpTime;
 	 	  	    tmpBackTime.hour   = 23;
 	 	  	    tmpBackTime.minute = 59;
 	 	  	    tmpBackTime.second = 59;
 	 	  	    while(tmpNode!=NULL)
 	 	  	    {
              //ly,2010-11-23,�������������˾Ҫ�󶳽�ֵ��Ӧ��ʵ���,���,07���汸���ն�������
              if (tmpNode->protocol!=DLT_645_2007)
              {
                //��ѯ������洢��Ϣ
   		          queryMeterStoreInfo(tmpNode->mp, meterInfo);
 
                tmpTime = timeHexToBcd(tmpBackTime);
                if (readMeterData(singlePhaseData, tmpNode->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0)==TRUE)
                {
                  if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
                  {
                   	saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[2], DAY_FREEZE_COPY_DATA, meterInfo[4] | meterInfo[5]<<8);
                   	    
                   	//���汸���¶�������
                   	if (copyCtrl[port].ifCopyLastFreeze==1)
                   	{
                   	  saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[3],MONTH_FREEZE_COPY_DATA, meterInfo[4] | meterInfo[5]<<8);
                   	}
                  }
                  else
                  {
                   	saveMeterData(tmpNode->mp, 31, tmpTime, singlePhaseData, meterInfo[2], 0, meterInfo[4] | meterInfo[5]<<8);
                   	    
               	    //���汸���¶�������
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
     	 	  	  printf("�����ն������ݴ洢���\n");
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
 	  	 	      printf("�洢�ն����־��λ\n");
 	  	 	    }
 	 	  	  }
 	 	    }
      }
     #endif   //PLUG_IN_CARRIER_MODULE
        
        
      	 
  	  //2-3.��������͡���ʱ��ת�������
  	  if (copyCtrl[port].meterCopying==FALSE)   //�ö˿ڵ�ǰδ����
  	  {
 	   #ifdef PLUG_IN_CARRIER_MODULE
     	   
 	  	  //2-3-1.1����ͨ��ģ��(�ز�)�˿�
 	  	 #ifdef LM_SUPPORT_UT
 	  	  if (port==4 && 0x55!=lmProtocol)
 	  	 #else
 	  	  if (port==4)
 	  	 #endif
 	  	  {
 	  	    //ly,2012-4-1,���������·�ɲ��ܲ����ز�ģ��
          if (upRtFlag!=0)
 	  	    {
 	  	 	    goto breakPoint;
 	  	    }
 	  	 	 
 	  	    if (carrierFlagSet.retryCmd==1 || carrierFlagSet.cmdContinue==1)
 	  	    {
        	  //2-3-1.1-1.��Ӳ����λ����
            if (carrierFlagSet.hardwareReest==0)
            {
        	    gdw3762Framing(INITIALIZE_3762, 1, NULL, NULL);
        	    carrierFlagSet.cmdType = CA_CMD_HARD_RESET;
              carrierFlagSet.hardwareReest = 1;
             
              //2010-1-13,Ϊ��μӵ�
              carrierFlagSet.delayTime    = nextTime(sysTime,  1, 5);
              carrierFlagSet.netOkTimeOut = nextTime(sysTime, 30, 0);

              //2010-04-30,SR��λ������ȴ�5��
              carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 5);

              goto breakPoint;
            }

        	  //2-3-1.1-2.�������֪����ʲôģ��
        	  if (carrierModuleType==NO_CARRIER_MODULE)
        	  {
        	    gdw3762Framing(QUERY_DATA_3762, 1, NULL, NULL);
        	    carrierFlagSet.cmdType = CA_CMD_READ_MODULE;

        	    goto breakPoint;
        	  }
        	 
        	  //2-3-1.1-3.����ģ�鲻ͬ�㴦��
         	  // 2-3-1.1-3-1.SRģ���������ʼ�����ȴ�5��,2011-04-28,�Ϻ�SR,add
            //    2012-07-21,��������ģ���ͼ�����°�SRWF-3E68ģ��,����ģ���ӽڵ�û�б䶯�����������������,
            //               ���Ƽ�ÿ���������������
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

        	  // 2-3-1.1-3-2.��Ѹ��ģ������ģ��ʱ��
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
            	 
        	  //2-3-1.1-4.�����Ҫǿ��ɾ���ز�ģ���ϵı��ַ��·��
        	  if (debugInfo & DELETE_LOCAL_MODULE)
        	  {
        	 	  switch (carrierModuleType)
        	 	  {
        	 	  	case RL_WIRELESS:  //���
        	 	  	case SC_WIRELESS:  //����
        	 	  	  memset(tmpBuff, 0x0, 6);
        	 	  	  gdw3762Framing(ROUTE_SET_3762, 2, NULL, tmpBuff);
        	 	  	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
        	 	  	  break;
        	 	  	
        	 	    case FC_WIRELESS:  //��Ѹ��,��֧�ֲ�������ʼ��,����������ʼ��
        	 	      gdw3762Framing(INITIALIZE_3762, 3, NULL, NULL);
        	 	  	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
        	 	  	  break;
        	 	    
        	 	    default:           //֧�ֲ�������ʼ����ģ��
        	 	      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
        	 	  	  carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;
        	 	  	  break;
        	 	  }

            	goto breakPoint;
            }
        	  	 
        	  //2-3-1.1-5.�㳭��ת��
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
          		    pDotFn129->ifProcessing = 1;         //���ڴ���
          		   
          		    goto breakPoint;
    	  	   	  }
    	  	    }
        	     
    	        if (copyCtrl[port].pForwardData!=NULL)
    	        {
                forwardData(port);
                goto breakPoint;
              }
    	      }

            //2-3-1.1-6. 2010-01-13,��Ӧ���ģ��,ģ���ϵ����ʱһ����
            //    if (carrierModuleType==RL_WIRELESS)
            //    2012-02-28,���Ժ�ز��°�3оƬ��·�ɰ�Ҳ�����Ҫ����
            if (carrierModuleType==RL_WIRELESS || localModuleType==CEPRI_CARRIER_3_CHIP)
            {
            	if (carrierFlagSet.hardwareReest==1 || carrierFlagSet.hardwareReest==2)
            	{
              	if (compareTwoTime(carrierFlagSet.delayTime, sysTime))
              	{
              	 	carrierFlagSet.hardwareReest = 3;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                 	  printf("�ӳ�һ����ʱ�䵽,��ʼ����ģ��\n");
                  }
                     
                  carrierFlagSet.delayTime = nextTime(sysTime, 1, 5);
              	}
              	 	
              	goto breakPoint;
            	}
            }
            	 
            //2-3-1.1-7. 2011-04-28,�Ϻ�SR,�������ڵ��ַ�����ڵ�Ḵλ��Ҫ�ȴ�5������
            if (carrierModuleType==SR_WIRELESS && carrierFlagSet.setMainNode==2)
            {
        	    if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
        	    {
        	 	    carrierFlagSet.setMainNode = 3;
        	 	 
        	 	    carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 15);
        	    }
        	 
        	    goto breakPoint;
            }
               
            //2-3-1.1-7-1.�ֵ�����ģ�����SR����ģ����ڲ����Ե��Զ��ϱ�����
            //    2011-04-28,�Ϻ�SR,����SR����ʦ��˼·,ȡ�������ϱ�����
            //if (carrierFlagSet.innerDebug == 0 && (carrierModuleType==HWWD_WIRELESS ||carrierModuleType==SR_WIRELESS))
            //{
            //	  gdw3762Framing(DEBUG_3762, 1, NULL, NULL);
            //	  goto breakPoint;
            //}

        	  //2-3-1.1-8.��ѯ�ز�ģ�����ڵ��ַ
        	  if (carrierFlagSet.mainNode==0)
        	  {
        	    //��������΢�ز�ģ��,��Ϊ·����������,ly,2011-07-07
        	    if (localCopyForm==0xaa)
        	    {
          	    //ly,2012-01-14,Ŀǰ�ж�������΢������֧��·����������
          	    if (carrierModuleType==EAST_SOFT_CARRIER
          	   	     || carrierModuleType==TC_CARRIER
          	   	       || carrierModuleType==MIA_CARRIER
          	   	   )
          	    {
            	    if (debugInfo&PRINT_CARRIER_DEBUG)
            	    {
            	    	 printf("*******************·������������ʽ*******************\n");
            	    }

                  carrierFlagSet.routeLeadCopy=1;
                 
                  //ly,2011-12-30,Add
                  if (carrierModuleType==TC_CARRIER)
                  {
                    carrierFlagSet.workStatus = 2;
                  }
          	    }
          	    else    //����ģ��Ϊ��������������
          	    {
          	   	  localCopyForm = 0x55;
          	   	 
            	    if (debugInfo&PRINT_CARRIER_DEBUG)
            	    {
            	    	printf("*******************ǿ��Ϊ����������������ʽ*******************\n");
            	    }
          	    }
          	  }
          	  else
          	  {
          	    if (debugInfo&PRINT_CARRIER_DEBUG)
          	    {
          	   	  printf("*******************����������������ʽ*******************\n");
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
               
            //2-3.1.1-9.���ñ���ͨ��ģ�����ڵ��ַ
            //    if (carrierFlagSet.setMainNode==1)
            //    2012-02-27,�°���Ժģ�鲻Ҫ���������ڵ��ַ
            if (carrierFlagSet.setMainNode==1 && localModuleType!=CEPRI_CARRIER_3_CHIP)
            {
              memcpy(carrierFramex, mainNodeAddr, 6);
            
      	      gdw3762Framing(CTRL_CMD_3762, 1, carrierFramex, NULL);
      	      carrierFlagSet.cmdType = CA_CMD_SET_MAIN_NODE;

              //ly,2011-04-28,�Ϻ�SR,add
              if (carrierModuleType==SR_WIRELESS)
              {
      	        carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 5);
      	      }
      	      goto breakPoint;
            }

            //2-3.1.1-10.������ģ�������ŵ���
            //    2012-07-20,SRWF-3E68��Ҫ�����ŵ�/�����ʶ����
            //    2012-08-13,��Ѷ��ģ��Ҳ��Ҫ�����ŵ���
            if (((carrierModuleType==SR_WIRELESS && localModuleType==SR_WF_3E68) || carrierModuleType==FC_WIRELESS)  && carrierFlagSet.setPanId==0)
            {
  		 	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	    {
  		 	 	  	  printf("�����ŵ���Ϊ:%d\n",(addrField.a2[0] | addrField.a2[1]<<8)%0x10);
  		 	 	    }
  		 	 	  
  		 	 	    if (carrierModuleType==FC_WIRELESS)
  		 	 	    {
                carrierFramex[0] = (addrField.a2[0] | addrField.a2[1]<<8)%10;   //�ŵ���
                carrierFramex[1] = 0x00;         //δ֪
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
                carrierFramex[0] = (addrField.a2[0] | addrField.a2[1]<<8)%0x10;   //�ŵ���
                carrierFramex[1] = 0x05;         //δ֪
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

         	  //2-3.1.1-11.��ʼ����ʱ�ù���ģʽ
        	  if (carrierFlagSet.startStopWork==1 && carrierModuleType!=RL_WIRELESS && carrierModuleType!=SC_WIRELESS)
        	  {
        	    switch(carrierModuleType)
        	    {
        	      case SR_WIRELESS:  //SR����ģ������ֵ�����ģ����Ϊ��������֤ģʽ
        	      case HWWD_WIRELESS:
        	        carrierFlagSet.startStopWork = 2;

        	        carrierFramex[0] = 0x03;   //����,��֤ģʽ
        	        carrierFramex[1] = 0x0;
        	        carrierFramex[2] = 0x0;
        	        gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
        	 	   		carrierFlagSet.cmdType = CA_CMD_SR_CHECK_NET;
        	        break;
        	     
        	      case MIA_CARRIER:            //����΢�ز�ģ�� ÿ���ϵ綼Ϊ"ֹͣ����",���÷���ͣ��
        	      case EAST_SOFT_CARRIER:      //�����ز�ģ�� ÿ���ϵ綼Ϊ"ֹͣ����",���÷���ͣ��
        	      case FC_WIRELESS:            //��Ѹ������ģ��
        	      case CEPRI_CARRIER:          //���Ժģ��,ly,2012-02-28,�ĵ�����,ԭ��default
        	        carrierFlagSet.startStopWork = 0;
        	        break;
        	     
        	      default:                     //�ز�ģ��ֹͣ��ǰ����
        	        carrierFlagSet.startStopWork = 2;
        	        gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
        	 	      carrierFlagSet.cmdType = CA_CMD_PAUSE_WORK;
        	        break;
        	    }
        	    goto breakPoint;
        	  }
            	 
            //2-3-1.1-12.��ѯ·������״̬(��κ�����������ģ�鲻֧�ֲ�ѯ·������״̬)
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

            //2-3-1.1-13.��ѯ�ӽڵ�����
            if (carrierFlagSet.querySlaveNum==1 && !(carrierModuleType==SR_WIRELESS || carrierModuleType==CEPRI_CARRIER))
            {
        	 	  gdw3762Framing(ROUTE_QUERY_3762, 1, NULL, NULL);
        	 	  carrierFlagSet.cmdType = CA_CMD_QUERY_SLAVE_NUM;

        	 	  goto breakPoint;
            }
            	 
            //2-3-1.1-14.��վ���ñ��ַ��ȴ�һ����ͬ����ģ��,�Ա���ַ��֡���͵�ʱ����һ��ͬ��
            //    2011-01-29,add
            if (carrierFlagSet.msSetAddr==1)
            {
            	if (compareTwoTime(carrierFlagSet.msSetWait, sysTime))
            	{
                carrierFlagSet.msSetAddr = 0;
                carrierFlagSet.synSlaveNode = 1;
              
                if (debugInfo&PRINT_CARRIER_DEBUG)
                {
             	    printf("��վ���ñ��ַ�ȴ�ʱ�䵽,��ʼͬ�����ַ��ģ��\n");
                }
              }
            }
            	 
        	  //2-3-1.1-15.����б����Ҫͬ�����ز�ģ��
        	  if (carrierFlagSet.synSlaveNode>0)
        	  {
     	        switch (carrierFlagSet.querySlaveNode)
     	        {
     	      	  case 0:	     //ͬ��ǰ�Ȳ�ѯ�ز��ӽڵ���Ϣ
      	      	  //if (carrierModuleType==RL_WIRELESS || carrierModuleType==SC_WIRELESS)
     	      	    //2012-07-20,ɣ���SRWF-3E68Ҳ�Ǵ�0��ʼ
     	      	    if (carrierModuleType==RL_WIRELESS || carrierModuleType==SC_WIRELESS || carrierModuleType==SR_WIRELESS)
     	      	    {
     	      	      carrierFlagSet.synMeterNo = 0;
     	      	    }
     	      	    else
     	      	    {
     	      	      carrierFlagSet.synMeterNo = 1;
     	      	    }
     	      	   
     	      	    carrierFlagSet.hasAddedMeter  = 0;   //û����ģ����ӱ��ַ
     	      	    carrierSlaveNode = NULL;
     	      	    prevSlaveNode = carrierSlaveNode;

     	      	    switch (carrierModuleType)
     	      	    {
     	      	   	
     	      	   	  case SR_WIRELESS:    //ɣ��
                      if (localModuleType==SR_WF_3E68)
                      {
     	      	          //SR_WF_3E68���ԱȶԵ�ַ,2012-07-21
     	      	          carrierFlagSet.querySlaveNode = 1;
                      }
                      else
                      {
                        //SR����ģ������ֵ�����ģ��ֱ�ӷ��Ͳ�ѯ�ز��ӽڵ�֡
     	      	          carrierFlagSet.querySlaveNode = 2;
     	      	        }
     	      	        break;
     	      	             	      	     
     	      	      case FC_WIRELESS:    //��Ѹ��
     	      	        gdw3762Framing(FC_NET_CMD_3762, 13, NULL, NULL);
     	      	        carrierFlagSet.cmdType = CA_CMD_UPDATE_ADDR;
     	      	        showInfo("������������");
     	      	        break;
     	      	     
     	      	      default:
     	      	        carrierFlagSet.querySlaveNode = 1; //���Է��Ͳ�ѯ�ز��ӽڵ�֡
     	      	        break;
     	      	    }
     	      	    goto breakPoint;
     	      	    break;
         	      	
     	      	  case 1:      //���Ͳ�ѯ�ز��ӽڵ�֡
        	        if (carrierModuleType==FC_WIRELESS)
        	        {
        	          gdw3762Framing(FC_QUERY_DATA_3762, 10, NULL, (INT8U *)&carrierFlagSet.synMeterNo);
        	        }
        	        else
        	        {
        	          gdw3762Framing(ROUTE_QUERY_3762, 2, NULL, (INT8U *)&carrierFlagSet.synMeterNo);
        	        }
        	        carrierFlagSet.cmdType = CA_CMD_QUERY_SLAVE_NODE;
        	        showInfo("�Ƚϱ���ͨ��ģ���ַ");
        	        goto breakPoint;
        	        break;
            	     
        	      case 2:      //�ز��ӽڵ��ѯ���,����ͬ��
        	        switch(carrierFlagSet.synSlaveNode)
        	        {
        	          case 1:  //�����������,�����Ҫ����������ʼ��
        	 	  	      checkCarrierMeter();     //����ز��������
        	 	  	      
	    	 	  	        //�������ز���Ŷ�����ͨ��ģ��(�ز�/����)�б��,��Ҫ����������ʼ��
	    	 	  	        if (copyCtrl[port].cpLinkHead==NULL && carrierFlagSet.numOfSalveNode>0 && carrierFlagSet.init==0)
	    	 	  	        {
	    	 	  	      	  carrierFlagSet.init = 2;
	    	 	  	      	  switch (carrierModuleType)
	    	 	  	      	  {
	    	 	  	      	    case RL_WIRELESS:  //�������ģ�鲻֧�ֲ�������ʼ��,ֻ֧��ɾ�����дӽڵ�
	    	 	  	      	    case SC_WIRELESS:  //����
		    	 	  	      	 	  memset(tmpBuff, 0x0, 6);
		    	 	  	      	 	  gdw3762Framing(ROUTE_SET_3762, 2, NULL, tmpBuff);
		    	 	  	      	 	  break;
	    	 	  	      	 	 
	    	 	  	      	    case FC_WIRELESS:  //��Ѹ��ģ��
	    	 	  	      	      gdw3762Framing(INITIALIZE_3762, 3, NULL, NULL);
	    	 	  	      	 	    break;
	    	 	  	      	 	 
	    	 	  	      	    default:           //����֧�ֲ�������ʼ����ģ��
	    	 	  	      	      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
	    	 	  	      	      break;
	    	 	  	      	  }
	               	      carrierFlagSet.cmdType = CA_CMD_CLEAR_PARA;

						 					  goto breakPoint;
	    	 	  	        }
            	 	  	      
        	 	  	      if (copyCtrl[port].cpLinkHead!=NULL)
        	 	  	      {
        	 	  	      	//ly,2011-01-30,�Ƚ���û���ڼ����������в����ڵı���ͨ��ģ���еı��,
        	 	  	      	//              ���в����ڵı��,ɾ�����б���ģ���еı��
        	 	  	      	ifFound = 0;

                        //2012-07-20,SR��SRWF-3E68���ԱȽ�,�ϰ�SR�޷��Ƚ�
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
              	 	  	      	//2014-09-19,·�ƿ��������ز����Ƶ�Ĳɼ�����ַ��ʾ����ռ�ձ���,���Բ���Ҳ���ñȽϲɼ�����ַ
              	 	  	       #ifndef LIGHTING 
              	 	  	      	//�����б��������вɼ�����ַ
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
        	 	  	      	 	    else   //�����б��������Ǳ��ַ
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
    	 	  	      	 	  	 
    	 	  	      	 	  	    //������������ģ���б����ַ��ͬ
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
    	 	  	      	 	   	  	  printf("����ͨ��ģ���б��ַ%02x%02x%02x%02x%02x%02x�ڼ����������в�����\n",
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
        	 	  	      	 	  printf("���ģ�������б��\n");
        	 	  	      	  }
            	 	  	      	 	 
        	 	  	      	  switch (carrierModuleType)
        	 	  	      	  {
        	 	  	      	    case RL_WIRELESS:  //�������ģ�鲻֧�ֲ�������ʼ��,ֻ֧��ɾ�����дӽڵ�
        	 	  	      	    case SC_WIRELESS:  //����
        	 	  	      	 	    memset(tmpBuff, 0x0, 6);
        	 	  	      	 	    gdw3762Framing(ROUTE_SET_3762, 2, NULL, tmpBuff);
        	 	  	      	 	    break;

        	 	  	      	 	  case FC_WIRELESS:  //��Ѹ������ģ��
        	 	  	      	      gdw3762Framing(INITIALIZE_3762, 3, NULL, NULL);
        	 	  	      	      break;
        	 	  	      	 	   
        	 	  	      	 	  default:           //����֧�ֲ�������ʼ����ģ��
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
    	 	  	      	 	    //���������ģ��,��ѯ�Ƿ���ڼ������еı��ģ���ϲ�����,�������������Ҫ���������
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
        	 	  	      	        //�����б��������вɼ�����ַ
        	 	  	      	        if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, NULL, 1)==FALSE)
        	 	  	      	        {
        	 	  	      	          if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, prevSlaveNode->addr, 0)==TRUE)
        	 	  	      	          {
        	 	  	      	    	      ifFound = 1;
        	 	  	      	          }
      	 	  	      	 	  	    }
      	 	  	      	 	  	    else   //�����б��������Ǳ��ַ
      	 	  	      	 	  	    {
        	 	  	      	          if (compareTwoAddr(copyCtrl[port].tmpCpLink->addr, prevSlaveNode->addr, 0)==TRUE)
        	 	  	      	          {
        	 	  	      	    	      ifFound = 2;
        	 	  	      	          }
      	 	  	      	 	  	    }
      	 	  	      	 	  	 
      	 	  	      	 	  	    //������������ģ���б����ַ��ͬ
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
          	 	  	      	 	   	  printf("�������б��ַ%02x%02x%02x%02x%02x%02x�ڱ���ͨ��ģ���в�����\n",
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
                	 	  	      	printf("���ģ�������б��\n");
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
        	 	  	      	carrierFlagSet.synSlaveNode   = 0;    //�޲�������Ϣ,����ͬ��
        	 	  	      	carrierFlagSet.querySlaveNode = 0;
        	 	  	      	 
        	 	  	      	if (debugInfo&PRINT_CARRIER_DEBUG)
        	 	  	      	{
        	 	  	      	  printf("û�в�����Ҫͬ��\n");
        	 	  	      	}
            	 	  	  }
            	 	  	  break;
            	 	  	      
            	 	  	case 2:  //��ʼͬ�����
    	 	  	    	    if (copyCtrl[port].tmpCpLink!=NULL)
    	 	  	    	    {
    	 	  	      	    while(copyCtrl[port].tmpCpLink!=NULL)
    	 	  	      	    {
    	 	  	      	      prevSlaveNode = carrierSlaveNode;
    	 	  	      	      ifFound = 0;
    	 	  	      	      hasCollector = 0;
    	 	  	      	    
    	 	  	      	      //ly,2011-07-19,ȥ����SR�жϵĵ�������
    	 	  	      	      //SR����ģ�鲻�ܲ�ѯ�ӽڵ�ڵ�,ÿ�ζ���ģ�����,���Լ�ȥ������û�������ַ
    	 	  	      	      //if (carrierModuleType==SR_WIRELESS)
    	 	  	      	      //{
    	 	  	      	      //	 if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE)
    	 	  	      	      //	 {
    	 	  	      	      //	 	 hasCollector = 1;  //�вɼ�����ַ,�Ӳɼ�����ַ
    	 	  	      	      //	 }
    	 	  	      	      //}
    	 	  	      	      //else   //����ģ���ճ�����
    	 	  	      	      //{
    	 	  	      	    
    	 	  	      	      //2014-09-05,add,·�ƿ��������ز����Ƶ�Ĳɼ�����ַ��ʾ����ռ�ձ���,���Բ���Ҳ���ñȽϲɼ�����ַ
    	 	  	      	     #ifndef LIGHTING
    	 	  	      	      //�вɼ�����ַ,�Ӳɼ�����ַ
    	 	  	      	      if (compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE)
      	 	  	      	    {
      	 	  	      	      hasCollector = 1;
      	 	  	      	    }
      	 	  	      	   #endif
      	 	  	      	    
		  	 	  	      	    //�����Ƿ�ģ�����б���ַ
		  	 	  	      	    while(prevSlaveNode!=NULL)
		  	 	  	      	    {
		  	 	  	      	      if (hasCollector==1)
		  	 	  	      	      {
			 	  	      	          if (compareTwoAddr(prevSlaveNode->addr, copyCtrl[port].tmpCpLink->collectorAddr, 0)==TRUE)
		  	 	  	      	        {
		  	 	  	      	    	    ifFound = 1;
		  	 	  	      	        }
		  	 	  	      	      }
		  	 	  	      	      else   //û�вɼ�����ַ�����ز���
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
      	 	  	      	    		  printf("���Ժģ��:ģ�����б��ַ,���뼯�����еĵ�ַ��һ��,ɾ���������ݺ�,������ӵ�ַ\n");
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
    	 	  	      	            printf("��Ӳɼ�����ַ\n");
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
    	 	  	        	        carrierFlagSet.hasAddedMeter = 1;    //�����ز�ģ����ӱ��ַ
            	 	  	      	         
                              tmpSalveNode = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                              memcpy(tmpSalveNode->addr,copyCtrl[port].tmpCpLink->collectorAddr,6); //�ӽڵ��ַ
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
    	 	  	      	            printf("����ز�/���߱��ַ\n");
    	 	  	      	          }
    	 	  	      	         
    	 	  	      	          tmpNumOfSyn++;
    	 	  	      	         
    	 	  	      	          tmpBuff[0] = carrierFlagSet.synMeterNo&0xff;
    	 	  	      	          tmpBuff[1] = carrierFlagSet.synMeterNo>>8&0xff;
    	 	  	      	          if (copyCtrl[port].tmpCpLink->protocol==1)
    	 	  	      	          {
    	 	  	      	            tmpBuff[2] = 1;    //97��Լ
    	 	  	      	          }
    	 	  	      	          else
    	 	  	      	          {
    	 	  	      	            tmpBuff[2] = 2;    //07��Լ
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
    	 	  	      	          carrierFlagSet.hasAddedMeter = 1;    //�����ز�ģ����ӱ��ַ
    	 	  	      	        }
                            carrierFlagSet.studyRouting   = 0;
                            //carrierFlagSet.routeRunStatus = 1;
            	 	  	      	 
            	 	  	      	showInfo("ͬ������ͨ��ģ���ַ");

            	 	  	      	goto breakPoint;
            	 	  	      }
            	 	  	      	    
            	 	  	      copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
            	 	  	    }
            	 	  	  }
            	 	  	  else
            	 	  	  {
            	 	  	    showInfo("ͬ����ַ���");
	 	  	    		        if (debugInfo&PRINT_CARRIER_DEBUG)
	 	  	    		        {
	 	  	    		          printf("�ز�/���ߴӽڵ�ͬ�����,ģ���нڵ���Ϊ%d\n", carrierFlagSet.synMeterNo-1);
	 	  	    		        }

                    	  //���·����������ʽ,���б��ַ����
                        if (carrierFlagSet.routeLeadCopy==1 && carrierFlagSet.synMeterNo>1)
                        {
                       	  copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 5);
                       	  
                       	  if (debugInfo&PRINT_CARRIER_DEBUG)
                       	  {
                       	 	  printf("·����������:5���ʼ\n");
                       	  }
                        }

    	 	  	    		    if (tmpNumOfSyn>0 && !(carrierModuleType==SR_WIRELESS || carrierModuleType==FC_WIRELESS || localModuleType==CEPRI_CARRIER_3_CHIP /*2012-02-28,add���Ժ�ж�*/))
    	 	  	    		    {
    	 	  	    		      carrierFlagSet.routeRunStatus = 1;
    	 	  	    		    }

    	 	  	    		    carrierFlagSet.synSlaveNode   = 0;
        	 	  	      	carrierFlagSet.querySlaveNode = 0;
        	 	  	      	carrierFlagSet.chkNodeBeforeCopy = 2;    //�Ѿ��ȶԹ����ַ��ʶ,2013-12-30,add

    	 	  	    		    switch (carrierModuleType)
    	 	  	    		    {
    	 	  	    		      case SR_WIRELESS:    //SR����ģ��,�����ʱ��Ϊ3����
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
     	  		 	 	  		  	     
     	  		 	 	  	      case CEPRI_CARRIER:    //����ģ����ӱ��ַ/�ɼ�����ַ,��Ҫ��ʼ�ѱ�,ly,10-08-27
                            if (carrierFlagSet.hasAddedMeter==1)
                            {
                              //����ʱ���ݶ�Ϊ50���� ly,2011-06-07
                              //carrierFlagSet.foundStudyTime = nextTime(sysTime, 50, 0);
                              //����ʱ���ݶ�Ϊ120���� ly,2011-07-18
                              if (localModuleType!=CEPRI_CARRIER_3_CHIP)
                              {
                                carrierFlagSet.foundStudyTime = nextTime(sysTime, 120, 0);
                              }
                              else
                              {
                                //�°汾����ʱ�䶨Ϊ240����(4Сʱ) ly,2012-02-28
                                carrierFlagSet.foundStudyTime = nextTime(sysTime, 240, 0);
                              }
          	 	  	      	   
          	 	  	            carrierFlagSet.searchMeter = 1;
          	 	  	      	   
          	 	  	            printf("���Ժģ��׼���ѱ�\n");
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
    	 	  	      	 	  	    printf("�ȴ������Ӳ�ѯ����״̬,�ȴ���������������ʱ����:%d-%d-%d %d:%d:%d\n",carrierFlagSet.netOkTimeOut.year,carrierFlagSet.netOkTimeOut.month,carrierFlagSet.netOkTimeOut.day,carrierFlagSet.netOkTimeOut.hour,carrierFlagSet.netOkTimeOut.minute,carrierFlagSet.netOkTimeOut.second);
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
		 	 	  		  	       
		 	 	  		  	          //2014-02-28,��30���Ӹĳ����ѱ��ʱ�����ǿ����������
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

            //2-3-1.1-16.�ȴ�����ģ������
            if (carrierFlagSet.wlNetOk<3)
            {
            	switch(carrierModuleType)
            	{
            	  case RL_WIRELESS:    //���ģ���ϵ��ѯ����״̬,�ȴ�3*2Min
              	 	if (compareTwoTime(carrierFlagSet.netOkTimeOut, sysTime))
              	 	{
          	 	 	 	  //��δ�����ڵ��
          	 	 	 	  gdw3762Framing(RL_EXTEND_3762, 4, NULL, NULL);
  
                    usleep(100000);
  
          	 	 	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
          	 	 	 	  {
          	 	 	 	 	  printf("�ȴ�����ʱ�䳬��(�ڵ���*20+120)s,����HDLCֹͣ��������\n");
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
              	 	 
              	case SR_WIRELESS:    //SRģ���ʼ����������ȴ���δ�����ڵ�Ĳ�ѯ,ly,2011-04-28,�Ϻ�SR,add
                  if (compareTwoTime(carrierFlagSet.netOkTimeOut, sysTime))
                  {
   	  		 	 	  	  if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  {
  		 	 	  		      if (localModuleType==SR_WF_3E68)
  		 	 	  		      {
  		 	 	  		        printf("SR - %d���������ڵ������ޱ仯,������������δ�����ڵ�\n", SRWF_NET_OK_TIME_OUT);
  		 	 	  		      }
  		 	 	  		      else
  		 	 	  		      {
  		 	 	  		        printf("SR - 3���������ڵ������ޱ仯,������������δ�����ڵ�\n");
  		 	 	  		      }
   	  		 	 	  	  }
       	  		 	 	  	 
                    carrierFlagSet.wlNetOk = 3;
                       
                    //�����ǰ��δ�����ڵ���Ϣ
                    while (noFoundMeterHead!=NULL)
                    {
                      tmpFound = noFoundMeterHead;
                      noFoundMeterHead = noFoundMeterHead->next;
                           
                      free(tmpFound);
                    }
                             
                	  //��δ�����ڵ�
                	  gdw3762Framing(RL_EXTEND_3762, 4, NULL, NULL);
                    carrierFlagSet.cmdType = CA_CMD_READ_NO_NET_NODE;
    
                	  goto breakPoint;
                  }
                     
                	//��δ�����ڵ��
                  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
                  {
                   	carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
                    carrierFlagSet.routeRunStatus = 1;
                  }
                  break;
                   
                case FC_WIRELESS:
                	//��CAC״̬��Ϣ
                  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
                  {
                   	carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 20);
                    carrierFlagSet.routeRunStatus = 1;

   	  		 	 	  	  if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  {
        	 	  	      printf("��Ѹ�� - �ȴ���������������ʱ����:%d-%d-%d %d:%d:%d\n", carrierFlagSet.netOkTimeOut.year,carrierFlagSet.netOkTimeOut.month,carrierFlagSet.netOkTimeOut.day,carrierFlagSet.netOkTimeOut.hour,carrierFlagSet.netOkTimeOut.minute,carrierFlagSet.netOkTimeOut.second);
            	 	  	}
                  }
                   	 

                  if (compareTwoTime(carrierFlagSet.netOkTimeOut, sysTime))
                  {
   	  		 	 	  	  showInfo("ǿ����������");
   	  		 	 	  	 
   	  		 	 	  	  if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  {
   	  		 	 	  		  printf("��Ѷ�� - ��ʱ������δ���,ǿ�ƽ�������\n");
   	  		 	 	  	  }
       	  		 	 	  	 
                    carrierFlagSet.wlNetOk = 3;
                       
                    //�����ǰ��δ�����ڵ���Ϣ
                    while (noFoundMeterHead!=NULL)
                    {
                      tmpFound = noFoundMeterHead;
                      noFoundMeterHead = noFoundMeterHead->next;
                     
                      free(tmpFound);
                    }
                  }
             	    break;
                   
                default:    //�����ز�ģ������Ҫ�ȴ�����
                  carrierFlagSet.wlNetOk = 3;
                  break;
              }
            	 	 
            	goto breakPoint;
            }

   	 	      //2-3-1.1-17. �ֶ��ѱ������ѷ���,������·��ѧϰ״̬,��ֹͣ·��ѧϰ
   	 	      if (carrierFlagSet.searchMeter==1 && carrierFlagSet.studyRouting==2)
   	 	      {
    	 	      //ֹͣѧϰ����
    	 	 	    gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);

    	 	 	    carrierFlagSet.studyRouting = 3;  //����ֹͣѧϰ
    	 	 	
    	 	 	    goto breakPoint;
   	 	      }

            //2-3-1.1-18.ѧϰ·��
            switch (carrierFlagSet.studyRouting)
            {
        	 	  case 1:   //��ʼѧϰ·��
        	 	 	  //2012-08-13,add,����ģ�鲻��Ҫѧϰ
        	 	 	  if (carrierModuleType==SC_WIRELESS
        	 	 	 	    || carrierModuleType==FC_WIRELESS
        	 	 	 	     || carrierModuleType==SR_WIRELESS
        	 	 	 	   )
        	 	 	  {
        	 	 	    carrierFlagSet.studyRouting = 0;
        	 	 	    break;
        	 	 	  }
        	 	 	 
        	 	 	  carrierFramex[0] = 0x1;   //����ģʽ(������ע��,����״̬Ϊѧϰ)
        	 	 	  carrierFramex[1] = 0x0;
        	 	 	  carrierFramex[2] = 0x0;
        	 	 	 
        	 	    gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
        	 	    goto breakPoint;
        	 	    break;
            	 	 
        	 	  case 2:   //·��ѧϰ��,��ѯѧϰ״̬
        	 	 	  if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
        	 	 	  {
        	 	 	    carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
        	 	 	    carrierFlagSet.routeRunStatus = 1;
        	 	 	    goto breakPoint;
        	 	 	  }
        	 	 	  if (compareTwoTime(carrierFlagSet.foundStudyTime, sysTime))
        	 	 	  {            	 	 		 
        	 	 	    //ֹͣѧϰ����
        	 	 	    gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);

        	 	 	    carrierFlagSet.studyRouting = 3;  //����ֹͣѧϰ
        	 	 		 
        	 	 	    goto breakPoint;
        	 	 	  }
        	 	 	  break;
            	 	 	 
        	 	  case 3:
        	 	 	  //2012-08-13,add����������ģ�鲻��Ҫѧϰ,Ҳ�Ͳ���Ҫѧϰ��
        	 	 	  if (carrierModuleType==SC_WIRELESS
        	 	 	 	    || carrierModuleType==FC_WIRELESS
        	 	 	 	     || carrierModuleType==SR_WIRELESS
        	 	 	 	   )
        	 	 	  {
        	 	 	    carrierFlagSet.studyRouting = 0;
        	 	 	    break;
        	 	 	  }

        	 	 	  //ֹͣѧϰ����
        	 	 	  gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
        	 	 	  goto breakPoint;
        	 	 	  break;
            }
            if (carrierFlagSet.studyRouting>0)
            {
            	goto breakPoint;
            }
            	 
            //2-3-1.1-19.�ѱ�
            switch (carrierFlagSet.searchMeter)
            {
        	 	  case 1:
		 	 	        //if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
		 	 	        //2012-08-13,add,��Ѷ��Ҳ����Ҫ�ѱ�
				 	 	    if (carrierModuleType==SR_WIRELESS 
				 	 	  	    || carrierModuleType==HWWD_WIRELESS 
				 	 	  	     || carrierModuleType==RL_WIRELESS
				 	 	  	      || carrierModuleType==FC_WIRELESS
				 	 	  	   )
				 	 	    {
				 	 		    carrierFlagSet.searchMeter = 0;
				 	 		    break;
				 	 	    }
    	  		
	    	  		  //ly,2012-02-28,���,�°汾���Ժģ�鲻�����ù���ģʽ
	    	  		  if (carrierModuleType==CEPRI_CARRIER && localModuleType==CEPRI_CARRIER_3_CHIP)
	    	  		  {
	    	  		    carrierFlagSet.searchMeter = 2;
	    	  		    goto breakPoint;
	    	  		  }
	    	  		
	    	  		  if (menuInLayer==0)
	    	  		  {
	    	  		    defaultMenu();
	    	  		  }

	  	 	 	      carrierFramex[0] = 0x2;                        //����ģʽ(����ע��,����״̬Ϊ����)
	  	 	 	      carrierFramex[1] = 0x0;
	  	 	 	      carrierFramex[2] = 0x0;
          	 	 	 
          	 	  gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
          	 	  carrierFlagSet.cmdType = CA_CMD_SET_WORK_MODE;
            	 	break;
            	 	  	
            	case 2:
        	 	    searchStart = sysTime;                         //������ʼʱ��
        	 	    carrierFramex[0] = hexToBcd(sysTime.second);   //��ʼʱ��
        	 	    carrierFramex[1] = hexToBcd(sysTime.minute);
        	 	    carrierFramex[2] = hexToBcd(sysTime.hour);
        	 	    carrierFramex[3] = hexToBcd(sysTime.day);
        	 	    carrierFramex[4] = hexToBcd(sysTime.month);
        	 	    carrierFramex[5] = hexToBcd(sysTime.year);
        	 	     
        	 	    carrierFramex[6] = 0;                          //����ʱ��1
        	 	    carrierFramex[7] = 5;                          //����ʱ��2
        	 	    carrierFramex[8] = 1;                          //�ӽڵ��ط�����
        	 	    carrierFramex[9] = 1;                          //����ȴ�ʱ��Ƭ����

        	 	    gdw3762Framing(ROUTE_SET_3762, 5, NULL, carrierFramex);
        	 	    carrierFlagSet.cmdType = CA_CMD_ACTIVE_REGISTER;
        	 	    break;
            	 	    
        	 	  case 3:
        	 	    carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
        	 	    carrierFlagSet.activeMeterNo   = 1;
        	 	    
        	 	    //ly,10-08-27,��Ӧ����
        	 	 	  carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
                carrierFlagSet.routeRunStatus  = 1;

        	 	  	if (carrierModuleType==MIA_CARRIER)
        	 	  	{
        	 	  	  gdw3762Framing(CTRL_CMD_3762, 2, NULL, NULL);    //�����ز��ӽڵ��ϱ�
        	 	  	  carrierFlagSet.cmdType = CA_CMD_PERMIT_ACTIVE;
        	 	  	} 
        	 	  	else
        	 	  	{
        	 	  	  gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);  //�����ѱ�
        	 	  	  carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
        	 	  	}
        	 	   	break;

            	case 4:
    	 	 	      //׼��ֹͣ�ѱ�
    	 	 	      if (compareTwoTime(carrierFlagSet.foundStudyTime, sysTime))
    	 	 	      {
		    	 	 		  if (debugInfo&PRINT_CARRIER_DEBUG)
		    	 	 		  {
		    	 	 		   	printf("��ѱ�ʱ���ѵ�,ֹͣ�ѱ�\n");
		    	 	 		  }
    	 	 		   
      	 	 		    //ֹͣ�ѱ�
    	 	 		      gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
    	 	 		      carrierFlagSet.cmdType = CA_CMD_PAUSE_WORK;
                  searchEnd = sysTime;                        //��������ʱ��

	    	 	 		    //if (carrierModuleType!=CEPRI_CARRIER)
	    	 	 		    //{
	    	 	 		    //  carrierFlagSet.searchMeter = 0;
	    	 	 		    //}
	  		 	 	  		 
			  		 	 	  if (menuInLayer==0)
			  		 	 	  {
			  		 	 	  	defaultMenu();
			  		 	 	  }
                  searchMeter(0xff);
               
	                //ly,2011-06-07,ȡ������
		              //ly,2011-12-02,�չ���������Ӧ���ָ�λЧ���Ե��Ժģ��Ϻ�,�����ִ�������
		              //if (carrierModuleType==CEPRI_CARRIER && carrierFlagSet.autoSearch==1)
		              if (carrierModuleType==CEPRI_CARRIER)
		              {
		               	//ly,2012-02-28,���,�°汾���������ٸ�λ��
		               	if (localModuleType!=CEPRI_CARRIER_3_CHIP)
		               	{
		               	  if (debugInfo&PRINT_CARRIER_DEBUG)
		               	  {
		               	 	  printf("���Ժģ��:�Զ�����������ģ��\n");
		               	  }
		               	 
		               	  sleep(1);
		               	 
		               	  ifReset=TRUE;
		               	}
		              }
               
                  goto breakPoint;
    	 	 	      }
    	 	 	  
    	 	 	      //�ѱ������,��ѯ·������״̬,ly,10-08-28
    	 	 	      if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
    	 	 	      {
    	 	 	 	      carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);
    	 	 	 	      carrierFlagSet.routeRunStatus = 1;
    	 	 	 	      goto breakPoint;
    	 	 	      }
    	 	 	  
    	 	 	      /*
    	 	 	      //����Ƕ����ز�ģ��,��ѯ����ע����ز��ӽڵ���Ϣ,�����Ѿ����ϸ�ָ��
    	 	 	      if (carrierModuleType==EAST_SOFT_CARRIER)
    	 	 	      {
    	 	 	        if (compareTwoTime(carrierFlagSet.operateWaitTime, sysTime))
    	 	 	        {
    	 	 	 	        carrierFramex[0] = carrierFlagSet.activeMeterNo;   //�ӽڵ���ʼ���
    	 	 	 	        carrierFramex[1] = 0x0;
    	 	 	 	        carrierFramex[2] = 0x5;
    	 	 	 	     
    	 	 	 	        gdw3762Framing(ROUTE_QUERY_3762, 6, NULL, carrierFramex);
    	 	 	 	  
    	 	 	 	        goto breakPoint;
    	 	 	 	      }
    	 	 	      }
    	 	 	      */
    	 	 	      break;
            	 	 	
            	case 0x05:
      	 	 	    carrierFramex[0] = 0x0;   //����ģʽ(������ע��,����״̬Ϊ����)
      	 	 	    carrierFramex[1] = 0x0;
      	 	 	    carrierFramex[2] = 0x0;
          	 	 	 
          	 	  gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
          	 	  carrierFlagSet.cmdType = CA_CMD_SET_WORK_MODE;
          	 	  goto breakPoint;
            	 	break;
            	 	 		
              case 0x06:
        	 	  	gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);  //��������
        	 	  	carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
        	 	  	break;
            }
            	 
    	  	  //2-3-1.1-20.�Ƚ�,ָ���ĳ���ʱ���Ƿ��ѵ�(����Ǹ�������ģ��,�����ѱ�ʱ�����ж��ѱ�,���ѱ���ɺ��ٳ���)
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
        	 	 	      printf("��Ѹ������ģ�� - ����ǰ��ѯģ��״̬\n");
        	 	 	    }
        	 	 	  }
                 	 
                goto breakPoint;
              }
                 	
              //�鿴�Ƿ�������ĳ���ʱ����
              ifFound = queryIfInCopyPeriod(4);
                 
              //�ڳ���ʱ���ڲſ��Կ�ʼ����
              if (ifFound==1)
              {
                if (debugInfo&PRINT_CARRIER_DEBUG)
                {
                  printf("��������ʱ����,");
                }
             
                if (copyCtrl[port].cmdPause==TRUE)
                {
                  //copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);
                
                  copyCtrl[port].nextCopyTime = nextTime(sysTime, 1, 0);                       
                  //2012-12-11,���ֳ����з���,�������ز��˿ڼ������������,��δ�������ַ��δ����
                  //           �޸�Ϊ���ޱ��ַʱ,1���Ӷ�һ��,ֱ���������ַΪֹ
                  checkCarrierMeter();
               
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("�����˿���ͣ����\n");
                  }
                }
                else
                {
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("�ұ��˿�δ��ͣ����\n");
                  }
               
                  if(!(carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS))
                  {
                    if (stopCarrierNowWork()==TRUE)
                    {
                      goto breakPoint;
                    }
                  }

                  //��ʼ�����˿ڵ������
                  copyCtrl[port].round = 1;          //��һ��
               
                  copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                  while(copyCtrl[port].tmpCpLink!=NULL)
                  {
           	        copyCtrl[port].tmpCpLink->thisRoundCopyed = FALSE;  //���γ����Ƿ�ɹ���Ϊfalse
           	        copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
                  }
               
                  //��������������Ļ�,�жϸó���������
                  if (carrierFlagSet.routeLeadCopy==0)
                  {
                    copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                    copyCtrl[port].ifCopyLastFreeze = 0;
                 
                    while(copyCtrl[port].tmpCpLink!=NULL)
                    {
      	              if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
         	            {
        	             #ifndef DKY_SUBMISSION
			 	 	  	       	  //2012-09-28,Ϊ��Ӧ�����ػ�,�������е�û���ն����07�������if����,ͬʱȡ�����Ϊ0x55������,��Ϊ�������վ�˲����ر��鷳
			 	 	  	       	  //  �޸�Ϊ:���þ����û�����ģʽΪ0x55(��ʵʱ����)��0xaa(��ʵʱ��ʾֵ)�����Ƴ�����
			 	 	  	       	  if (denizenDataType!=0x55 && denizenDataType!=0xaa)
			 	 	  	       	  {    
		 	  	       	        //2012-08-31,Ϊ�������������������if����
		 	  	       	        //  �����С��Ŷ�Ϊ0�Ļ�,�ն���ɼ��ɹ�,�Ͳ��ٲɼ�������������
		 	  	       	        //  �����Ժ���ϸ�ִ�б�׼��07���Ĭ�ϴ���ʽΪֻ�ɼ��ն�������
	          	            if (copyCtrl[port].tmpCpLink->bigAndLittleType!=0x00)
	          	            {
	          	         #endif
        	             
        	                  //����Ƿ���δ�ɼ���Сʱ��������
        	                  if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
        	                  {
        	                    copyCtrl[port].ifCopyLastFreeze = 3;
        	                    break;
        	                  }
        	           
        	             #ifndef DKY_SUBMISSION
        	                }
        	             #endif
        	           
        	                //0:10�ֺ�����Ƿ����ն�������,���û���ն�������,07����Ҫ�������ܱ���һ�ն�������
             	            if ((sysTime.hour==0 && sysTime.minute>=10) || sysTime.hour>0)
             	            {
      	                    //����Ƿ��е��� �ն�������
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
                    //2012-12-11,���ֳ����з���,�������ز��˿ڼ������������,��δ�������ַ��δ����
                    //           �޸�Ϊ���ޱ��ַʱ,1���Ӷ�һ��,ֱ���������ַΪֹ
                    checkCarrierMeter();
                 
                    //2013-12-30,�������ﻹ��������,��Ȼ�����ַ����ɿ��Ķ����˱��ַ,���ǲ�û��ͬ����·��ģ��
                    //           ����ͬ����ģ����
                    if (copyCtrl[port].tmpCpLink!=NULL)
                    {
                 	    carrierFlagSet.synSlaveNode = 1;
                    }
                       
            	      if (debugInfo&PRINT_CARRIER_DEBUG)
            	      {
              	      printf("�˿�31(�ز��˿�)�ޱ�ɳ�\n",port+1);
              	    }
                  }
                  else
                  {
                    //��鵥�����ݴ洢���Ƿ����
                    checkSpRealTable(0);

                    //����������ݴ洢���Ƿ����
                    checkSpRealTable(1);
 
                    //������㶳�����ݱ��Ƿ����
                    checkSpRealTable(2);

                    //���ݼ���������������·������������������Ӧ������������
                    if (carrierFlagSet.routeLeadCopy==1)   //·����������
                    {
                 	    //���ģ��Ϊѧϰ״̬,��Ϊ����״̬
                 	    if (carrierFlagSet.workStatus<=0)
                 	    {
                 	      carrierFlagSet.workStatus=1;
                 	    
                 	      if (debugInfo&PRINT_CARRIER_DEBUG)
                 	      {
                 	   	    printf("·����������:���ù���ģʽΪ����\n");
                 	      }
                	 	 	  carrierFramex[0] = 0x0;   //����ģʽ(������ע��,����״̬Ϊ����)
                	 	 	  carrierFramex[1] = 0x0;
                	 	 	  carrierFramex[2] = 0x0;
                    	 	 	 
                    	 	gdw3762Framing(ROUTE_SET_3762, 4, NULL, carrierFramex);
                    	 	carrierFlagSet.cmdType = CA_CMD_SET_WORK_MODE;
                    	 	goto breakPoint;
                      }
                         
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("·���������� - ģ�鹤��״̬Ϊ%d\n", carrierFlagSet.workStatus);
                      }
                         
                 	    //���ģ��ȷ��Ϊ����ʽ��,��������������
                 	    if (carrierFlagSet.workStatus==2 || carrierFlagSet.workStatus==5)
                 	    {
                 	      //2012-08-27,add,Ϊ��ֻͳ�Ƶ���ĳ����ɹ�����
                 	      //2013-12-30,ȥ����һ��,��������ж����Աȱ��ַ����ɿ�
                 	      //checkCarrierMeter();                       	   
                 	      if (0==carrierFlagSet.chkNodeBeforeCopy)
                 	      {
                 	   	    if (TC_CARRIER==carrierModuleType)
                 	   	    {
                 	   	      if (debugInfo&PRINT_CARRIER_DEBUG)
                 	   	      {
                 	   	 	      printf("·����������:�����ز�,��������ǰ��λ�ز�ģ��\n");
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
                 	   	 	    printf("·����������:��������ǰ����ȶԽڵ㵵��\n");
                 	   	    }
                 	   	 
                 	   	    goto breakPoint;
                 	      }
                     
                        carrierFlagSet.synSlaveNode   = 0;
                        carrierFlagSet.querySlaveNode = 0;
 
                 	      carrierFlagSet.workStatus = 2;
                 	   
                 	      if (debugInfo&PRINT_CARRIER_DEBUG)
                 	      {
                 	   	    printf("·����������:������������\n");
                 	      }

                 	      gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);
                 	      carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
                 	      carrierFlagSet.reStartPause = 0;

                 	      goto breakPoint;
                 	    }
                    }
                    else                                   //��������������
                    {
   	  	     	        //�ɼ���һ���ն�������/�������㶳������ʱҪ��λ��07��Լ��ʼ��
   	  	     	        if (copyCtrl[port].ifCopyLastFreeze==2 || copyCtrl[port].ifCopyLastFreeze==3)
   	  	     	        {
   	  	     	          while(copyCtrl[port].tmpCpLink!=NULL)
   	  	     	          {
   	  	     	 	          if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
   	  	     	 	          {
   	  	     	 	  	        if (copyCtrl[port].ifCopyLastFreeze==3)
   	  	     	 	  	        {
              	             #ifndef DKY_SUBMISSION
 	 	  	       	              //2012-09-28,Ϊ��Ӧ�����ػ�,�������е�û���ն����07�������if����,ͬʱȡ�����Ϊ0x55������,��Ϊ�������վ�˲����ر��鷳
 	 	  	       	              //  �޸�Ϊ:���þ����û�����ģʽΪ0x55(��ʵʱ����)��0xaa(��ʵʱ��ʾֵ)�����Ƴ�����
 	 	  	       	              if (denizenDataType!=0x55 && denizenDataType!=0xaa

     	 	  	       	               //2012-08-31,Ϊ�������������������if����
     	 	  	       	               //  �����С��Ŷ�Ϊ0�Ļ�,�ն���ɼ��ɹ�,�Ͳ��ٲɼ�������������
     	 	  	       	               //  �����Ժ���ϸ�ִ�б�׼��07���Ĭ�ϴ���ʽΪֻ�ɼ��ն�������
          	             	         && copyCtrl[port].tmpCpLink->bigAndLittleType!=0x00
          	             	       )
          	                  {
                	           #endif

        	                      //ȱ�������㶳�����ݵĻ��Ͳɼ��ñ�����㶳������
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
     	 	  	       	          //2012-09-28,Ϊ��Ӧ�����ػ�,�������е�û���ն����07�������if����,ͬʱȡ�����Ϊ0x55������,��Ϊ�������վ�˲����ر��鷳
     	 	  	       	          //  �޸�Ϊ:���þ����û�����ģʽΪ0x55(��ʵʱ����)��0xaa(��ʵʱ��ʾֵ)�����Ƴ�����
     	 	  	       	          if (denizenDataType!=0x55 && denizenDataType!=0xaa)
     	 	  	       	          {

                     	       #endif
                     	          
            	                  //ȱ��һ���ն������ݵĻ��Ͳɼ��ñ���ն�������
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
                         
                      //����ɼ�ʵʱ����,��λ����һ��δ�ɼ���ɵĲ����㿪ʼ�ɼ�
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
                     	 	 	 	    printf("��λ����һ��δ�ɼ���ɵĲ�����%d\n", copyCtrl[port].backMp);
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
                        printf("�˿�31��ʼ����\n");
                      }
                         
                     #ifdef PLUG_IN_CARRIER_MODULE
                      #ifndef MENU_FOR_CQ_CANON
                       showInfo("�ն����ڳ���...");
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
                  printf("����������ʱ����,����ʱ���������20��\n");
                }
	 	          }
   	 	      }
          }
          else       //�жϷ��ͳ�ʱ
          {
            if (carrierFlagSet.sending==1)
            {
             	if (compareTwoTime(carrierFlagSet.cmdTimeOut, sysTime))
             	{
             	  carrierFlagSet.retryCmd = 1;    //���ʱ,�����ط�
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
 	  	      //2-3-1.2-1.ת��(485�˿�)
    	      if (copyCtrl[port].pForwardData!=NULL)
    	      {
              forwardData(port);
              goto breakPoint;
    	      }
          	   
 	  	      //2-3-1.2-2.�Ƚ�,ָ���ĳ���ʱ���Ƿ��ѵ�
 	  	      if (compareTwoTime(copyCtrl[port].nextCopyTime, sysTime))
 	  	      {
              if (copyCtrl[port].cmdPause==FALSE)
              {
                //��ʼ�����˿ڵ������
               #ifdef LM_SUPPORT_UT
                if (4==port)
                {
             	    copyCtrl[port].cpLinkHead = initPortMeterLink(30);
                }
                else
                {
               #endif
            
                  copyCtrl[port].cpLinkHead = initPortMeterLink(port);
            
                  //�����ʼʱ�ж����������Ƿ�����
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
	                        //2015-06-12,��ӹ�ش���
	                        //2016-03-17,����ʱ�οؽ�Ϲ��ʱ���ַֺϷֵ����,��meter485Receive�������Ƶ����ﴦ��,
	                        //if (
	                        //	   CTRL_MODE_LIGHT==ctrlMode             //���
	                        //	    || CTRL_MODE_PERIOD_LIGHT==ctrlMode  //ʱ��+���
	                        //	  )
							            //2017-01-06,��������жϸĳ���������ж�,��Ϊ���żҸ۲��Է��ֹ���ھ�γ�Ƚ�Ϲ��ʱ��������
												  if (
													    CTRL_MODE_LIGHT==tmpNode->bigAndLittleType
														   || CTRL_MODE_PERIOD_LIGHT==tmpNode->bigAndLittleType
														    || CTRL_MODE_LA_LO_LIGHT==tmpNode->bigAndLittleType
													   )
                          {
                            //���Ҫ���Դ��բ,����·������Ϊ��բ
                            if (0==tmpNode->lcOnOff && 1==tmpNode->status)
                            {
                       	      if (copyCtrl[port].tmpCpLink!=NULL)
         					            {
         					              copyCtrl[port].tmpCpLink->flgOfAutoCopy |= REQUEST_OPEN_GATE;
         					        
         					              if (debugInfo&METER_DEBUG)
        	  	                  {
   	                              printf("%02d-%02d-%02d %02d:%02d:%02d,������,���Ҫ����·���Ƶ�%d��բ\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,copyCtrl[port].tmpCpLink->mp);
        	  	                  }
                 				      }
                            }
                               
                            //���Ϊ��բ,����·������Ϊ��բ(/��δ֪)
                            if (1==tmpNode->lcOnOff && 1!=tmpNode->status)
                            {
                       	      if (copyCtrl[port].tmpCpLink!=NULL)
         					            {
         					              copyCtrl[port].tmpCpLink->flgOfAutoCopy |= REQUEST_CLOSE_GATE;

         					              if (debugInfo&METER_DEBUG)
        	  	                  {
   	                              printf("%02d-%02d-%02d %02d:%02d:%02d,������,���Ҫ����·���Ƶ�%d��բ\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,copyCtrl[port].tmpCpLink->mp);
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

						        //2015-02-02,add,�������Ƕ˿�һ�ĲŲ�ѯ���µ���
                    if ((copyCtrl[port].tmpCpLink->protocol==AC_SAMPLE && copyCtrl[port].tmpCpLink->port==1)
               		      || copyCtrl[port].tmpCpLink->protocol==DLT_645_1997 
               		       || copyCtrl[port].tmpCpLink->protocol==DLT_645_2007
               		        || copyCtrl[port].tmpCpLink->protocol==EDMI_METER)
                    {
                      if ((copyCtrl[port].tmpCpLink->bigAndLittleType&0xf)==0x1)    //С���Ϊ1���ǵ����
                      {
                        if (debugInfo&METER_DEBUG)
                        {
                   	      printf("������%d����Ϊ�����,������������\n", copyCtrl[port].tmpCpLink->mp);
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
                   
                //���Ժ�ͼ�ʱ��Ҫ�ɼ���һ�ն�������,��Ϊģ���Ķ������ݶ���0,�ա��¶������ݹ�����
                //ly,2011-08-25
               #ifndef DKY_SUBMISSION
                copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                if (copyCtrl[port].ifCopyLastFreeze!=1)
                {
                  copyCtrl[port].ifCopyLastFreeze = 0;
                  while(copyCtrl[port].tmpCpLink!=NULL)
                  {
           	        //0:10�ֺ�����Ƿ����ն�������,���û���ն�������,07����Ҫ�������ܱ���һ�ն�������
           	        if ((sysTime.hour==0 && sysTime.minute>10) || sysTime.hour>0)
           	        {
    	                if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
  	     	 	          {
    	                  //����Ƿ��е��� �ն�������
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
                   
                if (copyCtrl[port].cpLinkHead==NULL)   //���˿��ޱ�ɳ�
                {
                  //�ϴγ���ʱ��(BCD��)
                  copyCtrl[port].lastCopyTime.second = copyCtrl[port].currentCopyTime.second/10<<4 | copyCtrl[port].currentCopyTime.second%10;;
                  copyCtrl[port].lastCopyTime.minute = copyCtrl[port].currentCopyTime.minute/10<<4 | copyCtrl[port].currentCopyTime.minute%10;;
                  copyCtrl[port].lastCopyTime.hour   = copyCtrl[port].currentCopyTime.hour/10<<4 | copyCtrl[port].currentCopyTime.hour%10;;
                  copyCtrl[port].lastCopyTime.day    = copyCtrl[port].currentCopyTime.day/10<<4 | copyCtrl[port].currentCopyTime.day%10;;
                  copyCtrl[port].lastCopyTime.month  = copyCtrl[port].currentCopyTime.month/10<<4 | copyCtrl[port].currentCopyTime.month%10;;
                  copyCtrl[port].lastCopyTime.year   = copyCtrl[port].currentCopyTime.year/10<<4 | copyCtrl[port].currentCopyTime.year%10;;

                  copyCtrl[port].nextCopyTime = nextTime(sysTime, teCopyRunPara.para[whichItem((4==port)?PORT_POWER_CARRIER:(port+1))].copyInterval ,0);
           	  
         	        if (debugInfo&METER_DEBUG)
         	        {
           	        printf("�˿�:%d�ޱ�ɳ�\n",port+1);
           	      }
                }
                else
                {
                  //���������ܱ����ݴ洢���Ƿ����
                  checkSpRealTable(1);
   	  	     	       
   	  	     	    //�ɼ���һ���ն�������ʱҪ��λ��07��Լ��ʼ��     	  	     	       
   	  	     	    if (copyCtrl[port].ifCopyLastFreeze==2)
   	  	     	    {
   	  	     	      if (debugInfo&METER_DEBUG)
   	  	     	      {
   	  	     	        printf("��ʼ����һ���ն�������ǰ��λ��07��\n");
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
  
   	  	     	      //���û��07��Լ�ı�,��ʼ��ʵʱ����
   	  	     	      if (copyCtrl[port].cpLinkHead==NULL)
   	  	     	      {
   	  	     	        copyCtrl[port].ifCopyLastFreeze = 0;
                      copyCtrl[port].cpLinkHead = initPortMeterLink(port);
   	  	     	      }
   	  	     	    }
   	  	     	       
   	  	     	    //���ɲ�����Ĵ���
      	  	   	  if (
						          copyCtrl[port].cpLinkHead->protocol==AC_SAMPLE
										 #ifdef LIGHTING
										  && 0==port
										 #endif
									   )
      	  	   	  {
        	          //�н���ģ��
        	         #ifdef AC_SAMPLE_DEVICE
        	          if (ifHasAcModule==TRUE)
        	          {
        	            copyCtrl[port].currentCopyTime = sysTime;
  
      	         	    //����ǳ���������,ת��������������
      	         	    if (copyCtrl[port].ifCopyLastFreeze==0x1)
      	         	    {
           	         	  copyAcValue(port, 2, sysTime);
  
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("CopyMeter:ת�������������ݲ�����\n");
                        }
                      }
                          
      	         	    //ת��ʵʱ����
      	         	    if (copyCtrl[port].ifCopyLastFreeze==0x0)
      	         	    {
           	         	  copyAcValue(port, 1, timeHexToBcd(copyCtrl[port].currentCopyTime));
                                           
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("CopyMeter:ת������ʵʱ���ݲ�����\n");
                        }
                      }
       	            }
       	           #endif
       	  	            
       	  	        tmpNode = copyCtrl[port].cpLinkHead;
      	  	        copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
      	  	   	    free(tmpNode);
  
             	  	  if (debugInfo&METER_DEBUG)
             	  	  {
             	  	    printf("CopyMeter:���ɲ�����,��λ\n");
             	  	  }
                        
                    if (copyCtrl[port].cpLinkHead==NULL)
                    {
             	  	   	if (debugInfo&METER_DEBUG)
             	  	   	{
             	  	   	  printf("CopyMeter:�˿�%dֻ��һ�����ɲ�����\n", port+1);
             	  	   	}
    
            	  	 	  //�ϴγ���ʱ��(BCD��)
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
       	 	                printf("copyMeter:Start-���˿�%dʵʱ����׼����λ, ifRealBalance=%d,ret=%d\n", port+1, copyCtrl[port].ifRealBalance, ret);
       	 	              }
       	 	             
         	 	            //���˿�ʵʱ������λ
       	 	              if (debugInfo&METER_DEBUG)
       	 	              {
       	 	                printf("copyMeter:���˿�%dʵʱ������λ\n",port+1);
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
                       
                    //2012-07-31,�ĵ����ﴦ������ֵ�Ĵ洢,
                    //  ԭ��:��̨�����ʱ�������������ܼӵ��������ǲ��ϸ�,�����������洢ʱ���Ⱥ��й�ϵ
           	 	     #ifdef PULSE_GATHER
           	 	      for(pulsei=0;pulsei<NUM_OF_SWITCH_PULSE;pulsei++)
           	 	      {
           	 	        if (pulse[pulsei].ifPlugIn == TRUE)
           	 	        {
                        memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
                        memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
                        memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);

                        //ת��
                        covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
                       		 	
                       	//����ʾֵ
                       	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                                       visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
                       	   
                        //�������
                       	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                                       paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
  
                       	//��������
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
                      printf("�˿�%d��ʼ����\n",port+1);
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
      else      //�ö˿����ڳ���
      {
  	    //2-3-2-1.�������쳣
  	    //2-3-2-2.��ֹ�ڳ���������´γ���ʱ���ѵ�,���ò�����ȷ���´γ���ʱ��
  	    if (port<5)
  	    {
  	      if (compareTwoTime(copyCtrl[port].nextCopyTime, sysTime))
  	      {
  	        copyCtrl[port].nextCopyTime = nextTime(copyCtrl[port].nextCopyTime, 0, 5);  //�´γ���ʱ���������5����

  	        if (debugInfo&METER_DEBUG)
  	        {
  	          if (port!=4)
  	          {
  	            printf("�˿�%d�´γ���ʱ���������5����\n",port+1);
  	          }
  	        }
  	      }
  	    }
   	  	   
   	  #ifdef PLUG_IN_CARRIER_MODULE
  	  	  
  	   #ifdef LM_SUPPORT_UT
  	  	if (4==port && 0x55!=lmProtocol)
  	   #else
  	  	if (port==4)    //�����ж�·�ɷ�����ͣ���������ָ���
  	   #endif
  	  	{
	  	 	  //2012-4-1,���������·�ɲ��ܲ����ز�ģ��
	  	 	  if (upRtFlag!=0)
	  	 	  {
	  	 	 	  goto breakPoint;
	  	 	  }
  	  	   	  
  	   	  //�����Ҫ������ͣ/�ָ���������
          //2012-01-16,·����������ʱ����ͣ/�ָ���������ķ��ʹ���
  	      if (carrierFlagSet.routeLeadCopy==1)
  	      {
  	        if (carrierFlagSet.reStartPause>0)
  	        {
  	      	  if (carrierFlagSet.retryCmd==1 || carrierFlagSet.cmdContinue==1)
  	      	  {
                switch (carrierFlagSet.reStartPause)
                {
               	  case 1:    //��������
                    if (carrierFlagSet.workStatus==2)
                    {
	  	     	          gdw3762Framing(ROUTE_CTRL_3762, 1, NULL, NULL);
    	  	            carrierFlagSet.cmdType = CA_CMD_RESTART_WORK;
    	  	            
    	  	            if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	            {
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,����������������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	            }
                    }
                    if (carrierFlagSet.workStatus==3)
                    {
                   	  carrierFlagSet.reStartPause = 0;

    	  	            if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	            {
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�յ�ȷ��������������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	            }
                    }
                    break;
               
                  case 2:    //��ͣ����
                    if (
                   	    carrierFlagSet.workStatus==4
                   	     || carrierFlagSet.workStatus==7        //��⵽������ʱ�ζ���ͣ
                   	      || carrierFlagSet.workStatus==8       //����ĩ���ز�ͨ�ŵ����ͣ
                   	       || carrierFlagSet.workStatus==9      //��������ֵ����ͣ
                   	        || carrierFlagSet.workStatus==10    //��ѯ���ƿ������ϵ�״̬����ͣ
                   	   )
                    {
	     	         			gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
	  	             		carrierFlagSet.cmdType = CA_CMD_PAUSE_WORK;
	  	            
	  	             		if (debugInfo&PRINT_CARRIER_DEBUG)
	  	             		{
                     		printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,������ͣ��������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
	  	             		}
                   	}
                   	if (carrierFlagSet.workStatus==5)
                   	{
                   	 	carrierFlagSet.reStartPause = 0;

    	  	         		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         		{
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�յ�ȷ����ͣ��������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	         		}
                   	}
                   	break;
               
               
                 	case 3:    //�ָ�����
                   	if (carrierFlagSet.workStatus==3)
                   	{
                   	 	carrierFlagSet.reStartPause = 0;

    	  	         		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         		{
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�յ�ȷ�ϻָ���������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	         		}
    	  	           
    	  	         		//2013-12-05,add
    	  	        	 #ifdef LIGHTING 
    	  	        	 	if (5==carrierFlagSet.broadCast)
 	  	             		{
 	  	               		carrierFlagSet.broadCast = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           		{
	                        printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�㲥����ȴ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}

    	  	         		if (1==carrierFlagSet.searchLddt)
 	  	             		{
 	  	               		carrierFlagSet.searchLddt = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           		{
	                       	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,����LDDT����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}
 	  	               
    	  	         		if (1==carrierFlagSet.searchOffLine)
 	  	             		{
 	  	               		carrierFlagSet.searchOffLine = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           		{
	                        printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�������ߵ��ƿ���������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
    	  	           		}
 	  	             		}

    	  	         		if (1==carrierFlagSet.searchLddStatus)
 	  	             		{
 	  	               		carrierFlagSet.searchLddStatus = 0;
    	  	              
    	  	           		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	          		{
	                        printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��ѯ���ϵ絥�ƿ�����״̬����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,���ͻָ���������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
             		else       //�жϷ��ͳ�ʱ
             		{
               		if (carrierFlagSet.sending==1)
                  {
             	 			if (compareTwoTime(carrierFlagSet.cmdTimeOut, sysTime))
             	 			{
             	  			carrierFlagSet.retryCmd = 1;    //���ʱ,�����ط�
             	 			}
               		}
             		}
             
             		goto breakPoint;
  	       		}
               
              //2011-07-12,·����������ʱ����վ�ӱ������ѱ�Ĵ���
    	  	   	//��վ���ñ��ַ��ȴ�һ����ͬ����ģ��
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
   	                printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��վ���ñ��ַ�ȴ�ʱ�䵽,��ʼͬ�����ַ��ģ��\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
   	              }
   	               
   	              goto breakPoint;
   	            }
   	          }
   	           
   	          //��⵽�ѱ�����
   	          if (carrierFlagSet.searchMeter==1 && carrierFlagSet.workStatus==3)
   	          {
   	            carrierFlagSet.workStatus = 4;
   	     	     	carrierFlagSet.reStartPause = 2;
                carrierFlagSet.operateWaitTime = nextTime(sysTime, 1, 0);
 
                copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);  
                      
   	            if (debugInfo&PRINT_CARRIER_DEBUG)
   	            {
   	              printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽�ѱ�����,��ͣ����ִ���ѱ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
   	            }
   	             
   	            goto breakPoint;
   	          }
   	           
   	          //·����������Ļ�,�ȴ�(ROUTE_REQUEST_OUT)���Ӻ���·������,��������
              if (compareTwoTime(carrierFlagSet.routeRequestOutTime,sysTime) && carrierFlagSet.workStatus==3)
              {
 	             	//2014-01-06,�޸ĵȴ�·������ʱ��Ĵ���,���κ���ļ��䴦��
 	             	//carrierFlagSet.workStatus = 2;
 	             	//carrierFlagSet.reStartPause = 1;
 	             
 	             	//if (debugInfo&PRINT_CARRIER_DEBUG)
 	             	//{
   	            //  printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽·������ʱ,����������������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
 	             	//}

  	  	      	if (carrierFlagSet.retryCmd==1 || carrierFlagSet.cmdContinue==1)
  	  	      	{
  	  	      	  if (carrierFlagSet.numOfCarrierSending<3)
  	  	      	  {
 	                  if (debugInfo&PRINT_CARRIER_DEBUG)
 	                  {
   	                  printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽·������ʱ,���Ͳ�ѯ·��״̬����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
 	                  }

            	 	 		gdw3762Framing(ROUTE_QUERY_3762, 4, NULL, NULL);
            	 	 		carrierFlagSet.cmdType = CA_CMD_QUERY_ROUTE_STATUS;
  	  	      	  }
  	  	      	  else
  	  	      	  {
 	                 	if (debugInfo&PRINT_CARRIER_DEBUG)
 	                 	{
   	                  printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,���Ͳ�ѯ·��״̬����3��δ�ظ�,��λ·��,���¿�ʼ\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
 	                 	}

  	  	      	 	 	close(fdOfCarrier);
                    carrierUartInit();
                    sleep(5);
                     
                    resetCarrierFlag();
                    carrierFlagSet.routeLeadCopy = 1;
                     
                    copyCtrl[4].meterCopying = FALSE;
                    if (TC_CARRIER==carrierModuleType)
                    {
                      printf("��������\n");
                     	 
                      copyCtrl[5].meterCopying = FALSE;
                      copyCtrl[6].meterCopying = FALSE;
                    }
  	  	      	  }
  	  	      	}
                else       //�жϷ��ͳ�ʱ
                {
                  if (carrierFlagSet.sending==1)
                  {
                 	  if (compareTwoTime(carrierFlagSet.cmdTimeOut, sysTime))
                 	 	{
                 	   	carrierFlagSet.retryCmd = 1;    //���ʱ,�����ط�
                 	 	}
                  }
                }
 	             
 	             	goto breakPoint;
              }
               
             #ifdef LIGHTING
              //�㲥������,�жϵȴ�����
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
     	             	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�㲥����ȴ���ɺ�ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                  }
   	  	     	 	}
   	  	     	 	 
   	  	     	 	goto breakPoint;
   	  	      }
   	  	     #endif
  	  	    }

continueCopying:
  	  	   	//ÿ���ӵĵ�9��鿴�Ƿ�������ĳ���ʱ����
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
     	               	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽������ʱ��,������ͣ��������\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	             	}
     	             
     	             	goto breakPoint;
  	  	          }
                }
               
                //2013-1-8,add�����λ�� 
                if (5==carrierFlagSet.workStatus || 6==carrierFlagSet.workStatus)
                {
     	           	if (debugInfo&PRINT_CARRIER_DEBUG)
     	           	{
     	             	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽������ʱ��,��������ͣ״̬,��Ϊδ����̬\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
            //���µ���� - �鿴��ĩ�˵ĵ��ƿ����������Ƿ�����
            if (0==carrierFlagSet.searchLddt)
            {
              if (compareTwoTime(carrierFlagSet.searchLddtTime, sysTime)==TRUE)
              {
               	carrierFlagSet.searchLddtTime = nextTime(sysTime, 2, 0);    //�̶�ÿ2���Ӳ�ѯһ��
               	 
                ifFound = 0;
               	carrierFlagSet.searchLdgmNode = ldgmLink;
               	while (carrierFlagSet.searchLdgmNode!=NULL)
               	{
           	 	    if (
           	 	 	   		((carrierFlagSet.searchLdgmNode->status&0x1)==1)    //��·�ǽ�������
           	 	 	   		 && 
           	 	 	    		//2�����ڻ�ȡ�ı������Ƶ�״̬
           	 	 	    		(
           	 	 	     			(timeCompare(carrierFlagSet.searchLdgmNode->statusTime, sysTime, 2) == TRUE)
           	 	 	      		|| (timeCompare(sysTime, carrierFlagSet.searchLdgmNode->statusTime,  2) == TRUE)
           	 	 	    		)
           	 	 	  	 )
           	 	   	{
         	 	     		//��һĩ�˵��ƿ�������ַ��Ϊȫ0
         	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->lddt1st, NULL, 1))
         	 	     		{
         	 	 	   			tmpNode = copyCtrl[4].cpLinkHead;
 	  		 	 	   				while(tmpNode!=NULL)
 	  		 	 	   				{
 	  		 	 	   	 				if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->lddt1st, 0)==TRUE)
 	  		 	 	   	 				{
				  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
			 	  		 	 	   	   	{
			  		 	 	   	 		 		printf("%02d-%02d-%02d %02d:%02d:%02d:���µ�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)�ĵ�һĩ�˵��ƿ�����(%02x-%02x-%02x-%02x-%02x-%02x)�����ͨ��ʱ��Ϊ%02d-%02d-%02d %02d:%02d:%02d",
			  		 	 	   	 		      	 	 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
			  		 	 	   	 		       			carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
			  		 	 	   	 		       			tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
			  		 	 	   	 		       			 tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
			  		 	 	   	 		       		);
			 	  		 	 	   	   	}
			 	  		 	 	   	 	 
			 	  		 	 	   	   	//tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
			 	  		 	 	   	   	tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
 	  		 	 	   	   				if (compareTwoTime(tmpTime, sysTime))    //�����������ʱ��
 	  		 	 	   	   				{
			  		 	 	   	 	 	 		if (
			  		 	 	   	 	 	 	   		tmpNode->lddtRetry>=99                     //����LDDT���Դ���=99,���Զ�����OK
			  		 	 	   	 	 	      	|| tmpNode->lddtRetry<pnGate.lddtRetry    //������LDDT���Դ���С��������Դ���
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
				 	  		 	 	   	 	    	 printf(",�ѳ����������ʱ��");
				 	  		 	 	   	 	   	}
				 	  		 	 	   	 	 	}
				  		 	 	   	 		 	else
				  		 	 	   	 		 	{
				  		 	 	   	 	    	if (debugInfo&PRINT_CARRIER_DEBUG)
				  		 	 	   	 	    	{
				  		 	 	   	 	        printf(",���������ѳ���");
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
                 	 	 
         	 	     		//�ڶ�ĩ�˵��ƿ�������ַ��Ϊȫ0
         	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->collectorAddr, NULL, 1))
         	 	     		{
         	 	 	   			tmpNode = copyCtrl[4].cpLinkHead;
			 	  		 	 	   	while(tmpNode!=NULL)
			 	  		 	 	   	{
			 	  		 	 	   	 	if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->collectorAddr, 0)==TRUE)
			 	  		 	 	   	 	{
			 	  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
			 	  		 	 	   	   	{
			  		 	 	   	 		 		printf("%02d-%02d-%02d %02d:%02d:%02d:���µ�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)�ĵڶ�ĩ�˵��ƿ�����(%02x-%02x-%02x-%02x-%02x-%02x)�����ͨ��ʱ��Ϊ%02d-%02d-%02d %02d:%02d:%02d",
			  		 	 	   	 		        		sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
			  		 	 	   	 		         		 carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
			  		 	 	   	 		          		tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
			  		 	 	   	 		           		 tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
			  		 	 	   	 		       		);
			 	  		 	 	   	   	}
			 	  		 	 	   	 	 
			  		 	 	   	 	    //tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
			  		 	 	   	 	   	tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
			  		 	 	   	 	   	if (compareTwoTime(tmpTime, sysTime))    //�����������ʱ��
			  		 	 	   	 	   	{
			  		 	 	   	 	     	if (
			  		 	 	   	 	 	 	 			tmpNode->lddtRetry>=99                     //����LDDT���Դ���=99,���Զ�����OK
			  		 	 	   	 	 	     		 || tmpNode->lddtRetry<pnGate.lddtRetry    //������LDDT���Դ���С��������Դ���
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
			  		 	 	   	 	         	printf(",�ѳ����������ʱ��");
			  		 	 	   	 		   		}
			  		 	 	   	 		 		}
			  		 	 	   	 		 		else
			  		 	 	   	 		 		{
			  		 	 	   	 	       	if (debugInfo&PRINT_CARRIER_DEBUG)
			  		 	 	   	 	       	{
			  		 	 	   	 	        	 printf(",���������ѳ���");
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
  		 	 	       			printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽��ʱ��ĩ�˵��ƿ�����,�����ٲ���ĩ�˵��ƿ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
  		 	 	     			}
     	             
     	             	goto breakPoint;
  	  	          }
               	}
              }
            }
             
            //�鿴�Ƿ������ߵĵ��ƿ�����,2015-11-09
            if (
             	  0==carrierFlagSet.searchOffLine
             	   && 0==carrierFlagSet.searchLddt    //��û������LDDTʱ���ж�,��LDDT������
             	 )
            {
              if (compareTwoTime(carrierFlagSet.searchOffLineTime, sysTime)==TRUE)
              {
                carrierFlagSet.searchOffLineTime = nextTime(sysTime, 3, 0);    //�̶�ÿ3���Ӳ�ѯһ��
               	 
               	ifFound = 0;
               	tmpNode = copyCtrl[port].cpLinkHead;
               	while(tmpNode!=NULL)
               	{
           	 	    if (
           	 	 	      //2016-11-28,����������ֵ�ĵ�λ�ӡ��֡���Ϊ��Сʱ��
					    				compareTwoTime(nextTime(tmpNode->statusTime, pnGate.lddOffGate*60, 0), sysTime)==TRUE
           	 	 	     	 && (tmpNode->offLineRetry<pnGate.offLineRetry || tmpNode->offLineRetry>=99)
                   	    && (tmpNode->lineOn>=1 && tmpNode->lineOn<=5)    //���ƿ����������ϵ�״̬
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
  		 	 	       			printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽��������ֵ���ƿ�����,�����ٲ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
  		 	 	     			}
     	             
     	             	goto breakPoint;
  	  	          }
               	}
               	else
               	{
  		 	 	   			if (debugInfo&PRINT_CARRIER_DEBUG)
  		 	 	   			{
  		 	 	     			printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�޳�������ֵ�ĵ��ƿ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
  		 	 	   			}
               	}
              }
            }

            //�鿴�Ƿ�����Ҫ�ٲ��ϵ�״̬�ĵ��ƿ�����,2015-11-12
            if (
             	  0==carrierFlagSet.searchLddt              //��û������LDDTʱ���ж�,��LDDT������,���ȼ����
             	   && 0==carrierFlagSet.searchOffLine       //��û���������ߵ��ƿ�����ʱ���ж�,���ȼ��ڶ�
             	    && 0==carrierFlagSet.searchLddStatus    //���ȼ����
             	 )
            {
              if (compareTwoTime(carrierFlagSet.searchLddStatusTime, sysTime)==TRUE)
              {
               	carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 1, 0);    //�̶�ÿ1���Ӳ�ѯһ��
               	 
               	ifFound = 0;
               	tmpNode = copyCtrl[port].cpLinkHead;
               	while(tmpNode!=NULL)
               	{
           	 	   	if (1==tmpNode->lineOn)    //�ز���·���ϵ�
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
		  		 	 	       	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��⵽���ƿ�������·���ϵ�,�����ٲ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
		  		 	 	     	}
     	             
     	             	goto breakPoint;
  	  	          }
               	}
               	else
               	{
			  		 	 	  if (debugInfo&PRINT_CARRIER_DEBUG)
			  		 	 	  {
			  		 	 	    printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�����ѯ��·���ϵ�ĵ��ƿ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
			  		 	 	  }
               	}
              }
            }
             
           #endif
 	        }
 	         
 	       #endif    //#ifdef PLUG_IN_CARRIER_MODULE
     	  	 
   	  	  //2-3-2-3.���������־��λ�����Ա�־��λ,�������³���
   	  	  if (copyCtrl[port].copyContinue == TRUE || copyCtrl[port].flagOfRetry == TRUE)
   	      {
   	        //2-4-2-3-0.����յ�������ͣ����
   	        if (copyCtrl[port].cmdPause==TRUE)
   	        {
   	          //·����������ʱ��ͣ����
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
         	          printf("�˿�%d��ͣ����\n",port);
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
   	         	    printf("�˿�%d��ͣ����\n",port);
   	         	  }
   	         	}
   	         	 
   	         	goto breakPoint;
   	        }
   	         
   	        //2-3-2-3-1.ת����㳭����
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
       	  	     	 
       	  	     	//�㲥����
       	  	     	if (0<carrierFlagSet.broadCast)
       	  	     	{
         	  	     	//���͹㲥Уʱ
         	  	     	if (1==carrierFlagSet.broadCast)
         	  	     	{
                     	memset(copyCtrl[port].destAddr, 0x99, 6);
                     	memset(carrierFlagSet.destAddr, 0x99, 6);
                     	copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
    
     		 	 	   			 	carrierFramex[2] = slcFrame(carrierFlagSet.destAddr, &carrierFramex[3], 1, copyCtrl[4].tmpCpLink);    //����+���ĳ���
                     	carrierFramex[1] = 0x02;    //Э��
                     	carrierFramex[0] = carrierFramex[2]+2;    //����
    
                     	gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
                       
                     	carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
     		 	 	   			 	carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
     	  	     	 		//���͹㲥��վ���Ƶ�����
     	  	     	 		if (2==carrierFlagSet.broadCast)
     	  	     	 		{
                      memset(copyCtrl[port].destAddr, 0x99, 6);
                      memset(carrierFlagSet.destAddr, 0x99, 6);
                      copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
    
	 	 	       	   			memcpy(&carrierFramex[3], copyCtrl[4].pForwardData->data, copyCtrl[4].pForwardData->length);  //����
	 	 	       	   			carrierFramex[2] = copyCtrl[4].pForwardData->length;  //���ĳ���
                      carrierFramex[1] = 0x02;    //Э��
                      carrierFramex[0] = carrierFramex[2]+2;    //����
    
                      gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
                       
                      carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
     		 	 	   				carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
         	  	     	//���͹㲥ʱ��
         	  	     	if (3==carrierFlagSet.broadCast)
         	  	     	{
                      memset(copyCtrl[port].destAddr, 0x99, 6);
                      memset(carrierFlagSet.destAddr, 0x99, 6);
                      copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
                       
     		 	 	   				carrierFramex[2] = slcFrame(carrierFlagSet.destAddr, &carrierFramex[3], 2, copyCtrl[4].tmpCpLink);    //����+���ĳ���
                      carrierFramex[1] = 0x02;                  //Э��
                      carrierFramex[0] = carrierFramex[2]+2;    //����
     		 	 	       	  
     		 	 	   				gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
     		 	 	       	   
     		 	 	   				carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
     		 	 	   				carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
         	  	     	//���͹㲥��ؿ��ص��������,2015-06-16,add
         	  	     	if (4==carrierFlagSet.broadCast)
         	  	     	{
                      memset(copyCtrl[port].destAddr, 0x99, 6);
                      memset(carrierFlagSet.destAddr, 0x99, 6);
                      copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
                       
     		 	 	   				carrierFramex[2] = slcFrame(carrierFlagSet.destAddr, &carrierFramex[3], 12, copyCtrl[4].tmpCpLink);    //����+���ĳ���
                      carrierFramex[1] = 0x02;    //Э��
                      carrierFramex[0] = carrierFramex[2]+2;    //����
     		 	 	       	   
	 	 	       	   			gdw3762Framing(CTRL_CMD_3762, 3, copyCtrl[port].destAddr, carrierFramex);
	 	 	       	   
	 	 	       	   			carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 5, 0);
	 	 	       	   			carrierFlagSet.cmdType = CA_CMD_BROAD_CAST;
         	  	     	}
         	  	     	 
                    copyCtrl[port].flagOfRetry  = FALSE;
     		 	 	 				copyCtrl[port].copyContinue = FALSE;
                     
                    copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 5);
    
         	  	     	goto breakPoint;
         	  	   	}
         	  	     
         	  	   	//��������LDDT
         	  	   	if (1==carrierFlagSet.searchLddt)
         	  	   	{
         	  	     	if (copyCtrl[port].tmpCpLink!=NULL && carrierFlagSet.searchLdgmNode!=NULL)
         	  	     	{
           	  	      copyCtrl[port].tmpCpLink->lddtRetry++;
           	  	      if (copyCtrl[port].tmpCpLink->lddtRetry>pnGate.lddtRetry)
           	  	      {
           	  	     	 	if (copyCtrl[port].tmpCpLink->lddtRetry<99)
           	  	     	 	{
           	  	     	   	//Ӧ�ü�¼�������Ƶ��쳣�ˣ���Ϊĩ�˵��ƿ�����������^^
                          searchMpStatis(sysTime, &meterStatisExtranTimeS, carrierFlagSet.searchLdgmNode->mp, 3); //������������ʱ���޹���
                      	
                      	  //�����������ж�Ϊ��·�쳣,��¼�����¼�
                 	  	   	printf("%02d-%02d-%02d %02d:%02d:%02d:���µ�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)��ĩ�˵��ƿ�����(%02x-%02x-%02x-%02x-%02x-%02x)������,�ж�Ϊ��·�쳣\n",
                 	  		  				sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
                 	  		 	   			 carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
                 	  		 	 					copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],copyCtrl[port].tmpCpLink->addr[3],copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]
                 	  		 	 			);

                  		   	//δ��¼�Ĳż�¼
                  		   	if (0==(meterStatisExtranTimeS.mixed&0x04))
                  		   	{
                    		 		if (eventRecordConfig.iEvent[7]&0x10)    //ERC61,��վ����
                    	     	{
                    		   		//��¼���Ƶ������Ϣ(ERC13)
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
                    	       	eventData[9] = (carrierFlagSet.searchLdgmNode->mp>>8&0xff) | 0x80;    //����
                    	       	eventData[10] = 0x40;    //�������Ƶ��쳣,�е�״̬����
                    	  
                    	       	writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                    	  
                    	       	if (eventRecordConfig.nEvent[1]&0x10)
                    	       	{
                    	         	writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                    	       	}
                    	  
                    	       	eventStatus[1] = eventStatus[1] | 0x10;
            					  
				    					       	if (debugInfo&PRINT_CARRIER_DEBUG)
				    					       	{
				    					        	printf("  --��¼�������Ƶ�%d(��������ʱ)�쳣�����¼�\n", carrierFlagSet.searchLdgmNode->mp);
				    					       	}
                         
                              activeReport3();    //�����ϱ��¼�
                    	   
                    	       	meterStatisExtranTimeS.mixed |= 0x04;
                          
                              //�洢���Ƶ�ͳ������(��ʱ���޹���)
                              saveMeterData(carrierFlagSet.searchLdgmNode->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                            }
                          }
                          else
                          {
				    					     	if (debugInfo&PRINT_CARRIER_DEBUG)
				    					     	{
				    					     	  printf("  --�������Ƶ�%d(��������ʱ)�쳣�����¼��Ѽ�¼����\n", carrierFlagSet.searchLdgmNode->mp);
				    					     	}
                          }
                           
                          //��λ����һ���������Ƶ�,�����������һ�������Ƶ������
                          if (compareTwoAddr(copyCtrl[port].tmpCpLink->addr, carrierFlagSet.searchLdgmNode->collectorAddr, 0)==TRUE)
         	  		 	   			{
         	  		 	 	 				carrierFlagSet.searchLdgmNode = carrierFlagSet.searchLdgmNode->next;
         	  		 	   			}
   	  	     	 	     		}
           	  	     	 	 
           	  	     	 	//��λ����һ����Ҫ�ٲ��ĩ�˵��ƿ�����
                       	ifFound = 0;
                       	while (carrierFlagSet.searchLdgmNode!=NULL)
                       	{
                   	 	   	if (
                   	 	 	   		((carrierFlagSet.searchLdgmNode->status&0x1)==1)    //��·�ǽ�������
                   	 	 	   		 && (    //2���ӻ�ȡ��״̬
                   	 	 	       			(timeCompare(carrierFlagSet.searchLdgmNode->statusTime, sysTime, 2) == TRUE)
                   	 	 	       			|| (timeCompare(sysTime, carrierFlagSet.searchLdgmNode->statusTime,  2) == TRUE)
                   	 	 	      		)
                   	 	 	  	 )
                       	 	{
                 	 	     		//��һĩ�˵��ƿ�������ַ��Ϊȫ0
                 	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->lddt1st, NULL, 1))
                 	 	     		{
                 	 	 	   			tmpNode = copyCtrl[4].cpLinkHead;
	         	  		 	 	   			while(tmpNode!=NULL)
         	  		 	 	   				{
         	  		 	 	   	 				if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->lddt1st, 0)==TRUE)
         	  		 	 	   	 				{
     	  		 	 	   	 	   					if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	   	 	   					{
					     	  		 	 	   	 		 	printf("%02d-%02d-%02d %02d:%02d:%02d:���µ�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)�ĵ�һĩ�˵��ƿ�����(%02x-%02x-%02x-%02x-%02x-%02x)�����ͨ��ʱ��Ϊ%02d-%02d-%02d %02d:%02d:%02d",
					     	  		 	 	   	 		       		sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
					     	  		 	 	   	 		       			carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
					     	  		 	 	   	 		       				tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
					     	  		 	 	   	 		       					tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
					     	  		 	 	   	 		       	);
					     	  		 	 	   	 	  }
         	  		 	 	   	 	 
				     	  		 	 	   	 	    //tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
				     	  		 	 	   	 	   	tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
				     	  		 	 	   	 	   	if (compareTwoTime(tmpTime, sysTime))    //�����������ʱ��
				     	  		 	 	   	 	   	{
				     	  		 	 	   	 	 	 		if (
				     	  		 	 	   	 	 	 	   		tmpNode->lddtRetry>=99                     //����LDDT���Դ���=99,���Զ�����OK
				     	  		 	 	   	 	 	     		 || tmpNode->lddtRetry<pnGate.lddtRetry    //������LDDT���Դ���С��������Դ���
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
				     	  		 	 	   	 	        	printf(",�ѳ����������ʱ��");
				     	  		 	 	   	 		   		}
					     	  		 	 	   	 		 	}
					     	  		 	 	   	 		 	else
					     	  		 	 	   	 		 	{
					     	  		 	 	   	 	      if (debugInfo&PRINT_CARRIER_DEBUG)
					     	  		 	 	   	 	      {
					     	  		 	 	   	 	        printf(",���������ѳ���");
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
                         	 	 
                 	 	     		//�ڶ�ĩ�˵��ƿ�������ַ��Ϊȫ0
                 	 	     		if (FALSE == compareTwoAddr(carrierFlagSet.searchLdgmNode->collectorAddr, NULL, 1))
                 	 	     		{
                 	 	       		tmpNode = copyCtrl[4].cpLinkHead;
         	  		 	 	   				while(tmpNode!=NULL)
         	  		 	 	   				{
         	  		 	 	   	 				if (compareTwoAddr(tmpNode->addr, carrierFlagSet.searchLdgmNode->collectorAddr, 0)==TRUE)
         	  		 	 	   	 				{
			         	  		 	 	   	   	if (debugInfo&PRINT_CARRIER_DEBUG)
			         	  		 	 	   	   	{
					         	  		 	 	   	 	printf("%02d-%02d-%02d %02d:%02d:%02d:���µ�������Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)�ĵڶ�ĩ�˵��ƿ�����(%02x-%02x-%02x-%02x-%02x-%02x)�����ͨ��ʱ��Ϊ%02d-%02d-%02d %02d:%02d:%02d",
					         	  		 	 	   	 	       sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
					         	  		 	 	   	 		      carrierFlagSet.searchLdgmNode->addr[5],carrierFlagSet.searchLdgmNode->addr[4],carrierFlagSet.searchLdgmNode->addr[3],carrierFlagSet.searchLdgmNode->addr[2],carrierFlagSet.searchLdgmNode->addr[1],carrierFlagSet.searchLdgmNode->addr[0],
					         	  		 	 	   	 		       tmpNode->addr[5],tmpNode->addr[4],tmpNode->addr[3],tmpNode->addr[2],tmpNode->addr[1],tmpNode->addr[0],
					         	  		 	 	   	 		         tmpNode->lddtStatusTime.year,tmpNode->lddtStatusTime.month, tmpNode->lddtStatusTime.day, tmpNode->lddtStatusTime.hour, tmpNode->lddtStatusTime.minute, tmpNode->lddtStatusTime.second
					         	  		 	 	   	 		    );
					         	  		 	 	   	}
         	  		 	 	   	 	 
         	  		 	 	   	   				//tmpTime = nextTime(tmpNode->statusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);
         	  		 	 	   	   				tmpTime = nextTime(tmpNode->lddtStatusTime, carrierFlagSet.searchLdgmNode->funOfMeasure, 0);    //2016-02-03,modify
     	  		 	 	   	 	   					if (compareTwoTime(tmpTime, sysTime))    //�����������ʱ��
     	  		 	 	   	 	   					{
     	  		 	 	   	 	 	 						if (
     	  		 	 	   	 	 	 	   						tmpNode->lddtRetry>=99                     //����LDDT���Դ���=99,���Զ�����OK
     	  		 	 	   	 	 	      					 || tmpNode->lddtRetry<pnGate.lddtRetry    //������LDDT���Դ���С��������Դ���
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
				     	  		 	 	   	 	          printf(",�ѳ����������ʱ��");
				     	  		 	 	   	 		      }
					     	  		 	 	   	 		 	}
					     	  		 	 	   	 		 	else
					     	  		 	 	   	 		 	{
				     	  		 	 	   	 	       	if (debugInfo&PRINT_CARRIER_DEBUG)
				     	  		 	 	   	 	       	{
				     	  		 	 	   	 	        	printf(",���������ѳ���");
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
                       	  //�̶�ÿ2���Ӳ�ѯһ��
                       	  carrierFlagSet.searchLddtTime = nextTime(sysTime, 2, 0);

                       	  //�ָ�����
                          carrierFlagSet.workStatus = 6;
                          carrierFlagSet.reStartPause = 3;
                          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,����LDDT��ɺ�ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                       	}
           	  	      }
           	  	      else
           	  	      {
                        if (
             	 	 	     			((carrierFlagSet.searchLdgmNode->status&0x1)==1)    //��·�ǽ�������
             	 	 	      		 && (    //2���ӻ�ȡ��״̬
             	 	 	          			(timeCompare(carrierFlagSet.searchLdgmNode->statusTime, sysTime, 2) == TRUE)
             	 	 	          			|| (timeCompare(sysTime, carrierFlagSet.searchLdgmNode->statusTime,  2) == TRUE)
             	 	 	         			)
             	 	 	    		 )
             	 	     		{
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                     	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,���ͽ�����������.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                           
                          memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
                          copyCtrl[port].protocol = 2;
                           
                          tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);
  
     	  		 	 	   				sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
                           
                          copyCtrl[port].flagOfRetry  = FALSE;
       		 	 	       			copyCtrl[port].copyContinue = FALSE;
                       
                          //�ٲ�ȴ�ʱ��Ϊ90��
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
         	  	     
         	  	   	//�������ߵ��ƿ�����
         	  	   	if (1==carrierFlagSet.searchOffLine)
         	  	   	{
     	  	     	 		if (copyCtrl[port].tmpCpLink!=NULL)
     	  	     	 		{
       	  	     	   	copyCtrl[port].tmpCpLink->offLineRetry++;
       	  	     	   	if (copyCtrl[port].tmpCpLink->offLineRetry>pnGate.offLineRetry)
       	  	     	   	{
       	  	     	     	if (copyCtrl[port].tmpCpLink->offLineRetry<99)
       	  	     	     	{
       	  	     	 	   		//Ӧ�ü�¼���ƿ��Ƶ�������
                          searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].tmpCpLink->mp, 3);    //������Ƶ���ʱ���޹���
                  	
                  	      //��¼�����¼�
             	  		   		printf("%02d-%02d-%02d %02d:%02d:%02d:���ƿ��Ƶ�(%02x-%02x-%02x-%02x-%02x-%02x)������,�ж�Ϊ����\n",
             	  		 	 	   				sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
             	  		 	 	    				copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],copyCtrl[port].tmpCpLink->addr[3],copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]
             	  		 	 	 				);

                  		   	//δ��¼�Ĳż�¼
                  		   	if (0==(meterStatisExtranTimeS.mixed&0x08))
                  		   	{
                    		 		//��¼���Ƶ������Ϣ(ERC13)
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
                    	     	eventData[9] = (copyCtrl[port].tmpCpLink->mp>>8&0xff) | 0x80;    //����
                    	     	eventData[10] = 0x40;    //���ƿ���������
                    	  
                    	     	writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
                    	  
                    	     	if (eventRecordConfig.nEvent[1]&0x10)
                    	     	{
                    	       	writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
                    	     	}
                    	  
                    	     	eventStatus[1] = eventStatus[1] | 0x10;
            					  
			            				 	if (debugInfo&PRINT_CARRIER_DEBUG)
			            				 	{
			            				   	printf("  --��¼���ƿ��Ƶ�%d���߷����¼�\n", copyCtrl[port].tmpCpLink->mp);
			            				 	}
                         
                            activeReport3();    //�����ϱ��¼�
                    	   
                    	     	meterStatisExtranTimeS.mixed |= 0x08;
                          
                            //�洢���Ƶ�ͳ������(��ʱ���޹���)
                            saveMeterData(copyCtrl[port].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                          }
	                       	else
	                       	{
    					     					if (debugInfo&PRINT_CARRIER_DEBUG)
    					     					{
    					       					printf("  --���ƿ��Ƶ�%d���߷����¼��Ѽ�¼����\n", copyCtrl[port].tmpCpLink->mp);
    					     					}
	                       	}
       	  	     	 	 		}
           	  	     	 	 
           	  	     	 	//��λ����һ����Ҫ�ٲ�����ߵ��ƿ�����
                       	ifFound = 0;
                       	tmpNode = copyCtrl[4].tmpCpLink->next;
                       	while(tmpNode!=NULL)
                       	{
                   	 	   	if (
                   	 	 	   		//2016-11-28,����������ֵ�ĵ�λ�ӡ��֡���Ϊ��Сʱ��
							   							compareTwoTime(nextTime(tmpNode->statusTime, pnGate.lddOffGate*60, 0), sysTime)==TRUE
           	 	 	            	 && (tmpNode->offLineRetry<pnGate.offLineRetry || tmpNode->offLineRetry>=99)
                   	 	 	     		&& (tmpNode->lineOn>=1 && tmpNode->lineOn<=5)    //���ƿ����������ϵ�״̬
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
                       	  //�̶�ÿ3���Ӳ�ѯһ��
                       	  carrierFlagSet.searchOffLineTime = nextTime(sysTime, 3, 0);

                       	  //�ָ�����
                          carrierFlagSet.workStatus = 6;
                          carrierFlagSet.reStartPause = 3;
                          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                     	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�������ߵ��ƿ�������ɺ�ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                       	}
           	  	      }
           	  	      else
           	  	      {
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
   	                      printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,�������ߵ��ƿ�����,���ͽ�����������.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                        }
                         
                        memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                        memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                        copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
                        copyCtrl[port].protocol = 2;
                         
                        tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);

   	  		 	 	     			sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
                         
                        copyCtrl[port].flagOfRetry  = FALSE;
     		 	 	     				copyCtrl[port].copyContinue = FALSE;
                     
                        //�ٲ�ȴ�ʱ��Ϊ90��
                        copyCtrl[port].copyTimeOut = nextTime(sysTime, 1, 30);
                      }
                    }
                    else
                    {
                      carrierFlagSet.searchOffLine = 0;
                    }
                     	 
                    goto breakPoint;
         	  	   	}
         	  	     
         	  	   	//��ѯ��·���ϵ絥�ƿ�����״̬,2015-11-12
         	  	   	if (1==carrierFlagSet.searchLddStatus)
         	  	   	{
         	  	    	if (pDotCopy==NULL && (copyCtrl[port].pForwardData==NULL))
         	  	     	{
           	  	      if (copyCtrl[port].tmpCpLink!=NULL)
           	  	      {
         	  	     	 		copyCtrl[port].tmpCpLink->lineOn++;
         	  	     	 		if (copyCtrl[port].tmpCpLink->lineOn>3)
         	  	     	 		{
     	  	     	 	   			//��λ����һ����Ҫ�ٲ�����ߵ��ƿ�����
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
                     	 	 		//�̶�ÿ1���Ӳ�ѯһ��
                     	 	 		carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 1, 0);

                     	 	 		//�ָ�����
                            carrierFlagSet.workStatus = 6;
                            carrierFlagSet.reStartPause = 3;
                            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     

                            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 2);
                            if (debugInfo&PRINT_CARRIER_DEBUG)
                            {
   	                          printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,��ѯ���ƿ�����״̬��ɺ�ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                            }
                     	   	}
         	  	     	 		}
         	  	     	 		else
         	  	     	 		{
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
     	                     	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,���Ͳ�ѯ���ϵ絥�ƿ�����״̬����.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                          }
                           
                          memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
                          copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
                          copyCtrl[port].protocol = 2;
                           
                          tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);
  
     	  		 	 	   				sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
                           
                          copyCtrl[port].flagOfRetry  = FALSE;
       		 	 	       			copyCtrl[port].copyContinue = FALSE;
                       
                          //�ٲ�ȴ�ʱ��Ϊ90��
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
                       	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,���ڲ�ѯ���ϵ絥�ƿ�����״̬ʱ��ת����㳭,������ѯִ��ת����㳭.\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                      }
                    }
                     	 
                    goto breakPoint;
         	  	   	}
       	  	    }
       	  	  }
     	  	     
     	  	  #endif
     	  	    
 	  	       	//2-3-2-3-1.1.�㳭(�ز��˿�)
 	  	       	//2013-12-30,���&& (copyCtrl[port].pForwardData==NULL)
 	  	       	if (pDotCopy!=NULL && (copyCtrl[port].pForwardData==NULL))
 	  	       	{
     	  	     	//·����������Ļ�,Ҫ�鿴�Ƿ��ڳ���������״̬
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
                    //·����������ʽ��,��������״̬�Ƕ������㳭��ʱ��
                    copyCtrl[port].flagOfRetry = TRUE;
                  }
                  else
                  {
                    //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
                    //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
                    //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
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
        		   		pDotFn129->ifProcessing = 1;         //���ڴ���

        		   		goto breakPoint;
      	  	   	}
      	  	  }
     	  		}
     	  	 #endif    //#ifdef PLUG_IN_CARRIER_MODULE

     	  	 	//2-3-2-3-1.2.ת��(485�˿ڼ��ز��˿�)
        	 	if (copyCtrl[port].pForwardData!=NULL)
        	 	{
             #ifdef PLUG_IN_CARRIER_MODULE
              //2013-12-30,���,���ڵ㳭�Ļ�,�ȴ��㳭���
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
             
   	        //2-3-2-3-2.���ͳ�������֡
          #ifdef PLUG_IN_CARRIER_MODULE
           #ifdef LM_SUPPORT_UT
            if (port>=4 && 0x55!=lmProtocol)
           #else
            //if (port==4)
            if (port>=4)    //ly,2012-01-09
           #endif
            {
              //�����ÿ��Сʱ50�ֶ�ʵʱ���ݻ�δ����,����ͣ��ʵʱ����,��ΪÿСʱ1�ֿ�ʼҪ�����㶳������
              //ly,2011-01-15,��ÿСʱ0�ָĳ�ÿСʱ50��ֹͣ����,��Ϊ���ģ��Ҫ����һ��ʱ��������
              //ly,2011-07-07,��������������ʱ�Ž����������
              if (copyCtrl[port].ifCopyLastFreeze==0 && sysTime.minute==50 && sysTime.second>=0 && carrierFlagSet.routeLeadCopy==0)
              {
               	copyCtrl[port].meterCopying = FALSE;

               	//ly,2011-01-14,add,Ϊ�˽��һ��Сʱ���������һ��СʱҪ���ų���û�н��ų���bug
               	copyCtrl[port].backMp = copyCtrl[port].tmpCpLink->mp;
               	goto breakPoint;
              }
               
              if (copyCtrl[port].flagOfRetry == TRUE && carrierFlagSet.routeLeadCopy==1)
              {
               	//·����������ʱ,Ϊ����ѭ���ĳ�ʱʱ��Ϊ2��
               	copyCtrl[port].copyTimeOut = nextTime(sysTime, 0, 2);
              }
              else
              {
                if (carrierModuleType==MIA_CARRIER || carrierModuleType==EAST_SOFT_CARRIER)
                {
                  //����΢����ĳ���ʱʱ��Ϊ40s,������ز�ģ��ʵ��Ϊ35s����
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
              copyCtrl[port].copyTimeOut = nextTime(sysTime,0,4);    //2016-03-21,����������485�ĳ�4��
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
	                   	  printf("�ز�����(������%d):��������û����,��ֹ�ò����㱾�γ���\n", copyCtrl[port].tmpCpLink->mp);
	                    }
	                   
	                 		goto stopThisMeterPresent;
	                  }

	                  copyCtrl[port].retry = 0;
	 
	                  if (debugInfo&METER_DEBUG)
	                  {
	                    printf("�ز�����:��ʱ����,������һ��\n");
	                  }

	                  ret = meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);           //��֡����(������һ��)                     
	                }
	                else
	                {
	                  if (debugInfo&METER_DEBUG)
	                  {
	                    printf("�ز�����:��ʱ,������һ��\n");
	                  }

	                  ret = meterFraming(PORT_POWER_CARRIER, LAST_COPY_ITEM_RETRY);    //��֡����(������һ��)
	                }
	              }
	              else
	              {
                	//·����������ʱ��������ָ��,�ȴ�·������ʱ�ٷ�
                	if (debugInfo&PRINT_CARRIER_DEBUG)
                	{
                	  //printf("·����������ʱ��������ָ��,�ȴ�·������ʱ�ٷ�\n");
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
                    
                  	//ֻҪ����ʧ��>6�����Ϊ485�������(LY 08.12.22�ĳ��������ж�,
                  	//ֻҪ����ʧ��>1�����Ϊ485�������(LY 18.08.06),
                  	//�������ڼ�ʹ��������ҲҪ�������ж�,�����Ļ��ڳ�������������2�������ڳ�����һ���)
                 	 #ifdef LIGHTING
					  				if (copyCtrl[port].numOfMeterAbort > 0)
					 				 #else
					  				if (copyCtrl[port].numOfMeterAbort > 14)
					 				 #endif
                    {
                      copyCtrl[port].numOfMeterAbort = 0xfe;
                      		  
                      //��¼�ն�485����ʧ���¼�
                      copy485Failure(copyCtrl[port].cpLinkHead->mp, 1, port+1);

                     #ifdef LIGHTING
												 
									    //2016-11-25,���ֳ������г���ʧ��
									    copyCtrl[port].thisRoundFailure = TRUE;

                     #else

									    //��¼�ն˹��� - 485�������
									    if ((eventRecordConfig.iEvent[2] & 0x10) || (eventRecordConfig.nEvent[2] & 0x10))
									    {
										  	eventData[0] = 0x15;
										  	eventData[1] = 0xa;
												
										  	//����ʱ��
										  	eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
										  	eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
										  	eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
										  	eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
										  	eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
										  	eventData[7] = sysTime.year/10<<4   | sysTime.year%10;

										  	//���ϱ���--485�������
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
											
										  	activeReport3();   //�����ϱ��¼�
									    }
										
									   #endif
                     	    
                     	copyCtrl[port].retry = 0;
                     	    
                     	if (debugInfo&METER_DEBUG)
                     	{
                     	  printf("copyMeter:��ʱ���������趨����ʧ�ܵĳ�ʱ����,�ж�Ϊ����485����.δ���泭��������\n");
                     	}
                     	    
                     	goto stopThisMeterPresent;
                    }
                  }
                     
                  copyCtrl[port].retry = 0;

                 #ifdef LIGHTING
                  //��·������
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

                    //��֡����(������һ��)
                    //2012-3-28,modify����Ĵ���
                    //ret = meterFraming(port+1, NEW_COPY_ITEM);        //��֡����(������һ��)
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
                else  //��֡����(������һ��)
                {
                   	
                 #ifdef LIGHTING
                 	//��·������
                 	if (LIGHTING_XL==copyCtrl[port].cpLinkHead->protocol)
                 	{
             	   		ret = xlcFrame(port);
                 	}
									  //����������
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

                   	//2012-3-28,modify����Ĵ���
                   	//ret = meterFraming(port+1, LAST_COPY_ITEM_RETRY);
                   #ifdef RS485_1_USE_PORT_1
                 
                   	ret = meterFraming(port+1, LAST_COPY_ITEM_RETRY);

                   	//ABB������
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
                
                    //ABB������
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
            else                                  //������һ���
            {
              copyCtrl[port].copyContinue = FALSE;
               
            #ifdef PLUG_IN_CARRIER_MODULE
             #ifdef LM_SUPPORT_UT
              if (port>=4 && 0x55!=lmProtocol)    //�ز�/���߶˿�
             #else
              if (port>=4)    //�ز�/���߶˿�
             #endif
              {
                carrierSendToMeter = 1;    //2013-12-25
                 
                if (carrierFlagSet.routeLeadCopy==1)    //·����������
                {
		  		 	 	   	if (compareTwoAddr(copyCtrl[port].needCopyMeter, 0, 1)==TRUE)
		  		 	 	   	{
		  		 	 	   	 	if (debugInfo&PRINT_CARRIER_DEBUG)
		  		 	 	   	 	{
		  		 	 	   	  	printf("·����������:�ݴ�·������ĵ�ַȫ��0,�����ͳ�������\n");
		  		 	 	   	 	}
		  		 	 	   	 
		  		 	 	   	 	goto breakPoint;
		  		 	 	   	}
		                   
                  //��������ĵ�ַ���ϴγ����ĵ�ַ��һ�µ�,���������־����0�Ļ�(�����쳣,�϶����ϴγ�ʱ��û������������,·�ɾͲ�������),
                  //   Ҫ������Ϊ0,�Ա��´�����ʱ��ͷ��ʼ��,����©��
		 	 	   				if (copyCtrl[port].tmpCpLink!=NULL)
		 	 	   				{
	 	 	   	 	 				if (debugInfo&PRINT_CARRIER_DEBUG)
	 	 	   	 	 				{
				 	 	   	 	   	//printf("�����ַ:%02x%02x%02x%02x%02x%02x\n",carrierFlagSet.needCopyMeter[5],carrierFlagSet.needCopyMeter[4],
				 	 	   	 	   	//  carrierFlagSet.needCopyMeter[3],carrierFlagSet.needCopyMeter[2],carrierFlagSet.needCopyMeter[1],carrierFlagSet.needCopyMeter[0]);
				 	 	   	 	   	//printf("tmpLink��ַ:%02x%02x%02x%02x%02x%02x\n",copyCtrl[port].tmpCpLink->addr[5],copyCtrl[port].tmpCpLink->addr[4],
				 	 	   	 	   	//  copyCtrl[port].tmpCpLink->addr[3],copyCtrl[port].tmpCpLink->addr[2],copyCtrl[port].tmpCpLink->addr[1],copyCtrl[port].tmpCpLink->addr[0]);
				 	 	   	 	 	}

		 	 	           #ifdef LIGHTING
		 	 	      			if (compareTwoAddr(copyCtrl[port].needCopyMeter, copyCtrl[port].tmpCpLink->addr, 0)==FALSE)     	  		 	 	     
		 	 	           #else
					 	 	     	//����вɼ�����ַ,��Ƚϲɼ���ַ��·������ĵ�ַ;���û�вɼ�����ַ,��Ƚϵ���ַ��ɼ�����ַ
					 	 	     	//   ��ǰ����ĵ�ַ����һ�βɼ��ĵ�ַ��ͬ,����һ��û�вɼ���ɵĻ�,Ҫ���ϴβɼ��Ľڵ㳭���־��Ϊ0,�´�����ʱ���³���
					 	 	     	if (((compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==FALSE) && (compareTwoAddr(copyCtrl[port].needCopyMeter, copyCtrl[port].tmpCpLink->collectorAddr, 0)==FALSE))
					 	 	     	    || ((compareTwoAddr(copyCtrl[port].tmpCpLink->collectorAddr, 0, 1)==TRUE) && (compareTwoAddr(copyCtrl[port].needCopyMeter, copyCtrl[port].tmpCpLink->addr, 0)==FALSE))
					 	 	     		)
					 	 	     #endif
					 	 	     	{
				 	 	   	      if (copyCtrl[port].tmpCpLink->flgOfAutoCopy!=0)
				 	 	   	      {
				 	 	   	 	     	copyCtrl[port].tmpCpLink->flgOfAutoCopy=0;
			   	  		 	 	   	 	     
                       #ifdef LIGHTING
                       	//30����δ��������CCB����Ϣ,����״̬��Ϊδ֪
                        if (!((timeCompare(copyCtrl[port].tmpCpLink->statusTime, sysTime, 30) == TRUE) 
     	                      || (timeCompare(sysTime, copyCtrl[port].tmpCpLink->statusTime, 30) == TRUE)))
     	                 	{
     	                   	//2014-09-19,status��ʾ���ǵƵ�������
     	                   	copyCtrl[port].tmpCpLink->status = 0xfe;
     	                 	}
     	                	#endif

				 	 	   	 	     	if (debugInfo&PRINT_CARRIER_DEBUG)
				 	 	   	 	     	{
				 	 	   	 	   	   	printf("·����������:��һ���û������,��λ��־,�´�����ʱ��ͷ��ʼ��\n");
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
  		 	 	   	 	 				printf("·����������:���ز��������ҵ�Ҫ�����ı��ַ\n");
  		 	 	   	   			}
  		 	 	   	 		
  		 	 	   	   			ifFound = 1;
  		 	 	   	   			break;
  		 	 	   	 			}
  		 	 	   	 			copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
  		 	 	   			}
     	  		 	 	   
		 	 	   				//����ĵ�ַ�ǲɼ�����ַ�Ĵ���(һ���ɼ����¿����ж��485���)
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
					 	 	   	 		      printf("·����������:���ز��������ҵ�Ҫ�����Ĳɼ�����ַ\n");
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
					 	 	   	 	 		printf("·����������:���ز��������ҵ�Ҫ�����Ĳɼ�����ַ,�����ɼ��±��ֶ��ѳ���,������λ��δ������־\n");
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
		  		 	 	     	//2013-10-17,���,·�Ƽ������ز��˿ڳ�������
		  		 	 	     #ifdef LIGHTING
		  		 	 	     
		  		 	 	     	//printf("autocopy=%x\n", copyCtrl[port].tmpCpLink->flgOfAutoCopy);
		  		 	 	     
		  		 	 	     	//AutoCopy-bit0-�����ƿ�����״̬
		  		 	 	     	//         bit1-������ʱ��
		  		 	 	    	//         bit2-�����ص�ʱ��
		  		 	 	     	//         bit3-��Сʱ��������
		  		 	 	     	//         bit4-Уʱ
		  		 	 	     	//         bit5-Уʱ��
		  		 	 	     	//         bit6-У�������
									 
		                //���ƿ��Ƶ���ʱ���޹���
							 			searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[4].tmpCpLink->mp, 3);

							 			memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
							 			memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);
							 			copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ

  		 	 	     			//���ƿ�������ҪУʱ
  		 	 	     			if (copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CHECK_TIME)
  		 	 	     			{
		 	 	       				tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 1, copyCtrl[4].tmpCpLink);
		 	 	       				sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
  		 	 	     			}
 	 	             		else
 	 	             		{
 	 	               		if (
					       					(
														copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_1         //SLC��Ҫͬ������ʱ�ε�һ��
														 || copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_2     //SLC��Ҫͬ������ʱ�εڶ���
														  || copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_CTRL_TIME_3    //SLC��Ҫͬ������ʱ�ε�����
													)
														 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)                          //����δͬ�����·�
												 )
 	 	               		{
                        tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 8, copyCtrl[4].tmpCpLink);
   	  		 	 	     			sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
     	  		 	   			}
  		 	 	       			else
  		 	 	       			{
  		 	 	         			if (
											     	copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SYN_ADJ_PARA    //SLC��Ҫ�������
												    	&& ((meterStatisExtranTimeS.mixed&0x20)==0x00)                 //����δͬ�����·�
											     )
			     	  		 	 	 	{
			                    tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 9, copyCtrl[4].tmpCpLink);
			   	  		 	 	      sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
			     	  		 	 	 	}
			     	  		 	 	 	else
			     	  		 	 	 	{
       	  		 	 	   			if ((copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_STATUS)==0x0)    //SLC��ѯ״̬���ݰ���δ����
       	  		 	 	   			{
                            tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 5, copyCtrl[4].tmpCpLink);
       	  		 	 	     			sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
       	  		 	 	   			}
       	  		 	 	   			else
       	  		 	 	   			{
                            //�еƾ߽����SLC��ѯ����汾��δ����
		  		 	 	              if (
		  		 	 	           	      (copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_SOFT_VER)==0x0
		  		 	 	           	       && copyCtrl[port].tmpCpLink->joinUp==0x1
								                  && ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //����δͬ�����·�
		  		 	 	           	     )
		  		 	 	             	{
		                         	tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 13, copyCtrl[4].tmpCpLink);
		       	  		 	 	       	sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
		       	  		 	 	     	}
		       	  		 	 	     	else
		       	  		 	 	     	{
													   	//�еƾ߽����SLC��ѯ����ʱ��С����δ����
													   	if (
														   		 (
																		(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_1)==0x0            //С��1
												             ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_2)==0x0         //С��2
												              ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_3)==0x0        //С��3
												               ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_4)==0x0       //С��4
												                ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_5)==0x0      //С��5
												                 ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_6)==0x0     //С��6
												                  ||(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_CTRL_TIME_7)==0x0    //С��7
																	 )
															  	  && copyCtrl[port].tmpCpLink->joinUp==0x1
																		 && ((meterStatisExtranTimeS.mixed&0x20)==0x00)    //����δͬ�����·�
														     )
													   	{
														 		tmpData = slcFrame(copyCtrl[port].tmpCpLink->addr, slSendBuf, 10, copyCtrl[4].tmpCpLink);
														 		sendCarrierFrame(PORT_POWER_CARRIER, slSendBuf, tmpData);
													   	}
													   	else
													   	{
														 		//�еƾ߽����SLC��ѯ���ص����ݰ���δ����
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
															   	//�еƾ߽������м������ܵĵ��ƿ�������Сʱ��������
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
																	   			//�еƾ߽����SLC״̬������ʱ�κͿ��ص�ʱ�̶������ݶ��Ѿ�����
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
																		     //�޵ƾ߽����SLC״̬�����Ѿ�����
																		     copyCtrl[port].tmpCpLink->joinUp==0x0
																			    && (REQUEST_STATUS ==(copyCtrl[port].tmpCpLink->flgOfAutoCopy&REQUEST_STATUS))
																		    )
																	    )
								     								{
																	   	//�����ɹ�
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
																		 		printf("·����������:��CCB�����ɹ�\n");
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
     	  		 	 	    
  		 	 	     			//δ��ʼ������
  		 	 	     			switch (copyCtrl[port].tmpCpLink->flgOfAutoCopy)
  		 	 	     			{
  		 	 	       			case 0:    //δ����
                        if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
     	 	             		{
 	 	  	       	      	 #ifndef DKY_SUBMISSION
 	 	  	       	       		//2012-09-28,Ϊ��Ӧ�����ػ�,�������е�û���ն����07�������if����,ͬʱȡ�����Ϊ0x55������,��Ϊ�������վ�˲����ر��鷳
 	 	  	       	       		//  �޸�Ϊ:���þ����û�����ģʽΪ0x55(��ʵʱ����)��0xaa(��ʵʱ��ʾֵ)�����Ƴ�����
 	 	  	       	       		if (denizenDataType==0x55 || denizenDataType==0xaa)
 	 	  	       	       		{
  		 	 	   	         			initCopyDataBuff(port, PRESENT_DATA);
  		 	 	   	         			copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
 	 	  	       	       		}
 	 	  	       	       		else
 	 	  	       	       		{
 	 	  	       	      	 #endif
 	 	  	       	    
     	 	  	             		//�ն�������
     	 	  	             		if (checkLastDayData(copyCtrl[port].tmpCpLink->mp, 0, sysTime, 1)==FALSE)
     	 	  	             		{
	  		 	 	   	       				initCopyDataBuff(port, LAST_DAY_DATA);
	  		 	 	   	       				copyCtrl[port].tmpCpLink->flgOfAutoCopy = 1;
     	 	  	             		}
     	 	  	             		else
     	 	  	             		{
     	 	  	           	
	     	 	  	       	       #ifndef DKY_SUBMISSION
	     	 	  	       	       	//2012-08-28,Ϊ�������������������if����
	     	 	  	       	       	//  �����С��Ŷ�Ϊ0�Ļ�,�ն���ɼ��ɹ�,�Ͳ��ٲɼ�������������,ֱ����"�����ɹ�"
	     	 	  	       	       	//  �����Ժ���ϸ�ִ�б�׼��07���Ĭ�ϴ���ʽΪֻ�ɼ��ն�������
	     	 	  	       	       	if (copyCtrl[port].tmpCpLink->bigAndLittleType==0x00)
	     	 	  	       	       	{
	     	 	  	       	     	 		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 8;

				                     		//2013-12-25,add
				                     		carrierSendToMeter = 0;
				                     		memcpy(carrierFlagSet.destAddr, copyCtrl[port].tmpCpLink->addr, 6);
				                     		memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);

	     	 	  	       	     	 		if (debugInfo&PRINT_CARRIER_DEBUG)
	     	 	  	       	     	 		{
	 	 	  	       	     	 	   			printf("%02d-%02d-%02d %02d:%02d:%02d,��ʼ��������ʱ,�Դ�С��Ŷ�Ϊ0��07��(%02x%02x%02x%02x%02x%02x),ֻ����������,flagOfAutoCopy��Ϊ8\n", 
	 	 	  	       	     	 	         			 sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
	 	 	  	       	     	 	          			copyCtrl[port].tmpCpLink->addr[5], copyCtrl[port].tmpCpLink->addr[4], copyCtrl[port].tmpCpLink->addr[3],
	 	 	  	       	     	 	           			copyCtrl[port].tmpCpLink->addr[2], copyCtrl[port].tmpCpLink->addr[1], copyCtrl[port].tmpCpLink->addr[0]
	 	 	  	       	     	 	        			);
	     	 	  	       	     	 		}
	     	 	  	       	       	}
	     	 	  	       	       	else
	     	 	  	       	       	{
	     	 	  	       	       #endif
	     	 	  	       	    
	     	 	  	       	         	//Сʱ��������
	     	 	  	       	         	if (checkHourFreezeData(copyCtrl[port].tmpCpLink->mp, copyCtrl[port].dataBuff)>0)
	     	 	  	       	         	{
	  		 	 	   	               		initCopyDataBuff(port, HOUR_FREEZE_DATA);
	  		 	 	   	               		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 3;
	     	 	  	       	         	}
	     	 	  	       	         	else    //��ʵʱ����
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
     	 	  	         		else     //97��ֱ�ӳ�ʵʱ����
     	 	  	         		{
  		 	 	   	       			initCopyDataBuff(port, PRESENT_DATA);
  		 	 	   	       			copyCtrl[port].tmpCpLink->flgOfAutoCopy = 5;
     	 	  	         		}
     	 	  	         		break;
  	     	 	  	       
 	 	  	           		case 2:   //�ն��������ѳ���
 	 	  	             		if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
 	 	  	             		{
 	 	  	       	      	 #ifndef DKY_SUBMISSION
 	 	  	       	       		//2012-08-28,Ϊ�������������������if����
 	 	  	       	       		//  �����С��Ŷ�Ϊ0�Ļ�,�ն���ɼ��ɹ�,�Ͳ��ٲɼ�������������,ֱ����"�����ɹ�"
 	 	  	       	       		if (copyCtrl[port].tmpCpLink->bigAndLittleType==0x00)
 	 	  	       	       		{
			                 			//2014-01-03,add
			                 			carrierSendToMeter = 0;

 	 	  	       	         		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 8;
 	 	  	       	   
		 	 	  	       	 				if (debugInfo&PRINT_CARRIER_DEBUG)
		 	 	  	       	 				{
		 	 	  	       	   				printf("%02d-%02d-%02d %02d:%02d:%02d,��ʼ��������ʱ,�Դ�С��Ŷ�Ϊ0��07��(%02x%02x%02x%02x%02x%02x),�ն��������ѳ���,flagOfAutoCopy��Ϊ8\n", 
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
	 	 	  	       	     			else    //��ʵʱ����
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
  		 	 	   	   
  		 	 	   	   			case 4:   //Сʱ���������ѳ���
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
  		 	 	   	   			printf("·����������:���ز�������δ�ҵ�·������ĵ�ַ,�ظ�ģ�����\n");
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
                 
                //2013-12-25,�������ж�
                if (carrierSendToMeter==0)
                {
                  ret = COPY_COMPLETE_SAVE_DATA;
                 	 
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                 	 	printf("���ڽ����ն������ݵ�07���������ѳ���,ֱ����Ϊ���ѱ���\n");
                  }
                }
                else
                {
                  ret = meterFraming(PORT_POWER_CARRIER+port-4, NEW_COPY_ITEM);    //��֡����,ly,2012-01-09
                }
              }
              else    //485�˿�
              {
             #endif
                 
               #ifdef LIGHTING
              	//��·������
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
                   
	                //2012-3-28,modify����Ĵ���
	               #ifdef RS485_1_USE_PORT_1
	                ret = meterFraming(port+1, NEW_COPY_ITEM);                //��֡����

	                //2012-09-20,add
	                //ABB������
	                if (abbHandclasp[port]==1)
	                {
	                  copyCtrl[port].copyTimeOut = nextTime(sysTime,0,1);
	                }

	               #else
	               
	                if (port==0)
	                {
	                  ret = meterFraming(1, NEW_COPY_ITEM);                   //��֡����
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
	                  
	                    ret = meterFraming(port, NEW_COPY_ITEM);                //��֡����
	                  
	                 #ifdef LM_SUPPORT_UT
	                  }
	                 #endif
	                }
	              
	                //2012-09-20,add
	                //ABB������
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
	       	       	printf("������������Ч�����ѱ���\n");
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
    	       	   	   	//·����������
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
    	       	   	   	else    //��������������
    	       	   	   	{
    	       	   	     	if (copyCtrl[port].ifCopyLastFreeze==0)
    	       	   	     	{
    	       	   	       	copyCtrl[port].tmpCpLink->thisRoundCopyed = TRUE;   //���ֳ���ɹ�
    	       	   	    
    	       	   	       	if (debugInfo&METER_DEBUG)
    	       	   	       	{
    	       	   	         	printf("������%d���ֳ���ǰ���ݳɹ�\n", copyCtrl[port].tmpCpLink->mp);
    	       	   	       	}	
    	       	   	     	}
    	       	   	     	else
    	       	   	     	{
    	       	   	       	if (debugInfo&METER_DEBUG)
    	       	   	       	{
    	       	   	         	if (copyCtrl[port].ifCopyLastFreeze==2)
    	       	   	         	{    	       	   	      
    	       	   	          	printf("������%d���ֳ��ն������ݳɹ�,thisRoundCopyed=%d\n", copyCtrl[port].tmpCpLink->mp,copyCtrl[port].tmpCpLink->thisRoundCopyed);
    	       	   	         	}
    	       	   	      
    	       	   	         	if (copyCtrl[port].ifCopyLastFreeze==3)
    	       	   	         	{    	       	   	      
    	       	   	           	printf("������%d���ֳ��������ݳɹ�,thisRoundCopyed=%d\n", copyCtrl[port].tmpCpLink->mp,copyCtrl[port].tmpCpLink->thisRoundCopyed);
    	       	   	         	}
    	       	   	       	}
    	       	   	    
    	       	   	       	//2012-3-29,ȥ�����ע��,����ͳ�Ƴ���ɹ���
    	       	   	       	//#ifdef CQDL_CSM
    	       	   	       	if (copyCtrl[port].ifCopyLastFreeze==2)
    	       	   	       	{ 
    	       	             	copyCtrl[port].tmpCpLink->copySuccess = TRUE;   //����ɹ�
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
	       	       	printf("�˿�%d��%02x%02x%02x%02x%02x%02x���굫����Ч����,flagofAutoCopy=%d\n", 
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
       	       	//·����������ʱ���������Ϊ����ʧ��
        	     	if (carrierFlagSet.routeLeadCopy==1 && port>=4)
        	     	{
    	       	   	//copyCtrl[port].tmpCpLink->flgOfAutoCopy = 7;
	       	   	   	//ly,2012-01-10,�ĳ�switch�ж�
	       	   	   	switch(copyCtrl[port].tmpCpLink->flgOfAutoCopy)
	       	   	   	{
	       	   	     	case 1:
     	 	  	       		if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
     	 	  	       		{
     	 	  	       		 #ifndef DKY_SUBMISSION
     	 	  	       	 		//2012-12-25,Ϊ�������������������if����
     	 	  	       	 		//  �����С��Ŷ�Ϊ0�Ļ�,�ն���ɼ��ɹ�,�Ͳ��ٲɼ�������������,ֱ����"����ʧ��"
     	 	  	       	 		if (copyCtrl[port].tmpCpLink->bigAndLittleType==0x00)
     	 	  	       	 		{
     	 	  	       	   		copyCtrl[port].tmpCpLink->flgOfAutoCopy = 9;
                          memcpy(copyCtrl[port].destAddr, copyCtrl[port].tmpCpLink->addr, 6);

     	 	  	       	   		if (debugInfo&PRINT_CARRIER_DEBUG)
     	 	  	       	   		{
     	 	  	       	     		printf("%02d-%02d-%02d %02d:%02d:%02d,�Դ�С��Ŷ�Ϊ0��07��(%02x%02x%02x%02x%02x%02x),�ն������ݱ�ʶ������ϵ���δ��������,flagOfAutoCopy��Ϊ9\n", 
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
	       	   	   
	       	   	     	case 8:    //�ն����ѳ���,û��������,2013-12-25
	       	   	       	copyCtrl[port].tmpCpLink->flgOfAutoCopy = 9;
	       	 	       		if (debugInfo&METER_DEBUG)
	       	 	       		{
	       	             	printf("%02d-%02d-%02d %02d:%02d:%02d,�˿�%d����(%02x%02x%02x%02x%02x%02x)�ն������ݱ�ʶ�ѷ����굫����Ч����,flagofAutoCopy��Ϊ9\n", 
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
                buf[1] = DATA_CTL_W_IEC1107;      //�˿�����
                usleep(200000);
                sendXmegaInTimeFrame(COPY_PORT_RATE_SET, buf, 2);
      
	              if (debugInfo&METER_DEBUG)
	              {
	               	printf("���ö˿�%d����\n", port);
	             	}

	             	copyCtrl[port].copyTimeOut = nextTime(sysTime,0,15);
	             	copyCtrl[port].retry = NUM_COPY_RETRY;
	         	 		break;
	         	 
	       	   	default:
	  	         	break;
    	  	 	}
    	  	   
	  	     	//������,ɾ�������еı��ڵ�,��������һ���
	  	     	if (ret==COPY_COMPLETE_NOT_SAVE || ret==COPY_COMPLETE_SAVE_DATA)
	  	     	{
    	  	     
  stopThisMeterPresent:
             	if (
			   	   			(port!=4 &&  copyCtrl[port].ifCopyLastFreeze==0 
               	    #ifdef LIGHTING 
               	     && LIGHTING_XL!=copyCtrl[port].cpLinkHead->protocol      //��·������
               	      && LIGHTING_DGM!=copyCtrl[port].cpLinkHead->protocol    //����������
               	       && LIGHTING_LS!=copyCtrl[port].cpLinkHead->protocol    //���նȴ�����
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
       	       	//������
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

               	//1)�ò����㳭��ɹ�����
               	if ((port!=4 && copyCtrl[port].numOfMeterAbort != 0xfe && ret==COPY_COMPLETE_SAVE_DATA)  //485�˿�������ǳ�����ϼ����յ���������
               	 	   || (port>=4 && ret==COPY_COMPLETE_SAVE_DATA)                                       //�ز��˿�������յ���������
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
                     
                  //�ò��������һ�γ���ɹ�ʱ���¼
                  meterStatisBearonTime.lastCopySuccessTime[0] = sysTime.minute/10<<4 | sysTime.minute%10;
                  meterStatisBearonTime.lastCopySuccessTime[1] = sysTime.hour/10<<4 | sysTime.hour%10;
                   	
                  if (port<4)
                  {
                   	//�������������485����ʧ��,���¼485����ָ�
                   	if (meterStatisExtranTime.mixed&0x2)
                   	{
                   	  //��¼�ն�485����ʧ���¼�
                      if (copy485Failure(copyCtrl[port].cpLinkHead->mp, 2, port+1)==TRUE)
                      {
                        meterStatisExtranTime.mixed &= 0xfd;
                      }
                   	}
                  }
               	}
       	          
     	         #ifdef PLUG_IN_CARRIER_MODULE 
     	          if (port>=4)   //�ز��˿�
     	          {
                  if ((1==carrierFlagSet.routeLeadCopy && carrierSendToMeter!=0) || 0==carrierFlagSet.routeLeadCopy)
                  {
                   #ifdef LM_SUPPORT_UT
                    if (0x55==lmProtocol)
                    {
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].cpLinkHead->mp, 3); //�������ʱ���޹���
                    }
                    else
                    {
                   #endif
                    
                      searchMpStatis(sysTime, &meterStatisExtranTimeS, copyCtrl[port].tmpCpLink->mp, 3); //�������ʱ���޹���
                       
                   #ifdef LM_SUPPORT_UT
                    }
                   #endif

                    //��ѯ������洢��Ϣ
                    if (carrierFlagSet.routeLeadCopy==1)  //ly,2012-01-09
                    {
  		               	queryMeterStoreInfo(copyCtrl[port].tmpCpLink->mp, meterInfo);
                    }
                    else
                    {
  		               	queryMeterStoreInfo(copyCtrl[port].cpLinkHead->mp, meterInfo);
  		             	}

                    //meterInfo,��0�ֽ� - �������(1-�������ܱ�,2-���౾�طѿر�,3-����Զ�̷ѿر�,
                    //           4-�������ܱ�,5-���౾�طѿر�,6-����Զ�̷ѿر�,7-485�����(���ݳ�����ȫ),8-�ص��û�
  		             	switch(meterInfo[0])
  		             	{
	             	   		case 1:  //�������ܱ�
	             	   		case 2:  //���౾�طѿر�
	             	   		case 3:  //����Զ�̷ѿر�
                        memcpy(tmpReadBuff, &copyCtrl[port].dataBuff[DATE_AND_WEEK_S], 7);
  		             	 		break;

	             	   		case 4:  //�������ܱ�
	             	   		case 5:  //���౾�طѿر�
	             	   		case 6:  //����Զ�̷ѿر�
                        memcpy(tmpReadBuff, &copyCtrl[port].dataBuff[DATE_AND_WEEK_T], 7);
  		             	 		break;

	             	   		case 7:  //�����
                        memcpy(tmpReadBuff, &copyCtrl[port].dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+DATE_AND_WEEK], 7);
             	 	     		break;
  		             	}
  		             
                    //�ز��˿ڵ��ܱ�ʱ�䳬���ж�
                   #ifdef LM_SUPPORT_UT
                    if (0x55==lmProtocol)
                    {
                      if (meterTimeOverEvent(copyCtrl[port].cpLinkHead->mp, tmpReadBuff, &meterStatisExtranTimeS.mixed, &meterStatisBearonTime))
                      {
                        //�洢������ͳ������(��ʱ���޹���)
                        saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
                      }
                    }
                    else
                    {
                   #endif 
                       
                     	if (meterTimeOverEvent(copyCtrl[port].tmpCpLink->mp, tmpReadBuff, &meterStatisExtranTimeS.mixed, &meterStatisBearonTime))
                     	{
                       	//�洢������ͳ������(��ʱ���޹���)
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
                 	//�������ʱ���޹���
                 	searchMpStatis(sysTime, &meterStatisExtranTime, copyCtrl[port].cpLinkHead->mp, 1);

                 	//��������㱾�γ���ĵ��ܱ�ʱ�估����������,�������ܱ�ʱ�䳬��ʱ���¼
                 	memcpy(tmpReadBuff,copyCtrl[port].dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD,LENGTH_OF_PARA_RECORD);                 
                 
                 	//���ܱ�ʱ�䳬���ж�
                 	if (meterTimeOverEvent(copyCtrl[port].cpLinkHead->mp, &tmpReadBuff[DATE_AND_WEEK], &meterStatisExtranTime.mixed, &meterStatisBearonTime))
                 	{
                   	//�洢������ͳ������(��ʱ���޹���)
                   	saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTime, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
                 	}
                   
               #ifdef PLUG_IN_CARRIER_MODULE
                }
               #endif

      	        //�洢������ͳ������(������)
      	       #ifdef LM_SUPPORT_UT
      	        if (port>=4 && 0x55!=lmProtocol)    //�ز�/���߶˿�
      	       #else
      	        if (port>=4)    //�ز�/���߶˿�
      	       #endif
      	        {
                  saveMeterData(copyCtrl[port].tmpCpLink->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisBearonTime, STATIS_DATA, 0xfe,sizeof(METER_STATIS_BEARON_TIME));

     	           	if (debugInfo&METER_DEBUG)
     	           	{
     	             	printf("������%d����ɹ�����:%d\n",copyCtrl[port].tmpCpLink->mp,meterStatisBearonTime.copySuccessTimes);
     	           	}
     	         	}
     	         	else            //485�˿�
     	         	{
                  saveMeterData(copyCtrl[port].cpLinkHead->mp, port+1, timeHexToBcd(sysTime), (INT8U *)&meterStatisBearonTime, STATIS_DATA, 0xfe,sizeof(METER_STATIS_BEARON_TIME));
                   
                  //2012-3-29,add
                  copyCtrl[port].cpLinkHead->copySuccess = TRUE;

     	           	if (debugInfo&METER_DEBUG)
     	           	{
     	             	printf("������%d����ɹ�����:%d\n",copyCtrl[port].cpLinkHead->mp,meterStatisBearonTime.copySuccessTimes);
     	           	}
     	         	}
              }
     	  	     
 	  	      #ifdef PLUG_IN_CARRIER_MODULE
 	  	       #ifdef LM_SUPPORT_UT
 	  	        if (port>=4 && 0x55!=lmProtocol)     //�ز��˿�
 	  	       #else
 	  	        if (port>=4)     //�ز��˿�
 	  	       #endif
 	  	       	{
     	  	     	//��������������
     	  	     	if (carrierFlagSet.routeLeadCopy==0)
     	  	     	{
       	  	      copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
    	     	       
	     	       		if (copyCtrl[port].ifCopyLastFreeze==2 || copyCtrl[port].ifCopyLastFreeze==3)
	     	       		{
	     	         		while(copyCtrl[port].tmpCpLink!=NULL)
	     	         		{
	  	     	 	   			//07��Լ�������ն���������ʼ�ɼ�
	     	 	       			if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
	     	 	       			{
	     	 	  	     			if (copyCtrl[port].ifCopyLastFreeze==2)
	     	 	  	     			{
         	               #ifndef DKY_SUBMISSION
     	 	  	       	   		//2012-09-28,Ϊ��Ӧ�����ػ�,�������е�û���ն����07�������if����,ͬʱȡ�����Ϊ0x55������,��Ϊ�������վ�˲����ر��鷳
     	 	  	       	   		//  �޸�Ϊ:���þ����û�����ģʽΪ0x55(��ʵʱ����)��0xaa(��ʵʱ��ʾֵ)�����Ƴ�����
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
     	 	  	       	   		//2012-09-28,Ϊ��Ӧ�����ػ�,�������е�û���ն����07�������if����,ͬʱȡ�����Ϊ0x55������,��Ϊ�������վ�˲����ر��鷳
     	 	  	       	     	//  �޸�Ϊ:���þ����û�����ģʽΪ0x55(��ʵʱ����)��0xaa(��ʵʱ��ʾֵ)�����Ƴ�����
  	       	              if (denizenDataType!=0x55 && denizenDataType!=0xaa 

 	 	  	       	          		//2012-08-31,Ϊ�������������������if����
 	 	  	       	           		//  �����С��Ŷ�Ϊ0�Ļ�,�ն���ɼ��ɹ�,�Ͳ��ٲɼ�������������
 	 	  	       	           		//  �����Ժ���ϸ�ִ�б�׼��07���Ĭ�ϴ���ʽΪֻ�ɼ��ն�������
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
    	     	         
    	     	     		//������һ�ն�������������ʼ��ʵʱ����
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
  	     	               	printf("�������㶳������������ʼ��һ�ն�������\n");
  	     	             	}
  	     	           
  	     	             	copyCtrl[port].round = 0;
                        copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
                        copyCtrl[port].ifCopyLastFreeze = 0;
                        while(copyCtrl[port].tmpCpLink!=NULL)
                        {
              	          if (copyCtrl[port].tmpCpLink->protocol==DLT_645_2007)
                 	       	{
              	            //����Ƿ��е��� �ն�������
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
      	     	           	printf("������һ�ն�������������ʼ��ʵʱ����\n");
      	     	         	}
                          
                        copyCtrl[port].round = 0;
  	     	         	 		copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
  	     	         	 		while(copyCtrl[port].tmpCpLink!=NULL)
  	     	         	 		{
  	     	         	   		copyCtrl[port].tmpCpLink->thisRoundCopyed = FALSE;
  	     	         	   		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
  	     	         	 		}
  	     	         	  
  	     	         	 		copyCtrl[port].tmpCpLink = copyCtrl[port].cpLinkHead;
  
                        //����ɼ�ʵʱ����,��λ����һ��δ�ɼ���ɵĲ����㿪ʼ�ɼ�
                       	if (copyCtrl[port].backMp>0)
                       	{
                   	 	   	while(copyCtrl[port].tmpCpLink!=NULL)
                   	 	   	{
                   	 	 	 		if (copyCtrl[port].tmpCpLink->mp==copyCtrl[port].backMp)
                   	 	 	 		{
                   	 	 	   		if (debugInfo&METER_DEBUG)
                   	 	 	   		{
                   	 	 	 	 			printf("(���ڳ���)��λ����һ��δ�ɼ���ɵĲ�����%d\n", copyCtrl[port].backMp);
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
   	  	     	 	   	 		printf("ĳ�����ɹ����ж�Ӧ�ó��Ŀ��:������%d���ֳ���ɹ�,������һ���\n",copyCtrl[port].tmpCpLink->mp);
   	  	     	 	   		}
   	  	     	 	   
   	  	     	 	   		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
       	  	     	 	}
       	  	      } 
       	  	    }
       	  	     	 
       	  	    tmpNode = copyCtrl[port].tmpCpLink;
     	  	   	}
     	  	   	else    //485�˿�
     	  	   	{
     	  	   #endif
     	  	    
     	  	     	tmpNode = copyCtrl[port].cpLinkHead;
    	  	     	copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
    	  	   	 	free(tmpNode);
    	  	   	 	//free(copyCtrl[port].dataBuff);
    	  	   	   
    	  	   		if (debugInfo&METER_DEBUG)
    	  	   	 	{
    	  	   	   	printf("�ͷ�ǰһ�����Ϣ\n");
    	  	   	 	}

  	     	     	//����ɼ���һ���ն�������,��Ҫ��λ��07��Լ��
  	     	     	if (copyCtrl[port].ifCopyLastFreeze==2)
  	     	     	{
  	     	       	while(copyCtrl[port].cpLinkHead!=NULL)
  	     	       	{
  	     	 	     		//07��Լ�������ն���������ʼ�ɼ�
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
  	   	        		printf("�ɼ���һ���ն���������λ��07��\n");
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
            	      
            	       	//�н���ģ��
            	       	if (ifHasAcModule==TRUE)
            	       	{
      	         	     	//����ǳ���������,ת����������
      	         	     	if (copyCtrl[port].ifCopyLastFreeze==0x1)
      	         	     	{
           	         	   	copyAcValue(port, 2, sysTime);
  
                          if (debugInfo&METER_DEBUG)
                          {
                            printf("CopyMeter:������,ת�������������ݲ�����\n");
                          }
                        }
                          
      	         	      //ת��ʵʱ����
      	         	      if (copyCtrl[port].ifCopyLastFreeze==0x0)
      	         	      {
           	         	   	copyAcValue(port, 1, timeHexToBcd(copyCtrl[port].currentCopyTime));
                                           
                          if (debugInfo&METER_DEBUG)
                          {
                            printf("CopyMeter:������,ת������ʵʱ���ݲ�����\n");
                          }
                        }
           	          }
           	  	     
           	  	      tmpNode = copyCtrl[port].cpLinkHead;
          	  	      copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
          	  	   	  free(tmpNode);
          	  	   	   	
          	  	   	  if (debugInfo&METER_DEBUG)
          	  	   	  {
          	  	   	    printf("�ͷ�ǰһ���(����)��Ϣ\n");
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
    	  	   	
    	  	   	if (tmpNode==NULL)    //������˿ڱ��ѳ���
    	  	   	{
    	         	if (debugInfo&METER_DEBUG)
    	         	{
    	           	printf("���˿��ѳ���\n");
    	         	}
    	        	 
    	       	#ifdef PLUG_IN_CARRIER_MODULE
    	         #ifdef LM_SUPPORT_UT
    	          if (port>=4 && 0x55!=lmProtocol)
    	         #else
    	         	if (port>=4)
    	         #endif
	        	 		{
	        	   		//���в�����δ����,��3��
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
	        	 	 	
        	 	 	 			//�����û�г����ı�,���Ժģ�������ѱ�
        	 	 	 			if (tmpData>0 && (carrierModuleType==CEPRI_CARRIER || localModuleType==CEPRI_CARRIER_3_CHIP))
        	 	 	 			{
        	 	 	   			//��ѱ�ʱ��
        	 	 	   			carrierFlagSet.searchMeter = 1;
        	 	 	   			carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0);

        	 	 	   			if (debugInfo&PRINT_CARRIER_DEBUG)
        	 	 	   			{
        	 	 	  	 			printf("��û�����ı�,��ʼ�ѱ�,��ֹʱ��:%d-%d-%d %d:%d:%d\n",carrierFlagSet.foundStudyTime.year,carrierFlagSet.foundStudyTime.month,
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
  	     	 	   	     		printf("����һ�ֺ��ж���Щ������δ����:������%d���ֳ���ɹ�,������һ���\n",copyCtrl[port].tmpCpLink->mp);
  	     	 	       		}

  	     	 	       		copyCtrl[port].tmpCpLink = copyCtrl[port].tmpCpLink->next;
 	  	     	     		}
     	  	     	      
  	     	         	//ȫ�������㶼����,���Խ������γ���
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
	   	                 	printf("��ʼ��%d�ֳ���\n",copyCtrl[port].round);
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
				       				printf("��������ʧ�ܴ���=%d\n",copyCtrl[port].portFailureRounds);
				     				}
					 					if (copyCtrl[port].portFailureRounds>=3)
					 					{
					   					//��¼485������Ϸ���
					   					happenRecovery485(1);
					 					}
				   				}
				   				else
				   				{
					 					if (copyCtrl[port].portFailureRounds>0)
					 					{
					   					copyCtrl[port].portFailureRounds = 0;
					 					}
					 
					 					//��¼485������ϻָ�
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
	  	   	         	printf("�˿�%d�������ݳ������,��ʼ��ʵʱ����\n",port+1);
	  	   	       	}
	  	   	     
	  	   	       	copyCtrl[port].ifCopyLastFreeze = 0;

                  copyCtrl[port].cpLinkHead = initPortMeterLink(port);
                  if (copyCtrl[port].cpLinkHead==NULL)
                  {
             	     	copyCtrl[port].meterCopying = FALSE;   //ֹͣ����

                    copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);
               	  
             	     	if (debugInfo&METER_DEBUG)
             	     	{
               	      printf("�����������ݺ�˿�:%d�ޱ�ɳ�\n",port+1);
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
      	  	   	      printf("�˿�%d��һ���ն������ݳ������,��ʼ��ʵʱ����\n",port+1);
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
              	      //�н���ģ��
              	      if (ifHasAcModule==TRUE)
              	      {
      	         	      copyAcValue(port, 1, timeHexToBcd(sysTime));  //��Ϊ����Ҫ��ʼ�����γ����ʱ��ΪsysTime
                                         
                        if (debugInfo&METER_DEBUG)
                        {
                          printf("CopyMeter:������(����07��һ���ն������ݺ�),ת������ʵʱ���ݲ�����\n");
                        }
           	          }
           	  	     
           	  	      tmpNode = copyCtrl[port].cpLinkHead;
          	  	      copyCtrl[port].cpLinkHead = copyCtrl[port].cpLinkHead->next;
          	  	   	  free(tmpNode);
          	  	   	   	
          	  	   	  if (debugInfo&METER_DEBUG)
          	  	   	  {
          	  	   	    printf("������(����07��һ���ն������ݺ�),�ͷ�ǰһ���(����)��Ϣ\n");
          	  	   	  }
      	  	   	    }
                     
                    if (copyCtrl[port].cpLinkHead==NULL)
                    {
               	      copyCtrl[port].meterCopying = FALSE;   //ֹͣ����
  
                      copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, 20);
                 	  
               	      if (debugInfo&METER_DEBUG)
               	      {
                 	     	printf("������һ���ն������ݺ�˿�:%d�ޱ�ɳ�\n",port+1);
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
			               		showInfo("���߶˿ڳ������!");
					 	 	        }
					 	 	        else
					 	 	       	{
			               		showInfo("����ͨ�ſڳ������!");
						          }
						        }
						       #endif
                  #endif
                     
           	 	     	//2012-07-31,�����ִ�����λ����ʼ���˿ڳ���ʱ
           	 	     	//#ifdef PULSE_GATHER
           	 	     	//  for(pulsei=0;pulsei<NUM_OF_SWITCH_PULSE;pulsei++)
           	 	     	//  {
           	 	     	//  	  if (pulse[pulsei].ifPlugIn == TRUE)
           	 	     	//  	  {
                    //       memset(visionBuff,0xee,LENGTH_OF_ENERGY_RECORD);
                    //       memset(reqBuff,0xee,LENGTH_OF_REQ_RECORD);
                    //       memset(paraBuff,0xee,LENGTH_OF_PARA_RECORD);
  
                    //       //ת��
                    //       covertPulseData(pulsei, visionBuff, reqBuff, paraBuff);
                    //  		 	
                    //  		 	//����ʾֵ
                    //  		 	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                    //                     visionBuff, PRESENT_DATA, ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
                    //  	   
                    //  		 	//�������
                    //  		 	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                    //                     paraBuff, PRESENT_DATA, PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
                    //
                    //  	    //��������
                    //  		 	saveMeterData(pulse[pulsei].pn, port, timeHexToBcd(copyCtrl[port].currentCopyTime), \
                    //                     reqBuff, PRESENT_DATA, REQ_REQTIME_DATA,LENGTH_OF_REQ_RECORD);
           	 	     	//  	  }
           	 	     	//  }
           	 	     	//#endif
       	 	       
      	  	   	    if (debugInfo&METER_DEBUG)
      	  	   	    {
      	  	   	      if (port==4)
      	  	   	      {
      	  	   	        printf("�˿�31�������\n");
      	  	   	      }
      	  	   	      else
      	  	   	      {
      	  	   	        printf("�˿�%d�������\n",port+1);
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
					 	 	             	printf("��Ѹ�︴λ��ȡ״̬��־\n");
					 	 	           	}
					 	 	         	}	
					 	 	       	}
					 	 	     
					 	 	     #ifdef LM_SUPPORT_UT
					 	 	     	}
					 	 	     #endif
					 	 	    #endif

      	  	 	      //�ϴγ���ʱ��(BCD��)
                    copyCtrl[port].lastCopyTime.second = copyCtrl[port].currentCopyTime.second/10<<4 | copyCtrl[port].currentCopyTime.second%10;;
                    copyCtrl[port].lastCopyTime.minute = copyCtrl[port].currentCopyTime.minute/10<<4 | copyCtrl[port].currentCopyTime.minute%10;;
                    copyCtrl[port].lastCopyTime.hour   = copyCtrl[port].currentCopyTime.hour/10<<4 | copyCtrl[port].currentCopyTime.hour%10;;
                    copyCtrl[port].lastCopyTime.day    = copyCtrl[port].currentCopyTime.day/10<<4 | copyCtrl[port].currentCopyTime.day%10;;
                    copyCtrl[port].lastCopyTime.month  = copyCtrl[port].currentCopyTime.month/10<<4 | copyCtrl[port].currentCopyTime.month%10;;
                    copyCtrl[port].lastCopyTime.year   = copyCtrl[port].currentCopyTime.year/10<<4 | copyCtrl[port].currentCopyTime.year%10;;
     	 	             
     	 	             
   	 	              //���˿�ʵʱ������λ
	 	 	            #ifdef PLUG_IN_CARRIER_MODULE
	 	 	             	if (ret==COPY_COMPLETE_SAVE_DATA && port<4)
	 	 	            #else
	 	 	             	if (ret==COPY_COMPLETE_SAVE_DATA)
	 	 	            #endif
	 	 	             	{
	 	 	               	if (debugInfo&METER_DEBUG)
	 	 	               	{
	 	 	                 	printf("copyMeter:���˿�%dʵʱ����׼����λ, ifRealBalance=%d,ret=%d\n", port+1, copyCtrl[port].ifRealBalance, ret);
	 	 	               	}  

	 	 	               	//ly,2012-04-26ȡ������ж�
	 	 	               	//if (copyCtrl[port].ifRealBalance==0)
	 	 	               	//{
	 	 	               	if (debugInfo&METER_DEBUG)
	 	 	               	{
	 	 	                 	printf("copyMeter:���˿�%dʵʱ������λ\n",port+1);
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
    	  	   	   	if (carrierFlagSet.routeLeadCopy==1)    //·����������
    	  	   	   	{
	  	   	   	 	 		//���ʵʱ���ݶ������˵Ļ�,�͸���·�ɳ����ɹ�
	  	   	   	 	 		switch(copyCtrl[port].tmpCpLink->flgOfAutoCopy)
	  	   	   	 	 		{
	  	   	   	 	   		case 6:    //ʵʱ�����ѳ���
	  	   	   	 	   		case 8:    //2012-08-28,Ϊ�����ն������ݶ�ֱ�ӻظ�����ɹ����ӵ����=8��ֵ
  	       	   	        copyCtrl[port].tmpCpLink->thisRoundCopyed = TRUE;   //���ֳ���ɹ�
                        copyCtrl[port].tmpCpLink->flgOfAutoCopy = 0;
                     
                        //2012-3-29,����μԴ���ֵļ�����·����������ʱͳ�Ƴ���ɹ���������ȷ,
                        //          ԭ���������ټ�����һ��
                        copyCtrl[port].tmpCpLink->copySuccess = TRUE;
  	       	   	     
  	       	   	        if (debugInfo&METER_DEBUG)
  	       	   	        {
                          printf("������%d���ֳ���ɹ�\n", copyCtrl[port].tmpCpLink->mp);
                        }

	  	       	   	     	//����вɼ�����ַ,Ҫ�ж��ǲ��Ǳ��ɼ����»�������485��
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
    	  	   	   	 	   		//�����ɹ�
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
    	  	   	   	 	   	 		printf("·����������:�������ɹ�\n");
    	  	   	   	 	   		}
    	  	   	   	 	 		}
    	  	   	   	 	 		else
    	  	   	   	 	 		{
    	  	   	   	 	   		if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	   	   	 	   		{
    	  	   	   	 	   	 		printf("·����������:���ɼ����±���485���γ����ɹ�,����������485��û�г����ɹ�\n");
    	  	   	   	 	   		}
    	  	   	   	 	 		}
                        goto breakPoint;
                        break;
                 
	  	   	   	 	   		case 7:    //��������굫����Ч���ݵĻ�,�͸���·�ɳ���ʧ��
	  	   	   	 	   		case 9:    //�ն��᳭���Ͳ��ٳ�������������,��Ҳ����·�ɳ���ʧ��,2013-12-25,add
  	  	   	   	 	     	//2013-12-24,���ݶ���ϣ���Ļظ�,���Ը�·��ģ��س���ʧ��
  	  	   	   	 	     	//if (carrierModuleType==TC_CARRIER)  //ly,2012-01-09
  	  	   	   	 	     	//{
    	  	   	   	 	 		//  if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	   	   	 	 		//  {
    	  	   	   	 	 		//  	 printf("·����������:(����)������ʧ��,���ǲ���ģ����ʧ�ܱ�־\n");
    	  	   	   	 	 		//  }
  	  	   	   	 	     	//}
  	  	   	   	 	     	//else
  	  	   	   	 	     	//{
    	  	   	   	 	 		//����ʧ��
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
	  	   	   	 	   	   		printf("·����������:������ʧ��\n");
	  	   	   	 	     		}
	  	   	   	 	     		//}
                        copyCtrl[port].tmpCpLink->flgOfAutoCopy = 0;
                        goto breakPoint;
                         
                        break;
                    }
    	  	   	    }
    	  	   	   	else                                     //��������������
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
                  showInfo("�ն����ڳ���...");
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
    	  	   	     	printf("��ʼ����һ���\n");
    	  	   	   	}
    	  	   	 	}
    	  	   	 #endif
    	  	  	}
    	  	 	}
   	      }
   	      else
   	      {
            //��ʱ����
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
                      printf("�˿�%d���ճ�ʱ!\n",port+1);
                    }
                  }
                  else
                  {
                    //if (debugInfo&PRINT_CARRIER_DEBUG)
                    //{
                    //  printf("�˿�%dΪ���ƽ��볭���ж����õĳ�ʱ!\n", port+1);
                    //}
                  }
                 #endif
                }
                else
                {
                  if (debugInfo&METER_DEBUG)
                  {
                    printf("�˿�%d���ճ�ʱ!\n", port+1);
                  }
                }
   	    	   	}
   	    	   	 
     	  	   	if (port>=4)
     	  	   	{
   	    	   	 #ifdef PLUG_IN_CARRIER_MODULE
   	    	   	 	//��������������
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
	    usleep(500);   //ly,2011-07-25,add,��top�۲�,dlzd��CPUռ���ʶ���ʱ���99%,���ϱ�����ռ���������½�
	       
	    continue;
    }
  }   //while(1)
}


/**************************************************
��������:queryIfInCopyPeriod
��������:�鿴�Ƿ�������ĳ���ʱ����
���ú���:
�����ú���:
�������:INT8U portNum,�˿�
�������:
����ֵ��״̬
�޸���ʷ:
    2013-12-31,�޸�ͬһСʱ���ʱ���ж�ʧ�ܵĴ���
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
   		  //���һ��ʱ����ʼ�ͽ���ʱ���Сʱ��ͬһ��Сʱ��
   		  (sysTime.hour==tmpStartHour && sysTime.minute>=tmpStartMin && sysTime.hour==tmpEndHour && sysTime.minute<tmpEndMin)
   		  
   		   //�����ʼ�ͽ�����Сʱ������һ��Сʱ,��ǰʱ���Сʱ������ʼʱ�ε�Сʱ�ҵ�ǰʱ��ķ��Ӵ��ڵ�����ʼʱ�εķ�������
   		   ||(tmpStartHour!=tmpEndHour && sysTime.hour==tmpStartHour && sysTime.minute>=tmpStartMin)

   		    //�����ʼ�ͽ�����Сʱ������һ��Сʱ,�����Ѿ����˽���ʱ�ε�Сʱ����
   		    ||(tmpStartHour!=tmpEndHour && sysTime.hour==tmpEndHour && sysTime.minute<=tmpEndMin)
   		  
   		     //�������ʼ�ͽ������м��Сʱ
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
��������:forwardReceive
��������:ת��ֱ�ӶԵ��ܱ�������������մ���
���ú���:
�����ú���:
�������:INT8U portNum,...
�������:
����ֵ��״̬
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
       //����У����Ұ��ֽڽ���������м�0x33����
       checkSum = 0;
       for(tmpi=20; tmpi<copyCtrl[portNum].pForwardData->recvFrameTail-2; tmpi++)
       {
         checkSum += copyCtrl[portNum].pForwardData->data[tmpi];
         if (tmpi>29)
         {
           copyCtrl[portNum].pForwardData->data[tmpi] -= 0x33;  //���ֽڽ��ж��������0x33����
         }
       }
            
       //���У�����ȷ,ִ��meterInput����
       if (checkSum == copyCtrl[portNum].pForwardData->data[copyCtrl[portNum].pForwardData->recvFrameTail-2])
       {
     	 if (debugInfo&METER_DEBUG)
     	 {
     	   printf("ֱ�ӳ������ظ���ȷ֡\n");
     	 }
     	   	
     	 copyCtrl[portNum].pForwardData->nextBytesTime = sysTime;
      	 copyCtrl[portNum].pForwardData->forwardResult = RESULT_HAS_DATA;          
       }
     }
   }
}

#ifdef PLUG_IN_CARRIER_MODULE
/**************************************************
��������:threadOfCarrierReceive
��������:ttys5(�ز��ӿ�)���մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
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
     
     //2011-05-31,�ĳ�ÿ�ν���30,�������̵Ľ��ջ�������,���˺�����˶�������̵Ŀ���
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
       if (copyCtrl[4].pForwardData!=NULL)  //ת��
       {
       	 if (copyCtrl[4].pForwardData->ifSend == TRUE)
       	 {
       	   if (debugInfo&METER_DEBUG)
       	   {
             printf("%02d-%02d-%02d %02d:%02d:%02d LM Rx(ת��):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
             for(i=0;i<recvLen;i++)
             {
      	       printf("%02x ",tmpBuf[i]);
             }
             printf("\n");
       	   }
  
       	   if (copyCtrl[4].pForwardData->fn==1)
       	   {
       	     //���յ��ظ����ݺ�ȴ�����,���Ƿ��к����ֽ�
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
         printf("����ͨ��ģ��:���ν��յĵڶ��δ���,���δ���%d�ֽ�\n", libLen);
       }
       
       ret = gdw3762Receiving(&tmpBuf[tmpLen], &libLen);
     }
     
     switch(ret)
     {
     	  case RECV_DATA_CORRECT:   //���յ���ȷ��һ֡
     	  	switch(carrierAfn)
     	  	{
     	  		case ACK_OR_NACK_3762:   //ȷ��/����
     	  		 	switch(carrierFn)
     	  		 	{
     	  		 	 	case 1:   //ȷ��
                  switch(carrierFlagSet.cmdType)
                  {
                  	case CA_CMD_HARD_RESET:
	                    //�Ա���ͨ��ģ��Ӳ����ʼ����ȷ��
	                    if (carrierFlagSet.hardwareReest == 1)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.hardwareReest = 2;
                       
                        carrierFlagSet.cmdTimeOut  = nextTime(sysTime, 0, 5);
     	  		 	 	  		 
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	  printf("ȷ��Ӳ����ʼ��\n");
     	  		 	 	  		  }
     	  		 	 	  	  }
     	  		 	 	  	  break;

                  	case CA_CMD_SR_PARA_INIT:    //ɣ��ģ�������������ȷ��                    
                      if (carrierFlagSet.paraClear==0 && carrierModuleType==SR_WIRELESS)
                      {
                    	  carrierFlagSet.paraClear = 1;
                    	  carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 5);
     	  		 	 	  		
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	   printf("SRȷ�ϲ�������ʼ��\n");
     	  		 	 	  		  }
                      }
                      break;
                    
                    case CA_CMD_CLEAR_PARA:    //��������ʼ��
                      //ǿ�Ʋ�������ʼ����ȷ��
                      if (debugInfo & DELETE_LOCAL_MODULE)
                      {
                    	   debugInfo &= ~DELETE_LOCAL_MODULE;
                    	 
     	  		 	 	  		   if (debugInfo&METER_DEBUG)
     	  		 	 	  		   {
     	  		 	 	  		 	   printf("ȷ��ǿ�Ʋ�������ʼ��\n");
     	  		 	 	  		   }
                      }

            	        if (carrierFlagSet.checkAddrClear==1)
            	        {
            	        	carrierFlagSet.checkAddrClear = 0;

            	      	  if (debugInfo&PRINT_CARRIER_DEBUG)
            	      	  {
            	      		  printf("ȷ�ϱȽϼ�������ģ�鵵����ַ��һ��ʱ������ַ\n");
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
     	  		 	 	  		 	      printf("RLȷ��ɾ�����дӽڵ�\n");
     	  		 	 	  		 	    }
     	  		 	 	  		 	    else
     	  		 	 	  		 	    {
     	  		 	 	  		 	      printf("ȷ�ϲ�������ʼ��\n");
     	  		 	 	  		 	    }
     	  		 	 	  		 	  }
     	  		 	 	  		 	  
     	  		 	 	  		 	  carrierFlagSet.synSlaveNode   = 0;
     	  		 	 	  		 	  carrierFlagSet.querySlaveNode = 0;
     	  		 	 	  		 	  carrierFlagSet.init = 0;
     	  		 	 	  		  }
     	  		 	 	  	  }
                      break;
                    
                    case CA_CMD_SET_MAIN_NODE:    //�������ڵ��ȷ��
                      if (carrierFlagSet.setMainNode==1)
                      {
                    	   carrierFlagSet.setMainNode = 2;
                    	   carrierFlagSet.mainNode = 0;     //Ҫ���²�ѯ���ڵ��ַ
                      }
                      break;
     	  		 	 	  	
     	  		 	 	  	case CA_CMD_SR_CHECK_NET:
     	  		 	 	  	  if (carrierFlagSet.startStopWork==2)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.startStopWork = 0;
     	  		 	 	  		   
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	  printf("SRģ��ȷ������Ϊ������֤ģʽ\n");
     	  		 	 	  		  }
     	  		 	 	  	  }
     	  		 	 	  		break;
     	  		 	 	  	
     	  		 	 	  	case CA_CMD_ADD_SLAVE_NODE:
     	  		 	 	  	  //������Ӵӽڵ�
     	  		 	 	  	  if (carrierFlagSet.synSlaveNode==2)
     	  		 	 	  	  {
     	  		 	 	  		  copyCtrl[4].tmpCpLink = copyCtrl[4].tmpCpLink->next;
     	  		 	 	  		  carrierFlagSet.synMeterNo++;
     	  		 	 	  	  }
     	  		 	 	  		break;
     	  		 	 	    
     	  		 	 	    case CA_CMD_SET_WORK_MODE:
     	  		 	 	  	  //���ñ���ͨ��ģ�鹤��״̬��ȷ��
     	  		 	 	  	  if (carrierFlagSet.workStatus==1)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.workStatus = 2;    //����ģʽΪ����
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("����ͨ��ģ��ȷ�����ù���Ϊ����ģʽ\n");
                        }
                      }
       	  		 	 	  	
       	  		 	 	  	if (carrierFlagSet.searchMeter==1)
       	  		 	 	  	{
       	  		 	 	  		carrierFlagSet.searchMeter = 2;
       	  		 	 	  		  
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("����ͨ��ģ��ȷ�����ù���Ϊ����ע��ģʽ\n");
       	  		 	 	  		}
       	  		 	 	    }
       	  		 	 	  	
       	  		 	 	  	if (carrierFlagSet.searchMeter==5)
       	  		 	 	  	{
       	  		 	 	  		carrierFlagSet.searchMeter = 6;
       	  		 	 	  		
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
     	  		 	 	  		    printf("�ز�ģ��ȷ���ѱ�����ù���ģʽΪ���������\n");
       	  		 	 	  		}
       	  		 	 	    }
     	  		 	 	    	break;
     	  		 	 	    	
     	  		 	 	    case CA_CMD_ACTIVE_REGISTER: //�����ز��ӽڵ�����ע��
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
     	  		 	 	  		    printf("ģ��ȷ������ע������\n");
     	  		 	 	  		  }
                        carrierFlagSet.ifSearched = 1;
                      }
     	  		 	 	    	break;
     	  		 	 	    
     	  		 	 	    case CA_CMD_RESTART_WORK:    //�ù���״̬Ϊ������ȷ��
     	  		 	 	  	  if (carrierFlagSet.workStatus==2)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 3;   //����״̬Ϊ�����ĳ���
                        
                        copyCtrl[4].meterCopying = TRUE;
                        copyCtrl[4].copyContinue = FALSE;

                        copyCtrl[4].tmpCpLink = copyCtrl[4].cpLinkHead;
       	  		 	 	      while(copyCtrl[4].tmpCpLink!=NULL)
       	  		 	 	      {
       	  		 	 	   	 		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	   	 		{
       	  		 	 	   	 		  printf("���������%d�Զ������־\n", copyCtrl[4].tmpCpLink->mp);
       	  		 	 	   	 		}
       	  		 	 	   	 		copyCtrl[4].tmpCpLink->flgOfAutoCopy = 0;
       	  		 	 	   	 	 
       	  		 	 	   	 	  //2013-10-24,������������Ӵ���
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
                        //��һ�������������Ƶ��ĩ�˵��ƿ�����ʱ��
                        //  2015-05-25,��Ϊ3���Ӻ�����
                        carrierFlagSet.searchLddtTime = nextTime(sysTime, 3, 0);
                        
                        //2015-11-09,�������ߵĵ��ƿ�����ʱ��Ϊ3���Ӻ�
                        carrierFlagSet.searchOffLineTime = nextTime(sysTime, 3, 0);
                        
                        //2015-11-12,�������ƿ�����״̬ʱ��Ϊ3���Ӻ�
                        carrierFlagSet.searchLddStatusTime = nextTime(sysTime, 3, 0);
                        
                       #endif
                        
                        carrierFlagSet.chkNodeBeforeCopy = 0;    //2013-12-30,add
                        
                        //ȷ���´γ�ʱ��
                        initCopyItem(4, 0);
                        
                        if (menuInLayer==0)
                        {
                          defaultMenu();
                        }
                        
                        //2012-08-28,add this line
                        copyCtrl[4].lastCopyTime = timeHexToBcd(sysTime);
                        
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ����������:�˿�31��ʼ����\n");
                        }
                        
                        #ifdef PLUG_IN_CARRIER_MODULE
                         #ifndef MENU_FOR_CQ_CANON
                           showInfo("�ն����ڳ���...");
                         #endif
                        #endif
                      }
       	  		 	 	  	
       	  		 	 	  	if (carrierFlagSet.searchMeter==3) //ģ��ȷ�������ѱ�����
       	  		 	 	  	{
       	  		 	 	  		carrierFlagSet.searchMeter = 4;
       	  		 	 	  		  
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("ģ��ȷ����������ע������\n");
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
     	  		 	 	  		    printf("�ز�ģ��ȷ���ѱ��������������\n");
     	  		 	 	  		  }
     	  		 	 	  		}
     	  		 	 	  		
     	  		 	 	  		if (carrierFlagSet.batchCopy==1)
     	  		 	 	  		{
     	  		 	 	  			carrierFlagSet.batchCopy = 2;
     	  		 	 	  			
     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("���Ժ�ز�ģ��ȷ�������ֳ�����\n");
     	  		 	 	  		  }
     	  		 	 	  		  
                        copyCtrl[4].meterCopying = TRUE;
                        copyCtrl[4].copyContinue = FALSE;

                        //ȷ���´γ�ʱ��
                        //initCopyItem(4, 0);
                        
                        if (menuInLayer==0)
                        {
                          defaultMenu();
                        }
                        
                        #ifdef PLUG_IN_CARRIER_MODULE
                         #ifndef MENU_FOR_CQ_CANON
                           showInfo("�ն����ڳ���...");
                         #endif
                        #endif
     	  		 	 	  		}
                      break;
                    
                    case CA_CMD_PAUSE_WORK:      //��ͣ��ǰ��������
     	  		 	 	  	  if (carrierFlagSet.startStopWork==2)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.startStopWork = 0;
     	  		 	 	  		   
     	  		 	 	  		  if (debugInfo&METER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		 	  printf("����ͨ��ģ��ȷ�ϳ�ʼ����ʱֹͣ��������\n");
     	  		 	 	  		  }
     	  		 	 	  	  }

     	  		 	 	  	  if (carrierFlagSet.workStatus==4)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //����״̬Ϊ��ͣ����
                        
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ����ͣ����\n");
                        }
                        
                        if (carrierFlagSet.synSlaveNode==1 && copyCtrl[4].meterCopying==TRUE)
                        {
                        	copyCtrl[4].meterCopying = FALSE;
                        	
                         	//ly,2012-01-10,���Ӷ��ŵĴ���
                         	if (carrierModuleType==TC_CARRIER)
                         	{
                         	  copyCtrl[5].meterCopying = FALSE;
                         	  copyCtrl[6].meterCopying = FALSE;
                         	}
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
                            printf("·����������:Ϊͬ�����ֹͣ����\n");
                          }
                          carrierFlagSet.reStartPause = 0;
                        }
                        
                        if (carrierFlagSet.searchMeter==1 && copyCtrl[4].meterCopying==TRUE)
                        {
                        	copyCtrl[4].meterCopying = FALSE;

                         	//ly,2012-01-10,���Ӷ��ŵĴ���
                         	if (carrierModuleType==TC_CARRIER)
                         	{
                         	  copyCtrl[4].meterCopying = FALSE;
                         	  copyCtrl[5].meterCopying = FALSE;
                         	}
                            
                          if (debugInfo&PRINT_CARRIER_DEBUG)
                          {
                            printf("·����������:Ϊִ���ѱ������ֹͣ����\n");
                          }
                          
                          carrierFlagSet.reStartPause = 0;
                        }
                      }

     	  		 	 	  	  if (carrierFlagSet.workStatus==7)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //����״̬Ϊ��ͣ����
                        
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ����ͣ����(������ʱ��)\n");
                        }
                        
                      	copyCtrl[4].meterCopying = FALSE;
                      	
                       	//ly,2012-01-10,���Ӷ��ŵĴ���
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
                        carrierFlagSet.workStatus = 5;   //����״̬Ϊ��ͣ����
                        
                        carrierFlagSet.searchLddt = 1;
                        
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);

                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ����ͣ����(������ⳬʱ��ĩ�˵��ƿ�����)\n");
                        }
     	  		 	 	  	  }
     	  		 	 	  	  if (carrierFlagSet.workStatus==9)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //����״̬Ϊ��ͣ����
                        
                        carrierFlagSet.searchOffLine = 1;
                        
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);

                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ����ͣ����(��⵽�г�������ֵ���ƿ�����)\n");
                        }
     	  		 	 	  	  }
     	  		 	 	  	  if (carrierFlagSet.workStatus==10)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 5;   //����״̬Ϊ��ͣ����
                        
                        carrierFlagSet.searchLddStatus = 1;
                        
                       	copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 20);

                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ����ͣ����(��ѯ��·���ϵ絥�ƿ�����״̬)\n");
                        }
     	  		 	 	  	  }
     	  		 	 	  	 #endif
     	  		 	 	  	 
       	  		 	 	  	if (carrierFlagSet.searchMeter==4) //ģ��ȷ��ֹͣ�ѱ�����
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
       	  		 	 	  		  printf("ģ��ȷ��ֹͣ�ѱ�����\n");
       	  		 	 	  		}
       	  		 	 	  		  
       	  		 	 	  		if (menuInLayer==0)
       	  		 	 	  		{
       	  		 	 	  		  defaultMenu();
       	  		 	 	  		}
       	  		 	 	  		
       	  		 	 	  		//5���Ӻ�ʼ����
       	  		 	 	  		copyCtrl[4].nextCopyTime = nextTime(sysTime, 0, 5);
       	  		 	 	    }
                    	break;
                    
                    case CA_CMD_RESTORE_WORK:    //�ָ���������
     	  		 	 	  	  if (carrierFlagSet.workStatus==6)
     	  		 	 	  	  {
                        carrierFlagSet.workStatus = 3;   //����״̬Ϊ�ָ�����,�ָ�����ǳ����״̬
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                          printf("�ز�ģ��ȷ�ϻָ�����\n");
                        }
                      }
                      break;
                    
                    case CA_CMD_SET_MODULE_TIME:  //����ʱ������
                      carrierFlagSet.setDateTime = 1;
                      
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("����ͨ��ģ��(�ز�/����)ģ��ȷ�����õ�ʱ��\n");
                      }                      
                      break;
                    
                    case CA_CMD_UPDATE_ADDR:     //��Ѹ����µ�������
                    	carrierFlagSet.querySlaveNode = 2;
                    	
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("��Ѹ������ģ��ȷ�ϸ��µ�������\n");
                      }
                    	break;
                    	
                    case CA_CMD_BATCH_COPY:     //���Ժ�ֳ�����
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        printf("���Ժ�ز�ģ��ȷ���ֳ�����\n");
                      }
                      
                      carrierFlagSet.batchCopy = 1;
                    	break;
                    
                    case CA_CMD_BROAD_CAST:    //�㲥����
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
     	  		 	 	  	  		printf("ģ��ȷ�Ϲ㲥����,ģ��Ҫ��ȴ�ʱ��Ϊ%d��,", tmpBuf[2]|tmpBuf[3]<<8);
     	  		 	 	  	  	}
     	  		 	 	  	  	
     	  		 	 	  	  	if (0==pnGate.boardcastWaitGate)
     	  		 	 	  	  	{
     	  		 	 	  	  	  carrierFlagSet.broadCastWaitTime = nextTime(sysTime, 0, tmpBuf[2]|tmpBuf[3]<<8);
                          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, tmpBuf[2]|tmpBuf[3]<<8);

     	  		 	 	  	  	  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  	  	  {
     	  		 	 	  	  		  printf("��ģ��Ҫ��ֵ�ȴ�\n");
     	  		 	 	  	  	  }
     	  		 	 	  	  	}
     	  		 	 	  	  	else
     	  		 	 	  	  	{
     	  		 	 	  	  	  if ((tmpBuf[2]|tmpBuf[3]<<8)<(pnGate.boardcastWaitGate*60))    //2015-10-26,�������ж�
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
     	  		 	 	  	  		    printf("����վ�����˸���ֵ,�ȴ�ʱ��Ϊ%d��\n", pnGate.boardcastWaitGate);
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
   	  		 	 	  		 	       printf("RLģ��:ȷ����������\n");
   	  		 	 	  		       }
   	  		 	 	  		 
   	  		 	 	  		       carrierFlagSet.setupNetwork = 2;
   	  		 	 	  		     }
     	  		 	 	  	  }
     	  		 	 	  	
     	  		 	 	  	  //��ʼѧϰ·������ȷ��
     	  		 	 	  	  if (carrierFlagSet.studyRouting==1)
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.studyRouting = 2;    //ģ�鿪ʼ·��ѧϰ
     	  		 	 	  	  }
     	  		 	 	  	
     	  		 	 	  	  if (carrierFlagSet.studyRouting==3)   //����ֹͣѧϰȷ��
     	  		 	 	  	  {
     	  		 	 	  		  carrierFlagSet.studyRouting = 0;    //ģ����ֹͣѧϰ
     	  		 	 	  		 
     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("ģ��ȷ��ֹͣѧϰ����\n");
     	  		 	 	  		  }
     	  		 	 	  	  }

     	  		 	 	  	  //�����ŵ���(SR,FC,etc.)
     	  		 	 	  	  if (carrierFlagSet.setPanId==0)
     	  		 	 	  	  {
     	  		 	 	  	  	carrierFlagSet.setPanId = 1;

     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
     	  		 	 	  		    printf("ģ��ȷ�������ŵ�������\n");
     	  		 	 	  		  }
     	  		 	 	  	  }     	  		 	 	  	  
       	  		 	 	  	break;
     	  		 	 	  }
     	  		 	 	  break;
     	  		 	 	  	
   	  		 	 	  case 2:   //����
   	  		 	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	{
   	  		 	 	  	  printf("����,����״̬��:%d\n",tmpBuf[0]);
   	  		 	 	  	}
   	  		 	 	  	
   	  		 	 	  	//������Ӵӽڵ�
   	  		 	 	  	if (carrierFlagSet.synSlaveNode==2)
   	  		 	 	  	{
   	  		 	 	  		 copyCtrl[4].tmpCpLink = copyCtrl[4].tmpCpLink->next;
   	  		 	 	  		 carrierFlagSet.synMeterNo++;
   	  		 	 	  	}
   	  		 	 	  	
   	  		 	 	  	switch(tmpBuf[0])
   	  		 	 	    {
   	  		 	 	    	case 7:  //��Ų�����(����,����)
   	  		 	 	  	    if (carrierFlagSet.querySlaveNode==1)
   	  		 	 	  	    {
 	                       carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
 	                   
 	                       if (debugInfo&PRINT_CARRIER_DEBUG)
 	                       {
 	                         printf("�ز�ģ��ӽڵ��б�\n");
 	                         tmpSalveNode = carrierSlaveNode;
 	                         while(tmpSalveNode!=NULL)
 	                         {
   	  		 	 	  	           printf("�ӽڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpSalveNode->addr[5],tmpSalveNode->addr[4],tmpSalveNode->addr[3],tmpSalveNode->addr[2],tmpSalveNode->addr[1],tmpSalveNode->addr[0]);
   	  		 	 	  	           tmpSalveNode = tmpSalveNode->next;
 	                         }
 	                       }
   	  		 	 	  	    }
   	  		 	 	  	    break;
   	  		 	 	  	    
   	  		 	 	  	  case 253:  //ly,2011-05-20,�ڴ�������������΢����ʱ�����㳭ʱ��253����,����΢����ʦ�з����������ʱ��λģ��
   	  		 	 	  	  	carrierFlagSet.numOf253++;
   	  		 	 	  	  	if (carrierModuleType==MIA_CARRIER)
   	  		 	 	  	  	{
   	  		 	 	  	  		if (carrierFlagSet.numOf253>10)
   	  		 	 	  	  		{
   	  		 	 	  	  		  ifReset = TRUE;
   	  		 	 	  	  		
   	  		 	 	  	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  	  		  {
   	  		 	 	  	  			  printf("����΢ģ�鷢�ִ���״̬����253����10�ε�,����������\n");
   	  		 	 	  	  		  }
   	  		 	 	  	  		}
   	  		 	 	  	  	}
   	  		 	 	  	  	break;
   	  		 	 	  	     
   	  		 	 	  	  case  0:   //ͨ�ų�ʱ
   	  		 	 	  	  case  4:   //��Ϣ�಻����(����)
   	  		 	 	  	  case  8:   //���ܱ����Ӧ��(����)
   	  		 	 	  	  case 14:   //�������ɹ�(����ģ��)
   	  		 	 	  	  case 16:   //���Э��(����ά����)
   	  		 	 	  	  case 18:   //���Э��(��Ų���������)
   	  		 	 	  	  case 19:   //���Э��(�����������,����ǰ������ʱ��ͨ)
   	  		 	 	  	  case 20:   //���Э��(�����������,��ǰ���糩ͨ,����Ʊ���RS485��ͨ)
   	  		 	 	  	  case 22:   //���Э��(�����������,��ǰ���糩ͨ,��II�ͱ�����ͨѶ�й���)
   	  		 	 	  	  case 0xff: //����΢(δ֪����)
                      if (debugInfo&PRINT_CARRIER_DEBUG)
                      {
                        switch(tmpBuf[0])
                        {
                        	case 0:
                            printf("ͨ�ų�ʱ\n");
                        		break;

                        	case 4:
                            printf("��Ϣ�಻����(���Ժģ��)\n");
                        		break;

                        	case 8:
                            printf("������Ӧ��(���Ժģ��)\n");
                        		break;
                        	
                        	case 14:
                            printf("����ʧ��(SR����ģ��)\n");
                        		break;

                        	case 16:
                            printf("����ά����(RL����ģ��)\n");                         	
                        		break;

                        	case 18:
                            printf("��Ų���������(RL����ģ��)\n");                         	
                        		break;

                        	case 19:
                            printf("�����������,����ǰ������ʱ��ͨ(RL����ģ��)\n");                         	
                        		break;

                        	case 20:
                            printf("�����������,��ǰ���糩ͨ,����Ʊ���RS485��ͨ(RL����ģ��)\n");                         	
                        		break;

                        	case 22:
                            printf("�����������,��ǰ���糩ͨ,��II�ͱ�����ͨѶ�й���(RL����ģ��)\n");                         	
                        		break;

                        	case 0xff:
                            printf("δ֪����\n");                         	
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
     	  		 
     	  		case QUERY_DATA_3762:    //��ѯ����
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	  case 1:    //��ѯ���̴���Ͱ汾��Ϣ
     	  		 	 	  	memcpy(carrierFlagSet.productInfo, tmpBuf, 9);
     	  		 	 	  	
     	  		 	 	  	//��������ز������ಢ�г���,�����չ2������˿�������
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
     	  		 	 	  				 printf("SR - SRWF-3E68ģ��\n");
     	  		 	 	  			}
     	  		 	 	  		}
     	  		 	 	  	}
     	  		 	 	  	break;
     	  		 	 	  	
     	  		 	 	  case 4:
     	  		 	 	  	carrierFlagSet.mainNode = 1;    //�ѵõ����ڵ��ַ
                    memcpy(carrierFlagSet.mainNodeAddr, &tmpBuf[13], 6);
                    
                    selectParameter(0x04, 133, mainNodeAddr, 6);

                    if (debugInfo&PRINT_CARRIER_DEBUG)
                    {
                    	printf("�ز�/����ģ�������ڵ��ַ:%02x%02x%02x%02x%02x%02x\n",carrierFlagSet.mainNodeAddr[0],
                    	       carrierFlagSet.mainNodeAddr[1],carrierFlagSet.mainNodeAddr[2],carrierFlagSet.mainNodeAddr[3],
                    	       carrierFlagSet.mainNodeAddr[4],carrierFlagSet.mainNodeAddr[5]);
                      printf("������mainNodeAddr=%02x%02x%02x%02x%02x%02x\n",mainNodeAddr[0],mainNodeAddr[1],mainNodeAddr[2],mainNodeAddr[3],mainNodeAddr[4],mainNodeAddr[5]);
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
                    
                    //�Ƚϱ���ͨ��ģ���ַ�뼯������ַ�Ƿ�һ��,��һ���Ļ�Ҫ�·�
                  	if (carrierModuleType==SC_WIRELESS)   //������ģ�鲻���·����ڵ��ַ
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
                        	  printf("���������ز�/�������ڵ��ַ\n"); 
                          }
                    	  }
                    	}
                    }
                    break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
            case ACTIVE_REPORT_3762: //�����ϱ�
             	 switch(carrierFn)
             	 {
             	 	  case 1:    //�ϱ��ز��ӽڵ���Ϣ
             	 	  	//�ظ�ȷ��֡
             	 	  	tmpSendBuf[0] = 0x01;    //�Ѵ���
             	 	  	tmpSendBuf[1] = 0x00;    
             	 	  	tmpSendBuf[2] = 0x05;    //��ʱ�ȴ�1
             	 	  	tmpSendBuf[2] = 0x00;    //��ʱ�ȴ�2
             	 	  	gdw3762Framing(ACK_OR_NACK_3762, 1, NULL, tmpSendBuf);
             	 	  	
             	 	  	//�ٴ����յ�������
             	 	  	tmpNode = NULL;
             	 	  	noRecordMeterHead = NULL;
             	 	  	printf("�ϱ��ӽڵ����=%d\n",tmpBuf[0]);
             	 	  	for(i=0; i<tmpBuf[0]; i++)
             	 	  	{
                       tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                       tmpFound->mp = 0;                       //�ӽڵ����
                       memcpy(tmpFound->addr,tmpBuf+i*9+1,6);  //�ӽڵ��ַ
                       tmpFound->protocol = *(tmpBuf+i*9+1+6); //��Լ����
                       tmpFound->mpNo     = *(tmpBuf+i*9+1+7) | *(tmpBuf+i*9+1+8)<<8;  //�ӽڵ����
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
                       	  	    printf("�ϱ�����������õĲ�������Ϣ�ɼ�����ַ��ͬ\n");
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
                       	  	   printf("�ϱ�����������õĲ�������Ϣ�ز����ַ��ͬ\n");
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
                       	  	   printf("�ϱ���������ϱ��ı�ŵ�ַ��ͬ\n");
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
             	 	  	  printf("�����ϱ��ı����Ϣ:\n");
             	 	  	  tmpFound = foundMeterHead;
             	 	  	  while(tmpFound!=NULL)
             	 	  	  {
        	  		 	 	    printf("�ϱ��ڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpFound->addr[5],tmpFound->addr[4],tmpFound->addr[3],tmpFound->addr[2],tmpFound->addr[1],tmpFound->addr[0]);
        	  		 	 	    printf("�ϱ��ڵ��Լ:%d\n",tmpFound->protocol);
        	  		 	 	    printf("�ϱ��ڵ����:%d\n",tmpFound->mpNo);
             	 	  		 
             	 	  		  tmpFound = tmpFound->next;
             	 	  	  }
             	 	    }
             	 	  	
             	 	  	if (tmpNode==NULL)
                    {
             	 	  	  searchMeterReport();   //�����������(LCD��ʾ)
             	 	  	}             	 	  	

                    if (noRecordMeterHead!=NULL)
                    {
                      //���ֵ���¼
                      foundMeterEvent(noRecordMeterHead);
                    }
             	 	  	break;
             	 	  	
             	 	  case 2:    //�ϱ���������
             	 	  	//�ظ�ȷ��֡
             	 	  	tmpSendBuf[0] = 0x01;    //�Ѵ���
             	 	  	tmpSendBuf[1] = 0x00;    
             	 	  	tmpSendBuf[2] = 0x02;    //��ʱ�ȴ�1
             	 	  	tmpSendBuf[3] = 0x00;    //��ʱ�ȴ�2
             	 	  	tmpSendBuf[4] = tmpBuf[tmpBuf[3]+4]&0xf;    //��λ,2013-12-24,add
             	 	  	gdw3762Framing(ACK_OR_NACK_3762, 1, NULL, tmpSendBuf);
             	 	  	
                    if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	          {
    	  	   	        if (carrierModuleType==TC_CARRIER)
    	  	   	        {
    	  	   	          printf("·����������:�ϱ���������,���ݳ���=%d,�ظ���λ=%d\n",tmpBuf[3],tmpBuf[tmpBuf[3]+4]);
    	  	   	        }
    	  	   	        else
    	  	   	        {
    	  	   	          printf("·����������:�ϱ���������,���ݳ���=%d\n",tmpBuf[3]);
    	  	   	        }
    	  	          }
    	  	          
    	  	          //����ɼ����е�ַ��ʽ,Ҫ�����Ľ���
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
    	  	          
    	  	          //�ٴ����յ�������
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
     	  		 
     	  		case ROUTE_QUERY_3762:   //·�ɲ�ѯ
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	  case 1:    //��ѯ�ӽڵ�����
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
     	  		 	 	  		 printf("����ͨ��ģ���ڴӽڵ�����Ϊ%d\n",carrierFlagSet.numOfSalveNode);
     	  		 	 	  	}
     	  		 	 	  	break;
     	  		 	 	  	
     	  		 	 	  case 2:    //��ѯ�ز��ӽڵ���Ϣ
     	  		 	 	  	if (carrierModuleType!=SC_WIRELESS)
     	  		 	 	  	{
     	  		 	 	  	  carrierFlagSet.numOfSalveNode = tmpBuf[0] | tmpBuf[1]<<8;
     	  		 	 	  	}
                    if (tmpBuf[2]==0)
                    {
     	  		 	 	  	   if (carrierFlagSet.querySlaveNode==1)
     	  		 	 	  	   {
   	                      carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
   	                   
   	                      if (debugInfo&PRINT_CARRIER_DEBUG)
   	                      {
   	                        printf("�ز�ģ��ӽڵ��б�(·�ɲ�ѯ)\n");
   	                        tmpSalveNode = carrierSlaveNode;
   	                        while(tmpSalveNode!=NULL)
   	                        {
     	  		 	 	  	          printf("�ӽڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpSalveNode->addr[5],tmpSalveNode->addr[4],tmpSalveNode->addr[3],tmpSalveNode->addr[2],tmpSalveNode->addr[1],tmpSalveNode->addr[0]);
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
        	  		 	 	  	   printf("�ӽڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpBuf[i*8+8],tmpBuf[i*8+7],tmpBuf[i*8+6],tmpBuf[i*8+5],tmpBuf[i*8+4],tmpBuf[i*8+3]);
        	  		 	 	  	 }
        	  		 	 	  	 
        	  		 	 	  	 //�ز��ӽڵ��ַΪȫ0,����Ϊ�����к����ӽڵ��ַ,ֹͣ��ȡ
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
   	                         carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
   	                   
   	                         if (debugInfo&PRINT_CARRIER_DEBUG)
   	                         {
   	                           printf("�������ز�ģ��ӽڵ��б�,�򷵻صĴӽڵ��ַΪȫ0\n");
   	                         }
     	  		 	 	  	       }
     	  		 	 	  	       
     	  		 	 	  	       break;
                         }
                         
                         tmpSalveNode = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                         tmpSalveNode->mpNo = carrierFlagSet.synMeterNo;           //�ӽڵ����
                         memcpy(tmpSalveNode->addr,tmpBuf+i*8+3,6); //�ӽڵ��ַ
                         memcpy(tmpSalveNode->info,tmpBuf+i*8+9,2); //�ӽڵ���Ϣ
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
                       
                       //������شӽڵ�ĸ������ڵ��ڴӽڵ�������,ֹͣ��ȡ
                       if (carrierFlagSet.synMeterNo > carrierFlagSet.numOfSalveNode)
                       {
   	                      carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
   	                   
   	                      if (debugInfo&PRINT_CARRIER_DEBUG)
   	                      {
   	                        printf("���شӽڵ�ĸ������ڵ��ڴӽڵ�������,ֹͣ��ȡ\n");
   	                      }
                       }
                    }                    
     	  		 	 	  	break;
     	  		 	 	  	
     	  		 	 	  case 4:    //·������״̬
     	  		 	 	  	carrierFlagSet.numOfSalveNode = tmpBuf[1] | tmpBuf[2]<<8;
     	  		 	 	  	
     	  		 	 	  	if (carrierModuleType==SR_WIRELESS)
     	  		 	 	  	{
   	  		 	 	  		  if (tmpBuf[0]==0x01)
   	  		 	 	  		  {
                        if (debugInfo&PRINT_CARRIER_DEBUG)
                        {
                        	printf("SR - �������\n");
                        }
                        
                        showInfo("����ͨ��ģ���������");
                        
                        carrierFlagSet.wlNetOk = 3;
                        
                        //�����ǰ��δ�����ڵ���Ϣ
                        while (noFoundMeterHead!=NULL)
                        {
                          tmpFound = noFoundMeterHead;
                          noFoundMeterHead = noFoundMeterHead->next;
                            
                          free(tmpFound);
                        }
                              
                 	      //��δ�����ڵ�
                 	      gdw3762Framing(RL_EXTEND_3762, 4, NULL, NULL);
                        carrierFlagSet.cmdType = CA_CMD_READ_NO_NET_NODE;
   	  		 	 	  		  }
   	  		 	 	  		  else
   	  		 	 	  		  {
     	  		 	 	  		  showInfo("����ͨ��ģ��������..");

     	  		 	 	  		  if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  		  {
                        	printf("SR - ����δ���\n");
     	  		 	 	  		    printf("SR - �ӽڵ�����=%d\n", carrierFlagSet.numOfSalveNode);
     	  		 	 	  		    printf("SR - �����ڵ�����=%d\n", tmpBuf[3] | tmpBuf[4]<<8);
     	  		 	 	  		  }
     	  		 	 	  		  
     	  		 	 	  		  if (carrierFlagSet.numOfSalveNode==(tmpBuf[3] | tmpBuf[4]<<8))
     	  		 	 	  		  {
     	  		 	 	  		  	if (localModuleType!=SR_WF_3E68)
     	  		 	 	  		  	{
     	  		 	 	  		  	  if (carrierFlagSet.numOfSalveNode!=0)
     	  		 	 	  		  	  {
     	  		 	 	  		  	    carrierFlagSet.wlNetOk = 3;
     	  		 	 	  		  	  }
                     
                            //�����ǰ��δ�����ڵ���Ϣ
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
     	  		 	 	  		        printf("SR - ���нڵ������\n");
     	  		 	 	  		      }
     	  		 	 	  		  	}
     	  		 	 	  		  }
   	  		 	 	  		    
   	  		 	 	  		    //������β�ѯ�������ڵ���������һ�εĲ�ͬ,��������ʱʱ���������3����
   	  		 	 	  		    if ((carrierFlagSet.lastNumInNet != (tmpBuf[3] | tmpBuf[4]<<8))
   	  		 	 	  		    	 && (carrierFlagSet.wlNetOk!= 3)
   	  		 	 	  		    	 )
   	  		 	 	  		    {
   	  		 	 	  		  	  if (localModuleType==SR_WF_3E68)
   	  		 	 	  		  	  {
   	  		 	 	  		  	    carrierFlagSet.netOkTimeOut = nextTime(sysTime, SRWF_NET_OK_TIME_OUT, 0);
   	  		 	 	  		  	
   	  		 	 	  		  	    if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  		  	    {
   	  		 	 	  		  		     printf("SR - ���β�ѯ�������ڵ���������һ�εĲ�ͬ,��������ʱʱ���������%d����\n", SRWF_NET_OK_TIME_OUT);
   	  		 	 	  		  	    }
   	  		 	 	  		  	  }
   	  		 	 	  		  	  else
   	  		 	 	  		  	  {
   	  		 	 	  		  	    carrierFlagSet.netOkTimeOut = nextTime(sysTime, 3, 0);
   	  		 	 	  		  	
   	  		 	 	  		  	    if(debugInfo&PRINT_CARRIER_DEBUG)
   	  		 	 	  		  	    {
   	  		 	 	  		  		     printf("SR - ���β�ѯ�������ڵ���������һ�εĲ�ͬ,��������ʱʱ���������3����\n");
   	  		 	 	  		  	    }
   	  		 	 	  		  	  }
   	  		 	 	  		    }
  
     	  		 	 	  		  //��һ�β�ѯ�����ڵ�����
     	  		 	 	  		  carrierFlagSet.lastNumInNet = tmpBuf[3] | tmpBuf[4]<<8;
     	  		 	 	  		}
     	  		 	 	  	}
     	  		 	 	  	else
     	  		 	 	  	{
       	  		 	 	  	if (tmpBuf[0]&0x2)
       	  		 	 	  	{
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("���ڹ���\n");
       	  		 	 	  	  }
       	  		 	 	  	}
       	  		 	 	  	else
       	  		 	 	  	{
       	  		 	 	  		if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		{
       	  		 	 	  		  printf("ֹͣ����\n");
       	  		 	 	  		}
       	  		 	 	  		
     	  		 	 	  		  if (tmpBuf[0]&0x1)
     	  		 	 	  		  {
     	  		 	 	  			   if (carrierFlagSet.studyRouting==2)
     	  		 	 	  			   {
     	  		 	 	  			     carrierFlagSet.studyRouting = 0;   //ֹͣѧϰ·��
     	  		 	 	  			   
     	  		 	 	  			     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	  			     {
     	  		 	 	  			       printf("ֹͣ·��ѧϰ\n");
     	  		 	 	  			     }
     	  		 	 	  			   }
       	  		 	 	  		   
       	  		 	 	  		   //������ѱ�,·������ɵĻ�,����ֹͣ�ѱ�ly,10-08-27,��Ӧ���̼ӵ�
       	  		 	 	  		   //ly,2011-06-07,�ĳ��ѱ��־��Ϊ0x08
       	  		 	 	  		   if (carrierFlagSet.searchMeter!=0)
       	  		 	 	  		   {
       	  		 	 	  		      if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  		      {
       	  		 	 	  		        printf("����ͨ��ģ����ֹͣ�ѱ�\n");
       	  		 	 	  		      }
              	 	 		
       	  		 	 	  		  	  carrierFlagSet.searchMeter = 0x00;  //ģ����ֹͣ�ѱ�
       	  		 	 	  		 
              	  		 	 	  	if (menuInLayer==0)
              	  		 	 	  	{
              	  		 	 	  		defaultMenu();
              	  		 	 	  	}
                              searchMeter(0xff);
  
       	  		 	 	  		  	  //carrierFlagSet.foundStudyTime = sysTime;  //ly,2011-06-07,ע��
                              //ly,2011-12-02,�չ���������Ӧ���ָ�λЧ�������̽Ϻ�,�����ִ�������
                              if (carrierModuleType==CEPRI_CARRIER)
                              {
                              	 //ly,2012-02-28,�������ж�
                              	 if (localModuleType!=CEPRI_CARRIER_3_CHIP)
                              	 {
                              	   if (debugInfo&PRINT_CARRIER_DEBUG)
                              	   {
                              	 	   printf("���Ժģ��:(�����߳�)�Զ�����������ģ��\n");
                              	   }
                              	 
                              	   sleep(1);
                              	 
                              	   ifReset=TRUE;
                              	 }
                              }
                              
                              if (carrierFlagSet.routeLeadCopy==1)
                              {
                                 copyCtrl[4].nextCopyTime = nextTime(sysTime, 2, 0);    //2�����Ժ�����������
                              }
       	  		 	 	  		   }
     	  		 	 	  		  }
       	  		 	 	  	}
       	  		 	 	  	
       	  		 	 	  	if (debugInfo&PRINT_CARRIER_DEBUG)
       	  		 	 	  	{
        	  		 	 	  	 if (tmpBuf[0]&0x1)
        	  		 	 	  	 {
        	  		 	 	  		 printf("ѧϰ·�����\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("ѧϰ·��δ���\n");
        	  		 	 	  	 }
        	  		 	 	  	 if (tmpBuf[0]&0x4)
        	  		 	 	  	 {
        	  		 	 	  		 printf("�дӽڵ��ϱ��¼�\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("�޴ӽڵ��ϱ��¼�\n");
        	  		 	 	  	 }
        	  		 	 	  	
        	  		 	 	  	 if (tmpBuf[7]&0x1)
        	  		 	 	  	 {
        	  		 	 	  		 printf("����״̬:ѧϰ\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("����״̬:����\n");
        	  		 	 	  	 }
        	  		 	 	  	 if (tmpBuf[7]&0x2)
        	  		 	 	  	 {
        	  		 	 	  		 printf("����״̬:����ע��\n");
        	  		 	 	  	 }
        	  		 	 	  	 else
        	  		 	 	  	 {
        	  		 	 	  		 printf("����״̬:������ע��\n");
        	  		 	 	  	 }
       	  		 	 	  	}
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	if (1==carrierFlagSet.routeLeadCopy && carrierFlagSet.workStatus==3 && TRUE==copyCtrl[4].meterCopying)
     	  		 	 	  	{
 	                    if (debugInfo&PRINT_CARRIER_DEBUG)
 	                    {
   	                    printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:������,ģ��ظ���ѯ·��״̬����,˵����·��ͨ������,����ʱ�������%d����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,ROUTE_REQUEST_OUT);
 	                    }
     	  		 	 	  		
     	  		 	 	  		//·������ʱʱ���������
                      carrierFlagSet.routeRequestOutTime = nextTime(sysTime, ROUTE_REQUEST_OUT, 0);
     	  		 	 	  	}
     	  		 	 	  	
     	  		 	 	  	carrierFlagSet.routeRunStatus = 0;
     	  		 	 	 	  break;
     	  		 	 	 	  
     	  		 	 	 	case 6:   //����ע����ز��ӽڵ���Ϣ
     	  		 	 	 		carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 10);  //10����ٲ�ѯ
     	  		 	 	 		
             	 	  	tmpNode = NULL;
             	 	  	noRecordMeterHead = NULL;
             	 	  	carrierFlagSet.activeMeterNo += tmpBuf[2];
             	 	  	for(i=0;i<tmpBuf[2];i++)
             	 	  	{
                       tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                       tmpFound->mp = 0;                           //
                       memcpy(tmpFound->addr,tmpBuf+i*8+3,6);      //�ӽڵ��ַ
                       tmpFound->info[0]     = *(tmpBuf+i*8+3+6);  //�ӽڵ���Ϣ1
                       tmpFound->info[1]     = *(tmpBuf+i*8+3+7);  //�ӽڵ���Ϣ2
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
                       	  	   printf("�ϱ�����������õĲ�������Ϣ�ɼ�����ַ��ͬ\n");
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
                       	  	   printf("�ϱ�����������õĲ�������Ϣ�ز����ַ��ͬ\n");
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
             	 	  	  printf("����ע����ز��ӽڵ���Ϣ:\n");
             	 	  	  tmpFound = foundMeterHead;
             	 	  	  while(tmpFound!=NULL)
             	 	  	  {
        	  		 	 	    printf("�ϱ��ڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpFound->addr[5],tmpFound->addr[4],tmpFound->addr[3],tmpFound->addr[2],tmpFound->addr[1],tmpFound->addr[0]);
             	 	  		 
             	 	  		  tmpFound = tmpFound->next;
             	 	  	  }
             	 	  	}
             	 	  	
             	 	  	if (tmpNode==NULL)
                    {
             	 	  	  searchMeterReport();   //�����������(LCD��ʾ)
             	 	  	}
                    
                    if (noRecordMeterHead!=NULL)
                    {
                       //���ֵ���¼
                       foundMeterEvent(noRecordMeterHead);
                    }
     	  		 	 	 		break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
     	  		case DATA_FORWARD_3762:        //����ת��
     	  		case ROUTE_DATA_FORWARD_3762:  //·������ת��
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	  case 1:   //����ز��ӽڵ�ظ�
     	  		 	 	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	    {
     	  		 	 	  	  if (carrierAfn==DATA_FORWARD_3762)
     	  		 	 	  	  {
   	                    printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfCarrierReceive,����ģ������ת���ظ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	  		 	 	  	  }
     	  		 	 	  	  else
     	  		 	 	  	  {
   	                    printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfCarrierReceive,����ز��ӽڵ�ظ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	  		 	 	  	  }
     	  		 	 	    }
     	  	          
     	  	          //ly,2011-05-27,��Ӧ����΢
     	  	          carrierFlagSet.numOfCarrierSending = 0;

                   #ifdef LIGHTING
                    //ִ������LDDT������ǰ,ִ�е㳭��ת�������ں�,2015-06-03,add
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
       	  	             	printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
       	  	             	if (1==carrierFlagSet.searchLddt)
       	  	             	{
       	  	             	  printf("����LDDT[1]");
       	  	             	}
       	  	             	else
       	  	             	{
       	  	             	  if (1==carrierFlagSet.searchOffLine)
       	  	             	  {
       	  	             	    printf("�������ߵ��ƿ�����[1]");
       	  	             	  }
       	  	             	  else
       	  	             	  {
       	  	             	  	printf("��ѯ���ϵ絥�ƿ�����״̬[1]");
       	  	             	  }
       	  	             	}
       	  	             	printf(",�ӽڵ�ظ�0�ֽ�\n");
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
                      
                      if (copyCtrl[4].pForwardData!=NULL)    //����ת������
     	  		 	 	  	  {
       	  	            //2015-10-19,add
       	  	            copyCtrl[4].copyTimeOut = sysTime;
       	  	            
       	  	            //2012-08-20,add
       	  	            if (tmpBuf[1]==0)
       	  	            {
                          //2013-12-25,�޸�����ж��ڵĴ���,ԭ���Ĵ�������֮����,����
                          //    ���ڵĴ���Ҳ���Ǵӽڵ�Ļظ�,ֻ����û�������ݶ���
                         
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
                      	    printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:ת��,�ӽڵ�ظ�0�ֽ�", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
                      	    if (copyCtrl[4].meterCopying==TRUE)
                      	    {
                      	      printf(",�ȴ�%d���Ӻ�ָ�����\n", WAIT_RECOVERY_COPY);
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
                          //ly,2012-8-20,�޸��������,���ھ���4��
                          //forwardReceive(3, &tmpBuf[2], tmpBuf[1]); 
                          forwardReceive(4, &tmpBuf[2], tmpBuf[1]);
     	  		 	 	  		  }
     	  		 	 	  		  else
     	  		 	 	  		  {  
                          if (copyCtrl[4].pForwardData->fn==0x01)  //͸��ת��
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
          	  	          //����ɼ����е�ַ��ʽ,Ҫ�����Ľ���
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
       	  	             	 	 	  printf("%02d-%02d-%02d %02d:%02d:%02d,·����������:", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
       	  	             	 	 	  if (1==carrierFlagSet.searchLddt)
       	  	             	 	 	  {
       	  	             	 	 	    printf("����LDDT[2]\n");
       	  	             	 	 	  }
       	  	             	 	 	  else
       	  	             	 	 	  {
       	  	             	 	 	    if (1==carrierFlagSet.searchOffLine)
       	  	             	 	 	    {
       	  	             	 	 	      printf("�������ߵ��ƿ�����[2]\n");
       	  	             	 	 	    }
       	  	             	 	 	    else
       	  	             	 	 	    {
       	  	             	 	 	      printf("��ѯ���ϵ絥�ƿ�����״̬[2]\n");
       	  	             	 	 	    }
       	  	             	 	 	  }
       	  	             	 	 	  printf(",�ӽڵ�ظ�0�ֽ�\n");
       	  	             	 	  }
       	  	             	  }
       	  	              }
       	  	             #endif
       	  	             
     	  		 	 	  	      meter485Receive(PORT_POWER_CARRIER, &tmpBuf[2], tmpBuf[1]);
     	  		 	 	  	    }
     	  		 	 	  	    else
     	  		 	 	  	    {
     	  		 	 	  	   	  #ifdef PRINT_CARRIER_DEBUG
     	  		 	 	  	   	   printf("����ת���ظ��ظ�֡,����\n");
     	  		 	 	  	   	  #endif
     	  		 	 	  	    }
     	  		 	 	  	  }
     	  		 	 	  	  
     	  		 	 	   #ifdef LIGHTING
     	  		 	 	  	}
      	  		 	 	 #endif
     	  		 	 	  	break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
     	  		case ROUTE_DATA_READ_3762:     //·�����ݳ���
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	 case 1:   //·�����ݳ�������
                   if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         {
    	  	   	       printf("·����������:���󳭶�,");
    	  	         }

                   if (carrierModuleType==TC_CARRIER)  //�����ز�
                   {
                     //ֻ������λA��B��C(��Ӧ1��2��3)
                     if (tmpBuf[0]>0 && tmpBuf[0]<4)
                     {
                       copyCtrl[tmpBuf[0]+3].copyContinue = TRUE;
     	  		 	 	       memcpy(copyCtrl[tmpBuf[0]+3].needCopyMeter, &tmpBuf[1], 6);
                       
                       if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	             {
    	  	   	           printf("�˿�%d", tmpBuf[0]+3+1);
    	  	             }
                     }
                   }
                   else                                //�����ز�(��������΢)
                   {
     	  		 	 	     memcpy(copyCtrl[4].needCopyMeter, &tmpBuf[1], 6);
                   	 copyCtrl[4].copyContinue = TRUE;
                     
                     if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	           {
    	  	   	         printf("�˿�5");
    	  	           }
                   }
                   
                   //·������ʱʱ���������
                   carrierFlagSet.routeRequestOutTime = nextTime(sysTime, ROUTE_REQUEST_OUT, 0);
                   
                   if (debugInfo&PRINT_CARRIER_DEBUG)
    	  	         {
    	  	   	       printf(",���ַ:%02x%02x%02x%02x%02x%02x,��λ=%d\n",
    	  	   	              tmpBuf[6],tmpBuf[5],tmpBuf[4],tmpBuf[3],tmpBuf[2],tmpBuf[1], tmpBuf[0]);
    	  	         }
     	  		 	 	   break;
     	  		 	 }
     	  		 	 break;
     	  		 	 
            case RL_EXTEND_3762:           //�����չ����
             	 switch(carrierFn)
             	 {
             	 	 case 4:    //��δ�����ڵ��
             	 	 	 if (debugInfo&PRINT_CARRIER_DEBUG)
             	 	 	 {
             	 	 	   printf("����ģ��  - δ�����ڵ��\n");
             	 	 	   printf("����=%d\n",tmpBuf[0]|tmpBuf[1]<<8);
             	 	 	 }
             	 	  
             	 	   tmpNode = NULL;
             	 	   for(i=0;i<tmpBuf[0]|tmpBuf[1]<<8;i++)
             	 	   {
                       tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                       tmpFound->mp = 0;
                       memcpy(tmpFound->addr,tmpBuf+i*6+2,6); //�ӽڵ��ַ
                       
                       if (debugInfo&PRINT_CARRIER_DEBUG)
                       {
                         printf("%02x%02x%02x%02x%02x%02x\n", *(tmpBuf+i*6+7), *(tmpBuf+i*6+6),*(tmpBuf+i*6+5),*(tmpBuf+i*6+4),*(tmpBuf+i*6+3),*(tmpBuf+i*6+2));
                       }
                       
                       tmpFound->info[0]       = 0x0;         //�ӽڵ���Ϣ1
                       tmpFound->info[1]       = 0x0;         //�ӽڵ���Ϣ2
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
             	 	     printf("δ�����Ľڵ���Ϣ:\n");
             	 	     tmpFound = noFoundMeterHead;
             	 	     while(tmpFound!=NULL)
             	 	     {
        	  		 	 	   printf("�ڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpFound->addr[5],tmpFound->addr[4],tmpFound->addr[3],tmpFound->addr[2],tmpFound->addr[1],tmpFound->addr[0]);
             	 	  		 
             	 	  	   tmpFound = tmpFound->next;
             	 	     }
             	 	   }
             	 	 	 break;
             	 	 	 
             	 	 case 6:    //������״̬
             	 	 	 printf("RL����ģ��  - ����״̬\n");
             	 	 	 
             	 	 	 if (tmpBuf[0]==0x03)
             	 	 	 {
             	 	 	 	 printf("RL����ģ��:��������(�����·��Ľڵ�ŵ�����)\n");
             	 	 	 }
             	 	 	 else
             	 	 	 {
             	 	 	 	 printf("RL����ģ��:���·��Ľڵ������\n");
             	 	 	 }
             	 	 	 
             	 	 	 if (tmpBuf[1]==0x0e)
             	 	 	 {
             	 	 	 	 printf("RL����ģ��:��������\n");
             	 	 	 	 
             	 	 	 	 carrierFlagSet.wlNetOk++;
             	 	 	 }
             	 	 	 else
             	 	 	 {
             	 	 	 	 printf("RL����ģ��:��������\n");
             	 	 	 }
             	 	 	 
             	 	 	 printf("RL����ģ��:������RobuNet���Ľڵ�����:%d\n", tmpBuf[3]<<8 | tmpBuf[2]);
             	 	 	 printf("RL����ģ��:����Modem���·��ĵ������:%d\n", tmpBuf[5]<<8 | tmpBuf[4]);
             	 	 	 if (tmpBuf[6]==0x99 && tmpBuf[7]==0x99)
             	 	 	 {
             	 	 	 	 printf("RL����ģ��:9999,�ڴ��ڼ��޷�����\n");
             	 	 	 }
             	 	 	 else
             	 	 	 {
             	 	 	 	 printf("RL����ģ��:δ����Robunet�ڵ������=%d\n",tmpBuf[7]<<8 | tmpBuf[6]);
             	 	 	 }
             	 	 	 //printf("RL����ģ��:������Robunet���Ľڵ���������:%02x\n", tmpBuf[8]);             	 	 	 
             	 	 	 break;
             	 }
               break;
               
     	  		case DEBUG_3762:
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	   case 1:    //�����ϱ����ܿ���
     	  		 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	     {
     	  		 	      if (tmpBuf[0]==1)
     	  		 	      {
     	  		 	      	 printf("����ģ�� - �����ϱ�����ʹ��\n");
     	  		 	      }
     	  		 	      else
     	  		 	      {
     	  		 	      	 printf("����ģ�� - �����ϱ����ܽ�ֹ\n");
     	  		 	      }
     	  		 	     }
     	  		 	     
     	  		 	     carrierFlagSet.innerDebug = 1;
     	  		 	     break;
     	  		 	     
     	  		 	   case 3:    //�ϱ������ɹ�
     	  		 	   	 printf("���������� - �����ɹ�\n");     	  		 	   	 
     	  		 	   	 if (tmpBuf[0]==1)
     	  		 	   	 {
     	  		 	   	 	 printf("����ģʽ   - ��֤ģʽ\n");
     	  		 	   	 }
     	  		 	   	 else
     	  		 	   	 {
     	  		 	   	 	 printf("����ģʽ   - ����֤ģʽ\n");     	  		 	   	 	 
     	  		 	   	 }     	  		 	   	 
     	  		 	   	 printf("�����ŵ�   - %d\n", tmpBuf[1]);
     	  		 	   	 printf("��������   - %d\n", tmpBuf[2] | tmpBuf[3]<<8);
     	  		 	   	 printf("����PanID  - %d\n", tmpBuf[4] | tmpBuf[5]<<8);
     	  		 	   	 printf("���ڵ�̶���ַ - %02x%02x%02x%02x%02x%02x\n",tmpBuf[11],tmpBuf[10],tmpBuf[9],tmpBuf[8],tmpBuf[7],tmpBuf[6]);
     	  		 	   	 printf("���ڵ����ڹ̶���ַ - %02x%02x%02x%02x%02x%02x%02x%02x\n",tmpBuf[19],tmpBuf[18],tmpBuf[17],tmpBuf[16],tmpBuf[15],tmpBuf[14],tmpBuf[13],tmpBuf[12]);
     	  		 	   	 break;
     	  		 	   	 
     	  		 	   case 4:    //�ϱ������ڵ�
     	  		 	   	 printf("����ģ��     - �ϱ������ڵ�\n");
     	  		 	   	 printf("�ڵ��ַ     - %02x%02x%02x%02x%02x%02x\n",tmpBuf[5],tmpBuf[4],tmpBuf[3],tmpBuf[2],tmpBuf[1],tmpBuf[0]);
     	  		 	   	 printf("���ڹ̶���ַ - %02x%02x%02x%02x%02x%02x%02x%02x\n",tmpBuf[13],tmpBuf[12],tmpBuf[11],tmpBuf[10],tmpBuf[9],tmpBuf[8],tmpBuf[7],tmpBuf[6]);
             	 	   
             	 	   noRecordMeterHead = NULL;
                   tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                   memcpy(tmpFound->addr,tmpBuf,6);  //�ӽڵ��ַ
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
                   	  	   printf("�ϱ���������ϱ��ı�ŵ�ַ��ͬ\n");
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

     	  		 	   case 5:    //�ϱ������ڵ�
     	  		 	   	 printf("����ģ��     - �ϱ������ڵ�\n");
     	  		 	   	 printf("�ڵ��ַ     - %02x%02x%02x%02x%02x%02x\n",tmpBuf[5],tmpBuf[4],tmpBuf[3],tmpBuf[2],tmpBuf[1],tmpBuf[0]);
     	  		 	   	 printf("���ڹ̶���ַ - %02x%02x%02x%02x%02x%02x%02x%02x\n",tmpBuf[13],tmpBuf[12],tmpBuf[11],tmpBuf[10],tmpBuf[9],tmpBuf[8],tmpBuf[7],tmpBuf[6]);

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
                   	  	   printf("�������ڵ���ɾ�������ڵ�\n");
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
     	  		 
     	  		case FC_QUERY_DATA_3762:  //��Ѹ����չ��ѯ����
     	  		 	 switch(carrierFn)
     	  		 	 {
     	  		 	 	 case 2:    //CAC ״̬��Ϣ
     	  		 	 	 	 switch(tmpBuf[0])
     	  		 	 	 	 {
     	  		 	 	 	 	 case 1:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	 	 printf("��Ѹ��ģ��:CAC����״̬\n");
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 	      	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 if (carrierFlagSet.readStatus==1)
     	  		 	 	 	 	 	 {
     	  		 	 	 	 	 	 	 carrierFlagSet.readStatus = 2;
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 carrierFlagSet.wlNetOk = 3;
     	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 showInfo("����ͨ��ģ���������");
     	  		 	 	 	 	 	 break;
     	  		 	 	 	 	 	 
     	  		 	 	 	 	 case 2:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	   printf("��Ѹ��ģ��:CAC����״̬\n");
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 	 
     	  		 	 	 	 	 	 showInfo("����ͨ��ģ��������..");
     	  		 	 	 	 	 	 break;
     	  		 	 	 	 	 	 
     	  		 	 	 	 	 case 3:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	   printf("��Ѹ��ģ��:CAC����ά��\n");
     	  		 	 	 	 	 	 }
     	  		 	 	 	 	 	 
     	  		 	 	 	 	 	 showInfo("����ͨ��ģ��������ά��");
     	  		 	 	 	 	 	 break;

     	  		 	 	 	 	 case 4:
     	  		 	 	 	     if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	     {
     	  		 	 	 	 	 	   printf("��Ѹ��ģ��:CAC���ڴ����ϸ�����\n");
     	  		 	 	 	 	 	 
							     }
     	  		 	 	 	 	 break;
     	  		 	 	 	 }
     	  		 	 	 	 	 
     	  		 	 	 	 if (debugInfo&PRINT_CARRIER_DEBUG)
     	  		 	 	 	 {
     	  		 	 	 	 	 printf("��Ѹ��ģ��:����DAU�ĸ���=%d\n",tmpBuf[1] | tmpBuf[2]<<8);
     	  		 	 	 	 	 printf("��Ѹ��ģ��:����DAU�ĸ���=%d\n",tmpBuf[3] | tmpBuf[4]<<8);
     	  		 	 	 	 	 printf("��Ѹ��ģ��:����DAU�ĸ���=%d\n",tmpBuf[5] | tmpBuf[6]<<8);
     	  		 	 	 	 }
     	  		 	 	 	 
     	  		 	 	 	 carrierFlagSet.routeRunStatus = 0;
     	  		 	 	 	 break;
     	  		 	 	 
     	  		 	 	 case 10:    //��֡��ȡDAU��Ϣ
    	  		 	 	   //carrierFlagSet.numOfSalveNode = tmpBuf[1] | tmpBuf[2]<<8;
                   if (tmpBuf[3]==0)
                   {
    	  		 	 	  	   if (carrierFlagSet.querySlaveNode==1)
    	  		 	 	  	   {
  	                      carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
  	                   
  	                      if (debugInfo&PRINT_CARRIER_DEBUG)
  	                      {
  	                        printf("��Ѹ������ģ��ӽڵ��б�(·�ɲ�ѯ)\n");
  	                        tmpSalveNode = carrierSlaveNode;
  	                        while(tmpSalveNode!=NULL)
  	                        {
    	  		 	 	  	          printf("�ӽڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpSalveNode->addr[5],tmpSalveNode->addr[4],tmpSalveNode->addr[3],tmpSalveNode->addr[2],tmpSalveNode->addr[1],tmpSalveNode->addr[0]);
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
       	  		 	 	  	   printf("�ӽڵ��ַ:%02x%02x%02x%02x%02x%02x\n",tmpBuf[i*7+9],tmpBuf[i*7+8],tmpBuf[i*7+7],tmpBuf[i*7+6],tmpBuf[i*7+5],tmpBuf[i*7+4]);
       	  		 	 	  	 }
       	  		 	 	  	 
       	  		 	 	  	 //�ز��ӽڵ��ַΪȫ0,����Ϊ�����к����ӽڵ��ַ,ֹͣ��ȡ
                        if (tmpBuf[i*7+8]==0x0 && tmpBuf[i*7+7]==0x0 && tmpBuf[i*7+6]==0x0 && tmpBuf[i*7+5]==0x0 && tmpBuf[i*7+4]==0x0 && tmpBuf[i*7+3]==0x0)
                        {
                        	 carrierFlagSet.numOfSalveNode = carrierFlagSet.synMeterNo;
    	  		 	 	  	       
    	  		 	 	  	       if (carrierFlagSet.querySlaveNode==1)
    	  		 	 	  	       {
  	                         carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
  	                   
  	                         if (debugInfo&PRINT_CARRIER_DEBUG)
  	                         {
  	                           printf("�������ز�ģ��ӽڵ��б�,�򷵻صĴӽڵ��ַΪȫ0\n");
  	                         }
    	  		 	 	  	       }
    	  		 	 	  	       
    	  		 	 	  	       break;
                        }
                        
                        tmpSalveNode = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                        tmpSalveNode->mpNo = carrierFlagSet.synMeterNo;           //�ӽڵ����
                        memcpy(tmpSalveNode->addr,tmpBuf+i*7+4,6); //�ӽڵ��ַ
                        memcpy(tmpSalveNode->info,tmpBuf+i*7+10,1); //�ӽڵ���Ϣ
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
                      
                      //������شӽڵ�ĸ������ڵ��ڴӽڵ�������,ֹͣ��ȡ
                      if (carrierFlagSet.synMeterNo > carrierFlagSet.numOfSalveNode)
                      {
  	                      carrierFlagSet.querySlaveNode = 2;   //��ȡ�ز��ӽڵ����
  	                   
  	                      if (debugInfo&PRINT_CARRIER_DEBUG)
  	                      {
  	                        printf("���شӽڵ�ĸ������ڵ��ڴӽڵ�������,ֹͣ��ȡ\n");
  	                      }
                      }
                   }
     	  		 	 	 	 break;
     	  		 	 }
     	  		 	 break;
     	  	}
     	  	
     	  	carrierFlagSet.cmdContinue = 1;      //���Լ�����������
     	  	carrierFlagSet.sending     = 0;      //���ڷ���������Ϊδ����
     	  	
     	  	carrierFlagSet.numOfCarrierSending = 0;
     	  	
     	  	if (debugInfo&PRINT_CARRIER_DEBUG)
     	  	{
     	  		//printf("�ز�/���߶˿��з���,���㷢�ͼ���\n");
     	  	}
     	  	break;
     }
     
     if (libLen<recvLen)
     {
     	 //printf("�ϴ�û�������,libLen=%d,recvLen=%d\n", libLen, recvLen);
     	 goto repeatRecv;
     }
     else
     {
     	 //printf("���մ�������\n");
     }
     
#ifdef LM_SUPPORT_UT
breakCarrierRecv:
#endif

     usleep(50);
  }
}
#endif

/*******************************************************
��������:whichItem
��������:�������ڳ�����Ʋ���(��վ����ֵ)�������һ��Ԫ��
���ú���:
�����ú���:
�������:
�������:
����ֵ:�����±�
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
��������:initCopyDataBuff
��������:��ʼ��������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
      //����û�иò����������
      return FALSE;
    }
  }
  else
  {
 #endif
   
    if (selectF10Data(copyCtrl[port].cpLinkHead->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
    {
      //����û�иò����������
      return FALSE;
    }
      
   #ifdef LIGHTING
    //2014-12-23,��Ϊ���ֽڵ����λbit7���ܱ�ʾ��������,����ȥ��
    //2015-01-21,���ֽڵ�bit6���ܱ�ʾ���빦��,����ȥ��
    meterConfig.rateAndPort &= 0x3f;
   #endif
      
    if ((meterConfig.rateAndPort&0xe0)==0x0)  //Ĭ������
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
						printf("%s==>RS485 %d����ΪĬ������9600-8-n-1\n", __func__, port);
					}
          break;
					
				case LIGHTING_TH:
					copyCtrl[port].backupCtrlWord = 0xC3; 										//9600bps,8-n-1 
					
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d����ΪĬ������9600-8-n-1\n", __func__, port);
					}
					break;
       #endif 
	    	case MODBUS_HY:      //ɽ����Զ��2017-5-24
	    	case MODBUS_ASS:     //������ɭ˼,2017-5-24
				case MODBUS_XY_F:    //�Ϻ���ҵ�๦�ܱ�,2017-9-21
				case MODBUS_XY_UI:   //�Ϻ���ҵ��ѹ������,2017-9-21
				case MODBUS_SWITCH:	 //MODBUS������ģ��,2017-9-22
				case MODBUS_XY_M:    //�Ϻ���ҵ���ģ��,2017-10-10
				case MODBUS_MW_F: 	 //�ɶ�����๦�ܱ�,2018-03-01
				case MODBUS_MW_UI:	 //�ɶ������ѹ������,2018-03-01
				case MODBUS_JZ_F: 	 //�Ϻ������๦�ܱ�,2018-03-01
				case MODBUS_JZ_UI:	 //�Ϻ�������ѹ������,2018-03-01				
				case MODBUS_WE6301_F://��˹��WE6301�๦�ܱ�,2018-05-03
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d����ΪMODBUSĬ������9600-8-n-1\n", __func__, port);
					}
		  		copyCtrl[port].backupCtrlWord = 0xC3;    //9600bps,8-n-1 
		  		break;
					
				case MODBUS_PMC350_F: 	 //�����е�PMC350�๦�ܱ�,2018-04-27
					copyCtrl[port].backupCtrlWord = 0xCb; 	 //9600bps,8-e-1
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d����ΪMODBUSĬ������9600-8-e-1\n", __func__, port);
					}
					break;
					
        default:
          copyCtrl[port].backupCtrlWord = DEFAULT_CTRL_WORD_2400;   //2400bps,8-e-1
					if (debugInfo&METER_DEBUG)
					{
						printf("%s==>RS485 %d����ΪĬ������2400-8-e-1\n", __func__, port);
					}
          
          //copyCtrl[port].backupCtrlWord = 0xc3;    //9600bps,8-n-1
					//if (debugInfo&METER_DEBUG)
					//{
					//	printf("%s==>RS485 %d����ΪĬ������9600-8-n-1\n", __func__, port);
					//}
          break;
      }
    }
    else
    {
      switch (meterConfig.protocol)
      {
        case EDMI_METER:
          //��У��
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
				printf("%s==>RS485 %d���ÿ�����Ϊ%x02X\n", __func__, port, copyCtrl[port].backupCtrlWord);
			}
    }
      
 #ifndef LM_SUPPORT_UT
  }
 #endif
   
  //����˿ڰ���վ����ֵ��������
  switch(meterConfig.rateAndPort&0x1f)
  {
   #ifdef RS485_1_USE_PORT_1
    case  PORT_NO_1:    //485�˿ں�1
   #else
    case  PORT_NO_2:    //485�˿ں�2
   #endif
     #ifdef  WDOG_USE_X_MEGA
      buf[0] = 0x01;                         //xMega�˿�1
      buf[1] = copyCtrl[port].backupCtrlWord;//�˿�����
      sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
     
      if (debugInfo&METER_DEBUG)
      { 
        printf("���ö˿�1����\n");
      }
     #else
      oneUartConfig(fdOfttyS2, copyCtrl[port].backupCtrlWord);
     #endif
        
      mcp.port = 1;
        
      if (meterConfig.protocol==ABB_METER)
      {
        abbHandclasp[0] = 1;    //ABB����δ�ɹ�
      }
      else
      {
        abbHandclasp[0] = 0;
      }
      break;

   #ifdef RS485_1_USE_PORT_1
    case PORT_NO_2:    //485�˿ں�2
   #else
    case PORT_NO_3:    //485�˿ں�3
   #endif
     #ifdef WDOG_USE_X_MEGA
      buf[0] = 0x02;                          //xMega�˿�2
      buf[1] = copyCtrl[port].backupCtrlWord; //�˿�����
      sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
      
      if (debugInfo&METER_DEBUG)
      {
        printf("���ö˿�2����\n");
      }
     #else
      //������֧�ּ���,���2·�����ΪttyS4;
      //���������ּ���,���2·�����ΪttyS3
      #ifdef SUPPORT_CASCADE
       oneUartConfig(fdOfttyS4, copyCtrl[port].backupCtrlWord);
      #else
       oneUartConfig(fdOfttyS3, copyCtrl[port].backupCtrlWord);
      #endif
     #endif
    
      mcp.port = 2;

      if (meterConfig.protocol==ABB_METER)
      {
    	abbHandclasp[1] = 1;    //ABB����δ�ɹ�
      }
      else
      {
    	abbHandclasp[1] = 0;
      }        
      break;
        
   #ifdef RS485_1_USE_PORT_1
    case PORT_NO_3:    //485�˿ں�3
   #else
    case PORT_NO_4:    //485�˿ں�4
   #endif

    #ifdef WDOG_USE_X_MEGA
      buf[0] = 0x03;                          //xMega�˿�2
      buf[1] = copyCtrl[port].backupCtrlWord; //�˿�����
      sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
      
      if (debugInfo&METER_DEBUG)
      {
        printf("���ö˿�3����\n");
      }
     #endif
        
      mcp.port = 3;

      if (meterConfig.protocol==ABB_METER)
      {
        abbHandclasp[2] = 1;    //ABB����δ�ɹ�
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
          printf("�ز�������:%d,�û������=%d,С���=%d\n", copyCtrl[port].tmpCpLink->mp, meterConfig.bigAndLittleType>>4, meterConfig.bigAndLittleType&0xf);
        }
       #endif
         
        if (initType==HOUR_FREEZE_DATA)
        {
          mcp.protocol = HOUR_FREEZE_2007;
        }
        else
        {
          queryMeterStoreInfo(meterConfig.measurePoint,meterInfo);
           
          //�������(1-�������ܱ�,2-���౾�طѿر�,3-����Զ�̷ѿر�,4-�������ܱ�,5-���౾�طѿر�,6-����Զ�̷ѿر�,7-485�����(���ݳ�����ȫ),8-�ص��û�
          switch(meterInfo[0])
          {
           	case 1:  //�������ܱ�
              switch(denizenDataType)
              {
                case 0x55:    //����ʵʱ����(�ܼ�������)
                  mcp.protocol = SINGLE_PHASE_645_2007_TARIFF;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("�������ܱ�(����ʵʱ����(�ܼ�������))\n");
                  }
                  break;

                case 0xaa:    //����ʵʱ����(��ʾֵ)
                  mcp.protocol = SINGLE_PHASE_645_2007_TOTAL;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("�������ܱ�(����ʵʱ����(��ʾֵ))\n");
                  }
                  break;
                   
                default:
                  mcp.protocol = SINGLE_PHASE_645_2007;
                  if (debugInfo&PRINT_CARRIER_DEBUG)
                  {
                    printf("�������ܱ�\n");
                  }
                  break;
              }
           	  break;
           	 
           	case 2:  //���౾�طѿر�
              mcp.protocol = SINGLE_LOCAL_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("���౾�طѿر�\n");
              }
       	 	  break;
           	 
           	case 3:  //����Զ�̷ѿر�
              mcp.protocol = SINGLE_REMOTE_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("���������Զ�̷ѿر�\n");
              }
              break;
             	 
            case 4:  //�������ܱ�
              mcp.protocol = THREE_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("�������ܱ�\n");
              }
              break;
             
            case 5:  //���౾�طѿر�
              mcp.protocol = THREE_LOCAL_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("���౾�طѿر�\n");
              }
              break;
             
            case 6:  //����Զ�̷ѿر�
              mcp.protocol = THREE_REMOTE_CHARGE_CTRL_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("����Զ�̷ѿر�\n");
              }
              break;
             	 
            case 7:  //07�����,������ȫ
              mcp.protocol = DLT_645_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("07�����\n");
              }
              break;
             	 
            case 8:  //07�ص��û�
              mcp.protocol = KEY_HOUSEHOLD_2007;
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("07�ص��û���\n");
              }
              break;
          }
        }
        copyCtrl[port].protocol = 2;
      }
       
      //����ɼ�����ַΪȫ0,���ز�Ŀ�Ľڵ�Ϊ���ַ,����Ŀ�Ľڵ�Ϊ�ɼ�����ַ
      if (meterConfig.collectorAddr[0]==0x00 && meterConfig.collectorAddr[1]==0x00
       	  && meterConfig.collectorAddr[2]==0x00 && meterConfig.collectorAddr[3]==0x00
       	   && meterConfig.collectorAddr[4]==0x00 && meterConfig.collectorAddr[5]==0x00
       	  )
      {
        memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
        memcpy(copyCtrl[port].destAddr, meterConfig.addr, 6);          //ly,2012-01-10,add
        copyCtrl[port].ifCollector = 0;      //�޲ɼ�����ַ
      }
      else
      {
        memcpy(carrierFlagSet.destAddr, meterConfig.collectorAddr, 6);
        memcpy(copyCtrl[port].destAddr, meterConfig.collectorAddr, 6); //ly,2012-01-10,add
        copyCtrl[port].ifCollector = 1;      //�вɼ�����ַ
      }

      //����ʱ��Ϊϵͳ��ǰʱ��
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
            case 1:    //С���Ϊ1
              mcp.protocol = SINGLE_PHASE_645_1997;
              break;
             
            case 13:   //С���Ϊ13ֻ�����������޹�����ʾֵ
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
            case 1:    //С���Ϊ1
              mcp.protocol = SINGLE_PHASE_645_2007;
              break;
             
            case 13:   //С���Ϊֻ�����������޹��ܵ���ʾֵ
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
       
      //��ǰʱ��Ϊ���˿ڱ��γ���ʱ��
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
          printf("��%02x%02x%02x%02x%02x%02x��ǰ����\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;

      case LAST_MONTH_DATA:
        mcp.copyDataType = LAST_MONTH_DATA;
      
        if (debugInfo&METER_DEBUG)
        {
          printf("��%02x%02x%02x%02x%02x%02x��������\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;

      case LAST_DAY_DATA:
        mcp.copyDataType = LAST_DAY_DATA;
      
        if (debugInfo&METER_DEBUG)
        {
          printf("��%02x%02x%02x%02x%02x%02x���ܱ���һ�ն�������\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;
        
    	case HOUR_FREEZE_DATA:
        mcp.copyDataType = HOUR_FREEZE_DATA;
        if (debugInfo&METER_DEBUG)
        {
          printf("��%02x%02x%02x%02x%02x%02x���ܱ����㶳������\n", meterConfig.addr[5], meterConfig.addr[4], meterConfig.addr[3], meterConfig.addr[2], meterConfig.addr[1], meterConfig.addr[0]);
        }
        break;
        
      default:
        if (debugInfo&METER_DEBUG)
        {
         printf("ָ���ĳ�����������δ֪\n");
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
��������:initCopyItem
��������:��ʼ��Ϊ�����ʼֵ
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
BOOL initCopyItem(INT8U port, INT8U ifCopyLastFreeze)
{
   INT16U tmpData;
   BOOL   ret;

   copyCtrl[port].currentCopyTime = sysTime;        //���γ���ʱ��
    
   //��������������
  #ifdef PLUG_IN_CARRIER_MODULE 
   if (!(carrierFlagSet.routeLeadCopy==1 && port==4))
   {
  #endif
     
     switch (ifCopyLastFreeze)
     {
       case 1:   //��������
   	     copyCtrl[port].copyDataType = LAST_MONTH_DATA;
   	     ret = initCopyDataBuff(port, LAST_MONTH_DATA);
   	     break;
   	     
   	   case 2:   //��һ���ն�������
   	     copyCtrl[port].copyDataType = LAST_DAY_DATA;
   	     ret = initCopyDataBuff(port, LAST_DAY_DATA);
   	     break;
  
   	   case 3:   //���㶳������
   	     copyCtrl[port].copyDataType = HOUR_FREEZE_DATA;
   	     ret = initCopyDataBuff(port, HOUR_FREEZE_DATA);
   	     break;
   	     
       default:  //�ɼ���ǰ����
         copyCtrl[port].copyDataType = PRESENT_DATA;
         ret = initCopyDataBuff(port, PRESENT_DATA);
         break;
     }

     //if (meterDeviceConfig.meterNumber>0)
     //{
        //��¼�ܳ������
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
          
          //��ΪҪ�����㶳�����ݲ���Ҫ�ϱ�,����Ҫ��ÿСʱ��1�ֿ�ʼ�����㶳������
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
            if (copyCtrl[port].nextCopyTime.hour==0x0)
            {
              printf("�ز�/���߶˿�0��ĳ���ʱ���0��10�ֿ�ʼ\n");
            }
            else
            {
              printf("�ز�/���߶˿ڳ���ʱ��ΪÿСʱ��1�ֿ�ʼ\n");
            }
          }
        }
        else
        {
         #ifdef LIGHTING
          //2016-07-02,����������485�ӿڵĳ�������Ϊ��10��Ϊ��λ
          copyCtrl[port].nextCopyTime = nextTime(sysTime, 0, teCopyRunPara.para[tmpData].copyInterval*10);
          if (debugInfo&METER_DEBUG)
          {
            printf("������:%d��\n", teCopyRunPara.para[tmpData].copyInterval*10);
          }
         #else
          copyCtrl[port].nextCopyTime = nextTime(sysTime, teCopyRunPara.para[tmpData].copyInterval, 0);
          copyCtrl[port].nextCopyTime.second = 0x0;
          if (debugInfo&METER_DEBUG)
          {
            printf("������:%d\n", teCopyRunPara.para[tmpData].copyInterval);
          }
         #endif
        }
     }
     else
     {
        copyCtrl[port].nextCopyTime = nextTime(sysTime,5,0);
        
        if (debugInfo&METER_DEBUG)
        {
          printf("������δ����,��Ϊ5����");
        }
     }
     
     //���������������485�˿�
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
��������:copy485Failure
��������:485����ʧ���¼�
���ú���:
�����ú���:
�������:startOrStop =1,����  =2,�ָ�
�������:
����ֵ��void
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
	   //�������ͳ������
     searchMpStatis(sysTime, &meterStatisRecord, mp, 1);
     
     //�Ѿ���¼��ʧ��,�ȴ��ָ�
     if ((meterStatisRecord.mixed&0x02) && startOrStop==1)
     {
     	  return FALSE;
     }

     tmpTime = timeHexToBcd(sysTime);
     if (readMeterData(dataBuff, mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0)==TRUE)
     {
         eventData[0] = 31;
         eventData[1] = 24;
      
         //����ʱ��
         eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
         eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
         eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
         eventData[7] = sysTime.year/10<<4   | sysTime.year%10;
   
         //�����㼰��/ֹ��־
         eventData[8] = mp&0xff;
         eventData[9] = mp>>8&0xff;
         
         //����
         if (startOrStop==1)
         {
         	  eventData[9] |= 0x80;
         	  
         	  meterStatisRecord.mixed |= 0x02;
            
            saveMeterData(mp, port, timeHexToBcd(sysTime), (INT8U *)&meterStatisRecord, STATIS_DATA, 0xfe,sizeof(METER_STATIS_EXTRAN_TIME));
         }
         
         //�ָ�
         if (startOrStop==2)
         {
         	  meterStatisRecord.mixed &= 0xfd;         	  
         }
         
         //���һ�γ���ɹ�ʱ��
         eventData[10] = tmpTime.minute;
         eventData[11] = tmpTime.hour;
         eventData[12] = tmpTime.day;
         eventData[13] = tmpTime.month;
         eventData[14] = tmpTime.year;
         
         //���һ�γ���ɹ������й���ʾֵ
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

         //���һ�γ���ɹ������й���ʾֵ
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
��������:initPortMeterLink
��������:��ʼ��Ϊ��������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void

��ʷ��
    1.ly,2012-08-09,�޳�ȫ0�ı��ַ
***************************************************/
struct cpAddrLink * initPortMeterLink(INT8U port)
{
	 struct cpAddrLink   *tmpHead, *tmpNode,*prevNode;
	 METER_DEVICE_CONFIG meterConfig;
	 sqlite3_stmt        *stmt;
	 char                *sql;
	 INT16U              result;
	 INT8U               tmpPort;
	 INT16U              numOfMp;    //���������
	 
	 tmpHead = NULL;
	 
	 prevNode = tmpHead;
	 
	 //��ѯ���б�������
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
      		 printf("������%d��ַΪȫ��,�޳�\n", meterConfig.measurePoint);
      	}
      }
      else
      {
        tmpNode = (struct cpAddrLink *)malloc(sizeof(struct cpAddrLink));
        tmpNode->mpNo = meterConfig.number;                         //���������
        tmpNode->mp   = meterConfig.measurePoint;                   //�������
        tmpNode->protocol = meterConfig.protocol;                   //Э��
        memcpy(tmpNode->addr,meterConfig.addr,6);                   //ͨ�ŵ�ַ
        memcpy(tmpNode->collectorAddr,meterConfig.collectorAddr,6); //�ɼ�����ַ
                                                                    //  ����������a)���ƿ��������ڱ�ʾ������ռ�ձȵĶ�Ӧ��ϵ�е�����
                                                                    //            b)�������Ƶ����ڱ�ʾ��2ֻĩ�˵��ز�ͨ�ŵ��ַ
                                                                    //            c)��·���Ƶ����ڱ�ʾ���Ⱥ�γ��,2016-02-15
        tmpNode->port = meterConfig.rateAndPort&0x1f;               //�˿�
        tmpNode->bigAndLittleType = meterConfig.bigAndLittleType;   //�û�����ż��û�С���
                                                                    //  ����������a)���ƿ��Ƶ����ڱ�ʾPWMƵ��
                                                                    //            b)�������Ƶ�
                                                                    //              bit5-0,��ʾ��������
                                                                    //              bit7,6��ʾ�ز�����ʱ��ĵ�2λ
                                                                    //            c)��·���Ƶ����ڱ�ʾ�����Ƶ�Ŀ���ģʽ,2016-02-15
        tmpNode->flgOfAutoCopy = 0;                                 //·�������Զ�������־(�ز�ģ����Ч)
       
       #ifdef LIGHTING
        tmpNode->ctrlTime = meterConfig.numOfTariff;                //����ʱ�κ�(�洢�ڷ����ֶ�)
        
        if (LIGHTING_DGM==tmpNode->protocol)
        {
          memcpy(tmpNode->lddt1st, meterConfig.password, 6);        //�������Ƶ��1ֻĩ�˵��ƿ�����

          tmpNode->funOfMeasure = tmpNode->bigAndLittleType & 0x3f; //ĩ�˵��ƿ�������������
          
          //�ϵ���ز�����ʱ��
          tmpNode->joinUp = (meterConfig.rateAndPort&0xc0)>>4 
                           | (tmpNode->bigAndLittleType&0xc0)>>6;
          tmpNode->joinUp *= 5;
          
          if (debugInfo&METER_DEBUG)
          {
            printf("���Ƶ�%d(%02x-%02x-%02x-%02x-%02x-%02x)����������=%d,����ʱ��=%d\n", tmpNode->mp, tmpNode->addr[5], tmpNode->addr[4], tmpNode->addr[3], tmpNode->addr[2], tmpNode->addr[1], tmpNode->addr[0], tmpNode->funOfMeasure, tmpNode->joinUp);
          }
          
          if (0==tmpNode->funOfMeasure)
          {
          	tmpNode->funOfMeasure = 2;
            if (debugInfo&METER_DEBUG)
            {
              printf("  - ���������������=%d\n", tmpNode->funOfMeasure);
            }
          }
          if (0==tmpNode->joinUp)
          {
          	tmpNode->joinUp = 5;
          	
            if (debugInfo&METER_DEBUG)
            {
              printf("  - ������Ľ���ʱ��=%d\n", tmpNode->joinUp);
            }
          }
        }
        else
        {
          tmpNode->funOfMeasure = (meterConfig.rateAndPort&0x80)?1:0; //�Ƿ��м�������
          tmpNode->joinUp = (meterConfig.rateAndPort&0x40)?1:0;       //�Ƿ����
        	                                                            //  ���ƿ����� - �Ƿ����ƾ�
                                                                      //  ��·���Ƶ� - �Ƿ���뷴��ң��
        }
        
        memcpy(tmpNode->duty, meterConfig.password, 6);               //���ƿ����������ȵ�ռ�ձ�
        if (
        	  LIGHTING_LS==tmpNode->protocol 
        	   || LIGHTING_DGM==tmpNode->protocol
        	     || AC_SAMPLE==tmpNode->protocol
        	      || LIGHTING_XL==tmpNode->protocol
        	 )
        {
        	memset(tmpNode->duty, 0x00, 6);
        }
        
        //2016-03-17,��վ���õľ�γ�ȿ���ģʽʱ��/��բ΢��ֵ
        if (LIGHTING_XL==tmpNode->protocol)
        {
        	tmpNode->duty[4] = meterConfig.password[0];    //��բ΢��ֵ
        	tmpNode->duty[5] = meterConfig.password[1];    //��բ΢��ֵ
        }
        
        tmpNode->startCurrent = meterConfig.mixed;                  //��������,2014-11-03,add
                                                                    //  ���ƿ���������������,�����жϵ��ƹ���
                                                                    //  ��������������·�������ֽ�,�����ж���·����

        tmpNode->status = 0xfe;
        tmpNode->statusUpdated = 0;                                 //2015-10-26,Add
        tmpNode->lineOn = 0;                                        //2015-11-09,Add

        tmpNode->copySuccess = FALSE;                               //2015-10-26,Add,������������Ҫ��һ��,�������ɳ����ɹ�ͳ�ƴ���

        tmpNode->lddtRetry = 0;                                     //����LDDT���Դ���
        
        tmpNode->offLineRetry = 0;                                  //2015-11-11,Add,����LDD���Դ���
        
        tmpNode->lcOnOff = 0xfe;                                    //2016-01-20,Add,��ص�Դ����
        tmpNode->lcDetectOnOff = 0xfe;                              //2016-01-21,Add,���ݹ�ؼ��ĵ�Դ���ر�־
				tmpNode->lcProtectTime.year = 0xff;                         //2017-01-09,Add,��طֺ�բ����ʱ��
        
        tmpNode->gateOn = 0xfe;                                     //2016-01-20,Add,��·���Ƶ�ӷ�բ��ɺ�բ
				
				tmpNode->softVer = 0;                                       //2016-11-04,Add,����汾��
        
       #endif
       
        tmpNode->next = NULL;
         
        numOfMp++;
   
   		  //if (debugInfo&METER_DEBUG)
   		  //{
   		  //  printf("������%d,Э��=%d\n", tmpNode->mp, tmpNode->protocol);
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
     //ly,2012-01-10,ע��
     //printf("�˿�%d���ò�����%d���������������\n",port+1,numOfMp);
   }
   
   return tmpHead;
}

/***************************************************
��������:findAcPn
��������:���ҽ��ɲ������
���ú���:
�����ú���:
�������:
�������:
����ֵ�����ɵĲ������,0Ϊû������
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
     printf("�ҵ����ɲ������Ϊ%d\n", tmpPn);
   }
   
   return tmpPn;
}


/***************************************************
��������:reSetCopyTime
��������:�������ó���ʱ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
      	  printf("����ȷ������ʱ��:�˿�%d������:δ����\n",i+1);
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
            printf("�ز��˿ڳ������̶�ΪÿСʱ1��\n");
          }
          
          //�ز��˿ڳ������̶�ΪÿСʱ1��,0���Ϊ�˵ȴ����ڵ��ܱ�ʱ�������ɵĶ������ݵ��γ�Ϊ10�ֿ�ʼ
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
          //  printf("����ȷ������ʱ��:������:%d\n",teCopyRunPara.para[ret].copyInterval);
          //}
         
          ////�������һ�γ���ʱ�������һ�γ���ʱ��
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
          		printf("�˿�%d���ڳ���,ֹͣ����\n",i+1);
          	}
          }
          
          if (debugInfo&METER_DEBUG)
          {
          	printf("�˿�%d��ǰʱ�����һ��0�뿪ʼ����\n",i+1);
          }
          
          copyCtrl[i].nextCopyTime = nextTime(sysTime, 1, 0);
          copyCtrl[i].nextCopyTime.second = 0x0;
        }
     }

     if (debugInfo&METER_DEBUG)
     {
       printf("����ȷ������ʱ��:�˿�%d�´γ���ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", i+1, 
              copyCtrl[i].nextCopyTime.year,copyCtrl[i].nextCopyTime.month,
              copyCtrl[i].nextCopyTime.day,copyCtrl[i].nextCopyTime.hour,
              copyCtrl[i].nextCopyTime.minute,copyCtrl[i].nextCopyTime.second);
     }
  }
}

#ifdef PLUG_IN_CARRIER_MODULE

/***************************************************
��������:��λ�ز���־
��������:resetCarrierFlag
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
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
  carrierFlagSet.routeLeadCopy  = 0;   //Ĭ��Ϊ��������������

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
��������:sendCarrierFrame
��������:���ز�ͨ��ģ�鷢�ͳ�������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void sendCarrierFrame(INT8U port, INT8U *pack, INT16U length)
{
  INT8U i;   
  INT8U meterFramexx[254];

  //if (port==PORT_POWER_CARRIER)
  //{
  //   meterFramexx[0] = copyCtrl[4].protocol;  //��Լ����
  //   meterFramexx[1] = length;
  //   memcpy(&meterFramexx[2], pack, length);
             	 	  
  //   gdw3762Framing(ROUTE_DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
  //}

  //if (port==PORT_POWER_CARRIER)
  if (port<PORT_POWER_CARRIER+3)  //ly,2012-01-09
  {
    //����Ƕ���I�Ͳɼ���
    if ((carrierModuleType==EAST_SOFT_CARRIER) && (copyCtrl[4].ifCollector==1))
    {
      meterFramexx[0] = copyCtrl[4].protocol;  //��Լ����
      meterFramexx[1] = length+6;
      
      if (copyCtrl[4].protocol==1)    //97��Լ��
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
    else    //�ز����������I�Ͳɼ���
    {
      meterFramexx[0] = copyCtrl[port-27].protocol;  //��Լ����
      
      meterFramexx[1] = length;
      memcpy(&meterFramexx[2], pack, length);
    }
    
    //����ģ��������ת��
    if (carrierModuleType==HWWD_WIRELESS
    	  || carrierModuleType==SR_WIRELESS 
    	   || carrierModuleType==RL_WIRELESS
      	  || carrierModuleType==FC_WIRELESS
      	   || carrierModuleType==SC_WIRELESS
    	 )
    {
      gdw3762Framing(DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
    }
    else                                   //�ز�ģ���ü���ز��ӽڵ�
    {
      if (carrierFlagSet.routeLeadCopy==1)  //·����������
      {
      	//if (carrierFlagSet.workStatus==5)
      	if (carrierFlagSet.workStatus==5 || carrierFlagSet.workStatus==6)
      	{
          if (debugInfo&PRINT_CARRIER_DEBUG)
  	  	  {
  	  	   	printf("·����������:�㳭��ת���ȶ��ü���ز��ӽڵ�����\n");
  	  	  }
          gdw3762Framing(ROUTE_DATA_FORWARD_3762, 1, carrierFlagSet.destAddr, meterFramexx);
      	}
      	else
      	{
          if (debugInfo&PRINT_CARRIER_DEBUG)
  	  	  {
  	  	   	 printf("·����������:������Գ���\n");
  	  	  }
      	  meterFramexx[0] = 0x02;         //���Գ���
          if ((carrierModuleType==EAST_SOFT_CARRIER) && (copyCtrl[port-27].ifCollector==1))
          {
            meterFramexx[length+8] = 0x00;     //�ز��ӽڵ㸽���ڵ�����Ϊ0
          }
          else
          {
            meterFramexx[length+2] = 0x00;     //�ز��ӽڵ㸽���ڵ�����Ϊ0
            
            //�ز���λ(�����ز���Ҫ),ly,2012-01-09
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
��������:dotCopyReply
��������:�㳭�ظ�
���ú���:
�����ú���:
�������:INT8U hasData,�Ƿ������ݷ���(1-����,0-�쳣Ӧ������޷���)
�������:
����ֵ��void
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
    //��������������
    if (copyCtrl[4].meterCopying==TRUE && carrierFlagSet.routeLeadCopy==0)
    {
      if (copyCtrl[4].ifCopyLastFreeze==3)
      {
        //����Ƿ���δ�ɼ���Сʱ��������
        checkHourFreezeData(copyCtrl[4].tmpCpLink->mp, copyCtrl[4].dataBuff);
      }
        
      initCopyDataBuff(4, copyCtrl[4].ifCopyLastFreeze);
    }

    switch (pDotCopy->dataFrom)
    {
      case DOT_CP_SINGLE_MP:    //������������㳭
      	if (hasData==1)
      	{
      	  singleMeterCopyReport(pDotCopy->data);
      	}
      	else
      	{
      	  pDotCopy->dotRecvItem = pDotCopy->dotTotalItem;
      	  singleMeterCopyReport(NULL);
      	}
      	 
      	//·����������,�ָ�����
      	if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
      	{
       	  carrierFlagSet.workStatus = 6;
          carrierFlagSet.reStartPause = 3;
          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
       
          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,������������㳭 - �㳭���,·����������,%d���ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
     	    }
        }
         
        free(pDotCopy);
        pDotCopy = NULL;
         
      	break;
      	 	 
      case DOT_CP_ALL_MP:    //����ȫ�������㳭
      	if (hasData==1)
      	{
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,����ȫ�������㳭 - ������%d�㳭�ɹ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, prevMpCopy->mp);
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
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,����ȫ�������㳭 - ������%d�㳭ʧ��\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, prevMpCopy->mp);
     	    }
        }
      	 	 
      	if (prevMpCopy->next==NULL)
      	{
      	  //·����������,�ָ�����
      	  if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
      	  {
         	  carrierFlagSet.workStatus = 6;
            carrierFlagSet.reStartPause = 3;
            copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
         
            carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,����ȫ�������㳭 - �㳭���,·����������,%d���ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
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
          
          //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
          //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
          //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
          pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);

          pDotCopy->dotRecvItem = 0;
          pDotCopy->dotResult   = RESULT_NONE;

     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,����ȫ�������㳭 - ������λ->������%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, prevMpCopy->mp);
     	    }
      	}
      	break;

      case DOT_CP_NEW_METER:      //�����㳭���ֵ��±�
      	if (hasData==1)
      	{
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,�±�㳭�ɹ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,�����㳭���ֵ��±� - �㳭���,·����������,%d���ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
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

          //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
          //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
          //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
          pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
      	}
      	break;
      	 
      case DATA_FROM_GPRS:    //��վ����㳭(����Զ��ͨ��ģ��)
      case DATA_FROM_LOCAL:   //���Ա���ͨ�ſ�
        tmpBuf[0] = 0x68;            //֡��ʼ�ַ�
      
        tmpBuf[5] = 0x68;            //֡��ʼ�ַ�
     
        tmpBuf[6] = 0x88;            //�����ֽ�10001000(DIR=1,PRM=0,������=0x8)
     
        //��ַ
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
              tmpBuf[13] = 0x60 | rSeq;  //01100001 ��֡
               
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - ��֡\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
            else
            {
             	tmpBuf[13] = 0x40 | rSeq;  //��֡
               
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - ��֡\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
             
            dotReplyStart = 1;
          }
          else
          {
            if (pDotFn129->next==NULL)
            {
              tmpBuf[13]   = 0x20 | rSeq;  //ĩ֡

              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - ĩ֡\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
            else
            {
              tmpBuf[13]   = 0x00 | rSeq;  //�м�֡
              
              if (debugInfo&PRINT_CARRIER_DEBUG)
              {
                printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - �м�֡\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
              }
            }
          }
        }
        else
        {
          tmpBuf[13]   = 0x60 | rSeq;  //01100001 ��֡
           
          if (debugInfo&PRINT_CARRIER_DEBUG)
          {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - ��֡δ֪\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
     	    tmpBuf[frameTail++] = 0x4;  //���ʸ���Ϊ4��
     	   #else
     	    tmpBuf[frameTail++] = 0x0;  //���ʸ���Ϊ0��
     	   #endif
     	 
     	    if (hasData==0)
     	    {
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - 07��㳭����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - 07��㳭�����ظ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
     	    //ly,2011-01-12,97��㳭�ظ�ȡ������,������Ϊ0,
     	    tmpBuf[frameTail++] = 0x0;  //���ʸ���
     	 
     	    if (hasData==0)
     	    {
     	      if (debugInfo&PRINT_CARRIER_DEBUG)
     	      {
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - 97��㳭����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - 97��㳭�����ظ�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	      }
     	    
     	      tmpBuf[frameTail++] = 0x0;
     	      tmpBuf[frameTail++] = pDotCopy->data[0];
     	      tmpBuf[frameTail++] = pDotCopy->data[1];
     	      tmpBuf[frameTail++] = pDotCopy->data[2];
     	      tmpBuf[frameTail++] = pDotCopy->data[3];
     	    }
     	     
     	    //ly,2011-01-12,97��㳭�ظ�ȡ������,������Ϊ0,
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
      	 
      	//2013-12-27,�ж������&& pDotFn129->next==NULL
      	if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE && pDotFn129->next==NULL)
      	{
      	  carrierFlagSet.workStatus = 6;
          carrierFlagSet.reStartPause = 3;
          copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);     
      
          carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, WAIT_RECOVERY_COPY);
     	    if (debugInfo&PRINT_CARRIER_DEBUG)
     	    {
            printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - �㳭���,·����������,%d���ָ�����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, WAIT_RECOVERY_COPY);
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
              printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopyReply,Զ�̵㳭 - ���������%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, pDotFn129->pn);
            }
     	    }
        }
        free(tmpPfn129);
         
        break;
     }
  }
}

/***************************************************
��������:dotCopy
��������:�㳭
���ú���:
�����ú���:
�������:INT8U port,����˿�
�������:
����ֵ��void
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
             printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,�㳭����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
   	 	 	   case DATA_FROM_LOCAL:   //��վ����㳭(���Դ���)
   	 	 	   case DATA_FROM_GPRS:    //��վ����㳭(����Զ��ͨ��Modem)
   	 	 	   case DOT_CP_SINGLE_MP:  //�������㰴���㳭
   	 	 	   case DOT_CP_ALL_MP:     //������㰴���㳭
             if (selectF10Data(pDotCopy->dotCopyMp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
             {
          	   //δ���øò��������Ϣ,�����
          	   dotCopyReply(0);
          	   return FALSE;
             }
             else
             {
               if (debugInfo&METER_DEBUG)
               {
     	           if (debugInfo&PRINT_CARRIER_DEBUG)
     	           {
                   printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,��ʼ�㳭,��ʱ�ȴ�%d��\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, MONITOR_WAIT_ROUTE);
     	           }
               }
               
               //2013-12-30,add,��ֹǰһ��㳭�ڵȴ��ָ�����ʱ��֮ǰ������һ�೭������,��ɷ����ָ�����ͣ
               //          ����ģ�黹��֧�ַ��˵㳭������û��ʱ�ַ��ָ�����,�����ɳ����ܷ�
               if (1==carrierFlagSet.routeLeadCopy && copyCtrl[4].meterCopying==TRUE)
               {
             	   carrierFlagSet.workStatus = 5;
                 carrierFlagSet.reStartPause = 0;

     	           if (debugInfo&PRINT_CARRIER_DEBUG)
     	           {
                   printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,·���������� - ���ü��������ز�ģ�鹤��״̬Ϊ5(������ͣ)\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	           }
               }

             	#ifdef LIGHTING
               
               memcpy(carrierFlagSet.destAddr, meterConfig.addr, 6);
               copyCtrl[4].ifCollector = 0;
               copyCtrl[4].protocol = 2;

 	 	       	   //��645֡
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
               
               //����ɼ�����ַΪȫ0,���ز�Ŀ�Ľڵ�Ϊ���ַ,����Ŀ�Ľڵ�Ϊ�ɼ�����ַ
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

               meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //��֡����
               
              #endif
               
               //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
               //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
               //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
               pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);

               return TRUE;
   	 	 	     }
   	 	 	     break;
   	 	 	   
   	 	 	   case DOT_CP_NEW_METER:   //���������ܱ�
   	 	 	   	 mcp.port     = PORT_POWER_CARRIER;
             mcp.send     = sendCarrierFrame;
             mcp.protocol = SINGLE_PHASE_645_1997;
             
             //ly,2010-03-18,���������ϱ��ı���еĹ�Լ������97�Ļ���07�Ķ�02(��07��Լ),�����޷���������жϵ㳭ʱ�Ĺ�Լ
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
               printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,��ʼ�㳭�±�,��ʱ�ȴ�%d��\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, MONITOR_WAIT_ROUTE);
     	       }
             
             meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);    //��֡����

             //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
             //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
             //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
             pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
             
             //2013-12-27,add,��ֹǰһ��㳭�ڵȴ��ָ�����ʱ��֮ǰ������һ�೭������,��ɷ����ָ�����ͣ
             //          ����ģ�黹��֧�ַ��˵㳭������û��ʱ�ַ��ָ�����,�����ɳ����ܷ�
             if (1==carrierFlagSet.routeLeadCopy && copyCtrl[4].meterCopying==TRUE)
             {
             	 carrierFlagSet.workStatus = 5;
               carrierFlagSet.reStartPause = 0;
     	         
     	         if (debugInfo&PRINT_CARRIER_DEBUG)
     	         {
                 printf("%02d-%02d-%02d %02d:%02d:%02d,dotCopy,�㳭�±�,·���������� - ���ü��������ز�ģ�鹤��״̬Ϊ5(������ͣ)\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
     	         }
             }

             return TRUE;
   	 	 	 	   break;
   	 	   }
   	 	 }
   	 	 else    //�㳭�����ѷ��ͺ��ж��Ƿ�ʱ
   	 	 {
   	 	 	 //�жϳ�ʱ
   	 	 	 if (compareTwoTime(pDotCopy->outTime, sysTime))
   	 	 	 {
 	    		 if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		 {
 	    		   printf("%02d-%02d-%02d %02d:%02d:%02d:dotCopy,��ʱ���쳣Ӧ��,dotRecvItem=%d,dotTotalItem=%d,dotRetry=%d",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pDotCopy->dotRecvItem,pDotCopy->dotTotalItem,pDotCopy->dotRetry);
 	    		 }

 	    	   if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
 	    	   {
             pDotCopy->dotRetry++;
   	    	   if (pDotCopy->dotRetry<1)
   	    	   {
 	    		     if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		     {
 	    		     	 printf(",������һ��");
 	    		     }
               
               meterFraming(PORT_POWER_CARRIER, LAST_COPY_ITEM_RETRY); //�ط���һ��
   	    	   }
   	    	   else
   	    	   {
   	    	     pDotCopy->dotRecvItem++;
   	    	     pDotCopy->dotRetry = 0;
 	    	     
 	    		     if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		     {
 	    		     	 printf(",��֡������������");
 	    		     }
 	    		     
               meterFraming(PORT_POWER_CARRIER, NEW_COPY_ITEM);        //��֡����
             }
             
             //2013-12-27,����Ҫ��ȴ�ʱ��Ϊ90s����
             //  ��˽�����ģ�鶼ͳһ������ȴ����ʱ����
             //  �ڴ�֮ǰ,����΢�ȴ��42��,����ģ�鶼�ǵȴ�30��        
             pDotCopy->outTime = nextTime(sysTime, 0, MONITOR_WAIT_ROUTE);
 	    		   
 	    		   if (debugInfo&PRINT_CARRIER_DEBUG)
 	    		   {
 	    		     printf(",��ʱ�ȴ�%d��", MONITOR_WAIT_ROUTE);
 	    		   }
 	    	   }
 	    	   else
 	    	   {
             switch (pDotCopy->dataFrom)
             {
               case DOT_CP_SINGLE_MP:   //������������㳭��ʱ�㱨�������
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
��������:checkCarrierMeter
��������:����ز��������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
      	   	    printf("������:%d����ɹ�(��ʵʱ����)\n",copyCtrl[4].tmpCpLink->mp);
      	   	  }
      	   }
      	   else
      	   {
      	   	  copyCtrl[4].tmpCpLink->copySuccess = FALSE;
      	   	  
      	   	  if (debugInfo&METER_DEBUG)
      	   	  {
      	   	    printf("������:%d����δ�ɹ�(��ʵʱ����)\n",copyCtrl[4].tmpCpLink->mp);
      	   	  }
      	   }
      	}
      	else
      	{
      	   copyCtrl[4].tmpCpLink->copySuccess = FALSE;
      	   
           //2012-08-28,add,���û��ʵʱ���ݲ����Ƿ��ж�������
           if (checkLastDayData(copyCtrl[4].tmpCpLink->mp, 0, sysTime, 1)==TRUE)
           {
      	     copyCtrl[4].tmpCpLink->copySuccess = TRUE;
           }
           else
           {
      	     if (debugInfo&METER_DEBUG)
      	     {
      	       printf("������:%d����δ�ɹ�(����)\n",copyCtrl[4].tmpCpLink->mp);
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
��������:checkHourFreezeData
��������:��鵱�����㶳�������Ƿ���ȫ,�����ȱʧ��ɼ�
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
INT8U checkHourFreezeData(INT16U pn, INT8U *lostBuff)
{
	 DATE_TIME tmpTime;
   INT8U     tmpReadBuff[LENGTH_OF_ENERGY_RECORD];
   INT16U    i, j, tmpPn;
   INT8U     tmpHour, k, tmpPnx, tmpData;
   BOOL      ifFound;
   
   //�����������Ƿ����øò���������򼰷���ʾֵ����,�����òŲɼ����㶳������
   ifFound = FALSE;
   for (i = 0; i < 64; i++)
   {
   	 //2012-09-06,���Ҫ������Сʱ��������Ųɼ�Сʱ��������
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
   	 	 	 	   	   printf("������%d���ҵ�������%d\n",reportTask2.task[i].taskNum,tmpPn);
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
     	 printf("δ���������ҵ�������%d���㶳��ʾֵ������,���ɼ��ò��������㶳������\n", pn);
     }
   	 
   	 return 0;
   }
   
   if (debugInfo&METER_DEBUG)
   {
     printf("��ѯ������%d(%02d-%02d-%2d)���㶳������\n", pn, sysTime.year, sysTime.month, sysTime.day);
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
       	   printf("������%d %d�����㶳������ȱʧ\n", pn, tmpHour - i);
       	 }

       	 lostBuff[j+1] = i+1;
       	 j++;
       }
     }
     else
     {
       if (debugInfo&PRINT_CARRIER_DEBUG)
       {
       	 printf("������%d��%d�����㶳������\n", pn, tmpHour - i);
       }

       lostBuff[j+1] = i+1;
       j++;
     }
   }
   
   *lostBuff = j;
   
   return j;
}

/***************************************************
��������:stopCarrierNowWork
��������:ֹͣ�ز�ģ�鵱ǰ����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
BOOL stopCarrierNowWork(void)
{
   if (copyCtrl[4].cpLinkHead==NULL)
   {
   	 return;
   }
   
   //ģ��ѧϰ·����...
   if (carrierFlagSet.studyRouting==2 || carrierFlagSet.studyRouting==3)
   {
   	 printf("��ֹͣ·��ѧϰ\n");
   	  
   	 //ֹͣѧϰ����
     gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
     carrierFlagSet.studyRouting = 3;
     return TRUE;
   }

   //ģ���ѱ���...
   if (carrierFlagSet.searchMeter==2 || carrierFlagSet.searchMeter==3)
   {
   	 printf("��ֹͣ�ѱ�\n");
      
     searchEnd = sysTime;                         //��������ʱ��

   	 //ֹͣѧϰ����
     gdw3762Framing(ROUTE_CTRL_3762, 2, NULL, NULL);
     carrierFlagSet.searchMeter = 3;
     return TRUE;
   }
   
   return FALSE;
}

/***************************************************
��������:foundMeterEvent
��������:���ֵ���¼���¼
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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

   	 eventData[8] = 31;      //�ն�ͨ�Ŷ˿�
   	 
   	 while(foundMeter!=NULL)
   	 {   	 
     	 memcpy(&eventData[10+meterNum*8],foundMeter->addr,6);
     	 eventData[10+meterNum*8+6] = 0xf;    //�ź�Ʒ��
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
     
     eventData[9] = meterNum;       //���ֵ�����
     
     if (eventRecordConfig.iEvent[4] & 0x04)
     {
     	  writeEvent(eventData, meterNum*8+10 , 1, DATA_FROM_GPRS);   //������Ҫ�¼�����
     }
     if (eventRecordConfig.nEvent[4] & 0x04)
     {
     	  writeEvent(eventData, meterNum*8+10, 2, DATA_FROM_LOCAL);  //����һ���¼�����
     }
     
     activeReport3();   //�����ϱ��¼�

   	 eventStatus[4] = eventStatus[4] | 0x04;
   }
}

#endif

/***************************************************
��������:checkLastDayData
��������:������յ����������Ƿ���ȫ,�����ȱʧ���²ɼ�
���ú���:
�����ú���:
�������:pn-������
         type-��ѯ����,0-��ѯ��ǰʱ�����һ��,1-����queryTimeʱ������ѯ
         queryTime-��ѯʱ��
         queryType-��ѯ����,1-�����,0-��ʷ
�������:
����ֵ��void
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
     //����������ն�������Ϊ����0�������Ϊ�����ն�������,���,����Ƿ��е��յ��ն�������
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
     printf("��ѯ������%d(%02x-%02x-%2x)�ն�������\n", pn, tmpTime.year, tmpTime.month, tmpTime.day);
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
     	     printf("������%d��һ�ζ�������Ϊ�����ն�������,Ӧ�ɼ����������\n", pn);
     	   }
     	       	  
     	   return FALSE;
     	 }
     	 
       if (tmpReadBuff[POSITIVE_WORK_OFFSET] == 0xEE || tmpReadBuff[tmpQueryOffset] == 0xEE)
       {
       	  if (debugInfo&METER_DEBUG)
       	  {
       	    printf("������%d��һ�ζ�������ȱʧ\n", pn);
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
       	    printf("������%d��һ�ζ�������ȱʧ\n", pn);
       	  }
  
       	  return FALSE;
       }
     }
     
     if (debugInfo&METER_DEBUG)
     {
     	 printf("������%d����һ�ζ�������\n", pn);
     }
   }
   else
   {
     if (debugInfo&METER_DEBUG)
     {
     	 printf("������%d����һ�ζ�������\n", pn);
     }
     
     return FALSE;
   }
   
   return TRUE;
}

/***************************************************
��������:checkLastMonthData
��������:������µ����������Ƿ���ȫ,�����ȱʧ���²ɼ�
���ú���:
�����ú���:
�������:pn-������
         queryTime-��ѯʱ��
�������:
����ֵ��void
�޸���ʷ:
  1,2012-06-06,�޸�Ϊֻ�������(/��һ������)�����й��ͷ����й��Ƿ���ȫ
    ԭ��Ϊ:�����ֻ�������й��ͷ����й�������
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

   //����û�иò����������
   if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
   {
     return FALSE;
   }
 	 
   //������µ����������Ƿ���ȫ�������ȱʧ���²ɼ�
   if (readMeterData(tmpReadBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &tmpTime, 0) == TRUE)
   {
     if (tmpReadBuff[POSITIVE_WORK_OFFSET] == 0xEE || tmpReadBuff[NEGTIVE_WORK_OFFSET] == 0xEE
  	   //2012-06-01,���ֵ�������������ֻ�������й��ͷ����й�,��˸ĳ�ֻ�ж�������
  	   //|| tmpReadBuff[POSITIVE_NO_WORK_OFFSET] == 0xEE || tmpReadBuff[NEGTIVE_NO_WORK_OFFSET] == 0xEE
  	   //  || tmpReadBuff[QUA1_NO_WORK_OFFSET] == 0xEE || tmpReadBuff[QUA2_NO_WORK_OFFSET] == 0xEE
  	   //    || tmpReadBuff[QUA3_NO_WORK_OFFSET] == 0xEE || tmpReadBuff[QUA4_NO_WORK_OFFSET] == 0xEE
  	    )
     {
       if (debugInfo&METER_DEBUG)
       {
         printf("������%d�������ݵ�������ȱʧ\n", pn);
       }
      
       hasData = FALSE;
     }
   }
   else
   {
     if (debugInfo&METER_DEBUG)
     {
       printf("������%d�������ݵ������ݲ�ѯʧ��\n",meterConfig.measurePoint);
     }
    
     hasData = FALSE;
   }
   
   return hasData;
}

/***************************************************
��������:forwardData
��������:����ת��
���ú���:
�����ú���:
�������:INT8U port,ת���˿��ڳ�����������е����
�������:
����ֵ��void
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
   	 //�ȴ������ظ���վ
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
       case 1:   //͸��ת��
         switch(portNum)
         {
          #ifdef RS485_1_USE_PORT_1
           case 0x0:   //�˿�1
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x01;                                     //xMega�˿�1
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //�˿�ת������
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("���ö˿�1ת������\n");
              }
             #else
              oneUartConfig(fdOfttyS2,copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("�˿�1��ʼ͸��ת��\n");
             }
             
             sendMeterFrame(PORT_NO_1,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);             
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             break;
       	 
           case 0x1:   //�˿�2
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x02;                                     //xMega�˿�2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //�˿�ת������
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("���ö˿�2ת������\n");
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
               printf("�˿�2��ʼ͸��ת��\n");
             }

             sendMeterFrame(PORT_NO_2,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif             
             break;
             
           case 0x2:   //�˿�3
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x03;                                     //xMega�˿�2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //�˿�ת������
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("���ö˿�3ת������\n");
              }
              
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             
             if (debugInfo&METER_DEBUG)
             {
               printf("�˿�3��ʼ͸��ת��\n");
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

           case 0x1:   //�˿�2
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x01;                                     //xMega�˿�1
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //�˿�ת������
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("���ö˿�2ת������\n");
              }
             #else
              oneUartConfig(fdOfttyS2,copyCtrl[portNum].pForwardData->ctrlWord&0xfb);
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("�˿�2��ʼ͸��ת��\n");
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
       	 
           case 0x2:   //�˿�3
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x02;                                     //xMega�˿�2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //�˿�ת������
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("���ö˿�3ת������\n");
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
               printf("�˿�3��ʼ͸��ת��\n");
             }

             sendMeterFrame(PORT_NO_2,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             
             #ifdef WDOG_USE_X_MEGA
  	          if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	          {
  	            xMegaQueue.inTimeFrameSend = TRUE;
  	          }
             #endif
             break;
             
           case 0x3:   //�˿�4
             #ifdef WDOG_USE_X_MEGA
              buf[0] = 0x03;                                     //xMega�˿�2
              buf[1] = copyCtrl[portNum].pForwardData->ctrlWord; //�˿�ת������
              sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
               
              if (debugInfo&METER_DEBUG)
              {
                printf("���ö˿�4ת������\n");
              }
             #endif

             if (debugInfo&METER_DEBUG)
             {
               printf("�˿�4��ʼ͸��ת��\n");
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
           case 0x4:   //�˿�31
           	#ifdef LM_SUPPORT_UT
           	 if (0x55==lmProtocol)
           	 {
               sendMeterFrame(PORT_POWER_CARRIER, copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
             }
             else
             {
   	  	    #endif 
   	  	    
     	  	     //·����������Ļ�,Ҫ�鿴�Ƿ��ڳ���������״̬
     	  	     if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
     	  	     {
     	  	     	 memset(copyCtrl[4].needCopyMeter, 0x00, 6);
  
     	  	     	 if (carrierFlagSet.workStatus<4)
     	  	     	 {
     	  	     	   carrierFlagSet.workStatus = 4;
     	  	     	   carrierFlagSet.reStartPause = 2;
  
         	  	     if (debugInfo&PRINT_CARRIER_DEBUG)
         	  	     {
         	  	       printf("%02d-%02d-%02d %02d:%02d:%02d,·���������� - ������,����ת��ǰ������ͣ����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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
               
               //for(j=1;j<=512;j++) ly,2012-03-12,�ĳ����2040
               /*2015-10-19,ȥ�������Ƿ�Ϊ���ò����ı��ַ
               for(j=1;j<=2040;j++)
               {
                 if (selectF10Data(j, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
                 {
              	   //δ���øò��������Ϣ
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
                     //����ɼ�����ַΪȫ0,���ز�Ŀ�Ľڵ�Ϊ���ַ,����Ŀ�Ľڵ�Ϊ�ɼ�����ַ
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
               
               //2015-10-19,ֱ�Ӹ�ֵ�ز�Ŀ���ַ
               memcpy(carrierFlagSet.destAddr, &copyCtrl[4].pForwardData->data[1], 6);
               copyCtrl[4].ifCollector = 0;
  
             	 if (debugInfo&METER_DEBUG)
             	 {
                 printf("�˿�31��ʼ͸��ת��\n");
             	 }
               sendCarrierFrame(PORT_POWER_CARRIER,copyCtrl[portNum].pForwardData->data, copyCtrl[portNum].pForwardData->length);
            
            #ifdef LM_SUPPORT_UT
             }
            #endif
            
           	 break;
          #endif
           	 
           default:   //�����˿�
           	 return FALSE;
         }

        #ifdef PLUG_IN_CARRIER_MODULE
       	 //2013-12-25,��������Ҫ�ļ����������Ҫ�ȴ�100��,�ſɻָ�
       	 if (carrierModuleType==TC_CARRIER)
       	 {
       	   if (copyCtrl[portNum].pForwardData->frameTimeOut<100)
       	   {
       	  	 copyCtrl[portNum].pForwardData->frameTimeOut = 100;
       	  	 
       	  	 if (debugInfo&PRINT_CARRIER_DEBUG)
       	  	 {
       	  	   printf("%02d-%02d-%02d %02d:%02d:%02d,�����ز�ģ���޸�ת����ʱʱ��Ϊ100��\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
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

         printf("ת����ʱʱ��%d\n", copyCtrl[portNum].pForwardData->frameTimeOut);

         copyCtrl[portNum].pForwardData->ifSend  = TRUE;
         
         return TRUE;
         break;
         
       case 9:    //ת����վֱ�ӶԵ��ܱ�ĳ�����������
     	   //֡��ʼ�ַ�
     	   sendBuf[0] = 0x68;
          
         //��д��ַ��
         memcpy(&sendBuf[1],copyCtrl[portNum].pForwardData->data,6);         
         
         //֡��ʼ�ַ�
         sendBuf[7] = 0x68;
         
         //��д������
         frameTail = 9;
         if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
         {
            sendBuf[8] = 0x01;              //�����ֶ�
            sendBuf[frameTail++] = 0x2;     //����
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[7] + 0x33;  //DI0
            sendBuf[frameTail++] = copyCtrl[portNum].pForwardData->data[8] + 0x33;  //DI1
         }
         else    //DLT_645_2007
         {
            sendBuf[8] = 0x11;              //�����ֶ�
            sendBuf[frameTail++] = 0x4;     //����
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
            case 0x0:   //�˿�1
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega�˿�1
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�1ת������Ϊ1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega�˿�1
                 buf[1] = 0x6b;                                     //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�1ת������Ϊ2400\n");
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
                printf("�˿�1��ʼת����վֱ�ӳ���\n");
              }
              break;
             
            case 0x1:   //�˿�2
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega�˿�2
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�2ת������Ϊ1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega�˿�2
                 buf[1] = 0x6b;                                     //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�2ת������Ϊ2400\n");
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
                printf("�˿�2��ʼת����վֱ�ӳ���\n");
              }
              break;
              
            case 0x2:   //�˿�3
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega�˿�3
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�3ת������Ϊ1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega�˿�2
                 buf[1] = 0x6b;                                     //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�3ת������Ϊ2400\n");
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
                printf("�˿�3��ʼת����վֱ�ӳ���\n");
              }
              break;
              
           #else
           
            case 0x1:   //�˿�2
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega�˿�1
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�2ת������Ϊ1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x01;                                     //xMega�˿�1
                 buf[1] = 0x6b;                                     //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�2ת������Ϊ2400\n");
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
                printf("�˿�2��ʼת����վֱ�ӳ���\n");
              }
              break;
             
            case 0x2:   //�˿�3
             #ifndef SUPPORT_CASCADE 
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega�˿�2
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�3ת������Ϊ1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS3, DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x02;                                     //xMega�˿�2
                 buf[1] = 0x6b;                                     //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�3ת������Ϊ1200\n");
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
                printf("�˿�3��ʼת����վֱ�ӳ���\n");
              }
              break;
              
            case 0x3:   //�˿�4
              if ((copyCtrl[portNum].pForwardData->data[6]&0x03)==0x0)  //DLT_645_1997
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega�˿�3
                 buf[1] = DEFAULT_SER_CTRL_WORD;                    //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�3ת������Ϊ1200\n");
                 }
                #else
                 oneUartConfig(fdOfttyS2,DEFAULT_SER_CTRL_WORD);
                #endif
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 buf[0] = 0x03;                                     //xMega�˿�3
                 buf[1] = 0x6b;                                     //�˿�ת������
                 sendXmegaFrame(COPY_PORT_RATE_SET, buf, 2);
                  
                 if (debugInfo&METER_DEBUG)
                 {
                   printf("���ö˿�3ת������Ϊ2400\n");
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
                printf("�˿�3��ʼת����վֱ�ӳ���\n");
              }
              break;
              
           #endif
             
          #ifdef PLUG_IN_CARRIER_MODULE 
           case 0x4:   //�˿�31
   	  	     //·����������Ļ�,Ҫ�鿴�Ƿ��ڳ���������״̬
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
            	   //δ���øò��������Ϣ
            	   continue;
               }
               else
               {
                 if (meterConfig.addr[0]==meterAddr[0] && meterConfig.addr[1]==meterAddr[1]
               	     && meterConfig.addr[2]==meterAddr[2] && meterConfig.addr[3]==meterAddr[3]
               	      && meterConfig.addr[4]==meterAddr[4] && meterConfig.addr[5]==meterAddr[5]
               	    )
                 {
                   //����ɼ�����ַΪȫ0,���ز�Ŀ�Ľڵ�Ϊ���ַ,����Ŀ�Ľڵ�Ϊ�ɼ�����ַ
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
               printf("�˿�31��ʼת������\n");
             }
           	 break;
          #endif
           	 
           default:   //�����˿�
           	 return FALSE;
         }
         return TRUE;
       	 break;
       	 
       default:
         return FALSE;
     }
   }
   else     //ת�������ѷ��ͺ��ж��Ƿ�ʱ
   {
   	 //�жϳ�ʱ
   	 if (compareTwoTime(copyCtrl[portNum].pForwardData->outTime, sysTime))
   	 {
   	 	 if (debugInfo&METER_DEBUG)
   	 	 {
   	     printf("%02d-%02d-%02d %02d:%02d:%02d,ת����ʱ\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second);
   	 	 }

   	 	 copyCtrl[portNum].pForwardData->forwardResult = RESULT_NO_REPLY;
   	 	 forwardDataReply(portNum);  //Ӧ�ûظ�����
   	 	 	 
   	 	 return FALSE;
   	 }
   }
   
   return FALSE;
}

//***********************************Q/GDW376.1���ҵ����ն�******end******************************




#ifdef SUPPORT_ETH_COPY

#define TOTAL_CMD_CURRENT_645_1997            79    //DL/T645-1997��ǰ������������

#define TOTAL_CMD_CURRENT_645_2007           100    //DL/T645-2007��ǰ������������

//DL/T645-1997����+����+�α���+ʱ�βα������������ݱ�ʶת����
INT8U  cmdDlt645Current1997[TOTAL_CMD_CURRENT_645_1997][5] = {
	                        //���������� 26��
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
	                        
	                        //�α�������
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
	                        //���¼���ʱ�βα���
	                        {0xC3, 0x10, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF,0x1},
	                        {0xC3, 0x11, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF,0x1},
	                        {0xC3, 0x12, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF,0x1},
	                        {0xC3, 0x13, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF,0x1},
	                        {0xC3, 0x14, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF,0x1},
	                        {0xC3, 0x2f, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF,0x1},
	                        
	                        //��������ʱ�α���ʼʱ�估���ʺ�
	                        {0xC3, 0x3f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x4f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x5f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x6f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x7f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x8f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x9f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0xaf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1}
	                        
	                        //�����Ǽ���ʱ��
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
   //2.2.1 ���� 22��(ȫ��4bytes,��97һ��)
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�����
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�����

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�����ʾֵ��,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�����ʾֵ��,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //����޹�1����
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //����޹�2����
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1�����޹�����
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4�����޹�����
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2�����޹�����
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3�����޹�����
   {0x00, 0x85, 0x00, 0x00, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //ͭ���й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x86, 0x00, 0x00, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //�����й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x15, 0x00, 0x00, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A�������й�����(4Bytes)
   {0x00, 0x16, 0x00, 0x00, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A�෴���й�����(4Bytes)
   {0x00, 0x17, 0x00, 0x00, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�1����(4Bytes)
   {0x00, 0x18, 0x00, 0x00, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�2����(4Bytes)
   {0x00, 0x29, 0x00, 0x00, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B�������й�����(4Bytes)
   {0x00, 0x2A, 0x00, 0x00, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B�෴���й�����(4Bytes)
   {0x00, 0x2B, 0x00, 0x00, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�1����(4Bytes)
   {0x00, 0x2C, 0x00, 0x00, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�2����(4Bytes)
   {0x00, 0x3D, 0x00, 0x00, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C�������й�����(4Bytes)
   {0x00, 0x3E, 0x00, 0x00, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C�෴���й�����(4Bytes)
   {0x00, 0x3F, 0x00, 0x00, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�1����(4Bytes)
   {0x00, 0x40, 0x00, 0x00, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�2����(4Bytes)
   
   //2.2.2 ��������������ʱ�� 4��(����3bytes+5bytes������ʱ��,97��3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //�����й��������������ʱ��
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //�����й��������������ʱ��
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//����޹�1�������������ʱ��
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //����޹�2�������������ʱ��
   
   //2.2.3 �������� 8��
   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //��ѹ���ݿ�(A,B,C���ѹ)(��2�ֽ�=97,����ʽ97��xxx��07��xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //�������ݿ�(A,B,C�����)(��3�ֽ�<>97,97�Ǹ�2�ֽ�xx.xx,07��xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //˲ʱ�й����ʿ�(��,A,B,C���й�����)(��3�ֽ�=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //˲ʱ�޹����ʿ�(��,A,B,C���޹�����)(��3�ֽ�xx.xxxx<>97��xx.xx)
   {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},          //˲ʱ���ڹ��ʿ�(��,A,B,C�����ڹ���)(��3�ֽ�xx.xxxx<>97û��)
   {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                  //����������(��,A,B,C�๦������)(��2�ֽ�=97,x.xxx)   
   {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},                        //������ݿ�(A,B,C�����)(��2�ֽ�,97��,x.xxx)
   {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},                //���ߵ���(3�ֽ�xxx.xxx<>97��2�ֽ�xx.xx)
   {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},                    //��ع���ʱ��(4�ֽ�NNNNNNNN<>97��3�ֽ�NNNNNN)

   //2.2.4.�¼���¼���� 26��
   //2.2.4-1.����ͳ������
   //2.2.4-1.1��Щ07��֧�ֹ�Լ,������4���
   //�ܶ������,���ۼ�ʱ��δ�ҵ�,�ѵ�Ҫ����(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x03, 0x04, 0x00, 0x00, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)
                                                                                                   //B������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)
                                                                                                   //C������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)

   //���һ�ζ��෢����ʼʱ�̼�����ʱ��δ�ҵ�,�ѵ�Ҫ����?(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x03, 0x04, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //��һ��A������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x04, 0x02, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //��һ��B������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x04, 0x03, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //��һ��C������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)

   //2.2.4-1.2 ��Щ��֧�ֱ����ļ�,����������12����������ͳ������
   //�ܶ������,���ۼ�ʱ��δ�ҵ�,�ѵ�Ҫ����(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x13, 0x01, 0x00, 0x01, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x02, 0x00, 0x01, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF},                  //B������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x03, 0x00, 0x01, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF},                  //B������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x01, 0x00, 0x02, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF},        //A���ۼ�ʱ��(�ۼ�3�ֽ�=97)
   {0x13, 0x02, 0x00, 0x02, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF},        //B���ۼ�ʱ��(�ۼ�3�ֽ�=97)
   {0x13, 0x03, 0x00, 0x02, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF},        //C���ۼ�ʱ��(�ۼ�3�ֽ�=97)

   //2.2.4-1.3 ���һ�ζ��෢����ʼʱ�̼�����ʱ��δ�ҵ�,�ѵ�Ҫ����?(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   
   {0x13, 0x01, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //��һ��A������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x02, 0x01, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //��һ��B������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x03, 0x01, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //��һ��C������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x01, 0x25, 0x01, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF},            //��һ��A������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x02, 0x25, 0x01, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF},            //��һ��B������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x03, 0x25, 0x01, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF},            //��һ��C������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   
   //2.2.4-2 ����ͳ����
   {0x03, 0x30, 0x00, 0x00, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF},                            //����ܴ���(3�ֽ�!=97,2�ֽ�)
   {0x03, 0x30, 0x00, 0x01, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF},                    //���һ�α��ʱ��(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x30, 0x01, 0x00, METER_CLEAR_TIMES&0xFF, METER_CLEAR_TIMES>>8&0xFF},                    //��������ܴ���(3�ֽ�!=97û��)
   {0x03, 0x30, 0x01, 0x01, LAST_METER_CLEAR_TIME&0xFF, LAST_METER_CLEAR_TIME>>8&0xFF},            //���һ�ε������ʱ��(ʱ��6�ֽ�<>97û��)
   {0x03, 0x30, 0x02, 0x00, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF},                        //������������ܴ���(3�ֽ�!=97,2�ֽ�)
   {0x03, 0x30, 0x02, 0x01, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF},              //���һ�������������ʱ��(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x30, 0x03, 0x00, EVENT_CLEAR_TIMES&0xFF, EVENT_CLEAR_TIMES>>8&0xFF},                    //�¼������ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x03, 0x01, EVENT_CLEAR_LAST_TIME&0xFF, EVENT_CLEAR_LAST_TIME>>8&0xFF},            //�¼�����ʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x04, 0x00, TIMING_TIMES&0xFF, TIMING_TIMES>>8&0xFF},                              //Уʱ�ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x04, 0x01, TIMING_LAST_TIME&0xFF, TIMING_LAST_TIME>>8&0xFF},                      //���һ��Уʱʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x05, 0x00, PERIOD_TIMES&0xFF, PERIOD_TIMES>>8&0xFF},                              //ʱ�α����ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x05, 0x01, PERIOD_LAST_TIME&0xFF, PERIOD_LAST_TIME>>8&0xFF},                      //���һ��ʱ�α��̷���ʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x0d, 0x00, OPEN_METER_COVER_TIMES&0xFF, OPEN_METER_COVER_TIMES>>8&0xFF},          //������ܴ���(2�ֽ�,97û��)
   {0x03, 0x30, 0x0d, 0x01, LAST_OPEN_METER_COVER_TIME&0xFF, LAST_OPEN_METER_COVER_TIME>>8&0xFF},  //��һ�ο���Ƿ���ʱ��(6�ֽ�,97û��)
   
   //2.2.5 �α��� 9��
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                            //���ڼ��ܴ�(4�ֽ�=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},                                  //���ʱ��(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x09, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF},                            //�����(�й�)(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x0a, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF},                      //�����(�޹�)(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x02, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF},                              //����(6�ֽ�=97)
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},                    //�������״̬��1��7(7*2<>97ֻ��2bytes)
   {0x04, 0x00, 0x0b, 0x01, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF},                            //ÿ�µ�1������
   {0x04, 0x00, 0x0b, 0x02, AUTO_COPY_DAY_2&0xFF, AUTO_COPY_DAY_2>>8&0xFF},                        //ÿ�µ�2������
   {0x04, 0x00, 0x0b, 0x03, AUTO_COPY_DAY_3&0xFF, AUTO_COPY_DAY_3>>8&0xFF},                        //ÿ�µ�3������
   
   //2.2.6 ���¼���ʱ�βα��� 6��
   {0x04, 0x00, 0x02, 0x01, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF},                              //��ʱ����(1�ֽ�)
   {0x04, 0x00, 0x02, 0x02, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF},                  //��ʱ�α���(1�ֽ�)
   {0x04, 0x00, 0x02, 0x03, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF},                            //��ʱ����(1�ֽ�)
   {0x04, 0x00, 0x02, 0x04, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF},                              //������(1�ֽ�)
   {0x04, 0x00, 0x02, 0x05, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF},                        //����������(1�ֽ�)   
   {0x04, 0x01, 0x00, 0x00, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF},                                  //��һ��ʱ��������(14*3)
   
   //2.2.7 �����ǵ�һ����ʱ�α���ʼʱ�估���ʺ� 8��
   {0x04, 0x01, 0x00, 0x01, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��1��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x02, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��2��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x03, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��3��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x04, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��4��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x05, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��5��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x06, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��6��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x07, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��7��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x08, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��8��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   
   //2.2.8 �ѿر����/����/������Ϣ
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME&0xFF, LAST_JUMPED_GATE_TIME>>8&0xFF},            //��һ����բ����ʱ��(6�ֽ�,97û��)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME&0xFF, LAST_CLOSED_GATE_TIME>>8&0xFF},            //��һ�κ�բ����ʱ��(6�ֽ�,97û��)
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME&0xFF, CHARGE_TOTAL_TIME>>8&0xFF},                    //��1�ι�����ܹ������(2�ֽ�)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY&0xFF, CHARGE_REMAIN_MONEY>>8&0xFF},                //��ǰʣ����(4�ֽ�,97û��)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY&0xFF, CHARGE_TOTAL_MONEY>>8&0xFF},                  //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY&0xFF, CHARGE_REMAIN_QUANTITY>>8&0xFF},          //��ǰʣ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY&0xFF, CHARGE_OVERDRAFT_QUANTITY>>8&0xFF},    //��ǰ͸֧����(4�ֽ�,97û��)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY&0xFF, CHARGE_TOTAL_QUANTITY>>8&0xFF},            //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT&0xFF, CHARGE_OVERDRAFT_LIMIT>>8&0xFF},          //͸֧������ֵ(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY&0xFF, CHARGE_ALARM_QUANTITY>>8&0xFF},            //��������1��ֵ
   };


int               fdOfClient=NULL;    //��CAN2ETHͨ�ż�����socket

struct cpAddrLink *pEthCpLinkHead;    //��̫�������������ָ��

INT8U             canloginIn=0;       //CAN2ETH�Ƿ��¼�ɹ�
DATE_TIME         canTimeOut;         //CAN2ETH������ʱ
INT8U             flagOfFrame=0;      //����֡��־

INT8U             setEthMeter=0;      //��վ������̫�������ַ
DATE_TIME         msSetWaitTime;      //�ȴ����ñ��ַ��
INT16U            monitorMp=0;        //��Ҫ����ԭʼ���ݵĲ��Ե��(������̫�������̫��,�����Ҫһ��һ����ʱ��)

/**************************************************
��������:sendToCan2Eth
��������:�������ݵ�CAN2ETH
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��void
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
  // afn - 0 ȷ��/����
  //     - 1 ��¼/����
  //     - 2 ��������
  // cs  - ָlen,afn,data��������
  
  //��������λ  
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
��������:dataOffset
��������:���ݵ��������DI��ȷ�������������
���ú���:
�����ú���:
�������:di
�������:
����ֵ���������
***************************************************/
INT16U dataOffset(INT8U protocol, INT8U *di)
{
  INT16U    ret=0x200;
  INT8U     i;
  char      tmpChrDi[100];
    
  switch (protocol)
  {         	
    case DLT_645_1997:
      if ((*(di+1)==0x94) || *(di+1)==0x95 || *(di+1)==0xa4 || *(di+1)==0xa5 || *(di+1)==0xb4 || *(di+1)==0xb5)  //��������
     	{
//    	  //������������������ʱ��
//    	  for (i = 0; i < TOTAL_CMD_LASTMONTH_645_1997; i++)
//    	  {
//    	    if (cmdDlt645LastMonth1997[i][0] == *(di+1) && (cmdDlt645LastMonth1997[i][1] == *di))
//    	    {
//    	      ret = (cmdDlt645LastMonth1997[i][2]) | ((cmdDlt645LastMonth1997[i][3])<<8);
//    	      break;
//    	    }
//    	  }
    	}
    	else      		//��ǰ����
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
      ret = 0x200;   //�����
      break;
  }
  
  return ret;
}

/***************************************************
��������:analyse645Dlt1997
��������:97��Լ���ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S analyse645Dlt1997(struct cpAddrLink *pCpLink)
{
  INT16U offset;
  INT8U  *pData;
  INT16U loadLen;

  //���ݴ洢ƫ����
  offset = findDataOffset(pCpLink->protocol, &pCpLink->recvBuf[10]);
      
  //68 33 47 00 11 12 92 68 81 04 46 e9 33 33 19 16
  if (debugInfox&ETH_COPY_METER)
  {
    printf(",1997��Լ,��ʶ%02X %02X,ƫ��:%02X\n",
          pCpLink->recvBuf[11], pCpLink->recvBuf[10], offset);
  }
  
  if (0x200==offset)
  {
    return -1;    //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
  }

  pData = &pCpLink->recvBuf[10];      //��������ʼָ��
  loadLen = pCpLink->recvBuf[9]-2;    //�����ݳ���

  switch(pCpLink->recvBuf[11])
  {
 	  case 0xB2:     //����
 	  case 0xB3:
 	  case 0xB6:
 	  case 0xC0:
 	  case 0xC1:
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //�в�������
      
      switch(loadLen)
      {
      	case  1:
        	*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = *(pData+2);
          break;
  
        case 2:
         	switch(offset)
         	{
         	 	case VOLTAGE_PHASE_A:  //A,B,C���ѹ��2�ֽ�xxx,�洢Ϊxxx.0
         	 	case VOLTAGE_PHASE_B:
         	 	case VOLTAGE_PHASE_C:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = (*(pData+2))<<4;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = ((*(pData+3))<<4) | ((*(pData+2))>>4);
        	    break;
        	     
       	    case CURRENT_PHASE_A:  //A,B,C�������2�ֽ�xx.xx�洢Ϊ0xx.xx0
       	    case CURRENT_PHASE_B:
       	    case CURRENT_PHASE_C:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = (*(pData+2))<<4;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = ((*(pData+3))<<4) | ((*(pData+2))>>4);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = (*(pData+3))>>4;
       	   	  break;
       	   	 
       	    case POWER_INSTANT_NO_WORK: //�޹�����Ϊ2bytesxx.xx,�洢Ϊxx.xx00
       	    case POWER_PHASE_A_NO_WORK:
       	    case POWER_PHASE_B_NO_WORK:
       	    case POWER_PHASE_C_NO_WORK:
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset)   = 0;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+2);
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+2) = *(pData+3);      	   	
       	   	  break;
 
       	    case PHASE_DOWN_TIMES:      //�������Ϊ2bytesNNNN,�洢Ϊ00NNNN
       	    case PHASE_A_DOWN_TIMES:
       	    case PHASE_B_DOWN_TIMES:
       	    case PHASE_C_DOWN_TIMES:
       	    case PROGRAM_TIMES:         //��̴���
       	    case UPDATA_REQ_TIME:       //�����������
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
           	case BATTERY_WORK_TIME:  //��ع���ʱ��3bytes(NNNNNN),�洢Ϊ4bytes(00NNNNNN)
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
         	 	case LAST_PROGRAM_TIME:     //���һ�α��ʱ��
         	 	case LAST_UPDATA_REQ_TIME:  //���һ����������ʱ��
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
 	  
 	  case 0xC3:    //ʱ��
 	  case 0xC4:
      pCpLink->copyedData |= HAS_SHIDUAN;     //��ʱ������

      if (*(pData+1)==0xc3)
      {
        if ((*pData>>4)==1)  //��ʱ����P��
      	{
      		*(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset) = *(pData+2);
      	}
      	else                     //ʱ������ʱ��
      	{
          if ((*pData & 0xF) != 0xF)  //�������
          {
            offset += 3*((*pData&0xf)-1);
             	
            *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset) = *(pData + 2);
            *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+1) = *(pData + 3);
            *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset+2) = *(pData + 4);
          }
          else   //���ݿ鴦�� ���ݱ�ʶxxxF
          {
            if ((*pData>>4&0xf)>2)
            {
             	offset += 30*((*pData>>4&0xf)-3);
            }
             
            pData += 2;     //ָ��������
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
                	  	  	
    default:    //����������
      //������������������ʱ������
      if ((*pData & 0xF) != 0xF)        //�������
      {
        if ((*(pData+1)&0xf0) == 0x90)  //������
        {
          if ((*(pData+1) & 0xf) == 0x0a)
          {
           	pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
          }
          else
          {
            if ((*(pData+1) & 0xc)>>2 == 0x00)
            {
           	   pCpLink->copyedData |= HAS_CURRENT_ENERGY;    //�е�ǰ��������
            }
            else
            {
           	   pCpLink->copyedData |= HAS_LAST_MONTH_ENERGY; //�����µ�������
           	}
          }
           
        	*(pCpLink->dataBuff+offset)   = *(pData + 2);
        	*(pCpLink->dataBuff+offset+1) = *(pData + 3);
        	*(pCpLink->dataBuff+offset+2) = *(pData + 4);
        	*(pCpLink->dataBuff+offset+3) = *(pData + 5);
        }
        else                      //����
        {
          if ((*(pData+1) & 0xc)>>2 == 0x00)
          {
           	 pCpLink->copyedData |= HAS_CURRENT_REQ;        //�е�ǰ��������
          }
          else
          {
           	 pCpLink->copyedData |= HAS_LAST_MONTH_REQ;     //��������������
          }
           
          if ((*(pData+1)&0xf0) == 0xA0)  //����
          {
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData + 2);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 3);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 4);
          }
          else    //����ʱ��
          {
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData + 2);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 3);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 4);
        	   *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+3) = *(pData + 5);
          }
        }
      }
      else    //���ݿ鴦�� ���ݱ�ʶxxxF
      {
        //68 30 00 01 00 00 00 68 81 17 1F 94 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AA 8B 16  
        if ((*(pData+1)&0xf0) == 0x90)           //������
        {
          if ((*(pData+1) & 0xf) == 0x0a)
          {
            pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
          }
          else
          {
            if ((*(pData+1) & 0xc)>>2 == 0x00)
            {
           	  pCpLink->copyedData |= HAS_CURRENT_ENERGY;    //�е�ǰ��������
            }
            else
            {
            	pCpLink->copyedData |= HAS_LAST_MONTH_ENERGY; //�����µ�������
            }
          }
           
          pData += 2;     //ָ��������
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
        else                           //����������ʱ��
        {
          if ((*(pData+1) & 0xc)>>2 == 0x00)
          {
           	pCpLink->copyedData |= HAS_CURRENT_REQ;        //�е�ǰ��������
          }
          else
          {
            pCpLink->copyedData |= HAS_LAST_MONTH_REQ;     //�����µ�������
          }
           
          if ((*(pData+1)&0xf0) == 0xA0)  //����
          {
            pData += 2;     //ָ��������
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
          else                  //��������ʱ��
          {
            pData += 2;     //ָ��������
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
��������:analyse645Dlt2007
��������:07��Լ���ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S analyse645Dlt2007(struct cpAddrLink *pCpLink)
{
  INT16U offset, offsetx;
  INT8U  i;
  INT8U  *pData;
  INT16U loadLen;
  
  //���ݴ洢ƫ����
  offset = dataOffset(pCpLink->protocol, &pCpLink->recvBuf[10]);
  
  if (debugInfox&ETH_COPY_METER)
  {
    printf(",2007��Լ,��ʶ%02X %02X %02X %02X,ƫ��:%02X\n",
          pCpLink->recvBuf[13], pCpLink->recvBuf[12],pCpLink->recvBuf[11], pCpLink->recvBuf[10], offset);
  }
    
  if (offset==0x200)
  {
    return -1;    //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
  }
  
  pData = &pCpLink->recvBuf[14];   //��������ʼָ��
  loadLen = pCpLink->recvBuf[9]-4;//�����ݳ���
  switch(pCpLink->recvBuf[13])    //DI3
  {
 	  case ENERGY_2007:   //������
 	 	  if (pCpLink->recvBuf[10]==0x0)    //DI0
 	 	  {
      	pCpLink->copyedData |= HAS_CURRENT_ENERGY;     //�е�ǰ��������
      }
      else
      {
      	pCpLink->copyedData |= HAS_LAST_MONTH_ENERGY;  //����һ�����յ�������
      }
      
 	 	  if (pCpLink->recvBuf[11]==0xff) //���ݿ�
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
 	 	  else             //������
 	 	  {
      	 if (pCpLink->recvBuf[12]==0x90)    //�����ǲα���
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
 	 	 
 	  case REQ_AND_REQ_TIME_2007:  //����������ʱ��
 	 	  if (pCpLink->recvBuf[10]==0x0)
 	 	  {
      	pCpLink->copyedData |= HAS_CURRENT_REQ;        //�е�ǰ��������
      }
      else
      {
      	pCpLink->copyedData |= HAS_LAST_MONTH_REQ;     //��������������
      }

 	 	  if (pCpLink->recvBuf[11]==0xff) //���ݿ�
 	 	  {
        offsetx = offset+27;
        while (loadLen>=8)
        {
      	  //����3bytes
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData);
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 1);
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 2);
      	    
      	  //��������ʱ��5bytes
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+0) = *(pData + 3);  //��
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+1) = *(pData + 4);  //ʱ
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+2) = *(pData + 5);  //��
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+3) = *(pData + 6);  //��
      	  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+4) = *(pData + 7);  //��

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
 	 	  else             //������
 	 	  {
 	 	  }    	 	 
 	 	  break;
 	 
 	  case VARIABLE_2007:   //����
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //�вα�������
 	 	 
 	 	  if (pCpLink->recvBuf[11]==0xff)
 	 	  {
        switch(pCpLink->recvBuf[12])
        {
        	case 0x01:  //��ѹ���ݿ�(��2���ֽ�xxx.x)
        	  for(i=0;i<6 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x02:  //�������ݿ�(��3���ֽ�xxx.xxx)
        	  for(i=0;i<9 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x03:  //˲ʱ�й��������ݿ�(��3���ֽ�xx.xxxx)
        	case 0x04:  //˲ʱ�޹��������ݿ�(��3���ֽ�xx.xxxx)
        	case 0x05:  //˲ʱ���ڹ������ݿ�(��3���ֽ�xx.xxxx)
        	  for(i=0;i<12 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x06:  //�����������ݿ�(��2�ֽ�x.xxx)
        	  for(i=0; i<8 && i<loadLen; i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
        	  break;

        	case 0x07:  //������ݿ�(��2�ֽ�x.xxx)
        	  for(i=0;i<6 && i<loadLen; i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);   //��ѹ���
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+6+i)   = *(pData+i);   //�������
        	  }
        	  break;
        	  	
        	default:
 	 	        return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����           	  	
        	  break;
        }
 	 	  }
 	 	  else
 	 	  {
        if (pCpLink->recvBuf[12]==0x80)
        {
          if (pCpLink->recvBuf[10]==0x01)  //���ߵ���(xxx.xxx!=97 2bytes xx.xx)
          {
        	  for(i=0;i<3 && i<loadLen;i++)
        	  {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
        	  }
          }
           
        	if (pCpLink->recvBuf[10]==0x0a)  //�ڲ���ع���ʱ��(4bytes)
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
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //�вα�������
      
 	 	  switch(pCpLink->recvBuf[12])
 	 	  {
 	 	 	  case 0x04:    //A,B,C�����¼,��������Ͷ���ʱ��,etc...
     	 	  switch(pCpLink->recvBuf[10])
     	 	  {
     	 	 	  case 0x00:  //A,B,C������������ۼ�ʱ��(������ʱ���3bytes,NNNNNN,3*2*3)
     	 	 	    for(i=0;i<18;i++)
     	 	 	    {
                *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i)   = *(pData+i);
     	 	 	    }
     	 	 	    break;
     	 	 	    
     	 	 	  case 0x01:  //A,B,C������¼(��ʼʱ��ͽ���ʱ���6bytes,ssmmhhDDMMYY)
            	for(i=0;i<12;i++)
            	{
                *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	}
            	break;
            	  
            default:
 	 	          return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����
     	 	  }
     	 	  break;
     	 	 
     	  case 0x30:   //���,�������,��������,�¼�����,Уʱ,ʱ�α��޸ļ�¼
     	 	  switch(pCpLink->recvBuf[11])
     	 	  {
     	 	 	  case 0x0:  //��̼�¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //��̴���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ�α�̼�¼
            	    //��������һ�α�̷���ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x1:  //��������¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //��������ܴ���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ�ε�������¼
            	    //��������һ�ε�����㷢��ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x2:  //���������¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //���������ܴ���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ�����������¼
            	    //��������һ���������㷢��ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x3:  //�¼������¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //�¼������ܴ���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ���¼������¼
            	    //��������һ���¼����㷢��ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;

     	 	 	  case 0x4:  //Уʱ��¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //Уʱ�ܴ���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ��Уʱ��¼
            	    //��������һ��Уʱ����ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i+4);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0x5:  //ʱ�α�̼�¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //ʱ�α���ܴ���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ��ʱ�α�̼�¼
            	    //��������һ��ʱ�α�̷���ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;
     	 	 	   
     	 	 	  case 0xd:  //���β�Ǵ򿪼�¼
     	 	 	    switch(pCpLink->recvBuf[10])
     	 	 	    {
     	 	 	      case 0x00:  //������ܴ���
            	    for(i=0;i<3;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  case 0x01:  //��һ�ο���Ƿ���ʱ��
            	    //��������һ�ο���Ƿ���ʱ��6bytes(ssmmhhDDMMYY) 
            	    for(i=0;i<6;i++)
            	    {
                    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	    }
            	    break;
            	      
            	  default:
 	 	              return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
     	 	 	    }
     	 	 	    break;
     	 	  }
     	 	  break;
     	 	 
     	  case 0x32:
     	 	  //��һ�ι�����ۼƹ�����
     	 	  if (pCpLink->recvBuf[11]==0x06 && pCpLink->recvBuf[10]==0x01)
     	 	  {
            for(i=0;i<4;i++)
            {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            }
     	 	  }
     	 	  break;
     	 	 
     	  case 0x33:
     	 	  //��һ�ι�����ۼƹ������
     	 	  if (pCpLink->recvBuf[11]==0x02 && pCpLink->recvBuf[10]==0x01)
     	 	  {
            for(i=0;i<2;i++)
            {
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            }
     	 	  }
     	 	 
     	 	  //��һ�ι�����ۼƹ�����
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
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //�вα�������

 	 	  switch(pCpLink->recvBuf[12])
 	 	  {
 	 	 	  case 0x0:
     	    switch(pCpLink->recvBuf[11])
     	    {
     	   	  case 0x01:
     	        if (pCpLink->recvBuf[10]==0x01)  //���ڼ�����
     	 	      {
            	  for(i=0;i<4;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      if (pCpLink->recvBuf[10]==0x02)  //ʱ��
     	 	      {
            	  for(i=0;i<3;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      break;
     	 	     
     	 	    case 0x02:
     	 	   	  //���˹��������������������ֽ�ֻ����һ���ֽ���,�����Ķ�����Լһ���ֽڴ洢
              pCpLink->copyedData |= HAS_SHIDUAN;       //��ʱ������
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD+offset) = *pData;
     	 	   	  break;
     	 	   
     	 	    case 0x04:        	 	 
     	 	      if (pCpLink->recvBuf[10]==0x09 || pCpLink->recvBuf[10]==0x0a)  //�����(�й����޹�)
     	 	      {
            	  for(i=0;i<3;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      if (pCpLink->recvBuf[10]==0x02)  //�����
     	 	      {
            	  for(i=0;i<6;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      break;
     	 	     
     	 	    case 0x05:        	 	 
     	 	      if (pCpLink->recvBuf[10]==0xff)    //�������״̬��
     	 	      {
                //������7��״̬��(7*2bytes),��645-1997ֻ��һ���ֽڵ�״̬��
            	  for(i=0;i<14;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	      break;
     	 	     
     	 	    case 0x0b:  //ÿ�½�����(1,2,3������,��2���ֽ�)
              //������3��������,��645-1997ֻ��һ��������
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset) = *pData;
              *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+1) = *(pData+1);
     	 	      break;
     	 	     
     	 	    case 0x0f:
     	 	      if (pCpLink->recvBuf[10]==0x04 || pCpLink->recvBuf[10]==0x01)    //͸֧������ֵ /��������1��ֵ
     	 	      {
            	  for(i=0;i<4;i++)
            	  {
                  *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
            	  }
     	 	      }
     	 	   	  break;
     	 	  }
     	 	  break;
     	 
     	  case 0x01:   //��һ��ʱ�������ݼ�ʱ�α�����
          pCpLink->copyedData |= HAS_SHIDUAN;       //��ʱ������
     	 	 
     	 	  if (pCpLink->recvBuf[10]==0x00)  //��һ��ʱ��������
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
     	 	  else   //��ʱ�α�����
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
 	 	 
 	  case FREEZE_DATA_2007:   //��������
 	 	  if (pCpLink->protocol==SINGLE_PHASE_645_2007
 	 	 	    || pCpLink->protocol==SINGLE_LOCAL_CHARGE_CTRL_2007
 	 	 	     || pCpLink->protocol==SINGLE_REMOTE_CHARGE_CTRL_2007
 	 	 	   )
 	 	  {
 	 	    if (pCpLink->recvBuf[12]==0x6 && (pCpLink->recvBuf[11]==0x00 || pCpLink->recvBuf[11]==0x01 || pCpLink->recvBuf[11]==0x02))
 	 	    {
      	  pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
      	 
          if (pCpLink->recvBuf[11]==0x00)   //�ն���ʱ��
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
      	  pCpLink->copyedData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
      	 
          if (pCpLink->recvBuf[11]==0x00)        //�ն���ʱ��
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
            pCpLink->copyedData |= HAS_LAST_DAY_REQ;     //����һ���ն�����������

            offsetx = offset+27;
            while (loadLen>=8)
            {
         	    //����3bytes
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset)   = *(pData);
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+1) = *(pData + 1);
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offset+2) = *(pData + 2);
         	    
         	    //��������ʱ��5bytes
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+0) = *(pData + 3);  //��
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+1) = *(pData + 4);  //ʱ
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+2) = *(pData + 5);  //��
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+3) = *(pData + 6);  //��
         	    *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+offsetx+4) = *(pData + 7);  //��
   
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
              pCpLink->copyedData |= HAS_HOUR_FREEZE_ENERGY;     //�����㶳���������
        	     
        	    //���㶳��ʱ��
        	    for(i=0;i<5;i++)
        	    {
        	      *(pCpLink->dataBuff+128+i)   = *(pData+i);
        	    }
        	     
        	    pData+=6;   //������һ��0xaaΪ�ָ���
        	     
        	    //���㶳�������й�����ʾֵ
        	    for(i=0;i<4;i++)
        	    {
        	      *(pCpLink->dataBuff+i)   = *(pData+i);
        	    }

        	    pData+=5;   //������һ��0xaaΪ�ָ���
        	     
        	    //���㶳�ᷴ���й�����ʾֵ
        	    for(i=0;i<4;i++)
        	    {
        	      *(pCpLink->dataBuff+4+i)   = *(pData+i);
        	    }
        	     
        	    // //��������㶳������
 		 	        //multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		 	        //                     pCpLink->dataBuff, HOUR_FREEZE, \
		 	        //                       0x0, LENGTH_OF_HOUR_FREEZE_RECORD);
            }
    	 	  }
        }
 	 	  }
 	 	  break;
 	 	 
 	  case EXT_EVENT_RECORD_13:    //07�����ļ�����ͳ������
      pCpLink->copyedData |= HAS_PARA_VARIABLE; //�вα�������
 	 	  if (pCpLink->recvBuf[11]==00)    //�������������ۼ�ʱ��
 	 	  {
        for(i=0;i<3;i++)
        {
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
        }
 	 	  }
 	 	  else                //���һ�ζ�����ʼʱ�估����ʱ��
 	 	  {
        for(i=0;i<6;i++)
        {
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
        }
 	 	  }
 	 	  break;
 	 	 
 	  case 0x1d:
 	  case 0x1e:
 	 	  if (pCpLink->recvBuf[12]==0x00 && pCpLink->recvBuf[11]==0x01 && pCpLink->recvBuf[10]==0x1)  //��һ����/��բ����ʱ��
 	 	  {
        pCpLink->copyedData |= HAS_PARA_VARIABLE; //�вα�������
        
        for(i=0;i<6;i++)
        {
          *(pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+offset+i) = *(pData+i);
        }
 	 	  }
 	 	  break;
 	 	     	 	
 	  default:
 	 	  return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����
 	 	  break;
  }
   
  return 0;    //��������֡�����ҽ�����ȷ,�ѱ��������
}

/**************************************************
��������:threadOfEthCopyDataRead
��������:��ȡ��̫������TCP�����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��void
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

  printf("%02d-%02d-%02d %02d:%02d:%02d,������̫������TCP�����߳�(PID=%d)����\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, getpid());
  
  while(1)
  {
    if (fdOfClient!=NULL)
    {
      if (compareTwoTime(canTimeOut, sysTime))
      {
        printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyDataRead:����CAN2ETH�������ݳ�ʱ,�رտͻ���socket%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, fdOfClient);

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

            printf("threadOfEthCopyDataRead:��������״̬ΪECONNRESET,�ر��˿ͻ���socket\n");
          }
        }
        else 
        {
        	if (recvLen==0)
        	{
            close(fdOfClient);
            fdOfClient=NULL;
            
            printf("threadOfEthCopyDataRead:���ճ���Ϊ0,�ر��˿ͻ���socket\n");
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
        // afn - 0 ȷ��/����
        //     - 1 ��¼/����
        //     - 2 ��������
        // cs  - ָlen,afn,data��������
        for(recvi=0; recvi<recvLen; recvi++)
        {
        	if (flagOfFrame==0)    //û����֡ͷ
        	{
        	  if (0x7e==recvBuf[recvi])
        	  {
        	  	flagOfFrame = 1;    //����֡��ʼ
        	  	
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
                    	case 1:    //��¼��������
                    		if (1==canTcpData[3])
                    		{
                    			canloginIn = 1;

                          if (debugInfox&ETH_COPY_METER)
                          {
                            printf(",CAN2ETH��¼");
                          }
                    		}
                    		
                        if (debugInfox&ETH_COPY_METER)
                        {
                    		  printf(",confirm it.\n");
                    		}
                    		
                    		sendToCan2Eth(0, canTcpData, 0);    //ȷ��
                    		break;
                      
                      case 2:    //��������
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

                              //����У����Ұ��ֽڽ���������м�0x33����
                              checkSum = 0;
                              for(tmpi=0; tmpi<pTmpCpLink->lenOfRecv-2; tmpi++)
                              {
                                checkSum += pTmpCpLink->recvBuf[tmpi];
                                
                                if (tmpi>9)
                                {
                                  pTmpCpLink->recvBuf[tmpi] -= 0x33;    //���ֽڽ��ж��������0x33����
                                }
                              }
                              
                              //���У�����ȷ,ִ�н��ղ���
                              if (checkSum == pTmpCpLink->recvBuf[pTmpCpLink->lenOfRecv-2])
                              {
                                if (debugInfox&ETH_COPY_METER)
                                {
                                  printf(" -> ���%02x%02x%02x%02x%02x%02x",
                                        pTmpCpLink->recvBuf[6], pTmpCpLink->recvBuf[5], pTmpCpLink->recvBuf[4], pTmpCpLink->recvBuf[3], pTmpCpLink->recvBuf[2], pTmpCpLink->recvBuf[1]
                                        );
                                }

                                switch(pTmpCpLink->recvBuf[8])
                                {
                                	case 0x81:    //97����ȷӦ��
                                		analyse645Dlt1997(pTmpCpLink);
                                		break;
                                		
                                	case 0xc1:    //97���쳣Ӧ��
                                    if (debugInfox&ETH_COPY_METER)
                                    {
                                    	printf(",1997��Լ,�쳣Ӧ��\n");
                                    }
                                		break;

                                	case 0x91:    //07����ȷӦ��
                                		analyse645Dlt2007(pTmpCpLink);
                                		break;
                                		
                                	case 0xd1:    //07���쳣Ӧ��
                                    if (debugInfox&ETH_COPY_METER)
                                    {
                                    	printf(",2007��Լ,�쳣Ӧ��\n");
                                    }                                		
                                		break;
                                		
                                	default:
                                    if (debugInfox&ETH_COPY_METER)
                                    {
                                		  printf(",δ�����Ӧ��(0x%0x)\n", pTmpCpLink->recvBuf[8]);
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
                    
                    canTimeOut = nextTime(sysTime, 0, 10);    //10��������ʱʱ��
                  }
                  else
                  {
                    if (debugInfox&ETH_COPY_METER)
                    {
                  	  printf("֡У�����\n");
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

  printf("%02d-%02d-%02d %02d:%02d:%02d,������̫������TCP�����߳���ֹ(socket=%d)\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, fdOfClient);
}

/**************************************************
��������:threadOfEthCopyServer
��������:��̫������TCPͨ��Server�߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void *threadOfEthCopyServer(void *arg)
{
  int                fdOfServerSocket;    //����socket
  int                tmpSocket;           //��ʱSocket
  struct sockaddr_in myAddr;              //������ַ��Ϣ
  struct sockaddr_in clientAddr;          //�ͻ���ַ��Ϣ
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
  
  //���õ�ַ���������úͼ���
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

    //ͬһʱ��ֻ����һ���豸����
    if (fdOfClient!=NULL)
    { 
      printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyServer:��socket��¼,�ر�����ԭ�ͻ���socket=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, fdOfClient);

    	close(fdOfClient);
      fdOfClient = NULL;
    }
    
    canTimeOut = nextTime(sysTime, 0, 20);
    canloginIn = 0;
    flagOfFrame = 0;    //���ձ�־��Ϊδ��ʼ
    
    setNonBlocking(tmpSocket);
    fdOfClient = tmpSocket;    
  }
  
  printf("�߳�Server����\n");
}

/***************************************************
��������:threadOfEthCopyMeter
��������:ͨ����̫�������߳�
���ú���:
�����ú���
�������:��
�������:
����ֵ��void
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
          printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyMeter,��ʼ��̫����������\n", 
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

        //ɾ�������еĸ��ڵ�
        while(pEthCpLinkHead!=NULL)
        {
          pTmpCpLink = pEthCpLinkHead;
        	
        	pEthCpLinkHead = pEthCpLinkHead->next;
        	free(pTmpCpLink);
        }
        pEthCpLinkHead = NULL;
        
        //��ʼ���˿�5�������
        pEthCpLinkHead = initPortMeterLink(4);
    
        pTmpCpLink = pEthCpLinkHead;
        while(pTmpCpLink!=NULL)
        {
  	      //��һ����0�뿪ʼ����
  	      pTmpCpLink->nextCopyTime = nextTime(sysTime, 1, 0);
  	      pTmpCpLink->nextCopyTime.second = 0;
  	      pTmpCpLink->copying = FALSE;
  	  
  	      pTmpCpLink = pTmpCpLink->next;
  	    }
  	    
  	    setEthMeter = 0;

        if (debugInfo&METER_DEBUG)
        {
          printf("%02d-%02d-%02d %02d:%02d:%02d,threadOfEthCopyMeter,��ʼ��̫�������������\n", 
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
              printf("%02d-%02d-%02d %02d:%02d:%02d,ethCopyMeter:���%02x%02x%02x%02x%02x%02x��ʼ����\n", sysTime.year, 
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
                printf("%02d-%02d-%02d %02d:%02d:%02d,ethCopyMeter:���%02x%02x%02x%02x%02x%02x���ֳ������,copyedData=%X\n", sysTime.year, 
                       sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second,
                        pTmpCpLink->addr[5],pTmpCpLink->addr[4],pTmpCpLink->addr[3],pTmpCpLink->addr[2],pTmpCpLink->addr[1],pTmpCpLink->addr[0]
                        ,pTmpCpLink->copyedData
                      );
              }
        		  
        		  //�вɼ����ĵ���������(��ǰ)
        		  if (pTmpCpLink->copyedData&HAS_CURRENT_ENERGY)
        		  {
        		 	  saveMeterData(pTmpCpLink->mp, 5, timeHexToBcd(sysTime), pTmpCpLink->dataBuff, 
                              PRESENT_DATA, ENERGY_DATA, LENGTH_OF_ENERGY_RECORD);
        		  }
        	   
        	    //�вɼ���������������ʱ������(��ǰ)
              if (pTmpCpLink->copyedData&HAS_CURRENT_REQ)
              {
        		 	  saveMeterData(pTmpCpLink->mp, 5, timeHexToBcd(sysTime), &pTmpCpLink->dataBuff[LENGTH_OF_ENERGY_RECORD], 
                              PRESENT_DATA, REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
              }
             
        	    //�вɼ����Ĳ������α�������(��ǰ)
              if (pTmpCpLink->copyedData&HAS_PARA_VARIABLE)
              {
      		      //����DLT_645_2007�������ܶ���������ۼƶ���ʱ�估���һ�ζ���ʱ��,����õ� 10-07-05
                /*
                if (multiCopy[arrayItem].protocol == DLT_645_2007)
                {
                 //�ֲ�ȡ�İ취����A,B,C��Ĵ�����ʱ������������Ϊ�ܶ���������ۼ�ʱ��
             	  pParaData = pCpLink->dataBuff+LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD;
             	
             	  //�����ܶ������
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
      
             	  //�����ۼƶ���ʱ��
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
      
             	  //���һ�ζ�����ʼʱ��
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
     
               	 
               	//���һ�ζ������ʱ��
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
         		
              //�вɼ�����ʱ������
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
            //��ʱ����
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


