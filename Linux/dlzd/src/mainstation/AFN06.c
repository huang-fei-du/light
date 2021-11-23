/***************************************************
Copyright,2011,Huawei WoDian co.,LTD
�ļ�����AFN06.c
���ߣ�Leiyong
�汾��0.9
������ڣ�2011��3��
��������վAFN06(�����֤����ԿЭ��)�����ļ���
�����б�
  01,11-03-11,Leiyong created.
***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"
#include "msSetPara.h"
#include "dataBase.h"
#include "teRunPara.h"
#include "copyMeter.h"
#include "ioChannel.h"
#include "hardwareConfig.h"

#include "AFN00.h"
#include "AFN06.h"

#define NUM_OF_MAC_BLOCK   240

//����
INT16U offset06;            //���յ���֡�е����ݵ�Ԫƫ����(�������ݱ�ʶ���ֽ�)
INT8U  esamSerial[8];       //ESAMоƬ���к�

/**************************************************
��������:delay01etu
��������:��ʱ0.1��etu
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
void delay01etu(unsigned int times)
{
   unsigned int i;
   
   while(times>0)
   {
     //for(i=0;i<474;i++)
     for(i=0;i<948;i++)
     {
   	   ;
     }
     
     times--;
   }
}

/*******************************************************
��������:AFN06
��������:��վ"�����֤����ԿЭ��"(AFN06)������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN06(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   INT16U    frameTail06;             //����β
   INT16U    tmpI,tmpFrameTail, tmpHead06;
   INT8U     frameCounter,checkSum;   //���ͼ����� 
   //char      say[20],str[8];
   INT16U    i;
   INT8U     fn, ackTail;
   INT8U     ackAll, nAckAll;       //ȫ��ȷ��,���ϱ�־      
   INT8U     maxCycle;              //���ѭ������
   INT8U     tmpDtCount;            //DT��λ����
   INT8U     tmpDt1;                //��ʱDT1
   INT8U     *pTpv;                 //TpVָ��

   INT16U (*AFN06Fun[134])(INT16U tail,INT8U *pHandle,INT8U dataFrom);
   
   //�������
   bzero(ackData, 100);

   for(i=0;i<10;i++)
   {
     AFN06Fun[i] = NULL;
   }
   
   //��1
   //AFN06Fun[0] = AFN06001;
   //AFN06Fun[1] = AFN06002;
   //AFN06Fun[2] = AFN06003;
   //AFN06Fun[3] = AFN06004;
   AFN06Fun[4] = AFN06005;
   AFN06Fun[5] = AFN06006;
   AFN06Fun[6] = AFN06007;
   AFN06Fun[7] = AFN06008;
   
   //��2
   AFN06Fun[8] = AFN06009;
   AFN06Fun[9] = AFN06010;
   
   if (fQueue.tailPtr == 0)
   {
     tmpHead06 = 0;
   }
   else
   {
     tmpHead06 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }
   frameTail06 = tmpHead06+14; 
   
   ackAll = 0;
   nAckAll = 0;
   ackTail = 0;
   maxCycle = 0;
   frameCounter = 0;
   while ((frame.loadLen > 0) && (maxCycle<5))
   {
      maxCycle++;
      
      offset06 = 0;
            
      tmpDt1 = *(pDataHead + 2);
      tmpDtCount = 0;
      while(tmpDtCount < 9)
      {
         tmpDtCount++;
         if ((tmpDt1 & 0x1) == 0x1)
         {
            fn = *(pDataHead + 3) * 8 + tmpDtCount;
            
            //printf("AFN06 Fn=%d\n",fn);
            
            if (fn>10)
            {
      	       maxCycle = 5;
      	       break;
      	    }

            if (AFN06Fun[fn-1] != NULL)
            {
               //����ȷ��/������д
               ackData[ackTail*5]   = *pDataHead;                         //DA1
               ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
               ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
               ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
               
               if ((tmpFrameTail = AFN06Fun[fn-1](frameTail06,pDataHead,dataFrom))==0)
               { 
               	 return;
               }
               else
               {
                 frameTail06 = tmpFrameTail;                 
               }
            }
         }
         
         tmpDt1 >>= 1;
         
         if (((frameTail06 - tmpHead06) > MAX_OF_PER_FRAME) || (((pDataHead+offset06+4) == pDataEnd) && tmpDtCount==8))
         {
             //�����������ϱ������¼�����
             if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail06 - tmpHead06) > 16)
             {
            	   msFrame[frameTail06++] = iEventCounter;
            	   msFrame[frameTail06++] = nEventCounter;
             }

             //��������վҪ���ж��Ƿ�Я��TP
             if (frame.pTp != NULL)
             {
                pTpv = frame.pTp;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv;
             }
             
             msFrame[tmpHead06 + 0] = 0x68;   //֡��ʼ�ַ�
           
             tmpI = ((frameTail06 - tmpHead06 -6) << 2) | PROTOCOL_FIELD;
             msFrame[tmpHead06 + 1] = tmpI & 0xFF;   //L
             msFrame[tmpHead06 + 2] = tmpI >> 8;
             msFrame[tmpHead06 + 3] = tmpI & 0xFF;   //L
             msFrame[tmpHead06 + 4] = tmpI >> 8; 
           
             msFrame[tmpHead06 + 5] = 0x68;  //֡��ʼ�ַ�
      
      			 //������
             if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
             {
                msFrame[tmpHead06 + 6] = 0xa8;  //�����ֽ�10001000
             }
             else
             {
             	  msFrame[tmpHead06 + 6] = 0x88;  //�����ֽ�10001000
             }
             	 
             //��ַ��
             msFrame[tmpHead06 + 7] = addrField.a1[0];
             msFrame[tmpHead06 + 8] = addrField.a1[1];
             msFrame[tmpHead06 + 9] = addrField.a2[0];
             msFrame[tmpHead06 + 10] = addrField.a2[1];
             msFrame[tmpHead06 + 11] = addrField.a3;
     
             msFrame[tmpHead06 + 12] = 0x06;  //AFN
      
             if ((pDataHead+4) == pDataEnd)
             {
                if (frameCounter == 0)
                {
                  msFrame[tmpHead06 + 13] = 0x60 | rSeq;    //01100000 | rSeq ��֡
                }
                else
                {
                  msFrame[tmpHead06 + 13] = 0x20 | rSeq;    //00100000 | rSeq ���һ֡
                }
             }
             else
             {
               if (frameCounter == 0)
               {
                 msFrame[tmpHead06 + 13] = 0x40 | rSeq;     //01000000 | rSeq  ��һ֡
               }
               else
               {
                 msFrame[tmpHead06 + 13] = 0x00 | rSeq;     //00000000 | rSeq �м�֡
               }
               frameCounter++;
             }

             if (frame.pTp != NULL)
             {
            	  msFrame[tmpHead06+13] |= 0x80;       //TpV��λ
             }
                          
             tmpI = tmpHead06 + 6;
             checkSum = 0;
             while (tmpI < frameTail06)
             {
                checkSum = msFrame[tmpI] + checkSum;
                tmpI++;
             }
             
             msFrame[frameTail06++] = checkSum;
             msFrame[frameTail06++] = 0x16;
             
             fQueue.frame[fQueue.tailPtr].head = tmpHead06;
             fQueue.frame[fQueue.tailPtr].len = frameTail06-tmpHead06;
             
             if (((frameTail06 - tmpHead06) > 16 && frame.pTp==NULL)||((frameTail06 - tmpHead06) > 22 && frame.pTp!=NULL))
             {
               tmpHead06 = frameTail06;
               if ((tmpHead06+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
               	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
               {
                  fQueue.frame[fQueue.tailPtr].next = 0x0;
               	  fQueue.tailPtr = 0;
               	  tmpHead06 = 0;
               }
               else
               {                 
                  fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                  fQueue.tailPtr++;
               }

               frameTail06 = tmpHead06 + 14;  //frameTail������Ϊ14��д��һ֡
             }
         }
      }
      
      if (maxCycle==5)
      {
      	 break;
      }
      
      //printf("offset06=%d\n",offset06);
      if (frame.loadLen < offset06+4)
      {
      	 break;
      }
      else
      {
         frame.loadLen -= (offset06 + 4);
         pDataHead = pDataHead + offset06 + 4;
      }
   }
}

/*******************************************************
��������:AFN04005
��������:��Ӧ��վ�����֤����ԿЭ��"��ȡ�ն������(F5)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
INT16U AFN06005(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	INT8U buf[20];
	INT8U i;

  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle;    //DT2
    
  //���ݵ�Ԫ

  //�ն������
  //��ȡ�ն������
  getChallenge(buf);  
  //��������
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  //��ȡESAM Serial
  getSerial(buf);
  //��������
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  offset06 = 0;

  return frameTail;
}

/*******************************************************
��������:AFN04006
��������:��Ӧ��վ�����֤����ԿЭ��"��Կ��֤(F6)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
INT16U AFN06006(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	INT8U cmdBuf[270];
	INT8U i;
	INT8U result;
	char  say[30];

	pHandle += 4;
  cmdBuf[0] = 0x02;     //��һ�ֽ� - ��������
  cmdBuf[1] = 145;     //�ڶ��ֽ� - ���������
  cmdBuf[2] = 0x80;
  cmdBuf[3] = 0x42;
  cmdBuf[4] = 0x00;
  if (*pHandle==2)
  {
  	cmdBuf[5] = 0x09;
  	strcpy(say,"��վ��Կ��֤");
  }
  else
  {
  	cmdBuf[5] = 0x01;
  	strcpy(say,"���ع�Կ��֤");
  }
  cmdBuf[6] = 0x8c;
  
  cmdBuf[7] = 0x81;
  cmdBuf[8] = 0x08;
  pHandle++;
  
  //Ŀǰ�ݹ۲���Ժ�������,�ǽ�������������������,����͵�ESAM��˳���ֵ�����
  //��վ�����
  for(i=0;i<8;i++)
  {
  	cmdBuf[16-i] = *pHandle++;
  	//cmdBuf[9+i] = *pHandle++;
  }
  cmdBuf[17] = 0x60;
  cmdBuf[18] = 0x80;
  
  //Ŀǰ�ݹ۲���Ժ�������,�ǽ�������������������,����͵�ESAM��˳���ֵ�����
  //����ǩ��
  for(i=0;i<128;i++)
  {
  	cmdBuf[146-i] = *pHandle++;
  	//cmdBuf[19+i] = *pHandle++;
  }

  result = read(fdOfIoChannel, cmdBuf, 256);
    
  if (result==0)
  {
     swAnalyse(say,cmdBuf[0], cmdBuf[1]);
    
     if (cmdBuf[0]==0x90 && cmdBuf[1]==0x00)
     {
    	  ackOrNack(TRUE, dataFrom);
     }
     else
     {
        AFN00004(dataFrom, 1, NULL);
     }
  }
  else
  {
    if (debugInfo&ESAM_DEBUG)
    {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("��Կ��֤:����У�����\n");
    	      break;
    	      
    	    case 2:
    	      printf("��Կ��֤:���յ�һ�ֽڲ���INS�ֽ�\n");
    	      break;
    	  }
    }
    
    AFN00004(dataFrom, 1, NULL);
  }

  offset06 = 137;
  
  return 0;
}

/*******************************************************
��������:AFN04007
��������:��Ӧ��վ�����֤����ԿЭ��"��Կ��������(F7)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
INT16U AFN06007(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	INT32U i;
	INT8U  buf[20];

  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle++;  //DT2
    
  //���ݵ�Ԫ
  msFrame[frameTail++] = *pHandle;    //DT2
  
  //��ȡ�ն������
  getChallenge(buf);  
  //��������
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  //��ȡESAM Serial
  getSerial(buf);
  //��������
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  offset06 = 1;

  return frameTail;
}

/*******************************************************
��������:AFN04008
��������:��Ӧ��վ�����֤����ԿЭ��"��Կ����(F8)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
INT16U AFN06008(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	 INT8U  i;
	 INT8U  tmpCmd[5], cmdBuf[500];
	 INT8U  result;
	 INT16U lenOfWrite;    //д�����ݳ���
	 INT8U  upType;        //��Կ��������
	 INT8U  numOfKey;      //�Գ���Կ����ָ�����Կ����
	 INT16U tmpTail;
	 char   say[50];

	 pHandle += 4;
	 
	 upType = *pHandle;
	 lenOfWrite = *(pHandle+1) | *(pHandle+2)<<8;
	 
	 pHandle+=3;
   
   offset06 = lenOfWrite + 3;

   //��һ������
   switch (upType)
   {
   	 case 1:    //���ع�Կ����
   	 case 2:    //��վ��Կ���ظ���       
       cmdBuf[0] = 0x06;    //��һ�ֽ� - ��������
       cmdBuf[0] |= 0x80;   //���ȸ�256�ֽ���0x80��һλ
       cmdBuf[1] = 286-256; //�ڶ��ֽ� - ���������
       cmdBuf[2] = 0x90;
       if (upType==1)
       {
         cmdBuf[3] = 0x40;  //���ع�Կ����ָ��INSΪ0x40
         sprintf(say, "��Կ����(���ع�Կ)");
       }
       else
       {
         cmdBuf[3] = 0x34;  //���ع�Կ���ظ���ָ��INSΪ0x34
         sprintf(say, "��Կ����(��վ��Կ����)");
       }
       cmdBuf[4] = 0x00;
       cmdBuf[5] = 0x00;
       cmdBuf[6] = 0x92;
       cmdBuf[7] = 0x62;
       cmdBuf[8] = 0x90;
       
       //����(/վ)��Կ144Bytes
       for(i=0;i<144;i++)
       {
    	   cmdBuf[152-i] = *pHandle++;
       }
       
       //�ն������8Bytes
       pHandle+=8;
       
       //�ڶ�������
       cmdBuf[153] = 0x80;
     
       if (upType==1)
       {
         cmdBuf[154] = 0x40; //���ع�Կ����ָ��INSΪ0x40
       }
       else
       {
         cmdBuf[154] = 0x34; //���ع�Կ���ظ���ָ��INSΪ0x34
       }
       cmdBuf[155] = 0x00;
       cmdBuf[156] = 0x00;
       cmdBuf[157] = 0x82;
       cmdBuf[158] = 0x60;
       cmdBuf[159] = 0x80;
       
       //����ǩ��128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[287-i] = *pHandle++;
       }
       
       result = read(fdOfIoChannel, cmdBuf, 2);
       break;
       
     case 3:  //��վ��ԿԶ�̸���
       sprintf(say, "��Կ����(��վ��ԿԶ��)");
       cmdBuf[0] = 0x06;    //��һ�ֽ� - ��������
       cmdBuf[0] |= 0x80;   //���ȸ�256�ֽ���0x80��һλ
       cmdBuf[1] = 416-256; //�ڶ��ֽ� - ���������
       cmdBuf[2] = 0x90;
       cmdBuf[3] = 0x3c;    //��վ��ԿԶ�̸���ָ��INSΪ0x3c
       cmdBuf[4] = 0x00;
       cmdBuf[5] = 0x00;
       cmdBuf[6] = 0xFA;    //Lc
       cmdBuf[7] = 0x63;
       cmdBuf[8] = 0x80;
       
       //�Ự���I128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[136-i] = *pHandle++;
       }
       cmdBuf[137] = 0x62;
       cmdBuf[138] = 0x90;       

       //��վ��Կǰ118Bytes
       for(i=0;i<118;i++)
       {
    	   cmdBuf[139+i] = *(pHandle+143-i);
       }       
       
       //�ڶ�������
       cmdBuf[257] = 0x80;     
       cmdBuf[258] = 0x3c; //��վ��ԿԶ�̸���ָ��INSΪ0x3c
       cmdBuf[259] = 0x00;
       cmdBuf[260] = 0x00;
       cmdBuf[261] = 0x9c;

       //��վ��Կ��26Bytes
       for(i=0;i<26;i++)
       {
    	   cmdBuf[262+i] = *(pHandle+25-i);
       }       
       pHandle+=144;
       
       //�ն������8Bytes
       pHandle+=8;
       
       cmdBuf[288] = 0x60;
       cmdBuf[289] = 0x80;

       //����ǩ��128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[417-i] = *pHandle++;
       }
       
       result = read(fdOfIoChannel, cmdBuf, 2);
     	 break;
     	 
     case 8:    //�ն˶Գ���Կ����
       sprintf(say, "��Կ����(�ն˶Գ���Կ)");
     	 numOfKey = (lenOfWrite-128-128-8)/32;
     	 if (numOfKey<1 || numOfKey>4)
     	 {
     	 	 return 0;
     	 }
     	 
       cmdBuf[0] = 0x06;    //��һ�ֽ� - ��������
       cmdBuf[0] |= 0x80;   //���ȸ�256�ֽ���0x80��һλ
       cmdBuf[1] = 308+(numOfKey-1)*32-256; //�ڶ��ֽ� - ���������
       cmdBuf[2] = 0x90;
       cmdBuf[3] = 0x3a;    //�ն˶Գ���Կ����ָ��INSΪ0x3a
       cmdBuf[4] = 0x00;
       cmdBuf[5] = 0x00;
       cmdBuf[6] = 0xa5;    //Lc
       cmdBuf[7] = 0x63;
       cmdBuf[8] = 0x80;
       
       //�Ự���I128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[136-i] = *pHandle++;
       }
       cmdBuf[137] = 0x64;
       cmdBuf[138] = 0x00;
       cmdBuf[139] = numOfKey*32;

       //��һ����Կ32Bytes
       for(i=0;i<32;i++)
       {
    	   cmdBuf[140+i] = *(pHandle+numOfKey*32-1-i);
       }
       
       //�ڶ�������
       cmdBuf[172] = 0x80;     
       cmdBuf[173] = 0x3a; //�ն˶Գ���Կ����ָ��INSΪ0x3a
       cmdBuf[174] = 0x00;
       cmdBuf[175] = 0x00;
       cmdBuf[176] = (numOfKey-1)*32+0x82;

       //�ڶ���������Կ(N-1)32Bytes
       for(i=0;i<(numOfKey-1)*32;i++)
       {
    	   cmdBuf[177+i] = *(pHandle+(numOfKey-1)*32-1-i);
       }
       
       pHandle += numOfKey*32;
       
       //�ն������8Bytes
       pHandle += 8;
       
       tmpTail = 177+(numOfKey-1)*32;
       
       cmdBuf[tmpTail++] = 0x60;
       cmdBuf[tmpTail++] = 0x80;

       //����ǩ��128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[tmpTail+127-i] = *pHandle++;
       }
       
       result = read(fdOfIoChannel, cmdBuf, 2);

     	 break;
     
     default:
     	 return 0;
   }
    
   if (result==0)
   {
     swAnalyse(say, cmdBuf[0], cmdBuf[1]);
     
     if (cmdBuf[0]==0x90 && cmdBuf[1]==0x00)
     {
    	  ackOrNack(TRUE, dataFrom);
     }
     else
     {
       if (cmdBuf[0]==0x90 && (cmdBuf[1]==0x86 || cmdBuf[1]==0x84))
       {
         AFN00004(dataFrom, 2, NULL);
       }
       else
       {
         AFN00004(dataFrom, 1, NULL);
       }
     }
   }
   else
   {
     if (debugInfo&ESAM_DEBUG)
     {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("��Կ����:����У�����\n");
    	      break;
    	      
    	    case 2:
    	      printf("��Կ����:���յ�һ�ֽڲ���INS�ֽ�\n");
    	      break;
    	  }
     }
    
     AFN00004(dataFrom, 1, NULL);
   }
	 
	 return 0;
}

/*******************************************************
��������:AFN04009
��������:��Ӧ��վ�����֤����ԿЭ��"�ն˷ǶԳ���Կע��(F9)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
INT16U AFN06009(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	 INT8U  cmdBuf[280];
	 INT32U i;
	 INT8U  tmpCmd[5];
	 INT8U  result;
	 INT8U  *pData;
	 INT8U  ackBuf;
   INT8U  sw1,sw2;
   char   say[30];
   
   sw1 = 0;
   sw2 = 0;

	 pData = pHandle;
	 
   offset06 = 145;

	 pHandle += 4;
   
   cmdBuf[0] = 0x03;    //��һ�ֽ� - ��������
   cmdBuf[1] = 145;     //�ڶ��ֽ� - ���������
   cmdBuf[2] = 0x80;
   cmdBuf[3] = 0x36;
   if (*pHandle==0x03)   //���������Կ����,3��ʾ�ն˷ǶԳ���Կ��1ע��,4��ʾ�ն˷ǶԳ���Կ��2ע��
   {
   	 cmdBuf[4] = 0x01;
   	 strcpy(say, "�ն˷ǶԳ���Կ��1ע��");
   }
   else
   {
   	 cmdBuf[4] = 0x02;
   	 strcpy(say, "�ն˷ǶԳ���Կ��2ע��");
   }

	 pHandle++;
	 
	 cmdBuf[5] = 0x00;
	 cmdBuf[6] = 0x8C;

   cmdBuf[7] = 0x81;
	 cmdBuf[8] = 0x08;
   
   //��վ�����
   for(i=0;i<8;i++)
   {
  	 cmdBuf[16-i] = *pHandle++;
  	 //cmdBuf[9+i] = *pHandle++;
   }
   
   pHandle+=8;   //�ն������
   
   cmdBuf[17] = 0x60;
   cmdBuf[18] = 0x80;

   //����ǩ��
   for(i=0;i<128;i++)
   {
  	 cmdBuf[146-i] = *pHandle++;
  	 //cmdBuf[19+i] = *pHandle++;
   }
   
   result = read(fdOfIoChannel, cmdBuf, 256);
    
   if (result==0)
   {
     swAnalyse(say, cmdBuf[256], cmdBuf[257]);
    
     if (cmdBuf[256]==0x90 && cmdBuf[257]==0x00)
     {
       //���ݵ�Ԫ��ʶ
       msFrame[frameTail++] = *pData++;  //DA1
       msFrame[frameTail++] = *pData++;  //DA2
       msFrame[frameTail++] = *pData++;  //DT1
       msFrame[frameTail++] = *pData++;  //DT2
       
       msFrame[frameTail++] = *pData++;  //��������
       
       //�ն˹�Կ1��2
       for(i=256;i>0;i--)
       {
    	   msFrame[frameTail++] = cmdBuf[i-1];
    	 }
    	 
    	 return frameTail;
     }
     else
     {
     	 ;
     }
   }
   else
   {
     if (debugInfo&ESAM_DEBUG)
     {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("�ն˷ǶԳ���Կע��:����У�����\n");
    	      break;
    	      
    	    case 2:
    	      printf("�ն˷ǶԳ���Կע��:���յ�һ�ֽڲ���INS�ֽ�\n");
    	      break;
    	    
    	    case 3:
    	      printf("�ն˷ǶԳ���Կע��:����δ֪����\n");
    	      break;
    	  }
     }
   }
   
	 return 0;
}

/*******************************************************
��������:AFN04010
��������:��Ӧ��վ�����֤����ԿЭ��"�ն˷ǶԳ���Կ����(F10)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
INT16U AFN06010(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	 INT16U i;
	 INT8U  cmdBuf[300];
	 INT8U  result;
	 INT8U *pData;
	 char  say[30];
	 
	 pData = pHandle;
	 
   offset06 = 273;

	 pHandle += 4;
	 
   cmdBuf[0] = 0x04;     //��һ�ֽ� - ��������
   cmdBuf[0] |= 0x80;    //���ȱ�һ�ֽڴ�Ľ����λ��1����256
   cmdBuf[1] = 280-256;  //�ڶ��ֽ� - ���������
   cmdBuf[2] = 0x90;
   cmdBuf[3] = 0x38;
   if (*pHandle==0x05)   //������¹�Կ����,5��ʾ�ն˷ǶԳ���Կ��1����,6��ʾ�ն˷ǶԳ���Կ��2����
   {
   	 cmdBuf[4] = 0x01;
   	 strcpy(say, "�ն˷ǶԳ���Կ��1����");
   }
   else
   {
   	 cmdBuf[4] = 0x02;
   	 strcpy(say, "�ն˷ǶԳ���Կ��2����");
   }
	 pHandle++;
	 
	 cmdBuf[5] = 0x00;
	 cmdBuf[6] = 0x8C;
	 cmdBuf[7] = 0x63;
	 cmdBuf[8] = 0x80;
   
   //�Ự��Կ
   for(i=0;i<128;i++)
   {
  	 cmdBuf[136-i] = *pHandle++;
  	 //cmdBuf[9+i] = *pHandle++;
   }
   
   cmdBuf[137] = 0x81;
   cmdBuf[138] = 0x08;

   //��վ�����
   for(i=0;i<8;i++)
   {
  	 cmdBuf[146-i] = *pHandle++;
  	 //cmdBuf[139+i] = *pHandle++;
   }
   
   pHandle+=8;   //�ն������
   
   cmdBuf[147] = 0x80;
   cmdBuf[148] = 0x38;
   cmdBuf[149] = cmdBuf[4];   //P1
   cmdBuf[150] = 0x00;
   cmdBuf[151] = 0x82;
   cmdBuf[152] = 0x60;
   cmdBuf[153] = 0x80;
   
   //����ǩ��
   for(i=0;i<128;i++)
   {
  	 cmdBuf[281-i] = *pHandle++;
  	 //cmdBuf[154+i] = *pHandle++;
   }
   
   result = read(fdOfIoChannel, cmdBuf, 278);
   
   
   if (result==0)
   {
     swAnalyse(say, cmdBuf[276], cmdBuf[277]);
    
     if (cmdBuf[276]==0x90 && cmdBuf[277]==0x00)
     {
       //���ݵ�Ԫ��ʶ
       msFrame[frameTail++] = *pData++;  //DA1
       msFrame[frameTail++] = *pData++;  //DA2
       msFrame[frameTail++] = *pData++;  //DT1
       msFrame[frameTail++] = *pData++;  //DT2
       
       msFrame[frameTail++] = *pData++;  //��������
       
       pData += 128;

       //��վ�����8�ֽ�
       memcpy(&msFrame[frameTail], pData, 8);
       frameTail += 8;

       //�ն˹�Կ1��2(144�ֽ�)
       for(i=0; i<144; i++)
       {
    	   msFrame[frameTail+143-i] = cmdBuf[2+i];
    	 }
    	 frameTail += 144;


       //�ն�����ǩ��(128�ֽ�)
       for(i=0;i<128;i++)
       {
    	   msFrame[frameTail+127-i] = cmdBuf[148+i];
    	 }
    	 
    	 frameTail += 128;
    	 
    	 return frameTail;
     }
   }
   else
   {
     if (debugInfo&ESAM_DEBUG)
     {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("�ն˷ǶԳ���Կ����:����У�����\n");
    	      break;
    	      
    	    case 2:
    	      printf("�ն˷ǶԳ���Կ����:���յ�һ�ֽڲ���INS�ֽ�\n");
    	      break;
    	      
    	    case 3:
    	      printf("�ն˷ǶԳ���Կ����:����δ֪����\n");
    	      break;
    	  }
     }
   }
   
	 return 0;
}

/*******************************************************
��������:getChallenge
��������:ȡ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void getChallenge(INT8U *buf)
{
	INT8U cmdBuf[20];
	INT8U i;
	INT8U result;
  
  for(i=0;i<5;i++)
  {
    buf[0] = 0x01;     //��һ�ֽ� - ��������
    buf[1] = 0x05;     //�ڶ��ֽ� - ���������
    buf[2] = 0x00;     //CLA
    buf[3] = 0x84;     //INS
    buf[4] = 0x00;     //P01
    buf[5] = 0x00;     //P02
    buf[6] = 0x08;     //P03
    result = read(fdOfIoChannel, buf, 8);
    
    if (result==0)
    {
      if (buf[8]==0x90 && buf[9]==0x00)
      {
    	  if (debugInfo&ESAM_DEBUG)
    	  {
    	    printf("��%d���ͷ���,�ն������:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",i+1,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
    	  }
    	  break;
    	}
    }
    else
    {
    	if (debugInfo&ESAM_DEBUG)
    	{
    	  switch(result)
    	  {
    	    case 1:
    	      printf("����У�����\n");
    	      break;
    	      
    	    case 2:
    	      printf("���յ�һ�ֽڲ���INS�ֽ�\n");
    	      break;
    	  }
    	}
    }
  }
}

/*******************************************************
��������:getSerial
��������:ȡ���к�
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void getSerial(INT8U *buf)
{
	 buf[0] = 88;     //��һ�ֽ� - ��������
	 
	 read(fdOfIoChannel, buf, 8);
}

/*******************************************************
��������:getChallenge
��������:ȡ�����
���ú���:
�����ú���:
�������:ifSingle - =0,����ַ,  >0,���ַ���
�������:
����ֵ��void
*******************************************************/
INT8U calcMac(INT8U afn, INT8U ifSingle, INT8U *buf, INT16U lenOfBuf, INT8U *retMac)
{
	INT8U  cmdBuf[1024];
	INT8U  i;
	INT8U  result;
	INT8U  retValue = 0;
	INT8U  numOfBlock;
	INT16U frameTail;
	
	//������Ҫ�����Ŀ���
	numOfBlock = lenOfBuf/NUM_OF_MAC_BLOCK;
	if (lenOfBuf%NUM_OF_MAC_BLOCK)
	{
		numOfBlock++;
	}

  cmdBuf[0] = 0x05;                   //��һ�ֽ� - ��������
  cmdBuf[1] = lenOfBuf+numOfBlock*5;  //�ڶ��ֽ� - ���������  
  frameTail = 2;
  for(i=0;i<numOfBlock;i++)
  {
    if ((i+1)==numOfBlock)
    {
      cmdBuf[frameTail++] = 0x80;     //CLA,����ָ������һ��
    }
    else
    {
      cmdBuf[frameTail++] = 0x90;     //CLA,����ָ��
    }
    
    cmdBuf[frameTail++] = 0xe8;       //INS
    
    if (ifSingle==0)   //����ַ
    {
      cmdBuf[frameTail++] = 0x00;     //P01
      switch(afn)
      {
      	case SET_PARAMETER:
          cmdBuf[frameTail] = 0x02;   //P02
          break;
  
      	case CTRL_COMMAND:
          cmdBuf[frameTail] = 0x03;   //P02
          break;
        
        case FILE_TRANSPORT:
          cmdBuf[frameTail] = 0x04;   //P02
          break;
  
        case DATA_FORWARD:
          cmdBuf[frameTail] = 0x05;   //P02
          break;        	
  
      	default:
          cmdBuf[frameTail] = 0x01;   //P02
          break;
      }
    }
    else
    {
			switch(ifSingle)
		  {
		  	case 1:
          cmdBuf[frameTail++] = 0x15;     //P01
		  		break;
		  	case 2:
          cmdBuf[frameTail++] = 0x16;     //P01
		  		break;
		  	case 3:
          cmdBuf[frameTail++] = 0x17;     //P01
		  		break;
		  	case 4:
          cmdBuf[frameTail++] = 0x18;     //P01
		  		break;
		  	case 5:
          cmdBuf[frameTail++] = 0x19;     //P01
		  		break;
		  	case 6:
          cmdBuf[frameTail++] = 0x1a;     //P01
		  		break;
		  	case 7:
          cmdBuf[frameTail++] = 0x1b;     //P01
		  		break;
		  	case 8:
          cmdBuf[frameTail++] = 0x1c;     //P01
		  		break;
		  	
		  	//2011-06-24,�������Ժ���ϵͳ�㲥��ַ���е�MAC��֤�����ϸ�,
		  	//  ��Ϊ���Ժ�ṩ��������û��ϵͳ�㲥��ַ�ĳ�ʼ����,�ʵ��Ժ���˲�֪��ϵͳ�㲥��ַ��������0x1d
		  	case 9:
          cmdBuf[frameTail++] = 0x1d;     //P01
		  		break;
		  		
		    default:
          cmdBuf[frameTail++] = 0x00;     //P01
		    	break;
		  }
		  
    	switch(afn)
    	{
    		case RESET_CMD:      //AFN==01H
          cmdBuf[frameTail] = 0x0b;     //P02
    		  break;

    		case SET_PARAMETER:  //AFN==04H
          cmdBuf[frameTail] = 0x0c;     //P02
          break;

    		case CTRL_COMMAND:   //AFN==05H
          cmdBuf[frameTail] = 0x0d;     //P02
    		  break;
    		  
        case FILE_TRANSPORT:
          cmdBuf[frameTail] = 0x0e;     //P02
          break;
  
        case DATA_FORWARD:
          cmdBuf[frameTail] = 0x0f;     //P02
          break;    		  
    	}
    }
    cmdBuf[frameTail]<<=2;
      
    if (numOfBlock==1)
    {
      cmdBuf[frameTail] |= 0x3;       //����һ��������
    }
    else
    {
    	if (i==0)
    	{
    		 cmdBuf[frameTail] |= 1;      //��һ��������
    	}
    	else
    	{
    		if ((i+1)<numOfBlock)
    		{
    			 cmdBuf[frameTail] |= 2;    //�м伶����
    		}
    	}
    }
    frameTail++;

    if ((i+1)==numOfBlock)
    {
      cmdBuf[frameTail++] = lenOfBuf%NUM_OF_MAC_BLOCK;   //Lc
      memcpy(&cmdBuf[frameTail], &buf[i*NUM_OF_MAC_BLOCK], lenOfBuf%NUM_OF_MAC_BLOCK);
      frameTail += lenOfBuf%NUM_OF_MAC_BLOCK;
    }
    else
    {
      cmdBuf[frameTail++] = NUM_OF_MAC_BLOCK;   //Lc
      memcpy(&cmdBuf[frameTail], &buf[i*NUM_OF_MAC_BLOCK], NUM_OF_MAC_BLOCK);
      frameTail += NUM_OF_MAC_BLOCK;
    }
  }
  
  result = read(fdOfIoChannel, cmdBuf, 4);
  
  if (result==0)
  {
  	swAnalyse("MAC����",cmdBuf[4],cmdBuf[5]);
  	
    if (cmdBuf[4]==0x90 && cmdBuf[5]==0x00)
    {
  	  retValue = 1;
  	  memcpy(retMac, cmdBuf, 4);  	  
  	  
  	  if (debugInfo&ESAM_DEBUG)
  	  {
  	    printf("�ն˼�����MACֵ:%02x-%02x-%02x-%02x\n",retMac[0],retMac[1],retMac[2],retMac[3]);
  	  }
  	}
  }
  else
  {
  	if (debugInfo&ESAM_DEBUG)
  	{
  	  switch(result)
  	  {
  	    case 1:
  	      printf("MAC����:����У�����\n");
  	      break;
  	      
  	    case 2:
  	      printf("MAC����:���յ�һ�ֽڲ���INS�ֽ�\n");
  	      break;
  	  }
  	}
  }
  
  return retValue;
}

/*******************************************************
��������:swAnalyse
��������:��Ӧ״̬�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void swAnalyse(char *say, INT8U sw1,INT8U sw2)
{
  if (debugInfo&ESAM_DEBUG)
  {
  	 printf("��Ӧ״̬��:SW1=%02x,SW2=%02x\n", sw1, sw2);
  	 
  	 if (sw1==0x90 && sw2==0x00)
     {
  	    printf("%s:��ȷִ��\n", say);
  	 }
     
     if (sw1==0x90 && sw2==0x72)
     {
      	printf("%s:ָ��ṹ����\n",say);
     }

  	 if (sw1==0x90 && sw2==0x73)
     {
    	 printf("%s:SM1��Կ����\n", say);
     }

     if (sw1==0x90 && sw2==0x74)
     {
      	printf("%s:��ǩ�ļ����Ͳ�ƥ��\n", say);
     }
  
     if (sw1==0x90 && sw2==0x75)
     {
      	printf("%s:��ǩ�ļ�δ�ҵ�\n", say);
     }
     
     if(sw1==0x90 && sw2==0x76)
     {
      	printf("%s:����RSA��Կ��ʱôԿ�ļ�δ�ҵ�\n", say);
     }
     
     if (sw1==0x90 && sw2==0x77)
     {
      	printf("%s:�������ܵĹ�Կ�ļ���ƥ��\n", say);
     }
    
     if (sw1==0x90 && sw2==0x78)
     {
       printf("%s:�������ܵĹ�Կ�ļ�û�ҵ�\n", say);
     }
     
  	 if (sw1==0x90 && sw2==0x79)
     {
    	 printf("%s:�������ܵĹ��I�ļ���ƥ��\n", say);
     }
  	 if (sw1==0x90 && sw2==0x7a)
     {
    	 printf("%s:�������ܵĹ�Կ�ļ�û�ҵ�\n", say);
     }
  	 if (sw1==0x90 && sw2==0x7b)
     {
    	 printf("%s:����ǩ����˽Կ�ļ���ƥ��\n", say);
     }
  	 if (sw1==0x90 && sw2==0x7)
     {
    	 printf("%s:����ǩ����˽Կ�ļ�û�ҵ�\n", say);
     }
     if (sw1==0x90 && sw2==0x82)
     {
       printf("%s:RSA���ܴ���\n", say);
     }
     if (sw1==0x90 && sw2==0x86)
     {
       printf("%s:RSA��ǩ����\n",say);
     }
     if (sw1==0x90 && sw2==0x88)
     {
       printf("%s:RSA������Կ�Դ���\n",say);
     }
  	 if (sw1==0x90 && sw2==0x84)
     {
    	 printf("%s:RSA���ܴ���\n",say);
     }

  	 if (sw1==0x90 && sw2==0x8a)
     {
    	 printf("%s:RSAǩ������\n",say);
     }
  	 if (sw1==0x90 && sw2==0x8c)
     {
    	 printf("%s:SM1�������\n",say);
     }
     if (sw1==0x67 && sw2==0x00)
     {
    	 printf("%s:���ݳ��ȴ���\n", say);
     }
     
     if (sw1==0x69 && sw2==0x81)
     {
    	 printf("%s:P1��P2��ָ�ı�ʶ��������Ӧ�Ĺ�Կ�ļ�\n", say);
     }
     if (sw1==0x69 && sw2==0x82)
     {
    	 printf("%s:ʹ������������\n", say);
     }
     if (sw1==0x69 && sw2==0x83)
     {
    	 printf("%s:��Կ�ļ�δ�ҵ�\n", say);
     }
     
     if (sw1==0x69 && sw2==0x84)
     {
       printf("%s:û�п��������\n",say);
     }
     
     if (sw1==0x6a && sw2==0x80)
     {
    	 printf("%s:���ݸ�ʽ����\n", say);
     }
     



  }
}
       
