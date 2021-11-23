/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:msSetPara.c
作者:leiyong
版本:0.9
完成日期:2009年11月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)主站设置参数文件
函数列表:
  1.
修改历史:
  01,09-10-27,Leiyong created.

***************************************************/

#include "common.h"
#include "workWithMS.h"
//#include "meterProtocol.h"

ANALYSE_FRAME						 frame;										//用于保存接收分组中各字段值的全局变量

COMM_PARA                commPara;                //终端通信参数(AFN04-FN01)
RELAY_CONFIG             relayConfig;             //终端中继转发设置(AFN04-FN02)
IP_AND_PORT              ipAndPort;               //主站IP地址和端口(AFN04-FN03)
PHONE_AND_SMS            phoneAndSmsNumber;       //主站电话号码和短信中心号码(AFN04-FN04)
INT8U                    messageAuth[3];          //终端消息认证(AFN04-FN05)
INT8U                    groupAddr[16];           //终端组地址(AFN04-FN6)

TE_IP_AND_PORT          teIpAndPort;             //终端IP地址和端口(AFN04-FN7)

PRIVATE_NET_METHOD      tePrivateNetMethod;      //终端上行通信工作方式(AFN04-FN8)
EVENT_RECORD_CONFIG     eventRecordConfig;       //终端事件记录配置设置(AFN04-FN9)
INT16U									 meterDeviceNum;					//电能表/交流采样装置配置数量(AFN04-FN10之个数)
INT8U 									 statusInput[2];		      //终端状态量输入参数(AFN04-FN12)

PULSE_CONFIG             pulseConfig;             //终端脉冲配置参数(AFN04-FN11)
IU_SIMULATE_CONFIG       simuIUConfig;            //电压/电流模拟量配置参数(AFN04-FN13)
TOTAL_ADD_GROUP          totalAddGroup;           //终端总加组配置参数(AFN04-FN14)
ENERGY_DIFFERENCE_CONFIG differenceConfig;        //电能量差动越限配置(AFN04-FN15)
VPN                      vpn;                     //虚拟专网用户名、密码(AFN04-FN16)
INT8U                    protectLimit[2];         //终端保安定值(AFN04-FN17)

CONTRL_PARA              ctrlPara;                //终端功控时段，时段功控浮动系数，月电能量控定值浮动系数(AFN04-FN18,FN19,FN20)

INT8U                   periodTimeOfCharge[49];  //终端电能量费率和费率数(AFN04-021)
CHARGE_RATE_NUM         chargeRateNum;           //终端电能量费率(AFN04-022)

TE_COPY_RUN_PARA         teCopyRunPara;          //终端抄表运行参数设置(AFN04-FN33)

DOWN_RIVER_MODULE_PARA   downRiverModulePara;		//集中器下行通信模块的参数设置(AFN04-FN34)
KEY_HOUSEHOLD						keyHouseHold;						//台区集中抄表重点户设置(AFN04-FN35)
INT8U       						  upTranslateLimit[4];			//终端上行通信流量门限设置(AFN04-FN36)
CASCADE_COMM_PARA        cascadeCommPara;         //终端级联通信参数(AFN04-FN37)
TYPE_1_2_DATA_CONFIG     typeDataConfig1;          //终端级联通信参数(AFN04-FN38) 
TYPE_1_2_DATA_CONFIG     typeDataConfig2;          //终端级联通信参数(AFN04-FN39)

INT8U                    chargeAlarm[3];          //终端催费告警参数(AFN04-023)

//控制参数集
STAY_SUPPORT_STATUS      staySupportStatus;                //终端保电状态
INT8U                    reminderFee;                      //催费告警状态
INT8U                    toEliminate;                      //剔除状态                  
CTRL_RUN_STATUS          ctrlRunStatus[8];                 //终端投运状态
WKD_CTRL_CONFIG          wkdCtrlConfig[8];                 //厂休功控参数(AFN04-FN42)
POWERCTRL_COUNT_TIME     powerCtrlCountTime[8];            //功率控制的功率计算滑差时间(AFN04-FN43)
OBS_CTRL_CONFIG          obsCtrlConfig[8];                 //营业报停控参数(AFN04-FN44)
POWER_DOWN_CONFIG        powerDownCtrl[8];                 //当前功率下浮控参数 
POWERCTRL_ROUND_FLAG     powerCtrlRoundFlag[8];            //功控轮次设定(AFN04-FN45)
MONTH_CTRL_CONFIG        monthCtrlConfig[8];               //月电量控定值(AFN04-FN46)
CHARGE_CTRL_CONFIG       chargeCtrlConfig[8];              //购电量控参数(AFN04-FN47)
ELECTCTRL_ROUND_FLAG     electCtrlRoundFlag[8];            //电控轮次设定(AFN04-FN48)

POWERCTRL_ALARM_TIME     powerCtrlAlarmTime[8];            //功控告警时间(AFN04-FN49)

REMOTE_CTRL_CONFIG       remoteCtrlConfig[CONTROL_OUTPUT]; //遥控参数
REMOTE_EVENT_INFOR       remoteEventInfor[8];              //遥控数据冻结参数
PERIOD_CTRL_CONFIG       periodCtrlConfig[8];              //时段功控定值(AFN04-FN41),时段功控参数
CTRL_STATUS              ctrlStatus;                       //控制状态
POWER_CTRL_EVENT_INFOR   powerCtrlEventInfor;              //功控现场(事件)信息

INT8U                    voiceAlarm[3];                    //终端声音告警允许//禁止设置(AFN04-FN57)
INT8U                    noCommunicationTime;              //允许无通信时间(AFN04-058)
METER_GATE               meterGate;                        //电能表异常判别阈值设定(AFN04-FN59)
WAVE_LIMIT               waveLimit;                        //谐波限值(AFN04-FN60)
INT8U 									 adcInFlag;                        //直流模拟量输入参数(AFN04-FN61)

//VNET_WORK_METHOD         vnetMethod;                       //虚拟专用网络工作方式(AFN04-FN62)

REPORT_TASK_PARA         reportTask1, reportTask2;         //定时发送1类，2类数据任务配置(AFN04-FN65,FN66)

CHN_MESSAGE              chnMessage;                       //中文信息

//规约扩展
INT8U                    teName[20];                       //终端名称(AFN04-FN97)

INT8U                    sysRunId[21];                     //系统运行标识码(AFN04-FN98)[重庆规约]
INT8U                    assignCopyTime[6];                //终端抄表搜索持续时间(AFN04-FN99)[重庆规约]
INT8U                    teApn[4][16];                     //终端预设apn(AFN04-FN100)[重庆规约]
 
 INT8U                   rlPara[4];                        //[沃电扩展]锐拔模块参数,字节1-基本功率,字节2-最大功率,字节3-信号强度,字节4-信道

AC_SAMPLE_PARA           acSamplePara;                     //交采校正参数
INT8U                    mainNodeAddr[6];                  //本地通信模块(载波/无线)主节点地址
INT16U                   deviceNumber;                     //设备编号


INT8U                    paraStatus[31];                   //终端参数状态
INT8U                    eventStatus[8];                   //事件标志状态

#ifdef LIGHTING
 PN_GATE                 pnGate;                           //控制点阈值
 
 INT8U                   ctrlMode;                         //控制模式
                                                           //  1-时段控
                                                           //  2-光控
                                                           //  3-光控+时段控
                                                           //  4-经纬度控

 INT8U                   beforeOnOff[4];                   //光控提前-延迟分钟有效
 
 INT16U                  lcHeartBeat=0;                    //关联光控流明值心跳周期,2016-11-21,Add
 
 INT16U                  statusHeartBeat=0;                //单灯、线路控制点状态更新心跳周期,2016-11-24,Add
#endif


//参数名称
char *paraName[132] = 
  { 
	  "终端通信参数设置",                     //F1
	  "终端中继转发设置",                     //F2
	  "主站IP地址和端口",                     //F3
	  "主站电话号码和短信中心号码",           //F4
	  "终端消息认证参数设置",                 //F5
	  "终端组地址设置",                       //F6
	  "终端抄表日设置",                       //F7
	  "终端事件记录配置设置",                 //F8
	  "终端配置数量表",                       //F9
	  "终端电能表／交流采样装置配置参数",     //F10
	  "终端脉冲配置参数",                     //F11
	  "终端状态量输入参数",                   //F12
	  "终端电压／电流模拟量配置参数",         //F13
	  "终端总加组配置参数",                   //F14
	  "有功总电能量差动越限事件参数设置",     //F15
	  "虚拟专网用户名、密码",                 //F16
	  "终端保安定值",                         //F17
	  "终端功控时段",                         //F18
	  "终端时段功控定值浮动系数",             //F19
	  "终端月电能量控定值浮动系数",           //F20
	  "终端电能量费率时段和费率数",           //F21
	  "终端电能费率",                         //F22
	  "终端催费告警参数",                     //F23
	  "终端抄表间隔设置",                     //F24
	  "测量点基本参数",                       //F25
	  "测量点限值参数",                       //F26
	  "测量点数据冻结参数",                   //F27
	  "测量点功率因数分段限值",               //F28
	  "",                                     //F29
	  "",                                     //F30
	  "",                                     //F31
	  "",                                     //F32
	  "总加组数据冻结参数",                   //F33
	  "",                                     //F34
	  "",                                     //F35
	  "",                                     //F36
	  "",                                     //F37
	  "",                                     //F38
	  "",                                     //F39
	  "",                                     //F40
	  "时段功控定值",                         //F41
	  "厂休功控参数",                         //F42
	  "功率控制的功率计算滑差时间",           //F43
	  "营业报停控参数",                       //F44
	  "功控制轮次设定",                       //F45
	  "月电量控定值",                         //F46
	  "购电量(费)控参数",                     //F47
	  "电控轮次设定",                         //F48
	  "功控告警时间",                         //F49
	  "",                                     //F50
	  "",                                     //F51
	  "",                                     //F52
	  "",                                     //F53
	  "",                                     //F54
	  "",                                     //F55
	  "",                                     //F56
	  "终端声音告警允许／禁止设置",           //F57
	  "终端自动保电参数",                     //F58
	  "电能表异常判别阈值设定",               //F59
	  "谐波限值",                             //F60
	  "直流模拟量接入参数",                   //F61
	  "虚拟专网工作方式",                     //F62
	  "",                                     //F63
	  "",                                     //F64
	  "定时发送一类数据任务设置",             //F65
	  "定时发送二类数据任务设置",             //F66
	  "定时发送一类数据任务启动／停止设置",   //F67
	  "定时发送二类数据任务启动／停止设置",   //F68
	  "",                                     //F69
	  "",                                     //F70
	  "",                                     //F71
	  "",                                     //F72
	  "电容器参数",                           //F73
	  "电容器投切运行参数",                   //F74
	  "电容器保护参数",                       //F75
	  "电容器投切控制方式",                   //F76
	  "",                                     //F77
	  "",                                     //F78
	  "",                                     //F79
	  "",                                     //F80
	  "直流模拟量变比",                       //F81
	  "直流模拟量限值",                       //F82
	  "直流模拟量冻结参数",                   //F83
	  "",                                     //F84
	  "",                                     //F85
	  "",                                     //F86
	  "",                                     //F87
	  "",                                     //F88
	  "",                                     //F89
	  "",                                     //F90
	  "",                                     //F91
	  "电表局编号",                           //F92
	  "",                                     //F93
	  "",                                     //F94
	  "",                                     //F95
	  "",                                     //F96
	  "终端名称",                             //F97
	  "终端ID号",                             //F98
	  "",                                     //F99
	  "",                                     //F100
	  "",                                     //F101
	  "",                                     //F102
	  "",                                     //F103
	  "",                                     //F104
	  "",                                     //F105
	  "",                                     //F106
	  "",                                     //F107
	  "",                                     //F108
	  "",                                     //F109
	  "",                                     //F110
	  "",                                     //F111
	  "",                                     //F112
	  "",                                     //F113
	  "",                                     //F114
	  "",                                     //F115
	  "",                                     //F116
	  "",                                     //F117
	  "",                                     //F118
	  "",                                     //F119
	  "",                                     //F120
	  "终端地址及行政区划码",                 //F121
	  "指定终端抄表时间",                     //F122
	  "自定义标准DL/T 645抄表项",             //F123
	  "",                                     //F124
	  "",                                     //F125
	  "",                                     //F126
	  "",                                     //F127
	  "",                                     //F128
	  "终端交流采样校正值",                   //F129
	  "终端以太网地址",                       //F130
	  "读以交流采样校正前数据值",             //F131
	  "虚拟专网用户名、密码[扩展]"            //F132
	};


char *ctrlName[42] =
  { 
	  "遥控跳闸",                     //F1
	  "允许合闸",                     //F2
	  "",                             //F3
	  "",                             //F4
	  "",                             //F5
	  "",                             //F6
	  "",                             //F7
	  "",                             //F8
	  "时段功控投入",                 //F9
	  "厂休功控投入",                 //F10
	  "营业报停功控投入",             //F11
	  "当前功率下浮控投入",           //F12
	  "",                             //F13
	  "",                             //F14
	  "月电控投入",                   //F15
	  "购电控投入",                   //F16
	  "时段功控解除",                 //F17
	  "厂休控解除",                   //F18
	  "营业报停控解除",               //F19
	  "当前功率下浮控解除",           //F20
	  "",                             //F21
	  "",                             //F22
	  "月电控解除",                   //F23
	  "购电控解除",                   //F24
	  "终端保电投入",                 //F25
	  "催费告警投入",                 //F26
	  "允许终端与主站通话",           //F27
	  "终端剔除投入",                 //F28
	  "允许终端主动上报",             //F29
	  "",                             //F30
	  "对时命令",                     //F31
	  "中文信息",                     //F32
	  "终端保电解除",                 //F33
	  "催费告警解除",                 //F34
	  "禁止终端与主站通话",           //F35
	  "终端剔除解除",                 //F36
	  "禁止终端主动上报",             //F37
	  "激活连接",                     //F38
	  "",                             //F39
	  "",                             //F40
	  "电容器控制投入",               //F41
	  "电容器控制切除"                //F42
	};

