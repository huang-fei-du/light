/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����AFN02.h
���ߣ�leiyong
�汾��0.9
�������:2006��7��
������AFN02ͷ�ļ���
�޸���ʷ��
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