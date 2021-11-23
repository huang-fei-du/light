/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:meterProtocol.c
作者:leiyong
版本:0.9
完成日期:2010年1月
描述:电表规约库处理文件
函数列表:
  1.
修改历史:
  01,10-01-07,Leiyong created.
  02,12-01-10,Leiyong增加至6个端口同时处理,适应鼎信载波路由请求抄读时三个相位同时抄表
  03,12-05-21,Leiyong修改,甘肃普光发现集中器抄不到当地华立07表,经查是该表不支持数据块,只支持正反向有功总示值,
              修改07表既抄数据块,同时增加正反向有功总示值(包括上一结算日、当前)的抄读
  04,12-05-29,Leiyong修改,在公司测试发现,三星DDZY188C-Z单相费控表在抄读电压、电流、功率时,设备发的数据块,
              表计在回复数据时也是回复的数据块,但是只有A相的,这样设备就会错误的识别B、C相的数据,
              修正: 添加判断"i<frameAnalyse.loadLen",这样，他回几相就是几相。
***************************************************/
#include "convert.h"
#include "string.h"
#include "stdio.h"

#include "dataBase.h"
#include "meterProtocolx.h"
#include "meterProtocol.h"

//1.抄表参数
struct meterCopy
{
  INT8U     port;              //抄表端口
  INT16U    pn;                //测量点号
  INT8U     protocol;          //通信协议
  INT8U     meterAddr[6];      //表地址
  INT8U     copyDataType;      //抄表数据类型(当前数据或是上月数据)
  DATE_TIME copyTime;          //抄表时间
  INT8U     copyItem;          //当前抄表项数
  INT8U     totalCopyItem;     //总的应抄项数
  INT16U    hasData;           //是否有抄到的数据
   
  INT8U     *energy;           //电能示值及参变量缓存指针
  INT8U     *reqAndReqTime;    //需及发生时间缓存指针
  INT8U     *paraVariable;     //参量及参变量指针
  INT8U     *shiDuan;          //时段缓存时针
   
  INT8U     meterRecvBuf[2048];//接收缓存
  INT16U    recvFrameTail;     //接收帧尾
  INT16U    totalLenOfFrame;   //帧总长度
   
  void (*send)(INT8U port,INT8U *pack,INT16U length);                   //向端口发送数据函数
  void (*save)(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len);  //数据保存函数

};

//结构 - 接收数据帧分析结构
struct recvFrameStruct
{
	INT8U     C;                 //控制码
	INT8U     L;                 //数据域长度
	INT8U     DI[4];             //数据标识
	INT8U     *pData;            //净数据开始指针
	INT8U     loadLen;           //净负荷长度
};

//变量定义及赋初值
//struct meterCopy multiCopy[4];           //因为可能同时抄4个端口的表,所以定义为数组
struct meterCopy multiCopy[6];           //因为可能同时抄6个端口(鼎信为3相同时抄)的表,所以定义为数组
	
//1.DL/T645-1997规约
#ifdef  PROTOCOL_645_1997
  //DL/T645-1997上月数据与数据标识转换表  16项
  INT8U cmdDlt645LastMonth1997[TOTAL_CMD_LASTMONTH_645_1997][5] = {
	                        {0x94, 0x1f, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x94, 0x2f, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x1f, POSITIVE_NO_WORK_OFFSET&0xFF, POSITIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xFF, NEGTIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x3f, QUA1_NO_WORK_OFFSET&0xFF, QUA1_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x4f, QUA4_NO_WORK_OFFSET&0xFF, QUA4_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x5f, QUA2_NO_WORK_OFFSET&0xFF, QUA2_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x6f, QUA3_NO_WORK_OFFSET&0xFF, QUA3_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA4, 0x1f, REQ_POSITIVE_WORK_OFFSET&0xFF, REQ_POSITIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA4, 0x2f, REQ_NEGTIVE_WORK_OFFSET&0xFF, REQ_NEGTIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA5, 0x1f, REQ_POSITIVE_NO_WORK_OFFSET&0xFF, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA5, 0x2f, REQ_NEGTIVE_NO_WORK_OFFSET&0xFF, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x3f, QUA1_REQ_NO_WORK_OFFSET&0xFF, QUA1_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x4f, QUA4_REQ_NO_WORK_OFFSET&0xFF, QUA4_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x5f, QUA2_REQ_NO_WORK_OFFSET&0xFF, QUA2_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x6f, QUA3_REQ_NO_WORK_OFFSET&0xFF, QUA3_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB4, 0x1f, REQ_TIME_P_WORK_OFFSET&0xFF, REQ_TIME_P_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB4, 0x2f, REQ_TIME_N_WORK_OFFSET&0xFF, REQ_TIME_N_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB5, 0x1f, REQ_TIME_P_NO_WORK_OFFSET&0xFF, REQ_TIME_P_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB5, 0x2f, REQ_TIME_N_NO_WORK_OFFSET&0xFF, REQ_TIME_N_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x3f, QUA1_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA1_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x4f, QUA4_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA4_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x5f, QUA2_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA2_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x6f, QUA3_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA3_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1}
                          };
                          
  //DL/T645-1997电量+需量+参变量+时段参变量数据与数据标识转换表
  INT8U  cmdDlt645Current1997[TOTAL_CMD_CURRENT_645_1997][5] = {
	                        //电量及需量 26项
	                        {0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x2f, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x1f, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x3f, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x4f, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x5f, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x6f, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA0, 0x1f, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA0, 0x2f, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA1, 0x1f, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA1, 0x2f, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x3f, QUA1_REQ_NO_WORK_OFFSET&0xff, QUA1_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x4f, QUA4_REQ_NO_WORK_OFFSET&0xff, QUA4_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x5f, QUA2_REQ_NO_WORK_OFFSET&0xff, QUA2_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x6f, QUA3_REQ_NO_WORK_OFFSET&0xff, QUA3_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB0, 0x1f, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB0, 0x2f, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB1, 0x1f, REQ_TIME_P_NO_WORK_OFFSET&0xff, REQ_TIME_P_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB1, 0x2f, REQ_TIME_N_NO_WORK_OFFSET&0xff, REQ_TIME_N_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x3f, QUA1_REQ_TIME_NO_WORK_OFFSET&0xff, QUA1_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x4f, QUA4_REQ_TIME_NO_WORK_OFFSET&0xff, QUA4_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x5f, QUA2_REQ_TIME_NO_WORK_OFFSET&0xff, QUA2_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x6f, QUA3_REQ_TIME_NO_WORK_OFFSET&0xff, QUA3_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x20, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        
	                        //参变量数据
	                        {0xB6, 0x11, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x12, VOLTAGE_PHASE_B&0xFF, VOLTAGE_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x13, VOLTAGE_PHASE_C&0xFF, VOLTAGE_PHASE_C>>8&0xFF,0x1},
	                        {0xB6, 0x21, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x22, CURRENT_PHASE_B&0xFF, CURRENT_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x23, CURRENT_PHASE_C&0xFF, CURRENT_PHASE_C>>8&0xFF,0x1},
	                        {0xB6, 0x30, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x31, POWER_PHASE_A_WORK&0xFF, POWER_PHASE_A_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x32, POWER_PHASE_B_WORK&0xFF, POWER_PHASE_B_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x33, POWER_PHASE_C_WORK&0xFF, POWER_PHASE_C_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x40, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x41, POWER_PHASE_A_NO_WORK&0xFF, POWER_PHASE_A_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x42, POWER_PHASE_B_NO_WORK&0xFF, POWER_PHASE_B_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x43, POWER_PHASE_C_NO_WORK&0xFF, POWER_PHASE_C_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x50, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF,0x1},
	                        {0xB6, 0x51, FACTOR_PHASE_A&0xFF, FACTOR_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x52, FACTOR_PHASE_B&0xFF, FACTOR_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x53, FACTOR_PHASE_C&0xFF, FACTOR_PHASE_C>>8&0xFF,0x1},
	                        {0xB3, 0x10, PHASE_DOWN_TIMES&0xFF, PHASE_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x11, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x12, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x13, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x20, TOTAL_PHASE_DOWN_TIME&0xFF, TOTAL_PHASE_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x21, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x22, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x23, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x30, LAST_PHASE_DOWN_BEGIN&0xFF, LAST_PHASE_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x31, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x32, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x33, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x40, LAST_PHASE_DOWN_END&0xFF, LAST_PHASE_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x41, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x42, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x43, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF,0x1},
	                        {0xB2, 0x10, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x11, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x12, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF,0x1},
	                        {0xB2, 0x13, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x14, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF,0x1},
	                        {0xC0, 0x10, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF,0x1},
	                        {0xC0, 0x11, METER_TIME&0xFF, METER_TIME>>8&0xFF,0x1},
	                        {0xC0, 0x20, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF,0x1},
	                        {0xC0, 0x21, NET_STATUS_WORD&0xFF, NET_STATUS_WORD>>8&0xFF,0x1},
	                        {0xC0, 0x30, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF,0x1},
	                        {0xC0, 0x31, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF,0x1},
	                        {0xC0, 0x32, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF,0x1},
	                        {0xC1, 0x17, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF,0x1},
	                        //以下几行时段参变量
	                        {0xC3, 0x10, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF,0x1},
	                        {0xC3, 0x11, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF,0x1},
	                        {0xC3, 0x12, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF,0x1},
	                        {0xC3, 0x13, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF,0x1},
	                        {0xC3, 0x14, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF,0x1},
	                        {0xC3, 0x2f, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF,0x1},
	                        
	                        //以下是日时段表、起始时间及费率号
	                        {0xC3, 0x3f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x4f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x5f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x6f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x7f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x8f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x9f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0xaf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1}
	                        
	                        //以下是假日时段
	                        //{0xC3, 0xbf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xcf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xdf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xef, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC4, 0x11, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x12, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x13, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x14, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x15, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x16, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x17, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x18, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x19, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1a, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1b, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1c, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1d, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1e, ZHOUXIURI_SHIDUAN&0xFF, ZHOUXIURI_SHIDUAN>>8&0xFF,0x1}
                          };

  //DL/T645-1997电量+需量+参变量+时段参变量数据与数据标识转换表
  INT8U  cmdDlt645pnWorkNwork1997[TOTAL_CMD_PN_WORK_NOWORK_645_07][5] = {
	                        //电量及需量 6项
	                        {0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x2f, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x1f, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x20, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1}
	                        };

#endif  //PROTOCOL_645_1997

//2.三相智能电能表DL/T645-2007规约
#ifdef  PROTOCOL_645_2007    
  //2.1 三相智能电能表DL/T645-2007上一结算日数据与数据标识转换表,26项
  INT8U cmdDlt645LastMonth2007[TOTAL_CMD_LASTMONTH_645_2007][6] = {
   {0x00, 0x01, 0xff, 0x01, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF},              //正向有功
   {0x00, 0x02, 0xff, 0x01, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF},                //反向有功
   
   {0x00, 0x01, 0x00, 0x01, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF},              //正向有功总,2012-05-21,add
   {0x00, 0x02, 0x00, 0x01, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF},                //反向有功总,2012-05-21,add
   
   {0x00, 0x03, 0xff, 0x01, POSITIVE_NO_WORK_OFFSET&0xFF, POSITIVE_NO_WORK_OFFSET>>8&0xFF},        //组合无功1
   {0x00, 0x04, 0xff, 0x01, NEGTIVE_NO_WORK_OFFSET&0xFF, NEGTIVE_NO_WORK_OFFSET>>8&0xFF},          //组合无功2
   {0x00, 0x05, 0xff, 0x01, QUA1_NO_WORK_OFFSET&0xFF, QUA1_NO_WORK_OFFSET>>8&0xFF},                //一象限无功
   {0x00, 0x08, 0xff, 0x01, QUA4_NO_WORK_OFFSET&0xFF, QUA4_NO_WORK_OFFSET>>8&0xFF},                //四象限无功
   {0x00, 0x06, 0xff, 0x01, QUA2_NO_WORK_OFFSET&0xFF, QUA2_NO_WORK_OFFSET>>8&0xFF},                //二象限无功
   {0x00, 0x07, 0xff, 0x01, QUA3_NO_WORK_OFFSET&0xFF, QUA3_NO_WORK_OFFSET>>8&0xFF},                //三象限无功
   {0x01, 0x01, 0xff, 0x01, REQ_POSITIVE_WORK_OFFSET&0xFF, REQ_POSITIVE_WORK_OFFSET>>8&0xFF},      //正向有功最大需量及发生时间
   {0x01, 0x02, 0xff, 0x01, REQ_NEGTIVE_WORK_OFFSET&0xFF, REQ_NEGTIVE_WORK_OFFSET>>8&0xFF},        //反向有功最大需量及发生时间
   {0x01, 0x03, 0xff, 0x01, REQ_POSITIVE_NO_WORK_OFFSET&0xFF, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xFF},//组合无功1最大需量及发生时间
   {0x01, 0x04, 0xff, 0x01, REQ_NEGTIVE_NO_WORK_OFFSET&0xFF, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xFF},  //组合无功2最大需量及发生时间
   {0x00, 0x85, 0x00, 0x01, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //铜损有功总电能示值(补偿量,4bytes)
   {0x00, 0x86, 0x00, 0x01, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //铁损有功总电能示值(补偿量,4bytes)
   {0x00, 0x15, 0x00, 0x01, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A相正向有功电能(4Bytes)
   {0x00, 0x16, 0x00, 0x01, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A相反向有功电能(4Bytes)
   {0x00, 0x17, 0x00, 0x01, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功1电能(4Bytes)
   {0x00, 0x18, 0x00, 0x01, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功2电能(4Bytes)
   {0x00, 0x29, 0x00, 0x01, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B相正向有功电能(4Bytes)
   {0x00, 0x2A, 0x00, 0x01, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B相反向有功电能(4Bytes)
   {0x00, 0x2B, 0x00, 0x01, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功1电能(4Bytes)
   {0x00, 0x2C, 0x00, 0x01, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功2电能(4Bytes)
   {0x00, 0x3D, 0x00, 0x01, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C相正向有功电能(4Bytes)
   {0x00, 0x3E, 0x00, 0x01, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C相反向有功电能(4Bytes)
   {0x00, 0x3F, 0x00, 0x01, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功1电能(4Bytes)
   {0x00, 0x40, 0x00, 0x01, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功2电能(4Bytes)
   };
                          
  //2.2 三相智能电能表DL/T645-2007电量+需量+参变量+时段参变量数据与数据标识转换表
 #ifdef DKY_SUBMISSION  //适应电科院送检
  INT8U  cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
   //2.2.1 电量 22项(全部4bytes,与97一致)
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能数据块
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能数据块

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能总,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能总,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //组合无功1电能数据块
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //组合无功2电能数据块
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1象限无功电能数据块
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4象限无功电能数据块
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2象限无功电能数据块
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3象限无功电能数据块
   //{0x00, 0x85, 0x00, 0x00, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //铜损有功总电能示值(补偿量,4bytes)
   //{0x00, 0x86, 0x00, 0x00, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //铁损有功总电能示值(补偿量,4bytes)
   {0x00, 0x15, 0x00, 0x00, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A相正向有功电能(4Bytes)
   {0x00, 0x16, 0x00, 0x00, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A相反向有功电能(4Bytes)
   {0x00, 0x17, 0x00, 0x00, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功1电能(4Bytes)
   {0x00, 0x18, 0x00, 0x00, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功2电能(4Bytes)
   {0x00, 0x29, 0x00, 0x00, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B相正向有功电能(4Bytes)
   {0x00, 0x2A, 0x00, 0x00, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B相反向有功电能(4Bytes)
   {0x00, 0x2B, 0x00, 0x00, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功1电能(4Bytes)
   {0x00, 0x2C, 0x00, 0x00, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功2电能(4Bytes)
   {0x00, 0x3D, 0x00, 0x00, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C相正向有功电能(4Bytes)
   {0x00, 0x3E, 0x00, 0x00, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C相反向有功电能(4Bytes)
   {0x00, 0x3F, 0x00, 0x00, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功1电能(4Bytes)
   {0x00, 0x40, 0x00, 0x00, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功2电能(4Bytes)
   
   //2.2.2 需量及需量发生时间 4项(需量3bytes+5bytes年月日时分,97是3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //正向有功最大需量及发生时间
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //反向有功最大需量及发生时间
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//组合无功1最大需量及发生时间
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //组合无功2最大需量及发生时间
   
   //2.2.3 变量数据 8项
   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //电压数据块(A,B,C相电压)(各2字节=97,但格式97是xxx而07是xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //电流数据块(A,B,C相电流)(各3字节<>97,97是各2字节xx.xx,07是xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //瞬时有功功率块(总,A,B,C相有功功率)(各3字节=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //瞬时无功功率块(总,A,B,C相无功功率)(各3字节xx.xxxx<>97是xx.xx)
   {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},          //瞬时视在功率块(总,A,B,C相视在功率)(各3字节xx.xxxx<>97没有)
   {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                  //功率因数块(总,A,B,C相功率因数)(各2字节=97,x.xxx)   
   {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},                        //相角数据块(A,B,C相相角)(各2字节,97无,x.xxx)
   {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},                //零线电流(3字节xxx.xxx<>97是2字节xx.xx)
   {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},                    //电池工作时间(4字节NNNNNNNN<>97是3字节NNNNNN)

   //2.2.4.事件记录数据 26项
   //2.2.4-1.断相统计数据
   //2.2.4-1.1有些07表支持规约,用以下4项抄读
   //总断相次数,总累计时间未找到,难道要计算(10-07-05,经与东软集中器对比,确实需要计算)
   {0x03, 0x04, 0x00, 0x00, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)
                                                                                                   //B相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)
                                                                                                   //C相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)

   //最近一次断相发生起始时刻及结束时刻未找到,难道要计算?(10-07-05,经与东软集中器对比,确实需要计算)
   //{0x03, 0x04, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //上一次A相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)
   //{0x03, 0x04, 0x02, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //上一次B相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)
   //{0x03, 0x04, 0x03, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //上一次C相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)

   //2.2.4-1.2 有些表支持备案文件,所以用以下12条抄读断相统计数据
   //总断相次数,总累计时间未找到,难道要计算(10-07-05,经与东软集中器对比,确实需要计算)
   {0x13, 0x01, 0x00, 0x01, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x02, 0x00, 0x01, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF},                  //B相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x03, 0x00, 0x01, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF},                  //B相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x01, 0x00, 0x02, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF},        //A相累计时间(累计3字节=97)
   {0x13, 0x02, 0x00, 0x02, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF},        //B相累计时间(累计3字节=97)
   {0x13, 0x03, 0x00, 0x02, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF},        //C相累计时间(累计3字节=97)

   //2.2.4-1.3 最近一次断相发生起始时刻及结束时刻未找到,难道要计算?(10-07-05,经与东软集中器对比,确实需要计算)
   
   //{0x13, 0x01, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //上一次A相断相记录(起始时刻)(时刻6字节<>97,4字节)
   //{0x13, 0x02, 0x01, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //上一次B相断相记录(起始时刻)(时刻6字节<>97,4字节)
   //{0x13, 0x03, 0x01, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //上一次C相断相记录(起始时刻)(时刻6字节<>97,4字节)
   //{0x13, 0x01, 0x25, 0x01, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF},            //上一次A相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   //{0x13, 0x02, 0x25, 0x01, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF},            //上一次B相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   //{0x13, 0x03, 0x25, 0x01, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF},            //上一次C相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   
   //2.2.4-2 其它统计量
   {0x03, 0x30, 0x00, 0x00, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF},                            //编程总次数(3字节!=97,2字节)
   {0x03, 0x30, 0x00, 0x01, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF},                    //最近一次编程时间(时间6字节<>97,4字节)
   {0x03, 0x30, 0x01, 0x00, METER_CLEAR_TIMES&0xFF, METER_CLEAR_TIMES>>8&0xFF},                    //电表清零总次数(3字节!=97没有)
   {0x03, 0x30, 0x01, 0x01, LAST_METER_CLEAR_TIME&0xFF, LAST_METER_CLEAR_TIME>>8&0xFF},            //最近一次电表清零时间(时间6字节<>97没有)
   {0x03, 0x30, 0x02, 0x00, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF},                        //最大需量清零总次数(3字节!=97,2字节)
   {0x03, 0x30, 0x02, 0x01, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF},              //最近一次最大需量清零时间(时间6字节<>97,4字节)
   {0x03, 0x30, 0x03, 0x00, EVENT_CLEAR_TIMES&0xFF, EVENT_CLEAR_TIMES>>8&0xFF},                    //事件清零总次数(3字节!=97,没有)
   {0x03, 0x30, 0x03, 0x01, EVENT_CLEAR_LAST_TIME&0xFF, EVENT_CLEAR_LAST_TIME>>8&0xFF},            //事件清零时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x04, 0x00, TIMING_TIMES&0xFF, TIMING_TIMES>>8&0xFF},                              //校时总次数(3字节!=97,没有)
   {0x03, 0x30, 0x04, 0x01, TIMING_LAST_TIME&0xFF, TIMING_LAST_TIME>>8&0xFF},                      //最近一次校时时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x05, 0x00, PERIOD_TIMES&0xFF, PERIOD_TIMES>>8&0xFF},                              //时段表编程总次数(3字节!=97,没有)
   //{0x03, 0x30, 0x05, 0x01, PERIOD_LAST_TIME&0xFF, PERIOD_LAST_TIME>>8&0xFF},                      //最近一次时段表编程发生时间(时间6字节<>97,没有)
   //{0x03, 0x30, 0x0d, 0x00, OPEN_METER_COVER_TIMES&0xFF, OPEN_METER_COVER_TIMES>>8&0xFF},          //开表盖总次数(2字节,97没有)
   //{0x03, 0x30, 0x0d, 0x01, LAST_OPEN_METER_COVER_TIME&0xFF, LAST_OPEN_METER_COVER_TIME>>8&0xFF},  //上一次开表盖发生时刻(6字节,97没有)
   
   //2.2.5 参变量 9项
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                            //日期及周次(4字节=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},                                  //电表时间(3字节=97)
   {0x04, 0x00, 0x04, 0x09, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF},                            //电表常数(有功)(3字节=97)
   {0x04, 0x00, 0x04, 0x0a, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF},                      //电表常数(无功)(3字节=97)
   //{0x04, 0x00, 0x04, 0x02, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF},                              //电表号(6字节=97)
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},                    //电表运行状态字1到7(7*2<>97只有2bytes)
   {0x04, 0x00, 0x0b, 0x01, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF},                            //每月第1结算日
   //{0x04, 0x00, 0x0b, 0x02, AUTO_COPY_DAY_2&0xFF, AUTO_COPY_DAY_2>>8&0xFF},                        //每月第2结算日
   //{0x04, 0x00, 0x0b, 0x03, AUTO_COPY_DAY_3&0xFF, AUTO_COPY_DAY_3>>8&0xFF},                        //每月第3结算日
   
   //2.2.6 以下几行时段参变量 6项
   //{0x04, 0x00, 0x02, 0x01, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF},                              //年时区数(1字节)
   //{0x04, 0x00, 0x02, 0x02, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF},                  //日时段表数(1字节)
   //{0x04, 0x00, 0x02, 0x03, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF},                            //日时段数(1字节)
   //{0x04, 0x00, 0x02, 0x04, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF},                              //费率数(1字节)
   {0x04, 0x00, 0x02, 0x05, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF},                        //公共假日数(1字节)   
   {0x04, 0x01, 0x00, 0x00, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF},                                  //第一套时区表数据(14*3)
   
   //2.2.7 以下是第一套日时段表、起始时间及费率号 1项
   {0x04, 0x01, 0x00, 0x01, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第1日时段表数据(当前存前10个时段起始时间和费率号)
   };
	 
 #elif defined JF_MONITOR          //机房监控使用
 
  INT8U cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
	 //2.2.1 电量 6项(全部4bytes,与97一致)
	 {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},							//正向有功电能
	 {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},								//反向有功电能
 
	 {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},							//正向有功电能示值总,2012-05-21,add
	 {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},								//反向有功电能示值总,2012-05-21,add
 
	 {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},				//组合无功1电能
	 {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},					//组合无功2电能
	
	 //2.2.2 需量及需量发生时间 4项(需量3bytes+5bytes年月日时分,97是3+4Bytes)
	 {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},			//正向有功最大需量及发生时间
	 {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},				//反向有功最大需量及发生时间
	 {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//组合无功1最大需量及发生时间
	 {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},	//组合无功2最大需量及发生时间
	 
	 //2.2.3 变量数据 8项
	 {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},												//电压数据块(A,B,C相电压)(各2字节=97,但格式97是xxx而07是xxx.x)
	 {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},												//电流数据块(A,B,C相电流)(各3字节<>97,97是各2字节xx.xx,07是xxx.xxx)
	 {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},									//瞬时有功功率块(总,A,B,C相有功功率)(各3字节=97,xx.xxxx)
	 {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},						//瞬时无功功率块(总,A,B,C相无功功率)(各3字节xx.xxxx<>97是xx.xx)
	 {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},					//瞬时视在功率块(总,A,B,C相视在功率)(各3字节xx.xxxx<>97没有)
	 {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},									//功率因数块(总,A,B,C相功率因数)(各2字节=97,x.xxx)	 
	 {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},												//相角数据块(A,B,C相相角)(各2字节,97无,x.xxx)
	 {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},								//零线电流(3字节xxx.xxx<>97是2字节xx.xx)
	 {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},										//电池工作时间(4字节NNNNNNNN<>97是3字节NNNNNN)
	
	 //2.2.5 参变量 9项
	 {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},														//日期及周次(4字节=97)
	 {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},																	//电表时间(3字节=97)
	 {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},										//电表运行状态字1到7(7*2<>97只有2bytes)

	 };
  
 #else           //正式使用程序
 
  INT8U  cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
   //2.2.1 电量 22项(全部4bytes,与97一致)
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能示值总,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能示值总,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //组合无功1电能
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //组合无功2电能
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1象限无功电能
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4象限无功电能
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2象限无功电能
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3象限无功电能
   {0x00, 0x85, 0x00, 0x00, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //铜损有功总电能示值(补偿量,4bytes)
   {0x00, 0x86, 0x00, 0x00, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //铁损有功总电能示值(补偿量,4bytes)
   {0x00, 0x15, 0x00, 0x00, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A相正向有功电能(4Bytes)
   {0x00, 0x16, 0x00, 0x00, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A相反向有功电能(4Bytes)
   {0x00, 0x17, 0x00, 0x00, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功1电能(4Bytes)
   {0x00, 0x18, 0x00, 0x00, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A相组合无功2电能(4Bytes)
   {0x00, 0x29, 0x00, 0x00, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B相正向有功电能(4Bytes)
   {0x00, 0x2A, 0x00, 0x00, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B相反向有功电能(4Bytes)
   {0x00, 0x2B, 0x00, 0x00, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功1电能(4Bytes)
   {0x00, 0x2C, 0x00, 0x00, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B相组合无功2电能(4Bytes)
   {0x00, 0x3D, 0x00, 0x00, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C相正向有功电能(4Bytes)
   {0x00, 0x3E, 0x00, 0x00, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C相反向有功电能(4Bytes)
   {0x00, 0x3F, 0x00, 0x00, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功1电能(4Bytes)
   {0x00, 0x40, 0x00, 0x00, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C相组合无功2电能(4Bytes)
   
   //2.2.2 需量及需量发生时间 4项(需量3bytes+5bytes年月日时分,97是3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //正向有功最大需量及发生时间
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //反向有功最大需量及发生时间
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//组合无功1最大需量及发生时间
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //组合无功2最大需量及发生时间
   
   //2.2.3 变量数据 8项
   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //电压数据块(A,B,C相电压)(各2字节=97,但格式97是xxx而07是xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //电流数据块(A,B,C相电流)(各3字节<>97,97是各2字节xx.xx,07是xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //瞬时有功功率块(总,A,B,C相有功功率)(各3字节=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //瞬时无功功率块(总,A,B,C相无功功率)(各3字节xx.xxxx<>97是xx.xx)
   {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},          //瞬时视在功率块(总,A,B,C相视在功率)(各3字节xx.xxxx<>97没有)
   {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                  //功率因数块(总,A,B,C相功率因数)(各2字节=97,x.xxx)   
   {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},                        //相角数据块(A,B,C相相角)(各2字节,97无,x.xxx)
   {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},                //零线电流(3字节xxx.xxx<>97是2字节xx.xx)
   {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},                    //电池工作时间(4字节NNNNNNNN<>97是3字节NNNNNN)

   //2.2.4.事件记录数据 26项
   //2.2.4-1.断相统计数据
   //2.2.4-1.1有些07表支持规约,用以下4项抄读
   //总断相次数,总累计时间未找到,难道要计算(10-07-05,经与东软集中器对比,确实需要计算)
   {0x03, 0x04, 0x00, 0x00, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)
                                                                                                   //B相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)
                                                                                                   //C相断相总次数,总累计时间(次数3字节<>97次数2字节,累计3字节=97)

   //最近一次断相发生起始时刻及结束时刻未找到,难道要计算?(10-07-05,经与东软集中器对比,确实需要计算)
   {0x03, 0x04, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //上一次A相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)
   {0x03, 0x04, 0x02, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //上一次B相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)
   {0x03, 0x04, 0x03, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //上一次C相断相记录(起始时刻及结束时刻)(时刻6字节<>97,4字节)

   //2.2.4-1.2 有些表支持备案文件,所以用以下12条抄读断相统计数据
   //总断相次数,总累计时间未找到,难道要计算(10-07-05,经与东软集中器对比,确实需要计算)
   {0x13, 0x01, 0x00, 0x01, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x02, 0x00, 0x01, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF},                  //B相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x03, 0x00, 0x01, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF},                  //B相断相总次数(次数3字节<>97次数2字节)
   {0x13, 0x01, 0x00, 0x02, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF},        //A相累计时间(累计3字节=97)
   {0x13, 0x02, 0x00, 0x02, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF},        //B相累计时间(累计3字节=97)
   {0x13, 0x03, 0x00, 0x02, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF},        //C相累计时间(累计3字节=97)

   //2.2.4-1.3 最近一次断相发生起始时刻及结束时刻未找到,难道要计算?(10-07-05,经与东软集中器对比,确实需要计算)
   
   {0x13, 0x01, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //上一次A相断相记录(起始时刻)(时刻6字节<>97,4字节)
   {0x13, 0x02, 0x01, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //上一次B相断相记录(起始时刻)(时刻6字节<>97,4字节)
   {0x13, 0x03, 0x01, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //上一次C相断相记录(起始时刻)(时刻6字节<>97,4字节)
   {0x13, 0x01, 0x25, 0x01, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF},            //上一次A相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   {0x13, 0x02, 0x25, 0x01, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF},            //上一次B相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   {0x13, 0x03, 0x25, 0x01, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF},            //上一次C相断相记录(及结束时刻)(时刻6字节<>97,4字节)
   
   //2.2.4-2 其它统计量
   {0x03, 0x30, 0x00, 0x00, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF},                            //编程总次数(3字节!=97,2字节)
   {0x03, 0x30, 0x00, 0x01, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF},                    //最近一次编程时间(时间6字节<>97,4字节)
   {0x03, 0x30, 0x01, 0x00, METER_CLEAR_TIMES&0xFF, METER_CLEAR_TIMES>>8&0xFF},                    //电表清零总次数(3字节!=97没有)
   {0x03, 0x30, 0x01, 0x01, LAST_METER_CLEAR_TIME&0xFF, LAST_METER_CLEAR_TIME>>8&0xFF},            //最近一次电表清零时间(时间6字节<>97没有)
   {0x03, 0x30, 0x02, 0x00, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF},                        //最大需量清零总次数(3字节!=97,2字节)
   {0x03, 0x30, 0x02, 0x01, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF},              //最近一次最大需量清零时间(时间6字节<>97,4字节)
   {0x03, 0x30, 0x03, 0x00, EVENT_CLEAR_TIMES&0xFF, EVENT_CLEAR_TIMES>>8&0xFF},                    //事件清零总次数(3字节!=97,没有)
   {0x03, 0x30, 0x03, 0x01, EVENT_CLEAR_LAST_TIME&0xFF, EVENT_CLEAR_LAST_TIME>>8&0xFF},            //事件清零时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x04, 0x00, TIMING_TIMES&0xFF, TIMING_TIMES>>8&0xFF},                              //校时总次数(3字节!=97,没有)
   {0x03, 0x30, 0x04, 0x01, TIMING_LAST_TIME&0xFF, TIMING_LAST_TIME>>8&0xFF},                      //最近一次校时时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x05, 0x00, PERIOD_TIMES&0xFF, PERIOD_TIMES>>8&0xFF},                              //时段表编程总次数(3字节!=97,没有)
   {0x03, 0x30, 0x05, 0x01, PERIOD_LAST_TIME&0xFF, PERIOD_LAST_TIME>>8&0xFF},                      //最近一次时段表编程发生时间(时间6字节<>97,没有)
   {0x03, 0x30, 0x0d, 0x00, OPEN_METER_COVER_TIMES&0xFF, OPEN_METER_COVER_TIMES>>8&0xFF},          //开表盖总次数(2字节,97没有)
   {0x03, 0x30, 0x0d, 0x01, LAST_OPEN_METER_COVER_TIME&0xFF, LAST_OPEN_METER_COVER_TIME>>8&0xFF},  //上一次开表盖发生时刻(6字节,97没有)
   
   //2.2.5 参变量 9项
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                            //日期及周次(4字节=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},                                  //电表时间(3字节=97)
   {0x04, 0x00, 0x04, 0x09, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF},                            //电表常数(有功)(3字节=97)
   {0x04, 0x00, 0x04, 0x0a, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF},                      //电表常数(无功)(3字节=97)
   {0x04, 0x00, 0x04, 0x02, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF},                              //电表号(6字节=97)
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},                    //电表运行状态字1到7(7*2<>97只有2bytes)
   {0x04, 0x00, 0x0b, 0x01, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF},                            //每月第1结算日
   {0x04, 0x00, 0x0b, 0x02, AUTO_COPY_DAY_2&0xFF, AUTO_COPY_DAY_2>>8&0xFF},                        //每月第2结算日
   {0x04, 0x00, 0x0b, 0x03, AUTO_COPY_DAY_3&0xFF, AUTO_COPY_DAY_3>>8&0xFF},                        //每月第3结算日
   
   //2.2.6 以下几行时段参变量 6项
   {0x04, 0x00, 0x02, 0x01, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF},                              //年时区数(1字节)
   {0x04, 0x00, 0x02, 0x02, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF},                  //日时段表数(1字节)
   {0x04, 0x00, 0x02, 0x03, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF},                            //日时段数(1字节)
   {0x04, 0x00, 0x02, 0x04, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF},                              //费率数(1字节)
   {0x04, 0x00, 0x02, 0x05, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF},                        //公共假日数(1字节)   
   {0x04, 0x01, 0x00, 0x00, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF},                                  //第一套时区表数据(14*3)
   
   //2.2.7 以下是第一套日时段表、起始时间及费率号 8项
   {0x04, 0x01, 0x00, 0x01, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第1日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x02, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第2日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x03, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第3日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x04, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第4日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x05, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第5日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x06, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第6日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x07, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第7日时段表数据(当前存前10个时段起始时间和费率号)
   {0x04, 0x01, 0x00, 0x08, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //第8日时段表数据(当前存前10个时段起始时间和费率号)
   
   //2.2.8 费控表控制/开关/购电信息
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME&0xFF, LAST_JUMPED_GATE_TIME>>8&0xFF},            //上一次跳闸发生时刻(6字节,97没有)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME&0xFF, LAST_CLOSED_GATE_TIME>>8&0xFF},            //上一次合闸发生时刻(6字节,97没有)
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME&0xFF, CHARGE_TOTAL_TIME>>8&0xFF},                    //上1次购电后总购电次数(2字节)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY&0xFF, CHARGE_REMAIN_MONEY>>8&0xFF},                //当前剩余金额(4字节,97没有)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY&0xFF, CHARGE_TOTAL_MONEY>>8&0xFF},                  //上一次购电后累计购电金额(4字节,97没有)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY&0xFF, CHARGE_REMAIN_QUANTITY>>8&0xFF},          //当前剩余电量(4字节,97没有)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY&0xFF, CHARGE_OVERDRAFT_QUANTITY>>8&0xFF},    //当前透支电量(4字节,97没有)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY&0xFF, CHARGE_TOTAL_QUANTITY>>8&0xFF},            //上一次购电后累计购电量(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT&0xFF, CHARGE_OVERDRAFT_LIMIT>>8&0xFF},          //透支电量限值(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY&0xFF, CHARGE_ALARM_QUANTITY>>8&0xFF},            //报警电量1限值
   };
 #endif
  
  //2.3 三相表(智能表/远程费控表/本地费控表)上一日冻结数据(DL/T645-2007) 数据与数据标识转换表
  INT8U cmdDlt645LastDay2007[TOTAL_CMD_LASTDAY_645_2007][6] = {
   {0x05, 0x06, 0x00, 0x01, DAY_FREEZE_TIME_FLAG_T&0xFF, DAY_FREEZE_TIME_FLAG_T>>8&0xFF},          //上一次日冻结时标
   {0x05, 0x06, 0x01, 0x01, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF},              //正向有功
   {0x05, 0x06, 0x02, 0x01, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF},                //反向有功
   {0x05, 0x06, 0x03, 0x01, POSITIVE_NO_WORK_OFFSET&0xFF, POSITIVE_NO_WORK_OFFSET>>8&0xFF},        //组合无功1
   {0x05, 0x06, 0x04, 0x01, NEGTIVE_NO_WORK_OFFSET&0xFF, NEGTIVE_NO_WORK_OFFSET>>8&0xFF},          //组合无功2
   {0x05, 0x06, 0x05, 0x01, QUA1_NO_WORK_OFFSET&0xFF, QUA1_NO_WORK_OFFSET>>8&0xFF},                //一象限无功
   {0x05, 0x06, 0x06, 0x01, QUA2_NO_WORK_OFFSET&0xFF, QUA2_NO_WORK_OFFSET>>8&0xFF},                //二象限无功
   {0x05, 0x06, 0x07, 0x01, QUA3_NO_WORK_OFFSET&0xFF, QUA3_NO_WORK_OFFSET>>8&0xFF},                //三象限无功
   {0x05, 0x06, 0x08, 0x01, QUA4_NO_WORK_OFFSET&0xFF, QUA4_NO_WORK_OFFSET>>8&0xFF},                //四象限无功
   {0x05, 0x06, 0x09, 0x01, REQ_POSITIVE_WORK_OFFSET&0xFF, REQ_POSITIVE_WORK_OFFSET>>8&0xFF},      //正向有功最大需量及发生时间
   {0x05, 0x06, 0x0a, 0x01, REQ_NEGTIVE_WORK_OFFSET&0xFF, REQ_NEGTIVE_WORK_OFFSET>>8&0xFF},        //反向有功最大需量及发生时间
   };
   
  //2.4 单相表(智能表/远程费控表/本地费控表)上一日冻结数据(DL/T645-2007) 数据类型与单元标识(DI)转换表
  INT8U single2007LastDay[TOTAL_CMD_LASTDAY_SINGLE_07][6] = {
   {0x05, 0x06, 0x00, 0x01, DAY_FREEZE_TIME_FLAG_S&0xff,  DAY_FREEZE_TIME_FLAG_S>>8&0xff},         //(上1次)日冻结时间
   {0x05, 0x06, 0x01, 0x01, POSITIVE_WORK_OFFSET_S&0xff,  POSITIVE_WORK_OFFSET_S>>8&0xff},         //(上1次)日冻结正向有功电能示值(总及各费率)
   {0x05, 0x06, 0x02, 0x01, NEGTIVE_WORK_OFFSET_S&0xff, NEGTIVE_WORK_OFFSET_S>>8&0xff},            //(上1次)日冻结反向有功电能示值(总及各费率)
	 };
#endif  //PROTOCOL_645_2007

//3.单相97表
#ifdef  PROTOCOL_SINGLE_PHASE_97
  //单相表(DL/T645-1997)数据类型与单元标识(DI)转换表
  INT8U single1997[TOTAL_CMD_SINGLE_645_97][5] = {
	 //{0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},                    //正向有功总及各费率
	 {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},                      //正向有功总示值
	 //{0x90, 0x2f, NEGTIVE_WORK_OFFSET_X&0xff, NEGTIVE_WORK_OFFSET_X>>8&0xff,0x1},                  //反向有功总及各费率
	 //{0x90, 0x20, NEGTIVE_WORK_OFFSET_X&0xff, NEGTIVE_WORK_OFFSET_X>>8&0xff,0x1},                  //反向有功总示值
	 };
  INT8U single1997LastDay[TOTAL_CMD_SINGLE_645_97][5] = {
	 {0x9A, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},              
	 };
#endif

//4.单相电能表(智能表/本地费控/远程费控)07规约 - 实时采集 数据类型与单元标识DI转换表
#if  PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
  INT8U single2007[TOTAL_CMD_SINGLE_LOCAL_CTRL_07][6] = {
   //4.1 正反向有功电能示值
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET_S&0xff, POSITIVE_WORK_OFFSET_S>>8&0xff},          //当前正向有功电能示值(总及各费率)
   
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET_S&0xff, NEGTIVE_WORK_OFFSET_S>>8&0xff},            //当前反向有功电能示值(总及各费率)
   
   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET_S&0xff, POSITIVE_WORK_OFFSET_S>>8&0xff},          //当前正向有功电能示值总, 2012-5-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET_S&0xff, NEGTIVE_WORK_OFFSET_S>>8&0xff},            //当前反向有功电能示值总, 2012-5-21,add
   
   //4.2 电表运行状态字
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD_S&0xFF, METER_STATUS_WORD_S>>8&0xFF},                //电表运行状态字1到7(7*2<>97只有2bytes)
   
   //4.3 电能表日期、周次及时间
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK_S&0xFF, DATE_AND_WEEK_S>>8&0xFF},                        //日期及周次(4字节=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME_S&0xFF, METER_TIME_S>>8&0xFF},                              //电表时间(3字节=97)

   //4.4 电能表远程控制通断电状态及记录
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME_S&0xFF, LAST_JUMPED_GATE_TIME_S>>8&0xFF},        //上一次跳闸发生时刻(6字节,97没有)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME_S&0xFF, LAST_CLOSED_GATE_TIME_S>>8&0xFF},        //上一次合闸发生时刻(6字节,97没有)
   
   //4.5 电能表购、用电信息
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME_S&0xFF, CHARGE_TOTAL_TIME_S>>8&0xFF},                //上1次购电后总购电次数(2字节)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY_S&0xFF, CHARGE_REMAIN_MONEY_S>>8&0xFF},            //当前剩余金额(4字节,97没有)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY_S&0xFF, CHARGE_TOTAL_MONEY_S>>8&0xFF},              //上一次购电后累计购电金额(4字节,97没有)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY_S&0xFF, CHARGE_REMAIN_QUANTITY_S>>8&0xFF},      //当前剩余电量(4字节,97没有)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY_S&0xFF, CHARGE_OVERDRAFT_QUANTITY_S>>8&0xFF},//当前透支电量(4字节,97没有)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY_S&0xFF, CHARGE_TOTAL_QUANTITY_S>>8&0xFF},        //上一次购电后累计购电量(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT_S&0xFF, CHARGE_OVERDRAFT_LIMIT_S>>8&0xFF},      //透支电量限值(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY_S&0xFF, CHARGE_ALARM_QUANTITY_S>>8&0xFF},        //报警电量1限值
	 };
#endif

//5.三相表(三相智能表/本地费控/远程费控)07规约 - 实时采集 数据类型与单元标识DI转换表
#if PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
  INT8U three2007[TOTAL_CMD_THREE_LOCAL_CTRL_07][6] = {
   //5.1 电能量示值
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //当前正向有功电能示值(总及各费率)
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //当前反向有功电能示值(总及各费率)

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //当前正向有功电能示值总,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //当前反向有功电能示值总,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //组合无功1电能
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //组合无功2电能
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1象限无功电能
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4象限无功电能
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2象限无功电能
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3象限无功电能   

   //5.2 电表运行状态字
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD_T&0xFF, METER_STATUS_WORD_T>>8&0xFF},                //电表运行状态字1到7(7*2<>97只有2bytes)

   //5.3 需量及需量发生时间 4项(需量3bytes+5bytes年月日时分,97是3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //正向有功最大需量及发生时间
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //反向有功最大需量及发生时间
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//组合无功1最大需量及发生时间
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //组合无功2最大需量及发生时间
   
   //5.4 电能表日期、周次及时间
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK_T&0xFF, DATE_AND_WEEK_T>>8&0xFF},                        //日期及周次(4字节=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME_T&0xFF, METER_TIME_T>>8&0xFF},                              //电表时间(3字节=97)

   //5.5 电能表远程控制通断电状态及记录
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME_T&0xFF, LAST_JUMPED_GATE_TIME_T>>8&0xFF},        //上一次跳闸发生时刻(6字节,97没有)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME_T&0xFF, LAST_CLOSED_GATE_TIME_T>>8&0xFF},        //上一次合闸发生时刻(6字节,97没有)
   
   //5.6 电能表购、用电信息
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME_T&0xFF, CHARGE_TOTAL_TIME_T>>8&0xFF},                //上1次购电后总购电次数(2字节)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY_T&0xFF, CHARGE_REMAIN_MONEY_T>>8&0xFF},            //当前剩余金额(4字节,97没有)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY_T&0xFF, CHARGE_TOTAL_MONEY_T>>8&0xFF},              //上一次购电后累计购电金额(4字节,97没有)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY_T&0xFF, CHARGE_REMAIN_QUANTITY_T>>8&0xFF},      //当前剩余电量(4字节,97没有)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY_T&0xFF, CHARGE_OVERDRAFT_QUANTITY_T>>8&0xFF},//当前透支电量(4字节,97没有)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY_T&0xFF, CHARGE_TOTAL_QUANTITY_T>>8&0xFF},        //上一次购电后累计购电量(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT_T&0xFF, CHARGE_OVERDRAFT_LIMIT_T>>8&0xFF},      //透支电量限值(4字节,97没有)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY_T&0xFF, CHARGE_ALARM_QUANTITY_T>>8&0xFF},        //报警电量1限值
	 };
#endif

//6.07规约的重点用户
#ifdef PROTOCOL_KEY_HOUSEHOLD_2007
  INT8U keyHousehold2007[TOTAL_CMD_KEY_2007][6] = {
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能总,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能总,2012-05-21,add

   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //电压数据块(A,B,C相电压)(各2字节=97,但格式97是xxx而07是xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //电流数据块(A,B,C相电流)(各3字节<>97,97是各2字节xx.xx,07是xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //瞬时有功功率块(总,A,B,C相有功功率)(各3字节=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //瞬时无功功率块(总,A,B,C相无功功率)(各3字节xx.xxxx<>97是xx.xx)
  };
#endif

//7.97点抄规约
  INT8U dotCopy1997[3][6] = {
	 {0x90, 0x10,  0,  0, 0x1},                                                                      //正向有功总示值
	 {0xC0, 0x11,  2, 20, 0x1},                                                                      //电表时间(3字节)
	 {0xC0, 0x10, 23, 24, 0x1},                                                                      //日期及周次(4字节)
  };
  
//8.07点抄规约
  INT8U dotCopy2007[3][6] = {
   #ifdef DKY_SUBMISSION
    {0x00, 0x01, 0xff, 0x00,  0, 0},                                                               //正向有功电能
   #else
    {0x00, 0x01, 0x00, 0x00,  0, 0},                                                               //正向有功电能,2012-5-21,modify
   #endif
   {0x04, 0x00, 0x01, 0x02, 20, 0},                                                                //电表时间(3字节=97)
   {0x04, 0x00, 0x01, 0x01, 23, 0},                                                                //日期及周次(4字节=97)
  };

//9.07表整点冻结数据块
  INT8U hourFreeze2007[24][6] = {
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //整点冻结数据块
  };

  INT8U  cmdDlt645pnWorkNwork2007[TOTAL_CMD_PN_WORK_NOWORK_645_07][6] = {
   //电量 6项
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能数据块
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能数据块
   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //正向有功电能总
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //反向有功电能总

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //组合无功1电能
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff}           //组合无功2电能
   };

//红相(EDMI规约Command Line)表
#ifdef PROTOCOL_EDMI_GROUP
  //上月数据
  INT8U  cmdEdmiLastMonth[TOTAL_COMMAND_LASTMONTH_EDMI][4] = {
   //{寄存器高字节,寄存器低字节,抄读回的数据存储偏移}
   {0x00, 0x00, 0, 0},                                                                             //进入命令模式
   {0x00, 0x00, 0, 0},                                                                             //登入命令
   
   {0x01, 0x49, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},                          //正向有功电能总
   {0x01, 0x40, (POSITIVE_WORK_OFFSET+4)&0xff, (POSITIVE_WORK_OFFSET+4)>>8&0xff},                  //正向有功电能峰
   {0x01, 0x41, (POSITIVE_WORK_OFFSET+8)&0xff, (POSITIVE_WORK_OFFSET+8)>>8&0xff},                  //正向有功电能平
   {0x01, 0x42, (POSITIVE_WORK_OFFSET+12)&0xff, (POSITIVE_WORK_OFFSET+12)>>8&0xff},                //正向有功电能谷
   {0x01, 0x43, (POSITIVE_WORK_OFFSET+16)&0xff, (POSITIVE_WORK_OFFSET+16)>>8&0xff},                //正向有功电能尖峰

   {0x00, 0x49, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                            //反向有功电能总
   {0x00, 0x40, (NEGTIVE_WORK_OFFSET+4)&0xff, (NEGTIVE_WORK_OFFSET+4)>>8&0xff},                    //反向有功电能峰
   {0x00, 0x41, (NEGTIVE_WORK_OFFSET+8)&0xff, (NEGTIVE_WORK_OFFSET+8)>>8&0xff},                    //反向有功电能平
   {0x00, 0x42, (NEGTIVE_WORK_OFFSET+12)&0xff, (NEGTIVE_WORK_OFFSET+12)>>8&0xff},                  //反向有功电能谷
   {0x00, 0x43, (NEGTIVE_WORK_OFFSET+16)&0xff, (NEGTIVE_WORK_OFFSET+16)>>8&0xff},                  //反向有功电能尖峰

   {0x03, 0x49, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},                    //正向无功电能总
   {0x03, 0x40, (POSITIVE_NO_WORK_OFFSET+4)&0xff, (POSITIVE_NO_WORK_OFFSET+4)>>8&0xff},            //正向无功电能峰
   {0x03, 0x41, (POSITIVE_NO_WORK_OFFSET+8)&0xff, (POSITIVE_NO_WORK_OFFSET+8)>>8&0xff},            //正向无功电能平
   {0x03, 0x42, (POSITIVE_NO_WORK_OFFSET+12)&0xff, (POSITIVE_NO_WORK_OFFSET+12)>>8&0xff},          //正向无功电能谷
   {0x03, 0x43, (POSITIVE_NO_WORK_OFFSET+16)&0xff, (POSITIVE_NO_WORK_OFFSET+16)>>8&0xff},          //正向无功电能尖峰

   {0x02, 0x49, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},                      //反向无功电能总
   {0x02, 0x40, (NEGTIVE_NO_WORK_OFFSET+4)&0xff, (NEGTIVE_NO_WORK_OFFSET+4)>>8&0xff},              //反向无功电能峰
   {0x02, 0x41, (NEGTIVE_NO_WORK_OFFSET+8)&0xff, (NEGTIVE_NO_WORK_OFFSET+8)>>8&0xff},              //反向无功电能平
   {0x02, 0x42, (NEGTIVE_NO_WORK_OFFSET+12)&0xff, (NEGTIVE_NO_WORK_OFFSET+12)>>8&0xff},            //反向无功电能谷
   {0x02, 0x43, (NEGTIVE_NO_WORK_OFFSET+16)&0xff, (NEGTIVE_NO_WORK_OFFSET+16)>>8&0xff},            //反向无功电能尖峰

   {0x11, 0x29, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},                  //正向有功最大需量总
   {0x11, 0x20, (REQ_POSITIVE_WORK_OFFSET+3)&0xff, (REQ_POSITIVE_WORK_OFFSET+3)>>8&0xff},          //正向有功最大需量峰
   {0x11, 0x21, (REQ_POSITIVE_WORK_OFFSET+6)&0xff, (REQ_POSITIVE_WORK_OFFSET+6)>>8&0xff},          //正向有功最大需量平
   {0x11, 0x22, (REQ_POSITIVE_WORK_OFFSET+9)&0xff, (REQ_POSITIVE_WORK_OFFSET+9)>>8&0xff},          //正向有功最大需量谷
   {0x11, 0x23, (REQ_POSITIVE_WORK_OFFSET+12)&0xff, (REQ_POSITIVE_WORK_OFFSET+12)>>8&0xff},        //正向有功最大需量尖峰

   {0x81, 0x29, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff},                      //正向有功最大需量总出现时间
   {0x81, 0x20, (REQ_TIME_P_WORK_OFFSET+5)&0xff, (REQ_TIME_P_WORK_OFFSET+5)>>8&0xff},              //正向有功最大需量峰出现时间
   {0x81, 0x21, (REQ_TIME_P_WORK_OFFSET+10)&0xff, (REQ_TIME_P_WORK_OFFSET+10)>>8&0xff},            //正向有功最大需量平出现时间
   {0x81, 0x22, (REQ_TIME_P_WORK_OFFSET+15)&0xff, (REQ_TIME_P_WORK_OFFSET+15)>>8&0xff},            //正向有功最大需量谷出现时间
   {0x81, 0x23, (REQ_TIME_P_WORK_OFFSET+20)&0xff, (REQ_TIME_P_WORK_OFFSET+20)>>8&0xff},            //正向有功最大需量尖峰出现时间

   {0x10, 0x29, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},                    //反向有功最大需量总出现时间
   {0x10, 0x20, (REQ_NEGTIVE_WORK_OFFSET+3)&0xff, (REQ_NEGTIVE_WORK_OFFSET+3)>>8&0xff},            //反向有功最大需量峰出现时间
   {0x10, 0x21, (REQ_NEGTIVE_WORK_OFFSET+6)&0xff, (REQ_NEGTIVE_WORK_OFFSET+6)>>8&0xff},            //反向有功最大需量平出现时间
   {0x10, 0x22, (REQ_NEGTIVE_WORK_OFFSET+9)&0xff, (REQ_NEGTIVE_WORK_OFFSET+9)>>8&0xff},            //反向有功最大需量谷出现时间
   {0x10, 0x23, (REQ_NEGTIVE_WORK_OFFSET+12)&0xff, (REQ_NEGTIVE_WORK_OFFSET+12)>>8&0xff},          //反向有功最大需量尖峰出现时间

   {0x80, 0x29, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff},                      //反向有功最大需量总出现时间
   {0x80, 0x20, (REQ_TIME_N_WORK_OFFSET+5)&0xff, (REQ_TIME_N_WORK_OFFSET+5)>>8&0xff},              //反向有功最大需量峰出现时间
   {0x80, 0x21, (REQ_TIME_N_WORK_OFFSET+10)&0xff, (REQ_TIME_N_WORK_OFFSET+10)>>8&0xff},            //反向有功最大需量平出现时间
   {0x80, 0x22, (REQ_TIME_N_WORK_OFFSET+15)&0xff, (REQ_TIME_N_WORK_OFFSET+15)>>8&0xff},            //反向有功最大需量谷出现时间
   {0x80, 0x23, (REQ_TIME_N_WORK_OFFSET+20)&0xff, (REQ_TIME_N_WORK_OFFSET+20)>>8&0xff},            //反向有功最大需量尖峰出现时间

   {0x00, 0x00, 0x0, 0x00},                                                                        //退出电表
   };

  //当前数据
  INT8U  cmdEdmi[TOTAL_COMMAND_REAL_EDMI][4] = {
 //{寄存器高字节,寄存器低字节,抄读回的数据存储偏移}
   {0x00, 0x00, 0, 0},                                                                             //进入命令模式
   {0x00, 0x00, 0, 0},                                                                             //登入命令
   
   {0x01, 0x69, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},                          //正向有功电能总
   {0x01, 0x60, (POSITIVE_WORK_OFFSET+4)&0xff, (POSITIVE_WORK_OFFSET+4)>>8&0xff},                  //正向有功电能峰
   {0x01, 0x61, (POSITIVE_WORK_OFFSET+8)&0xff, (POSITIVE_WORK_OFFSET+8)>>8&0xff},                  //正向有功电能平
   {0x01, 0x62, (POSITIVE_WORK_OFFSET+12)&0xff, (POSITIVE_WORK_OFFSET+12)>>8&0xff},                //正向有功电能谷
   {0x01, 0x63, (POSITIVE_WORK_OFFSET+16)&0xff, (POSITIVE_WORK_OFFSET+16)>>8&0xff},                //正向有功电能尖峰

   {0x00, 0x69, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                            //反向有功电能总
   {0x00, 0x60, (NEGTIVE_WORK_OFFSET+4)&0xff, (NEGTIVE_WORK_OFFSET+4)>>8&0xff},                    //反向有功电能峰
   {0x00, 0x61, (NEGTIVE_WORK_OFFSET+8)&0xff, (NEGTIVE_WORK_OFFSET+8)>>8&0xff},                    //反向有功电能平
   {0x00, 0x62, (NEGTIVE_WORK_OFFSET+12)&0xff, (NEGTIVE_WORK_OFFSET+12)>>8&0xff},                  //反向有功电能谷
   {0x00, 0x63, (NEGTIVE_WORK_OFFSET+16)&0xff, (NEGTIVE_WORK_OFFSET+16)>>8&0xff},                  //反向有功电能尖峰

   {0x03, 0x69, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},                    //正向无功电能总
   {0x03, 0x60, (POSITIVE_NO_WORK_OFFSET+4)&0xff, (POSITIVE_NO_WORK_OFFSET+4)>>8&0xff},            //正向无功电能峰
   {0x03, 0x61, (POSITIVE_NO_WORK_OFFSET+8)&0xff, (POSITIVE_NO_WORK_OFFSET+8)>>8&0xff},            //正向无功电能平
   {0x03, 0x62, (POSITIVE_NO_WORK_OFFSET+12)&0xff, (POSITIVE_NO_WORK_OFFSET+12)>>8&0xff},          //正向无功电能谷
   {0x03, 0x63, (POSITIVE_NO_WORK_OFFSET+16)&0xff, (POSITIVE_NO_WORK_OFFSET+16)>>8&0xff},          //正向无功电能尖峰

   {0x02, 0x69, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},                      //反向无功电能总
   {0x02, 0x60, (NEGTIVE_NO_WORK_OFFSET+4)&0xff, (NEGTIVE_NO_WORK_OFFSET+4)>>8&0xff},              //反向无功电能峰
   {0x02, 0x61, (NEGTIVE_NO_WORK_OFFSET+8)&0xff, (NEGTIVE_NO_WORK_OFFSET+8)>>8&0xff},              //反向无功电能平
   {0x02, 0x62, (NEGTIVE_NO_WORK_OFFSET+12)&0xff, (NEGTIVE_NO_WORK_OFFSET+12)>>8&0xff},            //反向无功电能谷
   {0x02, 0x63, (NEGTIVE_NO_WORK_OFFSET+16)&0xff, (NEGTIVE_NO_WORK_OFFSET+16)>>8&0xff},            //反向无功电能尖峰

   {0x11, 0x09, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},                  //正向有功最大需量总
   {0x11, 0x00, (REQ_POSITIVE_WORK_OFFSET+3)&0xff, (REQ_POSITIVE_WORK_OFFSET+3)>>8&0xff},          //正向有功最大需量峰
   {0x11, 0x01, (REQ_POSITIVE_WORK_OFFSET+6)&0xff, (REQ_POSITIVE_WORK_OFFSET+6)>>8&0xff},          //正向有功最大需量平
   {0x11, 0x02, (REQ_POSITIVE_WORK_OFFSET+9)&0xff, (REQ_POSITIVE_WORK_OFFSET+9)>>8&0xff},          //正向有功最大需量谷
   {0x11, 0x03, (REQ_POSITIVE_WORK_OFFSET+12)&0xff, (REQ_POSITIVE_WORK_OFFSET+12)>>8&0xff},        //正向有功最大需量尖峰

   {0x81, 0x09, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff},                      //正向有功最大需量总出现时间
   {0x81, 0x00, (REQ_TIME_P_WORK_OFFSET+5)&0xff, (REQ_TIME_P_WORK_OFFSET+5)>>8&0xff},              //正向有功最大需量峰出现时间
   {0x81, 0x01, (REQ_TIME_P_WORK_OFFSET+10)&0xff, (REQ_TIME_P_WORK_OFFSET+10)>>8&0xff},            //正向有功最大需量平出现时间
   {0x81, 0x02, (REQ_TIME_P_WORK_OFFSET+15)&0xff, (REQ_TIME_P_WORK_OFFSET+15)>>8&0xff},            //正向有功最大需量谷出现时间
   {0x81, 0x03, (REQ_TIME_P_WORK_OFFSET+20)&0xff, (REQ_TIME_P_WORK_OFFSET+20)>>8&0xff},            //正向有功最大需量尖峰出现时间

   {0x10, 0x09, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},                    //反向有功最大需量总出现时间
   {0x10, 0x00, (REQ_NEGTIVE_WORK_OFFSET+3)&0xff, (REQ_NEGTIVE_WORK_OFFSET+3)>>8&0xff},            //反向有功最大需量峰出现时间
   {0x10, 0x01, (REQ_NEGTIVE_WORK_OFFSET+6)&0xff, (REQ_NEGTIVE_WORK_OFFSET+6)>>8&0xff},            //反向有功最大需量平出现时间
   {0x10, 0x02, (REQ_NEGTIVE_WORK_OFFSET+9)&0xff, (REQ_NEGTIVE_WORK_OFFSET+9)>>8&0xff},            //反向有功最大需量谷出现时间
   {0x10, 0x03, (REQ_NEGTIVE_WORK_OFFSET+12)&0xff, (REQ_NEGTIVE_WORK_OFFSET+12)>>8&0xff},          //反向有功最大需量尖峰出现时间

   {0x80, 0x09, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff},                      //反向有功最大需量总出现时间
   {0x80, 0x00, (REQ_TIME_N_WORK_OFFSET+5)&0xff, (REQ_TIME_N_WORK_OFFSET+5)>>8&0xff},              //反向有功最大需量峰出现时间
   {0x80, 0x01, (REQ_TIME_N_WORK_OFFSET+10)&0xff, (REQ_TIME_N_WORK_OFFSET+10)>>8&0xff},            //反向有功最大需量平出现时间
   {0x80, 0x02, (REQ_TIME_N_WORK_OFFSET+15)&0xff, (REQ_TIME_N_WORK_OFFSET+15)>>8&0xff},            //反向有功最大需量谷出现时间
   {0x80, 0x03, (REQ_TIME_N_WORK_OFFSET+20)&0xff, (REQ_TIME_N_WORK_OFFSET+20)>>8&0xff},            //反向有功最大需量尖峰出现时间
   
   {0xE0, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                                    //A相电压
   {0xE0, 0x01, VOLTAGE_PHASE_B&0xFF, VOLTAGE_PHASE_B>>8&0xFF},                                    //B相电压
   {0xE0, 0x02, VOLTAGE_PHASE_C&0xFF, VOLTAGE_PHASE_C>>8&0xFF},                                    //C相电压
   
   {0xE0, 0x10, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                                    //A相电流
   {0xE0, 0x11, CURRENT_PHASE_B&0xFF, CURRENT_PHASE_B>>8&0xFF},                                    //B相电流
   {0xE0, 0x12, CURRENT_PHASE_C&0xFF, CURRENT_PHASE_C>>8&0xFF},                                    //B相电流
   
   {0xE0, 0x33, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                              //总有功功率
   {0xE0, 0x30, POWER_PHASE_A_WORK&0xFF, POWER_PHASE_A_WORK>>8&0xFF},                              //A相有功功率
   {0xE0, 0x31, POWER_PHASE_B_WORK&0xFF, POWER_PHASE_B_WORK>>8&0xFF},                              //B相有功功率
   {0xE0, 0x32, POWER_PHASE_C_WORK&0xFF, POWER_PHASE_C_WORK>>8&0xFF},                              //C相有功功率
   
   {0xE0, 0x43, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},                        //总无功功率
   {0xE0, 0x40, POWER_PHASE_A_NO_WORK&0xFF, POWER_PHASE_A_NO_WORK>>8&0xFF},                        //A相无功功率
   {0xE0, 0x41, POWER_PHASE_B_NO_WORK&0xFF, POWER_PHASE_B_NO_WORK>>8&0xFF},                        //B相无功功率
   {0xE0, 0x42, POWER_PHASE_C_NO_WORK&0xFF, POWER_PHASE_C_NO_WORK>>8&0xFF},                        //C相无功功率
   
   {0xE0, 0x26, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                              //总功率因数
   
   {0xF0, 0x3D, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                                        //电表时间/日期

   {0x00, 0x00, 0x0, 0x00},                                                                        //退出电表
   };
#endif

//兰吉尔(/西门子Landis规约)表
#ifdef LANDIS_GRY_ZD_PROTOCOL
  char landisChar[TOTAL_DATA_ITEM_ZD][8] = 
  {
    "1.8.0",       //正向有功总电能示值
    "1.8.1",       //正向有功费率1电能示值
    "1.8.2",       //正向有功费率2电能示值
    "1.8.3",       //正向有功费率3电能示值
    "1.8.4",       //正向有功费率4电能示值
    "2.8.0",       //反向有功总电能示值
    "2.8.1",       //反向有功费率1电能示值
    "2.8.2",       //反向有功费率2电能示值
    "2.8.3",       //反向有功费率3电能示值
    "2.8.4",       //反向有功费率4电能示值
    "3.8.0",       //正向无功总电能示值
    "3.8.1",       //正向无功费率1电能示值
    "3.8.2",       //正向无功费率2电能示值
    "3.8.3",       //正向无功费率3电能示值
    "3.8.4",       //正向无功费率4电能示值
    "4.8.0",       //反向无功总电能示值
    "4.8.1",       //反向无功费率1电能示值
    "4.8.2",       //反向无功费率2电能示值
    "4.8.3",       //反向无功费率3电能示值
    "4.8.4",       //反向无功费率4电能示值
    "5.8.0",       //Ⅰ象限无功总电能示值
    "5.8.1",       //Ⅰ象限无功费率1电能示值
    "5.8.2",       //Ⅰ象限无功费率2电能示值
    "5.8.3",       //Ⅰ象限无功费率3电能示值
    "5.8.4",       //Ⅰ象限无功费率4电能示值
    "6.8.0",       //Ⅱ象限无功总电能示值
    "6.8.1",       //Ⅱ象限无功费率1电能示值
    "6.8.2",       //Ⅱ象限无功费率2电能示值
    "6.8.3",       //Ⅱ象限无功费率3电能示值
    "6.8.4",       //Ⅱ象限无功费率4电能示值
    "7.8.0",       //Ⅲ象限无功总电能示值
    "7.8.1",       //Ⅲ象限无功费率1电能示值
    "7.8.2",       //Ⅲ象限无功费率2电能示值
    "7.8.3",       //Ⅲ象限无功费率3电能示值
    "7.8.4",       //Ⅲ象限无功费率4电能示值
    "8.8.0",       //Ⅳ象限无功总电能示值
    "8.8.1",       //Ⅳ象限无功费率1电能示值
    "8.8.2",       //Ⅳ象限无功费率2电能示值
    "8.8.3",       //Ⅳ象限无功费率3电能示值
    "8.8.4",       //Ⅳ象限无功费率4电能示值
    
    "1.6.0",       //正向有功总最大需量及日期时间
    "1.6.1",       //正向有功费率1最大需量及日期时间
    "1.6.2",       //正向有功费率2最大需量及日期时间
    "1.6.3",       //正向有功费率3最大需量及日期时间
    "1.6.4",       //正向有功费率4最大需量及日期时间
    "2.6.0",       //反向有功总最大需量及日期时间
    "2.6.1",       //反向有功费率1最大需量及日期时间
    "2.6.2",       //反向有功费率2最大需量及日期时间
    "2.6.3",       //反向有功费率3最大需量及日期时间
    "2.6.4",       //反向有功费率4最大需量及日期时间
    "3.6.0",       //正向无功总最大需量及日期时间
    "3.6.1",       //正向无功费率1最大需量及日期时间
    "3.6.2",       //正向无功费率2最大需量及日期时间
    "3.6.3",       //正向无功费率3最大需量及日期时间
    "3.6.4",       //正向无功费率4最大需量及日期时间
    "4.6.0",       //反向无功总最大需量及日期时间
    "4.6.1",       //反向无功费率1最大需量及日期时间
    "4.6.2",       //反向无功费率2最大需量及日期时间
    "4.6.3",       //反向无功费率3最大需量及日期时间
    "4.6.4",       //反向无功费率4最大需量及日期时间
    
    "0.9.1",       //当前时间
    "0.9.2",       //当前日期
    "16.7",        //有功功率
    "131.7",       //无功功率
    "32.7",        //A相电压
    "52.7",        //B相电压
    "72.7",        //C相电压
    "31.7",        //A相电流
    "51.7",        //B相电流
    "71.7",        //C相电流
    "C.2.0",       //参数设置的次数
    "C.2.1",       //上次参数设置的日期
    "C.6.0",       //电池使用时间(小时数)

    "31.1",        //A相电压相角
    "31.2",        //B相电压相角
    "31.3",        //C相电压相角
    "51.1",        //A相电流相角
    "51.2",        //B相电流相角
    "51.3",        //C相电流相角
    
    //"0.1.0",       //复位次数及上次复位时间
    //"C.7.0",       //总失压次数
    //"C.7.1",       //A相失压次数
    //"C.7.2",       //B相失压次数
    //"C.7.3",       //C相失压次数
  };
  
  INT16U landisOffset[TOTAL_DATA_ITEM_ZD] = 
  {
    POSITIVE_WORK_OFFSET,           //正向有功总电能示值
    POSITIVE_WORK_OFFSET+4,         //正向有功费率1电能示值
    POSITIVE_WORK_OFFSET+8,         //正向有功费率2电能示值
    POSITIVE_WORK_OFFSET+12,        //正向有功费率3电能示值
    POSITIVE_WORK_OFFSET+16,        //正向有功费率4电能示值
    NEGTIVE_WORK_OFFSET,            //反向有功总电能示值
    NEGTIVE_WORK_OFFSET+4,          //反向有功费率1电能示值
    NEGTIVE_WORK_OFFSET+8,          //反向有功费率2电能示值
    NEGTIVE_WORK_OFFSET+12,         //反向有功费率3电能示值
    NEGTIVE_WORK_OFFSET+16,         //反向有功费率4电能示值
    POSITIVE_NO_WORK_OFFSET,        //正向无功总电能示值
    POSITIVE_NO_WORK_OFFSET+4,      //正向无功费率1电能示值
    POSITIVE_NO_WORK_OFFSET+8,      //正向无功费率2电能示值
    POSITIVE_NO_WORK_OFFSET+12,     //正向无功费率3电能示值
    POSITIVE_NO_WORK_OFFSET+16,     //正向无功费率4电能示值
    NEGTIVE_NO_WORK_OFFSET,         //反向无功总电能示值
    NEGTIVE_NO_WORK_OFFSET+4,       //反向无功费率1电能示值
    NEGTIVE_NO_WORK_OFFSET+8,       //反向无功费率2电能示值
    NEGTIVE_NO_WORK_OFFSET+12,      //反向无功费率3电能示值
    NEGTIVE_NO_WORK_OFFSET+16,      //反向无功费率4电能示值
    QUA1_NO_WORK_OFFSET,            //Ⅰ象限无功总电能示值
    QUA1_NO_WORK_OFFSET+4,          //Ⅰ象限无功费率1电能示值
    QUA1_NO_WORK_OFFSET+8,          //Ⅰ象限无功费率2电能示值
    QUA1_NO_WORK_OFFSET+12,         //Ⅰ象限无功费率3电能示值
    QUA1_NO_WORK_OFFSET+16,         //Ⅰ象限无功费率4电能示值
    QUA2_NO_WORK_OFFSET,            //Ⅱ象限无功总电能示值
    QUA2_NO_WORK_OFFSET+4,          //Ⅱ象限无功费率1电能示值
    QUA2_NO_WORK_OFFSET+8,          //Ⅱ象限无功费率2电能示值
    QUA2_NO_WORK_OFFSET+12,         //Ⅱ象限无功费率3电能示值
    QUA2_NO_WORK_OFFSET+16,         //Ⅱ象限无功费率4电能示值
    QUA3_NO_WORK_OFFSET,            //Ⅲ象限无功总电能示值
    QUA3_NO_WORK_OFFSET+4,          //Ⅲ象限无功费率1电能示值
    QUA3_NO_WORK_OFFSET+8,          //Ⅲ象限无功费率2电能示值
    QUA3_NO_WORK_OFFSET+12,         //Ⅲ象限无功费率3电能示值
    QUA3_NO_WORK_OFFSET+16,         //Ⅲ象限无功费率4电能示值
    QUA4_NO_WORK_OFFSET,            //Ⅳ象限无功总电能示值
    QUA4_NO_WORK_OFFSET+4,          //Ⅳ象限无功费率1电能示值
    QUA4_NO_WORK_OFFSET+8,          //Ⅳ象限无功费率2电能示值
    QUA4_NO_WORK_OFFSET+12,         //Ⅳ象限无功费率3电能示值
    QUA4_NO_WORK_OFFSET+16,         //Ⅳ象限无功费率4电能示值
    
    REQ_POSITIVE_WORK_OFFSET,       //正向有功总最大需量及日期时间
    REQ_POSITIVE_WORK_OFFSET+3,     //正向有功费率1最大需量及日期时间
    REQ_POSITIVE_WORK_OFFSET+6,     //正向有功费率2最大需量及日期时间
    REQ_POSITIVE_WORK_OFFSET+9,     //正向有功费率3最大需量及日期时间
    REQ_POSITIVE_WORK_OFFSET+12,    //正向有功费率4最大需量及日期时间
    REQ_NEGTIVE_WORK_OFFSET,        //反向有功总最大需量及日期时间
    REQ_NEGTIVE_WORK_OFFSET+3,      //反向有功费率1最大需量及日期时间
    REQ_NEGTIVE_WORK_OFFSET+6,      //反向有功费率2最大需量及日期时间
    REQ_NEGTIVE_WORK_OFFSET+9,      //反向有功费率3最大需量及日期时间
    REQ_NEGTIVE_WORK_OFFSET+12,     //反向有功费率4最大需量及日期时间
    REQ_POSITIVE_NO_WORK_OFFSET,    //正向无功总最大需量及日期时间
    REQ_POSITIVE_NO_WORK_OFFSET+3,  //正向无功费率1最大需量及日期时间
    REQ_POSITIVE_NO_WORK_OFFSET+6,  //正向无功费率2最大需量及日期时间
    REQ_POSITIVE_NO_WORK_OFFSET+9,  //正向无功费率3最大需量及日期时间
    REQ_POSITIVE_NO_WORK_OFFSET+12, //正向无功费率4最大需量及日期时间
    REQ_NEGTIVE_NO_WORK_OFFSET,     //反向无功总最大需量及日期时间
    REQ_NEGTIVE_NO_WORK_OFFSET+3,   //反向无功费率1最大需量及日期时间
    REQ_NEGTIVE_NO_WORK_OFFSET+6,   //反向无功费率2最大需量及日期时间
    REQ_NEGTIVE_NO_WORK_OFFSET+9,   //反向无功费率3最大需量及日期时间
    REQ_NEGTIVE_NO_WORK_OFFSET+12,  //反向无功费率4最大需量及日期时间
    
    METER_TIME,                     //当前时间
    DATE_AND_WEEK,                  //当前日期
    POWER_INSTANT_WORK,             //有功功率
    POWER_INSTANT_NO_WORK,          //无功功率    
    VOLTAGE_PHASE_A,                //A相电压
    VOLTAGE_PHASE_B,                //B相电压
    VOLTAGE_PHASE_C,                //C相电压
    CURRENT_PHASE_A,                //A相电流
    CURRENT_PHASE_B,                //B相电流
    CURRENT_PHASE_C,                //C相电流
    PROGRAM_TIMES,                  //参数设置的次数
    LAST_PROGRAM_TIME,              //上次参数设置的日期
    BATTERY_WORK_TIME,              //电池使用时间(小时数)

    PHASE_ANGLE_V_A,                //A相电压相角
    PHASE_ANGLE_V_B,                //B相电压相角
    PHASE_ANGLE_V_C,                //C相电压相角
    PHASE_ANGLE_C_A,                //A相电流相角
    PHASE_ANGLE_C_B,                //B相电流相角
    PHASE_ANGLE_C_C,                //C相电流相角
    
    //"0.1.0",       //复位次数及上次复位时间
    //"C.7.0",       //总失压次数
    //"C.7.1",       //A相失压次数
    //"C.7.2",       //B相失压次数
    //"C.7.3",       //C相失压次数
  };
#endif

INT16U findDataOffset(INT8U protocol, INT8U *di);
INT8S  meterInput(INT8U arrayItem, INT8U *pFrameHead,INT16U frameLength);

#ifdef PROTOCOL_645_1997
 INT8S process645Para(INT8U arrayItem, INT8U *pDataHead, INT8U len);
 INT8S process645Data(INT8U arrayItem, INT8U *pDataHead,INT8U length);
 INT8S processShiDuanData(INT8U arrayItem, INT8U *pDataHead,INT8U length);
#endif

#ifdef PROTOCOL_645_2007
 INT8S process645Data2007(INT8U arrayItem,struct recvFrameStruct *frame);
#endif

#ifdef PROTOCOL_EDMI_GROUP
 INT8S processEdmiData(INT8U arrayItem, INT8U *frame, INT16U loadLen);

 //注意:因最高位一定为"1",故略去
 const unsigned short cnCRC_16    = 0x8005;
 //CRC-16 = X16 + X15 + X2 + X0
 const unsigned short cnCRC_CCITT = 0x1021;
 //CRC-CCITT = X16 + X12 + X5 + X0，据说这个16位CRC多项式比上一个要好
 //const unsigned long cnCRC_32    = 0x04C10DB7;
 //CRC-32 = X32 + X26 + X23 + X22 + X16 + X11 + X10 + X8 + X7 + X5 + X4 + X2 + X1 + X0

 unsigned long tableCrc[256];      //CRC表

/*******************************************************
函数名称:buildTable16
功能描述:构造16位CRC表
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：CRC值
*******************************************************/
void buildTable16( unsigned short aPoly )
{
  unsigned short i, j;
  unsigned short nData;
  unsigned short nAccum;

  for (i=0; i<256; i++)
  {
    nData = (unsigned short)(i<<8);
    nAccum = 0;
    for (j=0; j<8; j++)
    {
      if ((nData^nAccum) & 0x8000)
      {
        nAccum = (nAccum<<1) ^ aPoly;
      }
      else
      {
        nAccum <<= 1;
      }
      
      nData <<= 1;
    }
    
    tableCrc[i] = ( unsigned long )nAccum;
  }
}

/*******************************************************
函数名称:edmi_CRC_16
功能描述:计算16位CRC值(CRC-16或CRC-CCITT)
调用函数:
被调用函数:
输入参数:aData,要进行CRC校验的消息
         aSize,消息中字节数
输出参数:
返回值：CRC值
*******************************************************/
unsigned short edmi_CRC_16(unsigned char * aData, unsigned long aSize)
{
  unsigned long i;
  unsigned short nAccum = 0;

  buildTable16(cnCRC_CCITT); // or cnCRC_16
  for ( i = 0; i < aSize; i++ )
  {
    nAccum = (nAccum<<8) ^ (unsigned short)tableCrc[(nAccum>>8)^*aData++];
  }
  
  return nAccum;
}


#endif

#ifdef LANDIS_GRY_ZD_PROTOCOL
 INT8S processLandisGryData(INT8U arrayItem, INT8U *frame, INT16U loadLen);
#endif

#ifdef PROTOCOL_ABB_GROUP
 INT8S processAbbData(INT8U arrayItem, INT8U *frame, INT16U loadLen, INT8U copyItem);
 unsigned short CRC_ABB(unsigned char * aData, unsigned long aSize);
 unsigned long abbEncryption(unsigned long lKey, unsigned long rKey);
#endif

#ifdef PROTOCOL_MODUBUS_GROUP
 INT8U processAssData(INT8U arrayItem, INT8U *data,INT8U length);
 INT8U processHyData(INT8U arrayItem, INT8U *data,INT8U length);
 INT8U processXyData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processSwitchData(INT8U arrayItem, INT8U *data,INT8U length);
 INT8U processMwData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processJzData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processWe6301Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processPmc350Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
#endif


unsigned long rAbbMeterKey;       //ABB表远程Key
INT8U         abbHandclaspOk=0;
INT8U         abbClass0[40];      //ABB类0数据缓存
INT8U         abbClass2[104];     //ABB类2数据缓存




//算法二,查表法
unsigned char auchCRCHi[]=
{
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
  0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40
};

unsigned char auchCRCLo[] =
{
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
  0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
  0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
  0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
  0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
  0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
  0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
  0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
  0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
  0x40
};

unsigned int modbusCRC16(unsigned char *updata, unsigned char len)
{
  unsigned char uchCRCHi=0xff;
  unsigned char uchCRCLo=0xff;
  unsigned int  uindex;
  while(len--)
  {
    uindex=uchCRCHi^*updata++;
    uchCRCHi=uchCRCLo^auchCRCHi[uindex];
    uchCRCLo=auchCRCLo[uindex];
  }

  return (uchCRCHi<<8|uchCRCLo);
}


/*******************************************************
函数名称:initCopyProcess
功能描述:初始化抄表过程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE-初始化抄表成功
        FALSE-初始化抄表失败
*******************************************************/
BOOL initCopyProcess(struct meterCopyPara *cp)
{
  INT8U  arrayItem;     //数据下标
  INT8U  i;
  long   tmpData;
 
  if (cp->port>0 && cp->port<4)
  {
 	arrayItem = cp->port-1;
  }
  else
  {
 	if (cp->port>=31 && cp->port<=33)
 	{
 	  arrayItem = cp->port-28;
 	}
 	else
 	{
 	  return FALSE;
 	}
  }
	 
  //输入参数
  multiCopy[arrayItem].meterAddr[0] = cp->meterAddr[0];
  multiCopy[arrayItem].meterAddr[1] = cp->meterAddr[1];
  multiCopy[arrayItem].meterAddr[2] = cp->meterAddr[2];
  multiCopy[arrayItem].meterAddr[3] = cp->meterAddr[3];
  multiCopy[arrayItem].meterAddr[4] = cp->meterAddr[4];
  multiCopy[arrayItem].meterAddr[5] = cp->meterAddr[5];
  multiCopy[arrayItem].port = cp->port;
  multiCopy[arrayItem].protocol = cp->protocol;	 
  multiCopy[arrayItem].send = cp->send;
  multiCopy[arrayItem].save = cp->save;
  multiCopy[arrayItem].energy = cp->dataBuff;

  //if (arrayItem==3)  //载波端口
  if (arrayItem>=3)  //载波端口,ly,2012-01-09
  {
    switch(multiCopy[arrayItem].protocol)
    {
   	  case THREE_2007:
        multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_THREE_ENERGY_RECORD];	        
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
   	  	break;

   	  case THREE_LOCAL_CHARGE_CTRL_2007:
        multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_THREE_LOCAL_RECORD];	        
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
   	  	break;
   	  
   	  case THREE_REMOTE_CHARGE_CTRL_2007:
        multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_THREE_REMOTE_RECORD];	        
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
        break;
        
      case KEY_HOUSEHOLD_2007:
        multiCopy[arrayItem].reqAndReqTime = cp->dataBuff;
        multiCopy[arrayItem].paraVariable  = &cp->dataBuff[LENGTH_OF_KEY_ENERGY_RECORD];
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
      	break;
   	  
      default:
        multiCopy[arrayItem].reqAndReqTime = cp->dataBuff;
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
        break;
    }
  }
  else
  {
    if (multiCopy[arrayItem].protocol==KEY_HOUSEHOLD_2007)
    {
      multiCopy[arrayItem].reqAndReqTime = cp->dataBuff;
      multiCopy[arrayItem].paraVariable  = &cp->dataBuff[LENGTH_OF_KEY_ENERGY_RECORD];
      multiCopy[arrayItem].shiDuan       = cp->dataBuff;
    }
    else
    {
      multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_ENERGY_RECORD];
   
      multiCopy[arrayItem].paraVariable  = &cp->dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD];
      multiCopy[arrayItem].shiDuan       = &cp->dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD];
    }
  }

  //私有变量赋初值
  multiCopy[arrayItem].copyItem = 0;
  multiCopy[arrayItem].hasData = 0;
  multiCopy[arrayItem].recvFrameTail = 0;
  multiCopy[arrayItem].totalLenOfFrame = 2048;
  multiCopy[arrayItem].hasData = 0x0;
 
  multiCopy[arrayItem].copyDataType = cp->copyDataType;
  multiCopy[arrayItem].copyTime = cp->copyTime;
  multiCopy[arrayItem].pn = cp->pn;
	 
  switch(cp->protocol)
  {
	 	case DLT_645_1997:    //DL/T645-1997
	 	  if (cp->copyDataType==LAST_MONTH_DATA)
	 	  {
	 	  	multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTMONTH_645_1997;
	 	  }
	 	  else
	 	  {
	 	  	multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_CURRENT_645_1997;
	 	  }
	 	  break;
	 	  	
	 	case DLT_645_2007:
	 	  switch (cp->copyDataType)
	 	  {
	 	  	case LAST_MONTH_DATA:
	 	  	  multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTMONTH_645_2007;
	 	  	  break;
	 	  	    
	 	  	case PRESENT_DATA:
	 	  	  multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_CURRENT_645_2007;
	 	  	  break;
	 	  	  
	 	  	case LAST_DAY_DATA:
	 	  	  multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
	 	  	  break;
	 	  }
	 	  break;
 	  	
    case SINGLE_PHASE_645_1997:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_97;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_645_97;
  	  }
  	  break;

    case SINGLE_PHASE_645_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_07;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_645_07;
  	  }
  	  break;

    //2012-09-28
    case SINGLE_PHASE_645_2007_TARIFF:
  	  multiCopy[arrayItem].totalCopyItem = 2;
  	  break;

    case SINGLE_PHASE_645_2007_TOTAL:
  	  multiCopy[arrayItem].totalCopyItem = 2;
  	  break;

    case SINGLE_LOCAL_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_07;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_LOCAL_CTRL_07;
  	  }
  	  break;

    case SINGLE_REMOTE_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_07;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_REMOTE_CTRL_07;
  	  }
  	  break;

    case THREE_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_THREE__07;
  	  }
  	  break;

    case THREE_LOCAL_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_THREE_LOCAL_CTRL_07;
  	  }
  	  break;

    case THREE_REMOTE_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
  	  }
  	  else
   	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_THREE_REMOTE_CTRL_07;
  	  }
  	  break;

    case KEY_HOUSEHOLD_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_KEY_2007;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_KEY_2007;
  	  }
  	  break;
  	
    case DOT_COPY_1997:
    case DOT_COPY_2007:
  	  multiCopy[arrayItem].totalCopyItem = 4;
  	  break;
  	
    case HOUR_FREEZE_2007:
  	  multiCopy[arrayItem].totalCopyItem = cp->dataBuff[0];
  	  for(i=0;i<multiCopy[arrayItem].totalCopyItem;i++)
      {
    		hourFreeze2007[i][3] = cp->dataBuff[i+1];

       #ifdef SHOW_DEBUG_INFO
    		printf("冻结数据时间=%d\n", hourFreeze2007[i][3]);
       #endif
      }
      memset(cp->dataBuff, 0xee, 64);
  	  break;

    case PN_WORK_NOWORK_1997:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = 0;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_PN_WORK_NOWORK_645_97;
  	  }
  	  break;

    case PN_WORK_NOWORK_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = 0;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_PN_WORK_NOWORK_645_07;
  	  }
  	  break;

   #ifdef PROTOCOL_EDMI_GROUP
	  case EDMI_METER:       //红相(EDMI)表
  	  if (cp->copyDataType==LAST_MONTH_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_LASTMONTH_EDMI;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_REAL_EDMI;
  	  }

	  	//这个电表地址如果是超过9位的表地址计算可能不准确
	  	tmpData = (cp->meterAddr[0]&0xf) + (cp->meterAddr[0]>>4&0xf)*10 
	    	      + (cp->meterAddr[1]&0xf)*100 + (cp->meterAddr[1]>>4&0xf)*1000
	      	    + (cp->meterAddr[2]&0xf)*10000 + (cp->meterAddr[2]>>4&0xf)*100000 
	        	  + (cp->meterAddr[3]&0xf)*1000000 + (cp->meterAddr[3]>>4&0xf)*10000000
	          	+ (cp->meterAddr[4]&0xf)*100000000 + (cp->meterAddr[4]>>4&0xf)*1000000000;

		  multiCopy[arrayItem].meterAddr[0] = tmpData>>24&0xff;
		  multiCopy[arrayItem].meterAddr[1] = tmpData>>16&0xff;
		  multiCopy[arrayItem].meterAddr[2] = tmpData>>8&0xff;
		  multiCopy[arrayItem].meterAddr[3] = tmpData&0xff;
		  multiCopy[arrayItem].meterAddr[4] = 0x0;
		  multiCopy[arrayItem].meterAddr[5] = 0x0;
  	  break;
   #endif
  
   #ifdef LANDIS_GRY_ZD_PROTOCOL
		case SIMENS_ZD_METER:   //兰吉尔(/西门子)ZD表
    case SIMENS_ZB_METER:   //兰吉尔(/西门子)ZB表
  	  multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_REAL_ZD;
  	  break;
   #endif
	  
   #ifdef PROTOCOL_ABB_GROUP
    case ABB_METER:       //ABB方表
  	  multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_REAL_ABB;
  	
  	  rAbbMeterKey = 0x0;
  	  abbHandclaspOk = 0x0;
  	  break;
   #endif

   #ifdef PROTOCOL_MODUBUS_GROUP

		case MODBUS_ASS:		  //爱森思MODBUS表
	  	multiCopy[arrayItem].totalCopyItem = 1;
	  	break;

		case MODBUS_HY:		    //山东和远MODBUS表
	  	multiCopy[arrayItem].totalCopyItem = 2;
	  	break;
	
		case MODBUS_XY_F:		  //上海贤业MODBUS多功能表
		case MODBUS_XY_UI:		//上海贤业MODBUS电压电流表
	  	multiCopy[arrayItem].totalCopyItem = 1;
	  	break;
	
		case MODBUS_XY_M:		  //上海贤业MODBUS电表模块
	  	multiCopy[arrayItem].totalCopyItem = 2;
	  	break;
	
		case MODBUS_SWITCH:		//MODBUS开关量模块
	  	multiCopy[arrayItem].totalCopyItem = 1;
	 	  break;
		
		case MODBUS_T_H:	    //MODBUS温湿度模块
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_MW_F:	    //成都明武MODBUS全功能表
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_MW_UI:	  //成都明武MODBUS电压电流表
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_JZ_F:	    //上海居正MODBUS全功能表
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_JZ_UI:	  //上海居正MODBUS电压电流表
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
		
		case MODBUS_PMC350_F:    //深圳中电PMC350三相数字电表
			multiCopy[arrayItem].totalCopyItem = 2;
			break;
		
		case MODBUS_WE6301_F:	   //威斯顿WE6301全功能表
			multiCopy[arrayItem].totalCopyItem = 3;
			break;
   #endif

    default:
  	  multiCopy[arrayItem].totalCopyItem = 0;
  	  return FALSE;
  	  break;
  }

  return TRUE;
}

/*******************************************************
函数名称:meterFraming
功能描述:抄表数据组帧发送
调用函数:
被调用函数:
输入参数:INT8U flag,标志 D1,新抄表项或是上一抄表项重试(D1=0-新抄表项,D1=1-上一抄表项重试)   
输出参数:
返回值：=0,已发送
        =1,本表抄完
        -1,异常
*******************************************************/
INT8S meterFraming(INT8U port,INT8U flag)
{
  INT8U     arrayItem;     //数据下标
  INT8U     frameTail;
  INT16U    i;
  INT8U     sendBuf[128];
  INT8U     tmpSendBuf[135];
  INT8U     *pParaData;
  INT32U    tmpData;
  DATE_TIME tmpTimeA, tmpTimeB, tmpTimeC;
  char      tmpStr[5];
	
  INT16U    crcResult;
  INT8U     edmiTail;
	
  //ABB
  unsigned long abbKey;
  INT16U    offsetReq, offsetReqTime;    //ABB计算需要
  INT16U    j;                           //ABB计算需要
  INT32U    tmpDatax;                    //ABB计算需要
  INT8U     foundLoc;                    //发现位置(ABB计算需要)
  
 #ifdef SHOW_DEBUG_INFO
  printf("meterFraming:抄表端口=%d\n", port);
 #endif
  
  if (port>0 && port<4)
  {
		arrayItem = port-1;
  }
  else
  {
		if (port>=31 && port<=33)
		{
	 	  arrayItem = port-28;
		}
		else
		{
	 	  return -1;
	 	}
  }

  if ((flag&0x2)==0x0)  //新抄表项
  {
		multiCopy[arrayItem].copyItem++;
  }
  else                  //重发
  {
    //重发时清除以前接收后未处理的数据(可能是接收出错)
    multiCopy[arrayItem].recvFrameTail = 0;
    multiCopy[arrayItem].totalLenOfFrame = 2048;
  }

frameingPoint:

  //本表已抄完?
  if (multiCopy[arrayItem].copyItem>multiCopy[arrayItem].totalCopyItem)
  {
    //if (port==31)
    if (port>=31 && port<=33)
    {
   	  //有采集到的当前电能量数据
   	  if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
   	  {
	 	    switch (multiCopy[arrayItem].protocol)
	 	    {
	 	  	  case SINGLE_PHASE_645_1997:
	 	  	  case SINGLE_PHASE_645_2007:
	 	      case SINGLE_PHASE_645_2007_TARIFF:
	 	      case SINGLE_PHASE_645_2007_TOTAL:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, SINGLE_PHASE_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
	 	        break;

	 	  	  case SINGLE_LOCAL_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, SINGLE_LOCAL_CTRL_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_SINGLE_LOCAL_RECORD);
	 	        break;

	 	  	  case SINGLE_REMOTE_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, SINGLE_REMOTE_CTRL_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_SINGLE_REMOTE_RECORD);
	 	        break;

	        case THREE_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, THREE_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_THREE_ENERGY_RECORD);
		        break;
		
	 	  	  case THREE_LOCAL_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, THREE_LOCAL_CTRL_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_THREE_LOCAL_RECORD);
	 	        break;

	 	  	  case THREE_REMOTE_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                  multiCopy[arrayItem].energy, THREE_REMOTE_CTRL_PRESENT, \
	 	                                   ENERGY_DATA, LENGTH_OF_THREE_REMOTE_RECORD);

	 	  	  case KEY_HOUSEHOLD_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, KEY_HOUSEHOLD_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_KEY_ENERGY_RECORD);
	 	        break;
	 	      
	 	    }
      }
   		 
      //有采集到的需量数据(当前)
      if (multiCopy[arrayItem].hasData&HAS_CURRENT_REQ)
      {
			 	switch (multiCopy[arrayItem].protocol)
			 	{
			 	  case THREE_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].reqAndReqTime, THREE_PRESENT, \
		                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
		        break;

			 	  case THREE_LOCAL_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].reqAndReqTime, THREE_LOCAL_CTRL_PRESENT, \
		                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
		        break;

			 	  case THREE_REMOTE_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].reqAndReqTime, THREE_REMOTE_CTRL_PRESENT, \
		                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
		        break;
        }
      }
      	
      //有采集到的参量及参变量数据(当前)
      if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
      {
			 	if (multiCopy[arrayItem].protocol==KEY_HOUSEHOLD_2007)
			 	{
			 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                             multiCopy[arrayItem].paraVariable, KEY_HOUSEHOLD_PRESENT, \
		                              PARA_VARIABLE_DATA, LENGTH_OF_KEY_PARA_RECORD);
        }
      }
           
	  	if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_ENERGY)
	  	{
	    	switch (multiCopy[arrayItem].protocol)
	 			{
			 	  case SINGLE_PHASE_645_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_PHASE_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_PHASE_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	}
					 	break;

		  		case SINGLE_LOCAL_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_LOCAL_CTRL_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_LOCAL_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	   multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_LOCAL_CTRL_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_LOCAL_RECORD);
					 	}
					 	break;

		  		case SINGLE_REMOTE_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_REMOTE_CTRL_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_REMOTE_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_REMOTE_CTRL_MONTH, \
					 	                               MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_REMOTE_RECORD);
					 	}
					 	break;
		 	 
		  		case THREE_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, THREE_DAY, \
					 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, THREE_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_ENERGY_RECORD);
					 	}
					 	break;

		  		case THREE_LOCAL_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, THREE_LOCAL_CTRL_DAY, \
					 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_LOCAL_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, THREE_LOCAL_CTRL_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_LOCAL_RECORD);
					 	}
					 	break;

		  		case THREE_REMOTE_CHARGE_CTRL_2007:
				  	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
				 	                             multiCopy[arrayItem].energy, THREE_REMOTE_CTRL_DAY, \
				 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_REMOTE_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, THREE_REMOTE_CTRL_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_REMOTE_RECORD);
					 	}
					 	break;
		 	 
		  		case KEY_HOUSEHOLD_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, KEY_HOUSEHOLD_DAY, \
					 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_KEY_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, KEY_HOUSEHOLD_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_KEY_ENERGY_RECORD);
					 	}
					 	break;
        }
	  	}
       
      //有采集到的需量数据(上一次日冻结数据)
      if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_REQ)
      {
	 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                             multiCopy[arrayItem].reqAndReqTime, DAY_BALANCE, \
	 	                               DAY_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
	 	    if (multiCopy[arrayItem].copyTime.day==0x1)
	 	    {
	 	      multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                             multiCopy[arrayItem].reqAndReqTime, MONTH_BALANCE, \
	 	                               MONTH_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
	 	    }
      }

      if (multiCopy[arrayItem].hasData>0)
      {
		    multiCopy[arrayItem].hasData = 0x0;
		    return COPY_COMPLETE_SAVE_DATA;
	  	}
	  	else
	  	{
	    	multiCopy[arrayItem].hasData = 0x0;
	 			return COPY_COMPLETE_NOT_SAVE;
	  	}
    }
    else
    {
	  	switch(multiCopy[arrayItem].protocol)
	  	{
	 			case SINGLE_PHASE_645_1997:
	 			case SINGLE_PHASE_645_2007:
				  //有采集到的当前电能量数据
				  if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
				  {
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_PHASE_PRESENT, \
					 	                               ENERGY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
				  }

				  //有采集到的上一日电能量数据
				  if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_ENERGY)
				  {
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_PHASE_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_PHASE_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	}
				  }
    
          if (multiCopy[arrayItem].hasData>0)
          {		 
	       		multiCopy[arrayItem].hasData = 0x0;
	       		return COPY_COMPLETE_SAVE_DATA;
       	  }
				  else
				  {
				    multiCopy[arrayItem].hasData = 0x0;
				 		return COPY_COMPLETE_NOT_SAVE;
				  }
			 	  break;
   		 	   
			 	case DLT_645_1997:
			 	case DLT_645_2007:
			 	case PN_WORK_NOWORK_1997:
			 	case PN_WORK_NOWORK_2007:
				  //有采集到的电能量数据(上月/上一结算日)
				  if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_ENERGY)
				  {
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                           multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
					 	                            POWER_PARA_LASTMONTH, LENGTH_OF_ENERGY_RECORD);
				  }
      
      	  //有采集到的需量及发生时间数据(上月或上一结算日)
          if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_REQ)
          {
			 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
			 	                             multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
			 	                               REQ_REQTIME_LASTMONTH, LENGTH_OF_REQ_RECORD);
		      }
		      
		      //有采集到的电能量数据(上一次日冻结数据)
		      if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_ENERGY)
		      {
			 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
			 	                               multiCopy[arrayItem].energy, DAY_BALANCE, \
			 	                                 DAY_FREEZE_COPY_DATA_M, LENGTH_OF_ENERGY_RECORD);
			 	    if (multiCopy[arrayItem].copyTime.day==0x1)
			 	    {
			 	       multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
			 	                                 multiCopy[arrayItem].energy, MONTH_BALANCE, \
			 	                                   MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_ENERGY_RECORD);
			 	    }
			    }
		   
	  		  //有采集到的需量数据(上一次日冻结数据)
	  		  if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_REQ)
	  		  {
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		  		 	                           multiCopy[arrayItem].reqAndReqTime, DAY_BALANCE, \
		  		 	                             DAY_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
		  		 	if (multiCopy[arrayItem].copyTime.day==0x1)
		  		 	{
		  		 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		  		 	                             multiCopy[arrayItem].reqAndReqTime, MONTH_BALANCE, \
		  		 	                               MONTH_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
		  		 	}
		  		}

	  		  //有采集到的电能量数据(当前)
	  		  if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
	  		  {
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
		                                         ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
		  		}
		      	   
      	  //有采集到的需量及发生时间数据(当前)
          if ((multiCopy[arrayItem].hasData&HAS_CURRENT_REQ))
          {
	      		multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
	                                       multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
	                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
          }
		           
      	  //有采集到的参量及参变量数据(当前)
          if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
          {
		    		//由于DLT_645_2007抄不到总断相次数和累计断相时间及最近一次断相时间,计算得到 10-07-05
		             
            if (multiCopy[arrayItem].protocol == DLT_645_2007)
            {
              //现采取的办法是用A,B,C相的次数和时间来加起来计为总断相次数和累计时间
           	  pParaData = multiCopy[arrayItem].paraVariable;
           	
           	  //计算总断相次数
           	  tmpData = 0;
           	  if (pParaData[PHASE_A_DOWN_TIMES]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[PHASE_A_DOWN_TIMES] | pParaData[PHASE_A_DOWN_TIMES+1]<<8 | pParaData[PHASE_A_DOWN_TIMES+2]<<16);
           	  }
           	  if (pParaData[PHASE_B_DOWN_TIMES]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[PHASE_B_DOWN_TIMES] | pParaData[PHASE_B_DOWN_TIMES+1]<<8 | pParaData[PHASE_B_DOWN_TIMES+2]<<16);
           	  }
           	  if (pParaData[PHASE_A_DOWN_TIMES]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[PHASE_C_DOWN_TIMES] | pParaData[PHASE_C_DOWN_TIMES+1]<<8 | pParaData[PHASE_C_DOWN_TIMES+2]<<16);
           	  }
           	  tmpData = hexToBcd(tmpData);
           	  pParaData[PHASE_DOWN_TIMES]   = tmpData&0xff;
           	  pParaData[PHASE_DOWN_TIMES+1] = tmpData>>8&0xff;
           	  pParaData[PHASE_DOWN_TIMES+2] = tmpData>>16&0xff;
    
           	  //计算累计断相时间
           	  tmpData = 0;
           	  if (pParaData[TOTAL_PHASE_A_DOWN_TIME]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_A_DOWN_TIME] | pParaData[TOTAL_PHASE_A_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_A_DOWN_TIME+2]<<16);
           	  }
           	  if (pParaData[TOTAL_PHASE_B_DOWN_TIME]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_B_DOWN_TIME] | pParaData[TOTAL_PHASE_B_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_B_DOWN_TIME+2]<<16);
           	  }
           	  if (pParaData[TOTAL_PHASE_B_DOWN_TIME]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_C_DOWN_TIME] | pParaData[TOTAL_PHASE_C_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_C_DOWN_TIME+2]<<16);
           	  }
           	  tmpData = hexToBcd(tmpData);
           	  pParaData[TOTAL_PHASE_DOWN_TIME] = tmpData;
           	  pParaData[TOTAL_PHASE_DOWN_TIME+1] = tmpData>>8;
           	  pParaData[TOTAL_PHASE_DOWN_TIME+2] = tmpData>>16;
    
           	  //最近一次断相起始时间
           	  if (pParaData[LAST_PHASE_A_DOWN_BEGIN]!=0xee && pParaData[LAST_PHASE_B_DOWN_BEGIN]!=0xee && pParaData[LAST_PHASE_C_DOWN_BEGIN]!=0xee)
           	  {
               	memcpy((INT8U *)&tmpTimeA,pParaData+LAST_PHASE_A_DOWN_BEGIN,6);
               	memcpy((INT8U *)&tmpTimeB,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	memcpy((INT8U *)&tmpTimeC,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	tmpTimeA = timeBcdToHex(tmpTimeA);
               	tmpTimeB = timeBcdToHex(tmpTimeB);
               	tmpTimeC = timeBcdToHex(tmpTimeC);
               	
               	if (compareTwoTime(tmpTimeA, tmpTimeB))       //B>A
               	{
               	  if (compareTwoTime(tmpTimeC, tmpTimeB))     //B>C 
               	  {
                    memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	  }
               	  else
               	  {
               	    if (compareTwoTime(tmpTimeB, tmpTimeC))   //C>B
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	    }
               	    else                                      //C=B
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	    }
               	  }
               	}
               	else                                       //B<=A
               	{
               	  if (compareTwoTime(tmpTimeB, tmpTimeA))  //A>B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeA))  //A>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_A_DOWN_BEGIN,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	    }
               	  }
               	  else                                      //A<=B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeB))  //B>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	    }
               	  }
               	}
              }         	
   
             	 
             	//最近一次断相结束时间
           	  if (pParaData[LAST_PHASE_A_DOWN_END]!=0xee && pParaData[LAST_PHASE_B_DOWN_END]!=0xee && pParaData[LAST_PHASE_C_DOWN_END]!=0xee)
           	  {
               	memcpy((INT8U *)&tmpTimeA,pParaData+LAST_PHASE_A_DOWN_END,6);
               	memcpy((INT8U *)&tmpTimeB,pParaData+LAST_PHASE_B_DOWN_END,6);
               	memcpy((INT8U *)&tmpTimeC,pParaData+LAST_PHASE_C_DOWN_END,6);
               	tmpTimeA = timeBcdToHex(tmpTimeA);
               	tmpTimeB = timeBcdToHex(tmpTimeB);
               	tmpTimeC = timeBcdToHex(tmpTimeC);
               	
               	if (compareTwoTime(tmpTimeA, tmpTimeB))       //B>A
               	{
               	  if (compareTwoTime(tmpTimeC, tmpTimeB))     //B>C 
               	  {
                    memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
               	  }
               	  else
               	  {
               	    if (compareTwoTime(tmpTimeB, tmpTimeC))   //C>B
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
               	    }
               	    else                                      //C=B
               	    {
                        memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
               	    }
               	  }
               	}
               	else                                       //B<=A
               	{
               	  if (compareTwoTime(tmpTimeB, tmpTimeA))  //A>B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeA))  //A>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_A_DOWN_END,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
               	    }
               	  }
               	  else                                      //A<=B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeB))  //B>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
               	    }
               	  }
               	}
              }
            }
		   
	      		multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
	                                        multiCopy[arrayItem].paraVariable, multiCopy[arrayItem].copyDataType, \
	                                          PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
	        }
		       		
          //有采集到的时段数据
          if (multiCopy[arrayItem].hasData&HAS_SHIDUAN)
          {
      			multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                       multiCopy[arrayItem].shiDuan, multiCopy[arrayItem].copyDataType, \
                                        SHI_DUAN_DATA, LENGTH_OF_SHIDUAN_RECORD);
          }

          if (multiCopy[arrayItem].hasData>0)
          {
	      		multiCopy[arrayItem].hasData = 0x0;
	      		return COPY_COMPLETE_SAVE_DATA;
      	  }
      	  else
      	  {
	      		multiCopy[arrayItem].hasData = 0x0;
	      		return COPY_COMPLETE_NOT_SAVE;
      	  }
      	  break;
      		
      	case EDMI_METER:
      	case SIMENS_ZD_METER:
      	case SIMENS_ZB_METER:
      	case ABB_METER:
	  		  //有采集到的电能量数据(上月/上一结算日)
	  		  if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_ENERGY)
	  		  {
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		  		 	                           multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
		  		 	                            POWER_PARA_LASTMONTH, LENGTH_OF_ENERGY_RECORD);
	  		  }
      
      	  //有采集到的需量及发生时间数据(上月或上一结算日)
          if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_REQ)
          {
			 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
			 	                               multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
			 	                                 REQ_REQTIME_LASTMONTH, LENGTH_OF_REQ_RECORD);
          }

  		    //有采集到的电能量数据(当前)
  		    if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
  		    {
	  		 	  //ABB方表要计算总示值
	  		 	  /*
	  		 	  if (multiCopy[arrayItem].protocol==ABB_METER)
	  		 	  {
		          offsetVision = POSITIVE_WORK_OFFSET;
		          for(i=0; i<4; i++)
		          {
		            sumDec = 0xeeeeee;
		            sumInt = 0xeeeeee;
		            for(j=1;j<5;j++)
		            {
		              if (multiCopy[arrayItem].energy[offsetVision+j*4]!=0xee)
		              {
		                if (sumDec==0xeeeeee)
		                {
		                  sumDec = bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4]);
		                }
		                else
		                {
		                  sumDec += bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4]);
		                }
		                
		                if (sumInt==0xeeeeee)
		                {
		                  sumInt = bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4+1] | multiCopy[arrayItem].energy[offsetVision+j*4+2]<<8 | multiCopy[arrayItem].energy[offsetVision+j*4+3]<<16);
		                }
		                else
		                {
		                	sumInt += bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4+1] | multiCopy[arrayItem].energy[offsetVision+j*4+2]<<8 | multiCopy[arrayItem].energy[offsetVision+j*4+3]<<16);
		                }
		              }
		            }
		            if (sumDec!=0xeeeeee)
		            {
		            	sumInt += sumDec/100;
		            	sumDec = sumDec%100;
		            	
		            	sumDec = hexToBcd(sumDec);
		            	sumInt = hexToBcd(sumInt);
		            	
		            	multiCopy[arrayItem].energy[offsetVision] = sumDec;
		            	multiCopy[arrayItem].energy[offsetVision+1] = sumInt&0xff;
		            	multiCopy[arrayItem].energy[offsetVision+2] = sumInt>>8&0xff;
		            	multiCopy[arrayItem].energy[offsetVision+3] = sumInt>>16&0xff;
		            }
		            
		            offsetVision += 36;
		          }
	  		 	  }
	  		 	  */
  		 	  
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                   multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
		                                    ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
  		    }

      	  //有采集到的需量及发生时间数据(当前)
          if ((multiCopy[arrayItem].hasData&HAS_CURRENT_REQ))
          {
				 	  //ABB方表要比较最大需量及发生时间
				 	  if (multiCopy[arrayItem].protocol==ABB_METER)
				 	  {
				 	  	offsetReq = REQ_POSITIVE_WORK_OFFSET;
				 	  	offsetReqTime = REQ_TIME_P_WORK_OFFSET;
				 	  	for (i=0; i<4; i++)
				 	    {
				 	    	tmpData = 0xeeeeeeee;
				 	    	foundLoc = 0xfe;
				 	    	for(j=1;j<5;j++)
				 	      {
				 	    	  if (multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3]!=0xee)
				 	    	  {
				 	    	    tmpDatax = multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3] | multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3+1]<<8 | multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3+2]<<16;
				 	    	    if (tmpDatax>tmpData || tmpData==0xeeeeeeee)
				 	    	    {
				 	    	  	  foundLoc = j;
				 	    	  	  tmpData = tmpDatax;
				 	    	    }
				 	    	  }
				 	      }
				 	      
				 	      if (foundLoc<5)
				 	      {
				 	      	memcpy(&multiCopy[arrayItem].reqAndReqTime[offsetReq], &multiCopy[arrayItem].reqAndReqTime[offsetReq+foundLoc*3], 3);
				 	      	memcpy(&multiCopy[arrayItem].reqAndReqTime[offsetReqTime], &multiCopy[arrayItem].reqAndReqTime[offsetReqTime+foundLoc*5], 5);
				 	      }
				 	      
				 	      offsetReq += 72;
				 	      offsetReqTime += 72;
				 	    }
				 	  }
		 	 
 		 	      multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                        multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
                                         REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
          }

      	  //有采集到的参量及参变量数据(当前)
          if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
          {
  		 	 		multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                       multiCopy[arrayItem].paraVariable, multiCopy[arrayItem].copyDataType, \
                                        PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
          }

          if (multiCopy[arrayItem].hasData>0)
          {		 
      		  multiCopy[arrayItem].hasData = 0x0;
      		    
      		  return COPY_COMPLETE_SAVE_DATA;
      	  }
      	  else
      	  {
      		  multiCopy[arrayItem].hasData = 0x0;
      		    
      		  return COPY_COMPLETE_NOT_SAVE;
      	  }

      	  break;
		   
       #ifdef PROTOCOL_MODUBUS_GROUP
		    case MODBUS_ASS:
		 		case MODBUS_HY:
				case MODBUS_XY_F:
		 		case MODBUS_XY_UI:
		 		case MODBUS_SWITCH:
		 		case MODBUS_XY_M:
				case MODBUS_T_H:
				case MODBUS_MW_F:
				case MODBUS_MW_UI:
				case MODBUS_JZ_F:
				case MODBUS_JZ_UI:
				case MODBUS_PMC350_F:
				case MODBUS_WE6301_F:
		   		//有采集到的电能量数据(当前)
		   		if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
		   		{
			 			multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
									    multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
										  	ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
		   		}

		   		//有采集到的参量及参变量数据(当前)
		   		if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
		   		{
      			multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                       multiCopy[arrayItem].paraVariable, multiCopy[arrayItem].copyDataType, \
                                          PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
			 
					  multiCopy[arrayItem].hasData = 0x0;
					  return COPY_COMPLETE_SAVE_DATA;
		   		}

		   		multiCopy[arrayItem].hasData = 0x0;
      	  return COPY_COMPLETE_NOT_SAVE;
		   
		   		break;
		   #endif
		
      }
   	}
  }
	
  frameTail = 0;
  switch(multiCopy[arrayItem].protocol)
  {
   #ifdef PROTOCOL_645_1997
 	  case DLT_645_1997:                   //DL/T645-1997
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_97
 	  case SINGLE_PHASE_645_1997:          //DL/T645-1997
   #endif
   #ifdef PROTOCOL_645_2007
 	  case DLT_645_2007:                   //DL/T645-2007
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_07
 	  case SINGLE_PHASE_645_2007:          //DL/T645-2007单相智能表
 	  case SINGLE_PHASE_645_2007_TARIFF:   //DL/T645-2007单相智能表(仅实时数据总及各费率)
 	  case SINGLE_PHASE_645_2007_TOTAL:    //DL/T645-2007单相智能表(仅实时数据总示值)
   #endif
   #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
 	  case SINGLE_LOCAL_CHARGE_CTRL_2007:  //DL/T645-2007单相本地费控表
   #endif
   #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
 	  case SINGLE_REMOTE_CHARGE_CTRL_2007: //DL/T645-2007单相远程费控表
   #endif
   #ifdef PROTOCOL_THREE_2007
 	  case THREE_2007:                     //DL/T645-2007三相智能表
   #endif
   #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
 	  case THREE_REMOTE_CHARGE_CTRL_2007:  //DL/T645-2007三相远程费控表
   #endif
   #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
 	  case THREE_LOCAL_CHARGE_CTRL_2007:   //DL/T645-2007三相本地费控表
   #endif
   #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
 	  case KEY_HOUSEHOLD_2007:             //DL/T645-2007重点用户
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_97
 	  case PN_WORK_NOWORK_1997:            //97表只抄正反向有无功电能示值
   #endif    
   #ifdef PROTOCOL_PN_WORK_NOWORK_07
 	  case PN_WORK_NOWORK_2007:            //07表只抄正反向有无功电能示值
   #endif
     
    case DOT_COPY_1997:                  //97点抄规约
    case DOT_COPY_2007:                  //07点抄规约
    case HOUR_FREEZE_2007:               //07整点冻结规约
    
   	 #if PROTOCOL_645_1997 || PROTOCOL_645_2007 || PROTOCOL_SINGLE_PHASE_97 || PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007 || PROTOCOL_KEY_HOUSEHOLD_2007

   	  sendBuf[0] = 0x68;
        
      //填写地址域
      sendBuf[1] = multiCopy[arrayItem].meterAddr[0];
      sendBuf[2] = multiCopy[arrayItem].meterAddr[1];
      sendBuf[3] = multiCopy[arrayItem].meterAddr[2];
      sendBuf[4] = multiCopy[arrayItem].meterAddr[3];
      sendBuf[5] = multiCopy[arrayItem].meterAddr[4];
      sendBuf[6] = multiCopy[arrayItem].meterAddr[5];
       
      sendBuf[7] = 0x68;
       
      //控制字段
      if (multiCopy[arrayItem].protocol==DLT_645_1997 | multiCopy[arrayItem].protocol==SINGLE_PHASE_645_1997 | multiCopy[arrayItem].protocol==DOT_COPY_1997 | multiCopy[arrayItem].protocol==PN_WORK_NOWORK_1997)
      {
        sendBuf[8] = READ_DATA_645_1997;
      }
      else
      {
        sendBuf[8] = READ_DATA_645_2007;       	  
      }

      //填写抄表项
      frameTail = 9;
      switch(multiCopy[arrayItem].copyDataType)
      {
       	case  PRESENT_DATA:    //当前数据抄表项
          switch (multiCopy[arrayItem].protocol)
          {
            case DLT_645_1997:
              sendBuf[frameTail++] = 0x2;             //长度
              sendBuf[frameTail++] = cmdDlt645Current1997[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI0
              sendBuf[frameTail++] = cmdDlt645Current1997[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI1
              break;

            case SINGLE_PHASE_645_1997:
              sendBuf[frameTail++] = 0x2;             //长度
              sendBuf[frameTail++] = single1997[multiCopy[arrayItem].copyItem-1][1]+0x33;               //DI0
              sendBuf[frameTail++] = single1997[multiCopy[arrayItem].copyItem-1][0]+0x33;               //DI1
              break;

            case PN_WORK_NOWORK_1997:
              sendBuf[frameTail++] = 0x2;             //长度
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork1997[multiCopy[arrayItem].copyItem-1][1]+0x33; //DI0
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork1997[multiCopy[arrayItem].copyItem-1][0]+0x33; //DI1
              break;
              
            case DLT_645_2007:
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][3]+0x33;     //DI0
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][2]+0x33;     //DI1            	
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI2
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI3
              break;
              
            case SINGLE_PHASE_645_2007:
            case SINGLE_LOCAL_CHARGE_CTRL_2007:
            case SINGLE_REMOTE_CHARGE_CTRL_2007:
            case SINGLE_PHASE_645_2007_TARIFF:    //仅实时数据总及各费率
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][3]+0x33;               //DI0
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][2]+0x33;               //DI1            	
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][1]+0x33;               //DI2
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][0]+0x33;               //DI3
              break;

            case SINGLE_PHASE_645_2007_TOTAL:     //仅实时数据总示值
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][3]+0x33;               //DI0
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][2]+0x33;               //DI1            	
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][1]+0x33;               //DI2
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][0]+0x33;               //DI3
              break;

            case PN_WORK_NOWORK_2007:
          	sendBuf[frameTail++] = 0x04;             //长度
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][3]+0x33; //DI0
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][2]+0x33; //DI1            	
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][1]+0x33; //DI2
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][0]+0x33; //DI3
              break;

            case THREE_2007:
            case THREE_LOCAL_CHARGE_CTRL_2007:
            case THREE_REMOTE_CHARGE_CTRL_2007:
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][3]+0x33;                //DI0
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][2]+0x33;                //DI1            	
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][1]+0x33;                //DI2
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][0]+0x33;                //DI3
              break;

            case KEY_HOUSEHOLD_2007:
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][3]+0x33;         //DI0
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][2]+0x33;         //DI1            	
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][1]+0x33;         //DI2
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][0]+0x33;         //DI3
              break;

            case DOT_COPY_2007:
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][3]+0x33;              //DI0
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][2]+0x33;              //DI1            	
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][1]+0x33;              //DI2
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][0]+0x33;              //DI3
              break;

            case DOT_COPY_1997:
              sendBuf[frameTail++] = 0x2;             //长度
              sendBuf[frameTail++] = dotCopy1997[multiCopy[arrayItem].copyItem-1][1]+0x33;              //DI0
              sendBuf[frameTail++] = dotCopy1997[multiCopy[arrayItem].copyItem-1][0]+0x33;              //DI1
              break;
              
            default:
            	return PROTOCOL_NOT_SUPPORT;
            	break;
          }
          break;
              
        case LAST_MONTH_DATA:  //上月数据抄表项
          switch (multiCopy[arrayItem].protocol)            
          {
            case DLT_645_1997:
            case PN_WORK_NOWORK_1997:
              sendBuf[frameTail++] = 0x2;             //长度
              sendBuf[frameTail++] = cmdDlt645LastMonth1997[multiCopy[arrayItem].copyItem-1][1]+0x33;//DI0
              sendBuf[frameTail++] = cmdDlt645LastMonth1997[multiCopy[arrayItem].copyItem-1][0]+0x33;//DI1
              break;
              
            case DLT_645_2007:
            case PN_WORK_NOWORK_2007:
              sendBuf[frameTail++] = 0x4;           //长度

              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][3]+0x33;//DI0
              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][2]+0x33;//DI1
              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][1]+0x33;//DI2
              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][0]+0x33;//DI3
              break;
              
            default:
            	return PROTOCOL_NOT_SUPPORT;
            	break;
          }
          break;

        case LAST_DAY_DATA:    //上一次日冻结数据
        	switch(multiCopy[arrayItem].protocol)
          {
            case SINGLE_PHASE_645_1997:
              sendBuf[frameTail++] = 0x2;             //长度
              sendBuf[frameTail++] = single1997LastDay[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI0
              sendBuf[frameTail++] = single1997LastDay[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI1
              break;
              
            case SINGLE_PHASE_645_2007:
            case SINGLE_LOCAL_CHARGE_CTRL_2007:
            case SINGLE_REMOTE_CHARGE_CTRL_2007:
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][3]+0x33;     //DI0
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][2]+0x33;     //DI1            	
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI2
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI3
              break;
              
            case DLT_645_2007:
            case THREE_2007:
            case THREE_LOCAL_CHARGE_CTRL_2007:
            case THREE_REMOTE_CHARGE_CTRL_2007:
            case KEY_HOUSEHOLD_2007:
              sendBuf[frameTail++] = 0x4;             //长度
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][3]+0x33;     //DI0
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][2]+0x33;     //DI1            	
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI2
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI3
              break;

            default:
            	return PROTOCOL_NOT_SUPPORT;
            	break;
          }
        	break;
          	
        case HOUR_FREEZE_DATA:
          sendBuf[frameTail++] = 0x4;             //长度
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][3]+0x33;              //DI0
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][2]+0x33;              //DI1            	
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][1]+0x33;              //DI2
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][0]+0x33;              //DI3
        	break;
      }
       
      //Checksum
      sendBuf[frameTail] = 0x00;
      for (i = 0; i < frameTail; i++)
      {
        sendBuf[frameTail] += sendBuf[i];
      }
         
      frameTail++;
      sendBuf[frameTail++] = 0x16;

      //if (port==31)
      //2013-12-24,青岛鼎信测试发现,B,C相的表帧前面加了4个FE,而鼎信要求不加,修改这个判断
      //           鼎信是3相同抄的,所以会出现端口31,32,33
      if (port>=31)
      {
        memcpy(tmpSendBuf, sendBuf, frameTail);
      }
      else  //485端口加4个0xfe
      {
       #ifdef DKY_SUBMISSION
        memcpy(tmpSendBuf, sendBuf, frameTail);
       #else
        memcpy(&tmpSendBuf[4], sendBuf, frameTail);
        tmpSendBuf[0] = 0xfe;
        tmpSendBuf[1] = 0xfe;
        tmpSendBuf[2] = 0xfe;
        tmpSendBuf[3] = 0xfe;
     
        frameTail += 4;
       #endif
      }
	    break;
	   #endif   //PROTOCOL_645_1997

	 #ifdef PROTOCOL_EDMI_GROUP 	 
	  case EDMI_METER:    //红相(EDMI)表
      //02 45 0C A3 76 06 00 00 00 00 FF FF 95 A8 03
 	    frameTail = 0;
 	    sendBuf[frameTail++] = EDMI_STX;
 	    sendBuf[frameTail++] = 0x45;    //标志位(表示可以采集一对多方式[RS485,RS422])
 	 
 	    //表地址,(4字节就是表序列号[如212039174]转化成16进制)
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[0];
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[1];
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[2];
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[3];
       
      //4个字节的源地址(可随意定义)
      sendBuf[frameTail++] = 0x00;
      sendBuf[frameTail++] = 0x00;
      sendBuf[frameTail++] = 0x00;
      sendBuf[frameTail++] = 0x00;
       
      //2个字节的重发序号
      sendBuf[frameTail++] = 0xFF;
      sendBuf[frameTail++] = 0xFF;

      if (multiCopy[arrayItem].copyDataType==LAST_MONTH_DATA && multiCopy[arrayItem].copyItem==TOTAL_COMMAND_LASTMONTH_EDMI)
      {
  	 	  sendBuf[frameTail++] = 'X';
  	 	  sendBuf[frameTail++] = 0x0;
      }
      else
      {
  	 	  switch (multiCopy[arrayItem].copyItem)
  	 	  {
          case 1:
           	break;
           	 
          case 2:    //登入命令
            //02 45 0C A3 76 06 00 00 00 00 FF FF 4C 45 44 4D 49 2C 49 4D 44 45 49 4D 44 45 00 97 0C 03
	 	 	      sendBuf[frameTail++] = 'L';
	 	 	      sendBuf[frameTail++] = 'E';
	 	 	      sendBuf[frameTail++] = 'D';
	 	 	      sendBuf[frameTail++] = 'M';
	 	 	      sendBuf[frameTail++] = 'I';
	 	 	      sendBuf[frameTail++] = ',';
	 	 	      sendBuf[frameTail++] = 'I';
	 	 	      sendBuf[frameTail++] = 'M';
	 	 	      sendBuf[frameTail++] = 'D';
	 	 	      sendBuf[frameTail++] = 'E';
	 	 	      sendBuf[frameTail++] = 'I';
	 	 	      sendBuf[frameTail++] = 'M';
	 	 	      sendBuf[frameTail++] = 'D';
	 	 	      sendBuf[frameTail++] = 'E';
	 	 	      sendBuf[frameTail++] = 0x0;
	 	 	      break;
  	 	 	 	 
  	 	    case TOTAL_COMMAND_REAL_EDMI:      //退出命令
  	 	 	    sendBuf[frameTail++] = 'X';
  	 	 	 		sendBuf[frameTail++] = 0x0;
  	 	 	 		break;
  	 	 	 	   
  	 	    default:
  	 	 	 		sendBuf[frameTail++] = 'R';
            if (multiCopy[arrayItem].copyDataType==LAST_MONTH_DATA)
            {
  	 	 	      sendBuf[frameTail++] = cmdEdmiLastMonth[multiCopy[arrayItem].copyItem-1][0];
  	 	 	      sendBuf[frameTail++] = cmdEdmiLastMonth[multiCopy[arrayItem].copyItem-1][1];
            }
            else
            {
  	 	 	      sendBuf[frameTail++] = cmdEdmi[multiCopy[arrayItem].copyItem-1][0];
  	 	 	      sendBuf[frameTail++] = cmdEdmi[multiCopy[arrayItem].copyItem-1][1];
  	 	 	    }
  	 	 	    break;
  	 	  }
  	  }
	 	 	 
 	 		crcResult = edmi_CRC_16(sendBuf, frameTail);
 	 
		 	sendBuf[frameTail++] = crcResult>>8;
		 	sendBuf[frameTail++] = crcResult;
		 	sendBuf[frameTail++] = EDMI_ETX;

 	 		edmiTail = 0;
 	 		for(i=0; i<frameTail;i++)
 	 		{
        if (i==0 || i==(frameTail-1))
        {
     	 		tmpSendBuf[edmiTail++] = sendBuf[i];
        }
        else
        {
			 	 	switch (sendBuf[i])
			 	 	{
			      case EDMI_STX:
		          case EDMI_ETX:
		          case EDMI_DLE:
		          case EDMI_XON:
		          case EDMI_XOFF:
		 	 	      tmpSendBuf[edmiTail++] = EDMI_DLE;
		 	 	      tmpSendBuf[edmiTail++] = sendBuf[i]|0x40;
		 	 	      break;
				 	 	 	 	   
			 	 	  default:
			 	 	    tmpSendBuf[edmiTail++] = sendBuf[i];
			 	 	    break;
			 	 	}
 	   		}
      }
      frameTail = edmiTail;
	    break;
   #endif

	  case SIMENS_ZD_METER:    //兰吉尔(/西门子)ZD表
	  case SIMENS_ZB_METER:    //兰吉尔(/西门子)ZB表
  	  switch (multiCopy[arrayItem].copyItem)
  	  {
	      case 1:    //请求消息
          //2F 3F 38 34 32 38 35 36 37 34 21 0D 0A(/?84285674!)
 	        frameTail = 0;
 	     
 	        sendBuf[frameTail++] = '/';
 	        sendBuf[frameTail++] = '?';
 	        for(i=6;i>0;i--)
 	        {
 	     	    if (multiCopy[arrayItem].meterAddr[i-1]!=0x00)
 	     	    {
 	     	      sprintf(tmpStr,"%02d", bcdToHex(multiCopy[arrayItem].meterAddr[i-1]));
 	     	      sendBuf[frameTail++] = tmpStr[0];
 	     	      sendBuf[frameTail++] = tmpStr[1];
 	     	 		}
 	        }
 	        sendBuf[frameTail++] = '!';
 	        sendBuf[frameTail++] = 0x0D;
 	        sendBuf[frameTail++] = 0x0A;
 	     
 	        memcpy(tmpSendBuf, sendBuf, frameTail);
 	        break;
 	   
 	      case 2:    //选择消息
 	   	    //06 30 35 30 0D 0A(ACK  0  5  0)
 	   	    tmpSendBuf[0] = 0x06;
 	   	    tmpSendBuf[1] = 0x30;
 	   	    tmpSendBuf[2] = 0x35; //9600
 	   	    tmpSendBuf[3] = 0x30;
 	   	    tmpSendBuf[4] = 0x0D;
 	   	    tmpSendBuf[5] = 0x0A;

          multiCopy[arrayItem].send(multiCopy[arrayItem].port, tmpSendBuf, 6);
 	   	 
 	   	    return CHANGE_METER_RATE;
 	   	    break;
 	    }
 	 
 	    break;

	 #ifdef PROTOCOL_ABB_GROUP

	  case ABB_METER:         //ABB方表
	    frameTail = 0;

 	    switch (multiCopy[arrayItem].copyItem)
 	    {
	      case 1:    //握手
	      case 2:    //握手
	      case 3:    //握手
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x18;
	        sendBuf[frameTail++] = 0x06;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x01;
	        sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[0];
	        break;
	     
	      case 4:    //密码校验命令
	        if (abbHandclaspOk==0)
	        {
	       	  multiCopy[arrayItem].copyItem = TOTAL_COMMAND_REAL_ABB+1;
	       	  
	       	  goto frameingPoint;
	        }
	       
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x18;
	        sendBuf[frameTail++] = 0x01;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x04;
	     	 
          //rAbbMeterKey = 0x04367435;
	     	 
     	    abbKey = abbEncryption(0x0, rAbbMeterKey);
     	 
     	    sendBuf[frameTail++] = abbKey>>24;
     	    sendBuf[frameTail++] = abbKey>>16;
     	    sendBuf[frameTail++] = abbKey>>8;
     	    sendBuf[frameTail++] = abbKey&0xff;
     	    break;
	     	 
	      case 5:  //02 05 00 00 00 00 00 00 F6 01
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        break;
	     	 
	      case 6:    //02 05 00 00 00 00 00 02 xx xx 
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;	 	       
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;	 	       
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x02;	 	       
	        sendBuf[frameTail++] = 0x02;
	        break;
	       
	      case 7:
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x81;
	        break;

	      case 8:    //02 05 00 00 00 00 00 09 67 28 
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x09;
	        break;

	      case 9:    //02 05 00 00 00 00 00 0B 47 6A
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x0B;
	        break;
	     
	      case 10:
	      case 11:
	      case 12:
	      case 13:
	      case 14:
	      case 15:
	      case 16:
	      case 17:
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x81;
	        break;
	     
	      case 18:    //结束
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x80;
	       
	        multiCopy[arrayItem].copyItem++;
	        break;
 	    }
	   
	    crcResult = CRC_ABB(sendBuf, frameTail);
	    sendBuf[frameTail++] = crcResult>>8;
	    sendBuf[frameTail++] = crcResult&0xff;
	 	 
	    memcpy(tmpSendBuf, sendBuf, frameTail);
	 	 
	    if (multiCopy[arrayItem].copyItem==TOTAL_COMMAND_REAL_ABB+1)
	    {
	 	    multiCopy[arrayItem].send(multiCopy[arrayItem].port,tmpSendBuf,frameTail);
	 	 	 
	 	    goto frameingPoint;
	    }
	    break;
	 #endif

	 #ifdef PROTOCOL_MODUBUS_GROUP
	  case MODBUS_HY:		  //和远modbus表
	    tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	  //地址
	    tmpSendBuf[1] = 0x03;    //命令
	    if (1==multiCopy[arrayItem].copyItem)    //参变量
	    {
	      tmpSendBuf[2] = 0x10;    //寄存器地址
	      tmpSendBuf[3] = 0x00;    //
	      tmpSendBuf[4] = 0x00;    //寄存器个数
	      tmpSendBuf[5] = 0x1f;    //
	    }
	    else
	    {
		    tmpSendBuf[2] = 0x40;	//寄存器地址
		    tmpSendBuf[3] = 0x00;	//
		    tmpSendBuf[4] = 0x00;	//寄存器个数
		 		tmpSendBuf[5] = 0x08;	//	   
	    }
	
	    tmpData = modbusCRC16(tmpSendBuf, 6);
	    tmpSendBuf[6] = tmpData>>8;
	   	tmpSendBuf[7] = tmpData&0xff;
	   	frameTail = 8;
	   	break;
	
	  case MODBUS_ASS:		   //爱森思modbus表
	   	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);    //地址
	   	tmpSendBuf[1] = 0x03;    //命令
	   	tmpSendBuf[2] = 0x01;    //寄存器地址
	   	tmpSendBuf[3] = 0x00;    //
	   	tmpSendBuf[4] = 0x00;    //寄存器个数
	   	tmpSendBuf[5] = 0x22;    //

	   	tmpData = modbusCRC16(tmpSendBuf, 6);
	   	tmpSendBuf[6] = tmpData>>8;
	   	tmpSendBuf[7] = tmpData&0xff;
	   	frameTail = 8;
	   	break;
	   
	  case MODBUS_XY_F: 		 //上海贤业modbus多功能表
		 	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
		  tmpSendBuf[1] = 0x03;	  //命令
		 	tmpSendBuf[2] = 0x00;	  //寄存器地址
		 	tmpSendBuf[3] = 0x06;	  //
		 	tmpSendBuf[4] = 0x00;	  //寄存器个数
		 	tmpSendBuf[5] = 0x38;	  //
	   	
		 	tmpData = modbusCRC16(tmpSendBuf, 6);
		 	tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
		 	frameTail = 8;
		 	break;
		 
	 	case MODBUS_XY_UI:		 //上海贤业modbus电压电流表
		 	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
		 	tmpSendBuf[1] = 0x03;	  //命令
		 	tmpSendBuf[2] = 0x00;	  //寄存器地址
		 	tmpSendBuf[3] = 0x17;	  //
		 	tmpSendBuf[4] = 0x00;	  //寄存器个数
		 	tmpSendBuf[5] = 0x0c;	  //
	   
		 	tmpData = modbusCRC16(tmpSendBuf, 6);
		 	tmpSendBuf[6] = tmpData>>8;
		 	tmpSendBuf[7] = tmpData&0xff;
		 	frameTail = 8;
		 	break;
	   
	  case MODBUS_SWITCH: 	 //modbus开关量模块
		 	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
		 	tmpSendBuf[1] = 0x02;	  //命令
		 	tmpSendBuf[2] = 0x00;	  //寄存器地址
		 	tmpSendBuf[3] = 0x00;	  //
		 	tmpSendBuf[4] = 0x00;	  //寄存器个数
		 	tmpSendBuf[5] = 0x10;	  //
	   
		 	tmpData = modbusCRC16(tmpSendBuf, 6);
		 	tmpSendBuf[6] = tmpData>>8;
		 	tmpSendBuf[7] = tmpData&0xff;
		 	frameTail = 8;
		 	break;
		 
	 	case MODBUS_XY_M:		  //上海贤业modbus电表模块
	   	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);   //地址
	   	tmpSendBuf[1] = 0x03;	//命令
	   	if (1==multiCopy[arrayItem].copyItem)	//第一部分,寄存器个数最多限制为25个,分2次发送
	   	{
		 		tmpSendBuf[2] = 0x00;	  //寄存器地址
		 		tmpSendBuf[3] = 0x0A;	  //
		 		tmpSendBuf[4] = 0x00;	  //寄存器个数
		 		tmpSendBuf[5] = 0x18;	  //
	   	}
	   	else
	   	{
		 		tmpSendBuf[2] = 0x00;	//寄存器地址
		 		tmpSendBuf[3] = 0x22;	//
		 		tmpSendBuf[4] = 0x00;	//寄存器个数
		 		tmpSendBuf[5] = 0x18;	//	   
	   	}
	 
	   	tmpData = modbusCRC16(tmpSendBuf, 6);
	   	tmpSendBuf[6] = tmpData>>8;
	   	tmpSendBuf[7] = tmpData&0xff;
	   	frameTail = 8;
	   	break;
			
		case MODBUS_MW_F: 	//成都明武modbus多功能表
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
			tmpSendBuf[1] = 0x03; 	//命令
			tmpSendBuf[2] = 0x10; 	//寄存器地址
			tmpSendBuf[3] = 0x00; 	//
			tmpSendBuf[4] = 0x00; 	//寄存器个数
			tmpSendBuf[5] = 0x4A; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_MW_UI:	//成都明武modbus电压电流表
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
			tmpSendBuf[1] = 0x03; 	//命令
			tmpSendBuf[2] = 0x10; 	//寄存器地址
			tmpSendBuf[3] = 0x02; 	//
			tmpSendBuf[4] = 0x00; 	//寄存器个数
			tmpSendBuf[5] = 0x14; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;

		case MODBUS_JZ_F: 	//上海居正modbus多功能表
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
			tmpSendBuf[1] = 0x03; 	//命令
			tmpSendBuf[2] = 0x01; 	//寄存器地址
			tmpSendBuf[3] = 0x30; 	//
			tmpSendBuf[4] = 0x00; 	//寄存器个数
			tmpSendBuf[5] = 0x2e; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_JZ_UI:	//上海居正modbus电压电流表
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
			tmpSendBuf[1] = 0x03; 	//命令
			tmpSendBuf[2] = 0x01; 	//寄存器地址
			tmpSendBuf[3] = 0x31; 	//
			tmpSendBuf[4] = 0x00; 	//寄存器个数
			tmpSendBuf[5] = 0x0b; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_WE6301_F: 	//威斯顿WE6301多功能表
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
			tmpSendBuf[1] = 0x03; 	//命令
			if (1==multiCopy[arrayItem].copyItem) 				//第一部分,电压电流功率因数
			{
				tmpSendBuf[2] = 0x03; 	//寄存器地址
				tmpSendBuf[3] = 0xe7; 	//
				tmpSendBuf[4] = 0x00; 	//寄存器个数
				tmpSendBuf[5] = 0x24; 	//
			}
			else if (2==multiCopy[arrayItem].copyItem)		//第二部分,功率
			{
				tmpSendBuf[2] = 0x03; 	//寄存器地址
				tmpSendBuf[3] = 0x83; 	//
				tmpSendBuf[4] = 0x00; 	//寄存器个数
				tmpSendBuf[5] = 0x18; 	//
			}
			else if (3==multiCopy[arrayItem].copyItem)		//第二部分,电量
			{
				tmpSendBuf[2] = 0x04; 	//寄存器地址
				tmpSendBuf[3] = 0x4b; 	//
				tmpSendBuf[4] = 0x00; 	//寄存器个数
				tmpSendBuf[5] = 0x0c; 	//
			}
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_PMC350_F: 	//深圳中电PMC350多功能表
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //地址
			tmpSendBuf[1] = 0x03; 	//命令
			if (1==multiCopy[arrayItem].copyItem) 				//第一部分,电压电流功率功率因数
			{
				tmpSendBuf[2] = 0x00; 	//寄存器地址
				tmpSendBuf[3] = 0x00; 	//
				tmpSendBuf[4] = 0x00; 	//寄存器个数
				tmpSendBuf[5] = 0x3a; 	//
			}
			else if (2==multiCopy[arrayItem].copyItem)		//第二部分,电量
			{
				tmpSendBuf[2] = 0x01; 	//寄存器地址
				tmpSendBuf[3] = 0xf4; 	//
				tmpSendBuf[4] = 0x00; 	//寄存器个数
				tmpSendBuf[5] = 0x0c; 	//
			}
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
	 #endif

	  default:
	    return PROTOCOL_NOT_SUPPORT;
  }
  
  //发送数据
  multiCopy[arrayItem].send(multiCopy[arrayItem].port,tmpSendBuf,frameTail);
  
  return DATA_SENDED;
}

/*******************************************************
函数名称:meterReceiving
功能描述:抄表数据帧接收
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
INT8S meterReceiving(INT8U port,INT8U *data,INT16U recvLen)
{
  INT16U i,tmpi;
  INT8U  arrayItem;
  INT8U  checkSum;
  INT8S  ret=0;
   
  //for EDMI
  static char DLE_last;
  INT16U crcResult;
  
 #ifdef SHOW_DEBUG_INFO
  printf("meterReceiving:接收端口=%d,接收长度=%d\n", port, recvLen);
 #endif

  if (port>0 && port<4)
  {
  	arrayItem = port-1;
  }
  else
  {
  	if (port>=31 && port<=33)
  	{
  	 	arrayItem = port-28;
  	}
  	else
  	{
  		return FALSE;
  	}
  }
	 
  switch (multiCopy[arrayItem].protocol)
  {
   #ifdef PROTOCOL_645_1997
   	case DLT_645_1997:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_97
   	case SINGLE_PHASE_645_1997:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_97
  	case PN_WORK_NOWORK_1997:
   #endif
   #ifdef PROTOCOL_645_2007
   	case DLT_645_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_07
   	case SINGLE_PHASE_645_2007:
    case SINGLE_PHASE_645_2007_TARIFF:
   	case SINGLE_PHASE_645_2007_TOTAL:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_07
   	case PN_WORK_NOWORK_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
    case SINGLE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
    case SINGLE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_2007
    case THREE_2007:
   #endif
   #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
   	case THREE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
    case THREE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
    case KEY_HOUSEHOLD_2007:
   #endif
  	   
	  case DOT_COPY_2007:
	  case DOT_COPY_1997:
	  case HOUR_FREEZE_2007:
  	   	
  	 #if PROTOCOL_645_1997 || PROTOCOL_645_2007 || SINGLE_PHASE_645_1997 || SINGLE_PHASE_645_2007 || SINGLE_LOCAL_CHARGE_CTRL_2007 || SINGLE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007 || PROTOCOL_KEY_HOUSEHOLD_2007
     	for(i=0;i<recvLen;i++)
     	{
       	multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail++] = data[i];
       
       	if (multiCopy[arrayItem].recvFrameTail==1)
       	{
       	 	if (multiCopy[arrayItem].meterRecvBuf[0]!=0x68)
       	 	{
       	 		multiCopy[arrayItem].recvFrameTail   = 0;
       	 		multiCopy[arrayItem].totalLenOfFrame = 2048;
       	 	}
       	}
      
       	if (multiCopy[arrayItem].recvFrameTail==8)
       	{
         	if (multiCopy[arrayItem].meterRecvBuf[7]!=0x68)
         	{
       	    multiCopy[arrayItem].recvFrameTail   = 0;
       	    multiCopy[arrayItem].totalLenOfFrame = 2048;
       	    break;
         	}
       	}
       
       	if (multiCopy[arrayItem].recvFrameTail==10)
       	{
       	  multiCopy[arrayItem].totalLenOfFrame = multiCopy[arrayItem].meterRecvBuf[9]+12;
       	  
       	  //2012-07-10,add
       	  if (multiCopy[arrayItem].totalLenOfFrame>510)
       	  {
       	    multiCopy[arrayItem].recvFrameTail   = 0;
       	    multiCopy[arrayItem].totalLenOfFrame = 2048;
       	    break;           	  	
       	  }
       	}
       
       	if (multiCopy[arrayItem].recvFrameTail==multiCopy[arrayItem].totalLenOfFrame)
       	{
        	//计算校验和且按字节将数据域进行减0x33处理
         	checkSum = 0;
         	for(tmpi=0; tmpi<multiCopy[arrayItem].recvFrameTail-2; tmpi++)
         	{
           	checkSum += multiCopy[arrayItem].meterRecvBuf[tmpi];
            if (tmpi>9) // && flagOfForward == FALSE)
            {
              multiCopy[arrayItem].meterRecvBuf[tmpi] -= 0x33;  //按字节进行对数据域减0x33处理
            }
         	}
         
         	//如果校验和正确,执行meterInput操作
         	if (checkSum == multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-2])
         	{
            ret = meterInput(arrayItem, multiCopy[arrayItem].meterRecvBuf, multiCopy[arrayItem].recvFrameTail);
         	}
         	else
         	{
         	  ret = RECV_DATA_CHECKSUM_ERROR;   //接收数据校验和错误
         	}
         
         	multiCopy[arrayItem].totalLenOfFrame = 2048;
         	multiCopy[arrayItem].recvFrameTail = 0;
	      }
       	else
       	{
       	  ret = RECV_DATA_NOT_COMPLETE;  //接收数据不完整,继续等待接收
       	}
     	}
     	break;
     #endif
     
   #ifdef PROTOCOL_EDMI_GROUP
    case EDMI_METER:
     	ret = -7;
     	for(i=0; i<recvLen; i++)
     	{
       	multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail++] = data[i];
	
       	if (multiCopy[arrayItem].recvFrameTail==1)
       	{
       	 	if (multiCopy[arrayItem].meterRecvBuf[0]!=EDMI_STX)
       	 	{
       	   	multiCopy[arrayItem].recvFrameTail   = 0;
       	   	multiCopy[arrayItem].totalLenOfFrame = 2048;
       	 	  
       	   	continue;
       	 	}
       	}

       	if (DLE_last)
       	{
        	 multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-1] &= 0xBF;
         	DLE_last = FALSE;
       	}

       	if (data[i]==EDMI_DLE)
       	{
        	 multiCopy[arrayItem].recvFrameTail--;
       	 	DLE_last = TRUE;
       	}

       	if (data[i]==EDMI_ETX)
       	{
         	crcResult = edmi_CRC_16(multiCopy[arrayItem].meterRecvBuf,multiCopy[arrayItem].recvFrameTail-3);
         	if (((crcResult>>8)== multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-3])
         	    || ((crcResult&0xff)== multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-2])
         	   )
         	{
       	   	//printf("CRC OK\n");

       	   	ret = METER_NORMAL_REPLY;
           
           	//02 45 00 00 00 00 0C A3 76 06 FF FF 06 7F 44 03 
       	   	switch (multiCopy[arrayItem].meterRecvBuf[12])
       	   	{
       	   	 	case 0x06:
       	   	   	//printf("EDMI确认\n");
       	   	   	break;
       	   	 
       	   	 	case 0x18:
       	   	   	//printf("EDMI否认\n");
       	   	   	ret = METER_ABERRANT_REPLY;
       	   	   	break;
       	   	 
       	   	 	case 'R':
               	ret = meterInput(arrayItem, multiCopy[arrayItem].meterRecvBuf, multiCopy[arrayItem].recvFrameTail);
       	   	 	 	break;
       	   	}
       	 	}
       	 	else
       	 	{
          	ret = RECV_DATA_CHECKSUM_ERROR;   //接收数据校验和错误
       	 	}
       	 
       	 	multiCopy[arrayItem].recvFrameTail   = 0;
       	 	multiCopy[arrayItem].totalLenOfFrame = 2048;
       	}
       
       	if (multiCopy[arrayItem].recvFrameTail>510)
       	{
       	 	multiCopy[arrayItem].recvFrameTail   = 0;
       	}
     	}
   	 	break;
 	 #endif

   #ifdef LANDIS_GRY_ZD_PROTOCOL
    case SIMENS_ZD_METER:
    case SIMENS_ZB_METER:
     	switch (multiCopy[arrayItem].copyItem)
     	{
     	 	case 1:    //识别消息 2f 4c 47 5a 35 5c 32 5a 4d 44 34 30 35 34 34 30 37 2e 42 31 31 0d 0a /LGZ5\2ZMD4054407.B11
     	   	if (data[0]==0x2f && data[5]==0x5c && data[recvLen-2]==0x0d && data[recvLen-1]==0x0a)
     	   	{
     	 	   #ifdef SHOW_DEBUG_INFO
     	 	    data[recvLen] = '\0';
     	 	    printf("表计返回识别消息:%s", data);
     	 	   #endif
     	 	   ret = METER_REPLY_ANALYSE_OK;
     	   	}
     	   	break;
     	 
     	 	case 2:    //数据消息
	 	 	   	if (data[0]==0x02 && data[recvLen-5]==0x21 && data[recvLen-2]==0x03)
	 	 	   	{
	 	 	 	 		checkSum = data[1];
	 	 	 	 		for(i=2; i<recvLen-1; i++)
	 	 	 	 		{
	 	 	 	   		checkSum ^= data[i];
	 	 	 	 		}
	 	 	 	 
	 	 	 	 		if (checkSum = data[recvLen-1])
	 	 	 	 		{
	            ret = meterInput(arrayItem, data, recvLen);
	 	 	 	 		}
	 	 	 	 		else
	 	 	 	 		{
	 	 	 	  	 #ifdef SHOW_DEBUG_INFO
	 	 	 	   		printf("IEC1107数据消息BCC Error\n");
	 	 	 	  	 #endif

	 	 	 	   		ret = METER_NORMAL_REPLY;
	 	 	 	 		}
 	 	   		}
 	 	  		break;
 		  }
      break;
   #endif

   #ifdef PROTOCOL_ABB_GROUP
   	case ABB_METER:
   	 	crcResult = CRC_ABB(data, recvLen-2);       	 
   	 	//printf("ABB data len=%d,crcResult=%X,", recvLen, crcResult);
   	 	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
   	 	{
   	   	//printf("crc Ok\n");
   	   	
   	   	ret = METER_NORMAL_REPLY;
   	 
       	switch (multiCopy[arrayItem].copyItem)
       	{
     	   	case 1:    //02 ff 08 13 a1 00 00 02 10 03 51 14 64 5d 23
     	   	case 2:
     	   	case 3:
   	        if (data[0]==0x02 && recvLen==15)
   	        {
   	     	 	 	//远程密钥Key
   	     	 	 	rAbbMeterKey = data[9]<<24 | data[10]<<16 | data[11]<<8 | data[12];
   	     	 	 
   	     	 	 	//printf("rAbbMeterKey=%0x\n", rAbbMeterKey);
   	     	 	 	
   	     	 	 	multiCopy[arrayItem].copyItem = 3;
   	     	 	 	abbHandclaspOk = 1;
   	     	 	}
   	        break;
   	     
   	      case 4:    //02 18 00 A2 92 02
   	     	 	if (data[1]==0x18 && data[2]==0x00 && recvLen==6)
   	     	 	{
   	     	 		//printf("密码校验OK\n");
   	     	 	}
   	     	 	break;
   	     
   	      case 5:    //Class 0
   	     	 	//02 05 00 a2 a8 00 02 00 05 00 00 00 00 40 0f 0f 04 04 ff 00 01 00 00 01 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 90 7e 4b 
   	     	 	if (data[1]==0x5 && (data[4]&0x7f)==0x28)
   	     	 	{
   	     	 	 	memcpy(abbClass0, &data[5], data[4]&0x7f);
   	     	 	 
   	     	 	 	//printf("复制类0的数据\n");
   	     	 	}
   	     		break;
   	     	 
   	      case 6:    //Class-2前半部分
   	     	 	//02 05 00 a2 40 05 10 06 20 20 20 20 20 20 20 20 20 20 20 20 20 20 00 06 00 00 00 00 00 80 0c 00 00 01 00 0f 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 05 18 23 81 80 40 00 c0 0c 11 05 00 e4 00 00 00 00 bc 9a 
   	     	 	if (data[1]==0x5 && (data[4]&0x7f)==0x40)
   	     	 	{
   	     	 		memcpy(abbClass2, &data[5], data[4]&0x7f);
   	     	 	 
   	     	 	 	//printf("复制类2的前半部分数据\n");
   	     	 	}
   	     	 	break;

   	      case 7:    //Class-2后半部分
   	     	 	//02 81 00 a2 a8 00 00 31 00 06 00 00 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 00 00 00 00 00 09 80 00 00 00 00 00 b9 05 18 d9 91 
   	     		if (data[1]==0x81 && (data[4]&0x7f)==0x28)
   	     	 	{
   	     	 	 	memcpy(&abbClass2[64], &data[5], data[4]&0x7f);
   	     	 	 
   	     	 	 	//printf("复制类2的后半部分数据\n");
   	     	 	}	
   	     	 	break;
   	     
   	      case 9:
   	     	 	if (data[1]==0x05 && (data[4]&0x7f)==0x2a)
   	     	 	{
   	     	 	 	ret = processAbbData(arrayItem, &data[5], data[4]&0x7f, multiCopy[arrayItem].copyItem-9);
   	     	 	}
   	     	 	break;

   	      case 10:
   	      case 11:
   	      case 12:
   	      case 13:
   	      case 14:
   	      case 15:
   	      case 16:
   	      case 17:
   	     		if (data[1]==0x81)
   	     		{
   	     	 	 	ret = processAbbData(arrayItem, &data[5], data[4]&0x7f, multiCopy[arrayItem].copyItem-9);
   	     	 	}
   	     	 	break;
   	   	}
   	 	}
   	 	else
   	 	{
   	 	 	printf("CRC Error\n");
   	 	}
     	break;
   #endif

   #ifdef PROTOCOL_MODUBUS_GROUP
	  case MODBUS_HY:
     	crcResult = modbusCRC16(data, recvLen-2);		 
     	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
     	{
	   		printf("modbus hy crc Ok\n");
 
	   		ret = processHyData(arrayItem, &data[3], recvLen-5);
     	}
     	break;

	  case MODBUS_ASS:
			crcResult = modbusCRC16(data, recvLen-2);		   
		 	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
		 	{
		  	printf("modbus ass crc Ok\n");

		   	ret = processAssData(arrayItem, &data[3], recvLen-5);
		 	}
	    break;
		 
	  case MODBUS_XY_F:
	  case MODBUS_XY_UI:
	  case MODBUS_XY_M:
	    crcResult = modbusCRC16(data, recvLen-2);		 
	    if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
	    {
		   	printf("modbus XY crc Ok\n");
	 
		   	ret = processXyData(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
	    }
	    break;
		 
	  case MODBUS_SWITCH:
		 	crcResult = modbusCRC16(data, recvLen-2);	   
		 	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
		 	{
		 		printf("modbus switch crc Ok\n");

				ret = processSwitchData(arrayItem, &data[3], recvLen-5);
		 	}
		 	break;
			
		case MODBUS_MW_F:
		case MODBUS_MW_UI:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
				printf("modbus MW crc Ok\n");
		
				ret = processMwData(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;

		case MODBUS_JZ_F:
		case MODBUS_JZ_UI:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
				printf("modbus JZ crc Ok\n");
		
				ret = processJzData(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;
			
		case MODBUS_WE6301_F:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
       #ifdef SHOW_DEBUG_INFO
				printf("modbus WE6301 crc Ok\n");
			 #endif
		
				ret = processWe6301Data(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;
			
		case MODBUS_PMC350_F:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
       #ifdef SHOW_DEBUG_INFO
				printf("modbus PMC350 crc Ok\n");
       #endif		
				ret = processPmc350Data(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;

   #endif
	 
   #ifdef PROTOCOL_WASION_GROUP
    case PROTOCOL_WASION_GROUP:
      //sendDebugFrame(uart1RecvBuf, recvLen);
      break;
   #endif
     
    default:
      ret = PROTOCOL_NOT_SUPPORT;
      break;
  }
   
  return ret;
}

/***************************************************
函数名称:meterInput
功能描述:电能表数据输入
调用函数:
被调用函数:
输入参数:帧头指针及帧长度
输出参数:
返回值：void
***************************************************/
INT8S meterInput(INT8U arrayItem, INT8U *pFrameHead,INT16U frameLength)
{
 	struct recvFrameStruct frameAnalyse;   //帧分析
 	INT8U                  ctrlState;
 	INT8S                  ret;
 
 #ifdef PROTOCOL_WASION_GROUP
 	INT8U tmpAddr[6];
 	INT8U i;
 #endif   

 #ifdef SHOW_DEBUG_INFO
  INT16U j;
  printf("Rx:");
 	for(j=0;j<frameLength;j++)
 	{
   	printf("%02x ", pFrameHead[j]);
 	}
 	printf("\n");
 #endif
 
 	if (arrayItem>=3)
 	{
   	if (*(pFrameHead+1)==multiCopy[arrayItem].meterAddr[0] && *(pFrameHead+2)==multiCopy[arrayItem].meterAddr[1]
 	     	&& *(pFrameHead+3)==multiCopy[arrayItem].meterAddr[2] && *(pFrameHead+4)==multiCopy[arrayItem].meterAddr[3]
 	     	 && *(pFrameHead+5)==multiCopy[arrayItem].meterAddr[4] && *(pFrameHead+6)==multiCopy[arrayItem].meterAddr[5]
 	     )
   	{
 	   	//printf("收到的表地址与抄读的表地址相同\n");
   	}
   	else
   	{
 	   	//printf("收到的表地址与抄读的表地址不同\n");
 	  
 	   	return RECV_ADDR_ERROR;
   	}
 	}
 
 	//根据不同的协议调用不同函数处理帧
 	switch(multiCopy[arrayItem].protocol)
 	{
   #ifdef PROTOCOL_645_1997
    case DLT_645_1997:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_97
    case SINGLE_PHASE_645_1997:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_97
    case PN_WORK_NOWORK_1997:
   #endif
    case DOT_COPY_1997:

     #if PROTOCOL_645_1997 || PROTOCOL_SINGLE_PHASE_97
      if (*(pFrameHead + frameLength - 1) != 0x16)   //如果检查出帧在发送途中由错误发生，请求重发
      {
        return RECV_DATA_TAIL_ERROR;  //接收数据帧尾错误,丢弃
      }
      else
      {
        if (*(pFrameHead+8) >= 0xC0)
        {
          //这部分处理异常应答
          ctrlState = DATA_ERROR_645_1997;

          //copyContinue = TRUE;
          
          return METER_ABERRANT_REPLY;  //接收数据帧,但表端异常应答(可能是表端不持该命令)
        }
        else
        {
          ret = METER_NORMAL_REPLY;      //接收数据帧正常且表端正常应答(表端支持该命令)
        	
        	if ((*(pFrameHead+8) & 0xf) >= 4)
          {
            //这部分处理写数据、写地址、变更速率、变更密码的正常应答
          }
          else
          {
          	if ((*(pFrameHead+8) & FOLLOW_FRAME_645_1997) == FOLLOW_FRAME_645_1997)
            {
               ctrlState = FOLLOW_FRAME_645_1997;
            }
    
          	if ((*(pFrameHead+8) & FOLLOW_FRAME_645_1997) == LAST_FRAME_645_1997)
            {
               ctrlState = LAST_FRAME_645_1997;
            }
    
            //数据分类存储
            switch(*(pFrameHead + 11))
            {
            	case 0xB2:
            	case 0xB3:
            	case 0xB6:
            	case 0xC0:
            	case 0xC1:
                  //参数指向包含数据单元标识的参数数据项
                  ret = process645Para(arrayItem, pFrameHead+10, frameLength-14);
                  break;
            	  
            	case 0xC3:
            	case 0xC4:
            	  ret = processShiDuanData(arrayItem, pFrameHead+10, frameLength);
            	  break;
            	  	
            	default:
           	      ret = process645Data(arrayItem, pFrameHead+10, frameLength);
           	      break;
            }
          }
        }
      }
        
      if (ctrlState == FOLLOW_FRAME_645_1997)
      {
        //readFollowed(*pDataHead, *(pDataHead+1));
      }
      break;
     #endif //PROTOCOL_645_1997

   #ifdef PROTOCOL_645_2007
    case DLT_645_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_07
    case SINGLE_PHASE_645_2007:
    case SINGLE_PHASE_645_2007_TARIFF:
    case SINGLE_PHASE_645_2007_TOTAL:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_07
    case PN_WORK_NOWORK_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
    case SINGLE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
    case SINGLE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_2007
    case THREE_2007:
   #endif
   #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
    case THREE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
    case THREE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
    case KEY_HOUSEHOLD_2007:
   #endif
    
    case DOT_COPY_2007:
    case HOUR_FREEZE_2007:
      	
     #if PROTOCOL_645_2007 || PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007 || PROTOCOL_KEY_HOUSEHOLD_2007
      if (*(pFrameHead + frameLength - 1) != 0x16) //如果检查出帧在发送途中由错误发生，请求重发
      {
        return RECV_DATA_TAIL_ERROR;              //接收数据帧尾错误,丢弃
      }
      else
      {
        //帧分析结构赋值
        frameAnalyse.C     = *(pFrameHead+8);      //控制码
        frameAnalyse.L     = *(pFrameHead+9);      //数据域长度
        frameAnalyse.DI[0] = *(pFrameHead+10);     //DI0
        frameAnalyse.DI[1] = *(pFrameHead+11);     //DI1
        frameAnalyse.DI[2] = *(pFrameHead+12);     //DI2
        frameAnalyse.DI[3] = *(pFrameHead+13);     //DI3
        
        
        if ((frameAnalyse.C&ABERRANT_REPLY)==ABERRANT_REPLY)
        {
          //这部分处理异常应答
          return METER_ABERRANT_REPLY;            //接收数据帧,但表端异常应答(可能是表端不持该命令)
        }
        else
        {
          ret = METER_NORMAL_REPLY;      //接收数据帧正常且表端正常应答(表端支持该命令)
        	
          if (frameAnalyse.C&(NORMAL_REPLY|READ_DATA_645_2007))
          {
        		frameAnalyse.pData   = pFrameHead + 14;
        		frameAnalyse.loadLen = frameAnalyse.L - 4;
        		 
        		ret = process645Data2007(arrayItem,&frameAnalyse);
          }
        }
      }
      break;
     #endif //PROTOCOL_645_2007
    
   #ifdef PROTOCOL_EDMI_GROUP
    case EDMI_METER:
      ret = processEdmiData(arrayItem, &pFrameHead[13], frameLength-13-3);
      break;
   #endif

   #ifdef LANDIS_GRY_ZD_PROTOCOL
    case SIMENS_ZD_METER:
      ret = processLandisGryData(arrayItem, &pFrameHead[1], frameLength-6);
      break;
   #endif

   #ifdef PROTOCOL_WASION_GROUP
    case  PROTOCOL_WASION_GROUP:
     	tmpAddr[0] = *pFrameHead;
     	for (i = 1; i < 5; i++)
     	{
     	  tmpAddr[i] = 0;
     	}
     	measureObject = findMeasurePoint(tmpAddr);
     
     	if (*(pFrameHead+1) == 0xAA || *(pFrameHead+1) == 0xA5)  //查询数据应答命令
     	{
      	processWasionData(measureObject, pFrameHead);
     	}
     
     	if (*(pFrameHead+1) == 0xB8 || *(pFrameHead+2) == 0x6D) //瞬时数据打包查询应答
     	{
        processWasionPacket(measureObject, pFrameHead);
     	}
     
     	if (*(pFrameHead+1) == 0xC7 || *(pFrameHead+2) == 0x9A) //全部数据打包查询应答
     	{
     	  processWasionTotal(measureObject, pFrameHead);
     	}
     
     	break;
   #endif //PROTOCOL_WASION_GROUP
 }
 
 return ret;
}

INT32U twox[15]= {
	                50000000,
	                25000000,
	                12500000,
	                6250000,
	                3125000,
	                1562500,
	                781250,
	                390625,
	                195312,
	                97656,
	                48828,
	                24414,
	                12207,
	                6103,
	                3051
	               };

/***************************************************
函数名称:decodeIeee754Float
功能描述:IEEE754浮点数据解码
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8U decodeIeee754Float(INT8U *pData, INT8U *retValue)
{
	INT8U  sign=0;          //1-负数
	INT8U  exponent=0;      //阶
	INT32U mantissa;        //小数
	INT32U rawInt, rawDec;  //解码出来的整数、小数
	INT8U  i, j;
	INT32U checks;
	
	if (*pData&0x80)
	{
		sign = 1;
	}
	else
	{
		sign = 0;		
	}
	
	exponent = *pData<<1;
	if (*(pData+1)&0x80)
	{
		exponent |= 1;
	}
	
	mantissa = pData[3] | pData[2]<<8 | (pData[1]&0x7f)<<16 | 0x800000;
	
	rawInt = 0;
	rawDec = 0;
	if (exponent>=127)
	{
	  rawInt = mantissa>>(23-(exponent-127));
	}
	
	if (exponent>0)
	{
	  checks = 1<<(23-(exponent-127)-1);
	  //printf("小数位数=%d,checks=%x\n",23-(exponent-127),checks);
	  
	  for(i=23-(exponent-127),j=0; i>0 && j<15; i++,j++)
	  {
	  	if (mantissa&checks)
	  	{
	  		rawDec += twox[j];
	  	}
	  	checks>>=1;
	  }
	}

	retValue[4] = rawInt>>16;
	retValue[3] = rawInt>>8;
	retValue[2] = rawInt&0xff;
	
	retValue[1] = rawDec/1000000;
	retValue[0] = rawDec%1000000/10000;
  
 #ifdef SHOW_DEBUG_INFO
	printf("decodeIeee754Float:符号=%d,阶=%d,mantissa=%x,整数=%d,小数=%d\n", sign, exponent, mantissa,rawInt,rawDec);
 #endif
	
	return sign;
}


#ifdef PROTOCOL_645_1997

/***************************************************
函数名称:process645Data
功能描述:变量数据处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，pDataEnd数据段结束指针,
         measurePoint测量点
输出参数:
返回值：
***************************************************/
INT8S process645Data(INT8U arrayItem, INT8U *pDataHead,INT8U length)
{
  INT16U offset;
  
  //数据存储偏移量
  offset = findDataOffset(multiCopy[arrayItem].protocol, pDataHead);
  
 #ifdef SHOW_DEBUG_INFO
  printf("97规约电能量,需量%02X %02X 偏移:%02x\n",*(pDataHead+1), *pDataHead, offset);
 #endif
  
  if (offset == 0x200)
  {
    return RECV_DATA_OFFSET_ERROR;  //接收数据保存时偏移地址错误,未保存
  }
  
  //电能量、需量、需量时间数据
  if ((*pDataHead & 0xF) != 0xF)        //数据项处理
  {
    if ((*(pDataHead+1)&0xf0) == 0x90)  //电能量
    {
      if ((*(pDataHead+1) & 0xf) == 0x0a)
      {
       	multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
      }
      else
      {
        if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
        {
       	   multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;    //有当前电量数据
        }
        else
        {
       	   multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY; //有上月电量数据
        }
      }
       
    	*(multiCopy[arrayItem].energy+offset)   = *(pDataHead + 2);
    	*(multiCopy[arrayItem].energy+offset+1) = *(pDataHead + 3);
    	*(multiCopy[arrayItem].energy+offset+2) = *(pDataHead + 4);
    	*(multiCopy[arrayItem].energy+offset+3) = *(pDataHead + 5);
    }
    else                      //需量
    {
      if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
      {
        multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //有当前需量数据
      }
      else
      {
        multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //有上月需量数据
      }
       
      if ((*(pDataHead+1)&0xf0) == 0xA0)  //需量
      {
    	  *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead + 2);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 3);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 4);
      }
      else                  //需量时间
      {
    	  *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead + 2);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 3);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 4);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+3) = *(pDataHead + 5);
      }
    }
  }
  else   //数据块处理 数据标识xxxF
  {
    //length -= 15;
    //ly,2011-05-25,东软测试发现,如果数据块不带0xaa结束符的话,就没有最后一个费率的数据,改成-14应该可以适应所有
    length -= 14;

    //68 30 00 01 00 00 00 68 81 17 1F 94 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AA 8B 16

    if ((*(pDataHead+1)&0xf0) == 0x90)           //电能量
    {
     	if ((*(pDataHead+1) & 0xf) == 0x0a)
     	{
      	multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
     	}
     	else
     	{
       	if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
       	{
     	   	multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;    //有当前电量数据
       	}
       	else
       	{
      	 	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY; //有上月电量数据
       	}
     	}
     
     	pDataHead += 2;     //指向数据项
     	while (length>=4)
     	{
  	    *(multiCopy[arrayItem].energy+offset)   = *(pDataHead);
  	    *(multiCopy[arrayItem].energy+offset+1) = *(pDataHead + 1);
  	    *(multiCopy[arrayItem].energy+offset+2) = *(pDataHead + 2);
  	    *(multiCopy[arrayItem].energy+offset+3) = *(pDataHead + 3);
     	  
        offset += 4;
        pDataHead += 4;
        
        if (length<4)
        {
        	break;
        }
        else
        {
          length -= 4;
        }
     	}
    }
    else                           //需量及发生时间
    {
     	if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
     	{
     		multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //有当前需量数据
     	}
     	else
     	{
      	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //有上月电量数据
     	}
     
     	if ((*(pDataHead+1)&0xf0) == 0xA0)  //需量
     	{
       	pDataHead += 2;     //指向数据项
       	while (length>=3)
       	{
  	      *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 1);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 2);
  	      
  	      offset += 3;
          pDataHead += 3;
          
          if (length<3)
          {
          	break;
          }
          else
          {
            length -= 3;
          }
       	}
     	}
     	else                  //需量发生时间
     	{
       	pDataHead += 2;     //指向数据项
       	while (length>=4)
       	{
  	      *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 1);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 2);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+3) = *(pDataHead + 3);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+4) = 0x0;
     	    
     	    offset += 5;
          pDataHead += 4;
          
          if (length<4)
          {
          	break;
          }
          else
          {
            length -= 4;
          }
       	}
     	}
    }
  }
  
  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:process645Para
功能描述:参变量数据处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，pDataEnd数据段结束指针,
         measurePoint
输出参数:
返回值：
***************************************************/
INT8S process645Para(INT8U arrayItem, INT8U *pDataHead, INT8U len)
{
  INT16U    offset, tmpData;
  
  //数据存储偏移
  offset = findDataOffset(multiCopy[arrayItem].protocol,pDataHead);
  
 #ifdef SHOW_DEBUG_INFO
  printf("97规约%02X %02X,变量偏移:%02x\n",*(pDataHead+1), *pDataHead, offset);
 #endif
  
  if (offset == 0x200)
  {
  	return RECV_DATA_OFFSET_ERROR;  //接收数据保存时偏移地址错误,未保存
  }
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据
  
  switch(len)
  {
    case  1:
    	*(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
      break;

    case 2:
     	switch(offset)
     	{
     	  case VOLTAGE_PHASE_A:  //A,B,C相电压各2字节xxx,存储为xxx.0
     	  case VOLTAGE_PHASE_B:
     	  case VOLTAGE_PHASE_C:
          *(multiCopy[arrayItem].paraVariable+offset)   = (*(pDataHead+2))<<4;
          *(multiCopy[arrayItem].paraVariable+offset+1) = ((*(pDataHead+3))<<4) | ((*(pDataHead+2))>>4);
    	    break;
    	     
    	  case CURRENT_PHASE_A:  //A,B,C相电流各2字节xx.xx存储为0xx.xx0
    	  case CURRENT_PHASE_B:
    	  case CURRENT_PHASE_C:
          *(multiCopy[arrayItem].paraVariable+offset)   = (*(pDataHead+2))<<4;
          *(multiCopy[arrayItem].paraVariable+offset+1) = ((*(pDataHead+3))<<4) | ((*(pDataHead+2))>>4);
          *(multiCopy[arrayItem].paraVariable+offset+2) = (*(pDataHead+3))>>4;
    	   	break;
    	   	 
    	  case POWER_INSTANT_NO_WORK: //无功功率为2bytesxx.xx,存储为xx.xx00
    	  case POWER_PHASE_A_NO_WORK:
    	  case POWER_PHASE_B_NO_WORK:
    	  case POWER_PHASE_C_NO_WORK:
          *(multiCopy[arrayItem].paraVariable+offset)   = 0;
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+3);      	   	
    	   	break;

    	  case PHASE_DOWN_TIMES:      //断相次数为2bytesNNNN,存储为00NNNN
    	  case PHASE_A_DOWN_TIMES:
    	  case PHASE_B_DOWN_TIMES:
    	  case PHASE_C_DOWN_TIMES:
    	  case PROGRAM_TIMES:         //编程次数
    	  case UPDATA_REQ_TIME:       //需量清零次数
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);      	   	
          *(multiCopy[arrayItem].paraVariable+offset+2) = 0;
    	   	break;
    	   	 
    	  default:
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          break;
      }
      break;

    case 3:
      switch(offset)
      {
        case BATTERY_WORK_TIME:  //电池工作时间3bytes(NNNNNN),存储为4bytes(00NNNNNN)
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
          *(multiCopy[arrayItem].paraVariable+offset+3) = 0x0;
       	  break;
       	   
        default:
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
          break;
      }
      break;

    case 4:
     	switch(offset)
     	{
     	  case LAST_PHASE_DOWN_BEGIN:
     	  case LAST_PHASE_DOWN_END:
     	  case LAST_PHASE_A_DOWN_BEGIN:
     	  case LAST_PHASE_A_DOWN_END:
     	  case LAST_PHASE_B_DOWN_BEGIN:
     	  case LAST_PHASE_B_DOWN_END:
     	  case LAST_PHASE_C_DOWN_BEGIN:
     	  case LAST_PHASE_C_DOWN_END:
     	  case LAST_PROGRAM_TIME:     //最近一次编程时间
     	  case LAST_UPDATA_REQ_TIME:  //最近一次需量清零时间
          *(multiCopy[arrayItem].paraVariable+offset)   = 0x0;
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+3) = *(pDataHead+4);
          *(multiCopy[arrayItem].paraVariable+offset+4) = *(pDataHead+5);
          *(multiCopy[arrayItem].paraVariable+offset+5) = 0x0;
     	 	 	break;
     	 	  	
     	  default:
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
          *(multiCopy[arrayItem].paraVariable+offset+3) = *(pDataHead+5);
          break;
      }
      break;
       
    case 6:
       *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
       *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
       *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
       *(multiCopy[arrayItem].paraVariable+offset+3) = *(pDataHead+5);
       *(multiCopy[arrayItem].paraVariable+offset+4) = *(pDataHead+6);
       *(multiCopy[arrayItem].paraVariable+offset+5) = *(pDataHead+7);
       break;
  }

  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processShiDuanData
功能描述:时段参变量数据处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，pDataEnd数据段结束指针,
输出参数:
返回值：
***************************************************/
INT8S processShiDuanData(INT8U arrayItem, INT8U *pDataHead,INT8U length)
{   
  INT16U offset;
  
  multiCopy[arrayItem].hasData |= HAS_SHIDUAN; //有时段数据
  
  if (*(pDataHead+1)==0xc3)
  {
    offset = findDataOffset(multiCopy[arrayItem].protocol,pDataHead);
    
   #ifdef SHOW_DEBUG_INFO
    printf("97规约时段%02X %02X 偏移:%02x\n",*(pDataHead+1), *pDataHead, offset);
   #endif
  	
    if (offset == 0x200)
    {
  	 	return RECV_DATA_OFFSET_ERROR;  //接收数据保存时偏移地址错误,未保存
    }
    
    if ((*pDataHead>>4)==1)  //年时区数P等
    {
  	 	*(multiCopy[arrayItem].shiDuan+offset) = *(pDataHead+2);
    }
    else                     //时区及日时段
    {
      if ((*pDataHead & 0xF) != 0xF)  //数据项处理
      {
        offset += 3*((*pDataHead&0xf)-1);
       	
       	*(multiCopy[arrayItem].shiDuan+offset) = *(pDataHead + 2);
       	*(multiCopy[arrayItem].shiDuan+offset+1) = *(pDataHead + 3);
       	*(multiCopy[arrayItem].shiDuan+offset+2) = *(pDataHead + 4);
      }
      else   //数据块处理 数据标识xxxF
      {
        length -= 12;
     
        if ((*pDataHead>>4&0xf)>2)
        {
         	offset += 30*((*pDataHead>>4&0xf)-3);
        }
         
        pDataHead += 2;     //指向数据项
        while (length>=3)
        {
          *(multiCopy[arrayItem].shiDuan+offset)   = *pDataHead;
          *(multiCopy[arrayItem].shiDuan+offset+1) = *(pDataHead + 1);
          *(multiCopy[arrayItem].shiDuan+offset+2) = *(pDataHead + 2);
            
          offset += 3;              	  
          pDataHead += 3;
            
          if (*pDataHead==0xaa && *(pDataHead+2)==0x16)
          {
           	break;
          }
          if (length<3)
          {
            break;
          }
          else
          {
            length -= 3;
          }
        }
      }
  	}
  }
  else
  {
  	if (*(pDataHead+1)==0xc4)
  	{
		  if ((*pDataHead&0xf)==0xe)
		  {
		  	//shiDuanData[ZHOUXIURI_SHIDUAN] = *(pDataHead+2);
		  	
		  	*(multiCopy[arrayItem].shiDuan+ZHOUXIURI_SHIDUAN) = *(pDataHead+2);
		  }
		  else
		  {
		  	offset = JIA_RI_SHIDUAN+3*((*pDataHead&0xf)-1);
		  	
       	//shiDuanData[offset++] = *(pDataHead + 2);
       	//shiDuanData[offset++] = *(pDataHead + 3);
       	//shiDuanData[offset] = *(pDataHead + 4);    		  	 

       	*(multiCopy[arrayItem].shiDuan+offset)   = *(pDataHead + 2);
       	*(multiCopy[arrayItem].shiDuan+offset+1) = *(pDataHead + 3);
       	*(multiCopy[arrayItem].shiDuan+offset+2) = *(pDataHead + 4);    		  	 
		  }
  	}
  }

  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

#endif

#ifdef PROTOCOL_645_2007

/***************************************************
函数名称:process645Data2007
功能描述:07规约数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S process645Data2007(INT8U arrayItem,struct recvFrameStruct *frame)
{
  INT16U offset,offsetx;
  INT8U  i;
  
  //数据存储偏移量
  offset = findDataOffset(multiCopy[arrayItem].protocol, frame->DI);
  
 #ifdef SHOW_DEBUG_INFO
  printf("2007规约%02X %02X %02X %02X偏移:%02x\n", frame->DI[3],frame->DI[2],frame->DI[1],frame->DI[0], offset);
 #endif
  
  if (offset==0x200)
  {
  	return RECV_DATA_OFFSET_ERROR;  //接收数据保存时偏移地址错误,未保存
  }
  
  switch(frame->DI[3])
  {
    case ENERGY_2007:   //电能量
  	  if (frame->DI[0]==0x0)
  	  {
       	multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;     //有当前电量数据
      }
      else
      {
       	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY;  //有上一结算日电量数据
      }
       
  	  if (frame->DI[1]==0xff) //数据块
  	  {
        while (frame->loadLen>=4)
        {
       	  *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	  *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	  *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	  *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
 
          offset += 4;
          frame->pData += 4;
             
          if (frame->loadLen<4)
          {
            break;
          }
          else
          {
            frame->loadLen -= 4;
          }
        }
  	  }
  	  else             //数据项
  	  {
       	if (frame->DI[2]==0x90)
       	{
       	  *(multiCopy[arrayItem].paraVariable+offset)   = *(frame->pData);
       	  *(multiCopy[arrayItem].paraVariable+offset+1) = *(frame->pData + 1);
       	  *(multiCopy[arrayItem].paraVariable+offset+2) = *(frame->pData + 2);
       	  *(multiCopy[arrayItem].paraVariable+offset+3) = *(frame->pData + 3);         	 	 
       	}
       	else
       	{
       	  *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	  *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	  *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	  *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
       	}
  	  }
  	  break;
  	 	 
    case REQ_AND_REQ_TIME_2007:  //需量及发生时间
  	  if (frame->DI[0]==0x0)
  	  {
        multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //有当前需量数据
      }
      else
      {
       	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //有上月需量数据
      }

  	  if (frame->DI[1]==0xff) //数据块
  	  {
        offsetx = offset+27;
        while (frame->loadLen>=8)
        {
     	    //需量3bytes
     	    *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(frame->pData);
     	    *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(frame->pData + 1);
     	    *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(frame->pData + 2);
     	    
     	    //需量发生时间5bytes
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+0) = *(frame->pData + 3);  //分
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+1) = *(frame->pData + 4);  //时
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+2) = *(frame->pData + 5);  //日
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+3) = *(frame->pData + 6);  //月
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+4) = *(frame->pData + 7);  //年

          offset  += 3;
          offsetx += 5;
          frame->pData += 8;
           
          if (frame->loadLen<8)
          {
           	break;
          }
          else
          {
            frame->loadLen -= 8;
          }
        }
  	  }
  	  else             //数据项
  	  {
  	  }    	 	 
  	  break;
  	 
    case VARIABLE_2007:   //变量
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参变量数据
  	 	 
  	  if (frame->DI[1]==0xff)
  	  {
        switch(frame->DI[2])
        {
         	case 0x01:  //电压数据块(各2个字节xxx.x)
         	  for(i=0;i<6 && i<frame->loadLen;i++)
         	  {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
         	  }
         	  break;

         	case 0x02:  //电流数据块(各3个字节xxx.xxx)
         	  for(i=0;i<9 && i<frame->loadLen;i++)
         	  {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
         	  }
         	  break;

       	  case 0x03:  //瞬时有功功率数据块(各3个字节xx.xxxx)
       	  case 0x04:  //瞬时无功功率数据块(各3个字节xx.xxxx)
       	  case 0x05:  //瞬时视在功率数据块(各3个字节xx.xxxx)
       	  	for(i=0;i<12 && i<frame->loadLen;i++)
       	  	{
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	  	}
       	  	break;

       	  case 0x06:  //功率因数数据块(各2字节x.xxx)
       	  	for(i=0; i<8 && i<frame->loadLen; i++)
       	  	{
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	  	}
       	  	break;

       	  case 0x07:  //相角数据块(各2字节x.xxx)
       	  	for(i=0;i<6 && i<frame->loadLen; i++)
       	  	{
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);   //电压相角
              *(multiCopy[arrayItem].paraVariable+offset+6+i)   = *(frame->pData+i);   //电流相角
       	  	}
       	  	break;
       	  	
       	  default:
	 	        return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据           	  	
       	  	break;
        }           	  	
  	  }
  	  else
  	  {
       	if (frame->DI[2]==0x80)
       	{
          if (frame->DI[0]==0x01)  //零线电流(xxx.xxx!=97 2bytes xx.xx)
          {
       	    for(i=0;i<3 && i<frame->loadLen;i++)
       	    {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	    }
          }
          
       	  if (frame->DI[0]==0x0a)  //内部电池工作时间(4bytes)
       	  {
       	    for(i=0;i<4;i++)
       	    {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	    }
       	  }
       	}
  	  }
  	  break;
  	 	 
    case EVENT_RECORD_2007:
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参变量数据
       
	    switch(frame->DI[2])
	    {
	 	    case 0x04:    //A,B,C断相记录,断相次数和断相时间,etc...
 	      	switch(frame->DI[0])
 	     	 	{
 	 	      	case 0x00:  //A,B,C断相次数及总累计时间(次数和时间各3bytes,NNNNNN,3*2*3)
 	 	        	for(i=0;i<18;i++)
 	 	        	{
             	 	*(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
 	 	        	}
 	 	        	break;
 	 	    
 	 	   			case 0x01:  //A,B,C相断相记录(起始时间和结束时间各6bytes,ssmmhhDDMMYY)
			       	for(i=0;i<12;i++)
			       	{
			          *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	}
			       	break;
     	  
     	      default:
	            return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据
 	     		}
 	     		break;
      	 	 
			  case 0x30:   //编程,电表清零,需量清零,事件清零,校时,时段表修改记录
				 	switch(frame->DI[1])
				 	{
				 	  case 0x0:  //编程记录
				 	 	  switch(frame->DI[0])
				 	 	  {
				 	 	    case 0x00:  //编程次数
			 	          for(i=0;i<3;i++)
			 	          {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			 	          }
			 	          break;
			     	      
			     	    case 0x01:  //上一次编程记录
			     	      //仅保存上一次编程发生时间6bytes(ssmmhhDDMMYY) 
			     	      for(i=0;i<6;i++)
			     	      {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	      }
			     	      break;
			     	      
			     	    default:
			          	return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0x1:  //电表清零记录
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //电表清零总次数
			       	    for(i=0;i<3;i++)
			       	    {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	    }
			       	    break;
			       	      
			       	  case 0x01:  //上一次电表清零记录
			       	    //仅保存上一次电表清零发生时间6bytes(ssmmhhDDMMYY) 
			       	    for(i=0;i<6;i++)
			       	    {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	    }
			       	    break;
			       	      
			       	  default:
				         		return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	 	}
			    	 	break;
			    	 	 	   
				 	    case 0x2:  //需量清零记录
				 	     	switch(frame->DI[0])
				 	     	{
				 	       	case 0x00:  //需量清零总次数
			       	    for(i=0;i<3;i++)
			       	    {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	    }
			       	    break;
			           	      
			     	   	case 0x01:  //上一次需量清零记录
			     	     	//仅保存上一次需量清零发生时间6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0x3:  //事件清零记录
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //事件清零总次数
			     	     	for(i=0;i<3;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //上一次事件清零记录
			     	     	//仅保存上一次事件清零发生时间6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	  }
			    	 	break;

			    	case 0x4:  //校时记录
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //校时总次数
			     	     	for(i=0;i<3;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //上一次校时记录
			     	     	//仅保存上一次校时发生时间6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i+4);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0x5:  //时段编程记录
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //时段编程总次数
			     	      for(i=0;i<3;i++)
			     	      {
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //上一次时段编程记录
			     	     	//仅保存上一次时段编程发生时间6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0xd:  //电表尾盖打开记录
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //开表盖总次数
			     	      for(i=0;i<3;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //上一次开表盖发生时刻
			     	     	//仅保存上一次开表盖发生时间6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据               	    
			    	 	}
			    	 	break;
			    }
			    break;
			    	 	 
			  case 0x32:
				 	//上一次购电后累计购电量
				 	if (frame->DI[1]==0x06 && frame->DI[0]==0x01)
				 	{
			      for(i=0;i<4;i++)
			      {
			        *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			      }
				 	}
				 	break;
				 	 
			  case 0x33:
				 	//上一次购电后累计购电次数
				 	if (frame->DI[1]==0x02 && frame->DI[0]==0x01)
				 	{
			     	for(i=0;i<2;i++)
			     	{
			         *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	}
				 	}
				 	 
				 	//上一次购电后累计购电金额
				 	if (frame->DI[1]==0x06 && frame->DI[0]==0x01)
				 	{
			     	for(i=0;i<4;i++)
			     	{
			         *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	}
				 	}
				 	break;
			}
			break;
  	 	 
    case PARA_VARIABLE_2007:
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参变量数据

  	 	switch(frame->DI[2])
  	  {
  	   	case 0x0:
  	    	switch(frame->DI[1])
  	     	{
  	   	  	case 0x01:
  	         	if (frame->DI[0]==0x01)  //日期及星期
  	 	     		{
         	      for(i=0;i<4;i++)
         	      {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
         	      }
  	 	     		}
	  	 	     	if (frame->DI[0]==0x02)  //时间
	  	 	     	{
	           	  for(i=0;i<3;i++)
	           	  {
	                *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
	           	  }
	  	 	     	}
	  	 	     	break;
      	 	     
      	   	case 0x02:
      	 	 		//除了公共假日数本来是两个字节只存了一个字节外,其它的都按规约一个字节存储
              multiCopy[arrayItem].hasData |= HAS_SHIDUAN;       //有时段数据
              *(multiCopy[arrayItem].shiDuan+offset) = *frame->pData;
      	 	 		break;
      	 	   
      	   	case 0x04:        	 	 
      	 	 		if (frame->DI[0]==0x09 || frame->DI[0]==0x0a)  //电表常数(有功和无功)
      	 	 		{
             	 	for(i=0;i<3;i++)
             	  {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
             	  }
      	 	 		}
		      	 	if (frame->DI[0]==0x02)  //电表表号
		      	 	{
           	   	for(i=0;i<6;i++)
           	   	{
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
           	   	}
		      	 	}
		      	 	break;
      	 	     
      	   	case 0x05:        	 	 
      	 	 		if (frame->DI[0]==0xff)    //电表运行状态字
      	 	 		{
               	//本来是7个状态字(7*2bytes),但645-1997只有一个字节的状态字
             	 	for(i=0;i<14;i++)
             	  {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
             	  }
      	 	 		}
      	 	 		break;
      	 	     
      	   	case 0x0b:  //每月结算日(1,2,3结算日,各2个字节)
              //本来是3个结算日,但645-1997只有一个结算日
              *(multiCopy[arrayItem].paraVariable+offset) = *frame->pData;
              *(multiCopy[arrayItem].paraVariable+offset+1) = *(frame->pData+1);
      	 	 		break;
      	 	     
      	   	case 0x0f:
      	 	 		if (frame->DI[0]==0x04 || frame->DI[0]==0x01)    //透支电量限值 /报警电量1限值
      	 	 		{
             	  for(i=0;i<4;i++)
             	  {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
             	  }
      	 	 		}
      	 	  	break;
      	  }
      	 	break;
      	 
        case 0x01:   //第一套时区表数据及时段表数据
          multiCopy[arrayItem].hasData |= HAS_SHIDUAN;       //有时段数据
      	 	 
      	 	if (frame->DI[0]==0x00)  //第一套时区表数据
      	 	{
            while (frame->loadLen>=3)
            {
           	  *(multiCopy[arrayItem].shiDuan+offset)   = *(frame->pData);
           	  *(multiCopy[arrayItem].shiDuan+offset+1) = *(frame->pData + 1);
           	  *(multiCopy[arrayItem].shiDuan+offset+2) = *(frame->pData + 2);
     
              offset += 3;
              frame->pData += 3;
                 
              if (frame->loadLen<3)
              {
                break;
              }
              else
              {
                frame->loadLen -= 3;
              }
            }
      	 	}
      	 	else   //日时段表数据
      	 	{
         	  offset += 30*frame->DI[0];
      	 	 	 
      	   	for(i=0;i<10&&frame->loadLen>=3;i++)
      	   	{
           	 	*(multiCopy[arrayItem].shiDuan+offset)   = *(frame->pData);
           	 	*(multiCopy[arrayItem].shiDuan+offset+1) = *(frame->pData + 1);
           	 	*(multiCopy[arrayItem].shiDuan+offset+2) = *(frame->pData + 2);
     
              offset += 3;
              frame->pData += 3;
                
              if (frame->loadLen<3)
              {
                break;
              }
              else
              {
                frame->loadLen -= 3;
              }        	 	 	 	  
      	   	}
      	 	}
      	 	break;
      }
  	  break;
  	 	 
    case FREEZE_DATA_2007:   //冻结数据
  	 	if (multiCopy[arrayItem].protocol==SINGLE_PHASE_645_2007
  	 	  	|| multiCopy[arrayItem].protocol==SINGLE_LOCAL_CHARGE_CTRL_2007
  	 	  	 || multiCopy[arrayItem].protocol==SINGLE_REMOTE_CHARGE_CTRL_2007
  	 		 )
  	 	{
  	   	if (frame->DI[2]==0x6 && (frame->DI[1]==0x00 || frame->DI[1]==0x01 || frame->DI[1]==0x02))
  	   	{
       		multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
       	 
          if (frame->DI[1]==0x00)   //日冻结时标
          {
         	  *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
         	  *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
         	  *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
         	  *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
         	  *(multiCopy[arrayItem].energy+offset+4) = *(frame->pData + 4);
          }
          else
          {
            while (frame->loadLen>=4)
            {
         	    *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
         	    *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
         	    *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
         	    *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
   
              offset += 4;
              frame->pData += 4;
               
              if (frame->loadLen<4)
              {
                break;
              }
              else
              {
                frame->loadLen -= 4;
              }
            }
          }
        }
  	  }
  	  else
  	  {
  	   	//if (frame->DI[2]==0x6 && frame->DI[1]>=0x00 && frame->DI[1]<0x09 && frame->DI[0]==0x01)
  	   	if (frame->DI[2]==0x6 && frame->DI[1]<0x09 && frame->DI[0]==0x01)  //ly,2012-01-10,modify
  	   	{
       	 	multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //有上一次日冻结电量数据
       	 
          if (frame->DI[1]==0x00)        //日冻结时标
          {
       	   	*(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	   	*(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	   	*(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	   	*(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
       	   	*(multiCopy[arrayItem].energy+offset+4) = *(frame->pData + 4);
         	}
         	else
         	{
           	while (frame->loadLen>=4)
           	{
       	     	*(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	     	*(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	     	*(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	     	*(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
 
             	offset += 4;
             	frame->pData += 4;
             
             	if (frame->loadLen<4)
             	{
               	break;
             	}
             	else
             	{
                frame->loadLen -= 4;
             	}
           	}
         	}
       	}
        else
        {
          if (frame->DI[2]==0x6 && (frame->DI[1]==0x09 || frame->DI[1]==0x0a) && frame->DI[0]==0x01)
          {
            multiCopy[arrayItem].hasData |= HAS_LAST_DAY_REQ;     //有上一次日冻结需量数据

            offsetx = offset+27;
            while (frame->loadLen>=8)
            {
          	  //需量3bytes
          	  *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(frame->pData);
          	  *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(frame->pData + 1);
          	  *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(frame->pData + 2);
          	    
          	  //需量发生时间5bytes
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+0) = *(frame->pData + 3);  //分
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+1) = *(frame->pData + 4);  //时
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+2) = *(frame->pData + 5);  //日
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+3) = *(frame->pData + 6);  //月
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+4) = *(frame->pData + 7);  //年
    
              offset  += 3;
              offsetx += 5;
              frame->pData += 8;
                
              if (frame->loadLen<8)
              {
                break;
              }
              else
              {
                frame->loadLen -= 8;
              }
            }
     	 	 	}
     	 	 	else
     	 	 	{
            if (frame->DI[2]==0x4 && frame->DI[1]==0xff)
            {
              multiCopy[arrayItem].hasData |= HAS_HOUR_FREEZE_ENERGY;     //有整点冻结电能数据
         	     
       	     	//整点冻结时间
       	     	for(i=0;i<5;i++)
       	     	{
       	     	  *(multiCopy[arrayItem].energy+128+i)   = *(frame->pData+i);
       	     	}
       	     
       	     	frame->pData+=6;   //其中有一个0xaa为分隔符
       	     
       	     	//整点冻结正向有功电能示值
       	     	for(i=0;i<4;i++)
       	     	{
       	       	*(multiCopy[arrayItem].energy+i)   = *(frame->pData+i);
       	     	}

       	     	frame->pData+=5;   //其中有一个0xaa为分隔符
       	     
       	    	//整点冻结反向有功电能示值
       	     	for(i=0;i<4;i++)
       	     	{
       	       	*(multiCopy[arrayItem].energy+4+i)   = *(frame->pData+i);
       	     	}
       	     
       	     	//保存该整点冻结数据
		 	       	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		 	                             multiCopy[arrayItem].energy, HOUR_FREEZE, \
		 	                               0x0, LENGTH_OF_HOUR_FREEZE_RECORD);
            }
     	 		}
        }
  	 	}
  	 	break;
  	 	 
    case EXT_EVENT_RECORD_13:    //07备案文件断相统计数据
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参变量数据
  	 	if (frame->DI[1]==00)    //断相次数或断相累计时间
  	 	{
       	for(i=0;i<3;i++)
       	{
        	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
       	}
	 		}
	 		else                //最近一次断相起始时间及结束时间
	 		{
      	for(i=0;i<6;i++)
       	{
         	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
       	}
	 		}
	 		break;
  	 	 
    case 0x1d:
    case 0x1e:
  		if (frame->DI[2]==0x00 && frame->DI[1]==0x01 && frame->DI[0]==0x1)  //上一次跳/合闸发生时刻
  	 	{
        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参变量数据
         
        for(i=0;i<6;i++)
        {
          *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
        }
  	 	}
  	 	break;
  	 	     	 	
    default:
  	  return METER_NORMAL_REPLY; //接收数据帧正常且表端正常应答(表端支持该命令),但并不是本终端发出的命令而返回的数据
  	  break;
  }
  
  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

#endif   //PROTOCOL_645_2007

#ifdef PROTOCOL_EDMI_GROUP




/***************************************************
函数名称:processEdmiData
功能描述:EDMI数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S processEdmiData(INT8U arrayItem, INT8U *frame, INT16U loadLen)
{
  INT16U offset;
  INT8U  i;
  INT8U  retVal[5];
  INT32U dataSign, rawInt, rawDec;
  
  //数据存储偏移量
  offset = findDataOffset(multiCopy[arrayItem].protocol, frame);
  
  #ifdef SHOW_DEBUG_INFO
    printf("EDMI规约(LoadLen=%d):%02X %02X偏移:%02x\n", loadLen, frame[0], frame[1], offset);
  #endif
  
  if (offset==0x200)
  {
    return RECV_DATA_OFFSET_ERROR;  //接收数据保存时偏移地址错误,未保存
  }
  
  switch(frame[0])
  {
    case 0x01:    //正向有功电量
    case 0x00:    //反向有功电量
    case 0x03:    //正向无功电量
    case 0x02:    //反向无功电量
      decodeIeee754Float(&frame[2], retVal);
      //printf("示值:整数=%d,小数=%02d%02d\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);
     	
     	if (frame[1]<0x50)
     	{
     	  multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY;  //有上月电量数据
     	}
     	else
     	{
     	  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;     //有当前电量数据
     	}
      
      rawInt = retVal[4]<<16 | retVal[3]<<8 | retVal[2];
      rawDec = (rawInt%1000)*100/1000;
      rawInt /=1000;
      rawInt = hexToBcd(rawInt);
      rawDec = hexToBcd(rawDec);
      
     	*(multiCopy[arrayItem].energy+offset)   = rawDec;
     	*(multiCopy[arrayItem].energy+offset+1) = rawInt&0xff;
     	*(multiCopy[arrayItem].energy+offset+2) = rawInt>>8 & 0xff;
     	*(multiCopy[arrayItem].energy+offset+3) = rawInt>>16 & 0xff;
      break;
     
    case 0xe0:    //电压、电流、功率
      dataSign = decodeIeee754Float(&frame[2], retVal);
      //printf("电压/电流/功率:整数=%d,小数=%02d%02d\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);
     	
     	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;     //有参变量数据
     	
     	switch(frame[1])
     	{
     	  case 0x00:    //A相电压
     	  case 0x01:    //B相电压
     	  case 0x02:    //C相电压
           rawInt = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
           rawDec = hexToBcd(retVal[1]);
           *(multiCopy[arrayItem].paraVariable+offset+0) = (rawInt<<4&0xff) | (rawDec>>4&0xf);
           *(multiCopy[arrayItem].paraVariable+offset+1) = rawInt>>4&0xff;
     		 	 break;

     		 case 0x10:    //A相电流
     		 case 0x11:    //B相电流
     		 case 0x12:    //C相电流
           rawInt = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
           rawDec = hexToBcd(retVal[1])<<8 | hexToBcd(retVal[0]);
           *(multiCopy[arrayItem].paraVariable+offset+0) = rawDec>>4&0xff;
           *(multiCopy[arrayItem].paraVariable+offset+1) = (rawInt<<4&0xf0) | rawDec>>12&0xf;
           *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt>>4&0xff;
           if (dataSign==1)
           {
           	 *(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80; 
           }
     		 	 break;
     		 	 
     	  case 0x33:    //有功总功率
     	  case 0x30:    //有功A相总功率
     	  case 0x31:    //有功B相总功率
     	  case 0x32:    //有功C相总功率
     	  case 0x43:    //无功总功率
     	  case 0x40:    //无功A相总功率
     	  case 0x41:    //无功B相总功率
     	  case 0x42:    //无功C相总功率
           rawInt = retVal[4]<<16 | retVal[3]<<8 | retVal[2];
           rawDec = hexToBcd(rawInt%1000);
           rawInt = hexToBcd(rawInt/1000);
                   
           *(multiCopy[arrayItem].paraVariable+offset+0) = (rawDec<<4&0xf0) | (hexToBcd(retVal[1])>>4&0x0f);
           *(multiCopy[arrayItem].paraVariable+offset+1) = rawDec>>4;
           *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt&0xff;
           if (dataSign==1)
           {
           	 *(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80; 
           }
     		 	 break;
     		 	 
     		 case 0x26:    //总功率因数
           rawInt = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
           rawDec = hexToBcd(retVal[1])<<8 | hexToBcd(retVal[0]);
           *(multiCopy[arrayItem].paraVariable+offset+0) = rawDec>>4&0xff;
           *(multiCopy[arrayItem].paraVariable+offset+1) = (rawInt<<4&0xf0) | rawDec>>12&0xf;
           if (dataSign==1)
           {
           	 *(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80;
           }
     		 break;

     	}
    	break;
    
    case 0xF0:    //电表时间/日期
     	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;     //有参变量数据
      
      //F0 3d 06 07 0c 09 29 1e
     	*(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(frame[2]);  //日
     	*(multiCopy[arrayItem].paraVariable+offset+2) = hexToBcd(frame[3]);  //月
     	*(multiCopy[arrayItem].paraVariable+offset+3) = hexToBcd(frame[4]);  //年
     	*(multiCopy[arrayItem].paraVariable+offset+4) = hexToBcd(frame[6]);  //秒
     	*(multiCopy[arrayItem].paraVariable+offset+5) = hexToBcd(frame[6]);  //分
     	*(multiCopy[arrayItem].paraVariable+offset+6) = hexToBcd(frame[5]);  //时
    	break;
    
    case 0x11:    //当前正向有功最大需量
    case 0x10:    //当前反向有功最大需量,返回的数据是W,存的时候要存成kW
     	if (frame[1]&0x20)
     	{
     	  multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //有上月需量数据
     	}
     	else
     	{
     	  multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //有当前需量数据
     	}

      dataSign = decodeIeee754Float(&frame[2], retVal);
      //printf("需量:整数=%d,小数=%02d%02d\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);

      rawInt = retVal[4]<<16 | retVal[3]<<8 | retVal[2];
      rawDec = hexToBcd(rawInt%1000);
      rawInt = hexToBcd(rawInt/1000);
              
      *(multiCopy[arrayItem].reqAndReqTime+offset+0) = (rawDec<<4&0xf0) | (hexToBcd(retVal[1])>>4&0x0f);
      *(multiCopy[arrayItem].reqAndReqTime+offset+1) = rawDec>>4;
      *(multiCopy[arrayItem].reqAndReqTime+offset+2) = rawInt&0xff;
      if (dataSign==1)
      {
      	*(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80; 
      }
    	break;
    
    case 0x81:    //当前正向有功最大需量出现时间
    case 0x80:    //当前反向有功最大需量出现时间
    	//80 09 01 01 60 00 00 00
    	// 0  1  2  3  4  5  6  7
    	//      日  月 年 时 分 秒
     	if (frame[1]&0x20)
     	{
     	  multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //有上月需量数据
     	}
     	else
     	{
     	  multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //有当前需量数据
     	}
     	
     	*(multiCopy[arrayItem].reqAndReqTime+offset+0) = hexToBcd(frame[6]);  //分
     	*(multiCopy[arrayItem].reqAndReqTime+offset+1) = hexToBcd(frame[5]);  //时
     	*(multiCopy[arrayItem].reqAndReqTime+offset+2) = hexToBcd(frame[2]);  //日
     	*(multiCopy[arrayItem].reqAndReqTime+offset+3) = hexToBcd(frame[3]);  //月
     	*(multiCopy[arrayItem].reqAndReqTime+offset+4) = hexToBcd(frame[4]);  //年
     	break;
  }
  
  return METER_NORMAL_REPLY;
}

#endif

#ifdef LANDIS_GRY_ZD_PROTOCOL

/***************************************************
函数名称:findLeftBracket
功能描述:在字符串中查找左括号
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8U findLeftBracket(char dataChr[])
{
  INT8U j=0xff;
  
  for(j=0; j<strlen(dataChr); j++)
  {
    if (dataChr[j]=='(')
    {
      j++;
      break;
    }
  }
  
  return j;
}

/***************************************************
函数名称:processLandisGryData
功能描述:LandisGry/Simens数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S processLandisGryData(INT8U arrayItem, INT8U *frame, INT16U loadLen)
{
  INT16U offset, frameOffset;
  INT16U i;
  INT8U  retVal[5];
  INT32U dataSign, rawInt, rawDec;
  char   tmpLandis[100];
  INT16U itemInChr=0x201;        //数据在命令字符数组中的序号
  INT8U  bracketItemInData=0;    //左括号在字符串中的位置
  
  while(loadLen>0)
  {
    //46 2E 46 28 30 30 30 30 30 30 30 30 29 0D 0A 30 2E 39 2E 31 28 30 38 3A 31 37 3A 31 35 29 0D 0A
    frameOffset = 0;
    for(i=0; i<loadLen; i++)
    {
    	if (frame[i]==0x0d && frame[i+1]==0x0a)
    	{
    	  frameOffset+=2;
    	  
    	  memcpy(tmpLandis, frame, frameOffset);
    	  tmpLandis[frameOffset] = '\0';
    	  
    	  #ifdef SHOW_DEBUG_INFO
    	   printf("本次数据=%s", tmpLandis);
    	  #endif
        
        //数据存储item
        itemInChr = findDataOffset(multiCopy[arrayItem].protocol,(INT8U *)tmpLandis);
        
        if (itemInChr<=TOTAL_DATA_ITEM_ZD)
        {
          offset = landisOffset[itemInChr];
          bracketItemInData = findLeftBracket(tmpLandis);

          #ifdef SHOW_DEBUG_INFO
            printf("LandisGry规约:itemInChr=%d, brackeItemInData=%d, 偏移:%02x\n", itemInChr, bracketItemInData, landisOffset[itemInChr]);
          #endif
          
          //电能示值
          if (itemInChr<40)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
            	//1.8.0(000000.00*kWh)
            	rawInt = (tmpLandis[bracketItemInData]-0x30)*100000
            	       + (tmpLandis[bracketItemInData+1]-0x30)*10000
            	       + (tmpLandis[bracketItemInData+2]-0x30)*1000
            	       + (tmpLandis[bracketItemInData+3]-0x30)*100
            	       + (tmpLandis[bracketItemInData+4]-0x30)*10
            	       + (tmpLandis[bracketItemInData+5]-0x30);
            	rawDec = (tmpLandis[bracketItemInData+7]-0x30)*10
            	       + (tmpLandis[bracketItemInData+8]-0x30);
            	       
            	rawInt = hexToBcd(rawInt);
            	rawDec = hexToBcd(rawDec);
       	      
       	    multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;     //有当前电量数据
        
       	    *(multiCopy[arrayItem].energy+offset)   = rawDec;
       	    *(multiCopy[arrayItem].energy+offset+1) = rawInt&0xff;
       	    *(multiCopy[arrayItem].energy+offset+2) = rawInt>>8 & 0xff;
       	    *(multiCopy[arrayItem].energy+offset+3) = rawInt>>16 & 0xff;
       	  }
          }
          
          //需量及发生时间
          //1.6.0(03.310*kW)(06-12-30 15:35)
          if (itemInChr>=40 && itemInChr<60)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
            	rawInt = (tmpLandis[bracketItemInData+0]-0x30)*10
            	       + (tmpLandis[bracketItemInData+1]-0x30);
            	rawDec = (tmpLandis[bracketItemInData+3]-0x30)*100
            	       + (tmpLandis[bracketItemInData+4]-0x30)*10
            	       + (tmpLandis[bracketItemInData+5]-0x30);
            	       
            	rawInt = hexToBcd(rawInt);
            	rawDec = hexToBcd(rawDec*10);
       	      
       	    multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //有当前需量数据
              
              *(multiCopy[arrayItem].reqAndReqTime+offset+0) = rawDec&0xff;
              *(multiCopy[arrayItem].reqAndReqTime+offset+1) = rawDec>>8&0xff;
              *(multiCopy[arrayItem].reqAndReqTime+offset+2) = rawInt&0xff;
              
              tmpLandis[bracketItemInData-1] = 0x30;
              
              bracketItemInData = findLeftBracket(tmpLandis);

              #ifdef SHOW_DEBUG_INFO
               printf("LandisGry规约数据(需量发生时间):bracketItemInData=%d,偏移:%02x\n", bracketItemInData, landisOffset[itemInChr]);
              #endif
              
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+0) = hexToBcd((tmpLandis[bracketItemInData+12]-0x30)*10 + (tmpLandis[bracketItemInData+13]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+1) = hexToBcd((tmpLandis[bracketItemInData+9]-0x30)*10 + (tmpLandis[bracketItemInData+10]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+2) = hexToBcd((tmpLandis[bracketItemInData+6]-0x30)*10 + (tmpLandis[bracketItemInData+7]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+3) = hexToBcd((tmpLandis[bracketItemInData+3]-0x30)*10 + (tmpLandis[bracketItemInData+4]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+4) = hexToBcd((tmpLandis[bracketItemInData+0]-0x30)*10 + (tmpLandis[bracketItemInData+1]-0x30));
            }
          }

          //电表时间日期
          //0.9.1(11:01:47)
          //0.9.2(12-07-12)
          if (itemInChr==60 || itemInChr==61)
          {
     	      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //有参变量数据

     	      if (itemInChr==61)
     	      {
     	      	*(multiCopy[arrayItem].paraVariable+offset) = 0x0;
     	      	offset++;
     	      }

            *(multiCopy[arrayItem].paraVariable+offset)   = hexToBcd((tmpLandis[bracketItemInData+6]-0x30)*10 + (tmpLandis[bracketItemInData+7]-0x30));
            *(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd((tmpLandis[bracketItemInData+3]-0x30)*10 + (tmpLandis[bracketItemInData+4]-0x30));
            *(multiCopy[arrayItem].paraVariable+offset+2) = hexToBcd((tmpLandis[bracketItemInData+0]-0x30)*10 + (tmpLandis[bracketItemInData+1]-0x30));
          }

          //三相电压
          //32.7(---.-*V)
          //52.7(---.-*V)
          //72.7(227.7*V)
          if (itemInChr>=64 && itemInChr<=66)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //有参变量数据

          	 rawInt = (tmpLandis[bracketItemInData+0]-0x30)*100
          	         + (tmpLandis[bracketItemInData+1]-0x30)*10
          	         + (tmpLandis[bracketItemInData+2]-0x30);
          	 rawDec = (tmpLandis[bracketItemInData+4]-0x30);
          	 rawInt = hexToBcd(rawInt);
          	 rawDec = hexToBcd(rawDec);
              
              *(multiCopy[arrayItem].paraVariable+offset+0) = (rawInt<<4&0xff) | (rawDec&0xf);
              *(multiCopy[arrayItem].paraVariable+offset+1) = rawInt>>4&0xff;
            }
          }

          //三相电流
          //31.7(00.00*A)
          //51.7(00.00*A)
          //71.7(00.00*A)
          if (itemInChr>=67 && itemInChr<=69)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //有参变量数据

          	  rawInt = (tmpLandis[bracketItemInData+0]-0x30)*10
          	         + (tmpLandis[bracketItemInData+1]-0x30);
          	  rawDec = (tmpLandis[bracketItemInData+3]-0x30)*10
          	         + (tmpLandis[bracketItemInData+4]-0x30);
          	  rawInt = hexToBcd(rawInt);
          	  rawDec = hexToBcd(rawDec);
              
              *(multiCopy[arrayItem].paraVariable+offset+0) = rawDec<<4&0xff;
              *(multiCopy[arrayItem].paraVariable+offset+1) = (rawInt<<4&0xf0) | rawDec>>4&0xf;
              *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt>>4&0xff;
            }
          }
          
          //参数设置次数
          //C.2.0(00000003)
          if (itemInChr==70)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //有参变量数据
     	        
     	        rawInt = (tmpLandis[bracketItemInData+0]-0x30)*10000000
          	         + (tmpLandis[bracketItemInData+1]-0x30)*1000000
          	         + (tmpLandis[bracketItemInData+2]-0x30)*100000
          	         + (tmpLandis[bracketItemInData+3]-0x30)*10000
          	         + (tmpLandis[bracketItemInData+4]-0x30)*1000
          	         + (tmpLandis[bracketItemInData+5]-0x30)*100
          	         + (tmpLandis[bracketItemInData+6]-0x30)*10
          	         + (tmpLandis[bracketItemInData+7]-0x30)*1;
              *(multiCopy[arrayItem].paraVariable+offset+0) = rawInt&0xff;
              *(multiCopy[arrayItem].paraVariable+offset+1) = rawInt>>8&0xff;
              *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt>>16&0xff;
     	      }
          }
          
          //上次参数设置时间
          //C.2.1(05-10-19 16:36)
          if (itemInChr==71)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //有参变量数据
     	        
              *(multiCopy[arrayItem].paraVariable+offset+0) = 0x0;    //秒
          	*(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd((tmpLandis[bracketItemInData+12]-0x30)*10 + (tmpLandis[bracketItemInData+13]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+2) = hexToBcd((tmpLandis[bracketItemInData+9]-0x30)*10 + (tmpLandis[bracketItemInData+10]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+3) = hexToBcd((tmpLandis[bracketItemInData+6]-0x30)*10 + (tmpLandis[bracketItemInData+7]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+4) = hexToBcd((tmpLandis[bracketItemInData+3]-0x30)*10 + (tmpLandis[bracketItemInData+4]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+5) = hexToBcd((tmpLandis[bracketItemInData+0]-0x30)*10 + (tmpLandis[bracketItemInData+1]-0x30));
     	      }
          }
          
          #ifdef SHOW_DEBUG_INFO
           printf("\n");
          #endif
        }
    	  
    	  break;
    	}
    	else
    	{
    	  frameOffset++;
    	}
    }
    
    //移位
    frame += frameOffset;
    if (loadLen>=frameOffset)
    {
      loadLen -= frameOffset;
    }
    else
    {
    	break;
    }
  }
  
  return METER_NORMAL_REPLY;
}

#endif

#ifdef PROTOCOL_ABB_GROUP

/*******************************************************
函数名称:CRC_ABB
功能描述:计算16位ABB的CRC值
调用函数:
被调用函数:
输入参数:aData,要进行CRC校验的消息
         aSize,消息中字节数
输出参数:
返回值：CRC值
*******************************************************/
unsigned short CRC_ABB(unsigned char * aData, unsigned long aSize)
{
	int i,j;
	int flag;
	unsigned int iTemp=0;
	
	for(i=0; i<aSize; i++)
	{
	  iTemp ^= (aData[i]<<8);
	  for(j=0; j<8; j++)
	  {
	  	flag = iTemp&0x8000;
	  	iTemp<<=1;
	  	if (flag)
	  	{
	  	  iTemp ^= 0x1021;
	  	}
	  }
	}
	
	return iTemp;
}

/*******************************************************
函数名称:abbEncryption
功能描述:ABB方表加密
调用函数:
被调用函数:
输入参数:lKey,本地口令
         rKey,远程通讯口令
输出参数:
返回值：CRC值
*******************************************************/
unsigned long abbEncryption(unsigned long lKey, unsigned long rKey)
{
	 unsigned long pword;
	 int           i;
	 int           j, k=0;
	 
	 //printf("lKey=%08X,rKey=%08X\n", lKey, rKey);

	 union
	 {
	 	 unsigned long key;    //encryption key
	 	 
	 	 //broken into bytes
	 	 struct
	 	 {
	 	 	 unsigned char byta,bytb,bytc,bytd;
	 	 }parts;
	 }val;
	 
	 val.key = rKey;
	 pword   = lKey;
	 
	 //Add an arbitrary number to the key just for fun
	 val.key += 0xab41;
	 
	 //generate a four bit checksum to be used as loop index
	 i = val.parts.byta + val.parts.bytb+val.parts.bytc+val.parts.bytd;
	 i = i&0xf;
	 while(i>=0)
	 {
	 	 //Set 'j' to the value of the high bit before shifting.Simulates carry flag
	 	 if (val.parts.bytd>=0x80)
	 	 {
	 	 	 j=1;
	 	 }
	 	 else
	 	 {
	 	 	 j=0;
	 	 }
	 	 
	 	 //Shift the key.Add in the carry flag from the previous loop
	 	 val.key = val.key<<1;
	 	 val.key += k;
	 	 k =j;
	 	 
	 	 //Apply the key to the password
	 	 pword ^= val.key;
	 	 i--;
	 }
	 
	 //printf("pword=%08X\n", pword);
	 
	 return pword;
}


/***************************************************
函数名称:processAbbData
功能描述:ABB方表数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S abbBcdToData(INT8U *buf, INT16U offset, INT8U *inData, INT8U dploce,INT8U *calcBuf)
{
  INT8U j,k;
  
  if ((dploce+6)%2==0)
  {
    //小数
    buf[offset] = inData[7-(dploce+6)/2];
    
    calcBuf[0] = inData[7-(dploce+6)/2+1];
    calcBuf[1] = inData[7-(dploce+6)/2];
    
    //整数
    memset(buf+offset+1, 0x00, 3);
    
    for(j=7-(dploce+6)/2,k=1; j>0; j--,k++)
    {
      buf[offset+k] = inData[j-1];
      
      calcBuf[k+1] = inData[j-1];
    }
  }
  else
  {
    //小数
    buf[offset] = (inData[7-(dploce+6)/2]>>4) | (inData[7-(dploce+6)/2-1]&0xf)<<4;
    
    //整数
    memset(buf+offset+1, 0x00, 3);
    
    for(j=7-(dploce+6)/2,k=1; j>0; j--,k++)
    {
      buf[offset+k] = inData[j-1]>>4;

      if (j>1)
      {
        buf[offset+k] |= (inData[j-2]&0xf)<<4;
      }
    }
  }
}

/***************************************************
函数名称:processAbbData
功能描述:ABB方表数据处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
INT8S processAbbData(INT8U arrayItem, INT8U *frame,  INT16U loadLen, INT8U copyItem)
{
  INT8U  i, j, k;
  INT8U  eblkcf=0;
  INT8U  offsetItem=0;
  INT16U offsetVision, offsetReq, offsetReqTime;
  INT8U  dploce, dplocd;
  INT8U  calcBuf[10];
  static INT32U sumInt, sumDec;

  //printf("processAbbData(%d):", copyItem);
 	 
  //for(i=0; i<loadLen; i++)
  //{
  //  printf("%02x ", frame[i]);
  //}
  //printf("\n");
  
  dploce = abbClass0[11];
  dplocd = abbClass0[12];
  
  if ((copyItem%2)==0)
  {
  	sumInt = 0;
  	sumDec = 0;
  }
  
  switch(copyItem)
  {
  	case 0:
  	case 1:
  		eblkcf = abbClass2[51];
  		offsetItem = copyItem;
  		break;

  	case 2:
  	case 3:
  		eblkcf = abbClass2[52];
  		offsetItem = copyItem-2;
  		break;

  	case 4:
  	case 5:
  		//eblkcf = abbClass2[84];
  		eblkcf = 3;    //无法从class2中获取该数据块的功能项,定为无功正向
  		offsetItem = copyItem-4;
  		break;

  	case 6:
  	case 7:
  		//eblkcf = abbClass2[85];
  		eblkcf = 0x0c;    //无法从class2中获取该数据块的功能项,定为无功反向
  		offsetItem = copyItem-6;
  		break;
    
    case 8:    //四三二一象限总示值
      offsetVision = QUA4_NO_WORK_OFFSET;
      for(i=0; i<4; i++)
      {
    	  abbBcdToData(multiCopy[arrayItem].energy, offsetVision, frame+i*7, dploce, calcBuf);
    	  
    	  switch(i)
    	  {
    	  	case 0:
    	  		offsetVision = QUA3_NO_WORK_OFFSET;
    	  		break;

    	  	case 1:
    	  		offsetVision = QUA2_NO_WORK_OFFSET;
    	  		break;

    	  	case 2:
    	  		offsetVision = QUA1_NO_WORK_OFFSET;
    	  		break;
    	  }
      }
    	
      return METER_REPLY_ANALYSE_OK;
  }
  
  switch (eblkcf)
  {
	  case 0x80:    //有功正向 KW-del
	    offsetVision  = POSITIVE_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_POSITIVE_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_P_WORK_OFFSET+5+offsetItem*10;
	    //printf("有功正向数据\n");
	    break;
	    
	  case 0x40:    //有功反向 KW-rec
	    offsetVision  = NEGTIVE_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_NEGTIVE_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_N_WORK_OFFSET+5+offsetItem*10;
	    //printf("有功反向数据\n");
	  	break;
	  
	  case 0x03:    //无功正向 KVAR-del
	    offsetVision  = POSITIVE_NO_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_POSITIVE_NO_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_P_NO_WORK_OFFSET+5+offsetItem*10;
	    //printf("无功正向数据\n");
	  	break;

	  case 0x0C:    //无功正向 KVAR-rec
	    offsetVision  = NEGTIVE_NO_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_NEGTIVE_NO_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_N_NO_WORK_OFFSET+5+offsetItem*10;
	    //printf("无功反向数据\n");
	  	break;
	  	
	  default:
	  	return METER_NORMAL_REPLY;
  }
  
  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;  //有当前电能示值数据
  multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;     //有当前需量数据
         
  //每42个字节是两个费率的数据,两个数据帧回的才是A,B,C,D费率的数据
  for(i=0; i<2; i++)
  {
    //1电量(7Bytes,BCD码,小数位数由CLASS 0中的DPLOCE+6决定)
    memset(calcBuf, 0x0, 10);
    abbBcdToData(multiCopy[arrayItem].energy, offsetVision, frame+i*21, dploce, calcBuf);
    offsetVision += 4;
    
    //累加计算总示值
    sumDec += bcdToHex(calcBuf[1]<<8 | calcBuf[0]);
    sumInt += bcdToHex(calcBuf[3]<<16 | calcBuf[3]<<8 | calcBuf[2]);
    sumInt += sumDec/10000;
    sumDec = sumDec%10000;
  
    //2最大需量(3Bytes,BCD码,小数位数由CLASS 0中的DPLOCD决定)
    if ((dplocd%2)==0)
    {
      //整数
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+2) = frame[i*21+7+(3-dplocd/2-1)];

      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+0) = 0x0;
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+1) = 0x0;
      for(j=2,k=0; j>0 && k<(dplocd/2); j--,k++)
      {
        *(multiCopy[arrayItem].reqAndReqTime+offsetReq+j-1)   = frame[i*21+7+(3-dplocd/2)+k];
      }
    }
    else
    {
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+0) = 0x0;
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+1) = 0x0;

      //整数
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+2) = frame[i*21+7+(3-dplocd/2-1)]>>4;
      if (dplocd>1)
      { 
      	*(multiCopy[arrayItem].reqAndReqTime+offsetReq+2) |= (frame[i*21+7+(3-dplocd/2-2)]&0xf)<<4;
      }
      else
      {
        //只有一位小数的小数位处理
        *(multiCopy[arrayItem].reqAndReqTime+offsetReq+1) = frame[i*21+7+(3-dplocd/2-1)]<<4;
      }

      for(j=2,k=0; j>0 && k<((dplocd+1)/2); j--,k++)
      {
        *(multiCopy[arrayItem].reqAndReqTime+offsetReq+j-1) = (frame[i*21+7+(3-dplocd/2-1)+k]&0xf)<<4;
        
        if (j>1)
        {
          *(multiCopy[arrayItem].reqAndReqTime+offsetReq+j-1) |= frame[i*21+7+(3-dplocd/2)+k]>>4;
        }
      }
    }
    
    offsetReq += 3;
    
    //3最大需量发生时间(5Bytes,BCD格式的年月日时分)
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime)   = frame[i*21+14];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+1) = frame[i*21+13];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+2) = frame[i*21+12];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+3) = frame[i*21+11];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+4) = frame[i*21+10];
    offsetReqTime += 5;
  }
  
  if (copyItem%2)
  {
  	offsetVision -= 20;
  	sumDec = hexToBcd(sumDec);
  	sumInt = hexToBcd(sumInt);
  	multiCopy[arrayItem].energy[offsetVision] = sumDec>>8;
  	multiCopy[arrayItem].energy[offsetVision+1] = sumInt&0xff;
  	multiCopy[arrayItem].energy[offsetVision+2] = sumInt>>8;
  	multiCopy[arrayItem].energy[offsetVision+3] = sumInt>>16;
  }
  
  return METER_NORMAL_REPLY;
}

#endif


#ifdef PROTOCOL_MODUBUS_GROUP

/***************************************************
函数名称:processHyData
功能描述:山东和远规约数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processHyData(INT8U arrayItem, INT8U *data,INT8U length)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;

  if (16==length)
  {
	  //有当前电量数据
	  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;
	  
	  //正向有功电能,单位Wh
	  offset = POSITIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //反向有功电能,单位Wh
	  offset = NEGTIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //感性无功电能,单位Varh,正向无功
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //容性无功电能,单位Varh,反向无功
	  offset = NEGTIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

  }
  else
  {
	  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据

	  //电压
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
	    tmpData = hexToBcd((data[tmpTail]<<8 | data[tmpTail+1])/10);    
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 2;
			offset += 2;
	  }
	  //预留2个寄存器
	  tmpTail += 4;
	  
	  //三相线电压
	  tmpTail += 6;
	  
	  //预留1个寄存器
	  tmpTail += 2;
	  
	  //电流
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
	    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
		tmpTail += 2;
		offset += 3;
	  }

	  //预留2个寄存器
	  tmpTail += 4;
	  
	  //有功功率
	  offset = POWER_PHASE_A_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //总有功功率
	  offset = POWER_INSTANT_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //无功功率
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		 	 tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //总无功功率
	  offset = POWER_INSTANT_NO_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //视在功率
	  offset = POWER_PHASE_A_APPARENT;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //总视在功率
	  offset = POWER_INSTANT_APPARENT; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;

	  //功率因数
	  offset = FACTOR_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x8000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 2;
			offset += 2;
	  }  
	  //总功率因数
	  offset = TOTAL_POWER_FACTOR; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 2;

	  //电压频率
	  offset = METER_STATUS_WORD;    //2017-7-19,电压频率放在电表第一个状态字里 
    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail+=2;
  }
  
  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processAssData
功能描述:无锡爱森思规约数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processAssData(INT8U arrayItem, INT8U *data,INT8U length)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据

  //电压
  offset = VOLTAGE_PHASE_A;
  for(i=0; i<3; i++)
  {
    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);    
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	
		tmpTail += 2;
		offset += 2;
  }
  
  //三相线电压
  tmpTail += 6;
  
  //电流
  offset = CURRENT_PHASE_A;
  for(i=0; i<3; i++)
  {
    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }
  
  //有功功率
  offset = POWER_PHASE_A_WORK;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
		if (data[tmpTail]&0x80)
		{
	  	tmpData |= 0x800000;
		}
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }  
  //总有功功率
  offset = POWER_INSTANT_WORK; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
  if (data[tmpTail]&0x80)
  {
		tmpData |= 0x800000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
  tmpTail += 2;
  
  //无功功率
  offset = POWER_PHASE_A_NO_WORK;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
		if (data[tmpTail]&0x80)
		{
	  	tmpData |= 0x800000;
		}
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }  
  //总无功功率
  offset = POWER_INSTANT_NO_WORK; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
  if (data[tmpTail]&0x80)
  {
		tmpData |= 0x800000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
  tmpTail += 2;
  
  //视在功率
  offset = POWER_PHASE_A_APPARENT;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
		if (data[tmpTail]&0x80)
		{
		  tmpData |= 0x800000;
		}
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }  
  //总视在功率
  offset = POWER_INSTANT_APPARENT; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
  if (data[tmpTail]&0x80)
  {
	tmpData |= 0x800000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
  tmpTail += 2;

  //功率因数
  offset = FACTOR_PHASE_A;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2));
		if (data[tmpTail]&0x80)
		{
		  tmpData |= 0x8000;
		}

    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	
		tmpTail += 2;
		offset += 2;
  }  
  //总功率因数
  offset = TOTAL_POWER_FACTOR; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2));
  if (data[tmpTail]&0x80)
  {
		tmpData |= 0x8000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  tmpTail += 2;

  //电压频率
  offset = METER_STATUS_WORD;    //2017-7-19,电压频率放在电表第一个状态字里 
  tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]<<0);
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  tmpTail+=2;
  
  //有当前电量数据
  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

  //正向有功电能,单位Wh
  offset = POSITIVE_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //变成KWh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  //反向有功电能,单位Wh
  offset = NEGTIVE_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //变成KWh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  //感性无功电能,单位Varh,正向无功
  offset = POSITIVE_NO_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //变成KVarh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  //容性无功电能,单位Varh,反向无功
  offset = NEGTIVE_NO_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //变成KVarh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processXyData
功能描述:上海贤业modbus表数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processXyData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  INT8U  sign=0;
  
  INT8U  retVal[5];
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据

 #ifdef SHOW_DEBUG_INFO
  printf("%s==>protocol=%d,item=%d,copyitem=%d\n", __func__, protocol, arrayItem, multiCopy[arrayItem].copyItem);
 #endif
  
  if(
     protocol!=MODBUS_XY_M
     || (1==multiCopy[arrayItem].copyItem && MODBUS_XY_M==protocol)
  	)
  {
	  //电压
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = (hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2])<<4) | (retVal[1]/10);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电压%d:整数=%d,小数=%02d%02d,V=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1], retVal[0], tmpData);
		 #endif
		 
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
	    tmpTail += 4;
	  	offset += 2;
	  }

	  if (MODBUS_XY_F==protocol || MODBUS_XY_M==protocol)
	  {
	    //三相三线电压
	    tmpTail += 12;
	  }

	  //电流
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*1000 + (retVal[1]*100+retVal[2])/10);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电流%d:整数=%d,小数=%02d%02d,C=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
  }

  if (MODBUS_XY_F==protocol || MODBUS_XY_M==protocol )
  {
	  if(
		 protocol!=MODBUS_XY_M
		 || (1==multiCopy[arrayItem].copyItem && MODBUS_XY_M==protocol)
		)
	  {
		  //有功功率
		  offset = POWER_PHASE_A_WORK;
		  for(i=0; i<3; i++)
		  {
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				  
				if (sign)
				{
				  tmpData |= 0x800000;
				}
		    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
		    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
		  }

		  if (MODBUS_XY_M==protocol)
		  {
				return METER_REPLY_ANALYSE_OK;
		  }
	  }
		
	  //总有功功率
	  offset = POWER_INSTANT_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //无功功率
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }  
	  //总无功功率
	  offset = POWER_INSTANT_NO_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //总视功功率
	  offset = POWER_INSTANT_APPARENT; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //总功率因数
	  offset = TOTAL_POWER_FACTOR; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
    if (MODBUS_XY_M==protocol)
    {
      tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
    }
	  else
	  {
	    tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
    }
		
	 #ifdef SHOW_DEBUG_INFO
	  printf("%s==>功率因数:整数=%d,小数=%02d%02d,Factor=%x\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
	 #endif
	 
	  if (sign)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 4;
	  
	  //电压频率
	  offset = METER_STATUS_WORD;    //2017-7-19,电压频率放在电表第一个状态字里 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  *(multiCopy[arrayItem].paraVariable+offset+0) = hexToBcd(retVal[1]);
	  *(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(retVal[2]);
	  tmpTail+=4;

	  //二次有功电能
	  //有当前电量数据
	  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //正向有功电能,单位Wh
	  offset = POSITIVE_WORK_OFFSET; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

	  
	  if (MODBUS_XY_M==protocol)
	  {
		  //反向有功电能,单位Wh
		  offset = NEGTIVE_WORK_OFFSET; 
		  sign = decodeIeee754Float(&data[tmpTail], retVal);
		  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
		  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  }
	  else
	  {
	    //无意义
	  }
	  tmpTail += 4;
	  
	  //正向无功电能,单位Wh
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  if (MODBUS_XY_M==protocol)
	  {
		  //反向无  功电能,单位Wh
		  offset = NEGTIVE_NO_WORK_OFFSET; 
		  sign = decodeIeee754Float(&data[tmpTail], retVal);
		  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
		  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  }
	  else
	  {
	    //无意义
	  }
	  tmpTail += 4;
  }

  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processSwitchFData
功能描述:开关量模块数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processSwitchData(INT8U arrayItem, INT8U *data,INT8U length)
{ 
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据
  
  //2017-8-22,开关状态放在电表第二个状态字里
  *(multiCopy[arrayItem].paraVariable+METER_STATUS_WORD_2+0) = *(data+1);    //低字节
  *(multiCopy[arrayItem].paraVariable+METER_STATUS_WORD_2+1) = *data;        //高字节
  
  return METER_REPLY_ANALYSE_OK;	//接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processMwData
功能描述:成都明武modbus表数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
	INT8U processMwData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
	{
		INT32U tmpData;
		INT8U  tmpTail = 0;
		INT8U  i;
		INT16U offset;
		INT8U  sign=0;
		
		INT8U  retVal[5];
		
		multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据
	
		if(MODBUS_MW_F==protocol)
		{
			//电压频率
			offset = METER_STATUS_WORD; 	 //2018-3-1,电压频率放在电表第一个状态字里 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			*(multiCopy[arrayItem].paraVariable+offset+0) = hexToBcd(retVal[1]);
			*(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(retVal[2]);
			tmpTail+=4;
		}
		
		if (MODBUS_MW_F==protocol || MODBUS_MW_UI==protocol)
		{
			//电压
			offset = VOLTAGE_PHASE_A;
			for(i=0; i<3; i++)
			{
				decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = (hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2])<<4) | (retVal[1]/10);
			 #ifdef SHOW_DEBUG_INFO
				printf("%s==>电压%d:整数=%d,小数=%02d%02d,V=%x\r\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1], retVal[0], tmpData);
			 #endif
			
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			
				tmpTail += 4;
				offset += 2;
			}
	
			//相平均电压、三相线电压、线电压平均值
			if(MODBUS_MW_UI==protocol )
			{
				tmpTail += 16;
			}
			else
			{
				tmpTail += 20;
			}
	
			//电流
			offset = CURRENT_PHASE_A;
			for(i=0; i<3; i++)
			{
				decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*1000 + (retVal[1]*100+retVal[2])/10);
				
			 #ifdef SHOW_DEBUG_INFO
				printf("%s==>电流%d:整数=%d,小数=%02d%02d,C=%x\r\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
			 #endif
			
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
		}
	
		if (MODBUS_MW_F==protocol)
		{
			//三相电流平均值Iavg
			tmpTail += 4;
			
			//中线电流In
			tmpTail += 4;
	
			//有功功率
			offset = POWER_PHASE_A_WORK;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
					
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
			
			//总有功功率
			offset = POWER_INSTANT_WORK; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			tmpTail += 4;
	
			//无功功率
			offset = POWER_PHASE_A_NO_WORK;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
			//总无功功率
			offset = POWER_INSTANT_NO_WORK; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			tmpTail += 4;
	
			//视在功率
			offset = POWER_PHASE_A_APPARENT;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
					
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
			//总视功功率
			offset = POWER_INSTANT_APPARENT; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			tmpTail += 4;
	
			//功率因数
			offset = FACTOR_PHASE_A;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				//tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
				tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
				
			 #ifdef SHOW_DEBUG_INFO
				printf("功率因数%d:整数=%d,小数=%02d%02d,Factor=%x\r\n",i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
			 #endif
			 
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			
				tmpTail += 4;
				offset += 2;
			}
			
			//总功率因数
			offset = TOTAL_POWER_FACTOR; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			//tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
			tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("功率因数:整数=%d,小数=%02d%02d,Factor=%x\r\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		 
			if (sign)
			{
				tmpData |= 0x8000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			tmpTail += 4;
			
			//有功功率需量Dmd_P
			tmpTail += 4;
			
			//无功功率需量Dmd_Q
			tmpTail += 4;
			
			//视在功率需量Dmd_S
			tmpTail += 4;
	
			//有当前电量数据 	
			multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;
	
			//正向有功电能,单位Wh
			offset = POSITIVE_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
			
			//反向有功电能,单位Wh
			offset = NEGTIVE_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
			
			//正向无功电能,单位Wh
			offset = POSITIVE_NO_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
			
			//反向无  功电能,单位Wh
			offset = NEGTIVE_NO_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
		}
	
		return METER_REPLY_ANALYSE_OK;	//接收数据帧正常且解析正确,已保存进缓存
	}


/***************************************************
函数名称:processJzData
功能描述:上海居正modbus表数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processJzData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  INT8U  sign=0;
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据

  if(MODBUS_JZ_F==protocol)
  {
	  //电压频率
	  offset = METER_STATUS_WORD;    //2018-3-5,电压频率放在电表第一个状态字里 
		tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail+=2;
	}
	
	if (MODBUS_JZ_F==protocol || MODBUS_JZ_UI==protocol)
	{
		//电压
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电压%d=%x\n", __func__, i+1, tmpData);
		 #endif
		 
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    tmpTail += 2;
	  	offset += 2;
	  }

	  //相平均电压、三相线电压、线电压平均值
	  tmpTail += 10;

	  //电流
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
	    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电流%d=%x\n", __func__, i+1, tmpData);
		 #endif
			
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
		  tmpTail += 2;
		  offset += 3;
	  }
  }

  if (MODBUS_JZ_F==protocol)
  {
	  //三相电流平均值Iavg
		tmpTail += 2;
		
	  //中线电流In
		tmpTail += 2;

	  //有功功率
	  offset = POWER_PHASE_A_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }
	  //总有功功率
	  offset = POWER_INSTANT_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //无功功率
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		 	 tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //总无功功率
	  offset = POWER_INSTANT_NO_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //视在功率
	  offset = POWER_PHASE_A_APPARENT;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //总视在功率
	  offset = POWER_INSTANT_APPARENT; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;

	  //功率因数
	  offset = FACTOR_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x8000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 2;
			offset += 2;
	  }  
	  //总功率因数
	  offset = TOTAL_POWER_FACTOR; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 2;

		//电压不对称度
	  tmpTail += 2;

		//负载性质
	  tmpTail += 2;
	  
		//有功功率需量Dmd_P
		tmpTail += 2;
		
		//无功功率需量Dmd_Q
		tmpTail += 2;
		
		//视在功率需量Dmd_S
		tmpTail += 2;
		
		//DI
	  tmpTail += 2;
		
		//DO状态
	  tmpTail += 2;
		

	  //有当前电量数据	  
		multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //正向有功电能,单位Wh
	  offset = POSITIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //反向有功电能,单位Wh
	  offset = NEGTIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //感性无功电能,单位Varh,正向无功
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //容性无功电能,单位Varh,反向无功
	  offset = NEGTIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //变成KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
  }

  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processWe6301Data
功能描述:威斯顿we6301表数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processWe6301Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;

 #ifdef SHOW_DEBUG_INFO
	printf("%s==>multiCopy[%d].copyItem=%d\n", __func__, arrayItem, multiCopy[arrayItem].copyItem);
 #endif
	
  if (1==multiCopy[arrayItem].copyItem)
  {
  	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据
  		
		//电压
		offset = VOLTAGE_PHASE_A;
		for(i=0; i<3; i++)
		{
			tmpData = hexToBcd((data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3])/100);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电压%d=%x\n", __func__, i+1, tmpData);
		 #endif
		 
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			tmpTail += 4;
			offset += 2;
		}
		
		//三相线电压
		tmpTail += 12;
		
		//电流
		offset = CURRENT_PHASE_A;
		for(i=0; i<3; i++)
		{
			tmpData = hexToBcd(data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3]);
		
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电流%d=%x\n", __func__, i+1, tmpData);
		 #endif
			
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
		}

    //保留字节
		tmpTail += 12;
		
		//总功率因数
		offset = TOTAL_POWER_FACTOR; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
		if (data[tmpTail]&0x80)
		{
			tmpData |= 0x8000;
		}
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>总功率因数=%x\n", __func__, tmpData);
	 #endif

		*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		tmpTail += 4;
		
		//电压频率
		offset = METER_STATUS_WORD; 	 //2018-4-25,电压频率放在电表第一个状态字里 
		tmpData = hexToBcd(data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3]);

	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>频率=%x\n", __func__, tmpData);
	 #endif
	 
		*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		tmpTail += 4;

    //中性线电流
		tmpTail += 4;
		
		//功率因数
		offset = FACTOR_PHASE_A;
		for(i=0; i<3; i++)
		{
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
			if (data[tmpTail]&0x80)
			{
				tmpData |= 0x8000;
			}
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>功率因数%d=%x\n", __func__, i, tmpData);
		 #endif
			
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 4;
			offset += 2;
		}  
  }
	else if (2==multiCopy[arrayItem].copyItem)
  {
		multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据

	  //总有功功率
	  offset = POWER_INSTANT_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>总有功功率=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //总无功功率
	  offset = POWER_INSTANT_NO_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>总无功功率=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;
		
	  //总视在功率
	  offset = POWER_INSTANT_APPARENT; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>总视在功率=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;
		
	  //有功功率
	  offset = POWER_PHASE_A_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
	  
	  //无功功率
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
			if (data[tmpTail]&0x80)
			{
		 	 tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }  
	  
	  //视在功率
	  offset = POWER_PHASE_A_APPARENT;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
			if (data[tmpTail]&0x80)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
  }
	else if (3==multiCopy[arrayItem].copyItem)
  {
	  //有当前电量数据	  
		multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //正向有功电能,单位0.01kWh
	  offset = POSITIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>正向有功电量=%x\n", __func__, tmpData);
	 #endif

	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //反向有功电能,单位0.01kWh
	  offset = NEGTIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>反向有功电量=%x\n", __func__, tmpData);
	 #endif
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

    //保留字节,空闲
		tmpTail += 4;

		//有功总电能
		tmpTail += 4;
	  
	  //正向无功电能,单位0.01kVarh
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>正向无功电量=%x\n", __func__, tmpData);
	 #endif
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //反向无功电能,单位0.01kVarh
	  offset = NEGTIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);

   #ifdef SHOW_DEBUG_INFO
		printf("%s==>反向无功电量=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	}

  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}

/***************************************************
函数名称:processPmc350Data
功能描述:深圳中电PMC350 modbus表数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，length数据长度
输出参数:
返回值：
***************************************************/
INT8U processPmc350Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  INT8U  sign=0;
  
  INT8U  retVal[5];
	
 #ifdef SHOW_DEBUG_INFO
	printf("%s==>multiCopy[%d].copyItem=%d\n", __func__, arrayItem, multiCopy[arrayItem].copyItem);
 #endif
	
  if (1==multiCopy[arrayItem].copyItem)
  {
  	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //有参数数据

		//电压
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = (hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2])<<4) | (retVal[1]/10);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电压%d:整数=%d,小数=%02d%02d,V=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1], retVal[0], tmpData);
		 #endif
		
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
	    tmpTail += 4;
	  	offset += 2;
	  }

	  //平均相电压、三相线电压、平均线电压
		tmpTail += 20;

	  //电流
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*1000 + (retVal[1]*100+retVal[2])/10);
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>电流%d:整数=%d,小数=%02d%02d,C=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
		  tmpTail += 4;
		  offset += 3;
	  }
		
	  //平均相电流Iavg
		tmpTail += 4;

		//有功功率
		offset = POWER_PHASE_A_WORK;
		for(i=0; i<3; i++)
		{
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				
			if (sign)
			{
				tmpData |= 0x800000;
			}
			
     #ifdef SHOW_DEBUG_INFO
			printf("%s==>有功功率%d:整数=%d,小数=%02d%02d,P=%x\n", __func__, i, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		 
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
		}
		
	  //总有功功率
	  offset = POWER_INSTANT_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
   #ifdef SHOW_DEBUG_INFO
	  printf("%s==>总有功功率:整数=%d,小数=%02d%02d,P=%x\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
	 #endif
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //无功功率
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
	  //总无功功率
	  offset = POWER_INSTANT_NO_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

		//视在功率
		offset = POWER_PHASE_A_APPARENT;
		for(i=0; i<3; i++)
		{
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
		}
	  //总视功功率
	  offset = POWER_INSTANT_APPARENT; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //功率因数
	  offset = FACTOR_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			sign = decodeIeee754Float(&data[tmpTail], retVal);
		  //tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
	    tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>功率因数%d:整数=%d,小数=%02d%02d,Factor=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		 
			if (sign)
			{
				tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 4;
			offset += 2;
	  }
		
		//总功率因数
	  offset = TOTAL_POWER_FACTOR; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
		//tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
	  tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
		
	 #ifdef SHOW_DEBUG_INFO
	 	printf("%s==>功率因数:整数=%d,小数=%02d%02d,Factor=%x\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
	 #endif
	 
	  if (sign)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 4;
		
	  //电压频率
	  offset = METER_STATUS_WORD;    //2018-4-27,电压频率放在电表第一个状态字里 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
		
   #ifdef SHOW_DEBUG_INFO
	  printf("%s==>频率:整数=%d,小数=%02d%02d\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = hexToBcd(retVal[1]);
	  *(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(retVal[2]);
	  tmpTail+=4;
 	}
	else if (2==multiCopy[arrayItem].copyItem)
	{
	  //有当前电量数据	  
		multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //正向有功电能,单位0.01kWh
	  offset = POSITIVE_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
		
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>正向有功电量=%x\n", __func__, tmpData);
	 #endif
	 
		*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
		//反向有功电能,单位0.01kWh
		offset = NEGTIVE_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>反向有功电量=%x\n", __func__, tmpData);
	 #endif 
		*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

    //有功电能净值,有功电能总和
		tmpTail += 8;
		
	  //正向无功电能,单位0.01kvarh
	  offset = POSITIVE_NO_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>正向无功电量=%x\n", __func__, tmpData);
	 #endif
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
		//反向无  功电能,单位0.01kvarh
		offset = NEGTIVE_NO_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>反向无功电量=%x\n", __func__, tmpData);
	 #endif
		*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
  }

  return METER_REPLY_ANALYSE_OK;  //接收数据帧正常且解析正确,已保存进缓存
}


#endif


/***************************************************
函数名称:processWasionData
功能描述:威盛规约数据项处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，pDataEnd数据段结束指针,
         measurePoint
输出参数:
返回值：
***************************************************/
void processWasionData(INT8U measureObject, INT8U *pFrameHead)
{
    /*INT8U type, i;
    INT8U loadLength; 
    REAL_DATA_WITH_TARIFF  data;
    
    //type = *(pFrameHead+3) &
    loadLength = *(pFrameHead + 4);
    
    data.measureObject = measureObject;
    
    //读取终端时间存储在data.time中
    data.time.second = copyTime.second/10<<4 | copyTime.second%10;
    data.time.minute = copyTime.minute/10<<4 | copyTime.minute%10;
    data.time.hour   = copyTime.hour  /10<<4 | copyTime.hour  %10;
    data.time.day    = copyTime.day   /10<<4 | copyTime.day   %10;
    data.time.month  = copyTime.month /10<<4 | copyTime.month %10;
    data.time.year   = copyTime.year  /10<<4 | copyTime.year  %10;
    
    data.type = findWasionType(*(pFrameHead+2), *(pFrameHead+3));
    
    //未完成
    for (i = 0; i < 15; i++)
    {
    	data.data[0] = 0x00000000;
    }*/
}

/***************************************************
函数名称:processWasionPacket
功能描述:威盛规约数据包处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，pDataEnd数据段结束指针,
         measurePoint
输出参数:
返回值：
***************************************************/
void processWasionPacket(INT8U measureObject, INT8U *pFrameHead)
{
}

/***************************************************
函数名称:processWasionTotal
功能描述:威盛规约数据包处理
调用函数:
被调用函数:
输入参数:pDataHead数据段起始指针，pDataEnd数据段结束指针,
         measurePoint
输出参数:
返回值：
***************************************************/
void processWasionTotal(INT8U measureObject, INT8U *pFrameHead)
{
}

/***************************************************
函数名称:findParamType
功能描述:根据电表数据中DI的确定参数数据类别
调用函数:
被调用函数:
输入参数:di
输出参数:
返回值：数据类别
***************************************************/
INT16U findDataOffset(INT8U protocol, INT8U *di)
{
    INT16U    ret=0x200;
    INT8U     i;
    char      tmpChrDi[100];
    
    switch (protocol)
    {
      #ifdef PROTOCOL_645_1997
        case DLT_645_1997:        //DL/T645-1997
      #endif
      #ifdef PROTOCOL_PN_WORK_NOWORK_97
        case PN_WORK_NOWORK_1997: //DL/T645-1997只抄正向有无功电能示值
      #endif
      
      #if PROTOCOL_645_1997
        if ((*(di+1)==0x94) || *(di+1)==0x95 || *(di+1)==0xa4 || *(di+1)==0xa5 || *(di+1)==0xb4 || *(di+1)==0xb5)  //上月数据
       	{
      	  //电能量、需量及需量时间
      	  for (i = 0; i < TOTAL_CMD_LASTMONTH_645_1997; i++)
      	  {
      	    if (cmdDlt645LastMonth1997[i][0] == *(di+1) && (cmdDlt645LastMonth1997[i][1] == *di))
      	    {
      	      ret = (cmdDlt645LastMonth1997[i][2]) | ((cmdDlt645LastMonth1997[i][3])<<8);
      	      break;
      	    }
      	  }
      	}
      	else      		//当前数据
      	{
          for (i = 0; i < TOTAL_CMD_CURRENT_645_1997; i++)
        	{
        	  if (cmdDlt645Current1997[i][0] == *(di+1) && (cmdDlt645Current1997[i][1] == *di))
            {
              ret = (cmdDlt645Current1997[i][2]) | ((cmdDlt645Current1997[i][3])<<8);
              break;
            }
      	  }
      	}
      	break;
      #endif  //PROTOCOL_645_1997
      
      #ifdef PROTOCOL_SINGLE_PHASE_97
        case SINGLE_PHASE_645_1997:  //单相645表
      #endif
      #if PROTOCOL_SINGLE_PHASE_97
        if (*(di+1)==0x9a)   //上一次日冻结数据
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_SINGLE_97; i++)
          {
        	  if (single1997LastDay[i][0] == *(di+1) && (single1997LastDay[i][1] == *di))
            {
              ret = (single1997LastDay[i][2]) | ((single1997LastDay[i][3])<<8);
              break;
            }
          }
        }
        else
        {
          for (i = 0; i < TOTAL_CMD_SINGLE_645_97; i++)
          {
        	  if (single1997[i][0] == *(di+1) && (single1997[i][1] == *di))
            {
              ret = (single1997[i][2]) | ((single1997[i][3])<<8);
              break;
            }
          }
      	}
      	break;
      #endif  //PROTOCOL_SINGLE_PHASE_97
            	
      #ifdef PROTOCOL_645_2007
        case DLT_645_2007:  //DL/T645-2007
      #endif
      #ifdef PROTOCOL_PN_WORK_NOWORK_07
        case PN_WORK_NOWORK_2007: //DL/T645-2007只抄正向有无功电能示值
      #endif
      #if PROTOCOL_645_2007
          if (((*(di+3)==0x00) || (*(di+3)==0x01)) && ((*(di+1)==0xff) || (*(di+1)==0x00)) && (*di==0x1))
          {
            for (i = 0; i < TOTAL_CMD_LASTMONTH_645_2007; i++)
        	  {
        	    if (cmdDlt645LastMonth2007[i][0] == *(di+3)
        	  	   && (cmdDlt645LastMonth2007[i][1]==*(di+2))
        	  	    && (cmdDlt645LastMonth2007[i][2]==*(di+1))
        	  	     && (cmdDlt645LastMonth2007[i][3]==*di)
        	  	   )
              {
                ret = (cmdDlt645LastMonth2007[i][4]) | ((cmdDlt645LastMonth2007[i][5])<<8);
                break;
              }
            }
          }
          else
          {
            if ((*(di+3)==0x05) && (*(di+2)==0x06) && (*di==0x1))
            {
              for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
          	  {
          	    if (cmdDlt645LastDay2007[i][0] == *(di+3)
          	  	   && (cmdDlt645LastDay2007[i][1]==*(di+2))
          	  	    && (cmdDlt645LastDay2007[i][2]==*(di+1))
          	  	     && (cmdDlt645LastDay2007[i][3]==*di)
          	  	   )
                {
                  ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
                  break;
                }
              }
            }
            else
            {          	
              for (i = 0; i < TOTAL_CMD_CURRENT_645_2007; i++)
          	  {
          	    if (cmdDlt645Current2007[i][0] == *(di+3)
          	  	   && (cmdDlt645Current2007[i][1]==*(di+2))
          	  	    && (cmdDlt645Current2007[i][2]==*(di+1))
          	  	     && (cmdDlt645Current2007[i][3]==*di)
          	  	   )
                {
                  ret = (cmdDlt645Current2007[i][4]) | ((cmdDlt645Current2007[i][5])<<8);
                  break;
                }
        	  }
        	}
      	  }      	
      	break;
      #endif
      
      #ifdef PROTOCOL_SINGLE_PHASE_07
        case SINGLE_PHASE_645_2007:         //DL/T645-2007单相智能表
        case SINGLE_PHASE_645_2007_TARIFF:
        case SINGLE_PHASE_645_2007_TOTAL:
      #endif      
      #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
        case SINGLE_LOCAL_CHARGE_CTRL_2007: //DL/T645-2007单相本地费控表
      #endif
      #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
        case SINGLE_REMOTE_CHARGE_CTRL_2007://DL/T645-2007单相远程费控表
      #endif
      #if PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
        if ((*(di+3)==0x05) &&  (*(di+2)==0x06))  //上一次日冻结数据
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_SINGLE_07; i++)
    	    {
    	      if (single2007LastDay[i][0] == *(di+3)
    	  	     && (single2007LastDay[i][1]==*(di+2))
    	  	      && (single2007LastDay[i][2]==*(di+1))
    	  	       && (single2007LastDay[i][3]==*di)
    	  	     )
            {
              ret = (single2007LastDay[i][4]) | ((single2007LastDay[i][5])<<8);
              break;
            }
          }
        }
        else
        {
          for (i = 0; i < TOTAL_CMD_SINGLE_LOCAL_CTRL_07; i++)
    	    {
    	      if (single2007[i][0] == *(di+3)
    	  	     && (single2007[i][1]==*(di+2))
    	  	      && (single2007[i][2]==*(di+1))
    	  	       && (single2007[i][3]==*di)
    	  	     )
            {
              ret = (single2007[i][4]) | ((single2007[i][5])<<8);
              break;
            }
          }
  	    }
      	break;
      #endif
      
      #ifdef PROTOCOL_THREE_2007
        case THREE_2007:                   //DL/T645-2007三相智能表
      #endif
      #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
        case THREE_LOCAL_CHARGE_CTRL_2007: //DL/T645-2007三相本地费控表
      #endif
      #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
        case THREE_REMOTE_CHARGE_CTRL_2007://DL/T645-2007三相远程费控表
      #endif
      #if PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007
        if ((*(di+3)==0x05) &&  (*(di+2)==0x06))  //上一次日冻结数据
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
    	    {
    	      if (cmdDlt645LastDay2007[i][0] == *(di+3)
    	  	     && (cmdDlt645LastDay2007[i][1]==*(di+2))
    	  	      && (cmdDlt645LastDay2007[i][2]==*(di+1))
    	  	       && (cmdDlt645LastDay2007[i][3]==*di)
    	  	     )
            {
              ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
              break;
            }
          }
        }
        else
        {
          for (i = 0; i < TOTAL_CMD_THREE_LOCAL_CTRL_07; i++)
    	    {
    	      if (three2007[i][0] == *(di+3)
    	  	     && (three2007[i][1]==*(di+2))
    	  	      && (three2007[i][2]==*(di+1))
    	  	       && (three2007[i][3]==*di)
    	  	     )
            {
              ret = (three2007[i][4]) | ((three2007[i][5])<<8);
              break;
            }
          }
  	    }
      	break;
      #endif

      #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
       case KEY_HOUSEHOLD_2007:                   //DL/T645-2007重点用户
      #endif
      #if PROTOCOL_KEY_HOUSEHOLD_2007
        if ((*(di+3)==0x05) &&  (*(di+2)==0x06))  //上一次日冻结数据
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
    	    {
    	      if (cmdDlt645LastDay2007[i][0] == *(di+3)
    	  	     && (cmdDlt645LastDay2007[i][1]==*(di+2))
    	  	      && (cmdDlt645LastDay2007[i][2]==*(di+1))
    	  	       && (cmdDlt645LastDay2007[i][3]==*di)
    	  	     )
            {
              ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
              break;
            }
          }
        }
        else
        {
        	for (i = 0; i < TOTAL_CMD_KEY_2007; i++)
 	        {
     	       if (keyHousehold2007[i][0] == *(di+3)
     	  	      && (keyHousehold2007[i][1]==*(di+2))
     	  	       && (keyHousehold2007[i][2]==*(di+1))
     	  	        && (keyHousehold2007[i][3]==*di)
     	  	      )
              {
                ret = (keyHousehold2007[i][4]) | ((keyHousehold2007[i][5])<<8);
                break;
              }
          }
        }
        break;
      #endif
      
      case DOT_COPY_1997:                   //DL/T645-1997点抄规约
        for (i = 0; i < 3; i++)
      	{
      	  if (dotCopy1997[i][0] == *(di+1) && (dotCopy1997[i][1] == *di))
          {
            ret = (dotCopy1997[i][2]) | ((dotCopy1997[i][3])<<8);
            break;
          }
    	  }
        break;

      case DOT_COPY_2007:                   //DL/T645-2007点抄规约
      	for (i = 0; i < 3; i++)
        {
   	       if (dotCopy2007[i][0] == *(di+3)
   	  	      && (dotCopy2007[i][1]==*(di+2))
   	  	       && (dotCopy2007[i][2]==*(di+1))
   	  	        && (dotCopy2007[i][3]==*di)
   	  	      )
            {
              ret = (dotCopy2007[i][4]) | ((dotCopy2007[i][5])<<8);
              break;
            }
        }
        break;

      case HOUR_FREEZE_2007:                //07冻结数据规约
      	for (i = 0; i < 24; i++)
        {
   	       if (hourFreeze2007[i][0] == *(di+3)
   	  	      && (hourFreeze2007[i][1]==*(di+2))
   	  	       && (hourFreeze2007[i][2]==*(di+1))
   	  	        && (hourFreeze2007[i][3]==*di)
   	  	      )
            {
              ret = (hourFreeze2007[i][4]) | ((hourFreeze2007[i][5])<<8);
              break;
            }
        }
        break;
        
     #ifdef PROTOCOL_EDMI_GROUP
      case EDMI_METER:
      	if (
      		  (((*(di+1)&0x20) && (*(di+1)<0x30)) || ((*(di+1)&0x40) && (*(di+1)<0x50)))
      		  && (*di!=0xe0)
      		 )
      	{
        	for (i=0; i<TOTAL_COMMAND_LASTMONTH_EDMI; i++)
          {
     	      if ((cmdEdmiLastMonth[i][1]==*(di+1))
     	  	        && (cmdEdmiLastMonth[i][0]==*di)
     	  	     )
            {
              ret = (cmdEdmiLastMonth[i][2]) | ((cmdEdmiLastMonth[i][3])<<8);
              break;
            }
          }
      	}
      	else
      	{
        	for (i=0; i<TOTAL_COMMAND_REAL_EDMI; i++)
          {
     	      if ((cmdEdmi[i][1]==*(di+1))
     	  	        && (cmdEdmi[i][0]==*di)
     	  	     )
            {
              ret = (cmdEdmi[i][2]) | ((cmdEdmi[i][3])<<8);
              break;
            }
          }
        }
      	break;
     #endif
     
     #ifdef LANDIS_GRY_ZD_PROTOCOL
      case SIMENS_ZD_METER:
      	strcpy(tmpChrDi, (char *)di);
      	for(i=0; i<strlen(tmpChrDi); i++)
        {
        	if (tmpChrDi[i]=='(')
        	{
        		break;
          }
        }
        tmpChrDi[i] = '\0';
        
        for (i=0; i<TOTAL_DATA_ITEM_ZD; i++)
        {
      	  if (strcmp(landisChar[i],tmpChrDi)==0)
      	  {
      	  	ret = i;
      	  	break;
      	  }
      	}
      	break;
     #endif
      
      default:
      	ret = 0x200;   //待完成
      	break;
    }

    return ret;
}
