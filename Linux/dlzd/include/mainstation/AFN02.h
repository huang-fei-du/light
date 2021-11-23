/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：AFN02.h
作者：leiyong
版本：0.9
完成日期:2006年7月
描述：AFN02头文件。
修改历史：
  01,06-7-28,leiyong created.
**************************************************/

#ifndef __INCAfn02H
#define __INCAfn02H

void AFN02001Login(void);
void AFN02002Logout(void);
void AFN02003HeartBeat(void);

#ifdef CQDL_CSM
 void AFN02008(void);
#endif
#endif