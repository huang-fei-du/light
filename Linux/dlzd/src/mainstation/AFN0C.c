/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN0C.c
作者：wan guihua
版本：0.1
完成日期：2010年1月
描述：主站AFN0C(请求一类数据)处理文件。
函数列表：
     1.
修改历史：
  01,10-1-24,Leiyong created.
  02,11-05-13,Leiyong,添加集中器同时点抄多个测量点的功能
***************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMeter.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "dataBalance.h"
#include "convert.h"
#include "meterProtocol.h"
#include "att7022b.h"
#include "userInterface.h"
#include "gdw376-2.h"

#include "AFN0C.h"

INT8U  ackTail;

#ifdef PULSE_GATHER
 extern ONE_PULSE pulse[NUM_OF_SWITCH_PULSE];      //脉冲量采集
#endif

INT8U afn0cDataFrom;

/*******************************************************
函数名称:AFN0C
功能描述:主站"请求一类数据"(AFN0C)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN0C(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll)
{
    INT16U   tmpI,tmpFrameTail;
    INT8U    fn;
    INT8U    tmpDtCount;              //DT移位计数
    INT8U    tmpDt1;                  //临时DT1
    INT8U    *pTpv;                   //TpV指针
    INT8U    maxCycle;                //最大循环次数
    INT16U   frameTail0c;             //AFN0C帧尾
    INT16U   tmpHead0c;               //AFN0C临时帧头
    INT16U   tmpHead0cActive;         //主动上报AFN0C临时帧头

    INT16U (*AFN0CFun[248])(INT16U frameTail,INT8U *pHandlem, INT8U fn);
    for (tmpI=0; tmpI<248; tmpI++)
    {
       AFN0CFun[tmpI] = NULL;
    }
       
    //组1
    AFN0CFun[0] = AFN0C001;
    
    afn0cDataFrom = dataFrom;
    
    AFN0CFun[1] = AFN0C002;
    AFN0CFun[2] = AFN0C003;
    AFN0CFun[3] = AFN0C004;
    AFN0CFun[4] = AFN0C005;
    AFN0CFun[5] = AFN0C006;
    AFN0CFun[6] = AFN0C007;
    AFN0CFun[7] = AFN0C008;
    
    //组2
    AFN0CFun[8] = AFN0C009;
    AFN0CFun[9] = AFN0C010;
   
    AFN0CFun[10] = AFN0C011;
   
    AFN0CFun[13] = AFN0C014;   //重庆规约+13规约需要

    AFN0CFun[14] = AFN0C015;   //重庆规约
    AFN0CFun[15] = AFN0C016;   //重庆规约
    
    //组3
    AFN0CFun[16] = AFN0C017;
    AFN0CFun[17] = AFN0C018;
    AFN0CFun[18] = AFN0C019;
    AFN0CFun[19] = AFN0C020; 
    AFN0CFun[20] = AFN0C021;
    AFN0CFun[21] = AFN0C022; 
    AFN0CFun[22] = AFN0C023;
    AFN0CFun[23] = AFN0C024;
    
    //组4
    AFN0CFun[24] = AFN0C025;
    AFN0CFun[25] = AFN0C026;
    AFN0CFun[26] = AFN0C027;
    
    AFN0CFun[27] = AFN0C028;    
    AFN0CFun[28] = AFN0C029;
    AFN0CFun[29] = AFN0C030;
    AFN0CFun[30] = AFN0C031;
    AFN0CFun[31] = AFN0C032;
    
    //组5
    AFN0CFun[32] = AFN0C033;
    AFN0CFun[33] = AFN0C034;
    AFN0CFun[34] = AFN0C035;
    AFN0CFun[35] = AFN0C036;
    AFN0CFun[36] = AFN0C037;
    AFN0CFun[37] = AFN0C038;
    AFN0CFun[38] = AFN0C039;
    AFN0CFun[39] = AFN0C040;
    
    //组6
    AFN0CFun[40] = AFN0C041;
    AFN0CFun[41] = AFN0C042;
    AFN0CFun[42] = AFN0C043;
    AFN0CFun[43] = AFN0C044;
    AFN0CFun[44] = AFN0C045;
    AFN0CFun[45] = AFN0C046;
    AFN0CFun[46] = AFN0C047;
    AFN0CFun[47] = AFN0C048;
    
    //组7
    AFN0CFun[48] = AFN0C049;
    
    //组8
    AFN0CFun[56] = AFN0C057;
    AFN0CFun[57] = AFN0C058;
    
    //组9
    AFN0CFun[64] = AFN0C065;
    AFN0CFun[65] = AFN0C066;
    AFN0CFun[66] = AFN0C067;
    
    //组10
    AFN0CFun[72] = AFN0C073;
    
    //组11
    AFN0CFun[80] = AFN0C081;
    AFN0CFun[81] = AFN0C082;
    AFN0CFun[82] = AFN0C083;
    AFN0CFun[83] = AFN0C084;
    
    //组12
    AFN0CFun[88] = AFN0C089;
    AFN0CFun[89] = AFN0C090;
    AFN0CFun[90] = AFN0C091;
    AFN0CFun[91] = AFN0C092;
    AFN0CFun[92] = AFN0C093;
    AFN0CFun[93] = AFN0C094;
    AFN0CFun[94] = AFN0C095;
    AFN0CFun[95] = AFN0C096;
    
    //组13
    AFN0CFun[96] = AFN0C097;
    AFN0CFun[97] = AFN0C098;
    AFN0CFun[98] = AFN0C099;
    AFN0CFun[99] = AFN0C100;
    AFN0CFun[100] = AFN0C101;
    AFN0CFun[101] = AFN0C102;
    AFN0CFun[102] = AFN0C103;
    
    //组14
    AFN0CFun[104] = AFN0C105;
    AFN0CFun[105] = AFN0C106;
    AFN0CFun[106] = AFN0C107;
    AFN0CFun[107] = AFN0C108;
    AFN0CFun[108] = AFN0C109;
    AFN0CFun[109] = AFN0C110;
    AFN0CFun[110] = AFN0C111;
    AFN0CFun[111] = AFN0C112;
    
    //组15
    AFN0CFun[112] = AFN0C113;
    AFN0CFun[113] = AFN0C114;
    AFN0CFun[114] = AFN0C115;
    AFN0CFun[115] = AFN0C116;
    
    //组16
    AFN0CFun[120] = AFN0C121;
    
    //组17
    AFN0CFun[128] = AFN0C129;
    AFN0CFun[129] = AFN0C130;
    AFN0CFun[130] = AFN0C131;
    AFN0CFun[131] = AFN0C132;
    AFN0CFun[132] = AFN0C133;
    AFN0CFun[133] = AFN0C134;
    AFN0CFun[134] = AFN0C135;
    AFN0CFun[135] = AFN0C136;
    
    //组18
    AFN0CFun[136] = AFN0C137;
    AFN0CFun[137] = AFN0C138;
    AFN0CFun[138] = AFN0C139;
    AFN0CFun[139] = AFN0C140;
    AFN0CFun[140] = AFN0C141;
    AFN0CFun[141] = AFN0C142;
    AFN0CFun[142] = AFN0C143;
    AFN0CFun[143] = AFN0C144;
    
    //组19
    AFN0CFun[144] = AFN0C145;
    AFN0CFun[145] = AFN0C146;
    AFN0CFun[146] = AFN0C147;
    AFN0CFun[147] = AFN0C148;
    AFN0CFun[148] = AFN0C149;
    AFN0CFun[149] = AFN0C150;
    AFN0CFun[150] = AFN0C151;
    AFN0CFun[151] = AFN0C152;
    
    //组20
    AFN0CFun[152] = AFN0C153;
    AFN0CFun[153] = AFN0C154;
    AFN0CFun[154] = AFN0C155;
    AFN0CFun[155] = AFN0C156;
    AFN0CFun[156] = AFN0C157;
    AFN0CFun[157] = AFN0C158;
    AFN0CFun[158] = AFN0C159;
    AFN0CFun[159] = AFN0C160;
    
    //组21
    AFN0CFun[160] = AFN0C161;
    
    AFN0CFun[164] = AFN0C165;
    AFN0CFun[165] = AFN0C166;
    AFN0CFun[166] = AFN0C167;
    AFN0CFun[167] = AFN0C168;
    
    //组22
    AFN0CFun[168] = AFN0C169;
    AFN0CFun[169] = AFN0C170;
    
    if (fQueue.tailPtr == 0)
    {
       tmpHead0c = 0;
    }
    else
    {
       tmpHead0c = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }

    if (poll == AFN0B_REQUIRE)
    {
       frameTail0c = tmpHead0c + 18;
    }
    else
    {
       frameTail0c = tmpHead0c + 14;
    }
    
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    tmpDt1 = 0;
    tmpDtCount = 0;
    ackTail = 0;
    maxCycle = 0;
    
    if (poll==ACTIVE_REPORT)
    {
    	 frame.loadLen = pDataEnd - pDataHead;
    }

    //ly,2011-05-15
    dotReplyStart = 0;

    while ((frame.loadLen > 0) && (maxCycle<1500))
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
           	  
           	  //printf("AFN0C fn=%d\n",fn);
           	  
           	  if (fn<=170)
           	  {
           	    //执行函数
           	    //2013-11-28将fn<=170移到外面先判断,防止数组越界
                //if (AFN0CFun[fn-1] != NULL && fn <= 170)
                if (AFN0CFun[fn-1] != NULL)
                {
                   tmpFrameTail = AFN0CFun[fn-1](frameTail0c, pDataHead, fn);
                   if (tmpFrameTail== frameTail0c)
                   {
                 	   if (fn==129)
                 	   {
                       ;
                 	   }
                 	   else
                 	   {
                 	     ackData[ackTail*5]   = *pDataHead;                             //DA1
                 	     ackData[ackTail*5+1] = *(pDataHead+1);                       //DA2
                 	     ackData[ackTail*5+2] = 0x1 << ((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
                 	     ackData[ackTail*5+3] = (fn-1)/8;                             //DT2
                 	     ackData[ackTail*5+4] = 0x01;                                 //无有效数据
                 	     ackTail++;
                 	   }
                   }
                   else
                   {
                 	    frameTail0c = tmpFrameTail;
                   }
                 }
              }
           }
           
           tmpDt1 >>= 1;
                      
           if ((frameTail0c - tmpHead0c) > MAX_OF_PER_FRAME || (((pDataHead+4) == pDataEnd) && tmpDtCount==8))
           {
              //不允许主动上报且有事件发生
              if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail0c - tmpHead0c) > 16)
              {
              	  msFrame[frameTail0c++] = iEventCounter;
              	  msFrame[frameTail0c++] = nEventCounter;
              }
              
              //根据启动站要求判断是否携带TP
              //ly,2011-10-11,修改这个主动上报Tp的bug(if为true的处理)
              if (frame.pTp != NULL)  //ly,2011-10-24,add this if
              {
                if (poll==ACTIVE_REPORT)
                {
                  msFrame[frameTail0c++] = pfc++;
                  msFrame[frameTail0c++] = hexToBcd(sysTime.second);
                  msFrame[frameTail0c++] = hexToBcd(sysTime.minute);
                  msFrame[frameTail0c++] = hexToBcd(sysTime.hour);
                  msFrame[frameTail0c++] = hexToBcd(sysTime.day);
                  msFrame[frameTail0c++] = 0x0;
                }
                else
                {
                   pTpv = frame.pTp;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv++;
                   msFrame[frameTail0c++] = *pTpv;
                }
              }
              
              msFrame[tmpHead0c + 0] = 0x68;   //帧起始字符
            
              //if (poll==ACTIVE_REPORT)
              //{
              //  tmpI = ((frameTail0c - tmpHead0c - 6) << 2) | 0x01;
              //}
              //else
              //{
                tmpI = ((frameTail0c - tmpHead0c - 6) << 2) | PROTOCOL_FIELD;
              //}
              msFrame[tmpHead0c + 1] = tmpI & 0xFF;   //L
              msFrame[tmpHead0c + 2] = tmpI >> 8;
              msFrame[tmpHead0c + 3] = tmpI & 0xFF;   //L
              msFrame[tmpHead0c + 4] = tmpI >> 8; 
            
              msFrame[tmpHead0c + 5] = 0x68;  //帧起始字符

       
              if (poll == ACTIVE_REPORT)
              {
              	msFrame[tmpHead0c + 6] = 0xc4;    //DIR=1,PRM=1,功能码=0x4
              }
              else
              {
                msFrame[tmpHead0c + 6] = 0x88;     //控制字节10001000(DIR=1,PRM=0,功能码=0x8)
              }

              if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
              {
                  msFrame[tmpHead0c + 6] |= 0x20;
              }
       
              //地址
              msFrame[tmpHead0c + 7] = addrField.a1[0];
              msFrame[tmpHead0c + 8] = addrField.a1[1];
              msFrame[tmpHead0c + 9] = addrField.a2[0];
              msFrame[tmpHead0c + 10] = addrField.a2[1];
              if (poll == ACTIVE_REPORT)
                msFrame[tmpHead0c + 11] = 0;
              else
              	msFrame[tmpHead0c + 11] = addrField.a3;
              
              if (poll == AFN0B_REQUIRE)
              {
                msFrame[tmpHead0c + 12] = 0x0B;  //AFN
              }
              else
              {
                msFrame[tmpHead0c + 12] = 0x0C;  //AFN
              }
       
              msFrame[tmpHead0c+13] = 0;
              
              if (frame.pTp != NULL)
              {
              	 msFrame[tmpHead0c+13] |= 0x80;       //TpV置位
              }
              
              if (poll==AFN0B_REQUIRE)
              {
              	  msFrame[tmpHead0c+14] = *(frame.pData+2);
              	  msFrame[tmpHead0c+15] = *(frame.pData+3);
              	  msFrame[tmpHead0c+16] = *(frame.pData+4);
              	  msFrame[tmpHead0c+17] = *(frame.pData+5);              	  
              }
              
              //frameTail0c++;
              msFrame[frameTail0c+1] = 0x16;
              
              fQueue.frame[fQueue.tailPtr].head = tmpHead0c;
              fQueue.frame[fQueue.tailPtr].len = frameTail0c + 2 - tmpHead0c;
              
              if (((poll != AFN0B_REQUIRE)&&(((frameTail0c - tmpHead0c) > 16 && frame.pTp==NULL) || ((frameTail0c - tmpHead0c) > 22 && frame.pTp!=NULL)))
              	||  ((poll == AFN0B_REQUIRE) && (frameTail0c - tmpHead0c > 20)))
              {  
                 if (poll==ACTIVE_REPORT)
                 {
                 	  //将组好的帧复制到主动上报帧队列中
                    if (fQueue.activeTailPtr == 0)
                    {
                       tmpHead0cActive = 0;
                    }
                    else
                    {
                       tmpHead0cActive = fQueue.activeFrame[fQueue.activeTailPtr-1].head + fQueue.activeFrame[fQueue.activeTailPtr-1].len;
                    }
                    
                    fQueue.activeFrame[fQueue.activeTailPtr].head = tmpHead0cActive;
                    fQueue.activeFrame[fQueue.activeTailPtr].len = fQueue.frame[fQueue.tailPtr].len;
                    for(tmpI=0;tmpI<fQueue.activeFrame[fQueue.activeTailPtr].len;tmpI++)
                    {
                    	 activeFrame[tmpHead0cActive+tmpI] = msFrame[fQueue.frame[fQueue.tailPtr].head+tmpI];
                    }
                    
                    if ((tmpHead0cActive+tmpI+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                    	   || fQueue.activeTailPtr==LEN_OF_SEND_QUEUE-1)
                    {
                       fQueue.activeFrame[fQueue.activeTailPtr].next = 0x0;
                    	 fQueue.activeTailPtr = 0;
                    }
                    else
                    {
                       fQueue.activeFrame[fQueue.activeTailPtr].next = fQueue.activeTailPtr+1;
                       fQueue.activeTailPtr++;
                    }
                 }
                 else
                 {
                    tmpHead0c = frameTail0c+2;
                    if ((tmpHead0c+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                    {
                       fQueue.frame[fQueue.tailPtr].next = 0x0;
                    	 fQueue.tailPtr = 0;
                    	 tmpHead0c = 0;
                    }
                    else
                    {                 
                       fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                       fQueue.tailPtr++;
                    }
                 }

                 if (poll == AFN0B_REQUIRE)
                 {
                    frameTail0c = tmpHead0c + 18;  //frameTail重新置位填写下一帧
                 }
                 else
                 {
                    frameTail0c = tmpHead0c + 14;  //frameTail重新置位填写下一帧
                 }
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
函数名称:numOfTariff
功能描述:
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：测量点的费率个数
*******************************************************/
INT8U numOfTariff(INT16U pn)
{
	 METER_DEVICE_CONFIG meterDeviceConfig;
	
 	 if(selectF10Data(pn, 0, 0, (INT8U *)&meterDeviceConfig, sizeof(METER_DEVICE_CONFIG)) == TRUE)
	 {
		 return meterDeviceConfig.numOfTariff;
 	 }
	 else
	 {
		 return 0;
	 }	  
}

/*******************************************************
函数名称:AFN0C001
功能描述:响应主站请求一类数据"终端版本信息/376.1沃电扩展本地通信模块组网情况"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C001(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
 #ifdef PLUG_IN_CARRIER_MODULE
  INT16U tmpData, i;
  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;   //DA1
  msFrame[frameTail++] = *pHandle++;   //DA2
  msFrame[frameTail++] = 0x01;         //DT1
  msFrame[frameTail++] = 0x00;         //DT2
  
  //本地通信模块类型
  msFrame[frameTail++]   = carrierModuleType;

  tmpData = 0;
  if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
  {
    tmpFound = noFoundMeterHead;
  }
  else
  {
    tmpFound = foundMeterHead;
  }
  while(tmpFound!=NULL)
  {
  	tmpData++;
  	tmpFound = tmpFound->next;
  }

  //入网/未入网节点数量
  msFrame[frameTail++] = tmpData&0xff;
  msFrame[frameTail++] = tmpData>>8&0xff;

  if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
  {
    tmpFound = noFoundMeterHead;
  }
  else
  {
    tmpFound = foundMeterHead;
  }
  
  while(tmpFound!=NULL)
  {
  	memcpy(&msFrame[frameTail], tmpFound->addr, 6);
  	frameTail += 6;
  	
  	tmpFound = tmpFound->next;
  }
  
  //抄读方式
  msFrame[frameTail++] = localCopyForm;
  
  //升级中...
  msFrame[frameTail++] = upRtFlag;
  
  //路由版本
  msFrame[frameTail++] = carrierFlagSet.productInfo[7];
  msFrame[frameTail++] = carrierFlagSet.productInfo[8];

  //路由版本日期
  msFrame[frameTail++] = carrierFlagSet.productInfo[4];
  msFrame[frameTail++] = carrierFlagSet.productInfo[5];
  msFrame[frameTail++] = carrierFlagSet.productInfo[6];
  
 #endif
 
  return frameTail;
}

/*******************************************************
函数名称:AFN0C002
功能描述:响应主站请求一类数据"终端日历时钟"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C002(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    INT8U weekNumber;
    
	  weekNumber = dayWeek(2000+sysTime.year,sysTime.month,sysTime.day);
	  if (weekNumber == 0)
	  	weekNumber = 7;
    
    //数据单元标识
    msFrame[frameTail++] = 0x00;  			//DA1
    msFrame[frameTail++] = 0x00;    		//DA2
    msFrame[frameTail++] = 0x02;        //DT1
    msFrame[frameTail++] = 0x00;        //DT2
    
    //数据单元
	  msFrame[frameTail++] = sysTime.second/10<<4 | sysTime.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
	  msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
	  msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
	  msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;	     //日(前四位BCD码十位，后四位BCD码个位)
	  	
	  if (sysTime.month<10)
	  {
	     msFrame[frameTail++] = weekNumber<<5 | sysTime.month;           //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	  }
	  else
	  {
	  	 msFrame[frameTail++] = weekNumber<<5 | 0x10 | sysTime.month%10; //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	  }
	  msFrame[frameTail++] = sysTime.year/10<<4 | sysTime.year%10;       //年(十位+个位)
	
    return frameTail;
}

/*******************************************************
函数名称:AFN0C003
功能描述:响应主站请求一类数据"终端参数状态"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C003(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    INT8U i;
    
    //数据单元标识
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;    			//DA2
    msFrame[frameTail++] = 0x04;          //DT1
    msFrame[frameTail++] = 0x00;          //DT2
    
    //数据单元
    for(i=0;i<31;i++)
      msFrame[frameTail++] = paraStatus[i];

    return frameTail;
}

/*******************************************************
函数名称:AFN0C004
功能描述:响应主站请求一类数据"终端通信状态"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C004(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    //数据单元标识
    msFrame[frameTail++] = 0x00;   			  //DA1
    msFrame[frameTail++] = 0x00;    			//DA2
    msFrame[frameTail++] = 0x08;          //DT1
    msFrame[frameTail++] = 0x00;          //DT2
    
    //数据单元
	  msFrame[frameTail++] = callAndReport;
	  
	  printf("callAndReport=%02x\n",callAndReport);
	
    return frameTail;
}

/*******************************************************
函数名称:AFN0C005
功能描述:响应主站请求一类数据"终端控制设置状态"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C005(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   #ifdef LOAD_CTRL_MODULE
    INT8U  i,j;
    INT8U  tmpData;
    INT16U tmpTail;
    
    //数据单元标识
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;   				//DA2
    msFrame[frameTail++] = 0x10;          //DT1
    msFrame[frameTail++] = 0;             //DT2

    //数据单元
    
    //保电、剔除和催费告警投入状态
    tmpData = 0x0;
    if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
    {
    	 tmpData |= 0x1; 
    }
    if (toEliminate==CTRL_JUMP_IN)
    {
    	 tmpData |= 0x2;
    }
    if (reminderFee==CTRL_JUMP_IN)
    {
    	 tmpData |= 0x4;
    }    
    msFrame[frameTail++] = tmpData;
    
    
    tmpTail = frameTail++;
    tmpData = 0;
    for(i=0;i<8;i++)
    {
    	 for(j=0;j<totalAddGroup.numberOfzjz;j++)
    	 {
    	   if (totalAddGroup.perZjz[j].zjzNo==i+1)
    	   {
    	 	    tmpData |= 1<<i;
    	 	    
    	 	    msFrame[frameTail++] = periodCtrlConfig[i].ctrlPara.ctrlPeriod;  //功控定值方案号
    	 	    msFrame[frameTail++] = periodCtrlConfig[i].ctrlPara.limitPeriod; //功率时段有效标志位
    	 	    
    	 	    //功控状态
    	 	    msFrame[frameTail] = 0;
    	 	    if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)   //时段控
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x1;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)   //厂休控
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x2;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)   //营业报停控
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x4;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)   //当前功率下浮控
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x8;
    	 	    }
    	 	    frameTail++;
    	 	    
    	 	    //电控状态
    	 	    msFrame[frameTail] = 0x0;
    	 	    if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)   //月电控
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x1;
    	 	    }
    	 	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)   //购电控
    	 	    {
    	 	    	 msFrame[frameTail] |= 0x2;
    	 	    }
    	 	    frameTail++;
    	 	    
    	 	    //功控轮次状态
    	 	    msFrame[frameTail++] = powerCtrlRoundFlag[i].flag;
    	 	    
    	 	    //电控轮次状态
    	 	    msFrame[frameTail++] = electCtrlRoundFlag[i].flag;
    	 	 }
    	 }
    }
    msFrame[tmpTail] = tmpData;  //总加组有效位标志位
   #endif
     
   return frameTail;
}

/*******************************************************
函数名称:AFN0C006
功能描述:响应主站请求一类数据"终端当前控制状态"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C006(INT16U frameTail,INT8U *pHandle, INT8U fn)
{    
   #ifdef LOAD_CTRL_MODULE
    INT8U  i,j;
    INT8U  tmpData;
    INT16U tmpTail;
    INT32U powerInt,powerDec;
    INT8U  powerQuantity;
    
    //数据单元标识
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;    			//DA2
    msFrame[frameTail++] = 0x20;          //DT1
    msFrame[frameTail++] = 0;             //DT2
    
    //数据单元
    
    //遥控跳闸输出状态
    msFrame[frameTail] = 0x0;
    for(i=0;i<CONTROL_OUTPUT;i++)
    {
      if (remoteCtrlConfig[i].status==CTRL_JUMPED)   //是否处于跳闸状态   	
      {
    	   msFrame[frameTail] |= 0x1<<i;
      }
    }
    frameTail++;
    
   #ifndef CQDL_CSM 
    //当前催费告警状态
    if (reminderFee==CTRL_JUMP_IN)
    {
    	 msFrame[frameTail++] = 0x1;
    }
    else
    {
       msFrame[frameTail++] = 0x0;
    }
   #endif

    tmpTail = frameTail;
    frameTail++;
    
    tmpData = 0;
    for(i=0;i<8;i++)
    {
    	for(j=0;j<totalAddGroup.numberOfzjz;j++)
    	{
    		if (totalAddGroup.perZjz[j].zjzNo==i+1)
    	  {
    	  	tmpData |= 1<<i;
    	 	    
    	 	  //当前功控定值
    	 	  if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	  	//当前功率下浮控定值
    	 	    if (powerDownCtrl[i].freezeTime.year==0xff)
    	 	    {
    	 	    	powerInt = powerDownCtrl[i].powerDownLimit;
              powerDec = powerDownCtrl[i].powerLimitWatt*10;
              powerQuantity = dataFormat(&powerInt, &powerDec, 2);
                 
              powerInt = hexToBcd(powerInt);
              powerInt &=0xfff;
              powerInt |= (powerQuantity&0x10)<<8;
              powerInt |= (powerQuantity&0x07)<<13;
                  
              msFrame[frameTail++] = powerInt&0xff;
              msFrame[frameTail++] = powerInt>>8&0xff;
    	 	    }
    	 	    else   //还在等待计算定值
    	 	    {
    	 	    	msFrame[frameTail++] = 0xee;
    	 	    	msFrame[frameTail++] = 0xee;
    	 	    }
    	 	  }
    	 	  else
    	 	  {
    	 	  	if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
    	 	    {
    	 	    	msFrame[frameTail++] = obsCtrlConfig[i].obsLimit&0xff;
    	 	    	msFrame[frameTail++] = obsCtrlConfig[i].obsLimit>>8&0xff;
    	 	    }
    	 	    else
    	 	    {
    	 	    	if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
    	 	    	{
    	 	    		msFrame[frameTail++] = wkdCtrlConfig[i].wkdLimit&0xff;
    	 	    	  msFrame[frameTail++] = wkdCtrlConfig[i].wkdLimit>>8&0xff;
    	 	    	}
    	 	    	else
    	 	    	{
    	 	    	  if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
    	 	    	  {
                  //获取当前时间段的时段索引
                  if ((powerQuantity = getPowerPeriod(sysTime)) != 0)
                  {
                    //根据当前时间, 总加组号, 方案号, 时段索引获取时段功控定值
                    powerDec = 0;
               
                    if (getPowerLimit(i+1, periodCtrlConfig[i].ctrlPara.ctrlPeriod, powerQuantity, (INT8U *)&powerDec))
                    {
                   	  msFrame[frameTail++] = powerDec&0xff;
                   	  msFrame[frameTail++] = powerDec>>8&0xff;                           	  
                    }
                    else
                    {
    	 	    	 	      msFrame[frameTail++] = 0xee;
    	 	    	        msFrame[frameTail++] = 0xee;
                    }
                  }
                  else
                  {
    	 	    		  	msFrame[frameTail++] = 0xee;
    	 	    	      msFrame[frameTail++] = 0xee;
    	 	    	    }
    	 	    	  }
    	 	    	  else
    	 	    	  {
    	 	    	  	msFrame[frameTail++] = 0xee;
    	 	    	    msFrame[frameTail++] = 0xee;
    	 	    	  }
    	 	    	}
    	 	    }
    	 	  }
    	 	    
    	 	  //当前功率下浮控浮动系数
    	 	  if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    msFrame[frameTail++] = powerDownCtrl[i].floatFactor;
    	 	  }
    	 	  else
    	 	  {
    	 	    msFrame[frameTail++] = 0x00;
    	 	  }
    	 	    
    	 	  //功控跳闸输出状态
    	 	  msFrame[frameTail] = 0x0;
    	 	  if ((powerCtrlRoundFlag[i].flag&0x1) && (powerCtrlRoundFlag[i].ifJumped&0x1))
    	 	  {
    	 	    msFrame[frameTail] |= 0x1;
    	 	  }
    	 	  if ((powerCtrlRoundFlag[i].flag>>1&0x1) && (powerCtrlRoundFlag[i].ifJumped>>1&0x1))
    	 	  {
    	 	    msFrame[frameTail] |= 0x2;
    	 	  }
    	 	  if ((powerCtrlRoundFlag[i].flag>>2&0x1) && (powerCtrlRoundFlag[i].ifJumped>>2&0x1))
    	 	  {
    	 	    msFrame[frameTail] |= 0x4;
    	 	  }
    	 	  if ((powerCtrlRoundFlag[i].flag>>3&0x1) && (powerCtrlRoundFlag[i].ifJumped>>3&0x1))
    	 	  {
    	 	  	msFrame[frameTail] |= 0x8;
    	 	  }
    	 	  frameTail++;
    	 	    
    	 	  //月电控跳闸输出状态
    	 	  msFrame[frameTail] = 0x0;
    	 	  if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if ((electCtrlRoundFlag[i].flag&0x1) && (electCtrlRoundFlag[i].ifJumped&0x1))
    	 	    {
    	 	     	msFrame[frameTail] |= 0x1;
    	 	    }
    	 	    if ((electCtrlRoundFlag[i].flag>>1&0x1) && (electCtrlRoundFlag[i].ifJumped>>1&0x1))
    	 	    {
    	 	    	msFrame[frameTail] |= 0x2;
    	 	    }
    	 	    if ((electCtrlRoundFlag[i].flag>>2&0x1) && (electCtrlRoundFlag[i].ifJumped>>2&0x1))
    	 	    {
    	 	    	msFrame[frameTail] |= 0x4;
    	 	    }
    	 	    if ((electCtrlRoundFlag[i].flag>>3&0x1) && (electCtrlRoundFlag[i].ifJumped>>3&0x1))
    	 	    {
    	 	    	msFrame[frameTail] |= 0x8;
    	 	    }
    	 	    frameTail++;
    	 	       
    	 	    //购电控跳闸输出状态
    	 	    msFrame[frameTail++] = 0x0;
    	 	  }
    	 	  else
    	 	  {
    	 	    msFrame[frameTail] = 0x0;
    	 	    frameTail++;
    	 	    	 
    	 	    //购电控跳闸输出状态
            msFrame[frameTail] = 0x0;
    	 	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
    	 	    {
       	 	  	if ((electCtrlRoundFlag[i].flag&0x1) && (electCtrlRoundFlag[i].ifJumped&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x1;
       	 	    }
       	 	    if ((electCtrlRoundFlag[i].flag>>1&0x1) && (electCtrlRoundFlag[i].ifJumped>>1&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x2;
       	 	    }
       	 	    if ((electCtrlRoundFlag[i].flag>>2&0x1) && (electCtrlRoundFlag[i].ifJumped>>2&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x4;
       	 	    }
       	 	    if ((electCtrlRoundFlag[i].flag>>3&0x1) && (electCtrlRoundFlag[i].ifJumped>>3&0x1))
       	 	    {
       	 	    	msFrame[frameTail] |= 0x8;
       	 	    }
       	 	    frameTail++;
    	 	    }
    	 	    else
    	 	    {
    	 	    	msFrame[frameTail++] = 0x0;
    	 	    }
    	 	  }
    	 	    
    	 	  //功控越限告警状态
    	 	  msFrame[frameTail] = 0x0;
    	 	  if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (periodCtrlConfig[i].ctrlPara.prdAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x1;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (wkdCtrlConfig[i].wkdAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x2;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (obsCtrlConfig[i].obsAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x4;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	   	if (powerDownCtrl[i].pwrDownAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x8;
    	 	    }
    	 	  }
    	 	    
    	 	  frameTail++;
    	 	    
    	 	  //电控越限告警状态
    	 	  msFrame[frameTail] = 0x0;
    	 	  if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (monthCtrlConfig[i].monthAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x1;
    	 	    }
    	 	  }
    	 	  if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
    	 	  {
    	 	    if (chargeCtrlConfig[i].chargeAlarm == CTRL_ALARM)
    	 	    {
    	 	    	msFrame[frameTail] |= 0x2;
    	 	    }
    	 	  }    	 	    
    	 	  frameTail++;
            
          //催费控跳闸输出状态
          msFrame[frameTail] = 0x0;
          
    	  }
    	}
    }
    msFrame[tmpTail] = tmpData;  //总加组有效位标志位
  #endif
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0C007
功能描述:响应主站请求一类数据"终端事件计数器当前值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C007(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    //数据单元标识
    msFrame[frameTail++] = 0x00;      			//DA1
    msFrame[frameTail++] = 0x00;      			//DA2
    msFrame[frameTail++] = 0x40;            //DT1
    msFrame[frameTail++] = 0x00;            //DT2
    
    //数据单元
    msFrame[frameTail++] = iEventCounter;   //重要事件计数器EC1值
    msFrame[frameTail++] = nEventCounter;   //一般事件计数器EC2值
	
    return frameTail;
}

/*******************************************************
函数名称:AFN0C008
功能描述:响应主站请求一类数据"终端事件标志状态"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C008(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    INT8U i;
    
    //数据单元标识
    msFrame[frameTail++] = 0x00;    			//DA1
    msFrame[frameTail++] = 0x00;			    //DA2
    msFrame[frameTail++] = 0x80;          //DT1
    msFrame[frameTail++] = 0;             //DT2
    
    //数据单元
    for(i=0;i<8;i++)
    {
    	msFrame[frameTail++] = eventStatus[i];
    }
	
    return frameTail;
}

/*******************************************************
函数名称:AFN0C009
功能描述:响应主站请求一类数据"终端状态量及变位标志"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C009(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    //数据单元标识
    msFrame[frameTail++] = 0;          //DA1
    msFrame[frameTail++] = 0;          //DA2
    msFrame[frameTail++] = 0x1;        //DT1
    msFrame[frameTail++] = 0x1;        //DT2
    
    //数据单元
   #ifndef PLUG_IN_CARRIER_MODULE
    if (getGateKValue()==1)  //门控在第7路
    {
      msFrame[frameTail++] = stOfSwitch | 0x40; //ST
    }
    else
    {
   #endif
   
      msFrame[frameTail++] = stOfSwitch; //ST
   
   #ifndef PLUG_IN_CARRIER_MODULE
    }
   #endif
    
    msFrame[frameTail++] = cdOfSwitch; //CD

	  cdOfSwitch = 0;

    return frameTail;
}

/*******************************************************
函数名称:AFN0C010
功能描述:响应主站请求一类数据"终端与主站当日，月通信流量"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C010(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
	INT8U                  i, tmpCount;
	DATE_TIME              tmpTime;
	INT32U                 tmpData;
	
  //数据单元标识
  msFrame[frameTail++] = 0;       //DA1
  msFrame[frameTail++] = 0;       //DA2
  msFrame[frameTail++] = 0x02;    //DT1
  msFrame[frameTail++] = 0x01;    //DT2
  
 	tmpTime = timeHexToBcd(sysTime);
	if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  {
  	 tmpData = terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
  	 //当日通信流量
  	 msFrame[frameTail++] = tmpData&0xff;
  	 msFrame[frameTail++] = tmpData>>8&0xff;
  	 msFrame[frameTail++] = tmpData>>16&0xff;
  	 msFrame[frameTail++] = tmpData>>24&0xff;
  	 
  	 //当月通信流量
 	   tmpCount = monthDays(sysTime.year+2000,sysTime.month);
 	   tmpData = 0;
 	   for(i=1;i<=tmpCount && i<=sysTime.day;i++)
 	   {
 	  	 tmpTime = sysTime;
 	  	 tmpTime.day = i;
 	  	 tmpTime = timeHexToBcd(tmpTime);
       if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
       {
       	  tmpData += terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
       }
 	   }
  	 msFrame[frameTail++] = tmpData&0xff;
  	 msFrame[frameTail++] = tmpData>>8&0xff;
  	 msFrame[frameTail++] = tmpData>>16&0xff;
  	 msFrame[frameTail++] = tmpData>>24&0xff; 	   
  }
  else
  {
    //当日通信流量
    for(i = 0; i < 4; i++)
 	  {
 		  msFrame[frameTail++] = 0x0;
 	  }
	
	  //当月通信流量
    for(i = 0; i < 4; i++)
 	  {
 		  msFrame[frameTail++] = 0x0;
 	  }
 	}
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0C011
功能描述:响应主站请求一类数据"终端集中抄表状态信息"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2012-06-08,
    1)修正485端口抄表成功块数及重点用户抄表成功的正确性
    2)专变终端不能也不必看到端口31的统计
*******************************************************/
INT16U AFN0C011(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U             i, j, tmpBlock;
	INT16U            tmpSuccess, tmpCount, tmpTail;
	struct cpAddrLink *tmpNode;
	DATE_TIME         tmpTime;
  INT8U             weekNumber;
  INT8U             dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT8U             keyCount;
    	
  //数据单元标识
  msFrame[frameTail++] = 0;       //DA1
  msFrame[frameTail++] = 0;       //DA2
  msFrame[frameTail++] = 0x04;    //DT1
  msFrame[frameTail++] = 0x01;    //DT2
  
  //数据单元
  //本项数据块个数(后面填写)
  tmpTail = frameTail++;

  //1.终端抄表端口号(485端口)	
	tmpBlock = 0;
	for(i=0;i<4;i++)
	{
  	tmpNode = initPortMeterLink(i);
  	tmpSuccess = 0;
  	tmpCount   = 0;
  	keyCount   = 0;
  	while(tmpNode!=NULL)
  	{
  	 	tmpCount++;
      
      tmpTime = timeHexToBcd(sysTime);
      
     #ifdef LIGHTING
		  //2016-12-07,修改485口抄读成功块数处理方法
			if (
			    LIGHTING_XL== tmpNode->protocol
			     || LIGHTING_DGM== tmpNode->protocol
				 )
			{
				tmpTime = sysTime;
				tmpTime.second = 0x0;
				tmpTime.minute = 0x0;
				if (tmpTime.hour>0)
				{
				  tmpTime.hour--;
				}
				
				//十进制日期
				if (readMeterData(dataBuff, tmpNode->mp, HOUR_FREEZE_SLC, 0x0, &tmpTime, 0) == TRUE)
				{
					tmpSuccess++;
				}
			}
     #else		 
			if ((tmpNode->bigAndLittleType&0xf)==1)//单相智能表
      {
        if (readMeterData(dataBuff, tmpNode->mp, SINGLE_PHASE_PRESENT, ENERGY_DATA, &tmpTime, 0) == TRUE)
        {
  	 	    tmpSuccess++;
  	 	  
  	 	    for(j=0; j<keyHouseHold.numOfHousehold; j++)
  	 	    {
  	 	  	  if ((keyHouseHold.household[j*2] | keyHouseHold.household[j*2+1]<<8)==tmpNode->mpNo)
  	 	  	  {
  	 	  		  keyCount++;
  	 	  	  }
  	 	    }
  	 	  }
      }
		 #endif
      else
      {
        if (readMeterData(dataBuff, tmpNode->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0) == TRUE)
        {
  	 	    tmpSuccess++;
  	 	  
  	 	    for(j=0; j<keyHouseHold.numOfHousehold; j++)
  	 	    {
  	 	  	  if ((keyHouseHold.household[j*2] | keyHouseHold.household[j*2+1]<<8)==tmpNode->mpNo)
  	 	  	  {
  	 	  		  keyCount++;
  	 	  	  }
  	 	    }
  	 	  }
      }
  	 	   	  
  	 	tmpNode = tmpNode->next;
  	}
    
    if (tmpCount>0)
    {
      tmpBlock++;
      
      //端口号
      msFrame[frameTail++] = i+1;
  
      //要抄电能表总数
      msFrame[frameTail++] = tmpCount&0xff;
      msFrame[frameTail++] = tmpCount>>8&0xff;
      
      //当前抄表工作状态标志
      if (copyCtrl[i].meterCopying==TRUE)
      {
      	 msFrame[frameTail++] = 0x03;
      }
      else
      {
      	 msFrame[frameTail++] = 0x02;
      }
      
      msFrame[frameTail++] = tmpSuccess&0xff;
      msFrame[frameTail++] = tmpSuccess>>8&0xff;
      
      //抄重点用户成功块数
      msFrame[frameTail++] = keyCount;
      
      //抄表开始时间
      tmpTime = timeBcdToHex(copyCtrl[i].lastCopyTime);
    	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
    	if (weekNumber == 0)
    	{
    	  weekNumber = 7;
    	}
    	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
    	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
    	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
    	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //日(前四位BCD码十位，后四位BCD码个位)
    	  	
    	if (tmpTime.month<10)
    	{
    	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
    	}
    	else
    	{
    	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10;  //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
    	}
    	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //年(十位+个位)
    	      
      //抄表结束时间
      tmpTime = copyCtrl[i].nextCopyTime;
    	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
    	if (weekNumber == 0)
    	{
    	  weekNumber = 7;
    	}
    	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
    	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
    	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
    	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //日(前四位BCD码十位，后四位BCD码个位)
    	  	
    	if (tmpTime.month<10)
    	{
    	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
    	}
    	else
    	{
    	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10;  //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
    	}
    	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //年(十位+个位)
    }
  }
  
  //2.载波/无线端口
  #ifdef PLUG_IN_CARRIER_MODULE
    msFrame[frameTail++] = PORT_POWER_CARRIER;
  	
  	tmpNode = copyCtrl[4].cpLinkHead;
  	tmpSuccess = 0;
  	tmpCount   = 0;
  	keyCount   = 0;
  	while(tmpNode!=NULL)
  	{
  	 	tmpCount++;
  	 	if (tmpNode->copySuccess==TRUE)
  	 	{
  	 	  tmpSuccess++;
  	 	  
  	 	  for(j=0; j<keyHouseHold.numOfHousehold; j++)
  	 	  {
  	 	  	if ((keyHouseHold.household[j*2] | keyHouseHold.household[j*2+1]<<8)==tmpNode->mpNo)
  	 	  	{
  	 	  		keyCount++;
  	 	  	}
  	 	  }
  	 	}
  
  	 	tmpNode = tmpNode->next;
  	}
  	
    //要抄电能表总数
    msFrame[frameTail++] = tmpCount&0xff;
    msFrame[frameTail++] = tmpCount>>8&0xff;
    
    //当前抄表工作状态标志
    if (copyCtrl[4].meterCopying==TRUE)
    {
    	 msFrame[frameTail++] = 0x03;
    }
    else
    {
    	 msFrame[frameTail++] = 0x02;
    }
    
    //抄表成功块数
    msFrame[frameTail++] = tmpSuccess&0xff;
    msFrame[frameTail++] = tmpSuccess>>8&0xff;
    
    //抄重点用户成功块数
    msFrame[frameTail++] = keyHouseHold.numOfHousehold;
    
    //抄表开始时间
    tmpTime = timeBcdToHex(copyCtrl[4].lastCopyTime);
  	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
  	if (weekNumber == 0)
  	{
  	  weekNumber = 7;
  	}
  	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
  	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
  	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
  	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //日(前四位BCD码十位，后四位BCD码个位)
  	  	
  	if (tmpTime.month<10)
  	{
  	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
  	}
  	else
  	{
  	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10; //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
  	}
  	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //年(十位+个位)
  	    
    //抄表结束时间
    tmpTime = copyCtrl[4].nextCopyTime;
  	weekNumber = dayWeek(2000+tmpTime.year,tmpTime.month,tmpTime.day);
  	if (weekNumber == 0)
  	{
  	  weekNumber = 7;
  	}
  	msFrame[frameTail++] = tmpTime.second/10<<4 | tmpTime.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
  	msFrame[frameTail++] = tmpTime.minute/10<<4 | tmpTime.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
  	msFrame[frameTail++] = tmpTime.hour  /10<<4 | tmpTime.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
  	msFrame[frameTail++] = tmpTime.day   /10<<4 | tmpTime.day   %10;	     //日(前四位BCD码十位，后四位BCD码个位)
  	  	
  	if (tmpTime.month<10)
  	{
  	   msFrame[frameTail++] = weekNumber<<5 | tmpTime.month;           //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
  	}
  	else
  	{
  	  msFrame[frameTail++] = weekNumber<<5 | 0x10 | tmpTime.month%10; //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
  	}
  	msFrame[frameTail++] = tmpTime.year/10<<4 | tmpTime.year%10;       //年(十位+个位)
  	
  	//数据块数
  	msFrame[tmpTail] = tmpBlock+1;
  
  #else
  
  	//数据块数
  	msFrame[tmpTail] = tmpBlock;
  #endif
	
  return frameTail;
}

/*******************************************************
函数名称:AFN0C014
功能描述:响应主站请求一类数据"终端投运状态(重庆规约)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C014(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = 0;       //DA1
   msFrame[frameTail++] = 0;       //DA2
   msFrame[frameTail++] = 0x20;    //DT1
   msFrame[frameTail++] = 0x01;    //DT2
   
  #ifdef CQDL_CSM
   msFrame[frameTail++] = teInRunning;  //集中器投运状态
  #else    //13版-文件传输未收到的数据段
   unRecvSeg(&msFrame[frameTail]);
   frameTail += 130;
  #endif
   
   
   return frameTail;
}

/*******************************************************
函数名称:AFN0C015
功能描述:响应主站请求一类数据"终端搜索失败表统计信息(重庆规约)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C015(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   INT8U weekNumber;

   //数据单元标识
   msFrame[frameTail++] = 0;       //DA1
   msFrame[frameTail++] = 0;       //DA2
   msFrame[frameTail++] = 0x40;    //DT1
   msFrame[frameTail++] = 0x01;    //DT2

   //数据单元
   //1.搜索开始时间
	 weekNumber = dayWeek(2000+searchStart.year,searchStart.month,searchStart.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchStart.second/10<<4 | searchStart.second%10;   //秒(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchStart.minute/10<<4 | searchStart.minute%10;   //分(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchStart.hour  /10<<4 | searchStart.hour  %10;   //时(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchStart.day   /10<<4 | searchStart.day   %10;	  //日(前四位BCD码十位，后四位BCD码个位)
	  	
	 if (searchStart.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchStart.month;               //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchStart.month%10;     //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 msFrame[frameTail++] = searchStart.year/10<<4 | searchStart.year%10;       //年(十位+个位)

   //2.搜索结束时间
	 weekNumber = dayWeek(2000+searchEnd.year,searchEnd.month,searchEnd.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchEnd.second/10<<4 | searchEnd.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchEnd.minute/10<<4 | searchEnd.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchEnd.hour  /10<<4 | searchEnd.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchEnd.day   /10<<4 | searchEnd.day   %10;	      //日(前四位BCD码十位，后四位BCD码个位)
	  	
	 if (searchEnd.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchEnd.month;                 //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchEnd.month%10;       //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 msFrame[frameTail++] = searchEnd.year/10<<4 | searchEnd.year%10;           //年(十位+个位)
	 
	 //3.注册电表总块数
	 msFrame[frameTail++] =  meterDeviceNum&0xff;
	 msFrame[frameTail++] =  meterDeviceNum>>8&0xff;
	 
	 //4.抄读失败电表总块数
	 msFrame[frameTail++] =  0x0;
	 msFrame[frameTail++] =  0x0;
	 
	 return frameTail;
}

/*******************************************************
函数名称:AFN0C016
功能描述:响应主站请求一类数据"终端搜索新增表统计信息(重庆规约)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C016(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   struct carrierMeterInfo *tmpNewFound;
   INT8U                   weekNumber;
   INT16U                  numOfNewMeter;
   INT16U                  tmpTail;
	
   //数据单元标识
   msFrame[frameTail++] = 0;       //DA1
   msFrame[frameTail++] = 0;       //DA2
   msFrame[frameTail++] = 0x80;    //DT1
   msFrame[frameTail++] = 0x01;    //DT2
  
   //数据单元
   //1.搜索开始时间
	 weekNumber = dayWeek(2000+searchStart.year,searchStart.month,searchStart.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchStart.second/10<<4 | searchStart.second%10;   //秒(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchStart.minute/10<<4 | searchStart.minute%10;   //分(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchStart.hour  /10<<4 | searchStart.hour  %10;   //时(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchStart.day   /10<<4 | searchStart.day   %10;	  //日(前四位BCD码十位，后四位BCD码个位)
	  	
	 if (searchStart.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchStart.month;               //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchStart.month%10;     //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 msFrame[frameTail++] = searchStart.year/10<<4 | searchStart.year%10;       //年(十位+个位)

   //2.搜索结束时间
	 weekNumber = dayWeek(2000+searchEnd.year,searchEnd.month,searchEnd.day);
	 if (weekNumber == 0)
	 {
	  	weekNumber = 7;
	 }
	 msFrame[frameTail++] = searchEnd.second/10<<4 | searchEnd.second%10;       //秒(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchEnd.minute/10<<4 | searchEnd.minute%10;       //分(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchEnd.hour  /10<<4 | searchEnd.hour  %10;       //时(前四位BCD码十位，后四位BCD码个位)
	 msFrame[frameTail++] = searchEnd.day   /10<<4 | searchEnd.day   %10;	      //日(前四位BCD码十位，后四位BCD码个位)
	  	
	 if (searchEnd.month<10)
	 {
	    msFrame[frameTail++] = weekNumber<<5 | searchEnd.month;                 //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 else
	 {
	  	msFrame[frameTail++] = weekNumber<<5 | 0x10 | searchEnd.month%10;       //星期-月(前三位BCD码星期，第4位BCD码月十位，后四位BCD码月个位)
	 }
	 msFrame[frameTail++] = searchEnd.year/10<<4 | searchEnd.year%10;           //年(十位+个位)
	 
	 //3.注册电表总块数
	 msFrame[frameTail++] =  meterDeviceNum&0xff;
	 msFrame[frameTail++] =  meterDeviceNum>>8&0xff;
	 
	 //4.新增电表总块数及各表地址
	 tmpTail = frameTail;
	 frameTail += 2;
	 numOfNewMeter = 0;
  #ifdef PLUG_IN_CARRIER_MODULE
   tmpNewFound = foundMeterHead;
	 while(tmpNewFound!=NULL)
	 {
     numOfNewMeter++;
     
     memcpy(&msFrame[frameTail], tmpNewFound->addr, 6);
     frameTail += 6;     
     
     tmpNewFound = tmpNewFound->next;
	 }
	#endif
	
	 msFrame[tmpTail]   = numOfNewMeter&0xff;;
	 msFrame[tmpTail+1] = numOfNewMeter>>8&0xff;;
  
   return frameTail;
}

/*******************************************************
函数名称:AFN0C017
功能描述:响应主站请求一类数据"当前总加有功功率"命令(数据格式2)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C017(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
  INT16U    offset;
  DATE_TIME tmpTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		if(pn > 8)
  		{
  			return frameTail;
  		}
  		
  		//数据单元标识
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  //DA1
      msFrame[frameTail++] = 0x01<<((pn-1)/8);                    //DA2
      
      //查找数据
      if (fn == 17)
      {    
         msFrame[frameTail++] = 0x01;      //DT1
         offset = GP_WORK_POWER;
      }
      else
      {
         msFrame[frameTail++] = 0x02;      //DT1 
      	 offset = GP_NO_WORK_POWER;
      }
      msFrame[frameTail++] = 0x02;         //DT2
      
      tmpTime = timeHexToBcd(sysTime);
      if (readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
	    {
	      if (dataBuff[offset+1] != 0xEE)
	      {
	        msFrame[frameTail++] = dataBuff[offset+1];        //十位 个位
	        if (dataBuff[offset]==0xee)
	        {
	        }
	        else
	        {
            msFrame[frameTail++] = ((dataBuff[offset]&0x7)<<5)
                               |(dataBuff[offset]&0x10)
                               |dataBuff[offset+2]&0x0F;    //数量级 符号 百位
          }
        }
        else
        {
          #ifdef NO_DATA_USE_PART_ACK_03
           frameTail -= 4;
          #else          
           msFrame[frameTail++] = 0xee;
           msFrame[frameTail++] = 0xee;
          #endif
        }
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
          frameTail -= 4;
        #else          
          msFrame[frameTail++] = 0xee;
          msFrame[frameTail++] = 0xee;
        #endif
      }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0C018
功能描述:响应主站请求一类数据"当前总加无功功率"命令(数据格式2)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C018(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C017(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C019
功能描述:响应主站请求一类数据"当日总加有功电能量"命令（数据格式3）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C019(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
  INT8U     tariff, tmpTariff;             //费率数
  INT8U     da1, da2;
  INT16U    offset;
  BOOL      ifHasData;
  DATE_TIME tmpTime;
  INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-14,add
  BOOL      bufHasData;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		if(pn > 8)
  		{
  			return frameTail;
  		}  
  		
  		//数据单元标识
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  //DA1
      msFrame[frameTail++] = 0x01<<((pn-1)/8);                    //DA2

      //查找数据
      switch (fn)
      {
     	  case 19:
          msFrame[frameTail++] = 0x04;    //DT1
          offset = GP_DAY_WORK;      
          break;
          
        case 20:
       	  msFrame[frameTail++] = 0x08;    //DT1
       	  offset = GP_DAY_NO_WORK;
       	  break;
    	 
    	  case 21:
    	 	  msFrame[frameTail++] = 0x10;    //DT1
    	 	  offset = GP_MONTH_WORK;
          break;
         
        case 22:
       	  msFrame[frameTail++] = 0x20;    //DT1
       	  offset = GP_MONTH_NO_WORK;
       	  break;
      }
      msFrame[frameTail++] = 0x02;    //DT2
    
      //if ((tariff = numOfTariff(pn)) == 0)
      //{
      //	tariff = 4;
      //}
    
      tariff = 4;
      
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
      
      tmpTime = timeHexToBcd(sysTime);
      
      bufHasData = FALSE;
      
      if (onlyHasPulsePn==0xaa)  //只有脉冲测量点立即计算一次
      {
      	memset(dataBuff, 0xee, LEN_OF_ZJZ_BALANCE_RECORD);
      	
        if (fn==19 || fn==21)
        {
          bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, tmpTime);
        }
        else
        {
          bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_NO_WORK | 0x80, tmpTime);
        }
      	
      	tariff = periodTimeOfCharge[48];
      }
      else    //混合有485测量点读取上次计算值
      {
         bufHasData = readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0);
      }

      if (bufHasData == TRUE)
  	  {
	  	   ifHasData = FALSE;
    	   msFrame[frameTail++] = tariff;
       
         for(tmpTariff=0;tmpTariff <=tariff;tmpTariff++)
         {
           if (dataBuff[offset]!=0xEE)
           {
             ifHasData = TRUE;
           }
           msFrame[frameTail++] = dataBuff[offset+3];
           msFrame[frameTail++] = dataBuff[offset+4];
           msFrame[frameTail++] = dataBuff[offset+5];
          
           if (dataBuff[offset]!=0xee)
           {
             msFrame[frameTail++] = ((dataBuff[offset]&0x01)<<6)
                                  |(dataBuff[offset]&0x10)
                                  |(dataBuff[offset+6]&0x0f);
           }
           else
           {
             msFrame[frameTail++] = 0xee;
           }
           
           offset += 7;
         }
        
         #ifdef NO_DATA_USE_PART_ACK_03
          if (!ifHasData)
          {
            frameTail -= (tariff+1)*4+5;
          }
         #endif
	    } 
	    else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 4;
        #else
	  	  msFrame[frameTail++] = tariff;
        for(tmpTariff=0;tmpTariff < (tariff+1)*4;tmpTariff++)
        {
          msFrame[frameTail++] = 0xee;
        }
        #endif
      }
  	}
  	da1 >>= 1;
  }

  return frameTail;
}

/*******************************************************
函数名称:AFN0C020
功能描述:响应主站请求一类数据"当日总加无功电能量"命令（数据格式3）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C020(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    return AFN0C019(frameTail,pHandle,fn);
}

/*******************************************************
函数名称:AFN0C021
功能描述:响应主站请求一类数据"当月总加有功电能量"命令（数据格式3）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C021(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    return AFN0C019(frameTail,pHandle,fn);
}

/*******************************************************
函数名称:AFN0C022
功能描述:响应主站请求一类数据"当月总加无功电能量"命令（数据格式3）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C022(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    return AFN0C019(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C023
功能描述:响应主站请求一类数据"终端当前剩余电量(费)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C023(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   #ifdef LOAD_CTRL_MODULE
    INT16U    tmpPn,pn;
    INT8U     leftReadBuf[12];
    INT8U     da1, da2;
    DATE_TIME tmpTime;
    INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-14,add
    BOOL      bufHasData;

    da1 = *pHandle++;
    da2 = *pHandle++;
    
    tmpPn = 0;
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		if (pn>32)
		    {
		    	 return frameTail;
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

		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x40;
		    msFrame[frameTail++] = 0x02;
        
        tmpTime = sysTime;
        if (onlyHasPulsePn==0xaa)  //只有测量点立即计算一次
        {
        	if (debugInfo&PRINT_PULSE_DEBUG)
        	{
        	  printf("总加组只有脉冲测量点,计算及时剩余电量\n");
        	}
        	
        	memset(leftReadBuf,0xee,12);

          bufHasData = computeInTimeLeftPower(pn, tmpTime, leftReadBuf, 1);
        }
        else    //混合有485测量点读取上次计算值
        {
        	if (debugInfo&PRINT_PULSE_DEBUG)
        	{
        	  printf("总加组混合测量点,读取上次计算的剩余电量\n");
        	}

        	bufHasData = readMeterData(leftReadBuf, pn, LEFT_POWER, 0x0, &tmpTime, 0);
        }

        if (bufHasData==TRUE)
        {
      	  msFrame[frameTail++] = leftReadBuf[0];
      	  msFrame[frameTail++] = leftReadBuf[1];
      	  msFrame[frameTail++] = leftReadBuf[2];
      	  msFrame[frameTail++] = leftReadBuf[3];
        }
        else
        {
      	  msFrame[frameTail++] = 0xee;
      	  msFrame[frameTail++] = 0xee;
      	  msFrame[frameTail++] = 0xee;
      	  msFrame[frameTail++] = 0xee;        	 
        }		    
    	}
    	da1 >>= 1;
    }
   #endif
   
   return frameTail;
}

/*******************************************************
函数名称:AFN0C024
功能描述:响应主站请求一类数据"当前功率下浮控控后总加有功功率冻结值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C024(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   #ifdef LOAD_CTRL_MODULE
    INT16U pn, tmpPn = 0;
    INT8U  da1, da2;
    INT32U powerInt,powerDec;
    INT8U  powerQuantity;
    
    da1 = *pHandle++;
    da2 = *pHandle++;
    
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		if (pn>32)
		    {
		    	 return frameTail;
		    }
		    
		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x80;
		    msFrame[frameTail++] = 0x02;
		    
		    if (powerDownCtrl[pn-1].freezeTime.year==0xff)
		    {
		       powerInt = powerDownCtrl[pn-1].powerDownLimit;
		       powerDec = powerDownCtrl[pn-1].powerLimitWatt*10;
		       powerQuantity = dataFormat(&powerInt, &powerDec, 2);
		                 
		       powerInt = hexToBcd(powerInt);
		       powerInt &=0xfff;
		       powerInt |= (powerQuantity&0x10)<<8;
		       powerInt |= (powerQuantity&0x07)<<13;
		       
		       msFrame[frameTail++] = powerInt&0xff;
		       msFrame[frameTail++] = powerInt>>8&0xff;
		    }
		    else
		    {
		    	 msFrame[frameTail++] = 0xee;
		    	 msFrame[frameTail++] = 0xee;
		    }
    	}
    	da1 >>= 1;
    }
   #endif
   
   return frameTail;
}

/*******************************************************
函数名称:AFN0C025
功能描述:响应主站请求一类数据"(测量点)相及总有／无功功率、功率因数，
         三相电压、电流、零序电流"命令（数据格式15，9，5，7，6）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C025(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG  meterConfig;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT8U     tmpPn, pn;
  INT8U     i, j;
  INT8U     da1, da2;
  INT8U     pulsePnData = 0;  
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    tmpTail,counti;  
  DATE_TIME time;
  INT8U     meterInfo[10];
	
  tmpPn = 0;
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  meterInfo[0] = 0xff;
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);

   		#ifdef PULSE_GATHER
			 	//查看是否是脉冲采样的数据
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //是脉冲量的测量点
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
			    {
			      //P.1先初始化缓存
            memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);

				 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
			   	  covertPulseData(j, NULL,NULL,dataBuff);
  		      
  		      //ly,2011-05-21,add
  		      time = timeHexToBcd(sysTime);

			      pulsePnData = 1;
			   	  	 	    
			      buffHasData = TRUE;
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //不是脉冲测量点数据
		  {
		  	buffHasData = FALSE;
		  	
        if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
        {
		      if (meterConfig.protocol==AC_SAMPLE)
		      {
		     	  if (ifHasAcModule==TRUE)
		     	  {
			        //A.1先初始化缓存
			        for (counti = 0; counti < LENGTH_OF_PARA_RECORD;counti++)
			        {
			          dataBuff[counti] = 0xEE;
			        }
			   
			       	//A.2将交流采样数据填入dataBuff中
			       	covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
  		        
  		        //ly,2011-05-21,add
  		        time = timeHexToBcd(sysTime);

			     	  buffHasData = TRUE;
		   	    }
		      }
		  	  else
		  	  {
		  	    queryMeterStoreInfo(pn, meterInfo);
		  	      
		  	    if (meterInfo[0]==8)
		  	    {
		  	      buffHasData =  readMeterData(dataBuff, pn , KEY_HOUSEHOLD_PRESENT, PARA_VARIABLE_DATA, &time, 0);
		  	    }
		  	    else
		  	    {
		  	      //buffHasData =  readMeterData(dataBuff, pn , PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0);
		  	      buffHasData =  readMeterData(dataBuff, pn , LAST_TODAY, PARA_VARIABLE_DATA, &time, 0);
		  	    }
		  	  }
		  	}
		  }
		    	
		  msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
		  msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
		  msFrame[frameTail++] = 0x1;                                       //DT1
		  msFrame[frameTail++] = 0x3;                                       //DT2
		
		  ifHasData = FALSE;
		  if (buffHasData == TRUE)
		  {
		    //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
		     		     
		     //总有功功率
		     if (dataBuff[POWER_INSTANT_WORK] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_WORK];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_WORK+1];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_WORK+2];
		       
		     //A相有功功率
		     if (dataBuff[POWER_PHASE_A_WORK] != 0xEE)
		     {
		   	 		ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_WORK];
		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_WORK+1];
		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_WORK+2];
		        
		     //B相有功功率
         if (dataBuff[POWER_PHASE_B_WORK] != 0xEE && dataBuff[POWER_PHASE_B_WORK] != 0xFF)
         {
        	 ifHasData = TRUE;
           msFrame[frameTail++] = dataBuff[POWER_PHASE_B_WORK];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_B_WORK+1];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_B_WORK+2];
         }
         else
         {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
         }
        
         //C相有功功率
         if (dataBuff[POWER_PHASE_C_WORK] != 0xEE && dataBuff[POWER_PHASE_C_WORK] != 0xFF)
         {
        	 ifHasData = TRUE;
           msFrame[frameTail++] = dataBuff[POWER_PHASE_C_WORK];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_C_WORK+1];
           msFrame[frameTail++] = dataBuff[POWER_PHASE_C_WORK+2];
         }
         else
         {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
         }
		        
		     //总无功功率
		     if (dataBuff[POWER_INSTANT_NO_WORK] != 0xEE)
		     {
		      	ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_NO_WORK];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_NO_WORK+1];
		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_NO_WORK+2];
		        
		     //A相无功功率
		     if (dataBuff[POWER_PHASE_A_NO_WORK] != 0xEE && dataBuff[POWER_PHASE_A_NO_WORK]!=0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_A_NO_WORK];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_A_NO_WORK+1];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_A_NO_WORK+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		       
		     //B相无功功率
		     if (dataBuff[POWER_PHASE_B_NO_WORK] != 0xEE && dataBuff[POWER_PHASE_B_NO_WORK]!=0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_NO_WORK];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_NO_WORK+1];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_NO_WORK+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		        
		     //C相无功功率
		     if (dataBuff[POWER_PHASE_C_NO_WORK] != 0xEE && dataBuff[POWER_PHASE_C_NO_WORK]!=0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_NO_WORK];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_NO_WORK+1];
		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_NO_WORK+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		     
		     if (meterInfo[0]==8)
		     {
		     	  memset(&msFrame[frameTail], 0xee, 8);
		     	  frameTail += 8;
  		   }
  		   else
  		   {
  		     //总功率因数
  		     if (dataBuff[TOTAL_POWER_FACTOR] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[TOTAL_POWER_FACTOR];
  		     msFrame[frameTail++] = dataBuff[TOTAL_POWER_FACTOR+1];
  		      
  		     //A相功率因数
  		     if (dataBuff[FACTOR_PHASE_A] != 0xEE && dataBuff[FACTOR_PHASE_A] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_A];
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_A+1];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		       
  		     //B相功率因数
  		     if (dataBuff[FACTOR_PHASE_B] != 0xEE && dataBuff[FACTOR_PHASE_B] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_B];
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_B+1];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;		     	 
  		     }
  		        
  		     //C相功率因数
  		     if (dataBuff[FACTOR_PHASE_C] != 0xEE && dataBuff[FACTOR_PHASE_C] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_C];
  		       msFrame[frameTail++] = dataBuff[FACTOR_PHASE_C+1];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		   }
		        
		     //A相电压
		     if (dataBuff[VOLTAGE_PHASE_A] != 0xEE)
		     {
		      	ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_A];
		     msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_A+1];
		        
		     //B相电压
		     if (dataBuff[VOLTAGE_PHASE_B] != 0xEE  && dataBuff[VOLTAGE_PHASE_B] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_B];
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_B+1];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
         }
		        
		     //C相电压
		     if (dataBuff[VOLTAGE_PHASE_C] != 0xEE  && dataBuff[VOLTAGE_PHASE_C] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_C];
		       msFrame[frameTail++] = dataBuff[VOLTAGE_PHASE_C+1];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		       
		     //A相电流
		     if (dataBuff[CURRENT_PHASE_A] != 0xEE)
		     {
		      	ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[CURRENT_PHASE_A];
		     msFrame[frameTail++] = dataBuff[CURRENT_PHASE_A+1];
		     msFrame[frameTail++] = dataBuff[CURRENT_PHASE_A+2];
		        
		     //B相电流
		     if (dataBuff[CURRENT_PHASE_B] != 0xEE  && dataBuff[CURRENT_PHASE_B] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_B];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_B+1];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_B+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }
		        
		     //C相电流
		     if (dataBuff[CURRENT_PHASE_C] != 0xEE && dataBuff[CURRENT_PHASE_C] != 0xFF)
		     {
		       ifHasData = TRUE;
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_C];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_C+1];
		       msFrame[frameTail++] = dataBuff[CURRENT_PHASE_C+2];
		     }
		     else
		     {
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
        	 msFrame[frameTail++] = 0xee;
		     }

		     if (meterInfo[0]==8)
		     {
		     	  memset(&msFrame[frameTail], 0xee, 15);
		     	  frameTail += 15;
  		   }
  		   else
  		   {
  		     //零序电流
  		     if (dataBuff[ZERO_SERIAL_CURRENT] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[ZERO_SERIAL_CURRENT];
  		     msFrame[frameTail++] = dataBuff[ZERO_SERIAL_CURRENT+1];
  		     msFrame[frameTail++] = dataBuff[ZERO_SERIAL_CURRENT+2];
  		     
  		     //当前总视在功率
  		     if (dataBuff[POWER_INSTANT_APPARENT] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_APPARENT];
  		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_APPARENT+1];
  		     msFrame[frameTail++] = dataBuff[POWER_INSTANT_APPARENT+2];
  		     
  		     //当前A相视在功率
  		     if (dataBuff[POWER_PHASE_A_APPARENT] != 0xEE)
  		     {
  		      	ifHasData = TRUE;
  		     }
  		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_APPARENT];
  		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_APPARENT+1];
  		     msFrame[frameTail++] = dataBuff[POWER_PHASE_A_APPARENT+2];
  		     
  		     //当前B相视在功率
  		     if (dataBuff[POWER_PHASE_B_APPARENT] != 0xEE && dataBuff[POWER_PHASE_B_APPARENT] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_APPARENT];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_APPARENT+1];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_B_APPARENT+2];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		     
  		     //当前C相视在功率
  		     if (dataBuff[POWER_PHASE_C_APPARENT] != 0xEE && dataBuff[POWER_PHASE_C_APPARENT] != 0xFF)
  		     {
  		       ifHasData = TRUE;
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_APPARENT];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_APPARENT+1];
  		       msFrame[frameTail++] = dataBuff[POWER_PHASE_C_APPARENT+2];
  		     }
  		     else
  		     {
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
          	 msFrame[frameTail++] = 0xee;
  		     }
  		     
  		    #ifdef NO_DATA_USE_PART_ACK_03
  		     if (ifHasData == FALSE)
  		     {
  		       frameTail -= 71;
  		     }
  		    #endif
  		   }
		  }
		  else
		  {
		    #ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
			    msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
			    msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
			    msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
			    msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
			    for(j=0;j<62;j++)
			    {
			  	  msFrame[frameTail++] = 0xee;
			    }
		    #endif
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0C026
功能描述:响应主站请求一类数据"A、B、C三相断相统计数据
         及最后一次断相记录"命令（数据格式15，8，10，17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C026(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     i;  
  BOOL      ifHasData;
  DATE_TIME time;
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 -1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
  		
  		msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      msFrame[frameTail++] = 0x2;                                       //DT1
      msFrame[frameTail++] = 0x3;                                       //DT2

    	ifHasData = FALSE;
	        
      //读取制指定测量点、类型和时间的数据页
      if ( readMeterData(dataBuff, pn, PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0) == TRUE)
      {
        //数据均可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
  
        //总断相次数
        if (dataBuff[PHASE_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_DOWN_TIMES+1];
        
        //A相断相次数
        if (dataBuff[PHASE_A_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_A_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_A_DOWN_TIMES+1];
        
        //B相断相次数
        if (dataBuff[PHASE_B_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_B_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_B_DOWN_TIMES+1];
        
        //C相断相次数
        if (dataBuff[PHASE_C_DOWN_TIMES] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PHASE_C_DOWN_TIMES];
        msFrame[frameTail++] = dataBuff[PHASE_C_DOWN_TIMES+1];
        
        //总断时间
        if (dataBuff[TOTAL_PHASE_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_DOWN_TIME+2];
        
        //A相断相时间
        if (dataBuff[TOTAL_PHASE_A_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_A_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_A_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_A_DOWN_TIME+2];
        
        //B相断相时间
        if (dataBuff[TOTAL_PHASE_B_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_B_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_B_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_B_DOWN_TIME+2];
        
        //C相断相时间
        if (dataBuff[TOTAL_PHASE_C_DOWN_TIME] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_C_DOWN_TIME];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_C_DOWN_TIME+1];
        msFrame[frameTail++] = dataBuff[TOTAL_PHASE_C_DOWN_TIME+2];
        
        //最近一次断相起始时刻
        if (dataBuff[LAST_PHASE_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        //分时日月
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_BEGIN+4];
        
        //A相最近一次断相起始时刻
        if (dataBuff[LAST_PHASE_A_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_BEGIN+4];
        
        //B相最近一次断相起始时刻
        if (dataBuff[LAST_PHASE_B_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_BEGIN+4];
        
        //C相最近一次断相起始时刻
        if (dataBuff[LAST_PHASE_C_DOWN_BEGIN] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_BEGIN+4];
        
        //最近一次断相结束时刻
        if (dataBuff[LAST_PHASE_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_DOWN_END+4];
        
        //A相最近一次断相结束时刻
        if (dataBuff[LAST_PHASE_A_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_A_DOWN_END+4];
        
        //B相最近一次断相结束时刻
        if (dataBuff[LAST_PHASE_B_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_B_DOWN_END+4];
        
        //C相最近一次断相结束时刻
        if (dataBuff[LAST_PHASE_C_DOWN_END] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+1];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+2];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+3];
        msFrame[frameTail++] = dataBuff[LAST_PHASE_C_DOWN_END+4];
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	          frameTail -= 61;
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         
         for(i=0;i<52;i++)
         {
         	  msFrame[frameTail++] = 0xEE;
         }
        #endif
      }
  	}
  	da1 >>= 1;
  }
    	
  return frameTail;        
}

/*******************************************************
函数名称:AFN0C027
功能描述:响应主站请求一类数据"电能表日历时钟,编程次数及其
						最近一次操作时间"命令（数据格式15,1,17,8,10）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C027(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     i;  
  BOOL      ifHasData;  
  DATE_TIME time;
	
	da1 = *pHandle++;
	da2 = *pHandle++;
	  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
 			
 			msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      msFrame[frameTail++] = 0x4;                                       //DT1
      msFrame[frameTail++] = 0x3;                                       //DT2

    	ifHasData = FALSE;
	        
      //读取制指定测量点、类型和时间的数据页
      if ( readMeterData(dataBuff, pn, PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0) == TRUE)
      {
        //数据均可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
        
        //电能表日历时钟
        //秒分时
        if (dataBuff[METER_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[METER_TIME];
        msFrame[frameTail++] = dataBuff[METER_TIME+1];
        msFrame[frameTail++] = dataBuff[METER_TIME+2];
      
      	//日 星期-月 年
        if (dataBuff[DATE_AND_WEEK] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[DATE_AND_WEEK+1];
        msFrame[frameTail++] = (dataBuff[DATE_AND_WEEK]&0xFF)<<5 | (dataBuff[DATE_AND_WEEK+2] & 0x1F);
        msFrame[frameTail++] = dataBuff[DATE_AND_WEEK+3];
      
        //电池工作时间
        if (dataBuff[BATTERY_WORK_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME];
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME + 1];
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME + 2];
        msFrame[frameTail++] = dataBuff[BATTERY_WORK_TIME + 3];
  
        //编程总次数
        if (dataBuff[PROGRAM_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[PROGRAM_TIMES];
        msFrame[frameTail++] = dataBuff[PROGRAM_TIMES + 1];
        msFrame[frameTail++] = dataBuff[PROGRAM_TIMES + 2];
  
        //最近一次编程发生时刻[数据格式1]
        if (dataBuff[LAST_PROGRAM_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+1];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+2];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+3];
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[LAST_PROGRAM_TIME+5];
        
        //电表清零总次数
        if (dataBuff[METER_CLEAR_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[METER_CLEAR_TIMES];
        msFrame[frameTail++] = dataBuff[METER_CLEAR_TIMES + 1];
        msFrame[frameTail++] = dataBuff[METER_CLEAR_TIMES + 2];
      
        //最近一次清零时间[数据格式1]
        if (dataBuff[LAST_METER_CLEAR_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+1];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+2];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+3];
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[LAST_METER_CLEAR_TIME+5];      
      
        //需量清零总次数
        if (dataBuff[UPDATA_REQ_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[UPDATA_REQ_TIME];
        msFrame[frameTail++] = dataBuff[UPDATA_REQ_TIME+1];
        msFrame[frameTail++] = dataBuff[UPDATA_REQ_TIME+2];
      
      	//最近一次清零时间[数据格式1](需量)
        if (dataBuff[LAST_UPDATA_REQ_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+1];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+2];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+3];
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[LAST_UPDATA_REQ_TIME+5];  
      
        //事件清零总次数
        if (dataBuff[EVENT_CLEAR_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_TIMES];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_TIMES+1];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_TIMES+2];
      
      	//最近一次清零时间[数据格式1](事件)
        if (dataBuff[EVENT_CLEAR_LAST_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+1];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+2];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+3];
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[EVENT_CLEAR_LAST_TIME+5];
        
        //校时清零总次数
        if (dataBuff[TIMING_TIMES] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TIMING_TIMES];
        msFrame[frameTail++] = dataBuff[TIMING_TIMES+1];
        msFrame[frameTail++] = dataBuff[TIMING_TIMES+2];
      
      	//最近一次清零时间[数据格式1](校时)
        if (dataBuff[TIMING_LAST_TIME] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+1];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+2];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+3];
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+4] & 0x1F;
        msFrame[frameTail++] = dataBuff[TIMING_LAST_TIME+5]; 
        
       	#ifdef NO_DATA_USE_PART_ACK_03
	       if (ifHasData == FALSE)
	       {
	         frameTail -= 64;
	       }
       	#endif     
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
       
         for(i=0;i<55;i++)
         {
         	 msFrame[frameTail++] = 0xee;
         }         
        #endif      	  
      }
 		}
 		da1 >>= 1;
 	}

  return frameTail;
}

/*******************************************************
函数名称:AFN0C028
功能描述:响应主站请求一类数据"电表运行状态字及其变位标志"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C028(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[512];
  INT8U     lastLastCopyPara[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
  INT8U     i;  
  BOOL      ifHasData;  
  DATE_TIME time;
  BOOL      buffHasData;
  INT16U    offset;
  INT8U     meterInfo[10];
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
  		
  		msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      msFrame[frameTail++] = 0x08;                                      //DT1
      msFrame[frameTail++] = 0x03;                                      //DT2

    	ifHasData = FALSE;
    	
			//查询测量点存储信息
  		queryMeterStoreInfo(pn, meterInfo);
      
      if (meterInfo[0]<7)
      {
    		 buffHasData = readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &time, 0);
    		 if (meterInfo[0]<4)
    		 {
    		   offset = METER_STATUS_WORD_S;
    		 }
    		 else
    		 {
    		 	 offset = METER_STATUS_WORD_T;
    		 }
      }
      else
      {
    		 buffHasData = readMeterData(dataBuff, pn, meterInfo[1], PARA_VARIABLE_DATA, &time, 0);
    		 
    		 //bug,2012-07-26,发现,原来offset未赋值
    		 offset = METER_STATUS_WORD;
      }
    	
    	//读取指定测量点的数据页
      if (buffHasData == TRUE)
      {
        //数据均可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
      	
      	//电表运行状态字变位标志位1~7
        time = queryCopyTime(pn);
        if (readMeterData(lastLastCopyPara , pn, LAST_LAST_REAL_DATA, PARA_VARIABLE_DATA, &time, 1) == TRUE)
        {      	
          meterRunWordChangeBit(&msFrame[frameTail],&dataBuff[METER_STATUS_WORD],&lastLastCopyPara[METER_STATUS_WORD]);
        }
        else
        {
      	  //上次抄表数据没有,置全0
      	  for(i=0;i<14;i++)
      	  {
      	  	msFrame[frameTail+i] = 0;
      	  }
        }
        frameTail += 14;
        
        //电表运行状态字1
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //电表运行状态字2
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //电表运行状态字3
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //电表运行状态字4
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //电表运行状态字5
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //电表运行状态字6
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
        //电表运行状态字7
        if (dataBuff[offset] != 0xEE)
        {
          ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[offset++];
        msFrame[frameTail++] = dataBuff[offset++];
        
      	#ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	          frameTail -= 37;
	        }
       	#endif     
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         for(i=0;i<28;i++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif      	  
      }
  	}
  	da1 >>= 1;
  }

  return frameTail;
}

/*******************************************************
函数名称:AFN0C029
功能描述:响应主站请求一类数据"当前铜损/铁损有功总电能示值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C029(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     result, i;	
	BOOL      ifHasData;	
	DATE_TIME time;
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);

 			msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      
      if(fn == 29)
      {
      	msFrame[frameTail++] = 0x10;                                      //DT1
      }
      else	//F30
      {
      	msFrame[frameTail++] = 0x20;                                      //DT1
      }
      msFrame[frameTail++] = 0x03;                                        //DT2
      
      ifHasData = FALSE;
      
      //读取制指定测量点、类型和时间的数据页
      if(fn == 29)
      {
      	result =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      }
      else	//F30
      {
      	result =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
      }
      
      if (result== TRUE)
      {
      	//终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
		    
		    //当前铜损有功总电能示值
		    if(dataBuff[COPPER_LOSS_TOTAL_OFFSET] != 0xEE)
		    {
		    	ifHasData = TRUE;
		    	msFrame[frameTail++] = 0x0;
		    }
		    else
		    {
		    	msFrame[frameTail++] = 0xEE;
		    }
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET];
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET + 1];
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET + 2];
		    msFrame[frameTail++] = dataBuff[COPPER_LOSS_TOTAL_OFFSET + 3];
		    
		    //当前铁损有功总电能示值
		    if(dataBuff[IRON_LOSS_TOTAL_OFFSET] != 0xEE)
		    {
		    	ifHasData = TRUE;
		    	msFrame[frameTail++] = 0x0;
		    }
		    else
		    {
		    	msFrame[frameTail++] = 0xEE;
		    }
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET];
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET + 1];
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET + 2];
		    msFrame[frameTail++] = dataBuff[IRON_LOSS_TOTAL_OFFSET + 3];
		    
		    #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	          frameTail -= 19;
	        }
       	#endif   
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
          msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
          msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
          msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
          msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
          msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
        	
        	for(i=0;i<10;i++)
        	{
        		msFrame[frameTail++] = 0xee;
        	}      
        #endif
      }
 		}
 		da1 >>= 1;
 	}

	return frameTail;
}

/*******************************************************
函数名称:AFN0C030
功能描述:响应主站请求一类数据"上一结算日铜损，铁损有功总电能示值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C030(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C029(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C031
功能描述:响应主站请求一类数据"当前A,B,C三相正/反向有功电能示值,
					组合无功1/2电能示值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C031(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     result, i;  
  BOOL      ifHasData;  
  DATE_TIME time;
	
	da1 = *pHandle++;
	da2 = *pHandle++;
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
  		
  		msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
      
      if(fn == 31)
      {
      	msFrame[frameTail++] = 0x40;                                      //DT1
      }
      else
      {
      	msFrame[frameTail++] = 0x80;                                      //DT1
      }
      msFrame[frameTail++] = 0x03;                                        //DT2
			
			ifHasData = FALSE;
			
      //读取制指定测量点、类型和时间的数据页
      if(fn == 31)
      {
      	result =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      }
      else		//F32
      {
      	result =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
      }
      
      if(result == TRUE)
      {
        //数据可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
        
        //当前A相正向有功能电能示值
        if(dataBuff[POSITIVE_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_A_OFFSET + 3];
        
        //当前A相反向有功能电能示值
        if(dataBuff[NEGTIVE_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
          msFrame[frameTail++] = 0;
        }
        else
        {
        	msFrame[frameTail++] = 0xee;
        }
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_A_OFFSET + 3];
        
        //当前A相组合无功1电能示值
        if(dataBuff[COMB1_NO_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_A_OFFSET + 3];
        
        //当前A相组合无功2电能示值
        if(dataBuff[COMB2_NO_WORK_A_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_A_OFFSET + 3];
        
        //当前B相正向有功能电能示值
        if(dataBuff[POSITIVE_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_B_OFFSET + 3];
        
        //当前B相反向有功能电能示值
        if(dataBuff[NEGTIVE_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_B_OFFSET + 3];
        
        //当前B相组合无功1电能示值
        if(dataBuff[COMB1_NO_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_B_OFFSET + 3];
        
        //当前B相组合无功2电能示值
        if(dataBuff[COMB2_NO_WORK_B_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_B_OFFSET + 3];
        
        //当前C相正向有功能电能示值
        if(dataBuff[POSITIVE_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[POSITIVE_WORK_C_OFFSET + 3];
        
        //当前C相反向有功能电能示值
        if(dataBuff[NEGTIVE_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        	msFrame[frameTail++] = 0x0;
        }
        else
        {
        	msFrame[frameTail++] = 0xEE;
        }
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[NEGTIVE_WORK_C_OFFSET + 3];
        
        //当前C相组合无功1电能示值
        if(dataBuff[COMB1_NO_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB1_NO_WORK_C_OFFSET + 3];
        
        //当前C相组合无功2电能示值
        if(dataBuff[COMB2_NO_WORK_C_OFFSET] != 0xEE)
        {
        	ifHasData = TRUE;
        }
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET + 1];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET + 2];
        msFrame[frameTail++] = dataBuff[COMB2_NO_WORK_C_OFFSET + 3];
        
        #ifdef NO_DATA_USE_PART_ACK_03
        	if(ifHasData == FALSE)
        	{
        		frameTail -= 63;
        	}
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
        	frameTail -= 4;
        #else
          msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
          msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
          msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
          msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
          msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
        	
        	for(i=0;i<54;i++)
        	{
        		msFrame[frameTail++] = 0xee;
        	}
        #endif
      }
  	}
  	da1 >>= 1;
  }

  return frameTail;
}

/*******************************************************
函数名称:AFN0C032
功能描述:响应主站请求一类数据"上一结算日A,B,C三相正/反向有功电能示值,
					组合无功1/2电能示值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C032(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C031(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C033
功能描述:响应主站请求一类数据"当前正向有／无功电能示值、
         一／四象限无功电能示值(总、费率1~M)命令（数据格式15，14，11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C033(INT16U frameTail,INT8U *pHandle, INT8U fn)
{	
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U    pn, tmpPn=0;
  INT8U     da1, da2;
  INT8U     tariff, tmpTariff;
  INT8U     pulsePnData = 0;
  BOOL      ifHasData; 
  BOOL      buffHasData;  
  INT16U    offSet;
  INT16U    j;  
  DATE_TIME time;
  INT8U     meterInfo[10];
	
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif  

  da1 = *pHandle++;
  da2 = *pHandle++;
    
  if(da1 == 0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
      
  		#ifdef PULSE_GATHER
	 	   //查看是否是脉冲采样的数据
	  	 if (fn==33)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	     //是脉冲量的测量点
	  	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	     {
	              //P.1先初始化缓存
                memset(dataBuff,0xee,LENGTH_OF_ENERGY_RECORD);
	
			 	  	 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
	          	  covertPulseData(j, dataBuff, NULL, NULL);
  		          
  		          //ly,2011-05-21,add
  		          time = timeHexToBcd(sysTime);

	   	  	 	    pulsePnData = 1;
	   	  	 	    
	   	  	 	    buffHasData = TRUE;
	   	  	 	    break;
			 	  	 }
			 	  }
	  	 }
	  	#endif
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
	        
      //fn
      if(fn == 33)
      {
      	 msFrame[frameTail++] = 0x01;    //DT1
      }
      else		//F37
      {
      	 msFrame[frameTail++] = 0x10;    //DT1
      }
      msFrame[frameTail++] = 0x4;
    	

      ifHasData = FALSE;
      
      if (pulsePnData==0)
      {
        //读取测量点配置的费率数
        tariff = numOfTariff(pn);
      	
      	queryMeterStoreInfo(pn, meterInfo);
      	if(fn == 33)
      	{
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6 || meterInfo[0]==8)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &time, 0);
      		}
      		else
      		{
      		  if (meterInfo[0]==1)
      		  {
      		    buffHasData =  readMeterData(dataBuff, pn, SINGLE_PHASE_PRESENT, ENERGY_DATA, &time, 0);
      		  }
      		  else
      		  {
      		    if (meterInfo[0]==2)
      		    {
      		      buffHasData =  readMeterData(dataBuff, pn, SINGLE_LOCAL_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		    }
      		    else
      		    {
      		      if (meterInfo[0]==3)
      		      {
      		        buffHasData =  readMeterData(dataBuff, pn, SINGLE_REMOTE_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		      }
      		      else
      		      {
      		        //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      		        buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &time, 0);
      		      }
      		    }
      		  }
      		}
      	}
      	else		//F37
      	{
      		buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
      	}
      }
      else
      {
      	tariff = periodTimeOfCharge[48];
      	
      	if (tariff>14)
      	{
      		tariff = 0;
      	}
      }
      
      //读取制指定测量点、类型和时间的数据页
      if (buffHasData==TRUE)
      {
        //数据均可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;

        //费率数
        msFrame[frameTail++] =  tariff;
  
        //当前正向有功电能示值(总、费率1～m)[数据格式14]
        offSet = POSITIVE_WORK_OFFSET;
        for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          //只要有一个数据就置数据存在标志为true，下同
          if ((dataBuff[offSet] != 0xEE) && (dataBuff[offSet]!=0xFF))
          {
          	 ifHasData = TRUE;
             if (pulsePnData==1)
             {
               msFrame[frameTail++] = dataBuff[offSet++];
             }
             else
             {
               msFrame[frameTail++] = 0x0;
             }
          }
          else
          {
            if (pulsePnData==1)
            {
            	 offSet++;
            }
	          msFrame[frameTail++] = 0xEE;	          
          }
          
          if (dataBuff[offSet]==0xFF)
          {
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          }
          else
          {
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
          }
          
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 tmpTariff++;
          }
        }
      
        if (fn==33 && (meterInfo[0]==8 || meterInfo[0]==0x1 || meterInfo[0]==0x2 || meterInfo[0]==0x3))
        {
        	 memset(&msFrame[frameTail],0xee,(tariff+1)*12);
        	 frameTail += (tariff+1)*12;
        }
        else
        {
          //当前正向无功电能示值(总、费率1～m)[数据格式11]
          offSet = POSITIVE_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if ((dataBuff[offSet] != 0xEE) && (dataBuff[offSet]!=0xFF))
            {
        	    ifHasData = TRUE;
            }
            
            if (pulsePnData==1)
            {
            	offSet++;
            }

            if (dataBuff[offSet]==0xFF)
            {
            	msFrame[frameTail++] = 0xee;
            	msFrame[frameTail++] = 0xee;
            	msFrame[frameTail++] = 0xee;
            	msFrame[frameTail++] = 0xee;
            }
            else
            {
              msFrame[frameTail++] = dataBuff[offSet++];
              msFrame[frameTail++] = dataBuff[offSet++];
              msFrame[frameTail++] = dataBuff[offSet++];
              msFrame[frameTail++] = dataBuff[offSet++];
            }
  
            //单费率的处理.费率1与总一样
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //当前一象限无功电能示值(总、费率1～m)[数据格式14]
          offSet = QUA1_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //单费率的处理.费率1与总一样
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //当前四象限无功电能示值(总、费率1～m)[数据格式11]
          offSet = QUA4_NO_WORK_OFFSET;
          for(tmpTariff = 0;tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //单费率的处理.费率1与总一样
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
      
  	      #ifdef NO_DATA_USE_PART_ACK_03
           if (ifHasData == FALSE)
           {
             frameTail -= (10 + (tariff+1)*17);
           }
          #endif
        }
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
          frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*17;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
  	}
  	da1 >>= 1;
  }

 	return frameTail;
}


/*******************************************************
函数名称:AFN0C034
功能描述:响应主站请求一类数据"(测量点)当前反向有／无功电能示值、
         二／三象限无功电能示值(总、费率1~M)"命令（数据格式15，14，11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C034(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U  dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U  da1, da2;
  INT8U  tariff, tmpTariff;
  INT8U  pulsePnData = 0;
  
  BOOL ifHasData;
  BOOL buffHasData;
  
  INT16U offSet;
  INT16U j;
  INT8U  meterInfo[10];
  
  DATE_TIME time;
	
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif
  
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);

 			#ifdef PULSE_GATHER
			 	//查看是否是脉冲采样的数据
			  if (fn==34)
			  {
			      for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  	  {
			  	     //是脉冲量的测量点
			  	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
			  	     {
			            //P.1先初始化缓存
                  memset(dataBuff,0xee,LENGTH_OF_ENERGY_RECORD);

				 	  	 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
			        	  covertPulseData(j, dataBuff, NULL, NULL);
  		            
  		            //ly,2011-05-21,add
  		            time = timeHexToBcd(sysTime);

			    	 	    pulsePnData = 1;
			   	  	 	    
			    	 	    buffHasData = TRUE;
			    	 	    break;
					  	 }
					  }
			  }
		  #endif
  

      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      
      if (fn == 34)
      {
        msFrame[frameTail++] = 0x02;     //DT1
      }
      else		//F38
      {
      	msFrame[frameTail++] = 0x20;     //DT1
      }
      msFrame[frameTail++] = 0x4;    //DT2
            	 
      if (pulsePnData==0)
      {
        //读取测量点配置的费率数
        tariff = numOfTariff(pn);

      	queryMeterStoreInfo(pn, meterInfo);
        if(fn == 34)
        {
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6 || meterInfo[0]==8)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &time, 0);
      		}
      		else
      		{
      		  if (meterInfo[0]==1)
      		  {
      		    buffHasData =  readMeterData(dataBuff, pn, SINGLE_PHASE_PRESENT, ENERGY_DATA, &time, 0);
      		  }
      		  else
      		  {
      		    if (meterInfo[0]==2)
      		    {
      		      buffHasData =  readMeterData(dataBuff, pn, SINGLE_LOCAL_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		    }
      		    else
      		    {
      		      if (meterInfo[0]==3)
      		      {
      		        buffHasData =  readMeterData(dataBuff, pn, SINGLE_REMOTE_CTRL_PRESENT, ENERGY_DATA, &time, 0);
      		      }
      		      else
      		      {
      		        //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, ENERGY_DATA, &time, 0);
      		        buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &time, 0);
      		      }
      		    }
      		  }
      		  printf("buffHasData=%d\n", buffHasData);
      		}
        }
        else	//F38
        {
        	buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &time, 0);
        }
      }
      else
      {
      	tariff = periodTimeOfCharge[48];
      	
      	if (tariff>14)
      	{
      		 tariff = 0;
      	}
      }
      
      //读取制指定测量点、类型和时间的数据页
      if (buffHasData==TRUE)
      {
        //数据均可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;

        //费率数
        msFrame[frameTail++] =  tariff;
  
        //当前反向有功电能示值(总、费率1～m)[数据格式14]
        if (meterInfo[0]<4)
        {
          offSet = NEGTIVE_WORK_OFFSET_S;
        }
        else
        {
          offSet = NEGTIVE_WORK_OFFSET;
        }
        for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          //只要有一个数据就置数据存在标志为true，下同
          if ((dataBuff[offSet] != 0xEE) && (dataBuff[offSet] != 0xFF))
          {
          	ifHasData = TRUE;
          	if (pulsePnData==1)
          	{
              msFrame[frameTail++] = dataBuff[offSet++];
          	}
          	else
          	{
              msFrame[frameTail++] = 0x0;
            }
          }
          else
          {
          	msFrame[frameTail++] = 0xEE;
            
            if (pulsePnData==1)
            {
          	  offSet++;
          	}
          }
          
          if  (dataBuff[offSet] == 0xFF)
          {
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	msFrame[frameTail++] = 0xee;
          	
          	offSet += 4;
          }
          else
          {
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
          }

          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 msFrame[frameTail++] = msFrame[frameTail-5];
          	 tmpTariff++;
          }
        }
      
        if (fn==34 && (meterInfo[0]==8 || meterInfo[0]==0x1 || meterInfo[0]==0x2 || meterInfo[0]==0x3))
        {
        	 memset(&msFrame[frameTail],0xee,(tariff+1)*12);
        	 frameTail += (tariff+1)*12;
        }
        else
        {
          //当前反向无功电能示值(总、费率1～m)[数据格式11]
          offSet = NEGTIVE_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            
          	if (pulsePnData==1)
            {
            	offSet++;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //单费率的处理.费率1与总一样
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //当前二象限无功电能示值(总、费率1～m)[数据格式14]
          offSet = QUA2_NO_WORK_OFFSET;
          for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
  
            //单费率的处理.费率1与总一样
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
    
          //当前三象限无功电能示值(总、费率1～m)[数据格式11]
          offSet = QUA3_NO_WORK_OFFSET;
          for(tmpTariff = 0;tmpTariff <= tariff;tmpTariff++)
          {
            if (dataBuff[offSet] != 0xEE)
            {
        	    ifHasData = TRUE;
            }
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            msFrame[frameTail++] = dataBuff[offSet++];
            
            //单费率的处理.费率1与总一样
            if (tariff==1)
            {
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 msFrame[frameTail++] = msFrame[frameTail-4];
            	 tmpTariff++;
            }
          }
      
          #ifdef NO_DATA_USE_PART_ACK_03
           if (ifHasData == FALSE)
           {
             frameTail -= (10 + (tariff+1)*17);
           }
          #endif
        }
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03 
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*17;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
 		}
 		da1 >>= 1;
 	}

 	return frameTail;
}

/*******************************************************
函数名称:AFN0C035
功能描述:响应主站请求一类数据"(测量点)当月正向有／无功最大需量
         及发生时间(总、费率1～M)"命令（数据格式15，23，17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C035(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U dataBuff[LENGTH_OF_REQ_RECORD];
  INT16U pn, tmpPn = 0;
  INT8U da1, da2;
  INT8U tariff, tmpTariff;
  INT8U pulsePnData = 0;
  
  BOOL ifHasData; 
  BOOL buffHasData;    
  
  INT16U offSet;
  INT16U j;
  INT8U  meterInfo[10];
  
  DATE_TIME  time;
  
  #ifdef     PULSE_GATHER
    INT16U   counti;
  #endif
	
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x01) == 0x01)
 		{
 			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
 			
 			#ifdef PULSE_GATHER
	 	   //查看是否是脉冲采样的数据
	  	 if (fn==35)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	     //是脉冲量的测量点
	  	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	     {
	              //P.1先初始化缓存
                memset(dataBuff,0xee,LENGTH_OF_REQ_RECORD);
	
			 	  	 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
	          	  covertPulseData(j, NULL,dataBuff,NULL);
  		          
  		          //ly,2011-05-21,add
  		          time = timeHexToBcd(sysTime);

	   	  	 	    pulsePnData = 1;
	   	  	 	    
	   	  	 	    buffHasData = TRUE;
	   	  	 	    break;
			 	  	 }
			 	  }
	  	 }
	  	#endif
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  //DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                    //DA2
	        
      if (fn == 35)
      {
        msFrame[frameTail++] = 0x04;    //DT1
      }
      else		//F39
      {
      	msFrame[frameTail++] = 0x40;    //DT1
      }
      msFrame[frameTail++] = 0x4;     //DT2
    
      //读取测量点配置的费率数
      tariff = numOfTariff(pn);
      	
      ifHasData = FALSE;
      
      if (pulsePnData==0)
      {
      	if(fn == 35)
      	{
      		queryMeterStoreInfo(pn, meterInfo);
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], REQ_REQTIME_DATA, &time, 0);
      		}
      		else
      		{
      		  //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, REQ_REQTIME_DATA, &time, 0);
      		  buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, REQ_REQTIME_DATA, &time, 0);
      		}
      	}
      	else		//F39
      	{
      		buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, REQ_REQTIME_LASTMONTH, &time, 0);
      	}
      }
      
      if (buffHasData==TRUE)
      {
        //数据均可从原始数据记录中读取
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
      
      	//费率数
        msFrame[frameTail++] = tariff;
        
        //当月正向有功最大需量
        offSet = REQ_POSITIVE_WORK_OFFSET;
        for(tmpTariff=0; tmpTariff<=tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];

          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
        
        //当月正向有功最大需量发生时间(总、费率1～M)[数据格式17]
        offSet = REQ_TIME_P_WORK_OFFSET;
        for(tmpTariff=0; tmpTariff<=tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
        
        //当月正向无功最大需量
        offSet = REQ_POSITIVE_NO_WORK_OFFSET;
        for(tmpTariff=0;tmpTariff<=tariff;tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
        
        //当月正向无功最大需量发生时间(总、费率1～M)[数据格式17]
        offSet = REQ_TIME_P_NO_WORK_OFFSET;
        for(tmpTariff=0;tmpTariff<=tariff;tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff+1)*14);
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*14;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
 		}
 		da1 >>= 1;
 	}

	return frameTail;
}

/*******************************************************
函数名称:AFN0C036
功能描述:响应主站请求一类数据"(测量点)当月反向有／无功最大需量
         及发生时间(总、费率1～M)"命令（数据格式15，23，17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C036(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U dataBuff[LENGTH_OF_REQ_RECORD];
  INT16U pn, tmpPn = 0;
  INT8U da1, da2;
  INT8U tariff, tmpTariff;
  INT8U pulsePnData = 0;
  
  BOOL ifHasData;
  BOOL buffHasData;
  
  INT16U offSet;
  INT16U j;
  INT8U  meterInfo[10];
  
  DATE_TIME time;
  
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif
	
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
			
			#ifdef PULSE_GATHER
	 	   //查看是否是脉冲采样的数据
	  	 if (fn==36)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	     //是脉冲量的测量点
	  	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	     {
	              //P.1先初始化缓存
                memset(dataBuff,0xee,LENGTH_OF_REQ_RECORD);
	
			 	  	 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
	          	  covertPulseData(j, NULL,dataBuff,NULL);

  		          //ly,2011-05-21,add
  		          time = timeHexToBcd(sysTime);

	   	  	 	    pulsePnData = 1;
	   	  	 	    
	   	  	 	    buffHasData = TRUE;
	   	  	 	    break;
			 	  	 }
			 	  }
	  	 }
	  	#endif
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1)); 		 //DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                       //DA2

      if (fn == 36)
      {
        msFrame[frameTail++] = 0x08;    //DT1
      }
      else		//F40
      {
      	msFrame[frameTail++] = 0x80;    //DT1
      }
      msFrame[frameTail++] = 0x4;    //DT2

      //读取测量点配置的费率数
      tariff = numOfTariff(pn);
      
      if (pulsePnData == 0)
      {
      	if(fn == 36)
      	{
      		queryMeterStoreInfo(pn, meterInfo);
      		if (meterInfo[0]==4 || meterInfo[0]==5 || meterInfo[0]==6)
      		{
      		  buffHasData =  readMeterData(dataBuff, pn, meterInfo[1], REQ_REQTIME_DATA, &time, 0);
      		}
      		else
      		{
      		  //buffHasData =  readMeterData(dataBuff, pn, PRESENT_DATA, REQ_REQTIME_DATA, &time, 0);
      		  buffHasData =  readMeterData(dataBuff, pn, LAST_TODAY, REQ_REQTIME_DATA, &time, 0);
      		}
      	}
      	else		//F40
      	{
      		buffHasData =  readMeterData(dataBuff, pn, LAST_MONTH_DATA, REQ_REQTIME_LASTMONTH, &time, 0);
      	}
      }
      
      if (buffHasData==TRUE)
      { 
        //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
    
        //费率数
        msFrame[frameTail++] =  tariff;

     	  //当月反向有功总最大需量
        offSet = REQ_NEGTIVE_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];

          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
       
        //当月反向有功总最大需量发生时间
        offSet = REQ_TIME_N_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
       
        //当月反向无功总最大需量
        offSet = REQ_NEGTIVE_NO_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 msFrame[frameTail++] = msFrame[frameTail-3];
          	 tmpTariff++;
          }
        }
       
        //当月反向无功总最大需量发生时间
        offSet = REQ_TIME_N_NO_WORK_OFFSET;
        for (tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
          if (dataBuff[offSet] != 0xEE)
          {
            ifHasData = TRUE;
          }
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          msFrame[frameTail++] = dataBuff[offSet++];
          
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
            offSet++;
          }
        }
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff+1)*14);
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
         msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
         msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
         msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
         msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*14;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C037
功能描述:响应主站请求一类数据"上月(上一结算日)正向有/无功电能示值
					，一/四象限无功电能示值(总，费率1~M)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C037(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C033(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C038
功能描述:响应主站请求一类数据"上月(上一结算日)反向有/无功电能示值
					，二/三象限无功电能示值(总，费率1~M)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C038(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C034(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C039
功能描述:响应主站请求一类数据"上月(上一结算日)正向有/无功最大需量
					及发生时间(总，费率1~M)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C039(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C035(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C040
功能描述:响应主站请求一类数据"上月(上一结算日)反向有/无功最大需量
					及发生时间(总，费率1~M)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C040(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C036(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C041
功能描述:响应主站请求一类数据"当日正向有功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C041(INT16U frameTail, INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     tariff, tmpTariff;
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    offSet;
  INT16U    j;
  DATE_TIME time;
  
  #ifdef PULSE_GATHER
   INT8U    pulsePnData = 0;
  #endif
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		 return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		#ifdef PULSE_GATHER
	 	   //查看是否是脉冲采样的数据
       for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
 	 	   {
 	 	     //是脉冲量的测量点
 	 	     if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
 	 	     {
  	  	 	  pulsePnData = 1;
  	  	 	  break;
	 	  	 }
	 	   }
	  	#endif
	  	
  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 41:
      		msFrame[frameTail++] = 0x01;			//DT1
      		offSet = DAY_P_WORK_OFFSET;				//当日正向有功电能量(总，费率1~M)
      		break;
      	case 42:
      		msFrame[frameTail++] = 0x02;			//DT1
      		offSet = DAY_P_NO_WORK_OFFSET;		//当日正向无功电能量(总，费率1~M)
      		break;
      	case 43:
      		msFrame[frameTail++] = 0x04;			//DT1
      		offSet = DAY_N_WORK_OFFSET;		    //当日反向有功电能量(总，费率1~M)
      		break;
      	case 44:
      		msFrame[frameTail++] = 0x08;			//DT1
      		offSet = DAY_N_NO_WORK_OFFSET;		//当日反向无功电能量(总，费率1~M)
      		break;
      	case 45:
      		msFrame[frameTail++] = 0x10;			//DT1
      		offSet = MONTH_P_WORK_OFFSET;			//当月正向有功电能量(总，费率1~M)
      		break;
      	case 46:
      		msFrame[frameTail++] = 0x20;			//DT1
      		offSet = MONTH_P_NO_WORK_OFFSET;	//当月正向无功电能量(总，费率1~M)
      		break;
      	case 47:
      		msFrame[frameTail++] = 0x40;			//DT1
      		offSet = MONTH_N_WORK_OFFSET;	    //当月反向有功电能量(总，费率1~M)
      		break;
      	case 48:
      		msFrame[frameTail++] = 0x80;			//DT1
      		offSet = MONTH_N_NO_WORK_OFFSET;	//当月反向无功电能量(总，费率1~M)
      		break;
      }
			msFrame[frameTail++] = 0x05;					//DT2
			
			//取得费率数
			tariff = numOfTariff(pn);
			
			ifHasData = FALSE;
      
      #ifdef PULSE_GATHER
  	   if (pulsePnData==1)
  	   {
         buffHasData =  readMeterData(dataBuff, pn, LAST_REAL_BALANCE, REAL_BALANCE_POWER_DATA, &time, 0);
       }
       else
       {  	   	 
      #endif
      
      buffHasData =  readMeterData(dataBuff, pn, REAL_BALANCE, REAL_BALANCE_POWER_DATA, &time, 0);
      
      #ifdef PULSE_GATHER
       }
      #endif
      
      if (buffHasData==TRUE)
      {
        //费率数
        msFrame[frameTail++] = tariff;
        
        for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
        {
        	if(dataBuff[offSet + 1] != 0xEE)
        	{
        		ifHasData = TRUE;
        	}
        	offSet++;
        	msFrame[frameTail++] = dataBuff[offSet++];
        	msFrame[frameTail++] = dataBuff[offSet++];
        	msFrame[frameTail++] = dataBuff[offSet++];
        	msFrame[frameTail++] = dataBuff[offSet++];
        	
          //单费率的处理.费率1与总一样
          if (tariff==1)
          {
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 msFrame[frameTail++] = msFrame[frameTail-4];
          	 tmpTariff++;
          }
          else
          {
        	   offSet += 2;
        	}
        }
        
        #ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff+1)*4);
	        }
        #endif
      }
      else
      {
        #ifdef NO_DATA_USE_PART_ACK_03
         frameTail -= 4;
        #else
         msFrame[frameTail++] =  tariff;
         for(j=0;j<(tariff+1)*4;j++)
         {
         	  msFrame[frameTail++] = 0xee;
         }
        #endif
      }
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C042
功能描述:响应主站请求一类数据"当日正向无功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C042(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C043
功能描述:响应主站请求一类数据"当日反向有功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C043(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C044
功能描述:响应主站请求一类数据"当日反向无功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C044(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C045
功能描述:响应主站请求一类数据"当月正向有功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C045(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C046
功能描述:响应主站请求一类数据"当月正向无功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C046(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C047
功能描述:响应主站请求一类数据"当月反向有功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C047(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C048
功能描述:响应主站请求一类数据"当月反向无功电能量(总，费率1~M)"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C048(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C041(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C049
功能描述:响应主站请求一类数据"当前电压、电流相位角"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C049(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG  meterConfig;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT8U     tmpPn, pn;
  INT8U     i, j;
  INT8U     da1, da2;
  INT8U     pulsePnData = 0;  
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    tmpTail,counti;  
  DATE_TIME time;
	
  tmpPn = 0;
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);

	  	buffHasData = FALSE;
	  	
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
	      if (meterConfig.protocol==AC_SAMPLE)
	      {
	     	  if (ifHasAcModule==TRUE)
	     	  {
		        //A.1先初始化缓存
		        for (counti = 0; counti < LENGTH_OF_PARA_RECORD;counti++)
		        {
		          dataBuff[counti] = 0xEE;
		        }
		   
		       	//A.2将交流采样数据填入dataBuff中
		       	covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
		      	       	  
		     	  buffHasData = TRUE;
	   	    }
	      }
	  	  else
	  	  {
	  	    buffHasData =  readMeterData(dataBuff, pn , PRESENT_DATA, PARA_VARIABLE_DATA, &time, 0);
	  	  }
	  	}
		    	
		  msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
		  msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
		  msFrame[frameTail++] = 0x1;                                       //DT1
		  msFrame[frameTail++] = 0x6;                                       //DT2
		
		  ifHasData = FALSE;
		  if (buffHasData == TRUE)
		  {
		     //A相电压相位角
		     if (dataBuff[PHASE_ANGLE_V_A] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_A];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_A+1];

		     //B相电压相位角
		     if (dataBuff[PHASE_ANGLE_V_B] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_B];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_B+1];
		       
		     //C相电压相位角
		     if (dataBuff[PHASE_ANGLE_V_C] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_C];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_V_C+1];
		     
		     //A相电流相位角
		     if (dataBuff[PHASE_ANGLE_C_A] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_A];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_A+1];

		     //B相电流相位角
		     if (dataBuff[PHASE_ANGLE_C_B] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_B];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_B+1];
		       
		     //C相电流相位角
		     if (dataBuff[PHASE_ANGLE_C_C] != 0xEE)
		     {
		     	 ifHasData = TRUE;
		     }
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_C];
		     msFrame[frameTail++] = dataBuff[PHASE_ANGLE_C_C+1];
			   
			   #ifdef DKY_SUBMISSION
			    //2010-12-17,兰州测试发现没有抄表相角无数,所以先填00测试通过回公司再抄相角
			    if (ifHasData==FALSE)
			    {
			      frameTail-=12;
			     
			      for(j=0;j<12;j++)
			      {
			      	msFrame[frameTail++] = 0x00;
			      }
			    }
			   #endif
		  }
		  else
		  {
		    #ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    for(j=0;j<12;j++)
			    {
			     #ifdef DKY_SUBMISSION
			  	  //2010-12-17,兰州测试发现相角无数,所以先填00测试通过回公司再抄相角
			  	  msFrame[frameTail++] = 0x00;
			     #else
			  	  msFrame[frameTail++] = 0xee;
			  	 #endif
			    }
		    #endif
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0C057
功能描述:响应主站请求一类数据"当前A、B、C三相电压、电流2~N次谐波有效值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C057(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}
/*******************************************************
函数名称:AFN0C058
功能描述:响应主站请求一类数据"当前A、B、C三相电压、电流2~N次谐波含有率"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C058(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C065
功能描述:响应主站请求一类数据"当前电容器投切状态"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C065(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C066
功能描述:响应主站请求一类数据"当前电容器累计投入时间和次数"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C066(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C067
功能描述:响应主站请求一类数据"当日、月电容器累计补偿的无功电能量"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C067(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C073
功能描述:响应主站请求一类数据"直流模拟量实时数据"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C073(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	#ifdef PLUG_IN_CARRIER_MODULE
   //数据单元标识
   msFrame[frameTail++] = 0x01;       //DA1,Pn1
   msFrame[frameTail++] = 0x01;       //DA2
   msFrame[frameTail++] = 0x01;       //DT1
   msFrame[frameTail++] = 0x09;       //DT2
   
   //
   msFrame[frameTail++] = hexToBcd(adcData);
   msFrame[frameTail++] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
   
	#endif
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C081
功能描述:响应主站请求一类数据"小时冻结总加有功功率"命令（数据格式2）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C081(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
  INT16U pn, tmpPn = 0;
  INT8U da1, da2;
  INT8U density, interval, tmpMinute;
  
  BOOL ifHasData;
  BOOL buffHasData;
  
  INT16U offSet;
  INT16U i;
  
  DATE_TIME time, readTime;
  
  #ifdef PULSE_GATHER
    INT16U counti;
  #endif
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			//判断总加组号
			if(pn > 8)
			{
				return frameTail;
			}
    
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 81:
      		msFrame[frameTail++] = 0x01;   															//DT1
      		offSet = GP_WORK_POWER;
      		break;
      	case 82:
      		msFrame[frameTail++] = 0x02;   															//DT1
      		offSet = GP_NO_WORK_POWER;
      		break;
      	case 83:
      		msFrame[frameTail++] = 0x04;   															//DT1
      		offSet = GP_DAY_WORK;
      		break;
      	case 84:
      		msFrame[frameTail++] = 0x08;   															//DT1
      		offSet = GP_DAY_NO_WORK;
      		break;
      }
      msFrame[frameTail++] = 0xA;    																	//DT2
			
			//上一整点时间
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//小时冻结类数据时标Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//小时时间
			density = 0x1;
			msFrame[frameTail++] = density;			//冻结密度
			
			
			switch(density)
			{
				case 0:		//不冻结
					interval = 61;
					break;
				case 1:		//冻结间隔时间15分
					interval = 15;
					break;
				case 2:		//冻结间隔时间30分
					interval = 30;
					break;
				case 3:		//冻结间隔时间60分
					interval = 60;
					break;
				case 254:	//冻结间隔时间5分
					interval = 5;
					break;
				case 255:	//冻结间隔时间1分
					interval = 1;
					break;
			}
			
			//无效数据的判断
			if(interval > 60)
			{
				return frameTail -= 6;
			}

			time = backTime(time, 0, 0, 0, interval, 0);

			//查询数据
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = interval * (i + 1) / 10 << 4 | interval * (i + 1) % 10;
				tmpMinute = hexToBcd(interval);
				
				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, GROUP_REAL_BALANCE, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
					if(fn == 81 || fn == 82)
					{
						msFrame[frameTail++] = dataBuff[offSet+1];
	          msFrame[frameTail++] = ((dataBuff[offSet]&0x7)<<5) | (dataBuff[offSet]&0x10)
	                                | (dataBuff[offSet+2]&0xf);
          }
          
          if(fn == 83 || fn == 84)
          {
            msFrame[frameTail++] = dataBuff[offSet+3];
            msFrame[frameTail++] = dataBuff[offSet+4];
            msFrame[frameTail++] = dataBuff[offSet+5];
            msFrame[frameTail++] = ((dataBuff[offSet]&0x1)<<6) | (dataBuff[offSet]&0x10)
                                 | (dataBuff[offSet+6]&0xf);
          }
				}
				else
				{
					if(fn == 81 || fn == 82)
					{
						msFrame[frameTail++] = 0xEE;
	          msFrame[frameTail++] = 0xEE;
          }
          
          if(fn == 83 || fn == 84)
          {
            msFrame[frameTail++] = 0xEE;
            msFrame[frameTail++] = 0xEE;
            msFrame[frameTail++] = 0xEE;
            msFrame[frameTail++] = 0xEE;
          }
				}
				
				//每个冻结时刻冻结的数据总是在该时刻之前采集的
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C082
功能描述:响应主站请求一类数据"小时冻结总加无功功率"命令（数据格式2）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C082(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C081(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C083
功能描述:响应主站请求一类数据"小时冻结总加有功总电能量"命令（数据格式3）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C083(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C081(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C084
功能描述:响应主站请求一类数据"小时冻结总加无功总电能量"命令（数据格式3）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C084(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C081(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C089
功能描述:响应主站请求一类数据"小时冻结有功功率"命令（数据格式9,7,25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C089(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, interval, tmpMinute;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet;
  INT16U    i;
  
  DATE_TIME time, readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 89:		//小时冻结有功功率
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_INSTANT_WORK;
      		break;
      		
      	case 90:		//小时冻结A相有功功率
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_A_WORK;
      		break;
      		
      	case 91:		//小时冻结B相有功功率
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_B_WORK;
      		break;
      		
      	case 92:		//小时冻结C相有功功率
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_C_WORK;
      		break;
      		
      	case 93:		//小时冻结无功功率
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_INSTANT_NO_WORK;
      		break;
      		
      	case 94:		//小时冻结A相无功功率
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_A_NO_WORK;
      		break;
      		
      	case 95:		//小时冻结B相无功功率
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_B_NO_WORK;
      		break;
      		
      	case 96:		//小时冻结C相无功功率
      		msFrame[frameTail++] = 0x80;   															//DT1
      		msFrame[frameTail++] = 0xB;    															//DT2
      		offSet = POWER_PHASE_C_NO_WORK;
      		break;
      		
      	case 97:		//小时冻结A相电压
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = VOLTAGE_PHASE_A;
      		break;
      		
      	case 98:		//小时冻结B相电压
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = VOLTAGE_PHASE_B;
      		break;
      		
      	case 99:		//小时冻结C相电压
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = VOLTAGE_PHASE_C;
      		break;
      		
      	case 100:		//小时冻结A相电流
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = CURRENT_PHASE_A;
      		break;
      		
      	case 101:		//小时冻结B相电流
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = CURRENT_PHASE_B;
      		break;
      		
      	case 102:		//小时冻结C相电流
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = CURRENT_PHASE_C;
      		break;
      		
      	case 103:		//小时冻结零序电流
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0xC;    															//DT2
      		offSet = ZERO_SERIAL_CURRENT;
      		break;
      		
      	case 113:		//小时冻结总功率因数
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = TOTAL_POWER_FACTOR;
      		break;
      		
      	case 114:		//小时冻结A相功率因数
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = FACTOR_PHASE_A;
      		break;
      		
      	case 115:		//小时冻结B相功率因数
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = FACTOR_PHASE_B;
      		break;
      		
      	case 116:		//小时冻结C相功率因数
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0xE;    															//DT2
      		offSet = FACTOR_PHASE_C;
      		break;
      }
      
			//上一整点时间
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//小时冻结类数据时标Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//小时时间
			density = 0x1;
			msFrame[frameTail++] = density;			//冻结密度
			
			switch(density)
			{
				case 0:		//不冻结
					interval = 61;
					break;
					
				case 1:		//冻结间隔时间15分
					interval = 15;
					break;
				case 2:		//冻结间隔时间30分
					interval = 30;
					break;
				case 3:		//冻结间隔时间60分
					interval = 60;
					break;
				case 254:	//冻结间隔时间5分
					interval = 5;
					break;
				case 255:	//冻结间隔时间1分
					interval = 1;
					break;
			}
			
			//无效数据的判断
			if(interval > 60)
			{
				return frameTail -= 6;
			}
			
			time = backTime(time, 0, 0, 0, interval, 0);

			//查询数据
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(interval+i);
				tmpMinute = hexToBcd(interval);
				
				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
					if((fn >= 89 && fn <= 96) || (fn >= 100 && fn <= 103))	      //数据格式9,25
					{
						msFrame[frameTail++] = dataBuff[offSet];
	          msFrame[frameTail++] = dataBuff[offSet+1];
	          msFrame[frameTail++] = dataBuff[offSet+2];
					}
					else if((fn >= 97 && fn <= 99) || (fn >= 113 && fn <= 116))		//数据格式7
	        {
	        	msFrame[frameTail++] = dataBuff[offSet];
		     		msFrame[frameTail++] = dataBuff[offSet+1];
	        }
				}
				else
				{
					if((fn >= 89 && fn <= 96) || (fn >= 100 && fn <= 103))
					{
						msFrame[frameTail++] = 0xEE;
						msFrame[frameTail++] = 0xEE;
						msFrame[frameTail++] = 0xEE;
					}
					else if((fn >= 97 && fn <= 99) || (fn >= 113 && fn <= 116))
	        {
	        	msFrame[frameTail++] = 0xEE;
		     		msFrame[frameTail++] = 0xEE;
	        }
				}
				
				//每个冻结时刻冻结的数据总是在该时刻之前采集的
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C090
功能描述:响应主站请求一类数据"小时冻结A相有功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C090(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C091
功能描述:响应主站请求一类数据"小时冻结B相有功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C091(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C092
功能描述:响应主站请求一类数据"小时冻结C相有功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C092(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C093
功能描述:响应主站请求一类数据"小时冻结无功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C093(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C094
功能描述:响应主站请求一类数据"小时冻结A相无功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C094(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C095
功能描述:响应主站请求一类数据"小时冻结B相无功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C095(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C096
功能描述:响应主站请求一类数据"小时冻结C相无功功率"命令（数据格式9）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C096(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C097
功能描述:响应主站请求一类数据"小时冻结A相电压"命令（数据格式7）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C097(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C098
功能描述:响应主站请求一类数据"小时冻结B相电压"命令（数据格式7）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C098(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C099
功能描述:响应主站请求一类数据"小时冻结C相电压"命令（数据格式7）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C099(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C100
功能描述:响应主站请求一类数据"小时冻结A相电流"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C100(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C101
功能描述:响应主站请求一类数据"小时冻结B相电流"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C101(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C102
功能描述:响应主站请求一类数据"小时冻结C相电流"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C102(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C103
功能描述:响应主站请求一类数据"小时冻结零序电流"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C103(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C105
功能描述:响应主站请求一类数据"小时冻结正向有功总电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C105(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, interval, tmpMinute;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet;
  INT16U    i;
  
  DATE_TIME time,readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 105:		//小时冻结正向有功总电能量
      		msFrame[frameTail++] = 0x01;   															//DT1
      		offSet = DAY_P_WORK_OFFSET;
      		break;
      		
      	case 106:		//小时冻结正向无功总电能量
      		msFrame[frameTail++] = 0x02;   															//DT1
      		offSet = DAY_P_NO_WORK_OFFSET;
      		break;
      		
      	case 107:		//小时冻结反向有功总电能量
      		msFrame[frameTail++] = 0x04;   															//DT1
      		offSet = DAY_N_WORK_OFFSET;
      		break;
      		
      	case 108:		//小时冻结反向无功总电能量
      		msFrame[frameTail++] = 0x08;   															//DT1
      		offSet = DAY_N_NO_WORK_OFFSET;
      		break;
      }
      msFrame[frameTail++] = 0xD;    																	//DT2
      
			//上一整点时间
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//小时冻结类数据时标Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//小时时间
			density = 0x1;
			msFrame[frameTail++] = density;			//冻结密度
			
			switch(density)
			{
				case 0:		//不冻结
					interval = 61;
					break;
				case 1:		//冻结间隔时间15分
					interval = 15;
					break;
				case 2:		//冻结间隔时间30分
					interval = 30;
					break;
				case 3:		//冻结间隔时间60分
					interval = 60;
					break;
				case 254:	//冻结间隔时间5分
					interval = 5;
					break;
				case 255:	//冻结间隔时间1分
					interval = 1;
					break;
			}
			
			//无效数据的判断
			if(interval > 60)
			{
				return frameTail -= 6;
			}

			time = backTime(time, 0, 0, 0, interval, 0);

			//查询数据
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(interval+i);
				tmpMinute = hexToBcd(interval);

				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
					msFrame[frameTail++] = dataBuff[offSet + 1];
	        msFrame[frameTail++] = dataBuff[offSet + 2];
	        msFrame[frameTail++] = dataBuff[offSet + 3];
	        msFrame[frameTail++] = dataBuff[offSet + 4];
				}
				else
				{
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
				}
				
				//每个冻结时刻冻结的数据总是在该时刻之前采集的
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C106
功能描述:响应主站请求一类数据"小时冻结正向无功总电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C106(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C105(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C107
功能描述:响应主站请求一类数据"小时冻结反向有功总电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C107(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C105(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C108
功能描述:响应主站请求一类数据"小时冻结反向无功总电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C108(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C105(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C109
功能描述:响应主站请求一类数据"小时冻结正向有功总电能示值"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C109(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, interval, tmpMinute;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet;
  INT16U    i;
  
  DATE_TIME time, readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 109:		//小时冻结正向有功总电能示值
      		msFrame[frameTail++] = 0x10;   															//DT1
      		offSet = POSITIVE_WORK_OFFSET;
      		break;
      	case 110:		//小时冻结正向无功总电能示值
      		msFrame[frameTail++] = 0x20;   															//DT1
      		offSet = POSITIVE_NO_WORK_OFFSET;
      		break;
      	case 111:		//小时冻结反向有功总电能示值
      		msFrame[frameTail++] = 0x40;   															//DT1
      		offSet = NEGTIVE_WORK_OFFSET;
      		break;
      	case 112:		//小时冻结反向无功总电能示值
      		msFrame[frameTail++] = 0x80;   															//DT1
      		offSet = NEGTIVE_NO_WORK_OFFSET;
      		break;
      }
      msFrame[frameTail++] = 0xD;    																	//DT2
      
			//上一整点时间
			time = backTime(sysTime, 0, 0, 1, 0, 0);
			time.minute = 0;
			time.second = 0;
			
			//小时冻结类数据时标Td_h
			msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//小时时间
			density = 0x1;
			msFrame[frameTail++] = density;			//冻结密度
			
			switch(density)
			{
				case 0:		//不冻结
					interval = 61;
					break;
				case 1:		//冻结间隔时间15分
					interval = 15;
					break;
				case 2:		//冻结间隔时间30分
					interval = 30;
					break;
				case 3:		//冻结间隔时间60分
					interval = 60;
					break;
				case 254:	//冻结间隔时间5分
					interval = 5;
					break;
				case 255:	//冻结间隔时间1分
					interval = 1;
					break;
			}
			
			//无效数据的判断
			if(interval > 60)
			{
				return frameTail -= 6;
			}

			time = backTime(time, 0, 0, 0, interval, 0);

			//查询数据
			for(i = 0; i < 60; i += interval)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(interval+i);
				tmpMinute = hexToBcd(interval);
				
				readTime = timeHexToBcd(time);
				buffHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, ENERGY_DATA, &readTime, tmpMinute);
				
				if(buffHasData == TRUE)
				{
	        msFrame[frameTail++] = dataBuff[offSet];
					msFrame[frameTail++] = dataBuff[offSet + 1];
	        msFrame[frameTail++] = dataBuff[offSet + 2];
	        msFrame[frameTail++] = dataBuff[offSet + 3];
				}
				else
				{
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
					msFrame[frameTail++] = 0xEE;
				}
				
				//每个冻结时刻冻结的数据总是在该时刻之前采集的
        time = nextTime(time, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C110
功能描述:响应主站请求一类数据"小时冻结正向无功总电能示值"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C110(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C109(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C111
功能描述:响应主站请求一类数据"小时冻结反向有功总电能示值"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C111(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C109(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C112
功能描述:响应主站请求一类数据"小时冻结反向无功总电能示值"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C112(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C109(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C113
功能描述:响应主站请求一类数据"小时冻结总功率因数"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C113(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C114
功能描述:响应主站请求一类数据"小时冻结A相功率因数"命令（数据格式5）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C114(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C115
功能描述:响应主站请求一类数据"小时冻结B相功率因数"命令（数据格式5）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C115(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C116
功能描述:响应主站请求一类数据"小时冻结C相功率因数"命令（数据格式5）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C116(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C089(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C121
功能描述:响应主站请求一类数据"小时冻结直流模拟量"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C121(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
 	#ifdef PLUG_IN_CARRIER_MODULE 
 	 INT8U     dataBuff[5];
   INT16U    pn, tmpPn = 0;
   INT8U     da1, da2;
   INT8U     density, interval, tmpMinute;
   
   BOOL      ifHasData;
   BOOL      buffHasData;
   
   INT16U    i;
   
   DATE_TIME time, readTime;
 	
   da1 = *pHandle;
   da2 = *(pHandle+1);
 	
 	 if(da1 == 0x0)
 	 {
 		 return frameTail;
 	 }
 	
 	 while(tmpPn < 9)
 	 {
 		 tmpPn++;
 		 if((da1 & 0x01) == 0x01)
 		 {
 			 pn = tmpPn + (da2 - 1) * 8;
 			
       msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
       msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
       msFrame[frameTail++] = 0x01;   															//DT1
       msFrame[frameTail++] = 0x0F;    															//DT2

 			 //上一整点时间
 			 time = backTime(sysTime, 0, 0, 1, 0, 0);
 			 time.minute = 0;
 			 time.second = 0;
 			
 			 //小时冻结类数据时标Td_h
 			 msFrame[frameTail++] = time.hour / 10 << 4 | time.hour % 10;		//小时时间
 			 density = 0x1;
 			 msFrame[frameTail++] = density;			//冻结密度
 			
 			 switch(density)
 			 {
 				 case 0:		//不冻结
 					 interval = 61;
 					 break;
 				 case 1:		//冻结间隔时间15分
 					 interval = 15;
 					 break;
 				 case 2:		//冻结间隔时间30分
 					 interval = 30;
 					 break;
 				 case 3:		//冻结间隔时间60分
 					 interval = 60;
 					 break;
 				 case 254:	//冻结间隔时间5分
 					 interval = 5;
 					 break;
 				 case 255:	//冻结间隔时间1分
 					 interval = 1;
 					 break;
 			 }
 			
 			 //无效数据的判断
 			 if(interval > 60)
 			 {
 				 return frameTail -= 6;
 			 }
			 
			 time = backTime(time, 0, 0, 0, interval, 0);

 			 //查询数据
 			 for(i = 0; i < 60; i += interval)
 			 {
				 //2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				 //    原因:读取曲线数据时调用readMeterData参数错误
				 //tmpMinute = hexToBcd(interval+i);
				 tmpMinute = hexToBcd(interval);

 				 readTime = timeHexToBcd(time);
 				 buffHasData = readMeterData(dataBuff, pn, DC_ANALOG, DC_ANALOG_CURVE_DATA, &readTime, tmpMinute);
 				 
 				 if(buffHasData == TRUE)
 				 {
 					 msFrame[frameTail++] = dataBuff[0];
 	         msFrame[frameTail++] = dataBuff[1];
 				 }
 				 else
 				 {
 					 msFrame[frameTail++] = 0xEE;
 				   msFrame[frameTail++] = 0xEE;
 				 }
 				
 				 //每个冻结时刻冻结的数据总是在该时刻之前采集的
         time = nextTime(time, interval, 0);
 			 }
 		}
 		da1 >>= 1;
 	}
	
	#endif
	return frameTail;
}

/*******************************************************
函数名称:AFN0C129
功能描述:响应主站请求一类数据"当前正向有功电能示值"命令（数据格式14）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C129(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG  meterConfig;
	INT8U   dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U  pn, tmpPn = 0;
  INT8U   da1, da2;
  INT8U   tariff, tmpTariff;
  INT8U   queryType, dataType;
  struct dotFn129    *fnNew129, *prevFn129, *tmpPfn129;

  BOOL    ifHasData;
  BOOL    buffHasData;
  
  INT16U  offSet;
  INT16U  i;
  
  DATE_TIME time;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
  
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
      	;
      }
      
      switch(fn)
      {
      	case 129:		//当前正向有功电能示值(总，费率1~M)
      	
      	#ifdef PLUG_IN_CARRIER_MODULE
      	 #ifdef LM_SUPPORT_UT
      		if ((meterConfig.rateAndPort&0x1f)==31 && 0x55!=lmProtocol)
      	 #else
          if ((meterConfig.rateAndPort&0x1f)==31)
      	 #endif
          {
      		  fnNew129 = (struct dotFn129 *)malloc(sizeof(struct dotFn129));
      		  fnNew129->pn           = pn;
      		  fnNew129->from         = afn0cDataFrom;
      		  fnNew129->ifProcessing = 0;
      		  fnNew129->next         = NULL;

            prevFn129 = pDotFn129;
            tmpPfn129 = pDotFn129;
            while(tmpPfn129!=NULL)
            {
            	prevFn129 = tmpPfn129;
            	
            	if (tmpPfn129->next!=NULL)
            	{
            	  tmpPfn129 = tmpPfn129->next;
            	}
            	else
            	{
            		break;
            	}
            }
            
      		  if (pDotFn129==NULL)
      		  {
      		  	pDotFn129 = fnNew129;
      		  }
      		  else
      		  {
      		  	prevFn129->next = fnNew129;
      		  }
      		  
      		  tmpPfn129 = pDotFn129;
      		  while(tmpPfn129!=NULL)
      		  {
      		  	 printf("要点抄的测量点=%d\n",tmpPfn129->pn);
      		  	 
      		  	 tmpPfn129 = tmpPfn129->next;
      		  }
      		  
      		  frameTail -= 2;
      		  
      		  goto breakPoint129;
      		  
      		  /*
      		  if (pDotCopy==NULL)
      		  {
      		    pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
      		    pDotCopy->dotCopyMp   = pn;
      		    pDotCopy->dotCopying  = FALSE;
      		    pDotCopy->port        = PORT_POWER_CARRIER;
      		    pDotCopy->dataFrom    = afn0cDataFrom;
      		    pDotCopy->outTime     = nextTime(sysTime,0,15);
      		    pDotCopy->dotResult   = RESULT_NONE;
      		  }
      		  
      		  return frameTail-2;
      		  */
      		}
      		else
      		{
      		  msFrame[frameTail++] = 0x01;   						//DT1
      		  msFrame[frameTail++] = 0x10;   						//DT2
      		  offSet = POSITIVE_WORK_OFFSET;
      		}
      	#else      		
      		msFrame[frameTail++] = 0x01;   							//DT1
      		msFrame[frameTail++] = 0x10;   							//DT2
      		offSet = POSITIVE_WORK_OFFSET;
      	#endif
      	 
      		break;
      		
      	case 130:		//当前正向无功(组合无功1)电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = POSITIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 131:		//当前反向有功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = NEGTIVE_WORK_OFFSET;
      		break;
      		
      	case 132:		//当前反向无功(组合无功1)电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = NEGTIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 133:		//当前一象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA1_NO_WORK_OFFSET;
      		break;
      		
      	case 134:		//当前二象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA2_NO_WORK_OFFSET;
      		break;
      		
      	case 135:		//当前三象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA3_NO_WORK_OFFSET;
      		break;
      		
      	case 136:		//当前四象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x80;   															//DT1
      		msFrame[frameTail++] = 0x10;   															//DT2
      		offSet = QUA4_NO_WORK_OFFSET;
      		break;
      		
      	case 137:		//上月(上一结算日)正向有功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x01;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = POSITIVE_WORK_OFFSET;
      		break;
      		
      	case 138:		//上月(上一结算日)正向无功(组合无功1)电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x02;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = POSITIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 139:		//上月(上一结算日)反向有功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x04;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = NEGTIVE_WORK_OFFSET;
      		break;
      		
      	case 140:		//上月(上一结算日)反向无功(组合无功1)电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x08;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = NEGTIVE_NO_WORK_OFFSET;
      		break;
      		
      	case 141:		//上月(上一结算日)一象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x10;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA1_NO_WORK_OFFSET;
      		break;
      		
      	case 142:		//上月(上一结算日)二象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x20;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA2_NO_WORK_OFFSET;
      		break;
      		
      	case 143:		//上月(上一结算日)三象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x40;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA3_NO_WORK_OFFSET;
      		break;
      		
      	case 144:		//上月(上一结算日)四象限无功电能示值(总，费率1~M)
      		msFrame[frameTail++] = 0x80;   															//DT1
      		msFrame[frameTail++] = 0x11;   															//DT2
      		offSet = QUA4_NO_WORK_OFFSET;
      		break;
      }
      
      
      if(fn >= 129 && fn <= 136)
      {
      	queryType = PRESENT_DATA;
      	dataType = ENERGY_DATA;
      }
      else if(fn >= 137 && fn <= 144)
      {
      	queryType = LAST_MONTH_DATA;
      	dataType = POWER_PARA_LASTMONTH;
      }
      
      //取得费率个数
			tariff = numOfTariff(pn);
			if (((meterConfig.rateAndPort&0x1f)<5 && (meterConfig.bigAndLittleType&0xf)==1)
				  || ((meterConfig.rateAndPort&0x1f)==31)
				 )
			{
        buffHasData = readMeterData(dataBuff, pn, SINGLE_PHASE_PRESENT, dataType, &time, 0);
        
        if (fn==131)
        {
      		offSet = NEGTIVE_WORK_OFFSET_S;
        }
			}
			else
			{
			  buffHasData = readMeterData(dataBuff, pn, queryType, dataType, &time, 0);
			}

			if(buffHasData == TRUE)
			{
				//终端抄表时间
				msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
				
				//取得费率个数
				msFrame[frameTail++] = tariff;
				
				for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
				{
    			//ly,2011-12-12,加这个判断,以前的版本对单相表来说全是错误的
    			if ((((meterConfig.rateAndPort&0x1f)<5 && (meterConfig.bigAndLittleType&0xf)==1)
    				  || ((meterConfig.rateAndPort&0x1f)==31)) && (!((fn==131) || (fn==129)))
    				 )
    			{
    				if (fn==137 || fn==139)
    				{
    					msFrame[frameTail++] = 0xee;
    				}
    				msFrame[frameTail++] = 0xee;
    				msFrame[frameTail++] = 0xee;
    				msFrame[frameTail++] = 0xee;
    				msFrame[frameTail++] = 0xee;
  	      	
  	      	//单费率的处理.费率1与总一样
  	      	if (tariff==1)
  	      	{
    				  if (fn==137 || fn==139)
    				  {
    					  msFrame[frameTail++] = 0xee;
    				  }
    				  msFrame[frameTail++] = 0xee;
    				  msFrame[frameTail++] = 0xee;
    				  msFrame[frameTail++] = 0xee;
    				  msFrame[frameTail++] = 0xee;
    				  
    				  tariff++;  	      		
  	      	}
    			}
    			else
    			{
  					if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
  					{
  						if((dataBuff[offSet] != 0xEE) && (dataBuff[offSet]!=0xFF))
  						{
  							msFrame[frameTail++] = 0x0;
                ifHasData = TRUE;
  						}
  						else
  						{
  							msFrame[frameTail++] = 0xEE;
  						}
  					}
  				  
  				  if(dataBuff[offSet]==0xFF)
  				  {
  				  	msFrame[frameTail++] = 0xee;
  				  	msFrame[frameTail++] = 0xee;
  				  	msFrame[frameTail++] = 0xee;
  				  	msFrame[frameTail++] = 0xee;
  				  }
  				  else
  					{
  					  msFrame[frameTail++] = dataBuff[offSet];
  					  msFrame[frameTail++] = dataBuff[offSet + 1];
  	      	  msFrame[frameTail++] = dataBuff[offSet + 2];
  	      	  msFrame[frameTail++] = dataBuff[offSet + 3];
  	      	}
  	      	
  	      	//单费率的处理.费率1与总一样
  	      	if (tariff==1)
  	      	{
  					  if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
  					  {
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		  msFrame[frameTail++] = msFrame[frameTail-5];
  	      		}
  	      		else
  	      		{
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		  msFrame[frameTail++] = msFrame[frameTail-4];
  	      		}
  	      		
  	      		tmpTariff++;
  	      	}
	      	  offSet += 4;
  	      }
				}
				
				#ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	        	if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
						{
							frameTail -= (10 + (tariff + 1) * 5);
						}
						else
						{
							frameTail -= (10 + (tariff + 1) * 4);
						}
	        }
        #endif
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    //终端抄表时间
					msFrame[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
					msFrame[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
					msFrame[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
					msFrame[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
					msFrame[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
					//费率数
					msFrame[frameTail++] = tariff;
					
					if(fn == 129 || fn == 131 || fn == 137 || fn == 139)
					{
						for(i = 0; i <(tariff + 1) * 5; i++)
						{
							msFrame[frameTail++] = 0xEE;
						}
					}
					else
					{
						for(i = 0; i <(tariff + 1) * 4; i++)
						{
							msFrame[frameTail++] = 0xEE;
						}
					}
		    #endif
			}
		}

breakPoint129:
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C130
功能描述:响应主站请求一类数据"当前正向无功(组合无功1)电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C130(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C131
功能描述:响应主站请求一类数据"当前反向有功电能示值(总,费率1~M)"命令（数据格式14）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C131(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C132
功能描述:响应主站请求一类数据"当前反向无功(组合无功1)电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C132(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C133
功能描述:响应主站请求一类数据"当前一象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C133(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C134
功能描述:响应主站请求一类数据"当前二象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C134(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C135
功能描述:响应主站请求一类数据"当前三象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C135(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C136
功能描述:响应主站请求一类数据"当前四象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C136(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C137
功能描述:响应主站请求一类数据"上月(上一结算日)正向有功电能示值(总,费率1~M)"命令（数据格式14）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C137(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C138
功能描述:响应主站请求一类数据"上月(上一结算日)正向无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C138(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C139
功能描述:响应主站请求一类数据"上月(上一结算日)反向有功电能示值(总,费率1~M)"命令（数据格式14）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C139(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C140
功能描述:响应主站请求一类数据"上月(上一结算日)反向无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C140(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C141
功能描述:响应主站请求一类数据"上月(上一结算日)一象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C141(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C142
功能描述:响应主站请求一类数据"上月(上一结算日)二象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C142(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C143
功能描述:响应主站请求一类数据"上月(上一结算日)三象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C143(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C144
功能描述:响应主站请求一类数据"上月(上一结算日)四象限无功电能示值(总,费率1~M)"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C144(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C129(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C145
功能描述:响应主站请求一类数据"当月正向有功最大需量及发生时间(总,费率1~M)"命令（数据格式17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C145(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[LENGTH_OF_REQ_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     tariff, tmpTariff;
  INT8U     queryType, dataType;
  
  BOOL      ifHasData;
  BOOL      buffHasData;
  
  INT16U    offSet1, offSet2;
  INT16U    i;
  
  DATE_TIME time, readTime;
	
	da1 = *pHandle;
	da2 = *(pHandle+1);
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x01) == 0x01)
		{
			pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
			
      msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
      msFrame[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
      switch(fn)
      {
      	case 145:		//当月正向有功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x01;   															//DT1
      		offSet1 = REQ_POSITIVE_WORK_OFFSET;				//正向有功最大需量
      		offSet2 = REQ_TIME_P_WORK_OFFSET;					//正向有功最大需量发生时间
      		break;
      		
      	case 146:		//当月正向无功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x02;   															//DT1
      		offSet1 = REQ_POSITIVE_NO_WORK_OFFSET;		//正向无功最大需量
      		offSet2 = REQ_TIME_P_NO_WORK_OFFSET;			//正向无功最大需量发生时间
      		break;
      		
      	case 147:		//当月反向有功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x04;   															//DT1
      		offSet1 = REQ_NEGTIVE_WORK_OFFSET;				//反向有功最大需量
      		offSet2 = REQ_TIME_N_WORK_OFFSET;					//反向有功最大需量发生时间
      		break;
      		
      	case 148:		//当月反向无功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x08;   															//DT1
      		offSet1 = REQ_NEGTIVE_NO_WORK_OFFSET;			//反向无功最大需量
      		offSet2 = REQ_TIME_N_NO_WORK_OFFSET;			//反向无功最大需量发生时间
      		break;
      		
      	case 149:		//上月(上一结算日)正向有功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x10;   															//DT1
      		offSet1 = REQ_POSITIVE_WORK_OFFSET;				//正向有功最大需量
      		offSet2 = REQ_TIME_P_WORK_OFFSET;					//正向有功最大需量发生时间
      		break;
      		
      	case 150:		//上月(上一结算日)正向无功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x20;   															//DT1
      		offSet1 = REQ_POSITIVE_NO_WORK_OFFSET;		//正向无功最大需量
      		offSet2 = REQ_TIME_P_NO_WORK_OFFSET;			//正向无功最大需量发生时间
      		break;
      		
      	case 151:		//上月(上一结算日)反向有功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x40;   															//DT1
      		offSet1 = REQ_NEGTIVE_WORK_OFFSET;				//反向有功最大需量
      		offSet2 = REQ_TIME_N_WORK_OFFSET;					//反向有功最大需量发生时间
      		break;
      		
      	case 152:		//上月(上一结算日)反向无功最大需量及发生时间(总，费率1~M)
      		msFrame[frameTail++] = 0x80;   															//DT1
      		offSet1 = REQ_NEGTIVE_NO_WORK_OFFSET;			//反向无功最大需量
      		offSet2 = REQ_TIME_N_NO_WORK_OFFSET;			//反向无功最大需量发生时间
      		break;
      }
      msFrame[frameTail++] = 0x12;   																	//DT2
      
      
      if(fn >= 145 && fn <= 148)
      {
      	queryType = PRESENT_DATA;
      	dataType = REQ_REQTIME_DATA;
      }
      else if(fn >= 149 && fn <= 152)
      {
      	queryType = LAST_MONTH_DATA;
      	dataType = REQ_REQTIME_LASTMONTH;
      }
      
      ifHasData = FALSE;
      
      //取得费率个数
			tariff = numOfTariff(pn);      
			
			buffHasData = readMeterData(dataBuff, pn, queryType, dataType, &time, 0);
				
			if(buffHasData == TRUE)
			{
				//终端抄表时间
				msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
				
				msFrame[frameTail++] = tariff;
				
				//最大需量及发生时间
				for(tmpTariff = 0; tmpTariff <= tariff; tmpTariff++)
				{
					//最大需量
					if(dataBuff[offSet1] != 0xEE)
					{
						ifHasData = TRUE;
					}
					msFrame[frameTail++] = dataBuff[offSet1];
					msFrame[frameTail++] = dataBuff[offSet1 + 1];
					msFrame[frameTail++] = dataBuff[offSet1 + 2];
					
					//发生时间(分时日月)
					if(dataBuff[offSet2] != 0xEE)
					{
						ifHasData = TRUE;
					}
					msFrame[frameTail++] = dataBuff[offSet2];
					msFrame[frameTail++] = dataBuff[offSet2 + 1];
	      	msFrame[frameTail++] = dataBuff[offSet2 + 2];
	      	msFrame[frameTail++] = dataBuff[offSet2 + 3];
	      	
	      	//单费率的处理.费率1与总一样
	      	if (tariff==1)
	      	{
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 msFrame[frameTail++] = msFrame[frameTail-7];
	      		 
	      		 tmpTariff++;
	      	}
	      	else
	      	{
	      	  offSet1 += 3;
	      	  offSet2 += 5;
	      	}
				}
				
				#ifdef NO_DATA_USE_PART_ACK_03
	        if (ifHasData == FALSE)
	        {
	      	  frameTail -= (10 + (tariff + 1) * 7);
	        }
        #endif
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    //终端抄表时间
					msFrame[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
					msFrame[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
					msFrame[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
					msFrame[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
					msFrame[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
					//费率数
					msFrame[frameTail++] = tariff;
					
					for(i = 0; i < (tariff + 1) * 7; i++)
					{
						msFrame[frameTail++] = 0xEE;
					}
		    #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C146
功能描述:响应主站请求一类数据"当月正向无功最大需量及发生时间(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C146(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C147
功能描述:响应主站请求一类数据"当月反向有功最大需量及发生时间(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C147(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C148
功能描述:响应主站请求一类数据"当月反向无功最大需量及发生时间(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C148(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C149
功能描述:响应主站请求一类数据"上月(上一结算日)正向有功最大需量及发生时间
				(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C149(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C150
功能描述:响应主站请求一类数据"上月(上一结算日)正向无功最大需量及发生时间
				(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C150(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C151
功能描述:响应主站请求一类数据"上月(上一结算日)反向有功最大需量及发生时间
				(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C151(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C152
功能描述:响应主站请求一类数据"上月(上一结算日)反向无功最大需量及发生时间
				(总,费率1~M)"命令（数据格式15,17,23）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C152(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return AFN0C145(frameTail,pHandle, fn);
}

/*******************************************************
函数名称:AFN0C153
功能描述:响应主站请求一类数据"第一时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C153(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C154
功能描述:响应主站请求一类数据"第二时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C154(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C155
功能描述:响应主站请求一类数据"第三时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C155(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C156
功能描述:响应主站请求一类数据"第四时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C156(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C157
功能描述:响应主站请求一类数据"第五时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C157(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C158
功能描述:响应主站请求一类数据"第六时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C158(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C159
功能描述:响应主站请求一类数据"第七时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C159(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C160
功能描述:响应主站请求一类数据"第八时区冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C160(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C161
功能描述:响应主站请求一类数据"电能表远程控制通断电状态及记录"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C161(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	INT8U     dataBuff[512];
	INT8U     tmpPn, pn;
  INT8U     i, j;
  INT8U     da1, da2;
  BOOL      ifHasData;
  BOOL      buffHasData;  
  INT16U    tmpTail,counti;  
  DATE_TIME time;
  INT8U     meterInfo[10];
	
  tmpPn = 0;
  da1 = *pHandle;
  da2 = *(pHandle+1);
  
  if(da1 == 0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  		//查询本测量点的上次抄表时间
  		time = queryCopyTime(pn);
  		
  		//查询测量点存储信息
  		queryMeterStoreInfo(pn, meterInfo);
  		
  		if (meterInfo[0]<7)
  		{
  		   buffHasData =  readMeterData(dataBuff, pn , meterInfo[1], ENERGY_DATA, &time, 0);
  		}
  		else
  		{
  		   buffHasData =  readMeterData(dataBuff, pn , meterInfo[1], PARA_VARIABLE_DATA, &time, 0);
  		}
		    	
		  msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  			//DA1
		  msFrame[frameTail++] = (pn - 1) / 8 + 1;                      		//DA2
		  
		  switch(fn)
		  {
		    case 161:   //电能表远程控制通断电状态及记录
		      msFrame[frameTail++] = 0x01;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2
		      break;
		      
		    case 165:   //电能表开关操作次数及时间
		      msFrame[frameTail++] = 0x10;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2		    	
		    	break;

		    case 166:   //电能表参数修改次数及时间
		      msFrame[frameTail++] = 0x20;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2		    	
		    	break;

		    case 167:   //电能表购、用电信息
		      msFrame[frameTail++] = 0x40;                                      //DT1
		      msFrame[frameTail++] = 0x14;                                      //DT2		    	
		    	break;
		  }
		
		  ifHasData = FALSE;
		  if (buffHasData == TRUE)
		  {
		    //终端抄表时间
		    msFrame[frameTail++] = time.minute;
				msFrame[frameTail++] = time.hour;
				msFrame[frameTail++] = time.day;
				msFrame[frameTail++] = time.month;
				msFrame[frameTail++] = time.year;
		     		     
		    switch(fn)
		    {
		      case 161:
		        switch (meterInfo[0])
		        {
  		        case 1:  //单相智能表(没有该项数据)
  		        	for(i=0;i<11;i++)
  		        	{
  		        		 msFrame[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //单相本地费控表
  		        case 3:  //单相远程费控表
  		          //电能表通断电状态
  		          if (dataBuff[METER_STATUS_WORD_S_3]&0x40)
  		          {
  		            msFrame[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            msFrame[frameTail++] = 0x0;
  		          }
  		    
  		          //最近一次电能表远程控制通电时间
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		    
  		          //最近一次电能表远程控制断电时间
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		          break;

  		        case 5:  //三相本地费控表
  		        case 6:  //三相远程费控表
  		          //电能表通断电状态
  		          if (dataBuff[METER_STATUS_WORD_T_3]&0x40)
  		          {
  		            msFrame[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            msFrame[frameTail++] = 0x0;
  		          }
  		    
  		          //最近一次电能表远程控制通电时间
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		    
  		          //最近一次电能表远程控制断电时间
  		          for(i=1;i<6;i++)
  		          {
  		            msFrame[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		          break;
  		          
  		        default:
    		        //电能表通断电状态
    		        if (dataBuff[METER_STATUS_WORD_3]&0x40)
    		        {
    		          msFrame[frameTail++] = 0x11;
    		        }
    		        else
    		        {
    		          msFrame[frameTail++] = 0x0;
    		        }
    		    
    		        //最近一次电能表远程控制通电时间
    		        for(i=1;i<6;i++)
    		        {
    		          msFrame[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME+i];
    		        }
    		        frameTail += 5;
    		    
    		        //最近一次电能表远程控制断电时间
    		        for(i=1;i<6;i++)
    		        {
    		          msFrame[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME+i];
    		        }
    		        frameTail += 5;
    		        break;
  		      }
		        break;
		        
		      case 165:  //电能表开关操作次数衣时间
		      	//编程开关操作次数
		      	msFrame[frameTail++] = dataBuff[PROGRAM_TIMES];
		      	msFrame[frameTail++] = dataBuff[PROGRAM_TIMES+1];
		      	
		      	//最后一次编程开关操作时间
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[LAST_PROGRAM_TIME+i];
		        }
		        frameTail += 5;
		        
		        //电能表尾盖打开次数
		      	msFrame[frameTail++] = dataBuff[OPEN_METER_COVER_TIMES];
		      	msFrame[frameTail++] = dataBuff[OPEN_METER_COVER_TIMES+1];
		        
		        //最近一次尾盖打开时间
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[LAST_OPEN_METER_COVER_TIME+i];
		        }
		        frameTail += 5;
		      	break;
		      	
		      case 166:  //电能表参数修改次数及时间
		      	//电能表时钟修改次数
		      	msFrame[frameTail++] = dataBuff[TIMING_TIMES];
		      	msFrame[frameTail++] = dataBuff[TIMING_TIMES+1];
		      	
		        //最近一次校时发生时间
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[TIMING_LAST_TIME+i];
		        }
		        frameTail += 5;
		        
		      	//电能表时段参数修改次数
		      	msFrame[frameTail++] = dataBuff[PERIOD_TIMES];
		      	msFrame[frameTail++] = dataBuff[PERIOD_TIMES+1];
		      	
		        //最近一次时段参数修改发生时间
		        for(i=1;i<6;i++)
		        {
		          msFrame[frameTail+i-1] = dataBuff[PERIOD_LAST_TIME+i];
		        }
		        frameTail += 5;
		      	break;
		      	
		      case 167:   //电能表购、用电信息
		        switch (meterInfo[0])
		        {
  		        case 1:  //单相智能表(没有/未抄该项数据)
  		        case 3:  //单相远程费控表(没有/未抄该项数据)
  		        case 6:  //三相远程费控表(没有/未抄该项数据)
  		        	for(i=0;i<36;i++)
  		        	{
  		        		 msFrame[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //单相本地费控表
     		      	//购电次数
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S+1];
     		      	
     		      	//剩余金额
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_S]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+3];
     		      	
     		      	//累计购电金额
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_S]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+3];
     		      	
     		      	//剩余电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+3];
     		      	
     		      	//透支电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+3];
     		      	
     		      	//累计购电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+3];
     		      	
     		      	//赊欠门限电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+3];
     		      	
     		      	//报警电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+3];
     		      	
     		      	//故障电量
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
  		        	break;
  		        	
  		        case 5:  //三相本地费控表
     		      	//购电次数
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T+1];
     		      	
     		      	//剩余金额
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_T]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+3];
     		      	
     		      	//累计购电金额
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_T]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+3];
     		      	
     		      	//剩余电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+3];
     		      	
     		      	//透支电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+3];
     		      	
     		      	//累计购电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+3];
     		      	
     		      	//赊欠门限电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+3];
     		      	
     		      	//报警电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+3];
     		      	
     		      	//故障电量
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
  		        	break;
  		        	  		        	
  		        default:
     		      	//购电次数
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME+1];
     		      	
     		      	//剩余金额
     		      	if (dataBuff[CHARGE_REMAIN_MONEY]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+3];
     		      	
     		      	//累计购电金额
     		      	if (dataBuff[CHARGE_TOTAL_MONEY]==0xee)
     		      	{
     		      		msFrame[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		msFrame[frameTail++] = 0x00;		      		
     		      	}	
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+3];
     		      	
     		      	//剩余电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+3];
     		      	
     		      	//透支电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+3];
     		      	
     		      	//累计购电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+3];
     		      	
     		      	//赊欠门限电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+3];
     		      	
     		      	//报警电量
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+1];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+2];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+3];
     		      	
     		      	//故障电量
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	msFrame[frameTail++] = 0xee;
     		      	break;
     		    }
		      	break;
		    }
		  }
		  else
		  {
		    #ifdef NO_DATA_USE_PART_ACK_03
		    	frameTail -= 4;
		    #else
			    msFrame[frameTail++] = sysTime.minute/10<<4 | sysTime.minute%10;
			    msFrame[frameTail++] = sysTime.hour  /10<<4 | sysTime.hour  %10;
			    msFrame[frameTail++] = sysTime.day   /10<<4 | sysTime.day   %10;
			    msFrame[frameTail++] = sysTime.month /10<<4 | sysTime.month %10;
			    msFrame[frameTail++] = sysTime.year  /10<<4 | sysTime.year  %10;
			    switch(fn)
			    {
			    	case 161:
			        for(j=0;j<11;j++)
			        {
			  	      msFrame[frameTail++] = 0xee;
			        }
			        break;
			        
			      case 165:
			      case 166:
			        for(j=0;j<14;j++)
			        {
			  	      msFrame[frameTail++] = 0xee;
			        }
			      	break;

			      case 167:
			        for(j=0;j<36;j++)
			        {
			  	      msFrame[frameTail++] = 0xee;
			        }
			      	break;
			    }
		    #endif
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
函数名称:AFN0C165
功能描述:响应主站请求一类数据"电能表开关操作次数及时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C165(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C161(frameTail, pHandle, fn);
}

/*******************************************************
函数名称:AFN0C166
功能描述:响应主站请求一类数据"电能表参数修改次数及时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C166(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C161(frameTail, pHandle, fn);
}

/*******************************************************
函数名称:AFN0C167
功能描述:响应主站请求一类数据"电能表购、用电信息"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C167(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
  return AFN0C161(frameTail, pHandle, fn);
}

/*******************************************************
函数名称:AFN0C168
功能描述:响应主站请求一类数据"电能表结算信息"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C168(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	return frameTail;
}

/*******************************************************
函数名称:AFN0C169
功能描述:响应主站请求一类数据"集中抄表中继路由信息"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C169(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	#ifdef PLUG_IN_CARRIER_MODULE
    INT16U    tmpPn,pn;
    INT8U     da1, da2;
    METER_DEVICE_CONFIG meterConfig;

    da1 = *pHandle++;
    da2 = *pHandle++;
    
    tmpPn = 0;
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		
		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x01;                                   //DT1
		    msFrame[frameTail++] = 0x15;                                   //DT2
        
        if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
        {
		      msFrame[frameTail++] = 0xee;   //通信端口为
		      msFrame[frameTail++] = 0xee;   //中继路由个数
		    }
		    else
		    {
		      msFrame[frameTail++] = meterConfig.rateAndPort&0x1f;   //通信端口为31
		      msFrame[frameTail++] = 0x0;    //中继路由个数
		    }
    	}
    	da1 >>= 1;
    }	 
	#endif
	
	return frameTail;
}

/*******************************************************
函数名称:AFN0C170
功能描述:响应主站请求一类数据"集中抄表电能表抄读信息"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0C170(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	#ifdef PLUG_IN_CARRIER_MODULE
    INT16U    tmpPn,pn;
    INT8U     da1, da2;
    METER_DEVICE_CONFIG meterConfig;
    METER_STATIS_BEARON_TIME meterStatisBearonTime;   //一块表统计事件数据(与时间有关量)
    
    da1 = *pHandle++;
    da2 = *pHandle++;
    
    tmpPn = 0;
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		
    		printf("AFN0C FN170,pn=%d\n",pn);

		    msFrame[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  	 //DA1
		    msFrame[frameTail++] = 0x01<<((pn-1)/8);                       //DA2
		    msFrame[frameTail++] = 0x02;                                   //DT1
		    msFrame[frameTail++] = 0x15;                                   //DT2
        if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
        {
        	 memset(&msFrame[frameTail], 0xee, 18);
        }	
        else
        {
  		    msFrame[frameTail++] = meterConfig.rateAndPort&0x1f;   //通信端口号
  		    
  		    if((meterConfig.rateAndPort&0x1f)<4)
  		    {
  		      msFrame[frameTail++] = 0xee; //中继路由级数
  		      msFrame[frameTail++] = 0xee; //载波抄读通信相位
  		      msFrame[frameTail++] = 0xee; //载波信号品质
  		    }
  		    else
  		    {
  		      msFrame[frameTail++] = 0;    //中继路由级数
  		      msFrame[frameTail++] = 0x71; //载波抄读通信相位
  		      msFrame[frameTail++] = 0xfe; //载波信号品质
  		    }
  		    
          searchMpStatis(sysTime, &meterStatisBearonTime, pn, 2);

  		    if (meterStatisBearonTime.copySuccessTimes==0)
  		    {
  		      msFrame[frameTail++] = 0x0;  //最后一次抄表成功/失败标志为失败
  		    
  		      //最近一次抄表成功时间
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x0;
  		    
  		      //最近一次抄表失败时间
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x0;
  		    
  		      //最近连续失败累计次数
  		      msFrame[frameTail++] = 0;
  		    }
  		    else
  		    {  		    
  		      msFrame[frameTail++] = 0x1;  //最后一次抄表成功/失败标志为成功
  		    
  		      //最近一次抄表成功时间
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = meterStatisBearonTime.lastCopySuccessTime[0];
  		      msFrame[frameTail++] = meterStatisBearonTime.lastCopySuccessTime[1];
  		      msFrame[frameTail++] = sysTime.day/10<<4 | sysTime.day%10;
  		      msFrame[frameTail++] = sysTime.month/10<<4 | sysTime.month%10;
  		      msFrame[frameTail++] = sysTime.year/10<<4 | sysTime.year%10;
  		    
  		      //最近一次抄表失败时间
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x0;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x1;
  		      msFrame[frameTail++] = 0x0;
  		    
  		      //最近连续失败累计次数
  		      msFrame[frameTail++] = 0;
  		    }
  		  }		    
    	}
    	da1 >>= 1;
    }	 
	#endif
	
	return frameTail;
}

/*******************************************************
函数名称:queryCopyTime
功能描述:根据测量点号查询上次抄表时间
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：上次抄表时间
*******************************************************/
DATE_TIME queryCopyTime(INT16U pn)
{   
   METER_DEVICE_CONFIG meterConfig;         //一个测量点的配置信息
   DATE_TIME           time;
   
   if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
   {
     time = timeHexToBcd(sysTime);
     //库中没有该测量点的数据
   }
   else
   {
   	 switch (meterConfig.rateAndPort&0xf)
   	 {
   	   case 1:
   	     time = copyCtrl[0].lastCopyTime;
   	     break;
   	     
   	   case 2:
   	     time = copyCtrl[1].lastCopyTime;
   	     break;

   	   case 3:
   	     time = copyCtrl[2].lastCopyTime;
   	     break;

   	   case 4:
   	     time = copyCtrl[3].lastCopyTime;
   	     break;
   	     
   	   case 31:
   	     time = copyCtrl[4].lastCopyTime;
   	     break;
   	     
   	   default:
   	   	 time = timeHexToBcd(sysTime);
   	   	 break;
   	 }
   }
  
   return time;
}

