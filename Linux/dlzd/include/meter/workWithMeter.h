/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：workWithMeter.h
作者：TianYe
版本：0.9
完成日期:2006年5月31日
描述：终端与电表通信头文件
修改历史:
  01,06-5-31,Tianye created.
  02,09-03-09,Leiyong规范化文件.详细注释各定义宏的用途.
  03,10-01-14,Leiyong修改存储偏移适应DL/T645-1997和DL/T645-2007.
**************************************************/
#ifndef __INCworkWithMeterh
#define __INCworkWithMeterh

#include "common.h"
#include "timeUser.h"

//电科院送?
//#define  DKY_SUBMISSION

//2.发送给电能表的帧大小
#define  SIZE_OF_METER_FRAME                   64

//3.数据类别
#define  POSITIVE_WORK                         0x1   //正向有功电示值
#define  POSITIVE_NO_WORK                      0x2   //正向无功电示值
#define  NEGTIVE_WORK                          0x3   //反向有功电示值
#define  NEGTIVE_NO_WORK                       0x4   //反向无功电示值
#define  QUA1_NO_WORK                          0x5   //一象限无功电示值
#define  QUA2_NO_WORK                          0x6   //二象限无功电示值
#define  QUA3_NO_WORK                          0x7   //三象限无功电示值
#define  QUA4_NO_WORK                          0x8   //四象限无功电示值

#define  REQ_TIME_P_WORK                       0x9   //正向有功最大需量发生时间
#define  REQ_TIME_P_NO_WORK                    0xA   //正向无功最大需量发生时间
#define  REQ_TIME_N_WORK                       0xB   //反向有功最大需量发生时间
#define  REQ_TIME_N_NO_WORK                    0xC   //反向无功最大需量发生时间
#define  QUA1_REQ_TIME_NO_WORK                 0xD   //一象限无功最大需量发生时间
#define  QUA2_REQ_TIME_NO_WORK                 0xE   //二象限无功最大需量发生时间
#define  QUA3_REQ_TIME_NO_WORK                 0xF   //三象限无功最大需量发生时间
#define  QUA4_REQ_TIME_NO_WORK                0x10   //四象限无功最大需量发生时间

#define  REQ_POSITIVE_WORK                    0x11   //正向有功最大需量
#define  REQ_POSITIVE_NO_WORK                 0x12   //正向无功最大需量
#define  REQ_NEGTIVE_WORK                     0x13   //反向有功最大需量
#define  REQ_NEGTIVE_NO_WORK                  0x14   //反向无功最大需量
#define  QUA1_REQ_NO_WORK                     0x15   //一象限无功最大需量
#define  QUA2_REQ_NO_WORK                     0x16   //二象限无功最大需量
#define  QUA3_REQ_NO_WORK                     0x17   //三象限无功最大需量
#define  QUA4_REQ_NO_WORK                     0x18   //四象限无功最大需量

//3-1.变量数据结算扩展类别
#define  WORK_ENERGY                          0x1D   //有功电能量
#define  NO_WORK_ENERGY                       0x1E   //无功电能量
#define  MONTH_WORK_ENERGY                    0x1F   //月有功电能量
#define  MONTH_NO_WORK_ENERGY                 0x20   //月无功电能量

#define  DAY_P_WORK                           0x21   //日正向有功电能量
#define  DAY_P_NO_WORK                        0x22   //日正向无功电能量
#define  DAY_N_WORK                           0x23   //日反向有功电能量
#define  DAY_N_NO_WORK                        0x24   //日反向无功电能量

#define  MONTH_P_WORK                         0x31   //月正向有功电能量
#define  MONTH_P_NO_WORK                      0x32   //月正向无功电能量
#define  MONTH_N_WORK                         0x33   //月反向有功电能量
#define  MONTH_N_NO_WORK                      0x34   //月反向无功电能量

#define  MONTH_NO_WORK_QUA1                   0x35   //月一象限无功电能量
#define  MONTH_NO_WORK_QUA2                   0x36   //月二象限无功电能量
#define  MONTH_NO_WORK_QUA3                   0x37   //月三象限无功电能量
#define  MONTH_NO_WORK_QUA4                   0x38   //月四象限无功电能量

#define  COPYDAY_FREEZE_POSI_WORK             0x70   //抄表日冻结正向有功示值
#define  COPYDAY_FREEZE_POSI_NO_WORK          0x71   //抄表日冻结正向无功示值
#define  COPYDAY_FREEZE_NEG_WORK              0x72   //抄表日冻结反向有功示值
#define  COPYDAY_FREEZE_NEG_NO_WORK           0x73   //抄表日冻结反向无功示值

#define  COPYDAY_FREEZE_NO_WORK_QUA1          0x74   //抄表日冻结一象限无功示值
#define  COPYDAY_FREEZE_NO_WORK_QUA2          0x75   //抄表日冻结二象限无功功示值
#define  COPYDAY_FREEZE_NO_WORK_QUA3          0x76   //抄表日冻结三象限无功功示值
#define  COPYDAY_FREEZE_NO_WORK_QUA4          0x77   //抄表日冻结四象限无功功示值

#define  COPYDAY_FREEZE_POSI_WORK_REQ_TIME    0x78   //抄表日冻结正向有功最大需量发生时间
#define  COPYDAY_FREEZE_POSI_NO_WORK_REQ_TIME 0x79   //抄表日冻结正向无功最大需量发生时间
#define  COPYDAY_FREEZE_NEG_WORK_REQ_TIME     0x7A   //抄表日冻结反向有功最大需量发生时间
#define  COPYDAY_FREEZE_NEG_NO_WORK_REQ_TIME  0x7B   //抄表日冻结反向无功最大需量发生时间

#define  COPYDAY_FREEZE_POSI_WORK_REQ         0x7C   //抄表日冻结正向有功最大需量
#define  COPYDAY_FREEZE_POSI_NO_WORK_REQ      0x7D   //抄表日冻结正向无功最大需量
#define  COPYDAY_FREEZE_NEG_WORK_REQ          0x7E   //抄表日冻结反向有功最大需量
#define  COPYDAY_FREEZE_NEG_NO_WORK_REQ       0x7F   //抄表日冻结反向无功最大需量

#define  POWER_DATA_REFERENCE                 0x40   //用于判别电表事件发生的参考记录

//3-2.参变量数据结算扩展类别(存储于变量结算数据区)
#define  STASTIC_POWER                        0x60   //功率统计数据
#define  STASTIC_VOLTAGE                      0x61   //电压统计数据
#define  STASTIC_CURRENT                      0x62   //电流统计数据
#define  POWER_SUPERIOD                       0x63   //视在功率越限统计
#define  PHASE_DOWN_STATISTIC                 0x64   //断相统计
#define  REQ_STATISTIC                        0x65   //最大需量统计


//4.实时数据页内存储偏移(1,2)
//每次抄表(抄全部数据的话)每块表的数据需存储4页,第1页为示值,第2页为需量,第3参变量,第3页为时段有关参变量
//与费率相关的数据留有一个总及8个费率(即9个费率)的存储空间,
//如果要增加费率做的改动将会非常大,当然要从实时数据存储开始改起,其次是结算等数据
//4-0.每种类型的记录长度
#define  LENGTH_OF_ENERGY_RECORD               344   //电能量记录长度
#define  LENGTH_OF_REQ_RECORD                  288   //需量记录长度
#define  LENGTH_OF_PARA_RECORD                 294   //参量变量记录长度
#define  LENGTH_OF_SHIDUAN_RECORD              487   //时段记录长度
#define  LENGTH_OF_KEY_ENERGY_RECORD           144   //重点用户电能量记录长度
#define  LENGTH_OF_KEY_PARA_RECORD              42   //重点用户参变量记录长度

//4-1.示值及参数量页内存储偏移(1页内存储1块电表抄1次的示值及偏移)
//4-1-1.示值(示值数据均为4个字节,存储格式为xxxxxx.xx(BCD码),低位存储在存储区的低字节)
#define  POSITIVE_WORK_OFFSET                    0   //正向有功电能示值((1+8)*4Bytes)
#define  NEGTIVE_WORK_OFFSET                    36   //反向有功电能示值((1+8)*4Bytes)
#define  POSITIVE_NO_WORK_OFFSET                72   //正向无功电能示值((1+8)*4Bytes)(DL/T645-2007称其为组合无功1)
#define  NEGTIVE_NO_WORK_OFFSET                108   //反向无功电能示值((1+8)*4Bytes)(DL/T645-2007称其为组合无功2)
#define  QUA1_NO_WORK_OFFSET                   144   //一象限无功电能示值((1+8)*4Bytes)
#define  QUA4_NO_WORK_OFFSET                   180   //四象限无功电能示值((1+8)*4Bytes)
#define  QUA2_NO_WORK_OFFSET                   216   //二象限无功电能示值((1+8)*4Bytes)
#define  QUA3_NO_WORK_OFFSET                   252   //三象限无功电能示值((1+8)*4Bytes)
#define  COPPER_LOSS_TOTAL_OFFSET              288   //铜损有功总电能示值(补偿量,4bytes)
#define  IRON_LOSS_TOTAL_OFFSET                292   //铁损有功总电能示值(补偿量,4bytes)
#define  POSITIVE_WORK_A_OFFSET                296   //A相正向有功电能(4Bytes)
#define  NEGTIVE_WORK_A_OFFSET                 300   //A相反向有功电能(4Bytes)
#define  COMB1_NO_WORK_A_OFFSET                304   //A相组合无功1电能(4Bytes)
#define  COMB2_NO_WORK_A_OFFSET                308   //A相组合无功2电能(4Bytes)
#define  POSITIVE_WORK_B_OFFSET                312   //B相正向有功电能(4Bytes)
#define  NEGTIVE_WORK_B_OFFSET                 316   //B相反向有功电能(4Bytes)
#define  COMB1_NO_WORK_B_OFFSET                320   //B相组合无功1电能(4Bytes)
#define  COMB2_NO_WORK_B_OFFSET                324   //B相组合无功2电能(4Bytes)
#define  POSITIVE_WORK_C_OFFSET                328   //C相正向有功电能(4Bytes)
#define  NEGTIVE_WORK_C_OFFSET                 332   //C相反向有功电能(4Bytes)
#define  COMB1_NO_WORK_C_OFFSET                336   //C相组合无功1电能(4Bytes)
#define  COMB2_NO_WORK_C_OFFSET                340   //C相组合无功2电能(4Bytes)
//4-1,本页结束于第344字节***************************


//4-2需量记录存储偏移(1条记录存储1块电表抄1次的需量及发生时间)
//4-2.1 DL/T645-1997和DL/T645-2007的需量是3个字节(,存储格式为xx.xxxx(BCD码),低位存储在存储区的低字节)
//4-2.2 DL/T645-1997的需量发生时间为4个字节(分时日月,BCD码),DL/T645-2007的需量发生时间为5个字节(分时日月年,BCD码)
//      为了统一存储,存储为5个字节,DL/T645-1997的数据因为没有年则年这个字节存储为0xee
#define  REQ_POSITIVE_WORK_OFFSET                0   //正向有功最大需量((1+8)*3Bytes)
#define  REQ_TIME_P_WORK_OFFSET                 27   //正向有功最大需量发生时间((1+8)*5Bytes)
#define  REQ_POSITIVE_NO_WORK_OFFSET            72   //正向无功最大需量((1+8)*3Bytes)
#define  REQ_TIME_P_NO_WORK_OFFSET              99   //正向无功最大需量发生时间((1+8)*5Bytes)
#define  REQ_NEGTIVE_WORK_OFFSET               144   //反向有功最大需量((1+8)*3Bytes)
#define  REQ_TIME_N_WORK_OFFSET                171   //反向有功最大需量发生时间((1+8)*5Bytes)
#define  REQ_NEGTIVE_NO_WORK_OFFSET            216   //反向无功最大需量((1+8)*3Bytes)
#define  REQ_TIME_N_NO_WORK_OFFSET             243   //反向无功最大需量发生时间((1+8)*5Bytes)
//ly,2010-01-14,原来是抄了一二三四象限的需量及发生时间的,但有些表不支持,376.1-2009也没有这几个数据的要求,
//因此删除以下几行.如果需要抄一二三四象限的话,把以下几行的注释打开即可. 
//4-2本页结束于第288字节***************************
//#define  QUA1_REQ_NO_WORK_OFFSET               296   //一象限无功最大需量((1+8)*3Bytes)
//#define  QUA1_REQ_TIME_NO_WORK_OFFSET          323   //一象限无功最大需量发生时间((1+8)*5Bytes)
//#define  QUA2_REQ_NO_WORK_OFFSET               368   //二象限无功最大需量((1+8)*3Bytes)
//#define  QUA2_REQ_TIME_NO_WORK_OFFSET          395   //二象限无功最大需量发生时间((1+8)*5Bytes)
//#define  QUA3_REQ_NO_WORK_OFFSET               440   //三象限无功最大需量((1+8)*3Bytes)
//#define  QUA3_REQ_TIME_NO_WORK_OFFSET          467   //三象限无功最大需量发生时间((1+8)*5Bytes)
//#define  QUA4_REQ_NO_WORK_OFFSET               512   //四象限无功最大需量((1+8)*3Bytes)
//#define  QUA4_REQ_TIME_NO_WORK_OFFSET          539   //四象限无功最大需量发生时间((1+8)*5Bytes)
//4-2x如果加上一二三四象限的最大需量及发生时间的话,本页结束于第584字节***************************


//4-3.参变量
//4-3.1电压有效值2bytes(97[xxx.0],2007[xxx.x])
#define  VOLTAGE_PHASE_A                         0   //A相电压(2Bytes)
#define  VOLTAGE_PHASE_B                         2   //B相电压(2Bytes)
#define  VOLTAGE_PHASE_C                         4   //C相电压(2Bytes)

//4-3.2电流有效值(DL/T645-1997为2bytes,DL/T645-2007为3bytes,存储为3个字节[xxx.xxx(97存储后的格式为0xx.xx0)])
//97抄不到零序电流,而07的零线电流为3bytes,存储为3bytes,97没有则为0xeeeeee,抄回的数据带电流正/反向标志,不用加标志的
#define  CURRENT_PHASE_A                         6   //A相电流(3Bytes)
#define  CURRENT_PHASE_B                         9   //B相电流(3Bytes)
#define  CURRENT_PHASE_C                        12   //C相电流(3Bytes)
#define  ZERO_SERIAL_CURRENT                    15   //零序电流(3Bytes)

//4-3.3瞬时有功功率3bytes(xx.xxxx),抄回的数据带电流正/反向标志,不用加标志的
#define  POWER_INSTANT_WORK                     18   //瞬时有功功率(3Bytes)
#define  POWER_PHASE_A_WORK                     21   //A相有功功率(3Bytes)
#define  POWER_PHASE_B_WORK                     24   //B相有功功率(3Bytes)
#define  POWER_PHASE_C_WORK                     27   //C相有功功率(3Bytes)

//4-3.4瞬时无功功率(DL/T645-1997为2bytes,DL/T645-2007为3bytes,存储为3个字节[xx.xxxx(97存储后的格式为xx.xx00)])
//抄回的数据带电流正/反向标志,不用加标志的
#define  POWER_INSTANT_NO_WORK                  30   //瞬时无功功率(3Bytes)
#define  POWER_PHASE_A_NO_WORK                  33   //A相无功功率(3Bytes)
#define  POWER_PHASE_B_NO_WORK                  36   //B相无功功率(3Bytes)
#define  POWER_PHASE_C_NO_WORK                  39   //C相无功功率(3Bytes)

//4-3.5瞬时视在功率(DL/T645-1997没有,DL/T645-2007为3bytes,存储为3个字节[xx.xxxx(97存储后的格式为0xeeeeee)])
#define  POWER_INSTANT_APPARENT	                42   //瞬时视在功率(3Bytes)
#define  POWER_PHASE_A_APPARENT                 45   //A相视在功率(3Bytes)
#define  POWER_PHASE_B_APPARENT                 48   //B相视在功率(3Bytes)
#define  POWER_PHASE_C_APPARENT                 51   //C相视在功率(3Bytes)

//4-3.5功率因数2bytes(x.xxx)
#define  TOTAL_POWER_FACTOR                     54   //总功率因数(2Bytes)
#define  FACTOR_PHASE_A                         56   //A相功率因数(2Bytes)
#define  FACTOR_PHASE_B                         58   //B相功率因数(2Bytes)
#define  FACTOR_PHASE_C                         60   //C相功率因数(2Bytes)

//4-3.6断相次数及断相时间
//DL/T645-1997断相次数为2bytes,DL/T645-2007断相次数为3bytes,存储为3bytes(NNNNNN,BCD码(DL/T645-1997存储后的格式为00NNNN))
//DL/T645-1997和DL/T645-2007的断相累计时间均为3bytes,存储为3bytes(NNNNNN,BCD码)
#define  PHASE_DOWN_TIMES                       62   //总断相次数(3Bytes)
#define  TOTAL_PHASE_DOWN_TIME                  65   //总断相时间(3Bytes)
#define  PHASE_A_DOWN_TIMES                     68   //A相断相次数(3Bytes)
#define  TOTAL_PHASE_A_DOWN_TIME                71   //A相断相时间(3Bytes)
#define  PHASE_B_DOWN_TIMES                     74   //B相断相次数(3Bytes)
#define  TOTAL_PHASE_B_DOWN_TIME                77   //B相断相时间(3Bytes)
#define  PHASE_C_DOWN_TIMES                     80   //C相断相次数(3Bytes)
#define  TOTAL_PHASE_C_DOWN_TIME                83   //C相断相时间(3Bytes)

//4-3.7断相起始时间和结束时间
//DL/T645-1997时间为4bytes(分时日月),DL/T645-2007时间为6bytes(秒分时日月年),存储为6bytes(ssmmhhDDMMYY)
#define  LAST_PHASE_DOWN_BEGIN                  86   //最近一次断相起始时间(6Bytes)
#define  LAST_PHASE_DOWN_END                    92   //最近一次断相结束时间(6Bytes)
#define  LAST_PHASE_A_DOWN_BEGIN                98   //最近一次A相断相起始时间(6Bytes)
#define  LAST_PHASE_A_DOWN_END                 104   //最近一次A相断相结束时间(6Bytes)
#define  LAST_PHASE_B_DOWN_BEGIN               110   //最近一次B相断相起始时间(6Bytes)
#define  LAST_PHASE_B_DOWN_END                 116   //最近一次B相断相结束时间(6Bytes)
#define  LAST_PHASE_C_DOWN_BEGIN               122   //最近一次C相断相起始时间(6Bytes)
#define  LAST_PHASE_C_DOWN_END                 128   //最近一次C相断相结束时间(6Bytes)

//4-3.8次数及最近发生时间
//4-3.8.1编程次数及发生时间
#define  PROGRAM_TIMES                         134   //编程次数(3bytes)(97是2Bytes,2007是3bytes,存储为3bytes(NNNNNN,BCD码,97存储后为00NNNN))
#define  LAST_PROGRAM_TIME                     137   //最近一次编程时间(6bytes)(97是4bytes,2007是6bytes,存储为6bytes(ssmmhhDDMMYY,97存储后的格式为00mmhhDDMM00))
//4-3.8.2电表清零次数及发生时间
#define  METER_CLEAR_TIMES                     143   //电表清次数(3bytes)(97没有,2007是3bytes,存储为3bytes(NNNNNN,BCD码,97存储后为0xeeeeee))
#define  LAST_METER_CLEAR_TIME                 146   //电表清零最近一次发生时间(6bytes)(97没有,2007是6bytes,存储为6bytes(ssmmhhDDMMYY,97存储后的格式为0xeeeeeeeeeeee))
//4-3.8.3需量清零总次数及最近一次清零时间
#define  UPDATA_REQ_TIME                       152   //最大需量清零次数(3bytes)(97是2Bytes,2007是3bytes,存储为3bytes(NNNNNN,BCD码,97存储后为00NNNN))
#define  LAST_UPDATA_REQ_TIME                  155   //最近一次最大需量清零时间(6bytes)(97是4bytes,2007是6bytes,存储为6bytes(ssmmhhDDMMYY,97存储后的格式为00mmhhDDMM00))
//4-3.8.4事件清零总次数及最近一次清零时间
#define  EVENT_CLEAR_TIMES                     161   //事件清零次数(3bytes)(97没有,2007是3bytes,存储为3bytes(NNNNNN,BCD码,97存储后为0xeeeeee))
#define  EVENT_CLEAR_LAST_TIME                 164   //最近一次事件清零时间(6bytes)(97没有,2007是6bytes,存储为6bytes(ssmmhhDDMMYY,97存储后的格式为0xeeeeeeeeeeee))
//4-3.8.5校时总次数及最近一次校时时间
#define  TIMING_TIMES                          170   //校时总次数(3bytes)(97没有,2007是3bytes,存储为3bytes(NNNNNN,BCD码,97存储后为0xeeeeee))
#define  TIMING_LAST_TIME                      173   //最近一次校时时间(6bytes)(97没有,2007是6bytes,存储为6bytes(ssmmhhDDMMYY,97存储后的格式为0xeeeeeeeeeeee))

//4-3.9电表运行状态字
//DL/T645-1997只有一个字节的电表运行状态字,存储在METER_STATUS_WORD
//DL/T645-2007有7个状态字,每个状态字有2个字节
#define  METER_STATUS_WORD                     179   //电表运行状态字1(2Bytes)
#define  METER_STATUS_WORD_2                   181   //电表运行状态字2(2Bytes)
#define  METER_STATUS_WORD_3                   183   //电表运行状态字3(2Bytes)
#define  METER_STATUS_WORD_4                   185   //电表运行状态字4(2Bytes)
#define  METER_STATUS_WORD_5                   187   //电表运行状态字5(2Bytes)
#define  METER_STATUS_WORD_6                   189   //电表运行状态字6(2Bytes)
#define  METER_STATUS_WORD_7                   191   //电表运行状态字7(2Bytes)

//4-3.10电网状态字,97有一个字节,而07没有电网状态字
#define  NET_STATUS_WORD                       193   //电网状态字(1Bytes)

//4-3.11电表日期4bytes及时间3bytes,97和07相同
#define  DATE_AND_WEEK                         194   //日期及周次(4Bytes,WWDDMMYY)
#define  METER_TIME                            198   //电表时间(3Bytes,ssmmhh)

//4-3.12电池工作时间,97为3bytes,07为4bytes,存储为4bytes(NNNNNNNN,97存储后为00NNNNNN)
#define  BATTERY_WORK_TIME                     201   //电池工作时间(4Bytes)

//4-3.13电表常数各3bytes及表号6bytes,97和07相同
#define  CONSTANT_WORK                         205   //电表常数(有功,3字节)
#define  CONSTANT_NO_WORK                      208   //电表常数(无功,3字节)
#define  METER_NUMBER                          211   //表号(电表地址,6字节)

//4-3.14自动抄表日期
//97为一个自动抄表日期2bytes
//2007为三个每月结算日各2bytes
#define  AUTO_COPY_DAY                         217   //自动抄表日期/每月第1结算日(2字节)
#define  AUTO_COPY_DAY_2                       219   //每月第2结算日(2字节)
#define  AUTO_COPY_DAY_3                       221   //每月第3结算日(2字节)

//4-3.15 时段表编程总次数及上一次发生时间
#define  PERIOD_TIMES                          223   //时段表编程总次数(3bytes)(97没有,2007是3bytes,存储为3bytes(NNNNNN,BCD码,97存储后为0xeeeeee))
#define  PERIOD_LAST_TIME                      226   //最近一次时段表编程发生时间(6bytes)(97没有,2007是6bytes,存储为6bytes(ssmmhhDDMMYY,97存储后的格式为0xeeeeeeeeeeee))

//4-3.16 费控表控制/开关/购电信息
#define  LAST_JUMPED_GATE_TIME                 232   //上一次跳闸发生时刻(6字节,97没有)
#define  LAST_CLOSED_GATE_TIME                 238   //上一次合闸发生时刻(6字节,97没有)
#define  OPEN_METER_COVER_TIMES                244   //开表盖总次数(2字节,97没有)
#define  LAST_OPEN_METER_COVER_TIME            246   //上一次开表盖发生时刻(6字节,97没有)
#define  CHARGE_TOTAL_TIME                     252   //上1次购电后总购电次数(2字节)
#define  CHARGE_REMAIN_MONEY                   254   //当前剩余金额(4字节,97没有)
#define  CHARGE_TOTAL_MONEY                    258   //上一次购电后累计购电金额(4字节,97没有)
#define  CHARGE_REMAIN_QUANTITY                262   //当前剩余电量(4字节,97没有)
#define  CHARGE_OVERDRAFT_QUANTITY	           266   //当前透支电量(4字节,97没有)
#define  CHARGE_TOTAL_QUANTITY                 270   //上一次购电后累计购电量(4字节,97没有)
#define  CHARGE_OVERDRAFT_LIMIT                274   //透支电量限值(4字节,97没有)
#define  CHARGE_ALARM_QUANTITY                 278   //报警电量1限值(4字节,97没有)

//4-3.17 电压、电流相位角
#define PHASE_ANGLE_V_A                        282   //A相电压相位角(2字节)
#define PHASE_ANGLE_V_B                        284   //B相电压相位角(2字节)
#define PHASE_ANGLE_V_C                        286   //C相电压相位角(2字节)
#define PHASE_ANGLE_C_A                        288   //A相电流相位角(2字节)
#define PHASE_ANGLE_C_B                        290   //B相电流相位角(2字节)
#define PHASE_ANGLE_C_C                        292   //C相电流相位角(2字节)


//4-3本页结束于第294字节***************************


//4-4.时段有关参变量页内存储偏移(1页内存储1块电表抄1次的时段有关参变量)
#define  YEAR_SHIQU_P                            0   //年时区数(1字节)
#define  DAY_SHIDUAN_BIAO_Q                      1   //日时段表数(1字节)
#define  DAY_SHIDUAN_M                           2   //日时段数(1字节)
#define  NUM_TARIFF_K                            3   //费率数(1字节)
#define  NUM_OF_JIA_RI_N                         4   //公共假日数(1字节)
#define  YEAR_SHIDU                              5   //时区起始日期及日时段表号起始地址(预留15*3字节)
#define  DAY_SHIDUAN_BIAO                       50   //日时段表起始时间及费率号起始地址
                                                     //(预留13个日时段,每日留10个时段时段=13*10*3=390字节)
#define  JIA_RI_SHIDUAN                        440   //假日日期及时段起始地址(预留15个假日时段15*3=45字节)
#define  ZHOUXIURI_SHIDUAN                     485   //周休日采用的日时段表号
//4-4本页结束于第487字节***************************

//4-5.单相表正反向电能示值、电表运行状态字、费控相关存储偏移*****************************
#define  LENGTH_OF_SINGLE_ENERGY_RECORD         61   //电能量(单相表)记录长度
#define  LENGTH_OF_SINGLE_REMOTE_RECORD         73   //电能量(单相远程费控表)记录长度
#define  LENGTH_OF_SINGLE_LOCAL_RECORD         111   //电能量(单相本地费控表)记录长度

//4-5.1 正反向有功电能示值
#define  POSITIVE_WORK_OFFSET_S                  0   //正向有功电能示值((1+4)*4Bytes)
#define  NEGTIVE_WORK_OFFSET_S                  20   //反向有功电能示值((1+4)*4Bytes)
#define  DAY_FREEZE_TIME_FLAG_S               1024   //日冻结时标(5Bytes)

//4-5.2 电表运行状态字
#define  METER_STATUS_WORD_S                    40   //电表运行状态字1(2Bytes)
#define  METER_STATUS_WORD_S_2                  42   //电表运行状态字2(2Bytes)
#define  METER_STATUS_WORD_S_3                  44   //电表运行状态字3(2Bytes)
#define  METER_STATUS_WORD_S_4                  46   //电表运行状态字4(2Bytes)
#define  METER_STATUS_WORD_S_5                  48   //电表运行状态字5(2Bytes)
#define  METER_STATUS_WORD_S_6                  50   //电表运行状态字6(2Bytes)
#define  METER_STATUS_WORD_S_7                  52   //电表运行状态字7(2Bytes)

#define  DATE_AND_WEEK_S                        54   //电能表日期及周次(4Bytes)
#define  METER_TIME_S                           58   //电能表时间(3Bytes)
//单相表结束于第61字节*********************************************************

//4-5.3 费控表控制/开关/购电信息
#define  LAST_JUMPED_GATE_TIME_S                61   //上一次跳闸发生时刻(6字节)
#define  LAST_CLOSED_GATE_TIME_S                67   //上一次合闸发生时刻(6字节)
//单相远程费控表结束于73字节***************************************************

//4-5.4 电能表购、用电信息
#define  OPEN_METER_COVER_TIMES_S               73   //开表盖总次数(2字节,97没有)
#define  LAST_OPEN_METER_COVER_TIME_S           75   //上一次开表盖发生时刻(6字节,97没有)
#define  CHARGE_TOTAL_TIME_S                    81   //上1次购电后总购电次数(2字节)
#define  CHARGE_REMAIN_MONEY_S                  83   //当前剩余金额(4字节,97没有)
#define  CHARGE_TOTAL_MONEY_S                   87   //上一次购电后累计购电金额(4字节,97没有)
#define  CHARGE_REMAIN_QUANTITY_S               91   //当前剩余电量(4字节,97没有)
#define  CHARGE_OVERDRAFT_QUANTITY_S	          95   //当前透支电量(4字节,97没有)
#define  CHARGE_TOTAL_QUANTITY_S                99   //上一次购电后累计购电量(4字节,97没有)
#define  CHARGE_OVERDRAFT_LIMIT_S              103   //透支电量限值(4字节,97没有)
#define  CHARGE_ALARM_QUANTITY_S               107   //报警电量1限值(4字节,97没有)
//单相本地费控表结束于111字节**************************************************

//4-6.三相费控表电表状态字、费控相关存储偏移*****************************
#define  LENGTH_OF_THREE_ENERGY_RECORD         309   //电能量(三相智能表)记录长度
#define  LENGTH_OF_THREE_REMOTE_RECORD         321   //电能量(三相远程费控表)记录长度
#define  LENGTH_OF_THREE_LOCAL_RECORD          359   //电能量(三相本地费控表)记录长度

#define  DAY_FREEZE_TIME_FLAG_T               1530   //日冻结时标(5Bytes)

//4-6.1 电表运行状态字(接到电能量后面)
#define  METER_STATUS_WORD_T                   288   //电表运行状态字1(2Bytes)
#define  METER_STATUS_WORD_T_2                 290   //电表运行状态字2(2Bytes)
#define  METER_STATUS_WORD_T_3                 292   //电表运行状态字3(2Bytes)
#define  METER_STATUS_WORD_T_4                 294   //电表运行状态字4(2Bytes)
#define  METER_STATUS_WORD_T_5                 295   //电表运行状态字5(2Bytes)
#define  METER_STATUS_WORD_T_6                 306   //电表运行状态字6(2Bytes)
#define  METER_STATUS_WORD_T_7                 300   //电表运行状态字7(2Bytes)

//4-6.2 电能表日期、周次及时间
#define  DATE_AND_WEEK_T                       302   //电能表日期及周次(4Bytes)
#define  METER_TIME_T                          306   //电能表时间(3Bytes)

//三相智能表电能量记录结束于第309字节*********************************************************

//4-6.3 费控表控制/开关/购电信息
#define  LAST_JUMPED_GATE_TIME_T               309   //上一次跳闸发生时刻(6字节)
#define  LAST_CLOSED_GATE_TIME_T               315   //上一次合闸发生时刻(6字节)
//三相远程费控表电能量记录结束于第321字节*********************************************************

//4-6.4 电能表购、用电信息
#define  OPEN_METER_COVER_TIMES_T              321   //开表盖总次数(2字节,97没有)
#define  LAST_OPEN_METER_COVER_TIME_T          323   //上一次开表盖发生时刻(6字节,97没有)
#define  CHARGE_TOTAL_TIME_T                   329   //上1次购电后总购电次数(2字节)
#define  CHARGE_REMAIN_MONEY_T                 331   //当前剩余金额(4字节,97没有)
#define  CHARGE_TOTAL_MONEY_T                  335   //上一次购电后累计购电金额(4字节,97没有)
#define  CHARGE_REMAIN_QUANTITY_T              339   //当前剩余电量(4字节,97没有)
#define  CHARGE_OVERDRAFT_QUANTITY_T	         343   //当前透支电量(4字节,97没有)
#define  CHARGE_TOTAL_QUANTITY_T               347   //上一次购电后累计购电量(4字节,97没有)
#define  CHARGE_OVERDRAFT_LIMIT_T              351   //透支电量限值(4字节,97没有)
#define  CHARGE_ALARM_QUANTITY_T               355   //报警电量1限值(4字节,97没有)
//三相本地费控表结束于359字节**************************************************

//4-7 整点冻结数据
#define  LENGTH_OF_HOUR_FREEZE_RECORD            8   //整点冻结记录长度

#define  HOUR_FREEZE_P_WORK                    0x0   //整点冻结正向有功总电能(4字节)
#define  HOUR_FREEZE_N_WORK                    0x4   //整点冻结正向有功总电能(4字节)
//整点冻结数据表结束于8字节

//5.结算数据页内存储偏移(1,2)
//5-1.日当前电能量,月当前电能量记录
//各数据存储格式7bytes,第0字节为数据符号,第1,2字节为小数BCD数据,第3,4,5,6字节为整数BCD码
//存储后的格式1,2,3,4字节符号规约的数据格式13xxxx.xxxx
#define  LEN_OF_ENERGY_BALANCE_RECORD          504   //电能量结算记录长度

#define  DAY_P_WORK_OFFSET                       0   //日当前正向有功电能量((1+8)*7Bytes)
#define  DAY_N_WORK_OFFSET                      63   //日当前反向有功电能量((1+8)*7Bytes)
#define  DAY_P_NO_WORK_OFFSET                  126   //日当前正向无功电能量((1+8)*7Bytes)
#define  DAY_N_NO_WORK_OFFSET                  189   //日当前反向无功电能量((1+8)*7Bytes)
#define  MONTH_P_WORK_OFFSET                   252   //月当前正向有功电能量((1+8)*7Bytes)
#define  MONTH_N_WORK_OFFSET                   315   //月当前反向有功电能量((1+8)*7Bytes)
#define  MONTH_P_NO_WORK_OFFSET                378   //月当前正向无功电能量((1+8)*7Bytes)
#define  MONTH_N_NO_WORK_OFFSET                441   //月当前反向无功电能量((1+8)*7Bytes)
//5-1本页结束于第504字节***************************

//5-2.参变量统计值
#define  LEN_OF_PARA_BALANCE_RECORD            246   //参变量结算记录长度

//功率统计偏移
#define  MAX_TOTAL_POWER                       0                            //有功功率最大值(3Bytes)
#define  MAX_TOTAL_POWER_TIME                  (MAX_TOTAL_POWER+3)          //有功功率最大值发生时间(3Bytes)
#define  TOTAL_ZERO_POWER_TIME                 (MAX_TOTAL_POWER_TIME+3)     //有功功率为零时间(2Bytes)

#define  MAX_A_POWER                           (TOTAL_ZERO_POWER_TIME+2)    //A相最大有功功率(3Bytes)
#define  MAX_A_POWER_TIME                      (MAX_A_POWER+3)              //A相最大有功功率发生时间(3Bytes)
#define  A_ZERO_POWER_TIME                     (MAX_A_POWER_TIME+3)         //A相有功功率为零时间(2Bytes)

#define  MAX_B_POWER                           (A_ZERO_POWER_TIME+2)        //B相最大有功功率(3Bytes)
#define  MAX_B_POWER_TIME                      (MAX_B_POWER+3)              //B相最大有功功率发生时间(3Bytes)
#define  B_ZERO_POWER_TIME                     (MAX_B_POWER_TIME+3)         //B相有功功率为零时间(2Bytes)

#define  MAX_C_POWER                           (B_ZERO_POWER_TIME+2)        //C相最大有功功率(3Bytes)
#define  MAX_C_POWER_TIME                      (MAX_C_POWER+3)              //C相最大有功功率发生时间(3Bytes)
#define  C_ZERO_POWER_TIME                     (MAX_C_POWER_TIME+3)         //C相有功功率为零时间(2Bytes)

//需量统计偏移
#define  MAX_TOTAL_REQ                         (C_ZERO_POWER_TIME+2)        //三相总有功最大需量(3Bytes)
#define  MAX_TOTAL_REQ_TIME                    (MAX_TOTAL_REQ+3)            //三相总有功最大需量发生时间(3Bytes)
#define  MAX_A_REQ                             (MAX_TOTAL_REQ_TIME+3)       //A相有功最大需量(3Bytes)
#define  MAX_A_REQ_TIME                        (MAX_A_REQ+3)                //A相有功最大需量发生时间(3Bytes)
#define  MAX_B_REQ                             (MAX_A_REQ_TIME+3)           //B相有功最大需量(3Bytes)
#define  MAX_B_REQ_TIME                        (MAX_B_REQ+3)                //B相有功最大需量发生时间(3Bytes)
#define  MAX_C_REQ                             (MAX_B_REQ_TIME+3)           //C相有功最大需量(3Bytes)
#define  MAX_C_REQ_TIME                        (MAX_C_REQ+3)                //C相有功最大需量发生时间(3Bytes)

//电压统计值偏移
#define  VOL_A_UP_UP_TIME                      (MAX_C_REQ_TIME+3)           //A相电压越上上限时间(2Bytes)
#define  VOL_A_DOWN_DOWN_TIME                  (VOL_A_UP_UP_TIME+2)         //A相电压越下下限时间(2Bytes)
#define  VOL_A_UP_TIME                         (VOL_A_DOWN_DOWN_TIME+2)     //A相电压越上限时间(2Bytes)
#define  VOL_A_DOWN_TIME                       (VOL_A_UP_TIME+2)            //A相电压越下限时间(2Bytes)
#define  VOL_A_OK_TIME                         (VOL_A_DOWN_TIME+2)          //A相电压合格时间(2Bytes)

#define  VOL_B_UP_UP_TIME                      (VOL_A_OK_TIME+2)            //B相电压越上上限时间(2Bytes)
#define  VOL_B_DOWN_DOWN_TIME                  (VOL_B_UP_UP_TIME+2)         //B相电压越下下限时间(2Bytes)
#define  VOL_B_UP_TIME                         (VOL_B_DOWN_DOWN_TIME+2)     //B相电压越上限时间(2Bytes)
#define  VOL_B_DOWN_TIME                       (VOL_B_UP_TIME+2)            //B相电压越下限时间(2Bytes)
#define  VOL_B_OK_TIME                         (VOL_B_DOWN_TIME+2)          //B相电压合格时间(2Bytes)

#define  VOL_C_UP_UP_TIME                      (VOL_B_OK_TIME+2)            //C相电压越上上限时间(2Bytes)
#define  VOL_C_DOWN_DOWN_TIME                  (VOL_C_UP_UP_TIME+2)         //C相电压越下下限时间(2Bytes)
#define  VOL_C_UP_TIME                         (VOL_C_DOWN_DOWN_TIME+2)     //C相电压越上限时间(2Bytes)
#define  VOL_C_DOWN_TIME                       (VOL_C_UP_TIME+2)            //C相电压越下限时间(2Bytes)
#define  VOL_C_OK_TIME                         (VOL_C_DOWN_TIME+2)          //C相电压合格时间(2Bytes)

#define  VOL_A_MAX                             (VOL_C_OK_TIME+2)            //A相电压最大值(2Bytes)
#define  VOL_A_MAX_TIME                        (VOL_A_MAX+2)                //A相电压最大值发生时间(3Bytes)
#define  VOL_A_MIN                             (VOL_A_MAX_TIME+3)           //A相电压最小值(2Bytes)
#define  VOL_A_MIN_TIME                        (VOL_A_MIN+2)                //A相电压最小值发生时间(3Bytes)
#define  VOL_A_AVER                            (VOL_A_MIN_TIME+3)           //A相电压平均值(2Bytes)
#define  VOL_A_AVER_COUNTER                    (VOL_A_AVER+2)               //A相电压平均值计数(2Bytes)

#define  VOL_B_MAX                             (VOL_A_AVER_COUNTER+2)       //B相电压最大值(2Bytes)
#define  VOL_B_MAX_TIME                        (VOL_B_MAX+2)                //B相电压最大值发生时间(3Bytes)
#define  VOL_B_MIN                             (VOL_B_MAX_TIME+3)           //B相电压最小值(2Bytes)
#define  VOL_B_MIN_TIME                        (VOL_B_MIN+2)                //B相电压最小值发生时间(3Bytes)
#define  VOL_B_AVER                            (VOL_B_MIN_TIME+3)           //B相电压平均值(2Bytes)
#define  VOL_B_AVER_COUNTER                    (VOL_B_AVER+2)               //B相电压平均值计数(2Bytes)

#define  VOL_C_MAX                             (VOL_B_AVER_COUNTER+2)       //C相电压最大值(2Bytes)
#define  VOL_C_MAX_TIME                        (VOL_C_MAX+2)                //C相电压最大值发生时间(3Bytes)
#define  VOL_C_MIN                             (VOL_C_MAX_TIME+3)           //C相电压最小值(2Bytes)
#define  VOL_C_MIN_TIME                        (VOL_C_MIN+2)                //C相电压最小值发生时间(3Bytes)
#define  VOL_C_AVER                            (VOL_C_MIN_TIME+3)           //C相电压平均值(2Bytes)
#define  VOL_C_AVER_COUNTER                    (VOL_C_AVER+2)               //C相电压平均值计数(2Bytes)

//不平衡度越限累计时间
#define  VOL_UNBALANCE_TIME                    (VOL_C_AVER_COUNTER+2)       //电压不平衡度越限累计时间(2Bytes)
#define  CUR_UNBALANCE_TIME                    (VOL_UNBALANCE_TIME+2)       //电流不平衡度越限累计时间(2Bytes)
#define  VOL_UNB_MAX                           (CUR_UNBALANCE_TIME+2)       //电压不平衡度最大值(2Bytes)
#define  VOL_UNB_MAX_TIME                      (VOL_UNB_MAX+2)              //电压不平衡度最大值发生时间(3Bytes)
#define  CUR_UNB_MAX                           (VOL_UNB_MAX_TIME+3)         //电流不平衡度最大值(2Bytes)
#define  CUR_UNB_MAX_TIME                      (CUR_UNB_MAX+2)              //电流不平衡度最大值发生时间(3Bytes)

//电流统计
#define  CUR_A_UP_UP_TIME                      (CUR_UNB_MAX_TIME+3)         //A相电流越上上限累计时间(2Bytes)
#define  CUR_A_UP_TIME                         (CUR_A_UP_UP_TIME+2)         //A相电流越上限累计时间(2Bytes)
#define  CUR_A_MAX                             (CUR_A_UP_TIME+2)            //A相电流最大值(3Bytes)
#define  CUR_A_MAX_TIME                        (CUR_A_MAX+3)                //A相电流最大值发生时间(3Bytes)

#define  CUR_B_UP_UP_TIME                      (CUR_A_MAX_TIME+3)           //B相电流越上上限时间(2Bytes)
#define  CUR_B_UP_TIME                         (CUR_B_UP_UP_TIME+2)         //B相电流越上限时间(2Bytes)
#define  CUR_B_MAX                             (CUR_B_UP_TIME+2)            //B相电流最大值(3Bytes)
#define  CUR_B_MAX_TIME                        (CUR_B_MAX+3)                //B相电流最大值发生时间(3Bytes)

#define  CUR_C_UP_UP_TIME                      (CUR_B_MAX_TIME+3)           //C相电流越上上限累计时间(2Bytes)
#define  CUR_C_UP_TIME                         (CUR_C_UP_UP_TIME+2)         //C相电流越上限累计时间(2Bytes)
#define  CUR_C_MAX                             (CUR_C_UP_TIME+2)            //C相电流最大值(3Bytes)
#define  CUR_C_MAX_TIME                        (CUR_C_MAX+3)                //C相电流最大值发生时间(3Bytes)

#define  CUR_ZERO_UP_UP_TIME                   (CUR_C_MAX_TIME+3)           //零序电流越上上限累计时间(2Bytes)
#define  CUR_ZERO_UP_TIME                      (CUR_ZERO_UP_UP_TIME+2)      //零序电流越上限累计时间(3Bytes)
#define  CUR_ZERO_MAX                          (CUR_ZERO_UP_TIME+3)         //零序电流最大值(3Bytes)
#define  CUR_ZERO_MAX_TIME                     (CUR_ZERO_MAX+3)             //零序电流最大值发生时间(3Bytes)

//视在功率越限累计
#define  APPARENT_POWER_UP_UP_TIME             (CUR_ZERO_MAX_TIME+3)        //视在功率越上上限累计时间(2Bytes)
#define  APPARENT_POWER_UP_TIME                (APPARENT_POWER_UP_UP_TIME+2)//视在功率越上限累计时间(2Bytes)

//断相统计
#define  OPEN_PHASE_TIMES                      (APPARENT_POWER_UP_TIME+2)   //总断相次数(2Bytes)
#define  OPEN_PHASE_MINUTES                    (OPEN_PHASE_TIMES+2)         //总断相累计时间(3Bytes)
#define  OPEN_PHASE_LAST_BEG                   (OPEN_PHASE_MINUTES+3)       //最近一次断相起始时刻(4Bytes)
#define  OPEN_PHASE_LASE_END                   (OPEN_PHASE_LAST_BEG+4)      //最近一次断相结束时刻(4Bytes)
#define  A_OPEN_PHASE_TIMES                    (OPEN_PHASE_LASE_END+4)      //A相断相次数(2Bytes)
#define  A_OPEN_PHASE_MINUTES                  (A_OPEN_PHASE_TIMES+2)       //A相断相时间累计(3Bytes)
#define  A_OPEN_PHASE_LAST_BEG                 (A_OPEN_PHASE_MINUTES+3)     //A相最近断相起始时刻(4Bytes)
#define  A_OPEN_PHASE_LAST_END                 (A_OPEN_PHASE_LAST_BEG+4)    //A相最近断结束时刻(4Bytes)
#define  B_OPEN_PHASE_TIMES                    (A_OPEN_PHASE_LAST_END+4)    //B相断相次数(2Bytes)
#define  B_OPEN_PHASE_MINUTES                  (B_OPEN_PHASE_TIMES+2)       //B相断相累计时间(3Bytes)
#define  B_OPEN_PHASE_LAST_BEG                 (B_OPEN_PHASE_MINUTES+3)     //B相最近断相起始时刻(4Bytes)
#define  B_OPEN_PHASE_LAST_END                 (B_OPEN_PHASE_LAST_BEG+4)    //B相最近断相结束时刻(4Bytes)
#define  C_OPEN_PHASE_TIMES                    (B_OPEN_PHASE_LAST_END+4)    //C相断相次数(2Bytes)
#define  C_OPEN_PHASE_MINUTES                  (C_OPEN_PHASE_TIMES+2)       //C相断相累计时间(3Bytes)
#define  C_OPEN_PHASE_LAST_BEG                 (C_OPEN_PHASE_MINUTES+3)     //C相最近断相开始时刻(4Bytes)
#define  C_OPEN_PHASE_LAST_END                 (C_OPEN_PHASE_LAST_BEG+4)    //C相最近断相结束时刻(4Bytes)

//功率因数区段累计时间,ly,2011-06-25,add,北京测试第一阶段后添加
#define  FACTOR_SEG_1                          (C_OPEN_PHASE_LAST_END+4)    //功率因数区段1累计时间(2Bytes)
#define  FACTOR_SEG_2                          (FACTOR_SEG_1+2)             //功率因数区段2累计时间(2Bytes)
#define  FACTOR_SEG_3                          (FACTOR_SEG_2+2)             //功率因数区段3累计时间(2Bytes)

#define  NEXT_NEW_INSTANCE                     (FACTOR_SEG_3+2)             //下次实时结算重新开始统计参变量标志(1Bytes)
//5-2本页结束于第246字节***************************

//5-3.总加组实时统计量页
#define  LEN_OF_ZJZ_BALANCE_RECORD             291   //总加组结算记录长度

//5-3-1.日冻结
#define  GP_DAY_OVER                           0                            //总加日统计结束标志(1Byte)
#define  GP_DAY_WORK                           (GP_DAY_OVER+1)              //总加有功功率类型(/总加电能量)((1+8)*7Bytes)
#define  GP_DAY_NO_WORK                        (GP_DAY_WORK+63)             //总加无功功率类型(/总加电能量)((1+8)*7Bytes)
#define  GP_WORK_POWER                         (GP_DAY_NO_WORK+63)          //总加有功功率(3Bytes)
#define  GP_NO_WORK_POWER                      (GP_WORK_POWER+3)            //总加无功功率(3Bytes)
#define  GP_DAY_MAX_POWER                      (GP_NO_WORK_POWER+3)         //日总加最大有功功率(3Bytes)
#define  GP_DAY_MAX_POWER_TIME                 (GP_DAY_MAX_POWER+3)         //日总加最大有功功率发生时间(3Bytes)
#define  GP_DAY_MIN_POWER                      (GP_DAY_MAX_POWER_TIME+3)    //日总加最小有功功率(3Bytes)
#define  GP_DAY_MIN_POWER_TIME                 (GP_DAY_MIN_POWER+3)         //日总加最小有功功率发生时间(3Bytes)
#define  GP_DAY_ZERO_POWER_TIME                (GP_DAY_MIN_POWER_TIME+3)    //日总加有功功率为零时间(2Bytes)

//5-3-2.月冻结
#define  GP_MONTH_OVER                         (GP_DAY_ZERO_POWER_TIME+2)   //总加月统计结束标志(1Bytes)
#define  GP_MONTH_WORK                         (GP_MONTH_OVER+1)            //月总加有功电能量((1+8)*7Bytes)
#define  GP_MONTH_NO_WORK                      (GP_MONTH_WORK+63)           //月总加无功电能量((1+8)*7Bytes)
#define  GP_MONTH_MAX_POWER                    (GP_MONTH_NO_WORK+63)        //月总加最大有功功率(3Bytes)
#define  GP_MONTH_MAX_POWER_TIME               (GP_MONTH_MAX_POWER+3)       //月总加最大有功功率发生时间(3Bytes)
#define  GP_MONTH_MIN_POWER                    (GP_MONTH_MAX_POWER_TIME+3)  //月总加最小有功功率(3Bytes)
#define  GP_MONTH_MIN_POWER_TIME               (GP_MONTH_MIN_POWER+3)       //月总加最小有功功率发生时间(3Bytes)
#define  GP_MONTH_ZERO_POWER_TIME              (GP_MONTH_MIN_POWER_TIME+3)  //月总加有功功率为零时间(2Bytes)
//5-3本页结束于第291字节***************************

//6.有计量功能的单灯控制器/线路控制器/报警控制器冻结数据长度
#define  LEN_OF_LIGHTING_FREEZE                30                           //照明系统控制器冻结数据记录长度


//7.其他统计量

//结构 - 终端统计记录
typedef struct
{
   INT16U powerOnMinute;        //供电时间
   INT16U resetTimes;           //复位次数
   INT16U closeLinkTimes;       //主动关闭链路次数
   INT16U linkResetTimes;       //非主动关闭链路次数
   INT16U sysTimes;             //发起连接次数
   INT16U sysSuccessTimes;      //连接成功次数
   INT16U heartTimes;           //心跳连接次数
   INT16U heartSuccessTimes;    //心跳连接成功次数
   INT16U receiveFrames;        //接收帧数
   INT32U receiveBytes;         //接收字节
   INT16U sendFrames;           //发送帧数
   INT32U sendBytes;            //发送字节
   INT8U  minSignal;            //信号最小值
   INT8U  minSignalTime[3];     //信号最小值发生时间
   INT8U  maxSignal;            //信号最大值
   INT8U  maxSignalTime[3];     //信号最大值发生时间
   INT8U  totalCopyTimes;       //总抄表次数
   INT8U  totalLocalCopyTimes;  //本地通信端口(载波/无线)抄表总次数
   INT8U  overFlow;             //通信流量超门限?
   
  #ifdef LOAD_CTRL_MODULE 
   INT16U monthCtrlJumpedDay;   //月电控跳闸日累计次数
   INT16U chargeCtrlJumpedDay;  //购电控跳闸日累计次数
   INT16U powerCtrlJumpedDay;   //功控跳闸日累计次数
   INT16U remoteCtrlJumpedDay;  //遥控跳闸日累计次数
  #endif
  
  #ifdef PLUG_IN_CARRIER_MODULE
   INT16U dcOverUp;             //直流模拟量越上限累计时间
   INT16U dcOverDown;           //直流模拟量越下限累计时间
   INT8U  dcMax[2];             //直流模拟量最大值
   INT8U  dcMaxTime[3];         //直流模拟量最大值发生时间
   INT8U  dcMin[2];             //直流模拟量最小值
   INT8U  dcMinTime[3];         //直流模拟量最小值发生时间
  #endif
  
  #ifdef LIGHTING
   INT8U  mixed;                //杂项
	                              //  bit0-集中器485口通信故障
  #endif
    
}TERMINAL_STATIS_RECORD;

//结构 - 电表统计记录(与时间无关量)
typedef struct
{ 
	INT8U      emptyByte1;        //ly,2010-07-21,在用科陆测试台测试时发现,本结构的第一字节老是要被改成0,
	                              //   所以空闲一个字节

	INT8U      meterStop[10];     //电表停走事件(第0字节-停走是否发生,1-4bytes停走时的正向有功总电能示值,5-9bytes停走发生的时间)

	INT8U      flyFlag;           //飞走标志
	INT8U      flyVision[5];      //飞走前正向有功总电能示值
	INT8U      reverseFlag;       //示度下降标志
	INT8U      reverseVision[5];  //下降前正向有功总电能示值

	INT8U      vUnBalance;        //电压不平衡状态
	DATE_TIME  vUnBalanceTime;    //电压不平衡持续时间

	INT8U      cUnBalance;        //电流不平衡状态
	DATE_TIME  cUnBalanceTime;    //电流不平衡持续时间
	
	INT8U      vOverLimit;        //电压越限
	DATE_TIME  vUpUpTime[3];      //电压越上上限持续时间
	DATE_TIME  vDownDownTime[3];  //电压越下下限持续时间
	
	INT8U      cOverLimit;        //电流越限
	DATE_TIME  cUpTime[3];        //电压越上限持续时间
	DATE_TIME  cUpUpTime[3];      //电压越上上限持续时间
	
  INT8U      phaseBreak;        //断相事件
  INT8U      loseVoltage;       //失压事件
  
  INT8U      apparentPower;     //视在功率越限状态
  
	DATE_TIME  apparentUpTime;    //视在功率越上限持续时间
	DATE_TIME  apparentUpUpTime;  //视在功率越上上限持续时间
	
  
	INT8U      mixed;             //杂项
	                              //  bit0-电能表时间超差标志
	                              //  bit1-485抄表失败标志
	                              //  bit2-相序异常标志
	                              //  bit3-电池欠压,2013-11-21,add
	
 	INT8U      currentLoop;       //电流回路异常标志(bit0-表示A相回路反向标志,bit1-表示B相回路反向标志,bit2-表示C相回路反向标志,[1表示发生反向,0表示未发生反向])

	INT8U      overFlag;          //电能量超差标志,2013-11-21,add
	INT8U      overVision[5];     //电能量超差前正向有功总电能示值,2013-11-21,add
	
}METER_STATIS_EXTRAN_TIME;

#ifdef LIGHTING

//结构 - 控制器统计记录(与时间无关量)
typedef struct
{
	INT8U      vOverLimit;        //电压越限
	DATE_TIME  vUpUpTime[3];      //电压越上上限持续时间
	DATE_TIME  vDownDownTime[3];  //电压越下下限持续时间
	
	INT8U      cOverLimit;        //电流越限
	DATE_TIME  cUpUpTime[3];      //电压越上上限持续时间
	DATE_TIME  cDownDownTime[3];  //电压越下下限持续时间
  
  INT8U      powerLimit;        //功率越限状态
	DATE_TIME  powerUpTime;       //功率越上限持续时间
	DATE_TIME  powerDownTime;     //功率越下限持续时间
	
  INT8U      factorLimit;       //功率因数越限状态
	DATE_TIME  factorDownTime;    //功率因数越下限持续时间

}KZQ_STATIS_EXTRAN_TIME;

#endif

//结构 - 电表统计记录(与时间有关量)
typedef struct
{
  INT16U copySuccessTimes;      //抄表成功
	INT8U  lastCopySuccessTime[2];//最后一次抄表成功时间(分时)
	
	INT8U  mixed;                 //杂项(bit0-电能表时间超差当天是否记录标志)
}METER_STATIS_BEARON_TIME;

//结构 - 单相电表统计记录(与时间无关量)
typedef struct
{
	INT8U     emptyByte1;         //ly,2010-07-21,在用科陆测试台测试时发现,本结构的第一字节老是要被改成0,
	                              //   所以空闲一个字节
  
	INT8U     mixed;              //杂项
	                              //    bit0-电能表时间超差标志
 
 #ifdef LIGHTING
	                              //mixed 
	                              //    bit1-报警控制点线路异常标志(直流供电)
	                              //    bit2-报警控制点线路异常标志(交流供电)
	                              //    bit3-单灯控制点离线标志
																//    bit4-单灯控制点漏电流超限,2016-10-12,Add
																//    bit5-控制时段是否同步,2016-10-14,Add

	DATE_TIME lastFailure;        //上一次发生灯故障时间
	
	DATE_TIME lastDip;            //上一次发生倾斜时间,2016-10-11,Add
	
 #endif
	
}METER_STATIS_EXTRAN_TIME_S;

//结构 - 三相表示值/需量冻结文件
typedef struct
{
	INT16U pn;
	INT8U  type;    //0x0b-日冻结示值,0x0c-日冻结需量
	INT32U copyTime;
	INT8U  data[288];
}MP_F_DAY;

//结构 - 单相表冻结文件
typedef struct
{
	INT16U pn;
	INT32U copyTime;
	INT8U  data[40];
}SP_F_DAY;

#define  VOLTAGE_UNBALANCE                   0x03
#define  VOLTAGE_UNBALANCE_NOT               0x0C
#define  CURRENT_UNBALANCE                   0x30
#define  CURRENT_UNBALANCE_NOT               0xC0

#endif   /*__INCworkWithMeterh*/
