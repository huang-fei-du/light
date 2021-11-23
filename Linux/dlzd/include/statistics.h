/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名:statistics.h
作者：leiyong
版本：0.9
完成日期:2010年3月
描述：电力终端(负控终端、集中器)统计处理头文件
修改历史：
  01,10-03-19,leiyong created.

**************************************************/
#ifndef __INCstatisticsH
#define __INCstatisticsH

#include "workWithMeter.h"

void initTeStatisRecord(TERMINAL_STATIS_RECORD *teRecord);
void initMpStatis(void *record, INT8U type);
void frameReport(INT8U type, INT16U len, INT8U item);
void powerOffEvent(BOOL powerOff);
void dayPowerOnAmount(void);
void teResetReport(void);
void dcAnalogStatis(INT16U dcNow);
INT32U format2ToHex(INT16U format2Data);

#endif    /*__INCstatisticsH*/