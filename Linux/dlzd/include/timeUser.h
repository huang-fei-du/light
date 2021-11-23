/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����timeUser.h
���ߣ�leiyong
�汾��0.9
�������:2006��6�� ��
����������,ʱ��ͷ�ļ�
�޸���ʷ��
  01,06-6-25,leiyong created.
**************************************************/

#ifndef __INCUserTimeH
#define __INCUserTimeH

#include "common.h"
#include <stdio.h>
#include <stdlib.h>

//�ṹ--����ʱ��
typedef struct
{
   INT8U second;   //��[0-59]
	 INT8U minute;   //��[0-59]
	 INT8U hour;     //ʱ[0-23]
	 INT8U day;      //��[1-31]
	 INT8U month;    //��[1-12]
	 INT8U year;     //��[��ĺ���λ,2000���Ժ����,since 2000]
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


