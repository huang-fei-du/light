/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名:teRunPara.h
作者:leiyong
版本:0.9
完成日期:2009年10月
描述:电力终端(负控终端、集中器)主站设置参数头文件
修改历史:
  01,09-10-28,leiyong created.

**************************************************/

#ifndef __teRunParaH
#define __teRunParaH

#include "common.h"
#include "timeUser.h"
#include "workWithMS.h"

//结构 - 停/上电变量
typedef struct
{
  DATE_TIME           powerOnOffTime;                 //系统上电/停电时间
  INT8U               powerOnOrOff;                   //上电还是停电
}POWER_ON_OFF;

extern DATE_TIME      sysTime;                        //系统时间
extern DATE_TIME      powerOnStatisTime;              //上电统计时间
extern BOOL           ifPowerOff;                     //是否停电
extern POWER_ON_OFF   powerOnOffRecord;               //停/上电变量

extern INT16U         debugInfo;                      //调试信息控制
extern INT16U         debugInfox;                     //调试信息控制-扩展
extern INT8U          flagOfClearData;                //删除数据标志
extern INT8U          flagOfClearPulse;               //删除脉冲中间量数据标志

extern INT32U         workSecond;                     //工作时长(s)
extern INT32U         ipStartSecond;                  //IP工作时长(s)

extern BOOL           ifReset;                        //复位?
extern INT8U          numOfDayReset;                  //日复位次数
extern INT8U          moduleType, bakModuleType;      //通信模块类型
extern char           vers[5];                        //程序版本
extern char           dispenseDate[11];               //程序发布日期
extern char           hardwareVers[5];                //硬件版本
extern char           hardwareDate[11];               //硬件发布日期

extern char           csNameId[13];                   //厂商名称及代码
extern INT8U          localCopyForm;                  //本地通信模块抄读方式
extern INT8U          denizenDataType;                //居民用户电表数据类型
extern INT8U          cycleDataType;                  //轮显数据类型
extern INT8U          mainTainPortMode;               //维护串口模式
extern INT8U          rs485Port2Fun;                  //第2路485接口功能
extern INT8U          lmProtocol;                     //本地通信模块协议,默认为Q/GDW376.2

extern INT8U          statusOfGate;                   //跳闸指示
extern INT8U          oneSecondTimeUp;                //1秒定时到期
extern INT8U          cmdReset;                       //收到复位命令后的计数
extern INT8U          setParaWaitTime;                //设置参数显示停留时间
extern INT8U          lcdDegree;                      //LCD对比度值

extern INT8U        msFrame[SIZE_OF_SEND_MS_BUF];     //发送给主站的帧
extern INT8U        activeFrame[SIZE_OF_SEND_MS_BUF]; //主动上报给主站的帧
extern INT8U        recvMsBuf[SIZE_OF_RECV_MS_BUF];   //接收主站帧缓冲(本地控制端口和远程端口共用)

extern INT8U                    transmitState;        //传输层状态

extern INT8U                    pfc;                  //启动帧帧序号计数器
extern INT8U                    pSeq;                 //启动帧序号
extern INT8U                    rSeq;                 //响应帧序号

//extern ETH_ADDR                 addrOfEth;            //以太网地址
extern ADDR_FIELD               addrField;            //地址域
extern FRAME_QUEUE              fQueue;               //帧队列

extern ANALYSE_FRAME            frame;                //分析帧结构

//状态量
extern INT8U                    stOfSwitch;           //状态量状态ST
extern INT8U                    cdOfSwitch;           //状态量变位CD

//事件记数器
extern INT16U                   iEventCounter;        //重要事件计数器EC1
extern INT16U                   nEventCounter;        //一般事件计数器EC2
extern INT8U                    eventReadedPointer[2];//事件记录计数器值(最大事件指针第1字节)及已读事件记录指针(第2字节)

extern INT8U                    teInRunning;          //终端投入运行(重庆规约)

//确认/否认/无有效数据队列
extern INT8U                    ackData[100];            

//定时发送时间表
extern REPORT_TIME              reportTime1;          //定时发送一类数据时间表
extern REPORT_TIME_2            reportTime2;          //定时发送二类数据时间表
extern INT8U                    initReportFlag;       //初始化任务队列标志

//允许/禁止通话、主动上报(高2位表示通话,低2位表示主动上报)
extern INT8U                    callAndReport;

extern INT8U                    lastInterval;         //指定抄表时间的上一次抄表间隔

extern INT8U                    meterTimeOverLoad[8]; //电能表时间超差标志(64个测量点,每个测量点占1位)

#ifdef LOAD_CTRL_MODULE
 extern BOOL                    balanceComplete;      //结算完成
 extern INT8U                   gateJumpTime[CONTROL_OUTPUT]; //跳闸后计数值
#endif

#ifdef PULSE_GATHER
 extern ONE_PULSE pulse[NUM_OF_SWITCH_PULSE];           //脉冲量采集
 extern INT8U     pulseDataBuff[NUM_OF_SWITCH_PULSE*53];//脉冲量数据缓存
#endif

#ifdef WDOG_USE_X_MEGA
 extern INT8U                   xMegaFrame[2048];      //发送给xMega的帧
 extern XMEGA_FRAME_QUEUE       xMegaQueue;
 extern INT8U                   gatherModuleType;      //采集模块类型
 extern INT8U                   xMegaHeartBeat;        //向xMega发送心跳
#endif

#ifdef TE_CTRL_BOARD_V_1_3
 extern INT8U                   gateKvalue;            //门接点状态
#endif

extern INT8U                    acMsa;

extern INT8U                    upRtFlag;              //升级路由标志

extern INT8U                    _updating_flag;        //东软路由升级标志(1-在升级,0-未升级,定义在esRtUpdate.c中)
extern INT8U                    update_step;           //东软路由升级时的步骤(定义在esRtUpdate.c中)

#ifdef PLUG_IN_CARRIER_MODULE
 extern INT16U                  adcData;               //直流模拟量实时值
#endif

extern int                      socketOfUdp;            //UDP的socket

#endif   /*__teRunParaH*/
