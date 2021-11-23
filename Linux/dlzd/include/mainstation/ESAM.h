/*************************************************
Copyright,2011,Huawei WoDian co.,LTD
文件名：ESAM.h
作者：leiyong
版本：0.9
完成日期:2011年3月
描述：ESAM操作头文件。
修改历史：
  01,11-03-09,leiyong created.
**************************************************/

#ifndef __INCEsamH
#define __INCEsamH

#include "common.h"

extern BOOL  hasEsam;             //有ESAM芯片吗?
extern INT8U esamSerial[8];       //ESAM芯片序列号

void   resetEsam(void);
INT16U putGetBytes(INT8U *cmdBuf, INT8U lenOfCmd, INT8U respondType, INT8U *recvBuf,INT8U lenOfRecv);
void   getChallenge(INT8U *buf);

#endif   //__INCEsamH