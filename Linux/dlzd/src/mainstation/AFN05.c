/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN05.c
作者：leiyong
版本：0.9
完成日期：2006年6月
描述：主站AFN05(控制命令)处理文件。
函数列表：
     1.AFN05(),主站"控制命令"(AFN05)处理函数
     2.saveData(),保存数据
修改历史：
  01,06-6-26,Leiyong created.
***************************************************/

#include "gdw376-2.h"

#include "convert.h"
#include "workWithMS.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "timeUser.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "reportTask.h"
#include "userInterface.h"
#include "wlModem.h"

#include "AFN05.h"

extern INT8U                statusOfGate;

INT16U offset05;                                   //接收到的帧中的数据单元偏移量(不计数据标识的字节)
INT8U  ctrlCmdWaitTime=0;                          //收到控制命令显示停留时间


/*******************************************************
函数名称:AFN05
功能描述:主站"控制命令"(AFN05)处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN05(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   INT8U     fn, pn;                    //各fn的数据单元标识+数据单元长度(帧中的偏移量)   
   BOOL      (*AFN05Fun[66])(INT8U *p, INT8U n);
   BOOL      ifAck[66];                 //函数AFN05Fun[x]确认或否认
   INT8U     handleItem[54];            //已处理过的项(存储fn值)
   INT8U     handleTail;                //已处理过的项在数组是的存储位置尾部
   BOOL      ackAll,nAckAll;            //全部确认,否认标志
   INT8U     maxCycle;                  //最大循环次数
   INT8U     i;
   
   for(i=0;i<54;i++)
   {
     AFN05Fun[i] = NULL;
   }
  
  #ifndef LIGHTING 
   AFN05Fun[0] = AFN05001;
   AFN05Fun[1] = AFN05002;
   AFN05Fun[8] = AFN05009;
   AFN05Fun[9] = AFN05010;
   AFN05Fun[10] = AFN05011;
   AFN05Fun[11] = AFN05012;
   AFN05Fun[14] = AFN05015;
   AFN05Fun[15] = AFN05016;
   AFN05Fun[16] = AFN05017;
   AFN05Fun[17] = AFN05018;
   AFN05Fun[18] = AFN05019;
   AFN05Fun[19] = AFN05020;
   AFN05Fun[22] = AFN05023;
   AFN05Fun[23] = AFN05024;
   AFN05Fun[24] = AFN05025;
   AFN05Fun[25] = AFN05026;  
   AFN05Fun[26] = AFN05027;
   AFN05Fun[27] = AFN05028;
  #endif    //LIGHTING
  
   AFN05Fun[28] = AFN05029;
   AFN05Fun[29] = AFN05030;    //终端投入运行(重庆规约)
   
   AFN05Fun[30] = AFN05031;
   AFN05Fun[31] = AFN05032;
   AFN05Fun[32] = AFN05033;
   AFN05Fun[33] = AFN05034;
   AFN05Fun[34] = AFN05035;
   AFN05Fun[35] = AFN05036;
   AFN05Fun[36] = AFN05037;

   AFN05Fun[39] = AFN05040;    //终端退出运行(重庆规约)

   AFN05Fun[48] = AFN05049;
   AFN05Fun[49] = AFN05050;
   AFN05Fun[50] = AFN05051;
   AFN05Fun[51] = AFN05052;
   AFN05Fun[52] = AFN05053;    //删除指定通信端口下的全部电表
   AFN05Fun[53] = AFN05054;    //命令启动抄表(重庆规约)

   AFN05Fun[65] = AFN05066;
   
   //判断帧中的PW信息(代码未完成)

   handleTail = 0;
   ackAll = TRUE;
   nAckAll = TRUE;
   maxCycle = 0;

   while ((pDataHead != pDataEnd) && (maxCycle<10))
   {
      maxCycle++;
      
      pn = findFnPn(*pDataHead,*(pDataHead+1),FIND_PN);
      fn = findFnPn(*(pDataHead+2),*(pDataHead+3),FIND_FN);
      
      if (toEliminate==CTRL_JUMP_IN)
      {
      	 //剔除投入时,只响应对时命令(Fn31)和剔除解除(Fn36)
      	 if (fn==31 || fn==36)
      	 {
      	 }
      	 else
      	 {
      	 	  return;
      	 }
      }
         
      if (fn > 0 && fn <= 66 && AFN05Fun[fn-1] != NULL)
      {
         handleItem[handleTail++] = fn;
         ifAck[fn-1] = AFN05Fun[fn-1](pDataHead+4, pn);
         if (ifAck[fn-1]==TRUE)
         {
          #ifdef PLUG_IN_CARRIER_MODULE
           setParaWaitTime = SET_PARA_WAIT_TIME;
           
           //ly,2011-10-12,add,只有技术规范要求的才给声音示警
           if  (fn==1 || fn==2 
          	    || (fn>=9 && fn<=12)
          	     || (fn>=15 && fn<=20)
           	      || (fn>=23 && fn<=25)
           	       || fn==28 
           	        || fn==33
           	         || fn==36
          	  )
           {
    	       setBeeper(BEEPER_ON);         //给出声音示警
    	     }
    	    
 	         alarmLedCtrl(ALARM_LED_ON);          //指示灯亮
 	        #else
           lcdBackLight(LCD_LIGHT_SET_PARA);
       
           ctrlCmdWaitTime = SET_PARA_WAIT_TIME;
           
           //ly,2011-10-12,add,只有技术规范要求的才给声音示警
           if  (fn==1 || fn==2 
          	    || (fn>=9 && fn<=12)
          	     || (fn>=15 && fn<=20)
           	      || (fn>=23 && fn<=25)
           	       || fn==28 
           	        || fn==33
           	         || fn==36
          	  )
           {
    	       setBeeper(BEEPER_ON);         //给出声音示警
    	     }

 	         alarmLedCtrl(ALARM_LED_ON);          //指示灯亮
 	        #endif
           
           lcdLightOn = LCD_LIGHT_ON;
           lcdLightDelay = nextTime(sysTime, 0, SET_PARA_WAIT_TIME);
         }
         
         if (ifAck[fn-1]==TRUE)
         {
           nAckAll = FALSE;
         }
         else
         {
           ackAll = FALSE;
         }
         
         if (fn==31)
         {
           //重新确定下次心跳时间
	         if (dataFrom == DATA_FROM_GPRS)
	         {
 	          #ifdef LIGHTING
 	       
	           //2016-07-04,照明集中器心跳改为以秒为单位
 	           nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
 	      	
 	          #else

	            nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
	            
	          #endif
	          
             lastHeartBeat = sysTime;
             wlModemFlag.heartSuccess = 1;
           }
         }
      }
      else
      {
      	break;
      }
      	
      pDataHead = pDataHead + offset05 + 4;
   }
   
   //确认、否认、逐个确认/否认
   if (ackAll || nAckAll)
   {
     if (ackAll)
     {
        ackOrNack(TRUE,dataFrom);
     }
     if (nAckAll)
     {
        ackOrNack(FALSE,dataFrom);
     }
   }
   else
   {  
      ;//逐项确认或否认(代码未完成)
   }
}

#ifndef LIGHTING

/*******************************************************
函数名称:AFN05001
功能描述:遥控标志置位
          发送遥控返校帧
          计算遥控返校超时时限
          计算遥控结束时间
          
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05001(INT8U *pHandle, INT8U pn)
{ 
   //遥控路数大于最大路数或处于保电状态
   if (pn>CONTROL_OUTPUT || pn<1  || staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {
   	 offset05 = 1;
   	 return FALSE;
   }
   
   remoteCtrlConfig[pn-1].ifRemoteCtrl = 0x00;
   remoteCtrlConfig[pn-1].ifRemoteConfirm = 0x00;
   
   //计算限电起始时间
   if ((*pHandle>>4) == 0)
   {
     remoteCtrlConfig[pn-1].remoteStart = sysTime;
     remoteCtrlConfig[pn-1].alarmStart.year = 0xFF;
   }
   else
   {
     remoteCtrlConfig[pn-1].remoteStart = nextTime(sysTime,*pHandle>>4, 0);
     remoteCtrlConfig[pn-1].alarmStart = sysTime;
   }
   
   remoteCtrlConfig[pn-1].status = 0;
   
   //计算限电结束时间
   if ((*pHandle&0x0F) == 0)
   {
     remoteCtrlConfig[pn-1].remoteEnd.year = 0xFF;
   }
   else
   {
      remoteCtrlConfig[pn-1].remoteEnd = nextTime(remoteCtrlConfig[pn-1].remoteStart, (*pHandle&0xF)*30, 0);
   }

   //过零点自动合闸,根据重庆电科院测试人员所述,要求过零点合闸,ly,2010-01-12,add
   if (remoteCtrlConfig[pn-1].remoteEnd.day != sysTime.day)
   {
      remoteCtrlConfig[pn-1].remoteEnd.hour   = 0x0;
      remoteCtrlConfig[pn-1].remoteEnd.minute = 0x0;
      remoteCtrlConfig[pn-1].remoteEnd.second = 0x0;
   }
   
   #ifdef REMOTE_CTRL_CONFIRM
     //发送遥控返校帧
     
     //计算返校时限
     remoteCtrlConfig[pn-1].confirmTimeOut = nextTime(sysTime,CONFIRM_TIME_OUT, 0);
   #endif
   
   remoteCtrlConfig[pn-1].ifUseRemoteCtrl = CTRL_JUMP_IN;    //本路遥控投入(遥控跳闸),ly,2011-08-08,add
   
   saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);

	 offset05 = 1;
	  
	 return TRUE; 
}

/*******************************************************
函数名称:AFN05002
功能描述:允许合闸
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
BOOL AFN05002(INT8U *pHandle, INT8U pn)
{
    if (pn>CONTROL_OUTPUT || pn<1 || staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
    {
    	 offset05 = 0;
    	 return FALSE;
    }
      	  
	  //合闸事件设置为当前时间
	  remoteCtrlConfig[pn-1].remoteEnd.second = sysTime.second;
	  remoteCtrlConfig[pn-1].remoteEnd.minute = sysTime.minute;
	  remoteCtrlConfig[pn-1].remoteEnd.hour   = sysTime.hour;
	  remoteCtrlConfig[pn-1].remoteEnd.day    = sysTime.day;
	  remoteCtrlConfig[pn-1].remoteEnd.month  = sysTime.month;
	  remoteCtrlConfig[pn-1].remoteEnd.year   = sysTime.year;
	  
    saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
	  
	  offset05 = 0;
	  return TRUE; 
}

/*******************************************************
函数名称:AFN05009
功能描述:时段功控投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05009(INT8U *pHandle, INT8U pn)
{
   offset05 = 2;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {
   	  return FALSE;
   }
   ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_JUMP_IN;                               //时段功控
   periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl = 0x0;
   periodCtrlConfig[pn-1].ctrlPara.prdAlarm  = 0x0;    
   periodCtrlConfig[pn-1].ctrlPara.limitPeriod = *pHandle++;                      //时段功控投入标志
   periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod  = *pHandle;                        //时段功控定值方案号

	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替  
   saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
   
   statusOfGate |= 0x10;    //功控灯亮

	 return TRUE;
}

/*******************************************************
函数名称:AFN05010
功能描述:厂休控投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05010(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
	 
	 ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_JUMP_IN;    //厂休控
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替

   statusOfGate |= 0x10;    //功控灯亮

	 return TRUE;
}

/*******************************************************
函数名称:AFN05011
功能描述:报停控投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05011(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
	 
	 ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_JUMP_IN;    //报停控
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替

   statusOfGate |= 0x10;    //功控灯亮

	 return TRUE;
}

/*******************************************************
函数名称:AFN05012
功能描述:当前功率下浮控投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05012(INT8U *pHandle, INT8U pn)
{
	 offset05 = 8;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }

	 if (ctrlRunStatus[pn-1].ifUsePwrCtrl!=CTRL_JUMP_IN)
	 {
	    ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_JUMP_IN; //当前功率下浮功控
	    powerDownCtrl[pn-1].ifPwrDownCtrl = 0x0;
	    powerDownCtrl[pn-1].pwrDownAlarm = 0x0;
	    
	    powerDownCtrl[pn-1].slipTime = *pHandle++;       //当前功率下浮控定值滑差时间
	    powerDownCtrl[pn-1].floatFactor = *pHandle++;    //当前功率下浮控定值浮动系数
	    powerDownCtrl[pn-1].freezeDelay = *pHandle++;    //控后总加有功功率冻结延时时间
	    
	    powerDownCtrl[pn-1].downCtrlTime      = *pHandle++;//当前功率下浮控的控制时间
	    powerDownCtrl[pn-1].roundAlarmTime[0] = *pHandle++;//当前功率下浮控第1轮告警时间
	    powerDownCtrl[pn-1].roundAlarmTime[1] = *pHandle++;//当前功率下浮控第2轮告警时间
	    powerDownCtrl[pn-1].roundAlarmTime[2] = *pHandle++;//当前功率下浮控第3轮告警时间
	    powerDownCtrl[pn-1].roundAlarmTime[3] = *pHandle++;//当前功率下浮控第4轮告警时间
	  
	    //powerDownCtrl[pn-1].freezeTime = nextTime(sysTime,powerDownCtrl[pn-1].freezeDelay,0);
	    
	    powerDownCtrl[pn-1].freezeTime = nextTime(sysTime,2,30);
	    
	    saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替
	    saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //当前功率下浮功控
	 }
   
   statusOfGate |= 0x10;    //功控灯亮

	 return TRUE;
}

/*******************************************************
函数名称:AFN05015
功能描述:月电控投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05015(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
	 ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_JUMP_IN;  //月电控投入	
	 
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替

   statusOfGate |= 0x20;    //电控灯亮

	 return TRUE;
}

/*******************************************************
函数名称:AFN05016
功能描述:购电控投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05016(INT8U *pHandle, INT8U pn)
{
   INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-26,add
   BOOL      bufHasData;
   DATE_TIME readTime;
	 INT8U     leftPower[12];
   INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];

	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
	 
	 ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_JUMP_IN;    //购电控投入
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替

   readTime = sysTime;
   if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
   {
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
        
        readTime = timeHexToBcd(sysTime);
        if (onlyHasPulsePn==0xaa)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN16:总加组%d只有脉冲测量点\n", pn);
        	}
        	
        	bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime);
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN16::总加组%d混合脉冲测量点\n", pn);
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
              printf("AFN05-FN16::参考总加有功电能量返回TRUE但总加月电能量为0xee,参考置0\n");
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
            printf("AFN05-FN16::参考总加有功电能量返回FALSE,参考置0\n");
          }
          
          //ly,2011-04-16,改成用以下这句
         	leftPower[8]  = 0x0;
          leftPower[9]  = 0x0;
          leftPower[10] = 0x0;
          leftPower[11] = 0x0;
        }
     	  
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
     	    printf("AFN05-FN16::参考总加有功电能量=%02x%02x%02x%02x\n", leftPower[11], leftPower[10], leftPower[9], leftPower[8]);
     	  }

        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
   }
   
   statusOfGate |= 0x20;    //电控灯亮

	 return TRUE;
}

/*******************************************************
函数名称:AFN05017
功能描述:时段功控解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05017(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }

	 ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_RELEASE;    //时段功控
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替
	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05018
功能描述:厂休控解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05018(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_RELEASE;    //厂休控
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替
	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05019
功能描述:报停控解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05019(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_RELEASE;    //报停控解除
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替
	 	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05020
功能描述:功率下浮控解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05020(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_RELEASE;    //功率下浮控解除	  

	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替
	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05023
功能描述:月电控解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05023(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
	 ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_RELEASE;  //月电控解除
	 monthCtrlConfig[pn-1].monthAlarm = 0x00;          //总加组月电控告警置为未进入告警
	 
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替
	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05024
功能描述:购电控解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05024(INT8U *pHandle, INT8U pn)
{
   INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-26,add
   BOOL      bufHasData;
   DATE_TIME readTime;
	 INT8U     leftPower[12];
   INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];

	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_RELEASE;    //购电控解除
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //控制投运状态用FN04代替

   /*
   readTime = sysTime;
   if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
   {
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
        
        readTime = timeHexToBcd(sysTime);
        if (onlyHasPulsePn==0xaa)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN24:总加组%d只有脉冲测量点\n", pn);
        	}
        	
        	bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime);
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN24::总加组%d混合脉冲测量点\n", pn);
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
              printf("AFN05-FN24::参考总加有功电能量返回TRUE但总加月电能量为0xee,参考置0\n");
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
            printf("AFN05-FN24::参考总加有功电能量返回FALSE,参考置0\n");
          }
          
          //ly,2011-04-16,改成用以下这句
         	leftPower[8]  = 0x0;
          leftPower[9]  = 0x0;
          leftPower[10] = 0x0;
          leftPower[11] = 0x0;
        }
     	  
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
     	    printf("AFN05-FN24::参考总加有功电能量=%02x%02x%02x%02x\n", leftPower[11], leftPower[10], leftPower[9], leftPower[8]);
     	  }

        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
   }
   */

	 return TRUE;
}

/*******************************************************
函数名称:AFN05025
功能描述:终端保电投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/ 
BOOL AFN05025(INT8U *pHandle, INT8U pn)
{
	 #ifdef LOAD_CTRL_MODULE
	  int i;
	  if (*pHandle > 48)
	  {
	    offset05 = 1;
	    return FALSE;
	  }
	  
	  staySupportStatus.ifStaySupport = CTRL_JUMP_IN;
	  
	  if (*pHandle == 0)
	  {
	     staySupportStatus.endStaySupport = sysTime;
	     staySupportStatus.ifHold = 0xaa;
	  }
	  else
	  {
	     staySupportStatus.endStaySupport = nextTime(sysTime, (*pHandle)*30, 1);
	     staySupportStatus.ifHold = 0x55;
	  }
	  
	  saveParameter(0x05, 25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));  //终端保电投入
	  
	  //现有的各种控制状态是投入的置为解除
	  for (i = 0; i < 32; i++)
	  {
      //跳闸状态置为合闸
      if (i<CONTROL_OUTPUT)
      {
      	  jumpGate(i+1, 0);
          gateStatus(i+1,0);
      }
      
	    //遥控
	    if (i < 8)
	    {
	      remoteCtrlConfig[i].ifRemoteCtrl = 0x00;
	      remoteCtrlConfig[i].status = 0x0;
        remoteCtrlConfig[i].remoteStart.year = 0xFF;
        remoteCtrlConfig[i].remoteEnd.year = 0xFF;
      }
	    
	    //如果保电投入后,需要自动解除所有的控制,在保电解除后恢复其它控制(不用重新投入控制)的话,执行以下这一段
	    //月电控
	    if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseMthCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //购电控
	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseChgCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //当前功率下浮控
	    if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUsePwrCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //报停控
	    if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseObsCtrl=CTRL_AUTO_RELEASE;
	    }
	  
	    //厂休控
	    if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseWkdCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //时段控
	    if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUsePrdCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //如果保电投入后,需要解除所有的控制,在保电解除后也不恢复其它控制的话(除非重新投入控制),执行以下这一段
	    //月电控
	    if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
	    {
  	     ctrlRunStatus[i].ifUseMthCtrl=CTRL_RELEASE;
	       monthCtrlConfig[i].ifMonthCtrl = 0x0;
	       monthCtrlConfig[i].monthAlarm = 0x0;
	    }
	    
	    //购电控
	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseChgCtrl=CTRL_RELEASE;
	       chargeCtrlConfig[i].ifChargeCtrl = 0x0;
	       chargeCtrlConfig[i].chargeAlarm = 0x0;
	    }
	    
	    //当前功率下浮控
	    if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
	    {
	    	 ctrlRunStatus[i].ifUsePwrCtrl=CTRL_RELEASE;
	       powerDownCtrl[i].ifPwrDownCtrl = 0x0;
	       powerDownCtrl[i].pwrDownAlarm  = 0x0;
	    }
	    
	    //报停控
	    if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
	    {	    
	       ctrlRunStatus[i].ifUseObsCtrl=CTRL_RELEASE;
	       obsCtrlConfig[i].ifObsCtrl = 0x0;
	       obsCtrlConfig[i].obsAlarm = 0x0;
	    }
	  
	    //厂休控
	    if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseWkdCtrl=CTRL_RELEASE;
	       wkdCtrlConfig[i].ifWkdCtrl = 0x0;
	       wkdCtrlConfig[i].wkdAlarm = 0x0;
	    }
	    
	    //时段控
	    if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUsePrdCtrl=CTRL_RELEASE;
	       periodCtrlConfig[i].ctrlPara.ifPrdCtrl = 0x0;
	       periodCtrlConfig[i].ctrlPara.prdAlarm = 0x0;
	    }
	    
	    electCtrlRoundFlag[i].ifJumped = 0x00;              //电控跳闸已跳闸标志全部清除
      
      powerCtrlRoundFlag[pn-1].ifJumped = 0x00;           //功控已跳闸标志全部清除
	  }

	  //告警队列
	  ctrlStatus.numOfAlarm = 0;
	  alarmInQueue(PAUL_ELECTRICITY,0);
	  
	  //将保电参数以及改变了的控制参数一并保存
	  //saveParameter(WRITE_BLOCK_2);
	  
	  offset05 = 1;	  
	  return TRUE;
	#else
	  offset05 = 1;	  
	  return FALSE;	  
	#endif
}

/*******************************************************
函数名称:AFN05026
功能描述:催费告警投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05026(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   reminderFee = CTRL_JUMP_IN;                //催费告警投入
	  
	 saveParameter(0x05, 26, &reminderFee, 1);  //催费告警投入
	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05027
功能描述:响应主站控制命令"允许终端与主站通话(FN27)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05027(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport & 0x03;
   callAndReport = 0x04 | tmpData;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
函数名称:AFN05028
功能描述:剔除投入
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05028(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   toEliminate = CTRL_JUMP_IN;    //剔除投入
	  
	 saveParameter(0x05, 28, &toEliminate, 1);  //剔除投入

	 return TRUE;
}
#endif    //LIGHTING,about line 221


/*******************************************************
函数名称:AFN05029
功能描述:响应主站控制命令"允许终端主动上报(FN29)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05029(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport>>2 & 0x03;
   callAndReport = 0x01 | tmpData<<2;
    
   callAndReport &= 0x0D;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
函数名称:AFN05030
功能描述:响应主站控制命令"终端运行投入(FN30,重庆规约)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05030(INT8U *pHandle, INT8U pn)
{    
   teInRunning = 0x01;

   saveParameter(0x05, 30, &teInRunning, 1);

   if (menuInLayer==0)
   {
   	 defaultMenu();
   }

   offset05 = 0;
   return TRUE;
}

/*******************************************************
函数名称:AFN05031
功能描述:响应主站控制命令"对时命令(FN31)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05031(INT8U *pHandle, INT8U pn)
{	 
   INT8U weekDay;

   dayPowerOnAmount();           //ly,2011-10-11,add
   
   sysTime.second = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++;
   sysTime.minute = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++;
   sysTime.hour = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++;   
   sysTime.day = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++; 
   sysTime.month = ((*pHandle>>4 & 0x1)*10)+(*pHandle & 0xf);
   weekDay = (*pHandle>>5);
   pHandle++;
   sysTime.year = ((*pHandle>>4)*10)+(*pHandle & 0xf);   

   setSystemDateTime(sysTime);

   powerOnStatisTime = sysTime;  //ly,2011-10-11,add

   //刷新title时间显示
   refreshTitleTime();

   //重新设置抄表时间
   reSetCopyTime();
   copyCtrl[4].ifRealBalance = 0;   
   
   //初始化任务信息请求
   if (initReportFlag==0)
   {
	   initReportFlag = 1;
	 }

   offset05 = 6;
   
   //2013-12-05,add
  #ifdef LIGHTING
   carrierFlagSet.broadCast = 1;    //照明集中器请求广播校时CCB
  #endif

   return TRUE;
}

/*******************************************************
函数名称:AFN05032
功能描述:响应主站控制命令"中文信息(FN32)"
         只能保存8条中文信息,每条信息<=200字节
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05032(INT8U *pHandle, INT8U pn)
{
   INT8U i,j,ifFound;
   
   offset05 = *(pHandle+1)+2;
   
  #ifdef LOAD_CTRL_MODULE
   
   ifFound = 0;
   if (chnMessage.numOfMessage==0)
   {
   	  chnMessage.numOfMessage = 1;
   	  i=0;
   }
   else
   {
      for(i=0;i<chnMessage.numOfMessage;i++)
      {
   	    if ((chnMessage.message[i].typeAndNum & 0xf) == (*pHandle&0xf))
   	    {
   	  	   ifFound = 1;
   	   	   break;
   	    }
      }
   
      if (ifFound==0)
      {
   	    chnMessage.numOfMessage++;
   	    if (chnMessage.numOfMessage>8)
   	    {
   	    	 return FALSE;
   	    }
   	  }
   }
   
   chnMessage.message[i].typeAndNum = *pHandle++;
   chnMessage.message[i].len = *pHandle++;
   for(j=0;j<chnMessage.message[i].len;j++)
   {
   	  chnMessage.message[i].chn[j] = *pHandle++;
   }
   chnMessage.message[i].chn[j] = '\0';
   
   chinese(i,0,1);
   
   saveParameter(0x05, 32,(INT8U *)&chnMessage,sizeof(CHN_MESSAGE));
  
   messageTip(1);
   
   return TRUE;
  
  #else
  
   return FALSE;
   
  #endif
}

/*******************************************************
函数名称:AFN05033
功能描述:终端保电解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05033(INT8U *pHandle, INT8U pn)
{
    staySupportStatus.ifStaySupport = CTRL_RELEASE;

	  saveParameter(0x05, 25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));  //终端保电投入/解除
	  	  
	  offset05 = 0;
	  return TRUE;
}

/*******************************************************
函数名称:AFN05034
功能描述:催费告警解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05034(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   reminderFee = CTRL_RELEASE;    //催费告警解除

	 saveParameter(0x05, 26, &reminderFee, 1);  //催费告警投入/解除

	 return TRUE;
}

/*******************************************************
函数名称:AFN05035
功能描述:响应主站控制命令"禁止终端与主站通话(FN35)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05035(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport & 0x03;   
   callAndReport = 0x08 | tmpData;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
函数名称:AFN05036
功能描述:剔除解除
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
BOOL AFN05036(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //保电状态不接受其他控制命令
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   toEliminate = CTRL_RELEASE;    //剔除解除
	  
	 saveParameter(0x05, 28, &toEliminate, 1);  //剔除投入/解除
	  
	 return TRUE;
}

/*******************************************************
函数名称:AFN05037
功能描述:响应主站控制命令"禁止终端主动上报(FN37)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05037(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport>>2 & 0x03;
   callAndReport = 0x02 | tmpData<<2;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
函数名称:AFN05040
功能描述:响应主站控制命令"终端退出运行(FN40,重庆规约)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05040(INT8U *pHandle, INT8U pn)
{    
   teInRunning = 0x00;

   saveParameter(0x05, 30, &teInRunning, 1);
   
   if (menuInLayer==0)
   {
   	 defaultMenu();
   }
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
函数名称:AFN05049
功能描述:响应主站控制命令"命令指定通信端口暂停抄表"(FN49)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05049(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   if ((*pHandle>0 && *pHandle<5) || *pHandle==31)
   {
   	 if (*pHandle<=4)
   	 {
   	   copyCtrl[*pHandle-1].cmdPause = TRUE;  //485端口
   	 }
   	 else
   	 {
   	  #ifdef PLUG_IN_CARRIER_MODULE
   	   if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
   	   {
   	     carrierFlagSet.workStatus = 4;
  	     carrierFlagSet.reStartPause = 2;
         carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 3);

         copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
   	   }
   	   
   	   copyCtrl[4].cmdPause = TRUE;   	 	    //载波端口
   	  #endif
   	 }
   	 
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
函数名称:AFN05050
功能描述:响应主站控制命令"命令指定通信端口恢复抄表"(FN50)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05050(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   
   if ((*pHandle>0 && *pHandle<5) || *pHandle==31)
   {
   	 if (*pHandle<=4)
   	 {
   	    copyCtrl[*pHandle-1].cmdPause = FALSE;  //485端口
   	 }
   	 else
   	 {
   	  #ifdef PLUG_IN_CARRIER_MODULE
   	   if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
   	   {
         carrierFlagSet.workStatus = 6;
         carrierFlagSet.reStartPause = 3;

         carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 3);

         copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
   	   }

   	   copyCtrl[4].cmdPause = FALSE;   	 	    //载波端口
   	  #endif
   	 }
   	 
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
函数名称:AFN05051
功能描述:响应主站控制命令"命令指定通信端口重新抄表"(FN51)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05051(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   if ((*pHandle>0 && *pHandle<5) || *pHandle==31)
   {
   	 if (*pHandle<=4)
   	 {
   	    copyCtrl[*pHandle-1].cmdPause = FALSE;  //485端口
   	 }
   	 else
   	 {
   	  #ifdef PLUG_IN_CARRIER_MODULE 
   	   if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
   	   {
         carrierFlagSet.workStatus = 2;
         carrierFlagSet.reStartPause = 1;

         carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 3);

         copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
   	   }
   	   
   	   copyCtrl[4].cmdPause = FALSE;   	 	    //载波端口
   	  #endif
   	 }
   	 
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
函数名称:AFN05052
功能描述:响应主站控制命令"初始化指定通信端口下的全部中继路由信息"(FN52)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05052(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   if (*pHandle==31)
   {
   	 #ifdef PLUG_IN_CARRIER_MODULE
   	  //载波模块初始化
      carrierFlagSet.init = 2;
      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
     
      return TRUE;
     #else
      return FALSE;
     #endif
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
函数名称:AFN05053
功能描述:响应主站控制命令"删除指定通信端口下的全部电表"(FN53)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05053(INT8U *pHandle, INT8U pn)
{
   char * pSqlStr;
	 
   offset05 = 1;
   
   //if (*pHandle==1 || *pHandle==2 || *pHandle==31)
   //2012-08-23,改成可以删除端口1,2,3,4,31端口的全部电表
  #ifdef PLUG_IN_CARRIER_MODULE 
   if (*pHandle==1 || *pHandle==2 || *pHandle==3 || *pHandle==4 || *pHandle==31)
  #else
   if (*pHandle==1 || *pHandle==2 || *pHandle==3)
  #endif
   {
	   pSqlStr = sqlite3_mprintf("delete from f10_info where acc_port=%d;", *pHandle);
   
     if (sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL)!=SQLITE_OK)
     {
   	   return FALSE;
     }
	   
	   #ifdef PLUG_IN_CARRIER_MODULE
      if (*pHandle==31)
      {
      	carrierFlagSet.msSetAddr = 1;       //主站设置表地址

        carrierFlagSet.msSetWait = nextTime(sysTime, 0, 3);
        if (debugInfo&PRINT_CARRIER_DEBUG)
        {
          printf("等待3秒删除本地通信模块表地址\n");
        }
      }
	   #endif
	   
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
函数名称:AFN05054
功能描述:响应主站控制命令"命令启动搜索抄表"(FN54)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05054(INT8U *pHandle, INT8U pn)
{   
   offset05 = 0;   

  #ifdef PLUG_IN_CARRIER_MODULE
   //清除以前搜索到的表
   while(foundMeterHead!=NULL)
   {
      tmpFound = foundMeterHead;
      foundMeterHead = foundMeterHead->next;
      free(tmpFound);
   }
   foundMeterHead = NULL;
   prevFound = foundMeterHead;
   if (carrierModuleType==RL_WIRELESS)
   {
  	 carrierFlagSet.setupNetwork = 1;
  	 
  	 //组网(最长组网10分钟)
  	 carrierFlagSet.foundStudyTime = nextTime(sysTime, 10, 0);
   }
   else
   {
     if (carrierModuleType==CEPRI_CARRIER)
     {
       carrierFlagSet.foundStudyTime = nextTime(sysTime, 120, 0); //搜表时间,11-07-18,定义电科院模块最长搜表时间为2小时
     }
     else
     {
       carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0); //搜表时间
     }
        
     carrierFlagSet.searchMeter = 1;         //开始搜表标志置1
   }

   if (menuInLayer==0)
   {
   	 defaultMenu();
   }
   
   return TRUE;
  #else
   return FALSE;
  #endif
}

/*******************************************************
函数名称:AFN05066
功能描述:响应主站控制命令"跟踪运行"(FN66)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN05066(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   switch(*pHandle)
   {
   	 case 0:
   	 	 printf("打开与抄表有关的调试信息\r\n");
       debugInfo |= METER_DEBUG;
       #ifdef PLUG_IN_CARRIER_MODULE
        debugInfo |= PRINT_CARRIER_DEBUG;
       #endif
       break;

   	 case 1:
   	 	 printf("打开与645帧有关的调试信息\r\n");
       debugInfo |= PRINT_645_FRAME;
       break;

   	 case 2:
   	 	 printf("打开与计算有关的调试信息\r\n");
       debugInfo |= PRINT_BALANCE_DEBUG;
       break;

   	 case 3:
   	 	 printf("打开与无线Modem有关的调试信息\r\n");
       debugInfo |= WIRELESS_DEBUG;
       break;

   	 case 4:
   	 	 printf("打开与事件有关的调试信息\r\n");
       debugInfo |= PRINT_EVENT_DEBUG;
       break;
     
   	 case 5:
   	 	 printf("打开与数据库有关的调试信息\r\n");
       debugInfo |= PRINT_DATA_BASE;
       break;

   	 case 6:
   	 	 printf("打开与主动上报有关的调试信息\r\n");
       debugInfo |= PRINT_ACTIVE_REPORT;
       break;
   }
   
   return TRUE;
}
