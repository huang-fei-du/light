/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
文件名：wlModem.c
作者：leiyong
版本：0.9
完成日期：2010年1月
描述：无线Modem相关处理文件。
函数列表：
     1.
修改历史：
  01,10-01-24,Leiyong created.
  02,10-02-10,Leiyong,修改,为了避免因登录不上造成的复位终端引起载波抄表不正常,
              更改处理方式为只有处理器与Modem通信不正常时才复位终端,否则复位Modem模块
  03,12-08-01,M590E添加开关机流程处理            
***************************************************/
#include <unistd.h>

//2012-11-8,add------------
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>    
//2012-11-8,add------------

#include "wirelessModem.h"

#include "ioChannel.h"
#include "hardwareConfig.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "msOutput.h"
#include "msInput.h"
#include "userInterface.h"

#include "wlModem.h"

//变量
WL_MODEM_FLAG wlModemFlag;       //无线Modem控制标志
DATE_TIME     lastHeartBeat;     //上次心跳时间
DATE_TIME     nextHeartBeatTime; //下一次心跳时间
DATE_TIME     waitTimeOut;       //等待超时时间(发登录帧及心跳帧发送后的超时时间,如果超时要做相就的动作)
DATE_TIME     wlPowerTime;       //无线Modem开机时间
INT32U        wlLocalIpAddr;     //无线Modem本终端IP地址(GPRS/CDMA等获得的IP地址)

char          tmpWlStr[200];

INT8U         wlRecvBuf[2048];   //接收MODEM缓存
INT8U         bakWlBuf[2048];
INT16U        lenOfWlRecv = 0;   //MODEM接收长度

INT16U        bakLenOfWlRecv = 0;//备份modem接收长度
INT8U         countOfRecv;       //查看modem计数

INT8U         operateModem=1;    //可以操作modem? 2012-11-7
INT8U         dialerOk=0;        //dialer拨号Ok?  2012-11-7

void wlReceive(INT8U *buf, INT16U len);

/*******************************************************
函数名称:wlModem
功能描述:维护无线链路
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void wlModem(void)
{
  INT8U         wlRet;
  pthread_t     id;
  int           ret;
  INT8U         threadPara;
  INT8U         i;
  INT8U         tmpBuf[40];
   
  //2012-11-08,如果modem采用ppp拨号则需要等到dialer请求时才操作模块
  if (operateModem==0 && bakModuleType==MODEM_PPP)
  {
  	if (debugInfo&WIRELESS_DEBUG)
  	{
  	  printf("MODEM_PPP退出\n");
  	}
  	 
  	return;
  }
   
  if (bakLenOfWlRecv!=lenOfWlRecv)
  {
   	bakLenOfWlRecv = lenOfWlRecv;
   	countOfRecv = 0;
  }
  else
  {
   	countOfRecv++;
   	if (countOfRecv>5)
   	{
 	   	countOfRecv = 0;
 	   
 	   	memcpy(bakWlBuf, wlRecvBuf, lenOfWlRecv);
 	   	lenOfWlRecv = 0;
 	   
 	   	if (bakLenOfWlRecv!=0)
 	   	{
 	    	 wlReceive(bakWlBuf, bakLenOfWlRecv);
 	   
 	     	//if (ipPermit==FALSE)
 	     	//{
         	//  wlFlagsSet.atFrame = FRAME_NONE;
         	//  tailOfWireFrame = 0;
 	 	 		//}
 	   	}
   	}
  }

 	if (moduleType==GPRS_SIM300C 
 	   || moduleType==CDMA_DTGS800 
 	    || moduleType==GPRS_GR64 
 	     || moduleType==GPRS_MC52I 
 	      || moduleType==ETHERNET 
 	       || moduleType==GPRS_M590E 
 	        || moduleType==CDMA_CM180   	         
 	         || moduleType==CASCADE_TE   //被级联终端
 	          || moduleType==GPRS_M72D
 	           || moduleType==LTE_AIR720H
 	            || bakModuleType==MODEM_PPP
 	  )
  {
   	;
  }
  else
  {
   	//如果没有检测到模块或是不识别的模块
    if (compareTwoTime(wlModemFlag.waitModemRet, sysTime))
    {
      //被级联终端
      if (checkIfCascade()==2)
      {
        wlModemFlag.waitModemRet = nextTime(sysTime, WAIT_MODEM_MINUTES, 0);
      }
      else
      {
   	  	if (debugInfo&WIRELESS_DEBUG)
  	  	{
  	 			printf("未识到模块,重新开始\n");
  	  	}
  	 
  	  	logRun("未识到模块,重新开始");
        	
        setModemSoPara();                     //设置Modem库初始参数
  
        wlModemPowerOnOff(0);                 //重启远程Modem
          
       #ifdef PLUG_IN_CARRIER_MODULE 
        if (teIpAndPort.ethIfLoginMs==0x55)
        {
       	 #ifdef JZQ_CTRL_BOARD_V_1_4
       	  gatherModuleType = 2;
       	 #endif
       	 
       	  moduleType = ETHERNET;
        }
        else
        {
         #ifdef WDOG_USE_X_MEGA
          gatherModuleType = 0;
         #endif
        }
       #else
        #ifdef WDOG_USE_X_MEGA
         gatherModuleType = 0;
        #endif
       #endif
      }
    }
   	  
   	return;
  }
      
  if (wlModemFlag.power>0)
  {
      if (compareTwoTime(wlPowerTime, sysTime))
      {
      	switch (wlModemFlag.power)
      	{
      	 	case 1:
      	 	  if(moduleType==GPRS_MC52I)
      	 	  {
      	 	    resetWlModem(1);
      	 	  }
      	 	  else
      	 	  {
      	 	    resetWlModem(0);
      	 	  }

      	 	  wlModemFlag.power = 0;

						//2018-6-11,
						if (moduleType==ETHERNET)
						{
      	 	  	waitTimeOut = nextTime(sysTime,1,30); //超时时间为1分30秒
						}
						else
						{
      	 	  	waitTimeOut = nextTime(sysTime, 3, 0);//超时时间为3分
						}

						wlModemFlag.sendLoginFrame = 0;       //还没有发送登录帧
     	  	  wlModemFlag.permitSendData = 0;       //不允许传输应用层数据
     	  	  wlModemFlag.loginSuccess   = 0;       //应用层登录主站是否成功置未成功
     	  	  
     	  	  wlModemFlag.numOfWlReset++;           //复位次数加1
            
            if (debugInfo&WIRELESS_DEBUG)
            {
            	 printf("复位远程Modem,复位次数=%d\n",wlModemFlag.numOfWlReset);
            }
            sprintf(tmpWlStr,"复位远程Modem,复位次数=%d",wlModemFlag.numOfWlReset);
            
            #ifdef RECORD_LOG
             //logRun(tmpWlStr);
            #endif
            
            //没有登录成功且复位次数余3等于0,使用备用IP地址,否则使用主用IP地址
            if (wlModemFlag.lastIpSuccess==0 && ((wlModemFlag.numOfWlReset%3)==0))
            {
              wlModemFlag.useMainIp = 0;
            }
            else
            {
              wlModemFlag.useMainIp = 1;
            }

     	  	  wlModemFlag.lastIpSuccess = 0;        //上次IP登录置为未成功
            
            if (moduleType!=CDMA_CM180)
            {
              setModemSoPara();                   //设置Modem库初始参数
            }
            
            //被级联终端
            if (checkIfCascade()==2 && workSecond>10)
            {
            	printf("级联允许传输应用层数据\n");
     
              wlModemFlag.permitSendData = 1;     //允许传输应用层数据
            }
            
            if (moduleType==MODEM_PPP)
            {
              //开机完成
              tmpBuf[0]=1;
              sendToDialer(1, tmpBuf, 1);
              
              //发送APN
              memcpy(tmpBuf, ipAndPort.apn, 16);
              sendToDialer(8, tmpBuf, 16);
              
              //发送VPN
              memcpy(tmpBuf, vpn.vpnName, 32);
              sendToDialer(9, tmpBuf, 32);
              memcpy(tmpBuf, vpn.vpnPassword, 32);
              sendToDialer(10, tmpBuf, 16);
            }
            break;
      	 
      	  case 2:   //无线模块处于断电状态
      	 	  //2012-07-27,M590E在模块上电前,拉低ON/OFF管脚
      	 	  if (moduleType==GPRS_M590E)
      	 	  {
          	  ioctl(fdOfIoChannel, WIRELESS_IGT, 1);
               
          	  if (debugInfo&WIRELESS_DEBUG)
          	  {
          	    printf("M590E加电前置开机键值为1\n");
          	  }
      	 	  }
      	 	  
      	 	  //打开无线模块电源
      	 	  wlModemPowerOnOff(1);
      	 	  
      	 	  //无线Modem开机
      	 	  if(moduleType==GPRS_MC52I)
      	 	  {
      	 	    resetWlModem(0);
      	 	  }
      	 	  else
      	 	  {
      	 	    resetWlModem(1);
      	 	  }
      	 	  
      	 	  break;
      	 	
      	 	case 3:   //M590E等延迟关机,2012-08-02,add
        	  //关断远程MODEM电源
        	  ioctl(fdOfIoChannel, WIRELESS_POWER_ON_OFF, 0);
        	   
        	  if (debugInfo&WIRELESS_DEBUG)
        	  {
        	    printf("(延迟)无线Modem电源:0\n");
        	  }
        	   
            wlPowerTime = nextTime(sysTime,0,5);//5秒后打开Modem电源
            wlModemFlag.power = 2;              //无线Modem断电
             
            //被级联终端
            if (checkIfCascade()==2)
            {
            	;
            }
            else
            {
             #ifdef PLUG_IN_CARRIER_MODULE 
              if (teIpAndPort.ethIfLoginMs==0x55)
              {
           	    #ifdef JZQ_CTRL_BOARD_V_1_4
           	     gatherModuleType = 2;
           	    #endif
           	 
           	    moduleType = ETHERNET;
              }
              else
              {
                #ifdef WDOG_USE_X_MEGA
                 sendXmegaFrame(GATHER_IO_DATA, &i, 0);
                #endif
              }
             #else
              #ifdef WDOG_USE_X_MEGA
               sendXmegaFrame(GATHER_IO_DATA, &i, 0);        
              #endif
             #endif
            }
      	 		break;
      	}
      }
      
      return;
   }
   
   //允许传输IP数据为FALSE,也就是说无线链路没有建通
   if (wlModemFlag.permitSendData==0)
   {
      if (wlModemFlag.sendToModem == 1)
    	{
        if (compareTwoTime(wlModemFlag.waitModemRet, sysTime))
        {
        	 if (debugInfo&WIRELESS_DEBUG)
        	 {
        	 	  printf("配置远程Modem无回复超时时间到,可能CPU与Modem通信不正常,复位终端\n");
        	 }
        	 
           logRun("配置远程Modem无回复超时时间到,可能CPU与Modem通信不正常,复位终端");

        	 ifReset = TRUE;
        }
    	}

      //12-11-07
      if ((bakModuleType!=MODEM_PPP) && (operateModem==1))
      {
        if (compareTwoTime(waitTimeOut, sysTime))
        {
         	 //登录不成功超时超次,重启终端
         	 if (wlModemFlag.numOfWlReset>RE_LOGIN_TIMES)
         	 {
         	 	  if (debugInfo&WIRELESS_DEBUG)
         	 	  {
         	 	    printf("Modem:%d次登录不成功,重启终端\n",RE_LOGIN_TIMES);
         	 	  }
  
    	 	  	  sprintf(tmpWlStr,"上行无线Modem:%d次登录不成功,重启终端",RE_LOGIN_TIMES);
    	 	  	  logRun(tmpWlStr);
         	 	  
         	 	  ifReset = TRUE;
         	 }
         	 else
         	 {
         	   wlModemPowerOnOff(0);
         	 	 
         	 	 if (debugInfo&WIRELESS_DEBUG)
         	 	 {
         	 	   printf("登录不成功,重启模块\n");
         	 	 }
         	 }
         	 
         	 return;
        }

        wlRet = configWirelessModem();
      }
      else
      {
      	if (dialerOk)
      	{
          //连接不成功的处理
          if (compareTwoTime(waitTimeOut, sysTime))
          {
          	printf("dlzd超时判断,关闭fdOfModem\n");
          	
          	setModemSoPara();
          	
          	return;
          }
           
          wlRet = configWirelessModem();
      	}
      	else
      	{
      		return;
      	}
      }
      
      switch (wlRet)
      {
      	case 1:
      	  if (moduleType==ETHERNET || bakModuleType==MODEM_PPP)
      	  {
     	  	  wlModemFlag.permitSendData = 1;   //允许传输应用层数据
     	  	  
     	  	  if (bakModuleType==MODEM_PPP)
     	  	  {
              wlModemFlag.numOfWlReset = 0;
     	  	  }
      	  }
      	  break;
      }
   }
   else   //允许传输IP数据
   {
   	 if (moduleType==GPRS_MC52I || moduleType==GPRS_M590E)
   	 {
   	 	 configWirelessModem();
   	 }
   	 
   	 //登录
   	 if (wlModemFlag.sendLoginFrame==0)
   	 {
   	 	 //ly,2011-12-02,在测试台体测试时发现,如果任务配置过密的话,如果有发不出去,会堆积数据而使登录不能正常
       //v1.61 11-11-25的专变终端程序并未作此改变,11-12-02的专变终端程序已改变
       //v1.61的集中器作了此改变    	 
       //初始化发送队列
       fQueue.inTimeFrameSend = FALSE;
       fQueue.thisStartPtr = 0;
       fQueue.tailPtr = 0;
       fQueue.sendPtr = 0;
       fQueue.delay   = 0;
       for(i=0;i<LEN_OF_SEND_QUEUE;i++)
       {
        	fQueue.frame[i].head = 0;
        	fQueue.frame[i].len  = 0;
        	fQueue.frame[i].next = 0xff;
       }
       fQueue.active0dDataSending = 0;       
       //-----------------------------------------

   	 	 wlModemFlag.sendLoginFrame = 1;
   	 	  
   	 	 AFN02001Login();
      
       #ifdef PLUG_IN_CARRIER_MODULE
        #ifndef MENU_FOR_CQ_CANON
         showInfo("登录主站...");
        #endif
       #endif
   	 	 
   	 	 if (debugInfo&WIRELESS_DEBUG)
   	 	 {
   	 	   printf("%02d-%02d-%02d %02d:%02d:%02d发送登录帧\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
   	 	 }
 	  	 
 	  	 wlModemFlag.loginSuccess = 0;
 	  	 waitTimeOut = nextTime(sysTime,0,56);   //超时时间设为一分钟后
 	  	 
 	  	 return;
   	 }
   	 
     if (wlModemFlag.loginSuccess == 0)
     {
       //登录超时,做相应的动作(复位模块RE_LOGIN_TIMES次后还不行就复位终端)
       if (compareTwoTime(waitTimeOut, sysTime))
       {
         	if (wlModemFlag.numOfWlReset>RE_LOGIN_TIMES)  //复位终端/集中器
         	{
         		if (debugInfo&WIRELESS_DEBUG)
         		{
         		  printf("Modem:登录超时,复位终端\n");
         		}
  	 	  	  
  	 	  	  #ifdef RECORD_LOG
  	 	  	   logRun("上行无线Modem:登录超时超次,重启终端");
  	 	  	  #endif

         		ifReset = TRUE;
          }
          else    //复位模块
          {
         	  if (debugInfo&WIRELESS_DEBUG)
         	  {
         	    printf("登录超时？\n");
         	  }
         	  
         	  //2012-11-09
         	  if (bakModuleType==MODEM_PPP)
         	  {
         	  	resetPppWlStatus();
         	  }
         	  else
         	  {
         	    wlModemPowerOnOff(0);
         	  }
         	}
       }
     }
     else
     {
       	//心跳
       	if (compareTwoTime(nextHeartBeatTime, sysTime))
        {
        	
repeatHeart:	
 	         AFN02003HeartBeat();

 	         wlModemFlag.heartSuccess = 0;
 	         
    	    #ifdef LIGHTING
    	     
    	     //2016-06-22,Add这个判断 
	         //2016-07-04,照明集中器心跳改为以秒为单位
    	     nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);

 	         waitTimeOut = nextTime(sysTime, 0, (commPara.heartBeat-3));
    	      	
    	    #else

           nextHeartBeatTime = nextTime(sysTime, commPara.heartBeat, 0);
 	         waitTimeOut = nextTime(sysTime,0,50);
          
          #endif
          
           lastHeartBeat = sysTime;
           
           if (debugInfo&WIRELESS_DEBUG)
           {
             printf("%02d-%02d-%02d %02d:%02d:%02d,wlModem下次心跳时间:%02d-%02d-%02d %02d:%02d:%02d\n",
                     sysTime.year,
                     sysTime.month,
                     sysTime.day,
                     sysTime.hour,
                     sysTime.minute,
                     sysTime.second,
                     nextHeartBeatTime.year,
                     nextHeartBeatTime.month,
                     nextHeartBeatTime.day,
                     nextHeartBeatTime.hour,
                     nextHeartBeatTime.minute,
                     nextHeartBeatTime.second
                   );
           }
           
           #ifdef PLUG_IN_CARRIER_MODULE
            #ifndef MENU_FOR_CQ_CANON
             showInfo("发送心跳帧...");
            #endif
           #endif
        }
        
        if (wlModemFlag.heartSuccess==0)
        {
       	   //心跳超时,复位模块或是复位终端
       	   if (compareTwoTime(waitTimeOut, sysTime))
           {
         	    //心跳超时,复位模块或是复位终端或是再心跳一次
         	    wlModemFlag.numOfRetryTimes++;
         	    if (wlModemFlag.numOfRetryTimes<3)
         	    {
             	  if (debugInfo&WIRELESS_DEBUG)
             	  {
               	  printf("wlModem:前一次心跳超时,再心跳一次试试\n");
                }
                
         	   	  goto repeatHeart;
         	    }
         	    else
         	    {
             	  if (wlModemFlag.numOfWlReset>RE_LOGIN_TIMES)   //超时超次,复位终端/集中器
             	  {
           	      if (debugInfo&WIRELESS_DEBUG)
           	      {
             	      printf("远程Modem:心跳超时,复位终端\n");
             	    }
             	    
             	    #ifdef RECORD_LOG
             	     logRun("远程Modem:心跳超时超次,复位终端");
             	    #endif
             	    
             	    //ly,2010-12-24,严重错误,
             	    //   济南现场和保定均发现"发送心跳...",但发送不成功3次后也不复位终端,只写了日志,
             	    //   但没有复位动作,没有真正复位终端
             	    ifReset = TRUE;
             	  }
             	  else    //复位模块
             	  {
               	  //2012-11-09
               	  if (bakModuleType==MODEM_PPP)
               	  {
               	  	resetPppWlStatus();
               	  }
               	  else
               	  {
             	      wlModemPowerOnOff(0);
             	    }
           	    
           	      if (debugInfo&WIRELESS_DEBUG)
           	      {
             	      printf("心跳超时?\n");
             	    }
             	  }
             	}
           }
        }
 	   }
   }
}

/*******************************************************
函数名称:setModemSoPara
功能描述:设置Modem库初始化参数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
void setModemSoPara(void)
{
  struct wirelessPara modemPara;

  if (wlModemFlag.useMainIp==1)
  {
     memcpy(modemPara.loginIp,ipAndPort.ipAddr,4);
     modemPara.loginPort = ipAndPort.port[0] | ipAndPort.port[1]<<8;

     if (debugInfo&WIRELESS_DEBUG)
     {
       printf("远程Modem:使用主用IP地址\n");
     }
  }
  else
  {
     if ((ipAndPort.ipAddrBak[0]==0 && ipAndPort.ipAddrBak[1]==0 && ipAndPort.ipAddrBak[2]==0 && ipAndPort.ipAddrBak[3]==0)
     	   || (ipAndPort.portBak[0]==0 && ipAndPort.portBak[1]==0)
     	  )
     {
       memcpy(modemPara.loginIp,ipAndPort.ipAddr,4);
       modemPara.loginPort = ipAndPort.port[0] | ipAndPort.port[1]<<8;

       if (debugInfo&WIRELESS_DEBUG)
       {
         printf("远程Modem:备用IP地址或端口为0,仍使用主用IP地址\n");
       }
     }
     else
     {
       memcpy(modemPara.loginIp,ipAndPort.ipAddrBak,4);
       modemPara.loginPort = ipAndPort.portBak[0] | ipAndPort.portBak[1]<<8;  	 

       if (debugInfo&WIRELESS_DEBUG)
       {
         printf("远程Modem:使用备用IP地址\n");
       }
     }
  }
  
  if (debugInfo&WIRELESS_DEBUG)
  {
    printf("IP:%d.%d.%d.%d,port:%d\n",modemPara.loginIp[0],modemPara.loginIp[1],modemPara.loginIp[2],modemPara.loginIp[3],modemPara.loginPort);
    printf("APN:%s\n",(char *)&ipAndPort.apn);
  }
  
  //重新识别模块类型
  #ifdef JZQ_CTRL_BOARD_V_0_3
   if (teIpAndPort.ethIfLoginMs==0x55)
   {
   	 moduleType = ETHERNET;		 
   }
   else
   {
     moduleType = ioctl(fdOfIoChannel,READ_MODULE_TYPE,0);
     moduleType &=0xf;
   }
  #endif

  strcpy(modemPara.apn, (char *)&ipAndPort.apn);
  //ly,2011-06-12,add.因发现用设备键盘改了APN以后登录就不成功的bug
  modemPara.apn[16] = '\0';
  modemPara.vpnName[32] = '\0';
  modemPara.vpnPass[32] = '\0';
  
  memcpy(modemPara.vpnName,vpn.vpnName,32);
  memcpy(modemPara.vpnPass,vpn.vpnPassword,32);
  modemPara.pRecvMsBuf = recvMsBuf;
  modemPara.moduleType = moduleType;
  modemPara.reportSignal = signalReport;
  modemPara.sendFrame = sendWirelessFrame;
  modemPara.debugFrame = sendDebugFrame;
  modemPara.portConfig = uart0Config;
  modemPara.msInput = msInput;
  modemPara.localIp = &wlLocalIpAddr;
  modemPara.fdOfEthSocket = &fdOfModem;
  if (moduleType==ETHERNET)
  {
    modemPara.delay = 1000;
  }
  else
  {
    if (bakModuleType==GPRS_SIM900A)
    {
      modemPara.delay = 70;
    }
    else
    {
    	if (LTE_AIR720H==moduleType)
    	{
      	modemPara.delay = 100;
    	}
			else
			{
      	modemPara.delay = 50;
			}
    }
    
    //查看系统是否支持ppp协议
    if(access("/dev/ppp", F_OK) == 0)
    {
    	printf("/dev/ppp存在\n");
    	
    	modemPara.moduleType = MODEM_PPP;

    	bakModuleType = MODEM_PPP;
    	
    	waitTimeOut = nextTime(sysTime, 1, 20);
    	
    	wlModemFlag.numOfWlReset++;
      
      //2012-11-16,一直建立连接都没有成功,不管ppp0是否存在,关闭它,重新进行拨号连接
      //尽管这种在APN正在,但是主站IP地址(/端口)不正确的时候会浪费流量
      if (wlModemFlag.numOfWlReset>5)
      {
      	printf("dialerOk=1,但是一直建立连接都不成功,关闭ppp0,重新进行拨号连接\n");
      	
      	wlModemFlag.numOfWlReset = 0;
      	
      	system("/bin/ppp-off");    //关闭ppp0
      }
      
      //没有登录成功且复位次数余3等于0,使用备用IP地址,否则使用主用IP地址
      if (wlModemFlag.lastIpSuccess==0 && ((wlModemFlag.numOfWlReset%3)==0))
      {
        wlModemFlag.useMainIp = 0;
      }
      else
      {
        wlModemFlag.useMainIp = 1;
      }
      
      wlModemFlag.lastIpSuccess = 0;        //上次IP登录置为未成功
    }
    else
    {
    	printf("/dev/ppp不存在\n");
    }
  }
  
  //初始化无线Modem库
  initWirelessSo(&modemPara);
  
  wlModemFlag.sendToModem  = 0;
  wlModemFlag.waitModemRet = nextTime(sysTime, WAIT_MODEM_MINUTES, 0); 
}

/**************************************************
函数名称:threadOfTtys0Received
功能描述:ttys0接收处理线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void *threadOfTtys0Receive(void *arg)
{
  int    recvLen;
  INT8U  tmpBuf[200];

  sleep(10);    //2017-8-23,add
    
  while (1)
  {
  	//12-11-08
  	if ((fdOfModem==NULL) || (operateModem==0))
  	{
  	  usleep(100000);
  		
  	  continue;
  	}
  	
    recvLen = read(fdOfModem, &tmpBuf, 100);
    
    //printf("recvLen=%d\n", recvLen);
    
    if (moduleType==ETHERNET || bakModuleType==MODEM_PPP)
    {
      if (
    	  recvLen==0 
    	   && 1==wlModemFlag.permitSendData    //2015-03-12,add
    	 )
      {
    	printf("连接断开,socketfd=%d\n", fdOfModem);
    		 
    	wlModemFlag.ethRecvThread  = 0;
    	wlModemFlag.permitSendData = 0;
    	wlModemFlag.loginSuccess   = 0;   //应用层登录主站是否成功置未成功
    	wlModemFlag.sendLoginFrame = 0;   //是否发送登录帧置为未发送
    	   
    	//2012-11-07
    	if (bakModuleType==MODEM_PPP)
    	{
    	  close(fdOfModem);    //2015-07-17,移动到判断内
    	  fdOfModem = NULL;

    	  setModemSoPara();    //2015-07-17,移动到判断内
    	}
         
        //2015-07-17
        if (moduleType==ETHERNET)
        {
          wlModemPowerOnOff(0);
        }

    	continue;
      }
    }

    if (recvLen>0)
    {
      //2012-07-10,防止数组越界
      if ((lenOfWlRecv+recvLen)<2047)
      {
      	memcpy(&wlRecvBuf[lenOfWlRecv], tmpBuf, recvLen);   //接收MODEM缓存
        lenOfWlRecv += recvLen;
      }
    }
    
    usleep(50);  
  }
  
  printf("退出ttys0接收线程\n");
}

/**************************************************
函数名称:threadOfTtys0Received
功能描述:ttys0接收处理线程
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void wlReceive(INT8U *buf, INT16U len)
{  
  INT16U i;
  INT8U  ret;

  if (debugInfo&WIRELESS_DEBUG)
  {
    printf("%02d-%02d-%02d %02d:%02d:%02d远程Modem Rx(%d):",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,len);
    
    //建立连接期间,显示通信字符串
    if (wlModemFlag.loginSuccess==0)
    {
      buf[len] = '\0';
      printf("%s", buf);
    }
    
    for(i=0;i<len;i++)
    {
    	 printf("%02x ",buf[i]);
    }
    printf("\n");
  }
 
  ret = wirelessReceive(buf, len);
  switch(ret)
  {
  	case IP_PERMIT:
  	  wlModemFlag.permitSendData = 1;   //允许传输应用层数据
  	  if (debugInfo&WIRELESS_DEBUG)
  	  {
  	    printf("允许传输IP数据\n");
  	  }
  	  break;
  	    
  	case NO_CARRIER:      //No Carrier
  	case LINK_DISCONNECT: //Disconnect
  	case LINK_FAIL:       //连接失败
  	  if (debugInfo&WIRELESS_DEBUG)
  	  {
  	  	if (ret==LINK_FAIL)
  	  	{
  	  	  printf("连接失败,重新开始连接!\n");
  	  	}
  	  	else
  	  	{
  	  	  printf("掉线!\n");
  	  	}
  	  }

  	  wlModemFlag.permitSendData = 0;   //不允许传输应用层数据
  	  wlModemFlag.loginSuccess   = 0;   //应用层登录主站是否成功置未成功
  	  wlModemFlag.sendLoginFrame = 0;   //是否发送登录帧置为未发送
  	  	
  	  setModemSoPara();
      
      if (moduleType==GPRS_M590E || moduleType==CDMA_CM180 || bakModuleType==GPRS_SIM900A)
      {
        if (moduleType==CDMA_CM180)
        {
        	 sleep(1);
        }

        wlModemPowerOnOff(0);  //重启远程Modem
      }

  	  if (menuInLayer==0)
 	  	{
 	  	  defaultMenu();
 	  	}
  	  break;
  	
  	case MODEM_RET_FRAME:     //配置Modem回复数据
  		if (wlModemFlag.sendToModem == 1)
  		{
  			 wlModemFlag.sendToModem = 0;
  			 
  			 if (debugInfo&WIRELESS_DEBUG)
  			 {
  			 	  printf("远程无线Modem配置数据回复,清除标志\n");
  			 }
  		}
  		break;
  }
}

/**************************************************
函数名称:wlModemPowerOnOff
功能描述:开关无线Modem电源
调用函数:
被调用函数:
输入参数:onOff,1-开,0-关
输出参数:
返回值：void
***************************************************/
void wlModemPowerOnOff(INT8U onOff)
{
	INT8U i;
	
	char  atCmd[20];
  
	if (onOff<2)
	{
	  //2012-08-01,add
	  if (moduleType==GPRS_M590E && onOff==0)
	  {
 	    if (debugInfo&WIRELESS_DEBUG)
 	    {
 	      printf("M590E发送软关机命令\n");
 	    }

      strcpy(atCmd, "AT+CPWROFF\r");      
      sendWirelessFrame((INT8U *)&atCmd, strlen(atCmd));
      
      wlModemFlag.power = 3;              //无线Modem延迟关机
      wlPowerTime = nextTime(sysTime,0,5);//5秒后打开Modem电源
      
      return;
    }
    
	  //操作硬件
	  ioctl(fdOfIoChannel,WIRELESS_POWER_ON_OFF,onOff);
	   
	  if (debugInfo&WIRELESS_DEBUG)
	  {
	    printf("无线Modem电源:%d\n",onOff);
	  }
	   
    if (onOff==0)
    {
      wlPowerTime = nextTime(sysTime,0,5);//5秒后打开Modem电源
      wlModemFlag.power = 2;              //无线Modem断电
       
      //被级联终端
      if (checkIfCascade()==2)
      {
      	;
      }
      else
      {
       #ifdef PLUG_IN_CARRIER_MODULE 
        if (teIpAndPort.ethIfLoginMs==0x55)
        {
          //2018-06-11,Add,集中器以太网不通时启用GPRS通道
					if ((wlModemFlag.lastIpSuccess==0) && (wlModemFlag.useMainIp==0))
          {
					 #ifdef WDOG_USE_X_MEGA
            sendXmegaFrame(GATHER_IO_DATA, &i, 0);
           #endif

					  fdOfModem = open("/dev/ttyS1",O_RDWR|O_NOCTTY);  //读写方式打开串口1
          }
					else
					{
					 #ifdef JZQ_CTRL_BOARD_V_1_4
						gatherModuleType = 2;
					 #endif
						
						moduleType = ETHERNET;
					}
        }
        else
        {
          #ifdef WDOG_USE_X_MEGA
           sendXmegaFrame(GATHER_IO_DATA, &i, 0);
          #endif
        }
       #else
        #ifdef WDOG_USE_X_MEGA
         sendXmegaFrame(GATHER_IO_DATA, &i, 0);        
        #endif
       #endif
      }
    }
	}
}

/**************************************************
函数名称:resetWlModem
功能描述:复位无线Modem
调用函数:
被调用函数:
输入参数:press,1-按下开机键,0-松开开机键
输出参数:
返回值：void
***************************************************/
void resetWlModem(INT8U press)
{
	 if (press<2)
	 {
	   ioctl(fdOfIoChannel, WIRELESS_IGT, press);
     
	   if (debugInfo&WIRELESS_DEBUG)
	   {
	     printf("开机键值:%d\n",press);
	   }
     
     if(moduleType==GPRS_MC52I)
     {
       if(press==0)
       {
      	  wlModemFlag.power = 1;
      	  wlPowerTime = nextTime(sysTime,0,1);
       }
     }
     else
     {
       if(press==1)
       {
      	  wlModemFlag.power = 1;
      	  wlPowerTime = nextTime(sysTime,0,3);
       }
     }
	 }
}

/**************************************************
函数名称:checkIfCascade
功能描述:判断级联
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：1-级联方终端,2-被级联方终端
***************************************************/
INT8U checkIfCascade(void)
{
	//级联端口必须为3(485接口2)
	if (cascadeCommPara.commPort==0x03)
	{
  	if ((cascadeCommPara.flagAndTeNumber&0x80)                                           //级联标志为1
    	   && ((cascadeCommPara.flagAndTeNumber&0x0f)==0x1)                                //级联方终端只有1个
    	    && (((cascadeCommPara.divisionCode[0]|cascadeCommPara.divisionCode[1]<<8)>0) && ((cascadeCommPara.divisionCode[0]|cascadeCommPara.divisionCode[1]<<8)<0x9999))  //级联方终端行政区划在1到9999之间
    	     && ((cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8)>0) //级联方终端地址>0
    	 )
    {
    	return 2;
    }
  
  	if (((cascadeCommPara.flagAndTeNumber&0x80)==0x0)       //级联标志为0
    	    && (((cascadeCommPara.flagAndTeNumber&0x0f)>=0x1) && ((cascadeCommPara.flagAndTeNumber&0x0f)<=0x3))  //被级联方终端只能有1到3个
    	 )
    {
    	return 1;
    }
  }
  
  return 0;
}

/**************************************************
函数名称:resetPppWlStatus
功能描述:复位PPP方式下的Modem自定义的状态
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void resetPppWlStatus(void)
{
	wlModemFlag.permitSendData = 0;
	wlModemFlag.loginSuccess   = 0;   //应用层登录主站是否成功置未成功
	wlModemFlag.sendLoginFrame = 0;   //是否发送登录帧置为未发送
  
  wlModemFlag.ethRecvThread  = 0;
  
  if (fdOfModem!=NULL)
  {
    close(fdOfModem);
  }
  
  fdOfModem=0;
	
	setModemSoPara();
}

/***************************************************
函数名称:threadOfRecvUdpData
功能描述:接收UDP数据线程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void *threadOfRecvUdpData(void *arg)
{
  INT8U  recvBuff[64];
  INT8U  lenOfRecv;
  INT8U  i;
  INT8U  checkSum;
  struct sockaddr_in clientAddr;
  int    lenOfAddr = sizeof(struct sockaddr_in);

  while(1)
  {
    lenOfRecv = recvfrom(socketOfUdp, recvBuff, 64, 0, (struct sockaddr*)&clientAddr, &lenOfAddr);
    if (lenOfRecv>0)
    {
    	if (debugInfo&WIRELESS_DEBUG)
    	{
    	  printf("UDP Rx:");
    	  for(i=0; i<lenOfRecv; i++)
    	  {
    		  printf("%02X ", recvBuff[i]);
    	  }
    	  printf("\n");
    	}
    	
    	if (recvBuff[0]==0x68 && recvBuff[lenOfRecv-1]==0x16)
    	{
    		checkSum = 0;
    		for(i=1; i<lenOfRecv-2; i++)
    		{
    			checkSum += recvBuff[i];
    		}
    	}
    	if (checkSum==recvBuff[lenOfRecv-2])
    	{
    		switch(recvBuff[2])    //AFN
    	  {
    	    case 1:    //dialer申请重启modem
    	      //执行电源开及开关机操作
    	      wlModemPowerOnOff(0);
    	      operateModem = 1;
    	      break;
    	    
    	    case 2:    //网络注册状态
    	    	break;
    	    
    	    case 3:    //信号
    	    	if (wlRssi!=recvBuff[3])
    	    	{
    	    	  signalReport(0, recvBuff[3]);
    	    	}
    	    	break;
    	    
    	    case 4:    //dialer ok
  	        if (0x1==recvBuff[3])
  	        {
  	          if (!dialerOk)
  	          {
    	          if (debugInfo&WIRELESS_DEBUG)
    	          {
  	              printf("dataBase:detect dialer ok, may establish link\n");
  	            }
  	          }
  	        
  	          dialerOk = 1;
  	        }
    	      break;
    	      
    	    case 5:    //获取的IP地址
  	        //终端IP赋值
  	        wlLocalIpAddr = recvBuff[3]<<24 | recvBuff[4]<<16 | recvBuff[5]<<8 | recvBuff[6];
    	    	break;
    	    
    	    case 6:    //未插卡指示
        	  //显示无卡标志
        	  if (0x02==recvBuff[3])
        	  {
        	  	modemSignal(9);
        	  }
        	  else
        	  {
        	  	modemSignal(8);
        	  }
    	    	break;
    	  }
    	}
    	else
    	{
    		printf("校验错误%x\n",checkSum);    		
    	}
    }
    else
    {
      perror("recv");
    }
    
    usleep(1000);
  }
}

/***************************************************
函数名称:sendToDialer
功能描述:发送UDP数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
unsigned char sendToDialer(unsigned char afn, unsigned char *pData, unsigned char lenOfData)
{
  int                socketOfSendUdp;
  struct sockaddr_in destAddr;
  unsigned char      sendBuff[52];
  unsigned char      i;
  unsigned char      checkSum;
  
  if ( (socketOfSendUdp=socket(AF_INET, SOCK_DGRAM, 0)) <0)
  {
    perror("socket");
  }
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(65432);
  destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (destAddr.sin_addr.s_addr == INADDR_NONE)
  {
    printf("Incorrect ip address!");
    close(socketOfSendUdp);
  }  
  
  sendBuff[0] = 0x68;
  sendBuff[1] = lenOfData+1;
  sendBuff[2] = afn;
  memcpy(&sendBuff[3], pData, lenOfData);
  checkSum=0;
  for(i=1;i<lenOfData+3; i++)
  {
  	checkSum += sendBuff[i];
  }
  sendBuff[lenOfData+3] = checkSum;
  sendBuff[lenOfData+4] = 0x16;
  
  if (sendto(socketOfSendUdp, sendBuff, lenOfData+5, 0, (struct sockaddr *)&destAddr, sizeof(struct sockaddr_in))<0)
  {
    perror("sendto");
  }
  
}

