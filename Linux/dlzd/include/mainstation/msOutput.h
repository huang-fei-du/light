/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����msOutput.h
���ߣ�leiyong
�汾��0.9
�������:2006��7��
��������վ���֡ͷ�ļ�
�޸���ʷ��
  01,06-7-12,leiyong created.
**************************************************/
#ifndef __msOutputH
#define __msOutputH

//��������
void initSendQueue(void);
void replyToMs(void);
void sendToMs(INT8U *pack,INT16U length);
void sendToXmega(void);
void sendXmegaInTimeFrame(INT8U afn, INT8U *data, INT16U len);

#endif  //__msOutputH