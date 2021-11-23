/*************************************************
Copyright,2011,Huawei WoDian co.,LTD
�ļ�����ESAM.h
���ߣ�leiyong
�汾��0.9
�������:2011��3��
������ESAM����ͷ�ļ���
�޸���ʷ��
  01,11-03-09,leiyong created.
**************************************************/

#ifndef __INCEsamH
#define __INCEsamH

#include "common.h"

extern BOOL  hasEsam;             //��ESAMоƬ��?
extern INT8U esamSerial[8];       //ESAMоƬ���к�

void   resetEsam(void);
INT16U putGetBytes(INT8U *cmdBuf, INT8U lenOfCmd, INT8U respondType, INT8U *recvBuf,INT8U lenOfRecv);
void   getChallenge(INT8U *buf);

#endif   //__INCEsamH