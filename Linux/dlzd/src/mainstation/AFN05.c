/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN05.c
���ߣ�leiyong
�汾��0.9
������ڣ�2006��6��
��������վAFN05(��������)�����ļ���
�����б�
     1.AFN05(),��վ"��������"(AFN05)������
     2.saveData(),��������
�޸���ʷ��
  01,06-6-26,Leiyong created.
***************************************************/

#include "gdw376-2.h"

#include "convert.h"
#include "workWithMS.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "timeUser.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "reportTask.h"
#include "userInterface.h"
#include "wlModem.h"

#include "AFN05.h"

extern INT8U                statusOfGate;

INT16U offset05;                                   //���յ���֡�е����ݵ�Ԫƫ����(�������ݱ�ʶ���ֽ�)
INT8U  ctrlCmdWaitTime=0;                          //�յ�����������ʾͣ��ʱ��


/*******************************************************
��������:AFN05
��������:��վ"��������"(AFN05)������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN05(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   INT8U     fn, pn;                    //��fn�����ݵ�Ԫ��ʶ+���ݵ�Ԫ����(֡�е�ƫ����)   
   BOOL      (*AFN05Fun[66])(INT8U *p, INT8U n);
   BOOL      ifAck[66];                 //����AFN05Fun[x]ȷ�ϻ����
   INT8U     handleItem[54];            //�Ѵ��������(�洢fnֵ)
   INT8U     handleTail;                //�Ѵ���������������ǵĴ洢λ��β��
   BOOL      ackAll,nAckAll;            //ȫ��ȷ��,���ϱ�־
   INT8U     maxCycle;                  //���ѭ������
   INT8U     i;
   
   for(i=0;i<54;i++)
   {
     AFN05Fun[i] = NULL;
   }
  
  #ifndef LIGHTING 
   AFN05Fun[0] = AFN05001;
   AFN05Fun[1] = AFN05002;
   AFN05Fun[8] = AFN05009;
   AFN05Fun[9] = AFN05010;
   AFN05Fun[10] = AFN05011;
   AFN05Fun[11] = AFN05012;
   AFN05Fun[14] = AFN05015;
   AFN05Fun[15] = AFN05016;
   AFN05Fun[16] = AFN05017;
   AFN05Fun[17] = AFN05018;
   AFN05Fun[18] = AFN05019;
   AFN05Fun[19] = AFN05020;
   AFN05Fun[22] = AFN05023;
   AFN05Fun[23] = AFN05024;
   AFN05Fun[24] = AFN05025;
   AFN05Fun[25] = AFN05026;  
   AFN05Fun[26] = AFN05027;
   AFN05Fun[27] = AFN05028;
  #endif    //LIGHTING
  
   AFN05Fun[28] = AFN05029;
   AFN05Fun[29] = AFN05030;    //�ն�Ͷ������(�����Լ)
   
   AFN05Fun[30] = AFN05031;
   AFN05Fun[31] = AFN05032;
   AFN05Fun[32] = AFN05033;
   AFN05Fun[33] = AFN05034;
   AFN05Fun[34] = AFN05035;
   AFN05Fun[35] = AFN05036;
   AFN05Fun[36] = AFN05037;

   AFN05Fun[39] = AFN05040;    //�ն��˳�����(�����Լ)

   AFN05Fun[48] = AFN05049;
   AFN05Fun[49] = AFN05050;
   AFN05Fun[50] = AFN05051;
   AFN05Fun[51] = AFN05052;
   AFN05Fun[52] = AFN05053;    //ɾ��ָ��ͨ�Ŷ˿��µ�ȫ�����
   AFN05Fun[53] = AFN05054;    //������������(�����Լ)

   AFN05Fun[65] = AFN05066;
   
   //�ж�֡�е�PW��Ϣ(����δ���)

   handleTail = 0;
   ackAll = TRUE;
   nAckAll = TRUE;
   maxCycle = 0;

   while ((pDataHead != pDataEnd) && (maxCycle<10))
   {
      maxCycle++;
      
      pn = findFnPn(*pDataHead,*(pDataHead+1),FIND_PN);
      fn = findFnPn(*(pDataHead+2),*(pDataHead+3),FIND_FN);
      
      if (toEliminate==CTRL_JUMP_IN)
      {
      	 //�޳�Ͷ��ʱ,ֻ��Ӧ��ʱ����(Fn31)���޳����(Fn36)
      	 if (fn==31 || fn==36)
      	 {
      	 }
      	 else
      	 {
      	 	  return;
      	 }
      }
         
      if (fn > 0 && fn <= 66 && AFN05Fun[fn-1] != NULL)
      {
         handleItem[handleTail++] = fn;
         ifAck[fn-1] = AFN05Fun[fn-1](pDataHead+4, pn);
         if (ifAck[fn-1]==TRUE)
         {
          #ifdef PLUG_IN_CARRIER_MODULE
           setParaWaitTime = SET_PARA_WAIT_TIME;
           
           //ly,2011-10-12,add,ֻ�м����淶Ҫ��ĲŸ�����ʾ��
           if  (fn==1 || fn==2 
          	    || (fn>=9 && fn<=12)
          	     || (fn>=15 && fn<=20)
           	      || (fn>=23 && fn<=25)
           	       || fn==28 
           	        || fn==33
           	         || fn==36
          	  )
           {
    	       setBeeper(BEEPER_ON);         //��������ʾ��
    	     }
    	    
 	         alarmLedCtrl(ALARM_LED_ON);          //ָʾ����
 	        #else
           lcdBackLight(LCD_LIGHT_SET_PARA);
       
           ctrlCmdWaitTime = SET_PARA_WAIT_TIME;
           
           //ly,2011-10-12,add,ֻ�м����淶Ҫ��ĲŸ�����ʾ��
           if  (fn==1 || fn==2 
          	    || (fn>=9 && fn<=12)
          	     || (fn>=15 && fn<=20)
           	      || (fn>=23 && fn<=25)
           	       || fn==28 
           	        || fn==33
           	         || fn==36
          	  )
           {
    	       setBeeper(BEEPER_ON);         //��������ʾ��
    	     }

 	         alarmLedCtrl(ALARM_LED_ON);          //ָʾ����
 	        #endif
           
           lcdLightOn = LCD_LIGHT_ON;
           lcdLightDelay = nextTime(sysTime, 0, SET_PARA_WAIT_TIME);
         }
         
         if (ifAck[fn-1]==TRUE)
         {
           nAckAll = FALSE;
         }
         else
         {
           ackAll = FALSE;
         }
         
         if (fn==31)
         {
           //����ȷ���´�����ʱ��
	         if (dataFrom == DATA_FROM_GPRS)
	         {
 	          #ifdef LIGHTING
 	       
	           //2016-07-04,����������������Ϊ����Ϊ��λ
 	           nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
 	      	
 	          #else

	            nextHeartBeatTime = nextTime(sysTime,commPara.heartBeat,0);
	            
	          #endif
	          
             lastHeartBeat = sysTime;
             wlModemFlag.heartSuccess = 1;
           }
         }
      }
      else
      {
      	break;
      }
      	
      pDataHead = pDataHead + offset05 + 4;
   }
   
   //ȷ�ϡ����ϡ����ȷ��/����
   if (ackAll || nAckAll)
   {
     if (ackAll)
     {
        ackOrNack(TRUE,dataFrom);
     }
     if (nAckAll)
     {
        ackOrNack(FALSE,dataFrom);
     }
   }
   else
   {  
      ;//����ȷ�ϻ����(����δ���)
   }
}

#ifndef LIGHTING

/*******************************************************
��������:AFN05001
��������:ң�ر�־��λ
          ����ң�ط�У֡
          ����ң�ط�У��ʱʱ��
          ����ң�ؽ���ʱ��
          
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05001(INT8U *pHandle, INT8U pn)
{ 
   //ң��·���������·�����ڱ���״̬
   if (pn>CONTROL_OUTPUT || pn<1  || staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {
   	 offset05 = 1;
   	 return FALSE;
   }
   
   remoteCtrlConfig[pn-1].ifRemoteCtrl = 0x00;
   remoteCtrlConfig[pn-1].ifRemoteConfirm = 0x00;
   
   //�����޵���ʼʱ��
   if ((*pHandle>>4) == 0)
   {
     remoteCtrlConfig[pn-1].remoteStart = sysTime;
     remoteCtrlConfig[pn-1].alarmStart.year = 0xFF;
   }
   else
   {
     remoteCtrlConfig[pn-1].remoteStart = nextTime(sysTime,*pHandle>>4, 0);
     remoteCtrlConfig[pn-1].alarmStart = sysTime;
   }
   
   remoteCtrlConfig[pn-1].status = 0;
   
   //�����޵����ʱ��
   if ((*pHandle&0x0F) == 0)
   {
     remoteCtrlConfig[pn-1].remoteEnd.year = 0xFF;
   }
   else
   {
      remoteCtrlConfig[pn-1].remoteEnd = nextTime(remoteCtrlConfig[pn-1].remoteStart, (*pHandle&0xF)*30, 0);
   }

   //������Զ���բ,����������Ժ������Ա����,Ҫ�������բ,ly,2010-01-12,add
   if (remoteCtrlConfig[pn-1].remoteEnd.day != sysTime.day)
   {
      remoteCtrlConfig[pn-1].remoteEnd.hour   = 0x0;
      remoteCtrlConfig[pn-1].remoteEnd.minute = 0x0;
      remoteCtrlConfig[pn-1].remoteEnd.second = 0x0;
   }
   
   #ifdef REMOTE_CTRL_CONFIRM
     //����ң�ط�У֡
     
     //���㷵Уʱ��
     remoteCtrlConfig[pn-1].confirmTimeOut = nextTime(sysTime,CONFIRM_TIME_OUT, 0);
   #endif
   
   remoteCtrlConfig[pn-1].ifUseRemoteCtrl = CTRL_JUMP_IN;    //��·ң��Ͷ��(ң����բ),ly,2011-08-08,add
   
   saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);

	 offset05 = 1;
	  
	 return TRUE; 
}

/*******************************************************
��������:AFN05002
��������:�����բ
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
BOOL AFN05002(INT8U *pHandle, INT8U pn)
{
    if (pn>CONTROL_OUTPUT || pn<1 || staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
    {
    	 offset05 = 0;
    	 return FALSE;
    }
      	  
	  //��բ�¼�����Ϊ��ǰʱ��
	  remoteCtrlConfig[pn-1].remoteEnd.second = sysTime.second;
	  remoteCtrlConfig[pn-1].remoteEnd.minute = sysTime.minute;
	  remoteCtrlConfig[pn-1].remoteEnd.hour   = sysTime.hour;
	  remoteCtrlConfig[pn-1].remoteEnd.day    = sysTime.day;
	  remoteCtrlConfig[pn-1].remoteEnd.month  = sysTime.month;
	  remoteCtrlConfig[pn-1].remoteEnd.year   = sysTime.year;
	  
    saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
	  
	  offset05 = 0;
	  return TRUE; 
}

/*******************************************************
��������:AFN05009
��������:ʱ�ι���Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05009(INT8U *pHandle, INT8U pn)
{
   offset05 = 2;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {
   	  return FALSE;
   }
   ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_JUMP_IN;                               //ʱ�ι���
   periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl = 0x0;
   periodCtrlConfig[pn-1].ctrlPara.prdAlarm  = 0x0;    
   periodCtrlConfig[pn-1].ctrlPara.limitPeriod = *pHandle++;                      //ʱ�ι���Ͷ���־
   periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod  = *pHandle;                        //ʱ�ι��ض�ֵ������

	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����  
   saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
   
   statusOfGate |= 0x10;    //���ص���

	 return TRUE;
}

/*******************************************************
��������:AFN05010
��������:���ݿ�Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05010(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
	 
	 ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_JUMP_IN;    //���ݿ�
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����

   statusOfGate |= 0x10;    //���ص���

	 return TRUE;
}

/*******************************************************
��������:AFN05011
��������:��ͣ��Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05011(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
	 
	 ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_JUMP_IN;    //��ͣ��
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����

   statusOfGate |= 0x10;    //���ص���

	 return TRUE;
}

/*******************************************************
��������:AFN05012
��������:��ǰ�����¸���Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05012(INT8U *pHandle, INT8U pn)
{
	 offset05 = 8;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }

	 if (ctrlRunStatus[pn-1].ifUsePwrCtrl!=CTRL_JUMP_IN)
	 {
	    ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_JUMP_IN; //��ǰ�����¸�����
	    powerDownCtrl[pn-1].ifPwrDownCtrl = 0x0;
	    powerDownCtrl[pn-1].pwrDownAlarm = 0x0;
	    
	    powerDownCtrl[pn-1].slipTime = *pHandle++;       //��ǰ�����¸��ض�ֵ����ʱ��
	    powerDownCtrl[pn-1].floatFactor = *pHandle++;    //��ǰ�����¸��ض�ֵ����ϵ��
	    powerDownCtrl[pn-1].freezeDelay = *pHandle++;    //�غ��ܼ��й����ʶ�����ʱʱ��
	    
	    powerDownCtrl[pn-1].downCtrlTime      = *pHandle++;//��ǰ�����¸��صĿ���ʱ��
	    powerDownCtrl[pn-1].roundAlarmTime[0] = *pHandle++;//��ǰ�����¸��ص�1�ָ澯ʱ��
	    powerDownCtrl[pn-1].roundAlarmTime[1] = *pHandle++;//��ǰ�����¸��ص�2�ָ澯ʱ��
	    powerDownCtrl[pn-1].roundAlarmTime[2] = *pHandle++;//��ǰ�����¸��ص�3�ָ澯ʱ��
	    powerDownCtrl[pn-1].roundAlarmTime[3] = *pHandle++;//��ǰ�����¸��ص�4�ָ澯ʱ��
	  
	    //powerDownCtrl[pn-1].freezeTime = nextTime(sysTime,powerDownCtrl[pn-1].freezeDelay,0);
	    
	    powerDownCtrl[pn-1].freezeTime = nextTime(sysTime,2,30);
	    
	    saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����
	    saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //��ǰ�����¸�����
	 }
   
   statusOfGate |= 0x10;    //���ص���

	 return TRUE;
}

/*******************************************************
��������:AFN05015
��������:�µ��Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05015(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
	 ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_JUMP_IN;  //�µ��Ͷ��	
	 
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����

   statusOfGate |= 0x20;    //��ص���

	 return TRUE;
}

/*******************************************************
��������:AFN05016
��������:�����Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05016(INT8U *pHandle, INT8U pn)
{
   INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-26,add
   BOOL      bufHasData;
   DATE_TIME readTime;
	 INT8U     leftPower[12];
   INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];

	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
	 
	 ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_JUMP_IN;    //�����Ͷ��
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����

   readTime = sysTime;
   if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
   {
        //�鿴���ܼ����Ƿ�ֻ�����������
        onlyHasPulsePn = 0;
        for (i = 0; i < totalAddGroup.numberOfzjz; i++)
        {
        	if (totalAddGroup.perZjz[i].zjzNo == pn)
        	{
            onlyHasPulsePn = 0;
        		for(j=0;j<totalAddGroup.perZjz[i].pointNumber;j++)
        		{
        		 	for(k=0;k<pulseConfig.numOfPulse;k++)
        		 	{
        		 		 if (pulseConfig.perPulseConfig[k].pn==(totalAddGroup.perZjz[i].measurePoint[j]+1))
        		 		 {
        		 		 	  onlyHasPulsePn++;
        		 		 }
        		 	}
        		}
        		
        		if (onlyHasPulsePn==totalAddGroup.perZjz[i].pointNumber)
        		{
        			 onlyHasPulsePn = 0xaa;
        		}
        		
        		break;
        	}
        }
        
        readTime = timeHexToBcd(sysTime);
        if (onlyHasPulsePn==0xaa)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN16:�ܼ���%dֻ�����������\n", pn);
        	}
        	
        	bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime);
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN16::�ܼ���%d������������\n", pn);
        	}
        	bufHasData = readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0);
        }        
        
    	  if (bufHasData == TRUE)
        {
      	  if (dataBuff[GP_MONTH_WORK+3]!= 0xFF && dataBuff[GP_MONTH_WORK+3] != 0xEE)
      	  {
         	  leftPower[8]  = dataBuff[GP_MONTH_WORK+3];
            leftPower[9]  = dataBuff[GP_MONTH_WORK+4];
            leftPower[10] = dataBuff[GP_MONTH_WORK+5];
            leftPower[11] = ((dataBuff[GP_MONTH_WORK]&0x01)<<6)
                          | (dataBuff[GP_MONTH_WORK]&0x10)
                          | (dataBuff[GP_MONTH_WORK+6]&0x0f);
          }
          else
          {
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
              printf("AFN05-FN16::�ο��ܼ��й�����������TRUE���ܼ��µ�����Ϊ0xee,�ο���0\n");
            }

         	  leftPower[8]  = 0x0;
            leftPower[9]  = 0x0;
            leftPower[10] = 0x0;
            leftPower[11] = 0x0;
          }
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
            printf("AFN05-FN16::�ο��ܼ��й�����������FALSE,�ο���0\n");
          }
          
          //ly,2011-04-16,�ĳ����������
         	leftPower[8]  = 0x0;
          leftPower[9]  = 0x0;
          leftPower[10] = 0x0;
          leftPower[11] = 0x0;
        }
     	  
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
     	    printf("AFN05-FN16::�ο��ܼ��й�������=%02x%02x%02x%02x\n", leftPower[11], leftPower[10], leftPower[9], leftPower[8]);
     	  }

        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
   }
   
   statusOfGate |= 0x20;    //��ص���

	 return TRUE;
}

/*******************************************************
��������:AFN05017
��������:ʱ�ι��ؽ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05017(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }

	 ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_RELEASE;    //ʱ�ι���
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����
	  
	 return TRUE;
}

/*******************************************************
��������:AFN05018
��������:���ݿؽ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05018(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_RELEASE;    //���ݿ�
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����
	  
	 return TRUE;
}

/*******************************************************
��������:AFN05019
��������:��ͣ�ؽ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05019(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_RELEASE;    //��ͣ�ؽ��
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����
	 	  
	 return TRUE;
}

/*******************************************************
��������:AFN05020
��������:�����¸��ؽ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05020(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_RELEASE;    //�����¸��ؽ��	  

	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����
	  
	 return TRUE;
}

/*******************************************************
��������:AFN05023
��������:�µ�ؽ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05023(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
	 ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_RELEASE;  //�µ�ؽ��
	 monthCtrlConfig[pn-1].monthAlarm = 0x00;          //�ܼ����µ�ظ澯��Ϊδ����澯
	 
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����
	  
	 return TRUE;
}

/*******************************************************
��������:AFN05024
��������:����ؽ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05024(INT8U *pHandle, INT8U pn)
{
   INT8U     i, j, k, onlyHasPulsePn;       //ly,2011-04-26,add
   BOOL      bufHasData;
   DATE_TIME readTime;
	 INT8U     leftPower[12];
   INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];

	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_RELEASE;    //����ؽ��
	  
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����

   /*
   readTime = sysTime;
   if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
   {
        //�鿴���ܼ����Ƿ�ֻ�����������
        onlyHasPulsePn = 0;
        for (i = 0; i < totalAddGroup.numberOfzjz; i++)
        {
        	if (totalAddGroup.perZjz[i].zjzNo == pn)
        	{
            onlyHasPulsePn = 0;
        		for(j=0;j<totalAddGroup.perZjz[i].pointNumber;j++)
        		{
        		 	for(k=0;k<pulseConfig.numOfPulse;k++)
        		 	{
        		 		 if (pulseConfig.perPulseConfig[k].pn==(totalAddGroup.perZjz[i].measurePoint[j]+1))
        		 		 {
        		 		 	  onlyHasPulsePn++;
        		 		 }
        		 	}
        		}
        		
        		if (onlyHasPulsePn==totalAddGroup.perZjz[i].pointNumber)
        		{
        			 onlyHasPulsePn = 0xaa;
        		}
        		
        		break;
        	}
        }
        
        readTime = timeHexToBcd(sysTime);
        if (onlyHasPulsePn==0xaa)
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN24:�ܼ���%dֻ�����������\n", pn);
        	}
        	
        	bufHasData = groupBalance(dataBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime);
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
        	  printf("AFN05-FN24::�ܼ���%d������������\n", pn);
        	}
        	bufHasData = readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0);
        }        
        
    	  if (bufHasData == TRUE)
        {
      	  if (dataBuff[GP_MONTH_WORK+3]!= 0xFF && dataBuff[GP_MONTH_WORK+3] != 0xEE)
      	  {
         	  leftPower[8]  = dataBuff[GP_MONTH_WORK+3];
            leftPower[9]  = dataBuff[GP_MONTH_WORK+4];
            leftPower[10] = dataBuff[GP_MONTH_WORK+5];
            leftPower[11] = ((dataBuff[GP_MONTH_WORK]&0x01)<<6)
                          | (dataBuff[GP_MONTH_WORK]&0x10)
                          | (dataBuff[GP_MONTH_WORK+6]&0x0f);
          }
          else
          {
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
              printf("AFN05-FN24::�ο��ܼ��й�����������TRUE���ܼ��µ�����Ϊ0xee,�ο���0\n");
            }

         	  leftPower[8]  = 0x0;
            leftPower[9]  = 0x0;
            leftPower[10] = 0x0;
            leftPower[11] = 0x0;
          }
        }
        else
        {
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
            printf("AFN05-FN24::�ο��ܼ��й�����������FALSE,�ο���0\n");
          }
          
          //ly,2011-04-16,�ĳ����������
         	leftPower[8]  = 0x0;
          leftPower[9]  = 0x0;
          leftPower[10] = 0x0;
          leftPower[11] = 0x0;
        }
     	  
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
     	    printf("AFN05-FN24::�ο��ܼ��й�������=%02x%02x%02x%02x\n", leftPower[11], leftPower[10], leftPower[9], leftPower[8]);
     	  }

        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
   }
   */

	 return TRUE;
}

/*******************************************************
��������:AFN05025
��������:�ն˱���Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/ 
BOOL AFN05025(INT8U *pHandle, INT8U pn)
{
	 #ifdef LOAD_CTRL_MODULE
	  int i;
	  if (*pHandle > 48)
	  {
	    offset05 = 1;
	    return FALSE;
	  }
	  
	  staySupportStatus.ifStaySupport = CTRL_JUMP_IN;
	  
	  if (*pHandle == 0)
	  {
	     staySupportStatus.endStaySupport = sysTime;
	     staySupportStatus.ifHold = 0xaa;
	  }
	  else
	  {
	     staySupportStatus.endStaySupport = nextTime(sysTime, (*pHandle)*30, 1);
	     staySupportStatus.ifHold = 0x55;
	  }
	  
	  saveParameter(0x05, 25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));  //�ն˱���Ͷ��
	  
	  //���еĸ��ֿ���״̬��Ͷ�����Ϊ���
	  for (i = 0; i < 32; i++)
	  {
      //��բ״̬��Ϊ��բ
      if (i<CONTROL_OUTPUT)
      {
      	  jumpGate(i+1, 0);
          gateStatus(i+1,0);
      }
      
	    //ң��
	    if (i < 8)
	    {
	      remoteCtrlConfig[i].ifRemoteCtrl = 0x00;
	      remoteCtrlConfig[i].status = 0x0;
        remoteCtrlConfig[i].remoteStart.year = 0xFF;
        remoteCtrlConfig[i].remoteEnd.year = 0xFF;
      }
	    
	    //�������Ͷ���,��Ҫ�Զ�������еĿ���,�ڱ�������ָ���������(��������Ͷ�����)�Ļ�,ִ��������һ��
	    //�µ��
	    if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseMthCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //�����
	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseChgCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //��ǰ�����¸���
	    if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUsePwrCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //��ͣ��
	    if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseObsCtrl=CTRL_AUTO_RELEASE;
	    }
	  
	    //���ݿ�
	    if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseWkdCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //ʱ�ο�
	    if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUsePrdCtrl=CTRL_AUTO_RELEASE;
	    }
	    
	    //�������Ͷ���,��Ҫ������еĿ���,�ڱ�������Ҳ���ָ��������ƵĻ�(��������Ͷ�����),ִ��������һ��
	    //�µ��
	    if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN)
	    {
  	     ctrlRunStatus[i].ifUseMthCtrl=CTRL_RELEASE;
	       monthCtrlConfig[i].ifMonthCtrl = 0x0;
	       monthCtrlConfig[i].monthAlarm = 0x0;
	    }
	    
	    //�����
	    if (ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseChgCtrl=CTRL_RELEASE;
	       chargeCtrlConfig[i].ifChargeCtrl = 0x0;
	       chargeCtrlConfig[i].chargeAlarm = 0x0;
	    }
	    
	    //��ǰ�����¸���
	    if (ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN)
	    {
	    	 ctrlRunStatus[i].ifUsePwrCtrl=CTRL_RELEASE;
	       powerDownCtrl[i].ifPwrDownCtrl = 0x0;
	       powerDownCtrl[i].pwrDownAlarm  = 0x0;
	    }
	    
	    //��ͣ��
	    if (ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN)
	    {	    
	       ctrlRunStatus[i].ifUseObsCtrl=CTRL_RELEASE;
	       obsCtrlConfig[i].ifObsCtrl = 0x0;
	       obsCtrlConfig[i].obsAlarm = 0x0;
	    }
	  
	    //���ݿ�
	    if (ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUseWkdCtrl=CTRL_RELEASE;
	       wkdCtrlConfig[i].ifWkdCtrl = 0x0;
	       wkdCtrlConfig[i].wkdAlarm = 0x0;
	    }
	    
	    //ʱ�ο�
	    if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN)
	    {
	       ctrlRunStatus[i].ifUsePrdCtrl=CTRL_RELEASE;
	       periodCtrlConfig[i].ctrlPara.ifPrdCtrl = 0x0;
	       periodCtrlConfig[i].ctrlPara.prdAlarm = 0x0;
	    }
	    
	    electCtrlRoundFlag[i].ifJumped = 0x00;              //�����բ����բ��־ȫ�����
      
      powerCtrlRoundFlag[pn-1].ifJumped = 0x00;           //��������բ��־ȫ�����
	  }

	  //�澯����
	  ctrlStatus.numOfAlarm = 0;
	  alarmInQueue(PAUL_ELECTRICITY,0);
	  
	  //����������Լ��ı��˵Ŀ��Ʋ���һ������
	  //saveParameter(WRITE_BLOCK_2);
	  
	  offset05 = 1;	  
	  return TRUE;
	#else
	  offset05 = 1;	  
	  return FALSE;	  
	#endif
}

/*******************************************************
��������:AFN05026
��������:�߷Ѹ澯Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05026(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   reminderFee = CTRL_JUMP_IN;                //�߷Ѹ澯Ͷ��
	  
	 saveParameter(0x05, 26, &reminderFee, 1);  //�߷Ѹ澯Ͷ��
	  
	 return TRUE;
}

/*******************************************************
��������:AFN05027
��������:��Ӧ��վ��������"�����ն�����վͨ��(FN27)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05027(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport & 0x03;
   callAndReport = 0x04 | tmpData;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
��������:AFN05028
��������:�޳�Ͷ��
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05028(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   toEliminate = CTRL_JUMP_IN;    //�޳�Ͷ��
	  
	 saveParameter(0x05, 28, &toEliminate, 1);  //�޳�Ͷ��

	 return TRUE;
}
#endif    //LIGHTING,about line 221


/*******************************************************
��������:AFN05029
��������:��Ӧ��վ��������"�����ն������ϱ�(FN29)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05029(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport>>2 & 0x03;
   callAndReport = 0x01 | tmpData<<2;
    
   callAndReport &= 0x0D;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
��������:AFN05030
��������:��Ӧ��վ��������"�ն�����Ͷ��(FN30,�����Լ)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05030(INT8U *pHandle, INT8U pn)
{    
   teInRunning = 0x01;

   saveParameter(0x05, 30, &teInRunning, 1);

   if (menuInLayer==0)
   {
   	 defaultMenu();
   }

   offset05 = 0;
   return TRUE;
}

/*******************************************************
��������:AFN05031
��������:��Ӧ��վ��������"��ʱ����(FN31)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05031(INT8U *pHandle, INT8U pn)
{	 
   INT8U weekDay;

   dayPowerOnAmount();           //ly,2011-10-11,add
   
   sysTime.second = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++;
   sysTime.minute = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++;
   sysTime.hour = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++;   
   sysTime.day = ((*pHandle>>4)*10)+(*pHandle & 0xf);
   pHandle++; 
   sysTime.month = ((*pHandle>>4 & 0x1)*10)+(*pHandle & 0xf);
   weekDay = (*pHandle>>5);
   pHandle++;
   sysTime.year = ((*pHandle>>4)*10)+(*pHandle & 0xf);   

   setSystemDateTime(sysTime);

   powerOnStatisTime = sysTime;  //ly,2011-10-11,add

   //ˢ��titleʱ����ʾ
   refreshTitleTime();

   //�������ó���ʱ��
   reSetCopyTime();
   copyCtrl[4].ifRealBalance = 0;   
   
   //��ʼ��������Ϣ����
   if (initReportFlag==0)
   {
	   initReportFlag = 1;
	 }

   offset05 = 6;
   
   //2013-12-05,add
  #ifdef LIGHTING
   carrierFlagSet.broadCast = 1;    //��������������㲥УʱCCB
  #endif

   return TRUE;
}

/*******************************************************
��������:AFN05032
��������:��Ӧ��վ��������"������Ϣ(FN32)"
         ֻ�ܱ���8��������Ϣ,ÿ����Ϣ<=200�ֽ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05032(INT8U *pHandle, INT8U pn)
{
   INT8U i,j,ifFound;
   
   offset05 = *(pHandle+1)+2;
   
  #ifdef LOAD_CTRL_MODULE
   
   ifFound = 0;
   if (chnMessage.numOfMessage==0)
   {
   	  chnMessage.numOfMessage = 1;
   	  i=0;
   }
   else
   {
      for(i=0;i<chnMessage.numOfMessage;i++)
      {
   	    if ((chnMessage.message[i].typeAndNum & 0xf) == (*pHandle&0xf))
   	    {
   	  	   ifFound = 1;
   	   	   break;
   	    }
      }
   
      if (ifFound==0)
      {
   	    chnMessage.numOfMessage++;
   	    if (chnMessage.numOfMessage>8)
   	    {
   	    	 return FALSE;
   	    }
   	  }
   }
   
   chnMessage.message[i].typeAndNum = *pHandle++;
   chnMessage.message[i].len = *pHandle++;
   for(j=0;j<chnMessage.message[i].len;j++)
   {
   	  chnMessage.message[i].chn[j] = *pHandle++;
   }
   chnMessage.message[i].chn[j] = '\0';
   
   chinese(i,0,1);
   
   saveParameter(0x05, 32,(INT8U *)&chnMessage,sizeof(CHN_MESSAGE));
  
   messageTip(1);
   
   return TRUE;
  
  #else
  
   return FALSE;
   
  #endif
}

/*******************************************************
��������:AFN05033
��������:�ն˱�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05033(INT8U *pHandle, INT8U pn)
{
    staySupportStatus.ifStaySupport = CTRL_RELEASE;

	  saveParameter(0x05, 25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));  //�ն˱���Ͷ��/���
	  	  
	  offset05 = 0;
	  return TRUE;
}

/*******************************************************
��������:AFN05034
��������:�߷Ѹ澯���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05034(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   reminderFee = CTRL_RELEASE;    //�߷Ѹ澯���

	 saveParameter(0x05, 26, &reminderFee, 1);  //�߷Ѹ澯Ͷ��/���

	 return TRUE;
}

/*******************************************************
��������:AFN05035
��������:��Ӧ��վ��������"��ֹ�ն�����վͨ��(FN35)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05035(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport & 0x03;   
   callAndReport = 0x08 | tmpData;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
��������:AFN05036
��������:�޳����
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
BOOL AFN05036(INT8U *pHandle, INT8U pn)
{
	 offset05 = 0;

   //����״̬������������������
   if(staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
   {   	  
   	  return FALSE;
   }
   
   toEliminate = CTRL_RELEASE;    //�޳����
	  
	 saveParameter(0x05, 28, &toEliminate, 1);  //�޳�Ͷ��/���
	  
	 return TRUE;
}

/*******************************************************
��������:AFN05037
��������:��Ӧ��վ��������"��ֹ�ն������ϱ�(FN37)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05037(INT8U *pHandle, INT8U pn)
{
   INT8U tmpData;
   tmpData = callAndReport>>2 & 0x03;
   callAndReport = 0x02 | tmpData<<2;

   saveParameter(0x05, 29, &callAndReport, 1);
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
��������:AFN05040
��������:��Ӧ��վ��������"�ն��˳�����(FN40,�����Լ)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05040(INT8U *pHandle, INT8U pn)
{    
   teInRunning = 0x00;

   saveParameter(0x05, 30, &teInRunning, 1);
   
   if (menuInLayer==0)
   {
   	 defaultMenu();
   }
   
   offset05 = 0;
   return TRUE;
}

/*******************************************************
��������:AFN05049
��������:��Ӧ��վ��������"����ָ��ͨ�Ŷ˿���ͣ����"(FN49)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05049(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   if ((*pHandle>0 && *pHandle<5) || *pHandle==31)
   {
   	 if (*pHandle<=4)
   	 {
   	   copyCtrl[*pHandle-1].cmdPause = TRUE;  //485�˿�
   	 }
   	 else
   	 {
   	  #ifdef PLUG_IN_CARRIER_MODULE
   	   if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
   	   {
   	     carrierFlagSet.workStatus = 4;
  	     carrierFlagSet.reStartPause = 2;
         carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 3);

         copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
   	   }
   	   
   	   copyCtrl[4].cmdPause = TRUE;   	 	    //�ز��˿�
   	  #endif
   	 }
   	 
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
��������:AFN05050
��������:��Ӧ��վ��������"����ָ��ͨ�Ŷ˿ڻָ�����"(FN50)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05050(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   
   if ((*pHandle>0 && *pHandle<5) || *pHandle==31)
   {
   	 if (*pHandle<=4)
   	 {
   	    copyCtrl[*pHandle-1].cmdPause = FALSE;  //485�˿�
   	 }
   	 else
   	 {
   	  #ifdef PLUG_IN_CARRIER_MODULE
   	   if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
   	   {
         carrierFlagSet.workStatus = 6;
         carrierFlagSet.reStartPause = 3;

         carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 3);

         copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
   	   }

   	   copyCtrl[4].cmdPause = FALSE;   	 	    //�ز��˿�
   	  #endif
   	 }
   	 
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
��������:AFN05051
��������:��Ӧ��վ��������"����ָ��ͨ�Ŷ˿����³���"(FN51)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05051(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   if ((*pHandle>0 && *pHandle<5) || *pHandle==31)
   {
   	 if (*pHandle<=4)
   	 {
   	    copyCtrl[*pHandle-1].cmdPause = FALSE;  //485�˿�
   	 }
   	 else
   	 {
   	  #ifdef PLUG_IN_CARRIER_MODULE 
   	   if (carrierFlagSet.routeLeadCopy==1 && copyCtrl[4].meterCopying==TRUE)
   	   {
         carrierFlagSet.workStatus = 2;
         carrierFlagSet.reStartPause = 1;

         carrierFlagSet.operateWaitTime = nextTime(sysTime, 0, 3);

         copyCtrl[4].copyTimeOut = nextTime(sysTime, 0, 2);
   	   }
   	   
   	   copyCtrl[4].cmdPause = FALSE;   	 	    //�ز��˿�
   	  #endif
   	 }
   	 
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
��������:AFN05052
��������:��Ӧ��վ��������"��ʼ��ָ��ͨ�Ŷ˿��µ�ȫ���м�·����Ϣ"(FN52)
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05052(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   if (*pHandle==31)
   {
   	 #ifdef PLUG_IN_CARRIER_MODULE
   	  //�ز�ģ���ʼ��
      carrierFlagSet.init = 2;
      gdw3762Framing(INITIALIZE_3762, 2, NULL, NULL);
     
      return TRUE;
     #else
      return FALSE;
     #endif
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
��������:AFN05053
��������:��Ӧ��վ��������"ɾ��ָ��ͨ�Ŷ˿��µ�ȫ�����"(FN53)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05053(INT8U *pHandle, INT8U pn)
{
   char * pSqlStr;
	 
   offset05 = 1;
   
   //if (*pHandle==1 || *pHandle==2 || *pHandle==31)
   //2012-08-23,�ĳɿ���ɾ���˿�1,2,3,4,31�˿ڵ�ȫ�����
  #ifdef PLUG_IN_CARRIER_MODULE 
   if (*pHandle==1 || *pHandle==2 || *pHandle==3 || *pHandle==4 || *pHandle==31)
  #else
   if (*pHandle==1 || *pHandle==2 || *pHandle==3)
  #endif
   {
	   pSqlStr = sqlite3_mprintf("delete from f10_info where acc_port=%d;", *pHandle);
   
     if (sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL)!=SQLITE_OK)
     {
   	   return FALSE;
     }
	   
	   #ifdef PLUG_IN_CARRIER_MODULE
      if (*pHandle==31)
      {
      	carrierFlagSet.msSetAddr = 1;       //��վ���ñ��ַ

        carrierFlagSet.msSetWait = nextTime(sysTime, 0, 3);
        if (debugInfo&PRINT_CARRIER_DEBUG)
        {
          printf("�ȴ�3��ɾ������ͨ��ģ����ַ\n");
        }
      }
	   #endif
	   
     return TRUE;
   }
   else
   {
   	 return FALSE;
   }
}

/*******************************************************
��������:AFN05054
��������:��Ӧ��վ��������"����������������"(FN54)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05054(INT8U *pHandle, INT8U pn)
{   
   offset05 = 0;   

  #ifdef PLUG_IN_CARRIER_MODULE
   //�����ǰ�������ı�
   while(foundMeterHead!=NULL)
   {
      tmpFound = foundMeterHead;
      foundMeterHead = foundMeterHead->next;
      free(tmpFound);
   }
   foundMeterHead = NULL;
   prevFound = foundMeterHead;
   if (carrierModuleType==RL_WIRELESS)
   {
  	 carrierFlagSet.setupNetwork = 1;
  	 
  	 //����(�����10����)
  	 carrierFlagSet.foundStudyTime = nextTime(sysTime, 10, 0);
   }
   else
   {
     if (carrierModuleType==CEPRI_CARRIER)
     {
       carrierFlagSet.foundStudyTime = nextTime(sysTime, 120, 0); //�ѱ�ʱ��,11-07-18,������Ժģ����ѱ�ʱ��Ϊ2Сʱ
     }
     else
     {
       carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0); //�ѱ�ʱ��
     }
        
     carrierFlagSet.searchMeter = 1;         //��ʼ�ѱ��־��1
   }

   if (menuInLayer==0)
   {
   	 defaultMenu();
   }
   
   return TRUE;
  #else
   return FALSE;
  #endif
}

/*******************************************************
��������:AFN05066
��������:��Ӧ��վ��������"��������"(FN66)"
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL AFN05066(INT8U *pHandle, INT8U pn)
{
   offset05 = 1;
   
   switch(*pHandle)
   {
   	 case 0:
   	 	 printf("���볭���йصĵ�����Ϣ\r\n");
       debugInfo |= METER_DEBUG;
       #ifdef PLUG_IN_CARRIER_MODULE
        debugInfo |= PRINT_CARRIER_DEBUG;
       #endif
       break;

   	 case 1:
   	 	 printf("����645֡�йصĵ�����Ϣ\r\n");
       debugInfo |= PRINT_645_FRAME;
       break;

   	 case 2:
   	 	 printf("��������йصĵ�����Ϣ\r\n");
       debugInfo |= PRINT_BALANCE_DEBUG;
       break;

   	 case 3:
   	 	 printf("��������Modem�йصĵ�����Ϣ\r\n");
       debugInfo |= WIRELESS_DEBUG;
       break;

   	 case 4:
   	 	 printf("�����¼��йصĵ�����Ϣ\r\n");
       debugInfo |= PRINT_EVENT_DEBUG;
       break;
     
   	 case 5:
   	 	 printf("�������ݿ��йصĵ�����Ϣ\r\n");
       debugInfo |= PRINT_DATA_BASE;
       break;

   	 case 6:
   	 	 printf("���������ϱ��йصĵ�����Ϣ\r\n");
       debugInfo |= PRINT_ACTIVE_REPORT;
       break;
   }
   
   return TRUE;
}
