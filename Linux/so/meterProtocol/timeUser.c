/***************************************************
Copyright,2009,Huawei Wodian co.,LTD,All	Rights Reserved
�ļ�����timeUser.c
���ߣ�leiyong
�汾��0.9
������ڣ�2009��11��
������ʱ��(date,time)�ļ�
�����б�
     1.
�޸���ʷ��
  01,09-10-30,Leiyong created.
***************************************************/
#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>

#include "time.h"
#include "timeUser.h"

#define DEV_RTC "/dev/rtc0"

//�����������ṹû���ҵ��ֳɵ�ͷ�ļ�,�ҵ����ȥ���������ṹ 2009.10.30,ly
//struct timeval
//{
//	time_t		tv_sec;		  /* seconds */
//	long 	    tv_usec;	  /* microseconds */
//};

struct timezone
{
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

extern sendDebugFrame(INT8U *data, INT16U length);

/***************************************************
��������:monthDays
��������:����ĳ���ĳ���ж�����
���ú���:
�����ú���
�������:��
�������:
����ֵ:ĳ�µ�����
***************************************************/
int monthDays(int year,int month)
{
   int days;
   days=0;  
   if (month==2)
   {
      if (year%4==0)
      {
         if  (year%100==0)
         {
            if (year%1000==0)
            {
               days=29;
            }
            else
            {  
               days=28;
            }
         }
         else
         {
            days=29;
         }
      }
      else
      {
         days=28;
      }
   }
   else
   {  
      if(month==4 ||  month==6 || month==9 || month==11)
      {
         days=30;
      }
      else  
      {
         days=31;
      }
   }
   return days;
}

/***************************************************
��������:dayWeek
��������:����ĳ��ĳ�µ�ĳ�������ڼ�
���ú���:
�����ú���
�������:��
�������:
����ֵ:ĳ�������ڼ�
***************************************************/
int dayWeek(int year,int month,int day)
{
   int days;
   int i;  
   
   days=0;
   
   //��(�����2000��1��1�յ�ȥ��׵�����)
   for(i=2000;i<year;i++)
   {
     if((i%4==0 && i%100!=0)||i%400==0)
     {
        days += 366;
     }
     else
     {
        days += 365;
     }
   }
   
   //��  
   for(i=1;i<month;i++)
   {
      days += monthDays(year,i);
   }
   
   //��
   days += day;
   
   return (days+5)%7;   //��Ϊ2000��1��1����������,����Ҫ��5
}

/***************************************************
��������:nextTime
��������:������һ��ʱ��
���ú���:
�����ú���
�������:��
�������:
����ֵ:��һ�ε�ʱ��
***************************************************/
DATE_TIME nextTime(DATE_TIME nowTime,int minutes,int seconds)
{
    int         tmpi;       
   	DATE_TIME   next;
   	
    next.second = (seconds+nowTime.second)%60;

    tmpi = (seconds+nowTime.second)/60;
    next.minute = (minutes+tmpi+nowTime.minute)%60;

    tmpi = (minutes+tmpi+nowTime.minute)/60;    //�����Сʱ��
    next.hour = (tmpi+nowTime.hour)%24;

    tmpi = (tmpi+nowTime.hour)/24;         //���������
    if ((nowTime.day + tmpi)<= monthDays(nowTime.year,nowTime.month))
    {
       next.day =  nowTime.day + tmpi;
       tmpi = 0;
    }
    else
    {
       next.day =  (nowTime.day + tmpi) % monthDays(nowTime.year,nowTime.month);
       tmpi = 1;
    }

    if ((nowTime.month +tmpi)<=12)
    {
       next.month = nowTime.month + tmpi;
       tmpi = 0;
    }
    else
    {
    	 next.month = (nowTime.month + tmpi)%12;
       tmpi = 1;
    }

    next.year = nowTime.year+tmpi;
    
    return next;
}

/***************************************************
��������:backTime
��������:ʱ�����һ��ʱ��
���ú���:
�����ú���
�������:��
�������:
����ֵ:�µ�ʱ��
***************************************************/
DATE_TIME backTime(DATE_TIME nowTime, int month, int day, int hour,int minute,int second)
{
   	DATE_TIME prev;
   	
   	if (nowTime.second < second)
   	{
   	  do
   	  {
   	    nowTime.second += 60;
   	    minute++;
   	  }while (nowTime.second < second);
   	}
   	prev.second = nowTime.second - second;
   	
   	if (nowTime.minute < minute)
   	{
   	  do
   	  {
   	  	nowTime.minute += 60;
   	    hour++;
   	  }while (nowTime.minute < minute);
   	}
   	prev.minute = nowTime.minute - minute;
   	
   	if (nowTime.hour < hour)
   	{
   	  do
   	  {
   	    nowTime.hour += 24;
   	    day++;
   	  }while (nowTime.hour < hour);
	  }
   	prev.hour = nowTime.hour - hour;
   	
 	  if (nowTime.day < day)
   	{
   	  do
   	  {
   	    nowTime.month = (nowTime.month == 1) ? 12 : (nowTime.month-1);
   	    nowTime.year = (nowTime.month == 1) ? (nowTime.year-1) : nowTime.year;
   	  
   	    nowTime.day += monthDays(nowTime.year, nowTime.month);
   	  }while (nowTime.day < day);
   	}
   	
   	if (nowTime.month < month)
   	{
   	  do
   	  {
   	  	nowTime.month += 12;
   	    nowTime.year--;
   	  }while (nowTime.month < month);
   	}

    prev.day = nowTime.day - day;
    prev.month = (nowTime.month - month == 0) ? 12 : (nowTime.month - month);
    prev.year = (nowTime.month - month == 0) ? (nowTime.year-1) : nowTime.year;
    
    if (prev.day == 0)
    {
      if (prev.month == 1)
      {
      	prev.day = 31;
      	prev.month = 12;
        prev.year--;
      }
      else
      {
        prev.month--;
        prev.day = monthDays(prev.year, prev.month);
      }
    }

    return prev;
}

/*******************************************************
��������:timeCompare
��������:������ʱ��Ĳ���С�ڸ����������TRUE������
         ����FALSE,���time2��time1����time2��time1֮�����С��interval����true,���򷵻�false
���ú���:
�����ú���:
�������: 16����ʱ��ṹ
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL timeCompare(DATE_TIME time1, DATE_TIME time2, INT16U interval)
{
  DATE_TIME tmpTime;
  
  tmpTime = nextTime(time1, interval, 0);
  
  /* ��Ұ 2008-07-30 �޸�����Ӧ�ϴ�ļ��ֵ
  if (time2.year == time1.year && time2.month == time1.month
  	   && time2.day == time1.day && time2.hour == time1.hour)
  {
     if (time2.minute >= time1.minute && time2.minute - time1.minute < interval)
       return TRUE;
  }

  if (time2.year == tmpTime.year && time2.month == tmpTime.month
  	   && time2.day == tmpTime.day && time2.hour == tmpTime.hour)
  {
     if (time2.minute < tmpTime.minute && tmpTime.minute - time2.minute < interval)
        return TRUE;
  }
  */
  
  //���time1С��time2 ��time1 + ���ʱ���Ժ��ʱ��ֵ����time2��
  //��˵��time2����time1����time2��time1���ķ������������������
  if (compareTwoTime(time1, time2) && compareTwoTime(time2, tmpTime))
  {
  	return TRUE;
  }
  
  return FALSE;
}

/*******************************************************
��������:timeCompare
��������:�Ƚ�����ʱ����Ⱥ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:time2����time1������TRUE;���򷵻�FALSE
*******************************************************/
BOOL compareTwoTime(DATE_TIME time1, DATE_TIME time2)
{
    if (time1.year > time2.year)
    {
      return FALSE;
    }
    else
    {
      if (time1.year < time2.year)
      {
        return TRUE;
      }
      else     //�����ȱȽ���
      {
        if (time1.month > time2.month)        	
        {
          return FALSE;
        }
        else
        {
          if (time1.month < time2.month)
          {
            return TRUE;
          }
          else  //�·���ȱȽ���
          {
            if (time1.day > time2.day)
            {
              return FALSE;
            }
            else
            {
              if (time1.day < time2.day)
              {
                return TRUE;
              }
              else  //����ȱȽ�ʱ
              {
                if (time1.hour > time2.hour)
                {
                  return FALSE;
                }
                else
                {
                  if (time1.hour < time2.hour)
                  {
                    return TRUE;
                  }
                  else  //ʱ��ȱȽϷ�
                  {
                    if (time1.minute > time2.minute)
                    {
                      return FALSE;
                    }
                    else
                    {
                      if (time1.minute < time2.minute)
                      {
                        return TRUE;
                      }
                      else
                      {
                        if (time1.second > time2.second)
                        {
                          return FALSE;
                        }
                        else
                        {
                          return TRUE;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
}

/*******************************************************
��������:compareTwoDate
��������:�Ƚ��������ڵ��Ⱥ�
���ú���:
�����ú���:
�������:DATE_TIME��ʽ��ʱ��1,INT8U ��ʽtime2�������ա�
         type=1,�Ƚ�time1�Ƿ����time2
         type=2,�Ƚ�time2�Ƿ����time1
�������:
����ֵ:
  type=1,�Ƚ�time1���ڵ���time2����TRUE;���򷵻�FALSE
  type=2,�Ƚ�time2���ڵ���time1����TRUE;���򷵻�FALSE
*******************************************************/
BOOL compareTwoDate(DATE_TIME time1, INT8U time2Year,INT8U time2Month,INT8U time2Day,INT8U type)
{
   if (type==1)
   {
     if(time1.year > time2Year
       || (time1.year == time2Year && time1.month > time2Month)
       || (time1.year == time2Year && time1.month == time2Month && time1.day >= time2Day))
     {
   	    return TRUE;
     }
     else
     {
   	    return FALSE;
     }
   }
   else
   {
     if(time2Year>time1.year
       || (time2Year==time1.year && time2Month>time1.month)
       || (time2Year==time1.year && time2Month==time1.month &&  time2Day>=time1.day))
     {
   	    return TRUE;
     }
     else
     {
   	    return FALSE;
     }
   }
}


/*******************************************************
��������:delayedSpike
��������:��������ʱ���ʱ��������
���ú���:
�����ú���:
�������:
�������:
����ֵ:time1��time2ʱ��������
*******************************************************/
INT32U delayedSpike(DATE_TIME time1, DATE_TIME time2)
{
	 DATE_TIME tmpTime;
   struct timeval tv1,tv2;
	 
	 //���time2<time1,��������ʱ��,��֤time2>time1
	 if (compareTwoTime(time2, time1)==TRUE)
	 {
	 	  tmpTime = time1;
	 	  time1 = time2;
	 	  time2 = tmpTime;
	 }
	 
   getLinuxFormatDateTime(&time1,&tv1,1);
   getLinuxFormatDateTime(&time2,&tv2,1);
   
   return (tv2.tv_sec-tv1.tv_sec);
	 
	 /*
	 //��
	 if (time2.second<time1.second)
	 {
	 	  time2.second += 60;
	 	  if (time2.minute>0)
	 	  {
	 	  	 time2.minute--;
	 	  }
	 	  else
	 	  {
	 	  	 time2.minute = 59;
	 	  	 if (time2.hour>0)
	 	  	 {
	 	  	 	  time2.hour--;
	 	  	 }
	 	  	 else
	 	  	 {
	 	  	 	  time2.hour=23;
	 	  	 	  if (time2.day>1)
	 	  	 	  {
	 	  	 	  	 time2.day--;
	 	  	 	  }
	 	  	 	  else
	 	  	 	  {
	 	  	 	  	 if (time2.month>1)
	 	  	 	  	 {
	 	  	 	  	 	  time2.month--;
	 	  	 	  	 }
	 	  	 	  	 else
	 	  	 	  	 {
	 	  	 	  	 	  time2.month = 12;
	 	  	 	  	 	  if (time2.year>1)
	 	  	 	  	 	  {
	 	  	 	  	 	  	 time2.year = 99;
	 	  	 	  	 	  }
	 	  	 	  	 }
	 	  	 	  	 
	 	  	 	  	 time2.day = monthDays(time2.year,time2.month);
	 	  	 	  }	 	  	 	  
	 	  	 }
	 	  }
	 }	 
	 delayed += time2.second-time1.second;
	 	  
	 //��
	 if (time2.minute<time1.minute)
	 {
  	 time2.minute += 60;
  	 
  	 if (time2.hour>0)
  	 {
  	 	  time2.hour--;
  	 }
  	 else
  	 {
  	 	  time2.hour=23;
  	 	  if (time2.day>1)
  	 	  {
  	 	  	 time2.day--;
  	 	  }
  	 	  else
  	 	  {
  	 	  	 if (time2.month>1)
  	 	  	 {
  	 	  	 	  time2.month--;
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	 	  time2.month = 12;
  	 	  	 	  if (time2.year>1)
  	 	  	 	  {
  	 	  	 	  	 time2.year = 99;
  	 	  	 	  }
  	 	  	 }
  	 	  	 
  	 	  	 time2.day = monthDays(time2.year,time2.month);
  	 	  }	 	  	 	  
  	 }	 	  
	 }
   delayed += (time2.minute-time1.minute)*60;
   
   //ʱ
	 if (time2.hour<time1.hour)
	 {
	 	  time2.hour += 24;
	 	  if (time2.day>1)
	 	  {
	 	  	 time2.day--;
	 	  }
	 	  else
	 	  {
	 	  	 if (time2.month>1)
	 	  	 {
	 	  	 	  time2.month--;
	 	  	 }
	 	  	 else
	 	  	 {
	 	  	 	  time2.month = 12;
	 	  	 	  if (time2.year>1)
	 	  	 	  {
	 	  	 	  	 time2.year = 99;
	 	  	 	  }
	 	  	 }
	 	  	 
	 	  	 time2.day = monthDays(time2.year,time2.month);
	 	  }	  
	 }
   delayed += (time2.hour-time1.hour)*3600;
   
   //��
	 if (time2.day<time1.day)
	 {
  	 if (time2.month>1)
  	 {
  	 	  time2.month--;
  	 }
  	 else
  	 {
  	 	  time2.month = 12;
  	 	  if (time2.year>1)
  	 	  {
  	 	  	 time2.year = 99;
  	 	  }
  	 }
  	 
  	 time2.day += monthDays(time2.year,time2.month);
	 }
   delayed += (time2.hour-time1.hour)*86400;
   */
   /*
   //��
	 if (time2.month<time1.month)
	 {
  	 	time2.month += 12;
   }
   delayed += (time2.hour-time1.hour)*86400;   
   */
   
   //Ŀǰ�����꼰�²������
   
   //return delayed;
}

/*******************************************************
��������:setSystemDateTime
��������:����ϵͳʱ��
���ú���:
�����ú���:
�������:Ӧ�ó����û������ʱ���ʽ
�������:
����ֵ:TURE or FALSE
*******************************************************/
BOOL setSystemDateTime(DATE_TIME dateTime)
{
  struct tm       timeLinux;    //Linux�û�ʱ���ʽ
  struct timeval  tv;           //Linux timeval
  struct timezone tz;           //Linux timezone
  struct rtc_time wtime;
  int             fd;
  
  //дLinuxʱ��
  timeLinux.tm_year  = (dateTime.year+2000)-1900;
  timeLinux.tm_mon   = dateTime.month-1;
  timeLinux.tm_mday  = dateTime.day;
  timeLinux.tm_hour  = dateTime.hour;
  timeLinux.tm_min   = dateTime.minute;
  timeLinux.tm_sec   = dateTime.second;
  timeLinux.tm_isdst = 0;
  
  tv.tv_sec = mktime(&timeLinux);
  tv.tv_usec = 0;

  if (settimeofday(&tv,&tz)==0)
  {
  	 //return TRUE;
  }

	//����RTCоƬʱ��
	wtime.tm_year = dateTime.year+2000;
	wtime.tm_mon  = dateTime.month-1;
	wtime.tm_mday = dateTime.day;
	wtime.tm_wday = dayWeek(dateTime.year+2000,dateTime.month,dateTime.second);
	wtime.tm_hour = dateTime.hour;
	wtime.tm_min  = dateTime.minute;
	wtime.tm_sec  = dateTime.second;
  
  fd = open(DEV_RTC, O_RDWR);
  if (fd <0)
  {
	  printf("Open %s error\n",DEV_RTC);
	  return FALSE;
  }
  
	if (ioctl(fd, RTC_SET_TIME, &wtime)==0)
	{
		 printf("����ʱ��ɹ�\n");
	}
	else
	{
		 printf("����ʱ��ʧ��\n");
	}
	close(fd);
  
  return FALSE;
}

/*******************************************************
��������:getSystemDateTime
��������:ȡ��ϵͳʱ��
���ú���:
�����ú���:
�������:Ӧ�ó����û������ʱ���ʽָ��
�������:
����ֵ:TURE or FALSE
*******************************************************/
BOOL getSystemDateTime(DATE_TIME *dateTime)
{
   struct tm *pLinuxTime;
   time_t timep;

   time(&timep);
   pLinuxTime=localtime(&timep); /*ȡ�õ���ʱ��*/
   
   dateTime->year   = 1900+pLinuxTime->tm_year-2000;
   dateTime->month  = pLinuxTime->tm_mon+1;
   dateTime->day    = pLinuxTime->tm_mday;
   dateTime->hour   = pLinuxTime->tm_hour;
   dateTime->minute = pLinuxTime->tm_min;
   dateTime->second = pLinuxTime->tm_sec;
   
   return TRUE;
}

/*******************************************************
��������:getLinuxFormatDateTime
��������:ȡ��Linux��ʽ��ʱ��
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void getLinuxFormatDateTime(DATE_TIME *dateTime,struct timeval *tv,INT8U type)
{
  struct tm       timeLinuxx,*timex;    //Linux�û�ʱ���ʽ
  time_t          timeT;
  
  //�ɸ�����dataTime�����1970�������,����tv��
  if (type==1)
  {
    timeLinuxx.tm_year  = (dateTime->year+2000)-1900;
    timeLinuxx.tm_mon   = dateTime->month-1;
    timeLinuxx.tm_mday  = dateTime->day;
    timeLinuxx.tm_hour  = dateTime->hour;
    timeLinuxx.tm_min   = dateTime->minute;
    timeLinuxx.tm_sec   = dateTime->second;
    timeLinuxx.tm_isdst = 0;
  
    tv->tv_sec = mktime(&timeLinuxx);
  }
  else   //
  {
     timeT = tv->tv_sec;
     timex = gmtime(&timeT);
     
     dateTime->year  = timex->tm_year-100;
     dateTime->month = timex->tm_mon + 1;
     dateTime->day   = timex->tm_mday;
     dateTime->hour  = timex->tm_hour;
     dateTime->minute= timex->tm_min;
     dateTime->second= timex->tm_sec;     
  }
}
