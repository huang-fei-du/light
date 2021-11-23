/*************************************************
Copyright ly,2010
文件名:ioChannel.h
作者:lhh
版本：0.9
完成日期:2010年04月
描述：电力终端(负控终端)输入输出通道公用头文件
修改历史：
  01,10-02-18,lhh created.

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
#define   LOAD_CTRL_LINE_1       0xe      /*负荷控制第一路*/
#define   LOAD_CTRL_LINE_2       0xf      /*负荷控制第二路*/
#define   LOAD_CTRL_LINE_3      0x10      /*负荷控制第三路*/
#define   LOAD_CTRL_LINE_4      0x11      /*负荷控制第四路*/
#define   READ_GATEK_VALUE      0x12      /*读取门控值*/
#define   SET_BATTERY_CHARGE    0x13      /*后备充电管理*/

#define   IO_LOW                 0x0      /*IO脚为低*/
#define   IO_HIGH                0x1      /*IO脚为高*/

#define   CTRL_GATE_IDLE        0x00      /*控制继电器空闲*/
#define   CTRL_GATE_JUMP        0x55      /*控制继电器跳闸*/
#define   CTRL_GATE_CLOSE       0xaa      /*控制继电器合闸*/

/*键值*/
#define   KEY_UP                 0x1      /*上键*/
#define   KEY_DOWN               0x2      /*下键*/
#define   KEY_LEFT               0x3      /*左键*/
#define   KEY_RIGHT              0x4      /*右键*/
#define   KEY_OK                 0x5      /*确认键*/
#define   KEY_CANCEL             0x6      /*取消键*/



#endif   /*__INCioChannelH*/
