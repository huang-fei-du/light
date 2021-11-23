/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN00.c
���ߣ�leiyong
�汾��0.9
������ڣ�2006��7��
��������վAFN00(ȷ��/����)�����ļ���
�����б�
     1.
�޸���ʷ��
  01,06-07-28,Leiyong created.
  02,09-03-10,Leiyong����,�������ӳɹ�����ͳ��
***************************************************/

#include "teRunPara.h"
#include "msSetPara.h"
#include "wlModem.h"
#include "lcdGui.h"
#include "userInterface.h"
#include "statistics.h"

#include "AFN00.h"

#ifdef LIGHTING
 INT8U downLux[6]={0, 0, 0, 0, 0, 0};    //�������ϴ��ĵ�ǰ����ֵ,ǰ3���ֽڴ洢��һ�ηַ���ֵ,��3���ֽڴ�����ϴηַ���ֵ
 INT8U rcvLuxTimes=0;                    //���յ����ն�����ֵ�Ĵ���
#endif

/*******************************************************
��������:AFN00
��������:����ȷ��/����(AFN00)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN00(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   INT8U     fn, pn;
   INT16U    tmpData;   

   if (debugInfo&WIRELESS_DEBUG)
   {
     sendDebugFrame((INT8U *)pDataHead,10);
   }
    
   //�������ݵ�Ԫ��ʶ��ֵ,����FN��Pnֵ
   pn = findFnPn(*pDataHead, *(pDataHead+1),FIND_PN);
   fn = findFnPn(*(pDataHead+2), *(pDataHead+3),FIND_FN);
   
   if (pn == 0)
   { 
      switch(fn)
      {
     	   case 1:      //ȫ��ȷ��
           if (wlModemFlag.loginSuccess==0)    //��¼ȷ��
           {
	          #ifdef LIGHTING
	      
	           //2016-07-04,����������������Ϊ����Ϊ��λ
	           nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
	      	
	          #else

        	    nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
        	    
        	  #endif
        	  
        	    lastHeartBeat = sysTime;

              wlModemFlag.loginSuccess    = 1;    //��¼�ɹ�
              wlModemFlag.lastIpSuccess   = 1;    //�ϴ�IP��¼��Ϊδ�ɹ�
              wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add
        	    
              #ifdef PLUG_IN_CARRIER_MODULE
               #ifdef MENU_FOR_CQ_CANON
                switch(moduleType)
                {
                  case GPRS_SIM300C:
                  case GPRS_GR64:
                  case GPRS_MC52I:
                  case GPRS_M590E:
                  case GPRS_M72D:
        	          guiDisplay(6, 1, "G", 1);
                    break;
									
                  case LTE_AIR720H:
        	          guiDisplay(6, 1, "4", 1);
                    break;
              
                  case ETHERNET:
        	          guiDisplay(6, 1, "E", 1);
                    break;
                      
                  case CDMA_DTGS800:
                  case CDMA_CM180:
        	          guiDisplay(6, 1, "C", 1);
                    break;
                }

                lcdRefresh(1,16);
               #else
                 showInfo("��¼�ɹ�!");
               #endif
              #else    //��G����Ŀ�
                guiLine(20,1,20,15,1);
                guiLine(30,1,30,15,1);
                guiLine(20,1,30,1,1);
                guiLine(20,15,30,15,1);
                lcdRefresh(1,16);
              #endif
             
              //��¼�����ӳɹ�����
              frameReport(2, 0, 1);
              
              if (debugInfo&WIRELESS_DEBUG)
              {
              	 printf("ȫ��ȷ�ϵĵ�¼�ɹ�\n");
              }
           }
           else              //����ȷ��
           {
         	    if (wlModemFlag.heartSuccess==0)
         	    {
    	         #ifdef LIGHTING
    	      
	              //2016-07-04,����������������Ϊ����Ϊ��λ
	              nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
    	      	
    	         #else
         	      
         	      nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
    	         
    	         #endif
         	      
         	      lastHeartBeat = sysTime;
                wlModemFlag.heartSuccess = 1;
                
                wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add

               #ifdef PLUG_IN_CARRIER_MODULE
                #ifndef MENU_FOR_CQ_CANON
                 showInfo("��վȷ������!");
                #endif
               #endif
              
                //��¼�������ӳɹ�����
             	  frameReport(2, 0, 2);
                
                if (debugInfo&WIRELESS_DEBUG)
                {
              	  printf("ȫ��ȷ�ϵ������ɹ�\n");
                }
             	}
             	else   //�����ϱ�֡��ȷ��
             	{
             		//������֡Ҫ����ʱ��ȷ�Ͼ��ǿ��Է�һ��֡�����ϱ�֡
             		if (fQueue.activeSendPtr!=fQueue.activeTailPtr || fQueue.activeFrameSend==TRUE)
             		{
                  fQueue.activeSendPtr = fQueue.activeFrame[fQueue.activeSendPtr].next;
                  if (debugInfo&PRINT_ACTIVE_REPORT)
                  {
                    printf("�����ϱ�֡����ָ����λ��activeSendPtr=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeTailPtr);

             		    printf("�����ϱ��յ�ȷ��,���Լ�������\n");
  	 	  	          
  	 	  	          #ifdef RECORD_LOG
  	 	  	           logRun("�����ϱ��յ�ȷ��,���Լ�������");
  	 	  	          #endif
             		  }
             		  
             		  fQueue.continueActiveSend = 8;    //��վȷ��
             		}
             		
             		if (fQueue.active0dDataSending>0)
  	            {
  	            	 fQueue.active0dDataSending = 88;
                   
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
             		     printf("0D�����ϱ��յ�ȷ��,���Լ�������\n");
             		   }
  	 	  	         
  	 	  	         #ifdef RECORD_LOG
  	 	  	           logRun("0D�����ϱ��յ�ȷ��,���Լ�������");
  	 	  	         #endif
  	            }
                
               #ifdef PLUG_IN_CARRIER_MODULE
                #ifndef MENU_FOR_CQ_CANON
                 showInfo("ͨ�����!");
                #endif
               #endif
             	}
           }
            
           if (menuInLayer==0 && setParaWaitTime==0xfe)
           {
              defaultMenu();
           }
     	   	 break;
     	   	 
     	   case 2:      //ȫ������
     	   	  break;
     	   	  
     	   case 3:      //���յ������е�ȫ�����ݵ�Ԫ��ʶ�������ȷ��/����
     	   	  pDataHead += 4;
     	   	  switch(*pDataHead)
     	   	  {
     	   	  	  case 0x2:    //ȷ�ϻ����AFN02(��·�ӿڼ��)
                  pDataHead++;
                  
                  //�������ݵ�Ԫ��ʶ��ֵ,����FN��Pnֵ
                  pn = findFnPn(*pDataHead, *(pDataHead+1),FIND_PN);
                  fn = findFnPn(*(pDataHead+2), *(pDataHead+3),FIND_FN);

                  if (fn == 0x1 && pn == 0x0)
                  {
    	               pDataHead += 4;
    	               if (*pDataHead==0x0)
    	               {
            	         #ifdef LIGHTING
            	      
	                      //2016-07-04,����������������Ϊ����Ϊ��λ
	                      nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
            	      	
            	         #else
            	         
                  	    nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);

                  	   #endif
                  	   
                  	    lastHeartBeat = sysTime;

    	                  wlModemFlag.loginSuccess    = 1;    //��¼�ɹ�
                 	  	  wlModemFlag.lastIpSuccess   = 1;    //�ϴ�IP��¼��Ϊδ�ɹ�
                        wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add
        	             
                        #ifdef MENU_FOR_CQ_CANON
                         switch(moduleType)
                         {
                            case GPRS_SIM300C:
                            case GPRS_GR64:
                            case GPRS_MC52I:
                            case GPRS_M590E:
                            case GPRS_M72D:
                  	          guiDisplay(6, 1, "G", 1);
                              break;
                        
                            case ETHERNET:
                  	          guiDisplay(6, 1, "E", 1);
                              break;
                                
                            case CDMA_DTGS800:
                            case CDMA_CM180:
                  	          guiDisplay(6, 1, "C", 1);
                              break;
                         }

                         lcdRefresh(1,16);
                        #else
                         #ifdef PLUG_IN_CARRIER_MODULE
                          #ifndef MENU_FOR_CQ_CANON
                           showInfo("��¼�ɹ�!");
                          #endif
                         #else    //��G����Ŀ�
                          guiLine(20,1,20,15,1);
                          guiLine(30,1,30,15,1);
                          guiLine(20,1,30,1,1);
                          guiLine(20,15,30,15,1);
                          lcdRefresh(1,16);
                         #endif
                        #endif
                        
                        //��¼�����ӳɹ�����
                        frameReport(2, 0, 1);
                        
                        if (debugInfo&WIRELESS_DEBUG)
                        {
              	          printf("����ȷ�Ϸ��ϵĵ�¼�ɹ�\n");
                        }
                     }
                  }
                  if (fn == 0x2 && pn == 0x0)
                  {
   	                 pDataHead += 4;
   	                 if (*pDataHead==0x0)
   	                 {
                	     wlModemFlag.logoutSuccess = 1;    //�˳���¼�ɹ�
                     }
                  }
                  if (fn == 0x3 && pn == 0x0)
                  {
    	               pDataHead += 4;
    	               if (
    	               	   *pDataHead==0x0 
    	               	  #ifdef LIGHTING 
    	               	   || *pDataHead==0x03
    	               	  #endif
    	               	  )
    	               {
            	         #ifdef LIGHTING
            	      
	                      //2016-07-04,����������������Ϊ����Ϊ��λ
            	          nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
            	      	
            	         #else
                  	   
                  	    nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
                  	   
                  	   #endif
                  	   
                  	    lastHeartBeat = sysTime;
                        wlModemFlag.heartSuccess  = 1;
                        wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add

                        //��¼�������ӳɹ�����
                        frameReport(2, 0, 2);
                        
                         #ifdef PLUG_IN_CARRIER_MODULE
                          #ifndef MENU_FOR_CQ_CANON
                           showInfo("��վȷ������!");
                          #endif
                         #endif
                        
                        if (debugInfo&WIRELESS_DEBUG)
                        {
                          printf("%02d-%02d-%02d %02d:%02d:%02d,����ȷ�Ϸ��ϵ������ɹ�.�´�����ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",
                                 sysTime.year,
                                 sysTime.month,
                                 sysTime.day,
                                 sysTime.hour,
                                 sysTime.minute,
                                 sysTime.second,
                                 nextHeartBeatTime.year,
                                 nextHeartBeatTime.month,
                                 nextHeartBeatTime.day,
                                 nextHeartBeatTime.hour,
                                 nextHeartBeatTime.minute,
                                 nextHeartBeatTime.second
                                );
                        }
                        
                       #ifdef LIGHTING
                        //��ش���,2015-12-16
                        if (*pDataHead==0x3)
                        {
                          rcvLuxTimes++;
                          
                          if (debugInfo&METER_DEBUG)
                          {
                            printf("%02d-%02d-%02d %02d:%02d:%02d,�������ַ��ĵ�ǰ����ֵ=%ld,��һ�ηַ�ֵ=%ld,���ϴηַ�ֵ=%ld\n",
                                   sysTime.year,
                                   sysTime.month,
                                   sysTime.day,
                                   sysTime.hour,
                                   sysTime.minute,
                                   sysTime.second,
                                   *(pDataHead+1)|*(pDataHead+2)<<8 | *(pDataHead+3)<<16,
                                   downLux[0] | downLux[1]<<8 | downLux[2]<<16,
                                   downLux[3] | downLux[4]<<8 | downLux[5]<<16
                                  );
                          }

                          if (rcvLuxTimes>2)
                          {
                            lcProcess(downLux, *(pDataHead+1)|*(pDataHead+2)<<8 | *(pDataHead+3)<<16);
                          }

                          //����һ�ηַ���ֵ�ƶ������ϴ�
                          downLux[3] = downLux[0];
                          downLux[4] = downLux[1];
                          downLux[5] = downLux[2];
                          
                          //��ǰ�ַ�ֵ�洢����һ��λ��
                          downLux[0] = *(pDataHead+1);
                          downLux[1] = *(pDataHead+2);
                          downLux[2] = *(pDataHead+3);
													
													lcHeartBeat = 0;
                        }
                       #endif
                     }
                  }
            
                  if (menuInLayer==0 && setParaWaitTime==0xfe)
                  {
                    defaultMenu();
                  }                  
     	   	  	  	break;
     	   	  }
     	   	  break;
      }
   }
}

/*******************************************************
��������:ackOrNack
��������:����ȫ��ȷ�ϻ������վ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void ackOrNack(BOOL ack,INT8U dataFrom)
{
  INT16U i;
  INT8U  checkSum;
  INT16U frameTail00,tmpHead00;

  if (fQueue.tailPtr == 0)
  {
     tmpHead00 = 0;
  }
  else
  {
     tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
  }

  msFrame[tmpHead00+0] = 0x68;
  
  msFrame[tmpHead00+5] = 0x68;
  
  msFrame[tmpHead00+6] = 0x80;          //C:10000000
  
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
  {
     msFrame[tmpHead00+6] |= 0x20;
  }
    
  msFrame[tmpHead00+7] = addrField.a1[0];
  msFrame[tmpHead00+8] = addrField.a1[1];
  msFrame[tmpHead00+9] = addrField.a2[0];
  msFrame[tmpHead00+10] = addrField.a2[1];
  msFrame[tmpHead00+11] = addrField.a3;
  
  msFrame[tmpHead00+12] = 0;

  if (frame.pTp!=NULL)
  {
    msFrame[tmpHead00+13] = 0xe0 | rSeq;
  }
  else
  {
    msFrame[tmpHead00+13] = 0x60 | rSeq;
  }
  
  msFrame[tmpHead00+14] = 0;
  msFrame[tmpHead00+15] = 0;
  
  if (ack == TRUE)
  {
    msFrame[tmpHead00+16] = 0x1;  //ȷ��
  }
  else
  {
    msFrame[tmpHead00+16] = 0x2;  //����
  }
  
  msFrame[tmpHead00+17] = 0;
  
  frameTail00 = tmpHead00 + 18;
  
  //�����������ϱ������¼�����
  if (frame.acd==1 && (callAndReport&0x03)== 0x02)
  {
    msFrame[frameTail00++] = iEventCounter;
    msFrame[frameTail00++] = nEventCounter;
    
    //��������վҪ���ж��Ƿ�Я��TP
    if (frame.pTp != NULL)
    {
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp;
    }
  }
  else
  {
    //��������վҪ���ж��Ƿ�Я��TP
    if (frame.pTp != NULL)
    {
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp++;
       msFrame[frameTail00++] = *frame.pTp;
    }
  }

  i = ((frameTail00 - tmpHead00 - 6) << 2) | 0x2;
  msFrame[tmpHead00+1] = i & 0xFF;   //L
  msFrame[tmpHead00+2] = i >> 8;
  msFrame[tmpHead00+3] = i & 0xFF;   //L
  msFrame[tmpHead00+4] = i >> 8; 
    
  i = tmpHead00+6;
  checkSum = 0;
  while (i < frameTail00)
  {
     checkSum = checkSum + msFrame[i];
     i++;
  }
  msFrame[frameTail00++] = checkSum;
  
  msFrame[frameTail00++] = 0x16;
  
  fQueue.frame[fQueue.tailPtr].head = tmpHead00;
  fQueue.frame[fQueue.tailPtr].len = frameTail00-tmpHead00;
  
  if ((frameTail00+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
  	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
  {
     fQueue.frame[fQueue.tailPtr].next = 0x0;
  	 fQueue.tailPtr = 0;
  }
  else
  {                 
     fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
     fQueue.tailPtr++;
  }
}

/*******************************************************
��������:AFN0003
��������:���յ������е�ȫ�����ݵ�Ԫ��ʶ�������ȷ��/����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN00003(INT8U ackNum, INT8U dataFrom, INT8U afn)
{
   INT16U i, frameTail00;
   INT16U tmpHead00;
   INT8U  *pTpv;                   //TpVָ��
   
   if (fQueue.tailPtr == 0)
   {
      tmpHead00 = 0;
   }
   else
   {
      tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }

   msFrame[tmpHead00+ 0] = 0x68;
     
   msFrame[tmpHead00 + 5] = 0x68;

   msFrame[tmpHead00 + 6] = 0x89;                      //C:10001001
   if (frame.acd==1 && (callAndReport&0x03)== 0x02)  //�����������ϱ������¼�����
   {
      msFrame[tmpHead00 + 6] |= 0x20;
   }   
   msFrame[tmpHead00 + 7] = addrField.a1[0];
   msFrame[tmpHead00 + 8] = addrField.a1[1];
   msFrame[tmpHead00 + 9] = addrField.a2[0];
   msFrame[tmpHead00 + 10] = addrField.a2[1];
   msFrame[tmpHead00 + 11] = addrField.a3;
   
   msFrame[tmpHead00 + 12] = 0;
   msFrame[tmpHead00 + 13] = 0;
   
   if (frame.pTp!=NULL)
   {
   	 msFrame[tmpHead00 + 13] |= 0x80;
   }
     
   msFrame[tmpHead00 + 14] = 0;
   msFrame[tmpHead00 + 15] = 0;
   msFrame[tmpHead00 + 16] = 0x04;
   msFrame[tmpHead00 + 17] = 0x0;
   
   msFrame[tmpHead00 + 18] = afn;
   
   frameTail00 = tmpHead00 + 19;
   
   for (i = 0; i < ackNum; i++)
   {
      msFrame[frameTail00++] = ackData[i*5];
      msFrame[frameTail00++] = ackData[i*5+1];
      msFrame[frameTail00++] = ackData[i*5+2];
      msFrame[frameTail00++] = ackData[i*5+3];
      msFrame[frameTail00++] = ackData[i*5+4];
   }

   if (frame.acd==1 && (callAndReport&0x03)== 0x02)
   {
      msFrame[frameTail00++] = iEventCounter;
      msFrame[frameTail00++] = nEventCounter;
   }
   
   //��������վҪ���ж��Ƿ�Я��TP
   if (frame.pTp != NULL)
   {
      pTpv = frame.pTp;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv;
   }
   
   i = ((frameTail00 - tmpHead00 -6) << 2) | 0x2;
   msFrame[tmpHead00 + 1] = i & 0xFF;   //L
   msFrame[tmpHead00 + 2] = i >> 8;
   msFrame[tmpHead00 + 3] = i & 0xFF;   //L
   msFrame[tmpHead00 + 4] = i >> 8; 
     
   frameTail00++;   
   msFrame[frameTail00++] = 0x16;
 
   fQueue.frame[fQueue.tailPtr].head = tmpHead00;
   fQueue.frame[fQueue.tailPtr].len = frameTail00 - tmpHead00;
   
   if ((frameTail00+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
  	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
   {
     fQueue.frame[fQueue.tailPtr].next = 0x0;
  	 fQueue.tailPtr = 0;
   }
   else
   {                 
     fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
     fQueue.tailPtr++;
   }
}

/*******************************************************
��������:AFN0004
��������:���յ������е�Ӳ����ȫ��֤����Ӧ��
���ú���:
�����ú���:
�������:errorType-��������(1-��ʾǩ��У�����,2-��ʾ����У�����,3-��ʾ�Գ�MAC��֤ʧ��)
         data-ǩ������ʱ,�����巵��ȫFF,��ʾ��Ч
              ����У�����ʱ,�����巵��ȫFF,��ʾ��Ч
              �Գ�MAC��֤ʧ��ʱ:��8�ֽ�Ϊ��ǰ�ն������,��8�ֽ�ģ�����к�
�������:
����ֵ��void
*******************************************************/
void AFN00004(INT8U dataFrom, INT8U errorType, INT8U *data)
{
   INT16U i, frameTail00;
   INT16U tmpHead00;
   INT8U  *pTpv;                   //TpVָ��
   
   if (fQueue.tailPtr == 0)
   {
      tmpHead00 = 0;
   }
   else
   {
      tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }

   msFrame[tmpHead00+ 0] = 0x68;
     
   msFrame[tmpHead00 + 5] = 0x68;

   msFrame[tmpHead00 + 6] = 0x89;                      //C:10001001
   if (frame.acd==1 && (callAndReport&0x03)== 0x02)  //�����������ϱ������¼�����
   {
      msFrame[tmpHead00 + 6] |= 0x20;
   }   
   msFrame[tmpHead00 + 7] = addrField.a1[0];
   msFrame[tmpHead00 + 8] = addrField.a1[1];
   msFrame[tmpHead00 + 9] = addrField.a2[0];
   msFrame[tmpHead00 + 10] = addrField.a2[1];
   msFrame[tmpHead00 + 11] = addrField.a3;
   
   msFrame[tmpHead00 + 12] = 0;
   msFrame[tmpHead00 + 13] = 0;
   
   if (frame.pTp!=NULL)
   {
   	 msFrame[tmpHead00 + 13] |= 0x80;
   }
     
   msFrame[tmpHead00 + 14] = 0;
   msFrame[tmpHead00 + 15] = 0;
   msFrame[tmpHead00 + 16] = 0x08;  //FN=4
   msFrame[tmpHead00 + 17] = 0x0;
   
   msFrame[tmpHead00 + 18] = errorType;

   frameTail00 = tmpHead00 + 19;
   if (errorType==3)
   {
   	 memcpy(&msFrame[frameTail00], data, 16);
   }
   else
   {
   	 memset(&msFrame[frameTail00],0xff,16);
   }
   frameTail00+=16;   	 

   if (frame.acd==1 && (callAndReport&0x03)== 0x02)
   {
      msFrame[frameTail00++] = iEventCounter;
      msFrame[frameTail00++] = nEventCounter;
   }
   
   //��������վҪ���ж��Ƿ�Я��TP
   if (frame.pTp != NULL)
   {
      pTpv = frame.pTp;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv++;
      msFrame[frameTail00++] = *pTpv;
   }
   
   i = ((frameTail00 - tmpHead00 -6) << 2) | 0x2;
   msFrame[tmpHead00 + 1] = i & 0xFF;   //L
   msFrame[tmpHead00 + 2] = i >> 8;
   msFrame[tmpHead00 + 3] = i & 0xFF;   //L
   msFrame[tmpHead00 + 4] = i >> 8; 
     
   frameTail00++;   
   msFrame[frameTail00++] = 0x16;
 
   fQueue.frame[fQueue.tailPtr].head = tmpHead00;
   fQueue.frame[fQueue.tailPtr].len = frameTail00 - tmpHead00;
   
   if ((frameTail00+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
  	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
   {
     fQueue.frame[fQueue.tailPtr].next = 0x0;
  	 fQueue.tailPtr = 0;
   }
   else
   {                 
     fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
     fQueue.tailPtr++;
   }
}
