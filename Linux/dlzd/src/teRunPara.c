/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:teRunPara.c
作者:leiyong
版本:0.9
完成日期:2009年11月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)终端运行参数文件
函数列表:
  1.
修改历史:
  01,09-10-28,Leiyong created.

***************************************************/

#include "common.h"
#include "timeUser.h"
#include "workWithMS.h"
#include "teRunPara.h"

DATE_TIME      sysTime;                          //系统时间
DATE_TIME      powerOnStatisTime;                //上电统计时间
BOOL           ifPowerOff = FALSE;               //是否停电
POWER_ON_OFF   powerOnOffRecord;                 //停/上电变量

INT16U         debugInfo = 0;                    //调试信息控制
INT16U         debugInfox = 0;                   //调试信息控制-扩展
INT8U          flagOfClearData=0;                //删除数据标志
INT8U          flagOfClearPulse=0;               //删除脉冲中间量数据标志

INT32U         workSecond=0;                     //工作时长(s)
INT32U         ipStartSecond = 0;                //IP工作时长(s)

BOOL           ifReset = FALSE;                  //复位?
INT8U          numOfDayReset;                    //日复位次数
INT8U         moduleType,bakModuleType=NO_MODULE;//通信模块类型

#ifdef CQDL_CSM
 //char           vers[5] = "CQ01";                //程序版本
 //char           dispenseDate[11] = "2010-10-31"; //程序发布日期
 char           vers[5] = "CQ02";                //程序版本
 char           dispenseDate[11] = "2014-10-20"; //程序发布日期
 char           hardwareVers[5]  = "CQ01";       //硬件版本
 char           hardwareDate[11] = "2010-10-31"; //硬件发布日期
#else
 char           vers[5] = "2.20";                //程序版本
 char           dispenseDate[11] = "2020-11-19"; //程序发布日期
 #ifdef PLUG_IN_CARRIER_MODULE
  char          hardwareVers[5]  = "v1.6";       //硬件版本
  char          hardwareDate[11] = "2011-01-05"; //硬件发布日期
 #else
  char          hardwareVers[5]  = "v1.5";       //硬件版本
  char          hardwareDate[11] = "2011-07-20"; //硬件发布日期
 #endif
#endif

char           csNameId[13];                     //厂商名称及代码
INT8U          localCopyForm = 0;                //本地通信模块抄读方式
INT8U          denizenDataType = 0;              //居民用户电表数据类型
INT8U          cycleDataType = 0;                //轮显数据类型
INT8U          mainTainPortMode = 0;             //维护串口模式,默认为维护
INT8U          rs485Port2Fun = 0;                //第2路485接口功能,默认为抄表
INT8U          lmProtocol = 0;                   //本地通信模块协议,默认为Q/GDW376.2


INT8U          statusOfGate = 0;                 //跳闸指示
INT8U          oneSecondTimeUp=0;                //1秒定时到期
INT8U          cmdReset;                         //收到复位命令后的计数
INT8U          setParaWaitTime;                  //设置参数显示停留时间

INT8U          lcdDegree;                        //LCD对比度值

INT8U        msFrame[SIZE_OF_SEND_MS_BUF];        //发送给主站的帧
INT8U        activeFrame[SIZE_OF_SEND_MS_BUF];    //主动上报给主站的帧
INT8U        recvMsBuf[SIZE_OF_RECV_MS_BUF];      //接收主站帧缓冲(本地控制端口和远程端口共用)

INT8U                    transmitState;           //传输层状态

INT8U                    pfc = 0;                 //启动帧帧序号计数器
INT8U                    pSeq = 0;                //启动帧序号
INT8U                    rSeq = 0;                //响应帧序号

//ETH_ADDR                 addrOfEth;               //以太网地址
ADDR_FIELD               addrField;               //地址域
FRAME_QUEUE              fQueue;                  //帧队列

ANALYSE_FRAME            frame;                   //分析帧结构

//状态量
INT8U                    stOfSwitch;              //状态量状态ST
INT8U                    cdOfSwitch;              //状态量变位CD

//事件记数器
INT16U                   iEventCounter;           //重要事件计数器EC1
INT16U                   nEventCounter;           //一般事件计数器EC2
INT8U                    eventReadedPointer[2];   //事件记录计数器值(最大事件指针第1字节)及已读事件记录指针(第2字节)

INT8U                    teInRunning=0;           //终端投入运行(重庆规约)

//确认/否认/无有效数据队列
INT8U                    ackData[100];

//定时发送时间表
REPORT_TIME              reportTime1;             //定时发送一类数据时间表
REPORT_TIME_2            reportTime2;             //定时发送二类数据时间表

INT8U                    initReportFlag = 0;      //初始化任务队列标志

//允许/禁止通话、主动上报(高2位表示通话,低2位表示主动上报)
INT8U                    callAndReport;

INT8U                    lastInterval;            //指定抄表时间的上一次抄表间隔

INT8U                    meterTimeOverLoad[8]={0,0,0,0,0,0,0,0};  //电能表时间超差标志(64个测量点,每个测量点占1位)

#ifdef LOAD_CTRL_MODULE
 BOOL                    balanceComplete=FALSE;   //结算完成
 INT8U                   gateJumpTime[CONTROL_OUTPUT]; //跳闸后计数值
#endif

#ifdef PULSE_GATHER
 ONE_PULSE pulse[NUM_OF_SWITCH_PULSE];           //脉冲量采集
 INT8U     pulseDataBuff[NUM_OF_SWITCH_PULSE*53];//脉冲量数据缓存
#endif

#ifdef WDOG_USE_X_MEGA
 INT8U                   xMegaFrame[2048];       //发送给xMega的帧
 XMEGA_FRAME_QUEUE       xMegaQueue;
 INT8U                   gatherModuleType=0;     //采集模块类型
 INT8U                   xMegaHeartBeat=0;       //向xMega发送心跳
#endif

#ifdef TE_CTRL_BOARD_V_1_3
 INT8U                   gateKvalue = 0;         //门接点状态
#endif

INT8U                    acMsa;                  //校交采的MSA

INT8U                    upRtFlag = 0;           //升级路由标志

#ifdef PLUG_IN_CARRIER_MODULE
 INT16U                  adcData = 0;            //直流模拟量实时值
#endif

int                      socketOfUdp;            //UDP的socket

