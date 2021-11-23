/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN0E.c
���ߣ�TianYe
�汾��0.9
������ڣ�2006��8��
��������վ"��ʾ��������"(AFN0E)�Ĵ�����
�����б�
�޸���ʷ��
  01,06-8-9,TinaYe created.
  02,06-10-17,leiyong,����֡����
***************************************************/

#include "common.h"
#include "teRunPara.h"
#include "convert.h"

#include "dataBase.h"
#include "AFN0E.h"
#include "workWithMeter.h"

/*******************************************************
��������:AFN0E
��������:��վ"��ʾ��������"(AFN0E)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN0E(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom, INT8U poll)
{
    INT8U      fn, pn,tmpPm, i;
    INT16U     pointm, pointn;
    INT16U     eventNum;
    INT16U     frameTail0e;
    INT8U      frameCounter,*pHandle;
    INT16U     tmpI,tmpHead0e,tmpHead0eActive;    
    INT32U     baseAddr;
    INT16U     blkInSect;    
    INT32U     addrHead, addrWriteTo;
    INT8U     *pTpv;                   //TpVָ��
    EVENT_HEAD eventHead;
    INT16U     (*readEvent[36])(INT16U frameTail, INT8U *eventData);
    INT8U      tmpReadBuff[200];
    DATE_TIME  tmpTime;
    TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
    
    for(i=0;i<36;i++)
    {
    	readEvent[i] = NULL;
    }
    
    readEvent[0] = eventErc01;
    readEvent[1] = eventErc02;
    readEvent[2] = eventErc03;
    readEvent[3] = eventErc04;
    readEvent[4] = eventErc05;
    readEvent[5] = eventErc06;
    readEvent[6] = eventErc07;
    readEvent[7] = eventErc08;
    readEvent[8] = eventErc09;
    readEvent[9] = eventErc10;
    readEvent[10] = eventErc11;    
    readEvent[11] = eventErc12;
    readEvent[12] = eventErc13;
    readEvent[13] = eventErc14;

    readEvent[16] = eventErc17;
    
    readEvent[18] = eventErc19;
    readEvent[19] = eventErc20;
    readEvent[20] = eventErc21;
    readEvent[21] = eventErc22;
    readEvent[22] = eventErc23;
    
    readEvent[23] = eventErc24;
    readEvent[24] = eventErc25;
    readEvent[25] = eventErc26;
    readEvent[26] = eventErc27;
    readEvent[27] = eventErc28;
    readEvent[28] = eventErc29;
    readEvent[29] = eventErc30;
    
    readEvent[30] = eventErc31;
    readEvent[31] = eventErc32;
    readEvent[32] = eventErc33;
    readEvent[34] = eventErc35;
	 #ifdef LIGHTING
	  readEvent[35] = eventErc36;
   #endif	
        
    //�������ݵ�Ԫ��ʶ��ֵ,����FNֵ��ȷ������������
    pn = findFnPn(*pDataHead, *(pDataHead+1), FIND_PN);
    fn = findFnPn(*(pDataHead+2), *(pDataHead+3), FIND_FN);
    if (pn != 0 || fn > 2 || fn < 1)
    {
       return;
    }

    pHandle = pDataHead + 4;
    pointm = *pHandle++;      //Pm
    pointn = *pHandle;        //Pn
    
    //if (pointm<1)
    //{
    //	pointm = 1;
    //}
    //if (pointn<1)
    //{
    //	pointn = 1;
    //}
    
    printf("pm=%d,pn=%d\n",pointm,pointn);
    
    //���������¼��ĸ���
    eventNum = 0;
    if (pointm < pointn)
    {
      eventNum = pointn - pointm;
    }
    else
    {
      if (pointm > pointn)
      {
        eventNum = 256 + pointn - pointm;
      }
      else
      {
      	eventNum = 1;
      }
    }
    
    //��������¼��������¼�������ֵ����������������֡
    if (fn == 1)
    {
      if (eventNum > iEventCounter)
      {
        //����������֡
        ackOrNack(FALSE, dataFrom);
        return;
      }
      if (pointn > iEventCounter || (pointm >= iEventCounter && pointn < pointm))
      {
        //����������֡
        ackOrNack(FALSE, dataFrom);
        return;
      }
    }
    else
    {
      if (eventNum > nEventCounter)
      {
        ackOrNack(FALSE, dataFrom);
        return;
      }
      if (pointn > nEventCounter || (pointm >= nEventCounter && pointn < pointm))
      {
        //����������֡
        ackOrNack(FALSE, dataFrom);
        return;
      }
    }
    
    //��λpm�Ĵ洢��ַ(��δ��)

    //��������    
    frameCounter = 0;
    tmpPm = pointm;

    if (fQueue.tailPtr == 0)
    {
       tmpHead0e = 0;
    }
    else
    {
       tmpHead0e = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    frameTail0e = tmpHead0e + 22;

    printf("��������ѭ��pointm=%d,pointn=%d\n",pointm,pointn);
    //for(; pointm<pointn; pointm++)
    for(; eventNum > 0; eventNum--,pointm++)
    {
      printf("ѭ����pointm=%d,pointn=%d\n",pointm,pointn);
      tmpTime = sysTime;
      if (readMeterData(tmpReadBuff, fn , EVENT_RECORD, pointm+1, &tmpTime, 0)==TRUE)
      {
          eventHead.erc = tmpReadBuff[0];
          
          if (debugInfo&PRINT_EVENT_DEBUG)
          { 
            printf("ERC=%d\n",eventHead.erc);
          }
          
         #ifdef LIGHTING
					if (eventHead.erc >= 1 && eventHead.erc <= 36)
				 #else
					if (eventHead.erc >= 1 && eventHead.erc <= 35)
				 #endif
          {
            if (readEvent[eventHead.erc-1]!=NULL)
            {
              frameTail0e = readEvent[eventHead.erc-1](frameTail0e, tmpReadBuff);
            }
          }
      }
      else
      {
      	
      }
      
      //�¼��������ϻ��߷��ͻ�����������
      if ((frameTail0e-tmpHead0e) > MAX_OF_PER_FRAME || (eventNum <=1))
      {
        if (poll != ACTIVE_REPORT)
        {
          //��������վҪ���ж��Ƿ�Я��TP
          if (frame.pTp != NULL)
          {
             pTpv = frame.pTp;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv;
          }
        }

        msFrame[tmpHead0e+0] = 0x68;                  //֡��ʼ�ַ�
        
        tmpI = ((frameTail0e-tmpHead0e-6) << 2) | PROTOCOL_FIELD;
        msFrame[tmpHead0e+1] = tmpI & 0xFF;           //L
        msFrame[tmpHead0e+2] = tmpI >> 8;
        msFrame[tmpHead0e+3] = tmpI & 0xFF;           //L
        msFrame[tmpHead0e+4] = tmpI >> 8;
        
        msFrame[tmpHead0e+5] = 0x68;                  //֡��ʼ�ַ�
        
        if (poll==ACTIVE_REPORT)
        {
           msFrame[tmpHead0e + 6] = 0xc4;     //DIR=1,PRM=1,������=0x4
        }
        else
        {
           msFrame[tmpHead0e + 6] = 0x88;     //�����ֽ�10001000(DIR=1,PRM=0,������=0x8)
        }
          
        //��ַ
        msFrame[tmpHead0e+7]  = addrField.a1[0];
        msFrame[tmpHead0e+8]  = addrField.a1[1];
        msFrame[tmpHead0e+9]  = addrField.a2[0];
        msFrame[tmpHead0e+10] = addrField.a2[1];
        
        if (poll == ACTIVE_REPORT)
        {
           msFrame[tmpHead0e+11] = 0x0;
        }
        else
        {
           msFrame[tmpHead0e+11] = addrField.a3;
        }
        
        msFrame[tmpHead0e+12] = 0x0E;                //AFN

        msFrame[tmpHead0e+13] = 0;
        if (poll==ACTIVE_REPORT)
        {
        	//2015-2-10,�����ϱ��¼���Ҫȷ��
         	msFrame[tmpHead0e+13] |= 0x10;
        }
        else
        {
          //�������ϱ������б��Ĵ�ʱ��
         	if (frame.pTp != NULL)
         	{
         	  msFrame[tmpHead0e+13] |= 0x80;       //TpV��λ
         	}
        }
            
        pHandle = pDataHead;
        
        msFrame[tmpHead0e+14] = *pHandle++;
        msFrame[tmpHead0e+15] = *pHandle++;
        msFrame[tmpHead0e+16] = *pHandle++;
        msFrame[tmpHead0e+17] = *pHandle;
        
        msFrame[tmpHead0e+18] = iEventCounter;
        msFrame[tmpHead0e+19] = nEventCounter;

        msFrame[tmpHead0e+20] = tmpPm;          //��֡���Ĵ��͵��¼���¼��ʼָ��
        msFrame[tmpHead0e+21] = pointm+1;         //��֡���Ĵ��͵��¼���¼����ָ��
        tmpPm = pointm;
                       
        frameTail0e++;
        msFrame[frameTail0e++] = 0x16;

        fQueue.frame[fQueue.tailPtr].head = tmpHead0e;
        fQueue.frame[fQueue.tailPtr].len  = frameTail0e-tmpHead0e;
        
        if (poll==ACTIVE_REPORT)
        {
        	 printf("����֡\n");
        	 //����õ�֡���Ƶ������ϱ�֡������
           if (fQueue.activeTailPtr == 0)
           {
              tmpHead0eActive = 0;
           }
           else
           {
              tmpHead0eActive = fQueue.activeFrame[fQueue.activeTailPtr-1].head + fQueue.activeFrame[fQueue.activeTailPtr-1].len;
           }
           fQueue.activeFrame[fQueue.activeTailPtr].head = tmpHead0eActive;
           fQueue.activeFrame[fQueue.activeTailPtr].len  = fQueue.frame[fQueue.tailPtr].len;
           for(tmpI=0;tmpI<fQueue.activeFrame[fQueue.activeTailPtr].len;tmpI++)
           {
           	 activeFrame[tmpHead0eActive+tmpI] = msFrame[fQueue.frame[fQueue.tailPtr].head+tmpI];
           }

           if ((tmpHead0eActive+tmpI+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
           	   || fQueue.activeTailPtr==LEN_OF_SEND_QUEUE-1)
           {
              fQueue.activeFrame[fQueue.activeTailPtr].next = 0x0;
           	  fQueue.activeTailPtr = 0;
           }
           else
           {
              fQueue.activeFrame[fQueue.activeTailPtr].next = fQueue.activeTailPtr+1;
              fQueue.activeTailPtr++;
           }
        }
        else
        {
           tmpHead0e = frameTail0e;
           
           if ((tmpHead0e+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
           	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
           {
             fQueue.frame[fQueue.tailPtr].next = 0x0;
           	 fQueue.tailPtr = 0;
           	 tmpHead0e = 0;
           }
           else
           {
              fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
              fQueue.tailPtr++;
           }
        }
        
        frameTail0e = tmpHead0e+22;  //frameTail������Ϊ21��д��һ֡
      }
    }

    printf("�˳�ѭ��pointm=%d,pointn=%d\n",pointm,pointn);
   
    //�����ն�ͳ�Ƽ�¼    
    eventReadedPointer[0] = iEventCounter;
    if (pointn<=255)
    {
    	eventReadedPointer[1] = pointn;
    }
    else
    {
    	eventReadedPointer[1] = 255;
    }
    
    if (debugInfo&PRINT_EVENT_DEBUG)
    {
      printf("AFN0E:�Ѷ��¼�ָ��=%d\n",eventReadedPointer[1]);
    }
    
    saveParameter(88, 2,&eventReadedPointer, 2);
}

/*******************************************************
��������:eventErc01
��������:��дerc01�¼���¼�ϱ�֡�����ݲ���(���ݳ�ʼ���Ͱ汾�����¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc01(INT16U frameTail, INT8U *eventData)
{    
   msFrame[frameTail++] = ERC(1);     //erc
   msFrame[frameTail++] = 0x0E;       //����
  
   //��������ʱ��
   eventData += 3;
   
	 msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   msFrame[frameTail++] = *eventData;
   eventData += 2;
   
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;

   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   return frameTail;
}

/*******************************************************
��������:eventErc02
��������:��дerc02�¼���¼�ϱ�֡�����ݲ���(������ʧ��¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc02(INT16U frameTail, INT8U *eventData)
{
    msFrame[frameTail++] = ERC(2);
    msFrame[frameTail++] = 0x6;
    
    eventData += 3;
    
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
      
    msFrame[frameTail++] = *eventData++;
    
    return frameTail;
}

/*******************************************************
��������:eventErc03
��������:��дerc03�¼���¼�ϱ�֡�����ݲ���(���������¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc03(INT16U frameTail, INT8U *eventData)
{
    INT8U tmpNum;
    
    msFrame[frameTail++] = ERC(3);       //erc
    
    tmpNum = *(eventData+1);
    msFrame[frameTail++] = 6+tmpNum*4;   //����
  
    //��������ʱ��
    eventData += 3;
	  msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    
    msFrame[frameTail++] = *eventData++;
    
    //���ݵ�Ԫ��ʶ
    for(;tmpNum>0;tmpNum--)
    {
      msFrame[frameTail++] = *eventData++;
      msFrame[frameTail++] = *eventData++;
    
      msFrame[frameTail++] = *eventData++;
      msFrame[frameTail++] = *eventData++;
    }

    return frameTail;
}

/*******************************************************
��������:eventErc04
��������:��дerc04�¼���¼�ϱ�֡�����ݲ���(״̬����λ��¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc04(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(4);
	msFrame[frameTail++] = 0x7;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc05
��������:��дerc05�¼���¼�ϱ�֡�����ݲ���(ң����բ��¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc05(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(5);
	msFrame[frameTail++] = 0x0a;
	
	eventData += 3;
	
	//��բʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��բ�ִ�
  msFrame[frameTail++] = *eventData++;
	
	//��բʱ����(�ܼӹ���)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��բ��2min�Ĺ���
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
	
	return frameTail;
}

/*******************************************************
��������:eventErc06
��������:��дerc06�¼���¼�ϱ�֡�����ݲ���(������բ��¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc06(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(6);
	msFrame[frameTail++] = 0x0E;
	
	eventData += 3;
	
	//��բʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�ܼ����
	msFrame[frameTail++] = *eventData++;
	
	//��բ�ִ�
	msFrame[frameTail++] = *eventData++;

	//�������
	msFrame[frameTail++] = *eventData++;
	
	//��բǰ����(�ܼӹ���)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��բ��2min�Ĺ���
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //��բʱ�Ĺ��ʶ�ֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
		
	return frameTail;
}

/*******************************************************
��������:eventErc07
��������:��дerc07�¼���¼�ϱ�֡�����ݲ���(�����բ��¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc07(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(7);
	msFrame[frameTail++] = 0x10;
	
	eventData += 3;
	
	//��բʱ��(��ʱ������)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//�ܼ����
	msFrame[frameTail++] = *eventData++;
	  
	//��բ�ִ�
	msFrame[frameTail++] = *eventData++;
	
	//������
	msFrame[frameTail++] = *eventData++;

  //��բʱ������(�ܼӵ�����)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//��բʱ��������ֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
	
	return frameTail;
}

/*******************************************************
��������:eventErc08
��������:��дerc05�¼���¼�ϱ�֡�����ݲ���(�������������)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc08(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(8);
	msFrame[frameTail++] = 0x8;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������  
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//�����־
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc09
��������:��дerc09�¼���¼�ϱ�֡�����ݲ���(������·�쳣)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc09(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(9);
	msFrame[frameTail++] = 28;
	
	eventData += 3;
	
	//����ʱ��
  msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//�쳣��־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ia
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//����ʱ��Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ���ܱ������й��ܵ���ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc10
��������:��дerc10�¼���¼�ϱ�֡�����ݲ���(��ѹ��·�쳣)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc10(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(10);
	
	msFrame[frameTail++] = 0x1C;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������ż���/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�쳣��־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ia
  msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	  
	//����ʱ��Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//����ʱ�������й�ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc11
��������:��дerc11�¼���¼�ϱ�֡�����ݲ���(�����쳣)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc11(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(11);
	
	msFrame[frameTail++] = 0x18;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������ż���/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//��Ia
  msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	  
	//��Ib
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//��Ic
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//����ʱ�������й�ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc12
��������:��дerc12�¼���¼�ϱ�֡�����ݲ���(���ܱ�ʱ�䳬��)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc12(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(12);
	
	msFrame[frameTail++] = 0x7;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc13
��������:��дerc13�¼���¼�ϱ�֡�����ݲ���(��������Ϣ)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc13(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(13);
	msFrame[frameTail++] = 0x8;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������(��������־)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //�쳣��־
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc14
��������:��дerc14�¼���¼�ϱ�֡�����ݲ���(�ն�ͣ/�ϵ��¼�)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc14(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(14);
	msFrame[frameTail++] = 0x0A;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	return frameTail;
}

/*******************************************************
��������:eventErc17
��������:��ѹ/������ƽ��ȳ��޼�¼
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc17(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(17);
	
	msFrame[frameTail++] = 27;
	
	eventData += 2;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;  //min
	msFrame[frameTail++] = *eventData++;  //hour
	msFrame[frameTail++] = *eventData++;  //day
	msFrame[frameTail++] = *eventData++;  //month
	msFrame[frameTail++] = *eventData++;  //month

	//�����㼰��/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //�쳣��־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ�ĵ�ѹ��ƽ���
	msFrame[frameTail++] = *eventData++;	//unbalanceU
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ�ĵ�����ƽ���
	msFrame[frameTail++] = *eventData++;  //unbalanceC
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ia
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	return frameTail;
}


/*******************************************************
��������:eventErc19
��������:��дerc19�¼���¼�ϱ�֡�����ݲ���(����������ü�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc19(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(19);
	msFrame[frameTail++] = 0x1f;
	
	eventData += 3;
	
	//������������ʱ��(��ʱ������)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�ܼ����
	msFrame[frameTail++] = *eventData++;
	
	//���絥��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//׷��/ˢ�±�־  
	msFrame[frameTail++] = *eventData++;
	
	//������ֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//��������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//��բ����
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//����ǰʣ�������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//�����ʣ�������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc20
��������:��дerc20�¼���¼�ϱ�֡�����ݲ���(��Ϣ��֤�����¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc20(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(20);
	msFrame[frameTail++] = 0x16;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc21
��������:��дerc21�¼���¼�ϱ�֡�����ݲ���(�ն˹��ϼ�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc21(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(21);
	msFrame[frameTail++] = 0x06;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc22
��������:��дerc22�¼���¼�ϱ�֡�����ݲ���(�й��ܵ������Խ���¼�)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc22(INT16U frameTail, INT8U *eventData)
{
	INT16U tmpTail;
	INT8U i,tmpCount,tmpCountx;
	
	msFrame[frameTail++] = ERC(22);
	tmpTail = frameTail++;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//���������ż���/ֹ��־
	msFrame[frameTail++] = *eventData++;
	
	//Խ��ʱ�Ա��ܼ����й��ܵ�����
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//Խ��ʱ�����ܼ����й��ܵ�����
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//Խ��ʱ�Խ�����ƫ��ֵ
	msFrame[frameTail++] = *eventData++;

	//Խ��ʱ�Խ�޾���ƫ��ֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�Աȵ��ܼ������������n
	msFrame[frameTail++] = tmpCount = *eventData++;
	
	for(i=0;i<tmpCount;i++)
  {
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
  }

	//���յ��ܼ������������m
	msFrame[frameTail++] = tmpCountx = *eventData++;
	
	for(i=0;i<tmpCountx;i++)
  {
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
  }

	msFrame[tmpTail] = (tmpCount+tmpCountx)*5+20;

	return frameTail;
}

/*******************************************************
��������:eventErc23
��������:��дerc23�¼���¼�ϱ�֡�����ݲ���(��ظ澯�¼���¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc23(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(23);
	msFrame[frameTail++] = 0x10;
	
	eventData += 3;
	
	//�澯ʱ��(��ʱ������)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//�ܼ����
	msFrame[frameTail++] = *eventData++;
	  
	//Ͷ���ִ�
	msFrame[frameTail++] = *eventData++;
	
	//������
	msFrame[frameTail++] = *eventData++;

  //�澯ʱ������(�ܼӵ�����)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//�澯ʱ��������ֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
	
	return frameTail;
}

/*******************************************************
��������:eventErc24
��������:��дerc24�¼���¼�ϱ�֡�����ݲ���(��ѹԽ�޼�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc24(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(24);
  
	msFrame[frameTail++] = 0x0E;
	
	eventData += 2;	
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�����㼰��/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
		
	//Խ�ޱ�־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc25
��������:��дerc25�¼���¼�ϱ�֡�����ݲ���(����Խ�޼�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc25(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(25);

	msFrame[frameTail++] = 17;
	
	eventData += 2;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�����㼰��/ֹ��־
  msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
		
	//Խ�ޱ�־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ia
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ��Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc26
��������:��дerc26�¼���¼�ϱ�֡�����ݲ���(���ڹ���Խ�޼�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc26(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(26);
	
	msFrame[frameTail++] = 0x0E;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�����㼰��/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//���ޱ�־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ�����ڹ���  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ�����ڹ�����ֵ
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc27
��������:��дerc26�¼���¼�ϱ�֡�����ݲ���(���ܱ�ʾ���½���¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc27(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(27);
	
	msFrame[frameTail++] = 0x11;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������(����/ֹ��־)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�½�ǰ���ܱ������й�������ʾֵ
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//�½�����ܱ������й�������ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc28
��������:��дerc28�¼���¼�ϱ�֡�����ݲ���(�����������¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc28(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(28);
	msFrame[frameTail++] = 0x12;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//������(����/ֹ��־)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//�����ǰ�����й��ܵ���ʾֵ	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;

	//������������й��ܵ���ʾֵ	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//������ֵ
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc29
��������:��дerc29�¼���¼�ϱ�֡�����ݲ���(���ܱ���߼�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc29(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(29);
	msFrame[frameTail++] = 0x12;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������(����/ֹ��־)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//���߷���ǰ�����й��ܵ���ʾֵ	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;

	//���߷����������й��ܵ���ʾֵ	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//������ֵ
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc30
��������:��дerc30�¼���¼�ϱ�֡�����ݲ���(���ܱ�ͣ�߼�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc30(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(30);
	
	msFrame[frameTail++] = 0x0D;  
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//������(����/ֹ��־)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//ͣ�߷���ʱ�������й��ܵ���ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;  
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//ͣ����ֵ
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc31
��������:��дerc31�¼���¼�ϱ�֡�����ݲ���(�ն�485����ʧ���¼���¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc31(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(31);
	
	msFrame[frameTail++] = 21;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�����㼰��/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//���һ�γ���ɹ�ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;	
	
	//���һ�γ���ɹ������й�����ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//���һ�γ���ɹ������й�����ʾֵ
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc32
��������:��дerc32�¼���¼�ϱ�֡�����ݲ���(�ն�����վͨ�������������¼���¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc32(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(32);
	
	msFrame[frameTail++] = 13;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�����ѷ�����ͨ������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	
	//��ͨ����������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
��������:eventErc33
��������:��дerc33�¼���¼�ϱ�֡�����ݲ���(���ܱ�����״̬�ֱ�λ�¼���¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc33(INT16U frameTail, INT8U *eventData)
{
  INT8U i;
  
  msFrame[frameTail++] = ERC(33);
	
	msFrame[frameTail++] = 35;
	
	eventData += 3;

	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //������
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //��λ��־1-7�����ܱ�����״̬��1--7
  for(i=0;i<28;i++)
  {
	  msFrame[frameTail++] = *eventData++;  	
  }
  
  return frameTail;
}

/*******************************************************
��������:eventErc35
��������:��дerc35�¼���¼�ϱ�֡�����ݲ���(����δ֪����¼���¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc35(INT16U frameTail, INT8U *eventData)
{
  INT16U tmpTail;
  
  msFrame[frameTail++] = ERC(35);
	tmpTail = frameTail++;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	msFrame[frameTail++] = *eventData++;  //�ն�ͨ�Ŷ˿ں�
	msFrame[frameTail++] = *eventData;    //���ֿ���
	memcpy(&msFrame[frameTail],eventData+1,*eventData*8);
	frameTail+= *eventData*8;
	msFrame[tmpTail] = *eventData*8+7;
	
	return frameTail;
}


#ifdef LIGHTING
/*******************************************************
��������:eventErc36
��������:��дerc26�¼���¼�ϱ�֡�����ݲ���(��������Խ�޼�¼)
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U eventErc36(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(36);
	
	msFrame[frameTail++] = 0x0C;
	
	eventData += 3;
	
	//����ʱ��
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//�����㼰��/ֹ��־
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//���ޱ�־
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ�Ĺ�������  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//����ʱ�Ĺ���������ֵ
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}
#endif