/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ�����acSample.c
���ߣ�leiyong
�汾��0.9
������ڣ�2010��2��
���������������ļ�
      SSP1�ӿ���������ɼ�оƬͨ��
�����б�
     1.
�޸���ʷ��
  01,10-02-26,Leiyong created.
  02,10-08-04,Leiyong,���Ӳɼ���λ��������
  03,10-09-27,����v1.4�汾��������v1.3�汾III���ն˵�ѹͨ�����û���������,�������ͨ����ֻ��0.1V,
     ����Ȫ��ָ��,���������У��ʱӦ�ý�ADC�Ŵ�ֵд����ٶ���ѹ�Ĵ�����ֵ,�Ѿ�������ĳ�д����ADC
     ��ֵ�ٶ�ֵ.
  04,10-10-23,Leiyong,����,
     1)д��У�����ʱ��ATT7022B����һ��д�����ݼĴ����Ա�.
     2)����У������У��ͼĴ�����ʱ���ִ�����У��ֵ��ATT7022Bһ��,�粻һ��,����У��
  05,11-09-10,Leiyong,����
     1)���ܼ�������
     2)�ж������쳣�͵���������     
  06,11-11-25,Leiyong�޸ĵ��ܼ������ܴ���,�������Ĵ���ֵΪ0xffffff��ʱ��,�����ʹ�����,����������
***************************************************/

#include "hardwareConfig.h"

#include "convert.h"
#include "msSetPara.h"
#include "teRunPara.h"
#include "dataBase.h"
#include "copyMeter.h"

#include "att7022b.h"


//��������
BOOL   ifHasAcModule;   //�Ƿ��н���ģ��

#ifdef AC_SAMPLE_DEVICE

INT8U  readCheckData;   //��ȡУ�����
INT32U realAcData[TOTAL_COMMAND_REAL_AC];      //��������ʵʱ����
INT8U  realTableAC[TOTAL_COMMAND_REAL_AC] = {
             0x01,      //A���й�����
             0x02,      //B���й�����
             0x03,      //C���й�����
             0x04,      //�����й�����
             0x05,      //A���޹�����
             0x06,      //B���޹�����
             0x07,      //C���޹�����
             0x08,      //�����޹�����
             0x0D,      //A���ѹ��Чֵ
             0x0E,      //B���ѹ��Чֵ
             0x0F,      //C���ѹ��Чֵ
             0x10,      //A�������Чֵ
             0x11,      //B�������Чֵ
             0x12,      //C�������Чֵ
             0x14,      //A�๦������
             0x15,      //B�๦������
             0x16,      //C�๦������
             0x17,      //���๦������
             0x09,      //A�����ڹ���
             0x0a,      //A�����ڹ���
             0x0b,      //A�����ڹ���
             0x0c,      //A�����ڹ���
             0x18,      //A���ѹ��������
             0x19,      //B���ѹ��������
             0x1a,      //C���ѹ��������
             0x1b,      //�����ѹ��������
             0x5c,      //UaUb��ѹ�н�
             0x5e,      //UbUc��ѹ�н�
             0x5d,      //UaUc��ѹ�н�
             0x3e,      //У������У���1
             0x5f,      //У������У���2
             0x13,      //ABC�����ʸ���͵���Чֵ ly,2011-05-25,Ŀǰ�������û�нӻ������������,�����ֵ�����������
             0x2c,      //��־״̬�Ĵ��� ly,2011-08-04,��������⹦��
             0x3d,      //�й����޹����ʷ���Ĵ��� ly,2011-08-04,��ӵ��������⹦��
             //0x31,      //A���й�����Epa2
             //0x32,      //B���й�����Epb2
             //0x33,      //C���й�����Epc2
             //0x34,      //�����й�����Ept2
             //0x35,      //A���޹�����Eqa2
             //0x36,      //B���޹�����Eqb2
             //0x37,      //C���޹�����Eqc2
             //0x38,      //�����޹�����Eqt2
             0x63,      //���������й�����PosEpt2
             0x67,      //���෴���й�����NegEpt2
             0x6b,      //���������޹�����PosEqt2
             0x6f,      //���෴���޹�����NegEqt2
             
             0x60,      //A�������й�����PosEpa2
             0x64,      //A�෴���й�����NegEpa2
             0x68,      //A�������޹�����PosEqa2
             0x6c,      //A�෴���޹�����NegEqa2
             0x61,      //B�������й�����PosEpb2
             0x65,      //B�෴���й�����NegEpb2
             0x69,      //B�������޹�����PosEqb2
             0x6d,      //B�෴���޹�����NegEqb2
             0x62,      //C�������й�����PosEpc2
             0x66,      //C�෴���й�����NegEpc2
             0x6a,      //C�������޹�����PosEqc2
             0x6e,      //C�෴���޹�����NegEqc2
             };

INT8U  acReqTimeBuf[LENGTH_OF_REQ_RECORD];

INT8U  nowAcItem=0;                                      //��ǰ��ȡ�Ľ�������������Ŀ
INT8U  tmpAcI, checkSumAc;
INT8U  acBuffer[26];
INT8U  acReadBuffer[4];
INT32U checkSum3e, checkSum5f;                           //У������У���1��2
INT8U  countPhase=0;

INT8U sspWrite(INT8U cmd,INT32U data);

/***************************************************
��������:resetAtt7022b
��������:��λAT7022B�Ҹ�������·�У�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void resetAtt7022b(BOOL ifCheckMeter)
{   
   INT32U i;
   
   #ifdef JZQ_CTRL_BOARD_V_0_3
   	ifHasAcModule = FALSE;
   #else
    if (!ioctl(fdOfSample,READ_HAS_AC_SAMPLE,0))
    {
   	   ifHasAcModule = TRUE;
    }
    else
    {
   	   ifHasAcModule = FALSE;
   	   
   	   printf("�޽��ɵ�Ԫ\n");
    }
   #endif

   if (ifHasAcModule==TRUE)
   {
     //��λATT7022B
	   ioctl(fdOfSample,RESET_ATT7022B,0);
   
     //����Ϊ��������ģʽ
	   ioctl(fdOfSample,SET_AC_MODE,1);

     //�ȴ�ATT7022B״̬����
     for(i=0;i<100000;i++)
     {
   	   if (ioctl(fdOfSample,READ_SIG_OUT_VALUE,1)==0)
   	   {
   	      break;
   	   }
     }
   
     if (ifCheckMeter==TRUE)
     {
        sspCheckMeter();
     }
     else
     {
     	 #ifdef JZQ_CTRL_BOARD_V_1_4
        //ADC����
        if (sspWrite(0x3f | 0x80, 0x465501)==1)
        {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("ATT7022BУ��ֵд��ɹ�(ADC����Ϊ4��)\n");
          }
        }
        delayTime(2000);
     	 #endif

     	 #ifdef TE_CTRL_BOARD_V_1_3
        //ADC����
        if (sspWrite(0x3f | 0x80, 0x465501)==1)
        {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("ATT7022BУ��ֵд��ɹ�(ADC����Ϊ4��)\n");
          }
        }
        delayTime(2000);
     	 #endif
     }
   }
}

/***************************************************
��������:decideAcPhaseOrder
��������:�жϽ������� �¼�
���ú���:
�����ú���
�������:
�������:
����ֵ����
***************************************************/
void decideAcPhaseOrder(void)
{
  METER_STATIS_EXTRAN_TIME meterStatisRecord;   //һ����ͳ���¼�����
  INT8U                    acParaData[LENGTH_OF_PARA_RECORD];
  INT8U                    copyEnergyData[LENGTH_OF_ENERGY_RECORD];
	INT16U                   acPn;                               //���ɲ������
	DATE_TIME                tmpTime;
	INT8U                    eventData[50];
	INT8U                    saveStatisData = 0;

 	//�����ý��ɲ�����
 	if ((acPn=findAcPn())!=0)
 	{
	  tmpTime = sysTime;
    searchMpStatis(tmpTime, &meterStatisRecord, acPn, 1);  //��ʱ���޹���
    memset(acParaData, 0xee, LENGTH_OF_PARA_RECORD);
    memset(copyEnergyData, 0xee, LENGTH_OF_ENERGY_RECORD);
    
    covertAcSample(acParaData, copyEnergyData, NULL, 1, sysTime);

    //ERC09,������·�쳣[����]
    if ((eventRecordConfig.iEvent[1]&0x01)||(eventRecordConfig.nEvent[1] & 0x01))
    {
	    tmpTime = timeHexToBcd(sysTime);

	    //A���й����ʷ���
	    if (realAcData[33]&0x01)
	    {
        //A�෴����δ��¼�¼�,��¼
  	    if ((meterStatisRecord.currentLoop&0x1)==0x00)
  	    {
          meterStatisRecord.currentLoop |= 0x01;   //�÷����־
          cLoopEvent(acParaData, copyEnergyData, 0x01, acPn, 0x03, 0x80, tmpTime);
          saveStatisData = 1;
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,A���й����ʷ�������δ��¼,��¼�¼�\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,A���й����ʷ����Ѽ�¼\n", acPn);
          }
  	    }
	    }
	    else
	    {
        //A��δ��������,�������������������,��Ӧ��¼�ָ��¼�
  	    if ((meterStatisRecord.currentLoop&0x1)==0x01)
  	    {
          meterStatisRecord.currentLoop &= 0xfe;     //��������־
          cLoopEvent(acParaData, copyEnergyData, 0x01, acPn, 0x03, 0, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,A���й����ʷ���ָ���δ��¼,��¼�¼�\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,A������\n",acPn);
          }
  	    }
	    }
	    
	    //B���й����ʷ���
	    if (realAcData[33]&0x02)
	    {
        //B�෴����δ��¼�¼�,��¼
  	    if ((meterStatisRecord.currentLoop&0x2)==0x00)
  	    {
          meterStatisRecord.currentLoop |= 0x02;   //�÷����־
          cLoopEvent(acParaData, copyEnergyData, 0x02, acPn, 0x03, 0x80, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,B���й����ʷ�������δ��¼,��¼�¼�\n",acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,B���й����ʷ����Ѽ�¼\n",acPn);
          }
  	    }
	    }
	    else
	    {
        //B��δ��������,�������������������,��Ӧ��¼�ָ��¼�
  	    if ((meterStatisRecord.currentLoop&0x2)==0x02)
  	    {
          meterStatisRecord.currentLoop &= 0xfd;     //��������־
          cLoopEvent(acParaData, copyEnergyData, 0x02, acPn, 0x03, 0, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,B���й����ʷ���ָ���δ��¼,��¼�¼�\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,B������\n", acPn);
          }
  	    }
	    }
	    
	    //C���й����ʷ���
	    if (realAcData[33]&0x04)
	    {
        //C�෴����δ��¼�¼�,��¼
  	    if ((meterStatisRecord.currentLoop&0x4)==0x00)
  	    {
          meterStatisRecord.currentLoop |= 0x04;   //�÷����־
          cLoopEvent(acParaData, copyEnergyData, 0x04, acPn, 0x03, 0x80, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,C���й����ʷ�������δ��¼,��¼�¼�\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,C���й����ʷ����Ѽ�¼\n", acPn);
          }
  	    }
	    }
	    else
	    {
        //C��δ��������,�������������������,��Ӧ��¼�ָ��¼�
  	    if ((meterStatisRecord.currentLoop&0x4)==0x04)
  	    {
          meterStatisRecord.currentLoop &= 0xfb;     //��������־
          cLoopEvent(acParaData, copyEnergyData, 0x04, acPn, 0x03, 0, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,C���й����ʷ���ָ���δ��¼,��¼�¼�\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("������·�쳣:���ɲ�����%d,C������\n", acPn);
          }
  	    }
	    }
    }

    //ERC11,�����쳣
    if ((eventRecordConfig.iEvent[1]&0x04)||(eventRecordConfig.nEvent[1] & 0x04))
    {
   	  //�����ѹ��������������
      if ((realAcData[32]&0x08) || (realAcData[32]&0x10))
      {
         //��ѹ������������������δ��¼�¼�,��¼
   	    if ((meterStatisRecord.mixed&0x4)==0x00)
   	    {
          meterStatisRecord.mixed |= 0x04;         //�������쳣��־
          eventData[9] = 0x1;
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("�����쳣:���ɲ�����%d,�����쳣������δ��¼,��¼�¼�\n", acPn);
          }
   	    }
   	    else
   	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("�����쳣:���ɲ�����%d�����쳣���Ѽ�¼\n", acPn);
          }
   	    }
      }
      else
      {
         //��ѹ����������������,�������������������,��Ӧ��¼�ָ��¼�
   	    if ((meterStatisRecord.mixed&0x4)==0x04)
   	    {
           meterStatisRecord.mixed &= 0xfb;   //��������쳣��־
  
           eventData[9] = 0x2;
           saveStatisData = 1;

           if (debugInfo&PRINT_AC_SAMPLE)
           {
             printf("�����쳣:���ɲ�����%d,�����쳣�ָ���δ��¼,��¼�¼�\n", acPn);
           }
   	    }
   	    else
   	    {
           if (debugInfo&PRINT_AC_SAMPLE)
           {
             printf("�����쳣:���ɲ�����%d,��������\n", acPn);
           }
   	    }
      }
      
      if (eventData[9]==0x1 || eventData[9]==0x02)
      {
     	  eventData[0] = 11;
     	  eventData[1] = 27;
     	  tmpTime = timeHexToBcd(sysTime);
     	  eventData[2] = tmpTime.second;
     	  eventData[3] = tmpTime.minute;
     	  eventData[4] = tmpTime.hour;
     	  eventData[5] = tmpTime.day;
     	  eventData[6] = tmpTime.month;
     	  eventData[7] = tmpTime.year;
     	  
     	  eventData[8] = acPn&0xff;
     	  
     	  if (eventData[9]==1)
     	  {
   	      eventData[9] = (acPn>>8&0xff) | 0x80;      	      
     	  }
     	  else
     	  {
     	  	eventData[9] = (acPn>>8&0xff);
     	  }
		    
		    //A���ѹ��λ��
		    eventData[10] = acParaData[PHASE_ANGLE_V_A];
		    eventData[11] = acParaData[PHASE_ANGLE_V_A+1];

		    //B���ѹ��λ��
		    eventData[12] = acParaData[PHASE_ANGLE_V_B];
		    eventData[13] = acParaData[PHASE_ANGLE_V_B+1];
		       
		    //C���ѹ��λ��
		    eventData[14] = acParaData[PHASE_ANGLE_V_C];
		    eventData[15] = acParaData[PHASE_ANGLE_V_C+1];
		     
		    //A�������λ��
		    eventData[16] = acParaData[PHASE_ANGLE_C_A];
		    eventData[17] = acParaData[PHASE_ANGLE_C_A+1];

		    //B�������λ��
		    eventData[18] = acParaData[PHASE_ANGLE_C_B];
		    eventData[19] = acParaData[PHASE_ANGLE_C_B+1];
		       
		    //C�������λ��
		    eventData[20] = acParaData[PHASE_ANGLE_C_C];
		    eventData[21] = acParaData[PHASE_ANGLE_C_C+1];

        //����ʱ�ĵ��ܱ������й��ܵ���ʾֵ,���ɲ�����δ��
        eventData[22] = 0xee;
        eventData[23] = 0xee;
        eventData[24] = 0xee;
        eventData[25] = 0xee;
        eventData[26] = 0xee;
  
     	  if (eventRecordConfig.iEvent[1]&0x04)
     	  {
     	    writeEvent(eventData, 27, 1, DATA_FROM_GPRS);
     	  }
     	  
     	  if (eventRecordConfig.nEvent[1]&0x04)
     	  {
     	    writeEvent(eventData, 27, 2, DATA_FROM_LOCAL);
     	  }
     	  
     	  eventStatus[1] = eventStatus[1] | 0x04;
      }
    }
    
    if (saveStatisData==1)
    {
     	//�洢���ɲ�����ͳ������
      saveMeterData(acPn, 1, sysTime, (INT8U *)&meterStatisRecord, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
    }
 	}
 	else
 	{
 		if (debugInfo&PRINT_AC_SAMPLE)
 		{
 			 printf("�жϽ������� - δ���ý��ɲ�����\n");
 		}
 	}
}

/***************************************************
��������:decideQuadrant
��������:�ж����ĸ�����
���ú���:
�����ú���
�������:
�������:
����ֵ������ֵ(1,2,3,4)
***************************************************/
INT8U decideQuadrant(void)
{
  INT8U tmpQuadrant=0;
  
  if (realAcData[3]>>23 & 0x1)  //�й����ʷ���Ϊ��
  {
    if (realAcData[7]>>23 & 0x1)//�޹����ʷ���Ϊ��
    {
  	  tmpQuadrant = 3;
    }
    else                        //�޹����ʷ���Ϊ��
    {
  	  tmpQuadrant = 2;
    }
  }
  else                          //�й����ʷ���Ϊ��
  {
    if (realAcData[7]>>23 & 0x1)//�޹����ʷ���Ϊ��
    {
  	  tmpQuadrant = 4;
    }
    else                        //�޹����ʷ���Ϊ��
    {
  	  tmpQuadrant = 1;
    }
  }
  
  //bug,2011-11-25,����return,ԭ��δ��,�������޵�����ͳ�Ʋ���
  return tmpQuadrant;
}

/***************************************************
��������:updateVision
��������:����ĳ��ʾֵ
���ú���:
�����ú���
�������:vBuf    - ����
         offset - ƫ����
         added  - ������
�������:
����ֵ����
***************************************************/
void updateVision(INT8U *vBuf, INT16U offset, INT32U added)
{
	 INT32U visionInt;
	 INT16U visionDec;   
   
   visionInt = vBuf[offset+0] | vBuf[offset+1]<<8 | vBuf[offset+2]<<16;
   visionDec = vBuf[offset+3] | vBuf[offset+4]<<8;
   
   if (debugInfo&PRINT_AC_VISION)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d:����ǰ,visionInt=%d,visionDec=%d,��������ֵ=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, visionInt, visionDec, added);
   }
        
   visionDec += added;
   visionInt += visionDec/3200;
   visionDec %= 3200;
    	  
   //����
   vBuf[offset+0] = visionInt&0xff;
   vBuf[offset+1] = visionInt>>8&0xff;
   vBuf[offset+2] = visionInt>>16&0xff;
   
   //С��
   vBuf[offset+3] = visionDec&0xff;
   vBuf[offset+4] = visionDec>>8&0xff;

   if (debugInfo&PRINT_AC_VISION)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d:���º�, visionInt=%d, visionDec=%d,ƫ��=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, visionInt, visionDec, offset);
   }
}

/***************************************************
��������:recordVisionReq
��������:��¼����ʾֵ������
���ú���:
�����ú���
�������:
�������:
����ֵ����
***************************************************/
void recordVisionReq(void)
{
	INT8U  i;
	BOOL   ifChanged=FALSE;
	INT8U  energyVision[SIZE_OF_ENERGY_VISION];
	INT8U  tariff;           //���ʺ�
	INT8U  quadrant;
	INT16U offset;
	INT32U tmpData, tmpDataBak, tmpDataBefore;
	INT16U j;
  INT32U totalVision[4];   //��ʾֵ�仯��


  //���ҷ���
  tariff = 0xee;   //����Ϊ��Ч����
  if (sysTime.minute<30)
  {
   	if (periodTimeOfCharge[sysTime.hour*2]<4)
   	{
   	  tariff = periodTimeOfCharge[sysTime.hour*2];
   	}
  }
  else
  {
   	if (periodTimeOfCharge[sysTime.hour*2+1]<4)
   	{
   	  tariff = periodTimeOfCharge[sysTime.hour*2+1];
   	}
  }

  //�����й�����
  ifChanged=FALSE;  
  for(i=0;i<2;i++)
  {
    if (i==0)
    {
      tmpData = realAcData[3];
    }
    else
    {
      tmpData = realAcData[7];
    }
    
    if (tmpData!=0xffffff)
    {
      //�������λΪ1,��Ϊ����
      if (tmpData & 0x800000)
      {
        //�����뻹ԭΪԭ��ֵ�ٽ��м���
        tmpData = (~tmpData & 0x7fffff)+1;
        if (i==0)
        {
          offset = REQ_NEGTIVE_WORK_OFFSET;     //�����й�
        }
        else
        {
          offset = REQ_NEGTIVE_NO_WORK_OFFSET;  //�����޹�
        }
      }
      else
      {
        if (i==0)
        {
          offset = REQ_POSITIVE_WORK_OFFSET;    //�����й�
        }
        else
        {
          offset = REQ_POSITIVE_NO_WORK_OFFSET; //�����޹�      	 
        }
      }
      
      tmpData = hexDivision(tmpData,times2(6)*1000,4);
      tmpDataBak = tmpData;
      tmpData = bcdToHex(tmpData);
      
      tmpDataBefore =  bcdToHex(acReqTimeBuf[offset] | acReqTimeBuf[offset+1]<<8 | acReqTimeBuf[offset+2]<<16);
      
      //��
      if (tmpData>tmpDataBefore)
      {
        acReqTimeBuf[offset]   = tmpDataBak & 0xff;
        acReqTimeBuf[offset+1] = tmpDataBak>>8 & 0xff;
        acReqTimeBuf[offset+2] = tmpDataBak>>16 & 0xff;
        
        acReqTimeBuf[offset+27+0] = hexToBcd(sysTime.minute);
        acReqTimeBuf[offset+27+1] = hexToBcd(sysTime.hour);
        acReqTimeBuf[offset+27+2] = hexToBcd(sysTime.day);
        acReqTimeBuf[offset+27+3] = hexToBcd(sysTime.month);
        acReqTimeBuf[offset+27+4] = hexToBcd(sysTime.year);
        
        ifChanged = TRUE;
      }
      
      //����
      if (tariff<4)
      {
        tmpDataBefore = bcdToHex(acReqTimeBuf[offset+3+tariff*3+0] | acReqTimeBuf[offset+3+tariff*3+1]<<8 | acReqTimeBuf[offset+3+tariff*3+2]<<16);
        if (tmpData>tmpDataBefore)
        {
          acReqTimeBuf[offset+3+tariff*3]   = tmpDataBak & 0xff;
          acReqTimeBuf[offset+3+tariff*3+1] = tmpDataBak>>8 & 0xff;
          acReqTimeBuf[offset+3+tariff*3+2] = tmpDataBak>>16 & 0xff;
        
          acReqTimeBuf[offset+27+5+tariff*5+0] = hexToBcd(sysTime.minute);
          acReqTimeBuf[offset+27+5+tariff*5+1] = hexToBcd(sysTime.hour);
          acReqTimeBuf[offset+27+5+tariff*5+2] = hexToBcd(sysTime.day);
          acReqTimeBuf[offset+27+5+tariff*5+3] = hexToBcd(sysTime.month);
          acReqTimeBuf[offset+27+5+tariff*5+4] = hexToBcd(sysTime.year);
          
          ifChanged = TRUE;
        }
      }
    }
  }
  
  if (ifChanged==TRUE)
  {
    updateAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);
  }
  
  //����ʾֵ
	ifChanged = FALSE;
	for(i=34;i<50;i++)
  {
	  if (realAcData[i]>0 && realAcData[i]!=0xffffff && realAcData[i]<200)
	  {
	  	ifChanged=TRUE;
	  	break;
	  }
	}
	
	if (ifChanged==TRUE)
	{
		//��ȡ����ʾֵ
		readAcVision(energyVision, sysTime, ENERGY_DATA);
    
    totalVision[0] = 0;  //�������й�
	  if (realAcData[38]>0 && realAcData[38]!=0xffffff && realAcData[38]<200)
	  {
      totalVision[0] += realAcData[38];
	  }
	  if (realAcData[42]>0 && realAcData[42]!=0xffffff && realAcData[42]<200)
	  {
      totalVision[0] += realAcData[42];
	  }
	  if (realAcData[46]>0 && realAcData[46]!=0xffffff && realAcData[46]<200)
	  {
      totalVision[0] += realAcData[46];
	  }

    totalVision[1] = 0;  //�����޹�
	  if (realAcData[39]>0 && realAcData[39]!=0xffffff && realAcData[39]<200)
	  {
      totalVision[1] += realAcData[39];
	  }
	  if (realAcData[43]>0 && realAcData[43]!=0xffffff && realAcData[43]<200)
	  {
      totalVision[1] += realAcData[43];
	  }
	  if (realAcData[47]>0 && realAcData[47]!=0xffffff && realAcData[47]<200)
	  {
      totalVision[1] += realAcData[47];
	  }
    
    totalVision[2] = 0;  //�����й�
	  if (realAcData[40]>0 && realAcData[40]!=0xffffff && realAcData[40]<200)
	  {
      totalVision[2] += realAcData[40];
	  }
	  if (realAcData[44]>0 && realAcData[44]!=0xffffff && realAcData[44]<200)
	  {
      totalVision[2] += realAcData[44];
	  }
	  if (realAcData[48]>0 && realAcData[48]!=0xffffff && realAcData[48]<200)
	  {
      totalVision[2] += realAcData[48];
	  }

    totalVision[3] = 0;  //�����޹�
	  if (realAcData[41]>0 && realAcData[41]!=0xffffff && realAcData[41]<200)
	  {
      totalVision[3] += realAcData[41];
	  }
	  if (realAcData[45]>0 && realAcData[45]!=0xffffff && realAcData[45]<200)
	  {
      totalVision[3] += realAcData[45];
	  }
	  if (realAcData[49]>0 && realAcData[49]!=0xffffff && realAcData[49]<200)
	  {
      totalVision[3] += realAcData[49];
	  }
    
	  for(i=0; i<4; i++)
    {
    	if (totalVision[i]>0)
    	{
      	if (debugInfo&PRINT_AC_VISION)
      	{
    	    printf("������=%d,ƫ��=%d\n", i, i*25);
    	  }
    	  
    	  //��
    	  updateVision(energyVision, i*25, totalVision[i]);
    	  
     	  //�ַ���,��ǰֻ����4������
     	  if (tariff<4)
     	  {
      	  if (debugInfo&PRINT_AC_VISION)
      	  {
    	      printf("������=%d,����=%d,ƫ��=%d\n", i, tariff, i*25+5+tariff*5);
    	    }
    	    
    	    updateVision(energyVision, i*25+5+tariff*5, totalVision[i]);
     	  }
     	  
     	  if (i==2 || i==3)
     	  {
          offset = 0;
          
          quadrant = decideQuadrant();        	
        	if (quadrant==1)
        	{
  	        offset = QUA_1_EQT;
        	}
        	if (quadrant==4)
        	{
  	        offset = QUA_4_EQT;
        	}
        	if (quadrant==2)
        	{
  	        offset = QUA_2_EQT;
        	}
        	if (quadrant==3)
        	{
  	        offset = QUA_3_EQT;
        	}
          
          if (offset>0)
          {
      	    if (debugInfo&PRINT_AC_VISION)
      	    {
    	        printf("������=%d,�޹���,ƫ��=%d\n", i, offset);
    	      }
    	      
            updateVision(energyVision, offset, totalVision[i]);
     	      //�ַ���,��ǰֻ����4������
     	      if (tariff<4)
     	      {
      	      if (debugInfo&PRINT_AC_VISION)
      	      {
    	          printf("������=%d,�޹�,����=%d,ƫ��=%d\n", i, tariff, offset+5+tariff*5);
    	        }
    	        
    	        updateVision(energyVision, offset+5+tariff*5, totalVision[i]);
     	      }
     	    }
     	  }
     	}
    }


	  /*
	  for(i=34; i<38; i++)
    {
	    if (realAcData[i]>0 && realAcData[i]!=0xffffff && realAcData[i]<200)
	    {
	    	if (debugInfo&PRINT_AC_VISION)
	    	{
    	    printf("������=%d,ƫ��=%d\n", i, (i-34)*25);
    	  }
    	  
    	  //��
    	  updateVision(energyVision, (i-34)*25, realAcData[i]);
    	  
     	  //�ַ���,��ǰֻ����4������
     	  if (tariff<4)
     	  {
	    	  if (debugInfo&PRINT_AC_VISION)
	    	  {
    	      printf("������=%d,����=%d,ƫ��=%d\n", i, tariff, (i-34)*25+5+tariff*5);
    	    }
    	    
    	    updateVision(energyVision, (i-34)*25+5+tariff*5, realAcData[i]);
     	  }
     	  
     	  if (i==36 || i==37)
     	  {
          offset = 0;
          quadrant = decideQuadrant();
          if (i==36)   //�����޹�
          {
          	if (quadrant==1)
          	{
    	        offset = QUA_1_EQT;
          	}
          	if (quadrant==4)
          	{
    	        offset = QUA_4_EQT;
          	}
          }
          if (i==37)   //�����޹�
          {
          	if (quadrant==2)
          	{
    	        offset = QUA_2_EQT;
          	}
          	if (quadrant==3)
          	{
    	        offset = QUA_3_EQT;
          	}
          }
          
          if (offset>0)
          {
	    	    if (debugInfo&PRINT_AC_VISION)
	    	    {
    	        printf("������=%d,�޹���,ƫ��=%d\n", i, offset);
    	      }
    	      
            updateVision(energyVision, offset, realAcData[i]);
     	      //�ַ���,��ǰֻ����4������
     	      if (tariff<4)
     	      {
	    	      if (debugInfo&PRINT_AC_VISION)
	    	      {
    	          printf("������=%d,�޹�,����=%d,ƫ��=%d\n", i, tariff, offset+5+tariff*5);
    	        }
    	        
    	        updateVision(energyVision, offset+5+tariff*5, realAcData[i]);
     	      }
     	    }
     	  }
    	}
    }
    */
    
    //A,B,C�������й��������޹��������й��������޹�
	  for(i=38; i<50; i++)
    {
	    if (realAcData[i]>0 && realAcData[i]!=0xffffff && realAcData[i]<200)
	    {
	    	if (debugInfo&PRINT_AC_VISION)
	    	{
    	    printf("������=%d,ƫ��=%d\n", i, POS_EPA+(i-38)*5);
    	  }
    	  
    	  updateVision(energyVision, POS_EPA+(i-38)*5, realAcData[i]);
    	}
    }
    
    //���½���ʾֵ
    updateAcVision(energyVision,sysTime,ENERGY_DATA);
	}
}

/***************************************************
��������:readAcChipData
��������:������оƬ����
���ú���:
�����ú���
�������:
�������:
����ֵ����
***************************************************/
void readAcChipData(void)
{   
   INT16U i;
   INT16U frameTail00, tmpHead00;

   acReadBuffer[0] = realTableAC[nowAcItem];
   read(fdOfSample,&acReadBuffer,3);
   realAcData[nowAcItem] = acReadBuffer[0]<<16 | acReadBuffer[1]<<8 | acReadBuffer[2];
   
   //ר��III���ն˽���Ӳ��V1.2,���ڹ�ź�����ݵ�ʱ���Ƿ������,��˽����ݷ�����
   #ifndef PLUG_IN_CARRIER_MODULE
    #ifdef TE_AC_SAMPLE_MODE_V_1_2
     realAcData[nowAcItem] = (~realAcData[nowAcItem])&0xffffff;
    #endif
   #endif

   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d:���ɲɼ����%d,�ɼ���:%02x,�ɼ�ֵ:%08x\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, nowAcItem,realTableAC[nowAcItem],realAcData[nowAcItem]);
   }

   if (readCheckData==0x55)
   {
     if (debugInfox & CHECK_AC_FROM_SERIAL)
     {
       acBuffer[0] = 0x68;
     
       acBuffer[5] = 0x68;
     
       acBuffer[6] = 0x80;
       
       acBuffer[7]  = addrField.a1[0];
       acBuffer[8]  = addrField.a1[1];
       acBuffer[9]  = addrField.a2[0];
       acBuffer[10] = addrField.a2[1];
       acBuffer[11] = addrField.a3;
       
       acBuffer[12] = 0xA;
     
       acBuffer[13] = 0x60 | rSeq;
       
       acBuffer[14] = 0x0;
       acBuffer[15] = 0x0;
       acBuffer[16] = 0x04;   //FN131
       acBuffer[17] = 0x10;
       acBuffer[18] = realTableAC[nowAcItem];
       acBuffer[19] = realAcData[nowAcItem]&0xff;
       acBuffer[20] = realAcData[nowAcItem]>>8&0xff;
       acBuffer[21] = realAcData[nowAcItem]>>16&0xff;
       acBuffer[22] = realAcData[nowAcItem]>>24&0xff;
       
       tmpAcI = ((23 - 6) << 2) | 0x1;
       acBuffer[1] = tmpAcI & 0xFF;   //L
       acBuffer[2] = tmpAcI >> 8;
       acBuffer[3] = tmpAcI & 0xFF;   //L
       acBuffer[4] = tmpAcI >> 8; 
         
       tmpAcI = 6;
       checkSumAc = 0;
       while (tmpAcI < 23)
       {
          checkSumAc += acBuffer[tmpAcI];
          tmpAcI++;
       }
       acBuffer[23] = checkSumAc;
       
       acBuffer[24] = 0x16;
       
       sendLocalMsFrame(acBuffer,25);
     }
     else
     {
       if (fQueue.tailPtr == 0)
       {
          tmpHead00 = 0;
       }
       else
       {
          tmpHead00 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
       }
  
       msFrame[tmpHead00+0] = 0x68;
  
       tmpAcI = ((23 - 6) << 2) | 0x1;
       msFrame[tmpHead00+1] = tmpAcI & 0xFF;   //L
       msFrame[tmpHead00+2] = tmpAcI >> 8;
       msFrame[tmpHead00+3] = tmpAcI & 0xFF;   //L
       msFrame[tmpHead00+4] = tmpAcI >> 8; 
       
       msFrame[tmpHead00+5] = 0x68;
       
       msFrame[tmpHead00+6] = 0x80;          //C:10000000
       
       msFrame[tmpHead00+7] = addrField.a1[0];
       msFrame[tmpHead00+8] = addrField.a1[1];
       msFrame[tmpHead00+9] = addrField.a2[0];
       msFrame[tmpHead00+10] = addrField.a2[1];
       msFrame[tmpHead00+11] = acMsa;
       
       msFrame[tmpHead00+12] = 0xA;   
       msFrame[tmpHead00+13] = 0x60 | rSeq;
       
       msFrame[tmpHead00+14] = 0x0;
       msFrame[tmpHead00+14] = 0x0;
       msFrame[tmpHead00+15] = 0x0;
       msFrame[tmpHead00+16] = 0x04;   //FN131
       msFrame[tmpHead00+17] = 0x10;
       msFrame[tmpHead00+18] = realTableAC[nowAcItem];
       msFrame[tmpHead00+19] = realAcData[nowAcItem]&0xff;
       msFrame[tmpHead00+20] = realAcData[nowAcItem]>>8&0xff;
       msFrame[tmpHead00+21] = realAcData[nowAcItem]>>16&0xff;
       msFrame[tmpHead00+22] = realAcData[nowAcItem]>>24&0xff;
       
       frameTail00 = tmpHead00+23;
       
       i = tmpHead00+6;
       checkSumAc = 0;
       while (i < frameTail00)
       {
          checkSumAc = checkSumAc + msFrame[i];
          i++;
       }
       msFrame[frameTail00++] = checkSumAc;
       
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
       
       fQueue.inTimeFrameSend = TRUE;
     }
     
     if  (nowAcItem>=14)
     {
     	 nowAcItem = 0;
     }
     else
     {
     	 nowAcItem++;
     }
     
     return;
   }

   if (nowAcItem==30 && readCheckData==0x00)
   {
   	 if (checkSum3e!=realAcData[29] || checkSum5f!=realAcData[30])
   	 {
   	 	  resetAtt7022b(TRUE);
   	 }
   }
   
   if (nowAcItem>=TOTAL_COMMAND_REAL_AC-1)
   {
 	   nowAcItem = 0;

     if (readCheckData!=0x55)
     {
 	     //��¼����ʾֵ������
 	     recordVisionReq();
 	   
 	     countPhase++;
 	     //��Լÿ12����һ��
 	     if (countPhase>5 && (!(sysTime.hour==23 && sysTime.minute>=58)))
 	     {
         countPhase = 0;
                
 	   	   decideAcPhaseOrder();
 	   	 }
 	   }
   }
   else
   {
     nowAcItem++;
   }
}

/***************************************************
��������:sspWrite
��������:SSP1д
���ú���:
�����ú���:
�������:8bit����,32bit����(��8b��Ч)
�������:
����ֵ���ɹ���ʧ��
***************************************************/
INT8U sspWrite(INT8U cmd,INT32U data)
{
 	 INT32U i;
 	 INT8U  buf[4];
 	 
   buf[0] = cmd;
   buf[1] = data>>16&0xff;
   buf[2] = data>>8&0xff;
   buf[3] = data&0xff;
 	 write(fdOfSample,buf,4);
   
   //2012-07-27,�����ӳ�,�������ӳٹ�����Ӧ�ó�����������
   //for(i=0;i<0x10000;i++)
   for(i=0;i<0x20;i++)
   {
   	 if (ioctl(fdOfSample,READ_SIG_OUT_VALUE,0)==1)
   	 {
   	  	break;
   	 }
   }
   
   if (i==0x10000)
   {
   	 return 0;
   }
   else
   {
     return 1;
   }
}

/***************************************************
��������:writeOneCheckPara
��������:д��һ��У�����
���ú���:
�����ú���
�������:��
�������:
����ֵ����
***************************************************/
INT8U writeOneCheckPara(INT8U checkReg, INT32U data)
{
   INT8U returnData = 0;
   INT8U i;
   INT8U checkData[4];
   
   for(i=0; i<8; i++)
   {
     if (sspWrite(checkReg, data)==1)
     {
        checkData[0] = 0x2e;
        read(fdOfSample, checkData, 3);
        
        //printf("�Ĵ���=%x,д������=%x,��������=%02x%02x%02x\n",checkReg, data,checkData[0],checkData[1],checkData[2]);

        if (data==(checkData[0]<<16 | checkData[1]<<8 | checkData[2]))
        {
        	 returnData = 1;
        	 break;
        }
     }
   }
   
   return returnData;
}

/***************************************************
��������:sspCheckMeter
��������:SSP1У��
���ú���:
�����ú���
�������:��
�������:
����ֵ����
***************************************************/
void sspCheckMeter(void)
{
   //ADC����
   if (writeOneCheckPara(0x3f | 0x80,acSamplePara.UADCPga)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(ADC����)\n");
      }
   }

   //����������ʹ�ܿ���,ly,2011-08-04,Add
   if (writeOneCheckPara(0x30 | 0x80, 0x5678)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(����������ʹ�ܿ��ƹ���ʹ��)\n");
      }
   }

   //��Ƶ�����������
   if (writeOneCheckPara(0xa0,acSamplePara.HFConst)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(��Ƶ�����������)\n");
      }
   }
   else
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ʧ��(��Ƶ�����������)\n");
      }
   }
   
   //����(ʧѹ)��ֵ
   if (writeOneCheckPara(0xa9,acSamplePara.FailVoltage)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(����(ʧѹ)��ֵ)\n");
      }
   }
  
   //��������
   if (writeOneCheckPara(0x9f,acSamplePara.Istartup)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(��������)\n");
      }
   }   

   //�����ۼ�ģʽ
   if (writeOneCheckPara(0xac,acSamplePara.EAddMode)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(�����ۼ�ģʽ)\n");
      }
   }   
  
   //ʹ�ܵ�ѹ�нǲ���
  #ifdef MEASURE_V_ANGLE
   if (writeOneCheckPara(0x2E | 0x80, 0x3584)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(ʹ�ܵ�ѹ�нǲ���)\n");
      }
   }
  #endif
  
   //A������1
   if (writeOneCheckPara(0x89,acSamplePara.PgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A������1)\n");
      }
   }
   
   //A������0
   if (writeOneCheckPara(0x86,acSamplePara.PgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A������0)\n");
      }
   }   

   //A����λ4
   if (writeOneCheckPara(0x90,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A����λ4)\n");
      }
   }   
   
   //A����λ3
   if (writeOneCheckPara(0x8f,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A����λ3)\n");
      }
   }
   
   //A����λ2
   if (writeOneCheckPara(0x8e,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A����λ2)\n");
      }
   }   
   
   //A����λ1
   if (writeOneCheckPara(0x8d,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A����λ1)\n");
      }
   }   
   
   //A����λ0
   if (writeOneCheckPara(0x8c,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A����λ0)\n");
      }
   }   

   //A���ѹУ��ֵ
   if (writeOneCheckPara(0x9b,acSamplePara.UgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A���ѹУ��ֵ,UgainA=%0x)\n", acSamplePara.UgainA);
      }
   }
   
   //A�����У��ֵ
   if (writeOneCheckPara(0xa6,acSamplePara.IgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(A�����У��ֵ)\n");
      }
   }
   
   //B������1
   if (writeOneCheckPara(0x8A,acSamplePara.PgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B������1)\n");
      }
   }   
   
   //B������0
   if (writeOneCheckPara(0x87,acSamplePara.PgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B������0)\n");
      }
   }   

   //B����λ4
   if (writeOneCheckPara(0x95,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B����λ4)\n");
      }
   }   
   
   //B����λ3
   if (writeOneCheckPara(0x94,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B����λ3)\n");
      }
   }   
   
   //B����λ2
   if (writeOneCheckPara(0x93,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B����λ2)\n");
      }
   }   
   
   //B����λ1
   if (writeOneCheckPara(0x92,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B����λ1)\n");
      }
   }   

   //B����λ0
   if (writeOneCheckPara(0x91,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B����λ0)\n");
      }
   }   
   
   //B���ѹУ��ֵ
   if (writeOneCheckPara(0x9c,acSamplePara.UgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B���ѹУ��ֵ,UgainB=%0x)\n",acSamplePara.UgainB);
      }
   }
   
   //B�����У��ֵ
   if (writeOneCheckPara(0xa7,acSamplePara.IgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(B�����У��ֵ)\n");
      }
   }
   
   //C������1
   if (writeOneCheckPara(0x8B,acSamplePara.PgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C������1)\n");
      }
   }   
   
   //C������0
   if (writeOneCheckPara(0x88,acSamplePara.PgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C������0)\n");
      }
   }   

   //C����λ4
   if (writeOneCheckPara(0x9A,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C����λ4)\n");
      }
   }   
   
   //C����λ3
   if (writeOneCheckPara(0x99,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C����λ3)\n");
      }
   }   
   
   //C����λ2
   if (writeOneCheckPara(0x98,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C����λ2)\n");
      }
   }   
   
   //C����λ1
   if (writeOneCheckPara(0x97,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C����λ1)\n");
      }
   }   

   //C����λ0
   if (writeOneCheckPara(0x96,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C����λ0)\n");
      }
   }   
   
   //C���ѹУ��ֵ
   if (writeOneCheckPara(0x9D,acSamplePara.UgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C���ѹУ��ֵ,UgainC=%0x)\n", acSamplePara.UgainC);
      }
   }
   
   //C�����У��ֵ
   if (writeOneCheckPara(0xa8,acSamplePara.IgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(C�����У��ֵ)\n");
      }
   }

   //��������ʹ�ܿ��ƼĴ���(��Ϊȫ����)
   if (writeOneCheckPara(0x2d, 0x000000)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022BУ��ֵд��ɹ�(��������ʹ�ܿ��ƼĴ���,��Ϊȫ����)\n");
      }
   }
   
   usleep(800000);
   
   acReadBuffer[0] = 0x3e;
   read(fdOfSample,&acReadBuffer,3);
   checkSum3e = acReadBuffer[0]<<16 | acReadBuffer[1]<<8 | acReadBuffer[2];
   
   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("ATT7022BУ������У���1=0x%0x\n", checkSum3e);
   }
   
   acReadBuffer[0] = 0x5f;
   read(fdOfSample,&acReadBuffer,3);
   checkSum5f = acReadBuffer[0]<<16 | acReadBuffer[1]<<8 | acReadBuffer[2];
   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("ATT7022BУ������У���2=0x%0x\n", checkSum5f);
   }
}

#endif    //AC_SAMPLE_DEVICE
