/*************************************************
Copyright,2010,Huawei WoDian co.,LTD
�ļ�����hardwareConfig.h
���ߣ�leiyong
�汾��0.9
�������:2010��01��
�����������ն�(�����նˡ�������)Ӳ������ͷ�ļ�
�޸���ʷ��
  01,10-01-16,leiyong created.

**************************************************/
#ifndef __HardwareConfigH
#define __HardwareConfigH

#include "common.h"

extern int fdOfModem;     //����1(����Modem)�ļ�������
extern int fdOfttyS2;     //����2(���ؿ��ƿ�)�ļ�������
extern int fdOfttyS3;     //����3(485�ӿ�)�ļ�������
extern int fdOfttyS4;     //����4(485�ӿ�)�ļ�������
extern int fdOfCarrier;   //����5(�ز��ӿ�)�ļ�������
extern int fdOfLocal;     //����6(����ά���ӿ�)�ļ�������
extern int fdOfSample;    //�����ļ�������
extern int fdOfIoChannel; //I/Oͨ���ļ�������

//��������
int   initHardware(void);
void  oneUartConfig(INT32U fd,INT8U ctrlWord);
void  uart0Config(INT8U rate);
void  resetWlModem(INT8U press);
void  wlModemPowerOnOff(INT8U onOff);
void  sendLocalMsFrame(INT8U *pack,INT16U length);
void  sendWirelessFrame(INT8U *pack,INT16U length);
void  sendDebugFrame(INT8U *pack,INT16U length);
void  setBeeper(INT8U ifVoice);
void  alarmLedCtrl(INT8U ifLight);

void  sendToAvr(INT8U *data,INT16U len);
INT8U getGateKValue(void);
void  detectReset(BOOL secondChanged);

void  sendXmegaFrame(INT8U afn, INT8U *data, INT16U len);
void  carrierUartInit(void);

#endif /*__HardwareConfigH*/
