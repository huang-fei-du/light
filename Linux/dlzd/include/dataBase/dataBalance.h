/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：dataBalance.h
作者：TianYe
版本：0.9
完成日期:2006年7月
描述：终端数据结算头文件。
修改历史：
  01,06-07-21,TianYe created.
**************************************************/

#ifndef __INCdataBalanceh
#define __INCdataBalanceh

#include "timeUser.h"
#include "common.h"

//数据格式 从1到23
#define FORMAT(i)        i       //i从1到23

//数据符号
#define NEGTIVE_NUM      0x10
#define POSITIVE_NUM     0x00

//事件记录标志
#define METER_STOP_NOT_RECORDED 0x01  //电表停走事件已记录
#define METER_STOP_RECORDED     0x02  //电表停走事件未记录

#define START_NEW_INSTANCE      0x01  //下一次开始新的统计
#define START_NEW_INSTANCE_NOT  0x00  //下一次继续当前统计过程

//功率统计记录
typedef struct
{
   INT8U     powerInt;
   INT16U    powerDec;
   DATE_TIME time;
}RECORD_POWER_STATIS;

//结构 - 电能表事件记录
typedef struct
{
   DATE_TIME time;
   INT32U    energy;
   INT8U     recordFlag;
}METER_EVENT_RECORD;

//函数声明
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