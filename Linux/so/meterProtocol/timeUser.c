/***************************************************
Copyright,2009,Huawei Wodian co.,LTD,All	Rights Reserved
文件名：timeUser.c
作者：leiyong
版本：0.9
完成日期：2009年11月
描述：时间(date,time)文件
函数列表：
     1.
修改历史：
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

//以下这两个结构没有找到现成的头文件,找到后可去掉这两个结构 2009.10.30,ly
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
函数名称:monthDays
功能描述:计算某年的某月有多少天
调用函数:
被调用函数
输入参数:无
输出参数:
返回值:某月的天数
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
函数名称:dayWeek
功能描述:计算某年某月的某月是星期几
调用函数:
被调用函数
输入参数:无
输出参数:
返回值:某日是星期几
***************************************************/
int dayWeek(int year,int month,int day)
{
   int days;
   int i;  
   
   days=0;
   
   //年(计算从2000年1月1日到去年底的天数)
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
   
   //月  
   for(i=1;i<month;i++)
   {
      days += monthDays(year,i);
   }
   
   //日
   days += day;
   
   return (days+5)%7;   //因为2000年1月1日是星期六,所以要加5
}

/***************************************************
函数名称:nextTime
功能描述:计算下一次时间
调用函数:
被调用函数
输入参数:无
输出参数:
返回值:下一次的时间
***************************************************/
DATE_TIME nextTime(DATE_TIME nowTime,int minutes,int seconds)
{
    int         tmpi;       
   	DATE_TIME   next;
   	
    next.second = (seconds+nowTime.second)%60;

    tmpi = (seconds+nowTime.second)/60;
    next.minute = (minutes+tmpi+nowTime.minute)%60;

    tmpi = (minutes+tmpi+nowTime.minute)/60;    //多余的小时数
    next.hour = (tmpi+nowTime.hour)%24;

    tmpi = (tmpi+nowTime.hour)/24;         //多余的日数
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
函数名称:backTime
功能描述:时间后退一定时间
调用函数:
被调用函数
输入参数:无
输出参数:
返回值:新的时间
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
函数名称:timeCompare
功能描述:求两个时标的差，如果小于给定间隔返回TRUE，否则
         返回FALSE,如果time2比time1大，且time2与time1之间相差小于interval返回true,否则返回false
调用函数:
被调用函数:
输入参数: 16进制时间结构
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL timeCompare(DATE_TIME time1, DATE_TIME time2, INT16U interval)
{
  DATE_TIME tmpTime;
  
  tmpTime = nextTime(time1, interval, 0);
  
  /* 田野 2008-07-30 修改以适应较大的间隔值
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
  
  //如果time1小于time2 且time1 + 间隔时间以后的时间值大于time2，
  //则说明time2大于time1，且time2与time1相差的分钟数不超过所给间隔
  if (compareTwoTime(time1, time2) && compareTwoTime(time2, tmpTime))
  {
  	return TRUE;
  }
  
  return FALSE;
}

/*******************************************************
函数名称:timeCompare
功能描述:比较两个时间的先后
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:time2大于time1，返回TRUE;否则返回FALSE
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
      else     //年份相等比较月
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
          else  //月份相等比较日
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
              else  //日相等比较时
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
                  else  //时相等比较分
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
函数名称:compareTwoDate
功能描述:比较两个日期的先后
调用函数:
被调用函数:
输入参数:DATE_TIME格式的时间1,INT8U 格式time2的年月日。
         type=1,比较time1是否大于time2
         type=2,比较time2是否大于time1
输出参数:
返回值:
  type=1,比较time1大于等于time2返回TRUE;否则返回FALSE
  type=2,比较time2大于等于time1返回TRUE;否则返回FALSE
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
函数名称:delayedSpike
功能描述:计算两个时间的时间差的秒数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:time1与time2时间差的秒数
*******************************************************/
INT32U delayedSpike(DATE_TIME time1, DATE_TIME time2)
{
	 DATE_TIME tmpTime;
   struct timeval tv1,tv2;
	 
	 //如果time2<time1,交换两个时间,保证time2>time1
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
	 //秒
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
	 	  
	 //分
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
   
   //时
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
   
   //日
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
   //月
	 if (time2.month<time1.month)
	 {
  	 	time2.month += 12;
   }
   delayed += (time2.hour-time1.hour)*86400;   
   */
   
   //目前忽略年及月差的秒数
   
   //return delayed;
}

/*******************************************************
函数名称:setSystemDateTime
功能描述:设置系统时间
调用函数:
被调用函数:
输入参数:应用程序用户定义的时间格式
输出参数:
返回值:TURE or FALSE
*******************************************************/
BOOL setSystemDateTime(DATE_TIME dateTime)
{
  struct tm       timeLinux;    //Linux用户时间格式
  struct timeval  tv;           //Linux timeval
  struct timezone tz;           //Linux timezone
  struct rtc_time wtime;
  int             fd;
  
  //写Linux时间
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

	//设置RTC芯片时钟
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
		 printf("设置时间成功\n");
	}
	else
	{
		 printf("设置时间失败\n");
	}
	close(fd);
  
  return FALSE;
}

/*******************************************************
函数名称:getSystemDateTime
功能描述:取得系统时间
调用函数:
被调用函数:
输入参数:应用程序用户定义的时间格式指针
输出参数:
返回值:TURE or FALSE
*******************************************************/
BOOL getSystemDateTime(DATE_TIME *dateTime)
{
   struct tm *pLinuxTime;
   time_t timep;

   time(&timep);
   pLinuxTime=localtime(&timep); /*取得当地时间*/
   
   dateTime->year   = 1900+pLinuxTime->tm_year-2000;
   dateTime->month  = pLinuxTime->tm_mon+1;
   dateTime->day    = pLinuxTime->tm_mday;
   dateTime->hour   = pLinuxTime->tm_hour;
   dateTime->minute = pLinuxTime->tm_min;
   dateTime->second = pLinuxTime->tm_sec;
   
   return TRUE;
}

/*******************************************************
函数名称:getLinuxFormatDateTime
功能描述:取得Linux格式的时间
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void getLinuxFormatDateTime(DATE_TIME *dateTime,struct timeval *tv,INT8U type)
{
  struct tm       timeLinuxx,*timex;    //Linux用户时间格式
  time_t          timeT;
  
  //由给定的dataTime算出距1970年的秒数,存于tv中
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
