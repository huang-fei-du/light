/***************************************************
Copyright,2010,Huawei WoDian co.,LTD
文件名：AFN0A.c
作者：wan guihua
版本：0.9
完成日期：2010年1月
描述：主站AFN0A(查询参数)处理文件。
函数列表：
  01,10-1-13,Leiyong created.
***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msSetPara.h"
#include "teRunPara.h"
#include "dataBase.h"

#ifdef LIGHTING
 #include "copyMeter.h"
#endif

#include "AFN0A.h"
#include "AFN00.h"

INT16U offset0a;                     //接收到的帧中的数据单元偏移量(数据标识的字节)

/*******************************************************
函数名称:AFN0A
功能描述:接收查询参数分组(AFN0A)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN0A(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
    INT16U frameTail0a;             //分组尾
    INT16U tmpI,tmpFrameTail, tmpHead0a;
    INT8U  frameCounter,checkSum;   //发送计数器 
    INT8U  fn, ackTail;
    INT8U  tmpDtCount;              //DT移位计数
    INT8U  tmpDt1;                  //临时DT1
    INT8U  *pTpv;                   //TpV指针
    INT8U  maxCycle;                //最大循环次数 
    
    INT16U (*AFN0AFun[138])(INT16U tail,INT8U *pHandle);

    for(tmpI=0;tmpI<138;tmpI++)
    {
      AFN0AFun[tmpI] = NULL;
    }
    
    //组1  
    AFN0AFun[0] = AFN0A001;
    AFN0AFun[1] = AFN0A002;
    AFN0AFun[2] = AFN0A003;
    AFN0AFun[3] = AFN0A004;
    AFN0AFun[4] = AFN0A005;
    AFN0AFun[5] = AFN0A006;
    AFN0AFun[6] = AFN0A007;
    AFN0AFun[7] = AFN0A008;
    
    //组2
    AFN0AFun[8]  = AFN0A009;
    AFN0AFun[9]  = AFN0A010;
    AFN0AFun[10] = AFN0A011;
    AFN0AFun[11] = AFN0A012;
    AFN0AFun[12] = AFN0A013;
    AFN0AFun[13] = AFN0A014;
    AFN0AFun[14] = AFN0A015;
    AFN0AFun[15] = AFN0A016;
    
    //组3
    AFN0AFun[16] = AFN0A017;
    AFN0AFun[17] = AFN0A018;
    AFN0AFun[18] = AFN0A019;
    AFN0AFun[19] = AFN0A020;
    AFN0AFun[20] = AFN0A021;
    AFN0AFun[21] = AFN0A022;
    AFN0AFun[22] = AFN0A023;

    //组4
    AFN0AFun[24] = AFN0A025;
    AFN0AFun[25] = AFN0A026;
    AFN0AFun[26] = AFN0A027;
    AFN0AFun[27] = AFN0A028;
   
    AFN0AFun[28] = AFN0A029;
    AFN0AFun[29] = AFN0A030;
    AFN0AFun[30] = AFN0A031;
    
    //组5
    AFN0AFun[32] = AFN0A033;
    
    AFN0AFun[33] = AFN0A034;
    AFN0AFun[34] = AFN0A035;
    AFN0AFun[35] = AFN0A036;
    AFN0AFun[36] = AFN0A037;
    AFN0AFun[37] = AFN0A038;
    AFN0AFun[38] = AFN0A039;
    
    //组6
    AFN0AFun[40] = AFN0A041;
    AFN0AFun[41] = AFN0A042;
    AFN0AFun[42] = AFN0A043;
    AFN0AFun[43] = AFN0A044;
    AFN0AFun[44] = AFN0A045;
    AFN0AFun[45] = AFN0A046;
    AFN0AFun[46] = AFN0A047;
    AFN0AFun[47] = AFN0A048;
    
    //组7
    AFN0AFun[48] = AFN0A049;
    
   #ifdef LIGHTING
    AFN0AFun[49] = AFN0A050;
    AFN0AFun[50] = AFN0A051;
    AFN0AFun[51] = AFN0A052;
   #endif
    
    //组8
    AFN0AFun[56] = AFN0A057;
    AFN0AFun[57] = AFN0A058;
    AFN0AFun[58] = AFN0A059;
    AFN0AFun[59] = AFN0A060;
    AFN0AFun[60] = AFN0A061;
    
    //组9
    AFN0AFun[64] = AFN0A065;
    AFN0AFun[65] = AFN0A066;
    AFN0AFun[66] = AFN0A067;
    AFN0AFun[67] = AFN0A068;
    
    //组10
    AFN0AFun[72] = AFN0A073;
    AFN0AFun[73] = AFN0A074;
    AFN0AFun[74] = AFN0A075;
    AFN0AFun[75] = AFN0A076;
 
    //组11
    AFN0AFun[80] = AFN0A081;
    AFN0AFun[81] = AFN0A082;
    AFN0AFun[82] = AFN0A083;

    //扩展
   #ifdef SDDL_CSM
    AFN0AFun[87] = AFN0A088;
   #endif
    AFN0AFun[96] = AFN0A097;
    AFN0AFun[97] = AFN0A098;
    
    AFN0AFun[98] = AFN0A099;
    AFN0AFun[99] = AFN0A100;
   
    AFN0AFun[120] = AFN0A121;
    
    AFN0AFun[132] = AFN0A133;
    AFN0AFun[133] = AFN0A134;
    AFN0AFun[134] = AFN0A135;
    AFN0AFun[135] = AFN0A136;
    
    AFN0AFun[137] = AFN0A138;
    
    if (fQueue.tailPtr == 0)
    {
       tmpHead0a = 0;
    }
    else
    {
       tmpHead0a = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    frameTail0a = tmpHead0a+14;    
    
    frameCounter = 0;
    tmpDt1 = 0;
    tmpDtCount = 0;
   
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    maxCycle = 0;
    while ((frame.loadLen>0) && (maxCycle<500))
    {
       maxCycle++;
       
       offset0a = 0;
       tmpDt1 = *(pDataHead+2);
       tmpDtCount = 0;
       while(tmpDtCount<9)
       {    
           tmpDtCount++;
           if ((tmpDt1 & 0x1) == 0x1)
           {
           	 fn = *(pDataHead+3)*8 + tmpDtCount;
           	#ifdef SDDL_CSM
           	 if (fn==224)
           	 {
               frameTail0a = AFN0A224(frameTail0a,pDataHead);
           	 }
           	 else
           	 {
           	#endif

           	   //2013-11-28添加这个判断
           	   if(fn>138)
           	   {
           	   	 maxCycle = 500;
      	         break;
           	   }
           	   	
           	   //执行函数
               if ((AFN0AFun[fn-1] != NULL) 
              	  && (fn<=83 
              	     #ifdef SDDL_CSM
              	      || 88==fn
              	     #endif
              	       || fn == 92 || fn == 97 || fn == 98 || fn == 99 || fn==100 || fn==121 || fn==133 || fn==134 || fn==135 || fn==136 || fn==138))
               {
                 if ((tmpFrameTail = AFN0AFun[fn-1](frameTail0a,pDataHead))== frameTail0a)
                 {
                 	 ackData[ackTail*5]   = *pDataHead;                         //DA1
                 	 ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
                 	 ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
                 	 ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
                 	 ackData[ackTail*5+4] = 0x01;                               //无有效数据
                 	 ackTail++;
                 }
                 else
                 {
                 	 frameTail0a = tmpFrameTail;
                 }
               }
            #ifdef SDDL_CSM
             }
            #endif
           }
           
           tmpDt1 >>= 1;
           
           if (((frameTail0a - tmpHead0a) > MAX_OF_PER_FRAME) || (((pDataHead+offset0a+4) == pDataEnd) && tmpDtCount==8))
           {
               //不允许主动上报且有事件发生
               if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail0a - tmpHead0a) > 16)
               {
              	   msFrame[frameTail0a++] = iEventCounter;
              	   msFrame[frameTail0a++] = nEventCounter;
               }

               //根据启动站要求判断是否携带TP
               if (frame.pTp != NULL)
               {
                  pTpv = frame.pTp;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv++;
                  msFrame[frameTail0a++] = *pTpv;
               }
               
               msFrame[tmpHead0a + 0] = 0x68;   //帧起始字符
             
               tmpI = ((frameTail0a - tmpHead0a -6) << 2) | PROTOCOL_FIELD;
               msFrame[tmpHead0a + 1] = tmpI & 0xFF;   //L
               msFrame[tmpHead0a + 2] = tmpI >> 8;
               msFrame[tmpHead0a + 3] = tmpI & 0xFF;   //L
               msFrame[tmpHead0a + 4] = tmpI >> 8; 
             
               msFrame[tmpHead0a + 5] = 0x68;  //帧起始字符
        
        			 //控制域
               if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
               {
                  msFrame[tmpHead0a + 6] = 0xa8;  //控制字节10001000
               }
               else
               {
               	  msFrame[tmpHead0a + 6] = 0x88;  //控制字节10001000
               }
               	 
               //地址域
               msFrame[tmpHead0a + 7] = addrField.a1[0];
               msFrame[tmpHead0a + 8] = addrField.a1[1];
               msFrame[tmpHead0a + 9] = addrField.a2[0];
               msFrame[tmpHead0a + 10] = addrField.a2[1];
               msFrame[tmpHead0a + 11] = addrField.a3;
       
               msFrame[tmpHead0a + 12] = 0x0A;  //AFN
        
               if ((pDataHead+4) == pDataEnd)
               {
                  if (frameCounter == 0)
                  {
                    msFrame[tmpHead0a + 13] = 0x60 | rSeq;    //01100000 | rSeq 单帧
                  }
                  else
                  {
                    msFrame[tmpHead0a + 13] = 0x20 | rSeq;    //00100000 | rSeq 最后一帧
                  }
               }
               else
               {
                 if (frameCounter == 0)
                 {
                   msFrame[tmpHead0a + 13] = 0x40 | rSeq;     //01000000 | rSeq  第一帧
                 }
                 else
                 {
                   msFrame[tmpHead0a + 13] = 0x00 | rSeq;     //00000000 | rSeq 中间帧
                 }
                 frameCounter++;
               }

               if (frame.pTp != NULL)
               {
              	  msFrame[tmpHead0a+13] |= 0x80;       //TpV置位
               }
                            
               tmpI = tmpHead0a + 6;
               checkSum = 0;
               while (tmpI < frameTail0a)
               {
                 checkSum = msFrame[tmpI] + checkSum;
                 tmpI++;
               }
               
               msFrame[frameTail0a++] = checkSum;
               msFrame[frameTail0a++] = 0x16;
               
               fQueue.frame[fQueue.tailPtr].head = tmpHead0a;
               fQueue.frame[fQueue.tailPtr].len = frameTail0a-tmpHead0a;
               
               if (((frameTail0a - tmpHead0a) > 16 && frame.pTp==NULL)||((frameTail0a - tmpHead0a) > 22 && frame.pTp!=NULL))
               {
                 tmpHead0a = frameTail0a;
                 if ((tmpHead0a+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                 	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                 {
                    fQueue.frame[fQueue.tailPtr].next = 0x0;
                 	  fQueue.tailPtr = 0;
                 	  tmpHead0a = 0;
                 }
                 else
                 {                 
                    fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                    fQueue.tailPtr++;
                 }

                 frameTail0a = tmpHead0a + 14;  //frameTail重新置为14填写下一帧
               }
           }
       }
       
       if (frame.loadLen<(offset0a+4))
       {
       	 break;
       }
       else
       {
         frame.loadLen -= (offset0a + 4);
         pDataHead += offset0a + 4;
       }
   }   
   
   if (ackTail !=0)
   {     	           
     AFN00003(ackTail, dataFrom, 0x0a);
   }
}

/*******************************************************
函数名称:AFN0A001
功能描述:响应主站查询参数命令"终端上行通信口通信参数(F1)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A001(INT16U frameTail,INT8U *pHandle)
{  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle;    //DT2
    
  //数据单元
  msFrame[frameTail++] = commPara.rts;
  msFrame[frameTail++] = commPara.delay;
  msFrame[frameTail++] = commPara.timeOutReSendTimes[0];
  msFrame[frameTail++] = commPara.timeOutReSendTimes[1];
  msFrame[frameTail++] = commPara.flagOfCon;
  msFrame[frameTail++] = commPara.heartBeat;
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A002
功能描述:响应主站查询参数命令"终端上行通信口无线中继转发设置(F2)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A002(INT16U frameTail,INT8U *pHandle)
{
	  INT8U  num;
	  
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  msFrame[frameTail++] = relayConfig.relayAddrNumFlg;
	  
	  for (num = 0; num < (relayConfig.relayAddrNumFlg & 0x7F); num++)
	  {
	      msFrame[frameTail++] = relayConfig.relayAddr[num][0];
	      msFrame[frameTail++] = relayConfig.relayAddr[num][1];
	  }
	  
	  return frameTail;
}

/*******************************************************
函数名称:AFN0A003
功能描述:响应主站查询参数命令"主站IP地址和端口(F3)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A003(INT16U frameTail,INT8U *pHandle)
{  
  INT8U i;
  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle;    //DT2
    
  //数据单元
  //主用IP地址和端口
  for (i=0;i<4;i++)
  {
    msFrame[frameTail++] = ipAndPort.ipAddr[i];
  }
  msFrame[frameTail++] = ipAndPort.port[0];
  msFrame[frameTail++] = ipAndPort.port[1];
  
  //备用IP地址和端口
  for (i=0;i<4;i++)
  {
    msFrame[frameTail++] = ipAndPort.ipAddrBak[i];
  }
  msFrame[frameTail++] = ipAndPort.portBak[0];
  msFrame[frameTail++] = ipAndPort.portBak[1];
  
  //APN
  for (i=0;i<16;i++)
  {
    msFrame[frameTail++] = ipAndPort.apn[i];
  }

  return frameTail;
}

/*******************************************************
函数名称:AFN0A004
功能描述:响应主站查询"主站电话号码和短信中心号码"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A004(INT16U frameTail,INT8U *pHandle)
{
    INT8U i;
    
    //数据单元标识
	  msFrame[frameTail++] = *pHandle++;     //DA1
	  msFrame[frameTail++] = *pHandle++;     //DA2
	  msFrame[frameTail++] = *pHandle++;     //DT1
	  msFrame[frameTail++] = *pHandle;       //DT2
	
	  //主站电话号码
	  for(i=0;i<8;i++)
	    msFrame[frameTail++] = phoneAndSmsNumber.phoneNumber[i];
	
	  //主站短信中心号码
	  for(i=0;i<8;i++)
	    msFrame[frameTail++] = phoneAndSmsNumber.smsNumber[i];
	
    return frameTail;
}

/*******************************************************
函数名称:AFN0A005
功能描述:响应主站查询"终端上行通信消息认证参数设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A005(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  //消息认证方案号
	  msFrame[frameTail++] = messageAuth[0];
    
    //消息认证方案参数
    msFrame[frameTail++] = messageAuth[1];
    msFrame[frameTail++] = messageAuth[2];
    
    return frameTail;
}

/*******************************************************
函数名称:AFN0A006
功能描述:响应主站查询参数命令"终端组地址设置(F6)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A006(INT16U frameTail,INT8U *pHandle)
{   
  INT8U i;
  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2
    
  //数据单元 
  for(i=0;i<16;i++)
  {
    msFrame[frameTail++] = groupAddr[i];
  }
   
  return frameTail;
}

/*******************************************************
函数名称:AFN0A007
功能描述:响应主站查询"终端IP地址和端口"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A007(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	
	//数据单元标识
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2
	
	//终端IP地址
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.teIpAddr[i];
	}
	
	//子网掩码地址
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.mask[i];
	}
	
	//网关地址
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.gateWay[i];
	}
	
	//代理类型
	msFrame[frameTail++] = teIpAndPort.proxyType;
	
	//代理服务器地址
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = teIpAndPort.proxyServer[i];
	}
	
	//代理服务器端口
	msFrame[frameTail++] = teIpAndPort.proxyPort[0];
	msFrame[frameTail++] = teIpAndPort.proxyPort[1];
	
	//代理服务器连接方式
	msFrame[frameTail++] = teIpAndPort.proxyLinkType;
	
	//用户名长度
	msFrame[frameTail++] = teIpAndPort.userNameLen;
	
	//用户名
	for(i=0;i<teIpAndPort.userNameLen;i++)
	{
		msFrame[frameTail++] = teIpAndPort.userName[i];
	}
	
	//密码长度
	msFrame[frameTail++] = teIpAndPort.passwordLen;
	
	//密码
	for(i=0;i<teIpAndPort.passwordLen;i++)
	{
		msFrame[frameTail++] = teIpAndPort.password[i];
	}
	
	//侦听端口
	msFrame[frameTail++] = teIpAndPort.listenPort[0];
	msFrame[frameTail++] = teIpAndPort.listenPort[1];
	
  return frameTail;
}

/*******************************************************
函数名称:AFN0A008
功能描述:响应主站查询"终端上行通信工作方式"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A008(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	 
	//数据单元标识
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2
	
	//工作模式	
	msFrame[frameTail++] = tePrivateNetMethod.workMethod;
	 
	//永久在线，时段在线模式重拨间隔
	msFrame[frameTail++] = tePrivateNetMethod.redialInterval[0];
	msFrame[frameTail++] = tePrivateNetMethod.redialInterval[1];
	 
	//被动激活模式重拨次数
	msFrame[frameTail++] = tePrivateNetMethod.maxRedial;
	 
	//被动激活模式连续无通信自动断线时间
	msFrame[frameTail++] = tePrivateNetMethod.closeConnection;
	 
	//时段在线模式允许在线时段标志
	for(i=0;i<3;i++)
	{
	  msFrame[frameTail++] = tePrivateNetMethod.onLinePeriodTime[i];
	}
	 
  return frameTail;
}

/*******************************************************
函数名称:AFN0A009
功能描述:响应主站查询"终端事件记录配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A009(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	
	//数据单元标识
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2
	
	//数据单元
	//事件记录有效标志位
	for(i=0;i<8;i++)
	{
		msFrame[frameTail++] = eventRecordConfig.nEvent[i];
	}
	
	//事件重要性等级标志位
	for(i=0;i<8;i++)
	{
		msFrame[frameTail++] = eventRecordConfig.iEvent[i];
	}
   
  return frameTail;
}

/*******************************************************
函数名称:AFN0A010
功能描述:响应主站查询"终端电能表/交流采样装置配置参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A010(INT16U frameTail,INT8U *pHandle)
{
  METER_DEVICE_CONFIG meterDeviceConfig;

  INT16U i,j;
  
  INT16U num1, num2;
  INT16U tmpFrameTail;
  
  INT8U result;
  
  tmpFrameTail = frameTail;
  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle++;    //DT2
 
  num1 = *pHandle++;
  num1 |= (*pHandle++) << 8;
  msFrame[frameTail++] = num1;    			 //查询的数量
  msFrame[frameTail++] = num1 >> 8;
  offset0a += 2;
 
  if(num1 > 200)
  {
 	  return tmpFrameTail;
  }

  //数据单元	
  for(i=0;i<num1;i++)
  {
  	bzero(&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG));
  	   
  	num2 = *pHandle++;
  	num2 |= (*pHandle++)<<8;	   
  	offset0a += 2;
  	   
  	//2012-09-21,在鲁能星城发现,集中器上能读出表地址,而用主站召测全是0
  	//           原因为库中有一个测量点号为0的测量点信息,修改这个错误
  	//result = selectF10Data(0, 0, num2, (INT8U *)&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG));
  	result = selectF10Data(0xfffe, 0, num2, (INT8U *)&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG));

  	if(result == 1)
	  {
	   	msFrame[frameTail++] = meterDeviceConfig.number & 0xFF;							//序号
	    msFrame[frameTail++] = meterDeviceConfig.number>>8;
	    msFrame[frameTail++] = meterDeviceConfig.measurePoint & 0xFF;				//测量点号
	    msFrame[frameTail++] = meterDeviceConfig.measurePoint>>8;
	    msFrame[frameTail++] = meterDeviceConfig.rateAndPort;								//通信速率及端口号
	    msFrame[frameTail++] = meterDeviceConfig.protocol;										//协议类型
		
	    //通信地址
	    for(j=0;j<6;j++)
	    {
	 		  msFrame[frameTail++] = meterDeviceConfig.addr[j];
	    }
	 
	    //通信密码
	    for(j=0;j<6;j++)
	    {
	 		  msFrame[frameTail++] = meterDeviceConfig.password[j];
	    }
	 
	    msFrame[frameTail++] = meterDeviceConfig.numOfTariff;								//电能费率个数
	    msFrame[frameTail++] = meterDeviceConfig.mixed;											//有功电能示值整数位及小数位
	 
	    //所属采集器通信地址
	    for(j=0;j<6;j++)
	    {
	 		  msFrame[frameTail++] = meterDeviceConfig.collectorAddr[j];
	    }
	 
	    msFrame[frameTail++] = meterDeviceConfig.bigAndLittleType;						//用户大类及用户小类号
    }
    else
    {
    	for(j=0;j<27;j++)
    	{
    	 	msFrame[frameTail++] = 0xEE;
    	}
    }
  }

  return frameTail;
}

/*******************************************************
函数名称:AFN0A011
功能描述:响应主站查询"终端脉冲配置参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A011(INT16U frameTail,INT8U *pHandle)
{
  INT8U  num;
  INT16U count = 0;
  INT8U  i;
  
  msFrame[frameTail++] = *pHandle++;		//DA1
  msFrame[frameTail++] = *pHandle++;		//DA2
  msFrame[frameTail++] = *pHandle++;		//DT1
  msFrame[frameTail++] = *pHandle++;		//DT2
  
  count = frameTail++;	  
  
  num = *pHandle++;							//本次查询数量
  
  offset0a += 1;
  msFrame[count] = 0;           //本次查询到的数量
  for(i=0;i<num;i++)
  {
  	if(*pHandle <= pulseConfig.numOfPulse && *pHandle > 0)
  	{
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].ifNo;							//脉冲输入端口号
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].pn;								//脉冲输入端口号
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].character;				//脉冲输入端口号
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].meterConstant[0];	//脉冲输入端口号
  		msFrame[frameTail++] = pulseConfig.perPulseConfig[*pHandle - 1].meterConstant[1];	//脉冲输入端口号
  		
  		msFrame[count]++;
  	}
  	
  	//如果要查询的序号大于已存在的数量，丢弃
  	pHandle++;	  	
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
  {
	  frameTail -= 5;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A012
功能描述:响应主站查询"终端状态量输入参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A012(INT16U frameTail,INT8U *pHandle)
{
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	  
	msFrame[frameTail++] = statusInput[0];	//状态量接入标志位
	msFrame[frameTail++] = statusInput[1];	//状态量属性标志位
	  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A013
功能描述:响应主站查询"终端电压/电流模拟配置参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A013(INT16U frameTail,INT8U *pHandle)
{
  INT8U  num;
  INT16U count = 0;
  INT8U  i;
  
  msFrame[frameTail++] = *pHandle++;		//DA1
  msFrame[frameTail++] = *pHandle++;		//DA2
  msFrame[frameTail++] = *pHandle++;		//DT1
  msFrame[frameTail++] = *pHandle++;		//DT2
  
  count = frameTail++;
  msFrame[count] = 0;
  num = *pHandle++;
  offset0a += 1;
  
  for (i=0; i<num; i++)
  {
  	if(*pHandle <= simuIUConfig.numOfSimu && *pHandle > 0)
  	{
  		msFrame[frameTail++] = simuIUConfig.perIUConfig[*pHandle - 1].ifNo;			//电压/电流模拟量输入端口号
     	msFrame[frameTail++] = simuIUConfig.perIUConfig[*pHandle - 1].pn;				//所属测量点号
     	msFrame[frameTail++] = simuIUConfig.perIUConfig[*pHandle - 1].character;//电压电流模拟量属性
     	pHandle++;
     	msFrame[count]++;
  	}
  	else
  	{
  		//如果要查询的序号大于已存在的数量，丢弃
  		pHandle++;
  	}
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
  {
	  frameTail -= 5;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A014
功能描述:响应主站查询"终端总加组配置参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A014(INT16U frameTail,INT8U *pHandle)
{
	INT8U  num1, num2;
	INT8U  i, j;
	INT16U count = 0;
	  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle++;    //DT2

  count = frameTail++;
  msFrame[count] = 0;
  num1 = *pHandle++;
  offset0a += 1;
   
  for(i=0;i<num1;i++)
  {
  	if(*pHandle <= totalAddGroup.numberOfzjz && *pHandle > 0)
  	{
  		msFrame[frameTail++] = totalAddGroup.perZjz[*pHandle - 1].zjzNo;			  //总加组序号
  		msFrame[frameTail++] = totalAddGroup.perZjz[*pHandle - 1].pointNumber;  //本总加组测量点数量
  		num2 = totalAddGroup.perZjz[*pHandle - 1].pointNumber;
  		
  		//测量点号及总加标志
  		for(j=0;j<num2;j++)
  		{
  			msFrame[frameTail++] = totalAddGroup.perZjz[*pHandle - 1].measurePoint[j];
  		}
  		
  		msFrame[count] ++;
  		pHandle++;
  	}
  	else
  	{
  		//如果要查询的序号大于已存在的数量，丢弃
	  	pHandle++;
  	} 
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
	{
		frameTail -= 5;
	}
  	
  return frameTail;
}

/*******************************************************
函数名称:AFN0A015
功能描述:响应主站查询"有功总电能量差动越限事件参数设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A015(INT16U frameTail,INT8U *pHandle)
{
	INT8U  num;
	INT16U count = 0;
	INT8U  i, j;
	  
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	  
	count = frameTail++;
	msFrame[count] = 0;
	num = *pHandle++;
	offset0a += 1;
	  
  for (i = 0; i < num; i++)
  {
  	if(*pHandle <= differenceConfig.numOfConfig && *pHandle > 0)
  	{
  		msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].groupNum;						//有功总电能量差动组序号
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].toCompare;					//对比的总加组序号
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].toReference;				//参照的总加组序号
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].timeAndFlag;			 	//参与差动的电能量的时间区间及对比方法标志 
    	msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].ralitaveDifference;	//差动越限相对偏差值
			
			//差动越限绝对偏差值
			for(j=0;j<4;j++)
			{
				msFrame[frameTail++] = differenceConfig.perConfig[*pHandle - 1].absoluteDifference[j];
			}
			
			msFrame[count]++;
			pHandle++;    	
  	}
  	else
  	{
  		//如果要查询的序号大于已存在的数量，丢弃
	  	pHandle++;
  	}
  	offset0a += 1;
  }
  
  if(msFrame[count] == 0)
	{
		frameTail -= 5;
	}
    
  return frameTail;
}

/*******************************************************
函数名称:AFN0A016
功能描述:响应主站查询"虚拟专网用户名、密码"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A016(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;
  
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  
  //虚拟专网用户名
  for(i=0;i<32;i++)
  {
    msFrame[frameTail++] = vpn.vpnName[i];
  }

	//虚拟专网密码
  for(i=0;i<32;i++)
  {
    msFrame[frameTail++] = vpn.vpnPassword[i];
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A017
功能描述:响应主站查询"终端保安定值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A017(INT16U frameTail,INT8U *pHandle)
{
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	  
	msFrame[frameTail++] = protectLimit[0];
	msFrame[frameTail++] = protectLimit[1];
	  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A018
功能描述:响应主站查询"终端功控时段"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A018(INT16U frameTail,INT8U *pHandle)
{
	INT8U i;
	  
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	msFrame[frameTail++] = *pHandle++;
	  
	for (i = 0; i < 12; i++)
	{
	  msFrame[frameTail++] = ctrlPara.pCtrlPeriod[i];
	}
	  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A019
功能描述:响应主站查询"终端时段功控定值浮动系数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A019(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  msFrame[frameTail++] = ctrlPara.pCtrlIndex;
	  
	  return frameTail;
}

/*******************************************************
函数名称:AFN0A020
功能描述:响应主站查询"终端月电能量控定值浮动系数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A020(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
    msFrame[frameTail++] = ctrlPara.monthEnergCtrlIndex;
	  
	  return frameTail;
}

/*******************************************************
函数名称:AFN0A021
功能描述:响应主站查询"终端电能量费率时段和费率数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A021(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;

  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2

  //数据单元	
  for(i=0;i<49;i++)
  {
    msFrame[frameTail++] = periodTimeOfCharge[i];
  }
    		
  return frameTail;
}

/*******************************************************
函数名称:AFN0A022
功能描述:响应主站查询"终端电能量费率"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A022(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;

  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2

  //数据单元	
  msFrame[frameTail++] = chargeRateNum.chargeNum;	//费率数
  
  //费率
  for(i=0;i<chargeRateNum.chargeNum;i++)
  {
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][0];
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][1];
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][2];
    msFrame[frameTail++] = chargeRateNum.chargeRate[i][3];
  }
    		
  return frameTail;
}

/*******************************************************
函数名称:AFN0A023
功能描述:响应主站查询"催费告警参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A023(INT16U frameTail,INT8U *pHandle)
{
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  msFrame[frameTail++] = *pHandle++;
	  
	  //催费告警允许禁止标志位
	  msFrame[frameTail++] = chargeAlarm[0];
	  msFrame[frameTail++] = chargeAlarm[1];
	  msFrame[frameTail++] = chargeAlarm[2];
	  
	  return frameTail;
}

/*******************************************************
函数名称:AFN0A025
功能描述:响应主站查询"测量点基本参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A025(INT16U frameTail,INT8U *pHandle)
{
	MEASURE_POINT_PARA pointPara; 
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 25, pn, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
			{
				//电压互感器倍率
				msFrame[frameTail++] = pointPara.voltageTimes & 0xFF;
				msFrame[frameTail++] = pointPara.voltageTimes >> 8;
		
				//电流互感器倍率
				msFrame[frameTail++] = pointPara.currentTimes & 0xFF;
				msFrame[frameTail++] = pointPara.currentTimes >> 8;
		
				//额定电压
				msFrame[frameTail++] = pointPara.ratingVoltage & 0xFF;
				msFrame[frameTail++] = pointPara.ratingVoltage >> 8;
		
				//额定电流
				msFrame[frameTail++] = pointPara.maxCurrent;
		
				//额定负荷
				msFrame[frameTail++] = pointPara.powerRating[0];
				msFrame[frameTail++] = pointPara.powerRating[1];
				msFrame[frameTail++] = pointPara.powerRating[2];
		
				//电源接线方式
				msFrame[frameTail++] = pointPara.linkStyle;
			}
			else
			{
				for(i=0;i<11;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A026
功能描述:响应主站查询"测量点限值参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A026(INT16U frameTail,INT8U *pHandle)
{
	MEASUREPOINT_LIMIT_PARA pointLimitPara; 
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 26, pn, (INT8U *)&pointLimitPara, sizeof(MEASUREPOINT_LIMIT_PARA)) == TRUE)
			{
				//电压合格率辨别参数
				msFrame[frameTail++] = pointLimitPara.vUpLimit & 0xFF;				//电压合格上限
				msFrame[frameTail++] = pointLimitPara.vUpLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vLowLimit & 0xFF;				//电压合格下限
				msFrame[frameTail++] = pointLimitPara.vLowLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vPhaseDownLimit & 0xFF;	//电压断相门限
				msFrame[frameTail++] = pointLimitPara.vPhaseDownLimit >> 8;
		
				//过压辨别参数
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit & 0xFF;	//电压上上限
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vUpUpTimes;							//越限持续时间
				msFrame[frameTail++] = pointLimitPara.vUpUpResume[0];					//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.vUpUpResume[1];
		
				//欠压辨别参数
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit & 0xFF;	//电压下下限
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit >> 8;
				msFrame[frameTail++] = pointLimitPara.vDownDownTimes;			//越限持续时间
				msFrame[frameTail++] = pointLimitPara.vDownDownResume[0];	//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.vDownDownResume[1];
		
				//过流辨别参数
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[0];	//相电流上上限
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[1];
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[2];
				msFrame[frameTail++] = pointLimitPara.cUpUpTimes;					//越限持续时间
				msFrame[frameTail++] = pointLimitPara.cUpUpReume[0];			//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.cUpUpReume[1];
		
				//超额定辨别参数
				msFrame[frameTail++] = pointLimitPara.cUpLimit[0];				//相电流上限
				msFrame[frameTail++] = pointLimitPara.cUpLimit[1];
				msFrame[frameTail++] = pointLimitPara.cUpLimit[2];
				msFrame[frameTail++] = pointLimitPara.cUpTimes;						//越限持续时间
				msFrame[frameTail++] = pointLimitPara.cUpResume[0];				//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.cUpResume[1];
		
				//零序电流超限辨别参数
				msFrame[frameTail++] = pointLimitPara.cZeroSeqLimit[0];		//零序电流上限
				msFrame[frameTail++] = pointLimitPara.cZeroSeqLimit[1];
				msFrame[frameTail++] = pointLimitPara.cZeroSeqLimit[2];
				msFrame[frameTail++] = pointLimitPara.cZeroSeqTimes;			//越限持续时间
				msFrame[frameTail++] = pointLimitPara.cZeroSeqResume[0];	//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.cZeroSeqResume[1];
		
				//视在功率超上上限辨别参数
				msFrame[frameTail++] = pointLimitPara.pSuperiodLimit[0];	//视在功率上上限
				msFrame[frameTail++] = pointLimitPara.pSuperiodLimit[1];
				msFrame[frameTail++] = pointLimitPara.pSuperiodLimit[2];
				msFrame[frameTail++] = pointLimitPara.pSuperiodTimes;			//越限持续时间
				msFrame[frameTail++] = pointLimitPara.pSuperiodResume[0];	//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.pSuperiodResume[1];
		
				//视在功率超上限辨别参数
				msFrame[frameTail++] = pointLimitPara.pUpLimit[0];				//视在功率上限
				msFrame[frameTail++] = pointLimitPara.pUpLimit[1];
				msFrame[frameTail++] = pointLimitPara.pUpLimit[2];
				msFrame[frameTail++] = pointLimitPara.pUpTimes;						//越限持续时间
				msFrame[frameTail++] = pointLimitPara.pUpResume[0];				//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.pUpResume[1];
		
				//三相电压不平衡超限辨别参数
				msFrame[frameTail++] = pointLimitPara.uPhaseUnbalance[0];	//三相电压不平衡限值
				msFrame[frameTail++] = pointLimitPara.uPhaseUnbalance[1];
				msFrame[frameTail++] = pointLimitPara.uPhaseUnTimes;			//越限持续时间
				msFrame[frameTail++] = pointLimitPara.uPhaseUnResume[0];	//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.uPhaseUnResume[1];
		
				//三相电流不平衡超限辨别参数
				msFrame[frameTail++] = pointLimitPara.cPhaseUnbalance[0];	//三相电流不平衡限值
				msFrame[frameTail++] = pointLimitPara.cPhaseUnbalance[1];
				msFrame[frameTail++] = pointLimitPara.cPhaseUnTimes;			//越限持续时间
				msFrame[frameTail++] = pointLimitPara.cPhaseUnResume[0];	//越限恢复系数
				msFrame[frameTail++] = pointLimitPara.cPhaseUnResume[1];
		
				//连续失压时间限值
				msFrame[frameTail++] = pointLimitPara.uLostTimeLimit;
			}
			else
			{
				for(i=0;i<57;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			} 
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A027
功能描述:响应主站查询"测量点铜损，铁损参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A027(INT16U frameTail,INT8U *pHandle)
{
	COPPER_IRON_LOSS copperIronLoss;  
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 27, pn, (INT8U *)&copperIronLoss, sizeof(COPPER_IRON_LOSS)) == TRUE)
			{
				//A相电阻
				msFrame[frameTail++] = copperIronLoss.aResistance[0];
				msFrame[frameTail++] = copperIronLoss.aResistance[1];
		
				//A相电抗
				msFrame[frameTail++] = copperIronLoss.aReactance[0];
				msFrame[frameTail++] = copperIronLoss.aReactance[1];
		
				//A相电导
				msFrame[frameTail++] = copperIronLoss.aConductance[0];
				msFrame[frameTail++] = copperIronLoss.aConductance[1];
		
				//A相电钠
				msFrame[frameTail++] = copperIronLoss.aSusceptance[0];
				msFrame[frameTail++] = copperIronLoss.aSusceptance[1];
		
				//B相电阻
				msFrame[frameTail++] = copperIronLoss.bResistance[0];
				msFrame[frameTail++] = copperIronLoss.bResistance[1];
		
				//B相电抗
				msFrame[frameTail++] = copperIronLoss.bReactance[0];
				msFrame[frameTail++] = copperIronLoss.bReactance[1];
		
				//B相电导
				msFrame[frameTail++] = copperIronLoss.bConductance[0];
				msFrame[frameTail++] = copperIronLoss.bConductance[1];
		
				//B相电钠
				msFrame[frameTail++] = copperIronLoss.bSusceptance[0];
				msFrame[frameTail++] = copperIronLoss.bSusceptance[1];
		
				//C相电阻
				msFrame[frameTail++] = copperIronLoss.cResistance[0];
				msFrame[frameTail++] = copperIronLoss.cResistance[1];
		
				//C相电抗
				msFrame[frameTail++] = copperIronLoss.cReactance[0];
				msFrame[frameTail++] = copperIronLoss.cReactance[1];
		
				//C相电导
				msFrame[frameTail++] = copperIronLoss.cConductance[0];
				msFrame[frameTail++] = copperIronLoss.cConductance[1];
		
				//C相电钠
				msFrame[frameTail++] = copperIronLoss.cSusceptance[0];
				msFrame[frameTail++] = copperIronLoss.cSusceptance[1];
			}
			else
			{
				for(i=0;i<24;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A028
功能描述:响应主站查询"测量点功率因数分段限值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A028(INT16U frameTail,INT8U *pHandle)
{
	POWER_SEG_LIMIT powerSegLimit;
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2,dt1,dt2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
  dt1 = *pHandle++;
  dt2 = *pHandle++;
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 28, pn, (INT8U *)&powerSegLimit, sizeof(POWER_SEG_LIMIT)) == TRUE)
			{
	      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
	      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
        
        msFrame[frameTail++] = dt1;
        msFrame[frameTail++] = dt2;

				//功率因数分段限值1
				msFrame[frameTail++] = powerSegLimit.segLimit1[0];
				msFrame[frameTail++] = powerSegLimit.segLimit1[1];
		
				//功率因数分段限值2
				msFrame[frameTail++] = powerSegLimit.segLimit2[0];
				msFrame[frameTail++] = powerSegLimit.segLimit2[1];
			}
			else
			{
				;
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A029
功能描述:响应主站查询"终端当地电能表显示号"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A029(INT16U frameTail,INT8U *pHandle)
{
	INT8U teShowNum[12];
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 29, pn, (INT8U *)teShowNum, 12) == TRUE)
			{
				for(i=0;i<12;i++)
				{
					msFrame[frameTail++] = teShowNum[i];		//终端当地电能表显示号
				}
			}
			else
			{
				for(i=0;i<12;i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A030
功能描述:响应主站查询"台区集中抄表停抄投抄设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A030(INT16U frameTail,INT8U *pHandle)
{
	INT8U copyStopAdminSet;
  INT8U da1,da2;
	
	INT16U pn = 0;
	INT16U tempPn = 0;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 30, pn, (INT8U *)&copyStopAdminSet, 1) == TRUE)
			{
				msFrame[frameTail++] = copyStopAdminSet;		//台区集中抄表停抄投抄设置
			}
			else
			{
				msFrame[frameTail++] = 0xee;
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A031
功能描述:响应主站查询"载波从节点附属节点地址"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A031(INT16U frameTail,INT8U *pHandle)
{
	AUXILIARY_ADDR auxiliaryAddr;
	
  INT8U da1,da2;
	
	INT16U i, pn = 0;
	INT16U tempFrameTail, tempPn = 0;
   
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn += (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 31, pn, (INT8U *)&auxiliaryAddr, sizeof(AUXILIARY_ADDR)) == TRUE)
			{
				//载波从节点附属节点个数
				msFrame[frameTail++] = auxiliaryAddr.numOfAuxiliaryNode;
		
				//附属节点地址
				for(i=0;i<auxiliaryAddr.numOfAuxiliaryNode*6;i++)
				{
					msFrame[frameTail++] = auxiliaryAddr.auxiliaryNode[i];
				}
			}
			else
			{
				return tempFrameTail;
			}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A033
功能描述:响应主站查询"终端抄表运行参数设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A033(INT16U frameTail,INT8U *pHandle)
{
	INT8U count, num;
	
	INT16U i, j;
	 
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	count = *pHandle++;		              //参数块数
	msFrame[frameTail++] = count;
	offset0a += 1 + count;
	 
	for(i = 0; i < count; i++)
	{
   #ifdef SUPPORT_ETH_COPY
		if((*pHandle >0 && *pHandle < 6) || *pHandle == 31)
   #else
		if((*pHandle >0 && *pHandle < 5) || *pHandle == 31)
	 #endif
		{
			switch(*pHandle)
			{
				case 1:
					num = 0;
					break;
					
				case 2:
					num = 1;
					break;
					
				case 3:
					num = 2;
					break;

				case 4:    //2012-3-27,add
					num = 3;
					break;

				case 31:
					num = 4;
					break;
					
       #ifdef SUPPORT_ETH_COPY
				case 5:    //2014-4-17,add
					num = 5;
					break;
			 #endif
			}
			
			msFrame[frameTail++] = teCopyRunPara.para[num].commucationPort;			//终端通信端口号
			msFrame[frameTail++] = teCopyRunPara.para[num].copyRunControl[0];		//台区集中抄表运行控制字
			msFrame[frameTail++] = teCopyRunPara.para[num].copyRunControl[1];
				
			//抄表日-日期
			for(j=0;j<4;j++)
			{
				msFrame[frameTail++] = teCopyRunPara.para[num].copyDay[j];
			}
				
			//抄表日时间
			msFrame[frameTail++] = teCopyRunPara.para[num].copyTime[0];
			msFrame[frameTail++] = teCopyRunPara.para[num].copyTime[1];
				
			//抄表日间隔时间
			msFrame[frameTail++] = teCopyRunPara.para[num].copyInterval;
				
			//对电表广播校时定时时间
			for(j=0;j<3;j++)
			{
				msFrame[frameTail++] = teCopyRunPara.para[num].broadcastCheckTime[j];
			}
				
			//允许抄表时段数
			msFrame[frameTail++] = teCopyRunPara.para[num].hourPeriodNum;
			
			//抄表时段开始时间结束时间
			for(j=0;j<teCopyRunPara.para[num].hourPeriodNum*2;j++)
			{
				msFrame[frameTail++] = teCopyRunPara.para[num].hourPeriod[j][0];
				msFrame[frameTail++] = teCopyRunPara.para[num].hourPeriod[j][1];
			}
		}
		pHandle++;
	}
	
  return frameTail;
}

/*******************************************************
函数名称:AFN0A034
功能描述:响应主站查询"集中器下行通信模块的参数设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A034(INT16U frameTail,INT8U *pHandle)
{
	INT16U count;
	INT8U  num;
	
	INT16U i, j;
	INT16U ly;
	 
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	 
	count = frameTail++;
	msFrame[count] = 0;
	num = *pHandle++;
	offset0a += (1+num);
	 
	for(i=0;i<4;i++)
	{
		//if(*pHandle <= downRiverModulePara.numOfPara)
		if(*pHandle==downRiverModulePara.para[i].commucationPort)		
		{
			msFrame[frameTail++] = downRiverModulePara.para[i].commucationPort;			//终端通信端口号
			msFrame[frameTail++] = downRiverModulePara.para[i].commCtrlWord;				//终端接口端的通信控制字
			
			//终端接口对应端的通信速率
			for(j=0;j<4;j++)
			{
				msFrame[frameTail++] = downRiverModulePara.para[i].commRate[j];
			}
			
			pHandle++;
			msFrame[count]++;
		}
		else
		{
			//如果要查询的序号大于已存在的数量，丢弃
	  	pHandle++;
		}
	}
	
	if(msFrame[count] == 0)
	{
		frameTail -= 5;
	}
	
  return frameTail;
}

/*******************************************************
函数名称:AFN0A035
功能描述:响应主站查询"台区集中抄表重点户设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A035(INT16U frameTail,INT8U *pHandle)
{
	INT16U i;
	 
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	msFrame[frameTail++] = keyHouseHold.numOfHousehold;		//台区集中抄表重点户个数
	
	if(keyHouseHold.numOfHousehold <= 0)
	{
		return frameTail -= 5;
	}
	
	//重点户的电能表/交流采样装置序号
	for(i=0;i<keyHouseHold.numOfHousehold*2;i++)
	{
		msFrame[frameTail++] = keyHouseHold.household[i];
	}

  return frameTail;
}

/*******************************************************
函数名称:AFN0A036
功能描述:响应主站查询"终端上行通信流量门限设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A036(INT16U frameTail,INT8U *pHandle)
{
	INT16U i;
	
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	//月通信流量门限
	for(i=0;i<4;i++)
	{
		msFrame[frameTail++] = upTranslateLimit[i];
	}

  return frameTail;
}

/*******************************************************
函数名称:AFN0A037
功能描述:响应主站查询"终端级联通信参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A037(INT16U frameTail,INT8U *pHandle)
{
	INT16U i;
	
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	msFrame[frameTail++] = cascadeCommPara.commPort;					//终端级联通信端口号
	msFrame[frameTail++] = cascadeCommPara.ctrlWord;					//终端级联通信端口号
	msFrame[frameTail++] = cascadeCommPara.receiveMsgTimeout;	//接收等待报文超时时间
	msFrame[frameTail++] = cascadeCommPara.receiveByteTimeout;//接收等待字节超时时间
	msFrame[frameTail++] = cascadeCommPara.cascadeMretryTime;	//级联方接收失败重发次数
	msFrame[frameTail++] = cascadeCommPara.groundSurveyPeriod;//级联巡测周期
	msFrame[frameTail++] = cascadeCommPara.flagAndTeNumber;		//终端标准及终端个数
	
	for(i=0;i<(cascadeCommPara.flagAndTeNumber & 0x0F)*2;i+=2)
	{
		//被级联的终端行政区划码
		msFrame[frameTail++] = cascadeCommPara.divisionCode[i];
		msFrame[frameTail++] = cascadeCommPara.divisionCode[i + 1];
		
		//被级联的终端地址
		msFrame[frameTail++] = cascadeCommPara.cascadeTeAddr[i];
		msFrame[frameTail++] = cascadeCommPara.cascadeTeAddr[i + 1];
	}
	
  return frameTail;
}

/*******************************************************
函数名称:AFN0A038
功能描述:响应主站查询"1类数据配置设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A038(INT16U frameTail,INT8U *pHandle)
{
	INT16U i, j;
	INT16U tempTail;  
	INT8U  num;
	INT8U  bType, lType;
	
	tempTail = frameTail;
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	//用户大类号
	bType = *pHandle++;
	offset0a++;

	printf("AFN0A-FN38大类号%d\n", bType);

	if (bType<16)
	{
		//用户大类号
		msFrame[frameTail++] = bType;
		
		//查询数量
		msFrame[frameTail++] = num = *pHandle++;
		offset0a++;
		
	  printf("AFN0A-FN38查询数量%d\n", num);

		for(i=0;i<num && i<16;i++)
		{
			lType = *pHandle++;
			offset0a++;

			//用户小类号
			msFrame[frameTail++] = lType;
	    printf("AFN0A-FN38用户小类号%d\n", lType);

			//信息类组数个数
			msFrame[frameTail++] = typeDataConfig1.bigType[bType].littleType[lType].infoGroup; 
	    
	    printf("AFN0A-FN38信息类组数个数%d\n", typeDataConfig1.bigType[bType].littleType[lType].infoGroup);

			//信息类组所对应的信息类元标志位
			for(j=0;j<typeDataConfig1.bigType[bType].littleType[lType].infoGroup;j++)
			{
				msFrame[frameTail++] = typeDataConfig1.bigType[bType].littleType[lType].flag[j];
			}
		}
	}
	else
	{
		return tempTail;
	}

  return frameTail;
}

/*******************************************************
函数名称:AFN0A039
功能描述:响应主站查询"2类数据配置设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A039(INT16U frameTail,INT8U *pHandle)
{
	INT16U i, j;
	INT16U tempTail;  
	INT8U  num;
	INT8U  bType, lType;
	
	tempTail = frameTail;
	msFrame[frameTail++] = *pHandle++;	//DA1
	msFrame[frameTail++] = *pHandle++;	//DA2
	msFrame[frameTail++] = *pHandle++;	//DT1
	msFrame[frameTail++] = *pHandle++;	//DT2
	
	//用户大类号
	bType = *pHandle++;
	offset0a++;
	
	printf("AFN0A-FN39大类号%d\n", bType);
	
	if (bType<16)
	{
		//用户大类号
		msFrame[frameTail++] = bType;
		
		//查询数量
		msFrame[frameTail++] = num = *pHandle++;
	  printf("AFN0A-FN39查询数量%d\n", num);

		offset0a++;
		
		for(i=0;i<num && i<16;i++)
		{
			lType = *pHandle++;
			offset0a++;
	    
	    printf("AFN0A-FN39用户小类号%d\n", lType);

			//用户小类号
			msFrame[frameTail++] = lType;

			//信息类组数个数
			msFrame[frameTail++] = typeDataConfig2.bigType[bType].littleType[lType].infoGroup; 
	    
	    printf("AFN0A-FN39信息类组数个数%d\n", typeDataConfig2.bigType[bType].littleType[lType].infoGroup);

			//信息类组所对应的信息类元标志位
			for(j=0;j<typeDataConfig2.bigType[bType].littleType[lType].infoGroup;j++)
			{
				msFrame[frameTail++] = typeDataConfig2.bigType[bType].littleType[lType].flag[j];
			}
		}
	}
	else
	{
		return tempTail;
	}

  return frameTail;
}

/*******************************************************
函数名称:AFN0A041
功能描述:响应主站查询"时段功控定值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A041(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U i, j;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
 		  if(pn > 8)
 		  {
  			return tempFrameTail;
  	  }
  
      msFrame[frameTail++] = periodCtrlConfig[pn - 1].periodNum;	//方案标志
  
      for(i=0;i<3;i++)
      {
  	    //时段号
  	    msFrame[frameTail++] = periodCtrlConfig[pn - 1].period[i].timeCode;
  	
  	    //时段功控定值
  	    for(j=0;j<8;j++)
  	    {
  		    msFrame[frameTail++] = periodCtrlConfig[pn - 1].period[i].limitTime[j][0];
  		    msFrame[frameTail++] = periodCtrlConfig[pn - 1].period[i].limitTime[j][1];
  	    }
      }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}	

/*******************************************************
函数名称:AFN0A042
功能描述:响应主站查询"厂休功控参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A042(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		//厂休控定值
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdLimit & 0xFF;
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdLimit>>8;
  
  		//限电起始时间
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdStartMin;
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdStartHour;
  
  		//限电延续时间
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdTime;
  
  		//每周限电日
  		msFrame[frameTail++] = wkdCtrlConfig[pn - 1].wkdDate;
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A043
功能描述:响应主站查询"功率控制的功率计算滑差时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A043(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{ 
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		msFrame[frameTail++] = powerCtrlCountTime[pn - 1].countTime;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A044
功能描述:响应主站查询"营业报停控参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A044(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
	    
  		//报停起始时间
	    msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsStartDay%10)|(obsCtrlConfig[pn-1].obsStartDay/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsStartMonth%10)|(obsCtrlConfig[pn-1].obsStartMonth/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsStartYear%10)|(obsCtrlConfig[pn-1].obsStartYear/10<<4);
    	  
  		//报停结束时间
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsEndDay%10)|(obsCtrlConfig[pn-1].obsEndDay/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsEndMonth%10)|(obsCtrlConfig[pn-1].obsEndMonth/10<<4);
    	msFrame[frameTail++] = (obsCtrlConfig[pn-1].obsEndYear%10)|(obsCtrlConfig[pn-1].obsEndYear/10<<4);
    	  
 		  //报停控功率定值
 		  msFrame[frameTail++] = obsCtrlConfig[pn-1].obsLimit&0xFF;
    	msFrame[frameTail++] = obsCtrlConfig[pn-1].obsLimit>>8&0xFF;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A045
功能描述:响应主站查询"功控轮次设定"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A045(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		//功控轮次标志位
  		msFrame[frameTail++] = powerCtrlRoundFlag[pn - 1].flag;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A046
功能描述:响应主站查询"月电量控定值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A046(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		//月电量控定值
  		msFrame[frameTail++] = monthCtrlConfig[pn - 1].monthCtrl & 0xFF;
  		msFrame[frameTail++] = (monthCtrlConfig[pn - 1].monthCtrl >> 8) & 0xFF;
  		msFrame[frameTail++] = (monthCtrlConfig[pn - 1].monthCtrl >> 16) & 0xFF;
  		msFrame[frameTail++] = (monthCtrlConfig[pn - 1].monthCtrl >> 24) & 0xFF;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A047
功能描述:响应主站查询"购电量控参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A047(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
	
			//购电单号
  		msFrame[frameTail++] = chargeCtrlConfig[pn - 1].numOfBill & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].numOfBill >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].numOfBill >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].numOfBill >> 24) & 0xFF;
  
  		//追加刷新标志
  		msFrame[frameTail++] = chargeCtrlConfig[pn - 1].flag;
  
  		//购电量值
			msFrame[frameTail++] = chargeCtrlConfig[pn - 1].chargeCtrl & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].chargeCtrl >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].chargeCtrl >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].chargeCtrl >> 24) & 0xFF;

			//报警门限值
			msFrame[frameTail++] = chargeCtrlConfig[pn - 1].alarmLimit & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].alarmLimit >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].alarmLimit >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].alarmLimit >> 24) & 0xFF;
	
			//跳闸门限值
			msFrame[frameTail++] = chargeCtrlConfig[pn - 1].cutDownLimit & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].cutDownLimit >> 8) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].cutDownLimit >> 16) & 0xFF;
  		msFrame[frameTail++] = (chargeCtrlConfig[pn - 1].cutDownLimit >> 24) & 0xFF;
  	}
  	da1 >>= 1;
  }	
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A048
功能描述:响应主站查询"电控轮次设定"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A048(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn=0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return tempFrameTail;
  		}
  
  		msFrame[frameTail++] = electCtrlRoundFlag[pn - 1].flag;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A049
功能描述:响应主站查询"功控告警时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A049(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//判断pn的值是否合法
  		if(pn > 8)
  		{
  			return frameTail -= 4;
  		}
  
  		msFrame[frameTail++] = powerCtrlAlarmTime[pn - 1].alarmTime;
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

#ifdef LIGHTING

/*******************************************************
函数名称:AFN0A050
功能描述:响应主站查询"控制时段"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A050(INT16U frameTail,INT8U *pHandle)
{
	struct ctrlTimes *tmpCTimes;
	INT8U            i;
	INT16U           tmpTail;
	INT8U            tmpCount;
	INT8U            alreadySend;
  INT8U            tmpi;
	
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;	  //DA2
	msFrame[frameTail++] = *pHandle++;	  //DT1
	msFrame[frameTail++] = *pHandle++;	  //DT2
	tmpCount = *pHandle++;	              //发送的时段数量最大值,2016-12-16
	alreadySend = (tmpCount/20-1)*20;
	
	offset0a += 1;
	
	printf("tmpCount=%d,alreadySend=%d\n", tmpCount, alreadySend);
	
	tmpTail = frameTail++;    //控制时段个数  
	msFrame[tmpTail] = 0;
	
	tmpi = 0;
	tmpCTimes = cTimesHead;

	while(tmpCTimes!=NULL)
  {
	  if (tmpi>=alreadySend)
		{
			msFrame[tmpTail]++;
			
			msFrame[frameTail++] = tmpCTimes->startMonth;
			msFrame[frameTail++] = tmpCTimes->startDay;
			msFrame[frameTail++] = tmpCTimes->endMonth;
			msFrame[frameTail++] = tmpCTimes->endDay;
			msFrame[frameTail++] = tmpCTimes->deviceType;
			msFrame[frameTail++] = tmpCTimes->noOfTime;
			msFrame[frameTail++] = tmpCTimes->workDay;

			for(i=0; i<6; i++)
			{
				msFrame[frameTail++] = tmpCTimes->hour[i];
				msFrame[frameTail++] = tmpCTimes->min[i];
				msFrame[frameTail++] = tmpCTimes->bright[i];
			}
			
			//每次发送20个控制时段参数
			if (msFrame[tmpTail]>=20)
			{
				break;
			}
		}
		tmpi++;
	  
	  tmpCTimes = tmpCTimes->next;
  }
  
  //2015-06-09,添加,控制模式
  msFrame[frameTail++] = ctrlMode;
  
  //2015-06-25,添加,光控提前-延迟多少分钟有效
  memcpy(&msFrame[frameTail], beforeOnOff, 4);
  frameTail += 4;
	
  return frameTail;
}

/*******************************************************
函数名称:AFN0A051
功能描述:响应主站查询"控制点阈值设定"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A051(INT16U frameTail,INT8U *pHandle)
{
	//数据单元标识
	msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2	
	msFrame[frameTail++] = *pHandle++;    //DT1
	msFrame[frameTail++] = *pHandle;      //DT2	
	 
	//数据单元	 
	msFrame[frameTail++] = pnGate.failureRetry;
	msFrame[frameTail++] = pnGate.boardcastWaitGate;
	msFrame[frameTail++] = pnGate.checkTimeGate;
	msFrame[frameTail++] = pnGate.lddOffGate;
	msFrame[frameTail++] = pnGate.lddtRetry;
	msFrame[frameTail++] = pnGate.offLineRetry;
	msFrame[frameTail++] = pnGate.lcWave;
	msFrame[frameTail++] = pnGate.leakCurrent;

  return frameTail;
}

/*******************************************************
函数名称:AFN0A052
功能描述:响应主站查询"控制点限值参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A052(INT16U frameTail,INT8U *pHandle)
{
	PN_LIMIT_PARA pointLimitPara; 
	
	INT16U i, pn = 0;
	INT16U tempPn = 0;
  INT8U da1,da2;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据	
			if(selectViceParameter(0x04, 52, pn, (INT8U *)&pointLimitPara, sizeof(PN_LIMIT_PARA)) == TRUE)
			{
				//电压过压门限
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit & 0xFF;
				msFrame[frameTail++] = pointLimitPara.vSuperiodLimit >> 8;
			  
				//电压欠压门限
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit & 0xFF;
				msFrame[frameTail++] = pointLimitPara.vDownDownLimit >> 8;
				
				//电流过流门限
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[0];
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[1];
				msFrame[frameTail++] = pointLimitPara.cSuperiodLimit[2];

				//电流欠流门限
				msFrame[frameTail++] = pointLimitPara.cDownDownLimit[0];
				msFrame[frameTail++] = pointLimitPara.cDownDownLimit[1];
				msFrame[frameTail++] = pointLimitPara.cDownDownLimit[2];
				
				//功率上限
				msFrame[frameTail++] = pointLimitPara.pUpLimit[0];
				msFrame[frameTail++] = pointLimitPara.pUpLimit[1];
				msFrame[frameTail++] = pointLimitPara.pUpLimit[2];

				//功率下限
				msFrame[frameTail++] = pointLimitPara.pDownLimit[0];
				msFrame[frameTail++] = pointLimitPara.pDownLimit[1];
				msFrame[frameTail++] = pointLimitPara.pDownLimit[2];

				//功率因数下限
				msFrame[frameTail++] = pointLimitPara.factorDownLimit[0];
				msFrame[frameTail++] = pointLimitPara.factorDownLimit[1];

				//越限持续时间
				msFrame[frameTail++] = pointLimitPara.overContinued;
			}
			else
			{
				for(i=0; i<19; i++)
				{
					msFrame[frameTail++] = 0xee;
				}
			} 
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

#endif

/*******************************************************
函数名称:AFN0A057
功能描述:响应主站查询"终端声音告警允许/禁止设置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A057(INT16U frameTail,INT8U *pHandle)
{
	  //数据单元标识
	  msFrame[frameTail++] = *pHandle++;    //DA1
	  msFrame[frameTail++] = *pHandle++;    //DA2
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2	
	 
	  //数据单元
	  msFrame[frameTail++] = voiceAlarm[0];
	  msFrame[frameTail++] = voiceAlarm[1];
	  msFrame[frameTail++] = voiceAlarm[2];

    return frameTail;
}

/*******************************************************
函数名称:AFN0A058
功能描述:响应主站查询"终端自动保电参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A058(INT16U frameTail,INT8U *pHandle)
{
	  //数据单元标识
	  msFrame[frameTail++] = *pHandle++;    //DA1
	  msFrame[frameTail++] = *pHandle++;    //DA2
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2	
	 
	  //数据单元	 
	  msFrame[frameTail++] = noCommunicationTime;
	  
	  return frameTail;
}

/*******************************************************
函数名称:AFN0A059
功能描述:响应主站查询"电能表异常判别阈值设定"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A059(INT16U frameTail,INT8U *pHandle)
{
	  //数据单元标识
	  msFrame[frameTail++] = *pHandle++;    //DA1
	  msFrame[frameTail++] = *pHandle++;    //DA2	
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2	
	 
	  //数据单元	 
	  msFrame[frameTail++] = meterGate.powerOverGate;
	  msFrame[frameTail++] = meterGate.meterFlyGate;
	  msFrame[frameTail++] = meterGate.meterStopGate;
	  msFrame[frameTail++] = meterGate.meterCheckTimeGate;

    return frameTail;
}

/*******************************************************
函数名称:AFN0A060
功能描述:响应主站查询参数命令"谐波限值(F60)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A060(INT16U frameTail,INT8U *pHandle)
{
  INT8U i;
  
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  
  //总畸变电压含有率上限
  msFrame[frameTail++] = waveLimit.totalUPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.totalUPercentUpLimit[1];
  
  //奇次谐波电压含有率上限
  msFrame[frameTail++] = waveLimit.oddUPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.oddUPercentUpLimit[1];
  
  //偶次谐波电压含有率上限
  msFrame[frameTail++] = waveLimit.evenUPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.evenUPercentUpLimit[1];
  
  //谐波电压含有率上限
  for(i=0;i<18;i++)
  {
  	msFrame[frameTail++] = waveLimit.UPercentUpLimit[i][0];
  	msFrame[frameTail++] = waveLimit.UPercentUpLimit[i][1];
  }
  
  //总畸变电流有效值上限
  msFrame[frameTail++] = waveLimit.totalIPercentUpLimit[0];
  msFrame[frameTail++] = waveLimit.totalIPercentUpLimit[1];
  
  //谐波电流有效值上限
  for(i=0;i<18;i++)
  {
  	msFrame[frameTail++] = waveLimit.IPercentUpLimit[i][0];
  	msFrame[frameTail++] = waveLimit.IPercentUpLimit[i][1];
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A061
功能描述:响应主站查询参数命令"直流模拟量接入参数(F61)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A061(INT16U frameTail,INT8U *pHandle)
{
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  msFrame[frameTail++] = *pHandle++;
  
  msFrame[frameTail++] = adcInFlag;
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A065
功能描述:响应主站查询参数命令"定时发送1类数据任务设置(F65)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A065(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//任务号
	INT16U i;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendPeriod;		//定时上报周期及上报周期单位
  	
  		//上报基准时间
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.second / 10 << 4 | reportTask1.task[pn - 1].sendStart.second % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.minute / 10 << 4 | reportTask1.task[pn - 1].sendStart.minute % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.hour / 10 << 4 | reportTask1.task[pn - 1].sendStart.hour % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.day / 10 << 4 | reportTask1.task[pn - 1].sendStart.day % 10;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.month / 10 << 4 | reportTask1.task[pn - 1].sendStart.month % 10 | reportTask1.task[pn - 1].week;
  		msFrame[frameTail++] = reportTask1.task[pn - 1].sendStart.year / 10 << 4 | reportTask1.task[pn - 1].sendStart.year % 10;
  	  
  	  msFrame[frameTail++] = reportTask1.task[pn - 1].sampleTimes;	//曲线数据抽取倍率
  		msFrame[frameTail++] = reportTask1.task[pn - 1].numOfDataId;	//数据单元抽取个数
  	
  		for(i=0;i<reportTask1.task[pn - 1].numOfDataId;i++)
  		{
  			//DA
  			msFrame[frameTail++] = reportTask1.task[pn - 1].dataUnit[i].pn1;
  			msFrame[frameTail++] = reportTask1.task[pn - 1].dataUnit[i].pn2;
  			
  			//DT
  			msFrame[frameTail++] = 0x01<<((reportTask1.task[pn - 1].dataUnit[i].fn%8==0)?7:((reportTask1.task[pn - 1].dataUnit[i].fn-1)%8));
  			msFrame[frameTail++] = (reportTask1.task[pn - 1].dataUnit[i].fn - 1)/8;  		
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A066
功能描述:响应主站查询参数命令"定时发送2类数据任务设置(F66)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A066(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//任务号
	INT16U i;
	INT16U tempFrameTail, tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  tempFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendPeriod;		//定时上报周期及上报周期单位
  	
  		//上报基准时间
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.second / 10 << 4 | reportTask2.task[pn - 1].sendStart.second % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.minute / 10 << 4 | reportTask2.task[pn - 1].sendStart.minute % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.hour / 10 << 4 | reportTask2.task[pn - 1].sendStart.hour % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.day / 10 << 4 | reportTask2.task[pn - 1].sendStart.day % 10;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.month / 10 << 4 | reportTask2.task[pn - 1].sendStart.month % 10 | reportTask2.task[pn - 1].week;
  		msFrame[frameTail++] = reportTask2.task[pn - 1].sendStart.year / 10 << 4 | reportTask2.task[pn - 1].sendStart.year % 10;
  	  
  	  msFrame[frameTail++] = reportTask2.task[pn - 1].sampleTimes;	//曲线数据抽取倍率
  		msFrame[frameTail++] = reportTask2.task[pn - 1].numOfDataId;	//数据单元抽取个数
  	
  		for(i=0;i<reportTask2.task[pn - 1].numOfDataId;i++)
  		{
  			//DA
  			msFrame[frameTail++] = reportTask2.task[pn - 1].dataUnit[i].pn1;
  			msFrame[frameTail++] = reportTask2.task[pn - 1].dataUnit[i].pn2;
  			
  			//DT
  			msFrame[frameTail++] = 0x01<<((reportTask2.task[pn - 1].dataUnit[i].fn%8==0)?7:((reportTask2.task[pn - 1].dataUnit[i].fn-1)%8));
  			msFrame[frameTail++] = (reportTask2.task[pn - 1].dataUnit[i].fn - 1)/8;  		
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A067
功能描述:响应主站查询参数命令"定时发送1类数据任务启动/停止设置(F67)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A067(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//任务号
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		msFrame[frameTail++] = reportTask1.task[pn - 1].stopFlag;		//任务开启停止标志
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A068
功能描述:响应主站查询参数命令"定时发送2类数据任务启动/停止设置(F68)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A068(INT16U frameTail,INT8U *pHandle)
{
	INT16U pn = 0;			//任务号
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		msFrame[frameTail++] = reportTask2.task[pn - 1].stopFlag;		//任务开启停止标志
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A073
功能描述:响应主站查询参数命令"电容器配置(F73)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A073(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_PARA capacityPara;
	
	INT16U pn = 0;			//测量点号
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
  		if(result == TRUE)
  		{
  			for(i=0;i<16;i++)
  			{
  				//补偿方式
  				msFrame[frameTail++] = capacityPara.capacity[i].compensationMode;
  		
  				//电容装见容量
  				msFrame[frameTail++] = capacityPara.capacity[i].capacityNum[0];
  				msFrame[frameTail++] = capacityPara.capacity[i].capacityNum[1];
  			}
  		}
  		else
  		{
  			//没有数据
  			for(i=0;i<48;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A074
功能描述:响应主站查询参数命令"电容器投切运行状态(F74)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A074(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_RUN_PARA capacityRunPara;
	
	INT16U pn = 0;			//测量点号
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 74, pn, (INT8U *)&capacityRunPara, sizeof(CAPACITY_RUN_PARA));
  		if(result == TRUE)
  		{
  			//目标功率因数
  			msFrame[frameTail++] = capacityRunPara.targetPowerFactor[0];
  			msFrame[frameTail++] = capacityRunPara.targetPowerFactor[1];
  	
  			//投入无功功率门限
  			msFrame[frameTail++] = capacityRunPara.onPowerLimit[0];
  			msFrame[frameTail++] = capacityRunPara.onPowerLimit[1];
  			msFrame[frameTail++] = capacityRunPara.onPowerLimit[2];
  	
  			//切除无功功率门限
  			msFrame[frameTail++] = capacityRunPara.offPowerLimit[0];
  			msFrame[frameTail++] = capacityRunPara.offPowerLimit[1];
  			msFrame[frameTail++] = capacityRunPara.offPowerLimit[2];
  	
  			//延时时间
  			msFrame[frameTail++] = capacityRunPara.delay;
  	
  			//动作时间间隔
  			msFrame[frameTail++] = capacityRunPara.actInterval;
  		}
  		else
  		{
  			//没有数据
  			for(i=0;i<10;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A075
功能描述:响应主站查询参数命令"电容器保护参数(F75)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A075(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_PROTECT_PARA capacityProtectPara;
	
	INT16U pn = 0;			//测量点号
	INT16U i;
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 75, pn, (INT8U *)&capacityProtectPara, sizeof(CAPACITY_PROTECT_PARA));
  		if(result == TRUE)
 		  {
  			//过电压
  			msFrame[frameTail++] = capacityProtectPara.vSuperiod[0];
  			msFrame[frameTail++] = capacityProtectPara.vSuperiod[1];
  	
  			//过电压回差值
  			msFrame[frameTail++] = capacityProtectPara.vSuperiodQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.vSuperiodQuan[1];
  	
  			//欠电压
  			msFrame[frameTail++] = capacityProtectPara.vLack[0];
  			msFrame[frameTail++] = capacityProtectPara.vLack[1];
  	
  			//欠电压回差值
  			msFrame[frameTail++] = capacityProtectPara.vLackQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.vLackQuan[1];
  	
  			//总畸变电流含有率上限
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalUpLimit[0];
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalUpLimit[1];
  	
  			//总畸变电流含有率越限回差值
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.cAbnormalQuan[1];
  	
  			//总畸变电压含有率上限
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalUpLimit[0];
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalUpLimit[1];
  	
  			//总畸变电压含有率越限回差值
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalQuan[0];
  			msFrame[frameTail++] = capacityProtectPara.vAbnormalQuan[1];
  		}
  		else
  		{
  			//没有数据
  			for(i=0;i<16;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}	
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A076
功能描述:响应主站查询参数命令"电容器投切控制参数(F76)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A076(INT16U frameTail,INT8U *pHandle)
{
	CAPACITY_PARA capacityPara;
	
	INT16U pn = 0;			//测量点号
	INT16U tempPn = 0;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 73, pn, (INT8U *)&capacityPara, sizeof(CAPACITY_PARA));
  		if(result == TRUE)
  		{
  			msFrame[frameTail++] = capacityPara.controlMode;
  		}
  		else
  		{
  			//没有数据
  			msFrame[frameTail++] = 0xEE;
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A081
功能描述:响应主站查询参数命令"直流模拟量变比(F81)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A081(INT16U frameTail,INT8U *pHandle)
{
	ADC_PARA adcPara;
	
	INT16U pn = 0;			//测量点号
	INT16U tempPn = 0;
	INT16U i;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 81, pn, (INT8U *)&adcPara, sizeof(ADC_PARA));
  		if(result == TRUE)
  		{
  			//直流模拟量量程起始值
  			msFrame[frameTail++] = adcPara.adcStartValue[0];
  			msFrame[frameTail++] = adcPara.adcStartValue[1];
  	
  			//直流模拟量量程终止值
  			msFrame[frameTail++] = adcPara.adcEndValue[0];
  			msFrame[frameTail++] = adcPara.adcEndValue[1];
  		}
  		else
  		{
  			//没有数据
  			for(i=0;i<4;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
 		  }
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

/*******************************************************
函数名称:AFN0A082
功能描述:响应主站查询参数命令"直流模拟量限值(F82)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A082(INT16U frameTail,INT8U *pHandle)
{
	ADC_PARA adcPara;
	
	INT16U pn = 0;			//测量点号
	INT16U tempPn = 0;
	INT16U i;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 81, pn, (INT8U *)&adcPara, sizeof(ADC_PARA));
  		if(result == TRUE)
  		{
  			//直流模拟量上限
  			msFrame[frameTail++] = adcPara.adcUpLimit[0];
  			msFrame[frameTail++] = adcPara.adcUpLimit[1];
  	
  			//直流模拟量下限
  			msFrame[frameTail++] = adcPara.adcLowLimit[0];
  			msFrame[frameTail++] = adcPara.adcLowLimit[1];
  		}
  		else
  		{
  			//没有数据
  			for(i=0;i<4;i++)
  			{
  				msFrame[frameTail++] = 0xEE;
  			}
  		}
  	}
  	da1 >>= 1;
  }
  
 	return frameTail;
}

/*******************************************************
函数名称:AFN0A083
功能描述:响应主站查询参数命令"直流模拟量冻结参数(F81)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
INT16U AFN0A083(INT16U frameTail,INT8U *pHandle)
{
	ADC_PARA adcPara;
	
	INT16U pn = 0;			//测量点号
	INT16U tempPn = 0;
	INT16U i;
	
  INT8U da1,da2;
  INT8U result;
   
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
   
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
  		//查询数据
  		result = selectViceParameter(0x04, 81, pn, (INT8U *)&adcPara, sizeof(ADC_PARA));
  		if(result == TRUE)
  		{
  			//直流模拟量上限
  			msFrame[frameTail++] = adcPara.adcFreezeDensity;
  		}
  		else
  		{
  			//没有数据
  			msFrame[frameTail++] = 0xEE;
  		}
  	}
  	da1 >>= 1;
  }
  
	return frameTail;
}

#ifdef SDDL_CSM

/*******************************************************
函数名称:AFN0A088
功能描述:响应主站查询"测量点电能表资产编号"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A088(INT16U frameTail,INT8U *pHandle)
{
	INT16U           i, pn = 0;
	INT16U           tempPn = 0;
	
  INT8U da1,da2;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
   
  msFrame[frameTail++] = da1;
  msFrame[frameTail++] = da2;
  msFrame[frameTail++] = *pHandle++;	//DT1
  msFrame[frameTail++] = *pHandle++;	//DT2
  
  while(tempPn < 9)
  {
  	tempPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = (da2 - 1) * 8 + tempPn;
  		
			if(selectViceParameter(0x04, 88, pn, &msFrame[frameTail], 15) == FALSE)
			{
				memset(&msFrame[frameTail], 0x0,15);
			}

			frameTail+=15;
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

#endif

/*******************************************************
函数名称:AFN0A097
功能描述:响应主站查询参数命令"设置终端名称(F97)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A097(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i;
	
  //数据单元标识
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x01;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    
  //数据单元
  for(i = 0; i < 20; i++)
  {
  	msFrame[frameTail++] = teName[i];
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A098
功能描述:响应主站查询参数命令"设置系统运行标识码(F98)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A098(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i;
	
  //数据单元标识
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x02;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    
  //数据单元
  for(i = 0; i < 20; i++)
  {
  	msFrame[frameTail++] = sysRunId[i];
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A099
功能描述:响应主站查询参数命令"终端抄表搜索持续时间(F99)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A099(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i;
	
  //数据单元标识
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x04;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    

  //数据单元
  for(i = 0; i < 6; i++)
  {
  	 msFrame[frameTail++] = assignCopyTime[i];
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A100
功能描述:响应主站查询参数命令"终端预设APN(F100)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A100(INT16U frameTail,INT8U *pHandle)
{ 
	INT8U i,j;
	
  //数据单元标识
  msFrame[frameTail++] = 0;  			//DA1
  msFrame[frameTail++] = 0;  			//DA2
  msFrame[frameTail++] = 0x08;  	//DT1
  msFrame[frameTail++] = 0x0C;    //DT2
    
  //数据单元
  for(i = 0; i < 4; i++)
  {
  	for(j=0;j<16;j++)
    {
  	  msFrame[frameTail++] = teApn[i][j];
  	}
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0A121
功能描述:响应主站查询"终端地址及行政区划码"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A121(INT16U frameTail,INT8U *pHandle)
{
	  #ifdef TE_ADDR_USE_BCD_CODE
	   INT16U tmpAddr;
	  #endif
	  
	  //数据单元标识
	  msFrame[frameTail++] = *pHandle++;    //DA1
  	msFrame[frameTail++] = *pHandle++;    //DA2
	  msFrame[frameTail++] = *pHandle++;    //DT1
	  msFrame[frameTail++] = *pHandle;      //DT2
	
	  //数据单元
	  //行政区划码
	  #ifdef TE_ADDR_USE_BCD_CODE
	   tmpAddr = (addrField.a2[1]>>4)*1000 + (addrField.a2[1]&0xf)*100
	           + (addrField.a2[0]>>4)*10 + (addrField.a2[0]&0xf);
	   msFrame[frameTail++] = tmpAddr&0xff;
	   msFrame[frameTail++] = tmpAddr>>8;
	  #else
	   msFrame[frameTail++] = addrField.a2[0];
	   msFrame[frameTail++] = addrField.a2[1];
	  #endif

	  //终端地址
	  msFrame[frameTail++] = addrField.a1[0];
	  msFrame[frameTail++] = addrField.a1[1];
	  
	  return frameTail;
}

/*******************************************************
函数名称:AFN0A133
功能描述:响应主站查询"载波/无线主节点地址"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A133(INT16U frameTail,INT8U *pHandle)
{
	 //数据单元标识
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   memcpy(&msFrame[frameTail],  mainNodeAddr, 6);
   
   frameTail += 6;
	 
	 return frameTail;
}

/*******************************************************
函数名称:AFN0A134
功能描述:响应主站查询"设备编号"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A134(INT16U frameTail,INT8U *pHandle)
{
	 //数据单元标识
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //设备编号
   msFrame[frameTail++] = deviceNumber&0xff;
   msFrame[frameTail++] = deviceNumber>>8&0xff;
   
	 return frameTail;
}

/*******************************************************
函数名称:AFN0A135
功能描述:响应主站查询"锐拔模块参数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A135(INT16U frameTail,INT8U *pHandle)
{
	 //数据单元标识
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //参数
   msFrame[frameTail++] = rlPara[0];   //基本功率
   msFrame[frameTail++] = rlPara[1];   //最大功率
   msFrame[frameTail++] = rlPara[2];   //信号强度
   msFrame[frameTail++] = rlPara[3];   //信道

	 return frameTail;
}

/*******************************************************
函数名称:AFN0A136
功能描述:响应主站查询"设置厂商信息"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A136(INT16U frameTail,INT8U *pHandle)
{
	 //数据单元标识
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //厂商信息
   memcpy(&msFrame[frameTail], csNameId, 12);
   
   frameTail += 12;

	 return frameTail;
}

/*******************************************************
函数名称:AFN0A138
功能描述:响应主站查询"居民用户表数据类型"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A138(INT16U frameTail,INT8U *pHandle)
{
	 //数据单元标识
	 msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
	 msFrame[frameTail++] = *pHandle++;    //DT1
	 msFrame[frameTail++] = *pHandle;      //DT2
   
   //类型
   msFrame[frameTail++] = denizenDataType;

	 return frameTail;
}


#ifdef SDDL_CSM

/*******************************************************
函数名称:AFN0A224
功能描述:响应主站查询"终端地址码-山东"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0A224(INT16U frameTail, INT8U *pHandle)
{
  INT8U supplyWay;
  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
	msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = *pHandle++;    //DT1
  msFrame[frameTail++] = *pHandle;      //DT2

  //数据单元
  //行政区码
  msFrame[frameTail++] = addrField.a1[0];
  msFrame[frameTail++] = addrField.a1[1];

  //终端地址
  msFrame[frameTail++] = addrField.a2[0];
  msFrame[frameTail++] = addrField.a2[1];

  //供电方式
  selectParameter(0x04, 224, &supplyWay, 1);

  msFrame[frameTail++] = supplyWay;
  
  return frameTail;
}

#endif
