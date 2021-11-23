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
#define   READ_IO_PIN        0x1      /*读取IO脚值*/
#define   WIRELESS_ON_OFF    0x2      /*无线MODEM开关机*/

/*无线Modem开关机参数*/
#define   WIRELESS_PIN_LOW   0x0      /*无线Modem开机脚低*/
#define   WIRELESS_PIN_HIGH  0x1      /*无线Modem开机脚高*/

/*读取IO脚值参数*/
#define   KEY_1              0x1      /*第一个键*/
#define   KEY_2              0x2      /*第二个键*/
#define   KEY_3              0x3      /*第三个键*/
#define   KEY_4              0x4      /*第四个键*/



#endif   /*__INCioChannelH*/
