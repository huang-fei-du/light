/***************************************************
Copyright,2010,Huawei Wodian co.,LTD,All	Rights Reserved
�ļ�����AFN0C.c
���ߣ�Leiyong
�汾��0.9
������ڣ�2010��3��
��������վAFN09(�����ն����ü���Ϣ)�����ļ���
�����б�
     1.
�޸���ʷ��
  01,10-3-23,leiyong created.
***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMeter.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "convert.h"
#include "userInterface.h"

#include "AFN09.h"

extern INT8U  ackTail;

/*******************************************************
��������:AFN09
��������:��վ"�����ն����ü���Ϣ"(AFN09)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN09(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll)
{
    INT16U   tmpI,tmpFrameTail;
    INT8U    fn;
    INT8U    tmpDtCount;              //DT��λ����
    INT8U    tmpDt1;                  //��ʱDT1
    INT8U    *pTpv;                   //TpVָ��
    INT8U    maxCycle;                //���ѭ������
    INT16U   frameTail09;             //AFN09֡β
    INT16U   tmpHead09;               //AFN09��ʱ֡ͷ
    INT16U   tmpHead09Active;         //�����ϱ�AFN09��ʱ֡ͷ

    INT16U (*AFN09Fun[11])(INT16U frameTail,INT8U *pHandlem, INT8U fn);
    for (tmpI=0; tmpI<11; tmpI++)
    {
       AFN09Fun[tmpI] = NULL;
    }
       
    //��1
    AFN09Fun[0] = AFN09001;
    AFN09Fun[1] = AFN09002;
    AFN09Fun[2] = AFN09003;
    AFN09Fun[3] = AFN09004;
    AFN09Fun[4] = AFN09005;
    AFN09Fun[5] = AFN09006;
    AFN09Fun[6] = AFN09007;
    AFN09Fun[7] = AFN09008;
    
   #ifdef SDDL_CSM
    AFN09Fun[10] = AFN09011;
   #endif
    
    if (fQueue.tailPtr == 0)
    {
       tmpHead09 = 0;
    }
    else
    {
       tmpHead09 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }

    frameTail09 = tmpHead09 + 14;
    
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    tmpDt1 = 0;
    tmpDtCount = 0;
    ackTail = 0;
    maxCycle = 0;
    
    while ((frame.loadLen > 0) && (maxCycle<50))
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
           	  //ִ�к���
             #ifdef SDDL_CSM
              if (AFN09Fun[fn-1] != NULL && (fn <= 8 || 11==fn))
             #else
              if (AFN09Fun[fn-1] != NULL && fn <= 8)
             #endif
              {
                 tmpFrameTail = AFN09Fun[fn-1](frameTail09, pDataHead, fn);
                 if (tmpFrameTail== frameTail09)
                 {
                 	  ackData[ackTail*5] = *pDataHead;                             //DA1
                 	  ackData[ackTail*5+1] = *(pDataHead+1);                       //DA2
                 	  ackData[ackTail*5+2] = 0x1 << ((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
                 	  ackData[ackTail*5+3] = (fn-1)/8;                             //DT2
                 	  ackData[ackTail*5+4] = 0x01;                                 //����Ч����
                 	  ackTail++;
                 }
                 else
                 {
                 	  frameTail09 = tmpFrameTail;
                 }
              }
           }
           
           tmpDt1 >>= 1;
                      
           if ((frameTail09 - tmpHead09) > MAX_OF_PER_FRAME || (((pDataHead+4) == pDataEnd) && tmpDtCount==8))
           {
              //�����������ϱ������¼�����
              if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail09 - tmpHead09) > 16)
              {
              	  msFrame[frameTail09++] = iEventCounter;
              	  msFrame[frameTail09++] = nEventCounter;
              }
              
              //��������վҪ���ж��Ƿ�Я��TP
              if (frame.pTp != NULL)
              {
                 pTpv = frame.pTp;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv;
              }
              
              msFrame[tmpHead09 + 0] = 0x68;   //֡��ʼ�ַ�
            
              tmpI = ((frameTail09 - tmpHead09 - 6) << 2) | 0x2;
              msFrame[tmpHead09 + 1] = tmpI & 0xFF;   //L
              msFrame[tmpHead09 + 2] = tmpI >> 8;
              msFrame[tmpHead09 + 3] = tmpI & 0xFF;   //L
              msFrame[tmpHead09 + 4] = tmpI >> 8; 
            
              msFrame[tmpHead09 + 5] = 0x68;  //֡��ʼ�ַ�

       
              msFrame[tmpHead09 + 6] = 0x88;     //�����ֽ�10001000(DIR=1,PRM=0,������=0x8)

              if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
              {
                  msFrame[tmpHead09 + 6] |= 0x20;
              }
       
              //��ַ
              msFrame[tmpHead09 + 7] = addrField.a1[0];
              msFrame[tmpHead09 + 8] = addrField.a1[1];
              msFrame[tmpHead09 + 9] = addrField.a2[0];
              msFrame[tmpHead09 + 10] = addrField.a2[1];
              msFrame[tmpHead09 + 11] = addrField.a3;
              
              msFrame[tmpHead09 + 12] = 0x09;  //AFN
       
              msFrame[tmpHead09+13] = 0;
              
              if (frame.pTp != NULL)
              {
              	 msFrame[tmpHead09+13] |= 0x80;       //TpV��λ
              }
              
              msFrame[frameTail09+1] = 0x16;
              
              fQueue.frame[fQueue.tailPtr].head = tmpHead09;
              fQueue.frame[fQueue.tailPtr].len = frameTail09 + 2 - tmpHead09;
              
              if (((frameTail09 - tmpHead09) > 16 && frame.pTp==NULL) || ((frameTail09 - tmpHead09) > 22 && frame.pTp!=NULL))
              {
                 tmpHead09 = frameTail09+2;
                 if ((tmpHead09+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                 {
                    fQueue.frame[fQueue.tailPtr].next = 0x0;
                    fQueue.tailPtr = 0;
                    tmpHead09 = 0;
                 }
                 else
                 {                 
                    fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                    fQueue.tailPtr++;
                 }

                 frameTail09 = tmpHead09 + 14;  //frameTail������λ��д��һ֡
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
��������:AFN09001
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն˰汾��Ϣ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09001(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    char str[6];
    INT8U i;
    
    //���ݵ�Ԫ��ʶ
    msFrame[frameTail++] = *pHandle++;    //DA1
    msFrame[frameTail++] = *pHandle++;    //DA2
    msFrame[frameTail++] = 0x01;          //DT1
    msFrame[frameTail++] = 0x00;          //DT2
    
    //���ݵ�Ԫ
    //1.���̴���:4�ֽ�ASCII��
	  #ifdef CQDL_CSM
	   msFrame[frameTail++] = 'C';        //���̴���1(�ֵ�Ƽ�"C"��ASCII��)
	   msFrame[frameTail++] = 'Q';        //���̴���2(�ֵ�Ƽ�"Q"��ASCII��)
	   msFrame[frameTail++] = 'H';        //���̴���3(�ֵ�Ƽ�"H"��ASCII��)
	   msFrame[frameTail++] = 'W';        //���̴���4(�ֵ�Ƽ�"W"��ASCII��)
	  #else
	   msFrame[frameTail++] = csNameId[8];//���̴���1
	   msFrame[frameTail++] = csNameId[9];//���̴���2
	   msFrame[frameTail++] = csNameId[10];//���̴���3
	   msFrame[frameTail++] = csNameId[11];//���̴���4
	  #endif

	 /*
	 #ifdef OEM_BDZNDN 
	  msFrame[frameTail++] = 'Z';        //���̴���1(�������ܵ���"Z"��ASCII��)
	  msFrame[frameTail++] = 'N';        //���̴���2(�������ܵ���"N"��ASCII��)
	  msFrame[frameTail++] = 'D';        //���̴���3(�������ܵ���"D"��ASCII��)
	  msFrame[frameTail++] = 'N';        //���̴���4(�������ܵ���"N"��ASCII��)
	 #else
	  #ifdef OEM_YZKJ
	   msFrame[frameTail++] = 'Y';        //���̴���1(ӯ�޿Ƽ�"Y"��ASCII��)
	   msFrame[frameTail++] = 'Z';        //���̴���2(ӯ�޿Ƽ�"Z"��ASCII��)
	   msFrame[frameTail++] = 'S';        //���̴���3(ӯ�޿Ƽ�"S"��ASCII��)
	   msFrame[frameTail++] = 'C';        //���̴���4(ӯ�޿Ƽ�"C"��ASCII��)	    
	  #else
	   #ifdef OEM_GSPG	    
	    msFrame[frameTail++] = 'G';        //���̴���1(�����չ�"G"��ASCII��)
	    msFrame[frameTail++] = 'S';        //���̴���2(�����չ�"S"��ASCII��)
	    msFrame[frameTail++] = 'P';        //���̴���3(�����չ�"P"��ASCII��)
	    msFrame[frameTail++] = 'G';        //���̴���4(�����չ�"G"��ASCII��)
	   #else
	    #ifdef OEM_LCDZ
	     msFrame[frameTail++] = 'L';        //���̴���1(³�ɵ���"L"��ASCII��)
	     msFrame[frameTail++] = 'C';        //���̴���2(³�ɵ���"C"��ASCII��)
	     msFrame[frameTail++] = 'D';        //���̴���3(³�ɵ���"D"��ASCII��)
	     msFrame[frameTail++] = 'Z';        //���̴���4(³�ɵ���"Z"��ASCII��)
	    #else
	     #ifdef OEM_SCDL
	      msFrame[frameTail++] = 'S';        //���̴���1(��������"S"��ASCII��)
	      msFrame[frameTail++] = 'C';        //���̴���2(��������"C"��ASCII��)
	      msFrame[frameTail++] = 'D';        //���̴���3(��������"D"��ASCII��)
	      msFrame[frameTail++] = 'L';        //���̴���4(��������"L"��ASCII��)
	     #else
	      msFrame[frameTail++] = 'C';        //���̴���1(�ֵ�Ƽ�"C"��ASCII��)
	      msFrame[frameTail++] = 'Q';        //���̴���2(�ֵ�Ƽ�"Q"��ASCII��)
	      msFrame[frameTail++] = 'H';        //���̴���3(�ֵ�Ƽ�"H"��ASCII��)
	      msFrame[frameTail++] = 'W';        //���̴���4(�ֵ�Ƽ�"W"��ASCII��)
	     #endif
	    #endif
	   #endif
	  #endif
	 #endif
	 */

	  //2.�豸���:8�ֽ�ASCII��
	 #ifdef CQDL_CSM
	  intToString(deviceNumber, 3, str);
	  
	  //2010���2��"1002"
	  msFrame[frameTail++] = '1';
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '2';
	  
	  for(i=0;i<4-strlen(str);i++)
	  {
	    msFrame[frameTail++] = 0x30;
	  }
	 
	  for(i=0;i<strlen(str);i++)
	  {
	    msFrame[frameTail++] = str[i];    //�豸���1(ASCII��)
	  }
	 #else
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '0';
	  
	  #ifdef TE_ADDR_USE_BCD_CODE
	   intToString((addrField.a2[1]>>4)*1000 + (addrField.a2[1]&0xf)*100 + (addrField.a2[0]>>4)*10 + (addrField.a2[0]&0xf),3,str);
	  #else
	   intToString(addrField.a2[1]<<8 | addrField.a2[0],3,str);
	  #endif

	  for(i=0;i<5-strlen(str);i++)
	  {
	    msFrame[frameTail++] = 0x30;
	  }
	 
	  for(i=0;i<strlen(str);i++)
	  {
	    msFrame[frameTail++] = str[i];    //�ն˵�ַ��Ϊ�豸���1(ASCII��)
	  }
	 #endif
	
	  //3.�ն�����汾��:4�ֽ�ASCII
	  msFrame[frameTail++] = vers[0];        //�ն˰汾��1("v"��ASCII��)
	  msFrame[frameTail++] = vers[1];        //�ն˰汾��2("1"��ASCII��,������1�ɱ�)
	  msFrame[frameTail++] = vers[2];        //�ն˰汾��3("."��ASCII��)
	  msFrame[frameTail++] = vers[3];        //�ն˰汾��4("x"��ASCII��,����x�ɱ�)
	
	  //4.�ն������������:������
  	msFrame[frameTail++] = (dispenseDate[8]-0x30)<<4 | (dispenseDate[9]-0x30);    //��
  	msFrame[frameTail++] = (dispenseDate[5]-0x30)<<4 | (dispenseDate[6]-0x30);    //��
	  msFrame[frameTail++] = (dispenseDate[2]-0x30)<<4 | (dispenseDate[3]-0x30);    //��
	  
	  //5.�ն�����������Ϣ��:11�ֽ�ASCII��
	 
	  //DJGZ221002R
    //5.1
	  #ifdef PLUG_IN_CARRIER_MODULE
      msFrame[frameTail++] = 0x44;        //�ն�����������Ϣ��1("D"��ASCII��)
  	  msFrame[frameTail++] = 0x4A;        //�ն�����������Ϣ��2("J"��ASCII��)
  	#else
      msFrame[frameTail++] = 'F';        //�ն�����������Ϣ��1("F"��ASCII��)
  	  msFrame[frameTail++] = 'K';        //�ն�����������Ϣ��2("K"��ASCII��)  	
  	#endif
	  
	  //5.2ģ�����ͱ�ʶ
	  switch(moduleType)
	  {
	  	 case GPRS_SIM300C:
	  	 case GPRS_GR64:
	  	 case GPRS_M590E:
	  	 case GPRS_M72D:
	       msFrame[frameTail++] = 0x47;   //�ն�����������Ϣ��3("G"��ASCII��)
	       break;
	       
	  	 case CDMA_DTGS800:
	  	 case CDMA_CM180:
	       msFrame[frameTail++] = 0x43;   //�ն�����������Ϣ��3("C"��ASCII��)
	       break;

	  	 case ETHERNET:
	       msFrame[frameTail++] = 0x45;   //�ն�����������Ϣ��3("E"��ASCII��)
	       break;
	       
       case CASCADE_TE:
	       msFrame[frameTail++] = 'L';   //�ն�����������Ϣ��3("L"��ASCII��)
	       break;
			 
       case LTE_AIR720H:
	       msFrame[frameTail++] = '4';   //�ն�����������Ϣ��3("4"��ASCII��)
	       break;
	  	 
	  	 default:
	       msFrame[frameTail++] = 0x20;    //�ն�����������Ϣ��3(" "��ASCII��)
	       break;
	  } 
    
    //5.3
    #ifdef LOAD_CTRL_MODULE
  	  msFrame[frameTail++] = 'A';        //�ն�����������Ϣ��4("A"��ASCII��)
  	  msFrame[frameTail++] = '4';        //�ն�����������Ϣ��5("4"��ASCII��)
  	  msFrame[frameTail++] = '2';        //�ն�����������Ϣ��6("2"��ASCII��)
    #else
  	  msFrame[frameTail++] = 0x5a;        //�ն�����������Ϣ��4("Z"��ASCII��)
  	  msFrame[frameTail++] = 0x32;        //�ն�����������Ϣ��5("2"��ASCII��)
  	  msFrame[frameTail++] = 0x32;        //�ն�����������Ϣ��6("2"��ASCII��)
  	#endif

   #ifdef CQDL_CSM
   
    msFrame[frameTail++] = 'W';
    msFrame[frameTail++] = 'D';
    msFrame[frameTail++] = '0';
    msFrame[frameTail++] = '0';
    msFrame[frameTail++] = '1';
   
   #else
    //5.4CPUӲ���汾ȷ��
    //#ifdef CPU_HW_VER_1_0
     msFrame[frameTail++] = 0x31;      //�ն�����������Ϣ��7("1"��ASCII��)
     msFrame[frameTail++] = 0x30;      //�ն�����������Ϣ��8("0"��ASCII��)
    //#else
    // msFrame[frameTail++] = 0x20;      //�ն�����������Ϣ��7(" "��ASCII��)
    // msFrame[frameTail++] = 0x20;      //�ն�����������Ϣ��8(" "��ASCII��)
    //#endif
    
    //5.5���Ƶ�ԪӲ���汾ȷ��
    #ifdef JZQ_CTRL_BOARD_V_0_3
      msFrame[frameTail++] = 0x30;    //�ն�����������Ϣ��09("0"��ASCII��)
      msFrame[frameTail++] = 0x33;    //�ն�����������Ϣ��10("3"��ASCII��)
    #else
     #ifdef JZQ_CTRL_BOARD_V_1_4
      msFrame[frameTail++] = 0x31;    //�ն�����������Ϣ��09("1"��ASCII��)
      msFrame[frameTail++] = 0x34;    //�ն�����������Ϣ��10("4"��ASCII��)
     #else        
      msFrame[frameTail++] = 0x30;    //�ն�����������Ϣ��09("0"��ASCII��)
      msFrame[frameTail++] = 0x30;    //�ն�����������Ϣ��10("0"��ASCII��)
     #endif
    #endif
    
    //5.6ʱ���Ƿ���Ӳ������ʱ��
    if (bakModuleType==MODEM_PPP)  //2012-11-08,��Ӵ��ж�
    {
      msFrame[frameTail++] = 'P';      //�ն�����������Ϣ��11("P"��ASCII��)ʹ��PPP����
    }
    else
    {
     #ifdef RTC_USE_ISL12022M
       msFrame[frameTail++] = 0x49;    //�ն�����������Ϣ��11("I"��ASCII��)EPSON RX8025     
     #else
       msFrame[frameTail++] = 0x20;    //�ն�����������Ϣ��11(" "��ASCII��)      
     #endif
    }
   #endif 
    
    //6.�ն�ͨ��Э�顢�汾��4Bytes(376.1)
   #ifdef CQDL_CSM
    msFrame[frameTail++] = 'C';
    msFrame[frameTail++] = 'Q';
    msFrame[frameTail++] = '0';
    msFrame[frameTail++] = '4';
   #else
    msFrame[frameTail++] = 0x33;
    msFrame[frameTail++] = 0x37;
    msFrame[frameTail++] = 0x36;
    #ifdef SDDL_CSM
     msFrame[frameTail++] = 0x20;
    #else
     msFrame[frameTail++] = 0x31;
    #endif
   #endif
    
    //7.�ն�Ӳ���汾��4Bytes ASCII
	  msFrame[frameTail++] = hardwareVers[0];
	  msFrame[frameTail++] = hardwareVers[1];
	  msFrame[frameTail++] = hardwareVers[2];
	  msFrame[frameTail++] = hardwareVers[3];	
    
    //8.�ն�Ӳ����������:�ա��¡���
  	msFrame[frameTail++] = (hardwareDate[8]-0x30)<<4 | (hardwareDate[9]-0x30);    //��
  	msFrame[frameTail++] = (hardwareDate[5]-0x30)<<4 | (hardwareDate[6]-0x30);    //��
	  msFrame[frameTail++] = (hardwareDate[2]-0x30)<<4 | (hardwareDate[3]-0x30);    //��
    
    return frameTail;   
}

/*******************************************************
��������:AFN09002
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵ����롢�����ͨ�Ŷ˿�����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09002(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //ly,2011-5-21,add
   register int fd, interface, retn = 0;
   struct ifreq buf[MAXINTERFACES];
   struct ifconf ifc;
   INT8U  ifFoundMac;

   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x02;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //����������·��
   msFrame[frameTail++] = NUM_OF_SWITCH_PULSE;

   //����������·��
   msFrame[frameTail++] = NUM_OF_SWITCH_PULSE;
   
   //ֱ��ģ��������·��
   msFrame[frameTail++] = NUM_OF_ADC;
   
   //���������·��(�ִ�)
   #ifdef COLLECTOR
    msFrame[frameTail++] = 0;
   #else
    msFrame[frameTail++] = 0;
   #endif
   
   //֧�ֵĳ����ܱ�/��������װ�������� 2040
   msFrame[frameTail++] = 0xf8;
   msFrame[frameTail++] = 0x7;
   
   //֧�ֵ��ն�����ͨ�������ջ������ֽ��� 9600
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x08;
   
   //֧�ֵ��ն�����ͨ������ͻ������ֽ���
   msFrame[frameTail++] = 0x80;
   msFrame[frameTail++] = 0x25;
   
   //�ն�MAC��ַ�ζ�
   //ly,2011-05-21,����eth0��mac
   ifFoundMac = 0;
   if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
   {
     ifc.ifc_len = sizeof buf;
     ifc.ifc_buf = (caddr_t) buf;
     if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
     {
       interface = ifc.ifc_len / sizeof(struct ifreq);
       while (interface-- > 0)
       {
         if (strstr(buf[interface].ifr_name,"eth0"))
         {
          /*Get HW ADDRESS of the net card */
           if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf[interface]))) 
           {   
               ifFoundMac = 1;
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[0]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[1]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[2]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[3]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[4]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[5]);
           }
         }
       }//end of while
     }
     else
     {
       perror("cpm: ioctl");
     }
   }
   else
   {
     perror("cpm: socket");
   }

   close(fd);
   
   if (ifFoundMac==0)
   {
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
   }
   
   //ͨ�Ŷ˿�����
   msFrame[frameTail++] = 0x03;
   
   //��һ��ͨ�Ŷ˿ںż���Ϣ��(ר��0,��׼�첽���п�0,ֱ��RS485)
  #ifdef RS485_1_USE_PORT_1
   msFrame[frameTail++] = 0x1;  //�˿�1
  #else
   msFrame[frameTail++] = 0x2;  //�˿�2
  #endif
   msFrame[frameTail++] = 0x0;
   
   //��һ��ͨ�Ŷ˿�֧�ֵ���߲�����115200
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0xc2;
   msFrame[frameTail++] = 0x01;
   msFrame[frameTail++] = 0x00;
   
   //��һ��ͨ�Ŷ˿�֧�ֵ��豸���� 32��
   msFrame[frameTail++] = 0x20;
   msFrame[frameTail++] = 0x00;
   
   //��һ��ͨ�Ŷ˿�֧�ֵ������ջ������ֽ���
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x02;

   //��һ��ͨ�Ŷ˿�֧�ֵ�����ͻ������ֽ���
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x01;
   
   //�ڶ���ͨ�Ŷ˿ںż���Ϣ��(ר��0,��׼�첽���п�0,ֱ��RS485)
  #ifdef RS485_1_USE_PORT_1 
   msFrame[frameTail++] = 0x2;  //�˿�2
  #else
   msFrame[frameTail++] = 0x3;  //�˿�3
  #endif
   msFrame[frameTail++] = 0x0;
   
   //�ڶ���ͨ�Ŷ˿�֧�ֵ���߲�����115200
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0xc2;
   msFrame[frameTail++] = 0x01;
   msFrame[frameTail++] = 0x00;
   
   //�ڶ���ͨ�Ŷ˿�֧�ֵ��豸���� 32��
   msFrame[frameTail++] = 0x20;
   msFrame[frameTail++] = 0x00;
   
   //�ڶ���ͨ�Ŷ˿�֧�ֵ������ջ������ֽ���
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x02;

   //�ڶ���ͨ�Ŷ˿�֧�ֵ�����ͻ������ֽ���
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x01;

   //������ͨ�Ŷ˿ںż���Ϣ��(̨����ѹ����,��׼�첽���п�0,���нӿ�����խ����ѹ�ز�ͨ��ģ��,�˿�31)
   msFrame[frameTail++] = 0x2<<5 | 0x1f;
   msFrame[frameTail++] = 0x2<<5;
   
   //������ͨ�Ŷ˿�֧�ֵ���߲�����115200
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0xc2;
   msFrame[frameTail++] = 0x01;
   msFrame[frameTail++] = 0x00;
   
   //������ͨ�Ŷ˿�֧�ֵ��豸���� 1500��
   msFrame[frameTail++] = 0xdc;
   msFrame[frameTail++] = 0x05;
   
   //������ͨ�Ŷ˿�֧�ֵ������ջ������ֽ���
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x02;

   //������ͨ�Ŷ˿�֧�ֵ�����ͻ������ֽ���
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x01;
   
   return frameTail;
}

/*******************************************************
��������:AFN09003
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵ���������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09003(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x04;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //֧�ֵĲ����������� 2040
   msFrame[frameTail++] = 0xf8;
   msFrame[frameTail++] = 0x07;
   
   //֧�ֵ��ܼ���������� 8
   msFrame[frameTail++] = 0x08;
   
   //֧�ֵ����������� 64
   msFrame[frameTail++] = 0x40;
   
   //֧�ֵ��й��ܵ��ܵ����������� 0
   msFrame[frameTail++] = 0x00;
   
   //֧�ֵ��������� 8
   msFrame[frameTail++] = 0x08;
	 
	 //֧�ֵĲ�����������󶳽��ܶ�  15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //֧�ֵ��ܼ����й�����������󶳽��ܶ� 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //֧�ֵ��ܼ����޹�����������󶳽��ܶ� 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //֧�ֵ��ܼ����й�������������󶳽��ܶ� 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //֧�ֵ��ܼ����޹�������������󶳽��ܶ� 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //֧�ֵ����������������  31��
	 msFrame[frameTail++] = 0x1f;
	 
	 //֧�ֵ������ݴ������ 12��
	 msFrame[frameTail++] = 0x0c;
	 
	 //֧�ֵ�ʱ�ι��ض�ֵ����������  0
	 msFrame[frameTail++] = 0x0;
	 
	 //֧�ֵ�г��������г������ 0
	 msFrame[frameTail++] = 0x0;
	 
	 //֧�ֵ��޹������������������ 0
	 msFrame[frameTail++] = 0x0;

   //֧�ֵ�̨�����г����ص��û���໧�� 20
   msFrame[frameTail++] = 0x14;
	 
	 //֧�ֵ��û�����ű�־
	 msFrame[frameTail++] = 0x3f;   //00111111
	 msFrame[frameTail++] = 0x00;
	 
	 //ly,2011-05-13,֧��0��15���û��������û�С��Ÿ���
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 
	 return frameTail;
}

/*******************************************************
��������:AFN09004
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵĲ�������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09004(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x08;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //֧����Ϣ������n
   msFrame[frameTail++] = 11;
   
   //֧�ֵĵ�1����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0xff;
   
   //֧�ֵĵ�2����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0xff;

   //֧�ֵĵ�3����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x7f;

   //֧�ֵĵ�4����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x7f;

   //֧�ֵĵ�5����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x7f;

   //֧�ֵĵ�6����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0xff;

   //֧�ֵĵ�7����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x01;

   //֧�ֵĵ�8����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x1f;

   //֧�ֵĵ�9����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x0f;

   //֧�ֵĵ�10����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x0f;

   //֧�ֵĵ�11����Ϣ��������Ӧ����Ϣ���ޱ�־λ
   msFrame[frameTail++] = 0x07;
   
   return frameTail;
}

/*******************************************************
��������:AFN09005
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵĿ�������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09005(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x10;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //֧����Ϣ������n
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

/*******************************************************
��������:AFN09006
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵ�1����������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09006(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x20;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //֧����Ϣ������n
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

/*******************************************************
��������:AFN09007
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵ�2����������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09007(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x40;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //֧����Ϣ������n
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

/*******************************************************
��������:AFN09008
��������:��Ӧ��վ�����ն����ü���Ϣ"�ն�֧�ֵ��¼���¼����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09008(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //���ݵ�Ԫ��ʶ
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x80;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //֧���¼���¼��־λ
   msFrame[frameTail++] = 0x8f;
   msFrame[frameTail++] = 0xbf;
   msFrame[frameTail++] = 0x99;
   msFrame[frameTail++] = 0xff;
   msFrame[frameTail++] = 0x07;
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

#ifdef SDDL_CSM

/*******************************************************
��������:AFN09011
��������:��Ӧ��վ�����ն����ü���Ϣ"F11 �ն���Ч������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN09011(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG meterConfig;

  INT16U tmpFrameTail;
  INT16U numOfDa;
  INT16U numOfMp;
  INT8U  tmpDa[255];
  INT16U k;
  
  //���ݵ�Ԫ��ʶ
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = 0x04;          //DT1
  msFrame[frameTail++] = 0x01;          //DT2
  
  tmpFrameTail = frameTail;
  
  frameTail+=4;

	//DA����ֵ
	memset(tmpDa, 0x0, 255);
	numOfMp = 0;
	for(k=1; k<=2040; k++)
	{
  	if (selectF10Data(k, 0, 0, (INT8U *)&meterConfig, sizeof(meterConfig))==TRUE)
  	{
  		numOfMp++;
  				
  		tmpDa[(k-1)/8] |= 0x01<<((k%8 == 0) ? 7 : (k%8-1));
    }
  }
	
	//ͳ��DA����
	numOfDa = 0;
	for(k=0; k<255; k++)
	{
		if (tmpDa[k]!=0)
		{
			numOfDa++;
			msFrame[frameTail++] = tmpDa[k];
			msFrame[frameTail++] = k+1; 
		}
	}
	
	//���������
	msFrame[tmpFrameTail] = numOfMp&0xff;
	msFrame[tmpFrameTail+1] = numOfMp>>8&0xff;
	
	//DA�б���
	msFrame[tmpFrameTail+2] = numOfDa&0xff;
	msFrame[tmpFrameTail+3] = numOfDa>>8&0xff;
	
  return frameTail;
}

#endif

