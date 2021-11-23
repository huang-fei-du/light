/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ�����meterProtocol.h
����:leiyong
�汾��0.9
�������:2009��1��10��
���������ܱ�Э���û��ӿ�ͷ�ļ�
�޸���ʷ��
  01,09-01-07,Leiyong created.
**************************************************/

#ifndef __meterProtocolH
#define __meterProtocolH

#include "common.h"
#include "timeUser.h"

//1.����ǰ����ĳ������
struct meterCopyPara
{
   INT8U     port;                                       //����˿�
   INT16U    pn;                                         //�������
   INT8U     protocol;                                   //ͨ��Э��
   INT8U     meterAddr[6];                               //���ַ
   INT8U     copyDataType;                               //������������(��ǰ���ݻ�����������)
   DATE_TIME copyTime;                                   //����ʱ��
   
   INT8U     *dataBuff;                                  //���ݻ���
   
   void (*send)(INT8U port,INT8U *pack,INT16U length);   //��˿ڷ������ݺ���
   void (*save)(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len);  //���ݱ��溯��
   
};

//2.Э������
#define    DLT_645_1997                     1      //DL/T645-1997
#define    AC_SAMPLE                        2      //����װ��
#define    SIMENS_ZD_METER                  4      //������(/������)ZD��
#define    SIMENS_ZB_METER                  6      //������(/������)ZB��
#define    ABB_METER                        8      //ABB(����)��
#define    EDMI_METER                      10      //EDMI(����)��
#define    WASION_METER                    12      //��ʤ��Լ
#define    HONGHUA_METER                   13      //�뻪��Լ
#define    SUPERVISAL_DEVICE               15      //�ֳ�����豸
#define    DLT_645_2007                    30      //DL/T645-2007(�������ܵ��ܱ�)
#define    NARROW_CARRIER_MODE             31      //"���нӿ�����խ����ѹ�ز�ͨ��ģ��"�ӿ�Э��
#define    SINGLE_PHASE_645_1997          200      //����485��645-1997Э��
#define    SINGLE_PHASE_645_2007          201      //�������ܵ��645-2007Э��
#define    SINGLE_LOCAL_CHARGE_CTRL_2007  202      //���౾�طѿ����ܱ�
#define    SINGLE_REMOTE_CHARGE_CTRL_2007 203      //����Զ�̷ѿ����ܱ�
#define    THREE_2007                     204      //�����౾�طѿ����ܱ�
#define    THREE_LOCAL_CHARGE_CTRL_2007   205      //�����౾�طѿ����ܱ�
#define    THREE_REMOTE_CHARGE_CTRL_2007  206      //����Զ�̷ѿ����ܱ�
#define    KEY_HOUSEHOLD_2007             208      //07��Լ���ص��û�
#define    DOT_COPY_2007                  209      //07�㳭��Լ
#define    DOT_COPY_1997                  210      //97�㳭��Լ
#define    HOUR_FREEZE_2007               211      //07�����㶳��
#define    PN_WORK_NOWORK_1997            212      //645-1997Э��ֻ���������޹�����ʾֵ
#define    PN_WORK_NOWORK_2007            213      //645-2007Э��ֻ���������޹�����ʾֵ
#define    SINGLE_PHASE_645_2007_TARIFF   214      //�������ܵ��645-2007Э��(ֻ��ʵʱ�����ܼ�������)
#define    SINGLE_PHASE_645_2007_TOTAL    215      //�������ܵ��645-2007Э��(ֻ��ʵʱ������ʾֵ)

#define    LIGHTING_XL                    130      //��·������(������Ʒ)
#define    LIGHTING_DGM                   131      //���µ����������(������Ʒ)
#define    LIGHTING_LS                    132      //�նȴ�����(������Ʒ)
#define    LIGHTING_TH                    133      //��ʪ�ȴ�����(������Ʒ)

#define    MODBUS_HY                       50      //ɽ����ԶMODBUS���,2017-5-24
#define    MODBUS_ASS                      51      //������ɭ˼MODBUS���,2017-5-24
#define    MODBUS_XY_F                     52      //�Ϻ��ͻ�MODBUS�๦�ܵ��,2017-9-21
#define    MODBUS_XY_UI                    53      //�Ϻ��ͻ�MODBUS��ѹ������ϱ�,2017-9-21
#define    MODBUS_SWITCH                   54      //MODBUS������ģ��,2017-9-22
#define    MODBUS_XY_M                     55      //�Ϻ��ͻ�MODBUS���ģ��,2017-10-10
#define    MODBUS_T_H                      56      //MODBUS��ʪ��ģ��,2018-03-01
#define    MODBUS_MW_F                     57      //�ɶ�����MODBUS�๦�ܱ�,2018-03-01
#define    MODBUS_MW_UI                    58      //�ɶ�����MODBUS��ѹ������,2018-03-01
#define    MODBUS_JZ_F                     59      //�Ϻ�����MODBUS�๦�ܱ�,2018-03-01
#define    MODBUS_JZ_UI                    60      //�Ϻ�����MODBUS��ѹ������,2018-03-01
#define    MODBUS_PMC350_F                 61      //�����е�PMC350�������ֱ�,2018-05-03
#define    MODBUS_WE6301_F                 62      //��˹��WE6301�๦�ܱ�,2018-05-03


//3.��־������ֵ----------------------------------------------------------------------------------
//3.1��ǰ����/��������/��һ���ն�������
#define PRESENT_DATA                      0x0      //���յ�ǰ����
#define LAST_MONTH_DATA                   0x1      //������������(��һ����������)
#define LAST_DAY_DATA                     0x2      //������һ���ն�������
#define HOUR_FREEZE_DATA                  0x3      //�������㶳������
#define NEW_COPY_ITEM                     0x0      //�³�����
#define LAST_COPY_ITEM_RETRY              0x2      //��һ����������

//3.2����ֵ
#define CHANGE_METER_RATE                   7      //���ĳ�������
#define METER_REPLY_ANALYSE_OK              6      //��������֡�����ҽ�����ȷ,�ѱ��������
#define METER_NORMAL_REPLY                  5      //��������֡�����ұ������Ӧ��(���֧�ָ�����)
#define METER_ABERRANT_REPLY                4      //��������֡����,������쳣Ӧ��(�����Ǳ�˲�֧�ָ�����)
#define COPY_COMPLETE_NOT_SAVE              3      //�����굫����Ч����
#define COPY_COMPLETE_SAVE_DATA             2      //������������Ч�����ѱ���
#define DATA_SENDED                         1      //�����ѷ���
#define PROTOCOL_NOT_SUPPORT               -1      //Э�鲻֧��
#define RECV_DATA_CHECKSUM_ERROR           -2      //��������У��ʹ���
#define RECV_DATA_NOT_COMPLETE             -3      //�������ݲ�����,�����ȴ�����
#define RECV_DATA_TAIL_ERROR               -4      //��������֡β����,����
#define RECV_DATA_OFFSET_ERROR             -5      //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
#define RECV_ADDR_ERROR                    -6      //�������ݵĵ�ַ�뷢�͵ĵ�ַ��ͬ

//4.����Э���û��ӿں���
/*******************************************************
��������:initCopyProcess
��������:��ʼ���������
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE-��ʼ������ɹ�
        FALSE-��ʼ������ʧ��
*******************************************************/
BOOL initCopyProcess(struct meterCopyPara *cp);


/*******************************************************
��������:meterFraming
��������:����������֡����
���ú���:
�����ú���:
�������:INT8U flag,��־ D0,��ǰ���ݻ�����������(��һ����������)(D0=0-��ǰ����,D0=1-��������(��һ���������))
                         D1,�³����������һ����������(D1=0-�³�����,D1=1-��һ����������)   
�������:
����ֵ��=0,�ѷ���
        =1,������
        -1,
*******************************************************/
INT8S meterFraming(INT8U port,INT8U flag);

/*******************************************************
��������:meterReceiving
��������:��������֡����
���ú���:
�����ú���:
�������:port,����˿�
         data,��������ָ��
         recvLen,�������ݳ���
�������:
����ֵ:
*******************************************************/
INT8S meterReceiving(INT8U port,INT8U *data,INT16U recvLen);

#endif /*__meterProtocolH*/
