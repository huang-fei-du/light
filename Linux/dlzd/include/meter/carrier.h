/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
文件名:carrier.h
作者：leiyong
版本：0.9
完成日期:2010年3月
描述:载波模块处理头文件
修改历史：
  01,10-03-09,leiyong created.
**************************************************/
#ifndef __CarrierModuleH
#define __CarrierModuleH

#include "common.h"



//外部变量
extern INT8U            carrierModuleType; //载波模块类型
extern INT8U            carrierAfn;        //载波接收AFN
extern INT8U            carrierFn;         //载波接收FN
extern CARRIER_FLAG_SET carrierFlagSet;    //载波标志集

//函数声明
void carrier(void);
void sendCarrierFrame(INT8U *pack,INT16U length);
void *threadOfCarrierReceive(void *arg);
void resetCarrierFlag(void);

#endif  //__CarrierModuleH