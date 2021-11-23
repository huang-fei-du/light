/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：msOutput.h
作者：leiyong
版本：0.9
完成日期:2006年7月
描述：主站输出帧头文件
修改历史：
  01,06-7-12,leiyong created.
**************************************************/
#ifndef __msOutputH
#define __msOutputH

//函数声明
void initSendQueue(void);
void replyToMs(void);
void sendToMs(INT8U *pack,INT16U length);
void sendToXmega(void);
void sendXmegaInTimeFrame(INT8U afn, INT8U *data, INT16U len);

#endif  //__msOutputH