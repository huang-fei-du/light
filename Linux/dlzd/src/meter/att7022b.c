/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
文件名：acSample.c
作者：leiyong
版本：0.9
完成日期：2010年2月
描述：交流采样文件
      SSP1接口与电能量采集芯片通信
函数列表：
     1.
修改历史：
  01,10-02-26,Leiyong created.
  02,10-08-04,Leiyong,增加采集相位角数据项
  03,10-09-27,鉴于v1.4版本集中器和v1.3版本III型终端电压通道采用互感器采样,因此输入通道处只有0.1V,
     按钜泉的指导,这种情况下校表时应该将ADC放大值写入后再读电压寄存器的值,已经将程序改成写先入ADC
     的值再读值.
  04,10-10-23,Leiyong,增加,
     1)写入校表参数时与ATT7022B的上一次写入数据寄存器对比.
     2)利用校表数据校验和寄存器随时保持处理器校表值与ATT7022B一致,如不一致,重新校表
  05,11-09-10,Leiyong,增加
     1)电能计量功能
     2)判断相序异常和电流反向功能     
  06,11-11-25,Leiyong修改电能计量功能错误,当读到寄存器值为0xffffff的时候,计量就错误了,现修正过来
***************************************************/

#include "hardwareConfig.h"

#include "convert.h"
#include "msSetPara.h"
#include "teRunPara.h"
#include "dataBase.h"
#include "copyMeter.h"

#include "att7022b.h"


//变量声明
BOOL   ifHasAcModule;   //是否有交采模块

#ifdef AC_SAMPLE_DEVICE

INT8U  readCheckData;   //读取校表参数
INT32U realAcData[TOTAL_COMMAND_REAL_AC];      //交流采样实时数据
INT8U  realTableAC[TOTAL_COMMAND_REAL_AC] = {
             0x01,      //A相有功功率
             0x02,      //B相有功功率
             0x03,      //C相有功功率
             0x04,      //合相有功功率
             0x05,      //A相无功功率
             0x06,      //B相无功功率
             0x07,      //C相无功功率
             0x08,      //合相无功功率
             0x0D,      //A相电压有效值
             0x0E,      //B相电压有效值
             0x0F,      //C相电压有效值
             0x10,      //A相电流有效值
             0x11,      //B相电流有效值
             0x12,      //C相电流有效值
             0x14,      //A相功率因数
             0x15,      //B相功率因数
             0x16,      //C相功率因数
             0x17,      //合相功率因数
             0x09,      //A相视在功率
             0x0a,      //A相视在功率
             0x0b,      //A相视在功率
             0x0c,      //A相视在功率
             0x18,      //A相电压与电流相角
             0x19,      //B相电压与电流相角
             0x1a,      //C相电压与电流相角
             0x1b,      //合相电压与电流相角
             0x5c,      //UaUb电压夹角
             0x5e,      //UbUc电压夹角
             0x5d,      //UaUc电压夹角
             0x3e,      //校表数据校验和1
             0x5f,      //校表数据校验和2
             0x13,      //ABC相电流矢量和的有效值 ly,2011-05-25,目前零序电流没有接互感器的情况下,用这个值代替零序电流
             0x2c,      //标志状态寄存器 ly,2011-08-04,添加相序检测功能
             0x3d,      //有功和无功功率方向寄存器 ly,2011-08-04,添加电流反向检测功能
             //0x31,      //A相有功电能Epa2
             //0x32,      //B相有功电能Epb2
             //0x33,      //C相有功电能Epc2
             //0x34,      //合相有功电能Ept2
             //0x35,      //A相无功电能Eqa2
             //0x36,      //B相无功电能Eqb2
             //0x37,      //C相无功电能Eqc2
             //0x38,      //合相无功电能Eqt2
             0x63,      //合相正向有功电能PosEpt2
             0x67,      //合相反向有功电能NegEpt2
             0x6b,      //合相正向无功电能PosEqt2
             0x6f,      //合相反向无功电能NegEqt2
             
             0x60,      //A相正向有功电能PosEpa2
             0x64,      //A相反向有功电能NegEpa2
             0x68,      //A相正向无功电能PosEqa2
             0x6c,      //A相反向无功电能NegEqa2
             0x61,      //B相正向有功电能PosEpb2
             0x65,      //B相反向有功电能NegEpb2
             0x69,      //B相正向无功电能PosEqb2
             0x6d,      //B相反向无功电能NegEqb2
             0x62,      //C相正向有功电能PosEpc2
             0x66,      //C相反向有功电能NegEpc2
             0x6a,      //C相正向无功电能PosEqc2
             0x6e,      //C相反向无功电能NegEqc2
             };

INT8U  acReqTimeBuf[LENGTH_OF_REQ_RECORD];

INT8U  nowAcItem=0;                                      //当前读取的交流采样数据项目
INT8U  tmpAcI, checkSumAc;
INT8U  acBuffer[26];
INT8U  acReadBuffer[4];
INT32U checkSum3e, checkSum5f;                           //校表数据校验和1与2
INT8U  countPhase=0;

INT8U sspWrite(INT8U cmd,INT32U data);

/***************************************************
函数名称:resetAtt7022b
功能描述:复位AT7022B且根据情况下发校表参数
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
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
   	   
   	   printf("无交采单元\n");
    }
   #endif

   if (ifHasAcModule==TRUE)
   {
     //复位ATT7022B
	   ioctl(fdOfSample,RESET_ATT7022B,0);
   
     //设置为三相四线模式
	   ioctl(fdOfSample,SET_AC_MODE,1);

     //等待ATT7022B状态正常
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
        //ADC增益
        if (sspWrite(0x3f | 0x80, 0x465501)==1)
        {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("ATT7022B校表值写入成功(ADC增益为4倍)\n");
          }
        }
        delayTime(2000);
     	 #endif

     	 #ifdef TE_CTRL_BOARD_V_1_3
        //ADC增益
        if (sspWrite(0x3f | 0x80, 0x465501)==1)
        {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("ATT7022B校表值写入成功(ADC增益为4倍)\n");
          }
        }
        delayTime(2000);
     	 #endif
     }
   }
}

/***************************************************
函数名称:decideAcPhaseOrder
功能描述:判断交采相序 事件
调用函数:
被调用函数
输入参数:
输出参数:
返回值：无
***************************************************/
void decideAcPhaseOrder(void)
{
  METER_STATIS_EXTRAN_TIME meterStatisRecord;   //一块电表统计事件数据
  INT8U                    acParaData[LENGTH_OF_PARA_RECORD];
  INT8U                    copyEnergyData[LENGTH_OF_ENERGY_RECORD];
	INT16U                   acPn;                               //交采测量点号
	DATE_TIME                tmpTime;
	INT8U                    eventData[50];
	INT8U                    saveStatisData = 0;

 	//有配置交采测量点
 	if ((acPn=findAcPn())!=0)
 	{
	  tmpTime = sysTime;
    searchMpStatis(tmpTime, &meterStatisRecord, acPn, 1);  //与时间无关量
    memset(acParaData, 0xee, LENGTH_OF_PARA_RECORD);
    memset(copyEnergyData, 0xee, LENGTH_OF_ENERGY_RECORD);
    
    covertAcSample(acParaData, copyEnergyData, NULL, 1, sysTime);

    //ERC09,电流回路异常[反向]
    if ((eventRecordConfig.iEvent[1]&0x01)||(eventRecordConfig.nEvent[1] & 0x01))
    {
	    tmpTime = timeHexToBcd(sysTime);

	    //A相有功功率方向
	    if (realAcData[33]&0x01)
	    {
        //A相反向且未记录事件,记录
  	    if ((meterStatisRecord.currentLoop&0x1)==0x00)
  	    {
          meterStatisRecord.currentLoop |= 0x01;   //置反向标志
          cLoopEvent(acParaData, copyEnergyData, 0x01, acPn, 0x03, 0x80, tmpTime);
          saveStatisData = 1;
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,A相有功功率反向发生且未记录,记录事件\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,A相有功功率反向但已记录\n", acPn);
          }
  	    }
	    }
	    else
	    {
        //A相未发生反向,但如果曾经发生过反向,则应记录恢复事件
  	    if ((meterStatisRecord.currentLoop&0x1)==0x01)
  	    {
          meterStatisRecord.currentLoop &= 0xfe;     //清除反向标志
          cLoopEvent(acParaData, copyEnergyData, 0x01, acPn, 0x03, 0, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,A相有功功率反向恢复且未记录,记录事件\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,A相正常\n",acPn);
          }
  	    }
	    }
	    
	    //B相有功功率方向
	    if (realAcData[33]&0x02)
	    {
        //B相反向且未记录事件,记录
  	    if ((meterStatisRecord.currentLoop&0x2)==0x00)
  	    {
          meterStatisRecord.currentLoop |= 0x02;   //置反向标志
          cLoopEvent(acParaData, copyEnergyData, 0x02, acPn, 0x03, 0x80, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,B相有功功率反向发生且未记录,记录事件\n",acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,B相有功功率反向但已记录\n",acPn);
          }
  	    }
	    }
	    else
	    {
        //B相未发生反向,但如果曾经发生过反向,则应记录恢复事件
  	    if ((meterStatisRecord.currentLoop&0x2)==0x02)
  	    {
          meterStatisRecord.currentLoop &= 0xfd;     //清除反向标志
          cLoopEvent(acParaData, copyEnergyData, 0x02, acPn, 0x03, 0, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,B相有功功率反向恢复且未记录,记录事件\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,B相正常\n", acPn);
          }
  	    }
	    }
	    
	    //C相有功功率方向
	    if (realAcData[33]&0x04)
	    {
        //C相反向且未记录事件,记录
  	    if ((meterStatisRecord.currentLoop&0x4)==0x00)
  	    {
          meterStatisRecord.currentLoop |= 0x04;   //置反向标志
          cLoopEvent(acParaData, copyEnergyData, 0x04, acPn, 0x03, 0x80, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,C相有功功率反向发生且未记录,记录事件\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,C相有功功率反向但已记录\n", acPn);
          }
  	    }
	    }
	    else
	    {
        //C相未发生反向,但如果曾经发生过反向,则应记录恢复事件
  	    if ((meterStatisRecord.currentLoop&0x4)==0x04)
  	    {
          meterStatisRecord.currentLoop &= 0xfb;     //清除反向标志
          cLoopEvent(acParaData, copyEnergyData, 0x04, acPn, 0x03, 0, tmpTime);
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,C相有功功率反向恢复且未记录,记录事件\n", acPn);
          }
  	    }
  	    else
  	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("电流回路异常:交采测量点%d,C相正常\n", acPn);
          }
  	    }
	    }
    }

    //ERC11,相序异常
    if ((eventRecordConfig.iEvent[1]&0x04)||(eventRecordConfig.nEvent[1] & 0x04))
    {
   	  //如果电压相序错或电流相序错
      if ((realAcData[32]&0x08) || (realAcData[32]&0x10))
      {
         //电压逆相序或电流逆相序且未记录事件,记录
   	    if ((meterStatisRecord.mixed&0x4)==0x00)
   	    {
          meterStatisRecord.mixed |= 0x04;         //置相序异常标志
          eventData[9] = 0x1;
          saveStatisData = 1;

          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("相序异常:交采测量点%d,相序异常发生且未记录,记录事件\n", acPn);
          }
   	    }
   	    else
   	    {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
            printf("相序异常:交采测量点%d相序异常但已记录\n", acPn);
          }
   	    }
      }
      else
      {
         //电压逆相序或电流逆相序,但如果曾经发生过反向,则应记录恢复事件
   	    if ((meterStatisRecord.mixed&0x4)==0x04)
   	    {
           meterStatisRecord.mixed &= 0xfb;   //清除相序异常标志
  
           eventData[9] = 0x2;
           saveStatisData = 1;

           if (debugInfo&PRINT_AC_SAMPLE)
           {
             printf("相序异常:交采测量点%d,相序异常恢复且未记录,记录事件\n", acPn);
           }
   	    }
   	    else
   	    {
           if (debugInfo&PRINT_AC_SAMPLE)
           {
             printf("相序异常:交采测量点%d,相序正常\n", acPn);
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
		    
		    //A相电压相位角
		    eventData[10] = acParaData[PHASE_ANGLE_V_A];
		    eventData[11] = acParaData[PHASE_ANGLE_V_A+1];

		    //B相电压相位角
		    eventData[12] = acParaData[PHASE_ANGLE_V_B];
		    eventData[13] = acParaData[PHASE_ANGLE_V_B+1];
		       
		    //C相电压相位角
		    eventData[14] = acParaData[PHASE_ANGLE_V_C];
		    eventData[15] = acParaData[PHASE_ANGLE_V_C+1];
		     
		    //A相电流相位角
		    eventData[16] = acParaData[PHASE_ANGLE_C_A];
		    eventData[17] = acParaData[PHASE_ANGLE_C_A+1];

		    //B相电流相位角
		    eventData[18] = acParaData[PHASE_ANGLE_C_B];
		    eventData[19] = acParaData[PHASE_ANGLE_C_B+1];
		       
		    //C相电流相位角
		    eventData[20] = acParaData[PHASE_ANGLE_C_C];
		    eventData[21] = acParaData[PHASE_ANGLE_C_C+1];

        //发生时的电能表正向有功总电能示值,交采测量点未记
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
     	//存储交采测量点统计数据
      saveMeterData(acPn, 1, sysTime, (INT8U *)&meterStatisRecord, STATIS_DATA, 88,sizeof(METER_STATIS_EXTRAN_TIME));
    }
 	}
 	else
 	{
 		if (debugInfo&PRINT_AC_SAMPLE)
 		{
 			 printf("判断交采相序 - 未配置交采测量点\n");
 		}
 	}
}

/***************************************************
函数名称:decideQuadrant
功能描述:判断在哪个象限
调用函数:
被调用函数
输入参数:
输出参数:
返回值：象限值(1,2,3,4)
***************************************************/
INT8U decideQuadrant(void)
{
  INT8U tmpQuadrant=0;
  
  if (realAcData[3]>>23 & 0x1)  //有功功率方向为负
  {
    if (realAcData[7]>>23 & 0x1)//无功功率方向为负
    {
  	  tmpQuadrant = 3;
    }
    else                        //无功功率方向为正
    {
  	  tmpQuadrant = 2;
    }
  }
  else                          //有功功率方向为正
  {
    if (realAcData[7]>>23 & 0x1)//无功功率方向为负
    {
  	  tmpQuadrant = 4;
    }
    else                        //无功功率方向为正
    {
  	  tmpQuadrant = 1;
    }
  }
  
  //bug,2011-11-25,加上return,原来未加,导致象限的数据统计不到
  return tmpQuadrant;
}

/***************************************************
函数名称:updateVision
功能描述:更新某个示值
调用函数:
被调用函数
输入参数:vBuf    - 缓存
         offset - 偏移量
         added  - 增加量
输出参数:
返回值：无
***************************************************/
void updateVision(INT8U *vBuf, INT16U offset, INT32U added)
{
	 INT32U visionInt;
	 INT16U visionDec;   
   
   visionInt = vBuf[offset+0] | vBuf[offset+1]<<8 | vBuf[offset+2]<<16;
   visionDec = vBuf[offset+3] | vBuf[offset+4]<<8;
   
   if (debugInfo&PRINT_AC_VISION)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d:更新前,visionInt=%d,visionDec=%d,本次增加值=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, visionInt, visionDec, added);
   }
        
   visionDec += added;
   visionInt += visionDec/3200;
   visionDec %= 3200;
    	  
   //整数
   vBuf[offset+0] = visionInt&0xff;
   vBuf[offset+1] = visionInt>>8&0xff;
   vBuf[offset+2] = visionInt>>16&0xff;
   
   //小数
   vBuf[offset+3] = visionDec&0xff;
   vBuf[offset+4] = visionDec>>8&0xff;

   if (debugInfo&PRINT_AC_VISION)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d:更新后, visionInt=%d, visionDec=%d,偏移=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, visionInt, visionDec, offset);
   }
}

/***************************************************
函数名称:recordVisionReq
功能描述:记录电能示值及需量
调用函数:
被调用函数
输入参数:
输出参数:
返回值：无
***************************************************/
void recordVisionReq(void)
{
	INT8U  i;
	BOOL   ifChanged=FALSE;
	INT8U  energyVision[SIZE_OF_ENERGY_VISION];
	INT8U  tariff;           //费率号
	INT8U  quadrant;
	INT16U offset;
	INT32U tmpData, tmpDataBak, tmpDataBefore;
	INT16U j;
  INT32U totalVision[4];   //总示值变化量


  //查找费率
  tariff = 0xee;   //先置为无效费率
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

  //处理有功功率
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
      //如果符号位为1,即为负数
      if (tmpData & 0x800000)
      {
        //将补码还原为原码值再进行计算
        tmpData = (~tmpData & 0x7fffff)+1;
        if (i==0)
        {
          offset = REQ_NEGTIVE_WORK_OFFSET;     //反向有功
        }
        else
        {
          offset = REQ_NEGTIVE_NO_WORK_OFFSET;  //反向无功
        }
      }
      else
      {
        if (i==0)
        {
          offset = REQ_POSITIVE_WORK_OFFSET;    //正向有功
        }
        else
        {
          offset = REQ_POSITIVE_NO_WORK_OFFSET; //正向无功      	 
        }
      }
      
      tmpData = hexDivision(tmpData,times2(6)*1000,4);
      tmpDataBak = tmpData;
      tmpData = bcdToHex(tmpData);
      
      tmpDataBefore =  bcdToHex(acReqTimeBuf[offset] | acReqTimeBuf[offset+1]<<8 | acReqTimeBuf[offset+2]<<16);
      
      //总
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
      
      //费率
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
  
  //处理示值
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
		//读取交采示值
		readAcVision(energyVision, sysTime, ENERGY_DATA);
    
    totalVision[0] = 0;  //总正向有功
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

    totalVision[1] = 0;  //正向无功
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
    
    totalVision[2] = 0;  //反向有功
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

    totalVision[3] = 0;  //反向无功
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
    	    printf("更新项=%d,偏移=%d\n", i, i*25);
    	  }
    	  
    	  //总
    	  updateVision(energyVision, i*25, totalVision[i]);
    	  
     	  //分费率,当前只处理4个费率
     	  if (tariff<4)
     	  {
      	  if (debugInfo&PRINT_AC_VISION)
      	  {
    	      printf("更新项=%d,费率=%d,偏移=%d\n", i, tariff, i*25+5+tariff*5);
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
    	        printf("更新项=%d,无功总,偏移=%d\n", i, offset);
    	      }
    	      
            updateVision(energyVision, offset, totalVision[i]);
     	      //分费率,当前只处理4个费率
     	      if (tariff<4)
     	      {
      	      if (debugInfo&PRINT_AC_VISION)
      	      {
    	          printf("更新项=%d,无功,费率=%d,偏移=%d\n", i, tariff, offset+5+tariff*5);
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
    	    printf("更新项=%d,偏移=%d\n", i, (i-34)*25);
    	  }
    	  
    	  //总
    	  updateVision(energyVision, (i-34)*25, realAcData[i]);
    	  
     	  //分费率,当前只处理4个费率
     	  if (tariff<4)
     	  {
	    	  if (debugInfo&PRINT_AC_VISION)
	    	  {
    	      printf("更新项=%d,费率=%d,偏移=%d\n", i, tariff, (i-34)*25+5+tariff*5);
    	    }
    	    
    	    updateVision(energyVision, (i-34)*25+5+tariff*5, realAcData[i]);
     	  }
     	  
     	  if (i==36 || i==37)
     	  {
          offset = 0;
          quadrant = decideQuadrant();
          if (i==36)   //正向无功
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
          if (i==37)   //反向无功
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
    	        printf("更新项=%d,无功总,偏移=%d\n", i, offset);
    	      }
    	      
            updateVision(energyVision, offset, realAcData[i]);
     	      //分费率,当前只处理4个费率
     	      if (tariff<4)
     	      {
	    	      if (debugInfo&PRINT_AC_VISION)
	    	      {
    	          printf("更新项=%d,无功,费率=%d,偏移=%d\n", i, tariff, offset+5+tariff*5);
    	        }
    	        
    	        updateVision(energyVision, offset+5+tariff*5, realAcData[i]);
     	      }
     	    }
     	  }
    	}
    }
    */
    
    //A,B,C相正向有功、正向无功、反向有功、反向无功
	  for(i=38; i<50; i++)
    {
	    if (realAcData[i]>0 && realAcData[i]!=0xffffff && realAcData[i]<200)
	    {
	    	if (debugInfo&PRINT_AC_VISION)
	    	{
    	    printf("更新项=%d,偏移=%d\n", i, POS_EPA+(i-38)*5);
    	  }
    	  
    	  updateVision(energyVision, POS_EPA+(i-38)*5, realAcData[i]);
    	}
    }
    
    //更新交采示值
    updateAcVision(energyVision,sysTime,ENERGY_DATA);
	}
}

/***************************************************
函数名称:readAcChipData
功能描述:读采样芯片数据
调用函数:
被调用函数
输入参数:
输出参数:
返回值：无
***************************************************/
void readAcChipData(void)
{   
   INT16U i;
   INT16U frameTail00, tmpHead00;

   acReadBuffer[0] = realTableAC[nowAcItem];
   read(fdOfSample,&acReadBuffer,3);
   realAcData[nowAcItem] = acReadBuffer[0]<<16 | acReadBuffer[1]<<8 | acReadBuffer[2];
   
   //专变III型终端交采硬件V1.2,由于光藕回数据的时候是反了向的,因此将数据反过来
   #ifndef PLUG_IN_CARRIER_MODULE
    #ifdef TE_AC_SAMPLE_MODE_V_1_2
     realAcData[nowAcItem] = (~realAcData[nowAcItem])&0xffffff;
    #endif
   #endif

   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("%02d-%02d-%02d %02d:%02d:%02d:交采采集序号%d,采集项:%02x,采集值:%08x\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, nowAcItem,realTableAC[nowAcItem],realAcData[nowAcItem]);
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
 	     //记录电能示值及需量
 	     recordVisionReq();
 	   
 	     countPhase++;
 	     //大约每12秒检测一次
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
函数名称:sspWrite
功能描述:SSP1写
调用函数:
被调用函数:
输入参数:8bit命令,32bit数据(高8b无效)
输出参数:
返回值：成功或失败
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
   
   //2012-07-27,缩短延迟,避免因延迟过长而应用程序启动不起
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
函数名称:writeOneCheckPara
功能描述:写入一个校表参数
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：无
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
        
        //printf("寄存器=%x,写入数据=%x,读出数据=%02x%02x%02x\n",checkReg, data,checkData[0],checkData[1],checkData[2]);

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
函数名称:sspCheckMeter
功能描述:SSP1校表
调用函数:
被调用函数
输入参数:无
输出参数:
返回值：无
***************************************************/
void sspCheckMeter(void)
{
   //ADC增益
   if (writeOneCheckPara(0x3f | 0x80,acSamplePara.UADCPga)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(ADC增益)\n");
      }
   }

   //电流相序检测使能控制,ly,2011-08-04,Add
   if (writeOneCheckPara(0x30 | 0x80, 0x5678)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(电流相序检测使能控制功能使能)\n");
      }
   }

   //高频脉冲输出设置
   if (writeOneCheckPara(0xa0,acSamplePara.HFConst)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(高频脉冲输出设置)\n");
      }
   }
   else
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入失败(高频脉冲输出设置)\n");
      }
   }
   
   //断相(失压)阈值
   if (writeOneCheckPara(0xa9,acSamplePara.FailVoltage)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(断相(失压)阈值)\n");
      }
   }
  
   //启动电流
   if (writeOneCheckPara(0x9f,acSamplePara.Istartup)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(启动电流)\n");
      }
   }   

   //能量累加模式
   if (writeOneCheckPara(0xac,acSamplePara.EAddMode)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(能量累加模式)\n");
      }
   }   
  
   //使能电压夹角测量
  #ifdef MEASURE_V_ANGLE
   if (writeOneCheckPara(0x2E | 0x80, 0x3584)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(使能电压夹角测量)\n");
      }
   }
  #endif
  
   //A相增益1
   if (writeOneCheckPara(0x89,acSamplePara.PgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相增益1)\n");
      }
   }
   
   //A相增益0
   if (writeOneCheckPara(0x86,acSamplePara.PgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相增益0)\n");
      }
   }   

   //A相相位4
   if (writeOneCheckPara(0x90,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相相位4)\n");
      }
   }   
   
   //A相相位3
   if (writeOneCheckPara(0x8f,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相相位3)\n");
      }
   }
   
   //A相相位2
   if (writeOneCheckPara(0x8e,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相相位2)\n");
      }
   }   
   
   //A相相位1
   if (writeOneCheckPara(0x8d,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相相位1)\n");
      }
   }   
   
   //A相相位0
   if (writeOneCheckPara(0x8c,acSamplePara.PhsreagA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相相位0)\n");
      }
   }   

   //A相电压校正值
   if (writeOneCheckPara(0x9b,acSamplePara.UgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相电压校正值,UgainA=%0x)\n", acSamplePara.UgainA);
      }
   }
   
   //A相电流校正值
   if (writeOneCheckPara(0xa6,acSamplePara.IgainA)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(A相电流校正值)\n");
      }
   }
   
   //B相增益1
   if (writeOneCheckPara(0x8A,acSamplePara.PgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相增益1)\n");
      }
   }   
   
   //B相增益0
   if (writeOneCheckPara(0x87,acSamplePara.PgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相增益0)\n");
      }
   }   

   //B相相位4
   if (writeOneCheckPara(0x95,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相相位4)\n");
      }
   }   
   
   //B相相位3
   if (writeOneCheckPara(0x94,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相相位3)\n");
      }
   }   
   
   //B相相位2
   if (writeOneCheckPara(0x93,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相相位2)\n");
      }
   }   
   
   //B相相位1
   if (writeOneCheckPara(0x92,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相相位1)\n");
      }
   }   

   //B相相位0
   if (writeOneCheckPara(0x91,acSamplePara.PhsreagB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相相位0)\n");
      }
   }   
   
   //B相电压校正值
   if (writeOneCheckPara(0x9c,acSamplePara.UgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相电压校正值,UgainB=%0x)\n",acSamplePara.UgainB);
      }
   }
   
   //B相电流校正值
   if (writeOneCheckPara(0xa7,acSamplePara.IgainB)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(B相电流校正值)\n");
      }
   }
   
   //C相增益1
   if (writeOneCheckPara(0x8B,acSamplePara.PgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相增益1)\n");
      }
   }   
   
   //C相增益0
   if (writeOneCheckPara(0x88,acSamplePara.PgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相增益0)\n");
      }
   }   

   //C相相位4
   if (writeOneCheckPara(0x9A,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相相位4)\n");
      }
   }   
   
   //C相相位3
   if (writeOneCheckPara(0x99,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相相位3)\n");
      }
   }   
   
   //C相相位2
   if (writeOneCheckPara(0x98,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相相位2)\n");
      }
   }   
   
   //C相相位1
   if (writeOneCheckPara(0x97,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相相位1)\n");
      }
   }   

   //C相相位0
   if (writeOneCheckPara(0x96,acSamplePara.PhsreagC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相相位0)\n");
      }
   }   
   
   //C相电压校正值
   if (writeOneCheckPara(0x9D,acSamplePara.UgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相电压校正值,UgainC=%0x)\n", acSamplePara.UgainC);
      }
   }
   
   //C相电流校正值
   if (writeOneCheckPara(0xa8,acSamplePara.IgainC)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(C相电流校正值)\n");
      }
   }

   //基波测量使能控制寄存器(置为全波表)
   if (writeOneCheckPara(0x2d, 0x000000)==1)
   {
      if (debugInfo&PRINT_AC_SAMPLE)
      {
        printf("ATT7022B校表值写入成功(基波测量使能控制寄存器,置为全波表)\n");
      }
   }
   
   usleep(800000);
   
   acReadBuffer[0] = 0x3e;
   read(fdOfSample,&acReadBuffer,3);
   checkSum3e = acReadBuffer[0]<<16 | acReadBuffer[1]<<8 | acReadBuffer[2];
   
   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("ATT7022B校表数据校验和1=0x%0x\n", checkSum3e);
   }
   
   acReadBuffer[0] = 0x5f;
   read(fdOfSample,&acReadBuffer,3);
   checkSum5f = acReadBuffer[0]<<16 | acReadBuffer[1]<<8 | acReadBuffer[2];
   if (debugInfo&PRINT_AC_SAMPLE)
   {
     printf("ATT7022B校表数据校验和2=0x%0x\n", checkSum5f);
   }
}

#endif    //AC_SAMPLE_DEVICE
