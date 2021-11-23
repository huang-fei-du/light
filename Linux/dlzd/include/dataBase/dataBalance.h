/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����dataBalance.h
���ߣ�TianYe
�汾��0.9
�������:2006��7��
�������ն����ݽ���ͷ�ļ���
�޸���ʷ��
  01,06-07-21,TianYe created.
**************************************************/

#ifndef __INCdataBalanceh
#define __INCdataBalanceh

#include "timeUser.h"
#include "common.h"

//���ݸ�ʽ ��1��23
#define FORMAT(i)        i       //i��1��23

//���ݷ���
#define NEGTIVE_NUM      0x10
#define POSITIVE_NUM     0x00

//�¼���¼��־
#define METER_STOP_NOT_RECORDED 0x01  //���ͣ���¼��Ѽ�¼
#define METER_STOP_RECORDED     0x02  //���ͣ���¼�δ��¼

#define START_NEW_INSTANCE      0x01  //��һ�ο�ʼ�µ�ͳ��
#define START_NEW_INSTANCE_NOT  0x00  //��һ�μ�����ǰͳ�ƹ���

//����ͳ�Ƽ�¼
typedef struct
{
   INT8U     powerInt;
   INT16U    powerDec;
   DATE_TIME time;
}RECORD_POWER_STATIS;

//�ṹ - ���ܱ��¼���¼
typedef struct
{
   DATE_TIME time;
   INT32U    energy;
   INT8U     recordFlag;
}METER_EVENT_RECORD;

//��������
void    dataProcRealPoint(void *arg);

void    copyDayFreeze(INT8U port);

INT8U   dataFormat(INT32U *integer, INT32U *decimal, INT8U format);
INT32U  bcdToHex(INT32U bcd);
INT32U  hexToBcd(INT32U hex);
void    computeLeftPower(DATE_TIME statisTime);
void    unbalanceStatis(INT8U i, INT8U pn);

void    statisticMaxDemand(INT8U *copyReqBuff,INT8U *balanceParaBuff);
void    statisticPower(INT8U *copyParaBuff, INT8U *balanceParaBuff, INT8U statisInterval, DATE_TIME statisTime);
void    statisticOpenPhase(INT8U *pCopyParaBuff,INT8U *pBalanceParaBuff);
void    meterRunWordChangeBit(INT8U *changeWord,INT8U *thisRunWord,INT8U *lastRunWord);
BOOL    computeInTimeLeftPower(INT8U zjzNo, DATE_TIME statisTime, INT8U *leftPower, INT8U ifSave);
void    processOverLimit(INT8U port);

#ifdef LIGHTING
 void processKzqOverLimit(INT8U *acParaData, INT16U pn);
#endif

#endif //__INCdataBalanceh