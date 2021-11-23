/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
文件名:gdw376-2.h
作者：leiyong
版本：0.9
完成日期:2010年03月10日
描述：Q/GDW376-2集中器与下行通信模块(载波模块)
      本地接口通信协议动态链接库头文件(用户接口)
      
修改历史：
  01,10-03-09,leiyong created.
**************************************************/

#ifndef __GDW_376_2_H
#define __GDW_376_2_H

#include "common.h"

//载波模块类型
#define NO_CARRIER_MODULE          0x0     //无载波模块 
#define EAST_SOFT_CARRIER          0x1     //东软载波模块
#define CEPRI_CARRIER              0x2     //福星晓程模块
#define HWWD_WIRELESS              0x3     //华伟沃电无线模块
#define SR_WIRELESS                0x4     //SR无线模块
#define RL_WIRELESS                0x5     //锐拔无线模块
#define MIA_CARRIER                0x6     //弥亚微载波模块
#define TC_CARRIER                 0x7     //鼎信载波模块
#define LME_CARRIER                0x8     //力合微载波模块
#define FC_WIRELESS                0x9     //友迅达无线模块
#define SC_WIRELESS                0xa     //赛康无线模块
#define CEPRI_CARRIER_3_CHIP       0xb     //电科院模块(新版本3芯片)
#define SR_WF_3E68                 0xc     //桑锐SRWF-3E68模块

//GDW376.2帧特殊字符
#define GDW376_2_SOP              0x68     //起始字符
#define GDW376_2_EOP              0x16     //结束字符

//通信方式
#define CENTRE_ROUTE                 1     //集中式路由载波通信
#define DISTRIBUTED_ROUTE            2     //分布式路由载波通信
#define EAST_SOFT_ROUTER             7     //东软扩展路由协议
#define MICRO_POWER_WIRELESS        10     //微功率无线通信

//应用层功能代码
#define ACK_OR_NACK_3762          0x00     //确认/否认
#define INITIALIZE_3762           0x01     //初始化
#define DATA_FORWARD_3762         0x02     //数据转发
#define QUERY_DATA_3762           0x03     //查询数据
#define LINK_DETECT_3762          0x04     //链路接口检测
#define CTRL_CMD_3762             0x05     //控制命令
#define ACTIVE_REPORT_3762        0x06     //主动上报
#define ROUTE_QUERY_3762          0x10     //路由查询
#define ROUTE_SET_3762            0x11     //路由设置
#define ROUTE_CTRL_3762           0x12     //路由控制
#define ROUTE_DATA_FORWARD_3762   0x13     //路由数据转发
#define ROUTE_DATA_READ_3762      0x14     //路由数据抄读
#define RL_EXTEND_3762            0x15     //锐拔/SR扩展命令
#define DEBUG_3762                0xf0     //内部调试
#define FC_QUERY_DATA_3762        0x07     //友迅达扩展查询数据
#define FC_NET_CMD_3762           0x09     //友迅达扩展网络指令

//东软扩展AFN
#define ES_CTRL_CMD_3762          0x01     //东软扩展控制命令
#define ES_DATA_QUERY_3762        0x02     //东软扩展数据查询

//返回值
#define RECV_DATA_CORRECT            1     //接收到正确的一帧
#define RECV_DATA_UNKNOWN           -1     //接收到不认识的数据
#define RECV_NOT_COMPLETE_3762      -2     //接收数据不完整,继续等待接收
#define RECV_CHECKSUM_ERROR_3762    -3     //接收数据校验和错误
#define RECV_TAIL_ERROR_3762        -4     //接收数据帧尾错误,丢弃

//结构 - GDW376.2接收帧分析
typedef struct
{
	INT8U  sop;        //起始字符SOP
	INT16U l;          //长度L
	INT8U  c;          //控制域C
	INT8U  *pUserData; //用户数据指针
	INT8U  r[6];       //信息域R
  INT8U  a[12];      //地址域A
  INT8U  *afn;       //应用层功能码AFN
  INT8U  *fn;        //功能代码	
  INT8U  dt[2];      //数据单元标识
  INT8U  *pLoadData; //净数据起始指针
	INT8U  cs;         //校验和CS
	INT8U  eop;        //结束字符EOP
}GDW376_2_FRAME_ANALYSE;

//初始化结构
typedef struct
{	 
	 INT8U *afn;                                        //AFN指针
	 INT8U *fn;                                         //FN指针
	 INT8U *moduleType;                                 //模块类型指针
	 void (*send)(INT8U port,INT8U *pack,INT16U length);//向端口发送数据函数
}GDW376_2_INIT;

//函数返回值

//函数声明
BOOL  initGdw3762So(GDW376_2_INIT *init);
INT8U gdw3762Framing(INT8U afn,INT8U fn,INT8U *address,INT8U *pData);
INT8S gdw3762Receiving(INT8U *data,INT8U *recvLen);

#endif  //__GDW_376_2_H
