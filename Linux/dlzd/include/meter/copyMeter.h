/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：copyMeter.h
作者：leiyong
版本：0.9
完成日期:2006年7月
描述：电力负荷管理系统现场终端抄表任务头文件
      定义了所需的结构及函数声明。
修改历史：
  01,06-7-29,leiyong created.
**************************************************/

#ifndef __copyMeterH
#define __copyMeterH

#include "common.h"
#include "timeUser.h"
#include "workWithMeter.h"

extern INT8U  ifSystemStartup;                 //系统刚启动?

//结构 - 抄表地址链表
struct cpAddrLink
{
	INT16U            mpNo;                       //测量点序号
	INT16U            mp;                         //测量点号
	INT8U             protocol;                   //协议
	INT8U             addr[6];                    //通信地址
	INT8U             collectorAddr[6];           //采集器地址
	INT8U             port;                       //端口
	INT8U             bigAndLittleType;           //用户大类号及用户小类号
    
  BOOL              copySuccess;                //抄表是否成功
  BOOL              thisRoundCopyed;            //载波本轮是否抄到
   
 
 #ifndef LIGHTING 
  
  INT8U             flgOfAutoCopy;              //路由请求自动抄读标志(载波模块有效)
                                                // 0 - 所有数据未抄读
                                                // 1 - 抄读日冻结数据...
                                                // 2 - 抄读日冻结数据完成
                                                // 3 - 抄读小时冻结数据...
                                                // 4 - 抄读小时冻结数据完成
                                                // 5 - 抄读实时数据...
                                                // 6 - 抄读实时数据完成
                                                // 7 - 抄表失败(本表抄完但无有效数据视为抄表失败)
  
 #else    //照明集中器特有
 
  INT32U            flgOfAutoCopy;
  DATE_TIME         statusTime;                 //采集状态时刻
  INT8U             status;                     //灯亮度/线路状态
  INT8U             statusUpdated;              //状态已更新(更新后的状态需上送到服务器),2015-10-26,add
  INT8U             lineOn;                     //线路已上电,2015-11-09,add
  INT8U             msCtrlCmd;                  //主站直接控制命令
  DATE_TIME         msCtrlTime;                 //主站直接控制截止时间
   
  INT8U             ctrlTime;                   //单灯控制点和线路控制点的控制时段号/报警控制器的线路电流高字节

  INT8U             duty[6];                    //单灯控制器 - 6组可调光镇流器(/可调电源)占空比(相应的亮度值用采集器地址储存)
                                                //报警控制器 - 第0到3字节存储当前测量的电流值,第4,5字节存储当前测量电压值
                                                //照度传感器 - 第0到2字节存储当前测量的照度值,第3到5字节存储上一次的照度值
                                                //线路控制点 - 第0、1字节日出时刻,第2、3字节日落时刻,
                                                //             第4字节表示主站设置的合闸微调值,第5字节表示主站设置的分闸微调值

  INT8U             funOfMeasure;               //是否有计量功能
                                                //  单灯控制器 - 是否有计量功能
                                                //  单灯控制器 - 是否有计量功能
                                                //  报警控制点 - 末端单灯控制器的心跳时间,2015-5-25

  INT8U             joinUp;                     //是否接入
                                                //  单灯控制器 - 是否接入灯具
                                                //  线路控制点 - 是否接入反馈遥信
                                                //  报警控制点 - 上电后载波的建立时间

  INT8U             startCurrent;               //启动电流
                                                //  单灯控制器的启动电流,用于判断单灯故障
                                                //  报警控制器的线路电流低字节,用于判断线路故障
  
  INT8U             lddt1st[6];                 //报警控制点用第一只末端单灯控制器
  INT8U             lddtRetry;                  //搜索LDDT重试次数
  DATE_TIME         lddtStatusTime;             //采集到LDDT状态时集中器的时间,2016-02-03,Add

  INT8U             offLineRetry;               //搜索离线单灯控制器重试次数,2015-11-11,Add
  
  INT8U             lcOnOff;                    //光控电源开关操作标志,2016-01-20,Add
  INT8U             lcDetectOnOff;              //根据光控检测的电源开关标志,2016-01-21,Add
	DATE_TIME         lcProtectTime;              //光控合闸、分闸保护时间,2017-01-09,Add
  
  INT8U             gateOn;                     //线路控制点从分闸变为合闸,2016-01-20,Add

  INT8U             softVer;                    //软件版本号,2016-11-04,Add
  
 #endif
 
 #ifdef SUPPORT_ETH_COPY
 
  BOOL              copying;                    //抄表中...
  DATE_TIME         nextCopyTime;               //以太网抄表下次抄表时间
  
  INT8U             copyedData;                 //抄到了数据?
  INT8U             dataBuff[1536];             //数据缓存

  INT8U             recvBuf[384];               //接收数据
  INT16U            lenOfRecv;                  //接收数据长度
  
  INT8U             copyItem;                   //当前抄表项数
  INT8U             totalCopyItem;              //总的应抄项数

  BOOL              copyContinue;               //继续抄表
  BOOL              flagOfRetry;                //重试
  INT8U             retry;                      //重新发起抄读某项数据抄读
  DATE_TIME         copyTimeOut;                //抄表超时时间

 #endif

	struct cpAddrLink *next;                      //下一节点指针
};

//结构 - 数据转发
typedef struct
{
  INT8U     fn;                                 //数据转发的Fn
  INT8U     ctrlWord;                           //转发通信控制字
  INT8U     frameTimeOut;                       //转发接收等待报文超时时间s/ms
  INT8U     byteTimeOut;                        //转发接收等待字节超时时间
  INT16U    length;                             //转发长度
  INT16U    recvFrameTail;                      //接收字节帧尾
  INT8U     data[1024];                         //转发数据指针
  
  DATE_TIME outTime;                            //超时绝对时间
  DATE_TIME nextBytesTime;                      //收到字节后的等待时间
  BOOL      receivedBytes;                      //已收到回复数据
  INT8U     dataFrom;                           //数据来自何方(1-主站命令点抄/转发,2-按键单测量点点抄,3-按键全体测量点点抄,4-按键抄新增电能表)
  BOOL      ifSend;                             //点抄(/转发)命令是否发送
	INT8U     forwardResult;                      //转发结果
}FORWARD_DATA;

//结构 - 抄表控制链表
struct copyCtrlWord
{
   DATE_TIME         currentCopyTime;           //当前抄表时间
   DATE_TIME         lastCopyTime;              //上次抄表时间
   DATE_TIME         copyTimeOut;               //抄表超时时间
   DATE_TIME         nextCopyTime;              //下一次抄表时间
   BOOL              meterCopying;              //抄表中...
   BOOL              copyContinue;              //继续抄表
   BOOL              flagOfRetry;               //重试
   INT8U             copyDataType;              //抄表数据类型(当前数据/上月数据)
   INT8U             retry;                     //重新发起抄读某项数据抄读
   INT8U             numOfMeterAbort;           //电表485接口/载波接口抄表失败项数?
   INT8U             ifRealBalance;             //是否进行日结算
   INT8U             ifBackupDayBalance;        //是否进行备份日结算
   INT8U             ifCopyLastFreeze;          //是否采集上一次冻结数据
                                                //1.485表/多功能电能表(含载波多功能表) 0-当前数据 1-上月数据 2-上一次日冻结数据
                                                //2.载波表 0-当前数据 2-上一次日冻结数据 3-整点冻结数据
   INT16U            backMp;                    //载波端口暂停抄表的测量点号 ly,2010-11-4,增加
   INT8U             dataBuff[1536];            //数据缓存
   FORWARD_DATA      *pForwardData;             //数据转发指针
   INT8U             backupCtrlWord;            //备份端口控制字,防止在端口转发改变了端口属性后不能通信

   INT8U             ifCopyDayFreeze;           //是否已进行了抄表日冻结
   DATE_TIME         copyDayTimeReset;          //抄表日冻结标志复位时间

   INT8U             ifCheckTime;               //是否已进行了电表校时,2013-11-22,add
   DATE_TIME         checkTimeReset;            //电表校时标志复位时间,2013-11-22,add
   
   BOOL              cmdPause;                  //命令指定本端口暂停抄表
   INT8U             round;                     //载波抄表第几轮
   INT8U             protocol;                  //当前抄的表的协议
   INT8U             ifCollector;               //是否有采集器地址
   INT8U             hasSuccessItem;            //是否有采集成功项
   BOOL              thisMinuteProcess;         //本分钟处理判断?
	
	 INT8U             needCopyMeter[6];          //路由请求抄读的表(/采集器)地址,ly,2012-01-09从载波标志集改到copyCtrl中
   INT8U             destAddr[6];               //载波目的节点地址,ly,2012-01-10
	
	#ifdef LIGHTING
	 BOOL              thisRoundFailure;          //本轮抄表485口是否异常,2016-11-25
	 INT8U             portFailureRounds;         //485端口异常轮数,2016-11-25
	#endif

   struct cpAddrLink *cpLinkHead;               //抄表地址链表头
   struct cpAddrLink *tmpCpLink;                //抄表地址链临时指针变量
};

//载波标志集
typedef struct
{
	unsigned int currentWork   :3;                //载波模块当前工作(抄表/学习路由/主动上报表号等)
	unsigned int sending       :1;                //正在发送命令
	unsigned int cmdContinue   :1;                //可以继续发送命令
	unsigned int retryCmd      :1;                //重试上一命令

	unsigned int hardwareReest :2;                //硬件复位命令
	unsigned int mainNode      :1;                //查询主节点地址?
	unsigned int setMainNode   :2;                //设置主节点地址
	unsigned int startStopWork :2;                //启动时先停止工作
	unsigned int querySlaveNum :1;                //查询从节点数量
	unsigned int synSlaveNode  :2;                //是否有表需要同步到载波模块?
	unsigned int querySlaveNode:2;                //查询载波从节点信息
	unsigned int routeRunStatus:1;                //查询路由运行状态
	unsigned int studyRouting  :3;                //学习路由状态
	unsigned int setupNetwork  :3;                //组网状态
	unsigned int searchMeter   :3;                //搜表
	unsigned int init          :2;                //初始化
	unsigned int hasAddedMeter :1;                //有添加表地址
	unsigned int forceClearData:1;                //强制清除数据
	unsigned int ifSearched    :1;                //是否搜过表?
	unsigned int msSetAddr     :1;                //主站设置表档案(ly,2011-01-29,add)
	unsigned int checkAddrClear:1;                //比较表档案后清除数据(ly,2011-01-30,add)
		
	unsigned int reStartPause  :2;                //抄表中需要发送抄表重启暂停恢复命令(ly,2012-01-16,add)
		                                            // 1 - 重启
		                                            // 2 - 暂停
		                                            // 3 - 恢复
	
	unsigned int workStatus    :4;                //本地通信模块工作状态 ly,2011-07-06,add
		                                            //  0 - 设置为抄表模式
		                                            //  1 - 模块确认抄表模式
		                                            //  2 - 然后重启抄表
		                                            //  3 - 模块确认重启抄表
		                                            //  4 - 暂停模块抄表
		                                            //  5 - 模块确认暂停抄表
		                                            //  6 - 模块确认恢复抄表
		                                            //  7 - 出抄表时段而暂停抄表
		                                            //  8 - 报警控制点的末端单灯控制器最近通信时间超过监测时限而暂停抄表
		                                            //  9 - 单灯控制器超过离线阈值时限而暂停抄表
		                                            // 10 - 单灯控制器刚上电时召测状态而暂停抄表
		                                            
  unsigned int routeLeadCopy :1;                //路由主导抄表(1-是路由在主导抄表, 0-集中器主导抄表)
 
  unsigned int chkNodeBeforeCopy :2;            //鼎信要求抄表前比对节点档案,2013-12-30,add
   	                                            //  鉴于此,对所有路由主导抄读的模块都作此处理
  
  
  /***************各厂家扩展需要的命令********************/
	unsigned int innerDebug    :1;                //内部调试(SR)?
	unsigned int wlNetOk       :3;                //无线扩展 - 查询是否组网完成
	unsigned int paraClear     :2;                //SR扩展 - 清除参数
	INT16U       lastNumInNet;                    //SR扩展 - 上一次查询入网节点数量
	unsigned int setDateTime   :1;                //友迅达扩展 - 设置时间
  unsigned int readStatus    :2;                //友迅达扩展 - 读取状态

  unsigned int batchCopy     :3;                //电科院 - 轮抄状态 ly,2012-02-29,为适应新版电科院模块而添加
  	                                            // 0 - 未开始轮次
  	                                            // 1 - 重启轮抄
  	                                            // 2 - 模块确认重启轮抄命令

  unsigned int setPanId      :1;                //设置信道信道号
  	

  /***************扩展命令         END********************/

  INT8U        mainNodeAddr[6];                 //载波主节点地址
  INT8U        destAddr[6];                     //载波目的节点地址
  INT16U       synMeterNo;                      //同步载波从节点序号
  INT16U       activeMeterNo;                   //主动注册的载波从节点查询序号
  INT16U       numOfSalveNode;                  //从节点数量
	
	DATE_TIME    cmdTimeOut;                      //命令发送超时
  DATE_TIME    operateWaitTime;                 //操作等待时间
  DATE_TIME    foundStudyTime;                  //搜索/学习时间
  
  DATE_TIME    msSetWait;                       //主站设置表档案同步到本地通信模块等待时间(ly,2011-01-29,add)
  
	DATE_TIME    delayTime;                       //RL模块操作等待
	DATE_TIME    netOkTimeOut;                    //RL模块组网超时时间
	
	INT16U       numOf253;                        //否认状态字出现253的次数 ly,2011-05-20,为适应弥亚微
	INT16U       numOfCarrierSending;             //载波端口发送计数        ly,2011-05-27,为适应弥亚微
	
	INT8U        cmdType;                         //发送命令类型
	INT8U        productInfo[8];                  //厂商代码和版本信息
	
	DATE_TIME    routeRequestOutTime;             //路由请求超时时间
	
 #ifdef LIGHTING
   
  INT8U        broadCast;                       //广播
	DATE_TIME    broadCastWaitTime;               //广播命令等待时间
	
	INT8U        searchLddt;                      //搜索报警控制点的末端单灯控制器
	DATE_TIME    searchLddtTime;                  //搜索报警控制点的末端单灯控制器间隔时间
  struct cpAddrLink *searchLdgmNode;            //搜索报警控制点节点
  
	INT8U        searchOffLine;                   //搜索离线的单灯控制器,2015-11-09,Add
	DATE_TIME    searchOffLineTime;               //搜索离线的单灯控制器间隔时间,2015-11-09,Add

	INT8U        searchLddStatus;                 //搜索单灯控制器状态,2015-11-12,Add
	DATE_TIME    searchLddStatusTime;             //搜索单灯控制器状态间隔时间,2015-11-12,Add

 #endif

}CARRIER_FLAG_SET;

//结构 - 点抄
typedef struct
{
  INT8U     port;                               //转发通信端口号
  INT8U     dataFrom;                           //数据来自何方(1-主站命令点抄/转发,2-按键单测量点点抄,3-按键全体测量点点抄,4-按键抄新增电能表)

  INT8U     data[255];                          //转发数据指针
  
  INT8U     dotRetry;                           //点抄重试次数
  INT8U     dotTotalItem;                       //点抄应抄项数
  INT8U     dotRecvItem;                        //点抄接收项数
  
	INT16U    dotCopyMp;                          //点抄测量点
  INT8U     addr[6];                            //点抄地址
  INT8U     protocol;                           //测量点协议
  DATE_TIME outTime;                            //超时绝对时间
	BOOL      dotCopying;                         //正在点抄
	INT8U     dotResult;                          //点抄结果
}DOT_COPY;

//点抄F129结构
struct dotFn129
{
	INT16U pn;                                    //测量点
	INT8U  from;                                  //数据来源
	INT8U  ifProcessing;                          //是否正在处理
	
	struct dotFn129 *next;
};

#define METER_RECV_BUF_SIZE        212          //电能表数据接收缓冲大小
#define REPLENISH_MAX_RETRY          2          //补数最大重发次数

//抄表端口
#ifdef PLUG_IN_CARRIER_MODULE
 #define NUM_OF_COPY_METER           5          //抄表端口数量(集中器有5个端口抄表)
#else
 #define NUM_OF_COPY_METER           3          //抄表端口数量(专变III型终端只有3个端口抄表)
#endif
#define PORT_NO_1                    1          //485端口号1
#define PORT_NO_2                    2          //485端口号2
#define PORT_NO_3                    3          //485端口号3
#define PORT_NO_4                    4          //485端口号4
#define PORT_POWER_CARRIER          31          //电力载波接口

//点抄类型
#define DOT_CP_SINGLE_MP           0x5          //按键单测量点点抄
#define DOT_CP_ALL_MP              0x6          //按键全体测量点点抄
#define DOT_CP_NEW_METER           0x7          //按键点抄发现的新表
#define DOT_CP_IR                  0x8          //按键学习IR

//点抄/转发结果
#define RESULT_NONE                0x0          //还没有点抄结果
#define RESULT_HAS_DATA            0x1          //点抄/转发有数据回复
#define RESULT_NO_REPLY            0x2          //点抄/转发表端无应答

#ifdef JZQ_CTRL_BOARD_V_1_4 
 #define NUM_COPY_RETRY              3          //抄表无应答重发次数(用xMega转发485抄表,2路同时发送时要比
                                                //  CPU本身的串口2路同时发送延时要大,因此减小重发次数)
#else
 #define NUM_COPY_RETRY              3          //抄表无应答重发次数
#endif

#define CARRIER_TIME_OUT            10          //载波节点发送命令超时时限(10s)
                                                //2010-12-28,由于弥亚微建议发送命令的超时时间为10s,因此从5s改成10s

#define ROUTE_REQUEST_OUT           10          //路由请求超时时间(单位:分,便于过一段时间重启抄表任务)
                                                //2012-08-28,改成由10分钟改成20分钟
                                                //2014-01-02,改成由20分钟改成10分钟,鼎信要求改为10分钟

#define MONITOR_WAIT_ROUTE         100          //监控载波从节点最长等待时间(s),2013-12-27
                                                //鼎信路由要求等待至少90秒,为可靠起见,设置为100秒
                                                //至2013-12-27,鼎信是要求等待时间最长的路由

#define WAIT_RECOVERY_COPY          60          //恢复抄表等待时间(s),2013-12-27

//载波/无线端口发送命令类型
#define CA_CMD_NONE                0x0          //无命令
#define CA_CMD_HARD_RESET          0x1          //硬件复位命令
#define CA_CMD_READ_MODULE         0x2          //识别模块命令
#define CA_CMD_SR_PARA_INIT        0x3          //SR模块参数区初始化命令
#define CA_CMD_CLEAR_PARA          0x4          //参数区初始化命令
#define CA_CMD_QUERY_MAIN_NODE     0x5          //查询主节点地址命令
#define CA_CMD_SET_MAIN_NODE       0x6          //设置主节点地址命令
#define CA_CMD_SR_CHECK_NET        0x7          //SR模块设置组网验证命令
#define CA_CMD_RESTART_WORK        0x8          //重启命令
#define CA_CMD_PAUSE_WORK          0x9          //暂停当前工作命令
#define CA_CMD_RESTORE_WORK        0xa          //恢复暂停的工作命令
#define CA_CMD_QUERY_ROUTE_STATUS  0xb          //查询路由状态命令
#define CA_CMD_QUERY_SLAVE_NUM     0xc          //查询从节点数量命令
#define CA_CMD_QUERY_SLAVE_NODE    0xd          //查询从节点命令
#define CA_CMD_ADD_SLAVE_NODE      0xe          //添加从节点命令
#define CA_CMD_READ_NO_NET_NODE    0xf          //读未入网节点命令
#define CA_CMD_SET_WORK_MODE      0x10          //设置工作模式命令
#define CA_CMD_ACTIVE_REGISTER    0x11          //激活载波从节点主动注册命令
#define CA_CMD_PERMIT_ACTIVE      0x12          //允许载波从节点主动上报命令
#define CA_CMD_SET_MODULE_TIME    0x13          //设置模块时间命令
#define CA_CMD_UPDATE_ADDR        0x14          //友迅达启动档案更新命令
#define CA_CMD_BATCH_COPY         0x15          //电科院模块轮抄命令
#define CA_CMD_BROAD_CAST         0x16          //广播命令

#define SRWF_NET_OK_TIME_OUT        30          //SRWF_3E68等待网络Ok时间为30分钟

//外部变量
#ifdef PLUG_IN_CARRIER_MODULE
 extern struct copyCtrlWord copyCtrl[NUM_OF_COPY_METER+2];//抄表控制字
 extern CARRIER_FLAG_SET    carrierFlagSet;     //载波标志集
#else
 extern struct copyCtrlWord copyCtrl[NUM_OF_COPY_METER];  //抄表控制字
#endif
extern DOT_COPY            *pDotCopy;           //点抄指针

extern DATE_TIME           lastCopyTime;        //上次抄表时间(Q/GDW129-2005)
extern DATE_TIME           nextCopyTime;        //下一次抄表时间
extern INT16U              countOfCopyMeter;    //抄表时间计数

extern struct dotFn129     *pDotFn129;
extern INT8U  dotReplyStart;

//函数声明
//函.1 公共函数
void   copyMeter(void *arg);
void   detectSwitchPulse(void *arg);
void  *threadOf485x1Received(void *arg);
void  *threadOf485x2Received(void *arg);
void   sendMeterFrame(INT8U port,INT8U *pack,INT16U length);
void covertAcSample(INT8U *buffer, INT8U *visionBuf, INT8U *reqBuf, INT8U type, DATE_TIME covertTime);
INT32U times2(INT8U times);
INT32U hexDivision(INT32U dividend, INT32U divisor, INT8U deciBit);

void   reSetCopyTime(void);
BOOL   forwardData(INT8U portNum);
void   forwardDataReply(INT8U portNum);
void   queryMeterStoreInfo(INT16U pn, INT8U *meterInfo);
INT16U findAcPn(void);

//函.2 Q/GDW376.1终端
struct cpAddrLink * initPortMeterLink(INT8U port);
INT8U  whichItem(INT8U port);
void   searchMpStatis(DATE_TIME searchTime,void *record,INT16U mp,INT8U type);
BOOL   copy485Failure(INT16U mp,INT8U startOrStop,INT8U port);
BOOL   checkLastDayData(INT16U pn, INT8U type, DATE_TIME queryTime, INT8U queryType);
BOOL   checkLastMonthData(INT16U pn);

//载波相关
#ifdef PLUG_IN_CARRIER_MODULE
 extern INT8U carrierModuleType;                  //载波模块类型

 void *threadOfCarrierReceive(void *arg);
 void resetCarrierFlag(void);
 void sendCarrierFrame(INT8U port, INT8U *pack, INT16U length);
 void dotCopyReply(INT8U hasData);
 BOOL dotCopy(INT8U port);
 void checkCarrierMeter(void);
 INT8U checkHourFreezeData(INT16U pn, INT8U *lostBuff);
 BOOL stopCarrierNowWork(void);
 void foundMeterEvent(struct carrierMeterInfo *foundMeter);
#endif 

#ifdef WDOG_USE_X_MEGA
 void xMegaCopyData(INT8U port, INT8U *buf, INT16U len);
#endif

#ifdef PULSE_GATHER
 void fillPulseVar(INT8U type);
 void covertPulseData(INT8U port, INT8U *visionBuf,INT8U *needBuf,INT8U *paraBuff);
#endif

#ifdef SUPPORT_ETH_COPY
 extern INT8U     setEthMeter;      //主站设置以太网抄表地址
 extern DATE_TIME msSetWaitTime;    //等待设置表地址完
 extern INT16U    monitorMp;        //需要监视原始数据的测试点号(由于以太网抄表表太多,如果需要一个一个看时用)

 void *threadOfEthCopyServer(void *arg);
 void *threadOfEthCopyDataRead(void *arg);
 void *threadOfEthCopyMeter(void *arg);
#endif

#ifdef LIGHTING
 
 //SLC查询数据标志
 #define REQUEST_STATUS          0x00001   //请求查询当前状态
 #define REQUEST_FREEZE_TIMES    0x00002   //请求查询开关调数据
 #define REQUEST_HOUR_FREEZE     0x00004   //请求查询小时冻结数据
 #define REQUEST_CHECK_TIME      0x00008   //请求校时
 #define REQUEST_SYN_ADJ_PARA    0x00010   //请求同步调光参数
 #define REQUEST_SYN_CTRL_TIME_1 0x00020   //请求同步时段第一小包
 #define REQUEST_SYN_CTRL_TIME_2 0x00040   //请求同步时段第二小包
 #define REQUEST_SYN_CTRL_TIME_3 0x00080   //请求同步时段第三小包,单灯控制器因为载波信道需要发第三小包,线路控制点RS485信道不需要
 #define REQUEST_CTRL_TIME_1     0x00100   //请求查询控制时段第一小包
 #define REQUEST_CTRL_TIME_2     0x00200   //请求查询控制时段第二小包
 #define REQUEST_CTRL_TIME_3     0x00400   //请求查询控制时段第三小包
 #define REQUEST_CTRL_TIME_4     0x00800   //请求查询控制时段第四小包
 #define REQUEST_CTRL_TIME_5     0x01000   //请求查询控制时段第五小包
 #define REQUEST_CTRL_TIME_6     0x02000   //请求查询控制时段第六小包
 #define REQUEST_CTRL_TIME_7     0x04000   //请求查询控制时段第七小包
 #define REQUEST_CLOSE_GATE      0x08000   //请求合闸
 #define REQUEST_OPEN_GATE       0x10000   //请求分闸
 #define REQUEST_SOFT_VER        0x20000   //请求查询软件版本

 
 //LDGM相关

 //#define L_CTRL_WAVE_LUX             25    //光控用于判断的波动值
 //#define L_CTRL_WAVE_LUX             10    //光控用于判断的波动值,2016-03-11,根据张家港要求改成10
 //#define L_CTRL_WAVE_LUX             20    //光控用于判断的波动值,2016-03-17,修改为改成20
 //#define L_CTRL_WAVE_LUX             17    //光控用于判断的波动值,2016-06-02,修改为改成17
                                           //2016-05-31,张家港测试发现由于光控集中器一会儿离线一会儿上线造成时段控结合光控的线路控制点
                                           //           合闸晚了1个小时(时段是18:55合闸,光控开灯前后60分钟有效,实际19:55合闸),
                                           //           分闸早了1个小时(时段半夜灯23:00合闸,光控在关灯前后60分钟有效,实际22:00分闸;全夜灯4:57分闸,实际3:57分闸)
                                           //经分析,发现原因:张家港设置的是20Lux-0%,原来的震荡值设置的20,那么在光控集中器离线前的注明值为18:47(未进入光控范围)的流明值为326Lux,
                                           //       而后光控集中器离线，在19:12光控集中器上线可此时流明值已为0Lux了,
                                           //       a.根据判断规则三次流明值小于20+20=40Lux且一次比一次流明值小这个过程正好发生离线这段时间,
                                           //       从而得不到光控要求合闸的要求
                                           //       b.另一个判断条件tmpLux>(nowLux+L_CTRL_WAVE_LUX)(即当前流明值小于(关灯注明值-震荡值))也不满足,因为20Lux-20震荡值=0,
                                           //       要当前值0小于0不可能成立,
                                           //       因此修改为震荡值17,当发生这种情况时可避免出问题
 //#define L_CTRL_WAVE_LUX             29    //光控用于判断的波动值,2016-06-30,从17修改为改成29
                                           //  应张家港要求修改,因为他发现开灯晚了点，所以我想把20往上调一下，只能调30。改后范围变成1-59.
 //2016-08-24,光控照度震荡值改成参数,方便修改,取消宏(L_CTRL_WAVE_LUX)定义
 
 struct lightCtrl
 {
   INT8U bright;                           //亮度
   INT8U lcDetectBright;                   //光控检测后灯应该执行的亮度
 };
 
 extern struct ctrlTimes  *cTimesHead;
 extern struct cpAddrLink *xlcLink;
 extern struct cpAddrLink *ldgmLink;
 extern struct cpAddrLink *lsLink;
 extern struct cpAddrLink *acLink;         //2015-12-05,add
 extern struct cpAddrLink *thLink;		     //2018-05-11,add

 extern struct lightCtrl  lCtrl;           //光控标志集

 INT8U initXlcLink(void);
 INT8U initLdgmLink(void);
 INT8U initLsLink(void);
 INT8U initAcLink(void);
 
 void lcProcess(INT8U *lastLux, INT32U nowLux);

#endif

#endif   //__copyMeterH
