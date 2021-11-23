/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN0D.c
作者：TianYe
版本：0.9
完成日期：2010年1月
描述：主站AFN0D(请求二类数据)处理文件。
函数列表：
修改历史：
  01,2006-08-09,TinaYe created.
  02,2010-06-18,Leiyong modify,将主动上送二类数据改为线程处理
  03,2012-10-22,修改错误。在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
		 原因:读取曲线数据时调用readMeterData参数错误
***************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMeter.h"
#include "meterProtocol.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "convert.h"
#include "wlModem.h"

#include "AFN0C.h"
#include "AFN0D.h"

extern INT8U  ackTail;

BOOL   hasCurveVisionData;           //没有曲线示值数据? ly,2009-12-02,add
INT8U  bakDataBuff[1024];            //备份数据缓存

/*******************************************************
函数名称:AFN0D
功能描述:主站"请示二类数据"(AFN0D)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN0D(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll)
{
    INT16U    tmpFrameTail;             //分组尾
    INT16U    tmpI;
    INT8U     fn;
    INT8U     tmpDtCount;               //DT移位计数
    INT8U     tmpDt1;                   //临时DT1
    INT8U     *pTpv;                    //TpV指针
    INT8U     maxCycle;                 //最大循环次数
    INT16U    tmpHead0dActive = 0;      //主动上报AFN0D临时帧头
    INT16U    tmpHead0d   = 0;          //AFN0D临时帧头
    INT16U    frameTail0d = 0;          //AFN0D分组尾
    INT16U    offset0d;                 //接收到的帧中的数据单元偏移量(不计数据标识的字节)
    INT8U     checkSum;
    INT16U    frameLoadLen;
    INT8U     thisThreadSending;        //有主动上报数据要发送
    INT8U     taskNum;                  //任务号
    INT8U     tmpDataBuff[1024];        //临时数据缓存
    DATE_TIME activeThreadTimeOut;      //主动上报时作为线程的超时时间
	  
	  INT8U     *pTmpFrame;
	  
	  #ifdef RECORD_LOG
	   char    logStr[2000];
	   char    sayStr[10];
	   INT16U  i;
    #endif

    
    INT16U (*AFN0DFun[221])(INT8U *buff, INT16U frameTail,INT8U *pHandle,INT8U fn, INT16U *offset0d);

    for (tmpI=0; tmpI<221; tmpI++)
    {
       AFN0DFun[tmpI] = NULL;
    }

    //组1 电能表示值、最大需量及电能量
    AFN0DFun[0] = AFN0D001;
    AFN0DFun[1] = AFN0D002;
    AFN0DFun[2] = AFN0D003;
    AFN0DFun[3] = AFN0D004;
    AFN0DFun[4] = AFN0D005;
    AFN0DFun[5] = AFN0D006;
    AFN0DFun[6] = AFN0D007;
    AFN0DFun[7] = AFN0D008;
    
    //组2 电能示值、最大需量
    AFN0DFun[8] = AFN0D009;
    AFN0DFun[9] = AFN0D010;
    AFN0DFun[10] = AFN0D011;
    AFN0DFun[11] = AFN0D012;
    
    //组3 电能示值、最大需量及电能量
    AFN0DFun[16] = AFN0D017;
    AFN0DFun[17] = AFN0D018;
    AFN0DFun[18] = AFN0D019;
    AFN0DFun[19] = AFN0D020;
    AFN0DFun[20] = AFN0D021;
    AFN0DFun[21] = AFN0D022;
    AFN0DFun[22] = AFN0D023;
    AFN0DFun[23] = AFN0D024;
    
    //组4 测量点统计数据
    AFN0DFun[24] = AFN0D025;
    AFN0DFun[25] = AFN0D026;
    AFN0DFun[26] = AFN0D027;
    AFN0DFun[27] = AFN0D028;
    AFN0DFun[28] = AFN0D029;
    AFN0DFun[29] = AFN0D030;
    AFN0DFun[30] = AFN0D031;
    AFN0DFun[31] = AFN0D032;
    
    //组5 测量点统计数据
    AFN0DFun[32] = AFN0D033;
    AFN0DFun[33] = AFN0D034;
    AFN0DFun[34] = AFN0D035;
    AFN0DFun[35] = AFN0D036;
    AFN0DFun[36] = AFN0D037;
    AFN0DFun[37] = AFN0D038;
    AFN0DFun[38] = AFN0D039;
    
    //组6 无功补偿统计数据
    AFN0DFun[40] = AFN0D041;
    AFN0DFun[41] = AFN0D042;
    AFN0DFun[42] = AFN0D043;
    AFN0DFun[43] = AFN0D044;
    AFN0DFun[44] = AFN0D045;
    AFN0DFun[45] = AFN0D046;
    
    //组7 终端统计数据
    AFN0DFun[48] = AFN0D049;
    AFN0DFun[49] = AFN0D050;
    AFN0DFun[50] = AFN0D051;
    AFN0DFun[51] = AFN0D052;
    AFN0DFun[52] = AFN0D053;
    AFN0DFun[53] = AFN0D054;
    
    AFN0DFun[54] = AFN0D055;   //重庆规约
    AFN0DFun[55] = AFN0D056;   //重庆规约

    //组8 总加组统计数据
    AFN0DFun[56] = AFN0D057;
    AFN0DFun[57] = AFN0D058;
    AFN0DFun[58] = AFN0D059;
    AFN0DFun[59] = AFN0D060;
    AFN0DFun[60] = AFN0D061;
    AFN0DFun[61] = AFN0D062;
    
    //组9 总加组越限统计数据
    AFN0DFun[64] = AFN0D065;
    AFN0DFun[65] = AFN0D066;
    
    //组10 总加组曲线
    AFN0DFun[72] = AFN0D073;
    AFN0DFun[73] = AFN0D074;
    AFN0DFun[74] = AFN0D075;
    AFN0DFun[75] = AFN0D076;
	
	//视在功率曲线,2017-7-18,Add
    AFN0DFun[76] = AFN0D077;
    AFN0DFun[77] = AFN0D079;
    AFN0DFun[78] = AFN0D079;
    AFN0DFun[79] = AFN0D080;
    
    //组11 功率曲线
    AFN0DFun[80] = AFN0D081;
    AFN0DFun[81] = AFN0D082;
    AFN0DFun[82] = AFN0D083;
    AFN0DFun[83] = AFN0D084;
    AFN0DFun[84] = AFN0D085;
    AFN0DFun[85] = AFN0D086;
    AFN0DFun[86] = AFN0D087;
    AFN0DFun[87] = AFN0D088;
    
    //组12 电压电流曲线
    AFN0DFun[88] = AFN0D089;
    AFN0DFun[89] = AFN0D090;
    AFN0DFun[90] = AFN0D091;
    AFN0DFun[91] = AFN0D092;
    AFN0DFun[92] = AFN0D093;
    AFN0DFun[93] = AFN0D094;
    AFN0DFun[94] = AFN0D095;

    //2017-7-19,add,频率曲线
	AFN0DFun[95] = AFN0D096;
    
    //组13 总电能量，总电能示值曲线
    AFN0DFun[96] = AFN0D097;
    AFN0DFun[97] = AFN0D098;
    AFN0DFun[98] = AFN0D099;
    AFN0DFun[99] = AFN0D100;
    AFN0DFun[100] = AFN0D101;
    AFN0DFun[101] = AFN0D102;
    AFN0DFun[102] = AFN0D103;
    AFN0DFun[103] = AFN0D104;
    
    //组14 功率因数，电压相位角曲线，电流相位角曲线 
    AFN0DFun[104] = AFN0D105;
    AFN0DFun[105] = AFN0D106;
    AFN0DFun[106] = AFN0D107;
    AFN0DFun[107] = AFN0D108;
    AFN0DFun[108] = AFN0D109;
    AFN0DFun[109] = AFN0D110;
   
   #ifdef LIGHTING
    AFN0DFun[110] = AFN0D111;
    AFN0DFun[111] = AFN0D112;
   #else
    AFN0DFun[110] = AFN0D111;
   #endif
    
    //组15 谐波监测统计数据
    AFN0DFun[112] = AFN0D113;
    AFN0DFun[113] = AFN0D114;
    AFN0DFun[114] = AFN0D115;
    AFN0DFun[115] = AFN0D116;
    AFN0DFun[116] = AFN0D117;
    AFN0DFun[117] = AFN0D118;
    
    //组16 谐波越限统计数据
    AFN0DFun[120] = AFN0D121;
    AFN0DFun[121] = AFN0D122;
    AFN0DFun[122] = AFN0D123;
    
    //组17直流模拟量数据
    AFN0DFun[128] = AFN0D129;
    AFN0DFun[129] = AFN0D130;
    
    //组18 直流模拟量数据曲线
    AFN0DFun[137] = AFN0D138;
    
    //组19 四个象限无功总电能示值曲线
    AFN0DFun[144] = AFN0D145;
    AFN0DFun[145] = AFN0D146;
    AFN0DFun[146] = AFN0D147;
    AFN0DFun[147] = AFN0D148;

   #ifdef LIGHTING
    //当天最后一次开、关、调时刻
    AFN0DFun[148] = AFN0D149;
    AFN0DFun[149] = AFN0D150;
    AFN0DFun[150] = AFN0D151;
    AFN0DFun[151] = AFN0D152;   
   #endif
    
    //组20 分相电能示值
    AFN0DFun[152] = AFN0D153;
    AFN0DFun[153] = AFN0D154;
    AFN0DFun[154] = AFN0D155;
    AFN0DFun[155] = AFN0D156;
    AFN0DFun[156] = AFN0D157;
    AFN0DFun[157] = AFN0D158;
    AFN0DFun[158] = AFN0D159;
    AFN0DFun[159] = AFN0D160;
    
    //组21
    AFN0DFun[160] = AFN0D161;
    AFN0DFun[161] = AFN0D162;
    AFN0DFun[162] = AFN0D163;
    AFN0DFun[163] = AFN0D164;
    AFN0DFun[164] = AFN0D165;
    AFN0DFun[165] = AFN0D166;
    AFN0DFun[166] = AFN0D167;
    AFN0DFun[167] = AFN0D168;
    
    //组22
    AFN0DFun[168] = AFN0D169;
    AFN0DFun[169] = AFN0D170;
    AFN0DFun[170] = AFN0D171;
    AFN0DFun[171] = AFN0D172;
    AFN0DFun[172] = AFN0D173;
    AFN0DFun[173] = AFN0D174;
    AFN0DFun[174] = AFN0D175;
    AFN0DFun[175] = AFN0D176;

    //组23
    AFN0DFun[176] = AFN0D177;
    AFN0DFun[177] = AFN0D178;
    AFN0DFun[178] = AFN0D179;
    AFN0DFun[179] = AFN0D180;
    AFN0DFun[180] = AFN0D181;
    AFN0DFun[181] = AFN0D182;
    AFN0DFun[182] = AFN0D183;
    AFN0DFun[183] = AFN0D184;
    
    //组24
    AFN0DFun[184] = AFN0D185;
    AFN0DFun[185] = AFN0D186;
    AFN0DFun[186] = AFN0D187;
    AFN0DFun[187] = AFN0D188;
    AFN0DFun[188] = AFN0D189;
    AFN0DFun[189] = AFN0D190;
    AFN0DFun[190] = AFN0D191;
    AFN0DFun[191] = AFN0D192;
    
    //组25
    AFN0DFun[192] = AFN0D193;
    AFN0DFun[193] = AFN0D194;
    AFN0DFun[194] = AFN0D195;
    AFN0DFun[195] = AFN0D196;
    
    //组26
    AFN0DFun[200] = AFN0D201;
    AFN0DFun[201] = AFN0D202;
    AFN0DFun[202] = AFN0D203;
    AFN0DFun[203] = AFN0D204;
    AFN0DFun[204] = AFN0D205;
    AFN0DFun[205] = AFN0D206;
    AFN0DFun[206] = AFN0D207;
    AFN0DFun[207] = AFN0D208;
    
    //组27
    AFN0DFun[208] = AFN0D209;
    AFN0DFun[212] = AFN0D213;
    AFN0DFun[213] = AFN0D214;
    AFN0DFun[214] = AFN0D215;
    AFN0DFun[215] = AFN0D216;
    
    //组28
    AFN0DFun[216] = AFN0D217;
    AFN0DFun[217] = AFN0D218;

    AFN0DFun[220] = AFN0D221;
    
    if (poll == AFN0B_REQUIRE)
    {
       frameTail0d = 18;
    }
    else
    {
       frameTail0d = 14;
    }
    
    tmpDt1 = 0;
    tmpDtCount = 0;
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    frameLoadLen = frame.loadLen;
    if (poll==ACTIVE_REPORT)
    {
    	 frameLoadLen = pDataEnd - pDataHead;
    	 fQueue.active0dDataSending = 0;
    	 taskNum = *(pDataEnd+1);
    	 
       hasCurveVisionData = TRUE;   //有曲线示值数据
    }
    
    if (debugInfo&PRINT_ACTIVE_REPORT)
    {
      printf("AFN0D In-Frame:");
      pTmpFrame = pDataHead;
      while(pTmpFrame!=pDataEnd)
      {
    	  printf("%02X ", *pTmpFrame++);
      }
      printf("\n");
    }
    
    
    if (poll==ACTIVE_REPORT)
    {
    	 activeThreadTimeOut = nextTime(sysTime,60,0);    //作为线程的超时时间
    }
    thisThreadSending = 0;
    fQueue.active0dTaskId = 0;
    maxCycle = 0;
    while ((frameLoadLen > 0) && (maxCycle<1500))
    {
    	maxCycle++;

    	//printf("任务号=%d,loadLen=%d, maxCycle=%d,offset0d=%d,frameTail0d=%d\n",taskNum,frameLoadLen, maxCycle,offset0d,frameTail0d);
    	
    	offset0d = 0;
      tmpDt1 = *(pDataHead+2);
      tmpDtCount = 0;
      while(tmpDtCount<9)
      {
       	if (poll == ACTIVE_REPORT)
       	{
     	  	//因为线程在无线链路不通时会处于一个等待通信的状态,线程会越积越多,,这是一种灾难
     	  	//如果作为线程时一个小时都还没完成,直接退出线程
     	  	if (compareTwoTime(activeThreadTimeOut, sysTime))
     	  	{
     	  		 return;
     	  	}

       		if (thisThreadSending>0)
       		{
         		//有数据但一次都还没发
         		if (fQueue.active0dDataSending==0)
         		{
               //信道空闲,发送
               //if (fQueue.delay==0 && (fQueue.active0dTaskId==taskNum || fQueue.active0dTaskId==0) && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
       				 if (fQueue.active0dDataSending==0  && fQueue.delay==0 && fQueue.inTimeFrameSend==FALSE && fQueue.activeFrameSend==FALSE && fQueue.active0dTaskId==0 && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
               {
       				   thisThreadSending = TRUE;
       				   fQueue.active0dDataSending = 1;
       				   fQueue.active0dTaskId = taskNum;
       				  
       				   fQueue.delay = 1;
  	 	           fQueue.delayTime = nextTime(sysTime, 0, 15);   //延时15秒后再发下一帧

       				   memcpy(bakDataBuff, tmpDataBuff, frameTail0d+2);
       			     sendToMs(tmpDataBuff, frameTail0d+2);
    	 	                        
                 if (debugInfo&PRINT_ACTIVE_REPORT)
                 {
                   printf("AFN0D主动上报(任务号=%d):第一次发送本帧,长度=%d\n",taskNum, frameTail0d+2);
                 }
                 
                 #ifdef RECORD_LOG
                  strcpy(logStr,"");
                  sprintf(logStr,"AFN0D主动上报(任务号=%d):第一次发送本帧,长度=%d:",taskNum,frameTail0d+2);
                  for(i=0;i<frameTail0d+2;i++)
                  {
                  	strcpy(sayStr,"");
                  	sprintf(sayStr,"%02x ",tmpDataBuff[i]);
                  	strcat(logStr,sayStr);
                  }
                  logRun(logStr);
                 #endif
               }
               
               usleep(500000);
               continue;
         		}
         		
       		  if (fQueue.active0dTaskId==taskNum)
       		  {
         		  //发送第一次后的等待
         		  if (fQueue.active0dDataSending==1)
         		  {
         			   usleep(500000);
         			   continue;
         		  }
         		
           		//没等到第一次发送的确认,重发
           		if (fQueue.active0dDataSending==2)
           		{
                 //信道空闲,发送
                 if (fQueue.delay==0 && (fQueue.active0dTaskId==taskNum || fQueue.active0dTaskId==0) && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
                 {
                   fQueue.active0dDataSending = 3;
                   fQueue.active0dTaskId = taskNum;
           			   fQueue.delay = 1;
      	 	         fQueue.delayTime = nextTime(sysTime, 0, 15);   //延时15秒后再发下一帧
      	 	                        
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("AFN0D主动上报(任务号=%d):第二次发送本帧,长度=%d\n",taskNum ,frameTail0d+2);
                   }
       				     
       				     memcpy(tmpDataBuff,bakDataBuff,frameTail0d+2);
           			   sendToMs(tmpDataBuff, frameTail0d+2);
           			   
           			   sendDebugFrame(tmpDataBuff, frameTail0d+2);
    
                   #ifdef RECORD_LOG
                    strcpy(logStr,"");
                    sprintf(logStr,"AFN0D主动上报(任务号=%d):第二次发送本帧,长度=%d:",taskNum,frameTail0d+2);
                    for(i=0;i<frameTail0d+2;i++)
                    { 
                    	strcpy(sayStr,"");
                    	sprintf(sayStr,"%02x ",tmpDataBuff[i]);
                    	strcat(logStr,sayStr);
                    }
                    logRun(logStr);
                   #endif
      	 	       }
                 
                 usleep(500000);
                 continue;
           		}
           		
           		//发送第二次后的等待
           		if (fQueue.active0dDataSending==3)
           		{
                 usleep(500000);
                 continue;
           		}
           		
           		//超时或收到主站确认
           		if (fQueue.active0dDataSending>3)
           		{
           			 fQueue.active0dDataSending = 0;
           			 fQueue.delay = 0;
           			 fQueue.active0dTaskId = 0;
           			 thisThreadSending = 0;
           			 frameTail0d = 14;
           			 
           			 if (debugInfo&PRINT_ACTIVE_REPORT)
           			 {
                   printf("AFN0D主动上报(任务号=%d):继续\n",taskNum);
                 }
           		}
            }
            
            usleep(500000);
            continue;
          }
      	}
      	
      	tmpDtCount++;
        fn = 0;
        if ((tmpDt1 & 0x1) == 0x1)
        {
        	fn = *(pDataHead+3)*8 + tmpDtCount;
        	
        	if (debugInfo&PRINT_ACTIVE_REPORT)
        	{
        	  printf("AFN0D=%d\n", fn);
        	}

          //执行函数
          if (fn <= 221)
          {
            if (AFN0DFun[fn-1] != NULL)
            //2013-11-27,在甘肃测试发现集中器二类数据不上报,甘肃自定义了数据项AFN0D-F247
            //           引起二类数据任务上送失败,经观察还发现会异常退出运行
            //       修改为以上2个判断,以防止数组越界而出现段错误
            //           
            //if ((AFN0DFun[fn-1] != NULL) && (fn <= 221))
            {
              tmpFrameTail = AFN0DFun[fn-1](tmpDataBuff, frameTail0d, pDataHead, fn, &offset0d);
              if (tmpFrameTail == frameTail0d)
              {
           	    ackData[ackTail*5]   = *pDataHead;                         //DA1
           	    ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
           	    ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
           	    ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
           	    ackData[ackTail*5+4] = 0x03;                               //无有效数据
           	    ackTail++;
              }
              else
              {
        			  frameTail0d = tmpFrameTail;
              }
            }
          }
          else
          {
          	//2013-11-27,甘肃数据不上送的又一原因是这个else里面的offset0d没有赋值
          	//           造成本次AFN0D调用一直退不出while,则threadOfReport2一直处于挂起状态
          	//           最终不断的在那个数组里面直到数据越界而退出程序运行
          	offset0d = 3;
          }
        }
        tmpDt1 >>= 1;

        if (frameTail0d>MAX_OF_PER_FRAME || ((pDataHead+offset0d+4)== pDataEnd && tmpDtCount==8))
        {
        	//不允许主动上报且有事件发生
          if (frame.acd==1 && (callAndReport&0x03)== 0x02 && frameTail0d > 16)
          {
     	      tmpDataBuff[frameTail0d++] = iEventCounter;
     	      tmpDataBuff[frameTail0d++] = nEventCounter;
          }

          //根据启动站要求判断是否携带TP
          if (frame.pTp != NULL)
          {
            //ly,2011-10-11,修改这个主动上报Tp的bug(if为true的处理)
            if (poll==ACTIVE_REPORT)
            {
              tmpDataBuff[frameTail0d++] = pfc++;
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.second);
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.minute);
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.hour);
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.day);
              tmpDataBuff[frameTail0d++] = 0x0;
            }
            else
            {
              pTpv = frame.pTp;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv;
            }
          }
                             
          tmpDataBuff[0] = 0x68;          //帧起始字符
          
          if (poll==ACTIVE_REPORT)
          {
            //tmpI = ((frameTail0d - 6) << 2) | 0x01;   //积成主站用这句 2010.06.17
            
            tmpI = ((frameTail0d - 6) << 2) | PROTOCOL_FIELD;
          }
          else
          {
            tmpI = ((frameTail0d - 6) << 2) | PROTOCOL_FIELD;
          }
          tmpDataBuff[1] = tmpI & 0xFF;   //L
          tmpDataBuff[2] = tmpI >> 8;
          tmpDataBuff[3] = tmpI & 0xFF;   //L
          tmpDataBuff[4] = tmpI >> 8; 
           
          tmpDataBuff[5] = 0x68;          //帧起始字符
          
          if (poll==ACTIVE_REPORT)
       	  {
       	   	tmpDataBuff[6] = 0xc4;        //DIR=1,PRM=1,功能码=0x4
       	  }
          else
          {
           	tmpDataBuff[6] = 0x88;        //控制字符10001000(DIR=1,PRM=0,功能码=0x8)
          }

          if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
          {
            tmpDataBuff[6] |= 0x20;
          }
     
          //地址
          tmpDataBuff[7]  = addrField.a1[0];
          tmpDataBuff[8]  = addrField.a1[1];
          tmpDataBuff[9]  = addrField.a2[0];
          tmpDataBuff[10] = addrField.a2[1];
            
          if (poll == ACTIVE_REPORT)
          {
            tmpDataBuff[11] = 0;
          }
          else
          {
            tmpDataBuff[11] = addrField.a3;
          }
          
          if (poll == AFN0B_REQUIRE)
          {
            tmpDataBuff[12] = 0x0B;
          }
          else
          {
            tmpDataBuff[12] = 0x0D;          //AFN
          }
             
          tmpDataBuff[13] = 0x0;
          if (frame.pTp != NULL)
          {
     	      tmpDataBuff[13] |= 0x80;
          }
            
          if (poll == AFN0B_REQUIRE)
          {
            tmpDataBuff[14] = *(frame.pData+2);
            tmpDataBuff[15] = *(frame.pData+3);
            tmpDataBuff[16] = *(frame.pData+4);
            tmpDataBuff[17] = *(frame.pData+5);
          }
          
          //frameTail0d++;
          tmpDataBuff[frameTail0d+1] = 0x16;
           
          if ((poll == AFN0B_REQUIRE && frameTail0d > 22 && frame.pTp==NULL)
          	 ||((poll != AFN0B_REQUIRE && frameTail0d > 23 && frame.pTp ==NULL)
         	   ||(frameTail0d > 29 && frame.pTp!=NULL)))
          {
            if (poll==ACTIVE_REPORT)
            {
           	  tmpDataBuff[13] |= 0x70;       //主动上报帧为单帧且需要对该帧报文进行确认,ly,2010-10-12,add,当前认为是正确的处理
           	  tmpDataBuff[13] |= pSeq&0xf;
           	  pSeq++;
             
              tmpI = 6;
              checkSum = 0;
              while (tmpI < frameTail0d)
              {
                checkSum = tmpDataBuff[tmpI]+checkSum;
                tmpI++;
              }
              tmpDataBuff[frameTail0d] = checkSum;
              
       				//本线程实例有数据要发送
       				thisThreadSending = 1;

       				//ly,2011-10-11,modify
       				// if (fQueue.active0dDataSending==0 && fQueue.delay==0 && fQueue.active0dTaskId==0 && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
       				if (fQueue.active0dDataSending==0  && fQueue.delay==0 && fQueue.inTimeFrameSend==FALSE && fQueue.activeFrameSend==FALSE && fQueue.active0dTaskId==0 && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
       				{

       				  thisThreadSending = TRUE;
       				  fQueue.active0dDataSending = 1;
       				  fQueue.active0dTaskId = taskNum;
       				  
       				  fQueue.delay = 1;
  	 	          fQueue.delayTime = nextTime(sysTime, 0, 15);   //延时15秒后再发下一帧
                
                memcpy(bakDataBuff, tmpDataBuff, frameTail0d+2);
       			    
       			    sendToMs(tmpDataBuff, frameTail0d+2);

       				  if (debugInfo&PRINT_ACTIVE_REPORT)
       				  {
       				    printf("AFN0D主动上报(任务号=%d):组织数据后发送本帧第一次,长度=%d\n",taskNum,frameTail0d+2);
       				  }
                
                #ifdef RECORD_LOG
                 strcpy(logStr,"");
                 sprintf(logStr,"AFN0D主动上报(任务号=%d):组织数据后发送本帧第一次,长度=%d:",taskNum,frameTail0d+2);
                 for(i=0;i<frameTail0d+2;i++)
                 {
                	 strcpy(sayStr,"");
                	 sprintf(sayStr,"%02x ",tmpDataBuff[i]);
                	 strcat(logStr,sayStr);
                 }
                 logRun(logStr);
                #endif
       				}
            }
            else
            {
              if (fQueue.tailPtr == 0)
              {
                 tmpHead0d = 0;
              }
              else
              {
                 tmpHead0d = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
              }
              fQueue.frame[fQueue.tailPtr].head = tmpHead0d;
              fQueue.frame[fQueue.tailPtr].len  = frameTail0d + 2;
              
              //复制数据
              memcpy(&msFrame[fQueue.frame[fQueue.tailPtr].head], tmpDataBuff, fQueue.frame[fQueue.tailPtr].len);

          		if ((tmpHead0d+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
              	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
              {
                fQueue.frame[fQueue.tailPtr].next = 0x0;
          			fQueue.tailPtr = 0;
          			tmpHead0d = 0;
              }
              else
              {
                fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                fQueue.tailPtr++;
              }
           }               

           if (poll == AFN0B_REQUIRE)
           {
              frameTail0d = 18;  //frameTail重新置位填写下一帧
           }
           else
           {
              if (poll!=ACTIVE_REPORT)
              {
                frameTail0d = 14;  //frameTail重新置位填写下一帧
              }
           }
          }
        }
        
        if (poll==ACTIVE_REPORT)
        {
          usleep(1000);
        }
      }
       
      pDataHead += (offset0d + 4);
      if (frameLoadLen < offset0d)
      {
        break;
      }
      else
      {
        frameLoadLen -= (offset0d + 4);
      }
    }
    
    if (ackTail!=0 && poll!=ACTIVE_REPORT)
    {
       AFN00003(ackTail, dataFrom, 0x0D);
    }
    
    if (poll==ACTIVE_REPORT)
    {
      //有测量点没有曲线示值数据(1小时都没有数据了,自诊断是否抄表口硬件问题,复位试试?)
    	if (hasCurveVisionData!=TRUE)
    	{
    	  ifReset = TRUE;
    	}    	 
    }  
}

#ifdef LIGHTING

/*************************************************************
函数名称:AFN0D149
功能描述:响应主站请求二类数据"日冻结当日最后一次开灯时刻"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
*************************************************************/
INT16U AFN0D149(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U               dataBuff[512];
	INT8U               da1, da2;
	INT16U              pn, tmpPn = 0;
	//INT8U               i, tariff;
	//INT8U               queryType, dataType, tmpDay;
	DATE_TIME           tmpTime, readTime;
	INT16U              tmpFrameTail;
	//INT8U               meterInfo[10];
  //BOOL                bufHasData;

	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
      
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 149:	//日冻结正向有功电能示值
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x12;															  //DT2
					break;

				case 150:	//日冻结正向无功电能示值
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x12;																//DT2
					break;
										
				case 151:	//日冻结反向有功电能示值
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x12;																//DT2
					break;
				
				case 152:	//日冻结反向无功电能示值
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x12;																//DT2
					break;
			}
			
			*offset0d = 3;

			//冻结类数据时标
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			buff[frameTail++] = tmpTime.day   = *(pHandle+0);
			buff[frameTail++] = tmpTime.month = *(pHandle+1);
			buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			
			readTime = tmpTime;
      if (readSlDayData(pn, (fn-148), readTime, dataBuff)==TRUE)
			{
				memcpy(&buff[frameTail], &dataBuff[1] ,5);
			}
			else
			{
				memset(&buff[frameTail], 0xee ,5);
			}
		  frameTail += 5;
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D150
功能描述:响应主站请求二类数据"日冻结当日最后一次光调小时刻"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D150(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D149(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D151
功能描述:响应主站请求二类数据"日冻结当日最后一次光调正常时刻"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D151(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D149(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D152
功能描述:响应主站请求二类数据"日冻结当日最后一次关灯时刻"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D152(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D149(buff, frameTail, pHandle, fn, offset0d);
}

#endif


/*******************************************************
函数名称:AFN0D001
功能描述:响应主站请求二类数据"日冻结正向有/无功电能示值，
          一/四象限无功电能示值"命令（数据格式14、11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
	1.2011-08-27,add,三相表如果日冻结示值没有的话,用当天最后一次成功的抄到的数据作为日冻结(仅F1和F2)
  2.2012-05-23,add,单相表如果日冻结示值没有的话,用当天最后一次成功的抄到的数据作为日冻结(仅F1和F2)
  3.2012-05-24,修改,有些单相表(如普光用的华立)回的部分数据为0xff,这明显不是正确的数据,所以当0xEE处理
*******************************************************/
INT16U AFN0D001(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U    pn, tmpPn=0;
  INT8U     tariff, dataType, queryType;
  INT8U     da1, da2;
  INT8U     tmpDay, i, j;
  BOOL      ifHasData, bufHasData;
  INT16U    offset;
  DATE_TIME tmpTime, readTime;
  INT8U     meterInfo[10];
  INT8U     pulsePnData = 0;  

  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
    
  if(da1==0x0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
   	tmpPn++;
   	if((da1 & 0x01) == 0x01)
	  {
	  	pn = tmpPn + (da2 - 1) * 8;
      
      pulsePnData = 0;
      
  		#ifdef PULSE_GATHER
	 	   //查看是否是脉冲采样的数据
	  	 if (fn==1 || fn==2)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	    //是脉冲量的测量点
	  	 	    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	    {
	   	  	 	  pulsePnData = 1;
	   	  	 	  break;
			 	  	}
			 	  }
	  	 }
	  	#endif

		  tmpTime.second = 0x59;
		  tmpTime.minute = 0x59;
		  tmpTime.hour   = 0x23;
		  if ((fn >= 1 && fn <= 2)||(fn >= 9 && fn <= 10))
		  {
		    *offset0d = 3;
		    tmpTime.day   = *(pHandle+0);   //日
		    tmpTime.month = *(pHandle+1);   //月
		    tmpTime.year  = *(pHandle+2);   //年
		  }
		  else
		  {
		    if(fn >= 17 && fn <= 18)
		    {
		  	  *offset0d = 2;
			    tmpDay = monthDays(2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			    tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
		      tmpTime.month = *(pHandle+0);   //月
		      tmpTime.year  = *(pHandle+1);   //年
		    }
		  }
		  
      queryMeterStoreInfo(pn, meterInfo);

		  buff[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
		  buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
		  switch (fn)
		  {
		   	case 1:
		     	buff[frameTail++] = 0x01;																//DT1
			    buff[frameTail++] = 0x00;																//DT2

          switch (meterInfo[0])
          {
    			  case 1:    //单相智能表
    			    queryType = SINGLE_PHASE_DAY;
    	        break;
    	        
    	      case 2:    //单相本地费控表
    	      	queryType = SINGLE_LOCAL_CTRL_DAY;
    	      	break;
    
    	      case 3:    //单相远程费控表
    	      	queryType = SINGLE_REMOTE_CTRL_DAY;
    	      	break;
	      	
      	    case 4:
      	    	queryType = THREE_DAY;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_DAY;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_DAY;
      	      break;

      	    case 8:
      	      queryType = KEY_HOUSEHOLD_DAY;
      	      break;
      	      
      	    default:
			        queryType = DAY_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType  = ENERGY_DATA;
			    }
			    else
			    {
			      dataType  = DAY_FREEZE_COPY_DATA;
			    }
			    break;

			  case 2:
			  	buff[frameTail++] = 0x02;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
          
          switch (meterInfo[0])
          {
    			  case 1:    //单相智能表
    			    queryType = SINGLE_PHASE_DAY;
    	        break;
    	        
    	      case 2:    //单相本地费控表
    	      	queryType = SINGLE_LOCAL_CTRL_DAY;
    	      	break;
    
    	      case 3:    //单相远程费控表
    	      	queryType = SINGLE_REMOTE_CTRL_DAY;
    	      	break;

      	    case 4:
      	    	queryType = THREE_DAY;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_DAY;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_DAY;
      	      break;

      	    case 8:
      	      queryType = KEY_HOUSEHOLD_DAY;
      	      break;
      	      
      	    default:
			        queryType = DAY_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType  = ENERGY_DATA;
			    }
			    else
			    {
			      dataType  = DAY_FREEZE_COPY_DATA;
			    }
			    break;
			    
		   	case 9:
		     	buff[frameTail++] = 0x01;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
			    break;

			  case 10:
			  	buff[frameTail++] = 0x02;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
			    break;
			    
			  case 17:
			   	buff[frameTail++] = 0x01;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
          
          switch (meterInfo[0])
          {
    			  case 1:    //单相智能表
    			    queryType = SINGLE_PHASE_MONTH;
    	        break;
    	        
    	      case 2:    //单相本地费控表
    	      	queryType = SINGLE_LOCAL_CTRL_MONTH;
    	      	break;
    
    	      case 3:    //单相远程费控表
    	      	queryType = SINGLE_REMOTE_CTRL_MONTH;
    	      	break;

      	    case 4:
      	    	queryType = THREE_MONTH;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_MONTH;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_MONTH;
      	      break;

      	    case 8:
      	      queryType = KEY_HOUSEHOLD_MONTH;
      	      break;
      	      
      	    default:
			        queryType = MONTH_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType  = ENERGY_DATA;
			    }
			    else
			    {
			      dataType  = MONTH_FREEZE_COPY_DATA;
			    }
			   	break;

			  case 18:
			   	buff[frameTail++] = 0x02;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	
          switch (meterInfo[0])
          {
    			  case 1:    //单相智能表
    			    queryType = SINGLE_PHASE_MONTH;
    	        break;
    	        
    	      case 2:    //单相本地费控表
    	      	queryType = SINGLE_LOCAL_CTRL_MONTH;
    	      	break;
    
    	      case 3:    //单相远程费控表
    	      	queryType = SINGLE_REMOTE_CTRL_MONTH;
    	      	break;

      	    case 4:
      	    	queryType = THREE_MONTH;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_MONTH;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_MONTH;
      	      break;
      	      
      	    case 8:
      	      queryType = KEY_HOUSEHOLD_MONTH;
      	      break;
      	      
      	    default:
			        queryType = MONTH_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType = ENERGY_DATA;
			    }
			    else
			    {
			      dataType = MONTH_FREEZE_COPY_DATA;
			    }
			   	break;
			}
			
		  ifHasData = FALSE;
		  bufHasData = FALSE;
		  
		  //获得费率个数
		  tariff = numOfTariff(pn);
       
      readTime = tmpTime;
		  bufHasData = readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0);
		  
		  //如果日冻结示值没有的话,用当天最后一次成功的抄到的数据作为日冻结
		  //2011-08-27,add,三相表的处理
		  //2012-05-23,add,单相表(485口或是载波端口)的处理
		  if (bufHasData==FALSE && (fn==1 || fn==2))
		  {
        readTime = tmpTime;
        if (meterInfo[0]>0 && meterInfo[0]<4)
	      {
	      	bufHasData = readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &readTime, 0);
		      if (bufHasData==FALSE)
	      	{
	      		//readTime = timeBcdToHex(tmpTime);
	      		//2012-12-10,修改
	      		readTime = tmpTime;
	      		bufHasData = readBakDayFile(pn, &readTime, dataBuff, 1);
	      	}
	      }
	      else
	      {
		      bufHasData = readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &readTime, 0);
		      
		      if (bufHasData==FALSE)
	      	{
	      		//readTime = timeBcdToHex(tmpTime);
	      		//2012-12-10,修改
	      		readTime = tmpTime;
	      		bufHasData = readBakDayFile(pn, &readTime, dataBuff, DAY_FREEZE_COPY_DATA);
	      	}
		    }
		  }

		  if (bufHasData==TRUE)
		  {
		  	//数据冻结时标--数据格式20,21
		  	if(fn==1 || fn==2 || fn==9 || fn==10)
			  {
			    buff[frameTail++] = tmpTime.day;   //日
			  }
			  buff[frameTail++] = tmpTime.month;   //月
			  buff[frameTail++] = tmpTime.year;    //年
						
			  //终端抄表时间--数据格式15(分时日月年)
			  buff[frameTail++] = readTime.minute;
			  buff[frameTail++] = readTime.hour;
			  buff[frameTail++] = readTime.day;		    
			  buff[frameTail++] = readTime.month;
			  buff[frameTail++] = readTime.year;
				
			  //费率数
			  buff[frameTail++] = tariff;
		  	
		  	//正向有功电能示值(总,费率1~M)--数据格式14
		  	if(fn==1 || fn==9 || fn==17)
		  	{
		  		offset = POSITIVE_WORK_OFFSET;
		  	}
		  	else	//F2,F10,F18
		  	{
		  		if (meterInfo[0]<4)
		  		{
		  		  offset = NEGTIVE_WORK_OFFSET_S;
		  		}
		  		else
		  		{
		  		  offset = NEGTIVE_WORK_OFFSET;
		  		}
		  	}
			 	for(i = 0; i <= tariff; i++)
			 	{
			   	//2012-05-24,add,0xff的处理
			   	if((dataBuff[offset] != 0xEE) && (dataBuff[offset]!=0XFF))
			   	{
			  		ifHasData = TRUE;
			   		
            if (pulsePnData==1)
            {
              buff[frameTail++] = dataBuff[offset++];
            }
            else
            {
              buff[frameTail++] = 0x0;
            }
			  	}
			   	else
			   	{
			  		buff[frameTail++] = 0xEE;
			   	}
			   	
				  //2012-05-24,add,0xff的处理
		   	  if (dataBuff[offset]==0xff)
			   	{
			   		buff[frameTail++] = 0xee;
			   		buff[frameTail++] = 0xee;
			   		buff[frameTail++] = 0xee;
			   		buff[frameTail++] = 0xee;
			   	}
			   	else
			   	{
			   	  buff[frameTail++] = dataBuff[offset++];
			   	  buff[frameTail++] = dataBuff[offset++];
			   	  buff[frameTail++] = dataBuff[offset++];
			   	  buff[frameTail++] = dataBuff[offset++];
			   	}
			   	
			   	//单费率的处理,费率1与总一样
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		i++;
			   	}
			 	}

        if ((fn==1 || fn==2 || fn==17 || fn==18) && (meterInfo[0]==8 || meterInfo[0]<4))
        {
        	 memset(&buff[frameTail],0xee,(tariff+1)*12);
        	 frameTail += (tariff+1)*12;
        }
        else
        {
  			  //正向无功电能示值(总,费率1~M)--数据格式11
  			  if(fn==1 || fn==9 || fn==17)
  		  	{
  		  		offset = POSITIVE_NO_WORK_OFFSET;
  		  	}
  		  	else	//F2,F10,F18
  		  	{
  		  		offset = NEGTIVE_NO_WORK_OFFSET;
  		  	}
  			  for(i = 0; i <= tariff; i++)
  			  {
  			   	if((dataBuff[offset]!=0xEE) && (dataBuff[offset]!=0xFF))
  			  	{
  			  		ifHasData = TRUE;
  			   	}
  			   	
            if (pulsePnData==1)
            {
            	offset++;
            }
  			   	
  			   	if (dataBuff[offset]==0xff)
  			   	{
  			   		buff[frameTail++] = 0xee;
  			   		buff[frameTail++] = 0xee;
  			   		buff[frameTail++] = 0xee;
  			   		buff[frameTail++] = 0xee;
  			   	}
  			   	else
  			   	{
  			   	  buff[frameTail++] = dataBuff[offset++];
  			   	  buff[frameTail++] = dataBuff[offset++];
  			  	  buff[frameTail++] = dataBuff[offset++];
  			   	  buff[frameTail++] = dataBuff[offset++];
  			   	}
  
  			   	//单费率的处理,费率1与总一样
  			   	if (tariff==1)
  			   	{
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		i++;
  			   	}
  			  }
  			      	
  			  //一象限无功电能示值(总,费率1~M)--数据格式11
  			  if(fn == 1 || fn == 9 || fn == 17)
  		  	{
  		  		offset = QUA1_NO_WORK_OFFSET;
  		  	}
  		  	else	//F2,F10,F18
  		  	{
  		  		offset = QUA2_NO_WORK_OFFSET;
  		  	}
  			  for(i = 0; i <= tariff; i++)
  			  {
  			   	if(dataBuff[offset] != 0xEE)
  			   	{
  			   		ifHasData = TRUE;
  			   	}
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  
  			   	//单费率的处理,费率1与总一样
  			   	if (tariff==1)
  			   	{
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		i++;
  			   	}
  			  }
  			      	
  			  //四象限无功电能示值(总,费率1~M)--数据格式11
  			  if(fn == 1 || fn == 9 || fn == 17)
  		  	{
  		  		offset = QUA4_NO_WORK_OFFSET;
  		  	}
  		  	else	//F2,F10,F18
  		  	{
  		  		offset = QUA3_NO_WORK_OFFSET;
  		  	}
  			  for(i = 0; i <= tariff; i++)
  			  {
  			   	if(dataBuff[offset] != 0xEE)
  			   	{
  			   		ifHasData = TRUE;
  			   	}
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  
  			   	//单费率的处理,费率1与总一样
  			   	if (tariff==1)
  			   	{
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		i++;
  			   	}
  			  }
  			      	
  			  #ifdef NO_DATA_USE_PART_ACK_03
  			    if (ifHasData == FALSE)
  			  	{
  			  		if((fn >= 1 && fn <= 2) ||(fn >= 9 && fn <= 10))
  			  		{
  			  			frameTail -= 13 + (tariff + 1) * 17;
  			  		}
  			  		else
  			  		{
  			  			frameTail -= 12 + (tariff + 1) * 17;
  			  		}
  			   	}
  			 	#endif
  			}
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail -= 4;
			  #else
				  if ((fn >= 1 && fn <= 2) || (fn >= 9 && fn <= 10))
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
							
				  //终端抄表时间--数据格式15(分时日月年)
				  buff[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
				  buff[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
				  buff[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
				  buff[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
				  buff[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
				  //费率数
				  buff[frameTail++] = tariff;
				  
			    for(i = 0; i < (tariff + 1) * 17; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
	  }
    da1 >>= 1;
  }
    
  return frameTail;
}

/*******************************************************
函数名称:AFN0D002
功能描述:响应主站请求二类数据"日冻结反向有/无功电能量示值，
          二/三象限无功电能量示值"命令（数据格式14、11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D002(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D003
功能描述:响应主站请求二类数据"日冻结正向有/无功最大需量
         及发生时间"命令（数据格式23、17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D003(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LENGTH_OF_REQ_RECORD];
	INT16U pn, tmpPn = 0;
  INT8U tariff, dataType, queryType;
  INT8U da1, da2;
  INT8U i, tmpDay;
  BOOL  ifHasData, bufHasData;
  
  INT16U offset;
    
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
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
  		
  		tmpTime.second = 0x59;
	    tmpTime.minute = 0x59;
	    tmpTime.hour   = 0x23;
	    if (fn==3 || fn==4 || fn==11 || fn==12)
	    {
	      *offset0d = 3;
	      tmpTime.day   = *(pHandle+0);   //日
	      tmpTime.month = *(pHandle+1);   //月
	      tmpTime.year  = *(pHandle+2);   //年
	    }
	    else	//F19,F20
	    {
		    *offset0d = 2;
			  tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
	      tmpTime.month = *(pHandle+0);   //月
	      tmpTime.year  = *(pHandle+1);   //年
	    }
	    
	    buff[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
		  buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
		  switch (fn)
		  {
		   	case 3:
		     	buff[frameTail++] = 0x04;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = DAY_FREEZE_COPY_REQ;
			    break;

			  case 4:
			  	buff[frameTail++] = 0x08;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = DAY_FREEZE_COPY_REQ;
			    break;

		   	case 11:
		     	buff[frameTail++] = 0x04;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_REQ;
			    break;

			  case 12:
			  	buff[frameTail++] = 0x08;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_REQ;
			    break;

			  case 19:
			   	buff[frameTail++] = 0x04;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType  = MONTH_FREEZE_COPY_REQ;
			   	break;

			  case 20:
			   	buff[frameTail++] = 0x08;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType  = MONTH_FREEZE_COPY_REQ;
			   	break;
			}
			
		  ifHasData = FALSE;
		  bufHasData = FALSE;
		  
		  //获得费率个数
		  tariff = numOfTariff(pn);
			
			readTime = tmpTime;
		  bufHasData = readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0);
		  
		  if (bufHasData==FALSE && (fn==3 || fn==4))
		  {
		    readTime = tmpTime;
		    bufHasData = readMeterData(dataBuff, pn, LAST_TODAY, REQ_REQTIME_DATA, &readTime, 0);
		    
		    if (bufHasData==FALSE)
	      {
	      	readTime = timeBcdToHex(tmpTime);
	      	bufHasData = readBakDayFile(pn, &readTime, dataBuff, DAY_FREEZE_COPY_REQ);
	      }
		  }
		  
		  if (bufHasData==TRUE)
		  {
		  	//数据冻结时标--数据格式20,21
			  if (fn==3 || fn==4 || fn==11 || fn==12)
			  {
			    buff[frameTail++] = tmpTime.day;   //日
			  }
			  buff[frameTail++] = tmpTime.month;   //月
			  buff[frameTail++] = tmpTime.year;    //年
						
			  //终端抄表时间--数据格式15(分时日月年)
			  buff[frameTail++] = readTime.minute;
			  buff[frameTail++] = readTime.hour;
			  buff[frameTail++] = readTime.day;		    
			  buff[frameTail++] = readTime.month;
			  buff[frameTail++] = readTime.year;
				
			  //费率数
			  buff[frameTail++] = tariff;  
		  	
		  	//正向有功最大需量(总,费率1~M)--数据格式23
		  	if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_POSITIVE_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20  反向有功最大需量(总,费率1~M)--数据格式23
		  	{
		  		offset = REQ_NEGTIVE_WORK_OFFSET;
		  	}
			 	for(i = 0; i <= tariff; i++)
			 	{
			   	if(dataBuff[offset] != 0xEE)
			   	{
			  		ifHasData = TRUE;
			  	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	
			   	//单费率的处理,费率1与总一样
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		i++;
			   	}
			 	}
			      	
			  //正向有功最大需量发生时间(总,费率1~M)--数据格式17
			  if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_TIME_P_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20
		  	{
		  		offset = REQ_TIME_N_WORK_OFFSET;
		  	}
			  for(i = 0; i <= tariff; i++)
			  {
			   	if(dataBuff[offset] != 0xEE)
			  	{
			  		ifHasData = TRUE;
			   	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			  	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];

			   	//单费率的处理,费率1与总一样
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		
			   		i++;
			   	}
			   	else
			   	{
			   	  offset++;
			   	}
			  }
			      	
			  //正向无功最大需量(总,费率1~M)--数据格式23
			  if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_POSITIVE_NO_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20 反向无功最大需量(总,费率1~M)--数据格式23
		  	{
		  		offset = REQ_NEGTIVE_NO_WORK_OFFSET;
		  	}
			  for(i = 0; i <= tariff; i++)
			  {
			   	if(dataBuff[offset] != 0xEE)
			   	{
			   		ifHasData = TRUE;
			   	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];

			   	//单费率的处理,费率1与总一样
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		i++;
			   	}
			  }
			      	
			  //正向无功最大需量发生时间(总,费率1~M)--数据格式17
			  if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_TIME_P_NO_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20 反向无功最大需量发生时间(总,费率1~M)--数据格式17
		  	{
		  		offset = REQ_TIME_N_NO_WORK_OFFSET;
		  	}
			  for(i = 0; i <= tariff; i++)
			  {
			   	if(dataBuff[offset] != 0xEE)
			   	{
			   		ifHasData = TRUE;
			   	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	
			   	//单费率的处理,费率1与总一样
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		
			   		i++;
			   	}
			   	else
			   	{
			   	  offset++;
			   	}
			  }
			      	
			  #ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		if(fn==3 || fn==4 || fn==11 || fn==12)
			  		{
			  			frameTail -= 13 + (tariff + 1) * 14;
			  		}
			  		else
			  		{
			  			frameTail -= 12 + (tariff + 1) * 14;
			  		}
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail -= 4;
			  #else
			  	if(fn==3 || fn==4 || fn==11 || fn==12)
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
							
				  //终端抄表时间--数据格式15(分时日月年)
				  buff[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
				  buff[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
				  buff[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
				  buff[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
				  buff[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
				  //费率数
				  buff[frameTail++] = tariff;
				  
			    for(i = 0; i < (tariff + 1) * 14; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
  	}
  	da1 >>= 1;
  }

	return frameTail;
}

/*******************************************************
函数名称:AFN0D004
功能描述:响应主站请求二类数据"日冻结反向有/无功最大需量
          及发生时间"命令（数据格式23、17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D004(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D005
功能描述:响应主站请求二类数据"日冻结正向有功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D005(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U  dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U  tariff, dataType, queryType;
	INT8U  da1, da2;
	INT8U  i, tmpDay;
	INT16U offset;
	
  BOOL ifHasData;
  
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		tmpTime.second = 0x59;
	    tmpTime.minute = 0x59;
	    tmpTime.hour   = 0x23;
	    if (fn >= 5 && fn <= 8)	//日冻结
	    {
	      *offset0d = 3;
	      tmpTime.day   = *(pHandle+0);   //日
	      tmpTime.month = *(pHandle+1);   //月
	      tmpTime.year  = *(pHandle+2);   //年
	    }
	    else	//f21~f24 月冻结
	    {
		    *offset0d = 2;
			  tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
	      tmpTime.month = *(pHandle+0);   //月
	      tmpTime.year  = *(pHandle+1);   //年
	    }
	    
	    buff[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
		  buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
		  switch (fn)
		  {
		   	case 5:
		     	buff[frameTail++] = 0x10;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_P_WORK_OFFSET;
			    break;
			    
			  case 6:
			  	buff[frameTail++] = 0x20;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_P_NO_WORK_OFFSET;
			    break;

			  case 7:
			   	buff[frameTail++] = 0x40;																//DT1
			   	buff[frameTail++] = 0x00;																//DT2
			   	queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_N_WORK_OFFSET;
			   	break;

			  case 8:
			   	buff[frameTail++] = 0x80;																//DT1
			   	buff[frameTail++] = 0x00;																//DT2
			   	queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_N_NO_WORK_OFFSET;
			   	break;

			  case 21:
		     	buff[frameTail++] = 0x10;																//DT1
			    buff[frameTail++] = 0x02;																//DT2
			    queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_P_WORK_OFFSET;
			    break;

			  case 22:
			  	buff[frameTail++] = 0x20;																//DT1
			    buff[frameTail++] = 0x02;																//DT2
			    queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_P_NO_WORK_OFFSET;
			    break;

			  case 23:
			   	buff[frameTail++] = 0x40;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_N_WORK_OFFSET;
			   	break;

			  case 24:
			   	buff[frameTail++] = 0x80;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_N_NO_WORK_OFFSET;
			   	break;
			}
			
		  ifHasData = FALSE;
		  
		  //获得费率个数
		  tariff = numOfTariff(pn);
			
			readTime = tmpTime;      
		  if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
		  {
		  	//数据冻结时标--数据格式20,21
			  if (fn >= 5 && fn <= 8)
			  {
			    buff[frameTail++] = tmpTime.day;   //日
			  }
			  buff[frameTail++] = tmpTime.month;   //月
			  buff[frameTail++] = tmpTime.year;    //年
				
			  //费率数
			  buff[frameTail++] = tariff;  
		  	

			 	for(i = 0; i <= tariff; i++)
			 	{			   
			   #ifdef DKY_SUBMISSION    //2012-09-20,add
      	  if (dataBuff[offset] != 0xEE)
      	  {
      	   	ifHasData = TRUE;
      	    
      	    buff[frameTail++] = dataBuff[offset+1];
      	    buff[frameTail++] = dataBuff[offset+2];
    	      buff[frameTail++] = dataBuff[offset+3];
    	      buff[frameTail++] = dataBuff[offset+4];
    	    }
    	    else
    	    {
    	    	buff[frameTail++] = 0x00;
    	    	buff[frameTail++] = 0x00;
    	    	buff[frameTail++] = 0x00;
    	    	buff[frameTail++] = 0x00;
    	    }
			   #else
      	  if (dataBuff[offset] != 0xEE)
      	  {
      	   	ifHasData = TRUE;
      	  }
      	  buff[frameTail++] = dataBuff[offset+1];
      	  buff[frameTail++] = dataBuff[offset+2];
    	    buff[frameTail++] = dataBuff[offset+3];
    	    buff[frameTail++] = dataBuff[offset+4];
    	   #endif
    	    
    	    //单费率的处理
    	    if (tariff==1)
    	    {
    	    	 buff[frameTail++] = buff[frameTail-4];
    	    	 buff[frameTail++] = buff[frameTail-4];
    	    	 buff[frameTail++] = buff[frameTail-4];
    	    	 buff[frameTail++] = buff[frameTail-4];

    	    	 i++;
    	    }
    	    else
    	    { 
    	      offset += 7;
    	    }
    	 }
			 	
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		if(fn >= 5 && fn <= 8)
			  		{
			  			frameTail -= 8 + (tariff + 1) * 4;
			  		}
			  		else
			  		{
			  			frameTail -= 7 + (tariff + 1) * 4;
			  		}
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail -= 4;
			  #else
				  if (fn >= 5 && fn <= 8)
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
					
				  //费率数
				  buff[frameTail++] = tariff;
				  
			    for(i = 0; i < (tariff + 1) * 4; i++)
			    {
			   	  //2012-09-20,add
			   	  #ifdef DKY_SUBMISSION
			   	   buff[frameTail++] = 0x00;
			   	  #else
			   	   buff[frameTail++] = 0xee;
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
函数名称:AFN0D006
功能描述:响应主站请求二类数据"日冻结正向无功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D006(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 	 *offset0d = 7;	 
	 return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D007
功能描述:响应主站请求二类数据"日冻结正向无功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D007(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 	 *offset0d = 7;
	 return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D008
功能描述:响应主站请求二类数据"日冻结正向无功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D008(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 	 *offset0d = 7;
	 return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D009
功能描述:响应主站请求二类数据"抄表日冻结正向有\无功电能示值
         一\四象限无功电能量示值"命令（数据格式14、11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D009(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D010
功能描述:响应主站请求二类数据"抄表日冻结反向有\无功电能示值
         二\三象限无功电能量示值"命令（数据格式14、11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D010(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D011
功能描述:响应主站请求二类数据"抄表日冻结正向有\无功最
         大需量及发生时间"命令（数据格式23、17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D011(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D012
功能描述:响应主站请求二类数据"抄表日冻结反向有\无功最
         大需量及发生时间"命令（数据格式23、17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D012(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D017
功能描述:响应主站请求二类数据"月冻结正向有/无功电能量示值，
          一/四象限无功电能量示值"命令（数据格式14、11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D017(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D018
功能描述:响应主站请求二类数据"月冻结反向有/无功电能量示值，
          二/三象限无功电能量示值"命令（数据格式14、11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D018(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D019
功能描述:响应主站请求二类数据"月冻结正向有\无功最大需
         量及发生时间"命令（数据格式23、17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D019(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D020
功能描述:响应主站请求二类数据"月冻结反向有\无功最大需
         量及发生时间"命令（数据格式23、17）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D020(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D021
功能描述:响应主站请求二类数据"月冻结正向有功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D021(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D022
功能描述:响应主站请求二类数据"月冻结正向有功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D022(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D023
功能描述:响应主站请求二类数据"月冻结正向有功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D023(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D024
功能描述:响应主站请求二类数据"月冻结正向有功电能量"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D024(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
函数名称:AFN0D025
功能描述:响应主站请求二类数据"日冻结日总及分相最大有功功率及发生时间
          有功功率为零时间"命令（数据格式23、18）
调用函数:AFN0D033
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN0D025(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT8U dataType, queryType;
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U i, tmpDay;
  BOOL ifHasData;
  
  INT16U tmpFrameTail;
    
  DATE_TIME tmpTime, readTime;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		tmpTime.second = 0x59;
  		tmpTime.minute = 0x59;
  		tmpTime.hour = 0x23;
  		if(fn == 25)
  		{
  			tmpTime.day   = *(pHandle+0);		//日
  			tmpTime.month = *(pHandle+1);		//月
  			tmpTime.year  = *(pHandle+2);		//年
  		}
  		else
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 25)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x01;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F33
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x01;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//日冻结类数据时标  F25:Td_d(日月年) F33:Td_m(月年)
  			if (fn==25)
  			{
  			  buff[frameTail++] = tmpTime.day;
  			}
  			buff[frameTail++] = tmpTime.month;
  			buff[frameTail++] = tmpTime.year;
  			
  			//三相总最大有功功率
  			if(dataBuff[MAX_TOTAL_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER + 2];
  			
  			//三相总最大有功功率发生时间
  			if(dataBuff[MAX_TOTAL_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER_TIME + 2];
  			  			
  			//A相最大有功功率
  			if(dataBuff[MAX_A_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_A_POWER];
  			buff[frameTail++] = dataBuff[MAX_A_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_A_POWER + 2];
  			
  			//A相最大有功功率发生时间
  			if(dataBuff[MAX_A_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_A_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_A_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_A_POWER_TIME + 2];
  			
  			//B相最大有功功率
  			if(dataBuff[MAX_B_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_B_POWER];
  			buff[frameTail++] = dataBuff[MAX_B_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_B_POWER + 2];
  			
  			//B相最大有功功率发生时间
  			if(dataBuff[MAX_B_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_B_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_B_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_B_POWER_TIME + 2];
  			
  			//C相最大有功功率
  			if(dataBuff[MAX_C_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_C_POWER];
  			buff[frameTail++] = dataBuff[MAX_C_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_C_POWER + 2];
  			
  			//C相最大有功功率发生时间
  			if(dataBuff[MAX_C_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_C_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_C_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_C_POWER_TIME + 2];
  			
  			//三相总有功功率为零时间
  			if(dataBuff[TOTAL_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  buff[frameTail++] = dataBuff[TOTAL_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[TOTAL_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
          buff[frameTail++] = 0x0;
          buff[frameTail++] = 0x0;
  			}
  			
  			//A相有功功率为零时间
  			if(dataBuff[A_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  
  			  buff[frameTail++] = dataBuff[A_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[A_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
          buff[frameTail++] = 0x0;
          buff[frameTail++] = 0x0;
  			}
  			
  			//B相有功功率为零时间
  			if(dataBuff[B_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  
  			  buff[frameTail++] = dataBuff[B_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[B_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
  				buff[frameTail++] = 0x0;
  				buff[frameTail++] = 0x0;
  			}
  			
  			//C相有功功率为零时间
  			if(dataBuff[C_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  
  			  buff[frameTail++] = dataBuff[C_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[C_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
  				buff[frameTail++] = 0x0;
  				buff[frameTail++] = 0x0;
  			}
  			
  			#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		frameTail = tmpFrameTail;
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 25)
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
				  
			    for(i = 0; i < 32; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D026
功能描述:响应主站请求二类数据"日总及分相最大需量及发生时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D026(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U dataType, queryType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
	DATE_TIME tmpTime, readTime;
	
	INT16U tmpFrameTail;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 8)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 26)
			{
				tmpTime.day   = *(pHandle+0);
				tmpTime.month = *(pHandle+1);
				tmpTime.year  = *(pHandle+2);
			}
			else
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));			//DA!
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 26)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x02;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F34
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x02;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//冻结类数据时标Td_d Td_m
  			if(fn == 26)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				
				//三相总有功最大需量
				if(dataBuff[MAX_TOTAL_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ + 2];
				
				//三相总有功最大需量发生时间
				if(dataBuff[MAX_TOTAL_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ_TIME + 2];
				
				//A相有功最大需量
				if(dataBuff[MAX_A_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_A_REQ];
				buff[frameTail++] = dataBuff[MAX_A_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_A_REQ + 2];
				
				//A相有功最大需量发生时间
				if(dataBuff[MAX_A_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_A_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_A_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_A_REQ_TIME + 2];
				
				//B相有功最大需量
				if(dataBuff[MAX_B_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_B_REQ];
				buff[frameTail++] = dataBuff[MAX_B_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_B_REQ + 2];
				
				//B相有功最大需量发生时间
				if(dataBuff[MAX_B_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_B_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_B_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_B_REQ_TIME + 2];
				
				//C相有功最大需量
				if(dataBuff[MAX_C_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_C_REQ];
				buff[frameTail++] = dataBuff[MAX_C_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_C_REQ + 2];
				
				//C相有功最大需量发生时间
				if(dataBuff[MAX_C_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_C_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_C_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_C_REQ_TIME + 2];
				
  			#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		frameTail = tmpFrameTail;
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 26)
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
				  
			    for(i = 0; i < 24; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D027
功能描述:响应主站请求二类数据"日电压统计数据"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D027(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     queryType, dataType;
	INT8U     tmpFrameTail, i, tmpDay;
	BOOL      ifHasData;	
	DATE_TIME tmpTime, readTime;
	INT16U    offset;
	INT16U    tmpData;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 27)
			{
				tmpTime.day   = *(pHandle+0);
				tmpTime.month = *(pHandle+1);
				tmpTime.year  = *(pHandle+2);
			}
			else		//F35
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));			//DA!
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 27)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x04;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F35
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x04;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//冻结类数据时标Td_d Td_m
  			if(fn == 27)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				
				//A相,B相,C相电压
				offset = VOL_A_UP_UP_TIME;
				for(i = 0; i < 3; i++)
				{
					//越上上限日累计时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x0;
					  buff[frameTail++] = 0x0;
					  offset += 2;
					}
					
					//越下下限日累计时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
					
					//越上限日累计时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
					
					//越下限日累计时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
					
					//合格日累计时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
				}
				
				//A相,B相,C相电压
				offset = VOL_A_MAX;
				for(i = 0; i < 3; i++)
				{
					//电压最大值
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//电压最大值发生时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//电压最小值
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//电压最小值发生时间
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					offset += 4;
				}
				
        offset = VOL_A_AVER;
        for (i = 0; i < 3; i++)
        {
            tmpData = hexToBcd(dataBuff[offset]|dataBuff[offset+1]<<8);
            buff[frameTail++] = tmpData&0xFF;
            buff[frameTail++] = tmpData>>8&0xFF;
            
            offset += 14;
        }
          				
				#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		frameTail = tmpFrameTail;
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 27)
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
				  
			    for(i = 0; i < 66; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
  return frameTail;
}

/*************************************************************
函数名称:AFN0D028
功能描述:响应主站请求二类数据"日不平衡度越限累计时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D028(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType, dataType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
	DATE_TIME tmpTime, readTime;
  
  INT16U tmpFrameTail, offset;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			if(fn == 28)
			{
				tmpTime.day   = *(pHandle+0);    //日
				tmpTime.month = *(pHandle+1);    //月
				tmpTime.year  = *(pHandle+2);    //年
			}
			else		//F36
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));			//DA!
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 28)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x08;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F36
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x08;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//冻结类数据时标Td_d Td_m
  			if(fn == 28)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				
				//电流不平衡越限日累计时间
				if (dataBuff[CUR_UNBALANCE_TIME]!=0xee && dataBuff[CUR_UNBALANCE_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[CUR_UNBALANCE_TIME];
				  buff[frameTail++] = dataBuff[CUR_UNBALANCE_TIME+1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
					
				//电压不平衡越限日累计时间
				if (dataBuff[VOL_UNBALANCE_TIME]!=0xee && dataBuff[VOL_UNBALANCE_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[VOL_UNBALANCE_TIME];
				  buff[frameTail++] = dataBuff[VOL_UNBALANCE_TIME+1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
				
				//电流不平衡最大值
				buff[frameTail++] = dataBuff[CUR_UNB_MAX];
				buff[frameTail++] = dataBuff[CUR_UNB_MAX+1];

				//电流不平衡最大值发生日期
				buff[frameTail++] = dataBuff[CUR_UNB_MAX_TIME];
				buff[frameTail++] = dataBuff[CUR_UNB_MAX_TIME+1];
				buff[frameTail++] = dataBuff[CUR_UNB_MAX_TIME+2];
				if (fn==36)
				{
					 buff[frameTail++] = tmpTime.month;
				}

				//电压不平衡最大值
				buff[frameTail++] = dataBuff[VOL_UNB_MAX];
				buff[frameTail++] = dataBuff[VOL_UNB_MAX+1];

				//电压不平衡最大值发生日期
				buff[frameTail++] = dataBuff[VOL_UNB_MAX_TIME];
				buff[frameTail++] = dataBuff[VOL_UNB_MAX_TIME+1];
				buff[frameTail++] = dataBuff[VOL_UNB_MAX_TIME+2];
				if (fn==36)
				{
					buff[frameTail++] = tmpTime.month;
				}
			}
			else
			{
				if (fn == 28)
				{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				  
			  for(i = 0; i < 14; i++)
			  {
			    buff[frameTail++] = 0xee;
			  }
			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D029
功能描述:响应主站请求二类数据"日电流越限统计"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D029(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType, dataType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
	DATE_TIME tmpTime, readTime;
  
  INT16U tmpFrameTail, offset;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 29)
			{
				tmpTime.day   = *(pHandle+0);    //日
				tmpTime.month = *(pHandle+1);    //月
				tmpTime.year  = *(pHandle+2);    //年
			}
			else		//F37
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 29)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x10;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F37
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x10;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//冻结类数据时标Td_d Td_m
  			if(fn == 29)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				
				//A,B,C相电流
				offset = CUR_A_UP_UP_TIME;
				for(i = 0; i < 3; i++)
				{
					//电流越上上限累计时间
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//电流越上限累计时间
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					offset += 6;
				}
				
				//零序电流越上限累计时间
				buff[frameTail++] = dataBuff[offset++];
				buff[frameTail++] = dataBuff[offset++];
				
				//A,B,C相电流，零序电流
				offset = CUR_A_MAX;
				for(i = 0; i < 4; i++)
				{
					//电流最大值
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
				 
					buff[frameTail++] = dataBuff[offset++];
					
					//最大值发生时间
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					offset += 4;
				}
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 29)
				  {
				    buff[frameTail++] = tmpTime.day;   //日
				  }
				  buff[frameTail++] = tmpTime.month;   //月
				  buff[frameTail++] = tmpTime.year;    //年
				  
			    for(i = 0; i < 38; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
 		}
 		da1 >>= 1;
 	}
 	
 	return frameTail;
}

/*************************************************************
函数名称:AFN0D030
功能描述:响应主站请求二类数据"日视在功率越限累计时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D030(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U dataType, queryType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 30)
			{
				tmpTime.day   = *(pHandle+0);   //日
				tmpTime.month = *(pHandle+1);   //月
				tmpTime.year  = *(pHandle+2);   //年
			}
			else		//F38
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 30)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x20;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F38
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x20;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//冻结类数据时标Td_d Td_m
  			if(fn == 30)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				
				//视在功率越上上限累计时间
				if (dataBuff[APPARENT_POWER_UP_UP_TIME]!=0xee && dataBuff[APPARENT_POWER_UP_UP_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_UP_TIME];
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_UP_TIME + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
				
				//视在功率越上限累计时间
				if (dataBuff[APPARENT_POWER_UP_TIME]!=0xee && dataBuff[APPARENT_POWER_UP_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_TIME];
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_TIME + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
        	frameTail -= 4;
        #else
        	//冻结类数据时标Td_d Td_m
	  			if(fn == 30)
	  			{
					  buff[frameTail++] = tmpTime.day;   //日
					}
					buff[frameTail++] = tmpTime.month;   //月
					buff[frameTail++] = tmpTime.year;    //年
          for(i=0;i<4;i++)
          {
          	buff[frameTail++] = 0xee;
          }
        #endif
			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D031
功能描述:响应主站请求二类数据"日负载率统计"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D031(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   	//ly,10-12-17,为兰州供电公司测试做的,可能需要修正
   	
   	INT8U  dataBuff[LEN_OF_PARA_BALANCE_RECORD];
  	INT16U pn, tmpPn = 0;
  	INT8U  da1, da2;
  	INT8U  dataType, queryType;
  	INT8U  i, tmpDay;
  	BOOL  ifHasData;
  	
    DATE_TIME tmpTime, readTime;
    
    da1 = *pHandle++;
    da2 = *pHandle++;
    pHandle += 2;
    
    if(da1 == 0x0)
    {
    	return frameTail;
    }
    
    while(tmpPn < 9)
   	{
   		tmpPn++;
   		if((da1 & 0x1) == 0x1)
   		{
   			pn = tmpPn + (da2 - 1) * 8;
   			
   			tmpTime.second = 0x59;
  			tmpTime.minute = 0x59;
  			tmpTime.hour   = 0x23;
  			if(fn == 31)
  			{
  				tmpTime.day   = *(pHandle+0);   //日
  				tmpTime.month = *(pHandle+1);   //月
  				tmpTime.year  = *(pHandle+2);   //年
  				
  				*offset0d = 3;
  			}
  			else		//F39
    		{
    			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
  		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
  			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
  			  tmpTime.month = *(pHandle+0);		//月
  			  tmpTime.year  = *(pHandle+1);		//年
  				*offset0d = 2;
    		}
    		
    		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
    		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  			if(fn == 31)
  			{
    		  buff[frameTail++] = 0x40;																	//DT1
    		  buff[frameTail++] = 0x03;																	//DT2
				  buff[frameTail++] = tmpTime.day;   //日
				}
				else  //F39
				{
    		  buff[frameTail++] = 0x40;																	//DT1
    		  buff[frameTail++] = 0x04;																	//DT2
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
    		
    		if (fn==31)
    		{
    		  memset(&buff[frameTail], 0x0, 10);
    		  frameTail += 10;
    		}
    		else
    		{
    		  memset(&buff[frameTail], 0x0, 12);
    		  frameTail += 12;
    		}
    		
    	}
    	da1 >>= 1;
    }
  	
   return frameTail;
}

/*************************************************************
函数名称:AFN0D032
功能描述:响应主站请求二类数据"日电能表断相数据"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D032(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U i;
	
	DATE_TIME tmpTime, readTime;
  
  *offset0d = 3;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
 			tmpTime.minute = 0x59;
 			tmpTime.hour   = 0x23;
			tmpTime.day    = *(pHandle+0);   //日
			tmpTime.month  = *(pHandle+1);   //月
			tmpTime.year   = *(pHandle+2);   //年
 			
 			buff[frameTail++] = 0x01 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
 			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
 		  if (fn==31)
 		  {
 			  buff[frameTail++] = 0x40;				  												//DT1
 		  }
 		  else
 		  {	
 			  buff[frameTail++] = 0x80;																	//DT1
 			}
 			buff[frameTail++] = 0x03;																		//DT2
 			
 			readTime = tmpTime;
 			if(readMeterData(dataBuff, pn, DAY_BALANCE, DAY_BALANCE_PARA_DATA, &readTime, 0) == TRUE)
 			{
 				//日冻结类数据时标Td_d
 				buff[frameTail++] = tmpTime.day;
 				buff[frameTail++] = tmpTime.month;
 				buff[frameTail++] = tmpTime.year;
 				
 				//终端抄表时间
 				buff[frameTail++] = tmpTime.minute;
 				buff[frameTail++] = tmpTime.hour;
 				buff[frameTail++] = tmpTime.day;
 				buff[frameTail++] = tmpTime.month;
 				buff[frameTail++] = tmpTime.year;
 				
 				//总断相次数
 				buff[frameTail++] = dataBuff[OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_TIMES + 1];
 				
 				//A相断相次数
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_TIMES + 1];
 				
 				//B相断相次数
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_TIMES + 1];
 				
 				//C相断相次数
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_TIMES + 1];
 				
 				//断相累计时间
 				buff[frameTail++] = dataBuff[OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_MINUTES + 2];
 				
 				//A断相累计时间
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_MINUTES + 2];
 				
 				//B断相累计时间
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_MINUTES + 2];
 				
 				//C断相累计时间
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_MINUTES + 2];
 				
 				//最近一次断相起始时刻
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG + 3];
 				
 				//A相最近断相起始时刻
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 0];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 3];
 				
 				//B相最近断相起始时刻
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 0];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 3];
 				
 				//C相最近断相起始时刻
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 0];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 3];
 				
 				//最近一次断相结束时刻
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 0];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 1];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 2];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 3];
 				
 				//A相最近断相结束时刻
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 0];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 1];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 2];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 3];
 				
 				//B相最近断相结束时刻
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 0];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 1];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 2];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 3];
 				
 				//C相最近断相结束时刻
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 0];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 1];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 2];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 3];
 			}
 			else
 			{
 				#ifdef NO_DATA_USE_PART_ACK_03
        	frameTail -= 4;
        #else
        	//日冻结类数据时标Td_d
	 				buff[frameTail++] = tmpTime.day;
	 				buff[frameTail++] = tmpTime.month;
	 				buff[frameTail++] = tmpTime.year;
	 				
	 				//终端抄表时间
	 				buff[frameTail++] = tmpTime.minute;
	 				buff[frameTail++] = tmpTime.hour;
	 				buff[frameTail++] = tmpTime.day;
	 				buff[frameTail++] = tmpTime.month;
	 				buff[frameTail++] = tmpTime.year;
					
          for(i=0;i<52;i++)
          {
          	buff[frameTail++] = 0xee;
          }
        #endif
 			}
 		}
 		da1 >>= 1;
 	}
  
	return frameTail;
}

/*************************************************************
函数名称:AFN0D033
功能描述:响应主站请求二类数据"月总及分相最大有功功率发生时间，
          有功功率为零时间"命令（数据格式23、18）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D033(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D025(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D034
功能描述:响应主站请求二类数据"月总及分相有功最大需量及发生时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D034(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D026(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D035
功能描述:响应主站请求二类数据"月电压统计数据"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D035(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D027(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D036
功能描述:响应主站请求二类数据"月不平衡度越限累计时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D036(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D028(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D037
功能描述:响应主站请求二类数据"月电流越限统计"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D037(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D029(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D038
功能描述:响应主站请求二类数据"月视在功率越限累计时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D038(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D030(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D039
功能描述:响应主站请求二类数据"月负载统计率"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D039(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D031(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D041
功能描述:响应主站请求二类数据"日冻结电容器累计投入时间和次数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D041(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	//无有效数据
  *offset0d = 3;
  return frameTail;
}

/*************************************************************
函数名称:AFN0D042
功能描述:响应主站请求二类数据"日冻结日、月电容器累计补偿的无功电能量"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D042(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  //无有效数据
  *offset0d = 3;
  return frameTail;
}

/*************************************************************
函数名称:AFN0D043
功能描述:响应主站请求二类数据"日冻结日功率因数区段累计时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D043(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U dataType, queryType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 43)
			{
				tmpTime.day   = *(pHandle+0);   //日
				tmpTime.month = *(pHandle+1);   //月
				tmpTime.year  = *(pHandle+2);   //年
			}
			else		//F38
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//月
			  tmpTime.year  = *(pHandle+1);		//年
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 43)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x04;																	//DT1
  			buff[frameTail++] = 0x05;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F44
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x08;																	//DT1
  			buff[frameTail++] = 0x05;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//冻结类数据时标Td_d Td_m
  			if(fn == 43)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
				
				//区段1累计时间
				if (dataBuff[FACTOR_SEG_1]!=0xee && dataBuff[FACTOR_SEG_1+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[FACTOR_SEG_1];
				  buff[frameTail++] = dataBuff[FACTOR_SEG_1 + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}

				//区段2累计时间
				if (dataBuff[FACTOR_SEG_2]!=0xee && dataBuff[FACTOR_SEG_2+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[FACTOR_SEG_2];
				  buff[frameTail++] = dataBuff[FACTOR_SEG_2 + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}

				//区段3累计时间
				if (dataBuff[FACTOR_SEG_3]!=0xee && dataBuff[FACTOR_SEG_3+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[FACTOR_SEG_3];
				  buff[frameTail++] = dataBuff[FACTOR_SEG_3 + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
			}
			else
			{
      	//冻结类数据时标Td_d Td_m
  			if(fn == 43)
  			{
				  buff[frameTail++] = tmpTime.day;   //日
				}
				buff[frameTail++] = tmpTime.month;   //月
				buff[frameTail++] = tmpTime.year;    //年
        for(i=0;i<6;i++)
        {
        	buff[frameTail++] = 0xee;
        }
			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D044
功能描述:响应主站请求二类数据"月冻结月功率因数区段累计时间"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D044(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D043(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D045
功能描述:响应主站请求二类数据"日冻结铜损,铁损有功电能示值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D045(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	//无有效数据
	*offset0d = 3;
  return frameTail;
}

/*************************************************************
函数名称:AFN0D046
功能描述:响应主站请求二类数据"月冻结铜损,铁损有功电能示值"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D046(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	//无有效数据
	*offset0d = 2;
  return frameTail;
}

/*************************************************************
函数名称:AFN0D049
功能描述:响应主站请求二类数据"日冻结终端日供电时间、日复位累计次数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D049(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	INT8U                  i, tmpDay;
	DATE_TIME              tmpTime, bakTime;
	INT8U                  tmpCount;
	INT32U                 tmpMonthReset,tmpMonthPowerOn;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//冻结类数据时标Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour = 0x23;
	if(fn == 49)
	{
		*offset0d = 3;
		buff[frameTail++] = tmpTime.day   = *pHandle++;
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	else		//F51
	{
		*offset0d = 2;
		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
													, (*pHandle >> 4) * 10 + *pHandle % 10);
		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
		
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	
	if(fn == 49)
	{
	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	  {
			//终端日供电时间
			buff[frameTail++] = terminalStatisRecord.powerOnMinute & 0xFF;
			buff[frameTail++] = terminalStatisRecord.powerOnMinute >> 8;
			
			//终端日复位累计次数
			buff[frameTail++] = terminalStatisRecord.resetTimes & 0xFF;
			buff[frameTail++] = terminalStatisRecord.resetTimes >> 8;
	  }
	  else
	  {
		  #ifdef NO_DATA_USE_PART_ACK_03
          frameTail -= 7;
      #else
        for(i = 0; i < 4; i++)
        {
     	    buff[frameTail++] = 0xee;
        }
      #endif
    }
	}
	else
	{
  	bakTime = timeBcdToHex(tmpTime);
  	tmpCount = monthDays(bakTime.year+2000,bakTime.month);
  	tmpMonthReset   = 0;
  	tmpMonthPowerOn = 0;
  	for(i=1; i<=tmpCount; i++)
  	{
  		tmpTime = bakTime;
  		tmpTime.day = i;
  	 	tmpTime = timeHexToBcd(tmpTime);
      if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
      {
        tmpMonthReset   += terminalStatisRecord.resetTimes;
        tmpMonthPowerOn += terminalStatisRecord.powerOnMinute;
      }
  	}
      
		//终端月供电时间
		buff[frameTail++] = tmpMonthPowerOn & 0xFF;
		buff[frameTail++] = tmpMonthPowerOn >> 8;
			
		//终端月复位累计次数
		buff[frameTail++] = tmpMonthReset & 0xFF;
		buff[frameTail++] = tmpMonthReset >> 8;
    
  	/*ly,2011-10-11,修改为上面的写法
  	tmpCount = monthDays(sysTime.year+2000,sysTime.month);
  	tmpMonthReset   = 0;
  	tmpMonthPowerOn = 0;
  	for(i=1;i<=tmpCount && i<=sysTime.day;i++)
  	{
  		 tmpTime = sysTime;
  		 tmpTime.day = i;
  	 	 tmpTime = timeHexToBcd(tmpTime);
       if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
       {
         tmpMonthReset   += terminalStatisRecord.resetTimes;
         tmpMonthPowerOn += terminalStatisRecord.powerOnMinute;
       }
  	}  	
      
    if (tmpMonthReset!=0)
    {
			 //终端月供电时间
			 buff[frameTail++] = tmpMonthPowerOn & 0xFF;
			 buff[frameTail++] = tmpMonthPowerOn >> 8;
			
			 //终端月复位累计次数
			 buff[frameTail++] = tmpMonthReset & 0xFF;
			 buff[frameTail++] = tmpMonthReset >> 8;
		}
	  else
	  {
		   #ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 6;
       #else
        for(i = 0; i < 4; i++)
        {
     	    buff[frameTail++] = 0xee;
        }
       #endif
    }
    */
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D050
功能描述:响应主站请求二类数据"日冻结终端日控制统计数据"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D050(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	if (fn==50)
	{
  	*offset0d = 3;
	}
	else
	{
  	*offset0d = 2;
	}
	
	#ifdef LOAD_CTRL_MODULE
  	TERMINAL_STATIS_RECORD terminalStatisRecord;
  	INT8U                  i, tmpDay;
  	DATE_TIME              tmpTime, bakTime;
  	INT8U                  tmpCount;
  	INT16U                 tmpData[4];
  	
  	buff[frameTail++] = *pHandle++;		//DA1
  	buff[frameTail++] = *pHandle++;		//DA2
  	buff[frameTail++] = *pHandle++;		//DT1
  	buff[frameTail++] = *pHandle++;		//DT2
  	
  	//冻结类数据时标Td_d Td_m
  	tmpTime.second = 0x59;
  	tmpTime.minute = 0x59;
  	tmpTime.hour = 0x23;
  	if(fn == 50)
  	{
  		*offset0d = 3;
  		buff[frameTail++] = tmpTime.day   = *pHandle++;
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	else		//F52
  	{
  		*offset0d = 2;
  		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
  													, (*pHandle >> 4) * 10 + *pHandle % 10);
  		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
  		
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	
  	if(fn == 50)
  	{
  	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  	  {
  			buff[frameTail++] = terminalStatisRecord.monthCtrlJumpedDay;   //月电控跳闸日累计次数
  			buff[frameTail++] = terminalStatisRecord.chargeCtrlJumpedDay;  //购电控跳闸日累计次数
  			buff[frameTail++] = terminalStatisRecord.powerCtrlJumpedDay;   //功控跳闸日累计次数
  			buff[frameTail++] = terminalStatisRecord.remoteCtrlJumpedDay;  //遥控跳闸日累计次数
  	  }
  	  else
  	  {
        for(i = 0; i < 4; i++)
        {
       	   buff[frameTail++] = 0xee;
        }
      }
  	}
  	else
  	{
    	bakTime = timeBcdToHex(tmpTime);
    	tmpCount = monthDays(bakTime.year+2000, bakTime.month);
    	tmpData[0]   = 0;
    	tmpData[1]   = 0;
    	tmpData[2]   = 0;
    	tmpData[3]   = 0;
    	for(i=1;i<=tmpCount;i++)
    	{
    		 tmpTime = bakTime;
    		 tmpTime.day = i;
    	 	 tmpTime = timeHexToBcd(tmpTime);
         if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
         {
           tmpData[0] += terminalStatisRecord.monthCtrlJumpedDay;
           tmpData[1] += terminalStatisRecord.chargeCtrlJumpedDay;
           tmpData[2] += terminalStatisRecord.powerCtrlJumpedDay;
           tmpData[3] += terminalStatisRecord.remoteCtrlJumpedDay;
         }
    	}
        
  		buff[frameTail++] = tmpData[0];
  		buff[frameTail++] = tmpData[1];
  		buff[frameTail++] = tmpData[2];
  		buff[frameTail++] = tmpData[3];
  	}
  #endif

  return frameTail;
}

/*************************************************************
函数名称:AFN0D051
功能描述:响应主站请求二类数据"月冻结终端月供电时间、月复位累计次数"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D051(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D049(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D052
功能描述:响应主站请求二类数据"月冻结终端月电控制统计数据"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D052(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D050(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D053
功能描述:响应主站请求二类数据"终端与主站日通信流量"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D053(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	INT8U  queryType, dataType;
	INT8U  i, tmpDay, tmpCount;
	INT32U tmpData;
	
	DATE_TIME tmpTime,backTime;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//冻结类数据时标Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	if(fn == 53)
	{
		*offset0d = 3;
		buff[frameTail++] = tmpTime.day   = *pHandle++;
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	else		//F54
	{
		*offset0d = 2;
		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
													, (*pHandle >> 4) * 10 + *pHandle % 10);
		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
		
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	
	if(fn == 53)
	{
	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	  {
			tmpData = terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
		  buff[frameTail++] = tmpData&0xff;
		  buff[frameTail++] = tmpData>>8&0xff;
		  buff[frameTail++] = tmpData>>16&0xFF;
		  buff[frameTail++] = tmpData>>32&0xff;
	  }
  	else
  	{
  		#ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 7;
      #else
        for(i = 0; i < 4; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      #endif
  	}
	}
	else  //FN=54
	{
   	backTime = timeBcdToHex(tmpTime);
   	tmpCount = monthDays(backTime.year+2000,backTime.month);
   	tmpData = 0;
   	for(i=1;i<=tmpCount;i++)
   	{
   	  tmpTime = backTime;
   	  tmpTime.day = i;
   	  tmpTime = timeHexToBcd(tmpTime);
      if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
      {
         tmpData += terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
      }
   	}
		buff[frameTail++] = tmpData&0xff;
		buff[frameTail++] = tmpData>>8&0xff;
		buff[frameTail++] = tmpData>>16&0xFF;
		buff[frameTail++] = tmpData>>32&0xff;
	}
		
	return frameTail;
}

/*************************************************************
函数名称:AFN0D054
功能描述:响应主站请求二类数据"终端与主站月通信流量"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D054(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	 return AFN0D053(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D055
功能描述:响应主站请求二类数据"日冻结 终端日集中抄表统计信息(重庆规约)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D055(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	DATE_TIME              tmpTime;
	INT16U                 i;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//冻结类数据时标Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	
	*offset0d = 3;
	buff[frameTail++] = tmpTime.day   = *pHandle++;
	buff[frameTail++] = tmpTime.month = *pHandle++;
	buff[frameTail++] = tmpTime.year  = *pHandle++;
	
	if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	{
    //注册电表总数
    buff[frameTail++] = meterDeviceNum&0xff;
    buff[frameTail++] = meterDeviceNum>>8&0xff;
  
    //重点用户总数
	  buff[frameTail++] = keyHouseHold.numOfHousehold;
	
	  //抄读失败电表总数
	  buff[frameTail++] = 0;
	  buff[frameTail++] = 0;
	}
  else
  {
  		#ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 7;
      #else
        for(i = 0; i < 5; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      #endif
  }
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D056
功能描述:响应主站请求二类数据"日冻结 终端日集中抄表中继统计信息(重庆规约)"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D056(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	DATE_TIME              tmpTime;
	INT16U                 i;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//冻结类数据时标Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	
	*offset0d = 3;
	buff[frameTail++] = tmpTime.day   = *pHandle++;
	buff[frameTail++] = tmpTime.month = *pHandle++;
	buff[frameTail++] = tmpTime.year  = *pHandle++;
	
	if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	{
    //0级中继路由表数
    buff[frameTail++] = meterDeviceNum&0xff;
    buff[frameTail++] = meterDeviceNum>>8&0xff;

    //1级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;

    //2级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;

    //3级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //4级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //5级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //6级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //7级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //8级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //9级中继路由表数
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
	}
  else
  {
  		#ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 7;
      #else
        for(i = 0; i < 20; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      #endif
  }
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D057
功能描述:响应主站请求二类数据"总加组日最大、最小有功功率及
         发生时间，有功功率为零时间"命令（数据格式02、18、BIN）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D057(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType, dataType;
	INT8U i, tmpDay;
	
	INT16U offset;
	INT16U tmpFrameTail;
  
  DATE_TIME tmpTime;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
 			tmpTime.minute = 0x59;
 			tmpTime.hour = 0x23;
 			if(fn == 57)
 			{
 				*offset0d = 3;
 				tmpTime.day = *pHandle++;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			else		//f60
 			{
 				*offset0d = 2;
 				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
 															, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
 				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			
 			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DT1
 			buff[frameTail++] = (pn - 1) / 8 + 1;											//DT2
 			if(fn == 57)
 			{
 				buff[frameTail++] = 0x01;																//DA1
 				queryType = DAY_BALANCE;
 				dataType = GROUP_DAY_BALANCE;
 				offset = GP_DAY_MAX_POWER;
 			}
 			else
 			{
 				buff[frameTail++] = 0x08;																//DA1
 				queryType = MONTH_BALANCE;
 				dataType = GROUP_MONTH_BALANCE;
 				offset = GP_MONTH_MAX_POWER;
 			}
 			buff[frameTail++] = 0x07;																	//DA2
 			
 			//冻结类数据时标 f57:Td_d f60:Td_m
 			if(fn == 57)
 			{
 				buff[frameTail++] = tmpTime.day;
 			}
 			buff[frameTail++] = tmpTime.month;
 			buff[frameTail++] = tmpTime.year;
 			
 			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
 			{
 				//最大有功功率
 				buff[frameTail++] = dataBuff[offset + 1];
 				buff[frameTail++] = ((dataBuff[offset] & 0x7) << 5) | (dataBuff[offset] & 0x10)
 																		| (dataBuff[offset + 2] & 0xF);
 				offset += 3;
 				
 				//最大有功功率发生时间
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				
 				//最小有功功率
 				buff[frameTail++] = dataBuff[offset + 1];
 				buff[frameTail++] = ((dataBuff[offset] & 0x7) << 5) | (dataBuff[offset] & 0x10)
 																		| (dataBuff[offset + 2] & 0xF);
 				offset += 3;
 				
 				//最小有功功率发生时间
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				
 				//有功功率为零累计时间
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 			}
 			else
 			{
 				#ifdef NO_DATA_USE_PART_ACK_03
	        frameTail = tmpFrameTail;
	      #else
	        for(i=0;i<12;i++)
	        {
	        	buff[frameTail++] = 0xee;
	        }
	      #endif
 			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D058
功能描述:响应主站请求二类数据"日冻结总加组日累计有功电能量
          "命令（数据格式03）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D058(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U tariff, queryType, dataType;
	INT8U tmpDay;
	
	DATE_TIME tmpTime;
  
  INT16U i, offset;
  INT16U tmpFrameTail;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			tmpTime.second = 0x59;
 			tmpTime.minute = 0x59;
 			tmpTime.hour = 0x23;
 			if(fn >= 58 && fn <= 59)
 			{
 				*offset0d = 3;
 				tmpTime.day = *pHandle++;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			else		//f61 f62
 			{
 				*offset0d = 2;
 				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
 															, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
 				
 				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			
 			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DT1
 			buff[frameTail++] = (pn - 1) / 8 + 1;											//DT2
 			switch(fn)
 			{
 				case 58:
 					buff[frameTail++] = 0x02;															//DA1
 					queryType = DAY_BALANCE;
 					dataType = GROUP_DAY_BALANCE;
 					offset = GP_DAY_WORK;
 					break;
 				case 59:
 					buff[frameTail++] = 0x04;															//DA1
 					queryType = DAY_BALANCE;
 					dataType = GROUP_DAY_BALANCE;
 					offset = GP_DAY_NO_WORK;
 					break;
 				case 61:
 					buff[frameTail++] = 0x10;															//DA1
 					queryType = MONTH_BALANCE;
 					dataType = GROUP_MONTH_BALANCE;
 					offset = GP_MONTH_WORK;
 					break;
 				case 62:
 					buff[frameTail++] = 0x10;															//DA1
 					queryType = MONTH_BALANCE;
 					dataType = GROUP_MONTH_BALANCE;
 					offset = GP_MONTH_NO_WORK;
 					break;
 			}
 			buff[frameTail++] = 0x07;																	//DA2
 			
 			
 			//冻结类数据时标 f58, f59:Td_d f61, f62:Td_m
 			if(fn >= 58 && fn <= 59)
 			{
 				buff[frameTail++] = tmpTime.day;
 			}
 			buff[frameTail++] = tmpTime.month;
 			buff[frameTail++] = tmpTime.year;
 			
 			//费率数
 			buff[frameTail++] = tariff = numOfTariff(pn);
 			
 			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
 			{
 				//总有功电能量(f58,f61) 总无功电能量(f59,f62)(总，费率1~M)
 				for(i = 0; i <= tariff; i++)
 				{
 					buff[frameTail++] = dataBuff[offset+3];
          buff[frameTail++] = dataBuff[offset+4];
          buff[frameTail++] = dataBuff[offset+5];
          buff[frameTail++] = (dataBuff[offset+6] & 0x0F)
          	                       | dataBuff[offset];
         offset += 7;
 				}
 			}
 			else
 			{
 				#ifdef NO_DATA_USE_PART_ACK_03
          frameTail = tmpFrameTail;
        #else
        	for(i=0;i<(tariff+1)*4;i++)
          {
          	buff[frameTail++] = 0xEE;
          }
        #endif
 			}
 		}
 		da1 >>= 1;
 	}

  return frameTail;
}

/*************************************************************
函数名称:AFN0D059
功能描述:响应主站请求二类数据"日冻结总加组日累计无功电能量
          "命令（数据格式03）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D059(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D058(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D060
功能描述:响应主站请求二类数据"月冻结总加组月最大、最小有功
          功率及其发生时间"命令（数据格式02、18）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D060(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{	
	return AFN0D057(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D061
功能描述:响应主站请求二类数据"月冻结总加组月有功电能量"命令（数据格式03）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D061(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D058(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D062
功能描述:响应主站请求二类数据"月冻结总加组月无功电能量"命令（数据格式BIN、03）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D062(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D058(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D065
功能描述:响应主站请求二类数据"月冻结总加组超功率定值的月累计时间及月累计电能量"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D065(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
    *offset0d = 2;
    return frameTail;
}

/*************************************************************
函数名称:AFN0D066
功能描述:响应主站请求二类数据"月冻结总加组超月电能量定值的月累计时间及月累计电能量"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D066(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
    *offset0d = 2;
    return frameTail;
}

/*************************************************************
函数名称:AFN0D073
功能描述:响应主站请求二类数据"总加组有功功率曲线"命令（数据格式02）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D073(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U i, interval;
	INT8U da1, da2;
	INT8U density, dataNum;
	
	DATE_TIME tmpTime, readTime;
	
	INT16U offset, tmpFrameTail;
	
	*offset0d = 7;
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			//起始时间
			tmpTime.second = 0;
			tmpTime.minute = *pHandle++;
			tmpTime.hour = *pHandle++;
			tmpTime.day = *pHandle++;
			tmpTime.month = *pHandle++;
			tmpTime.year = *pHandle++;
			
			//冻结密度
			density = *pHandle++;
			
			switch(density)
			{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			dataNum = *pHandle++;
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
			switch(fn)
			{
				case 73:
					buff[frameTail++] = 0x01;															//DT1
					offset = GP_WORK_POWER;			//总加组有功功率曲线
					break;
				case 74:
					buff[frameTail++] = 0x02;															//DT1
					offset = GP_NO_WORK_POWER;	//总加组无功功率曲线
					break;
				case 75:
					buff[frameTail++] = 0x04;															//DT1
					offset = GP_DAY_WORK;				//总加组有功电能量曲线
					break;
				case 76:
					buff[frameTail++] = 0x08;															//DT1
					offset = GP_DAY_NO_WORK;		//总加组无功电能量曲线
					break;
			}
			buff[frameTail++] = 0x09;																	//DT2
			
			//Td_c
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			//冻结密度m
			buff[frameTail++] = density;
			
			//数据点数
			buff[frameTail++] = dataNum;
			
			//时间转换
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				readTime = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, GROUP_REAL_BALANCE, &readTime, hexToBcd(interval)) == TRUE)
				{
					if(fn >= 73 && fn <= 74)
					{
						buff[frameTail++] = dataBuff[offset + 1];
						buff[frameTail++] = ((dataBuff[offset] & 0x7) << 5) | (dataBuff[offset] & 0x10)
																			| (dataBuff[offset + 2] & 0xF);
					}
					else if(fn >= 75 && fn <= 76)
					{
						buff[frameTail++] = dataBuff[offset + 3];
						buff[frameTail++] = dataBuff[offset + 4];
						buff[frameTail++] = dataBuff[offset + 5];
						buff[frameTail++] = ((dataBuff[offset] & 0x1) << 6) | (dataBuff[offset] & 0x10)
																	| (dataBuff[offset + 6]);
					}
				}
				else
				{
					if(fn >= 73 && fn <= 74)
					{
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
					}
					else
					{
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
					}
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
  return frameTail;
}

/*************************************************************
函数名称:AFN0D074
功能描述:响应主站请求二类数据"总加组无功功率曲线"命令（数据格式02）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D074(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D073(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D075
功能描述:响应主站请求二类数据"总加组有功电能量曲线"命令（数据格式03）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D075(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D073(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D076
功能描述:响应主站请求二类数据"总加组无功电能量曲线"命令（数据格式03）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D076(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D073(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D077
功能描述:响应主站请求二类数据"测量点视在功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D077(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}
/*************************************************************
函数名称:AFN0D078
功能描述:响应主站请求二类数据"测量点A相视在功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D078(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}
/*************************************************************
函数名称:AFN0D079
功能描述:响应主站请求二类数据"测量点B相视在功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D079(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}
/*************************************************************
函数名称:AFN0D080
功能描述:响应主站请求二类数据"测量点C相视在功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D080(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D081
功能描述:响应主站请求二类数据"测量点有功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2012-06-06,
    1)支持重点用户功率曲线的读取
    2)某些单相表B、C相回0xff数据的处理
*************************************************************/
INT16U AFN0D081(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif
  INT8U     meterInfo[10];
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	BOOL      bufHasData = FALSE;

  DATE_TIME tmpTime,readTime;
  
  INT16U    offset, tmpFrameTail;

  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
      
     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

      queryMeterStoreInfo(pn, meterInfo);

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			//2017-7-18,添加,视在功率曲线77-80
				case 77:		//测量点视在功率曲线
  				buff[frameTail++] = 0x10;															//DT1
  				offset = POWER_INSTANT_APPARENT;
  				break;
  				
  			case 78:		//测量点A相视在功率曲线
  				buff[frameTail++] = 0x20;															//DT1
  				offset = POWER_PHASE_A_APPARENT;
  				break;
  				
  			case 79:		//测量点B相视在功率曲线
  				buff[frameTail++] = 0x40;															//DT1
  				offset = POWER_PHASE_B_APPARENT;
  				break;
  				
  			case 80:		//测量点C相视在功率曲线
  				buff[frameTail++] = 0x80;															//DT1
  				offset = POWER_PHASE_C_APPARENT;
  				break;
				
				
				
				
  			case 81:		//测量点有功功率曲线
  				buff[frameTail++] = 0x01;															//DT1
  				offset = POWER_INSTANT_WORK;
  				break;
  				
  			case 82:		//测量点A相有功功率曲线
  				buff[frameTail++] = 0x02;															//DT1
  				offset = POWER_PHASE_A_WORK;
  				break;
  				
  			case 83:		//测量点B相有功功率曲线
  				buff[frameTail++] = 0x04;															//DT1
  				offset = POWER_PHASE_B_WORK;
  				break;
  				
  			case 84:		//测量点C相有功功率曲线
  				buff[frameTail++] = 0x08;															//DT1
  				offset = POWER_PHASE_C_WORK;
  				break;
  				
  			case 85:		//测量点无功功率曲线
  				buff[frameTail++] = 0x10;															//DT1
  				offset = POWER_INSTANT_NO_WORK;
  				break;
  				
  			case 86:		//测量点A相无功功率曲线
  				buff[frameTail++] = 0x20;															//DT1
  				offset = POWER_PHASE_A_NO_WORK;
  				break;
  				
  			case 87:		//测量点B相无功功率曲线
  				buff[frameTail++] = 0x40;															//DT1
  				offset = POWER_PHASE_B_NO_WORK;
  				break;
  				
  			case 88:		//测量点C相无功功率曲线
  				buff[frameTail++] = 0x80;															//DT1
  				offset = POWER_PHASE_C_NO_WORK;
  				break;
  		}
			if (fn<81)
			{
  		  buff[frameTail++] = 0x09;															  //DT2			
			}
			else
			{
  		  buff[frameTail++] = 0x0A;																//DT2
			}
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//数据冻结密度
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
					
				case 2:
					interval = 30;
					break;
					
				case 3:
					interval = 60;
					break;
					
				case 254:
					interval = 5;
					break;
					
				case 255:
					interval = 1;
					break;
					
				default:	//不冻结或其他情况
					interval = 61;
					break;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//时间转换
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
			  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
		 
		 #ifdef LIGHTING
		  } 
		 #endif
		 
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				       )
				   )
			  {
				  readTime = tmpTime;
				  bufHasData = readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0);
				  if (85==fn)
				  {
					  offset = 13;
				  }
				  else
				  {
					  if (81==fn)
					  {
					    offset = 10;
					  }
					  else
					  {
						  bufHasData = FALSE;
					  }
					}
				}
				else
				{
			 #endif
				
				  readTime  = timeHexToBcd(tmpTime);
				  if (meterInfo[0]==8)
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  else
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
			 #ifdef LIGHTING
			  }
			 #endif
				
				if(bufHasData == TRUE)
				{
          if (dataBuff[offset] != 0xFF)
          {
				    buff[frameTail++] = dataBuff[offset+0];
				    buff[frameTail++] = dataBuff[offset+1];
				    buff[frameTail++] = dataBuff[offset+2];
          }
          else
          {
            buff[frameTail++] = 0xee;
            buff[frameTail++] = 0xee;
            buff[frameTail++] = 0xee;
          }
				}
				else
				{
					 buff[frameTail++] = 0xEE;
					 buff[frameTail++] = 0xEE;
					 buff[frameTail++] = 0xEE;
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(10000);
			}
  	}
  	da1 >>= 1;
  }
	
  return frameTail;
}

/*************************************************************
函数名称:AFN0D082
功能描述:响应主站请求二类数据"测量点A相有功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D082(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D083
功能描述:响应主站请求二类数据"测量点B相有功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D083(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D084
功能描述:响应主站请求二类数据"测量点C相有功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D084(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D085
功能描述:响应主站请求二类数据"测量点无功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D085(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D086
功能描述:响应主站请求二类数据"测量点A相无功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D086(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D087
功能描述:响应主站请求二类数据"测量点B相无功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D087(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D088
功能描述:响应主站请求二类数据"测量点C相无功功率曲线"命令（数据格式09）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D088(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D089
功能描述:响应主站请求二类数据"测量点A相电压曲线"命令（数据格式07）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2012-06-06,
    1)支持重点电压曲线的读取
    2)某些单相表B、C相回0xff数据的处理
*************************************************************/
INT16U AFN0D089(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif
  INT8U     meterInfo[10];
	BOOL      bufHasData = FALSE;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

      queryMeterStoreInfo(pn, meterInfo);

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 89:		//测量点A相电压曲线
  				buff[frameTail++] = 0x01;															//DT1
  				offset = VOLTAGE_PHASE_A;
  				break;
  			case 90:		//测量点B相电压曲线
  				buff[frameTail++] = 0x02;															//DT1
  				offset = VOLTAGE_PHASE_B;
  				break;
  			case 91:		//测量点C相电压曲线
  				buff[frameTail++] = 0x04;															//DT1
  				offset = VOLTAGE_PHASE_C;
  				break;
  		}
  		buff[frameTail++] = 0x0B;																	//DT2
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		density = *(pHandle+5);    //数据冻结密度
			dataNum = *(pHandle+6);    //数据点数

  	 #ifdef LIGHTING
  	  //报警控制点的发送密度置为15分钟一个数据
  	  if (TRUE==pnSeted && meterConfig.protocol==LIGHTING_DGM)
  	  {
  	 	  if (density==3)
  	 	  {
  	 	  	dataNum *= 4;
  	 	  }
  	 	  if (density==2)
  	 	  {
  	 	  	dataNum *=2;
  	 	  }
  	 	  
  	 	  density = 0x1;
  	  }
  	 #endif
  		
  		//数据冻结密度
  		buff[frameTail++] = density;

  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum;
			
			//时间转换
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
			  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
		 
		 #ifdef LIGHTING
		  }
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);

			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				       )
				   )
			  {
			    if (89==fn)
			    {
				    readTime  = tmpTime;
				    bufHasData = readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0);
				    offset = 5;
			    }
			    else
			    {
			  	  bufHasData = FALSE;
			    }
			  }
			  else
			  {
			 #endif
				  
				  readTime  = timeHexToBcd(tmpTime);
				  if (meterInfo[0]==8)
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  else
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  
			 #ifdef LIGHTING
				}
			 #endif

				if(bufHasData == TRUE)
				{
					if (dataBuff[offset]!=0xFF)
					{
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
					}
					else
					{
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					}
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);
		    
		    usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D090
功能描述:响应主站请求二类数据"测量点B相电压曲线"命令（数据格式07）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D090(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D089(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D091
功能描述:响应主站请求二类数据"测量点C相电压曲线"命令（数据格式07）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D091(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D089(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D092
功能描述:响应主站请求二类数据"测量点A相电流曲线"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2012-06-06,
    1)支持重点电流曲线的读取
    2)某些单相表B、C相回0xff数据的处理
*************************************************************/
INT16U AFN0D092(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif
  INT8U     meterInfo[10];
	BOOL      bufHasData = FALSE;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

      queryMeterStoreInfo(pn, meterInfo);

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 92:		//测量点A相电流曲线
  				buff[frameTail++] = 0x08;															//DT1
  				offset = CURRENT_PHASE_A;
  				break;
  				
  			case 93:		//测量点B相电流曲线
  				buff[frameTail++] = 0x10;															//DT1
  				offset = CURRENT_PHASE_B;
  				break;
  				
  			case 94:		//测量点C相电流曲线
  				buff[frameTail++] = 0x20;															//DT1
  				offset = CURRENT_PHASE_C;
  				break;
  				
  			case 95:		//测量点零序电流曲线
  				buff[frameTail++] = 0x40;															//DT1
  				offset = ZERO_SERIAL_CURRENT;
  				break;
  		}
  		
  		buff[frameTail++] = 0x0B;																	//DT2
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		density = *(pHandle+5);    //数据冻结密度
			dataNum = *(pHandle+6);    //数据点数

  	 #ifdef LIGHTING
  	  //报警控制点的发送密度置为15分钟一个数据
  	  if (TRUE==pnSeted && meterConfig.protocol==LIGHTING_DGM)
  	  {
  	 	  if (density==3)
  	 	  {
  	 	  	dataNum *= 4;
  	 	  }
  	 	  if (density==2)
  	 	  {
  	 	  	dataNum *=2;
  	 	  }
  	 	  
  	 	  density = 0x1;
  	  }
  	 #endif
  	 
  		//数据冻结密度
  		buff[frameTail++] = density;

  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum;
			
			//时间转换
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
			  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			  
		 #ifdef LIGHTING
			}
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				       )
				   )
		  	{
				  if (92==fn)
				  {
				    readTime  = tmpTime;
				    bufHasData = readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0);
				    offset = 7;
				  }
				  else
				  {
					  bufHasData = FALSE;
					}
				}
				else
				{
			 #endif
				
				  readTime  = timeHexToBcd(tmpTime);
				
				  if (meterInfo[0]==8)
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  else
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  
			 #ifdef LIGHTING
				}
			 #endif

				if(bufHasData == TRUE)
				{
          if (dataBuff[offset]!=0xFF)
          {
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
					  buff[frameTail++] = dataBuff[offset+2];
					}
					else
					{
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;						 
					}
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}

				tmpTime = nextTime(tmpTime, interval, 0);

		    usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D093
功能描述:响应主站请求二类数据"测量点B相电流曲线"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D093(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D092(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D094
功能描述:响应主站请求二类数据"测量点C相电流曲线"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D094(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D092(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D095
功能描述:响应主站请求二类数据"测量点零序电流曲线曲线"命令（数据格式25）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D095(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D092(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D096
功能描述:响应主站请求二类数据"测量点频率曲线"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2017-07-19,
    1)应同远要求添加本函数
*************************************************************/
INT16U AFN0D096(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  INT8U     meterInfo[10];
  BOOL      bufHasData = FALSE;
  INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, dataNum;
  INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  	  pn = tmpPn + (da2 - 1) * 8;
  		

      queryMeterStoreInfo(pn, meterInfo);

  	  buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));    //DA1
  	  buff[frameTail++] = (pn - 1) / 8 + 1;						  //DA2
      
	  	//测量点频率曲线
  	  buff[frameTail++] = 0x80;									  //DT1
  	  offset = METER_STATUS_WORD;
 	  	buff[frameTail++] = 0x0B;									  //DT2
  		
  	  //曲线类数据时标Td_c
  	  //起始时间：分时日月年
  	  tmpTime.second = 0;
  	  buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  	  buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  	  buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  	  buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  	  buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  	  density = *(pHandle+5);    //数据冻结密度
	  	dataNum = *(pHandle+6);    //数据点数
  	 
  	  //数据冻结密度
  	  buff[frameTail++] = density;

		  switch(density)
		  {
				case 1:
				  interval = 15;
				  break;
				case 2:
				  interval = 30;
				  break;
				case 3:
				  interval = 60;
				  break;
				case 254:
				  interval = 5;
				  break;
				case 255:
				  interval = 1;
				  break;
				default:	//不冻结或其他情况
				  interval = 61;
		  }
			
		  //无效数据判断
		  if(interval > 60)
		  {
				return tmpFrameTail;
		  }
			
		  //数据点数
		  buff[frameTail++] = dataNum;
				
		  //时间转换
		  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
				  
				
		  for(i = 0; i < dataNum; i++)
		  {
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
						
				readTime  = timeHexToBcd(tmpTime);
						
				if (meterInfo[0]==8)
				{
				  bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				}
				else
				{
				  bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				}
					  

				if(bufHasData == TRUE)
				{
          if (dataBuff[offset]!=0xFF)
          {
						buff[frameTail++] = dataBuff[offset+0];
						buff[frameTail++] = dataBuff[offset+1];
				  }
				  else
				  {
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
				  buff[frameTail++] = 0xEE;
				  buff[frameTail++] = 0xEE;
				}

				tmpTime = nextTime(tmpTime, interval, 0);

				usleep(30000);
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}


/*************************************************************
函数名称:AFN0D097
功能描述:响应主站请求二类数据"测量点正向有功总电能量曲线"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D097(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 97:		//测量点正向有功总电能量曲线
  				buff[frameTail++] = 0x01;															//DT1
  				offset = DAY_P_WORK_OFFSET;
  				break;
  				
  			case 98:		//测量点正向无功总电能量曲线
  				buff[frameTail++] = 0x02;															//DT1
  				offset = DAY_P_NO_WORK_OFFSET;
  				break;
  				
  			case 99:		//测量点反向有功总电能量曲线
  				buff[frameTail++] = 0x04;															//DT1
  				offset = DAY_N_WORK_OFFSET;
  				break;
  				
  			case 100:		//测量点反向无功总电能量曲线
  				buff[frameTail++] = 0x08;															//DT1
  				offset = DAY_N_NO_WORK_OFFSET;
  				break;
  		}
  		buff[frameTail++] = 0x0C;																	//DT2
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//数据冻结密度
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//时间转换
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
				readTime  = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, tmpMinute) == TRUE)
				{
					buff[frameTail++] = dataBuff[offset + 1];
					buff[frameTail++] = dataBuff[offset + 2];
					buff[frameTail++] = dataBuff[offset + 3];
					buff[frameTail++] = dataBuff[offset + 4];
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}

				tmpTime = nextTime(tmpTime, interval, 0);

		    usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D098
功能描述:响应主站请求二类数据"测量点正向无功总电能量曲线"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D098(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D097(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D099
功能描述:响应主站请求二类数据"测量点反向有功总电能量曲线"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D099(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D097(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D100
功能描述:响应主站请求二类数据"测量点反向无功总电能量曲线"命令（数据格式13）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D100(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D097(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D101
功能描述:响应主站请求二类数据"测量点正向有功总电能示值曲线"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2012-06-06,修改,重点用户和单相户表的示值曲线能读取,以前的版本都不能读取
*************************************************************/
INT16U AFN0D101(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	METER_DEVICE_CONFIG meterConfig;
	INT8U               dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U              pn, tmpPn = 0;
	INT8U               da1, da2;
	INT8U               density, dataNum;
	INT8U               i, interval, tmpMinute;
  DATE_TIME           tmpTime, readTime;  
  INT16U              offset, tmpFrameTail;
	INT8U               queryType;
  INT8U               meterInfo[10];
  BOOL                carrierMeterFlag = FALSE;

  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
			
			carrierMeterFlag = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
      	if ((meterConfig.rateAndPort&0x1f)==PORT_POWER_CARRIER && meterConfig.protocol==DLT_645_2007)
      	{
      	 	carrierMeterFlag = TRUE;
      	}
        queryMeterStoreInfo(pn, meterInfo);
      }

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 101:		//测量点正向有功总电能示值曲线
  				buff[frameTail++] = 0x10;															//DT1
  				offset = POSITIVE_WORK_OFFSET;
  				break;
  				
  			case 102:		//测量点正向无功总电能示值曲线
  				buff[frameTail++] = 0x20;															//DT1
  				offset = POSITIVE_NO_WORK_OFFSET;
  				break;
  				
  			case 103:		//测量点反向有功总电能示值曲线
  				buff[frameTail++] = 0x40;															//DT1
  				offset = NEGTIVE_WORK_OFFSET;
  				break;
  				
  			case 104:		//测量点反向无功总电能示值曲线
  				buff[frameTail++] = 0x80;															//DT1
  				offset = NEGTIVE_NO_WORK_OFFSET;
  				break;
  		}
  		buff[frameTail++] = 0x0C;																	//DT2

  		//确定查询类型
  		queryType = CURVE_DATA_PRESENT;
  		
  		if(fn == 101 || fn == 103)
  		{
    		//重点用户
    		if (meterInfo[0]==8)
    		{
    			queryType = CURVE_KEY_HOUSEHOLD;
    		}
    		else
    		{
      		//单相表(载波或其它)
      		if (meterInfo[0]<4)
      		{
      	   #ifdef CQDL_CSM
      		  if (carrierMeterFlag == TRUE)
      		  {
      			  queryType = HOUR_FREEZE;
      			  if (fn==103)
      			  {
      				   offset = HOUR_FREEZE_N_WORK;
      			  }
      		  }
      		  else
      		  {
      		 #endif
      		 
      		  	queryType = CURVE_SINGLE_PHASE;
      		  	
      		  	if (fn==103)
      		  	{
      		  		offset = NEGTIVE_WORK_OFFSET_S;
      		  	}
      		  	
      		 #ifdef CQDL_CSM
      		  }
      		 #endif
      		}
      	}
      }
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//数据冻结密度
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum = *(pHandle+6);
			
  	 #ifdef CQDL_CSM
  		if((fn == 101 || fn == 103) && carrierMeterFlag == TRUE)
  		{
  			tmpTime = timeBcdToHex(tmpTime);
  		}
  		else
  		{
  	 #endif
			  
			  //时间转换
			 #ifdef LIGHTING
			  if (
			  	  (meterConfig.rateAndPort&0x1f)==PORT_POWER_CARRIER
			  	   || meterConfig.protocol==LIGHTING_DGM    //报警控制点
			  	 )
			  {
			    tmpTime = timeBcdToHex(tmpTime);
			  }
			  else
			  {
			 #endif
			   
			    tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			    
			 #ifdef LIGHTING
			  }
			 #endif
			  
		 #ifdef CQDL_CSM
		 	}
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (
			  	  (meterConfig.rateAndPort&0x1f)==PORT_POWER_CARRIER
			  	   || meterConfig.protocol==LIGHTING_DGM    //报警控制点
			  	 )
			  {
				  if (101==fn || 102==fn)
				  {
				    if (101==fn)
				    {
				  	  offset = 18;
				    }
				    else
				    {
				  	  offset = 22;
				    }
				    readTime  = tmpTime;
				    if(readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0) == TRUE)
				    {
					    buff[frameTail++] = dataBuff[offset+0];
					    buff[frameTail++] = dataBuff[offset+1];
					    buff[frameTail++] = dataBuff[offset+2];
					    buff[frameTail++] = dataBuff[offset+3];
				    }
				    else
				    {
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					  }
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  	buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
			 #endif
			 	
				  readTime  = timeHexToBcd(tmpTime);
				  if(readMeterData(dataBuff, pn, queryType, ENERGY_DATA, &readTime, tmpMinute) == TRUE)
				  {
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
					  buff[frameTail++] = dataBuff[offset+2];
					  buff[frameTail++] = dataBuff[offset+3];
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				  
			 #ifdef LIGHTING
			  }
			 #endif

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(100000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D102
功能描述:响应主站请求二类数据"测量点正向无功总电能示值曲线"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D102(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D101(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D103
功能描述:响应主站请求二类数据"测量点反向有功总电能示值曲线"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D103(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D101(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D104
功能描述:响应主站请求二类数据"测量点反向无功总电能示值曲线"命令（数据格式11）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D104(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D101(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D105
功能描述:响应主站请求二类数据"测量点功率因数曲线"命令（数据格式05）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D105(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif

	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime,readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 105:		//测量点功率因数曲线
  				buff[frameTail++] = 0x01;															//DT1
  				offset = TOTAL_POWER_FACTOR;
  				break;
  				
  			case 106:		//测量点A相功率因数曲线
  				buff[frameTail++] = 0x02;															//DT1
  				offset = FACTOR_PHASE_A;
  				break;
  				
  			case 107:		//测量点B相功率因数曲线
  				buff[frameTail++] = 0x04;															//DT1
  				offset = FACTOR_PHASE_B;
  				break;
  				
  			case 108:		//测量点C相功率因数曲线
  				buff[frameTail++] = 0x08;															//DT1
  				offset = FACTOR_PHASE_C;
  				break;
  		}
  		buff[frameTail++] = 0x0D;																	//DT2
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//数据冻结密度
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//时间转换
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
  			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
  	 
  	 #ifdef LIGHTING
  	  }
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //报警控制点
				       )
				   )
			  {
				  if (105==fn)
				  {
				    readTime  = tmpTime;
				
				    if(readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0) == TRUE)
				    {
					    buff[frameTail++] = dataBuff[16];
					    buff[frameTail++] = dataBuff[17];
				    }
				    else
				    {
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					  }
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
			 #endif
			 
				  readTime  = timeHexToBcd(tmpTime);
				  if(readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute) == TRUE)
				  {
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				  
			 #ifdef LIGHTING
			  }
			 #endif

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D106
功能描述:响应主站请求二类数据"测量点A相功率因数曲线"命令（数据格式05）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D106(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D105(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D107
功能描述:响应主站请求二类数据"测量点B相功率因数曲线"命令（数据格式05）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D107(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D105(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D108
功能描述:响应主站请求二类数据"测量点C相功率因数曲线"命令（数据格式05）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D108(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D105(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D109
功能描述:响应主站请求二类数据"测量点电压相位角曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D109(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime,readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 109:		//测量点电压相位角曲线
  				buff[frameTail++] = 0x10;															//DT1
  				offset = PHASE_ANGLE_V_A;
  				break;
  				
  			case 110:		//测量点电流相位角曲线
  				buff[frameTail++] = 0x20;															//DT1
  				offset = PHASE_ANGLE_C_A;
  				break;
  		}
  		
  		buff[frameTail++] = 0x0E;																	//DT2
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//数据冻结密度
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//时间转换
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
				//    原因:读取曲线数据时调用readMeterData参数错误
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
				readTime  = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute) == TRUE)
				{
					//memcpy(&buff[frameTail], &dataBuff[offset],6);
					memset(&buff[frameTail], 0x00, 6);
				}
				else
				{
					//为兰州供电公司测试做的					
					memset(&buff[frameTail], 0xee, 6);
					frameTail += 6;					
				}
				frameTail += 6;

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D110
功能描述:响应主站请求二类数据"测量点电流相位角曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D110(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D109(buff, frameTail, pHandle, fn, offset0d);
}

#ifndef LIGHTING

/*************************************************************
函数名称:AFN0D111
功能描述:响应主站请求二类数据"开关量曲线"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2017-09-22,
    1)添加开关量曲线
*************************************************************/
INT16U AFN0D111(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  INT8U     meterInfo[10];
  BOOL      bufHasData = FALSE;
  INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, dataNum;
  INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  	  pn = tmpPn + (da2 - 1) * 8;
  		

      queryMeterStoreInfo(pn, meterInfo);

  	  buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));    //DA1
  	  buff[frameTail++] = (pn - 1) / 8 + 1;						  //DA2
      
	  //测量点开关量曲线
  	  buff[frameTail++] = 0x40;									  //DT1
 	  buff[frameTail++] = 0x0D;									  //DT2
  	  offset = METER_STATUS_WORD_2;
  		
  	  //曲线类数据时标Td_c
  	  //起始时间：分时日月年
  	  tmpTime.second = 0;
  	  buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  	  buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  	  buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  	  buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  	  buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  	  density = *(pHandle+5);    //数据冻结密度
	  dataNum = *(pHandle+6);    //数据点数
  	 
  	  //数据冻结密度
  	  buff[frameTail++] = density;

	  switch(density)
	  {
		case 1:
		  interval = 15;
		  break;
		case 2:
		  interval = 30;
		  break;
		case 3:
		  interval = 60;
		  break;
		case 254:
		  interval = 5;
		  break;
		case 255:
		  interval = 1;
		  break;
		default:	//不冻结或其他情况
		  interval = 61;
	  }
			
	  //无效数据判断
	  if(interval > 60)
	  {
		return tmpFrameTail;
	  }
			
	  //数据点数
	  buff[frameTail++] = dataNum;
			
	  //时间转换
	  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			  
			
	  for(i = 0; i < dataNum; i++)
	  {
		//2012-10-22,修改这个错误,在甘肃榆中运行时发现,用不同的密度召回来的数据在某点不一致
		//    原因:读取曲线数据时调用readMeterData参数错误
		//tmpMinute = hexToBcd(tmpTime.minute+interval);
		tmpMinute = hexToBcd(interval);
				
		readTime  = timeHexToBcd(tmpTime);
				
		if (meterInfo[0]==8)
		{
		  bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
		}
		else
		{
		  bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
		}
				  

		if(bufHasData == TRUE)
		{
		  buff[frameTail++] = dataBuff[offset+0];
		  buff[frameTail++] = dataBuff[offset+1];
		}
		else
		{
		  buff[frameTail++] = 0xEE;
		  buff[frameTail++] = 0xEE;
		}

		tmpTime = nextTime(tmpTime, interval, 0);

		usleep(30000);
	  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

#else

/*************************************************************
函数名称:AFN0D111
功能描述:响应主站请求二类数据"测量点亮度曲线"命令（数据格式05）
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D111(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;

	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime,readTime;
  
  INT16U    tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 111:		//测量点亮度曲线
  				buff[frameTail++] = 0x40;															//DT1
  				break;
  				
  			case 112:		//测量点温度曲线
  				buff[frameTail++] = 0x80;															//DT1
  				break;
  		}
  		buff[frameTail++] = 0x0D;																	//DT2
  		
  		//曲线类数据时标Td_c
  		//起始时间：分时日月年
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		density = *(pHandle+5);    //数据冻结密度
			dataNum = *(pHandle+6);    //数据点数

  	 #ifdef LIGHTING
  	  //报警控制点的发送密度置为15分钟一个数据
  	  if (TRUE==pnSeted && meterConfig.protocol==LIGHTING_DGM)
  	  {
  	 	  if (density==3)
  	 	  {
  	 	  	dataNum *= 4;
  	 	  }
  	 	  if (density==2)
  	 	  {
  	 	  	dataNum *=2;
  	 	  }
  	 	  
  	 	  density = 0x1;
  	  }
  	 #endif
  	 
  		//数据冻结密度
  		buff[frameTail++] = density;
  		 
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//不冻结或其他情况
					interval = 61;
			}
			
			//无效数据判断
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//数据点数
			buff[frameTail++] = dataNum;
			
			//时间转换
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)      //报警控制点
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_XL)     //线路控制点
									|| (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_TH)	 //湿温度测量点
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
  			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
  	  }
			
			for(i = 0; i < dataNum; i++)
			{
				tmpMinute = hexToBcd(interval);
				
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //单灯控制器控制点
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)      //报警控制点
				          || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_XL)     //线路控制点
										|| (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_TH)	 //湿温度测量点
				       )
				   )
			  {
			    readTime  = tmpTime;
			    if(readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0) == TRUE)
			    {
				    if (111==fn)
				    {
				      //温湿度传感器的湿度用在此传送				      
				      buff[frameTail++] = dataBuff[28];
				      buff[frameTail++] = dataBuff[29];
				    }
				    else    //FN=112
				    {
				      buff[frameTail++] = dataBuff[26];
				      buff[frameTail++] = dataBuff[27];
				    }
			    }
			    else
			    {
				    buff[frameTail++] = 0xEE;
				    buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
			  }

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
函数名称:AFN0D112
功能描述:响应主站请求二类数据"测量点温度曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D112(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D111(buff, frameTail, pHandle, fn, offset0d);
}

#endif

/*************************************************************
函数名称:AFN0D113
功能描述:响应主站请求二类数据"日冻结测量点U相2-19次谐波电流日最大值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D113(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D114
功能描述:响应主站请求二类数据"日冻结测量点V相2-19次谐波电流日最大值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D114(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D115
功能描述:响应主站请求二类数据"日冻结测量点W相2-19次谐波电流日最大值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D115(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D116
功能描述:响应主站请求二类数据"日冻结测量点U相2-19次谐波电压含有率及总畸变率日最大值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D116(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D117
功能描述:响应主站请求二类数据"日冻结测量点V相2-19次谐波电压含有率及总畸变率日最大值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D117(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D118
功能描述:响应主站请求二类数据"日冻结测量点W相2-19次谐波电压含有率及总畸变率日最大值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D118(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D121
功能描述:响应主站请求二类数据"日冻结测量点A相谐波越限日统计数据"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D121(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D122
功能描述:响应主站请求二类数据"日冻结测量点B相谐波越限日统计数据"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D122(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D123
功能描述:响应主站请求二类数据"日冻结测量点C相谐波越限日统计数据"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D123(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
函数名称:AFN0D129
功能描述:响应主站请求二类数据"日冻结直流模拟量越限日累计时间、最大/最小值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D129(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	 #ifdef PLUG_IN_CARRIER_MODULE
  	TERMINAL_STATIS_RECORD terminalStatisRecord;
  	INT8U                  i, tmpDay;
  	DATE_TIME              tmpTime, bakTime;
  	INT8U                  tmpCount;
  	INT32U                 tmpMonthUp,tmpMonthDown;
  	INT16U                 tmpMonthMax,tmpMonthMin;
  	INT8U                  tmpMaxTime[3],tmpMinTime[3];
  	INT32U                 tmpData;
  	
  	buff[frameTail++] = *pHandle++;		//DA1
  	buff[frameTail++] = *pHandle++;		//DA2
  	buff[frameTail++] = *pHandle++;		//DT1
  	buff[frameTail++] = *pHandle++;		//DT2
  	
  	//冻结类数据时标Td_d Td_m
  	tmpTime.second = 0x59;
  	tmpTime.minute = 0x59;
  	tmpTime.hour = 0x23;
  	if(fn == 129)
  	{
  		*offset0d = 3;
  		buff[frameTail++] = tmpTime.day   = *pHandle++;
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	else		//F130
  	{
  		*offset0d = 2;
  		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
  													, (*pHandle >> 4) * 10 + *pHandle % 10);
  		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
  		
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	
  	if(fn == 129)
  	{
  	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  	  {
  			//直流模拟量越上限累计时间
  			buff[frameTail++] = terminalStatisRecord.dcOverUp & 0xFF;
  			buff[frameTail++] = terminalStatisRecord.dcOverUp >> 8;
  			
  			//直流模拟量越下限累计时间
  			buff[frameTail++] = terminalStatisRecord.dcOverDown & 0xFF;
  			buff[frameTail++] = terminalStatisRecord.dcOverDown >> 8;

  			//直流模拟量最大值
  			buff[frameTail++] = terminalStatisRecord.dcMax[0];
  			buff[frameTail++] = terminalStatisRecord.dcMax[1];

  			//直流模拟量最大值发生时间
  			buff[frameTail++] = terminalStatisRecord.dcMaxTime[0];
  			buff[frameTail++] = terminalStatisRecord.dcMaxTime[1];
  			buff[frameTail++] = terminalStatisRecord.dcMaxTime[2];

  			//直流模拟量最小值
  			buff[frameTail++] = terminalStatisRecord.dcMin[0];
  			buff[frameTail++] = terminalStatisRecord.dcMin[1];

  			//直流模拟量最小值发生时间
  			buff[frameTail++] = terminalStatisRecord.dcMinTime[0];
  			buff[frameTail++] = terminalStatisRecord.dcMinTime[1];
  			buff[frameTail++] = terminalStatisRecord.dcMinTime[2];
  	  }
  	  else
  	  {
        for(i = 0; i < 14; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      }
  	}
  	else
  	{
    	bakTime = timeBcdToHex(tmpTime);
    	tmpCount = monthDays(bakTime.year+2000,bakTime.month);
    	tmpMonthUp   = 0;
    	tmpMonthDown = 0;
    	tmpMonthMax  = 0xeeee;
    	tmpMonthMin  = 0xeeee;
    	for(i=1; i<=tmpCount; i++)
    	{
    		tmpTime = bakTime;
    		tmpTime.day = i;
    	 	tmpTime = timeHexToBcd(tmpTime);
        if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
        {
          tmpMonthUp   += terminalStatisRecord.dcOverUp;
          tmpMonthDown += terminalStatisRecord.dcOverDown;
          
          //找最大值及发生时间
          tmpData = format2ToHex(terminalStatisRecord.dcMax[0] | terminalStatisRecord.dcMax[1]<<8);
          if (tmpMonthMax==0xeeee)
          {
          	tmpMonthMax = tmpData;
          	tmpMaxTime[0] = terminalStatisRecord.dcMaxTime[0]; 
          	tmpMaxTime[1] = terminalStatisRecord.dcMaxTime[1]; 
          	tmpMaxTime[2] = terminalStatisRecord.dcMaxTime[2]; 
          }
          else
          {
          	if (tmpData>tmpMonthMax)
          	{
          	  tmpMonthMax = tmpData;
          	  tmpMaxTime[0] = terminalStatisRecord.dcMaxTime[0]; 
          	  tmpMaxTime[1] = terminalStatisRecord.dcMaxTime[1]; 
          	  tmpMaxTime[2] = terminalStatisRecord.dcMaxTime[2]; 
          	}
          }
          
          //找最小值及发生时间
          tmpData = format2ToHex(terminalStatisRecord.dcMin[0] | terminalStatisRecord.dcMin[1]<<8);
          if (tmpMonthMin==0xeeee)
          {
          	tmpMonthMin = tmpData;
          	tmpMinTime[0] = terminalStatisRecord.dcMinTime[0]; 
          	tmpMinTime[1] = terminalStatisRecord.dcMinTime[1]; 
          	tmpMinTime[2] = terminalStatisRecord.dcMinTime[2]; 
          }
          else
          {
          	if (tmpData<tmpMonthMin)
          	{
          	  tmpMonthMin = tmpData;
          	  tmpMinTime[0] = terminalStatisRecord.dcMinTime[0]; 
          	  tmpMinTime[1] = terminalStatisRecord.dcMinTime[1]; 
          	  tmpMinTime[2] = terminalStatisRecord.dcMinTime[2]; 
          	}
          }
        }
    	}
      
  		//直流模拟量越上限月累计时间
  		buff[frameTail++] = tmpMonthUp & 0xFF;
  		buff[frameTail++] = tmpMonthUp >> 8;
  			
  		//直流模拟量越下限月累计时间
  		buff[frameTail++] = tmpMonthDown & 0xFF;
  		buff[frameTail++] = tmpMonthDown >> 8;
  		
  		//直流模拟量月最大值及发生时间
      if (tmpMonthMax==0xeeee)
      {
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      }
      else
      {
        buff[frameTail++] = hexToBcd(tmpMonthMax/100);
        buff[frameTail++] = (hexToBcd(tmpMonthMax/100)>>8&0xf) | 0xa0;
      	buff[frameTail++] = tmpMaxTime[0];
      	buff[frameTail++] = tmpMaxTime[1];
      	buff[frameTail++] = tmpMaxTime[2];
      }
      
  		//直流模拟量月最小值及发生时间
      if (tmpMonthMin==0xeeee)
      {
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      }
      else
      {
        buff[frameTail++] = hexToBcd(tmpMonthMin/100);
        buff[frameTail++] = (hexToBcd(tmpMonthMin/100)>>8&0xf) | 0xa0;
      	buff[frameTail++] = tmpMinTime[0];
      	buff[frameTail++] = tmpMinTime[1];
      	buff[frameTail++] = tmpMinTime[2];
      }
  	}
   #else
    *offset0d = 3;
   #endif

	 return frameTail;
}

/*************************************************************
函数名称:AFN0D130
功能描述:响应主站请求二类数据"月冻结直流模拟量越限月累计时间、最大/最小值及发生时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D130(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	 #ifdef PLUG_IN_CARRIER_MODULE
	  return AFN0D129(buff, frameTail, pHandle, fn, offset0d);
	 #else
    *offset0d = 2;
	   return frameTail;
   #endif
}

/*************************************************************
函数名称:AFN0D138
功能描述:响应主站请求二类数据"直流模拟量数据曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D138(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{	 
	#ifdef PLUG_IN_CARRIER_MODULE
  	INT8U     dataBuff[5];
  	INT16U    pn, tmpPn = 0;
  	INT8U     da1, da2;
  	INT8U     density, interval, tmpMinute;
  	INT8U     i, dataNum;
  	
  	INT16U    tmpFrameTail;
  	
  	DATE_TIME tmpTime,readTime;
  	
    *offset0d = 7; 
    tmpFrameTail = frameTail;
    da1 = *pHandle++;
    da2 = *pHandle++;
    pHandle += 2;
    
    if(da1 == 0x0)
    {
    	return frameTail;
    }
    
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		
    		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	 //DA1
    		buff[frameTail++] = (pn - 1) / 8 + 1;											 //DA2
    		buff[frameTail++] = 0x02;															     //DT1
    		buff[frameTail++] = 0x11;																	 //DT2
    		
    		//曲线类数据时标Td_c
    		tmpTime.second = 0;
    		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
    		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
    		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
    		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
    		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
    		
    		//冻结密度
    		buff[frameTail++] = density = *(pHandle+5);
    		switch(density)
    		{
    			case 1:
    				interval = 15;
    				break;
    			case 2:
    				interval = 30;
    				break;
    			case 3:
    				interval = 60;
    				break;
    			case 254:
    				interval = 5;
    				break;
    			case 255:
    				interval = 1;
    				break;
    			default:	//不冻结或其他情况
    				interval = 61;
    		}
    		
    		if(interval > 60)
    		{
    			return tmpFrameTail;
    		}
    		
    		//数据点数
    		buff[frameTail++] = dataNum = *(pHandle+6);
    		
  			//时间转换
  			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
  			
  			for(i = 0; i < dataNum; i++)
  			{
  				//tmpMinute = hexToBcd(tmpTime.minute+interval);
  				tmpMinute = hexToBcd(interval);
  				
  				readTime  = timeHexToBcd(tmpTime);
  				if(readMeterData(dataBuff, pn, DC_ANALOG, DC_ANALOG_CURVE_DATA, &readTime, tmpMinute) == TRUE)
  				{
  					buff[frameTail++] = dataBuff[0];
  					buff[frameTail++] = dataBuff[1];
  				}
  				else
  				{
  					buff[frameTail++] = 0xEE;
  					buff[frameTail++] = 0xEE;
  				}
  				
  				tmpTime = nextTime(tmpTime, interval, 0);
  
  				usleep(30000);
  			}
    	}
    	da1 >>= 1;
    }
  #else
   *offset0d = 7; 
  #endif

	 return frameTail;
}

/*************************************************************
函数名称:AFN0D145
功能描述:响应主站请求二类数据"测量点一象限无功总电能示值曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D145(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, interval, tmpMinute;
	INT8U     i, dataNum;
	
	INT16U    tmpFrameTail;
	INT16U    offset;
	
	DATE_TIME tmpTime,readTime;
	
  *offset0d = 7; 
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 145:		//测量点一象限无功总电能示值曲线
  				buff[frameTail++] = 0x01;															//DT1
  				offset = QUA1_NO_WORK_OFFSET;
  				break;
  				
  			case 146:		//测量点四象限无功总电能示值曲线
  				buff[frameTail++] = 0x02;															//DT1
  				offset = QUA4_NO_WORK_OFFSET;
  				break;
  				
  			case 147:		//测量点二象限无功电能示值曲线
  				buff[frameTail++] = 0x04;															//DT1
  				offset = QUA2_NO_WORK_OFFSET;
  				break;
  				
  			case 148:		//测量点三象限无功电能示值曲线
  				buff[frameTail++] = 0x08;															//DT1
  				offset = QUA3_NO_WORK_OFFSET;
  				break;
  		}
  		buff[frameTail++] = 0x12;																	//DT2
  		
  		//曲线类数据时标Td_c
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//冻结密度
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
  			case 1:
  				interval = 15;
  				break;
  			case 2:
  				interval = 30;
  				break;
  			case 3:
  				interval = 60;
  				break;
  			case 254:
  				interval = 5;
  				break;
  			case 255:
  				interval = 1;
  				break;
  			default:	//不冻结或其他情况
  				interval = 61;
  		}
  		
  		if(interval > 60)
  		{
  			return tmpFrameTail;
  		}
  		
  		//数据点数
  		buff[frameTail++] = dataNum = *(pHandle+6);
  		
			//时间转换
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
				readTime  = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, ENERGY_DATA, &readTime, tmpMinute) == TRUE)
				{
					buff[frameTail++] = dataBuff[offset+0];
					buff[frameTail++] = dataBuff[offset+1];
					buff[frameTail++] = dataBuff[offset+2];
					buff[frameTail++] = dataBuff[offset+3];
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);

				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D146
功能描述:响应主站请求二类数据"测量点四象限无功总电能示值曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D146(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D145(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D147
功能描述:响应主站请求二类数据"测量点二象限无功总电能示值曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D147(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D145(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D148
功能描述:响应主站请求二类数据"测量点三象限无功总电能示值曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D148(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D145(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D153
功能描述:响应主站请求二类数据"日冻结测量点分相正向有功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D153(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     queryType, dataType;
	INT8U     i, tmpDay;
	
	DATE_TIME tmpTime,readTime;
	
	INT16U    tmpFrameTail;
	INT16U    offset;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 153 || fn == 155)
			{
				*offset0d = 3;
				tmpTime.day   = *(pHandle+0);
				tmpTime.month = *(pHandle+1);
				tmpTime.year  = *(pHandle+2);
			}
			else	//f157 f159
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				tmpTime.month = *(pHandle+0);
				tmpTime.year  = *(pHandle+1);
			}
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
			switch(fn)
			{
				case 153:		//日冻结测量点分相正向有功电能示值
					buff[frameTail++] = 0x01;															//DT1
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_DATA;
					break;
					
				case 155:		//日冻结测量点分相反向有功电能示值
					buff[frameTail++] = 0x04;															//DT1
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_DATA;
					break;
					
				case 157:		//月冻结测量点分相正向有功电能示值
					buff[frameTail++] = 0x10;															//DT1
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_DATA;
					break;
					
				case 159:		//月冻结测量点分相反向有功电能示值
					buff[frameTail++] = 0x40;															//DT1
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_DATA;
					break;
			}
			buff[frameTail++] = 0x13;																	//DT2
			
			//冻结类数据时标
			if(fn == 153 || fn == 155)
			{
				buff[frameTail++] = tmpTime.day;
			}
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			//终端抄表时间(分时日月年)
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
			{
				//数据格式14
				//A相,B相,C相正向有功电能示值
				if(fn == 153 || fn == 157)
				{
					offset = POSITIVE_WORK_A_OFFSET;
				}
				else	//f155 f159
				{
					offset = NEGTIVE_WORK_A_OFFSET;
				}
				for(i = 0; i < 3; i++)
				{
					if(dataBuff[offset] != 0xEE)
					{
						buff[frameTail++] = 0x0;
					}
					else
					{
						buff[frameTail++] = 0xEE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					offset += 12;
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  for(i = 0; i < 15; i++)
			    {
			   	  buff[frameTail++] = 0xEE;
			    }
			  #endif
			}
		}
		da1 >>= 1;
		
		usleep(30000);
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D154
功能描述:响应主站请求二类数据"日冻结测量点分相正向无功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D154(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType;
	INT8U i, tmpDay;
	
	DATE_TIME tmpTime;
	
	INT16U tmpFrameTail;
	INT16U offset;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 154 || fn == 156)
			{
				*offset0d = 3;
				tmpTime.day = *pHandle++;
				tmpTime.month = *pHandle++;
				tmpTime.year = *pHandle++;
			}
			else	//f158
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				tmpTime.month = *pHandle++;
				tmpTime.year = *pHandle++;
			}
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
			switch(fn)
			{
				case 154:		//日冻结测量点分相正向无功电能示值
					buff[frameTail++] = 0x02;															//DT1
					queryType = DAY_BALANCE;
					break;
				case 156:		//日冻结测量点分相反向无功电能示值
					buff[frameTail++] = 0x08;															//DT1
					queryType = DAY_BALANCE;
					break;
				case 158:		//月冻结测量点分相正向无功电能示值
					buff[frameTail++] = 0x20;															//DT1
					queryType = MONTH_BALANCE;
					break;
				case 160:		//月冻结测量点分相反向无功电能示值
					buff[frameTail++] = 0x80;															//DT1
					queryType = MONTH_BALANCE;
					break;
			}
			buff[frameTail++] = 0x13;																	//DT2
			
			//冻结类数据时标
			if(fn == 154 || fn == 156)
			{
				buff[frameTail++] = tmpTime.day;
			}
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			//终端抄表时间(分时日月年)
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			if(readMeterData(dataBuff, pn, queryType, ENERGY_DATA, &tmpTime, 0) == TRUE)
			{
				//数据格式14
				//A相,B相,C相正向有功电能示值
				if(fn == 154 || fn == 158)
				{
					offset = COMB1_NO_WORK_A_OFFSET;
				}
				else
				{
					offset = COMB2_NO_WORK_A_OFFSET;
				}
				for(i = 0; i < 3; i++)
				{
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					offset += 12;
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  for(i = 0; i < 12; i++)
			    {
			   	  buff[frameTail++] = 0xEE;
			    }
			  #endif
			}
		}
		da1 >>= 1;
		
		usleep(30000);
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D155
功能描述:响应主站请求二类数据"月冻结测量点分相反向有功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D155(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D153(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D156
功能描述:响应主站请求二类数据"月冻结测量点分相反向无功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D156(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D154(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D157
功能描述:响应主站请求二类数据"月冻结测量点分相正向有功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D157(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D153(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D158
功能描述:响应主站请求二类数据"月冻结测量点分相正向无功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D158(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D154(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D159
功能描述:响应主站请求二类数据"月冻结测量点分相反向有功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D159(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D153(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D160
功能描述:响应主站请求二类数据"月冻结测量点分相反向无功电能示值"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D160(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D154(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D161
功能描述:响应主站请求二类数据"日冻结正向有功电能示值(总、费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
修改历史:
  1.2012-05-23,修改,如果没有日冻结数据,则正向有功,反向有功读取当天的最后一个实时数据作为日冻结数据,
                 其它数据未作这个处理.
  2.2012-05-24,修改,有些单相表(如华立单相表)回的部分数据为0xff,这明显不是正确的数据,所以当0xEE处理
*************************************************************/
INT16U AFN0D161(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	METER_DEVICE_CONFIG meterConfig;
	INT8U               dataBuff[512];
	INT8U               da1, da2;
	INT16U              pn, tmpPn = 0;
	INT8U               i, tariff;
	INT8U               queryType, dataType, tmpDay;
	DATE_TIME           tmpTime, readTime;
	INT16U              offset, tmpFrameTail;
	INT8U               meterInfo[10];
  BOOL                bufHasData;

	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
      
  		queryMeterStoreInfo(pn, meterInfo);
			switch (meterInfo[0])
			{
			  case 1:    //单相智能表
			    if (fn<169)
			    {
			      queryType = SINGLE_PHASE_DAY;
			    }
			    else
			    {
			      queryType = SINGLE_PHASE_MONTH;  			    	
			    }
	        dataType  = ENERGY_DATA;
	        break;
	        
	      case 2:    //单相本地费控表
	      	if (fn<169)
	      	{
	      	  queryType = SINGLE_LOCAL_CTRL_DAY;
	      	}
	      	else
	      	{
	      	  queryType = SINGLE_LOCAL_CTRL_MONTH;
	      	}
	      	dataType  = ENERGY_DATA;
	      	break;

	      case 3:    //单相远程费控表
	      	if (fn<169)
	      	{
	      	  queryType = SINGLE_REMOTE_CTRL_DAY;
	      	}
	      	else
	      	{
	      	  queryType = SINGLE_REMOTE_CTRL_MONTH;
	      	}
	      	dataType  = ENERGY_DATA;
	      	break;
	      	
	      case 4:    //三相本地费控表
	      	if (fn<169)
	      	{
	      	  queryType = THREE_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = THREE_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;

	      case 5:    //三相本地费控表
	      	if (fn<169)
	      	{
	      	  queryType = THREE_LOCAL_CTRL_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = THREE_LOCAL_CTRL_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;

	      case 6:    //三相远程费控表
	      	if (fn<169)
	      	{
	      	  queryType = THREE_LOCAL_CTRL_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = THREE_LOCAL_CTRL_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;

	      case 8:    //重点用户
	      	if (fn<169)
	      	{
	      	  queryType = KEY_HOUSEHOLD_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = KEY_HOUSEHOLD_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;
	      	
	      default:
	      	if (fn<169)
	      	{
			      queryType = DAY_BALANCE;
	          dataType  = DAY_FREEZE_COPY_DATA;
	        }
	        else
	        {
			      queryType = MONTH_BALANCE;
	          dataType  = MONTH_FREEZE_COPY_DATA;
	        }
	        break;
	    }
	    
	    //单相表只有Fn161 Fn163 Fn177 Fn179,如果要读取除这4个Fn以外的数据应该让他回0xEE
	    if (!(fn==161 || fn==163 || fn==177 || fn==179))
	    {
	    	if (meterInfo[0]>0 && meterInfo[0]<4)
	    	{
			    if (fn<169)
			    {
			      queryType = DAY_BALANCE;
	          dataType  = DAY_FREEZE_COPY_DATA;
	        }
	        else
	        {
			      queryType = MONTH_BALANCE;
	          dataType  = MONTH_FREEZE_COPY_DATA;
	        }
	    	}
	    }
			    
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 161:	//日冻结正向有功电能示值
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x14;															  //DT2
					offset = POSITIVE_WORK_OFFSET;
					break;

				case 162:	//日冻结正向无功电能示值
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = POSITIVE_NO_WORK_OFFSET;
					break;
										
				case 163:	//日冻结反向有功电能示值
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					if (meterInfo[0]>0 && meterInfo[0]<4)
					{
					   offset = NEGTIVE_WORK_OFFSET_S;
					}
					else
					{
						 offset = NEGTIVE_WORK_OFFSET;
					}
					break;
				
				case 164:	//日冻结反向无功电能示值
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = NEGTIVE_NO_WORK_OFFSET;
					break;
					
				case 165:	//日冻结一象限无功电能示值
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA1_NO_WORK_OFFSET;
					break;

				case 166:	//日冻结二象限无功电能示值
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA2_NO_WORK_OFFSET;
					break;

				case 167:	//日冻结三象限无功电能示值
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA3_NO_WORK_OFFSET;
					break;

				case 168:	//日冻结四象限无功电能示值
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA4_NO_WORK_OFFSET;
					break;
					
				case 169:	//抄表日冻结正向有功电能示值
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset = POSITIVE_WORK_OFFSET;
					break;
					
				case 170:	//抄表日冻结正向无功电能示值
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = POSITIVE_NO_WORK_OFFSET;
					break;

				case 171:	//抄表日冻结反向有功电能示值
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = NEGTIVE_WORK_OFFSET;
					break;

				case 172:	//抄表日冻结反向无功电能示值
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = NEGTIVE_NO_WORK_OFFSET;
					break;

				case 173:	//抄表日冻结一象限无功电能示值
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA1_NO_WORK_OFFSET;
					break;

				case 174:	//抄表日冻结二象限无功电能示值
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA2_NO_WORK_OFFSET;
					break;

				case 175:	//抄表日冻结三象限无功电能示值
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA3_NO_WORK_OFFSET;
					break;

				case 176:	//抄表日冻结四象限无功电能示值
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA4_NO_WORK_OFFSET;
					break;

				case 177:	//月冻结正向有功电能示值
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = POSITIVE_WORK_OFFSET;
					break;

				case 178:	//月冻结正向无功电能示值
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = POSITIVE_NO_WORK_OFFSET;
					break;
										
				case 179:	//月冻结反向有功电能示值
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					if (meterInfo[0]>0 && meterInfo[0]<4)
					{
  					offset = NEGTIVE_WORK_OFFSET_S;
  			  }
  			  else
  			  {
  					offset = NEGTIVE_WORK_OFFSET;
			    }
					break;
					
				case 180:	//月冻结反向无功电能示值
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = NEGTIVE_NO_WORK_OFFSET;
					break;

				case 181:	//月冻结一象限无功电能示值
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset = QUA1_NO_WORK_OFFSET;
					break;

				case 182:	//月冻结二象限无功电能示值
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = QUA2_NO_WORK_OFFSET;
					break;

				case 183:	//月冻结三象限无功电能示值
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = QUA3_NO_WORK_OFFSET;
					break;

				case 184:	//月冻结四象限无功电能示值
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = QUA4_NO_WORK_OFFSET;
					break;
			}
			
			//冻结类数据时标
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			if(fn>=161 && fn<=176)
			{
				*offset0d = 3;
				buff[frameTail++] = tmpTime.day   = *(pHandle+0);
				buff[frameTail++] = tmpTime.month = *(pHandle+1);
				buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			}
			else
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				buff[frameTail++] = tmpTime.month = *(pHandle+0);
				buff[frameTail++] = tmpTime.year  = *(pHandle+1);
			}
			tariff = numOfTariff(pn);
			
			readTime = tmpTime;
      bufHasData = readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0);
      
		  //2012-05-23,add,如果日冻结示值没有的话,用当天最后一次成功的抄到的数据作为日冻结
		  if (bufHasData==FALSE && (fn==161 || fn==163))
		  {
        tmpTime = readTime;
        if (meterInfo[0]>0 && meterInfo[0]<4)
	      {
	      	bufHasData = readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
	      	
	      	if (bufHasData==FALSE)
	      	{
	      		//tmpTime = timeBcdToHex(tmpTime);
	      		
            //2012-12-10,修改
	      		tmpTime = readTime;
	      		bufHasData = readBakDayFile(pn, &tmpTime, dataBuff, 1);
	      	}
	      }
	      else
	      {
		      bufHasData = readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &tmpTime, 0);
		      
		      if (bufHasData==FALSE)
	      	{
	      		//tmpTime = timeBcdToHex(tmpTime);
            //2012-12-10,修改
            tmpTime = readTime;
	      		bufHasData = readBakDayFile(pn, &tmpTime, dataBuff, DAY_FREEZE_COPY_DATA);
	      	}
		    }
		  }
      
			if(bufHasData == TRUE)
			{
			  //终端抄表时间
			  buff[frameTail++] = tmpTime.minute;
			  buff[frameTail++] = tmpTime.hour;
			  buff[frameTail++] = tmpTime.day;
			  buff[frameTail++] = tmpTime.month;
			  buff[frameTail++] = tmpTime.year;
			
			  //费率数
			  buff[frameTail++] = tariff;
				
				for(i = 0; i <= tariff; i++)
				{
					if (fn==161 || fn==163 || fn==169 || fn==171 || fn==177 || fn==179)
					{
					  //2012-5-24,有些表回的是0xff,明显不是正常的数据
					  if((dataBuff[offset]!=0xEE) && (dataBuff[offset]!=0xFF))
					  {
						  buff[frameTail++] = 0x00;
					  }
					  else
					  {
						  buff[frameTail++] = 0xEE;
					  }
					}
					
					//2012-5-24,有些表回的是0xff,明显不是正常的数据
					if (dataBuff[offset]==0xff)
					{
					  buff[frameTail++] = 0xee;
					  buff[frameTail++] = 0xee;
					  buff[frameTail++] = 0xee;
					  buff[frameTail++] = 0xee;
					}
					else
					{
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					
					if (tariff==1)  //单费率的处理
					{
					  //费率1与总一样
					  if (fn==161 || fn==163 || fn==169 || fn==171 || fn==177 || fn==179)
					  {
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
					  }
					  else
					  {
						  buff[frameTail++] = buff[frameTail-4];
						  buff[frameTail++] = buff[frameTail-4];
						  buff[frameTail++] = buff[frameTail-4];
						  buff[frameTail++] = buff[frameTail-4];
					  }
					  
					  i++;
					}
					else            //多费率
					{
						;
					}
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
			    //终端抄表时间
			    buff[frameTail++] = tmpTime.minute;
			    buff[frameTail++] = tmpTime.hour;
			    buff[frameTail++] = tmpTime.day;
			    buff[frameTail++] = tmpTime.month;
			    buff[frameTail++] = tmpTime.year;
			
			    //费率数
			    buff[frameTail++] = tariff;
					
					if (fn==161 || fn==163 || fn==169 || fn==171 || fn==177 || fn==179)
					{
				    for(i = 0; i < (tariff + 1) * 5; i++)
			      {
			   	    buff[frameTail++] = 0xEE;
			      }
			    }
			    else
			    {
				    for(i = 0; i < (tariff + 1) * 4; i++)
			      {
			   	    buff[frameTail++] = 0xEE;
			      }
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D162
功能描述:响应主站请求二类数据"日冻结正向无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D162(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D163
功能描述:响应主站请求二类数据"日冻结反向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D163(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D164
功能描述:响应主站请求二类数据"日冻结反向无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D164(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D165
功能描述:响应主站请求二类数据"日冻结一象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D165(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D166
功能描述:响应主站请求二类数据"日冻结二象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D166(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D167
功能描述:响应主站请求二类数据"日冻结三象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D167(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D168
功能描述:响应主站请求二类数据"日冻结四象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D168(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D169
功能描述:响应主站请求二类数据"抄表日冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D169(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D170
功能描述:响应主站请求二类数据"抄表日冻结正向无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D170(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D171
功能描述:响应主站请求二类数据"抄表日冻结反向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D171(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D172
功能描述:响应主站请求二类数据"抄表日冻结反向无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D172(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D173
功能描述:响应主站请求二类数据"抄表日冻结一象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D173(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D174
功能描述:响应主站请求二类数据"抄表日冻结二象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D174(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D175
功能描述:响应主站请求二类数据"抄表日冻结三象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D175(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D176
功能描述:响应主站请求二类数据"抄表日冻结四象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D176(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D177
功能描述:响应主站请求二类数据"月冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D177(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D178
功能描述:响应主站请求二类数据"日冻结正向无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D178(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D179
功能描述:响应主站请求二类数据"月冻结反向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D179(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D180
功能描述:响应主站请求二类数据"日冻结反向无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D180(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D181
功能描述:响应主站请求二类数据"月冻结一象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D181(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D182
功能描述:响应主站请求二类数据"月冻结二象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D182(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D183
功能描述:响应主站请求二类数据"月冻结三象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D183(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D184
功能描述:响应主站请求二类数据"月冻结四象限无功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D184(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D185
功能描述:响应主站请求二类数据"日冻结正向有功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D185(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_REQ_RECORD];
	INT8U     da1, da2;
	INT16U    pn, tmpPn = 0;
	INT8U     i, tariff;
	INT8U     queryType, dataType, tmpDay;	
	DATE_TIME tmpTime;	
	INT16U    offset1, offset2, tmpFrameTail;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 185:	//日冻结正向有功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_WORK_OFFSET;
					offset2 = REQ_TIME_P_WORK_OFFSET;
					break;

				case 186:	//日冻结正向无功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_P_NO_WORK_OFFSET;
					break;

				case 187:	//日冻结反向有功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_WORK_OFFSET;
					offset2 = REQ_TIME_N_WORK_OFFSET;
					break;

				case 188:	//日冻结反向无功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_N_NO_WORK_OFFSET;
					break;
					
				case 189:	//抄表日冻结正向有功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_WORK_OFFSET;
					offset2 = REQ_TIME_P_WORK_OFFSET;
					break;

				case 190:	//抄表日冻结正向无功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_P_NO_WORK_OFFSET;
					break;

				case 191:	//抄表日冻结反向有功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_WORK_OFFSET;
					offset2 = REQ_TIME_N_WORK_OFFSET;
					break;

				case 192:	//抄表日冻结反向无功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_N_NO_WORK_OFFSET;
					break;

				case 193:	//月冻结正向有功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_WORK_OFFSET;
					offset2 = REQ_TIME_P_WORK_OFFSET;
					break;

				case 194:	//月冻结正向无功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_P_NO_WORK_OFFSET;
					break;

				case 195:	//月冻结反向有功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_WORK_OFFSET;
					offset2 = REQ_TIME_N_WORK_OFFSET;
					break;
				case 196:	//月冻结反向无功最大需量及发生时间(总,费率1~M)
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_N_NO_WORK_OFFSET;
					break;
			}
			
			//冻结类数据时标
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			
			if(fn >= 185 && fn <= 192)
			{
				*offset0d = 3;
				buff[frameTail++] = tmpTime.day   = *(pHandle+0);
				buff[frameTail++] = tmpTime.month = *(pHandle+1);
				buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			}
			else		//f193~f196
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				buff[frameTail++] = tmpTime.month = *(pHandle+0);
				buff[frameTail++] = tmpTime.year  = *(pHandle+1);
			}
			
			tariff = numOfTariff(pn);
			
			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
			{
			  //终端抄表时间
			  buff[frameTail++] = tmpTime.minute;
			  buff[frameTail++] = tmpTime.hour;
			  buff[frameTail++] = tmpTime.day;
			  buff[frameTail++] = tmpTime.month;
			  buff[frameTail++] = tmpTime.year;
			
			  //费率数
			  buff[frameTail++] = tariff;

				for(i = 0; i <= tariff; i++)
				{
					//最大需量
					buff[frameTail++] = dataBuff[offset1++];
					buff[frameTail++] = dataBuff[offset1++];
					buff[frameTail++] = dataBuff[offset1++];
					
					//最大需量发生时间
					buff[frameTail++] = dataBuff[offset2++];
					buff[frameTail++] = dataBuff[offset2++];
					buff[frameTail++] = dataBuff[offset2++];
					buff[frameTail++] = dataBuff[offset2++];
					if (tariff==1)   //单费率的处理
					{
						 //费率1与总一样
						 //最大需量
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 
						 //最大需量发生时间
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 
						 i++;
					}
					else
					{
					  offset2++;
					}
			  }
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
			    //终端抄表时间
			    buff[frameTail++] = tmpTime.minute;
			    buff[frameTail++] = tmpTime.hour;
			    buff[frameTail++] = tmpTime.day;
			    buff[frameTail++] = tmpTime.month;
			    buff[frameTail++] = tmpTime.year;
			
			    //费率数
			    buff[frameTail++] = tariff;

				  for(i = 0; i < (tariff + 1) * 7; i++)
			    {
			   	  buff[frameTail++] = 0xEE;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D186
功能描述:响应主站请求二类数据"日冻结正向无功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D186(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D187
功能描述:响应主站请求二类数据"日冻结反向有功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D187(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D188
功能描述:响应主站请求二类数据"日冻结反向无功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D188(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D189
功能描述:响应主站请求二类数据"抄表日冻结正向有功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D189(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D190
功能描述:响应主站请求二类数据"抄表日冻结正向无功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D190(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D191
功能描述:响应主站请求二类数据"抄表日冻结反向有功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D191(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D192
功能描述:响应主站请求二类数据"抄表日冻结反向无功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D192(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D193
功能描述:响应主站请求二类数据"月冻结正向有功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D193(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D194
功能描述:响应主站请求二类数据"月冻结正向无功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D194(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D195
功能描述:响应主站请求二类数据"月冻结反向有功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D195(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D196
功能描述:响应主站请求二类数据"月冻结反向无功最大需量及发生时间(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D196(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D201
功能描述:响应主站请求二类数据"第一时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D201(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D202
功能描述:响应主站请求二类数据"第二时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D202(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D203
功能描述:响应主站请求二类数据"第三时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D203(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D204
功能描述:响应主站请求二类数据"第四时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D204(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D205
功能描述:响应主站请求二类数据"第五时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D205(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D206
功能描述:响应主站请求二类数据"第六时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D206(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D207
功能描述:响应主站请求二类数据"第七时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D207(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D208
功能描述:响应主站请求二类数据"第八时区冻结正向有功电能示值(总,费率1~M)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D208(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D209
功能描述:响应主站请求二类数据"日冻结电能表远程控制通断电状态及记录"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D209(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U               dataBuff[768];
	INT8U               da1, da2;
	INT16U              pn, tmpPn = 0;
	INT8U               i, tariff;
	INT8U               queryType, dataType, tmpDay;
	DATE_TIME           tmpTime;
	INT16U              offset, tmpFrameTail;
	INT8U               meterInfo[10];	
	INT8U               buffHasData;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		 return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 209:
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x1a;															  //DT2
					offset    = POSITIVE_WORK_OFFSET;
					break;
					
			  case 215:
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x1a;															  //DT2
			  	break;
			}
			
			//冻结类数据时标
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			if(fn==209)
			{
				*offset0d = 3;
				buff[frameTail++] = tmpTime.day   = *(pHandle+0);
				buff[frameTail++] = tmpTime.month = *(pHandle+1);
				buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			}
			else
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				buff[frameTail++] = tmpTime.month = *(pHandle+0);
				buff[frameTail++] = tmpTime.year  = *(pHandle+1);
			}
			
			//查询测量点存储信息
  		queryMeterStoreInfo(pn, meterInfo);
  		
  		if (fn==209)
  		{
  		  if (meterInfo[0]<4)
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[2], ENERGY_DATA, &tmpTime, 0);
  		  }
  		  else
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[2], DAY_FREEZE_COPY_DATA, &tmpTime, 0);
  		  }
  		}
  		else  //FN=215
  		{
  		  if (meterInfo[0]<4)
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[3], ENERGY_DATA, &tmpTime, 0);
  		  }
  		  else
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[3], MONTH_FREEZE_COPY_DATA, &tmpTime, 0);
  		  }
  		}

			//终端抄表时间
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			if(buffHasData == TRUE)
			{ 
		    switch(fn)
		    {
		    	case 209:
		        switch (meterInfo[0])
		        {
  		        case 1:  //单相智能表(没有该项数据)
  		        case 4:  //三相智能表(没有/未抄该项数据)
  		        	for(i=0;i<11;i++)
  		        	{
  		        		 buff[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //单相本地费控表
  		        case 3:  //单相远程费控表
  		          //电能表通断电状态
  		          if (dataBuff[METER_STATUS_WORD_S_3]&0x40)
  		          {
  		            buff[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            buff[frameTail++] = 0x0;
  		          }
  		    
  		          //最近一次电能表远程控制通电时间
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		    
  		          //最近一次电能表远程控制断电时间
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		          break;

  		        case 5:  //三相本地费控表
  		        case 6:  //三相远程费控表
  		          //电能表通断电状态
  		          if (dataBuff[METER_STATUS_WORD_T_3]&0x40)
  		          {
  		            buff[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            buff[frameTail++] = 0x0;
  		          }
  		    
  		          //最近一次电能表远程控制通电时间
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		    
  		          //最近一次电能表远程控制断电时间
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		          break;
  		          
  		        default:
    		        //电能表通断电状态
    		        if (dataBuff[METER_STATUS_WORD_S_3]&0x40)
    		        {
    		          buff[frameTail++] = 0x11;
    		        }
    		        else
    		        {
    		          buff[frameTail++] = 0x0;
    		        }
    		    
    		        //最近一次电能表远程控制通电时间
    		        for(i=1;i<6;i++)
    		        {
    		          buff[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_S+i];
    		        }
    		        frameTail += 5;
    		    
    		        //最近一次电能表远程控制断电时间
    		        for(i=1;i<6;i++)
    		        {
    		          buff[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_S+i];
    		        }
    		        frameTail += 5;
    		        break;
  		      }
		        break;
		        
		      case 215:
		        switch (meterInfo[0])
		        {
  		        case 1:  //单相智能表(没有/未抄该项数据)
  		        case 3:  //单相远程费控表(没有/未抄该项数据)
  		        case 4:  //三相智能表(没有/未抄该项数据)
  		        case 6:  //三相远程费控表(没有/未抄该项数据)
  		        	for(i=1;i<36;i++)
  		        	{
  		        		 buff[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //单相本地费控表
     		      	//购电次数
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S+1];
     		      	
     		      	//剩余金额
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_S]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+3];
     		      	
     		      	//累计购电金额
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_S]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+3];
     		      	
     		      	//剩余电量
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+3];
     		      	
     		      	//透支电量
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+3];
     		      	
     		      	//累计购电量
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+3];
     		      	
     		      	//赊欠门限电量
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+3];
     		      	
     		      	//报警电量
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+3];
     		      	
     		      	//故障电量
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
  		        	break;
  		        	
  		        case 5:  //三相本地费控表
     		      	//购电次数
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T+1];
     		      	
     		      	//剩余金额
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_T]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+3];
     		      	
     		      	//累计购电金额
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_T]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+3];
     		      	
     		      	//剩余电量
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+3];
     		      	
     		      	//透支电量
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+3];
     		      	
     		      	//累计购电量
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+3];
     		      	
     		      	//赊欠门限电量
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+3];
     		      	
     		      	//报警电量
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+3];
     		      	
     		      	//故障电量
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
  		        	break;
  		        	
  		        default:
     		      	//购电次数
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_TIME];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_TIME+1];
     		      	
     		      	//剩余金额
     		      	if (dataBuff[CHARGE_REMAIN_MONEY]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+3];
     		      	
     		      	//累计购电金额
     		      	if (dataBuff[CHARGE_TOTAL_MONEY]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+3];
     		      	
     		      	//剩余电量
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+3];
     		      	
     		      	//透支电量
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+3];
     		      	
     		      	//累计购电量
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+3];
     		      	
     		      	//赊欠门限电量
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+3];
     		      	
     		      	//报警电量
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+3];
     		      	
     		      	//故障电量
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	break;
     		    }
		      	break;
			  }				
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
					switch (fn)
					{
					  case 209:
				      memset(&buff[frameTail],0xee,1);
				      frameTail += 11;
				      break;
				      
				    case 215:
				      memset(&buff[frameTail],0xee,36);
				      frameTail += 36;
				      break;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D213
功能描述:响应主站请求二类数据"电能表开关操作次数及时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D213(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D214
功能描述:响应主站请求二类数据"电能表参数修改次数及时间"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D214(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D215
功能描述:响应主站请求二类数据"电能表购、用电信息"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D215(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D209(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
函数名称:AFN0D216
功能描述:响应主站请求二类数据"电能表结算信息"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D216(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D217
功能描述:响应主站请求二类数据"台区集中抄表载波主节点白噪声曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D217(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 7;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D218
功能描述:响应主站请求二类数据"台区集中抄表载波主节点色噪声曲线"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D218(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 7;
	
	return frameTail;
}

/*************************************************************
函数名称:AFN0D221
功能描述:响应主站请求二类数据"终端日冻结网络运行统计数据(重庆规约)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*************************************************************/
INT16U AFN0D221(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;	
	INT8U                  i, tmpDay;	
	DATE_TIME              tmpTime;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//冻结类数据时标Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	buff[frameTail++] = tmpTime.day   = *pHandle++;
	buff[frameTail++] = tmpTime.month = *pHandle++;
	buff[frameTail++] = tmpTime.year  = *pHandle++;


  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  {
		//最小信号强度
		buff[frameTail++] = terminalStatisRecord.minSignal;
		
		//最小信号强度时间
		buff[frameTail++] = hexToBcd(terminalStatisRecord.minSignalTime[0]);
		buff[frameTail++] = hexToBcd(terminalStatisRecord.minSignalTime[1]);

		//最大信号强度
		buff[frameTail++] = terminalStatisRecord.maxSignal;
		
		//最大信号强度时间
		buff[frameTail++] = hexToBcd(terminalStatisRecord.maxSignalTime[0]);
		buff[frameTail++] = hexToBcd(terminalStatisRecord.maxSignalTime[1]);
  }
  else
  {
	  #ifdef NO_DATA_USE_PART_ACK_03
      frameTail -= 7;
    #else
      for(i = 0; i < 6; i++)
      {
   	    buff[frameTail++] = 0xee;
      }
    #endif
  }

	*offset0d = 3;
  
	return frameTail;	 
}
