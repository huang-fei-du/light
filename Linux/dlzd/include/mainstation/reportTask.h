/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：AFN04.h
作者：leiyong
版本：0.9
完成日期:2006年5月 日
描述：主站“设置参数(AFN04)”头文件
修改历史：
  01,06-05-28,leiyong created.
**************************************************/
#ifndef __INCreporttaskh
#define __INCreporttaskh

#include "timeUser.h"
#include "workWithMS.h"

//外部变量
extern   DATE_TIME          sysTime;                 //系统时间
extern   REPORT_TASK_PARA   reportTask1, reportTask2;//定时发送1类，2类数据任务配置(AFN04-FN65,FN66)
extern   REPORT_TIME        reportTime1;             //定时发送一类数据时间表
extern   REPORT_TIME_2      reportTime2;             //定时发送二类数据时间表
extern   BOOL               ipPermit;                //允许传输IP包
extern   ANALYSE_FRAME      frame;
extern   BOOL               initTasking;      //正在初始化任务标志

//外部函数
extern   void   AFN0C(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom,INT8U poll);
extern   void   AFN0D(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom,INT8U poll);

//函数声明
void initReportTask(void *arg);
void activeReport1(void);
void threadOfReport2(void *arg);
void activeReport3(void);

#endif //__INCreporttaskh
