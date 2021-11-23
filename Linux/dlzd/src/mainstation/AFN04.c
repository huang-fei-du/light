/***************************************************
Copyright,2010,Huawei WoDian co.,LTD
�ļ�����AFN0A.c
���ߣ�Leiyong
�汾��0.9
������ڣ�2010��1��
��������վAFN0A(��ѯ����)�����ļ���
�����б�
  01,10-01-13,wan guihua created.
  02,10-03-22,Leiyong,modify,��ԭ����һ�����ݱ�ʶ��¼��һ���¼���Ϊһ�η��Ĳ�����¼��һ���¼�
  03,10-11-23,Leiyong,�����������˾����ʱ����:���������������·��׳�ʱ,
              ԭ��Ϊ: AFN04���·�������Ҫ����վ��һ���в������õ��¼�,��������վ���·�����ʱ�������
              �ϱ��¼�����Ϊ��ʱ���������ĳ�������F10��F25���¼������ϱ������Ժ����ʱ�ϱ�
  04,10-12-17,Leiyong,���ݹ��繫˾���Է���:F38,F39 1�ࡢ2�������������óɹ����ٲⲻ����
              ԭ��:ԭ���������ǲ���ȷ��,ֻ����һ������ŵ�λ��,����ԼҪ���������16������ŵ�����,
              ����:��Ϊ��������16������ŵ����á�
  05,11-05-16,Leiyong,Ϊ��Ӧ������վ�·�����ʱFN���ص�������,���pDataUnit04���ݵ�Ԫָ��,
              �Ա�ÿ��FN�ܷ��ʵ��Լ������ݵ�Ԫ.
              ����ԭ��Ҳ�ǰ����˼·������,���д���,�����������ڵ����ϵͳ�в���ͨ��.
***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"
#include "msSetPara.h"
#include "dataBase.h"
#include "teRunPara.h"
#include "att7022b.h"
#include "workWithMeter.h"
#include "reportTask.h"
#include "copyMeter.h"
#include "userInterface.h"

#include "AFN00.h"
#include "AFN04.h"

//����
INT16U offset04;                     //���յ���֡�е����ݵ�Ԫƫ����(�������ݱ�ʶ���ֽ�)
INT8U  *pDataUnit04;                 //04�����ݵ�Ԫָ��

/*******************************************************
��������:AFN04
��������:��վ"���ò���"(AFN04)������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN04(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   char    say[20],str[8];
   INT16U  i;
   INT8U   fn, ackTail;
   INT8U   eventData[512];
   INT8U   paraNumCount=0;        //��������ͳ��

   INT8U   ackAll,nAckAll;        //ȫ��ȷ��,���ϱ�־
   INT8U   pseudoFrame[6];
      
   INT8U   maxCycle;              //���ѭ������
   INT8U   tmpDtCount;            //DT��λ����
   INT8U   tmpDt1;                //��ʱDT1

   BOOL    (*AFN04Fun[138])(INT8U *p);
   
   //�������
   bzero(ackData, 100);

   for(i=0;i<138;i++)
   {
     AFN04Fun[i] = NULL;
   }
   
   //��1
   AFN04Fun[0] = AFN04001;
   AFN04Fun[1] = AFN04002;
   AFN04Fun[2] = AFN04003;
   AFN04Fun[3] = AFN04004;
   AFN04Fun[4] = AFN04005;
   AFN04Fun[5] = AFN04006;
   AFN04Fun[6] = AFN04007;
   AFN04Fun[7] = AFN04008;
   
   //��2
   AFN04Fun[8] = AFN04009;
   AFN04Fun[9] = AFN04010;
   AFN04Fun[10] = AFN04011;
   AFN04Fun[11] = AFN04012;
   AFN04Fun[12] = AFN04013;
   AFN04Fun[13] = AFN04014;
   AFN04Fun[14] = AFN04015;
   AFN04Fun[15] = AFN04016;
   
   //��3
   AFN04Fun[16] = AFN04017;
   AFN04Fun[17] = AFN04018;
   AFN04Fun[18] = AFN04019;
   AFN04Fun[19] = AFN04020;
   AFN04Fun[20] = AFN04021;
   AFN04Fun[21] = AFN04022;
   AFN04Fun[22] = AFN04023;
   
   //��4
   AFN04Fun[24] = AFN04025;
   AFN04Fun[25] = AFN04026;
   AFN04Fun[26] = AFN04027;
   AFN04Fun[27] = AFN04028;
   
   AFN04Fun[28] = AFN04029;
   AFN04Fun[29] = AFN04030;
   AFN04Fun[30] = AFN04031;
   //AFN04Fun[31] = AFN04032;	//����
   
   //��5
   AFN04Fun[32] = AFN04033;
  
   AFN04Fun[33] = AFN04034;
   AFN04Fun[34] = AFN04035;
   AFN04Fun[35] = AFN04036;
   AFN04Fun[36] = AFN04037;
   AFN04Fun[37] = AFN04038;
   AFN04Fun[38] = AFN04039;
   //AFN04Fun[39] = AFN04040;	//����
   
  #ifndef LIGHTING
   //��6
   AFN04Fun[40] = AFN04041;
   AFN04Fun[41] = AFN04042;
   AFN04Fun[42] = AFN04043;
   AFN04Fun[43] = AFN04044;
   AFN04Fun[44] = AFN04045;
   AFN04Fun[45] = AFN04046;
   AFN04Fun[46] = AFN04047;
   AFN04Fun[47] = AFN04048;
   
   //��7
   AFN04Fun[48] = AFN04049;
  #else
   AFN04Fun[49] = AFN04050;
   AFN04Fun[50] = AFN04051;
   AFN04Fun[51] = AFN04052;
	 
   AFN04Fun[52] = AFN04053;    //2018-06-27,��Ӻ����������
   AFN04Fun[53] = AFN04054;    //2018-07-10,��ӳ�����������
   AFN04Fun[54] = AFN04055;    //2018-07-10,��Ӳ�����������
  #endif    //LIGHTING
   
   //��8
   AFN04Fun[56] = AFN04057;
   AFN04Fun[57] = AFN04058;
   AFN04Fun[58] = AFN04059;
   AFN04Fun[59] = AFN04060;
   AFN04Fun[60] = AFN04061;
   //AFN04Fun[61] = AFN04062;
   
   //��9
   AFN04Fun[64] = AFN04065;
   AFN04Fun[65] = AFN04066;
   AFN04Fun[66] = AFN04067;
   AFN04Fun[67] = AFN04068;
   
   //��10
   AFN04Fun[72] = AFN04073;
   AFN04Fun[73] = AFN04074;
   AFN04Fun[74] = AFN04075;
   AFN04Fun[75] = AFN04076;
   
   //��11
   AFN04Fun[80] = AFN04081;
   AFN04Fun[81] = AFN04082;
   AFN04Fun[82] = AFN04083;
   
   //��չ
  #ifdef SDDL_CSM
   AFN04Fun[87] = AFN04088;
  #endif

   AFN04Fun[96] = AFN04097;
   AFN04Fun[97] = AFN04098;
   AFN04Fun[120] = AFN04121;
   AFN04Fun[128] = AFN04129;
  
   AFN04Fun[98] = AFN04099;
   AFN04Fun[99] = AFN04100;
   
   AFN04Fun[130] = AFN04131;
   AFN04Fun[132] = AFN04133;
   AFN04Fun[133] = AFN04134;
   AFN04Fun[134] = AFN04135;
   AFN04Fun[135] = AFN04136;
   AFN04Fun[136] = AFN04137;
   AFN04Fun[137] = AFN04138;
   
   ackAll = 0;
   nAckAll = 0;
   ackTail = 0;
   maxCycle = 0;
   
   printf("AFN04 loadLen=%d\n",frame.loadLen);
   
   while ((frame.loadLen > 0) && (maxCycle<50))
   {
      maxCycle++;
      
      offset04 = 0;
      
      tmpDt1 = *(pDataHead + 2);
      tmpDtCount = 0;
      while(tmpDtCount < 9)
      {
         tmpDtCount++;
         if ((tmpDt1 & 0x1) == 0x1)
         {
            fn = *(pDataHead + 3) * 8 + tmpDtCount;
            
            //ר���ն�,�޳�Ͷ��ʱ,����Ӧ���ı�����ֵ
           #ifndef PLUG_IN_CARRIER_MODULE
            if (toEliminate==CTRL_JUMP_IN)
            {
      	      if (fn==17)
      	      {
      	 	      return;
      	      }
            }
           #endif
            
            //ly,2011-05-14,pDataUnit04�Ǳ�FN�����ݵ�Ԫ��ʼָ��
            pDataUnit04 = pDataHead + 4 + offset04;

            if (debugInfo&WIRELESS_DEBUG)
            {
              printf("AFN04 fn=%d,offset04=%d,��FN������ǰ�����ֽ�:%02x-%02x-%02x\n", fn, offset04,*pDataUnit04,*(pDataUnit04+1),*(pDataUnit04+2));
            }
            
      	   #ifdef SDDL_CSM
      	    //����һ�����⣬û����AFN04Fun[223]������Ŷ
      	    //Ϊ�˽�ʡRAM
      	    if (224==fn)
      	    {
              AFN04224(pDataHead);
              
              ackAll++;

              goto thisWayPlease;
      	    }
      	   #endif
            
            if (fn>138)
            {
      	       maxCycle = 50;
      	       break;
      	    }

            if (AFN04Fun[fn-1] != NULL 
            	  && (fn <= 83 
            	    #ifdef SDDL_CSM
            	     || (88==fn)
            	    #endif
            	      || fn==92 || fn==97 || fn==98 || fn==99 || fn==100 || fn==121 || fn==129 || (fn>=131 && fn<=138)))
            {
               //����ȷ��/������д
               ackData[ackTail*5]   = *pDataHead;                         //DA1
               ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
               ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
               ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
               
               if (AFN04Fun[fn-1](pDataHead) == TRUE)
               {
                  ackAll++;
                  ackData[ackTail*5+4] = 0x00;                            //��ȷ
    	            
    	            if (fn>0 && fn<84)
    	            {
    	              paraStatus[(fn-1)/8] |= 1<<((fn-1)%8);               //��"�ն˲���״̬"λ
    	            }
               }
               else
               {
                  nAckAll++;
                  ackData[ackTail*5+4] = 0x01;                            //����
                  break;
               }
               ackTail++;
               
               //��������ʾ��
               if ((fn>17 && fn<21) || (fn>41 && fn<50))   //ly,2011-10-12,add,ֻ�п��Ʋ����Ÿ�����ʾ��
               {
         	       setBeeper(BEEPER_ON);         //������
         	     }
         	     alarmLedCtrl(ALARM_LED_ON);      //ָʾ����
               setParaWaitTime = SET_PARA_WAIT_TIME;
               lcdBackLight(LCD_LIGHT_ON);
               
               lcdLightOn = LCD_LIGHT_ON;
               lcdLightDelay = nextTime(sysTime, 0, SET_PARA_WAIT_TIME);
            }
         }
         
thisWayPlease:

         tmpDt1 >>= 1;
      }
      
      if (maxCycle==50)
      {
      	break;
      }
      
   	  eventData[9 +paraNumCount*4] = *pDataHead;
   	  eventData[10+paraNumCount*4] = *(pDataHead+1);
   	  eventData[11+paraNumCount*4] = *(pDataHead+2);
   	  eventData[12+paraNumCount*4] = *(pDataHead+3);
      paraNumCount++;
            
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("ִ�к�offset04=%d\n",offset04);
      }

      if (frame.loadLen < offset04+4)
      {
      	 break;
      }
      else
      {
         frame.loadLen -= (offset04 + 4);
         pDataHead = pDataHead + offset04 + 4;
      }
   }
  
   //ȷ�ϡ����ϡ����ȷ��/����
   if (ackAll !=0  &&  nAckAll!=0)
   {
      AFN00003(ackTail, dataFrom, 0x04);      //����ȷ�ϻ����
   }
   else
   {
      if (nAckAll==0)
      {
         ackOrNack(TRUE,dataFrom);            //ȫ��ȷ��
      }
      else
      {
         ackOrNack(FALSE,dataFrom);           //ȫ������
      }
   }

   //��¼�¼������
   
   if (((eventRecordConfig.iEvent[0] & 0x04) || (eventRecordConfig.nEvent[0] & 0x04)) && paraNumCount>0)
   {
   	 eventData[0] = 0x3;
   	 eventData[1] = paraNumCount;       //��¼�����ݱ�ʶ����
   	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
   	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
   	 eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
   	 eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
   	 eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
   	 eventData[7] = sysTime.year/10<<4   | sysTime.year%10;
   	 eventData[8] = addrField.a3>>1 & 0xff;
   	  
     if (eventRecordConfig.iEvent[0] & 0x04)
     {
   	    writeEvent(eventData, 9+paraNumCount*4, 1, dataFrom);  //������Ҫ�¼�����
   	 }
     if (eventRecordConfig.nEvent[0] & 0x04)
     {
   	    writeEvent(eventData, 9+paraNumCount*4, 2, dataFrom);  //����һ���¼�����
   	 }
   	  
   	 eventStatus[0] = eventStatus[0] | 0x04;
     
     
     //ly,2010-11-19,�����������˾����ʱ����,���ɵ���վ�������·����ַ����ʱ��������ϱ��¼���
     //   ��������ʱ,���,��������õ��ն˲������ϱ����¼�
     //ly,2011-01-29,ȡ��ֻ�������Լ�Ų��ϱ�10��26
     //#ifdef CQDL_CSM
      //if (fn!=10 && fn!=25 && fn!=3 && fn!=121)
      if (fn!=10 && fn!=25)
      {
     //#endif

        if (debugInfo&PRINT_EVENT_DEBUG)
        {
          printf("AFN04���������ϱ�\n");
        }
     
        activeReport3();   //�����ϱ��¼�
     
     //#ifdef CQDL_CSM
      }
     //#endif
   }
}

/*******************************************************
��������:AFN04001
��������:��Ӧ��վ���ò�������"�ն�ͨ�Ų���(F1)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04001(INT8U *pData)
{
	//���ݵ�Ԫ��ʶ
	pData += 4;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  commPara.rts = *pData++;
  commPara.delay = *pData++;
 
  commPara.timeOutReSendTimes[0] = *pData++;
  commPara.timeOutReSendTimes[1] = *pData++;
 
  commPara.flagOfCon = *pData++;
    
  if (*pData!=0)
  {
    commPara.heartBeat = *pData;
  }
    
  offset04 += 6;
  saveParameter(0x04, 1,(INT8U *)&commPara,sizeof(COMM_PARA));
  
  return TRUE;
}

/*******************************************************
��������:AFN04002
��������:��Ӧ��վ���ò�������"�ն�����ͨ�ſ������м�ת������(F2)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04002(INT8U *pData)
{
	INT16U i;
	INT8U  tempData;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;

  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//��ת�����ն˵�ַ��n��ת������/��ֹ��־
	tempData = *pData++;
	
	//�����ת���ĵ�ַ����>16�Ļ�,�����ܸò��������÷��ط���
	if((tempData & 0x7F) > 16)
	{
		offset04 += 1 + (tempData & 0x7F) * 2;
		*pData += (tempData & 0x7F) * 2;
		return FALSE;
	}
	
	//��ת�����ն˵�ַ���������ֹ��־
	relayConfig.relayAddrNumFlg = tempData;
	
	//��ת�����ն˵�ַ
	for(i=0;i<(tempData & 0x7F);i++)
	{
		relayConfig.relayAddr[i][0] = *pData++;
		relayConfig.relayAddr[i][1] = *pData++;
	}
	
	offset04 += 1 + (tempData & 0x7F) * 2;
	
  saveParameter(0x04, 2,(INT8U *)&relayConfig,sizeof(RELAY_CONFIG));
    
  return TRUE;
}

/*******************************************************
��������:AFN04003
��������:��Ӧ��վ���ò�������"��վIP��ַ�Ͷ˿�(F3)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04003(INT8U *pData)
{
   INT8U i;
   
   //���ݵ�Ԫ��ʶ
   pData+=4;

   //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
  
   //����IP��ַ�Ͷ˿�
   for (i=0;i<4;i++)
   {
     ipAndPort.ipAddr[i] = *pData++;
   }
   ipAndPort.port[0] = *pData++;
   ipAndPort.port[1] = *pData++;
   
   //����IP��ַ�Ͷ˿�
   for (i=0;i<4;i++)
   {
     ipAndPort.ipAddrBak[i] = *pData++;
   }
   ipAndPort.portBak[0] = *pData++;
   ipAndPort.portBak[1] = *pData++;
  
   //APN
   for (i=0;i<16;i++)
   {
     ipAndPort.apn[i] = *pData++;
   }
     
   offset04 += 28;
 
   saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
   
   saveBakKeyPara(3);    //2012-8-9,add

   return TRUE;
}

/*******************************************************
��������:AFN04004
��������:��Ӧ��վ���ò�������"��վ�绰����Ͷ������ĺ���(F4)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04004(INT8U *pData)
{
	INT16U i;
	
  //���ݵ�Ԫ��ʶ
	pData+=4;

  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//��վ�绰����
	for(i=0;i<8;i++)
	{
		phoneAndSmsNumber.phoneNumber[i] = *pData++;
	}
	
	//�������ĺ���
	for(i=0;i<8;i++)
	{
		phoneAndSmsNumber.smsNumber[i] = *pData++;
	}
	
	offset04 += 16;
	
  saveParameter(0x04, 4,(INT8U *)&phoneAndSmsNumber,sizeof(PHONE_AND_SMS));
  
  return TRUE;
	
}

/*******************************************************
��������:AFN04005
��������:��Ӧ��վ���ò�������"�ն�����ͨ����Ϣ��֤��������(F5)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04005(INT8U *pData)
{
	//���ݵ�Ԫ��ʶ
	pData+=4;

  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  //��Ϣ��֤������
	messageAuth[0] = *pData++;
	
	//��Ϣ��֤��������2Bytes
	messageAuth[1] = *pData++;
	messageAuth[2] = *pData++;
	
	offset04 += 3;
	
	saveParameter(0x04, 5, (INT8U *)&messageAuth, 3);
	
	return TRUE;
}

/*******************************************************
��������:AFN04006
��������:��Ӧ��վ���ò�������"�ն�����ͨ����Ϣ��֤��������(F6)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04006(INT8U *pData)
{
	 INT16U i;
	 
	 //���ݵ�Ԫ��ʶ
	 pData+=4;

   //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

   //���ַ
	 for(i=0;i<16;i++)
	 {
		 groupAddr[i] = *pData++;
	 }
	
	 offset04 += 16;

	 saveParameter(0x04, 6, (INT8U *)groupAddr, 16);	 
	 
	 return TRUE;
}

/*******************************************************
��������:AFN04007
��������:��Ӧ��վ���ò�������"�ն�IP��ַ�Ͷ˿�(F7)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04007(INT8U *pData)
{
  //09��Լ�������ն�IP��ַ�Ͷ˿�
 
  TE_IP_AND_PORT tempTeIpAndPort;

  INT16U i, errFlg=0;
 
  //���ݵ�Ԫ��ʶ
  pData+=4;

  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;

  //�ն�IP��ַ
  for(i=0;i<4;i++)
  {
    tempTeIpAndPort.teIpAddr[i] = *pData++;
  }

  //��������
  for(i=0;i<4;i++)
  {
	  tempTeIpAndPort.mask[i] = *pData++;
  }

  //����
  for(i=0;i<4;i++)
  {
	  tempTeIpAndPort.gateWay[i] = *pData++;
  }

  //��������
  tempTeIpAndPort.proxyType = *pData++;

  //�����������ַ
  for(i=0;i<4;i++)
  {
	  tempTeIpAndPort.proxyServer[i] = *pData++;
  }

  //����������˿�
  tempTeIpAndPort.proxyPort[0] = *pData++;
  tempTeIpAndPort.proxyPort[1] = *pData++;

  //������������ӷ�ʽ
  tempTeIpAndPort.proxyLinkType = *pData++;

  //�û�������
  tempTeIpAndPort.userNameLen = *pData++;
  if(tempTeIpAndPort.userNameLen > 20)
  {
	  errFlg = 1;
	  pData += tempTeIpAndPort.userNameLen;
  }
  else
  {
	  //�û���
	  for(i=0;i<tempTeIpAndPort.userNameLen;i++)
	  {
	 	  tempTeIpAndPort.userName[i] = *pData++;
	  }
  }

  //���볤��
  tempTeIpAndPort.passwordLen = *pData++;
  if(tempTeIpAndPort.passwordLen > 20)
  {
	  errFlg = 1;
	  pData += tempTeIpAndPort.passwordLen;
  }
  else
  {
	  //����
	  for(i=0;i<tempTeIpAndPort.passwordLen;i++)
	  {
		  tempTeIpAndPort.password[i] = *pData++;
	  }
  }

  //�ն������˿�
  tempTeIpAndPort.listenPort[0] = *pData++;
  tempTeIpAndPort.listenPort[1] = *pData++;

  offset04 += (24 + tempTeIpAndPort.userNameLen + tempTeIpAndPort.passwordLen);

  if(errFlg == 1)
  {
	  return FALSE;
  }

  bzero(&teIpAndPort, sizeof(TE_IP_AND_PORT));
  teIpAndPort = tempTeIpAndPort;
 
  saveIpMaskGateway(teIpAndPort.teIpAddr,teIpAndPort.mask,teIpAndPort.gateWay);  //���浽rcS��,ly,2011-04-12
 
  saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));
 
  return TRUE;
}

/*******************************************************
��������:AFN04008
��������:��Ӧ��վ���ò�������"�ն�����ͨ�Ź�����ʽ(F8)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04008(INT8U *pData)
{
	 //09��Լ���ն�����ͨ�Ź�����ʽ(��̫��������ר��)
	 
	 INT16U i;
	
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

	 //����ģʽ
	 tePrivateNetMethod.workMethod = *pData++;
	
	 //�������ߣ�ʱ������ģʽ�ز����
	 tePrivateNetMethod.redialInterval[0] = *pData++;
	 tePrivateNetMethod.redialInterval[1] = *pData++;
	
	 //��������ģʽ�ز�����
	 tePrivateNetMethod.maxRedial = *pData++;
	
	 //��������ģʽ������ͨ���Զ�����ʱ��
	 tePrivateNetMethod.closeConnection = *pData++;
	
	 //ʱ������ģʽ��������ʱ�α�־
	 for(i=0;i<3;i++)
	 {
		 tePrivateNetMethod.onLinePeriodTime[i] = *pData++;
	 }
	
	 offset04 += 8;
	 
	 saveParameter(0x04, 8, (INT8U *)&tePrivateNetMethod, 8);

	 return TRUE;
}

/*******************************************************
��������:AFN04009
��������:��Ӧ��վ���ò�������"�ն��¼���¼��������(F9)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04009(INT8U *pData)
{
	 INT16U i;
	
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

	 //�¼���¼��Ч��־λ
	 for(i=0;i<8;i++)
	 {
		 eventRecordConfig.nEvent[i] = *pData++;
	 }
	
	 //�¼���Ҫ�Եȼ���־λ
	 for(i=0;i<8;i++)
	 {
		 eventRecordConfig.iEvent[i] = *pData++;
	 }
	
	 offset04 += 16;
	 
	 saveParameter(0x04, 9, (INT8U *)&eventRecordConfig, 16);
	 
	 return TRUE;
}

/*******************************************************
��������:AFN04010
��������:��Ӧ��վ���ò�������"�ն˵��ܱ�/��������װ�����ò���(F10)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE

��ʷ��
    1.ly,2012-08-09,�޳�ȫ0�ı��ַ
*******************************************************/
BOOL AFN04010(INT8U *pData)
{
	METER_DEVICE_CONFIG meterDeviceConfig[2040];
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //һ��������ͳ���¼�����(��ʱ���޹���)
	DATE_TIME tmpTime;

	INT16U i, j, len;
	
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	pData+=4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	len = *pData++;
	len |= (*pData++) << 8;
	if(len > 2040)
	{
		offset04 += 2 + len * 27; 
		pData += len * 27;
		return FALSE;
	}
	
	for(i=0;i<len;i++)
	{
		//���ܱ�/��������װ�����
		meterDeviceConfig[i].number = *pData++;
		meterDeviceConfig[i].number |= (*pData++)<<8;
		
		//2012-09-06,modify
		//if(meterDeviceConfig[i].number > 2040 || meterDeviceConfig[i].number < 1)
	 #ifndef LIGHTING    //2015-12-05,add
		if(meterDeviceConfig[i].number > 2040)
		{
			errFlg = 1;
		}
	 #endif
		
		//�����������
		meterDeviceConfig[i].measurePoint = *pData++;
		meterDeviceConfig[i].measurePoint |= (*pData++)<<8;
		if(meterDeviceConfig[i].measurePoint > 2040)
		{
			errFlg = 1;
		}
		
		//ͨ�����ʼ�ͨ�Ŷ˿ں�
		meterDeviceConfig[i].rateAndPort = *pData++;
		
		//ͨ��Э������
		meterDeviceConfig[i].protocol = *pData++;
		
		//ͨ�ŵ�ַ
		for(j=0;j<6;j++)
	  {
			meterDeviceConfig[i].addr[j] = *pData++;
			
			//2013-05-07,�жϱ��ַ�Ƿ�Ƿ�
			if ((meterDeviceConfig[i].addr[j]&0xf)>9 || (meterDeviceConfig[i].addr[j]>>4&0xf)>9)
			{
				errFlg = 1;
			}
		}
		
		
		//ͨ������
		//  2014-09-05,�����������вɼ�����ַ�������������ռ�ձȵĶ�Ӧֵ�е�����
		for(j=0;j<6;j++)
	  {
			meterDeviceConfig[i].password[j] = *pData++;
		}
		
		//���ܷ��ʸ���
		meterDeviceConfig[i].numOfTariff = *pData++;
	 #ifndef LIGHTING	
		if((meterDeviceConfig[i].numOfTariff & 0x3F) > 48)
		{
			errFlg = 1;
		}
	 #endif
		
		//�й�����ʾֵ����λ��С��λ����
		meterDeviceConfig[i].mixed = *pData++;
		
		//�����ɼ���ͨ�ŵ�ַ		
		for(j=0;j<6;j++)
		{
			meterDeviceConfig[i].collectorAddr[j] = *pData++;

		  //2014-09-05,�����������вɼ�����ַ�������������ռ�ձȵĶ�Ӧֵ�е�����
		 #ifndef LIGHTING	
			//2013-05-07,�жϲɼ�����ַ�Ƿ�Ƿ�
			if ((meterDeviceConfig[i].collectorAddr[j]&0xf)>9 || (meterDeviceConfig[i].collectorAddr[j]>>4&0xf)>9)
			{
				errFlg = 1;
			}
		 #endif
		}
		
		//�û�����ż��û�С���
		meterDeviceConfig[i].bigAndLittleType = *pData++;
	}

	offset04 += (2 + len * 27);

	if(errFlg == 1)
	{
		return FALSE;
	}
		
	//�����ܱ�/��������װ�����ô���base_vice_info(������Ϣ����)����
	for(j=0; j<i; j++)
	{
	 #ifdef LIGHTING
		//2015-11-14,���ڲ����������������,��������ͬ�Ĳ���ɾ����
		saveDataF10(meterDeviceConfig[j].measurePoint, meterDeviceConfig[j].rateAndPort&0x1f, meterDeviceConfig[j].addr,
		   meterDeviceConfig[j].measurePoint, (INT8U *)&meterDeviceConfig[j], 27);
		
    //���Ƶ���ʱ���޹���
		tmpTime = timeHexToBcd(sysTime);
		if (readMeterData((INT8U *)&meterStatisExtranTimeS, meterDeviceConfig[j].measurePoint, STATIS_DATA, 99, &tmpTime, 0)==TRUE)
		{
			if (meterStatisExtranTimeS.mixed & 0x20)
			{
				//���ʱ����ͬ����־
				meterStatisExtranTimeS.mixed &= 0xdf;
        
				//�洢���Ƶ�ͳ������(��ʱ���޹���)
        saveMeterData(meterDeviceConfig[j].measurePoint, meterDeviceConfig[j].rateAndPort&0x1f, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
				
				if (debugInfo&PRINT_CARRIER_DEBUG)
				{
					printf("������Ƶ�%dʱ��ͬ����־\n", meterDeviceConfig[j].measurePoint);
				}
			}
		}
	 #else
		saveDataF10(meterDeviceConfig[j].measurePoint, meterDeviceConfig[j].rateAndPort&0x1f, meterDeviceConfig[j].addr,
			 meterDeviceConfig[j].number, (INT8U *)&meterDeviceConfig[j], 27);
	 #endif
	}
	
	//��F10�ĵ��ܱ�/��������װ���������������base_info(������Ϣ��)����
	countParameter(0x04, 10, &meterDeviceNum);
	saveParameter(0x04, 10, (INT8U *)&meterDeviceNum, 2);
	
 #ifdef PLUG_IN_CARRIER_MODULE
   carrierFlagSet.msSetAddr = 1;       //��վ���ñ��ַ
  #ifdef DKY_SUBMISSION
   carrierFlagSet.msSetWait = nextTime(sysTime, 0, 5);
   if (debugInfo&PRINT_CARRIER_DEBUG)
   {
     printf("�ȴ�5���ӽ���վ���õı��ַͬ��������ͨ��ģ��\n");
   }
  #else
   carrierFlagSet.msSetWait = nextTime(sysTime, 1, 0);
   if (debugInfo&PRINT_CARRIER_DEBUG)
   {
     printf("�ȴ�һ���ӽ���վ���õı��ַͬ��������ͨ��ģ��\n");
   }
  #endif        	 	  	      	 
 #endif
 
 #ifdef SUPPORT_ETH_COPY
  setEthMeter = 1;
  msSetWaitTime = nextTime(sysTime, 0, 20);
  if (debugInfo&METER_DEBUG)
  {
    printf("�ȴ�20���ӽ����ַ���µ���̫������������\n");
  }
 #endif
 
 #ifdef LIGHTING
  initXlcLink();
  initLdgmLink();
  initLsLink();
  initAcLink();
	initThLink();    //2018-08-04��Add
 #endif

	return TRUE;
}

/*******************************************************
��������:AFN04011
��������:��Ӧ��վ���ò�������"�ն��������ò���(F11)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04011(INT8U *pData)
{
	INT8U len;
	
	INT16U i;
	
	//���ݵ�Ԫ��ʶ
	pData+=4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	len = *pData++;
	
	if(len > NUM_OF_SWITCH_PULSE)
	{
		pData += len * 5;
		offset04 += 1 + len * 5;
		return FALSE;
	}
	
	//�������ø���
	pulseConfig.numOfPulse = len;
	
	//��������
	for(i=0;i<pulseConfig.numOfPulse;i++)
	{
		pulseConfig.perPulseConfig[i].ifNo = *pData++;							//��������˿ں�
		pulseConfig.perPulseConfig[i].pn   = *pData++;							//�����������
		pulseConfig.perPulseConfig[i].character = *pData++;					//��������
		pulseConfig.perPulseConfig[i].meterConstant[0] = *pData++;	//�����K
		pulseConfig.perPulseConfig[i].meterConstant[1] = *pData++;
	}
	
	offset04 += (1 + pulseConfig.numOfPulse * 5);
	
	saveParameter(0x04, 11, (INT8U *)&pulseConfig, sizeof(PULSE_CONFIG));
  
  #ifdef PULSE_GATHER
    fillPulseVar(2);
  #endif
    	
	return TRUE;
}

/*******************************************************
��������:AFN04012
��������:��Ӧ��վ���ò�������"�ն�״̬���������(F12)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04012(INT8U *pData)
{
 	//���ݵ�Ԫ��ʶ
 	pData+=4;
 	
 	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

 	//״̬�������־λ
 	statusInput[0] = *pData++;
 	
 	//״̬�����Ա�־λ
 	statusInput[1] = *pData++;
 	
 	offset04 += 2;
 	saveParameter(0x04, 12, (INT8U *)statusInput, 2);
   
  ifSystemStartup = 0;
 	
 	return TRUE;
}

/*******************************************************
��������:AFN04013
��������:��Ӧ��վ���ò�������"�ն˵�ѹ/����ģ�������ò���(F13)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04013(INT8U *pData)
{
	IU_SIMULATE_CONFIG tempSimuIUConfig;
	
	INT16U i;
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	pData+=4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	tempSimuIUConfig.numOfSimu = *pData++;
	if(tempSimuIUConfig.numOfSimu > 64)
	{
		offset04 += 1 + tempSimuIUConfig.numOfSimu * 3;
		pData += tempSimuIUConfig.numOfSimu * 3;
		return FALSE;
	}
	
	for(i=0;i<tempSimuIUConfig.numOfSimu;i++)
	{
		tempSimuIUConfig.perIUConfig[i].ifNo = *pData++;
		tempSimuIUConfig.perIUConfig[i].pn = *pData++;
		tempSimuIUConfig.perIUConfig[i].character = *pData++;
	}
	
	offset04 += (1 + tempSimuIUConfig.numOfSimu * 3);
	bzero(&simuIUConfig, sizeof(IU_SIMULATE_CONFIG));
	simuIUConfig = tempSimuIUConfig;
	saveParameter(0x04, 12, (INT8U *)&simuIUConfig, sizeof(IU_SIMULATE_CONFIG));

	return TRUE;
}

/*******************************************************
��������:AFN04014
��������:��Ӧ��վ���ò�������"�ն��ܼ������ò���(F14)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04014(INT8U *pData)
{
	TOTAL_ADD_GROUP tempTotalAddGroup;
	
	INT8U  i, j, temp, errFlg = 0;
	
	offset04 = 0;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	tempTotalAddGroup.numberOfzjz = *pData++;
	offset04++;
	if(tempTotalAddGroup.numberOfzjz > 8 || tempTotalAddGroup.numberOfzjz < 1)
	{
		//�����FNռ�е��ֽ���
		for(i=0;i<tempTotalAddGroup.numberOfzjz;i++)
		{
			pData++;
			temp = *pData++;
			pData += temp;
			offset04 += 2 + temp;
		}
		return FALSE;
	}
	
	for(i=0;i<tempTotalAddGroup.numberOfzjz;i++)
	{
		//�ܼ������
		tempTotalAddGroup.perZjz[i].zjzNo = *pData++;
		offset04++;
		if(tempTotalAddGroup.perZjz[i].zjzNo < 1 || tempTotalAddGroup.perZjz[i].zjzNo > 8)
		{
			errFlg = 1;
		}
		
		//�ܼ������������
		tempTotalAddGroup.perZjz[i].pointNumber = *pData++;
		offset04++;
		if(tempTotalAddGroup.perZjz[i].pointNumber > 64)
		{
			errFlg = 1;
			for(j=0;j<tempTotalAddGroup.perZjz[i].pointNumber;j++)
			{
				pData++;
				offset04++;
			}
		}
		else
		{
			//��n��������ż��ܼӱ�־
			for(j=0;j<tempTotalAddGroup.perZjz[i].pointNumber;j++)
			{
				tempTotalAddGroup.perZjz[i].measurePoint[j] = *pData++;
				offset04++;
			}
		}
	}

	if(errFlg == 1)
	{
		return FALSE;
	}  	
	
	bzero(&totalAddGroup, sizeof(TOTAL_ADD_GROUP));
	totalAddGroup = tempTotalAddGroup;
	saveParameter(0x04, 14,(INT8U *)&totalAddGroup, sizeof(TOTAL_ADD_GROUP));

  return TRUE;
}

/*******************************************************
��������:AFN04015
��������:��Ӧ��վ���ò�������"�й��ܵ������Խ���¼���������(F15)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04015(INT8U *pData)
{
 	ENERGY_DIFFERENCE_CONFIG tempDifferenceConfig;
	
	INT8U  i, j, errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	tempDifferenceConfig.numOfConfig = *pData++;
	offset04++;
	if(tempDifferenceConfig.numOfConfig > 8 || tempDifferenceConfig.numOfConfig < 1)
	{
		pData += tempDifferenceConfig.numOfConfig * 9;
		offset04 += tempDifferenceConfig.numOfConfig * 9;
		
		return FALSE;
	}
	
	for(i=0;i<tempDifferenceConfig.numOfConfig;i++)
	{
		//�й��ܵ�����������
		tempDifferenceConfig.perConfig[i].groupNum = *pData++;
		if(tempDifferenceConfig.perConfig[i].groupNum > 8
			|| tempDifferenceConfig.perConfig[i].groupNum < 1)
			errFlg = 1;
		
		tempDifferenceConfig.perConfig[i].toCompare = *pData++;			//�Աȵ��ܼ������
		tempDifferenceConfig.perConfig[i].toReference = *pData++;		//���յ��ܼ������
		if(tempDifferenceConfig.perConfig[i].toCompare > 8
			|| tempDifferenceConfig.perConfig[i].toReference > 8)
			errFlg = 1;
			
		tempDifferenceConfig.perConfig[i].timeAndFlag = *pData++;					//�����ĵ�������ʱ�����估�Աȷ�����־
		tempDifferenceConfig.perConfig[i].ralitaveDifference = *pData++;	//�Խ�����ƫ��ֵ
		
		//�Խ�޾���ƫ��ֵ
		for(j=0;j<4;j++)
		{
			tempDifferenceConfig.perConfig[i].absoluteDifference[j] = *pData++;
		}
		
		tempDifferenceConfig.perConfig[i].startStop = 0;
		
		offset04 += 9;
	}
	
	if(errFlg == 1)
	{
		return FALSE;
	}
	
	bzero(&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
	differenceConfig = tempDifferenceConfig;
	saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));

  return TRUE;
}

/*******************************************************
��������:AFN04016
��������:��Ӧ��վ���ò�������"����ר���û���������(F16)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04016(INT8U *pData)
{
	INT8U  i;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//����ר���û���
	for(i=0;i<32;i++)
	{
		vpn.vpnName[i] = *pData++;
		
		offset04++;
	}
	
	//����ר������
	for(i=0;i<32;i++)
	{
		vpn.vpnPassword[i] = *pData++;
		offset04++;
	}
	
	saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));
	
	saveBakKeyPara(16);    //2012-8-9,add

  return TRUE;
}

/*******************************************************
��������:AFN04017
��������:��Ӧ��վ���ò�������"�ն˱�����ֵ(F17)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04017(INT8U *pData)
{
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//������ֵ
	protectLimit[0] = *pData++;
	protectLimit[1] = *pData++;
	
	offset04 += 2;
		
	saveParameter(0x04, 17,(INT8U *)protectLimit, 2);
  return TRUE;
}

/*******************************************************
��������:AFN04018
��������:��Ӧ��վ���ò�������"�ն˹���ʱ��(F18)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04018(INT8U *pData)
{
	 INT16U i;
	
	 //���ݵ�Ԫ��ʶ
	 pData += 4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

	 //�ն˹���ʱ��
	 for(i=0;i<12;i++)
	 {
		 ctrlPara.pCtrlPeriod[i] = *pData++;
	 }
	
	 offset04 += 12;
	 	
	 saveParameter(0x04, 18,(INT8U *)&ctrlPara, sizeof(CONTRL_PARA));
   return TRUE;
}

/*******************************************************
��������:AFN04019
��������:��Ӧ��վ���ò�������"�ն�ʱ�ι��ض�ֵ����ϵ��(F19)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04019(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData += 4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

	 //ʱ�ι��ض�ֵ����ϵ��
	 ctrlPara.pCtrlIndex = *pData++;
	
	 offset04 += 1;
	 	
	 saveParameter(0x04, 18,(INT8U *)&ctrlPara, sizeof(CONTRL_PARA));
   return TRUE;
}

/*******************************************************
��������:AFN04020
��������:��Ӧ��վ���ò�������"�ն��µ������ض�ֵ����ϵ��(F20)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04020(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData += 4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

	 //ʱ�ι��ض�ֵ����ϵ��
	 ctrlPara.monthEnergCtrlIndex = *pData++;
	
	 offset04 += 1;
		
	 saveParameter(0x04, 18,(INT8U *)&ctrlPara, sizeof(CONTRL_PARA));
   return TRUE;
}

/*******************************************************
��������:AFN04021
��������:��Ӧ��վ���ò�������"�ն˵���������ʱ�κͷ�����(F21)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04021(INT8U *pData)
{
	INT16U i;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//�ն˵���������ʱ�κͷ�����
	for(i=0;i<49;i++)
	{
		periodTimeOfCharge[i] = *pData++;
	}
	
	offset04 += 49;
		
	saveParameter(0x04, 21,(INT8U *)periodTimeOfCharge, 49);
  
  return TRUE;
}

/*******************************************************
��������:AFN04022
��������:��Ӧ��վ���ò�������"�ն˵���������(F22)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04022(INT8U *pData)
{
	 INT16U i, j;
	
	 //���ݵ�Ԫ��ʶ
	 pData += 4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

	 //������
	 chargeRateNum.chargeNum = *pData++;
	 offset04++;
	
	 //����
	 for(i=0;i<chargeRateNum.chargeNum;i++)
	 {
		 for(j=0;j<4;j++)
		 {
		   chargeRateNum.chargeRate[i][j] = *pData++;
		 	 offset04++;
		 }
	 }
	
	 saveParameter(0x04, 22,(INT8U *)&chargeRateNum, sizeof(CHARGE_RATE_NUM));
   
   return TRUE;
}

/*******************************************************
��������:AFN04023
��������:��Ӧ��վ���ò�������"�ն˴߷Ѹ澯����(F23)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04023(INT8U *pData)
{
	INT16U i;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//�߷Ѹ澯�����ֹ��־λ
	for(i=0;i<3;i++)
	{
		chargeAlarm[i] = *pData++;
	}
	
	offset04 += 3;
	
	saveParameter(0x04, 23,(INT8U *)chargeAlarm, 3);
  return TRUE;
}

/*******************************************************
��������:AFN04025
��������:��Ӧ��վ���ò�������"�������������(FN25)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04025(INT8U *pData)
{
	MEASURE_POINT_PARA pointPara;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&pointPara, sizeof(MEASURE_POINT_PARA));
			
			//��ѹ����������
			pointPara.voltageTimes = *pData | *(pData+1)<<8;
			pData += 2;
	
			//��������������
			pointPara.currentTimes = *pData | *(pData+1)<<8;
			pData += 2;
	
			//���ѹ
			pointPara.ratingVoltage = *pData | *(pData+1)<<8;
			pData += 2;
	
			//�����
			pointPara.maxCurrent = *pData++;
	
			//�����
			pointPara.powerRating[0] = *pData++;
			pointPara.powerRating[1] = *pData++;
			pointPara.powerRating[2] = *pData++;
	
			//��Դ���߷�ʽ
			pointPara.linkStyle = *pData++;
   
  		offset04 += 11;
  
 		 	saveViceParameter(0x04, 25, pn, (INT8U *)&pointPara, sizeof(CHARGE_RATE_NUM));
		}
		da1 >>=1;
	}
  
  #ifdef PULSE_GATHER
    fillPulseVar(2);
  #endif
  
  return TRUE;
}

/*******************************************************
��������:AFN04026
��������:��Ӧ��վ���ò�������"��������ֵ����(FN26)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04026(INT8U *pData)
{
	MEASUREPOINT_LIMIT_PARA pointLimitPara;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&pointLimitPara, sizeof(MEASUREPOINT_LIMIT_PARA));
		 
			//��ѹ�ϸ����б����
  		pointLimitPara.vUpLimit = *pData | *(pData+1)<<8;							//��ѹ�ϸ�����
  		pointLimitPara.vLowLimit = *(pData+2) | *(pData+3)<<8;				//��ѹ�ϸ�����
  		pointLimitPara.vPhaseDownLimit = *(pData+4) | *(pData+5)<<8;	//��ѹ��������
  		pData += 6;
 		  offset04 += 6;
  
  		//��ѹ�б����
  		pointLimitPara.vSuperiodLimit = *pData | *(pData+1)<<8;;	    //��ѹ������(��ѹ����)
  		pData += 2;
  		pointLimitPara.vUpUpTimes = *pData++;												  //��ѹ������Խ�޳���ʱ��
  		pointLimitPara.vUpUpResume[0] = *pData++;										  //��ѹ������Խ�޻ָ�ϵ��
  		pointLimitPara.vUpUpResume[1] = *pData++;
  		offset04 += 5;
   
  		//Ƿѹ�б����
  		pointLimitPara.vDownDownLimit = *pData | *(pData+1)<<8;;	//��ѹ������(Ƿѹ����)
  		pData += 2;
  		pointLimitPara.vDownDownTimes = *pData++;			//��ѹ������Խ�޳���ʱ��
  		pointLimitPara.vDownDownResume[0] = *pData++;	//��ѹ������Խ�޻ָ�ϵ��
  		pointLimitPara.vDownDownResume[1] = *pData++;
  		offset04 += 5;
  
  		//�����б����
  		pointLimitPara.cSuperiodLimit[0] = *pData++;	//�����������(��������
  		pointLimitPara.cSuperiodLimit[1] = *pData++;
  		pointLimitPara.cSuperiodLimit[2] = *pData++;
  		pointLimitPara.cUpUpTimes = *pData++;					//�����������Խ�޳���ʱ��
  		pointLimitPara.cUpUpReume[0] = *pData++;			//�����������Խ�޻ָ�ϵ��
  		pointLimitPara.cUpUpReume[1] = *pData++;
  		offset04 += 6;
  
  		//��������б����
  		pointLimitPara.cUpLimit[0] = *pData++;				//���������(���������)
  		pointLimitPara.cUpLimit[1] = *pData++;
  		pointLimitPara.cUpLimit[2] = *pData++;
  		pointLimitPara.cUpTimes = *pData++;						//���������Խ��ʱ��
  		pointLimitPara.cUpResume[0] = *pData++;				//���������Խ�޻ָ�ϵ��
  		pointLimitPara.cUpResume[1] = *pData++;
  		offset04 += 6;
  
  		//������������б����
 	 		pointLimitPara.cZeroSeqLimit[0] = *pData++;		//�����������
  		pointLimitPara.cZeroSeqLimit[1] = *pData++;
  		pointLimitPara.cZeroSeqLimit[2] = *pData++;
  		pointLimitPara.cZeroSeqTimes = *pData++;			//�������Խ�޳���ʱ��
  		pointLimitPara.cZeroSeqResume[0] = *pData++;	//�������Խ�޻ָ�ϵ��
  		pointLimitPara.cZeroSeqResume[1] = *pData++;
  		offset04 += 6;
  
  		//���ڹ��ʳ��������б����
  		pointLimitPara.pSuperiodLimit[0] = *pData++;		//���ع���������
  		pointLimitPara.pSuperiodLimit[1] = *pData++;
  		pointLimitPara.pSuperiodLimit[2] = *pData++;
  		pointLimitPara.pSuperiodTimes = *pData++;				//���ڹ���Խ�����޳���ʱ��
  		pointLimitPara.pSuperiodResume[0] = *pData++;		//���ڹ���Խ�����޻ָ�ϵ��
  		pointLimitPara.pSuperiodResume[1] = *pData++;
  		offset04 += 6;
  
  		//���ڹ��ʳ������б����
  		pointLimitPara.pUpLimit[0] = *pData++;			//���ع�������
  		pointLimitPara.pUpLimit[1] = *pData++;
  		pointLimitPara.pUpLimit[2] = *pData++;
  		pointLimitPara.pUpTimes = *pData++;					//���ڹ���Խ���޳���ʱ��
  		pointLimitPara.pUpResume[0] = *pData++;			//���ڹ���Խ���޻ָ�ϵ��
  		pointLimitPara.pUpResume[1] = *pData++;
  		offset04 += 6;
  
  		//�����ѹ��ƽ�ⳬ���б����
  		pointLimitPara.uPhaseUnbalance[0] = *pData++;			//�����ѹ��ƽ����ֵ
  		pointLimitPara.uPhaseUnbalance[1] = *pData++;
  		pointLimitPara.uPhaseUnTimes = *pData++;					//�����ѹ��ƽ��Խ�޳���ʱ��
  		pointLimitPara.uPhaseUnResume[0] = *pData++;			//�����ѹ��ƽ��Խ�޻ָ�ϵ��
  		pointLimitPara.uPhaseUnResume[1] = *pData++;
  		offset04 += 5;
  
  		//���������ƽ�ⳬ���б����
  		pointLimitPara.cPhaseUnbalance[0] = *pData++;			//���������ƽ����ֵ
  		pointLimitPara.cPhaseUnbalance[1] = *pData++;
  		pointLimitPara.cPhaseUnTimes = *pData++;					//���������ƽ��Խ�޳���ʱ��
  		pointLimitPara.cPhaseUnResume[0] = *pData++;			//���������ƽ��Խ�޻ָ�ϵ��
  		pointLimitPara.cPhaseUnResume[1] = *pData++;
  		offset04 += 5;
  
  		pointLimitPara.uLostTimeLimit = *pData++;
  		offset04++;
  
  		saveViceParameter(0x04, 26, pn, (INT8U *)&pointLimitPara, sizeof(MEASUREPOINT_LIMIT_PARA));    		
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
��������:AFN04027
��������:��Ӧ��վ���ò�������"������ͭ���������(FN27)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04027(INT8U *pData)
{
	COPPER_IRON_LOSS copperIronLoss;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&copperIronLoss, sizeof(COPPER_IRON_LOSS));
			
			//A��
  		copperIronLoss.aResistance[0] = *pData++;		//A�����
  		copperIronLoss.aResistance[1] = *pData++;
  		copperIronLoss.aReactance[0] = *pData++;		//A��翹
  		copperIronLoss.aReactance[1] = *pData++;
  		copperIronLoss.aConductance[0] = *pData++;	//A��絼
  		copperIronLoss.aConductance[1] = *pData++;
  		copperIronLoss.aSusceptance[0] = *pData++;	//A�����
  		copperIronLoss.aSusceptance[1] = *pData++;
  		offset04 += 8;
  
  		//B��
  		copperIronLoss.bResistance[0] = *pData++;		//B�����
  		copperIronLoss.bResistance[1] = *pData++;
  		copperIronLoss.bReactance[0] = *pData++;		//B��翹
  		copperIronLoss.bReactance[1] = *pData++;
  		copperIronLoss.bConductance[0] = *pData++;	//B��絼
  		copperIronLoss.bConductance[1] = *pData++;
  		copperIronLoss.bSusceptance[0] = *pData++;	//B�����
  		copperIronLoss.bSusceptance[1] = *pData++;
  		offset04 += 8;
  
  		//C��
  		copperIronLoss.cResistance[0] = *pData++;		//C�����
  		copperIronLoss.cResistance[1] = *pData++;
  		copperIronLoss.cReactance[0] = *pData++;		//C��翹
 	 		copperIronLoss.cReactance[1] = *pData++;
  		copperIronLoss.cConductance[0] = *pData++;	//C��絼
  		copperIronLoss.cConductance[1] = *pData++;
  		copperIronLoss.cSusceptance[0] = *pData++;	//C�����
  		copperIronLoss.cSusceptance[1] = *pData++;
  		offset04 += 8;
  
  		saveViceParameter(0x04, 27, pn, (INT8U *)&copperIronLoss, sizeof(COPPER_IRON_LOSS));
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
��������:AFN04028
��������:��Ӧ��վ���ò�������"�����㹦�������ֶ���ֵ(FN28)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04028(INT8U *pData)
{
	POWER_SEG_LIMIT powerSegLimit;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&powerSegLimit, sizeof(POWER_SEG_LIMIT));
			
			//���������ֶ���ֵ1
  		powerSegLimit.segLimit1[0] = *pData++;
  		powerSegLimit.segLimit1[1] = *pData++;
  
  		//���������ֶ���ֵ2
  		powerSegLimit.segLimit2[0] = *pData++;
  		powerSegLimit.segLimit2[1] = *pData++;
  
  		offset04 += 4;
  		saveViceParameter(0x04, 28, pn, (INT8U *)&powerSegLimit, sizeof(POWER_SEG_LIMIT));
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
��������:AFN04029
��������:��Ӧ��վ���ò�������"�ն˵��ص��ܱ���ʾ��(FN29)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04029(INT8U *pData)
{
	INT8U teShowNum[12];	
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(teShowNum, 12);
			
			//�ն˵��ص��ܱ���ʾ��
  		for(i=0;i<12;i++)
  		{
  			teShowNum[i] = *pData++;
  		}
  
  		offset04 += 12;
  		saveViceParameter(0x04, 29, pn, (INT8U *)teShowNum, 12);
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
��������:AFN04030
��������:��Ӧ��վ���ò�������"̨�����г���ͣ��/Ͷ������(FN30)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04030(INT8U *pData)
{
	INT8U copyStopAdminSet;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//̨�����г���ͣ��/Ͷ������
  		copyStopAdminSet = *pData++;
  
  		offset04 += 1;
  		saveViceParameter(0x04, 30, pn, (INT8U *)&copyStopAdminSet, 1);
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
��������:AFN04031
��������:��Ӧ��վ���ò�������"�ز��ӽڵ㸽���ڵ��ַ(FN31)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04031(INT8U *pData)
{
	AUXILIARY_ADDR auxiliaryAddr;
	
	INT16U i, pn;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&auxiliaryAddr, sizeof(AUXILIARY_ADDR));
			
			//�ز��ӽڵ㸽���ڵ��ַ����
  		auxiliaryAddr.numOfAuxiliaryNode = *pData++;
  		offset04++;
  		if(auxiliaryAddr.numOfAuxiliaryNode > 20)
  		{
  			pData += auxiliaryAddr.numOfAuxiliaryNode * 6;
  			offset04 += auxiliaryAddr.numOfAuxiliaryNode * 6;
  			errFlg = 1;
  		}
  		else
  		{
  			//�ز��ӽڵ㸽���ڵ��ַ
  			for(i=0;i<auxiliaryAddr.numOfAuxiliaryNode*6;i++)
  			{
  				auxiliaryAddr.auxiliaryNode[i] = *pData++;
  				offset04++;
  			}
  		}
  		
  		if(errFlg == 0)
  		{
  			saveViceParameter(0x04, 31, pn, (INT8U *)&auxiliaryAddr, sizeof(AUXILIARY_ADDR));
  		}
		}
		da1 >>=1;
	}
  
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04033
��������:��Ӧ��վ���ò�������"�ն˳������в�������(FN33)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04033(INT8U *pData)
{
	TE_COPY_RUN_PARA tempTeCopyRunPara;
	
  INT16U i,j;
  
  INT8U num, errFlg = 0;
  INT8U count;
  INT8U dataSet, dataFlg = 0;			//������Ч���ݱ�־

  //���ݵ�Ԫ��ʶ    
  pData += 4;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;

  tempTeCopyRunPara = teCopyRunPara;
   
  //���������
  count = *pData++;
  offset04++;
  if(count > 8)
  {
  	for(i=0;i<count;i++)
  	{
  		num = 0;
  		pData += 13;
  		num += *pData++;
  		pData += num * 4;
  		offset04 = offset04 + 14 + num * 4;
  	}
  	return FALSE;
  }
  
  for(i=0;i<count;i++)
  {
  	num = 0;
   #ifdef SUPPORT_ETH_COPY
  	if((*pData > 0 && *pData <6) || *pData == 31)
   #else	
  	if((*pData > 0 && *pData <5) || *pData == 31)
   #endif
  	{
  		dataFlg = 1;
  		switch(*pData)
  		{
  			case 1:
  				dataSet = 0;
  				break;
  				
  			case 2:
  				dataSet = 1;
  				break;

  			case 3:
  				dataSet = 2;
  				break;

  			case 4:   //2012-3-27,add
  				dataSet = 3;
  				break;
  				
  			case 31:
  				dataSet = 4;
  				break;
  			
  		 #ifdef SUPPORT_ETH_COPY
  			case 5:
  				dataSet = 5;
  				break;
  		 #endif
  		}
  		
  		teCopyRunPara.para[dataSet].commucationPort = *pData++;						//�ն�ͨ�Ŷ˿ں�
	  	teCopyRunPara.para[dataSet].copyRunControl[0] = *pData++;					//̨�����г������п�����
	  	teCopyRunPara.para[dataSet].copyRunControl[1] = *pData++;					
	  	for(j=0;j<4;j++)
	    {
	  		teCopyRunPara.para[dataSet].copyDay[j] = *pData++;							//������-����
	  	}
	  	teCopyRunPara.para[dataSet].copyTime[0] = *pData++;								//������-ʱ��
	  	teCopyRunPara.para[dataSet].copyTime[1] = *pData++;								
	  	teCopyRunPara.para[dataSet].copyInterval = *pData++;							//������ʱ��
	  	for(j=0;j<3;j++)
	  		teCopyRunPara.para[dataSet].broadcastCheckTime[j] = *pData++;		//�㲥Уʱ��ʱʱ��
	  	teCopyRunPara.para[dataSet].hourPeriodNum = *pData++;							//������ʱ����
	  	
	  	offset04 += 14 + teCopyRunPara.para[dataSet].hourPeriodNum * 4;
	  	
	  	if(teCopyRunPara.para[dataSet].hourPeriodNum > 24)
	  	{
	  		errFlg = 1;
	  		pData += teCopyRunPara.para[dataSet].hourPeriodNum * 4;
	  	}
	  	else
	  	{
	  		for(j=0;j<teCopyRunPara.para[dataSet].hourPeriodNum*2;j++)
	  		{
	  			teCopyRunPara.para[dataSet].hourPeriod[j][0] = *pData++;
	  			teCopyRunPara.para[dataSet].hourPeriod[j][1] = *pData++;
	  		}
	  	}
  	}
  	else
  	{
  		pData += 13;
  		num += *pData++;
  		pData += num * 4;
  		offset04 += 14 + num * 4;
  	}
  }
   
  if(errFlg == 1 || dataFlg == 0)
  {
  	bzero(&teCopyRunPara, sizeof(TE_COPY_RUN_PARA));
		teCopyRunPara = tempTeCopyRunPara;
  	return FALSE;
  }
  
	saveParameter(0x04, 33, (INT8U *)&teCopyRunPara, sizeof(TE_COPY_RUN_PARA));
  
  //�������ó���ʱ��
  reSetCopyTime();
  
  return TRUE;
}

/*******************************************************
��������:AFN04034
��������:��Ӧ��վ���ò�������"����������ͨ��ģ��Ĳ�������(F34)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04034(INT8U *pData)
{
	DOWN_RIVER_MODULE_PARA tempDownRiverModulePara;
	
	INT16U i, j, errFlg = 0; 
	
	//���ݵ�Ԫ��ʶ
  pData += 4;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

  //���������
  tempDownRiverModulePara.numOfPara = *pData++;
  offset04++;
  if(tempDownRiverModulePara.numOfPara  < 1 || tempDownRiverModulePara.numOfPara  > 31)
  {
  	offset04 += 6*tempDownRiverModulePara.numOfPara;
  	pData += 6*tempDownRiverModulePara.numOfPara;
  	return FALSE;
  }
  	
  for(i=0;i<tempDownRiverModulePara.numOfPara;i++)
  {
  	tempDownRiverModulePara.para[i].commucationPort = *pData++;	//�ն�ͨ�Ŷ˿ں�
  	
  	if(tempDownRiverModulePara.para[i].commucationPort < 1
  		|| tempDownRiverModulePara.para[i].commucationPort > 31)
  	{
  		errFlg = 1;
  	}
  	
  	tempDownRiverModulePara.para[i].commCtrlWord = *pData++;		//���ն˽ӿڶ˵�ͨ�ſ�����
  	
  	//���ն˽ӿڶ�Ӧ�˵�ͨ������
  	for(j=0;j<4;j++)
  	{
  		tempDownRiverModulePara.para[i].commRate[j] = *pData++;
  	}
  	offset04 += 6;
  }
  
  if(errFlg == 1)
  {
  	return FALSE;
  }
  
  bzero(&downRiverModulePara, sizeof(DOWN_RIVER_MODULE_PARA));
	downRiverModulePara = tempDownRiverModulePara;
  saveParameter(0x04, 34, (INT8U *)&downRiverModulePara, sizeof(DOWN_RIVER_MODULE_PARA));
  return TRUE;
}

/*******************************************************
��������:AFN04035
��������:��Ӧ��վ���ò�������"̨�����г����ص㻧����(F35)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04035(INT8U *pData)
{
	KEY_HOUSEHOLD tempKeyHouseHold;
	
	INT16U i, num, errFlg = 0; 

	//���ݵ�Ԫ��ʶ
  pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  //̨�����г����ص㻧����
  tempKeyHouseHold.numOfHousehold = *pData++;
  offset04++;
  if(tempKeyHouseHold.numOfHousehold > 20)
  {
  	offset04 += tempKeyHouseHold.numOfHousehold * 2;
  	pData += tempKeyHouseHold.numOfHousehold * 2;
  	return FALSE;
  }
  
  //�ص㻧�ĵ��ܱ�/��������װ�����(1~2040)
  for(i=0;i<tempKeyHouseHold.numOfHousehold*2;i++)
  {
  	tempKeyHouseHold.household[i] = *pData++;
  	offset04++;
  	if((i+1)%2==0)
  	{
  		num = tempKeyHouseHold.household[i -1];
  		num |= tempKeyHouseHold.household[i]<<8;
  		if(num > 2040 || num < 1)
  		{
  			errFlg = 1;
  		}
  	}
  }
  
  if(errFlg == 1)
  {
  	return FALSE;
  }
  
  bzero(&keyHouseHold, sizeof(KEY_HOUSEHOLD));
	keyHouseHold = tempKeyHouseHold;
  saveParameter(0x04, 35, (INT8U *)&keyHouseHold, sizeof(KEY_HOUSEHOLD));
  return TRUE;
}

/*******************************************************
��������:AFN04036
��������:��Ӧ��վ���ò�������"�ն�����ͨ��������������(F36)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04036(INT8U *pData)
{
	INT16U i;

	//���ݵ�Ԫ��ʶ
	pData += 4;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

  //��ͨ����������
  for(i=0;i<4;i++)
  {
  	upTranslateLimit[i] = *pData++;
  }
  
  offset04 += 4;
  
  saveParameter(0x04, 36, (INT8U *)upTranslateLimit, offset04);
  return TRUE;
}

/*******************************************************
��������:AFN04037
��������:��Ӧ��վ���ò�������"�ն˼���ͨ�Ų���(F37)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04037(INT8U *pData)
{
	CASCADE_COMM_PARA tempCascadeCommPara;
	
	INT16U i, num, errFlg = 0;

	//���ݵ�Ԫ��ʶ	
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  tempCascadeCommPara.commPort = *pData++;						//�ն˼���ͨ�Ŷ˿ں�
  
  //�����˿�ֻ��485-2(�˿�3)
  if(tempCascadeCommPara.commPort != 3)
  {
  	offset04 += 7 + (*(pData+5)&0xf)*4; 
  	
  	return FALSE;
  }
  	
  tempCascadeCommPara.ctrlWord = *pData++;						//�ն˼���ͨ�ſ�����
  tempCascadeCommPara.receiveMsgTimeout  = *pData++;		//���ձ��ĳ�ʱʱ��
  tempCascadeCommPara.receiveByteTimeout = *pData++;	//���յȴ��ֽڳ�ʱʱ��
  tempCascadeCommPara.cascadeMretryTime  = *pData++;		//������(����վ)����ʧ���ط�����
  if(tempCascadeCommPara.cascadeMretryTime > 3)
  {
  	errFlg = 1;
  }
  	
  tempCascadeCommPara.groundSurveyPeriod = *pData++;	//����Ѳ������
  tempCascadeCommPara.flagAndTeNumber = *pData++;			//������־���ն˸���
  offset04 += 7;
  
  num = tempCascadeCommPara.flagAndTeNumber & 0x0F;
  offset04 += num * 4;
  if((tempCascadeCommPara.flagAndTeNumber >> 7) == 0)
  {
  	if(num <= 3)
  	{
  		for(i=0;i<num*2;i+=2)
  		{
  			tempCascadeCommPara.divisionCode[i] = *pData++;				//�ն�����������
  			tempCascadeCommPara.divisionCode[i + 1] = *pData++;
  			tempCascadeCommPara.cascadeTeAddr[i] = *pData++;			//�����ն˵�ַ
  			tempCascadeCommPara.cascadeTeAddr[i + 1] = *pData++;
  		}
  	}
  	else
  	{
  		pData += num * 4;
  		return FALSE;
  	}
  }
  else
  {
  	if(num == 1)
  	{
  		tempCascadeCommPara.divisionCode[0] = *pData++;				//�ն�����������
  		tempCascadeCommPara.divisionCode[1] = *pData++;
  		tempCascadeCommPara.cascadeTeAddr[0] = *pData++;			//�����ն˵�ַ
  		tempCascadeCommPara.cascadeTeAddr[1] = *pData++;
  	}
  	else
  	{
  		pData += num * 4;
  		return FALSE;
  	}
  }
  
  //������ȷ����������
  bzero(&cascadeCommPara, sizeof(CASCADE_COMM_PARA));
	cascadeCommPara = tempCascadeCommPara;
  saveParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));
  return TRUE;
}

/*******************************************************
��������:AFN04038
��������:��Ӧ��վ���ò�������"1��������������(F38)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04038(INT8U *pData)
{
  TYPE_1_2_DATA_CONFIG typeData;
	INT16U i, j, errFlg = 0;
	INT8U  bType, lType, num;  //��ʱ����š�С��ż���Ϣ������
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  bzero(&typeData, sizeof(TYPE_1_2_DATA_CONFIG));

	//������������Ӧ�Ĵ����
	bType = *pData++;
	
	printf("AFN04-FN38�����%d\n", bType);
	
	offset04++;
	if(bType > 15)
	{
		errFlg = 1;
	}
	
	//�������õ�����(Ҳ���Ǳ�������µ�С������)
	typeData.bigType[bType].groupNum = *pData++;
	offset04++;

	printf("AFN04-FN38����%d\n", typeData.bigType[bType].groupNum);
	
	if(typeData.bigType[bType].groupNum > 16 || typeData.bigType[bType].groupNum < 1)
	{
		for(i=0;i<typeData.bigType[bType].groupNum;i++)
		{
			pData += 2;
			num   = *pData;
			pData += num;
			offset04 += 2 + num;
		}
		
		return FALSE;
	}
	
	for(i=0;i<typeData.bigType[bType].groupNum;i++)
	{
		//�û�С���
		lType = *pData++;
		offset04++;
		
	  printf("AFN04-FN38�û�С���%d\n", lType);		
		
		if(lType > 15)
		{
			errFlg = 1;
		}
		
		//��С�����Ϣ������
		typeData.bigType[bType].littleType[lType].infoGroup = *pData++;
		offset04++;
		
		if(typeData.bigType[bType].littleType[lType].infoGroup > 31)
		{
			errFlg = 1;
			for(j=0;j<typeData.bigType[bType].littleType[lType].infoGroup;j++)
		  {
				pData++;
				offset04++;
			}
		}
		else
		{
			for(j=0;j<typeData.bigType[bType].littleType[lType].infoGroup;j++)
			{
				typeData.bigType[bType].littleType[lType].flag[j] = *pData++;
				offset04++;
			}
		}
	}
	
	if(errFlg == 1)
	{
		return FALSE;
	}
  
  //������ȷ,��������
  typeDataConfig1 = typeData;
  saveParameter(0x04, 38, (INT8U *)&typeDataConfig1, sizeof(TYPE_1_2_DATA_CONFIG));
  
  return TRUE;
}

/*******************************************************
��������:AFN04039
��������:��Ӧ��վ���ò�������"2��������������(F39)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04039(INT8U *pData)
{
  TYPE_1_2_DATA_CONFIG typeData;
	INT16U i, j, errFlg = 0;
	INT8U  bType, lType, num;  //��ʱ����š�С��ż���Ϣ������
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

  bzero(&typeData, sizeof(TYPE_1_2_DATA_CONFIG));

	//������������Ӧ�Ĵ����
	bType = *pData++;
	offset04++;

	printf("AFN04-FN39�����%d\n", bType);
	
	if(bType > 15)
	{
		errFlg = 1;
	}
	
	//�������õ�����(Ҳ���Ǳ�������µ�С������)
	typeData.bigType[bType].groupNum = *pData++;
	offset04++;

	printf("AFN04-FN39����%d\n", typeData.bigType[bType].groupNum);
	
	if(typeData.bigType[bType].groupNum > 16 || typeData.bigType[bType].groupNum < 1)
	{
		for(i=0;i<typeData.bigType[bType].groupNum;i++)
		{
			pData += 2;
			num   = *pData;
			pData += num;
			offset04 += 2 + num;
		}
		
		return FALSE;
	}
	
	for(i=0;i<typeData.bigType[bType].groupNum;i++)
	{
		//�û�С���
		lType = *pData++;
		offset04++;
		
	  printf("AFN04-FN39�û�С���%d\n", lType);		
		
		if(lType > 15)
		{
			errFlg = 1;
		}
		
		//��С�����Ϣ������
		typeData.bigType[bType].littleType[lType].infoGroup = *pData++;
		offset04++;
		
		if(typeData.bigType[bType].littleType[lType].infoGroup > 31)
		{
			errFlg = 1;
			for(j=0;j<typeData.bigType[bType].littleType[lType].infoGroup;j++)
		  {
				pData++;
				offset04++;
			}
		}
		else
		{
			for(j=0;j<typeData.bigType[bType].littleType[lType].infoGroup;j++)
			{
				typeData.bigType[bType].littleType[lType].flag[j] = *pData++;
				offset04++;
			}
		}
	}
	
	if(errFlg == 1)
	{
		return FALSE;
	}
  
  //������ȷ,��������
  typeDataConfig2 = typeData;
  saveParameter(0x04, 39, (INT8U *)&typeDataConfig2, sizeof(TYPE_1_2_DATA_CONFIG));
  
  return TRUE;
}

#ifndef LIGHTING

/*******************************************************
��������:AFN04041
��������:��Ӧ��վ���ò�������"ʱ�ι��ض�ֵ(F41)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04041(INT8U *pData)
{	
	PERIOD_CTRL_CONFIG tempPeriodCtrlConfig;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	INT16U i, j;

	INT8U da1, da2;			//�ܼ����
	INT8U temp1, temp2;
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&tempPeriodCtrlConfig, sizeof(PERIOD_CTRL_CONFIG));
			
			//������־
			tempPeriodCtrlConfig.periodNum = *pData++;
			offset04++;
			temp1 = tempPeriodCtrlConfig.periodNum & 0x07;
	
			//��������
			for(i=0;i<3;i++)
			{
				if((temp1 & 0x01) == 0x01)
				{
					tempPeriodCtrlConfig.period[i].timeCode = *pData++;
					offset04++;
					temp2 = tempPeriodCtrlConfig.period[i].timeCode;
					for(j=0;j<8;j++)
					{
						if((temp2 & 0x01) == 0x01)
						{
							tempPeriodCtrlConfig.period[i].limitTime[j][0] = *pData++;
							tempPeriodCtrlConfig.period[i].limitTime[j][1] = *pData++;
							offset04 += 2;
						}
						temp2 >>= 1;
					}
				}
				temp1 >>= 1;
			}
	
			//�ж�pn��ֵ�Ƿ�Ϸ�, da1�Ƿ�Ϸ�
  		if(pn > 8 || errFlg == 1)
  		{
  			errFlg = 1;
  		}
			else
			{
				bzero(&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
				periodCtrlConfig[pn -1] = tempPeriodCtrlConfig;
  			saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04042
��������:��Ӧ��վ���ò�������"���ݹ��ز���(F42)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04042(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�ж�pn��ֵ�Ƿ�Ϸ�
  		if(pn > 8 || errFlg == 1)
  		{
  			pData += 6;
  			offset04 += 6;
  			errFlg = 1;
  		}
			else
			{
				//���ݿض�ֵ
				wkdCtrlConfig[pn - 1].wkdLimit = *pData | (*(pData + 1)<<8);
				pData += 2;
		
				//�޵���ʼʱ��
				wkdCtrlConfig[pn - 1].wkdStartMin = *pData++;
				wkdCtrlConfig[pn - 1].wkdStartHour = *pData++;
		
				wkdCtrlConfig[pn - 1].wkdTime = *pData++;			//�޵�ʱ��
				wkdCtrlConfig[pn - 1].wkdDate = *pData++;			//�޵��ܴ�
				offset04 += 6;
		
	  		saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04043
��������:��Ӧ��վ���ò�������"���ʿ��ƵĹ��ʼ��㻬��ʱ��(F43)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04043(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U temp, errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//���ݿض�ֵ
			temp = *pData++;
			offset04 += 1;
			
			if(temp > 60 || temp < 1 || pn > 8 || errFlg == 1)
			{
				errFlg = 1;
			}
			else
			{
				powerCtrlCountTime[pn - 1].countTime = temp;
  			saveViceParameter(0x04, 43, pn, (INT8U *)&powerCtrlCountTime[pn -1], sizeof(POWERCTRL_COUNT_TIME));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04044
��������:��Ӧ��վ���ò�������"Ӫҵ��ͣ�ز���(F44)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04044(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 8 || errFlg == 1)
			{
				pData += 8;
				offset04 += 8;
				errFlg = 1;
			}
			else
			{
				//��ͣ��ʼʱ�䣨�գ��£��꣩
   	    obsCtrlConfig[pn-1].obsStartDay = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsStartMonth = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsStartYear = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;

				//��ͣ����ʱ�䣨�գ��£��꣩
   	    obsCtrlConfig[pn-1].obsEndDay = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsEndMonth = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsEndYear = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;

				//��ͣ�ع�����ֵ
   	    obsCtrlConfig[pn-1].obsLimit = *pData | *(pData+1)<<8;
   	    pData+=2;
				offset04 += 8;

	  		saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
	  	}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04045
��������:��Ӧ��վ���ò�������"�����ִ��趨(F45)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04045(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 8 || errFlg == 1)
			{
				pData++;
				offset04++;
				errFlg = 1;
			}
			else
			{
				//�����ִ�
				powerCtrlRoundFlag[pn - 1].flag = *pData++;
				offset04++;
				
			  saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04046
��������:��Ӧ��վ���ò�������"�µ����ض�ֵ(F46)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04046(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 8 || errFlg == 1)
			{
				pData += 4;
				offset04 += 4;
				errFlg = 1;
			}
			else
			{
				//�µ����ض�ֵ
				monthCtrlConfig[pn - 1].monthCtrl = *pData | (*(pData + 1)<<8)
														| (*(pData + 2)<<16) | (*(pData + 3)<<24);
				pData += 4;
				offset04 += 4;
				
			  saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04047
��������:��Ӧ��վ���ò�������"������(��)�ز���(F47)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04047(INT8U *pData)
{	
	INT16U    pn = 0;
	INT16U    tempPn = 0;
  
	INT8U     da1, da2;			//�ܼ����
	INT8U     errFlg = 0;
	INT8U     eventData[34];
	INT8U     leftPower[12];
  INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
  DATE_TIME readTime;
	INT32U    tmpData1, tmpData2;
	INT32U    tmpDatax,tmpByte;
	INT8U     quantity;
  INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-25,add
  BOOL      bufHasData;
  INT8U     negLeft, negAppend;

	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�ж�pn��ֵ�Ƿ�Ϸ�
		  if(pn > 8 || errFlg == 1)
		  {
		  	pData += 17;
		  	offset04 += 17;
		  	errFlg = 1;
		  }
		  else
		  {
     	  //�¼�����-�ܼ����
     	  eventData[8] = pn;
     	  
     	  //���絥��
     	  chargeCtrlConfig[pn-1].numOfBill = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[9]  = *pData++;
     	  eventData[10] = *pData++;
     	  eventData[11] = *pData++;
     	  eventData[12] = *pData++;
     	  
     	  //׷��/ˢ�±�־
     	  chargeCtrlConfig[pn-1].flag = *pData;
     	  eventData[13] = *pData++;
     	  
     	  //������ֵ
     	  chargeCtrlConfig[pn-1].chargeCtrl = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[14] = *pData++;
     	  eventData[15] = *pData++;
     	  eventData[16] = *pData++;
     	  eventData[17] = *pData++;
     	  
     	  //�澯����
     	  chargeCtrlConfig[pn-1].alarmLimit = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[18] = *pData++;
     	  eventData[19] = *pData++;
     	  eventData[20] = *pData++;
     	  eventData[21] = *pData++;
     	  
     	  //��բ����
     	  chargeCtrlConfig[pn-1].cutDownLimit = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[22] = *pData++;
     	  eventData[23] = *pData++;
     	  eventData[24] = *pData++;
     	  eventData[25] = *pData++;
     	  
     	  chargeCtrlConfig[pn-1].alarmTimeOut.year = 0xFF;
     	   
     	  //�¼�����-��ǰʣ�������
     	  readTime = sysTime;
        if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
     	  {
     	  	 eventData[26] = leftPower[0];
     	  	 eventData[27] = leftPower[1];
     	  	 eventData[28] = leftPower[2];
     	  	 eventData[29] = leftPower[3];
     	  }
     	  else
     	  {
     	  	 //ԭ����ʣ�����,�ù�ǰ������Ϊ0
     	  	 eventData[26] = 0;
     	  	 eventData[27] = 0;
     	  	 eventData[28] = 0;
     	  	 eventData[29] = 0;
     	  	 
     	  	 leftPower[0] = 0x0;
     	  	 leftPower[1] = 0x0;
     	  	 leftPower[2] = 0x0;
     	  	 leftPower[3] = 0x0;
     	  }
     
     	  //ˢ��ʣ�����
     	  if (chargeCtrlConfig[pn-1].flag == 0xAA) //ˢ��ʣ�����(��ǰʣ�����=�·�������)
     	  {
     	     leftPower[0] = chargeCtrlConfig[pn-1].chargeCtrl&0xFF;
     	     leftPower[1] = chargeCtrlConfig[pn-1].chargeCtrl>>8&0xFF;
     	     leftPower[3] = chargeCtrlConfig[pn-1].chargeCtrl>>24&0xFF;
     	     leftPower[2] = chargeCtrlConfig[pn-1].chargeCtrl>>16&0xFF;
     	     
     	     printf("������(��)�ز���(F47)�ܼ���%d:ˢ��ʣ�����\n", pn);
        }
        else      //׷��ʣ�����(��ǰʣ�����+=�·�������)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
    	      printf("������(��)�ز���(F47)�ܼ���%d:׷��ʣ�����\n", pn);
    	    }
     	    
     	    negLeft   = 0;
     	    negAppend = 0;
     	    
     	    //��ǰʣ�����
     	    tmpData1 = (leftPower[0]&0xF)       + ((leftPower[0]>>4)&0xF)*10
     	             + (leftPower[1]&0xF)*100   + ((leftPower[1]>>4)&0xF)*1000
     	             + (leftPower[2]&0xF)*10000 + ((leftPower[2]>>4)&0xF)*100000
     	             + (leftPower[3]&0xF)*1000000;
     	    if ((leftPower[3]>>6&0x01) == 0x01)
     	    {
     	       tmpData1 *= 1000;
     	    }
     	    
     	    if ((leftPower[3]>>4&0x01) == 0x01)
     	    {
     	    	 negLeft = 1;
     	    }
    	    
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
       	    if (negLeft==1)
       	    {
       	      printf("������(��)�ز���(F47):��ǰʣ�����=-%d\n", tmpData1);
       	    }
       	    else
       	    {
       	      printf("������(��)�ز���(F47):��ǰʣ�����=%d\n", tmpData1);
       	    }
       	  }
     	    
     	    //�·�������
     	    tmpData2 = (chargeCtrlConfig[pn-1].chargeCtrl&0xF)
     	             + ((chargeCtrlConfig[pn-1].chargeCtrl>>4)&0xF)*10
     	             + ((chargeCtrlConfig[pn-1].chargeCtrl>>8)&0xF)*100
     	             + ((chargeCtrlConfig[pn-1].chargeCtrl>>12)&0xF)*1000
     	             + ((chargeCtrlConfig[pn-1].chargeCtrl>>16)&0xF)*10000
     	             + ((chargeCtrlConfig[pn-1].chargeCtrl>>20)&0xF)*100000
     	             + ((chargeCtrlConfig[pn-1].chargeCtrl>>24)&0xF)*1000000;
     	    if ((chargeCtrlConfig[pn-1].chargeCtrl>>30)&0x01 == 0x01)
     	    {
     	      tmpData2 *= 1000;
     	    }
     	    
     	    if ((chargeCtrlConfig[pn-1].chargeCtrl>>28)&0x01 == 0x01)
     	    {
     	    	 negAppend = 1;
     	    }

    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
     	      if (negAppend==1)
     	      {
     	        printf("������(��)�ز���(F47):�����·�������=-%d\n", tmpData2);
     	      }
     	      else
     	      {
     	        printf("������(��)�ز���(F47):�����·�������=%d\n", tmpData2);
     	      }
     	    }

     	    //�µ�ʣ�����
     	    if ((negLeft==0 && negAppend==0)       //��Ϊ��
     	        || (negLeft==1 && negAppend==1)     //��Ϊ��
     	       )
     	    {
     	      tmpData1 += tmpData2;
     	      
     	      if (negLeft==1)
     	      {
     	        negAppend = 0xbb;   //���Ϊ��
     	      }
     	      else
     	      {
     	        negAppend = 0xaa;   //���Ϊ��
     	      }
     	    }
     	    else
     	    {
     	    	if (negLeft==0 && negAppend==1)     //ʣ��Ϊ��,׷�Ӷ�Ϊ��
     	    	{
     	        if (tmpData1>=tmpData2)
     	        {
     	          tmpData1 -= tmpData2;
     	          negAppend = 0xaa;     //���Ϊ��
     	        }
     	        else
     	        {
     	          tmpData1 = tmpData2-tmpData1;
     	          negAppend = 0xbb;     //���Ϊ��
     	        }
     	      }
     	      else     //ʣ��Ϊ��,׷��Ϊ��
     	      {
     	        if (tmpData1>=tmpData2)
     	        {
     	          tmpData1 -= tmpData2;
     	          negAppend = 0xbb;     //���Ϊ��
     	        }
     	        else
     	        {
     	          tmpData1 = tmpData2-tmpData1;
     	          negAppend = 0xaa;     //���Ϊ��
     	        }
     	      }
     	    }

    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
     	      if (negAppend==0xbb)
     	      {
     	        printf("������(��)�ز���(F47):׷�Ӻ��µĹ�����=-%d\n", tmpData1);
     	      }
     	      else
     	      {
     	        printf("������(��)�ز���(F47):׷�Ӻ��µĹ�����=%d\n", tmpData1);
     	      }
     	    }
     			
     			//�������ݸ�ʽ
     			tmpData2 = 0x00000000;
     	    quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
     	    
     	    tmpData1 = hexToBcd(tmpData1);
     	    
     	    leftPower[0] = tmpData1&0xFF;
     	    leftPower[1] = (tmpData1>>8)&0xFF;
     	    leftPower[2] = (tmpData1>>16)&0xFF;
     	    leftPower[3] = (tmpData1>>24)&0xFF;
     	    
     	    if (negAppend==0xbb)
     	    {
     	      leftPower[3] |= (1 <<4);
     	    }
     	    else
     	    {
     	      leftPower[3] |= (0 <<4);
     	    }
        }
     	  
     	  //�¼�����-�����ʣ�������
     	  eventData[30] = leftPower[0];
     	  eventData[31] = leftPower[1];
     	  eventData[32] = leftPower[2];
     	  eventData[33] = leftPower[3];
     	  
        //�����òο�ʣ�����=��ǰʣ�����
        leftPower[4] = leftPower[0];
        leftPower[5] = leftPower[1];
        leftPower[6] = leftPower[2];
        leftPower[7] = leftPower[3];
     
        //�����òο��ܼ��й�������=��ǰ�ܼ��й�����
        readTime = timeHexToBcd(sysTime);

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
        
        readTime = timeHexToBcd(sysTime);
        if (onlyHasPulsePn==0xaa)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("������(��)�ز���(F47):�ܼ���%dֻ�����������\n", pn);
        	}
        	
        	bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime);
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("������(��)�ز���(F47):�ܼ���%d������������\n", pn);
        	}
        	bufHasData = readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0);
        }
        
    	  if (bufHasData == TRUE)
        {
      	  if (dataBuff[GP_MONTH_WORK+3]!= 0xFF && dataBuff[GP_MONTH_WORK+3] != 0xEE)
      	  {
         	  leftPower[8]  = dataBuff[GP_MONTH_WORK+3];
            leftPower[9]  = dataBuff[GP_MONTH_WORK+4];
            leftPower[10] = dataBuff[GP_MONTH_WORK+5];
            leftPower[11] = ((dataBuff[GP_MONTH_WORK]&0x01)<<6)
                          | (dataBuff[GP_MONTH_WORK]&0x10)
                          | (dataBuff[GP_MONTH_WORK+6]&0x0f);
          }
          else
          {
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
              printf("������(��)�ز���(F47):�ο��ܼ��й�����������TRUE���ܼ��µ�����Ϊ0xee,�ο���0\n");
            }

         	  leftPower[8]  = 0x0;
            leftPower[9]  = 0x0;
            leftPower[10] = 0x0;
            leftPower[11] = 0x0;
          }
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
            printf("������(��)�ز���(F47):�ο��ܼ��й�����������FALSE,�ο���0\n");
          }
          
          //ly,2011-04-16,�ĳ����������
         	leftPower[8]  = 0x0;
          leftPower[9]  = 0x0;
          leftPower[10] = 0x0;
          leftPower[11] = 0x0;
        }
     	  
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
     	    printf("������(��)�ز���(F47):�ο��ܼ��й�������=%02x%02x%02x%02x\n", leftPower[11], leftPower[10], leftPower[9], leftPower[8]);
     	  }

        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);

        //ERC19.����������ü�¼(��¼�¼������)
        if ((eventRecordConfig.iEvent[2] & 0x04) || (eventRecordConfig.nEvent[2] & 0x04))
        {
         	 eventData[0] = 19;  //ERC
         	 eventData[1] = 34;  //��¼����
         	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         	 eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
         	 eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
         	 eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
         	 eventData[7] = sysTime.year/10<<4   | sysTime.year%10;
         	  
           if (eventRecordConfig.iEvent[2] & 0x04)
           {
         	    writeEvent(eventData, 34, 1, DATA_FROM_GPRS);   //������Ҫ�¼�����
         	 }
           if (eventRecordConfig.nEvent[2] & 0x04)
           {
         	    writeEvent(eventData, 34, 2, DATA_FROM_LOCAL);  //����һ���¼�����
         	 }
         	  
         	 eventStatus[2] |= 0x04;
        }
        
				offset04 += 17;
				
			  saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
		  }
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04048
��������:��Ӧ��վ���ò�������"����ִ��趨(F48)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04048(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	if (da1==0 && da2==0)
	{
		 pData += 1;
		 offset04++;
		 
		 return FALSE;
	}
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�ж�pn��ֵ�Ƿ�Ϸ�
		  if(pn > 8 || errFlg == 1)
		  {
		  	pData += 1;
		  	offset04++;
		  	errFlg = 1;
		  }
		  else
		  {
		  	//���絥��
				electCtrlRoundFlag[pn - 1].flag = *pData++;
				offset04++;

			  saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
		  }
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04049
��������:��Ӧ��վ���ò�������"���ظ澯ʱ��(F49)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04049(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�ܼ����
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�ж�pn��ֵ�Ƿ�Ϸ�
		  if(pn > 8 || errFlg == 1)
		  {
		  	errFlg = 1;
		  }
			else
			{
				//���ظ澯ʱ��
        powerCtrlAlarmTime[pn-1].def = 0xAA;
				powerCtrlAlarmTime[pn-1].alarmTime = *pData;
				
			  saveViceParameter(0x04, 49, pn, (INT8U *)&powerCtrlAlarmTime[pn -1], sizeof(POWERCTRL_ALARM_TIME));
			}
			offset04++;
			pData++;
		}
		da1 >>=1;
	}
	
  return TRUE;
}

#else

/*******************************************************
��������:AFN04050
��������:��Ӧ��վ���ò�������"����ʱ��(F50)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04050(INT8U *pData)
{
  struct ctrlTimes           cTimes;
  BOOL                       ifCtrlPeriod=0;
  INT8U                      i, j;
  INT8U                      count;
  struct cpAddrLink          *tmpCpLink;
	DATE_TIME                  tmpTime;
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //һ��������ͳ���¼�����(��ʱ���޹���)

  //���ݵ�Ԫ��ʶ    
  pData += 4;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
   
  //���������
  count = *pData++;
  
  //2016-12-15,��ʱ�β����ĳ�С������,��һ������Ҫ���ԭʱ��
	if (count>0 && ((count&0x80)==0x80))
  {
  	if (debugInfo&WIRELESS_DEBUG)
    {
      printf("AFN04050:���ԭʱ�β���\n");
    }
		deleteCtimes();
  }
	count &=0x7f;
  
  //offset04 += 1+count*24+1+4;
  offset04 += 1+count*25+1+4;    //2016-08-17,�޸�Ϊһ��ʱ�β���25�ֽ�

  for(i=0; i<count; i++)
  {
  	cTimes.startMonth = *pData++;    //��ʼ��
  	cTimes.startDay   = *pData++;    //��ʼ��

  	cTimes.endMonth   = *pData++;    //������
  	cTimes.endDay     = *pData++;    //������
  	
  	cTimes.deviceType = *pData++;    //�豸����
  	cTimes.noOfTime   = *pData++;    //ʱ�κ�
  	cTimes.workDay    = *pData++;    //������,2016-08-17,Add
  	
  	for(j=0; j<6; j++)
    {
  	  cTimes.hour[j]  = *pData++;    //ʱ��1-6ʱ
  	  cTimes.min[j]   = *pData++;    //ʱ��1-6��
  	  cTimes.bright[j]= *pData++;    //ʱ��1-6����
  	}
  	
  	cTimes.next       = NULL;
  	
  	saveViceParameter(cTimes.deviceType, cTimes.startMonth, cTimes.startDay | cTimes.noOfTime<<8, (INT8U *)&cTimes, sizeof(struct ctrlTimes));
  }

  //2015-06-09,Add����ģʽ
  ctrlMode = *pData++;
  saveParameter(0x04, 52, &ctrlMode, 1);
  
  //2015-06-25,�����ǰ-�ӳٷ�����Ч
  memcpy(beforeOnOff, pData, 4);
  saveParameter(0x04, 53, beforeOnOff, 4);

  
  initCtrlTimesLink();
	
	tmpCpLink = initPortMeterLink(0xff);
	
	while(tmpCpLink!=NULL)
	{
    //���Ƶ���ʱ���޹���
		tmpTime = timeHexToBcd(sysTime);
		if (readMeterData((INT8U *)&meterStatisExtranTimeS, tmpCpLink->mp, STATIS_DATA, 99, &tmpTime, 0)==TRUE)
		{
			if (meterStatisExtranTimeS.mixed & 0x20)
			{
				//���ʱ����ͬ����־
				meterStatisExtranTimeS.mixed &= 0xdf;
        
				//�洢���Ƶ�ͳ������(��ʱ���޹���)
        saveMeterData(tmpCpLink->mp, tmpCpLink->port, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
				
				if (debugInfo&WIRELESS_DEBUG)
				{
					printf("������Ƶ�%dʱ��ͬ����־\n", tmpCpLink->mp);
				}
			}
		}
		
		tmpCpLink = tmpCpLink->next;
	}

  if (debugInfo&WIRELESS_DEBUG)
  {
    printf("AFN04050:��%d������\n", count);
  }

  return TRUE;   
}

/*******************************************************
��������:AFN04051
��������:��Ӧ��վ���ò�������"�쳣�ж���ֵ(F51)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04051(INT8U *pData)
{	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	pnGate.failureRetry = *pData++;			//���ֵƾ߹������Դ���
	pnGate.boardcastWaitGate = *pData++;//�㲥����ȴ�ʱ��
	pnGate.checkTimeGate = *pData++;    //Уʱ��ֵ
	pnGate.lddOffGate = *pData++;       //���ƿ��Ƶ�������ֵ
	pnGate.lddtRetry = *pData++;        //����ĩ�����Դ���
	pnGate.offLineRetry = *pData++;     //�������Դ���
	pnGate.lcWave = *pData++;           //����ն���ֵ
	pnGate.leakCurrent = *pData++;      //©������ֵ
	offset04 += 8;
	
  saveParameter(0x04, 51, (INT8U *)&pnGate, sizeof(PN_GATE));

  return TRUE;
}

/*******************************************************
��������:AFN04052
��������:��Ӧ��վ���ò�������"���Ƶ���ֵ����(FN52)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04052(INT8U *pData)
{
	PN_LIMIT_PARA pointLimitPara;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//�������
			bzero(&pointLimitPara, sizeof(PN_LIMIT_PARA));
		 
  		pointLimitPara.vSuperiodLimit = *pData | *(pData+1)<<8;;	//��ѹ��ѹ����
  		pData += 2;

  		pointLimitPara.vDownDownLimit = *pData | *(pData+1)<<8;;	//��ѹǷѹ����
  		pData += 2;

  		pointLimitPara.cSuperiodLimit[0] = *pData++;	            //������������
  		pointLimitPara.cSuperiodLimit[1] = *pData++;
  		pointLimitPara.cSuperiodLimit[2] = *pData++;

  		pointLimitPara.cDownDownLimit[0] = *pData++;	            //����Ƿ������
  		pointLimitPara.cDownDownLimit[1] = *pData++;
  		pointLimitPara.cDownDownLimit[2] = *pData++;
  		
  		pointLimitPara.pUpLimit[0] = *pData++;			              //��������
  		pointLimitPara.pUpLimit[1] = *pData++;
  		pointLimitPara.pUpLimit[2] = *pData++;

  		pointLimitPara.pDownLimit[0] = *pData++;			            //��������
  		pointLimitPara.pDownLimit[1] = *pData++;
  		pointLimitPara.pDownLimit[2] = *pData++;
			
  		pointLimitPara.factorDownLimit[0] = *pData++;             //������������
  		pointLimitPara.factorDownLimit[1] = *pData++;

  		pointLimitPara.overContinued = *pData++;                  //Խ�޳���ʱ��
  		
  		offset04 += 23;
  
  		saveViceParameter(0x04, 52, pn, (INT8U *)&pointLimitPara, sizeof(PN_LIMIT_PARA));    		
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
��������:AFN04053
��������:��Ӧ��վ���ò�������"ң������<��չ>(F053)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04053(INT8U *pData)
{
	INT16U i;
	INT16U tmpPn;
	INT8U  tmpBuf[5];
	
	//���ݵ�Ԫ��ʶ
	tmpPn = findFnPn(*pData, *(pData+1), FIND_PN);
	//pData+=4;
	 
	//ָ�����ݵ�Ԫ
  //pData = pDataUnit04;
  
	offset04 += 0;

	if (tmpPn<=4 && tmpPn>0 && copyCtrl[1].pForwardData==NULL)
	{
		if (selectParameter(5, 160+tmpPn-1, tmpBuf, 2)==FALSE)
		{
			printf("%s==>%d,�޺���ѧϰ����\n", __func__, tmpPn);
			return FALSE;
		}
	
		copyCtrl[1].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
		copyCtrl[1].pForwardData->fn						= 1;
		copyCtrl[1].pForwardData->dataFrom			= DOT_CP_IR;
		copyCtrl[1].pForwardData->ifSend				= FALSE;
		copyCtrl[1].pForwardData->receivedBytes = 0;
		copyCtrl[1].pForwardData->forwardResult = RESULT_NONE;
	
		//͸��ת��ͨ�ſ�����,2400-8-n-1
		copyCtrl[1].pForwardData->ctrlWord = 0x63;
	 
		//͸��ת�����յȴ��ֽڳ�ʱʱ��
		copyCtrl[1].pForwardData->byteTimeOut = 1;
	 
		copyCtrl[1].pForwardData->data[768] = tmpPn-1; 	 //���ֽڱ�����������
	
		//͸��ת�����յȴ����ĳ�ʱʱ��
		//��λΪs
		copyCtrl[1].pForwardData->frameTimeOut = 1;
		
		//*pDataΪ�������ͣ�1-���俪,2-���ȿ�,3-��ʪ��,4-��
		selectParameter(5, 160+tmpPn-1, copyCtrl[1].pForwardData->data, 2);
		copyCtrl[1].pForwardData->length = copyCtrl[1].pForwardData->data[0] | copyCtrl[1].pForwardData->data[1]<<8;
		printf("��������=%d\n", copyCtrl[1].pForwardData->length);
		selectParameter(5, 160+tmpPn-1, copyCtrl[1].pForwardData->data, copyCtrl[1].pForwardData->length+2);
		for(i=0; i<copyCtrl[1].pForwardData->length; i++)
		{
			copyCtrl[1].pForwardData->data[i] = copyCtrl[1].pForwardData->data[i+2];
		}
		
		return TRUE;
	}
	 

	return FALSE;
}

/*******************************************************
��������:AFN04054
��������:��Ӧ��վ���ò�������"�������Ƶ�<��չ>(F054)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04054(INT8U *pData)
{
	INT16U tmpPn;
	struct cpAddrLink *tmpQueryMpLink;
	
	//���ݵ�Ԫ��ʶ
	tmpPn = findFnPn(*pData, *(pData+1), FIND_PN);
	//pData+=4;
	 
	//ָ�����ݵ�Ԫ
  //pData = pDataUnit04;

  
	offset04 += 0;

	if (tmpPn>0 && tmpPn<2040)
	{
		saveParameter(0x04, 54, (INT8U *)&tmpPn, 2);
		
		printf("��������%d\n", tmpPn);

		tmpQueryMpLink = xlcLink;
		while (tmpQueryMpLink!=NULL)
		{
			if (tmpQueryMpLink->mp==tmpPn)
			{
				break;
			}
		
			tmpQueryMpLink = tmpQueryMpLink->next;
		}
		
		if (tmpQueryMpLink!=NULL)
		{
			xlcForwardFrame(2, tmpQueryMpLink->addr, 0);
		}
		
		return TRUE;
	}

	return FALSE;
}

/*******************************************************
��������:AFN04055
��������:��Ӧ��վ���ò�������"�������Ƶ�<��չ>(F055)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04055(INT8U *pData)
{
	INT16U tmpPn;
	
	//���ݵ�Ԫ��ʶ
	tmpPn = findFnPn(*pData, *(pData+1), FIND_PN);
	//pData+=4;
	 
	//ָ�����ݵ�Ԫ
  //pData = pDataUnit04;
  
	offset04 += 0;

	if (tmpPn>0 && tmpPn<2040)
	{
		deleteParameter(0x04, 54);
		saveParameter(0x04, 55, (INT8U *)&tmpPn, 2);
		
		printf("��������%d\n", tmpPn);

		return TRUE;
	}

	return FALSE;

}


#endif    //LIGHTING,about line 2564

/*******************************************************
��������:AFN04057
��������:��Ӧ��վ���ò�������"�ն������澯�����ֹ��־(F57)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04057(INT8U *pData)
{	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;

	//�����澯����/��ֹ��־λ
	voiceAlarm[0] = *pData++;
	voiceAlarm[1] = *pData++;
	voiceAlarm[2] = *pData++;
	offset04 += 3;
	
  saveParameter(0x04, 57, (INT8U *)voiceAlarm, 3);
  
  return TRUE;
}

/*******************************************************
��������:AFN04058
��������:��Ӧ��վ���ò�������"�ն��Զ��������(F58)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04058(INT8U *pData)
{	
	//���ݵ�Ԫ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	//��������վ������ͨ��ʱ��
	noCommunicationTime = *pData++;
	offset04 += 1;
	
  saveParameter(0x04, 58, (INT8U *)&noCommunicationTime, 1);
  
  return TRUE;
}

/*******************************************************
��������:AFN04059
��������:��Ӧ��վ���ò�������"���ܱ��쳣�б���ֵ(F59)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04059(INT8U *pData)
{	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	meterGate.powerOverGate = *pData++;				//������������ֵ
	meterGate.meterFlyGate = *pData++;				//���ܱ������ֵ
	meterGate.meterStopGate = *pData++;				//���ܱ�ͣ����ֵ
	meterGate.meterCheckTimeGate = *pData++;	//���ܱ�Уʱ��ֵ
	offset04 += 4;
	
  saveParameter(0x04, 59, (INT8U *)&meterGate, 4);

  return TRUE;
}

/*******************************************************
��������:AFN04060
��������:��Ӧ��վ���ò�������"г����ֵ(F60)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04060(INT8U *pData)
{	
	INT8U i;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	//�ܻ����ѹ����������
	waveLimit.totalUPercentUpLimit[0] = *pData++;
	waveLimit.totalUPercentUpLimit[1] = *pData++;
	
	//���г����ѹ����������
	waveLimit.oddUPercentUpLimit[0] = *pData++;
	waveLimit.oddUPercentUpLimit[1] = *pData++;
	
	//ż��г����ѹ����������
	waveLimit.evenUPercentUpLimit[0] = *pData++;
	waveLimit.evenUPercentUpLimit[1] = *pData++;
	offset04 += 6;
	
	//г����ѹ����������
	for(i=0;i<18;i++)
	{
		waveLimit.UPercentUpLimit[i][0] = *pData++;
		waveLimit.UPercentUpLimit[i][1] = *pData++;
		offset04 += 2;
	}
	
	//�ܻ��������Чֵ����
	waveLimit.totalIPercentUpLimit[0] = *pData++;
	waveLimit.totalIPercentUpLimit[1] = *pData++;
	offset04 += 2;
	
	//г��������Чֵ����
	for(i=0;i<18;i++)
	{
		waveLimit.IPercentUpLimit[i][0] = *pData++;
		waveLimit.IPercentUpLimit[i][1] = *pData++;
		offset04 += 2;
	}
	
  saveParameter(0x04, 60, (INT8U *)&waveLimit, sizeof(WAVE_LIMIT));

  return TRUE;
}

/*******************************************************
��������:AFN04061
��������:��Ӧ��վ���ò�������"ֱ��ģ�����������(F61)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04061(INT8U *pData)
{
	//���ݵ�Ԫ��ʶ
	pData+=4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	adcInFlag = *pData++;
	
	offset04 += 1;
	
	saveParameter(0x04, 61, (INT8U *)&adcInFlag, 1);
	
	return TRUE;
}

/*******************************************************
��������:AFN04065
��������:��Ӧ��վ���ò�������"��ʱ�ϱ�1��������������(F65)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04065(INT8U *pData)
{	
	REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U  da1, da2;			//�����
	INT8U  errFlg = 0;
	INT16U numOfItem = 0;    //2014-10-16,add
  INT8U  iOfPn,iOfFn;

	tmpreportTask = reportTask1;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 64)
			{
				return FALSE;
			}
			
			if(reportTask1.task[pn - 1].taskNum == 0)
			{
				reportTask1.numOfTask++;
				reportTask1.task[pn - 1].taskNum = pn;
			}
			
			//�ϱ����ڼ��������ڵ�λ
			reportTask1.task[pn - 1].sendPeriod = *pData++;
			offset04++;
			
			//�ϱ���׼ʱ��
			reportTask1.task[pn - 1].sendStart.second = (*pData >> 4) * 10 + (*pData & 0x0F);
			reportTask1.task[pn - 1].sendStart.minute = (*(pData + 1) >> 4) * 10 + (*(pData + 1) & 0x0F);
			reportTask1.task[pn - 1].sendStart.hour = (*(pData + 2) >> 4) * 10 + (*(pData + 2) & 0x0F);
			reportTask1.task[pn - 1].sendStart.day = (*(pData + 3) >> 4) * 10 + (*(pData + 3) & 0x0F);
			reportTask1.task[pn - 1].sendStart.month = ((*(pData + 4) & 0x10) >> 4) * 10 + (*(pData + 4) & 0x0F);
			reportTask1.task[pn - 1].week = *(pData + 4) & 0xE0;
			reportTask1.task[pn - 1].sendStart.year = (*(pData + 5) >> 4) * 10 + (*(pData + 5) & 0x0F);
			pData += 6;
			offset04 += 6;
			
			//�������ݳ�ȡ����
			reportTask1.task[pn - 1].sampleTimes = *pData++;
			offset04++;
			if(reportTask1.task[pn - 1].sampleTimes > 96 || reportTask1.task[pn - 1].sampleTimes < 1)
			{
				errFlg = 1;
			}
			
			//���ݵ�Ԫ��ʾ����
			reportTask1.task[pn-1].numOfDataId = *pData++;
			offset04++;
			
			//���ݵ�Ԫ��ʶ
			numOfItem = 0;
			for(i=0; i<reportTask1.task[pn-1].numOfDataId; i++)
			{
				if (0x00==*pData && 0x00==*(pData+1))    //PN=0
				{
  				for(iOfFn=1; iOfFn<=8; iOfFn++)
  				{
  					if((*(pData+2)&0x01)==0x01)
  					{
  				    reportTask1.task[pn-1].dataUnit[numOfItem].pn1 = 1<<iOfPn;
  				    reportTask1.task[pn-1].dataUnit[numOfItem].pn2 = *(pData+1);
  
  						reportTask1.task[pn-1].dataUnit[numOfItem].fn = *(pData+3)*8 + iOfFn;
  						
  						numOfItem++;
  						
  						printf("numOfItem-1-1=%d\n", numOfItem);
  					}
  					
  					*(pData+2)>>=1;
  				}
				}
				else
				{
  				for(iOfPn=0; iOfPn<8; iOfPn++)
  			  {
    				if ((*pData&0x1)==0x1)
    				{
      				for(iOfFn=1; iOfFn<=8; iOfFn++)
      				{
      					if((*(pData+2)&0x01)==0x01)
      					{
      				    reportTask1.task[pn-1].dataUnit[numOfItem].pn1 = 1<<iOfPn;
      				    reportTask1.task[pn-1].dataUnit[numOfItem].pn2 = *(pData+1);
      
      						reportTask1.task[pn-1].dataUnit[numOfItem].fn = *(pData+3)*8 + iOfFn;
      						
      						numOfItem++;
      						
      						printf("numOfItem-1-2=%d\n", numOfItem);
      					}
      					
      					*(pData+2)>>=1;
      				}
      			}
    				
    				*pData>>=1;
    		  }
    		}
  		  
				pData += 4;
				offset04 += 4;
			}
			
			//�����վΪ�ѵ�����Fn,Pn�Ļ�,�������ݵ�Ԫ����
			reportTask1.task[pn-1].numOfDataId = numOfItem;
			
		}
		da1 >>=1;
	}
	
	if(errFlg == 1)
	{
		bzero(&reportTask1, sizeof(REPORT_TASK_PARA));
		reportTask1 = tmpreportTask;
		return FALSE;
	}
	
  //��ʼ��������Ϣ����
  if (initReportFlag==0)
  {
    initReportFlag = 1;
  }
	
	saveParameter(0x04, 65, (INT8U *)&reportTask1, sizeof(REPORT_TASK_PARA));
  return TRUE;
}

/*******************************************************
��������:AFN04066
��������:��Ӧ��վ���ò�������"��ʱ�ϱ�2��������������(F66)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04066(INT8U *pData)
{	
	REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U  da1, da2;			//�����
	INT8U  errFlg = 0;
	INT16U numOfItem = 0;    //2014-10-16,add
  INT8U  iOfPn,iOfFn;
  	
	tmpreportTask = reportTask2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 64)
			{
				return FALSE;
			}
			
			if(reportTask2.task[pn - 1].taskNum == 0)
			{
				reportTask2.numOfTask++;
				reportTask2.task[pn - 1].taskNum = pn;
			}
			
			//�ϱ����ڼ��������ڵ�λ
			reportTask2.task[pn - 1].sendPeriod = *pData++;
			offset04++;
			
			//�ϱ���׼ʱ��
			reportTask2.task[pn - 1].sendStart.second = (*pData >> 4) * 10 + (*pData & 0x0F);
			reportTask2.task[pn - 1].sendStart.minute = (*(pData + 1) >> 4) * 10 + (*(pData + 1) & 0x0F);
			reportTask2.task[pn - 1].sendStart.hour = (*(pData + 2) >> 4) * 10 + (*(pData + 2) & 0x0F);
			reportTask2.task[pn - 1].sendStart.day = (*(pData + 3) >> 4) * 10 + (*(pData + 3) & 0x0F);
			reportTask2.task[pn - 1].sendStart.month = ((*(pData + 4) & 0x10) >> 4) * 10 + (*(pData + 4) & 0x0F);
			reportTask2.task[pn - 1].week = *(pData + 4) & 0xE0;
			reportTask2.task[pn - 1].sendStart.year = (*(pData + 5) >> 4) * 10 + (*(pData + 5) & 0x0F);
			pData += 6;
			offset04 += 6;
			
			//�������ݳ�ȡ����
			reportTask2.task[pn - 1].sampleTimes = *pData++;
			offset04++;
		 #ifndef LIGHTING    //2016-10-25,����������ȡ������ж�,�����·���ѵ����Ϊ254
			if(reportTask2.task[pn - 1].sampleTimes > 96 || reportTask2.task[pn - 1].sampleTimes < 1)
			{
				errFlg = 1;
			}
		 #endif
			
			//���ݵ�Ԫ��ʶ����
			reportTask2.task[pn-1].numOfDataId = *pData++;
			offset04++;
			
			//���ݵ�Ԫ��ʶ
			numOfItem = 0;
			for(i=0; i<reportTask2.task[pn-1].numOfDataId; i++)
			{
				if (0x00==*pData && 0x00==*(pData+1))    //PN=0
				{
  				for(iOfFn=1; iOfFn<=8; iOfFn++)
  				{
  					if((*(pData+2)&0x01)==0x01)
  					{
  				    reportTask2.task[pn-1].dataUnit[numOfItem].pn1 = 1<<iOfPn;
  				    reportTask2.task[pn-1].dataUnit[numOfItem].pn2 = *(pData+1);
  
  						reportTask2.task[pn-1].dataUnit[numOfItem].fn = *(pData+3)*8 + iOfFn;
  						
  						numOfItem++;
  						
  						printf("numOfItem-2-1=%d\n", numOfItem);
  					}
  					
  					*(pData+2)>>=1;
  				}
				}
				else
				{
  				for(iOfPn=0; iOfPn<8; iOfPn++)
  			  {
    				if ((*pData&0x1)==0x1)
    				{
      				for(iOfFn=1; iOfFn<=8; iOfFn++)
      				{
      					if((*(pData+2)&0x01)==0x01)
      					{
      				    reportTask2.task[pn-1].dataUnit[numOfItem].pn1 = 1<<iOfPn;
      				    reportTask2.task[pn-1].dataUnit[numOfItem].pn2 = *(pData+1);
      
      						reportTask2.task[pn-1].dataUnit[numOfItem].fn = *(pData+3)*8 + iOfFn;
      						
      						numOfItem++;
      						
      						printf("numOfItem-2-2=%d\n", numOfItem);
      					}
      					
      					*(pData+2)>>=1;
      				}
      			}
    				
    				*pData>>=1;
    		  }
    		}
  		  
				pData += 4;
				offset04 += 4;
			}
			
			//�����վΪ�ѵ�����Fn,Pn�Ļ�,�������ݵ�Ԫ����
			reportTask2.task[pn-1].numOfDataId = numOfItem;
		}
		da1 >>=1;
	}
	
	if(errFlg == 1)
	{
		bzero(&reportTask2, sizeof(REPORT_TASK_PARA));
		reportTask1 = tmpreportTask;
		return FALSE;
	}
	
  //��ʼ��������Ϣ����
  if (initReportFlag==0)
  {
    initReportFlag = 1;
  }
	
	saveParameter(0x04, 66, (INT8U *)&reportTask2, sizeof(REPORT_TASK_PARA));

  return TRUE;
}

/*******************************************************
��������:AFN04067
��������:��Ӧ��վ���ò�������"��ʱ�ϱ�1��������������/ֹͣ����(F67)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04067(INT8U *pData)
{	
	REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�����
	INT8U errFlg = 0;
	
	tmpreportTask = reportTask1;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 64)
			{
				return FALSE;
			}
			
			if(reportTask1.task[pn - 1].taskNum == 0)
			{
				reportTask1.numOfTask++;
				reportTask1.task[pn - 1].taskNum = pn;
			}
			
			//����ֹͣ��־
			reportTask1.task[pn - 1].stopFlag = *pData++;
			offset04++;
		}
		da1 >>=1;
	}
	
	saveParameter(0x04, 65, (INT8U *)&reportTask1, sizeof(REPORT_TASK_PARA));
  return TRUE;
}

/*******************************************************
��������:AFN04068
��������:��Ӧ��վ���ò�������"��ʱ�ϱ�2��������������/ֹͣ����(F68)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04068(INT8U *pData)
{	
  REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�����
	INT8U errFlg = 0;
	
	tmpreportTask = reportTask2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			if(pn > 64)
			{
				return FALSE;
			}
			
			if(reportTask2.task[pn - 1].taskNum == 0)
			{
				reportTask2.numOfTask++;
				reportTask2.task[pn - 1].taskNum = pn;
			}
			
			//����ֹͣ��־
			reportTask2.task[pn - 1].stopFlag = *pData++;
			offset04++;
		}
		da1 >>=1;
	}
	
	saveParameter(0x04, 66, (INT8U *)&reportTask2, sizeof(REPORT_TASK_PARA));
  return TRUE;
}

/*******************************************************
��������:AFN04073
��������:��Ӧ��վ���ò�������"����������(F73)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04073(INT8U *pData)
{	
	CAPACITY_PARA capacityPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�������
	INT8U errFlg = 0;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			bzero(&capacityPara, sizeof(CAPACITY_PARA));
			
			//�����ݿ�ȡ���Ѵ��ڵ�����
			selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
			
			for(i=0;i<16;i++)
			{
				//������ʽ
				capacityPara.capacity[i].compensationMode = *pData++;
				if((capacityPara.capacity[i].compensationMode & 0xC0) == 0x40
					&& (capacityPara.capacity[i].compensationMode & 0x07) != 0x07)
				{
					errFlg = 1;
				}
				
				//����װ������
				capacityPara.capacity[i].capacityNum[0] = *pData++;
				capacityPara.capacity[i].capacityNum[1] = *pData++;
			}
			offset04 += 48;
			
			if(errFlg == 0)
			{
				saveViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
��������:AFN04074
��������:��Ӧ��վ���ò�������"������Ͷ�����в���(F74)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04074(INT8U *pData)
{	
	CAPACITY_RUN_PARA capacityRunPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�������
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//Ŀ�깦������
			capacityRunPara.targetPowerFactor[0] = *pData++;
			capacityRunPara.targetPowerFactor[1] = *pData++;
			
			//Ͷ���޹���������
			for(i=0;i<3;i++)
				capacityRunPara.onPowerLimit[i] = *pData++;
			
			//�г��޹���������
			for(i=0;i<3;i++)
				capacityRunPara.offPowerLimit[i] = *pData++;
			
			capacityRunPara.delay = *pData++;					//��ʱʱ��
			capacityRunPara.actInterval = *pData++;		//����ʱ����
			
			offset04 += 10;
		  saveViceParameter(0x04, 74, pn, (INT8U *)&capacityRunPara, sizeof(CAPACITY_RUN_PARA));
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
��������:AFN04075
��������:��Ӧ��վ���ò�������"��������������(F75)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04075(INT8U *pData)
{	
	CAPACITY_PROTECT_PARA capacityProtectPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�������
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//����ѹ
			capacityProtectPara.vSuperiod[0] = *pData++;
			capacityProtectPara.vSuperiod[1] = *pData++;
			
			//����ѹ�ز�ֵ
			capacityProtectPara.vSuperiodQuan[0] = *pData++;
			capacityProtectPara.vSuperiodQuan[1] = *pData++;
			
			//Ƿ��ѹ
			capacityProtectPara.vLack[0] = *pData++;
			capacityProtectPara.vLack[1] = *pData++;
			
			//Ƿ��ѹ�ز�ֵ
			capacityProtectPara.vLackQuan[0] = *pData++;
			capacityProtectPara.vLackQuan[1] = *pData++;
			
			//�ܻ����������������
			capacityProtectPara.cAbnormalUpLimit[0] = *pData++;
			capacityProtectPara.cAbnormalUpLimit[1] = *pData++;
			
			//�ܻ������������Խ�޻ز�ֵ
			capacityProtectPara.cAbnormalQuan[0] = *pData++;
			capacityProtectPara.cAbnormalQuan[1] = *pData++;
			
			//�ܻ����ѹ����������
			capacityProtectPara.vAbnormalUpLimit[0] = *pData++;
			capacityProtectPara.vAbnormalUpLimit[1] = *pData++;
			
			//�ܻ����ѹ������Խ�޻ز�ֵ
			capacityProtectPara.vAbnormalQuan[0] = *pData++;
			capacityProtectPara.vAbnormalQuan[1] = *pData++;
			
			offset04 += 16;
		  saveViceParameter(0x04, 75, pn, (INT8U *)&capacityProtectPara, sizeof(CAPACITY_PROTECT_PARA));
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
��������:AFN04076
��������:��Ӧ��վ���ò�������"������Ͷ�п��Ʒ�ʽ(F76)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04076(INT8U *pData)
{	
	CAPACITY_PARA capacityPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//�������
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;  	
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//���Ʒ�ʽ
			selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
			capacityPara.controlMode = *pData++;
			
			offset04++;
		  saveViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
��������:AFN04081
��������:��Ӧ��վ���ò�������"ֱ��ģ����������(F81)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04081(INT8U *pData)
{	
	ADC_PARA accPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//ֱ��ģ�����˿ں�
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//ȡ���ݿ��е�����
			selectViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
			
			//ֱ��ģ����������ʼֵ
			accPara.adcStartValue[0] = *pData++;
			accPara.adcStartValue[1] = *pData++;
			
			//ֱ��ģ����������ֵֹ
			accPara.adcEndValue[0] = *pData++;
			accPara.adcEndValue[1] = *pData++;
			
			offset04 += 4;
		  saveViceParameter(0x04, 81, pn, (INT8U *)&accPara, sizeof(ADC_PARA));
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
��������:AFN04082
��������:��Ӧ��վ���ò�������"ֱ��ģ������ֵ(F82)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04082(INT8U *pData)
{	
	ADC_PARA accPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//ֱ��ģ�����˿ں�
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			bzero(&accPara, sizeof(ADC_PARA));
			
			//ȡ���ݿ��е�����
			selectViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
			
			//ֱ��ģ��������
			accPara.adcUpLimit[0] = *pData++;
			accPara.adcUpLimit[1] = *pData++;
			
			//ֱ��ģ��������
			accPara.adcLowLimit[0] = *pData++;
			accPara.adcLowLimit[1] = *pData++;
			
			offset04 += 4;
		  saveViceParameter(0x04, 81, pn, (INT8U *)&accPara, sizeof(ADC_PARA));
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
��������:AFN04083
��������:��Ӧ��վ���ò�������"ֱ��ģ�����������(F83)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04083(INT8U *pData)
{	
	ADC_PARA accPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//ֱ��ģ�����˿ں�
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//ȡ���ݿ��е�����
			selectViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
			
			//ֱ��ģ����������ʼֵ
			accPara.adcFreezeDensity = *pData++;
			
			offset04++;
		  saveViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
		}
		da1 >>=1;
	}
	
  return TRUE;
}

#ifdef SDDL_CSM
/*******************************************************
��������:AFN04088
��������:��Ӧ��վ���ò�������"��������ܱ��ʲ����(FN88)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04088(INT8U *pData)
{
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//���ݵ�Ԫ��ʶ
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //ָ�����ݵ�Ԫ
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
      saveViceParameter(0x04, 88, pn, pData, 15);

  		offset04 += 15;
		}
		da1 >>=1;
	}
  
  return TRUE;
}

#endif

/*******************************************************
��������:AFN04097
��������:��Ӧ��վ���ò�������"�����ն�����(F97)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04097(INT8U *pData)
{
	INT8U i;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
    
  for(i = 0; i < 20; i++)
  {
  	teName[i] = *pData++;
  }
    
  offset04 += 20;
  saveParameter(0x04, 97,(INT8U *)teName, 20);
  return TRUE;
}

/*******************************************************
��������:AFN04098
��������:��Ӧ��վ���ò�������"����ϵͳ���б�ʶ��(F98)[�����Լ]"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04098(INT8U *pData)
{
	 INT8U i;

 	 //���ݵ�Ԫ��ʶ
 	 pData += 4;
 	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
    
   for(i = 0; i < 20; i++)
   {
  	 sysRunId[i] = *pData++;
   }
    
   offset04 += 20;
   saveParameter(0x04, 98,(INT8U *)sysRunId, 20);
  
  return TRUE;
}

/*******************************************************
��������:AFN04099
��������:��Ӧ��վ���ò�������"�ն˳�����������ʱ��(F99)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04099(INT8U *pData)
{
	 INT8U i;
	 
	 //���ݵ�Ԫ��ʶ
	 pData += 4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

   for(i = 0; i < 6; i++)
   {
  	 assignCopyTime[i] = *pData++;
   }
    
   offset04 += 6;
   saveParameter(0x04, 99,(INT8U *)assignCopyTime, 6);
  
   return TRUE;
}

/*******************************************************
��������:AFN040100
��������:��Ӧ��վ���ò�������"�ն�Ԥ��apn(F100)[�����Լ]"
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE or FALSE
*******************************************************/
BOOL AFN04100(INT8U *pData)
{
	INT8U i;
	
	//���ݵ�Ԫ��ʶ
	pData += 4;
	
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  for(i = 0; i < 4; i++)
  {
  	memcpy(&teApn[i],pData,16);
  	pData += 16;
  }
    
  offset04 += 64;
  saveParameter(0x04, 100, (INT8U *)teApn, 64);
  return TRUE;
}

/*******************************************************
��������:AFN04121
��������:��Ӧ��վ���ò�������"�ն���������<��չ>(F121)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04121(INT8U *pData)
{
   #ifdef TE_ADDR_USE_BCD_CODE
    INT16U tmpAddr;
   #endif

   //���ݵ�Ԫ��ʶ
   pData += 4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
  
   #ifdef TE_ADDR_USE_BCD_CODE
    tmpAddr = *pData | *(pData+1)<<8;
    pData += 2;
        
    addrField.a2[0] = ((tmpAddr%100)/10)<<4 | ((tmpAddr%100)%10);
    addrField.a2[1] = (tmpAddr/1000)<<4 | ((tmpAddr%1000)/100);
   #else
    addrField.a2[0] = *pData++;
    addrField.a2[1] = *pData++;
   #endif
   
   addrField.a1[0] = *pData++;
   addrField.a1[1] = *pData;
   
   offset04 += 4;
   
   saveParameter(0x04, 121,(INT8U *)&addrField,4);
   
   saveBakKeyPara(121);    //2012-8-9,add

   return TRUE;
}

/*******************************************************
��������:AFN040129
��������:��Ӧ��վ���ò�������"�ն˽�������У��ֵ<��չ>(F129)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04129(INT8U *pData)
{
 #ifdef AC_SAMPLE_DEVICE 
  offset04 += 81;
  
  //���ݵ�Ԫ��ʶ
 	pData += 4;   //ָ������Fn,Pn�Ժ�
 	
 	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  acSamplePara.lineInType = *pData++;
 	
 	acSamplePara.HFDouble = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.HFConst = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.UADCPga = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 
 	acSamplePara.Irechg = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	pData += 1;
 	
 	acSamplePara.Iregion4 = *pData | *(pData+1)<<8 | *(pData+2)<<16;
   pData += 3;
 
 	acSamplePara.Iregion3 = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.Iregion2 = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.Iregion1 = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
  acSamplePara.PhsreagA = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	  
  acSamplePara.PhsreagB = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	  
 	acSamplePara.PhsreagC = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.FailVoltage = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.Istartup = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.EAddMode = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.EnLineFreq = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.EnHarmonic = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.PgainA = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.PgainB = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.PgainC = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.UgainA = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.UgainB = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.UgainC = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.IgainA = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.IgainB = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.IgainC = *pData | *(pData+1)<<8 | *(pData+2)<<16;
 	pData += 3;
 	
 	acSamplePara.vAjustTimes = *pData | *(pData+1)<<8;
 	pData += 2;
 	
 	acSamplePara.cAjustTimes = *pData | *(pData+1)<<16;
  pData += 2;

  saveParameter(0x04, 129,(INT8U *)&acSamplePara,sizeof(AC_SAMPLE_PARA));
  
  saveBakKeyPara(129);   //2012-09-26,add

  resetAtt7022b(TRUE);   //��λATT7022B���·�У�����
  resetAtt7022b(TRUE);   //��λATT7022B���·�У�����
     
  return TRUE;
 
 #else
 
  return FALSE;
 
 #endif
}

/*******************************************************
��������:AFN040131
��������:��Ӧ��վ���ò�������"�ն˽�������У��ֵ<��չ>(F131)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04131(INT8U *pData)
{
	offset04 += 1;

	#ifdef AC_SAMPLE_DEVICE
	  //���ݵ�Ԫ��ʶ
	  pData += 4;
	  
	  //ָ�����ݵ�Ԫ
    pData = pDataUnit04;
	  
	  if (*pData==0x55)         //��ȡУ������ֵ
	  {
	    acMsa = addrField.a3;
	    
	  	readCheckData = 0x55;
      resetAtt7022b(FALSE);   //��λATT7022B�����·�У�����	  	
	  }
	  else                      //ֹͣ��ȡУ������
	  {
	  	readCheckData = 0x0;
      resetAtt7022b(TRUE);    //��λATT7022B���·�У�����
      
      acMsa = 0;
	  }
	  
	  
	  return TRUE;
	#else
	  return FALSE;
	#endif
	
	return TRUE;
}

/*******************************************************
��������:AFN040133
��������:��Ӧ��վ���ò�������"�ز����ڵ��ַ<�ֵ���չ>(F133)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04133(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
	 
	 memcpy(mainNodeAddr, pData, 6);
   
   saveParameter(0x04, 133, mainNodeAddr, 6);
	 
	 offset04 += 6;
	 
	 return TRUE;
}

/*******************************************************
��������:AFN040134
��������:��Ӧ��վ���ò�������"�豸���<�ֵ���չ>(F134)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04134(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
	 
	 deviceNumber = *pData | *(pData+1)<<8;
   
   saveParameter(0x04, 134, (INT8U *)&deviceNumber, 2);
	 
	 offset04 += 2;

	 return TRUE;
}

/*******************************************************
��������:AFN040135
��������:��Ӧ��վ���ò�������"���ģ�����<�ֵ���չ>(F135)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04135(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
	 
	 rlPara[0] = *pData++;
	 rlPara[1] = *pData++;
	 rlPara[2] = *pData++;
	 rlPara[3] = *pData++;
   
   saveParameter(0x04, 135, (INT8U *)&rlPara, 4);
	 
	 offset04 += 4;

	 return TRUE;
}

/*******************************************************
��������:AFN040136
��������:��Ӧ��վ���ò�������"��������<�ֵ���չ>(F136)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04136(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
	 
	 memcpy(csNameId, pData, 12);
   
   saveParameter(0x04, 136, (INT8U *)&csNameId, 12);
	 
	 offset04 += 12;

	 return TRUE;
}


/*******************************************************
��������:AFN040137
��������:��Ӧ��վ���ò�������"�������<�ֵ���չ>(F137)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04137(INT8U *pData)
{
   INT8U  port;
   INT8U  i;
	 
	 //���ݵ�Ԫ��ʶ
   port  =  findFnPn(*pData, *(pData+1), FIND_PN);
   if (port>0)
   {
   	 port-=1;
   }
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;

   //ˢ������ʾֵ
   pulseDataBuff[53*port]   = *(pData+1);
   pulseDataBuff[53*port+1] = *(pData+2);
   pulseDataBuff[53*port+2] = *(pData+3);
   
   //ˢ��С��
   pulse[port].pulseCount = (*pData)*pulse[port].meterConstant/100;
   
   //�������������ݻ���
 	 saveParameter(88, 3, pulseDataBuff, NUM_OF_SWITCH_PULSE*53);
 	 
 	 //�����������
 	 saveParameter(88, 13, pulse, sizeof(ONE_PULSE)*NUM_OF_SWITCH_PULSE);

	 offset04 += 4;

	 return TRUE;
}

/*******************************************************
��������:AFN040138
��������:��Ӧ��վ���ò�������"�����û�����������<��չ>(F138)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04138(INT8U *pData)
{
	 //���ݵ�Ԫ��ʶ
	 pData+=4;
	 
	 //ָ�����ݵ�Ԫ
   pData = pDataUnit04;
	 
	 denizenDataType = *pData;
	 
   saveParameter(0x04, 138, (INT8U *)&denizenDataType, 1);
	 
	 offset04 += 1;

	 return TRUE;
}

#ifdef SDDL_CSM

/*******************************************************
��������:AFN04224
��������:��Ӧ��վ���ò�������"�ն˵�ַ������<��չ>(F224)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN04224(INT8U *pData)
{
  //���ݵ�Ԫ��ʶ
  pData += 4;
	 
	//ָ�����ݵ�Ԫ
  pData = pDataUnit04;
  
  //��������
  addrField.a1[0] = *pData++;
  addrField.a1[1] = *pData++;
   
  //�ն˵�ַ
  addrField.a2[0] = *pData++;
  addrField.a2[1] = *pData++;
  
  saveParameter(0x04, 121,(INT8U *)&addrField,4);
  saveBakKeyPara(121);
  
  //�ն˹��緽ʽ
  saveParameter(0x04, 224, pData, 1);
  
  offset04 += 5;
  
  return TRUE;
}

#endif
