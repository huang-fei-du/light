/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN0C.c
���ߣ�wan guihua
�汾��0.1
������ڣ�2010��1��
��������վAFN0C(����һ������)�����ļ���
�����б�
     1.
�޸���ʷ��
  01,10-1-24,Leiyong created.
  02,11-05-13,Leiyong,��Ӽ�����ͬʱ�㳭���������Ĺ���
***************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMeter.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "dataBalance.h"
#include "convert.h"
#include "meterProtocol.h"
#include "att7022b.h"
#include "userInterface.h"
#include "gdw376-2.h"

#include "AFN0C.h"

INT8U  ackTail;

#ifdef PULSE_GATHER
 extern ONE_PULSE pulse[NUM_OF_SWITCH_PULSE];      //�������ɼ�
#endif

INT8U afn0cDataFrom;

/*******************************************************
��������:AFN0C
��������:��վ"����һ������"(AFN0C)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN0C(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll)
{
    INT16U   tmpI,tmpFrameTail;
    INT8U    fn;
    INT8U    tmpDtCount;              //DT��λ����
    INT8U    tmpDt1;                  //��ʱDT1
    INT8U    *pTpv;                   //TpVָ��
    INT8U    maxCycle;                //���ѭ������
    INT16U   frameTail0c;             //AFN0C֡β
    INT16U   tmpHead0c;               //AFN0C��ʱ֡ͷ
    INT16U   tmpHead0cActive;         //�����ϱ�AFN0C��ʱ֡ͷ

    INT16U (*AFN0CFun[248])(INT16U frameTail,INT8U *pHandlem, INT8U fn);
    for (tmpI=0; tmpI<248; tmpI++)
    {
       AFN0CFun[tmpI] = NULL;
    }
       
    //��1
    AFN0CFun[0] = AFN0C001;
    
    afn0cDataFrom = dataFrom;
    
    AFN0CFun[1] = AFN0C002;
    AFN0CFun[2] = AFN0C003;
    AFN0CFun[3] = AFN0C004;
    AFN0CFun[4] = AFN0C005;
    AFN0CFun[5] = AFN0C006;
    AFN0CFun[6] = AFN0C007;
    AFN0CFun[7] = AFN0C008;
    
    //��2
    AFN0CFun[8] = AFN0C009;
    AFN0CFun[9] = AFN0C010;
   
    AFN0CFun[10] = AFN0C011;
   
    AFN0CFun[13] = AFN0C014;   //�����Լ+13��Լ��Ҫ

    AFN0CFun[14] = AFN0C015;   //�����Լ
    AFN0CFun[15] = AFN0C016;   //�����Լ
    
    //��3
    AFN0CFun[16] = AFN0C017;
    AFN0CFun[17] = AFN0C018;
    AFN0CFun[18] = AFN0C019;
    AFN0CFun[19] = AFN0C020; 
    AFN0CFun[20] = AFN0C021;
    AFN0CFun[21] = AFN0C022; 
    AFN0CFun[22] = AFN0C023;
    AFN0CFun[23] = AFN0C024;
    
    //��4
    AFN0CFun[24] = AFN0C025;
    AFN0CFun[25] = AFN0C026;
    AFN0CFun[26] = AFN0C027;
    
    AFN0CFun[27] = AFN0C028;    
    AFN0CFun[28] = AFN0C029;
    AFN0CFun[29] = AFN0C030;
    AFN0CFun[30] = AFN0C031;
    AFN0CFun[31] = AFN0C032;
    
    //��5
    AFN0CFun[32] = AFN0C033;
    AFN0CFun[33] = AFN0C034;
    AFN0CFun[34] = AFN0C035;
    AFN0CFun[35] = AFN0C036;
    AFN0CFun[36] = AFN0C037;
    AFN0CFun[37] = AFN0C038;
    AFN0CFun[38] = AFN0C039;
    AFN0CFun[39] = AFN0C040;
    
    //��6
    AFN0CFun[40] = AFN0C041;
    AFN0CFun[41] = AFN0C042;
    AFN0CFun[42] = AFN0C043;
    AFN0CFun[43] = AFN0C044;
    AFN0CFun[44] = AFN0C045;
    AFN0CFun[45] = AFN0C046;
    AFN0CFun[46] = AFN0C047;
    AFN0CFun[47] = AFN0C048;
    
    //��7
    AFN0CFun[48] = AFN0C049;
    
    //��8
    AFN0CFun[56] = AFN0C057;
    AFN0CFun[57] = AFN0C058;
    
    //��9
    AFN0CFun[64] = AFN0C065;
    AFN0CFun[65] = AFN0C066;
    AFN0CFun[66] = AFN0C067;
    
    //��10
    AFN0CFun[72] = AFN0C073;
    
    //��11
    AFN0CFun[80] = AFN0C081;
    AFN0CFun[81] = AFN0C082;
    AFN0CFun[82] = AFN0C083;
    AFN0CFun[83] = AFN0C084;
    
    //��12
    AFN0CFun[88] = AFN0C089;
    AFN0CFun[89] = AFN0C090;
    AFN0CFun[90] = AFN0C091;
    AFN0CFun[91] = AFN0C092;
    AFN0CFun[92] = AFN0C093;
    AFN0CFun[93] = AFN0C094;
    AFN0CFun[94] = AFN0C095;
    AFN0CFun[95] = AFN0C096;
    
    //��13
    AFN0CFun[96] = AFN0C097;
    AFN0CFun[97] = AFN0C098;
    AFN0CFun[98] = AFN0C099;
    AFN0CFun[99] = AFN0C100;
    AFN0CFun[100] = AFN0C101;
    AFN0CFun[101] = AFN0C102;
    AFN0CFun[102] = AFN0C103;
    
    //��14
    AFN0CFun[104] = AFN0C105;
    AFN0CFun[105] = AFN0C106;
    AFN0CFun[106] = AFN0C107;
    AFN0CFun[107] = AFN0C108;
    AFN0CFun[108] = AFN0C109;
    AFN0CFun[109] = AFN0C110;
    AFN0CFun[110] = AFN0C111;
    AFN0CFun[111] = AFN0C112;
    
    //��15
    AFN0CFun[112] = AFN0C113;
    AFN0CFun[113] = AFN0C114;
    AFN0CFun[114] = AFN0C115;
    AFN0CFun[115] = AFN0C116;
    
    //��16
    AFN0CFun[120] = AFN0C121;
    
    //��17
    AFN0CFun[128] = AFN0C129;
    AFN0CFun[129] = AFN0C130;
    AFN0CFun[130] = AFN0C131;
    AFN0CFun[131] = AFN0C132;
    AFN0CFun[132] = AFN0C133;
    AFN0CFun[133] = AFN0C134;
    AFN0CFun[134] = AFN0C135;
    AFN0CFun[135] = AFN0C136;
    
    //��18
    AFN0CFun[136] = AFN0C137;
    AFN0CFun[137] = AFN0C138;
    AFN0CFun[138] = AFN0C139;
    AFN0CFun[139] = AFN0C140;
    AFN0CFun[140] = AFN0C141;
    AFN0CFun[141] = AFN0C142;
    AFN0CFun[142] = AFN0C143;
    AFN0CFun[143] = AFN0C144;
    
    //��19
    AFN0CFun[144] = AFN0C145;
    AFN0CFun[145] = AFN0C146;
    AFN0CFun[146] = AFN0C147;
    AFN0CFun[147] = AFN0C148;
    AFN0CFun[148] = AFN0C149;
    AFN0CFun[149] = AFN0C150;
    AFN0CFun[150] = AFN0C151;
    AFN0CFun[151] = AFN0C152;
    
    //��20
    AFN0CFun[152] = AFN0C153;
    AFN0CFun[153] = AFN0C154;
    AFN0CFun[154] = AFN0C155;
    AFN0CFun[155] = AFN0C156;
    AFN0CFun[156] = AFN0C157;
    AFN0CFun[157] = AFN0C158;
    AFN0CFun[158] = AFN0C159;
    AFN0CFun[159] = AFN0C160;
    
    //��21
    AFN0CFun[160] = AFN0C161;
    
    AFN0CFun[164] = AFN0C165;
    AFN0CFun[165] = AFN0C166;
    AFN0CFun[166] = AFN0C167;
    AFN0CFun[167] = AFN0C168;
    
    //��22
    AFN0CFun[168] = AFN0C169;
    AFN0CFun[169] = AFN0C170;
    
    if (fQueue.tailPtr == 0)
    {
       tmpHead0c = 0;
    }
    else
    {
       tmpHead0c = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }

    if (poll == AFN0B_REQUIRE)
    {
       frameTail0c = tmpHead0c + 18;
    }
    else
    {
       frameTail0c = tmpHead0c + 14;
    }
    
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    tmpDt1 = 0;
    tmpDtCount = 0;
    ackTail = 0;
    maxCycle = 0;
    
    if (poll==ACTIVE_REPORT)
    {
    	 frame.loadLen = pDataEnd - pDataHead;
    }

    //ly,2011-05-15
    dotReplyStart = 0;

    while ((frame.loadLen > 0) && (maxCycle<1500))
    {
        maxCycle++;
        
        tmpDt1 = *(pDataHead + 2);
        tmpDtCount = 0;
        while(tmpDtCount < 9)
        {
           tmpDtCount++;
           if ((tmpDt1 & 0x1) == 0x1)
           {
           	  fn = *(pDataHead + 3) * 8 + tmpDtCount;
           	  
           	  //printf("AFN0C fn=%d\n",fn);
           	  
           	  if (fn<=170)
           	  {
           	    //ִ�к���
           	    //2013-11-28��fn<=170�Ƶ��������ж�,��ֹ����Խ��
                //if (AFN0CFun[fn-1] != NULL && fn <= 170)
                if (AFN0CFun[fn-1] != NULL)
                {
                   tmpFrameTail = AFN0CFun[fn-1](frameTail0c, pDataHead, fn);
                   if (tmpFrameTail== frameTail0c)
                   {
                 	   if (fn==129)
                 	   {
                       ;
                 	   }
                 	   else
                 	   {
                 	     ackData[ackTail*5]   = *pDataHead;                             //DA1
                 	     ackData[ackTail*5+1] = *(pDataHead+1);                       //DA2
                 	     ackData[ackTail*5+2] = 0x1 << ((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
                 	     ackData[ackTail*5+3] = (fn-1)/8;                             //DT2
                 	     ackData[ackTail*5+4] = 0x01;                                 //����Ч����
                 	     ackTail++;
                 	   }
                   }
                   else
                   {
                 	    frameTail0c = tmpFrameTail;
                   }
                 }
              }
           }
           
           tmpDt1 >>= 1;
                      
           if ((frameTail0c - tmpHead0c) > MAX_OF_PER_FRAME || (((pDataHead+4) == pDataEnd) && tmpDtCount==8))
           {
              //�����������ϱ������¼�����
              if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail0c - tmpHead0c) > 16)
              {
              	  msFrame[frameTail0c++] = iEventCounter;
              	  msFrame[frameTail0c++] = nEventCounter;
              }
              
              //��������վҪ���ж��Ƿ�Я��TP
              //ly,2011-10-11,�޸���������ϱ�Tp��bug(ifΪtrue�Ĵ���)
              if (frame.pTp != NULL)  //ly,2011-10-24,add this if
              {
                if (poll==ACTIVE_REPORT)
                {
                  msFrame[frameTail0c++] = pfc++;
                  msFrame[frameTail0c++] = hexToBcd(sysTime.second);
                  msFrame[frameTail0c++] = hexToBcd(sysTime.minute);
                  msFrame[frameTail0c++] = hexToBcd(sysTime.hour);
                  msFrame[frameTail0c++] = hexToBcd(sysTime.day);
                  msFrame[frameTail0c++] = 0x0;
                }
                else
                {
                   pTpv = frame.pTp;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv;
                }
              }
              
              msFrame[tmpHead0c + 0] = 0x68;   //֡��ʼ�ַ�
            
              //if (poll==ACTIVE_REPORT)
              //{
              //  tmpI = ((frameTail0c - tmpHead0c - 6) << 2) | 0x01;
              //}
              //else
              //{
                tmpI = ((frameTail0c - tmpHead0c - 6) << 2) | PROTOCOL_FIELD;
              //}
              msFrame[tmpHead0c + 1] = tmpI & 0xFF;   //L
              msFrame[tmpHead0c + 2] = tmpI >> 8;
              msFrame[tmpHead0c + 3] = tmpI & 0xFF;   //L
              msFrame[tmpHead0c + 4] = tmpI >> 8; 
            
              msFrame[tmpHead0c + 5] = 0x68;  //֡��ʼ�ַ�

       
              if (poll == ACTIVE_REPORT)
              {
              	msFrame[tmpHead0c + 6] = 0xc4;    //DIR=1,PRM=1,������=0x4
              }
              else
              {
                msFrame[tmpHead0c + 6] = 0x88;     //�����ֽ�10001000(DIR=1,PRM=0,������=0x8)
              }

              if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
              {
                  msFrame[tmpHead0c + 6] |= 0x20;
              }
       
              //��ַ
              msFrame[tmpHead0c + 7] = addrField.a1[0];
              msFrame[tmpHead0c + 8] = addrField.a1[1];
              msFrame[tmpHead0c + 9] = addrField.a2[0];
              msFrame[tmpHead0c + 10] = addrField.a2[1];
              if (poll == ACTIVE_REPORT)
                msFrame[tmpHead0c + 11] = 0;
              else
              	msFrame[tmpHead0c + 11] = addrField.a3;
              
              if (poll == AFN0B_REQUIRE)
              {
                msFrame[tmpHead0c + 12] = 0x0B;  //AFN
              }
              else
              {
                msFrame[tmpHead0c + 12] = 0x0C;  //AFN
              }
       
              msFrame[tmpHead0c+13] = 0;
              
              if (frame.pTp != NULL)
              {
              	 msFrame[tmpHead0c+13] |= 0x80;       //TpV��λ
              }
              
              if (poll==AFN0B_REQUIRE)
              {
              	  msFrame[tmpHead0c+14] = *(frame.pData+2);
              	  msFrame[tmpHead0c+15] = *(frame.pData+3);
              	  msFrame[tmpHead0c+16] = *(frame.pData+4);
              	  msFrame[tmpHead0c+17] = *(frame.pData+5);              	  
              }
              
              //frameTail0c++;
              msFrame[frameTail0c+1] = 0x16;
              
              fQueue.frame[fQueue.tailPtr].head = tmpHead0c;
              fQueue.frame[fQueue.tailPtr].len = frameTail0c + 2 - tmpHead0c;
              
              if (((poll != AFN0B_REQUIRE)&&(((frameTail0c - tmpHead0c) > 16 && frame.pTp==NULL) || ((frameTail0c - tmpHead0c) > 22 && frame.pTp!=NULL)))
              	||  ((poll == AFN0B_REQUIRE) && (frameTail0c - tmpHead0c > 20)))
              {  
                 if (poll==ACTIVE_REPORT)
                 {
                 	  //����õ�֡���Ƶ������ϱ�֡������
                    if (fQueue.activeTailPtr == 0)
                    {
                       tmpHead0cActive = 0;
                    }
                    else
                    {
                       tmpHead0cActive = fQueue.activeFrame[fQueue.activeTailPtr-1].head + fQueue.activeFrame[fQueue.activeTailPtr-1].len;
                    }
                    
                    fQueue.activeFrame[fQueue.activeTailPtr].head = tmpHead0cActive;
                    fQueue.activeFrame[fQueue.activeTailPtr].len = fQueue.frame[fQueue.tailPtr].len;
                    for(tmpI=0;tmpI<fQueue.activeFrame[fQueue.activeTailPtr].len;tmpI++)
                    {
                    	 activeFrame[tmpHead0cActive+tmpI] = msFrame[fQueue.frame[fQueue.tailPtr].head+tmpI];
                    }
                    
                    if ((tmpHead0cActive+tmpI+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
                    tmpHead0c = frameTail0c+2;
                    if ((tmpHead0c+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                    {
                       fQueue.frame[fQueue.tailPtr].next = 0x0;
                    	 fQueue.tailPtr = 0;
                    	 tmpHead0c = 0;
                    }
                    else
                    {                 
                       fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                       fQueue.tailPtr++;
                    }
                 }

                 if (poll == AFN0B_REQUIRE)
                 {
                    frameTail0c = tmpHead0c + 18;  //frameTail������λ��д��һ֡
                 }
                 else
                 {
                    frameTail0c = tmpHead0c + 14;  //frameTail������λ��д��һ֡
                 }
              }
           }
        }
        
        pDataHead += 4;
        if (frame.loadLen<4)
        {
        	break;
        }
        else
        {
          frame.loadLen -= 4;
        }
    }
    
    if (ackTail !=0)
    {
       AFN00003(ackTail, dataFrom, 0x0c);
    }
}

/*******************************************************
��������:numOfTariff
��������:
���ú���:
�����ú���:
�������:
�������:
����ֵ��������ķ��ʸ���
*******************************************************/
INT8U numOfTariff(INT16U pn)
{
	 METER_DEVICE_CONFIG meterDeviceConfig;
	
 	 if(selectF10Data(pn, 0, 0, (INT8U *)&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG)) == TRUE)
	 {
		 return meterDeviceConfig.numOfTariff;
 	 }
	 else
	 {
		 return 0;
	 }	  
}

/*******************************************************
��������:AFN0C001
��������:��Ӧ��վ����һ������"�ն˰汾��Ϣ/376.1�ֵ���չ����ͨ��ģ���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C001(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
 #ifdef PLUG_IN_CARRIER_MODULE
  INT16U tmpData, i;
  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;   //DA1
  msFrame[frameTail++] = *pHandle++;   //DA2
  msFrame[frameTail++] = 0x01;         //DT1
  msFrame[frameTail++] = 0x00;         //DT2
  
  //����ͨ��ģ������
  msFrame[frameTail++]   = carrierModuleType;

  tmpData = 0;
  if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
  {
    tmpFound = noFoundMeterHead;
  }
  else
  {
    tmpFound = foundMeterHead;
  }
  while(tmpFound!=NULL)
  {
  	tmpData++;
  	tmpFound = tmpFound->next;
  }

  //����/δ�����ڵ�����
  msFrame[frameTail++] = tmpData&0xff;
  msFrame[frameTail++] = tmpData>>8&0xff;

  if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
  {
    tmpFound = noFoundMeterHead;
  }
  else
  {
    tmpFound = foundMeterHead;
  }
  
  while(tmpFound!=NULL)
  {
  	memcpy(&msFrame[frameTail], tmpFound->addr, 6);
  	frameTail += 6;
  	
  	tmpFound = tmpFound->next;
  }
  
  //������ʽ
  msFrame[frameTail++] = localCopyForm;
  
  //������...
  msFrame[frameTail++] = upRtFlag;
  
  //·�ɰ汾
  msFrame[frameTail++] = carrierFlagSet.productInfo[7];
  msFrame[frameTail++] = carrierFlagSet.productInfo[8];

  //·�ɰ汾����
  msFrame[frameTail++] = carrierFlagSet.productInfo[4];
  msFrame[frameTail++] = carrierFlagSet.productInfo[5];
  msFrame[frameTail++] = carrierFlagSet.productInfo[6];
  
 #endif
 
  return frameTail;
}

/*******************************************************
��������:AFN0C002
��������:��Ӧ��վ����һ������"�ն�����ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C002(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    INT8U weekNumber;
    
	  weekNumber = dayWeek(2000+sysTime.year,sysTime.month,sysTime.day);
	  if (weekNumber == 0)
	  	weekNumber = 7;
    
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;  			//DA1
    msFrame[frameTail++] = 0x00;    		//DA2
    msFrame[frameTail++] = 0x02;        //DT1
    msFrame[frameTail++] = 0x00;        //DT2
    
    //���ݵ�Ԫ
	  msFrame[frameTail++] = sysTime.second/10<<4 | sysTime.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
	  msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;	     //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  	
	  if (sysTime.month<10)
	  {
	     msFrame[frameTail++] = weekNumber<<5 | sysTime.month;           //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	  }
	  else
	  {
	  	 msFrame[frameTail++] = weekNumber<<5 | 0x10 | sysTime.month%10; //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	  }
	  msFrame[frameTail++] = sysTime.year/10<<4 | sysTime.year%10;       //��(ʮλ+��λ)
	
    return frameTail;
}

/*******************************************************
��������:AFN0C003
��������:��Ӧ��վ����һ������"�ն˲���״̬"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C003(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    INT8U i;
    
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;    			//DA2
    msFrame[frameTail++] = 0x04;          //DT1
    msFrame[frameTail++] = 0x00;          //DT2
    
    //���ݵ�Ԫ
    for(i=0;i<31;i++)
      msFrame[frameTail++] = paraStatus[i];

    return frameTail;
}

/*******************************************************
��������:AFN0C004
��������:��Ӧ��վ����һ������"�ն�ͨ��״̬"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C004(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;   			  //DA1
    msFrame[frameTail++] = 0x00;    			//DA2
    msFrame[frameTail++] = 0x08;          //DT1
    msFrame[frameTail++] = 0x00;          //DT2
    
    //���ݵ�Ԫ
	  msFrame[frameTail++] = callAndReport;
	  
	  printf("callAndReport=%02x\n",callAndReport);
	
    return frameTail;
}

/*******************************************************
��������:AFN0C005
��������:��Ӧ��վ����һ������"�ն˿�������״̬"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C005(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   #ifdef LOAD_CTRL_MODULE
    INT8U  i,j;
    INT8U  tmpData;
    INT16U tmpTail;
    
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;   				//DA2
    msFrame[frameTail++] = 0x10;          //DT1
    msFrame[frameTail++] = 0;             //DT2

    //���ݵ�Ԫ
    
    //���硢�޳��ʹ߷Ѹ澯Ͷ��״̬
    tmpData = 0x0;
    if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
    {
    	 tmpData |= 0x1; 
    }
    if (toEliminate==CTRL_JUMP_IN)
    {
    	 tmpData |= 0x2;
    }
    if (reminderFee==CTRL_JUMP_IN)
    {
    	 tmpData |= 0x4;
    }    
    msFrame[frameTail++] = tmpData;
    
    
    tmpTail = frameTail++;
    tmpData = 0;
    for(i=0;i<8;i++)
    {
    	 for(j=0;j<totalAddGroup.numberOfzjz;j++)
    	 {
    	   if (totalAddGroup.perZjz[j].zjzNo==i+1)
    	   {
    	 	    tmpData |= 1<<i;
    	 	    
    	 	    msFrame[frameTail++] = periodCtrlConfig[i].ctrlPara.ctrlPeriod;  //���ض�ֵ������
    	 	    msFrame[frameTail++] = periodCtrlConfig[i].ctrlPara.limitPeriod; //����ʱ����Ч��־λ
    	 	    
    	 	    //����״̬
    	 	    msFrame[frameTail] = 0;
    	 	    if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)   //ʱ�ο�
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x1;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)   //���ݿ�
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x2;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)   //Ӫҵ��ͣ��
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x4;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)   //��ǰ�����¸���
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x8;
    	 	    }
    	 	    frameTail++;
    	 	    
    	 	    //���״̬
    	 	    msFrame[frameTail] = 0x0;
    	 	    if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)   //�µ��
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x1;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)   //�����
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x2;
    	 	    }
    	 	    frameTail++;
    	 	    
    	 	    //�����ִ�״̬
    	 	    msFrame[frameTail++] = powerCtrlRoundFlag[i].flag;
    	 	    
    	 	    //����ִ�״̬
    	 	    msFrame[frameTail++] = electCtrlRoundFlag[i].flag;
    	 	 }
    	 }
    }
    msFrame[tmpTail] = tmpData;  //�ܼ�����Чλ��־λ
   #endif
     
   return frameTail;
}

/*******************************************************
��������:AFN0C006
��������:��Ӧ��վ����һ������"�ն˵�ǰ����״̬"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C006(INT16U frameTail,INT8U *pHandle, INT8U fn)
{    
   #ifdef LOAD_CTRL_MODULE
    INT8U  i,j;
    INT8U  tmpData;
    INT16U tmpTail;
    INT32U powerInt,powerDec;
    INT8U  powerQuantity;
    
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;    			//DA2
    msFrame[frameTail++] = 0x20;          //DT1
    msFrame[frameTail++] = 0;             //DT2
    
    //���ݵ�Ԫ
    
    //ң����բ���״̬
    msFrame[frameTail] = 0x0;
    for(i=0;i<CONTROL_OUTPUT;i++)
    {
      if (remoteCtrlConfig[i].status==CTRL_JUMPED)   //�Ƿ�����բ״̬   	
      {
    	   msFrame[frameTail] |= 0x1<<i;
      }
    }
    frameTail++;
    
   #ifndef CQDL_CSM 
    //��ǰ�߷Ѹ澯״̬
    if (reminderFee==CTRL_JUMP_IN)
    {
    	 msFrame[frameTail++] = 0x1;
    }
    else
    {
       msFrame[frameTail++] = 0x0;
    }
   #endif

    tmpTail = frameTail;
    frameTail++;
    
    tmpData = 0;
    for(i=0;i<8;i++)
    {
    	for(j=0;j<totalAddGroup.numberOfzjz;j++)
    	{
    		if (totalAddGroup.perZjz[j].zjzNo==i+1)
    	  {
    	  	tmpData |= 1<<i;
    	 	    
    	 	  //��ǰ���ض�ֵ
    	 	  if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	  	//��ǰ�����¸��ض�ֵ
    	 	    if (powerDownCtrl[i].freezeTime.year==0xff)
    	 	    {
    	 	    	powerInt = powerDownCtrl[i].powerDownLimit;
              powerDec = powerDownCtrl[i].powerLimitWatt*10;
              powerQuantity = dataFormat(&powerInt, &powerDec, 2);
                 
              powerInt = hexToBcd(powerInt);
              powerInt &=0xfff;
              powerInt |= (powerQuantity&0x10)<<8;
              powerInt |= (powerQuantity&0x07)<<13;
                  
              msFrame[frameTail++] = powerInt&0xff;
              msFrame[frameTail++] = powerInt>>8&0xff;
    	 	    }
    	 	    else   //���ڵȴ����㶨ֵ
    	 	    {
    	 	    	msFrame[frameTail++] = 0xee;
    	 	    	msFrame[frameTail++] = 0xee;
    	 	    }
    	 	  }
    	 	  else
    	 	  {
    	 	  	if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
    	 	    {
    	 	    	msFrame[frameTail++] = obsCtrlConfig[i].obsLimit&0xff;
    	 	    	msFrame[frameTail++] = obsCtrlConfig[i].obsLimit>>8&0xff;
    	 	    }
    	 	    else
    	 	    {
    	 	    	if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
    	 	    	{
    	 	    		msFrame[frameTail++] = wkdCtrlConfig[i].wkdLimit&0xff;
    	 	    	  msFrame[frameTail++] = wkdCtrlConfig[i].wkdLimit>>8&0xff;
    	 	    	}
    	 	    	else
    	 	    	{
    	 	    	  if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
    	 	    	  {
                  //��ȡ��ǰʱ��ε�ʱ������
                  if ((powerQuantity = getPowerPeriod(sysTime)) != 0)
                  {
                    //���ݵ�ǰʱ��, �ܼ����, ������, ʱ��������ȡʱ�ι��ض�ֵ
                    powerDec = 0;
               
                    if (getPowerLimit(i+1, periodCtrlConfig[i].ctrlPara.ctrlPeriod, powerQuantity, (INT8U *)&powerDec))
                    {
                   	  msFrame[frameTail++] = powerDec&0xff;
                   	  msFrame[frameTail++] = powerDec>>8&0xff;                           	  
                    }
                    else
                    {
    	 	    	 	      msFrame[frameTail++] = 0xee;
    	 	    	        msFrame[frameTail++] = 0xee;
                    }
                  }
                  else
                  {
    	 	    		  	msFrame[frameTail++] = 0xee;
    	 	    	      msFrame[frameTail++] = 0xee;
    	 	    	    }
    	 	    	  }
    	 	    	  else
    	 	    	  {
    	 	    	  	msFrame[frameTail++] = 0xee;
    	 	    	    msFrame[frameTail++] = 0xee;
    	 	    	  }
    	 	    	}
    	 	    }
    	 	  }
    	 	    
    	 	  //��ǰ�����¸��ظ���ϵ��
    	 	  if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    msFrame[frameTail++] = powerDownCtrl[i].floatFactor;
    	 	  }
    	 	  else
    	 	  {
    	 	    msFrame[frameTail++] = 0x00;
    	 	  }
    	 	    
    	 	  //������բ���״̬
    	 	  msFrame[frameTail] = 0x0;
    	 	  if ((powerCtrlRoundFlag[i].flag&0x1) && (powerCtrlRoundFlag[i].ifJumped&0x1))
    	 	  {
    	 	    msFrame[frameTail] |= 0x1;
    	 	  }
    	 	  if ((powerCtrlRoundFlag[i].flag>>1&0x1) && (powerCtrlRoundFlag[i].ifJumped>>1&0x1))
    	 	  {
    	 	    msFrame[frameTail] |= 0x2;
    	 	  }
    	 	  if ((powerCtrlRoundFlag[i].flag>>2&0x1) && (powerCtrlRoundFlag[i].ifJumped>>2&0x1))
    	 	  {
    	 	    msFrame[frameTail] |= 0x4;
    	 	  }
    	 	  if ((powerCtrlRoundFlag[i].flag>>3&0x1) && (powerCtrlRoundFlag[i].ifJumped>>3&0x1))
    	 	  {
    	 	  	msFrame[frameTail] |= 0x8;
    	 	  }
    	 	  frameTail++;
    	 	    
    	 	  //�µ����բ���״̬
    	 	  msFrame[frameTail] = 0x0;
    	 	  if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if ((electCtrlRoundFlag[i].flag&0x1) && (electCtrlRoundFlag[i].ifJumped&0x1))
    	 	    {
    	 	     	msFrame[frameTail] |= 0x1;
    	 	    }
    	 	    if ((electCtrlRoundFlag[i].flag>>1&0x1) && (electCtrlRoundFlag[i].ifJumped>>1&0x1))
    	 	    {
    	 	    	msFrame[frameTail] |= 0x2;
    	 	    }
    	 	    if ((electCtrlRoundFlag[i].flag>>2&0x1) && (electCtrlRoundFlag[i].ifJumped>>2&0x1))
    	 	    {
    	 	    	msFrame[frameTail] |= 0x4;
    	 	    }
    	 	    if ((electCtrlRoundFlag[i].flag>>3&0x1) && (electCtrlRoundFlag[i].ifJumped>>3&0x1))
    	 	    {
    	 	    	msFrame[frameTail] |= 0x8;
    	 	    }
    	 	    frameTail++;
    	 	       
    	 	    //�������բ���״̬
    	 	    msFrame[frameTail++] = 0x0;
    	 	  }
    	 	  else
    	 	  {
    	 	    msFrame[frameTail] = 0x0;
    	 	    frameTail++;
    	 	    	 
    	 	    //�������բ���״̬
            msFrame[frameTail] = 0x0;
    	 	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
    	 	    {
       	 	  	if ((electCtrlRoundFlag[i].flag&0x1) && (electCtrlRoundFlag[i].ifJumped&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x1;
       	 	    }
       	 	    if ((electCtrlRoundFlag[i].flag>>1&0x1) && (electCtrlRoundFlag[i].ifJumped>>1&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x2;
       	 	    }
       	 	    if ((electCtrlRoundFlag[i].flag>>2&0x1) && (electCtrlRoundFlag[i].ifJumped>>2&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x4;
       	 	    }
       	 	    if ((electCtrlRoundFlag[i].flag>>3&0x1) && (electCtrlRoundFlag[i].ifJumped>>3&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x8;
       	 	    }
       	 	    frameTail++;
    	 	    }
    	 	    else
    	 	    {
    	 	    	msFrame[frameTail++] = 0x0;
    	 	    }
    	 	  }
    	 	    
    	 	  //����Խ�޸澯״̬
    	 	  msFrame[frameTail] = 0x0;
    	 	  if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (periodCtrlConfig[i].ctrlPara.prdAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x1;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (wkdCtrlConfig[i].wkdAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x2;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (obsCtrlConfig[i].obsAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x4;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	   	if (powerDownCtrl[i].pwrDownAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x8;
    	 	    }
    	 	  }
    	 	    
    	 	  frameTail++;
    	 	    
    	 	  //���Խ�޸澯״̬
    	 	  msFrame[frameTail] = 0x0;
    	 	  if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (monthCtrlConfig[i].monthAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x1;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (chargeCtrlConfig[i].chargeAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x2;
    	 	    }
    	 	  }    	 	    
    	 	  frameTail++;
            
          //�߷ѿ���բ���״̬
          msFrame[frameTail] = 0x0;
          
    	  }
    	}
    }
    msFrame[tmpTail] = tmpData;  //�ܼ�����Чλ��־λ
  #endif
  
  return frameTail;
}

/*******************************************************
��������:AFN0C007
��������:��Ӧ��վ����һ������"�ն��¼���������ǰֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C007(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;      			//DA1
    msFrame[frameTail++] = 0x00;      			//DA2
    msFrame[frameTail++] = 0x40;            //DT1
    msFrame[frameTail++] = 0x00;            //DT2
    
    //���ݵ�Ԫ
    msFrame[frameTail++] = iEventCounter;   //��Ҫ�¼�������EC1ֵ
    msFrame[frameTail++] = nEventCounter;   //һ���¼�������EC2ֵ
	
    return frameTail;
}

/*******************************************************
��������:AFN0C008
��������:��Ӧ��վ����һ������"�ն��¼���־״̬"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C008(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    INT8U i;
    
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;			    //DA2
    msFrame[frameTail++] = 0x80;          //DT1
    msFrame[frameTail++] = 0;             //DT2
    
    //���ݵ�Ԫ
    for(i=0;i<8;i++)
    {
    	msFrame[frameTail++] = eventStatus[i];
    }
	
    return frameTail;
}

/*******************************************************
��������:AFN0C009
��������:��Ӧ��վ����һ������"�ն�״̬������λ��־"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C009(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = 0;          //DA1
    msFrame[frameTail++] = 0;          //DA2
    msFrame[frameTail++] = 0x1;        //DT1
    msFrame[frameTail++] = 0x1;        //DT2
    
    //���ݵ�Ԫ
   #ifndef PLUG_IN_CARRIER_MODULE
    if (getGateKValue()==1)  //�ſ��ڵ�7·
    {
      msFrame[frameTail++] = stOfSwitch | 0x40; //ST
    }
    else
    {
   #endif
   
      msFrame[frameTail++] = stOfSwitch; //ST
   
   #ifndef PLUG_IN_CARRIER_MODULE
    }
   #endif
    
    msFrame[frameTail++] = cdOfSwitch; //CD

	  cdOfSwitch = 0;

    return frameTail;
}

/*******************************************************
��������:AFN0C010
��������:��Ӧ��վ����һ������"�ն�����վ���գ���ͨ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C010(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
	INT8U                  i, tmpCount;
	DATE_TIME              tmpTime;
	INT32U                 tmpData;
	
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = 0;       //DA1
  msFrame[frameTail++] = 0;       //DA2
  msFrame[frameTail++] = 0x02;    //DT1
  msFrame[frameTail++] = 0x01;    //DT2
  
 	tmpTime = timeHexToBcd(sysTime);
	if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  {
  	 tmpData = terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
  	 //����ͨ������
  	 msFrame[frameTail++] = tmpData&0xff;
  	 msFrame[frameTail++] = tmpData>>8&0xff;
  	 msFrame[frameTail++] = tmpData>>16&0xff;
  	 msFrame[frameTail++] = tmpData>>24&0xff;
  	 
  	 //����ͨ������
 	   tmpCount = monthDays(sysTime.year+2000,sysTime.month);
 	   tmpData = 0;
 	   for(i=1;i<=tmpCount && i<=sysTime.day;i++)
 	   {
 	  	 tmpTime = sysTime;
 	  	 tmpTime.day = i;
 	  	 tmpTime = timeHexToBcd(tmpTime);
       if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
       {
       	  tmpData += terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
       }
 	   }
  	 msFrame[frameTail++] = tmpData&0xff;
  	 msFrame[frameTail++] = tmpData>>8&0xff;
  	 msFrame[frameTail++] = tmpData>>16&0xff;
  	 msFrame[frameTail++] = tmpData>>24&0xff; 	   
  }
  else
  {
    //����ͨ������
    for(i = 0; i < 4; i++)
 	  {
 		  msFrame[frameTail++] = 0x0;
 	  }
	
	  //����ͨ������
    for(i = 0; i < 4; i++)
 	  {
 		  msFrame[frameTail++] = 0x0;
 	  }
 	}
  
  return frameTail;
}

/*******************************************************
��������:AFN0C011
��������:��Ӧ��վ����һ������"�ն˼��г���״̬��Ϣ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2012-06-08,
    1)����485�˿ڳ���ɹ��������ص��û�����ɹ�����ȷ��
    2)ר���ն˲���Ҳ���ؿ����˿�31��ͳ��
*******************************************************/
INT16U AFN0C011(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U             i, j, tmpBlock;
	INT16U            tmpSuccess, tmpCount, tmpTail;
	struct cpAddrLink *tmpNode;
	DATE_TIME         tmpTime;
  INT8U             weekNumber;
  INT8U             dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U             keyCount;
    	
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = 0;       //DA1
  msFrame[frameTail++] = 0;       //DA2
  msFrame[frameTail++] = 0x04;    //DT1
  msFrame[frameTail++] = 0x01;    //DT2
  
  //���ݵ�Ԫ
  //�������ݿ����(������д)
  tmpTail = frameTail++;

  //1.�ն˳���˿ں�(485�˿�)	
	tmpBlock = 0;
	for(i=0;i<4;i++)
	{
  	tmpNode = initPortMeterLink(i);
  	tmpSuccess = 0;
  	tmpCount   = 0;
  	keyCount   = 0;
  	while(tmpNode!=NULL)
  	{
  	 	tmpCount++;
      
      tmpTime = timeHexToBcd(sysTime);
      
     #ifdef LIGHTING
		  //2016-12-07,�޸�485�ڳ����ɹ�����������
			if (
			    LIGHTING_XL== tmpNode->protocol
			     || LIGHTING_DGM== tmpNode->protocol
				 )
			{
				tmpTime = sysTime;
				tmpTime.second = 0x0;
				tmpTime.minute = 0x0;
				if (tmpTime.hour>0)
				{
				  tmpTime.hour--;
				}
				
				//ʮ��������
				if (readMeterData(dataBuff, tmpNode->mp, HOUR_FREEZE_SLC, 0x0, &tmpTime, 0) == TRUE)
				{
					tmpSuccess++;
				}
			}
     #else		 
			if ((tmpNode->bigAndLittleType&0xf)==1)//�������ܱ�
      {
        if (readMeterData(dataBuff, tmpNode->mp, SINGLE_PHASE_PRESENT, ENERGY_DATA, &tmpTime, 0) == TRUE)
        {
  	 	    tmpSuccess++;
  	 	  
  	 	    for(j=0; j<keyHouseHold.numOfHousehold; j++)
  	 	    {
  	 	  	  if ((keyHouseHold.household[j*2] | keyHouseHold.household[j*2+1]<<8)==tmpNode->mpNo)
  	 	  	  {
  	 	  		  keyCount++;
  	 	  	  }
  	 	    }
  	 	  }
      }
		 #endif
      else
      {
        if (readMeterData(dataBuff, tmpNode->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0) == TRUE)
        {
  	 	    tmpSuccess++;
  	 	  
  	 	    for(j=0; j<keyHouseHold.numOfHousehold; j++)
  	 	    {
  	 	  	  if ((keyHouseHold.household[j*2] | keyHouseHold.household[j*2+1]<<8)==tmpNode->mpNo)
  	 	  	  {
  	 	  		  keyCount++;
  	 	  	  }
  	 	    }
  	 	  }
      }
  	 	   	  
  	 	tmpNode = tmpNode->next;
  	}
    
    if (tmpCount>0)
    {
      tmpBlock++;
      
      //�˿ں�
      msFrame[frameTail++] = i+1;
  
      //Ҫ�����ܱ�����
      msFrame[frameTail++] = tmpCount&0xff;
      msFrame[frameTail++] = tmpCount>>8&0xff;
      
      //��ǰ������״̬��־
      if (copyCtrl[i].meterCopying==TRUE)
      {
      	 msFrame[frameTail++] = 0x03;
      }
      else
      {
      	 msFrame[frameTail++] = 0x02;
      }
      
      msFrame[frameTail++] = tmpSuccess&0xff;
      msFrame[frameTail++] = tmpSuccess>>8&0xff;
      
      //���ص��û��ɹ�����
      msFrame[frameTail++] = keyCount;
      
      //����ʼʱ��
      tmpTime = timeBcdToHex(copyCtrl[i].lastCopyTime);
    	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
    	if (weekNumber == 0)
    	{
    	  weekNumber = 7;
    	}
    	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
    	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
    	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
    	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //��(ǰ��λBCD��ʮλ������λBCD���λ)
    	  	
    	if (tmpTime.month<10)
    	{
    	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
    	}
    	else
    	{
    	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10;  //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
    	}
    	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //��(ʮλ+��λ)
    	      
      //�������ʱ��
      tmpTime = copyCtrl[i].nextCopyTime;
    	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
    	if (weekNumber == 0)
    	{
    	  weekNumber = 7;
    	}
    	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
    	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
    	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
    	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //��(ǰ��λBCD��ʮλ������λBCD���λ)
    	  	
    	if (tmpTime.month<10)
    	{
    	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
    	}
    	else
    	{
    	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10;  //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
    	}
    	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //��(ʮλ+��λ)
    }
  }
  
  //2.�ز�/���߶˿�
  #ifdef PLUG_IN_CARRIER_MODULE
    msFrame[frameTail++] = PORT_POWER_CARRIER;
  	
  	tmpNode = copyCtrl[4].cpLinkHead;
  	tmpSuccess = 0;
  	tmpCount   = 0;
  	keyCount   = 0;
  	while(tmpNode!=NULL)
  	{
  	 	tmpCount++;
  	 	if (tmpNode->copySuccess==TRUE)
  	 	{
  	 	  tmpSuccess++;
  	 	  
  	 	  for(j=0; j<keyHouseHold.numOfHousehold; j++)
  	 	  {
  	 	  	if ((keyHouseHold.household[j*2] | keyHouseHold.household[j*2+1]<<8)==tmpNode->mpNo)
  	 	  	{
  	 	  		keyCount++;
  	 	  	}
  	 	  }
  	 	}
  
  	 	tmpNode = tmpNode->next;
  	}
  	
    //Ҫ�����ܱ�����
    msFrame[frameTail++] = tmpCount&0xff;
    msFrame[frameTail++] = tmpCount>>8&0xff;
    
    //��ǰ������״̬��־
    if (copyCtrl[4].meterCopying==TRUE)
    {
    	 msFrame[frameTail++] = 0x03;
    }
    else
    {
    	 msFrame[frameTail++] = 0x02;
    }
    
    //����ɹ�����
    msFrame[frameTail++] = tmpSuccess&0xff;
    msFrame[frameTail++] = tmpSuccess>>8&0xff;
    
    //���ص��û��ɹ�����
    msFrame[frameTail++] = keyHouseHold.numOfHousehold;
    
    //����ʼʱ��
    tmpTime = timeBcdToHex(copyCtrl[4].lastCopyTime);
  	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
  	if (weekNumber == 0)
  	{
  	  weekNumber = 7;
  	}
  	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
  	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
  	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
  	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //��(ǰ��λBCD��ʮλ������λBCD���λ)
  	  	
  	if (tmpTime.month<10)
  	{
  	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
  	}
  	else
  	{
  	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10; //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
  	}
  	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //��(ʮλ+��λ)
  	    
    //�������ʱ��
    tmpTime = copyCtrl[4].nextCopyTime;
  	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
  	if (weekNumber == 0)
  	{
  	  weekNumber = 7;
  	}
  	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
  	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
  	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
  	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //��(ǰ��λBCD��ʮλ������λBCD���λ)
  	  	
  	if (tmpTime.month<10)
  	{
  	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
  	}
  	else
  	{
  	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10; //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
  	}
  	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //��(ʮλ+��λ)
  	
  	//���ݿ���
  	msFrame[tmpTail] = tmpBlock+1;
  
  #else
  
  	//���ݿ���
  	msFrame[tmpTail] = tmpBlock;
  #endif
	
  return frameTail;
}

/*******************************************************
��������:AFN0C014
��������:��Ӧ��վ����һ������"�ն�Ͷ��״̬(�����Լ)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C014(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = 0;       //DA1
   msFrame[frameTail++] = 0;       //DA2
   msFrame[frameTail++] = 0x20;    //DT1
   msFrame[frameTail++] = 0x01;    //DT2
   
  #ifdef CQDL_CSM
   msFrame[frameTail++] = teInRunning;  //������Ͷ��״̬
  #else    //13��-�ļ�����δ�յ������ݶ�
   unRecvSeg(&msFrame[frameTail]);
   frameTail += 130;
  #endif
   
   
   return frameTail;
}

/*******************************************************
��������:AFN0C015
��������:��Ӧ��վ����һ������"�ն�����ʧ�ܱ�ͳ����Ϣ(�����Լ)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C015(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   INT8U weekNumber;

   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = 0;       //DA1
   msFrame[frameTail++] = 0;       //DA2
   msFrame[frameTail++] = 0x40;    //DT1
   msFrame[frameTail++] = 0x01;    //DT2

   //���ݵ�Ԫ
   //1.������ʼʱ��
	 weekNumber = dayWeek(2000+searchStart.year,searchStart.month,searchStart.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchStart.second/10<<4 | searchStart.second%10;   //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchStart.minute/10<<4 | searchStart.minute%10;   //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchStart.hour  /10<<4 | searchStart.hour  %10;   //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchStart.day   /10<<4 | searchStart.day   %10;	  //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  	
	 if (searchStart.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchStart.month;               //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchStart.month%10;     //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 msFrame[frameTail++] = searchStart.year/10<<4 | searchStart.year%10;       //��(ʮλ+��λ)

   //2.��������ʱ��
	 weekNumber = dayWeek(2000+searchEnd.year,searchEnd.month,searchEnd.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchEnd.second/10<<4 | searchEnd.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchEnd.minute/10<<4 | searchEnd.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchEnd.hour  /10<<4 | searchEnd.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchEnd.day   /10<<4 | searchEnd.day   %10;	      //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  	
	 if (searchEnd.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchEnd.month;                 //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchEnd.month%10;       //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 msFrame[frameTail++] = searchEnd.year/10<<4 | searchEnd.year%10;           //��(ʮλ+��λ)
	 
	 //3.ע�����ܿ���
	 msFrame[frameTail++] =  meterDeviceNum&0xff;
	 msFrame[frameTail++] =  meterDeviceNum>>8&0xff;
	 
	 //4.����ʧ�ܵ���ܿ���
	 msFrame[frameTail++] =  0x0;
	 msFrame[frameTail++] =  0x0;
	 
	 return frameTail;
}

/*******************************************************
��������:AFN0C016
��������:��Ӧ��վ����һ������"�ն�����������ͳ����Ϣ(�����Լ)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C016(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   struct carrierMeterInfo *tmpNewFound;
   INT8U                   weekNumber;
   INT16U                  numOfNewMeter;
   INT16U                  tmpTail;
	
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = 0;       //DA1
   msFrame[frameTail++] = 0;       //DA2
   msFrame[frameTail++] = 0x80;    //DT1
   msFrame[frameTail++] = 0x01;    //DT2
  
   //���ݵ�Ԫ
   //1.������ʼʱ��
	 weekNumber = dayWeek(2000+searchStart.year,searchStart.month,searchStart.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchStart.second/10<<4 | searchStart.second%10;   //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchStart.minute/10<<4 | searchStart.minute%10;   //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchStart.hour  /10<<4 | searchStart.hour  %10;   //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchStart.day   /10<<4 | searchStart.day   %10;	  //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  	
	 if (searchStart.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchStart.month;               //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchStart.month%10;     //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 msFrame[frameTail++] = searchStart.year/10<<4 | searchStart.year%10;       //��(ʮλ+��λ)

   //2.��������ʱ��
	 weekNumber = dayWeek(2000+searchEnd.year,searchEnd.month,searchEnd.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchEnd.second/10<<4 | searchEnd.second%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchEnd.minute/10<<4 | searchEnd.minute%10;       //��(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchEnd.hour  /10<<4 | searchEnd.hour  %10;       //ʱ(ǰ��λBCD��ʮλ������λBCD���λ)
	 msFrame[frameTail++] = searchEnd.day   /10<<4 | searchEnd.day   %10;	      //��(ǰ��λBCD��ʮλ������λBCD���λ)
	  	
	 if (searchEnd.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchEnd.month;                 //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchEnd.month%10;       //����-��(ǰ��λBCD�����ڣ���4λBCD����ʮλ������λBCD���¸�λ)
	 }
	 msFrame[frameTail++] = searchEnd.year/10<<4 | searchEnd.year%10;           //��(ʮλ+��λ)
	 
	 //3.ע�����ܿ���
	 msFrame[frameTail++] =  meterDeviceNum&0xff;
	 msFrame[frameTail++] =  meterDeviceNum>>8&0xff;
	 
	 //4.��������ܿ����������ַ
	 tmpTail = frameTail;
	 frameTail += 2;
	 numOfNewMeter = 0;
  #ifdef PLUG_IN_CARRIER_MODULE
   tmpNewFound = foundMeterHead;
	 while(tmpNewFound!=NULL)
	 {
     numOfNewMeter++;
     
     memcpy(&msFrame[frameTail], tmpNewFound->addr, 6);
     frameTail += 6;     
     
     tmpNewFound = tmpNewFound->next;
	 }
	#endif
	
	 msFrame[tmpTail]   = numOfNewMeter&0xff;;
	 msFrame[tmpTail+1] = numOfNewMeter>>8&0xff;;
  
   return frameTail;
}

/*******************************************************
��������:AFN0C017
��������:��Ӧ��վ����һ������"��ǰ�ܼ��й�����"����(���ݸ�ʽ2)
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C017(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
  INT16U    offset;
  DATE_TIME tmpTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		if(pn > 8)
  		{
  			return frameTail;
  		}
  		
  		//���ݵ�Ԫ��ʶ
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  //DA1
      msFrame[frameTail++] = 0x01<<((pn-1)/8);                    //DA2
      
      //��������
      if (fn == 17)
      {    
         msFrame[frameTail++] = 0x01;      //DT1
         offset = GP_WORK_POWER;
      }
      else
      {
         msFrame[frameTail++] = 0x02;      //DT1 
      	 offset = GP_NO_WORK_POWER;
      }
      msFrame[frameTail++] = 0x02;         //DT2
      
      tmpTime = timeHexToBcd(sysTime);
      if (readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
	    {
	      if (dataBuff[offset+1] != 0xEE)
	      {
	        msFrame[frameTail++] = dataBuff[offset+1];        //ʮλ ��λ
	        if (dataBuff[offset]==0xee)
	        {
	        }
	        else
	        {
            msFrame[frameTail++] = ((dataBuff[offset]&0x7)<<5)
                               |(dataBuff[offset]&0x10)
                               |dataBuff[offset+2]&0x0F;    //������ ���� ��λ
          }
        }
        else
        {
          #ifdef NO_DATA_USE_PART_ACK_03
           frameTail -= 4;
          #else          
           msFrame[frameTail++] = 0xee;
           msFrame[frameTail++] = 0xee;
          #endif
        }
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
          frameTail -= 4;
        #else          
          msFrame[frameTail++] = 0xee;
          msFrame[frameTail++] = 0xee;
        #endif
      }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0C018
��������:��Ӧ��վ����һ������"��ǰ�ܼ��޹�����"����(���ݸ�ʽ2)
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C018(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C017(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C019
��������:��Ӧ��վ����һ������"�����ܼ��й�������"������ݸ�ʽ3��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C019(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
  INT8U     tariff, tmpTariff;             //������
  INT8U     da1, da2;
  INT16U    offset;
  BOOL      ifHasData;
  DATE_TIME tmpTime;
  INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-14,add
  BOOL      bufHasData;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		if(pn > 8)
  		{
  			return frameTail;
  		}  
  		
  		//���ݵ�Ԫ��ʶ
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  //DA1
      msFrame[frameTail++] = 0x01<<((pn-1)/8);                    //DA2

      //��������
      switch (fn)
      {
     	  case 19:
          msFrame[frameTail++] = 0x04;    //DT1
          offset = GP_DAY_WORK;      
          break;
          
        case 20:
       	  msFrame[frameTail++] = 0x08;    //DT1
       	  offset = GP_DAY_NO_WORK;
       	  break;
    	 
    	  case 21:
    	 	  msFrame[frameTail++] = 0x10;    //DT1
    	 	  offset = GP_MONTH_WORK;
          break;
         
        case 22:
       	  msFrame[frameTail++] = 0x20;    //DT1
       	  offset = GP_MONTH_NO_WORK;
       	  break;
      }
      msFrame[frameTail++] = 0x02;    //DT2
    
      //if ((tariff = numOfTariff(pn)) == 0)
      //{
      //	tariff = 4;
      //}
    
      tariff = 4;
      
      //�鿴���ܼ����Ƿ�ֻ�����������
      onlyHasPulsePn = 0;
      for (i = 0; i < totalAddGroup.numberOfzjz; i++)
      {
      	if (totalAddGroup.perZjz[i].zjzNo == pn)
      	{
          onlyHasPulsePn = 0;
      		for(j=0;j<totalAddGroup.perZjz[i].pointNumber;j++)
      		{
      		 	for(k=0;k<pulseConfig.numOfPulse;k++)
      		 	{
      		 		 if (pulseConfig.perPulseConfig[k].pn==(totalAddGroup.perZjz[i].measurePoint[j]+1))
      		 		 {
      		 		 	  onlyHasPulsePn++;
      		 		 }
      		 	}
      		}
      		
      		if (onlyHasPulsePn==totalAddGroup.perZjz[i].pointNumber)
      		{
      			 onlyHasPulsePn = 0xaa;
      		}
      		
      		break;
      	}
      }
      
      tmpTime = timeHexToBcd(sysTime);
      
      bufHasData = FALSE;
      
      if (onlyHasPulsePn==0xaa)  //ֻ�������������������һ��
      {
      	memset(dataBuff, 0xee, LEN_OF_ZJZ_BALANCE_RECORD);
      	
        if (fn==19 || fn==21)
        {
          bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, tmpTime);
        }
        else
        {
          bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_NO_WORK | 0x80, tmpTime);
        }
      	
      	tariff = periodTimeOfCharge[48];
      }
      else    //�����485�������ȡ�ϴμ���ֵ
      {
         bufHasData = readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0);
      }

      if (bufHasData == TRUE)
  	  {
	  	   ifHasData = FALSE;
    	   msFrame[frameTail++] = tariff;
       
         for(tmpTariff=0;tmpTariff <=tariff;tmpTariff++)
         {
           if (dataBuff[offset]!=0xEE)
           {
             ifHasData = TRUE;
           }
           msFrame[frameTail++] = dataBuff[offset+3];
           msFrame[frameTail++] = dataBuff[offset+4];
           msFrame[frameTail++] = dataBuff[offset+5];
          
           if (dataBuff[offset]!=0xee)
           {
             msFrame[frameTail++] = ((dataBuff[offset]&0x01)<<6)
                                  |(dataBuff[offset]&0x10)
                                  |(dataBuff[offset+6]&0x0f);
           }
           else
           {
             msFrame[frameTail++] = 0xee;
           }
           
           offset += 7;
         }
        
         #ifdef NO_DATA_USE_PART_ACK_03
          if (!ifHasData)
          {
            frameTail -= (tariff+1)*4+5;
          }
         #endif
	    } 
	    else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 4;
        #else
	  	  msFrame[frameTail++] = tariff;
        for(tmpTariff=0;tmpTariff < (tariff+1)*4;tmpTariff++)
        {
          msFrame[frameTail++] = 0xee;
        }
        #endif
      }
  	}
  	da1 >>= 1;
  }

  return frameTail;
}

/*******************************************************
��������:AFN0C020
��������:��Ӧ��վ����һ������"�����ܼ��޹�������"������ݸ�ʽ3��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C020(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    return AFN0C019(frameTail,pHandle,fn);
}

/*******************************************************
��������:AFN0C021
��������:��Ӧ��վ����һ������"�����ܼ��й�������"������ݸ�ʽ3��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C021(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    return AFN0C019(frameTail,pHandle,fn);
}

/*******************************************************
��������:AFN0C022
��������:��Ӧ��վ����һ������"�����ܼ��޹�������"������ݸ�ʽ3��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C022(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    return AFN0C019(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C023
��������:��Ӧ��վ����һ������"�ն˵�ǰʣ�����(��)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C023(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   #ifdef LOAD_CTRL_MODULE
    INT16U    tmpPn,pn;
    INT8U     leftReadBuf[12];
    INT8U     da1, da2;
    DATE_TIME tmpTime;
    INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-14,add
    BOOL      bufHasData;

    da1 = *pHandle++;
    da2 = *pHandle++;
    
    tmpPn = 0;
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		if (pn>32)
		    {
		    	 return frameTail;
		    }

        //�鿴���ܼ����Ƿ�ֻ�����������
        onlyHasPulsePn = 0;
        for (i = 0; i < totalAddGroup.numberOfzjz; i++)
        {
        	if (totalAddGroup.perZjz[i].zjzNo == pn)
        	{
            onlyHasPulsePn = 0;
        		for(j=0;j<totalAddGroup.perZjz[i].pointNumber;j++)
        		{
        		 	for(k=0;k<pulseConfig.numOfPulse;k++)
        		 	{
        		 		 if (pulseConfig.perPulseConfig[k].pn==(totalAddGroup.perZjz[i].measurePoint[j]+1))
        		 		 {
        		 		 	  onlyHasPulsePn++;
        		 		 }
        		 	}
        		}
        		
        		if (onlyHasPulsePn==totalAddGroup.perZjz[i].pointNumber)
        		{
        			 onlyHasPulsePn = 0xaa;
        		}
        		
        		break;
        	}
        }

		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x40;
		    msFrame[frameTail++] = 0x02;
        
        tmpTime = sysTime;
        if (onlyHasPulsePn==0xaa)  //ֻ�в�������������һ��
        {
        	if (debugInfo&PRINT_PULSE_DEBUG)
        	{
        	  printf("�ܼ���ֻ�����������,���㼰ʱʣ�����\n");
        	}
        	
        	memset(leftReadBuf,0xee,12);

          bufHasData = computeInTimeLeftPower(pn, tmpTime, leftReadBuf, 1);
        }
        else    //�����485�������ȡ�ϴμ���ֵ
        {
        	if (debugInfo&PRINT_PULSE_DEBUG)
        	{
        	  printf("�ܼ����ϲ�����,��ȡ�ϴμ����ʣ�����\n");
        	}

        	bufHasData = readMeterData(leftReadBuf, pn, LEFT_POWER, 0x0, &tmpTime, 0);
        }

        if (bufHasData==TRUE)
        {
      	  msFrame[frameTail++] = leftReadBuf[0];
      	  msFrame[frameTail++] = leftReadBuf[1];
      	  msFrame[frameTail++] = leftReadBuf[2];
      	  msFrame[frameTail++] = leftReadBuf[3];
        }
        else
        {
      	  msFrame[frameTail++] = 0xee;
      	  msFrame[frameTail++] = 0xee;
      	  msFrame[frameTail++] = 0xee;
      	  msFrame[frameTail++] = 0xee;        	 
        }		    
    	}
    	da1 >>= 1;
    }
   #endif
   
   return frameTail;
}

/*******************************************************
��������:AFN0C024
��������:��Ӧ��վ����һ������"��ǰ�����¸��ؿغ��ܼ��й����ʶ���ֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C024(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   #ifdef LOAD_CTRL_MODULE
    INT16U pn, tmpPn = 0;
    INT8U  da1, da2;
    INT32U powerInt,powerDec;
    INT8U  powerQuantity;
    
    da1 = *pHandle++;
    da2 = *pHandle++;
    
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		if (pn>32)
		    {
		    	 return frameTail;
		    }
		    
		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x80;
		    msFrame[frameTail++] = 0x02;
		    
		    if (powerDownCtrl[pn-1].freezeTime.year==0xff)
		    {
		       powerInt = powerDownCtrl[pn-1].powerDownLimit;
		       powerDec = powerDownCtrl[pn-1].powerLimitWatt*10;
		       powerQuantity = dataFormat(&powerInt, &powerDec, 2);
		                 
		       powerInt = hexToBcd(powerInt);
		       powerInt &=0xfff;
		       powerInt |= (powerQuantity&0x10)<<8;
		       powerInt |= (powerQuantity&0x07)<<13;
		       
		       msFrame[frameTail++] = powerInt&0xff;
		       msFrame[frameTail++] = powerInt>>8&0xff;
		    }
		    else
		    {
		    	 msFrame[frameTail++] = 0xee;
		    	 msFrame[frameTail++] = 0xee;
		    }
    	}
    	da1 >>= 1;
    }
   #endif
   
   return frameTail;
}

/*******************************************************
��������:AFN0C025
��������:��Ӧ��վ����һ������"(������)�༰���У��޹����ʡ�����������
         �����ѹ���������������"������ݸ�ʽ15��9��5��7��6��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C025(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG  meterConfig;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT8U     tmpPn, pn;
  INT8U     i, j;
  INT8U     da1, da2;
  INT8U     pulsePnData = 0;  
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    tmpTail,counti;  
  DATE_TIME time;
  INT8U     meterInfo[10];
	
  tmpPn = 0;
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  meterInfo[0] = 0xff;
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);

   		#ifdef PULSE_GATHER
			 	//�鿴�Ƿ����������������
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //���������Ĳ�����
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
			    {
			      //P.1�ȳ�ʼ������
            memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);

				 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
			   	  covertPulseData(j, NULL,NULL,dataBuff);
  		      
  		      //ly,2011-05-21,add
  		      time = timeHexToBcd(sysTime);

			      pulsePnData = 1;
			   	  	 	    
			      buffHasData = TRUE;
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //�����������������
		  {
		  	buffHasData = FALSE;
		  	
        if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
        {
		      if (meterConfig.protocol==AC_SAMPLE)
		      {
		     	  if (ifHasAcModule==TRUE)
		     	  {
			        //A.1�ȳ�ʼ������
			        for (counti = 0; counti < LENGTH_OF_PARA_RECORD;counti++)
			        {
			          dataBuff[counti] = 0xEE;
			        }
			   
			       	//A.2������������������dataBuff��
			       	covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
  		        
  		        //ly,2011-05-21,add
  		        time = timeHexToBcd(sysTime);

			     	  buffHasData = TRUE;
		   	    }
		      }
		  	  else
		  	  {
		  	    queryMeterStoreInfo(pn, meterInfo);
		  	      
		  	    if (meterInfo[0]==8)
		  	    {
		  	      buffHasData =  readMeterData(dataBuff, pn , KEY_HOUSEHOLD_PRESENT, PARA_VARIABLE_DATA, &time, 0);
		  	    }
		  	    else
		  	    {
		  	      //buffHasData =  readMeterData(dataBuff, pn , PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0);
		  	      buffHasData =  readMeterData(dataBuff, pn , LAST_TODAY, PARA_VARIABLE_DATA, &time, 0);
		  	    }
		  	  }
		  	}
		  }
		    	
		  msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
		  msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
		  msFrame[frameTail++] = 0x1;                                       //DT1
		  msFrame[frameTail++] = 0x3;                                       //DT2
		
		  ifHasData = FALSE;
		  if (buffHasData == TRUE)
		  {
		    //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
		     		     
		     //���й�����
		     if (dataBuff[POWER_INSTANT_WORK] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_WORK];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_WORK+1];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_WORK+2];
		       
		     //A���й�����
		     if (dataBuff[POWER_PHASE_A_WORK] != 0xEE)
		     {
		   	 		ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_WORK];
		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_WORK+1];
		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_WORK+2];
		        
		     //B���й�����
         if (dataBuff[POWER_PHASE_B_WORK] != 0xEE && dataBuff[POWER_PHASE_B_WORK] != 0xFF)
         {
        	 ifHasData = TRUE;
           msFrame[frameTail++] = dataBuff[POWER_PHASE_B_WORK];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_B_WORK+1];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_B_WORK+2];
         }
         else
         {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
         }
        
         //C���й�����
         if (dataBuff[POWER_PHASE_C_WORK] != 0xEE && dataBuff[POWER_PHASE_C_WORK] != 0xFF)
         {
        	 ifHasData = TRUE;
           msFrame[frameTail++] = dataBuff[POWER_PHASE_C_WORK];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_C_WORK+1];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_C_WORK+2];
         }
         else
         {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
         }
		        
		     //���޹�����
		     if (dataBuff[POWER_INSTANT_NO_WORK] != 0xEE)
		     {
		      	ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_NO_WORK];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_NO_WORK+1];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_NO_WORK+2];
		        
		     //A���޹�����
		     if (dataBuff[POWER_PHASE_A_NO_WORK] != 0xEE && dataBuff[POWER_PHASE_A_NO_WORK]!=0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_A_NO_WORK];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_A_NO_WORK+1];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_A_NO_WORK+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		       
		     //B���޹�����
		     if (dataBuff[POWER_PHASE_B_NO_WORK] != 0xEE && dataBuff[POWER_PHASE_B_NO_WORK]!=0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_NO_WORK];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_NO_WORK+1];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_NO_WORK+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		        
		     //C���޹�����
		     if (dataBuff[POWER_PHASE_C_NO_WORK] != 0xEE && dataBuff[POWER_PHASE_C_NO_WORK]!=0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_NO_WORK];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_NO_WORK+1];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_NO_WORK+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		     
		     if (meterInfo[0]==8)
		     {
		     	  memset(&msFrame[frameTail], 0xee, 8);
		     	  frameTail += 8;
  		   }
  		   else
  		   {
  		     //�ܹ�������
  		     if (dataBuff[TOTAL_POWER_FACTOR] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[TOTAL_POWER_FACTOR];
  		     msFrame[frameTail++] = dataBuff[TOTAL_POWER_FACTOR+1];
  		      
  		     //A�๦������
  		     if (dataBuff[FACTOR_PHASE_A] != 0xEE && dataBuff[FACTOR_PHASE_A] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_A];
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_A+1];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		       
  		     //B�๦������
  		     if (dataBuff[FACTOR_PHASE_B] != 0xEE && dataBuff[FACTOR_PHASE_B] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_B];
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_B+1];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;		     	 
  		     }
  		        
  		     //C�๦������
  		     if (dataBuff[FACTOR_PHASE_C] != 0xEE && dataBuff[FACTOR_PHASE_C] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_C];
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_C+1];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		   }
		        
		     //A���ѹ
		     if (dataBuff[VOLTAGE_PHASE_A] != 0xEE)
		     {
		      	ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_A];
		     msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_A+1];
		        
		     //B���ѹ
		     if (dataBuff[VOLTAGE_PHASE_B] != 0xEE  && dataBuff[VOLTAGE_PHASE_B] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_B];
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_B+1];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
         }
		        
		     //C���ѹ
		     if (dataBuff[VOLTAGE_PHASE_C] != 0xEE  && dataBuff[VOLTAGE_PHASE_C] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_C];
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_C+1];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		       
		     //A�����
		     if (dataBuff[CURRENT_PHASE_A] != 0xEE)
		     {
		      	ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[CURRENT_PHASE_A];
		     msFrame[frameTail++] = dataBuff[CURRENT_PHASE_A+1];
		     msFrame[frameTail++] = dataBuff[CURRENT_PHASE_A+2];
		        
		     //B�����
		     if (dataBuff[CURRENT_PHASE_B] != 0xEE  && dataBuff[CURRENT_PHASE_B] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_B];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_B+1];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_B+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		        
		     //C�����
		     if (dataBuff[CURRENT_PHASE_C] != 0xEE && dataBuff[CURRENT_PHASE_C] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_C];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_C+1];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_C+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }

		     if (meterInfo[0]==8)
		     {
		     	  memset(&msFrame[frameTail], 0xee, 15);
		     	  frameTail += 15;
  		   }
  		   else
  		   {
  		     //�������
  		     if (dataBuff[ZERO_SERIAL_CURRENT] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[ZERO_SERIAL_CURRENT];
  		     msFrame[frameTail++] = dataBuff[ZERO_SERIAL_CURRENT+1];
  		     msFrame[frameTail++] = dataBuff[ZERO_SERIAL_CURRENT+2];
  		     
  		     //��ǰ�����ڹ���
  		     if (dataBuff[POWER_INSTANT_APPARENT] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_APPARENT];
  		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_APPARENT+1];
  		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_APPARENT+2];
  		     
  		     //��ǰA�����ڹ���
  		     if (dataBuff[POWER_PHASE_A_APPARENT] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_APPARENT];
  		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_APPARENT+1];
  		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_APPARENT+2];
  		     
  		     //��ǰB�����ڹ���
  		     if (dataBuff[POWER_PHASE_B_APPARENT] != 0xEE && dataBuff[POWER_PHASE_B_APPARENT] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_APPARENT];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_APPARENT+1];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_APPARENT+2];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		     
  		     //��ǰC�����ڹ���
  		     if (dataBuff[POWER_PHASE_C_APPARENT] != 0xEE && dataBuff[POWER_PHASE_C_APPARENT] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_APPARENT];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_APPARENT+1];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_APPARENT+2];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		     
  		    #ifdef NO_DATA_USE_PART_ACK_03
  		     if (ifHasData == FALSE)
  		     {
  		       frameTail -= 71;
  		     }
  		    #endif
  		   }
		  }
		  else
		  {
		    #ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
			    msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
			    msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
			    msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
			    msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
			    for(j=0;j<62;j++)
			    {
			  	  msFrame[frameTail++] = 0xee;
			    }
		    #endif
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0C026
��������:��Ӧ��վ����һ������"A��B��C�������ͳ������
         �����һ�ζ����¼"������ݸ�ʽ15��8��10��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C026(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     i;  
  BOOL      ifHasData;
  DATE_TIME time;
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 -1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
  		
  		msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      msFrame[frameTail++] = 0x2;                                       //DT1
      msFrame[frameTail++] = 0x3;                                       //DT2

    	ifHasData = FALSE;
	        
      //��ȡ��ָ�������㡢���ͺ�ʱ�������ҳ
      if ( readMeterData(dataBuff, pn, PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0) == TRUE)
      {
        //���ݾ��ɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
  
        //�ܶ������
        if (dataBuff[PHASE_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_DOWN_TIMES+1];
        
        //A��������
        if (dataBuff[PHASE_A_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_A_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_A_DOWN_TIMES+1];
        
        //B��������
        if (dataBuff[PHASE_B_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_B_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_B_DOWN_TIMES+1];
        
        //C��������
        if (dataBuff[PHASE_C_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_C_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_C_DOWN_TIMES+1];
        
        //�ܶ�ʱ��
        if (dataBuff[TOTAL_PHASE_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_DOWN_TIME+2];
        
        //A�����ʱ��
        if (dataBuff[TOTAL_PHASE_A_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_A_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_A_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_A_DOWN_TIME+2];
        
        //B�����ʱ��
        if (dataBuff[TOTAL_PHASE_B_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_B_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_B_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_B_DOWN_TIME+2];
        
        //C�����ʱ��
        if (dataBuff[TOTAL_PHASE_C_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_C_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_C_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_C_DOWN_TIME+2];
        
        //���һ�ζ�����ʼʱ��
        if (dataBuff[LAST_PHASE_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        //��ʱ����
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+4];
        
        //A�����һ�ζ�����ʼʱ��
        if (dataBuff[LAST_PHASE_A_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+4];
        
        //B�����һ�ζ�����ʼʱ��
        if (dataBuff[LAST_PHASE_B_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+4];
        
        //C�����һ�ζ�����ʼʱ��
        if (dataBuff[LAST_PHASE_C_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+4];
        
        //���һ�ζ������ʱ��
        if (dataBuff[LAST_PHASE_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+4];
        
        //A�����һ�ζ������ʱ��
        if (dataBuff[LAST_PHASE_A_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+4];
        
        //B�����һ�ζ������ʱ��
        if (dataBuff[LAST_PHASE_B_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+4];
        
        //C�����һ�ζ������ʱ��
        if (dataBuff[LAST_PHASE_C_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+4];
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	          frameTail -= 61;
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         
         for(i=0;i<52;i++)
         {
         	  msFrame[frameTail++] = 0xEE;
         }
        #endif
      }
  	}
  	da1 >>= 1;
  }
    	
  return frameTail;        
}

/*******************************************************
��������:AFN0C027
��������:��Ӧ��վ����һ������"���ܱ�����ʱ��,��̴�������
						���һ�β���ʱ��"������ݸ�ʽ15,1,17,8,10��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C027(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     i;  
  BOOL      ifHasData;  
  DATE_TIME time;
	
	da1 = *pHandle++;
	da2 = *pHandle++;
	  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
 			
 			msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      msFrame[frameTail++] = 0x4;                                       //DT1
      msFrame[frameTail++] = 0x3;                                       //DT2

    	ifHasData = FALSE;
	        
      //��ȡ��ָ�������㡢���ͺ�ʱ�������ҳ
      if ( readMeterData(dataBuff, pn, PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0) == TRUE)
      {
        //���ݾ��ɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
        
        //���ܱ�����ʱ��
        //���ʱ
        if (dataBuff[METER_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[METER_TIME];
        msFrame[frameTail++] = dataBuff[METER_TIME+1];
        msFrame[frameTail++] = dataBuff[METER_TIME+2];
      
      	//�� ����-�� ��
        if (dataBuff[DATE_AND_WEEK] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[DATE_AND_WEEK+1];
        msFrame[frameTail++] = (dataBuff[DATE_AND_WEEK]&0xFF)<<5 | (dataBuff[DATE_AND_WEEK+2] & 0x1F);
        msFrame[frameTail++] = dataBuff[DATE_AND_WEEK+3];
      
        //��ع���ʱ��
        if (dataBuff[BATTERY_WORK_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME];
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME + 1];
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME + 2];
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME + 3];
  
        //����ܴ���
        if (dataBuff[PROGRAM_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PROGRAM_TIMES];
        msFrame[frameTail++] = dataBuff[PROGRAM_TIMES + 1];
        msFrame[frameTail++] = dataBuff[PROGRAM_TIMES + 2];
  
        //���һ�α�̷���ʱ��[���ݸ�ʽ1]
        if (dataBuff[LAST_PROGRAM_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+1];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+2];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+3];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+5];
        
        //��������ܴ���
        if (dataBuff[METER_CLEAR_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[METER_CLEAR_TIMES];
        msFrame[frameTail++] = dataBuff[METER_CLEAR_TIMES + 1];
        msFrame[frameTail++] = dataBuff[METER_CLEAR_TIMES + 2];
      
        //���һ������ʱ��[���ݸ�ʽ1]
        if (dataBuff[LAST_METER_CLEAR_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+1];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+2];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+3];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+5];      
      
        //���������ܴ���
        if (dataBuff[UPDATA_REQ_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[UPDATA_REQ_TIME];
        msFrame[frameTail++] = dataBuff[UPDATA_REQ_TIME+1];
        msFrame[frameTail++] = dataBuff[UPDATA_REQ_TIME+2];
      
      	//���һ������ʱ��[���ݸ�ʽ1](����)
        if (dataBuff[LAST_UPDATA_REQ_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+1];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+2];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+3];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+5];  
      
        //�¼������ܴ���
        if (dataBuff[EVENT_CLEAR_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_TIMES];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_TIMES+1];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_TIMES+2];
      
      	//���һ������ʱ��[���ݸ�ʽ1](�¼�)
        if (dataBuff[EVENT_CLEAR_LAST_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+1];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+2];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+3];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+5];
        
        //Уʱ�����ܴ���
        if (dataBuff[TIMING_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TIMING_TIMES];
        msFrame[frameTail++] = dataBuff[TIMING_TIMES+1];
        msFrame[frameTail++] = dataBuff[TIMING_TIMES+2];
      
      	//���һ������ʱ��[���ݸ�ʽ1](Уʱ)
        if (dataBuff[TIMING_LAST_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+1];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+2];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+3];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+5]; 
        
       	#ifdef NO_DATA_USE_PART_ACK_03
	       if (ifHasData == FALSE)
	       {
	         frameTail -= 64;
	       }
       	#endif     
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
       
         for(i=0;i<55;i++)
         {
         	 msFrame[frameTail++] = 0xee;
         }         
        #endif      	  
      }
 		}
 		da1 >>= 1;
 	}

  return frameTail;
}

/*******************************************************
��������:AFN0C028
��������:��Ӧ��վ����һ������"�������״̬�ּ����λ��־"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C028(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[512];
  INT8U     lastLastCopyPara[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
  INT8U     i;  
  BOOL      ifHasData;  
  DATE_TIME time;
  BOOL      buffHasData;
  INT16U    offset;
  INT8U     meterInfo[10];
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
  		
  		msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      msFrame[frameTail++] = 0x08;                                      //DT1
      msFrame[frameTail++] = 0x03;                                      //DT2

    	ifHasData = FALSE;
    	
			//��ѯ������洢��Ϣ
  		queryMeterStoreInfo(pn, meterInfo);
      
      if (meterInfo[0]<7)
      {
    		 buffHasData = readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &time, 0);
    		 if (meterInfo[0]<4)
    		 {
    		   offset = METER_STATUS_WORD_S;
    		 }
    		 else
    		 {
    		 	 offset = METER_STATUS_WORD_T;
    		 }
      }
      else
      {
    		 buffHasData = readMeterData(dataBuff, pn, meterInfo[1], PARA_VARIABLE_DATA, &time, 0);
    		 
    		 //bug,2012-07-26,����,ԭ��offsetδ��ֵ
    		 offset = METER_STATUS_WORD;
      }
    	
    	//��ȡָ�������������ҳ
      if (buffHasData == TRUE)
      {
        //���ݾ��ɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
      	
      	//�������״̬�ֱ�λ��־λ1~7
        time = queryCopyTime(pn);
        if (readMeterData(lastLastCopyPara , pn, LAST_LAST_REAL_DATA, PARA_VARIABLE_DATA, &time, 1) == TRUE)
        {      	
          meterRunWordChangeBit(&msFrame[frameTail],&dataBuff[METER_STATUS_WORD],&lastLastCopyPara[METER_STATUS_WORD]);
        }
        else
        {
      	  //�ϴγ�������û��,��ȫ0
      	  for(i=0;i<14;i++)
      	  {
      	  	msFrame[frameTail+i] = 0;
      	  }
        }
        frameTail += 14;
        
        //�������״̬��1
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //�������״̬��2
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //�������״̬��3
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //�������״̬��4
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //�������״̬��5
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //�������״̬��6
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //�������״̬��7
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
      	#ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	          frameTail -= 37;
	        }
       	#endif     
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         for(i=0;i<28;i++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif      	  
      }
  	}
  	da1 >>= 1;
  }

  return frameTail;
}

/*******************************************************
��������:AFN0C029
��������:��Ӧ��վ����һ������"��ǰͭ��/�����й��ܵ���ʾֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C029(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     result, i;	
	BOOL      ifHasData;	
	DATE_TIME time;
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);

 			msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      
      if(fn == 29)
      {
      	msFrame[frameTail++] = 0x10;                                      //DT1
      }
      else	//F30
      {
      	msFrame[frameTail++] = 0x20;                                      //DT1
      }
      msFrame[frameTail++] = 0x03;                                        //DT2
      
      ifHasData = FALSE;
      
      //��ȡ��ָ�������㡢���ͺ�ʱ�������ҳ
      if(fn == 29)
      {
      	result =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      }
      else	//F30
      {
      	result =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
      }
      
      if (result== TRUE)
      {
      	//�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
		    
		    //��ǰͭ���й��ܵ���ʾֵ
		    if(dataBuff[COPPER_LOSS_TOTAL_OFFSET] != 0xEE)
		    {
		    	ifHasData = TRUE;
		    	msFrame[frameTail++] = 0x0;
		    }
		    else
		    {
		    	msFrame[frameTail++] = 0xEE;
		    }
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET];
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET + 1];
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET + 2];
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET + 3];
		    
		    //��ǰ�����й��ܵ���ʾֵ
		    if(dataBuff[IRON_LOSS_TOTAL_OFFSET] != 0xEE)
		    {
		    	ifHasData = TRUE;
		    	msFrame[frameTail++] = 0x0;
		    }
		    else
		    {
		    	msFrame[frameTail++] = 0xEE;
		    }
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET];
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET + 1];
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET + 2];
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET + 3];
		    
		    #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	          frameTail -= 19;
	        }
       	#endif   
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
          msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
          msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
          msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
          msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
          msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
        	
        	for(i=0;i<10;i++)
        	{
        		msFrame[frameTail++] = 0xee;
        	}      
        #endif
      }
 		}
 		da1 >>= 1;
 	}

	return frameTail;
}

/*******************************************************
��������:AFN0C030
��������:��Ӧ��վ����һ������"��һ������ͭ�������й��ܵ���ʾֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C030(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C029(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C031
��������:��Ӧ��վ����һ������"��ǰA,B,C������/�����й�����ʾֵ,
					����޹�1/2����ʾֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C031(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     result, i;  
  BOOL      ifHasData;  
  DATE_TIME time;
	
	da1 = *pHandle++;
	da2 = *pHandle++;
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
  		
  		msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      
      if(fn == 31)
      {
      	msFrame[frameTail++] = 0x40;                                      //DT1
      }
      else
      {
      	msFrame[frameTail++] = 0x80;                                      //DT1
      }
      msFrame[frameTail++] = 0x03;                                        //DT2
			
			ifHasData = FALSE;
			
      //��ȡ��ָ�������㡢���ͺ�ʱ�������ҳ
      if(fn == 31)
      {
      	result =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      }
      else		//F32
      {
      	result =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
      }
      
      if(result == TRUE)
      {
        //���ݿɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
        
        //��ǰA�������й��ܵ���ʾֵ
        if(dataBuff[POSITIVE_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET + 3];
        
        //��ǰA�෴���й��ܵ���ʾֵ
        if(dataBuff[NEGTIVE_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
          msFrame[frameTail++] = 0;
        }
        else
        {
        	msFrame[frameTail++] = 0xee;
        }
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET + 3];
        
        //��ǰA������޹�1����ʾֵ
        if(dataBuff[COMB1_NO_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET + 3];
        
        //��ǰA������޹�2����ʾֵ
        if(dataBuff[COMB2_NO_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET + 3];
        
        //��ǰB�������й��ܵ���ʾֵ
        if(dataBuff[POSITIVE_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET + 3];
        
        //��ǰB�෴���й��ܵ���ʾֵ
        if(dataBuff[NEGTIVE_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET + 3];
        
        //��ǰB������޹�1����ʾֵ
        if(dataBuff[COMB1_NO_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET + 3];
        
        //��ǰB������޹�2����ʾֵ
        if(dataBuff[COMB2_NO_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET + 3];
        
        //��ǰC�������й��ܵ���ʾֵ
        if(dataBuff[POSITIVE_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET + 3];
        
        //��ǰC�෴���й��ܵ���ʾֵ
        if(dataBuff[NEGTIVE_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET + 3];
        
        //��ǰC������޹�1����ʾֵ
        if(dataBuff[COMB1_NO_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET + 3];
        
        //��ǰC������޹�2����ʾֵ
        if(dataBuff[COMB2_NO_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET + 3];
        
        #ifdef NO_DATA_USE_PART_ACK_03
        	if(ifHasData == FALSE)
        	{
        		frameTail -= 63;
        	}
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
        	frameTail -= 4;
        #else
          msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
          msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
          msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
          msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
          msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
        	
        	for(i=0;i<54;i++)
        	{
        		msFrame[frameTail++] = 0xee;
        	}
        #endif
      }
  	}
  	da1 >>= 1;
  }

  return frameTail;
}

/*******************************************************
��������:AFN0C032
��������:��Ӧ��վ����һ������"��һ������A,B,C������/�����й�����ʾֵ,
					����޹�1/2����ʾֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C032(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C031(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C033
��������:��Ӧ��վ����һ������"��ǰ�����У��޹�����ʾֵ��
         һ���������޹�����ʾֵ(�ܡ�����1~M)������ݸ�ʽ15��14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C033(INT16U frameTail,INT8U *pHandle, INT8U fn)
{	
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U    pn, tmpPn=0;
  INT8U     da1, da2;
  INT8U     tariff, tmpTariff;
  INT8U     pulsePnData = 0;
  BOOL      ifHasData; 
  BOOL      buffHasData;  
  INT16U    offSet;
  INT16U    j;  
  DATE_TIME time;
  INT8U     meterInfo[10];
	
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif  

  da1 = *pHandle++;
  da2 = *pHandle++;
    
  if(da1 == 0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
      
  		#ifdef PULSE_GATHER
	 	   //�鿴�Ƿ����������������
	  	 if (fn==33)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	     //���������Ĳ�����
	  	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	     {
	              //P.1�ȳ�ʼ������
                memset(dataBuff,0xee,LENGTH_OF_ENERGY_RECORD);
	
			 	  	 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
	          	  covertPulseData(j, dataBuff, NULL, NULL);
  		          
  		          //ly,2011-05-21,add
  		          time = timeHexToBcd(sysTime);

	   	  	 	    pulsePnData = 1;
	   	  	 	    
	   	  	 	    buffHasData = TRUE;
	   	  	 	    break;
			 	  	 }
			 	  }
	  	 }
	  	#endif
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
	        
      //fn
      if(fn == 33)
      {
      	 msFrame[frameTail++] = 0x01;    //DT1
      }
      else		//F37
      {
      	 msFrame[frameTail++] = 0x10;    //DT1
      }
      msFrame[frameTail++] = 0x4;
    	

      ifHasData = FALSE;
      
      if (pulsePnData==0)
      {
        //��ȡ���������õķ�����
        tariff = numOfTariff(pn);
      	
      	queryMeterStoreInfo(pn, meterInfo);
      	if(fn == 33)
      	{
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6 || meterInfo[0]==8)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &time, 0);
      		}
      		else
      		{
      		  if (meterInfo[0]==1)
      		  {
      		    buffHasData =  readMeterData(dataBuff, pn, SINGLE_PHASE_PRESENT, ENERGY_DATA, &time, 0);
      		  }
      		  else
      		  {
      		    if (meterInfo[0]==2)
      		    {
      		      buffHasData =  readMeterData(dataBuff, pn, SINGLE_LOCAL_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		    }
      		    else
      		    {
      		      if (meterInfo[0]==3)
      		      {
      		        buffHasData =  readMeterData(dataBuff, pn, SINGLE_REMOTE_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		      }
      		      else
      		      {
      		        //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      		        buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &time, 0);
      		      }
      		    }
      		  }
      		}
      	}
      	else		//F37
      	{
      		buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
      	}
      }
      else
      {
      	tariff = periodTimeOfCharge[48];
      	
      	if (tariff>14)
      	{
      		tariff = 0;
      	}
      }
      
      //��ȡ��ָ�������㡢���ͺ�ʱ�������ҳ
      if (buffHasData==TRUE)
      {
        //���ݾ��ɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;

        //������
        msFrame[frameTail++] =  tariff;
  
        //��ǰ�����й�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ14]
        offSet = POSITIVE_WORK_OFFSET;
        for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          //ֻҪ��һ�����ݾ������ݴ��ڱ�־Ϊtrue����ͬ
          if ((dataBuff[offSet] != 0xEE) && (dataBuff[offSet]!=0xFF))
          {
          	 ifHasData = TRUE;
             if (pulsePnData==1)
             {
               msFrame[frameTail++] = dataBuff[offSet++];
             }
             else
             {
               msFrame[frameTail++] = 0x0;
             }
          }
          else
          {
            if (pulsePnData==1)
            {
            	 offSet++;
            }
	          msFrame[frameTail++] = 0xEE;	          
          }
          
          if (dataBuff[offSet]==0xFF)
          {
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          }
          else
          {
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
          }
          
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 tmpTariff++;
          }
        }
      
        if (fn==33 && (meterInfo[0]==8 || meterInfo[0]==0x1 || meterInfo[0]==0x2 || meterInfo[0]==0x3))
        {
        	 memset(&msFrame[frameTail],0xee,(tariff+1)*12);
        	 frameTail += (tariff+1)*12;
        }
        else
        {
          //��ǰ�����޹�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ11]
          offSet = POSITIVE_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if ((dataBuff[offSet] != 0xEE) && (dataBuff[offSet]!=0xFF))
            {
        	    ifHasData = TRUE;
            }
            
            if (pulsePnData==1)
            {
            	offSet++;
            }

            if (dataBuff[offSet]==0xFF)
            {
            	msFrame[frameTail++] = 0xee;
            	msFrame[frameTail++] = 0xee;
            	msFrame[frameTail++] = 0xee;
            	msFrame[frameTail++] = 0xee;
            }
            else
            {
              msFrame[frameTail++] = dataBuff[offSet++];
              msFrame[frameTail++] = dataBuff[offSet++];
              msFrame[frameTail++] = dataBuff[offSet++];
              msFrame[frameTail++] = dataBuff[offSet++];
            }
  
            //�����ʵĴ���.����1����һ��
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //��ǰһ�����޹�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ14]
          offSet = QUA1_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //�����ʵĴ���.����1����һ��
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //��ǰ�������޹�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ11]
          offSet = QUA4_NO_WORK_OFFSET;
          for(tmpTariff = 0;tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //�����ʵĴ���.����1����һ��
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
      
  	      #ifdef NO_DATA_USE_PART_ACK_03
           if (ifHasData == FALSE)
           {
             frameTail -= (10 + (tariff+1)*17);
           }
          #endif
        }
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
          frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*17;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
  	}
  	da1 >>= 1;
  }

 	return frameTail;
}


/*******************************************************
��������:AFN0C034
��������:��Ӧ��վ����һ������"(������)��ǰ�����У��޹�����ʾֵ��
         �����������޹�����ʾֵ(�ܡ�����1~M)"������ݸ�ʽ15��14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C034(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U  dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U  da1, da2;
  INT8U  tariff, tmpTariff;
  INT8U  pulsePnData = 0;
  
  BOOL ifHasData;
  BOOL buffHasData;
  
  INT16U offSet;
  INT16U j;
  INT8U  meterInfo[10];
  
  DATE_TIME time;
	
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif
  
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);

 			#ifdef PULSE_GATHER
			 	//�鿴�Ƿ����������������
			  if (fn==34)
			  {
			      for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  	  {
			  	     //���������Ĳ�����
			  	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
			  	     {
			            //P.1�ȳ�ʼ������
                  memset(dataBuff,0xee,LENGTH_OF_ENERGY_RECORD);

				 	  	 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
			        	  covertPulseData(j, dataBuff, NULL, NULL);
  		            
  		            //ly,2011-05-21,add
  		            time = timeHexToBcd(sysTime);

			    	 	    pulsePnData = 1;
			   	  	 	    
			    	 	    buffHasData = TRUE;
			    	 	    break;
					  	 }
					  }
			  }
		  #endif
  

      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      
      if (fn == 34)
      {
        msFrame[frameTail++] = 0x02;     //DT1
      }
      else		//F38
      {
      	msFrame[frameTail++] = 0x20;     //DT1
      }
      msFrame[frameTail++] = 0x4;    //DT2
            	 
      if (pulsePnData==0)
      {
        //��ȡ���������õķ�����
        tariff = numOfTariff(pn);

      	queryMeterStoreInfo(pn, meterInfo);
        if(fn == 34)
        {
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6 || meterInfo[0]==8)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &time, 0);
      		}
      		else
      		{
      		  if (meterInfo[0]==1)
      		  {
      		    buffHasData =  readMeterData(dataBuff, pn, SINGLE_PHASE_PRESENT, ENERGY_DATA, &time, 0);
      		  }
      		  else
      		  {
      		    if (meterInfo[0]==2)
      		    {
      		      buffHasData =  readMeterData(dataBuff, pn, SINGLE_LOCAL_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		    }
      		    else
      		    {
      		      if (meterInfo[0]==3)
      		      {
      		        buffHasData =  readMeterData(dataBuff, pn, SINGLE_REMOTE_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		      }
      		      else
      		      {
      		        //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      		        buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &time, 0);
      		      }
      		    }
      		  }
      		  printf("buffHasData=%d\n", buffHasData);
      		}
        }
        else	//F38
        {
        	buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
        }
      }
      else
      {
      	tariff = periodTimeOfCharge[48];
      	
      	if (tariff>14)
      	{
      		 tariff = 0;
      	}
      }
      
      //��ȡ��ָ�������㡢���ͺ�ʱ�������ҳ
      if (buffHasData==TRUE)
      {
        //���ݾ��ɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;

        //������
        msFrame[frameTail++] =  tariff;
  
        //��ǰ�����й�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ14]
        if (meterInfo[0]<4)
        {
          offSet = NEGTIVE_WORK_OFFSET_S;
        }
        else
        {
          offSet = NEGTIVE_WORK_OFFSET;
        }
        for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          //ֻҪ��һ�����ݾ������ݴ��ڱ�־Ϊtrue����ͬ
          if ((dataBuff[offSet] != 0xEE) && (dataBuff[offSet] != 0xFF))
          {
          	ifHasData = TRUE;
          	if (pulsePnData==1)
          	{
              msFrame[frameTail++] = dataBuff[offSet++];
          	}
          	else
          	{
              msFrame[frameTail++] = 0x0;
            }
          }
          else
          {
          	msFrame[frameTail++] = 0xEE;
            
            if (pulsePnData==1)
            {
          	  offSet++;
          	}
          }
          
          if  (dataBuff[offSet] == 0xFF)
          {
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	
          	offSet += 4;
          }
          else
          {
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
          }

          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 tmpTariff++;
          }
        }
      
        if (fn==34 && (meterInfo[0]==8 || meterInfo[0]==0x1 || meterInfo[0]==0x2 || meterInfo[0]==0x3))
        {
        	 memset(&msFrame[frameTail],0xee,(tariff+1)*12);
        	 frameTail += (tariff+1)*12;
        }
        else
        {
          //��ǰ�����޹�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ11]
          offSet = NEGTIVE_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            
          	if (pulsePnData==1)
            {
            	offSet++;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //�����ʵĴ���.����1����һ��
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //��ǰ�������޹�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ14]
          offSet = QUA2_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //�����ʵĴ���.����1����һ��
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //��ǰ�������޹�����ʾֵ(�ܡ�����1��m)[���ݸ�ʽ11]
          offSet = QUA3_NO_WORK_OFFSET;
          for(tmpTariff = 0;tmpTariff <= tariff;tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            
            //�����ʵĴ���.����1����һ��
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
      
          #ifdef NO_DATA_USE_PART_ACK_03
           if (ifHasData == FALSE)
           {
             frameTail -= (10 + (tariff+1)*17);
           }
          #endif
        }
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03 
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*17;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
 		}
 		da1 >>= 1;
 	}

 	return frameTail;
}

/*******************************************************
��������:AFN0C035
��������:��Ӧ��վ����һ������"(������)���������У��޹��������
         ������ʱ��(�ܡ�����1��M)"������ݸ�ʽ15��23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C035(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U dataBuff[LENGTH_OF_REQ_RECORD];
  INT16U pn, tmpPn = 0;
  INT8U da1, da2;
  INT8U tariff, tmpTariff;
  INT8U pulsePnData = 0;
  
  BOOL ifHasData; 
  BOOL buffHasData;    
  
  INT16U offSet;
  INT16U j;
  INT8U  meterInfo[10];
  
  DATE_TIME  time;
  
  #ifdef     PULSE_GATHER
    INT16U   counti;
  #endif
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
 			
 			#ifdef PULSE_GATHER
	 	   //�鿴�Ƿ����������������
	  	 if (fn==35)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	     //���������Ĳ�����
	  	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	     {
	              //P.1�ȳ�ʼ������
                memset(dataBuff,0xee,LENGTH_OF_REQ_RECORD);
	
			 	  	 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
	          	  covertPulseData(j, NULL,dataBuff,NULL);
  		          
  		          //ly,2011-05-21,add
  		          time = timeHexToBcd(sysTime);

	   	  	 	    pulsePnData = 1;
	   	  	 	    
	   	  	 	    buffHasData = TRUE;
	   	  	 	    break;
			 	  	 }
			 	  }
	  	 }
	  	#endif
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  //DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                    //DA2
	        
      if (fn == 35)
      {
        msFrame[frameTail++] = 0x04;    //DT1
      }
      else		//F39
      {
      	msFrame[frameTail++] = 0x40;    //DT1
      }
      msFrame[frameTail++] = 0x4;     //DT2
    
      //��ȡ���������õķ�����
      tariff = numOfTariff(pn);
      	
      ifHasData = FALSE;
      
      if (pulsePnData==0)
      {
      	if(fn == 35)
      	{
      		queryMeterStoreInfo(pn, meterInfo);
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], REQ_REQTIME_DATA, &time, 0);
      		}
      		else
      		{
      		  //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, REQ_REQTIME_DATA, &time, 0);
      		  buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, REQ_REQTIME_DATA, &time, 0);
      		}
      	}
      	else		//F39
      	{
      		buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, REQ_REQTIME_LASTMONTH, &time, 0);
      	}
      }
      
      if (buffHasData==TRUE)
      {
        //���ݾ��ɴ�ԭʼ���ݼ�¼�ж�ȡ
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
      
      	//������
        msFrame[frameTail++] = tariff;
        
        //���������й��������
        offSet = REQ_POSITIVE_WORK_OFFSET;
        for(tmpTariff=0; tmpTariff<=tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];

          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
        
        //���������й������������ʱ��(�ܡ�����1��M)[���ݸ�ʽ17]
        offSet = REQ_TIME_P_WORK_OFFSET;
        for(tmpTariff=0; tmpTariff<=tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
        
        //���������޹��������
        offSet = REQ_POSITIVE_NO_WORK_OFFSET;
        for(tmpTariff=0;tmpTariff<=tariff;tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
        
        //���������޹������������ʱ��(�ܡ�����1��M)[���ݸ�ʽ17]
        offSet = REQ_TIME_P_NO_WORK_OFFSET;
        for(tmpTariff=0;tmpTariff<=tariff;tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff+1)*14);
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*14;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
 		}
 		da1 >>= 1;
 	}

	return frameTail;
}

/*******************************************************
��������:AFN0C036
��������:��Ӧ��վ����һ������"(������)���·����У��޹��������
         ������ʱ��(�ܡ�����1��M)"������ݸ�ʽ15��23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C036(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U dataBuff[LENGTH_OF_REQ_RECORD];
  INT16U pn, tmpPn = 0;
  INT8U da1, da2;
  INT8U tariff, tmpTariff;
  INT8U pulsePnData = 0;
  
  BOOL ifHasData;
  BOOL buffHasData;
  
  INT16U offSet;
  INT16U j;
  INT8U  meterInfo[10];
  
  DATE_TIME time;
  
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif
	
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
			
			#ifdef PULSE_GATHER
	 	   //�鿴�Ƿ����������������
	  	 if (fn==36)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	     //���������Ĳ�����
	  	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	     {
	              //P.1�ȳ�ʼ������
                memset(dataBuff,0xee,LENGTH_OF_REQ_RECORD);
	
			 	  	 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
	          	  covertPulseData(j, NULL,dataBuff,NULL);

  		          //ly,2011-05-21,add
  		          time = timeHexToBcd(sysTime);

	   	  	 	    pulsePnData = 1;
	   	  	 	    
	   	  	 	    buffHasData = TRUE;
	   	  	 	    break;
			 	  	 }
			 	  }
	  	 }
	  	#endif
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1)); 		 //DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                       //DA2

      if (fn == 36)
      {
        msFrame[frameTail++] = 0x08;    //DT1
      }
      else		//F40
      {
      	msFrame[frameTail++] = 0x80;    //DT1
      }
      msFrame[frameTail++] = 0x4;    //DT2

      //��ȡ���������õķ�����
      tariff = numOfTariff(pn);
      
      if (pulsePnData == 0)
      {
      	if(fn == 36)
      	{
      		queryMeterStoreInfo(pn, meterInfo);
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], REQ_REQTIME_DATA, &time, 0);
      		}
      		else
      		{
      		  //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, REQ_REQTIME_DATA, &time, 0);
      		  buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, REQ_REQTIME_DATA, &time, 0);
      		}
      	}
      	else		//F40
      	{
      		buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, REQ_REQTIME_LASTMONTH, &time, 0);
      	}
      }
      
      if (buffHasData==TRUE)
      { 
        //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
    
        //������
        msFrame[frameTail++] =  tariff;

     	  //���·����й����������
        offSet = REQ_NEGTIVE_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];

          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
       
        //���·����й��������������ʱ��
        offSet = REQ_TIME_N_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
       
        //���·����޹����������
        offSet = REQ_NEGTIVE_NO_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
       
        //���·����޹��������������ʱ��
        offSet = REQ_TIME_N_NO_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff+1)*14);
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*14;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C037
��������:��Ӧ��վ����һ������"����(��һ������)������/�޹�����ʾֵ
					��һ/�������޹�����ʾֵ(�ܣ�����1~M)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C037(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C033(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C038
��������:��Ӧ��վ����һ������"����(��һ������)������/�޹�����ʾֵ
					����/�������޹�����ʾֵ(�ܣ�����1~M)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C038(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C034(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C039
��������:��Ӧ��վ����һ������"����(��һ������)������/�޹��������
					������ʱ��(�ܣ�����1~M)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C039(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C035(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C040
��������:��Ӧ��վ����һ������"����(��һ������)������/�޹��������
					������ʱ��(�ܣ�����1~M)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C040(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C036(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C041
��������:��Ӧ��վ����һ������"���������й�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C041(INT16U frameTail, INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     tariff, tmpTariff;
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    offSet;
  INT16U    j;
  DATE_TIME time;
  
  #ifdef PULSE_GATHER
   INT8U    pulsePnData = 0;
  #endif
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		 return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		#ifdef PULSE_GATHER
	 	   //�鿴�Ƿ����������������
       for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
 	 	   {
 	 	     //���������Ĳ�����
 	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
 	 	     {
  	  	 	  pulsePnData = 1;
  	  	 	  break;
	 	  	 }
	 	   }
	  	#endif
	  	
  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 41:
      		msFrame[frameTail++] = 0x01;			//DT1
      		offSet = DAY_P_WORK_OFFSET;				//���������й�������(�ܣ�����1~M)
      		break;
      	case 42:
      		msFrame[frameTail++] = 0x02;			//DT1
      		offSet = DAY_P_NO_WORK_OFFSET;		//���������޹�������(�ܣ�����1~M)
      		break;
      	case 43:
      		msFrame[frameTail++] = 0x04;			//DT1
      		offSet = DAY_N_WORK_OFFSET;		    //���շ����й�������(�ܣ�����1~M)
      		break;
      	case 44:
      		msFrame[frameTail++] = 0x08;			//DT1
      		offSet = DAY_N_NO_WORK_OFFSET;		//���շ����޹�������(�ܣ�����1~M)
      		break;
      	case 45:
      		msFrame[frameTail++] = 0x10;			//DT1
      		offSet = MONTH_P_WORK_OFFSET;			//���������й�������(�ܣ�����1~M)
      		break;
      	case 46:
      		msFrame[frameTail++] = 0x20;			//DT1
      		offSet = MONTH_P_NO_WORK_OFFSET;	//���������޹�������(�ܣ�����1~M)
      		break;
      	case 47:
      		msFrame[frameTail++] = 0x40;			//DT1
      		offSet = MONTH_N_WORK_OFFSET;	    //���·����й�������(�ܣ�����1~M)
      		break;
      	case 48:
      		msFrame[frameTail++] = 0x80;			//DT1
      		offSet = MONTH_N_NO_WORK_OFFSET;	//���·����޹�������(�ܣ�����1~M)
      		break;
      }
			msFrame[frameTail++] = 0x05;					//DT2
			
			//ȡ�÷�����
			tariff = numOfTariff(pn);
			
			ifHasData = FALSE;
      
      #ifdef PULSE_GATHER
  	   if (pulsePnData==1)
  	   {
         buffHasData =  readMeterData(dataBuff, pn, LAST_REAL_BALANCE, REAL_BALANCE_POWER_DATA, &time, 0);
       }
       else
       {  	   	 
      #endif
      
      buffHasData =  readMeterData(dataBuff, pn, REAL_BALANCE, REAL_BALANCE_POWER_DATA, &time, 0);
      
      #ifdef PULSE_GATHER
       }
      #endif
      
      if (buffHasData==TRUE)
      {
        //������
        msFrame[frameTail++] = tariff;
        
        for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
        	if(dataBuff[offSet + 1] != 0xEE)
        	{
        		ifHasData = TRUE;
        	}
        	offSet++;
        	msFrame[frameTail++] = dataBuff[offSet++];
        	msFrame[frameTail++] = dataBuff[offSet++];
        	msFrame[frameTail++] = dataBuff[offSet++];
        	msFrame[frameTail++] = dataBuff[offSet++];
        	
          //�����ʵĴ���.����1����һ��
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
        	   offSet += 2;
        	}
        }
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff+1)*4);
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*4;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C042
��������:��Ӧ��վ����һ������"���������޹�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C042(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C043
��������:��Ӧ��վ����һ������"���շ����й�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C043(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C044
��������:��Ӧ��վ����һ������"���շ����޹�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C044(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C045
��������:��Ӧ��վ����һ������"���������й�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C045(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C046
��������:��Ӧ��վ����һ������"���������޹�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C046(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C047
��������:��Ӧ��վ����һ������"���·����й�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C047(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C048
��������:��Ӧ��վ����һ������"���·����޹�������(�ܣ�����1~M)"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C048(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C049
��������:��Ӧ��վ����һ������"��ǰ��ѹ��������λ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C049(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG  meterConfig;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT8U     tmpPn, pn;
  INT8U     i, j;
  INT8U     da1, da2;
  INT8U     pulsePnData = 0;  
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    tmpTail,counti;  
  DATE_TIME time;
	
  tmpPn = 0;
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);

	  	buffHasData = FALSE;
	  	
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
	      if (meterConfig.protocol==AC_SAMPLE)
	      {
	     	  if (ifHasAcModule==TRUE)
	     	  {
		        //A.1�ȳ�ʼ������
		        for (counti = 0; counti < LENGTH_OF_PARA_RECORD;counti++)
		        {
		          dataBuff[counti] = 0xEE;
		        }
		   
		       	//A.2������������������dataBuff��
		       	covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
		      	       	  
		     	  buffHasData = TRUE;
	   	    }
	      }
	  	  else
	  	  {
	  	    buffHasData =  readMeterData(dataBuff, pn , PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0);
	  	  }
	  	}
		    	
		  msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
		  msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
		  msFrame[frameTail++] = 0x1;                                       //DT1
		  msFrame[frameTail++] = 0x6;                                       //DT2
		
		  ifHasData = FALSE;
		  if (buffHasData == TRUE)
		  {
		     //A���ѹ��λ��
		     if (dataBuff[PHASE_ANGLE_V_A] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_A];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_A+1];

		     //B���ѹ��λ��
		     if (dataBuff[PHASE_ANGLE_V_B] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_B];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_B+1];
		       
		     //C���ѹ��λ��
		     if (dataBuff[PHASE_ANGLE_V_C] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_C];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_C+1];
		     
		     //A�������λ��
		     if (dataBuff[PHASE_ANGLE_C_A] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_A];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_A+1];

		     //B�������λ��
		     if (dataBuff[PHASE_ANGLE_C_B] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_B];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_B+1];
		       
		     //C�������λ��
		     if (dataBuff[PHASE_ANGLE_C_C] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_C];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_C+1];
			   
			   #ifdef DKY_SUBMISSION
			    //2010-12-17,���ݲ��Է���û�г����������,��������00����ͨ���ع�˾�ٳ����
			    if (ifHasData==FALSE)
			    {
			      frameTail-=12;
			     
			      for(j=0;j<12;j++)
			      {
			      	msFrame[frameTail++] = 0x00;
			      }
			    }
			   #endif
		  }
		  else
		  {
		    #ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    for(j=0;j<12;j++)
			    {
			     #ifdef DKY_SUBMISSION
			  	  //2010-12-17,���ݲ��Է����������,��������00����ͨ���ع�˾�ٳ����
			  	  msFrame[frameTail++] = 0x00;
			     #else
			  	  msFrame[frameTail++] = 0xee;
			  	 #endif
			    }
		    #endif
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0C057
��������:��Ӧ��վ����һ������"��ǰA��B��C�����ѹ������2~N��г����Чֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C057(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}
/*******************************************************
��������:AFN0C058
��������:��Ӧ��վ����һ������"��ǰA��B��C�����ѹ������2~N��г��������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C058(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C065
��������:��Ӧ��վ����һ������"��ǰ������Ͷ��״̬"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C065(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C066
��������:��Ӧ��վ����һ������"��ǰ�������ۼ�Ͷ��ʱ��ʹ���"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C066(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C067
��������:��Ӧ��վ����һ������"���ա��µ������ۼƲ������޹�������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C067(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C073
��������:��Ӧ��վ����һ������"ֱ��ģ����ʵʱ����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C073(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	#ifdef PLUG_IN_CARRIER_MODULE
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = 0x01;       //DA1,Pn1
   msFrame[frameTail++] = 0x01;       //DA2
   msFrame[frameTail++] = 0x01;       //DT1
   msFrame[frameTail++] = 0x09;       //DT2
   
   //
   msFrame[frameTail++] = hexToBcd(adcData);
   msFrame[frameTail++] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
   
	#endif
	
	return frameTail;
}

/*******************************************************
��������:AFN0C081
��������:��Ӧ��վ����һ������"Сʱ�����ܼ��й�����"������ݸ�ʽ2��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C081(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
  INT16U pn, tmpPn = 0;
  INT8U da1, da2;
  INT8U density, interval, tmpMinute;
  
  BOOL ifHasData;
  BOOL buffHasData;
  
  INT16U offSet;
  INT16U i;
  
  DATE_TIME time, readTime;
  
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			//�ж��ܼ����
			if(pn > 8)
			{
				return frameTail;
			}
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 81:
      		msFrame[frameTail++] = 0x01;   															//DT1
      		offSet = GP_WORK_POWER;
      		break;
      	case 82:
      		msFrame[frameTail++] = 0x02;   															//DT1
      		offSet = GP_NO_WORK_POWER;
      		break;
      	case 83:
      		msFrame[frameTail++] = 0x04;   															//DT1
      		offSet = GP_DAY_WORK;
      		break;
      	case 84:
      		msFrame[frameTail++] = 0x08;   															//DT1
      		offSet = GP_DAY_NO_WORK;
      		break;
      }
      msFrame[frameTail++] = 0xA;    																	//DT2
			
			//��һ����ʱ��
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//Сʱ����������ʱ��Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//Сʱʱ��
			density = 0x1;
			msFrame[frameTail++] = density;			//�����ܶ�
			
			
			switch(density)
			{
				case 0:		//������
					interval = 61;
					break;
				case 1:		//������ʱ��15��
					interval = 15;
					break;
				case 2:		//������ʱ��30��
					interval = 30;
					break;
				case 3:		//������ʱ��60��
					interval = 60;
					break;
				case 254:	//������ʱ��5��
					interval = 5;
					break;
				case 255:	//������ʱ��1��
					interval = 1;
					break;
			}
			
			//��Ч���ݵ��ж�
			if(interval > 60)
			{
				return frameTail -= 6;
			}

			time = backTime(time, 0, 0, 0, interval, 0);

			//��ѯ����
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = interval * (i + 1) / 10 << 4 | interval * (i + 1) % 10;
				tmpMinute = hexToBcd(interval);
				
				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, GROUP_REAL_BALANCE, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
					if(fn == 81 || fn == 82)
					{
						msFrame[frameTail++] = dataBuff[offSet+1];
	          msFrame[frameTail++] = ((dataBuff[offSet]&0x7)<<5) | (dataBuff[offSet]&0x10)
	                                | (dataBuff[offSet+2]&0xf);
          }
          
          if(fn == 83 || fn == 84)
          {
            msFrame[frameTail++] = dataBuff[offSet+3];
            msFrame[frameTail++] = dataBuff[offSet+4];
            msFrame[frameTail++] = dataBuff[offSet+5];
            msFrame[frameTail++] = ((dataBuff[offSet]&0x1)<<6) | (dataBuff[offSet]&0x10)
                                 | (dataBuff[offSet+6]&0xf);
          }
				}
				else
				{
					if(fn == 81 || fn == 82)
					{
						msFrame[frameTail++] = 0xEE;
	          msFrame[frameTail++] = 0xEE;
          }
          
          if(fn == 83 || fn == 84)
          {
            msFrame[frameTail++] = 0xEE;
            msFrame[frameTail++] = 0xEE;
            msFrame[frameTail++] = 0xEE;
            msFrame[frameTail++] = 0xEE;
          }
				}
				
				//ÿ������ʱ�̶�������������ڸ�ʱ��֮ǰ�ɼ���
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C082
��������:��Ӧ��վ����һ������"Сʱ�����ܼ��޹�����"������ݸ�ʽ2��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C082(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C081(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C083
��������:��Ӧ��վ����һ������"Сʱ�����ܼ��й��ܵ�����"������ݸ�ʽ3��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C083(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C081(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C084
��������:��Ӧ��վ����һ������"Сʱ�����ܼ��޹��ܵ�����"������ݸ�ʽ3��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C084(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C081(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C089
��������:��Ӧ��վ����һ������"Сʱ�����й�����"������ݸ�ʽ9,7,25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C089(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, interval, tmpMinute;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet;
  INT16U    i;
  
  DATE_TIME time, readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 89:		//Сʱ�����й�����
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_INSTANT_WORK;
      		break;
      		
      	case 90:		//Сʱ����A���й�����
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_A_WORK;
      		break;
      		
      	case 91:		//Сʱ����B���й�����
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_B_WORK;
      		break;
      		
      	case 92:		//Сʱ����C���й�����
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_C_WORK;
      		break;
      		
      	case 93:		//Сʱ�����޹�����
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_INSTANT_NO_WORK;
      		break;
      		
      	case 94:		//Сʱ����A���޹�����
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_A_NO_WORK;
      		break;
      		
      	case 95:		//Сʱ����B���޹�����
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_B_NO_WORK;
      		break;
      		
      	case 96:		//Сʱ����C���޹�����
      		msFrame[frameTail++] = 0x80;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_C_NO_WORK;
      		break;
      		
      	case 97:		//Сʱ����A���ѹ
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = VOLTAGE_PHASE_A;
      		break;
      		
      	case 98:		//Сʱ����B���ѹ
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = VOLTAGE_PHASE_B;
      		break;
      		
      	case 99:		//Сʱ����C���ѹ
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = VOLTAGE_PHASE_C;
      		break;
      		
      	case 100:		//Сʱ����A�����
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = CURRENT_PHASE_A;
      		break;
      		
      	case 101:		//Сʱ����B�����
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = CURRENT_PHASE_B;
      		break;
      		
      	case 102:		//Сʱ����C�����
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = CURRENT_PHASE_C;
      		break;
      		
      	case 103:		//Сʱ�����������
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = ZERO_SERIAL_CURRENT;
      		break;
      		
      	case 113:		//Сʱ�����ܹ�������
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = TOTAL_POWER_FACTOR;
      		break;
      		
      	case 114:		//Сʱ����A�๦������
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = FACTOR_PHASE_A;
      		break;
      		
      	case 115:		//Сʱ����B�๦������
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = FACTOR_PHASE_B;
      		break;
      		
      	case 116:		//Сʱ����C�๦������
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = FACTOR_PHASE_C;
      		break;
      }
      
			//��һ����ʱ��
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//Сʱ����������ʱ��Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//Сʱʱ��
			density = 0x1;
			msFrame[frameTail++] = density;			//�����ܶ�
			
			switch(density)
			{
				case 0:		//������
					interval = 61;
					break;
					
				case 1:		//������ʱ��15��
					interval = 15;
					break;
				case 2:		//������ʱ��30��
					interval = 30;
					break;
				case 3:		//������ʱ��60��
					interval = 60;
					break;
				case 254:	//������ʱ��5��
					interval = 5;
					break;
				case 255:	//������ʱ��1��
					interval = 1;
					break;
			}
			
			//��Ч���ݵ��ж�
			if(interval > 60)
			{
				return frameTail -= 6;
			}
			
			time = backTime(time, 0, 0, 0, interval, 0);

			//��ѯ����
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(interval+i);
				tmpMinute = hexToBcd(interval);
				
				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
					if((fn >= 89 && fn <= 96) || (fn >= 100 && fn <= 103))	      //���ݸ�ʽ9,25
					{
						msFrame[frameTail++] = dataBuff[offSet];
	          msFrame[frameTail++] = dataBuff[offSet+1];
	          msFrame[frameTail++] = dataBuff[offSet+2];
					}
					else if((fn >= 97 && fn <= 99) || (fn >= 113 && fn <= 116))		//���ݸ�ʽ7
	        {
	        	msFrame[frameTail++] = dataBuff[offSet];
		     		msFrame[frameTail++] = dataBuff[offSet+1];
	        }
				}
				else
				{
					if((fn >= 89 && fn <= 96) || (fn >= 100 && fn <= 103))
					{
						msFrame[frameTail++] = 0xEE;
						msFrame[frameTail++] = 0xEE;
						msFrame[frameTail++] = 0xEE;
					}
					else if((fn >= 97 && fn <= 99) || (fn >= 113 && fn <= 116))
	        {
	        	msFrame[frameTail++] = 0xEE;
		     		msFrame[frameTail++] = 0xEE;
	        }
				}
				
				//ÿ������ʱ�̶�������������ڸ�ʱ��֮ǰ�ɼ���
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C090
��������:��Ӧ��վ����һ������"Сʱ����A���й�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C090(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C091
��������:��Ӧ��վ����һ������"Сʱ����B���й�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C091(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C092
��������:��Ӧ��վ����һ������"Сʱ����C���й�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C092(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C093
��������:��Ӧ��վ����һ������"Сʱ�����޹�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C093(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C094
��������:��Ӧ��վ����һ������"Сʱ����A���޹�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C094(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C095
��������:��Ӧ��վ����һ������"Сʱ����B���޹�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C095(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C096
��������:��Ӧ��վ����һ������"Сʱ����C���޹�����"������ݸ�ʽ9��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C096(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C097
��������:��Ӧ��վ����һ������"Сʱ����A���ѹ"������ݸ�ʽ7��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C097(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C098
��������:��Ӧ��վ����һ������"Сʱ����B���ѹ"������ݸ�ʽ7��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C098(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C099
��������:��Ӧ��վ����һ������"Сʱ����C���ѹ"������ݸ�ʽ7��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C099(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C100
��������:��Ӧ��վ����һ������"Сʱ����A�����"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C100(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C101
��������:��Ӧ��վ����һ������"Сʱ����B�����"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C101(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C102
��������:��Ӧ��վ����һ������"Сʱ����C�����"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C102(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C103
��������:��Ӧ��վ����һ������"Сʱ�����������"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C103(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C105
��������:��Ӧ��վ����һ������"Сʱ���������й��ܵ�����"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C105(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, interval, tmpMinute;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet;
  INT16U    i;
  
  DATE_TIME time,readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 105:		//Сʱ���������й��ܵ�����
      		msFrame[frameTail++] = 0x01;   															//DT1
      		offSet = DAY_P_WORK_OFFSET;
      		break;
      		
      	case 106:		//Сʱ���������޹��ܵ�����
      		msFrame[frameTail++] = 0x02;   															//DT1
      		offSet = DAY_P_NO_WORK_OFFSET;
      		break;
      		
      	case 107:		//Сʱ���ᷴ���й��ܵ�����
      		msFrame[frameTail++] = 0x04;   															//DT1
      		offSet = DAY_N_WORK_OFFSET;
      		break;
      		
      	case 108:		//Сʱ���ᷴ���޹��ܵ�����
      		msFrame[frameTail++] = 0x08;   															//DT1
      		offSet = DAY_N_NO_WORK_OFFSET;
      		break;
      }
      msFrame[frameTail++] = 0xD;    																	//DT2
      
			//��һ����ʱ��
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//Сʱ����������ʱ��Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//Сʱʱ��
			density = 0x1;
			msFrame[frameTail++] = density;			//�����ܶ�
			
			switch(density)
			{
				case 0:		//������
					interval = 61;
					break;
				case 1:		//������ʱ��15��
					interval = 15;
					break;
				case 2:		//������ʱ��30��
					interval = 30;
					break;
				case 3:		//������ʱ��60��
					interval = 60;
					break;
				case 254:	//������ʱ��5��
					interval = 5;
					break;
				case 255:	//������ʱ��1��
					interval = 1;
					break;
			}
			
			//��Ч���ݵ��ж�
			if(interval > 60)
			{
				return frameTail -= 6;
			}

			time = backTime(time, 0, 0, 0, interval, 0);

			//��ѯ����
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(interval+i);
				tmpMinute = hexToBcd(interval);

				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
					msFrame[frameTail++] = dataBuff[offSet + 1];
	        msFrame[frameTail++] = dataBuff[offSet + 2];
	        msFrame[frameTail++] = dataBuff[offSet + 3];
	        msFrame[frameTail++] = dataBuff[offSet + 4];
				}
				else
				{
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
				}
				
				//ÿ������ʱ�̶�������������ڸ�ʱ��֮ǰ�ɼ���
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C106
��������:��Ӧ��վ����һ������"Сʱ���������޹��ܵ�����"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C106(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C105(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C107
��������:��Ӧ��վ����һ������"Сʱ���ᷴ���й��ܵ�����"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C107(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C105(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C108
��������:��Ӧ��վ����һ������"Сʱ���ᷴ���޹��ܵ�����"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C108(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C105(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C109
��������:��Ӧ��վ����һ������"Сʱ���������й��ܵ���ʾֵ"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C109(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, interval, tmpMinute;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet;
  INT16U    i;
  
  DATE_TIME time, readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 109:		//Сʱ���������й��ܵ���ʾֵ
      		msFrame[frameTail++] = 0x10;   															//DT1
      		offSet = POSITIVE_WORK_OFFSET;
      		break;
      	case 110:		//Сʱ���������޹��ܵ���ʾֵ
      		msFrame[frameTail++] = 0x20;   															//DT1
      		offSet = POSITIVE_NO_WORK_OFFSET;
      		break;
      	case 111:		//Сʱ���ᷴ���й��ܵ���ʾֵ
      		msFrame[frameTail++] = 0x40;   															//DT1
      		offSet = NEGTIVE_WORK_OFFSET;
      		break;
      	case 112:		//Сʱ���ᷴ���޹��ܵ���ʾֵ
      		msFrame[frameTail++] = 0x80;   															//DT1
      		offSet = NEGTIVE_NO_WORK_OFFSET;
      		break;
      }
      msFrame[frameTail++] = 0xD;    																	//DT2
      
			//��һ����ʱ��
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//Сʱ����������ʱ��Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//Сʱʱ��
			density = 0x1;
			msFrame[frameTail++] = density;			//�����ܶ�
			
			switch(density)
			{
				case 0:		//������
					interval = 61;
					break;
				case 1:		//������ʱ��15��
					interval = 15;
					break;
				case 2:		//������ʱ��30��
					interval = 30;
					break;
				case 3:		//������ʱ��60��
					interval = 60;
					break;
				case 254:	//������ʱ��5��
					interval = 5;
					break;
				case 255:	//������ʱ��1��
					interval = 1;
					break;
			}
			
			//��Ч���ݵ��ж�
			if(interval > 60)
			{
				return frameTail -= 6;
			}

			time = backTime(time, 0, 0, 0, interval, 0);

			//��ѯ����
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(interval+i);
				tmpMinute = hexToBcd(interval);
				
				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, ENERGY_DATA, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
	        msFrame[frameTail++] = dataBuff[offSet];
					msFrame[frameTail++] = dataBuff[offSet + 1];
	        msFrame[frameTail++] = dataBuff[offSet + 2];
	        msFrame[frameTail++] = dataBuff[offSet + 3];
				}
				else
				{
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
				}
				
				//ÿ������ʱ�̶�������������ڸ�ʱ��֮ǰ�ɼ���
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C110
��������:��Ӧ��վ����һ������"Сʱ���������޹��ܵ���ʾֵ"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C110(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C109(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C111
��������:��Ӧ��վ����һ������"Сʱ���ᷴ���й��ܵ���ʾֵ"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C111(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C109(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C112
��������:��Ӧ��վ����һ������"Сʱ���ᷴ���޹��ܵ���ʾֵ"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C112(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C109(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C113
��������:��Ӧ��վ����һ������"Сʱ�����ܹ�������"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C113(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C114
��������:��Ӧ��վ����һ������"Сʱ����A�๦������"������ݸ�ʽ5��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C114(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C115
��������:��Ӧ��վ����һ������"Сʱ����B�๦������"������ݸ�ʽ5��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C115(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C116
��������:��Ӧ��վ����һ������"Сʱ����C�๦������"������ݸ�ʽ5��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C116(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C121
��������:��Ӧ��վ����һ������"Сʱ����ֱ��ģ����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C121(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
 	#ifdef PLUG_IN_CARRIER_MODULE 
 	 INT8U     dataBuff[5];
   INT16U    pn, tmpPn = 0;
   INT8U     da1, da2;
   INT8U     density, interval, tmpMinute;
   
   BOOL      ifHasData;
   BOOL      buffHasData;
   
   INT16U    i;
   
   DATE_TIME time, readTime;
 	
   da1 = *pHandle;
   da2 = *(pHandle+1);
 	
 	 if(da1 == 0x0)
 	 {
 		 return frameTail;
 	 }
 	
 	 while(tmpPn < 9)
 	 {
 		 tmpPn++;
 		 if((da1 & 0x01) == 0x01)
 		 {
 			 pn = tmpPn + (da2 - 1) * 8;
 			
       msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
       msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
       msFrame[frameTail++] = 0x01;   															//DT1
       msFrame[frameTail++] = 0x0F;    															//DT2

 			 //��һ����ʱ��
 			 time = backTime(sysTime, 0, 0, 1, 0, 0);
 			 time.minute = 0;
 			 time.second = 0;
 			
 			 //Сʱ����������ʱ��Td_h
 			 msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//Сʱʱ��
 			 density = 0x1;
 			 msFrame[frameTail++] = density;			//�����ܶ�
 			
 			 switch(density)
 			 {
 				 case 0:		//������
 					 interval = 61;
 					 break;
 				 case 1:		//������ʱ��15��
 					 interval = 15;
 					 break;
 				 case 2:		//������ʱ��30��
 					 interval = 30;
 					 break;
 				 case 3:		//������ʱ��60��
 					 interval = 60;
 					 break;
 				 case 254:	//������ʱ��5��
 					 interval = 5;
 					 break;
 				 case 255:	//������ʱ��1��
 					 interval = 1;
 					 break;
 			 }
 			
 			 //��Ч���ݵ��ж�
 			 if(interval > 60)
 			 {
 				 return frameTail -= 6;
 			 }
			 
			 time = backTime(time, 0, 0, 0, interval, 0);

 			 //��ѯ����
 			 for(i = 0; i < 60; i += interval)
 			 {
				 //2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				 //    ԭ��:��ȡ��������ʱ����readMeterData��������
				 //tmpMinute = hexToBcd(interval+i);
				 tmpMinute = hexToBcd(interval);

 				 readTime = timeHexToBcd(time);
 				 buffHasData = readMeterData(dataBuff, pn, DC_ANALOG, DC_ANALOG_CURVE_DATA, &readTime, tmpMinute);
 				 
 				 if(buffHasData == TRUE)
 				 {
 					 msFrame[frameTail++] = dataBuff[0];
 	         msFrame[frameTail++] = dataBuff[1];
 				 }
 				 else
 				 {
 					 msFrame[frameTail++] = 0xEE;
 				   msFrame[frameTail++] = 0xEE;
 				 }
 				
 				 //ÿ������ʱ�̶�������������ڸ�ʱ��֮ǰ�ɼ���
         time = nextTime(time, interval, 0);
 			 }
 		}
 		da1 >>= 1;
 	}
	
	#endif
	return frameTail;
}

/*******************************************************
��������:AFN0C129
��������:��Ӧ��վ����һ������"��ǰ�����й�����ʾֵ"������ݸ�ʽ14��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C129(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG  meterConfig;
	INT8U   dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U  pn, tmpPn = 0;
  INT8U   da1, da2;
  INT8U   tariff, tmpTariff;
  INT8U   queryType, dataType;
  struct dotFn129    *fnNew129, *prevFn129, *tmpPfn129;

  BOOL    ifHasData;
  BOOL    buffHasData;
  
  INT16U  offSet;
  INT16U  i;
  
  DATE_TIME time;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
  
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
      	;
      }
      
      switch(fn)
      {
      	case 129:		//��ǰ�����й�����ʾֵ(�ܣ�����1~M)
      	
      	#ifdef PLUG_IN_CARRIER_MODULE
      	 #ifdef LM_SUPPORT_UT
      		if ((meterConfig.rateAndPort&0x1f)==31 && 0x55!=lmProtocol)
      	 #else
          if ((meterConfig.rateAndPort&0x1f)==31)
      	 #endif
          {
      		  fnNew129 = (struct dotFn129 *)malloc(sizeof(struct dotFn129));
      		  fnNew129->pn           = pn;
      		  fnNew129->from         = afn0cDataFrom;
      		  fnNew129->ifProcessing = 0;
      		  fnNew129->next         = NULL;

            prevFn129 = pDotFn129;
            tmpPfn129 = pDotFn129;
            while(tmpPfn129!=NULL)
            {
            	prevFn129 = tmpPfn129;
            	
            	if (tmpPfn129->next!=NULL)
            	{
            	  tmpPfn129 = tmpPfn129->next;
            	}
            	else
            	{
            		break;
            	}
            }
            
      		  if (pDotFn129==NULL)
      		  {
      		  	pDotFn129 = fnNew129;
      		  }
      		  else
      		  {
      		  	prevFn129->next = fnNew129;
      		  }
      		  
      		  tmpPfn129 = pDotFn129;
      		  while(tmpPfn129!=NULL)
      		  {
      		  	 printf("Ҫ�㳭�Ĳ�����=%d\n",tmpPfn129->pn);
      		  	 
      		  	 tmpPfn129 = tmpPfn129->next;
      		  }
      		  
      		  frameTail -= 2;
      		  
      		  goto breakPoint129;
      		  
      		  /*
      		  if (pDotCopy==NULL)
      		  {
      		    pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
      		    pDotCopy->dotCopyMp   = pn;
      		    pDotCopy->dotCopying  = FALSE;
      		    pDotCopy->port        = PORT_POWER_CARRIER;
      		    pDotCopy->dataFrom    = afn0cDataFrom;
      		    pDotCopy->outTime     = nextTime(sysTime,0,15);
      		    pDotCopy->dotResult   = RESULT_NONE;
      		  }
      		  
      		  return frameTail-2;
      		  */
      		}
      		else
      		{
      		  msFrame[frameTail++] = 0x01;   						//DT1
      		  msFrame[frameTail++] = 0x10;   						//DT2
      		  offSet = POSITIVE_WORK_OFFSET;
      		}
      	#else      		
      		msFrame[frameTail++] = 0x01;   							//DT1
      		msFrame[frameTail++] = 0x10;   							//DT2
      		offSet = POSITIVE_WORK_OFFSET;
      	#endif
      	 
      		break;
      		
      	case 130:		//��ǰ�����޹�(����޹�1)����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = POSITIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 131:		//��ǰ�����й�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = NEGTIVE_WORK_OFFSET;
      		break;
      		
      	case 132:		//��ǰ�����޹�(����޹�1)����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = NEGTIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 133:		//��ǰһ�����޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA1_NO_WORK_OFFSET;
      		break;
      		
      	case 134:		//��ǰ�������޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA2_NO_WORK_OFFSET;
      		break;
      		
      	case 135:		//��ǰ�������޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA3_NO_WORK_OFFSET;
      		break;
      		
      	case 136:		//��ǰ�������޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x80;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA4_NO_WORK_OFFSET;
      		break;
      		
      	case 137:		//����(��һ������)�����й�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = POSITIVE_WORK_OFFSET;
      		break;
      		
      	case 138:		//����(��һ������)�����޹�(����޹�1)����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = POSITIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 139:		//����(��һ������)�����й�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = NEGTIVE_WORK_OFFSET;
      		break;
      		
      	case 140:		//����(��һ������)�����޹�(����޹�1)����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = NEGTIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 141:		//����(��һ������)һ�����޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA1_NO_WORK_OFFSET;
      		break;
      		
      	case 142:		//����(��һ������)�������޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA2_NO_WORK_OFFSET;
      		break;
      		
      	case 143:		//����(��һ������)�������޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA3_NO_WORK_OFFSET;
      		break;
      		
      	case 144:		//����(��һ������)�������޹�����ʾֵ(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x80;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA4_NO_WORK_OFFSET;
      		break;
      }
      
      
      if(fn >= 129 && fn <= 136)
      {
      	queryType = PRESENT_DATA;
      	dataType = ENERGY_DATA;
      }
      else if(fn >= 137 && fn <= 144)
      {
      	queryType = LAST_MONTH_DATA;
      	dataType = POWER_PARA_LASTMONTH;
      }
      
      //ȡ�÷��ʸ���
			tariff = numOfTariff(pn);
			if (((meterConfig.rateAndPort&0x1f)<5 && (meterConfig.bigAndLittleType&0xf)==1)
				  || ((meterConfig.rateAndPort&0x1f)==31)
				 )
			{
        buffHasData = readMeterData(dataBuff, pn, SINGLE_PHASE_PRESENT, dataType, &time, 0);
        
        if (fn==131)
        {
      		offSet = NEGTIVE_WORK_OFFSET_S;
        }
			}
			else
			{
			  buffHasData = readMeterData(dataBuff, pn, queryType, dataType, &time, 0);
			}

			if(buffHasData == TRUE)
			{
				//�ն˳���ʱ��
				msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
				
				//ȡ�÷��ʸ���
				msFrame[frameTail++] = tariff;
				
				for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
				{
    			//ly,2011-12-12,������ж�,��ǰ�İ汾�Ե������˵ȫ�Ǵ����
    			if ((((meterConfig.rateAndPort&0x1f)<5 && (meterConfig.bigAndLittleType&0xf)==1)
    				  || ((meterConfig.rateAndPort&0x1f)==31)) && (!((fn==131) || (fn==129)))
    				 )
    			{
    				if (fn==137 || fn==139)
    				{
    					msFrame[frameTail++] = 0xee;
    				}
    				msFrame[frameTail++] = 0xee;
    				msFrame[frameTail++] = 0xee;
    				msFrame[frameTail++] = 0xee;
    				msFrame[frameTail++] = 0xee;
  	      	
  	      	//�����ʵĴ���.����1����һ��
  	      	if (tariff==1)
  	      	{
    				  if (fn==137 || fn==139)
    				  {
    					  msFrame[frameTail++] = 0xee;
    				  }
    				  msFrame[frameTail++] = 0xee;
    				  msFrame[frameTail++] = 0xee;
    				  msFrame[frameTail++] = 0xee;
    				  msFrame[frameTail++] = 0xee;
    				  
    				  tariff++;  	      		
  	      	}
    			}
    			else
    			{
  					if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
  					{
  						if((dataBuff[offSet] != 0xEE) && (dataBuff[offSet]!=0xFF))
  						{
  							msFrame[frameTail++] = 0x0;
                ifHasData = TRUE;
  						}
  						else
  						{
  							msFrame[frameTail++] = 0xEE;
  						}
  					}
  				  
  				  if(dataBuff[offSet]==0xFF)
  				  {
  				  	msFrame[frameTail++] = 0xee;
  				  	msFrame[frameTail++] = 0xee;
  				  	msFrame[frameTail++] = 0xee;
  				  	msFrame[frameTail++] = 0xee;
  				  }
  				  else
  					{
  					  msFrame[frameTail++] = dataBuff[offSet];
  					  msFrame[frameTail++] = dataBuff[offSet + 1];
  	      	  msFrame[frameTail++] = dataBuff[offSet + 2];
  	      	  msFrame[frameTail++] = dataBuff[offSet + 3];
  	      	}
  	      	
  	      	//�����ʵĴ���.����1����һ��
  	      	if (tariff==1)
  	      	{
  					  if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
  					  {
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		}
  	      		else
  	      		{
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		}
  	      		
  	      		tmpTariff++;
  	      	}
	      	  offSet += 4;
  	      }
				}
				
				#ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	        	if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
						{
							frameTail -= (10 + (tariff + 1) * 5);
						}
						else
						{
							frameTail -= (10 + (tariff + 1) * 4);
						}
	        }
        #endif
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    //�ն˳���ʱ��
					msFrame[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
					msFrame[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
					msFrame[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
					msFrame[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
					msFrame[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
					//������
					msFrame[frameTail++] = tariff;
					
					if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
					{
						for(i = 0; i <(tariff + 1) * 5; i++)
						{
							msFrame[frameTail++] = 0xEE;
						}
					}
					else
					{
						for(i = 0; i <(tariff + 1) * 4; i++)
						{
							msFrame[frameTail++] = 0xEE;
						}
					}
		    #endif
			}
		}

breakPoint129:
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C130
��������:��Ӧ��վ����һ������"��ǰ�����޹�(����޹�1)����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C130(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C131
��������:��Ӧ��վ����һ������"��ǰ�����й�����ʾֵ(��,����1~M)"������ݸ�ʽ14��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C131(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C132
��������:��Ӧ��վ����һ������"��ǰ�����޹�(����޹�1)����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C132(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C133
��������:��Ӧ��վ����һ������"��ǰһ�����޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C133(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C134
��������:��Ӧ��վ����һ������"��ǰ�������޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C134(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C135
��������:��Ӧ��վ����һ������"��ǰ�������޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C135(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C136
��������:��Ӧ��վ����һ������"��ǰ�������޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C136(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C137
��������:��Ӧ��վ����һ������"����(��һ������)�����й�����ʾֵ(��,����1~M)"������ݸ�ʽ14��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C137(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C138
��������:��Ӧ��վ����һ������"����(��һ������)�����޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C138(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C139
��������:��Ӧ��վ����һ������"����(��һ������)�����й�����ʾֵ(��,����1~M)"������ݸ�ʽ14��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C139(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C140
��������:��Ӧ��վ����һ������"����(��һ������)�����޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C140(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C141
��������:��Ӧ��վ����һ������"����(��һ������)һ�����޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C141(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C142
��������:��Ӧ��վ����һ������"����(��һ������)�������޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C142(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C143
��������:��Ӧ��վ����һ������"����(��һ������)�������޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C143(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C144
��������:��Ӧ��վ����һ������"����(��һ������)�������޹�����ʾֵ(��,����1~M)"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C144(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C145
��������:��Ӧ��վ����һ������"���������й��������������ʱ��(��,����1~M)"������ݸ�ʽ17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C145(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_REQ_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     tariff, tmpTariff;
  INT8U     queryType, dataType;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet1, offSet2;
  INT16U    i;
  
  DATE_TIME time, readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 145:		//���������й��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x01;   															//DT1
      		offSet1 = REQ_POSITIVE_WORK_OFFSET;				//�����й��������
      		offSet2 = REQ_TIME_P_WORK_OFFSET;					//�����й������������ʱ��
      		break;
      		
      	case 146:		//���������޹��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x02;   															//DT1
      		offSet1 = REQ_POSITIVE_NO_WORK_OFFSET;		//�����޹��������
      		offSet2 = REQ_TIME_P_NO_WORK_OFFSET;			//�����޹������������ʱ��
      		break;
      		
      	case 147:		//���·����й��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x04;   															//DT1
      		offSet1 = REQ_NEGTIVE_WORK_OFFSET;				//�����й��������
      		offSet2 = REQ_TIME_N_WORK_OFFSET;					//�����й������������ʱ��
      		break;
      		
      	case 148:		//���·����޹��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x08;   															//DT1
      		offSet1 = REQ_NEGTIVE_NO_WORK_OFFSET;			//�����޹��������
      		offSet2 = REQ_TIME_N_NO_WORK_OFFSET;			//�����޹������������ʱ��
      		break;
      		
      	case 149:		//����(��һ������)�����й��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x10;   															//DT1
      		offSet1 = REQ_POSITIVE_WORK_OFFSET;				//�����й��������
      		offSet2 = REQ_TIME_P_WORK_OFFSET;					//�����й������������ʱ��
      		break;
      		
      	case 150:		//����(��һ������)�����޹��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x20;   															//DT1
      		offSet1 = REQ_POSITIVE_NO_WORK_OFFSET;		//�����޹��������
      		offSet2 = REQ_TIME_P_NO_WORK_OFFSET;			//�����޹������������ʱ��
      		break;
      		
      	case 151:		//����(��һ������)�����й��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x40;   															//DT1
      		offSet1 = REQ_NEGTIVE_WORK_OFFSET;				//�����й��������
      		offSet2 = REQ_TIME_N_WORK_OFFSET;					//�����й������������ʱ��
      		break;
      		
      	case 152:		//����(��һ������)�����޹��������������ʱ��(�ܣ�����1~M)
      		msFrame[frameTail++] = 0x80;   															//DT1
      		offSet1 = REQ_NEGTIVE_NO_WORK_OFFSET;			//�����޹��������
      		offSet2 = REQ_TIME_N_NO_WORK_OFFSET;			//�����޹������������ʱ��
      		break;
      }
      msFrame[frameTail++] = 0x12;   																	//DT2
      
      
      if(fn >= 145 && fn <= 148)
      {
      	queryType = PRESENT_DATA;
      	dataType = REQ_REQTIME_DATA;
      }
      else if(fn >= 149 && fn <= 152)
      {
      	queryType = LAST_MONTH_DATA;
      	dataType = REQ_REQTIME_LASTMONTH;
      }
      
      ifHasData = FALSE;
      
      //ȡ�÷��ʸ���
			tariff = numOfTariff(pn);      
			
			buffHasData = readMeterData(dataBuff, pn, queryType, dataType, &time, 0);
				
			if(buffHasData == TRUE)
			{
				//�ն˳���ʱ��
				msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
				
				msFrame[frameTail++] = tariff;
				
				//�������������ʱ��
				for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
				{
					//�������
					if(dataBuff[offSet1] != 0xEE)
					{
						ifHasData = TRUE;
					}
					msFrame[frameTail++] = dataBuff[offSet1];
					msFrame[frameTail++] = dataBuff[offSet1 + 1];
					msFrame[frameTail++] = dataBuff[offSet1 + 2];
					
					//����ʱ��(��ʱ����)
					if(dataBuff[offSet2] != 0xEE)
					{
						ifHasData = TRUE;
					}
					msFrame[frameTail++] = dataBuff[offSet2];
					msFrame[frameTail++] = dataBuff[offSet2 + 1];
	      	msFrame[frameTail++] = dataBuff[offSet2 + 2];
	      	msFrame[frameTail++] = dataBuff[offSet2 + 3];
	      	
	      	//�����ʵĴ���.����1����һ��
	      	if (tariff==1)
	      	{
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 
	      		 tmpTariff++;
	      	}
	      	else
	      	{
	      	  offSet1 += 3;
	      	  offSet2 += 5;
	      	}
				}
				
				#ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff + 1) * 7);
	        }
        #endif
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    //�ն˳���ʱ��
					msFrame[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
					msFrame[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
					msFrame[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
					msFrame[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
					msFrame[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
					//������
					msFrame[frameTail++] = tariff;
					
					for(i = 0; i < (tariff + 1) * 7; i++)
					{
						msFrame[frameTail++] = 0xEE;
					}
		    #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
��������:AFN0C146
��������:��Ӧ��վ����һ������"���������޹��������������ʱ��(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C146(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C147
��������:��Ӧ��վ����һ������"���·����й��������������ʱ��(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C147(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C148
��������:��Ӧ��վ����һ������"���·����޹��������������ʱ��(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C148(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C149
��������:��Ӧ��վ����һ������"����(��һ������)�����й��������������ʱ��
				(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C149(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C150
��������:��Ӧ��վ����һ������"����(��һ������)�����޹��������������ʱ��
				(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C150(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C151
��������:��Ӧ��վ����һ������"����(��һ������)�����й��������������ʱ��
				(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C151(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C152
��������:��Ӧ��վ����һ������"����(��һ������)�����޹��������������ʱ��
				(��,����1~M)"������ݸ�ʽ15,17,23��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C152(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
��������:AFN0C153
��������:��Ӧ��վ����һ������"��һʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C153(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C154
��������:��Ӧ��վ����һ������"�ڶ�ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C154(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C155
��������:��Ӧ��վ����һ������"����ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C155(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C156
��������:��Ӧ��վ����һ������"����ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C156(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C157
��������:��Ӧ��վ����һ������"����ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C157(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C158
��������:��Ӧ��վ����һ������"����ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C158(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C159
��������:��Ӧ��վ����һ������"����ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C159(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C160
��������:��Ӧ��վ����һ������"�ڰ�ʱ�����������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C160(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C161
��������:��Ӧ��վ����һ������"���ܱ�Զ�̿���ͨ�ϵ�״̬����¼"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C161(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[512];
	INT8U     tmpPn, pn;
  INT8U     i, j;
  INT8U     da1, da2;
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    tmpTail,counti;  
  DATE_TIME time;
  INT8U     meterInfo[10];
	
  tmpPn = 0;
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//��ѯ����������ϴγ���ʱ��
  		time = queryCopyTime(pn);
  		
  		//��ѯ������洢��Ϣ
  		queryMeterStoreInfo(pn, meterInfo);
  		
  		if (meterInfo[0]<7)
  		{
  		   buffHasData =  readMeterData(dataBuff, pn , meterInfo[1], ENERGY_DATA, &time, 0);
  		}
  		else
  		{
  		   buffHasData =  readMeterData(dataBuff, pn , meterInfo[1], PARA_VARIABLE_DATA, &time, 0);
  		}
		    	
		  msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
		  msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
		  
		  switch(fn)
		  {
		    case 161:   //���ܱ�Զ�̿���ͨ�ϵ�״̬����¼
		      msFrame[frameTail++] = 0x01;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2
		      break;
		      
		    case 165:   //���ܱ��ز���������ʱ��
		      msFrame[frameTail++] = 0x10;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2		    	
		    	break;

		    case 166:   //���ܱ�����޸Ĵ�����ʱ��
		      msFrame[frameTail++] = 0x20;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2		    	
		    	break;

		    case 167:   //���ܱ����õ���Ϣ
		      msFrame[frameTail++] = 0x40;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2		    	
		    	break;
		  }
		
		  ifHasData = FALSE;
		  if (buffHasData == TRUE)
		  {
		    //�ն˳���ʱ��
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
		     		     
		    switch(fn)
		    {
		      case 161:
		        switch (meterInfo[0])
		        {
  		        case 1:  //�������ܱ�(û�и�������)
  		        	for(i=0;i<11;i++)
  		        	{
  		        		 msFrame[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //���౾�طѿر�
  		        case 3:  //����Զ�̷ѿر�
  		          //���ܱ�ͨ�ϵ�״̬
  		          if (dataBuff[METER_STATUS_WORD_S_3]&0x40)
  		          {
  		            msFrame[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            msFrame[frameTail++] = 0x0;
  		          }
  		    
  		          //���һ�ε��ܱ�Զ�̿���ͨ��ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		    
  		          //���һ�ε��ܱ�Զ�̿��ƶϵ�ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		          break;

  		        case 5:  //���౾�طѿر�
  		        case 6:  //����Զ�̷ѿر�
  		          //���ܱ�ͨ�ϵ�״̬
  		          if (dataBuff[METER_STATUS_WORD_T_3]&0x40)
  		          {
  		            msFrame[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            msFrame[frameTail++] = 0x0;
  		          }
  		    
  		          //���һ�ε��ܱ�Զ�̿���ͨ��ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		    
  		          //���һ�ε��ܱ�Զ�̿��ƶϵ�ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		          break;
  		          
  		        default:
    		        //���ܱ�ͨ�ϵ�״̬
    		        if (dataBuff[METER_STATUS_WORD_3]&0x40)
    		        {
    		          msFrame[frameTail++] = 0x11;
    		        }
    		        else
    		        {
    		          msFrame[frameTail++] = 0x0;
    		        }
    		    
    		        //���һ�ε��ܱ�Զ�̿���ͨ��ʱ��
    		        for(i=1;i<6;i++)
    		        {
    		          msFrame[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME+i];
    		        }
    		        frameTail += 5;
    		    
    		        //���һ�ε��ܱ�Զ�̿��ƶϵ�ʱ��
    		        for(i=1;i<6;i++)
    		        {
    		          msFrame[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME+i];
    		        }
    		        frameTail += 5;
    		        break;
  		      }
		        break;
		        
		      case 165:  //���ܱ��ز���������ʱ��
		      	//��̿��ز�������
		      	msFrame[frameTail++] = dataBuff[PROGRAM_TIMES];
		      	msFrame[frameTail++] = dataBuff[PROGRAM_TIMES+1];
		      	
		      	//���һ�α�̿��ز���ʱ��
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[LAST_PROGRAM_TIME+i];
		        }
		        frameTail += 5;
		        
		        //���ܱ�β�Ǵ򿪴���
		      	msFrame[frameTail++] = dataBuff[OPEN_METER_COVER_TIMES];
		      	msFrame[frameTail++] = dataBuff[OPEN_METER_COVER_TIMES+1];
		        
		        //���һ��β�Ǵ�ʱ��
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[LAST_OPEN_METER_COVER_TIME+i];
		        }
		        frameTail += 5;
		      	break;
		      	
		      case 166:  //���ܱ�����޸Ĵ�����ʱ��
		      	//���ܱ�ʱ���޸Ĵ���
		      	msFrame[frameTail++] = dataBuff[TIMING_TIMES];
		      	msFrame[frameTail++] = dataBuff[TIMING_TIMES+1];
		      	
		        //���һ��Уʱ����ʱ��
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[TIMING_LAST_TIME+i];
		        }
		        frameTail += 5;
		        
		      	//���ܱ�ʱ�β����޸Ĵ���
		      	msFrame[frameTail++] = dataBuff[PERIOD_TIMES];
		      	msFrame[frameTail++] = dataBuff[PERIOD_TIMES+1];
		      	
		        //���һ��ʱ�β����޸ķ���ʱ��
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[PERIOD_LAST_TIME+i];
		        }
		        frameTail += 5;
		      	break;
		      	
		      case 167:   //���ܱ����õ���Ϣ
		        switch (meterInfo[0])
		        {
  		        case 1:  //�������ܱ�(û��/δ����������)
  		        case 3:  //����Զ�̷ѿر�(û��/δ����������)
  		        case 6:  //����Զ�̷ѿر�(û��/δ����������)
  		        	for(i=0;i<36;i++)
  		        	{
  		        		 msFrame[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //���౾�طѿر�
     		      	//�������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S+1];
     		      	
     		      	//ʣ����
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_S]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+3];
     		      	
     		      	//�ۼƹ�����
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_S]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+3];
     		      	
     		      	//ʣ�����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+3];
     		      	
     		      	//͸֧����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+3];
     		      	
     		      	//�ۼƹ�����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+3];
     		      	
     		      	//��Ƿ���޵���
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+3];
     		      	
     		      	//��������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+3];
     		      	
     		      	//���ϵ���
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
  		        	break;
  		        	
  		        case 5:  //���౾�طѿر�
     		      	//�������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T+1];
     		      	
     		      	//ʣ����
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_T]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+3];
     		      	
     		      	//�ۼƹ�����
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_T]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+3];
     		      	
     		      	//ʣ�����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+3];
     		      	
     		      	//͸֧����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+3];
     		      	
     		      	//�ۼƹ�����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+3];
     		      	
     		      	//��Ƿ���޵���
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+3];
     		      	
     		      	//��������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+3];
     		      	
     		      	//���ϵ���
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
  		        	break;
  		        	  		        	
  		        default:
     		      	//�������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME+1];
     		      	
     		      	//ʣ����
     		      	if (dataBuff[CHARGE_REMAIN_MONEY]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+3];
     		      	
     		      	//�ۼƹ�����
     		      	if (dataBuff[CHARGE_TOTAL_MONEY]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+3];
     		      	
     		      	//ʣ�����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+3];
     		      	
     		      	//͸֧����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+3];
     		      	
     		      	//�ۼƹ�����
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+3];
     		      	
     		      	//��Ƿ���޵���
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+3];
     		      	
     		      	//��������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+3];
     		      	
     		      	//���ϵ���
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	break;
     		    }
		      	break;
		    }
		  }
		  else
		  {
		    #ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
			    msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
			    msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
			    msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
			    msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
			    switch(fn)
			    {
			    	case 161:
			        for(j=0;j<11;j++)
			        {
			  	      msFrame[frameTail++] = 0xee;
			        }
			        break;
			        
			      case 165:
			      case 166:
			        for(j=0;j<14;j++)
			        {
			  	      msFrame[frameTail++] = 0xee;
			        }
			      	break;

			      case 167:
			        for(j=0;j<36;j++)
			        {
			  	      msFrame[frameTail++] = 0xee;
			        }
			      	break;
			    }
		    #endif
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0C165
��������:��Ӧ��վ����һ������"���ܱ��ز���������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C165(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C161(frameTail, pHandle, fn);
}

/*******************************************************
��������:AFN0C166
��������:��Ӧ��վ����һ������"���ܱ�����޸Ĵ�����ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C166(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C161(frameTail, pHandle, fn);
}

/*******************************************************
��������:AFN0C167
��������:��Ӧ��վ����һ������"���ܱ����õ���Ϣ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C167(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C161(frameTail, pHandle, fn);
}

/*******************************************************
��������:AFN0C168
��������:��Ӧ��վ����һ������"���ܱ������Ϣ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C168(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
��������:AFN0C169
��������:��Ӧ��վ����һ������"���г����м�·����Ϣ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C169(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	#ifdef PLUG_IN_CARRIER_MODULE
    INT16U    tmpPn,pn;
    INT8U     da1, da2;
    METER_DEVICE_CONFIG meterConfig;

    da1 = *pHandle++;
    da2 = *pHandle++;
    
    tmpPn = 0;
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		
		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x01;                                   //DT1
		    msFrame[frameTail++] = 0x15;                                   //DT2
        
        if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
        {
		      msFrame[frameTail++] = 0xee;   //ͨ�Ŷ˿�Ϊ
		      msFrame[frameTail++] = 0xee;   //�м�·�ɸ���
		    }
		    else
		    {
		      msFrame[frameTail++] = meterConfig.rateAndPort&0x1f;   //ͨ�Ŷ˿�Ϊ31
		      msFrame[frameTail++] = 0x0;    //�м�·�ɸ���
		    }
    	}
    	da1 >>= 1;
    }	 
	#endif
	
	return frameTail;
}

/*******************************************************
��������:AFN0C170
��������:��Ӧ��վ����һ������"���г�����ܱ�����Ϣ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0C170(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	#ifdef PLUG_IN_CARRIER_MODULE
    INT16U    tmpPn,pn;
    INT8U     da1, da2;
    METER_DEVICE_CONFIG meterConfig;
    METER_STATIS_BEARON_TIME meterStatisBearonTime;   //һ���ͳ���¼�����(��ʱ���й���)
    
    da1 = *pHandle++;
    da2 = *pHandle++;
    
    tmpPn = 0;
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		
    		printf("AFN0C FN170,pn=%d\n",pn);

		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x02;                                   //DT1
		    msFrame[frameTail++] = 0x15;                                   //DT2
        if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
        {
        	 memset(&msFrame[frameTail], 0xee, 18);
        }	
        else
        {
  		    msFrame[frameTail++] = meterConfig.rateAndPort&0x1f;   //ͨ�Ŷ˿ں�
  		    
  		    if((meterConfig.rateAndPort&0x1f)<4)
  		    {
  		      msFrame[frameTail++] = 0xee; //�м�·�ɼ���
  		      msFrame[frameTail++] = 0xee; //�ز�����ͨ����λ
  		      msFrame[frameTail++] = 0xee; //�ز��ź�Ʒ��
  		    }
  		    else
  		    {
  		      msFrame[frameTail++] = 0;    //�м�·�ɼ���
  		      msFrame[frameTail++] = 0x71; //�ز�����ͨ����λ
  		      msFrame[frameTail++] = 0xfe; //�ز��ź�Ʒ��
  		    }
  		    
          searchMpStatis(sysTime, &meterStatisBearonTime, pn, 2);

  		    if (meterStatisBearonTime.copySuccessTimes==0)
  		    {
  		      msFrame[frameTail++] = 0x0;  //���һ�γ���ɹ�/ʧ�ܱ�־Ϊʧ��
  		    
  		      //���һ�γ���ɹ�ʱ��
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x0;
  		    
  		      //���һ�γ���ʧ��ʱ��
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x0;
  		    
  		      //�������ʧ���ۼƴ���
  		      msFrame[frameTail++] = 0;
  		    }
  		    else
  		    {  		    
  		      msFrame[frameTail++] = 0x1;  //���һ�γ���ɹ�/ʧ�ܱ�־Ϊ�ɹ�
  		    
  		      //���һ�γ���ɹ�ʱ��
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = meterStatisBearonTime.lastCopySuccessTime[0];
  		      msFrame[frameTail++] = meterStatisBearonTime.lastCopySuccessTime[1];
  		      msFrame[frameTail++] = sysTime.day/10<<4 | sysTime.day%10;
  		      msFrame[frameTail++] = sysTime.month/10<<4 | sysTime.month%10;
  		      msFrame[frameTail++] = sysTime.year/10<<4 | sysTime.year%10;
  		    
  		      //���һ�γ���ʧ��ʱ��
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x0;
  		    
  		      //�������ʧ���ۼƴ���
  		      msFrame[frameTail++] = 0;
  		    }
  		  }		    
    	}
    	da1 >>= 1;
    }	 
	#endif
	
	return frameTail;
}

/*******************************************************
��������:queryCopyTime
��������:���ݲ�����Ų�ѯ�ϴγ���ʱ��
���ú���:
�����ú���:
�������:
�������:
����ֵ���ϴγ���ʱ��
*******************************************************/
DATE_TIME queryCopyTime(INT16U pn)
{   
   METER_DEVICE_CONFIG meterConfig;         //һ���������������Ϣ
   DATE_TIME           time;
   
   if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
   {
     time = timeHexToBcd(sysTime);
     //����û�иò����������
   }
   else
   {
   	 switch (meterConfig.rateAndPort&0xf)
   	 {
   	   case 1:
   	     time = copyCtrl[0].lastCopyTime;
   	     break;
   	     
   	   case 2:
   	     time = copyCtrl[1].lastCopyTime;
   	     break;

   	   case 3:
   	     time = copyCtrl[2].lastCopyTime;
   	     break;

   	   case 4:
   	     time = copyCtrl[3].lastCopyTime;
   	     break;
   	     
   	   case 31:
   	     time = copyCtrl[4].lastCopyTime;
   	     break;
   	     
   	   default:
   	   	 time = timeHexToBcd(sysTime);
   	   	 break;
   	 }
   }
  
   return time;
}

