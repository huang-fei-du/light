/*************************************************
Copyright,2007,Huawei WoDian co.,LTD
文件名：att7022b.h
作者：leiyong
版本：0.9
完成日期:2010年2月 日
描述：电力终端交流采样头文件
修改历史：
  01,10-02-27,leiyong created.
**************************************************/

#ifndef __att7022bH
#define __att7022bH

#include "common.h"
#include "workWithMeter.h"

#define   TOTAL_COMMAND_REAL_AC   50    //交采采集命令条数,0x3e和0x5f为ATT7022B校表数据校验和1和2
//#define   TOTAL_COMMAND_REAL_AC   34    //交采采集命令条数,0x3e和0x5f为ATT7022B校表数据校验和1和2

#define   READ_SIG_OUT_VALUE      0x01  //读取芯片输入信号
#define   READ_HAS_AC_SAMPLE      0x02  //读取是否有交采模块
#define   RESET_ATT7022B          0x03  //复位交采芯片
#define   SET_AC_MODE             0x04  //设置交采模块模式

#define   MEASURE_V_ANGLE               //使能电压夹角测量功能

#define   SIZE_OF_ENERGY_VISION    260  //电能示值存储大小
#define   POS_EPT                    0  //正向有功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   NEG_EPT                   25  //反向有功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   POS_EQT                   50  //正向无功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   NEG_EQT                   75  //反向无功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   QUA_1_EQT                100  //一象限无功无功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   QUA_4_EQT                125  //四象限无功无功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   QUA_2_EQT                150  //二象限无功无功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5
#define   QUA_3_EQT                175  //三象限无功无功总及4个费率整数(整数3Bytes+小数脉冲个数2Bytes) 5*5

#define   POS_EPA                  200  //A相正向有功(整数3Bytes+小数脉冲个数2Bytes)
#define   NEG_EPA                  205  //A相反向有功(整数3Bytes+小数脉冲个数2Bytes)
#define   POS_EQA                  210  //A相正向无功(整数3Bytes+小数脉冲个数2Bytes)
#define   NEG_EQA                  215  //A相反向无功(整数3Bytes+小数脉冲个数2Bytes)
#define   POS_EPB                  220  //B相正向有功(整数3Bytes+小数脉冲个数2Bytes)
#define   NEG_EPB                  225  //B相反向有功(整数3Bytes+小数脉冲个数2Bytes)
#define   POS_EQB                  230  //B相正向无功(整数3Bytes+小数脉冲个数2Bytes)
#define   NEG_EQB                  235  //B相反向无功(整数3Bytes+小数脉冲个数2Bytes)
#define   POS_EPC                  240  //C相正向有功(整数3Bytes+小数脉冲个数2Bytes)
#define   NEG_EPC                  245  //C相反向有功(整数3Bytes+小数脉冲个数2Bytes)
#define   POS_EQC                  250  //C相正向无功(整数3Bytes+小数脉冲个数2Bytes)
#define   NEG_EQC                  255  //C相反向无功(整数3Bytes+小数脉冲个数2Bytes)

#define   AC_SIZE_OF_REQ_TIME      260  //电能示值存储大小


//外部变量
extern INT8U  readCheckData;                      //读取校表参数
extern INT32U realAcData[TOTAL_COMMAND_REAL_AC];  //交流采样实时数据
extern BOOL   ifHasAcModule;                      //是否有交采模块
extern INT8U  acReqTimeBuf[LENGTH_OF_REQ_RECORD];

//函数声明
void readAcChipData(void);
void sspCheckMeter(void);
void resetAtt7022b(BOOL ifCheckMeter);

#endif   //__att7022bH