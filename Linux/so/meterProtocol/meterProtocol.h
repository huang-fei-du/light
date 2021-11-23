/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：meterProtocol.h
作者:leiyong
版本：0.9
完成日期:2009年1月10日
描述：电能表协议用户接口头文件
修改历史：
  01,09-01-07,Leiyong created.
**************************************************/

#ifndef __meterProtocolH
#define __meterProtocolH

#include "common.h"
#include "timeUser.h"

//1.抄表前输入的抄表参数
struct meterCopyPara
{
   INT8U     port;                                       //抄表端口
   INT16U    pn;                                         //测量点号
   INT8U     protocol;                                   //通信协议
   INT8U     meterAddr[6];                               //表地址
   INT8U     copyDataType;                               //抄表数据类型(当前数据或是上月数据)
   DATE_TIME copyTime;                                   //抄表时间
   
   INT8U     *dataBuff;                                  //数据缓存
   
   void (*send)(INT8U port,INT8U *pack,INT16U length);   //向端口发送数据函数
   void (*save)(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len);  //数据保存函数
   
};

//2.协议类型
#define    DLT_645_1997                     1      //DL/T645-1997
#define    AC_SAMPLE                        2      //交采装置
#define    SIMENS_ZD_METER                  4      //兰吉尔(/西门子)ZD表
#define    SIMENS_ZB_METER                  6      //兰吉尔(/西门子)ZB表
#define    ABB_METER                        8      //ABB(方型)表
#define    EDMI_METER                      10      //EDMI(红相)表
#define    WASION_METER                    12      //威胜规约
#define    HONGHUA_METER                   13      //弘华规约
#define    SUPERVISAL_DEVICE               15      //现场监测设备
#define    DLT_645_2007                    30      //DL/T645-2007(三相智能电能表)
#define    NARROW_CARRIER_MODE             31      //"串行接口连接窄带低压载波通信模块"接口协议
#define    SINGLE_PHASE_645_1997          200      //单相485表645-1997协议
#define    SINGLE_PHASE_645_2007          201      //单相智能电表645-2007协议
#define    SINGLE_LOCAL_CHARGE_CTRL_2007  202      //单相本地费控智能表
#define    SINGLE_REMOTE_CHARGE_CTRL_2007 203      //单相远程费控智能表
#define    THREE_2007                     204      //三相相本地费控智能表
#define    THREE_LOCAL_CHARGE_CTRL_2007   205      //三相相本地费控智能表
#define    THREE_REMOTE_CHARGE_CTRL_2007  206      //三相远程费控智能表
#define    KEY_HOUSEHOLD_2007             208      //07规约的重点用户
#define    DOT_COPY_2007                  209      //07点抄规约
#define    DOT_COPY_1997                  210      //97点抄规约
#define    HOUR_FREEZE_2007               211      //07表整点冻结
#define    PN_WORK_NOWORK_1997            212      //645-1997协议只抄正反有无功电能示值
#define    PN_WORK_NOWORK_2007            213      //645-2007协议只抄正反有无功电能示值
#define    SINGLE_PHASE_645_2007_TARIFF   214      //单相智能电表645-2007协议(只抄实时数据总及各费率)
#define    SINGLE_PHASE_645_2007_TOTAL    215      //单相智能电表645-2007协议(只抄实时数据总示值)

#define    LIGHTING_XL                    130      //线路控制器(照明产品)
#define    LIGHTING_DGM                   131      //线缆盗割报警控制器(照明产品)
#define    LIGHTING_LS                    132      //照度传感器(照明产品)
#define    LIGHTING_TH                    133      //温湿度传感器(照明产品)

#define    MODBUS_HY                       50      //山东和远MODBUS电表,2017-5-24
#define    MODBUS_ASS                      51      //无锡爱森思MODBUS电表,2017-5-24
#define    MODBUS_XY_F                     52      //上海贤化MODBUS多功能电表,2017-9-21
#define    MODBUS_XY_UI                    53      //上海贤化MODBUS电压电流组合表,2017-9-21
#define    MODBUS_SWITCH                   54      //MODBUS开关量模块,2017-9-22
#define    MODBUS_XY_M                     55      //上海贤化MODBUS电表模块,2017-10-10
#define    MODBUS_T_H                      56      //MODBUS温湿度模块,2018-03-01
#define    MODBUS_MW_F                     57      //成都明武MODBUS多功能表,2018-03-01
#define    MODBUS_MW_UI                    58      //成都明武MODBUS电压电流表,2018-03-01
#define    MODBUS_JZ_F                     59      //上海居正MODBUS多功能表,2018-03-01
#define    MODBUS_JZ_UI                    60      //上海居正MODBUS电压电流表,2018-03-01
#define    MODBUS_PMC350_F                 61      //深圳中电PMC350三相数字表,2018-05-03
#define    MODBUS_WE6301_F                 62      //威斯顿WE6301多功能表,2018-05-03


//3.标志及返回值----------------------------------------------------------------------------------
//3.1当前数据/上月数据/上一次日冻结数据
#define PRESENT_DATA                      0x0      //抄收当前数据
#define LAST_MONTH_DATA                   0x1      //抄收上月数据(上一结算日数据)
#define LAST_DAY_DATA                     0x2      //抄收上一次日冻结数据
#define HOUR_FREEZE_DATA                  0x3      //抄收整点冻结数据
#define NEW_COPY_ITEM                     0x0      //新抄表项
#define LAST_COPY_ITEM_RETRY              0x2      //上一抄表项重试

//3.2返回值
#define CHANGE_METER_RATE                   7      //更改抄表速率
#define METER_REPLY_ANALYSE_OK              6      //接收数据帧正常且解析正确,已保存进缓存
#define METER_NORMAL_REPLY                  5      //接收数据帧正常且表端正常应答(表端支持该命令)
#define METER_ABERRANT_REPLY                4      //接收数据帧正常,但表端异常应答(可能是表端不支持该命令)
#define COPY_COMPLETE_NOT_SAVE              3      //本表抄完但无有效数据
#define COPY_COMPLETE_SAVE_DATA             2      //本表抄完且有有效数据已保存
#define DATA_SENDED                         1      //数据已发送
#define PROTOCOL_NOT_SUPPORT               -1      //协议不支持
#define RECV_DATA_CHECKSUM_ERROR           -2      //接收数据校验和错误
#define RECV_DATA_NOT_COMPLETE             -3      //接收数据不完整,继续等待接收
#define RECV_DATA_TAIL_ERROR               -4      //接收数据帧尾错误,丢弃
#define RECV_DATA_OFFSET_ERROR             -5      //接收数据保存时偏移地址错误,未保存
#define RECV_ADDR_ERROR                    -6      //接收数据的地址与发送的地址不同

//4.抄表协议用户接口函数
/*******************************************************
函数名称:initCopyProcess
功能描述:初始化抄表过程
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE-初始化抄表成功
        FALSE-初始化抄表失败
*******************************************************/
BOOL initCopyProcess(struct meterCopyPara *cp);


/*******************************************************
函数名称:meterFraming
功能描述:抄表数据组帧发送
调用函数:
被调用函数:
输入参数:INT8U flag,标志 D0,当前数据或是上月数据(上一结算日数据)(D0=0-当前数据,D0=1-上月数据(上一结算的数据))
                         D1,新抄表项或是上一抄表项重试(D1=0-新抄表项,D1=1-上一抄表项重试)   
输出参数:
返回值：=0,已发送
        =1,本表抄完
        -1,
*******************************************************/
INT8S meterFraming(INT8U port,INT8U flag);

/*******************************************************
函数名称:meterReceiving
功能描述:抄表数据帧接收
调用函数:
被调用函数:
输入参数:port,抄表端口
         data,接收数据指针
         recvLen,接收数据长度
输出参数:
返回值:
*******************************************************/
INT8S meterReceiving(INT8U port,INT8U *data,INT16U recvLen);

#endif /*__meterProtocolH*/
