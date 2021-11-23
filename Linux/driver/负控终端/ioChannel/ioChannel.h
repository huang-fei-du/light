/*************************************************
Copyright ly,2010
�ļ���:ioChannel.h
����:lhh
�汾��0.9
�������:2010��04��
�����������ն�(�����ն�)�������ͨ������ͷ�ļ�
�޸���ʷ��
  01,10-02-18,lhh created.

**************************************************/
#ifndef __INCioChannelH
#define __INCioChannelH

/*IO��������*/
#define   READ_KEY_VALUE         0x1      /*��ȡ��ֵ*/
#define   READ_MODULE_TYPE       0x2      /*��ȡģ������*/
#define   WIRELESS_POWER_ON_OFF  0x3      /*����MODEM���ص�Դ*/
#define   WIRELESS_IGT           0x4      /*����MODEM���ػ�,press key*/
#define   WIRELESS_RESET         0x5      /*����MODEM��λ*/
#define   READ_YX_VALUE          0x6      /*��ȡң��ֵ*/
#define   DETECT_POWER_FAILURE   0x7      /*ͣ����*/
#define   SET_CARRIER_MODULE     0x8      /*�����ز�ģ��*/
#define   RST_CARRIER_MODULE     0x9      /*��λ�ز�ģ��*/
#define   SET_ALARM_LIGHT        0xA      /*���ø澯����/��*/
#define   SET_ALARM_VOICE        0xB      /*���ø澯����/����*/
#define   SET_WATCH_DOG          0xc      /*���Ź�ι*/
#define   SET_BATTERY_ON         0xd      /*�󱸵��ͨ��*/
#define   LOAD_CTRL_LINE_1       0xe      /*���ɿ��Ƶ�һ·*/
#define   LOAD_CTRL_LINE_2       0xf      /*���ɿ��Ƶڶ�·*/
#define   LOAD_CTRL_LINE_3      0x10      /*���ɿ��Ƶ���·*/
#define   LOAD_CTRL_LINE_4      0x11      /*���ɿ��Ƶ���·*/
#define   READ_GATEK_VALUE      0x12      /*��ȡ�ſ�ֵ*/
#define   SET_BATTERY_CHARGE    0x13      /*�󱸳�����*/

#define   IO_LOW                 0x0      /*IO��Ϊ��*/
#define   IO_HIGH                0x1      /*IO��Ϊ��*/

#define   CTRL_GATE_IDLE        0x00      /*���Ƽ̵�������*/
#define   CTRL_GATE_JUMP        0x55      /*���Ƽ̵�����բ*/
#define   CTRL_GATE_CLOSE       0xaa      /*���Ƽ̵�����բ*/

/*��ֵ*/
#define   KEY_UP                 0x1      /*�ϼ�*/
#define   KEY_DOWN               0x2      /*�¼�*/
#define   KEY_LEFT               0x3      /*���*/
#define   KEY_RIGHT              0x4      /*�Ҽ�*/
#define   KEY_OK                 0x5      /*ȷ�ϼ�*/
#define   KEY_CANCEL             0x6      /*ȡ����*/



#endif   /*__INCioChannelH*/
