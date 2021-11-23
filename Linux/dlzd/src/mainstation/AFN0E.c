/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN0E.c
作者：TianYe
版本：0.9
完成日期：2006年8月
描述：主站"请示三类数据"(AFN0E)的处理函数
函数列表：
修改历史：
  01,06-8-9,TinaYe created.
  02,06-10-17,leiyong,作分帧处理
***************************************************/

#include "common.h"
#include "teRunPara.h"
#include "convert.h"

#include "dataBase.h"
#include "AFN0E.h"
#include "workWithMeter.h"

/*******************************************************
函数名称:AFN0E
功能描述:主站"请示三类数据"(AFN0E)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN0E(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom, INT8U poll)
{
    INT8U      fn, pn,tmpPm, i;
    INT16U     pointm, pointn;
    INT16U     eventNum;
    INT16U     frameTail0e;
    INT8U      frameCounter,*pHandle;
    INT16U     tmpI,tmpHead0e,tmpHead0eActive;    
    INT32U     baseAddr;
    INT16U     blkInSect;    
    INT32U     addrHead, addrWriteTo;
    INT8U     *pTpv;                   //TpV指针
    EVENT_HEAD eventHead;
    INT16U     (*readEvent[36])(INT16U frameTail, INT8U *eventData);
    INT8U      tmpReadBuff[200];
    DATE_TIME  tmpTime;
    TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
    
    for(i=0;i<36;i++)
    {
    	readEvent[i] = NULL;
    }
    
    readEvent[0] = eventErc01;
    readEvent[1] = eventErc02;
    readEvent[2] = eventErc03;
    readEvent[3] = eventErc04;
    readEvent[4] = eventErc05;
    readEvent[5] = eventErc06;
    readEvent[6] = eventErc07;
    readEvent[7] = eventErc08;
    readEvent[8] = eventErc09;
    readEvent[9] = eventErc10;
    readEvent[10] = eventErc11;    
    readEvent[11] = eventErc12;
    readEvent[12] = eventErc13;
    readEvent[13] = eventErc14;

    readEvent[16] = eventErc17;
    
    readEvent[18] = eventErc19;
    readEvent[19] = eventErc20;
    readEvent[20] = eventErc21;
    readEvent[21] = eventErc22;
    readEvent[22] = eventErc23;
    
    readEvent[23] = eventErc24;
    readEvent[24] = eventErc25;
    readEvent[25] = eventErc26;
    readEvent[26] = eventErc27;
    readEvent[27] = eventErc28;
    readEvent[28] = eventErc29;
    readEvent[29] = eventErc30;
    
    readEvent[30] = eventErc31;
    readEvent[31] = eventErc32;
    readEvent[32] = eventErc33;
    readEvent[34] = eventErc35;
	 #ifdef LIGHTING
	  readEvent[35] = eventErc36;
   #endif	
        
    //根据数据单元标识的值,查找FN值，确定操作函数号
    pn = findFnPn(*pDataHead, *(pDataHead+1), FIND_PN);
    fn = findFnPn(*(pDataHead+2), *(pDataHead+3), FIND_FN);
    if (pn != 0 || fn > 2 || fn < 1)
    {
       return;
    }

    pHandle = pDataHead + 4;
    pointm = *pHandle++;      //Pm
    pointn = *pHandle;        //Pn
    
    //if (pointm<1)
    //{
    //	pointm = 1;
    //}
    //if (pointn<1)
    //{
    //	pointn = 1;
    //}
    
    printf("pm=%d,pn=%d\n",pointm,pointn);
    
    //计算请求事件的个数
    eventNum = 0;
    if (pointm < pointn)
    {
      eventNum = pointn - pointm;
    }
    else
    {
      if (pointm > pointn)
      {
        eventNum = 256 + pointn - pointm;
      }
      else
      {
      	eventNum = 1;
      }
    }
    
    //如果请求事件数大于事件记数器值，返回无请求数据帧
    if (fn == 1)
    {
      if (eventNum > iEventCounter)
      {
        //返回无数据帧
        ackOrNack(FALSE, dataFrom);
        return;
      }
      if (pointn > iEventCounter || (pointm >= iEventCounter && pointn < pointm))
      {
        //返回无数据帧
        ackOrNack(FALSE, dataFrom);
        return;
      }
    }
    else
    {
      if (eventNum > nEventCounter)
      {
        ackOrNack(FALSE, dataFrom);
        return;
      }
      if (pointn > nEventCounter || (pointm >= nEventCounter && pointn < pointm))
      {
        //返回无数据帧
        ackOrNack(FALSE, dataFrom);
        return;
      }
    }
    
    //定位pm的存储地址(还未做)

    //正常处理    
    frameCounter = 0;
    tmpPm = pointm;

    if (fQueue.tailPtr == 0)
    {
       tmpHead0e = 0;
    }
    else
    {
       tmpHead0e = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    frameTail0e = tmpHead0e + 22;

    printf("即将进入循环pointm=%d,pointn=%d\n",pointm,pointn);
    //for(; pointm<pointn; pointm++)
    for(; eventNum > 0; eventNum--,pointm++)
    {
      printf("循环中pointm=%d,pointn=%d\n",pointm,pointn);
      tmpTime = sysTime;
      if (readMeterData(tmpReadBuff, fn , EVENT_RECORD, pointm+1, &tmpTime, 0)==TRUE)
      {
          eventHead.erc = tmpReadBuff[0];
          
          if (debugInfo&PRINT_EVENT_DEBUG)
          { 
            printf("ERC=%d\n",eventHead.erc);
          }
          
         #ifdef LIGHTING
					if (eventHead.erc >= 1 && eventHead.erc <= 36)
				 #else
					if (eventHead.erc >= 1 && eventHead.erc <= 35)
				 #endif
          {
            if (readEvent[eventHead.erc-1]!=NULL)
            {
              frameTail0e = readEvent[eventHead.erc-1](frameTail0e, tmpReadBuff);
            }
          }
      }
      else
      {
      	
      }
      
      //事件逐项处理完毕或者发送缓冲满，发送
      if ((frameTail0e-tmpHead0e) > MAX_OF_PER_FRAME || (eventNum <=1))
      {
        if (poll != ACTIVE_REPORT)
        {
          //根据启动站要求判断是否携带TP
          if (frame.pTp != NULL)
          {
             pTpv = frame.pTp;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv++;
             msFrame[frameTail0e++] = *pTpv;
          }
        }

        msFrame[tmpHead0e+0] = 0x68;                  //帧起始字符
        
        tmpI = ((frameTail0e-tmpHead0e-6) << 2) | PROTOCOL_FIELD;
        msFrame[tmpHead0e+1] = tmpI & 0xFF;           //L
        msFrame[tmpHead0e+2] = tmpI >> 8;
        msFrame[tmpHead0e+3] = tmpI & 0xFF;           //L
        msFrame[tmpHead0e+4] = tmpI >> 8;
        
        msFrame[tmpHead0e+5] = 0x68;                  //帧起始字符
        
        if (poll==ACTIVE_REPORT)
        {
           msFrame[tmpHead0e + 6] = 0xc4;     //DIR=1,PRM=1,功能码=0x4
        }
        else
        {
           msFrame[tmpHead0e + 6] = 0x88;     //控制字节10001000(DIR=1,PRM=0,功能码=0x8)
        }
          
        //地址
        msFrame[tmpHead0e+7]  = addrField.a1[0];
        msFrame[tmpHead0e+8]  = addrField.a1[1];
        msFrame[tmpHead0e+9]  = addrField.a2[0];
        msFrame[tmpHead0e+10] = addrField.a2[1];
        
        if (poll == ACTIVE_REPORT)
        {
           msFrame[tmpHead0e+11] = 0x0;
        }
        else
        {
           msFrame[tmpHead0e+11] = addrField.a3;
        }
        
        msFrame[tmpHead0e+12] = 0x0E;                //AFN

        msFrame[tmpHead0e+13] = 0;
        if (poll==ACTIVE_REPORT)
        {
        	//2015-2-10,主动上报事件需要确认
         	msFrame[tmpHead0e+13] |= 0x10;
        }
        else
        {
          //非主动上报且下行报文带时标
         	if (frame.pTp != NULL)
         	{
         	  msFrame[tmpHead0e+13] |= 0x80;       //TpV置位
         	}
        }
            
        pHandle = pDataHead;
        
        msFrame[tmpHead0e+14] = *pHandle++;
        msFrame[tmpHead0e+15] = *pHandle++;
        msFrame[tmpHead0e+16] = *pHandle++;
        msFrame[tmpHead0e+17] = *pHandle;
        
        msFrame[tmpHead0e+18] = iEventCounter;
        msFrame[tmpHead0e+19] = nEventCounter;

        msFrame[tmpHead0e+20] = tmpPm;          //本帧报文传送的事件记录起始指针
        msFrame[tmpHead0e+21] = pointm+1;         //本帧报文传送的事件记录结束指针
        tmpPm = pointm;
                       
        frameTail0e++;
        msFrame[frameTail0e++] = 0x16;

        fQueue.frame[fQueue.tailPtr].head = tmpHead0e;
        fQueue.frame[fQueue.tailPtr].len  = frameTail0e-tmpHead0e;
        
        if (poll==ACTIVE_REPORT)
        {
        	 printf("复制帧\n");
        	 //将组好的帧复制到主动上报帧队列中
           if (fQueue.activeTailPtr == 0)
           {
              tmpHead0eActive = 0;
           }
           else
           {
              tmpHead0eActive = fQueue.activeFrame[fQueue.activeTailPtr-1].head + fQueue.activeFrame[fQueue.activeTailPtr-1].len;
           }
           fQueue.activeFrame[fQueue.activeTailPtr].head = tmpHead0eActive;
           fQueue.activeFrame[fQueue.activeTailPtr].len  = fQueue.frame[fQueue.tailPtr].len;
           for(tmpI=0;tmpI<fQueue.activeFrame[fQueue.activeTailPtr].len;tmpI++)
           {
           	 activeFrame[tmpHead0eActive+tmpI] = msFrame[fQueue.frame[fQueue.tailPtr].head+tmpI];
           }

           if ((tmpHead0eActive+tmpI+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
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
           tmpHead0e = frameTail0e;
           
           if ((tmpHead0e+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
           	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
           {
             fQueue.frame[fQueue.tailPtr].next = 0x0;
           	 fQueue.tailPtr = 0;
           	 tmpHead0e = 0;
           }
           else
           {
              fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
              fQueue.tailPtr++;
           }
        }
        
        frameTail0e = tmpHead0e+22;  //frameTail重新置为21填写下一帧
      }
    }

    printf("退出循环pointm=%d,pointn=%d\n",pointm,pointn);
   
    //更新终端统计记录    
    eventReadedPointer[0] = iEventCounter;
    if (pointn<=255)
    {
    	eventReadedPointer[1] = pointn;
    }
    else
    {
    	eventReadedPointer[1] = 255;
    }
    
    if (debugInfo&PRINT_EVENT_DEBUG)
    {
      printf("AFN0E:已读事件指针=%d\n",eventReadedPointer[1]);
    }
    
    saveParameter(88, 2,&eventReadedPointer, 2);
}

/*******************************************************
函数名称:eventErc01
功能描述:填写erc01事件记录上报帧的数据部分(数据初始化和版本变更记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc01(INT16U frameTail, INT8U *eventData)
{    
   msFrame[frameTail++] = ERC(1);     //erc
   msFrame[frameTail++] = 0x0E;       //长度
  
   //参数更新时间
   eventData += 3;
   
	 msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   msFrame[frameTail++] = *eventData;
   eventData += 2;
   
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;

   msFrame[frameTail++] = *eventData++;
   msFrame[frameTail++] = *eventData++;
   
   return frameTail;
}

/*******************************************************
函数名称:eventErc02
功能描述:填写erc02事件记录上报帧的数据部分(参数丢失记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc02(INT16U frameTail, INT8U *eventData)
{
    msFrame[frameTail++] = ERC(2);
    msFrame[frameTail++] = 0x6;
    
    eventData += 3;
    
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
      
    msFrame[frameTail++] = *eventData++;
    
    return frameTail;
}

/*******************************************************
函数名称:eventErc03
功能描述:填写erc03事件记录上报帧的数据部分(参数变更记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc03(INT16U frameTail, INT8U *eventData)
{
    INT8U tmpNum;
    
    msFrame[frameTail++] = ERC(3);       //erc
    
    tmpNum = *(eventData+1);
    msFrame[frameTail++] = 6+tmpNum*4;   //长度
  
    //参数更新时间
    eventData += 3;
	  msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    msFrame[frameTail++] = *eventData++;
    
    msFrame[frameTail++] = *eventData++;
    
    //数据单元标识
    for(;tmpNum>0;tmpNum--)
    {
      msFrame[frameTail++] = *eventData++;
      msFrame[frameTail++] = *eventData++;
    
      msFrame[frameTail++] = *eventData++;
      msFrame[frameTail++] = *eventData++;
    }

    return frameTail;
}

/*******************************************************
函数名称:eventErc04
功能描述:填写erc04事件记录上报帧的数据部分(状态量变位记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc04(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(4);
	msFrame[frameTail++] = 0x7;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc05
功能描述:填写erc05事件记录上报帧的数据部分(遥控跳闸记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc05(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(5);
	msFrame[frameTail++] = 0x0a;
	
	eventData += 3;
	
	//跳闸时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//跳闸轮次
  msFrame[frameTail++] = *eventData++;
	
	//跳闸时功率(总加功率)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//跳闸后2min的功率
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc06
功能描述:填写erc06事件记录上报帧的数据部分(功控跳闸记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc06(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(6);
	msFrame[frameTail++] = 0x0E;
	
	eventData += 3;
	
	//跳闸时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//总加组号
	msFrame[frameTail++] = *eventData++;
	
	//跳闸轮次
	msFrame[frameTail++] = *eventData++;

	//功控类别
	msFrame[frameTail++] = *eventData++;
	
	//跳闸前功率(总加功率)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//跳闸后2min的功率
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //跳闸时的功率定值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
		
	return frameTail;
}

/*******************************************************
函数名称:eventErc07
功能描述:填写erc07事件记录上报帧的数据部分(电控跳闸记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc07(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(7);
	msFrame[frameTail++] = 0x10;
	
	eventData += 3;
	
	//跳闸时间(分时日月年)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//总加组号
	msFrame[frameTail++] = *eventData++;
	  
	//跳闸轮次
	msFrame[frameTail++] = *eventData++;
	
	//电控类别
	msFrame[frameTail++] = *eventData++;

  //跳闸时电能量(总加电能量)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//跳闸时电能量定值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc08
功能描述:填写erc05事件记录上报帧的数据部分(电能量参数变更)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc08(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(8);
	msFrame[frameTail++] = 0x8;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点  
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//变更标志
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc09
功能描述:填写erc09事件记录上报帧的数据部分(电流回路异常)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc09(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(9);
	msFrame[frameTail++] = 28;
	
	eventData += 3;
	
	//发生时间
  msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//异常标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ia
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//发生时的Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时电能表自向有功总电能示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc10
功能描述:填写erc10事件记录上报帧的数据部分(电压回路异常)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc10(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(10);
	
	msFrame[frameTail++] = 0x1C;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点号及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//异常标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ia
  msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	  
	//发生时的Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//发生时的Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//发生时的正向有功示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc11
功能描述:填写erc11事件记录上报帧的数据部分(相序异常)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc11(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(11);
	
	msFrame[frameTail++] = 0x18;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点号及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//∠Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//∠Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//∠Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//∠Ia
  msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	  
	//∠Ib
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//∠Ic
	msFrame[frameTail++] = *eventData++;
  msFrame[frameTail++] = *eventData++;
	
	//发生时的正向有功示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc12
功能描述:填写erc12事件记录上报帧的数据部分(电能表时间超差)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc12(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(12);
	
	msFrame[frameTail++] = 0x7;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc13
功能描述:填写erc13事件记录上报帧的数据部分(电表故障信息)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc13(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(13);
	msFrame[frameTail++] = 0x8;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点(及发生标志)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //异常标志
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc14
功能描述:填写erc14事件记录上报帧的数据部分(终端停/上电事件)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc14(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(14);
	msFrame[frameTail++] = 0x0A;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	return frameTail;
}

/*******************************************************
函数名称:eventErc17
功能描述:电压/电流不平衡度超限记录
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc17(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(17);
	
	msFrame[frameTail++] = 27;
	
	eventData += 2;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;  //min
	msFrame[frameTail++] = *eventData++;  //hour
	msFrame[frameTail++] = *eventData++;  //day
	msFrame[frameTail++] = *eventData++;  //month
	msFrame[frameTail++] = *eventData++;  //month

	//测量点及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //异常标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的电压不平衡度
	msFrame[frameTail++] = *eventData++;	//unbalanceU
	msFrame[frameTail++] = *eventData++;
	
	//发生时的电流不平衡度
	msFrame[frameTail++] = *eventData++;  //unbalanceC
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ia
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	return frameTail;
}


/*******************************************************
函数名称:eventErc19
功能描述:填写erc19事件记录上报帧的数据部分(购电参数设置记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc19(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(19);
	msFrame[frameTail++] = 0x1f;
	
	eventData += 3;
	
	//购电能量设置时间(分时日月年)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//总加组号
	msFrame[frameTail++] = *eventData++;
	
	//购电单号
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//追加/刷新标志  
	msFrame[frameTail++] = *eventData++;
	
	//购电量值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//报警门限
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//跳闸门限
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//购电前剩余电能量
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//购电后剩余电能量
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc20
功能描述:填写erc20事件记录上报帧的数据部分(消息认证错误记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc20(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(20);
	msFrame[frameTail++] = 0x16;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc21
功能描述:填写erc21事件记录上报帧的数据部分(终端故障记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc21(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(21);
	msFrame[frameTail++] = 0x06;
	
	eventData += 3;
	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc22
功能描述:填写erc22事件记录上报帧的数据部分(有功总电能量差动越限事件)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc22(INT16U frameTail, INT8U *eventData)
{
	INT16U tmpTail;
	INT8U i,tmpCount,tmpCountx;
	
	msFrame[frameTail++] = ERC(22);
	tmpTail = frameTail++;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//电能量差动组号及起/止标志
	msFrame[frameTail++] = *eventData++;
	
	//越限时对比总加组有功总电能量
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//越限时参照总加组有功总电能量
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//越限时差动越限相对偏差值
	msFrame[frameTail++] = *eventData++;

	//越限时差动越限绝对偏差值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//对比的总加组测量点数量n
	msFrame[frameTail++] = tmpCount = *eventData++;
	
	for(i=0;i<tmpCount;i++)
  {
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
  }

	//参照的总加组测量点数量m
	msFrame[frameTail++] = tmpCountx = *eventData++;
	
	for(i=0;i<tmpCountx;i++)
  {
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
	  msFrame[frameTail++] = *eventData++;
  }

	msFrame[tmpTail] = (tmpCount+tmpCountx)*5+20;

	return frameTail;
}

/*******************************************************
函数名称:eventErc23
功能描述:填写erc23事件记录上报帧的数据部分(电控告警事件记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc23(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(23);
	msFrame[frameTail++] = 0x10;
	
	eventData += 3;
	
	//告警时间(分时日月年)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//总加组号
	msFrame[frameTail++] = *eventData++;
	  
	//投入轮次
	msFrame[frameTail++] = *eventData++;
	
	//电控类别
	msFrame[frameTail++] = *eventData++;

  //告警时电能量(总加电能量)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//告警时电能量定值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc24
功能描述:填写erc24事件记录上报帧的数据部分(电压越限记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc24(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(24);
  
	msFrame[frameTail++] = 0x0E;
	
	eventData += 2;	
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
		
	//越限标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ua/Uab
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ub
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Uc/Ucb
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc25
功能描述:填写erc25事件记录上报帧的数据部分(电流越限记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc25(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(25);

	msFrame[frameTail++] = 17;
	
	eventData += 2;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点及起/止标志
  msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
		
	//越限标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ia
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ib
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的Ic
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc26
功能描述:填写erc26事件记录上报帧的数据部分(视在功率越限记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc26(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(26);
	
	msFrame[frameTail++] = 0x0E;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//超限标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的视在功率  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	//发生时的视在功率限值
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc27
功能描述:填写erc26事件记录上报帧的数据部分(电能表示度下降记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc27(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(27);
	
	msFrame[frameTail++] = 0x11;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点(及起/止标志)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//下降前电能表正向有功电能总示值
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//下降后电能表正向有功电能总示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc28
功能描述:填写erc28事件记录上报帧的数据部分(电能量超差记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc28(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(28);
	msFrame[frameTail++] = 0x12;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//测量点(及起/止标志)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//超差发生前正向有功总电能示值	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;

	//超差发生后正向有功总电能示值	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//超差阈值
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc29
功能描述:填写erc29事件记录上报帧的数据部分(电能表飞走记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc29(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(29);
	msFrame[frameTail++] = 0x12;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点(及起/止标志)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	//飞走发生前正向有功总电能示值	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;

	//飞走发生后正向有功总电能示值	
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//飞走阈值
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc30
功能描述:填写erc30事件记录上报帧的数据部分(电能表停走记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc30(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(30);
	
	msFrame[frameTail++] = 0x0D;  
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点(及起/止标志)
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//停走发生时的正向有功总电能示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;  
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;
	
	//停走阈值
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc31
功能描述:填写erc31事件记录上报帧的数据部分(终端485抄表失败事件记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc31(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(31);
	
	msFrame[frameTail++] = 21;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//最后一次抄表成功时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;	
	msFrame[frameTail++] = *eventData++;	
	
	//最后一次抄表成功正向有功电能示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;

	//最后一次抄表成功反向有功电能示值
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc32
功能描述:填写erc32事件记录上报帧的数据部分(终端与主站通信流量超门限事件记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc32(INT16U frameTail, INT8U *eventData)
{
  msFrame[frameTail++] = ERC(32);
	
	msFrame[frameTail++] = 13;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//当月已发生的通信流量
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;	
	
	//月通信流量门限
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}

/*******************************************************
函数名称:eventErc33
功能描述:填写erc33事件记录上报帧的数据部分(电能表运行状态字变位事件记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc33(INT16U frameTail, INT8U *eventData)
{
  INT8U i;
  
  msFrame[frameTail++] = ERC(33);
	
	msFrame[frameTail++] = 35;
	
	eventData += 3;

	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //测量点
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
  
  //变位标志1-7及电能表运行状态字1--7
  for(i=0;i<28;i++)
  {
	  msFrame[frameTail++] = *eventData++;  	
  }
  
  return frameTail;
}

/*******************************************************
函数名称:eventErc35
功能描述:填写erc35事件记录上报帧的数据部分(发现未知电表事件记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc35(INT16U frameTail, INT8U *eventData)
{
  INT16U tmpTail;
  
  msFrame[frameTail++] = ERC(35);
	tmpTail = frameTail++;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	  
	msFrame[frameTail++] = *eventData++;  //终端通信端口号
	msFrame[frameTail++] = *eventData;    //发现块数
	memcpy(&msFrame[frameTail],eventData+1,*eventData*8);
	frameTail+= *eventData*8;
	msFrame[tmpTail] = *eventData*8+7;
	
	return frameTail;
}


#ifdef LIGHTING
/*******************************************************
函数名称:eventErc36
功能描述:填写erc26事件记录上报帧的数据部分(功率因数越限记录)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U eventErc36(INT16U frameTail, INT8U *eventData)
{
	msFrame[frameTail++] = ERC(36);
	
	msFrame[frameTail++] = 0x0C;
	
	eventData += 3;
	
	//发生时间
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//测量点及起/止标志
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//超限标志
	msFrame[frameTail++] = *eventData++;
	
	//发生时的功率因数  
	msFrame[frameTail++] = *eventData++;
	msFrame[frameTail++] = *eventData++;
	
	//发生时的功率因数限值
	msFrame[frameTail++] = *eventData++;	  
	msFrame[frameTail++] = *eventData++;
	
	return frameTail;
}
#endif