/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ�����msInput.h
���ߣ�wan guihua
�汾��0.1
�������: �� �� ��
��������վ������Ϣͷ�ļ�
�޸���ʷ��
  
**************************************************/
#ifndef __msInputH
#define __msInputH

#include "common.h"

//xMega֡����
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

//��������
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
