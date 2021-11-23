/***************************************************
Copyright,2010,Huawei WoDian co.,LTD
文件名：AFN0A.c
作者：Leiyong
版本：0.9
完成日期：2010年1月
描述：主站AFN0A(查询参数)处理文件。
函数列表：
  01,10-01-13,wan guihua created.
  02,10-03-22,Leiyong,modify,将原来的一个数据标识记录成一个事件改为一次发的参数记录成一个事件
  03,10-11-23,Leiyong,在重庆电力公司测试时发现:电表参数批量设置下发易超时,
              原因为: AFN04中下发参数后要给主站发一个有参数设置的事件,当积成主站在下发参数时如果回了
              上报事件它认为超时，将重庆版的程序屏蔽F10和F25的事件主动上报，待以后空闲时上报
  04,10-12-17,Leiyong,兰州供电公司测试发现:F38,F39 1类、2类数据配置设置成功而召测不回来
              原因:原来的做法是不正确的,只留了一个大类号的位置,而规约要求可以设置16个大类号的配置,
              修正:改为可以设置16个大类号的配置。
  05,11-05-16,Leiyong,为适应地玮主站下发参数时FN有重叠的现象,添加pDataUnit04数据单元指针,
              以便每个FN能访问到自己的数据单元.
              本来原来也是按这个思路来做的,但有错误,现在修正后在地玮的系统中测试通过.
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

//变量
INT16U offset04;                     //接收到的帧中的数据单元偏移量(不计数据标识的字节)
INT8U  *pDataUnit04;                 //04的数据单元指针

/*******************************************************
函数名称:AFN04
功能描述:主站"设置参数"(AFN04)处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN04(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   char    say[20],str[8];
   INT16U  i;
   INT8U   fn, ackTail;
   INT8U   eventData[512];
   INT8U   paraNumCount=0;        //参数个数统计

   INT8U   ackAll,nAckAll;        //全部确认,否认标志
   INT8U   pseudoFrame[6];
      
   INT8U   maxCycle;              //最大循环次数
   INT8U   tmpDtCount;            //DT移位计数
   INT8U   tmpDt1;                //临时DT1

   BOOL    (*AFN04Fun[138])(INT8U *p);
   
   //清空数据
   bzero(ackData, 100);

   for(i=0;i<138;i++)
   {
     AFN04Fun[i] = NULL;
   }
   
   //组1
   AFN04Fun[0] = AFN04001;
   AFN04Fun[1] = AFN04002;
   AFN04Fun[2] = AFN04003;
   AFN04Fun[3] = AFN04004;
   AFN04Fun[4] = AFN04005;
   AFN04Fun[5] = AFN04006;
   AFN04Fun[6] = AFN04007;
   AFN04Fun[7] = AFN04008;
   
   //组2
   AFN04Fun[8] = AFN04009;
   AFN04Fun[9] = AFN04010;
   AFN04Fun[10] = AFN04011;
   AFN04Fun[11] = AFN04012;
   AFN04Fun[12] = AFN04013;
   AFN04Fun[13] = AFN04014;
   AFN04Fun[14] = AFN04015;
   AFN04Fun[15] = AFN04016;
   
   //组3
   AFN04Fun[16] = AFN04017;
   AFN04Fun[17] = AFN04018;
   AFN04Fun[18] = AFN04019;
   AFN04Fun[19] = AFN04020;
   AFN04Fun[20] = AFN04021;
   AFN04Fun[21] = AFN04022;
   AFN04Fun[22] = AFN04023;
   
   //组4
   AFN04Fun[24] = AFN04025;
   AFN04Fun[25] = AFN04026;
   AFN04Fun[26] = AFN04027;
   AFN04Fun[27] = AFN04028;
   
   AFN04Fun[28] = AFN04029;
   AFN04Fun[29] = AFN04030;
   AFN04Fun[30] = AFN04031;
   //AFN04Fun[31] = AFN04032;	//备用
   
   //组5
   AFN04Fun[32] = AFN04033;
  
   AFN04Fun[33] = AFN04034;
   AFN04Fun[34] = AFN04035;
   AFN04Fun[35] = AFN04036;
   AFN04Fun[36] = AFN04037;
   AFN04Fun[37] = AFN04038;
   AFN04Fun[38] = AFN04039;
   //AFN04Fun[39] = AFN04040;	//备用
   
  #ifndef LIGHTING
   //组6
   AFN04Fun[40] = AFN04041;
   AFN04Fun[41] = AFN04042;
   AFN04Fun[42] = AFN04043;
   AFN04Fun[43] = AFN04044;
   AFN04Fun[44] = AFN04045;
   AFN04Fun[45] = AFN04046;
   AFN04Fun[46] = AFN04047;
   AFN04Fun[47] = AFN04048;
   
   //组7
   AFN04Fun[48] = AFN04049;
  #else
   AFN04Fun[49] = AFN04050;
   AFN04Fun[50] = AFN04051;
   AFN04Fun[51] = AFN04052;
	 
   AFN04Fun[52] = AFN04053;    //2018-06-27,添加红外控制命令
   AFN04Fun[53] = AFN04054;    //2018-07-10,添加撤防警铃命令
   AFN04Fun[54] = AFN04055;    //2018-07-10,添加布防警铃命令
  #endif    //LIGHTING
   
   //组8
   AFN04Fun[56] = AFN04057;
   AFN04Fun[57] = AFN04058;
   AFN04Fun[58] = AFN04059;
   AFN04Fun[59] = AFN04060;
   AFN04Fun[60] = AFN04061;
   //AFN04Fun[61] = AFN04062;
   
   //组9
   AFN04Fun[64] = AFN04065;
   AFN04Fun[65] = AFN04066;
   AFN04Fun[66] = AFN04067;
   AFN04Fun[67] = AFN04068;
   
   //组10
   AFN04Fun[72] = AFN04073;
   AFN04Fun[73] = AFN04074;
   AFN04Fun[74] = AFN04075;
   AFN04Fun[75] = AFN04076;
   
   //组11
   AFN04Fun[80] = AFN04081;
   AFN04Fun[81] = AFN04082;
   AFN04Fun[82] = AFN04083;
   
   //扩展
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
            
            //专变终端,剔除投入时,不响应更改保安定值
           #ifndef PLUG_IN_CARRIER_MODULE
            if (toEliminate==CTRL_JUMP_IN)
            {
      	      if (fn==17)
      	      {
      	 	      return;
      	      }
            }
           #endif
            
            //ly,2011-05-14,pDataUnit04是本FN的数据单元起始指针
            pDataUnit04 = pDataHead + 4 + offset04;

            if (debugInfo&WIRELESS_DEBUG)
            {
              printf("AFN04 fn=%d,offset04=%d,本FN数据体前三个字节:%02x-%02x-%02x\n", fn, offset04,*pDataUnit04,*(pDataUnit04+1),*(pDataUnit04+2));
            }
            
      	   #ifdef SDDL_CSM
      	    //这是一个例外，没有用AFN04Fun[223]来调用哦
      	    //为了节省RAM
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
               //逐项确认/否认填写
               ackData[ackTail*5]   = *pDataHead;                         //DA1
               ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
               ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
               ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
               
               if (AFN04Fun[fn-1](pDataHead) == TRUE)
               {
                  ackAll++;
                  ackData[ackTail*5+4] = 0x00;                            //正确
    	            
    	            if (fn>0 && fn<84)
    	            {
    	              paraStatus[(fn-1)/8] |= 1<<((fn-1)%8);               //置"终端参数状态"位
    	            }
               }
               else
               {
                  nAckAll++;
                  ackData[ackTail*5+4] = 0x01;                            //出错
                  break;
               }
               ackTail++;
               
               //给出声音示警
               if ((fn>17 && fn<21) || (fn>41 && fn<50))   //ly,2011-10-12,add,只有控制参数才给声音示警
               {
         	       setBeeper(BEEPER_ON);         //蜂鸣器
         	     }
         	     alarmLedCtrl(ALARM_LED_ON);      //指示灯亮
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
        printf("执行后offset04=%d\n",offset04);
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
  
   //确认、否认、逐个确认/否认
   if (ackAll !=0  &&  nAckAll!=0)
   {
      AFN00003(ackTail, dataFrom, 0x04);      //逐项确认或否认
   }
   else
   {
      if (nAckAll==0)
      {
         ackOrNack(TRUE,dataFrom);            //全部确认
      }
      else
      {
         ackOrNack(FALSE,dataFrom);           //全部否认
      }
   }

   //记录事件入队列
   
   if (((eventRecordConfig.iEvent[0] & 0x04) || (eventRecordConfig.nEvent[0] & 0x04)) && paraNumCount>0)
   {
   	 eventData[0] = 0x3;
   	 eventData[1] = paraNumCount;       //记录的数据标识个数
   	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
   	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
   	 eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
   	 eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
   	 eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
   	 eventData[7] = sysTime.year/10<<4   | sysTime.year%10;
   	 eventData[8] = addrField.a3>>1 & 0xff;
   	  
     if (eventRecordConfig.iEvent[0] & 0x04)
     {
   	    writeEvent(eventData, 9+paraNumCount*4, 1, dataFrom);  //记入重要事件队列
   	 }
     if (eventRecordConfig.nEvent[0] & 0x04)
     {
   	    writeEvent(eventData, 9+paraNumCount*4, 2, dataFrom);  //记入一般事件队列
   	 }
   	  
   	 eventStatus[0] = eventStatus[0] | 0x04;
     
     
     //ly,2010-11-19,在重庆电力公司测试时发现,积成的主站在批量下发表地址参数时如果主动上报事件的
     //   话会引起超时,因此,重庆电力用的终端不主动上报该事件
     //ly,2011-01-29,取消只有重庆规约才不上报10和26
     //#ifdef CQDL_CSM
      //if (fn!=10 && fn!=25 && fn!=3 && fn!=121)
      if (fn!=10 && fn!=25)
      {
     //#endif

        if (debugInfo&PRINT_EVENT_DEBUG)
        {
          printf("AFN04调用主动上报\n");
        }
     
        activeReport3();   //主动上报事件
     
     //#ifdef CQDL_CSM
      }
     //#endif
   }
}

/*******************************************************
函数名称:AFN04001
功能描述:响应主站设置参数命令"终端通信参数(F1)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04001(INT8U *pData)
{
	//数据单元标识
	pData += 4;
  
  //指向数据单元
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
函数名称:AFN04002
功能描述:响应主站设置参数命令"终端上行通信口无线中继转发设置(F2)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04002(INT8U *pData)
{
	INT16U i;
	INT8U  tempData;
	
	//数据单元标识
	pData += 4;

  //指向数据单元
  pData = pDataUnit04;

	//被转发的终端地址数n及转发允许/禁止标志
	tempData = *pData++;
	
	//如果被转发的地址个数>16的话,不接受该参数的设置返回否认
	if((tempData & 0x7F) > 16)
	{
		offset04 += 1 + (tempData & 0x7F) * 2;
		*pData += (tempData & 0x7F) * 2;
		return FALSE;
	}
	
	//被转发的终端地址数和允许禁止标志
	relayConfig.relayAddrNumFlg = tempData;
	
	//被转发的终端地址
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
函数名称:AFN04003
功能描述:响应主站设置参数命令"主站IP地址和端口(F3)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04003(INT8U *pData)
{
   INT8U i;
   
   //数据单元标识
   pData+=4;

   //指向数据单元
   pData = pDataUnit04;
  
   //主用IP地址和端口
   for (i=0;i<4;i++)
   {
     ipAndPort.ipAddr[i] = *pData++;
   }
   ipAndPort.port[0] = *pData++;
   ipAndPort.port[1] = *pData++;
   
   //备用IP地址和端口
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
函数名称:AFN04004
功能描述:响应主站设置参数命令"主站电话号码和短信中心号码(F4)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04004(INT8U *pData)
{
	INT16U i;
	
  //数据单元标识
	pData+=4;

  //指向数据单元
  pData = pDataUnit04;

	//主站电话号码
	for(i=0;i<8;i++)
	{
		phoneAndSmsNumber.phoneNumber[i] = *pData++;
	}
	
	//短信中心号码
	for(i=0;i<8;i++)
	{
		phoneAndSmsNumber.smsNumber[i] = *pData++;
	}
	
	offset04 += 16;
	
  saveParameter(0x04, 4,(INT8U *)&phoneAndSmsNumber,sizeof(PHONE_AND_SMS));
  
  return TRUE;
	
}

/*******************************************************
函数名称:AFN04005
功能描述:响应主站设置参数命令"终端上行通信消息认证参数设置(F5)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04005(INT8U *pData)
{
	//数据单元标识
	pData+=4;

  //指向数据单元
  pData = pDataUnit04;
  
  //消息认证方案号
	messageAuth[0] = *pData++;
	
	//消息认证方案参数2Bytes
	messageAuth[1] = *pData++;
	messageAuth[2] = *pData++;
	
	offset04 += 3;
	
	saveParameter(0x04, 5, (INT8U *)&messageAuth, 3);
	
	return TRUE;
}

/*******************************************************
函数名称:AFN04006
功能描述:响应主站设置参数命令"终端上行通信消息认证参数设置(F6)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04006(INT8U *pData)
{
	 INT16U i;
	 
	 //数据单元标识
	 pData+=4;

   //指向数据单元
   pData = pDataUnit04;

   //组地址
	 for(i=0;i<16;i++)
	 {
		 groupAddr[i] = *pData++;
	 }
	
	 offset04 += 16;

	 saveParameter(0x04, 6, (INT8U *)groupAddr, 16);	 
	 
	 return TRUE;
}

/*******************************************************
函数名称:AFN04007
功能描述:响应主站设置参数命令"终端IP地址和端口(F7)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04007(INT8U *pData)
{
  //09规约是设置终端IP地址和端口
 
  TE_IP_AND_PORT tempTeIpAndPort;

  INT16U i, errFlg=0;
 
  //数据单元标识
  pData+=4;

  //指向数据单元
  pData = pDataUnit04;

  //终端IP地址
  for(i=0;i<4;i++)
  {
    tempTeIpAndPort.teIpAddr[i] = *pData++;
  }

  //子网掩码
  for(i=0;i<4;i++)
  {
	  tempTeIpAndPort.mask[i] = *pData++;
  }

  //网关
  for(i=0;i<4;i++)
  {
	  tempTeIpAndPort.gateWay[i] = *pData++;
  }

  //代理类型
  tempTeIpAndPort.proxyType = *pData++;

  //代理服务器地址
  for(i=0;i<4;i++)
  {
	  tempTeIpAndPort.proxyServer[i] = *pData++;
  }

  //代理服务器端口
  tempTeIpAndPort.proxyPort[0] = *pData++;
  tempTeIpAndPort.proxyPort[1] = *pData++;

  //代理服务器连接方式
  tempTeIpAndPort.proxyLinkType = *pData++;

  //用户名长度
  tempTeIpAndPort.userNameLen = *pData++;
  if(tempTeIpAndPort.userNameLen > 20)
  {
	  errFlg = 1;
	  pData += tempTeIpAndPort.userNameLen;
  }
  else
  {
	  //用户名
	  for(i=0;i<tempTeIpAndPort.userNameLen;i++)
	  {
	 	  tempTeIpAndPort.userName[i] = *pData++;
	  }
  }

  //密码长度
  tempTeIpAndPort.passwordLen = *pData++;
  if(tempTeIpAndPort.passwordLen > 20)
  {
	  errFlg = 1;
	  pData += tempTeIpAndPort.passwordLen;
  }
  else
  {
	  //密码
	  for(i=0;i<tempTeIpAndPort.passwordLen;i++)
	  {
		  tempTeIpAndPort.password[i] = *pData++;
	  }
  }

  //终端侦听端口
  tempTeIpAndPort.listenPort[0] = *pData++;
  tempTeIpAndPort.listenPort[1] = *pData++;

  offset04 += (24 + tempTeIpAndPort.userNameLen + tempTeIpAndPort.passwordLen);

  if(errFlg == 1)
  {
	  return FALSE;
  }

  bzero(&teIpAndPort, sizeof(TE_IP_AND_PORT));
  teIpAndPort = tempTeIpAndPort;
 
  saveIpMaskGateway(teIpAndPort.teIpAddr,teIpAndPort.mask,teIpAndPort.gateWay);  //保存到rcS中,ly,2011-04-12
 
  saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));
 
  return TRUE;
}

/*******************************************************
函数名称:AFN04008
功能描述:响应主站设置参数命令"终端上行通信工作方式(F8)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04008(INT8U *pData)
{
	 //09规约是终端上行通信工作方式(以太网或虚拟专网)
	 
	 INT16U i;
	
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
   pData = pDataUnit04;

	 //工作模式
	 tePrivateNetMethod.workMethod = *pData++;
	
	 //永久在线，时段在线模式重拨间隔
	 tePrivateNetMethod.redialInterval[0] = *pData++;
	 tePrivateNetMethod.redialInterval[1] = *pData++;
	
	 //被动激活模式重拨次数
	 tePrivateNetMethod.maxRedial = *pData++;
	
	 //被动激活模式连续无通信自动断线时间
	 tePrivateNetMethod.closeConnection = *pData++;
	
	 //时段在线模式允许在线时段标志
	 for(i=0;i<3;i++)
	 {
		 tePrivateNetMethod.onLinePeriodTime[i] = *pData++;
	 }
	
	 offset04 += 8;
	 
	 saveParameter(0x04, 8, (INT8U *)&tePrivateNetMethod, 8);

	 return TRUE;
}

/*******************************************************
函数名称:AFN04009
功能描述:响应主站设置参数命令"终端事件记录配置设置(F9)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04009(INT8U *pData)
{
	 INT16U i;
	
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
   pData = pDataUnit04;

	 //事件记录有效标志位
	 for(i=0;i<8;i++)
	 {
		 eventRecordConfig.nEvent[i] = *pData++;
	 }
	
	 //事件重要性等级标志位
	 for(i=0;i<8;i++)
	 {
		 eventRecordConfig.iEvent[i] = *pData++;
	 }
	
	 offset04 += 16;
	 
	 saveParameter(0x04, 9, (INT8U *)&eventRecordConfig, 16);
	 
	 return TRUE;
}

/*******************************************************
函数名称:AFN04010
功能描述:响应主站设置参数命令"终端电能表/交流采样装置配置参数(F10)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE

历史：
    1.ly,2012-08-09,剔除全0的表地址
*******************************************************/
BOOL AFN04010(INT8U *pData)
{
	METER_DEVICE_CONFIG meterDeviceConfig[2040];
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //一个控制器统计事件数据(与时间无关量)
	DATE_TIME tmpTime;

	INT16U i, j, len;
	
	INT8U errFlg = 0;
	
	//数据单元标识
	pData+=4;
	
	//指向数据单元
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
		//电能表/交流采样装置序号
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
		
		//所属测量点号
		meterDeviceConfig[i].measurePoint = *pData++;
		meterDeviceConfig[i].measurePoint |= (*pData++)<<8;
		if(meterDeviceConfig[i].measurePoint > 2040)
		{
			errFlg = 1;
		}
		
		//通信速率及通信端口号
		meterDeviceConfig[i].rateAndPort = *pData++;
		
		//通信协议类型
		meterDeviceConfig[i].protocol = *pData++;
		
		//通信地址
		for(j=0;j<6;j++)
	  {
			meterDeviceConfig[i].addr[j] = *pData++;
			
			//2013-05-07,判断表地址是否非法
			if ((meterDeviceConfig[i].addr[j]&0xf)>9 || (meterDeviceConfig[i].addr[j]>>4&0xf)>9)
			{
				errFlg = 1;
			}
		}
		
		
		//通信密码
		//  2014-09-05,照明集中器中采集器地址保存的是亮度与占空比的对应值中的亮度
		for(j=0;j<6;j++)
	  {
			meterDeviceConfig[i].password[j] = *pData++;
		}
		
		//电能费率个数
		meterDeviceConfig[i].numOfTariff = *pData++;
	 #ifndef LIGHTING	
		if((meterDeviceConfig[i].numOfTariff & 0x3F) > 48)
		{
			errFlg = 1;
		}
	 #endif
		
		//有功电能示值整数位及小数位个数
		meterDeviceConfig[i].mixed = *pData++;
		
		//所属采集器通信地址		
		for(j=0;j<6;j++)
		{
			meterDeviceConfig[i].collectorAddr[j] = *pData++;

		  //2014-09-05,照明集中器中采集器地址保存的是亮度与占空比的对应值中的亮度
		 #ifndef LIGHTING	
			//2013-05-07,判断采集器地址是否非法
			if ((meterDeviceConfig[i].collectorAddr[j]&0xf)>9 || (meterDeviceConfig[i].collectorAddr[j]>>4&0xf)>9)
			{
				errFlg = 1;
			}
		 #endif
		}
		
		//用户大类号及用户小类号
		meterDeviceConfig[i].bigAndLittleType = *pData++;
	}

	offset04 += (2 + len * 27);

	if(errFlg == 1)
	{
		return FALSE;
	}
		
	//将电能表/交流采样装置配置存入base_vice_info(基本信息副表)表中
	for(j=0; j<i; j++)
	{
	 #ifdef LIGHTING
		//2015-11-14,由于测量点序号用作他用,因此序号相同的不能删除了
		saveDataF10(meterDeviceConfig[j].measurePoint, meterDeviceConfig[j].rateAndPort&0x1f, meterDeviceConfig[j].addr,
		   meterDeviceConfig[j].measurePoint, (INT8U *)&meterDeviceConfig[j], 27);
		
    //控制点与时间无关量
		tmpTime = timeHexToBcd(sysTime);
		if (readMeterData((INT8U *)&meterStatisExtranTimeS, meterDeviceConfig[j].measurePoint, STATIS_DATA, 99, &tmpTime, 0)==TRUE)
		{
			if (meterStatisExtranTimeS.mixed & 0x20)
			{
				//清除时段已同步标志
				meterStatisExtranTimeS.mixed &= 0xdf;
        
				//存储控制点统计数据(与时间无关量)
        saveMeterData(meterDeviceConfig[j].measurePoint, meterDeviceConfig[j].rateAndPort&0x1f, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
				
				if (debugInfo&PRINT_CARRIER_DEBUG)
				{
					printf("清除控制点%d时段同步标志\n", meterDeviceConfig[j].measurePoint);
				}
			}
		}
	 #else
		saveDataF10(meterDeviceConfig[j].measurePoint, meterDeviceConfig[j].rateAndPort&0x1f, meterDeviceConfig[j].addr,
			 meterDeviceConfig[j].number, (INT8U *)&meterDeviceConfig[j], 27);
	 #endif
	}
	
	//将F10的电能表/交流采样装置配置数量保存进base_info(基本信息表)表中
	countParameter(0x04, 10, &meterDeviceNum);
	saveParameter(0x04, 10, (INT8U *)&meterDeviceNum, 2);
	
 #ifdef PLUG_IN_CARRIER_MODULE
   carrierFlagSet.msSetAddr = 1;       //主站设置表地址
  #ifdef DKY_SUBMISSION
   carrierFlagSet.msSetWait = nextTime(sysTime, 0, 5);
   if (debugInfo&PRINT_CARRIER_DEBUG)
   {
     printf("等待5秒钟将主站设置的表地址同步到本地通信模块\n");
   }
  #else
   carrierFlagSet.msSetWait = nextTime(sysTime, 1, 0);
   if (debugInfo&PRINT_CARRIER_DEBUG)
   {
     printf("等待一分钟将主站设置的表地址同步到本地通信模块\n");
   }
  #endif        	 	  	      	 
 #endif
 
 #ifdef SUPPORT_ETH_COPY
  setEthMeter = 1;
  msSetWaitTime = nextTime(sysTime, 0, 20);
  if (debugInfo&METER_DEBUG)
  {
    printf("等待20秒钟将表地址更新到以太网抄表链表中\n");
  }
 #endif
 
 #ifdef LIGHTING
  initXlcLink();
  initLdgmLink();
  initLsLink();
  initAcLink();
	initThLink();    //2018-08-04，Add
 #endif

	return TRUE;
}

/*******************************************************
函数名称:AFN04011
功能描述:响应主站设置参数命令"终端脉冲配置参数(F11)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04011(INT8U *pData)
{
	INT8U len;
	
	INT16U i;
	
	//数据单元标识
	pData+=4;
	
	//指向数据单元
  pData = pDataUnit04;

	len = *pData++;
	
	if(len > NUM_OF_SWITCH_PULSE)
	{
		pData += len * 5;
		offset04 += 1 + len * 5;
		return FALSE;
	}
	
	//脉冲配置个数
	pulseConfig.numOfPulse = len;
	
	//脉冲配置
	for(i=0;i<pulseConfig.numOfPulse;i++)
	{
		pulseConfig.perPulseConfig[i].ifNo = *pData++;							//脉冲输入端口号
		pulseConfig.perPulseConfig[i].pn   = *pData++;							//所属测量点号
		pulseConfig.perPulseConfig[i].character = *pData++;					//脉冲属性
		pulseConfig.perPulseConfig[i].meterConstant[0] = *pData++;	//电表常数K
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
函数名称:AFN04012
功能描述:响应主站设置参数命令"终端状态量输入参数(F12)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04012(INT8U *pData)
{
 	//数据单元标识
 	pData+=4;
 	
 	//指向数据单元
  pData = pDataUnit04;

 	//状态量接入标志位
 	statusInput[0] = *pData++;
 	
 	//状态量属性标志位
 	statusInput[1] = *pData++;
 	
 	offset04 += 2;
 	saveParameter(0x04, 12, (INT8U *)statusInput, 2);
   
  ifSystemStartup = 0;
 	
 	return TRUE;
}

/*******************************************************
函数名称:AFN04013
功能描述:响应主站设置参数命令"终端电压/电流模拟量配置参数(F13)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04013(INT8U *pData)
{
	IU_SIMULATE_CONFIG tempSimuIUConfig;
	
	INT16U i;
	INT8U errFlg = 0;
	
	//数据单元标识
	pData+=4;
	
	//指向数据单元
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
函数名称:AFN04014
功能描述:响应主站设置参数命令"终端总加组配置参数(F14)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04014(INT8U *pData)
{
	TOTAL_ADD_GROUP tempTotalAddGroup;
	
	INT8U  i, j, temp, errFlg = 0;
	
	offset04 = 0;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

	tempTotalAddGroup.numberOfzjz = *pData++;
	offset04++;
	if(tempTotalAddGroup.numberOfzjz > 8 || tempTotalAddGroup.numberOfzjz < 1)
	{
		//计算该FN占有的字节数
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
		//总加组序号
		tempTotalAddGroup.perZjz[i].zjzNo = *pData++;
		offset04++;
		if(tempTotalAddGroup.perZjz[i].zjzNo < 1 || tempTotalAddGroup.perZjz[i].zjzNo > 8)
		{
			errFlg = 1;
		}
		
		//总加组测量点数量
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
			//第n个测量点号及总加标志
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
函数名称:AFN04015
功能描述:响应主站设置参数命令"有功总电能量差动越限事件参数设置(F15)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04015(INT8U *pData)
{
 	ENERGY_DIFFERENCE_CONFIG tempDifferenceConfig;
	
	INT8U  i, j, errFlg = 0;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
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
		//有功总电能量差动组序号
		tempDifferenceConfig.perConfig[i].groupNum = *pData++;
		if(tempDifferenceConfig.perConfig[i].groupNum > 8
			|| tempDifferenceConfig.perConfig[i].groupNum < 1)
			errFlg = 1;
		
		tempDifferenceConfig.perConfig[i].toCompare = *pData++;			//对比的总加组序号
		tempDifferenceConfig.perConfig[i].toReference = *pData++;		//参照的总加组序号
		if(tempDifferenceConfig.perConfig[i].toCompare > 8
			|| tempDifferenceConfig.perConfig[i].toReference > 8)
			errFlg = 1;
			
		tempDifferenceConfig.perConfig[i].timeAndFlag = *pData++;					//参与差动的电能量的时间区间及对比方法标志
		tempDifferenceConfig.perConfig[i].ralitaveDifference = *pData++;	//差动越限相对偏差值
		
		//差动越限绝对偏差值
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
函数名称:AFN04016
功能描述:响应主站设置参数命令"虚拟专网用户名，密码(F16)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04016(INT8U *pData)
{
	INT8U  i;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

	//虚拟专网用户名
	for(i=0;i<32;i++)
	{
		vpn.vpnName[i] = *pData++;
		
		offset04++;
	}
	
	//虚拟专网密码
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
函数名称:AFN04017
功能描述:响应主站设置参数命令"终端保安定值(F17)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04017(INT8U *pData)
{
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

	//保安定值
	protectLimit[0] = *pData++;
	protectLimit[1] = *pData++;
	
	offset04 += 2;
		
	saveParameter(0x04, 17,(INT8U *)protectLimit, 2);
  return TRUE;
}

/*******************************************************
函数名称:AFN04018
功能描述:响应主站设置参数命令"终端功控时段(F18)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04018(INT8U *pData)
{
	 INT16U i;
	
	 //数据单元标识
	 pData += 4;
	 
	 //指向数据单元
   pData = pDataUnit04;

	 //终端功控时段
	 for(i=0;i<12;i++)
	 {
		 ctrlPara.pCtrlPeriod[i] = *pData++;
	 }
	
	 offset04 += 12;
	 	
	 saveParameter(0x04, 18,(INT8U *)&ctrlPara, sizeof(CONTRL_PARA));
   return TRUE;
}

/*******************************************************
函数名称:AFN04019
功能描述:响应主站设置参数命令"终端时段功控定值浮动系数(F19)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04019(INT8U *pData)
{
	 //数据单元标识
	 pData += 4;
	 
	 //指向数据单元
   pData = pDataUnit04;

	 //时段功控定值浮动系数
	 ctrlPara.pCtrlIndex = *pData++;
	
	 offset04 += 1;
	 	
	 saveParameter(0x04, 18,(INT8U *)&ctrlPara, sizeof(CONTRL_PARA));
   return TRUE;
}

/*******************************************************
函数名称:AFN04020
功能描述:响应主站设置参数命令"终端月电能量控定值浮动系数(F20)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04020(INT8U *pData)
{
	 //数据单元标识
	 pData += 4;
	 
	 //指向数据单元
   pData = pDataUnit04;

	 //时段功控定值浮动系数
	 ctrlPara.monthEnergCtrlIndex = *pData++;
	
	 offset04 += 1;
		
	 saveParameter(0x04, 18,(INT8U *)&ctrlPara, sizeof(CONTRL_PARA));
   return TRUE;
}

/*******************************************************
函数名称:AFN04021
功能描述:响应主站设置参数命令"终端电能量费率时段和费率数(F21)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04021(INT8U *pData)
{
	INT16U i;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

	//终端电能量费率时段和费率数
	for(i=0;i<49;i++)
	{
		periodTimeOfCharge[i] = *pData++;
	}
	
	offset04 += 49;
		
	saveParameter(0x04, 21,(INT8U *)periodTimeOfCharge, 49);
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04022
功能描述:响应主站设置参数命令"终端电能量费率(F22)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04022(INT8U *pData)
{
	 INT16U i, j;
	
	 //数据单元标识
	 pData += 4;
	 
	 //指向数据单元
   pData = pDataUnit04;

	 //费率数
	 chargeRateNum.chargeNum = *pData++;
	 offset04++;
	
	 //费率
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
函数名称:AFN04023
功能描述:响应主站设置参数命令"终端催费告警参数(F23)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04023(INT8U *pData)
{
	INT16U i;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

	//催费告警允许禁止标志位
	for(i=0;i<3;i++)
	{
		chargeAlarm[i] = *pData++;
	}
	
	offset04 += 3;
	
	saveParameter(0x04, 23,(INT8U *)chargeAlarm, 3);
  return TRUE;
}

/*******************************************************
函数名称:AFN04025
功能描述:响应主站设置参数命令"测量点基本参数(FN25)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04025(INT8U *pData)
{
	MEASURE_POINT_PARA pointPara;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&pointPara, sizeof(MEASURE_POINT_PARA));
			
			//电压互感器倍率
			pointPara.voltageTimes = *pData | *(pData+1)<<8;
			pData += 2;
	
			//电流互感器倍率
			pointPara.currentTimes = *pData | *(pData+1)<<8;
			pData += 2;
	
			//额定电压
			pointPara.ratingVoltage = *pData | *(pData+1)<<8;
			pData += 2;
	
			//额定电流
			pointPara.maxCurrent = *pData++;
	
			//额定负荷
			pointPara.powerRating[0] = *pData++;
			pointPara.powerRating[1] = *pData++;
			pointPara.powerRating[2] = *pData++;
	
			//电源接线方式
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
函数名称:AFN04026
功能描述:响应主站设置参数命令"测量点限值参数(FN26)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04026(INT8U *pData)
{
	MEASUREPOINT_LIMIT_PARA pointLimitPara;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&pointLimitPara, sizeof(MEASUREPOINT_LIMIT_PARA));
		 
			//电压合格率判别参数
  		pointLimitPara.vUpLimit = *pData | *(pData+1)<<8;							//电压合格上限
  		pointLimitPara.vLowLimit = *(pData+2) | *(pData+3)<<8;				//电压合格下限
  		pointLimitPara.vPhaseDownLimit = *(pData+4) | *(pData+5)<<8;	//电压断相门限
  		pData += 6;
 		  offset04 += 6;
  
  		//过压判别参数
  		pointLimitPara.vSuperiodLimit = *pData | *(pData+1)<<8;;	    //电压上上限(过压门限)
  		pData += 2;
  		pointLimitPara.vUpUpTimes = *pData++;												  //电压上上限越限持续时间
  		pointLimitPara.vUpUpResume[0] = *pData++;										  //电压上上限越限恢复系数
  		pointLimitPara.vUpUpResume[1] = *pData++;
  		offset04 += 5;
   
  		//欠压判别参数
  		pointLimitPara.vDownDownLimit = *pData | *(pData+1)<<8;;	//电压下下限(欠压门限)
  		pData += 2;
  		pointLimitPara.vDownDownTimes = *pData++;			//电压下下限越限持续时间
  		pointLimitPara.vDownDownResume[0] = *pData++;	//电压下下限越限恢复系数
  		pointLimitPara.vDownDownResume[1] = *pData++;
  		offset04 += 5;
  
  		//过流判别参数
  		pointLimitPara.cSuperiodLimit[0] = *pData++;	//相电流上上限(过流门限
  		pointLimitPara.cSuperiodLimit[1] = *pData++;
  		pointLimitPara.cSuperiodLimit[2] = *pData++;
  		pointLimitPara.cUpUpTimes = *pData++;					//相电流上上限越限持续时间
  		pointLimitPara.cUpUpReume[0] = *pData++;			//相电流上上限越限恢复系数
  		pointLimitPara.cUpUpReume[1] = *pData++;
  		offset04 += 6;
  
  		//超额定电流判别参数
  		pointLimitPara.cUpLimit[0] = *pData++;				//相电流上限(额定电流门限)
  		pointLimitPara.cUpLimit[1] = *pData++;
  		pointLimitPara.cUpLimit[2] = *pData++;
  		pointLimitPara.cUpTimes = *pData++;						//相电流上限越限时间
  		pointLimitPara.cUpResume[0] = *pData++;				//相电流上限越限恢复系数
  		pointLimitPara.cUpResume[1] = *pData++;
  		offset04 += 6;
  
  		//零序电流超限判别参数
 	 		pointLimitPara.cZeroSeqLimit[0] = *pData++;		//零序电流上限
  		pointLimitPara.cZeroSeqLimit[1] = *pData++;
  		pointLimitPara.cZeroSeqLimit[2] = *pData++;
  		pointLimitPara.cZeroSeqTimes = *pData++;			//零序电流越限持续时间
  		pointLimitPara.cZeroSeqResume[0] = *pData++;	//零序电流越限恢复系数
  		pointLimitPara.cZeroSeqResume[1] = *pData++;
  		offset04 += 6;
  
  		//视在功率超上上限判别参数
  		pointLimitPara.pSuperiodLimit[0] = *pData++;		//视载功率上上限
  		pointLimitPara.pSuperiodLimit[1] = *pData++;
  		pointLimitPara.pSuperiodLimit[2] = *pData++;
  		pointLimitPara.pSuperiodTimes = *pData++;				//视在功率越上上限持续时间
  		pointLimitPara.pSuperiodResume[0] = *pData++;		//视在功率越上上限恢复系数
  		pointLimitPara.pSuperiodResume[1] = *pData++;
  		offset04 += 6;
  
  		//视在功率超上限判别参数
  		pointLimitPara.pUpLimit[0] = *pData++;			//视载功率上限
  		pointLimitPara.pUpLimit[1] = *pData++;
  		pointLimitPara.pUpLimit[2] = *pData++;
  		pointLimitPara.pUpTimes = *pData++;					//视在功率越上限持续时间
  		pointLimitPara.pUpResume[0] = *pData++;			//视在功率越上限恢复系数
  		pointLimitPara.pUpResume[1] = *pData++;
  		offset04 += 6;
  
  		//三相电压不平衡超限判别参数
  		pointLimitPara.uPhaseUnbalance[0] = *pData++;			//三相电压不平衡限值
  		pointLimitPara.uPhaseUnbalance[1] = *pData++;
  		pointLimitPara.uPhaseUnTimes = *pData++;					//三相电压不平衡越限持续时间
  		pointLimitPara.uPhaseUnResume[0] = *pData++;			//三相电压不平衡越限恢复系数
  		pointLimitPara.uPhaseUnResume[1] = *pData++;
  		offset04 += 5;
  
  		//三相电流不平衡超限判别参数
  		pointLimitPara.cPhaseUnbalance[0] = *pData++;			//三相电流不平衡限值
  		pointLimitPara.cPhaseUnbalance[1] = *pData++;
  		pointLimitPara.cPhaseUnTimes = *pData++;					//三相电流不平衡越限持续时间
  		pointLimitPara.cPhaseUnResume[0] = *pData++;			//三相电流不平衡越限恢复系数
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
函数名称:AFN04027
功能描述:响应主站设置参数命令"测量点铜损，铁损参数(FN27)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04027(INT8U *pData)
{
	COPPER_IRON_LOSS copperIronLoss;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&copperIronLoss, sizeof(COPPER_IRON_LOSS));
			
			//A相
  		copperIronLoss.aResistance[0] = *pData++;		//A相电阻
  		copperIronLoss.aResistance[1] = *pData++;
  		copperIronLoss.aReactance[0] = *pData++;		//A相电抗
  		copperIronLoss.aReactance[1] = *pData++;
  		copperIronLoss.aConductance[0] = *pData++;	//A相电导
  		copperIronLoss.aConductance[1] = *pData++;
  		copperIronLoss.aSusceptance[0] = *pData++;	//A相电纳
  		copperIronLoss.aSusceptance[1] = *pData++;
  		offset04 += 8;
  
  		//B相
  		copperIronLoss.bResistance[0] = *pData++;		//B相电阻
  		copperIronLoss.bResistance[1] = *pData++;
  		copperIronLoss.bReactance[0] = *pData++;		//B相电抗
  		copperIronLoss.bReactance[1] = *pData++;
  		copperIronLoss.bConductance[0] = *pData++;	//B相电导
  		copperIronLoss.bConductance[1] = *pData++;
  		copperIronLoss.bSusceptance[0] = *pData++;	//B相电纳
  		copperIronLoss.bSusceptance[1] = *pData++;
  		offset04 += 8;
  
  		//C相
  		copperIronLoss.cResistance[0] = *pData++;		//C相电阻
  		copperIronLoss.cResistance[1] = *pData++;
  		copperIronLoss.cReactance[0] = *pData++;		//C相电抗
 	 		copperIronLoss.cReactance[1] = *pData++;
  		copperIronLoss.cConductance[0] = *pData++;	//C相电导
  		copperIronLoss.cConductance[1] = *pData++;
  		copperIronLoss.cSusceptance[0] = *pData++;	//C相电纳
  		copperIronLoss.cSusceptance[1] = *pData++;
  		offset04 += 8;
  
  		saveViceParameter(0x04, 27, pn, (INT8U *)&copperIronLoss, sizeof(COPPER_IRON_LOSS));
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04028
功能描述:响应主站设置参数命令"测量点功率因数分段限值(FN28)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04028(INT8U *pData)
{
	POWER_SEG_LIMIT powerSegLimit;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&powerSegLimit, sizeof(POWER_SEG_LIMIT));
			
			//功率因数分段限值1
  		powerSegLimit.segLimit1[0] = *pData++;
  		powerSegLimit.segLimit1[1] = *pData++;
  
  		//功率因数分段限值2
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
函数名称:AFN04029
功能描述:响应主站设置参数命令"终端当地电能表显示号(FN29)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04029(INT8U *pData)
{
	INT8U teShowNum[12];	
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(teShowNum, 12);
			
			//终端当地电能表显示号
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
函数名称:AFN04030
功能描述:响应主站设置参数命令"台区集中抄表停抄/投抄设置(FN30)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04030(INT8U *pData)
{
	INT8U copyStopAdminSet;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
  pData = pDataUnit04;

	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//台区集中抄表停抄/投抄设置
  		copyStopAdminSet = *pData++;
  
  		offset04 += 1;
  		saveViceParameter(0x04, 30, pn, (INT8U *)&copyStopAdminSet, 1);
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
函数名称:AFN04031
功能描述:响应主站设置参数命令"载波从节点附属节点地址(FN31)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04031(INT8U *pData)
{
	AUXILIARY_ADDR auxiliaryAddr;
	
	INT16U i, pn;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
  pData = pDataUnit04;
  
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&auxiliaryAddr, sizeof(AUXILIARY_ADDR));
			
			//载波从节点附属节点地址个数
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
  			//载波从节点附属节点地址
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
函数名称:AFN04033
功能描述:响应主站设置参数命令"终端抄表运行参数设置(FN33)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04033(INT8U *pData)
{
	TE_COPY_RUN_PARA tempTeCopyRunPara;
	
  INT16U i,j;
  
  INT8U num, errFlg = 0;
  INT8U count;
  INT8U dataSet, dataFlg = 0;			//有无有效数据标志

  //数据单元标识    
  pData += 4;
  
  //指向数据单元
  pData = pDataUnit04;

  tempTeCopyRunPara = teCopyRunPara;
   
  //参数块个数
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
  		
  		teCopyRunPara.para[dataSet].commucationPort = *pData++;						//终端通信端口号
	  	teCopyRunPara.para[dataSet].copyRunControl[0] = *pData++;					//台区集中抄表运行控制字
	  	teCopyRunPara.para[dataSet].copyRunControl[1] = *pData++;					
	  	for(j=0;j<4;j++)
	    {
	  		teCopyRunPara.para[dataSet].copyDay[j] = *pData++;							//抄表日-日期
	  	}
	  	teCopyRunPara.para[dataSet].copyTime[0] = *pData++;								//抄表日-时间
	  	teCopyRunPara.para[dataSet].copyTime[1] = *pData++;								
	  	teCopyRunPara.para[dataSet].copyInterval = *pData++;							//抄表间隔时间
	  	for(j=0;j<3;j++)
	  		teCopyRunPara.para[dataSet].broadcastCheckTime[j] = *pData++;		//广播校时定时时间
	  	teCopyRunPara.para[dataSet].hourPeriodNum = *pData++;							//允许抄表时段数
	  	
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
  
  //重新设置抄表时间
  reSetCopyTime();
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04034
功能描述:响应主站设置参数命令"集中器下行通信模块的参数设置(F34)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04034(INT8U *pData)
{
	DOWN_RIVER_MODULE_PARA tempDownRiverModulePara;
	
	INT16U i, j, errFlg = 0; 
	
	//数据单元标识
  pData += 4;

	//指向数据单元
  pData = pDataUnit04;

  //参数块个数
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
  	tempDownRiverModulePara.para[i].commucationPort = *pData++;	//终端通信端口号
  	
  	if(tempDownRiverModulePara.para[i].commucationPort < 1
  		|| tempDownRiverModulePara.para[i].commucationPort > 31)
  	{
  		errFlg = 1;
  	}
  	
  	tempDownRiverModulePara.para[i].commCtrlWord = *pData++;		//与终端接口端的通信控制字
  	
  	//与终端接口对应端的通信速率
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
函数名称:AFN04035
功能描述:响应主站设置参数命令"台区集中抄表重点户设置(F35)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04035(INT8U *pData)
{
	KEY_HOUSEHOLD tempKeyHouseHold;
	
	INT16U i, num, errFlg = 0; 

	//数据单元标识
  pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
  
  //台区集中抄表重点户个数
  tempKeyHouseHold.numOfHousehold = *pData++;
  offset04++;
  if(tempKeyHouseHold.numOfHousehold > 20)
  {
  	offset04 += tempKeyHouseHold.numOfHousehold * 2;
  	pData += tempKeyHouseHold.numOfHousehold * 2;
  	return FALSE;
  }
  
  //重点户的电能表/交流采样装置序号(1~2040)
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
函数名称:AFN04036
功能描述:响应主站设置参数命令"终端上行通信流量门限设置(F36)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04036(INT8U *pData)
{
	INT16U i;

	//数据单元标识
	pData += 4;

	//指向数据单元
  pData = pDataUnit04;

  //月通信流量门限
  for(i=0;i<4;i++)
  {
  	upTranslateLimit[i] = *pData++;
  }
  
  offset04 += 4;
  
  saveParameter(0x04, 36, (INT8U *)upTranslateLimit, offset04);
  return TRUE;
}

/*******************************************************
函数名称:AFN04037
功能描述:响应主站设置参数命令"终端级联通信参数(F37)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04037(INT8U *pData)
{
	CASCADE_COMM_PARA tempCascadeCommPara;
	
	INT16U i, num, errFlg = 0;

	//数据单元标识	
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
  
  tempCascadeCommPara.commPort = *pData++;						//终端级联通信端口号
  
  //级联端口只是485-2(端口3)
  if(tempCascadeCommPara.commPort != 3)
  {
  	offset04 += 7 + (*(pData+5)&0xf)*4; 
  	
  	return FALSE;
  }
  	
  tempCascadeCommPara.ctrlWord = *pData++;						//终端级联通信控制字
  tempCascadeCommPara.receiveMsgTimeout  = *pData++;		//接收报文超时时间
  tempCascadeCommPara.receiveByteTimeout = *pData++;	//接收等待字节超时时间
  tempCascadeCommPara.cascadeMretryTime  = *pData++;		//级联方(主动站)接收失败重发次数
  if(tempCascadeCommPara.cascadeMretryTime > 3)
  {
  	errFlg = 1;
  }
  	
  tempCascadeCommPara.groundSurveyPeriod = *pData++;	//级联巡测周期
  tempCascadeCommPara.flagAndTeNumber = *pData++;			//级联标志及终端个数
  offset04 += 7;
  
  num = tempCascadeCommPara.flagAndTeNumber & 0x0F;
  offset04 += num * 4;
  if((tempCascadeCommPara.flagAndTeNumber >> 7) == 0)
  {
  	if(num <= 3)
  	{
  		for(i=0;i<num*2;i+=2)
  		{
  			tempCascadeCommPara.divisionCode[i] = *pData++;				//终端行政区划码
  			tempCascadeCommPara.divisionCode[i + 1] = *pData++;
  			tempCascadeCommPara.cascadeTeAddr[i] = *pData++;			//级联终端地址
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
  		tempCascadeCommPara.divisionCode[0] = *pData++;				//终端行政区划码
  		tempCascadeCommPara.divisionCode[1] = *pData++;
  		tempCascadeCommPara.cascadeTeAddr[0] = *pData++;			//级联终端地址
  		tempCascadeCommPara.cascadeTeAddr[1] = *pData++;
  	}
  	else
  	{
  		pData += num * 4;
  		return FALSE;
  	}
  }
  
  //设置正确，保存数据
  bzero(&cascadeCommPara, sizeof(CASCADE_COMM_PARA));
	cascadeCommPara = tempCascadeCommPara;
  saveParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));
  return TRUE;
}

/*******************************************************
函数名称:AFN04038
功能描述:响应主站设置参数命令"1类数据配置设置(F38)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04038(INT8U *pData)
{
  TYPE_1_2_DATA_CONFIG typeData;
	INT16U i, j, errFlg = 0;
	INT8U  bType, lType, num;  //临时大类号、小类号及信息类组数
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
  
  bzero(&typeData, sizeof(TYPE_1_2_DATA_CONFIG));

	//本次设置所对应的大类号
	bType = *pData++;
	
	printf("AFN04-FN38大类号%d\n", bType);
	
	offset04++;
	if(bType > 15)
	{
		errFlg = 1;
	}
	
	//本次设置的组数(也就是本大类号下的小类数量)
	typeData.bigType[bType].groupNum = *pData++;
	offset04++;

	printf("AFN04-FN38组数%d\n", typeData.bigType[bType].groupNum);
	
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
		//用户小类号
		lType = *pData++;
		offset04++;
		
	  printf("AFN04-FN38用户小类号%d\n", lType);		
		
		if(lType > 15)
		{
			errFlg = 1;
		}
		
		//本小类的信息类组数
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
  
  //设置正确,保存数据
  typeDataConfig1 = typeData;
  saveParameter(0x04, 38, (INT8U *)&typeDataConfig1, sizeof(TYPE_1_2_DATA_CONFIG));
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04039
功能描述:响应主站设置参数命令"2类数据配置设置(F39)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04039(INT8U *pData)
{
  TYPE_1_2_DATA_CONFIG typeData;
	INT16U i, j, errFlg = 0;
	INT8U  bType, lType, num;  //临时大类号、小类号及信息类组数
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

  bzero(&typeData, sizeof(TYPE_1_2_DATA_CONFIG));

	//本次设置所对应的大类号
	bType = *pData++;
	offset04++;

	printf("AFN04-FN39大类号%d\n", bType);
	
	if(bType > 15)
	{
		errFlg = 1;
	}
	
	//本次设置的组数(也就是本大类号下的小类数量)
	typeData.bigType[bType].groupNum = *pData++;
	offset04++;

	printf("AFN04-FN39组数%d\n", typeData.bigType[bType].groupNum);
	
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
		//用户小类号
		lType = *pData++;
		offset04++;
		
	  printf("AFN04-FN39用户小类号%d\n", lType);		
		
		if(lType > 15)
		{
			errFlg = 1;
		}
		
		//本小类的信息类组数
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
  
  //设置正确,保存数据
  typeDataConfig2 = typeData;
  saveParameter(0x04, 39, (INT8U *)&typeDataConfig2, sizeof(TYPE_1_2_DATA_CONFIG));
  
  return TRUE;
}

#ifndef LIGHTING

/*******************************************************
函数名称:AFN04041
功能描述:响应主站设置参数命令"时段功控定值(F41)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04041(INT8U *pData)
{	
	PERIOD_CTRL_CONFIG tempPeriodCtrlConfig;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	INT16U i, j;

	INT8U da1, da2;			//总加组号
	INT8U temp1, temp2;
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&tempPeriodCtrlConfig, sizeof(PERIOD_CTRL_CONFIG));
			
			//方案标志
			tempPeriodCtrlConfig.periodNum = *pData++;
			offset04++;
			temp1 = tempPeriodCtrlConfig.periodNum & 0x07;
	
			//方案数量
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
	
			//判断pn的值是否合法, da1是否合法
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
函数名称:AFN04042
功能描述:响应主站设置参数命令"厂休功控参数(F42)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04042(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//判断pn的值是否合法
  		if(pn > 8 || errFlg == 1)
  		{
  			pData += 6;
  			offset04 += 6;
  			errFlg = 1;
  		}
			else
			{
				//厂休控定值
				wkdCtrlConfig[pn - 1].wkdLimit = *pData | (*(pData + 1)<<8);
				pData += 2;
		
				//限电起始时间
				wkdCtrlConfig[pn - 1].wkdStartMin = *pData++;
				wkdCtrlConfig[pn - 1].wkdStartHour = *pData++;
		
				wkdCtrlConfig[pn - 1].wkdTime = *pData++;			//限电时段
				wkdCtrlConfig[pn - 1].wkdDate = *pData++;			//限电周次
				offset04 += 6;
		
	  		saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
			}
		}
		da1 >>=1;
	}
	
  return (errFlg == 0?TRUE:FALSE);
}

/*******************************************************
函数名称:AFN04043
功能描述:响应主站设置参数命令"功率控制的功率计算滑差时间(F43)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04043(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U temp, errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//厂休控定值
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
函数名称:AFN04044
功能描述:响应主站设置参数命令"营业报停控参数(F44)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04044(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
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
				//报停起始时间（日，月，年）
   	    obsCtrlConfig[pn-1].obsStartDay = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsStartMonth = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsStartYear = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;

				//报停结束时间（日，月，年）
   	    obsCtrlConfig[pn-1].obsEndDay = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsEndMonth = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;
   	    obsCtrlConfig[pn-1].obsEndYear = (*pData&0xF)+ (*pData>>4&0xF)*10;
   	    pData++;

				//报停控功率限值
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
函数名称:AFN04045
功能描述:响应主站设置参数命令"功控轮次设定(F45)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04045(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
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
				//功控轮次
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
函数名称:AFN04046
功能描述:响应主站设置参数命令"月电量控定值(F46)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04046(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
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
				//月电量控定值
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
函数名称:AFN04047
功能描述:响应主站设置参数命令"购电量(费)控参数(F47)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04047(INT8U *pData)
{	
	INT16U    pn = 0;
	INT16U    tempPn = 0;
  
	INT8U     da1, da2;			//总加组号
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

	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//判断pn的值是否合法
		  if(pn > 8 || errFlg == 1)
		  {
		  	pData += 17;
		  	offset04 += 17;
		  	errFlg = 1;
		  }
		  else
		  {
     	  //事件数据-总加组号
     	  eventData[8] = pn;
     	  
     	  //购电单号
     	  chargeCtrlConfig[pn-1].numOfBill = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[9]  = *pData++;
     	  eventData[10] = *pData++;
     	  eventData[11] = *pData++;
     	  eventData[12] = *pData++;
     	  
     	  //追加/刷新标志
     	  chargeCtrlConfig[pn-1].flag = *pData;
     	  eventData[13] = *pData++;
     	  
     	  //购电量值
     	  chargeCtrlConfig[pn-1].chargeCtrl = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[14] = *pData++;
     	  eventData[15] = *pData++;
     	  eventData[16] = *pData++;
     	  eventData[17] = *pData++;
     	  
     	  //告警门限
     	  chargeCtrlConfig[pn-1].alarmLimit = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[18] = *pData++;
     	  eventData[19] = *pData++;
     	  eventData[20] = *pData++;
     	  eventData[21] = *pData++;
     	  
     	  //跳闸门限
     	  chargeCtrlConfig[pn-1].cutDownLimit = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
     	  eventData[22] = *pData++;
     	  eventData[23] = *pData++;
     	  eventData[24] = *pData++;
     	  eventData[25] = *pData++;
     	  
     	  chargeCtrlConfig[pn-1].alarmTimeOut.year = 0xFF;
     	   
     	  //事件数据-购前剩余电能量
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
     	  	 //原来无剩余电量,置购前电能量为0
     	  	 eventData[26] = 0;
     	  	 eventData[27] = 0;
     	  	 eventData[28] = 0;
     	  	 eventData[29] = 0;
     	  	 
     	  	 leftPower[0] = 0x0;
     	  	 leftPower[1] = 0x0;
     	  	 leftPower[2] = 0x0;
     	  	 leftPower[3] = 0x0;
     	  }
     
     	  //刷新剩余电量
     	  if (chargeCtrlConfig[pn-1].flag == 0xAA) //刷新剩余电量(当前剩余电量=下发购电量)
     	  {
     	     leftPower[0] = chargeCtrlConfig[pn-1].chargeCtrl&0xFF;
     	     leftPower[1] = chargeCtrlConfig[pn-1].chargeCtrl>>8&0xFF;
     	     leftPower[3] = chargeCtrlConfig[pn-1].chargeCtrl>>24&0xFF;
     	     leftPower[2] = chargeCtrlConfig[pn-1].chargeCtrl>>16&0xFF;
     	     
     	     printf("购电量(费)控参数(F47)总加组%d:刷新剩余电量\n", pn);
        }
        else      //追加剩余电量(当前剩余电量+=下发购电量)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
    	      printf("购电量(费)控参数(F47)总加组%d:追加剩余电量\n", pn);
    	    }
     	    
     	    negLeft   = 0;
     	    negAppend = 0;
     	    
     	    //当前剩余电量
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
       	      printf("购电量(费)控参数(F47):当前剩余电量=-%d\n", tmpData1);
       	    }
       	    else
       	    {
       	      printf("购电量(费)控参数(F47):当前剩余电量=%d\n", tmpData1);
       	    }
       	  }
     	    
     	    //下发购电量
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
     	        printf("购电量(费)控参数(F47):本次下发购电量=-%d\n", tmpData2);
     	      }
     	      else
     	      {
     	        printf("购电量(费)控参数(F47):本次下发购电量=%d\n", tmpData2);
     	      }
     	    }

     	    //新的剩余电量
     	    if ((negLeft==0 && negAppend==0)       //都为正
     	        || (negLeft==1 && negAppend==1)     //都为负
     	       )
     	    {
     	      tmpData1 += tmpData2;
     	      
     	      if (negLeft==1)
     	      {
     	        negAppend = 0xbb;   //结果为负
     	      }
     	      else
     	      {
     	        negAppend = 0xaa;   //结果为正
     	      }
     	    }
     	    else
     	    {
     	    	if (negLeft==0 && negAppend==1)     //剩余为正,追加都为负
     	    	{
     	        if (tmpData1>=tmpData2)
     	        {
     	          tmpData1 -= tmpData2;
     	          negAppend = 0xaa;     //结果为正
     	        }
     	        else
     	        {
     	          tmpData1 = tmpData2-tmpData1;
     	          negAppend = 0xbb;     //结果为负
     	        }
     	      }
     	      else     //剩余为负,追加为正
     	      {
     	        if (tmpData1>=tmpData2)
     	        {
     	          tmpData1 -= tmpData2;
     	          negAppend = 0xbb;     //结果为负
     	        }
     	        else
     	        {
     	          tmpData1 = tmpData2-tmpData1;
     	          negAppend = 0xaa;     //结果为正
     	        }
     	      }
     	    }

    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
     	      if (negAppend==0xbb)
     	      {
     	        printf("购电量(费)控参数(F47):追加后新的购电量=-%d\n", tmpData1);
     	      }
     	      else
     	      {
     	        printf("购电量(费)控参数(F47):追加后新的购电量=%d\n", tmpData1);
     	      }
     	    }
     			
     			//调整数据格式
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
     	  
     	  //事件数据-购电后剩余电能量
     	  eventData[30] = leftPower[0];
     	  eventData[31] = leftPower[1];
     	  eventData[32] = leftPower[2];
     	  eventData[33] = leftPower[3];
     	  
        //计算用参考剩余电量=当前剩余电量
        leftPower[4] = leftPower[0];
        leftPower[5] = leftPower[1];
        leftPower[6] = leftPower[2];
        leftPower[7] = leftPower[3];
     
        //计算用参考总加有功电能量=当前总加有功电能
        readTime = timeHexToBcd(sysTime);

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
        	  printf("购电量(费)控参数(F47):总加组%d只有脉冲测量点\n", pn);
        	}
        	
        	bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime);
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("购电量(费)控参数(F47):总加组%d混合脉冲测量点\n", pn);
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
              printf("购电量(费)控参数(F47):参考总加有功电能量返回TRUE但总加月电能量为0xee,参考置0\n");
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
            printf("购电量(费)控参数(F47):参考总加有功电能量返回FALSE,参考置0\n");
          }
          
          //ly,2011-04-16,改成用以下这句
         	leftPower[8]  = 0x0;
          leftPower[9]  = 0x0;
          leftPower[10] = 0x0;
          leftPower[11] = 0x0;
        }
     	  
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
     	    printf("购电量(费)控参数(F47):参考总加有功电能量=%02x%02x%02x%02x\n", leftPower[11], leftPower[10], leftPower[9], leftPower[8]);
     	  }

        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);

        //ERC19.购电参数设置记录(记录事件入队列)
        if ((eventRecordConfig.iEvent[2] & 0x04) || (eventRecordConfig.nEvent[2] & 0x04))
        {
         	 eventData[0] = 19;  //ERC
         	 eventData[1] = 34;  //记录长度
         	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         	 eventData[4] = sysTime.hour/10<<4   | sysTime.hour%10;
         	 eventData[5] = sysTime.day/10<<4    | sysTime.day%10;
         	 eventData[6] = sysTime.month/10<<4  | sysTime.month%10;
         	 eventData[7] = sysTime.year/10<<4   | sysTime.year%10;
         	  
           if (eventRecordConfig.iEvent[2] & 0x04)
           {
         	    writeEvent(eventData, 34, 1, DATA_FROM_GPRS);   //记入重要事件队列
         	 }
           if (eventRecordConfig.nEvent[2] & 0x04)
           {
         	    writeEvent(eventData, 34, 2, DATA_FROM_LOCAL);  //记入一般事件队列
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
函数名称:AFN04048
功能描述:响应主站设置参数命令"电控轮次设定(F48)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04048(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
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
			
			//判断pn的值是否合法
		  if(pn > 8 || errFlg == 1)
		  {
		  	pData += 1;
		  	offset04++;
		  	errFlg = 1;
		  }
		  else
		  {
		  	//购电单号
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
函数名称:AFN04049
功能描述:响应主站设置参数命令"功控告警时间(F49)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04049(INT8U *pData)
{	
	INT16U pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//总加组号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;

	//指向数据单元
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//判断pn的值是否合法
		  if(pn > 8 || errFlg == 1)
		  {
		  	errFlg = 1;
		  }
			else
			{
				//功控告警时间
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
函数名称:AFN04050
功能描述:响应主站设置参数命令"控制时段(F50)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04050(INT8U *pData)
{
  struct ctrlTimes           cTimes;
  BOOL                       ifCtrlPeriod=0;
  INT8U                      i, j;
  INT8U                      count;
  struct cpAddrLink          *tmpCpLink;
	DATE_TIME                  tmpTime;
  METER_STATIS_EXTRAN_TIME_S meterStatisExtranTimeS;  //一个控制器统计事件数据(与时间无关量)

  //数据单元标识    
  pData += 4;
  
  //指向数据单元
  pData = pDataUnit04;
   
  //参数块个数
  count = *pData++;
  
  //2016-12-15,将时段参数改成小包发送,第一包数据要清除原时段
	if (count>0 && ((count&0x80)==0x80))
  {
  	if (debugInfo&WIRELESS_DEBUG)
    {
      printf("AFN04050:清除原时段参数\n");
    }
		deleteCtimes();
  }
	count &=0x7f;
  
  //offset04 += 1+count*24+1+4;
  offset04 += 1+count*25+1+4;    //2016-08-17,修改为一个时段参数25字节

  for(i=0; i<count; i++)
  {
  	cTimes.startMonth = *pData++;    //起始月
  	cTimes.startDay   = *pData++;    //起始日

  	cTimes.endMonth   = *pData++;    //结束月
  	cTimes.endDay     = *pData++;    //结束日
  	
  	cTimes.deviceType = *pData++;    //设备类型
  	cTimes.noOfTime   = *pData++;    //时段号
  	cTimes.workDay    = *pData++;    //启用日,2016-08-17,Add
  	
  	for(j=0; j<6; j++)
    {
  	  cTimes.hour[j]  = *pData++;    //时刻1-6时
  	  cTimes.min[j]   = *pData++;    //时刻1-6分
  	  cTimes.bright[j]= *pData++;    //时刻1-6亮度
  	}
  	
  	cTimes.next       = NULL;
  	
  	saveViceParameter(cTimes.deviceType, cTimes.startMonth, cTimes.startDay | cTimes.noOfTime<<8, (INT8U *)&cTimes, sizeof(struct ctrlTimes));
  }

  //2015-06-09,Add控制模式
  ctrlMode = *pData++;
  saveParameter(0x04, 52, &ctrlMode, 1);
  
  //2015-06-25,光控提前-延迟分钟有效
  memcpy(beforeOnOff, pData, 4);
  saveParameter(0x04, 53, beforeOnOff, 4);

  
  initCtrlTimesLink();
	
	tmpCpLink = initPortMeterLink(0xff);
	
	while(tmpCpLink!=NULL)
	{
    //控制点与时间无关量
		tmpTime = timeHexToBcd(sysTime);
		if (readMeterData((INT8U *)&meterStatisExtranTimeS, tmpCpLink->mp, STATIS_DATA, 99, &tmpTime, 0)==TRUE)
		{
			if (meterStatisExtranTimeS.mixed & 0x20)
			{
				//清除时段已同步标志
				meterStatisExtranTimeS.mixed &= 0xdf;
        
				//存储控制点统计数据(与时间无关量)
        saveMeterData(tmpCpLink->mp, tmpCpLink->port, timeHexToBcd(sysTime), (INT8U *)&meterStatisExtranTimeS, STATIS_DATA, 99, sizeof(METER_STATIS_EXTRAN_TIME_S));
				
				if (debugInfo&WIRELESS_DEBUG)
				{
					printf("清除控制点%d时段同步标志\n", tmpCpLink->mp);
				}
			}
		}
		
		tmpCpLink = tmpCpLink->next;
	}

  if (debugInfo&WIRELESS_DEBUG)
  {
    printf("AFN04050:共%d参数块\n", count);
  }

  return TRUE;   
}

/*******************************************************
函数名称:AFN04051
功能描述:响应主站设置参数命令"异常判断阈值(F51)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04051(INT8U *pData)
{	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
	
	pnGate.failureRetry = *pData++;			//发现灯具故障重试次数
	pnGate.boardcastWaitGate = *pData++;//广播命令等待时长
	pnGate.checkTimeGate = *pData++;    //校时阈值
	pnGate.lddOffGate = *pData++;       //单灯控制点离线阈值
	pnGate.lddtRetry = *pData++;        //搜索末端重试次数
	pnGate.offLineRetry = *pData++;     //离线重试次数
	pnGate.lcWave = *pData++;           //光控照度震荡值
	pnGate.leakCurrent = *pData++;      //漏电流阈值
	offset04 += 8;
	
  saveParameter(0x04, 51, (INT8U *)&pnGate, sizeof(PN_GATE));

  return TRUE;
}

/*******************************************************
函数名称:AFN04052
功能描述:响应主站设置参数命令"控制点限值参数(FN52)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04052(INT8U *pData)
{
	PN_LIMIT_PARA pointLimitPara;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
  pData = pDataUnit04;

	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//清空数据
			bzero(&pointLimitPara, sizeof(PN_LIMIT_PARA));
		 
  		pointLimitPara.vSuperiodLimit = *pData | *(pData+1)<<8;;	//电压过压门限
  		pData += 2;

  		pointLimitPara.vDownDownLimit = *pData | *(pData+1)<<8;;	//电压欠压门限
  		pData += 2;

  		pointLimitPara.cSuperiodLimit[0] = *pData++;	            //电流过流门限
  		pointLimitPara.cSuperiodLimit[1] = *pData++;
  		pointLimitPara.cSuperiodLimit[2] = *pData++;

  		pointLimitPara.cDownDownLimit[0] = *pData++;	            //电流欠流门限
  		pointLimitPara.cDownDownLimit[1] = *pData++;
  		pointLimitPara.cDownDownLimit[2] = *pData++;
  		
  		pointLimitPara.pUpLimit[0] = *pData++;			              //功率上限
  		pointLimitPara.pUpLimit[1] = *pData++;
  		pointLimitPara.pUpLimit[2] = *pData++;

  		pointLimitPara.pDownLimit[0] = *pData++;			            //功率下限
  		pointLimitPara.pDownLimit[1] = *pData++;
  		pointLimitPara.pDownLimit[2] = *pData++;
			
  		pointLimitPara.factorDownLimit[0] = *pData++;             //功率因数下限
  		pointLimitPara.factorDownLimit[1] = *pData++;

  		pointLimitPara.overContinued = *pData++;                  //越限持续时间
  		
  		offset04 += 23;
  
  		saveViceParameter(0x04, 52, pn, (INT8U *)&pointLimitPara, sizeof(PN_LIMIT_PARA));    		
		}
		da1 >>=1;
	}
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04053
功能描述:响应主站设置参数命令"遥控命令<扩展>(F053)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04053(INT8U *pData)
{
	INT16U i;
	INT16U tmpPn;
	INT8U  tmpBuf[5];
	
	//数据单元标识
	tmpPn = findFnPn(*pData, *(pData+1), FIND_PN);
	//pData+=4;
	 
	//指向数据单元
  //pData = pDataUnit04;
  
	offset04 += 0;

	if (tmpPn<=4 && tmpPn>0 && copyCtrl[1].pForwardData==NULL)
	{
		if (selectParameter(5, 160+tmpPn-1, tmpBuf, 2)==FALSE)
		{
			printf("%s==>%d,无红外学习数据\n", __func__, tmpPn);
			return FALSE;
		}
	
		copyCtrl[1].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
		copyCtrl[1].pForwardData->fn						= 1;
		copyCtrl[1].pForwardData->dataFrom			= DOT_CP_IR;
		copyCtrl[1].pForwardData->ifSend				= FALSE;
		copyCtrl[1].pForwardData->receivedBytes = 0;
		copyCtrl[1].pForwardData->forwardResult = RESULT_NONE;
	
		//透明转发通信控制字,2400-8-n-1
		copyCtrl[1].pForwardData->ctrlWord = 0x63;
	 
		//透明转发接收等待字节超时时间
		copyCtrl[1].pForwardData->byteTimeOut = 1;
	 
		copyCtrl[1].pForwardData->data[768] = tmpPn-1; 	 //本字节保存命令类型
	
		//透明转发接收等待报文超时时间
		//单位为s
		copyCtrl[1].pForwardData->frameTimeOut = 1;
		
		//*pData为命令类型，1-制冷开,2-制热开,3-除湿开,4-关
		selectParameter(5, 160+tmpPn-1, copyCtrl[1].pForwardData->data, 2);
		copyCtrl[1].pForwardData->length = copyCtrl[1].pForwardData->data[0] | copyCtrl[1].pForwardData->data[1]<<8;
		printf("读出长度=%d\n", copyCtrl[1].pForwardData->length);
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
函数名称:AFN04054
功能描述:响应主站设置参数命令"撤防控制点<扩展>(F054)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04054(INT8U *pData)
{
	INT16U tmpPn;
	struct cpAddrLink *tmpQueryMpLink;
	
	//数据单元标识
	tmpPn = findFnPn(*pData, *(pData+1), FIND_PN);
	//pData+=4;
	 
	//指向数据单元
  //pData = pDataUnit04;

  
	offset04 += 0;

	if (tmpPn>0 && tmpPn<2040)
	{
		saveParameter(0x04, 54, (INT8U *)&tmpPn, 2);
		
		printf("撤防警铃%d\n", tmpPn);

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
函数名称:AFN04055
功能描述:响应主站设置参数命令"布防控制点<扩展>(F055)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04055(INT8U *pData)
{
	INT16U tmpPn;
	
	//数据单元标识
	tmpPn = findFnPn(*pData, *(pData+1), FIND_PN);
	//pData+=4;
	 
	//指向数据单元
  //pData = pDataUnit04;
  
	offset04 += 0;

	if (tmpPn>0 && tmpPn<2040)
	{
		deleteParameter(0x04, 54);
		saveParameter(0x04, 55, (INT8U *)&tmpPn, 2);
		
		printf("布防警铃%d\n", tmpPn);

		return TRUE;
	}

	return FALSE;

}


#endif    //LIGHTING,about line 2564

/*******************************************************
函数名称:AFN04057
功能描述:响应主站设置参数命令"终端声音告警允许禁止标志(F57)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04057(INT8U *pData)
{	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;

	//声音告警允许/禁止标志位
	voiceAlarm[0] = *pData++;
	voiceAlarm[1] = *pData++;
	voiceAlarm[2] = *pData++;
	offset04 += 3;
	
  saveParameter(0x04, 57, (INT8U *)voiceAlarm, 3);
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04058
功能描述:响应主站设置参数命令"终端自动保电参数(F58)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04058(INT8U *pData)
{	
	//数据单元
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
	
	//允许与主站连续无通信时间
	noCommunicationTime = *pData++;
	offset04 += 1;
	
  saveParameter(0x04, 58, (INT8U *)&noCommunicationTime, 1);
  
  return TRUE;
}

/*******************************************************
函数名称:AFN04059
功能描述:响应主站设置参数命令"电能表异常判别阈值(F59)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04059(INT8U *pData)
{	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
	
	meterGate.powerOverGate = *pData++;				//电能量超差阈值
	meterGate.meterFlyGate = *pData++;				//电能表飞走阈值
	meterGate.meterStopGate = *pData++;				//电能表停走阈值
	meterGate.meterCheckTimeGate = *pData++;	//电能表校时阈值
	offset04 += 4;
	
  saveParameter(0x04, 59, (INT8U *)&meterGate, 4);

  return TRUE;
}

/*******************************************************
函数名称:AFN04060
功能描述:响应主站设置参数命令"谐波限值(F60)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04060(INT8U *pData)
{	
	INT8U i;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
  pData = pDataUnit04;
	
	//总畸变电压含有率上限
	waveLimit.totalUPercentUpLimit[0] = *pData++;
	waveLimit.totalUPercentUpLimit[1] = *pData++;
	
	//奇次谐波电压含有率上限
	waveLimit.oddUPercentUpLimit[0] = *pData++;
	waveLimit.oddUPercentUpLimit[1] = *pData++;
	
	//偶次谐波电压含有率上限
	waveLimit.evenUPercentUpLimit[0] = *pData++;
	waveLimit.evenUPercentUpLimit[1] = *pData++;
	offset04 += 6;
	
	//谐波电压含有率上限
	for(i=0;i<18;i++)
	{
		waveLimit.UPercentUpLimit[i][0] = *pData++;
		waveLimit.UPercentUpLimit[i][1] = *pData++;
		offset04 += 2;
	}
	
	//总畸变电流有效值上限
	waveLimit.totalIPercentUpLimit[0] = *pData++;
	waveLimit.totalIPercentUpLimit[1] = *pData++;
	offset04 += 2;
	
	//谐波电流有效值上限
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
函数名称:AFN04061
功能描述:响应主站设置参数命令"直流模拟量接入参数(F61)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04061(INT8U *pData)
{
	//数据单元标识
	pData+=4;
	
	//指向数据单元
  pData = pDataUnit04;
	
	adcInFlag = *pData++;
	
	offset04 += 1;
	
	saveParameter(0x04, 61, (INT8U *)&adcInFlag, 1);
	
	return TRUE;
}

/*******************************************************
函数名称:AFN04065
功能描述:响应主站设置参数命令"定时上报1类数据任务设置(F65)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04065(INT8U *pData)
{	
	REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U  da1, da2;			//任务号
	INT8U  errFlg = 0;
	INT16U numOfItem = 0;    //2014-10-16,add
  INT8U  iOfPn,iOfFn;

	tmpreportTask = reportTask1;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
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
			
			//上报周期及发送周期单位
			reportTask1.task[pn - 1].sendPeriod = *pData++;
			offset04++;
			
			//上报基准时间
			reportTask1.task[pn - 1].sendStart.second = (*pData >> 4) * 10 + (*pData & 0x0F);
			reportTask1.task[pn - 1].sendStart.minute = (*(pData + 1) >> 4) * 10 + (*(pData + 1) & 0x0F);
			reportTask1.task[pn - 1].sendStart.hour = (*(pData + 2) >> 4) * 10 + (*(pData + 2) & 0x0F);
			reportTask1.task[pn - 1].sendStart.day = (*(pData + 3) >> 4) * 10 + (*(pData + 3) & 0x0F);
			reportTask1.task[pn - 1].sendStart.month = ((*(pData + 4) & 0x10) >> 4) * 10 + (*(pData + 4) & 0x0F);
			reportTask1.task[pn - 1].week = *(pData + 4) & 0xE0;
			reportTask1.task[pn - 1].sendStart.year = (*(pData + 5) >> 4) * 10 + (*(pData + 5) & 0x0F);
			pData += 6;
			offset04 += 6;
			
			//曲线数据抽取倍率
			reportTask1.task[pn - 1].sampleTimes = *pData++;
			offset04++;
			if(reportTask1.task[pn - 1].sampleTimes > 96 || reportTask1.task[pn - 1].sampleTimes < 1)
			{
				errFlg = 1;
			}
			
			//数据单元表示个数
			reportTask1.task[pn-1].numOfDataId = *pData++;
			offset04++;
			
			//数据单元标识
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
			
			//如果主站为堆叠发送Fn,Pn的话,更新数据单元个数
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
	
  //初始化任务信息请求
  if (initReportFlag==0)
  {
    initReportFlag = 1;
  }
	
	saveParameter(0x04, 65, (INT8U *)&reportTask1, sizeof(REPORT_TASK_PARA));
  return TRUE;
}

/*******************************************************
函数名称:AFN04066
功能描述:响应主站设置参数命令"定时上报2类数据任务设置(F66)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04066(INT8U *pData)
{	
	REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U  da1, da2;			//任务号
	INT8U  errFlg = 0;
	INT16U numOfItem = 0;    //2014-10-16,add
  INT8U  iOfPn,iOfFn;
  	
	tmpreportTask = reportTask2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
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
			
			//上报周期及发送周期单位
			reportTask2.task[pn - 1].sendPeriod = *pData++;
			offset04++;
			
			//上报基准时间
			reportTask2.task[pn - 1].sendStart.second = (*pData >> 4) * 10 + (*pData & 0x0F);
			reportTask2.task[pn - 1].sendStart.minute = (*(pData + 1) >> 4) * 10 + (*(pData + 1) & 0x0F);
			reportTask2.task[pn - 1].sendStart.hour = (*(pData + 2) >> 4) * 10 + (*(pData + 2) & 0x0F);
			reportTask2.task[pn - 1].sendStart.day = (*(pData + 3) >> 4) * 10 + (*(pData + 3) & 0x0F);
			reportTask2.task[pn - 1].sendStart.month = ((*(pData + 4) & 0x10) >> 4) * 10 + (*(pData + 4) & 0x0F);
			reportTask2.task[pn - 1].week = *(pData + 4) & 0xE0;
			reportTask2.task[pn - 1].sendStart.year = (*(pData + 5) >> 4) * 10 + (*(pData + 5) & 0x0F);
			pData += 6;
			offset04 += 6;
			
			//曲线数据抽取倍率
			reportTask2.task[pn - 1].sampleTimes = *pData++;
			offset04++;
		 #ifndef LIGHTING    //2016-10-25,照明集中器取消这个判断,可以下发倍训练法为254
			if(reportTask2.task[pn - 1].sampleTimes > 96 || reportTask2.task[pn - 1].sampleTimes < 1)
			{
				errFlg = 1;
			}
		 #endif
			
			//数据单元标识个数
			reportTask2.task[pn-1].numOfDataId = *pData++;
			offset04++;
			
			//数据单元标识
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
			
			//如果主站为堆叠发送Fn,Pn的话,更新数据单元个数
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
	
  //初始化任务信息请求
  if (initReportFlag==0)
  {
    initReportFlag = 1;
  }
	
	saveParameter(0x04, 66, (INT8U *)&reportTask2, sizeof(REPORT_TASK_PARA));

  return TRUE;
}

/*******************************************************
函数名称:AFN04067
功能描述:响应主站设置参数命令"定时上报1类数据任务启动/停止设置(F67)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04067(INT8U *pData)
{	
	REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//任务号
	INT8U errFlg = 0;
	
	tmpreportTask = reportTask1;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
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
			
			//启动停止标志
			reportTask1.task[pn - 1].stopFlag = *pData++;
			offset04++;
		}
		da1 >>=1;
	}
	
	saveParameter(0x04, 65, (INT8U *)&reportTask1, sizeof(REPORT_TASK_PARA));
  return TRUE;
}

/*******************************************************
函数名称:AFN04068
功能描述:响应主站设置参数命令"定时上报2类数据任务启动/停止设置(F68)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04068(INT8U *pData)
{	
  REPORT_TASK_PARA tmpreportTask;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//任务号
	INT8U errFlg = 0;
	
	tmpreportTask = reportTask2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
	//指向数据单元
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
			
			//启动停止标志
			reportTask2.task[pn - 1].stopFlag = *pData++;
			offset04++;
		}
		da1 >>=1;
	}
	
	saveParameter(0x04, 66, (INT8U *)&reportTask2, sizeof(REPORT_TASK_PARA));
  return TRUE;
}

/*******************************************************
函数名称:AFN04073
功能描述:响应主站设置参数命令"电容器参数(F73)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04073(INT8U *pData)
{	
	CAPACITY_PARA capacityPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//测量点号
	INT8U errFlg = 0;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			bzero(&capacityPara, sizeof(CAPACITY_PARA));
			
			//从数据库取得已存在的数据
			selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
			
			for(i=0;i<16;i++)
			{
				//补偿方式
				capacityPara.capacity[i].compensationMode = *pData++;
				if((capacityPara.capacity[i].compensationMode & 0xC0) == 0x40
					&& (capacityPara.capacity[i].compensationMode & 0x07) != 0x07)
				{
					errFlg = 1;
				}
				
				//电容装见容量
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
函数名称:AFN04074
功能描述:响应主站设置参数命令"电容器投切运行参数(F74)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04074(INT8U *pData)
{	
	CAPACITY_RUN_PARA capacityRunPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//测量点号
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//目标功率因数
			capacityRunPara.targetPowerFactor[0] = *pData++;
			capacityRunPara.targetPowerFactor[1] = *pData++;
			
			//投入无功功率门限
			for(i=0;i<3;i++)
				capacityRunPara.onPowerLimit[i] = *pData++;
			
			//切除无功功率门限
			for(i=0;i<3;i++)
				capacityRunPara.offPowerLimit[i] = *pData++;
			
			capacityRunPara.delay = *pData++;					//延时时间
			capacityRunPara.actInterval = *pData++;		//动作时间间隔
			
			offset04 += 10;
		  saveViceParameter(0x04, 74, pn, (INT8U *)&capacityRunPara, sizeof(CAPACITY_RUN_PARA));
		}
		da1 >>=1;
	}
	
  return TRUE;
}

/*******************************************************
函数名称:AFN04075
功能描述:响应主站设置参数命令"电容器保护参数(F75)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04075(INT8U *pData)
{	
	CAPACITY_PROTECT_PARA capacityProtectPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//测量点号
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//过电压
			capacityProtectPara.vSuperiod[0] = *pData++;
			capacityProtectPara.vSuperiod[1] = *pData++;
			
			//过电压回差值
			capacityProtectPara.vSuperiodQuan[0] = *pData++;
			capacityProtectPara.vSuperiodQuan[1] = *pData++;
			
			//欠电压
			capacityProtectPara.vLack[0] = *pData++;
			capacityProtectPara.vLack[1] = *pData++;
			
			//欠电压回差值
			capacityProtectPara.vLackQuan[0] = *pData++;
			capacityProtectPara.vLackQuan[1] = *pData++;
			
			//总畸变电流含有率上限
			capacityProtectPara.cAbnormalUpLimit[0] = *pData++;
			capacityProtectPara.cAbnormalUpLimit[1] = *pData++;
			
			//总畸变电流含有率越限回差值
			capacityProtectPara.cAbnormalQuan[0] = *pData++;
			capacityProtectPara.cAbnormalQuan[1] = *pData++;
			
			//总畸变电压含有率上限
			capacityProtectPara.vAbnormalUpLimit[0] = *pData++;
			capacityProtectPara.vAbnormalUpLimit[1] = *pData++;
			
			//总畸变电压含有率越限回差值
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
函数名称:AFN04076
功能描述:响应主站设置参数命令"电容器投切控制方式(F76)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04076(INT8U *pData)
{	
	CAPACITY_PARA capacityPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//测量点号
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
  pData = pDataUnit04;  	
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//控制方式
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
函数名称:AFN04081
功能描述:响应主站设置参数命令"直流模拟量输入变比(F81)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04081(INT8U *pData)
{	
	ADC_PARA accPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//直流模拟量端口号
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//取数据库中的数据
			selectViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
			
			//直流模拟量量程起始值
			accPara.adcStartValue[0] = *pData++;
			accPara.adcStartValue[1] = *pData++;
			
			//直流模拟量量程终止值
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
函数名称:AFN04082
功能描述:响应主站设置参数命令"直流模拟量限值(F82)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04082(INT8U *pData)
{	
	ADC_PARA accPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//直流模拟量端口号
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			bzero(&accPara, sizeof(ADC_PARA));
			
			//取数据库中的数据
			selectViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
			
			//直流模拟量上限
			accPara.adcUpLimit[0] = *pData++;
			accPara.adcUpLimit[1] = *pData++;
			
			//直流模拟量下限
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
函数名称:AFN04083
功能描述:响应主站设置参数命令"直流模拟量冻结参数(F83)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04083(INT8U *pData)
{	
	ADC_PARA accPara;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;

	INT8U da1, da2;			//直流模拟量端口号
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
	
  //指向数据单元
  pData = pDataUnit04;
	
	while(tempPn < 9)
	{
		tempPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = (da2 - 1) * 8 + tempPn;
			
			//取数据库中的数据
			selectViceParameter(0x04, 81, pn, (INT8U *)&accPara, 9);
			
			//直流模拟量量程起始值
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
函数名称:AFN04088
功能描述:响应主站设置参数命令"测量点电能表资产编号(FN88)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04088(INT8U *pData)
{
	INT16U pn = 0;
	INT16U tempPn = 0;
	
	INT8U da1,da2;
	
	//数据单元标识
	da1 = *pData++;
	da2 = *pData++;
	pData += 2;
  
  //指向数据单元
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
函数名称:AFN04097
功能描述:响应主站设置参数命令"设置终端名称(F97)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04097(INT8U *pData)
{
	INT8U i;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
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
函数名称:AFN04098
功能描述:响应主站设置参数命令"设置系统运行标识码(F98)[重庆规约]"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04098(INT8U *pData)
{
	 INT8U i;

 	 //数据单元标识
 	 pData += 4;
 	 
	 //指向数据单元
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
函数名称:AFN04099
功能描述:响应主站设置参数命令"终端抄表搜索持续时间(F99)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04099(INT8U *pData)
{
	 INT8U i;
	 
	 //数据单元标识
	 pData += 4;
	 
	 //指向数据单元
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
函数名称:AFN040100
功能描述:响应主站设置参数命令"终端预设apn(F100)[重庆规约]"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
BOOL AFN04100(INT8U *pData)
{
	INT8U i;
	
	//数据单元标识
	pData += 4;
	
	//指向数据单元
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
函数名称:AFN04121
功能描述:响应主站设置参数命令"终端属性设置<扩展>(F121)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04121(INT8U *pData)
{
   #ifdef TE_ADDR_USE_BCD_CODE
    INT16U tmpAddr;
   #endif

   //数据单元标识
   pData += 4;
	 
	 //指向数据单元
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
函数名称:AFN040129
功能描述:响应主站设置参数命令"终端交流采样校正值<扩展>(F129)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04129(INT8U *pData)
{
 #ifdef AC_SAMPLE_DEVICE 
  offset04 += 81;
  
  //数据单元标识
 	pData += 4;   //指针移至Fn,Pn以后
 	
 	//指向数据单元
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

  resetAtt7022b(TRUE);   //复位ATT7022B且下发校表参数
  resetAtt7022b(TRUE);   //复位ATT7022B且下发校表参数
     
  return TRUE;
 
 #else
 
  return FALSE;
 
 #endif
}

/*******************************************************
函数名称:AFN040131
功能描述:响应主站设置参数命令"终端交流采样校正值<扩展>(F131)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04131(INT8U *pData)
{
	offset04 += 1;

	#ifdef AC_SAMPLE_DEVICE
	  //数据单元标识
	  pData += 4;
	  
	  //指向数据单元
    pData = pDataUnit04;
	  
	  if (*pData==0x55)         //读取校表数据值
	  {
	    acMsa = addrField.a3;
	    
	  	readCheckData = 0x55;
      resetAtt7022b(FALSE);   //复位ATT7022B但不下发校表参数	  	
	  }
	  else                      //停止读取校表数据
	  {
	  	readCheckData = 0x0;
      resetAtt7022b(TRUE);    //复位ATT7022B且下发校表参数
      
      acMsa = 0;
	  }
	  
	  
	  return TRUE;
	#else
	  return FALSE;
	#endif
	
	return TRUE;
}

/*******************************************************
函数名称:AFN040133
功能描述:响应主站设置参数命令"载波主节点地址<沃电扩展>(F133)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04133(INT8U *pData)
{
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
   pData = pDataUnit04;
	 
	 memcpy(mainNodeAddr, pData, 6);
   
   saveParameter(0x04, 133, mainNodeAddr, 6);
	 
	 offset04 += 6;
	 
	 return TRUE;
}

/*******************************************************
函数名称:AFN040134
功能描述:响应主站设置参数命令"设备编号<沃电扩展>(F134)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04134(INT8U *pData)
{
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
   pData = pDataUnit04;
	 
	 deviceNumber = *pData | *(pData+1)<<8;
   
   saveParameter(0x04, 134, (INT8U *)&deviceNumber, 2);
	 
	 offset04 += 2;

	 return TRUE;
}

/*******************************************************
函数名称:AFN040135
功能描述:响应主站设置参数命令"锐拔模块参数<沃电扩展>(F135)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04135(INT8U *pData)
{
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
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
函数名称:AFN040136
功能描述:响应主站设置参数命令"厂商名称<沃电扩展>(F136)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04136(INT8U *pData)
{
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
   pData = pDataUnit04;
	 
	 memcpy(csNameId, pData, 12);
   
   saveParameter(0x04, 136, (INT8U *)&csNameId, 12);
	 
	 offset04 += 12;

	 return TRUE;
}


/*******************************************************
函数名称:AFN040137
功能描述:响应主站设置参数命令"脉冲底数<沃电扩展>(F137)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04137(INT8U *pData)
{
   INT8U  port;
   INT8U  i;
	 
	 //数据单元标识
   port  =  findFnPn(*pData, *(pData+1), FIND_PN);
   if (port>0)
   {
   	 port-=1;
   }
	 
	 //指向数据单元
   pData = pDataUnit04;

   //刷新整数示值
   pulseDataBuff[53*port]   = *(pData+1);
   pulseDataBuff[53*port+1] = *(pData+2);
   pulseDataBuff[53*port+2] = *(pData+3);
   
   //刷新小数
   pulse[port].pulseCount = (*pData)*pulse[port].meterConstant/100;
   
   //保存脉冲量数据缓存
 	 saveParameter(88, 3, pulseDataBuff, NUM_OF_SWITCH_PULSE*53);
 	 
 	 //保存脉冲计数
 	 saveParameter(88, 13, pulse, sizeof(ONE_PULSE)*NUM_OF_SWITCH_PULSE);

	 offset04 += 4;

	 return TRUE;
}

/*******************************************************
函数名称:AFN040138
功能描述:响应主站设置参数命令"居民用户表数据类型<扩展>(F138)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04138(INT8U *pData)
{
	 //数据单元标识
	 pData+=4;
	 
	 //指向数据单元
   pData = pDataUnit04;
	 
	 denizenDataType = *pData;
	 
   saveParameter(0x04, 138, (INT8U *)&denizenDataType, 1);
	 
	 offset04 += 1;

	 return TRUE;
}

#ifdef SDDL_CSM

/*******************************************************
函数名称:AFN04224
功能描述:响应主站设置参数命令"终端地址码设置<扩展>(F224)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL AFN04224(INT8U *pData)
{
  //数据单元标识
  pData += 4;
	 
	//指向数据单元
  pData = pDataUnit04;
  
  //行政区划
  addrField.a1[0] = *pData++;
  addrField.a1[1] = *pData++;
   
  //终端地址
  addrField.a2[0] = *pData++;
  addrField.a2[1] = *pData++;
  
  saveParameter(0x04, 121,(INT8U *)&addrField,4);
  saveBakKeyPara(121);
  
  //终端供电方式
  saveParameter(0x04, 224, pData, 1);
  
  offset04 += 5;
  
  return TRUE;
}

#endif
