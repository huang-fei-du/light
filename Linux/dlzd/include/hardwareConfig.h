/*************************************************
Copyright,2010,Huawei WoDian co.,LTD
文件名：hardwareConfig.h
作者：leiyong
版本：0.9
完成日期:2010年01月
描述：电力终端(负控终端、集中器)硬件配置头文件
修改历史：
  01,10-01-16,leiyong created.

**************************************************/
#ifndef __HardwareConfigH
#define __HardwareConfigH

#include "common.h"

extern int fdOfModem;     //串口1(无线Modem)文件描述符
extern int fdOfttyS2;     //串口2(本地控制口)文件描述符
extern int fdOfttyS3;     //串口3(485接口)文件描述符
extern int fdOfttyS4;     //串口4(485接口)文件描述符
extern int fdOfCarrier;   //串口5(载波接口)文件描述符
extern int fdOfLocal;     //串口6(本地维护接口)文件描述符
extern int fdOfSample;    //交采文件描述符
extern int fdOfIoChannel; //I/O通道文件描述符

//函数声明
int   initHardware(void);
void  oneUartConfig(INT32U fd,INT8U ctrlWord);
void  uart0Config(INT8U rate);
void  resetWlModem(INT8U press);
void  wlModemPowerOnOff(INT8U onOff);
void  sendLocalMsFrame(INT8U *pack,INT16U length);
void  sendWirelessFrame(INT8U *pack,INT16U length);
void  sendDebugFrame(INT8U *pack,INT16U length);
void  setBeeper(INT8U ifVoice);
void  alarmLedCtrl(INT8U ifLight);

void  sendToAvr(INT8U *data,INT16U len);
INT8U getGateKValue(void);
void  detectReset(BOOL secondChanged);

void  sendXmegaFrame(INT8U afn, INT8U *data, INT16U len);
void  carrierUartInit(void);

#endif /*__HardwareConfigH*/
