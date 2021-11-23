/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:dlzd.c
作者:leiyong
版本:0.9
完成日期:2009年10月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)main入口文件
函数列表:
  1.入口函数(main)
修改历史:
  01,09-10-10,Leiyong created.
  02,10-10-01,Leiyong添加485表可靠日冻结处理
  03,10-10-01,Leiyong修改日供电时间处理方法,改为每5分钟更新一次供电时间
  04,10-11-24,Leiyong增加-n参数,可强制删除载波模块内表地址及路由,为了防止晓程模块添加不进表地址的问题
  05,14-09-26,Leiyong,增加,每天晚上0点7分清理一次过期数据

***************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include "common.h"
#include "timeUser.h"
#include "lcdGui.h"
#include "convert.h"

#include "ioChannel.h"
#include "hardwareConfig.h"

#include "teRunPara.h"
#include "msSetPara.h"
#include "dataBase.h"
#include "copyMeter.h"
#include "wirelessModem.h"
#include "loadCtrl.h"
#include "msOutput.h"
#include "msInput.h"
#include "userInterface.h"
#include "wlModem.h"
#include "att7022b.h"
#include "reportTask.h"
#include "dataBalance.h"
#include "AFN01.h"
#include "AFN0F.h"

#ifdef PLUG_IN_CARRIER_MODULE
 #include "gdw376-2.h"
#endif

#include <sys/socket.h> 
#include <net/if.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>

extern INT32U avrHeartTime;                //与AVR单片机通信的心跳时间

BOOL          secondChanged=FALSE;         //秒已改变
INT8U         prevSecond;
DATE_TIME     powerOffTime;                //关机时间

extern void *threadOfUpRt(void *arg);

/***************************************************
函数名称:teMaintain
功能描述:维护终端线程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void teMaintain(void *arg)
{
  INT16U    ifWatchdog=0;
  INT8U     buf[20];
  INT16U    offset;
  INT32U    acVolA,acVolB,acVolC;
  ADC_PARA  adcPara;               //直流模拟量配置参数(AFN04-FN81,FN82,FN83)
  INT8U     saveAdcData=0;
 
 #ifdef LIGHTING 
  struct cpAddrLink *tmpCpLink;
 #endif
  
  while(1)
  {
    //1.得到系统时间
    getSystemDateTime(&sysTime);
     
    //2.秒信号的改变判断
    secondChanged = FALSE;
    if (prevSecond!=sysTime.second)
    {
     	secondChanged = TRUE;
    }
    prevSecond = sysTime.second;
     
    //3.检测是否应该复位
    detectReset(secondChanged);
     
    //2012-11-08,由于建立socket时要挂起数秒,按键将无反应,为了不影响按键操作
    //           将这个调用移到主进程中处理
    //4.维护无线链路
    //wlModem();
      
    //5.用户接口
    userInterface(secondChanged);
     
    //7.回复主站(及时帧和主动上报帧)
    replyToMs();
     
   #ifdef WDOG_USE_X_MEGA
    if (secondChanged==TRUE)
    {
      xMegaHeartBeat++;

      if (xMegaHeartBeat>5)
      {
      	xMegaHeartBeat = 0;
      	sendXmegaFrame(CPU_HEART_BEAT, buf, 0);
      	xMegaQueue.inTimeFrameSend = TRUE;
      }
    }
      
    sendToXmega();
   #endif
     
    //8.与秒有关的处理
    if (secondChanged==TRUE)
    {
      //集中器开盖检测
      //if (ioctl(fdOfIoChannel,READ_SWITCH_KH,0))
      //{
      //	 printf("合\n");
      //}
      //else
      //{
      //	 printf("开\n");
      //}
        
      if (sysTime.hour==0 && sysTime.minute==10 && sysTime.second == 0)
      {
        signalReport(0, wlRssi);
        if (wlModemFlag.loginSuccess == 1)
        {
         #ifdef PLUG_IN_CARRIER_MODULE
          #ifdef MENU_FOR_CQ_CANON
           switch(moduleType)
           {
             case GPRS_SIM300C:
             case GPRS_GR64:
             case GPRS_MC52I:
             case GPRS_M590E:
             case GPRS_M72D:
      	       guiDisplay(6, 1, "G", 1);
      	       break;
						 
             case LTE_AIR720H:
      	       guiDisplay(6, 1, "4", 1);
      	       break;
      	          
      	     case CDMA_DTGS800:
      	     case CDMA_CM180:
      	       guiDisplay(6, 1, "C", 1);
      	       break;
      	   }
           lcdRefresh(1,16);
          #else
           showInfo("登录成功!");
          #endif
         #else    //画G上面的框
          guiLine(20,1,20,15,1);
          guiLine(30,1,30,15,1);
          guiLine(20,1,30,1,1);
          guiLine(20,15,30,15,1);
          lcdRefresh(1,16);
         #endif
        }
      }
        
      if (sysTime.hour==23 && sysTime.minute==59 && sysTime.second == 55)
      {
        //记录日累计供电时间
        dayPowerOnAmount();
      }
        
      //每5分钟更新一次供电时间
      if ((sysTime.minute%5)==0 && sysTime.second==0)
      {
        //记录日累计供电时间
        dayPowerOnAmount();
      }
        
      workSecond++;

      if (workSecond==2)
      {
       #ifdef LOAD_CTRL_MODULE
        statusOfGate |= 0x40;  //运行灯亮
       #endif
      
        //通知协处理器自复位
       #ifdef WDOG_USE_X_MEGA
        sendXmegaFrame(ADVISE_RESET_XMEGA, buf, 0);
        xMegaQueue.inTimeFrameSend = TRUE;
         
        sendToXmega();
       #endif
      }
       
      if (workSecond==10)
      {
        //12-1.级联功能
        if (checkIfCascade()==2 || checkIfCascade()==1)
        {
         #ifdef  WDOG_USE_X_MEGA
          buf[0] = 0x02;                         //xMega上的485物理接口2
          buf[1] = cascadeCommPara.ctrlWord;     //端口速率
          sendXmegaFrame(ADVISE_PORT_CASCADE, buf, 2);
             
          printf("通知485-2为级联端口并设置其速率\n");
         #endif
           
          //被级联终端
          if (checkIfCascade()==2)
          {
            printf("级联允许传输应用层数据\n");
     
            wlModemFlag.permitSendData = 1;   //允许传输应用层数据
          }
        }
      }

      //8.1 1类数据任务
      if (initTasking==FALSE)
      {
        if (reportTask1.numOfTask>0)
        {
          activeReport1();
        }
      
        if (fQueue.activeSendPtr!=fQueue.activeTailPtr && fQueue.activeFrameSend==FALSE)
        {
          if (debugInfo&PRINT_ACTIVE_REPORT)
          {
            printf("启动主动上报帧 activeSendPtr=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeTailPtr);
          }
         
          addFrameFlag(TRUE,TRUE);
          fQueue.activeFrameSend = TRUE;  //启动定时器发送TCP数据
        }
      }
       
      if (ioctl(fdOfIoChannel, DETECT_POWER_FAILURE, 0)==IO_LOW)     //停电
      {
        if (ifPowerOff==FALSE)
        {
          powerOffEvent(FALSE);    //停电事件
             
          activeReport3();         //主动上报事件

          dayPowerOnAmount();      //记录日供电时间累计
             
          ifPowerOff = TRUE;
             
          //2013-11-22,根据山东电力公司营销部通知,将电池供电时间改成70秒
         #ifdef SDDL_CSM
          powerOffTime = nextTime(sysTime,0,70);
         #else
          powerOffTime = nextTime(sysTime,0,35);
         #endif
    	 	     
    	  	lcdBackLight(LCD_LIGHT_OFF);            //关闭背景光
             
          //工作时长置为0
          guiLine(1,17,160,160,0);
          guiDisplay(48,64,"终端停电",1);
          lcdRefresh(17,160);
        }
          
      	if (compareTwoTime(powerOffTime, sysTime)==TRUE)
        {
          powerOffTime = nextTime(sysTime,0,10);
             
          ioctl(fdOfIoChannel, SET_BATTERY_ON, IO_LOW);  //断开后备电池
          printf("已关机\n");
        }
      }
      else                                               //有电
      {
    	  if (ifPowerOff==TRUE)
    	  {
    	    //#ifdef MENU_FOR_CQ_CANON
    	    defaultMenu();
    	    //#endif
            
          ioctl(fdOfIoChannel, SET_BATTERY_ON, IO_HIGH); //接通后备电池
  
    	    ifPowerOff = FALSE;
    	    powerOffEvent(TRUE);                           //上电事件
    	      
    	    activeReport3();                               //主动上报事件

          powerOnStatisTime = sysTime;
    	    alarmLedCtrl(ALARM_LED_OFF);
        }
      }
       
      //交采看门狗
      //if ((sysTime.minute%2)==0 && (sysTime.second==7))
      //每隔11秒查看一下交采是否正常,ly,2011-04-18
     #ifdef AC_SAMPLE_DEVICE
      if ((sysTime.second%11)==0)
      {
        //转换交采值
        acVolA = hexDivision((realAcData[8]>>acSamplePara.vAjustTimes),times2(13),1);
        acVolA = (acVolA>>12 & 0xf)*100 + (acVolA>>8 & 0xf)*10 + (acVolA>>4 & 0xf);
        acVolB = hexDivision((realAcData[9]>>acSamplePara.vAjustTimes),times2(13),1);
        acVolB = (acVolB>>12 & 0xf)*100 + (acVolB>>8 & 0xf)*10 + (acVolB>>4 & 0xf);
        acVolC = hexDivision((realAcData[10]>>acSamplePara.vAjustTimes),times2(13),1);
        acVolC = (acVolC>>12 & 0xf)*100 + (acVolC>>8 & 0xf)*10 + (acVolC>>4 & 0xf);
         
        if (debugInfo&PRINT_AC_SAMPLE)
        {
         	printf("%02d-%02d-%02d %02d:%02d:%02d:交采值A相电压=%d,B相电压=%d,C相电压=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, acVolA,acVolB,acVolC);
        }
         
        //比较,如果某相电压在45V以后且在10V以上,没有校表的情况下则认为校表参数不正确,重新下发校表参数
        if (((acVolA>10 && acVolA<45) || (acVolB>10 && acVolB<45) || (acVolC>10 && acVolC<45)) && (readCheckData!=0x55))
        {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
         	  printf("某相电压在45V以后且在10V以上,重新下发校表参数\n");
         	}

       	  resetAtt7022b(TRUE);
       	}
      }
     #endif
       
     #ifdef PLUG_IN_CARRIER_MODULE
      if (sysTime.minute==0 && sysTime.second==0)
      {
        //友迅达每小时校表
        if (carrierModuleType==FC_WIRELESS)
        {
          carrierFlagSet.setDateTime = 0;
            
          //每天晚上9点同步档案信息
          if (sysTime.hour==21)
          {
            carrierFlagSet.synSlaveNode = 1;
          }
        }
      }
        
      //每15分钟保存一次直流模拟量值
      if (sysTime.second==0 && workSecond>15)
      {
        if (selectViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA))==TRUE)
        {
          saveAdcData = 0;
          switch(adcPara.adcFreezeDensity)
          {
            case 1:    //15分钟
              if (sysTime.minute%15==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;
          	  
          	case 2:    //30分钟
              if (sysTime.minute%30==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;

          	case 3:    //60分钟
              if (sysTime.minute==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;

          	case 254:  //5分钟
              if (sysTime.minute%5==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;

          	case 255:  //1分钟
          	  saveAdcData = 1;
          	  break;
          }
          	
          if (saveAdcData==1)
          {
          	buf[0] = hexToBcd(adcData);
          	buf[1] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
          	saveMeterData(1, 0, timeHexToBcd(sysTime), buf, DC_ANALOG, 0, 2);
            if (debugInfo&PRINT_DATA_BASE)
            {
          	  printf("%02d-%02d-%02d %02d:%02d:%02d保存直流模拟量\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
          	}
          }
        }
          
        dcAnalogStatis(adcData);
      }
     #endif
        
      //sqlite3数据库看门狗,每分钟的23秒看一下数据库是否正常
      //ly,2012-04-25,add,因为在现场发现个别设备下发参数成功,但是召测失败
      if (sysTime.second==23 && flagOfClearData!=0x55 && flagOfClearData!=0xaa)
      {
        sqlite3Watch();
      }
        
      //2014-09-26,add
      if (0==sysTime.hour && 7==sysTime.minute && flagOfClearData==0x00)
      {
        flagOfClearData = 0x99;
      }
       
      //Modem心跳看门狗,2015-10-28
      if(31==sysTime.second && wlModemFlag.loginSuccess == TRUE)
      {
       	//2015-11-06,修改为只有在超过30分钟未发心跳时,本看门狗起作用
       	if (compareTwoTime(nextTime(lastHeartBeat, 30, 0), sysTime)==TRUE)
       	{
       	 #ifdef RECORD_LOG
          logRun("Modem:heartBeat time out over over 30 minutes");
         #endif
          if (debugInfo&WIRELESS_DEBUG)
          {
            printf("Modem:heartBeat time out over over 30 minutes\n");
          }
           
          ifReset = TRUE;
       	}
      }
			
		 #ifdef LIGHTING
	    //2016-11-21,Add,关联照度传感值心跳
		  if (lcHeartBeat<300)
			{
				lcHeartBeat++;
				if (lcHeartBeat>=300)    //已有5分钟没收到关联的照度值了
				{
    			//线路控制点
      	  tmpCpLink = xlcLink;
          while(tmpCpLink!=NULL)
          {
    			  if (tmpCpLink->lcDetectOnOff<=1)
    			  {
      			  tmpCpLink->lcOnOff = 0xfe;
      			  tmpCpLink->lcDetectOnOff = 0xfe;
      			  
      			  if (debugInfo&METER_DEBUG)
              {
      	        printf(
      	               "%02d-%02d-%02d %02d:%02d:%02d,长时间未收到关联光照度值,置线路控制点%02x%02x%02x%02x%02x%02x的lcDetectOnOff状态为未知\n",
      	                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
      	                  tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
      	              );
      	      }
      	    }
						
						tmpCpLink = tmpCpLink->next;
    			}
				}
			}
		   
			//2016-11-24,Add,单灯、线路控制点状态更新心跳,10分钟发一次
			if (statusHeartBeat<600)
			{
				statusHeartBeat++;
			}
	   #endif

    }
     
    //看门狗
   #ifdef PLUG_IN_CARRIER_MODULE
    ifWatchdog++;
    if (ifWatchdog==0x1)
    {
   	  ioctl(fdOfIoChannel,SET_WATCH_DOG,1);
    }
    if (ifWatchdog==0x2)
    {
   	  ifWatchdog = 0x0;
   	  ioctl(fdOfIoChannel,SET_WATCH_DOG,0);
   	}
   #endif

    usleep(1000);
  }
}

/**************************************************
函数名称:main
功能描述:电力终端(负控终端、集中器,AT91SAM9260处理器)main函数
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：状态
***************************************************/
int main(int argc,char *argv[])
{
  DATE_TIME     powerOffTime;        //关机时间
  INT8U         prevSecond;
  INT8U         i;
  INT32U        j,k;
  pthread_t     id;
  int           ret;
  int           kkk;
  INT8U         tmpData;  
  INT8U         threadPara;
  INT8U         threadParax[11];     //线程参数
  DATE_TIME     nowTime;  
  INT8U         buf[3];              //for test acSample ly,2010-01-27
  INT8U         port;
  pthread_attr_t     attr;
  struct sched_param param;
  int    newprio=20;

  struct sockaddr_in addr;
  int                socketOfSendUdp;

  debugInfo = 0;
  debugInfox = 0;
  if (argc>1)
  {
  	for(i=1;i<argc;i++)
  	{
  	 	if (argv[i][0]=='-' && (argv[i][1]=='h' || argv[i][1]=='H'))
  	 	{
        printf("Usage:dlzd [-h] [-a] [-b] [-c] [-d] [-e] [-l] [-m] [-s] [-u] [-w] [-x] [-ac]\n");
        printf("    -h  显示帮助信息\n");
        printf("    -a  显示与主动上报有关的调试信息\n");
        printf("    -b  显示与结算有关的调试信息\n");
        printf("    -c  显示与本地通信端口(载波/无线)有关的调试信息\n");
        printf("    -d  显示与本地数据库有关的调试信息\n");
        printf("    -e  显示与事件有关的调试信息\n");
        printf("    -l  显示与负荷控制有关的调试信息\n");
        printf("    -n  强制删除载波模块数据(表地址及路由信息)\n");
        printf("    -m  显示与抄表有关的调试信息\n");
        printf("    -s  显示与ESAM芯片有关的调试信息\n");
        printf("    -u  显示与远程升级有关的调试信息\n");
        printf("    -w  显示与远程通信模块有关的调试信息\n");
        printf("    -x  显示与协处理器有关的调试信息\n");
        printf("    -ac 显示与交流采样有关的调试信息\n");
        printf("    -p  显示与脉冲采集有关的调试信息\n");
        printf("    -6  显示485接口的抄表645帧\n");
        printf("    -v  显示与交采示值有关的调试信息\n");
        printf("    -3  校交采数据从串口返回,否则就是从MODEM返回\n");
        printf("    -r  进程看门狗重新运行主程序\n");
        printf("    -t  显示与以太网抄表相关的调试信息\n");
        
        exit(0);
  	 	}
  	 	
  	 	//与主动上报有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='a' || argv[i][1]=='A'))
  	 	{
  	 		 debugInfo |= PRINT_ACTIVE_REPORT;
  	 	}

  	 	//与结算有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='b' || argv[i][1]=='B'))
  	 	{
  	 		 debugInfo |= PRINT_BALANCE_DEBUG;
  	 	}

  	 	//与载波有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='c' || argv[i][1]=='C'))
  	 	{
  	 		 debugInfo |= PRINT_CARRIER_DEBUG;
  	 	}
  	 	
  	 	//与数据库有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='d' || argv[i][1]=='D'))
  	 	{
  	 		 debugInfo |= PRINT_DATA_BASE;
  	 	}

  	 	//与事件有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='e' || argv[i][1]=='E'))
  	 	{
  	 		 debugInfo |= PRINT_EVENT_DEBUG;
  	 	}

  	 	//与负荷控制有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='l' || argv[i][1]=='L'))
  	 	{
  	 		 debugInfo |= PRINT_LOAD_CTRL_DEBUG;
  	 	}

  	 	//与抄表有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='m' || argv[i][1]=='M'))
  	 	{
  	 		 debugInfo |= METER_DEBUG;
  	 	}

  	 	//与ESAM芯片有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='s' || argv[i][1]=='S'))
  	 	{
  	 		 debugInfo |= ESAM_DEBUG;
  	 	}

  	 	//与远程升级有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='u' || argv[i][1]=='U'))
  	 	{
  	 		 debugInfo |= PRINT_UPGRADE_DEBUG;
  	 	}

  	 	//与无线Modem有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='w' || argv[i][1]=='W'))
  	 	{
  	 		 debugInfo |= WIRELESS_DEBUG;
  	 	}

  	 	//与xMega有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='x' || argv[i][1]=='X'))
  	 	{
  	 		 debugInfo |= PRINT_XMEGA_DEBUG;
  	 	}

  	 	//与交采有关的调试信息
  	 	if (argv[i][0]=='-' && ((argv[i][1]=='a' && argv[i][2]=='c') || (argv[i][1]=='A' && argv[i][2]=='C')))
  	 	{
  	 		 debugInfo |= PRINT_AC_SAMPLE;
  	 	}

  	 	//与脉冲采集有关的调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='p' || argv[i][1]=='P'))
  	 	{
  	 		 debugInfo |= PRINT_PULSE_DEBUG;
  	 	}

  	 	//抄表645帧
  	 	if (argv[i][0]=='-' && argv[i][1]=='6')
  	 	{
  	 		debugInfo |= PRINT_645_FRAME;
        
       #ifdef SUPPORT_ETH_COPY
  	 		monitorMp = 0;
  	 		if (argv[i][2]=='m' || argv[i][2]=='M')
  	 	  {
  	 	    switch (strlen(argv[i]))
  	 	    {
  	 	    	case 4:
  	 	    		monitorMp = argv[i][3]-0x30;
  	 	    		break;

  	 	    	case 5:
  	 	    		monitorMp = (argv[i][3]-0x30)*10 + argv[i][4]-0x30;
  	 	    		break;

  	 	    	case 6:
  	 	    		monitorMp = (argv[i][3]-0x30)*100 + (argv[i][4]-0x30)*10 + argv[i][5]-0x30;
  	 	    		break;

  	 	    	case 7:
  	 	    		monitorMp = (argv[i][3]-0x30)*1000 + (argv[i][4]-0x30)*100 + (argv[i][5]-0x30)*10 + argv[i][6]-0x30;
  	 	    		break;
  	 	    }
  	 	  }
  	 	 #endif
  	 	}

  	 	//交采示值相关
  	 	if (argv[i][0]=='-' && argv[i][1]=='v')
  	 	{
  	 		debugInfo |= PRINT_AC_VISION;
  	 	}
  	 	
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	 	//强制删除载波模块数据(表地址及路由信息)
  	 	if (argv[i][0]=='-' && (argv[i][1]=='n' || argv[i][1]=='N'))
  	 	{
  	 		debugInfo |= DELETE_LOCAL_MODULE;
  	 		carrierFlagSet.forceClearData = 1;
  	 	}
  	 #endif

  	 	//交采示值相关
  	 	if (argv[i][0]=='-' && argv[i][1]=='3')
  	 	{
  	 		debugInfox |= 0x01;
  	 	}

  	 	//进程看门狗重启主应用程序
  	 	if (argv[i][0]=='-' && (argv[i][1]=='r' || argv[i][1]=='R'))
  	 	{
  	 		debugInfox |= WATCH_RESTART;
  	 	}

  	 	//以太网抄表相关调试信息
  	 	if (argv[i][0]=='-' && (argv[i][1]=='t' || argv[i][1]=='T'))
  	 	{
  	 		debugInfox |= ETH_COPY_METER;
  	 	}
  	}
  }
  
  printf("终端启动...\n");
  
  //debugInfo |= PRINT_CARRIER_DEBUG;
  //debugInfo |= METER_DEBUG;
  //debugInfo |= PRINT_PULSE_DEBUG;        //ly,2011-07-29
  //debugInfo |= PRINT_LOAD_CTRL_DEBUG;    //ly,2011-07-29
  //debugInfo |= PRINT_BALANCE_DEBUG;      //ly,2011-07-29
  //debugInfo |= PRINT_DATA_BASE;          //ly,2011-08-15
  //debugInfo |= PRINT_UPGRADE_DEBUG;      //ly,2011-08-15

  //0.取得系统时间,初始化统计时间
  getSystemDateTime(&sysTime);
  powerOnStatisTime = backTime(sysTime, 0, 0, 0, 0, 15); //终端上电时间(由于系统启动要15到20秒左右)
  lcdLightDelay = nextTime(sysTime, 0, 30);              //LCD背光延时

  //1.初始化数据库
  initDataBase();

  //2.载入参数
  loadParameter();
  
  //3.初始化硬件(一定在载入参数后初始化工作,2012-11-08)
  initHardware();
  
  if (debugInfo&ESAM_DEBUG)
  {
  	ioctl(fdOfIoChannel, ESAM_DEBUG_INFO, 1);
  }
  else
  {
  	ioctl(fdOfIoChannel, ESAM_DEBUG_INFO, 0);
  }

  //3-1-2.无线Modem复位次数
  wlModemFlag.numOfWlReset  = 0;
  wlModemFlag.useMainIp     = 1;
  wlModemFlag.lastIpSuccess = 0;
  wlModemFlag.ethRecvThread = 0;
  wlModemFlag.waitModemRet  = nextTime(sysTime, WAIT_MODEM_MINUTES, 0);
  wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add

  //3-2.无线Modem断电
  if (bakModuleType==MODEM_PPP)  //2012-11-08,添加此判断
  {
  	waitTimeOut = nextTime(sysTime, 3, 0);
  	
  	system("/bin/ppp-off");    //关闭ppp0
  }
  else
  {
    wlModemPowerOnOff(0);
  }

  //4.初始化LCD
  lcdInit();

  if ((debugInfox&WATCH_RESTART)==0x00)
  {
   #ifdef PLUG_IN_CARRIER_MODULE
    guiStart(8,50,1);
   #else
    guiStart(32,50,1);
   #endif
  }
  
  //6.初始化发送队列
  initSendQueue();

  #ifdef PULSE_GATHER
    fillPulseVar(1);
  #endif
  
	//7.复位ATT7022B	
	//ly,2010-10-28,经在科陆测试台测试发现,如果开机时只复位一次的话,att7022b可能复位不太正常,因此用两次复位及发送校验值
 #ifdef AC_SAMPLE_DEVICE	
	resetAtt7022b(TRUE);
	resetAtt7022b(TRUE);
 #endif

  //8.初始化任务信息请求
	initReportFlag = 1;
  
  
  //9.创建线程
  //9-1.创建抄485表线程
  threadPara = 0x00;
  ret=pthread_create(&id,NULL,copyMeter,&threadPara);
  if(ret!=0)
  {
    printf ("CreatecopyMeter pthread error!\n");
    return 1;
  }

 #ifndef JZQ_CTRL_BOARD_V_1_4
  #ifndef TE_CTRL_BOARD_V_1_3
   //9-2.创建485接口1接收线程
   threadPara = 0x01;
   ret=pthread_create(&id,NULL,threadOf485x1Received,&threadPara);
   if(ret!=0)
   {
     printf ("Create copy meter receive(port 1) pthread error!\n");
     return 1;
   }

   //9-3.创建485接口2接收线程
   threadPara = 0x02;
   ret=pthread_create(&id,NULL,threadOf485x2Received,&threadPara);
   if(ret!=0)
   {
     printf ("Create copy meter receive(port 2) pthread error!\n");
     return 1;
   }

  #endif
 #endif
  
  //9-4.创建本地接口与主站通信接收线程
  threadPara = 0x03;
  ret=pthread_create(&id,NULL,threadOfmsLocalReceive,&threadPara);
  if(ret!=0)
  {
    printf ("Create ms local receive pthread error!\n");
    return 1;
  }

  //9-5.创建无线Modem串口接收线程
  threadPara = 0x04;
  ret=pthread_create(&id, NULL, threadOfTtys0Receive, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread wireless receive error!\n");
    return 1;
  }

 #ifdef PLUG_IN_CARRIER_MODULE 
  //9-6.创建载波数据接收线程
  threadPara= 0x05;
  ret=pthread_create(&id, NULL, threadOfCarrierReceive, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread carrier receive error!\n");
    return 1;
  }
 #endif
  
  //9-7.创建清理数据线程
  //ly,2011-10-14,将主站要求清除数据/参数改到这里个线程里,不用动态创建线程
  flagOfClearData = 0x99;
  pthread_attr_init(&attr);
  pthread_attr_getschedparam(&attr, &param);
  param.sched_priority=1;
  pthread_attr_setschedparam(&attr, &param);
  threadPara= 0x07;
  ret=pthread_create(&id, &attr, threadOfClearData, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread clear data error!\n");
    return 1;
  }

  //9-8.创建检测开关量及脉冲线程
  pthread_attr_init(&attr);
  pthread_attr_getschedparam(&attr, &param);
  param.sched_priority = 88;    //优先40,目前最高,2010-05-27,ly
  pthread_attr_setschedparam(&attr, &param);
  threadPara= 0x08;
  ret=pthread_create(&id,&attr,detectSwitchPulse,&threadPara);
  if(ret!=0)
  {
     printf ("Create pthread detectSwitchPulse error!\n");
     return 1;
  }

  //9-9.创建维护终端线程
  ret=pthread_create(&id, NULL, teMaintain, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread teMaintain error!\n");
    return 1;
  }
  
  //9-10.数据结算线程
  //ly,2011-10-14,改到这里,不用动态创建线程  
  ret=pthread_create(&id, NULL, dataProcRealPoint, &threadParax);
  if(ret!=0)
  {
    printf("创建结算线程失败!\n");
    
    return 1;
  }
  
  //9-11.二类数据任务
  //ly,2011-10-15,改到这里,不用动态创建线程  
  ret=pthread_create(&id,NULL,threadOfReport2,&threadParax);
  if(ret!=0)
  {
    printf("创建二类数据任务线程失败!\n");
    
    return 1;
  }
  
  //9-12.专变III型创建控制线程
 #ifndef PLUG_IN_CARRIER_MODULE
  ret=pthread_create(&id, NULL, threadOfCtrl, &threadPara);
  if(ret!=0)
  {
    printf("Create pthread threadOfCtrl error!\n");
    return 1;
  }

  //所有路为合闸状态
  for(i=1; i<CONTROL_OUTPUT+1; i++)
  {
    gateJumpTime[i] = 0x4;
  }
 #endif



  addr.sin_family = AF_INET;
  addr.sin_port = htons(65431);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (addr.sin_addr.s_addr == INADDR_NONE)
  {
    printf("Incorrect ip address!");
    close(socketOfUdp);
  }
  
  if ((socketOfUdp = socket(AF_INET, SOCK_DGRAM, 0))<0)
  {
    perror("socket");
  }

  if (bind(socketOfUdp, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
  }

  //9-13.创建UDP接收线程
  if(pthread_create(&id, NULL, threadOfRecvUdpData, &threadPara)!=0)
  {
    printf ("Create pthread threadOfRecvUdpData error!\n");
    return 1;
  }



 #ifdef SUPPORT_ETH_COPY
  //9-14.创建以太网抄表服务线程
  threadPara= 0x14;
  ret=pthread_create(&id, NULL, threadOfEthCopyServer, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread threadOfEthCopyServer error!\n");
    
    return 1;
  }

  //9-15.创建读取以太网抄表TCP数据线程
  threadPara = 0x15;
  if(pthread_create(&id, NULL, threadOfEthCopyDataRead, &threadPara)!=0)
  {
    printf("Create pthread threadOfTcpDataRecv error!\n");
  }

  //9-16.创建以太网抄表线程
  threadPara= 0x16;
  ret=pthread_create(&id, NULL, threadOfEthCopyMeter, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread threadOfEthCopyMeter error!\n");
    
    return 1;
  }
 #endif



  //10.记录已上电及复位次数
  if (powerOnOffRecord.powerOnOrOff==0x11 || powerOnOffRecord.powerOnOrOff==0x22)
  {
   	if (powerOnOffRecord.powerOnOrOff==0x22)    //有停电时间记录,记录系统上电事件记录
   	{
      powerOffEvent(TRUE);
       
      activeReport3();    //主动上报事件
   	}
  }
  else    //如果没有停/上电时间记录,记录当前时间为上电时间
  {
   	powerOnOffRecord.powerOnOrOff = 0x11;
   	powerOnOffRecord.powerOnOffTime = sysTime;

    //终端的参数记录为AFN=0x66
    saveParameter(88, 1,(INT8U *)&powerOnOffRecord,sizeof(POWER_ON_OFF));
  }  
  teResetReport();    //记录终端复位次数
  
  logRun("terminal starting...");

  //11.集中器检测是否用以太网登录主站
 #ifdef PLUG_IN_CARRIER_MODULE 
  if (teIpAndPort.ethIfLoginMs==0x55)
  {
   #ifdef JZQ_CTRL_BOARD_V_1_4
   	gatherModuleType = 2;
   #endif

   #ifdef TE_CTRL_BOARD_V_1_3
   	gatherModuleType = 2;
   #endif
   	 
    moduleType = ETHERNET;
   	 
   	operateModem = 1;    //2012-11-8
   	 
   	bakModuleType = 0;   //2013-04-28
  }
 #endif
  
  //12.级联处理
  //12-1.被级联终端
  if (checkIfCascade()==2)
  {
  	printf("本终端为被级联终端\n");

	 #ifdef JZQ_CTRL_BOARD_V_1_4
	  gatherModuleType = 2;
	 #endif

	 #ifdef TE_CTRL_BOARD_V_1_3
	  gatherModuleType = 2;
	 #endif
	 
	  moduleType = CASCADE_TE;
  }
  
  //13.开机显示
  if (debugInfox&WATCH_RESTART)
  {
  	defaultMenu();
  }
  else
  {
    startDisplay();
  }

  //14.主进程处理
  while(1)
  {
    if (initReportFlag==1)
    {
     	initReportFlag = 2;

     	initReportTask(NULL);
    }
     
   #ifdef WDOG_USE_X_MEGA
    if (gatherModuleType==0)
    {
      sendXmegaFrame(GATHER_IO_DATA, &i, 0);
      sleep(2);
    }

    if (gatherModuleType==1)
    {
      gatherModuleType=2;

      if (bakModuleType!=MODEM_PPP)    //2012-11-14,add
      {
        if (moduleType==GPRS_MC52I)
        {
          uart0Config(2);  //波特率设置为57600bps
          printf("设置为57600\n");
        }
        else
        {
          uart0Config(0);  //波特率设置为115200bps
          //printf("设置为115200");
        }
      	 
        if (moduleType==GPRS_MC52I)
        {
          resetWlModem(1);
        }
      }
         
      //以太网
      if (moduleType==ETHERNET)
      {
				fdOfModem=NULL;    //2017-8-23,add

	    	//打开无线模块电源
      	wlModemPowerOnOff(1);
      }
    }
   #endif
     
    //2012-11-7,modify,通信socket改成非阻塞式后,就不用每次都创建线程了
    //if (wlModemFlag.ethRecvThread==0 && moduleType==ETHERNET && wlModemFlag.permitSendData==1)
    //{
    //	 printf("创建以太网接收线程\n");
    	 
    //	 wlModemFlag.ethRecvThread = 1;
     	 
    //  threadPara= 0x04;
    //  ret=pthread_create(&id,NULL,threadOfTtys0Receive,&threadPara);
    //  if(ret!=0)
    //  {
    //    printf ("Create pthread wireless receive error!\n");
    //    return 1;
    //  }
    //}
    
   #ifdef PLUG_IN_CARRIER_MODULE
    for(port=0;port<4;port++)
   #else
    for(port=0;port<NUM_OF_COPY_METER;port++)
   #endif
    {
      if (((sysTime.second>2 && sysTime.second<10) || (sysTime.second>=15 && sysTime.second<17)) && (copyCtrl[port].ifRealBalance==0) && (copyCtrl[port].thisMinuteProcess==FALSE))
      {
       	copyCtrl[port].thisMinuteProcess = TRUE;
       	
       	if (debugInfo&PRINT_BALANCE_DEBUG)
       	{
       	  printf("%02d-%02d-%02d %02d:%02d:%02d,主进程:处理端口%d越限持续时间到期开始\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port+1);
       	}
        
        processOverLimit(port);
        
        activeReport3();  //主动上报事件

       	if (debugInfo&PRINT_BALANCE_DEBUG)
       	{
       	  printf("%02d-%02d-%02d %02d:%02d:%02d,主进程:处理端口%d越限持续时间到期结束\n",
       	   sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port+1);
       	}
      }
       
      if (((sysTime.second>=10 && sysTime.second<15) || (sysTime.second>=17 && sysTime.second<20))  && (copyCtrl[port].thisMinuteProcess==TRUE))
      {
      	 copyCtrl[port].thisMinuteProcess=FALSE;
      }
    }
     
   #ifdef PLUG_IN_CARRIER_MODULE
    if (upRtFlag==1)
    {
     	upRtFlag = 2;
     	 
      threadPara= 0x00;
      ret=pthread_create(&id,NULL,threadOfUpRt,&threadPara);
      if(ret!=0)
      {
        printf ("Create pthread update router error!\n");
        return 1;
      }
    }
   #endif
     
    //2012-11-08,由于建立socket时要挂起数秒,按键将无反应,为了不影响按键操作
    //           将这个调用移到主进程中处理
    //4.维护无线链路
    wlModem();

    usleep(1000);
  }
  
  close(fdOfModem);
  close(fdOfttyS2);
  close(fdOfttyS3);
  close(fpOfUpgrade);
  printf("终端程序退出\n");
  
  //关闭数据库
  sqlite3_close(sqlite3Db);
}

