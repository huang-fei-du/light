/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ���:statistics.h
���ߣ�leiyong
�汾��0.9
�������:2010��3��
�����������ն�(�����նˡ�������)ͳ�ƴ���ͷ�ļ�
�޸���ʷ��
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