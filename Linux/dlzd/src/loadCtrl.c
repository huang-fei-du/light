/***************************************************
Copyright,2008,LongTong co.,LTD,All	Rights Reserved
�ļ�����localCtrl.c
���ߣ�TianYe
�汾��0.9
������ڣ�2008��12��
��������������
�����б�
�޸���ʷ:
  01,08-01-22,Tianye created.
  02,08-12-10,leiyong modify,��д���п��ƹ���(����ʵʱ���ơ����رջ����Ƽ������)
  03,10-04-27,leiyong��ֲ��AT91SAM9260������
***************************************************/
#include "convert.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "copyMeter.h"
#include "workWithMeter.h"
#include "hardwareConfig.h"
#include "userInterface.h"
#include "ioChannel.h"
#include "att7022b.h"
#include "AFN0C.h"

#include "loadCtrl.h"

#ifdef LOAD_CTRL_MODULE

INT8U         ctrlReadBuf[512];
INT8U         gateCloseWaitTime=0;             //�����բ�澯��ʱ��
DATE_TIME     nextRound;                       //��һ�ִεȴ�ʱ��(��������½�����ֵ����,�򲻼�����բ)
INT8U         ctrlCount=0;

/*
static unsigned char auchCRCHi[]={
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,
	0x01,0xc0,0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,
	0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x01,0xc0,0x80,0x41,0x00,0xc1,0x81,0x40,0x01,0xc0,
	0x80,0x41,0x00,0xc1,0x81,0x40,0x00,0xc1,0x81,0x40,
	0x01,0xc0,0x80,0x41,0x01,0xc0,0x80,0x41,0x00,0xc1,
	0x81,0x40,0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,
	0x00,0xc1,0x81,0x40,0x01,0xc0,0x80,0x41,0x01,0xc0,
  0x80,0x41,0x00,0xc1,0x81,0x40
  };

static char auchCRCLo[]={
	0x00,0xc0,0xc1,0x01,0xc3,0x03,0x02,0xc2,0xc6,0x06,
	0x07,0xc7,0x05,0xc5,0xc4,0x04,0xcc,0x0c,0x0d,0xcd,
	0x0f,0xcf,0xce,0x0e,0x0a,0xca,0xcb,0x0b,0xc9,0x09,
	0x08,0xc8,0xd8,0x18,0x19,0xd9,0x1b,0xdb,0xda,0x1a,
	0x1e,0xde,0xdf,0x1f,0xdd,0x1d,0x1c,0xdc,0x14,0xd4,
	0xd5,0x15,0xd7,0x17,0x16,0xd6,0xd2,0x12,0x13,0xd3,
	0x11,0xd1,0xd0,0x10,0xf0,0x30,0x31,0xf1,0x33,0xf3,
	0xf2,0x32,0x36,0xf6,0xf7,0x37,0xf5,0x35,0x34,0xf4,
	0x3c,0xfc,0xfd,0x3d,0xff,0x3f,0x3e,0xfe,0xfa,0x3a,
	0x3b,0xfb,0x39,0xf9,0xf8,0x38,0x28,0xe8,0xe9,0x29,
	0xeb,0x2b,0x2a,0xea,0xee,0x2e,0x2f,0xef,0x2d,0xed,
	0xec,0x2c,0xe4,0x24,0x25,0xe5,0x27,0xe7,0xe6,0x26,
	0x22,0xe2,0xe3,0x23,0xe1,0x21,0x20,0xe0,0xa0,0x60,
	0x61,0xa1,0x63,0xa3,0xa2,0x62,0x66,0xa6,0xa7,0x67,
	0xa5,0x65,0x64,0xa4,0x6c,0xac,0xad,0x6d,0xaf,0x6f,
	0x6e,0xae,0xaa,0x6a,0x6b,0xab,0x69,0xa9,0xa8,0x68,
	0x78,0xb8,0xb9,0x79,0xbb,0x7b,0x7a,0xba,0xbe,0x7e,
	0x7f,0xbf,0x7d,0xbd,0xbc,0x7c,0xb4,0x74,0x75,0xb5,
	0x77,0xb7,0xb6,0x76,0x72,0xb2,0xb3,0x73,0xb1,0x71,
	0x70,0xb0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,
	0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9c,0x5c,
	0x5d,0x9d,0x5f,0x9f,0x9e,0x5e,0x5a,0x9a,0x9b,0x5b,
	0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4b,0x8b,
	0x8a,0x4a,0x4e,0x8e,0x8f,0x4f,0x8d,0x4d,0x4c,0x8c,
	0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
	0x43,0x83,0x41,0x81,0x80,0x40
  };



//CRC�����
//puchMsg,Ҫ����CRCУ�����Ϣ
//��Ϣ���ֽ���
unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen)
{
	unsigned char uchCRCHi = 0xff;    //CRC���ֽڳ�ʼ��
	unsigned char uchCRCLo = 0xff;    //CRC���ֽڳ�ʼ��
	unsigned char uIndex;             //CRCѭ������
	while (usDataLen--)
  {
  	uIndex = uchCRCHi^*puchMsg++;  //����CRC
  	uchCRCHi = uchCRCLo^auchCRCHi[uIndex];
  	uchCRCLo = auchCRCLo[uIndex];
  }
  
  return (uchCRCHi<<8 | uchCRCLo);
}
*/

/*******************************************************
��������:threadOfCtrl
��������:���رջ���������(��أ�����;)
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
void threadOfCtrl(void *arg)
{
   INT8U     tmpNumOfZjz;
   INT8U     i,j, pn, tmpDate;
   INT8U     tmpRound, tmpRoundx;
   INT32U    protectSafeKw;                              //������ֵKw
   INT16U    protectSafeWatt;                            //������ֵWatt
	 INT32U    presentData, alarmLimit, ctrlLimit;         //��������õ�����(KW)
	 INT16U    presentWatt, alarmLimitWatt, ctrlLimitWatt; //��������õ�����(W)
	 INT8U     tmpPeriodCount;                             //ʱ������ֵ   ��ʾ��xxʱ��
	 INT16U    periodCtrlLimit;                            //ʱ�ι��ض�ֵ ��ʾ��xxʱ�εĹ��ض�ֵ
	 DATE_TIME timeLimitStart, timeLimitEnd;
	 INT32U    tmpLimit,tmpData,tmpWatt;
	 INT32U    eventDataKw;  
   INT8U     powerQuantity;
   DATE_TIME tmpTime;
   INT8U     k, onlyHasPulsePn;       //ly,2011-04-25,add
   BOOL      ifExec,bufHasData;
   INT8U     negPresent, negAlarm, negCtrl;
	 INT8U     eventData[13];
   
   //�ù���,��ص�
   for (i = 0; i < totalAddGroup.numberOfzjz; i++)
   {
   	  if (ctrlRunStatus[i].ifUsePrdCtrl==CTRL_JUMP_IN || ctrlRunStatus[i].ifUseWkdCtrl==CTRL_JUMP_IN 
   	  	  || ctrlRunStatus[i].ifUseObsCtrl==CTRL_JUMP_IN || ctrlRunStatus[i].ifUsePwrCtrl==CTRL_JUMP_IN 
   	  	 )
   	  {
         statusOfGate |= 0x10;    //���ص���
   	  }

   	  if (ctrlRunStatus[i].ifUseMthCtrl==CTRL_JUMP_IN || ctrlRunStatus[i].ifUseChgCtrl==CTRL_JUMP_IN)
   	  {
         statusOfGate |= 0x20;    //��ص���
   	  }
   }
   
	 while(1)
	 {
      //0.����̵�������բ����,5���̵����ص�����״̬,ly,2011-08-15,add
      for(i=1;i<CONTROL_OUTPUT+1;i++)
      {
        if (gateJumpTime[i]>0x00)
        {
          gateJumpTime[i]++;
          if (gateJumpTime[i]>4)
          {
          	gateJumpTime[i] = 0;
          	 
          	jumpGate(i, 0);  //�̵����ص�����״̬
          }
        }
      }
      
      //1.����
      //1-1.�ն˵�ǰ״̬Ϊ����Ͷ��
      if (staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
      {
      	//�����һֱ����
        if (staySupportStatus.ifHold==0xaa)
        {
           alarmInQueue(PAUL_ELECTRICITY,0);
        }
        else
        {
          //��ǰʱ���Ѿ������˱������ʱ�䣬�������
          if (compareTwoTime(staySupportStatus.endStaySupport, sysTime))
          {
             staySupportStatus.ifStaySupport = CTRL_RELEASE;
          }
          else
          {
             alarmInQueue(PAUL_ELECTRICITY,0);
          }
        }
      }
      
      //1-2.������
      if (staySupportStatus.ifStaySupport == CTRL_RELEASE)
      {
      	staySupportStatus.ifStaySupport = CTRL_RELEASEED;  //�����ѽ��
      	ctrlStatus.numOfAlarm = 0;
      	menuInLayer = 1;
  	     
  	    saveParameter(0x05, 25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));  //�ն˱���Ͷ��/���
      }
      
      //����״̬��ִ��ң�ء���ء�����
      if (staySupportStatus.ifStaySupport!=CTRL_JUMP_IN)
      {
        //2.ң��
        for (i=0; i<CONTROL_OUTPUT; i++)
        {
        	//ң���¼�
        	if (remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_JUMP_IN       //��·ң��Ͷ��״̬
        		  || remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_RELEASEED  //�����ѽ��״̬
        		 )
        	{
          	//��¼ң����բ�¼�
          	if (compareTwoTime(remoteEventInfor[i].freezeTime, sysTime) && remoteEventInfor[i].freezeTime.year != 0x00)
          	{
        	    if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	    {
        	    	 printf("threadOfCtrl-ң��:%02d-%02d-%02d %02d:%02d:%02d,��¼��%dң���¼�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
        	    }
        	    
        	    //��¼�¼������
              if ((eventRecordConfig.iEvent[0] & 0x10) || (eventRecordConfig.nEvent[0] & 0x10))
              {
         	      eventData[0] = 0x5;
         	      eventData[1] = 0xd;
         	      
         	      //��բʱ��
         	      eventData[2] = remoteEventInfor[i].remoteTime.second/10<<4 | remoteEventInfor[i].remoteTime.second%10;
         	      eventData[3] = remoteEventInfor[i].remoteTime.minute/10<<4 | remoteEventInfor[i].remoteTime.minute%10;
         	      eventData[4] = remoteEventInfor[i].remoteTime.hour  /10<<4 | remoteEventInfor[i].remoteTime.hour  %10;
         	      eventData[5] = remoteEventInfor[i].remoteTime.day   /10<<4 | remoteEventInfor[i].remoteTime.day   %10;
         	      eventData[6] = remoteEventInfor[i].remoteTime.month /10<<4 | remoteEventInfor[i].remoteTime.month %10;
         	      eventData[7] = remoteEventInfor[i].remoteTime.year  /10<<4 | remoteEventInfor[i].remoteTime.year  %10;
         	      
         	      //��բ�ִ�
         	      eventData[8] = 0x01<<i;
         	      
         	      //��բʱ����(�ܼӹ���)
         	      eventData[9] = remoteEventInfor[i].remotePower[0];
         	      eventData[10] = remoteEventInfor[i].remotePower[1];
            	  
            	  //��բ���������ܼӹ���
            	  //�����ִ�1-4�ֱ��¼�ܼ���1-4�Ĺ���,δ�����ܼ���������ݼ�Ϊ0xee(������)
            		tmpTime = timeHexToBcd(sysTime);
                if (readMeterData(ctrlReadBuf, i+1, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
          	    {   	      
         	         eventData[11] = ctrlReadBuf[GP_WORK_POWER+1];
         	         eventData[12] = ((ctrlReadBuf[GP_WORK_POWER]&0x7)<<5)
                                 | (ctrlReadBuf[GP_WORK_POWER]&0x10)
                                 | (ctrlReadBuf[GP_WORK_POWER+2]&0x0F);
                }
                else
                {
                	 eventData[11] = 0xee;
                	 eventData[12] = 0xee;
                }
         	  
                if (eventRecordConfig.iEvent[0] & 0x10)
                {
         	         writeEvent(eventData, 13, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
         	      }
                if (eventRecordConfig.nEvent[0] & 0x10)
                {
         	         writeEvent(eventData, 13, 2, DATA_FROM_LOCAL);  //����һ���¼�����
         	      }
                
                if (debugInfo&PRINT_EVENT_DEBUG)
                {
                  printf("realCtrlTask���������ϱ�\n");
                }
      
                activeReport3();   //�����ϱ��¼�
         	      
         	      eventStatus[0] = eventStatus[0] | 0x10;
              }
            	  
            	remoteEventInfor[i].freezeTime.year = 0xFF;
              
              //ly,09-06-22,add,û����һ��Ļ�,�����ң����բ��ÿ�����������ң����բ�¼�
            	saveParameter(0x05, 3,(INT8U *)&remoteEventInfor,sizeof(REMOTE_EVENT_INFOR)*8);   //ң���ֳ���¼
          	}
          }
        	
        	if (remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_JUMP_IN)   //��·ң��Ͷ��״̬
        	{
        	  if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	  {
        	    printf("threadOfCtrl-ң��:��%d·Ͷ��\n", i+1);
        	  }
        	  
          	//��δ�������״̬
          	if (remoteCtrlConfig[i].ifRemoteCtrl == 0x00)
        	  {
              //�Ѿ�����ң��ʱ��
              if (compareTwoTime(remoteCtrlConfig[i].remoteStart, sysTime)
            	   && compareTwoTime(sysTime, remoteCtrlConfig[i].remoteEnd))
            	{
            	  if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            	  {
            	    printf("threadOfCtrl-ң��:%02d-%02d-%02d %02d:%02d:%02d,��%d·��բʱ�̵�\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
            	  }

          	    #ifdef REMOTE_CTRL_CONFIRM
            	    if (remoteCtrlConfig[i].ifRemoteConfirm == 0xAA)  //��У��ȷ��
            	    {
            	  #endif //REMOTE_CTRL_CONFIRM
            	      //����澯  	      
            	      remoteCtrlConfig[i].alarmStart.year = 0xFF;
            	       
                    remoteEventInfor[i].remoteTime.second = sysTime.second;
                    remoteEventInfor[i].remoteTime.minute = sysTime.minute;
                    remoteEventInfor[i].remoteTime.hour   = sysTime.hour;
                    remoteEventInfor[i].remoteTime.day    = sysTime.day;
                    remoteEventInfor[i].remoteTime.month  = sysTime.month;
                    remoteEventInfor[i].remoteTime.year   = sysTime.year;
                  
                    remoteEventInfor[i].freezeTime = nextTime(sysTime, 2, 0);
                      
            			  //�����ִ�1-4�ֱ��¼�ܼ���1-4�Ĺ���,δ�����ܼ���������ݼ�Ϊ0xee(������)
            			  tmpTime = timeHexToBcd(sysTime);
                    if (readMeterData(ctrlReadBuf, i+1, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
                    {
                       remoteEventInfor[i].remotePower[0] = ctrlReadBuf[GP_WORK_POWER+1];
                       remoteEventInfor[i].remotePower[1] = ((ctrlReadBuf[GP_WORK_POWER]&0x7)<<5)
                                                          |(ctrlReadBuf[GP_WORK_POWER]&0x10)
                                                          |ctrlReadBuf[GP_WORK_POWER+2]&0x0F;
                    }
                    else
                    {
                       remoteEventInfor[i].remotePower[0] = 0xee;
                       remoteEventInfor[i].remotePower[1] = 0xee;
                    }
            	       
            	      //��������ź�
          	        jumpGate(i+1, 1);
          	        
          	        ctrlJumpedStatis(REMOTE_CTRL,1);
          	        
          	        //���բ״ָ̬ʾ
          	        gateStatus(i+1, GATE_STATUS_ON);
          	              	        
          	        //�����Ѿ�ִ��
          	        remoteCtrlConfig[i].ifRemoteCtrl = 0xAA;
          	        remoteCtrlConfig[i].status = CTRL_JUMPED;  //����բ
                    saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
      
                    alarmInQueue(REMOTE_CTRL,0);               //ң��
            	      
          	        //���ټ����������������������һ���ܼ���
      	            continue;
      	            
            	  #ifdef REMOTE_CTRL_CONFIRM
            	    }
          	      else   //��Уδȷ��
          	      {
          	        if (remoteCtrlConfig[i].confirmTimeOut.year != 0xFF
          	      	  && compareTwoTime(remoteCtrlConfig[i].confirmTimeOut, sysTime))
          	        {
          	          //��У��ʱ�����ң�ز���        	      
          	          remoteCtrlConfig[i].confirmTimeOut.year = 0xFF;
                      
                      saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);    	          
          	        }
          	      }
          	    #endif //REMOTE_CTRL_CONFIRM
          	  }
          	  else
          	  {
          	    //�Ѿ�����澯ʱ��
          	    if (compareTwoTime(remoteCtrlConfig[i].alarmStart, sysTime)
            	     && compareTwoTime(sysTime, remoteCtrlConfig[i].remoteStart))
            	  {
              	  if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
              	  {
              	    printf("threadOfCtrl-ң��:%02d-%02d-%02d %02d:%02d:%02d,��%d·�澯\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
              	  }

            	    //�澯���
            	    alarmInQueue(REMOTE_CTRL,0);             //ң��
            	    
          	      remoteCtrlConfig[i].status = CTRL_ALARM; //�澯
            	    
            	    //��ʾ�澯��Ϣ
          	      
          	      //����澯ʱ�κ��յ������բ����
          	      if (compareTwoTime(remoteCtrlConfig[i].remoteEnd, sysTime))
                  {
                  	 alarmInQueue(REMOTE_CTRL,0);                  //ң��
          	         remoteCtrlConfig[i].status = CTRL_CLOSE_GATE; //�����բ
                      
                     //������Ʋ���
                     remoteCtrlConfig[i].ifRemoteCtrl = 0x00;
                     remoteCtrlConfig[i].alarmStart.year = 0xFF;
                     remoteCtrlConfig[i].remoteStart.year = 0xFF;
            	       remoteCtrlConfig[i].remoteEnd.year = 0xFF;
            	        
                     saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
                  }
            	  }
          	  }
          	}
          	else    //�Ѿ��������״̬
          	{
          	  //�Ѿ�����������ʱ��
          	  if (compareTwoTime(remoteCtrlConfig[i].remoteEnd, sysTime))
              {
                //�����բ
              	if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
              	{
              	  printf("threadOfCtrl-ң��:%02d-%02d-%02d %02d:%02d:%02d,��%d·�������ʱ�䵽\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, i+1);
              	}

                alarmInQueue(REMOTE_CTRL,0);                          //ң��
          	    remoteCtrlConfig[i].status = CTRL_CLOSE_GATE;         //�����բ
          	    remoteCtrlConfig[i].ifUseRemoteCtrl = CTRL_RELEASEED; //��·ң���ѽ��
                
                //��������ź�(����������)
                jumpGate(i+1, 0);
                
          	    //���բ״ָ̬ʾ
          	    gateStatus(i+1, GATE_STATUS_OFF);
      	                  
                //������Ʋ���
                remoteCtrlConfig[i].ifRemoteCtrl = 0x00;
                remoteCtrlConfig[i].remoteStart.year = 0xFF;
            	  remoteCtrlConfig[i].remoteEnd.year = 0xFF;
                
                saveParameter(0x05, 1,(INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);
            	  
            	  gateCloseWaitTime=GATE_CLOSE_WAIT_TIME;       //�����բ�澯��ʱ��
            	  setBeeper(BEEPER_ON);                         //��������ʾ��
       	        alarmLedCtrl(ALARM_LED_ON);                   //ָʾ����
              }
              else   //����բ��ִ�д˶�
              {
              	//���ﻹҪ����ſ�״̬�����������Ժ�բ�������¼��澯
      
              	//�����բ״ָ̬ʾ�������ն˸�λ���״̬����
                if (workSecond==1)
                {
                  remoteCtrlConfig[i].status = CTRL_JUMPED; //����բ
                  alarmInQueue(REMOTE_CTRL, 0);             //ң��
                              
          	      //���բ״ָ̬ʾ
          	      gateStatus(i+1, GATE_STATUS_ON);
        	      }
              }
          	}
          }
        }
  	    
  	    //3.��غ͹���
   	    for (tmpNumOfZjz = 0; tmpNumOfZjz<totalAddGroup.numberOfzjz; tmpNumOfZjz++)
   	    {
   	    	pn = totalAddGroup.perZjz[tmpNumOfZjz].zjzNo;
   
          //�µ�غ͹����ͬһʱ��ֻ��Ͷ��һ��
   	    	//------------------- �� �� ��  --------------------
       	  //�µ��Ͷ��
       	  if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_JUMP_IN)
       	  {
             if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
             {
    	         printf("\n�ܼ���%d�µ�����Ͷ��\n",pn);
    	       }
  
             //ÿ��ֻ��բһ���ִ�
             if (balanceComplete==TRUE)
      	     {
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //��������ִ�δ����,ִ�м������(����ִ���ҪͶ��)
                 if (!((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((electCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	 nextRound = sysTime;
                   }
                   if (compareTwoTime(nextRound, sysTime))   //��һ�ִεȴ�WAIT_NEXT_ROUND����
                   {
    	                 ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                 ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
    	                  
    	                 //�µ�����Ҫ�����Ҳ���Ϊ0
    	                 if (monthCtrlConfig[pn-1].monthCtrl==0)
    	                 {
    	                  	break;
    	                 }
    	                 
    	                 //monthCtrl(�µ�������ֵΪ4�ֽ�,���4λ(28~31λ)��ʾ������,�ڼ���ʱ��������
    	                 j = monthCtrlConfig[pn-1].monthCtrl>>28&0xf;
    	                 
    	                 //ȡ���µ�������ֵ�ĵ�28λ
    	                 tmpLimit = monthCtrlConfig[pn-1].monthCtrl&0xfffffff;
    	                 
    	                 //�µ������и���ϵ��,������¼�����㸡�������ֵ
    	                 tmpLimit = countAlarmLimit((INT8U *)&tmpLimit, 0x3, ctrlPara.monthEnergCtrlIndex,&presentWatt);
                       presentData = 0;
    	                 tmpLimit = hexToBcd(tmpLimit);
    	                 tmpLimit |= j<<28;      //��ԭ������
    	                 
    	                 if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
    	                 {
    	                   printf("�µ����ؼ���!\n");
    	                 }
                       
                       if (calcData(ctrlReadBuf,tmpLimit,&presentData,&alarmLimit,&ctrlLimit,&presentWatt,&alarmLimitWatt,&ctrlLimitWatt,0xa0, 0x03, pn))
                       {
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("�µ�����:�澯ֵ=%0d\n",alarmLimit);
                            printf("�µ�����:��ǰ����=%0d\n",presentData);
                            printf("�µ�����:����ֵ=%0d\n",ctrlLimit);
                          }
  
                          //�������澯��ֵ
                          if (presentData > alarmLimit)
                          {
                             //�ѽ���澯״̬
                             if (monthCtrlConfig[pn-1].monthAlarm == CTRL_ALARM)
                             {
                               //������������ֵ
                               if (presentData>ctrlLimit)
                               {
                                 //��������źţ�����澯�ź�
                                 //���ݸ��ܼ���ĵ���ִ��趨����ִ��趨
                                 tmpRound = 0;
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);
                                     
                                     statusOfGate |= 0x20;    //��ص���
  
                                     ctrlJumpedStatis(MONTH_CTRL,1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     electCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                     
                                     //�µ�������բ��¼
                                     eventDataKw = ctrlReadBuf[GP_MONTH_WORK+3]
                                                 | ctrlReadBuf[GP_MONTH_WORK+4] << 8 
                                                 | ctrlReadBuf[GP_MONTH_WORK+5] << 16 
                                                 | (((ctrlReadBuf[GP_MONTH_WORK]&0x01)<<6)|(ctrlReadBuf[GP_MONTH_WORK]&0x10)|(ctrlReadBuf[GP_MONTH_WORK+6]&0x0f))<<24;
                                 
                                     #ifdef ONCE_JUMP_ONE_ROUND
                                      electCtrlRecord(1, pn, 1<<j, MONTH_CTRL,(INT8U *)&eventDataKw,(INT8U *)&monthCtrlConfig[pn-1].monthCtrl);
                                      break;
                                     #else
                                      tmpRound |= 1<<j;
                                     #endif
                                   }
                                 }
                                 
                                 #ifndef ONCE_JUMP_ONE_ROUND
                                  if (tmpRound)  //������ִ���բ,��¼�¼�
                                  {
                                    electCtrlRecord(1, pn, tmpRound, MONTH_CTRL,(INT8U *)&eventDataKw,(INT8U *)&monthCtrlConfig[pn-1].monthCtrl);
                                  }
                                 #endif
                                 
                                 monthCtrlConfig[pn-1].ifMonthCtrl = 0xAA;
                                 
                                 //���Ʊ�־��λ���澯��־���
                                 monthCtrlConfig[pn-1].monthAlarm = CTRL_JUMPED;      //����բ
  			                         saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
  			                         saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
  
                                 alarmInQueue(MONTH_CTRL,pn);                         //�µ��
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0);     //��һ�ִεȴ�WAIT_NEXT_ROUND����
                                                                 
                                 //���ټ����������������������һ���ܼ���
                                 break;
                               }
                             }
                             else  //δ����澯״̬
                             {
                               //����澯ʱ��
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //����ִ����趨��δ��բ
                                  if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  
                                  tmpRound++;
                               }
                               
                               //�µ�ظ澯��¼
                               tmpRoundx = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                 //����趨�ִ�
                                 if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01)
                                 {
                                  	if (j==0)
                                  	{
                                  	  tmpRoundx = 1;
                                  	}
                                  	else
                                  	{
                                  	  tmpRoundx |= 1<<j;
                                  	}
                                 }
                               }                             
                               eventDataKw = ctrlReadBuf[GP_MONTH_WORK+3]
                                           | ctrlReadBuf[GP_MONTH_WORK+4] << 8 
                                           | ctrlReadBuf[GP_MONTH_WORK+5] << 16 
                                           | (((ctrlReadBuf[GP_MONTH_WORK]&0x01)<<6)|(ctrlReadBuf[GP_MONTH_WORK]&0x10)|(ctrlReadBuf[GP_MONTH_WORK+6]&0x0f))<<24;
                               electCtrlRecord(2, pn, tmpRoundx , MONTH_CTRL,(INT8U *)&eventDataKw, (INT8U *)&monthCtrlConfig[pn-1].monthCtrl);
  
                             	 //�澯��ʼʱ��Ϊ��ǰʱ��
                             	 monthCtrlConfig[pn-1].mthAlarmTimeOut = sysTime;
                               
                               //�����û���ִν�����բ״̬,��������բ״̬��Ϊδ��բ,����������ʽת���µ����ɻ���
                               if (monthCtrlConfig[pn-1].ifMonthCtrl == 0x00)
                               {
                                  electCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               
                               //����澯�ź�
                               monthCtrlConfig[pn-1].monthAlarm = CTRL_ALARM;  //�澯
                               alarmInQueue(MONTH_CTRL,pn);                    //�µ��
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;        //��ʼ�澯��ʾ��ʱ��
       	                       setBeeper(BEEPER_ON);                           //��������ʾ��
  	                           alarmLedCtrl(ALARM_LED_ON);                     //ָʾ����                                
  
  			                       saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
                             }
                          }
                       }
                   }
                    
                   break;  //ÿ��ֻ��բһ���ִ�,��������Ƚ���һ���ִξ��˳�
                 }
               }
             }
  
             //�����Զ����
             timeLimitEnd = nextTime(sysTime,0,1);
             if (timeLimitEnd.month!= sysTime.month)
             {
              	ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_AUTO_RELEASE;
             }
              
           	 //��λ�Ժ����ʾ����
           	 if (workSecond==1)
           	 {
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,���ң���Ѿ���բ�Ͳ��ٴ����ʼ״̬
                 {
                   if ((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
       	             gateStatus(j+1, GATE_STATUS_ON);
                     alarmInQueue(MONTH_CTRL,pn);     //�µ�����
                     statusOfGate |= 0x20;            //��ص���
       	           }
       	           else    //ly,2011-03-31,��ӵ�else
       	           {
       	             gateStatus(j+1, GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	 }
       	  }
       	  
       	  //�µ����ؽ��
          if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_AUTO_RELEASE)
          {
             ctrlRelease(pn,MONTH_CTRL,monthCtrlConfig[pn-1].ifMonthCtrl);
          }
  
       	  //-------------------  �� �� �� --------------------
   	    	//��������ȼ������µ�أ�ֻ��δͶ���µ��ʱ�Ž��й���ش���
   	    	//Ŀǰû�п����µ����غ͹���ص����ȼ� LY 2008.12.17
   	    	//�����Ͷ��
   	    	if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_JUMP_IN)
   	    	{
             if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
             {
    	         printf("\n�ܼ���%d�����Ͷ��\n",pn);
    	       }
    	       
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
    	       
    	       ifExec = FALSE;
    	       if (onlyHasPulsePn==0xaa)
    	       {
    	       	 ctrlCount++;
    	       	 if (ctrlCount>4)
    	       	 {
    	       	   ctrlCount = 0;
    	       	   tmpTime = sysTime;
    	       	   ifExec = computeInTimeLeftPower(pn, tmpTime, ctrlReadBuf, 1);
    	       	 }
    	       }
    	       else
    	       {
    	       	 ifExec = balanceComplete;
    	       }
  
             //ÿ��ֻ��բһ���ִ�
             if (ifExec==TRUE)
      	     {
               ctrlLimit = alarmLimit = presentData = 0x00000000;
               ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
               
               tmpTime = sysTime;
               if (readMeterData(ctrlReadBuf, pn, LEFT_POWER, 0x0, &tmpTime, 0)==TRUE)
               {
                 if (ctrlReadBuf[0] != 0xFF || ctrlReadBuf[0] != 0xEE)
                 {
                   //��ǰʣ�����
                   negPresent = 0;
                   negAlarm = 0;
                   negCtrl = 0;
                   
                   presentData = ctrlReadBuf[0]
                          | ctrlReadBuf[1] << 8
                          | ctrlReadBuf[2] << 16
                          | ctrlReadBuf[3] << 24;
                   if (ctrlReadBuf[3]&0x10)
                   {
                   	 negPresent = 1;
                   }
  
                   if (chargeCtrlConfig[pn-1].alarmLimit&0x10000000)
                   {
                   	 negAlarm = 1;
                   }
  
                   if (chargeCtrlConfig[pn-1].cutDownLimit&0x10000000)
                   {
                   	 negCtrl = 1;
                   }
            
                   presentData = countAlarmLimit((INT8U* )&presentData, 0x03, 0,&presentWatt);
                   alarmLimit = countAlarmLimit((INT8U* )&chargeCtrlConfig[pn-1].alarmLimit, 0x03, 0,&alarmLimitWatt);
                   ctrlLimit = countAlarmLimit((INT8U* )&chargeCtrlConfig[pn-1].cutDownLimit, 0x03, 0,&ctrlLimitWatt);
                   
                   if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                   {
                     if (negAlarm==1)
                     {
                       printf("�����:�澯ֵ=-%0d\n",alarmLimit);
                     }
                     else
                     {
                       printf("�����:�澯ֵ=%0d\n",alarmLimit);
                     }
                     
                     if (negPresent==1)
                     {
                       printf("�����:��ǰʣ�����=-%0d\n",presentData);
                     }
                     else
                     {
                       printf("�����:��ǰʣ�����=%0d\n",presentData);
                     }
                     
                     if (negCtrl==1)
                     {
                       printf("�����:����ֵ=-%0d\n",ctrlLimit);
                     }
                     else
                     {
                       printf("�����:����ֵ=%0d\n",ctrlLimit);
                     }
                   }
  
                   for(j=0;j<CONTROL_OUTPUT;j++)
                   {
                     //��������ִ�δ����,ִ�м������
                     if (!((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((electCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                     {
                       if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                       {
                       	 nextRound = sysTime;
                       }
                       if (compareTwoTime(nextRound,sysTime))   //��һ�ִεȴ�WAIT_NEXT_ROUND����
                       {
                         //��ʣ������澯��ֵ
                         if ((negPresent==0 && negAlarm==0 && presentData <= alarmLimit)   //�����ǰֵ�͸澯ֵ��������
                         	   || (negPresent==1 && negAlarm==0)                             //�����ǰֵ�Ǹ������澯ֵ������
                         	   || (negPresent==1 && negAlarm==1 && presentData >= alarmLimit)//�����ǰֵ�͸澯���Ǹ���
                         	  )
                         {
                            //�ѽ���澯״̬
                            if (chargeCtrlConfig[pn-1].chargeAlarm == CTRL_ALARM)
                            {
                              //��ʣ�����������ֵ
                              //if (presentData<=ctrlLimit)
                              if ((negPresent==0 && negCtrl==0 && presentData <= ctrlLimit)    //�����ǰֵ����բֵ��������
                         	        || (negPresent==1 && negCtrl==0)                             //�����ǰֵ�Ǹ�������բֵ������
                         	         || (negPresent==1 && negCtrl==1 && presentData >= ctrlLimit)//�����ǰֵ����բ���Ǹ���
                         	       )
                              {
                                //��������źţ�����澯�ź�
                                //���ݸ��ܼ���ĵ���ִ��趨����ִ��趨
                                tmpRound = 0;
                                for (j = 0; j < CONTROL_OUTPUT; j++)
                                {
                                  if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                    jumpGate(j+1, 1);        //�̵�����������                                    
                                    statusOfGate |= 0x20;    //��ص���
                                    
                                    ctrlJumpedStatis(CHARGE_CTRL,1);
                                    gateStatus(j+1,GATE_STATUS_ON);
                                    electCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                    
                                    //�������բ��¼
                                    eventDataKw = ctrlReadBuf[0]
                                                | ctrlReadBuf[1] << 8
                                                | ctrlReadBuf[2] << 16
                                                | ctrlReadBuf[3] << 24;
                                    #ifdef ONCE_JUMP_ONE_ROUND
                                     electCtrlRecord(1, pn, 1<<j, CHARGE_CTRL,(INT8U *)&eventDataKw,(INT8U *)&chargeCtrlConfig[pn-1].cutDownLimit);
                                    
                                     break;
                                    #else
                                     tmpRound |= 1<<j;
                                    #endif
                                  }
                                }
                                
                                #ifndef ONCE_JUMP_ONE_ROUND
                                 if (tmpRound)
                                 {
                                   electCtrlRecord(1, pn, tmpRound, CHARGE_CTRL,(INT8U *)&eventDataKw,(INT8U *)&chargeCtrlConfig[pn-1].cutDownLimit);
                                 }
                                #endif
                                
                                chargeCtrlConfig[pn-1].ifChargeCtrl = 0xAA;
                                
                                //���Ʊ�־��λ���澯��־���
                                chargeCtrlConfig[pn-1].chargeAlarm = CTRL_JUMPED;   //����բ
  
  			                        saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
  			                        saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
                            
                                alarmInQueue(CHARGE_CTRL,pn);                       //�����
                                
                                nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0);    //��һ�ִεȴ�WAIT_NEXT_ROUND����
  
                                //���ټ��������������
                                break;
                              }
                            }
                            else  //δ����澯״̬
                            {
                            	//�澯��ʼʱ��Ϊ��ǰʱ��
                            	chargeCtrlConfig[pn-1].alarmTimeOut = sysTime;
                              
                              //�����û���ִν�����բ״̬,��������բ״̬��Ϊδ��բ,����������ʽת�빺�����ɻ���
                              if (chargeCtrlConfig[pn-1].ifChargeCtrl == 0x00)
                              {
                                 electCtrlRoundFlag[pn-1].ifJumped = 0x0;
                              }
  
                              //�µ�ظ澯��¼
                              tmpRoundx = 0;
                              for(j=0;j<CONTROL_OUTPUT;j++)
                              {
                                 //����趨�ִ�
                                 if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01)
                                 {
                                  	if (j==0)
                                  	{
                                  	  tmpRoundx = 1;
                                  	}
                                  	else
                                  	{
                                  	  tmpRoundx |= 1<<j;
                                  	}
                                 }
                              }
                              
                              eventDataKw = ctrlReadBuf[(pn-1)*12+0]
                                          | ctrlReadBuf[(pn-1)*12+1] << 8
                                          | ctrlReadBuf[(pn-1)*12+2] << 16
                                          | ctrlReadBuf[(pn-1)*12+3] << 24;
                              electCtrlRecord(2, pn, tmpRoundx , CHARGE_CTRL,(INT8U *)&eventDataKw, (INT8U *)&chargeCtrlConfig[pn-1].cutDownLimit);
  
                              //����澯�ź�
                              chargeCtrlConfig[pn-1].chargeAlarm = CTRL_ALARM; //�澯
                              alarmInQueue(CHARGE_CTRL,pn);                    //�����
                              gateCloseWaitTime=START_ALARM_WAIT_TIME;         //��ʼ�澯��ʾ��ʱ��
      	                      setBeeper(BEEPER_ON);                            //��������ʾ��
                              alarmLedCtrl(ALARM_LED_ON);                      //ָʾ����
  			                      saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
                            }
                         }
                       }
                       break;  //ÿ��ֻ��բһ���ִ�,��������Ƚ���һ���ִξ��˳�
                     }
                   }
                   
                   //���ʣ�����������բ���������ִ�����բ,�Զ��������
                   if (((negPresent==0 && negCtrl==0 && presentData > ctrlLimit)    //�����ǰֵ����բֵ��������
               	       || (negPresent==0 && negCtrl==1)                             //�����ǰֵ����������բֵ�Ǹ���
               	        || (negPresent==1 && negCtrl==1 && presentData < ctrlLimit))//�����ǰֵ����բ���Ǹ���
               	       && chargeCtrlConfig[pn-1].chargeAlarm == CTRL_JUMPED
               	      )
                   {
                   	 ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_AUTO_RELEASE;
                   }
                 }
               }
             }
  
           	 //��λ�Ժ����ʾ����(ֻ����Ͷ��״̬����բ��״̬)
           	 if (workSecond==1)
           	 {
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,���ң���Ѿ���բ�Ͳ��ٴ����ʼ״̬
                 {
                   if ((electCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
       	             gateStatus(j+1,GATE_STATUS_ON);
       	             statusOfGate |= 0x20;                  //��ص���
                     alarmInQueue(CHARGE_CTRL,pn);          //�����
       	           }
       	           else    //ly,2011-03-31,��ӵ�else
       	           {
       	             gateStatus(j+1,GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	 }
       	  }
   	      
       	  //����ؽ��
          if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_AUTO_RELEASE)
          {
            ctrlRelease(pn,CHARGE_CTRL,chargeCtrlConfig[pn-1].ifChargeCtrl);
          }
  
          //------------------- �߷Ѹ澯 -------------------
          //�߷Ѹ澯Ͷ��
          if (reminderFee==CTRL_JUMP_IN)
          {
              if (chargeAlarm[sysTime.hour/8]>>(sysTime.hour%8) & 0x1)
              {
        	       setBeeper(BEEPER_ON);                     //������
        	       alarmLedCtrl(ALARM_LED_ON);               //ָʾ����
                 gateCloseWaitTime=GATE_CLOSE_WAIT_TIME;   //�����բ�澯��ʱ��
                 alarmInQueue(REMINDER_FEE,0);
        	    }
        	    else
        	    {
        	       setBeeper(BEEPER_OFF);                    //������
        	       alarmLedCtrl(ALARM_LED_OFF);              //ָʾ����
                 gateCloseWaitTime=0;
        	    }
          }
  
          //�߷Ѹ澯���
          if (reminderFee==CTRL_RELEASE)
          {
        	   setBeeper(BEEPER_OFF);                    //������
        	   alarmLedCtrl(ALARM_LED_OFF);              //ָʾ����
             gateCloseWaitTime=0;
             reminderFee = CTRL_RELEASEED;
                           
             for(i=0;i<ctrlStatus.numOfAlarm;i++)
             {
                if (ctrlStatus.aQueue[i]==REMINDER_FEE)
                {
             	    for(j=i;j<ctrlStatus.numOfAlarm;j++)
             	    {
             	       ctrlStatus.aQueue[j] = ctrlStatus.aQueue[j+1];
             	       ctrlStatus.allPermitClose[j]=ctrlStatus.allPermitClose[j+1];
             	    }
             	    ctrlStatus.numOfAlarm--;
             	    if (ctrlStatus.numOfAlarm==0)
             	    {
             	    	menuInLayer=1;           	    	 
             	    }
             	    break;
                }
             }
          }
                 	  
       	  powerCtrlRecord();     //�ж��Ƿ��¼�����¼���¼
       	  
       	  //------------------- �� ǰ �� �� �� �� �� --------------------
       	  //��ǰ�����¸���Ͷ��
       	  if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
       	  {
             if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
             {
    	         printf("\n�ܼ���%d��ǰ�����¸���Ͷ��\n", pn);
    	       }
   	      	 
   	      	 //��ֵ��ȷ��
   	      	 if (powerDownCtrl[pn-1].freezeTime.year==0xff)
   	      	 {
               //ÿ��ֻ��բһ���ִ�
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //��������ִ�δ����,ִ�м������
                 if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	  nextRound = sysTime;
                   }
                   if (compareTwoTime(nextRound,sysTime))   //��һ�ִεȴ�WAIT_NEXT_ROUND����
                   {
    	                 ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                 ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
  
    	                 if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
    	                 {
    	                   printf("�����¸��ؼ���\n");
    	                 }
  
                       if (calcData(ctrlReadBuf,0,&presentData,NULL,NULL,&presentWatt,NULL,NULL,0, 0x02, pn))
                       {
                          //������ֵ
                          protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
  
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("�����¸���:������ֵ=%0d\n",protectSafeKw);
                            printf("�����¸���:����ֵ=%0d\n",powerDownCtrl[pn-1].powerDownLimit);
                            printf("�����¸���:��ǰ����=%0d\n",presentData);
                          }
  
                          //����ֵ�뱣����ֵ�Ƚ�,�������ֵС�ڱ�����ֵ,�����ֵ=������ֵ
                          if (powerDownCtrl[pn-1].powerDownLimit<protectSafeKw 
                          	|| (powerDownCtrl[pn-1].powerDownLimit==protectSafeKw && powerDownCtrl[pn-1].powerLimitWatt<protectSafeWatt))
                          {
                             ctrlLimit = protectSafeKw;
                             ctrlLimitWatt = protectSafeWatt;
                          }
                          else
                          {
                             ctrlLimit = powerDownCtrl[pn-1].powerDownLimit;
                             ctrlLimitWatt = powerDownCtrl[pn-1].powerLimitWatt;
                          }
                          
                          //�����ʸ澯��ֵ
                          if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                          {
                             //�ѽ���澯״̬
                             if (powerDownCtrl[pn-1].pwrDownAlarm == CTRL_ALARM)
                             {
                               //�澯ʱ�䵽��,��բһ���ִ�
                               if (compareTwoTime(powerDownCtrl[pn-1].alarmEndTime,sysTime))
                               {
                                 //��������źţ�����澯�ź�
                                 //���ݸ��ܼ���Ĺ����ִ��趨����ִ��趨
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);

                                     statusOfGate |= 0x10;    //���ص���
                                     
                                     ctrlJumpedStatis(POWER_CTRL,1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
  
                                     //��¼������բ�ֳ���Ϣ
                                     tmpData = powerDownCtrl[pn-1].powerDownLimit;
                                     tmpWatt = powerDownCtrl[pn-1].powerLimitWatt*10;
                                     powerQuantity = dataFormat(&tmpData, &tmpWatt, 2);
                                     tmpData = hexToBcd(tmpData);
  
                                     tmpLimit = tmpData&0xFF;
                                     tmpLimit |= ((tmpData>>8&0xFF)&0xF)<<8;
                                     tmpLimit |= (powerQuantity&0x10)<<8;
                                     tmpLimit |= (powerQuantity&0x07)<<13;
                  	                  
                                     powerCtrlInQueue(pn,POWER_CTRL,j,(INT8U *)&tmpLimit);
                                     break;
                                   }
                                 }
                                 
                                 powerDownCtrl[pn-1].ifPwrDownCtrl = 0xAA;
                                 
                                 //���Ʊ�־��λ���澯��־���
                                 powerDownCtrl[pn-1].alarmEndTime.year = 0xFF;
                                 powerDownCtrl[pn-1].pwrDownAlarm = CTRL_JUMPED;      //����բ
  	                             
  	                             saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //��ǰ�����¸�����
  			                         saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
                             
                                 alarmInQueue(POWER_CTRL, pn);                        //��ǰ�����¸���
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0);     //��һ�ִεȴ�WAIT_NEXT_ROUND����
                             
                                 //���ټ����������������������һ���ܼ���
                                 break;
                               }
                             }
                             else  //δ����澯״̬
                             {
                               //����澯ʱ��
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //�����ִ����趨��δ��բ
                                  if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  tmpRound++;
                               }
                             	 if (powerCtrlAlarmTime[j].def==0xaa)
                             	 {
                             	   powerDownCtrl[pn-1].alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                             	 }
                             	 else     //���û�����ø��ִεĹ��ظ澯ʱ��,Ĭ��Ϊ1����
                             	 {
                             	    powerDownCtrl[pn-1].alarmEndTime = nextTime(sysTime, 1, 0);
                             	 }
                               
                               //�����û���ִν�����բ״̬,��������բ״̬��Ϊδ��բ,����������ʽת�뵱ǰ�����¸�����ɻ���
                               if (powerDownCtrl[pn-1].ifPwrDownCtrl == 0x00)
                               {
                                  powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               
                               //����澯�ź�
                               powerDownCtrl[pn-1].pwrDownAlarm = CTRL_ALARM;  //�澯
                               alarmInQueue(POWER_CTRL,pn);                    //��ǰ�����¸���
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;        //��ʼ�澯��ʾ��ʱ��
       	                       setBeeper(BEEPER_ON);                           //��������ʾ��
  	                           alarmLedCtrl(ALARM_LED_ON);                     //ָʾ����
  	                           saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //��ǰ�����¸�����
                             }
                          }
                       }
                    }
                    
                    break;  //ÿ��ֻ��բһ���ִ�,��������Ƚ���һ���ִξ��˳�
                  }
                }
   	      	 }
   	      	 else  //��δ���㶨ֵ(powerDownCtrl[pn-1].freezeTime.year��Ϊ0xffΪδ���㶨ֵ,�������Ϊ0xff)
   	      	 {
   	      	   if (compareTwoTime(powerDownCtrl[pn-1].freezeTime,sysTime) && powerDownCtrl[pn-1].freezeTime.year!=0x00)
   	      	   {
   	      	 	    if (calcData(ctrlReadBuf,0,&powerDownCtrl[pn-1].powerDownLimit,&powerDownCtrl[pn-1].powerDownLimit,NULL,&presentWatt,&powerDownCtrl[pn-1].powerLimitWatt,NULL,powerDownCtrl[pn-1].floatFactor, 0x02, pn))
                  {
   	      	 	       powerDownCtrl[pn-1].freezeTime.year = 0xff;
   	      	 	    }
   	      	   }
   	      	 }
  
             //��0���Զ����
             timeLimitEnd = nextTime(sysTime,0,1);
             if (timeLimitEnd.day!= sysTime.day)
             {             	  
              	ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_AUTO_RELEASE;
             }
              
           	 //��λ�Ժ����ʾ����(ֻ����Ͷ��״̬����բ��״̬)
           	 if (workSecond==1)
           	 {
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,���ң���Ѿ���բ�Ͳ��ٴ����ʼ״̬
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
       	             gateStatus(j+1,GATE_STATUS_ON);
                     alarmInQueue(POWER_CTRL,pn);      //��ǰ�����¸���
       	             statusOfGate |= 0x10;             //��ص���
       	           }
       	           else    //ly,2011-03-31,��ӵ�else
       	           {
       	             gateStatus(j+1,GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	 }
       	  }
       	  
       	  //��ǰ�����¸��ؽ��
          if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_AUTO_RELEASE)
          {
             ctrlRelease(pn,POWER_CTRL,powerDownCtrl[pn-1].ifPwrDownCtrl);
             continue;
          }
  
       	  //----------------------- ��  ͣ  �� --------------------------
       	  //���ֹ������ȼ�,�������ȼ�����Ͷ��������ȼ����Ʋ�ִ��
       	  if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
       	  {
       	    continue;
       	  }
       	  
       	  //��ͣ��Ͷ��
       	  if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_JUMP_IN)
       	  {
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
    	        printf("\n�ܼ���%dӪҵ��ͣ��Ͷ��\n",pn);
     	    	  printf("��ͣ��ʼ:%02d-%02d-%02d\n",obsCtrlConfig[pn-1].obsStartYear,obsCtrlConfig[pn-1].obsStartMonth,obsCtrlConfig[pn-1].obsStartDay);
     	    	  printf("��ͣ����:%02d-%02d-%02d\n",obsCtrlConfig[pn-1].obsEndYear,obsCtrlConfig[pn-1].obsEndMonth,obsCtrlConfig[pn-1].obsEndDay);
    	      }
     	    	
     	    	//��ǰʱ�䴦�ڱ�ͣ��ʱ��(ʱ���жϱ�׼:����ʱ���ǽ����յ�24:00������0:00)
   	      	if  (compareTwoDate(sysTime,obsCtrlConfig[pn-1].obsStartYear,obsCtrlConfig[pn-1].obsStartMonth,obsCtrlConfig[pn-1].obsStartDay,1)
   	      		&& compareTwoDate(sysTime,obsCtrlConfig[pn-1].obsEndYear,obsCtrlConfig[pn-1].obsEndMonth,obsCtrlConfig[pn-1].obsEndDay,2))
     	      {
               //ÿ��ֻ��բһ���ִ�
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //��������ִ�δ����,ִ�м������
                 if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	  nextRound = sysTime;
                   }
                   if (compareTwoTime(nextRound,sysTime))   //��һ�ִεȴ�WAIT_NEXT_ROUND����
                   {
    	                 ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                 ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
                       
                       if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                       {
    	                   printf("Ӫҵ��ͣ�ؼ���\n");
    	                 }
    	                 
                       if (calcData(ctrlReadBuf,obsCtrlConfig[pn-1].obsLimit,&presentData,NULL,&ctrlLimit,&presentWatt,NULL,&ctrlLimitWatt,ctrlPara.pCtrlIndex, 0x02, pn))
                       {
                          //������ֵ
                          protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
                          
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("Ӫҵ��ͣ��:������ֵ=%0d\n",protectSafeKw);
                            printf("Ӫҵ��ͣ��:����ֵ=%0d\n",ctrlLimit);
                            printf("Ӫҵ��ͣ��:��ǰ����=%0d\n",presentData);
                          }
  
                          //����ֵ�뱣����ֵ�Ƚ�,�������ֵС�ڱ�����ֵ,�����ֵ=������ֵ
                          if (ctrlLimit<protectSafeKw || (ctrlLimit==protectSafeKw && ctrlLimitWatt<protectSafeWatt))
                          {
                            	ctrlLimit = protectSafeKw;
                            	ctrlLimitWatt = protectSafeWatt;
                          }
  
                          //�����ʸ澯��ֵ
                          if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                          {
                             //�ѽ���澯״̬
                             if (obsCtrlConfig[pn-1].obsAlarm == CTRL_ALARM)
                             {
                               //�澯ʱ�䵽��,��բһ���ִ�
                               if (compareTwoTime(obsCtrlConfig[pn-1].alarmEndTime,sysTime))
                               {
                                 //��������ź�,����澯�ź�
                                 //���ݸ��ܼ���ĵ���ִ��趨����ִ��趨
                                 tmpRound = 0;
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);

                                     statusOfGate |= 0x10;    //���ص���

                                     ctrlJumpedStatis(OBS_CTRL,1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                     
                                     //#ifdef ONCE_JUMP_ONE_ROUND
                                      //��¼������բ�ֳ���Ϣ
                                      powerCtrlInQueue(pn,OBS_CTRL,j,(INT8U *)&obsCtrlConfig[pn-1].obsLimit);
                                     
                                      break;
                                     //#else
                                     // tmpRound |= 1<<j;
                                     //#endif
                                   }
                                 }
                                     
                                 //#ifndef ONCE_JUMP_ONE_ROUND
                                   //��¼������բ�ֳ���Ϣ
                                 //  powerCtrlInQueue(pn,OBS_CTRL,tmpRound,(INT8U *)&obsCtrlConfig[pn-1].obsLimit);
                                 //#endif
                                                                
                                 obsCtrlConfig[pn-1].ifObsCtrl = 0xAA;
                                 
                                 //���Ʊ�־��λ���澯��־���
                                 obsCtrlConfig[pn-1].alarmEndTime.year = 0xFF;
                                 obsCtrlConfig[pn-1].obsAlarm = CTRL_JUMPED;      //����բ
                                 
                                 saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
  			                         saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
                                 
                                 alarmInQueue(OBS_CTRL,pn);                       //��ͣ��
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0); //��һ�ִεȴ�WAIT_NEXT_ROUND����
                             
                                 //���ټ����������������������һ���ܼ���
                                 break;
                               }
                             }
                             else  //δ����澯״̬
                             {
                               //����澯ʱ��
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //�����ִ����趨��δ��բ
                                  if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  tmpRound++;
                               }
                             	if (powerCtrlAlarmTime[j].def==0xaa)
                             	{
                             	   obsCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                             	}
                             	else     //���û�����ø��ִεĹ��ظ澯ʱ��,Ĭ��Ϊ1����
                             	{
                             	   obsCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, 1, 0);
                             	}
                               
                               //�����û���ִν�����բ״̬,��������բ״̬��Ϊδ��բ,����������ʽת�뱨ͣ����ɻ���
                               if (obsCtrlConfig[pn-1].ifObsCtrl == 0x00)
                               {
                                  powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               
                               //����澯�ź�
                               obsCtrlConfig[pn-1].obsAlarm = CTRL_ALARM;  //�澯
                               alarmInQueue(OBS_CTRL,pn);                  //��ͣ��
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;    //��ʼ�澯��ʾ��ʱ��
       	                       setBeeper(BEEPER_ON);                       //��������ʾ��
  	                           alarmLedCtrl(ALARM_LED_ON);                 //ָʾ����
                               
                               saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
                             }
                          }
                       }
                   }
                    
                   break;  //ÿ��ֻ��բһ���ִ�,��������Ƚ���һ���ִξ��˳�
                 }
               }
     	      }
     	      else  //��ǰʱ���Ѿ��뿪��ͣ��ʱ��
     	      {
     	         //����ѽ��뱨ͣ�ؿ���״̬(�������ִ���բ)״̬
     	         if (obsCtrlConfig[pn-1].ifObsCtrl == 0xAA)
     	         {
     	            ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_AUTO_RELEASE;   //��Ϊ�����Զ����
        	        
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("�뿪��ͣ��ʱ��,�Զ���բ\n");
        	        }
     	         }
     	      }
           	
           	//��λ�Ժ����ʾ����(ֻ����Ͷ��״̬����բ��״̬)
           	if (workSecond==1)
           	{
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,���ң���Ѿ���բ�Ͳ��ٴ����ʼ״̬
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
                     statusOfGate |= 0x10;              //���ص���
       	             gateStatus(j+1,GATE_STATUS_ON);
                     alarmInQueue(OBS_CTRL,pn);         //��ͣ��
       	           }
       	           else    //ly,2011-03-31,��ӵ�else
       	           {
       	             gateStatus(j+1,GATE_STATUS_OFF);
       	           }
       	         }
     	         }
           	}
          }
  
          //��ͣ�ؽ��
          if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_RELEASE || ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_AUTO_RELEASE)
          {
             ctrlRelease(pn,OBS_CTRL,obsCtrlConfig[pn-1].ifObsCtrl);
             continue;
          }
  
   	      //----------------------- ��  ��  �� ---------------------------
   	      //���ֹ������ȼ�,�������ȼ�����Ͷ��������ȼ����Ʋ�ִ��
   	      if (ctrlRunStatus[pn-1].ifUsePwrCtrl == 0xAA
   	      	  || ctrlRunStatus[pn-1].ifUseObsCtrl == 0xAA)
       	  {
       	     continue;
       	  }
   	      
       	  //���ݿ�Ͷ��
       	  if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_JUMP_IN)
       	  {
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
    	        printf("\n�ܼ���%d���ݿ�Ͷ��\n",pn);
    	      }
             
            //���ݿ�ÿ���޵���
            tmpDate = dayWeek(2000+sysTime.year, sysTime.month, sysTime.day);
            if (tmpDate == 0)
            {
              tmpDate = 7;
            }
            tmpDate = (wkdCtrlConfig[pn-1].wkdDate>>tmpDate)&0x1;
  
            if (tmpDate)
            {
       	      timeLimitStart = sysTime;
       	      timeLimitEnd = sysTime;
       	      
         	    //������ʼʱ��(�޵���ʼʱ�������ݸ�ʽ19,����Ҫ��BCD��ʽ��ʱ���ʽת����16���Ƶ�ʱ���ʽ)
         	    timeLimitStart.minute = (wkdCtrlConfig[pn-1].wkdStartMin&0xf)+(wkdCtrlConfig[pn-1].wkdStartMin>>4&0xf)*10;
         	    timeLimitStart.hour = (wkdCtrlConfig[pn-1].wkdStartHour&0xf)+(wkdCtrlConfig[pn-1].wkdStartHour>>4&0xf)*10;
         	    timeLimitStart.second = 0;
  
       	      //���ƽ���ʱ�ޣ�����֤�޵�ʱ��û�п����
       	      timeLimitEnd = nextTime(timeLimitStart, wkdCtrlConfig[pn-1].wkdTime*30, 0);
       	      if (timeLimitEnd.day != timeLimitStart.day)
       	      {
       	         timeLimitEnd = timeLimitStart;
       	         timeLimitEnd.hour = 23;
       	         timeLimitEnd.minute = 59;
       	         timeLimitEnd.second = 59;
       	      }
       	      
       	      if (compareTwoTime(timeLimitStart, sysTime) && compareTwoTime(sysTime, timeLimitEnd))
         	    {
                  //ÿ��ֻ��բһ���ִ�
   	             for(j=0;j<CONTROL_OUTPUT;j++)
   	             {
   	               //��������ִ�δ����,ִ�м������
   	               if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
   	               {
   	                 if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
   	                 {
   	                 	  nextRound = sysTime;
   	                 }
  
   	                 if (compareTwoTime(nextRound,sysTime))   //��һ�ִεȴ�WAIT_NEXT_ROUND����
   	                 {
                         ctrlLimit = alarmLimit = presentData = 0x00000000;
    	                   ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
                         
                         if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                         {
    	                    printf("���ݿؼ���\n");
    	                   }
    	                   
                         if (calcData(ctrlReadBuf,wkdCtrlConfig[pn-1].wkdLimit,&presentData,NULL,&ctrlLimit,&presentWatt,NULL,&ctrlLimitWatt,ctrlPara.pCtrlIndex, 0x02, pn))
                         {
                            //������ֵ
                            protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
                            
                            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                            {
                              printf("���ݿ�:������ֵ=%0d\n",protectSafeKw);
                              printf("���ݿ�:����ֵ=%0d\n",ctrlLimit);
                              printf("���ݿ�:��ǰ����=%0d\n",presentData);
                            }
                            
                            //����ֵ�뱣����ֵ�Ƚ�,�������ֵС�ڱ�����ֵ,�����ֵ=������ֵ
                            if (ctrlLimit<protectSafeKw || (ctrlLimit==protectSafeKw && ctrlLimitWatt<protectSafeWatt))
                            {
                            	  ctrlLimit = protectSafeKw;
                            	  ctrlLimitWatt = protectSafeWatt;
                            }
                                                       
                            //�����ʸ澯��ֵ
                            if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                            {
                               //�ѽ���澯״̬
                               if (wkdCtrlConfig[pn-1].wkdAlarm == CTRL_ALARM)
                               {
                                 //�澯ʱ�䵽��,��բһ���ִ�
                                 if (compareTwoTime(wkdCtrlConfig[pn-1].alarmEndTime,sysTime))
                                 {
                                   //��������źţ�����澯�ź�
                                   //���ݸ��ܼ���ĵ���ִ��趨����ִ��趨
                                   tmpRound = 0;
                                   for (j = 0; j < CONTROL_OUTPUT; j++)
                                   {
                                     if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                     {
                                       jumpGate(j+1, 1);
                                       
                                       statusOfGate |= 0x10;    //���ص���

                                       ctrlJumpedStatis(WEEKEND_CTRL,1);
                                       
                                       gateStatus(j+1,GATE_STATUS_ON);
                                       powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                       
                                       //#ifdef ONCE_JUMP_ONE_ROUND
                                        //��¼������բ�ֳ���Ϣ
                                        powerCtrlInQueue(pn,WEEKEND_CTRL,j,(INT8U *)&wkdCtrlConfig[pn-1].wkdLimit);
                                       
                                        break;
                                       //#else
                                       // tmpRound |= 1<<j;
                                       //#endif                                     
                                     }
                                   }
                                   
                                   //#ifndef ONCE_JUMP_ONE_ROUND
                                     //��¼������բ�ֳ���Ϣ
                                   //  powerCtrlInQueue(pn,WEEKEND_CTRL,tmpRound,(INT8U *)&wkdCtrlConfig[pn-1].wkdLimit);                                   
                                   //#endif
  
                                   wkdCtrlConfig[pn-1].ifWkdCtrl = 0xAA;
                                   
                                   //���Ʊ�־��λ���澯��־���
                                   wkdCtrlConfig[pn-1].alarmEndTime.year = 0xFF;
                                   wkdCtrlConfig[pn-1].wkdAlarm = CTRL_JUMPED;      //����բ
  
                                   saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
  			                           saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
  
                                   alarmInQueue(WEEKEND_CTRL,pn);                   //���ݿ�
                                   
                                   nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0); //��һ�ִεȴ�WAIT_NEXT_ROUND����
                               
                                   //���ټ����������������������һ���ܼ���
                                   break;
                                 }
                               }
                               else  //δ����澯״̬
                               {
                                 //����澯ʱ��
                                 tmpRound = 0;
                                 for(j=0;j<CONTROL_OUTPUT;j++)
                                 {
                                    //�����ִ����趨��δ��բ
                                    if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                    {
                                    	 break;
                                    }
                                    tmpRound++;
                                 }
                                 
                               	 if (powerCtrlAlarmTime[j].def==0xaa)
                               	 {
                               	   wkdCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                               	 }
                               	 else     //���û�����ø��ִεĹ��ظ澯ʱ��,Ĭ��Ϊ1����
                               	 {
                               	   wkdCtrlConfig[pn-1].alarmEndTime = nextTime(sysTime, 1, 0);
                               	 }
                               	 gateCloseWaitTime=START_ALARM_WAIT_TIME;           //��ʼ�澯��ʾ��ʱ��
       	                         setBeeper(BEEPER_ON);                              //��������ʾ��
  	                             alarmLedCtrl(ALARM_LED_ON);                        //ָʾ����
                                 
                                 //�����û���ִν�����բ״̬,��������բ״̬��Ϊδ��բ,����������ʽת�볧�ݿ���ɻ���
                                 if (wkdCtrlConfig[pn-1].ifWkdCtrl == 0x00)
                                 {
                                    powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                                 }
                                 //����澯�ź�
                                 wkdCtrlConfig[pn-1].wkdAlarm = CTRL_ALARM;  //�澯
                                 alarmInQueue(WEEKEND_CTRL,pn);              //���ݿ�
                                 
                                 saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
                               }
                            }
                         }
                      }
                      
                      break;  //ÿ��ֻ��բһ���ִ�,��������Ƚ���һ���ִξ��˳�
                    }
                  }
       	      }
       	      else  //��ǰʱ���Ѿ��뿪���ݿ�ʱ��
       	      {
       	         //����ѽ��볧�ݿؿ���״̬(�������ִ���բ)״̬
       	         if (wkdCtrlConfig[pn-1].ifWkdCtrl == 0xAA)
       	         {
       	           ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_AUTO_RELEASE;   //��Ϊ�����Զ����
       	         }
       	      }
       	    }
       	    else  //��ǰʱ���Ѿ��뿪���ݿ�ʱ��
       	    {
       	       //����ѽ��볧�ݿؿ���״̬(�������ִ���բ)״̬
       	       if (wkdCtrlConfig[pn-1].ifWkdCtrl == 0xAA)
       	       {
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("�뿪����ʱ��,�Զ���բ\n");
        	        }
       	          
       	          ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_AUTO_RELEASE;   //��Ϊ�����Զ����
       	       }
       	    }
           	
           	//��λ�Ժ����ʾ����(ֻ����Ͷ��״̬����բ��״̬)
           	if (workSecond==1)
           	{
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,���ң���Ѿ���բ�Ͳ��ٴ����ʼ״̬
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
                     statusOfGate |= 0x10;               //���ص���
     	               gateStatus(j+1, GATE_STATUS_ON);
                     alarmInQueue(WEEKEND_CTRL, pn);     //���ݿ�
     	             } 
     	             else    //ly,2011-03-31,��ӵ�else
     	             {
     	               gateStatus(j+1,GATE_STATUS_OFF);
     	             }
     	           }
     	         }
           	}
          }
  
          //���ݿؽ��
          if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_RELEASE  || ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_AUTO_RELEASE)
          {
              ctrlRelease(pn,WEEKEND_CTRL,wkdCtrlConfig[pn-1].ifWkdCtrl);
              continue;
          }  	      
   	        
   	      //---------------------- ʱ �� �� �� --------------------------	      
          //���ֹ������ȼ�,�������ȼ�����Ͷ��������ȼ����Ʋ�ִ��
   	      if (ctrlRunStatus[pn-1].ifUsePwrCtrl == 0xAA
   	      	  || ctrlRunStatus[pn-1].ifUseObsCtrl == 0xAA
   	      	    || ctrlRunStatus[pn-1].ifUseWkdCtrl == 0xAA)
       	  {
       	    continue;
       	  }
       	  //ʱ�ι���Ͷ��
       	  if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_JUMP_IN)
       	  {
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
    	        printf("\n�ܼ���%dʱ�ι���Ͷ��\n",pn);
    	      }
  
            //��ȡ��ǰʱ��ε�ʱ������,����ʱ����Ч�������ǡ��,������ʱ�ι����ж�
            if ((tmpPeriodCount = getPowerPeriod(sysTime)) == 0)
            {
       	       //����ѽ��볧�ݿؿ���״̬(�������ִ���բ)��澯״̬
       	       if (periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl == 0xAA || periodCtrlConfig[pn-1].ctrlPara.prdAlarm==CTRL_ALARM)
       	       {
       	          ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_AUTO_RELEASE;   //��Ϊ�����Զ����
  
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("�ѽ��볧��,ʱ�ο��Զ���բ\n");
        	        }
       	       }
               continue;
            }
  
            //���ݵ�ǰʱ��, �ܼ����, ������, ʱ��������ȡʱ�ι��ض�ֵ
            periodCtrlLimit = 0;
  
            if (getPowerLimit(pn, periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod, tmpPeriodCount, (INT8U *)&periodCtrlLimit))
            {
                //ÿ��ֻ��բһ���ִ�
               for(j=0;j<CONTROL_OUTPUT;j++)
               {
                 //��������ִ�δ����,ִ�м������
                 if (!((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01) && ((powerCtrlRoundFlag[pn-1].flag >>j) & 0x01))
                 {
                   if (nextRound.year==0x0 || nextRound.month==0x0 || nextRound.day==0x00)
                   {
                   	  nextRound = sysTime;
                   }
  
                   if (compareTwoTime(nextRound,sysTime))   //��һ�ִεȴ�WAIT_NEXT_ROUND����
                   {
                       ctrlLimit = alarmLimit = presentData = 0x00000000;
  	                   ctrlLimitWatt = alarmLimitWatt = presentWatt = 0x0000;
  	                   
  	                   if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
  	                   {
  	                     printf("ʱ�ι��ؼ���\n");
  	                   }
  
                       if (calcData(ctrlReadBuf,periodCtrlLimit,&presentData,NULL,&ctrlLimit,&presentWatt,NULL,&ctrlLimitWatt,ctrlPara.pCtrlIndex, 0x02, pn))
                       {
                          //������ֵ
                          protectSafeKw = countAlarmLimit((INT8U *)&protectLimit,0x2, 0,&protectSafeWatt);
                          //����ֵ�뱣����ֵ�Ƚ�,�������ֵС�ڱ�����ֵ,�����ֵ=������ֵ
                          if (ctrlLimit<protectSafeKw || (ctrlLimit==protectSafeKw && ctrlLimitWatt<protectSafeWatt))
                          {
                             ctrlLimit = protectSafeKw;
                             ctrlLimitWatt = protectSafeWatt;
                          }
                          
                          if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
                          {
                            printf("ʱ�ι���:������ֵ=%0d\n",protectSafeKw);
                            printf("ʱ�ι���:����ֵ=%0d\n",ctrlLimit);
                            printf("ʱ�ι���:��ǰ����=%0d\n",presentData);
                          }
                          
                          //�����ʸ澯��ֵ
                          if (presentData > ctrlLimit || (presentData==ctrlLimit && presentWatt>=ctrlLimitWatt))
                          {
                             //�ѽ���澯״̬
                             if (periodCtrlConfig[pn-1].ctrlPara.prdAlarm == CTRL_ALARM)
                             {
                               //�澯ʱ�䵽��,��բһ���ִ�
                               if (compareTwoTime(periodCtrlConfig[pn-1].ctrlPara.alarmEndTime,sysTime))
                               {
                                 //��������źţ�����澯�ź�
                                 //���ݸ��ܼ���ĵ���ִ��趨����ִ��趨
                                 for (j = 0; j < CONTROL_OUTPUT; j++)
                                 {
                                   if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                   {
                                     jumpGate(j+1, 1);
                                     
                                     statusOfGate |= 0x10;    //���ص���

                                     ctrlJumpedStatis(TIME_CTRL, 1);
                                     
                                     gateStatus(j+1,GATE_STATUS_ON);
                                     powerCtrlRoundFlag[pn-1].ifJumped |= 1<<j;
                                     
                                     //��¼������բ�ֳ���Ϣ
                                     powerCtrlInQueue(pn,TIME_CTRL, j, (INT8U *)&periodCtrlLimit);
                                     
                                     break;
                                   }
                                 }
                                 
                                 periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl = 0xAA;
                                 
                                 //���Ʊ�־��λ���澯��־���
                                 periodCtrlConfig[pn-1].ctrlPara.alarmEndTime.year = 0xFF;
                                 periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_JUMPED;      //����բ
  
    			                       saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
  			                         saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn -1], sizeof(POWERCTRL_ROUND_FLAG));
                             
                                 alarmInQueue(TIME_CTRL,pn);                                  //ʱ�ι���
                                 
                                 nextRound = nextTime(sysTime,WAIT_NEXT_ROUND,0); //��һ�ִεȴ�WAIT_NEXT_ROUND����
                             
                                 //���ټ����������������������һ���ܼ���
                                 break;
                               }
                             }
                             else  //δ����澯״̬
                             {
                               //����澯ʱ��
                               tmpRound = 0;
                               for(j=0;j<CONTROL_OUTPUT;j++)
                               {
                                  //�����ִ����趨��δ��բ
                                  if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x01 && ((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x00)
                                  {
                                  	 break;
                                  }
                                  tmpRound++;
                               }
                             	 if (powerCtrlAlarmTime[j].def==0xaa)
                             	 {
                             	   periodCtrlConfig[pn-1].ctrlPara.alarmEndTime = nextTime(sysTime, powerCtrlAlarmTime[j].alarmTime, 0);
                             	 }
                             	 else     //���û�����ø��ִεĹ��ظ澯ʱ��,Ĭ��Ϊ1����
                             	 {
                             	   periodCtrlConfig[pn-1].ctrlPara.alarmEndTime = nextTime(sysTime, 1, 0);
                             	 }                                  
                               gateCloseWaitTime=START_ALARM_WAIT_TIME;           //��ʼ�澯��ʾ��ʱ��
     	                         setBeeper(BEEPER_ON);                              //��������ʾ��
                               alarmLedCtrl(ALARM_LED_ON);                        //ָʾ����
                               
                               //�����û���ִν�����բ״̬,��������բ״̬��Ϊδ��բ,����������ʽת��ʱ�ι�����ɳɻ���
                               if (periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl == 0x00)
                               {
                                  powerCtrlRoundFlag[pn-1].ifJumped = 0x0;
                               }
                               //����澯�ź�
                               periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_ALARM;  //�澯
                               alarmInQueue(TIME_CTRL,pn);                             //ʱ�ι���
                               
    			                     saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
                             }
                          }
                       }
                    }
                    
                    break;  //ÿ��ֻ��բһ���ִ�,��������Ƚ���һ���ִξ��˳�
                  }
                }
       	    }
       	    else
       	    {
       	       //����ѽ���ʱ�ι��ؿ���״̬(�������ִ���բ)״̬
       	       if (periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl == 0xAA)
       	       {
       	          ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_AUTO_RELEASE;   //��Ϊ�����Զ����
  
        	        if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
        	        {
        	          printf("�뿪����ʱ��,ʱ�ο��Զ���բ\n");
        	        }
       	       }      	    	 
       	    }
           	
           	//��λ�Ժ����ʾ����(ֻ����Ͷ��״̬����բ��״̬)
           	if (workSecond==1)
           	{
       	       for (j = 0; j < CONTROL_OUTPUT; j++)
               {
                 if (((statusOfGate>>j)&0x1)==0)    //ly,2011-04-01,add,���ң���Ѿ���բ�Ͳ��ٴ����ʼ״̬
                 {
                   if ((powerCtrlRoundFlag[pn-1].ifJumped >>j) & 0x01)
                   {
     	               gateStatus(j+1,GATE_STATUS_ON);
                     statusOfGate |= 0x10;            //���ص���
                     alarmInQueue(TIME_CTRL,pn);      //ʱ�ι���
     	             }
     	             else    //ly,2011-03-31,��ӵ�else
     	             {
     	                gateStatus(j+1,GATE_STATUS_OFF);
     	             }
     	           }
     	         }
           	}
           }
  
           //ʱ�ι��ؽ��
           if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_RELEASE   || ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_AUTO_RELEASE)
           {
              ctrlRelease(pn,TIME_CTRL,periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl);
           }
   	    }
    	}
    	
    	balanceComplete = FALSE;
    	
    	usleep(800000);
   }
}

/*******************************************************
��������:countAlarmLimit
��������:�����ݸ�ʽ2��3����Ϊ16������,���ڱȽ�
���ú���:
�����ú���:
�������:
�������:
����ֵ�������ݸ�ʽ2������16����KW��,�����ķ���ֵwatt���ص���С��KW��Wֵ
        �����ݸ�ʽ3
*******************************************************/
INT32U countAlarmLimit(INT8U *data, INT8U dataFormat, INT8U ctrlIndex,INT16U *watt)
{
    INT32U   tmpData;
    INT8U    tmpIndex;
    
    if (dataFormat == 0x02)
    {
      tmpData = (*data&0xF) + (*data>>4&0xF)*10
              + (*(data+1)&0xF)*100;
      
      //����ʵ�ʸ���ϵ��
      if (ctrlIndex != 0x00)
      {
        tmpIndex = (ctrlIndex&0xF) + (ctrlIndex>>4&0x7)*10;
      }
      else
      {
        tmpIndex = 0;
      }
     
      if (ctrlIndex>>7)//�¸�
      {
        tmpData = (tmpData * (100-tmpIndex)) / 100;
      }
      else             //�ϸ�
      {
        tmpData = (tmpData * (100+tmpIndex)) / 100;
      }
      
      *watt=0;
      switch ((*(data+1)>>5&0x07))
      {
        case 0:
        	tmpData *= 10000;
        	break;
        case 1:
        	tmpData *= 1000;
        	break;
        case 2:
        	tmpData *= 100;
        	break;
        case 3:
        	tmpData *= 10;
        	break;
        case 5:
        	*watt = tmpData%10*100;
        	tmpData /= 10;
        	break;
        case 6:
        	*watt = tmpData%100*10;
        	tmpData /= 100;
        	break;
        case 7:
        	*watt = tmpData%1000;
        	tmpData /= 1000;
        	break;
        default:
        	break;
      }
    }

    if (dataFormat == 0x03)
    {
      tmpData = (*data&0xF) + (*data>>4&0xF)*10
               + (*(data+1)&0xF)*100 + (*(data+1)>>4&0xF)*1000
               + (*(data+2)&0xF)*10000 + (*(data+2)>>4&0xF)*100000
               + (*(data+3)&0xF)*1000000;
      
      //����ʵ�ʸ���ϵ��
      if (ctrlIndex != 0x00)
      {
         tmpIndex = (ctrlIndex&0xF) + (ctrlIndex>>4&0x7)*10;
      }
      else
      {
         tmpIndex = 0;
      }
     
      if (ctrlIndex>>7) //�¸�
      {
         tmpData = (tmpData * (100-tmpIndex)) / 100;
      }
      else              //�ϸ�
      {
         tmpData = (tmpData * (100+tmpIndex)) / 100;
      }
      
      //MWh
      if ((*(data+3)>>6&0x01) == 0x01)
      {
        if (tmpData<4294967)
        {
           tmpData *= 1000;
           *watt = 0;
        }
        else  //����4294967*1000�����,��Ҫ���� LY 2008.12.17
        {
           tmpData *= 1000;
           *watt = 0;
        }
      }
    }
    
    return tmpData;
}

/**************************************************
��������:getPresentPowerLimit
��������:��û�ȡָ��ʱ���ʱ������
���ú���:
�����ú���:
�������:1.DATE_TIME limitTime    ָ��ʱ��
�������:INT8U* powerLimit  ʱ�ι��ض�ֵ
����ֵ��ʱ����(���ڻ�ȡʱ�ι��ض�ֵ������)
        ���󷵻�0
***************************************************/
INT8U getPowerPeriod(DATE_TIME  limitTime)
{
    INT8U      count, tmpData, i, j;
    INT8U      hourIndex = limitTime.hour;
    INT8U      minuteIndex = limitTime.minute;
    
    //�������ʱ���ڲ����ṹ�е�����λ��
    count = 0;
    tmpData = 0xFF;
    for (i = 0; i < 12; i++)
    {
      for (j = 0; j < 4; j++)
      {
        if (tmpData != ((ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03))
        {
          if (((ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03)==0x1 || ((ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03)==0x2)
          {
             count++;
          }
          tmpData = (ctrlPara.pCtrlPeriod[i]>>(j*2))&0x03;
        }

        if ((hourIndex*2 + minuteIndex/30) == (i*4 + j))
        {
          i = 12;
          break;
        }
      }
    }
    
    //��ѯ���̳������ʱ�α�����Ϊ"������"�����ز�ѯʧ�ܱ�־
    if (count > 8 || count == 0 || tmpData == 0x00)
    {
       return 0;
    }
    
    return  count;
}

/**************************************************
��������:getPowerLimit
��������:��ȡָ��ʱ�εĹ��ض�ֵ
���ú���:
�����ú���:
�������:1.INT8U     grpPn        �ܼ��������� 1~8����1~8�ܼ���
         2.INT8U     limitPeriod  ʱ�η����� 0~2����1~3ʱ��
         3.INT8U     periodCount  ʱ�κ� 1~8
�������:INT8U* powerLimit  ʱ�ι��ض�ֵ
����ֵ����ѯ���ض�ֵ�о�����ֵtrue
        ��ѯ���ض�ֵû����Ҫ��ֵ false
        ��ѯ���� false
        �������� false
***************************************************/
BOOL getPowerLimit(INT8U grpPn, INT8U limitPeriod, INT8U periodCount, INT8U* powerLimit)
{
   //���ܼ����ָ��ʱ�η���δ����,���ز�ѯʧ�ܱ�־
   if ((periodCtrlConfig[grpPn-1].periodNum>>limitPeriod&0x01) == 0x00)
   {
      return FALSE;
   }
   
   //���ܼ���ָ��ʱ�εĹ���ʱ��δ����,���ز�ѯʧ�ܱ�־
   if ((periodCtrlConfig[grpPn-1].period[limitPeriod].timeCode>>(periodCount-1)&0x01) == 0x00)
   {
      return FALSE;
   }
   
   //���ܼ����ָ��ʱ��ΪδͶ��,���ز�ѯʧ�ܱ�־
   if ((periodCtrlConfig[grpPn-1].ctrlPara.limitPeriod>>(periodCount-1)&0x01) == 0x00)
   {
      return FALSE;
   }

   //����������ѯ���ض�ֵ
   *powerLimit = periodCtrlConfig[grpPn-1].period[limitPeriod].limitTime[periodCount-1][0];
   *(powerLimit+1) = periodCtrlConfig[grpPn-1].period[limitPeriod].limitTime[periodCount-1][1];

   return TRUE;
}

/**************************************************
��������:jumpGate
��������:��բ����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void jumpGate(INT8U line,INT8U onOff)
{
 	 INT8U  buf;
 	 INT32U i;

	 if (line<1 || line>4)
	 {
	 	  return;
	 }
	 
	 if (onOff==1)
	 {
  	 gateJumpTime[line] = 1;  //��·�̵�������բ,��ʼ��ʱ

 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 0);
         printf("jumpGate:��1·��բ\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 0);
         printf("jumpGate:��2·��բ\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 0);
         printf("jumpGate:��3·��բ\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 0);
         printf("jumpGate:��4·��բ\n");
 	 	     break;
 	   }
   }
   else
   {
 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 1);
         printf("jumpGate:��1·��բ\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 1);
         printf("jumpGate:��2·��բ\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 1);
         printf("jumpGate:��3·��բ\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 1);
         printf("jumpGate:��4·��բ\n");
 	 	     break;
 	   }
   }
   
   ioctl(fdOfIoChannel, LOAD_CTRL_CLOCK, 0);
   
 	 buf = 0xc;
 	 buf |= 0xa0;   //�����������
   
   ioctl(fdOfIoChannel, LOAD_CTRL_CLOCK, 1);
   
   for(i=0;i<0x20000;i++)
   {
     ;
   }

 	 buf = 0xc;
 	 buf |= 0x00;   //���������ֹ
   
   //ly,2011-03-24,���������ߵ��µ�ʱ����,��ʱ����/��բ���ɿ�,���ǵ�ʱ������,��ATXMEGA32A4�ϵ�ENֱ�ӵ��ؾͲ����·�����EN��
   //sendXmegaInTimeFrame(TE_CTRL_EN, &buf, 1);

   ioctl(fdOfIoChannel, LOAD_CTRL_CLOCK, 0);
}

/**************************************************
��������:jumpGateNoAction
��������:������բ��ƽ��������Ƽ̵�������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void jumpGateNoAction(INT8U line,INT8U onOff)
{
 	 INT8U  buf;
 	 INT32U i;

	 if (line<1 || line>4)
	 {
	 	  return;
	 }
	 
	 if (onOff==1)
	 {
 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 0);
         printf("jumpGateNoAction:��1·��բ������ʹ��\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 0);
         printf("jumpGateNoAction:��2·��բ������ʹ��\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 0);
         printf("jumpGateNoAction:��3·��բ������ʹ��\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 0);
         printf("jumpGateNoAction:��4·��բ������ʹ��\n");
 	 	     break;
 	   }
   }
   else
   {
 	   switch(line)
 	   {
 	 	   case 1:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_1, 1);
         printf("jumpGateNoAction:��1·��բ�����߽�ֹ\n");
 	 	     break;
 
 	 	   case 2:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_2, 1);
         printf("jumpGateNoAction:��2·��բ�����߽�ֹ\n");
 	 	     break;
 	 	   
 	 	   case 3:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_3, 1);
         printf("jumpGateNoAction:��3·��բ�����߽�ֹ\n");
 	 	     break;
 	 	   
 	 	   case 4:
         ioctl(fdOfIoChannel, LOAD_CTRL_LINE_4, 1);
         printf("jumpGateNoAction:��4·��բ�����߽�ֹ\n");
 	 	     break;
 	   }
   }
}

/**************************************************
��������:gateStatus
��������:բ״ָ̬ʾ
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void gateStatus(INT8U line,INT8U onOff)
{
	 if (line<1 || line>CONTROL_OUTPUT)
	 {
	 	  return;
	 }
	 
	 if (onOff==GATE_STATUS_ON)
	 {
  	 switch(line)
  	 {
  	 	 case 1:
         statusOfGate |= CTRL_STATUS_JUMP_1;
  	 	   break;
  
  	 	 case 2:
         statusOfGate |= CTRL_STATUS_JUMP_2;
  	 	   break;
  	 	   
  	 	 case 3:
         statusOfGate |= CTRL_STATUS_JUMP_3;
  	 	   break;
  	 	   
  	 	 case 4:
         statusOfGate |= CTRL_STATUS_JUMP_4;
  	 	   break;
  	 }
   }
   else
   {
  	 #ifdef TE_CTRL_BOARD_V_1_3
  	  switch(line)
  	  {
  	 	  case 1:
          statusOfGate &= CTRL_STATUS_CLOSE_1|0xf0;
  	 	    break;
  
  	 	  case 2:
          statusOfGate &= CTRL_STATUS_CLOSE_2|0xf0;
  	 	    break;
  	 	   
  	 	  case 3:
  	 	    statusOfGate &= CTRL_STATUS_CLOSE_3|0xf0;
  	 	    break;
  	 	   
  	 	  case 4:
          statusOfGate &= CTRL_STATUS_CLOSE_4|0xf0;
  	 	    break;
  	  }
  	 #else
  	  switch(line)
  	  {
  	 	  case 1:
          statusOfGate &= CTRL_STATUS_CLOSE_1;
  	 	    break;
  
  	 	  case 2:
          statusOfGate &= CTRL_STATUS_CLOSE_2;
  	 	    break;
  	 	   
  	 	  case 3:
  	 	    statusOfGate &= CTRL_STATUS_CLOSE_3;
  	 	    break;
  	 	   
  	 	  case 4:
          statusOfGate &= CTRL_STATUS_CLOSE_4;
  	 	    break;
  	  }
  	 #endif   	  
   }
}

/**************************************************
��������:calcData
��������:��������(��ǰֵ,��ֵ,����ֵ)
���ú���:
�����ú���:
�������:��
�������:
����ֵ��TRUE-�������,FALSE-�����ݻ����ݲ�����
***************************************************/
BOOL calcData(INT8U *buff,INT32U limit,INT32U *powerKw,INT32U *limitKw,INT32U *ctrlKw,INT16U *powerWatt,INT16U *limitWatt,INT16U *ctrlWatt,INT8U index, INT8U format,INT8U pn)
{
	  DATE_TIME tmpTime;
	  
	  switch (format)
	  {
	  	case 0x2:
        if (calcRealPower(buff, pn)==FALSE)
        {
        	 return FALSE;
        }
  	    if (buff[GP_WORK_POWER+1] != 0xee)
  	    {
          *powerKw = buff[GP_WORK_POWER+1];
          *powerKw |= (buff[GP_WORK_POWER+2]&0xF)<<8;
          *powerKw |= (buff[GP_WORK_POWER]&0x10)<<8;
          *powerKw |= (buff[GP_WORK_POWER]&0x07)<<13;
        }
        else
        {
      	  return FALSE;
        }
        break;
      
      case 0x3:
        tmpTime = timeHexToBcd(sysTime);
        if (readMeterData(buff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
	      {
          if (buff[GP_MONTH_WORK+3] != 0xFF || buff[GP_MONTH_WORK+3] != 0xEE)
          {
            *powerKw = buff[GP_MONTH_WORK+3]
                     | buff[GP_MONTH_WORK+4] << 8 
                     | buff[GP_MONTH_WORK+5] << 16 
                     | (((buff[GP_MONTH_WORK]&0x01)<<6)|(buff[GP_MONTH_WORK]&0x10)|(buff[GP_MONTH_WORK+6]&0x0f))<<24;
          }
          else
          {
      	    return FALSE;
          }
        }
        else
        {
        	 return FALSE;
        }
        break;
    }
    
    if (powerKw!=limitKw)
    {
      *powerKw = countAlarmLimit((INT8U*)powerKw, format, 0x00,powerWatt);
    }
    if (limitKw!=NULL)
    {
      if (powerKw!=limitKw)
      {
        *limitKw = countAlarmLimit((INT8U*)&limit, format, index,limitWatt);
      }
      else
      {
        *limitKw = countAlarmLimit((INT8U*)powerKw, format, index,limitWatt);
      }
    }
    if (ctrlKw!=NULL)
    {
      *ctrlKw  = countAlarmLimit((INT8U*)&limit, format, 0x00,ctrlWatt);
    }
    
    return TRUE;
}


/**************************************************
��������:alarmInQueue
��������:�澯�źŽ������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void alarmInQueue(INT8U alarmType,INT8U pn)
{
	  INT8U i;
	  for(i=0;i<ctrlStatus.numOfAlarm;i++)
	  {
	  	 if (ctrlStatus.aQueue[i]==alarmType && ctrlStatus.pn[i]==pn)
	  	 {
	  	 	  return;
	  	 }
	  }
	  ctrlStatus.aQueue[i] = alarmType;
	  ctrlStatus.allPermitClose[i] = 0;
	  ctrlStatus.pn[i] = pn;
	  ctrlStatus.numOfAlarm++;
}

/**************************************************
��������:ctrlRelease
��������:���ƽ������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void ctrlRelease(INT8U pn, INT8U ctrlType,INT8U ifJumped)
{  
   INT8U j;
   INT8U tmpFlag;    //��ʱ��־
   
   //����������
   for (j = 0; j < CONTROL_OUTPUT; j++)
   {
      if (ctrlType==MONTH_CTRL || ctrlType==CHARGE_CTRL)
      {
      	 tmpFlag = electCtrlRoundFlag[pn-1].flag;
      }
      else
      {
      	 tmpFlag = powerCtrlRoundFlag[pn-1].flag;
      }
      
      if (((tmpFlag>>j) & 0x01) == 0x01)
      {
         jumpGate(j+1, 0);
         gateStatus(j+1,GATE_STATUS_OFF);
      }
   }
   
   if (ctrlType==MONTH_CTRL || ctrlType==CHARGE_CTRL)
   {
     electCtrlRoundFlag[pn-1].ifJumped = 0x00;              //��բ����բ��־ȫ�����
		 saveViceParameter(0x04, 48, pn, (INT8U *)&electCtrlRoundFlag[pn -1], sizeof(ELECTCTRL_ROUND_FLAG));
     
     statusOfGate &= 0xdf;    //��ص���
   }
   else
   {
     powerCtrlRoundFlag[pn-1].ifJumped = 0x00;              //��������բ��־ȫ�����
	   saveViceParameter(0x04, 45, pn, (INT8U *)&powerCtrlRoundFlag[pn-1], sizeof(POWERCTRL_ROUND_FLAG));
     statusOfGate &= 0xef;    //���ص���
   }
 
   if (ifJumped==0xaa)
   {
     gateCloseWaitTime=GATE_CLOSE_WAIT_TIME;              //�����բ�澯��ʱ��
     setBeeper(BEEPER_ON);                                //��������ʾ��
     alarmLedCtrl(ALARM_LED_ON);                          //ָʾ����
   }

   switch(ctrlType)
   {
   	  case POWER_CTRL:     //��ǰ�����¸���
        if (ifJumped==0xaa)
        {
           powerDownCtrl[pn-1].pwrDownAlarm = CTRL_CLOSE_GATE; //�����բ
           alarmInQueue(POWER_CTRL,pn);                        //�����¸���
        }
        else
        {
        	 if (powerDownCtrl[pn-1].pwrDownAlarm==CTRL_ALARM)
        	 {
        	 	  powerDownCtrl[pn-1].pwrDownAlarm=CTRL_ALARM_CANCEL;
        	 }
        	 else
        	 {
        	 	  powerDownCtrl[pn-1].pwrDownAlarm = 0;
        	 }
        }
  	    powerDownCtrl[pn-1].ifPwrDownCtrl = 0x00;                 //��Ϊδ�������״̬
        if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_JUMP_IN;       //���ƻ�ԭΪͶ��
           powerDownCtrl[pn-1].pwrDownAlarm=CTRL_ALARM_AUTO_CLOSE;//�Զ���բ
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_RELEASEED;     //���ƽ����ִ��
  	    }
  	    
	      saveParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);  //��ǰ�����¸�����
   	  	break;

   	  case OBS_CTRL:       //Ӫҵ��ͣ��
        if (ifJumped==0xaa)
        {
           obsCtrlConfig[pn-1].obsAlarm = CTRL_CLOSE_GATE;      //�����բ
           alarmInQueue(OBS_CTRL,pn);                           //��ͣ��
        }
        else
        {
        	 if (obsCtrlConfig[pn-1].obsAlarm == CTRL_ALARM)
        	 {
        	 	  obsCtrlConfig[pn-1].obsAlarm = CTRL_ALARM_CANCEL; //�澯ȡ��
        	 }
        	 else
        	 {
        	 	  obsCtrlConfig[pn-1].obsAlarm = 0;
        	 }
        }
  	    obsCtrlConfig[pn-1].ifObsCtrl = 0x00;                   //��Ϊδ�������״̬
        if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_JUMP_IN;     //���ƻ�ԭΪͶ��
           obsCtrlConfig[pn-1].obsAlarm=CTRL_ALARM_AUTO_CLOSE;  //�Զ���բ
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_RELEASEED;   //���ƽ����ִ��
  	    }

	  		saveViceParameter(0x04, 44, pn, (INT8U *)&obsCtrlConfig[pn -1], sizeof(OBS_CTRL_CONFIG));
   	  	break;
   	  	
   	  case WEEKEND_CTRL:   //���ݹ���
        if (ifJumped==0xaa)
        {
          wkdCtrlConfig[pn-1].wkdAlarm = CTRL_CLOSE_GATE;  //�����բ
          alarmInQueue(WEEKEND_CTRL,pn);                   //���ݹ���
        }
        else
        {
          if (wkdCtrlConfig[pn-1].wkdAlarm == CTRL_ALARM)
          {
          	 wkdCtrlConfig[pn-1].wkdAlarm = CTRL_ALARM_CANCEL;  //�澯ȡ��
          }
          else
          {
             wkdCtrlConfig[pn-1].wkdAlarm = 0x0;
          }
        }
 	      wkdCtrlConfig[pn-1].ifWkdCtrl = 0x00;              //��Ϊδ�������״̬

        if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_JUMP_IN;     //���ƻ�ԭΪͶ��
           wkdCtrlConfig[pn-1].wkdAlarm=CTRL_ALARM_AUTO_CLOSE;  //�Զ���բ
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_RELEASEED;   //���ƽ����ִ��
  	    }
	  		saveViceParameter(0x04, 42, pn, (INT8U *)&wkdCtrlConfig[pn -1], sizeof(WKD_CTRL_CONFIG));
   	  	break;
   	  
   	  case TIME_CTRL:   //ʱ�ι���
        if (ifJumped==0xaa)
        {
          periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_CLOSE_GATE;  //�����բ
          alarmInQueue(TIME_CTRL,pn);                                  //���ݹ���
        }
        else
        {
          if (periodCtrlConfig[pn-1].ctrlPara.prdAlarm == CTRL_ALARM)
          {
          	 periodCtrlConfig[pn-1].ctrlPara.prdAlarm = CTRL_ALARM_CANCEL;  //�澯ȡ��
          }
          else
          {
             periodCtrlConfig[pn-1].ctrlPara.prdAlarm = 0x0;
          }
        }
 	      periodCtrlConfig[pn-1].ctrlPara.ifPrdCtrl = 0x00;                 //��Ϊδ�������״̬
        if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_JUMP_IN;      //���ƻ�ԭΪͶ��
           periodCtrlConfig[pn-1].ctrlPara.prdAlarm=CTRL_ALARM_AUTO_CLOSE;//�Զ���բ
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_RELEASEED;   //���ƽ����ִ��
  	    }
  			saveViceParameter(0x04, 41, pn, (INT8U *)&periodCtrlConfig[pn -1], sizeof(PERIOD_CTRL_CONFIG));
   	  	break;
   	  	
   	  case MONTH_CTRL:   //�µ��
        if (ifJumped==0xaa)
        {
          monthCtrlConfig[pn-1].monthAlarm = CTRL_CLOSE_GATE;  //�����բ
          alarmInQueue(MONTH_CTRL,pn);                         //�µ��
        }
        else
        {
          if (monthCtrlConfig[pn-1].monthAlarm == CTRL_ALARM)
          {
          	 monthCtrlConfig[pn-1].monthAlarm = CTRL_ALARM_CANCEL;  //�澯ȡ��
          }
          else
          {
             monthCtrlConfig[pn-1].monthAlarm = 0x0;
          }
        }
 	      monthCtrlConfig[pn-1].ifMonthCtrl = 0x00;                   //��Ϊδ�������״̬
        if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_JUMP_IN;         //���ƻ�ԭΪͶ��
           monthCtrlConfig[pn-1].monthAlarm = CTRL_ALARM_AUTO_CLOSE;//�Զ���բ
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_RELEASEED;       //���ƽ����ִ��
  	    }
  	    
			  saveViceParameter(0x04, 46, pn, (INT8U *)&monthCtrlConfig[pn -1], sizeof(MONTH_CTRL_CONFIG));
   	  	break;
   	  	
   	  case CHARGE_CTRL:   //�����
        if (ifJumped==0xaa)
        {
          chargeCtrlConfig[pn-1].chargeAlarm = CTRL_CLOSE_GATE;  //�����բ
          alarmInQueue(CHARGE_CTRL,pn);                          //�����
        }
        else
        {
          if (chargeCtrlConfig[pn-1].chargeAlarm == CTRL_ALARM)
          {
          	 chargeCtrlConfig[pn-1].chargeAlarm = CTRL_ALARM_CANCEL;  //�澯ȡ��
          }
          else
          {
             chargeCtrlConfig[pn-1].chargeAlarm = 0x0;
          }
        }
 	      chargeCtrlConfig[pn-1].ifChargeCtrl = 0x00;                   //��Ϊδ�������״̬
        if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_AUTO_RELEASE)
        {
  	       ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_JUMP_IN;         //���ƻ�ԭΪͶ��
           chargeCtrlConfig[pn-1].chargeAlarm = CTRL_ALARM_AUTO_CLOSE;//�Զ���բ
        }
        else
        {
  	       ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_RELEASEED;       //���ƽ����ִ��
  	    }
  	    
			  saveViceParameter(0x04, 47, pn, (INT8U *)&chargeCtrlConfig[pn -1], sizeof(CHARGE_CTRL_CONFIG));
   	  	break;   	  	
   }
	 
	 saveParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);    //����Ͷ��״̬��FN04����

   nextRound.year=0x0;                                     //��һ�ִεȴ�ʱ����Ϊ0
}

/**************************************************
��������:electCtrlRecord
��������:ERC7.�����բ/�澯��¼
���ú���:
�����ú���:
�������:type-1,��բ��¼ 2,��ظ澯��¼
�������:
����ֵ��void
***************************************************/
void electCtrlRecord(INT8U type, INT8U pn, INT8U round, INT8U ctrlType,INT8U *totalEnergy,INT8U *limit)
{
  INT8U eventData[19];
  
  if ((type==1 && ((eventRecordConfig.iEvent[0] & 0x40) || (eventRecordConfig.nEvent[0] & 0x40)))
  	 || (type==2 && ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[0] & 0x80)))
  	 )
  {
  	 if (type==1)
  	 {
  	   eventData[0] = 0x7;
  	 }
  	 else
  	 {
  	   eventData[0] = 23;
  	 }
  	 eventData[1] = 19;
  	  
  	 //��բ/�澯ʱ��
  	 eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
  	 eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
  	 eventData[4] = sysTime.hour/10<<4 | sysTime.hour%10;
  	 eventData[5] = sysTime.day/10<<4 | sysTime.day%10;
  	 eventData[6] = sysTime.month/10<<4 | sysTime.month%10;
  	 eventData[7] = sysTime.year/10<<4 | sysTime.year%10;
  	  
  	 //�ܼ���
  	 eventData[8] = pn;
  	  
  	 //��բ/�澯Ͷ���ִ�
  	 eventData[9] = round;
  	 
  	 //������
  	 if (ctrlType==MONTH_CTRL)
  	 {
  	 	 eventData[10] = 0x01;    //�µ��
  	 }
  	 else
  	 {
  	 	 eventData[10] = 0x02;    //�����
  	 }
  	 
  	 //��բʱ������(�ܼӵ�����)
  	 eventData[11] = *totalEnergy++;
  	 eventData[12] = *totalEnergy++;
  	 eventData[13] = *totalEnergy++;
  	 eventData[14] = *totalEnergy;
  	 
  	 //��բʱ��������ֵ
  	 eventData[15] = *limit++;
  	 eventData[16] = *limit++;
  	 eventData[17] = *limit++;
  	 eventData[18] = *limit;

     if (type==1)   //��բ��¼
     {
       if (eventRecordConfig.iEvent[0] & 0x40)
       {
  	     writeEvent(eventData, 19, 1, DATA_FROM_GPRS);   //������Ҫ�¼�����
  	   }
       if (eventRecordConfig.nEvent[0] & 0x40)
       {
  	     writeEvent(eventData, 19, 2, DATA_FROM_LOCAL);  //����һ���¼�����
  	   }
  	 }
  	 else           //��ظ澯��¼
  	 {
       if (eventRecordConfig.iEvent[2] & 0x80)
       {
  	     writeEvent(eventData, 19, 1, DATA_FROM_GPRS);   //������Ҫ�¼�����
  	   }
       if (eventRecordConfig.nEvent[2] & 0x80)
       {
  	     writeEvent(eventData, 19, 2, DATA_FROM_LOCAL);  //����һ���¼�����
  	   }
  	 }
     
     if (debugInfo&PRINT_EVENT_DEBUG)
     {
       printf("electCtrlRecord���������ϱ�\n");
     }

     activeReport3();   //�����ϱ��¼�
  	 
  	 eventStatus[0] = eventStatus[0] | 0x40;
  }
}

/**************************************************
��������:powerCtrlRecord
��������:ERC6.������բ��¼
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void powerCtrlRecord(void)
{
  INT8U  i,j;
  INT8U  eventData[19];
  INT32U eventKw;
  
  for(i=0;i<powerCtrlEventInfor.numOfCtrl;i++)
  {
     if (compareTwoTime(powerCtrlEventInfor.perCtrl[i].freezeTime,sysTime))
     {
        if ((eventRecordConfig.iEvent[0] & 0x20) || (eventRecordConfig.nEvent[0] & 0x20))
        {
        	 eventData[0] = 0x6;
        	 eventData[1] = 17;
        	  
        	 //��բʱ��
        	 eventData[2] = powerCtrlEventInfor.perCtrl[i].ctrlTime.second/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.second%10;
        	 eventData[3] = powerCtrlEventInfor.perCtrl[i].ctrlTime.minute/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.minute%10;
        	 eventData[4] = powerCtrlEventInfor.perCtrl[i].ctrlTime.hour/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.hour%10;
        	 eventData[5] = powerCtrlEventInfor.perCtrl[i].ctrlTime.day/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.day%10;
        	 eventData[6] = powerCtrlEventInfor.perCtrl[i].ctrlTime.month/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.month%10;
        	 eventData[7] = powerCtrlEventInfor.perCtrl[i].ctrlTime.year/10<<4 | powerCtrlEventInfor.perCtrl[i].ctrlTime.year%10;
        	  
        	 //�ܼ���
        	 eventData[8] = powerCtrlEventInfor.perCtrl[i].pn;
        	  
        	 //��բ�ִ�
        	 //#ifdef ONCE_JUMP_ONE_ROUND
        	  eventData[9] = 1<<powerCtrlEventInfor.perCtrl[i].gate;
        	 //#else
        	 // eventData[9] = powerCtrlEventInfor.perCtrl[i].gate;
        	 //#endif
        	 
        	 //�������
        	 switch(powerCtrlEventInfor.perCtrl[i].ctrlType)
        	 {
        	 	  case TIME_CTRL:     //ʱ�ι���
        	 	  	eventData[10] = 0x1;
        	 	  	break;
        	 	  	
        	 	  case WEEKEND_CTRL:  //���ݿ�
        	 	  	eventData[10] = 0x2;
        	 	  	break;
      
        	 	  case OBS_CTRL:     //Ӫҵͣ��
        	 	  	eventData[10] = 0x4;
        	 	  	break;
      
        	 	  case POWER_CTRL:   //��ǰ�����¸���
        	 	  	eventData[10] = 0x8;
        	 	  	break;
        	 }
        	 
        	 //��բʱ����(�ܼӹ���)
        	 eventData[11] = powerCtrlEventInfor.perCtrl[i].ctrlPower[0];
        	 eventData[12] = powerCtrlEventInfor.perCtrl[i].ctrlPower[1];
        	 
        	 //��բ��2min��Ĺ���
  	       if (calcRealPower(ctrlReadBuf, powerCtrlEventInfor.perCtrl[i].pn))
           {
             eventKw = ctrlReadBuf[GP_WORK_POWER+1];
             eventKw |= (ctrlReadBuf[GP_WORK_POWER+2]&0xF)<<8;
             eventKw |= (ctrlReadBuf[GP_WORK_POWER]&0x10)<<8;
             eventKw |= (ctrlReadBuf[GP_WORK_POWER]&0x07)<<13;
        	   eventData[13] = eventKw&0xff;
        	   eventData[14] = eventKw>>8&0xff;
        	 }
        	 else
        	 {
        	   eventData[13] = 0x0;
        	   eventData[14] = 0x0;
        	 }
        	 
        	 //��բʱ���ʶ�ֵ
        	 eventData[15] = powerCtrlEventInfor.perCtrl[i].limit[0];
        	 eventData[16] = powerCtrlEventInfor.perCtrl[i].limit[1];
      
           if (eventRecordConfig.iEvent[0] & 0x20)
           {
        	    writeEvent(eventData, 17 , 1, DATA_FROM_GPRS);   //������Ҫ�¼�����
        	 }
           if (eventRecordConfig.nEvent[0] & 0x20)
           {
        	    writeEvent(eventData, 17 , 2, DATA_FROM_LOCAL);  //����һ���¼�����
        	 }
        	  
        	 eventStatus[0] = eventStatus[0] | 0x20;
           
           if (debugInfo&PRINT_EVENT_DEBUG)
           {
             printf("powerCtrlRecord���������ϱ�\n");
           }

           activeReport3();   //�����ϱ��¼�
        }
        
        //������
        for(j=i;j<powerCtrlEventInfor.numOfCtrl;j++)
        {
         	 powerCtrlEventInfor.perCtrl[j].pn = powerCtrlEventInfor.perCtrl[j+1].pn;
         	 powerCtrlEventInfor.perCtrl[j].ctrlTime = powerCtrlEventInfor.perCtrl[j+1].ctrlTime;
         	 powerCtrlEventInfor.perCtrl[j].freezeTime = powerCtrlEventInfor.perCtrl[j+1].freezeTime;
         	 powerCtrlEventInfor.perCtrl[j].gate = powerCtrlEventInfor.perCtrl[j+1].gate;
         	 powerCtrlEventInfor.perCtrl[j].ctrlType = powerCtrlEventInfor.perCtrl[j+1].ctrlType;
            
         	 powerCtrlEventInfor.perCtrl[j].ctrlPower[0] = powerCtrlEventInfor.perCtrl[j+1].ctrlPower[0];
         	 powerCtrlEventInfor.perCtrl[j].ctrlPower[1] = powerCtrlEventInfor.perCtrl[j+1].ctrlPower[1];
         	 
         	 powerCtrlEventInfor.perCtrl[j].limit[0] = powerCtrlEventInfor.perCtrl[j+1].limit[0];
         	 powerCtrlEventInfor.perCtrl[j].limit[1] = powerCtrlEventInfor.perCtrl[j+1].limit[1];
        }
        powerCtrlEventInfor.numOfCtrl--;
        
        saveParameter(0x05, 5, (INT8U *)&powerCtrlEventInfor, sizeof(POWER_CTRL_EVENT_INFOR));    //�����ֳ���¼״̬��FN05����
        break;
     }
  }
}


/**************************************************
��������:powerCtrlInQueue
��������:�����ֳ���Ϣ�������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void powerCtrlInQueue(INT8U pn,INT8U ctrlType,INT8U gate,INT8U *limit)
{
   INT32U eventDataKw;
   
   //��¼�����ֳ���Ϣ
   if (powerCtrlEventInfor.numOfCtrl>6)
   {
   	 powerCtrlEventInfor.numOfCtrl=0;
   }                                  
   if (powerCtrlEventInfor.numOfCtrl<6)
   {
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].pn = pn;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlTime = sysTime;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].freezeTime = nextTime(sysTime,2,0);
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].gate = gate;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlType = ctrlType;
      
     eventDataKw =ctrlReadBuf[GP_WORK_POWER+1];
     eventDataKw |= (ctrlReadBuf[GP_WORK_POWER+2]&0xF)<<8;
     eventDataKw |= (ctrlReadBuf[GP_WORK_POWER]&0x10)<<8;
     eventDataKw |= (ctrlReadBuf[GP_WORK_POWER]&0x07)<<13;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlPower[0] = eventDataKw&0xff;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].ctrlPower[1] = eventDataKw>>8&0xff;
   	 
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].limit[0] = *limit++;
   	 powerCtrlEventInfor.perCtrl[powerCtrlEventInfor.numOfCtrl].limit[1] = *limit;
   	 
   	 powerCtrlEventInfor.numOfCtrl++;  //���и�����1
   }
   
   //ly,2011-04-21,add
   saveParameter(0x05, 5, (INT8U *)&powerCtrlEventInfor, sizeof(POWER_CTRL_EVENT_INFOR));    //�����ֳ���¼״̬��FN05����
}

/**************************************************
��������:calcRealPower
��������:�����ܼ���ʵʱ����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
BOOL calcRealPower(INT8U *buff,INT8U pn)
{
	  MEASURE_POINT_PARA  tmpPointPara;
	  METER_DEVICE_CONFIG meterConfig;
    INT8U     i,j,k;
    INT8U     tmpPn, direction, sign;
    BOOL      ifFound = FALSE;
    INT32U    powerInt, powerDec, powerSign;
    INT8U     powerQuantity;
    INT32U    dataRawInt, dataRawDec, tmpData;
    
    INT8U     pulsePnData = 0;
    BOOL      buffHasData;  
    DATE_TIME tmpTime;

    #ifdef PULSE_GATHER
     INT16U  counti;
    #endif
    
    powerInt  = 0x0;
    powerDec  = 0x0;
    powerSign = POSITIVE_NUM;
    
    //�����ܼ����ȷ���洢���
  	ifFound = FALSE;
  	for (i = 0; i < totalAddGroup.numberOfzjz; i++)
  	{
  	   if (totalAddGroup.perZjz[i].zjzNo==pn)
  	   {
  	      pn = i;
  	      ifFound = TRUE;
  	      break;
  	   }
    }
    if(ifFound==FALSE)
    {
    	 return FALSE;
    }
    
    //�����ܼӹ���
	  for(i = 0; i < totalAddGroup.perZjz[pn].pointNumber; i++)
    { 
  	  //ȷ��������ţ����򣬷���
      tmpPn = (totalAddGroup.perZjz[pn].measurePoint[i] & 0x3F) + 1;
      direction = totalAddGroup.perZjz[pn].measurePoint[i] & 0x40;
      sign = totalAddGroup.perZjz[pn].measurePoint[i] & 0x80;
      buffHasData = FALSE;
      pulsePnData = 0;
      
   		#ifdef PULSE_GATHER
			 	//�鿴�Ƿ����������������
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //���������Ĳ�����
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==tmpPn)
			    {
			      //P.1�ȳ�ʼ������
            memset(buff,0xee,LENGTH_OF_PARA_RECORD);

				 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
			   	  covertPulseData(j, NULL,NULL,buff);
			   	  	 	    
			      pulsePnData = 1;
			      
            if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
            {
              printf("calcRealPower:ת��������%d������\n",tmpPn);
            }
			   	  
			      buffHasData = TRUE;
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //�����������������
		  {
        if (selectF10Data(tmpPn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
        {
		      if (meterConfig.protocol==AC_SAMPLE)
		      {
		     	  if (ifHasAcModule==TRUE)
		     	  {
			        //A.1�ȳ�ʼ������
              memset(buff,0xee,LENGTH_OF_PARA_RECORD);

			       	//A.2������������������dataBuff��
			       	covertAcSample(buff, NULL, NULL, 1, sysTime);
              
              if (debugInfo&PRINT_LOAD_CTRL_DEBUG)
              {
                printf("calcRealPower:ת��������%d����ֵ\n",tmpPn);
              }
			      	       	  
			     	  buffHasData = TRUE;
		   	    }
		      }
		  	  else
		  	  {
            tmpTime = queryCopyTime(tmpPn);
            buffHasData = readMeterData(buff, tmpPn, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0);
          }
        }
      }
      
      if (buffHasData == TRUE)
      {
        if (buff[POWER_INSTANT_WORK] != 0xEE)
        {
      	  dataRawInt = bcdToHex(buff[POWER_INSTANT_WORK+2]&0x7f);
          dataRawDec = bcdToHex(buff[POWER_INSTANT_WORK]|buff[POWER_INSTANT_WORK+1]<<8);
        }
        else
        {
          if (pulsePnData==1)
          {
            continue;
          }
          else
          {
          	//���������ݲ�������ͳ�����ݱ��Ϊ�޸������ݣ��˳�ѭ��
            powerInt = 0xEE;
            break;              
          }
        }

      	//4-5-3.��ȡ�������Ӧ�����Ʋ���
		    if(selectViceParameter(0x04, 25, tmpPn, (INT8U *)&tmpPointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
    	  {
          if (tmpPointPara.voltageTimes != 0)
          {
            if (tmpPointPara.currentTimes != 0)
            {
   	          dataRawDec *= tmpPointPara.voltageTimes * tmpPointPara.currentTimes;
              dataRawInt *= tmpPointPara.voltageTimes * tmpPointPara.currentTimes;
   	        }
   	        else
   	        {
   	          dataRawDec *= tmpPointPara.voltageTimes;
   	          dataRawInt *= tmpPointPara.voltageTimes;
   	        }
   	      }
   	      else
   	      {
   	        if (tmpPointPara.currentTimes != 0)
   	        {
   	          dataRawDec *= tmpPointPara.currentTimes;
              dataRawInt *= tmpPointPara.currentTimes;
   	        }
   	      }
    	  }
    	  
	      if (sign == 0)   //�ܼ�����+
        {
          if (powerSign == POSITIVE_NUM)  //����+����
          {
            powerInt += dataRawInt;
            powerDec += dataRawDec;
          }
          else    //����-����
          {
        	  //���������ڼ��������Ϊ��
            if (dataRawInt > powerInt || (dataRawInt == powerInt && dataRawDec >= powerDec))
            {
              if (dataRawDec >= powerDec)
              {
                powerDec = dataRawDec - powerDec;
                powerInt = dataRawInt - powerInt;
              }
              else
              {
                powerDec = 10000+ dataRawDec - powerDec;
                powerInt = dataRawInt - powerInt - 1;
              }
            
              powerSign = POSITIVE_NUM;
            }
            else  //������С�ڼ��������Ϊ��
            {
              if (powerDec >= dataRawDec)
              {
                powerDec = powerDec - dataRawDec;
                powerInt = powerInt - dataRawInt;
              }
              else
              {
                powerDec = 10000 + powerDec - dataRawDec;
                powerInt = powerInt -dataRawInt - 1;
              }
          
              powerSign = NEGTIVE_NUM;
            }
          }
 	      }
 	      else  //�ܼ�����-
 	      {
 	        if (powerSign == NEGTIVE_NUM)   //����-����
 	        {
 	          powerInt = powerInt + dataRawInt;
 	          powerDec = powerDec + dataRawDec;
 	        }
 	        else  //����-����
 	        {
 	          //���������ڼ���, ���Ϊ����
 	          if (powerInt > dataRawInt || (powerInt == dataRawInt && powerDec >= dataRawDec))
 	          {
 	            if (powerDec >= dataRawDec)
 	            {
 	              powerDec = powerDec - dataRawDec;
 	              powerInt = powerInt - dataRawInt;
 	            }
 	            else
 	            {
 	              powerDec = 10000 + powerDec - dataRawDec;
 	              powerInt = powerInt - dataRawInt - 1;
 	            }
 	            
 	            powerSign = POSITIVE_NUM;
 	          }
 	          else  //������С�ڼ��������Ϊ����
 	          {
 	            if (dataRawDec >= powerDec)
 	            {
 	              powerDec = dataRawDec - powerDec;
 	              powerInt = dataRawInt - powerInt;
 	            }
 	            else
 	            {
 	              powerDec = 10000 + dataRawDec -powerDec;
 	              powerInt = dataRawInt - powerInt - 1; 
 	            }
 	            
 	            powerSign = NEGTIVE_NUM;
 	          }
 	        }
 	      }
      }
      else
      {
        powerInt = 0xEE;

        break;
      }
    }
    
    //�������ݸ�ʽ
    if (powerInt != 0xEE)
    {
       if (powerDec > 9999)
       {
         do
         {
           powerInt += 1;
           powerDec -= 10000;
         }while (powerDec > 10000);
       }
    
       powerQuantity = dataFormat(&powerInt, &powerDec, 2);
      
       buff[GP_WORK_POWER] = powerQuantity;
       tmpData = hexToBcd(powerInt);
       buff[GP_WORK_POWER+1] = tmpData&0xFF;
       buff[GP_WORK_POWER+2] = tmpData>>8&0xFF;
    }
    else
    {
    	 return FALSE;
    }
    
    return TRUE;
}

/**************************************************
��������:ctrlJumpedStatis
��������:������բ����ͳ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void ctrlJumpedStatis(INT8U ctrlType,INT8U num)
{
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
   DATE_TIME              tmpTime; 
   
   tmpTime = timeHexToBcd(sysTime);
	 if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
   {
   	 initTeStatisRecord(&terminalStatisRecord);
   }
   
 	 switch(ctrlType)
 	 {
 	 	 case REMOTE_CTRL:   //ң��
 	 	   terminalStatisRecord.remoteCtrlJumpedDay += num;
 	 	   break;

 	 	 case CHARGE_CTRL:
 	 	   terminalStatisRecord.chargeCtrlJumpedDay += num;
 	 	   break;
 	 	 
 	 	 case MONTH_CTRL:
 	 	   terminalStatisRecord.monthCtrlJumpedDay += num;
 	 	 	 break;
 	 	 	 
 	 	 case OBS_CTRL:
 	 	 case TIME_CTRL:
 	 	 case WEEKEND_CTRL:
 	 	 case POWER_CTRL:
 	 	   terminalStatisRecord.powerCtrlJumpedDay += num;
 	 }
 	 
   tmpTime = timeHexToBcd(sysTime);
   saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
}

#endif