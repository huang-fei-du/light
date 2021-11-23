/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:statistics.c
����:leiyong
�汾:0.9
�������:2010��3��
����:�����ն�(�����ն�/������,AT91SAM9260������)ͳ�ƴ����ļ�
�����б�:

�޸���ʷ:
  01,10-03-19,Leiyong created.

***************************************************/
#include "common.h"
#include "dataBase.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "convert.h"

#include "statistics.h"

/*******************************************************
��������:initTeStatisRecord
��������:��ʼ���ն�ͳ�Ƽ�¼
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void initTeStatisRecord(TERMINAL_STATIS_RECORD *teRecord)
{  
   //.����ֵ������ͳ�Ƶĸ���ֵ
   teRecord->powerOnMinute = 0x0;       //����ʱ��
   teRecord->resetTimes=0x0;            //��λ����
   
   teRecord->closeLinkTimes=0;          //�����ر���·����
   teRecord->linkResetTimes=0;          //�������ر���·����
   teRecord->sysTimes=0;                //�������Ӵ���
   teRecord->sysSuccessTimes=0;         //���ӳɹ�����
   teRecord->heartTimes=0;              //�������Ӵ���
   teRecord->heartSuccessTimes=0;       //�������ӳɹ�����
   teRecord->receiveFrames=0;           //����֡��
   teRecord->receiveBytes=0;            //�����ֽ�   
   teRecord->sendFrames=0;              //����֡��
   teRecord->sendBytes=0;               //�����ֽ�
   
   teRecord->minSignal=0xee;            //�ź���Сֵ
   teRecord->minSignalTime[0]=0x0;      //�ź���Сֵ����ʱ��
   teRecord->minSignalTime[1]=0x0;      //�ź���Сֵ����ʱ��
   teRecord->minSignalTime[2]=0x0;      //�ź���Сֵ����ʱ��
   teRecord->maxSignal=0x0;             //�ź����ֵ
   teRecord->maxSignalTime[0]=0x0;      //�ź����ֵ����ʱ��
   teRecord->maxSignalTime[1]=0x0;      //�ź����ֵ����ʱ��
   teRecord->maxSignalTime[2]=0x0;      //�ź����ֵ����ʱ��   
   
   teRecord->totalCopyTimes=0x0;        //�ܳ������
   teRecord->overFlow = 0x0;            //�����Ƿ�����
   
  #ifdef LOAD_CTRL_MODULE 
   teRecord->monthCtrlJumpedDay =0x0;   //�µ����բ���ۼƴ���
   teRecord->chargeCtrlJumpedDay=0x0;   //�������բ���ۼƴ���
   teRecord->powerCtrlJumpedDay =0x0;   //������բ���ۼƴ���
   teRecord->remoteCtrlJumpedDay=0x0;   //ң����բ���ۼƴ���
  #endif
  
  #ifdef PLUG_IN_CARRIER_MODULE
   teRecord->dcOverUp   = 0;            //ֱ��ģ����Խ�����ۼ�ʱ��
   teRecord->dcOverDown = 0;            //ֱ��ģ����Խ�����ۼ�ʱ��
   teRecord->dcMax[0] = 0xee;           //ֱ��ģ�������ֵ
   teRecord->dcMax[1] = 0xee;
   teRecord->dcMaxTime[0] = 0xee;       //���ֵʱ��
   teRecord->dcMaxTime[1] = 0xee;       //
   teRecord->dcMaxTime[2] = 0xee;       //
   teRecord->dcMin[0] = 0xee;           //ֱ��ģ������Сֵ
   teRecord->dcMin[1] = 0xee;
   teRecord->dcMinTime[0] = 0xee;       //��Сֵʱ��
   teRecord->dcMinTime[1] = 0xee;       //
   teRecord->dcMinTime[2] = 0xee;       //
  #endif
  
  #ifdef LIGHTING
   teRecord->mixed = 0x0;               //����,2015-12-09
  #endif
}

/***************************************************
��������:dayPowerOnAmount
��������:��¼���ۼƹ���ʱ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void dayPowerOnAmount(void)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
   DATE_TIME              tmpTime;
   INT16U                 tmpData;
   
   tmpTime = timeHexToBcd(sysTime);
	 if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
   {
   	 initTeStatisRecord(&terminalStatisRecord);
   }
   	
   tmpData = delayedSpike(powerOnStatisTime, sysTime)/60;
   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("�����ϵ������=%d\n",tmpData);
   }
    
   tmpData += terminalStatisRecord.powerOnMinute;
 	 if (tmpData>1440)
 	 {
 	  	tmpData = 1440;
 	 }
 	 terminalStatisRecord.powerOnMinute = tmpData;
   
   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("�����ϵ������=%d\n",tmpData);
   }
  
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));

   powerOnStatisTime = sysTime;
}

/*******************************************************
��������:frameReport
��������:���ͽ�����֡��֡,�ֽڵȼ���
���ú���:
�����ú���:
�������:INT8U type,����1���ǽ���2
         INT8U item,����֡������˵��
               0 - ����¼�����ֽ���
               1 - ��Ҫͳ���շ������Ӵ���(����)��ɹ����Ӵ���(����)
               2 - ��Ҫͳ������������(�������/�ɹ���������)
�������:
����ֵ��
*******************************************************/
void frameReport(INT8U type, INT16U len, INT8U item)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
   DATE_TIME              tmpTime;
  
   INT8U                  eventData[20];
   INT32U                 tmpData;
   INT32U                 tmpFlow;
   
   tmpTime = timeHexToBcd(sysTime);
	 if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
   {
   	 initTeStatisRecord(&terminalStatisRecord);
   }
   
   if (type==1) //����
   {
     switch(item)
     {
   	   case 1:    //��¼�շ������Ӵ���
   	 	   terminalStatisRecord.sysTimes++;
   	 	   break;
   	 	 
   	   case 2:    //��¼�����������
   	 	   terminalStatisRecord.heartTimes++;
   	 	   break;
   	 }
     terminalStatisRecord.sendFrames++;      //����֡����1
     terminalStatisRecord.sendBytes += len;  //�����ֽڼӱ��η��͵��ֽ���
   }
   else         //����
   {
   	  switch(item)
   	  {
   	  	case 1:  //�ɹ����Ӵ���
   	  		terminalStatisRecord.sysSuccessTimes++;
     	 	  break;
     	 	  
     	 	case 2:  //�����ɹ�����
     	 		terminalStatisRecord.heartSuccessTimes++;
     	 		break;
   	  }
     	
     	if (item==0)
     	{
     	  terminalStatisRecord.receiveFrames++;     //����֡��
     	  terminalStatisRecord.receiveBytes += len; //�����ֽ���
     	}
   }   
   
   //�ж������Ƿ������¼�
   if ((eventRecordConfig.iEvent[3]&0x80)||(eventRecordConfig.nEvent[3] & 0x80))
   {
     if (terminalStatisRecord.overFlow==0)
     {
   	    tmpFlow = upTranslateLimit[0]|upTranslateLimit[1]<<8|upTranslateLimit[2]<<16|upTranslateLimit[3]<<24;
   	    if (tmpData>tmpFlow && tmpFlow!=0)
   	    {
   	    	 terminalStatisRecord.overFlow = 1;   //����������
   	    	 
   	    	 eventData[0] = 32;       //ERC
   	    	 eventData[1] = 16;       //����
   	    	 
   	    	 //����ʱ��
           eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
           eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
           eventData[4] = sysTime.hour  /10<<4 | sysTime.hour  %10;
           eventData[5] = sysTime.day   /10<<4 | sysTime.day   %10;
           eventData[6] = sysTime.month /10<<4 | sysTime.month %10;
           eventData[7] = sysTime.year  /10<<4 | sysTime.year  %10;
           
           //�����ѷ�����ͨ������
           eventData[8] = tmpData&0xff;
           eventData[9] = tmpData>>8&0xff;
           eventData[10] = tmpData>>16&0xff;
           eventData[11] = tmpData>>24&0xff;
           
           //��������
           eventData[12] = upTranslateLimit[0];
           eventData[13] = upTranslateLimit[1];
           eventData[14] = upTranslateLimit[2];
           eventData[15] = upTranslateLimit[3];
           
           if (eventRecordConfig.iEvent[3] & 0x80) 
           {
             writeEvent(eventData, 16, 1, DATA_FROM_GPRS);
           }
           if (eventRecordConfig.nEvent[3] & 0x80)
           {
             writeEvent(eventData, 16, 2, DATA_FROM_LOCAL);
           }
           
           if (type==1)
           {
             if (debugInfo&PRINT_EVENT_DEBUG)
             {
               printf("frameReport���������ϱ�\n");
             }
             
             activeReport3();   //�����ϱ��¼�
           }
   	    }
     }
   }
  
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
}

/***************************************************
��������:powerOffEvent
��������:�ն�ͣ/�ϵ��¼�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void  powerOffEvent(BOOL powerOff)
{
   INT8U  eventData[14];

   if (
   	  #ifndef SDDL_CSM    //ɽ��������˾Ҫ�󲻹��Ƿ�����Ϊ��Ҫ�¼���Ҫ�����ϱ�ͣ���¼�
   	   (eventRecordConfig.iEvent[1] & 0x20) || 
   	  #endif
   	  (eventRecordConfig.nEvent[1] & 0x20))
   {
   	 eventData[0] = 0x0e;
     eventData[1] = 0xe;
     if (powerOff==TRUE)   //�ϵ��¼�
     {
       eventData[2] = powerOnOffRecord.powerOnOffTime.second/10<<4 | powerOnOffRecord.powerOnOffTime.second%10;
       eventData[3] = powerOnOffRecord.powerOnOffTime.minute/10<<4 | powerOnOffRecord.powerOnOffTime.minute%10;
       eventData[4] = powerOnOffRecord.powerOnOffTime.hour/10<<4 | powerOnOffRecord.powerOnOffTime.hour%10;
       eventData[5] = powerOnOffRecord.powerOnOffTime.day/10<<4 | powerOnOffRecord.powerOnOffTime.day%10;
       eventData[6] = powerOnOffRecord.powerOnOffTime.month/10<<4 | powerOnOffRecord.powerOnOffTime.month%10;
       eventData[7] = powerOnOffRecord.powerOnOffTime.year/10<<4 | powerOnOffRecord.powerOnOffTime.year%10;
       eventData[8] = sysTime.second/10<<4 | sysTime.second%10;
       eventData[9] = sysTime.minute/10<<4 | sysTime.minute%10;
       eventData[10] = sysTime.hour/10<<4 | sysTime.hour%10;
       eventData[11] = sysTime.day/10<<4 | sysTime.day%10;
       eventData[12] = sysTime.month/10<<4 | sysTime.month%10;
       eventData[13] = sysTime.year/10<<4 | sysTime.year%10;
     	 powerOnOffRecord.powerOnOrOff = 0x11;   //��־Ϊ�ϵ�
     }
     else                  //ͣ���¼�
     {
         eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         eventData[4] = sysTime.hour/10<<4 | sysTime.hour%10;
         eventData[5] = sysTime.day/10<<4 | sysTime.day%10;
         eventData[6] = sysTime.month/10<<4 | sysTime.month%10;
         eventData[7] = sysTime.year/10<<4 | sysTime.year%10;
         eventData[8] = powerOnOffRecord.powerOnOffTime.second/10<<4 | powerOnOffRecord.powerOnOffTime.second%10;
         eventData[9] = powerOnOffRecord.powerOnOffTime.minute/10<<4 | powerOnOffRecord.powerOnOffTime.minute%10;
         eventData[10] = powerOnOffRecord.powerOnOffTime.hour/10<<4 | powerOnOffRecord.powerOnOffTime.hour%10;
         eventData[11] = powerOnOffRecord.powerOnOffTime.day/10<<4 | powerOnOffRecord.powerOnOffTime.day%10;
         eventData[12] = powerOnOffRecord.powerOnOffTime.month/10<<4 | powerOnOffRecord.powerOnOffTime.month%10;
         eventData[13] = powerOnOffRecord.powerOnOffTime.year/10<<4 | powerOnOffRecord.powerOnOffTime.year%10;
   	     powerOnOffRecord.powerOnOrOff = 0x22;   //��־Ϊͣ��
     }

    #ifndef RECORD_POWER_OFF_EVENT
     if (powerOff==TRUE)   //�����Ҫ���¼ͣ���¼�,��ֻ��¼�ϵ��¼�
     {
    #endif
      if (eventRecordConfig.iEvent[1] & 0x20)
      {
        writeEvent(eventData,14, 1, DATA_FROM_GPRS);    //������Ҫ�¼�����
      }
      if (eventRecordConfig.nEvent[1] & 0x20)
      {
        writeEvent(eventData,14,2, DATA_FROM_LOCAL);    //����һ���¼�����
      }
       
      eventStatus[1] = eventStatus[1] | 0x20;
    
    #ifndef RECORD_POWER_OFF_EVENT       //��¼ͣ���¼�
     } 
    #endif
    
     //��¼ͣ��/�ϵ�ʱ��
   	 powerOnOffRecord.powerOnOffTime = sysTime;

   	 saveParameter(88, 1,(INT8U *)&powerOnOffRecord,sizeof(POWER_ON_OFF));
   }
}

/*******************************************************
��������:teResetReport
��������:��¼�ն˵��ո�λ����
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
void teResetReport(void)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
   DATE_TIME              tmpTime; 
   
   tmpTime = timeHexToBcd(sysTime);
	 if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
   {
   	 initTeStatisRecord(&terminalStatisRecord);
   }
   
   terminalStatisRecord.resetTimes++;
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
}

/*******************************************************
��������:initMpStatis
��������:��ʼ��������ͳ�Ƽ�¼
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void initMpStatis(void *record, INT8U type)
{ 
  METER_STATIS_EXTRAN_TIME   *meterStatisExtranTime;
  METER_STATIS_BEARON_TIME   *meterStatisBearonTime;
  METER_STATIS_EXTRAN_TIME_S *meterStatisExtranTimeS;
 #ifdef LIGHTING
  KZQ_STATIS_EXTRAN_TIME     *kzqStatisExtranTime;
 #endif

  switch (type)
  {
    case 1:   //�������ʱ���޹���
      meterStatisExtranTime = (METER_STATIS_EXTRAN_TIME *)record;
      
      //���߱�־
      meterStatisExtranTime->flyFlag = 0x0;
    	 
    	//ʾ���½���־
    	meterStatisExtranTime->reverseFlag = 0x0;
    
      //��ѹ��ƽ���¼����ֵ
      meterStatisExtranTime->vUnBalance = VOLTAGE_UNBALANCE_NOT;
      meterStatisExtranTime->vUnBalanceTime.year=0xff;
      
      //������ƽ���¼����ֵ
      meterStatisExtranTime->cUnBalance = CURRENT_UNBALANCE_NOT;
      meterStatisExtranTime->cUnBalanceTime.year=0xff;
      
      //��ѹԽ�޸���ֵ
      meterStatisExtranTime->vOverLimit = 0x0;
      meterStatisExtranTime->vUpUpTime[0].year=0xff;
      meterStatisExtranTime->vUpUpTime[1].year=0xff;
      meterStatisExtranTime->vUpUpTime[2].year=0xff;
      meterStatisExtranTime->vDownDownTime[0].year=0xff;
      meterStatisExtranTime->vDownDownTime[1].year=0xff;
      meterStatisExtranTime->vDownDownTime[2].year=0xff;
      
      //����Խ�޸���ֵ
      meterStatisExtranTime->cOverLimit = 0x0;
      meterStatisExtranTime->cUpTime[0].year=0xff;
      meterStatisExtranTime->cUpTime[1].year=0xff;
      meterStatisExtranTime->cUpTime[2].year=0xff;
      meterStatisExtranTime->cUpUpTime[0].year=0xff;
      meterStatisExtranTime->cUpUpTime[1].year=0xff;
      meterStatisExtranTime->cUpUpTime[2].year=0xff;
      
      //��ѹ��·�쳣(����)״̬����ֵ
      meterStatisExtranTime->phaseBreak = 0x0;
      
      //��ѹ��·�쳣(ʧѹ)״̬����ֵ
      meterStatisExtranTime->loseVoltage = 0x0;
    
      //���ڹ����쳣״̬����ֵ
      meterStatisExtranTime->apparentPower = 0x0;
      meterStatisExtranTime->apparentUpTime.year=0xff;
      meterStatisExtranTime->apparentUpUpTime.year=0xff;
      
      //����(bit0-���ܱ�ʱ�䳬���־,bit1-485����ʧ�ܱ�־)
      meterStatisExtranTime->mixed = 0x0;
      
      //��·��·�쳣(�����־)����ֵ
      meterStatisExtranTime->currentLoop = 0x0;
      break;
      
    case 2:    //�������ʱ���й���
      meterStatisBearonTime = (METER_STATIS_BEARON_TIME *)record;

      meterStatisBearonTime->copySuccessTimes       = 0x0;
      meterStatisBearonTime->lastCopySuccessTime[0] = 0x0;
      meterStatisBearonTime->lastCopySuccessTime[1] = 0x0;
      meterStatisBearonTime->mixed                  = 0x0;
      break;
      
    case 3:
      meterStatisExtranTimeS = (METER_STATIS_EXTRAN_TIME_S *)record;
      meterStatisExtranTimeS->emptyByte1 = 0x00;
      meterStatisExtranTimeS->mixed      = 0x00;
      
     #ifdef LIGHTING
      meterStatisExtranTimeS->lastFailure.year = 0xff;
      meterStatisExtranTimeS->lastDip.year = 0xff;
     #endif
    	break;
    	
   #ifdef LIGHTING
    case 4:   //�������ʱ���޹���
      kzqStatisExtranTime = (KZQ_STATIS_EXTRAN_TIME *)record;
      
      //��ѹԽ�޸���ֵ
      kzqStatisExtranTime->vOverLimit = 0x0;
      kzqStatisExtranTime->vUpUpTime[0].year=0xff;
      kzqStatisExtranTime->vUpUpTime[1].year=0xff;
      kzqStatisExtranTime->vUpUpTime[2].year=0xff;
      kzqStatisExtranTime->vDownDownTime[0].year=0xff;
      kzqStatisExtranTime->vDownDownTime[1].year=0xff;
      kzqStatisExtranTime->vDownDownTime[2].year=0xff;
      
      //����Խ�޸���ֵ
      kzqStatisExtranTime->cOverLimit = 0x0;
      kzqStatisExtranTime->cUpUpTime[0].year=0xff;
      kzqStatisExtranTime->cUpUpTime[1].year=0xff;
      kzqStatisExtranTime->cUpUpTime[2].year=0xff;
      kzqStatisExtranTime->cDownDownTime[0].year=0xff;
      kzqStatisExtranTime->cDownDownTime[1].year=0xff;
      kzqStatisExtranTime->cDownDownTime[2].year=0xff;
    
      //����Խ�޸���ֵ
      kzqStatisExtranTime->powerLimit = 0x0;
      kzqStatisExtranTime->powerUpTime.year=0xff;
      kzqStatisExtranTime->powerDownTime.year=0xff;

      //��������Խ�޸���ֵ
      kzqStatisExtranTime->factorLimit = 0x0;
      kzqStatisExtranTime->factorDownTime.year=0xff;
      
      break;
   #endif
  }
}


#ifdef PLUG_IN_CARRIER_MODULE



/***************************************************
��������:format2ToHex
��������:���ݸ�ʽ2���16��������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
INT32U format2ToHex(INT16U format2Data)
{
   INT32U tmpData;
   tmpData = bcdToHex(format2Data&0xfff);
   
   switch(format2Data>>13)
   {
   	 case 0:    //10^4
   	 	 tmpData *= 10000000;
   	 	 break;
   	 
   	 case 1:    //10^3
   	 	 tmpData *= 1000000;
   	 	 break;

   	 case 2:    //10^2
   	 	 tmpData *= 100000;
   	 	 break;

   	 case 3:    //10^1
   	 	 tmpData *= 10000;
   	 	 break;

   	 case 4:    //10^0
   	 	 tmpData *= 1000;
   	 	 break;

   	 case 5:    //10^-1
   	 	 tmpData *= 100;
   	 	 break;

   	 case 6:    //10^-2
   	 	 tmpData *= 10;
   	 	 break;

   	 case 7:    //10^-3
   	 	 tmpData *= 1;
   	 	 break;
   }
   
   return tmpData;
}

/***************************************************
��������:dayPowerOnAmount
��������:��¼���ۼƹ���ʱ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void dcAnalogStatis(INT16U dcNow)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord; //�ն�ͳ�Ƽ�¼
   ADC_PARA               adcPara;              //ֱ��ģ�������ò���(AFN04-FN81,FN82,FN83)
   DATE_TIME              tmpTime;
   INT32U                 tmpUpLimit;
   INT32U                 tmpDownLimit;
   INT32U                 tmpOrigMax;
   INT32U                 tmpOrigMin;
   INT8U                  needSaveDc = 0;
   
   if (selectViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA))==TRUE)
   {
     tmpTime = timeHexToBcd(sysTime);
	   if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
     {
   	   initTeStatisRecord(&terminalStatisRecord);
     }
     
     tmpUpLimit =format2ToHex(adcPara.adcUpLimit[0] | adcPara.adcUpLimit[1]<<8);
     tmpDownLimit = format2ToHex(adcPara.adcLowLimit[0] | adcPara.adcLowLimit[1]<<8);
     
     if (debugInfo&PRINT_DATA_BASE)
     {
       printf("��ǰֱ��ģ����ֵ=%d,����=%d,����=%d\n", dcNow*100, tmpUpLimit, tmpDownLimit);
     }
     
     if (dcNow*100>tmpUpLimit)
     {
       if (debugInfo&PRINT_DATA_BASE)
       {
     	   printf("ֱ��ģ����Խ����\n");
     	 }
     	 
     	 terminalStatisRecord.dcOverUp++;
     	 needSaveDc = 1;
     }
     
     if (dcNow*100<tmpDownLimit)
     {
       if (debugInfo&PRINT_DATA_BASE)
       {
     	   printf("ֱ��ģ����Խ����\n");
     	 }
     	 terminalStatisRecord.dcOverDown++;
     	 needSaveDc = 1;
     }


     if (terminalStatisRecord.dcMax[0]!=0xee)
     {
       tmpOrigMax = format2ToHex(terminalStatisRecord.dcMax[0] | terminalStatisRecord.dcMax[1]<<8);
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("ԭֱ��ģ�������ֵ=%d\n",tmpOrigMax);
       }
       if (dcNow*100>tmpOrigMax)
       {
     	   terminalStatisRecord.dcMax[0] = hexToBcd(adcData);
         terminalStatisRecord.dcMax[1] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
         terminalStatisRecord.dcMaxTime[0] = hexToBcd(sysTime.minute);
         terminalStatisRecord.dcMaxTime[1] = hexToBcd(sysTime.hour);
         terminalStatisRecord.dcMaxTime[2] = hexToBcd(sysTime.day);
         needSaveDc = 1;

         if (debugInfo&PRINT_DATA_BASE)
         {
           printf("����ֱ��ģ�������ֵ������ʱ��\n");
         }
       }
     }
     else
     {
     	 terminalStatisRecord.dcMax[0] = hexToBcd(adcData);
       terminalStatisRecord.dcMax[1] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
       terminalStatisRecord.dcMaxTime[0] = hexToBcd(sysTime.minute);
       terminalStatisRecord.dcMaxTime[1] = hexToBcd(sysTime.hour);
       terminalStatisRecord.dcMaxTime[2] = hexToBcd(sysTime.day);
       needSaveDc = 1;
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("�ó�ֵ(ֱ��ģ�������ֵ������ʱ��)\n");
       }
     }

     if (terminalStatisRecord.dcMin[0]!=0xee)
     {
       tmpOrigMin = format2ToHex(terminalStatisRecord.dcMin[0] | terminalStatisRecord.dcMin[1]<<8);
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("ԭֱ��ģ������Сֵ=%d\n",tmpOrigMin);
       }
       
       if (dcNow*100<tmpOrigMin)
       {
     	   terminalStatisRecord.dcMin[0] = hexToBcd(adcData);
         terminalStatisRecord.dcMin[1] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
         terminalStatisRecord.dcMinTime[0] = hexToBcd(sysTime.minute);
         terminalStatisRecord.dcMinTime[1] = hexToBcd(sysTime.hour);
         terminalStatisRecord.dcMinTime[2] = hexToBcd(sysTime.day);
         needSaveDc = 1;
         
         if (debugInfo&PRINT_DATA_BASE)
         {
           printf("����ֱ��ģ������Сֵ������ʱ��\n");
         }
       }
     }
     else
     {
     	 terminalStatisRecord.dcMin[0] = hexToBcd(adcData);
       terminalStatisRecord.dcMin[1] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
       terminalStatisRecord.dcMinTime[0] = hexToBcd(sysTime.minute);
       terminalStatisRecord.dcMinTime[1] = hexToBcd(sysTime.hour);
       terminalStatisRecord.dcMinTime[2] = hexToBcd(sysTime.day);
       needSaveDc = 1;
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("�ó�ֵ(ֱ��ģ������Сֵ������ʱ��)\n");
       }
     }
   
     if (needSaveDc==1)
     {
       saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0, sizeof(TERMINAL_STATIS_RECORD));
     }
   }
}

#endif

