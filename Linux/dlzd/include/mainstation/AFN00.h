/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����AFN00.h
���ߣ�leiyong
�汾��0.9
�������:2006��7��
������AFN00ͷ�ļ���
�޸���ʷ��
  01,06-7-28,leiyong created.
**************************************************/

#ifndef __INCAfn00H
#define __INCAfn00H

#include "common.h"

void AFN00(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom);
void ackOrNack(BOOL ack,INT8U dataFrom);
void AFN00003(INT8U ackNum, INT8U dataFrom, INT8U afn);
void AFN00004(INT8U dataFrom, INT8U errorType, INT8U *data);

#endif