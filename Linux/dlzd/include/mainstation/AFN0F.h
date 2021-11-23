/*************************************************
Copyright,2006-2007,LongTong co.,LTD
文件名：AFN0F.h
作者：leiyong
版本：0.9
完成日期:2007年1月
描述：主站"文件传输(AFN0F)"头文件
修改历史：
  01,07-01-18,leiyong created.
  02,10-03-31,Leiyong,移植到Linux2.6.20
**************************************************/
#ifndef __AFN0FH
#define __AFN0FH

#include <stdio.h>

#include "common.h"

#define SIZE_OF_UPGRADE    512   //远程升级每片段大小

extern FILE *fpOfUpgrade;        //升级文件指针

typedef struct
{
	INT8U  flag;                   //升级标志(=1,升级中,2=升级完成)
  INT16U counter;                //共收到的程序片段个数
  INT16U perFrameSize;           //每程序片段大小  
}UPGRADE_FLAG;


//函数声明
void AFN0F(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);
void requestPacket(INT16U num);

#endif  /*__AFN0CH*/