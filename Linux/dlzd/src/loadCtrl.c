/***************************************************
Copyright,2008,LongTong co.,LTD,All	Rights Reserved
文件名：localCtrl.c
作者：TianYe
版本：0.9
完成日期：2008年12月
描述：负控任务
函数列表：
修改历史:
  01,08-01-22,Tianye created.
  02,08-12-10,leiyong modify,重写所有控制功能(包括实时控制、本地闭环控制及保电等)
  03,10-04-27,leiyong移植到AT91SAM9260处理器
***************************************************/
#include "convert.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "copyMeter.h"
#include "workWithMeter.h"
#include "hardwareConfig.h"
#include "userInterface.h"
#include "ioChannel.h"
#include "att7022b.h"
#include "AFN0C.h"

#include "loadCtrl.h"

#ifdef LOAD_CTRL_MODULE

INT8U         ctrlReadBuf[512];
INT8U         gateCloseWaitTime=0;             //允许合闸告警音时间
DATE_TIME     nextRound;                       //下一轮次等待时间(如果功率下降到定值以下,则不继续跳闸)
INT8U         ctrlCount=0;

/*
static unsigned char auchCRCHi[]={
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,
	0x01,0xc0,0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,
	0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,
	0x01,0xc0,0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
  0x80,0x41,0x00,0xc1,0x81,0x40
  };

static char auchCRCLo[]={
	0x00,0xc0,0xc1,0x01,0xc3,0x03,0x02,0xc2,0xc6,0x06,
	0x07,0xc7,0x05,0xc5,0xc4,0x04,0xcc,0x0c,0x0d,0xcd,
	0x0f,0xcf,0xce,0x0e,0x0a,0xca,0xcb,0x0b,0xc9,0x09,
	0x08,0xc8,0xd8,0x18,0x19,0xd9,0x1b,0xdb,0xda,0x1a,
	0x1e,0xde,0xdf,0x1f,0xdd,0x1d,0x1c,0xdc,0x14,0xd4,
	0xd5,0x15,0xd7,0x17,0x16,0xd6,0xd2,0x12,0x13,0xd3,
	0x11,0xd1,0xd0,0x10,0xf0,0x30,0x31,0xf1,0x33,0xf3,
	0xf2,0x32,0x36,0xf6,0xf7,0x37,0xf5,0x35,0x34,0xf4,
	0x3c,0xfc,0xfd,0x3d,0xff,0x3f,0x3e,0xfe,0xfa,0x3a,
	0x3b,0xfb,0x39,0xf9,0xf8,0x38,0x28,0xe8,0xe9,0x29,
	0xeb,0x2b,0x2a,0xea,0xee,0x2e,0x2f,0xef,0x2d,0xed,
	0xec,0x2c,0xe4,0x24,0x25,0xe5,0x27,0xe7,0xe6,0x26,
	0x22,0xe2,0xe3,0x23,0xe1,0x21,0x20,0xe0,0xa0,0x60,
	0x61,0xa1,0x63,0xa3,0xa2,0x62,0x66,0xa6,0xa7,0x67,
	0xa5,0x65,0x64,0xa4,0x6c,0xac,0xad,0x6d,0xaf,0x6f,
	0x6e,0xae,0xaa,0x6a,0x6b,0xab,0x69,0xa9,0xa8,0x68,
	0x78,0xb8,0xb9,0x79,0xbb,0x7b,0x7a,0xba,0xbe,0x7e,
	0x7f,0xbf,0x7d,0xbd,0xbc,0x7c,0xb4,0x74,0x75,0xb5,
	0x77,0xb7,0xb6,0x76,0x72,0xb2,0xb3,0x73,0xb1,0x71,
	0x70,0xb0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,
	0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9c,0x5c,
	0x5d,0x9d,0x5f,0x9f,0x9e,0x5e,0x5a,0x9a,0x9b,0x5b,
	0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4b,0x8b,
	0x8a,0x4a,0x4e,0x8e,0x8f,0x4f,0x8d,0x4d,0x4c,0x8c,
	0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
	0x43,0x83,0x41,0x81,0x80,0x40
  };



//CRC查表函数
//puchMsg,要进行CRC校验的消息
//消息中字节数
unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen)
{
	unsigned char uchCRCHi = 0xff;    //CRC高字节初始化
	unsigned char uchCRCLo = 0xff;    //CRC低字节初始化
	unsigned char uIndex;             //CRC循环索引
	while (usDataLen--)
  {
  	uIndex = uchCRCHi^*puchMsg++;  //计算CRC
  	uchCRCHi = uchCRCLo^auchCRCHi[uIndex];
  	uchCRCLo = auchCRCLo[uIndex];
  }
  
  return (uchCRCHi<<8 | uchCRCLo);
}
*/

/*******************************************************
函数名称:threadOfCtrl
功能描述:本地闭环控制任务(电控，功控;)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void threadOfCtrl(void *arg)
{
   INT8U     tmpNumOfZjz;
   INT8U     i,j, pn, tmpDate;
   INT8U     tmpRound, tmpRoundx;
   INT32U    protectSafeKw;                              //保安定值Kw
   INT16U    protectSafeWatt;                            //保安定值Watt
	 INT32U    presentData, alarmLimit, ctrlLimit;         //保存计算用的数据(KW)
	 INT16U    presentWatt, alarmLimitWatt, ctrlLimitWatt; //保存计算用的数据(W)
	 INT8U     tmpPeriodCount;                             //时段索引值   表示第xx时段
	 INT16U    periodCtrlLimit;                            //时段功控定值 表示第xx时段的功控定值
	 DATE_TIME timeLimitStart, timeLimitEnd;
	 INT32U    tmpLimit,tmpData,tmpWatt;
	 INT32U    eventDataKw;  
   INT8U     powerQuantity;
   DATE_TIME tmpTime;
   INT8U     k, onlyHasPulsePn;       //ly,2011-04-25,add
   BOOL      ifExec,bufHasData;
   INT8U     negPresent, negAlarm, negCtrl;
	 INT8U     eventData[13];
   
   //置功控,电控灯
   for (i = 0; i < totalAddGroup.numberOfzjz; i++)
   {
   	  if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN || ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN 
   	  	  || ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN || ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN 
   	  	 )
   	  {
         statusOfGate |= 0x10;    //功控灯亮
   	  }

   	  if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN || ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
   	  {
         statusOfGate |= 0x20;    //电控灯亮
   	  }
   }
   
	 while(1)
	 {
      //0.如果继电器有跳闸动作,5秒后继电器回到常闭状态,ly,2011-08-15,add
      for(i=1;i<CONTROL_OUTPUT+1;i++)
      {
        if (gateJumpTime[i]>0x00)
        {
          gateJumpTime[i]++;
          if (gateJumpTime[i]>4)
          {
          	gateJumpTime[i] = 0;
          	 
          	jumpGate(i, 0);  //继电器回到常闭状态
          }
        }
      }
      
      //1.保电
      //1-1.终端当前状态为保电投入
      if (staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
      {
      	//如果是一直保电
        if (staySupportStatus.ifHold==0xaa)
        {
           alarmInQueue(PAUL_ELECTRICITY,0);
        }
        else
        {
          //当前时间已经超过了保电结束时间，保电结束
          if (compareTwoTime(staySupportStatus.endStaySupport, sysTime))
          {
             staySupportStatus.ifStaySupport = CTRL_RELEASE;
          }
          else
          {
             alarmInQueue(PAUL_ELECTRICITY,0);
          }
        }
      }
      
      //1-2.保电解除
      if (staySupportStatus.ifStaySupport == CTRL_RELEASE)
      {
      	staySupportStatus.ifStaySupport = CTRL_RELEASEED;  //保电已解除
      	ctrlStatus.numOfAlarm = 0;
      	menuInLayer = 1;
  	     
  	    saveParameter(0x05, 25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));  //终端保电投入/解除
      }
      
      //保电状态不执行遥控、电控、功控
      if (staySupportStatus.ifStaySupport!=CTRL_JUMP_IN)
      {
        //2.遥控
        for (i=0; i<CONTROL_OUTPUT; i++)
        {
        	//遥控事件
        	if (remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_JUMP_IN       //本路遥控投入状态
        		  || remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_RELEASEED  //或是已解除状态
        		 )
        	{
          	//记录遥控跳闸事件
          	if (compareTwoTime(remoteEventInfor[i].freezeTime, sysTime) && remoteEventInfor[i].freezeTime.year != 0x00)
          	{
        	    if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	    {
        	    	 printf("threadOfCtrl-遥控:%02d-%02d-%02d %02d:%02d:%02d,记录第%d遥控事件\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
        	    }
        	    
        	    //记录事件入队列
              if ((eventRecordConfig.iEvent[0] & 0x10) || (eventRecordConfig.nEvent[0] & 0x10))
              {
         	      eventData[0] = 0x5;
         	      eventData[1] = 0xd;
         	      
         	      //跳闸时间
         	      eventData[2] = remoteEventInfor[i].remoteTime.second/10<<4 | remoteEventInfor[i].remoteTime.second%10;
         	      eventData[3] = remoteEventInfor[i].remoteTime.minute/10<<4 | remoteEventInfor[i].remoteTime.minute%10;
         	      eventData[4] = remoteEventInfor[i].remoteTime.hour  /10<<4 | remoteEventInfor[i].remoteTime.hour  %10;
         	      eventData[5] = remoteEventInfor[i].remoteTime.day   /10<<4 | remoteEventInfor[i].remoteTime.day   %10;
         	      eventData[6] = remoteEventInfor[i].remoteTime.month /10<<4 | remoteEventInfor[i].remoteTime.month %10;
         	      eventData[7] = remoteEventInfor[i].remoteTime.year  /10<<4 | remoteEventInfor[i].remoteTime.year  %10;
         	      
         	      //跳闸轮次
         	      eventData[8] = 0x01<<i;
         	      
         	      //跳闸时功率(总加功率)
         	      eventData[9] = remoteEventInfor[i].remotePower[0];
         	      eventData[10] = remoteEventInfor[i].remotePower[1];
            	  
            	  //跳闸后两分钟总加功率
            	  //控制轮次1-4分别记录总加组1-4的功率,未配置总加组或无数据记为0xee(无数据)
            		tmpTime = timeHexToBcd(sysTime);
                if (readMeterData(ctrlReadBuf, i+1, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
          	    {   	      
         	         eventData[11] = ctrlReadBuf[GP_WORK_POWER+1];
         	         eventData[12] = ((ctrlReadBuf[GP_WORK_POWER]&0x7)<<5)
                                 | (ctrlReadBuf[GP_WORK_POWER]&0x10)
                                 | (ctrlReadBuf[GP_WORK_POWER+2]&0x0F);
                }
                else
                {
                	 eventData[11] = 0xee;
                	 eventData[12] = 0xee;
                }
         	  
                if (eventRecordConfig.iEvent[0] & 0x10)
                {
         	         writeEvent(eventData, 13, 1, DATA_FROM_GPRS);  //记入重要事件队列
         	      }
                if (eventRecordConfig.nEvent[0] & 0x10)
                {
         	         writeEvent(eventData, 13, 2, DATA_FROM_LOCAL);  //记入一般事件队列
         	      }
                
                if (debugInfo&PRINT_EVENT_DEBUG)
                {
                  printf("realCtrlTask调用主动上报\n");
                }
      
                activeReport3();   //主动上报事件
         	      
         	      eventStatus[0] = eventStatus[0] | 0x10;
              }
            	  
            	remoteEventInfor[i].freezeTime.year = 0xFF;
              
              //ly,09-06-22,add,没有这一句的话,如果已遥控跳闸后每次启动都会记遥控跳闸事件
            	saveParameter(0x05, 3,(INT8U *)&remoteEventInfor,sizeof(REMOTE_EVENT_INFOR)*8);   //遥控现场记录
          	}
          }
        	
        	if (remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_JUMP_IN)   //本路遥控投入状态
        	{
        	  if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	  {
        	    printf("threadOfCtrl-遥控:第%d路投入\n", i+1);
        	  }
        	  
          	//还未进入控制状态
          	if (remoteCtrlConfig[i].ifRemoteCtrl == 0x00)
        	  {
              //已经进入遥控时段
              if (compareTwoTime(remoteCtrlConfig[i].remoteStart, sysTime)
            	   && compareTwoTime(sysTime, remoteCtrlConfig[i].remoteEnd))
            	{
            	  if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            	  {
            	    printf("threadOfCtrl-遥控:%02d-%02d-%02d %02d:%02d:%02d,第%d路跳闸时刻到\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
            	  }

          	    #ifdef REMOTE_CTRL_CONFIRM
            	    if (remoteCtrlConfig[i].ifRemoteConfirm == 0xAA)  //返校已确认
            	    {
            	  #endif //REMOTE_CTRL_CONFIRM
            	      //解除告警  	      
            	      remoteCtrlConfig[i].alarmStart.year = 0xFF;
            	       
                    remoteEventInfor[i].remoteTime.second = sysTime.second;
                    remoteEventInfor[i].remoteTime.minute = sysTime.minute;
                    remoteEventInfor[i].remoteTime.hour   = sysTime.hour;
                    remoteEventInfor[i].remoteTime.day    = sysTime.day;
                    remoteEventInfor[i].remoteTime.month  = sysTime.month;
                    remoteEventInfor[i].remoteTime.year   = sysTime.year;
                  
                    remoteEventInfor[i].freezeTime = nextTime(sysTime, 2, 0);
                      
            			  //控制轮次1-4分别记录总加组1-4的功率,未配置总加组或无数据记为0xee(无数据)
            			  tmpTime = timeHexToBcd(sysTime);
                    if (readMeterData(ctrlReadBuf, i+1, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
                    {
                       remoteEventInfor[i].remotePower[0] = ctrlReadBuf[GP_WORK_POWER+1];
                       remoteEventInfor[i].remotePower[1] = ((ctrlReadBuf[GP_WORK_POWER]&0x7)<<5)
                                                          |(ctrlReadBuf[GP_WORK_POWER]&0x10)
                                                          |ctrlReadBuf[GP_WORK_POWER+2]&0x0F;
                    }
                    else
                    {
                       remoteEventInfor[i].remotePower[0] = 0xee;
                       remoteEventInfor[i].remotePower[1] = 0xee;
                    }
            	       
            	      //输出控制信号
          	        jumpGate(i+1, 1);
          	        
          	        ctrlJumpedStatis(REMOTE_CTRL,1);
          	        
          	        //输出闸状态指示
          	        gateStatus(i+1, GATE_STATUS_ON);
          	              	        
          	        //控制已经执行
          	        remoteCtrlConfig[i].ifRemoteCtrl = 0xAA;
          	        remoteCtrlConfig[i].status = CTRL_JUMPED;  //已跳闸
                    saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
      
                    alarmInQueue(REMOTE_CTRL,0);               //遥控
            	      
          	        //不再检查其他控制条件，继续下一个总加组
      	            continue;
      	            
            	  #ifdef REMOTE_CTRL_CONFIRM
            	    }
          	      else   //返校未确认
          	      {
          	        if (remoteCtrlConfig[i].confirmTimeOut.year != 0xFF
          	      	  && compareTwoTime(remoteCtrlConfig[i].confirmTimeOut, sysTime))
          	        {
          	          //返校超时，清除遥控参数        	      
          	          remoteCtrlConfig[i].confirmTimeOut.year = 0xFF;
                      
                      saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);    	          
          	        }
          	      }
          	    #endif //REMOTE_CTRL_CONFIRM
          	  }
          	  else
          	  {
          	    //已经进入告警时段
          	    if (compareTwoTime(remoteCtrlConfig[i].alarmStart, sysTime)
            	     && compareTwoTime(sysTime, remoteCtrlConfig[i].remoteStart))
            	  {
              	  if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
              	  {
              	    printf("threadOfCtrl-遥控:%02d-%02d-%02d %02d:%02d:%02d,第%d路告警\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
              	  }

            	    //告警输出
            	    alarmInQueue(REMOTE_CTRL,0);             //遥控
            	    
          	      remoteCtrlConfig[i].status = CTRL_ALARM; //告警
            	    
            	    //显示告警信息
          	      
          	      //进入告警时段后收到允许合闸命令
          	      if (compareTwoTime(remoteCtrlConfig[i].remoteEnd, sysTime))
                  {
                  	 alarmInQueue(REMOTE_CTRL,0);                  //遥控
          	         remoteCtrlConfig[i].status = CTRL_CLOSE_GATE; //允许合闸
                      
                     //清除控制参数
                     remoteCtrlConfig[i].ifRemoteCtrl = 0x00;
                     remoteCtrlConfig[i].alarmStart.year = 0xFF;
                     remoteCtrlConfig[i].remoteStart.year = 0xFF;
            	       remoteCtrlConfig[i].remoteEnd.year = 0xFF;
            	        
                     saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
                  }
            	  }
          	  }
          	}
          	else    //已经经入控制状态
          	{
          	  //已经进入解除控制时段
          	  if (compareTwoTime(remoteCtrlConfig[i].remoteEnd, sysTime))
              {
                //允许合闸
              	if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
              	{
              	  printf("threadOfCtrl-遥控:%02d-%02d-%02d %02d:%02d:%02d,第%d路解除控制时间到\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
              	}

                alarmInQueue(REMOTE_CTRL,0);                          //遥控
          	    remoteCtrlConfig[i].status = CTRL_CLOSE_GATE;         //允许合闸
          	    remoteCtrlConfig[i].ifUseRemoteCtrl = CTRL_RELEASEED; //本路遥控已解除
                
                //输出控制信号(清除控制输出)
                jumpGate(i+1, 0);
                
          	    //输出闸状态指示
          	    gateStatus(i+1, GATE_STATUS_OFF);
      	                  
                //清除控制参数
                remoteCtrlConfig[i].ifRemoteCtrl = 0x00;
                remoteCtrlConfig[i].remoteStart.year = 0xFF;
            	  remoteCtrlConfig[i].remoteEnd.year = 0xFF;
                
                saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
            	  
            	  gateCloseWaitTime=GATE_CLOSE_WAIT_TIME;       //允许合闸告警音时间
            	  setBeeper(BEEPER_ON);                         //给出声音示警
       	        alarmLedCtrl(ALARM_LED_ON);                   //指示灯亮
              }
              else   //已跳闸后执行此段
              {
              	//这里还要检查门控状态，若存在擅自合闸，生成事件告警
      
              	//输出合闸状态指示，避免终端复位造成状态混乱
                if (workSecond==1)
                {
                  remoteCtrlConfig[i].status = CTRL_JUMPED; //已跳闸
                  alarmInQueue(REMOTE_CTRL, 0);             //遥控
                              
          	      //输出闸状态指示
          	      gateStatus(i+1, GATE_STATUS_ON);
        	      }
              }
          	}
          }
        }
  	    
  	    //3.电控和功控
   	    for (tmpNumOfZjz = 0; tmpNumOfZjz<totalAddGroup.numberOfzjz; tmpNumOfZjz++)
   	    {
   	    	pn = totalAddGroup.perZjz[tmpNumOfZjz].zjzNo;
   
          //月电控和购电控同一时间只能投入一个
   	    	//------------------- 月 电 控  --------------------
       	  //月电控投入
       	  if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_JUMP_IN)
       	  {
             if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
             {
    	         printf("\n总加组%d月电量控投入\n",pn);
    	       }
  
             //每次只跳闸一个轮次
             if (balanceComplete==TRUE)
      	     {
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //如果还有轮次未跳完,执行继续监控(电控轮次需要投入)
                 if (!((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((electCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	 nextRound = sysTime;
                   }
                   if (compareTwoTime(nextRound, sysTime))   //下一轮次等待WAIT_NEXT_ROUND分钟
                   {
    	                 ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                 ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
    	                  
    	                 //月电量需要设置且不能为0
    	                 if (monthCtrlConfig[pn-1].monthCtrl==0)
    	                 {
    	                  	break;
    	                 }
    	                 
    	                 //monthCtrl(月电量控限值为4字节,最高4位(28~31位)表示数量级,在计算时保留数量
    	                 j = monthCtrlConfig[pn-1].monthCtrl>>28&0xf;
    	                 
    	                 //取出月电量控限值的低28位
    	                 tmpLimit = monthCtrlConfig[pn-1].monthCtrl&0xfffffff;
    	                 
    	                 //月电量控有浮动系数,因此以下几句计算浮动后的限值
    	                 tmpLimit = countAlarmLimit((INT8U *)&tmpLimit, 0x3, ctrlPara.monthEnergCtrlIndex,&presentWatt);
                       presentData = 0;
    	                 tmpLimit = hexToBcd(tmpLimit);
    	                 tmpLimit |= j<<28;      //还原数量级
    	                 
    	                 if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
    	                 {
    	                   printf("月电量控计算!\n");
    	                 }
                       
                       if (calcData(ctrlReadBuf,tmpLimit,&presentData,&alarmLimit,&ctrlLimit,&presentWatt,&alarmLimitWatt,&ctrlLimitWatt,0xa0, 0x03, pn))
                       {
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("月电量控:告警值=%0d\n",alarmLimit);
                            printf("月电量控:当前电量=%0d\n",presentData);
                            printf("月电量控:控制值=%0d\n",ctrlLimit);
                          }
  
                          //超电量告警限值
                          if (presentData > alarmLimit)
                          {
                             //已进入告警状态
                             if (monthCtrlConfig[pn-1].monthAlarm == CTRL_ALARM)
                             {
                               //超电量控制限值
                               if (presentData>ctrlLimit)
                               {
                                 //输出控制信号，清除告警信号
                                 //依据该总加组的电控轮次设定逐个轮次设定
                                 tmpRound = 0;
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);
                                     
                                     statusOfGate |= 0x20;    //电控灯亮
  
                                     ctrlJumpedStatis(MONTH_CTRL,1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     electCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                     
                                     //月电量控跳闸记录
                                     eventDataKw = ctrlReadBuf[GP_MONTH_WORK+3]
                                                 | ctrlReadBuf[GP_MONTH_WORK+4] << 8 
                                                 | ctrlReadBuf[GP_MONTH_WORK+5] << 16 
                                                 | (((ctrlReadBuf[GP_MONTH_WORK]&0x01)<<6)|(ctrlReadBuf[GP_MONTH_WORK]&0x10)|(ctrlReadBuf[GP_MONTH_WORK+6]&0x0f))<<24;
                                 
                                     #ifdef ONCE_JUMP_ONE_ROUND
                                      electCtrlRecord(1, pn, 1<<j, MONTH_CTRL,(INT8U *)&eventDataKw,(INT8U *)&monthCtrlConfig[pn-1].monthCtrl);
                                      break;
                                     #else
                                      tmpRound |= 1<<j;
                                     #endif
                                   }
                                 }
                                 
                                 #ifndef ONCE_JUMP_ONE_ROUND
                                  if (tmpRound)  //如果有轮次跳闸,记录事件
                                  {
                                    electCtrlRecord(1, pn, tmpRound, MONTH_CTRL,(INT8U *)&eventDataKw,(INT8U *)&monthCtrlConfig[pn-1].monthCtrl);
                                  }
                                 #endif
                                 
                                 monthCtrlConfig[pn-1].ifMonthCtrl = 0xAA;
                                 
                                 //控制标志置位，告警标志清除
                                 monthCtrlConfig[pn-1].monthAlarm = CTRL_JUMPED;      //已跳闸
  			                         saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
  			                         saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
  
                                 alarmInQueue(MONTH_CTRL,pn);                         //月电控
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0);     //下一轮次等待WAIT_NEXT_ROUND分钟
                                                                 
                                 //不再检查其他控制条件，继续下一个总加组
                                 break;
                               }
                             }
                             else  //未进入告警状态
                             {
                               //计算告警时限
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //电控轮次已设定且未跳闸
                                  if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  
                                  tmpRound++;
                               }
                               
                               //月电控告警记录
                               tmpRoundx = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                 //电控设定轮次
                                 if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01)
                                 {
                                  	if (j==0)
                                  	{
                                  	  tmpRoundx = 1;
                                  	}
                                  	else
                                  	{
                                  	  tmpRoundx |= 1<<j;
                                  	}
                                 }
                               }                             
                               eventDataKw = ctrlReadBuf[GP_MONTH_WORK+3]
                                           | ctrlReadBuf[GP_MONTH_WORK+4] << 8 
                                           | ctrlReadBuf[GP_MONTH_WORK+5] << 16 
                                           | (((ctrlReadBuf[GP_MONTH_WORK]&0x01)<<6)|(ctrlReadBuf[GP_MONTH_WORK]&0x10)|(ctrlReadBuf[GP_MONTH_WORK+6]&0x0f))<<24;
                               electCtrlRecord(2, pn, tmpRoundx , MONTH_CTRL,(INT8U *)&eventDataKw, (INT8U *)&monthCtrlConfig[pn-1].monthCtrl);
  
                             	 //告警起始时间为当前时间
                             	 monthCtrlConfig[pn-1].mthAlarmTimeOut = sysTime;
                               
                               //如果还没有轮次进入跳闸状态,将所有跳闸状态置为未跳闸,避免其它方式转入月电控造成混乱
                               if (monthCtrlConfig[pn-1].ifMonthCtrl == 0x00)
                               {
                                  electCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               
                               //输出告警信号
                               monthCtrlConfig[pn-1].monthAlarm = CTRL_ALARM;  //告警
                               alarmInQueue(MONTH_CTRL,pn);                    //月电控
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;        //开始告警提示音时间
       	                       setBeeper(BEEPER_ON);                           //给出声音示警
  	                           alarmLedCtrl(ALARM_LED_ON);                     //指示灯亮                                
  
  			                       saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
                             }
                          }
                       }
                   }
                    
                   break;  //每次只跳闸一个轮次,所以如果比较了一个轮次就退出
                 }
               }
             }
  
             //过月自动解除
             timeLimitEnd = nextTime(sysTime,0,1);
             if (timeLimitEnd.month!= sysTime.month)
             {
              	ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_AUTO_RELEASE;
             }
              
           	 //复位以后的显示处理
           	 if (workSecond==1)
           	 {
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,如果遥控已经跳闸就不再处理初始状态
                 {
                   if ((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
       	             gateStatus(j+1, GATE_STATUS_ON);
                     alarmInQueue(MONTH_CTRL,pn);     //月电量控
                     statusOfGate |= 0x20;            //电控灯亮
       	           }
       	           else    //ly,2011-03-31,添加的else
       	           {
       	             gateStatus(j+1, GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	 }
       	  }
       	  
       	  //月电量控解除
          if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_AUTO_RELEASE)
          {
             ctrlRelease(pn,MONTH_CTRL,monthCtrlConfig[pn-1].ifMonthCtrl);
          }
  
       	  //-------------------  购 电 控 --------------------
   	    	//购电控优先级低于月电控，只有未投入月电控时才进行购电控处理
   	    	//目前没有考虑月电量控和购电控的优先级 LY 2008.12.17
   	    	//购电控投入
   	    	if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_JUMP_IN)
   	    	{
             if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
             {
    	         printf("\n总加组%d购电控投入\n",pn);
    	       }
    	       
             //查看本总加组是否只有脉冲测量点
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
    	       
    	       ifExec = FALSE;
    	       if (onlyHasPulsePn==0xaa)
    	       {
    	       	 ctrlCount++;
    	       	 if (ctrlCount>4)
    	       	 {
    	       	   ctrlCount = 0;
    	       	   tmpTime = sysTime;
    	       	   ifExec = computeInTimeLeftPower(pn, tmpTime, ctrlReadBuf, 1);
    	       	 }
    	       }
    	       else
    	       {
    	       	 ifExec = balanceComplete;
    	       }
  
             //每次只跳闸一个轮次
             if (ifExec==TRUE)
      	     {
               ctrlLimit = alarmLimit = presentData = 0x00000000;
               ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
               
               tmpTime = sysTime;
               if (readMeterData(ctrlReadBuf, pn, LEFT_POWER, 0x0, &tmpTime, 0)==TRUE)
               {
                 if (ctrlReadBuf[0] != 0xFF || ctrlReadBuf[0] != 0xEE)
                 {
                   //当前剩余电量
                   negPresent = 0;
                   negAlarm = 0;
                   negCtrl = 0;
                   
                   presentData = ctrlReadBuf[0]
                          | ctrlReadBuf[1] << 8
                          | ctrlReadBuf[2] << 16
                          | ctrlReadBuf[3] << 24;
                   if (ctrlReadBuf[3]&0x10)
                   {
                   	 negPresent = 1;
                   }
  
                   if (chargeCtrlConfig[pn-1].alarmLimit&0x10000000)
                   {
                   	 negAlarm = 1;
                   }
  
                   if (chargeCtrlConfig[pn-1].cutDownLimit&0x10000000)
                   {
                   	 negCtrl = 1;
                   }
            
                   presentData = countAlarmLimit((INT8U* )&presentData, 0x03, 0,&presentWatt);
                   alarmLimit = countAlarmLimit((INT8U* )&chargeCtrlConfig[pn-1].alarmLimit, 0x03, 0,&alarmLimitWatt);
                   ctrlLimit = countAlarmLimit((INT8U* )&chargeCtrlConfig[pn-1].cutDownLimit, 0x03, 0,&ctrlLimitWatt);
                   
                   if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                   {
                     if (negAlarm==1)
                     {
                       printf("购电控:告警值=-%0d\n",alarmLimit);
                     }
                     else
                     {
                       printf("购电控:告警值=%0d\n",alarmLimit);
                     }
                     
                     if (negPresent==1)
                     {
                       printf("购电控:当前剩余电量=-%0d\n",presentData);
                     }
                     else
                     {
                       printf("购电控:当前剩余电量=%0d\n",presentData);
                     }
                     
                     if (negCtrl==1)
                     {
                       printf("购电控:控制值=-%0d\n",ctrlLimit);
                     }
                     else
                     {
                       printf("购电控:控制值=%0d\n",ctrlLimit);
                     }
                   }
  
                   for(j=0;j<CONTROL_OUTPUT;j++)
                   {
                     //如果还有轮次未跳完,执行继续监控
                     if (!((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((electCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                     {
                       if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                       {
                       	 nextRound = sysTime;
                       }
                       if (compareTwoTime(nextRound,sysTime))   //下一轮次等待WAIT_NEXT_ROUND分钟
                       {
                         //超剩余电量告警限值
                         if ((negPresent==0 && negAlarm==0 && presentData <= alarmLimit)   //如果当前值和告警值都是正数
                         	   || (negPresent==1 && negAlarm==0)                             //如果当前值是负数而告警值是正数
                         	   || (negPresent==1 && negAlarm==1 && presentData >= alarmLimit)//如果当前值和告警都是负数
                         	  )
                         {
                            //已进入告警状态
                            if (chargeCtrlConfig[pn-1].chargeAlarm == CTRL_ALARM)
                            {
                              //超剩余电量控制限值
                              //if (presentData<=ctrlLimit)
                              if ((negPresent==0 && negCtrl==0 && presentData <= ctrlLimit)    //如果当前值和跳闸值都是正数
                         	        || (negPresent==1 && negCtrl==0)                             //如果当前值是负数而跳闸值是正数
                         	         || (negPresent==1 && negCtrl==1 && presentData >= ctrlLimit)//如果当前值和跳闸都是负数
                         	       )
                              {
                                //输出控制信号，清除告警信号
                                //依据该总加组的电控轮次设定逐个轮次设定
                                tmpRound = 0;
                                for (j = 0; j < CONTROL_OUTPUT; j++)
                                {
                                  if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                    jumpGate(j+1, 1);        //继电器跳到常开                                    
                                    statusOfGate |= 0x20;    //电控灯亮
                                    
                                    ctrlJumpedStatis(CHARGE_CTRL,1);
                                    gateStatus(j+1,GATE_STATUS_ON);
                                    electCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                    
                                    //购电控跳闸记录
                                    eventDataKw = ctrlReadBuf[0]
                                                | ctrlReadBuf[1] << 8
                                                | ctrlReadBuf[2] << 16
                                                | ctrlReadBuf[3] << 24;
                                    #ifdef ONCE_JUMP_ONE_ROUND
                                     electCtrlRecord(1, pn, 1<<j, CHARGE_CTRL,(INT8U *)&eventDataKw,(INT8U *)&chargeCtrlConfig[pn-1].cutDownLimit);
                                    
                                     break;
                                    #else
                                     tmpRound |= 1<<j;
                                    #endif
                                  }
                                }
                                
                                #ifndef ONCE_JUMP_ONE_ROUND
                                 if (tmpRound)
                                 {
                                   electCtrlRecord(1, pn, tmpRound, CHARGE_CTRL,(INT8U *)&eventDataKw,(INT8U *)&chargeCtrlConfig[pn-1].cutDownLimit);
                                 }
                                #endif
                                
                                chargeCtrlConfig[pn-1].ifChargeCtrl = 0xAA;
                                
                                //控制标志置位，告警标志清除
                                chargeCtrlConfig[pn-1].chargeAlarm = CTRL_JUMPED;   //已跳闸
  
  			                        saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
  			                        saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
                            
                                alarmInQueue(CHARGE_CTRL,pn);                       //购电控
                                
                                nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0);    //下一轮次等待WAIT_NEXT_ROUND分钟
  
                                //不再检查其他控制条件
                                break;
                              }
                            }
                            else  //未进入告警状态
                            {
                            	//告警起始时间为当前时间
                            	chargeCtrlConfig[pn-1].alarmTimeOut = sysTime;
                              
                              //如果还没有轮次进入跳闸状态,将所有跳闸状态置为未跳闸,避免其它方式转入购电控造成混乱
                              if (chargeCtrlConfig[pn-1].ifChargeCtrl == 0x00)
                              {
                                 electCtrlRoundFlag[pn-1].ifJumped = 0x0;
                              }
  
                              //月电控告警记录
                              tmpRoundx = 0;
                              for(j=0;j<CONTROL_OUTPUT;j++)
                              {
                                 //电控设定轮次
                                 if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01)
                                 {
                                  	if (j==0)
                                  	{
                                  	  tmpRoundx = 1;
                                  	}
                                  	else
                                  	{
                                  	  tmpRoundx |= 1<<j;
                                  	}
                                 }
                              }
                              
                              eventDataKw = ctrlReadBuf[(pn-1)*12+0]
                                          | ctrlReadBuf[(pn-1)*12+1] << 8
                                          | ctrlReadBuf[(pn-1)*12+2] << 16
                                          | ctrlReadBuf[(pn-1)*12+3] << 24;
                              electCtrlRecord(2, pn, tmpRoundx , CHARGE_CTRL,(INT8U *)&eventDataKw, (INT8U *)&chargeCtrlConfig[pn-1].cutDownLimit);
  
                              //输出告警信号
                              chargeCtrlConfig[pn-1].chargeAlarm = CTRL_ALARM; //告警
                              alarmInQueue(CHARGE_CTRL,pn);                    //购电控
                              gateCloseWaitTime=START_ALARM_WAIT_TIME;         //开始告警提示音时间
      	                      setBeeper(BEEPER_ON);                            //给出声音示警
                              alarmLedCtrl(ALARM_LED_ON);                      //指示灯亮
  			                      saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
                            }
                         }
                       }
                       break;  //每次只跳闸一个轮次,所以如果比较了一个轮次就退出
                     }
                   }
                   
                   //如果剩余电量大于跳闸电量且有轮次已跳闸,自动解除控制
                   if (((negPresent==0 && negCtrl==0 && presentData > ctrlLimit)    //如果当前值和跳闸值都是正数
               	       || (negPresent==0 && negCtrl==1)                             //如果当前值是正数而跳闸值是负数
               	        || (negPresent==1 && negCtrl==1 && presentData < ctrlLimit))//如果当前值和跳闸都是负数
               	       && chargeCtrlConfig[pn-1].chargeAlarm == CTRL_JUMPED
               	      )
                   {
                   	 ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_AUTO_RELEASE;
                   }
                 }
               }
             }
  
           	 //复位以后的显示处理(只处理投入状态已跳闸的状态)
           	 if (workSecond==1)
           	 {
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,如果遥控已经跳闸就不再处理初始状态
                 {
                   if ((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
       	             gateStatus(j+1,GATE_STATUS_ON);
       	             statusOfGate |= 0x20;                  //电控灯亮
                     alarmInQueue(CHARGE_CTRL,pn);          //购电控
       	           }
       	           else    //ly,2011-03-31,添加的else
       	           {
       	             gateStatus(j+1,GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	 }
       	  }
   	      
       	  //购电控解除
          if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_AUTO_RELEASE)
          {
            ctrlRelease(pn,CHARGE_CTRL,chargeCtrlConfig[pn-1].ifChargeCtrl);
          }
  
          //------------------- 催费告警 -------------------
          //催费告警投入
          if (reminderFee==CTRL_JUMP_IN)
          {
              if (chargeAlarm[sysTime.hour/8]>>(sysTime.hour%8) & 0x1)
              {
        	       setBeeper(BEEPER_ON);                     //蜂鸣器
        	       alarmLedCtrl(ALARM_LED_ON);               //指示灯亮
                 gateCloseWaitTime=GATE_CLOSE_WAIT_TIME;   //允许合闸告警音时间
                 alarmInQueue(REMINDER_FEE,0);
        	    }
        	    else
        	    {
        	       setBeeper(BEEPER_OFF);                    //蜂鸣器
        	       alarmLedCtrl(ALARM_LED_OFF);              //指示灯亮
                 gateCloseWaitTime=0;
        	    }
          }
  
          //催费告警解除
          if (reminderFee==CTRL_RELEASE)
          {
        	   setBeeper(BEEPER_OFF);                    //蜂鸣器
        	   alarmLedCtrl(ALARM_LED_OFF);              //指示灯亮
             gateCloseWaitTime=0;
             reminderFee = CTRL_RELEASEED;
                           
             for(i=0;i<ctrlStatus.numOfAlarm;i++)
             {
                if (ctrlStatus.aQueue[i]==REMINDER_FEE)
                {
             	    for(j=i;j<ctrlStatus.numOfAlarm;j++)
             	    {
             	       ctrlStatus.aQueue[j] = ctrlStatus.aQueue[j+1];
             	       ctrlStatus.allPermitClose[j]=ctrlStatus.allPermitClose[j+1];
             	    }
             	    ctrlStatus.numOfAlarm--;
             	    if (ctrlStatus.numOfAlarm==0)
             	    {
             	    	menuInLayer=1;           	    	 
             	    }
             	    break;
                }
             }
          }
                 	  
       	  powerCtrlRecord();     //判断是否记录功控事件记录
       	  
       	  //------------------- 当 前 功 率 下 浮 控 --------------------
       	  //当前功率下浮控投入
       	  if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
       	  {
             if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
             {
    	         printf("\n总加组%d当前功率下浮控投入\n", pn);
    	       }
   	      	 
   	      	 //定值已确定
   	      	 if (powerDownCtrl[pn-1].freezeTime.year==0xff)
   	      	 {
               //每次只跳闸一个轮次
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //如果还有轮次未跳完,执行继续监控
                 if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	  nextRound = sysTime;
                   }
                   if (compareTwoTime(nextRound,sysTime))   //下一轮次等待WAIT_NEXT_ROUND分钟
                   {
    	                 ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                 ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
  
    	                 if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
    	                 {
    	                   printf("功率下浮控计算\n");
    	                 }
  
                       if (calcData(ctrlReadBuf,0,&presentData,NULL,NULL,&presentWatt,NULL,NULL,0, 0x02, pn))
                       {
                          //保安定值
                          protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
  
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("功率下浮控:保安定值=%0d\n",protectSafeKw);
                            printf("功率下浮控:控制值=%0d\n",powerDownCtrl[pn-1].powerDownLimit);
                            printf("功率下浮控:当前功率=%0d\n",presentData);
                          }
  
                          //控制值与保安定值比较,如果控制值小于保安定值,则控制值=保安定值
                          if (powerDownCtrl[pn-1].powerDownLimit<protectSafeKw 
                          	|| (powerDownCtrl[pn-1].powerDownLimit==protectSafeKw && powerDownCtrl[pn-1].powerLimitWatt<protectSafeWatt))
                          {
                             ctrlLimit = protectSafeKw;
                             ctrlLimitWatt = protectSafeWatt;
                          }
                          else
                          {
                             ctrlLimit = powerDownCtrl[pn-1].powerDownLimit;
                             ctrlLimitWatt = powerDownCtrl[pn-1].powerLimitWatt;
                          }
                          
                          //超功率告警限值
                          if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                          {
                             //已进入告警状态
                             if (powerDownCtrl[pn-1].pwrDownAlarm == CTRL_ALARM)
                             {
                               //告警时间到期,跳闸一个轮次
                               if (compareTwoTime(powerDownCtrl[pn-1].alarmEndTime,sysTime))
                               {
                                 //输出控制信号，清除告警信号
                                 //依据该总加组的功控轮次设定逐个轮次设定
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);

                                     statusOfGate |= 0x10;    //功控灯亮
                                     
                                     ctrlJumpedStatis(POWER_CTRL,1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
  
                                     //记录功控跳闸现场信息
                                     tmpData = powerDownCtrl[pn-1].powerDownLimit;
                                     tmpWatt = powerDownCtrl[pn-1].powerLimitWatt*10;
                                     powerQuantity = dataFormat(&tmpData, &tmpWatt, 2);
                                     tmpData = hexToBcd(tmpData);
  
                                     tmpLimit = tmpData&0xFF;
                                     tmpLimit |= ((tmpData>>8&0xFF)&0xF)<<8;
                                     tmpLimit |= (powerQuantity&0x10)<<8;
                                     tmpLimit |= (powerQuantity&0x07)<<13;
                  	                  
                                     powerCtrlInQueue(pn,POWER_CTRL,j,(INT8U *)&tmpLimit);
                                     break;
                                   }
                                 }
                                 
                                 powerDownCtrl[pn-1].ifPwrDownCtrl = 0xAA;
                                 
                                 //控制标志置位，告警标志清除
                                 powerDownCtrl[pn-1].alarmEndTime.year = 0xFF;
                                 powerDownCtrl[pn-1].pwrDownAlarm = CTRL_JUMPED;      //已跳闸
  	                             
  	                             saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //当前功率下浮功控
  			                         saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
                             
                                 alarmInQueue(POWER_CTRL, pn);                        //当前功率下浮控
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0);     //下一轮次等待WAIT_NEXT_ROUND分钟
                             
                                 //不再检查其他控制条件，继续下一个总加组
                                 break;
                               }
                             }
                             else  //未进入告警状态
                             {
                               //计算告警时限
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //功控轮次已设定且未跳闸
                                  if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  tmpRound++;
                               }
                             	 if (powerCtrlAlarmTime[j].def==0xaa)
                             	 {
                             	   powerDownCtrl[pn-1].alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                             	 }
                             	 else     //如果没有设置该轮次的功控告警时间,默认为1分钟
                             	 {
                             	    powerDownCtrl[pn-1].alarmEndTime = nextTime(sysTime, 1, 0);
                             	 }
                               
                               //如果还没有轮次进入跳闸状态,将所有跳闸状态置为未跳闸,避免其它方式转入当前功率下浮控造成混乱
                               if (powerDownCtrl[pn-1].ifPwrDownCtrl == 0x00)
                               {
                                  powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               
                               //输出告警信号
                               powerDownCtrl[pn-1].pwrDownAlarm = CTRL_ALARM;  //告警
                               alarmInQueue(POWER_CTRL,pn);                    //当前功率下浮控
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;        //开始告警提示音时间
       	                       setBeeper(BEEPER_ON);                           //给出声音示警
  	                           alarmLedCtrl(ALARM_LED_ON);                     //指示灯亮
  	                           saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //当前功率下浮功控
                             }
                          }
                       }
                    }
                    
                    break;  //每次只跳闸一个轮次,所以如果比较了一个轮次就退出
                  }
                }
   	      	 }
   	      	 else  //还未计算定值(powerDownCtrl[pn-1].freezeTime.year不为0xff为未计算定值,计算后置为0xff)
   	      	 {
   	      	   if (compareTwoTime(powerDownCtrl[pn-1].freezeTime,sysTime) && powerDownCtrl[pn-1].freezeTime.year!=0x00)
   	      	   {
   	      	 	    if (calcData(ctrlReadBuf,0,&powerDownCtrl[pn-1].powerDownLimit,&powerDownCtrl[pn-1].powerDownLimit,NULL,&presentWatt,&powerDownCtrl[pn-1].powerLimitWatt,NULL,powerDownCtrl[pn-1].floatFactor, 0x02, pn))
                  {
   	      	 	       powerDownCtrl[pn-1].freezeTime.year = 0xff;
   	      	 	    }
   	      	   }
   	      	 }
  
             //过0点自动解除
             timeLimitEnd = nextTime(sysTime,0,1);
             if (timeLimitEnd.day!= sysTime.day)
             {             	  
              	ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_AUTO_RELEASE;
             }
              
           	 //复位以后的显示处理(只处理投入状态已跳闸的状态)
           	 if (workSecond==1)
           	 {
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,如果遥控已经跳闸就不再处理初始状态
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
       	             gateStatus(j+1,GATE_STATUS_ON);
                     alarmInQueue(POWER_CTRL,pn);      //当前功率下浮控
       	             statusOfGate |= 0x10;             //电控灯亮
       	           }
       	           else    //ly,2011-03-31,添加的else
       	           {
       	             gateStatus(j+1,GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	 }
       	  }
       	  
       	  //当前功率下浮控解除
          if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_AUTO_RELEASE)
          {
             ctrlRelease(pn,POWER_CTRL,powerDownCtrl[pn-1].ifPwrDownCtrl);
             continue;
          }
  
       	  //----------------------- 报  停  控 --------------------------
       	  //体现功控优先级,若高优先级控制投入则低优先级控制不执行
       	  if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
       	  {
       	    continue;
       	  }
       	  
       	  //报停控投入
       	  if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_JUMP_IN)
       	  {
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
    	        printf("\n总加组%d营业报停控投入\n",pn);
     	    	  printf("报停起始:%02d-%02d-%02d\n",obsCtrlConfig[pn-1].obsStartYear,obsCtrlConfig[pn-1].obsStartMonth,obsCtrlConfig[pn-1].obsStartDay);
     	    	  printf("报停结束:%02d-%02d-%02d\n",obsCtrlConfig[pn-1].obsEndYear,obsCtrlConfig[pn-1].obsEndMonth,obsCtrlConfig[pn-1].obsEndDay);
    	      }
     	    	
     	    	//当前时间处于报停控时段(时段判断标准:结束时间是结束日的24:00而不是0:00)
   	      	if  (compareTwoDate(sysTime,obsCtrlConfig[pn-1].obsStartYear,obsCtrlConfig[pn-1].obsStartMonth,obsCtrlConfig[pn-1].obsStartDay,1)
   	      		&& compareTwoDate(sysTime,obsCtrlConfig[pn-1].obsEndYear,obsCtrlConfig[pn-1].obsEndMonth,obsCtrlConfig[pn-1].obsEndDay,2))
     	      {
               //每次只跳闸一个轮次
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //如果还有轮次未跳完,执行继续监控
                 if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	  nextRound = sysTime;
                   }
                   if (compareTwoTime(nextRound,sysTime))   //下一轮次等待WAIT_NEXT_ROUND分钟
                   {
    	                 ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                 ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
                       
                       if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                       {
    	                   printf("营业报停控计算\n");
    	                 }
    	                 
                       if (calcData(ctrlReadBuf,obsCtrlConfig[pn-1].obsLimit,&presentData,NULL,&ctrlLimit,&presentWatt,NULL,&ctrlLimitWatt,ctrlPara.pCtrlIndex, 0x02, pn))
                       {
                          //保安定值
                          protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
                          
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("营业报停控:保安定值=%0d\n",protectSafeKw);
                            printf("营业报停控:控制值=%0d\n",ctrlLimit);
                            printf("营业报停控:当前功率=%0d\n",presentData);
                          }
  
                          //控制值与保安定值比较,如果控制值小于保安定值,则控制值=保安定值
                          if (ctrlLimit<protectSafeKw || (ctrlLimit==protectSafeKw && ctrlLimitWatt<protectSafeWatt))
                          {
                            	ctrlLimit = protectSafeKw;
                            	ctrlLimitWatt = protectSafeWatt;
                          }
  
                          //超功率告警限值
                          if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                          {
                             //已进入告警状态
                             if (obsCtrlConfig[pn-1].obsAlarm == CTRL_ALARM)
                             {
                               //告警时间到期,跳闸一个轮次
                               if (compareTwoTime(obsCtrlConfig[pn-1].alarmEndTime,sysTime))
                               {
                                 //输出控制信号,清除告警信号
                                 //依据该总加组的电控轮次设定逐个轮次设定
                                 tmpRound = 0;
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);

                                     statusOfGate |= 0x10;    //功控灯亮

                                     ctrlJumpedStatis(OBS_CTRL,1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                     
                                     //#ifdef ONCE_JUMP_ONE_ROUND
                                      //记录功控跳闸现场信息
                                      powerCtrlInQueue(pn,OBS_CTRL,j,(INT8U *)&obsCtrlConfig[pn-1].obsLimit);
                                     
                                      break;
                                     //#else
                                     // tmpRound |= 1<<j;
                                     //#endif
                                   }
                                 }
                                     
                                 //#ifndef ONCE_JUMP_ONE_ROUND
                                   //记录功控跳闸现场信息
                                 //  powerCtrlInQueue(pn,OBS_CTRL,tmpRound,(INT8U *)&obsCtrlConfig[pn-1].obsLimit);
                                 //#endif
                                                                
                                 obsCtrlConfig[pn-1].ifObsCtrl = 0xAA;
                                 
                                 //控制标志置位，告警标志清除
                                 obsCtrlConfig[pn-1].alarmEndTime.year = 0xFF;
                                 obsCtrlConfig[pn-1].obsAlarm = CTRL_JUMPED;      //已跳闸
                                 
                                 saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
  			                         saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
                                 
                                 alarmInQueue(OBS_CTRL,pn);                       //报停控
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0); //下一轮次等待WAIT_NEXT_ROUND分钟
                             
                                 //不再检查其他控制条件，继续下一个总加组
                                 break;
                               }
                             }
                             else  //未进入告警状态
                             {
                               //计算告警时限
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //功控轮次已设定且未跳闸
                                  if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  tmpRound++;
                               }
                             	if (powerCtrlAlarmTime[j].def==0xaa)
                             	{
                             	   obsCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                             	}
                             	else     //如果没有设置该轮次的功控告警时间,默认为1分钟
                             	{
                             	   obsCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, 1, 0);
                             	}
                               
                               //如果还没有轮次进入跳闸状态,将所有跳闸状态置为未跳闸,避免其它方式转入报停控造成混乱
                               if (obsCtrlConfig[pn-1].ifObsCtrl == 0x00)
                               {
                                  powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               
                               //输出告警信号
                               obsCtrlConfig[pn-1].obsAlarm = CTRL_ALARM;  //告警
                               alarmInQueue(OBS_CTRL,pn);                  //报停控
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;    //开始告警提示音时间
       	                       setBeeper(BEEPER_ON);                       //给出声音示警
  	                           alarmLedCtrl(ALARM_LED_ON);                 //指示灯亮
                               
                               saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
                             }
                          }
                       }
                   }
                    
                   break;  //每次只跳闸一个轮次,所以如果比较了一个轮次就退出
                 }
               }
     	      }
     	      else  //当前时段已经离开报停控时段
     	      {
     	         //如果已进入报停控控制状态(即已有轮次跳闸)状态
     	         if (obsCtrlConfig[pn-1].ifObsCtrl == 0xAA)
     	         {
     	            ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_AUTO_RELEASE;   //置为控制自动解除
        	        
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("离开报停控时段,自动合闸\n");
        	        }
     	         }
     	      }
           	
           	//复位以后的显示处理(只处理投入状态已跳闸的状态)
           	if (workSecond==1)
           	{
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,如果遥控已经跳闸就不再处理初始状态
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
                     statusOfGate |= 0x10;              //功控灯亮
       	             gateStatus(j+1,GATE_STATUS_ON);
                     alarmInQueue(OBS_CTRL,pn);         //报停控
       	           }
       	           else    //ly,2011-03-31,添加的else
       	           {
       	             gateStatus(j+1,GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	}
          }
  
          //报停控解除
          if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_RELEASE || ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_AUTO_RELEASE)
          {
             ctrlRelease(pn,OBS_CTRL,obsCtrlConfig[pn-1].ifObsCtrl);
             continue;
          }
  
   	      //----------------------- 厂  休  控 ---------------------------
   	      //体现功控优先级,若高优先级控制投入则低优先级控制不执行
   	      if (ctrlRunStatus[pn-1].ifUsePwrCtrl == 0xAA
   	      	  || ctrlRunStatus[pn-1].ifUseObsCtrl == 0xAA)
       	  {
       	     continue;
       	  }
   	      
       	  //厂休控投入
       	  if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_JUMP_IN)
       	  {
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
    	        printf("\n总加组%d厂休控投入\n",pn);
    	      }
             
            //厂休控每周限电日
            tmpDate = dayWeek(2000+sysTime.year, sysTime.month, sysTime.day);
            if (tmpDate == 0)
            {
              tmpDate = 7;
            }
            tmpDate = (wkdCtrlConfig[pn-1].wkdDate>>tmpDate)&0x1;
  
            if (tmpDate)
            {
       	      timeLimitStart = sysTime;
       	      timeLimitEnd = sysTime;
       	      
         	    //控制起始时限(限电起始时间是数据格式19,所以要将BCD格式的时间格式转换成16进制的时间格式)
         	    timeLimitStart.minute = (wkdCtrlConfig[pn-1].wkdStartMin&0xf)+(wkdCtrlConfig[pn-1].wkdStartMin>>4&0xf)*10;
         	    timeLimitStart.hour = (wkdCtrlConfig[pn-1].wkdStartHour&0xf)+(wkdCtrlConfig[pn-1].wkdStartHour>>4&0xf)*10;
         	    timeLimitStart.second = 0;
  
       	      //控制结束时限，并保证限电时段没有跨零点
       	      timeLimitEnd = nextTime(timeLimitStart, wkdCtrlConfig[pn-1].wkdTime*30, 0);
       	      if (timeLimitEnd.day != timeLimitStart.day)
       	      {
       	         timeLimitEnd = timeLimitStart;
       	         timeLimitEnd.hour = 23;
       	         timeLimitEnd.minute = 59;
       	         timeLimitEnd.second = 59;
       	      }
       	      
       	      if (compareTwoTime(timeLimitStart, sysTime) && compareTwoTime(sysTime, timeLimitEnd))
         	    {
                  //每次只跳闸一个轮次
   	             for(j=0;j<CONTROL_OUTPUT;j++)
   	             {
   	               //如果还有轮次未跳完,执行继续监控
   	               if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
   	               {
   	                 if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
   	                 {
   	                 	  nextRound = sysTime;
   	                 }
  
   	                 if (compareTwoTime(nextRound,sysTime))   //下一轮次等待WAIT_NEXT_ROUND分钟
   	                 {
                         ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                   ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
                         
                         if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                         {
    	                    printf("厂休控计算\n");
    	                   }
    	                   
                         if (calcData(ctrlReadBuf,wkdCtrlConfig[pn-1].wkdLimit,&presentData,NULL,&ctrlLimit,&presentWatt,NULL,&ctrlLimitWatt,ctrlPara.pCtrlIndex, 0x02, pn))
                         {
                            //保安定值
                            protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
                            
                            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                            {
                              printf("厂休控:保安定值=%0d\n",protectSafeKw);
                              printf("厂休控:控制值=%0d\n",ctrlLimit);
                              printf("厂休控:当前功率=%0d\n",presentData);
                            }
                            
                            //控制值与保安定值比较,如果控制值小于保安定值,则控制值=保安定值
                            if (ctrlLimit<protectSafeKw || (ctrlLimit==protectSafeKw && ctrlLimitWatt<protectSafeWatt))
                            {
                            	  ctrlLimit = protectSafeKw;
                            	  ctrlLimitWatt = protectSafeWatt;
                            }
                                                       
                            //超功率告警限值
                            if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                            {
                               //已进入告警状态
                               if (wkdCtrlConfig[pn-1].wkdAlarm == CTRL_ALARM)
                               {
                                 //告警时间到期,跳闸一个轮次
                                 if (compareTwoTime(wkdCtrlConfig[pn-1].alarmEndTime,sysTime))
                                 {
                                   //输出控制信号，清除告警信号
                                   //依据该总加组的电控轮次设定逐个轮次设定
                                   tmpRound = 0;
                                   for (j = 0; j < CONTROL_OUTPUT; j++)
                                   {
                                     if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                     {
                                       jumpGate(j+1, 1);
                                       
                                       statusOfGate |= 0x10;    //功控灯亮

                                       ctrlJumpedStatis(WEEKEND_CTRL,1);
                                       
                                       gateStatus(j+1,GATE_STATUS_ON);
                                       powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                       
                                       //#ifdef ONCE_JUMP_ONE_ROUND
                                        //记录功控跳闸现场信息
                                        powerCtrlInQueue(pn,WEEKEND_CTRL,j,(INT8U *)&wkdCtrlConfig[pn-1].wkdLimit);
                                       
                                        break;
                                       //#else
                                       // tmpRound |= 1<<j;
                                       //#endif                                     
                                     }
                                   }
                                   
                                   //#ifndef ONCE_JUMP_ONE_ROUND
                                     //记录功控跳闸现场信息
                                   //  powerCtrlInQueue(pn,WEEKEND_CTRL,tmpRound,(INT8U *)&wkdCtrlConfig[pn-1].wkdLimit);                                   
                                   //#endif
  
                                   wkdCtrlConfig[pn-1].ifWkdCtrl = 0xAA;
                                   
                                   //控制标志置位，告警标志清除
                                   wkdCtrlConfig[pn-1].alarmEndTime.year = 0xFF;
                                   wkdCtrlConfig[pn-1].wkdAlarm = CTRL_JUMPED;      //已跳闸
  
                                   saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
  			                           saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
  
                                   alarmInQueue(WEEKEND_CTRL,pn);                   //厂休控
                                   
                                   nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0); //下一轮次等待WAIT_NEXT_ROUND分钟
                               
                                   //不再检查其他控制条件，继续下一个总加组
                                   break;
                                 }
                               }
                               else  //未进入告警状态
                               {
                                 //计算告警时限
                                 tmpRound = 0;
                                 for(j=0;j<CONTROL_OUTPUT;j++)
                                 {
                                    //功控轮次已设定且未跳闸
                                    if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                    {
                                    	 break;
                                    }
                                    tmpRound++;
                                 }
                                 
                               	 if (powerCtrlAlarmTime[j].def==0xaa)
                               	 {
                               	   wkdCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                               	 }
                               	 else     //如果没有设置该轮次的功控告警时间,默认为1分钟
                               	 {
                               	   wkdCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, 1, 0);
                               	 }
                               	 gateCloseWaitTime=START_ALARM_WAIT_TIME;           //开始告警提示音时间
       	                         setBeeper(BEEPER_ON);                              //给出声音示警
  	                             alarmLedCtrl(ALARM_LED_ON);                        //指示灯亮
                                 
                                 //如果还没有轮次进入跳闸状态,将所有跳闸状态置为未跳闸,避免其它方式转入厂休控造成混乱
                                 if (wkdCtrlConfig[pn-1].ifWkdCtrl == 0x00)
                                 {
                                    powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                                 }
                                 //输出告警信号
                                 wkdCtrlConfig[pn-1].wkdAlarm = CTRL_ALARM;  //告警
                                 alarmInQueue(WEEKEND_CTRL,pn);              //厂休控
                                 
                                 saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
                               }
                            }
                         }
                      }
                      
                      break;  //每次只跳闸一个轮次,所以如果比较了一个轮次就退出
                    }
                  }
       	      }
       	      else  //当前时段已经离开厂休控时段
       	      {
       	         //如果已进入厂休控控制状态(即已有轮次跳闸)状态
       	         if (wkdCtrlConfig[pn-1].ifWkdCtrl == 0xAA)
       	         {
       	           ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_AUTO_RELEASE;   //置为控制自动解除
       	         }
       	      }
       	    }
       	    else  //当前时段已经离开厂休控时段
       	    {
       	       //如果已进入厂休控控制状态(即已有轮次跳闸)状态
       	       if (wkdCtrlConfig[pn-1].ifWkdCtrl == 0xAA)
       	       {
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("离开控制时段,自动合闸\n");
        	        }
       	          
       	          ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_AUTO_RELEASE;   //置为控制自动解除
       	       }
       	    }
           	
           	//复位以后的显示处理(只处理投入状态已跳闸的状态)
           	if (workSecond==1)
           	{
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,如果遥控已经跳闸就不再处理初始状态
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
                     statusOfGate |= 0x10;               //功控灯亮
     	               gateStatus(j+1, GATE_STATUS_ON);
                     alarmInQueue(WEEKEND_CTRL, pn);     //厂休控
     	             } 
     	             else    //ly,2011-03-31,添加的else
     	             {
     	               gateStatus(j+1,GATE_STATUS_OFF);
     	             }
     	           }
     	         }
           	}
          }
  
          //厂休控解除
          if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_AUTO_RELEASE)
          {
              ctrlRelease(pn,WEEKEND_CTRL,wkdCtrlConfig[pn-1].ifWkdCtrl);
              continue;
          }  	      
   	        
   	      //---------------------- 时 段 功 控 --------------------------	      
          //体现功控优先级,若高优先级控制投入则低优先级控制不执行
   	      if (ctrlRunStatus[pn-1].ifUsePwrCtrl == 0xAA
   	      	  || ctrlRunStatus[pn-1].ifUseObsCtrl == 0xAA
   	      	    || ctrlRunStatus[pn-1].ifUseWkdCtrl == 0xAA)
       	  {
       	    continue;
       	  }
       	  //时段功控投入
       	  if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_JUMP_IN)
       	  {
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
    	        printf("\n总加组%d时段功控投入\n",pn);
    	      }
  
            //获取当前时间段的时段索引,若该时段无效或参数不恰当,不进行时段功控判断
            if ((tmpPeriodCount = getPowerPeriod(sysTime)) == 0)
            {
       	       //如果已进入厂休控控制状态(即已有轮次跳闸)或告警状态
       	       if (periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl == 0xAA || periodCtrlConfig[pn-1].ctrlPara.prdAlarm==CTRL_ALARM)
       	       {
       	          ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_AUTO_RELEASE;   //置为控制自动解除
  
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("已进入厂休,时段控自动合闸\n");
        	        }
       	       }
               continue;
            }
  
            //根据当前时间, 总加组号, 方案号, 时段索引获取时段功控定值
            periodCtrlLimit = 0;
  
            if (getPowerLimit(pn, periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod, tmpPeriodCount, (INT8U *)&periodCtrlLimit))
            {
                //每次只跳闸一个轮次
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //如果还有轮次未跳完,执行继续监控
                 if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	  nextRound = sysTime;
                   }
  
                   if (compareTwoTime(nextRound,sysTime))   //下一轮次等待WAIT_NEXT_ROUND分钟
                   {
                       ctrlLimit = alarmLimit = presentData = 0x00000000;
  	                   ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
  	                   
  	                   if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
  	                   {
  	                     printf("时段功控计算\n");
  	                   }
  
                       if (calcData(ctrlReadBuf,periodCtrlLimit,&presentData,NULL,&ctrlLimit,&presentWatt,NULL,&ctrlLimitWatt,ctrlPara.pCtrlIndex, 0x02, pn))
                       {
                          //保安定值
                          protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
                          //控制值与保安定值比较,如果控制值小于保安定值,则控制值=保安定值
                          if (ctrlLimit<protectSafeKw || (ctrlLimit==protectSafeKw && ctrlLimitWatt<protectSafeWatt))
                          {
                             ctrlLimit = protectSafeKw;
                             ctrlLimitWatt = protectSafeWatt;
                          }
                          
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("时段功控:保安定值=%0d\n",protectSafeKw);
                            printf("时段功控:控制值=%0d\n",ctrlLimit);
                            printf("时段功控:当前功率=%0d\n",presentData);
                          }
                          
                          //超功率告警限值
                          if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                          {
                             //已进入告警状态
                             if (periodCtrlConfig[pn-1].ctrlPara.prdAlarm == CTRL_ALARM)
                             {
                               //告警时间到期,跳闸一个轮次
                               if (compareTwoTime(periodCtrlConfig[pn-1].ctrlPara.alarmEndTime,sysTime))
                               {
                                 //输出控制信号，清除告警信号
                                 //依据该总加组的电控轮次设定逐个轮次设定
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);
                                     
                                     statusOfGate |= 0x10;    //功控灯亮

                                     ctrlJumpedStatis(TIME_CTRL, 1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                     
                                     //记录功控跳闸现场信息
                                     powerCtrlInQueue(pn,TIME_CTRL, j, (INT8U *)&periodCtrlLimit);
                                     
                                     break;
                                   }
                                 }
                                 
                                 periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl = 0xAA;
                                 
                                 //控制标志置位，告警标志清除
                                 periodCtrlConfig[pn-1].ctrlPara.alarmEndTime.year = 0xFF;
                                 periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_JUMPED;      //已跳闸
  
    			                       saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
  			                         saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
                             
                                 alarmInQueue(TIME_CTRL,pn);                                  //时段功控
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0); //下一轮次等待WAIT_NEXT_ROUND分钟
                             
                                 //不再检查其他控制条件，继续下一个总加组
                                 break;
                               }
                             }
                             else  //未进入告警状态
                             {
                               //计算告警时限
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //功控轮次已设定且未跳闸
                                  if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  tmpRound++;
                               }
                             	 if (powerCtrlAlarmTime[j].def==0xaa)
                             	 {
                             	   periodCtrlConfig[pn-1].ctrlPara.alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                             	 }
                             	 else     //如果没有设置该轮次的功控告警时间,默认为1分钟
                             	 {
                             	   periodCtrlConfig[pn-1].ctrlPara.alarmEndTime = nextTime(sysTime, 1, 0);
                             	 }                                  
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;           //开始告警提示音时间
     	                         setBeeper(BEEPER_ON);                              //给出声音示警
                               alarmLedCtrl(ALARM_LED_ON);                        //指示灯亮
                               
                               //如果还没有轮次进入跳闸状态,将所有跳闸状态置为未跳闸,避免其它方式转入时段功控造成成混乱
                               if (periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl == 0x00)
                               {
                                  powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               //输出告警信号
                               periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_ALARM;  //告警
                               alarmInQueue(TIME_CTRL,pn);                             //时段功控
                               
    			                     saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
                             }
                          }
                       }
                    }
                    
                    break;  //每次只跳闸一个轮次,所以如果比较了一个轮次就退出
                  }
                }
       	    }
       	    else
       	    {
       	       //如果已进入时段功控控制状态(即已有轮次跳闸)状态
       	       if (periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl == 0xAA)
       	       {
       	          ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_AUTO_RELEASE;   //置为控制自动解除
  
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("离开控制时段,时段控自动合闸\n");
        	        }
       	       }      	    	 
       	    }
           	
           	//复位以后的显示处理(只处理投入状态已跳闸的状态)
           	if (workSecond==1)
           	{
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,如果遥控已经跳闸就不再处理初始状态
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
     	               gateStatus(j+1,GATE_STATUS_ON);
                     statusOfGate |= 0x10;            //功控灯亮
                     alarmInQueue(TIME_CTRL,pn);      //时段功控
     	             }
     	             else    //ly,2011-03-31,添加的else
     	             {
     	                gateStatus(j+1,GATE_STATUS_OFF);
     	             }
     	           }
     	         }
           	}
           }
  
           //时段功控解除
           if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_RELEASE   || ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_AUTO_RELEASE)
           {
              ctrlRelease(pn,TIME_CTRL,periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl);
           }
   	    }
    	}
    	
    	balanceComplete = FALSE;
    	
    	usleep(800000);
   }
}

/*******************************************************
函数名称:countAlarmLimit
功能描述:将数据格式2或3计算为16进制数,便于比较
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：对数据格式2而言是16进制KW数,隐含的返回值watt返回的是小于KW的W值
        对数据格式3
*******************************************************/
INT32U countAlarmLimit(INT8U *data, INT8U dataFormat, INT8U ctrlIndex,INT16U *watt)
{
    INT32U   tmpData;
    INT8U    tmpIndex;
    
    if (dataFormat == 0x02)
    {
      tmpData = (*data&0xF) + (*data>>4&0xF)*10
              + (*(data+1)&0xF)*100;
      
      //计算实际浮动系数
      if (ctrlIndex != 0x00)
      {
        tmpIndex = (ctrlIndex&0xF) + (ctrlIndex>>4&0x7)*10;
      }
      else
      {
        tmpIndex = 0;
      }
     
      if (ctrlIndex>>7)//下浮
      {
        tmpData = (tmpData * (100-tmpIndex)) / 100;
      }
      else             //上浮
      {
        tmpData = (tmpData * (100+tmpIndex)) / 100;
      }
      
      *watt=0;
      switch ((*(data+1)>>5&0x07))
      {
        case 0:
        	tmpData *= 10000;
        	break;
        case 1:
        	tmpData *= 1000;
        	break;
        case 2:
        	tmpData *= 100;
        	break;
        case 3:
        	tmpData *= 10;
        	break;
        case 5:
        	*watt = tmpData%10*100;
        	tmpData /= 10;
        	break;
        case 6:
        	*watt = tmpData%100*10;
        	tmpData /= 100;
        	break;
        case 7:
        	*watt = tmpData%1000;
        	tmpData /= 1000;
        	break;
        default:
        	break;
      }
    }

    if (dataFormat == 0x03)
    {
      tmpData = (*data&0xF) + (*data>>4&0xF)*10
               + (*(data+1)&0xF)*100 + (*(data+1)>>4&0xF)*1000
               + (*(data+2)&0xF)*10000 + (*(data+2)>>4&0xF)*100000
               + (*(data+3)&0xF)*1000000;
      
      //计算实际浮动系数
      if (ctrlIndex != 0x00)
      {
         tmpIndex = (ctrlIndex&0xF) + (ctrlIndex>>4&0x7)*10;
      }
      else
      {
         tmpIndex = 0;
      }
     
      if (ctrlIndex>>7) //下浮
      {
         tmpData = (tmpData * (100-tmpIndex)) / 100;
      }
      else              //上浮
      {
         tmpData = (tmpData * (100+tmpIndex)) / 100;
      }
      
      //MWh
      if ((*(data+3)>>6&0x01) == 0x01)
      {
        if (tmpData<4294967)
        {
           tmpData *= 1000;
           *watt = 0;
        }
        else  //大于4294967*1000将溢出,需要处理 LY 2008.12.17
        {
           tmpData *= 1000;
           *watt = 0;
        }
      }
    }
    
    return tmpData;
}

/**************************************************
函数名称:getPresentPowerLimit
功能描述:获得获取指定时间的时段索引
调用函数:
被调用函数:
输入参数:1.DATE_TIME limitTime    指定时间
输出参数:INT8U* powerLimit  时段功控定值
返回值：时段数(用于获取时段功控定值的索引)
        错误返回0
***************************************************/
INT8U getPowerPeriod(DATE_TIME  limitTime)
{
    INT8U      count, tmpData, i, j;
    INT8U      hourIndex = limitTime.hour;
    INT8U      minuteIndex = limitTime.minute;
    
    //计算给定时段在参数结构中的索引位置
    count = 0;
    tmpData = 0xFF;
    for (i = 0; i < 12; i++)
    {
      for (j = 0; j < 4; j++)
      {
        if (tmpData != ((ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03))
        {
          if (((ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03)==0x1 || ((ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03)==0x2)
          {
             count++;
          }
          tmpData = (ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03;
        }

        if ((hourIndex*2 + minuteIndex/30) == (i*4 + j))
        {
          i = 12;
          break;
        }
      }
    }
    
    //查询过程出错，或该时段被设置为"不控制"，返回查询失败标志
    if (count > 8 || count == 0 || tmpData == 0x00)
    {
       return 0;
    }
    
    return  count;
}

/**************************************************
函数名称:getPowerLimit
功能描述:获取指定时段的功控定值
调用函数:
被调用函数:
输入参数:1.INT8U     grpPn        总加组测量点号 1~8代表1~8总加组
         2.INT8U     limitPeriod  时段方案号 0~2代表1~3时段
         3.INT8U     periodCount  时段号 1~8
输出参数:INT8U* powerLimit  时段功控定值
返回值：查询功控定值有具体数值true
        查询功控定值没有需要数值 false
        查询错误 false
        参数错误 false
***************************************************/
BOOL getPowerLimit(INT8U grpPn, INT8U limitPeriod, INT8U periodCount, INT8U* powerLimit)
{
   //若总加组的指定时段方案未配置,返回查询失败标志
   if ((periodCtrlConfig[grpPn-1].periodNum>>limitPeriod&0x01) == 0x00)
   {
      return FALSE;
   }
   
   //若总加组指定时段的功控时段未配置,返回查询失败标志
   if ((periodCtrlConfig[grpPn-1].period[limitPeriod].timeCode>>(periodCount-1)&0x01) == 0x00)
   {
      return FALSE;
   }
   
   //若总加组的指定时段为未投入,返回查询失败标志
   if ((periodCtrlConfig[grpPn-1].ctrlPara.limitPeriod>>(periodCount-1)&0x01) == 0x00)
   {
      return FALSE;
   }

   //根据索引查询功控定值
   *powerLimit = periodCtrlConfig[grpPn-1].period[limitPeriod].limitTime[periodCount-1][0];
   *(powerLimit+1) = periodCtrlConfig[grpPn-1].period[limitPeriod].limitTime[periodCount-1][1];

   return TRUE;
}

/**************************************************
函数名称:jumpGate
功能描述:跳闸动作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void jumpGate(INT8U line,INT8U onOff)
{
 	 INT8U  buf;
 	 INT32U i;

	 if (line<1 || line>4)
	 {
	 	  return;
	 }
	 
	 if (onOff==1)
	 {
  	 gateJumpTime[line] = 1;  //本路继电器已跳闸,开始计时

 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 0);
         printf("jumpGate:第1路跳闸\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 0);
         printf("jumpGate:第2路跳闸\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 0);
         printf("jumpGate:第3路跳闸\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 0);
         printf("jumpGate:第4路跳闸\n");
 	 	     break;
 	   }
   }
   else
   {
 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 1);
         printf("jumpGate:第1路合闸\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 1);
         printf("jumpGate:第2路合闸\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 1);
         printf("jumpGate:第3路合闸\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 1);
         printf("jumpGate:第4路合闸\n");
 	 	     break;
 	   }
   }
   
   ioctl(fdOfIoChannel, LOAD_CTRL_CLOCK, 0);
   
 	 buf = 0xc;
 	 buf |= 0xa0;   //控制输出允许
   
   ioctl(fdOfIoChannel, LOAD_CTRL_CLOCK, 1);
   
   for(i=0;i<0x20000;i++)
   {
     ;
   }

 	 buf = 0xc;
 	 buf |= 0x00;   //控制输出禁止
   
   //ly,2011-03-24,由于在做高低温的时候发现,有时候跳/合闸不可靠,考虑到时序问题,将ATXMEGA32A4上的EN直接到地就不用下发控制EN了
   //sendXmegaInTimeFrame(TE_CTRL_EN, &buf, 1);

   ioctl(fdOfIoChannel, LOAD_CTRL_CLOCK, 0);
}

/**************************************************
函数名称:jumpGateNoAction
功能描述:给出跳闸电平不输出控制继电器动作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void jumpGateNoAction(INT8U line,INT8U onOff)
{
 	 INT8U  buf;
 	 INT32U i;

	 if (line<1 || line>4)
	 {
	 	  return;
	 }
	 
	 if (onOff==1)
	 {
 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 0);
         printf("jumpGateNoAction:第1路跳闸控制线使能\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 0);
         printf("jumpGateNoAction:第2路跳闸控制线使能\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 0);
         printf("jumpGateNoAction:第3路跳闸控制线使能\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 0);
         printf("jumpGateNoAction:第4路跳闸控制线使能\n");
 	 	     break;
 	   }
   }
   else
   {
 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 1);
         printf("jumpGateNoAction:第1路合闸控制线禁止\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 1);
         printf("jumpGateNoAction:第2路合闸控制线禁止\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 1);
         printf("jumpGateNoAction:第3路合闸控制线禁止\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 1);
         printf("jumpGateNoAction:第4路合闸控制线禁止\n");
 	 	     break;
 	   }
   }
}

/**************************************************
函数名称:gateStatus
功能描述:闸状态指示
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void gateStatus(INT8U line,INT8U onOff)
{
	 if (line<1 || line>CONTROL_OUTPUT)
	 {
	 	  return;
	 }
	 
	 if (onOff==GATE_STATUS_ON)
	 {
  	 switch(line)
  	 {
  	 	 case 1:
         statusOfGate |= CTRL_STATUS_JUMP_1;
  	 	   break;
  
  	 	 case 2:
         statusOfGate |= CTRL_STATUS_JUMP_2;
  	 	   break;
  	 	   
  	 	 case 3:
         statusOfGate |= CTRL_STATUS_JUMP_3;
  	 	   break;
  	 	   
  	 	 case 4:
         statusOfGate |= CTRL_STATUS_JUMP_4;
  	 	   break;
  	 }
   }
   else
   {
  	 #ifdef TE_CTRL_BOARD_V_1_3
  	  switch(line)
  	  {
  	 	  case 1:
          statusOfGate &= CTRL_STATUS_CLOSE_1|0xf0;
  	 	    break;
  
  	 	  case 2:
          statusOfGate &= CTRL_STATUS_CLOSE_2|0xf0;
  	 	    break;
  	 	   
  	 	  case 3:
  	 	    statusOfGate &= CTRL_STATUS_CLOSE_3|0xf0;
  	 	    break;
  	 	   
  	 	  case 4:
          statusOfGate &= CTRL_STATUS_CLOSE_4|0xf0;
  	 	    break;
  	  }
  	 #else
  	  switch(line)
  	  {
  	 	  case 1:
          statusOfGate &= CTRL_STATUS_CLOSE_1;
  	 	    break;
  
  	 	  case 2:
          statusOfGate &= CTRL_STATUS_CLOSE_2;
  	 	    break;
  	 	   
  	 	  case 3:
  	 	    statusOfGate &= CTRL_STATUS_CLOSE_3;
  	 	    break;
  	 	   
  	 	  case 4:
          statusOfGate &= CTRL_STATUS_CLOSE_4;
  	 	    break;
  	  }
  	 #endif   	  
   }
}

/**************************************************
函数名称:calcData
功能描述:计算数据(当前值,限值,控制值)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：TRUE-计算完成,FALSE-无数据或数据不完整
***************************************************/
BOOL calcData(INT8U *buff,INT32U limit,INT32U *powerKw,INT32U *limitKw,INT32U *ctrlKw,INT16U *powerWatt,INT16U *limitWatt,INT16U *ctrlWatt,INT8U index, INT8U format,INT8U pn)
{
	  DATE_TIME tmpTime;
	  
	  switch (format)
	  {
	  	case 0x2:
        if (calcRealPower(buff, pn)==FALSE)
        {
        	 return FALSE;
        }
  	    if (buff[GP_WORK_POWER+1] != 0xee)
  	    {
          *powerKw = buff[GP_WORK_POWER+1];
          *powerKw |= (buff[GP_WORK_POWER+2]&0xF)<<8;
          *powerKw |= (buff[GP_WORK_POWER]&0x10)<<8;
          *powerKw |= (buff[GP_WORK_POWER]&0x07)<<13;
        }
        else
        {
      	  return FALSE;
        }
        break;
      
      case 0x3:
        tmpTime = timeHexToBcd(sysTime);
        if (readMeterData(buff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
	      {
          if (buff[GP_MONTH_WORK+3] != 0xFF || buff[GP_MONTH_WORK+3] != 0xEE)
          {
            *powerKw = buff[GP_MONTH_WORK+3]
                     | buff[GP_MONTH_WORK+4] << 8 
                     | buff[GP_MONTH_WORK+5] << 16 
                     | (((buff[GP_MONTH_WORK]&0x01)<<6)|(buff[GP_MONTH_WORK]&0x10)|(buff[GP_MONTH_WORK+6]&0x0f))<<24;
          }
          else
          {
      	    return FALSE;
          }
        }
        else
        {
        	 return FALSE;
        }
        break;
    }
    
    if (powerKw!=limitKw)
    {
      *powerKw = countAlarmLimit((INT8U*)powerKw, format, 0x00,powerWatt);
    }
    if (limitKw!=NULL)
    {
      if (powerKw!=limitKw)
      {
        *limitKw = countAlarmLimit((INT8U*)&limit, format, index,limitWatt);
      }
      else
      {
        *limitKw = countAlarmLimit((INT8U*)powerKw, format, index,limitWatt);
      }
    }
    if (ctrlKw!=NULL)
    {
      *ctrlKw  = countAlarmLimit((INT8U*)&limit, format, 0x00,ctrlWatt);
    }
    
    return TRUE;
}


/**************************************************
函数名称:alarmInQueue
功能描述:告警信号进入队列
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void alarmInQueue(INT8U alarmType,INT8U pn)
{
	  INT8U i;
	  for(i=0;i<ctrlStatus.numOfAlarm;i++)
	  {
	  	 if (ctrlStatus.aQueue[i]==alarmType && ctrlStatus.pn[i]==pn)
	  	 {
	  	 	  return;
	  	 }
	  }
	  ctrlStatus.aQueue[i] = alarmType;
	  ctrlStatus.allPermitClose[i] = 0;
	  ctrlStatus.pn[i] = pn;
	  ctrlStatus.numOfAlarm++;
}

/**************************************************
函数名称:ctrlRelease
功能描述:控制解除处理
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void ctrlRelease(INT8U pn, INT8U ctrlType,INT8U ifJumped)
{  
   INT8U j;
   INT8U tmpFlag;    //临时标志
   
   //解除控制输出
   for (j = 0; j < CONTROL_OUTPUT; j++)
   {
      if (ctrlType==MONTH_CTRL || ctrlType==CHARGE_CTRL)
      {
      	 tmpFlag = electCtrlRoundFlag[pn-1].flag;
      }
      else
      {
      	 tmpFlag = powerCtrlRoundFlag[pn-1].flag;
      }
      
      if (((tmpFlag>>j) & 0x01) == 0x01)
      {
         jumpGate(j+1, 0);
         gateStatus(j+1,GATE_STATUS_OFF);
      }
   }
   
   if (ctrlType==MONTH_CTRL || ctrlType==CHARGE_CTRL)
   {
     electCtrlRoundFlag[pn-1].ifJumped = 0x00;              //跳闸已跳闸标志全部清除
		 saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
     
     statusOfGate &= 0xdf;    //电控灯灭
   }
   else
   {
     powerCtrlRoundFlag[pn-1].ifJumped = 0x00;              //功控已跳闸标志全部清除
	   saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn-1], sizeof(POWERCTRL_ROUND_FLAG));
     statusOfGate &= 0xef;    //功控灯灭
   }
 
   if (ifJumped==0xaa)
   {
     gateCloseWaitTime=GATE_CLOSE_WAIT_TIME;              //允许合闸告警音时间
     setBeeper(BEEPER_ON);                                //给出声音示警
     alarmLedCtrl(ALARM_LED_ON);                          //指示灯亮
   }

   switch(ctrlType)
   {
   	  case POWER_CTRL:     //当前功率下浮控
        if (ifJumped==0xaa)
        {
           powerDownCtrl[pn-1].pwrDownAlarm = CTRL_CLOSE_GATE; //允许合闸
           alarmInQueue(POWER_CTRL,pn);                        //功率下浮控
        }
        else
        {
        	 if (powerDownCtrl[pn-1].pwrDownAlarm==CTRL_ALARM)
        	 {
        	 	  powerDownCtrl[pn-1].pwrDownAlarm=CTRL_ALARM_CANCEL;
        	 }
        	 else
        	 {
        	 	  powerDownCtrl[pn-1].pwrDownAlarm = 0;
        	 }
        }
  	    powerDownCtrl[pn-1].ifPwrDownCtrl = 0x00;                 //置为未进入控制状态
        if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_JUMP_IN;       //控制还原为投入
           powerDownCtrl[pn-1].pwrDownAlarm=CTRL_ALARM_AUTO_CLOSE;//自动合闸
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_RELEASEED;     //控制解除已执行
  	    }
  	    
	      saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //当前功率下浮功控
   	  	break;

   	  case OBS_CTRL:       //营业报停控
        if (ifJumped==0xaa)
        {
           obsCtrlConfig[pn-1].obsAlarm = CTRL_CLOSE_GATE;      //允许合闸
           alarmInQueue(OBS_CTRL,pn);                           //报停控
        }
        else
        {
        	 if (obsCtrlConfig[pn-1].obsAlarm == CTRL_ALARM)
        	 {
        	 	  obsCtrlConfig[pn-1].obsAlarm = CTRL_ALARM_CANCEL; //告警取消
        	 }
        	 else
        	 {
        	 	  obsCtrlConfig[pn-1].obsAlarm = 0;
        	 }
        }
  	    obsCtrlConfig[pn-1].ifObsCtrl = 0x00;                   //置为未进入控制状态
        if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_JUMP_IN;     //控制还原为投入
           obsCtrlConfig[pn-1].obsAlarm=CTRL_ALARM_AUTO_CLOSE;  //自动合闸
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_RELEASEED;   //控制解除已执行
  	    }

	  		saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
   	  	break;
   	  	
   	  case WEEKEND_CTRL:   //厂休功控
        if (ifJumped==0xaa)
        {
          wkdCtrlConfig[pn-1].wkdAlarm = CTRL_CLOSE_GATE;  //允许合闸
          alarmInQueue(WEEKEND_CTRL,pn);                   //厂休功控
        }
        else
        {
          if (wkdCtrlConfig[pn-1].wkdAlarm == CTRL_ALARM)
          {
          	 wkdCtrlConfig[pn-1].wkdAlarm = CTRL_ALARM_CANCEL;  //告警取消
          }
          else
          {
             wkdCtrlConfig[pn-1].wkdAlarm = 0x0;
          }
        }
 	      wkdCtrlConfig[pn-1].ifWkdCtrl = 0x00;              //置为未进入控制状态

        if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_JUMP_IN;     //控制还原为投入
           wkdCtrlConfig[pn-1].wkdAlarm=CTRL_ALARM_AUTO_CLOSE;  //自动合闸
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_RELEASEED;   //控制解除已执行
  	    }
	  		saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
   	  	break;
   	  
   	  case TIME_CTRL:   //时段功控
        if (ifJumped==0xaa)
        {
          periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_CLOSE_GATE;  //允许合闸
          alarmInQueue(TIME_CTRL,pn);                                  //厂休功控
        }
        else
        {
          if (periodCtrlConfig[pn-1].ctrlPara.prdAlarm == CTRL_ALARM)
          {
          	 periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_ALARM_CANCEL;  //告警取消
          }
          else
          {
             periodCtrlConfig[pn-1].ctrlPara.prdAlarm = 0x0;
          }
        }
 	      periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl = 0x00;                 //置为未进入控制状态
        if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_JUMP_IN;      //控制还原为投入
           periodCtrlConfig[pn-1].ctrlPara.prdAlarm=CTRL_ALARM_AUTO_CLOSE;//自动合闸
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_RELEASEED;   //控制解除已执行
  	    }
  			saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
   	  	break;
   	  	
   	  case MONTH_CTRL:   //月电控
        if (ifJumped==0xaa)
        {
          monthCtrlConfig[pn-1].monthAlarm = CTRL_CLOSE_GATE;  //允许合闸
          alarmInQueue(MONTH_CTRL,pn);                         //月电控
        }
        else
        {
          if (monthCtrlConfig[pn-1].monthAlarm == CTRL_ALARM)
          {
          	 monthCtrlConfig[pn-1].monthAlarm = CTRL_ALARM_CANCEL;  //告警取消
          }
          else
          {
             monthCtrlConfig[pn-1].monthAlarm = 0x0;
          }
        }
 	      monthCtrlConfig[pn-1].ifMonthCtrl = 0x00;                   //置为未进入控制状态
        if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_JUMP_IN;         //控制还原为投入
           monthCtrlConfig[pn-1].monthAlarm = CTRL_ALARM_AUTO_CLOSE;//自动合闸
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_RELEASEED;       //控制解除已执行
  	    }
  	    
			  saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
   	  	break;
   	  	
   	  case CHARGE_CTRL:   //购电控
        if (ifJumped==0xaa)
        {
          chargeCtrlConfig[pn-1].chargeAlarm = CTRL_CLOSE_GATE;  //允许合闸
          alarmInQueue(CHARGE_CTRL,pn);                          //购电控
        }
        else
        {
          if (chargeCtrlConfig[pn-1].chargeAlarm == CTRL_ALARM)
          {
          	 chargeCtrlConfig[pn-1].chargeAlarm = CTRL_ALARM_CANCEL;  //告警取消
          }
          else
          {
             chargeCtrlConfig[pn-1].chargeAlarm = 0x0;
          }
        }
 	      chargeCtrlConfig[pn-1].ifChargeCtrl = 0x00;                   //置为未进入控制状态
        if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_JUMP_IN;         //控制还原为投入
           chargeCtrlConfig[pn-1].chargeAlarm = CTRL_ALARM_AUTO_CLOSE;//自动合闸
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_RELEASEED;       //控制解除已执行
  	    }
  	    
			  saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
   	  	break;   	  	
   }
	 
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替

   nextRound.year=0x0;                                     //下一轮次等待时间置为0
}

/**************************************************
函数名称:electCtrlRecord
功能描述:ERC7.电控跳闸/告警记录
调用函数:
被调用函数:
输入参数:type-1,跳闸记录 2,电控告警记录
输出参数:
返回值：void
***************************************************/
void electCtrlRecord(INT8U type, INT8U pn, INT8U round, INT8U ctrlType,INT8U *totalEnergy,INT8U *limit)
{
  INT8U eventData[19];
  
  if ((type==1 && ((eventRecordConfig.iEvent[0] & 0x40) || (eventRecordConfig.nEvent[0] & 0x40)))
  	 || (type==2 && ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[0] & 0x80)))
  	 )
  {
  	 if (type==1)
  	 {
  	   eventData[0] = 0x7;
  	 }
  	 else
  	 {
  	   eventData[0] = 23;
  	 }
  	 eventData[1] = 19;
  	  
  	 //跳闸/告警时间
  	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
  	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
  	 eventData[4] = sysTime.hour/10<<4 | sysTime.hour%10;
  	 eventData[5] = sysTime.day/10<<4 | sysTime.day%10;
  	 eventData[6] = sysTime.month/10<<4 | sysTime.month%10;
  	 eventData[7] = sysTime.year/10<<4 | sysTime.year%10;
  	  
  	 //总加组
  	 eventData[8] = pn;
  	  
  	 //跳闸/告警投入轮次
  	 eventData[9] = round;
  	 
  	 //电控类别
  	 if (ctrlType==MONTH_CTRL)
  	 {
  	 	 eventData[10] = 0x01;    //月电控
  	 }
  	 else
  	 {
  	 	 eventData[10] = 0x02;    //购电控
  	 }
  	 
  	 //跳闸时电能量(总加电能量)
  	 eventData[11] = *totalEnergy++;
  	 eventData[12] = *totalEnergy++;
  	 eventData[13] = *totalEnergy++;
  	 eventData[14] = *totalEnergy;
  	 
  	 //跳闸时电能量定值
  	 eventData[15] = *limit++;
  	 eventData[16] = *limit++;
  	 eventData[17] = *limit++;
  	 eventData[18] = *limit;

     if (type==1)   //跳闸记录
     {
       if (eventRecordConfig.iEvent[0] & 0x40)
       {
  	     writeEvent(eventData, 19, 1, DATA_FROM_GPRS);   //记入重要事件队列
  	   }
       if (eventRecordConfig.nEvent[0] & 0x40)
       {
  	     writeEvent(eventData, 19, 2, DATA_FROM_LOCAL);  //记入一般事件队列
  	   }
  	 }
  	 else           //电控告警记录
  	 {
       if (eventRecordConfig.iEvent[2] & 0x80)
       {
  	     writeEvent(eventData, 19, 1, DATA_FROM_GPRS);   //记入重要事件队列
  	   }
       if (eventRecordConfig.nEvent[2] & 0x80)
       {
  	     writeEvent(eventData, 19, 2, DATA_FROM_LOCAL);  //记入一般事件队列
  	   }
  	 }
     
     if (debugInfo&PRINT_EVENT_DEBUG)
     {
       printf("electCtrlRecord调用主动上报\n");
     }

     activeReport3();   //主动上报事件
  	 
  	 eventStatus[0] = eventStatus[0] | 0x40;
  }
}

/**************************************************
函数名称:powerCtrlRecord
功能描述:ERC6.功控跳闸记录
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void powerCtrlRecord(void)
{
  INT8U  i,j;
  INT8U  eventData[19];
  INT32U eventKw;
  
  for(i=0;i<powerCtrlEventInfor.numOfCtrl;i++)
  {
     if (compareTwoTime(powerCtrlEventInfor.perCtrl[i].freezeTime,sysTime))
     {
        if ((eventRecordConfig.iEvent[0] & 0x20) || (eventRecordConfig.nEvent[0] & 0x20))
        {
        	 eventData[0] = 0x6;
        	 eventData[1] = 17;
        	  
        	 //跳闸时间
        	 eventData[2] = powerCtrlEventInfor.perCtrl[i].ctrlTime.second/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.second%10;
        	 eventData[3] = powerCtrlEventInfor.perCtrl[i].ctrlTime.minute/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.minute%10;
        	 eventData[4] = powerCtrlEventInfor.perCtrl[i].ctrlTime.hour/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.hour%10;
        	 eventData[5] = powerCtrlEventInfor.perCtrl[i].ctrlTime.day/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.day%10;
        	 eventData[6] = powerCtrlEventInfor.perCtrl[i].ctrlTime.month/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.month%10;
        	 eventData[7] = powerCtrlEventInfor.perCtrl[i].ctrlTime.year/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.year%10;
        	  
        	 //总加组
        	 eventData[8] = powerCtrlEventInfor.perCtrl[i].pn;
        	  
        	 //跳闸轮次
        	 //#ifdef ONCE_JUMP_ONE_ROUND
        	  eventData[9] = 1<<powerCtrlEventInfor.perCtrl[i].gate;
        	 //#else
        	 // eventData[9] = powerCtrlEventInfor.perCtrl[i].gate;
        	 //#endif
        	 
        	 //功控类别
        	 switch(powerCtrlEventInfor.perCtrl[i].ctrlType)
        	 {
        	 	  case TIME_CTRL:     //时段功控
        	 	  	eventData[10] = 0x1;
        	 	  	break;
        	 	  	
        	 	  case WEEKEND_CTRL:  //厂休控
        	 	  	eventData[10] = 0x2;
        	 	  	break;
      
        	 	  case OBS_CTRL:     //营业停控
        	 	  	eventData[10] = 0x4;
        	 	  	break;
      
        	 	  case POWER_CTRL:   //当前功率下浮控
        	 	  	eventData[10] = 0x8;
        	 	  	break;
        	 }
        	 
        	 //跳闸时功率(总加功率)
        	 eventData[11] = powerCtrlEventInfor.perCtrl[i].ctrlPower[0];
        	 eventData[12] = powerCtrlEventInfor.perCtrl[i].ctrlPower[1];
        	 
        	 //跳闸后2min后的功率
  	       if (calcRealPower(ctrlReadBuf, powerCtrlEventInfor.perCtrl[i].pn))
           {
             eventKw = ctrlReadBuf[GP_WORK_POWER+1];
             eventKw |= (ctrlReadBuf[GP_WORK_POWER+2]&0xF)<<8;
             eventKw |= (ctrlReadBuf[GP_WORK_POWER]&0x10)<<8;
             eventKw |= (ctrlReadBuf[GP_WORK_POWER]&0x07)<<13;
        	   eventData[13] = eventKw&0xff;
        	   eventData[14] = eventKw>>8&0xff;
        	 }
        	 else
        	 {
        	   eventData[13] = 0x0;
        	   eventData[14] = 0x0;
        	 }
        	 
        	 //跳闸时功率定值
        	 eventData[15] = powerCtrlEventInfor.perCtrl[i].limit[0];
        	 eventData[16] = powerCtrlEventInfor.perCtrl[i].limit[1];
      
           if (eventRecordConfig.iEvent[0] & 0x20)
           {
        	    writeEvent(eventData, 17 , 1, DATA_FROM_GPRS);   //记入重要事件队列
        	 }
           if (eventRecordConfig.nEvent[0] & 0x20)
           {
        	    writeEvent(eventData, 17 , 2, DATA_FROM_LOCAL);  //记入一般事件队列
        	 }
        	  
        	 eventStatus[0] = eventStatus[0] | 0x20;
           
           if (debugInfo&PRINT_EVENT_DEBUG)
           {
             printf("powerCtrlRecord调用主动上报\n");
           }

           activeReport3();   //主动上报事件
        }
        
        //出队列
        for(j=i;j<powerCtrlEventInfor.numOfCtrl;j++)
        {
         	 powerCtrlEventInfor.perCtrl[j].pn = powerCtrlEventInfor.perCtrl[j+1].pn;
         	 powerCtrlEventInfor.perCtrl[j].ctrlTime = powerCtrlEventInfor.perCtrl[j+1].ctrlTime;
         	 powerCtrlEventInfor.perCtrl[j].freezeTime = powerCtrlEventInfor.perCtrl[j+1].freezeTime;
         	 powerCtrlEventInfor.perCtrl[j].gate = powerCtrlEventInfor.perCtrl[j+1].gate;
         	 powerCtrlEventInfor.perCtrl[j].ctrlType = powerCtrlEventInfor.perCtrl[j+1].ctrlType;
            
         	 powerCtrlEventInfor.perCtrl[j].ctrlPower[0] = powerCtrlEventInfor.perCtrl[j+1].ctrlPower[0];
         	 powerCtrlEventInfor.perCtrl[j].ctrlPower[1] = powerCtrlEventInfor.perCtrl[j+1].ctrlPower[1];
         	 
         	 powerCtrlEventInfor.perCtrl[j].limit[0] = powerCtrlEventInfor.perCtrl[j+1].limit[0];
         	 powerCtrlEventInfor.perCtrl[j].limit[1] = powerCtrlEventInfor.perCtrl[j+1].limit[1];
        }
        powerCtrlEventInfor.numOfCtrl--;
        
        saveParameter(0x05, 5, (INT8U *)&powerCtrlEventInfor, sizeof(POWER_CTRL_EVENT_INFOR));    //功控现场记录状态用FN05代替
        break;
     }
  }
}


/**************************************************
函数名称:powerCtrlInQueue
功能描述:功控现场信息进入队列
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void powerCtrlInQueue(INT8U pn,INT8U ctrlType,INT8U gate,INT8U *limit)
{
   INT32U eventDataKw;
   
   //记录功控现场信息
   if (powerCtrlEventInfor.numOfCtrl>6)
   {
   	 powerCtrlEventInfor.numOfCtrl=0;
   }                                  
   if (powerCtrlEventInfor.numOfCtrl<6)
   {
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].pn = pn;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlTime = sysTime;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].freezeTime = nextTime(sysTime,2,0);
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].gate = gate;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlType = ctrlType;
      
     eventDataKw =ctrlReadBuf[GP_WORK_POWER+1];
     eventDataKw |= (ctrlReadBuf[GP_WORK_POWER+2]&0xF)<<8;
     eventDataKw |= (ctrlReadBuf[GP_WORK_POWER]&0x10)<<8;
     eventDataKw |= (ctrlReadBuf[GP_WORK_POWER]&0x07)<<13;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlPower[0] = eventDataKw&0xff;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlPower[1] = eventDataKw>>8&0xff;
   	 
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].limit[0] = *limit++;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].limit[1] = *limit;
   	 
   	 powerCtrlEventInfor.numOfCtrl++;  //队列个数加1
   }
   
   //ly,2011-04-21,add
   saveParameter(0x05, 5, (INT8U *)&powerCtrlEventInfor, sizeof(POWER_CTRL_EVENT_INFOR));    //功控现场记录状态用FN05代替
}

/**************************************************
函数名称:calcRealPower
功能描述:计算总加组实时功率
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
BOOL calcRealPower(INT8U *buff,INT8U pn)
{
	  MEASURE_POINT_PARA  tmpPointPara;
	  METER_DEVICE_CONFIG meterConfig;
    INT8U     i,j,k;
    INT8U     tmpPn, direction, sign;
    BOOL      ifFound = FALSE;
    INT32U    powerInt, powerDec, powerSign;
    INT8U     powerQuantity;
    INT32U    dataRawInt, dataRawDec, tmpData;
    
    INT8U     pulsePnData = 0;
    BOOL      buffHasData;  
    DATE_TIME tmpTime;

    #ifdef PULSE_GATHER
     INT16U  counti;
    #endif
    
    powerInt  = 0x0;
    powerDec  = 0x0;
    powerSign = POSITIVE_NUM;
    
    //根据总加组号确定存储序号
  	ifFound = FALSE;
  	for (i = 0; i < totalAddGroup.numberOfzjz; i++)
  	{
  	   if (totalAddGroup.perZjz[i].zjzNo==pn)
  	   {
  	      pn = i;
  	      ifFound = TRUE;
  	      break;
  	   }
    }
    if(ifFound==FALSE)
    {
    	 return FALSE;
    }
    
    //计算总加功率
	  for(i = 0; i < totalAddGroup.perZjz[pn].pointNumber; i++)
    { 
  	  //确定测量点号，方向，符号
      tmpPn = (totalAddGroup.perZjz[pn].measurePoint[i] & 0x3F) + 1;
      direction = totalAddGroup.perZjz[pn].measurePoint[i] & 0x40;
      sign = totalAddGroup.perZjz[pn].measurePoint[i] & 0x80;
      buffHasData = FALSE;
      pulsePnData = 0;
      
   		#ifdef PULSE_GATHER
			 	//查看是否是脉冲采样的数据
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //是脉冲量的测量点
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==tmpPn)
			    {
			      //P.1先初始化缓存
            memset(buff,0xee,LENGTH_OF_PARA_RECORD);

				 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
			   	  covertPulseData(j, NULL,NULL,buff);
			   	  	 	    
			      pulsePnData = 1;
			      
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
              printf("calcRealPower:转换测量点%d脉冲量\n",tmpPn);
            }
			   	  
			      buffHasData = TRUE;
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //不是脉冲测量点数据
		  {
        if (selectF10Data(tmpPn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
        {
		      if (meterConfig.protocol==AC_SAMPLE)
		      {
		     	  if (ifHasAcModule==TRUE)
		     	  {
			        //A.1先初始化缓存
              memset(buff,0xee,LENGTH_OF_PARA_RECORD);

			       	//A.2将交流采样数据填入dataBuff中
			       	covertAcSample(buff, NULL, NULL, 1, sysTime);
              
              if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
              {
                printf("calcRealPower:转换测量点%d交采值\n",tmpPn);
              }
			      	       	  
			     	  buffHasData = TRUE;
		   	    }
		      }
		  	  else
		  	  {
            tmpTime = queryCopyTime(tmpPn);
            buffHasData = readMeterData(buff, tmpPn, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0);
          }
        }
      }
      
      if (buffHasData == TRUE)
      {
        if (buff[POWER_INSTANT_WORK] != 0xEE)
        {
      	  dataRawInt = bcdToHex(buff[POWER_INSTANT_WORK+2]&0x7f);
          dataRawDec = bcdToHex(buff[POWER_INSTANT_WORK]|buff[POWER_INSTANT_WORK+1]<<8);
        }
        else
        {
          if (pulsePnData==1)
          {
            continue;
          }
          else
          {
          	//测量点数据不完整，统计数据标记为无该项数据，退出循环
            powerInt = 0xEE;
            break;              
          }
        }

      	//4-5-3.读取测量点对应的限制参数
		    if(selectViceParameter(0x04, 25, tmpPn, (INT8U *)&tmpPointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
    	  {
          if (tmpPointPara.voltageTimes != 0)
          {
            if (tmpPointPara.currentTimes != 0)
            {
   	          dataRawDec *= tmpPointPara.voltageTimes * tmpPointPara.currentTimes;
              dataRawInt *= tmpPointPara.voltageTimes * tmpPointPara.currentTimes;
   	        }
   	        else
   	        {
   	          dataRawDec *= tmpPointPara.voltageTimes;
   	          dataRawInt *= tmpPointPara.voltageTimes;
   	        }
   	      }
   	      else
   	      {
   	        if (tmpPointPara.currentTimes != 0)
   	        {
   	          dataRawDec *= tmpPointPara.currentTimes;
              dataRawInt *= tmpPointPara.currentTimes;
   	        }
   	      }
    	  }
    	  
	      if (sign == 0)   //总加运算+
        {
          if (powerSign == POSITIVE_NUM)  //正数+正数
          {
            powerInt += dataRawInt;
            powerDec += dataRawDec;
          }
          else    //正数-正数
          {
        	  //被减数大于减数，结果为正
            if (dataRawInt > powerInt || (dataRawInt == powerInt && dataRawDec >= powerDec))
            {
              if (dataRawDec >= powerDec)
              {
                powerDec = dataRawDec - powerDec;
                powerInt = dataRawInt - powerInt;
              }
              else
              {
                powerDec = 10000+ dataRawDec - powerDec;
                powerInt = dataRawInt - powerInt - 1;
              }
            
              powerSign = POSITIVE_NUM;
            }
            else  //被减数小于减数，结果为负
            {
              if (powerDec >= dataRawDec)
              {
                powerDec = powerDec - dataRawDec;
                powerInt = powerInt - dataRawInt;
              }
              else
              {
                powerDec = 10000 + powerDec - dataRawDec;
                powerInt = powerInt -dataRawInt - 1;
              }
          
              powerSign = NEGTIVE_NUM;
            }
          }
 	      }
 	      else  //总加运算-
 	      {
 	        if (powerSign == NEGTIVE_NUM)   //负数-正数
 	        {
 	          powerInt = powerInt + dataRawInt;
 	          powerDec = powerDec + dataRawDec;
 	        }
 	        else  //正数-正数
 	        {
 	          //被减数大于减数, 结果为正数
 	          if (powerInt > dataRawInt || (powerInt == dataRawInt && powerDec >= dataRawDec))
 	          {
 	            if (powerDec >= dataRawDec)
 	            {
 	              powerDec = powerDec - dataRawDec;
 	              powerInt = powerInt - dataRawInt;
 	            }
 	            else
 	            {
 	              powerDec = 10000 + powerDec - dataRawDec;
 	              powerInt = powerInt - dataRawInt - 1;
 	            }
 	            
 	            powerSign = POSITIVE_NUM;
 	          }
 	          else  //被减数小于减数，结果为负数
 	          {
 	            if (dataRawDec >= powerDec)
 	            {
 	              powerDec = dataRawDec - powerDec;
 	              powerInt = dataRawInt - powerInt;
 	            }
 	            else
 	            {
 	              powerDec = 10000 + dataRawDec -powerDec;
 	              powerInt = dataRawInt - powerInt - 1; 
 	            }
 	            
 	            powerSign = NEGTIVE_NUM;
 	          }
 	        }
 	      }
      }
      else
      {
        powerInt = 0xEE;

        break;
      }
    }
    
    //调整数据格式
    if (powerInt != 0xEE)
    {
       if (powerDec > 9999)
       {
         do
         {
           powerInt += 1;
           powerDec -= 10000;
         }while (powerDec > 10000);
       }
    
       powerQuantity = dataFormat(&powerInt, &powerDec, 2);
      
       buff[GP_WORK_POWER] = powerQuantity;
       tmpData = hexToBcd(powerInt);
       buff[GP_WORK_POWER+1] = tmpData&0xFF;
       buff[GP_WORK_POWER+2] = tmpData>>8&0xFF;
    }
    else
    {
    	 return FALSE;
    }
    
    return TRUE;
}

/**************************************************
函数名称:ctrlJumpedStatis
功能描述:控制跳闸次数统计
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void ctrlJumpedStatis(INT8U ctrlType,INT8U num)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
   DATE_TIME              tmpTime; 
   
   tmpTime = timeHexToBcd(sysTime);
	 if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
   {
   	 initTeStatisRecord(&terminalStatisRecord);
   }
   
 	 switch(ctrlType)
 	 {
 	 	 case REMOTE_CTRL:   //遥控
 	 	   terminalStatisRecord.remoteCtrlJumpedDay += num;
 	 	   break;

 	 	 case CHARGE_CTRL:
 	 	   terminalStatisRecord.chargeCtrlJumpedDay += num;
 	 	   break;
 	 	 
 	 	 case MONTH_CTRL:
 	 	   terminalStatisRecord.monthCtrlJumpedDay += num;
 	 	 	 break;
 	 	 	 
 	 	 case OBS_CTRL:
 	 	 case TIME_CTRL:
 	 	 case WEEKEND_CTRL:
 	 	 case POWER_CTRL:
 	 	   terminalStatisRecord.powerCtrlJumpedDay += num;
 	 }
 	 
   tmpTime = timeHexToBcd(sysTime);
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
}

#endif