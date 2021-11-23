/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:statistics.c
作者:leiyong
版本:0.9
完成日期:2010年3月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)统计处理文件
函数列表:

修改历史:
  01,10-03-19,Leiyong created.

***************************************************/
#include "common.h"
#include "dataBase.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "convert.h"

#include "statistics.h"

/*******************************************************
函数名称:initTeStatisRecord
功能描述:初始化终端统计记录
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void initTeStatisRecord(TERMINAL_STATIS_RECORD *teRecord)
{  
   //.其它值赋按日统计的赋初值
   teRecord->powerOnMinute = 0x0;       //供电时间
   teRecord->resetTimes=0x0;            //复位次数
   
   teRecord->closeLinkTimes=0;          //主动关闭链路次数
   teRecord->linkResetTimes=0;          //非主动关闭链路次数
   teRecord->sysTimes=0;                //发起连接次数
   teRecord->sysSuccessTimes=0;         //连接成功次数
   teRecord->heartTimes=0;              //心跳连接次数
   teRecord->heartSuccessTimes=0;       //心跳连接成功次数
   teRecord->receiveFrames=0;           //接收帧数
   teRecord->receiveBytes=0;            //接收字节   
   teRecord->sendFrames=0;              //发送帧数
   teRecord->sendBytes=0;               //发送字节
   
   teRecord->minSignal=0xee;            //信号最小值
   teRecord->minSignalTime[0]=0x0;      //信号最小值发生时间
   teRecord->minSignalTime[1]=0x0;      //信号最小值发生时间
   teRecord->minSignalTime[2]=0x0;      //信号最小值发生时间
   teRecord->maxSignal=0x0;             //信号最大值
   teRecord->maxSignalTime[0]=0x0;      //信号最大值发生时间
   teRecord->maxSignalTime[1]=0x0;      //信号最大值发生时间
   teRecord->maxSignalTime[2]=0x0;      //信号最大值发生时间   
   
   teRecord->totalCopyTimes=0x0;        //总抄表次数
   teRecord->overFlow = 0x0;            //流量是否超流量
   
  #ifdef LOAD_CTRL_MODULE 
   teRecord->monthCtrlJumpedDay =0x0;   //月电控跳闸日累计次数
   teRecord->chargeCtrlJumpedDay=0x0;   //购电控跳闸日累计次数
   teRecord->powerCtrlJumpedDay =0x0;   //功控跳闸日累计次数
   teRecord->remoteCtrlJumpedDay=0x0;   //遥控跳闸日累计次数
  #endif
  
  #ifdef PLUG_IN_CARRIER_MODULE
   teRecord->dcOverUp   = 0;            //直流模拟量越上限累计时间
   teRecord->dcOverDown = 0;            //直流模拟量越上限累计时间
   teRecord->dcMax[0] = 0xee;           //直流模拟量最大值
   teRecord->dcMax[1] = 0xee;
   teRecord->dcMaxTime[0] = 0xee;       //最大值时间
   teRecord->dcMaxTime[1] = 0xee;       //
   teRecord->dcMaxTime[2] = 0xee;       //
   teRecord->dcMin[0] = 0xee;           //直流模拟量最小值
   teRecord->dcMin[1] = 0xee;
   teRecord->dcMinTime[0] = 0xee;       //最小值时间
   teRecord->dcMinTime[1] = 0xee;       //
   teRecord->dcMinTime[2] = 0xee;       //
  #endif
  
  #ifdef LIGHTING
   teRecord->mixed = 0x0;               //杂项,2015-12-09
  #endif
}

/***************************************************
函数名称:dayPowerOnAmount
功能描述:记录日累计供电时间
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void dayPowerOnAmount(void)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
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
     printf("本次上电分钟数=%d\n",tmpData);
   }
    
   tmpData += terminalStatisRecord.powerOnMinute;
 	 if (tmpData>1440)
 	 {
 	  	tmpData = 1440;
 	 }
 	 terminalStatisRecord.powerOnMinute = tmpData;
   
   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("本日上电分钟数=%d\n",tmpData);
   }
  
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));

   powerOnStatisTime = sysTime;
}

/*******************************************************
函数名称:frameReport
功能描述:发送接收主帧的帧,字节等计数
调用函数:
被调用函数:
输入参数:INT8U type,发送1或是接收2
         INT8U item,发送帧的子项说明
               0 - 仅记录发送字节数
               1 - 需要统计日发起连接次数(发送)或成功连接次数(接收)
               2 - 需要统计日心跳次数(发起次数/成功心跳次数)
输出参数:
返回值：
*******************************************************/
void frameReport(INT8U type, INT16U len, INT8U item)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
   DATE_TIME              tmpTime;
  
   INT8U                  eventData[20];
   INT32U                 tmpData;
   INT32U                 tmpFlow;
   
   tmpTime = timeHexToBcd(sysTime);
	 if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
   {
   	 initTeStatisRecord(&terminalStatisRecord);
   }
   
   if (type==1) //发送
   {
     switch(item)
     {
   	   case 1:    //记录日发起连接次数
   	 	   terminalStatisRecord.sysTimes++;
   	 	   break;
   	 	 
   	   case 2:    //记录心跳发起次数
   	 	   terminalStatisRecord.heartTimes++;
   	 	   break;
   	 }
     terminalStatisRecord.sendFrames++;      //发送帧数加1
     terminalStatisRecord.sendBytes += len;  //发送字节加本次发送的字节数
   }
   else         //接收
   {
   	  switch(item)
   	  {
   	  	case 1:  //成功连接次数
   	  		terminalStatisRecord.sysSuccessTimes++;
     	 	  break;
     	 	  
     	 	case 2:  //心跳成功次数
     	 		terminalStatisRecord.heartSuccessTimes++;
     	 		break;
   	  }
     	
     	if (item==0)
     	{
     	  terminalStatisRecord.receiveFrames++;     //接收帧数
     	  terminalStatisRecord.receiveBytes += len; //接收字节数
     	}
   }   
   
   //判断流量是否超门限事件
   if ((eventRecordConfig.iEvent[3]&0x80)||(eventRecordConfig.nEvent[3] & 0x80))
   {
     if (terminalStatisRecord.overFlow==0)
     {
   	    tmpFlow = upTranslateLimit[0]|upTranslateLimit[1]<<8|upTranslateLimit[2]<<16|upTranslateLimit[3]<<24;
   	    if (tmpData>tmpFlow && tmpFlow!=0)
   	    {
   	    	 terminalStatisRecord.overFlow = 1;   //流量超门限
   	    	 
   	    	 eventData[0] = 32;       //ERC
   	    	 eventData[1] = 16;       //长度
   	    	 
   	    	 //发生时间
           eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
           eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
           eventData[4] = sysTime.hour  /10<<4 | sysTime.hour  %10;
           eventData[5] = sysTime.day   /10<<4 | sysTime.day   %10;
           eventData[6] = sysTime.month /10<<4 | sysTime.month %10;
           eventData[7] = sysTime.year  /10<<4 | sysTime.year  %10;
           
           //当月已发生的通信流量
           eventData[8] = tmpData&0xff;
           eventData[9] = tmpData>>8&0xff;
           eventData[10] = tmpData>>16&0xff;
           eventData[11] = tmpData>>24&0xff;
           
           //流量门限
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
               printf("frameReport调用主动上报\n");
             }
             
             activeReport3();   //主动上报事件
           }
   	    }
     }
   }
  
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
}

/***************************************************
函数名称:powerOffEvent
功能描述:终端停/上电事件
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void  powerOffEvent(BOOL powerOff)
{
   INT8U  eventData[14];

   if (
   	  #ifndef SDDL_CSM    //山东电力公司要求不管是否配置为重要事件都要主动上报停电事件
   	   (eventRecordConfig.iEvent[1] & 0x20) || 
   	  #endif
   	  (eventRecordConfig.nEvent[1] & 0x20))
   {
   	 eventData[0] = 0x0e;
     eventData[1] = 0xe;
     if (powerOff==TRUE)   //上电事件
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
     	 powerOnOffRecord.powerOnOrOff = 0x11;   //标志为上电
     }
     else                  //停电事件
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
   	     powerOnOffRecord.powerOnOrOff = 0x22;   //标志为停电
     }

    #ifndef RECORD_POWER_OFF_EVENT
     if (powerOff==TRUE)   //如果不要求记录停电事件,则只记录上电事件
     {
    #endif
      if (eventRecordConfig.iEvent[1] & 0x20)
      {
        writeEvent(eventData,14, 1, DATA_FROM_GPRS);    //记入重要事件队列
      }
      if (eventRecordConfig.nEvent[1] & 0x20)
      {
        writeEvent(eventData,14,2, DATA_FROM_LOCAL);    //记入一般事件队列
      }
       
      eventStatus[1] = eventStatus[1] | 0x20;
    
    #ifndef RECORD_POWER_OFF_EVENT       //记录停电事件
     } 
    #endif
    
     //记录停电/上电时间
   	 powerOnOffRecord.powerOnOffTime = sysTime;

   	 saveParameter(88, 1,(INT8U *)&powerOnOffRecord,sizeof(POWER_ON_OFF));
   }
}

/*******************************************************
函数名称:teResetReport
功能描述:记录终端当日复位次数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void teResetReport(void)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
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
函数名称:initMpStatis
功能描述:初始化测量点统计记录
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
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
    case 1:   //三相表与时间无关量
      meterStatisExtranTime = (METER_STATIS_EXTRAN_TIME *)record;
      
      //飞走标志
      meterStatisExtranTime->flyFlag = 0x0;
    	 
    	//示度下降标志
    	meterStatisExtranTime->reverseFlag = 0x0;
    
      //电压不平衡记录赋初值
      meterStatisExtranTime->vUnBalance = VOLTAGE_UNBALANCE_NOT;
      meterStatisExtranTime->vUnBalanceTime.year=0xff;
      
      //电流不平衡记录赋初值
      meterStatisExtranTime->cUnBalance = CURRENT_UNBALANCE_NOT;
      meterStatisExtranTime->cUnBalanceTime.year=0xff;
      
      //电压越限赋初值
      meterStatisExtranTime->vOverLimit = 0x0;
      meterStatisExtranTime->vUpUpTime[0].year=0xff;
      meterStatisExtranTime->vUpUpTime[1].year=0xff;
      meterStatisExtranTime->vUpUpTime[2].year=0xff;
      meterStatisExtranTime->vDownDownTime[0].year=0xff;
      meterStatisExtranTime->vDownDownTime[1].year=0xff;
      meterStatisExtranTime->vDownDownTime[2].year=0xff;
      
      //电流越限赋初值
      meterStatisExtranTime->cOverLimit = 0x0;
      meterStatisExtranTime->cUpTime[0].year=0xff;
      meterStatisExtranTime->cUpTime[1].year=0xff;
      meterStatisExtranTime->cUpTime[2].year=0xff;
      meterStatisExtranTime->cUpUpTime[0].year=0xff;
      meterStatisExtranTime->cUpUpTime[1].year=0xff;
      meterStatisExtranTime->cUpUpTime[2].year=0xff;
      
      //电压回路异常(断相)状态赋初值
      meterStatisExtranTime->phaseBreak = 0x0;
      
      //电压回路异常(失压)状态赋初值
      meterStatisExtranTime->loseVoltage = 0x0;
    
      //视在功率异常状态赋初值
      meterStatisExtranTime->apparentPower = 0x0;
      meterStatisExtranTime->apparentUpTime.year=0xff;
      meterStatisExtranTime->apparentUpUpTime.year=0xff;
      
      //杂项(bit0-电能表时间超差标志,bit1-485抄表失败标志)
      meterStatisExtranTime->mixed = 0x0;
      
      //电路回路异常(反向标志)赋初值
      meterStatisExtranTime->currentLoop = 0x0;
      break;
      
    case 2:    //三相表与时间有关量
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
    case 4:   //三相表与时间无关量
      kzqStatisExtranTime = (KZQ_STATIS_EXTRAN_TIME *)record;
      
      //电压越限赋初值
      kzqStatisExtranTime->vOverLimit = 0x0;
      kzqStatisExtranTime->vUpUpTime[0].year=0xff;
      kzqStatisExtranTime->vUpUpTime[1].year=0xff;
      kzqStatisExtranTime->vUpUpTime[2].year=0xff;
      kzqStatisExtranTime->vDownDownTime[0].year=0xff;
      kzqStatisExtranTime->vDownDownTime[1].year=0xff;
      kzqStatisExtranTime->vDownDownTime[2].year=0xff;
      
      //电流越限赋初值
      kzqStatisExtranTime->cOverLimit = 0x0;
      kzqStatisExtranTime->cUpUpTime[0].year=0xff;
      kzqStatisExtranTime->cUpUpTime[1].year=0xff;
      kzqStatisExtranTime->cUpUpTime[2].year=0xff;
      kzqStatisExtranTime->cDownDownTime[0].year=0xff;
      kzqStatisExtranTime->cDownDownTime[1].year=0xff;
      kzqStatisExtranTime->cDownDownTime[2].year=0xff;
    
      //功率越限赋初值
      kzqStatisExtranTime->powerLimit = 0x0;
      kzqStatisExtranTime->powerUpTime.year=0xff;
      kzqStatisExtranTime->powerDownTime.year=0xff;

      //功率因数越限赋初值
      kzqStatisExtranTime->factorLimit = 0x0;
      kzqStatisExtranTime->factorDownTime.year=0xff;
      
      break;
   #endif
  }
}


#ifdef PLUG_IN_CARRIER_MODULE



/***************************************************
函数名称:format2ToHex
功能描述:数据格式2变成16进制数据
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
函数名称:dayPowerOnAmount
功能描述:记录日累计供电时间
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void dcAnalogStatis(INT16U dcNow)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord; //终端统计记录
   ADC_PARA               adcPara;              //直流模拟量配置参数(AFN04-FN81,FN82,FN83)
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
       printf("当前直流模拟量值=%d,上限=%d,下限=%d\n", dcNow*100, tmpUpLimit, tmpDownLimit);
     }
     
     if (dcNow*100>tmpUpLimit)
     {
       if (debugInfo&PRINT_DATA_BASE)
       {
     	   printf("直流模拟量越上限\n");
     	 }
     	 
     	 terminalStatisRecord.dcOverUp++;
     	 needSaveDc = 1;
     }
     
     if (dcNow*100<tmpDownLimit)
     {
       if (debugInfo&PRINT_DATA_BASE)
       {
     	   printf("直流模拟量越下限\n");
     	 }
     	 terminalStatisRecord.dcOverDown++;
     	 needSaveDc = 1;
     }


     if (terminalStatisRecord.dcMax[0]!=0xee)
     {
       tmpOrigMax = format2ToHex(terminalStatisRecord.dcMax[0] | terminalStatisRecord.dcMax[1]<<8);
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("原直流模拟量最大值=%d\n",tmpOrigMax);
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
           printf("更新直流模拟量最大值及发生时间\n");
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
         printf("置初值(直流模拟量最大值及发生时间)\n");
       }
     }

     if (terminalStatisRecord.dcMin[0]!=0xee)
     {
       tmpOrigMin = format2ToHex(terminalStatisRecord.dcMin[0] | terminalStatisRecord.dcMin[1]<<8);
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("原直流模拟量最小值=%d\n",tmpOrigMin);
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
           printf("更新直流模拟量最小值及发生时间\n");
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
         printf("置初值(直流模拟量最小值及发生时间)\n");
       }
     }
   
     if (needSaveDc==1)
     {
       saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0, sizeof(TERMINAL_STATIS_RECORD));
     }
   }
}

#endif

