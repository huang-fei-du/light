/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：timeUser.h
作者：leiyong
版本：0.9
完成日期:2006年6月 日
描述：日期,时间头文件
修改历史：
  01,06-6-25,leiyong created.
**************************************************/

#ifndef __INCUserTimeH
#define __INCUserTimeH

#include "common.h"
#include <stdio.h>
#include <stdlib.h>

//结构--日期时间
typedef struct
{
   INT8U second;   //秒[0-59]
	 INT8U minute;   //分[0-59]
	 INT8U hour;     //时[0-23]
	 INT8U day;      //日[1-31]
	 INT8U month;    //月[1-12]
	 INT8U year;     //年[年的后两位,2000年以后的年,since 2000]
}DATE_TIME;

BOOL setSystemDateTime(DATE_TIME dateTime);
BOOL getSystemDateTime(DATE_TIME *dateTime);
void getLinuxFormatDateTime(DATE_TIME *dateTime,struct timeval *tv,INT8U type);

int       monthDays(int yeari,int month);
int       dayWeek(int year,int month,int day);
DATE_TIME nextTime(DATE_TIME nowTime,int minutes,int seconds);
DATE_TIME backTime(DATE_TIME nowTime, int month, int day, int hour,int minute,int second);
BOOL      timeCompare(DATE_TIME time1, DATE_TIME time2, INT16U interval);
BOOL      compareTwoTime(DATE_TIME time1, DATE_TIME time2);
INT32U    delayedSpike(DATE_TIME time1, DATE_TIME time2);
BOOL      compareTwoDate(DATE_TIME time1, INT8U time2Year,INT8U time2Month,INT8U time2Day,INT8U type);
void      delayTime(INT32U ticks);

#endif   //__INCUserTimeH


