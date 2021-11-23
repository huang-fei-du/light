/***************************************************
Copyright,2008,LongTong co.,LTD,All	Rights Reserved
文件名：localCtrl.h
作者：TianYe
版本：0.9
完成日期：2008年12月
描述：负控任务头文件
函数列表：
修改历史:
  01,08-01-22,Tianye created.
  02,08-12-10,leiyong modify,重写所有控制功能
  03,10-04-27,leiyong移植到AT91SAM9260处理器
***************************************************/

#ifndef __INCCONTROLh
#define __INCCONTROLh

#include "workWithMS.h"
#include "workWithMeter.h"
#include "dataBase.h"

//闸状态指示
#define GATE_STATUS_ON          0x1    //闸状态为On
#define GATE_STATUS_OFF         0x2    //闸状态为Off

#define CTRL_STATUS_JUMP_1      0x1    //第1路跳闸
#define CTRL_STATUS_JUMP_2      0x2    //第2路跳闸
#define CTRL_STATUS_JUMP_3      0x4    //第3路跳闸
#define CTRL_STATUS_JUMP_4      0x8    //第4路跳闸

#define CTRL_STATUS_CLOSE_1     0xE    //第1路合闸
#define CTRL_STATUS_CLOSE_2     0xD    //第2路合闸
#define CTRL_STATUS_CLOSE_3     0xB    //第3路合闸
#define CTRL_STATUS_CLOSE_4     0x7    //第4路合闸

#define GATE_CLOSE_WAIT_TIME     10    //合闸提示音时长
#define START_ALARM_WAIT_TIME    10    //开始告警提示音时长
#define WAIT_NEXT_ROUND           1    //下一轮次等待时长(minute)

#define NEGTIVE_NUM      0x10
#define POSITIVE_NUM     0x00

extern INT8U     gateCloseWaitTime;    //允许合闸告警音时间
extern DATE_TIME nextRound;            //下一轮次等待时间(如果功率下降到定值以下,则不继续跳闸)

//函数声明
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