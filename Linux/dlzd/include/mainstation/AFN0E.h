/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：AFN0E.h
作者：TianYe
版本：0.9
完成日期:2006年8月22日
描述：AFN0E头文件
修改历史:
  01,06-8-22,Tianye created.
**************************************************/

#ifndef __INCafn0eH
#define __INCafc0eH

#include "workWithMS.h"

#define ERC(i)         i                            //事件记录代码ERC

//函数声明
void AFN0E(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom, INT8U poll);

INT16U eventErc01(INT16U frameTail, INT8U *eventData);
INT16U eventErc02(INT16U frameTail, INT8U *eventData);
INT16U eventErc03(INT16U frameTail, INT8U *eventData);
INT16U eventErc04(INT16U frameTail, INT8U *eventData);
INT16U eventErc05(INT16U frameTail, INT8U *eventData);
INT16U eventErc06(INT16U frameTail, INT8U *eventData);
INT16U eventErc07(INT16U frameTail, INT8U *eventData);
INT16U eventErc08(INT16U frameTail, INT8U *eventData);
INT16U eventErc09(INT16U frameTail, INT8U *eventData);
INT16U eventErc10(INT16U frameTail, INT8U *eventData);
INT16U eventErc11(INT16U frameTail, INT8U *eventData);
INT16U eventErc12(INT16U frameTail, INT8U *eventData);
INT16U eventErc13(INT16U frameTail, INT8U *eventData);
INT16U eventErc14(INT16U frameTail, INT8U *eventData);
INT16U eventErc17(INT16U frameTail, INT8U *eventData);

INT16U eventErc19(INT16U frameTail, INT8U *eventData);
INT16U eventErc20(INT16U frameTail, INT8U *eventData);
INT16U eventErc21(INT16U frameTail, INT8U *eventData);
INT16U eventErc22(INT16U frameTail, INT8U *eventData);
INT16U eventErc23(INT16U frameTail, INT8U *eventData);
INT16U eventErc24(INT16U frameTail, INT8U *eventData);
INT16U eventErc25(INT16U frameTail, INT8U *eventData);
INT16U eventErc26(INT16U frameTail, INT8U *eventData);
INT16U eventErc27(INT16U frameTail, INT8U *eventData);
INT16U eventErc28(INT16U frameTail, INT8U *eventData);
INT16U eventErc29(INT16U frameTail, INT8U *eventData);
INT16U eventErc30(INT16U frameTail, INT8U *eventData);

INT16U eventErc31(INT16U frameTail, INT8U *eventData);
INT16U eventErc32(INT16U frameTail, INT8U *eventData);
INT16U eventErc33(INT16U frameTail, INT8U *eventData);
INT16U eventErc35(INT16U frameTail, INT8U *eventData);

INT16U eventErc36(INT16U frameTail, INT8U *eventData);

#endif //__INCafc0eH