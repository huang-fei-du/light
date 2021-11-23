/***************************************************
Copyright,2008,LongTong co.,LTD,All	Rights Reserved
�ļ�����localCtrl.h
���ߣ�TianYe
�汾��0.9
������ڣ�2008��12��
��������������ͷ�ļ�
�����б�
�޸���ʷ:
  01,08-01-22,Tianye created.
  02,08-12-10,leiyong modify,��д���п��ƹ���
  03,10-04-27,leiyong��ֲ��AT91SAM9260������
***************************************************/

#ifndef __INCCONTROLh
#define __INCCONTROLh

#include "workWithMS.h"
#include "workWithMeter.h"
#include "dataBase.h"

//բ״ָ̬ʾ
#define GATE_STATUS_ON          0x1    //բ״̬ΪOn
#define GATE_STATUS_OFF         0x2    //բ״̬ΪOff

#define CTRL_STATUS_JUMP_1      0x1    //��1·��բ
#define CTRL_STATUS_JUMP_2      0x2    //��2·��բ
#define CTRL_STATUS_JUMP_3      0x4    //��3·��բ
#define CTRL_STATUS_JUMP_4      0x8    //��4·��բ

#define CTRL_STATUS_CLOSE_1     0xE    //��1·��բ
#define CTRL_STATUS_CLOSE_2     0xD    //��2·��բ
#define CTRL_STATUS_CLOSE_3     0xB    //��3·��բ
#define CTRL_STATUS_CLOSE_4     0x7    //��4·��բ

#define GATE_CLOSE_WAIT_TIME     10    //��բ��ʾ��ʱ��
#define START_ALARM_WAIT_TIME    10    //��ʼ�澯��ʾ��ʱ��
#define WAIT_NEXT_ROUND           1    //��һ�ִεȴ�ʱ��(minute)

#define NEGTIVE_NUM      0x10
#define POSITIVE_NUM     0x00

extern INT8U     gateCloseWaitTime;    //�����բ�澯��ʱ��
extern DATE_TIME nextRound;            //��һ�ִεȴ�ʱ��(��������½�����ֵ����,�򲻼�����բ)

//��������
void   realCtrlTask(void);
void   threadOfCtrl(void *arg);
INT32U countAlarmLimit(INT8U *data, INT8U dataFormat, INT8U ctrlIndex,INT16U *watt);
BOOL   getPowerLimit(INT8U grpPn, INT8U limitPeriod, INT8U periodCount, INT8U* powerLimit);
INT8U  getPowerPeriod(DATE_TIME  limitTime);
void   jumpGate(INT8U line,INT8U onOff);
void   gateStatus(INT8U line,INT8U onOff);
BOOL   calcData(INT8U *buff,INT32U limit,INT32U *powerKw,INT32U *limitKw,INT32U *ctrlKw,INT16U *powerWatt,INT16U *limitWatt,INT16U *ctrlWatt,INT8U index, INT8U format,INT8U pn);
void   alarmInQueue(INT8U alarmType,INT8U pn);
void   ctrlRelease(INT8U pn, INT8U ctrlType,INT8U ifJumped);
void   electCtrlRecord(INT8U type, INT8U pn, INT8U round, INT8U ctrlType,INT8U *totalEnergy,INT8U *limit);
void   powerCtrlInQueue(INT8U pn,INT8U ctrlType,INT8U gate,INT8U *limit);
void   powerCtrlRecord(void);
BOOL   calcRealPower(INT8U *buff,INT8U pn);
void   ctrlJumpedStatis(INT8U ctrlType,INT8U num);

//BOOL     getPresentPowerLimit(DATE_TIME  limitTime, INT8U grpPn, INT8U limitPeriod, INT8U* powerLimit);

#endif //__INCCONTROLh