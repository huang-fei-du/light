/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
�ļ���:carrier.h
���ߣ�leiyong
�汾��0.9
�������:2010��3��
����:�ز�ģ�鴦��ͷ�ļ�
�޸���ʷ��
  01,10-03-09,leiyong created.
**************************************************/
#ifndef __CarrierModuleH
#define __CarrierModuleH

#include "common.h"



//�ⲿ����
extern INT8U            carrierModuleType; //�ز�ģ������
extern INT8U            carrierAfn;        //�ز�����AFN
extern INT8U            carrierFn;         //�ز�����FN
extern CARRIER_FLAG_SET carrierFlagSet;    //�ز���־��

//��������
void carrier(void);
void sendCarrierFrame(INT8U *pack,INT16U length);
void *threadOfCarrierReceive(void *arg);
void resetCarrierFlag(void);

#endif  //__CarrierModuleH