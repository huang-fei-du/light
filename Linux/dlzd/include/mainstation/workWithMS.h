/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：workWithMS.h
作者：TianYe
版本：0.9
完成日期：2006年5月
描述：与主站有关的参数头文件
函数列表：
修改历史:
  01,06-05-29,Leiyong created.
  02,07-07-27,Leiyong modify,将发给主站的帧增加到8192字节,以适应主站请求数据量大的情况
             (例如,主站要请求32个测量点的F33,F34的数据就需要用到这么大的发送缓存)
  03,09-12-08,Leiyong,移植到AT91SAM9260.
  04,10-12-17,Leiyong,兰州供电公司测试发现:F38,F39 1类、2类数据配置设置成功而召测不回来
              原因:原来的做法是不正确的,只留了一个大类号的位置,而规约要求可以设置15个大类号的配置,
              修正:改为可以设置15个大类号的配置。 
  
***************************************************/
#ifndef __INCworkWithMSh
#define __INCworkWithMSh

#include "common.h"
#include "timeUser.h"

#define PROCESSING              1 //正在处理其他帧
#define PROCESS_DONE            0 //资源空闲，可以处理新的帧

#define CHECK_PW_FAILED         1 //密码校验未通过
#define CHECK_PW_OK             0 //密码校验通过

//通用地址
#define BROADCASTADDR      0xffff

//AFN功能代码
#define RESPONSE              0x0 //确认/否认,只在上行帧中出现
#define RESET_CMD             0x1 //复位
#define LINK_IF_CHECK         0x2 //链路接口检测
#define RELAY_STATION_COMMAND 0x3 //中继站命令
#define SET_PARAMETER         0x4 //参数设置
#define CTRL_COMMAND          0x5 //控制命令
#define AUTHENTICATION        0x6 //身份认证及密钥协商
#define REQUEST_CASCADE_TE    0x8 //请求被级联终端主动上报
#define REQUEST_TE_CONFIG     0x9 //请求终端配置
#define INQUIRE_INDEX         0xA //查询参数
#define INQUIRE_QUEST_INDEX   0xB //查询任务数据
#define INQUIRE_TYPE_1        0xC //请求1类数据(实时)
#define INQUIRE_TYPE_2        0xD //请求2类数据(历史)
#define INQUIRE_TYPE_3        0xE //请求3类数据(事件)
#define FILE_TRANSPORT        0xF //文件传输
#define DATA_FORWARD         0x10 //数据转发

#define FIND_FN               0x1 //查找Fn
#define FIND_PN               0x2 //查找Pn

#define SIZE_OF_SEND_MS_BUF  9600 //发给主站的帧大小(32帧*300Bytes)
#define SIZE_OF_RECV_MS_BUF  2048 //接收主站的帧大小
#define LEN_OF_SEND_QUEUE     128 //发送队列长度

//主站数据来自?
#define DATA_FROM_LOCAL       0x1 //主站数据来自本地控制RS232口
#define DATA_FROM_GPRS        0x2 //主站数据来自GPRS接口
#define DATA_FROM_IR          0x3 //主站数据来自红外
#define DATA_FROM_RS485_2     0x4 //主站数据来自RS485-2口(设置为维护口)

//投切控制方式
#define LOCAL_CONTRL      0x01
#define REMOTE_CONTRL     0x02
#define LOCK_OFF          0x03
#define LOCK_ON           0x04

//数据查询/主动上报标志
#define QUERY             0x00    //数据查询标志
#define ACTIVE_REPORT     0x01    //数据主动上报标志
#define AFN0B_REQUIRE     0x10    //查询任务

//定时发送数据任务开启/停止标志
#define  TASK_STOP        0xaa    //任务停止标志
#define  TASK_START       0x55    //任务开启标志
#define  TASK_INVALID     0x00    //任务启动代码无效

#define  RELAY_IN_USE     0x00    //允许中继转发
#define  RELAY_NO_USE     0x01    //禁止中继转发

//结构 - 地址域
typedef struct
{
   INT8U a1[2];                   //行政区划码
   INT8U a2[2];                   //终端地址
   INT8U a3;                      //主站地址和组地址标志
}ADDR_FIELD;

//结构 - 事件记录头
typedef struct
{
	 INT8U     erc;                 //事件记录代码
	 INT8U     length;              //存储长度
	 DATE_TIME time;                //发生时间 
}EVENT_HEAD;

//用于保存接收分组中各字段值的全局变量
typedef struct 
{
  INT16U  l1, l2;                 //帧长度(2个字节合并成一个16进制数)
  INT8U   c;                      //控制域
  INT16U  a1, a2;                 //地址[a1为行政区划码,a2为终端地址]
  INT8U   a3;                     //主站地址
  INT8U   *pData;                 //净数据开始指针(afn开始)
  INT8U   *pDataEnd;              //净数据结束指针
  INT8U   afn;                    //AFN
  INT8U   seq;                    //帧序号域
  INT8U   *pw;                    //密码指针
  INT8U   *pTp;                   //帧时间标签有效Tp指针
  INT8U   cs;                     //帧校验和
  INT8U   acd;                    //事件标志位
  INT16U  loadLen;                //净用户数据长度
}ANALYSE_FRAME;

//结构 - 帧队列
typedef struct
{
	 BOOL   inTimeFrameSend;        //及时帧发送
	 INT8U  thisStartPtr;           //本次处理指针
	 INT8U  tailPtr;                //尾指针
	 INT8U  sendPtr;                //发送帧指针
	 struct
	 {
	   INT16U head;
	   INT16U len;                  //帧长度
	   INT8U  next;                 //下一帧
	 }frame[LEN_OF_SEND_QUEUE];
	 
	 BOOL   continueActiveSend;     //继续上送下一帧主动上报帧
	 BOOL   activeFrameSend;        //主动上报帧发送
	 INT8U  activeThisStartPtr;     //主动上报本次处理指针
	 INT8U  activeTailPtr;          //主动上报尾指针
	 INT8U  activeSendPtr;          //主动上报发送帧指针	 
	 struct
	 {
	   INT16U head;
	   INT16U len;                  //帧长度
	   INT8U  next;                 //下一帧
	 }activeFrame[LEN_OF_SEND_QUEUE];
	 
	 INT8U     delay;               //帧之间的延时
	 DATE_TIME delayTime;           //延时发送绝对时间
	 
	 INT8U     active0dDataSending; //0D主动上报数据正在发送标志
	 INT8U     active0dTaskId;      //0D主动上报数据任务ID
}FRAME_QUEUE;

#ifdef WDOG_USE_X_MEGA
//结构 - xMega帧队列
typedef struct
{
	 BOOL   inTimeFrameSend;        //及时帧发送
	 INT8U  thisStartPtr;           //本次处理指针
	 INT8U  tailPtr;                //尾指针
	 INT8U  sendPtr;                //发送帧指针
	 struct
	 {
	   INT16U head;
	   INT16U len;                  //帧长度
	   INT8U  next;                 //下一帧
	 }frame[16];
	 
	 INT8U     delay;               //帧之间的延时
}XMEGA_FRAME_QUEUE;
#endif

//结构 -- 终端上行通信口通信参数设置(终端通信参数设置)(AFN04-FN01)
typedef struct
{
  INT8U rts;                			//终端数传机延时时间RTS
  INT8U delay;              			//终端作为启动站允许发送传输延时时间
  
  INT8U timeOutReSendTimes[2];    //终端等待从动站响应的超时时间和重发次数
  
  INT8U flagOfCon;          			//需要主站确认的通信服务(con=1)的标志
  INT8U heartBeat;          			//心跳周期
}COMM_PARA;

//结构 -- 终端上行通信口无线中继转发设置(终端中继转发设置)(AFN04-FN02)
typedef struct
{
  INT8U  relayAddrNumFlg;         //被转发的终端地址数和使用标志
  INT8U  relayAddr[16][2];        //被转发终端地址
}RELAY_CONFIG;

//结构 － 主站IP地址和端口(AFN04-F3)
typedef struct
{
  INT8U ipAddr[4];                //主用IP地址
  INT8U port[2];                  //主用端口
  INT8U ipAddrBak[4];             //备用IP地址
  INT8U portBak[2];               //备用端口
  INT8U apn[16];                  //APN
}IP_AND_PORT;

//结构 - 主站电话号码和短信中心号码(AFN04-F4)
typedef struct
{
  INT8U phoneNumber[8];           //主站电话号码或主站手机号码
  INT8U smsNumber[8];             //短信中心号码
}PHONE_AND_SMS;

//结构 - 终端IP地址和端口(AFN04-07)
typedef struct
{
  INT8U teIpAddr[4];              //终端IP地址
  INT8U mask[4];                  //子网掩码地址
  INT8U gateWay[4];               //网关地址
  INT8U proxyType;                //代理类型
  INT8U proxyServer[4];           //代理服务器地址
  INT8U proxyPort[2];             //代理服务器端口
  INT8U proxyLinkType;            //代理服务器连接方式
  INT8U userNameLen;              //用户名长度m
  INT8U userName[20];             //用户名
  INT8U passwordLen;              //密码长度n
  INT8U password[20];             //密码
  INT8U listenPort[2];            //终端侦听端口
  INT8U ethIfLoginMs;             //是否使用以太网登录主站
}TE_IP_AND_PORT;

//结构 - 终端上行通信工作方式(以太专网或虚拟专网)(AFN04-08)
typedef struct
{
  INT8U workMethod;               //工作模式
  INT8U redialInterval[2];        //永久在线、时段在线模式重拨间隔
  INT8U maxRedial;                //被动激活模式重拨次数
  INT8U closeConnection;          //被动激活模式连续无通信自动断线时间
  INT8U onLinePeriodTime[3];      //时段在线模式允许在线时段标志
}PRIVATE_NET_METHOD;

//结构--终端事件记录配置(AFN04-F9)
typedef struct
{
  INT8U  nEvent[8];               //事件记录有效标志位(一般事件)
  INT8U  iEvent[8];	              //事件重要性等级标志位(重要事件)
}EVENT_RECORD_CONFIG;

//结构--终端电能表/交流采样装置配置参数(AFN04-F10)
//该结构表示一块表的信息
typedef struct
{
  INT16U number;                  //电能表/交流采样装置序号
  INT16U measurePoint;            //所属测量点号
  INT8U  rateAndPort;             //通信速率及端口号
  INT8U  protocol;                //通信规约类型
  INT8U  addr[6];                 //通信地址
  INT8U  password[6];             //通信密码
  INT8U  numOfTariff;             //费率个数
  INT8U  mixed;                   //有功电能示值整数位及小数位
  INT8U  collectorAddr[6];        //所属采集器通信地址
  INT8U  bigAndLittleType;        //用户大类及用户小类号
}METER_DEVICE_CONFIG;

//结构--终端脉冲配置参数(AFN04-011)
typedef struct
{
  INT8U     numOfPulse;           //脉冲配置个数
  struct
  {
    INT8U ifNo;                   //脉冲输入端口号
    INT8U pn;                     //所属测量点号
    INT8U character;              //脉冲属性
    INT8U meterConstant[2];       //电表常数k
  }perPulseConfig[NUM_OF_SWITCH_PULSE];
  //规约要求是64路脉冲,但我们的集中器只有2路脉冲量输入
}PULSE_CONFIG;

//结构--电压/电流模拟量配置参数(AFN04-FN13)
typedef struct
{
  INT16U  numOfSimu;
  struct
  {
    INT8U ifNo;						        //电压/电流模拟量输入端口号
    INT8U pn;							        //所属测量点号
    INT8U character;			        //电压/电流模拟量属性
  }perIUConfig[64];
}IU_SIMULATE_CONFIG;

//结构--终端总加组配置参数(AFN04-FN14)
typedef struct
{
	INT8U   numberOfzjz;            //总加组数量
	struct
  {
	  INT8U zjzNo;                  //总加组序号
	  INT8U pointNumber;            //本总加组测量点数量
    INT8U measurePoint[64];       //每个测量点及总加标志
  }perZjz[8];                     //每个总加组具体情况
}TOTAL_ADD_GROUP;

//结构--电能量差动越限配置(AFN04-FN15)
typedef struct
{
  INT16U     numOfConfig;				  //有功总电能量差动组配置数量
  struct
  {
    INT16U  groupNum;					    //有功总电能量差动组序号
    INT8U   toCompare;					  //对比的总加组序号
    INT8U   toReference;				  //参照的总加组序号
    INT8U   timeAndFlag;				  //参与差动的电能量的时间区间及对比方法标志
    INT8U   ralitaveDifference;   //差动越限相对偏差值
    INT8U   absoluteDifference[4];//差动越限绝对偏差值
       
    INT8U   startStop;            //越限起止标志(不是规约规定,是记录的需求)
  }perConfig[8];//结构--单组电能量差动越限配置
   
}ENERGY_DIFFERENCE_CONFIG;

//结构 - 虚拟专网用户名、密码(AFN04-FN16)
typedef struct
{
	INT8U vpnName[32];              //虚拟专网用户名
	INT8U vpnPassword[32];          //虚拟专网密码
}VPN;

//结构 -- 控制参数(AFN04-FN18,FN19,FN20)
typedef struct
{
  INT8U  pCtrlPeriod[12];         //终端功控时段
  INT8U  pCtrlIndex;              //时段功控定值浮动系数
  INT8U  monthEnergCtrlIndex;     //终端月电能量控定值浮动系数
}CONTRL_PARA;

//结构 - 终端电能量费率(AFN04-FN22)
typedef struct
{
  INT8U chargeNum;        		    //费率数
  INT8U chargeRate[48][4];        //费率
}CHARGE_RATE_NUM;

//结构--测量点基本参数(AFN04-F25)
typedef struct
{
  INT16U voltageTimes;            //电压互感器倍率
  INT16U currentTimes;            //电流互感器倍率
  INT16U ratingVoltage;           //额定电压
  INT8U  maxCurrent;              //额定电流
  INT8U  powerRating[3];          //额定负荷
  INT8U  linkStyle;               //电源接线方式
}MEASURE_POINT_PARA;

//结构--测量点限制参数(AFN04-026)
typedef struct
{
  INT16U   vUpLimit;              //电压合格上限
  INT16U   vLowLimit;             //电压合格下限
  INT16U   vPhaseDownLimit;       //电压断相门限
     
  INT16U   vSuperiodLimit;        //电压上上限(过压门限)
  INT8U    vUpUpTimes;            //电压上上限越限持续时间
  INT8U    vUpUpResume[2];        //电压上上限越限恢复系数
     
  INT16U   vDownDownLimit;        //电压下下限(欠压门限)
  INT8U    vDownDownTimes;        //电压下下限越限持续时间
  INT8U    vDownDownResume[2];    //电压下下限越限恢复系数
  
  INT8U    cSuperiodLimit[3];     //相电流上上限(过流门限
  INT8U    cUpUpTimes;            //相电流上上限越限持续时间
  INT8U    cUpUpReume[2];         //相电流上上限越限恢复系数
     
  INT8U    cUpLimit[3];           //相电流上限(额定电流门限)
  INT8U    cUpTimes;              //相电流上限越限时间
  INT8U    cUpResume[2];          //相电流上限越限恢复系数
     
  INT8U    cZeroSeqLimit[3];      //零序电流上限
  INT8U    cZeroSeqTimes;         //零序电流越限持续时间
  INT8U    cZeroSeqResume[2];     //零序电流越限恢复系数
  
  INT8U    pSuperiodLimit[3];     //视在功率上上限
  INT8U    pSuperiodTimes;        //视在功率越上上限持续时间
  INT8U    pSuperiodResume[2];    //视在功率越上上限恢复系数
     
  INT8U    pUpLimit[3];           //视在功率上限
  INT8U    pUpTimes;              //视在功率越上限持续时间
  INT8U    pUpResume[2];          //视在功率越上限恢复系数
        
  INT8U    uPhaseUnbalance[2];    //三相电压不平衡限值
  INT8U    uPhaseUnTimes;         //三相电压不平衡越限持续时间
  INT8U    uPhaseUnResume[2];     //三相电压不平衡越限恢复系数
  
  INT8U    cPhaseUnbalance[2];    //三相电流不平衡限值
  INT8U    cPhaseUnTimes;         //三相电流不平衡越限持续时间
  INT8U    cPhaseUnResume[2];     //三相电流不平衡越限恢复系数
  
  INT8U    uLostTimeLimit;        //连续失压时间限值
 
}MEASUREPOINT_LIMIT_PARA;

#ifdef LIGHTING

//结构--控制点限制参数(AFN04-052)
typedef struct
{
  INT16U   vSuperiodLimit;        //电压过压门限
  INT16U   vDownDownLimit;        //电压欠压门限

  INT8U    cSuperiodLimit[3];     //电流过流门限
  INT8U    cDownDownLimit[3];     //电流欠流门限
  
  INT8U    pUpLimit[3];           //功率上限
  INT8U    pDownLimit[3];         //功率下限
  
  INT8U    factorDownLimit[2];		//功率因数下限
  
  INT8U    overContinued;         //越限持续时间(分)
}PN_LIMIT_PARA;

#endif

//结构 - 测量点铜损、铁损参数(AFN04-027)
typedef struct
{
	INT8U aResistance[2];           //A相电阻
	INT8U aReactance[2];            //A相电抗
	INT8U aConductance[2];          //A相电导
	INT8U aSusceptance[2];          //A相电纳

	INT8U bResistance[2];           //B相电阻
	INT8U bReactance[2];            //B相电抗
	INT8U bConductance[2];          //B相电导
	INT8U bSusceptance[2];          //B相电纳

	INT8U cResistance[2];           //C相电阻
	INT8U cReactance[2];            //C相电抗
	INT8U cConductance[2];          //C相电导
	INT8U cSusceptance[2];          //C相电纳
}COPPER_IRON_LOSS;

//结构 - 测量点功率因数分段限值(AFN04-F28)
typedef struct
{
  INT8U  segLimit1[2];						//功率因数分段限值1
  INT8U  segLimit2[2];						//功率因数分段限值2
}POWER_SEG_LIMIT;

//结构 - 载波从节点附属节点地址(AFN04-F31)
typedef struct
{
  INT8U numOfAuxiliaryNode;       //载波从节点附属节点地址个数
	INT8U auxiliaryNode[120];       //载波从节点附属节点地址
}AUXILIARY_ADDR;

//结构 - 终端抄表运行参数设置(AFN04-F33)
typedef struct
{
  INT8U  numOfPara;               //参数块个数
  struct
  {
    //规约要求下发的参数
    INT8U commucationPort;        //终端通道端口号
    INT8U copyRunControl[2];      //台区集中抄表运行控制字
    INT8U copyDay[4];             //抄表日-日期
    INT8U copyTime[2];            //抄表时间
    INT8U copyInterval;           //抄表间隔
    INT8U broadcastCheckTime[3];  //广播校时
    INT8U hourPeriodNum;          //允许抄表时段数m(0<=m<=24)
    INT8U hourPeriod[48][2];      //允许抄表时段
 #ifdef SUPPORT_ETH_COPY
  }para[6];                       //2014-4-17,增加一个ETH抄表端口(端口为5)
 #else
  }para[5];	                      //各个参数块(集中器只有5个接口,所以这里只支持5个数据块),   //2012-3-27,从4个接口变成5个
 #endif
}TE_COPY_RUN_PARA;

//结构 - 集中器下行通信模块的参数设置(AFN04-F34)
typedef struct
{
  INT8U  numOfPara;    //参数个数
  struct
  {
 	  INT8U commucationPort;        //终端通信端口号
 	  INT8U commCtrlWord;           //与终端接口端的通信控制字
 	  INT8U commRate[4];            //与终端接口对应端的通信速率	 	  
  }para[32];  //各参数块
}DOWN_RIVER_MODULE_PARA;

//结构 - 台区集中抄表重点户设置(AFN04-F35)
typedef struct
{
  INT8U  numOfHousehold;          //重点户个数
  INT8U  household[40];           //重点户电表装置序号
}KEY_HOUSEHOLD;

//结构 - 终端级联通信参数(AFN04-037)
typedef struct
{
  INT8U commPort;                 //终端级联通信端口号
  INT8U ctrlWord;                 //终端级联通信控制字
  INT8U receiveMsgTimeout;        //接收报文超时时间
  INT8U receiveByteTimeout;       //接收等待字节超时时间
  INT8U cascadeMretryTime;        //级联方(主动站)接收失败重发次数
  INT8U groundSurveyPeriod;       //级联巡测周期
  INT8U flagAndTeNumber;          //级联标志及终端个数
  INT8U divisionCode[6];          //终端行政区划码
  INT8U cascadeTeAddr[6];         //级联终端地址
}CASCADE_COMM_PARA;

//结构 - 1类及2类数据配置设置(AFN04-038,039)
//10-12-17,Leiyong,兰州供电公司测试发现:F38,F39 1类、2类数据配置设置成功而召测不回来
//             原因:原来的做法是不正确的,只留了一个大类号的位置,而规约要求可以设置16个大类号的配置,
//             修正:改为可以设置16个大类号的配置。
typedef struct
{
  struct
  {
    INT8U groupNum;               //该大类下的小类号个数
    struct
    {
	    INT8U infoGroup;            //信息类组数
	    INT8U flag[32];             //信息类元标志位
    }littleType[16];	            //小类号16个,用户小类号用数组下标表示
  }bigType[16];                   //大类号16个,用户大类号用数组下标表示
}TYPE_1_2_DATA_CONFIG;

//结构--控制投运状态
typedef struct
{
  INT8U       ifUsePrdCtrl;       //时段控投入
  INT8U       ifUseWkdCtrl;       //厂休控投入
  INT8U       ifUseObsCtrl;       //报停控投入
  INT8U       ifUsePwrCtrl;       //当前功率下浮控投入
  INT8U       ifUseMthCtrl;       //月电控投入
  INT8U       ifUseChgCtrl;       //购电控投入
}CTRL_RUN_STATUS;

//结构 - 进入控制状态
typedef struct
{
	INT8U  aQueue[8];               //告警队列
	INT8U  pn[8];                   //总加组号
	INT8U  allPermitClose[8];       //允许合闸所有设定控制路合闸
	INT8U  numOfAlarm;              //告警队列的成员个数
	INT8U  nowAlarm;                //当前的队列指针

}CTRL_STATUS;

//结构--终端保电状态
typedef struct
{
  INT8U      ifStaySupport;       //保电状态
  INT8U      ifHold;              //一直保电
  DATE_TIME  endStaySupport;      //保电结束时间
}STAY_SUPPORT_STATUS;

//结构--遥控参数
typedef struct
{
  //遥控参数
  INT8U      ifUseRemoteCtrl;     //遥控投入,ly,2011-08-08,add
  INT8U      ifRemoteCtrl;        //是否遥控状态
  INT8U      ifRemoteConfirm;     //是否返校成功
  DATE_TIME  confirmTimeOut;      //返校时限
  DATE_TIME  alarmStart;          //告警时限
  DATE_TIME  remoteStart;         //遥控开始时间
  DATE_TIME  remoteEnd;           //遥控结束时间
  INT8U      status;              //状态(1.告警,2.已跳闸,3.允许合闸)  2008.12.05 ly added
}REMOTE_CTRL_CONFIG;

//结构--时段功控定值及参数(AFN04-FN41及AFN05-FN009,etc)
typedef struct
{
  //F41功控定值
  INT8U      periodNum;           //时段方案标志
  struct
  {
    INT8U    timeCode;            //时段定值标志
    INT8U    limitTime[8][2];
  }period[3];
    
  //时段控控制参数
  struct
  {
    INT8U     limitPeriod;        //时段功控投入标志
    INT8U     ctrlPeriod;         //时段功控定值方案号
    INT8U     ifPrdCtrl;          //是否时段功控状态
    INT8U     prdAlarm;           //时段功控告警状态(1.告警,2.已跳闸,3.允许合闸)
    DATE_TIME alarmEndTime;       //告警结束时间
  }ctrlPara;
}PERIOD_CTRL_CONFIG;

//结构--厂休功控参数(AFN04-FN42)
typedef struct
{
  INT8U      ifWkdCtrl;           //是否厂休控状态
  INT8U      wkdAlarm;            //厂休控告警状态(1.告警,2.已跳闸,3.允许合闸)
  INT16U     wkdLimit;            //厂休控功率限值  
  INT8U      wkdStartMin;         //控制起始时间
  INT8U      wkdStartHour;
  INT8U      wkdTime;             //限电时段
  INT8U      wkdDate;             //限电周次
  DATE_TIME  alarmEndTime;        //告警结束时间
}WKD_CTRL_CONFIG;

//结构--功率控制的功率计算滑差时间(F43)
typedef struct
{
  INT8U     def;
  INT8U     countTime;	
}POWERCTRL_COUNT_TIME;

//结构--营业报停控参数(AFN04-FN44)
typedef struct
{
  INT8U      ifObsCtrl;           //是否报停控状态
  INT8U      obsAlarm;            //报停控制告警状态(1.告警,2.已跳闸,3.允许合闸)
  INT16U     obsLimit;            //报停控功率限值
  INT8U      obsStartDay;         //控制起始时间日
  INT8U      obsStartMonth;       //控制起始时间月
  INT8U      obsStartYear;        //控制起始时间年
  INT8U      obsEndDay;           //控制结束时间日
  INT8U      obsEndMonth;         //控制结束时间月
  INT8U      obsEndYear;          //控制结束时间年
  DATE_TIME  alarmEndTime;        //告警结束时间
}OBS_CTRL_CONFIG;

//结构--当前功率下浮控参数(这个参数不是在AFN04中定义的,而是在下浮控投入(AFN05)时下发的,但为了整齐插入在功率控制参数一起)
typedef struct
{
  INT8U      ifPwrDownCtrl;       //是否下浮控状态
  INT8U      pwrDownAlarm;        //报停控制告警状态(1.告警,2.已跳闸,3.允许合闸)
	  
	INT8U      slipTime;            //当前功率下浮控定值滑差时间
	INT8U      floatFactor;         //当前功率下浮控定值浮动系数
	INT8U      freezeDelay;         //控后总加有功功率冻结延时时间
	  
	INT32U     powerDownLimit;      //保存定值Kw
	INT16U     powerLimitWatt;      //保存定值watt
	       
	DATE_TIME  freezeTime;          //冻结当时负荷的时刻	  
	DATE_TIME  alarmEndTime;        //告警结束时间
	  
	INT8U      downCtrlTime;        //当前功率下浮的控制时间
	INT8U      roundAlarmTime[4];   //当前功率下浮控告警时间(第1轮到第4轮)
	 
}POWER_DOWN_CONFIG;

//结构--功控轮次设定(F45)
typedef struct
{
  INT8U     ifJumped;             //是否跳闸(BS8,按位对位)
  INT8U     flag;
}POWERCTRL_ROUND_FLAG;

//结构--月电量控定值(AFN04-FN46)
typedef struct
{
  INT8U      ifMonthCtrl;         //是否月电控状态
  INT8U      monthAlarm;          //是否超限告警
  INT32U     monthCtrl;           //月电控电量限值
  DATE_TIME  mthAlarmTimeOut;     //月电控告警超时
}MONTH_CTRL_CONFIG;

//结构--购电量控参数(AFN04-FN47)
typedef struct
{
  //控制参数
  INT8U      ifChargeCtrl;        //是否购电控状态
  INT8U      chargeAlarm;         //购电量控制告警状态(1.告警,2.已跳闸,3.允许合闸)
  DATE_TIME  alarmTimeOut;        //告警时限
  
  //最近一次下发的购电量控参数
  INT8U      flag;                //追加刷新标志
  INT32U     numOfBill;           //购电单号
  INT32U     chargeCtrl;          //购电量值
  INT32U     alarmLimit;          //告警门限
  INT32U     cutDownLimit;        //跳闸门限
}CHARGE_CTRL_CONFIG;

//结构--电控轮次设定(F48)
typedef struct
{
  INT8U     ifJumped;
  INT8U     flag;
}ELECTCTRL_ROUND_FLAG;

//结构--功控告警时间(F49)
typedef struct
{
  INT8U     def;
  INT8U     alarmTime;
}POWERCTRL_ALARM_TIME;

//遥控现场记录
typedef struct
{
	INT8U      remotePower[2];
	INT8U      gate;
	DATE_TIME  remoteTime;
	DATE_TIME  freezeTime;
}REMOTE_EVENT_INFOR;

//功控现场记录
typedef struct
{
  INT8U   numOfCtrl;              //功控事件未记录数
  struct
  {
    INT8U      pn;                //总加组号
    DATE_TIME  ctrlTime;          //功控跳闸时间
    INT8U      gate;              //功控跳闸轮次
    INT8U      ctrlType;          //功控类型
    INT8U      ctrlPower[2];      //跳闸前(总加)功率
    INT8U      limit[2];          //跳闸时功率定值
    DATE_TIME  freezeTime;        //应当记录事件时间
  }perCtrl[6];
}POWER_CTRL_EVENT_INFOR;

//结构 - 电能表异常判别阈值设定(AFN04-F59)
typedef struct
{
	INT8U powerOverGate;            //电能量超差阈值
	INT8U meterFlyGate;             //电能表飞走阈值
	INT8U meterStopGate;            //电能表停走阈值
	INT8U meterCheckTimeGate;       //电能表校时阈值
}METER_GATE;

//结构--谐波限值(AFN04-FN60)
typedef struct
{
  INT8U totalUPercentUpLimit[2];	//总畸变电压含有率上限
    
  INT8U oddUPercentUpLimit[2];		//奇次谐波电压含有率上限
  INT8U evenUPercentUpLimit[2];		//偶次谐波电压含有率上限
    
  INT8U UPercentUpLimit[18][2];		//谐波电压含有率上限
    
  INT8U totalIPercentUpLimit[2];	//总畸变电流有效值上限
    
  INT8U IPercentUpLimit[18][2];		//谐波电流有效值上限
}WAVE_LIMIT;

//结构--定时发送数据参数配置(AFN04-F65,F66,F67,F68)  F65,F67共用F65  F66,F68共用F68
typedef struct
{
  INT8U       numOfTask;          //任务个数
  struct
  {
    INT8U     stopFlag;           //任务开启/停止标志
 
    INT8U     taskNum;            //任务代码(支持1~64个任务)
    INT8U     sendPeriod;         //发送周期及发送周期单位
    INT8U     week;							  //发送周期中的星期
    DATE_TIME sendStart;          //发送基准时间
    INT8U     sampleTimes;        //曲线数据抽取倍率
    INT16U    numOfDataId;        //数据单元表示个数
    struct
    {
      INT8U   fn;                 //数据单元fn
      INT8U   pn1;                //数据单元pn
      INT8U   pn2;
    }dataUnit[1024];
 }task[64];    //单个任务配置
}REPORT_TASK_PARA;

//结构--电容器参数(AFN04-FN73, FN76)
typedef struct
{
  struct
  {
	  INT8U compensationMode;				//补偿方式
	  INT8U capacityNum[2];					//电容装见容量
  }capacity[16];

  INT8U controlMode;						  //控制方式(FN76)
}CAPACITY_PARA;

//结构--电容器投切运行参数(AFN04-FN74)
typedef struct
{
  INT8U  targetPowerFactor[2];		//目标功率因数
  INT8U  onPowerLimit[3];					//投入无功功率门限
  INT8U  offPowerLimit[3];				//切除无功功率门限
  INT8U  delay;										//延时时间
  INT8U  actInterval;							//动作时间间隔
}CAPACITY_RUN_PARA;

//结构--电容器保护参数(AFN04-FN75)
typedef struct
{
  INT8U vSuperiod[2];							//过电压
  INT8U vSuperiodQuan[2];					//过电压回差值
  INT8U vLack[2];									//欠电压
  INT8U vLackQuan[2];							//欠电压回差值
  INT8U cAbnormalUpLimit[2];			//总畸变电流含有率上限
  INT8U cAbnormalQuan[2];					//总畸变电流含有率越限回差值
  INT8U vAbnormalUpLimit[2];			//总畸变电压含有率上限
  INT8U vAbnormalQuan[2];					//总畸变电压含有率越限回差值
}CAPACITY_PROTECT_PARA;

//直流模拟量配置参数(AFN04-FN81,FN82,FN83)
typedef struct
{
  //FN81
  INT8U adcStartValue[2];			    //直流模拟量量程起始值
  INT8U adcEndValue[2];				    //直流模拟量量程终止值
 
  //FN82
  INT8U adcUpLimit[2];  	        //直流模拟量上限
  INT8U adcLowLimit[2];	          //直流模拟量下限
 
  //FN83 
  INT8U adcFreezeDensity;         //直流模拟量冻结密度(实际使用长度1字节)
}ADC_PARA;

//结构--定时发送1类数据任务任务时间表
typedef struct
{
  INT8U  numOfTask;
  struct
  {
    INT8U      taskNum;
    DATE_TIME  nextReportTime;    //保存二进制时间
  }taskConfig[64];
}REPORT_TIME;

//结构--定时发送2类数据任务任务时间表
typedef struct
{
  INT8U numOfTask;
  struct
  {
    INT8U      taskNum;
    DATE_TIME  nextReportTime;    //二进制时间 
    DATE_TIME  lastReportTime;    //二进制时间
    DATE_TIME  backLastReportTime;//二进制时间
  }taskConfig[64];
}REPORT_TIME_2;

//结构--F92电表局编号<扩展>
typedef struct
{
  INT8U numOfPoint;               //测量点数量
  struct
  {
    INT8U measurePoint;           //测量点号
    INT8U dlNum[12];              //电表局编号
  }meterDevice[39];
}DL_NUMBER;

//结构 - 交流采样校正值参数
typedef struct
{
  INT8U   lineInType;             //接线方式(1-三相三线,2-三相四线,默认为三相四线)
  INT16U  vAjustTimes;            //电压校正使用的电压放大倍数
  INT16U  cAjustTimes;            //电流校正使用的电流放大倍数

  INT32U  UADCPga;                //电压通道增益寄存器值
  INT32U  HFConst;                //高频脉冲输出参数寄存器值
  INT32U  HFDouble;               //脉冲常数加倍倍数寄存器值
  INT32U  Irechg;                 //比差补偿区域寄存器值
  INT32U  EAddMode;               //合相能量累加模式寄存器值
  INT32U  EnLineFreq;             //基波测量使能控制寄存器值
  INT32U  EnHarmonic;             //基波测量与谐波测量切换寄存器值

  INT32U  FailVoltage;            //电压断相阈值
  INT32U  Istartup;               //启动电流设置

  INT32U  Iregion1;               //相位补偿区域设置寄存器值1
  INT32U  Iregion2;               //相位补偿区域设置寄存器值2
  INT32U  Iregion3;               //相位补偿区域设置寄存器值3
  INT32U  Iregion4;               //相位补偿区域设置寄存器值4
  INT32U  PgainA;                 //功率增益寄存器值A
  INT32U  PgainB;                 //功率增益寄存器值B
  INT32U  PgainC;                 //功率增益寄存器值C
  INT32U  PhsreagA;               //相位补偿寄存器值A
  INT32U  PhsreagB;               //相位补偿寄存器值B
  INT32U  PhsreagC;               //相位补偿寄存器值C
  INT32U  UgainA;                 //电压校正寄存器值A
  INT32U  UgainB;                 //电压校正寄存器值B
  INT32U  UgainC;                 //电压校正寄存器值C
  INT32U  IgainA;                 //电流校正寄存器值A
  INT32U  IgainB;                 //电流校正寄存器值B
  INT32U  IgainC;                 //电流校正寄存器值C
}AC_SAMPLE_PARA;

//结构--中文信息
typedef struct
{
	INT8U numOfMessage;             //信息条数
	struct
	{
	  INT8U typeAndNum;             //信息种类和信息编号
	  INT8U len;                    //信息长度
	  char  chn[202];               //汉字信息
	}message[8];
}CHN_MESSAGE;

//结构--转发记录
typedef struct
{
  INT8U     port;                 //转发通信端口号
  INT8U     ctrlWord;             //转发通信控制字
  INT8U     frameTimeOut;         //转发接收等待报文超时时间
  INT8U     byteTimeOut;          //转发接收等待字节超时时间
  INT8U     length;               //转发长度
  INT8U     forwardData[255];     //转发数据指针
  INT8U     dataFrom;
  BOOL      ifSend;
  
  DATE_TIME nextBytesTime;        //收到字节后的等待时间
  BOOL      receivedBytes;        //已收到回复数据
  INT8U     data[1024];           //数据缓存
  INT16U    recvLength;           //接收长度
}FORWARD_STRUCT;

//结构 - 脉冲量采集
typedef struct
{
	 BOOL      ifPlugIn;            //是否接入
	 
   INT8U     pn;                  //所属测量点号
   INT8U     character;           //脉冲属性
   INT16U    meterConstant;       //电表常数k

   INT16U    voltageTimes;        //电压互感器倍率
   INT16U    currentTimes;        //电流互感器倍率
	 
	 INT16U    pulseCount;          //本路脉冲计数
	 INT16U    pulseCountTariff[14];//本路脉冲分费率计数
	 
	 INT8U     pulseValue;          //本路脉冲IO脚当前值
	 INT8U     pulseValueBak;       //本路脉冲IO脚前一次值
	 INT8U     pulseValueBak2;      //本路脉冲IO脚前前一次值
	 
	 INT8U     findPulse;           //发现脉冲

	 INT16U    prevMinutePulse;     //前1分钟脉冲个数
	 INT16U    minutePulse;         //1分钟脉冲个数
	 struct    timeval calcTv;      //下一次计算时间
}ONE_PULSE;

#ifdef LIGHTING

//结构--单灯控制器/线路控制器控制时段(AFN04-F50)
struct ctrlTimes
{
	
  INT8U            startMonth;    //时间段起始月份
  INT8U            startDay;      //时间段起始日
  INT8U            endMonth;      //时间段结束月份
  INT8U            endDay;        //时间段结束日
  
  INT8U            deviceType;    //控制器类型
                                  //  2-单灯控制器时段
                                  //  3-线路控制器时段
                                  //  5-经纬度控制启用日
                                  //  7-光照度控制
  
  INT8U            noOfTime;      //时段号(在一个时间段中的时段号)
  
  INT8U            workDay;       //启用日,2016-08-17,Add
  
  INT8U            hour[6];       //时刻1-6时
  INT8U            min[6];        //时刻1-6分
  INT8U            bright[6];     //时刻1-6亮度或是通断
  
  struct ctrlTimes *next;         //下一节点指针
  
};

//结构 - 控制点阈值设定(AFN04-F51)
typedef struct
{
	INT8U failureRetry;         //单灯控制器发现故障重试次数
	INT8U boardcastWaitGate;    //广播命令等待时长
	INT8U checkTimeGate;        //校时阈值
	INT8U lddOffGate;           //单灯控制点离线阈值
	INT8U lddtRetry;            //搜索末端重试次数
	INT8U offLineRetry;         //离线重试次数
	INT8U lcWave;               //光控照度震荡值
	INT8U leakCurrent;          //漏电流阈值
}PN_GATE;

#endif

#endif /*__INCworkWithMSh*/
