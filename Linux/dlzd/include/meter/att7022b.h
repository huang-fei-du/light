/*************************************************
Copyright,2007,Huawei WoDian co.,LTD
�ļ�����att7022b.h
���ߣ�leiyong
�汾��0.9
�������:2010��2�� ��
�����������ն˽�������ͷ�ļ�
�޸���ʷ��
  01,10-02-27,leiyong created.
**************************************************/

#ifndef __att7022bH
#define __att7022bH

#include "common.h"
#include "workWithMeter.h"

#define   TOTAL_COMMAND_REAL_AC   50    //���ɲɼ���������,0x3e��0x5fΪATT7022BУ������У���1��2
//#define   TOTAL_COMMAND_REAL_AC   34    //���ɲɼ���������,0x3e��0x5fΪATT7022BУ������У���1��2

#define   READ_SIG_OUT_VALUE      0x01  //��ȡоƬ�����ź�
#define   READ_HAS_AC_SAMPLE      0x02  //��ȡ�Ƿ��н���ģ��
#define   RESET_ATT7022B          0x03  //��λ����оƬ
#define   SET_AC_MODE             0x04  //���ý���ģ��ģʽ

#define   MEASURE_V_ANGLE               //ʹ�ܵ�ѹ�нǲ�������

#define   SIZE_OF_ENERGY_VISION    260  //����ʾֵ�洢��С
#define   POS_EPT                    0  //�����й��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   NEG_EPT                   25  //�����й��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   POS_EQT                   50  //�����޹��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   NEG_EQT                   75  //�����޹��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   QUA_1_EQT                100  //һ�����޹��޹��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   QUA_4_EQT                125  //�������޹��޹��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   QUA_2_EQT                150  //�������޹��޹��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5
#define   QUA_3_EQT                175  //�������޹��޹��ܼ�4����������(����3Bytes+С���������2Bytes) 5*5

#define   POS_EPA                  200  //A�������й�(����3Bytes+С���������2Bytes)
#define   NEG_EPA                  205  //A�෴���й�(����3Bytes+С���������2Bytes)
#define   POS_EQA                  210  //A�������޹�(����3Bytes+С���������2Bytes)
#define   NEG_EQA                  215  //A�෴���޹�(����3Bytes+С���������2Bytes)
#define   POS_EPB                  220  //B�������й�(����3Bytes+С���������2Bytes)
#define   NEG_EPB                  225  //B�෴���й�(����3Bytes+С���������2Bytes)
#define   POS_EQB                  230  //B�������޹�(����3Bytes+С���������2Bytes)
#define   NEG_EQB                  235  //B�෴���޹�(����3Bytes+С���������2Bytes)
#define   POS_EPC                  240  //C�������й�(����3Bytes+С���������2Bytes)
#define   NEG_EPC                  245  //C�෴���й�(����3Bytes+С���������2Bytes)
#define   POS_EQC                  250  //C�������޹�(����3Bytes+С���������2Bytes)
#define   NEG_EQC                  255  //C�෴���޹�(����3Bytes+С���������2Bytes)

#define   AC_SIZE_OF_REQ_TIME      260  //����ʾֵ�洢��С


//�ⲿ����
extern INT8U  readCheckData;                      //��ȡУ�����
extern INT32U realAcData[TOTAL_COMMAND_REAL_AC];  //��������ʵʱ����
extern BOOL   ifHasAcModule;                      //�Ƿ��н���ģ��
extern INT8U  acReqTimeBuf[LENGTH_OF_REQ_RECORD];

//��������
void readAcChipData(void);
void sspCheckMeter(void);
void resetAtt7022b(BOOL ifCheckMeter);

#endif   //__att7022bH