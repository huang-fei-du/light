/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：repotTask.c
作者：TianYe
版本：0.9
完成日期：2006年9月
描述：定时发送数据文件。
函数列表：
修改历史：
  01,06-09-05,TianYe created.
***************************************************/

#include "stdio.h"
#include "stdlib.h"

#include "workWithMeter.h"
#include "teRunPara.h"
#include "reportTask.h"

extern void sendDebugFrame(INT8U *pack,INT16U length);

void threadOfReport2(void *arg);

BOOL initTasking;      //正在初始化任务标志

/*******************************************************
函数名称:initReportTask
功能描述:初始化定时发送数据任务，完成下一次发送时间的确定
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void initReportTask(void *arg)
{
    INT8U   i, j = 0;;
    INT8U   interval, rate;
    
    initTasking = TRUE;

    printf("初始化主动上报任务队列线程开始\n");
    
    //初始化定时发送一类数据时间表
    for(i=0;i<64;i++)
    {
    	reportTime1.taskConfig[j].taskNum = 0x0;
    }
    reportTime1.numOfTask = reportTask1.numOfTask;
    
    j = 0;
    for (i = 0; i < 64; i++)
    {
    	if(reportTask1.task[i].taskNum != 0)
    	{
    		//按照任务配置逐项填写发送时间表
	      reportTime1.taskConfig[j].taskNum = reportTask1.task[i].taskNum;
	      reportTime1.taskConfig[j].nextReportTime = reportTask1.task[i].sendStart;
	      reportTime1.taskConfig[j].nextReportTime.month &= 0x1F;
	      
	      interval = reportTask1.task[i].sendPeriod & 0x3f;
	      
	      rate = reportTask1.task[i].sendPeriod>>6 & 0x03;
	      
	      //如果系统时间大于发送初始时间，就在发送初始时间的基础上加上发送间隔，直到大于系统时间
	      while(1)
	      {
	         if (interval==0)
	         {
	         	  break;
	         }
	         
	         if (compareTwoTime(sysTime, reportTime1.taskConfig[j].nextReportTime) == FALSE)
	         {
	           switch (rate)
	           {
	             case 0://minute
	               reportTime1.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime1.taskConfig[j].nextReportTime, interval, 0);
	            	 break;
	            	
	             case 1://hour
	            	 reportTime1.taskConfig[j].nextReportTime
	            	     = nextTime(reportTime1.taskConfig[j].nextReportTime, interval*60, 0);
	               break;
	            
	             case 2://day
	            	 reportTime1.taskConfig[j].nextReportTime
	            	     = nextTime(reportTime1.taskConfig[j].nextReportTime, interval*60*24, 0);
	            	 break;
	            	
	             case 3://month
	            	 reportTime1.taskConfig[j].nextReportTime.month += interval;
	                
	               if (reportTime1.taskConfig[j].nextReportTime.month > 12)
	               {
	                 reportTime1.taskConfig[j].nextReportTime.month -= 12;
	                 reportTime1.taskConfig[j].nextReportTime.year++;
	               }
	               break;
	           }
	         }
	         else
	         {
	         	 break;
	         }
	      }
	      j++;
    	}
    }
    
    printf("初始化二类数据任务时间表\n");

    //初始化定时发送二类数据时间表
    reportTime2.numOfTask = reportTask2.numOfTask;
    for(i=0; i<64; i++)
    {
    	 reportTime2.taskConfig[j].taskNum = 0x0;
    }
    
    j = 0;
    for (i = 0; i < 64; i++)
    {
    	if(reportTask2.task[i].taskNum != 0)
    	{
    		//按照任务配置逐项填写发送时间表
	      reportTime2.taskConfig[j].taskNum = reportTask2.task[i].taskNum;
	      reportTime2.taskConfig[j].nextReportTime = reportTask2.task[i].sendStart;
	      
	      interval = reportTask2.task[i].sendPeriod & 0x3f;
	      rate = reportTask2.task[i].sendPeriod>>6 & 0x03;
	      
	      if (interval==0)
	      {
	      	 continue;
	      }
	      
	      //如果系统时间大于发送初始时间，就在发送初始时间的基础上加上发送间隔，直到大于系统时间
	      if (compareTwoTime(sysTime, reportTime2.taskConfig[j].nextReportTime) == FALSE)
	      {
	        do
	        {
	        	//将下一次发送时间保存为上一次发送时间
	          reportTime2.taskConfig[j].lastReportTime = reportTime2.taskConfig[j].nextReportTime;
	          
	          //将下一次发送时间增加一个发送间隔
	          switch (rate)
	          {
	            case 0:   //分
	            	reportTime2.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].nextReportTime, interval, 0);
	            	break;
	            	
	            case 1:   //时
	            	reportTime2.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].nextReportTime, interval*60, 0);
	            	break;
	            
	            case 2:   //日
	            	reportTime2.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].nextReportTime, interval*60*24, 0);
	            	break;
	            	
	            case 3:   //月
	            	reportTime2.taskConfig[j].nextReportTime.month += interval;
	                
	              while(reportTime2.taskConfig[j].nextReportTime.month > 12)
	              {
	                reportTime2.taskConfig[j].nextReportTime.month -= 12;
	                reportTime2.taskConfig[j].nextReportTime.year++;
	              }
	              break;
	          }
	          
	        }while (compareTwoTime(sysTime, reportTime2.taskConfig[j].nextReportTime) == FALSE);
	      }
	      else   //如果下一次发送时间大于系统时间，则上一次发送时间等于下一次发送时间减间隔
	      {
	      	 reportTime2.taskConfig[j].lastReportTime = reportTime2.taskConfig[j].nextReportTime;
	      	
	         switch (rate)
	         {
	            case 0:    //分
	            	reportTime2.taskConfig[j].lastReportTime 
	            	     = backTime(reportTime2.taskConfig[j].lastReportTime,0,0,0, interval, 0);
	            	break;
	            	
	            case 1:    //时
	            	reportTime2.taskConfig[j].lastReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].lastReportTime, interval*60, 0);
	            	break;
	            
	            case 2:    //日
	            	reportTime2.taskConfig[j].lastReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].lastReportTime, interval*60*24, 0);
	            	break;
	            	
	            case 3:    //月
	            	if (reportTime2.taskConfig[j].lastReportTime.month < interval)
	            	{
	            	  do
	            	  {
	            	    reportTime2.taskConfig[j].lastReportTime.month
	            	          = reportTime2.taskConfig[j].lastReportTime.month + 12;
	            	    reportTime2.taskConfig[j].lastReportTime.year--;
	            	  }while (reportTime2.taskConfig[j].lastReportTime.month < interval);
	            	}
	            	
	            	reportTime2.taskConfig[j].lastReportTime.month -= interval;
	            	
	              break;
	         }//switch
	      }//else
        
        if (debugInfo&PRINT_ACTIVE_REPORT)
        {
          printf("任务序号=%d,j=%d,初始化lastReportTime:%02d-%02d-%02d %02d:%02d:%02d\n", reportTime2.taskConfig[j].taskNum, j, reportTime2.taskConfig[j].lastReportTime.year,reportTime2.taskConfig[j].lastReportTime.month,reportTime2.taskConfig[j].lastReportTime.day,reportTime2.taskConfig[j].lastReportTime.hour,reportTime2.taskConfig[j].lastReportTime.minute,reportTime2.taskConfig[j].lastReportTime.second);
          printf("任务序号=%d,j=%d,初始化nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n", reportTime2.taskConfig[j].taskNum, j, reportTime2.taskConfig[j].nextReportTime.year,reportTime2.taskConfig[j].nextReportTime.month,reportTime2.taskConfig[j].nextReportTime.day,reportTime2.taskConfig[j].nextReportTime.hour,reportTime2.taskConfig[j].nextReportTime.minute,reportTime2.taskConfig[j].nextReportTime.second);
        }

	      j++;
    	}//if
    }//for
    
    initTasking = FALSE;
    initReportFlag = 0;

    printf("初始化主动上报任务队列线程结束\n");
}

/*******************************************************
函数名称:reportTask1
功能描述:定时发送一类数据任务处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void activeReport1(void)
{
    INT16U     i, loop,tail;
    INT8U      interval;
    INT8U      pseudoFrame[10240];    //定时发送任务的伪分组
    
    //遍历定时发送一类数据发送时间，当某任务的发送时间等于系统时间，作上报处理
    for (i = 0; i < reportTime1.numOfTask; i++)
    {
      if ((compareTwoTime(reportTime1.taskConfig[i].nextReportTime,sysTime)==TRUE) && (reportTime1.taskConfig[i].taskNum!=0))
      {
        //按照时间表中的任务代码编号查找对应的任务配置并作处理
        for (loop = 0; loop < 64; loop++)
        {
          if (reportTask1.task[loop].taskNum == reportTime1.taskConfig[i].taskNum)
          {
          	//如果自动上报是允许的，在这里向AFN0C函数发送一个伪帧
           #ifdef DKY_SUBMISSION
          	if ((reportTask1.task[loop].stopFlag == TASK_START) && ((callAndReport&0x03) == 0x01))
           #else
          	//2012-09-07,重庆南坪局发现3台设备不停的发送数据,经查是地玮主站下发的任务的发送周期是0
          	//           更改处理为:如果周期是0的不发送
          	//if (reportTask1.task[loop].stopFlag == TASK_START)
          	if ((reportTask1.task[loop].stopFlag == TASK_START) && (reportTask1.task[loop].sendPeriod>0))
           #endif
          	{
            	//逐项填写伪分组的内容
            	for (tail = 0; tail < reportTask1.task[loop].numOfDataId; tail++)
            	{
            	 	//fn, pn
            	 	pseudoFrame[tail*4] = reportTask1.task[loop].dataUnit[tail].pn1;
                pseudoFrame[tail*4+1] = reportTask1.task[loop].dataUnit[tail].pn2;
                   
                pseudoFrame[tail*4+2] = 0x01<<((reportTask1.task[loop].dataUnit[tail].fn%8 == 0) ? 7 : (reportTask1.task[loop].dataUnit[tail].fn%8-1));
                pseudoFrame[tail*4+3] = (reportTask1.task[loop].dataUnit[tail].fn - 1) / 8;
                  
                if (tail*4>10200)
                {
                  break;
                }
              }
            	 	
            	//调用函数组帧,待ipPermit==TRUE后发送
            	AFN0C(pseudoFrame,pseudoFrame+tail*4, DATA_FROM_GPRS,ACTIVE_REPORT);
            }
            
            //最后计算并记录下一次发送时间
          	interval = reportTask1.task[loop].sendPeriod & 0x3f;
         	  
            //取发送时间间隔的单位
            switch (reportTask1.task[loop].sendPeriod>>6 & 0x03)
            {
              case 0:  //分
              	reportTime1.taskConfig[i].nextReportTime 
              	    = nextTime(reportTime1.taskConfig[i].nextReportTime, interval, 0);
              	break;
              	
              case 1:  //时
                reportTime1.taskConfig[i].nextReportTime 
                    = nextTime(reportTime1.taskConfig[i].nextReportTime, interval*60, 0);
              	break;
              	
              case 2:  //日
                reportTime1.taskConfig[i].nextReportTime 
                    = nextTime(reportTime1.taskConfig[i].nextReportTime, interval*60*24, 0);
              	break;
              	
              case 3:  //月
                reportTime1.taskConfig[i].nextReportTime.month += interval;
                
                if (reportTime1.taskConfig[i].nextReportTime.month > 12)
                {
                  	reportTime1.taskConfig[i].nextReportTime.month -= 12;
                    reportTime1.taskConfig[i].nextReportTime.year++;
                }
              	break;
            }
          }
        }
      }
    }
}

/*******************************************************
函数名称:activeReport3
功能描述:主动上报三类数据(事件)处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void activeReport3(void)
{
   INT8U pseudoFrame[6];

   //在允许主动上报的条件下向主站报告 
   if ((callAndReport&0x03) == 0x01)
   {
     //读出终端记录中已读的事件指针和最大事件指针
     selectParameter(88, 2,eventReadedPointer,2);
     
     //if (eventReadedPointer[1] < iEventCounter || (eventReadedPointer[1]==255 && iEventCounter==255))
     if (eventReadedPointer[1] < iEventCounter)
     {
       pseudoFrame[0] = 0x00;
       pseudoFrame[1] = 0x00;
       pseudoFrame[2] = 0x01;
     	 pseudoFrame[3] = 0x00;
     
       if (debugInfo&PRINT_EVENT_DEBUG)
       {
     	   printf("activeReport3:调用主动上报.已读指针=%d,最大事件计数器=%d\n",eventReadedPointer[1],iEventCounter);
       }
     	 
       if (eventReadedPointer[1]<255)
       {
     	   pseudoFrame[4] = eventReadedPointer[1];
     	 }
     	 else
     	 {
     	   pseudoFrame[4] = 255;
     	 }
     	 pseudoFrame[5] = iEventCounter;

    	 AFN0E(pseudoFrame, pseudoFrame+6, DATA_FROM_GPRS, ACTIVE_REPORT);
    	
       //如果有数需要发送
       if (fQueue.activeSendPtr!=fQueue.activeTailPtr)
       {
          //启动定时器发送TCP数据
          addFrameFlag(TRUE,FALSE);
          fQueue.activeFrameSend = TRUE;
       }
     }
   }
}

/***************************************************
函数名称:threadOfReport2
功能描述:维护终端线程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
***************************************************/
void threadOfReport2(void *arg)
{
   INT16U     i, loop, dataDone;
   INT16U     tail;
   INT8U      interval, byteOfTime;
   INT8U      density;             //密度
   INT8U      densityInterval;     //密度的时间间隔(15,30,60分钟)
   INT8U      pseudoFrame[10240];  //定时发送任务的伪分组
   DATE_TIME  backUpTime;          //备注时间
   INT8U      defaultM=1;          //默认数据冻结密度
	 DATE_TIME  startTime;

   while(1)
   {
     //ly,2011-08-04,添加if判断,因测试时发现,一对时的话,如果有任务配置的话就会产生很多主动上报2类数据的线程
     if (initTasking==FALSE)
     {
       //遍历定时发送二类数据发送时间，当某任务的发送时间等于系统时间，作上报处理
       for (i = 0; i < reportTime2.numOfTask; i++)
       {
         if ((compareTwoTime(reportTime2.taskConfig[i].nextReportTime,sysTime)==TRUE)
         	  && (reportTime2.taskConfig[i].taskNum!=0)
         	   && (initTasking==FALSE)
         	 )
         {
           //查找对应的任务配置并作处理
           for (loop = 0; loop < 64; loop++)
           {
             if (reportTask2.task[loop].taskNum == reportTime2.taskConfig[i].taskNum)
             {
             	 interval = reportTask2.task[loop].sendPeriod & 0x3f;
             	
             	 if (debugInfo&PRINT_ACTIVE_REPORT)
             	 {
             	  //printf("interval=%d\n",interval);
             	 }
             	
             	 //如果自动上报是允许的，在这里向AFN0D函数发送一个伪帧
              #ifdef DKY_SUBMISSION   //ly,2011-10-18,add
          	   if ((reportTask2.task[loop].stopFlag == TASK_START) && ((callAndReport&0x03) == 0x01))
              #else
          	   //2012-09-07,重庆南坪局发现3台设备不停的发送数据,经查是地玮主站下发的任务的发送周期是0
          	   //           更改处理为:如果周期是0的不发送
             	 //if (reportTask2.task[loop].stopFlag == TASK_START)
          	   if ((reportTask2.task[loop].stopFlag == TASK_START) && (reportTask2.task[loop].sendPeriod>0))
             	#endif	
             	 {
                 reportTime2.taskConfig[i].backLastReportTime = reportTime2.taskConfig[i].lastReportTime;

             	   startTime = reportTime2.taskConfig[i].backLastReportTime;
             	 
             	   if (debugInfo&PRINT_ACTIVE_REPORT)
             	   {
             	     printf("threadOfReport2:任务号%d,序号loop=%d线程开始,时间序号=%d,发送起始时间=%d-%d-%d %d:%d:%d\n",reportTask2.task[loop].taskNum,
             	            loop,i,startTime.year,startTime.month,startTime.day,startTime.hour,startTime.minute,startTime.second);
             	   }
                
                 interval = reportTask2.task[loop].sendPeriod & 0x3f;
             	
              	 //逐项填写伪分组的内容
             	   tail = 0;
             	
             	   //dataDone代表伪帧中已经填入的数据单元标示个数          		
             	   for (dataDone = 1;dataDone <= reportTask2.task[loop].numOfDataId;dataDone++)
             	   {
              	   backUpTime = startTime;
              	 	
              	   pseudoFrame[tail++] = reportTask2.task[loop].dataUnit[dataDone-1].pn1;
                   pseudoFrame[tail++] = reportTask2.task[loop].dataUnit[dataDone-1].pn2;
                  
                   pseudoFrame[tail++] = 0x01<<((reportTask2.task[loop].dataUnit[dataDone-1].fn%8 == 0) ? 7 : (reportTask2.task[loop].dataUnit[dataDone-1].fn%8-1));
                   pseudoFrame[tail++] = (reportTask2.task[loop].dataUnit[dataDone-1].fn - 1) / 8;
                  
                   //填写时标
                   if ((reportTask2.task[loop].dataUnit[dataDone-1].fn >= 1 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 12)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 25 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 32)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 41 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 43)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 49 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 50)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 57 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 59)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 113 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 118)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 121 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 123)
                     
                     #ifdef LIGHTING
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 149 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 152)
                     #endif

                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 153 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 156)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 161 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 176)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 185 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 192)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 45) || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 53)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 129) || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 209))
                   {
                  	 byteOfTime = 3;        //日冻结、抄表日冻结
                   }
                  
                   if ((reportTask2.task[loop].dataUnit[dataDone-1].fn >= 17 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 24)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 33 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 39)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 44)|| (reportTask2.task[loop].dataUnit[dataDone-1].fn == 46)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 51 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 52)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 54)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 60 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 62)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 65 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 66)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 130)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 177 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 184)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 193 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 196)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 201 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 208)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 213 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 216))
                   {
                  	 byteOfTime = 2;        //月冻结
                   }
                  
                  if ((reportTask2.task[loop].dataUnit[dataDone-1].fn >= 73 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 76)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 77 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 80)    //2017-8-16,扩展,为同远
					  || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 81 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 95)
                      || reportTask2.task[loop].dataUnit[dataDone-1].fn == 96                                                              //2017-8-16,扩展,为同远
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 97 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 110)
                     #ifdef LIGHTING
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 111 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 112)
                     #else
					  ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 111)                                                            //2017-9-22,扩展,开关量曲线
                     #endif 
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 138)
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 145 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 148)
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 157 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 160)
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 217 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 218))
                  {
                  	 byteOfTime = 7;       //曲线
                  }
                  
                  switch (byteOfTime)
                  {
                  	case 3:    //日冻结、抄表日冻结
                  		pseudoFrame[tail++] = startTime.day/10<<4   | startTime.day%10;
                  		pseudoFrame[tail++] = startTime.month/10<<4 | startTime.month%10;
                  		pseudoFrame[tail++] = startTime.year/10<<4  | startTime.year%10;
                  		break;
                  		
                  	case 2:   //月冻结
                  		pseudoFrame[tail++] = startTime.month/10<<4 | startTime.month%10;
                  		pseudoFrame[tail++] = startTime.year/10<<4  | startTime.year%10;
                  		break;
                  		
                  	case 7:   //曲线
                  	 #ifdef LIGHTING
										  
											density = reportTask2.task[loop].sampleTimes;
										 
										 #else
											
											#ifdef PLUG_IN_CARRIER_MODULE
                  		 defaultM = 3;    //载波默认密度为3,冻结间隔为60min
                  		#else
                  		 defaultM = 1;    //载波默认密度为3,冻结间隔为15min
                  		#endif
                  		
                  		switch(reportTask2.task[loop].sampleTimes)
                  		{
                  			 case 1:
                  			 	 if (debugInfo&PRINT_ACTIVE_REPORT)
                  			 	 {
                  			 	   printf("threadOfReport2:任务号%d,抽取倍率为1\n", reportTask2.task[loop].taskNum);
                  			 	 }
                  			 	  
                  			 	 density = defaultM*1;
                  			 	 break;
             
                  			 case 2:
                  			 	 if (debugInfo&PRINT_ACTIVE_REPORT)
                  			 	 {
                  			 	   printf("threadOfReport2:任务号%d,抽取倍率为2\n", reportTask2.task[loop].taskNum);
                  			 	 }
                  			 	 
                  			 	 density = defaultM*2;
                  			 	 break;
                  			 
                  			 default:
                  			 	 if (debugInfo&PRINT_ACTIVE_REPORT)
                  			 	 {
                    			 	 printf("threadOfReport2:任务号%d,抽取倍率>=3\n", reportTask2.task[loop].taskNum);
                    			 }
                    			 
                  			 	 density = defaultM*3;
                  			 	 break;
                  		}
                  		
                  		if (density>3)
                  		{
                  			 density = 3;
                  		}
										 #endif
                  		
                  		switch(density)
                  	  {
                  	  	 case 1:
                  	  	 	 densityInterval = 15;
                  	  	 	 break;
             
                  	  	 case 2:
                  	  	 	 densityInterval = 30;
                  	  	 	 break;

												case 254:
                  	  	 	 densityInterval = 5;
                  	  	 	 break;

												case 255:
                  	  	 	 densityInterval = 1;
                  	  	 	 break;
             
                  	  	 default:
                  	  	 	 densityInterval = 60;
                  	  	 	 break;
                  	  }
                  		
                  		if (debugInfo&PRINT_ACTIVE_REPORT)
                  		{
                  		  printf("threadOfReport2:任务号%d,density=%d,densityInterval=%d\n",reportTask2.task[loop].taskNum,density,densityInterval);
                  		}
                  		
                  		//2009.06.20将数据时标改成与15,30,45,00minute对齐，原来是根据任务发送基准的未对齐时刻
                  		startTime.minute = (startTime.minute/densityInterval)*densityInterval;
										 #ifdef LIGHTING
										  if (density==254)    //2018-07-14,add这个判断
										  {
										 	 	startTime = nextTime(startTime, densityInterval, 0);	
										  }
										 #else
										  startTime = nextTime(startTime, densityInterval, 0);
										 #endif
             
                  		pseudoFrame[tail++] = startTime.minute/10<<4 | startTime.minute%10;
                  		pseudoFrame[tail++] = startTime.hour/10<<4   | startTime.hour%10;
                  		pseudoFrame[tail++] = startTime.day/10<<4    | startTime.day%10;
                  		pseudoFrame[tail++] = startTime.month/10<<4  | startTime.month%10;
                  		pseudoFrame[tail++] = startTime.year/10<<4   | startTime.year%10;
                  		
                  		//密度
                  		pseudoFrame[tail++] = density;
                  		                		
                  		//数据点数等于以分钟为单位的发送间隔除以冻结密度
                  		switch (reportTask2.task[loop].sendPeriod>>6 & 0x03)
                  		{
                  		  case 0:
                  		  	pseudoFrame[tail++] = interval / densityInterval;
                  		  	break;
             
                  		  case 1:
                  		  	pseudoFrame[tail++] = (interval*60) / densityInterval;
                  		  	break;
             
                  		  case 2:
                  		  	pseudoFrame[tail++] = (interval*60*24) / densityInterval;
                  		  	break;
             
                  		  case 3:
                  		  	pseudoFrame[tail++] = (interval*60*24*30) / densityInterval;
                  		  	break;
                  		}
                  		break;
                  	
                  	default:
                  		continue;
                  }
                  
              	  startTime = backUpTime;
             
                  if (tail>10200)
                  {
                  	 break;
                  }
              	 }
              	
              	 if (debugInfo&PRINT_ACTIVE_REPORT)
              	 {
              	  //sendDebugFrame(pseudoFrame,tail);
              	 }
              	
              	 if (debugInfo&PRINT_ACTIVE_REPORT)
              	 {
              	   printf("threadOfReport2:任务号%d,开始调用AFN0D\n",reportTask2.task[loop].taskNum);
              	 }
             
              	 //调用函数组帧,待ipPermit==TRUE后发送
              	 pseudoFrame[tail+1] = reportTask2.task[loop].taskNum;   
              	 
              	 AFN0D(pseudoFrame,pseudoFrame+tail, DATA_FROM_GPRS, ACTIVE_REPORT);
              	 
              	 if (debugInfo&PRINT_ACTIVE_REPORT)
              	 {
              	   printf("threadOfReport2,任务号%d执行完成\n",reportTask2.task[loop].taskNum);
              	 }                
               }
               
               //最后,保存上一次发送时间,计算并记录下一次发送时间
             	 reportTime2.taskConfig[i].lastReportTime = reportTime2.taskConfig[i].nextReportTime;
               
               //取单位
               switch ((reportTask2.task[loop].sendPeriod>>6) & 0x03)
               {
                 case 0:  //分
								   while(1)
								   {
										 reportTime2.taskConfig[i].nextReportTime
											 = nextTime(reportTime2.taskConfig[i].nextReportTime, interval, 0);
								   
									 	 if (compareTwoTime(sysTime, reportTime2.taskConfig[i].nextReportTime))
									 	 {
									 	   break;
									 	 }
								   }
									 
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:任务号%d,发送周期单位为分nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                 	 break;
                 	
                 case 1:  //时
                   reportTime2.taskConfig[i].nextReportTime
                      = nextTime(reportTime2.taskConfig[i].nextReportTime, interval*60, 0);
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:任务号%d,发送周期单位为时nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                   break;
                 	
                 case 2:  //日
                   reportTime2.taskConfig[i].nextReportTime
                      = nextTime(reportTime2.taskConfig[i].nextReportTime, interval*60*24, 0);
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:任务号%d,发送周期单位为日nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                 	 break;
                 	
                 case 3:  //月
                   reportTime2.taskConfig[i].nextReportTime.month += interval;
                   
                   if (reportTime2.taskConfig[i].nextReportTime.month > 12)
                   {
                     reportTime2.taskConfig[i].nextReportTime.month -= 12;
                     reportTime2.taskConfig[i].nextReportTime.year++;
                   }
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:任务号%d,发送周期单位为月nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                 	 break;
                 	
                 //default:
               }
             }
           }
           
           //ly,2010-11-23,同一时刻如果有多个任务的话,几个线程的参数会同时执行最后一个调用的线程参数,
           //              这是一个错误。
           //          修正:如果有一个任务创建了线程即退出本循环,待下一秒再执行下一个任务的线程。
           break;
         }
         
         usleep(5000);
       }  
     }
     
     usleep(500000);
   }
}
