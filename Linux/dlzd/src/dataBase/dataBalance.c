/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����dataBalance.c
���ߣ�TianYe
�汾��0.9
������ڣ�2006��7��
���������ݽ����ļ�
�����б�
�޸���ʷ��
  01,2006-7-19 TianYe created.
  02,2007-3-10,TianYe and Leiyong�޸��ս��㼰�½���,����δ�����ս�����½���
  03,2007-4-19,Leiyong And TianYe���ӳ����ն���
  04,2007-7-2,Leiyong And TianYe�������ͣ���¼�����
  05,2008-09-19,Tianye�޸�,�淶���������޸Ĵ��롣���������û�г����Ĳ������������ʱ������...
  06,2008-12-23,Leiyong�޸�,�����ܼӹ���ʱ����CT,PT�õ����ι���
  07,2010-01-22,Leiyong,����GDW376.1-2009Э����Ҫ�Ľ��㼰�ж�
  08,2010-07-20,Leiyong,�ڿ�½����̨�ϲ���linux�µ�Ӧ��,GDW129-2005����ͨ��
  09,2012-04-17,ȥ�������ս���
  10,2012-04-25,ȥ�����˿ڲ��ܲ��н�����ж�

***************************************************/


#include "teRunPara.h"
#include "msSetPara.h"

#include "convert.h"

#include "att7022b.h"
#include "workWithMeter.h"
#include "meterProtocol.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "AFN0C.h"

#include "dataBalance.h"

//���ݴ洢��־
#define  NO_DATA_NEED_STORE         0x0    //û��������Ҫ����
#define  STORE_ENERGY_BALANCE_DATA  0x1    //�洢ʵʱ�������������
#define  STORE_DAY_BALANCE_DATA     0x2    //�洢�ն�������
#define  STORE_MONTH_BALANCE_DATA   0x4    //�洢�¶�������
#define  STORE_ZJZ_BALANCE_DATA     0x8    //�洢�ܼ����������

#define maxData(data1, data2) ((data1>=data2)?data1:data2)
#define minData(data1, data2) ((data1<=data2)?data1:data2)

/*******************************************************
��������: dataFormat
��������: ������Ҫ�����ݸ�ʽ�������ݸ�ʽ��ȷ��������
���ú���:     
�����ú���:
�������:   ��������ָ��integer
            С������ָ��decimal(4λ������ʾ��С������,�����0.23��ô�����decimal����Ӧ����2300������23)
            ��Ҫת���ĸ�ʽ��format
�������:  ���������ݽ�����������������,
           ���������ݽ���������С������
����ֵ�� ���ݽ����������
*******************************************************/
INT8U dataFormat(INT32U *integer, INT32U *decimal, INT8U format)
{
    INT8U quantity, tmp;
    quantity = 0;
  
    switch (format)
    {
        case FORMAT(2):
          if (*integer > 999)
          {
            do
            {
              tmp = *integer % 10;   //��λ�Ƶ�С������
              *integer /= 10;        
              *decimal /= 10;        //����С����������
              *decimal += 1000 * tmp;//ԭ�������ĸ�λ����С����ʮ��λ
            
              quantity++;
              
              if (quantity >= 4)
              {
                break;
              }
            }while (*integer > 999);
          }
          else 
          {
            if (*integer < 100)
            {
              quantity = 4;
              do 
              {
              	tmp = *decimal / 1000;              //С��ʮ��λ�Ƶ���������
              	*decimal = (*decimal % 1000) * 10;  //С����������
              	*integer = *integer * 10 + tmp;     //��������+С���Ƴ��Ĳ���
            	
              	quantity++;
            	
              	if (quantity >= 7)
              	{
                   break;
              	}
              }while (*integer < 100);
            }
          }
          
          switch (quantity)
          {
            case 0:
            	quantity = 0x4;   //10��0�η�
            	break;     
            case 1:
            	quantity = 0x3;   //10��1�η�
            	break;
            case 2:
            	quantity = 0x2;   //10��2�η�
            	break;
            case 3:
            	quantity = 0x1;   //10��3�η�
            	break;
            case 4:
            	quantity = 0x0;   //10��4�η�
            	break;
            case 5:
            	quantity = 0x5;   //10��-1�η�
            	break;
            case 6:
          	  quantity = 0x6;   //10��-2�η�
          	  break;
            case 7:
          	  quantity = 0x7;   //10��-3�η�
            	break;
          }
          
          //С���������뵽����λ
          if (*decimal > 5000)
          {
             *integer += 1;
             *decimal = 0;
          }
          
      	  break;
      	
        case FORMAT(3):
      	  if (*integer > 9999999)
      	  {
      	    tmp = *integer % 1000;
      	    *integer = *integer / 1000;
      	    *decimal = tmp;
      	  
      	    quantity = 0x40;    //10��3�η�
      	  }
      	  else
      	  {
      	    quantity = 0;
      	  }
      	  
      	  //С���������뵽����λ
      	  if (*decimal > 5000)
      	  {
      	    *integer += 1;
      	    *decimal = 0;
      	  }
      	  
      	  break;
     
        default:
      	  break;
    }
    
    return quantity;
}

/*******************************************************
��������: meterRunWordChangeBit
��������: �жϵ������״̬���Ƿ��λ
���ú���:     
�����ú���:
�������:

�������:
����ֵ�� 
*******************************************************/
void meterRunWordChangeBit(INT8U *changeWord,INT8U *thisRunWord,INT8U *lastRunWord)
{
	 INT8U i,j;
	 INT8U tmpShift,tmpThisData,tmpLastData;
	 
	 if (debugInfo&PRINT_BALANCE_DEBUG)
	 {
	  printf("��ʼ�жϵ������״̬�ֱ�λ:\n");
	 }
	 
	 for(i=0;i<14;i++)
	 {
	 	 *changeWord=0; 
	 	 if (*thisRunWord!=0xee && *lastRunWord!=0xee)
	 	 {
	 	 	 if (debugInfo&PRINT_BALANCE_DEBUG)
	 	 	 {
	 	 	   printf("��%02d�ֽڶԱ�:Last=%02X,Curent=%02X\n", i+1, *lastRunWord, *thisRunWord);
	 	 	 }
	 	 	 
	 	 	 tmpThisData = *thisRunWord++;
	 	 	 tmpLastData = *lastRunWord++;
	 	 	 tmpShift = 1;
	 	 	 for(j=0;j<8;j++)
	 	 	 {
	 	 	 	 if ((tmpThisData&tmpShift)!=(tmpLastData&tmpShift))
	 	 	 	 {
	 	 	 	 	 *changeWord |= tmpShift;
	 	 	 	 }
	 	 	 	 tmpShift<<=1;
	 	 	 }
	 	 }
	 	 
	 	 changeWord++;
	 }
	 
	 if (debugInfo&PRINT_BALANCE_DEBUG)
	 {
	   printf("�жϵ������״̬�ֱ�λ����\n");
	 }
}


//�ڲ�˽�к���
void freeMpLink(struct cpAddrLink *linkHead);
BOOL energyCompute(INT16U pn,INT8U *pCopyEnergyBuff,INT8U *pBalanceEnergyBuff, DATE_TIME statisTime,INT8U type);
void realStatisticPerPoint(INT16U pn,INT8U *pBalanceParaBuff, DATE_TIME statisTime);
void eventRecord(INT16U pn, INT8U *pCopyEnergyBuff, INT8U *pCopyParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, INT8U statisInterval, DATE_TIME statisTime);
void cOverLimitEvent(INT8U *pCopyParaBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime);
void cLoopEvent(INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime);
void vOverLimitEvent(INT8U *pCopyParaBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime);
void vAbnormalEvent(INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U phase, INT16U pn, INT8U type, BOOL recovery, DATE_TIME statisTime);
void changeEvent(INT16U pn, INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, DATE_TIME statisTime,INT8U protocol, INT8U statisInterval);
void statisticVoltage(INT16U pn, INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit,INT8U statisInterval,DATE_TIME statisTime);
void statisticCurrent(INT16U pn, INT8U *pCopyParaBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit, INT8U statisInterval, DATE_TIME statisTime);
void statisticUnbalance(INT16U pn, INT8U *copyParaBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA * pLimit,INT8U statisInterval,DATE_TIME statisTime);
void pOverLimitEvent(INT16U pn, INT8U type, BOOL recovery, INT8U *data, void *pLimit,DATE_TIME statisTime);
void statisticApparentPowerAndFactor(INT16U pn, INT8U *copyParaBuff, INT8U *balanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit, INT8U statisInterval,DATE_TIME statisTime);

BOOL groupBalance(INT8U *ppBalanceZjzData, INT8U gp, INT8U ptNum, INT8U balanceType, DATE_TIME balanceTime);
BOOL groupStatistic(INT8U *pBalanceZjzData, INT8U gp, INT8U ptNum, INT8U balanceType,DATE_TIME statisTime);
INT32U calcResumeLimit(INT32U limit, INT16U factor);

/*************************************************************************
��������:dataProcRealPoint
��������:������ʵʱ�������ݴ���,�Թ�����ο���
         �������ݣ�
         ������ص������� ���������й�������
                          ���������޹�������
                          ���շ����й�������
                          ���շ����޹�������
                          ...
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*************************************************************************/
void dataProcRealPoint(void *arg)
{
    INT8U                    port;                                            //�˿�
    INT8U                    type;                                            //����
    INT8U                    balanceInterval;                                 //������
    DATE_TIME                balanceTime;                                     //����ʱ��
    
    struct cpAddrLink        *mpLinkHead;                                     //����������
    struct cpAddrLink        *tmpNode;                                        //������������ʱָ��
    MEASUREPOINT_LIMIT_PARA  *pMpLimitValue;                                  //��������ֵ����ָ��
    METER_STATIS_EXTRAN_TIME meterStatisRecord;                               //һ����ͳ���¼�����
    METER_DEVICE_CONFIG      meterConfig;                                     //һ���������������Ϣ
    INT8U                    lastCopyEnergyData[LENGTH_OF_ENERGY_RECORD];     //��һ�γ������������
    INT8U                    lastCopyParaData[LENGTH_OF_PARA_RECORD];         //��һ�γ������,�α�������
    INT8U                    lastCopyReqData[LENGTH_OF_REQ_RECORD];           //��һ�γ�����������������ʱ������
    INT8U                    balanceEnergyData[LEN_OF_ENERGY_BALANCE_RECORD]; //�����õ���������
    INT8U                    balanceParaData[LEN_OF_PARA_BALANCE_RECORD];     //�����òα�������
    INT8U                    balanceZjzData[LEN_OF_ZJZ_BALANCE_RECORD];       //�������ܼ�������
    INT8U                    balanceZjzDatax[LEN_OF_ZJZ_BALANCE_RECORD];      //�������ܼ�������x

    BOOL                     ifSinglePhaseMeter;                              //�Ƿ�����־    
    INT8U                    flagOfStoreData;                                 //�洢���ݱ�־
    INT8U                    flagOfLastData;                                  //�Ƿ�����һ�γ�������?
    INT16U                   i, j, k;
    DATE_TIME                tmpTime, readTime;
    INT16U                   balancePn;                                       //���������
    INT32U                   compData,diffData,absoluteData;                  //�ܼ��й��������ж���
    INT8U                    tmpRecord;
    INT8U                    eventData[200];
    INT8U                    tmpTail,tmpTailx;
    
    while(1)
    {
      type = 0;
      for(port=0; port<NUM_OF_COPY_METER; port++)
      {
        //��һ�α�����ʵʱ����
       #ifdef PLUG_IN_CARRIER_MODULE
        if (copyCtrl[port].ifRealBalance==1 && port<4)
       #else
        if (copyCtrl[port].ifRealBalance==1)
       #endif
        {
          copyCtrl[port].ifRealBalance = 2;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("\n\n%02d-%02d-%02d %02d:%02d:%02d׼���˿�%d����,2=%d,3=%d\n\n",sysTime.year,sysTime.month,
                   sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                     port+1,copyCtrl[1].ifRealBalance,copyCtrl[2].ifRealBalance);
          }
          
          type = 1;                                                  //��������
          balanceInterval = teCopyRunPara.para[port].copyInterval;   //������
          balanceTime = copyCtrl[port].lastCopyTime;                 //����ʱ��Ϊ�ϴγ���ʱ��

          break;
        }
      }
      
      //����(����ʵʱ���㡢�ս��㼰�½���)
      if (type==1)
      {
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("�˿�%d���㿪ʼ\n", port+1);
        }
        
    	  //0.���ݴ洢��־��Ϊ����Ҫ���������
    	  flagOfStoreData = NO_DATA_NEED_STORE;
    	  
    	  //1.��ʼ�����˿ڲ���������
    	  mpLinkHead = initPortMeterLink(port);
    
    	  //1-1.û�в���������,ֱ�ӷ���
    	  if (mpLinkHead==NULL)
    	  {
    	  	if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	{
    	  	  printf("�˿�%dû�в���������,ֱ�ӷ���\n", port+1);
    	  	}
    	  	
    	  	copyCtrl[port].ifRealBalance = 0;
    	  	
    	  	continue;
    	  }
      	  
    	  //2.����645��Լ�Ĳ�������ʼָ��
    	  tmpNode = mpLinkHead;
    	  while(tmpNode!=NULL)
    	  {
           if (selectF10Data(tmpNode->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
           {
        	    //����û�иò����������
        	    
    	        tmpNode = tmpNode->next;  //������λ
        	    continue;
           }
    
    	     if (meterConfig.protocol == DLT_645_1997
    	      	|| meterConfig.protocol == DLT_645_2007
    	      	 || meterConfig.protocol == SINGLE_PHASE_645_1997
    	      	  || (meterConfig.protocol == AC_SAMPLE && ifHasAcModule==TRUE)
    	      	   || meterConfig.protocol == EDMI_METER)
    	     {
    	      	break;
    	     }
    	     
    	     tmpNode = tmpNode->next;  //������λ
    	  }
        
        //2-1.û�п��Լ���Ĳ�����,�ͷ�����Ȼ�󷵻�
        if (tmpNode==NULL)
        {
         	 freeMpLink(mpLinkHead);
    
    	  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	 {
    	  	   printf("�˿�%dû�п��Լ���Ĳ�����,�ͷ�����Ȼ�󷵻�\n", port+1);
    	  	 }
    	  	 
    	  	 copyCtrl[port].ifRealBalance = 0;

        	 continue;
        }
        
        //3.�����ϴκ��´γ���ʱ�䣬�ж��Ƿ�����ս�����½���
        tmpTime = timeHexToBcd(copyCtrl[port].nextCopyTime);
 
        //�����һ�γ���ʱ�����һ�γ���ʱ�����һ��,��Ҫ�����ս���ת��
        if (tmpTime.day != balanceTime.day)
        {
          while(tmpNode!=NULL)
          {
            if (selectF10Data(tmpNode->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
            {
     	        //����û�иò����������
     	        tmpNode = tmpNode->next;
            }
            else
            {
            	break;
            }
          }
          
          if (tmpNode == NULL)
          {
          	 freeMpLink(mpLinkHead);
          	 
          	 copyCtrl[port].ifRealBalance = 0;
          	 
          	 continue;
          }
          
          //�鿴�Ƿ��е�����ս�������,����оͲ��������ս�����(���˼·�Ѹ���Ϊ�������еĴ���)
          //ly,10-10-02,�ĳ����ԭ�������ն������ݾ͸����ն�������,��Ϊ�������0�������
          readTime = balanceTime;
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	   printf("�洢�ն������ݱ�־��λ\n");
        	}
        	 
        	flagOfStoreData |= STORE_DAY_BALANCE_DATA;
        
    	    //�����һ�γ���ʱ�����һ�γ���ʱ�����һ�£���Ҫ�����½���ת��
          if (tmpTime.month != balanceTime.month)
          {
        	  flagOfStoreData |= STORE_MONTH_BALANCE_DATA;
          }
        }
       
        //4.���˿���������������ò��������,ͳ��
    	  if (mpLinkHead != NULL)
    	  {
    	    //Ϊ��������ֵ����洢�ռ�
    	    pMpLimitValue = (MEASUREPOINT_LIMIT_PARA *)malloc(sizeof(MEASUREPOINT_LIMIT_PARA));
    	    
    	    while (tmpNode!=NULL)
    	    {
    	      flagOfStoreData &= ~STORE_ENERGY_BALANCE_DATA;
    	      flagOfLastData = 0x7;   //��һ�����ݶ���
    	      
    	      //4-1ȷ����������Ϣ
    	      ifSinglePhaseMeter = FALSE;
            
            //4-1-1ȷ�����������õĲ�������Ϣ
            if (selectF10Data(tmpNode->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
            {
        	     //����û�иò����������
    	         tmpNode = tmpNode->next;  //������λ
        	     continue;
            }
    	      
    	      //4-1-2ͨ�Ź�Լ����Ϊ�ֳ�����豸���ǽ�������װ��,����ʵʱ�����������
    	      //if (meterConfig.protocol == AC_SAMPLE || meterConfig.protocol == SUPERVISAL_DEVICE)
    	      if (meterConfig.protocol == SUPERVISAL_DEVICE)
    	      {
    	         tmpNode = tmpNode->next;  //������λ
    	         continue;
    	      }
            
            //4-1-3.������õ����־
            if (meterConfig.protocol==SINGLE_PHASE_645_1997)
            {
            	 ifSinglePhaseMeter = TRUE;
            }
            balancePn = meterConfig.measurePoint;
    
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
    	        printf("\n������%d��ʼ����,����ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n",balancePn,
    	             balanceTime.year,balanceTime.month,balanceTime.day,balanceTime.hour,balanceTime.minute,balanceTime.second);
    	      }
            
            //4-2.�����ϴγ�������
            //4-2-1.����������
            tmpTime = balanceTime;
            if (readMeterData(lastCopyEnergyData, balancePn, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == FALSE)
            {
            	flagOfLastData &= 0xFE;  //���޵��������ݱ�־
            }
            
            usleep(50000);
                    
            //4-2-2.��������������ʱ��
            tmpTime = balanceTime;
            if (readMeterData(lastCopyReqData, balancePn, PRESENT_DATA, REQ_REQTIME_DATA, &tmpTime, 0) == FALSE)
            {
            	flagOfLastData &= 0xFD;  //�����������ݱ�־
            }
    
            usleep(50000);        
            
            //4-2-3.�������α���
            tmpTime = balanceTime;
            if (readMeterData(lastCopyParaData, balancePn, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0) == FALSE)
            {
            	flagOfLastData &= 0xFB;  //���ޱ����α������ݱ�־
            }
            
            usleep(50000);
            
            //4-3.����������ͳ��
            //4-3-1.��ʼ��ͳ���û�����
            //��ȡǰһ��ʵʱ����α���ͳ������,��ϱ��γ������������µ�ͳ������
            tmpTime = balanceTime;
            if (readMeterData(balanceParaData, balancePn, LAST_REAL_BALANCE, REAL_BALANCE_PARA_DATA, &tmpTime, 0) == TRUE)
            {
    	        balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE_NOT;  //�������ͳ�Ʊ�־

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("dataProcRealPoint:��ǰһ��ʵʱ����α���ͳ������\n");
              }
            }
            else         //��ʼ�µ�ͳ�Ƽ�¼
            {
              memset(balanceParaData, 0xee, LEN_OF_PARA_BALANCE_RECORD);
              
              //û��ͳ����ʷ,����ͳ�Ƶ�ͬ������ͳ��
              balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE;

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("dataProcRealPoint:��ǰһ��ʵʱ����α���ͳ������,������ͳ�Ʊ�־\n");
              }
            }
    
            usleep(50000);
    
          	//4-3-2.��������ֵ���������¼�ͳ������
          	//4-3-2-1.��ȡ�������Ӧ����ֵ����
        	  if(selectViceParameter(0x04, 26, balancePn, (INT8U *)pMpLimitValue, sizeof(MEASUREPOINT_LIMIT_PARA)) == FALSE)
        	  {
        	  	pMpLimitValue = NULL;
        	  }
    
    	      //4-3-2-2.�������ͳ�Ƽ�¼
    	      tmpTime = timeBcdToHex(balanceTime);
            searchMpStatis(tmpTime, &meterStatisRecord, balancePn, 1);  //��ʱ���޹���
    
            //4-3-3.�����������ͳ��
            statisticMaxDemand(lastCopyReqData, balanceParaData);
            usleep(50000);
    
          	//4-3-4.����ͳ��
            statisticPower(lastCopyParaData, balanceParaData, balanceInterval, balanceTime);
            usleep(50000);
    
            //4-3-5.���ڹ���Խ���ۼƼ����������ֶ��ۼ�
            statisticApparentPowerAndFactor(balancePn, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, balanceInterval, balanceTime);
            usleep(50000);
    
          	//4-3-6.��ѹͳ��
            statisticVoltage(balancePn, lastCopyParaData, lastCopyEnergyData, balanceParaData, &meterStatisRecord, pMpLimitValue,balanceInterval, balanceTime);
            usleep(50000);
    
          	//4-3-7.����ͳ��
            statisticCurrent(balancePn, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, balanceInterval, balanceTime);
            usleep(50000);
    
          	//4-3-8.��ƽ���Խ���ۼ�
            statisticUnbalance(balancePn, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue,balanceInterval, balanceTime);
            usleep(50000);
    
            //4-3-9.����ͳ��
            statisticOpenPhase(lastCopyParaData, balanceParaData);
            usleep(50000);
            
            //4-3-10.�洢����α�������            
    	      balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE_NOT;  //����״�ͳ��ʱ��λ������ͳ�Ʊ�־
            
            //4-3-10.1ʵʱ����α���ͳ�����ݴ洢��ʵʱ����������
            if (balanceParaData[MAX_TOTAL_POWER] != 0xEE 
          	   || (lastCopyParaData[VOLTAGE_PHASE_A] != 0xEE || lastCopyParaData[VOLTAGE_PHASE_B] != 0xEE || lastCopyParaData[VOLTAGE_PHASE_C] != 0xEE)
          	    || (lastCopyParaData[CURRENT_PHASE_A] != 0xEE || lastCopyParaData[CURRENT_PHASE_B] != 0xEE || lastCopyParaData[CURRENT_PHASE_C] != 0xEE)
          	     || (balanceParaData[MAX_TOTAL_REQ] != 0xEE))
            {
              saveMeterData(balancePn, port+1, balanceTime, balanceParaData, REAL_BALANCE, REAL_BALANCE_PARA_DATA,LEN_OF_PARA_BALANCE_RECORD);
               
              //4-3-10.2 �ս���α���ͳ�����ݴ洢���ս����
              if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
              {
                saveMeterData(balancePn, port+1, balanceTime, balanceParaData, DAY_BALANCE, DAY_BALANCE_PARA_DATA,LEN_OF_PARA_BALANCE_RECORD);
            
                //4-3-10.3 ͳ�Ʊ��µ���ǰΪֹ�Ĳα���ͳ��ֵ
                //ly,2011-08-13,ԭ�����������������������,��ͳ�����ݲ���ȷ
                realStatisticPerPoint(balancePn, balanceParaData, balanceTime);
        	       
        	      tmpTime = balanceTime;
        	      tmpTime.second = 0x59;
                tmpTime.minute = 0x59;
                tmpTime.hour   = 0x23;
                saveMeterData(balancePn, port+1, tmpTime, balanceParaData, DAY_BALANCE, MONTH_BALANCE_PARA_DATA, LEN_OF_PARA_BALANCE_RECORD);
                 
                //4-3-10.4 ������ʵʱ��������ݽ���洢���½������,��������ͳ��
                if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
                {
                  saveMeterData(balancePn, port+1, tmpTime, balanceParaData, MONTH_BALANCE, MONTH_BALANCE_PARA_DATA,LEN_OF_PARA_BALANCE_RECORD);
                }
              }
            }
    
    	      //4-4.���������ݽ���
    	      //4-4-1.��ʼ�������û���
    	      memset(balanceEnergyData, 0xee, LEN_OF_ENERGY_BALANCE_RECORD);
    	      
            //4-4-2.���ܱ��¼�
            eventRecord(balancePn, lastCopyEnergyData, lastCopyParaData, &meterStatisRecord, balanceInterval, balanceTime);
            usleep(50000);
            
            //4-4-3.����¼�
            changeEvent(balancePn, lastCopyParaData, lastCopyEnergyData, &meterStatisRecord, balanceTime,meterConfig.protocol, balanceInterval);
            usleep(50000);
            
            //4-4-4.���㵱�յ�����
            if (energyCompute(balancePn, lastCopyEnergyData, balanceEnergyData, balanceTime, 1)==TRUE)
            {
            	flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
            }
            usleep(50000);
            
            //4-4-5.���㵱�µ�����
            if (ifSinglePhaseMeter==FALSE)
            {
              if (energyCompute(balancePn,lastCopyEnergyData,balanceEnergyData, balanceTime, 2)==TRUE)
              {
            	   flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
              }
            }
            usleep(50000);
    
    	      //4-4-6.�洢�����������
            if (flagOfStoreData&STORE_ENERGY_BALANCE_DATA)
            {
              //ʵʱ������������ݴ洢��ʵʱ�����
              saveMeterData(balancePn, port+1, balanceTime, balanceEnergyData, REAL_BALANCE, REAL_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
              
              if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
              {
            	   //�ս���������洢���ս����
                 saveMeterData(balancePn, port+1, balanceTime, balanceEnergyData, DAY_BALANCE, DAY_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);             
              }
            
              //������ʵʱ��������ݽ���洢���½����
              if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
              {
                saveMeterData(balancePn, port+1, balanceTime, balanceEnergyData, MONTH_BALANCE, MONTH_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
              }
            }
    
            //4-5.��(��)ĩת��ʾֵ����������
            if (flagOfStoreData&STORE_DAY_BALANCE_DATA)
        	  {
    	        if (debugInfo&PRINT_BALANCE_DEBUG)
    	        {
             	  printf("ת���ս��������/����\n");
             	}
    
        	    tmpTime = balanceTime;
        	    tmpTime.second = 0x59;
              tmpTime.minute = 0x59;
              tmpTime.hour   = 0x23;
                
              //ת���ս������ʾֵ
              if (flagOfLastData&0x1)
              { 
              	saveMeterData(balancePn, port+1, tmpTime, lastCopyEnergyData, DAY_BALANCE, DAY_FREEZE_COPY_DATA,LENGTH_OF_ENERGY_RECORD);
              }
                
              //ת���ս�������ʾֵ
              if (flagOfLastData&0x2)
              {
                saveMeterData(balancePn, port+1, tmpTime, lastCopyReqData, DAY_BALANCE, DAY_FREEZE_COPY_REQ, LENGTH_OF_REQ_RECORD);
              }
    
              //������ν�����Ҫ�����½���ת��,���������ݶ�����洢���½���������
              if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
              {
    	          if (debugInfo&PRINT_BALANCE_DEBUG)
    	          {
             	    printf("ת���½��������/����\n");
             	  }
                
                //ת���½������ʾֵ
                if (flagOfLastData&0x1)
                {
                  saveMeterData(balancePn, port+1, tmpTime, lastCopyEnergyData, MONTH_BALANCE, MONTH_FREEZE_COPY_DATA,LENGTH_OF_ENERGY_RECORD);
                }
                
                //ת���½�������ʾֵ
                if (flagOfLastData&0x2)
                {
                  saveMeterData(balancePn, port+1, tmpTime, lastCopyReqData, MONTH_BALANCE, MONTH_FREEZE_COPY_REQ,LENGTH_OF_REQ_RECORD);
                }
              }
            }
            usleep(1);
         	  
         	  //4-6�洢������ͳ������
            saveMeterData(balancePn, port+1, balanceTime, (INT8U *)&meterStatisRecord, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
            
    	      //4-7.������λ
    	      tmpNode = tmpNode->next;
    	    }
    	    
    	    free(pMpLimitValue);   //�ͷŲ�������ֵָ��
    	    freeMpLink(mpLinkHead);
    	  }
    	  
    	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	 {
    	   printf("������������\n");
    	 }
    	 
    	  //5.���������������� ly,09-07-14,Add
    	  #ifdef PULSE_GATHER
    	   if (pulseConfig.numOfPulse>0)
    	   {
    	     for(i=0;i<pulseConfig.numOfPulse;i++)
      	   {
             tmpTime = balanceTime;
             if (readMeterData(lastCopyEnergyData, pulseConfig.perPulseConfig[i].pn, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == TRUE)
             {
    	         if (debugInfo&PRINT_BALANCE_DEBUG)
    	         {
    	           printf("���������%d����\n", pulseConfig.perPulseConfig[i].pn);
    	         }
    	         
    	         flagOfStoreData &= ~STORE_ENERGY_BALANCE_DATA;
    	         
    	         //��ʼ�������û���
    	         memset(balanceEnergyData,0xee,LEN_OF_ENERGY_BALANCE_RECORD);
               
               //���㵱�յ�����
               if (energyCompute(pulseConfig.perPulseConfig[i].pn,lastCopyEnergyData,balanceEnergyData, balanceTime, 3)==TRUE)
               {
            	    flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
               }
               
               usleep(50000);
    
               //���㵱�µ�����
               if (energyCompute(pulseConfig.perPulseConfig[i].pn,lastCopyEnergyData,balanceEnergyData, balanceTime, 4)==TRUE)
               {
            	    flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
               }
    
               usleep(50000);
               
    	         //�洢�����������
               if (flagOfStoreData&STORE_ENERGY_BALANCE_DATA)
               {
                 //ʵʱ������������ݴ洢��ʵʱ�����
                 saveMeterData(pulseConfig.perPulseConfig[i].pn, 0, balanceTime, balanceEnergyData, REAL_BALANCE, REAL_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
              
                 if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
                 {
            	     //�ս���������洢���ս����
                   saveMeterData(pulseConfig.perPulseConfig[i].pn, 0, balanceTime, balanceEnergyData, DAY_BALANCE, DAY_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
                 }
               }
             }
             else
             {
    	         if (debugInfo&PRINT_BALANCE_DEBUG)
    	         {
    	           printf("���������%d����\n", pulseConfig.perPulseConfig[i].pn);
    	         }
             }
             
             usleep(50000);
      	   }
           
           usleep(50000);
      	 }
      	#endif
    	  
    	  //6.�ܼ���ʵʱ������������ݣ����������������
    	  if (totalAddGroup.numberOfzjz != 0)
    	  {
    	    //�ܼ���ʵʱ�������������
    	    for (i = 0; i < totalAddGroup.numberOfzjz; i++)
    	    {
    	  	  flagOfStoreData &= ~STORE_ZJZ_BALANCE_DATA;
    	  	  
    	  	  //��ʼ�������û���
            memset(balanceZjzData,0xee,LEN_OF_ZJZ_BALANCE_RECORD);
            
            //�ܼ��������ͳ��
            if (groupBalance(balanceZjzData, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK, balanceTime)==TRUE)
            {
            	 flagOfStoreData |= STORE_ZJZ_BALANCE_DATA;
            }
    
            usleep(50000);
            
            if (groupBalance(balanceZjzData, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_NO_WORK, balanceTime)==TRUE)
            {
            	 flagOfStoreData |= STORE_ZJZ_BALANCE_DATA;
            }
            usleep(50000);
    	  	  
    	  	  //�ܼ��鹦��ͳ��
            if (groupStatistic(balanceZjzData, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK, balanceTime)==TRUE)
            {
            	 flagOfStoreData |= STORE_ZJZ_BALANCE_DATA;
            }
            usleep(50000);
    
            if (groupStatistic(balanceZjzData, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_NO_WORK,balanceTime)==TRUE)
            {
            	 flagOfStoreData |= STORE_ZJZ_BALANCE_DATA;
            }
            usleep(50000);
            
    	  	  //�洢�ܼӽ�����
            if (flagOfStoreData&STORE_ZJZ_BALANCE_DATA)
            {
              saveMeterData(totalAddGroup.perZjz[i].zjzNo, port+1, balanceTime, balanceZjzData, REAL_BALANCE, GROUP_REAL_BALANCE,LEN_OF_ZJZ_BALANCE_RECORD);
              if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
              {
                 //������ν��㴦���ս�����½����ʱ��,��Ӧ��־��λ    
                 balanceZjzData[GP_DAY_OVER] = 0x01;
                 if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
                 {
                   balanceZjzData[GP_MONTH_OVER] = 0x01;
                 }
    
                 saveMeterData(totalAddGroup.perZjz[i].zjzNo, port+1, balanceTime, balanceZjzData, DAY_BALANCE, GROUP_DAY_BALANCE,LEN_OF_ZJZ_BALANCE_RECORD);
                 if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
                 {
                    saveMeterData(totalAddGroup.perZjz[i].zjzNo, port+1, balanceTime, balanceZjzData, MONTH_BALANCE, GROUP_MONTH_BALANCE,LEN_OF_ZJZ_BALANCE_RECORD);
                 }
              }
            }
            usleep(50000);
          }
          
          usleep(50000);
        }
    	  
    	  //�й��ܵ������Խ���¼��ж�
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
    	  	 printf("��ʼ�ж��й��ܵ������Խ���¼�\n");
    	  }
    	  for (i = 0; i < differenceConfig.numOfConfig; i++)
    	  {
    	  	tmpRecord = 0;
    	  	
    	  	if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	{
    	  	  printf("�Ա��ܼ����=%d,�����ܼ����=%d\n",differenceConfig.perConfig[i].toCompare,differenceConfig.perConfig[i].toReference);
    	  	}
    	  	
    	  	tmpTime = balanceTime;
    	  	readMeterData(balanceZjzData, differenceConfig.perConfig[i].toCompare, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0);
    
    	  	if (balanceZjzData[GP_MONTH_WORK]==0xee)
    	  	{
    	  	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	  {
    	  	  	 printf("�Ա��ܼ��������������\n");
    	  	  }
    	  		continue;
    	  	}
    	  	
    	  	compData = balanceZjzData[GP_MONTH_WORK+6]<<24;
    	  	compData |= balanceZjzData[GP_MONTH_WORK+5]<<16;
    	  	compData |= balanceZjzData[GP_MONTH_WORK+4]<<8;
    	  	compData |= balanceZjzData[GP_MONTH_WORK+3];
    	  	compData = bcdToHex(compData);
    	  	
    	  	if (balanceZjzData[GP_MONTH_WORK]&0x1)
    	  	{
    	  		 compData *= 1000;
    	  	}
    	  	if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	{
    	  	  printf("�Ա��ܼ������ܵ�����=%d\n",compData);
    	    }
    	  	
    	  	tmpTime = balanceTime;
    	  	readMeterData(balanceZjzDatax, differenceConfig.perConfig[i].toReference, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0);
    	  	if (balanceZjzDatax[GP_MONTH_WORK]==0xee)
    	  	{
    	  	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	  {
    	  	  	 printf("�ο��ܼ��������������\n");
    	  	  }
    	  	  
    	  		continue;
    	  	}
    	  	diffData = balanceZjzDatax[GP_MONTH_WORK+6]<<24;
    	  	diffData |= balanceZjzDatax[GP_MONTH_WORK+5]<<16;
    	  	diffData |= balanceZjzDatax[GP_MONTH_WORK+4]<<8;
    	  	diffData |= balanceZjzDatax[GP_MONTH_WORK+3];
    	  	if (balanceZjzDatax[GP_MONTH_WORK]&0x1)
    	  	{
    	  		 diffData *= 1000;
    	  	}
    	  	diffData = bcdToHex(diffData);
    	  	if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	{
    	  	  printf("�����ܼ������ܵ�����=%d\n", diffData);
    	  	}
    	  	
    	  	if (differenceConfig.perConfig[i].timeAndFlag&0x80)
    	  	{
    	  	  absoluteData = (differenceConfig.perConfig[i].absoluteDifference[3]&0xf)<<24;
    	  	  absoluteData |= differenceConfig.perConfig[i].absoluteDifference[2]<<16;
    	  	  absoluteData |= differenceConfig.perConfig[i].absoluteDifference[1]<<8;
    	  	  absoluteData |= differenceConfig.perConfig[i].absoluteDifference[0];
    	  	  
    	  	  if (differenceConfig.perConfig[i].absoluteDifference[3]&0x80)
    	  	  {
    	  	  	absoluteData*=1000;
    	  	  }
    	  	  
    	  		if (diffData>compData)
    	  		{
    	  			compData = diffData-compData; 
    	  		}
    	  		else
    	  		{
    	  			compData -= diffData; 
    	  		}
    	  		
    	  		if (compData>absoluteData)
    	  		{
    	  		  if ((differenceConfig.perConfig[i].startStop&0x1)==0x0)
    	  		  {
    	  		  	printf("�й��ܵ��������%d���ԶԱ�Խ�޷���δ��¼���ڼ�¼,���Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 1;
                differenceConfig.perConfig[i].startStop |= 0x1;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  		  }
    	  		  else
    	  		  {
    	  		    printf("�й��ܵ��������%d���ԶԱ�Խ�޷����Ѽ�¼,���Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  }
    	  		}
    	  		else
    	  		{
    	  		  if ((differenceConfig.perConfig[i].startStop&0x1)==0x0)
    	  		  {	  			
    	  			  printf("�й��ܵ��������%d���ԶԱȻָ����ڼ�¼,���Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 2;
                differenceConfig.perConfig[i].startStop &= 0xfe;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  			}
    	  			else
    	  			{
    	  			  printf("�й��ܵ��������%d���ԶԱ�δԽ��,���Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  			}
    	  		}
    	  	}
    	    else
    	    {
    	  		if (diffData==0)
    	  		{
    	  			continue;
    	  		}
    	  		
    	  		if (diffData>compData)
    	  		{
    	  			compData = (diffData-compData)*100/diffData;
    	  		}
    	  		else
    	  		{
    	  			compData = (compData-diffData)*100/diffData;
    	  		}
    	  		
    	  		if (compData>differenceConfig.perConfig[i].ralitaveDifference)
    	  		{
    	  		  if ((differenceConfig.perConfig[i].startStop&0x2)==0x0)
    	  		  {
    	  		    printf("�й��ܵ��������%d��ԶԱ�Խ�޷���δ��¼���ڼ�¼,��Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 3;
                differenceConfig.perConfig[i].startStop |= 0x2;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  		  }
    	  		  else
    	  		  {
    	  		    printf("�й��ܵ��������%d��ԶԱ�Խ�޷����Ѽ�¼,��Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  }	  		  
    	  		}
    	  		else
    	  		{
    	  		  if ((differenceConfig.perConfig[i].startStop&0x2)==0x2)
    	  		  {
    	  			  printf("�й��ܵ��������%d��ԶԱ�δԽ�޻ָ����ڼ�¼,��Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 4;
                differenceConfig.perConfig[i].startStop &= 0xfd;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  		  }
    	  		  else
    	  		  {
    	  			  printf("�й��ܵ��������%d��ԶԱ�δԽ��,��Բ�ֵ=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  			}
    	  		}
    	    }
    	    
    	    if (tmpRecord>0)
    	    {
            //�����й��������Խ�޼�¼
            if ((eventRecordConfig.iEvent[2]&0x20)||(eventRecordConfig.nEvent[2] & 0x20))
            {
       	    	eventData[0] = 22;       //ERC
       	    	eventData[1] = 34;       //����
       	    	 
       	    	//����ʱ��
              eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
              eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
              eventData[4] = sysTime.hour  /10<<4 | sysTime.hour  %10;
              eventData[5] = sysTime.day   /10<<4 | sysTime.day   %10;
              eventData[6] = sysTime.month /10<<4 | sysTime.month %10;
              eventData[7] = sysTime.year  /10<<4 | sysTime.year  %10;
               
              //����������
              eventData[8] = differenceConfig.perConfig[i].groupNum;
              
              //������־
              if (tmpRecord==1 || tmpRecord==3)
              {
              	 eventData[8] |= 0x80;
              }
              
              
              //Խ��ʱ�Ա��ܼ����й��ܵ�����
              eventData[9]  = balanceZjzData[GP_MONTH_WORK+3];
              eventData[10] = balanceZjzData[GP_MONTH_WORK+4];
              eventData[11] = balanceZjzData[GP_MONTH_WORK+5];
              eventData[12] = ((balanceZjzData[GP_MONTH_WORK]&0x01)<<6)
                            |(balanceZjzData[GP_MONTH_WORK]&0x10)
                            |(balanceZjzData[GP_MONTH_WORK+6]&0x0f);
    
              //Խ��ʱ�����ܼ����й��ܵ�����
              eventData[13] = balanceZjzDatax[GP_MONTH_WORK+3];
              eventData[14] = balanceZjzDatax[GP_MONTH_WORK+4];
              eventData[15] = balanceZjzDatax[GP_MONTH_WORK+5];
              eventData[16] = ((balanceZjzDatax[GP_MONTH_WORK]&0x01)<<6)
                            |(balanceZjzDatax[GP_MONTH_WORK]&0x10)
                            |(balanceZjzDatax[GP_MONTH_WORK+6]&0x0f);
              
              //Խ��ʱ�Խ�����ƫ��ֵ
    	  	    if (differenceConfig.perConfig[i].timeAndFlag&0x80)
    	  	    {
    	  	    	eventData[17] = 0x0;
    	  	    }
    	  	    else
    	  	    {
    	  	    	eventData[17] = compData;
    	  	    }
    	  	    
    	  	    //Խ��ʱ�Խ�޾���ƫ��ֵ
    	  	    if (differenceConfig.perConfig[i].timeAndFlag&0x80)
    	  	    {
    	  	    	 compData = hexToBcd(compData);
    	  	    	 eventData[18] = compData&0xff;
    	  	    	 eventData[19] = compData>>8&0xff;
    	  	    	 eventData[20] = compData>>16&0xff;
    	  	    	 eventData[21] = compData>>24&0xff;
    	  	    }
    	  	    else
    	  	    {
    	  	    	 eventData[18] = 0x00;
    	  	    	 eventData[19] = 0x00;
    	  	    	 eventData[20] = 0x00;
    	  	    	 eventData[21] = 0x00;
    	  	    }
    	  	    
    	  	    //�Ա��ܼ������������n
    	  	    eventData[22] = 0;
    	  	    
    	  	    //�Ա��ܼ��������1�й��ܵ���ʾֵ
    	  	    tmpTail = 23;
    	  	    
    	  	    for (j = 0; j < totalAddGroup.numberOfzjz; j++)
    	  	    {
    	  	      if (totalAddGroup.perZjz[j].zjzNo==differenceConfig.perConfig[i].toCompare)
    	  	      {
    	  	        for(k=0;k<totalAddGroup.perZjz[j].pointNumber;k++)
    	  	        {
    	  	        	eventData[22]++;
    	  	        	tmpTime = balanceTime;
                    if (readMeterData(lastCopyEnergyData, (totalAddGroup.perZjz[j].measurePoint[k]&0x3f)+1, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == FALSE)
                    {
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;                	 
                    }
                    else
                    {
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET];
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET+1];
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET+2];
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET+3];
    	  	          }
    	  	        }
    	  	      }
    	  	    }
    
    	  	    //�����ܼ������������n
    	  	    tmpTailx = tmpTail++;
    	  	    eventData[tmpTailx] = 0;
    
    	  	    //�����ܼ��������1�й��ܵ���ʾֵ
    	  	    for (j = 0; j < totalAddGroup.numberOfzjz; j++)
    	  	    {
    	  	      if (totalAddGroup.perZjz[j].zjzNo==differenceConfig.perConfig[i].toReference)
    	  	      {
    	  	        for(k=0;k<totalAddGroup.perZjz[j].pointNumber;k++)
    	  	        {
    	  	        	eventData[tmpTailx]++;
    	  	        	tmpTime = balanceTime;
                    if (readMeterData(lastCopyEnergyData, (totalAddGroup.perZjz[j].measurePoint[k]&0x3f)+1, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == FALSE)
                    {
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = 0x00;                	 
                    }
                    else
                    {
    	  	            eventData[tmpTail++] = 0x00;
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET];
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET+1];
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET+2];
    	  	            eventData[tmpTail++] = lastCopyEnergyData[POSITIVE_WORK_OFFSET+3];
    	  	          }
    	  	        }
    	  	      }
    	  	    }
    	  	    
              
              if (eventRecordConfig.iEvent[2] & 0x20)
              {
                writeEvent(eventData, 34 , 1, DATA_FROM_GPRS);
              }
              if (eventRecordConfig.nEvent[2] & 0x20)
              {
                writeEvent(eventData, 34 , 2, DATA_FROM_LOCAL);
              }
               
              if (debugInfo&PRINT_EVENT_DEBUG)
              {
                printf("�й��ܵ������Խ�޵��������ϱ�\n");
              }
                 
              activeReport3();   //�����ϱ��¼�
            }
    	    }
    	  }
    	    
        #ifdef LOAD_CTRL_MODULE
         computeLeftPower(copyCtrl[port].lastCopyTime);
         
         balanceComplete = TRUE;
        #endif
        
        //ly,2011-08-22,add
        processOverLimit(port);
    
        if (debugInfo&PRINT_EVENT_DEBUG)
        {
          printf("������� �����ϱ�\n");
        }
        
        activeReport3();                  //�����ϱ��¼�
        
        copyCtrl[port].ifRealBalance = 0; //ʵʱ�����־��λ
    
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	 printf("�˿�%d�������\n", port+1);
        }
        
        usleep(50000);
      }
      
      usleep(50000);
    }
}

/*******************************************************
��������: statisticMaxDemand
��������: �������ͳ��
���ú���:     
�����ú���:
�������:*copyBuff,��һ�γ�����������ݻ���
         *balanceBuff,�α������㻺��
�������:  
����ֵ:void
*******************************************************/
void statisticMaxDemand(INT8U *copyReqBuff,INT8U *balanceParaBuff)
{
	  //������һ�ε�ͳ�ƽ������ͳ�Ƽ���
	  if (balanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)
	  {
      if (balanceParaBuff[MAX_TOTAL_REQ] != 0xEE)  //��һ��ͳ�ƽ������
      {
        if (copyReqBuff[REQ_POSITIVE_WORK_OFFSET] != 0xEE)  //���γ���ɹ�����
        {
          if ((copyReqBuff[REQ_POSITIVE_WORK_OFFSET+2] > balanceParaBuff[MAX_TOTAL_REQ+2])
            || (copyReqBuff[REQ_POSITIVE_WORK_OFFSET+2] == balanceParaBuff[MAX_TOTAL_REQ+2]&&copyReqBuff[REQ_POSITIVE_WORK_OFFSET+1] > balanceParaBuff[MAX_TOTAL_REQ+1])
              || (copyReqBuff[REQ_POSITIVE_WORK_OFFSET+2] == balanceParaBuff[MAX_TOTAL_REQ+2]&&copyReqBuff[REQ_POSITIVE_WORK_OFFSET+1] == balanceParaBuff[MAX_TOTAL_REQ+1]&&copyReqBuff[REQ_POSITIVE_WORK_OFFSET] > balanceParaBuff[MAX_TOTAL_REQ]))
          {
            balanceParaBuff[MAX_TOTAL_REQ]   = copyReqBuff[REQ_POSITIVE_WORK_OFFSET];
            balanceParaBuff[MAX_TOTAL_REQ+1] = copyReqBuff[REQ_POSITIVE_WORK_OFFSET+1];
            balanceParaBuff[MAX_TOTAL_REQ+2] = copyReqBuff[REQ_POSITIVE_WORK_OFFSET+2];
        
            balanceParaBuff[MAX_TOTAL_REQ_TIME]   = copyReqBuff[REQ_TIME_P_WORK_OFFSET];
            balanceParaBuff[MAX_TOTAL_REQ_TIME+1] = copyReqBuff[REQ_TIME_P_WORK_OFFSET+1];
            balanceParaBuff[MAX_TOTAL_REQ_TIME+2] = copyReqBuff[REQ_TIME_P_WORK_OFFSET+2];
          }
        }
      }
      else  //��һ��ͳ�ƽ��������
      {
        if (copyReqBuff[REQ_POSITIVE_WORK_OFFSET] != 0xEE)
        {
          balanceParaBuff[MAX_TOTAL_REQ]   = copyReqBuff[REQ_POSITIVE_WORK_OFFSET];
          balanceParaBuff[MAX_TOTAL_REQ+1] = copyReqBuff[REQ_POSITIVE_WORK_OFFSET+1];
          balanceParaBuff[MAX_TOTAL_REQ+2] = copyReqBuff[REQ_POSITIVE_WORK_OFFSET+2];
        
          balanceParaBuff[MAX_TOTAL_REQ_TIME]   = copyReqBuff[REQ_TIME_P_WORK_OFFSET];
          balanceParaBuff[MAX_TOTAL_REQ_TIME+1] = copyReqBuff[REQ_TIME_P_WORK_OFFSET+1];
          balanceParaBuff[MAX_TOTAL_REQ_TIME+2] = copyReqBuff[REQ_TIME_P_WORK_OFFSET+2];
        }
      }
    }
    else  //���¿�ʼͳ�ƹ���
    {
      if (copyReqBuff[REQ_POSITIVE_WORK_OFFSET] != 0xEE)
  	  {
  	    balanceParaBuff[MAX_TOTAL_REQ]   = copyReqBuff[REQ_POSITIVE_WORK_OFFSET];
  	    balanceParaBuff[MAX_TOTAL_REQ+1] = copyReqBuff[REQ_POSITIVE_WORK_OFFSET+1];
  	    balanceParaBuff[MAX_TOTAL_REQ+2] = copyReqBuff[REQ_POSITIVE_WORK_OFFSET+2];
  	          
  	    balanceParaBuff[MAX_TOTAL_REQ_TIME]   = copyReqBuff[REQ_TIME_P_WORK_OFFSET];
  	    balanceParaBuff[MAX_TOTAL_REQ_TIME+1] = copyReqBuff[REQ_TIME_P_WORK_OFFSET+1];
  	    balanceParaBuff[MAX_TOTAL_REQ_TIME+2] = copyReqBuff[REQ_TIME_P_WORK_OFFSET+2];
  	  }
  	}
}

/*******************************************************
��������: statisticPower
��������: ����ͳ�� 
���ú���:     
�����ú���:
�������:INT8U *copyParaBuff,�ϴγ���Ĳα�������
         INT8U *balanceParaBuff,����α�������
         INT8U statisInterval,ͳ�Ƽ��
         DATE_TIME statisTime,ͳ��ʱ��(��һ�γ���ʱ��)
�������:  
����ֵ:
*******************************************************/
void statisticPower(INT8U *copyParaBuff, INT8U *balanceParaBuff, INT8U statisInterval, DATE_TIME statisTime)
{
   INT16U  offset, offsetSta, tmpData;
   INT8U   i;

   //������һ�ε�ͳ�ƽ������ͳ�Ƽ���
   if (balanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)
   {
   	 offset = POWER_INSTANT_WORK;
   	 offsetSta = MAX_TOTAL_POWER;
     for (i = 0; i < 4; i++)
     {
       //�й�����ͳ��
   	  if (copyParaBuff[offset] != 0xEE)
   	  {
   	    //�й��������ֵͳ��
   	    if (balanceParaBuff[offsetSta] == 0xEE)
   	    {
   	    	//�й��������ֵ
   	    	balanceParaBuff[offsetSta]   = copyParaBuff[offset];
   	    	balanceParaBuff[offsetSta+1] = copyParaBuff[offset+1];
   	    	balanceParaBuff[offsetSta+2] = copyParaBuff[offset+2];
   	    	    
   	    	//���й��������ֵʱ��
   	    	balanceParaBuff[offsetSta+3] = statisTime.minute;
   	    	balanceParaBuff[offsetSta+4] = statisTime.hour;
   	    	balanceParaBuff[offsetSta+5] = statisTime.day;
   	    }
   	    else
   	    {
   	    	if ((copyParaBuff[offset+2] > balanceParaBuff[offsetSta+2])
   	    	 	|| ((copyParaBuff[offset+2] == balanceParaBuff[offsetSta+2])&&(copyParaBuff[offset+1] > balanceParaBuff[offsetSta+1]))
   	    		  || ((copyParaBuff[offset+2] == balanceParaBuff[offsetSta+2])&&(copyParaBuff[offset+1] == balanceParaBuff[offsetSta+1])&&(copyParaBuff[offset] > balanceParaBuff[offsetSta])))
   	    	{
   	    	  //���й��������ֵ
   	    	  balanceParaBuff[offsetSta] = copyParaBuff[offset];
   	    	  balanceParaBuff[offsetSta+1] = copyParaBuff[offset+1];
   	    	  balanceParaBuff[offsetSta+2] = copyParaBuff[offset+2];
   	    	      
   	    	  //���й��������ֵʱ��
   	    	  balanceParaBuff[offsetSta+3] = statisTime.minute;
   	    	  balanceParaBuff[offsetSta+4] = statisTime.hour;
   	    	  balanceParaBuff[offsetSta+5] = statisTime.day;
   	    	}
   	    }
   	    	  
   	    //���й�����Ϊ��ʱ��
   	    if (copyParaBuff[offset] == 0x00 && copyParaBuff[offset+1] == 0x00 && copyParaBuff[offset+2] == 0x00)
   	    {
   	    	if (balanceParaBuff[offsetSta+6]==0xEE
   	    	 	||(balanceParaBuff[offsetSta+6]==0x00&&balanceParaBuff[offsetSta+7]==0x00))
   	    	{
   	    	  balanceParaBuff[offsetSta+6] = statisInterval&0xFF;
   	    	  balanceParaBuff[offsetSta+7] = statisInterval>>8&0xFF;
   	    	}
   	    	else
   	    	{
   	    	  tmpData = balanceParaBuff[offsetSta+6] | balanceParaBuff[offsetSta+7]<<8;
   	    	  tmpData += statisInterval;
   	    	    
   	    	  balanceParaBuff[offsetSta+6] = tmpData&0xFF;
   	    	  balanceParaBuff[offsetSta+7] = tmpData>>8&0xFF;
   	    	}
   	    }
   	  }
 	    	
 	    offset += 3;
 	    offsetSta += 8;
 	   }
   }
 	 else
 	 {
 		 offset = POWER_INSTANT_WORK;
 		 offsetSta = MAX_TOTAL_POWER;
 	   for (i = 0; i < 4; i++)
 	   {
 	     if (copyParaBuff[offset] != 0xEE)
   	   {
   	     //�й��������ֵ
   	     balanceParaBuff[offsetSta]   = copyParaBuff[offset];
   	     balanceParaBuff[offsetSta+1] = copyParaBuff[offset+1];
   	     balanceParaBuff[offsetSta+2] = copyParaBuff[offset+2];
   	    	    
   	     //���й��������ֵʱ��
   	     balanceParaBuff[offsetSta+3] = statisTime.minute;
   	     balanceParaBuff[offsetSta+4] = statisTime.hour;
   	     balanceParaBuff[offsetSta+5] = statisTime.day;
   	   }

   	   if (copyParaBuff[offset] == 0x00 && copyParaBuff[offset+1] == 0x00 && copyParaBuff[offset+2] == 0x00)
   	   {
   	     //�й�����Ϊ��ʱ��
   	     balanceParaBuff[offsetSta+6] = statisInterval&0xFF;
   	     balanceParaBuff[offsetSta+7] = statisInterval>>8&0xFF;
   	   }

 	     offset += 3;
 	     offsetSta += 8;
 	   }
 	 }
 	
 	 if (debugInfo&PRINT_BALANCE_DEBUG)
 	 {
 		 printf("statisticPower:�й��������ֵ=%02x%02x%02x\n",balanceParaBuff[offsetSta+2],balanceParaBuff[offsetSta+1],balanceParaBuff[offsetSta+0]);
 		 printf("statisticPower:�й��������ֵʱ��=%02x %02x:%02x\n",balanceParaBuff[offsetSta+5],balanceParaBuff[offsetSta+4],balanceParaBuff[offsetSta+3]);
 		 printf("statisticPower:�й�����Ϊ��ʱ��=%0d\n", balanceParaBuff[offsetSta+6] | balanceParaBuff[offsetSta+7]<<8);
 	 }
}

/*******************************************************
��������: statisticApparentPowerAndFactor
��������: ���ڹ���ͳ�Ƽ����ڹ���Խ���¼��ж���������������ͳ��
���ú���:     
�����ú���:
�������:INT16U pn,������(1-2040)
         INT8U *copyParaBuff,�ϴγ���α������� 
         INT8U *balanceParaBuff,����α�������
         MEASUREPOINT_LIMIT_PARA *pLimit,����������ֵ����ָ��
         INT8U statisInterval,ͳ�Ƽ��
�������:  
����ֵ:
*******************************************************/
void statisticApparentPowerAndFactor(INT16U pn, INT8U *copyParaBuff, INT8U *balanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit, INT8U statisInterval,DATE_TIME statisTime)
{
    INT32U       sumInt, rawInt;
    INT32U       appPower, appPowerLimit, appPowerBCD;
    INT32U       tmpData;
    INT16U       factorSeg1, factorSeg2;
    INT32U       totalFactor;

  	POWER_SEG_LIMIT *pPowerSegLimit;
    
	  pPowerSegLimit = (POWER_SEG_LIMIT *)malloc(sizeof(POWER_SEG_LIMIT));
  	
  	if(selectViceParameter(0x04, 28, pn, (INT8U *)pPowerSegLimit, sizeof(POWER_SEG_LIMIT)) == FALSE)
  	{
  		pPowerSegLimit = NULL;
  	}
  	  
    INT32U      appUpUpResume,appUpResume;    //���ڹ��ʻָ���ֵ
    
    //���¿�ʼͳ��
    if (balanceParaBuff[NEXT_NEW_INSTANCE] == START_NEW_INSTANCE)
    {
      balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = 0x00;
      balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = 0x00;
      balanceParaBuff[APPARENT_POWER_UP_TIME] = 0x00;
      balanceParaBuff[APPARENT_POWER_UP_TIME+1] = 0x00;

      balanceParaBuff[FACTOR_SEG_1] = 0x00;
      balanceParaBuff[FACTOR_SEG_1+1] = 0x00;
      balanceParaBuff[FACTOR_SEG_2] = 0x00;
      balanceParaBuff[FACTOR_SEG_2+1] = 0x00;
      balanceParaBuff[FACTOR_SEG_3] = 0x00;
      balanceParaBuff[FACTOR_SEG_3+1] = 0x00;
    }

    //���ڹ���ͳ��
    if (pLimit != NULL)
    {
      if (copyParaBuff[POWER_INSTANT_WORK] != 0xEE && copyParaBuff[TOTAL_POWER_FACTOR] != 0xEE)
      {
        //ͨ���й����ʺ͹��������������ڹ���
        //���ڹ���=�й�����/��������
        sumInt = copyParaBuff[POWER_INSTANT_WORK] | copyParaBuff[POWER_INSTANT_WORK+1]<<8 | (copyParaBuff[POWER_INSTANT_WORK+2]&0x7f)<<16;  	        
        sumInt = bcdToHex(sumInt);
        rawInt = copyParaBuff[TOTAL_POWER_FACTOR] | (copyParaBuff[TOTAL_POWER_FACTOR+1]&0x7f)<<8;
        rawInt = bcdToHex(rawInt);
        
        if (rawInt==0)
        {
          //ly,2011-10-10,�����������
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("���ڹ���ͳ��:��������Ϊ0,���ܼ�������ڹ���\n");
          }
          
          goto factorPoint;
        }
        else
        {
          appPower = sumInt*1000/rawInt;
        }
        
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("���ڹ���ͳ��:���ڹ���=%d\n",appPower);
        }
        
        //���ڹ���������
        appPowerLimit = pLimit->pSuperiodLimit[0] | pLimit->pSuperiodLimit[1]<<8 | pLimit->pSuperiodLimit[2]<<16;
        appPowerLimit = bcdToHex(appPowerLimit);
        
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("���ڹ���ͳ��:��������ֵ=%d\n",appPowerLimit);
        }
        
        appUpResume = calcResumeLimit(pLimit->pUpLimit[0] | pLimit->pUpLimit[1]<<8 | pLimit->pUpLimit[2]<<16, pLimit->pUpResume[0] | pLimit->pUpResume[1]<<8);
        appUpUpResume = calcResumeLimit(pLimit->pSuperiodLimit[0] | pLimit->pSuperiodLimit[1]<<8 | pLimit->pSuperiodLimit[2]<<16, pLimit->pSuperiodResume[0] | pLimit->pSuperiodResume[1]<<8);      

        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("���ڹ���ͳ��:���޻ָ���ֵ=%d\n",appUpResume);
          printf("���ڹ���ͳ��:�����޻ָ���ֵ=%d\n",appUpUpResume);
        }
                
        if (appPower != 0)
        {
          //���ڹ���Խ������
          if (appPower > appPowerLimit)
          {
            if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
            {
              //��һ�η���Խ������,��¼Խ���¼�
              if ((pStatisRecord->apparentPower&0x01) == 0x00)
              {
          	     if (pStatisRecord->apparentUpUpTime.year==0xff)
          	     {
          	   	   pStatisRecord->apparentUpUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pSuperiodTimes, 0);
          	   	   
          	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
          	   	   {
          	   	     printf("���ڹ���ͳ��:Խ�����޷���,%d���Ӻ��¼\n",pLimit->pSuperiodTimes);
          	   	   }
          	     }
          	     else
          	     {
                   if (compareTwoTime(pStatisRecord->apparentUpUpTime,sysTime))
                   {
                 	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   {
                 	     printf("���ڹ���ͳ��:Խ�����޷�������ʱ���ѵ�\n");
                 	   }
                 	 
                 	   pStatisRecord->apparentUpUpTime.year = 0xff;
                     
                     pStatisRecord->apparentPower |= 0x01;
                     appPowerBCD = hexToBcd(appPower);
                     pOverLimitEvent(pn, 1, FALSE, (INT8U *)&appPowerBCD, pLimit, statisTime);
          	       }
          	     }
          	  }               	   
          	  else
          	  {
                pStatisRecord->apparentUpUpTime.year = 0xff;
          	  }
            }
            
            //Խ������ʱ��
            if (balanceParaBuff[APPARENT_POWER_UP_UP_TIME]==0xEE)
            {
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = statisInterval&0xff;
              //balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = statisInterval>>8&0xff;
              //ly,2011-10-26,���ִ���(δ��1)
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = statisInterval>>8&0xff;
            }
            else
            {
              tmpData = balanceParaBuff[APPARENT_POWER_UP_UP_TIME] | balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1]<<8;
              tmpData += statisInterval;
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = tmpData&0xFF;
              //balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = tmpData>>8&0xFF;
              //ly,2011-10-26,���ִ���(δ��1)
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = tmpData>>8&0xFF;
            }
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
            	 printf("���ڹ���ͳ��Խ������ʱ��=%d\n", tmpData);
            }
            
            //ly,2011-10-20,add,Խ�����޵Ļ�,�϶�Խ������,��˼���Խ����ʱ��
            //Խ����ʱ��
            if (balanceParaBuff[APPARENT_POWER_UP_TIME]==0xEE)
            {
              balanceParaBuff[APPARENT_POWER_UP_TIME] = statisInterval&0xff;
              balanceParaBuff[APPARENT_POWER_UP_TIME+1] = statisInterval>>8&0xff;
            }
            else
            {
              tmpData = balanceParaBuff[APPARENT_POWER_UP_TIME] | balanceParaBuff[APPARENT_POWER_UP_TIME+1]<<8;
              tmpData += statisInterval;
              balanceParaBuff[APPARENT_POWER_UP_TIME] = tmpData&0xFF;
              balanceParaBuff[APPARENT_POWER_UP_TIME+1] = tmpData>>8&0xFF;
            }
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
            	 printf("���ڹ���ͳ��Խ����(Խ������ʱ)ʱ��=%d\n", tmpData);
            }
          }
          else
          {
            if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
            {
            	//Խ�����޻ָ�
               if (appPower<=appUpUpResume)
               {
              	//��������Խ������,��¼Խ�����޻ָ�
                if ((pStatisRecord->apparentPower&0x01) == 0x01)
                {
            	     if (pStatisRecord->apparentUpUpTime.year==0xff)
            	     {
            	   	   pStatisRecord->apparentUpUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pSuperiodTimes, 0);
            	   	   printf("���ڹ���ͳ��:Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->pSuperiodTimes);
            	     }
            	     else
            	     {
                     if (compareTwoTime(pStatisRecord->apparentUpUpTime,sysTime))
                     {
                  	   if (debugInfo&PRINT_BALANCE_DEBUG)
                  	   {
                  	     printf("���ڹ���ͳ��:�����޻ָ�����ʱ���ѵ�\n");
                  	   }
                  	 
                  	   pStatisRecord->apparentUpUpTime.year = 0xff;
            	   
                       pStatisRecord->apparentPower &= 0xFE;
                       appPowerBCD = hexToBcd(appPower);
                       pOverLimitEvent(pn, 1, TRUE, (INT8U *)&appPowerBCD, pLimit, statisTime);

            	       }
            	     }
            	   }
            	   else
            	   {
                   pStatisRecord->apparentUpUpTime.year = 0xff;
            	   }
            	  }
            }
            
            //���ڹ�������
            appPowerLimit = pLimit->pUpLimit[0] | pLimit->pUpLimit[1]<<8 | pLimit->pUpLimit[2]<<16;
            appPowerLimit = bcdToHex(appPowerLimit);
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("���ڹ���ͳ��������ֵ%d\n",appPowerLimit);
            }
          
            //���ڹ���Խ����
            if (appPower > appPowerLimit)
            {
              if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
              {
                //��һ�η���Խ����,��¼Խ���¼�
                if ((pStatisRecord->apparentPower&0x10) == 0x00)
                {
            	    if (pStatisRecord->apparentUpTime.year==0xff)
            	    {
            	   	  pStatisRecord->apparentUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pUpTimes, 0);
            	   	   
            	   	  if (debugInfo&PRINT_BALANCE_DEBUG)
            	   	  {
            	   	    printf("���ڹ���ͳ��:Խ���޷���,%d���Ӻ��¼\n",pLimit->pUpTimes);
            	   	  }
            	    }
            	    else
            	    {
                    if (compareTwoTime(pStatisRecord->apparentUpTime,sysTime))
                    {
                   	  if (debugInfo&PRINT_BALANCE_DEBUG)
                   	  {
                   	    printf("���ڹ���ͳ��:Խ���޷�������ʱ���ѵ�\n");
                   	  }
                   	 
                   	  pStatisRecord->apparentUpTime.year = 0xff;
                       
                      pStatisRecord->apparentPower |= 0x10;
                      appPowerBCD = hexToBcd(appPower);
                      pOverLimitEvent(pn, 2, FALSE, (INT8U *)&appPowerBCD, pLimit, statisTime);
            	      }
            	    }
            	  }               	   
            	  else
            	  {
                  pStatisRecord->apparentUpTime.year = 0xff;
            	  }
              }
              
              //Խ����ʱ��
              if (balanceParaBuff[APPARENT_POWER_UP_TIME]==0xEE)
              {
                balanceParaBuff[APPARENT_POWER_UP_TIME] = statisInterval&0xff;
                balanceParaBuff[APPARENT_POWER_UP_TIME+1] = statisInterval>>8&0xff;
              }
              else
              {
                tmpData = balanceParaBuff[APPARENT_POWER_UP_TIME] | balanceParaBuff[APPARENT_POWER_UP_TIME+1]<<8;
                tmpData += statisInterval;
                balanceParaBuff[APPARENT_POWER_UP_TIME] = tmpData&0xFF;
                balanceParaBuff[APPARENT_POWER_UP_TIME+1] = tmpData>>8&0xFF;
              }
              
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
            	  printf("���ڹ���ͳ��Խ����ʱ��=%d\n", tmpData);
              }
            }
            else
            {
              if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
              {
              	//Խ���޻ָ�
                 if (appPower<=appUpResume)
                 {
                	//��������Խ����,��¼Խ���޻ָ�
                  if ((pStatisRecord->apparentPower&0x10) == 0x10)
                  {
              	     if (pStatisRecord->apparentUpTime.year==0xff)
              	     {
              	   	   pStatisRecord->apparentUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pUpTimes, 0);
              	   	   printf("���ڹ���ͳ��:Խ���޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->pUpTimes);
              	     }
              	     else
              	     {
                       if (compareTwoTime(pStatisRecord->apparentUpTime,sysTime))
                       {
                    	   if (debugInfo&PRINT_BALANCE_DEBUG)
                    	   {
                    	     printf("���ڹ���ͳ��:���޻ָ�����ʱ���ѵ�\n");
                    	   }
                    	 
                    	   pStatisRecord->apparentUpTime.year = 0xff;
              	   
                         pStatisRecord->apparentPower &= 0xEF;
                         appPowerBCD = hexToBcd(appPower);
                         pOverLimitEvent(pn, 2, TRUE, (INT8U *)&appPowerBCD, pLimit, statisTime);
  
              	       }
              	     }
              	   }
              	   else
              	   {
                     pStatisRecord->apparentUpTime.year = 0xff;
              	   }
              	  }
              }
              
              if (balanceParaBuff[APPARENT_POWER_UP_UP_TIME] == 0xEE)
              {
                balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = 0x00;
                balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = 0x00;
              }
              
              if (balanceParaBuff[APPARENT_POWER_UP_TIME] == 0xEE)
              {
                balanceParaBuff[APPARENT_POWER_UP_TIME] = 0x00;
                balanceParaBuff[APPARENT_POWER_UP_TIME+1] = 0x00;
              }
            }
          }
        }
      }
    }

factorPoint:
  	//2.�������������ۼ�ʱ��
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("��ʼͳ�ƹ������������ۼ�ʱ��\n");
    }
    if (pPowerSegLimit!=NULL)
    {
      totalFactor = bcdToHex((copyParaBuff[TOTAL_POWER_FACTOR]|copyParaBuff[TOTAL_POWER_FACTOR+1]<<8)&0x7fff);
      factorSeg1  = bcdToHex((pPowerSegLimit->segLimit1[0] | pPowerSegLimit->segLimit1[1]<<8)&0x7fff);
      factorSeg2  = bcdToHex((pPowerSegLimit->segLimit2[0] | pPowerSegLimit->segLimit2[1]<<8)&0x7fff);
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
      	printf("�������������ۼ�:�ܹ�������=%d\n", totalFactor);
      	
      	printf("�������������ۼ�:�ֶ���ֵ1=%d\n", factorSeg1);
      	printf("�������������ۼ�:�ֶ���ֵ2=%d\n", factorSeg2);
      }
      
      if (totalFactor<factorSeg1)    //��������<��ֵ1
      {
      	 tmpData = balanceParaBuff[FACTOR_SEG_1] | balanceParaBuff[FACTOR_SEG_1+1]<<8;
         if (debugInfo&PRINT_BALANCE_DEBUG)
         {
          	printf("�������������ۼ�:��������<��ֵ1(ԭ���ķ�����=%d)\n", tmpData);
         }
      	 tmpData += statisInterval;
      	 
      	 balanceParaBuff[FACTOR_SEG_1] = tmpData & 0xff;
      	 balanceParaBuff[FACTOR_SEG_1+1] = tmpData>>8 & 0xff;
         
      }
      else
      {
      	if (totalFactor<factorSeg2)  //��ֵ1<=��������<��ֵ2
      	{
      	  tmpData = balanceParaBuff[FACTOR_SEG_2] | balanceParaBuff[FACTOR_SEG_2+1]<<8;
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
          	 printf("�������������ۼ�:��ֵ1<=��������<��ֵ2(ԭ���ķ�����=%d)\n", tmpData);
          }
      	  tmpData += statisInterval;
      	 
      	  balanceParaBuff[FACTOR_SEG_2] = tmpData & 0xff;
      	  balanceParaBuff[FACTOR_SEG_2+1] = tmpData>>8 & 0xff;      		 
      	}
      	else                         //��������>=��ֵ2
      	{
      	  tmpData = balanceParaBuff[FACTOR_SEG_3] | balanceParaBuff[FACTOR_SEG_3+1]<<8;
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
          	 printf("�������������ۼ�:��������>=��ֵ2(ԭ���ķ�����=%d)\n", tmpData);
          }
      	  tmpData += statisInterval;
      	  
      	  balanceParaBuff[FACTOR_SEG_3] = tmpData & 0xff;
      	  balanceParaBuff[FACTOR_SEG_3+1] = tmpData>>8 & 0xff;      		 
      	}
      }
    }
}

/*******************************************************
��������: statisticVoltage
��������: ��ѹͳ�� 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ:
*******************************************************/
void statisticVoltage(INT16U pn, INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit,INT8U statisInterval,DATE_TIME statisTime)
{
   INT32U             voltage;
   INT16U             volLoss;
   INT8U              j, phase, bitShift;
   INT16U             offset, offsetCurrent, offsetSta, offsetExtre;
   INT16U             tmpDataShort;
	 
	 MEASURE_POINT_PARA pointPara;
	 INT16U             vUpUpResume,vDownDownResume;   //�ָ�ֵ

   offset = VOLTAGE_PHASE_A;
   offsetCurrent = CURRENT_PHASE_A;
   offsetSta = VOL_A_UP_UP_TIME;
   offsetExtre = VOL_A_MAX;
   bitShift = 0x01;
  	
   volLoss = 0xFFFF;

   //ȡ���ѹ��80%Ϊʧѹ��ֵ
	 if(selectViceParameter(0x04, 25, pn, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
	 {
     volLoss = (INT16U)bcdToHex(pointPara.ratingVoltage);
     volLoss *= 4;
     volLoss /= 5;     
   }
   
 	 if (pLimit != NULL)
 	 {
     vUpUpResume = calcResumeLimit(pLimit->vSuperiodLimit, pLimit->vUpUpResume[0] | pLimit->vUpUpResume[1]<<8);
     vDownDownResume = calcResumeLimit(pLimit->vDownDownLimit, pLimit->vDownDownResume[0] | pLimit->vDownDownResume[1]<<8);      

     if (debugInfo&PRINT_BALANCE_DEBUG)
     {
       printf("��ѹͳ��:�����޻ָ���ֵ=%d\n",vUpUpResume);
       printf("��ѹͳ��:�����޻ָ���ֵ=%d\n",vDownDownResume);
     }
 	 }
   
   if (debugInfo&PRINT_BALANCE_DEBUG)
   {
    printf("��ѹͳ��:ʧѹ��ֵ=%d\n",volLoss);

    if (pLimit!=NULL)
    {
      printf("��ѹͳ��:������ֵ=%x\n",pLimit->vSuperiodLimit);
      printf("��ѹͳ��:������ֵ=%x\n",pLimit->vDownDownLimit);
    }
    else
    {
    	printf("��ѹͳ��:δ���ò�����%d��ֵ����\n", pn);
    }
   }
   
   for (phase = 1; phase <= 3; phase++)
   {
    	//����ǰһ��ͳ��
    	if (pBalanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)
      {
 	    	if (pCopyParaBuff[offset] != 0xEE)
   	    {
   	    	//��ѹ�쳣�¼���ʧѹ�ж�
 	        if (volLoss != 0xFFFF)
 	        {
 	        	voltage = (INT16U)bcdToHex(pCopyParaBuff[offset]|pCopyParaBuff[offset+1]<<8);
 	        	
      	    if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
            {
      	       //��ѹС��ʧѹֵ�ҵ���������0�ж�Ϊʧѹ
      	       if (voltage < volLoss && (pCopyParaBuff[offsetCurrent] != 0x00 || pCopyParaBuff[offsetCurrent+1] != 0x00 ||pCopyParaBuff[offsetCurrent+2] != 0x00))
      	       {
                  //ʧѹ����
                  if ((pStatisRecord->loseVoltage&bitShift) == 0x00)
                  {
                     if (debugInfo&PRINT_BALANCE_DEBUG)
                     {
                      printf("��ѹͳ��:%d��ʧѹ����\n",phase);
                     }
                     
                     pStatisRecord->loseVoltage |= bitShift;
                     vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, FALSE, statisTime);
                  }
               }
      	       else
      	       {
      	         //ʧѹ�ָ�
      	         if ((pStatisRecord->loseVoltage&bitShift) == bitShift)
                 {
                    if (debugInfo&PRINT_BALANCE_DEBUG)
                    {
                      printf("��ѹͳ��:%d��ָ�����\n",phase);
                    }
                    
                    pStatisRecord->loseVoltage &= (~bitShift);
                    vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, TRUE, statisTime);
                 }
      	       }
      	    }
      	  }
   	    	
   	    	voltage = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8;

          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("��ѹͳ��:%d���ѹֵ=%x\n",phase,voltage);
          }
   	    	
 	    	  if (pLimit != NULL)
 	    	  {
     	      //���ڶ�������,����Խ���ж�
     	      if (voltage >= pLimit->vPhaseDownLimit)
     	      {
     	      	//��¼����ָ��¼�
     	      	if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
              {
                 if ((pStatisRecord->phaseBreak&bitShift) == bitShift)
                 {
                   if (debugInfo&PRINT_BALANCE_DEBUG)
                   {
                     printf("��ѹͳ��:%d�����ָ�\n",phase);
                   }
                   
                   pStatisRecord->phaseBreak &= (~bitShift);
                   vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, TRUE, statisTime);
                 }
     	      	}
     	      	  
     	      	//Խ������
              if (voltage >= pLimit->vSuperiodLimit)
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {
                   //��һ�η���Խ������,��¼Խ���¼�
                   if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                   {
               	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
               	   	   
               	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
               	   	   {
               	   	     printf("��ѹͳ��:Խ�����޿�ʼ����,%d���Ӻ��¼\n",pLimit->vUpUpTimes);
               	   	   }
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("��ѹͳ��:��ѹԽ�����޷�������ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
               	   
               	         pStatisRecord->vOverLimit |= bitShift;
               	         vOverLimitEvent(pCopyParaBuff, phase, pn, 2, FALSE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
               	   }
                }
                   
                if (pBalanceParaBuff[offsetSta]!=0xee && pBalanceParaBuff[offsetSta+1]!=0xee)
                {
                  tmpDataShort = pBalanceParaBuff[offsetSta] | pBalanceParaBuff[offsetSta+1]<<8;
                }
                else
                {
                	tmpDataShort = 0;
                }
                tmpDataShort += statisInterval;
                pBalanceParaBuff[offsetSta] = tmpDataShort&0xff;
                pBalanceParaBuff[offsetSta+1] = (tmpDataShort>>8)&0xff;
                
                if (debugInfo&PRINT_BALANCE_DEBUG)
                {
                  printf("��ѹͳ��:%d��Խ������ʱ��=%d\n", phase, tmpDataShort);
                }
                
                if (pBalanceParaBuff[offsetSta+4]!=0xee && pBalanceParaBuff[offsetSta+5]!=0xee)
                {
                   tmpDataShort = pBalanceParaBuff[offsetSta+4] | pBalanceParaBuff[offsetSta+5]<<8;
                }
                else
                {
                 	 tmpDataShort = 0;
                }
                tmpDataShort += statisInterval;
                pBalanceParaBuff[offsetSta+4] = tmpDataShort&0xff;
                pBalanceParaBuff[offsetSta+5] = (tmpDataShort>>8)&0xff;
                 
                if (debugInfo&PRINT_BALANCE_DEBUG)
                {
                   printf("��ѹͳ��:%d��Խ����(��Խ�����޵�ͬʱ)ʱ��=%d\n", phase, tmpDataShort);
                }
              }
              else   
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {
                  if (bcdToHex(voltage) <=vUpUpResume)
                  {
                 	 //��������Խ������,��¼Խ�����޻ָ�
                   if (pStatisRecord->vOverLimit&bitShift)
                   {
               	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
               	   	   printf("��ѹͳ��:Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->vUpUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("��ѹͳ��:Խ�����޻ָ�����ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
               	   
                   	     pStatisRecord->vOverLimit &= (~bitShift);                    	
                   	     vOverLimitEvent(pCopyParaBuff, phase, pn, 2, TRUE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
               	   }
               	  }
                }
              }
                 	
              //Խ����
              if (voltage >= pLimit->vUpLimit && voltage < pLimit->vSuperiodLimit)
              {
                 if (pBalanceParaBuff[offsetSta+4]!=0xee && pBalanceParaBuff[offsetSta+5]!=0xee)
                 {
                   tmpDataShort = pBalanceParaBuff[offsetSta+4] | pBalanceParaBuff[offsetSta+5]<<8;
                 }
                 else
                 {
                 	 tmpDataShort = 0;
                 }
                 tmpDataShort += statisInterval;
                 pBalanceParaBuff[offsetSta+4] = tmpDataShort&0xff;
                 pBalanceParaBuff[offsetSta+5] = (tmpDataShort>>8)&0xff;
                 
                 if (debugInfo&PRINT_BALANCE_DEBUG)
                 {
                   printf("��ѹͳ��:%d��Խ����ʱ��=%d\n", phase, tmpDataShort);
                 }
              }
                   
              //Խ������
              bitShift <<= 1;
              if (voltage <= pLimit->vDownDownLimit && voltage != 0)   //Խ������
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {	  
                  //��һ�η���Խ��,��¼Խ���¼�
                  if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                  {                   	
               	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
               	   	   
               	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
               	   	   {
               	   	     printf("��ѹͳ��:��ѹԽ�����޷���,%d���Ӻ��¼\n",pLimit->vDownDownTimes);
               	   	   }
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("��ѹͳ��:��ѹԽ�����޷�������ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->vDownDownTime[phase-1].year = 0xff;
               	   
               	         pStatisRecord->vOverLimit |= bitShift;
                 	       vOverLimitEvent(pCopyParaBuff, phase, pn, 1, FALSE, statisTime);
               	       }
               	     }
               	  }               	   
               	  else
               	  {
  	                pStatisRecord->vDownDownTime[phase-1].year = 0xff;
               	  }
                }
                 
                if (pBalanceParaBuff[offsetSta+2]!=0xee && pBalanceParaBuff[offsetSta+3]!=0xee)
                {
                  tmpDataShort = pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8;
                }
                else
                {
                	tmpDataShort = 0;
                }
                tmpDataShort += statisInterval;
                pBalanceParaBuff[offsetSta+2] = tmpDataShort&0xff;
                pBalanceParaBuff[offsetSta+3] = (tmpDataShort>>8)&0xff;
                
                if (debugInfo&PRINT_BALANCE_DEBUG)
                {
                  printf("��ѹͳ��:%d��Խ������ʱ��=%d\n", phase, tmpDataShort);
                }
                
                if (pBalanceParaBuff[offsetSta+6]!=0xee && pBalanceParaBuff[offsetSta+7]!=0xee)
                {
                   tmpDataShort = pBalanceParaBuff[offsetSta+6] | pBalanceParaBuff[offsetSta+7]<<8;
                }
                else
                {
                 	 tmpDataShort = 0;
                }
                tmpDataShort += statisInterval;
                pBalanceParaBuff[offsetSta+6] = tmpDataShort&0xff;
                pBalanceParaBuff[offsetSta+7] = (tmpDataShort>>8)&0xff;
                 
                if (debugInfo&PRINT_BALANCE_DEBUG)
                {
                  printf("��ѹͳ��:%d��Խ����(��Խ�����޵�ͬʱͳ��)ʱ��=%d\n", phase, tmpDataShort);
                }
              }
              else
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {
                 	 //��������Խ������,��¼Խ�޻ָ��¼�
                  if (bcdToHex(voltage) >=vDownDownResume)
                  {
                   if (pStatisRecord->vOverLimit&bitShift)
                   {
               	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
               	   	   
               	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
               	   	   {
               	   	     printf("��ѹͳ��:��ѹԽ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->vDownDownTimes);
               	   	   }
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("��ѹͳ��:��ѹ�����޻ָ�����ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->vDownDownTime[phase-1].year = 0xff;
               	   
               	         pStatisRecord->vOverLimit &= (~bitShift);
               	         vOverLimitEvent(pCopyParaBuff, phase, pn, 1, TRUE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->vDownDownTime[phase-1].year = 0xff;
               	   }
               	  }
                }
              }
              bitShift >>= 1;
                 
              //Խ����
              if (voltage <= pLimit->vLowLimit && voltage > pLimit->vDownDownLimit)
              {
                 if (pBalanceParaBuff[offsetSta+6]!=0xee && pBalanceParaBuff[offsetSta+7]!=0xee)
                 {
                   tmpDataShort = pBalanceParaBuff[offsetSta+6] | pBalanceParaBuff[offsetSta+7]<<8;
                 }
                 else
                 {
                 	 tmpDataShort = 0;
                 }
                 tmpDataShort += statisInterval;
                 pBalanceParaBuff[offsetSta+6] = tmpDataShort&0xff;
                 pBalanceParaBuff[offsetSta+7] = (tmpDataShort>>8)&0xff;
                 
                 if (debugInfo&PRINT_BALANCE_DEBUG)
                 {
                   printf("��ѹͳ��:%d��Խ����ʱ��=%d\n", phase, tmpDataShort);
                 }
              }
                       
              //�ϸ�      
              if (voltage > pLimit->vLowLimit && voltage < pLimit->vUpLimit)
              {
                 if (pBalanceParaBuff[offsetSta+8]!=0xee && pBalanceParaBuff[offsetSta+9]!=0xee)
                 {
                   tmpDataShort = pBalanceParaBuff[offsetSta+8] | pBalanceParaBuff[offsetSta+9]<<8;
                 }
                 else
                 {
                 	 tmpDataShort = 0;
                 }
                 tmpDataShort += statisInterval;
                 pBalanceParaBuff[offsetSta+8] = tmpDataShort&0xff;
                 pBalanceParaBuff[offsetSta+9] = (tmpDataShort>>8)&0xff;

                 if (debugInfo&PRINT_BALANCE_DEBUG)
                 {
                   printf("��ѹͳ��:%d��ϸ�ʱ��=%d\n", phase, tmpDataShort);
                 }
              }
            }
            else  //�ж϶���
            {
              if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
              {
                //��������0�ж�Ϊ����
                if (pCopyParaBuff[offsetCurrent] == 0x00 && pCopyParaBuff[offsetCurrent+1] == 0x00 && pCopyParaBuff[offsetCurrent+2] == 0x00)
                {
                  //��¼�����¼�
                  if ((pStatisRecord->phaseBreak&bitShift) == 0x00)
                  {
                    if (debugInfo&PRINT_BALANCE_DEBUG)
                    {
                      printf("��ѹͳ��:%d����෢��\n",phase);
                    }

                    pStatisRecord->phaseBreak |= bitShift;
                    vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, FALSE, statisTime);
                  }
                }
              }
            }
   	      }
   	      else  //δ���õ�ѹ��ֵ,��Ϊ�ϸ�
   	      {
            if (pBalanceParaBuff[offsetSta+8]!=0xee && pBalanceParaBuff[offsetSta+9]!=0xee)
            {
   	          tmpDataShort = pBalanceParaBuff[offsetSta+8] | pBalanceParaBuff[offsetSta+9]<<8;
   	        }
   	        else
   	        {
   	        	tmpDataShort = 0;
   	        }
            tmpDataShort += statisInterval;
            pBalanceParaBuff[offsetSta+8] = tmpDataShort&0xff;
            pBalanceParaBuff[offsetSta+9] = (tmpDataShort>>8)&0xff;
   	      }
   	      
   	      //�յ�ѹ���ֵ������ʱ��
          if ((pCopyParaBuff[offset+1] > pBalanceParaBuff[offsetExtre+1])
           	|| (pCopyParaBuff[offset+1] == pBalanceParaBuff[offsetExtre+1]&&pCopyParaBuff[offset] > pBalanceParaBuff[offsetExtre])
           	  || (pBalanceParaBuff[offsetExtre+1] == 0xEE&&pBalanceParaBuff[offsetExtre]== 0xEE))
          {
             pBalanceParaBuff[offsetExtre] = pCopyParaBuff[offset];
             pBalanceParaBuff[offsetExtre+1] = pCopyParaBuff[offset+1];
             
             pBalanceParaBuff[offsetExtre+2] = statisTime.minute;
             pBalanceParaBuff[offsetExtre+3] = statisTime.hour;
             pBalanceParaBuff[offsetExtre+4] = statisTime.day;
          }
           
          //�յ�ѹ��Сֵ������ʱ��
          if ((pCopyParaBuff[offset+1] < pBalanceParaBuff[offsetExtre+6])
           	|| (pCopyParaBuff[offset+1] == pBalanceParaBuff[offsetExtre+6] && pCopyParaBuff[offset] < pBalanceParaBuff[offsetExtre+5])
           	  || (pBalanceParaBuff[offsetExtre+5] == 0xEE && pBalanceParaBuff[offsetExtre+6] == 0xEE))
          {
             pBalanceParaBuff[offsetExtre+5] = pCopyParaBuff[offset];
             pBalanceParaBuff[offsetExtre+6] = pCopyParaBuff[offset+1];
             
             pBalanceParaBuff[offsetExtre+7] = statisTime.minute;
             pBalanceParaBuff[offsetExtre+8] = statisTime.hour;
             pBalanceParaBuff[offsetExtre+9] = statisTime.day;
          }
           
          //�յ�ѹƽ��ֵ
          if (pBalanceParaBuff[offsetExtre+10] != 0xEE && pBalanceParaBuff[offsetExtre+11] != 0xEE 
           	&& pBalanceParaBuff[offsetExtre+12] != 0xEE && pBalanceParaBuff[offsetExtre+13] != 0xEE)
          {
             voltage = pBalanceParaBuff[offsetExtre+10] | pBalanceParaBuff[offsetExtre+11]<<8;
             voltage *= pBalanceParaBuff[offsetExtre+12] | pBalanceParaBuff[offsetExtre+13]<<8;
             voltage += (INT16U)bcdToHex(pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8);
             voltage /= (pBalanceParaBuff[offsetExtre+12] | pBalanceParaBuff[offsetExtre+13]<<8)+1;
          }
          else
          {
             voltage = (INT16U)bcdToHex(pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8);
             pBalanceParaBuff[offsetExtre+12] = 0;
             pBalanceParaBuff[offsetExtre+13] = 0;
          }
           
          pBalanceParaBuff[offsetExtre+10] = voltage&0xFF;
          pBalanceParaBuff[offsetExtre+11] = (voltage>>8)&0xFF;
          tmpDataShort = pBalanceParaBuff[offsetExtre+12] | pBalanceParaBuff[offsetExtre+13]<<8;
          tmpDataShort++;
           
          pBalanceParaBuff[offsetExtre+12] = tmpDataShort&0xFF;
          pBalanceParaBuff[offsetExtre+13] = tmpDataShort>>8&0xFF;
   	    }
	    }
	    else //��ʼ�µ�ͳ��
	    {
        if (pCopyParaBuff[offset] != 0xEE)
   	    {
   	    	voltage = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8;
 	    	  
 	    	  if (pLimit!=NULL)
 	    	  {
     	      //��ѹ�쳣�¼���ʧѹ�ж�
     	      if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
            {
     	        if (voltage < volLoss && (pCopyParaBuff[offsetCurrent] != 0x00 || pCopyParaBuff[offsetCurrent+1] != 0x00 || pCopyParaBuff[offsetCurrent+2] != 0x00))
     	        {
                 //ʧѹ����
                 if ((pStatisRecord->loseVoltage&bitShift) == 0x00)
                 {
                    pStatisRecord->loseVoltage |= bitShift;  
                    vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, FALSE, statisTime);
                 }
               }
     	        else
     	        {
     	          //ʧѹ�ָ�
     	          if (pStatisRecord->loseVoltage&bitShift)
                 {
                   pStatisRecord->loseVoltage &= (~bitShift);
                   vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, TRUE, statisTime);
                 }
     	        }
     	      }
     	      
     	      //���ڶ�������,����Խ���ж�
     	      if (voltage >= pLimit->vPhaseDownLimit)
     	      {
     	      	  //��¼����ָ��¼�
     	      	  if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
                 {
                   if (pStatisRecord->phaseBreak&bitShift)
                   {
                     pStatisRecord->phaseBreak &= (~bitShift);
                     vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, TRUE, statisTime);
                   }
     	      	  }
     	      	  
     	      	  //Խ������
                 if (voltage >= pLimit->vSuperiodLimit)
                 {
                   if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                   {
                     //��һ�η���Խ������,��¼Խ���¼�
                     if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                     {
                 	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
                 	   	   printf("��ѹͳ��(��):Խ�����޿�ʼ����,%d���Ӻ��¼\n",pLimit->vUpUpTimes);
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("��ѹͳ��(��):��ѹԽ�����޷�������ʱ���ѵ�\n");
    	                  	 }
    	                  	 
    	                  	 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
                 	   
                 	         pStatisRecord->vOverLimit |= bitShift;
                 	         vOverLimitEvent(pCopyParaBuff, phase, pn, 2, FALSE, statisTime);
                 	       }
                 	     }
                 	   }               	   
                 	   else
                 	   {
    	                 pStatisRecord->vUpUpTime[phase-1].year = 0xff;               	   	 
                 	   }
                   }
                   
                   pBalanceParaBuff[offsetSta] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+1] = (statisInterval>>8)&0xff;
                   pBalanceParaBuff[offsetSta+2] = 0x00;
                   pBalanceParaBuff[offsetSta+3] = 0x00;
                   pBalanceParaBuff[offsetSta+4] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+5] = (statisInterval>>8)&0xff;
                   pBalanceParaBuff[offsetSta+6] = 0x00;
                   pBalanceParaBuff[offsetSta+7] = 0x00;
                   pBalanceParaBuff[offsetSta+8] = 0x00;
                   pBalanceParaBuff[offsetSta+9] = 0x00;
                 }
                 else   
                 {
                 	if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                   {
                      if (bcdToHex(voltage) <=vUpUpResume)
                      {
                     	 //��������Խ������,��¼Խ�����޻ָ�
                       if (pStatisRecord->vOverLimit&bitShift)
                       {
                   	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
                   	     {
                   	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
                   	   	   printf("��ѹͳ��(��):Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->vUpUpTimes);
                   	     }
                   	     else
                   	     {
      	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
      	                   {
      	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
      	                  	 {
      	                  	   printf("��ѹͳ��(��):��ѹ�����޻ָ�����ʱ���ѵ�\n");
      	                  	 }
      	                  	 
      	                  	 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
                   	   
                       	     pStatisRecord->vOverLimit &= (~bitShift);                    	
                       	     vOverLimitEvent(pCopyParaBuff, phase, pn, 2, TRUE, statisTime);
                   	       }
                   	     }
                   	   }
                   	   else
                   	   {
      	                 pStatisRecord->vUpUpTime[phase-1].year = 0xff;
                   	   }
                   	  }
                   }
                 }
                 	
               	//Խ����
                 if (voltage >= pLimit->vUpLimit && voltage < pLimit->vSuperiodLimit)
                 {
                   pBalanceParaBuff[offsetSta] = 0x00;
                   pBalanceParaBuff[offsetSta+1] = 0x00;
                   pBalanceParaBuff[offsetSta+2] = 0x00;
                   pBalanceParaBuff[offsetSta+3] = 0x00;
                   pBalanceParaBuff[offsetSta+4] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+5] = (statisInterval>>8)&0xff;
                   pBalanceParaBuff[offsetSta+6] = 0x00;
                   pBalanceParaBuff[offsetSta+7] = 0x00;
                   pBalanceParaBuff[offsetSta+8] = 0x00;
                   pBalanceParaBuff[offsetSta+9] = 0x00;
                 }
                   
                 //Խ������
                 bitShift <<= 1;
                 if (voltage <= pLimit->vDownDownLimit && voltage != 0)   //Խ������
                 {
                   if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                   {	  
                      //��һ�η���Խ��,��¼Խ���¼�
                      if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                      {
                   	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
                   	     {
                   	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
                   	   	   
                   	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                   	   	   {
                   	   	     printf("��ѹͳ��(��):��ѹԽ�����޷���,%d���Ӻ��¼\n",pLimit->vDownDownTimes);
                   	   	   }
                   	     }
                   	     else
                   	     {
      	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
      	                   {
      	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
      	                  	 {
      	                  	   printf("��ѹͳ��(��):��ѹԽ�����޷�������ʱ���ѵ�\n");
      	                  	 }
      	                  	 
      	                  	 pStatisRecord->vDownDownTime[phase-1].year = 0xff;
                   	   
                   	         pStatisRecord->vOverLimit |= bitShift;
                     	       vOverLimitEvent(pCopyParaBuff, phase, pn, 1, FALSE, statisTime);
                   	       }
                   	     }
                   	  }
                   	  else
                   	  {
      	                pStatisRecord->vDownDownTime[phase-1].year = 0xff;
                   	  }
                   }
                   
                   pBalanceParaBuff[offsetSta] = 0x00;
                   pBalanceParaBuff[offsetSta+1] = 0x00;
                   pBalanceParaBuff[offsetSta+2] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+3] = (statisInterval>>8)&0xff;
                   pBalanceParaBuff[offsetSta+4] = 0x00;
                   pBalanceParaBuff[offsetSta+5] = 0x00;
                   pBalanceParaBuff[offsetSta+6] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+7] = (statisInterval>>8)&0xff;
                   pBalanceParaBuff[offsetSta+8] = 0x00;
                   pBalanceParaBuff[offsetSta+9] = 0x00;
                 }
                 else
                 {
                   if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                   {
                 	  //��������Խ������,��¼Խ�޻ָ��¼�
                     if (bcdToHex(voltage) >=vDownDownResume)
                     {
                      if (pStatisRecord->vOverLimit&bitShift)
                      {
                  	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
                  	     {
                  	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
                  	   	   
                  	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                  	   	   {
                  	   	     printf("��ѹͳ��(��):��ѹԽ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->vDownDownTimes);
                  	   	   }
                  	     }
                  	     else
                  	     {
     	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
     	                   {
     	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
     	                  	 {
     	                  	   printf("��ѹͳ��(��):��ѹ�����޻ָ�����ʱ���ѵ�\n");
     	                  	 }
     	                  	 
     	                  	 pStatisRecord->vDownDownTime[phase-1].year = 0xff;
                  	   
                  	         pStatisRecord->vOverLimit &= (~bitShift);
                  	         vOverLimitEvent(pCopyParaBuff, phase, pn, 1, TRUE, statisTime);
                  	       }
                  	     }
                  	   }
                  	   else
                  	   {
     	                 pStatisRecord->vDownDownTime[phase-1].year = 0xff;
                  	   }
                  	  }
                   }
                 }
                 bitShift >>= 1;
                       
               	//Խ����
                 if (voltage <= pLimit->vLowLimit && voltage > pLimit->vDownDownLimit)
                 {
                 	pBalanceParaBuff[offsetSta] = 0x00;
                   pBalanceParaBuff[offsetSta+1] = 0x00;
                   pBalanceParaBuff[offsetSta+2] = 0x00;
                   pBalanceParaBuff[offsetSta+3] = 0x00;
                   pBalanceParaBuff[offsetSta+4] = 0x00;
                   pBalanceParaBuff[offsetSta+5] = 0x00;
                   pBalanceParaBuff[offsetSta+6] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+7] = (statisInterval>>8)&0xff;
                   pBalanceParaBuff[offsetSta+8] = 0x00;
                   pBalanceParaBuff[offsetSta+9] = 0x00;
                 }
                       
                 //�ϸ�      
                 if (voltage > pLimit->vLowLimit && voltage < pLimit->vUpLimit)
                 {
                 	 pBalanceParaBuff[offsetSta] = 0x00;
                   pBalanceParaBuff[offsetSta+1] = 0x00;
                   pBalanceParaBuff[offsetSta+2] = 0x00;
                   pBalanceParaBuff[offsetSta+3] = 0x00;
                   pBalanceParaBuff[offsetSta+4] = 0x00;
                   pBalanceParaBuff[offsetSta+5] = 0x00;
                   pBalanceParaBuff[offsetSta+6] = 0x00;
                   pBalanceParaBuff[offsetSta+7] = 0x00;
                   pBalanceParaBuff[offsetSta+8] = statisInterval&0xff;
                   pBalanceParaBuff[offsetSta+9] = (statisInterval>>8)&0xff;
                 }   
             }
             else  //�ж϶���
             {
               if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
               {
                 //��������0�ж�Ϊ����?
                 if (pCopyParaBuff[offsetCurrent] == 0x00 && pCopyParaBuff[offsetCurrent+1] == 0x00 && pCopyParaBuff[offsetCurrent+2] == 0x00)
                 {
                   //��¼�����¼�
                   if ((pStatisRecord->phaseBreak&bitShift) == 0x00)
                   {
                     pStatisRecord->phaseBreak |= bitShift;
                     vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, FALSE, statisTime);
                   }
                 }
               }
             }
   	      }
   	      else  //δ���õ�ѹ��ֵ,��Ϊ�ϸ�
   	      {
             pBalanceParaBuff[offsetSta] = 0x00;
             pBalanceParaBuff[offsetSta+1] = 0x00;
             pBalanceParaBuff[offsetSta+2] = 0x00;
             pBalanceParaBuff[offsetSta+3] = 0x00;
             pBalanceParaBuff[offsetSta+4] = 0x00;
             pBalanceParaBuff[offsetSta+5] = 0x00;
             pBalanceParaBuff[offsetSta+6] = 0x00;
             pBalanceParaBuff[offsetSta+7] = 0x00;
             

             pBalanceParaBuff[offsetSta+8] = statisInterval&0xff;
             pBalanceParaBuff[offsetSta+9] = (statisInterval>>8)&0xff;  
   	      }
   	      
   	      //�յ�ѹ���ֵ������ʱ��
          pBalanceParaBuff[offsetExtre] = pCopyParaBuff[offset];
          pBalanceParaBuff[offsetExtre+1] = pCopyParaBuff[offset+1];
          pBalanceParaBuff[offsetExtre+2] = statisTime.minute;
          pBalanceParaBuff[offsetExtre+3] = statisTime.hour;
          pBalanceParaBuff[offsetExtre+4] = statisTime.day;
          
          //�յ�ѹ��Сֵ������ʱ��
          pBalanceParaBuff[offsetExtre+5] = pCopyParaBuff[offset];
          pBalanceParaBuff[offsetExtre+6] = pCopyParaBuff[offset+1];
          pBalanceParaBuff[offsetExtre+7] = statisTime.minute;
          pBalanceParaBuff[offsetExtre+8] = statisTime.hour;
          pBalanceParaBuff[offsetExtre+9] = statisTime.day;
           
          //�յ�ѹƽ��ֵ
          voltage = (INT16U)bcdToHex(pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8);
          pBalanceParaBuff[offsetExtre+10] = voltage&0xFF;
          pBalanceParaBuff[offsetExtre+11] = (voltage>>8)&0xFF;
          pBalanceParaBuff[offsetExtre+12] = 0x01;
          pBalanceParaBuff[offsetExtre+13] = 0x00;
   	    }
   	    else
   	    {
   	    	//��ѹԽ��ʱ��
   	      pBalanceParaBuff[offsetSta] = 0x00;
          pBalanceParaBuff[offsetSta+1] = 0x00;
          pBalanceParaBuff[offsetSta+2] = 0x00;
          pBalanceParaBuff[offsetSta+3] = 0x00;
          pBalanceParaBuff[offsetSta+4] = 0x00;
          pBalanceParaBuff[offsetSta+5] = 0x00;
          pBalanceParaBuff[offsetSta+6] = 0x00;
          pBalanceParaBuff[offsetSta+7] = 0x00;
          pBalanceParaBuff[offsetSta+8] = 0x00;
          pBalanceParaBuff[offsetSta+9] = 0x00;
           
          //��ѹƽ��ֵ
          pBalanceParaBuff[offsetExtre] = 0x00;
          pBalanceParaBuff[offsetExtre+1] = 0x00;
          pBalanceParaBuff[offsetExtre+2] = 0x00;
          pBalanceParaBuff[offsetExtre+3] = 0x00;
          pBalanceParaBuff[offsetExtre+4] = 0x00;
          pBalanceParaBuff[offsetExtre+5] = 0x00;
          pBalanceParaBuff[offsetExtre+6] = 0x00;
          pBalanceParaBuff[offsetExtre+7] = 0x00;
          pBalanceParaBuff[offsetExtre+8] = 0x00;
          pBalanceParaBuff[offsetExtre+9] = 0x00;
           
          //�յ�ѹƽ��ֵ
          pBalanceParaBuff[offsetExtre+10] = 0x00;
          pBalanceParaBuff[offsetExtre+11] = 0x00;
          pBalanceParaBuff[offsetExtre+12] = 0x00;
          pBalanceParaBuff[offsetExtre+13] = 0x00;
   	    }
	    }
	    
	    offset += 2;
	    offsetCurrent += 3;
	    offsetSta += 10;
	    offsetExtre += 14;
	    bitShift <<= 2;
   }
}

/*******************************************************
��������: statisticCurrent
��������: ����ͳ�� 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ:
*******************************************************/

void statisticCurrent(INT16U pn, INT8U *pCopyParaBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit, INT8U statisInterval, DATE_TIME statisTime)
{
    INT32U  current;
    INT32U  tmpDataShort;
    INT8U   phase;
    INT16U  offset, offsetSta;
    INT8U   bitShift;
    INT32U  cUpUpLimit, cUpLimit;    //������ֵ
    INT32U  cUpUpResume, cUpResume;  //�����ָ���ֵ
    
    offset    = CURRENT_PHASE_A;
    offsetSta = CUR_A_UP_UP_TIME;
    bitShift  = 0x01;    
    
    if(pLimit!=NULL)
    {
    	cUpLimit    = pLimit->cUpLimit[0] | pLimit->cUpLimit[1]<<8 | pLimit->cUpLimit[2]<<16;
    	cUpUpLimit  = pLimit->cSuperiodLimit[0] | pLimit->cSuperiodLimit[1]<<8 | pLimit->cSuperiodLimit[2]<<16;
      cUpResume   = calcResumeLimit(cUpLimit, pLimit->cUpResume[0] | pLimit->cUpResume[1]<<8);
      cUpUpResume = calcResumeLimit(cUpUpLimit, pLimit->cUpUpReume[0] | pLimit->cUpUpReume[1]<<8);      
    	
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("����ͳ��:����ֵ=%x\n",cUpLimit);         //BCD
    	 
    	  printf("����ͳ��:���޻ָ�ֵ=%d\n",cUpResume);    //16����
    	 
    	  printf("����ͳ��:������ֵ=%x\n",cUpUpLimit);     //BCD
    	 
    	  printf("����ͳ��:�����޻ָ�ֵ=%d\n",cUpUpResume);//16����
    	}
    }
    
    //A,B,C���������������ж�
    for (phase = 1; phase <= 4; phase++)
    {
 	    if (pBalanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)  //����ǰһ��ͳ��
 	    {
 	      if (pCopyParaBuff[offset] != 0xEE)
     	  {
   	      current = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8 | (pCopyParaBuff[offset+2]&0x7f)<<16;

          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("����ͳ��:%d�����ֵ=%x\n",phase,current);
          }
           
   	      if (pLimit!=NULL)
   	      {
            //����Խ����
       	 	  if (current >= cUpLimit && current < cUpUpLimit)
            {
               if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
                  //��һ�η���Խ���ޣ���¼Խ�����¼�
                 	if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
                 	{
                 	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("����ͳ��:Խ���޿�ʼ����,%d���Ӻ��¼\n",pLimit->cUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("����ͳ��:Խ���޷�������ʱ���ѵ�\n");
    	                  	 }
    	                  	 
    	                  	 pStatisRecord->cUpTime[phase-1].year = 0xff;
                 	   
                 	         pStatisRecord->cOverLimit |= bitShift;
               	           cOverLimitEvent(pCopyParaBuff, phase, pn, 1, FALSE, statisTime);
                 	       }
                 	     }
                 	}               	   
                 	else
                 	{
    	              pStatisRecord->cUpTime[phase-1].year = 0xff;
                 	}
         	 	   }
         	 	      
         	 	   //ͳ��Խ����ʱ��         	 	   
               if (pBalanceParaBuff[offsetSta+2]!=0xee && pBalanceParaBuff[offsetSta+3]!=0xee)
               {
         	 	     tmpDataShort = pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8;
               }
               else
               {
                 tmpDataShort = 0;
               }

            	 tmpDataShort += statisInterval;
            	 pBalanceParaBuff[offsetSta+2] = tmpDataShort&0xFF;
            	 pBalanceParaBuff[offsetSta+3] = (tmpDataShort>>8)&0xFF;
   	           
   	           if (debugInfo&PRINT_BALANCE_DEBUG)
   	           {
   	      	      printf("����ͳ��:%d��Խ����ʱ��=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	           }
            }
          	else
          	{
          	   //������Խ�����¼���������¼Խ�����¼��ָ�
          	   if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {	
                  if (bcdToHex(current)<=cUpResume)
                  {
          	    	 if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
               	   	   printf("����ͳ��:Խ���޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->cUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("����ͳ��:Խ���޻ָ�����ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpTime[phase-1].year = 0xff;
               	   
             	    	     //��¼Խ���޻ָ�
             	           pStatisRecord->cOverLimit &= (~bitShift);
       	 	               cOverLimitEvent(pCopyParaBuff, phase, pn, 1, TRUE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->cUpTime[phase-1].year = 0xff;
               	   }
               	  }
       	 	     }
          	}
          	    
          	bitShift <<= 1;
            //����Խ������
            if (current >= cUpUpLimit)  
            {
               if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
               	  //��һ�η���Խ�ޣ���¼Խ���¼�
               	  if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
               	  {
                 	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("����ͳ��:Խ�����޿�ʼ����,%d���Ӻ��¼\n",pLimit->cUpUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("����ͳ��:Խ�����޷�������ʱ���ѵ�\n");
    	                  	 }
    	                  	 
    	                  	 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
                 	                  	           
               	           pStatisRecord->cOverLimit |= bitShift;
               	           cOverLimitEvent(pCopyParaBuff, phase, pn, 2, FALSE, statisTime);
                 	       }
                 	     }
                 	}               	   
                 	else
                 	{
    	              pStatisRecord->cUpUpTime[phase-1].year = 0xff;
                 	}
               	}
               	
               	//Խ������ʱ��
               if (pBalanceParaBuff[offsetSta]!=0xee && pBalanceParaBuff[offsetSta+1]!=0xee)
               {
          	     tmpDataShort = pBalanceParaBuff[offsetSta] | pBalanceParaBuff[offsetSta+1]<<8;
               }
               else
               {
                 tmpDataShort = 0;
               }

          	   tmpDataShort += statisInterval;
          	   pBalanceParaBuff[offsetSta] = tmpDataShort&0xFF;
          	   pBalanceParaBuff[offsetSta+1] = (tmpDataShort>>8)&0xFF;
   	           if (debugInfo&PRINT_BALANCE_DEBUG)
   	           {
   	      	      printf("����ͳ��:%d��Խ����ʱ��=%d\n", phase, pBalanceParaBuff[offsetSta] | pBalanceParaBuff[offsetSta+1]<<8);
   	           }
               
               //ly,2011-10-20,add,Խ�����޵Ļ�,�϶�Խ������,��˼���Խ����ʱ��
         	 	   //ͳ��Խ����ʱ��
               if (pBalanceParaBuff[offsetSta+2]!=0xee && pBalanceParaBuff[offsetSta+3]!=0xee)
               {
         	 	     tmpDataShort = pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8;
               }
               else
               {
                 tmpDataShort = 0;
               }

            	 tmpDataShort += statisInterval;
            	 pBalanceParaBuff[offsetSta+2] = tmpDataShort&0xFF;
            	 pBalanceParaBuff[offsetSta+3] = (tmpDataShort>>8)&0xFF;
   	           
   	           if (debugInfo&PRINT_BALANCE_DEBUG)
   	           {
   	      	      printf("����ͳ��:%d��Խ����(�������޵�ͬʱ)ʱ��=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	           }
            }
            else   
       	    {
       	 	     if ((eventRecordConfig.iEvent[3]&0x01) || (eventRecordConfig.nEvent[3]&0x01))
               {
                 	//������Խ�������¼�����,��¼Խ�����޻ָ�
                  if (bcdToHex(current)<=cUpUpResume)
                  {
       	 	         if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
               	   	   printf("����ͳ��:Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->cUpUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("����ͳ��:Խ�����޻ָ�����ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
               	   
             	    	     //��¼Խ�����޻ָ�
             	           pStatisRecord->cOverLimit &= (~bitShift);
               	         cOverLimitEvent(pCopyParaBuff, phase, pn, 2, TRUE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
               	   }
               	  }
       	 	     } 
         	 	}
         	 	
         	 	bitShift >>= 1;
          }
          else
          {
           	//Խ������ʱ��
            pBalanceParaBuff[offsetSta] = 0x00;
          	pBalanceParaBuff[offsetSta+1] = 0x00;
          	//Խ����ʱ��
          	pBalanceParaBuff[offsetSta+2] = 0x00;
          	pBalanceParaBuff[offsetSta+3] = 0x00;
          }
           
          //�������ֵ
          if (pBalanceParaBuff[offsetSta+4]!=0xee && pBalanceParaBuff[offsetSta+5]!=0xee && pBalanceParaBuff[offsetSta+6]!=0xee)
          {
            tmpDataShort = pBalanceParaBuff[offsetSta+4] | pBalanceParaBuff[offsetSta+5]<<8 | pBalanceParaBuff[offsetSta+6]<<16;
            if (tmpDataShort < current)  //�������ֵ
            {
               pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[offset];
               pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[offset+1];
               pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[offset+2];
               
               pBalanceParaBuff[offsetSta+7] = statisTime.minute;
               pBalanceParaBuff[offsetSta+8] = statisTime.hour;
               pBalanceParaBuff[offsetSta+9] = statisTime.day;
            }
          }
          else    //��ֵ
          {
            pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[offset];
            pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[offset+1];
            pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[offset+2];
               
            pBalanceParaBuff[offsetSta+7] = statisTime.minute;
            pBalanceParaBuff[offsetSta+8] = statisTime.hour;
            pBalanceParaBuff[offsetSta+9] = statisTime.day;
          }
        }
      }
      else  //���¿�ʼ�µ�ͳ��
      {
        if (pCopyParaBuff[offset] != 0xEE)
     	  {
   	      current = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8 | (pCopyParaBuff[offset+2]&0x7f)<<16;
   	      
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	      	 printf("����ͳ��(���¿�ʼͳ��):%d�����=%x\n", phase, current);
   	      }

   	      if (pLimit!=NULL)
   	      {
             //����Խ����  
       	 	   if (current >= cUpLimit && current < cUpUpLimit)
             {
               if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
                 	//��һ�η���Խ���ޣ���¼Խ�����¼�
                 	if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
                 	{
                 	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("����ͳ��:Խ���޿�ʼ����,%d���Ӻ��¼\n",pLimit->cUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("����ͳ��:Խ���޷�������ʱ���ѵ�\n");
    	                  	 }
    	                  	 
    	                  	 pStatisRecord->cUpTime[phase-1].year = 0xff;
                 	   
                 	         pStatisRecord->cOverLimit |= bitShift;
               	           cOverLimitEvent(pCopyParaBuff, phase, pn, 1, FALSE, statisTime);
                 	       }
                 	     }
                 	}               	   
                 	else
                 	{
    	              pStatisRecord->cUpTime[phase-1].year = 0xff;
                 	}
               }
         	 	      
         	 	   //Խ����ʱ��
         	 	   pBalanceParaBuff[offsetSta] = 0x00;
          	   pBalanceParaBuff[offsetSta+1] = 0x00;
         	 	   pBalanceParaBuff[offsetSta+2] = statisInterval&0xFF;
            	 pBalanceParaBuff[offsetSta+3] = (statisInterval>>8)&0xFF;
   	           
   	           if (debugInfo&PRINT_BALANCE_DEBUG)
   	           {
   	      	      printf("����ͳ��(���¿�ʼͳ��):%d��Խ����ʱ��=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	           }
             }
          	 else
          	 {
          	   if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
          	    	//������Խ�����¼���������¼Խ�����¼��ָ�
                  if (bcdToHex(current)<=cUpResume)
                  {
          	    	 if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
               	   	   printf("����ͳ��:Խ���޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->cUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("����ͳ��:Խ���޻ָ�����ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpTime[phase-1].year = 0xff;
               	   
             	    	     //��¼Խ���޻ָ�
             	           pStatisRecord->cOverLimit &= (~bitShift);
       	 	               cOverLimitEvent(pCopyParaBuff, phase, pn, 1, TRUE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->cUpTime[phase-1].year = 0xff;
               	   }
               	  }
       	 	     }
             }
          	    
             //����Խ������
             bitShift <<= 1;
             if (current >= cUpUpLimit)  
             {
               	if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
                {
                 	//��һ�η���Խ�ޣ���¼Խ���¼�
               	  if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
               	  {
                 	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("����ͳ��:Խ�����޿�ʼ����,%d���Ӻ��¼\n",pLimit->cUpUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("����ͳ��:Խ�����޷�������ʱ���ѵ�\n");
    	                  	 }
    	                  	 
    	                  	 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
                 	                  	           
               	           pStatisRecord->cOverLimit |= bitShift;
               	           cOverLimitEvent(pCopyParaBuff, phase, pn, 2, FALSE, statisTime);
                 	       }
                 	     }
                 	}               	   
                 	else
                 	{
    	              pStatisRecord->cUpUpTime[phase-1].year = 0xff;
                 	}
               	}
               	
               	//Խ������ʱ��
               	pBalanceParaBuff[offsetSta] = statisInterval&0xFF;
          	    pBalanceParaBuff[offsetSta+1] = (statisInterval>>8)&0xFF;
   	            if (debugInfo&PRINT_BALANCE_DEBUG)
   	            {
   	      	      printf("����ͳ��(���¿�ʼͳ��):%d��Խ������ʱ��=%d\n", phase, pBalanceParaBuff[offsetSta] | pBalanceParaBuff[offsetSta+1]<<8);
   	            }
         	 	    
         	 	    //ly,2011-10-20,add
         	 	    //Խ����ʱ��
         	 	    pBalanceParaBuff[offsetSta+2] = statisInterval&0xFF;
            	  pBalanceParaBuff[offsetSta+3] = (statisInterval>>8)&0xFF;
   	           
   	            if (debugInfo&PRINT_BALANCE_DEBUG)
   	            {
   	      	      printf("����ͳ��(���¿�ʼͳ��):%d��Խ����(��Խ�����޵�ͬʱ)ʱ��=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	            }
             }
             else   
       	     {
       	 	      if ((eventRecordConfig.iEvent[3]&0x01) || (eventRecordConfig.nEvent[3]&0x01))
                {
       	 	        //������Խ�������¼�����,��¼Խ�����޻ָ�
                  if (bcdToHex(current)<=cUpUpResume)
                  {
       	 	         if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
               	   	   printf("����ͳ��(���¿�ʼͳ��):Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->cUpUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("����ͳ��(���¿�ʼͳ��):Խ�����޻ָ�����ʱ���ѵ�\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
               	   
             	    	     //��¼Խ�����޻ָ�
             	           pStatisRecord->cOverLimit &= (~bitShift);
               	         cOverLimitEvent(pCopyParaBuff, phase, pn, 2, TRUE, statisTime);
               	       }
               	     }
               	   }
               	   else
               	   {
  	                 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
               	   }
               	  }
       	 	      } 
         	 	 }
         	 	 
         	 	 //��������������,��Խ���޺�Խ������ʱ��Ϊ0
         	 	 if (current<cUpLimit)
         	 	 {
           	   //Խ������ʱ��
               pBalanceParaBuff[offsetSta] = 0x00;
          	   pBalanceParaBuff[offsetSta+1] = 0x00;
          	  
          	   //Խ����ʱ��
          	   pBalanceParaBuff[offsetSta+2] = 0x00;
          	   pBalanceParaBuff[offsetSta+3] = 0x00;
         	 	 }

         	 	 bitShift >>= 1;
          }
          else
          {
             pBalanceParaBuff[offsetSta]   = 0x00;
          	 pBalanceParaBuff[offsetSta+1] = 0x00;
          	 pBalanceParaBuff[offsetSta+2] = 0x00;
             pBalanceParaBuff[offsetSta+3] = 0x00;
          }
           
          //�������ֵ
          pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[offset];
          pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[offset+1];
          pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[offset+2];
               
          pBalanceParaBuff[offsetSta+7] = statisTime.minute;
          pBalanceParaBuff[offsetSta+8] = statisTime.hour;
          pBalanceParaBuff[offsetSta+9] = statisTime.day;
        }
        else  //û�гɹ����ر��γ������ݣ�Խ��ʱ�估���ֵ��Ϊ0
        {
         	 pBalanceParaBuff[offsetSta]   = 0x00;
           pBalanceParaBuff[offsetSta+1] = 0x00;
         	
         	 pBalanceParaBuff[offsetSta+2] = 0x00;
           pBalanceParaBuff[offsetSta+3] = 0x00;
         	
           pBalanceParaBuff[offsetSta+4] = 0x00;
           pBalanceParaBuff[offsetSta+5] = 0x00;
           pBalanceParaBuff[offsetSta+6] = 0x00;
        }
      }
       
      offset += 3;
      offsetSta += 10;
      bitShift <<= 2;
    }
}

/*******************************************************
��������: ��ѹ������ƽ���Խ���¼�����
��������: 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ�� 
*******************************************************/
void statisticUnbalance(INT16U pn, INT8U *pCopyParaBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA * pLimit,INT8U statisInterval,DATE_TIME statisTime)
{
	  INT16U  tmpDataA;
	  INT16U  tmpDataB;
	  INT16U  tmpDataC;
	  INT16U  tmpDataLimit, tmpData;
	  INT32U  maxPhase, minPhase;
	  INT16U  unbalanceU, unbalanceC;
	  INT8U   eventFlag1 = 0x00, eventFlag2 = 0x00;
	  INT8U   eventData[25];
	  INT8U   j,tmpLineInType;
	  MEASURE_POINT_PARA pointPara;
	  INT8U   dataTail; 
	  INT16U  vUnBalanceResume;
	  INT16U  cUnBalanceResume;
	  
	  if (pBalanceParaBuff[VOL_UNBALANCE_TIME] == 0xEE || pBalanceParaBuff[NEXT_NEW_INSTANCE] == START_NEW_INSTANCE)
    {
      pBalanceParaBuff[VOL_UNBALANCE_TIME] = 0x00;
      pBalanceParaBuff[VOL_UNBALANCE_TIME+1] = 0x00;
    }
    
    if (pBalanceParaBuff[CUR_UNBALANCE_TIME] == 0xEE || pBalanceParaBuff[NEXT_NEW_INSTANCE] == START_NEW_INSTANCE)
    {
      pBalanceParaBuff[CUR_UNBALANCE_TIME] = 0x00;
      pBalanceParaBuff[CUR_UNBALANCE_TIME+1] = 0x00;
    }
	  
	  if (pLimit != NULL)
	  {
        //����������ѹһ��ͳ��
    	  if (pCopyParaBuff[VOLTAGE_PHASE_A] != 0xEE && pCopyParaBuff != 0xEE && pCopyParaBuff[VOLTAGE_PHASE_C] != 0xEE)
      	{
      	    //xxx.x
      	    tmpDataA = pCopyParaBuff[VOLTAGE_PHASE_A] | pCopyParaBuff[VOLTAGE_PHASE_A+1]<<8;
      	    tmpDataB = pCopyParaBuff[VOLTAGE_PHASE_B] | pCopyParaBuff[VOLTAGE_PHASE_B+1]<<8;
      	    tmpDataC = pCopyParaBuff[VOLTAGE_PHASE_C] | pCopyParaBuff[VOLTAGE_PHASE_C+1]<<8;
            
            //xxx.x
      	    tmpDataLimit = pLimit->uPhaseUnbalance[0]
      	                    | (pLimit->uPhaseUnbalance[1]&0x7f)<<8;
      	    
      	    //���ݸ�ʽת��bcd => hex
      	    tmpDataA = bcdToHex(tmpDataA);
      	    tmpDataB = bcdToHex(tmpDataB);
      	    tmpDataC = bcdToHex(tmpDataC);
      	    tmpDataLimit = bcdToHex(tmpDataLimit);
      	   
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
             printf("��ѹ��ƽ���:��ֵ=%d\n",tmpDataLimit);
           }

      	   vUnBalanceResume = calcResumeLimit(pLimit->uPhaseUnbalance[0] | pLimit->uPhaseUnbalance[1]<<8 , pLimit->uPhaseUnResume[0] | pLimit->uPhaseUnResume[1]<<8);
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
             printf("��ѹ��ƽ���:�ָ���ֵ=%d\n",vUnBalanceResume);
           }
      	    
      	    //Ѱ�������Сֵ
      	    maxPhase = maxData(maxData(tmpDataA, tmpDataB), tmpDataC);
      	    minPhase = minData(minData(tmpDataA, tmpDataB), tmpDataC);
            
            if (maxPhase == 0 || minPhase == 0)
            {
              return;
            }
      	    
      	    //���㲻ƽ���
      	    unbalanceU = (maxPhase - minPhase)*1000 / maxPhase;
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("��ѹ��ƽ���=%d\n",unbalanceU);
            }
      	    
      	    if (unbalanceU >= tmpDataLimit)
      	    {
      	    	//��ƽ���Խ���״η���
      	    	if ((eventRecordConfig.iEvent[2] & 0x01) || (eventRecordConfig.nEvent[2] & 0x01))
              {
      	    	  if (pStatisRecord->vUnBalance == VOLTAGE_UNBALANCE_NOT)
      	    	  {
      	    	    //�¼���ʼ
             	     if (pStatisRecord->vUnBalanceTime.year==0xff)
             	     {
             	   	   pStatisRecord->vUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->uPhaseUnTimes, 0);
             	   	   
             	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
             	   	   {
             	   	     printf("��ѹ��ƽ��:Խ�޷���,%d���Ӻ��¼\n",pLimit->uPhaseUnTimes);
             	   	   }
             	     }
             	     else
             	     {
	                   if (compareTwoTime(pStatisRecord->vUnBalanceTime,sysTime))
	                   {
	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
	                  	 {
	                  	   printf("��ѹ��ƽ��:Խ�޷�������ʱ���ѵ�\n");
	                  	 }
	                  	 
	                  	 pStatisRecord->vUnBalanceTime.year = 0xff;
             	   
      	    	         eventFlag1 = 0x03;
      	    	         pStatisRecord->vUnBalance = VOLTAGE_UNBALANCE;      	    	    
             	       }
             	     }
             	  }
             	  else
             	  {
	                pStatisRecord->vUnBalanceTime.year = 0xff;
             	  }
      	    	}
      	    	
      	    	//��¼��ѹ��ƽ���Խ��ʱ��
      	    	tmpData = pBalanceParaBuff[VOL_UNBALANCE_TIME] | pBalanceParaBuff[VOL_UNBALANCE_TIME+1]<<8;
      	    	tmpData += statisInterval;
      	    	pBalanceParaBuff[VOL_UNBALANCE_TIME] = tmpData&0xFF;
      	    	pBalanceParaBuff[VOL_UNBALANCE_TIME+1] = tmpData>>8&0xFF;
      	    	
      	    	if (pBalanceParaBuff[VOL_UNB_MAX]==0xee)
      	    	{
      	    		 //��ѹ��ƽ�����ֵ
      	    		 pBalanceParaBuff[VOL_UNB_MAX]   = hexToBcd(unbalanceU);
      	    		 pBalanceParaBuff[VOL_UNB_MAX+1] = hexToBcd(unbalanceU)>>8;

      	    		 //��ѹ��ƽ�����ֵ����ʱ��
      	    		 pBalanceParaBuff[VOL_UNB_MAX_TIME]   = statisTime.minute;
      	    		 pBalanceParaBuff[VOL_UNB_MAX_TIME+1] = statisTime.hour;
      	    		 pBalanceParaBuff[VOL_UNB_MAX_TIME+2] = statisTime.day;
      	    	}
      	    	else
      	    	{
      	    	   tmpData = pBalanceParaBuff[VOL_UNB_MAX] | pBalanceParaBuff[VOL_UNB_MAX+1]<<8;
      	    	   if (tmpData<unbalanceU)
      	    	   {
      	    		   //��ѹ��ƽ�����ֵ
      	    		   pBalanceParaBuff[VOL_UNB_MAX]   = hexToBcd(unbalanceU);
      	    		   pBalanceParaBuff[VOL_UNB_MAX+1] = hexToBcd(unbalanceU)>>8;

      	    		   //��ѹ��ƽ�����ֵ����ʱ��
      	    		   pBalanceParaBuff[VOL_UNB_MAX_TIME]   = statisTime.minute;
      	    		   pBalanceParaBuff[VOL_UNB_MAX_TIME+1] = statisTime.hour;
      	    		   pBalanceParaBuff[VOL_UNB_MAX_TIME+2] = statisTime.day;      	    	   	 
      	    	   }
      	    	}
      	    }
      	    else
      	    {
      	    	if ((eventRecordConfig.iEvent[2] & 0x01) || (eventRecordConfig.nEvent[2] & 0x01))
              {
      	    	  //��ƽ���Խ���״λָ�
                 if (unbalanceU<=vUnBalanceResume)
                 {
                	//��������Խ������,��¼Խ�����޻ָ�
      	          if (pStatisRecord->vUnBalance == VOLTAGE_UNBALANCE)
                  {
              	     if (pStatisRecord->vUnBalanceTime.year==0xff)
              	     {
              	   	   pStatisRecord->vUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->uPhaseUnTimes, 0);
            	   	     
            	   	     if (debugInfo&PRINT_BALANCE_DEBUG)
            	   	     {
              	   	     printf("��ѹ��ƽ��:Խ�޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->uPhaseUnTimes);
              	   	   }
              	     }
              	     else
              	     {
 	                   if (compareTwoTime(pStatisRecord->vUnBalanceTime,sysTime))
 	                   {
 	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
 	                  	 {
 	                  	   printf("��ѹ��ƽ��:Խ�޻ָ�����ʱ���ѵ�\n");
 	                  	 }
 	                  	 
 	                  	 pStatisRecord->vUnBalanceTime.year = 0xff;
              	   
      	      	       //�¼��ָ�
      	      	       eventFlag1 = 0x0C;
      	      	       pStatisRecord->vUnBalance = VOLTAGE_UNBALANCE_NOT;
              	       }
              	     }
              	  }
              	  else
              	  {
 	                  pStatisRecord->vUnBalanceTime.year = 0xff;
              	  }
              	 }
      	      }
      	    }
      	}
      	
        //������������һ��ͳ��
      	if (pCopyParaBuff[CURRENT_PHASE_A] != 0xEE && pCopyParaBuff[CURRENT_PHASE_B] != 0xEE && pCopyParaBuff[CURRENT_PHASE_C] != 0xEE)
    	  {
      	    tmpDataA = pCopyParaBuff[CURRENT_PHASE_A] | pCopyParaBuff[CURRENT_PHASE_A+1]<<8;
      	    tmpDataB = pCopyParaBuff[CURRENT_PHASE_B] | pCopyParaBuff[CURRENT_PHASE_B+1]<<8;
      	    tmpDataC = pCopyParaBuff[CURRENT_PHASE_C] | pCopyParaBuff[CURRENT_PHASE_C+1]<<8;

      	    tmpDataLimit = (pLimit->cPhaseUnbalance[0] | pLimit->cPhaseUnbalance[1]<<8);
      	    
      	    //���ݸ�ʽת��bcd => hex
      	    tmpDataA = bcdToHex(tmpDataA);
      	    tmpDataB = bcdToHex(tmpDataB);
      	    tmpDataC = bcdToHex(tmpDataC);
      	    tmpDataLimit = bcdToHex(tmpDataLimit);
      	    
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("������ƽ���:��ֵ=%d\n",tmpDataLimit);
            }
      	    
      	     cUnBalanceResume = calcResumeLimit(pLimit->cPhaseUnbalance[0] | pLimit->cPhaseUnbalance[1]<<8 , pLimit->cPhaseUnResume[0] | pLimit->cPhaseUnResume[1]<<8);
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("������ƽ���:�ָ���ֵ=%d\n",cUnBalanceResume);
             }
      	    
  	        tmpLineInType = 2;
  	        
			      if(selectViceParameter(0x04, 25, pn, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
			      {
               tmpLineInType = pointPara.linkStyle;
			      }
            
            //��Դ���߷�ʽ�����������߻���������,���ü���
            if (tmpLineInType>2 || tmpLineInType<1)
            {
            	  return;
            }
      	    
      	    //Ѱ�������Сֵ  
            if (tmpLineInType==2)   //��������
            {
      	      maxPhase = maxData(maxData(tmpDataA, tmpDataB), tmpDataC);
      	      minPhase = minData(minData(tmpDataA, tmpDataB), tmpDataC);
      	    }
      	    else                    //��������,B���޵���
      	    {
      	      maxPhase = maxData(tmpDataA, tmpDataC);
      	      minPhase = minData(tmpDataA, tmpDataC);
      	    }
      	    
            if (maxPhase == 0 || minPhase == 0)
            {
              return;
            }
            
      	    //������ƽ���
      	    unbalanceC = (maxPhase - minPhase)*1000 / maxPhase;

            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("������ƽ���=%d\n",unbalanceC);
            }
                        
      	    if (unbalanceC >= tmpDataLimit)
      	    {
      	    	if (pStatisRecord->cUnBalance == CURRENT_UNBALANCE_NOT)
      	    	{
    	    	    //�¼���ʼ
           	     if (pStatisRecord->cUnBalanceTime.year==0xff)
           	     {
           	   	   pStatisRecord->cUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->cPhaseUnTimes, 0);
           	   	   
           	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
           	   	   {
           	   	     printf("������ƽ��:Խ�޷���,%d���Ӻ��¼\n",pLimit->cPhaseUnTimes);
           	   	   }
           	     }
           	     else
           	     {
                   if (compareTwoTime(pStatisRecord->cUnBalanceTime,sysTime))
                   {
                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
                  	 {
                  	   printf("������ƽ��:Խ�޷�������ʱ���ѵ�\n");
                  	 }
                  	 
                  	 pStatisRecord->cUnBalanceTime.year = 0xff;
           	   
      	    	       eventFlag2 = 0x03;
      	    	       pStatisRecord->cUnBalance = CURRENT_UNBALANCE;
           	       }
           	     }
      	    	}
      	    	
      	    	//��¼������ƽ���Խ��ʱ��
      	    	tmpData = pBalanceParaBuff[CUR_UNBALANCE_TIME] | pBalanceParaBuff[CUR_UNBALANCE_TIME+1]<<8;
      	    	tmpData += statisInterval;
      	    	pBalanceParaBuff[CUR_UNBALANCE_TIME] = tmpData&0xFF;
      	    	pBalanceParaBuff[CUR_UNBALANCE_TIME+1] = tmpData>>8&0xFF;
      	    	
      	    	if (pBalanceParaBuff[CUR_UNB_MAX]==0xee)
      	    	{
      	    		 //������ƽ�����ֵ
      	    		 pBalanceParaBuff[CUR_UNB_MAX]   = hexToBcd(unbalanceC);
      	    		 pBalanceParaBuff[CUR_UNB_MAX+1] = hexToBcd(unbalanceC)>>8;

      	    		 //������ƽ�����ֵ����ʱ��
      	    		 pBalanceParaBuff[CUR_UNB_MAX_TIME]   = statisTime.minute;
      	    		 pBalanceParaBuff[CUR_UNB_MAX_TIME+1] = statisTime.hour;
      	    		 pBalanceParaBuff[CUR_UNB_MAX_TIME+2] = statisTime.day;
      	    	}
      	    	else
      	    	{
      	    	   tmpData = pBalanceParaBuff[CUR_UNB_MAX] | pBalanceParaBuff[CUR_UNB_MAX+1]<<8;
      	    	   if (tmpData<unbalanceU)
      	    	   {
      	    		   //������ƽ�����ֵ
      	    		   pBalanceParaBuff[CUR_UNB_MAX]   = hexToBcd(unbalanceC);
      	    		   pBalanceParaBuff[CUR_UNB_MAX+1] = hexToBcd(unbalanceC)>>8;

      	    		   //������ƽ�����ֵ����ʱ��
      	    		   pBalanceParaBuff[CUR_UNB_MAX_TIME]   = statisTime.minute;
      	    		   pBalanceParaBuff[CUR_UNB_MAX_TIME+1] = statisTime.hour;
      	    		   pBalanceParaBuff[CUR_UNB_MAX_TIME+2] = statisTime.day;
      	    	   }
      	    	}
      	    }
      	    else
      	    {
    	    	  //��ƽ���Խ���״λָ�
               if (unbalanceC<=cUnBalanceResume)
               {
              	//��������Խ������,��¼Խ�����޻ָ�
      	        if (pStatisRecord->cUnBalance == CURRENT_UNBALANCE)
                {
            	     if (pStatisRecord->cUnBalanceTime.year==0xff)
            	     {
            	   	   pStatisRecord->cUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->cPhaseUnTimes, 0);
            	   	   
            	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
            	   	   {
            	   	     printf("������ƽ��:Խ�޿�ʼ�ָ�,%d���Ӻ��¼\n",pLimit->cPhaseUnTimes);
            	   	   }
            	     }
            	     else
            	     {
                     if (compareTwoTime(pStatisRecord->cUnBalanceTime,sysTime))
                     {
                  	   if (debugInfo&PRINT_BALANCE_DEBUG)
                  	   {
                  	     printf("������ƽ��:Խ�޻ָ�����ʱ���ѵ�\n");
                  	   }
                  	 
                  	   pStatisRecord->cUnBalanceTime.year = 0xff;
            	   
    	      	         //�¼��ָ�
      	    	         eventFlag2 = 0x0c;
      	    	         pStatisRecord->cUnBalance = CURRENT_UNBALANCE_NOT;
            	       }
            	     }
            	  }
            	  else
            	  {
                  pStatisRecord->cUnBalanceTime.year = 0xff;
            	  }
            	 }
      	    }
      	}
      	
        unbalanceU = hexToBcd(unbalanceU);
        unbalanceC = hexToBcd(unbalanceC);
      	
      	//��ѹ��ƽ���Խ���¼�
        if (eventFlag1 != 0x00)
        {
            eventData[0] = 17;  //erc
                
            eventData[2] = statisTime.minute;
            eventData[3] = statisTime.hour;
            eventData[4] = statisTime.day;
            eventData[5] = statisTime.month;
            eventData[6] = statisTime.year;
            
            dataTail = 7;    
            
            eventData[dataTail++] = pn&0xff;   //�������8λ    
            eventData[dataTail] = (pn>>8)&0xf;    
            
            if (eventFlag1 == 0x03)
            {
              eventData[dataTail] |= 0x80;
            }
            if (eventFlag2 == 0x03)
            {
              eventData[dataTail] |= 0x80;
            }
            dataTail++;
            
            eventData[dataTail++]  = 0x01;

            eventData[dataTail++] = unbalanceU&0xFF;
            eventData[dataTail++] = unbalanceU>>8&0xFF;
            
            eventData[dataTail++] = unbalanceC&0xFF;
            eventData[dataTail++] = unbalanceC>>8&0xFF;
                
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A];
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A+1];
                
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B];
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B+1];
                
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C];
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C+1];
                
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+1];           
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+2];
                
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+1];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+2];
                
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+1];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];

            eventData[1] = dataTail;  //len
            
            if (eventRecordConfig.iEvent[2] & 0x01)
            {
               writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
            }
            
            if (eventRecordConfig.nEvent[2] & 0x01)
            {
               writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //������Ҫ�¼�����
            }
        }
        
        //������ƽ���Խ���¼�
        if (eventFlag2 != 0x00)
        {
            eventData[0] = 17;  //erc
                
            eventData[2] = statisTime.minute;
            eventData[3] = statisTime.hour;
            eventData[4] = statisTime.day;
            eventData[5] = statisTime.month;
            eventData[6] = statisTime.year;
            
            dataTail = 7;    
            
            eventData[dataTail++] = pn&0xff;   //�������8λ    
            eventData[dataTail] = (pn>>8)&0xf;    
            
            if (eventFlag2 == 0x03)
            {
              eventData[dataTail] |= 0x80;
            }
            dataTail++;
            
            eventData[dataTail++]  = 0x02;

            eventData[dataTail++] = unbalanceU&0xFF;
            eventData[dataTail++] = unbalanceU>>8&0xFF;
            
            eventData[dataTail++] = unbalanceC&0xFF;
            eventData[dataTail++] = unbalanceC>>8&0xFF;

            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A];
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A+1];
                
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B];
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B+1];
                
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C];
            eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C+1];
                
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+1];           
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+2];
                
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+1];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+2];
                
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+1];
            eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];
            eventData[1] = dataTail;  //len
            
            if (eventRecordConfig.iEvent[2] & 0x01)
            {
               writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
            }
            
            if (eventRecordConfig.nEvent[2] & 0x01)
            {
               writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //������Ҫ�¼�����
            }
        }
    }
}

/*******************************************************
��������: statisticOpenPhase
��������: ����ͳ�� 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ:
*******************************************************/
void statisticOpenPhase(INT8U *pCopyParaBuff,INT8U *pBalanceParaBuff)
{
		INT8U      i;
		INT16U     offset, offsetSta;
	  
		offsetSta = OPEN_PHASE_TIMES;
		offset = PHASE_DOWN_TIMES;

    /*
    //�����������һ��������Ϊͳ�Ʋο�
    if (readMeterData(readBuff, meterDeviceConfig.meterDevice[balancePn].measurePoint, POWER_PARA_DATA, LAST_TODAY, tmpTime) == TRUE)
    {
      //�������û������,����������һ��������Ϊͳ�Ʋο�
      if (readMeterData(reqReqTimeData, pn, POWER_PARA_DATA, FIRST_TODAY, lastCopyTime) == FALSE)
      {
      	 
      }    	 
    }
    */
             		  
		//�ܼ�A��B��C�����ͳ������
		for (i = 0; i < 4; i++)
		{
  	    //�������
  	    if (pCopyParaBuff[offset] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta] = pCopyParaBuff[PHASE_DOWN_TIMES+i*6];
  	      pBalanceParaBuff[offsetSta+1] = pCopyParaBuff[PHASE_DOWN_TIMES+i*6+1];
  	    }
  	    
  	    //������ʱ��
  	    if (pCopyParaBuff[offset+8] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta+2] = pCopyParaBuff[TOTAL_PHASE_DOWN_TIME+i*6];
  	      pBalanceParaBuff[offsetSta+3] = pCopyParaBuff[TOTAL_PHASE_DOWN_TIME+i*6+1];
  	      pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[TOTAL_PHASE_DOWN_TIME+i*6+2];
  	    }
  	    
  	    //���һ�ζ�����ʼʱ��
  	    if (pCopyParaBuff[offset+20] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+1];
  	      pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+2];
  	      pBalanceParaBuff[offsetSta+7] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+3];
  	      pBalanceParaBuff[offsetSta+8] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+4];
  	    }
  	    
  	    //���һ�ζ������ʱ��
  	    if (pCopyParaBuff[offset+36] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta+9]  = pCopyParaBuff[LAST_PHASE_DOWN_END+i*12+1];
  	      pBalanceParaBuff[offsetSta+10] = pCopyParaBuff[LAST_PHASE_DOWN_END+i*12+2];
  	      pBalanceParaBuff[offsetSta+11] = pCopyParaBuff[LAST_PHASE_DOWN_END+i*12+3];
  	      pBalanceParaBuff[offsetSta+12] = pCopyParaBuff[LAST_PHASE_DOWN_END+i*12+4];
    	  }
    	  
    	  offsetSta += 13;
    }
}

/*******************************************************
��������: energyCompute
��������: ����������(��,�µ�����)
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ:
*******************************************************/
BOOL energyCompute(INT16U pn,INT8U *pCopyEnergyBuff,INT8U *pBalanceEnergyBuff, DATE_TIME statisTime,INT8U type)
{
	  DATE_TIME   tmpTime;
	  INT16U      offsetCopy, offsetBalance;
	  INT8U       dataType, tariff, i;
	  INT32U      rawInt, rawDec, sumInt, sumDec, tmpData;
	  INT8U       tmpSign;
	  INT8U       tmpBuff[LEN_OF_ENERGY_BALANCE_RECORD];
	  MEASURE_POINT_PARA *pPointPara;
	  DATE_TIME          readTime;
	  
	  tmpTime = timeBcdToHex(statisTime);
    tmpTime = backTime(tmpTime,0,1,0,0,0);
    tmpTime = timeHexToBcd(tmpTime);
    
    if (type==1 || type==3) //�����յ���
    {
       //��ȡǰһ�����һ��������Ϊ����ο�
       readTime = tmpTime;
       if (readMeterData(tmpBuff, pn, DAY_BALANCE, DAY_FREEZE_COPY_DATA, &readTime,0) == FALSE)
       {
    	   //����ò���ǰһ�����һ�����ݣ���ȡ�����һ��������Ϊ����ο�
    	   readTime = statisTime;
         if (readMeterData(tmpBuff, pn, FIRST_TODAY, ENERGY_DATA, &readTime, 0) == FALSE)
         {
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
           	 printf("energyCompute:δ�������յ�һ������\n");
           }

           return FALSE;
         }
         else
         {
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
           	 printf("energyCompute:�������յ�һ������,����ʱ��=%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
           }
         }
       }
       else
       {
         if (debugInfo&PRINT_BALANCE_DEBUG)
         {
           printf("energyCompute:������һ���ն�������,����ʱ��=%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
         }
       }
       
       //�����յ�ǰ����������/������/�޹�����4�ÿ��8���ʣ�ÿ����7�ֽ�(��1�ֽڷ���)
       offsetCopy = POSITIVE_WORK_OFFSET;
       offsetBalance = DAY_P_WORK_OFFSET;
		}
		else         //�����µ���
		{
       //��ȡ����������Ϊ����ο�
       //readTime = statisTime;
       //ly,2011-04-20,���㵱�µ����õ�ǰֵ��ȥ���µ�һ������,
       //if (readMeterData(tmpBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &readTime, 0) == FALSE)
       //{
    	   //����ò�����������,��ȡ���µ�һ��������Ϊ����ο�
    	   readTime = statisTime;
         if (readMeterData(tmpBuff, pn, FIRST_MONTH, ENERGY_DATA, &readTime, 0) == FALSE)
         {
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
           	 printf("energyCompute:δ�������µ�һ������\n");
           }
           return FALSE;
         }
    	 //}
    	 
    	 //�����µ�ǰ����������/������/�޹�����4�ÿ��8���ʣ�ÿ����7�ֽ�(��1�ֽڷ���)
    	 offsetCopy = POSITIVE_WORK_OFFSET;
       offsetBalance = MONTH_P_WORK_OFFSET;
		}
		
		 pPointPara = (MEASURE_POINT_PARA *)malloc(sizeof(MEASURE_POINT_PARA));
		 if(selectViceParameter(0x04, 25, pn, (INT8U *)pPointPara, sizeof(MEASURE_POINT_PARA)) == FALSE)
		 {
       if (pPointPara!=NULL)
       {
         free(pPointPara);
         pPointPara = NULL;
       }
     }

    for (dataType = 0; dataType < 4; dataType++)
    {
      //������ѭ������
      for (tariff = 0; tariff <= 8; tariff++)
  	  {
  	  	 //ֻҪ����һ��������ԭʼ���ݲ����ڣ����������Ϊ�޸�������
  	     if (pCopyEnergyBuff[offsetCopy] == 0xEE || tmpBuff[offsetCopy] == 0xEE)
  	     {
  	       pBalanceEnergyBuff[offsetBalance]   = 0xEE;   //����
  	     
  	       pBalanceEnergyBuff[offsetBalance+1] = 0xEE;   //С��
  	       pBalanceEnergyBuff[offsetBalance+2] = 0xEE;   //С��
  	     
  	       pBalanceEnergyBuff[offsetBalance+3] = 0xEE;   //����
  	       pBalanceEnergyBuff[offsetBalance+4] = 0xEE;   //����
  	       pBalanceEnergyBuff[offsetBalance+5] = 0xEE;   //����
  	       pBalanceEnergyBuff[offsetBalance+6] = 0xEE;   //����
  	     }
  	     else
  	     {
  	       rawInt = rawDec = sumInt = sumDec = 0;
  	     
  	       //��ԭʼ����ת���ɶ����Ƹ�ʽ
  	       if (type==1 || type==2) //485�˿ڲ��������ʾֵ��4���ֽ�
  	       {
  	         tmpData = pCopyEnergyBuff[offsetCopy]<<8;
  	         sumDec  = bcdToHex(tmpData);
    	     
    	       tmpData = pCopyEnergyBuff[offsetCopy+1] | pCopyEnergyBuff[offsetCopy+2]<<8 | pCopyEnergyBuff[offsetCopy+3]<<16;
    	       sumInt  = bcdToHex(tmpData);
    	       
    	       tmpData = tmpBuff[offsetCopy]<<8;
    	       rawDec  = bcdToHex(tmpData);
    	     
    	       tmpData = tmpBuff[offsetCopy+1] | tmpBuff[offsetCopy+2]<<8 | tmpBuff[offsetCopy+3]<<16;
    	       rawInt  = bcdToHex(tmpData);
    	     }
    	     else          //������������ʾֵ��5���ֽ�
    	     {
  	         tmpData = pCopyEnergyBuff[offsetCopy+1]<<8 | pCopyEnergyBuff[offsetCopy];
  	         sumDec  = bcdToHex(tmpData);
    	     
    	       tmpData = pCopyEnergyBuff[offsetCopy+2] | pCopyEnergyBuff[offsetCopy+3]<<8 | pCopyEnergyBuff[offsetCopy+4]<<16;
    	       sumInt  = bcdToHex(tmpData);

    	       tmpData = tmpBuff[offsetCopy+1]<<8 | tmpBuff[offsetCopy];
    	       rawDec  = bcdToHex(tmpData);
    	     
    	       tmpData = tmpBuff[offsetCopy+2] | tmpBuff[offsetCopy+3]<<8 | tmpBuff[offsetCopy+4]<<16;
    	       rawInt  = bcdToHex(tmpData);
    	     }    	     
    	     
    	     if (debugInfo&PRINT_BALANCE_DEBUG)
    	     {
    	     	 printf("energyCompute:������%d,��������%d����%d,��ǰ����ʾֵ=%d.%04d,�ο�ֵ=%d.%04d\n", pn, dataType, tariff, sumInt, sumDec, rawInt, rawDec);
    	     }
    	     
    	     //Ĭ�ϼ�����Ϊ��
    	     tmpSign = POSITIVE_NUM;
    	    
    	     //��������С���������Ϊ��
           if (sumInt > rawInt || (sumInt == rawInt && sumDec >= rawDec))
           {
             if (sumDec >= rawDec)
             {
                sumDec = sumDec - rawDec;
                sumInt = sumInt - rawInt;
             }
             else
             {
                sumDec = 10000 + sumDec - rawDec;
                sumInt = sumInt - rawInt - 1;
             }
           
             tmpSign = POSITIVE_NUM;
           }
           else  //С�����������������Ϊ��
           {
             if (rawDec >= sumDec)
             {
                sumDec = rawDec - sumDec;
                sumInt = rawInt - sumInt;
             }
             else
             {
                sumDec = 10000 + rawDec - sumDec;
                sumInt = rawInt - sumInt - 1;
             }
           
             tmpSign = NEGTIVE_NUM;
           }

    	     //���Ի��б��ʵõ�����
		       if(pPointPara!=NULL)
		       {
    	       if (pPointPara->voltageTimes != 0)
    	       {
    	         if (pPointPara->currentTimes != 0)
    	         {
      	          sumDec = sumDec * pPointPara->voltageTimes * pPointPara->currentTimes;
                  sumInt = sumInt * pPointPara->voltageTimes * pPointPara->currentTimes;
      	       }
      	       else
      	       {
      	          sumDec = sumDec * pPointPara->voltageTimes;
      	          sumInt = sumInt * pPointPara->voltageTimes;
      	       }
      	     }
      	     else
      	     {
      	       if (pPointPara->currentTimes != 0)
      	       {
      	          sumDec = sumDec * pPointPara->currentTimes;
                  sumInt = sumInt * pPointPara->currentTimes;
      	       }
      	     }
    	     }
    	     
    	     //����С����λ
    	     if (sumDec > 9999)
    	     {
    	       do
    	       {
    	          sumDec -= 10000;
    	          sumInt++;
    	       }while (sumDec > 9999);
    	     }
    	     
    	     //��д���ṹ��
    	     pBalanceEnergyBuff[offsetBalance] = tmpSign;
    	     
    	     tmpData = hexToBcd(sumDec);
    	     pBalanceEnergyBuff[offsetBalance+1] = tmpData & 0xff;
    	     pBalanceEnergyBuff[offsetBalance+2] = tmpData>>8 & 0xff;
    	     
    	     tmpData = hexToBcd(sumInt);
    	     pBalanceEnergyBuff[offsetBalance+3] = tmpData & 0xff;
    	     pBalanceEnergyBuff[offsetBalance+4] = tmpData>>8 & 0xff;
    	     pBalanceEnergyBuff[offsetBalance+5] = tmpData>>16 & 0xff;
    	     pBalanceEnergyBuff[offsetBalance+6] = tmpData>>24 & 0xff;
    	   }
    	   
    	   if (type==3 || type==4)
    	   {
    	     offsetCopy += 5;
    	   }
    	   else
    	   {
    	     offsetCopy += 4;
    	   }
  	     offsetBalance += 7;
  	  }
    }
    
    if (pPointPara!=NULL)
    {
      free(pPointPara);
    }
    
    //������ԭʼ���ݶ��洢���������Ӧ�ô洢������
    return TRUE;
}

/*******************************************************
��������:groupBalance
��������:�ܼ������  
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ:
*******************************************************/
BOOL groupBalance(INT8U *pBalanceZjzData, INT8U gp, INT8U ptNum, INT8U balanceType, DATE_TIME balanceTime)
{
	  INT8U               tmpPn, direction, sign;
	  INT8U               i, j, tariff;
	  INT16U              offset, offsetMonth;
	  INT32U              dataInt[9], dataDec[9], dataSign[9], dataQuantity;
	  INT32U              monthInt[9], monthDec[9], monthSign[9];
    INT32U              dataRawInt, dataRawDec, tmpData;
    METER_DEVICE_CONFIG meterConfig;
    INT8U               realBalanceEnergy[LEN_OF_ENERGY_BALANCE_RECORD];
    DATE_TIME           readTime;
    INT16U              k;
    INT8U               pulsePnData = 0;
    INT8U               tmpBalanceType;
    BOOL                bufHasData;
    BOOL                pnHasNoData;
    
    #ifdef PULSE_GATHER
     INT8U              visionBuff[LENGTH_OF_ENERGY_RECORD];
    #endif
  
    tmpBalanceType = balanceType;
    
    balanceType &=0x7f;

    //�������������㣬Ĭ�ϼ�����Ϊ��
	  for (tariff = 0; tariff < 9; tariff++)
    {
      dataDec[tariff]  = monthDec[tariff]  = 0x00;
      dataInt[tariff]  = monthInt[tariff]  = 0x00;
	    dataSign[tariff] = monthSign[tariff] = POSITIVE_NUM;
	  }
    
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("groupBalance:�ܼ���=%d,���������%d\n",totalAddGroup.perZjz[gp].zjzNo ,ptNum);
    }
    
    //��ָ���ܼ����ÿ������������ܼ�����
   	pnHasNoData = TRUE;
    for(i = 0; i < ptNum; i++)
    {
  	  //ȷ��������ţ����򣬷���
      tmpPn = (totalAddGroup.perZjz[gp].measurePoint[i] & 0x3F) + 1;
      direction = totalAddGroup.perZjz[gp].measurePoint[i] & 0x40;
      sign = totalAddGroup.perZjz[gp].measurePoint[i] & 0x80;
      
			readTime = balanceTime;
   		
   		bufHasData = FALSE;
   		#ifdef PULSE_GATHER
			 	//�鿴�Ƿ����������������
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //���������Ĳ�����
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==tmpPn)
			    {
			      pulsePnData = 1;
 	          
            //if (tmpBalanceType&0x80)
            //{
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("groupBalance:ʵʱ�����������������µ���\n");
              }
              
              memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);
              memset(realBalanceEnergy, 0xee, LEN_OF_ENERGY_BALANCE_RECORD);
  
              //ת��
              covertPulseData(j, visionBuff, NULL, NULL);
              
              //���㵱�յ�����
              bufHasData = energyCompute(tmpPn, visionBuff, realBalanceEnergy, readTime, 3);
  
              //���㵱�µ�����
              bufHasData = energyCompute(tmpPn, visionBuff, realBalanceEnergy, readTime, 4);
            
            /*ly,2011-04-25,����ע��
            }
            else
            {
              //��ȡʵʱͳ�Ƶ��������ݣ�������ʽ����ܼ�
              readTime = balanceTime;

              bufHasData = readMeterData(realBalanceEnergy, tmpPn, REAL_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, 0);
              
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                if (bufHasData)
                {
                  printf("��ȡ�����������ܼӵ����ɹ�\n");
                }
                else
                {
                  printf("��ȡ�����������ܼӵ���ʧ��\n");
                }
              }
            }
            */
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //�����������������
		  {
        //��������Ϸ���
        if (selectF10Data(tmpPn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
        {
      	   //����û�иò����������,�޷������ܼӵ������ļ���
      	   return FALSE;
        }
        else
        {
  	      //�����������д��ڸõ���Ϣ���Ҹò����㲻���ֳ�����豸�ͽ�������װ�ã����ԶԸõ�����ܼ�����
  	      //if (meterConfig.protocol == AC_SAMPLE 
  	      //	  || meterConfig.protocol == SUPERVISAL_DEVICE)
  	      if (meterConfig.protocol == SUPERVISAL_DEVICE)
  	      {
  	      	 return FALSE;
  	      }
        }
        
        //��ȡʵʱͳ�Ƶ��������ݣ�������ʽ����ܼ�
        readTime = queryCopyTime(tmpPn);
        
        bufHasData = readMeterData(realBalanceEnergy, tmpPn, REAL_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, 0);
      }
      	
      if (bufHasData == TRUE)
      {
        if (balanceType == GP_DAY_WORK)  //�ܼ��й�����
        {
          if (direction == 0)  //�����㷽����Ϊ��,ʹ�������й������������ܼ�
          {
        	  offset = DAY_P_WORK_OFFSET;
        	  offsetMonth = MONTH_P_WORK_OFFSET;
          }
          else                 //�����㷽����Ϊ��,ʹ�÷����й������������ܼ�
          {
            offset = DAY_N_WORK_OFFSET;
            offsetMonth = MONTH_N_WORK_OFFSET;
          }
    	  }
    	  else                             //�ܼ��޹����� 
    	  {
    	    if (direction == 0)  //�����㷽����Ϊ��,ʹ�������޹������������ܼ�
          {
        	  offset = DAY_P_NO_WORK_OFFSET;
        	  offsetMonth = MONTH_P_NO_WORK_OFFSET;
          }
          else                 //�����㷽����Ϊ��,ʹ�÷����޹������������ܼ�
          {
            offset = DAY_N_NO_WORK_OFFSET;
            offsetMonth = MONTH_N_NO_WORK_OFFSET;
          }
    	  }
    	  
    	  //�����ʽ����ܼ�
  	    for (tariff = 0; tariff < 9; tariff++)
        {
          if (realBalanceEnergy[offset] == 0xEE)
          {
             dataInt[tariff] = 0xEE;
          }
          else
          {
            dataRawInt = bcdToHex(realBalanceEnergy[offset+3] | realBalanceEnergy[offset+4]<<8 | realBalanceEnergy[offset+5]<<16 | realBalanceEnergy[offset+6]<<24);
            dataRawDec = bcdToHex(realBalanceEnergy[offset+1] | realBalanceEnergy[offset+2]<<8);
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
            	printf("groupBalance:������%d���㵱�շ���%d������=%d.%d\n", tmpPn, tariff, dataRawInt, dataRawDec);
            }
            
            if (sign == 0)    // +
            {
              if (dataSign[tariff] == POSITIVE_NUM)       //�������
              {
                dataInt[tariff] += dataRawInt;
                dataDec[tariff] += dataRawDec;
              }
              else      //�������
              {
                //�ȼ�С����ͬʱ�����λ
                if (dataDec[tariff] >= dataRawDec)
                {
                   dataDec[tariff] -= dataRawDec;
                }
                else
                {
                  dataDec[tariff] = 10000 + dataDec[tariff] - dataRawDec;
                  dataInt[tariff]++;
                }
                //�ټ�����
                if (dataInt[tariff] >= dataRawInt)
                {
                  dataInt[tariff] -= dataRawInt;
                }
                else
                {
                  dataSign[tariff] = POSITIVE_NUM;
                  dataInt[tariff] = dataRawInt - dataInt[tariff];
                }
              }
            }
            else              // -
            {   
              if (dataSign[tariff] == POSITIVE_NUM)
              {
              	//��������С���������Ϊ������������������Ϊ��
                if (dataInt[tariff] > dataRawInt || (dataInt[tariff] == dataRawInt && dataDec[tariff] >= dataRawDec))
                {
                  if (dataDec[tariff] >= dataRawDec)
                  {
                     dataDec[tariff] -= dataRawDec;
                     dataInt[tariff] -= dataRawDec;
                  }
                  else
                  {
                    dataDec[tariff] = 10000 + dataDec[tariff] - dataRawDec;
                    dataInt[tariff] = dataInt[tariff] - dataRawInt - 1;
                  }
                }
                else   //С�����������������Ϊ������������������Ϊ��
                {
                	if (dataRawDec > dataDec[tariff])
                	{
                	  dataDec[tariff] = dataRawDec - dataDec[tariff];
                	  dataInt[tariff] = dataRawInt - dataInt[tariff];
                	  dataSign[tariff] = NEGTIVE_NUM;
                	}
                	else
                	{
                	  dataDec[tariff] = 10000 + dataRawDec - dataDec[tariff];
                	  dataInt[tariff] = dataRawInt - dataInt[tariff] - 1;
                	  dataSign[tariff] = NEGTIVE_NUM;
                	}
                }
              }
              else
              {
              	dataInt[tariff] += dataRawInt;
              	dataDec[tariff] += dataRawDec;
              }
            }
          }
          
          if (realBalanceEnergy[offsetMonth] == 0xEE)
          {
             monthInt[tariff] = 0xEE;
          }
          else
          {
            dataRawInt = bcdToHex(realBalanceEnergy[offsetMonth+3] | realBalanceEnergy[offsetMonth+4]<<8 | realBalanceEnergy[offsetMonth+5]<<16 | realBalanceEnergy[offsetMonth+6]<<24);
            dataRawDec = bcdToHex(realBalanceEnergy[offsetMonth+1] | realBalanceEnergy[offsetMonth+2]<<8);
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
            	printf("groupBalance:������%d���㵱�·���%d������=%d.%d\n", tmpPn, tariff, dataRawInt, dataRawDec);
            }

            if (sign == 0)    // +
            {
              if (monthSign[tariff] == POSITIVE_NUM)       //�������
              {
                monthInt[tariff] += dataRawInt;
                monthDec[tariff] += dataRawDec;
              }
              else      //�������
              {
                //�ȼ�С����ͬʱ�����λ
                if (monthDec[tariff] >= dataRawDec)
                {
                   monthDec[tariff] -= dataRawDec;
                }
                else
                {
                  monthDec[tariff] = 10000 + monthDec[tariff] - dataRawDec;
                  monthInt[tariff]++;
                }
                //�ټ�����
                if (monthInt[tariff] >= dataRawInt)
                {
                  monthInt[tariff] -= dataRawInt;
                }
                else
                {
                  monthSign[tariff] = POSITIVE_NUM;
                  monthInt[tariff] = dataRawInt - monthInt[tariff];
                }
              }
            }
            else              // -
            {
              if (monthSign[tariff] == POSITIVE_NUM)
              {
              	//��������С���������Ϊ������������������Ϊ��
                if (monthInt[tariff] > dataRawInt || (monthInt[tariff] == dataRawInt && monthDec[tariff] >= dataRawDec))
                {
                  if (monthDec[tariff] >= dataRawDec)
                  {
                     monthDec[tariff] -= dataRawDec;
                     monthInt[tariff] -= dataRawDec;
                  }
                  else
                  {
                    monthDec[tariff] = 10000 + monthDec[tariff] - dataRawDec;
                    monthInt[tariff] = monthInt[tariff] - dataRawInt - 1;
                  }
                }
                else   //С�����������������Ϊ������������������Ϊ��
                {
                	if (dataRawDec > monthDec[tariff])
                	{
                	  monthDec[tariff] = dataRawDec - monthDec[tariff];
                	  monthInt[tariff] = dataRawInt - monthInt[tariff];
                	  monthSign[tariff] = NEGTIVE_NUM;
                	}
                	else
                	{
                	  monthDec[tariff] = 10000 + dataRawDec - monthDec[tariff];
                	  monthInt[tariff] = dataRawInt - monthInt[tariff] - 1;
                	  monthSign[tariff] = NEGTIVE_NUM;
                	}
                }
              }
              else
              {
              	monthInt[tariff] += dataRawInt;
              	monthDec[tariff] += dataRawDec;
              }
            }
          }

          offset += 7;
          offsetMonth += 7;
        }
      }
      else
      {
        for (tariff = 0; tariff < 9; tariff++)
        {
          dataInt[tariff] = 0xEE;
          monthInt[tariff] = 0xEE;
	      }
        
        pnHasNoData = FALSE;
        
        break;
      }
      
      usleep(50000);
    }
    
    if (balanceType == GP_DAY_WORK)
    {
    	offset = GP_DAY_WORK;
    	offsetMonth = GP_MONTH_WORK;
    }
    else
    {
    	offset = GP_DAY_NO_WORK;
    	offsetMonth = GP_MONTH_NO_WORK;
    }
    
    for (tariff = 0; tariff < 9; tariff++)
    {
  	  if (dataInt[tariff] != 0xEE)
  	  {
  	    //������λ
  	    if (dataDec[tariff] > 9999)
	      {
	        do
	        {
	          dataDec[tariff] -= 10000;
	          dataInt[tariff] += 1;
	        }while (dataDec[tariff] > 9999);
	      }

  	    dataQuantity = 0;
        dataQuantity = dataFormat(&dataInt[tariff], &dataDec[tariff], FORMAT(3));
        pBalanceZjzData[offset] = dataSign[tariff] | dataQuantity;
        
        tmpData = hexToBcd(dataDec[tariff]);
        pBalanceZjzData[offset+1] = tmpData&0xFF;
        pBalanceZjzData[offset+2] = tmpData>>8&0xFF;
       
        tmpData = hexToBcd(dataInt[tariff]);
        pBalanceZjzData[offset+3] = tmpData&0xFF;
        pBalanceZjzData[offset+4] = tmpData>>8&0xFF;
        pBalanceZjzData[offset+5] = tmpData>>16&0xFF;
        pBalanceZjzData[offset+6] = tmpData>>24&0xFF;
      }
      
      if (monthInt[tariff] != 0xEE)
  	  {
  	    //������λ
  	    if (monthDec[tariff] > 9999)
	      {
	        do
	        {
	          monthDec[tariff] -= 10000;
	          monthInt[tariff] += 1;
	        }while (monthDec[tariff] > 9999);
	      }

  	    dataQuantity = 0;
        dataQuantity = dataFormat(&monthInt[tariff], &monthDec[tariff], FORMAT(3));

        pBalanceZjzData[offsetMonth] = monthSign[tariff] | dataQuantity;
        tmpData = hexToBcd(monthDec[tariff]);

        pBalanceZjzData[offsetMonth+1] = tmpData&0xFF;
        pBalanceZjzData[offsetMonth+2] = tmpData>>8&0xFF;

        tmpData = hexToBcd(monthInt[tariff]);
        pBalanceZjzData[offsetMonth+3] = tmpData&0xFF;
        pBalanceZjzData[offsetMonth+4] = tmpData>>8&0xFF;
        pBalanceZjzData[offsetMonth+5] = tmpData>>16&0xFF;
        pBalanceZjzData[offsetMonth+6] = tmpData>>24&0xFF;
      }
      
      offset += 7;
      offsetMonth += 7;
    }
    
    if (pnHasNoData==FALSE)
    {
      return FALSE;
    }
    else
    {
      return TRUE;
    }
}

/*******************************************************
��������: groupStatistic
��������: �ܼ���ͳ�� 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ:
*******************************************************/
BOOL groupStatistic(INT8U *pBalanceZjzData, INT8U gp, INT8U ptNum, INT8U balanceType,DATE_TIME statisTime)
{        
    INT8U               k;
    INT16U              tmpPn, direction, sign;
	  INT8U               tmpReadBuff[maxData(LENGTH_OF_PARA_RECORD,LEN_OF_ZJZ_BALANCE_RECORD)];
	  INT16U              i, j, offset;
    INT32U              powerInt, powerDec, powerSign;
    INT8U               powerQuantity;
    INT32U              dataRawInt, dataRawDec, tmpData;
	 
	  MEASURE_POINT_PARA  pointPara;
	 
	  METER_DEVICE_CONFIG meterConfig;
	  DATE_TIME           readTime;
    INT8U               pulsePnData = 0;  
    BOOL                buffHasData;  
  
    powerInt  = 0x0;
    powerDec  = 0x0;
    powerSign = POSITIVE_NUM;
    
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("groupStatistic:���������%d\n",ptNum);
    }
    
    //<- - - - - �ܼӹ���ͳ��- - - - - ->
	  for(i = 0; i < ptNum; i++)
    { 
  	  //ȷ��������ţ����򣬷���
      tmpPn = (totalAddGroup.perZjz[gp].measurePoint[i] & 0x3F) + 1;
      direction = totalAddGroup.perZjz[gp].measurePoint[i] & 0x40;
      sign = totalAddGroup.perZjz[gp].measurePoint[i] & 0x80;
      buffHasData = FALSE;
      pulsePnData = 0;
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("�ܼ������=%d,������%d\n",gp,tmpPn);
      }
      
   		#ifdef PULSE_GATHER
			 	//�鿴�Ƿ����������������
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //���������Ĳ�����
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==tmpPn)
			    {
			      //P.1�ȳ�ʼ������
            memset(tmpReadBuff,0xee,LENGTH_OF_PARA_RECORD);

				 	  //P.2���������Ĺ�������dataBuff��Ӧ��λ����
			   	  covertPulseData(j, NULL,NULL,tmpReadBuff);
			   	  	 	    
			      pulsePnData = 1;
			      
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("groupStatistic:ת��������%d������\n",tmpPn);
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
              memset(tmpReadBuff,0xee,LENGTH_OF_PARA_RECORD);

			       	//A.2������������������dataBuff��
			       	covertAcSample(tmpReadBuff, NULL, NULL, 1, sysTime);
              
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("groupStatistic:ת��������%d����ֵ\n",tmpPn);
              }
			      	       	  
			     	  buffHasData = TRUE;
		   	    }
		      }
		  	  else
		  	  {
            readTime = queryCopyTime(tmpPn);
            buffHasData = readMeterData(tmpReadBuff, tmpPn, PRESENT_DATA, PARA_VARIABLE_DATA, &readTime, 0);
          }
        }
      }
      
      if (buffHasData == TRUE)
      {
        if (balanceType == GP_DAY_WORK)   //�ܼ��й�����
        {
          if (tmpReadBuff[POWER_INSTANT_WORK] != 0xEE)
          {
      	  	dataRawInt = bcdToHex(tmpReadBuff[POWER_INSTANT_WORK+2]&0x7f);
            dataRawDec = bcdToHex(tmpReadBuff[POWER_INSTANT_WORK]|tmpReadBuff[POWER_INSTANT_WORK+1]<<8);
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
    	  }
    	  else                 //�ܼ��޹�����
    	  {
    	  	if (tmpReadBuff[POWER_INSTANT_NO_WORK] != 0xEE)
          {
      	  	dataRawInt = bcdToHex(tmpReadBuff[POWER_INSTANT_NO_WORK+2]&0x7f);
            dataRawDec = bcdToHex(tmpReadBuff[POWER_INSTANT_NO_WORK]|tmpReadBuff[POWER_INSTANT_NO_WORK+1]<<8);
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
        }
      	//4-5-3.��ȡ�������Ӧ�����Ʋ���
		    if(selectViceParameter(0x04, 25, tmpPn, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
    	  {
           if (pointPara.voltageTimes != 0)
           {
              if (pointPara.currentTimes != 0)
              {
   	             dataRawDec *= pointPara.voltageTimes * pointPara.currentTimes;
                 dataRawInt *= pointPara.voltageTimes * pointPara.currentTimes;
   	          }
   	          else
   	          {
   	             dataRawDec *= pointPara.voltageTimes;
   	             dataRawInt *= pointPara.voltageTimes;
   	          }
   	       }
   	       else
   	       {
   	          if (pointPara.currentTimes != 0)
   	          {
   	             dataRawDec *= pointPara.currentTimes;
                 dataRawInt *= pointPara.currentTimes;
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
    
    //<- - - - - �������ݸ�ʽ�� ����α�������- - - - - >
    if (balanceType == GP_DAY_WORK)
    {
    	offset = GP_WORK_POWER;
    }
    else
    {
      offset = GP_NO_WORK_POWER;
    }
    
    if (powerInt != 0xEE)
    {
      //********************100 for testing power ctrl*********************************
      //powerInt *= 100;
      //powerDec *= 100;
      //********************100 for testing power ctrl*********************************
      if (powerDec > 9999)
      {
        do
        {
          powerInt += 1;
          powerDec -= 10000;
        }while (powerDec > 10000);
      }
    
      powerQuantity = dataFormat(&powerInt, &powerDec, FORMAT(2));
      
      pBalanceZjzData[offset] = powerQuantity;

      tmpData = hexToBcd(powerInt);
      
      pBalanceZjzData[offset+1] = tmpData&0xFF;
      pBalanceZjzData[offset+2] = tmpData>>8&0xFF;
    }
    
    //<- - - - - �Ƚϵ��չ������ֵ�͵��¹������ֵ- - - - - >
    //��ȡǰһ��ʵʱ�ܼ�������Ϊͳ�Ʋο�
    if (balanceType == GP_DAY_WORK)
    {
      //if (readMeterData(tmpReadBuff, totalAddGroup.perZjz[gp].zjzNo, GROUP_REAL_BALANCE, LAST_TODAY, statisTime) == TRUE)
      readTime = statisTime;
      if (readMeterData(tmpReadBuff, totalAddGroup.perZjz[gp].zjzNo, REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0) == TRUE)
      {
      	//�ϴ�ͳ�Ƶ������ͳ����δ�������ڴ˻����ϼ���ͳ��
      	if (tmpReadBuff[GP_DAY_OVER] != 0x01)
      	{
          //���ν������ܼ��й����ʽ������������һ��ͳ�ƽ�����бȽ�
          if (pBalanceZjzData[GP_WORK_POWER] != 0xEE)
          {
          	//�� �� �� �� �� �� �� �� ��
            if (tmpReadBuff[GP_DAY_MAX_POWER] != 0xEE)
            {
              if ((tmpReadBuff[GP_DAY_MAX_POWER+2]<pBalanceZjzData[GP_WORK_POWER+2])
        	      || (tmpReadBuff[GP_DAY_MAX_POWER+2]==pBalanceZjzData[GP_WORK_POWER+2] && tmpReadBuff[GP_DAY_MAX_POWER+1]<pBalanceZjzData[GP_WORK_POWER+1]))
        	    {
        	      pBalanceZjzData[GP_DAY_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	      pBalanceZjzData[GP_DAY_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	      pBalanceZjzData[GP_DAY_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	      pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=statisTime.minute;
        	      pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=statisTime.hour;
        	      pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=statisTime.day;
        	    }
        	    else
        	    {
        	      pBalanceZjzData[GP_DAY_MAX_POWER]=tmpReadBuff[GP_DAY_MAX_POWER];
        	      pBalanceZjzData[GP_DAY_MAX_POWER+1]=tmpReadBuff[GP_DAY_MAX_POWER+1];
        	      pBalanceZjzData[GP_DAY_MAX_POWER+2]=tmpReadBuff[GP_DAY_MAX_POWER+2];
        	    
        	      pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=tmpReadBuff[GP_DAY_MAX_POWER_TIME];
        	      pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=tmpReadBuff[GP_DAY_MAX_POWER_TIME+1];
        	      pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=tmpReadBuff[GP_DAY_MAX_POWER_TIME+2];
        	    }
        	  }
        	  else   //�ϴ�ͳ��û�еó�������ܼ��й����ʣ����Ա����ܼӵ��й�������Ϊͳ�ƻ���
        	  {
        	    pBalanceZjzData[GP_DAY_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	    pBalanceZjzData[GP_DAY_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_DAY_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	    
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=statisTime.minute;
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=statisTime.hour;
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=statisTime.day;
        	  }
        	  
        	  //�� �� С �� �� �� �� �� ��
        	  if (tmpReadBuff[GP_DAY_MIN_POWER] != 0xEE)
            {
              if ((tmpReadBuff[GP_DAY_MIN_POWER+2]>pBalanceZjzData[GP_WORK_POWER+2])
        	      || (tmpReadBuff[GP_DAY_MIN_POWER+2]==pBalanceZjzData[GP_WORK_POWER+2] && tmpReadBuff[GP_DAY_MIN_POWER+1]>pBalanceZjzData[GP_WORK_POWER+1]))
        	    {
        	      pBalanceZjzData[GP_DAY_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	      pBalanceZjzData[GP_DAY_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	      pBalanceZjzData[GP_DAY_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	      pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=statisTime.minute;
        	      pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=statisTime.hour;
        	      pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=statisTime.day;
        	    }
        	    else
        	    {
        	      pBalanceZjzData[GP_DAY_MIN_POWER]=tmpReadBuff[GP_DAY_MIN_POWER];
        	      pBalanceZjzData[GP_DAY_MIN_POWER+1]=tmpReadBuff[GP_DAY_MIN_POWER+1];
        	      pBalanceZjzData[GP_DAY_MIN_POWER+2]=tmpReadBuff[GP_DAY_MIN_POWER+2];
        	    
        	      pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=tmpReadBuff[GP_DAY_MIN_POWER_TIME];
        	      pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=tmpReadBuff[GP_DAY_MIN_POWER_TIME+1];
        	      pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=tmpReadBuff[GP_DAY_MIN_POWER_TIME+2];
        	    }
        	  }
        	  else   //�ϴ�ͳ��û�еó�����С�ܼ��й����ʣ����Ա����ܼӵ��й�������Ϊͳ�ƻ���
        	  {
        	    pBalanceZjzData[GP_DAY_MIN_POWER]   = pBalanceZjzData[GP_WORK_POWER];
        	    pBalanceZjzData[GP_DAY_MIN_POWER+1] = pBalanceZjzData[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_DAY_MIN_POWER+2] = pBalanceZjzData[GP_WORK_POWER+2];
        	    
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME]   = statisTime.minute;
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1] = statisTime.hour;
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2] = statisTime.day;
        	  }
        	  
        	  //�չ���Ϊ��ʱ��
        	  if (tmpReadBuff[GP_DAY_ZERO_POWER_TIME] != 0xEE)
        	  {
        	  	if (pBalanceZjzData[GP_WORK_POWER+1]==0x00 && pBalanceZjzData[GP_WORK_POWER+2]==0x00)
        	  	{
        	  	  tmpData = tmpReadBuff[GP_DAY_ZERO_POWER_TIME]|tmpReadBuff[GP_DAY_ZERO_POWER_TIME+1]<<8;
        	  	  //ly,10-01-18 tmpData += copyInterval;
        	  	  pBalanceZjzData[GP_DAY_ZERO_POWER_TIME]   = tmpData&0xff;
        	  	  pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = tmpData>>8&0xff;
        	  	}
        	  	else
        	  	{
        	  	  pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = tmpReadBuff[GP_DAY_ZERO_POWER_TIME];
        	  	  pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = tmpReadBuff[GP_DAY_ZERO_POWER_TIME+1];
        	  	}
        	  }
        	  else  //�ϴ�ͳ��û�еó��չ���Ϊ��ʱ�䣬���Ա����ܼӵ��й�������Ϊͳ�ƻ���
        	  {
        	  	if (pBalanceZjzData[GP_WORK_POWER+1]==0x00&&pBalanceZjzData[GP_WORK_POWER+2]==0x00)
        	  	{
          	    /*ly,10-01-18
          	    pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = copyInterval&0xff;
          	  	pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = copyInterval>>8&0xff;
          	  	*/
          	  }
          	  else
          	  {
          	    pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = 0x00;
          	  	pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = 0x00;
          	  }
        	  }
          }
          else //���ν���û���ܼ��й����ʽ����ͳ��ֵΪ��һ��ͳ�ƽ�������ϴ�ͳ��Ҳû�н����ͳ��ֵΪ��
          {
          	//�� �� �� �� �� ֵ
            if (tmpReadBuff[GP_DAY_MAX_POWER] != 0xEE)
            {
              pBalanceZjzData[GP_DAY_MAX_POWER]=tmpReadBuff[GP_DAY_MAX_POWER];
        	    pBalanceZjzData[GP_DAY_MAX_POWER+1]=tmpReadBuff[GP_DAY_MAX_POWER+1];
        	    pBalanceZjzData[GP_DAY_MAX_POWER+2]=tmpReadBuff[GP_DAY_MAX_POWER+2];
        	  
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=tmpReadBuff[GP_DAY_MAX_POWER_TIME];
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=tmpReadBuff[GP_DAY_MAX_POWER_TIME+1];
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=tmpReadBuff[GP_DAY_MAX_POWER_TIME+2];
        	  }
        	  else
        	  {
        	    pBalanceZjzData[GP_DAY_MAX_POWER]=0x00;
        	    pBalanceZjzData[GP_DAY_MAX_POWER+1]=0x00;
        	    pBalanceZjzData[GP_DAY_MAX_POWER+2]=0x00;
        	  
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=0xEE;
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=0xEE;
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=0xEE;
        	  }
        	  
        	  //�� �� �� �� С ֵ
        	  if (tmpReadBuff[GP_DAY_MIN_POWER] != 0xEE)
        	  {  
        	    pBalanceZjzData[GP_DAY_MIN_POWER]=tmpReadBuff[GP_DAY_MIN_POWER];
        	    pBalanceZjzData[GP_DAY_MIN_POWER+1]=tmpReadBuff[GP_DAY_MIN_POWER+1];
        	    pBalanceZjzData[GP_DAY_MIN_POWER+2]=tmpReadBuff[GP_DAY_MIN_POWER+2];
        	  
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=tmpReadBuff[GP_DAY_MIN_POWER_TIME];
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=tmpReadBuff[GP_DAY_MIN_POWER_TIME+1];
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=tmpReadBuff[GP_DAY_MIN_POWER_TIME+2];
            }
            else
            {
              pBalanceZjzData[GP_DAY_MIN_POWER]=0x00;
        	    pBalanceZjzData[GP_DAY_MIN_POWER+1]=0x00;
        	    pBalanceZjzData[GP_DAY_MIN_POWER+2]=0x00;
        	  
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=0xEE;
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=0xEE;
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=0xEE;
            }
            
            //�� �� �� Ϊ �� ʱ ��
            if (tmpReadBuff[GP_DAY_ZERO_POWER_TIME] != 0xEE)
            {
              pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = tmpReadBuff[GP_DAY_ZERO_POWER_TIME];
              pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = tmpReadBuff[GP_DAY_ZERO_POWER_TIME+1];
            }
            else
            {
              pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = 0x00;
              pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = 0x00;
            }
          }
        }	
        else  //ǰһ��ͳ�ƽ��������¿�ʼ����ͳ��
        {
          tmpReadBuff[GP_DAY_OVER] = 0x0;
          if (pBalanceZjzData[GP_WORK_POWER] != 0xEE)
          {
          	pBalanceZjzData[GP_DAY_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	  pBalanceZjzData[GP_DAY_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	  pBalanceZjzData[GP_DAY_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	  pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=statisTime.minute;
        	  pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=statisTime.hour;
        	  pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=statisTime.day;
        	  
        	  pBalanceZjzData[GP_DAY_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	  pBalanceZjzData[GP_DAY_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	  pBalanceZjzData[GP_DAY_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	  pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=statisTime.minute;
        	  pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=statisTime.hour;
        	  pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=statisTime.day;
        	  
        	  if (pBalanceZjzData[GP_WORK_POWER+1]==0x00&&pBalanceZjzData[GP_WORK_POWER+2]==0x00)
        	  {
        	    /*ly,10-01-18
        	    pBalanceZjzData[GP_DAY_ZERO_POWER_TIME]=copyInterval&0xFF;
        	    pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1]=copyInterval>>8&0xFF;
        	    */
        	  }
          }
          else
          {
            pBalanceZjzData[GP_DAY_MAX_POWER]=0x00;
        	  pBalanceZjzData[GP_DAY_MAX_POWER+1]=0x00;
        	  pBalanceZjzData[GP_DAY_MAX_POWER+2]=0x00;
        	      
        	  pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=0xEE;
        	  pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=0xEE;
        	  pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=0xEE;
        	  
        	  pBalanceZjzData[GP_DAY_MIN_POWER]=0x00;
        	  pBalanceZjzData[GP_DAY_MIN_POWER+1]=0x00;
        	  pBalanceZjzData[GP_DAY_MIN_POWER+2]=0x00;
        	      
        	  pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=0xEE;
        	  pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=0xEE;
        	  pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=0xEE;
        	  
        	  pBalanceZjzData[GP_DAY_ZERO_POWER_TIME]=0x00;
        	  pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1]=0x00;
          }
        }	
        
        //ǰһ��ͳ��δ������ͳ�ƽ�������ǰһ�λ����ϼ���ͳ��
        if (tmpReadBuff[GP_MONTH_OVER] != 0x01)
        {
        	//���ν������ܼ��й����ʽ������������һ��ͳ�ƽ�����бȽ�
          if (pBalanceZjzData[GP_WORK_POWER] != 0xEE)
          {
            //������й�����
            if (tmpReadBuff[GP_MONTH_MAX_POWER] != 0xEE)
            {
              if ((tmpReadBuff[GP_MONTH_MAX_POWER+2]<pBalanceZjzData[GP_WORK_POWER+2])
        	      || (tmpReadBuff[GP_MONTH_MAX_POWER+2]==pBalanceZjzData[GP_WORK_POWER+2] && tmpReadBuff[GP_DAY_MAX_POWER+1]<pBalanceZjzData[GP_WORK_POWER+1]))
        	    {
        	      pBalanceZjzData[GP_MONTH_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	      pBalanceZjzData[GP_MONTH_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	      pBalanceZjzData[GP_MONTH_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	      pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=statisTime.minute;
        	      pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=statisTime.hour;
        	      pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=statisTime.day;
        	    }
        	    else
        	    {
        	      pBalanceZjzData[GP_MONTH_MAX_POWER]=tmpReadBuff[GP_MONTH_MAX_POWER];
        	      pBalanceZjzData[GP_MONTH_MAX_POWER+1]=tmpReadBuff[GP_MONTH_MAX_POWER+1];
        	      pBalanceZjzData[GP_MONTH_MAX_POWER+2]=tmpReadBuff[GP_MONTH_MAX_POWER+2];
        	    
        	      pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=tmpReadBuff[GP_MONTH_MAX_POWER_TIME];
        	      pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=tmpReadBuff[GP_MONTH_MAX_POWER_TIME+1];
        	      pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=tmpReadBuff[GP_MONTH_MAX_POWER_TIME+2];
        	    }
        	  }
        	  else
        	  {
        	    pBalanceZjzData[GP_MONTH_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	    pBalanceZjzData[GP_MONTH_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_MONTH_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	    
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=statisTime.minute;
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=statisTime.hour;
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=statisTime.day;
        	  }
        	  
        	  //����С�й�����
        	  if (tmpReadBuff[GP_MONTH_MIN_POWER] != 0xEE)
            {
              if ((tmpReadBuff[GP_MONTH_MIN_POWER+2]>pBalanceZjzData[GP_WORK_POWER+2])
        	      || (tmpReadBuff[GP_MONTH_MIN_POWER+2]==pBalanceZjzData[GP_WORK_POWER+2] && tmpReadBuff[GP_MONTH_MIN_POWER+1]>pBalanceZjzData[GP_WORK_POWER+1]))
        	    {
        	      pBalanceZjzData[GP_MONTH_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	      pBalanceZjzData[GP_MONTH_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	      pBalanceZjzData[GP_MONTH_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	      pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=statisTime.minute;
        	      pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=statisTime.hour;
        	      pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=statisTime.day;
        	    }
        	    else
        	    {
        	      pBalanceZjzData[GP_MONTH_MIN_POWER]=tmpReadBuff[GP_MONTH_MIN_POWER];
        	      pBalanceZjzData[GP_MONTH_MIN_POWER+1]=tmpReadBuff[GP_MONTH_MIN_POWER+1];
        	      pBalanceZjzData[GP_MONTH_MIN_POWER+2]=tmpReadBuff[GP_MONTH_MIN_POWER+2];
        	    
        	      pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=tmpReadBuff[GP_MONTH_MIN_POWER_TIME];
        	      pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=tmpReadBuff[GP_MONTH_MIN_POWER_TIME+1];
        	      pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=tmpReadBuff[GP_MONTH_MIN_POWER_TIME+2];
        	    }
        	  }
        	  else
        	  {
        	    pBalanceZjzData[GP_MONTH_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	    pBalanceZjzData[GP_MONTH_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_MONTH_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	    
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=statisTime.minute;
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=statisTime.hour;
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=statisTime.day;
        	  }
        	  
        	  //���й�����Ϊ��ʱ��
        	  if (pBalanceZjzData[GP_WORK_POWER+1]==0x00 && pBalanceZjzData[GP_WORK_POWER+2]==0x00)
        	  {
        	    if (tmpReadBuff[GP_MONTH_ZERO_POWER_TIME]!=0xEE)
        	    {
        	      tmpData = tmpReadBuff[GP_MONTH_ZERO_POWER_TIME]|tmpReadBuff[GP_MONTH_ZERO_POWER_TIME+1]<<8;
        	      //ly,10-01-18 tmpData += copyInterval;
        	      pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = tmpData&0xFF;
       	        pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = tmpData>>8&0xFF;
        	    }
        	    else
        	    {
        	      /*ly,10-01-18
        	      pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = copyInterval&0xff;
       	        pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = copyInterval>>8&0xFF;
       	        */
        	    }
        	  }
        	  else
        	  {
        	    if (tmpReadBuff[GP_MONTH_ZERO_POWER_TIME]!=0xEE)
        	    {
        	      pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = tmpReadBuff[GP_MONTH_ZERO_POWER_TIME];
       	        pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = tmpReadBuff[GP_MONTH_ZERO_POWER_TIME+1];
        	    }
        	    else
        	    {
        	      pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = 0x00;
       	        pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = 0x00;
        	    }
        	  }
          }
          else
          {
            //������й�����
            if (tmpReadBuff[GP_MONTH_MAX_POWER] != 0xEE)
            {
              pBalanceZjzData[GP_MONTH_MAX_POWER]=tmpReadBuff[GP_WORK_POWER];
        	    pBalanceZjzData[GP_MONTH_MAX_POWER+1]=tmpReadBuff[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_MONTH_MAX_POWER+2]=tmpReadBuff[GP_WORK_POWER+2];
        	  
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=tmpReadBuff[GP_MONTH_MAX_POWER_TIME];
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=tmpReadBuff[GP_MONTH_MAX_POWER_TIME+1];
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=tmpReadBuff[GP_MONTH_MAX_POWER_TIME+2];
        	  }
        	  else
        	  {
        	    pBalanceZjzData[GP_MONTH_MAX_POWER]=0x00;
        	    pBalanceZjzData[GP_MONTH_MAX_POWER+1]=0x00;
        	    pBalanceZjzData[GP_MONTH_MAX_POWER+2]=0x00;
        	  
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=0xEE;
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=0xEE;
        	    pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=0xEE;
        	  }
        	  
        	  //����С�й����� 
        	  if (tmpReadBuff[GP_MONTH_MIN_POWER] != 0xEE)
        	  {
        	    pBalanceZjzData[GP_MONTH_MIN_POWER]=tmpReadBuff[GP_MONTH_MIN_POWER];
        	    pBalanceZjzData[GP_MONTH_MIN_POWER+1]=tmpReadBuff[GP_MONTH_MIN_POWER+1];
        	    pBalanceZjzData[GP_MONTH_MIN_POWER+2]=tmpReadBuff[GP_MONTH_MIN_POWER+2];
        	  
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=tmpReadBuff[GP_MONTH_MIN_POWER_TIME];
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=tmpReadBuff[GP_MONTH_MIN_POWER_TIME+1];
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=tmpReadBuff[GP_MONTH_MIN_POWER_TIME+2];
            }
            else
            {
        	    pBalanceZjzData[GP_MONTH_MIN_POWER]=0x00;
        	    pBalanceZjzData[GP_MONTH_MIN_POWER+1]=0x00;
        	    pBalanceZjzData[GP_MONTH_MIN_POWER+2]=0x00;
        	  
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=0xEE;
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=0xEE;
        	    pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=0xEE;
            }
            
            //���й�����Ϊ��ʱ��
            if (tmpReadBuff[GP_MONTH_ZERO_POWER_TIME] != 0xEE)
        	  {  
        	    pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME]=tmpReadBuff[GP_MONTH_ZERO_POWER_TIME];
        	    pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1]=tmpReadBuff[GP_MONTH_ZERO_POWER_TIME+1];
            }
            else
            {
        	    pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME]=0x00;
        	    pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1]=0x00;
            }
          }
        }
        else   //ǰһ����ͳ�ƽ�������ʼ�µ���ͳ������
        {
          tmpReadBuff[GP_MONTH_OVER] = 0x00;
          if (pBalanceZjzData[GP_WORK_POWER] != 0xEE)
          {
          	pBalanceZjzData[GP_MONTH_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	  pBalanceZjzData[GP_MONTH_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	  pBalanceZjzData[GP_MONTH_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	  pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=statisTime.minute;
        	  pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=statisTime.hour;
        	  pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=statisTime.day;
        	  
        	  pBalanceZjzData[GP_MONTH_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	  pBalanceZjzData[GP_MONTH_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	  pBalanceZjzData[GP_MONTH_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	      
        	  pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=statisTime.minute;
        	  pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=statisTime.hour;
        	  pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=statisTime.day;
        	  
        	  if (pBalanceZjzData[GP_WORK_POWER+1]==0x00&&pBalanceZjzData[GP_WORK_POWER+2]==0x00)
        	  {
        	    /*ly,10-01-18
        	    pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME]=copyInterval&0xFF;
        	    pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1]=copyInterval>>8&0xFF;
        	    */
        	  }
          }
          else
          {
            pBalanceZjzData[GP_MONTH_MAX_POWER]=0x00;
        	  pBalanceZjzData[GP_MONTH_MAX_POWER+1]=0x00;
        	  pBalanceZjzData[GP_MONTH_MAX_POWER+2]=0x00;
        	      
        	  pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=0xEE;
        	  pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=0xEE;
        	  pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=0xEE;
        	  
        	  pBalanceZjzData[GP_MONTH_MIN_POWER]=0x00;
        	  pBalanceZjzData[GP_MONTH_MIN_POWER+1]=0x00;
        	  pBalanceZjzData[GP_MONTH_MIN_POWER+2]=0X00;
        	      
        	  pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=0xEE;
        	  pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=0xEE;
        	  pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=0xEE;
        	  
        	  pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME]=0x00;
        	  pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1]=0x00;
          }
        }
      }
      else  //����ȡ��ǰһ�ε�ͳ�����ݣ����¿�ʼͳ��
      {
        if (pBalanceZjzData[GP_WORK_POWER+1] != 0xEE)
        {
          //�����
          pBalanceZjzData[GP_DAY_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_DAY_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_DAY_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=statisTime.day;
        	//����С
        	pBalanceZjzData[GP_DAY_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_DAY_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_DAY_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=statisTime.day;
          //�����
        	pBalanceZjzData[GP_MONTH_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_MONTH_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_MONTH_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=statisTime.day;
        	//����С
        	pBalanceZjzData[GP_MONTH_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_MONTH_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_MONTH_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=statisTime.day;
        	//�ա��¹���Ϊ��
        	if (pBalanceZjzData[GP_WORK_POWER+1] == 0x00 && pBalanceZjzData[GP_WORK_POWER+2] == 0x00)
          {
            /*ly,10-01-18
            pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = copyInterval&0xFF;
            pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = copyInterval>>8&0xFF;
            
            pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = copyInterval&0xFF;
            pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = copyInterval>>8&0xFF;
            */
          }
          else
          {
            pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = 0x00;
            pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = 0x0;
            
            pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = 0x00;
            pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = 0x00;
          }
        }
        else
        {
          //�����
          pBalanceZjzData[GP_DAY_MAX_POWER]=0x00;
        	pBalanceZjzData[GP_DAY_MAX_POWER+1]=0x00;
        	pBalanceZjzData[GP_DAY_MAX_POWER+2]=0x00;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=0xEE;
        	//����С
        	pBalanceZjzData[GP_DAY_MIN_POWER]=0x00;
        	pBalanceZjzData[GP_DAY_MIN_POWER+1]=0x00;
        	pBalanceZjzData[GP_DAY_MIN_POWER+2]=0x00;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=0xEE;
          //�����
        	pBalanceZjzData[GP_MONTH_MAX_POWER]=0x00;
        	pBalanceZjzData[GP_MONTH_MAX_POWER+1]=0x00;
        	pBalanceZjzData[GP_MONTH_MAX_POWER+2]=0x00;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=0xEE;
        	//����С
        	pBalanceZjzData[GP_MONTH_MIN_POWER]=0x00;
        	pBalanceZjzData[GP_MONTH_MIN_POWER+1]=0x00;
        	pBalanceZjzData[GP_MONTH_MIN_POWER+2]=0x00;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=0xEE;
        	//�ա�����Ϊ��
          pBalanceZjzData[GP_DAY_ZERO_POWER_TIME] = 0x00;
          pBalanceZjzData[GP_DAY_ZERO_POWER_TIME+1] = 0x0;
            
          pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME] = 0x00;
          pBalanceZjzData[GP_MONTH_ZERO_POWER_TIME+1] = 0x00;
        }
      }
    }
    
    return TRUE;
}

/*******************************************************
��������: eventRecord
��������: �������¼������������������ܱ���ߡ�ͣ�ߡ�ʾ���½�
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ�� 
*******************************************************/
void eventRecord(INT16U pn, INT8U *pCopyEnergyBuff, INT8U *pCopyParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, INT8U statisInterval, DATE_TIME statisTime)
{
  INT8U      stopEvent[25];       //ͣ���¼�
  INT8U      flyEvent[24];        //�����¼�
  INT8U      reverseEvent[22];    //�½��¼�
  INT8U      overEvent[22];       //�����¼�,2013-11-21,add
  INT8U      meterEvent[10];      //���ܱ������Ϣ
  INT32U     tmpPresHex, tmpLastHex, tmpGateHex, tmpMeterData, tmpCountData;
  INT32U     tmpOverGateHex;      //������ֵ,2013-11-21,add
  DATE_TIME  tmpCopyTime, tmpStopTime;
  INT8U      lastLastCopyEnergy[LENGTH_OF_ENERGY_RECORD];
  DATE_TIME  readTime;
  INT8U      stopDataTail,reverseDataTail,flyDataTail,overDataTail=0;
  INT16U     i;

  stopEvent[0] = flyEvent[0] = reverseEvent[0] = meterEvent[0] = overEvent[0] = 0;
  
  if (debugInfo&PRINT_BALANCE_DEBUG)
  {
    printf("�ж�������%d�������¼�\n",pn);
  }
  
  if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)
  {
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("������%d���γ����޵���������\n",pn);
    }
    
    return;    	 
  }
  
  //1.���ܱ�ͣ���¼��б�
  if (debugInfo&PRINT_BALANCE_DEBUG)
  {
    printf("��ʼ���ܱ�ͣ���ж�,ͣ����ֵ=%d��\n",meterGate.meterStopGate*15);
  }
  
  switch(pStatisRecord->meterStop[0])
  {
    case 0xee:                      //��û��ͣ���ֳ����ݣ���¼�µ�ͣ���ֳ�����
    case 0x0:
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("���ܱ�ͣ��:������%d��ͣ���ֳ�����,��¼ͣ���ֳ�����\n",pn);
    	}
    	
    	pStatisRecord->meterStop[0] = METER_STOP_NOT_RECORDED;
      
      pStatisRecord->meterStop[1] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
      pStatisRecord->meterStop[2] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
      pStatisRecord->meterStop[3] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
      pStatisRecord->meterStop[4] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
      
      pStatisRecord->meterStop[5] = statisTime.minute;
      pStatisRecord->meterStop[6] = statisTime.hour;
      pStatisRecord->meterStop[7] = statisTime.day;
      pStatisRecord->meterStop[8] = statisTime.month;
      pStatisRecord->meterStop[9] = statisTime.year;
      break;

    case METER_STOP_NOT_RECORDED:  //��ͣ����δ��¼������Ƿ�Ӧ�ü�¼ͣ��
 	     if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET] == pStatisRecord->meterStop[1]
 	    	 && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1] == pStatisRecord->meterStop[2]
 	    	   && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] == pStatisRecord->meterStop[3]
 	    	     && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == pStatisRecord->meterStop[4])
 	     {
    	   //ȡ���洢��ͣ����ʼʱ�䣬���ϴγ���ʱ��Ƚ�
         tmpCopyTime = timeBcdToHex(statisTime);
         tmpCopyTime.second = 0;
       	
         tmpStopTime.second = 0;
       	 tmpStopTime.minute = ((pStatisRecord->meterStop[5]>>4)&0xF)*10 + (pStatisRecord->meterStop[5]&0x0F);
       	 tmpStopTime.hour   = ((pStatisRecord->meterStop[6]>>4)&0xF)*10 + (pStatisRecord->meterStop[6]&0x0F);
       	 tmpStopTime.day    = ((pStatisRecord->meterStop[7]>>4)&0xF)*10 + (pStatisRecord->meterStop[7]&0x0F);
       	 tmpStopTime.month  = ((pStatisRecord->meterStop[8]>>4)&0xF)*10 + (pStatisRecord->meterStop[8]&0x0F);
       	 tmpStopTime.year   = ((pStatisRecord->meterStop[9]>>4)&0xF)*10 + (pStatisRecord->meterStop[9]&0x0F);
 	    	
 	    	 //ͣ�ߵ�����ֵʱ��
 	       if (timeCompare(tmpStopTime, tmpCopyTime, meterGate.meterStopGate*15) == FALSE)
 	       {
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
    	        printf("���ܱ�ͣ��:������ֵ,��¼�����¼�\n");
    	      }
    	      
    	      //ͣ���Ѽ�¼
            pStatisRecord->meterStop[0] = METER_STOP_RECORDED;
             
            //��¼�����¼�
         	  stopEvent[0] = 0x1E;   //ERC30
           
            stopEvent[2] = 0;      //����ֽ�
            stopEvent[3] = pStatisRecord->meterStop[5]; //ͣ��ʱ�� ��
            stopEvent[4] = pStatisRecord->meterStop[6]; //ͣ��ʱ�� ʱ
            stopEvent[5] = pStatisRecord->meterStop[7]; //ͣ��ʱ�� ��
            stopEvent[6] = pStatisRecord->meterStop[8]; //ͣ��ʱ�� ��
  	        stopEvent[7] = pStatisRecord->meterStop[9]; //ͣ��ʱ�� ��
  	        
  	        stopDataTail = 8;
  	        stopEvent[stopDataTail++] = pn&0xff;
  	        stopEvent[stopDataTail++] = (pn>>8&0xf) | 0x80; //ͣ�߷���
  	       
  	        //ͣ��ʾֵ
  	        if (pStatisRecord->meterStop[1]==0xee)
  	        {
  	          stopEvent[stopDataTail++] = 0xee;
  	        }
  	        else
  	        {
  	          stopEvent[stopDataTail++] = 0x0;
  	        }
  	        stopEvent[stopDataTail++] = pStatisRecord->meterStop[1];
    	      stopEvent[stopDataTail++] = pStatisRecord->meterStop[2];
    	      stopEvent[stopDataTail++] = pStatisRecord->meterStop[3];
    	      stopEvent[stopDataTail++] = pStatisRecord->meterStop[4];
    	      
    	      stopEvent[stopDataTail++] = meterGate.meterStopGate;    //��ֵ
            
            stopEvent[1] = stopDataTail;   //�洢����
         }
         else //ͣ����δ������ֵʱ��
         {
    	     if (debugInfo&PRINT_BALANCE_DEBUG)
    	     {
    	       printf("���ܱ�ͣ��:������%d��������δ������ֵ\n",pn);
    	     }

    	     pStatisRecord->meterStop[0] = METER_STOP_NOT_RECORDED;
         }
       }
       else  //δ����ͣ��
       {
    	   if (debugInfo&PRINT_BALANCE_DEBUG)
    	   {
    	     printf("���ܱ�ͣ��:������%dδ����ͣ��\n",pn);
    	   }
    	     
    	   pStatisRecord->meterStop[0] = METER_STOP_NOT_RECORDED;
       	
         pStatisRecord->meterStop[1] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
         pStatisRecord->meterStop[2] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
         pStatisRecord->meterStop[3] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
         pStatisRecord->meterStop[4] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
         
         pStatisRecord->meterStop[5] = statisTime.minute;
         pStatisRecord->meterStop[6] = statisTime.hour;
         pStatisRecord->meterStop[7] = statisTime.day;
         pStatisRecord->meterStop[8] = statisTime.month;
         pStatisRecord->meterStop[9] = statisTime.year;
       }
       break;
  
    case METER_STOP_RECORDED:       //��ͣ���Ѿ���¼����ʾֵ��ͬ����¼�µ�ͣ���ֳ�
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("���ܱ�ͣ��:������%dͣ���Ѽ�¼\n",pn);
    	}
    	
    	if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]!=pStatisRecord->meterStop[1]
	    	|| pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1]!=pStatisRecord->meterStop[2]
	    	  || pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2]!=pStatisRecord->meterStop[3]
	    	    || pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3]!=pStatisRecord->meterStop[4])
	    {
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
    	    printf("���ܱ�ͣ��:������%dͣ�߻ָ�,��¼�ָ��¼�\n",pn);
    	  }
    	  
        //��¼�ָ��¼�
        stopEvent[0] = 0x1E;   //ERC30
          
        stopEvent[2] = 0;      //����ֽ�
        stopEvent[3] = pStatisRecord->meterStop[5]; //ͣ��ʱ�� ��
        stopEvent[4] = pStatisRecord->meterStop[6]; //ͣ��ʱ�� ʱ
        stopEvent[5] = pStatisRecord->meterStop[7]; //ͣ��ʱ�� ��
        stopEvent[6] = pStatisRecord->meterStop[8]; //ͣ��ʱ�� ��
 	      stopEvent[7] = pStatisRecord->meterStop[9]; //ͣ��ʱ�� ��
 	        
 	      stopDataTail = 8;
 	      stopEvent[stopDataTail++] = pn&0xff;
 	      stopEvent[stopDataTail++] = (pn>>8&0xf);    //ͣ�߻ָ�
 	       
 	      //ͣ��ʾֵ
 	      if (pStatisRecord->meterStop[1]==0xee)
 	      {
 	        stopEvent[stopDataTail++] = 0xee;
 	      }
 	      else
 	      {
 	        stopEvent[stopDataTail++] = 0x0;
 	      }
 	      stopEvent[stopDataTail++] = pStatisRecord->meterStop[1];
   	    stopEvent[stopDataTail++] = pStatisRecord->meterStop[2];
   	    stopEvent[stopDataTail++] = pStatisRecord->meterStop[3];
   	    stopEvent[stopDataTail++] = pStatisRecord->meterStop[4];
   	      
   	    stopEvent[stopDataTail++] = meterGate.meterStopGate;    //��ֵ
        
        stopEvent[1] = stopDataTail;   //�洢����
    	            	         	  
    	  pStatisRecord->meterStop[0] = METER_STOP_NOT_RECORDED;
      
        pStatisRecord->meterStop[1] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
        pStatisRecord->meterStop[2] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
        pStatisRecord->meterStop[3] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
        pStatisRecord->meterStop[4] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
      
        pStatisRecord->meterStop[5] = statisTime.minute;
        pStatisRecord->meterStop[6] = statisTime.hour;
        pStatisRecord->meterStop[7] = statisTime.day;
        pStatisRecord->meterStop[8] = statisTime.month;
        pStatisRecord->meterStop[9] = statisTime.year;
      }
      break;
  }
  
  //2.���ܱ�ʾ���½������ܱ�����¼��б�
  readTime = statisTime;
  if (readMeterData(lastLastCopyEnergy, pn, LAST_LAST_REAL_DATA, ENERGY_DATA, &readTime, statisInterval) == TRUE)
  {
  	if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET] != 0xEE)
  	{
      //���ܱ�ʾ���½�
  	  if ((pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3])
  		  || (pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2])
  		   || (pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1])
  		    || (pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET]))
      {
        //��¼�¼�
        if ((eventRecordConfig.iEvent[3] & 0x04) || (eventRecordConfig.nEvent[3] & 0x04))
   	    {
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("����ʾ���½�:������%d�ѷ���\n",pn);
   	      }

   	      reverseEvent[0] = 0x1B;    //ERC27
     	    
     	    reverseEvent[2] = 0;   //����ֽ�
          reverseEvent[3] = statisTime.minute;
          reverseEvent[4] = statisTime.hour;
          reverseEvent[5] = statisTime.day;
          reverseEvent[6] = statisTime.month;
     	    reverseEvent[7] = statisTime.year;
          
          reverseDataTail = 8;

  	      reverseEvent[reverseDataTail++] = pn&0xff;
  	      reverseEvent[reverseDataTail++] = (pn>>8&0xf) | 0x80; //ʾ���½�����
   	     
   	      //ly,2011-06-25,��¼�½�ǰʾֵ 
     	    if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)
     	    {
     	      reverseEvent[reverseDataTail++] = 0xee;
     	      pStatisRecord->reverseVision[0] = 0xee;
     	    }
     	    else
     	    {
     	      reverseEvent[reverseDataTail++] = 0x0;
     	      pStatisRecord->reverseVision[0] = 0x0;
     	    }
   	      reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[1] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[2] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[3] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[4] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
     	    
     	    if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)
     	    {
     	      reverseEvent[reverseDataTail++] = 0xee;
     	    }
     	    else
     	    {
     	      reverseEvent[reverseDataTail++] = 0x0;
     	    }
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];

   	      reverseEvent[1] = reverseDataTail;    //�洢����
   	    }
   	    pStatisRecord->reverseFlag = 0x1;     //�½��ѷ���
      }
      else
      {
      	//����½��ѷ���,��¼�ָ��¼�
      	if (pStatisRecord->reverseFlag == 0x1)
      	{
   	      pStatisRecord->reverseFlag = 0x0;     //�½��ѻָ�

   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("����ʾ���½�:������%d�ѻָ�\n",pn);
   	      }
   	      
   	      reverseEvent[0] = 0x1B;    //ERC27
     	    
     	    reverseEvent[2] = 0;   //����ֽ�
          reverseEvent[3] = statisTime.minute;
          reverseEvent[4] = statisTime.hour;
          reverseEvent[5] = statisTime.day;
          reverseEvent[6] = statisTime.month;
     	    reverseEvent[7] = statisTime.year;
          
          reverseDataTail = 8;

  	      reverseEvent[reverseDataTail++] = pn&0xff;
  	      reverseEvent[reverseDataTail++] = (pn>>8&0xf);    //ʾ���½��ָ�

     	    //ly,�ָ�ʱ,�½�ǰʾֵ=����ʱ���½�ǰʾֵ
     	    //if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)
     	    //{
     	    //  reverseEvent[reverseDataTail++] = 0xee;
     	    //}
     	    //else
     	    //{
     	    //  reverseEvent[reverseDataTail++] = 0x0;
     	    //}
   	      //reverseEvent[reverseDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
     	    //reverseEvent[reverseDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
     	    //reverseEvent[reverseDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
     	    //reverseEvent[reverseDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
     	    
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[0];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[1];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[2];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[3];
     	    reverseEvent[reverseDataTail++] = pStatisRecord->reverseVision[4];
     	    
     	    if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)
     	    {
     	      reverseEvent[reverseDataTail++] = 0xee;
     	    }
     	    else
     	    {
     	      reverseEvent[reverseDataTail++] = 0x0;
     	    }
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
     	    reverseEvent[reverseDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];     	       
   	      reverseEvent[1] = reverseDataTail;    //�洢����
      	}
      }
      
      //3.����
      //���ܱ����(���γ����ʾֵ�Ĳ�/���ݹ����������ʵ��ʾֵ��ֵ>=������ֵ���Ƿ���)
     	tmpPresHex = bcdToHex(pCopyEnergyBuff[POSITIVE_WORK_OFFSET]|(pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1]<<8)|(pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2]<<16)|(pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3]<<24));
     	//tmpLastHex = bcdToHex(powerParaData[POSITIVE_WORK_OFFSET]|(powerParaData[POSITIVE_WORK_OFFSET+1]<<8)|(powerParaData[POSITIVE_WORK_OFFSET+2]<<16)|(powerParaData[POSITIVE_WORK_OFFSET+3]<<24));
     	tmpLastHex = bcdToHex(lastLastCopyEnergy[POSITIVE_WORK_OFFSET]|(lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1]<<8)|(lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2]<<16)|(lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3]<<24));
     	
     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("��ʼ�жϷ���\n");
     	}
     	
     	if (tmpLastHex > tmpPresHex)
     	{
     	  goto recordEvent;
     	}
     	
     	tmpMeterData = tmpPresHex - tmpLastHex; 
     	
     	tmpCountData = bcdToHex(pCopyParaBuff[POWER_INSTANT_WORK]|(pCopyParaBuff[POWER_INSTANT_WORK+1]<<8)|(pCopyParaBuff[POWER_INSTANT_WORK+2]<<16));
     	tmpCountData = tmpCountData*statisInterval/60;

     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("�жϷ���:���γ�����ʾֵ=%d,�ϴγ�����ܱ�ʾֵ=%d\n",tmpPresHex,tmpLastHex);
     	  printf("�жϷ���:���γ����ֵ=%d,����=%d\n",tmpMeterData,tmpCountData);       	 
     	}
     	
     	if (tmpCountData == 0)
     	{
     	  goto recordEvent;
     	}
     	
     	tmpGateHex = meterGate.meterFlyGate;
     	tmpGateHex = bcdToHex(tmpGateHex);
     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("�жϷ���:������ֵ=%d\n",tmpGateHex);
     	}

     	//ÿ�����г��Է������õ�ʵ�ʷ�����ֵ
     	if (tmpMeterData*1000/tmpCountData >= tmpGateHex)
     	{
      	if (0x0==pStatisRecord->flyFlag)    //2013-11-22,�������ж�
      	{
     	    pStatisRecord->flyFlag = 0x1;     //�����ѷ���
     	    
     	    if (debugInfo&PRINT_BALANCE_DEBUG)
     	    {
     	      printf("���ܱ����:������%d����\n",pn);
     	    }
  
       	  if ((eventRecordConfig.iEvent[3] & 0x10) || (eventRecordConfig.nEvent[3] & 0x10))
       	  {
     	      flyEvent[0] = 0x1D;    //ERC29
       	    
       	    flyEvent[2] = 0;       //����ֽ�
            flyEvent[3] = statisTime.minute;
            flyEvent[4] = statisTime.hour;
            flyEvent[5] = statisTime.day;
            flyEvent[6] = statisTime.month;
       	    flyEvent[7] = statisTime.year;
  
            flyDataTail = 8;
  
    	      flyEvent[flyDataTail++] = pn&0xff;
    	      flyEvent[flyDataTail++] = (pn>>8&0xf) | 0x80; //���߷���
       	    
       	    //���߷���ǰ�����й�ʾֵ ly,2011-06-25,��¼���߷���ǰʾֵ
            if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)
            {
       	      flyEvent[flyDataTail++] = 0xee;   //����ֽ�
       	      pStatisRecord->flyVision[0] = 0xee;
            }
            else
            {
       	      flyEvent[flyDataTail++] = 0;   //����ֽ�
       	      pStatisRecord->flyVision[0] = 0x0;
       	    }
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[1] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
       	    flyEvent[flyDataTail++] = pStatisRecord->flyVision[2] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
       	    flyEvent[flyDataTail++] = pStatisRecord->flyVision[3] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
       	    flyEvent[flyDataTail++] = pStatisRecord->flyVision[4] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
       	    
       	    //���߷����������й�ʾֵ
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
       	      flyEvent[flyDataTail++] = 0xee;   //����ֽ�
            }
            else
            {
       	      flyEvent[flyDataTail++] = 0;   //����ֽ�
       	    }
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
     	      
     	      flyEvent[flyDataTail++] = meterGate.meterFlyGate;
     	      flyEvent[1] = flyDataTail;
       	  } 
       	}   	    
   	  }
      else
      {
      	//��������ѷ���,��¼�ָ��¼�
      	if (pStatisRecord->flyFlag == 0x1)
      	{
   	      pStatisRecord->flyFlag = 0x0;     //�����ѻָ�
   	      
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("���ܱ����:������%d�ѻָ�\n",pn);
   	      }
   	      
       	  if ((eventRecordConfig.iEvent[3] & 0x10) || (eventRecordConfig.nEvent[3] & 0x10))
       	  {
     	      flyEvent[0] = 0x1D;    //ERC29
       	    
       	    flyEvent[2] = 0;       //����ֽ�
            flyEvent[3] = statisTime.minute;
            flyEvent[4] = statisTime.hour;
            flyEvent[5] = statisTime.day;
            flyEvent[6] = statisTime.month;
       	    flyEvent[7] = statisTime.year;

            flyDataTail = 8;
    	      flyEvent[flyDataTail++] = pn&0xff;
    	      flyEvent[flyDataTail++] = (pn>>8&0xff); //���߻ָ�
       	    
       	    //���߷���ǰ�����й�ʾֵ
       	    //ly,2011-06-25,����ǰʾֵ�ĳ��뷢��ʱһ���ķ���ǰʾֵ
            //if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)            
            //{
     	      //  flyEvent[flyDataTail++] = 0xee;   //����ֽ�
            //}
            //else
            //{
     	      //  flyEvent[flyDataTail++] = 0;   //����ֽ�
     	      //}
   	        //flyEvent[flyDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
     	      //flyEvent[flyDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
     	      //flyEvent[flyDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
     	      //flyEvent[flyDataTail++] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
     	      
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[0];
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[1];
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[2];
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[3];
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[4];
     	      
     	      //���߷����������й�ʾֵ
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
     	        flyEvent[flyDataTail++] = 0xee;   //����ֽ�
            }
            else
            {
     	        flyEvent[flyDataTail++] = 0;   //����ֽ�
     	      }
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
       	    flyEvent[flyDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
     	      
     	      flyEvent[flyDataTail++] = meterGate.meterFlyGate;
   	        
   	        flyEvent[1] = flyDataTail;
       	  }
   	    }
   	  }
   	  
   	  
   	  
   	  
   	  //��ʼ�жϵ���������
      tmpOverGateHex = bcdToHex(meterGate.powerOverGate);
     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("�жϵ���������:\n");
     	  printf("    ������ֵ=%d\r\n", tmpOverGateHex);
     	  printf("    ���γ�����ʾֵ=%d,�ϴγ�����ܱ�ʾֵ=%d,���γ����ֵ=%d\n", tmpPresHex, tmpLastHex, tmpMeterData);
     	  printf("    ���ݹ����������ʵ��ʾֵ��ֵ=%d\n", tmpCountData);
     	}

     	//ÿ�����г��Է������õ�ʵ�ʳ�����ֵ
     	if (((tmpMeterData*1000/tmpCountData)<tmpGateHex) && ((tmpMeterData*1000/tmpCountData)>tmpOverGateHex))    //����
     	{
   	    if (0x0==pStatisRecord->overFlag)
   	    {
     	    pStatisRecord->overFlag = 0x01;    //�����ѷ���
     	    
     	    if (debugInfo&PRINT_BALANCE_DEBUG)
     	    {
     	      printf("����������:������%d����\r\n", pn);
     	    }
  
       	  if ((eventRecordConfig.iEvent[3] & 0x08) || (eventRecordConfig.nEvent[3] & 0x08))
       	  {
     	      overEvent[0] = 28;    //ERC28
       	    
       	    overEvent[2] = 0;       //����ֽ�
            overEvent[3] = statisTime.minute;
            overEvent[4] = statisTime.hour;
            overEvent[5] = statisTime.day;
            overEvent[6] = statisTime.month;
       	    overEvent[7] = statisTime.year;
  
            overDataTail = 8;
  
    	      overEvent[overDataTail++] = pn&0xff;
    	      overEvent[overDataTail++] = (pn>>8&0xf) | 0x80; //�����
       	    
       	    //�����ǰ�����й�ʾֵ
            if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)
            {
       	      overEvent[overDataTail++] = 0xee;   //����ֽ�
       	      pStatisRecord->overVision[0] = 0xee;
            }
            else
            {
       	      overEvent[overDataTail++] = 0;   //����ֽ�
       	      pStatisRecord->overVision[0] = 0x0;
       	    }
     	      overEvent[overDataTail++] = pStatisRecord->overVision[1] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
       	    overEvent[overDataTail++] = pStatisRecord->overVision[2] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
       	    overEvent[overDataTail++] = pStatisRecord->overVision[3] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
       	    overEvent[overDataTail++] = pStatisRecord->overVision[4] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
       	    
       	    //���߷����������й�ʾֵ
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
       	      overEvent[overDataTail++] = 0xee;   //����ֽ�
            }
            else
            {
       	      overEvent[overDataTail++] = 0;   //����ֽ�
       	    }
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
     	      
     	      overEvent[overDataTail++] = meterGate.powerOverGate;
     	      overEvent[1] = overDataTail;
       	  }
       	}
   	  }
      else
      {
      	//��������ѷ���,��¼�ָ��¼�
      	if (0x01==pStatisRecord->overFlag)
      	{
   	      pStatisRecord->overFlag = 0x00;     //�����ѻָ�
   	      
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("����������:������%d�ѻָ�\r\n",pn);
   	      }
   	      
       	  if ((eventRecordConfig.iEvent[3] & 0x08) || (eventRecordConfig.nEvent[3] & 0x08))
       	  {
     	      overEvent[0] = 28;    //ERC28
       	    
       	    overEvent[2] = 0;       //����ֽ�
            overEvent[3] = statisTime.minute;
            overEvent[4] = statisTime.hour;
            overEvent[5] = statisTime.day;
            overEvent[6] = statisTime.month;
       	    overEvent[7] = statisTime.year;

            overDataTail = 8;
    	      overEvent[overDataTail++] = pn&0xff;
    	      overEvent[overDataTail++] = (pn>>8&0xff); //����ָ�
       	    
       	    //�����ǰ�����й�ʾֵ
     	      overEvent[overDataTail++] = pStatisRecord->overVision[0];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[1];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[2];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[3];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[4];
     	      
     	      //������������й�ʾֵ
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
     	        overEvent[overDataTail++] = 0xee;   //����ֽ�
            }
            else
            {
     	        overEvent[overDataTail++] = 0;   //����ֽ�
     	      }
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
       	    overEvent[overDataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
     	      
     	      overEvent[overDataTail++] = meterGate.powerOverGate;
   	        
   	        overEvent[1] = overDataTail;
       	  }
   	    }
   	  }     	  
   	}
  }
  
recordEvent:    
  //��ͣ���¼���������¼ͣ���¼�
  if (stopEvent[0] != 0x00)
  {
    if (eventRecordConfig.iEvent[3] & 0x20)
    {
      writeEvent(stopEvent, stopDataTail, 1, DATA_FROM_GPRS);
      eventStatus[3] = eventStatus[3] | 0x20;
    }

    if (eventRecordConfig.nEvent[3] & 0x20)
    {
      writeEvent(stopEvent, stopDataTail, 2, DATA_FROM_GPRS);
      eventStatus[3] = eventStatus[3] | 0x20;
    }
  }
  
  //��ʾ���½��¼���������¼�½��¼�
  if (reverseEvent[0] != 0x00)
  {
    if (eventRecordConfig.iEvent[3] & 0x04)
    {
      writeEvent(reverseEvent, reverseDataTail, 1, DATA_FROM_GPRS);
      eventStatus[3] = eventStatus[3] | 0x04;
    }

    if (eventRecordConfig.nEvent[3] & 0x04)
    {
      writeEvent(reverseEvent, reverseDataTail, 2, DATA_FROM_GPRS);
      eventStatus[3] = eventStatus[3] | 0x04;
    }
  }
  
  //�������¼���������¼�����¼�
  if (overEvent[0] != 0x00)
  {
    if (eventRecordConfig.iEvent[3] & 0x08)
    {
      writeEvent(overEvent, overDataTail++, 1, DATA_FROM_GPRS);
      
      eventStatus[3] = eventStatus[3] | 0x08;
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
   	  {
   	    printf("���������%d���ܱ������������Ҫ�¼�\r\n",pn);
   	  }
    }
 	  
 	  if (eventRecordConfig.nEvent[3] & 0x08)
    {
      writeEvent(overEvent, overDataTail++, 2, DATA_FROM_LOCAL);
      eventStatus[3] = eventStatus[3] | 0x08;
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
 	    {
 	      printf("���������%d���ܱ����������һ���¼�\r\n",pn);
 	    }
    }
  }
  
  //�������¼���������¼�����¼�
  if (flyEvent[0] != 0x00)
  {
    if (eventRecordConfig.iEvent[3] & 0x10)
    {
      writeEvent(flyEvent, flyDataTail++, 1, DATA_FROM_GPRS);
      eventStatus[3] = eventStatus[3] | 0x10;
    }
    else
    {
   	  if (eventRecordConfig.nEvent[3] & 0x10)
      {
        writeEvent(flyEvent, flyDataTail++, 2, DATA_FROM_LOCAL);
        eventStatus[3] = eventStatus[3] | 0x10;
      }
    }
  }
}

/*******************************************************
��������: realStatisticPerPoint
��������: ͳ�Ʋ����㱾�µ���ǰΪֹ�Ĳα���ͳ��ֵ
          ��ȡǰһ�ղα���ͳ��ֵ���뵱�ղα���ͳ���ý��бȽ�
���ú���:     
�����ú���:
�������:
�������:  
����ֵ�� 
*******************************************************/
void realStatisticPerPoint(INT16U pn, INT8U *pBalanceParaBuff, DATE_TIME statisTime)
{
    DATE_TIME tmpTime, readTime, backupTime;
	  INT32U    tmpData;
	  INT16U    tmpDataShort;
	  INT8U     tmpBuff[LEN_OF_PARA_BALANCE_RECORD];
	  BOOL      bufHasData=FALSE;
	  INT8U     i;

    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
    	printf("realStatisticPerPoint:��ʼͳ�Ʊ��²α���,ͳ��ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n",statisTime.year,statisTime.month,statisTime.day,statisTime.hour,statisTime.minute,statisTime.second);
    }

	  //��ȡ���γ���ǰһ�յ��¶���α�������
	  tmpTime = timeBcdToHex(statisTime);
    tmpTime = backTime(tmpTime, 0, 1, 0, 0, 0);
    
    //2012-09-11,�޸��������,Ӧ�ò���ǰһ�մ���ĩ��ʼ��ͳ������
    tmpTime.hour   = 23;
    tmpTime.minute = 59;
    tmpTime.second = 59;
    
	  tmpTime = timeHexToBcd(tmpTime);
    
    //��ȡǰһ�ղα���ͳ��ֵ���뵱�ղα���ͳ��ֵ���бȽ�
    //  ��ǰһ��ͳ�����ݲ����ڻ�����ͳ���Ѿ�����Ӧ������ͳ�Ƶ�������,�ñ���ʵʱ���������,���ü���
    bufHasData = FALSE;
    readTime = tmpTime;
    if (readMeterData(tmpBuff, pn, DAY_BALANCE, MONTH_BALANCE_PARA_DATA, &readTime, 0) == TRUE)
    {
    	bufHasData = TRUE;
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    		 printf("realStatisticPerPoint:��ǰһ�ղα���ͳ��ֵ,ʱ��Ϊ%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
    	}
    }
    else
    {
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    		 printf("realStatisticPerPoint:��ǰһ�ղα���ͳ��ֵ\n");
    	}
    }
    
    if (bufHasData==FALSE)
    {
      //2012-09-11,�޸��������,
      //backupTime = nextTime(sysTime, 60, 0);
      //for(i=sysTime.day; i>0; i--)
      
      //2012-09-11,����ͳ��ʱ���ǰһ�쵽���µ�һ�յ����һ�̲��Ҹ�������һ��ʵʱ��������
      //    �ҵ������һ��ʵʱ���������˳�
      backupTime = timeBcdToHex(statisTime);
      backupTime.hour   = 23;
      backupTime.minute = 59;
      backupTime.second = 59;
      for(i=bcdToHex(statisTime.day)-1; i>0; i--)
      {
        backupTime.day = i;
        readTime = timeHexToBcd(backupTime);
        
        if (readMeterData(tmpBuff, pn, LAST_REAL_BALANCE, REAL_BALANCE_PARA_DATA, &readTime, 0) == TRUE)
        {
    	    bufHasData = TRUE;
    	    
    	    break;  //2012-09-11,���
    	  }
    	  
    	  usleep(10);
    	}
    	
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    		 if (bufHasData==TRUE)
    		 {
    		    printf("realStatisticPerPoint:�����һ�βα���ͳ��ֵ,ʱ��Ϊ:%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
    		 }
    		 else
    		 {
    		    printf("realStatisticPerPoint:�����һ�βα���ͳ��ֵ\n");
    	   }
    	}
    }
    
    //����������ݲ��Ҷ�����ʱ����ͳ��ʱ�䲻ͬ���Ҳ���һ���µ�ͳ��
    //if (bufHasData==TRUE && tmpBuff[NEXT_NEW_INSTANCE] != 0x01 
    //	  && (!(statisTime.year==readTime.year && statisTime.month==readTime.month && statisTime.day==readTime.day && statisTime.hour==readTime.hour && statisTime.minute==readTime.minute && statisTime.second==readTime.second))
    //2012-09-12,�޸�����ж�
    if (bufHasData==TRUE)
    {
    	printf("realStatisticPerPoint:������%d�����α���ͳ������\n", pn);
    	
    	//1.����ͳ��***********************************************************
    	//1.1 ���й�����ͳ��
    	if (tmpBuff[MAX_TOTAL_POWER] != 0xEE)
    	{
    	  //���й��������ֵͳ��,ǰһ����ͳ��ֵ,����û��ͳ��ֵ������ͳ��ֵ����Ϊ�����ͳ�ƽ��
    	  if (pBalanceParaBuff[MAX_TOTAL_POWER] == 0xEE)
    	  {
    	  	//�����й��������ֵ
    	    pBalanceParaBuff[MAX_TOTAL_POWER] = tmpBuff[MAX_TOTAL_POWER];
    	    pBalanceParaBuff[MAX_TOTAL_POWER+1] = tmpBuff[MAX_TOTAL_POWER+1];
    	    pBalanceParaBuff[MAX_TOTAL_POWER+2] = tmpBuff[MAX_TOTAL_POWER+2];
    	    
    	    //�����й��������ֵʱ��
    	    pBalanceParaBuff[MAX_TOTAL_POWER_TIME] = tmpBuff[MAX_TOTAL_POWER_TIME];
    	    pBalanceParaBuff[MAX_TOTAL_POWER_TIME+1] = tmpBuff[MAX_TOTAL_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_TOTAL_POWER_TIME+2] = tmpBuff[MAX_TOTAL_POWER_TIME+2];
      	}
      	else  //ǰһ��͵�ǰ����ͳ��ֵ���ȽϺ�õ����
      	{
    	    //ǰһ�����ֵ���ڵ������ֵ�������ֵȡǰһ�����ֵ������ȡ�������ֵ
    	    if ((tmpBuff[MAX_TOTAL_POWER+2] > pBalanceParaBuff[MAX_TOTAL_POWER+2])
    	    	|| ((tmpBuff[MAX_TOTAL_POWER+2] == pBalanceParaBuff[MAX_TOTAL_POWER+2])&&(tmpBuff[MAX_TOTAL_POWER+1] > pBalanceParaBuff[MAX_TOTAL_POWER+1]))
    	    	  || ((tmpBuff[MAX_TOTAL_POWER+2] == pBalanceParaBuff[MAX_TOTAL_POWER+2])&&(tmpBuff[MAX_TOTAL_POWER+1] == pBalanceParaBuff[MAX_TOTAL_POWER+1])&&(tmpBuff[MAX_TOTAL_POWER] > pBalanceParaBuff[MAX_TOTAL_POWER])))
    	    {
    	      //�����й��������ֵ
    	      pBalanceParaBuff[MAX_TOTAL_POWER]   = tmpBuff[MAX_TOTAL_POWER];
    	      pBalanceParaBuff[MAX_TOTAL_POWER+1] = tmpBuff[MAX_TOTAL_POWER+1];
    	      pBalanceParaBuff[MAX_TOTAL_POWER+2] = tmpBuff[MAX_TOTAL_POWER+2];
    	      
    	      //�����й��������ֵʱ��
    	      pBalanceParaBuff[MAX_TOTAL_POWER_TIME]   = tmpBuff[MAX_TOTAL_POWER_TIME];
    	      pBalanceParaBuff[MAX_TOTAL_POWER_TIME+1] = tmpBuff[MAX_TOTAL_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_TOTAL_POWER_TIME+2] = tmpBuff[MAX_TOTAL_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //�����й�����Ϊ��ʱ��
    	  //�������Ĺ���Ϊ��ʱ�䲻�����㣬�򵽵���Ϊֹ���µ��й�����Ϊ��ʱ��Ϊǰһ��ͳ��ֵ�뵱��ͳ��ֵ֮��
    	  //�������ǰһ��ͳ��ֵ
    	  if (pBalanceParaBuff[TOTAL_ZERO_POWER_TIME] != 0x00
    	  	&& pBalanceParaBuff[TOTAL_ZERO_POWER_TIME+1] != 0x00)
    	  {
    	    if (tmpBuff[TOTAL_ZERO_POWER_TIME] != 0x00 
    	    	&& tmpBuff[TOTAL_ZERO_POWER_TIME+1] != 0x00)
    	    {
    	      tmpDataShort = tmpBuff[TOTAL_ZERO_POWER_TIME] | tmpBuff[TOTAL_ZERO_POWER_TIME+1]<<8;
    	      tmpDataShort += pBalanceParaBuff[TOTAL_ZERO_POWER_TIME] | pBalanceParaBuff[TOTAL_ZERO_POWER_TIME+1]<<8;
    	    
    	      pBalanceParaBuff[TOTAL_ZERO_POWER_TIME] = tmpDataShort&0xFF;
    	      pBalanceParaBuff[TOTAL_ZERO_POWER_TIME+1] = tmpDataShort>>8&0xFF;
    	    }
    	  }
    	  else
    	  {
    	    pBalanceParaBuff[TOTAL_ZERO_POWER_TIME] = tmpBuff[TOTAL_ZERO_POWER_TIME];
    	    pBalanceParaBuff[TOTAL_ZERO_POWER_TIME+1] = tmpBuff[TOTAL_ZERO_POWER_TIME+1];
    	  }
    	}
    	
    	//1.2 A���й�����ͳ��
    	if (tmpBuff[MAX_A_POWER] != 0xEE)
    	{
    	  //A�๦�������ֵͳ��,ǰһ����ͳ��ֵ,����û��ͳ��ֵ������ͳ��ֵ����Ϊ�����ͳ�ƽ��
    	  if (pBalanceParaBuff[MAX_A_POWER] == 0xEE)
    	  {
    	  	//��A���й��������ֵ
    	    pBalanceParaBuff[MAX_A_POWER] = tmpBuff[MAX_A_POWER];
    	    pBalanceParaBuff[MAX_A_POWER+1] = tmpBuff[MAX_A_POWER+1];
    	    pBalanceParaBuff[MAX_A_POWER+2] = tmpBuff[MAX_A_POWER+2];
    	    
    	    //��A���й��������ֵʱ��
    	    pBalanceParaBuff[MAX_A_POWER] = tmpBuff[MAX_A_POWER_TIME];
    	    pBalanceParaBuff[MAX_A_POWER+1] = tmpBuff[MAX_A_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_A_POWER+2] = tmpBuff[MAX_A_POWER_TIME+2];
      	}
      	else  //ǰһ��͵�ǰ����ͳ��ֵ���ȽϺ�õ����
      	{
    	    if ((tmpBuff[MAX_A_POWER+2] > pBalanceParaBuff[MAX_A_POWER+2])
    	    	|| ((tmpBuff[MAX_A_POWER+2] == pBalanceParaBuff[MAX_A_POWER+2])&&(tmpBuff[MAX_A_POWER+1] > pBalanceParaBuff[MAX_A_POWER+1]))
    	    	  || ((tmpBuff[MAX_A_POWER+2] == pBalanceParaBuff[MAX_A_POWER+2])&&(tmpBuff[MAX_A_POWER+1] == pBalanceParaBuff[MAX_A_POWER+1])&&(tmpBuff[MAX_A_POWER] > pBalanceParaBuff[MAX_A_POWER])))
    	    {
    	      //��A���й��������ֵ
    	      pBalanceParaBuff[MAX_A_POWER] = tmpBuff[MAX_A_POWER];
    	      pBalanceParaBuff[MAX_A_POWER+1] = tmpBuff[MAX_A_POWER+1];
    	      pBalanceParaBuff[MAX_A_POWER+2] = tmpBuff[MAX_A_POWER+2];
    	      
    	      //��A���й��������ֵʱ��
    	      pBalanceParaBuff[MAX_A_POWER_TIME] = tmpBuff[MAX_A_POWER_TIME];
    	      pBalanceParaBuff[MAX_A_POWER_TIME+1] = tmpBuff[MAX_A_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_A_POWER_TIME+2] = tmpBuff[MAX_A_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //��A���й�����Ϊ��ʱ��
    	  //�������Ĺ���Ϊ��ʱ�䲻�����㣬�򵽵���Ϊֹ���µ��й�����Ϊ��ʱ��Ϊǰһ��ͳ��ֵ�뵱��ͳ��ֵ֮��
    	  //�������ǰһ��ͳ��ֵ
    	  if (pBalanceParaBuff[A_ZERO_POWER_TIME] != 0x00
    	  	&& pBalanceParaBuff[A_ZERO_POWER_TIME+1] != 0x00)
    	  {
    	    if (tmpBuff[A_ZERO_POWER_TIME] != 0x00
    	    	&& tmpBuff[A_ZERO_POWER_TIME+1] != 0x00)
    	    {
    	      tmpDataShort = tmpBuff[A_ZERO_POWER_TIME] | tmpBuff[A_ZERO_POWER_TIME+1]<<8;
    	      tmpDataShort += pBalanceParaBuff[A_ZERO_POWER_TIME] | pBalanceParaBuff[A_ZERO_POWER_TIME+1]<<8;
    	    
    	      pBalanceParaBuff[A_ZERO_POWER_TIME] = tmpDataShort&0xFF;
    	      pBalanceParaBuff[A_ZERO_POWER_TIME+1] = tmpDataShort>>8&0xFF;
    	    }
    	  }
    	  else
    	  {
    	    pBalanceParaBuff[A_ZERO_POWER_TIME] = tmpBuff[A_ZERO_POWER_TIME];
    	    pBalanceParaBuff[A_ZERO_POWER_TIME+1] = tmpBuff[A_ZERO_POWER_TIME+1];
    	  }
    	}
    	
    	//1.3 B���й�����ͳ��
    	if (tmpBuff[MAX_B_POWER] != 0xEE)
    	{
    	  //���й��������ֵͳ��,ǰһ����ͳ��ֵ,����û��ͳ��ֵ������ͳ��ֵ����Ϊ�����ͳ�ƽ��
    	  if (pBalanceParaBuff[MAX_B_POWER] == 0xEE)
    	  {
    	  	//�����й��������ֵ
    	    pBalanceParaBuff[MAX_B_POWER] = tmpBuff[MAX_B_POWER];
    	    pBalanceParaBuff[MAX_B_POWER+1] = tmpBuff[MAX_B_POWER+1];
    	    pBalanceParaBuff[MAX_B_POWER+2] = tmpBuff[MAX_B_POWER+2];
    	    
    	    //�����й��������ֵʱ��
    	    pBalanceParaBuff[MAX_B_POWER_TIME] = tmpBuff[MAX_B_POWER_TIME];
    	    pBalanceParaBuff[MAX_B_POWER_TIME+1] = tmpBuff[MAX_B_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_B_POWER_TIME+2] = tmpBuff[MAX_B_POWER_TIME+2];
      	}
      	else  //ǰһ��͵�ǰ����ͳ��ֵ���ȽϺ�õ����
      	{
    	    if ((tmpBuff[MAX_B_POWER+2] > pBalanceParaBuff[MAX_B_POWER+2])
    	    	|| ((tmpBuff[MAX_B_POWER+2] == pBalanceParaBuff[MAX_B_POWER+2])&&(tmpBuff[MAX_B_POWER+1] > pBalanceParaBuff[MAX_TOTAL_POWER+1]))
    	    	  || ((tmpBuff[MAX_B_POWER+2] == pBalanceParaBuff[MAX_B_POWER+2])&&(tmpBuff[MAX_B_POWER+1] == pBalanceParaBuff[MAX_TOTAL_POWER+1])&&(tmpBuff[MAX_B_POWER] > pBalanceParaBuff[MAX_B_POWER])))
    	    {
    	      //�����й��������ֵ
    	      pBalanceParaBuff[MAX_B_POWER] = tmpBuff[MAX_B_POWER];
    	      pBalanceParaBuff[MAX_B_POWER+1] = tmpBuff[MAX_B_POWER+1];
    	      pBalanceParaBuff[MAX_B_POWER+2] = tmpBuff[MAX_B_POWER+2];
    	      
    	      //�����й��������ֵʱ��
    	      pBalanceParaBuff[MAX_B_POWER_TIME] = tmpBuff[MAX_B_POWER_TIME];
    	      pBalanceParaBuff[MAX_B_POWER_TIME+1] = tmpBuff[MAX_B_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_B_POWER_TIME+2] = tmpBuff[MAX_B_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //�����й�����Ϊ��ʱ��
    	  //�������Ĺ���Ϊ��ʱ�䲻�����㣬�򵽵���Ϊֹ���µ��й�����Ϊ��ʱ��Ϊǰһ��ͳ��ֵ�뵱��ͳ��ֵ֮��
    	  //�������ǰһ��ͳ��ֵ
    	  if (pBalanceParaBuff[B_ZERO_POWER_TIME] != 0x00
    	  	&& pBalanceParaBuff[B_ZERO_POWER_TIME+1] != 0x00)
    	  {
    	    if (tmpBuff[B_ZERO_POWER_TIME] != 0x00
    	    	&& tmpBuff[B_ZERO_POWER_TIME+1] != 0x00)
    	    {
    	      tmpDataShort = tmpBuff[B_ZERO_POWER_TIME] | tmpBuff[B_ZERO_POWER_TIME+1]<<8;
    	      tmpDataShort += pBalanceParaBuff[B_ZERO_POWER_TIME] | pBalanceParaBuff[B_ZERO_POWER_TIME+1]<<8;
    	    
    	      pBalanceParaBuff[B_ZERO_POWER_TIME] = tmpDataShort&0xFF;
    	      pBalanceParaBuff[B_ZERO_POWER_TIME+1] = tmpDataShort>>8&0xFF;
    	    }
    	  }
    	  else
    	  {
    	    pBalanceParaBuff[B_ZERO_POWER_TIME] = tmpBuff[B_ZERO_POWER_TIME];
    	    pBalanceParaBuff[B_ZERO_POWER_TIME+1] = tmpBuff[B_ZERO_POWER_TIME+1];
    	  }
    	}
    	
    	//1.4 C���й�����ͳ��
    	if (tmpBuff[MAX_C_POWER] != 0xEE)
    	{
    	  //���й��������ֵͳ��,ǰһ����ͳ��ֵ,����û��ͳ��ֵ������ͳ��ֵ����Ϊ�����ͳ�ƽ��
    	  if (pBalanceParaBuff[MAX_C_POWER] == 0xEE)
    	  {
    	  	//�����й��������ֵ
    	    pBalanceParaBuff[MAX_C_POWER] = tmpBuff[MAX_C_POWER];
    	    pBalanceParaBuff[MAX_C_POWER+1] = tmpBuff[MAX_C_POWER+1];
    	    pBalanceParaBuff[MAX_C_POWER+2] = tmpBuff[MAX_C_POWER+2];
    	    
    	    //�����й��������ֵʱ��
    	    pBalanceParaBuff[MAX_C_POWER_TIME] = tmpBuff[MAX_C_POWER_TIME];
    	    pBalanceParaBuff[MAX_C_POWER_TIME+1] = tmpBuff[MAX_C_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_C_POWER_TIME+2] = tmpBuff[MAX_C_POWER_TIME+2];
      	}
      	else  //ǰһ��͵�ǰ����ͳ��ֵ���ȽϺ�õ����
      	{
    	    if ((tmpBuff[MAX_C_POWER+2] > pBalanceParaBuff[MAX_C_POWER+2])
    	    	|| ((tmpBuff[MAX_C_POWER+2] == pBalanceParaBuff[MAX_C_POWER+2])&&(tmpBuff[MAX_C_POWER+1] > pBalanceParaBuff[MAX_C_POWER+1]))
    	    	  || ((tmpBuff[MAX_C_POWER+2] == pBalanceParaBuff[MAX_C_POWER+2])&&(tmpBuff[MAX_C_POWER+1] == pBalanceParaBuff[MAX_C_POWER+1])&&(tmpBuff[MAX_C_POWER] > pBalanceParaBuff[MAX_C_POWER])))
    	    {
    	      //�����й��������ֵ
    	      pBalanceParaBuff[MAX_C_POWER] = tmpBuff[MAX_C_POWER];
    	      pBalanceParaBuff[MAX_C_POWER+1] = tmpBuff[MAX_C_POWER+1];
    	      pBalanceParaBuff[MAX_C_POWER+2] = tmpBuff[MAX_C_POWER+2];
    	      
    	      //�����й��������ֵʱ��
    	      pBalanceParaBuff[MAX_C_POWER_TIME] = tmpBuff[MAX_C_POWER_TIME];
    	      pBalanceParaBuff[MAX_C_POWER_TIME+1] = tmpBuff[MAX_C_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_C_POWER_TIME+2] = tmpBuff[MAX_C_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //�����й�����Ϊ��ʱ��
    	  //�������Ĺ���Ϊ��ʱ�䲻�����㣬�򵽵���Ϊֹ���µ��й�����Ϊ��ʱ��Ϊǰһ��ͳ��ֵ�뵱��ͳ��ֵ֮��
    	  //�������ǰһ��ͳ��ֵ
    	  if (pBalanceParaBuff[C_ZERO_POWER_TIME] != 0x00
    	  	&& pBalanceParaBuff[C_ZERO_POWER_TIME+1] != 0x00)
    	  {
    	    if (tmpBuff[C_ZERO_POWER_TIME] != 0x00
    	    	&& tmpBuff[C_ZERO_POWER_TIME+1] != 0x00)
    	    {
    	      tmpDataShort = tmpBuff[C_ZERO_POWER_TIME] | tmpBuff[C_ZERO_POWER_TIME+1]<<8;
    	      tmpDataShort += pBalanceParaBuff[C_ZERO_POWER_TIME] | pBalanceParaBuff[C_ZERO_POWER_TIME+1]<<8;
    	    
    	      pBalanceParaBuff[C_ZERO_POWER_TIME] = tmpDataShort&0xFF;
    	      pBalanceParaBuff[C_ZERO_POWER_TIME+1] = tmpDataShort>>8&0xFF;
    	    }
    	  }
    	  else
    	  {
    	    pBalanceParaBuff[C_ZERO_POWER_TIME] = tmpBuff[C_ZERO_POWER_TIME];
    	    pBalanceParaBuff[C_ZERO_POWER_TIME+1] = tmpBuff[C_ZERO_POWER_TIME+1];
    	  }
    	}
    	
    	//2.��ѹͳ��***********************************************************
    	//2.1 A���ѹͳ��
    	if (tmpBuff[VOL_A_UP_UP_TIME] != 0xEE) //Խ������
      {
        if (pBalanceParaBuff[VOL_A_UP_UP_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_UP_UP_TIME]   = tmpBuff[VOL_A_UP_UP_TIME];
          pBalanceParaBuff[VOL_A_UP_UP_TIME+1] = tmpBuff[VOL_A_UP_UP_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_A_UP_UP_TIME] | tmpBuff[VOL_A_UP_UP_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_A_UP_UP_TIME]| pBalanceParaBuff[VOL_A_UP_UP_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_A_UP_UP_TIME]   = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_A_UP_UP_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_A_UP_TIME] != 0xEE) //Խ����
      {
        if (pBalanceParaBuff[VOL_A_UP_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_UP_TIME]   = tmpBuff[VOL_A_UP_TIME];
          pBalanceParaBuff[VOL_A_UP_TIME+1] = tmpBuff[VOL_A_UP_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_A_UP_TIME] | tmpBuff[VOL_A_UP_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_A_UP_TIME] | pBalanceParaBuff[VOL_A_UP_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_A_UP_TIME]   = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_A_UP_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_A_DOWN_DOWN_TIME] != 0xEE) //Խ������
      {
        if (pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME]   = tmpBuff[VOL_A_DOWN_DOWN_TIME];
          pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME+1] = tmpBuff[VOL_A_DOWN_DOWN_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_A_DOWN_DOWN_TIME] | tmpBuff[VOL_A_DOWN_DOWN_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME]   = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_A_DOWN_TIME] != 0xEE) //Խ����
      {
        if (pBalanceParaBuff[VOL_A_DOWN_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_DOWN_TIME] = tmpBuff[VOL_A_DOWN_TIME];
          pBalanceParaBuff[VOL_A_DOWN_TIME+1] = tmpBuff[VOL_A_DOWN_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_A_DOWN_TIME] | tmpBuff[VOL_A_DOWN_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_A_DOWN_TIME]| pBalanceParaBuff[VOL_A_DOWN_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_A_DOWN_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_A_DOWN_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_A_OK_TIME] != 0xEE) //�ϸ�
      {
        if (pBalanceParaBuff[VOL_A_OK_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_OK_TIME] = tmpBuff[VOL_A_OK_TIME];
          pBalanceParaBuff[VOL_A_OK_TIME+1] = tmpBuff[VOL_A_OK_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_A_OK_TIME] | tmpBuff[VOL_A_OK_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_A_OK_TIME]| pBalanceParaBuff[VOL_A_OK_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_A_OK_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_A_OK_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      //�µ�ѹ���ֵ������ʱ��
      if (tmpBuff[VOL_A_MAX]!=0xEE)
      {
      	//��ǰһ��ͳ�����ֵ���ڵ���ͳ�����ֵ����ͳ�����ֵû�н����
      	//����ͳ�����ֵȡǰһ��ͳ�����ֵ
        if ((tmpBuff[VOL_A_MAX+1] > pBalanceParaBuff[VOL_A_MAX+1])
        	|| (tmpBuff[VOL_A_MAX+1] == pBalanceParaBuff[VOL_A_MAX+1]&&tmpBuff[VOL_A_MAX] > pBalanceParaBuff[VOL_A_MAX])
        	  || pBalanceParaBuff[VOL_A_MAX] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_MAX] = tmpBuff[VOL_A_MAX];
          pBalanceParaBuff[VOL_A_MAX+1] = tmpBuff[VOL_A_MAX+1];
        
          pBalanceParaBuff[VOL_A_MAX_TIME] = tmpBuff[VOL_A_MAX_TIME];
          pBalanceParaBuff[VOL_A_MAX_TIME+1] = tmpBuff[VOL_A_MAX_TIME+1];
          pBalanceParaBuff[VOL_A_MAX_TIME+2] = tmpBuff[VOL_A_MAX_TIME+2];
        }
      }

      //�µ�ѹ��Сֵ������ʱ��
      if (tmpBuff[VOL_A_MIN] != 0xEE && (tmpBuff[VOL_A_MIN] != 0x00&&tmpBuff[VOL_A_MIN] != 0x00))
      {
        //��ǰһ����Сֵ��Ϊ����С�ڵ�����Сֵ���ߵ�����Сֵ������
        //������Сֵȡǰһ����Сֵ
        if ((tmpBuff[VOL_A_MIN+1]<pBalanceParaBuff[VOL_A_MIN+1])
        	|| (tmpBuff[VOL_A_MIN+1]==pBalanceParaBuff[VOL_A_MIN+1]&&tmpBuff[VOL_A_MIN]<pBalanceParaBuff[VOL_A_MIN])
        	  || pBalanceParaBuff[VOL_A_MIN] == 0xEE)
        {
          pBalanceParaBuff[VOL_A_MIN] = tmpBuff[VOL_A_MIN];
          pBalanceParaBuff[VOL_A_MIN+1] = tmpBuff[VOL_A_MIN+1];
        
          pBalanceParaBuff[VOL_A_MIN_TIME] = tmpBuff[VOL_A_MIN_TIME];
          pBalanceParaBuff[VOL_A_MIN_TIME+1] = tmpBuff[VOL_A_MIN_TIME+1];
          pBalanceParaBuff[VOL_A_MIN_TIME+2] = tmpBuff[VOL_A_MIN_TIME+2];
        }
      }

      //�µ�ѹƽ��ֵ
      if (tmpBuff[VOL_A_AVER] != 0xEE)
      { 
        if (pBalanceParaBuff[VOL_A_AVER]!=0xEE)
        {
          tmpData = 0;
          tmpData = (pBalanceParaBuff[VOL_A_AVER]|pBalanceParaBuff[VOL_A_AVER+1]<<8)*(pBalanceParaBuff[VOL_A_AVER_COUNTER]|pBalanceParaBuff[VOL_A_AVER_COUNTER+1]<<8);
          tmpData += (tmpBuff[VOL_A_AVER]|tmpBuff[VOL_A_AVER+1]<<8)*(tmpBuff[VOL_A_AVER_COUNTER]|tmpBuff[VOL_A_AVER_COUNTER+1]<<8);
          tmpData /= (pBalanceParaBuff[VOL_A_AVER_COUNTER]|pBalanceParaBuff[VOL_A_AVER_COUNTER+1]<<8)+(tmpBuff[VOL_A_AVER_COUNTER]|tmpBuff[VOL_A_AVER_COUNTER+1]<<8);
       
          pBalanceParaBuff[VOL_A_AVER] = tmpData&0xFF;
          pBalanceParaBuff[VOL_A_AVER+1] = (tmpData>>8)&0xFF;
          tmpData = (pBalanceParaBuff[VOL_A_AVER_COUNTER]|pBalanceParaBuff[VOL_A_AVER_COUNTER+1]<<8)+(tmpBuff[VOL_A_AVER_COUNTER]|tmpBuff[VOL_A_AVER_COUNTER+1]<<8);
          pBalanceParaBuff[VOL_A_AVER_COUNTER] = tmpData&0xFF;
          pBalanceParaBuff[VOL_A_AVER_COUNTER+1] = tmpData>>8&0xFF;
        }
        else
        {
          pBalanceParaBuff[VOL_A_AVER] = tmpBuff[VOL_A_AVER];
          pBalanceParaBuff[VOL_A_AVER+1] = tmpBuff[VOL_A_AVER+1];
          pBalanceParaBuff[VOL_A_AVER_COUNTER] = tmpBuff[VOL_A_AVER_COUNTER];
          pBalanceParaBuff[VOL_A_AVER_COUNTER+1] = tmpBuff[VOL_A_AVER_COUNTER+1];
        }
      }

    	//2.2 B���ѹͳ��
    	if (tmpBuff[VOL_B_UP_UP_TIME] != 0xEE) //Խ������
      {
        if (pBalanceParaBuff[VOL_B_UP_UP_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_B_UP_UP_TIME] = tmpBuff[VOL_B_UP_UP_TIME];
          pBalanceParaBuff[VOL_B_UP_UP_TIME+1] = tmpBuff[VOL_B_UP_UP_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_B_UP_UP_TIME] | tmpBuff[VOL_B_UP_UP_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_B_UP_UP_TIME]| pBalanceParaBuff[VOL_B_UP_UP_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_B_UP_UP_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_B_UP_UP_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_B_UP_TIME] != 0xEE) //Խ����
      {
        if (pBalanceParaBuff[VOL_B_UP_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_B_UP_TIME] = tmpBuff[VOL_B_UP_TIME];
          pBalanceParaBuff[VOL_B_UP_TIME+1] = tmpBuff[VOL_B_UP_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_B_UP_TIME] | tmpBuff[VOL_B_UP_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_B_UP_TIME]| pBalanceParaBuff[VOL_B_UP_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_B_UP_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_B_UP_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_B_DOWN_DOWN_TIME] != 0xEE) //Խ������
      {
        if (pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME] = tmpBuff[VOL_B_DOWN_DOWN_TIME];
          pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME+1] = tmpBuff[VOL_B_DOWN_DOWN_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_B_DOWN_DOWN_TIME] | tmpBuff[VOL_B_DOWN_DOWN_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_B_DOWN_TIME] != 0xEE) //Խ����
      {
        if (pBalanceParaBuff[VOL_B_DOWN_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_B_DOWN_TIME] = tmpBuff[VOL_B_DOWN_TIME];
          pBalanceParaBuff[VOL_B_DOWN_TIME+1] = tmpBuff[VOL_B_DOWN_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_B_DOWN_TIME] | tmpBuff[VOL_B_DOWN_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_B_DOWN_TIME]| pBalanceParaBuff[VOL_B_DOWN_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_B_DOWN_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_B_DOWN_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_B_OK_TIME] != 0xEE) //�ϸ�
      {
        if (pBalanceParaBuff[VOL_B_OK_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_B_OK_TIME] = tmpBuff[VOL_B_OK_TIME];
          pBalanceParaBuff[VOL_B_OK_TIME+1] = tmpBuff[VOL_B_OK_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_B_OK_TIME] | tmpBuff[VOL_B_OK_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_B_OK_TIME]| pBalanceParaBuff[VOL_B_OK_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_B_OK_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_B_OK_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      //�µ�ѹ���ֵ������ʱ��
      if (tmpBuff[VOL_B_MAX]!=0xEE)
      {
      	//��ǰһ��ͳ�����ֵ���ڵ���ͳ�����ֵ����ͳ�����ֵû�н����
      	//����ͳ�����ֵȡǰһ��ͳ�����ֵ
        if ((tmpBuff[VOL_B_MAX+1] > pBalanceParaBuff[VOL_B_MAX+1])
        	|| (tmpBuff[VOL_B_MAX+1] == pBalanceParaBuff[VOL_B_MAX+1]&&tmpBuff[VOL_B_MAX] > pBalanceParaBuff[VOL_B_MAX])
        	  || pBalanceParaBuff[VOL_B_MAX] == 0xEE) 
        {
          pBalanceParaBuff[VOL_B_MAX] = tmpBuff[VOL_B_MAX];
          pBalanceParaBuff[VOL_B_MAX+1] = tmpBuff[VOL_B_MAX+1];
        
          pBalanceParaBuff[VOL_B_MAX_TIME] = tmpBuff[VOL_B_MAX_TIME];
          pBalanceParaBuff[VOL_B_MAX_TIME+1] = tmpBuff[VOL_B_MAX_TIME+1];
          pBalanceParaBuff[VOL_B_MAX_TIME+2] = tmpBuff[VOL_B_MAX_TIME+2];
        }
      }
      
      //�µ�ѹ��Сֵ������ʱ��
      if (tmpBuff[VOL_B_MIN] != 0xEE && (tmpBuff[VOL_B_MIN] != 0x00&&tmpBuff[VOL_B_MIN] != 0x00))
      {
        //��ǰһ����Сֵ��Ϊ����С�ڵ�����Сֵ���ߵ�����Сֵ������
        //������Сֵȡǰһ����Сֵ
        if ((tmpBuff[VOL_B_MIN+1]<pBalanceParaBuff[VOL_B_MIN+1])
        	|| (tmpBuff[VOL_B_MIN+1]==pBalanceParaBuff[VOL_B_MIN+1]&&tmpBuff[VOL_B_MIN]<pBalanceParaBuff[VOL_B_MIN])
        	  || pBalanceParaBuff[VOL_B_MIN] == 0xEE)
        {
          pBalanceParaBuff[VOL_B_MIN] = tmpBuff[VOL_B_MIN];
          pBalanceParaBuff[VOL_B_MIN+1] = tmpBuff[VOL_B_MIN+1];
        
          pBalanceParaBuff[VOL_B_MIN_TIME] = tmpBuff[VOL_B_MIN_TIME];
          pBalanceParaBuff[VOL_B_MIN_TIME+1] = tmpBuff[VOL_B_MIN_TIME+1];
          pBalanceParaBuff[VOL_B_MIN_TIME+2] = tmpBuff[VOL_B_MIN_TIME+2];
        }
      }
     
      //�µ�ѹƽ��ֵ
      if (tmpBuff[VOL_B_AVER] != 0xEE)
      { 
        if (pBalanceParaBuff[VOL_B_AVER]!=0xEE)
        {
          tmpData = 0;
          tmpData = (pBalanceParaBuff[VOL_B_AVER] | pBalanceParaBuff[VOL_B_AVER+1]<<8)*(pBalanceParaBuff[VOL_B_AVER_COUNTER]|pBalanceParaBuff[VOL_B_AVER_COUNTER+1]<<8);
          tmpData += (tmpBuff[VOL_B_AVER]|tmpBuff[VOL_B_AVER+1]<<8)*(tmpBuff[VOL_B_AVER_COUNTER]|tmpBuff[VOL_B_AVER_COUNTER+1]<<8);
          tmpData /= (pBalanceParaBuff[VOL_B_AVER_COUNTER]|pBalanceParaBuff[VOL_B_AVER_COUNTER+1]<<8)+(tmpBuff[VOL_B_AVER_COUNTER]|tmpBuff[VOL_B_AVER_COUNTER+1]<<8);
       
          pBalanceParaBuff[VOL_B_AVER] = tmpData&0xFF;
          pBalanceParaBuff[VOL_B_AVER+1] = (tmpData>>8)&0xFF;
          tmpData = (pBalanceParaBuff[VOL_B_AVER_COUNTER]|pBalanceParaBuff[VOL_B_AVER_COUNTER+1]<<8)+(tmpBuff[VOL_B_AVER_COUNTER]|tmpBuff[VOL_B_AVER_COUNTER+1]<<8);
          pBalanceParaBuff[VOL_B_AVER_COUNTER] = tmpData&0xFF;
          pBalanceParaBuff[VOL_B_AVER_COUNTER+1] = tmpData>>8&0xFF;
        }
        else
        {
          pBalanceParaBuff[VOL_B_AVER] = tmpBuff[VOL_B_AVER];
          pBalanceParaBuff[VOL_B_AVER+1] = tmpBuff[VOL_B_AVER+1];
          pBalanceParaBuff[VOL_B_AVER_COUNTER] = tmpBuff[VOL_B_AVER_COUNTER];
          pBalanceParaBuff[VOL_B_AVER_COUNTER+1] = tmpBuff[VOL_B_AVER_COUNTER+1];
        }
      }
      
      //2.3 C���ѹͳ��
    	if (tmpBuff[VOL_C_UP_UP_TIME] != 0xEE) //Խ������
      {
        if (pBalanceParaBuff[VOL_C_UP_UP_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_C_UP_UP_TIME] = tmpBuff[VOL_C_UP_UP_TIME];
          pBalanceParaBuff[VOL_C_UP_UP_TIME+1] = tmpBuff[VOL_C_UP_UP_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_C_UP_UP_TIME] | tmpBuff[VOL_C_UP_UP_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_C_UP_UP_TIME]| pBalanceParaBuff[VOL_C_UP_UP_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_C_UP_UP_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_C_UP_UP_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_C_UP_TIME] != 0xEE) //Խ����
      {
        if (pBalanceParaBuff[VOL_C_UP_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_C_UP_TIME] = tmpBuff[VOL_C_UP_TIME];
          pBalanceParaBuff[VOL_C_UP_TIME+1] = tmpBuff[VOL_C_UP_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_C_UP_TIME] | tmpBuff[VOL_C_UP_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_C_UP_TIME]| pBalanceParaBuff[VOL_C_UP_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_C_UP_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_C_UP_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_C_DOWN_DOWN_TIME] != 0xEE) //Խ������
      {
        if (pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME] = tmpBuff[VOL_C_DOWN_DOWN_TIME];
          pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME+1] = tmpBuff[VOL_C_DOWN_DOWN_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_C_DOWN_DOWN_TIME] | tmpBuff[VOL_C_DOWN_DOWN_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_C_DOWN_TIME] != 0xEE) //Խ����
      {
        if (pBalanceParaBuff[VOL_C_DOWN_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_C_DOWN_TIME] = tmpBuff[VOL_C_DOWN_TIME];
          pBalanceParaBuff[VOL_C_DOWN_TIME+1] = tmpBuff[VOL_C_DOWN_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_C_DOWN_TIME] | tmpBuff[VOL_C_DOWN_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_C_DOWN_TIME]| pBalanceParaBuff[VOL_C_DOWN_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_C_DOWN_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_C_DOWN_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      if (tmpBuff[VOL_C_OK_TIME] != 0xEE) //�ϸ�
      {
        if (pBalanceParaBuff[VOL_C_OK_TIME] == 0xEE)
        {
          pBalanceParaBuff[VOL_C_OK_TIME] = tmpBuff[VOL_C_OK_TIME];
          pBalanceParaBuff[VOL_C_OK_TIME+1] = tmpBuff[VOL_C_OK_TIME+1];
        }
        else
        {
          tmpDataShort = tmpBuff[VOL_C_OK_TIME] | tmpBuff[VOL_C_OK_TIME+1]<<8;
          tmpDataShort += pBalanceParaBuff[VOL_C_OK_TIME]| pBalanceParaBuff[VOL_C_OK_TIME+1]<<8;
          
          pBalanceParaBuff[VOL_C_OK_TIME] = tmpDataShort&0xff;
          pBalanceParaBuff[VOL_C_OK_TIME+1] = (tmpDataShort>>8)&0xff;
        }
      }
      
      //�µ�ѹ���ֵ������ʱ��
      if (tmpBuff[VOL_C_MAX]!=0xEE)
      {
      	//��ǰһ��ͳ�����ֵ���ڵ���ͳ�����ֵ����ͳ�����ֵû�н����
      	//����ͳ�����ֵȡǰһ��ͳ�����ֵ
        if ((tmpBuff[VOL_C_MAX+1] > pBalanceParaBuff[VOL_C_MAX+1])
        	|| (tmpBuff[VOL_C_MAX+1] == pBalanceParaBuff[VOL_C_MAX+1]&&tmpBuff[VOL_C_MAX] > pBalanceParaBuff[VOL_C_MAX])
        	  || pBalanceParaBuff[VOL_C_MAX] == 0xEE) 
        {
          pBalanceParaBuff[VOL_C_MAX] = tmpBuff[VOL_C_MAX];
          pBalanceParaBuff[VOL_C_MAX+1] = tmpBuff[VOL_C_MAX+1];
        
          pBalanceParaBuff[VOL_C_MAX_TIME] = tmpBuff[VOL_C_MAX_TIME];
          pBalanceParaBuff[VOL_C_MAX_TIME+1] = tmpBuff[VOL_C_MAX_TIME+1];
          pBalanceParaBuff[VOL_C_MAX_TIME+2] = tmpBuff[VOL_C_MAX_TIME+2];
        }
      }
      
      //�µ�ѹ��Сֵ������ʱ��
      if (tmpBuff[VOL_C_MIN] != 0xEE && (tmpBuff[VOL_C_MIN] != 0x00&&tmpBuff[VOL_C_MIN] != 0x00))
      {
        //��ǰһ����Сֵ��Ϊ����С�ڵ�����Сֵ���ߵ�����Сֵ������
        //������Сֵȡǰһ����Сֵ
        if ((tmpBuff[VOL_C_MIN+1]<pBalanceParaBuff[VOL_C_MIN+1])
        	|| (tmpBuff[VOL_C_MIN+1]==pBalanceParaBuff[VOL_C_MIN+1]&&tmpBuff[VOL_C_MIN]<pBalanceParaBuff[VOL_C_MIN])
        	  || pBalanceParaBuff[VOL_C_MIN] == 0xEE)
        {
          pBalanceParaBuff[VOL_C_MIN] = tmpBuff[VOL_C_MIN];
          pBalanceParaBuff[VOL_C_MIN+1] = tmpBuff[VOL_C_MIN+1];
        
          pBalanceParaBuff[VOL_C_MIN_TIME] = tmpBuff[VOL_C_MIN_TIME];
          pBalanceParaBuff[VOL_C_MIN_TIME+1] = tmpBuff[VOL_C_MIN_TIME+1];
          pBalanceParaBuff[VOL_C_MIN_TIME+2] = tmpBuff[VOL_C_MIN_TIME+2];
        }
      }
     
      //�µ�ѹƽ��ֵ
      if (tmpBuff[VOL_C_AVER] != 0xEE)
      { 
        if (pBalanceParaBuff[VOL_C_AVER]!=0xEE)
        {
          tmpData = 0;
          tmpData = (pBalanceParaBuff[VOL_C_AVER]|pBalanceParaBuff[VOL_C_AVER+1]<<8)*(pBalanceParaBuff[VOL_C_AVER_COUNTER]|pBalanceParaBuff[VOL_C_AVER_COUNTER+1]<<8);
          tmpData += (tmpBuff[VOL_C_AVER]|tmpBuff[VOL_C_AVER+1]<<8)*(tmpBuff[VOL_C_AVER_COUNTER]|tmpBuff[VOL_C_AVER_COUNTER+1]<<8);
          tmpData /= (pBalanceParaBuff[VOL_C_AVER_COUNTER]|pBalanceParaBuff[VOL_C_AVER_COUNTER+1]<<8)+(tmpBuff[VOL_C_AVER_COUNTER]|tmpBuff[VOL_C_AVER_COUNTER+1]<<8);
       
          pBalanceParaBuff[VOL_C_AVER] = tmpData&0xFF;
          pBalanceParaBuff[VOL_C_AVER+1] = (tmpData>>8)&0xFF;
          tmpData = (pBalanceParaBuff[VOL_C_AVER_COUNTER]|pBalanceParaBuff[VOL_C_AVER_COUNTER+1]<<8)+(tmpBuff[VOL_C_AVER_COUNTER]|tmpBuff[VOL_C_AVER_COUNTER+1]<<8);
          pBalanceParaBuff[VOL_A_AVER_COUNTER] = tmpData&0xFF;
          pBalanceParaBuff[VOL_A_AVER_COUNTER+1] = tmpData>>8&0xFF;
        }
        else
        {
          pBalanceParaBuff[VOL_C_AVER] = tmpBuff[VOL_C_AVER];
          pBalanceParaBuff[VOL_C_AVER+1] = tmpBuff[VOL_C_AVER+1];
          pBalanceParaBuff[VOL_C_AVER_COUNTER] = tmpBuff[VOL_C_AVER_COUNTER];
          pBalanceParaBuff[VOL_C_AVER_COUNTER+1] = tmpBuff[VOL_C_AVER_COUNTER+1];
        }
      } 

      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("realStatisticPerPoint:A���µ�ѹԽ������ʱ��=%d\n",pBalanceParaBuff[VOL_A_UP_UP_TIME]| pBalanceParaBuff[VOL_A_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ�ѹԽ����ʱ��=%d\n",pBalanceParaBuff[VOL_A_UP_TIME]| pBalanceParaBuff[VOL_A_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ�ѹԽ������ʱ��=%d\n",pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ�ѹԽ����ʱ��=%d\n",pBalanceParaBuff[VOL_A_DOWN_TIME]| pBalanceParaBuff[VOL_A_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ�ѹ�ϸ�ʱ��=%d\n",pBalanceParaBuff[VOL_A_OK_TIME]| pBalanceParaBuff[VOL_A_OK_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ�ѹ���ֵ=%x������ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_A_MAX]| pBalanceParaBuff[VOL_A_MAX+1]<<8,pBalanceParaBuff[VOL_A_MAX_TIME+2],pBalanceParaBuff[VOL_A_MAX_TIME+1],pBalanceParaBuff[VOL_A_MAX_TIME]);
        printf("realStatisticPerPoint:A���µ�ѹ��Сֵ=%x������ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_A_MIN]| pBalanceParaBuff[VOL_A_MIN+1]<<8,pBalanceParaBuff[VOL_A_MIN_TIME+2],pBalanceParaBuff[VOL_A_MIN_TIME+1],pBalanceParaBuff[VOL_A_MIN_TIME]);
        printf("realStatisticPerPoint:A���µ�ѹƽ��ֵ=%x\n",pBalanceParaBuff[VOL_A_AVER]| pBalanceParaBuff[VOL_A_AVER+1]<<8);

        printf("realStatisticPerPoint:B���µ�ѹԽ������ʱ��=%d\n",pBalanceParaBuff[VOL_B_UP_UP_TIME]| pBalanceParaBuff[VOL_B_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:B���µ�ѹԽ����ʱ��=%d\n",pBalanceParaBuff[VOL_B_UP_TIME]| pBalanceParaBuff[VOL_B_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:B���µ�ѹԽ������ʱ��=%d\n",pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:B���µ�ѹԽ����ʱ��=%d\n",pBalanceParaBuff[VOL_B_DOWN_TIME]| pBalanceParaBuff[VOL_B_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:B���µ�ѹ�ϸ�ʱ��=%d\n",pBalanceParaBuff[VOL_B_OK_TIME]| pBalanceParaBuff[VOL_B_OK_TIME+1]<<8);
        printf("realStatisticPerPoint:B���µ�ѹ���ֵ=%x������ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_B_MAX]| pBalanceParaBuff[VOL_B_MAX+1]<<8,pBalanceParaBuff[VOL_B_MAX_TIME+2],pBalanceParaBuff[VOL_B_MAX_TIME+1],pBalanceParaBuff[VOL_B_MAX_TIME]);
        printf("realStatisticPerPoint:B���µ�ѹ��Сֵ=%x������ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_B_MIN]| pBalanceParaBuff[VOL_B_MIN+1]<<8,pBalanceParaBuff[VOL_B_MIN_TIME+2],pBalanceParaBuff[VOL_B_MIN_TIME+1],pBalanceParaBuff[VOL_B_MIN_TIME]);
        printf("realStatisticPerPoint:B���µ�ѹƽ��ֵ=%x\n",pBalanceParaBuff[VOL_B_AVER]| pBalanceParaBuff[VOL_B_AVER+1]<<8);

        printf("realStatisticPerPoint:C���µ�ѹԽ������ʱ��=%d\n",pBalanceParaBuff[VOL_C_UP_UP_TIME]| pBalanceParaBuff[VOL_C_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:C���µ�ѹԽ����ʱ��=%d\n",pBalanceParaBuff[VOL_C_UP_TIME]| pBalanceParaBuff[VOL_C_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:C���µ�ѹԽ������ʱ��=%d\n",pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:C���µ�ѹԽ����ʱ��=%d\n",pBalanceParaBuff[VOL_C_DOWN_TIME]| pBalanceParaBuff[VOL_C_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:C���µ�ѹ�ϸ�ʱ��=%d\n",pBalanceParaBuff[VOL_C_OK_TIME]| pBalanceParaBuff[VOL_C_OK_TIME+1]<<8);
        printf("realStatisticPerPoint:C���µ�ѹ���ֵ=%x������ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_C_MAX]| pBalanceParaBuff[VOL_C_MAX+1]<<8,pBalanceParaBuff[VOL_C_MAX_TIME+2],pBalanceParaBuff[VOL_C_MAX_TIME+1],pBalanceParaBuff[VOL_C_MAX_TIME]);
        printf("realStatisticPerPoint:C���µ�ѹ��Сֵ=%x������ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_C_MIN]| pBalanceParaBuff[VOL_C_MIN+1]<<8,pBalanceParaBuff[VOL_C_MIN_TIME+2],pBalanceParaBuff[VOL_C_MIN_TIME+1],pBalanceParaBuff[VOL_C_MIN_TIME]);
        printf("realStatisticPerPoint:C���µ�ѹƽ��ֵ=%x\n",pBalanceParaBuff[VOL_C_AVER]| pBalanceParaBuff[VOL_C_AVER+1]<<8);
      }


	    //3.��ƽ���Խ���ۼ�***********************************************************
	    //3.1 ��ѹ��ƽ��Խ���ۼ�ʱ��
	    if (tmpBuff[VOL_UNBALANCE_TIME] != 0xEE) //��ѹ��ƽ��
  	  {
	      if (pBalanceParaBuff[VOL_UNBALANCE_TIME] != 0xEE)  
        {
       	  tmpDataShort = tmpBuff[VOL_UNBALANCE_TIME] | tmpBuff[VOL_UNBALANCE_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[VOL_UNBALANCE_TIME] | pBalanceParaBuff[VOL_UNBALANCE_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[VOL_UNBALANCE_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[VOL_UNBALANCE_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[VOL_UNBALANCE_TIME] = tmpBuff[VOL_UNBALANCE_TIME];
       	  pBalanceParaBuff[VOL_UNBALANCE_TIME+1] = tmpBuff[VOL_UNBALANCE_TIME|1];
        }
      }
      
      //3.2 ��ѹ��ƽ��Խ�����ֵ������ʱ��
      if (tmpBuff[VOL_UNB_MAX] != 0xEE)  //��ѹ��ƽ�����ֵ
      {
      	if (pBalanceParaBuff[VOL_UNB_MAX] != 0xEE)
      	{
          if ((tmpBuff[VOL_UNB_MAX+1] > pBalanceParaBuff[VOL_UNB_MAX+1])
          	|| ((tmpBuff[VOL_UNB_MAX+1] == pBalanceParaBuff[VOL_UNB_MAX+1]) && (tmpBuff[VOL_UNB_MAX] > pBalanceParaBuff[VOL_UNB_MAX])))
          {
            pBalanceParaBuff[VOL_UNB_MAX] = tmpBuff[VOL_UNB_MAX];
            pBalanceParaBuff[VOL_UNB_MAX+1] = tmpBuff[VOL_UNB_MAX+1];
            
            pBalanceParaBuff[VOL_UNB_MAX_TIME] = tmpBuff[VOL_UNB_MAX_TIME];
            pBalanceParaBuff[VOL_UNB_MAX_TIME+1] = tmpBuff[VOL_UNB_MAX_TIME+1];
            pBalanceParaBuff[VOL_UNB_MAX_TIME+2] = tmpBuff[VOL_UNB_MAX_TIME+2];
          }
        }
        else
        {
          pBalanceParaBuff[VOL_UNB_MAX] = tmpBuff[VOL_UNB_MAX];
          pBalanceParaBuff[VOL_UNB_MAX+1] = tmpBuff[VOL_UNB_MAX+1];
            
          pBalanceParaBuff[VOL_UNB_MAX_TIME] = tmpBuff[VOL_UNB_MAX_TIME];
          pBalanceParaBuff[VOL_UNB_MAX_TIME+1] = tmpBuff[VOL_UNB_MAX_TIME+1];
          pBalanceParaBuff[VOL_UNB_MAX_TIME+2] = tmpBuff[VOL_UNB_MAX_TIME+2];
        }
	    }
	    //3.3 ������ƽ��Խ���ۼ�ʱ��
	    if (tmpBuff[CUR_UNBALANCE_TIME] != 0xEE) //������ƽ��
  	  {
	      if (pBalanceParaBuff[CUR_UNBALANCE_TIME] != 0xEE)
        {
       	  tmpDataShort = tmpBuff[CUR_UNBALANCE_TIME] | tmpBuff[CUR_UNBALANCE_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_UNBALANCE_TIME] | pBalanceParaBuff[CUR_UNBALANCE_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_UNBALANCE_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_UNBALANCE_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_UNBALANCE_TIME] = tmpBuff[CUR_UNBALANCE_TIME];
       	  pBalanceParaBuff[CUR_UNBALANCE_TIME+1] = tmpBuff[CUR_UNBALANCE_TIME|1];
        }
      }
      
      //3.4 ������ƽ��Խ�����ֵ������ʱ��
      if (tmpBuff[CUR_UNB_MAX] != 0xEE)  //������ƽ�����ֵ
      {
      	if (pBalanceParaBuff[CUR_UNB_MAX] != 0xEE)
      	{
          if ((tmpBuff[CUR_UNB_MAX+1] > pBalanceParaBuff[CUR_UNB_MAX+1])
          	|| ((tmpBuff[CUR_UNB_MAX+1] == pBalanceParaBuff[CUR_UNB_MAX+1]) && (tmpBuff[CUR_UNB_MAX] > pBalanceParaBuff[CUR_UNB_MAX])))
          {
            pBalanceParaBuff[CUR_UNB_MAX] = tmpBuff[CUR_UNB_MAX];
            pBalanceParaBuff[CUR_UNB_MAX+1] = tmpBuff[CUR_UNB_MAX+1];
            
            pBalanceParaBuff[CUR_UNB_MAX_TIME] = tmpBuff[CUR_UNB_MAX_TIME];
            pBalanceParaBuff[CUR_UNB_MAX_TIME+1] = tmpBuff[CUR_UNB_MAX_TIME+1];
            pBalanceParaBuff[CUR_UNB_MAX_TIME+2] = tmpBuff[CUR_UNB_MAX_TIME+2];
          }
        }
        else
        {
          pBalanceParaBuff[CUR_UNB_MAX] = tmpBuff[CUR_UNB_MAX];
          pBalanceParaBuff[CUR_UNB_MAX+1] = tmpBuff[CUR_UNB_MAX+1];
            
          pBalanceParaBuff[CUR_UNB_MAX_TIME] = tmpBuff[CUR_UNB_MAX_TIME];
          pBalanceParaBuff[CUR_UNB_MAX_TIME+1] = tmpBuff[CUR_UNB_MAX_TIME+1];
          pBalanceParaBuff[CUR_UNB_MAX_TIME+2] = tmpBuff[CUR_UNB_MAX_TIME+2];
        }
	    }
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("realStatisticPerPoint:��ѹ��ƽ���ۼ�ʱ��=%d\n",pBalanceParaBuff[VOL_UNBALANCE_TIME]| pBalanceParaBuff[VOL_UNBALANCE_TIME+1]<<8);
        printf("realStatisticPerPoint:��ѹ��ƽ������ֵ=%x,����ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[VOL_UNB_MAX]| pBalanceParaBuff[VOL_UNB_MAX+1]<<8,tmpBuff[VOL_UNB_MAX_TIME+2],tmpBuff[VOL_UNB_MAX_TIME+1],tmpBuff[VOL_UNB_MAX_TIME]);
        printf("realStatisticPerPoint:������ƽ���ۼ�ʱ��=%d\n",pBalanceParaBuff[CUR_UNBALANCE_TIME]| pBalanceParaBuff[CUR_UNBALANCE_TIME+1]<<8);
        printf("realStatisticPerPoint:������ƽ������ֵ=%x,����ʱ��=%02x %02x:%02x\n",pBalanceParaBuff[CUR_UNB_MAX]| pBalanceParaBuff[CUR_UNB_MAX+1]<<8,tmpBuff[CUR_UNB_MAX_TIME+2],tmpBuff[CUR_UNB_MAX_TIME+1],tmpBuff[CUR_UNB_MAX_TIME]);
      }

	    //4.����ͳ��***********************************************************
	    //4.1 A�����ͳ��
	    if (tmpBuff[CUR_A_UP_UP_TIME] != 0xEE) //Խ������
  	  {
	      if (pBalanceParaBuff[CUR_A_UP_UP_TIME] != 0xEE)  
        {
       	  tmpDataShort = tmpBuff[CUR_A_UP_UP_TIME] | tmpBuff[CUR_A_UP_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_A_UP_UP_TIME] | pBalanceParaBuff[CUR_A_UP_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_A_UP_UP_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_A_UP_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_A_UP_UP_TIME] = tmpBuff[CUR_A_UP_UP_TIME];
       	  pBalanceParaBuff[CUR_A_UP_UP_TIME+1] = tmpBuff[CUR_A_UP_UP_TIME|1];
        }
      }

      if (tmpBuff[CUR_A_UP_TIME] != 0xEE)  //Խ����
  	  {  	        
	      if (pBalanceParaBuff[CUR_A_UP_TIME] != 0xEE)  
        {
       	  tmpDataShort = tmpBuff[CUR_A_UP_TIME] | tmpBuff[CUR_A_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_A_UP_TIME] | pBalanceParaBuff[CUR_A_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_A_UP_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_A_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_A_UP_TIME] = tmpBuff[CUR_A_UP_TIME];
       	  pBalanceParaBuff[CUR_A_UP_TIME+1] = tmpBuff[CUR_A_UP_TIME|1];
        }
      }

      if (tmpBuff[CUR_A_MAX] != 0xEE)  //�������ֵ
      {
      	if (pBalanceParaBuff[CUR_A_MAX] != 0xEE)
      	{
          if ((tmpBuff[CUR_A_MAX] | (tmpBuff[CUR_A_MAX+1]<<8) | ((tmpBuff[CUR_A_MAX+2]&0x7f)<<16))>(pBalanceParaBuff[CUR_A_MAX] | (pBalanceParaBuff[CUR_A_MAX+1]<<8) | ((pBalanceParaBuff[CUR_A_MAX+2]&0x7f)<<16)))
          {
            pBalanceParaBuff[CUR_A_MAX] = tmpBuff[CUR_A_MAX];
            pBalanceParaBuff[CUR_A_MAX+1] = tmpBuff[CUR_A_MAX+1];
            
            pBalanceParaBuff[CUR_A_MAX_TIME] = tmpBuff[CUR_A_MAX_TIME];
            pBalanceParaBuff[CUR_A_MAX_TIME+1] = tmpBuff[CUR_A_MAX_TIME+1];
            pBalanceParaBuff[CUR_A_MAX_TIME+2] = tmpBuff[CUR_A_MAX_TIME+2];
          }
        }
        else
        {
          pBalanceParaBuff[CUR_A_MAX] = tmpBuff[CUR_A_MAX];
          pBalanceParaBuff[CUR_A_MAX+1] = tmpBuff[CUR_A_MAX+1];
            
          pBalanceParaBuff[CUR_A_MAX_TIME] = tmpBuff[CUR_A_MAX_TIME];
          pBalanceParaBuff[CUR_A_MAX_TIME+1] = tmpBuff[CUR_A_MAX_TIME+1];
          pBalanceParaBuff[CUR_A_MAX_TIME+2] = tmpBuff[CUR_A_MAX_TIME+2];
        }
	    }
	    
	    //4.2 B�����ͳ��
	    if (tmpBuff[CUR_B_UP_UP_TIME] != 0xEE) //Խ������
  	  {
	      if (pBalanceParaBuff[CUR_B_UP_UP_TIME] != 0xEE)  
        {
       	  tmpDataShort = tmpBuff[CUR_B_UP_UP_TIME] | tmpBuff[CUR_B_UP_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_B_UP_UP_TIME] | pBalanceParaBuff[CUR_B_UP_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_B_UP_UP_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_B_UP_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_B_UP_UP_TIME] = tmpBuff[CUR_B_UP_UP_TIME];
       	  pBalanceParaBuff[CUR_B_UP_UP_TIME+1] = tmpBuff[CUR_B_UP_UP_TIME|1];
        }
      }
      
      if (tmpBuff[CUR_B_UP_TIME] != 0xEE)  //Խ����
  	  {
	      if (pBalanceParaBuff[CUR_B_UP_TIME] != 0xEE)  
        {
       	  tmpDataShort = tmpBuff[CUR_B_UP_TIME] | tmpBuff[CUR_B_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_B_UP_TIME] | pBalanceParaBuff[CUR_B_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_B_UP_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_B_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_B_UP_TIME] = tmpBuff[CUR_B_UP_TIME];
       	  pBalanceParaBuff[CUR_B_UP_TIME+1] = tmpBuff[CUR_B_UP_TIME|1];
        }
      }
      
      if (tmpBuff[CUR_B_MAX] != 0xEE)  //�������ֵ
      {
      	if (pBalanceParaBuff[CUR_B_MAX] != 0xEE)
      	{
          if ((tmpBuff[CUR_B_MAX] | (tmpBuff[CUR_B_MAX+1]<<8) | ((tmpBuff[CUR_B_MAX+2]&0x7f)<<16))>(pBalanceParaBuff[CUR_B_MAX] | (pBalanceParaBuff[CUR_B_MAX+1]<<8) | ((pBalanceParaBuff[CUR_B_MAX+2]&0x7f)<<16)))
          {
            pBalanceParaBuff[CUR_B_MAX] = tmpBuff[CUR_B_MAX];
            pBalanceParaBuff[CUR_B_MAX+1] = tmpBuff[CUR_B_MAX+1];
            
            pBalanceParaBuff[CUR_B_MAX_TIME] = tmpBuff[CUR_B_MAX_TIME];
            pBalanceParaBuff[CUR_B_MAX_TIME+1] = tmpBuff[CUR_B_MAX_TIME+1];
            pBalanceParaBuff[CUR_B_MAX_TIME+2] = tmpBuff[CUR_B_MAX_TIME+2];
          }
        }
        else
        {
          pBalanceParaBuff[CUR_B_MAX] = tmpBuff[CUR_B_MAX];
          pBalanceParaBuff[CUR_B_MAX+1] = tmpBuff[CUR_B_MAX+1];
            
          pBalanceParaBuff[CUR_B_MAX_TIME] = tmpBuff[CUR_B_MAX_TIME];
          pBalanceParaBuff[CUR_B_MAX_TIME+1] = tmpBuff[CUR_B_MAX_TIME+1];
          pBalanceParaBuff[CUR_B_MAX_TIME+2] = tmpBuff[CUR_B_MAX_TIME+2];
        }
	    }
	    
	    //4.3 C�����ͳ��
	    if (tmpBuff[CUR_C_UP_UP_TIME] != 0xEE) //Խ������
  	  {  	        
	      if (pBalanceParaBuff[CUR_C_UP_UP_TIME] != 0xEE)
        {
       	  tmpDataShort = tmpBuff[CUR_C_UP_UP_TIME] | tmpBuff[CUR_C_UP_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_C_UP_UP_TIME] | pBalanceParaBuff[CUR_C_UP_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_C_UP_UP_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_C_UP_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_C_UP_UP_TIME] = tmpBuff[CUR_C_UP_UP_TIME];
       	  pBalanceParaBuff[CUR_C_UP_UP_TIME+1] = tmpBuff[CUR_C_UP_UP_TIME|1];
        }
      }
      
      if (tmpBuff[CUR_C_UP_TIME] != 0xEE)  //Խ����
  	  {  	        
	      if (pBalanceParaBuff[CUR_C_UP_TIME] != 0xEE)  
        {
       	  tmpDataShort = tmpBuff[CUR_C_UP_TIME] | tmpBuff[CUR_C_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[CUR_C_UP_TIME] | pBalanceParaBuff[CUR_C_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[CUR_C_UP_TIME] = tmpDataShort&0xFF;
       	  pBalanceParaBuff[CUR_C_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
        }
        else
        {
          pBalanceParaBuff[CUR_C_UP_TIME] = tmpBuff[CUR_C_UP_TIME];
       	  pBalanceParaBuff[CUR_C_UP_TIME+1] = tmpBuff[CUR_C_UP_TIME|1];
        }
      }
      
      if (tmpBuff[CUR_C_MAX] != 0xEE)  //�������ֵ
      {
      	if (pBalanceParaBuff[CUR_C_MAX] != 0xEE)
      	{
          if ((tmpBuff[CUR_C_MAX] | (tmpBuff[CUR_C_MAX+1]<<8) | ((tmpBuff[CUR_C_MAX+2]&0x7f)<<16))>(pBalanceParaBuff[CUR_C_MAX] | (pBalanceParaBuff[CUR_C_MAX+1]<<8) | ((pBalanceParaBuff[CUR_C_MAX+2]&0x7f)<<16)))
          {
            pBalanceParaBuff[CUR_C_MAX] = tmpBuff[CUR_C_MAX];
            pBalanceParaBuff[CUR_C_MAX+1] = tmpBuff[CUR_C_MAX+1];
            
            pBalanceParaBuff[CUR_C_MAX_TIME] = tmpBuff[CUR_C_MAX_TIME];
            pBalanceParaBuff[CUR_C_MAX_TIME+1] = tmpBuff[CUR_C_MAX_TIME+1];
            pBalanceParaBuff[CUR_C_MAX_TIME+2] = tmpBuff[CUR_C_MAX_TIME+2];
          }
        }
        else
        {
          pBalanceParaBuff[CUR_C_MAX] = tmpBuff[CUR_C_MAX];
          pBalanceParaBuff[CUR_C_MAX+1] = tmpBuff[CUR_C_MAX+1];
            
          pBalanceParaBuff[CUR_C_MAX_TIME] = tmpBuff[CUR_C_MAX_TIME];
          pBalanceParaBuff[CUR_C_MAX_TIME+1] = tmpBuff[CUR_C_MAX_TIME+1];
          pBalanceParaBuff[CUR_C_MAX_TIME+2] = tmpBuff[CUR_C_MAX_TIME+2];
        }
	    }

      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("realStatisticPerPoint:A���µ���Խ������ʱ��=%d\n", pBalanceParaBuff[CUR_A_UP_UP_TIME] | pBalanceParaBuff[CUR_A_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ���Խ����ʱ��=%d\n", pBalanceParaBuff[CUR_A_UP_TIME] | pBalanceParaBuff[CUR_A_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ������ֵ=%x,����ʱ��=%02x %02x:%02x\n", pBalanceParaBuff[CUR_A_MAX] | pBalanceParaBuff[CUR_A_MAX+1]<<8,pBalanceParaBuff[CUR_A_MAX_TIME+2],pBalanceParaBuff[CUR_A_MAX_TIME+1],pBalanceParaBuff[CUR_A_MAX_TIME]);

        printf("realStatisticPerPoint:A���µ���Խ������ʱ��=%d\n", pBalanceParaBuff[CUR_B_UP_UP_TIME] | pBalanceParaBuff[CUR_B_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ���Խ����ʱ��=%d\n", pBalanceParaBuff[CUR_B_UP_TIME] | pBalanceParaBuff[CUR_B_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ������ֵ=%x,����ʱ��=%02x %02x:%02x\n", pBalanceParaBuff[CUR_B_MAX] | pBalanceParaBuff[CUR_B_MAX+1]<<8,pBalanceParaBuff[CUR_B_MAX_TIME+2],pBalanceParaBuff[CUR_B_MAX_TIME+1],pBalanceParaBuff[CUR_B_MAX_TIME]);

        printf("realStatisticPerPoint:A���µ���Խ������ʱ��=%d\n", pBalanceParaBuff[CUR_C_UP_UP_TIME] | pBalanceParaBuff[CUR_C_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ���Խ����ʱ��=%d\n", pBalanceParaBuff[CUR_C_UP_TIME] | pBalanceParaBuff[CUR_C_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A���µ������ֵ=%x,����ʱ��=%02x %02x:%02x\n", pBalanceParaBuff[CUR_C_MAX] | pBalanceParaBuff[CUR_C_MAX+1]<<8,pBalanceParaBuff[CUR_C_MAX_TIME+2],pBalanceParaBuff[CUR_C_MAX_TIME+1],pBalanceParaBuff[CUR_C_MAX_TIME]);
      }

	    //5.���ڹ���Խ���ۼ�ʱ��***********************************************************
	    if (tmpBuff[APPARENT_POWER_UP_TIME] != 0xEE)
	    {
	      if (pBalanceParaBuff[APPARENT_POWER_UP_TIME] != 0xEE)
	      {
	        tmpDataShort = tmpBuff[APPARENT_POWER_UP_TIME] | tmpBuff[APPARENT_POWER_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[APPARENT_POWER_UP_TIME] | pBalanceParaBuff[APPARENT_POWER_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[APPARENT_POWER_UP_TIME] = tmpDataShort&0xFF;
	        pBalanceParaBuff[APPARENT_POWER_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
	      }
	      else
	      {
       	  pBalanceParaBuff[APPARENT_POWER_UP_TIME] = tmpBuff[APPARENT_POWER_UP_TIME];
	        pBalanceParaBuff[APPARENT_POWER_UP_TIME+1] = tmpBuff[APPARENT_POWER_UP_TIME+1];
	      }
	    }
	    
	    if (tmpBuff[APPARENT_POWER_UP_UP_TIME] != 0xEE)
	    {
	      if (pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME] != 0xEE)
	      {
	        tmpDataShort = tmpBuff[APPARENT_POWER_UP_UP_TIME] | tmpBuff[APPARENT_POWER_UP_UP_TIME+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME] | pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME+1]<<8;
       	  
       	  pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME] = tmpDataShort&0xFF;
	        pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = (tmpDataShort>>8)&0xFF;
	      }
	      else
	      {
       	  pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME] = tmpBuff[APPARENT_POWER_UP_UP_TIME];
	        pBalanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = tmpBuff[APPARENT_POWER_UP_UP_TIME+1];
	      }
	    }
	    
	    //6.���ܱ��������***********************************************************
	    //6.1 �ܶ���ͳ��
	    if (tmpBuff[PHASE_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[OPEN_PHASE_TIMES] = tmpBuff[PHASE_DOWN_TIMES];
	      pBalanceParaBuff[OPEN_PHASE_TIMES+1] = tmpBuff[PHASE_DOWN_TIMES+1];
	    }
	    if (tmpBuff[TOTAL_PHASE_DOWN_TIME] != 0xEE)
	    {
	      pBalanceParaBuff[OPEN_PHASE_MINUTES] = tmpBuff[TOTAL_PHASE_DOWN_TIME];
	      pBalanceParaBuff[OPEN_PHASE_MINUTES+1] = tmpBuff[TOTAL_PHASE_DOWN_TIME+1];
	    }
	    if (tmpBuff[LAST_PHASE_DOWN_BEGIN] != 0xEE)
	    {
	      pBalanceParaBuff[OPEN_PHASE_LAST_BEG] = tmpBuff[LAST_PHASE_DOWN_BEGIN];
	      pBalanceParaBuff[OPEN_PHASE_LAST_BEG+1] = tmpBuff[LAST_PHASE_DOWN_BEGIN+1];
	      pBalanceParaBuff[OPEN_PHASE_LAST_BEG+2] = tmpBuff[LAST_PHASE_DOWN_BEGIN+2];
	      pBalanceParaBuff[OPEN_PHASE_LAST_BEG+3] = tmpBuff[LAST_PHASE_DOWN_BEGIN+3];
	    }
	    if (tmpBuff[LAST_PHASE_DOWN_END] != 0xEE)
	    {
	      pBalanceParaBuff[LAST_PHASE_DOWN_END] = tmpBuff[LAST_PHASE_DOWN_END];
	      pBalanceParaBuff[LAST_PHASE_DOWN_END+1] = tmpBuff[LAST_PHASE_DOWN_END+1];
	      pBalanceParaBuff[LAST_PHASE_DOWN_END+2] = tmpBuff[LAST_PHASE_DOWN_END+2];
	      pBalanceParaBuff[LAST_PHASE_DOWN_END+3] = tmpBuff[LAST_PHASE_DOWN_END+3];
	    }
	    
	    //6.2 A�����ͳ��
	    if (tmpBuff[PHASE_A_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[A_OPEN_PHASE_TIMES] = tmpBuff[PHASE_A_DOWN_TIMES];
	      pBalanceParaBuff[A_OPEN_PHASE_TIMES+1] = tmpBuff[PHASE_A_DOWN_TIMES+1];
	    }
	    if (tmpBuff[PHASE_A_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[A_OPEN_PHASE_MINUTES] = tmpBuff[TOTAL_PHASE_A_DOWN_TIME];
	      pBalanceParaBuff[A_OPEN_PHASE_MINUTES+1] = tmpBuff[TOTAL_PHASE_A_DOWN_TIME+1];
	    }
	    if (tmpBuff[LAST_PHASE_A_DOWN_BEGIN] != 0xEE)
	    {
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_BEG] = tmpBuff[LAST_PHASE_A_DOWN_BEGIN];
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_BEG+1] = tmpBuff[LAST_PHASE_A_DOWN_BEGIN+1];
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_BEG+2] = tmpBuff[LAST_PHASE_A_DOWN_BEGIN+2];
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_BEG+3] = tmpBuff[LAST_PHASE_A_DOWN_BEGIN+3];
	    }
	    if (tmpBuff[LAST_PHASE_A_DOWN_END] != 0xEE)
	    {
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_END] = tmpBuff[LAST_PHASE_A_DOWN_END];
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_END+1] = tmpBuff[LAST_PHASE_A_DOWN_END+1];
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_END+2] = tmpBuff[LAST_PHASE_A_DOWN_END+2];
	      pBalanceParaBuff[A_OPEN_PHASE_LAST_END+3] = tmpBuff[LAST_PHASE_A_DOWN_END+3];
	    }
	    
	    //6.3 B�����ͳ��
	    if (tmpBuff[PHASE_B_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[B_OPEN_PHASE_TIMES] = tmpBuff[PHASE_B_DOWN_TIMES];
	      pBalanceParaBuff[B_OPEN_PHASE_TIMES+1] = tmpBuff[PHASE_B_DOWN_TIMES+1];
	    }
	    if (tmpBuff[PHASE_B_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[B_OPEN_PHASE_MINUTES] = tmpBuff[TOTAL_PHASE_B_DOWN_TIME];
	      pBalanceParaBuff[B_OPEN_PHASE_MINUTES+1] = tmpBuff[TOTAL_PHASE_B_DOWN_TIME+1];
	    }
	    if (tmpBuff[LAST_PHASE_B_DOWN_BEGIN] != 0xEE)
	    {
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_BEG] = tmpBuff[LAST_PHASE_B_DOWN_BEGIN];
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_BEG+1] = tmpBuff[LAST_PHASE_B_DOWN_BEGIN+1];
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_BEG+2] = tmpBuff[LAST_PHASE_B_DOWN_BEGIN+2];
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_BEG+3] = tmpBuff[LAST_PHASE_B_DOWN_BEGIN+3];
	    }
	    if (tmpBuff[LAST_PHASE_B_DOWN_END] != 0xEE)
	    {
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_END] = tmpBuff[LAST_PHASE_B_DOWN_END];
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_END+1] = tmpBuff[LAST_PHASE_B_DOWN_END+1];
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_END+2] = tmpBuff[LAST_PHASE_B_DOWN_END+2];
	      pBalanceParaBuff[B_OPEN_PHASE_LAST_END+3] = tmpBuff[LAST_PHASE_B_DOWN_END+3];
	    }
	    
	    //6.4 C�����ͳ��
	    if (tmpBuff[PHASE_C_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[C_OPEN_PHASE_TIMES] = tmpBuff[PHASE_C_DOWN_TIMES];
	      pBalanceParaBuff[C_OPEN_PHASE_TIMES+1] = tmpBuff[PHASE_C_DOWN_TIMES+1];
	    }
	    if (tmpBuff[PHASE_C_DOWN_TIMES] != 0xEE)
	    {
	      pBalanceParaBuff[C_OPEN_PHASE_MINUTES] = tmpBuff[TOTAL_PHASE_C_DOWN_TIME];
	      pBalanceParaBuff[C_OPEN_PHASE_MINUTES+1] = tmpBuff[TOTAL_PHASE_C_DOWN_TIME+1];
	    }
	    if (tmpBuff[LAST_PHASE_C_DOWN_BEGIN] != 0xEE)
	    {
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_BEG] = tmpBuff[LAST_PHASE_C_DOWN_BEGIN];
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_BEG+1] = tmpBuff[LAST_PHASE_C_DOWN_BEGIN+1];
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_BEG+2] = tmpBuff[LAST_PHASE_C_DOWN_BEGIN+2];
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_BEG+3] = tmpBuff[LAST_PHASE_C_DOWN_BEGIN+3];
	    }
	    if (tmpBuff[LAST_PHASE_C_DOWN_END] != 0xEE)
	    {
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_END] = tmpBuff[LAST_PHASE_C_DOWN_END];
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_END+1] = tmpBuff[LAST_PHASE_C_DOWN_END+1];
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_END+2] = tmpBuff[LAST_PHASE_C_DOWN_END+2];
	      pBalanceParaBuff[C_OPEN_PHASE_LAST_END+3] = tmpBuff[LAST_PHASE_C_DOWN_END+3];
	    }
	    
	    //7.����ͳ��***********************************************************
	    if (tmpBuff[MAX_TOTAL_REQ_TIME] != 0xEE)
	    {
	      if (pBalanceParaBuff[MAX_TOTAL_REQ_TIME] != 0xEE)
	      {
	        if ((tmpBuff[MAX_TOTAL_REQ+2]>pBalanceParaBuff[MAX_TOTAL_REQ+2])
	      	  || (tmpBuff[MAX_TOTAL_REQ+2]==pBalanceParaBuff[MAX_TOTAL_REQ+2]&&(tmpBuff[MAX_TOTAL_REQ+1]>pBalanceParaBuff[MAX_TOTAL_REQ+1]))
	      	    || (tmpBuff[MAX_TOTAL_REQ+2]==pBalanceParaBuff[MAX_TOTAL_REQ+2]&&tmpBuff[MAX_TOTAL_REQ+1]==pBalanceParaBuff[MAX_TOTAL_REQ+1]&&tmpBuff[MAX_TOTAL_REQ]>pBalanceParaBuff[MAX_TOTAL_REQ]))
	        {
	          pBalanceParaBuff[MAX_TOTAL_REQ] = tmpBuff[MAX_TOTAL_REQ];
	          pBalanceParaBuff[MAX_TOTAL_REQ+1] = tmpBuff[MAX_TOTAL_REQ+1];
	          pBalanceParaBuff[MAX_TOTAL_REQ+2] = tmpBuff[MAX_TOTAL_REQ+2];
	          
	          pBalanceParaBuff[MAX_TOTAL_REQ_TIME] = tmpBuff[MAX_TOTAL_REQ_TIME];
	          pBalanceParaBuff[MAX_TOTAL_REQ_TIME+1] = tmpBuff[MAX_TOTAL_REQ_TIME+1];
	          pBalanceParaBuff[MAX_TOTAL_REQ_TIME+2] = tmpBuff[MAX_TOTAL_REQ_TIME+2];
	        }
	      }
	      else
	      {
	        pBalanceParaBuff[MAX_TOTAL_REQ] = tmpBuff[MAX_TOTAL_REQ];
	        pBalanceParaBuff[MAX_TOTAL_REQ+1] = tmpBuff[MAX_TOTAL_REQ+1];
	        pBalanceParaBuff[MAX_TOTAL_REQ+2] = tmpBuff[MAX_TOTAL_REQ+2];
	          
	        pBalanceParaBuff[MAX_TOTAL_REQ_TIME] = tmpBuff[MAX_TOTAL_REQ_TIME];
	        pBalanceParaBuff[MAX_TOTAL_REQ_TIME+1] = tmpBuff[MAX_TOTAL_REQ_TIME+1];
	        pBalanceParaBuff[MAX_TOTAL_REQ_TIME+2] = tmpBuff[MAX_TOTAL_REQ_TIME+2];
	      }
	    }
	    
	    //8.���������ֶ��ۼ�ͳ��***********************************************************
	    //8.1 �¹�����������1�ۼ�ʱ��
	    if (tmpBuff[FACTOR_SEG_1] != 0xEE)
	    {
	      if (pBalanceParaBuff[FACTOR_SEG_1] != 0xEE)
	      {
	        tmpDataShort = tmpBuff[FACTOR_SEG_1] | tmpBuff[FACTOR_SEG_1+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[FACTOR_SEG_1] | pBalanceParaBuff[FACTOR_SEG_1+1]<<8;
       	  
       	  pBalanceParaBuff[FACTOR_SEG_1] = tmpDataShort&0xFF;
	        pBalanceParaBuff[FACTOR_SEG_1+1] = (tmpDataShort>>8)&0xFF;
	      }
	      else
	      {
	      	 pBalanceParaBuff[FACTOR_SEG_1] = tmpBuff[FACTOR_SEG_1];
	      	 pBalanceParaBuff[FACTOR_SEG_1+1] = tmpBuff[FACTOR_SEG_1+1];
	      }
	    }
	    //8.2 �¹�����������2�ۼ�ʱ��
	    if (tmpBuff[FACTOR_SEG_2] != 0xEE)
	    {
	      if (pBalanceParaBuff[FACTOR_SEG_2] != 0xEE)
	      {
	        tmpDataShort = tmpBuff[FACTOR_SEG_2] | tmpBuff[FACTOR_SEG_2+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[FACTOR_SEG_2] | pBalanceParaBuff[FACTOR_SEG_2+1]<<8;
       	  
       	  pBalanceParaBuff[FACTOR_SEG_2] = tmpDataShort&0xFF;
	        pBalanceParaBuff[FACTOR_SEG_2+1] = (tmpDataShort>>8)&0xFF;
	      }
	      else
	      {
	      	 pBalanceParaBuff[FACTOR_SEG_2] = tmpBuff[FACTOR_SEG_2];
	      	 pBalanceParaBuff[FACTOR_SEG_2+1] = tmpBuff[FACTOR_SEG_2+1];
	      }
	    }
	    //8.3 �¹�����������3�ۼ�ʱ��
	    if (tmpBuff[FACTOR_SEG_3] != 0xEE)
	    {
	      if (pBalanceParaBuff[FACTOR_SEG_3] != 0xEE)
	      {
	        tmpDataShort = tmpBuff[FACTOR_SEG_3] | tmpBuff[FACTOR_SEG_3+1]<<8;
       	  tmpDataShort += pBalanceParaBuff[FACTOR_SEG_3] | pBalanceParaBuff[FACTOR_SEG_3+1]<<8;
       	  
       	  pBalanceParaBuff[FACTOR_SEG_3] = tmpDataShort&0xFF;
	        pBalanceParaBuff[FACTOR_SEG_3+1] = (tmpDataShort>>8)&0xFF;
	      }
	      else
	      {
	      	 pBalanceParaBuff[FACTOR_SEG_3] = tmpBuff[FACTOR_SEG_3];
	      	 pBalanceParaBuff[FACTOR_SEG_3+1] = tmpBuff[FACTOR_SEG_3+1];
	      }
	    }

    }
}

/*******************************************************
��������: copyDayFreeze
��������: 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ�� 
*******************************************************/
void copyDayFreeze(INT8U port)
{
	  INT8U              i;
	  DATE_TIME          tmpTime;
	  struct cpAddrLink  *tmpNode;
	  INT8U              readBuff[LENGTH_OF_ENERGY_RECORD];
	 
	  if((tmpNode=initPortMeterLink(port))!=NULL)
	  {
	    while(tmpNode!=NULL)
	    {
	      //ͨ�Ź�Լ����Ϊ�ֳ�����豸���ǽ�������װ�ã�����ʵʱ�����������
	      //if (tmpNode->protocol == AC_SAMPLE || tmpNode->protocol == SUPERVISAL_DEVICE)    
	      if (tmpNode->protocol == SUPERVISAL_DEVICE)    
	      {
	        tmpNode = tmpNode->next;   //bug,2011-08-03
	        
	        continue;
	      }

        //��ȡ��ǰ���ݲ�����
	      tmpTime = timeHexToBcd(sysTime);
        if (readMeterData(readBuff, tmpNode->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0) == TRUE)
        {
          //ת��Ϊ�����ն������ʾֵ
	        tmpTime = timeHexToBcd(sysTime);
          tmpTime.minute = teCopyRunPara.para[port].copyTime[0];
  	  	  tmpTime.hour = teCopyRunPara.para[port].copyTime[1];
  	  	 
          saveMeterData(tmpNode->mp, port+1, tmpTime, readBuff, DAY_BALANCE, COPY_FREEZE_COPY_DATA,LENGTH_OF_ENERGY_RECORD);
        }
        
	      tmpTime = timeHexToBcd(sysTime);
        if (readMeterData(readBuff, tmpNode->mp, LAST_TODAY, REQ_REQTIME_DATA, &tmpTime, 0) == TRUE)
        {
          //ת��Ϊ�����ն�������
	        tmpTime = timeHexToBcd(sysTime);
          tmpTime.minute = teCopyRunPara.para[port].copyTime[0];
  	  	  tmpTime.hour = teCopyRunPara.para[port].copyTime[1];
  	  	 
          saveMeterData(tmpNode->mp, port+1, tmpTime, readBuff, DAY_BALANCE, COPY_FREEZE_COPY_REQ,LENGTH_OF_REQ_RECORD);
        }
        
        usleep(50000);
        
        tmpNode = tmpNode->next;
	    }
	  }
}

/*******************************************************
��������: computeLeftPower
��������: 
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ��
�޸���ʷ:
    1.2012-5-22,�޸Ĺ����δͶ��ʱ,���õ�������ʣ�������,�����ʣ�������ɸ�ֵ�Ĵ���.
      ����ˢ�µ�ʣ�����Ϊ1000Kwh,���õ�������1000��,ʣ������ͻ���-1000,�Ժ�Ͳ����ٱ�
*******************************************************/
void computeLeftPower(DATE_TIME statisTime)
{
    INT16U    i, pn;
    INT8U     leftPower[12];
    INT8U     tmpByte, quantity;
    INT32U    tmpData1 , tmpData2;
    INT8U     readBuff[LEN_OF_ZJZ_BALANCE_RECORD];
    DATE_TIME readTime;
    INT16U    j, k, onlyHasPulsePn;
    
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
    	 printf("computeLeftPower:��ʼ����ʣ�����\n");
    }
    
   	if (totalAddGroup.numberOfzjz != 0)
    {
      //�����ܼ��������ý��д���
      for (i = 0; i < totalAddGroup.numberOfzjz; i++)
      {
      	pn = totalAddGroup.perZjz[i].zjzNo;
      	
 	    	if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_JUMP_IN)
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
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
    	         printf("computeLeftPower:�ܼ���%d�����Ͷ��״̬�ұ��ܼ���ֻ�����������,�������������ʣ�����\n", pn);
             }
 	    		
 	    		   continue;
 	    		}
 	    	}

  	    
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
    	    printf("computeLeftPower:�����ܼ���%dʣ�����\n", pn);
        }
  	    
        //��ȡ��ǰʣ�����
        readTime = statisTime;
        if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
        {
     	    //��ȡ��ǰ�ܼ��鵱�µ�����
     	    readTime = statisTime;
        	if (readMeterData(readBuff, pn, REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0) == TRUE)
  	      {
  	      	if (readBuff[GP_MONTH_WORK+3]!= 0xFF && readBuff[GP_MONTH_WORK+3] != 0xEE)
  	      	{
  	      			tmpData1 = 0x00000000;
      	        tmpData2 = 0x00000000;
      	        
      	        //�ο��ܼ��鵱�µ�����C
      	        if (leftPower[8]==0xee)  //���ԭ�����ܼ����µ���������,����ǰ�ܼ��µ�������C
      	        {
    	            leftPower[8]  = readBuff[GP_MONTH_WORK+3];
                  leftPower[9]  = readBuff[GP_MONTH_WORK+4];
                  leftPower[10] = readBuff[GP_MONTH_WORK+5];
                  leftPower[11] = ((readBuff[GP_MONTH_WORK]&0x01)<<6)
                                          | (readBuff[GP_MONTH_WORK]&0x10)
                                          | (readBuff[GP_MONTH_WORK+6]&0x0f);
      	        }
      	        
      	        tmpData1 = (leftPower[8]&0xF)        + ((leftPower[8]>>4)&0xF)*10
      	                 + (leftPower[9]&0xF)*100    + ((leftPower[9]>>4)&0xF)*1000
      	                 + (leftPower[10]&0xF)*10000 + ((leftPower[10]>>4)&0xF)*100000
      	                 + (leftPower[11]&0xF)*1000000;
          	    if ((leftPower[11]>>6)&0x01 == 0x01)
          	    {
          	      tmpData1 *= 1000;
          	    }
          	    
          	    //���ε����ܼӵ���D
          	    tmpByte = ((readBuff[GP_MONTH_WORK]&0x01)<<6)
                        |(readBuff[GP_MONTH_WORK]&0x10)
                        |(readBuff[GP_MONTH_WORK+6]&0x0f);
      	    	  tmpData2 = (readBuff[GP_MONTH_WORK+3]&0xF)       + ((readBuff[GP_MONTH_WORK+3]>>4)&0xF)*10
      	                 + (readBuff[GP_MONTH_WORK+4]&0xF)*100   + ((readBuff[GP_MONTH_WORK+4]>>4)&0xF)*1000
      	                 + (readBuff[GP_MONTH_WORK+5]&0xF)*10000 + ((readBuff[GP_MONTH_WORK+5]>>4)&0xF)*100000
      	                 + (tmpByte&0xF)*1000000;
      	        if ((tmpByte >> 8) &0x01 == 0x01)
      	        {
      	           tmpData2 *= 1000;
      	        }
      	        
      	        //��ǰ�ܼ��µ�����D-�ο��ܼ��µ�����C
    	          if (debugInfo&PRINT_BALANCE_DEBUG)
    	          {
    	            printf("computeLeftPower:��ǰ�ܼ��µ�����D=%d,�ο��ܼ��µ�����C=%d\n",tmpData2,tmpData1);
    	          }
      	        if (tmpData2 >= tmpData1)
      	        {
      	          tmpData2 -= tmpData1;
      	            
      	          //�ο�ʣ�������B
      	          tmpData1 = (leftPower[4]&0xF)       + ((leftPower[4]>>4)&0xF)*10
      	                    + (leftPower[5]&0xF)*100   + ((leftPower[5]>>4)&0xF)*1000
      	                    + (leftPower[6]&0xF)*10000 + ((leftPower[6]>>4)&0xF)*100000
      	                    + (leftPower[7]&0xF)*1000000;
          	      if ((leftPower[7]>>6)&0x01 == 0x01)
          	      {
          	        tmpData1 *= 1000;
          	      }

          	      if ((leftPower[7]>>4)&0x01 == 0x01)   //ʣ�������Ϊ��
          	      {
        	           if (debugInfo&PRINT_BALANCE_DEBUG)
        	           {
        	             printf("computeLeftPower:��ǰ�ο�ʣ�������Ϊ��\n");
        	           }
    
                     if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                     {
        	             printf("computeLeftPower(��ǰ�ο�ʣ�������Ϊ��):δͶ��,����ʣ�����\n");
                     }
                     else
                     {
                       tmpData1 += tmpData2;
    
      	        	     tmpData2 = 0x00000000;
      	               quantity = dataFormat(&tmpData1, &tmpData2, 0x03);
      	    
      	               tmpData1 = hexToBcd(tmpData1);
      	     
      	               leftPower[0] = tmpData1&0xFF;
      	               leftPower[1] = (tmpData1>>8)&0xFF;
      	               leftPower[2] = (tmpData1>>16)&0xFF;//�����ܼ������
      	               leftPower[3] = (tmpData1>>24)&0xFF;
      	    
      	               leftPower[3] |= (1 <<4);    //�����ο�ʣ�������Ϊ��,���Լ�����Ϊ��
      	             
                       saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                     }
          	      }
          	      else
          	      {
                    //��ǰʣ�������A=B-(D-C)
                    if (tmpData1>= tmpData2)
                    {
                      if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                      {
      	                if (debugInfo&PRINT_BALANCE_DEBUG)
      	                {
      	                  printf("computeLeftPower:��ǰʣ�����>���µ���,�����δͶ��,����\n");
      	                }
                      }
                      else
                      {
                        tmpData1 -= tmpData2;
  
        	        	    tmpData2 = 0x00000000;
        	              quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
        	    
        	              tmpData1 = hexToBcd(tmpData1);
        	     
        	              leftPower[0] = tmpData1&0xFF;
        	              leftPower[1] = (tmpData1>>8)&0xFF;
        	              leftPower[2] = (tmpData1>>16)&0xFF;
        	              leftPower[3] = (tmpData1>>24)&0xFF;
        	    
        	              leftPower[3] |= (quantity <<4);
        	             
                        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);                      
                      }
        	          }
        	          else
        	          {
                      if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                      {
      	                if (debugInfo&PRINT_BALANCE_DEBUG)
      	                {
      	                  printf("computeLeftPower:��ǰʣ�����<���µ���,�����δͶ��,����\n");
      	                }
                      }
                      else
                      {
                        tmpData1 = tmpData2-tmpData1;
      
        	        	    tmpData2 = 0x00000000;
        	              quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
        	    
        	              tmpData1 = hexToBcd(tmpData1);
        	     
        	              leftPower[0] = tmpData1&0xFF;
        	              leftPower[1] = (tmpData1>>8)&0xFF;
        	              leftPower[2] = (tmpData1>>16)&0xFF;
        	              leftPower[3] = (tmpData1>>24)&0xFF;
        	     
        	              leftPower[3] |= (quantity <<4);
        	              leftPower[3] |= 0x10;
        	             
                        saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                      }
        	          }
        	        }
      	        }
  	      	}
  	      	else
  	      	{
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
    	          printf("computeLeftPower:������ǰ�ܼ��鵱�µ�����,���µ���Ϊ0xee\n", pn);
              }
              
  	      	  continue;
  	      	}
  	    	}
  	    	else
  	    	{
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
    	        printf("computeLeftPower:δ������ǰ�ܼ��鵱�µ�����\n", pn);
            }
  	    	}
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
    	      printf("computeLeftPower:δ����ʣ�����\n", pn);
          }
  	    }
      }
    }
}

/*******************************************************
��������:computeInTimeLeftPower
��������:���㼰ʱʣ�����
���ú���:     
�����ú���:
�������:   
�������:  
����ֵ�� 
*******************************************************/
BOOL computeInTimeLeftPower(INT8U zjzNo, DATE_TIME statisTime, INT8U *leftPower, INT8U ifSave)
{
    INT16U    i, pn, kk;
    INT8U     tmpByte, quantity;
    INT32U    tmpData1 , tmpData2;
    INT8U     readBuff[LEN_OF_ZJZ_BALANCE_RECORD];
    DATE_TIME readTime;

    pn = zjzNo;

    //��ȡ��ǰʣ�����
    readTime = statisTime;
    if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
    {
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("computeInTimeLeftPower:��ȡʣ������ɹ�\n");
    	}

 	    //��ȡ��ǰ�ܼ��鵱�µ�����
 	    readTime = statisTime;
 	    
 	    memset(readBuff, 0xee, LEN_OF_ZJZ_BALANCE_RECORD);
 	    
      for (i = 0; i < totalAddGroup.numberOfzjz; i++)
      {
      	if (totalAddGroup.perZjz[i].zjzNo == zjzNo)
      	{
      		 break;
      	}
      }
      
      readTime = timeHexToBcd(statisTime);
    	if (groupBalance(readBuff, i, totalAddGroup.perZjz[i].pointNumber, GP_DAY_WORK | 0x80, readTime) == TRUE)
      {
      	if (readBuff[GP_MONTH_WORK+3]!= 0xFF && readBuff[GP_MONTH_WORK+3] != 0xEE)
      	{
      			tmpData1 = 0x00000000;
  	        tmpData2 = 0x00000000;
  	        
  	        //�ο��ܼ��鵱�µ�����C
  	        if (leftPower[8]==0xee)  //���ԭ�����ܼ����µ���������,����ǰ�ܼ��µ�������C
  	        {
              leftPower[8]  = readBuff[GP_MONTH_WORK+3];
              leftPower[9]  = readBuff[GP_MONTH_WORK+4];
              leftPower[10] = readBuff[GP_MONTH_WORK+5];
              leftPower[11] = ((readBuff[GP_MONTH_WORK]&0x01)<<6)
                                      | (readBuff[GP_MONTH_WORK]&0x10)
                                      | (readBuff[GP_MONTH_WORK+6]&0x0f);
  	        }
  	        
  	        tmpData1 = (leftPower[8]&0xF)        + ((leftPower[8]>>4)&0xF)*10
  	                 + (leftPower[9]&0xF)*100    + ((leftPower[9]>>4)&0xF)*1000
  	                 + (leftPower[10]&0xF)*10000 + ((leftPower[10]>>4)&0xF)*100000
  	                 + (leftPower[11]&0xF)*1000000;
      	    if ((leftPower[11]>>6)&0x01 == 0x01)
      	    {
      	      tmpData1 *= 1000;
      	    }
      	    
      	    //���ε����ܼӵ���D
      	    tmpByte = ((readBuff[GP_MONTH_WORK]&0x01)<<6)
                    |(readBuff[GP_MONTH_WORK]&0x10)
                    |(readBuff[GP_MONTH_WORK+6]&0x0f);
  	    	  tmpData2 = (readBuff[GP_MONTH_WORK+3]&0xF)       + ((readBuff[GP_MONTH_WORK+3]>>4)&0xF)*10
  	                 + (readBuff[GP_MONTH_WORK+4]&0xF)*100   + ((readBuff[GP_MONTH_WORK+4]>>4)&0xF)*1000
  	                 + (readBuff[GP_MONTH_WORK+5]&0xF)*10000 + ((readBuff[GP_MONTH_WORK+5]>>4)&0xF)*100000
  	                 + (tmpByte&0xF)*1000000;
  	        if ((tmpByte >> 8) &0x01 == 0x01)
  	        {
  	           tmpData2 *= 1000;
  	        }

    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
    	        printf("computeInTimeLeftPower:��ǰ�ܼ��µ�����D=%d,�ο��ܼ��µ�����C=%d\n",tmpData2,tmpData1);
    	      }
  	        
  	        //��ǰ�ܼ��µ�����D-�ο��ܼ��µ�����C
  	        if (tmpData2 >= tmpData1)
  	        {
  	          tmpData2 -= tmpData1;
  	            
  	          //�ο�ʣ�������B
  	          tmpData1 = (leftPower[4]&0xF)       + ((leftPower[4]>>4)&0xF)*10
  	                    + (leftPower[5]&0xF)*100   + ((leftPower[5]>>4)&0xF)*1000
  	                    + (leftPower[6]&0xF)*10000 + ((leftPower[6]>>4)&0xF)*100000
  	                    + (leftPower[7]&0xF)*1000000;
      	      if ((leftPower[7]>>6)&0x01 == 0x01)
      	      {
      	        tmpData1 *= 1000;
      	      }

      	      if ((leftPower[7]>>4)&0x01 == 0x01)   //ʣ�������Ϊ��
      	      {
    	           if (debugInfo&PRINT_BALANCE_DEBUG)
    	           {
    	             printf("computeInTimeLeftPower:��ǰ�ο�ʣ�������Ϊ��\n");
    	           }

                 if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                 {
    	             printf("computeInTimeLeftPower(��ǰ�ο�ʣ�������Ϊ��):δͶ��,����ʣ�����\n");
                 }
                 else
                 {
                   tmpData1 += tmpData2;

  	        	     tmpData2 = 0x00000000;
  	               quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
  	    
  	               tmpData1 = hexToBcd(tmpData1);
  	     
  	               leftPower[0] = tmpData1&0xFF;
  	               leftPower[1] = (tmpData1>>8)&0xFF;
  	               leftPower[2] = (tmpData1>>16)&0xFF;//�����ܼ������
  	               leftPower[3] = (tmpData1>>24)&0xFF;
  	    
  	               leftPower[3] |= (1 <<4);    //�����ο�ʣ�������Ϊ��,���Լ�����Ϊ��
  	             
  	               if (ifSave)
  	               {
                     saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                   }
                 }
                 return TRUE;
      	      }
      	      else
      	      {
                //��ǰʣ�������A=B-(D-C)
                if (tmpData1>= tmpData2)
                {
      	           if (debugInfo&PRINT_BALANCE_DEBUG)
      	           {
      	             printf("computeInTimeLeftPower:��ǰʣ�������>��ǰ�µ���\n");
      	           }
  
                   if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                   {
      	             printf("computeInTimeLeftPower(��ǰʣ�������>��ǰ�µ���):δͶ��,����ʣ�����\n");
                   }
                   else
                   {
                     tmpData1 -= tmpData2;
  
    	        	     tmpData2 = 0x00000000;
    	               quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
    	    
    	               tmpData1 = hexToBcd(tmpData1);
    	     
    	               leftPower[0] = tmpData1&0xFF;
    	               leftPower[1] = (tmpData1>>8)&0xFF;
    	               leftPower[2] = (tmpData1>>16)&0xFF;//�����ܼ������
    	               leftPower[3] = (tmpData1>>24)&0xFF;
    	    
    	               leftPower[3] |= (quantity <<4);
    	             
    	               if (ifSave)
    	               {
                       saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                     }
                   }
                   return TRUE;
    	          }
    	          else
    	          {
      	           if (debugInfo&PRINT_BALANCE_DEBUG)
      	           {
      	             printf("computeInTimeLeftPower:��ǰʣ�������<��ǰ�µ���,ʣ��Ϊ��\n");
      	           }
      	           
                   if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                   {
      	             if (debugInfo&PRINT_BALANCE_DEBUG)
      	             {
      	               printf("computeInTimeLeftPower:��ǰʣ�������<��ǰ�µ���,�����δͶ��,����\n");
      	             }    	             
                   }
                   else
                   {
                     tmpData1 = tmpData2-tmpData1;
  
      	        	   tmpData2 = 0x00000000;
      	             quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
      	    
      	             tmpData1 = hexToBcd(tmpData1);
      	     
      	             leftPower[0] = tmpData1&0xFF;
      	             leftPower[1] = (tmpData1>>8)&0xFF;
      	             leftPower[2] = (tmpData1>>16)&0xFF;
      	             leftPower[3] = (tmpData1>>24)&0xFF;
      	    
      	             leftPower[3] |= (quantity <<4);
      	             leftPower[3] |= 0x10;
      	             
      	             if (ifSave)
      	             {
                       saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                     }
                   }
  
                   return TRUE;
    	          }
    	        }
  	        }
      	}
      	else
      	{
    	    if (debugInfo&PRINT_BALANCE_DEBUG)
    	    {
    	      printf("computeInTimeLeftPower:ʵʱ�����µ���������TRUE,���µ�����Ϊ0xee\n");
    	    }

      		return TRUE;
      	}
      }
      else
      {
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
    	    printf("computeInTimeLeftPower:ʵʱ�����µ���������FALSE\n");
    	  }

      	return TRUE;
      }
    }
    else
    {
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("computeInTimeLeftPower:δ����ʣ�����\n");
    	}
    }
    
    return FALSE;
}

/*******************************************************
��������: cLoopEvent
��������: ������·�쳣�¼���¼
���ú���:     
�����ú���:
�������:phase,��(0x01-A, 0x02-B, 0x04-C)
         pn,������
         whichLimit,��Խ��(1-��·,2-��·,3-����)
         recovery,�¼��������ǻָ�
�������:  
����ֵ��
*******************************************************/
void cLoopEvent(INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime)
{
  INT8U  eventData[50];
  INT8U  dataTail;
  eventData[0] = 9;

  eventData[2] = statisTime.second;
  eventData[3] = statisTime.minute;
  eventData[4] = statisTime.hour;
  eventData[5] = statisTime.day;
  eventData[6] = statisTime.month;
  eventData[7] = statisTime.year;
  
  eventData[8] = pn&0xff;
  eventData[9] = (pn>>8&0xff) | recovery;
  eventData[10] = whichLimit<<6 | phase;
  dataTail = 11;
 
  //����ʱ��Ua/Uab
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A];
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A+1];

  //����ʱ��Ub
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B];
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B+1];

  //����ʱ��Uc/Ucb
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C];
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C+1];
  
  //����ʱ��Ia
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+1];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+2];

  //����ʱ��Ib
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+1];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+2];
  
  //����ʱ��Ic
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+1];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];
  
  //����ʱ���ܱ������й��ܵ���ʾֵ
  if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]!=0xee)
  {
  	eventData[dataTail++] = 0x0;
  }
  else
  {
  	eventData[dataTail++] = 0x0;
  }
  eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
  eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
  eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
  eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];
  eventData[1] = dataTail;

  if (eventRecordConfig.iEvent[1]&0x01)
  {
    writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);
  }
  
  if (eventRecordConfig.nEvent[1]&0x01)
  {
    writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);
  }
  
  eventStatus[1] = eventStatus[1] | 0x01;
}

/*******************************************************
��������: cOverLimitEvent
��������: ����Խ���¼���¼
���ú���:     
�����ú���:
�������:��(0x01, 0x02, 0x04 a, b, c)
         ������
         ��Խ��
         �Ƿ��¼��ָ�
�������:  
����ֵ��
*******************************************************/
void cOverLimitEvent(INT8U *pCopyParaBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime)
{          
    INT8U  eventData[25];
    INT8U  dataTail;
    
    //��¼����Խ���¼�
    eventData[0] = 25;
    
    eventData[2] = statisTime.minute;
    eventData[3] = statisTime.hour;
    eventData[4] = statisTime.day;
    eventData[5] = statisTime.month;
    eventData[6] = statisTime.year;
    
    dataTail = 7;
    
    eventData[dataTail++] = pn&0xff;   //�������8λ    
    eventData[dataTail] = (pn>>8)&0xf;    

    if (recovery == FALSE)
    {
      eventData[dataTail] |= 0x80;  //�¼�����
    }
    dataTail++;
    
    eventData[dataTail] = 1<<(phase-1);   //��
    
	 #ifdef LIGHTING	
    if (whichLimit == 0) //Խ������
    {
    	eventData[dataTail] |= 0x00;
    }
	 #endif
    if (whichLimit == 2) //Խ������
    {
      eventData[dataTail] |= 0x40;
    }
    if (whichLimit == 1) //Խ����
    {
    	eventData[dataTail] |= 0x80;
    }
    dataTail++;
    
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+1];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+2];
    
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+1];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];
    
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+1];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];

    eventData[1] = dataTail;
    
    if (eventRecordConfig.iEvent[3] & 0x01)
    {
 	     writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
 	  }
    if (eventRecordConfig.nEvent[3] & 0x01)
    {
 	     writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //����һ���¼�����
 	  }
 	  
 	  eventStatus[3] = eventStatus[3] | 0x01;  
}

/*******************************************************
��������: vOverLimitEvent
��������: ��ѹԽ���¼���¼
���ú���:     
�����ú���:
�������:��(0x01, 0x02, 0x04 a, b, c)
         ������
         ��Խ��
         �Ƿ��¼��ָ�
�������:  
����ֵ��
*******************************************************/	    
void vOverLimitEvent(INT8U *pCopyParaBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime)
{  	    
    INT8U eventData[20];
    INT8U dataTail;
    
    eventData[0] = 24;
    eventData[1] = 15;
    
    eventData[2] = statisTime.minute;
    eventData[3] = statisTime.hour;
    eventData[4] = statisTime.day;
    eventData[5] = statisTime.month;
    eventData[6] = statisTime.year;
    
    dataTail = 7;
    
    eventData[dataTail++] = pn&0xff;   //�������8λ    
    eventData[dataTail] = (pn>>8)&0xf;    
    
    if (recovery == FALSE)
    {
      eventData[dataTail] |= 0x80;
    }    
    dataTail++;

    eventData[dataTail] = 1<<(phase-1);
    
    if (whichLimit == 2) //Խ������
    {
      eventData[dataTail] |= 0x40;
    }
    if (whichLimit == 1) //Խ������
    {
    	eventData[dataTail] |= 0x80;
    }
    dataTail++;
   
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A];
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A+1];
    
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B];
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B+1];
    
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C];
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C+1];
    
    eventData[1] = dataTail;
    
    if (eventRecordConfig.iEvent[2] & 0x80)
    {
       writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
    }
    if (eventRecordConfig.nEvent[2] & 0X80)
    {
       writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //����һ���¼�����
    }
    
    eventStatus[2] = eventStatus[2] | 0x80;
}

/*******************************************************
��������: vAbnormalEvent
��������: ��ѹ��·�쳣�¼���¼
���ú���:     
�����ú���:
�������:��(0x01, 0x02, 0x04 a, b, c)
         ������
         ��Խ��
         �Ƿ��¼��ָ�
�������:  
����ֵ��
*******************************************************/
void vAbnormalEvent(INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U phase, INT16U pn, INT8U type, BOOL recovery, DATE_TIME statisTime)
{
	  INT8U  eventData[40];
	  INT8U  dataTail;
	  
    eventData[0] = 10;
    	
    eventData[2] = statisTime.second;
    eventData[3] = statisTime.minute;
    eventData[4] = statisTime.hour;
    eventData[5] = statisTime.day;
    eventData[6] = statisTime.month;
    eventData[7] = statisTime.year;
    
    dataTail = 8;
    eventData[dataTail++] = pn&0xff;
    eventData[dataTail] = pn>>8&0xf;

    if (recovery == FALSE)
    {
      eventData[dataTail] |= 0x80;
    }
    dataTail++;
    	
    eventData[dataTail] = 1<<(phase-1);
    if (type == 0x01)
    {
      eventData[dataTail] |= 0x40;
    }
    if (type == 0x02)
    {
      eventData[dataTail] |= 0x80;
    }
    dataTail++;
    	
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A];
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A+1];
    
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B];
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B+1];
    
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C];
    eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C+1];
    
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+1];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+2];
    
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+1];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+2];
    
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+1];
    eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];
    
    eventData[dataTail] = pCopyParaBuff[POSITIVE_WORK_OFFSET];    
    if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET] != 0xEE)
    {
      eventData[dataTail] = 0x00;
    }
    else
    {
	    eventData[dataTail] = 0xEE;
    }
    dataTail++;
    
    eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
    eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
    eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
    eventData[dataTail++] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];

    eventData[1] = dataTail;
      
    if (eventRecordConfig.iEvent[1] & 0x02)
    {
      writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
    }
    if (eventRecordConfig.nEvent[1] & 0x02)
    {
   	  writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //����һ���¼�����
   	}
   	  
   	eventStatus[1] = eventStatus[1] | 0x02;
}

/*******************************************************
��������:pOverLimitEvent
��������:���ڹ���Խ���¼�
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
void pOverLimitEvent(INT16U pn, INT8U type, BOOL recovery, INT8U *data, void *pLimit,DATE_TIME statisTime)
{
   MEASUREPOINT_LIMIT_PARA *pMpLimit;

  #ifndef LIGHTING
   pMpLimit = (MEASUREPOINT_LIMIT_PARA *)pLimit;
  #else
   PN_LIMIT_PARA           *pPnLimit;
		
   if (type<3)
   {
	 pMpLimit = (MEASUREPOINT_LIMIT_PARA *)pLimit;
   }
   else
   {
	 pPnLimit = (PN_LIMIT_PARA *)pLimit;
   }
  #endif
	
   INT8U eventData[16];
   INT8U dataTail;
		
    eventData[0] = 26;
  	
  	eventData[2] = statisTime.second;
  	eventData[3] = statisTime.minute;
  	eventData[4] = statisTime.hour;
  	eventData[5] = statisTime.day;
  	eventData[6] = statisTime.month;
  	eventData[7] = statisTime.year;

    dataTail = 8;
    
    eventData[dataTail++] = pn&0xff;   //�������8λ    
    eventData[dataTail] = (pn>>8)&0xf;    
  	
  	if (recovery == FALSE)
  	{
  	  eventData[dataTail] |= 0x80;
  	}
  	dataTail++;
  	
  	if (type == 0x00)  //Խ����
  	{
  	  eventData[dataTail] = 0x00;
  	}

  	//Խ������
	if (type == 0x01)
  	{
  	  eventData[dataTail] = 0x40;
  	}
  	if (
        type == 0x02
		 || type==0x03
	   )  //Խ����
  	{
  	  eventData[dataTail] = 0x80;
  	}
  	dataTail++;
  	
  	//��ǰ���ڹ���
  	eventData[dataTail++] = *data;
  	eventData[dataTail++] = *(data+1);
  	eventData[dataTail++] = *(data+2);
  	
  	if (type == 0x01)
  	{
  	  eventData[dataTail++] = pMpLimit->pSuperiodLimit[0];
  	  eventData[dataTail++] = pMpLimit->pSuperiodLimit[1];
  	  eventData[dataTail++] = pMpLimit->pSuperiodLimit[2];
  	}
  	if (type == 0x02)
  	{
  	  eventData[dataTail++] = pMpLimit->pUpLimit[0];
  	  eventData[dataTail++] = pMpLimit->pUpLimit[1];
  	  eventData[dataTail++] = pMpLimit->pUpLimit[2];
  	}

   #ifdef LIGHTING
	if (type == 0x03)
  	{
  	  eventData[dataTail++] = pPnLimit->pUpLimit[0];
  	  eventData[dataTail++] = pPnLimit->pUpLimit[1];
  	  eventData[dataTail++] = pPnLimit->pUpLimit[2];
  	}
  	if (type == 0x00)
  	{
  	  eventData[dataTail++] = pPnLimit->pDownLimit[0];
  	  eventData[dataTail++] = pPnLimit->pDownLimit[1];
  	  eventData[dataTail++] = pPnLimit->pDownLimit[2];
  	}
   #endif
    
    eventData[1] = dataTail;

    if (eventRecordConfig.iEvent[3] & 0x02)
    {
       writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
    }
    if (eventRecordConfig.nEvent[3] & 0x02)
    {
 	  writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //����һ���¼�����
 	}
 	  
 	eventStatus[3] = eventStatus[3] | 0x02;
}

/*******************************************************
��������:changeEvent
��������:�������������������Ϣ�¼�
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
void changeEvent(INT16U pn, INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, DATE_TIME statisTime,INT8U protocol, INT8U statisInterval)
{
    INT16U    i;
    INT8U     eventData[40];
    INT8U     lastLastCopyPara[LENGTH_OF_PARA_RECORD];
    INT8U     copyShiDuanData[LENGTH_OF_SHIDUAN_RECORD];
    INT8U     lastCopyShiDuanData[LENGTH_OF_SHIDUAN_RECORD];
    DATE_TIME readTime;
    BOOL      ifChanged;
    
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("�жϲ�����%d���ܱ�����������¼�.ͳ��ʱ��=%02x-%02x-%02x %02x:%02x:%02x\n",pn,statisTime.year,statisTime.month,statisTime.day,statisTime.hour,statisTime.minute,statisTime.second);
    }
    
    readTime = statisTime;
    if (readMeterData(lastLastCopyPara , pn, LAST_LAST_REAL_DATA, PARA_VARIABLE_DATA, &readTime, statisInterval) == TRUE)
    {
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("��һ�γ���ʱ��=%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
      }

      //----------ERC09(������·�쳣[����])----------
      if ((eventRecordConfig.iEvent[1]&0x01)||(eventRecordConfig.nEvent[1] & 0x01))
      {
        //�������״̬��2
        if (pCopyParaBuff[METER_STATUS_WORD_2] != 0xEE)
        {
  		    //A���й����ʷ���
  		    if (pCopyParaBuff[METER_STATUS_WORD_2]&0x01)
  		    {
            //A�෴����δ��¼�¼�,��¼
	    	    if ((pStatisRecord->currentLoop&0x1)==0x00)
	    	    {
              pStatisRecord->currentLoop |= 0x01;   //�÷����־
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x01, pn, 0x03, 0x80, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,A���й����ʷ�������δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,A���й����ʷ����Ѽ�¼\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //A��δ��������,�������������������,��Ӧ��¼�ָ��¼�
	    	    if ((pStatisRecord->currentLoop&0x1)==0x01)
	    	    {
              pStatisRecord->currentLoop &= 0xfe;     //��������־
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x01, pn, 0x03, 0, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,A���й����ʷ���ָ���δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,A������\n",pn);
              }
	    	    }
  		    }
  		    
  		    //B���й����ʷ���
  		    if (pCopyParaBuff[METER_STATUS_WORD_2]&0x02)
  		    {
            //B�෴����δ��¼�¼�,��¼
	    	    if ((pStatisRecord->currentLoop&0x2)==0x00)
	    	    {
              pStatisRecord->currentLoop |= 0x02;   //�÷����־
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x02, pn, 0x03, 0x80, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,B���й����ʷ�������δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,B���й����ʷ����Ѽ�¼\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //B��δ��������,�������������������,��Ӧ��¼�ָ��¼�
	    	    if ((pStatisRecord->currentLoop&0x2)==0x02)
	    	    {
              pStatisRecord->currentLoop &= 0xfd;     //��������־
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x02, pn, 0x03, 0, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,B���й����ʷ���ָ���δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,B������\n",pn);
              }
	    	    }
  		    }
  		    
  		    //C���й����ʷ���
  		    if (pCopyParaBuff[METER_STATUS_WORD_2]&0x04)
  		    {
            //C�෴����δ��¼�¼�,��¼
	    	    if ((pStatisRecord->currentLoop&0x4)==0x00)
	    	    {
              pStatisRecord->currentLoop |= 0x04;   //�÷����־
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x04, pn, 0x03, 0x80, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,C���й����ʷ�������δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,C���й����ʷ����Ѽ�¼\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //C��δ��������,�������������������,��Ӧ��¼�ָ��¼�
	    	    if ((pStatisRecord->currentLoop&0x4)==0x04)
	    	    {
              pStatisRecord->currentLoop &= 0xfb;     //��������־
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x04, pn, 0x03, 0, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,C���й����ʷ���ָ���δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("������·�쳣:������%d,C������\n",pn);
              }
	    	    }
  		    }
        }
      }
      
      //----------ERC11(�����쳣)----------
      if ((eventRecordConfig.iEvent[1]&0x04)||(eventRecordConfig.nEvent[1] & 0x04))
      {
        //�������״̬��2
        if (pCopyParaBuff[METER_STATUS_WORD_7] != 0xEE)
        {
  		    eventData[9] = 0x0;
  		    
  		    //�����쳣?
  		    if ((pCopyParaBuff[METER_STATUS_WORD_7]&0x01) || (pCopyParaBuff[METER_STATUS_WORD_7]&0x02))
  		    {
            //��ѹ������������������δ��¼�¼�,��¼
	    	    if ((pStatisRecord->mixed&0x4)==0x00)
	    	    {
              pStatisRecord->mixed |= 0x04;         //�������쳣��־
              eventData[9] = 0x1;

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("�����쳣:������%d,�����쳣������δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("�����쳣:������%d�����쳣���Ѽ�¼\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //��ѹ����������������,�������������������,��Ӧ��¼�ָ��¼�
	    	    if ((pStatisRecord->mixed&0x4)==0x04)
	    	    {
              pStatisRecord->mixed &= 0xfb;   //��������쳣��־

              eventData[9] = 0x2;

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("�����쳣:������%d,�����쳣�ָ���δ��¼,��¼�¼�\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("�����쳣:������%d,��������\n",pn);
              }
	    	    }
  		    }
  		    
  		    if (eventData[9]==0x1 || eventData[9]==0x02)
  		    {
        	  eventData[0] = 11;
        	  eventData[1] = 27;
        	
        	  eventData[2] = statisTime.second;
        	  eventData[3] = statisTime.minute;
        	  eventData[4] = statisTime.hour;
        	  eventData[5] = statisTime.day;
        	  eventData[6] = statisTime.month;
        	  eventData[7] = statisTime.year;
        	  
        	  eventData[8] = pn&0xff;
        	  
        	  if (eventData[9]==1)
        	  {
      	      eventData[9] = (pn>>8&0xff) | 0x80;      	      
        	  }
        	  else
        	  {
        	  	eventData[9] = (pn>>8&0xff);
        	  }
        	  eventData[10] = 0xee;
        	  eventData[11] = 0xee;
        	  eventData[12] = 0xee;
        	  eventData[13] = 0xee;
        	  eventData[14] = 0xee;
        	  eventData[15] = 0xee;
        	  eventData[16] = 0xee;
        	  eventData[17] = 0xee;
        	  eventData[18] = 0xee;
        	  eventData[19] = 0xee;
        	  eventData[20] = 0xee;
        	  eventData[21] = 0xee;
            
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)
            {
            	 eventData[22] = 0xee;
            }
            else
            {
            	 eventData[22] = 0x0;
            }
            eventData[23] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET];
            eventData[24] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1];
            eventData[25] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2];
            eventData[26] = pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3];

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
      }
            
      //----------ERC13(���ܱ������Ϣ)----------
      if ((eventRecordConfig.iEvent[1]&0x10)||(eventRecordConfig.nEvent[1]&0x10))
      {
        eventData[9] = 0x00;
        
        //��������仯
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	printf("�жϵ��ܱ����:���ζ������=%02x%02x%02x\n",pCopyParaBuff[PHASE_DOWN_TIMES+2],pCopyParaBuff[PHASE_DOWN_TIMES+1],pCopyParaBuff[PHASE_DOWN_TIMES]);
        	printf("�жϵ��ܱ����:�ϴζ������=%02x%02x%02x\n",lastLastCopyPara[PHASE_DOWN_TIMES+2],lastLastCopyPara[PHASE_DOWN_TIMES+1],lastLastCopyPara[PHASE_DOWN_TIMES]);
        }
        if ((pCopyParaBuff[PHASE_DOWN_TIMES] != lastLastCopyPara[PHASE_DOWN_TIMES] || pCopyParaBuff[PHASE_DOWN_TIMES+1] != lastLastCopyPara[PHASE_DOWN_TIMES+1] || pCopyParaBuff[PHASE_DOWN_TIMES+2] != lastLastCopyPara[PHASE_DOWN_TIMES+2])
       	  && pCopyParaBuff[PHASE_DOWN_TIMES] !=0xEE && lastLastCopyPara[PHASE_DOWN_TIMES] != 0xEE)
        {
        	eventData[9] |= 0x02;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("���ܱ����:������%d��������仯\n",pn);
          }
        }
                
        //���������������仯
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	printf("�жϵ��ܱ����:������������������=%02x%02x%02x\n",pCopyParaBuff[UPDATA_REQ_TIME+2],pCopyParaBuff[UPDATA_REQ_TIME+1],pCopyParaBuff[UPDATA_REQ_TIME]);
        	printf("�жϵ��ܱ����:�ϴ���������������=%02x%02x%02x\n",lastLastCopyPara[UPDATA_REQ_TIME+2],lastLastCopyPara[UPDATA_REQ_TIME+1],lastLastCopyPara[UPDATA_REQ_TIME]);
        }
        if ((pCopyParaBuff[UPDATA_REQ_TIME] != lastLastCopyPara[UPDATA_REQ_TIME] || pCopyParaBuff[UPDATA_REQ_TIME+1] != lastLastCopyPara[UPDATA_REQ_TIME+1]|| pCopyParaBuff[UPDATA_REQ_TIME+2] != lastLastCopyPara[UPDATA_REQ_TIME+2])
      	    && pCopyParaBuff[UPDATA_REQ_TIME] != 0xEE && lastLastCopyPara[UPDATA_REQ_TIME] != 0xEE)
      	{
      	  eventData[9] |= 0x01;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("���ܱ����:������%d���������������仯\n",pn);
          }
      	}
      	
      	//��̴����仯
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	printf("�жϵ��ܱ����:���α�̴���=%02x%02x%02x\n",pCopyParaBuff[PROGRAM_TIMES+2],pCopyParaBuff[PROGRAM_TIMES+1],pCopyParaBuff[PROGRAM_TIMES]);
        	printf("�жϵ��ܱ����:�ϴα�̴���=%02x%02x%02x\n",lastLastCopyPara[PROGRAM_TIMES+2],lastLastCopyPara[PROGRAM_TIMES+1],lastLastCopyPara[PROGRAM_TIMES]);
        }
      	if ((pCopyParaBuff[PROGRAM_TIMES] != lastLastCopyPara[PROGRAM_TIMES] || pCopyParaBuff[PROGRAM_TIMES+1] != lastLastCopyPara[PROGRAM_TIMES+1]|| pCopyParaBuff[PROGRAM_TIMES+2] != lastLastCopyPara[PROGRAM_TIMES+2])
          	&& pCopyParaBuff[PROGRAM_TIMES] != 0xEE && lastLastCopyPara[PROGRAM_TIMES] != 0xEE)
        {
          eventData[9] |= 0x01;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("���ܱ����:������%d��̴����仯\n",pn);
          }
        }
        
        //��¼��������Ϣ(ERC13)
        //���¼���������¼���ܱ��������¼�
        if (eventData[9] != 0)
        {
      	  eventData[0] = 13;
      	  eventData[1] = 11;
      	
      	  eventData[2] = statisTime.second;
      	  eventData[3] = statisTime.minute;
      	  eventData[4] = statisTime.hour;
      	  eventData[5] = statisTime.day;
      	  eventData[6] = statisTime.month;
      	  eventData[7] = statisTime.year;
      	  
      	  eventData[8] = pn&0xff;
      	  eventData[10] = eventData[9];   //ly,2011-08-03,�������������̨�����ͨ��,�Ȿ���Ǹ�����
      	  eventData[9] = (pn>>8&0xff) | 0x80;
      	  
      	  if (eventRecordConfig.iEvent[1]&0x10)
      	  {
      	    writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
      	  }
      	  
      	  if (eventRecordConfig.nEvent[1]&0x10)
      	  {
      	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
      	  }
      	  
      	  eventStatus[1] = eventStatus[1] | 0x10;
      	}
      	
      	
        //���Ƿѹ
        //ly,2011-10-18,���ע��,���ֲ���̨����ĵ��״̬�־���0xee
        //ly,2011-10-20,���ֽ��ɲ��Ե��Ƿѹ����ֽ�����0xee,ҪӰ���̴�����,����Ӷ������������ee������
        //ly,2012-02-04,�����õ���07��Ķ��������Ϊ0xee,���ǵ��״̬��Ҳ������0xee,�����ö���������ж�Ҳ��������
        //              v1.63����ǰ�İ汾��07������ж�Ϊ���Ƿѹ,���Ǹ�����
        //       �޸�Ϊ���״̬�ֵ�1���ֽڵ�Bit7,Bit6Ϊ������,�������λ��0�Ļ�,˵���������ĵ��״̬��,��Ϊ0xee���Բ������������
        //if (pCopyEnergyBuff[PHASE_DOWN_TIMES] != 0xEE)
        //{
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	 printf("�жϵ��ܱ����:���״̬��=%02x\n",pCopyParaBuff[METER_STATUS_WORD]);
        }

      	eventData[9] = 0x0;
  		  if ((pCopyParaBuff[METER_STATUS_WORD]&0xc0)==0x00)
  		  {
  		    if (pCopyParaBuff[METER_STATUS_WORD]&0x04)
  		    {
	    	    //2013-11-21,�������ж�
	    	    if (0x0==(pStatisRecord->mixed&0x08))
	    	    {
	    	    	pStatisRecord->mixed |= 0x08;
	    	      
	    	      eventData[9] |= 0x1;
            
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("���ܱ����:������%d��ѹǷѹ\n",pn);
              }
            }
  		    }
  		    else    //���Ƿѹ�ָ�
  		    {
	    	    if (pStatisRecord->mixed&0x08)
	    	    {
	    	    	pStatisRecord->mixed &= 0xf7;

	    	      eventData[9] |= 0x2;
            
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("���ܱ����:������%d��ѹǷѹ�ָ�\n",pn);
              }
            }
  		    }
        }
        
        //��¼��������Ϣ(ERC13)-���Ƿѹ
        //���¼���������¼���ܱ��������¼�
        if (1==eventData[9] || 2==eventData[9])
        {
      	  eventData[0] = 13;
      	  eventData[1] = 11;
      	
      	  eventData[2] = statisTime.second;
      	  eventData[3] = statisTime.minute;
      	  eventData[4] = statisTime.hour;
      	  eventData[5] = statisTime.day;
      	  eventData[6] = statisTime.month;
      	  eventData[7] = statisTime.year;
      	  
      	  eventData[8] = pn&0xff;
      	  if (eventData[9]==0x1)    //����
      	  {
      	    eventData[9] = (pn>>8&0xff) | 0x80;
      	  }
      	  else                      //�ָ�
      	  {
      	    eventData[9] = (pn>>8&0xff);
      	  }
      	  eventData[10] = 0x10;
      	  
      	  if (eventRecordConfig.iEvent[1]&0x10)
      	  {
      	    writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
      	  }
      	  
      	  if (eventRecordConfig.nEvent[1]&0x10)
      	  {
      	    writeEvent(eventData, 11, 2, DATA_FROM_LOCAL);
      	  }
      	  
      	  eventStatus[1] = eventStatus[1] | 0x10;
      	}
      }
      
      //----------ERC08----------
      if ((eventRecordConfig.iEvent[0]&0x80)||(eventRecordConfig.nEvent[0] & 0x80))
      {
      	  eventData[9] = 0;
      	  
          //��������
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("�жϵ��ܱ�������:���ε����=%02x%02x%02x\n",pCopyParaBuff[CONSTANT_WORK+2],pCopyParaBuff[CONSTANT_WORK+1],pCopyParaBuff[CONSTANT_WORK]);
        	  printf("�жϵ��ܱ�������:�ϴε����=%02x%02x%02x\n",lastLastCopyPara[CONSTANT_WORK+2],lastLastCopyPara[CONSTANT_WORK+1],lastLastCopyPara[CONSTANT_WORK]);
          }
          if ((pCopyParaBuff[CONSTANT_WORK] != lastLastCopyPara[CONSTANT_WORK] || pCopyParaBuff[CONSTANT_WORK+1] != lastLastCopyPara[CONSTANT_WORK+1] || pCopyParaBuff[CONSTANT_WORK+2] != lastLastCopyPara[CONSTANT_WORK+2])
          	  && (pCopyParaBuff[CONSTANT_WORK] != 0xEE && lastLastCopyPara[CONSTANT_WORK] != 0xEE))
           //||((readBuff[CONSTANT_NO_WORK] != lastCopyParaData[CONSTANT_NO_WORK] || readBuff[CONSTANT_NO_WORK+1] != lastCopyParaData[CONSTANT_NO_WORK+1] || readBuff[CONSTANT_NO_WORK+2] != lastCopyParaData[CONSTANT_NO_WORK+2])
           //	  && (readBuff[CONSTANT_NO_WORK] != 0xEE && lastCopyParaData[CONSTANT_NO_WORK] != 0xEE)))
          {
             //��¼���������¼�
             eventData[9] |= 0x08;  //D3
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("���ܱ�������:������%d��������\n",pn);
             }
          }
          
          //�����ձ���¼�
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("�жϵ��ܱ�������:���γ�����=%02x%02x\n",pCopyParaBuff[AUTO_COPY_DAY+1],pCopyParaBuff[AUTO_COPY_DAY]);
        	  printf("�жϵ��ܱ�������:�ϴγ�����=%02x%02x\n",lastLastCopyPara[AUTO_COPY_DAY+1],lastLastCopyPara[AUTO_COPY_DAY]);
          }
          if ((pCopyParaBuff[AUTO_COPY_DAY] != lastLastCopyPara[AUTO_COPY_DAY] || pCopyParaBuff[AUTO_COPY_DAY+1] != lastLastCopyPara[AUTO_COPY_DAY+1])
          	&& pCopyParaBuff[AUTO_COPY_DAY] != 0xEE && lastLastCopyPara[AUTO_COPY_DAY] != 0xEE)
          {
             //��¼�����ձ���¼�
             eventData[9] |= 0x04;  //D2
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("���ܱ�������:������%d�����ձ��\n",pn);
             }
          }
          
          //���ʱ��ı�
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("�жϵ��ܱ�������:���α��ʱ��=%02x%02x%02x%02x\n",pCopyParaBuff[LAST_PROGRAM_TIME+3],pCopyParaBuff[LAST_PROGRAM_TIME+2],pCopyParaBuff[LAST_PROGRAM_TIME+1],pCopyParaBuff[LAST_PROGRAM_TIME]);
        	  printf("�жϵ��ܱ�������:�ϴα��ʱ��=%02x%02x%02x%02x\n",lastLastCopyPara[LAST_PROGRAM_TIME+3],lastLastCopyPara[LAST_PROGRAM_TIME+2],lastLastCopyPara[LAST_PROGRAM_TIME+1],lastLastCopyPara[LAST_PROGRAM_TIME]);
          }
          if ((pCopyParaBuff[LAST_PROGRAM_TIME] != lastLastCopyPara[LAST_PROGRAM_TIME] || pCopyParaBuff[LAST_PROGRAM_TIME+1] != lastLastCopyPara[LAST_PROGRAM_TIME+1]
          	 || pCopyParaBuff[LAST_PROGRAM_TIME+2] != lastLastCopyPara[LAST_PROGRAM_TIME+2] || pCopyParaBuff[LAST_PROGRAM_TIME+3] != lastLastCopyPara[LAST_PROGRAM_TIME+3])
          	&& pCopyParaBuff[LAST_PROGRAM_TIME] != 0xEE && lastLastCopyPara[LAST_PROGRAM_TIME] != 0xEE)
          {
             //��¼���ʱ��ı��¼�
             eventData[9] |= 0x02;
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("���ܱ�������:������%d���ʱ����\n",pn);
             }
          }
          
          //�����������
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("�жϵ��ܱ�������:������������=%02x%02x%02x\n",pCopyParaBuff[UPDATA_REQ_TIME+2],pCopyParaBuff[UPDATA_REQ_TIME+1],pCopyParaBuff[UPDATA_REQ_TIME]);
        	  printf("�жϵ��ܱ�������:�ϴ���������=%02x%02x%02x\n",lastLastCopyPara[UPDATA_REQ_TIME+2],lastLastCopyPara[UPDATA_REQ_TIME+1],lastLastCopyPara[UPDATA_REQ_TIME]);
          }
          if ((pCopyParaBuff[UPDATA_REQ_TIME] != lastLastCopyPara[UPDATA_REQ_TIME] || pCopyParaBuff[UPDATA_REQ_TIME+1] != lastLastCopyPara[UPDATA_REQ_TIME+1]|| pCopyParaBuff[UPDATA_REQ_TIME+2] != lastLastCopyPara[UPDATA_REQ_TIME+2])
      	    && pCopyParaBuff[UPDATA_REQ_TIME] != 0xEE && lastLastCopyPara[UPDATA_REQ_TIME] != 0xEE)
          {
             //��¼�����������
             eventData[9] |= 0x20;
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("���ܱ�������:������%d�����������仯\n",pn);
             }
          }
          
          //�������γ����ʱ��
          readTime = statisTime;
          if (readMeterData(copyShiDuanData, pn, PRESENT_DATA, SHI_DUAN_DATA, &readTime, 0) == TRUE)
          {
        	  readTime = statisTime;
        	  if (readMeterData(lastCopyShiDuanData, pn, LAST_LAST_REAL_DATA, SHI_DUAN_DATA, &readTime, statisInterval) == TRUE)
        	  {
        	  	for (i = 0; i < LENGTH_OF_SHIDUAN_RECORD; i++)
        	  	{
        	  	 	if (copyShiDuanData[i]!=0xee && lastCopyShiDuanData[i]!=0xee)
        	  	 	{
        	  	 	   if (copyShiDuanData[i] != lastCopyShiDuanData[i])
        	  	 	   {
        	  	 	      eventData[9] |= 0x01;  //D0
                     
                      if (debugInfo&PRINT_BALANCE_DEBUG)
                      {
                       printf("���ܱ�������:������%d����ʱ�α��\n",pn);
                      }
        	  	 	      break;
        	  	 	   }
        	  	 	}
        	  	}
        	  }
          }

          //���¼���������¼���ܱ��������¼�(ERC8)
          if (eventData[9] != 0)
          {
              eventData[0] = 0x08;
              eventData[1] = 11;

              eventData[2] = statisTime.second;
              eventData[3] = statisTime.minute;
              eventData[4] = statisTime.hour;
              eventData[5] = statisTime.day;
              eventData[6] = statisTime.month;
              eventData[7] = statisTime.year;
              
              eventData[8] = pn&0xff;
              eventData[10] = eventData[9];
              eventData[9] = pn>>8&0xff;
              
              if (eventRecordConfig.iEvent[0]&0x80)
              {
                writeEvent(eventData, 11, 1, DATA_FROM_GPRS);
              }
              if (eventRecordConfig.nEvent[0]&0x80)
              {
                writeEvent(eventData, 11, 2, DATA_FROM_GPRS);
              }
              
              eventStatus[0] = eventStatus[0] | 0x80;
          }
      }
      
      //----------ERC33----------
      if ((eventRecordConfig.iEvent[4]&0x01)||(eventRecordConfig.nEvent[4]&0x01))
      {
        if (protocol==DLT_645_2007)
        {
          meterRunWordChangeBit(&eventData[10],&pCopyParaBuff[METER_STATUS_WORD], &lastLastCopyPara[METER_STATUS_WORD]);
          ifChanged = FALSE;
          for (i=0;i<14;i++)
          {
          	if (eventData[10+i]!=0)
          	{
          	 	ifChanged = TRUE;
          	}
          }
          
          //��¼���ܱ�����״̬�ֱ�λ�¼���¼(ERC33)
          //���¼���������¼
          if (ifChanged==TRUE)
          {
        	  eventData[0] = 33;
        	  eventData[1] = 38;
        	
        	  eventData[2] = statisTime.second;
        	  eventData[3] = statisTime.minute;
        	  eventData[4] = statisTime.hour;
        	  eventData[5] = statisTime.day;
        	  eventData[6] = statisTime.month;
        	  eventData[7] = statisTime.year;
        	  
        	  eventData[8] = pn&0xff;
        	  eventData[9] = pn>>8&0xff;
        	  
        	  for(i=0;i<14;i++)
        	  {
        	  	 eventData[24+i] = pCopyParaBuff[METER_STATUS_WORD+i];
        	  }
        	  
        	  if (eventRecordConfig.iEvent[4]&0x01)
        	  {
        	    writeEvent(eventData, 38, 1, DATA_FROM_GPRS);
        	  }
        	  
        	  if (eventRecordConfig.nEvent[4]&0x01)
        	  {
        	    writeEvent(eventData, 38, 2, DATA_FROM_LOCAL);
        	  }
        	  
        	  eventStatus[4] = eventStatus[4] | 0x01;
        	}
      	}
      }       
    }
}


/*******************************************************
��������:
��������:
���ú���:     
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
void freeMpLink(struct cpAddrLink *linkHead)
{
   struct cpAddrLink *tmpNode;
	 
	 tmpNode = linkHead;
   while(linkHead !=NULL)
   {
     	tmpNode = linkHead;
    	linkHead = linkHead->next;
    	free(tmpNode);
   }
   
   return;
}



//#endif

/*******************************************************
��������: calcResumeLimit
��������: ����ָ���ֵ
���ú���:     
�����ú���:
�������:

�������:
����ֵ�� 
*******************************************************/
INT32U calcResumeLimit(INT32U limit, INT16U factor)
{
   INT32U limitValue,tmpFactor;
   
   limitValue = bcdToHex(limit);
   
   tmpFactor = bcdToHex(factor&0x7fff);
       
   if (factor&0x8000)
   {
   	 tmpFactor = 1000-tmpFactor;
   }
   else
   {
   	 tmpFactor += 1000;
   }
   limitValue = limitValue*tmpFactor/1000;
   
   return limitValue;
}

/*******************************************************
��������: processOverLimit
��������: ����Խ��
���ú���:     
�����ú���:
�������:

�������:
����ֵ�� 
*******************************************************/
void processOverLimit(INT8U port)
{
  struct cpAddrLink        *cpLinkHead, *tmpNode;
  MEASUREPOINT_LIMIT_PARA  *pMpLimitValue;                              //��������ֵ����ָ��
  METER_STATIS_EXTRAN_TIME meterStatisRecord, bakStatisRec;             //һ����ͳ���¼�����
  INT8U                    lastCopyEnergyData[LENGTH_OF_ENERGY_RECORD]; //��һ�γ������������
  INT8U                    lastCopyParaData[LENGTH_OF_PARA_RECORD];     //��һ�γ������,�α�������
  INT8U                    balanceParaData[LEN_OF_PARA_BALANCE_RECORD]; //�����òα�������
  DATE_TIME                tmpTime;

 	if ((cpLinkHead=initPortMeterLink(port))!=NULL)
 	{
     //Ϊ��������ֵ����洢�ռ�
     pMpLimitValue = (MEASUREPOINT_LIMIT_PARA *)malloc(sizeof(MEASUREPOINT_LIMIT_PARA));

 		 tmpNode = cpLinkHead;
 		 while(tmpNode!=NULL)
 		 {
 		 	 //if (tmpNode->protocol!=AC_SAMPLE)
 		 	 //{
         if(selectViceParameter(0x04, 26, tmpNode->mp, (INT8U *)pMpLimitValue, sizeof(MEASUREPOINT_LIMIT_PARA)) == FALSE)
         {
	         pMpLimitValue = NULL;
         }
         
         if (pMpLimitValue!=NULL)
         {
         	 if (debugInfo&PRINT_BALANCE_DEBUG)
         	 {
         	 	  printf("processOverLimit-����Խ�޳���ʱ�䵽��:������%d��������ֵ����\n",tmpNode->mp);
         	 }
         	 	  
           //�������ͳ�Ƽ�¼
           tmpTime = timeBcdToHex(copyCtrl[port].lastCopyTime);
           searchMpStatis(tmpTime, &meterStatisRecord, tmpNode->mp, 1);  //��ʱ���޹���
           bakStatisRec = meterStatisRecord;
           
           if (meterStatisRecord.vUpUpTime[0].year!=0xff || meterStatisRecord.vUpUpTime[1].year!=0xff ||  meterStatisRecord.vUpUpTime[2].year!=0xff   
               || meterStatisRecord.vDownDownTime[0].year!=0xff || meterStatisRecord.vDownDownTime[1].year!=0xff ||  meterStatisRecord.vDownDownTime[2].year!=0xff
                || meterStatisRecord.cUpTime[0].year!=0xff || meterStatisRecord.cUpTime[1].year!=0xff ||  meterStatisRecord.cUpTime[2].year!=0xff 
                 || meterStatisRecord.cUpUpTime[0].year!=0xff || meterStatisRecord.cUpUpTime[1].year!=0xff ||  meterStatisRecord.cUpUpTime[2].year!=0xff
                  || meterStatisRecord.apparentUpTime.year!=0xff ||  meterStatisRecord.apparentUpUpTime.year!=0xff
                   || meterStatisRecord.vUnBalanceTime.year!=0xff || meterStatisRecord.cUnBalanceTime.year!=0xff
              )
           {
             //�����ϴγ�������
             //����������
             tmpTime = copyCtrl[port].lastCopyTime;
             if (readMeterData(lastCopyEnergyData, tmpNode->mp, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == FALSE)
             {
             	 ;  //���޵��������ݱ�־
             }

             //�������α���
             tmpTime = copyCtrl[port].lastCopyTime;
             if (readMeterData(lastCopyParaData, tmpNode->mp, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0) == FALSE)
             {
             	 ;  //���ޱ����α������ݱ�־
             }
             
             //����������ͳ��
             //��ȡǰһ��ʵʱ����α���ͳ������,��ϱ��γ������������µ�ͳ������
             tmpTime = copyCtrl[port].lastCopyTime;
             if (readMeterData(balanceParaData, tmpNode->mp, LAST_REAL_BALANCE, REAL_BALANCE_PARA_DATA, &tmpTime, 0) == TRUE)
             {
               //��һ��ͳ��֮ǰ�����ݻ�������(��Ϊ0xee)
               //if (balanceParaData[NEXT_NEW_INSTANCE] == START_NEW_INSTANCE)
               //{
               //  memset(balanceParaData,0xee,LEN_OF_PARA_BALANCE_RECORD-1);
               //}
             }
             else         //��ʼ�µ�ͳ�Ƽ�¼
             {
               memset(balanceParaData,0xee,LEN_OF_PARA_BALANCE_RECORD);
               
               //û��ͳ����ʷ,����ͳ�Ƶ�ͬ������ͳ��
               balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE;
             }
             
             if (meterStatisRecord.vUpUpTime[0].year!=0xff || meterStatisRecord.vUpUpTime[1].year!=0xff ||  meterStatisRecord.vUpUpTime[2].year!=0xff 
                 || meterStatisRecord.vDownDownTime[0].year!=0xff || meterStatisRecord.vDownDownTime[1].year!=0xff ||  meterStatisRecord.vDownDownTime[2].year!=0xff
                )
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:��ѹԽ���ڳ���\n");
               }

               //��ѹͳ��
               statisticVoltage(tmpNode->mp, lastCopyParaData, lastCopyEnergyData, balanceParaData, &meterStatisRecord, pMpLimitValue,2, copyCtrl[port].lastCopyTime);
             }

             if (meterStatisRecord.apparentUpTime.year!=0xff ||  meterStatisRecord.apparentUpUpTime.year!=0xff)
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:���ڹ���Խ���ڳ���\n");
               }
               
               //���ڹ���Խ���ۼƼ����������ֶ��ۼ�
               statisticApparentPowerAndFactor(tmpNode->mp, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, 2, copyCtrl[port].lastCopyTime);
             }
       
             if (meterStatisRecord.cUpTime[0].year!=0xff || meterStatisRecord.cUpTime[1].year!=0xff ||  meterStatisRecord.cUpTime[2].year!=0xff 
                  || meterStatisRecord.cUpUpTime[0].year!=0xff || meterStatisRecord.cUpUpTime[1].year!=0xff ||  meterStatisRecord.cUpUpTime[2].year!=0xff
             	  )
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:����Խ���ڳ���\n");
               }

             	 //����ͳ��
               statisticCurrent(tmpNode->mp, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, 2, copyCtrl[port].lastCopyTime);
             }
             	
             if (meterStatisRecord.vUnBalanceTime.year!=0xff || meterStatisRecord.cUnBalanceTime.year!=0xff)
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:��ƽ��Խ���ڳ���\n");
               }
               
             	 //��ƽ���Խ���ۼ�
               statisticUnbalance(tmpNode->mp, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue,2, copyCtrl[port].lastCopyTime);
             }


             if (bakStatisRec.vOverLimit != meterStatisRecord.vOverLimit
             	   || bakStatisRec.cOverLimit != meterStatisRecord.cOverLimit
              	  || bakStatisRec.vUnBalance != meterStatisRecord.vUnBalance
              	   || bakStatisRec.cUnBalance != meterStatisRecord.cUnBalance
              	    || bakStatisRec.apparentPower != meterStatisRecord.apparentPower
             	  )
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:�洢������ͳ������\n");
               }
               //�洢������ͳ������
               saveMeterData(tmpNode->mp, port+1, copyCtrl[port].lastCopyTime, (INT8U *)&meterStatisRecord, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
             }
           }
         //}
 		 	 }
 		 	 
 		 	 tmpNode = tmpNode->next;
 		 }
     free(pMpLimitValue);   //�ͷŲ�������ֵָ��
     
     //�ͷ�����
 		 tmpNode = cpLinkHead;
 		 while(cpLinkHead!=NULL)
 		 {
 		 	 tmpNode = cpLinkHead;
 		 	 cpLinkHead = tmpNode->next;
 		 	 free(tmpNode);
 		 }
 	}
}

#ifdef LIGHTING

/*******************************************************
��������: processKzqOverLimit
��������: ������·������Խ��
���ú���:     
�����ú���:
�������:

�������:
����ֵ�� 
*******************************************************/
void processKzqOverLimit(INT8U *acParaData, INT16U pn)
{
  KZQ_STATIS_EXTRAN_TIME kzqStatisRecord;         //һ����Ƶ�(��·������)ͳ���¼�����
  PN_LIMIT_PARA          *pPnLimitValue;          //���Ƶ���ֵ����ָ��
	
  INT8U                  phase=0;
	INT8U                  bitShift=0x01;
	INT16U                 offset;
  INT32U                 voltage;
	INT32U                 current;
	INT32U                 cUpUpLimit, cDownDownLimit;
	INT32U                 pUpLimit, pDownLimit;
	INT32U                 power;
	INT32U                 factor;
	INT32U                 fDownLimit;
	
	//1,Ϊ��������ֵ����洢�ռ�
	pPnLimitValue = (PN_LIMIT_PARA *)malloc(sizeof(PN_LIMIT_PARA));
	
	//2,��ȡ���Ƶ��Ӧ����ֵ����
	if(selectViceParameter(0x04, 52, pn, (INT8U *)pPnLimitValue, sizeof(PN_LIMIT_PARA)) == FALSE)
	{
		pPnLimitValue = NULL;
	}
	
	if (pPnLimitValue!=NULL)
	{
		//3,������·������ͳ�Ƽ�¼(��ʱ���޹���)
		searchMpStatis(sysTime, &kzqStatisRecord, pn, 4);
		
		if (debugInfo&PRINT_BALANCE_DEBUG)
		{
			printf("��·������%d��ѹԽ��״ֵ̬=%x\n", pn, kzqStatisRecord.vOverLimit);
			printf("��·������%d����Խ��״ֵ̬=%x\n", pn, kzqStatisRecord.cOverLimit);
			printf("��·������%d����Խ��״ֵ̬=%x\n", pn, kzqStatisRecord.powerLimit);
			printf("��·������%d��������Խ��ֵ=%x\n", pn, kzqStatisRecord.factorLimit);
		}
		
		if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
		{
		  //4,��ѹԽ���ж�
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("��·�������ѹͳ��:��ѹ��ѹ��ֵ=%x\n", pPnLimitValue->vSuperiodLimit);
				printf("��·�������ѹͳ��:��ѹǷѹ��ֵ=%x\n", pPnLimitValue->vDownDownLimit);
			}
			bitShift = 0x01;
			offset = VOLTAGE_PHASE_A;
			for(phase=1; phase<=3; phase++)
			{
				voltage = acParaData[offset] | acParaData[offset+1]<<8;
				
				//20V��Ϊ��������
				if (voltage>0x200)
				{
					if (debugInfo&PRINT_BALANCE_DEBUG)
					{
						printf("��·�������ѹͳ��:%d���ѹֵ=%x\n", phase, voltage);
					}
					
					//Խ������
					if (voltage >= pPnLimitValue->vSuperiodLimit)
					{
						//��һ�η���Խ������,��¼Խ���¼�
						if ((kzqStatisRecord.vOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.vUpUpTime[phase-1].year==0xff)
							{
								kzqStatisRecord.vUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("��·�������ѹͳ��:Խ�����޿�ʼ����,%d���Ӻ��¼\n", pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.vUpUpTime[phase-1], sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("��·�������ѹͳ��:��ѹԽ�����޷�������ʱ���ѵ�\n");
									}
									 
									kzqStatisRecord.vUpUpTime[phase-1].year = 0xff;
						 
									kzqStatisRecord.vOverLimit |= bitShift;
									vOverLimitEvent(acParaData, phase, pn, 2, FALSE, timeHexToBcd(sysTime));
								}
							}
						}
						else
						{
							kzqStatisRecord.vUpUpTime[phase-1].year = 0xff;
						}
					}
					else   
					{
						if (voltage<pPnLimitValue->vSuperiodLimit)
						{
							//��������Խ������,��¼Խ�����޻ָ�
							if (kzqStatisRecord.vOverLimit&bitShift)
							{
								if (kzqStatisRecord.vUpUpTime[phase-1].year==0xff)
								{
									kzqStatisRecord.vUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									printf("��·�������ѹͳ��:Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.vUpUpTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("��·�������ѹͳ��:Խ�����޻ָ�����ʱ���ѵ�\n");
										}
									 
										kzqStatisRecord.vUpUpTime[phase-1].year = 0xff;
						 
										kzqStatisRecord.vOverLimit &= (~bitShift);                    	
										vOverLimitEvent(acParaData, phase, pn, 2, TRUE, timeHexToBcd(sysTime));
									}
								}
							}
							else
							{
								kzqStatisRecord.vUpUpTime[phase-1].year = 0xff;
							}
						}
					}
					
					//Խ������
					bitShift <<= 1;
					if (voltage <= pPnLimitValue->vDownDownLimit)   //Խ������
					{
						//��һ�η���Խ��,��¼Խ���¼�
						if ((kzqStatisRecord.vOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.vDownDownTime[phase-1].year==0xff)
							{
								kzqStatisRecord.vDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
								 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("��·�������ѹͳ��:��ѹԽ�����޷���,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.vDownDownTime[phase-1],sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("��·�������ѹͳ��:��ѹԽ�����޷�������ʱ���ѵ�\n");
									}
									 
									kzqStatisRecord.vDownDownTime[phase-1].year = 0xff;
						 
									kzqStatisRecord.vOverLimit |= bitShift;
									vOverLimitEvent(acParaData, phase, pn, 1, FALSE, timeHexToBcd(sysTime));
								}
							}
						}
						else
						{
							kzqStatisRecord.vDownDownTime[phase-1].year = 0xff;
						}
					}
					else
					{
						//��������Խ������,��¼Խ�޻ָ��¼�
						if (voltage>pPnLimitValue->vDownDownLimit)
						{
							if (kzqStatisRecord.vOverLimit&bitShift)
							{
								if (kzqStatisRecord.vDownDownTime[phase-1].year==0xff)
								{
									kzqStatisRecord.vDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									 
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("��·�������ѹͳ��:��ѹԽ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
									}
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.vDownDownTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("��·�������ѹͳ��:��ѹ�����޻ָ�����ʱ���ѵ�\n");
										}
									 
										kzqStatisRecord.vDownDownTime[phase-1].year = 0xff;
							 
										kzqStatisRecord.vOverLimit &= (~bitShift);
										vOverLimitEvent(acParaData, phase, pn, 1, TRUE, timeHexToBcd(sysTime));
									}
								}
							}
							else
							{
								kzqStatisRecord.vDownDownTime[phase-1].year = 0xff;
							}
						}
					}
					bitShift >>= 1;
				}
				else    //2016-12-23,Add
				{
					if (kzqStatisRecord.vUpUpTime[phase-1].year!=0xff)
					{
						kzqStatisRecord.vUpUpTime[phase-1].year = 0xff;
					}
					if (kzqStatisRecord.vDownDownTime[phase-1].year!=0xff)
					{
						kzqStatisRecord.vDownDownTime[phase-1].year = 0xff;
					}
				}
				
				bitShift <<= 2;
				offset += 2;
			}
		}
		
	  if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
	  {
			//����Խ���ж�
			bitShift = 0x01;
			offset = CURRENT_PHASE_A;
      cUpUpLimit  = pPnLimitValue->cSuperiodLimit[0] | pPnLimitValue->cSuperiodLimit[1]<<8 | pPnLimitValue->cSuperiodLimit[2]<<16;
			cDownDownLimit = pPnLimitValue->cDownDownLimit[0] | pPnLimitValue->cDownDownLimit[1]<<8 | pPnLimitValue->cDownDownLimit[2]<<16;
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("��·���������ͳ��:����������ֵ=%x\n", cUpUpLimit);
				printf("��·���������ͳ��:����Ƿ����ֵ=%x\n", cDownDownLimit);
			}
			for (phase = 1; phase <= 3; phase++)
			{
				current = acParaData[offset] | acParaData[offset+1]<<8 | (acParaData[offset+2]&0x7f)<<16;
				if (debugInfo&PRINT_BALANCE_DEBUG)
				{
					printf("��·���������ͳ��:%d�����ֵ=%x\n", phase, current);
				}
				
				//0.1A��Ϊ��������,<0.1�Ĳ���Խ���жϴ���
				if (current>0x100)
				{
					//����Խ������
					if (current >= cUpUpLimit)  
					{
						//��һ�η���Խ�ޣ���¼Խ���¼�
						if ((kzqStatisRecord.cOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.cUpUpTime[phase-1].year==0xff)
							{
								kzqStatisRecord.cUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{ 
									printf("��·���������ͳ��:Խ�����޿�ʼ����,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.cUpUpTime[phase-1],sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("��·���������ͳ��:Խ�����޷�������ʱ���ѵ�\n");
									}
								 
									kzqStatisRecord.cUpUpTime[phase-1].year = 0xff;
																			 
									kzqStatisRecord.cOverLimit |= bitShift;
									cOverLimitEvent(acParaData, phase, pn, 2, FALSE, timeHexToBcd(sysTime));
								}
							}
						}               	   
						else
						{
							kzqStatisRecord.cUpUpTime[phase-1].year = 0xff;
						}
					}
					else   
					{
						//������Խ�������¼�����,��¼Խ�����޻ָ�
						if (current<cUpUpLimit)
						{
							if ((kzqStatisRecord.cOverLimit&bitShift) == bitShift)
							{
								if (kzqStatisRecord.cUpUpTime[phase-1].year==0xff)
								{
									kzqStatisRecord.cUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									printf("��·���������ͳ��:Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.cUpUpTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("��·���������ͳ��:Խ�����޻ָ�����ʱ���ѵ�\n");
										}
									 
										kzqStatisRecord.cUpUpTime[phase-1].year = 0xff;
						 
										//��¼Խ�����޻ָ�
										kzqStatisRecord.cOverLimit &= (~bitShift);
										cOverLimitEvent(acParaData, phase, pn, 2, TRUE, timeHexToBcd(sysTime));
									}
								}
							}
							else
							{
								kzqStatisRecord.cUpUpTime[phase-1].year = 0xff;
							}
						}
					}
					
					bitShift<<=1;
					//����Խ������
					if (current<cDownDownLimit)
					{
						//��һ�η���Խ�ޣ���¼Խ���¼�
						if ((kzqStatisRecord.cOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.cDownDownTime[phase-1].year==0xff)
							{
								kzqStatisRecord.cDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{ 
									printf("��·���������ͳ��:Խ�����޿�ʼ����,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.cDownDownTime[phase-1], sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("��·���������ͳ��:Խ�����޷�������ʱ���ѵ�\n");
									}
								 
									kzqStatisRecord.cDownDownTime[phase-1].year = 0xff;
																			 
									kzqStatisRecord.cOverLimit |= bitShift;
									cOverLimitEvent(acParaData, phase, pn, 0, FALSE, timeHexToBcd(sysTime));
								}
							}
						}               	   
						else
						{
							kzqStatisRecord.cDownDownTime[phase-1].year = 0xff;
						}
					}
					else   
					{
						//������Խ�������¼�����,��¼Խ�����޻ָ�
						if (current>cDownDownLimit)
						{
							if ((kzqStatisRecord.cOverLimit&bitShift) == bitShift)
							{
								if (kzqStatisRecord.cDownDownTime[phase-1].year==0xff)
								{
									kzqStatisRecord.cDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									printf("��·���������ͳ��:Խ�����޿�ʼ�ָ�,%d���Ӻ��¼\n", pPnLimitValue->overContinued);
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.cDownDownTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("��·���������ͳ��:Խ�����޻ָ�����ʱ���ѵ�\n");
										}
									 
										kzqStatisRecord.cDownDownTime[phase-1].year = 0xff;
						 
										//��¼Խ�����޻ָ�
										kzqStatisRecord.cOverLimit &= (~bitShift);
										cOverLimitEvent(acParaData, phase, pn, 0, TRUE, timeHexToBcd(sysTime));
									}
								}
							}
							else
							{
								kzqStatisRecord.cDownDownTime[phase-1].year = 0xff;
							}
						}
					}
					bitShift>>=1;
				}
				else
				{
					//2016-12-23,����⼸��,�żҸ����ֳ�Ӧ��ʱ������ʱ��һ��բ�ͻ��¼����Ƿ���¼�
					if (kzqStatisRecord.cUpUpTime[phase-1].year!=0xff)
					{
						kzqStatisRecord.cUpUpTime[phase-1].year = 0xff;
					}
          if (kzqStatisRecord.cDownDownTime[phase-1].year!=0xff)
					{
						kzqStatisRecord.cDownDownTime[phase-1].year = 0xff;
					}
				}
					
				bitShift<<=2;
				offset += 3;
			}
		}
		
		if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
		{
			power = acParaData[POWER_INSTANT_WORK] | acParaData[POWER_INSTANT_WORK+1]<<8 | (acParaData[POWER_INSTANT_WORK+2]&0x7f)<<16;
      pUpLimit  = pPnLimitValue->pUpLimit[0] | pPnLimitValue->pUpLimit[1]<<8 | pPnLimitValue->pUpLimit[2]<<16;
			pDownLimit = pPnLimitValue->pDownLimit[0] | pPnLimitValue->pDownLimit[1]<<8 | pPnLimitValue->pDownLimit[2]<<16;
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("��·�����㵱ǰ����ֵ=%x\n",power);
				printf("��·�����㹦��������ֵ=%x\n",pUpLimit);				
				printf("��·�����㹦��������ֵ=%x\n",pDownLimit);
			}

			//>0.01kW���жϹ���Խ��
			if (power >0x000100)
			{
				//����Խ����
				if (power > pUpLimit)
				{
					if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
					{
						//��һ�η���Խ����,��¼Խ���¼�
						if ((kzqStatisRecord.powerLimit&0x01) == 0x00)
						{
							if (kzqStatisRecord.powerUpTime.year==0xff)
							{
								kzqStatisRecord.powerUpTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
								 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("��·�����㹦��ͳ��:Խ���޷���,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.powerUpTime, sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("��·�����㹦��ͳ��:Խ���޷�������ʱ���ѵ�\n");
									}
								 
									kzqStatisRecord.powerUpTime.year = 0xff;
									kzqStatisRecord.powerLimit |= 0x01;
									pOverLimitEvent(pn, 3, FALSE, (INT8U *)&power, pPnLimitValue, timeHexToBcd(sysTime));
								}
							}
						}               	   
						else
						{
							kzqStatisRecord.powerUpTime.year = 0xff;
						}
					}
				}
				else
				{
				  //Խ���޻ָ�
				  if (power<=pUpLimit)
				  {
					  //��������Խ����,��¼Խ���޻ָ�
					  if ((kzqStatisRecord.powerLimit&0x01) == 0x01)
					  {
						  if (kzqStatisRecord.powerUpTime.year==0xff)
						  {
							  kzqStatisRecord.powerUpTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							  printf("��·�����㹦��ͳ��:Խ���޿�ʼ�ָ�,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
						  }
						  else
						  {
							  if (compareTwoTime(kzqStatisRecord.powerUpTime,sysTime))
							  {
								  if (debugInfo&PRINT_BALANCE_DEBUG)
								  {
									  printf("��·�����㹦��ͳ��:���޻ָ�����ʱ���ѵ�\n");
								  }
							 
								  kzqStatisRecord.powerUpTime.year = 0xff;
								  kzqStatisRecord.powerLimit &= 0xFE;
								  pOverLimitEvent(pn, 3, TRUE, (INT8U *)&power, pPnLimitValue, timeHexToBcd(sysTime));

							  }
						  }
					  }
					  else
					  {
						  kzqStatisRecord.powerUpTime.year = 0xff;
					  }
					}
				}
				
				//����Խ����
				if (power < pDownLimit)
				{
					//��һ�η���Խ����,��¼Խ���¼�
					if ((kzqStatisRecord.powerLimit&0x10) == 0x00)
					{
						if (kzqStatisRecord.powerDownTime.year==0xff)
						{
							kzqStatisRecord.powerDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
							if (debugInfo&PRINT_BALANCE_DEBUG)
							{
								printf("��·�����㹦��ͳ��:Խ���޷���,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
							}
						}
						else
						{
							if (compareTwoTime(kzqStatisRecord.powerDownTime, sysTime))
							{
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("��·�����㹦��ͳ��:Խ���޷�������ʱ���ѵ�\n");
								}
							 
								kzqStatisRecord.powerDownTime.year = 0xff;
								kzqStatisRecord.powerLimit |= 0x10;
								pOverLimitEvent(pn, 0, FALSE, (INT8U *)&power, pPnLimitValue, timeHexToBcd(sysTime));
							}
						}
					}
					else
					{
						kzqStatisRecord.powerDownTime.year = 0xff;
					}
				}
				else
				{
				  //Խ���޻ָ�
				  if (power>=pDownLimit)
				  {
					  //��������Խ����,��¼Խ���޻ָ�
					  if ((kzqStatisRecord.powerLimit&0x10) == 0x10)
					  {
						  if (kzqStatisRecord.powerDownTime.year==0xff)
						  {
							  kzqStatisRecord.powerDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							  printf("��·�����㹦��ͳ��:Խ���޿�ʼ�ָ�,%d���Ӻ��¼\n", pPnLimitValue->overContinued);
						  }
						  else
						  {
							  if (compareTwoTime(kzqStatisRecord.powerDownTime, sysTime))
							  {
								  if (debugInfo&PRINT_BALANCE_DEBUG)
								  {
									  printf("��·�����㹦��ͳ��:���޻ָ�����ʱ���ѵ�\n");
								  }
							 
								  kzqStatisRecord.powerDownTime.year = 0xff;
								  kzqStatisRecord.powerLimit &= 0xEF;
								  pOverLimitEvent(pn, 0, TRUE, (INT8U *)&power, pPnLimitValue, timeHexToBcd(sysTime));
							  }
						  }
					  }
					  else
					  {
						  kzqStatisRecord.powerDownTime.year = 0xff;
					  }
					}
				}
			}
			else    //2016-12-23,Add
			{
				if (kzqStatisRecord.powerUpTime.year!=0xff)
				{
					kzqStatisRecord.powerUpTime.year = 0xff;
				}
				if (kzqStatisRecord.powerDownTime.year!=0xff)
				{
					kzqStatisRecord.powerDownTime.year = 0xff;
				}
			}
		}
		
		//��������Խ��ERC36
		if ((eventRecordConfig.iEvent[4] & 0x08) || (eventRecordConfig.nEvent[4] & 0x08))
		{
			factor = acParaData[TOTAL_POWER_FACTOR] | (acParaData[TOTAL_POWER_FACTOR+1]&0x7f)<<8;
			fDownLimit = pPnLimitValue->factorDownLimit[0] | (pPnLimitValue->factorDownLimit[1]&0x7f)<<8;
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("��·�����㵱ǰ��������ֵ=%x\n", factor);
				printf("��·�����㹦������������ֵ=%x\n", fDownLimit);
			}

			//>10%���жϹ���Խ��
			if (factor >0x100)
			{
				//����Խ����
				if (factor < fDownLimit)
				{
					//��һ�η���Խ����,��¼Խ���¼�
					if ((kzqStatisRecord.factorLimit&0x10) == 0x00)
					{
						if (kzqStatisRecord.factorDownTime.year==0xff)
						{
							kzqStatisRecord.factorDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
							if (debugInfo&PRINT_BALANCE_DEBUG)
							{
								printf("��·�����㹦������ͳ��:Խ���޷���,%d���Ӻ��¼\n",pPnLimitValue->overContinued);
							}
						}
						else
						{
							if (compareTwoTime(kzqStatisRecord.factorDownTime, sysTime))
							{
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("��·�����㹦������ͳ��:Խ���޷�������ʱ���ѵ�\n");
								}
							 
								kzqStatisRecord.factorDownTime.year = 0xff;
								kzqStatisRecord.factorLimit |= 0x10;
								fOverLimitEvent(pn, 0, FALSE, (INT8U *)&factor, pPnLimitValue, timeHexToBcd(sysTime));
							}
						}
					}               	   
					else
					{
						kzqStatisRecord.factorDownTime.year = 0xff;
					}
				}
				else
				{
				  //Խ���޻ָ�
				  if (factor>=fDownLimit)
				  {
					  //��������Խ����,��¼Խ���޻ָ�
					  if ((kzqStatisRecord.factorLimit&0x10) == 0x10)
					  {
						  if (kzqStatisRecord.factorDownTime.year==0xff)
						  {
							  kzqStatisRecord.factorDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							  printf("��·�����㹦������ͳ��:Խ���޿�ʼ�ָ�,%d���Ӻ��¼\n", pPnLimitValue->overContinued);
						  }
						  else
						  {
							  if (compareTwoTime(kzqStatisRecord.factorDownTime, sysTime))
							  {
								  if (debugInfo&PRINT_BALANCE_DEBUG)
								  {
									  printf("��·�����㹦������ͳ��:���޻ָ�����ʱ���ѵ�\n");
								  }
							 
								  kzqStatisRecord.factorDownTime.year = 0xff;
								  kzqStatisRecord.factorLimit &= 0xEF;
								  fOverLimitEvent(pn, 0, TRUE, (INT8U *)&factor, pPnLimitValue, timeHexToBcd(sysTime));
							  }
						  }
					  }
					  else
					  {
						  kzqStatisRecord.factorDownTime.year = 0xff;
					  }
					}
				}
			}
			else    //2016-12-23,Add
			{
				if (kzqStatisRecord.factorDownTime.year!=0xff)
				{
					kzqStatisRecord.factorDownTime.year = 0xff;
				}
			}
		}
		
		//�洢��·������ͳ������
    saveMeterData(pn, 2, sysTime, (INT8U *)&kzqStatisRecord, STATIS_DATA, 89, sizeof(KZQ_STATIS_EXTRAN_TIME));
	}
}

/*******************************************************
��������:pOverLimitEvent
��������:��������Խ���¼�
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
void fOverLimitEvent(INT16U pn, INT8U type, BOOL recovery, INT8U *data, PN_LIMIT_PARA *pLimit,DATE_TIME statisTime)
{	
	INT8U eventData[16];
	INT8U dataTail;
	
	eventData[0] = 36;
	
	eventData[2] = statisTime.second;
	eventData[3] = statisTime.minute;
	eventData[4] = statisTime.hour;
	eventData[5] = statisTime.day;
	eventData[6] = statisTime.month;
	eventData[7] = statisTime.year;

	dataTail = 8;
	
	eventData[dataTail++] = pn&0xff;   //�������8λ    
	eventData[dataTail] = (pn>>8)&0xf;    
	
	if (recovery == FALSE)
	{
		eventData[dataTail] |= 0x80;
	}
	dataTail++;
	
	if (type == 0x01)  //Խ����
	{
		eventData[dataTail] = 0x00;
	}
	dataTail++;
	
	//����ʱ�Ĺ�������
	eventData[dataTail++] = *data;
	eventData[dataTail++] = *(data+1);
	
	//����ʱ�Ĺ���������ֵ
	eventData[dataTail++] = pLimit->factorDownLimit[0];
	eventData[dataTail++] = pLimit->factorDownLimit[1];
	
	eventData[1] = dataTail;

	if (eventRecordConfig.iEvent[4] & 0x08)
	{
		 writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
	}
	if (eventRecordConfig.nEvent[4] & 0x08)
	{
		 writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //����һ���¼�����
	}
	
	eventStatus[4] = eventStatus[4] | 0x08;
}

#endif
