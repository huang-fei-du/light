/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ�����dataBase.h
���ߣ�leiyong
�汾��0.9
�������:2009��11��
�����������ն�(�����նˡ�������)���ݿ�ͷ�ļ�
�޸���ʷ��
  01,09-10-27,leiyong created.

**************************************************/
#ifndef __dataBaseH
#define __dataBaseH

#include "sqlite3.h"
#include "timeUser.h"

//������ݶ���,ly,2011-10-19,add
struct clearQueue
{
	char tableName[15];
	
	struct clearQueue *next;
};

extern sqlite3   *sqlite3Db;              //ȫ��SQLite3���ݿ���(SQLite db handle)

//���ݲ�������---------------------------------------------------
#define PRESENT_DATA                  0x0   //��ǰ����
#define LAST_MONTH_DATA               0x1   //��������
#define FIRST_TODAY                   0x2   //����ĵ�һ����
#define LAST_TODAY                    0x3   //�������һ����
#define LAST_LAST_REAL_DATA           0x4   //���ϴ�ʵʱ����
#define LAST_REAL_BALANCE             0x5   //���һ��ʵʱ��������
#define REAL_BALANCE                  0x6   //ʵʱ��������
#define DAY_BALANCE                   0x7   //�ս�������
#define MONTH_BALANCE                 0x8   //�½�������
#define EVENT_RECORD                  0x9   //�¼���¼����
#define CURVE_DATA_PRESENT            0xa   //��������(�ڵ�ǰ�����в��ҵ���������)
#define CURVE_DATA_BALANCE            0xb   //��������(��ʵʱ���������в��ҵ���������)
#define STATIS_DATA                   0xc   //ͳ������(�ն�ͳ�����ݼ�����ͳ������)
#define SINGLE_PHASE_PRESENT          0xd   //�����ǰ����
#define SINGLE_PHASE_DAY              0xe   //������ն�������
#define CURVE_SINGLE_PHASE            0xf   //�������������
#define SINGLE_PHASE_MONTH           0x10   //������¶�������
#define FIRST_MONTH                  0x11   //���µĵ�һ����
#define LEFT_POWER                   0x12   //ʣ�����
#define SINGLE_LOCAL_CTRL_PRESENT    0x13   //���౾�طѿص�ǰ����
#define SINGLE_LOCAL_CTRL_DAY        0x14   //���౾�طѿ��ն�������
#define SINGLE_LOCAL_CTRL_MONTH      0x15   //���౾�طѿ��¶�������
#define SINGLE_REMOTE_CTRL_PRESENT   0x16   //����Զ�̷ѿص�ǰ����
#define SINGLE_REMOTE_CTRL_DAY       0x17   //����Զ�̷ѿ��ն�������
#define SINGLE_REMOTE_CTRL_MONTH     0x18   //����Զ�̷ѿ��¶�������
#define THREE_PRESENT                0x19   //�������ܱ�ǰ����
#define THREE_DAY                    0x1A   //�������ܱ��ն�������
#define THREE_MONTH                  0x1B   //�������ܱ��¶�������
#define THREE_LOCAL_CTRL_PRESENT     0x1C   //���౾�طѿص�ǰ����
#define THREE_LOCAL_CTRL_DAY         0x1D   //���౾�طѿ��ն�������
#define THREE_LOCAL_CTRL_MONTH       0x1E   //���౾�طѿ��¶�������
#define THREE_REMOTE_CTRL_PRESENT    0x1F   //����Զ�̷ѿص�ǰ����
#define THREE_REMOTE_CTRL_DAY        0x20   //����Զ�̷ѿ��ն�������
#define THREE_REMOTE_CTRL_MONTH      0x21   //����Զ�̷ѿ��¶�������
#define KEY_HOUSEHOLD_PRESENT        0x22   //�ص��û���ǰ����
#define KEY_HOUSEHOLD_DAY            0x23   //�ص��û��ն�������
#define KEY_HOUSEHOLD_MONTH          0x24   //�ص��û��ն�������
#define CURVE_KEY_HOUSEHOLD          0x25   //�ص��û���������
#define HOUR_FREEZE                  0x26   //���㶳������
#define DC_ANALOG                    0x27   //ֱ��ģ��������
#define HOUR_FREEZE_SLC              0x28   //���ƿ�����-���㶳������


//���ݴ洢����----------------------------------------------------
#define  ENERGY_DATA                 0x01   //ʵʱ����ʾֵ
#define  REQ_REQTIME_DATA            0x02   //ʵʱ����������ʱ��
#define  PARA_VARIABLE_DATA          0x03   //�������α���
#define  SHI_DUAN_DATA               0x04   //ʵʱ����--ʱ������
#define  REAL_BALANCE_POWER_DATA     0x05   //ʵʱ���������
#define  REAL_BALANCE_PARA_DATA      0x06   //ʵʱ����α���
#define  DAY_BALANCE_POWER_DATA      0x07   //�ս��������
#define  DAY_BALANCE_PARA_DATA       0x08   //�ս���α���
#define  MONTH_BALANCE_POWER_DATA    0x09   //�½��������
#define  MONTH_BALANCE_PARA_DATA     0x0A   //�½���α���
#define  DAY_FREEZE_COPY_DATA        0x0B   //�ս���ת��ʾֵ
#define  DAY_FREEZE_COPY_REQ         0x0C   //�ս���ת������
#define  MONTH_FREEZE_COPY_DATA      0x0D   //�½���ת��ʾֵ
#define  MONTH_FREEZE_COPY_REQ       0x0E   //�½���ת������
#define  POWER_PARA_LASTMONTH        0x0F   //���µ���ʾֵ������
#define  REQ_REQTIME_LASTMONTH       0x10   //��������������ʱ��
#define  GROUP_REAL_BALANCE          0x11   //ʵʱ�ܼ������
#define  GROUP_DAY_BALANCE           0x12   //���ܼ������
#define  GROUP_MONTH_BALANCE         0x13   //���ܼ������
#define  COMMUNICATION_STATIS        0x14   //�ն˹�����ͨ��ͳ��
#define  COPY_FREEZE_COPY_DATA       0x15   //�����ն������ʾֵ
#define  COPY_FREEZE_COPY_REQ        0x16   //�����ն�������������ʱ��
#define  DAY_FREEZE_COPY_DATA_M      0x17   //�ս�������ʾֵ
#define  DAY_FREEZE_COPY_REQ_M       0x18   //�ս�����������
#define  MONTH_FREEZE_COPY_DATA_M    0x19   //�½�������ʾֵ
#define  MONTH_FREEZE_COPY_REQ_M     0x1A   //�½�����������
#define  DC_ANALOG_CURVE_DATA        0x1B   //ֱ��ģ������������

//FLASHд�����־------------------------------------------------
#define FLUSH_REAL                   0x10   //дʵʱ���ݻ����־
#define FLUSH_LASTMONTH              0x11   //д�������ݻ����־
#define FLUSH_REAL_BALANCE           0x12   //дʵʱ��������
#define FLUSH_DAY_BALANCE            0x13   //д�ս�������
#define FLUSH_MONTH_BALANCE          0x14   //д�½�������
#define FLUSH_STATIS                 0x15   //дʵʱͳ������
#define FLUSH_LEFTPOWER              0x16   //дʣ�����
#define FLUSH_STATIS2                0x17   //дʵʱͳ������2

#define FLUSH_REAL_2                 0x02   //дʵʱ�޷������ݻ����־
#define FLUSH_HOUR                   0x05   //д�������ݻ����־

void   initDataBase(void);
void   loadParameter(void);
void   saveParameter(INT8U, INT8U, INT8U *, INT32U);
void   saveViceParameter(INT8U, INT8U, INT16U, INT8U *, INT16U);
void   deleteParameter(INT8U, INT8U);
void   insertParameter(INT8U, INT8U, INT16U, INT8U *, INT16U);
void   countParameter(INT8U, INT8U, INT16U *);
BOOL   selectParameter(INT8U, INT8U, INT8U *, INT32U);
BOOL   selectViceParameter(INT8U, INT8U, INT16U, INT8U *, INT16U);
void   saveDataF10(INT16U pn, INT8U port, INT8U *meterAddr, INT16U num, INT8U *para, INT16U len);
BOOL   selectF10Data(INT16U pn, INT8U port, INT16U num, INT8U *para, INT16U len);
INT16U queryData(char *pSql,INT8U *data,INT8U type);

INT32U queryEventStoreNo(INT8U eventType);

BOOL   insertData(char *pSql);
BOOL   saveMeterData(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len);
BOOL   readMeterData(INT8U *tmpData, INT16U pn, INT8U queryType, INT16U dataType, DATE_TIME *time, INT8U mix);

void   flushBuff(INT8U buffType, INT8U dataType);
BOOL   writeEvent(INT8U *data,INT8U length,INT8U type, INT8U dataFromType);
void   deleteData(INT8U type);
char   * bringTableName(DATE_TIME time,INT8U type);
void   checkSpRealTable(INT8U type);
void   threadOfClearData(void *arg);
void   logRun(char *str);

BOOL   readAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType);
BOOL   updateAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType);

void   readIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4]);
void   saveIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4]);

void   sqlite3Watch(void);
void   deletePresentData(INT16U pn);
void   saveBakKeyPara(INT8U type);
void   readBakKeyPara(INT8U type, INT8U *buf);

BOOL   readBakDayFile(INT16U pn, DATE_TIME *time, INT8U *buf, INT8U type);

INT8U  queryModemStatus(void);

INT8U  insertModemCtrl(void);

#ifdef LIGHTING

BOOL readSlDayData(INT16U pn, INT8U type, DATE_TIME readTime, INT8U *dataBuf);
BOOL saveSlDayData(INT16U pn, INT8U type, DATE_TIME saveTime, INT8U *dataBuf, INT8U len);
void deleteCtimes(void);

#endif

#endif /*__dataBaseH*/
