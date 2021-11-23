/***************************************************
Copyright,2010,Huawei WoDian co.,LTD
�ļ�����AFN0A.c
���ߣ�wan guihua
�汾��0.9
������ڣ�2010��1��
��������վAFN0A(��ѯ����)�����ļ���
�����б�
  01,10-1-13,Leiyong created.
***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msSetPara.h"
#include "teRunPara.h"
#include "dataBase.h"

#ifdef LIGHTING
 #include "copyMeter.h"
#endif

#include "AFN0A.h"
#include "AFN00.h"

INT16U offset0a;                     //���յ���֡�е����ݵ�Ԫƫ����(���ݱ�ʶ���ֽ�)

/*******************************************************
��������:AFN0A
��������:���ղ�ѯ��������(AFN0A)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN0A(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
    INT16U frameTail0a;             //����β
    INT16U tmpI,tmpFrameTail, tmpHead0a;
    INT8U  frameCounter,checkSum;   //���ͼ����� 
    INT8U  fn, ackTail;
    INT8U  tmpDtCount;              //DT��λ����
    INT8U  tmpDt1;                  //��ʱDT1
    INT8U  *pTpv;                   //TpVָ��
    INT8U  maxCycle;                //���ѭ������ 
    
    INT16U (*AFN0AFun[138])(INT16U tail,INT8U *pHandle);

    for(tmpI=0;tmpI<138;tmpI++)
    {
      AFN0AFun[tmpI] = NULL;
    }
    
    //��1  
    AFN0AFun[0] = AFN0A001;
    AFN0AFun[1] = AFN0A002;
    AFN0AFun[2] = AFN0A003;
    AFN0AFun[3] = AFN0A004;
    AFN0AFun[4] = AFN0A005;
    AFN0AFun[5] = AFN0A006;
    AFN0AFun[6] = AFN0A007;
    AFN0AFun[7] = AFN0A008;
    
    //��2
    AFN0AFun[8]  = AFN0A009;
    AFN0AFun[9]  = AFN0A010;
    AFN0AFun[10] = AFN0A011;
    AFN0AFun[11] = AFN0A012;
    AFN0AFun[12] = AFN0A013;
    AFN0AFun[13] = AFN0A014;
    AFN0AFun[14] = AFN0A015;
    AFN0AFun[15] = AFN0A016;
    
    //��3
    AFN0AFun[16] = AFN0A017;
    AFN0AFun[17] = AFN0A018;
    AFN0AFun[18] = AFN0A019;
    AFN0AFun[19] = AFN0A020;
    AFN0AFun[20] = AFN0A021;
    AFN0AFun[21] = AFN0A022;
    AFN0AFun[22] = AFN0A023;

    //��4
    AFN0AFun[24] = AFN0A025;
    AFN0AFun[25] = AFN0A026;
    AFN0AFun[26] = AFN0A027;
    AFN0AFun[27] = AFN0A028;
   
    AFN0AFun[28] = AFN0A029;
    AFN0AFun[29] = AFN0A030;
    AFN0AFun[30] = AFN0A031;
    
    //��5
    AFN0AFun[32] = AFN0A033;
    
    AFN0AFun[33] = AFN0A034;
    AFN0AFun[34] = AFN0A035;
    AFN0AFun[35] = AFN0A036;
    AFN0AFun[36] = AFN0A037;
    AFN0AFun[37] = AFN0A038;
    AFN0AFun[38] = AFN0A039;
    
    //��6
    AFN0AFun[40] = AFN0A041;
    AFN0AFun[41] = AFN0A042;
    AFN0AFun[42] = AFN0A043;
    AFN0AFun[43] = AFN0A044;
    AFN0AFun[44] = AFN0A045;
    AFN0AFun[45] = AFN0A046;
    AFN0AFun[46] = AFN0A047;
    AFN0AFun[47] = AFN0A048;
    
    //��7
    AFN0AFun[48] = AFN0A049;
    
   #ifdef LIGHTING
    AFN0AFun[49] = AFN0A050;
    AFN0AFun[50] = AFN0A051;
    AFN0AFun[51] = AFN0A052;
   #endif
    
    //��8
    AFN0AFun[56] = AFN0A057;
    AFN0AFun[57] = AFN0A058;
    AFN0AFun[58] = AFN0A059;
    AFN0AFun[59] = AFN0A060;
    AFN0AFun[60] = AFN0A061;
    
    //��9
    AFN0AFun[64] = AFN0A065;
    AFN0AFun[65] = AFN0A066;
    AFN0AFun[66] = AFN0A067;
    AFN0AFun[67] = AFN0A068;
    
    //��10
    AFN0AFun[72] = AFN0A073;
    AFN0AFun[73] = AFN0A074;
    AFN0AFun[74] = AFN0A075;
    AFN0AFun[75] = AFN0A076;
 
    //��11
    AFN0AFun[80] = AFN0A081;
    AFN0AFun[81] = AFN0A082;
    AFN0AFun[82] = AFN0A083;

    //��չ
   #ifdef SDDL_CSM
    AFN0AFun[87] = AFN0A088;
   #endif
    AFN0AFun[96] = AFN0A097;
    AFN0AFun[97] = AFN0A098;
    
    AFN0AFun[98] = AFN0A099;
    AFN0AFun[99] = AFN0A100;
   
    AFN0AFun[120] = AFN0A121;
    
    AFN0AFun[132] = AFN0A133;
    AFN0AFun[133] = AFN0A134;
    AFN0AFun[134] = AFN0A135;
    AFN0AFun[135] = AFN0A136;
    
    AFN0AFun[137] = AFN0A138;
    
    if (fQueue.tailPtr == 0)
    {
       tmpHead0a = 0;
    }
    else
    {
       tmpHead0a = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    frameTail0a = tmpHead0a+14;    
    
    frameCounter = 0;
    tmpDt1 = 0;
    tmpDtCount = 0;
   
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    maxCycle = 0;
    while ((frame.loadLen>0) && (maxCycle<500))
    {
       maxCycle++;
       
       offset0a = 0;
       tmpDt1 = *(pDataHead+2);
       tmpDtCount = 0;
       while(tmpDtCount<9)
       {    
           tmpDtCount++;
           if ((tmpDt1 & 0x1) == 0x1)
           {
           	 fn = *(pDataHead+3)*8 + tmpDtCount;
           	#ifdef SDDL_CSM
           	 if (fn==224)
           	 {
               frameTail0a = AFN0A224(frameTail0a,pDataHead);
           	 }
           	 else
           	 {
           	#endif

           	   //2013-11-28�������ж�
           	   if(fn>138)
           	   {
           	   	 maxCycle = 500;
      	         break;
           	   }
           	   	
           	   //ִ�к���
               if ((AFN0AFun[fn-1] != NULL) 
              	  && (fn<=83 
              	     #ifdef SDDL_CSM
              	      || 88==fn
              	     #endif
              	       || fn == 92 || fn == 97 || fn == 98 || fn == 99 || fn==100 || fn==121 || fn==133 || fn==134 || fn==135 || fn==136 || fn==138))
               {
                 if ((tmpFrameTail = AFN0AFun[fn-1](frameTail0a,pDataHead))== frameTail0a)
                 {
                 	 ackData[ackTail*5]   = *pDataHead;                         //DA1
                 	 ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
                 	 ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
                 	 ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
                 	 ackData[ackTail*5+4] = 0x01;                               //����Ч����
                 	 ackTail++;
                 }
                 else
                 {
                 	 frameTail0a = tmpFrameTail;
                 }
               }
            #ifdef SDDL_CSM
             }
            #endif
           }
           
           tmpDt1 >>= 1;
           
           if (((frameTail0a - tmpHead0a) > MAX_OF_PER_FRAME) || (((pDataHead+offset0a+4) == pDataEnd) && tmpDtCount==8))
           {
               //�����������ϱ������¼�����
               if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail0a - tmpHead0a) > 16)
               {
              	   msFrame[frameTail0a++] = iEventCounter;
              	   msFrame[frameTail0a++] = nEventCounter;
               }

               //��������վҪ���ж��Ƿ�Я��TP
               if (frame.pTp != NULL)
               {
                  pTpv = frame.pTp;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv;
               }
               
               msFrame[tmpHead0a + 0] = 0x68;   //֡��ʼ�ַ�
             
               tmpI = ((frameTail0a - tmpHead0a -6) << 2) | PROTOCOL_FIELD;
               msFrame[tmpHead0a + 1] = tmpI & 0xFF;   //L
               msFrame[tmpHead0a + 2] = tmpI >> 8;
               msFrame[tmpHead0a + 3] = tmpI & 0xFF;   //L
               msFrame[tmpHead0a + 4] = tmpI >> 8; 
             
               msFrame[tmpHead0a + 5] = 0x68;  //֡��ʼ�ַ�
        
        			 //������
               if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
               {
                  msFrame[tmpHead0a + 6] = 0xa8;  //�����ֽ�10001000
               }
               else
               {
               	  msFrame[tmpHead0a + 6] = 0x88;  //�����ֽ�10001000
               }
               	 
               //��ַ��
               msFrame[tmpHead0a + 7] = addrField.a1[0];
               msFrame[tmpHead0a + 8] = addrField.a1[1];
               msFrame[tmpHead0a + 9] = addrField.a2[0];
               msFrame[tmpHead0a + 10] = addrField.a2[1];
               msFrame[tmpHead0a + 11] = addrField.a3;
       
               msFrame[tmpHead0a + 12] = 0x0A;  //AFN
        
               if ((pDataHead+4) == pDataEnd)
               {
                  if (frameCounter == 0)
                  {
                    msFrame[tmpHead0a + 13] = 0x60 | rSeq;    //01100000 | rSeq ��֡
                  }
                  else
                  {
                    msFrame[tmpHead0a + 13] = 0x20 | rSeq;    //00100000 | rSeq ���һ֡
                  }
               }
               else
               {
                 if (frameCounter == 0)
                 {
                   msFrame[tmpHead0a + 13] = 0x40 | rSeq;     //01000000 | rSeq  ��һ֡
                 }
                 else
                 {
                   msFrame[tmpHead0a + 13] = 0x00 | rSeq;     //00000000 | rSeq �м�֡
                 }
                 frameCounter++;
               }

               if (frame.pTp != NULL)
               {
              	  msFrame[tmpHead0a+13] |= 0x80;       //TpV��λ
               }
                            
               tmpI = tmpHead0a + 6;
               checkSum = 0;
               while (tmpI < frameTail0a)
               {
                 checkSum = msFrame[tmpI] + checkSum;
                 tmpI++;
               }
               
               msFrame[frameTail0a++] = checkSum;
               msFrame[frameTail0a++] = 0x16;
               
               fQueue.frame[fQueue.tailPtr].head = tmpHead0a;
               fQueue.frame[fQueue.tailPtr].len = frameTail0a-tmpHead0a;
               
               if (((frameTail0a - tmpHead0a) > 16 && frame.pTp==NULL)||((frameTail0a - tmpHead0a) > 22 && frame.pTp!=NULL))
               {
                 tmpHead0a = frameTail0a;
                 if ((tmpHead0a+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                 	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                 {
                    fQueue.frame[fQueue.tailPtr].next = 0x0;
                 	  fQueue.tailPtr = 0;
                 	  tmpHead0a = 0;
                 }
                 else
                 {                 
                    fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                    fQueue.tailPtr++;
                 }

                 frameTail0a = tmpHead0a + 14;  //frameTail������Ϊ14��д��һ֡
               }
           }
       }
       
       if (frame.loadLen<(offset0a+4))
       {
       	 break;
       }
       else
       {
         frame.loadLen -= (offset0a + 4);
         pDataHead += offset0a + 4;
       }
   }   
   
   if (ackTail !=0)
   {     	           
     AFN00003(ackTail, dataFrom, 0x0a);
   }
}

/*******************************************************
��������:AFN0A001
��������:��Ӧ��վ��ѯ��������"�ն�����ͨ�ſ�ͨ�Ų���(F1)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A001(INT16U frameTail,INT8U *pHandle)
{  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle;    //DT2
    
  //���ݵ�Ԫ
  msFrame[frameTail++] = commPara.rts;
  msFrame[frameTail++] = commPara.delay;
  msFrame[frameTail++] = commPara.timeOutReSendTimes[0];
  msFrame[frameTail++] = commPara.timeOutReSendTimes[1];
  msFrame[frameTail++] = commPara.flagOfCon;
  msFrame[frameTail++] = commPara.heartBeat;
  
  return frameTail;
}

/*******************************************************
��������:AFN0A002
��������:��Ӧ��վ��ѯ��������"�ն�����ͨ�ſ������м�ת������(F2)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A002(INT16U frameTail,INT8U *pHandle)
{
	  INT8U  num;
	  
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  msFrame[frameTail++] = relayConfig.relayAddrNumFlg;
	  
	  for (num = 0; num < (relayConfig.relayAddrNumFlg & 0x7F); num++)
	  {
	      msFrame[frameTail++] = relayConfig.relayAddr[num][0];
	      msFrame[frameTail++] = relayConfig.relayAddr[num][1];
	  }
	  
	  return frameTail;
}

/*******************************************************
��������:AFN0A003
��������:��Ӧ��վ��ѯ��������"��վIP��ַ�Ͷ˿�(F3)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A003(INT16U frameTail,INT8U *pHandle)
{  
  INT8U i;
  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle;    //DT2
    
  //���ݵ�Ԫ
  //����IP��ַ�Ͷ˿�
  for (i=0;i<4;i++)
  {
    msFrame[frameTail++] = ipAndPort.ipAddr[i];
  }
  msFrame[frameTail++] = ipAndPort.port[0];
  msFrame[frameTail++] = ipAndPort.port[1];
  
  //����IP��ַ�Ͷ˿�
  for (i=0;i<4;i++)
  {
    msFrame[frameTail++] = ipAndPort.ipAddrBak[i];
  }
  msFrame[frameTail++] = ipAndPort.portBak[0];
  msFrame[frameTail++] = ipAndPort.portBak[1];
  
  //APN
  for (i=0;i<16;i++)
  {
    msFrame[frameTail++] = ipAndPort.apn[i];
  }

  return frameTail;
}

/*******************************************************
��������:AFN0A004
��������:��Ӧ��վ��ѯ"��վ�绰����Ͷ������ĺ���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A004(INT16U frameTail,INT8U *pHandle)
{
    INT8U i;
    
    //���ݵ�Ԫ��ʶ
	  msFrame[frameTail++] = *pHandle++;     //DA1
	  msFrame[frameTail++] = *pHandle++;     //DA2
	  msFrame[frameTail++] = *pHandle++;     //DT1
	  msFrame[frameTail++] = *pHandle;       //DT2
	
	  //��վ�绰����
	  for(i=0;i<8;i++)
	    msFrame[frameTail++] = phoneAndSmsNumber.phoneNumber[i];
	
	  //��վ�������ĺ���
	  for(i=0;i<8;i++)
	    msFrame[frameTail++] = phoneAndSmsNumber.smsNumber[i];
	
    return frameTail;
}

/*******************************************************
��������:AFN0A005
��������:��Ӧ��վ��ѯ"�ն�����ͨ����Ϣ��֤��������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A005(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  //��Ϣ��֤������
	  msFrame[frameTail++] = messageAuth[0];
    
    //��Ϣ��֤��������
    msFrame[frameTail++] = messageAuth[1];
    msFrame[frameTail++] = messageAuth[2];
    
    return frameTail;
}

/*******************************************************
��������:AFN0A006
��������:��Ӧ��վ��ѯ��������"�ն����ַ����(F6)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A006(INT16U frameTail,INT8U *pHandle)
{   
  INT8U i;
  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2
    
  //���ݵ�Ԫ 
  for(i=0;i<16;i++)
  {
    msFrame[frameTail++] = groupAddr[i];
  }
   
  return frameTail;
}

/*******************************************************
��������:AFN0A007
��������:��Ӧ��վ��ѯ"�ն�IP��ַ�Ͷ˿�"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A007(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	
	//���ݵ�Ԫ��ʶ
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2
	
	//�ն�IP��ַ
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.teIpAddr[i];
	}
	
	//���������ַ
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.mask[i];
	}
	
	//���ص�ַ
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.gateWay[i];
	}
	
	//��������
	msFrame[frameTail++] = teIpAndPort.proxyType;
	
	//�����������ַ
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.proxyServer[i];
	}
	
	//����������˿�
	msFrame[frameTail++] = teIpAndPort.proxyPort[0];
	msFrame[frameTail++] = teIpAndPort.proxyPort[1];
	
	//������������ӷ�ʽ
	msFrame[frameTail++] = teIpAndPort.proxyLinkType;
	
	//�û�������
	msFrame[frameTail++] = teIpAndPort.userNameLen;
	
	//�û���
	for(i=0;i<teIpAndPort.userNameLen;i++)
	{
		msFrame[frameTail++] = teIpAndPort.userName[i];
	}
	
	//���볤��
	msFrame[frameTail++] = teIpAndPort.passwordLen;
	
	//����
	for(i=0;i<teIpAndPort.passwordLen;i++)
	{
		msFrame[frameTail++] = teIpAndPort.password[i];
	}
	
	//�����˿�
	msFrame[frameTail++] = teIpAndPort.listenPort[0];
	msFrame[frameTail++] = teIpAndPort.listenPort[1];
	
  return frameTail;
}

/*******************************************************
��������:AFN0A008
��������:��Ӧ��վ��ѯ"�ն�����ͨ�Ź�����ʽ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A008(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	 
	//���ݵ�Ԫ��ʶ
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2
	
	//����ģʽ	
	msFrame[frameTail++] = tePrivateNetMethod.workMethod;
	 
	//�������ߣ�ʱ������ģʽ�ز����
	msFrame[frameTail++] = tePrivateNetMethod.redialInterval[0];
	msFrame[frameTail++] = tePrivateNetMethod.redialInterval[1];
	 
	//��������ģʽ�ز�����
	msFrame[frameTail++] = tePrivateNetMethod.maxRedial;
	 
	//��������ģʽ������ͨ���Զ�����ʱ��
	msFrame[frameTail++] = tePrivateNetMethod.closeConnection;
	 
	//ʱ������ģʽ��������ʱ�α�־
	for(i=0;i<3;i++)
	{
	  msFrame[frameTail++] = tePrivateNetMethod.onLinePeriodTime[i];
	}
	 
  return frameTail;
}

/*******************************************************
��������:AFN0A009
��������:��Ӧ��վ��ѯ"�ն��¼���¼����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A009(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	
	//���ݵ�Ԫ��ʶ
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2
	
	//���ݵ�Ԫ
	//�¼���¼��Ч��־λ
	for(i=0;i<8;i++)
	{
		msFrame[frameTail++] = eventRecordConfig.nEvent[i];
	}
	
	//�¼���Ҫ�Եȼ���־λ
	for(i=0;i<8;i++)
	{
		msFrame[frameTail++] = eventRecordConfig.iEvent[i];
	}
   
  return frameTail;
}

/*******************************************************
��������:AFN0A010
��������:��Ӧ��վ��ѯ"�ն˵��ܱ�/��������װ�����ò���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A010(INT16U frameTail,INT8U *pHandle)
{
  METER_DEVICE_CONFIG meterDeviceConfig;

  INT16U i,j;
  
  INT16U num1, num2;
  INT16U tmpFrameTail;
  
  INT8U result;
  
  tmpFrameTail = frameTail;
  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle++;    //DT2
 
  num1 = *pHandle++;
  num1 |= (*pHandle++) << 8;
  msFrame[frameTail++] = num1;    			 //��ѯ������
  msFrame[frameTail++] = num1 >> 8;
  offset0a += 2;
 
  if(num1 > 200)
  {
 	  return tmpFrameTail;
  }

  //���ݵ�Ԫ	
  for(i=0;i<num1;i++)
  {
  	bzero(&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG));
  	   
  	num2 = *pHandle++;
  	num2 |= (*pHandle++)<<8;	   
  	offset0a += 2;
  	   
  	//2012-09-21,��³���ǳǷ���,���������ܶ������ַ,������վ�ٲ�ȫ��0
  	//           ԭ��Ϊ������һ���������Ϊ0�Ĳ�������Ϣ,�޸��������
  	//result = selectF10Data(0, 0, num2, (INT8U *)&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG));
  	result = selectF10Data(0xfffe, 0, num2, (INT8U *)&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG));

  	if(result == 1)
	  {
	   	msFrame[frameTail++] = meterDeviceConfig.number & 0xFF;							//���
	    msFrame[frameTail++] = meterDeviceConfig.number>>8;
	    msFrame[frameTail++] = meterDeviceConfig.measurePoint & 0xFF;				//�������
	    msFrame[frameTail++] = meterDeviceConfig.measurePoint>>8;
	    msFrame[frameTail++] = meterDeviceConfig.rateAndPort;								//ͨ�����ʼ��˿ں�
	    msFrame[frameTail++] = meterDeviceConfig.protocol;										//Э������
		
	    //ͨ�ŵ�ַ
	    for(j=0;j<6;j++)
	    {
	 		  msFrame[frameTail++] = meterDeviceConfig.addr[j];
	    }
	 
	    //ͨ������
	    for(j=0;j<6;j++)
	    {
	 		  msFrame[frameTail++] = meterDeviceConfig.password[j];
	    }
	 
	    msFrame[frameTail++] = meterDeviceConfig.numOfTariff;								//���ܷ��ʸ���
	    msFrame[frameTail++] = meterDeviceConfig.mixed;											//�й�����ʾֵ����λ��С��λ
	 
	    //�����ɼ���ͨ�ŵ�ַ
	    for(j=0;j<6;j++)
	    {
	 		  msFrame[frameTail++] = meterDeviceConfig.collectorAddr[j];
	    }
	 
	    msFrame[frameTail++] = meterDeviceConfig.bigAndLittleType;						//�û����༰�û�С���
    }
    else
    {
    	for(j=0;j<27;j++)
    	{
    	 	msFrame[frameTail++] = 0xEE;
    	}
    }
  }

  return frameTail;
}

/*******************************************************
��������:AFN0A011
��������:��Ӧ��վ��ѯ"�ն��������ò���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A011(INT16U frameTail,INT8U *pHandle)
{
  INT8U  num;
  INT16U count = 0;
  INT8U  i;
  
  msFrame[frameTail++] = *pHandle++;		//DA1
  msFrame[frameTail++] = *pHandle++;		//DA2
  msFrame[frameTail++] = *pHandle++;		//DT1
  msFrame[frameTail++] = *pHandle++;		//DT2
  
  count = frameTail++;	  
  
  num = *pHandle++;							//���β�ѯ����
  
  offset0a += 1;
  msFrame[count] = 0;           //���β�ѯ��������
  for(i=0;i<num;i++)
  {
  	if(*pHandle <= pulseConfig.numOfPulse && *pHandle > 0)
  	{
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].ifNo;							//��������˿ں�
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].pn;								//��������˿ں�
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].character;				//��������˿ں�
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].meterConstant[0];	//��������˿ں�
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].meterConstant[1];	//��������˿ں�
  		
  		msFrame[count]++;
  	}
  	
  	//���Ҫ��ѯ����Ŵ����Ѵ��ڵ�����������
  	pHandle++;	  	
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
  {
	  frameTail -= 5;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A012
��������:��Ӧ��վ��ѯ"�ն�״̬���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A012(INT16U frameTail,INT8U *pHandle)
{
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	  
	msFrame[frameTail++] = statusInput[0];	//״̬�������־λ
	msFrame[frameTail++] = statusInput[1];	//״̬�����Ա�־λ
	  
	return frameTail;
}

/*******************************************************
��������:AFN0A013
��������:��Ӧ��վ��ѯ"�ն˵�ѹ/����ģ�����ò���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A013(INT16U frameTail,INT8U *pHandle)
{
  INT8U  num;
  INT16U count = 0;
  INT8U  i;
  
  msFrame[frameTail++] = *pHandle++;		//DA1
  msFrame[frameTail++] = *pHandle++;		//DA2
  msFrame[frameTail++] = *pHandle++;		//DT1
  msFrame[frameTail++] = *pHandle++;		//DT2
  
  count = frameTail++;
  msFrame[count] = 0;
  num = *pHandle++;
  offset0a += 1;
  
  for (i=0; i<num; i++)
  {
  	if(*pHandle <= simuIUConfig.numOfSimu && *pHandle > 0)
  	{
  		msFrame[frameTail++] = simuIUConfig.perIUConfig[*pHandle - 1].ifNo;			//��ѹ/����ģ��������˿ں�
     	msFrame[frameTail++] = simuIUConfig.perIUConfig[*pHandle - 1].pn;				//�����������
     	msFrame[frameTail++] = simuIUConfig.perIUConfig[*pHandle - 1].character;//��ѹ����ģ��������
     	pHandle++;
     	msFrame[count]++;
  	}
  	else
  	{
  		//���Ҫ��ѯ����Ŵ����Ѵ��ڵ�����������
  		pHandle++;
  	}
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
  {
	  frameTail -= 5;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A014
��������:��Ӧ��վ��ѯ"�ն��ܼ������ò���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A014(INT16U frameTail,INT8U *pHandle)
{
	INT8U  num1, num2;
	INT8U  i, j;
	INT16U count = 0;
	  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle++;    //DT2

  count = frameTail++;
  msFrame[count] = 0;
  num1 = *pHandle++;
  offset0a += 1;
   
  for(i=0;i<num1;i++)
  {
  	if(*pHandle <= totalAddGroup.numberOfzjz && *pHandle > 0)
  	{
  		msFrame[frameTail++] = totalAddGroup.perZjz[*pHandle - 1].zjzNo;			  //�ܼ������
  		msFrame[frameTail++] = totalAddGroup.perZjz[*pHandle - 1].pointNumber;  //���ܼ������������
  		num2 = totalAddGroup.perZjz[*pHandle - 1].pointNumber;
  		
  		//������ż��ܼӱ�־
  		for(j=0;j<num2;j++)
  		{
  			msFrame[frameTail++] = totalAddGroup.perZjz[*pHandle - 1].measurePoint[j];
  		}
  		
  		msFrame[count] ++;
  		pHandle++;
  	}
  	else
  	{
  		//���Ҫ��ѯ����Ŵ����Ѵ��ڵ�����������
	  	pHandle++;
  	} 
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
	{
		frameTail -= 5;
	}
  	
  return frameTail;
}

/*******************************************************
��������:AFN0A015
��������:��Ӧ��վ��ѯ"�й��ܵ������Խ���¼���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A015(INT16U frameTail,INT8U *pHandle)
{
	INT8U  num;
	INT16U count = 0;
	INT8U  i, j;
	  
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	  
	count = frameTail++;
	msFrame[count] = 0;
	num = *pHandle++;
	offset0a += 1;
	  
  for (i = 0; i < num; i++)
  {
  	if(*pHandle <= differenceConfig.numOfConfig && *pHandle > 0)
  	{
  		msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].groupNum;						//�й��ܵ�����������
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].toCompare;					//�Աȵ��ܼ������
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].toReference;				//���յ��ܼ������
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].timeAndFlag;			 	//�����ĵ�������ʱ�����估�Աȷ�����־ 
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].ralitaveDifference;	//�Խ�����ƫ��ֵ
			
			//�Խ�޾���ƫ��ֵ
			for(j=0;j<4;j++)
			{
				msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].absoluteDifference[j];
			}
			
			msFrame[count]++;
			pHandle++;    	
  	}
  	else
  	{
  		//���Ҫ��ѯ����Ŵ����Ѵ��ڵ�����������
	  	pHandle++;
  	}
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
	{
		frameTail -= 5;
	}
    
  return frameTail;
}

/*******************************************************
��������:AFN0A016
��������:��Ӧ��վ��ѯ"����ר���û���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A016(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;
  
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  
  //����ר���û���
  for(i=0;i<32;i++)
  {
    msFrame[frameTail++] = vpn.vpnName[i];
  }

	//����ר������
  for(i=0;i<32;i++)
  {
    msFrame[frameTail++] = vpn.vpnPassword[i];
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A017
��������:��Ӧ��վ��ѯ"�ն˱�����ֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A017(INT16U frameTail,INT8U *pHandle)
{
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	  
	msFrame[frameTail++] = protectLimit[0];
	msFrame[frameTail++] = protectLimit[1];
	  
	return frameTail;
}

/*******************************************************
��������:AFN0A018
��������:��Ӧ��վ��ѯ"�ն˹���ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A018(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	  
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	  
	for (i = 0; i < 12; i++)
	{
	  msFrame[frameTail++] = ctrlPara.pCtrlPeriod[i];
	}
	  
	return frameTail;
}

/*******************************************************
��������:AFN0A019
��������:��Ӧ��վ��ѯ"�ն�ʱ�ι��ض�ֵ����ϵ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A019(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  msFrame[frameTail++] = ctrlPara.pCtrlIndex;
	  
	  return frameTail;
}

/*******************************************************
��������:AFN0A020
��������:��Ӧ��վ��ѯ"�ն��µ������ض�ֵ����ϵ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A020(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
    msFrame[frameTail++] = ctrlPara.monthEnergCtrlIndex;
	  
	  return frameTail;
}

/*******************************************************
��������:AFN0A021
��������:��Ӧ��վ��ѯ"�ն˵���������ʱ�κͷ�����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A021(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;

  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2

  //���ݵ�Ԫ	
  for(i=0;i<49;i++)
  {
    msFrame[frameTail++] = periodTimeOfCharge[i];
  }
    		
  return frameTail;
}

/*******************************************************
��������:AFN0A022
��������:��Ӧ��վ��ѯ"�ն˵���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A022(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;

  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2

  //���ݵ�Ԫ	
  msFrame[frameTail++] = chargeRateNum.chargeNum;	//������
  
  //����
  for(i=0;i<chargeRateNum.chargeNum;i++)
  {
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][0];
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][1];
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][2];
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][3];
  }
    		
  return frameTail;
}

/*******************************************************
��������:AFN0A023
��������:��Ӧ��վ��ѯ"�߷Ѹ澯����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A023(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  //�߷Ѹ澯�����ֹ��־λ
	  msFrame[frameTail++] = chargeAlarm[0];
	  msFrame[frameTail++] = chargeAlarm[1];
	  msFrame[frameTail++] = chargeAlarm[2];
	  
	  return frameTail;
}

/*******************************************************
��������:AFN0A025
��������:��Ӧ��վ��ѯ"�������������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A025(INT16U frameTail,INT8U *pHandle)
{
	MEASURE_POINT_PARA pointPara; 
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 25, pn, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
			{
				//��ѹ����������
				msFrame[frameTail++] = pointPara.voltageTimes & 0xFF;
				msFrame[frameTail++] = pointPara.voltageTimes >> 8;
		
				//��������������
				msFrame[frameTail++] = pointPara.currentTimes & 0xFF;
				msFrame[frameTail++] = pointPara.currentTimes >> 8;
		
				//���ѹ
				msFrame[frameTail++] = pointPara.ratingVoltage & 0xFF;
				msFrame[frameTail++] = pointPara.ratingVoltage >> 8;
		
				//�����
				msFrame[frameTail++] = pointPara.maxCurrent;
		
				//�����
				msFrame[frameTail++] = pointPara.powerRating[0];
				msFrame[frameTail++] = pointPara.powerRating[1];
				msFrame[frameTail++] = pointPara.powerRating[2];
		
				//��Դ���߷�ʽ
				msFrame[frameTail++] = pointPara.linkStyle;
			}
			else
			{
				for(i=0;i<11;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A026
��������:��Ӧ��վ��ѯ"��������ֵ����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A026(INT16U frameTail,INT8U *pHandle)
{
	MEASUREPOINT_LIMIT_PARA pointLimitPara; 
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 26, pn, (INT8U *)&pointLimitPara, sizeof(MEASUREPOINT_LIMIT_PARA)) == TRUE)
			{
				//��ѹ�ϸ��ʱ�����
				msFrame[frameTail++] = pointLimitPara.vUpLimit & 0xFF;				//��ѹ�ϸ�����
				msFrame[frameTail++] = pointLimitPara.vUpLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vLowLimit & 0xFF;				//��ѹ�ϸ�����
				msFrame[frameTail++] = pointLimitPara.vLowLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vPhaseDownLimit & 0xFF;	//��ѹ��������
				msFrame[frameTail++] = pointLimitPara.vPhaseDownLimit >> 8;
		
				//��ѹ������
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit & 0xFF;	//��ѹ������
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vUpUpTimes;							//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.vUpUpResume[0];					//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.vUpUpResume[1];
		
				//Ƿѹ������
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit & 0xFF;	//��ѹ������
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vDownDownTimes;			//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.vDownDownResume[0];	//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.vDownDownResume[1];
		
				//����������
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[0];	//�����������
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[1];
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[2];
				msFrame[frameTail++] = pointLimitPara.cUpUpTimes;					//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.cUpUpReume[0];			//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.cUpUpReume[1];
		
				//���������
				msFrame[frameTail++] = pointLimitPara.cUpLimit[0];				//���������
				msFrame[frameTail++] = pointLimitPara.cUpLimit[1];
				msFrame[frameTail++] = pointLimitPara.cUpLimit[2];
				msFrame[frameTail++] = pointLimitPara.cUpTimes;						//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.cUpResume[0];				//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.cUpResume[1];
		
				//����������ޱ�����
				msFrame[frameTail++] = pointLimitPara.cZeroSeqLimit[0];		//�����������
				msFrame[frameTail++] = pointLimitPara.cZeroSeqLimit[1];
				msFrame[frameTail++] = pointLimitPara.cZeroSeqLimit[2];
				msFrame[frameTail++] = pointLimitPara.cZeroSeqTimes;			//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.cZeroSeqResume[0];	//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.cZeroSeqResume[1];
		
				//���ڹ��ʳ������ޱ�����
				msFrame[frameTail++] = pointLimitPara.pSuperiodLimit[0];	//���ڹ���������
				msFrame[frameTail++] = pointLimitPara.pSuperiodLimit[1];
				msFrame[frameTail++] = pointLimitPara.pSuperiodLimit[2];
				msFrame[frameTail++] = pointLimitPara.pSuperiodTimes;			//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.pSuperiodResume[0];	//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.pSuperiodResume[1];
		
				//���ڹ��ʳ����ޱ�����
				msFrame[frameTail++] = pointLimitPara.pUpLimit[0];				//���ڹ�������
				msFrame[frameTail++] = pointLimitPara.pUpLimit[1];
				msFrame[frameTail++] = pointLimitPara.pUpLimit[2];
				msFrame[frameTail++] = pointLimitPara.pUpTimes;						//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.pUpResume[0];				//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.pUpResume[1];
		
				//�����ѹ��ƽ�ⳬ�ޱ�����
				msFrame[frameTail++] = pointLimitPara.uPhaseUnbalance[0];	//�����ѹ��ƽ����ֵ
				msFrame[frameTail++] = pointLimitPara.uPhaseUnbalance[1];
				msFrame[frameTail++] = pointLimitPara.uPhaseUnTimes;			//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.uPhaseUnResume[0];	//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.uPhaseUnResume[1];
		
				//���������ƽ�ⳬ�ޱ�����
				msFrame[frameTail++] = pointLimitPara.cPhaseUnbalance[0];	//���������ƽ����ֵ
				msFrame[frameTail++] = pointLimitPara.cPhaseUnbalance[1];
				msFrame[frameTail++] = pointLimitPara.cPhaseUnTimes;			//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.cPhaseUnResume[0];	//Խ�޻ָ�ϵ��
				msFrame[frameTail++] = pointLimitPara.cPhaseUnResume[1];
		
				//����ʧѹʱ����ֵ
				msFrame[frameTail++] = pointLimitPara.uLostTimeLimit;
			}
			else
			{
				for(i=0;i<57;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			} 
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A027
��������:��Ӧ��վ��ѯ"������ͭ���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A027(INT16U frameTail,INT8U *pHandle)
{
	COPPER_IRON_LOSS copperIronLoss;  
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 27, pn, (INT8U *)&copperIronLoss, sizeof(COPPER_IRON_LOSS)) == TRUE)
			{
				//A�����
				msFrame[frameTail++] = copperIronLoss.aResistance[0];
				msFrame[frameTail++] = copperIronLoss.aResistance[1];
		
				//A��翹
				msFrame[frameTail++] = copperIronLoss.aReactance[0];
				msFrame[frameTail++] = copperIronLoss.aReactance[1];
		
				//A��絼
				msFrame[frameTail++] = copperIronLoss.aConductance[0];
				msFrame[frameTail++] = copperIronLoss.aConductance[1];
		
				//A�����
				msFrame[frameTail++] = copperIronLoss.aSusceptance[0];
				msFrame[frameTail++] = copperIronLoss.aSusceptance[1];
		
				//B�����
				msFrame[frameTail++] = copperIronLoss.bResistance[0];
				msFrame[frameTail++] = copperIronLoss.bResistance[1];
		
				//B��翹
				msFrame[frameTail++] = copperIronLoss.bReactance[0];
				msFrame[frameTail++] = copperIronLoss.bReactance[1];
		
				//B��絼
				msFrame[frameTail++] = copperIronLoss.bConductance[0];
				msFrame[frameTail++] = copperIronLoss.bConductance[1];
		
				//B�����
				msFrame[frameTail++] = copperIronLoss.bSusceptance[0];
				msFrame[frameTail++] = copperIronLoss.bSusceptance[1];
		
				//C�����
				msFrame[frameTail++] = copperIronLoss.cResistance[0];
				msFrame[frameTail++] = copperIronLoss.cResistance[1];
		
				//C��翹
				msFrame[frameTail++] = copperIronLoss.cReactance[0];
				msFrame[frameTail++] = copperIronLoss.cReactance[1];
		
				//C��絼
				msFrame[frameTail++] = copperIronLoss.cConductance[0];
				msFrame[frameTail++] = copperIronLoss.cConductance[1];
		
				//C�����
				msFrame[frameTail++] = copperIronLoss.cSusceptance[0];
				msFrame[frameTail++] = copperIronLoss.cSusceptance[1];
			}
			else
			{
				for(i=0;i<24;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A028
��������:��Ӧ��վ��ѯ"�����㹦�������ֶ���ֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A028(INT16U frameTail,INT8U *pHandle)
{
	POWER_SEG_LIMIT powerSegLimit;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2,dt1,dt2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
  dt1 = *pHandle++;
  dt2 = *pHandle++;
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 28, pn, (INT8U *)&powerSegLimit, sizeof(POWER_SEG_LIMIT)) == TRUE)
			{
	      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
	      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
        
        msFrame[frameTail++] = dt1;
        msFrame[frameTail++] = dt2;

				//���������ֶ���ֵ1
				msFrame[frameTail++] = powerSegLimit.segLimit1[0];
				msFrame[frameTail++] = powerSegLimit.segLimit1[1];
		
				//���������ֶ���ֵ2
				msFrame[frameTail++] = powerSegLimit.segLimit2[0];
				msFrame[frameTail++] = powerSegLimit.segLimit2[1];
			}
			else
			{
				;
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A029
��������:��Ӧ��վ��ѯ"�ն˵��ص��ܱ���ʾ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A029(INT16U frameTail,INT8U *pHandle)
{
	INT8U teShowNum[12];
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 29, pn, (INT8U *)teShowNum, 12) == TRUE)
			{
				for(i=0;i<12;i++)
				{
					msFrame[frameTail++] = teShowNum[i];		//�ն˵��ص��ܱ���ʾ��
				}
			}
			else
			{
				for(i=0;i<12;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A030
��������:��Ӧ��վ��ѯ"̨�����г���ͣ��Ͷ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A030(INT16U frameTail,INT8U *pHandle)
{
	INT8U copyStopAdminSet;
  INT8U da1,da2;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 30, pn, (INT8U *)&copyStopAdminSet, 1) == TRUE)
			{
				msFrame[frameTail++] = copyStopAdminSet;		//̨�����г���ͣ��Ͷ������
			}
			else
			{
				msFrame[frameTail++] = 0xee;
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A031
��������:��Ӧ��վ��ѯ"�ز��ӽڵ㸽���ڵ��ַ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A031(INT16U frameTail,INT8U *pHandle)
{
	AUXILIARY_ADDR auxiliaryAddr;
	
  INT8U da1,da2;
	
	INT16U i, pn = 0;
	INT16U tempFrameTail, tempPn = 0;
   
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn += (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 31, pn, (INT8U *)&auxiliaryAddr, sizeof(AUXILIARY_ADDR)) == TRUE)
			{
				//�ز��ӽڵ㸽���ڵ����
				msFrame[frameTail++] = auxiliaryAddr.numOfAuxiliaryNode;
		
				//�����ڵ��ַ
				for(i=0;i<auxiliaryAddr.numOfAuxiliaryNode*6;i++)
				{
					msFrame[frameTail++] = auxiliaryAddr.auxiliaryNode[i];
				}
			}
			else
			{
				return tempFrameTail;
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A033
��������:��Ӧ��վ��ѯ"�ն˳������в�������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A033(INT16U frameTail,INT8U *pHandle)
{
	INT8U count, num;
	
	INT16U i, j;
	 
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	count = *pHandle++;		              //��������
	msFrame[frameTail++] = count;
	offset0a += 1 + count;
	 
	for(i = 0; i < count; i++)
	{
   #ifdef SUPPORT_ETH_COPY
		if((*pHandle >0 && *pHandle < 6) || *pHandle == 31)
   #else
		if((*pHandle >0 && *pHandle < 5) || *pHandle == 31)
	 #endif
		{
			switch(*pHandle)
			{
				case 1:
					num = 0;
					break;
					
				case 2:
					num = 1;
					break;
					
				case 3:
					num = 2;
					break;

				case 4:    //2012-3-27,add
					num = 3;
					break;

				case 31:
					num = 4;
					break;
					
       #ifdef SUPPORT_ETH_COPY
				case 5:    //2014-4-17,add
					num = 5;
					break;
			 #endif
			}
			
			msFrame[frameTail++] = teCopyRunPara.para[num].commucationPort;			//�ն�ͨ�Ŷ˿ں�
			msFrame[frameTail++] = teCopyRunPara.para[num].copyRunControl[0];		//̨�����г������п�����
			msFrame[frameTail++] = teCopyRunPara.para[num].copyRunControl[1];
				
			//������-����
			for(j=0;j<4;j++)
			{
				msFrame[frameTail++] = teCopyRunPara.para[num].copyDay[j];
			}
				
			//������ʱ��
			msFrame[frameTail++] = teCopyRunPara.para[num].copyTime[0];
			msFrame[frameTail++] = teCopyRunPara.para[num].copyTime[1];
				
			//�����ռ��ʱ��
			msFrame[frameTail++] = teCopyRunPara.para[num].copyInterval;
				
			//�Ե��㲥Уʱ��ʱʱ��
			for(j=0;j<3;j++)
			{
				msFrame[frameTail++] = teCopyRunPara.para[num].broadcastCheckTime[j];
			}
				
			//������ʱ����
			msFrame[frameTail++] = teCopyRunPara.para[num].hourPeriodNum;
			
			//����ʱ�ο�ʼʱ�����ʱ��
			for(j=0;j<teCopyRunPara.para[num].hourPeriodNum*2;j++)
			{
				msFrame[frameTail++] = teCopyRunPara.para[num].hourPeriod[j][0];
				msFrame[frameTail++] = teCopyRunPara.para[num].hourPeriod[j][1];
			}
		}
		pHandle++;
	}
	
  return frameTail;
}

/*******************************************************
��������:AFN0A034
��������:��Ӧ��վ��ѯ"����������ͨ��ģ��Ĳ�������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A034(INT16U frameTail,INT8U *pHandle)
{
	INT16U count;
	INT8U  num;
	
	INT16U i, j;
	INT16U ly;
	 
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	 
	count = frameTail++;
	msFrame[count] = 0;
	num = *pHandle++;
	offset0a += (1+num);
	 
	for(i=0;i<4;i++)
	{
		//if(*pHandle <= downRiverModulePara.numOfPara)
		if(*pHandle==downRiverModulePara.para[i].commucationPort)		
		{
			msFrame[frameTail++] = downRiverModulePara.para[i].commucationPort;			//�ն�ͨ�Ŷ˿ں�
			msFrame[frameTail++] = downRiverModulePara.para[i].commCtrlWord;				//�ն˽ӿڶ˵�ͨ�ſ�����
			
			//�ն˽ӿڶ�Ӧ�˵�ͨ������
			for(j=0;j<4;j++)
			{
				msFrame[frameTail++] = downRiverModulePara.para[i].commRate[j];
			}
			
			pHandle++;
			msFrame[count]++;
		}
		else
		{
			//���Ҫ��ѯ����Ŵ����Ѵ��ڵ�����������
	  	pHandle++;
		}
	}
	
	if(msFrame[count] == 0)
	{
		frameTail -= 5;
	}
	
  return frameTail;
}

/*******************************************************
��������:AFN0A035
��������:��Ӧ��վ��ѯ"̨�����г����ص㻧����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A035(INT16U frameTail,INT8U *pHandle)
{
	INT16U i;
	 
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	msFrame[frameTail++] = keyHouseHold.numOfHousehold;		//̨�����г����ص㻧����
	
	if(keyHouseHold.numOfHousehold <= 0)
	{
		return frameTail -= 5;
	}
	
	//�ص㻧�ĵ��ܱ�/��������װ�����
	for(i=0;i<keyHouseHold.numOfHousehold*2;i++)
	{
		msFrame[frameTail++] = keyHouseHold.household[i];
	}

  return frameTail;
}

/*******************************************************
��������:AFN0A036
��������:��Ӧ��վ��ѯ"�ն�����ͨ��������������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A036(INT16U frameTail,INT8U *pHandle)
{
	INT16U i;
	
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	//��ͨ����������
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = upTranslateLimit[i];
	}

  return frameTail;
}

/*******************************************************
��������:AFN0A037
��������:��Ӧ��վ��ѯ"�ն˼���ͨ�Ų���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A037(INT16U frameTail,INT8U *pHandle)
{
	INT16U i;
	
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	msFrame[frameTail++] = cascadeCommPara.commPort;					//�ն˼���ͨ�Ŷ˿ں�
	msFrame[frameTail++] = cascadeCommPara.ctrlWord;					//�ն˼���ͨ�Ŷ˿ں�
	msFrame[frameTail++] = cascadeCommPara.receiveMsgTimeout;	//���յȴ����ĳ�ʱʱ��
	msFrame[frameTail++] = cascadeCommPara.receiveByteTimeout;//���յȴ��ֽڳ�ʱʱ��
	msFrame[frameTail++] = cascadeCommPara.cascadeMretryTime;	//����������ʧ���ط�����
	msFrame[frameTail++] = cascadeCommPara.groundSurveyPeriod;//����Ѳ������
	msFrame[frameTail++] = cascadeCommPara.flagAndTeNumber;		//�ն˱�׼���ն˸���
	
	for(i=0;i<(cascadeCommPara.flagAndTeNumber & 0x0F)*2;i+=2)
	{
		//���������ն�����������
		msFrame[frameTail++] = cascadeCommPara.divisionCode[i];
		msFrame[frameTail++] = cascadeCommPara.divisionCode[i + 1];
		
		//���������ն˵�ַ
		msFrame[frameTail++] = cascadeCommPara.cascadeTeAddr[i];
		msFrame[frameTail++] = cascadeCommPara.cascadeTeAddr[i + 1];
	}
	
  return frameTail;
}

/*******************************************************
��������:AFN0A038
��������:��Ӧ��վ��ѯ"1��������������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A038(INT16U frameTail,INT8U *pHandle)
{
	INT16U i, j;
	INT16U tempTail;  
	INT8U  num;
	INT8U  bType, lType;
	
	tempTail = frameTail;
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	//�û������
	bType = *pHandle++;
	offset0a++;

	printf("AFN0A-FN38�����%d\n", bType);

	if (bType<16)
	{
		//�û������
		msFrame[frameTail++] = bType;
		
		//��ѯ����
		msFrame[frameTail++] = num = *pHandle++;
		offset0a++;
		
	  printf("AFN0A-FN38��ѯ����%d\n", num);

		for(i=0;i<num && i<16;i++)
		{
			lType = *pHandle++;
			offset0a++;

			//�û�С���
			msFrame[frameTail++] = lType;
	    printf("AFN0A-FN38�û�С���%d\n", lType);

			//��Ϣ����������
			msFrame[frameTail++] = typeDataConfig1.bigType[bType].littleType[lType].infoGroup; 
	    
	    printf("AFN0A-FN38��Ϣ����������%d\n", typeDataConfig1.bigType[bType].littleType[lType].infoGroup);

			//��Ϣ��������Ӧ����Ϣ��Ԫ��־λ
			for(j=0;j<typeDataConfig1.bigType[bType].littleType[lType].infoGroup;j++)
			{
				msFrame[frameTail++] = typeDataConfig1.bigType[bType].littleType[lType].flag[j];
			}
		}
	}
	else
	{
		return tempTail;
	}

  return frameTail;
}

/*******************************************************
��������:AFN0A039
��������:��Ӧ��վ��ѯ"2��������������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A039(INT16U frameTail,INT8U *pHandle)
{
	INT16U i, j;
	INT16U tempTail;  
	INT8U  num;
	INT8U  bType, lType;
	
	tempTail = frameTail;
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	//�û������
	bType = *pHandle++;
	offset0a++;
	
	printf("AFN0A-FN39�����%d\n", bType);
	
	if (bType<16)
	{
		//�û������
		msFrame[frameTail++] = bType;
		
		//��ѯ����
		msFrame[frameTail++] = num = *pHandle++;
	  printf("AFN0A-FN39��ѯ����%d\n", num);

		offset0a++;
		
		for(i=0;i<num && i<16;i++)
		{
			lType = *pHandle++;
			offset0a++;
	    
	    printf("AFN0A-FN39�û�С���%d\n", lType);

			//�û�С���
			msFrame[frameTail++] = lType;

			//��Ϣ����������
			msFrame[frameTail++] = typeDataConfig2.bigType[bType].littleType[lType].infoGroup; 
	    
	    printf("AFN0A-FN39��Ϣ����������%d\n", typeDataConfig2.bigType[bType].littleType[lType].infoGroup);

			//��Ϣ��������Ӧ����Ϣ��Ԫ��־λ
			for(j=0;j<typeDataConfig2.bigType[bType].littleType[lType].infoGroup;j++)
			{
				msFrame[frameTail++] = typeDataConfig2.bigType[bType].littleType[lType].flag[j];
			}
		}
	}
	else
	{
		return tempTail;
	}

  return frameTail;
}

/*******************************************************
��������:AFN0A041
��������:��Ӧ��վ��ѯ"ʱ�ι��ض�ֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A041(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U i, j;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
 		  if(pn > 8)
 		  {
  			return tempFrameTail;
  	  }
  
      msFrame[frameTail++] = periodCtrlConfig[pn - 1].periodNum;	//������־
  
      for(i=0;i<3;i++)
      {
  	    //ʱ�κ�
  	    msFrame[frameTail++] = periodCtrlConfig[pn - 1].period[i].timeCode;
  	
  	    //ʱ�ι��ض�ֵ
  	    for(j=0;j<8;j++)
  	    {
  		    msFrame[frameTail++] = periodCtrlConfig[pn - 1].period[i].limitTime[j][0];
  		    msFrame[frameTail++] = periodCtrlConfig[pn - 1].period[i].limitTime[j][1];
  	    }
      }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}	

/*******************************************************
��������:AFN0A042
��������:��Ӧ��վ��ѯ"���ݹ��ز���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A042(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		//���ݿض�ֵ
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdLimit & 0xFF;
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdLimit>>8;
  
  		//�޵���ʼʱ��
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdStartMin;
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdStartHour;
  
  		//�޵�����ʱ��
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdTime;
  
  		//ÿ���޵���
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdDate;
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A043
��������:��Ӧ��վ��ѯ"���ʿ��ƵĹ��ʼ��㻬��ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A043(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{ 
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		msFrame[frameTail++] = powerCtrlCountTime[pn - 1].countTime;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A044
��������:��Ӧ��վ��ѯ"Ӫҵ��ͣ�ز���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A044(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
	    
  		//��ͣ��ʼʱ��
	    msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsStartDay%10)|(obsCtrlConfig[pn-1].obsStartDay/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsStartMonth%10)|(obsCtrlConfig[pn-1].obsStartMonth/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsStartYear%10)|(obsCtrlConfig[pn-1].obsStartYear/10<<4);
    	  
  		//��ͣ����ʱ��
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsEndDay%10)|(obsCtrlConfig[pn-1].obsEndDay/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsEndMonth%10)|(obsCtrlConfig[pn-1].obsEndMonth/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsEndYear%10)|(obsCtrlConfig[pn-1].obsEndYear/10<<4);
    	  
 		  //��ͣ�ع��ʶ�ֵ
 		  msFrame[frameTail++] = obsCtrlConfig[pn-1].obsLimit&0xFF;
    	msFrame[frameTail++] = obsCtrlConfig[pn-1].obsLimit>>8&0xFF;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A045
��������:��Ӧ��վ��ѯ"�����ִ��趨"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A045(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		//�����ִα�־λ
  		msFrame[frameTail++] = powerCtrlRoundFlag[pn - 1].flag;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A046
��������:��Ӧ��վ��ѯ"�µ����ض�ֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A046(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		//�µ����ض�ֵ
  		msFrame[frameTail++] = monthCtrlConfig[pn - 1].monthCtrl & 0xFF;
  		msFrame[frameTail++] = (monthCtrlConfig[pn - 1].monthCtrl >> 8) & 0xFF;
  		msFrame[frameTail++] = (monthCtrlConfig[pn - 1].monthCtrl >> 16) & 0xFF;
  		msFrame[frameTail++] = (monthCtrlConfig[pn - 1].monthCtrl >> 24) & 0xFF;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A047
��������:��Ӧ��վ��ѯ"�������ز���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A047(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
	
			//���絥��
  		msFrame[frameTail++] = chargeCtrlConfig[pn - 1].numOfBill & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].numOfBill >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].numOfBill >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].numOfBill >> 24) & 0xFF;
  
  		//׷��ˢ�±�־
  		msFrame[frameTail++] = chargeCtrlConfig[pn - 1].flag;
  
  		//������ֵ
			msFrame[frameTail++] = chargeCtrlConfig[pn - 1].chargeCtrl & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].chargeCtrl >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].chargeCtrl >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].chargeCtrl >> 24) & 0xFF;

			//��������ֵ
			msFrame[frameTail++] = chargeCtrlConfig[pn - 1].alarmLimit & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].alarmLimit >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].alarmLimit >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].alarmLimit >> 24) & 0xFF;
	
			//��բ����ֵ
			msFrame[frameTail++] = chargeCtrlConfig[pn - 1].cutDownLimit & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].cutDownLimit >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].cutDownLimit >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].cutDownLimit >> 24) & 0xFF;
  	}
  	da1 >>= 1;
  }	
  
	return frameTail;
}

/*******************************************************
��������:AFN0A048
��������:��Ӧ��վ��ѯ"����ִ��趨"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A048(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn=0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		msFrame[frameTail++] = electCtrlRoundFlag[pn - 1].flag;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A049
��������:��Ӧ��վ��ѯ"���ظ澯ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A049(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8)
  		{
  			return frameTail -= 4;
  		}
  
  		msFrame[frameTail++] = powerCtrlAlarmTime[pn - 1].alarmTime;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

#ifdef LIGHTING

/*******************************************************
��������:AFN0A050
��������:��Ӧ��վ��ѯ"����ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A050(INT16U frameTail,INT8U *pHandle)
{
	struct ctrlTimes *tmpCTimes;
	INT8U            i;
	INT16U           tmpTail;
	INT8U            tmpCount;
	INT8U            alreadySend;
  INT8U            tmpi;
	
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;	  //DA2
	msFrame[frameTail++] = *pHandle++;	  //DT1
	msFrame[frameTail++] = *pHandle++;	  //DT2
	tmpCount = *pHandle++;	              //���͵�ʱ���������ֵ,2016-12-16
	alreadySend = (tmpCount/20-1)*20;
	
	offset0a += 1;
	
	printf("tmpCount=%d,alreadySend=%d\n", tmpCount, alreadySend);
	
	tmpTail = frameTail++;    //����ʱ�θ���  
	msFrame[tmpTail] = 0;
	
	tmpi = 0;
	tmpCTimes = cTimesHead;

	while(tmpCTimes!=NULL)
  {
	  if (tmpi>=alreadySend)
		{
			msFrame[tmpTail]++;
			
			msFrame[frameTail++] = tmpCTimes->startMonth;
			msFrame[frameTail++] = tmpCTimes->startDay;
			msFrame[frameTail++] = tmpCTimes->endMonth;
			msFrame[frameTail++] = tmpCTimes->endDay;
			msFrame[frameTail++] = tmpCTimes->deviceType;
			msFrame[frameTail++] = tmpCTimes->noOfTime;
			msFrame[frameTail++] = tmpCTimes->workDay;

			for(i=0; i<6; i++)
			{
				msFrame[frameTail++] = tmpCTimes->hour[i];
				msFrame[frameTail++] = tmpCTimes->min[i];
				msFrame[frameTail++] = tmpCTimes->bright[i];
			}
			
			//ÿ�η���20������ʱ�β���
			if (msFrame[tmpTail]>=20)
			{
				break;
			}
		}
		tmpi++;
	  
	  tmpCTimes = tmpCTimes->next;
  }
  
  //2015-06-09,���,����ģʽ
  msFrame[frameTail++] = ctrlMode;
  
  //2015-06-25,���,�����ǰ-�ӳٶ��ٷ�����Ч
  memcpy(&msFrame[frameTail], beforeOnOff, 4);
  frameTail += 4;
	
  return frameTail;
}

/*******************************************************
��������:AFN0A051
��������:��Ӧ��վ��ѯ"���Ƶ���ֵ�趨"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A051(INT16U frameTail,INT8U *pHandle)
{
	//���ݵ�Ԫ��ʶ
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2	
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2	
	 
	//���ݵ�Ԫ	 
	msFrame[frameTail++] = pnGate.failureRetry;
	msFrame[frameTail++] = pnGate.boardcastWaitGate;
	msFrame[frameTail++] = pnGate.checkTimeGate;
	msFrame[frameTail++] = pnGate.lddOffGate;
	msFrame[frameTail++] = pnGate.lddtRetry;
	msFrame[frameTail++] = pnGate.offLineRetry;
	msFrame[frameTail++] = pnGate.lcWave;
	msFrame[frameTail++] = pnGate.leakCurrent;

  return frameTail;
}

/*******************************************************
��������:AFN0A052
��������:��Ӧ��վ��ѯ"���Ƶ���ֵ����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A052(INT16U frameTail,INT8U *pHandle)
{
	PN_LIMIT_PARA pointLimitPara; 
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����	
			if(selectViceParameter(0x04, 52, pn, (INT8U *)&pointLimitPara, sizeof(PN_LIMIT_PARA)) == TRUE)
			{
				//��ѹ��ѹ����
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit & 0xFF;
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit >> 8;
			  
				//��ѹǷѹ����
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit & 0xFF;
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit >> 8;
				
				//������������
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[0];
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[1];
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[2];

				//����Ƿ������
				msFrame[frameTail++] = pointLimitPara.cDownDownLimit[0];
				msFrame[frameTail++] = pointLimitPara.cDownDownLimit[1];
				msFrame[frameTail++] = pointLimitPara.cDownDownLimit[2];
				
				//��������
				msFrame[frameTail++] = pointLimitPara.pUpLimit[0];
				msFrame[frameTail++] = pointLimitPara.pUpLimit[1];
				msFrame[frameTail++] = pointLimitPara.pUpLimit[2];

				//��������
				msFrame[frameTail++] = pointLimitPara.pDownLimit[0];
				msFrame[frameTail++] = pointLimitPara.pDownLimit[1];
				msFrame[frameTail++] = pointLimitPara.pDownLimit[2];

				//������������
				msFrame[frameTail++] = pointLimitPara.factorDownLimit[0];
				msFrame[frameTail++] = pointLimitPara.factorDownLimit[1];

				//Խ�޳���ʱ��
				msFrame[frameTail++] = pointLimitPara.overContinued;
			}
			else
			{
				for(i=0; i<19; i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			} 
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

#endif

/*******************************************************
��������:AFN0A057
��������:��Ӧ��վ��ѯ"�ն������澯����/��ֹ����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A057(INT16U frameTail,INT8U *pHandle)
{
	  //���ݵ�Ԫ��ʶ
	  msFrame[frameTail++] = *pHandle++;    //DA1
	  msFrame[frameTail++] = *pHandle++;    //DA2
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2	
	 
	  //���ݵ�Ԫ
	  msFrame[frameTail++] = voiceAlarm[0];
	  msFrame[frameTail++] = voiceAlarm[1];
	  msFrame[frameTail++] = voiceAlarm[2];

    return frameTail;
}

/*******************************************************
��������:AFN0A058
��������:��Ӧ��վ��ѯ"�ն��Զ��������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A058(INT16U frameTail,INT8U *pHandle)
{
	  //���ݵ�Ԫ��ʶ
	  msFrame[frameTail++] = *pHandle++;    //DA1
	  msFrame[frameTail++] = *pHandle++;    //DA2
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2	
	 
	  //���ݵ�Ԫ	 
	  msFrame[frameTail++] = noCommunicationTime;
	  
	  return frameTail;
}

/*******************************************************
��������:AFN0A059
��������:��Ӧ��վ��ѯ"���ܱ��쳣�б���ֵ�趨"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A059(INT16U frameTail,INT8U *pHandle)
{
	  //���ݵ�Ԫ��ʶ
	  msFrame[frameTail++] = *pHandle++;    //DA1
	  msFrame[frameTail++] = *pHandle++;    //DA2	
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2	
	 
	  //���ݵ�Ԫ	 
	  msFrame[frameTail++] = meterGate.powerOverGate;
	  msFrame[frameTail++] = meterGate.meterFlyGate;
	  msFrame[frameTail++] = meterGate.meterStopGate;
	  msFrame[frameTail++] = meterGate.meterCheckTimeGate;

    return frameTail;
}

/*******************************************************
��������:AFN0A060
��������:��Ӧ��վ��ѯ��������"г����ֵ(F60)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A060(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;
  
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  
  //�ܻ����ѹ����������
  msFrame[frameTail++] = waveLimit.totalUPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.totalUPercentUpLimit[1];
  
  //���г����ѹ����������
  msFrame[frameTail++] = waveLimit.oddUPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.oddUPercentUpLimit[1];
  
  //ż��г����ѹ����������
  msFrame[frameTail++] = waveLimit.evenUPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.evenUPercentUpLimit[1];
  
  //г����ѹ����������
  for(i=0;i<18;i++)
  {
  	msFrame[frameTail++] = waveLimit.UPercentUpLimit[i][0];
  	msFrame[frameTail++] = waveLimit.UPercentUpLimit[i][1];
  }
  
  //�ܻ��������Чֵ����
  msFrame[frameTail++] = waveLimit.totalIPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.totalIPercentUpLimit[1];
  
  //г��������Чֵ����
  for(i=0;i<18;i++)
  {
  	msFrame[frameTail++] = waveLimit.IPercentUpLimit[i][0];
  	msFrame[frameTail++] = waveLimit.IPercentUpLimit[i][1];
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A061
��������:��Ӧ��վ��ѯ��������"ֱ��ģ�����������(F61)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A061(INT16U frameTail,INT8U *pHandle)
{
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  
  msFrame[frameTail++] = adcInFlag;
  
  return frameTail;
}

/*******************************************************
��������:AFN0A065
��������:��Ӧ��վ��ѯ��������"��ʱ����1��������������(F65)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A065(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//�����
	INT16U i;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendPeriod;		//��ʱ�ϱ����ڼ��ϱ����ڵ�λ
  	
  		//�ϱ���׼ʱ��
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.second / 10 << 4 | reportTask1.task[pn - 1].sendStart.second % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.minute / 10 << 4 | reportTask1.task[pn - 1].sendStart.minute % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.hour / 10 << 4 | reportTask1.task[pn - 1].sendStart.hour % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.day / 10 << 4 | reportTask1.task[pn - 1].sendStart.day % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.month / 10 << 4 | reportTask1.task[pn - 1].sendStart.month % 10 | reportTask1.task[pn - 1].week;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.year / 10 << 4 | reportTask1.task[pn - 1].sendStart.year % 10;
  	  
  	  msFrame[frameTail++] = reportTask1.task[pn - 1].sampleTimes;	//�������ݳ�ȡ����
  		msFrame[frameTail++] = reportTask1.task[pn - 1].numOfDataId;	//���ݵ�Ԫ��ȡ����
  	
  		for(i=0;i<reportTask1.task[pn - 1].numOfDataId;i++)
  		{
  			//DA
  			msFrame[frameTail++] = reportTask1.task[pn - 1].dataUnit[i].pn1;
  			msFrame[frameTail++] = reportTask1.task[pn - 1].dataUnit[i].pn2;
  			
  			//DT
  			msFrame[frameTail++] = 0x01<<((reportTask1.task[pn - 1].dataUnit[i].fn%8==0)?7:((reportTask1.task[pn - 1].dataUnit[i].fn-1)%8));
  			msFrame[frameTail++] = (reportTask1.task[pn - 1].dataUnit[i].fn - 1)/8;  		
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A066
��������:��Ӧ��վ��ѯ��������"��ʱ����2��������������(F66)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A066(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//�����
	INT16U i;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendPeriod;		//��ʱ�ϱ����ڼ��ϱ����ڵ�λ
  	
  		//�ϱ���׼ʱ��
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.second / 10 << 4 | reportTask2.task[pn - 1].sendStart.second % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.minute / 10 << 4 | reportTask2.task[pn - 1].sendStart.minute % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.hour / 10 << 4 | reportTask2.task[pn - 1].sendStart.hour % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.day / 10 << 4 | reportTask2.task[pn - 1].sendStart.day % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.month / 10 << 4 | reportTask2.task[pn - 1].sendStart.month % 10 | reportTask2.task[pn - 1].week;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.year / 10 << 4 | reportTask2.task[pn - 1].sendStart.year % 10;
  	  
  	  msFrame[frameTail++] = reportTask2.task[pn - 1].sampleTimes;	//�������ݳ�ȡ����
  		msFrame[frameTail++] = reportTask2.task[pn - 1].numOfDataId;	//���ݵ�Ԫ��ȡ����
  	
  		for(i=0;i<reportTask2.task[pn - 1].numOfDataId;i++)
  		{
  			//DA
  			msFrame[frameTail++] = reportTask2.task[pn - 1].dataUnit[i].pn1;
  			msFrame[frameTail++] = reportTask2.task[pn - 1].dataUnit[i].pn2;
  			
  			//DT
  			msFrame[frameTail++] = 0x01<<((reportTask2.task[pn - 1].dataUnit[i].fn%8==0)?7:((reportTask2.task[pn - 1].dataUnit[i].fn-1)%8));
  			msFrame[frameTail++] = (reportTask2.task[pn - 1].dataUnit[i].fn - 1)/8;  		
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A067
��������:��Ӧ��վ��ѯ��������"��ʱ����1��������������/ֹͣ����(F67)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A067(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//�����
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		msFrame[frameTail++] = reportTask1.task[pn - 1].stopFlag;		//������ֹͣ��־
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A068
��������:��Ӧ��վ��ѯ��������"��ʱ����2��������������/ֹͣ����(F68)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A068(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//�����
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		msFrame[frameTail++] = reportTask2.task[pn - 1].stopFlag;		//������ֹͣ��־
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A073
��������:��Ӧ��վ��ѯ��������"����������(F73)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A073(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_PARA capacityPara;
	
	INT16U pn = 0;			//�������
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
  		if(result == TRUE)
  		{
  			for(i=0;i<16;i++)
  			{
  				//������ʽ
  				msFrame[frameTail++] = capacityPara.capacity[i].compensationMode;
  		
  				//����װ������
  				msFrame[frameTail++] = capacityPara.capacity[i].capacityNum[0];
  				msFrame[frameTail++] = capacityPara.capacity[i].capacityNum[1];
  			}
  		}
  		else
  		{
  			//û������
  			for(i=0;i<48;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A074
��������:��Ӧ��վ��ѯ��������"������Ͷ������״̬(F74)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A074(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_RUN_PARA capacityRunPara;
	
	INT16U pn = 0;			//�������
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 74, pn, (INT8U *)&capacityRunPara, sizeof(CAPACITY_RUN_PARA));
  		if(result == TRUE)
  		{
  			//Ŀ�깦������
  			msFrame[frameTail++] = capacityRunPara.targetPowerFactor[0];
  			msFrame[frameTail++] = capacityRunPara.targetPowerFactor[1];
  	
  			//Ͷ���޹���������
  			msFrame[frameTail++] = capacityRunPara.onPowerLimit[0];
  			msFrame[frameTail++] = capacityRunPara.onPowerLimit[1];
  			msFrame[frameTail++] = capacityRunPara.onPowerLimit[2];
  	
  			//�г��޹���������
  			msFrame[frameTail++] = capacityRunPara.offPowerLimit[0];
  			msFrame[frameTail++] = capacityRunPara.offPowerLimit[1];
  			msFrame[frameTail++] = capacityRunPara.offPowerLimit[2];
  	
  			//��ʱʱ��
  			msFrame[frameTail++] = capacityRunPara.delay;
  	
  			//����ʱ����
  			msFrame[frameTail++] = capacityRunPara.actInterval;
  		}
  		else
  		{
  			//û������
  			for(i=0;i<10;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A075
��������:��Ӧ��վ��ѯ��������"��������������(F75)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A075(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_PROTECT_PARA capacityProtectPara;
	
	INT16U pn = 0;			//�������
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 75, pn, (INT8U *)&capacityProtectPara, sizeof(CAPACITY_PROTECT_PARA));
  		if(result == TRUE)
 		  {
  			//����ѹ
  			msFrame[frameTail++] = capacityProtectPara.vSuperiod[0];
  			msFrame[frameTail++] = capacityProtectPara.vSuperiod[1];
  	
  			//����ѹ�ز�ֵ
  			msFrame[frameTail++] = capacityProtectPara.vSuperiodQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.vSuperiodQuan[1];
  	
  			//Ƿ��ѹ
  			msFrame[frameTail++] = capacityProtectPara.vLack[0];
  			msFrame[frameTail++] = capacityProtectPara.vLack[1];
  	
  			//Ƿ��ѹ�ز�ֵ
  			msFrame[frameTail++] = capacityProtectPara.vLackQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.vLackQuan[1];
  	
  			//�ܻ����������������
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalUpLimit[0];
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalUpLimit[1];
  	
  			//�ܻ������������Խ�޻ز�ֵ
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalQuan[1];
  	
  			//�ܻ����ѹ����������
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalUpLimit[0];
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalUpLimit[1];
  	
  			//�ܻ����ѹ������Խ�޻ز�ֵ
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalQuan[1];
  		}
  		else
  		{
  			//û������
  			for(i=0;i<16;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}	
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A076
��������:��Ӧ��վ��ѯ��������"������Ͷ�п��Ʋ���(F76)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A076(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_PARA capacityPara;
	
	INT16U pn = 0;			//�������
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
  		if(result == TRUE)
  		{
  			msFrame[frameTail++] = capacityPara.controlMode;
  		}
  		else
  		{
  			//û������
  			msFrame[frameTail++] = 0xEE;
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A081
��������:��Ӧ��վ��ѯ��������"ֱ��ģ�������(F81)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A081(INT16U frameTail,INT8U *pHandle)
{
	ADC_PARA adcPara;
	
	INT16U pn = 0;			//�������
	INT16U tempPn = 0;
	INT16U i;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 81, pn, (INT8U *)&adcPara, sizeof(ADC_PARA));
  		if(result == TRUE)
  		{
  			//ֱ��ģ����������ʼֵ
  			msFrame[frameTail++] = adcPara.adcStartValue[0];
  			msFrame[frameTail++] = adcPara.adcStartValue[1];
  	
  			//ֱ��ģ����������ֵֹ
  			msFrame[frameTail++] = adcPara.adcEndValue[0];
  			msFrame[frameTail++] = adcPara.adcEndValue[1];
  		}
  		else
  		{
  			//û������
  			for(i=0;i<4;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
 		  }
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
��������:AFN0A082
��������:��Ӧ��վ��ѯ��������"ֱ��ģ������ֵ(F82)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A082(INT16U frameTail,INT8U *pHandle)
{
	ADC_PARA adcPara;
	
	INT16U pn = 0;			//�������
	INT16U tempPn = 0;
	INT16U i;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 81, pn, (INT8U *)&adcPara, sizeof(ADC_PARA));
  		if(result == TRUE)
  		{
  			//ֱ��ģ��������
  			msFrame[frameTail++] = adcPara.adcUpLimit[0];
  			msFrame[frameTail++] = adcPara.adcUpLimit[1];
  	
  			//ֱ��ģ��������
  			msFrame[frameTail++] = adcPara.adcLowLimit[0];
  			msFrame[frameTail++] = adcPara.adcLowLimit[1];
  		}
  		else
  		{
  			//û������
  			for(i=0;i<4;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}
  	}
  	da1 >>= 1;
  }
  
 	return frameTail;
}

/*******************************************************
��������:AFN0A083
��������:��Ӧ��վ��ѯ��������"ֱ��ģ�����������(F81)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
INT16U AFN0A083(INT16U frameTail,INT8U *pHandle)
{
	ADC_PARA adcPara;
	
	INT16U pn = 0;			//�������
	INT16U tempPn = 0;
	INT16U i;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//��ѯ����
  		result = selectViceParameter(0x04, 81, pn, (INT8U *)&adcPara, sizeof(ADC_PARA));
  		if(result == TRUE)
  		{
  			//ֱ��ģ��������
  			msFrame[frameTail++] = adcPara.adcFreezeDensity;
  		}
  		else
  		{
  			//û������
  			msFrame[frameTail++] = 0xEE;
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

#ifdef SDDL_CSM

/*******************************************************
��������:AFN0A088
��������:��Ӧ��վ��ѯ"��������ܱ��ʲ����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A088(INT16U frameTail,INT8U *pHandle)
{
	INT16U           i, pn = 0;
	INT16U           tempPn = 0;
	
  INT8U da1,da2;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
			if(selectViceParameter(0x04, 88, pn, &msFrame[frameTail], 15) == FALSE)
			{
				memset(&msFrame[frameTail], 0x0,15);
			}

			frameTail+=15;
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

#endif

/*******************************************************
��������:AFN0A097
��������:��Ӧ��վ��ѯ��������"�����ն�����(F97)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A097(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i;
	
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x01;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    
  //���ݵ�Ԫ
  for(i = 0; i < 20; i++)
  {
  	msFrame[frameTail++] = teName[i];
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A098
��������:��Ӧ��վ��ѯ��������"����ϵͳ���б�ʶ��(F98)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A098(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i;
	
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x02;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    
  //���ݵ�Ԫ
  for(i = 0; i < 20; i++)
  {
  	msFrame[frameTail++] = sysRunId[i];
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A099
��������:��Ӧ��վ��ѯ��������"�ն˳�����������ʱ��(F99)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A099(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i;
	
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x04;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    

  //���ݵ�Ԫ
  for(i = 0; i < 6; i++)
  {
  	 msFrame[frameTail++] = assignCopyTime[i];
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A100
��������:��Ӧ��վ��ѯ��������"�ն�Ԥ��APN(F100)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A100(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i,j;
	
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x08;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    
  //���ݵ�Ԫ
  for(i = 0; i < 4; i++)
  {
  	for(j=0;j<16;j++)
    {
  	  msFrame[frameTail++] = teApn[i][j];
  	}
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0A121
��������:��Ӧ��վ��ѯ"�ն˵�ַ������������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A121(INT16U frameTail,INT8U *pHandle)
{
	  #ifdef TE_ADDR_USE_BCD_CODE
	   INT16U tmpAddr;
	  #endif
	  
	  //���ݵ�Ԫ��ʶ
	  msFrame[frameTail++] = *pHandle++;    //DA1
  	msFrame[frameTail++] = *pHandle++;    //DA2
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2
	
	  //���ݵ�Ԫ
	  //����������
	  #ifdef TE_ADDR_USE_BCD_CODE
	   tmpAddr = (addrField.a2[1]>>4)*1000 + (addrField.a2[1]&0xf)*100
	           + (addrField.a2[0]>>4)*10 + (addrField.a2[0]&0xf);
	   msFrame[frameTail++] = tmpAddr&0xff;
	   msFrame[frameTail++] = tmpAddr>>8;
	  #else
	   msFrame[frameTail++] = addrField.a2[0];
	   msFrame[frameTail++] = addrField.a2[1];
	  #endif

	  //�ն˵�ַ
	  msFrame[frameTail++] = addrField.a1[0];
	  msFrame[frameTail++] = addrField.a1[1];
	  
	  return frameTail;
}

/*******************************************************
��������:AFN0A133
��������:��Ӧ��վ��ѯ"�ز�/�������ڵ��ַ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A133(INT16U frameTail,INT8U *pHandle)
{
	 //���ݵ�Ԫ��ʶ
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   memcpy(&msFrame[frameTail],  mainNodeAddr, 6);
   
   frameTail += 6;
	 
	 return frameTail;
}

/*******************************************************
��������:AFN0A134
��������:��Ӧ��վ��ѯ"�豸���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A134(INT16U frameTail,INT8U *pHandle)
{
	 //���ݵ�Ԫ��ʶ
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //�豸���
   msFrame[frameTail++] = deviceNumber&0xff;
   msFrame[frameTail++] = deviceNumber>>8&0xff;
   
	 return frameTail;
}

/*******************************************************
��������:AFN0A135
��������:��Ӧ��վ��ѯ"���ģ�����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A135(INT16U frameTail,INT8U *pHandle)
{
	 //���ݵ�Ԫ��ʶ
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //����
   msFrame[frameTail++] = rlPara[0];   //��������
   msFrame[frameTail++] = rlPara[1];   //�����
   msFrame[frameTail++] = rlPara[2];   //�ź�ǿ��
   msFrame[frameTail++] = rlPara[3];   //�ŵ�

	 return frameTail;
}

/*******************************************************
��������:AFN0A136
��������:��Ӧ��վ��ѯ"���ó�����Ϣ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A136(INT16U frameTail,INT8U *pHandle)
{
	 //���ݵ�Ԫ��ʶ
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //������Ϣ
   memcpy(&msFrame[frameTail], csNameId, 12);
   
   frameTail += 12;

	 return frameTail;
}

/*******************************************************
��������:AFN0A138
��������:��Ӧ��վ��ѯ"�����û�����������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A138(INT16U frameTail,INT8U *pHandle)
{
	 //���ݵ�Ԫ��ʶ
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //����
   msFrame[frameTail++] = denizenDataType;

	 return frameTail;
}


#ifdef SDDL_CSM

/*******************************************************
��������:AFN0A224
��������:��Ӧ��վ��ѯ"�ն˵�ַ��-ɽ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0A224(INT16U frameTail, INT8U *pHandle)
{
  INT8U supplyWay;
  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2

  //���ݵ�Ԫ
  //��������
  msFrame[frameTail++] = addrField.a1[0];
  msFrame[frameTail++] = addrField.a1[1];

  //�ն˵�ַ
  msFrame[frameTail++] = addrField.a2[0];
  msFrame[frameTail++] = addrField.a2[1];

  //���緽ʽ
  selectParameter(0x04, 224, &supplyWay, 1);

  msFrame[frameTail++] = supplyWay;
  
  return frameTail;
}

#endif
