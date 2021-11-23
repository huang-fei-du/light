/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
文件名：convert.h
作者：leiyong
版本：0.9
完成日期:2009年12月
描述：转换函数头文件
修改历史：
  01,09-12-23,leiyong created.

**************************************************/
#ifndef __INCConvertH
#define __INCConvertH

#include "common.h"

#include "timeUser.h"

//函数声明
char      *digitalToChar(INT8U digital);
char      *intToString(INT32U in,int type,char *returnStr);
char      *digital2ToString(INT8U in,char *returnStr);
char      *int8uToHex(INT8U in,char *returnStr);
char      *floatToString(INT32U integer,INT32U decimal,INT8U precision, INT8U decNum,char *returnStr);
char      *intToIpadd(INT32U ip,char * returnStr);
INT32U    hexToBcd(INT32U hex);
INT32U    bcdToHex(INT32U bcd);
DATE_TIME timeBcdToHex(DATE_TIME bcdTime);
DATE_TIME timeHexToBcd(DATE_TIME hexTime);

#endif    /*__INCConvertH*/
