/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN02.c
���ߣ�leiyong
�汾��0.9
������ڣ�2006��7��
��������վAFN02(��·�ӿڼ��)�����ļ���
�����б�
     1.
�޸���ʷ��
  01,06-7-28,Leiyong created.
  02,09-03-10,Leiyong����,�������Ӵ���ͳ��
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
 extern DATE_TIME      nextCdmaHeartTime;            //��һ��CDMA����ʱ��
#endif

extern FRAME_QUEUE          fQueue;                  //֡����

/*******************************************************
��������:AFN02001Login
��������:���͵�¼�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
  
   /*  ly,2011-10-11,ע��,�ĵ�replyToMsͳһ����
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
     	 printf("AFN02001Login:��������\n");
     }

     //������ʱ������TCP����
     fQueue.inTimeFrameSend = TRUE;
   }
   
   
   //fQueue.sendPtr = fQueue.tailPtr;
   //fQueue.thisStartPtr = fQueue.tailPtr;

   //��¼�շ������Ӵ����������ֽ���
   //frameReport(1, 20, 1);
}

/*******************************************************
��������:AFN02001Logout
��������:�����˳���¼�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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
   
   /*  ly,2011-10-11,ע��,�ĵ�replyToMsͳһ����
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
     //������ʱ������TCP����
     fQueue.inTimeFrameSend = TRUE;
   }

   //fQueue.sendPtr = fQueue.tailPtr;
   //fQueue.thisStartPtr = fQueue.tailPtr;

 	 //frameReport(1, 20, 0);
}

/*******************************************************
��������:AFN02003HeartBeat
��������:�����ն�������������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void()
*******************************************************/
void AFN02003HeartBeat(void)
{
  //INT8U  i;
  INT16U i;    //2015-11-06,�޸��������,ˮ��������һֱ���ܽ�������Ϣ������������i��8λ,�������ѭ��
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

  //2015-7-17,����L�ں�����д
     
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

	msFrame[frameTail02+18] = sysTime.second/10<<4 | sysTime.second%10;    //��(ǰ��λBCD��ʮλ������λBCD���λ)
	msFrame[frameTail02+19] = sysTime.minute/10<<4 | sysTime.minute%10;    //��(ǰ��λBCD��ʮλ������λBCD���λ)
	msFrame[frameTail02+20] = sysTime.hour  /10<<4 | sysTime.hour  %10;    //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
	msFrame[frameTail02+21] = sysTime.day   /10<<4 | sysTime.day   %10;    //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  	
	if (sysTime.month<10)
	{
	  msFrame[frameTail02+22] = weekNumber<<5 | sysTime.month;             //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	}
	else
	{
	  msFrame[frameTail02+22] = weekNumber<<5 | 0x10 | sysTime.month%10;   //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	}
	msFrame[frameTail02+23] = sysTime.year/10<<4 | sysTime.year%10;        //��(ʮλ+��λ)

	//2015-7-14
	msFrame[frameTail02+24] = wlRssi;    //����MODEM�ź�
  if (teIpAndPort.ethIfLoginMs==0x55)
  {
    msFrame[frameTail02+24] |= 0x80;
  }
	 
	//2015-7-14,���ն�
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
   
  //2015-7-20,��ͬ���ĵ��ƿ���������
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
  
  //2015-10-26,״̬���µĿ��Ƶ�
  tmpCount = 0;  
  tmpSuccess = tmpTail;    //tmpSuccess����и��µĿ��Ƶ�ĸ���
  tmpTail += 2;
  
  //��·���Ƶ�
  tmpNode = xlcLink;
  while(tmpNode!=NULL)
  {
  	 if( 
		    (1==(tmpNode->statusUpdated&0x1))
				 || statusHeartBeat>=600    //2016-11-24,Add,ÿ10���Ӹ���һ��
			 )
  	 {
  	 	 //���Ƶ��
  	 	 msFrame[tmpTail++] = tmpNode->mp&0xff;
  	 	 msFrame[tmpTail++] = tmpNode->mp>>8&0xff;
  	 	 
  	 	 //״̬
  	 	 msFrame[tmpTail++] = tmpNode->status;
  	 	 
  	 	 tmpNode->statusUpdated &= 0xfe;
  	 	 tmpCount++;
  	 }
  	 
  	 tmpNode = tmpNode->next;
  }
  
  //���ƿ��Ƶ�
  tmpNode = copyCtrl[4].cpLinkHead;
  while(tmpNode!=NULL)
  {
  	if (
		    (1==(tmpNode->statusUpdated&0x1))
				 || statusHeartBeat>=600    //2016-11-24,Add,ÿ10���Ӹ���һ��
			 )
  	{
  	 	//���Ƶ��
  	 	msFrame[tmpTail++] = tmpNode->mp&0xff;
  	 	msFrame[tmpTail++] = tmpNode->mp>>8&0xff;
  	 	 
  	 	//����
  	 	msFrame[tmpTail++] = tmpNode->status;
  	 	 
  	 	tmpNode->statusUpdated &= 0xfe;
  	 	tmpCount++;
  	}
  	 
  	//����200�����Ƶ��и���״̬,�´������ٷ���
  	if (tmpCount>=200)
  	{
  	 	break;
  	}
  	 
  	tmpNode = tmpNode->next;
  }
  
  //״̬���µĿ��Ƶ�����
  msFrame[tmpSuccess] = tmpCount&0xff;
  msFrame[tmpSuccess+1] = tmpCount>>8&0xff;
  
  //2016-03-01,�ճ�����״̬���µ���·���Ƶ�
  tmpCount = 0;
  tmpSuccess = tmpTail;    //tmpSuccess����и��µĿ��Ƶ�ĸ���
  tmpTail += 2;
  
  //��·���Ƶ�
  tmpNode = xlcLink;
  while(tmpNode!=NULL)
  {
  	 if (
		     (0x2==(tmpNode->statusUpdated&0x2))
			 		|| statusHeartBeat>=600    //2016-11-24,Add,ÿ10���Ӹ���һ��
				)
  	 {
  	 	 //���Ƶ��
  	 	 msFrame[tmpTail++] = tmpNode->mp&0xff;
  	 	 msFrame[tmpTail++] = tmpNode->mp>>8&0xff;
  	 	 
  	 	 //�ճ�����ʱ��
  	 	 msFrame[tmpTail++] = tmpNode->duty[0];
  	 	 msFrame[tmpTail++] = tmpNode->duty[1];
  	 	 msFrame[tmpTail++] = tmpNode->duty[2];
  	 	 msFrame[tmpTail++] = tmpNode->duty[3];
  	 	 
  	 	 tmpNode->statusUpdated &= 0xfd;
  	 	 
  	 	 tmpCount++;
  	 }
  	 
  	 tmpNode = tmpNode->next;
  }
  //�ճ�����״̬���µĿ��Ƶ�����
  msFrame[tmpSuccess] = tmpCount&0xff;
  msFrame[tmpSuccess+1] = tmpCount>>8&0xff;

	//������ң��,2018-6-15
	//msFrame[tmpTail++] = ioctl(fdOfIoChannel, READ_YX_VALUE, 0);
  //2018-09-14,�ĳ���Ӧ���ϺͶ���
	msFrame[tmpTail++] = stOfSwitch;

	//���¶ȴ���,2018-6-15
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
  tmpData = ((tmpTail-frameTail02-6)<<2) | PROTOCOL_FIELD;    //2015-10-28,�޸��������
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
			 "%02d-%02d-%02d %02d:%02d:%02d,�������״̬����ֵ\n",
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
    //������ʱ������TCP����
    fQueue.inTimeFrameSend = TRUE;
  }

  //��¼���������Ӵ����������ֽ���
  //frameReport(1, 20, 2);
}

#ifdef CQDL_CSM

/*******************************************************
��������:AFN02008
��������:�����·�ע�����Ϣ����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
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

   //��¼���������Ӵ����������ֽ���
   frameReport(1, 20, 2);
}

#endif
