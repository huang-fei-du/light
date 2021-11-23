/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：dataBase.h
作者：leiyong
版本：0.9
完成日期:2009年11月
描述：电力终端(负控终端、集中器)数据库头文件
修改历史：
  01,09-10-27,leiyong created.

**************************************************/
#ifndef __dataBaseH
#define __dataBaseH

#include "sqlite3.h"
#include "timeUser.h"

//清除数据队列,ly,2011-10-19,add
struct clearQueue
{
	char tableName[15];
	
	struct clearQueue *next;
};

extern sqlite3   *sqlite3Db;              //全局SQLite3数据库句柄(SQLite db handle)

//数据查找类型---------------------------------------------------
#define PRESENT_DATA                  0x0   //当前数据
#define LAST_MONTH_DATA               0x1   //上月数据
#define FIRST_TODAY                   0x2   //今天的第一个数
#define LAST_TODAY                    0x3   //今天最后一个数
#define LAST_LAST_REAL_DATA           0x4   //上上次实时数据
#define LAST_REAL_BALANCE             0x5   //最近一条实时结算数据
#define REAL_BALANCE                  0x6   //实时结算数据
#define DAY_BALANCE                   0x7   //日结算数据
#define MONTH_BALANCE                 0x8   //月结算数据
#define EVENT_RECORD                  0x9   //事件记录数据
#define CURVE_DATA_PRESENT            0xa   //曲线数据(在当前数据中查找的曲线数据)
#define CURVE_DATA_BALANCE            0xb   //曲线数据(在实时结算数据中查找的曲线数据)
#define STATIS_DATA                   0xc   //统计数据(终端统计数据及电能统计数据)
#define SINGLE_PHASE_PRESENT          0xd   //单相表当前数据
#define SINGLE_PHASE_DAY              0xe   //单相表日冻结数据
#define CURVE_SINGLE_PHASE            0xf   //单相表曲线数据
#define SINGLE_PHASE_MONTH           0x10   //单相表月冻结数据
#define FIRST_MONTH                  0x11   //当月的第一个数
#define LEFT_POWER                   0x12   //剩余电量
#define SINGLE_LOCAL_CTRL_PRESENT    0x13   //单相本地费控当前数据
#define SINGLE_LOCAL_CTRL_DAY        0x14   //单相本地费控日冻结数据
#define SINGLE_LOCAL_CTRL_MONTH      0x15   //单相本地费控月冻结数据
#define SINGLE_REMOTE_CTRL_PRESENT   0x16   //单相远程费控当前数据
#define SINGLE_REMOTE_CTRL_DAY       0x17   //单相远程费控日冻结数据
#define SINGLE_REMOTE_CTRL_MONTH     0x18   //单相远程费控月冻结数据
#define THREE_PRESENT                0x19   //三相智能表当前数据
#define THREE_DAY                    0x1A   //三相智能表日冻结数据
#define THREE_MONTH                  0x1B   //三相智能表月冻结数据
#define THREE_LOCAL_CTRL_PRESENT     0x1C   //三相本地费控当前数据
#define THREE_LOCAL_CTRL_DAY         0x1D   //三相本地费控日冻结数据
#define THREE_LOCAL_CTRL_MONTH       0x1E   //三相本地费控月冻结数据
#define THREE_REMOTE_CTRL_PRESENT    0x1F   //三相远程费控当前数据
#define THREE_REMOTE_CTRL_DAY        0x20   //三相远程费控日冻结数据
#define THREE_REMOTE_CTRL_MONTH      0x21   //三相远程费控月冻结数据
#define KEY_HOUSEHOLD_PRESENT        0x22   //重点用户当前数据
#define KEY_HOUSEHOLD_DAY            0x23   //重点用户日冻结数据
#define KEY_HOUSEHOLD_MONTH          0x24   //重点用户日冻结数据
#define CURVE_KEY_HOUSEHOLD          0x25   //重点用户曲线数据
#define HOUR_FREEZE                  0x26   //整点冻结数据
#define DC_ANALOG                    0x27   //直流模拟量数据
#define HOUR_FREEZE_SLC              0x28   //单灯控制器-整点冻结数据


//数据存储类型----------------------------------------------------
#define  ENERGY_DATA                 0x01   //实时电能示值
#define  REQ_REQTIME_DATA            0x02   //实时需量及发生时间
#define  PARA_VARIABLE_DATA          0x03   //参量及参变量
#define  SHI_DUAN_DATA               0x04   //实时数据--时段数据
#define  REAL_BALANCE_POWER_DATA     0x05   //实时结算电能量
#define  REAL_BALANCE_PARA_DATA      0x06   //实时结算参变量
#define  DAY_BALANCE_POWER_DATA      0x07   //日结算电能量
#define  DAY_BALANCE_PARA_DATA       0x08   //日结算参变量
#define  MONTH_BALANCE_POWER_DATA    0x09   //月结算电能量
#define  MONTH_BALANCE_PARA_DATA     0x0A   //月结算参变量
#define  DAY_FREEZE_COPY_DATA        0x0B   //日结算转存示值
#define  DAY_FREEZE_COPY_REQ         0x0C   //日结算转存需量
#define  MONTH_FREEZE_COPY_DATA      0x0D   //月结算转存示值
#define  MONTH_FREEZE_COPY_REQ       0x0E   //月结算转存需量
#define  POWER_PARA_LASTMONTH        0x0F   //上月电能示值及参数
#define  REQ_REQTIME_LASTMONTH       0x10   //上月需量及发生时间
#define  GROUP_REAL_BALANCE          0x11   //实时总加组结算
#define  GROUP_DAY_BALANCE           0x12   //日总加组结算
#define  GROUP_MONTH_BALANCE         0x13   //月总加组结算
#define  COMMUNICATION_STATIS        0x14   //终端工况及通信统计
#define  COPY_FREEZE_COPY_DATA       0x15   //抄表日冻结电量示值
#define  COPY_FREEZE_COPY_REQ        0x16   //抄表日冻结需量及发生时间
#define  DAY_FREEZE_COPY_DATA_M      0x17   //日结算电表冻结示值
#define  DAY_FREEZE_COPY_REQ_M       0x18   //日结算电表冻结需量
#define  MONTH_FREEZE_COPY_DATA_M    0x19   //月结算电表冻结示值
#define  MONTH_FREEZE_COPY_REQ_M     0x1A   //月结算电表冻结需量
#define  DC_ANALOG_CURVE_DATA        0x1B   //直流模拟量数据曲线

//FLASH写缓存标志------------------------------------------------
#define FLUSH_REAL                   0x10   //写实时数据缓冲标志
#define FLUSH_LASTMONTH              0x11   //写上月数据缓冲标志
#define FLUSH_REAL_BALANCE           0x12   //写实时结算数据
#define FLUSH_DAY_BALANCE            0x13   //写日结算数据
#define FLUSH_MONTH_BALANCE          0x14   //写月结算数据
#define FLUSH_STATIS                 0x15   //写实时统计数据
#define FLUSH_LEFTPOWER              0x16   //写剩余电量
#define FLUSH_STATIS2                0x17   //写实时统计数据2

#define FLUSH_REAL_2                 0x02   //写实时无费率数据缓冲标志
#define FLUSH_HOUR                   0x05   //写负荷数据缓冲标志

void   initDataBase(void);
void   loadParameter(void);
void   saveParameter(INT8U, INT8U, INT8U *, INT32U);
void   saveViceParameter(INT8U, INT8U, INT16U, INT8U *, INT16U);
void   deleteParameter(INT8U, INT8U);
void   insertParameter(INT8U, INT8U, INT16U, INT8U *, INT16U);
void   countParameter(INT8U, INT8U, INT16U *);
BOOL   selectParameter(INT8U, INT8U, INT8U *, INT32U);
BOOL   selectViceParameter(INT8U, INT8U, INT16U, INT8U *, INT16U);
void   saveDataF10(INT16U pn, INT8U port, INT8U *meterAddr, INT16U num, INT8U *para, INT16U len);
BOOL   selectF10Data(INT16U pn, INT8U port, INT16U num, INT8U *para, INT16U len);
INT16U queryData(char *pSql,INT8U *data,INT8U type);

INT32U queryEventStoreNo(INT8U eventType);

BOOL   insertData(char *pSql);
BOOL   saveMeterData(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len);
BOOL   readMeterData(INT8U *tmpData, INT16U pn, INT8U queryType, INT16U dataType, DATE_TIME *time, INT8U mix);

void   flushBuff(INT8U buffType, INT8U dataType);
BOOL   writeEvent(INT8U *data,INT8U length,INT8U type, INT8U dataFromType);
void   deleteData(INT8U type);
char   * bringTableName(DATE_TIME time,INT8U type);
void   checkSpRealTable(INT8U type);
void   threadOfClearData(void *arg);
void   logRun(char *str);

BOOL   readAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType);
BOOL   updateAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType);

void   readIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4]);
void   saveIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4]);

void   sqlite3Watch(void);
void   deletePresentData(INT16U pn);
void   saveBakKeyPara(INT8U type);
void   readBakKeyPara(INT8U type, INT8U *buf);

BOOL   readBakDayFile(INT16U pn, DATE_TIME *time, INT8U *buf, INT8U type);

INT8U  queryModemStatus(void);

INT8U  insertModemCtrl(void);

#ifdef LIGHTING

BOOL readSlDayData(INT16U pn, INT8U type, DATE_TIME readTime, INT8U *dataBuf);
BOOL saveSlDayData(INT16U pn, INT8U type, DATE_TIME saveTime, INT8U *dataBuf, INT8U len);
void deleteCtimes(void);

#endif

#endif /*__dataBaseH*/
