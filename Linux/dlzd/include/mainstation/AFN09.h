/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
�ļ�����AFN09.h
���ߣ�leiyong
�汾��0.9
�������:2010��3�� ��
��������վ"�����ն����ü���Ϣ(AFN09)"ͷ�ļ�
�޸���ʷ��
  01,10-03-23,leiyong created.
**************************************************/
#ifndef __AFN09H
#define __AFN09H

#include "common.h"

//��������

//��1
INT16U AFN09001(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09002(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09003(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09004(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09005(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09006(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09007(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09008(INT16U frameTail,INT8U *pHandle, INT8U fn);

#ifdef SDDL_CSM
 INT16U AFN09011(INT16U frameTail,INT8U *pHandle, INT8U fn);
#endif

#endif  /*__AFN09H*/
