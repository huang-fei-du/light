/*************************************************
Copyright,2011,Huawei WoDian co.,LTD
�ļ�����AFN06.h
���ߣ�leiyong
�汾��0.9
�������:2011��3��
��������վ"�����֤����ԿЭ��(AFN06)"ͷ�ļ�
�޸���ʷ��
  01,11-03-11,leiyong created.
**************************************************/
#ifndef __AFN06H
#define __AFN06H

#include "common.h"

void AFN06(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);

INT16U AFN06001(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06002(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06003(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06004(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06005(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06006(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06007(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06008(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06009(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT16U AFN06010(INT16U frameTail,INT8U *pHandle,INT8U dataFrom);
INT8U calcMac(INT8U afn, INT8U ifSingle, INT8U *buf, INT16U lenOfBuf, INT8U *retMac);

#endif  /*__AFN04H*/
