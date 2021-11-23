/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN0B.c
���ߣ�TianYe
�汾��0.9
������ڣ�2007��3��12��
��������վAFN0B(������������)�����ļ���
�����б�
�޸���ʷ��
  01,07-03-12,TinaYe created.
  02,10-04-03,Leiyong,��ֲ��AT91SAM9260,Q/GDW376.1.  
***************************************************/

#include "teRunPara.h"
#include "msSetPara.h"

#include "AFN0B.h"

/*******************************************************
��������:AFN0B
��������:��վ"��ʾ��������"(AFN0B)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN0B(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom)
{
	 INT8U      pn, fn;
   INT8U      maxCycle;                  //���ѭ������
   INT8U      tmpDtCount;                //DT��λ����
   INT8U      tmpDt1;                    //��ʱDT1
   INT16U     tmpPn=0;
   INT8U      da1, da2;
   INT16U     offset0b;                  //���յ���֡�е����ݵ�Ԫƫ����(�������ݱ�ʶ���ֽ�)
   INT16U     tmpLoadLen;
   INT8U      density;                   //�ܶ�
   INT8U      densityInterval;           //�ܶȵ�ʱ����(15,30,60����)
	 INT8U      i, rate, interval, sample;
	 INT16U     tail, dataDone, byteOfTime;
	 INT8U      pseudoFrame[64];           //��ʱ���������α����
	 DATE_TIME  tmpTime;
   INT8U      ackTail;
   	  
	 for (i = 0; i < 100; i++)
	 {
	   ackData[i] = 0;
	 }
	 
	 maxCycle = 0;
	 ackTail  = 0;
	 tmpLoadLen = frame.loadLen;
	 while((tmpLoadLen > 0) && (maxCycle<500))
	 {
     maxCycle++;

     tmpDt1 = *(pDataHead + 2);
     tmpDtCount = 0;
     offset0b = 0;
     while(tmpDtCount < 9)
     {
       tmpDtCount++;
       if ((tmpDt1 & 0x1) == 0x1)
       {
         fn = *(pDataHead + 3) * 8 + tmpDtCount;
        
         if (fn!=1 && fn!=2)
         {
        	 return;
         }
         
         tmpPn = 0;
         da1   = *pDataHead;
         da2   = *(pDataHead+1);
         while(tmpPn < 9)
         {
  	       tmpPn++;
  	       if((da1 & 0x01) == 0x01)
  	       {
  		       pn = tmpPn + (da2 - 1) * 8;

        	   if (pn>64)
        	   {
        	   	  return;
        	   }
        	   
        	   printf("AFN0B:loadLen=%d,fn=%d,pn=%d\n",frame.loadLen, fn, pn);        	   
        	   
             tmpTime.day = 0;   //��tmpTime.day��Ϊ�Ƿ��ѯ���������õı�־
            
        	   //��Ӧ��ѯ��ʱ����1����������
        	   if (fn == 1)
        	   {
        	      //���Ҷ�Ӧ����������
        	      for (i = 0; i < reportTask1.numOfTask; i++)
                {
                	//��ȡ��һ�η���ʱ��
                	if (reportTime1.taskConfig[i].taskNum == pn)
                	{
                	   tmpTime = reportTime1.taskConfig[i].nextReportTime;
                  }
              	  
              	  //��ȡ��������
                	if (reportTask1.task[i].taskNum == pn)
              	  {
              	     interval = reportTask1.task[i].sendPeriod & 0x3f;
                     rate = reportTask1.task[i].sendPeriod>>6 & 0x03;
                     break;
              	  }
                }
                
                //���ڶ�Ӧ���������ã��������ݳ�֡
                if (tmpTime.day != 0)
                {
                   //������дα���������
              		 for (tail = 0; tail < reportTask1.task[i].numOfDataId; tail++)
              		 {
              	 	    //��ԭfn, pn
                    	pseudoFrame[tail*4] = reportTask1.task[i].dataUnit[tail].pn1;
                      pseudoFrame[tail*4+1] = reportTask1.task[i].dataUnit[tail].pn2;
                           
                      pseudoFrame[tail*4+2] = 0x01<<((reportTask1.task[i].dataUnit[tail].fn%8 == 0) ? 7 : (reportTask1.task[i].dataUnit[tail].fn%8-1));
                      pseudoFrame[tail*4+3] = (reportTask1.task[i].dataUnit[tail].fn - 1) / 8;
              	   }
              	 	
              	 	 //����
              	   frame.loadLen = tail*4;
              	 	 AFN0C(pseudoFrame, pseudoFrame+tail*4, dataFrom, QUERY);
                }
                else   //û�ж�Ӧ���������ã���������Ч����֡
                {
                   ackData[ackTail*5]   = *pDataHead;        //DA1
                   ackData[ackTail*5+1] = *(pDataHead+1);  //DA2
                   ackData[ackTail*5+2] = *(pDataHead+2);  //DT1
                   ackData[ackTail*5+3] = *(pDataHead+3);  //DT1
                   ackData[ackTail*5+4] = 0x01;            //����
                   ackTail++;
                }
             }
            
             //��Ӧ��ѯ��ʱ����2����������
        	   if (fn == 2)
        	   {
        	      //���Ҷ�Ӧ����������
        	      for (i = 0; i < reportTask2.numOfTask; i++)
                {
                  //��ȡ��������
                	if (reportTask2.task[i].taskNum == pn)
              	  {
              	     //�������ݵ���ʼʱ������վָ��
              	     tmpTime.second = 0;
              	     tmpTime.minute = *(pDataHead+4);
              	     tmpTime.hour = *(pDataHead+5);
              	     tmpTime.day = *(pDataHead+6);
              	     tmpTime.month = *(pDataHead+7);
              	     tmpTime.year = *(pDataHead+8);
              	     
              	     interval = reportTask2.task[i].sendPeriod & 0x3f;
                     rate = reportTask2.task[i].sendPeriod>>6 & 0x03;
                     sample = reportTask2.task[i].sampleTimes;                     
                     break;
              	  }
                }
                
                //���ڶ�Ӧ���������ã��������ݳ�֡
                if (tmpTime.day > 0)
                {
                    //������дα���������
                		tail = 0;
                		
                		//dataDone����α֡���Ѿ���������ݵ�Ԫ��ʾ����          		
                		for (dataDone = 1; dataDone <= reportTask2.task[i].numOfDataId; dataDone++)
                		{
          	 	        pseudoFrame[tail++] = reportTask2.task[i].dataUnit[dataDone-1].pn1;
                      pseudoFrame[tail++] = reportTask2.task[i].dataUnit[dataDone-1].pn2;
                
                      pseudoFrame[tail++] = 0x01<<((reportTask2.task[i].dataUnit[dataDone-1].fn%8 == 0) ? 7 : (reportTask2.task[i].dataUnit[dataDone-1].fn%8-1));
                      pseudoFrame[tail++] = (reportTask2.task[i].dataUnit[dataDone-1].fn - 1) / 8;

                      //��дʱ��
                      if ((reportTask2.task[i].dataUnit[dataDone-1].fn >= 1 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 12)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 25 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 32)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 41 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 43)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 49 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 50)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 57 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 59)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 113 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 118)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 121 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 123)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 151 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 156)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 161 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 176)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 185 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 192)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn == 45) || (reportTask2.task[i].dataUnit[dataDone-1].fn == 53)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn == 129) || (reportTask2.task[i].dataUnit[dataDone-1].fn == 209))
                      {
                      	 byteOfTime = 3;        //�ն��ᡢ�����ն���
                      }
                      
                      if ((reportTask2.task[i].dataUnit[dataDone-1].fn >= 17 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 24)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 33 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 39)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn == 44)|| (reportTask2.task[i].dataUnit[dataDone-1].fn == 46)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 51 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 52)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn == 54)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 60 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 62)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 65 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 66)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn == 130)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 177 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 184)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 193 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 196)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 201 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 208)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 213 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 216))
                      {
                      	 byteOfTime = 2;        //�¶���
                      }
                      
                      if ((reportTask2.task[i].dataUnit[dataDone-1].fn >= 73 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 76)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn >= 81 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 95)
                          ||(reportTask2.task[i].dataUnit[dataDone-1].fn >= 97 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 110)
                          || (reportTask2.task[i].dataUnit[dataDone-1].fn == 138)
                          ||(reportTask2.task[i].dataUnit[dataDone-1].fn >= 145 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 148)
                          ||(reportTask2.task[i].dataUnit[dataDone-1].fn >= 157 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 160)
                          ||(reportTask2.task[i].dataUnit[dataDone-1].fn >= 217 && reportTask2.task[i].dataUnit[dataDone-1].fn <= 218))
                      {
                      	 byteOfTime = 7;       //����
                      }
                      
                      switch (byteOfTime)
                      {
                      	case 3:    //�ն��ᡢ�����ն���
                      		pseudoFrame[tail++] = tmpTime.day;
                      		pseudoFrame[tail++] = tmpTime.month;
                      		pseudoFrame[tail++] = tmpTime.year;
                      		break;
                      		
                      	case 2:   //�¶���
                      		pseudoFrame[tail++] = tmpTime.month;
                      		pseudoFrame[tail++] = tmpTime.year;
                      		break;
                      		
                      	case 7:   //����
                      		pseudoFrame[tail++] = tmpTime.minute;
                      		pseudoFrame[tail++] = tmpTime.hour;
                      		pseudoFrame[tail++] = tmpTime.day;
                      		pseudoFrame[tail++] = tmpTime.month;
                      		pseudoFrame[tail++] = tmpTime.year;
                      		
                		      switch(sample)
                		      {
                			      case 1:
                			 	      density = 1;
                			 	      densityInterval = 15;
                			 	      break;

                			      case 2:
                			 	      density = 2;
                			 	      densityInterval = 30;
                			 	      break;
                			 
                			      default:
                			 	      density = 3;
                			 	      densityInterval = 60;
                			 	      break;
                		      }
                		      
                		      //�����ܶ�
                		      pseudoFrame[tail++] = density;
                      		
                      		//���ݵ��������Է���Ϊ��λ�ķ��ͼ�����Զ����ܶ�
                      		switch (rate)
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
                      		return;
                      }
                	 	}
                	 	
                	 	//����
                  	frame.loadLen = tail;
                  	AFN0D(pseudoFrame, pseudoFrame+tail, dataFrom, QUERY);
                }
                else  //�޴�����ŵ���������
                {
                  ackData[ackTail*5]   = *pDataHead;        //DA1
                  ackData[ackTail*5+1] = *(pDataHead+1);  //DA2
                  ackData[ackTail*5+2] = *(pDataHead+2);  //DT1
                  ackData[ackTail*5+3] = *(pDataHead+3);  //DT1
                  ackData[ackTail*5+4] = 0x01;            //����
                  
                  ackTail++;
                }
                
                offset0b += 5;
             }
           }
           da1 >>= 1;
         }
       }
       tmpDt1 >>= 1;
     }
     
     pDataHead += (offset0b + 4);
     if (tmpLoadLen < (offset0b+4))
     {
        break;
     }
     else
     {
        tmpLoadLen -= (offset0b + 4);
     }
   }
   
   if (ackTail !=0)
   {
     AFN00003(ackTail, dataFrom, 0x0b);
   }
}