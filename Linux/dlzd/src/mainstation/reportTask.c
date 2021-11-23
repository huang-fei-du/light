/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����repotTask.c
���ߣ�TianYe
�汾��0.9
������ڣ�2006��9��
��������ʱ���������ļ���
�����б�
�޸���ʷ��
  01,06-09-05,TianYe created.
***************************************************/

#include "stdio.h"
#include "stdlib.h"

#include "workWithMeter.h"
#include "teRunPara.h"
#include "reportTask.h"

extern void sendDebugFrame(INT8U *pack,INT16U length);

void threadOfReport2(void *arg);

BOOL initTasking;      //���ڳ�ʼ�������־

/*******************************************************
��������:initReportTask
��������:��ʼ����ʱ�����������������һ�η���ʱ���ȷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void initReportTask(void *arg)
{
    INT8U   i, j = 0;;
    INT8U   interval, rate;
    
    initTasking = TRUE;

    printf("��ʼ�������ϱ���������߳̿�ʼ\n");
    
    //��ʼ����ʱ����һ������ʱ���
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
    		//������������������д����ʱ���
	      reportTime1.taskConfig[j].taskNum = reportTask1.task[i].taskNum;
	      reportTime1.taskConfig[j].nextReportTime = reportTask1.task[i].sendStart;
	      reportTime1.taskConfig[j].nextReportTime.month &= 0x1F;
	      
	      interval = reportTask1.task[i].sendPeriod & 0x3f;
	      
	      rate = reportTask1.task[i].sendPeriod>>6 & 0x03;
	      
	      //���ϵͳʱ����ڷ��ͳ�ʼʱ�䣬���ڷ��ͳ�ʼʱ��Ļ����ϼ��Ϸ��ͼ����ֱ������ϵͳʱ��
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
    
    printf("��ʼ��������������ʱ���\n");

    //��ʼ����ʱ���Ͷ�������ʱ���
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
    		//������������������д����ʱ���
	      reportTime2.taskConfig[j].taskNum = reportTask2.task[i].taskNum;
	      reportTime2.taskConfig[j].nextReportTime = reportTask2.task[i].sendStart;
	      
	      interval = reportTask2.task[i].sendPeriod & 0x3f;
	      rate = reportTask2.task[i].sendPeriod>>6 & 0x03;
	      
	      if (interval==0)
	      {
	      	 continue;
	      }
	      
	      //���ϵͳʱ����ڷ��ͳ�ʼʱ�䣬���ڷ��ͳ�ʼʱ��Ļ����ϼ��Ϸ��ͼ����ֱ������ϵͳʱ��
	      if (compareTwoTime(sysTime, reportTime2.taskConfig[j].nextReportTime) == FALSE)
	      {
	        do
	        {
	        	//����һ�η���ʱ�䱣��Ϊ��һ�η���ʱ��
	          reportTime2.taskConfig[j].lastReportTime = reportTime2.taskConfig[j].nextReportTime;
	          
	          //����һ�η���ʱ������һ�����ͼ��
	          switch (rate)
	          {
	            case 0:   //��
	            	reportTime2.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].nextReportTime, interval, 0);
	            	break;
	            	
	            case 1:   //ʱ
	            	reportTime2.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].nextReportTime, interval*60, 0);
	            	break;
	            
	            case 2:   //��
	            	reportTime2.taskConfig[j].nextReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].nextReportTime, interval*60*24, 0);
	            	break;
	            	
	            case 3:   //��
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
	      else   //�����һ�η���ʱ�����ϵͳʱ�䣬����һ�η���ʱ�������һ�η���ʱ������
	      {
	      	 reportTime2.taskConfig[j].lastReportTime = reportTime2.taskConfig[j].nextReportTime;
	      	
	         switch (rate)
	         {
	            case 0:    //��
	            	reportTime2.taskConfig[j].lastReportTime 
	            	     = backTime(reportTime2.taskConfig[j].lastReportTime,0,0,0, interval, 0);
	            	break;
	            	
	            case 1:    //ʱ
	            	reportTime2.taskConfig[j].lastReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].lastReportTime, interval*60, 0);
	            	break;
	            
	            case 2:    //��
	            	reportTime2.taskConfig[j].lastReportTime 
	            	     = nextTime(reportTime2.taskConfig[j].lastReportTime, interval*60*24, 0);
	            	break;
	            	
	            case 3:    //��
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
          printf("�������=%d,j=%d,��ʼ��lastReportTime:%02d-%02d-%02d %02d:%02d:%02d\n", reportTime2.taskConfig[j].taskNum, j, reportTime2.taskConfig[j].lastReportTime.year,reportTime2.taskConfig[j].lastReportTime.month,reportTime2.taskConfig[j].lastReportTime.day,reportTime2.taskConfig[j].lastReportTime.hour,reportTime2.taskConfig[j].lastReportTime.minute,reportTime2.taskConfig[j].lastReportTime.second);
          printf("�������=%d,j=%d,��ʼ��nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n", reportTime2.taskConfig[j].taskNum, j, reportTime2.taskConfig[j].nextReportTime.year,reportTime2.taskConfig[j].nextReportTime.month,reportTime2.taskConfig[j].nextReportTime.day,reportTime2.taskConfig[j].nextReportTime.hour,reportTime2.taskConfig[j].nextReportTime.minute,reportTime2.taskConfig[j].nextReportTime.second);
        }

	      j++;
    	}//if
    }//for
    
    initTasking = FALSE;
    initReportFlag = 0;

    printf("��ʼ�������ϱ���������߳̽���\n");
}

/*******************************************************
��������:reportTask1
��������:��ʱ����һ������������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void activeReport1(void)
{
    INT16U     i, loop,tail;
    INT8U      interval;
    INT8U      pseudoFrame[10240];    //��ʱ���������α����
    
    //������ʱ����һ�����ݷ���ʱ�䣬��ĳ����ķ���ʱ�����ϵͳʱ�䣬���ϱ�����
    for (i = 0; i < reportTime1.numOfTask; i++)
    {
      if ((compareTwoTime(reportTime1.taskConfig[i].nextReportTime,sysTime)==TRUE) && (reportTime1.taskConfig[i].taskNum!=0))
      {
        //����ʱ����е���������Ų��Ҷ�Ӧ���������ò�������
        for (loop = 0; loop < 64; loop++)
        {
          if (reportTask1.task[loop].taskNum == reportTime1.taskConfig[i].taskNum)
          {
          	//����Զ��ϱ�������ģ���������AFN0C��������һ��α֡
           #ifdef DKY_SUBMISSION
          	if ((reportTask1.task[loop].stopFlag == TASK_START) && ((callAndReport&0x03) == 0x01))
           #else
          	//2012-09-07,������ƺ�ַ���3̨�豸��ͣ�ķ�������,�����ǵ�����վ�·�������ķ���������0
          	//           ���Ĵ���Ϊ:���������0�Ĳ�����
          	//if (reportTask1.task[loop].stopFlag == TASK_START)
          	if ((reportTask1.task[loop].stopFlag == TASK_START) && (reportTask1.task[loop].sendPeriod>0))
           #endif
          	{
            	//������дα���������
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
            	 	
            	//���ú�����֡,��ipPermit==TRUE����
            	AFN0C(pseudoFrame,pseudoFrame+tail*4, DATA_FROM_GPRS,ACTIVE_REPORT);
            }
            
            //�����㲢��¼��һ�η���ʱ��
          	interval = reportTask1.task[loop].sendPeriod & 0x3f;
         	  
            //ȡ����ʱ�����ĵ�λ
            switch (reportTask1.task[loop].sendPeriod>>6 & 0x03)
            {
              case 0:  //��
              	reportTime1.taskConfig[i].nextReportTime 
              	    = nextTime(reportTime1.taskConfig[i].nextReportTime, interval, 0);
              	break;
              	
              case 1:  //ʱ
                reportTime1.taskConfig[i].nextReportTime 
                    = nextTime(reportTime1.taskConfig[i].nextReportTime, interval*60, 0);
              	break;
              	
              case 2:  //��
                reportTime1.taskConfig[i].nextReportTime 
                    = nextTime(reportTime1.taskConfig[i].nextReportTime, interval*60*24, 0);
              	break;
              	
              case 3:  //��
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
��������:activeReport3
��������:�����ϱ���������(�¼�)����
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void activeReport3(void)
{
   INT8U pseudoFrame[6];

   //�����������ϱ�������������վ���� 
   if ((callAndReport&0x03) == 0x01)
   {
     //�����ն˼�¼���Ѷ����¼�ָ�������¼�ָ��
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
     	   printf("activeReport3:���������ϱ�.�Ѷ�ָ��=%d,����¼�������=%d\n",eventReadedPointer[1],iEventCounter);
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
    	
       //���������Ҫ����
       if (fQueue.activeSendPtr!=fQueue.activeTailPtr)
       {
          //������ʱ������TCP����
          addFrameFlag(TRUE,FALSE);
          fQueue.activeFrameSend = TRUE;
       }
     }
   }
}

/***************************************************
��������:threadOfReport2
��������:ά���ն��߳�
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
void threadOfReport2(void *arg)
{
   INT16U     i, loop, dataDone;
   INT16U     tail;
   INT8U      interval, byteOfTime;
   INT8U      density;             //�ܶ�
   INT8U      densityInterval;     //�ܶȵ�ʱ����(15,30,60����)
   INT8U      pseudoFrame[10240];  //��ʱ���������α����
   DATE_TIME  backUpTime;          //��עʱ��
   INT8U      defaultM=1;          //Ĭ�����ݶ����ܶ�
	 DATE_TIME  startTime;

   while(1)
   {
     //ly,2011-08-04,���if�ж�,�����ʱ����,һ��ʱ�Ļ�,������������õĻ��ͻ�����ܶ������ϱ�2�����ݵ��߳�
     if (initTasking==FALSE)
     {
       //������ʱ���Ͷ������ݷ���ʱ�䣬��ĳ����ķ���ʱ�����ϵͳʱ�䣬���ϱ�����
       for (i = 0; i < reportTime2.numOfTask; i++)
       {
         if ((compareTwoTime(reportTime2.taskConfig[i].nextReportTime,sysTime)==TRUE)
         	  && (reportTime2.taskConfig[i].taskNum!=0)
         	   && (initTasking==FALSE)
         	 )
         {
           //���Ҷ�Ӧ���������ò�������
           for (loop = 0; loop < 64; loop++)
           {
             if (reportTask2.task[loop].taskNum == reportTime2.taskConfig[i].taskNum)
             {
             	 interval = reportTask2.task[loop].sendPeriod & 0x3f;
             	
             	 if (debugInfo&PRINT_ACTIVE_REPORT)
             	 {
             	  //printf("interval=%d\n",interval);
             	 }
             	
             	 //����Զ��ϱ�������ģ���������AFN0D��������һ��α֡
              #ifdef DKY_SUBMISSION   //ly,2011-10-18,add
          	   if ((reportTask2.task[loop].stopFlag == TASK_START) && ((callAndReport&0x03) == 0x01))
              #else
          	   //2012-09-07,������ƺ�ַ���3̨�豸��ͣ�ķ�������,�����ǵ�����վ�·�������ķ���������0
          	   //           ���Ĵ���Ϊ:���������0�Ĳ�����
             	 //if (reportTask2.task[loop].stopFlag == TASK_START)
          	   if ((reportTask2.task[loop].stopFlag == TASK_START) && (reportTask2.task[loop].sendPeriod>0))
             	#endif	
             	 {
                 reportTime2.taskConfig[i].backLastReportTime = reportTime2.taskConfig[i].lastReportTime;

             	   startTime = reportTime2.taskConfig[i].backLastReportTime;
             	 
             	   if (debugInfo&PRINT_ACTIVE_REPORT)
             	   {
             	     printf("threadOfReport2:�����%d,���loop=%d�߳̿�ʼ,ʱ�����=%d,������ʼʱ��=%d-%d-%d %d:%d:%d\n",reportTask2.task[loop].taskNum,
             	            loop,i,startTime.year,startTime.month,startTime.day,startTime.hour,startTime.minute,startTime.second);
             	   }
                
                 interval = reportTask2.task[loop].sendPeriod & 0x3f;
             	
              	 //������дα���������
             	   tail = 0;
             	
             	   //dataDone����α֡���Ѿ���������ݵ�Ԫ��ʾ����          		
             	   for (dataDone = 1;dataDone <= reportTask2.task[loop].numOfDataId;dataDone++)
             	   {
              	   backUpTime = startTime;
              	 	
              	   pseudoFrame[tail++] = reportTask2.task[loop].dataUnit[dataDone-1].pn1;
                   pseudoFrame[tail++] = reportTask2.task[loop].dataUnit[dataDone-1].pn2;
                  
                   pseudoFrame[tail++] = 0x01<<((reportTask2.task[loop].dataUnit[dataDone-1].fn%8 == 0) ? 7 : (reportTask2.task[loop].dataUnit[dataDone-1].fn%8-1));
                   pseudoFrame[tail++] = (reportTask2.task[loop].dataUnit[dataDone-1].fn - 1) / 8;
                  
                   //��дʱ��
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
                  	 byteOfTime = 3;        //�ն��ᡢ�����ն���
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
                  	 byteOfTime = 2;        //�¶���
                   }
                  
                  if ((reportTask2.task[loop].dataUnit[dataDone-1].fn >= 73 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 76)
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 77 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 80)    //2017-8-16,��չ,ΪͬԶ
					  || (reportTask2.task[loop].dataUnit[dataDone-1].fn >= 81 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 95)
                      || reportTask2.task[loop].dataUnit[dataDone-1].fn == 96                                                              //2017-8-16,��չ,ΪͬԶ
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 97 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 110)
                     #ifdef LIGHTING
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 111 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 112)
                     #else
					  ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 111)                                                            //2017-9-22,��չ,����������
                     #endif 
                      || (reportTask2.task[loop].dataUnit[dataDone-1].fn == 138)
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 145 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 148)
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 157 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 160)
                      ||(reportTask2.task[loop].dataUnit[dataDone-1].fn >= 217 && reportTask2.task[loop].dataUnit[dataDone-1].fn <= 218))
                  {
                  	 byteOfTime = 7;       //����
                  }
                  
                  switch (byteOfTime)
                  {
                  	case 3:    //�ն��ᡢ�����ն���
                  		pseudoFrame[tail++] = startTime.day/10<<4   | startTime.day%10;
                  		pseudoFrame[tail++] = startTime.month/10<<4 | startTime.month%10;
                  		pseudoFrame[tail++] = startTime.year/10<<4  | startTime.year%10;
                  		break;
                  		
                  	case 2:   //�¶���
                  		pseudoFrame[tail++] = startTime.month/10<<4 | startTime.month%10;
                  		pseudoFrame[tail++] = startTime.year/10<<4  | startTime.year%10;
                  		break;
                  		
                  	case 7:   //����
                  	 #ifdef LIGHTING
										  
											density = reportTask2.task[loop].sampleTimes;
										 
										 #else
											
											#ifdef PLUG_IN_CARRIER_MODULE
                  		 defaultM = 3;    //�ز�Ĭ���ܶ�Ϊ3,������Ϊ60min
                  		#else
                  		 defaultM = 1;    //�ز�Ĭ���ܶ�Ϊ3,������Ϊ15min
                  		#endif
                  		
                  		switch(reportTask2.task[loop].sampleTimes)
                  		{
                  			 case 1:
                  			 	 if (debugInfo&PRINT_ACTIVE_REPORT)
                  			 	 {
                  			 	   printf("threadOfReport2:�����%d,��ȡ����Ϊ1\n", reportTask2.task[loop].taskNum);
                  			 	 }
                  			 	  
                  			 	 density = defaultM*1;
                  			 	 break;
             
                  			 case 2:
                  			 	 if (debugInfo&PRINT_ACTIVE_REPORT)
                  			 	 {
                  			 	   printf("threadOfReport2:�����%d,��ȡ����Ϊ2\n", reportTask2.task[loop].taskNum);
                  			 	 }
                  			 	 
                  			 	 density = defaultM*2;
                  			 	 break;
                  			 
                  			 default:
                  			 	 if (debugInfo&PRINT_ACTIVE_REPORT)
                  			 	 {
                    			 	 printf("threadOfReport2:�����%d,��ȡ����>=3\n", reportTask2.task[loop].taskNum);
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
                  		  printf("threadOfReport2:�����%d,density=%d,densityInterval=%d\n",reportTask2.task[loop].taskNum,density,densityInterval);
                  		}
                  		
                  		//2009.06.20������ʱ��ĳ���15,30,45,00minute���룬ԭ���Ǹ��������ͻ�׼��δ����ʱ��
                  		startTime.minute = (startTime.minute/densityInterval)*densityInterval;
										 #ifdef LIGHTING
										  if (density==254)    //2018-07-14,add����ж�
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
                  		
                  		//�ܶ�
                  		pseudoFrame[tail++] = density;
                  		                		
                  		//���ݵ��������Է���Ϊ��λ�ķ��ͼ�����Զ����ܶ�
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
              	   printf("threadOfReport2:�����%d,��ʼ����AFN0D\n",reportTask2.task[loop].taskNum);
              	 }
             
              	 //���ú�����֡,��ipPermit==TRUE����
              	 pseudoFrame[tail+1] = reportTask2.task[loop].taskNum;   
              	 
              	 AFN0D(pseudoFrame,pseudoFrame+tail, DATA_FROM_GPRS, ACTIVE_REPORT);
              	 
              	 if (debugInfo&PRINT_ACTIVE_REPORT)
              	 {
              	   printf("threadOfReport2,�����%dִ�����\n",reportTask2.task[loop].taskNum);
              	 }                
               }
               
               //���,������һ�η���ʱ��,���㲢��¼��һ�η���ʱ��
             	 reportTime2.taskConfig[i].lastReportTime = reportTime2.taskConfig[i].nextReportTime;
               
               //ȡ��λ
               switch ((reportTask2.task[loop].sendPeriod>>6) & 0x03)
               {
                 case 0:  //��
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
                     printf("threadOfReport2:�����%d,�������ڵ�λΪ��nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                 	 break;
                 	
                 case 1:  //ʱ
                   reportTime2.taskConfig[i].nextReportTime
                      = nextTime(reportTime2.taskConfig[i].nextReportTime, interval*60, 0);
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:�����%d,�������ڵ�λΪʱnextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                   break;
                 	
                 case 2:  //��
                   reportTime2.taskConfig[i].nextReportTime
                      = nextTime(reportTime2.taskConfig[i].nextReportTime, interval*60*24, 0);
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:�����%d,�������ڵ�λΪ��nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                 	 break;
                 	
                 case 3:  //��
                   reportTime2.taskConfig[i].nextReportTime.month += interval;
                   
                   if (reportTime2.taskConfig[i].nextReportTime.month > 12)
                   {
                     reportTime2.taskConfig[i].nextReportTime.month -= 12;
                     reportTime2.taskConfig[i].nextReportTime.year++;
                   }
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("threadOfReport2:�����%d,�������ڵ�λΪ��nextReportTime:%02d-%02d-%02d %02d:%02d:%02d\n",reportTask2.task[loop].taskNum,reportTime2.taskConfig[i].nextReportTime.year,reportTime2.taskConfig[i].nextReportTime.month,reportTime2.taskConfig[i].nextReportTime.day,reportTime2.taskConfig[i].nextReportTime.hour,reportTime2.taskConfig[i].nextReportTime.minute,reportTime2.taskConfig[i].nextReportTime.second);
                   }
                 	 break;
                 	
                 //default:
               }
             }
           }
           
           //ly,2010-11-23,ͬһʱ������ж������Ļ�,�����̵߳Ĳ�����ͬʱִ�����һ�����õ��̲߳���,
           //              ����һ������
           //          ����:�����һ�����񴴽����̼߳��˳���ѭ��,����һ����ִ����һ��������̡߳�
           break;
         }
         
         usleep(5000);
       }  
     }
     
     usleep(500000);
   }
}
