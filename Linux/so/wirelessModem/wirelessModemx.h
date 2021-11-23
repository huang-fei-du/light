/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
文件名：wirelessModemx.h
作者：leiyong
版本：0.9
完成日期:2010年01月24日
描述：无线Modem动态链接库私有头文件
      定义了所需的么有结构及宏
      
修改历史：
  01,10-01-24,leiyong created.
**************************************************/

#ifndef __wirelessModemxH
#define __wirelessModemxH

#include "common.h"

//#define SIZE_OF_WIRELESS        128      //无线接收缓冲大小
#define SIZE_OF_WIRELESS       2048      //无线接收缓冲大小
#define SIZE_OF_RECV_MS_BUF    2048      //接收主站的帧大小
#define DATA_FROM_GPRS          0x2      //主站数据来自GPRS接口

/***********************************************************
*AT命令类型
***************************/
#define AT_NONE                 0x0      //不是AT命令

//GPRS & CDMA共用
#define AT_ECHO                 0x1      //回显
#define AT_SIGNAL               0x2      //信号
#define AT_DAILING              0x3      //拨号
#define AT_REG                  0x4      //注册入网络?
#define AT_COPS                 0x5      //查询运营商
#define AT_IP_START            0x10      //IP start

//GPRS共用
#define AT_SIM                  0x5      //SIM插入
#define AT_DCD                  0x6      //DCD设置
#define AT_CGDCONT              0x7      //配置CGDCONT参数
#define AT_TO_AT                0x8      //转换GPRS数据模式至AT命令模式
#define AT_TO_GPRS              0x9      //转换AT模式至GPRS模式
#define AT_RE_SIGNAL            0xa      //动态检测信号
#define AT_IP_MODE              0xb      //IP模式
#define AT_TRANSPARENT          0xc      //透传参数
#define AT_APN                  0xd      //设置APN
#define AT_CIICR                0xe      //激活移动场景
#define AT_CIFSR                0xf      //获取IP地址
#define AT_LC_TCP_PORT         0x61      //设置本地TCP端口
#define AT_CGATT               0x62      //附着或分离GPRS
#define AT_CGREG               0x63      //GPRS注册入网络

//CDMA共用
#define AT_IPR                 0x11      //串口速率设置
#define AT_UIM                 0x12      //UIM卡是否插入
#define AT_TIME                0x13      //网络时间
#define AT_CRM                 0x14      //串口通信协议
#define AT_DIP                 0x15      //设置目的IP地址
#define AT_DPORT               0x16      //设置目的端口
#define AT_SPC                 0x17      //输入SPC号
#define AT_AINF                0x18      //设置内置 TCPIP 协议鉴权信息

//GR64专用
#define AT_GR64_IPS            0x20      //IPS(设置IP参数)
#define AT_GR64_IPA            0x21      //IPA
#define AT_GR64_ADDR           0x22      //获得PDP IP地址

//MC52i(MC5x)专用
#define AT_MC5X_SET_CREG       0x31      //注册网络提示
#define AT_MC5X_QUERY_CREG     0x32      //查询注册网络情况
#define AT_MC5X_SET_CGREG      0x33      //GPRS注册网络提示
#define AT_MC5X_QUERY_CGREG    0x34      //查询GPRS注册网络情况
#define AT_MC5X_SELECTT_CONN   0x35      //选择连接类型(Select connection type GPRS0)
#define AT_MC5X_SOCKET         0x36      //设置服务类型为Socket
#define AT_MC5X_CON_ID         0x37      //设置连接ID
#define AT_MC5X_IP_PORT        0x38      //设置IP地址及端口
#define AT_MC5X_REQUEST_SEND   0x39      //请求发送

//m590e专用
#define AT_M590E_XISP          0x41      //设置内部协议栈
#define AT_M590E_SET_XIIC      0x42      //进行PPP连接
#define AT_M590E_QUERY_XIIC    0x43      //查询PPP连接
#define AT_M590E_IPSTATUS      0x44      //查询链路状态
#define AT_M590E_TCPCLOSE      0x45      //关闭链路的TCP连接

//MC180专用
#define AT_CM180_PNUM          0x51      //设置数据业务号码
#define AT_CM180_GETIP         0x52      //查询模块IP地址
#define AT_CM180_PPPSTATUS     0x53      //查询当前PPP连接状态

//M72D专用
#define AT_QIFGCNT             0x71      //上下文
#define AT_QIAPN               0x72      //APN
#define AT_QIMUX               0x73      //IP MUX
#define AT_QIMODE              0x74      //IP Mode
#define AT_QITCFG              0x75      //
#define AT_QIDNSIP             0x76      //
#define AT_QIREGAPP            0x77      //
#define AT_QIACT               0x78      //
#define AT_QILOCIP             0x79      //


/************************************************************/

#define DAIL_TIMEOUT             40      //拨号超时?

#define HEART_WAIT_TIME          60      //心跳确认等待时长(s)

//对于AT命令有两段回复的处理:第一段回复收到后置一个标志,待第二段(一般是OK)收到后置为成功
#define AT_M_RESPONSE_NONE       0x0     //多段回复-无效
#define AT_M_RESPONSE_INFO_ACK   0x1     //多段回复-信息段确认
#define AT_M_RESPONSE_INFO_NACK  0x2     //多段回复-信息段否认
#define AT_M_RESPONSE_OK         0x3     //多段回复-命令得到确认

//无线接口标志集
typedef struct
{
	 unsigned int startup       :1;        //上电开机回码标志
	 unsigned int atFrame       :3;        //AT帧标志
	 unsigned int dialConnected :1;        //无线模块[GPRS/CDMA/etc]拨号成功(连接)标志
   unsigned int ipr           :1;        //串口速率设置成功?

   unsigned int callReady     :1;        //无请求应答(通话准备好)
   unsigned int echo          :1;        //回显关闭
   unsigned int cardInserted  :2;        //是否插卡[有两段回复,因此用2位]
   unsigned int dcd           :1;        //DCD设置标志
   unsigned int cgdcont       :1;        //配置CGDCONT参数成功与否标志
   unsigned int signal        :2;        //读取信号质量
   unsigned int wlLcTcpPort   :1;        //设置本地TCP端口    ly,2011-12-28,add
   unsigned int creg          :2;        //注册入网络?        ly,2012-02-09,add
   unsigned int cgreg         :2;        //GPRS注册入网络?    ly,2012-05-18,add
   unsigned int cops          :2;        //运营商查询,    2018-12-27,add

   unsigned int ipMode        :1;        //Select TCPIP Application mode
   unsigned int ipStarted     :1;        //START UP TCP OR UDP CONNECTION
   unsigned int wlSendStatus  :3;        //向无线MODEM发送数据状态  	
   

   //SIM300C特有标志位----start
   unsigned int simTransparent:1;        //透传参数
   unsigned int simApn        :1;        //APN
   unsigned int simCiicr      :7;        //激活移动场景
   unsigned int simCifsr      :1;        //获取本地IP地址
   unsigned int simCgatt      :2;        //附着和分离GPRS
   //SIM300C特有标志位----end

   //GR64特有标志位----start
   unsigned int gr64Ips       :1;        //GR64设置IP参数IPS
   unsigned int gr64Ipa       :1;        //GR64 IPA
   unsigned int numOfGr64Ipa  :2;        //
   unsigned int gr64Addr      :1;        //获取PDP IP地址?
   	
   //GR64特有标志位----end
   
   //DTGS-800特有标志位----start
   unsigned int dtgsRegNet    :2;        //DTGS-800注册网络
   unsigned int dtgsTime      :1;        //DTGS-800时间已读取?
   unsigned int dtgsCrm       :1;        //DTGS-800设置串口通讯协议
   unsigned int dtgsDip       :1;        //DTGS-800设置目的IP地址
   unsigned int dtgsDPort     :1;        //DTGS-800设置目的端口
   unsigned int dtgsDial      :1;        //DTGS-800拨号指令是否发送?
   unsigned int dtgsSpc       :1;        //DTGS-900输入SPC号
   unsigned int dtgsAinf      :1;        //DTGS-800设置内置TCP/IP协议鉴权信息
   unsigned int ainfInputed   :1;        //DTGS-800鉴权信息已输入
   //unsigned int dtgsCta       :1;      //DTGS-800内置
   //DTGS-800特有标志位----end

   //MC52i(MC5x)特有标志位----start
   unsigned int mc5xSetCreg   :1;        //MC5x网络注册提示
   unsigned int mc5xQueryCreg :2;        //MC5x检查网络注册
   unsigned int mc5xSetCGreg  :1;        //MC5x网络GPRS注册提示
   unsigned int mc5xQueryCGreg:2;        //MC5x检查GPRS网络注册
   unsigned int mc5xSelectConn:1;        //MC5x选择连接类型
   unsigned int mc5xSocket    :1;        //MC5x设置服务类型为Socket
   unsigned int mc5xConId     :1;        //MC5x设置连接Id
   unsigned int mc5xIpPort    :1;        //MC5x设置IP地址及端口
   unsigned int mc5xDial      :1;        //MC5x连接命令已发出
   unsigned int mc5xLastSended:1;        //MC5x上一帧已发送
   unsigned int mc5xRecvStatus:3;        //MC5x接收状态
   	
   //MC52i特有标志位----end
   
   //M590E特有标志位----start
   unsigned int m590eQueryCreg :2;             //查询模块是否注册上GSM网络
   unsigned int m590eXisp      :1;             //设置内部协议栈
   unsigned int m590eSetXiic   :1;             //进行PPP连接
   unsigned int m590eQueryXiic :2;             //查询PPP连接
   unsigned int numOfConnect   :2;             //TCP连接次数
   //M590E特有标志位----end

   //CM180特有标志位----start
   unsigned int cm180pnum      :1;             //设置数据业务号码
   unsigned int cm180plogin    :1;             //设置数据业务用户名和密码
   unsigned int cm180QueryCreg :2;             //查询模块是否注册上网络
   unsigned int cm180Getip     :2;             //查询模块IP地址
   unsigned int cm180Dial      :1;             //拨号指令是否发送
   unsigned int	cm180pppStatus :2;             //查询PPP连接状态
   unsigned int cm180ipStatus  :2;             //查询TCP连接状态
   //CM180特有标志位----end 	

   //M72-D特有标志位----start
   unsigned int m72qifgcnt     :1;             //上下文
   unsigned int m72Apn         :1;             //APN
   unsigned int m72mux         :1;             //MUX
   unsigned int m72IpMode      :1;             //IP Mode
   unsigned int m72Itcfg       :1;             //
   unsigned int m72dnsip       :1;             //
   unsigned int m72regapp      :1;             //
   unsigned int m72act         :1;             //
   unsigned int m72locip       :1;
   	
   //M72-D特有标志位----end
   
}WL_FLAG_SET;

char * intToString(INT32U in,int type,char *returnStr);
char * trim(char * s);
char * loginIpAddr(void);
char * loginPort(void);
void   closeLinkReport(void);
void   singleReport(INT8U rssi);
INT8U  atResponse(INT8U *pFrame,INT8U len);

#endif  //__wirelessModemxH
