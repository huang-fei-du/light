/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名:common.h
作者:leiyong
版本:0.9
完成日期:2009年10月
描述:电力终端(负控终端、集中器)主站设置参数头文件
修改历史:
  

**************************************************/
#ifndef __MsSetParaH
#define __MsSetParaH

#include "common.h"
#include "workWithMS.h"

extern ANALYSE_FRAME 					  frame;									 //用于保存接收分组中各字段值的全局变量

extern COMM_PARA                commPara;                //终端通信参数(AFN04-FN01)
extern RELAY_CONFIG             relayConfig;             //终端中继转发设置(AFN04-FN02)
extern IP_AND_PORT              ipAndPort;               //主站IP地址和端口(AFN04-FN03)
extern PHONE_AND_SMS            phoneAndSmsNumber;       //主站电话号码和短信中心号码(AFN04-FN04)
extern INT8U                    messageAuth[3];          //终端消息认证(AFN04-FN05)
extern INT8U                    groupAddr[32];           //终端组地址(AFN04-FN6)

extern TE_IP_AND_PORT           teIpAndPort;             //终端IP地址和端口(AFN04-FN7)
extern PRIVATE_NET_METHOD       tePrivateNetMethod;      //终端上行通信工作方式(AFN04-FN8)
extern EVENT_RECORD_CONFIG      eventRecordConfig;       //终端事件记录配置设置(AFN04-FN9)
extern INT16U									  meterDeviceNum;				   //电能表/交流采样装置配置数量
extern INT8U 									  statusInput[2];		       //终端状态量输入参数(AFN04-FN12)

extern PULSE_CONFIG             pulseConfig;             //终端脉冲配置参数(AFN04-FN11)
extern IU_SIMULATE_CONFIG       simuIUConfig;            //电压/电流模拟量配置参数(AFN04-FN13)
extern TOTAL_ADD_GROUP          totalAddGroup;           //终端总加组配置参数(AFN04-FN14)
extern ENERGY_DIFFERENCE_CONFIG differenceConfig;        //电能量差动越限配置(AFN04-FN15)
extern VPN                      vpn;                     //虚拟专网用户名、密码(AFN04-FN16)
extern INT8U                    protectLimit[2];         //终端保安定值(AFN04-FN17)
extern CONTRL_PARA              ctrlPara;                //终端功控时段，时段功控浮动系数，月电能量控定值浮动系数(AFN04-FN18,FN19,FN20)

extern INT8U                    periodTimeOfCharge[49];  //终端电能量费率和费率数(AFN04-021)

extern CHARGE_RATE_NUM          chargeRateNum;           //终端电能量费率(AFN04-022)

extern INT8U                    chargeAlarm[3];          //终端催费告警参数(AFN04-023)

extern TE_COPY_RUN_PARA         teCopyRunPara;           //终端抄表运行参数设置(AFN04-FN33)

extern DOWN_RIVER_MODULE_PARA   downRiverModulePara;		 //集中器下行通信模块的参数设置(AFN04-FN34)
extern KEY_HOUSEHOLD						keyHouseHold;						 //台区集中抄表重点户设置(AFN04-FN35)
extern INT8U       						  upTranslateLimit[4];		 //终端上行通信流量门限设置(AFN04-FN36)
extern CASCADE_COMM_PARA        cascadeCommPara;         //终端级联通信参数
extern TYPE_1_2_DATA_CONFIG     typeDataConfig1;         //终端级联通信参数(AFN04-FN38)
extern TYPE_1_2_DATA_CONFIG     typeDataConfig2;         //终端级联通信参数(AFN04-FN39)

//控制参数集
extern STAY_SUPPORT_STATUS      staySupportStatus;                //终端保电状态
extern INT8U                    reminderFee;                      //催费告警状态
extern INT8U                    toEliminate;                      //剔除状态                  
extern CTRL_RUN_STATUS          ctrlRunStatus[8];                 //终端投运状态
extern WKD_CTRL_CONFIG          wkdCtrlConfig[8];                 //厂休功控参数(AFN04-FN42)
extern POWERCTRL_COUNT_TIME     powerCtrlCountTime[8];            //功率控制的功率计算滑差时间(AFN04-FN43)
extern OBS_CTRL_CONFIG          obsCtrlConfig[8];                 //营业报停控参数(AFN04-FN44)
extern POWER_DOWN_CONFIG        powerDownCtrl[8];                 //当前功率下浮控参数 
extern POWERCTRL_ROUND_FLAG     powerCtrlRoundFlag[8];            //功控轮次设定(AFN04-FN45)
extern MONTH_CTRL_CONFIG        monthCtrlConfig[8];               //月电量控定值(AFN04-FN46)
extern CHARGE_CTRL_CONFIG       chargeCtrlConfig[8];              //购电量控参数(AFN04-FN47)
extern ELECTCTRL_ROUND_FLAG     electCtrlRoundFlag[8];            //电控轮次设定(AFN04-FN48)

extern POWERCTRL_ALARM_TIME     powerCtrlAlarmTime[8];            //功控告警时间(AFN04-FN49)

extern REMOTE_CTRL_CONFIG       remoteCtrlConfig[CONTROL_OUTPUT]; //遥控参数
extern REMOTE_EVENT_INFOR       remoteEventInfor[8];              //遥控数据冻结参数
extern PERIOD_CTRL_CONFIG       periodCtrlConfig[8];              //时段功控定值(AFN04-FN41),时段功控参数
extern CTRL_STATUS              ctrlStatus;                       //控制状态
extern POWER_CTRL_EVENT_INFOR   powerCtrlEventInfor;              //功控现场(事件)信息

extern INT8U                    voiceAlarm[3];                    //终端声音告警允许//禁止设置(AFN04-FN57)
extern INT8U                    noCommunicationTime;              //允许无通信时间(AFN04-058)
extern METER_GATE               meterGate;                        //电能表异常判别阈值设定(AFN04-FN59)
extern WAVE_LIMIT               waveLimit;                        //谐波限值(AFN04-FN60)
extern INT8U 									  adcInFlag;                        //直流模拟量输入参数(AFN04-FN61)

extern REPORT_TASK_PARA         reportTask1, reportTask2;         //定时发送1类，2类数据任务配置(AFN04-FN65,FN66)

extern CHN_MESSAGE              chnMessage;                       //中文信息

//规约扩展
extern INT8U                    teName[20];                       //终端名称(AFN04-FN97)

extern INT8U                    sysRunId[20];                     //系统运行标识码(AFN04-FN98)[重庆规约]
extern INT8U                    assignCopyTime[6];                //终端抄表搜索持续时间(AFN04-FN99)[重庆规约]
extern INT8U                    teApn[4][16];                     //终端预设apn(AFN04-FN100)[重庆规约]
extern INT8U                    rlPara[4];                        //[沃电扩展]锐拔模块参数,字节1-基本功率,字节2-最大功率,字节3-信号强度,字节4-信道
extern AC_SAMPLE_PARA           acSamplePara;                     //交采校正参数
extern INT8U                    mainNodeAddr[6];                  //本地通信模块(载波/无线)主节点地址
extern INT16U                   deviceNumber;                     //设备编号

extern INT8U                    paraStatus[31];                   //终端参数状态
extern INT8U                    eventStatus[8];                   //事件标志状态

extern char *paraName[132];  //参数名称
extern char *ctrlName[42];   //控制名称

#ifdef LIGHTING

 extern PN_GATE                 pnGate;                           //控制点阈值
 extern INT8U                   ctrlMode;                         //控制模式
 extern INT8U                   beforeOnOff[4];                   //光控提前-延迟分钟有效
 
 extern INT16U                  lcHeartBeat;                      //关联光控流明值心跳周期
 
 extern INT16U                  statusHeartBeat;                  //单灯、线路控制点状态更新心跳周期,2016-11-24,Add

#endif

#endif /*__MsSetParaH*/
