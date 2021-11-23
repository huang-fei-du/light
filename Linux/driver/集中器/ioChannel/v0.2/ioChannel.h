/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：ioChannel.h
作者：leiyong
版本：0.9
完成日期:2009年12月
描述：电力终端(负控终端、集中器)IO通道公用头文件
修改历史：
  01,09-12-22,leiyong created.

**************************************************/
#ifndef __INCioChannelH
#define __INCioChannelH

/*IO控制命令*/
#define   READ_KEY_VALUE         0x1      /*读取键值*/
#define   READ_MODULE_TYPE       0x2      /*读取模块类型*/
#define   WIRELESS_POWER_ON_OFF  0x3      /*无线MODEM开关电源*/
#define   WIRELESS_IGT           0x4      /*无线MODEM开关机,press key*/
#define   WIRELESS_RESET         0x5      /*无线MODEM复位*/
#define   READ_YX_VALUE          0x6      /*读取遥信值*/
#define   DETECT_POWER_FAILURE   0x7      /*停电检测*/
#define   SET_CARRIER_MODULE     0x8      /*设置载波模块*/
#define   RST_CARRIER_MODULE     0x9      /*复位载波模块*/
#define   SET_ALARM_LIGHT        0xA      /*设置告警灯亮/灭*/
#define   SET_ALARM_VOICE        0xB      /*设置告警音响/静音*/
#define   SET_WATCH_DOG          0xc      /*看门狗喂*/
#define   SET_BATTERY_ON         0xd      /*后备电池通断*/


#define   IO_LOW                 0x0      /*IO脚为低*/
#define   IO_HIGH                0x1      /*IO脚为高*/

/*键值*/
#define   KEY_UP                 0x1      /*上键*/
#define   KEY_DOWN               0x2      /*下键*/
#define   KEY_LEFT               0x3      /*左键*/
#define   KEY_RIGHT              0x4      /*右键*/
#define   KEY_OK                 0x5      /*确认键*/
#define   KEY_CANCEL             0x6      /*取消键*/



#endif   /*__INCioChannelH*/
