/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：dataBalance.c
作者：TianYe
版本：0.9
完成日期：2006年7月
描述：数据结算文件
函数列表：
修改历史：
  01,2006-7-19 TianYe created.
  02,2007-3-10,TianYe and Leiyong修改日结算及月结算,补充未做的日结算和月结算
  03,2007-4-19,Leiyong And TianYe增加抄表日冻结
  04,2007-7-2,Leiyong And TianYe修正电表停走事件错误
  05,2008-09-19,Tianye修改,规范北京测试修改代码。解决配置了没有抄数的测量点引起的延时大问题...
  06,2008-12-23,Leiyong修改,计算总加功率时乘以CT,PT得到二次功率
  07,2010-01-22,Leiyong,增加GDW376.1-2009协议需要的结算及判断
  08,2010-07-20,Leiyong,在科陆测试台上测试linux下的应用,GDW129-2005测试通过
  09,2012-04-17,去掉备份日结算
  10,2012-04-25,去掉各端口不能并行结算的判断

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

//数据存储标志
#define  NO_DATA_NEED_STORE         0x0    //没有数据需要保存
#define  STORE_ENERGY_BALANCE_DATA  0x1    //存储实时结算电能量数据
#define  STORE_DAY_BALANCE_DATA     0x2    //存储日冻结数据
#define  STORE_MONTH_BALANCE_DATA   0x4    //存储月冻结数据
#define  STORE_ZJZ_BALANCE_DATA     0x8    //存储总加组结算数据

#define maxData(data1, data2) ((data1>=data2)?data1:data2)
#define minData(data1, data2) ((data1<=data2)?data1:data2)

/*******************************************************
函数名称: dataFormat
功能描述: 按照需要的数据格式调整数据格式，确定数量级
调用函数:     
被调用函数:
输入参数:   整数部分指针integer
            小数部分指针decimal(4位整数表示的小数部分,如果是0.23那么输入的decimal参数应该是2300而不是23)
            需要转换的格式号format
输出参数:  二进制数据结果调整后的整数部分,
           二进制数据结果调整后的小数部分
返回值： 数据结果的数量级
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
              tmp = *integer % 10;   //个位移到小数部分
              *integer /= 10;        
              *decimal /= 10;        //整数小数整体右移
              *decimal += 1000 * tmp;//原来整数的个位移作小数的十分位
            
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
              	tmp = *decimal / 1000;              //小数十分位移到整数部分
              	*decimal = (*decimal % 1000) * 10;  //小数整体左移
              	*integer = *integer * 10 + tmp;     //整数右移+小数移出的部分
            	
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
            	quantity = 0x4;   //10的0次方
            	break;     
            case 1:
            	quantity = 0x3;   //10的1次方
            	break;
            case 2:
            	quantity = 0x2;   //10的2次方
            	break;
            case 3:
            	quantity = 0x1;   //10的3次方
            	break;
            case 4:
            	quantity = 0x0;   //10的4次方
            	break;
            case 5:
            	quantity = 0x5;   //10的-1次方
            	break;
            case 6:
          	  quantity = 0x6;   //10的-2次方
          	  break;
            case 7:
          	  quantity = 0x7;   //10的-3次方
            	break;
          }
          
          //小数四舍五入到整数位
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
      	  
      	    quantity = 0x40;    //10的3次方
      	  }
      	  else
      	  {
      	    quantity = 0;
      	  }
      	  
      	  //小数四舍五入到整数位
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
函数名称: meterRunWordChangeBit
功能描述: 判断电表运行状态字是否变位
调用函数:     
被调用函数:
输入参数:

输出参数:
返回值： 
*******************************************************/
void meterRunWordChangeBit(INT8U *changeWord,INT8U *thisRunWord,INT8U *lastRunWord)
{
	 INT8U i,j;
	 INT8U tmpShift,tmpThisData,tmpLastData;
	 
	 if (debugInfo&PRINT_BALANCE_DEBUG)
	 {
	  printf("开始判断电表运行状态字变位:\n");
	 }
	 
	 for(i=0;i<14;i++)
	 {
	 	 *changeWord=0; 
	 	 if (*thisRunWord!=0xee && *lastRunWord!=0xee)
	 	 {
	 	 	 if (debugInfo&PRINT_BALANCE_DEBUG)
	 	 	 {
	 	 	   printf("第%02d字节对比:Last=%02X,Curent=%02X\n", i+1, *lastRunWord, *thisRunWord);
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
	   printf("判断电表运行状态字变位结束\n");
	 }
}


//内部私有函数
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
函数名称:dataProcRealPoint
功能描述:测量点实时结算数据处理,以供计算参考用
         处理内容：
         费率相关当日数据 当日正向有功电能量
                          当日正向无功电能量
                          当日反向有功电能量
                          当日反向无功电能量
                          ...
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*************************************************************************/
void dataProcRealPoint(void *arg)
{
    INT8U                    port;                                            //端口
    INT8U                    type;                                            //类型
    INT8U                    balanceInterval;                                 //结算间隔
    DATE_TIME                balanceTime;                                     //结算时间
    
    struct cpAddrLink        *mpLinkHead;                                     //测量点链表
    struct cpAddrLink        *tmpNode;                                        //测量点链表临时指针
    MEASUREPOINT_LIMIT_PARA  *pMpLimitValue;                                  //测量点限值参数指针
    METER_STATIS_EXTRAN_TIME meterStatisRecord;                               //一块电表统计事件数据
    METER_DEVICE_CONFIG      meterConfig;                                     //一个测量点的配置信息
    INT8U                    lastCopyEnergyData[LENGTH_OF_ENERGY_RECORD];     //上一次抄表电能量数据
    INT8U                    lastCopyParaData[LENGTH_OF_PARA_RECORD];         //上一次抄表参量,参变量数据
    INT8U                    lastCopyReqData[LENGTH_OF_REQ_RECORD];           //上一次抄表需量及需量发生时间数据
    INT8U                    balanceEnergyData[LEN_OF_ENERGY_BALANCE_RECORD]; //结算用电能量数据
    INT8U                    balanceParaData[LEN_OF_PARA_BALANCE_RECORD];     //结算用参变量数据
    INT8U                    balanceZjzData[LEN_OF_ZJZ_BALANCE_RECORD];       //结算用总加组数据
    INT8U                    balanceZjzDatax[LEN_OF_ZJZ_BALANCE_RECORD];      //结算用总加组数据x

    BOOL                     ifSinglePhaseMeter;                              //是否单相表标志    
    INT8U                    flagOfStoreData;                                 //存储数据标志
    INT8U                    flagOfLastData;                                  //是否有上一次抄表数据?
    INT16U                   i, j, k;
    DATE_TIME                tmpTime, readTime;
    INT16U                   balancePn;                                       //结算测量点
    INT32U                   compData,diffData,absoluteData;                  //总加有功电能量判断用
    INT8U                    tmpRecord;
    INT8U                    eventData[200];
    INT8U                    tmpTail,tmpTailx;
    
    while(1)
    {
      type = 0;
      for(port=0; port<NUM_OF_COPY_METER; port++)
      {
        //抄一次表后进行实时结算
       #ifdef PLUG_IN_CARRIER_MODULE
        if (copyCtrl[port].ifRealBalance==1 && port<4)
       #else
        if (copyCtrl[port].ifRealBalance==1)
       #endif
        {
          copyCtrl[port].ifRealBalance = 2;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("\n\n%02d-%02d-%02d %02d:%02d:%02d准备端口%d结算,2=%d,3=%d\n\n",sysTime.year,sysTime.month,
                   sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,
                     port+1,copyCtrl[1].ifRealBalance,copyCtrl[2].ifRealBalance);
          }
          
          type = 1;                                                  //结算类型
          balanceInterval = teCopyRunPara.para[port].copyInterval;   //抄表间隔
          balanceTime = copyCtrl[port].lastCopyTime;                 //结算时间为上次抄表时间

          break;
        }
      }
      
      //结算(包括实时结算、日结算及月结算)
      if (type==1)
      {
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("端口%d结算开始\n", port+1);
        }
        
    	  //0.数据存储标志置为无需要保存的数据
    	  flagOfStoreData = NO_DATA_NEED_STORE;
    	  
    	  //1.初始化本端口测量点链表
    	  mpLinkHead = initPortMeterLink(port);
    
    	  //1-1.没有测量点配置,直接返回
    	  if (mpLinkHead==NULL)
    	  {
    	  	if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	{
    	  	  printf("端口%d没有测量点配置,直接返回\n", port+1);
    	  	}
    	  	
    	  	copyCtrl[port].ifRealBalance = 0;
    	  	
    	  	continue;
    	  }
      	  
    	  //2.查找645规约的测量点起始指针
    	  tmpNode = mpLinkHead;
    	  while(tmpNode!=NULL)
    	  {
           if (selectF10Data(tmpNode->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
           {
        	    //库中没有该测量点的数据
        	    
    	        tmpNode = tmpNode->next;  //链表移位
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
    	     
    	     tmpNode = tmpNode->next;  //链表移位
    	  }
        
        //2-1.没有可以计算的测量点,释放链表然后返回
        if (tmpNode==NULL)
        {
         	 freeMpLink(mpLinkHead);
    
    	  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	 {
    	  	   printf("端口%d没有可以计算的测量点,释放链表然后返回\n", port+1);
    	  	 }
    	  	 
    	  	 copyCtrl[port].ifRealBalance = 0;

        	 continue;
        }
        
        //3.根据上次和下次抄表时间，判断是否进行日结算和月结算
        tmpTime = timeHexToBcd(copyCtrl[port].nextCopyTime);
 
        //如果上一次抄表时间和下一次抄表时间跨了一天,需要进行日结算转存
        if (tmpTime.day != balanceTime.day)
        {
          while(tmpNode!=NULL)
          {
            if (selectF10Data(tmpNode->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
            {
     	        //库中没有该测量点的数据
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
          
          //查看是否有当天的日结算数据,如果有就不用再做日结算了(这个思路已更新为下面这行的处理)
          //ly,10-10-02,改成如果原来已有日冻结数据就更新日冻结数据,因为这是最靠近0点的数据
          readTime = balanceTime;
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	   printf("存储日冻结数据标志置位\n");
        	}
        	 
        	flagOfStoreData |= STORE_DAY_BALANCE_DATA;
        
    	    //如果上一次抄表时间和下一次抄表时间跨了一月，需要进行月结算转存
          if (tmpTime.month != balanceTime.month)
          {
        	  flagOfStoreData |= STORE_MONTH_BALANCE_DATA;
          }
        }
       
        //4.本端口逐个检查测量点配置测量点结算,统计
    	  if (mpLinkHead != NULL)
    	  {
    	    //为测量点限值分配存储空间
    	    pMpLimitValue = (MEASUREPOINT_LIMIT_PARA *)malloc(sizeof(MEASUREPOINT_LIMIT_PARA));
    	    
    	    while (tmpNode!=NULL)
    	    {
    	      flagOfStoreData &= ~STORE_ENERGY_BALANCE_DATA;
    	      flagOfLastData = 0x7;   //上一次数据都有
    	      
    	      //4-1确定测量点信息
    	      ifSinglePhaseMeter = FALSE;
            
            //4-1-1确定库中有配置的测量点信息
            if (selectF10Data(tmpNode->mp, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
            {
        	     //库中没有该测量点的数据
    	         tmpNode = tmpNode->next;  //链表移位
        	     continue;
            }
    	      
    	      //4-1-2通信规约配置为现场监测设备或是交流采样装置,则无实时结算的数据项
    	      //if (meterConfig.protocol == AC_SAMPLE || meterConfig.protocol == SUPERVISAL_DEVICE)
    	      if (meterConfig.protocol == SUPERVISAL_DEVICE)
    	      {
    	         tmpNode = tmpNode->next;  //链表移位
    	         continue;
    	      }
            
            //4-1-3.单相表置单表标志
            if (meterConfig.protocol==SINGLE_PHASE_645_1997)
            {
            	 ifSinglePhaseMeter = TRUE;
            }
            balancePn = meterConfig.measurePoint;
    
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
    	        printf("\n测量点%d开始结算,结算时间:%02x-%02x-%02x %02x:%02x:%02x\n",balancePn,
    	             balanceTime.year,balanceTime.month,balanceTime.day,balanceTime.hour,balanceTime.minute,balanceTime.second);
    	      }
            
            //4-2.读出上次抄表数据
            //4-2-1.电能量数据
            tmpTime = balanceTime;
            if (readMeterData(lastCopyEnergyData, balancePn, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == FALSE)
            {
            	flagOfLastData &= 0xFE;  //置无电能量数据标志
            }
            
            usleep(50000);
                    
            //4-2-2.需量及需量发生时间
            tmpTime = balanceTime;
            if (readMeterData(lastCopyReqData, balancePn, PRESENT_DATA, REQ_REQTIME_DATA, &tmpTime, 0) == FALSE)
            {
            	flagOfLastData &= 0xFD;  //置无需量数据标志
            }
    
            usleep(50000);        
            
            //4-2-3.参量及参变量
            tmpTime = balanceTime;
            if (readMeterData(lastCopyParaData, balancePn, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0) == FALSE)
            {
            	flagOfLastData &= 0xFB;  //置无变量参变量数据标志
            }
            
            usleep(50000);
            
            //4-3.测量点数据统计
            //4-3-1.初始化统计用缓存区
            //读取前一次实时结算参变量统计数据,结合本次抄表数据生成新的统计数据
            tmpTime = balanceTime;
            if (readMeterData(balanceParaData, balancePn, LAST_REAL_BALANCE, REAL_BALANCE_PARA_DATA, &tmpTime, 0) == TRUE)
            {
    	        balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE_NOT;  //清除重新统计标志

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("dataProcRealPoint:有前一次实时结算参变量统计数据\n");
              }
            }
            else         //开始新的统计记录
            {
              memset(balanceParaData, 0xee, LEN_OF_PARA_BALANCE_RECORD);
              
              //没有统计历史,本次统计等同于重新统计
              balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE;

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("dataProcRealPoint:无前一次实时结算参变量统计数据,置重新统计标志\n");
              }
            }
    
            usleep(50000);
    
          	//4-3-2.测量点限值参数与电表事件统计数据
          	//4-3-2-1.读取测量点对应的限值参数
        	  if(selectViceParameter(0x04, 26, balancePn, (INT8U *)pMpLimitValue, sizeof(MEASUREPOINT_LIMIT_PARA)) == FALSE)
        	  {
        	  	pMpLimitValue = NULL;
        	  }
    
    	      //4-3-2-2.读出电表统计记录
    	      tmpTime = timeBcdToHex(balanceTime);
            searchMpStatis(tmpTime, &meterStatisRecord, balancePn, 1);  //与时间无关量
    
            //4-3-3.最大需量数据统计
            statisticMaxDemand(lastCopyReqData, balanceParaData);
            usleep(50000);
    
          	//4-3-4.功率统计
            statisticPower(lastCopyParaData, balanceParaData, balanceInterval, balanceTime);
            usleep(50000);
    
            //4-3-5.视在功率越限累计及功率因数分段累计
            statisticApparentPowerAndFactor(balancePn, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, balanceInterval, balanceTime);
            usleep(50000);
    
          	//4-3-6.电压统计
            statisticVoltage(balancePn, lastCopyParaData, lastCopyEnergyData, balanceParaData, &meterStatisRecord, pMpLimitValue,balanceInterval, balanceTime);
            usleep(50000);
    
          	//4-3-7.电流统计
            statisticCurrent(balancePn, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, balanceInterval, balanceTime);
            usleep(50000);
    
          	//4-3-8.不平衡度越限累计
            statisticUnbalance(balancePn, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue,balanceInterval, balanceTime);
            usleep(50000);
    
            //4-3-9.断相统计
            statisticOpenPhase(lastCopyParaData, balanceParaData);
            usleep(50000);
            
            //4-3-10.存储结算参变量数据            
    	      balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE_NOT;  //清除首次统计时置位的重新统计标志
            
            //4-3-10.1实时结算参变量统计数据存储在实时结算数据区
            if (balanceParaData[MAX_TOTAL_POWER] != 0xEE 
          	   || (lastCopyParaData[VOLTAGE_PHASE_A] != 0xEE || lastCopyParaData[VOLTAGE_PHASE_B] != 0xEE || lastCopyParaData[VOLTAGE_PHASE_C] != 0xEE)
          	    || (lastCopyParaData[CURRENT_PHASE_A] != 0xEE || lastCopyParaData[CURRENT_PHASE_B] != 0xEE || lastCopyParaData[CURRENT_PHASE_C] != 0xEE)
          	     || (balanceParaData[MAX_TOTAL_REQ] != 0xEE))
            {
              saveMeterData(balancePn, port+1, balanceTime, balanceParaData, REAL_BALANCE, REAL_BALANCE_PARA_DATA,LEN_OF_PARA_BALANCE_RECORD);
               
              //4-3-10.2 日结算参变量统计数据存储于日结算表
              if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
              {
                saveMeterData(balancePn, port+1, balanceTime, balanceParaData, DAY_BALANCE, DAY_BALANCE_PARA_DATA,LEN_OF_PARA_BALANCE_RECORD);
            
                //4-3-10.3 统计本月到当前为止的参变量统计值
                //ly,2011-08-13,原来忘加下面这个函数调用了,月统计数据不正确
                realStatisticPerPoint(balancePn, balanceParaData, balanceTime);
        	       
        	      tmpTime = balanceTime;
        	      tmpTime.second = 0x59;
                tmpTime.minute = 0x59;
                tmpTime.hour   = 0x23;
                saveMeterData(balancePn, port+1, tmpTime, balanceParaData, DAY_BALANCE, MONTH_BALANCE_PARA_DATA, LEN_OF_PARA_BALANCE_RECORD);
                 
                //4-3-10.4 将本次实时结算的数据结果存储到月结算表中,结束当月统计
                if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
                {
                  saveMeterData(balancePn, port+1, tmpTime, balanceParaData, MONTH_BALANCE, MONTH_BALANCE_PARA_DATA,LEN_OF_PARA_BALANCE_RECORD);
                }
              }
            }
    
    	      //4-4.测量点数据结算
    	      //4-4-1.初始化结算用缓存
    	      memset(balanceEnergyData, 0xee, LEN_OF_ENERGY_BALANCE_RECORD);
    	      
            //4-4-2.电能表事件
            eventRecord(balancePn, lastCopyEnergyData, lastCopyParaData, &meterStatisRecord, balanceInterval, balanceTime);
            usleep(50000);
            
            //4-4-3.变更事件
            changeEvent(balancePn, lastCopyParaData, lastCopyEnergyData, &meterStatisRecord, balanceTime,meterConfig.protocol, balanceInterval);
            usleep(50000);
            
            //4-4-4.结算当日电能量
            if (energyCompute(balancePn, lastCopyEnergyData, balanceEnergyData, balanceTime, 1)==TRUE)
            {
            	flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
            }
            usleep(50000);
            
            //4-4-5.结算当月电能量
            if (ifSinglePhaseMeter==FALSE)
            {
              if (energyCompute(balancePn,lastCopyEnergyData,balanceEnergyData, balanceTime, 2)==TRUE)
              {
            	   flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
              }
            }
            usleep(50000);
    
    	      //4-4-6.存储测量点结算结果
            if (flagOfStoreData&STORE_ENERGY_BALANCE_DATA)
            {
              //实时结算电能量数据存储于实时结算表
              saveMeterData(balancePn, port+1, balanceTime, balanceEnergyData, REAL_BALANCE, REAL_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
              
              if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
              {
            	   //日结算电能量存储于日结算表
                 saveMeterData(balancePn, port+1, balanceTime, balanceEnergyData, DAY_BALANCE, DAY_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);             
              }
            
              //将本次实时结算的数据结果存储于月结算表
              if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
              {
                saveMeterData(balancePn, port+1, balanceTime, balanceEnergyData, MONTH_BALANCE, MONTH_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
              }
            }
    
            //4-5.日(月)末转存示值及需量数据
            if (flagOfStoreData&STORE_DAY_BALANCE_DATA)
        	  {
    	        if (debugInfo&PRINT_BALANCE_DEBUG)
    	        {
             	  printf("转存日结算电能量/需量\n");
             	}
    
        	    tmpTime = balanceTime;
        	    tmpTime.second = 0x59;
              tmpTime.minute = 0x59;
              tmpTime.hour   = 0x23;
                
              //转存日结算电能示值
              if (flagOfLastData&0x1)
              { 
              	saveMeterData(balancePn, port+1, tmpTime, lastCopyEnergyData, DAY_BALANCE, DAY_FREEZE_COPY_DATA,LENGTH_OF_ENERGY_RECORD);
              }
                
              //转存日结算需量示值
              if (flagOfLastData&0x2)
              {
                saveMeterData(balancePn, port+1, tmpTime, lastCopyReqData, DAY_BALANCE, DAY_FREEZE_COPY_REQ, LENGTH_OF_REQ_RECORD);
              }
    
              //如果本次结算需要进行月结算转存,将抄表数据读出后存储到月结算数据区
              if (flagOfStoreData & STORE_MONTH_BALANCE_DATA)
              {
    	          if (debugInfo&PRINT_BALANCE_DEBUG)
    	          {
             	    printf("转存月结算电能量/需量\n");
             	  }
                
                //转存月结算电能示值
                if (flagOfLastData&0x1)
                {
                  saveMeterData(balancePn, port+1, tmpTime, lastCopyEnergyData, MONTH_BALANCE, MONTH_FREEZE_COPY_DATA,LENGTH_OF_ENERGY_RECORD);
                }
                
                //转存月结算需量示值
                if (flagOfLastData&0x2)
                {
                  saveMeterData(balancePn, port+1, tmpTime, lastCopyReqData, MONTH_BALANCE, MONTH_FREEZE_COPY_REQ,LENGTH_OF_REQ_RECORD);
                }
              }
            }
            usleep(1);
         	  
         	  //4-6存储测量点统计数据
            saveMeterData(balancePn, port+1, balanceTime, (INT8U *)&meterStatisRecord, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
            
    	      //4-7.链表移位
    	      tmpNode = tmpNode->next;
    	    }
    	    
    	    free(pMpLimitValue);   //释放测量点限值指针
    	    freeMpLink(mpLinkHead);
    	  }
    	  
    	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	 {
    	   printf("测量点结算完成\n");
    	 }
    	 
    	  //5.脉冲量电能量结算 ly,09-07-14,Add
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
    	           printf("脉冲测量点%d有数\n", pulseConfig.perPulseConfig[i].pn);
    	         }
    	         
    	         flagOfStoreData &= ~STORE_ENERGY_BALANCE_DATA;
    	         
    	         //初始化结算用缓存
    	         memset(balanceEnergyData,0xee,LEN_OF_ENERGY_BALANCE_RECORD);
               
               //结算当日电能量
               if (energyCompute(pulseConfig.perPulseConfig[i].pn,lastCopyEnergyData,balanceEnergyData, balanceTime, 3)==TRUE)
               {
            	    flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
               }
               
               usleep(50000);
    
               //结算当月电能量
               if (energyCompute(pulseConfig.perPulseConfig[i].pn,lastCopyEnergyData,balanceEnergyData, balanceTime, 4)==TRUE)
               {
            	    flagOfStoreData |= STORE_ENERGY_BALANCE_DATA;
               }
    
               usleep(50000);
               
    	         //存储测量点结算结果
               if (flagOfStoreData&STORE_ENERGY_BALANCE_DATA)
               {
                 //实时结算电能量数据存储于实时结算表
                 saveMeterData(pulseConfig.perPulseConfig[i].pn, 0, balanceTime, balanceEnergyData, REAL_BALANCE, REAL_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
              
                 if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
                 {
            	     //日结算电能量存储于日结算表
                   saveMeterData(pulseConfig.perPulseConfig[i].pn, 0, balanceTime, balanceEnergyData, DAY_BALANCE, DAY_BALANCE_POWER_DATA,LEN_OF_ENERGY_BALANCE_RECORD);
                 }
               }
             }
             else
             {
    	         if (debugInfo&PRINT_BALANCE_DEBUG)
    	         {
    	           printf("脉冲测量点%d无数\n", pulseConfig.perPulseConfig[i].pn);
    	         }
             }
             
             usleep(50000);
      	   }
           
           usleep(50000);
      	 }
      	#endif
    	  
    	  //6.总加组实时结算电能量数据：逐个检查测量点配置
    	  if (totalAddGroup.numberOfzjz != 0)
    	  {
    	    //总加组实时结算电能量数据
    	    for (i = 0; i < totalAddGroup.numberOfzjz; i++)
    	    {
    	  	  flagOfStoreData &= ~STORE_ZJZ_BALANCE_DATA;
    	  	  
    	  	  //初始化结算用缓存
            memset(balanceZjzData,0xee,LEN_OF_ZJZ_BALANCE_RECORD);
            
            //总加组电能量统计
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
    	  	  
    	  	  //总加组功率统计
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
            
    	  	  //存储总加结算结果
            if (flagOfStoreData&STORE_ZJZ_BALANCE_DATA)
            {
              saveMeterData(totalAddGroup.perZjz[i].zjzNo, port+1, balanceTime, balanceZjzData, REAL_BALANCE, GROUP_REAL_BALANCE,LEN_OF_ZJZ_BALANCE_RECORD);
              if (flagOfStoreData & STORE_DAY_BALANCE_DATA)
              {
                 //如果本次结算处在日结算或月结算的时刻,对应标志置位    
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
    	  
    	  //有功总电能量差动越限事件判断
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
    	  	 printf("开始判断有功总电能量差动越限事件\n");
    	  }
    	  for (i = 0; i < differenceConfig.numOfConfig; i++)
    	  {
    	  	tmpRecord = 0;
    	  	
    	  	if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	{
    	  	  printf("对比总加组号=%d,参照总加组号=%d\n",differenceConfig.perConfig[i].toCompare,differenceConfig.perConfig[i].toReference);
    	  	}
    	  	
    	  	tmpTime = balanceTime;
    	  	readMeterData(balanceZjzData, differenceConfig.perConfig[i].toCompare, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0);
    
    	  	if (balanceZjzData[GP_MONTH_WORK]==0xee)
    	  	{
    	  	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	  {
    	  	  	 printf("对比总加组电能量无数据\n");
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
    	  	  printf("对比总加组月总电能量=%d\n",compData);
    	    }
    	  	
    	  	tmpTime = balanceTime;
    	  	readMeterData(balanceZjzDatax, differenceConfig.perConfig[i].toReference, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0);
    	  	if (balanceZjzDatax[GP_MONTH_WORK]==0xee)
    	  	{
    	  	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  	  {
    	  	  	 printf("参考总加组电能量无数据\n");
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
    	  	  printf("参照总加组月总电能量=%d\n", diffData);
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
    	  		  	printf("有功总电能量差动组%d绝对对比越限发生未记录现在记录,绝对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 1;
                differenceConfig.perConfig[i].startStop |= 0x1;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  		  }
    	  		  else
    	  		  {
    	  		    printf("有功总电能量差动组%d绝对对比越限发生已记录,绝对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  }
    	  		}
    	  		else
    	  		{
    	  		  if ((differenceConfig.perConfig[i].startStop&0x1)==0x0)
    	  		  {	  			
    	  			  printf("有功总电能量差动组%d绝对对比恢复现在记录,绝对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 2;
                differenceConfig.perConfig[i].startStop &= 0xfe;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  			}
    	  			else
    	  			{
    	  			  printf("有功总电能量差动组%d绝对对比未越限,绝对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
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
    	  		    printf("有功总电能量差动组%d相对对比越限发生未记录现在记录,相对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 3;
                differenceConfig.perConfig[i].startStop |= 0x2;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  		  }
    	  		  else
    	  		  {
    	  		    printf("有功总电能量差动组%d相对对比越限发生已记录,相对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  }	  		  
    	  		}
    	  		else
    	  		{
    	  		  if ((differenceConfig.perConfig[i].startStop&0x2)==0x2)
    	  		  {
    	  			  printf("有功总电能量差动组%d相对对比未越限恢复现在记录,相对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  		  	tmpRecord = 4;
                differenceConfig.perConfig[i].startStop &= 0xfd;
      	        saveParameter(0x04, 15,(INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));
    	  		  }
    	  		  else
    	  		  {
    	  			  printf("有功总电能量差动组%d相对对比未越限,相对差值=%d\n", differenceConfig.perConfig[i].groupNum ,compData);
    	  			}
    	  		}
    	    }
    	    
    	    if (tmpRecord>0)
    	    {
            //生成有功电能量差动越限记录
            if ((eventRecordConfig.iEvent[2]&0x20)||(eventRecordConfig.nEvent[2] & 0x20))
            {
       	    	eventData[0] = 22;       //ERC
       	    	eventData[1] = 34;       //长度
       	    	 
       	    	//发生时间
              eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
              eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
              eventData[4] = sysTime.hour  /10<<4 | sysTime.hour  %10;
              eventData[5] = sysTime.day   /10<<4 | sysTime.day   %10;
              eventData[6] = sysTime.month /10<<4 | sysTime.month %10;
              eventData[7] = sysTime.year  /10<<4 | sysTime.year  %10;
               
              //电能量差动组号
              eventData[8] = differenceConfig.perConfig[i].groupNum;
              
              //发生标志
              if (tmpRecord==1 || tmpRecord==3)
              {
              	 eventData[8] |= 0x80;
              }
              
              
              //越限时对比总加组有功总电能量
              eventData[9]  = balanceZjzData[GP_MONTH_WORK+3];
              eventData[10] = balanceZjzData[GP_MONTH_WORK+4];
              eventData[11] = balanceZjzData[GP_MONTH_WORK+5];
              eventData[12] = ((balanceZjzData[GP_MONTH_WORK]&0x01)<<6)
                            |(balanceZjzData[GP_MONTH_WORK]&0x10)
                            |(balanceZjzData[GP_MONTH_WORK+6]&0x0f);
    
              //越限时参照总加组有功总电能量
              eventData[13] = balanceZjzDatax[GP_MONTH_WORK+3];
              eventData[14] = balanceZjzDatax[GP_MONTH_WORK+4];
              eventData[15] = balanceZjzDatax[GP_MONTH_WORK+5];
              eventData[16] = ((balanceZjzDatax[GP_MONTH_WORK]&0x01)<<6)
                            |(balanceZjzDatax[GP_MONTH_WORK]&0x10)
                            |(balanceZjzDatax[GP_MONTH_WORK+6]&0x0f);
              
              //越限时差动越限相对偏差值
    	  	    if (differenceConfig.perConfig[i].timeAndFlag&0x80)
    	  	    {
    	  	    	eventData[17] = 0x0;
    	  	    }
    	  	    else
    	  	    {
    	  	    	eventData[17] = compData;
    	  	    }
    	  	    
    	  	    //越限时差动越限绝对偏差值
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
    	  	    
    	  	    //对比总加组测量点数量n
    	  	    eventData[22] = 0;
    	  	    
    	  	    //对比总加组测量点1有功总电能示值
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
    
    	  	    //参照总加组测量点数量n
    	  	    tmpTailx = tmpTail++;
    	  	    eventData[tmpTailx] = 0;
    
    	  	    //参照总加组测量点1有功总电能示值
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
                printf("有功总电能量差动越限调用主动上报\n");
              }
                 
              activeReport3();   //主动上报事件
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
          printf("结算调用 主动上报\n");
        }
        
        activeReport3();                  //主动上报事件
        
        copyCtrl[port].ifRealBalance = 0; //实时结算标志复位
    
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	 printf("端口%d结算结束\n", port+1);
        }
        
        usleep(50000);
      }
      
      usleep(50000);
    }
}

/*******************************************************
函数名称: statisticMaxDemand
功能描述: 最大需量统计
调用函数:     
被调用函数:
输入参数:*copyBuff,上一次抄表的需量数据缓存
         *balanceBuff,参变量结算缓存
输出参数:  
返回值:void
*******************************************************/
void statisticMaxDemand(INT8U *copyReqBuff,INT8U *balanceParaBuff)
{
	  //继续上一次的统计结果进行统计计算
	  if (balanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)
	  {
      if (balanceParaBuff[MAX_TOTAL_REQ] != 0xEE)  //上一次统计结果存在
      {
        if (copyReqBuff[REQ_POSITIVE_WORK_OFFSET] != 0xEE)  //本次抄表成功返回
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
      else  //上一次统计结果不存在
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
    else  //重新开始统计过程
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
函数名称: statisticPower
功能描述: 功率统计 
调用函数:     
被调用函数:
输入参数:INT8U *copyParaBuff,上次抄表的参变量缓存
         INT8U *balanceParaBuff,结算参变量缓存
         INT8U statisInterval,统计间隔
         DATE_TIME statisTime,统计时间(上一次抄表时间)
输出参数:  
返回值:
*******************************************************/
void statisticPower(INT8U *copyParaBuff, INT8U *balanceParaBuff, INT8U statisInterval, DATE_TIME statisTime)
{
   INT16U  offset, offsetSta, tmpData;
   INT8U   i;

   //继续上一次的统计结果进行统计计算
   if (balanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)
   {
   	 offset = POWER_INSTANT_WORK;
   	 offsetSta = MAX_TOTAL_POWER;
     for (i = 0; i < 4; i++)
     {
       //有功功率统计
   	  if (copyParaBuff[offset] != 0xEE)
   	  {
   	    //有功功率最大值统计
   	    if (balanceParaBuff[offsetSta] == 0xEE)
   	    {
   	    	//有功功率最大值
   	    	balanceParaBuff[offsetSta]   = copyParaBuff[offset];
   	    	balanceParaBuff[offsetSta+1] = copyParaBuff[offset+1];
   	    	balanceParaBuff[offsetSta+2] = copyParaBuff[offset+2];
   	    	    
   	    	//日有功功率最大值时间
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
   	    	  //日有功功率最大值
   	    	  balanceParaBuff[offsetSta] = copyParaBuff[offset];
   	    	  balanceParaBuff[offsetSta+1] = copyParaBuff[offset+1];
   	    	  balanceParaBuff[offsetSta+2] = copyParaBuff[offset+2];
   	    	      
   	    	  //日有功功率最大值时间
   	    	  balanceParaBuff[offsetSta+3] = statisTime.minute;
   	    	  balanceParaBuff[offsetSta+4] = statisTime.hour;
   	    	  balanceParaBuff[offsetSta+5] = statisTime.day;
   	    	}
   	    }
   	    	  
   	    //日有功功率为零时间
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
   	     //有功功率最大值
   	     balanceParaBuff[offsetSta]   = copyParaBuff[offset];
   	     balanceParaBuff[offsetSta+1] = copyParaBuff[offset+1];
   	     balanceParaBuff[offsetSta+2] = copyParaBuff[offset+2];
   	    	    
   	     //日有功功率最大值时间
   	     balanceParaBuff[offsetSta+3] = statisTime.minute;
   	     balanceParaBuff[offsetSta+4] = statisTime.hour;
   	     balanceParaBuff[offsetSta+5] = statisTime.day;
   	   }

   	   if (copyParaBuff[offset] == 0x00 && copyParaBuff[offset+1] == 0x00 && copyParaBuff[offset+2] == 0x00)
   	   {
   	     //有功功率为零时间
   	     balanceParaBuff[offsetSta+6] = statisInterval&0xFF;
   	     balanceParaBuff[offsetSta+7] = statisInterval>>8&0xFF;
   	   }

 	     offset += 3;
 	     offsetSta += 8;
 	   }
 	 }
 	
 	 if (debugInfo&PRINT_BALANCE_DEBUG)
 	 {
 		 printf("statisticPower:有功功率最大值=%02x%02x%02x\n",balanceParaBuff[offsetSta+2],balanceParaBuff[offsetSta+1],balanceParaBuff[offsetSta+0]);
 		 printf("statisticPower:有功功率最大值时间=%02x %02x:%02x\n",balanceParaBuff[offsetSta+5],balanceParaBuff[offsetSta+4],balanceParaBuff[offsetSta+3]);
 		 printf("statisticPower:有功功率为零时间=%0d\n", balanceParaBuff[offsetSta+6] | balanceParaBuff[offsetSta+7]<<8);
 	 }
}

/*******************************************************
函数名称: statisticApparentPowerAndFactor
功能描述: 视在功率统计及视在功率越限事件判定及功率因数区段统计
调用函数:     
被调用函数:
输入参数:INT16U pn,测量点(1-2040)
         INT8U *copyParaBuff,上次抄表参变量缓存 
         INT8U *balanceParaBuff,结算参变量缓存
         MEASUREPOINT_LIMIT_PARA *pLimit,本测量点限值参数指针
         INT8U statisInterval,统计间隔
输出参数:  
返回值:
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
  	  
    INT32U      appUpUpResume,appUpResume;    //视在功率恢复限值
    
    //重新开始统计
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

    //视在功率统计
    if (pLimit != NULL)
    {
      if (copyParaBuff[POWER_INSTANT_WORK] != 0xEE && copyParaBuff[TOTAL_POWER_FACTOR] != 0xEE)
      {
        //通过有功功率和功率因数计算视在功率
        //视在功率=有功功率/功率因数
        sumInt = copyParaBuff[POWER_INSTANT_WORK] | copyParaBuff[POWER_INSTANT_WORK+1]<<8 | (copyParaBuff[POWER_INSTANT_WORK+2]&0x7f)<<16;  	        
        sumInt = bcdToHex(sumInt);
        rawInt = copyParaBuff[TOTAL_POWER_FACTOR] | (copyParaBuff[TOTAL_POWER_FACTOR+1]&0x7f)<<8;
        rawInt = bcdToHex(rawInt);
        
        if (rawInt==0)
        {
          //ly,2011-10-10,发现这个错误
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("视在功率统计:功率因数为0,不能计算出视在功率\n");
          }
          
          goto factorPoint;
        }
        else
        {
          appPower = sumInt*1000/rawInt;
        }
        
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("视在功率统计:视在功率=%d\n",appPower);
        }
        
        //视在功率上上限
        appPowerLimit = pLimit->pSuperiodLimit[0] | pLimit->pSuperiodLimit[1]<<8 | pLimit->pSuperiodLimit[2]<<16;
        appPowerLimit = bcdToHex(appPowerLimit);
        
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("视在功率统计:上上限限值=%d\n",appPowerLimit);
        }
        
        appUpResume = calcResumeLimit(pLimit->pUpLimit[0] | pLimit->pUpLimit[1]<<8 | pLimit->pUpLimit[2]<<16, pLimit->pUpResume[0] | pLimit->pUpResume[1]<<8);
        appUpUpResume = calcResumeLimit(pLimit->pSuperiodLimit[0] | pLimit->pSuperiodLimit[1]<<8 | pLimit->pSuperiodLimit[2]<<16, pLimit->pSuperiodResume[0] | pLimit->pSuperiodResume[1]<<8);      

        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
          printf("视在功率统计:上限恢复限值=%d\n",appUpResume);
          printf("视在功率统计:上上限恢复限值=%d\n",appUpUpResume);
        }
                
        if (appPower != 0)
        {
          //视在功率越上上限
          if (appPower > appPowerLimit)
          {
            if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
            {
              //第一次发生越上上限,记录越限事件
              if ((pStatisRecord->apparentPower&0x01) == 0x00)
              {
          	     if (pStatisRecord->apparentUpUpTime.year==0xff)
          	     {
          	   	   pStatisRecord->apparentUpUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pSuperiodTimes, 0);
          	   	   
          	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
          	   	   {
          	   	     printf("视在功率统计:越上上限发生,%d分钟后记录\n",pLimit->pSuperiodTimes);
          	   	   }
          	     }
          	     else
          	     {
                   if (compareTwoTime(pStatisRecord->apparentUpUpTime,sysTime))
                   {
                 	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   {
                 	     printf("视在功率统计:越上上限发生持续时间已到\n");
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
            
            //越上上限时间
            if (balanceParaBuff[APPARENT_POWER_UP_UP_TIME]==0xEE)
            {
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = statisInterval&0xff;
              //balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = statisInterval>>8&0xff;
              //ly,2011-10-26,发现错误(未加1)
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = statisInterval>>8&0xff;
            }
            else
            {
              tmpData = balanceParaBuff[APPARENT_POWER_UP_UP_TIME] | balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1]<<8;
              tmpData += statisInterval;
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = tmpData&0xFF;
              //balanceParaBuff[APPARENT_POWER_UP_UP_TIME] = tmpData>>8&0xFF;
              //ly,2011-10-26,发现错误(未加1)
              balanceParaBuff[APPARENT_POWER_UP_UP_TIME+1] = tmpData>>8&0xFF;
            }
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
            	 printf("视在功率统计越上上限时间=%d\n", tmpData);
            }
            
            //ly,2011-10-20,add,越上上限的话,肯定越了上限,因此加上越上限时间
            //越上限时间
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
            	 printf("视在功率统计越上限(越上上限时)时间=%d\n", tmpData);
            }
          }
          else
          {
            if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
            {
            	//越上上限恢复
               if (appPower<=appUpUpResume)
               {
              	//曾经发生越上上限,记录越上上限恢复
                if ((pStatisRecord->apparentPower&0x01) == 0x01)
                {
            	     if (pStatisRecord->apparentUpUpTime.year==0xff)
            	     {
            	   	   pStatisRecord->apparentUpUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pSuperiodTimes, 0);
            	   	   printf("视在功率统计:越上上限开始恢复,%d分钟后记录\n",pLimit->pSuperiodTimes);
            	     }
            	     else
            	     {
                     if (compareTwoTime(pStatisRecord->apparentUpUpTime,sysTime))
                     {
                  	   if (debugInfo&PRINT_BALANCE_DEBUG)
                  	   {
                  	     printf("视在功率统计:上上限恢复持续时间已到\n");
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
            
            //视在功率上限
            appPowerLimit = pLimit->pUpLimit[0] | pLimit->pUpLimit[1]<<8 | pLimit->pUpLimit[2]<<16;
            appPowerLimit = bcdToHex(appPowerLimit);
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("视在功率统计上限限值%d\n",appPowerLimit);
            }
          
            //视在功率越上限
            if (appPower > appPowerLimit)
            {
              if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
              {
                //第一次发生越上限,记录越限事件
                if ((pStatisRecord->apparentPower&0x10) == 0x00)
                {
            	    if (pStatisRecord->apparentUpTime.year==0xff)
            	    {
            	   	  pStatisRecord->apparentUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pUpTimes, 0);
            	   	   
            	   	  if (debugInfo&PRINT_BALANCE_DEBUG)
            	   	  {
            	   	    printf("视在功率统计:越上限发生,%d分钟后记录\n",pLimit->pUpTimes);
            	   	  }
            	    }
            	    else
            	    {
                    if (compareTwoTime(pStatisRecord->apparentUpTime,sysTime))
                    {
                   	  if (debugInfo&PRINT_BALANCE_DEBUG)
                   	  {
                   	    printf("视在功率统计:越上限发生持续时间已到\n");
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
              
              //越上限时间
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
            	  printf("视在功率统计越上限时间=%d\n", tmpData);
              }
            }
            else
            {
              if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
              {
              	//越上限恢复
                 if (appPower<=appUpResume)
                 {
                	//曾经发生越上限,记录越上限恢复
                  if ((pStatisRecord->apparentPower&0x10) == 0x10)
                  {
              	     if (pStatisRecord->apparentUpTime.year==0xff)
              	     {
              	   	   pStatisRecord->apparentUpTime = nextTime(timeBcdToHex(statisTime), pLimit->pUpTimes, 0);
              	   	   printf("视在功率统计:越上限开始恢复,%d分钟后记录\n",pLimit->pUpTimes);
              	     }
              	     else
              	     {
                       if (compareTwoTime(pStatisRecord->apparentUpTime,sysTime))
                       {
                    	   if (debugInfo&PRINT_BALANCE_DEBUG)
                    	   {
                    	     printf("视在功率统计:上限恢复持续时间已到\n");
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
  	//2.功率因数区段累计时间
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("开始统计功率因数区段累计时间\n");
    }
    if (pPowerSegLimit!=NULL)
    {
      totalFactor = bcdToHex((copyParaBuff[TOTAL_POWER_FACTOR]|copyParaBuff[TOTAL_POWER_FACTOR+1]<<8)&0x7fff);
      factorSeg1  = bcdToHex((pPowerSegLimit->segLimit1[0] | pPowerSegLimit->segLimit1[1]<<8)&0x7fff);
      factorSeg2  = bcdToHex((pPowerSegLimit->segLimit2[0] | pPowerSegLimit->segLimit2[1]<<8)&0x7fff);
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
      	printf("功率因数区段累计:总功率因数=%d\n", totalFactor);
      	
      	printf("功率因数区段累计:分段限值1=%d\n", factorSeg1);
      	printf("功率因数区段累计:分段限值2=%d\n", factorSeg2);
      }
      
      if (totalFactor<factorSeg1)    //功率因数<定值1
      {
      	 tmpData = balanceParaBuff[FACTOR_SEG_1] | balanceParaBuff[FACTOR_SEG_1+1]<<8;
         if (debugInfo&PRINT_BALANCE_DEBUG)
         {
          	printf("功率因数区段累计:功率因数<定值1(原来的分钟数=%d)\n", tmpData);
         }
      	 tmpData += statisInterval;
      	 
      	 balanceParaBuff[FACTOR_SEG_1] = tmpData & 0xff;
      	 balanceParaBuff[FACTOR_SEG_1+1] = tmpData>>8 & 0xff;
         
      }
      else
      {
      	if (totalFactor<factorSeg2)  //定值1<=功率因数<定值2
      	{
      	  tmpData = balanceParaBuff[FACTOR_SEG_2] | balanceParaBuff[FACTOR_SEG_2+1]<<8;
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
          	 printf("功率因数区段累计:定值1<=功率因数<定值2(原来的分钟数=%d)\n", tmpData);
          }
      	  tmpData += statisInterval;
      	 
      	  balanceParaBuff[FACTOR_SEG_2] = tmpData & 0xff;
      	  balanceParaBuff[FACTOR_SEG_2+1] = tmpData>>8 & 0xff;      		 
      	}
      	else                         //功率因数>=定值2
      	{
      	  tmpData = balanceParaBuff[FACTOR_SEG_3] | balanceParaBuff[FACTOR_SEG_3+1]<<8;
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
          	 printf("功率因数区段累计:功率因数>=定值2(原来的分钟数=%d)\n", tmpData);
          }
      	  tmpData += statisInterval;
      	  
      	  balanceParaBuff[FACTOR_SEG_3] = tmpData & 0xff;
      	  balanceParaBuff[FACTOR_SEG_3+1] = tmpData>>8 & 0xff;      		 
      	}
      }
    }
}

/*******************************************************
函数名称: statisticVoltage
功能描述: 电压统计 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:
*******************************************************/
void statisticVoltage(INT16U pn, INT8U *pCopyParaBuff, INT8U *pCopyEnergyBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit,INT8U statisInterval,DATE_TIME statisTime)
{
   INT32U             voltage;
   INT16U             volLoss;
   INT8U              j, phase, bitShift;
   INT16U             offset, offsetCurrent, offsetSta, offsetExtre;
   INT16U             tmpDataShort;
	 
	 MEASURE_POINT_PARA pointPara;
	 INT16U             vUpUpResume,vDownDownResume;   //恢复值

   offset = VOLTAGE_PHASE_A;
   offsetCurrent = CURRENT_PHASE_A;
   offsetSta = VOL_A_UP_UP_TIME;
   offsetExtre = VOL_A_MAX;
   bitShift = 0x01;
  	
   volLoss = 0xFFFF;

   //取额定电压的80%为失压限值
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
       printf("电压统计:上上限恢复限值=%d\n",vUpUpResume);
       printf("电压统计:下下限恢复限值=%d\n",vDownDownResume);
     }
 	 }
   
   if (debugInfo&PRINT_BALANCE_DEBUG)
   {
    printf("电压统计:失压限值=%d\n",volLoss);

    if (pLimit!=NULL)
    {
      printf("电压统计:上上限值=%x\n",pLimit->vSuperiodLimit);
      printf("电压统计:下下限值=%x\n",pLimit->vDownDownLimit);
    }
    else
    {
    	printf("电压统计:未设置测量点%d限值参数\n", pn);
    }
   }
   
   for (phase = 1; phase <= 3; phase++)
   {
    	//继续前一次统计
    	if (pBalanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)
      {
 	    	if (pCopyParaBuff[offset] != 0xEE)
   	    {
   	    	//电压异常事件：失压判断
 	        if (volLoss != 0xFFFF)
 	        {
 	        	voltage = (INT16U)bcdToHex(pCopyParaBuff[offset]|pCopyParaBuff[offset+1]<<8);
 	        	
      	    if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
            {
      	       //电压小于失压值且电流不等于0判断为失压
      	       if (voltage < volLoss && (pCopyParaBuff[offsetCurrent] != 0x00 || pCopyParaBuff[offsetCurrent+1] != 0x00 ||pCopyParaBuff[offsetCurrent+2] != 0x00))
      	       {
                  //失压发生
                  if ((pStatisRecord->loseVoltage&bitShift) == 0x00)
                  {
                     if (debugInfo&PRINT_BALANCE_DEBUG)
                     {
                      printf("电压统计:%d相失压发生\n",phase);
                     }
                     
                     pStatisRecord->loseVoltage |= bitShift;
                     vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, FALSE, statisTime);
                  }
               }
      	       else
      	       {
      	         //失压恢复
      	         if ((pStatisRecord->loseVoltage&bitShift) == bitShift)
                 {
                    if (debugInfo&PRINT_BALANCE_DEBUG)
                    {
                      printf("电压统计:%d相恢复发生\n",phase);
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
            printf("电压统计:%d相电压值=%x\n",phase,voltage);
          }
   	    	
 	    	  if (pLimit != NULL)
 	    	  {
     	      //大于断相门限,进行越限判断
     	      if (voltage >= pLimit->vPhaseDownLimit)
     	      {
     	      	//记录断相恢复事件
     	      	if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
              {
                 if ((pStatisRecord->phaseBreak&bitShift) == bitShift)
                 {
                   if (debugInfo&PRINT_BALANCE_DEBUG)
                   {
                     printf("电压统计:%d相断相恢复\n",phase);
                   }
                   
                   pStatisRecord->phaseBreak &= (~bitShift);
                   vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, TRUE, statisTime);
                 }
     	      	}
     	      	  
     	      	//越上上限
              if (voltage >= pLimit->vSuperiodLimit)
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {
                   //第一次发生越上上限,记录越限事件
                   if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                   {
               	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
               	   	   
               	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
               	   	   {
               	   	     printf("电压统计:越上上限开始发生,%d分钟后记录\n",pLimit->vUpUpTimes);
               	   	   }
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电压统计:电压越上上限发生持续时间已到\n");
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
                  printf("电压统计:%d相越上上限时间=%d\n", phase, tmpDataShort);
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
                   printf("电压统计:%d相越上限(在越上上限的同时)时间=%d\n", phase, tmpDataShort);
                }
              }
              else   
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {
                  if (bcdToHex(voltage) <=vUpUpResume)
                  {
                 	 //曾经发生越上上限,记录越上上限恢复
                   if (pStatisRecord->vOverLimit&bitShift)
                   {
               	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
               	   	   printf("电压统计:越上上限开始恢复,%d分钟后记录\n",pLimit->vUpUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电压统计:越上上限恢复持续时间已到\n");
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
                 	
              //越上限
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
                   printf("电压统计:%d相越上限时间=%d\n", phase, tmpDataShort);
                 }
              }
                   
              //越下下限
              bitShift <<= 1;
              if (voltage <= pLimit->vDownDownLimit && voltage != 0)   //越下下限
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {	  
                  //第一次发生越限,记录越限事件
                  if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                  {                   	
               	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
               	   	   
               	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
               	   	   {
               	   	     printf("电压统计:电压越下下限发生,%d分钟后记录\n",pLimit->vDownDownTimes);
               	   	   }
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电压统计:电压越下下限发生持续时间已到\n");
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
                  printf("电压统计:%d相越下下限时间=%d\n", phase, tmpDataShort);
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
                  printf("电压统计:%d相越下限(在越下下限的同时统计)时间=%d\n", phase, tmpDataShort);
                }
              }
              else
              {
                if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                {
                 	 //曾经发生越下下限,记录越限恢复事件
                  if (bcdToHex(voltage) >=vDownDownResume)
                  {
                   if (pStatisRecord->vOverLimit&bitShift)
                   {
               	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
               	   	   
               	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
               	   	   {
               	   	     printf("电压统计:电压越下下限开始恢复,%d分钟后记录\n",pLimit->vDownDownTimes);
               	   	   }
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电压统计:电压下下限恢复持续时间已到\n");
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
                 
              //越下限
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
                   printf("电压统计:%d相越下限时间=%d\n", phase, tmpDataShort);
                 }
              }
                       
              //合格      
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
                   printf("电压统计:%d相合格时间=%d\n", phase, tmpDataShort);
                 }
              }
            }
            else  //判断断相
            {
              if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
              {
                //电流等于0判断为断相
                if (pCopyParaBuff[offsetCurrent] == 0x00 && pCopyParaBuff[offsetCurrent+1] == 0x00 && pCopyParaBuff[offsetCurrent+2] == 0x00)
                {
                  //记录断相事件
                  if ((pStatisRecord->phaseBreak&bitShift) == 0x00)
                  {
                    if (debugInfo&PRINT_BALANCE_DEBUG)
                    {
                      printf("电压统计:%d相断相发生\n",phase);
                    }

                    pStatisRecord->phaseBreak |= bitShift;
                    vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, FALSE, statisTime);
                  }
                }
              }
            }
   	      }
   	      else  //未设置电压限值,记为合格
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
   	      
   	      //日电压最大值及发生时间
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
           
          //日电压最小值及发生时间
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
           
          //日电压平均值
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
	    else //开始新的统计
	    {
        if (pCopyParaBuff[offset] != 0xEE)
   	    {
   	    	voltage = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8;
 	    	  
 	    	  if (pLimit!=NULL)
 	    	  {
     	      //电压异常事件：失压判断
     	      if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
            {
     	        if (voltage < volLoss && (pCopyParaBuff[offsetCurrent] != 0x00 || pCopyParaBuff[offsetCurrent+1] != 0x00 || pCopyParaBuff[offsetCurrent+2] != 0x00))
     	        {
                 //失压发生
                 if ((pStatisRecord->loseVoltage&bitShift) == 0x00)
                 {
                    pStatisRecord->loseVoltage |= bitShift;  
                    vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, FALSE, statisTime);
                 }
               }
     	        else
     	        {
     	          //失压恢复
     	          if (pStatisRecord->loseVoltage&bitShift)
                 {
                   pStatisRecord->loseVoltage &= (~bitShift);
                   vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 2, TRUE, statisTime);
                 }
     	        }
     	      }
     	      
     	      //大于断相门限,进行越限判断
     	      if (voltage >= pLimit->vPhaseDownLimit)
     	      {
     	      	  //记录断相恢复事件
     	      	  if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
                 {
                   if (pStatisRecord->phaseBreak&bitShift)
                   {
                     pStatisRecord->phaseBreak &= (~bitShift);
                     vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, TRUE, statisTime);
                   }
     	      	  }
     	      	  
     	      	  //越上上限
                 if (voltage >= pLimit->vSuperiodLimit)
                 {
                   if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                   {
                     //第一次发生越上上限,记录越限事件
                     if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                     {
                 	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
                 	   	   printf("电压统计(新):越上上限开始发生,%d分钟后记录\n",pLimit->vUpUpTimes);
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("电压统计(新):电压越上上限发生持续时间已到\n");
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
                     	 //曾经发生越上上限,记录越上上限恢复
                       if (pStatisRecord->vOverLimit&bitShift)
                       {
                   	     if (pStatisRecord->vUpUpTime[phase-1].year==0xff)
                   	     {
                   	   	   pStatisRecord->vUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vUpUpTimes, 0);
                   	   	   printf("电压统计(新):越上上限开始恢复,%d分钟后记录\n",pLimit->vUpUpTimes);
                   	     }
                   	     else
                   	     {
      	                   if (compareTwoTime(pStatisRecord->vUpUpTime[phase-1],sysTime))
      	                   {
      	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
      	                  	 {
      	                  	   printf("电压统计(新):电压上上限恢复持续时间已到\n");
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
                 	
               	//越上限
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
                   
                 //越下下限
                 bitShift <<= 1;
                 if (voltage <= pLimit->vDownDownLimit && voltage != 0)   //越下下限
                 {
                   if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
                   {	  
                      //第一次发生越限,记录越限事件
                      if ((pStatisRecord->vOverLimit&bitShift) == 0x00)
                      {
                   	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
                   	     {
                   	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
                   	   	   
                   	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                   	   	   {
                   	   	     printf("电压统计(新):电压越下下限发生,%d分钟后记录\n",pLimit->vDownDownTimes);
                   	   	   }
                   	     }
                   	     else
                   	     {
      	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
      	                   {
      	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
      	                  	 {
      	                  	   printf("电压统计(新):电压越下下限发生持续时间已到\n");
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
                 	  //曾经发生越下下限,记录越限恢复事件
                     if (bcdToHex(voltage) >=vDownDownResume)
                     {
                      if (pStatisRecord->vOverLimit&bitShift)
                      {
                  	     if (pStatisRecord->vDownDownTime[phase-1].year==0xff)
                  	     {
                  	   	   pStatisRecord->vDownDownTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->vDownDownTimes, 0);
                  	   	   
                  	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                  	   	   {
                  	   	     printf("电压统计(新):电压越下下限开始恢复,%d分钟后记录\n",pLimit->vDownDownTimes);
                  	   	   }
                  	     }
                  	     else
                  	     {
     	                   if (compareTwoTime(pStatisRecord->vDownDownTime[phase-1],sysTime))
     	                   {
     	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
     	                  	 {
     	                  	   printf("电压统计(新):电压下下限恢复持续时间已到\n");
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
                       
               	//越下限
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
                       
                 //合格      
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
             else  //判断断相
             {
               if ((eventRecordConfig.iEvent[1] & 0x02) || (eventRecordConfig.nEvent[1] & 0x02))
               {
                 //电流等于0判断为断相?
                 if (pCopyParaBuff[offsetCurrent] == 0x00 && pCopyParaBuff[offsetCurrent+1] == 0x00 && pCopyParaBuff[offsetCurrent+2] == 0x00)
                 {
                   //记录断相事件
                   if ((pStatisRecord->phaseBreak&bitShift) == 0x00)
                   {
                     pStatisRecord->phaseBreak |= bitShift;
                     vAbnormalEvent(pCopyParaBuff, pCopyEnergyBuff, phase, pn, 1, FALSE, statisTime);
                   }
                 }
               }
             }
   	      }
   	      else  //未设置电压限值,记为合格
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
   	      
   	      //日电压最大值及发生时间
          pBalanceParaBuff[offsetExtre] = pCopyParaBuff[offset];
          pBalanceParaBuff[offsetExtre+1] = pCopyParaBuff[offset+1];
          pBalanceParaBuff[offsetExtre+2] = statisTime.minute;
          pBalanceParaBuff[offsetExtre+3] = statisTime.hour;
          pBalanceParaBuff[offsetExtre+4] = statisTime.day;
          
          //日电压最小值及发生时间
          pBalanceParaBuff[offsetExtre+5] = pCopyParaBuff[offset];
          pBalanceParaBuff[offsetExtre+6] = pCopyParaBuff[offset+1];
          pBalanceParaBuff[offsetExtre+7] = statisTime.minute;
          pBalanceParaBuff[offsetExtre+8] = statisTime.hour;
          pBalanceParaBuff[offsetExtre+9] = statisTime.day;
           
          //日电压平均值
          voltage = (INT16U)bcdToHex(pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8);
          pBalanceParaBuff[offsetExtre+10] = voltage&0xFF;
          pBalanceParaBuff[offsetExtre+11] = (voltage>>8)&0xFF;
          pBalanceParaBuff[offsetExtre+12] = 0x01;
          pBalanceParaBuff[offsetExtre+13] = 0x00;
   	    }
   	    else
   	    {
   	    	//电压越限时间
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
           
          //电压平均值
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
           
          //日电压平均值
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
函数名称: statisticCurrent
功能描述: 电流统计 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:
*******************************************************/

void statisticCurrent(INT16U pn, INT8U *pCopyParaBuff, INT8U *pBalanceParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, MEASUREPOINT_LIMIT_PARA *pLimit, INT8U statisInterval, DATE_TIME statisTime)
{
    INT32U  current;
    INT32U  tmpDataShort;
    INT8U   phase;
    INT16U  offset, offsetSta;
    INT8U   bitShift;
    INT32U  cUpUpLimit, cUpLimit;    //电流限值
    INT32U  cUpUpResume, cUpResume;  //电流恢复限值
    
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
    	  printf("电流统计:上限值=%x\n",cUpLimit);         //BCD
    	 
    	  printf("电流统计:上限恢复值=%d\n",cUpResume);    //16进制
    	 
    	  printf("电流统计:上上限值=%x\n",cUpUpLimit);     //BCD
    	 
    	  printf("电流统计:上上限恢复值=%d\n",cUpUpResume);//16进制
    	}
    }
    
    //A,B,C相电流及零序电流判断
    for (phase = 1; phase <= 4; phase++)
    {
 	    if (pBalanceParaBuff[NEXT_NEW_INSTANCE] != START_NEW_INSTANCE)  //继续前一次统计
 	    {
 	      if (pCopyParaBuff[offset] != 0xEE)
     	  {
   	      current = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8 | (pCopyParaBuff[offset+2]&0x7f)<<16;

          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("电流统计:%d相电流值=%x\n",phase,current);
          }
           
   	      if (pLimit!=NULL)
   	      {
            //电流越上限
       	 	  if (current >= cUpLimit && current < cUpUpLimit)
            {
               if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
                  //第一次发生越上限，记录越上限事件
                 	if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
                 	{
                 	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("电流统计:越上限开始发生,%d分钟后记录\n",pLimit->cUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("电流统计:越上限发生持续时间已到\n");
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
         	 	      
         	 	   //统计越上限时间         	 	   
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
   	      	      printf("电流统计:%d相越上限时间=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	           }
            }
          	else
          	{
          	   //曾经有越上限事件发生，记录越上限事件恢复
          	   if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {	
                  if (bcdToHex(current)<=cUpResume)
                  {
          	    	 if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
               	   	   printf("电流统计:越上限开始恢复,%d分钟后记录\n",pLimit->cUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电流统计:越上限恢复持续时间已到\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpTime[phase-1].year = 0xff;
               	   
             	    	     //记录越上限恢复
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
            //电流越上上限
            if (current >= cUpUpLimit)  
            {
               if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
               	  //第一次发生越限，记录越限事件
               	  if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
               	  {
                 	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("电流统计:越上上限开始发生,%d分钟后记录\n",pLimit->cUpUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("电流统计:越上上限发生持续时间已到\n");
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
               	
               	//越上上限时间
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
   	      	      printf("电流统计:%d相越上限时间=%d\n", phase, pBalanceParaBuff[offsetSta] | pBalanceParaBuff[offsetSta+1]<<8);
   	           }
               
               //ly,2011-10-20,add,越上上限的话,肯定越了上限,因此加上越上限时间
         	 	   //统计越上限时间
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
   	      	      printf("电流统计:%d相越上限(在上上限的同时)时间=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	           }
            }
            else   
       	    {
       	 	     if ((eventRecordConfig.iEvent[3]&0x01) || (eventRecordConfig.nEvent[3]&0x01))
               {
                 	//曾经有越上上限事件发生,记录越上上限恢复
                  if (bcdToHex(current)<=cUpUpResume)
                  {
       	 	         if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
               	   	   printf("电流统计:越上上限开始恢复,%d分钟后记录\n",pLimit->cUpUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电流统计:越上上限恢复持续时间已到\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
               	   
             	    	     //记录越上上限恢复
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
           	//越上上限时间
            pBalanceParaBuff[offsetSta] = 0x00;
          	pBalanceParaBuff[offsetSta+1] = 0x00;
          	//越上限时间
          	pBalanceParaBuff[offsetSta+2] = 0x00;
          	pBalanceParaBuff[offsetSta+3] = 0x00;
          }
           
          //电流最大值
          if (pBalanceParaBuff[offsetSta+4]!=0xee && pBalanceParaBuff[offsetSta+5]!=0xee && pBalanceParaBuff[offsetSta+6]!=0xee)
          {
            tmpDataShort = pBalanceParaBuff[offsetSta+4] | pBalanceParaBuff[offsetSta+5]<<8 | pBalanceParaBuff[offsetSta+6]<<16;
            if (tmpDataShort < current)  //电流最大值
            {
               pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[offset];
               pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[offset+1];
               pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[offset+2];
               
               pBalanceParaBuff[offsetSta+7] = statisTime.minute;
               pBalanceParaBuff[offsetSta+8] = statisTime.hour;
               pBalanceParaBuff[offsetSta+9] = statisTime.day;
            }
          }
          else    //初值
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
      else  //重新开始新的统计
      {
        if (pCopyParaBuff[offset] != 0xEE)
     	  {
   	      current = pCopyParaBuff[offset] | pCopyParaBuff[offset+1]<<8 | (pCopyParaBuff[offset+2]&0x7f)<<16;
   	      
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	      	 printf("电流统计(重新开始统计):%d相电流=%x\n", phase, current);
   	      }

   	      if (pLimit!=NULL)
   	      {
             //电流越上限  
       	 	   if (current >= cUpLimit && current < cUpUpLimit)
             {
               if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
                 	//第一次发生越上限，记录越上限事件
                 	if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
                 	{
                 	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("电流统计:越上限开始发生,%d分钟后记录\n",pLimit->cUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("电流统计:越上限发生持续时间已到\n");
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
         	 	      
         	 	   //越上限时间
         	 	   pBalanceParaBuff[offsetSta] = 0x00;
          	   pBalanceParaBuff[offsetSta+1] = 0x00;
         	 	   pBalanceParaBuff[offsetSta+2] = statisInterval&0xFF;
            	 pBalanceParaBuff[offsetSta+3] = (statisInterval>>8)&0xFF;
   	           
   	           if (debugInfo&PRINT_BALANCE_DEBUG)
   	           {
   	      	      printf("电流统计(重新开始统计):%d相越上限时间=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	           }
             }
          	 else
          	 {
          	   if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
               {
          	    	//曾经有越上限事件发生，记录越上限事件恢复
                  if (bcdToHex(current)<=cUpResume)
                  {
          	    	 if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpTimes, 0);
               	   	   printf("电流统计:越上限开始恢复,%d分钟后记录\n",pLimit->cUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电流统计:越上限恢复持续时间已到\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpTime[phase-1].year = 0xff;
               	   
             	    	     //记录越上限恢复
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
          	    
             //电流越上上限
             bitShift <<= 1;
             if (current >= cUpUpLimit)  
             {
               	if ((eventRecordConfig.iEvent[3] & 0x01) || (eventRecordConfig.nEvent[3] & 0x01))
                {
                 	//第一次发生越限，记录越限事件
               	  if ((pStatisRecord->cOverLimit&bitShift) == 0x00)
               	  {
                 	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
                 	     {
                 	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
                 	   	   
                 	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
                 	   	   {
                 	   	     printf("电流统计:越上上限开始发生,%d分钟后记录\n",pLimit->cUpUpTimes);
                 	   	   }
                 	     }
                 	     else
                 	     {
    	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
    	                   {
    	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
    	                  	 {
    	                  	   printf("电流统计:越上上限发生持续时间已到\n");
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
               	
               	//越上上限时间
               	pBalanceParaBuff[offsetSta] = statisInterval&0xFF;
          	    pBalanceParaBuff[offsetSta+1] = (statisInterval>>8)&0xFF;
   	            if (debugInfo&PRINT_BALANCE_DEBUG)
   	            {
   	      	      printf("电流统计(重新开始统计):%d相越上上限时间=%d\n", phase, pBalanceParaBuff[offsetSta] | pBalanceParaBuff[offsetSta+1]<<8);
   	            }
         	 	    
         	 	    //ly,2011-10-20,add
         	 	    //越上限时间
         	 	    pBalanceParaBuff[offsetSta+2] = statisInterval&0xFF;
            	  pBalanceParaBuff[offsetSta+3] = (statisInterval>>8)&0xFF;
   	           
   	            if (debugInfo&PRINT_BALANCE_DEBUG)
   	            {
   	      	      printf("电流统计(重新开始统计):%d相越上限(在越上上限的同时)时间=%d\n", phase, pBalanceParaBuff[offsetSta+2] | pBalanceParaBuff[offsetSta+3]<<8);
   	            }
             }
             else   
       	     {
       	 	      if ((eventRecordConfig.iEvent[3]&0x01) || (eventRecordConfig.nEvent[3]&0x01))
                {
       	 	        //曾经有越上上限事件发生,记录越上上限恢复
                  if (bcdToHex(current)<=cUpUpResume)
                  {
       	 	         if ((pStatisRecord->cOverLimit&bitShift) == bitShift)
                   {
               	     if (pStatisRecord->cUpUpTime[phase-1].year==0xff)
               	     {
               	   	   pStatisRecord->cUpUpTime[phase-1] = nextTime(timeBcdToHex(statisTime), pLimit->cUpUpTimes, 0);
               	   	   printf("电流统计(重新开始统计):越上上限开始恢复,%d分钟后记录\n",pLimit->cUpUpTimes);
               	     }
               	     else
               	     {
  	                   if (compareTwoTime(pStatisRecord->cUpUpTime[phase-1],sysTime))
  	                   {
  	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
  	                  	 {
  	                  	   printf("电流统计(重新开始统计):越上上限恢复持续时间已到\n");
  	                  	 }
  	                  	 
  	                  	 pStatisRecord->cUpUpTime[phase-1].year = 0xff;
               	   
             	    	     //记录越上上限恢复
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
         	 	 
         	 	 //电流在上限以下,置越上限和越上上限时间为0
         	 	 if (current<cUpLimit)
         	 	 {
           	   //越上上限时间
               pBalanceParaBuff[offsetSta] = 0x00;
          	   pBalanceParaBuff[offsetSta+1] = 0x00;
          	  
          	   //越上限时间
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
           
          //电流最大值
          pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[offset];
          pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[offset+1];
          pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[offset+2];
               
          pBalanceParaBuff[offsetSta+7] = statisTime.minute;
          pBalanceParaBuff[offsetSta+8] = statisTime.hour;
          pBalanceParaBuff[offsetSta+9] = statisTime.day;
        }
        else  //没有成功返回本次抄表数据，越限时间及最大值置为0
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
函数名称: 电压电流不平衡度越限事件处理
功能描述: 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值： 
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
        //必须对三相电压一起统计
    	  if (pCopyParaBuff[VOLTAGE_PHASE_A] != 0xEE && pCopyParaBuff != 0xEE && pCopyParaBuff[VOLTAGE_PHASE_C] != 0xEE)
      	{
      	    //xxx.x
      	    tmpDataA = pCopyParaBuff[VOLTAGE_PHASE_A] | pCopyParaBuff[VOLTAGE_PHASE_A+1]<<8;
      	    tmpDataB = pCopyParaBuff[VOLTAGE_PHASE_B] | pCopyParaBuff[VOLTAGE_PHASE_B+1]<<8;
      	    tmpDataC = pCopyParaBuff[VOLTAGE_PHASE_C] | pCopyParaBuff[VOLTAGE_PHASE_C+1]<<8;
            
            //xxx.x
      	    tmpDataLimit = pLimit->uPhaseUnbalance[0]
      	                    | (pLimit->uPhaseUnbalance[1]&0x7f)<<8;
      	    
      	    //数据格式转换bcd => hex
      	    tmpDataA = bcdToHex(tmpDataA);
      	    tmpDataB = bcdToHex(tmpDataB);
      	    tmpDataC = bcdToHex(tmpDataC);
      	    tmpDataLimit = bcdToHex(tmpDataLimit);
      	   
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
             printf("电压不平衡度:限值=%d\n",tmpDataLimit);
           }

      	   vUnBalanceResume = calcResumeLimit(pLimit->uPhaseUnbalance[0] | pLimit->uPhaseUnbalance[1]<<8 , pLimit->uPhaseUnResume[0] | pLimit->uPhaseUnResume[1]<<8);
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
             printf("电压不平衡度:恢复限值=%d\n",vUnBalanceResume);
           }
      	    
      	    //寻找最大最小值
      	    maxPhase = maxData(maxData(tmpDataA, tmpDataB), tmpDataC);
      	    minPhase = minData(minData(tmpDataA, tmpDataB), tmpDataC);
            
            if (maxPhase == 0 || minPhase == 0)
            {
              return;
            }
      	    
      	    //计算不平衡度
      	    unbalanceU = (maxPhase - minPhase)*1000 / maxPhase;
            
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("电压不平衡度=%d\n",unbalanceU);
            }
      	    
      	    if (unbalanceU >= tmpDataLimit)
      	    {
      	    	//不平衡度越限首次发生
      	    	if ((eventRecordConfig.iEvent[2] & 0x01) || (eventRecordConfig.nEvent[2] & 0x01))
              {
      	    	  if (pStatisRecord->vUnBalance == VOLTAGE_UNBALANCE_NOT)
      	    	  {
      	    	    //事件开始
             	     if (pStatisRecord->vUnBalanceTime.year==0xff)
             	     {
             	   	   pStatisRecord->vUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->uPhaseUnTimes, 0);
             	   	   
             	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
             	   	   {
             	   	     printf("电压不平衡:越限发生,%d分钟后记录\n",pLimit->uPhaseUnTimes);
             	   	   }
             	     }
             	     else
             	     {
	                   if (compareTwoTime(pStatisRecord->vUnBalanceTime,sysTime))
	                   {
	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
	                  	 {
	                  	   printf("电压不平衡:越限发生持续时间已到\n");
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
      	    	
      	    	//记录电压不平衡度越限时间
      	    	tmpData = pBalanceParaBuff[VOL_UNBALANCE_TIME] | pBalanceParaBuff[VOL_UNBALANCE_TIME+1]<<8;
      	    	tmpData += statisInterval;
      	    	pBalanceParaBuff[VOL_UNBALANCE_TIME] = tmpData&0xFF;
      	    	pBalanceParaBuff[VOL_UNBALANCE_TIME+1] = tmpData>>8&0xFF;
      	    	
      	    	if (pBalanceParaBuff[VOL_UNB_MAX]==0xee)
      	    	{
      	    		 //电压不平衡最大值
      	    		 pBalanceParaBuff[VOL_UNB_MAX]   = hexToBcd(unbalanceU);
      	    		 pBalanceParaBuff[VOL_UNB_MAX+1] = hexToBcd(unbalanceU)>>8;

      	    		 //电压不平衡最大值发生时间
      	    		 pBalanceParaBuff[VOL_UNB_MAX_TIME]   = statisTime.minute;
      	    		 pBalanceParaBuff[VOL_UNB_MAX_TIME+1] = statisTime.hour;
      	    		 pBalanceParaBuff[VOL_UNB_MAX_TIME+2] = statisTime.day;
      	    	}
      	    	else
      	    	{
      	    	   tmpData = pBalanceParaBuff[VOL_UNB_MAX] | pBalanceParaBuff[VOL_UNB_MAX+1]<<8;
      	    	   if (tmpData<unbalanceU)
      	    	   {
      	    		   //电压不平衡最大值
      	    		   pBalanceParaBuff[VOL_UNB_MAX]   = hexToBcd(unbalanceU);
      	    		   pBalanceParaBuff[VOL_UNB_MAX+1] = hexToBcd(unbalanceU)>>8;

      	    		   //电压不平衡最大值发生时间
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
      	    	  //不平衡度越限首次恢复
                 if (unbalanceU<=vUnBalanceResume)
                 {
                	//曾经发生越上上限,记录越上上限恢复
      	          if (pStatisRecord->vUnBalance == VOLTAGE_UNBALANCE)
                  {
              	     if (pStatisRecord->vUnBalanceTime.year==0xff)
              	     {
              	   	   pStatisRecord->vUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->uPhaseUnTimes, 0);
            	   	     
            	   	     if (debugInfo&PRINT_BALANCE_DEBUG)
            	   	     {
              	   	     printf("电压不平衡:越限开始恢复,%d分钟后记录\n",pLimit->uPhaseUnTimes);
              	   	   }
              	     }
              	     else
              	     {
 	                   if (compareTwoTime(pStatisRecord->vUnBalanceTime,sysTime))
 	                   {
 	                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
 	                  	 {
 	                  	   printf("电压不平衡:越限恢复持续时间已到\n");
 	                  	 }
 	                  	 
 	                  	 pStatisRecord->vUnBalanceTime.year = 0xff;
              	   
      	      	       //事件恢复
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
      	
        //必须对三相电流一起统计
      	if (pCopyParaBuff[CURRENT_PHASE_A] != 0xEE && pCopyParaBuff[CURRENT_PHASE_B] != 0xEE && pCopyParaBuff[CURRENT_PHASE_C] != 0xEE)
    	  {
      	    tmpDataA = pCopyParaBuff[CURRENT_PHASE_A] | pCopyParaBuff[CURRENT_PHASE_A+1]<<8;
      	    tmpDataB = pCopyParaBuff[CURRENT_PHASE_B] | pCopyParaBuff[CURRENT_PHASE_B+1]<<8;
      	    tmpDataC = pCopyParaBuff[CURRENT_PHASE_C] | pCopyParaBuff[CURRENT_PHASE_C+1]<<8;

      	    tmpDataLimit = (pLimit->cPhaseUnbalance[0] | pLimit->cPhaseUnbalance[1]<<8);
      	    
      	    //数据格式转换bcd => hex
      	    tmpDataA = bcdToHex(tmpDataA);
      	    tmpDataB = bcdToHex(tmpDataB);
      	    tmpDataC = bcdToHex(tmpDataC);
      	    tmpDataLimit = bcdToHex(tmpDataLimit);
      	    
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("电流不平衡度:限值=%d\n",tmpDataLimit);
            }
      	    
      	     cUnBalanceResume = calcResumeLimit(pLimit->cPhaseUnbalance[0] | pLimit->cPhaseUnbalance[1]<<8 , pLimit->cPhaseUnResume[0] | pLimit->cPhaseUnResume[1]<<8);
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("电流不平衡度:恢复限值=%d\n",cUnBalanceResume);
             }
      	    
  	        tmpLineInType = 2;
  	        
			      if(selectViceParameter(0x04, 25, pn, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
			      {
               tmpLineInType = pointPara.linkStyle;
			      }
            
            //电源接线方式不是三相三线或三相四线,不用计算
            if (tmpLineInType>2 || tmpLineInType<1)
            {
            	  return;
            }
      	    
      	    //寻找最大最小值  
            if (tmpLineInType==2)   //三相四线
            {
      	      maxPhase = maxData(maxData(tmpDataA, tmpDataB), tmpDataC);
      	      minPhase = minData(minData(tmpDataA, tmpDataB), tmpDataC);
      	    }
      	    else                    //三相三线,B相无电流
      	    {
      	      maxPhase = maxData(tmpDataA, tmpDataC);
      	      minPhase = minData(tmpDataA, tmpDataC);
      	    }
      	    
            if (maxPhase == 0 || minPhase == 0)
            {
              return;
            }
            
      	    //电流不平衡度
      	    unbalanceC = (maxPhase - minPhase)*1000 / maxPhase;

            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("电流不平衡度=%d\n",unbalanceC);
            }
                        
      	    if (unbalanceC >= tmpDataLimit)
      	    {
      	    	if (pStatisRecord->cUnBalance == CURRENT_UNBALANCE_NOT)
      	    	{
    	    	    //事件开始
           	     if (pStatisRecord->cUnBalanceTime.year==0xff)
           	     {
           	   	   pStatisRecord->cUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->cPhaseUnTimes, 0);
           	   	   
           	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
           	   	   {
           	   	     printf("电流不平衡:越限发生,%d分钟后记录\n",pLimit->cPhaseUnTimes);
           	   	   }
           	     }
           	     else
           	     {
                   if (compareTwoTime(pStatisRecord->cUnBalanceTime,sysTime))
                   {
                  	 if (debugInfo&PRINT_BALANCE_DEBUG)
                  	 {
                  	   printf("电流不平衡:越限发生持续时间已到\n");
                  	 }
                  	 
                  	 pStatisRecord->cUnBalanceTime.year = 0xff;
           	   
      	    	       eventFlag2 = 0x03;
      	    	       pStatisRecord->cUnBalance = CURRENT_UNBALANCE;
           	       }
           	     }
      	    	}
      	    	
      	    	//记录电流不平衡度越限时间
      	    	tmpData = pBalanceParaBuff[CUR_UNBALANCE_TIME] | pBalanceParaBuff[CUR_UNBALANCE_TIME+1]<<8;
      	    	tmpData += statisInterval;
      	    	pBalanceParaBuff[CUR_UNBALANCE_TIME] = tmpData&0xFF;
      	    	pBalanceParaBuff[CUR_UNBALANCE_TIME+1] = tmpData>>8&0xFF;
      	    	
      	    	if (pBalanceParaBuff[CUR_UNB_MAX]==0xee)
      	    	{
      	    		 //电流不平衡最大值
      	    		 pBalanceParaBuff[CUR_UNB_MAX]   = hexToBcd(unbalanceC);
      	    		 pBalanceParaBuff[CUR_UNB_MAX+1] = hexToBcd(unbalanceC)>>8;

      	    		 //电流不平衡最大值发生时间
      	    		 pBalanceParaBuff[CUR_UNB_MAX_TIME]   = statisTime.minute;
      	    		 pBalanceParaBuff[CUR_UNB_MAX_TIME+1] = statisTime.hour;
      	    		 pBalanceParaBuff[CUR_UNB_MAX_TIME+2] = statisTime.day;
      	    	}
      	    	else
      	    	{
      	    	   tmpData = pBalanceParaBuff[CUR_UNB_MAX] | pBalanceParaBuff[CUR_UNB_MAX+1]<<8;
      	    	   if (tmpData<unbalanceU)
      	    	   {
      	    		   //电流不平衡最大值
      	    		   pBalanceParaBuff[CUR_UNB_MAX]   = hexToBcd(unbalanceC);
      	    		   pBalanceParaBuff[CUR_UNB_MAX+1] = hexToBcd(unbalanceC)>>8;

      	    		   //电流不平衡最大值发生时间
      	    		   pBalanceParaBuff[CUR_UNB_MAX_TIME]   = statisTime.minute;
      	    		   pBalanceParaBuff[CUR_UNB_MAX_TIME+1] = statisTime.hour;
      	    		   pBalanceParaBuff[CUR_UNB_MAX_TIME+2] = statisTime.day;
      	    	   }
      	    	}
      	    }
      	    else
      	    {
    	    	  //不平衡度越限首次恢复
               if (unbalanceC<=cUnBalanceResume)
               {
              	//曾经发生越上上限,记录越上上限恢复
      	        if (pStatisRecord->cUnBalance == CURRENT_UNBALANCE)
                {
            	     if (pStatisRecord->cUnBalanceTime.year==0xff)
            	     {
            	   	   pStatisRecord->cUnBalanceTime = nextTime(timeBcdToHex(statisTime), pLimit->cPhaseUnTimes, 0);
            	   	   
            	   	   if (debugInfo&PRINT_BALANCE_DEBUG)
            	   	   {
            	   	     printf("电流不平衡:越限开始恢复,%d分钟后记录\n",pLimit->cPhaseUnTimes);
            	   	   }
            	     }
            	     else
            	     {
                     if (compareTwoTime(pStatisRecord->cUnBalanceTime,sysTime))
                     {
                  	   if (debugInfo&PRINT_BALANCE_DEBUG)
                  	   {
                  	     printf("电流不平衡:越限恢复持续时间已到\n");
                  	   }
                  	 
                  	   pStatisRecord->cUnBalanceTime.year = 0xff;
            	   
    	      	         //事件恢复
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
      	
      	//电压不平衡度越限事件
        if (eventFlag1 != 0x00)
        {
            eventData[0] = 17;  //erc
                
            eventData[2] = statisTime.minute;
            eventData[3] = statisTime.hour;
            eventData[4] = statisTime.day;
            eventData[5] = statisTime.month;
            eventData[6] = statisTime.year;
            
            dataTail = 7;    
            
            eventData[dataTail++] = pn&0xff;   //测量点低8位    
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
               writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
            }
            
            if (eventRecordConfig.nEvent[2] & 0x01)
            {
               writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入重要事件队列
            }
        }
        
        //电流不平衡度越限事件
        if (eventFlag2 != 0x00)
        {
            eventData[0] = 17;  //erc
                
            eventData[2] = statisTime.minute;
            eventData[3] = statisTime.hour;
            eventData[4] = statisTime.day;
            eventData[5] = statisTime.month;
            eventData[6] = statisTime.year;
            
            dataTail = 7;    
            
            eventData[dataTail++] = pn&0xff;   //测量点低8位    
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
               writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
            }
            
            if (eventRecordConfig.nEvent[2] & 0x01)
            {
               writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入重要事件队列
            }
        }
    }
}

/*******************************************************
函数名称: statisticOpenPhase
功能描述: 断相统计 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:
*******************************************************/
void statisticOpenPhase(INT8U *pCopyParaBuff,INT8U *pBalanceParaBuff)
{
		INT8U      i;
		INT16U     offset, offsetSta;
	  
		offsetSta = OPEN_PHASE_TIMES;
		offset = PHASE_DOWN_TIMES;

    /*
    //读出昨天最后一个数据作为统计参考
    if (readMeterData(readBuff, meterDeviceConfig.meterDevice[balancePn].measurePoint, POWER_PARA_DATA, LAST_TODAY, tmpTime) == TRUE)
    {
      //如果昨天没有数据,则读出今天和一个数据作为统计参考
      if (readMeterData(reqReqTimeData, pn, POWER_PARA_DATA, FIRST_TODAY, lastCopyTime) == FALSE)
      {
      	 
      }    	 
    }
    */
             		  
		//总及A、B、C相断相统计数据
		for (i = 0; i < 4; i++)
		{
  	    //断相次数
  	    if (pCopyParaBuff[offset] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta] = pCopyParaBuff[PHASE_DOWN_TIMES+i*6];
  	      pBalanceParaBuff[offsetSta+1] = pCopyParaBuff[PHASE_DOWN_TIMES+i*6+1];
  	    }
  	    
  	    //断相总时间
  	    if (pCopyParaBuff[offset+8] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta+2] = pCopyParaBuff[TOTAL_PHASE_DOWN_TIME+i*6];
  	      pBalanceParaBuff[offsetSta+3] = pCopyParaBuff[TOTAL_PHASE_DOWN_TIME+i*6+1];
  	      pBalanceParaBuff[offsetSta+4] = pCopyParaBuff[TOTAL_PHASE_DOWN_TIME+i*6+2];
  	    }
  	    
  	    //最近一次断相起始时刻
  	    if (pCopyParaBuff[offset+20] != 0xEE)
  	    {
  	      pBalanceParaBuff[offsetSta+5] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+1];
  	      pBalanceParaBuff[offsetSta+6] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+2];
  	      pBalanceParaBuff[offsetSta+7] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+3];
  	      pBalanceParaBuff[offsetSta+8] = pCopyParaBuff[LAST_PHASE_DOWN_BEGIN+i*12+4];
  	    }
  	    
  	    //最近一次断相结束时刻
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
函数名称: energyCompute
功能描述: 电能量结算(日,月电能量)
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:
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
    
    if (type==1 || type==3) //结算日电量
    {
       //读取前一日最后一点数据作为结算参考
       readTime = tmpTime;
       if (readMeterData(tmpBuff, pn, DAY_BALANCE, DAY_FREEZE_COPY_DATA, &readTime,0) == FALSE)
       {
    	   //如果得不到前一日最后一点数据，读取当天第一点数据作为结算参考
    	   readTime = statisTime;
         if (readMeterData(tmpBuff, pn, FIRST_TODAY, ENERGY_DATA, &readTime, 0) == FALSE)
         {
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
           	 printf("energyCompute:未读到当日第一点数据\n");
           }

           return FALSE;
         }
         else
         {
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
           	 printf("energyCompute:读到当日第一点数据,数据时间=%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
           }
         }
       }
       else
       {
         if (debugInfo&PRINT_BALANCE_DEBUG)
         {
           printf("energyCompute:读到上一日日冻结数据,数据时间=%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
         }
       }
       
       //结算日当前电能量，正/反向有/无功，共4项，每项8费率，每费率7字节(含1字节符号)
       offsetCopy = POSITIVE_WORK_OFFSET;
       offsetBalance = DAY_P_WORK_OFFSET;
		}
		else         //结算月电量
		{
       //读取上月数据作为结算参考
       //readTime = statisTime;
       //ly,2011-04-20,计算当月电量用当前值减去当月第一点数据,
       //if (readMeterData(tmpBuff, pn, LAST_MONTH_DATA, POWER_PARA_LASTMONTH, &readTime, 0) == FALSE)
       //{
    	   //如果得不到上月数据,读取当月第一点数据作为结算参考
    	   readTime = statisTime;
         if (readMeterData(tmpBuff, pn, FIRST_MONTH, ENERGY_DATA, &readTime, 0) == FALSE)
         {
           if (debugInfo&PRINT_BALANCE_DEBUG)
           {
           	 printf("energyCompute:未读到当月第一点数据\n");
           }
           return FALSE;
         }
    	 //}
    	 
    	 //结算月当前电能量，正/反向有/无功，共4项，每项8费率，每费率7字节(含1字节符号)
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
      //按费率循环结算
      for (tariff = 0; tariff <= 8; tariff++)
  	  {
  	  	 //只要其中一个计算用原始数据不存在，计算结果标记为无该项数据
  	     if (pCopyEnergyBuff[offsetCopy] == 0xEE || tmpBuff[offsetCopy] == 0xEE)
  	     {
  	       pBalanceEnergyBuff[offsetBalance]   = 0xEE;   //符号
  	     
  	       pBalanceEnergyBuff[offsetBalance+1] = 0xEE;   //小数
  	       pBalanceEnergyBuff[offsetBalance+2] = 0xEE;   //小数
  	     
  	       pBalanceEnergyBuff[offsetBalance+3] = 0xEE;   //整数
  	       pBalanceEnergyBuff[offsetBalance+4] = 0xEE;   //整数
  	       pBalanceEnergyBuff[offsetBalance+5] = 0xEE;   //整数
  	       pBalanceEnergyBuff[offsetBalance+6] = 0xEE;   //整数
  	     }
  	     else
  	     {
  	       rawInt = rawDec = sumInt = sumDec = 0;
  	     
  	       //将原始数据转换成二进制格式
  	       if (type==1 || type==2) //485端口测量点电能示值是4个字节
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
    	     else          //脉冲测量点电能示值是5个字节
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
    	     	 printf("energyCompute:测量点%d,数据类型%d费率%d,当前电能示值=%d.%04d,参考值=%d.%04d\n", pn, dataType, tariff, sumInt, sumDec, rawInt, rawDec);
    	     }
    	     
    	     //默认计算结果为正
    	     tmpSign = POSITIVE_NUM;
    	    
    	     //大正数减小正数，结果为正
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
           else  //小正数减大正数，结果为负
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

    	     //乘以互感倍率得电能量
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
    	     
    	     //调整小数进位
    	     if (sumDec > 9999)
    	     {
    	       do
    	       {
    	          sumDec -= 10000;
    	          sumInt++;
    	       }while (sumDec > 9999);
    	     }
    	     
    	     //填写到结构中
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
    
    //结算用原始数据都存储，结算过后应该存储结算结果
    return TRUE;
}

/*******************************************************
函数名称:groupBalance
功能描述:总加组结算  
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:
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

    //计算用数据清零，默认计算结果为正
	  for (tariff = 0; tariff < 9; tariff++)
    {
      dataDec[tariff]  = monthDec[tariff]  = 0x00;
      dataInt[tariff]  = monthInt[tariff]  = 0x00;
	    dataSign[tariff] = monthSign[tariff] = POSITIVE_NUM;
	  }
    
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("groupBalance:总加组=%d,测量点个数%d\n",totalAddGroup.perZjz[gp].zjzNo ,ptNum);
    }
    
    //将指定总加组的每个测量点进行总加运算
   	pnHasNoData = TRUE;
    for(i = 0; i < ptNum; i++)
    {
  	  //确定测量点号，方向，符号
      tmpPn = (totalAddGroup.perZjz[gp].measurePoint[i] & 0x3F) + 1;
      direction = totalAddGroup.perZjz[gp].measurePoint[i] & 0x40;
      sign = totalAddGroup.perZjz[gp].measurePoint[i] & 0x80;
      
			readTime = balanceTime;
   		
   		bufHasData = FALSE;
   		#ifdef PULSE_GATHER
			 	//查看是否是脉冲采样的数据
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //是脉冲量的测量点
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==tmpPn)
			    {
			      pulsePnData = 1;
 	          
            //if (tmpBalanceType&0x80)
            //{
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("groupBalance:实时计算脉冲测量点的日月电量\n");
              }
              
              memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);
              memset(realBalanceEnergy, 0xee, LEN_OF_ENERGY_BALANCE_RECORD);
  
              //转换
              covertPulseData(j, visionBuff, NULL, NULL);
              
              //结算当日电能量
              bufHasData = energyCompute(tmpPn, visionBuff, realBalanceEnergy, readTime, 3);
  
              //结算当月电能量
              bufHasData = energyCompute(tmpPn, visionBuff, realBalanceEnergy, readTime, 4);
            
            /*ly,2011-04-25,晚上注释
            }
            else
            {
              //读取实时统计电能量数据，逐个费率进行总加
              readTime = balanceTime;

              bufHasData = readMeterData(realBalanceEnergy, tmpPn, REAL_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, 0);
              
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                if (bufHasData)
                {
                  printf("读取脉冲测量点的总加电量成功\n");
                }
                else
                {
                  printf("读取脉冲测量点的总加电量失败\n");
                }
              }
            }
            */
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //不是脉冲测量点数据
		  {
        //检查测量点合法性
        if (selectF10Data(tmpPn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==FALSE)
        {
      	   //库中没有该测量点的数据,无法进行总加电能量的计算
      	   return FALSE;
        }
        else
        {
  	      //测量点配置中存在该点信息，且该测量点不是现场监测设备和交流采样装置，可以对该点进行总加运算
  	      //if (meterConfig.protocol == AC_SAMPLE 
  	      //	  || meterConfig.protocol == SUPERVISAL_DEVICE)
  	      if (meterConfig.protocol == SUPERVISAL_DEVICE)
  	      {
  	      	 return FALSE;
  	      }
        }
        
        //读取实时统计电能量数据，逐个费率进行总加
        readTime = queryCopyTime(tmpPn);
        
        bufHasData = readMeterData(realBalanceEnergy, tmpPn, REAL_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, 0);
      }
      	
      if (bufHasData == TRUE)
      {
        if (balanceType == GP_DAY_WORK)  //总加有功数据
        {
          if (direction == 0)  //测量点方向标记为正,使用正向有功电能量参与总加
          {
        	  offset = DAY_P_WORK_OFFSET;
        	  offsetMonth = MONTH_P_WORK_OFFSET;
          }
          else                 //测量点方向标记为负,使用反向有功电能量参与总加
          {
            offset = DAY_N_WORK_OFFSET;
            offsetMonth = MONTH_N_WORK_OFFSET;
          }
    	  }
    	  else                             //总加无功数据 
    	  {
    	    if (direction == 0)  //测量点方向标记为正,使用正向无功电能量参与总加
          {
        	  offset = DAY_P_NO_WORK_OFFSET;
        	  offsetMonth = MONTH_P_NO_WORK_OFFSET;
          }
          else                 //测量点方向标记为负,使用反向无功电能量参与总加
          {
            offset = DAY_N_NO_WORK_OFFSET;
            offsetMonth = MONTH_N_NO_WORK_OFFSET;
          }
    	  }
    	  
    	  //按费率进行总加
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
            	printf("groupBalance:测量点%d结算当日费率%d电能量=%d.%d\n", tmpPn, tariff, dataRawInt, dataRawDec);
            }
            
            if (sign == 0)    // +
            {
              if (dataSign[tariff] == POSITIVE_NUM)       //正数相加
              {
                dataInt[tariff] += dataRawInt;
                dataDec[tariff] += dataRawDec;
              }
              else      //负数相减
              {
                //先减小数，同时处理借位
                if (dataDec[tariff] >= dataRawDec)
                {
                   dataDec[tariff] -= dataRawDec;
                }
                else
                {
                  dataDec[tariff] = 10000 + dataDec[tariff] - dataRawDec;
                  dataInt[tariff]++;
                }
                //再减整数
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
              	//大正数减小正数，结果为被减数减减数，符号为正
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
                else   //小正数减大正数，结果为减数减被减数，符号为负
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
            	printf("groupBalance:测量点%d结算当月费率%d电能量=%d.%d\n", tmpPn, tariff, dataRawInt, dataRawDec);
            }

            if (sign == 0)    // +
            {
              if (monthSign[tariff] == POSITIVE_NUM)       //正数相加
              {
                monthInt[tariff] += dataRawInt;
                monthDec[tariff] += dataRawDec;
              }
              else      //负数相减
              {
                //先减小数，同时处理借位
                if (monthDec[tariff] >= dataRawDec)
                {
                   monthDec[tariff] -= dataRawDec;
                }
                else
                {
                  monthDec[tariff] = 10000 + monthDec[tariff] - dataRawDec;
                  monthInt[tariff]++;
                }
                //再减整数
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
              	//大正数减小正数，结果为被减数减减数，符号为正
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
                else   //小正数减大正数，结果为减数减被减数，符号为负
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
  	    //调整进位
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
  	    //调整进位
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
函数名称: groupStatistic
功能描述: 总加组统计 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:
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
      printf("groupStatistic:测量点个数%d\n",ptNum);
    }
    
    //<- - - - - 总加功率统计- - - - - ->
	  for(i = 0; i < ptNum; i++)
    { 
  	  //确定测量点号，方向，符号
      tmpPn = (totalAddGroup.perZjz[gp].measurePoint[i] & 0x3F) + 1;
      direction = totalAddGroup.perZjz[gp].measurePoint[i] & 0x40;
      sign = totalAddGroup.perZjz[gp].measurePoint[i] & 0x80;
      buffHasData = FALSE;
      pulsePnData = 0;
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("总加组序号=%d,测量点%d\n",gp,tmpPn);
      }
      
   		#ifdef PULSE_GATHER
			 	//查看是否是脉冲采样的数据
			  for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
			  {
			    //是脉冲量的测量点
			    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==tmpPn)
			    {
			      //P.1先初始化缓存
            memset(tmpReadBuff,0xee,LENGTH_OF_PARA_RECORD);

				 	  //P.2将脉冲量的功率填入dataBuff对应的位置中
			   	  covertPulseData(j, NULL,NULL,tmpReadBuff);
			   	  	 	    
			      pulsePnData = 1;
			      
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
              printf("groupStatistic:转换测量点%d脉冲量\n",tmpPn);
            }
			   	  
			      buffHasData = TRUE;
			      break;
				  }
				}
		  #endif
		    
		  if (pulsePnData==0)   //不是脉冲测量点数据
		  {
        if (selectF10Data(tmpPn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
        {
		      if (meterConfig.protocol==AC_SAMPLE)
		      {
		     	  if (ifHasAcModule==TRUE)
		     	  {
			        //A.1先初始化缓存
              memset(tmpReadBuff,0xee,LENGTH_OF_PARA_RECORD);

			       	//A.2将交流采样数据填入dataBuff中
			       	covertAcSample(tmpReadBuff, NULL, NULL, 1, sysTime);
              
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("groupStatistic:转换测量点%d交采值\n",tmpPn);
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
        if (balanceType == GP_DAY_WORK)   //总加有功功率
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
          	  //测量点数据不完整，统计数据标记为无该项数据，退出循环
              powerInt = 0xEE;
              break;              
            }
          }
    	  }
    	  else                 //总加无功功率
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
              //测量点数据不完整，统计数据标记为无该项数据，退出循环
              powerInt = 0xEE;
              break;
            }
          }
        }
      	//4-5-3.读取测量点对应的限制参数
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

	      if (sign == 0)   //总加运算+
        {
          if (powerSign == POSITIVE_NUM)  //正数+正数
          {
            powerInt += dataRawInt;
            powerDec += dataRawDec;
          }
          else    //正数-正数
          {
        	  //被减数大于减数，结果为正
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
            else  //被减数小于减数，结果为负
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
 	      else  //总加运算-
 	      {
 	        if (powerSign == NEGTIVE_NUM)   //负数-正数
 	        {
 	          powerInt = powerInt + dataRawInt;
 	          powerDec = powerDec + dataRawDec;
 	        }
 	        else  //正数-正数
 	        {
 	          //被减数大于减数, 结果为正数
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
 	          else  //被减数小于减数，结果为负数
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
    
    //<- - - - - 调整数据格式， 保存参变量数据- - - - - >
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
    
    //<- - - - - 比较当日功率最大值和当月功率最大值- - - - - >
    //读取前一次实时总加数据作为统计参考
    if (balanceType == GP_DAY_WORK)
    {
      //if (readMeterData(tmpReadBuff, totalAddGroup.perZjz[gp].zjzNo, GROUP_REAL_BALANCE, LAST_TODAY, statisTime) == TRUE)
      readTime = statisTime;
      if (readMeterData(tmpReadBuff, totalAddGroup.perZjz[gp].zjzNo, REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0) == TRUE)
      {
      	//上次统计当天的日统计尚未结束，在此基础上继续统计
      	if (tmpReadBuff[GP_DAY_OVER] != 0x01)
      	{
          //本次结算有总加有功功率结果，将其与上一次统计结果进行比较
          if (pBalanceZjzData[GP_WORK_POWER] != 0xEE)
          {
          	//日 最 大 总 加 有 功 功 率
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
        	  else   //上次统计没有得出日最大总加有功功率，则以本次总加的有功功率作为统计基础
        	  {
        	    pBalanceZjzData[GP_DAY_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	    pBalanceZjzData[GP_DAY_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_DAY_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	    
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=statisTime.minute;
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=statisTime.hour;
        	    pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=statisTime.day;
        	  }
        	  
        	  //日 最 小 总 加 有 功 功 率
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
        	  else   //上次统计没有得出日最小总加有功功率，则以本次总加的有功功率作为统计基础
        	  {
        	    pBalanceZjzData[GP_DAY_MIN_POWER]   = pBalanceZjzData[GP_WORK_POWER];
        	    pBalanceZjzData[GP_DAY_MIN_POWER+1] = pBalanceZjzData[GP_WORK_POWER+1];
        	    pBalanceZjzData[GP_DAY_MIN_POWER+2] = pBalanceZjzData[GP_WORK_POWER+2];
        	    
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME]   = statisTime.minute;
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1] = statisTime.hour;
        	    pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2] = statisTime.day;
        	  }
        	  
        	  //日功率为零时间
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
        	  else  //上次统计没有得出日功率为零时间，则以本次总加的有功功率作为统计基础
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
          else //本次结算没有总加有功功率结果，统计值为上一次统计结果，若上次统计也没有结果，统计值为零
          {
          	//日 功 率 最 大 值
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
        	  
        	  //日 功 率 最 小 值
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
            
            //日 功 率 为 零 时 间
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
        else  //前一日统计结束，重新开始当日统计
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
        
        //前一次统计未标明月统计结束，在前一次基础上继续统计
        if (tmpReadBuff[GP_MONTH_OVER] != 0x01)
        {
        	//本次结算有总加有功功率结果，将其与上一次统计结果进行比较
          if (pBalanceZjzData[GP_WORK_POWER] != 0xEE)
          {
            //月最大有功功率
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
        	  
        	  //月最小有功功率
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
        	  
        	  //月有功功率为零时间
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
            //月最大有功功率
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
        	  
        	  //月最小有功功率 
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
            
            //月有功功率为零时间
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
        else   //前一次月统计结束，开始新的月统计数据
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
      else  //不能取得前一次的统计数据，重新开始统计
      {
        if (pBalanceZjzData[GP_WORK_POWER+1] != 0xEE)
        {
          //日最大
          pBalanceZjzData[GP_DAY_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_DAY_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_DAY_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=statisTime.day;
        	//日最小
        	pBalanceZjzData[GP_DAY_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_DAY_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_DAY_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=statisTime.day;
          //月最大
        	pBalanceZjzData[GP_MONTH_MAX_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_MONTH_MAX_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_MONTH_MAX_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=statisTime.day;
        	//月最小
        	pBalanceZjzData[GP_MONTH_MIN_POWER]=pBalanceZjzData[GP_WORK_POWER];
        	pBalanceZjzData[GP_MONTH_MIN_POWER+1]=pBalanceZjzData[GP_WORK_POWER+1];
        	pBalanceZjzData[GP_MONTH_MIN_POWER+2]=pBalanceZjzData[GP_WORK_POWER+2];
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=statisTime.minute;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=statisTime.hour;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=statisTime.day;
        	//日、月功率为零
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
          //日最大
          pBalanceZjzData[GP_DAY_MAX_POWER]=0x00;
        	pBalanceZjzData[GP_DAY_MAX_POWER+1]=0x00;
        	pBalanceZjzData[GP_DAY_MAX_POWER+2]=0x00;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_DAY_MAX_POWER_TIME+2]=0xEE;
        	//日最小
        	pBalanceZjzData[GP_DAY_MIN_POWER]=0x00;
        	pBalanceZjzData[GP_DAY_MIN_POWER+1]=0x00;
        	pBalanceZjzData[GP_DAY_MIN_POWER+2]=0x00;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_DAY_MIN_POWER_TIME+2]=0xEE;
          //月最大
        	pBalanceZjzData[GP_MONTH_MAX_POWER]=0x00;
        	pBalanceZjzData[GP_MONTH_MAX_POWER+1]=0x00;
        	pBalanceZjzData[GP_MONTH_MAX_POWER+2]=0x00;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_MONTH_MAX_POWER_TIME+2]=0xEE;
        	//月最小
        	pBalanceZjzData[GP_MONTH_MIN_POWER]=0x00;
        	pBalanceZjzData[GP_MONTH_MIN_POWER+1]=0x00;
        	pBalanceZjzData[GP_MONTH_MIN_POWER+2]=0x00;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME]=0xEE;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+1]=0xEE;
        	pBalanceZjzData[GP_MONTH_MIN_POWER_TIME+2]=0xEE;
        	//日、功率为零
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
函数名称: eventRecord
功能描述: 电能量事件分析处理，包括：电能表飞走、停走、示度下降
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值： 
*******************************************************/
void eventRecord(INT16U pn, INT8U *pCopyEnergyBuff, INT8U *pCopyParaBuff, METER_STATIS_EXTRAN_TIME *pStatisRecord, INT8U statisInterval, DATE_TIME statisTime)
{
  INT8U      stopEvent[25];       //停走事件
  INT8U      flyEvent[24];        //飞走事件
  INT8U      reverseEvent[22];    //下降事件
  INT8U      overEvent[22];       //超差事件,2013-11-21,add
  INT8U      meterEvent[10];      //电能表故障信息
  INT32U     tmpPresHex, tmpLastHex, tmpGateHex, tmpMeterData, tmpCountData;
  INT32U     tmpOverGateHex;      //超差阈值,2013-11-21,add
  DATE_TIME  tmpCopyTime, tmpStopTime;
  INT8U      lastLastCopyEnergy[LENGTH_OF_ENERGY_RECORD];
  DATE_TIME  readTime;
  INT8U      stopDataTail,reverseDataTail,flyDataTail,overDataTail=0;
  INT16U     i;

  stopEvent[0] = flyEvent[0] = reverseEvent[0] = meterEvent[0] = overEvent[0] = 0;
  
  if (debugInfo&PRINT_BALANCE_DEBUG)
  {
    printf("判定测量点%d电能量事件\n",pn);
  }
  
  if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)
  {
    if (debugInfo&PRINT_BALANCE_DEBUG)
    {
      printf("测量点%d本次抄表无电能量数据\n",pn);
    }
    
    return;    	 
  }
  
  //1.电能表停走事件判别
  if (debugInfo&PRINT_BALANCE_DEBUG)
  {
    printf("开始电能表停走判断,停走阈值=%d分\n",meterGate.meterStopGate*15);
  }
  
  switch(pStatisRecord->meterStop[0])
  {
    case 0xee:                      //若没有停走现场数据，记录新的停走现场数据
    case 0x0:
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("电能表停走:测量点%d无停走现场数据,记录停走现场数据\n",pn);
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

    case METER_STOP_NOT_RECORDED:  //若停走尚未记录则分析是否应该记录停走
 	     if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET] == pStatisRecord->meterStop[1]
 	    	 && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1] == pStatisRecord->meterStop[2]
 	    	   && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] == pStatisRecord->meterStop[3]
 	    	     && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == pStatisRecord->meterStop[4])
 	     {
    	   //取出存储的停走起始时间，与上次抄表时间比较
         tmpCopyTime = timeBcdToHex(statisTime);
         tmpCopyTime.second = 0;
       	
         tmpStopTime.second = 0;
       	 tmpStopTime.minute = ((pStatisRecord->meterStop[5]>>4)&0xF)*10 + (pStatisRecord->meterStop[5]&0x0F);
       	 tmpStopTime.hour   = ((pStatisRecord->meterStop[6]>>4)&0xF)*10 + (pStatisRecord->meterStop[6]&0x0F);
       	 tmpStopTime.day    = ((pStatisRecord->meterStop[7]>>4)&0xF)*10 + (pStatisRecord->meterStop[7]&0x0F);
       	 tmpStopTime.month  = ((pStatisRecord->meterStop[8]>>4)&0xF)*10 + (pStatisRecord->meterStop[8]&0x0F);
       	 tmpStopTime.year   = ((pStatisRecord->meterStop[9]>>4)&0xF)*10 + (pStatisRecord->meterStop[9]&0x0F);
 	    	
 	    	 //停走到达阈值时间
 	       if (timeCompare(tmpStopTime, tmpCopyTime, meterGate.meterStopGate*15) == FALSE)
 	       {
    	      if (debugInfo&PRINT_BALANCE_DEBUG)
    	      {
    	        printf("电能表停走:到达阈值,记录发生事件\n");
    	      }
    	      
    	      //停走已记录
            pStatisRecord->meterStop[0] = METER_STOP_RECORDED;
             
            //记录发生事件
         	  stopEvent[0] = 0x1E;   //ERC30
           
            stopEvent[2] = 0;      //填充字节
            stopEvent[3] = pStatisRecord->meterStop[5]; //停走时间 分
            stopEvent[4] = pStatisRecord->meterStop[6]; //停走时间 时
            stopEvent[5] = pStatisRecord->meterStop[7]; //停走时间 日
            stopEvent[6] = pStatisRecord->meterStop[8]; //停走时间 月
  	        stopEvent[7] = pStatisRecord->meterStop[9]; //停走时间 年
  	        
  	        stopDataTail = 8;
  	        stopEvent[stopDataTail++] = pn&0xff;
  	        stopEvent[stopDataTail++] = (pn>>8&0xf) | 0x80; //停走发生
  	       
  	        //停走示值
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
    	      
    	      stopEvent[stopDataTail++] = meterGate.meterStopGate;    //阈值
            
            stopEvent[1] = stopDataTail;   //存储长度
         }
         else //停走尚未到达阈值时间
         {
    	     if (debugInfo&PRINT_BALANCE_DEBUG)
    	     {
    	       printf("电能表停走:测量点%d发生但尚未到达阈值\n",pn);
    	     }

    	     pStatisRecord->meterStop[0] = METER_STOP_NOT_RECORDED;
         }
       }
       else  //未发生停走
       {
    	   if (debugInfo&PRINT_BALANCE_DEBUG)
    	   {
    	     printf("电能表停走:测量点%d未发生停走\n",pn);
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
  
    case METER_STOP_RECORDED:       //若停走已经记录，但示值不同，记录新的停走现场
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("电能表停走:测量点%d停走已记录\n",pn);
    	}
    	
    	if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]!=pStatisRecord->meterStop[1]
	    	|| pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1]!=pStatisRecord->meterStop[2]
	    	  || pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2]!=pStatisRecord->meterStop[3]
	    	    || pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3]!=pStatisRecord->meterStop[4])
	    {
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
    	    printf("电能表停走:测量点%d停走恢复,记录恢复事件\n",pn);
    	  }
    	  
        //记录恢复事件
        stopEvent[0] = 0x1E;   //ERC30
          
        stopEvent[2] = 0;      //填充字节
        stopEvent[3] = pStatisRecord->meterStop[5]; //停走时间 分
        stopEvent[4] = pStatisRecord->meterStop[6]; //停走时间 时
        stopEvent[5] = pStatisRecord->meterStop[7]; //停走时间 日
        stopEvent[6] = pStatisRecord->meterStop[8]; //停走时间 月
 	      stopEvent[7] = pStatisRecord->meterStop[9]; //停走时间 年
 	        
 	      stopDataTail = 8;
 	      stopEvent[stopDataTail++] = pn&0xff;
 	      stopEvent[stopDataTail++] = (pn>>8&0xf);    //停走恢复
 	       
 	      //停走示值
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
   	      
   	    stopEvent[stopDataTail++] = meterGate.meterStopGate;    //阈值
        
        stopEvent[1] = stopDataTail;   //存储长度
    	            	         	  
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
  
  //2.电能表示度下降及电能表飞走事件判别
  readTime = statisTime;
  if (readMeterData(lastLastCopyEnergy, pn, LAST_LAST_REAL_DATA, ENERGY_DATA, &readTime, statisInterval) == TRUE)
  {
  	if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET] != 0xEE)
  	{
      //电能表示度下降
  	  if ((pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3])
  		  || (pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2])
  		   || (pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1])
  		    || (pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1] == lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1] && pCopyEnergyBuff[POSITIVE_WORK_OFFSET] < lastLastCopyEnergy[POSITIVE_WORK_OFFSET]))
      {
        //记录事件
        if ((eventRecordConfig.iEvent[3] & 0x04) || (eventRecordConfig.nEvent[3] & 0x04))
   	    {
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("电能示度下降:测量点%d已发生\n",pn);
   	      }

   	      reverseEvent[0] = 0x1B;    //ERC27
     	    
     	    reverseEvent[2] = 0;   //填充字节
          reverseEvent[3] = statisTime.minute;
          reverseEvent[4] = statisTime.hour;
          reverseEvent[5] = statisTime.day;
          reverseEvent[6] = statisTime.month;
     	    reverseEvent[7] = statisTime.year;
          
          reverseDataTail = 8;

  	      reverseEvent[reverseDataTail++] = pn&0xff;
  	      reverseEvent[reverseDataTail++] = (pn>>8&0xf) | 0x80; //示度下降发生
   	     
   	      //ly,2011-06-25,记录下降前示值 
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

   	      reverseEvent[1] = reverseDataTail;    //存储长度
   	    }
   	    pStatisRecord->reverseFlag = 0x1;     //下降已发生
      }
      else
      {
      	//如果下降已发生,记录恢复事件
      	if (pStatisRecord->reverseFlag == 0x1)
      	{
   	      pStatisRecord->reverseFlag = 0x0;     //下降已恢复

   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("电能示度下降:测量点%d已恢复\n",pn);
   	      }
   	      
   	      reverseEvent[0] = 0x1B;    //ERC27
     	    
     	    reverseEvent[2] = 0;   //填充字节
          reverseEvent[3] = statisTime.minute;
          reverseEvent[4] = statisTime.hour;
          reverseEvent[5] = statisTime.day;
          reverseEvent[6] = statisTime.month;
     	    reverseEvent[7] = statisTime.year;
          
          reverseDataTail = 8;

  	      reverseEvent[reverseDataTail++] = pn&0xff;
  	      reverseEvent[reverseDataTail++] = (pn>>8&0xf);    //示度下降恢复

     	    //ly,恢复时,下降前示值=发生时的下降前示值
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
   	      reverseEvent[1] = reverseDataTail;    //存储长度
      	}
      }
      
      //3.飞走
      //电能表飞走(两次抄电表示值的差/根据功率算出来的实际示值差值>=飞走阈值就是飞走)
     	tmpPresHex = bcdToHex(pCopyEnergyBuff[POSITIVE_WORK_OFFSET]|(pCopyEnergyBuff[POSITIVE_WORK_OFFSET+1]<<8)|(pCopyEnergyBuff[POSITIVE_WORK_OFFSET+2]<<16)|(pCopyEnergyBuff[POSITIVE_WORK_OFFSET+3]<<24));
     	//tmpLastHex = bcdToHex(powerParaData[POSITIVE_WORK_OFFSET]|(powerParaData[POSITIVE_WORK_OFFSET+1]<<8)|(powerParaData[POSITIVE_WORK_OFFSET+2]<<16)|(powerParaData[POSITIVE_WORK_OFFSET+3]<<24));
     	tmpLastHex = bcdToHex(lastLastCopyEnergy[POSITIVE_WORK_OFFSET]|(lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1]<<8)|(lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2]<<16)|(lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3]<<24));
     	
     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("开始判断飞走\n");
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
     	  printf("判断飞走:本次抄表电表示值=%d,上次抄表电能表示值=%d\n",tmpPresHex,tmpLastHex);
     	  printf("判断飞走:两次抄表差值=%d,功率=%d\n",tmpMeterData,tmpCountData);       	 
     	}
     	
     	if (tmpCountData == 0)
     	{
     	  goto recordEvent;
     	}
     	
     	tmpGateHex = meterGate.meterFlyGate;
     	tmpGateHex = bcdToHex(tmpGateHex);
     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("判断飞走:飞走阈值=%d\n",tmpGateHex);
     	}

     	//每分钟阈乘以分钟数得到实际飞走阈值
     	if (tmpMeterData*1000/tmpCountData >= tmpGateHex)
     	{
      	if (0x0==pStatisRecord->flyFlag)    //2013-11-22,添加这个判断
      	{
     	    pStatisRecord->flyFlag = 0x1;     //飞走已发生
     	    
     	    if (debugInfo&PRINT_BALANCE_DEBUG)
     	    {
     	      printf("电能表飞走:测量点%d发生\n",pn);
     	    }
  
       	  if ((eventRecordConfig.iEvent[3] & 0x10) || (eventRecordConfig.nEvent[3] & 0x10))
       	  {
     	      flyEvent[0] = 0x1D;    //ERC29
       	    
       	    flyEvent[2] = 0;       //填充字节
            flyEvent[3] = statisTime.minute;
            flyEvent[4] = statisTime.hour;
            flyEvent[5] = statisTime.day;
            flyEvent[6] = statisTime.month;
       	    flyEvent[7] = statisTime.year;
  
            flyDataTail = 8;
  
    	      flyEvent[flyDataTail++] = pn&0xff;
    	      flyEvent[flyDataTail++] = (pn>>8&0xf) | 0x80; //飞走发生
       	    
       	    //飞走发生前正向有功示值 ly,2011-06-25,记录飞走发生前示值
            if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)
            {
       	      flyEvent[flyDataTail++] = 0xee;   //填充字节
       	      pStatisRecord->flyVision[0] = 0xee;
            }
            else
            {
       	      flyEvent[flyDataTail++] = 0;   //填充字节
       	      pStatisRecord->flyVision[0] = 0x0;
       	    }
     	      flyEvent[flyDataTail++] = pStatisRecord->flyVision[1] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
       	    flyEvent[flyDataTail++] = pStatisRecord->flyVision[2] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
       	    flyEvent[flyDataTail++] = pStatisRecord->flyVision[3] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
       	    flyEvent[flyDataTail++] = pStatisRecord->flyVision[4] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
       	    
       	    //飞走发生后正向有功示值
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
       	      flyEvent[flyDataTail++] = 0xee;   //填充字节
            }
            else
            {
       	      flyEvent[flyDataTail++] = 0;   //填充字节
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
      	//如果飞走已发生,记录恢复事件
      	if (pStatisRecord->flyFlag == 0x1)
      	{
   	      pStatisRecord->flyFlag = 0x0;     //飞走已恢复
   	      
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("电能表飞走:测量点%d已恢复\n",pn);
   	      }
   	      
       	  if ((eventRecordConfig.iEvent[3] & 0x10) || (eventRecordConfig.nEvent[3] & 0x10))
       	  {
     	      flyEvent[0] = 0x1D;    //ERC29
       	    
       	    flyEvent[2] = 0;       //填充字节
            flyEvent[3] = statisTime.minute;
            flyEvent[4] = statisTime.hour;
            flyEvent[5] = statisTime.day;
            flyEvent[6] = statisTime.month;
       	    flyEvent[7] = statisTime.year;

            flyDataTail = 8;
    	      flyEvent[flyDataTail++] = pn&0xff;
    	      flyEvent[flyDataTail++] = (pn>>8&0xff); //飞走恢复
       	    
       	    //飞走发生前正向有功示值
       	    //ly,2011-06-25,飞走前示值改成与发生时一样的飞走前示值
            //if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)            
            //{
     	      //  flyEvent[flyDataTail++] = 0xee;   //填充字节
            //}
            //else
            //{
     	      //  flyEvent[flyDataTail++] = 0;   //填充字节
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
     	      
     	      //飞走发生后正向有功示值
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
     	        flyEvent[flyDataTail++] = 0xee;   //填充字节
            }
            else
            {
     	        flyEvent[flyDataTail++] = 0;   //填充字节
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
   	  
   	  
   	  
   	  
   	  //开始判断电能量超差
      tmpOverGateHex = bcdToHex(meterGate.powerOverGate);
     	if (debugInfo&PRINT_BALANCE_DEBUG)
     	{
     	  printf("判断电能量超差:\n");
     	  printf("    超差阈值=%d\r\n", tmpOverGateHex);
     	  printf("    本次抄表电表示值=%d,上次抄表电能表示值=%d,两次抄表差值=%d\n", tmpPresHex, tmpLastHex, tmpMeterData);
     	  printf("    根据功率算出来的实际示值差值=%d\n", tmpCountData);
     	}

     	//每分钟阈乘以分钟数得到实际超差阈值
     	if (((tmpMeterData*1000/tmpCountData)<tmpGateHex) && ((tmpMeterData*1000/tmpCountData)>tmpOverGateHex))    //发生
     	{
   	    if (0x0==pStatisRecord->overFlag)
   	    {
     	    pStatisRecord->overFlag = 0x01;    //超差已发生
     	    
     	    if (debugInfo&PRINT_BALANCE_DEBUG)
     	    {
     	      printf("电能量超差:测量点%d发生\r\n", pn);
     	    }
  
       	  if ((eventRecordConfig.iEvent[3] & 0x08) || (eventRecordConfig.nEvent[3] & 0x08))
       	  {
     	      overEvent[0] = 28;    //ERC28
       	    
       	    overEvent[2] = 0;       //填充字节
            overEvent[3] = statisTime.minute;
            overEvent[4] = statisTime.hour;
            overEvent[5] = statisTime.day;
            overEvent[6] = statisTime.month;
       	    overEvent[7] = statisTime.year;
  
            overDataTail = 8;
  
    	      overEvent[overDataTail++] = pn&0xff;
    	      overEvent[overDataTail++] = (pn>>8&0xf) | 0x80; //超差发生
       	    
       	    //超差发生前正向有功示值
            if (lastLastCopyEnergy[POSITIVE_WORK_OFFSET]==0xee)
            {
       	      overEvent[overDataTail++] = 0xee;   //填充字节
       	      pStatisRecord->overVision[0] = 0xee;
            }
            else
            {
       	      overEvent[overDataTail++] = 0;   //填充字节
       	      pStatisRecord->overVision[0] = 0x0;
       	    }
     	      overEvent[overDataTail++] = pStatisRecord->overVision[1] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET];
       	    overEvent[overDataTail++] = pStatisRecord->overVision[2] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+1];
       	    overEvent[overDataTail++] = pStatisRecord->overVision[3] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+2];
       	    overEvent[overDataTail++] = pStatisRecord->overVision[4] = lastLastCopyEnergy[POSITIVE_WORK_OFFSET+3];
       	    
       	    //飞走发生后正向有功示值
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
       	      overEvent[overDataTail++] = 0xee;   //填充字节
            }
            else
            {
       	      overEvent[overDataTail++] = 0;   //填充字节
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
      	//如果超差已发生,记录恢复事件
      	if (0x01==pStatisRecord->overFlag)
      	{
   	      pStatisRecord->overFlag = 0x00;     //超差已恢复
   	      
   	      if (debugInfo&PRINT_BALANCE_DEBUG)
   	      {
   	        printf("电能量超差:测量点%d已恢复\r\n",pn);
   	      }
   	      
       	  if ((eventRecordConfig.iEvent[3] & 0x08) || (eventRecordConfig.nEvent[3] & 0x08))
       	  {
     	      overEvent[0] = 28;    //ERC28
       	    
       	    overEvent[2] = 0;       //填充字节
            overEvent[3] = statisTime.minute;
            overEvent[4] = statisTime.hour;
            overEvent[5] = statisTime.day;
            overEvent[6] = statisTime.month;
       	    overEvent[7] = statisTime.year;

            overDataTail = 8;
    	      overEvent[overDataTail++] = pn&0xff;
    	      overEvent[overDataTail++] = (pn>>8&0xff); //超差恢复
       	    
       	    //超差发生前正向有功示值
     	      overEvent[overDataTail++] = pStatisRecord->overVision[0];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[1];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[2];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[3];
     	      overEvent[overDataTail++] = pStatisRecord->overVision[4];
     	      
     	      //超差发生后正向有功示值
            if (pCopyEnergyBuff[POSITIVE_WORK_OFFSET]==0xee)            
            {
     	        overEvent[overDataTail++] = 0xee;   //填充字节
            }
            else
            {
     	        overEvent[overDataTail++] = 0;   //填充字节
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
  //若停走事件发生，记录停走事件
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
  
  //若示度下降事件发生，记录下降事件
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
  
  //若超差事件发生，记录超差事件
  if (overEvent[0] != 0x00)
  {
    if (eventRecordConfig.iEvent[3] & 0x08)
    {
      writeEvent(overEvent, overDataTail++, 1, DATA_FROM_GPRS);
      
      eventStatus[3] = eventStatus[3] | 0x08;
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
   	  {
   	    printf("保存测量点%d电能表电能量超差重要事件\r\n",pn);
   	  }
    }
 	  
 	  if (eventRecordConfig.nEvent[3] & 0x08)
    {
      writeEvent(overEvent, overDataTail++, 2, DATA_FROM_LOCAL);
      eventStatus[3] = eventStatus[3] | 0x08;
      
      if (debugInfo&PRINT_BALANCE_DEBUG)
 	    {
 	      printf("保存测量点%d电能表电能量超差一般事件\r\n",pn);
 	    }
    }
  }
  
  //若飞走事件发生，记录飞走事件
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
函数名称: realStatisticPerPoint
功能描述: 统计测量点本月到当前为止的参变量统计值
          读取前一日参变量统计值，与当日参变量统计置进行比较
调用函数:     
被调用函数:
输入参数:
输出参数:  
返回值： 
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
    	printf("realStatisticPerPoint:开始统计本月参变量,统计时间:%02x-%02x-%02x %02x:%02x:%02x\n",statisTime.year,statisTime.month,statisTime.day,statisTime.hour,statisTime.minute,statisTime.second);
    }

	  //读取本次抄表前一日的月冻结参变量数据
	  tmpTime = timeBcdToHex(statisTime);
    tmpTime = backTime(tmpTime, 0, 1, 0, 0, 0);
    
    //2012-09-11,修改这个错误,应该查找前一日从日末开始的统计数据
    tmpTime.hour   = 23;
    tmpTime.minute = 59;
    tmpTime.second = 59;
    
	  tmpTime = timeHexToBcd(tmpTime);
    
    //读取前一日参变量统计值，与当日参变量统计值进行比较
    //  如前一次统计数据不存在或上月统计已经结束应该重新统计当月数据,用本次实时结算的数据,不用计算
    bufHasData = FALSE;
    readTime = tmpTime;
    if (readMeterData(tmpBuff, pn, DAY_BALANCE, MONTH_BALANCE_PARA_DATA, &readTime, 0) == TRUE)
    {
    	bufHasData = TRUE;
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    		 printf("realStatisticPerPoint:有前一日参变量统计值,时间为%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
    	}
    }
    else
    {
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    		 printf("realStatisticPerPoint:无前一日参变量统计值\n");
    	}
    }
    
    if (bufHasData==FALSE)
    {
      //2012-09-11,修改这个错误,
      //backupTime = nextTime(sysTime, 60, 0);
      //for(i=sysTime.day; i>0; i--)
      
      //2012-09-11,遍历统计时间的前一天到该月的一日的最后一刻查找该天的最后一个实时结算数据
      //    找到最近的一个实时结算数据退出
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
    	    
    	    break;  //2012-09-11,添加
    	  }
    	  
    	  usleep(10);
    	}
    	
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    		 if (bufHasData==TRUE)
    		 {
    		    printf("realStatisticPerPoint:有最近一次参变量统计值,时间为:%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
    		 }
    		 else
    		 {
    		    printf("realStatisticPerPoint:无最近一次参变量统计值\n");
    	   }
    	}
    }
    
    //如果读到数据并且读到的时间与统计时间不同并且不是一次新的统计
    //if (bufHasData==TRUE && tmpBuff[NEXT_NEW_INSTANCE] != 0x01 
    //	  && (!(statisTime.year==readTime.year && statisTime.month==readTime.month && statisTime.day==readTime.day && statisTime.hour==readTime.hour && statisTime.minute==readTime.minute && statisTime.second==readTime.second))
    //2012-09-12,修改这个判断
    if (bufHasData==TRUE)
    {
    	printf("realStatisticPerPoint:测量点%d读到参变量统计数据\n", pn);
    	
    	//1.功率统计***********************************************************
    	//1.1 总有功功率统计
    	if (tmpBuff[MAX_TOTAL_POWER] != 0xEE)
    	{
    	  //总有功功率最大值统计,前一天有统计值,今天没有统计值，当月统计值设置为昨天的统计结果
    	  if (pBalanceParaBuff[MAX_TOTAL_POWER] == 0xEE)
    	  {
    	  	//月总有功功率最大值
    	    pBalanceParaBuff[MAX_TOTAL_POWER] = tmpBuff[MAX_TOTAL_POWER];
    	    pBalanceParaBuff[MAX_TOTAL_POWER+1] = tmpBuff[MAX_TOTAL_POWER+1];
    	    pBalanceParaBuff[MAX_TOTAL_POWER+2] = tmpBuff[MAX_TOTAL_POWER+2];
    	    
    	    //月总有功功率最大值时间
    	    pBalanceParaBuff[MAX_TOTAL_POWER_TIME] = tmpBuff[MAX_TOTAL_POWER_TIME];
    	    pBalanceParaBuff[MAX_TOTAL_POWER_TIME+1] = tmpBuff[MAX_TOTAL_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_TOTAL_POWER_TIME+2] = tmpBuff[MAX_TOTAL_POWER_TIME+2];
      	}
      	else  //前一天和当前都有统计值，比较后得到结果
      	{
    	    //前一天最大值大于当天最大值，月最大值取前一天最大值，否则取当天最大值
    	    if ((tmpBuff[MAX_TOTAL_POWER+2] > pBalanceParaBuff[MAX_TOTAL_POWER+2])
    	    	|| ((tmpBuff[MAX_TOTAL_POWER+2] == pBalanceParaBuff[MAX_TOTAL_POWER+2])&&(tmpBuff[MAX_TOTAL_POWER+1] > pBalanceParaBuff[MAX_TOTAL_POWER+1]))
    	    	  || ((tmpBuff[MAX_TOTAL_POWER+2] == pBalanceParaBuff[MAX_TOTAL_POWER+2])&&(tmpBuff[MAX_TOTAL_POWER+1] == pBalanceParaBuff[MAX_TOTAL_POWER+1])&&(tmpBuff[MAX_TOTAL_POWER] > pBalanceParaBuff[MAX_TOTAL_POWER])))
    	    {
    	      //日总有功功率最大值
    	      pBalanceParaBuff[MAX_TOTAL_POWER]   = tmpBuff[MAX_TOTAL_POWER];
    	      pBalanceParaBuff[MAX_TOTAL_POWER+1] = tmpBuff[MAX_TOTAL_POWER+1];
    	      pBalanceParaBuff[MAX_TOTAL_POWER+2] = tmpBuff[MAX_TOTAL_POWER+2];
    	      
    	      //日总有功功率最大值时间
    	      pBalanceParaBuff[MAX_TOTAL_POWER_TIME]   = tmpBuff[MAX_TOTAL_POWER_TIME];
    	      pBalanceParaBuff[MAX_TOTAL_POWER_TIME+1] = tmpBuff[MAX_TOTAL_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_TOTAL_POWER_TIME+2] = tmpBuff[MAX_TOTAL_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //日总有功功率为零时间
    	  //如果当天的功率为零时间不等于零，则到当天为止当月的有功功率为零时间为前一天统计值与当天统计值之和
    	  //否则等于前一天统计值
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
    	
    	//1.2 A相有功功率统计
    	if (tmpBuff[MAX_A_POWER] != 0xEE)
    	{
    	  //A相功功率最大值统计,前一天有统计值,今天没有统计值，当月统计值设置为昨天的统计结果
    	  if (pBalanceParaBuff[MAX_A_POWER] == 0xEE)
    	  {
    	  	//月A相有功功率最大值
    	    pBalanceParaBuff[MAX_A_POWER] = tmpBuff[MAX_A_POWER];
    	    pBalanceParaBuff[MAX_A_POWER+1] = tmpBuff[MAX_A_POWER+1];
    	    pBalanceParaBuff[MAX_A_POWER+2] = tmpBuff[MAX_A_POWER+2];
    	    
    	    //月A相有功功率最大值时间
    	    pBalanceParaBuff[MAX_A_POWER] = tmpBuff[MAX_A_POWER_TIME];
    	    pBalanceParaBuff[MAX_A_POWER+1] = tmpBuff[MAX_A_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_A_POWER+2] = tmpBuff[MAX_A_POWER_TIME+2];
      	}
      	else  //前一天和当前都有统计值，比较后得到结果
      	{
    	    if ((tmpBuff[MAX_A_POWER+2] > pBalanceParaBuff[MAX_A_POWER+2])
    	    	|| ((tmpBuff[MAX_A_POWER+2] == pBalanceParaBuff[MAX_A_POWER+2])&&(tmpBuff[MAX_A_POWER+1] > pBalanceParaBuff[MAX_A_POWER+1]))
    	    	  || ((tmpBuff[MAX_A_POWER+2] == pBalanceParaBuff[MAX_A_POWER+2])&&(tmpBuff[MAX_A_POWER+1] == pBalanceParaBuff[MAX_A_POWER+1])&&(tmpBuff[MAX_A_POWER] > pBalanceParaBuff[MAX_A_POWER])))
    	    {
    	      //日A相有功功率最大值
    	      pBalanceParaBuff[MAX_A_POWER] = tmpBuff[MAX_A_POWER];
    	      pBalanceParaBuff[MAX_A_POWER+1] = tmpBuff[MAX_A_POWER+1];
    	      pBalanceParaBuff[MAX_A_POWER+2] = tmpBuff[MAX_A_POWER+2];
    	      
    	      //日A相有功功率最大值时间
    	      pBalanceParaBuff[MAX_A_POWER_TIME] = tmpBuff[MAX_A_POWER_TIME];
    	      pBalanceParaBuff[MAX_A_POWER_TIME+1] = tmpBuff[MAX_A_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_A_POWER_TIME+2] = tmpBuff[MAX_A_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //日A相有功功率为零时间
    	  //如果当天的功率为零时间不等于零，则到当天为止当月的有功功率为零时间为前一天统计值与当天统计值之和
    	  //否则等于前一天统计值
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
    	
    	//1.3 B相有功功率统计
    	if (tmpBuff[MAX_B_POWER] != 0xEE)
    	{
    	  //总有功功率最大值统计,前一天有统计值,今天没有统计值，当月统计值设置为昨天的统计结果
    	  if (pBalanceParaBuff[MAX_B_POWER] == 0xEE)
    	  {
    	  	//月总有功功率最大值
    	    pBalanceParaBuff[MAX_B_POWER] = tmpBuff[MAX_B_POWER];
    	    pBalanceParaBuff[MAX_B_POWER+1] = tmpBuff[MAX_B_POWER+1];
    	    pBalanceParaBuff[MAX_B_POWER+2] = tmpBuff[MAX_B_POWER+2];
    	    
    	    //月总有功功率最大值时间
    	    pBalanceParaBuff[MAX_B_POWER_TIME] = tmpBuff[MAX_B_POWER_TIME];
    	    pBalanceParaBuff[MAX_B_POWER_TIME+1] = tmpBuff[MAX_B_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_B_POWER_TIME+2] = tmpBuff[MAX_B_POWER_TIME+2];
      	}
      	else  //前一天和当前都有统计值，比较后得到结果
      	{
    	    if ((tmpBuff[MAX_B_POWER+2] > pBalanceParaBuff[MAX_B_POWER+2])
    	    	|| ((tmpBuff[MAX_B_POWER+2] == pBalanceParaBuff[MAX_B_POWER+2])&&(tmpBuff[MAX_B_POWER+1] > pBalanceParaBuff[MAX_TOTAL_POWER+1]))
    	    	  || ((tmpBuff[MAX_B_POWER+2] == pBalanceParaBuff[MAX_B_POWER+2])&&(tmpBuff[MAX_B_POWER+1] == pBalanceParaBuff[MAX_TOTAL_POWER+1])&&(tmpBuff[MAX_B_POWER] > pBalanceParaBuff[MAX_B_POWER])))
    	    {
    	      //日总有功功率最大值
    	      pBalanceParaBuff[MAX_B_POWER] = tmpBuff[MAX_B_POWER];
    	      pBalanceParaBuff[MAX_B_POWER+1] = tmpBuff[MAX_B_POWER+1];
    	      pBalanceParaBuff[MAX_B_POWER+2] = tmpBuff[MAX_B_POWER+2];
    	      
    	      //日总有功功率最大值时间
    	      pBalanceParaBuff[MAX_B_POWER_TIME] = tmpBuff[MAX_B_POWER_TIME];
    	      pBalanceParaBuff[MAX_B_POWER_TIME+1] = tmpBuff[MAX_B_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_B_POWER_TIME+2] = tmpBuff[MAX_B_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //日总有功功率为零时间
    	  //如果当天的功率为零时间不等于零，则到当天为止当月的有功功率为零时间为前一天统计值与当天统计值之和
    	  //否则等于前一天统计值
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
    	
    	//1.4 C相有功功率统计
    	if (tmpBuff[MAX_C_POWER] != 0xEE)
    	{
    	  //总有功功率最大值统计,前一天有统计值,今天没有统计值，当月统计值设置为昨天的统计结果
    	  if (pBalanceParaBuff[MAX_C_POWER] == 0xEE)
    	  {
    	  	//月总有功功率最大值
    	    pBalanceParaBuff[MAX_C_POWER] = tmpBuff[MAX_C_POWER];
    	    pBalanceParaBuff[MAX_C_POWER+1] = tmpBuff[MAX_C_POWER+1];
    	    pBalanceParaBuff[MAX_C_POWER+2] = tmpBuff[MAX_C_POWER+2];
    	    
    	    //月总有功功率最大值时间
    	    pBalanceParaBuff[MAX_C_POWER_TIME] = tmpBuff[MAX_C_POWER_TIME];
    	    pBalanceParaBuff[MAX_C_POWER_TIME+1] = tmpBuff[MAX_C_POWER_TIME+1];
    	    pBalanceParaBuff[MAX_C_POWER_TIME+2] = tmpBuff[MAX_C_POWER_TIME+2];
      	}
      	else  //前一天和当前都有统计值，比较后得到结果
      	{
    	    if ((tmpBuff[MAX_C_POWER+2] > pBalanceParaBuff[MAX_C_POWER+2])
    	    	|| ((tmpBuff[MAX_C_POWER+2] == pBalanceParaBuff[MAX_C_POWER+2])&&(tmpBuff[MAX_C_POWER+1] > pBalanceParaBuff[MAX_C_POWER+1]))
    	    	  || ((tmpBuff[MAX_C_POWER+2] == pBalanceParaBuff[MAX_C_POWER+2])&&(tmpBuff[MAX_C_POWER+1] == pBalanceParaBuff[MAX_C_POWER+1])&&(tmpBuff[MAX_C_POWER] > pBalanceParaBuff[MAX_C_POWER])))
    	    {
    	      //日总有功功率最大值
    	      pBalanceParaBuff[MAX_C_POWER] = tmpBuff[MAX_C_POWER];
    	      pBalanceParaBuff[MAX_C_POWER+1] = tmpBuff[MAX_C_POWER+1];
    	      pBalanceParaBuff[MAX_C_POWER+2] = tmpBuff[MAX_C_POWER+2];
    	      
    	      //日总有功功率最大值时间
    	      pBalanceParaBuff[MAX_C_POWER_TIME] = tmpBuff[MAX_C_POWER_TIME];
    	      pBalanceParaBuff[MAX_C_POWER_TIME+1] = tmpBuff[MAX_C_POWER_TIME+1];
    	      pBalanceParaBuff[MAX_C_POWER_TIME+2] = tmpBuff[MAX_C_POWER_TIME+2];
    	    }
    	  }
    	  
    	  //日总有功功率为零时间
    	  //如果当天的功率为零时间不等于零，则到当天为止当月的有功功率为零时间为前一天统计值与当天统计值之和
    	  //否则等于前一天统计值
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
    	
    	//2.电压统计***********************************************************
    	//2.1 A相电压统计
    	if (tmpBuff[VOL_A_UP_UP_TIME] != 0xEE) //越上上限
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
      
      if (tmpBuff[VOL_A_UP_TIME] != 0xEE) //越上限
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
      
      if (tmpBuff[VOL_A_DOWN_DOWN_TIME] != 0xEE) //越下下限
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
      
      if (tmpBuff[VOL_A_DOWN_TIME] != 0xEE) //越下限
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
      
      if (tmpBuff[VOL_A_OK_TIME] != 0xEE) //合格
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
      
      //月电压最大值及发生时间
      if (tmpBuff[VOL_A_MAX]!=0xEE)
      {
      	//若前一天统计最大值大于当天统计最大值或当天统计最大值没有结果，
      	//当天统计最大值取前一天统计最大值
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

      //月电压最小值及发生时间
      if (tmpBuff[VOL_A_MIN] != 0xEE && (tmpBuff[VOL_A_MIN] != 0x00&&tmpBuff[VOL_A_MIN] != 0x00))
      {
        //若前一天最小值不为零且小于当天最小值或者当天最小值不存在
        //当天最小值取前一天最小值
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

      //月电压平均值
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

    	//2.2 B相电压统计
    	if (tmpBuff[VOL_B_UP_UP_TIME] != 0xEE) //越上上限
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
      
      if (tmpBuff[VOL_B_UP_TIME] != 0xEE) //越上限
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
      
      if (tmpBuff[VOL_B_DOWN_DOWN_TIME] != 0xEE) //越下下限
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
      
      if (tmpBuff[VOL_B_DOWN_TIME] != 0xEE) //越下限
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
      
      if (tmpBuff[VOL_B_OK_TIME] != 0xEE) //合格
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
      
      //月电压最大值及发生时间
      if (tmpBuff[VOL_B_MAX]!=0xEE)
      {
      	//若前一天统计最大值大于当天统计最大值或当天统计最大值没有结果，
      	//当天统计最大值取前一天统计最大值
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
      
      //月电压最小值及发生时间
      if (tmpBuff[VOL_B_MIN] != 0xEE && (tmpBuff[VOL_B_MIN] != 0x00&&tmpBuff[VOL_B_MIN] != 0x00))
      {
        //若前一天最小值不为零且小于当天最小值或者当天最小值不存在
        //当天最小值取前一天最小值
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
     
      //月电压平均值
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
      
      //2.3 C相电压统计
    	if (tmpBuff[VOL_C_UP_UP_TIME] != 0xEE) //越上上限
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
      
      if (tmpBuff[VOL_C_UP_TIME] != 0xEE) //越上限
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
      
      if (tmpBuff[VOL_C_DOWN_DOWN_TIME] != 0xEE) //越下下限
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
      
      if (tmpBuff[VOL_C_DOWN_TIME] != 0xEE) //越下限
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
      
      if (tmpBuff[VOL_C_OK_TIME] != 0xEE) //合格
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
      
      //月电压最大值及发生时间
      if (tmpBuff[VOL_C_MAX]!=0xEE)
      {
      	//若前一天统计最大值大于当天统计最大值或当天统计最大值没有结果，
      	//当天统计最大值取前一天统计最大值
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
      
      //月电压最小值及发生时间
      if (tmpBuff[VOL_C_MIN] != 0xEE && (tmpBuff[VOL_C_MIN] != 0x00&&tmpBuff[VOL_C_MIN] != 0x00))
      {
        //若前一天最小值不为零且小于当天最小值或者当天最小值不存在
        //当天最小值取前一天最小值
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
     
      //月电压平均值
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
        printf("realStatisticPerPoint:A相月电压越上上限时间=%d\n",pBalanceParaBuff[VOL_A_UP_UP_TIME]| pBalanceParaBuff[VOL_A_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电压越上限时间=%d\n",pBalanceParaBuff[VOL_A_UP_TIME]| pBalanceParaBuff[VOL_A_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电压越下下限时间=%d\n",pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_A_DOWN_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电压越下限时间=%d\n",pBalanceParaBuff[VOL_A_DOWN_TIME]| pBalanceParaBuff[VOL_A_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电压合格时间=%d\n",pBalanceParaBuff[VOL_A_OK_TIME]| pBalanceParaBuff[VOL_A_OK_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电压最大值=%x及发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_A_MAX]| pBalanceParaBuff[VOL_A_MAX+1]<<8,pBalanceParaBuff[VOL_A_MAX_TIME+2],pBalanceParaBuff[VOL_A_MAX_TIME+1],pBalanceParaBuff[VOL_A_MAX_TIME]);
        printf("realStatisticPerPoint:A相月电压最小值=%x及发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_A_MIN]| pBalanceParaBuff[VOL_A_MIN+1]<<8,pBalanceParaBuff[VOL_A_MIN_TIME+2],pBalanceParaBuff[VOL_A_MIN_TIME+1],pBalanceParaBuff[VOL_A_MIN_TIME]);
        printf("realStatisticPerPoint:A相月电压平均值=%x\n",pBalanceParaBuff[VOL_A_AVER]| pBalanceParaBuff[VOL_A_AVER+1]<<8);

        printf("realStatisticPerPoint:B相月电压越上上限时间=%d\n",pBalanceParaBuff[VOL_B_UP_UP_TIME]| pBalanceParaBuff[VOL_B_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:B相月电压越上限时间=%d\n",pBalanceParaBuff[VOL_B_UP_TIME]| pBalanceParaBuff[VOL_B_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:B相月电压越下下限时间=%d\n",pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_B_DOWN_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:B相月电压越下限时间=%d\n",pBalanceParaBuff[VOL_B_DOWN_TIME]| pBalanceParaBuff[VOL_B_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:B相月电压合格时间=%d\n",pBalanceParaBuff[VOL_B_OK_TIME]| pBalanceParaBuff[VOL_B_OK_TIME+1]<<8);
        printf("realStatisticPerPoint:B相月电压最大值=%x及发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_B_MAX]| pBalanceParaBuff[VOL_B_MAX+1]<<8,pBalanceParaBuff[VOL_B_MAX_TIME+2],pBalanceParaBuff[VOL_B_MAX_TIME+1],pBalanceParaBuff[VOL_B_MAX_TIME]);
        printf("realStatisticPerPoint:B相月电压最小值=%x及发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_B_MIN]| pBalanceParaBuff[VOL_B_MIN+1]<<8,pBalanceParaBuff[VOL_B_MIN_TIME+2],pBalanceParaBuff[VOL_B_MIN_TIME+1],pBalanceParaBuff[VOL_B_MIN_TIME]);
        printf("realStatisticPerPoint:B相月电压平均值=%x\n",pBalanceParaBuff[VOL_B_AVER]| pBalanceParaBuff[VOL_B_AVER+1]<<8);

        printf("realStatisticPerPoint:C相月电压越上上限时间=%d\n",pBalanceParaBuff[VOL_C_UP_UP_TIME]| pBalanceParaBuff[VOL_C_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:C相月电压越上限时间=%d\n",pBalanceParaBuff[VOL_C_UP_TIME]| pBalanceParaBuff[VOL_C_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:C相月电压越下下限时间=%d\n",pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME]| pBalanceParaBuff[VOL_C_DOWN_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:C相月电压越下限时间=%d\n",pBalanceParaBuff[VOL_C_DOWN_TIME]| pBalanceParaBuff[VOL_C_DOWN_TIME+1]<<8);
        printf("realStatisticPerPoint:C相月电压合格时间=%d\n",pBalanceParaBuff[VOL_C_OK_TIME]| pBalanceParaBuff[VOL_C_OK_TIME+1]<<8);
        printf("realStatisticPerPoint:C相月电压最大值=%x及发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_C_MAX]| pBalanceParaBuff[VOL_C_MAX+1]<<8,pBalanceParaBuff[VOL_C_MAX_TIME+2],pBalanceParaBuff[VOL_C_MAX_TIME+1],pBalanceParaBuff[VOL_C_MAX_TIME]);
        printf("realStatisticPerPoint:C相月电压最小值=%x及发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_C_MIN]| pBalanceParaBuff[VOL_C_MIN+1]<<8,pBalanceParaBuff[VOL_C_MIN_TIME+2],pBalanceParaBuff[VOL_C_MIN_TIME+1],pBalanceParaBuff[VOL_C_MIN_TIME]);
        printf("realStatisticPerPoint:C相月电压平均值=%x\n",pBalanceParaBuff[VOL_C_AVER]| pBalanceParaBuff[VOL_C_AVER+1]<<8);
      }


	    //3.不平衡度越限累计***********************************************************
	    //3.1 电压不平衡越限累计时间
	    if (tmpBuff[VOL_UNBALANCE_TIME] != 0xEE) //电压不平衡
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
      
      //3.2 电压不平衡越限最大值及发生时间
      if (tmpBuff[VOL_UNB_MAX] != 0xEE)  //电压不平衡最大值
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
	    //3.3 电流不平衡越限累计时间
	    if (tmpBuff[CUR_UNBALANCE_TIME] != 0xEE) //电流不平衡
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
      
      //3.4 电流不平衡越限最大值及发生时间
      if (tmpBuff[CUR_UNB_MAX] != 0xEE)  //电流不平衡最大值
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
        printf("realStatisticPerPoint:电压不平衡累计时间=%d\n",pBalanceParaBuff[VOL_UNBALANCE_TIME]| pBalanceParaBuff[VOL_UNBALANCE_TIME+1]<<8);
        printf("realStatisticPerPoint:电压不平衡度最大值=%x,发生时间=%02x %02x:%02x\n",pBalanceParaBuff[VOL_UNB_MAX]| pBalanceParaBuff[VOL_UNB_MAX+1]<<8,tmpBuff[VOL_UNB_MAX_TIME+2],tmpBuff[VOL_UNB_MAX_TIME+1],tmpBuff[VOL_UNB_MAX_TIME]);
        printf("realStatisticPerPoint:电流不平衡累计时间=%d\n",pBalanceParaBuff[CUR_UNBALANCE_TIME]| pBalanceParaBuff[CUR_UNBALANCE_TIME+1]<<8);
        printf("realStatisticPerPoint:电流不平衡度最大值=%x,发生时间=%02x %02x:%02x\n",pBalanceParaBuff[CUR_UNB_MAX]| pBalanceParaBuff[CUR_UNB_MAX+1]<<8,tmpBuff[CUR_UNB_MAX_TIME+2],tmpBuff[CUR_UNB_MAX_TIME+1],tmpBuff[CUR_UNB_MAX_TIME]);
      }

	    //4.电流统计***********************************************************
	    //4.1 A相电流统计
	    if (tmpBuff[CUR_A_UP_UP_TIME] != 0xEE) //越上上限
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

      if (tmpBuff[CUR_A_UP_TIME] != 0xEE)  //越上限
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

      if (tmpBuff[CUR_A_MAX] != 0xEE)  //电流最大值
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
	    
	    //4.2 B相电流统计
	    if (tmpBuff[CUR_B_UP_UP_TIME] != 0xEE) //越上上限
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
      
      if (tmpBuff[CUR_B_UP_TIME] != 0xEE)  //越上限
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
      
      if (tmpBuff[CUR_B_MAX] != 0xEE)  //电流最大值
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
	    
	    //4.3 C相电流统计
	    if (tmpBuff[CUR_C_UP_UP_TIME] != 0xEE) //越上上限
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
      
      if (tmpBuff[CUR_C_UP_TIME] != 0xEE)  //越上限
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
      
      if (tmpBuff[CUR_C_MAX] != 0xEE)  //电流最大值
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
        printf("realStatisticPerPoint:A相月电流越上上限时间=%d\n", pBalanceParaBuff[CUR_A_UP_UP_TIME] | pBalanceParaBuff[CUR_A_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电流越上限时间=%d\n", pBalanceParaBuff[CUR_A_UP_TIME] | pBalanceParaBuff[CUR_A_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电流最大值=%x,发生时间=%02x %02x:%02x\n", pBalanceParaBuff[CUR_A_MAX] | pBalanceParaBuff[CUR_A_MAX+1]<<8,pBalanceParaBuff[CUR_A_MAX_TIME+2],pBalanceParaBuff[CUR_A_MAX_TIME+1],pBalanceParaBuff[CUR_A_MAX_TIME]);

        printf("realStatisticPerPoint:A相月电流越上上限时间=%d\n", pBalanceParaBuff[CUR_B_UP_UP_TIME] | pBalanceParaBuff[CUR_B_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电流越上限时间=%d\n", pBalanceParaBuff[CUR_B_UP_TIME] | pBalanceParaBuff[CUR_B_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电流最大值=%x,发生时间=%02x %02x:%02x\n", pBalanceParaBuff[CUR_B_MAX] | pBalanceParaBuff[CUR_B_MAX+1]<<8,pBalanceParaBuff[CUR_B_MAX_TIME+2],pBalanceParaBuff[CUR_B_MAX_TIME+1],pBalanceParaBuff[CUR_B_MAX_TIME]);

        printf("realStatisticPerPoint:A相月电流越上上限时间=%d\n", pBalanceParaBuff[CUR_C_UP_UP_TIME] | pBalanceParaBuff[CUR_C_UP_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电流越上限时间=%d\n", pBalanceParaBuff[CUR_C_UP_TIME] | pBalanceParaBuff[CUR_C_UP_TIME+1]<<8);
        printf("realStatisticPerPoint:A相月电流最大值=%x,发生时间=%02x %02x:%02x\n", pBalanceParaBuff[CUR_C_MAX] | pBalanceParaBuff[CUR_C_MAX+1]<<8,pBalanceParaBuff[CUR_C_MAX_TIME+2],pBalanceParaBuff[CUR_C_MAX_TIME+1],pBalanceParaBuff[CUR_C_MAX_TIME]);
      }

	    //5.视在功率越限累计时间***********************************************************
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
	    
	    //6.电能表断相数据***********************************************************
	    //6.1 总断相统计
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
	    
	    //6.2 A相断相统计
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
	    
	    //6.3 B相断相统计
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
	    
	    //6.4 C相断相统计
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
	    
	    //7.需量统计***********************************************************
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
	    
	    //8.功率因数分段累计统计***********************************************************
	    //8.1 月功率因数区段1累计时间
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
	    //8.2 月功率因数区段2累计时间
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
	    //8.3 月功率因数区段3累计时间
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
函数名称: copyDayFreeze
功能描述: 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值： 
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
	      //通信规约配置为现场监测设备或是交流采样装置，则无实时结算的数据项
	      //if (tmpNode->protocol == AC_SAMPLE || tmpNode->protocol == SUPERVISAL_DEVICE)    
	      if (tmpNode->protocol == SUPERVISAL_DEVICE)    
	      {
	        tmpNode = tmpNode->next;   //bug,2011-08-03
	        
	        continue;
	      }

        //读取当前数据并保存
	      tmpTime = timeHexToBcd(sysTime);
        if (readMeterData(readBuff, tmpNode->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0) == TRUE)
        {
          //转存为抄表日冻结电能示值
	        tmpTime = timeHexToBcd(sysTime);
          tmpTime.minute = teCopyRunPara.para[port].copyTime[0];
  	  	  tmpTime.hour = teCopyRunPara.para[port].copyTime[1];
  	  	 
          saveMeterData(tmpNode->mp, port+1, tmpTime, readBuff, DAY_BALANCE, COPY_FREEZE_COPY_DATA,LENGTH_OF_ENERGY_RECORD);
        }
        
	      tmpTime = timeHexToBcd(sysTime);
        if (readMeterData(readBuff, tmpNode->mp, LAST_TODAY, REQ_REQTIME_DATA, &tmpTime, 0) == TRUE)
        {
          //转存为抄表日冻结需量
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
函数名称: computeLeftPower
功能描述: 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值：
修改历史:
    1.2012-5-22,修改购电控未投入时,当用电量超过剩余电量后,会出现剩余电量变成负值的错误.
      比如刷新的剩余电量为1000Kwh,当用电量超过1000后,剩余电量就会变成-1000,以后就不会再变
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
    	 printf("computeLeftPower:开始计算剩余电量\n");
    }
    
   	if (totalAddGroup.numberOfzjz != 0)
    {
      //根据总加组数配置进行处理
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
    	         printf("computeLeftPower:总加组%d购电控投入状态且本总加组只有脉冲测量点,不用在这里计算剩余电量\n", pn);
             }
 	    		
 	    		   continue;
 	    		}
 	    	}

  	    
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
    	    printf("computeLeftPower:计算总加组%d剩余电量\n", pn);
        }
  	    
        //读取当前剩余电量
        readTime = statisTime;
        if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
        {
     	    //读取当前总加组当月电能量
     	    readTime = statisTime;
        	if (readMeterData(readBuff, pn, REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0) == TRUE)
  	      {
  	      	if (readBuff[GP_MONTH_WORK+3]!= 0xFF && readBuff[GP_MONTH_WORK+3] != 0xEE)
  	      	{
  	      			tmpData1 = 0x00000000;
      	        tmpData2 = 0x00000000;
      	        
      	        //参考总加组当月电能量C
      	        if (leftPower[8]==0xee)  //如果原来无总加组月电能量数据,将当前总加月电能量给C
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
          	    
          	    //本次的月总加电量D
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
      	        
      	        //当前总加月电能量D-参考总加月电能量C
    	          if (debugInfo&PRINT_BALANCE_DEBUG)
    	          {
    	            printf("computeLeftPower:当前总加月电能量D=%d,参考总加月电能量C=%d\n",tmpData2,tmpData1);
    	          }
      	        if (tmpData2 >= tmpData1)
      	        {
      	          tmpData2 -= tmpData1;
      	            
      	          //参考剩余电能量B
      	          tmpData1 = (leftPower[4]&0xF)       + ((leftPower[4]>>4)&0xF)*10
      	                    + (leftPower[5]&0xF)*100   + ((leftPower[5]>>4)&0xF)*1000
      	                    + (leftPower[6]&0xF)*10000 + ((leftPower[6]>>4)&0xF)*100000
      	                    + (leftPower[7]&0xF)*1000000;
          	      if ((leftPower[7]>>6)&0x01 == 0x01)
          	      {
          	        tmpData1 *= 1000;
          	      }

          	      if ((leftPower[7]>>4)&0x01 == 0x01)   //剩余电能量为负
          	      {
        	           if (debugInfo&PRINT_BALANCE_DEBUG)
        	           {
        	             printf("computeLeftPower:当前参考剩余电能量为负\n");
        	           }
    
                     if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                     {
        	             printf("computeLeftPower(当前参考剩余电能量为负):未投入,不减剩余电量\n");
                     }
                     else
                     {
                       tmpData1 += tmpData2;
    
      	        	     tmpData2 = 0x00000000;
      	               quantity = dataFormat(&tmpData1, &tmpData2, 0x03);
      	    
      	               tmpData1 = hexToBcd(tmpData1);
      	     
      	               leftPower[0] = tmpData1&0xFF;
      	               leftPower[1] = (tmpData1>>8)&0xFF;
      	               leftPower[2] = (tmpData1>>16)&0xFF;//查找总加组序号
      	               leftPower[3] = (tmpData1>>24)&0xFF;
      	    
      	               leftPower[3] |= (1 <<4);    //本来参考剩余电能量为负,所以减后还是为负
      	             
                       saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                     }
          	      }
          	      else
          	      {
                    //当前剩余电能量A=B-(D-C)
                    if (tmpData1>= tmpData2)
                    {
                      if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                      {
      	                if (debugInfo&PRINT_BALANCE_DEBUG)
      	                {
      	                  printf("computeLeftPower:当前剩余电量>当月电量,购电控未投入,不减\n");
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
      	                  printf("computeLeftPower:当前剩余电量<当月电量,购电控未投入,不减\n");
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
    	          printf("computeLeftPower:读到当前总加组当月电能量,但月电量为0xee\n", pn);
              }
              
  	      	  continue;
  	      	}
  	    	}
  	    	else
  	    	{
            if (debugInfo&PRINT_BALANCE_DEBUG)
            {
    	        printf("computeLeftPower:未读到当前总加组当月电能量\n", pn);
            }
  	    	}
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
    	      printf("computeLeftPower:未读到剩余电量\n", pn);
          }
  	    }
      }
    }
}

/*******************************************************
函数名称:computeInTimeLeftPower
功能描述:结算及时剩余电量
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值： 
*******************************************************/
BOOL computeInTimeLeftPower(INT8U zjzNo, DATE_TIME statisTime, INT8U *leftPower, INT8U ifSave)
{
    INT16U    i, pn, kk;
    INT8U     tmpByte, quantity;
    INT32U    tmpData1 , tmpData2;
    INT8U     readBuff[LEN_OF_ZJZ_BALANCE_RECORD];
    DATE_TIME readTime;

    pn = zjzNo;

    //读取当前剩余电量
    readTime = statisTime;
    if (readMeterData(leftPower, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
    {
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("computeInTimeLeftPower:读取剩余电量成功\n");
    	}

 	    //读取当前总加组当月电能量
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
  	        
  	        //参考总加组当月电能量C
  	        if (leftPower[8]==0xee)  //如果原来无总加组月电能量数据,将当前总加月电能量给C
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
      	    
      	    //本次的月总加电量D
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
    	        printf("computeInTimeLeftPower:当前总加月电能量D=%d,参考总加月电能量C=%d\n",tmpData2,tmpData1);
    	      }
  	        
  	        //当前总加月电能量D-参考总加月电能量C
  	        if (tmpData2 >= tmpData1)
  	        {
  	          tmpData2 -= tmpData1;
  	            
  	          //参考剩余电能量B
  	          tmpData1 = (leftPower[4]&0xF)       + ((leftPower[4]>>4)&0xF)*10
  	                    + (leftPower[5]&0xF)*100   + ((leftPower[5]>>4)&0xF)*1000
  	                    + (leftPower[6]&0xF)*10000 + ((leftPower[6]>>4)&0xF)*100000
  	                    + (leftPower[7]&0xF)*1000000;
      	      if ((leftPower[7]>>6)&0x01 == 0x01)
      	      {
      	        tmpData1 *= 1000;
      	      }

      	      if ((leftPower[7]>>4)&0x01 == 0x01)   //剩余电能量为负
      	      {
    	           if (debugInfo&PRINT_BALANCE_DEBUG)
    	           {
    	             printf("computeInTimeLeftPower:当前参考剩余电能量为负\n");
    	           }

                 if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                 {
    	             printf("computeInTimeLeftPower(当前参考剩余电能量为负):未投入,不减剩余电量\n");
                 }
                 else
                 {
                   tmpData1 += tmpData2;

  	        	     tmpData2 = 0x00000000;
  	               quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
  	    
  	               tmpData1 = hexToBcd(tmpData1);
  	     
  	               leftPower[0] = tmpData1&0xFF;
  	               leftPower[1] = (tmpData1>>8)&0xFF;
  	               leftPower[2] = (tmpData1>>16)&0xFF;//查找总加组序号
  	               leftPower[3] = (tmpData1>>24)&0xFF;
  	    
  	               leftPower[3] |= (1 <<4);    //本来参考剩余电能量为负,所以减后还是为负
  	             
  	               if (ifSave)
  	               {
                     saveMeterData(pn, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
                   }
                 }
                 return TRUE;
      	      }
      	      else
      	      {
                //当前剩余电能量A=B-(D-C)
                if (tmpData1>= tmpData2)
                {
      	           if (debugInfo&PRINT_BALANCE_DEBUG)
      	           {
      	             printf("computeInTimeLeftPower:当前剩余电能量>当前月电量\n");
      	           }
  
                   if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                   {
      	             printf("computeInTimeLeftPower(当前剩余电能量>当前月电量):未投入,不减剩余电量\n");
                   }
                   else
                   {
                     tmpData1 -= tmpData2;
  
    	        	     tmpData2 = 0x00000000;
    	               quantity = dataFormat(&tmpData1,&tmpData2, 0x03);
    	    
    	               tmpData1 = hexToBcd(tmpData1);
    	     
    	               leftPower[0] = tmpData1&0xFF;
    	               leftPower[1] = (tmpData1>>8)&0xFF;
    	               leftPower[2] = (tmpData1>>16)&0xFF;//查找总加组序号
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
      	             printf("computeInTimeLeftPower:当前剩余电能量<当前月电量,剩余为负\n");
      	           }
      	           
                   if (ctrlRunStatus[pn-1].ifUseChgCtrl != CTRL_JUMP_IN)
                   {
      	             if (debugInfo&PRINT_BALANCE_DEBUG)
      	             {
      	               printf("computeInTimeLeftPower:当前剩余电能量<当前月电量,购电控未投入,不减\n");
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
    	      printf("computeInTimeLeftPower:实时计算月电能量返回TRUE,但月电能量为0xee\n");
    	    }

      		return TRUE;
      	}
      }
      else
      {
    	  if (debugInfo&PRINT_BALANCE_DEBUG)
    	  {
    	    printf("computeInTimeLeftPower:实时计算月电能量返回FALSE\n");
    	  }

      	return TRUE;
      }
    }
    else
    {
    	if (debugInfo&PRINT_BALANCE_DEBUG)
    	{
    	  printf("computeInTimeLeftPower:未读到剩余电量\n");
    	}
    }
    
    return FALSE;
}

/*******************************************************
函数名称: cLoopEvent
功能描述: 电流回路异常事件记录
调用函数:     
被调用函数:
输入参数:phase,相(0x01-A, 0x02-B, 0x04-C)
         pn,测量点
         whichLimit,所越限(1-短路,2-开路,3-反向)
         recovery,事件发生还是恢复
输出参数:  
返回值：
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
 
  //发生时的Ua/Uab
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A];
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_A+1];

  //发生时的Ub
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B];
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_B+1];

  //发生时的Uc/Ucb
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C];
  eventData[dataTail++] = pCopyParaBuff[VOLTAGE_PHASE_C+1];
  
  //发生时的Ia
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+1];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_A+2];

  //发生时的Ib
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+1];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_B+2];
  
  //发生时的Ic
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+1];
  eventData[dataTail++] = pCopyParaBuff[CURRENT_PHASE_C+2];
  
  //发生时电能表正向有功总电能示值
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
函数名称: cOverLimitEvent
功能描述: 电流越限事件记录
调用函数:     
被调用函数:
输入参数:相(0x01, 0x02, 0x04 a, b, c)
         测量点
         所越限
         是否事件恢复
输出参数:  
返回值：
*******************************************************/
void cOverLimitEvent(INT8U *pCopyParaBuff, INT8U phase, INT16U pn, INT8U whichLimit, BOOL recovery, DATE_TIME statisTime)
{          
    INT8U  eventData[25];
    INT8U  dataTail;
    
    //记录电流越限事件
    eventData[0] = 25;
    
    eventData[2] = statisTime.minute;
    eventData[3] = statisTime.hour;
    eventData[4] = statisTime.day;
    eventData[5] = statisTime.month;
    eventData[6] = statisTime.year;
    
    dataTail = 7;
    
    eventData[dataTail++] = pn&0xff;   //测量点低8位    
    eventData[dataTail] = (pn>>8)&0xf;    

    if (recovery == FALSE)
    {
      eventData[dataTail] |= 0x80;  //事件发生
    }
    dataTail++;
    
    eventData[dataTail] = 1<<(phase-1);   //相
    
	 #ifdef LIGHTING	
    if (whichLimit == 0) //越下下限
    {
    	eventData[dataTail] |= 0x00;
    }
	 #endif
    if (whichLimit == 2) //越上上限
    {
      eventData[dataTail] |= 0x40;
    }
    if (whichLimit == 1) //越上限
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
 	     writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
 	  }
    if (eventRecordConfig.nEvent[3] & 0x01)
    {
 	     writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入一般事件队列
 	  }
 	  
 	  eventStatus[3] = eventStatus[3] | 0x01;  
}

/*******************************************************
函数名称: vOverLimitEvent
功能描述: 电压越限事件记录
调用函数:     
被调用函数:
输入参数:相(0x01, 0x02, 0x04 a, b, c)
         测量点
         所越限
         是否事件恢复
输出参数:  
返回值：
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
    
    eventData[dataTail++] = pn&0xff;   //测量点低8位    
    eventData[dataTail] = (pn>>8)&0xf;    
    
    if (recovery == FALSE)
    {
      eventData[dataTail] |= 0x80;
    }    
    dataTail++;

    eventData[dataTail] = 1<<(phase-1);
    
    if (whichLimit == 2) //越上上限
    {
      eventData[dataTail] |= 0x40;
    }
    if (whichLimit == 1) //越下下限
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
       writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
    }
    if (eventRecordConfig.nEvent[2] & 0X80)
    {
       writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入一般事件队列
    }
    
    eventStatus[2] = eventStatus[2] | 0x80;
}

/*******************************************************
函数名称: vAbnormalEvent
功能描述: 电压回路异常事件记录
调用函数:     
被调用函数:
输入参数:相(0x01, 0x02, 0x04 a, b, c)
         测量点
         所越限
         是否事件恢复
输出参数:  
返回值：
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
      writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
    }
    if (eventRecordConfig.nEvent[1] & 0x02)
    {
   	  writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入一般事件队列
   	}
   	  
   	eventStatus[1] = eventStatus[1] | 0x02;
}

/*******************************************************
函数名称:pOverLimitEvent
功能描述:视在功率越限事件
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
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
    
    eventData[dataTail++] = pn&0xff;   //测量点低8位    
    eventData[dataTail] = (pn>>8)&0xf;    
  	
  	if (recovery == FALSE)
  	{
  	  eventData[dataTail] |= 0x80;
  	}
  	dataTail++;
  	
  	if (type == 0x00)  //越下限
  	{
  	  eventData[dataTail] = 0x00;
  	}

  	//越上上限
	if (type == 0x01)
  	{
  	  eventData[dataTail] = 0x40;
  	}
  	if (
        type == 0x02
		 || type==0x03
	   )  //越上限
  	{
  	  eventData[dataTail] = 0x80;
  	}
  	dataTail++;
  	
  	//当前视在功率
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
       writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
    }
    if (eventRecordConfig.nEvent[3] & 0x02)
    {
 	  writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入一般事件队列
 	}
 	  
 	eventStatus[3] = eventStatus[3] | 0x02;
}

/*******************************************************
函数名称:changeEvent
功能描述:电表参数变更及电表故障信息事件
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
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
      printf("判断测量点%d电能表参数及故障事件.统计时间=%02x-%02x-%02x %02x:%02x:%02x\n",pn,statisTime.year,statisTime.month,statisTime.day,statisTime.hour,statisTime.minute,statisTime.second);
    }
    
    readTime = statisTime;
    if (readMeterData(lastLastCopyPara , pn, LAST_LAST_REAL_DATA, PARA_VARIABLE_DATA, &readTime, statisInterval) == TRUE)
    {
      if (debugInfo&PRINT_BALANCE_DEBUG)
      {
        printf("上一次抄表时间=%02x-%02x-%02x %02x:%02x:%02x\n",readTime.year,readTime.month,readTime.day,readTime.hour,readTime.minute,readTime.second);
      }

      //----------ERC09(电流回路异常[反向])----------
      if ((eventRecordConfig.iEvent[1]&0x01)||(eventRecordConfig.nEvent[1] & 0x01))
      {
        //电表运行状态字2
        if (pCopyParaBuff[METER_STATUS_WORD_2] != 0xEE)
        {
  		    //A相有功功率方向
  		    if (pCopyParaBuff[METER_STATUS_WORD_2]&0x01)
  		    {
            //A相反向且未记录事件,记录
	    	    if ((pStatisRecord->currentLoop&0x1)==0x00)
	    	    {
              pStatisRecord->currentLoop |= 0x01;   //置反向标志
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x01, pn, 0x03, 0x80, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,A相有功功率反向发生且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,A相有功功率反向但已记录\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //A相未发生反向,但如果曾经发生过反向,则应记录恢复事件
	    	    if ((pStatisRecord->currentLoop&0x1)==0x01)
	    	    {
              pStatisRecord->currentLoop &= 0xfe;     //清除反向标志
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x01, pn, 0x03, 0, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,A相有功功率反向恢复且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,A相正常\n",pn);
              }
	    	    }
  		    }
  		    
  		    //B相有功功率方向
  		    if (pCopyParaBuff[METER_STATUS_WORD_2]&0x02)
  		    {
            //B相反向且未记录事件,记录
	    	    if ((pStatisRecord->currentLoop&0x2)==0x00)
	    	    {
              pStatisRecord->currentLoop |= 0x02;   //置反向标志
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x02, pn, 0x03, 0x80, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,B相有功功率反向发生且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,B相有功功率反向但已记录\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //B相未发生反向,但如果曾经发生过反向,则应记录恢复事件
	    	    if ((pStatisRecord->currentLoop&0x2)==0x02)
	    	    {
              pStatisRecord->currentLoop &= 0xfd;     //清除反向标志
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x02, pn, 0x03, 0, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,B相有功功率反向恢复且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,B相正常\n",pn);
              }
	    	    }
  		    }
  		    
  		    //C相有功功率方向
  		    if (pCopyParaBuff[METER_STATUS_WORD_2]&0x04)
  		    {
            //C相反向且未记录事件,记录
	    	    if ((pStatisRecord->currentLoop&0x4)==0x00)
	    	    {
              pStatisRecord->currentLoop |= 0x04;   //置反向标志
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x04, pn, 0x03, 0x80, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,C相有功功率反向发生且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,C相有功功率反向但已记录\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //C相未发生反向,但如果曾经发生过反向,则应记录恢复事件
	    	    if ((pStatisRecord->currentLoop&0x4)==0x04)
	    	    {
              pStatisRecord->currentLoop &= 0xfb;     //清除反向标志
              cLoopEvent(pCopyParaBuff, pCopyEnergyBuff, 0x04, pn, 0x03, 0, statisTime);

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,C相有功功率反向恢复且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电流回路异常:测量点%d,C相正常\n",pn);
              }
	    	    }
  		    }
        }
      }
      
      //----------ERC11(相序异常)----------
      if ((eventRecordConfig.iEvent[1]&0x04)||(eventRecordConfig.nEvent[1] & 0x04))
      {
        //电表运行状态字2
        if (pCopyParaBuff[METER_STATUS_WORD_7] != 0xEE)
        {
  		    eventData[9] = 0x0;
  		    
  		    //相序异常?
  		    if ((pCopyParaBuff[METER_STATUS_WORD_7]&0x01) || (pCopyParaBuff[METER_STATUS_WORD_7]&0x02))
  		    {
            //电压逆相序或电流逆相序且未记录事件,记录
	    	    if ((pStatisRecord->mixed&0x4)==0x00)
	    	    {
              pStatisRecord->mixed |= 0x04;         //置相序异常标志
              eventData[9] = 0x1;

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("相序异常:测量点%d,相序异常发生且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("相序异常:测量点%d相序异常但已记录\n",pn);
              }
	    	    }
  		    }
  		    else
  		    {
            //电压逆相序或电流逆相序,但如果曾经发生过反向,则应记录恢复事件
	    	    if ((pStatisRecord->mixed&0x4)==0x04)
	    	    {
              pStatisRecord->mixed &= 0xfb;   //清除相序异常标志

              eventData[9] = 0x2;

              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("相序异常:测量点%d,相序异常恢复且未记录,记录事件\n",pn);
              }
	    	    }
	    	    else
	    	    {
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("相序异常:测量点%d,相序正常\n",pn);
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
            
      //----------ERC13(电能表故障信息)----------
      if ((eventRecordConfig.iEvent[1]&0x10)||(eventRecordConfig.nEvent[1]&0x10))
      {
        eventData[9] = 0x00;
        
        //断相次数变化
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	printf("判断电能表故障:本次断相次数=%02x%02x%02x\n",pCopyParaBuff[PHASE_DOWN_TIMES+2],pCopyParaBuff[PHASE_DOWN_TIMES+1],pCopyParaBuff[PHASE_DOWN_TIMES]);
        	printf("判断电能表故障:上次断相次数=%02x%02x%02x\n",lastLastCopyPara[PHASE_DOWN_TIMES+2],lastLastCopyPara[PHASE_DOWN_TIMES+1],lastLastCopyPara[PHASE_DOWN_TIMES]);
        }
        if ((pCopyParaBuff[PHASE_DOWN_TIMES] != lastLastCopyPara[PHASE_DOWN_TIMES] || pCopyParaBuff[PHASE_DOWN_TIMES+1] != lastLastCopyPara[PHASE_DOWN_TIMES+1] || pCopyParaBuff[PHASE_DOWN_TIMES+2] != lastLastCopyPara[PHASE_DOWN_TIMES+2])
       	  && pCopyParaBuff[PHASE_DOWN_TIMES] !=0xEE && lastLastCopyPara[PHASE_DOWN_TIMES] != 0xEE)
        {
        	eventData[9] |= 0x02;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("电能表故障:测量点%d断相次数变化\n",pn);
          }
        }
                
        //最大需量清零次数变化
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	printf("判断电能表故障:本次最大需量清零次数=%02x%02x%02x\n",pCopyParaBuff[UPDATA_REQ_TIME+2],pCopyParaBuff[UPDATA_REQ_TIME+1],pCopyParaBuff[UPDATA_REQ_TIME]);
        	printf("判断电能表故障:上次最大需量清零次数=%02x%02x%02x\n",lastLastCopyPara[UPDATA_REQ_TIME+2],lastLastCopyPara[UPDATA_REQ_TIME+1],lastLastCopyPara[UPDATA_REQ_TIME]);
        }
        if ((pCopyParaBuff[UPDATA_REQ_TIME] != lastLastCopyPara[UPDATA_REQ_TIME] || pCopyParaBuff[UPDATA_REQ_TIME+1] != lastLastCopyPara[UPDATA_REQ_TIME+1]|| pCopyParaBuff[UPDATA_REQ_TIME+2] != lastLastCopyPara[UPDATA_REQ_TIME+2])
      	    && pCopyParaBuff[UPDATA_REQ_TIME] != 0xEE && lastLastCopyPara[UPDATA_REQ_TIME] != 0xEE)
      	{
      	  eventData[9] |= 0x01;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("电能表故障:测量点%d最大需量清零次数变化\n",pn);
          }
      	}
      	
      	//编程次数变化
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	printf("判断电能表故障:本次编程次数=%02x%02x%02x\n",pCopyParaBuff[PROGRAM_TIMES+2],pCopyParaBuff[PROGRAM_TIMES+1],pCopyParaBuff[PROGRAM_TIMES]);
        	printf("判断电能表故障:上次编程次数=%02x%02x%02x\n",lastLastCopyPara[PROGRAM_TIMES+2],lastLastCopyPara[PROGRAM_TIMES+1],lastLastCopyPara[PROGRAM_TIMES]);
        }
      	if ((pCopyParaBuff[PROGRAM_TIMES] != lastLastCopyPara[PROGRAM_TIMES] || pCopyParaBuff[PROGRAM_TIMES+1] != lastLastCopyPara[PROGRAM_TIMES+1]|| pCopyParaBuff[PROGRAM_TIMES+2] != lastLastCopyPara[PROGRAM_TIMES+2])
          	&& pCopyParaBuff[PROGRAM_TIMES] != 0xEE && lastLastCopyPara[PROGRAM_TIMES] != 0xEE)
        {
          eventData[9] |= 0x01;
          
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
            printf("电能表故障:测量点%d编程次数变化\n",pn);
          }
        }
        
        //记录电表故障信息(ERC13)
        //有事件发生，记录电能表参数变更事件
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
      	  eventData[10] = eventData[9];   //ly,2011-08-03,交换这两句后在台体测试通过,这本身是个错误
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
      	
      	
        //电池欠压
        //ly,2011-10-18,添加注释,发现测试台输出的电表状态字就是0xee
        //ly,2011-10-20,发现交采测试点的欠压这个字节老是0xee,要影响编程次数等,现添加断相次数不等于ee来区分
        //ly,2012-02-04,发现用单相07表的断相次数不为0xee,但是电表状态字也可能是0xee,所以用断相次数来判断也不合适了
        //              v1.63及以前的版本对07单相表都判断为电池欠压,这是个错误
        //       修改为电表状态字第1个字节的Bit7,Bit6为保留字,如果这两位是0的话,说明是正常的电表状态字,因为0xee明显不符合这个条件
        //if (pCopyEnergyBuff[PHASE_DOWN_TIMES] != 0xEE)
        //{
        if (debugInfo&PRINT_BALANCE_DEBUG)
        {
        	 printf("判断电能表故障:电表状态字=%02x\n",pCopyParaBuff[METER_STATUS_WORD]);
        }

      	eventData[9] = 0x0;
  		  if ((pCopyParaBuff[METER_STATUS_WORD]&0xc0)==0x00)
  		  {
  		    if (pCopyParaBuff[METER_STATUS_WORD]&0x04)
  		    {
	    	    //2013-11-21,添加这个判断
	    	    if (0x0==(pStatisRecord->mixed&0x08))
	    	    {
	    	    	pStatisRecord->mixed |= 0x08;
	    	      
	    	      eventData[9] |= 0x1;
            
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电能表故障:测量点%d电压欠压\n",pn);
              }
            }
  		    }
  		    else    //添加欠压恢复
  		    {
	    	    if (pStatisRecord->mixed&0x08)
	    	    {
	    	    	pStatisRecord->mixed &= 0xf7;

	    	      eventData[9] |= 0x2;
            
              if (debugInfo&PRINT_BALANCE_DEBUG)
              {
                printf("电能表故障:测量点%d电压欠压恢复\n",pn);
              }
            }
  		    }
        }
        
        //记录电表故障信息(ERC13)-电池欠压
        //有事件发生，记录电能表参数变更事件
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
      	  if (eventData[9]==0x1)    //发生
      	  {
      	    eventData[9] = (pn>>8&0xff) | 0x80;
      	  }
      	  else                      //恢复
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
      	  
          //电表常数变更
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("判断电能表参数变更:本次电表常数=%02x%02x%02x\n",pCopyParaBuff[CONSTANT_WORK+2],pCopyParaBuff[CONSTANT_WORK+1],pCopyParaBuff[CONSTANT_WORK]);
        	  printf("判断电能表参数变更:上次电表常数=%02x%02x%02x\n",lastLastCopyPara[CONSTANT_WORK+2],lastLastCopyPara[CONSTANT_WORK+1],lastLastCopyPara[CONSTANT_WORK]);
          }
          if ((pCopyParaBuff[CONSTANT_WORK] != lastLastCopyPara[CONSTANT_WORK] || pCopyParaBuff[CONSTANT_WORK+1] != lastLastCopyPara[CONSTANT_WORK+1] || pCopyParaBuff[CONSTANT_WORK+2] != lastLastCopyPara[CONSTANT_WORK+2])
          	  && (pCopyParaBuff[CONSTANT_WORK] != 0xEE && lastLastCopyPara[CONSTANT_WORK] != 0xEE))
           //||((readBuff[CONSTANT_NO_WORK] != lastCopyParaData[CONSTANT_NO_WORK] || readBuff[CONSTANT_NO_WORK+1] != lastCopyParaData[CONSTANT_NO_WORK+1] || readBuff[CONSTANT_NO_WORK+2] != lastCopyParaData[CONSTANT_NO_WORK+2])
           //	  && (readBuff[CONSTANT_NO_WORK] != 0xEE && lastCopyParaData[CONSTANT_NO_WORK] != 0xEE)))
          {
             //记录电表常数变更事件
             eventData[9] |= 0x08;  //D3
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("电能表参数变更:测量点%d电表常数变更\n",pn);
             }
          }
          
          //抄表日变更事件
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("判断电能表参数变更:本次抄表日=%02x%02x\n",pCopyParaBuff[AUTO_COPY_DAY+1],pCopyParaBuff[AUTO_COPY_DAY]);
        	  printf("判断电能表参数变更:上次抄表日=%02x%02x\n",lastLastCopyPara[AUTO_COPY_DAY+1],lastLastCopyPara[AUTO_COPY_DAY]);
          }
          if ((pCopyParaBuff[AUTO_COPY_DAY] != lastLastCopyPara[AUTO_COPY_DAY] || pCopyParaBuff[AUTO_COPY_DAY+1] != lastLastCopyPara[AUTO_COPY_DAY+1])
          	&& pCopyParaBuff[AUTO_COPY_DAY] != 0xEE && lastLastCopyPara[AUTO_COPY_DAY] != 0xEE)
          {
             //记录抄表日变更事件
             eventData[9] |= 0x04;  //D2
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("电能表参数变更:测量点%d抄表日变更\n",pn);
             }
          }
          
          //编程时间改变
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("判断电能表参数变更:本次编程时间=%02x%02x%02x%02x\n",pCopyParaBuff[LAST_PROGRAM_TIME+3],pCopyParaBuff[LAST_PROGRAM_TIME+2],pCopyParaBuff[LAST_PROGRAM_TIME+1],pCopyParaBuff[LAST_PROGRAM_TIME]);
        	  printf("判断电能表参数变更:上次编程时间=%02x%02x%02x%02x\n",lastLastCopyPara[LAST_PROGRAM_TIME+3],lastLastCopyPara[LAST_PROGRAM_TIME+2],lastLastCopyPara[LAST_PROGRAM_TIME+1],lastLastCopyPara[LAST_PROGRAM_TIME]);
          }
          if ((pCopyParaBuff[LAST_PROGRAM_TIME] != lastLastCopyPara[LAST_PROGRAM_TIME] || pCopyParaBuff[LAST_PROGRAM_TIME+1] != lastLastCopyPara[LAST_PROGRAM_TIME+1]
          	 || pCopyParaBuff[LAST_PROGRAM_TIME+2] != lastLastCopyPara[LAST_PROGRAM_TIME+2] || pCopyParaBuff[LAST_PROGRAM_TIME+3] != lastLastCopyPara[LAST_PROGRAM_TIME+3])
          	&& pCopyParaBuff[LAST_PROGRAM_TIME] != 0xEE && lastLastCopyPara[LAST_PROGRAM_TIME] != 0xEE)
          {
             //记录编程时间改变事件
             eventData[9] |= 0x02;
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("电能表参数变更:测量点%d编程时间变更\n",pn);
             }
          }
          
          //最大需量清零
          if (debugInfo&PRINT_BALANCE_DEBUG)
          {
        	  printf("判断电能表参数变更:本次需量清零=%02x%02x%02x\n",pCopyParaBuff[UPDATA_REQ_TIME+2],pCopyParaBuff[UPDATA_REQ_TIME+1],pCopyParaBuff[UPDATA_REQ_TIME]);
        	  printf("判断电能表参数变更:上次需量清零=%02x%02x%02x\n",lastLastCopyPara[UPDATA_REQ_TIME+2],lastLastCopyPara[UPDATA_REQ_TIME+1],lastLastCopyPara[UPDATA_REQ_TIME]);
          }
          if ((pCopyParaBuff[UPDATA_REQ_TIME] != lastLastCopyPara[UPDATA_REQ_TIME] || pCopyParaBuff[UPDATA_REQ_TIME+1] != lastLastCopyPara[UPDATA_REQ_TIME+1]|| pCopyParaBuff[UPDATA_REQ_TIME+2] != lastLastCopyPara[UPDATA_REQ_TIME+2])
      	    && pCopyParaBuff[UPDATA_REQ_TIME] != 0xEE && lastLastCopyPara[UPDATA_REQ_TIME] != 0xEE)
          {
             //记录最大需量清零
             eventData[9] |= 0x20;
             
             if (debugInfo&PRINT_BALANCE_DEBUG)
             {
               printf("电能表参数变更:测量点%d最大需量清零变化\n",pn);
             }
          }
          
          //读出本次抄表的时段
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
                       printf("电能表参数变更:测量点%d抄表时段变更\n",pn);
                      }
        	  	 	      break;
        	  	 	   }
        	  	 	}
        	  	}
        	  }
          }

          //有事件发生，记录电能表参数变更事件(ERC8)
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
          
          //记录电能表运行状态字变位事件记录(ERC33)
          //有事件发生，记录
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
函数名称:
功能描述:
调用函数:     
被调用函数:
输入参数:
输出参数:
返回值:
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
函数名称: calcResumeLimit
功能描述: 计算恢复限值
调用函数:     
被调用函数:
输入参数:

输出参数:
返回值： 
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
函数名称: processOverLimit
功能描述: 处理越限
调用函数:     
被调用函数:
输入参数:

输出参数:
返回值： 
*******************************************************/
void processOverLimit(INT8U port)
{
  struct cpAddrLink        *cpLinkHead, *tmpNode;
  MEASUREPOINT_LIMIT_PARA  *pMpLimitValue;                              //测量点限值参数指针
  METER_STATIS_EXTRAN_TIME meterStatisRecord, bakStatisRec;             //一块电表统计事件数据
  INT8U                    lastCopyEnergyData[LENGTH_OF_ENERGY_RECORD]; //上一次抄表电能量数据
  INT8U                    lastCopyParaData[LENGTH_OF_PARA_RECORD];     //上一次抄表参量,参变量数据
  INT8U                    balanceParaData[LEN_OF_PARA_BALANCE_RECORD]; //结算用参变量数据
  DATE_TIME                tmpTime;

 	if ((cpLinkHead=initPortMeterLink(port))!=NULL)
 	{
     //为测量点限值分配存储空间
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
         	 	  printf("processOverLimit-处理越限持续时间到期:测量点%d设置有限值参数\n",tmpNode->mp);
         	 }
         	 	  
           //读出电表统计记录
           tmpTime = timeBcdToHex(copyCtrl[port].lastCopyTime);
           searchMpStatis(tmpTime, &meterStatisRecord, tmpNode->mp, 1);  //与时间无关量
           bakStatisRec = meterStatisRecord;
           
           if (meterStatisRecord.vUpUpTime[0].year!=0xff || meterStatisRecord.vUpUpTime[1].year!=0xff ||  meterStatisRecord.vUpUpTime[2].year!=0xff   
               || meterStatisRecord.vDownDownTime[0].year!=0xff || meterStatisRecord.vDownDownTime[1].year!=0xff ||  meterStatisRecord.vDownDownTime[2].year!=0xff
                || meterStatisRecord.cUpTime[0].year!=0xff || meterStatisRecord.cUpTime[1].year!=0xff ||  meterStatisRecord.cUpTime[2].year!=0xff 
                 || meterStatisRecord.cUpUpTime[0].year!=0xff || meterStatisRecord.cUpUpTime[1].year!=0xff ||  meterStatisRecord.cUpUpTime[2].year!=0xff
                  || meterStatisRecord.apparentUpTime.year!=0xff ||  meterStatisRecord.apparentUpUpTime.year!=0xff
                   || meterStatisRecord.vUnBalanceTime.year!=0xff || meterStatisRecord.cUnBalanceTime.year!=0xff
              )
           {
             //读出上次抄表数据
             //电能量数据
             tmpTime = copyCtrl[port].lastCopyTime;
             if (readMeterData(lastCopyEnergyData, tmpNode->mp, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0) == FALSE)
             {
             	 ;  //置无电能量数据标志
             }

             //参量及参变量
             tmpTime = copyCtrl[port].lastCopyTime;
             if (readMeterData(lastCopyParaData, tmpNode->mp, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0) == FALSE)
             {
             	 ;  //置无变量参变量数据标志
             }
             
             //测量点数据统计
             //读取前一次实时结算参变量统计数据,结合本次抄表数据生成新的统计数据
             tmpTime = copyCtrl[port].lastCopyTime;
             if (readMeterData(balanceParaData, tmpNode->mp, LAST_REAL_BALANCE, REAL_BALANCE_PARA_DATA, &tmpTime, 0) == TRUE)
             {
               //新一次统计之前将数据缓存清零(置为0xee)
               //if (balanceParaData[NEXT_NEW_INSTANCE] == START_NEW_INSTANCE)
               //{
               //  memset(balanceParaData,0xee,LEN_OF_PARA_BALANCE_RECORD-1);
               //}
             }
             else         //开始新的统计记录
             {
               memset(balanceParaData,0xee,LEN_OF_PARA_BALANCE_RECORD);
               
               //没有统计历史,本次统计等同于重新统计
               balanceParaData[NEXT_NEW_INSTANCE] = START_NEW_INSTANCE;
             }
             
             if (meterStatisRecord.vUpUpTime[0].year!=0xff || meterStatisRecord.vUpUpTime[1].year!=0xff ||  meterStatisRecord.vUpUpTime[2].year!=0xff 
                 || meterStatisRecord.vDownDownTime[0].year!=0xff || meterStatisRecord.vDownDownTime[1].year!=0xff ||  meterStatisRecord.vDownDownTime[2].year!=0xff
                )
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:电压越限在持续\n");
               }

               //电压统计
               statisticVoltage(tmpNode->mp, lastCopyParaData, lastCopyEnergyData, balanceParaData, &meterStatisRecord, pMpLimitValue,2, copyCtrl[port].lastCopyTime);
             }

             if (meterStatisRecord.apparentUpTime.year!=0xff ||  meterStatisRecord.apparentUpUpTime.year!=0xff)
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:视在功率越限在持续\n");
               }
               
               //视在功率越限累计及功率因数分段累计
               statisticApparentPowerAndFactor(tmpNode->mp, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, 2, copyCtrl[port].lastCopyTime);
             }
       
             if (meterStatisRecord.cUpTime[0].year!=0xff || meterStatisRecord.cUpTime[1].year!=0xff ||  meterStatisRecord.cUpTime[2].year!=0xff 
                  || meterStatisRecord.cUpUpTime[0].year!=0xff || meterStatisRecord.cUpUpTime[1].year!=0xff ||  meterStatisRecord.cUpUpTime[2].year!=0xff
             	  )
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:电流越限在持续\n");
               }

             	 //电流统计
               statisticCurrent(tmpNode->mp, lastCopyParaData, balanceParaData, &meterStatisRecord, pMpLimitValue, 2, copyCtrl[port].lastCopyTime);
             }
             	
             if (meterStatisRecord.vUnBalanceTime.year!=0xff || meterStatisRecord.cUnBalanceTime.year!=0xff)
             {
               if (debugInfo&PRINT_BALANCE_DEBUG)
               {
                 printf("processOverLimit:不平衡越限在持续\n");
               }
               
             	 //不平衡度越限累计
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
                 printf("processOverLimit:存储测量点统计数据\n");
               }
               //存储测量点统计数据
               saveMeterData(tmpNode->mp, port+1, copyCtrl[port].lastCopyTime, (INT8U *)&meterStatisRecord, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
             }
           }
         //}
 		 	 }
 		 	 
 		 	 tmpNode = tmpNode->next;
 		 }
     free(pMpLimitValue);   //释放测量点限值指针
     
     //释放链表
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
函数名称: processKzqOverLimit
功能描述: 处理线路测量点越限
调用函数:     
被调用函数:
输入参数:

输出参数:
返回值： 
*******************************************************/
void processKzqOverLimit(INT8U *acParaData, INT16U pn)
{
  KZQ_STATIS_EXTRAN_TIME kzqStatisRecord;         //一块控制点(线路测量点)统计事件数据
  PN_LIMIT_PARA          *pPnLimitValue;          //控制点限值参数指针
	
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
	
	//1,为测量点限值分配存储空间
	pPnLimitValue = (PN_LIMIT_PARA *)malloc(sizeof(PN_LIMIT_PARA));
	
	//2,读取控制点对应的限值参数
	if(selectViceParameter(0x04, 52, pn, (INT8U *)pPnLimitValue, sizeof(PN_LIMIT_PARA)) == FALSE)
	{
		pPnLimitValue = NULL;
	}
	
	if (pPnLimitValue!=NULL)
	{
		//3,读出线路测量点统计记录(与时间无关量)
		searchMpStatis(sysTime, &kzqStatisRecord, pn, 4);
		
		if (debugInfo&PRINT_BALANCE_DEBUG)
		{
			printf("线路测量点%d电压越限状态值=%x\n", pn, kzqStatisRecord.vOverLimit);
			printf("线路测量点%d电流越限状态值=%x\n", pn, kzqStatisRecord.cOverLimit);
			printf("线路测量点%d功率越限状态值=%x\n", pn, kzqStatisRecord.powerLimit);
			printf("线路测量点%d功率因数越限值=%x\n", pn, kzqStatisRecord.factorLimit);
		}
		
		if ((eventRecordConfig.iEvent[2] & 0x80) || (eventRecordConfig.nEvent[2] & 0x80))
		{
		  //4,电压越限判断
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("线路测量点电压统计:电压过压限值=%x\n", pPnLimitValue->vSuperiodLimit);
				printf("线路测量点电压统计:电压欠压限值=%x\n", pPnLimitValue->vDownDownLimit);
			}
			bitShift = 0x01;
			offset = VOLTAGE_PHASE_A;
			for(phase=1; phase<=3; phase++)
			{
				voltage = acParaData[offset] | acParaData[offset+1]<<8;
				
				//20V作为断相门限
				if (voltage>0x200)
				{
					if (debugInfo&PRINT_BALANCE_DEBUG)
					{
						printf("线路测量点电压统计:%d相电压值=%x\n", phase, voltage);
					}
					
					//越上上限
					if (voltage >= pPnLimitValue->vSuperiodLimit)
					{
						//第一次发生越上上限,记录越限事件
						if ((kzqStatisRecord.vOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.vUpUpTime[phase-1].year==0xff)
							{
								kzqStatisRecord.vUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("线路测量点电压统计:越上上限开始发生,%d分钟后记录\n", pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.vUpUpTime[phase-1], sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("线路测量点电压统计:电压越上上限发生持续时间已到\n");
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
							//曾经发生越上上限,记录越上上限恢复
							if (kzqStatisRecord.vOverLimit&bitShift)
							{
								if (kzqStatisRecord.vUpUpTime[phase-1].year==0xff)
								{
									kzqStatisRecord.vUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									printf("线路测量点电压统计:越上上限开始恢复,%d分钟后记录\n",pPnLimitValue->overContinued);
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.vUpUpTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("线路测量点电压统计:越上上限恢复持续时间已到\n");
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
					
					//越下下限
					bitShift <<= 1;
					if (voltage <= pPnLimitValue->vDownDownLimit)   //越下下限
					{
						//第一次发生越限,记录越限事件
						if ((kzqStatisRecord.vOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.vDownDownTime[phase-1].year==0xff)
							{
								kzqStatisRecord.vDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
								 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("线路测量点电压统计:电压越下下限发生,%d分钟后记录\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.vDownDownTime[phase-1],sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("线路测量点电压统计:电压越下下限发生持续时间已到\n");
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
						//曾经发生越下下限,记录越限恢复事件
						if (voltage>pPnLimitValue->vDownDownLimit)
						{
							if (kzqStatisRecord.vOverLimit&bitShift)
							{
								if (kzqStatisRecord.vDownDownTime[phase-1].year==0xff)
								{
									kzqStatisRecord.vDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									 
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("线路测量点电压统计:电压越下下限开始恢复,%d分钟后记录\n",pPnLimitValue->overContinued);
									}
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.vDownDownTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("线路测量点电压统计:电压下下限恢复持续时间已到\n");
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
			//电流越限判断
			bitShift = 0x01;
			offset = CURRENT_PHASE_A;
      cUpUpLimit  = pPnLimitValue->cSuperiodLimit[0] | pPnLimitValue->cSuperiodLimit[1]<<8 | pPnLimitValue->cSuperiodLimit[2]<<16;
			cDownDownLimit = pPnLimitValue->cDownDownLimit[0] | pPnLimitValue->cDownDownLimit[1]<<8 | pPnLimitValue->cDownDownLimit[2]<<16;
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("线路测量点电流统计:电流过流限值=%x\n", cUpUpLimit);
				printf("线路测量点电流统计:电流欠流限值=%x\n", cDownDownLimit);
			}
			for (phase = 1; phase <= 3; phase++)
			{
				current = acParaData[offset] | acParaData[offset+1]<<8 | (acParaData[offset+2]&0x7f)<<16;
				if (debugInfo&PRINT_BALANCE_DEBUG)
				{
					printf("线路测量点电流统计:%d相电流值=%x\n", phase, current);
				}
				
				//0.1A作为启动电流,<0.1的不作越限判断处理
				if (current>0x100)
				{
					//电流越上上限
					if (current >= cUpUpLimit)  
					{
						//第一次发生越限，记录越限事件
						if ((kzqStatisRecord.cOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.cUpUpTime[phase-1].year==0xff)
							{
								kzqStatisRecord.cUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{ 
									printf("线路测量点电流统计:越上上限开始发生,%d分钟后记录\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.cUpUpTime[phase-1],sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("线路测量点电流统计:越上上限发生持续时间已到\n");
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
						//曾经有越上上限事件发生,记录越上上限恢复
						if (current<cUpUpLimit)
						{
							if ((kzqStatisRecord.cOverLimit&bitShift) == bitShift)
							{
								if (kzqStatisRecord.cUpUpTime[phase-1].year==0xff)
								{
									kzqStatisRecord.cUpUpTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									printf("线路测量点电流统计:越上上限开始恢复,%d分钟后记录\n",pPnLimitValue->overContinued);
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.cUpUpTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("线路测量点电流统计:越上上限恢复持续时间已到\n");
										}
									 
										kzqStatisRecord.cUpUpTime[phase-1].year = 0xff;
						 
										//记录越上上限恢复
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
					//电流越下下限
					if (current<cDownDownLimit)
					{
						//第一次发生越限，记录越限事件
						if ((kzqStatisRecord.cOverLimit&bitShift) == 0x00)
						{
							if (kzqStatisRecord.cDownDownTime[phase-1].year==0xff)
							{
								kzqStatisRecord.cDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{ 
									printf("线路测量点电流统计:越下下限开始发生,%d分钟后记录\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.cDownDownTime[phase-1], sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("线路测量点电流统计:越下下限发生持续时间已到\n");
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
						//曾经有越下下限事件发生,记录越下下限恢复
						if (current>cDownDownLimit)
						{
							if ((kzqStatisRecord.cOverLimit&bitShift) == bitShift)
							{
								if (kzqStatisRecord.cDownDownTime[phase-1].year==0xff)
								{
									kzqStatisRecord.cDownDownTime[phase-1] = nextTime(sysTime, pPnLimitValue->overContinued, 0);
									printf("线路测量点电流统计:越下下限开始恢复,%d分钟后记录\n", pPnLimitValue->overContinued);
								}
								else
								{
									if (compareTwoTime(kzqStatisRecord.cDownDownTime[phase-1], sysTime))
									{
										if (debugInfo&PRINT_BALANCE_DEBUG)
										{
											printf("线路测量点电流统计:越下下限恢复持续时间已到\n");
										}
									 
										kzqStatisRecord.cDownDownTime[phase-1].year = 0xff;
						 
										//记录越上上限恢复
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
					//2016-12-23,添加这几句,张家港在现场应用时发现有时候一合闸就会记录电流欠流事件
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
				printf("线路测量点当前功率值=%x\n",power);
				printf("线路测量点功率上限限值=%x\n",pUpLimit);				
				printf("线路测量点功率下限限值=%x\n",pDownLimit);
			}

			//>0.01kW才判断功率越限
			if (power >0x000100)
			{
				//功率越上限
				if (power > pUpLimit)
				{
					if ((eventRecordConfig.iEvent[3] & 0x02) || (eventRecordConfig.nEvent[3] & 0x02))
					{
						//第一次发生越上限,记录越限事件
						if ((kzqStatisRecord.powerLimit&0x01) == 0x00)
						{
							if (kzqStatisRecord.powerUpTime.year==0xff)
							{
								kzqStatisRecord.powerUpTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
								 
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("线路测量点功率统计:越上限发生,%d分钟后记录\n",pPnLimitValue->overContinued);
								}
							}
							else
							{
								if (compareTwoTime(kzqStatisRecord.powerUpTime, sysTime))
								{
									if (debugInfo&PRINT_BALANCE_DEBUG)
									{
										printf("线路测量点功率统计:越上限发生持续时间已到\n");
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
				  //越上限恢复
				  if (power<=pUpLimit)
				  {
					  //曾经发生越上限,记录越上限恢复
					  if ((kzqStatisRecord.powerLimit&0x01) == 0x01)
					  {
						  if (kzqStatisRecord.powerUpTime.year==0xff)
						  {
							  kzqStatisRecord.powerUpTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							  printf("线路测量点功率统计:越上限开始恢复,%d分钟后记录\n",pPnLimitValue->overContinued);
						  }
						  else
						  {
							  if (compareTwoTime(kzqStatisRecord.powerUpTime,sysTime))
							  {
								  if (debugInfo&PRINT_BALANCE_DEBUG)
								  {
									  printf("线路测量点功率统计:上限恢复持续时间已到\n");
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
				
				//功率越下限
				if (power < pDownLimit)
				{
					//第一次发生越下限,记录越限事件
					if ((kzqStatisRecord.powerLimit&0x10) == 0x00)
					{
						if (kzqStatisRecord.powerDownTime.year==0xff)
						{
							kzqStatisRecord.powerDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
							if (debugInfo&PRINT_BALANCE_DEBUG)
							{
								printf("线路测量点功率统计:越下限发生,%d分钟后记录\n",pPnLimitValue->overContinued);
							}
						}
						else
						{
							if (compareTwoTime(kzqStatisRecord.powerDownTime, sysTime))
							{
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("线路测量点功率统计:越下限发生持续时间已到\n");
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
				  //越下限恢复
				  if (power>=pDownLimit)
				  {
					  //曾经发生越下限,记录越下限恢复
					  if ((kzqStatisRecord.powerLimit&0x10) == 0x10)
					  {
						  if (kzqStatisRecord.powerDownTime.year==0xff)
						  {
							  kzqStatisRecord.powerDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							  printf("线路测量点功率统计:越下限开始恢复,%d分钟后记录\n", pPnLimitValue->overContinued);
						  }
						  else
						  {
							  if (compareTwoTime(kzqStatisRecord.powerDownTime, sysTime))
							  {
								  if (debugInfo&PRINT_BALANCE_DEBUG)
								  {
									  printf("线路测量点功率统计:下限恢复持续时间已到\n");
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
		
		//功率因数越限ERC36
		if ((eventRecordConfig.iEvent[4] & 0x08) || (eventRecordConfig.nEvent[4] & 0x08))
		{
			factor = acParaData[TOTAL_POWER_FACTOR] | (acParaData[TOTAL_POWER_FACTOR+1]&0x7f)<<8;
			fDownLimit = pPnLimitValue->factorDownLimit[0] | (pPnLimitValue->factorDownLimit[1]&0x7f)<<8;
			if (debugInfo&PRINT_BALANCE_DEBUG)
			{
				printf("线路测量点当前功率因数值=%x\n", factor);
				printf("线路测量点功率因数下限限值=%x\n", fDownLimit);
			}

			//>10%才判断功率越限
			if (factor >0x100)
			{
				//功率越下限
				if (factor < fDownLimit)
				{
					//第一次发生越下限,记录越限事件
					if ((kzqStatisRecord.factorLimit&0x10) == 0x00)
					{
						if (kzqStatisRecord.factorDownTime.year==0xff)
						{
							kzqStatisRecord.factorDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							 
							if (debugInfo&PRINT_BALANCE_DEBUG)
							{
								printf("线路测量点功率因数统计:越下限发生,%d分钟后记录\n",pPnLimitValue->overContinued);
							}
						}
						else
						{
							if (compareTwoTime(kzqStatisRecord.factorDownTime, sysTime))
							{
								if (debugInfo&PRINT_BALANCE_DEBUG)
								{
									printf("线路测量点功率因数统计:越下限发生持续时间已到\n");
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
				  //越下限恢复
				  if (factor>=fDownLimit)
				  {
					  //曾经发生越下限,记录越下限恢复
					  if ((kzqStatisRecord.factorLimit&0x10) == 0x10)
					  {
						  if (kzqStatisRecord.factorDownTime.year==0xff)
						  {
							  kzqStatisRecord.factorDownTime = nextTime(sysTime, pPnLimitValue->overContinued, 0);
							  printf("线路测量点功率因数统计:越下限开始恢复,%d分钟后记录\n", pPnLimitValue->overContinued);
						  }
						  else
						  {
							  if (compareTwoTime(kzqStatisRecord.factorDownTime, sysTime))
							  {
								  if (debugInfo&PRINT_BALANCE_DEBUG)
								  {
									  printf("线路测量点功率因数统计:下限恢复持续时间已到\n");
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
		
		//存储线路测量点统计数据
    saveMeterData(pn, 2, sysTime, (INT8U *)&kzqStatisRecord, STATIS_DATA, 89, sizeof(KZQ_STATIS_EXTRAN_TIME));
	}
}

/*******************************************************
函数名称:pOverLimitEvent
功能描述:功率因数越限事件
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
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
	
	eventData[dataTail++] = pn&0xff;   //测量点低8位    
	eventData[dataTail] = (pn>>8)&0xf;    
	
	if (recovery == FALSE)
	{
		eventData[dataTail] |= 0x80;
	}
	dataTail++;
	
	if (type == 0x01)  //越下限
	{
		eventData[dataTail] = 0x00;
	}
	dataTail++;
	
	//发生时的功率因数
	eventData[dataTail++] = *data;
	eventData[dataTail++] = *(data+1);
	
	//发生时的功率因数限值
	eventData[dataTail++] = pLimit->factorDownLimit[0];
	eventData[dataTail++] = pLimit->factorDownLimit[1];
	
	eventData[1] = dataTail;

	if (eventRecordConfig.iEvent[4] & 0x08)
	{
		 writeEvent(eventData, dataTail, 1, DATA_FROM_GPRS);  //记入重要事件队列
	}
	if (eventRecordConfig.nEvent[4] & 0x08)
	{
		 writeEvent(eventData, dataTail, 2, DATA_FROM_LOCAL);  //记入一般事件队列
	}
	
	eventStatus[4] = eventStatus[4] | 0x08;
}

#endif
