/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：meterProtocol.h
作者:leiyong
版本：0.9
完成日期:2009年1月10日
描述：电能表协议私有头文件
修改历史：
  01,09-01-07,Leiyong created.
**************************************************/

#ifndef __meterProtocolxH
#define __meterProtocolxH

#include "common.h"
#include "workWithMeter.h"
#include "meterProtocol.h"

//#define  SHOW_DEBUG_INFO                       //显示调试信息

#define JF_MONITOR                               //机房监控

//单相智能电能表DL/T645-1997规约
#define PROTOCOL_645_1997                       DLT_645_1997
#ifdef PROTOCOL_645_1997 
  #define TOTAL_CMD_CURRENT_645_1997             79  //DL/T645-1997当前数据命令条数
  #define TOTAL_CMD_LASTMONTH_645_1997           16  //DL/T645-1997上月数据命令条数
#endif

//三相智能电能表DL/T645-2007规约
#define PROTOCOL_645_2007                       DLT_645_2007
#ifdef PROTOCOL_645_2007 
  
  #ifdef DKY_SUBMISSION
   #define TOTAL_CMD_CURRENT_645_2007            62  //DL/T645-2007当前数据命令条数
  #elif defined JF_MONITOR
	 #define TOTAL_CMD_CURRENT_645_2007						 22	 //DL/T645-2007当前数据命令条数	 
  #else
   #define TOTAL_CMD_CURRENT_645_2007           100  //DL/T645-2007当前数据命令条数
  #endif
  
  #define TOTAL_CMD_LASTMONTH_645_2007           28  //DL/T645-2007 上一结算日数据命令条数
  #define TOTAL_CMD_LASTDAY_645_2007             11  //DL/T645-2007 上一次日冻结数据命令条数
  #define TOTAL_CMD_LASTDAY_SINGLE_07             3  //单相表07     上一次日冻结数据命令条数
#endif

//单相表DL/T645-1997
#define PROTOCOL_SINGLE_PHASE_97                SINGLE_PHASE_645_1997
#ifdef  PROTOCOL_SINGLE_PHASE_97
  #define TOTAL_CMD_SINGLE_645_97                 1  //单相97表中实时数据命令条数
  #define TOTAL_CMD_LASTDAY_SINGLE_97             1  //单相97表中上一次日冻结数据命令条数
#endif

//DL/T645-1997三相表只抄正反有无功电能示值
#define PROTOCOL_PN_WORK_NOWORK_97              PN_WORK_NOWORK_1997
#ifdef  PROTOCOL_PN_WORK_NOWORK_97
  #define TOTAL_CMD_PN_WORK_NOWORK_645_97         6  //实时数据命令条数
  #define TOTAL_CMD_PN_WORK_NOWORK_97             0  //上一次日冻结数据命令条数
#endif

//DL/T645-2007三相表只抄正反有无功电能示值
#define PROTOCOL_PN_WORK_NOWORK_07              PN_WORK_NOWORK_2007
#ifdef  PROTOCOL_PN_WORK_NOWORK_07
  #define TOTAL_CMD_PN_WORK_NOWORK_645_07         6  //实时数据命令条数
  #define TOTAL_CMD_PN_WORK_NOWORK_07             0  //上一次日冻结数据命令条数
#endif


//单相智能电能表DL/T645-2007
#define PROTOCOL_SINGLE_PHASE_07                SINGLE_PHASE_645_2007
#ifdef  PROTOCOL_SINGLE_PHASE_07
  #define TOTAL_CMD_SINGLE_645_07                 7  //单相智能电表07中实时数据命令条数
#endif

//单相本地费控智能电能表DL/T645-2007
#define PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007  SINGLE_LOCAL_CHARGE_CTRL_2007
#ifdef  PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
  #define TOTAL_CMD_SINGLE_LOCAL_CTRL_07         17  //单相本地费控智能电表中实时数据命令条数
#endif

//单相远程费控智能电能表DL/T645-2007
#define PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007 SINGLE_REMOTE_CHARGE_CTRL_2007
#ifdef  PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
  #define TOTAL_CMD_SINGLE_REMOTE_CTRL_07         9  //单相远程费控智能电表中实时数据命令条数
#endif

//三相智能电能表DL/T645-2007
#define PROTOCOL_THREE_2007   THREE_2007
#ifdef  PROTOCOL_THREE_2007
  #define TOTAL_CMD_THREE__07                    17  //三相智能电表中实时数据命令条数
#endif

//三相本地费控智能电能表DL/T645-2007
#define PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007   THREE_LOCAL_CHARGE_CTRL_2007
#ifdef  PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
  #define TOTAL_CMD_THREE_LOCAL_CTRL_07          27  //三相本地费控智能电表中实时数据命令条数
#endif

//三相远程费控智能电能表DL/T645-2007
#define PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007  THREE_REMOTE_CHARGE_CTRL_2007
#ifdef  PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
  #define TOTAL_CMD_THREE_REMOTE_CTRL_07         19  //三相远程费控智能电表中实时数据命令条数
#endif

#define PROTOCOL_KEY_HOUSEHOLD_2007             KEY_HOUSEHOLD_2007
#ifdef  PROTOCOL_KEY_HOUSEHOLD_2007
  #define TOTAL_CMD_KEY_2007                      8  //07规约的重点用户实时数据命令条数
  #define TOTAL_CMD_LASTDAY_KEY_2007              3  //DL/T645-2007重点用户上一次日冻结数据命令条数
#endif

//#define LANDIS_GRY_ZD_PROTOCOL SIMENS_ZD_METER   //兰吉尔(/西门子)ZD表协议
#ifdef  LANDIS_GRY_ZD_PROTOCOL
  #define TOTAL_COMMAND_REAL_ZD                   2  //兰吉尔(/西门子)ZD表协议中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_ZD              0  //兰吉尔(/西门子)ZD表协议中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_ZD                   0  //兰吉尔(/西门子)ZD表协议中关于参数数据的命令条数
  #define TOTAL_DATA_ITEM_ZD                     79  //兰吉尔(/西门子)ZD表协议数据消息数据项数
#endif

//#define LANDIS_GRY_ZB_PROTOCOL SIMENS_ZB_METER   //兰吉尔(/西门子)ZB表协议
#ifdef  LANDIS_GRY_ZB_PROTOCOL
  #define TOTAL_COMMAND_REAL_ZB                   2  //兰吉尔(/西门子)ZB表协议中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_ZB              0  //兰吉尔(/西门子)ZB表协议中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_ZB                   0  //兰吉尔(/西门子)ZB表协议中关于参数数据的命令条数
#endif

//#define PROTOCOL_ABB_GROUP  ABB_METER        //ABB表协议
#ifdef  PROTOCOL_ABB_GROUP
  #define TOTAL_COMMAND_REAL_ABB                 18  //ABB表协议中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_ABB             0  //ABB表协议中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_ABB                  0  //ABB表协议中关于参数数据的命令条数
#endif

//#define PROTOCOL_EDMI_GROUP EDMI_METER       //EDMI表协议
#ifdef  PROTOCOL_EDMI_GROUP
  #define TOTAL_COMMAND_REAL_EDMI                59  //EDMI表协议中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_EDMI           43  //EDMI表协议中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_EDMI                 0  //EDMI表协议中关于参数数据的命令条数
#endif

//#define PROTOCOL_WASION_GROUP WASION_METER   //威盛规约协议簇
#ifdef  PROTOCOL_WASION_GROUP
  #define TOTAL_COMMAND_REAL_WASION              10  //威盛规约协议簇中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_WASION         10  //威盛规约协议簇中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_WASION              18  //威盛规约协议簇中关于参数数据的命令条数
  #define TOTAL_COMMAND_EXPAND_WASION             1  //645规约协议簇中扩展部分的命令条数
#endif

//#define PROTOCOL_HONGHUA_GROUP HONGHUA_METER //弘华规约协议簇
#ifdef  PROTOCOL_HONGHUA_GROUP
  #define TOTAL_COMMAND_REAL_HONGHUA           //弘华规约协议簇中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_HONGHUA      //弘华规约协议簇中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_HONGHUA           //弘华规约协议簇中关于参数数据的命令条数
#endif

//#define LOCALE_SUPERVISAL_DEVICE SUPERVISAL_DEVICE //现场监测设备规约
#ifdef  LOCALE_SUPERVISAL_DEVICE
  #define TOTAL_COMMAND_REAL_LOCALE            //现场监测设备规约中关于实时数据的命令条数
  #define TOTAL_COMMAND_LASTMONTH_LOCALE       //现场监测设备规约中关于上月数据的命令条数
  #define TOTAL_COMMAND_PARA_LOCALE            //现场监测设备规约中关于参数数据的命令条数
#endif

#define PROTOCOL_MODUBUS_GROUP     MODBUS_HY    //MODBUS规约协议簇


//2.抄回的数据标志
#define HAS_CURRENT_ENERGY                       0x1  //有当前电量或参变量数据
#define HAS_CURRENT_REQ                          0x2  //有当前需量数据
#define HAS_PARA_VARIABLE                        0x4  //有参量及参变量数据
#define HAS_SHIDUAN                              0x8  //有时段数据
#define HAS_LAST_MONTH_ENERGY                   0x10  //有上月电量数据
#define HAS_LAST_MONTH_REQ                      0x20  //有上月需量数据
#define HAS_LAST_DAY_ENERGY                     0x40  //有上一次日冻结电能数据
#define HAS_LAST_DAY_REQ                        0x80  //有上一次日冻结需量数据
#define HAS_HOUR_FREEZE_ENERGY                 0x100  //有整点冻结电能数据

//3.控制字段标志位(抄表发命令时用)--------------------------------------------------------
//3.1 DL/T645-1997
#ifdef PROTOCOL_645_1997 
  #define READ_DATA_645_1997                      0x1   //读数据
  #define READ_D_FOLLOW_645_1997                 0x81   //读数据应答－无后续数据帧
  #define READ_D_LAST_645_1997                   0xA1   //读数据应答－有后续数据帧
  #define READ_D_ERROR_645_1997                  0xC1   //读数据应答－异常应答
  #define READ_FOLLOWING_DATA_645_1997            0x2   //读后继数据
  #define READ_F_FOLLOW_645_1997                 0x82   //读后续数据应答－无后续数据帧
  #define READ_F_LAST_645_1997                   0xA2   //读后续数据应答－有后续数据帧
  #define READ_F_ERROR_645_1997                  0xC2   //读后续数据应答－异常应答
  #define REREAD_DATA_645_1997                    0x3   //重读数据
  #define REREAD_FOLLOW_645_1997                 0x83   //重读数据应答－无后续数据帧
  #define REREAD_LAST_645_1997                   0xA3   //重读数据应答－有后续数据帧
  #define REREAD_ERROR_645_1997                  0xC3   //重读数据应答－异常应答
  #define WRITE_DATA_645_1997                     0x4   //写数据
  #define WRITE_OK_645_1997                      0x84   //写数据应答－正常应答
  #define WRITE_ERROR_645_1997                   0xC4   //写数据应答－异常应答  
  #define TIME_ADJUSTING_645_1997                 0x8   //广播校时
  #define WRITE_ADDR_645_1997                     0xA   //写设备地址
  #define WRITE_ADDR_OK_645_1997                 0x8A   //写设备地址应答－正常应答  
  #define SWITCH_SPEED_645_1997                   0xC   //更改通信速率
  #define SWITCH_SPEED_REP_645_1997              0x8C   //更改通信速率应答  
  #define CHANGE_PASSWORD_645_1997                0xF   //更改密码
  #define CHANGE_PASSWORD_REP_645_1997           0x8F   //更改密码应答  
  #define CLEAR_645_1997                         0x10   //最大需量清零
  #define LAST_FRAME_645_1997                    0x80   //无后续帧
  #define FOLLOW_FRAME_645_1997                  0xA0   //有后续帧
  #define DATA_ERROR_645_1997                     0x2   //数据错误
#endif   //PROTOCOL_645_1997

//3.2 DL/T645-2007
#ifdef PROTOCOL_645_2007
  #define READ_DATA_645_2007                     0x11   //读数据
  #define READ_D_FOLLOW_645_2007                 0x91   //读数据应答－无后续数据帧
  #define READ_D_LAST_645_2007                   0xB1   //读数据应答－有后续数据帧
  
  #define NORMAL_REPLY                           0x90   //10010000(D7=1,从站发出的应答帧 D6=0,从站正常应答 D4=1 2007固定置1
  #define ABERRANT_REPLY                         0xD0   //11010000(D7=1,从站发出的应答帧 D6=1,从站异常应答 D4=1 2007固定置1
                                                        
  #define ENERGY_2007                            0x00   //DI3为0的是电能量
  #define REQ_AND_REQ_TIME_2007                  0x01   //DI3为1的是需量及发时间
  #define VARIABLE_2007                          0x02   //变量
  #define EVENT_RECORD_2007                      0x03   //事件记录
  #define PARA_VARIABLE_2007                     0x04   //参变量
  #define FREEZE_DATA_2007                       0x05   //冻结数据
  #define EXT_EVENT_RECORD_13                    0x13   //备案文件(断相统计数据)
  
#endif   //PROTOCOL_645_2007

#ifdef PROTOCOL_EDMI_GROUP
  #define EDMI_STX                                  2   //帧头
  #define EDMI_ETX                                  3   //帧结束字符
  #define EDMI_ACK                                  6   //确认
  #define EDMI_DLE                                 16   //DLE
  #define EDMI_CAN                                 24   //CRC校验正确,但指令有误
  #define EDMI_XON                                 17
  #define EDMI_XOFF                                19
#endif

#endif /*__meterProtocolxH*/
