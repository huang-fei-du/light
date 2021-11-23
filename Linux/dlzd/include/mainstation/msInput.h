/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：msInput.h
作者：wan guihua
版本：0.1
完成日期: 年 月 日
描述：主站输入信息头文件
修改历史：
  
**************************************************/
#ifndef __msInputH
#define __msInputH

#include "common.h"

//xMega帧分析
typedef struct
{
   INT16U len;
   INT8U  address;
   INT8U  *pData;
   INT8U  *pDataEnd;
   INT8U  afn;
   INT8U  seq;
   INT8U  cs;
}ANALYSE_XMEGA_FRAME;

//函数声明
INT8U  msInput(INT8U *pFrame,INT16U dataLength,INT8U dataFrom);
void   crcKeyCount(INT8U pw0, INT8U pw1);
INT16U countCRC(INT8U *pData);
INT8U  findFnPn(INT8U data1, INT8U data2,INT8U type);
void   addFrameFlag(BOOL activeReport,BOOL singleFrame);
void   *threadOfmsLocalReceive(void *arg);

#ifdef WDOG_USE_X_MEGA
 void  xMegaReceiving(INT8U * pFrame, INT16U dataLen);
#endif

#endif  /*__msInputH*/
