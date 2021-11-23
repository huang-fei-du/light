/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：userInterface.h
作者：leiyong
版本：0.9
完成日期:2009年12月
描述：电力终端(负控终端、集中器)人机接口处理头文件
修改历史：
  01,09-12-23,leiyong created.

**************************************************/
#ifndef __INCUserInterfaceH
#define __INCUserInterfaceH

#include "common.h"

//显示模式
#define  DEFAULT_DISPLAY_MODE       1         //默认菜单方式
#define  CYCLE_DISPLAY_MODE         2         //轮显菜单方式
#define  KEYPRESS_DISPLAY_MODE      3         //按键菜单方式

#define  NUM_MP_PER_PAGE            8         //每页面显示的测量点个数

#define  STATUS_NONE              0x0         //还未输入密码
#define  STATUS_INPUT_PASS        0x1         //正在输入密码
#define  PASS_STATUS_CHECK_OK     0x2         //输入密码正确
#define  PASS_STATUS_CHECK_ERROR  0x3         //输入密码错误
#define  STATUS_SELECT_DIGITAL    0x4         //选择数字
#define  STATUS_SELECT_CHAR       0x5         //选择字符
#define  STATUS_SELECT_PROTOCOL   0x6         //选择协议
#define  STATUS_INPUT_NEW_PASS    0x7         //输入新密码
#define  STATUS_INPUT_TE_ADDR     0x8         //输入终端地址
#define  STATUS_INPUT_ADD_POINT   0x9         //输入测量点信息
#define  STATUS_INPUT_EDIT_POINT  0xA         //修改测量点信息
#define  STATUS_INPUT_VPN         0xB         //输入VPN

#ifdef PLUG_IN_CARRIER_MODULE   //集中器
 #ifdef MENU_FOR_CQ_CANON
  #define MAX_LAYER1_MENU           6         //一层菜单最大值
  #define MAX_LAYER2_MENU           6         //二层菜单最大值
 #else
  #ifdef LIGHTING
   #define MAX_LAYER1_MENU          6         //一层菜单最大值
  #else
   #define MAX_LAYER1_MENU          3         //一层菜单最大值
  #endif
  #define MAX_LAYER2_MENU          16         //二层菜单最大值
 #endif
#else    //专变III型终端
  #define MAX_LAYER1_MENU           7         //一层菜单最大值
  #define MAX_LAYER2_MENU          15         //二层菜单最大值
#endif

#define MAXINTERFACES 16

//结构 - 载波表信息
struct carrierMeterInfo
{
	 INT16U mp;                                 //测量点号
	 INT16U mpNo;                               //载波从节点序号
	 INT8U  info[2];                            //从节点信息
	 INT8U  addr[6];                            //载波从节点地址
	 INT8U  protocol;                           //载波从节点规约类型
	 INT8U  copyTime[2];
	 INT8U  copyEnergy[4];
	 
	 struct carrierMeterInfo *next;	            //下一节点
};

//结构 - 异常告警显示
typedef struct
{
	 unsigned int aberrantFlag    :1;           //异常标志(是否有异常发生)
	 unsigned int blinkCount      :3;           //闪烁计数
	 
	 INT8U        eventNum;                     //事件编号
	 DATE_TIME    timeOut;                      //异常闪烁持续时间

}ABERRANT_ALARM;


//外部变量
extern INT8U          displayMode;            //显示模式(默认菜单-1,轮显-2,按键-3)
extern struct         carrierMeterInfo *mpCopyHead,*tmpMpCopy,*prevMpCopy;                    //所有测量点抄表指针
extern struct         carrierMeterInfo *foundMeterHead,*tmpFound,*prevFound,*noFoundMeterHead;//发现电表指针
extern struct         carrierMeterInfo *existMeterHead,*prevExistFound;                       //发现电表与配置相同指针

extern DATE_TIME      foundTimeOut;           //发现电表超时时间
extern ABERRANT_ALARM aberrantAlarm;          //异常告警

//函数声明
void netInfo(void);
void terminalAddr(void);
void terminalStatus(INT8U num);

//国家电网显示规范函数
void showInfo(char *info);
void refreshTitleTime(void);

//重庆显示规范函数
void copyQueryMenu(INT8U layer2Light,INT8U layer3Light);
void paraQueryMenu(INT8U layer2Light,INT8U layer3Light);
void keyHouseholdMenu(INT8U layer2Light,INT8U layer3Light);
void statisQueryMenu(INT8U layer2Light,INT8U layer3Light);
void paraSetMenu(int lightNum,int layer1Num);
void debugMenu(int lightNum,int layerNum);
void realCopyMeterMenu(int lightNum);
void inputPassWord(INT8U lightNum);
void modifyPasswordMenu(INT8U layer2Light,INT8U layer3Light);
void singleMeterCopy(char *mp,char *time,INT8U *energyData,INT8U lightNum);
void singleMeterCopyReport(INT8U *data);
void allMpCopy(INT8U lightNum);
void searchMeter(INT8U lightNum);
void newAddMeter(INT8U lightNum);
void activeReportMenu(INT8U lightNum);
void freeQueryMpLink(void);
void fillTimeStr(void);
void stringUpDown(char *processStr, INT8U timeChar, INT8U upDown);
BOOL checkInputTime(void);
void showInputTime(INT8U layer3Light);
void adjustCommParaLight(INT8U leftRight);
void searchMeterReport(void);
void newMeterCpStatus(INT8U lightNum);
void inputApn(INT8U leftRight);
void searchCtrlEvent(INT8U erc);
void eventRecordShow(INT8U erc);
void setRlPara(INT8U layer2Light,INT8U layer3Light);
void setVpn(INT8U lightNum);
void setCopyForm(INT8U lightNum);

//专变III型终端函数
void configPara(INT8U rowLight,INT8U colLight);


extern INT8U          wlRssi;                 //无线Modem信号

//各种机型公共外部变量
extern DATE_TIME      lcdLightDelay;          //LCD背光延时
extern INT8S          menuInLayer;            //进菜单层数
extern INT8U          lcdLightOn;             //LCD背光打开?

extern DATE_TIME      searchStart, searchEnd; //搜索开始时间

//各种机型公共函数
void userInterface(BOOL secondChanged);
void startDisplay(void);
void modemSignal(int type);
void signalReport(INT8U type,INT8U rssi);
void layer1Menu(int	num);
void layer2Menu(int lightNum,int layer1Num);
void messageTip(int type);

#endif    /*__INCUserInterfaceH*/