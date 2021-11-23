/***************************************************
Copyright,2010,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:meterProtocol.c
����:leiyong
�汾:0.9
�������:2010��1��
����:����Լ�⴦���ļ�
�����б�:
  1.
�޸���ʷ:
  01,10-01-07,Leiyong created.
  02,12-01-10,Leiyong������6���˿�ͬʱ����,��Ӧ�����ز�·�����󳭶�ʱ������λͬʱ����
  03,12-05-21,Leiyong�޸�,�����չⷢ�ּ��������������ػ���07��,�����Ǹñ�֧�����ݿ�,ֻ֧���������й���ʾֵ,
              �޸�07��ȳ����ݿ�,ͬʱ�����������й���ʾֵ(������һ�����ա���ǰ)�ĳ���
  04,12-05-29,Leiyong�޸�,�ڹ�˾���Է���,����DDZY188C-Z����ѿر��ڳ�����ѹ������������ʱ,�豸�������ݿ�,
              ����ڻظ�����ʱҲ�ǻظ������ݿ�,����ֻ��A���,�����豸�ͻ�����ʶ��B��C�������,
              ����: ����ж�"i<frameAnalyse.loadLen",���������ؼ�����Ǽ��ࡣ
***************************************************/
#include "convert.h"
#include "string.h"
#include "stdio.h"

#include "dataBase.h"
#include "meterProtocolx.h"
#include "meterProtocol.h"

//1.�������
struct meterCopy
{
  INT8U     port;              //����˿�
  INT16U    pn;                //�������
  INT8U     protocol;          //ͨ��Э��
  INT8U     meterAddr[6];      //���ַ
  INT8U     copyDataType;      //������������(��ǰ���ݻ�����������)
  DATE_TIME copyTime;          //����ʱ��
  INT8U     copyItem;          //��ǰ��������
  INT8U     totalCopyItem;     //�ܵ�Ӧ������
  INT16U    hasData;           //�Ƿ��г���������
   
  INT8U     *energy;           //����ʾֵ���α�������ָ��
  INT8U     *reqAndReqTime;    //�輰����ʱ�仺��ָ��
  INT8U     *paraVariable;     //�������α���ָ��
  INT8U     *shiDuan;          //ʱ�λ���ʱ��
   
  INT8U     meterRecvBuf[2048];//���ջ���
  INT16U    recvFrameTail;     //����֡β
  INT16U    totalLenOfFrame;   //֡�ܳ���
   
  void (*send)(INT8U port,INT8U *pack,INT16U length);                   //��˿ڷ������ݺ���
  void (*save)(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len);  //���ݱ��溯��

};

//�ṹ - ��������֡�����ṹ
struct recvFrameStruct
{
	INT8U     C;                 //������
	INT8U     L;                 //�����򳤶�
	INT8U     DI[4];             //���ݱ�ʶ
	INT8U     *pData;            //�����ݿ�ʼָ��
	INT8U     loadLen;           //�����ɳ���
};

//�������弰����ֵ
//struct meterCopy multiCopy[4];           //��Ϊ����ͬʱ��4���˿ڵı�,���Զ���Ϊ����
struct meterCopy multiCopy[6];           //��Ϊ����ͬʱ��6���˿�(����Ϊ3��ͬʱ��)�ı�,���Զ���Ϊ����
	
//1.DL/T645-1997��Լ
#ifdef  PROTOCOL_645_1997
  //DL/T645-1997�������������ݱ�ʶת����  16��
  INT8U cmdDlt645LastMonth1997[TOTAL_CMD_LASTMONTH_645_1997][5] = {
	                        {0x94, 0x1f, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x94, 0x2f, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x1f, POSITIVE_NO_WORK_OFFSET&0xFF, POSITIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xFF, NEGTIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x3f, QUA1_NO_WORK_OFFSET&0xFF, QUA1_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x4f, QUA4_NO_WORK_OFFSET&0xFF, QUA4_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x5f, QUA2_NO_WORK_OFFSET&0xFF, QUA2_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0x95, 0x6f, QUA3_NO_WORK_OFFSET&0xFF, QUA3_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA4, 0x1f, REQ_POSITIVE_WORK_OFFSET&0xFF, REQ_POSITIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA4, 0x2f, REQ_NEGTIVE_WORK_OFFSET&0xFF, REQ_NEGTIVE_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA5, 0x1f, REQ_POSITIVE_NO_WORK_OFFSET&0xFF, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xA5, 0x2f, REQ_NEGTIVE_NO_WORK_OFFSET&0xFF, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x3f, QUA1_REQ_NO_WORK_OFFSET&0xFF, QUA1_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x4f, QUA4_REQ_NO_WORK_OFFSET&0xFF, QUA4_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x5f, QUA2_REQ_NO_WORK_OFFSET&0xFF, QUA2_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xA5, 0x6f, QUA3_REQ_NO_WORK_OFFSET&0xFF, QUA3_REQ_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB4, 0x1f, REQ_TIME_P_WORK_OFFSET&0xFF, REQ_TIME_P_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB4, 0x2f, REQ_TIME_N_WORK_OFFSET&0xFF, REQ_TIME_N_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB5, 0x1f, REQ_TIME_P_NO_WORK_OFFSET&0xFF, REQ_TIME_P_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        {0xB5, 0x2f, REQ_TIME_N_NO_WORK_OFFSET&0xFF, REQ_TIME_N_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x3f, QUA1_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA1_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x4f, QUA4_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA4_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x5f, QUA2_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA2_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1},
	                        //{0xB5, 0x6f, QUA3_REQ_TIME_NO_WORK_OFFSET&0xFF, QUA3_REQ_TIME_NO_WORK_OFFSET>>8&0xFF,0x1}
                          };
                          
  //DL/T645-1997����+����+�α���+ʱ�βα������������ݱ�ʶת����
  INT8U  cmdDlt645Current1997[TOTAL_CMD_CURRENT_645_1997][5] = {
	                        //���������� 26��
	                        {0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x2f, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x1f, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x3f, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x4f, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x5f, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x6f, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA0, 0x1f, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA0, 0x2f, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA1, 0x1f, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xA1, 0x2f, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x3f, QUA1_REQ_NO_WORK_OFFSET&0xff, QUA1_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x4f, QUA4_REQ_NO_WORK_OFFSET&0xff, QUA4_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x5f, QUA2_REQ_NO_WORK_OFFSET&0xff, QUA2_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xA1, 0x6f, QUA3_REQ_NO_WORK_OFFSET&0xff, QUA3_REQ_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB0, 0x1f, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB0, 0x2f, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB1, 0x1f, REQ_TIME_P_NO_WORK_OFFSET&0xff, REQ_TIME_P_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0xB1, 0x2f, REQ_TIME_N_NO_WORK_OFFSET&0xff, REQ_TIME_N_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x3f, QUA1_REQ_TIME_NO_WORK_OFFSET&0xff, QUA1_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x4f, QUA4_REQ_TIME_NO_WORK_OFFSET&0xff, QUA4_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x5f, QUA2_REQ_TIME_NO_WORK_OFFSET&0xff, QUA2_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        //{0xB1, 0x6f, QUA3_REQ_TIME_NO_WORK_OFFSET&0xff, QUA3_REQ_TIME_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x20, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        
	                        //�α�������
	                        {0xB6, 0x11, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x12, VOLTAGE_PHASE_B&0xFF, VOLTAGE_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x13, VOLTAGE_PHASE_C&0xFF, VOLTAGE_PHASE_C>>8&0xFF,0x1},
	                        {0xB6, 0x21, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x22, CURRENT_PHASE_B&0xFF, CURRENT_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x23, CURRENT_PHASE_C&0xFF, CURRENT_PHASE_C>>8&0xFF,0x1},
	                        {0xB6, 0x30, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x31, POWER_PHASE_A_WORK&0xFF, POWER_PHASE_A_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x32, POWER_PHASE_B_WORK&0xFF, POWER_PHASE_B_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x33, POWER_PHASE_C_WORK&0xFF, POWER_PHASE_C_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x40, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x41, POWER_PHASE_A_NO_WORK&0xFF, POWER_PHASE_A_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x42, POWER_PHASE_B_NO_WORK&0xFF, POWER_PHASE_B_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x43, POWER_PHASE_C_NO_WORK&0xFF, POWER_PHASE_C_NO_WORK>>8&0xFF,0x1},
	                        {0xB6, 0x50, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF,0x1},
	                        {0xB6, 0x51, FACTOR_PHASE_A&0xFF, FACTOR_PHASE_A>>8&0xFF,0x1},
	                        {0xB6, 0x52, FACTOR_PHASE_B&0xFF, FACTOR_PHASE_B>>8&0xFF,0x1},
	                        {0xB6, 0x53, FACTOR_PHASE_C&0xFF, FACTOR_PHASE_C>>8&0xFF,0x1},
	                        {0xB3, 0x10, PHASE_DOWN_TIMES&0xFF, PHASE_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x11, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x12, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x13, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF,0x1},
	                        {0xB3, 0x20, TOTAL_PHASE_DOWN_TIME&0xFF, TOTAL_PHASE_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x21, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x22, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x23, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF,0x1},
	                        {0xB3, 0x30, LAST_PHASE_DOWN_BEGIN&0xFF, LAST_PHASE_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x31, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x32, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x33, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF,0x1},
	                        {0xB3, 0x40, LAST_PHASE_DOWN_END&0xFF, LAST_PHASE_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x41, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x42, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF,0x1},
	                        {0xB3, 0x43, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF,0x1},
	                        {0xB2, 0x10, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x11, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x12, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF,0x1},
	                        {0xB2, 0x13, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF,0x1},
	                        {0xB2, 0x14, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF,0x1},
	                        {0xC0, 0x10, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF,0x1},
	                        {0xC0, 0x11, METER_TIME&0xFF, METER_TIME>>8&0xFF,0x1},
	                        {0xC0, 0x20, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF,0x1},
	                        {0xC0, 0x21, NET_STATUS_WORD&0xFF, NET_STATUS_WORD>>8&0xFF,0x1},
	                        {0xC0, 0x30, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF,0x1},
	                        {0xC0, 0x31, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF,0x1},
	                        {0xC0, 0x32, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF,0x1},
	                        {0xC1, 0x17, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF,0x1},
	                        //���¼���ʱ�βα���
	                        {0xC3, 0x10, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF,0x1},
	                        {0xC3, 0x11, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF,0x1},
	                        {0xC3, 0x12, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF,0x1},
	                        {0xC3, 0x13, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF,0x1},
	                        {0xC3, 0x14, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF,0x1},
	                        {0xC3, 0x2f, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF,0x1},
	                        
	                        //��������ʱ�α���ʼʱ�估���ʺ�
	                        {0xC3, 0x3f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x4f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x5f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x6f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x7f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x8f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0x9f, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        {0xC3, 0xaf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1}
	                        
	                        //�����Ǽ���ʱ��
	                        //{0xC3, 0xbf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xcf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xdf, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC3, 0xef, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF,0x1},
	                        //{0xC4, 0x11, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x12, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x13, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x14, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x15, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x16, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x17, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x18, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x19, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1a, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1b, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1c, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1d, JIA_RI_SHIDUAN&0xFF, JIA_RI_SHIDUAN>>8&0xFF,0x1},
	                        //{0xC4, 0x1e, ZHOUXIURI_SHIDUAN&0xFF, ZHOUXIURI_SHIDUAN>>8&0xFF,0x1}
                          };

  //DL/T645-1997����+����+�α���+ʱ�βα������������ݱ�ʶת����
  INT8U  cmdDlt645pnWorkNwork1997[TOTAL_CMD_PN_WORK_NOWORK_645_07][5] = {
	                        //���������� 6��
	                        {0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x2f, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x1f, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x91, 0x2f, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},
	                        {0x90, 0x20, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff,0x1}
	                        };

#endif  //PROTOCOL_645_1997

//2.�������ܵ��ܱ�DL/T645-2007��Լ
#ifdef  PROTOCOL_645_2007    
  //2.1 �������ܵ��ܱ�DL/T645-2007��һ���������������ݱ�ʶת����,26��
  INT8U cmdDlt645LastMonth2007[TOTAL_CMD_LASTMONTH_645_2007][6] = {
   {0x00, 0x01, 0xff, 0x01, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF},              //�����й�
   {0x00, 0x02, 0xff, 0x01, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF},                //�����й�
   
   {0x00, 0x01, 0x00, 0x01, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF},              //�����й���,2012-05-21,add
   {0x00, 0x02, 0x00, 0x01, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF},                //�����й���,2012-05-21,add
   
   {0x00, 0x03, 0xff, 0x01, POSITIVE_NO_WORK_OFFSET&0xFF, POSITIVE_NO_WORK_OFFSET>>8&0xFF},        //����޹�1
   {0x00, 0x04, 0xff, 0x01, NEGTIVE_NO_WORK_OFFSET&0xFF, NEGTIVE_NO_WORK_OFFSET>>8&0xFF},          //����޹�2
   {0x00, 0x05, 0xff, 0x01, QUA1_NO_WORK_OFFSET&0xFF, QUA1_NO_WORK_OFFSET>>8&0xFF},                //һ�����޹�
   {0x00, 0x08, 0xff, 0x01, QUA4_NO_WORK_OFFSET&0xFF, QUA4_NO_WORK_OFFSET>>8&0xFF},                //�������޹�
   {0x00, 0x06, 0xff, 0x01, QUA2_NO_WORK_OFFSET&0xFF, QUA2_NO_WORK_OFFSET>>8&0xFF},                //�������޹�
   {0x00, 0x07, 0xff, 0x01, QUA3_NO_WORK_OFFSET&0xFF, QUA3_NO_WORK_OFFSET>>8&0xFF},                //�������޹�
   {0x01, 0x01, 0xff, 0x01, REQ_POSITIVE_WORK_OFFSET&0xFF, REQ_POSITIVE_WORK_OFFSET>>8&0xFF},      //�����й��������������ʱ��
   {0x01, 0x02, 0xff, 0x01, REQ_NEGTIVE_WORK_OFFSET&0xFF, REQ_NEGTIVE_WORK_OFFSET>>8&0xFF},        //�����й��������������ʱ��
   {0x01, 0x03, 0xff, 0x01, REQ_POSITIVE_NO_WORK_OFFSET&0xFF, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xFF},//����޹�1�������������ʱ��
   {0x01, 0x04, 0xff, 0x01, REQ_NEGTIVE_NO_WORK_OFFSET&0xFF, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xFF},  //����޹�2�������������ʱ��
   {0x00, 0x85, 0x00, 0x01, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //ͭ���й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x86, 0x00, 0x01, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //�����й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x15, 0x00, 0x01, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A�������й�����(4Bytes)
   {0x00, 0x16, 0x00, 0x01, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A�෴���й�����(4Bytes)
   {0x00, 0x17, 0x00, 0x01, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�1����(4Bytes)
   {0x00, 0x18, 0x00, 0x01, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�2����(4Bytes)
   {0x00, 0x29, 0x00, 0x01, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B�������й�����(4Bytes)
   {0x00, 0x2A, 0x00, 0x01, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B�෴���й�����(4Bytes)
   {0x00, 0x2B, 0x00, 0x01, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�1����(4Bytes)
   {0x00, 0x2C, 0x00, 0x01, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�2����(4Bytes)
   {0x00, 0x3D, 0x00, 0x01, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C�������й�����(4Bytes)
   {0x00, 0x3E, 0x00, 0x01, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C�෴���й�����(4Bytes)
   {0x00, 0x3F, 0x00, 0x01, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�1����(4Bytes)
   {0x00, 0x40, 0x00, 0x01, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�2����(4Bytes)
   };
                          
  //2.2 �������ܵ��ܱ�DL/T645-2007����+����+�α���+ʱ�βα������������ݱ�ʶת����
 #ifdef DKY_SUBMISSION  //��Ӧ���Ժ�ͼ�
  INT8U  cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
   //2.2.1 ���� 22��(ȫ��4bytes,��97һ��)
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й��������ݿ�
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й��������ݿ�

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�������,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�������,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //����޹�1�������ݿ�
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //����޹�2�������ݿ�
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1�����޹��������ݿ�
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4�����޹��������ݿ�
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2�����޹��������ݿ�
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3�����޹��������ݿ�
   //{0x00, 0x85, 0x00, 0x00, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //ͭ���й��ܵ���ʾֵ(������,4bytes)
   //{0x00, 0x86, 0x00, 0x00, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //�����й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x15, 0x00, 0x00, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A�������й�����(4Bytes)
   {0x00, 0x16, 0x00, 0x00, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A�෴���й�����(4Bytes)
   {0x00, 0x17, 0x00, 0x00, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�1����(4Bytes)
   {0x00, 0x18, 0x00, 0x00, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�2����(4Bytes)
   {0x00, 0x29, 0x00, 0x00, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B�������й�����(4Bytes)
   {0x00, 0x2A, 0x00, 0x00, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B�෴���й�����(4Bytes)
   {0x00, 0x2B, 0x00, 0x00, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�1����(4Bytes)
   {0x00, 0x2C, 0x00, 0x00, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�2����(4Bytes)
   {0x00, 0x3D, 0x00, 0x00, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C�������й�����(4Bytes)
   {0x00, 0x3E, 0x00, 0x00, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C�෴���й�����(4Bytes)
   {0x00, 0x3F, 0x00, 0x00, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�1����(4Bytes)
   {0x00, 0x40, 0x00, 0x00, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�2����(4Bytes)
   
   //2.2.2 ��������������ʱ�� 4��(����3bytes+5bytes������ʱ��,97��3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //�����й��������������ʱ��
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //�����й��������������ʱ��
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//����޹�1�������������ʱ��
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //����޹�2�������������ʱ��
   
   //2.2.3 �������� 8��
   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //��ѹ���ݿ�(A,B,C���ѹ)(��2�ֽ�=97,����ʽ97��xxx��07��xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //�������ݿ�(A,B,C�����)(��3�ֽ�<>97,97�Ǹ�2�ֽ�xx.xx,07��xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //˲ʱ�й����ʿ�(��,A,B,C���й�����)(��3�ֽ�=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //˲ʱ�޹����ʿ�(��,A,B,C���޹�����)(��3�ֽ�xx.xxxx<>97��xx.xx)
   {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},          //˲ʱ���ڹ��ʿ�(��,A,B,C�����ڹ���)(��3�ֽ�xx.xxxx<>97û��)
   {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                  //����������(��,A,B,C�๦������)(��2�ֽ�=97,x.xxx)   
   {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},                        //������ݿ�(A,B,C�����)(��2�ֽ�,97��,x.xxx)
   {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},                //���ߵ���(3�ֽ�xxx.xxx<>97��2�ֽ�xx.xx)
   {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},                    //��ع���ʱ��(4�ֽ�NNNNNNNN<>97��3�ֽ�NNNNNN)

   //2.2.4.�¼���¼���� 26��
   //2.2.4-1.����ͳ������
   //2.2.4-1.1��Щ07��֧�ֹ�Լ,������4���
   //�ܶ������,���ۼ�ʱ��δ�ҵ�,�ѵ�Ҫ����(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x03, 0x04, 0x00, 0x00, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)
                                                                                                   //B������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)
                                                                                                   //C������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)

   //���һ�ζ��෢����ʼʱ�̼�����ʱ��δ�ҵ�,�ѵ�Ҫ����?(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   //{0x03, 0x04, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //��һ��A������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x03, 0x04, 0x02, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //��һ��B������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x03, 0x04, 0x03, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //��һ��C������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)

   //2.2.4-1.2 ��Щ��֧�ֱ����ļ�,����������12����������ͳ������
   //�ܶ������,���ۼ�ʱ��δ�ҵ�,�ѵ�Ҫ����(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x13, 0x01, 0x00, 0x01, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x02, 0x00, 0x01, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF},                  //B������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x03, 0x00, 0x01, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF},                  //B������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x01, 0x00, 0x02, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF},        //A���ۼ�ʱ��(�ۼ�3�ֽ�=97)
   {0x13, 0x02, 0x00, 0x02, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF},        //B���ۼ�ʱ��(�ۼ�3�ֽ�=97)
   {0x13, 0x03, 0x00, 0x02, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF},        //C���ۼ�ʱ��(�ۼ�3�ֽ�=97)

   //2.2.4-1.3 ���һ�ζ��෢����ʼʱ�̼�����ʱ��δ�ҵ�,�ѵ�Ҫ����?(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   
   //{0x13, 0x01, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //��һ��A������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x13, 0x02, 0x01, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //��һ��B������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x13, 0x03, 0x01, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //��һ��C������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x13, 0x01, 0x25, 0x01, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF},            //��һ��A������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x13, 0x02, 0x25, 0x01, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF},            //��һ��B������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   //{0x13, 0x03, 0x25, 0x01, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF},            //��һ��C������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   
   //2.2.4-2 ����ͳ����
   {0x03, 0x30, 0x00, 0x00, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF},                            //����ܴ���(3�ֽ�!=97,2�ֽ�)
   {0x03, 0x30, 0x00, 0x01, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF},                    //���һ�α��ʱ��(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x30, 0x01, 0x00, METER_CLEAR_TIMES&0xFF, METER_CLEAR_TIMES>>8&0xFF},                    //��������ܴ���(3�ֽ�!=97û��)
   {0x03, 0x30, 0x01, 0x01, LAST_METER_CLEAR_TIME&0xFF, LAST_METER_CLEAR_TIME>>8&0xFF},            //���һ�ε������ʱ��(ʱ��6�ֽ�<>97û��)
   {0x03, 0x30, 0x02, 0x00, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF},                        //������������ܴ���(3�ֽ�!=97,2�ֽ�)
   {0x03, 0x30, 0x02, 0x01, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF},              //���һ�������������ʱ��(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x30, 0x03, 0x00, EVENT_CLEAR_TIMES&0xFF, EVENT_CLEAR_TIMES>>8&0xFF},                    //�¼������ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x03, 0x01, EVENT_CLEAR_LAST_TIME&0xFF, EVENT_CLEAR_LAST_TIME>>8&0xFF},            //�¼�����ʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x04, 0x00, TIMING_TIMES&0xFF, TIMING_TIMES>>8&0xFF},                              //Уʱ�ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x04, 0x01, TIMING_LAST_TIME&0xFF, TIMING_LAST_TIME>>8&0xFF},                      //���һ��Уʱʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x05, 0x00, PERIOD_TIMES&0xFF, PERIOD_TIMES>>8&0xFF},                              //ʱ�α����ܴ���(3�ֽ�!=97,û��)
   //{0x03, 0x30, 0x05, 0x01, PERIOD_LAST_TIME&0xFF, PERIOD_LAST_TIME>>8&0xFF},                      //���һ��ʱ�α��̷���ʱ��(ʱ��6�ֽ�<>97,û��)
   //{0x03, 0x30, 0x0d, 0x00, OPEN_METER_COVER_TIMES&0xFF, OPEN_METER_COVER_TIMES>>8&0xFF},          //������ܴ���(2�ֽ�,97û��)
   //{0x03, 0x30, 0x0d, 0x01, LAST_OPEN_METER_COVER_TIME&0xFF, LAST_OPEN_METER_COVER_TIME>>8&0xFF},  //��һ�ο���Ƿ���ʱ��(6�ֽ�,97û��)
   
   //2.2.5 �α��� 9��
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                            //���ڼ��ܴ�(4�ֽ�=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},                                  //���ʱ��(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x09, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF},                            //�����(�й�)(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x0a, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF},                      //�����(�޹�)(3�ֽ�=97)
   //{0x04, 0x00, 0x04, 0x02, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF},                              //����(6�ֽ�=97)
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},                    //�������״̬��1��7(7*2<>97ֻ��2bytes)
   {0x04, 0x00, 0x0b, 0x01, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF},                            //ÿ�µ�1������
   //{0x04, 0x00, 0x0b, 0x02, AUTO_COPY_DAY_2&0xFF, AUTO_COPY_DAY_2>>8&0xFF},                        //ÿ�µ�2������
   //{0x04, 0x00, 0x0b, 0x03, AUTO_COPY_DAY_3&0xFF, AUTO_COPY_DAY_3>>8&0xFF},                        //ÿ�µ�3������
   
   //2.2.6 ���¼���ʱ�βα��� 6��
   //{0x04, 0x00, 0x02, 0x01, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF},                              //��ʱ����(1�ֽ�)
   //{0x04, 0x00, 0x02, 0x02, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF},                  //��ʱ�α���(1�ֽ�)
   //{0x04, 0x00, 0x02, 0x03, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF},                            //��ʱ����(1�ֽ�)
   //{0x04, 0x00, 0x02, 0x04, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF},                              //������(1�ֽ�)
   {0x04, 0x00, 0x02, 0x05, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF},                        //����������(1�ֽ�)   
   {0x04, 0x01, 0x00, 0x00, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF},                                  //��һ��ʱ��������(14*3)
   
   //2.2.7 �����ǵ�һ����ʱ�α���ʼʱ�估���ʺ� 1��
   {0x04, 0x01, 0x00, 0x01, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��1��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   };
	 
 #elif defined JF_MONITOR          //�������ʹ��
 
  INT8U cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
	 //2.2.1 ���� 6��(ȫ��4bytes,��97һ��)
	 {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},							//�����й�����
	 {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},								//�����й�����
 
	 {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},							//�����й�����ʾֵ��,2012-05-21,add
	 {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},								//�����й�����ʾֵ��,2012-05-21,add
 
	 {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},				//����޹�1����
	 {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},					//����޹�2����
	
	 //2.2.2 ��������������ʱ�� 4��(����3bytes+5bytes������ʱ��,97��3+4Bytes)
	 {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},			//�����й��������������ʱ��
	 {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},				//�����й��������������ʱ��
	 {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//����޹�1�������������ʱ��
	 {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},	//����޹�2�������������ʱ��
	 
	 //2.2.3 �������� 8��
	 {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},												//��ѹ���ݿ�(A,B,C���ѹ)(��2�ֽ�=97,����ʽ97��xxx��07��xxx.x)
	 {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},												//�������ݿ�(A,B,C�����)(��3�ֽ�<>97,97�Ǹ�2�ֽ�xx.xx,07��xxx.xxx)
	 {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},									//˲ʱ�й����ʿ�(��,A,B,C���й�����)(��3�ֽ�=97,xx.xxxx)
	 {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},						//˲ʱ�޹����ʿ�(��,A,B,C���޹�����)(��3�ֽ�xx.xxxx<>97��xx.xx)
	 {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},					//˲ʱ���ڹ��ʿ�(��,A,B,C�����ڹ���)(��3�ֽ�xx.xxxx<>97û��)
	 {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},									//����������(��,A,B,C�๦������)(��2�ֽ�=97,x.xxx)	 
	 {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},												//������ݿ�(A,B,C�����)(��2�ֽ�,97��,x.xxx)
	 {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},								//���ߵ���(3�ֽ�xxx.xxx<>97��2�ֽ�xx.xx)
	 {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},										//��ع���ʱ��(4�ֽ�NNNNNNNN<>97��3�ֽ�NNNNNN)
	
	 //2.2.5 �α��� 9��
	 {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},														//���ڼ��ܴ�(4�ֽ�=97)
	 {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},																	//���ʱ��(3�ֽ�=97)
	 {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},										//�������״̬��1��7(7*2<>97ֻ��2bytes)

	 };
  
 #else           //��ʽʹ�ó���
 
  INT8U  cmdDlt645Current2007[TOTAL_CMD_CURRENT_645_2007][6] = {
   //2.2.1 ���� 22��(ȫ��4bytes,��97һ��)
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�����
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�����

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�����ʾֵ��,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�����ʾֵ��,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //����޹�1����
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //����޹�2����
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1�����޹�����
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4�����޹�����
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2�����޹�����
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3�����޹�����
   {0x00, 0x85, 0x00, 0x00, COPPER_LOSS_TOTAL_OFFSET&0xff, COPPER_LOSS_TOTAL_OFFSET>>8&0xff},      //ͭ���й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x86, 0x00, 0x00, IRON_LOSS_TOTAL_OFFSET&0xff, IRON_LOSS_TOTAL_OFFSET>>8&0xff},          //�����й��ܵ���ʾֵ(������,4bytes)
   {0x00, 0x15, 0x00, 0x00, POSITIVE_WORK_A_OFFSET&0xff, POSITIVE_WORK_A_OFFSET>>8&0xff},          //A�������й�����(4Bytes)
   {0x00, 0x16, 0x00, 0x00, NEGTIVE_WORK_A_OFFSET&0xff, NEGTIVE_WORK_A_OFFSET>>8&0xff},            //A�෴���й�����(4Bytes)
   {0x00, 0x17, 0x00, 0x00, COMB1_NO_WORK_A_OFFSET&0xff, COMB1_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�1����(4Bytes)
   {0x00, 0x18, 0x00, 0x00, COMB2_NO_WORK_A_OFFSET&0xff, COMB2_NO_WORK_A_OFFSET>>8&0xff},          //A������޹�2����(4Bytes)
   {0x00, 0x29, 0x00, 0x00, POSITIVE_WORK_B_OFFSET&0xff, POSITIVE_WORK_B_OFFSET>>8&0xff},          //B�������й�����(4Bytes)
   {0x00, 0x2A, 0x00, 0x00, NEGTIVE_WORK_B_OFFSET&0xff, NEGTIVE_WORK_B_OFFSET>>8&0xff},            //B�෴���й�����(4Bytes)
   {0x00, 0x2B, 0x00, 0x00, COMB1_NO_WORK_B_OFFSET&0xff, COMB1_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�1����(4Bytes)
   {0x00, 0x2C, 0x00, 0x00, COMB2_NO_WORK_B_OFFSET&0xff, COMB2_NO_WORK_B_OFFSET>>8&0xff},          //B������޹�2����(4Bytes)
   {0x00, 0x3D, 0x00, 0x00, POSITIVE_WORK_C_OFFSET&0xff, POSITIVE_WORK_C_OFFSET>>8&0xff},          //C�������й�����(4Bytes)
   {0x00, 0x3E, 0x00, 0x00, NEGTIVE_WORK_C_OFFSET&0xff, NEGTIVE_WORK_C_OFFSET>>8&0xff},            //C�෴���й�����(4Bytes)
   {0x00, 0x3F, 0x00, 0x00, COMB1_NO_WORK_C_OFFSET&0xff, COMB1_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�1����(4Bytes)
   {0x00, 0x40, 0x00, 0x00, COMB2_NO_WORK_C_OFFSET&0xff, COMB2_NO_WORK_C_OFFSET>>8&0xff},          //C������޹�2����(4Bytes)
   
   //2.2.2 ��������������ʱ�� 4��(����3bytes+5bytes������ʱ��,97��3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //�����й��������������ʱ��
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //�����й��������������ʱ��
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//����޹�1�������������ʱ��
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //����޹�2�������������ʱ��
   
   //2.2.3 �������� 8��
   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //��ѹ���ݿ�(A,B,C���ѹ)(��2�ֽ�=97,����ʽ97��xxx��07��xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //�������ݿ�(A,B,C�����)(��3�ֽ�<>97,97�Ǹ�2�ֽ�xx.xx,07��xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //˲ʱ�й����ʿ�(��,A,B,C���й�����)(��3�ֽ�=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //˲ʱ�޹����ʿ�(��,A,B,C���޹�����)(��3�ֽ�xx.xxxx<>97��xx.xx)
   {0x02, 0x05, 0xff, 0x00, POWER_INSTANT_APPARENT&0xFF, POWER_INSTANT_APPARENT>>8&0xFF},          //˲ʱ���ڹ��ʿ�(��,A,B,C�����ڹ���)(��3�ֽ�xx.xxxx<>97û��)
   {0x02, 0x06, 0xff, 0x00, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                  //����������(��,A,B,C�๦������)(��2�ֽ�=97,x.xxx)   
   {0x02, 0x07, 0xff, 0x00, PHASE_ANGLE_V_A&0xFF, PHASE_ANGLE_V_A>>8&0xFF},                        //������ݿ�(A,B,C�����)(��2�ֽ�,97��,x.xxx)
   {0x02, 0x80, 0x00, 0x01, ZERO_SERIAL_CURRENT&0xFF, ZERO_SERIAL_CURRENT>>8&0xFF},                //���ߵ���(3�ֽ�xxx.xxx<>97��2�ֽ�xx.xx)
   {0x02, 0x80, 0x00, 0x0a, BATTERY_WORK_TIME&0xFF, BATTERY_WORK_TIME>>8&0xFF},                    //��ع���ʱ��(4�ֽ�NNNNNNNN<>97��3�ֽ�NNNNNN)

   //2.2.4.�¼���¼���� 26��
   //2.2.4-1.����ͳ������
   //2.2.4-1.1��Щ07��֧�ֹ�Լ,������4���
   //�ܶ������,���ۼ�ʱ��δ�ҵ�,�ѵ�Ҫ����(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x03, 0x04, 0x00, 0x00, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)
                                                                                                   //B������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)
                                                                                                   //C������ܴ���,���ۼ�ʱ��(����3�ֽ�<>97����2�ֽ�,�ۼ�3�ֽ�=97)

   //���һ�ζ��෢����ʼʱ�̼�����ʱ��δ�ҵ�,�ѵ�Ҫ����?(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x03, 0x04, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //��һ��A������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x04, 0x02, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //��һ��B������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x04, 0x03, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //��һ��C������¼(��ʼʱ�̼�����ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)

   //2.2.4-1.2 ��Щ��֧�ֱ����ļ�,����������12����������ͳ������
   //�ܶ������,���ۼ�ʱ��δ�ҵ�,�ѵ�Ҫ����(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   {0x13, 0x01, 0x00, 0x01, PHASE_A_DOWN_TIMES&0xFF, PHASE_A_DOWN_TIMES>>8&0xFF},                  //A������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x02, 0x00, 0x01, PHASE_B_DOWN_TIMES&0xFF, PHASE_B_DOWN_TIMES>>8&0xFF},                  //B������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x03, 0x00, 0x01, PHASE_C_DOWN_TIMES&0xFF, PHASE_C_DOWN_TIMES>>8&0xFF},                  //B������ܴ���(����3�ֽ�<>97����2�ֽ�)
   {0x13, 0x01, 0x00, 0x02, TOTAL_PHASE_A_DOWN_TIME&0xFF, TOTAL_PHASE_A_DOWN_TIME>>8&0xFF},        //A���ۼ�ʱ��(�ۼ�3�ֽ�=97)
   {0x13, 0x02, 0x00, 0x02, TOTAL_PHASE_B_DOWN_TIME&0xFF, TOTAL_PHASE_B_DOWN_TIME>>8&0xFF},        //B���ۼ�ʱ��(�ۼ�3�ֽ�=97)
   {0x13, 0x03, 0x00, 0x02, TOTAL_PHASE_C_DOWN_TIME&0xFF, TOTAL_PHASE_C_DOWN_TIME>>8&0xFF},        //C���ۼ�ʱ��(�ۼ�3�ֽ�=97)

   //2.2.4-1.3 ���һ�ζ��෢����ʼʱ�̼�����ʱ��δ�ҵ�,�ѵ�Ҫ����?(10-07-05,���붫�������Ա�,ȷʵ��Ҫ����)
   
   {0x13, 0x01, 0x01, 0x01, LAST_PHASE_A_DOWN_BEGIN&0xFF, LAST_PHASE_A_DOWN_BEGIN>>8&0xFF},        //��һ��A������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x02, 0x01, 0x01, LAST_PHASE_B_DOWN_BEGIN&0xFF, LAST_PHASE_B_DOWN_BEGIN>>8&0xFF},        //��һ��B������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x03, 0x01, 0x01, LAST_PHASE_C_DOWN_BEGIN&0xFF, LAST_PHASE_C_DOWN_BEGIN>>8&0xFF},        //��һ��C������¼(��ʼʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x01, 0x25, 0x01, LAST_PHASE_A_DOWN_END&0xFF, LAST_PHASE_A_DOWN_END>>8&0xFF},            //��һ��A������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x02, 0x25, 0x01, LAST_PHASE_B_DOWN_END&0xFF, LAST_PHASE_B_DOWN_END>>8&0xFF},            //��һ��B������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x13, 0x03, 0x25, 0x01, LAST_PHASE_C_DOWN_END&0xFF, LAST_PHASE_C_DOWN_END>>8&0xFF},            //��һ��C������¼(������ʱ��)(ʱ��6�ֽ�<>97,4�ֽ�)
   
   //2.2.4-2 ����ͳ����
   {0x03, 0x30, 0x00, 0x00, PROGRAM_TIMES&0xFF, PROGRAM_TIMES>>8&0xFF},                            //����ܴ���(3�ֽ�!=97,2�ֽ�)
   {0x03, 0x30, 0x00, 0x01, LAST_PROGRAM_TIME&0xFF, LAST_PROGRAM_TIME>>8&0xFF},                    //���һ�α��ʱ��(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x30, 0x01, 0x00, METER_CLEAR_TIMES&0xFF, METER_CLEAR_TIMES>>8&0xFF},                    //��������ܴ���(3�ֽ�!=97û��)
   {0x03, 0x30, 0x01, 0x01, LAST_METER_CLEAR_TIME&0xFF, LAST_METER_CLEAR_TIME>>8&0xFF},            //���һ�ε������ʱ��(ʱ��6�ֽ�<>97û��)
   {0x03, 0x30, 0x02, 0x00, UPDATA_REQ_TIME&0xFF, UPDATA_REQ_TIME>>8&0xFF},                        //������������ܴ���(3�ֽ�!=97,2�ֽ�)
   {0x03, 0x30, 0x02, 0x01, LAST_UPDATA_REQ_TIME&0xFF, LAST_UPDATA_REQ_TIME>>8&0xFF},              //���һ�������������ʱ��(ʱ��6�ֽ�<>97,4�ֽ�)
   {0x03, 0x30, 0x03, 0x00, EVENT_CLEAR_TIMES&0xFF, EVENT_CLEAR_TIMES>>8&0xFF},                    //�¼������ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x03, 0x01, EVENT_CLEAR_LAST_TIME&0xFF, EVENT_CLEAR_LAST_TIME>>8&0xFF},            //�¼�����ʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x04, 0x00, TIMING_TIMES&0xFF, TIMING_TIMES>>8&0xFF},                              //Уʱ�ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x04, 0x01, TIMING_LAST_TIME&0xFF, TIMING_LAST_TIME>>8&0xFF},                      //���һ��Уʱʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x05, 0x00, PERIOD_TIMES&0xFF, PERIOD_TIMES>>8&0xFF},                              //ʱ�α����ܴ���(3�ֽ�!=97,û��)
   {0x03, 0x30, 0x05, 0x01, PERIOD_LAST_TIME&0xFF, PERIOD_LAST_TIME>>8&0xFF},                      //���һ��ʱ�α��̷���ʱ��(ʱ��6�ֽ�<>97,û��)
   {0x03, 0x30, 0x0d, 0x00, OPEN_METER_COVER_TIMES&0xFF, OPEN_METER_COVER_TIMES>>8&0xFF},          //������ܴ���(2�ֽ�,97û��)
   {0x03, 0x30, 0x0d, 0x01, LAST_OPEN_METER_COVER_TIME&0xFF, LAST_OPEN_METER_COVER_TIME>>8&0xFF},  //��һ�ο���Ƿ���ʱ��(6�ֽ�,97û��)
   
   //2.2.5 �α��� 9��
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                            //���ڼ��ܴ�(4�ֽ�=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME&0xFF, METER_TIME>>8&0xFF},                                  //���ʱ��(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x09, CONSTANT_WORK&0xFF, CONSTANT_WORK>>8&0xFF},                            //�����(�й�)(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x0a, CONSTANT_NO_WORK&0xFF, CONSTANT_NO_WORK>>8&0xFF},                      //�����(�޹�)(3�ֽ�=97)
   {0x04, 0x00, 0x04, 0x02, METER_NUMBER&0xFF, METER_NUMBER>>8&0xFF},                              //����(6�ֽ�=97)
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD&0xFF, METER_STATUS_WORD>>8&0xFF},                    //�������״̬��1��7(7*2<>97ֻ��2bytes)
   {0x04, 0x00, 0x0b, 0x01, AUTO_COPY_DAY&0xFF, AUTO_COPY_DAY>>8&0xFF},                            //ÿ�µ�1������
   {0x04, 0x00, 0x0b, 0x02, AUTO_COPY_DAY_2&0xFF, AUTO_COPY_DAY_2>>8&0xFF},                        //ÿ�µ�2������
   {0x04, 0x00, 0x0b, 0x03, AUTO_COPY_DAY_3&0xFF, AUTO_COPY_DAY_3>>8&0xFF},                        //ÿ�µ�3������
   
   //2.2.6 ���¼���ʱ�βα��� 6��
   {0x04, 0x00, 0x02, 0x01, YEAR_SHIQU_P&0xFF, YEAR_SHIQU_P>>8&0xFF},                              //��ʱ����(1�ֽ�)
   {0x04, 0x00, 0x02, 0x02, DAY_SHIDUAN_BIAO_Q&0xFF, DAY_SHIDUAN_BIAO_Q>>8&0xFF},                  //��ʱ�α���(1�ֽ�)
   {0x04, 0x00, 0x02, 0x03, DAY_SHIDUAN_M&0xFF, DAY_SHIDUAN_M>>8&0xFF},                            //��ʱ����(1�ֽ�)
   {0x04, 0x00, 0x02, 0x04, NUM_TARIFF_K&0xFF, NUM_TARIFF_K>>8&0xFF},                              //������(1�ֽ�)
   {0x04, 0x00, 0x02, 0x05, NUM_OF_JIA_RI_N&0xFF, NUM_OF_JIA_RI_N>>8&0xFF},                        //����������(1�ֽ�)   
   {0x04, 0x01, 0x00, 0x00, YEAR_SHIDU&0xFF, YEAR_SHIDU>>8&0xFF},                                  //��һ��ʱ��������(14*3)
   
   //2.2.7 �����ǵ�һ����ʱ�α���ʼʱ�估���ʺ� 8��
   {0x04, 0x01, 0x00, 0x01, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��1��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x02, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��2��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x03, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��3��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x04, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��4��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x05, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��5��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x06, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��6��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x07, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��7��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   {0x04, 0x01, 0x00, 0x08, DAY_SHIDUAN_BIAO&0xFF, DAY_SHIDUAN_BIAO>>8&0xFF},                      //��8��ʱ�α�����(��ǰ��ǰ10��ʱ����ʼʱ��ͷ��ʺ�)
   
   //2.2.8 �ѿر����/����/������Ϣ
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME&0xFF, LAST_JUMPED_GATE_TIME>>8&0xFF},            //��һ����բ����ʱ��(6�ֽ�,97û��)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME&0xFF, LAST_CLOSED_GATE_TIME>>8&0xFF},            //��һ�κ�բ����ʱ��(6�ֽ�,97û��)
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME&0xFF, CHARGE_TOTAL_TIME>>8&0xFF},                    //��1�ι�����ܹ������(2�ֽ�)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY&0xFF, CHARGE_REMAIN_MONEY>>8&0xFF},                //��ǰʣ����(4�ֽ�,97û��)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY&0xFF, CHARGE_TOTAL_MONEY>>8&0xFF},                  //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY&0xFF, CHARGE_REMAIN_QUANTITY>>8&0xFF},          //��ǰʣ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY&0xFF, CHARGE_OVERDRAFT_QUANTITY>>8&0xFF},    //��ǰ͸֧����(4�ֽ�,97û��)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY&0xFF, CHARGE_TOTAL_QUANTITY>>8&0xFF},            //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT&0xFF, CHARGE_OVERDRAFT_LIMIT>>8&0xFF},          //͸֧������ֵ(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY&0xFF, CHARGE_ALARM_QUANTITY>>8&0xFF},            //��������1��ֵ
   };
 #endif
  
  //2.3 �����(���ܱ�/Զ�̷ѿر�/���طѿر�)��һ�ն�������(DL/T645-2007) ���������ݱ�ʶת����
  INT8U cmdDlt645LastDay2007[TOTAL_CMD_LASTDAY_645_2007][6] = {
   {0x05, 0x06, 0x00, 0x01, DAY_FREEZE_TIME_FLAG_T&0xFF, DAY_FREEZE_TIME_FLAG_T>>8&0xFF},          //��һ���ն���ʱ��
   {0x05, 0x06, 0x01, 0x01, POSITIVE_WORK_OFFSET&0xFF, POSITIVE_WORK_OFFSET>>8&0xFF},              //�����й�
   {0x05, 0x06, 0x02, 0x01, NEGTIVE_WORK_OFFSET&0xFF, NEGTIVE_WORK_OFFSET>>8&0xFF},                //�����й�
   {0x05, 0x06, 0x03, 0x01, POSITIVE_NO_WORK_OFFSET&0xFF, POSITIVE_NO_WORK_OFFSET>>8&0xFF},        //����޹�1
   {0x05, 0x06, 0x04, 0x01, NEGTIVE_NO_WORK_OFFSET&0xFF, NEGTIVE_NO_WORK_OFFSET>>8&0xFF},          //����޹�2
   {0x05, 0x06, 0x05, 0x01, QUA1_NO_WORK_OFFSET&0xFF, QUA1_NO_WORK_OFFSET>>8&0xFF},                //һ�����޹�
   {0x05, 0x06, 0x06, 0x01, QUA2_NO_WORK_OFFSET&0xFF, QUA2_NO_WORK_OFFSET>>8&0xFF},                //�������޹�
   {0x05, 0x06, 0x07, 0x01, QUA3_NO_WORK_OFFSET&0xFF, QUA3_NO_WORK_OFFSET>>8&0xFF},                //�������޹�
   {0x05, 0x06, 0x08, 0x01, QUA4_NO_WORK_OFFSET&0xFF, QUA4_NO_WORK_OFFSET>>8&0xFF},                //�������޹�
   {0x05, 0x06, 0x09, 0x01, REQ_POSITIVE_WORK_OFFSET&0xFF, REQ_POSITIVE_WORK_OFFSET>>8&0xFF},      //�����й��������������ʱ��
   {0x05, 0x06, 0x0a, 0x01, REQ_NEGTIVE_WORK_OFFSET&0xFF, REQ_NEGTIVE_WORK_OFFSET>>8&0xFF},        //�����й��������������ʱ��
   };
   
  //2.4 �����(���ܱ�/Զ�̷ѿر�/���طѿر�)��һ�ն�������(DL/T645-2007) ���������뵥Ԫ��ʶ(DI)ת����
  INT8U single2007LastDay[TOTAL_CMD_LASTDAY_SINGLE_07][6] = {
   {0x05, 0x06, 0x00, 0x01, DAY_FREEZE_TIME_FLAG_S&0xff,  DAY_FREEZE_TIME_FLAG_S>>8&0xff},         //(��1��)�ն���ʱ��
   {0x05, 0x06, 0x01, 0x01, POSITIVE_WORK_OFFSET_S&0xff,  POSITIVE_WORK_OFFSET_S>>8&0xff},         //(��1��)�ն��������й�����ʾֵ(�ܼ�������)
   {0x05, 0x06, 0x02, 0x01, NEGTIVE_WORK_OFFSET_S&0xff, NEGTIVE_WORK_OFFSET_S>>8&0xff},            //(��1��)�ն��ᷴ���й�����ʾֵ(�ܼ�������)
	 };
#endif  //PROTOCOL_645_2007

//3.����97��
#ifdef  PROTOCOL_SINGLE_PHASE_97
  //�����(DL/T645-1997)���������뵥Ԫ��ʶ(DI)ת����
  INT8U single1997[TOTAL_CMD_SINGLE_645_97][5] = {
	 //{0x90, 0x1f, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},                    //�����й��ܼ�������
	 {0x90, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},                      //�����й���ʾֵ
	 //{0x90, 0x2f, NEGTIVE_WORK_OFFSET_X&0xff, NEGTIVE_WORK_OFFSET_X>>8&0xff,0x1},                  //�����й��ܼ�������
	 //{0x90, 0x20, NEGTIVE_WORK_OFFSET_X&0xff, NEGTIVE_WORK_OFFSET_X>>8&0xff,0x1},                  //�����й���ʾֵ
	 };
  INT8U single1997LastDay[TOTAL_CMD_SINGLE_645_97][5] = {
	 {0x9A, 0x10, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff,0x1},              
	 };
#endif

//4.������ܱ�(���ܱ�/���طѿ�/Զ�̷ѿ�)07��Լ - ʵʱ�ɼ� ���������뵥Ԫ��ʶDIת����
#if  PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
  INT8U single2007[TOTAL_CMD_SINGLE_LOCAL_CTRL_07][6] = {
   //4.1 �������й�����ʾֵ
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET_S&0xff, POSITIVE_WORK_OFFSET_S>>8&0xff},          //��ǰ�����й�����ʾֵ(�ܼ�������)
   
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET_S&0xff, NEGTIVE_WORK_OFFSET_S>>8&0xff},            //��ǰ�����й�����ʾֵ(�ܼ�������)
   
   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET_S&0xff, POSITIVE_WORK_OFFSET_S>>8&0xff},          //��ǰ�����й�����ʾֵ��, 2012-5-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET_S&0xff, NEGTIVE_WORK_OFFSET_S>>8&0xff},            //��ǰ�����й�����ʾֵ��, 2012-5-21,add
   
   //4.2 �������״̬��
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD_S&0xFF, METER_STATUS_WORD_S>>8&0xFF},                //�������״̬��1��7(7*2<>97ֻ��2bytes)
   
   //4.3 ���ܱ����ڡ��ܴμ�ʱ��
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK_S&0xFF, DATE_AND_WEEK_S>>8&0xFF},                        //���ڼ��ܴ�(4�ֽ�=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME_S&0xFF, METER_TIME_S>>8&0xFF},                              //���ʱ��(3�ֽ�=97)

   //4.4 ���ܱ�Զ�̿���ͨ�ϵ�״̬����¼
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME_S&0xFF, LAST_JUMPED_GATE_TIME_S>>8&0xFF},        //��һ����բ����ʱ��(6�ֽ�,97û��)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME_S&0xFF, LAST_CLOSED_GATE_TIME_S>>8&0xFF},        //��һ�κ�բ����ʱ��(6�ֽ�,97û��)
   
   //4.5 ���ܱ����õ���Ϣ
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME_S&0xFF, CHARGE_TOTAL_TIME_S>>8&0xFF},                //��1�ι�����ܹ������(2�ֽ�)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY_S&0xFF, CHARGE_REMAIN_MONEY_S>>8&0xFF},            //��ǰʣ����(4�ֽ�,97û��)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY_S&0xFF, CHARGE_TOTAL_MONEY_S>>8&0xFF},              //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY_S&0xFF, CHARGE_REMAIN_QUANTITY_S>>8&0xFF},      //��ǰʣ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY_S&0xFF, CHARGE_OVERDRAFT_QUANTITY_S>>8&0xFF},//��ǰ͸֧����(4�ֽ�,97û��)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY_S&0xFF, CHARGE_TOTAL_QUANTITY_S>>8&0xFF},        //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT_S&0xFF, CHARGE_OVERDRAFT_LIMIT_S>>8&0xFF},      //͸֧������ֵ(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY_S&0xFF, CHARGE_ALARM_QUANTITY_S>>8&0xFF},        //��������1��ֵ
	 };
#endif

//5.�����(�������ܱ�/���طѿ�/Զ�̷ѿ�)07��Լ - ʵʱ�ɼ� ���������뵥Ԫ��ʶDIת����
#if PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
  INT8U three2007[TOTAL_CMD_THREE_LOCAL_CTRL_07][6] = {
   //5.1 ������ʾֵ
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //��ǰ�����й�����ʾֵ(�ܼ�������)
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //��ǰ�����й�����ʾֵ(�ܼ�������)

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //��ǰ�����й�����ʾֵ��,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //��ǰ�����й�����ʾֵ��,2012-05-21,add

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //����޹�1����
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},          //����޹�2����
   {0x00, 0x05, 0xff, 0x00, QUA1_NO_WORK_OFFSET&0xff, QUA1_NO_WORK_OFFSET>>8&0xff},                //1�����޹�����
   {0x00, 0x08, 0xff, 0x00, QUA4_NO_WORK_OFFSET&0xff, QUA4_NO_WORK_OFFSET>>8&0xff},                //4�����޹�����
   {0x00, 0x06, 0xff, 0x00, QUA2_NO_WORK_OFFSET&0xff, QUA2_NO_WORK_OFFSET>>8&0xff},                //2�����޹�����
   {0x00, 0x07, 0xff, 0x00, QUA3_NO_WORK_OFFSET&0xff, QUA3_NO_WORK_OFFSET>>8&0xff},                //3�����޹�����   

   //5.2 �������״̬��
   {0x04, 0x00, 0x05, 0xFF, METER_STATUS_WORD_T&0xFF, METER_STATUS_WORD_T>>8&0xFF},                //�������״̬��1��7(7*2<>97ֻ��2bytes)

   //5.3 ��������������ʱ�� 4��(����3bytes+5bytes������ʱ��,97��3+4Bytes)
   {0x01, 0x01, 0xff, 0x00, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},      //�����й��������������ʱ��
   {0x01, 0x02, 0xff, 0x00, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},        //�����й��������������ʱ��
   {0x01, 0x03, 0xff, 0x00, REQ_POSITIVE_NO_WORK_OFFSET&0xff, REQ_POSITIVE_NO_WORK_OFFSET>>8&0xff},//����޹�1�������������ʱ��
   {0x01, 0x04, 0xff, 0x00, REQ_NEGTIVE_NO_WORK_OFFSET&0xff, REQ_NEGTIVE_NO_WORK_OFFSET>>8&0xff},  //����޹�2�������������ʱ��
   
   //5.4 ���ܱ����ڡ��ܴμ�ʱ��
   {0x04, 0x00, 0x01, 0x01, DATE_AND_WEEK_T&0xFF, DATE_AND_WEEK_T>>8&0xFF},                        //���ڼ��ܴ�(4�ֽ�=97)
   {0x04, 0x00, 0x01, 0x02, METER_TIME_T&0xFF, METER_TIME_T>>8&0xFF},                              //���ʱ��(3�ֽ�=97)

   //5.5 ���ܱ�Զ�̿���ͨ�ϵ�״̬����¼
   {0x1e, 0x00, 0x01, 0x01, LAST_JUMPED_GATE_TIME_T&0xFF, LAST_JUMPED_GATE_TIME_T>>8&0xFF},        //��һ����բ����ʱ��(6�ֽ�,97û��)
   {0x1d, 0x00, 0x01, 0x01, LAST_CLOSED_GATE_TIME_T&0xFF, LAST_CLOSED_GATE_TIME_T>>8&0xFF},        //��һ�κ�բ����ʱ��(6�ֽ�,97û��)
   
   //5.6 ���ܱ����õ���Ϣ
   {0x03, 0x33, 0x02, 0x01, CHARGE_TOTAL_TIME_T&0xFF, CHARGE_TOTAL_TIME_T>>8&0xFF},                //��1�ι�����ܹ������(2�ֽ�)
   {0x00, 0x90, 0x02, 0x00, CHARGE_REMAIN_MONEY_T&0xFF, CHARGE_REMAIN_MONEY_T>>8&0xFF},            //��ǰʣ����(4�ֽ�,97û��)
   {0x03, 0x33, 0x06, 0x01, CHARGE_TOTAL_MONEY_T&0xFF, CHARGE_TOTAL_MONEY_T>>8&0xFF},              //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x00, CHARGE_REMAIN_QUANTITY_T&0xFF, CHARGE_REMAIN_QUANTITY_T>>8&0xFF},      //��ǰʣ�����(4�ֽ�,97û��)
   {0x00, 0x90, 0x01, 0x01, CHARGE_OVERDRAFT_QUANTITY_T&0xFF, CHARGE_OVERDRAFT_QUANTITY_T>>8&0xFF},//��ǰ͸֧����(4�ֽ�,97û��)
   {0x03, 0x32, 0x06, 0x01, CHARGE_TOTAL_QUANTITY_T&0xFF, CHARGE_TOTAL_QUANTITY_T>>8&0xFF},        //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x04, CHARGE_OVERDRAFT_LIMIT_T&0xFF, CHARGE_OVERDRAFT_LIMIT_T>>8&0xFF},      //͸֧������ֵ(4�ֽ�,97û��)
   {0x04, 0x00, 0x0f, 0x01, CHARGE_ALARM_QUANTITY_T&0xFF, CHARGE_ALARM_QUANTITY_T>>8&0xFF},        //��������1��ֵ
	 };
#endif

//6.07��Լ���ص��û�
#ifdef PROTOCOL_KEY_HOUSEHOLD_2007
  INT8U keyHousehold2007[TOTAL_CMD_KEY_2007][6] = {
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�����
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�����

   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�������,2012-05-21,add
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�������,2012-05-21,add

   {0x02, 0x01, 0xff, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                        //��ѹ���ݿ�(A,B,C���ѹ)(��2�ֽ�=97,����ʽ97��xxx��07��xxx.x)
   {0x02, 0x02, 0xff, 0x00, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                        //�������ݿ�(A,B,C�����)(��3�ֽ�<>97,97�Ǹ�2�ֽ�xx.xx,07��xxx.xxx)
   {0x02, 0x03, 0xff, 0x00, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                  //˲ʱ�й����ʿ�(��,A,B,C���й�����)(��3�ֽ�=97,xx.xxxx)
   {0x02, 0x04, 0xff, 0x00, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},            //˲ʱ�޹����ʿ�(��,A,B,C���޹�����)(��3�ֽ�xx.xxxx<>97��xx.xx)
  };
#endif

//7.97�㳭��Լ
  INT8U dotCopy1997[3][6] = {
	 {0x90, 0x10,  0,  0, 0x1},                                                                      //�����й���ʾֵ
	 {0xC0, 0x11,  2, 20, 0x1},                                                                      //���ʱ��(3�ֽ�)
	 {0xC0, 0x10, 23, 24, 0x1},                                                                      //���ڼ��ܴ�(4�ֽ�)
  };
  
//8.07�㳭��Լ
  INT8U dotCopy2007[3][6] = {
   #ifdef DKY_SUBMISSION
    {0x00, 0x01, 0xff, 0x00,  0, 0},                                                               //�����й�����
   #else
    {0x00, 0x01, 0x00, 0x00,  0, 0},                                                               //�����й�����,2012-5-21,modify
   #endif
   {0x04, 0x00, 0x01, 0x02, 20, 0},                                                                //���ʱ��(3�ֽ�=97)
   {0x04, 0x00, 0x01, 0x01, 23, 0},                                                                //���ڼ��ܴ�(4�ֽ�=97)
  };

//9.07�����㶳�����ݿ�
  INT8U hourFreeze2007[24][6] = {
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
   {0x05, 0x04, 0xff, 0x01,  0, 0},                                                                //���㶳�����ݿ�
  };

  INT8U  cmdDlt645pnWorkNwork2007[TOTAL_CMD_PN_WORK_NOWORK_645_07][6] = {
   //���� 6��
   {0x00, 0x01, 0xff, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й��������ݿ�
   {0x00, 0x02, 0xff, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й��������ݿ�
   {0x00, 0x01, 0x00, 0x00, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},              //�����й�������
   {0x00, 0x02, 0x00, 0x00, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                //�����й�������

   {0x00, 0x03, 0xff, 0x00, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},        //����޹�1����
   {0x00, 0x04, 0xff, 0x00, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff}           //����޹�2����
   };

//����(EDMI��ԼCommand Line)��
#ifdef PROTOCOL_EDMI_GROUP
  //��������
  INT8U  cmdEdmiLastMonth[TOTAL_COMMAND_LASTMONTH_EDMI][4] = {
   //{�Ĵ������ֽ�,�Ĵ������ֽ�,�����ص����ݴ洢ƫ��}
   {0x00, 0x00, 0, 0},                                                                             //��������ģʽ
   {0x00, 0x00, 0, 0},                                                                             //��������
   
   {0x01, 0x49, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},                          //�����й�������
   {0x01, 0x40, (POSITIVE_WORK_OFFSET+4)&0xff, (POSITIVE_WORK_OFFSET+4)>>8&0xff},                  //�����й����ܷ�
   {0x01, 0x41, (POSITIVE_WORK_OFFSET+8)&0xff, (POSITIVE_WORK_OFFSET+8)>>8&0xff},                  //�����й�����ƽ
   {0x01, 0x42, (POSITIVE_WORK_OFFSET+12)&0xff, (POSITIVE_WORK_OFFSET+12)>>8&0xff},                //�����й����ܹ�
   {0x01, 0x43, (POSITIVE_WORK_OFFSET+16)&0xff, (POSITIVE_WORK_OFFSET+16)>>8&0xff},                //�����й����ܼ��

   {0x00, 0x49, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                            //�����й�������
   {0x00, 0x40, (NEGTIVE_WORK_OFFSET+4)&0xff, (NEGTIVE_WORK_OFFSET+4)>>8&0xff},                    //�����й����ܷ�
   {0x00, 0x41, (NEGTIVE_WORK_OFFSET+8)&0xff, (NEGTIVE_WORK_OFFSET+8)>>8&0xff},                    //�����й�����ƽ
   {0x00, 0x42, (NEGTIVE_WORK_OFFSET+12)&0xff, (NEGTIVE_WORK_OFFSET+12)>>8&0xff},                  //�����й����ܹ�
   {0x00, 0x43, (NEGTIVE_WORK_OFFSET+16)&0xff, (NEGTIVE_WORK_OFFSET+16)>>8&0xff},                  //�����й����ܼ��

   {0x03, 0x49, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},                    //�����޹�������
   {0x03, 0x40, (POSITIVE_NO_WORK_OFFSET+4)&0xff, (POSITIVE_NO_WORK_OFFSET+4)>>8&0xff},            //�����޹����ܷ�
   {0x03, 0x41, (POSITIVE_NO_WORK_OFFSET+8)&0xff, (POSITIVE_NO_WORK_OFFSET+8)>>8&0xff},            //�����޹�����ƽ
   {0x03, 0x42, (POSITIVE_NO_WORK_OFFSET+12)&0xff, (POSITIVE_NO_WORK_OFFSET+12)>>8&0xff},          //�����޹����ܹ�
   {0x03, 0x43, (POSITIVE_NO_WORK_OFFSET+16)&0xff, (POSITIVE_NO_WORK_OFFSET+16)>>8&0xff},          //�����޹����ܼ��

   {0x02, 0x49, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},                      //�����޹�������
   {0x02, 0x40, (NEGTIVE_NO_WORK_OFFSET+4)&0xff, (NEGTIVE_NO_WORK_OFFSET+4)>>8&0xff},              //�����޹����ܷ�
   {0x02, 0x41, (NEGTIVE_NO_WORK_OFFSET+8)&0xff, (NEGTIVE_NO_WORK_OFFSET+8)>>8&0xff},              //�����޹�����ƽ
   {0x02, 0x42, (NEGTIVE_NO_WORK_OFFSET+12)&0xff, (NEGTIVE_NO_WORK_OFFSET+12)>>8&0xff},            //�����޹����ܹ�
   {0x02, 0x43, (NEGTIVE_NO_WORK_OFFSET+16)&0xff, (NEGTIVE_NO_WORK_OFFSET+16)>>8&0xff},            //�����޹����ܼ��

   {0x11, 0x29, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},                  //�����й����������
   {0x11, 0x20, (REQ_POSITIVE_WORK_OFFSET+3)&0xff, (REQ_POSITIVE_WORK_OFFSET+3)>>8&0xff},          //�����й����������
   {0x11, 0x21, (REQ_POSITIVE_WORK_OFFSET+6)&0xff, (REQ_POSITIVE_WORK_OFFSET+6)>>8&0xff},          //�����й��������ƽ
   {0x11, 0x22, (REQ_POSITIVE_WORK_OFFSET+9)&0xff, (REQ_POSITIVE_WORK_OFFSET+9)>>8&0xff},          //�����й����������
   {0x11, 0x23, (REQ_POSITIVE_WORK_OFFSET+12)&0xff, (REQ_POSITIVE_WORK_OFFSET+12)>>8&0xff},        //�����й�����������

   {0x81, 0x29, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff},                      //�����й���������ܳ���ʱ��
   {0x81, 0x20, (REQ_TIME_P_WORK_OFFSET+5)&0xff, (REQ_TIME_P_WORK_OFFSET+5)>>8&0xff},              //�����й�������������ʱ��
   {0x81, 0x21, (REQ_TIME_P_WORK_OFFSET+10)&0xff, (REQ_TIME_P_WORK_OFFSET+10)>>8&0xff},            //�����й��������ƽ����ʱ��
   {0x81, 0x22, (REQ_TIME_P_WORK_OFFSET+15)&0xff, (REQ_TIME_P_WORK_OFFSET+15)>>8&0xff},            //�����й���������ȳ���ʱ��
   {0x81, 0x23, (REQ_TIME_P_WORK_OFFSET+20)&0xff, (REQ_TIME_P_WORK_OFFSET+20)>>8&0xff},            //�����й��������������ʱ��

   {0x10, 0x29, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},                    //�����й���������ܳ���ʱ��
   {0x10, 0x20, (REQ_NEGTIVE_WORK_OFFSET+3)&0xff, (REQ_NEGTIVE_WORK_OFFSET+3)>>8&0xff},            //�����й�������������ʱ��
   {0x10, 0x21, (REQ_NEGTIVE_WORK_OFFSET+6)&0xff, (REQ_NEGTIVE_WORK_OFFSET+6)>>8&0xff},            //�����й��������ƽ����ʱ��
   {0x10, 0x22, (REQ_NEGTIVE_WORK_OFFSET+9)&0xff, (REQ_NEGTIVE_WORK_OFFSET+9)>>8&0xff},            //�����й���������ȳ���ʱ��
   {0x10, 0x23, (REQ_NEGTIVE_WORK_OFFSET+12)&0xff, (REQ_NEGTIVE_WORK_OFFSET+12)>>8&0xff},          //�����й��������������ʱ��

   {0x80, 0x29, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff},                      //�����й���������ܳ���ʱ��
   {0x80, 0x20, (REQ_TIME_N_WORK_OFFSET+5)&0xff, (REQ_TIME_N_WORK_OFFSET+5)>>8&0xff},              //�����й�������������ʱ��
   {0x80, 0x21, (REQ_TIME_N_WORK_OFFSET+10)&0xff, (REQ_TIME_N_WORK_OFFSET+10)>>8&0xff},            //�����й��������ƽ����ʱ��
   {0x80, 0x22, (REQ_TIME_N_WORK_OFFSET+15)&0xff, (REQ_TIME_N_WORK_OFFSET+15)>>8&0xff},            //�����й���������ȳ���ʱ��
   {0x80, 0x23, (REQ_TIME_N_WORK_OFFSET+20)&0xff, (REQ_TIME_N_WORK_OFFSET+20)>>8&0xff},            //�����й��������������ʱ��

   {0x00, 0x00, 0x0, 0x00},                                                                        //�˳����
   };

  //��ǰ����
  INT8U  cmdEdmi[TOTAL_COMMAND_REAL_EDMI][4] = {
 //{�Ĵ������ֽ�,�Ĵ������ֽ�,�����ص����ݴ洢ƫ��}
   {0x00, 0x00, 0, 0},                                                                             //��������ģʽ
   {0x00, 0x00, 0, 0},                                                                             //��������
   
   {0x01, 0x69, POSITIVE_WORK_OFFSET&0xff, POSITIVE_WORK_OFFSET>>8&0xff},                          //�����й�������
   {0x01, 0x60, (POSITIVE_WORK_OFFSET+4)&0xff, (POSITIVE_WORK_OFFSET+4)>>8&0xff},                  //�����й����ܷ�
   {0x01, 0x61, (POSITIVE_WORK_OFFSET+8)&0xff, (POSITIVE_WORK_OFFSET+8)>>8&0xff},                  //�����й�����ƽ
   {0x01, 0x62, (POSITIVE_WORK_OFFSET+12)&0xff, (POSITIVE_WORK_OFFSET+12)>>8&0xff},                //�����й����ܹ�
   {0x01, 0x63, (POSITIVE_WORK_OFFSET+16)&0xff, (POSITIVE_WORK_OFFSET+16)>>8&0xff},                //�����й����ܼ��

   {0x00, 0x69, NEGTIVE_WORK_OFFSET&0xff, NEGTIVE_WORK_OFFSET>>8&0xff},                            //�����й�������
   {0x00, 0x60, (NEGTIVE_WORK_OFFSET+4)&0xff, (NEGTIVE_WORK_OFFSET+4)>>8&0xff},                    //�����й����ܷ�
   {0x00, 0x61, (NEGTIVE_WORK_OFFSET+8)&0xff, (NEGTIVE_WORK_OFFSET+8)>>8&0xff},                    //�����й�����ƽ
   {0x00, 0x62, (NEGTIVE_WORK_OFFSET+12)&0xff, (NEGTIVE_WORK_OFFSET+12)>>8&0xff},                  //�����й����ܹ�
   {0x00, 0x63, (NEGTIVE_WORK_OFFSET+16)&0xff, (NEGTIVE_WORK_OFFSET+16)>>8&0xff},                  //�����й����ܼ��

   {0x03, 0x69, POSITIVE_NO_WORK_OFFSET&0xff, POSITIVE_NO_WORK_OFFSET>>8&0xff},                    //�����޹�������
   {0x03, 0x60, (POSITIVE_NO_WORK_OFFSET+4)&0xff, (POSITIVE_NO_WORK_OFFSET+4)>>8&0xff},            //�����޹����ܷ�
   {0x03, 0x61, (POSITIVE_NO_WORK_OFFSET+8)&0xff, (POSITIVE_NO_WORK_OFFSET+8)>>8&0xff},            //�����޹�����ƽ
   {0x03, 0x62, (POSITIVE_NO_WORK_OFFSET+12)&0xff, (POSITIVE_NO_WORK_OFFSET+12)>>8&0xff},          //�����޹����ܹ�
   {0x03, 0x63, (POSITIVE_NO_WORK_OFFSET+16)&0xff, (POSITIVE_NO_WORK_OFFSET+16)>>8&0xff},          //�����޹����ܼ��

   {0x02, 0x69, NEGTIVE_NO_WORK_OFFSET&0xff, NEGTIVE_NO_WORK_OFFSET>>8&0xff},                      //�����޹�������
   {0x02, 0x60, (NEGTIVE_NO_WORK_OFFSET+4)&0xff, (NEGTIVE_NO_WORK_OFFSET+4)>>8&0xff},              //�����޹����ܷ�
   {0x02, 0x61, (NEGTIVE_NO_WORK_OFFSET+8)&0xff, (NEGTIVE_NO_WORK_OFFSET+8)>>8&0xff},              //�����޹�����ƽ
   {0x02, 0x62, (NEGTIVE_NO_WORK_OFFSET+12)&0xff, (NEGTIVE_NO_WORK_OFFSET+12)>>8&0xff},            //�����޹����ܹ�
   {0x02, 0x63, (NEGTIVE_NO_WORK_OFFSET+16)&0xff, (NEGTIVE_NO_WORK_OFFSET+16)>>8&0xff},            //�����޹����ܼ��

   {0x11, 0x09, REQ_POSITIVE_WORK_OFFSET&0xff, REQ_POSITIVE_WORK_OFFSET>>8&0xff},                  //�����й����������
   {0x11, 0x00, (REQ_POSITIVE_WORK_OFFSET+3)&0xff, (REQ_POSITIVE_WORK_OFFSET+3)>>8&0xff},          //�����й����������
   {0x11, 0x01, (REQ_POSITIVE_WORK_OFFSET+6)&0xff, (REQ_POSITIVE_WORK_OFFSET+6)>>8&0xff},          //�����й��������ƽ
   {0x11, 0x02, (REQ_POSITIVE_WORK_OFFSET+9)&0xff, (REQ_POSITIVE_WORK_OFFSET+9)>>8&0xff},          //�����й����������
   {0x11, 0x03, (REQ_POSITIVE_WORK_OFFSET+12)&0xff, (REQ_POSITIVE_WORK_OFFSET+12)>>8&0xff},        //�����й�����������

   {0x81, 0x09, REQ_TIME_P_WORK_OFFSET&0xff, REQ_TIME_P_WORK_OFFSET>>8&0xff},                      //�����й���������ܳ���ʱ��
   {0x81, 0x00, (REQ_TIME_P_WORK_OFFSET+5)&0xff, (REQ_TIME_P_WORK_OFFSET+5)>>8&0xff},              //�����й�������������ʱ��
   {0x81, 0x01, (REQ_TIME_P_WORK_OFFSET+10)&0xff, (REQ_TIME_P_WORK_OFFSET+10)>>8&0xff},            //�����й��������ƽ����ʱ��
   {0x81, 0x02, (REQ_TIME_P_WORK_OFFSET+15)&0xff, (REQ_TIME_P_WORK_OFFSET+15)>>8&0xff},            //�����й���������ȳ���ʱ��
   {0x81, 0x03, (REQ_TIME_P_WORK_OFFSET+20)&0xff, (REQ_TIME_P_WORK_OFFSET+20)>>8&0xff},            //�����й��������������ʱ��

   {0x10, 0x09, REQ_NEGTIVE_WORK_OFFSET&0xff, REQ_NEGTIVE_WORK_OFFSET>>8&0xff},                    //�����й���������ܳ���ʱ��
   {0x10, 0x00, (REQ_NEGTIVE_WORK_OFFSET+3)&0xff, (REQ_NEGTIVE_WORK_OFFSET+3)>>8&0xff},            //�����й�������������ʱ��
   {0x10, 0x01, (REQ_NEGTIVE_WORK_OFFSET+6)&0xff, (REQ_NEGTIVE_WORK_OFFSET+6)>>8&0xff},            //�����й��������ƽ����ʱ��
   {0x10, 0x02, (REQ_NEGTIVE_WORK_OFFSET+9)&0xff, (REQ_NEGTIVE_WORK_OFFSET+9)>>8&0xff},            //�����й���������ȳ���ʱ��
   {0x10, 0x03, (REQ_NEGTIVE_WORK_OFFSET+12)&0xff, (REQ_NEGTIVE_WORK_OFFSET+12)>>8&0xff},          //�����й��������������ʱ��

   {0x80, 0x09, REQ_TIME_N_WORK_OFFSET&0xff, REQ_TIME_N_WORK_OFFSET>>8&0xff},                      //�����й���������ܳ���ʱ��
   {0x80, 0x00, (REQ_TIME_N_WORK_OFFSET+5)&0xff, (REQ_TIME_N_WORK_OFFSET+5)>>8&0xff},              //�����й�������������ʱ��
   {0x80, 0x01, (REQ_TIME_N_WORK_OFFSET+10)&0xff, (REQ_TIME_N_WORK_OFFSET+10)>>8&0xff},            //�����й��������ƽ����ʱ��
   {0x80, 0x02, (REQ_TIME_N_WORK_OFFSET+15)&0xff, (REQ_TIME_N_WORK_OFFSET+15)>>8&0xff},            //�����й���������ȳ���ʱ��
   {0x80, 0x03, (REQ_TIME_N_WORK_OFFSET+20)&0xff, (REQ_TIME_N_WORK_OFFSET+20)>>8&0xff},            //�����й��������������ʱ��
   
   {0xE0, 0x00, VOLTAGE_PHASE_A&0xFF, VOLTAGE_PHASE_A>>8&0xFF},                                    //A���ѹ
   {0xE0, 0x01, VOLTAGE_PHASE_B&0xFF, VOLTAGE_PHASE_B>>8&0xFF},                                    //B���ѹ
   {0xE0, 0x02, VOLTAGE_PHASE_C&0xFF, VOLTAGE_PHASE_C>>8&0xFF},                                    //C���ѹ
   
   {0xE0, 0x10, CURRENT_PHASE_A&0xFF, CURRENT_PHASE_A>>8&0xFF},                                    //A�����
   {0xE0, 0x11, CURRENT_PHASE_B&0xFF, CURRENT_PHASE_B>>8&0xFF},                                    //B�����
   {0xE0, 0x12, CURRENT_PHASE_C&0xFF, CURRENT_PHASE_C>>8&0xFF},                                    //B�����
   
   {0xE0, 0x33, POWER_INSTANT_WORK&0xFF, POWER_INSTANT_WORK>>8&0xFF},                              //���й�����
   {0xE0, 0x30, POWER_PHASE_A_WORK&0xFF, POWER_PHASE_A_WORK>>8&0xFF},                              //A���й�����
   {0xE0, 0x31, POWER_PHASE_B_WORK&0xFF, POWER_PHASE_B_WORK>>8&0xFF},                              //B���й�����
   {0xE0, 0x32, POWER_PHASE_C_WORK&0xFF, POWER_PHASE_C_WORK>>8&0xFF},                              //C���й�����
   
   {0xE0, 0x43, POWER_INSTANT_NO_WORK&0xFF, POWER_INSTANT_NO_WORK>>8&0xFF},                        //���޹�����
   {0xE0, 0x40, POWER_PHASE_A_NO_WORK&0xFF, POWER_PHASE_A_NO_WORK>>8&0xFF},                        //A���޹�����
   {0xE0, 0x41, POWER_PHASE_B_NO_WORK&0xFF, POWER_PHASE_B_NO_WORK>>8&0xFF},                        //B���޹�����
   {0xE0, 0x42, POWER_PHASE_C_NO_WORK&0xFF, POWER_PHASE_C_NO_WORK>>8&0xFF},                        //C���޹�����
   
   {0xE0, 0x26, TOTAL_POWER_FACTOR&0xFF, TOTAL_POWER_FACTOR>>8&0xFF},                              //�ܹ�������
   
   {0xF0, 0x3D, DATE_AND_WEEK&0xFF, DATE_AND_WEEK>>8&0xFF},                                        //���ʱ��/����

   {0x00, 0x00, 0x0, 0x00},                                                                        //�˳����
   };
#endif

//������(/������Landis��Լ)��
#ifdef LANDIS_GRY_ZD_PROTOCOL
  char landisChar[TOTAL_DATA_ITEM_ZD][8] = 
  {
    "1.8.0",       //�����й��ܵ���ʾֵ
    "1.8.1",       //�����й�����1����ʾֵ
    "1.8.2",       //�����й�����2����ʾֵ
    "1.8.3",       //�����й�����3����ʾֵ
    "1.8.4",       //�����й�����4����ʾֵ
    "2.8.0",       //�����й��ܵ���ʾֵ
    "2.8.1",       //�����й�����1����ʾֵ
    "2.8.2",       //�����й�����2����ʾֵ
    "2.8.3",       //�����й�����3����ʾֵ
    "2.8.4",       //�����й�����4����ʾֵ
    "3.8.0",       //�����޹��ܵ���ʾֵ
    "3.8.1",       //�����޹�����1����ʾֵ
    "3.8.2",       //�����޹�����2����ʾֵ
    "3.8.3",       //�����޹�����3����ʾֵ
    "3.8.4",       //�����޹�����4����ʾֵ
    "4.8.0",       //�����޹��ܵ���ʾֵ
    "4.8.1",       //�����޹�����1����ʾֵ
    "4.8.2",       //�����޹�����2����ʾֵ
    "4.8.3",       //�����޹�����3����ʾֵ
    "4.8.4",       //�����޹�����4����ʾֵ
    "5.8.0",       //�������޹��ܵ���ʾֵ
    "5.8.1",       //�������޹�����1����ʾֵ
    "5.8.2",       //�������޹�����2����ʾֵ
    "5.8.3",       //�������޹�����3����ʾֵ
    "5.8.4",       //�������޹�����4����ʾֵ
    "6.8.0",       //�������޹��ܵ���ʾֵ
    "6.8.1",       //�������޹�����1����ʾֵ
    "6.8.2",       //�������޹�����2����ʾֵ
    "6.8.3",       //�������޹�����3����ʾֵ
    "6.8.4",       //�������޹�����4����ʾֵ
    "7.8.0",       //�������޹��ܵ���ʾֵ
    "7.8.1",       //�������޹�����1����ʾֵ
    "7.8.2",       //�������޹�����2����ʾֵ
    "7.8.3",       //�������޹�����3����ʾֵ
    "7.8.4",       //�������޹�����4����ʾֵ
    "8.8.0",       //�������޹��ܵ���ʾֵ
    "8.8.1",       //�������޹�����1����ʾֵ
    "8.8.2",       //�������޹�����2����ʾֵ
    "8.8.3",       //�������޹�����3����ʾֵ
    "8.8.4",       //�������޹�����4����ʾֵ
    
    "1.6.0",       //�����й����������������ʱ��
    "1.6.1",       //�����й�����1�������������ʱ��
    "1.6.2",       //�����й�����2�������������ʱ��
    "1.6.3",       //�����й�����3�������������ʱ��
    "1.6.4",       //�����й�����4�������������ʱ��
    "2.6.0",       //�����й����������������ʱ��
    "2.6.1",       //�����й�����1�������������ʱ��
    "2.6.2",       //�����й�����2�������������ʱ��
    "2.6.3",       //�����й�����3�������������ʱ��
    "2.6.4",       //�����й�����4�������������ʱ��
    "3.6.0",       //�����޹����������������ʱ��
    "3.6.1",       //�����޹�����1�������������ʱ��
    "3.6.2",       //�����޹�����2�������������ʱ��
    "3.6.3",       //�����޹�����3�������������ʱ��
    "3.6.4",       //�����޹�����4�������������ʱ��
    "4.6.0",       //�����޹����������������ʱ��
    "4.6.1",       //�����޹�����1�������������ʱ��
    "4.6.2",       //�����޹�����2�������������ʱ��
    "4.6.3",       //�����޹�����3�������������ʱ��
    "4.6.4",       //�����޹�����4�������������ʱ��
    
    "0.9.1",       //��ǰʱ��
    "0.9.2",       //��ǰ����
    "16.7",        //�й�����
    "131.7",       //�޹�����
    "32.7",        //A���ѹ
    "52.7",        //B���ѹ
    "72.7",        //C���ѹ
    "31.7",        //A�����
    "51.7",        //B�����
    "71.7",        //C�����
    "C.2.0",       //�������õĴ���
    "C.2.1",       //�ϴβ������õ�����
    "C.6.0",       //���ʹ��ʱ��(Сʱ��)

    "31.1",        //A���ѹ���
    "31.2",        //B���ѹ���
    "31.3",        //C���ѹ���
    "51.1",        //A��������
    "51.2",        //B��������
    "51.3",        //C��������
    
    //"0.1.0",       //��λ�������ϴθ�λʱ��
    //"C.7.0",       //��ʧѹ����
    //"C.7.1",       //A��ʧѹ����
    //"C.7.2",       //B��ʧѹ����
    //"C.7.3",       //C��ʧѹ����
  };
  
  INT16U landisOffset[TOTAL_DATA_ITEM_ZD] = 
  {
    POSITIVE_WORK_OFFSET,           //�����й��ܵ���ʾֵ
    POSITIVE_WORK_OFFSET+4,         //�����й�����1����ʾֵ
    POSITIVE_WORK_OFFSET+8,         //�����й�����2����ʾֵ
    POSITIVE_WORK_OFFSET+12,        //�����й�����3����ʾֵ
    POSITIVE_WORK_OFFSET+16,        //�����й�����4����ʾֵ
    NEGTIVE_WORK_OFFSET,            //�����й��ܵ���ʾֵ
    NEGTIVE_WORK_OFFSET+4,          //�����й�����1����ʾֵ
    NEGTIVE_WORK_OFFSET+8,          //�����й�����2����ʾֵ
    NEGTIVE_WORK_OFFSET+12,         //�����й�����3����ʾֵ
    NEGTIVE_WORK_OFFSET+16,         //�����й�����4����ʾֵ
    POSITIVE_NO_WORK_OFFSET,        //�����޹��ܵ���ʾֵ
    POSITIVE_NO_WORK_OFFSET+4,      //�����޹�����1����ʾֵ
    POSITIVE_NO_WORK_OFFSET+8,      //�����޹�����2����ʾֵ
    POSITIVE_NO_WORK_OFFSET+12,     //�����޹�����3����ʾֵ
    POSITIVE_NO_WORK_OFFSET+16,     //�����޹�����4����ʾֵ
    NEGTIVE_NO_WORK_OFFSET,         //�����޹��ܵ���ʾֵ
    NEGTIVE_NO_WORK_OFFSET+4,       //�����޹�����1����ʾֵ
    NEGTIVE_NO_WORK_OFFSET+8,       //�����޹�����2����ʾֵ
    NEGTIVE_NO_WORK_OFFSET+12,      //�����޹�����3����ʾֵ
    NEGTIVE_NO_WORK_OFFSET+16,      //�����޹�����4����ʾֵ
    QUA1_NO_WORK_OFFSET,            //�������޹��ܵ���ʾֵ
    QUA1_NO_WORK_OFFSET+4,          //�������޹�����1����ʾֵ
    QUA1_NO_WORK_OFFSET+8,          //�������޹�����2����ʾֵ
    QUA1_NO_WORK_OFFSET+12,         //�������޹�����3����ʾֵ
    QUA1_NO_WORK_OFFSET+16,         //�������޹�����4����ʾֵ
    QUA2_NO_WORK_OFFSET,            //�������޹��ܵ���ʾֵ
    QUA2_NO_WORK_OFFSET+4,          //�������޹�����1����ʾֵ
    QUA2_NO_WORK_OFFSET+8,          //�������޹�����2����ʾֵ
    QUA2_NO_WORK_OFFSET+12,         //�������޹�����3����ʾֵ
    QUA2_NO_WORK_OFFSET+16,         //�������޹�����4����ʾֵ
    QUA3_NO_WORK_OFFSET,            //�������޹��ܵ���ʾֵ
    QUA3_NO_WORK_OFFSET+4,          //�������޹�����1����ʾֵ
    QUA3_NO_WORK_OFFSET+8,          //�������޹�����2����ʾֵ
    QUA3_NO_WORK_OFFSET+12,         //�������޹�����3����ʾֵ
    QUA3_NO_WORK_OFFSET+16,         //�������޹�����4����ʾֵ
    QUA4_NO_WORK_OFFSET,            //�������޹��ܵ���ʾֵ
    QUA4_NO_WORK_OFFSET+4,          //�������޹�����1����ʾֵ
    QUA4_NO_WORK_OFFSET+8,          //�������޹�����2����ʾֵ
    QUA4_NO_WORK_OFFSET+12,         //�������޹�����3����ʾֵ
    QUA4_NO_WORK_OFFSET+16,         //�������޹�����4����ʾֵ
    
    REQ_POSITIVE_WORK_OFFSET,       //�����й����������������ʱ��
    REQ_POSITIVE_WORK_OFFSET+3,     //�����й�����1�������������ʱ��
    REQ_POSITIVE_WORK_OFFSET+6,     //�����й�����2�������������ʱ��
    REQ_POSITIVE_WORK_OFFSET+9,     //�����й�����3�������������ʱ��
    REQ_POSITIVE_WORK_OFFSET+12,    //�����й�����4�������������ʱ��
    REQ_NEGTIVE_WORK_OFFSET,        //�����й����������������ʱ��
    REQ_NEGTIVE_WORK_OFFSET+3,      //�����й�����1�������������ʱ��
    REQ_NEGTIVE_WORK_OFFSET+6,      //�����й�����2�������������ʱ��
    REQ_NEGTIVE_WORK_OFFSET+9,      //�����й�����3�������������ʱ��
    REQ_NEGTIVE_WORK_OFFSET+12,     //�����й�����4�������������ʱ��
    REQ_POSITIVE_NO_WORK_OFFSET,    //�����޹����������������ʱ��
    REQ_POSITIVE_NO_WORK_OFFSET+3,  //�����޹�����1�������������ʱ��
    REQ_POSITIVE_NO_WORK_OFFSET+6,  //�����޹�����2�������������ʱ��
    REQ_POSITIVE_NO_WORK_OFFSET+9,  //�����޹�����3�������������ʱ��
    REQ_POSITIVE_NO_WORK_OFFSET+12, //�����޹�����4�������������ʱ��
    REQ_NEGTIVE_NO_WORK_OFFSET,     //�����޹����������������ʱ��
    REQ_NEGTIVE_NO_WORK_OFFSET+3,   //�����޹�����1�������������ʱ��
    REQ_NEGTIVE_NO_WORK_OFFSET+6,   //�����޹�����2�������������ʱ��
    REQ_NEGTIVE_NO_WORK_OFFSET+9,   //�����޹�����3�������������ʱ��
    REQ_NEGTIVE_NO_WORK_OFFSET+12,  //�����޹�����4�������������ʱ��
    
    METER_TIME,                     //��ǰʱ��
    DATE_AND_WEEK,                  //��ǰ����
    POWER_INSTANT_WORK,             //�й�����
    POWER_INSTANT_NO_WORK,          //�޹�����    
    VOLTAGE_PHASE_A,                //A���ѹ
    VOLTAGE_PHASE_B,                //B���ѹ
    VOLTAGE_PHASE_C,                //C���ѹ
    CURRENT_PHASE_A,                //A�����
    CURRENT_PHASE_B,                //B�����
    CURRENT_PHASE_C,                //C�����
    PROGRAM_TIMES,                  //�������õĴ���
    LAST_PROGRAM_TIME,              //�ϴβ������õ�����
    BATTERY_WORK_TIME,              //���ʹ��ʱ��(Сʱ��)

    PHASE_ANGLE_V_A,                //A���ѹ���
    PHASE_ANGLE_V_B,                //B���ѹ���
    PHASE_ANGLE_V_C,                //C���ѹ���
    PHASE_ANGLE_C_A,                //A��������
    PHASE_ANGLE_C_B,                //B��������
    PHASE_ANGLE_C_C,                //C��������
    
    //"0.1.0",       //��λ�������ϴθ�λʱ��
    //"C.7.0",       //��ʧѹ����
    //"C.7.1",       //A��ʧѹ����
    //"C.7.2",       //B��ʧѹ����
    //"C.7.3",       //C��ʧѹ����
  };
#endif

INT16U findDataOffset(INT8U protocol, INT8U *di);
INT8S  meterInput(INT8U arrayItem, INT8U *pFrameHead,INT16U frameLength);

#ifdef PROTOCOL_645_1997
 INT8S process645Para(INT8U arrayItem, INT8U *pDataHead, INT8U len);
 INT8S process645Data(INT8U arrayItem, INT8U *pDataHead,INT8U length);
 INT8S processShiDuanData(INT8U arrayItem, INT8U *pDataHead,INT8U length);
#endif

#ifdef PROTOCOL_645_2007
 INT8S process645Data2007(INT8U arrayItem,struct recvFrameStruct *frame);
#endif

#ifdef PROTOCOL_EDMI_GROUP
 INT8S processEdmiData(INT8U arrayItem, INT8U *frame, INT16U loadLen);

 //ע��:�����λһ��Ϊ"1",����ȥ
 const unsigned short cnCRC_16    = 0x8005;
 //CRC-16 = X16 + X15 + X2 + X0
 const unsigned short cnCRC_CCITT = 0x1021;
 //CRC-CCITT = X16 + X12 + X5 + X0����˵���16λCRC����ʽ����һ��Ҫ��
 //const unsigned long cnCRC_32    = 0x04C10DB7;
 //CRC-32 = X32 + X26 + X23 + X22 + X16 + X11 + X10 + X8 + X7 + X5 + X4 + X2 + X1 + X0

 unsigned long tableCrc[256];      //CRC��

/*******************************************************
��������:buildTable16
��������:����16λCRC��
���ú���:
�����ú���:
�������:
�������:
����ֵ��CRCֵ
*******************************************************/
void buildTable16( unsigned short aPoly )
{
  unsigned short i, j;
  unsigned short nData;
  unsigned short nAccum;

  for (i=0; i<256; i++)
  {
    nData = (unsigned short)(i<<8);
    nAccum = 0;
    for (j=0; j<8; j++)
    {
      if ((nData^nAccum) & 0x8000)
      {
        nAccum = (nAccum<<1) ^ aPoly;
      }
      else
      {
        nAccum <<= 1;
      }
      
      nData <<= 1;
    }
    
    tableCrc[i] = ( unsigned long )nAccum;
  }
}

/*******************************************************
��������:edmi_CRC_16
��������:����16λCRCֵ(CRC-16��CRC-CCITT)
���ú���:
�����ú���:
�������:aData,Ҫ����CRCУ�����Ϣ
         aSize,��Ϣ���ֽ���
�������:
����ֵ��CRCֵ
*******************************************************/
unsigned short edmi_CRC_16(unsigned char * aData, unsigned long aSize)
{
  unsigned long i;
  unsigned short nAccum = 0;

  buildTable16(cnCRC_CCITT); // or cnCRC_16
  for ( i = 0; i < aSize; i++ )
  {
    nAccum = (nAccum<<8) ^ (unsigned short)tableCrc[(nAccum>>8)^*aData++];
  }
  
  return nAccum;
}


#endif

#ifdef LANDIS_GRY_ZD_PROTOCOL
 INT8S processLandisGryData(INT8U arrayItem, INT8U *frame, INT16U loadLen);
#endif

#ifdef PROTOCOL_ABB_GROUP
 INT8S processAbbData(INT8U arrayItem, INT8U *frame, INT16U loadLen, INT8U copyItem);
 unsigned short CRC_ABB(unsigned char * aData, unsigned long aSize);
 unsigned long abbEncryption(unsigned long lKey, unsigned long rKey);
#endif

#ifdef PROTOCOL_MODUBUS_GROUP
 INT8U processAssData(INT8U arrayItem, INT8U *data,INT8U length);
 INT8U processHyData(INT8U arrayItem, INT8U *data,INT8U length);
 INT8U processXyData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processSwitchData(INT8U arrayItem, INT8U *data,INT8U length);
 INT8U processMwData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processJzData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processWe6301Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
 INT8U processPmc350Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol);
#endif


unsigned long rAbbMeterKey;       //ABB��Զ��Key
INT8U         abbHandclaspOk=0;
INT8U         abbClass0[40];      //ABB��0���ݻ���
INT8U         abbClass2[104];     //ABB��2���ݻ���




//�㷨��,���
unsigned char auchCRCHi[]=
{
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
  0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
  0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
  0x40
};

unsigned char auchCRCLo[] =
{
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
  0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
  0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
  0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
  0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
  0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
  0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
  0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
  0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
  0x40
};

unsigned int modbusCRC16(unsigned char *updata, unsigned char len)
{
  unsigned char uchCRCHi=0xff;
  unsigned char uchCRCLo=0xff;
  unsigned int  uindex;
  while(len--)
  {
    uindex=uchCRCHi^*updata++;
    uchCRCHi=uchCRCLo^auchCRCHi[uindex];
    uchCRCLo=auchCRCLo[uindex];
  }

  return (uchCRCHi<<8|uchCRCLo);
}


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
BOOL initCopyProcess(struct meterCopyPara *cp)
{
  INT8U  arrayItem;     //�����±�
  INT8U  i;
  long   tmpData;
 
  if (cp->port>0 && cp->port<4)
  {
 	arrayItem = cp->port-1;
  }
  else
  {
 	if (cp->port>=31 && cp->port<=33)
 	{
 	  arrayItem = cp->port-28;
 	}
 	else
 	{
 	  return FALSE;
 	}
  }
	 
  //�������
  multiCopy[arrayItem].meterAddr[0] = cp->meterAddr[0];
  multiCopy[arrayItem].meterAddr[1] = cp->meterAddr[1];
  multiCopy[arrayItem].meterAddr[2] = cp->meterAddr[2];
  multiCopy[arrayItem].meterAddr[3] = cp->meterAddr[3];
  multiCopy[arrayItem].meterAddr[4] = cp->meterAddr[4];
  multiCopy[arrayItem].meterAddr[5] = cp->meterAddr[5];
  multiCopy[arrayItem].port = cp->port;
  multiCopy[arrayItem].protocol = cp->protocol;	 
  multiCopy[arrayItem].send = cp->send;
  multiCopy[arrayItem].save = cp->save;
  multiCopy[arrayItem].energy = cp->dataBuff;

  //if (arrayItem==3)  //�ز��˿�
  if (arrayItem>=3)  //�ز��˿�,ly,2012-01-09
  {
    switch(multiCopy[arrayItem].protocol)
    {
   	  case THREE_2007:
        multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_THREE_ENERGY_RECORD];	        
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
   	  	break;

   	  case THREE_LOCAL_CHARGE_CTRL_2007:
        multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_THREE_LOCAL_RECORD];	        
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
   	  	break;
   	  
   	  case THREE_REMOTE_CHARGE_CTRL_2007:
        multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_THREE_REMOTE_RECORD];	        
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
        break;
        
      case KEY_HOUSEHOLD_2007:
        multiCopy[arrayItem].reqAndReqTime = cp->dataBuff;
        multiCopy[arrayItem].paraVariable  = &cp->dataBuff[LENGTH_OF_KEY_ENERGY_RECORD];
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
      	break;
   	  
      default:
        multiCopy[arrayItem].reqAndReqTime = cp->dataBuff;
        multiCopy[arrayItem].paraVariable  = cp->dataBuff;
        multiCopy[arrayItem].shiDuan       = cp->dataBuff;
        break;
    }
  }
  else
  {
    if (multiCopy[arrayItem].protocol==KEY_HOUSEHOLD_2007)
    {
      multiCopy[arrayItem].reqAndReqTime = cp->dataBuff;
      multiCopy[arrayItem].paraVariable  = &cp->dataBuff[LENGTH_OF_KEY_ENERGY_RECORD];
      multiCopy[arrayItem].shiDuan       = cp->dataBuff;
    }
    else
    {
      multiCopy[arrayItem].reqAndReqTime = &cp->dataBuff[LENGTH_OF_ENERGY_RECORD];
   
      multiCopy[arrayItem].paraVariable  = &cp->dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD];
      multiCopy[arrayItem].shiDuan       = &cp->dataBuff[LENGTH_OF_ENERGY_RECORD+LENGTH_OF_REQ_RECORD+LENGTH_OF_PARA_RECORD];
    }
  }

  //˽�б�������ֵ
  multiCopy[arrayItem].copyItem = 0;
  multiCopy[arrayItem].hasData = 0;
  multiCopy[arrayItem].recvFrameTail = 0;
  multiCopy[arrayItem].totalLenOfFrame = 2048;
  multiCopy[arrayItem].hasData = 0x0;
 
  multiCopy[arrayItem].copyDataType = cp->copyDataType;
  multiCopy[arrayItem].copyTime = cp->copyTime;
  multiCopy[arrayItem].pn = cp->pn;
	 
  switch(cp->protocol)
  {
	 	case DLT_645_1997:    //DL/T645-1997
	 	  if (cp->copyDataType==LAST_MONTH_DATA)
	 	  {
	 	  	multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTMONTH_645_1997;
	 	  }
	 	  else
	 	  {
	 	  	multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_CURRENT_645_1997;
	 	  }
	 	  break;
	 	  	
	 	case DLT_645_2007:
	 	  switch (cp->copyDataType)
	 	  {
	 	  	case LAST_MONTH_DATA:
	 	  	  multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTMONTH_645_2007;
	 	  	  break;
	 	  	    
	 	  	case PRESENT_DATA:
	 	  	  multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_CURRENT_645_2007;
	 	  	  break;
	 	  	  
	 	  	case LAST_DAY_DATA:
	 	  	  multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
	 	  	  break;
	 	  }
	 	  break;
 	  	
    case SINGLE_PHASE_645_1997:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_97;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_645_97;
  	  }
  	  break;

    case SINGLE_PHASE_645_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_07;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_645_07;
  	  }
  	  break;

    //2012-09-28
    case SINGLE_PHASE_645_2007_TARIFF:
  	  multiCopy[arrayItem].totalCopyItem = 2;
  	  break;

    case SINGLE_PHASE_645_2007_TOTAL:
  	  multiCopy[arrayItem].totalCopyItem = 2;
  	  break;

    case SINGLE_LOCAL_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_07;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_LOCAL_CTRL_07;
  	  }
  	  break;

    case SINGLE_REMOTE_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_SINGLE_07;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_SINGLE_REMOTE_CTRL_07;
  	  }
  	  break;

    case THREE_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_THREE__07;
  	  }
  	  break;

    case THREE_LOCAL_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_THREE_LOCAL_CTRL_07;
  	  }
  	  break;

    case THREE_REMOTE_CHARGE_CTRL_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_645_2007;
  	  }
  	  else
   	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_THREE_REMOTE_CTRL_07;
  	  }
  	  break;

    case KEY_HOUSEHOLD_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_LASTDAY_KEY_2007;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_KEY_2007;
  	  }
  	  break;
  	
    case DOT_COPY_1997:
    case DOT_COPY_2007:
  	  multiCopy[arrayItem].totalCopyItem = 4;
  	  break;
  	
    case HOUR_FREEZE_2007:
  	  multiCopy[arrayItem].totalCopyItem = cp->dataBuff[0];
  	  for(i=0;i<multiCopy[arrayItem].totalCopyItem;i++)
      {
    		hourFreeze2007[i][3] = cp->dataBuff[i+1];

       #ifdef SHOW_DEBUG_INFO
    		printf("��������ʱ��=%d\n", hourFreeze2007[i][3]);
       #endif
      }
      memset(cp->dataBuff, 0xee, 64);
  	  break;

    case PN_WORK_NOWORK_1997:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = 0;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_PN_WORK_NOWORK_645_97;
  	  }
  	  break;

    case PN_WORK_NOWORK_2007:
  	  if (cp->copyDataType==LAST_DAY_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = 0;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_CMD_PN_WORK_NOWORK_645_07;
  	  }
  	  break;

   #ifdef PROTOCOL_EDMI_GROUP
	  case EDMI_METER:       //����(EDMI)��
  	  if (cp->copyDataType==LAST_MONTH_DATA)
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_LASTMONTH_EDMI;
  	  }
  	  else
  	  {
  	    multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_REAL_EDMI;
  	  }

	  	//�������ַ����ǳ���9λ�ı��ַ������ܲ�׼ȷ
	  	tmpData = (cp->meterAddr[0]&0xf) + (cp->meterAddr[0]>>4&0xf)*10 
	    	      + (cp->meterAddr[1]&0xf)*100 + (cp->meterAddr[1]>>4&0xf)*1000
	      	    + (cp->meterAddr[2]&0xf)*10000 + (cp->meterAddr[2]>>4&0xf)*100000 
	        	  + (cp->meterAddr[3]&0xf)*1000000 + (cp->meterAddr[3]>>4&0xf)*10000000
	          	+ (cp->meterAddr[4]&0xf)*100000000 + (cp->meterAddr[4]>>4&0xf)*1000000000;

		  multiCopy[arrayItem].meterAddr[0] = tmpData>>24&0xff;
		  multiCopy[arrayItem].meterAddr[1] = tmpData>>16&0xff;
		  multiCopy[arrayItem].meterAddr[2] = tmpData>>8&0xff;
		  multiCopy[arrayItem].meterAddr[3] = tmpData&0xff;
		  multiCopy[arrayItem].meterAddr[4] = 0x0;
		  multiCopy[arrayItem].meterAddr[5] = 0x0;
  	  break;
   #endif
  
   #ifdef LANDIS_GRY_ZD_PROTOCOL
		case SIMENS_ZD_METER:   //������(/������)ZD��
    case SIMENS_ZB_METER:   //������(/������)ZB��
  	  multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_REAL_ZD;
  	  break;
   #endif
	  
   #ifdef PROTOCOL_ABB_GROUP
    case ABB_METER:       //ABB����
  	  multiCopy[arrayItem].totalCopyItem = TOTAL_COMMAND_REAL_ABB;
  	
  	  rAbbMeterKey = 0x0;
  	  abbHandclaspOk = 0x0;
  	  break;
   #endif

   #ifdef PROTOCOL_MODUBUS_GROUP

		case MODBUS_ASS:		  //��ɭ˼MODBUS��
	  	multiCopy[arrayItem].totalCopyItem = 1;
	  	break;

		case MODBUS_HY:		    //ɽ����ԶMODBUS��
	  	multiCopy[arrayItem].totalCopyItem = 2;
	  	break;
	
		case MODBUS_XY_F:		  //�Ϻ���ҵMODBUS�๦�ܱ�
		case MODBUS_XY_UI:		//�Ϻ���ҵMODBUS��ѹ������
	  	multiCopy[arrayItem].totalCopyItem = 1;
	  	break;
	
		case MODBUS_XY_M:		  //�Ϻ���ҵMODBUS���ģ��
	  	multiCopy[arrayItem].totalCopyItem = 2;
	  	break;
	
		case MODBUS_SWITCH:		//MODBUS������ģ��
	  	multiCopy[arrayItem].totalCopyItem = 1;
	 	  break;
		
		case MODBUS_T_H:	    //MODBUS��ʪ��ģ��
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_MW_F:	    //�ɶ�����MODBUSȫ���ܱ�
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_MW_UI:	  //�ɶ�����MODBUS��ѹ������
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_JZ_F:	    //�Ϻ�����MODBUSȫ���ܱ�
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
			
		case MODBUS_JZ_UI:	  //�Ϻ�����MODBUS��ѹ������
			multiCopy[arrayItem].totalCopyItem = 1;
			break;
		
		case MODBUS_PMC350_F:    //�����е�PMC350�������ֵ��
			multiCopy[arrayItem].totalCopyItem = 2;
			break;
		
		case MODBUS_WE6301_F:	   //��˹��WE6301ȫ���ܱ�
			multiCopy[arrayItem].totalCopyItem = 3;
			break;
   #endif

    default:
  	  multiCopy[arrayItem].totalCopyItem = 0;
  	  return FALSE;
  	  break;
  }

  return TRUE;
}

/*******************************************************
��������:meterFraming
��������:����������֡����
���ú���:
�����ú���:
�������:INT8U flag,��־ D1,�³����������һ����������(D1=0-�³�����,D1=1-��һ����������)   
�������:
����ֵ��=0,�ѷ���
        =1,������
        -1,�쳣
*******************************************************/
INT8S meterFraming(INT8U port,INT8U flag)
{
  INT8U     arrayItem;     //�����±�
  INT8U     frameTail;
  INT16U    i;
  INT8U     sendBuf[128];
  INT8U     tmpSendBuf[135];
  INT8U     *pParaData;
  INT32U    tmpData;
  DATE_TIME tmpTimeA, tmpTimeB, tmpTimeC;
  char      tmpStr[5];
	
  INT16U    crcResult;
  INT8U     edmiTail;
	
  //ABB
  unsigned long abbKey;
  INT16U    offsetReq, offsetReqTime;    //ABB������Ҫ
  INT16U    j;                           //ABB������Ҫ
  INT32U    tmpDatax;                    //ABB������Ҫ
  INT8U     foundLoc;                    //����λ��(ABB������Ҫ)
  
 #ifdef SHOW_DEBUG_INFO
  printf("meterFraming:����˿�=%d\n", port);
 #endif
  
  if (port>0 && port<4)
  {
		arrayItem = port-1;
  }
  else
  {
		if (port>=31 && port<=33)
		{
	 	  arrayItem = port-28;
		}
		else
		{
	 	  return -1;
	 	}
  }

  if ((flag&0x2)==0x0)  //�³�����
  {
		multiCopy[arrayItem].copyItem++;
  }
  else                  //�ط�
  {
    //�ط�ʱ�����ǰ���պ�δ���������(�����ǽ��ճ���)
    multiCopy[arrayItem].recvFrameTail = 0;
    multiCopy[arrayItem].totalLenOfFrame = 2048;
  }

frameingPoint:

  //�����ѳ���?
  if (multiCopy[arrayItem].copyItem>multiCopy[arrayItem].totalCopyItem)
  {
    //if (port==31)
    if (port>=31 && port<=33)
    {
   	  //�вɼ����ĵ�ǰ����������
   	  if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
   	  {
	 	    switch (multiCopy[arrayItem].protocol)
	 	    {
	 	  	  case SINGLE_PHASE_645_1997:
	 	  	  case SINGLE_PHASE_645_2007:
	 	      case SINGLE_PHASE_645_2007_TARIFF:
	 	      case SINGLE_PHASE_645_2007_TOTAL:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, SINGLE_PHASE_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
	 	        break;

	 	  	  case SINGLE_LOCAL_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, SINGLE_LOCAL_CTRL_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_SINGLE_LOCAL_RECORD);
	 	        break;

	 	  	  case SINGLE_REMOTE_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, SINGLE_REMOTE_CTRL_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_SINGLE_REMOTE_RECORD);
	 	        break;

	        case THREE_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, THREE_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_THREE_ENERGY_RECORD);
		        break;
		
	 	  	  case THREE_LOCAL_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, THREE_LOCAL_CTRL_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_THREE_LOCAL_RECORD);
	 	        break;

	 	  	  case THREE_REMOTE_CHARGE_CTRL_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                  multiCopy[arrayItem].energy, THREE_REMOTE_CTRL_PRESENT, \
	 	                                   ENERGY_DATA, LENGTH_OF_THREE_REMOTE_RECORD);

	 	  	  case KEY_HOUSEHOLD_2007:
	 	        multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                                   multiCopy[arrayItem].energy, KEY_HOUSEHOLD_PRESENT, \
	 	                                    ENERGY_DATA, LENGTH_OF_KEY_ENERGY_RECORD);
	 	        break;
	 	      
	 	    }
      }
   		 
      //�вɼ�������������(��ǰ)
      if (multiCopy[arrayItem].hasData&HAS_CURRENT_REQ)
      {
			 	switch (multiCopy[arrayItem].protocol)
			 	{
			 	  case THREE_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].reqAndReqTime, THREE_PRESENT, \
		                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
		        break;

			 	  case THREE_LOCAL_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].reqAndReqTime, THREE_LOCAL_CTRL_PRESENT, \
		                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
		        break;

			 	  case THREE_REMOTE_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].reqAndReqTime, THREE_REMOTE_CTRL_PRESENT, \
		                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
		        break;
        }
      }
      	
      //�вɼ����Ĳ������α�������(��ǰ)
      if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
      {
			 	if (multiCopy[arrayItem].protocol==KEY_HOUSEHOLD_2007)
			 	{
			 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                             multiCopy[arrayItem].paraVariable, KEY_HOUSEHOLD_PRESENT, \
		                              PARA_VARIABLE_DATA, LENGTH_OF_KEY_PARA_RECORD);
        }
      }
           
	  	if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_ENERGY)
	  	{
	    	switch (multiCopy[arrayItem].protocol)
	 			{
			 	  case SINGLE_PHASE_645_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_PHASE_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_PHASE_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	}
					 	break;

		  		case SINGLE_LOCAL_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_LOCAL_CTRL_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_LOCAL_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	   multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_LOCAL_CTRL_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_LOCAL_RECORD);
					 	}
					 	break;

		  		case SINGLE_REMOTE_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_REMOTE_CTRL_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_REMOTE_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_REMOTE_CTRL_MONTH, \
					 	                               MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_REMOTE_RECORD);
					 	}
					 	break;
		 	 
		  		case THREE_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, THREE_DAY, \
					 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, THREE_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_ENERGY_RECORD);
					 	}
					 	break;

		  		case THREE_LOCAL_CHARGE_CTRL_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, THREE_LOCAL_CTRL_DAY, \
					 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_LOCAL_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, THREE_LOCAL_CTRL_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_LOCAL_RECORD);
					 	}
					 	break;

		  		case THREE_REMOTE_CHARGE_CTRL_2007:
				  	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
				 	                             multiCopy[arrayItem].energy, THREE_REMOTE_CTRL_DAY, \
				 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_REMOTE_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, THREE_REMOTE_CTRL_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_THREE_REMOTE_RECORD);
					 	}
					 	break;
		 	 
		  		case KEY_HOUSEHOLD_2007:
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, KEY_HOUSEHOLD_DAY, \
					 	                               DAY_FREEZE_COPY_DATA_M, LENGTH_OF_KEY_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, KEY_HOUSEHOLD_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_KEY_ENERGY_RECORD);
					 	}
					 	break;
        }
	  	}
       
      //�вɼ�������������(��һ���ն�������)
      if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_REQ)
      {
	 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                             multiCopy[arrayItem].reqAndReqTime, DAY_BALANCE, \
	 	                               DAY_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
	 	    if (multiCopy[arrayItem].copyTime.day==0x1)
	 	    {
	 	      multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
	 	                             multiCopy[arrayItem].reqAndReqTime, MONTH_BALANCE, \
	 	                               MONTH_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
	 	    }
      }

      if (multiCopy[arrayItem].hasData>0)
      {
		    multiCopy[arrayItem].hasData = 0x0;
		    return COPY_COMPLETE_SAVE_DATA;
	  	}
	  	else
	  	{
	    	multiCopy[arrayItem].hasData = 0x0;
	 			return COPY_COMPLETE_NOT_SAVE;
	  	}
    }
    else
    {
	  	switch(multiCopy[arrayItem].protocol)
	  	{
	 			case SINGLE_PHASE_645_1997:
	 			case SINGLE_PHASE_645_2007:
				  //�вɼ����ĵ�ǰ����������
				  if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
				  {
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_PHASE_PRESENT, \
					 	                               ENERGY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
				  }

				  //�вɼ�������һ�յ���������
				  if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_ENERGY)
				  {
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                             multiCopy[arrayItem].energy, SINGLE_PHASE_DAY, \
					 	                               DAY_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	if (multiCopy[arrayItem].copyTime.day==0x1)
					 	{
					 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                              multiCopy[arrayItem].energy, SINGLE_PHASE_MONTH, \
					 	                                MONTH_FREEZE_COPY_DATA, LENGTH_OF_SINGLE_ENERGY_RECORD);
					 	}
				  }
    
          if (multiCopy[arrayItem].hasData>0)
          {		 
	       		multiCopy[arrayItem].hasData = 0x0;
	       		return COPY_COMPLETE_SAVE_DATA;
       	  }
				  else
				  {
				    multiCopy[arrayItem].hasData = 0x0;
				 		return COPY_COMPLETE_NOT_SAVE;
				  }
			 	  break;
   		 	   
			 	case DLT_645_1997:
			 	case DLT_645_2007:
			 	case PN_WORK_NOWORK_1997:
			 	case PN_WORK_NOWORK_2007:
				  //�вɼ����ĵ���������(����/��һ������)
				  if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_ENERGY)
				  {
					 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
					 	                           multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
					 	                            POWER_PARA_LASTMONTH, LENGTH_OF_ENERGY_RECORD);
				  }
      
      	  //�вɼ���������������ʱ������(���»���һ������)
          if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_REQ)
          {
			 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
			 	                             multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
			 	                               REQ_REQTIME_LASTMONTH, LENGTH_OF_REQ_RECORD);
		      }
		      
		      //�вɼ����ĵ���������(��һ���ն�������)
		      if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_ENERGY)
		      {
			 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
			 	                               multiCopy[arrayItem].energy, DAY_BALANCE, \
			 	                                 DAY_FREEZE_COPY_DATA_M, LENGTH_OF_ENERGY_RECORD);
			 	    if (multiCopy[arrayItem].copyTime.day==0x1)
			 	    {
			 	       multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
			 	                                 multiCopy[arrayItem].energy, MONTH_BALANCE, \
			 	                                   MONTH_FREEZE_COPY_DATA_M, LENGTH_OF_ENERGY_RECORD);
			 	    }
			    }
		   
	  		  //�вɼ�������������(��һ���ն�������)
	  		  if (multiCopy[arrayItem].hasData&HAS_LAST_DAY_REQ)
	  		  {
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		  		 	                           multiCopy[arrayItem].reqAndReqTime, DAY_BALANCE, \
		  		 	                             DAY_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
		  		 	if (multiCopy[arrayItem].copyTime.day==0x1)
		  		 	{
		  		 	  multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		  		 	                             multiCopy[arrayItem].reqAndReqTime, MONTH_BALANCE, \
		  		 	                               MONTH_FREEZE_COPY_REQ_M, LENGTH_OF_REQ_RECORD);
		  		 	}
		  		}

	  		  //�вɼ����ĵ���������(��ǰ)
	  		  if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
	  		  {
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                       multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
		                                         ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
		  		}
		      	   
      	  //�вɼ���������������ʱ������(��ǰ)
          if ((multiCopy[arrayItem].hasData&HAS_CURRENT_REQ))
          {
	      		multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
	                                       multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
	                                        REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
          }
		           
      	  //�вɼ����Ĳ������α�������(��ǰ)
          if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
          {
		    		//����DLT_645_2007�������ܶ���������ۼƶ���ʱ�估���һ�ζ���ʱ��,����õ� 10-07-05
		             
            if (multiCopy[arrayItem].protocol == DLT_645_2007)
            {
              //�ֲ�ȡ�İ취����A,B,C��Ĵ�����ʱ������������Ϊ�ܶ���������ۼ�ʱ��
           	  pParaData = multiCopy[arrayItem].paraVariable;
           	
           	  //�����ܶ������
           	  tmpData = 0;
           	  if (pParaData[PHASE_A_DOWN_TIMES]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[PHASE_A_DOWN_TIMES] | pParaData[PHASE_A_DOWN_TIMES+1]<<8 | pParaData[PHASE_A_DOWN_TIMES+2]<<16);
           	  }
           	  if (pParaData[PHASE_B_DOWN_TIMES]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[PHASE_B_DOWN_TIMES] | pParaData[PHASE_B_DOWN_TIMES+1]<<8 | pParaData[PHASE_B_DOWN_TIMES+2]<<16);
           	  }
           	  if (pParaData[PHASE_A_DOWN_TIMES]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[PHASE_C_DOWN_TIMES] | pParaData[PHASE_C_DOWN_TIMES+1]<<8 | pParaData[PHASE_C_DOWN_TIMES+2]<<16);
           	  }
           	  tmpData = hexToBcd(tmpData);
           	  pParaData[PHASE_DOWN_TIMES]   = tmpData&0xff;
           	  pParaData[PHASE_DOWN_TIMES+1] = tmpData>>8&0xff;
           	  pParaData[PHASE_DOWN_TIMES+2] = tmpData>>16&0xff;
    
           	  //�����ۼƶ���ʱ��
           	  tmpData = 0;
           	  if (pParaData[TOTAL_PHASE_A_DOWN_TIME]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_A_DOWN_TIME] | pParaData[TOTAL_PHASE_A_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_A_DOWN_TIME+2]<<16);
           	  }
           	  if (pParaData[TOTAL_PHASE_B_DOWN_TIME]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_B_DOWN_TIME] | pParaData[TOTAL_PHASE_B_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_B_DOWN_TIME+2]<<16);
           	  }
           	  if (pParaData[TOTAL_PHASE_B_DOWN_TIME]!=0xee)
           	  {
           	    tmpData += bcdToHex(pParaData[TOTAL_PHASE_C_DOWN_TIME] | pParaData[TOTAL_PHASE_C_DOWN_TIME+1]<<8 | pParaData[TOTAL_PHASE_C_DOWN_TIME+2]<<16);
           	  }
           	  tmpData = hexToBcd(tmpData);
           	  pParaData[TOTAL_PHASE_DOWN_TIME] = tmpData;
           	  pParaData[TOTAL_PHASE_DOWN_TIME+1] = tmpData>>8;
           	  pParaData[TOTAL_PHASE_DOWN_TIME+2] = tmpData>>16;
    
           	  //���һ�ζ�����ʼʱ��
           	  if (pParaData[LAST_PHASE_A_DOWN_BEGIN]!=0xee && pParaData[LAST_PHASE_B_DOWN_BEGIN]!=0xee && pParaData[LAST_PHASE_C_DOWN_BEGIN]!=0xee)
           	  {
               	memcpy((INT8U *)&tmpTimeA,pParaData+LAST_PHASE_A_DOWN_BEGIN,6);
               	memcpy((INT8U *)&tmpTimeB,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	memcpy((INT8U *)&tmpTimeC,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	tmpTimeA = timeBcdToHex(tmpTimeA);
               	tmpTimeB = timeBcdToHex(tmpTimeB);
               	tmpTimeC = timeBcdToHex(tmpTimeC);
               	
               	if (compareTwoTime(tmpTimeA, tmpTimeB))       //B>A
               	{
               	  if (compareTwoTime(tmpTimeC, tmpTimeB))     //B>C 
               	  {
                    memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	  }
               	  else
               	  {
               	    if (compareTwoTime(tmpTimeB, tmpTimeC))   //C>B
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	    }
               	    else                                      //C=B
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	    }
               	  }
               	}
               	else                                       //B<=A
               	{
               	  if (compareTwoTime(tmpTimeB, tmpTimeA))  //A>B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeA))  //A>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_A_DOWN_BEGIN,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	    }
               	  }
               	  else                                      //A<=B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeB))  //B>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_B_DOWN_BEGIN,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_BEGIN,pParaData+LAST_PHASE_C_DOWN_BEGIN,6);
               	    }
               	  }
               	}
              }         	
   
             	 
             	//���һ�ζ������ʱ��
           	  if (pParaData[LAST_PHASE_A_DOWN_END]!=0xee && pParaData[LAST_PHASE_B_DOWN_END]!=0xee && pParaData[LAST_PHASE_C_DOWN_END]!=0xee)
           	  {
               	memcpy((INT8U *)&tmpTimeA,pParaData+LAST_PHASE_A_DOWN_END,6);
               	memcpy((INT8U *)&tmpTimeB,pParaData+LAST_PHASE_B_DOWN_END,6);
               	memcpy((INT8U *)&tmpTimeC,pParaData+LAST_PHASE_C_DOWN_END,6);
               	tmpTimeA = timeBcdToHex(tmpTimeA);
               	tmpTimeB = timeBcdToHex(tmpTimeB);
               	tmpTimeC = timeBcdToHex(tmpTimeC);
               	
               	if (compareTwoTime(tmpTimeA, tmpTimeB))       //B>A
               	{
               	  if (compareTwoTime(tmpTimeC, tmpTimeB))     //B>C 
               	  {
                    memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
               	  }
               	  else
               	  {
               	    if (compareTwoTime(tmpTimeB, tmpTimeC))   //C>B
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
               	    }
               	    else                                      //C=B
               	    {
                        memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
               	    }
               	  }
               	}
               	else                                       //B<=A
               	{
               	  if (compareTwoTime(tmpTimeB, tmpTimeA))  //A>B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeA))  //A>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_A_DOWN_END,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
               	    }
               	  }
               	  else                                      //A<=B
               	  {
               	    if (compareTwoTime(tmpTimeC, tmpTimeB))  //B>C
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_B_DOWN_END,6);
               	    }
               	    else
               	    {
                      memcpy(pParaData+LAST_PHASE_DOWN_END,pParaData+LAST_PHASE_C_DOWN_END,6);
               	    }
               	  }
               	}
              }
            }
		   
	      		multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
	                                        multiCopy[arrayItem].paraVariable, multiCopy[arrayItem].copyDataType, \
	                                          PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
	        }
		       		
          //�вɼ�����ʱ������
          if (multiCopy[arrayItem].hasData&HAS_SHIDUAN)
          {
      			multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                       multiCopy[arrayItem].shiDuan, multiCopy[arrayItem].copyDataType, \
                                        SHI_DUAN_DATA, LENGTH_OF_SHIDUAN_RECORD);
          }

          if (multiCopy[arrayItem].hasData>0)
          {
	      		multiCopy[arrayItem].hasData = 0x0;
	      		return COPY_COMPLETE_SAVE_DATA;
      	  }
      	  else
      	  {
	      		multiCopy[arrayItem].hasData = 0x0;
	      		return COPY_COMPLETE_NOT_SAVE;
      	  }
      	  break;
      		
      	case EDMI_METER:
      	case SIMENS_ZD_METER:
      	case SIMENS_ZB_METER:
      	case ABB_METER:
	  		  //�вɼ����ĵ���������(����/��һ������)
	  		  if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_ENERGY)
	  		  {
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		  		 	                           multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
		  		 	                            POWER_PARA_LASTMONTH, LENGTH_OF_ENERGY_RECORD);
	  		  }
      
      	  //�вɼ���������������ʱ������(���»���һ������)
          if (multiCopy[arrayItem].hasData&HAS_LAST_MONTH_REQ)
          {
			 	    multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
			 	                               multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
			 	                                 REQ_REQTIME_LASTMONTH, LENGTH_OF_REQ_RECORD);
          }

  		    //�вɼ����ĵ���������(��ǰ)
  		    if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
  		    {
	  		 	  //ABB����Ҫ������ʾֵ
	  		 	  /*
	  		 	  if (multiCopy[arrayItem].protocol==ABB_METER)
	  		 	  {
		          offsetVision = POSITIVE_WORK_OFFSET;
		          for(i=0; i<4; i++)
		          {
		            sumDec = 0xeeeeee;
		            sumInt = 0xeeeeee;
		            for(j=1;j<5;j++)
		            {
		              if (multiCopy[arrayItem].energy[offsetVision+j*4]!=0xee)
		              {
		                if (sumDec==0xeeeeee)
		                {
		                  sumDec = bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4]);
		                }
		                else
		                {
		                  sumDec += bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4]);
		                }
		                
		                if (sumInt==0xeeeeee)
		                {
		                  sumInt = bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4+1] | multiCopy[arrayItem].energy[offsetVision+j*4+2]<<8 | multiCopy[arrayItem].energy[offsetVision+j*4+3]<<16);
		                }
		                else
		                {
		                	sumInt += bcdToHex(multiCopy[arrayItem].energy[offsetVision+j*4+1] | multiCopy[arrayItem].energy[offsetVision+j*4+2]<<8 | multiCopy[arrayItem].energy[offsetVision+j*4+3]<<16);
		                }
		              }
		            }
		            if (sumDec!=0xeeeeee)
		            {
		            	sumInt += sumDec/100;
		            	sumDec = sumDec%100;
		            	
		            	sumDec = hexToBcd(sumDec);
		            	sumInt = hexToBcd(sumInt);
		            	
		            	multiCopy[arrayItem].energy[offsetVision] = sumDec;
		            	multiCopy[arrayItem].energy[offsetVision+1] = sumInt&0xff;
		            	multiCopy[arrayItem].energy[offsetVision+2] = sumInt>>8&0xff;
		            	multiCopy[arrayItem].energy[offsetVision+3] = sumInt>>16&0xff;
		            }
		            
		            offsetVision += 36;
		          }
	  		 	  }
	  		 	  */
  		 	  
		  		 	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
		                                   multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
		                                    ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
  		    }

      	  //�вɼ���������������ʱ������(��ǰ)
          if ((multiCopy[arrayItem].hasData&HAS_CURRENT_REQ))
          {
				 	  //ABB����Ҫ�Ƚ��������������ʱ��
				 	  if (multiCopy[arrayItem].protocol==ABB_METER)
				 	  {
				 	  	offsetReq = REQ_POSITIVE_WORK_OFFSET;
				 	  	offsetReqTime = REQ_TIME_P_WORK_OFFSET;
				 	  	for (i=0; i<4; i++)
				 	    {
				 	    	tmpData = 0xeeeeeeee;
				 	    	foundLoc = 0xfe;
				 	    	for(j=1;j<5;j++)
				 	      {
				 	    	  if (multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3]!=0xee)
				 	    	  {
				 	    	    tmpDatax = multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3] | multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3+1]<<8 | multiCopy[arrayItem].reqAndReqTime[offsetReq+j*3+2]<<16;
				 	    	    if (tmpDatax>tmpData || tmpData==0xeeeeeeee)
				 	    	    {
				 	    	  	  foundLoc = j;
				 	    	  	  tmpData = tmpDatax;
				 	    	    }
				 	    	  }
				 	      }
				 	      
				 	      if (foundLoc<5)
				 	      {
				 	      	memcpy(&multiCopy[arrayItem].reqAndReqTime[offsetReq], &multiCopy[arrayItem].reqAndReqTime[offsetReq+foundLoc*3], 3);
				 	      	memcpy(&multiCopy[arrayItem].reqAndReqTime[offsetReqTime], &multiCopy[arrayItem].reqAndReqTime[offsetReqTime+foundLoc*5], 5);
				 	      }
				 	      
				 	      offsetReq += 72;
				 	      offsetReqTime += 72;
				 	    }
				 	  }
		 	 
 		 	      multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                        multiCopy[arrayItem].reqAndReqTime, multiCopy[arrayItem].copyDataType, \
                                         REQ_REQTIME_DATA, LENGTH_OF_REQ_RECORD);
          }

      	  //�вɼ����Ĳ������α�������(��ǰ)
          if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
          {
  		 	 		multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                       multiCopy[arrayItem].paraVariable, multiCopy[arrayItem].copyDataType, \
                                        PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
          }

          if (multiCopy[arrayItem].hasData>0)
          {		 
      		  multiCopy[arrayItem].hasData = 0x0;
      		    
      		  return COPY_COMPLETE_SAVE_DATA;
      	  }
      	  else
      	  {
      		  multiCopy[arrayItem].hasData = 0x0;
      		    
      		  return COPY_COMPLETE_NOT_SAVE;
      	  }

      	  break;
		   
       #ifdef PROTOCOL_MODUBUS_GROUP
		    case MODBUS_ASS:
		 		case MODBUS_HY:
				case MODBUS_XY_F:
		 		case MODBUS_XY_UI:
		 		case MODBUS_SWITCH:
		 		case MODBUS_XY_M:
				case MODBUS_T_H:
				case MODBUS_MW_F:
				case MODBUS_MW_UI:
				case MODBUS_JZ_F:
				case MODBUS_JZ_UI:
				case MODBUS_PMC350_F:
				case MODBUS_WE6301_F:
		   		//�вɼ����ĵ���������(��ǰ)
		   		if (multiCopy[arrayItem].hasData&HAS_CURRENT_ENERGY)
		   		{
			 			multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
									    multiCopy[arrayItem].energy, multiCopy[arrayItem].copyDataType, \
										  	ENERGY_DATA,LENGTH_OF_ENERGY_RECORD);
		   		}

		   		//�вɼ����Ĳ������α�������(��ǰ)
		   		if (multiCopy[arrayItem].hasData&HAS_PARA_VARIABLE)
		   		{
      			multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime, \
                                       multiCopy[arrayItem].paraVariable, multiCopy[arrayItem].copyDataType, \
                                          PARA_VARIABLE_DATA, LENGTH_OF_PARA_RECORD);
			 
					  multiCopy[arrayItem].hasData = 0x0;
					  return COPY_COMPLETE_SAVE_DATA;
		   		}

		   		multiCopy[arrayItem].hasData = 0x0;
      	  return COPY_COMPLETE_NOT_SAVE;
		   
		   		break;
		   #endif
		
      }
   	}
  }
	
  frameTail = 0;
  switch(multiCopy[arrayItem].protocol)
  {
   #ifdef PROTOCOL_645_1997
 	  case DLT_645_1997:                   //DL/T645-1997
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_97
 	  case SINGLE_PHASE_645_1997:          //DL/T645-1997
   #endif
   #ifdef PROTOCOL_645_2007
 	  case DLT_645_2007:                   //DL/T645-2007
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_07
 	  case SINGLE_PHASE_645_2007:          //DL/T645-2007�������ܱ�
 	  case SINGLE_PHASE_645_2007_TARIFF:   //DL/T645-2007�������ܱ�(��ʵʱ�����ܼ�������)
 	  case SINGLE_PHASE_645_2007_TOTAL:    //DL/T645-2007�������ܱ�(��ʵʱ������ʾֵ)
   #endif
   #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
 	  case SINGLE_LOCAL_CHARGE_CTRL_2007:  //DL/T645-2007���౾�طѿر�
   #endif
   #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
 	  case SINGLE_REMOTE_CHARGE_CTRL_2007: //DL/T645-2007����Զ�̷ѿر�
   #endif
   #ifdef PROTOCOL_THREE_2007
 	  case THREE_2007:                     //DL/T645-2007�������ܱ�
   #endif
   #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
 	  case THREE_REMOTE_CHARGE_CTRL_2007:  //DL/T645-2007����Զ�̷ѿر�
   #endif
   #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
 	  case THREE_LOCAL_CHARGE_CTRL_2007:   //DL/T645-2007���౾�طѿر�
   #endif
   #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
 	  case KEY_HOUSEHOLD_2007:             //DL/T645-2007�ص��û�
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_97
 	  case PN_WORK_NOWORK_1997:            //97��ֻ�����������޹�����ʾֵ
   #endif    
   #ifdef PROTOCOL_PN_WORK_NOWORK_07
 	  case PN_WORK_NOWORK_2007:            //07��ֻ�����������޹�����ʾֵ
   #endif
     
    case DOT_COPY_1997:                  //97�㳭��Լ
    case DOT_COPY_2007:                  //07�㳭��Լ
    case HOUR_FREEZE_2007:               //07���㶳���Լ
    
   	 #if PROTOCOL_645_1997 || PROTOCOL_645_2007 || PROTOCOL_SINGLE_PHASE_97 || PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007 || PROTOCOL_KEY_HOUSEHOLD_2007

   	  sendBuf[0] = 0x68;
        
      //��д��ַ��
      sendBuf[1] = multiCopy[arrayItem].meterAddr[0];
      sendBuf[2] = multiCopy[arrayItem].meterAddr[1];
      sendBuf[3] = multiCopy[arrayItem].meterAddr[2];
      sendBuf[4] = multiCopy[arrayItem].meterAddr[3];
      sendBuf[5] = multiCopy[arrayItem].meterAddr[4];
      sendBuf[6] = multiCopy[arrayItem].meterAddr[5];
       
      sendBuf[7] = 0x68;
       
      //�����ֶ�
      if (multiCopy[arrayItem].protocol==DLT_645_1997 | multiCopy[arrayItem].protocol==SINGLE_PHASE_645_1997 | multiCopy[arrayItem].protocol==DOT_COPY_1997 | multiCopy[arrayItem].protocol==PN_WORK_NOWORK_1997)
      {
        sendBuf[8] = READ_DATA_645_1997;
      }
      else
      {
        sendBuf[8] = READ_DATA_645_2007;       	  
      }

      //��д������
      frameTail = 9;
      switch(multiCopy[arrayItem].copyDataType)
      {
       	case  PRESENT_DATA:    //��ǰ���ݳ�����
          switch (multiCopy[arrayItem].protocol)
          {
            case DLT_645_1997:
              sendBuf[frameTail++] = 0x2;             //����
              sendBuf[frameTail++] = cmdDlt645Current1997[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI0
              sendBuf[frameTail++] = cmdDlt645Current1997[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI1
              break;

            case SINGLE_PHASE_645_1997:
              sendBuf[frameTail++] = 0x2;             //����
              sendBuf[frameTail++] = single1997[multiCopy[arrayItem].copyItem-1][1]+0x33;               //DI0
              sendBuf[frameTail++] = single1997[multiCopy[arrayItem].copyItem-1][0]+0x33;               //DI1
              break;

            case PN_WORK_NOWORK_1997:
              sendBuf[frameTail++] = 0x2;             //����
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork1997[multiCopy[arrayItem].copyItem-1][1]+0x33; //DI0
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork1997[multiCopy[arrayItem].copyItem-1][0]+0x33; //DI1
              break;
              
            case DLT_645_2007:
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][3]+0x33;     //DI0
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][2]+0x33;     //DI1            	
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI2
              sendBuf[frameTail++] = cmdDlt645Current2007[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI3
              break;
              
            case SINGLE_PHASE_645_2007:
            case SINGLE_LOCAL_CHARGE_CTRL_2007:
            case SINGLE_REMOTE_CHARGE_CTRL_2007:
            case SINGLE_PHASE_645_2007_TARIFF:    //��ʵʱ�����ܼ�������
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][3]+0x33;               //DI0
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][2]+0x33;               //DI1            	
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][1]+0x33;               //DI2
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem-1][0]+0x33;               //DI3
              break;

            case SINGLE_PHASE_645_2007_TOTAL:     //��ʵʱ������ʾֵ
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][3]+0x33;               //DI0
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][2]+0x33;               //DI1            	
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][1]+0x33;               //DI2
              sendBuf[frameTail++] = single2007[multiCopy[arrayItem].copyItem+1][0]+0x33;               //DI3
              break;

            case PN_WORK_NOWORK_2007:
          	sendBuf[frameTail++] = 0x04;             //����
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][3]+0x33; //DI0
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][2]+0x33; //DI1            	
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][1]+0x33; //DI2
              sendBuf[frameTail++] = cmdDlt645pnWorkNwork2007[multiCopy[arrayItem].copyItem-1][0]+0x33; //DI3
              break;

            case THREE_2007:
            case THREE_LOCAL_CHARGE_CTRL_2007:
            case THREE_REMOTE_CHARGE_CTRL_2007:
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][3]+0x33;                //DI0
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][2]+0x33;                //DI1            	
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][1]+0x33;                //DI2
              sendBuf[frameTail++] = three2007[multiCopy[arrayItem].copyItem-1][0]+0x33;                //DI3
              break;

            case KEY_HOUSEHOLD_2007:
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][3]+0x33;         //DI0
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][2]+0x33;         //DI1            	
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][1]+0x33;         //DI2
              sendBuf[frameTail++] = keyHousehold2007[multiCopy[arrayItem].copyItem-1][0]+0x33;         //DI3
              break;

            case DOT_COPY_2007:
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][3]+0x33;              //DI0
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][2]+0x33;              //DI1            	
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][1]+0x33;              //DI2
              sendBuf[frameTail++] = dotCopy2007[multiCopy[arrayItem].copyItem-1][0]+0x33;              //DI3
              break;

            case DOT_COPY_1997:
              sendBuf[frameTail++] = 0x2;             //����
              sendBuf[frameTail++] = dotCopy1997[multiCopy[arrayItem].copyItem-1][1]+0x33;              //DI0
              sendBuf[frameTail++] = dotCopy1997[multiCopy[arrayItem].copyItem-1][0]+0x33;              //DI1
              break;
              
            default:
            	return PROTOCOL_NOT_SUPPORT;
            	break;
          }
          break;
              
        case LAST_MONTH_DATA:  //�������ݳ�����
          switch (multiCopy[arrayItem].protocol)            
          {
            case DLT_645_1997:
            case PN_WORK_NOWORK_1997:
              sendBuf[frameTail++] = 0x2;             //����
              sendBuf[frameTail++] = cmdDlt645LastMonth1997[multiCopy[arrayItem].copyItem-1][1]+0x33;//DI0
              sendBuf[frameTail++] = cmdDlt645LastMonth1997[multiCopy[arrayItem].copyItem-1][0]+0x33;//DI1
              break;
              
            case DLT_645_2007:
            case PN_WORK_NOWORK_2007:
              sendBuf[frameTail++] = 0x4;           //����

              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][3]+0x33;//DI0
              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][2]+0x33;//DI1
              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][1]+0x33;//DI2
              sendBuf[frameTail++] = cmdDlt645LastMonth2007[multiCopy[arrayItem].copyItem-1][0]+0x33;//DI3
              break;
              
            default:
            	return PROTOCOL_NOT_SUPPORT;
            	break;
          }
          break;

        case LAST_DAY_DATA:    //��һ���ն�������
        	switch(multiCopy[arrayItem].protocol)
          {
            case SINGLE_PHASE_645_1997:
              sendBuf[frameTail++] = 0x2;             //����
              sendBuf[frameTail++] = single1997LastDay[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI0
              sendBuf[frameTail++] = single1997LastDay[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI1
              break;
              
            case SINGLE_PHASE_645_2007:
            case SINGLE_LOCAL_CHARGE_CTRL_2007:
            case SINGLE_REMOTE_CHARGE_CTRL_2007:
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][3]+0x33;     //DI0
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][2]+0x33;     //DI1            	
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI2
              sendBuf[frameTail++] = single2007LastDay[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI3
              break;
              
            case DLT_645_2007:
            case THREE_2007:
            case THREE_LOCAL_CHARGE_CTRL_2007:
            case THREE_REMOTE_CHARGE_CTRL_2007:
            case KEY_HOUSEHOLD_2007:
              sendBuf[frameTail++] = 0x4;             //����
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][3]+0x33;     //DI0
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][2]+0x33;     //DI1            	
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][1]+0x33;     //DI2
              sendBuf[frameTail++] = cmdDlt645LastDay2007[multiCopy[arrayItem].copyItem-1][0]+0x33;     //DI3
              break;

            default:
            	return PROTOCOL_NOT_SUPPORT;
            	break;
          }
        	break;
          	
        case HOUR_FREEZE_DATA:
          sendBuf[frameTail++] = 0x4;             //����
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][3]+0x33;              //DI0
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][2]+0x33;              //DI1            	
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][1]+0x33;              //DI2
          sendBuf[frameTail++] = hourFreeze2007[multiCopy[arrayItem].copyItem-1][0]+0x33;              //DI3
        	break;
      }
       
      //Checksum
      sendBuf[frameTail] = 0x00;
      for (i = 0; i < frameTail; i++)
      {
        sendBuf[frameTail] += sendBuf[i];
      }
         
      frameTail++;
      sendBuf[frameTail++] = 0x16;

      //if (port==31)
      //2013-12-24,�ൺ���Ų��Է���,B,C��ı�֡ǰ�����4��FE,������Ҫ�󲻼�,�޸�����ж�
      //           ������3��ͬ����,���Ի���ֶ˿�31,32,33
      if (port>=31)
      {
        memcpy(tmpSendBuf, sendBuf, frameTail);
      }
      else  //485�˿ڼ�4��0xfe
      {
       #ifdef DKY_SUBMISSION
        memcpy(tmpSendBuf, sendBuf, frameTail);
       #else
        memcpy(&tmpSendBuf[4], sendBuf, frameTail);
        tmpSendBuf[0] = 0xfe;
        tmpSendBuf[1] = 0xfe;
        tmpSendBuf[2] = 0xfe;
        tmpSendBuf[3] = 0xfe;
     
        frameTail += 4;
       #endif
      }
	    break;
	   #endif   //PROTOCOL_645_1997

	 #ifdef PROTOCOL_EDMI_GROUP 	 
	  case EDMI_METER:    //����(EDMI)��
      //02 45 0C A3 76 06 00 00 00 00 FF FF 95 A8 03
 	    frameTail = 0;
 	    sendBuf[frameTail++] = EDMI_STX;
 	    sendBuf[frameTail++] = 0x45;    //��־λ(��ʾ���Բɼ�һ�Զ෽ʽ[RS485,RS422])
 	 
 	    //���ַ,(4�ֽھ��Ǳ����к�[��212039174]ת����16����)
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[0];
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[1];
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[2];
 	    sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[3];
       
      //4���ֽڵ�Դ��ַ(�����ⶨ��)
      sendBuf[frameTail++] = 0x00;
      sendBuf[frameTail++] = 0x00;
      sendBuf[frameTail++] = 0x00;
      sendBuf[frameTail++] = 0x00;
       
      //2���ֽڵ��ط����
      sendBuf[frameTail++] = 0xFF;
      sendBuf[frameTail++] = 0xFF;

      if (multiCopy[arrayItem].copyDataType==LAST_MONTH_DATA && multiCopy[arrayItem].copyItem==TOTAL_COMMAND_LASTMONTH_EDMI)
      {
  	 	  sendBuf[frameTail++] = 'X';
  	 	  sendBuf[frameTail++] = 0x0;
      }
      else
      {
  	 	  switch (multiCopy[arrayItem].copyItem)
  	 	  {
          case 1:
           	break;
           	 
          case 2:    //��������
            //02 45 0C A3 76 06 00 00 00 00 FF FF 4C 45 44 4D 49 2C 49 4D 44 45 49 4D 44 45 00 97 0C 03
	 	 	      sendBuf[frameTail++] = 'L';
	 	 	      sendBuf[frameTail++] = 'E';
	 	 	      sendBuf[frameTail++] = 'D';
	 	 	      sendBuf[frameTail++] = 'M';
	 	 	      sendBuf[frameTail++] = 'I';
	 	 	      sendBuf[frameTail++] = ',';
	 	 	      sendBuf[frameTail++] = 'I';
	 	 	      sendBuf[frameTail++] = 'M';
	 	 	      sendBuf[frameTail++] = 'D';
	 	 	      sendBuf[frameTail++] = 'E';
	 	 	      sendBuf[frameTail++] = 'I';
	 	 	      sendBuf[frameTail++] = 'M';
	 	 	      sendBuf[frameTail++] = 'D';
	 	 	      sendBuf[frameTail++] = 'E';
	 	 	      sendBuf[frameTail++] = 0x0;
	 	 	      break;
  	 	 	 	 
  	 	    case TOTAL_COMMAND_REAL_EDMI:      //�˳�����
  	 	 	    sendBuf[frameTail++] = 'X';
  	 	 	 		sendBuf[frameTail++] = 0x0;
  	 	 	 		break;
  	 	 	 	   
  	 	    default:
  	 	 	 		sendBuf[frameTail++] = 'R';
            if (multiCopy[arrayItem].copyDataType==LAST_MONTH_DATA)
            {
  	 	 	      sendBuf[frameTail++] = cmdEdmiLastMonth[multiCopy[arrayItem].copyItem-1][0];
  	 	 	      sendBuf[frameTail++] = cmdEdmiLastMonth[multiCopy[arrayItem].copyItem-1][1];
            }
            else
            {
  	 	 	      sendBuf[frameTail++] = cmdEdmi[multiCopy[arrayItem].copyItem-1][0];
  	 	 	      sendBuf[frameTail++] = cmdEdmi[multiCopy[arrayItem].copyItem-1][1];
  	 	 	    }
  	 	 	    break;
  	 	  }
  	  }
	 	 	 
 	 		crcResult = edmi_CRC_16(sendBuf, frameTail);
 	 
		 	sendBuf[frameTail++] = crcResult>>8;
		 	sendBuf[frameTail++] = crcResult;
		 	sendBuf[frameTail++] = EDMI_ETX;

 	 		edmiTail = 0;
 	 		for(i=0; i<frameTail;i++)
 	 		{
        if (i==0 || i==(frameTail-1))
        {
     	 		tmpSendBuf[edmiTail++] = sendBuf[i];
        }
        else
        {
			 	 	switch (sendBuf[i])
			 	 	{
			      case EDMI_STX:
		          case EDMI_ETX:
		          case EDMI_DLE:
		          case EDMI_XON:
		          case EDMI_XOFF:
		 	 	      tmpSendBuf[edmiTail++] = EDMI_DLE;
		 	 	      tmpSendBuf[edmiTail++] = sendBuf[i]|0x40;
		 	 	      break;
				 	 	 	 	   
			 	 	  default:
			 	 	    tmpSendBuf[edmiTail++] = sendBuf[i];
			 	 	    break;
			 	 	}
 	   		}
      }
      frameTail = edmiTail;
	    break;
   #endif

	  case SIMENS_ZD_METER:    //������(/������)ZD��
	  case SIMENS_ZB_METER:    //������(/������)ZB��
  	  switch (multiCopy[arrayItem].copyItem)
  	  {
	      case 1:    //������Ϣ
          //2F 3F 38 34 32 38 35 36 37 34 21 0D 0A(/?84285674!)
 	        frameTail = 0;
 	     
 	        sendBuf[frameTail++] = '/';
 	        sendBuf[frameTail++] = '?';
 	        for(i=6;i>0;i--)
 	        {
 	     	    if (multiCopy[arrayItem].meterAddr[i-1]!=0x00)
 	     	    {
 	     	      sprintf(tmpStr,"%02d", bcdToHex(multiCopy[arrayItem].meterAddr[i-1]));
 	     	      sendBuf[frameTail++] = tmpStr[0];
 	     	      sendBuf[frameTail++] = tmpStr[1];
 	     	 		}
 	        }
 	        sendBuf[frameTail++] = '!';
 	        sendBuf[frameTail++] = 0x0D;
 	        sendBuf[frameTail++] = 0x0A;
 	     
 	        memcpy(tmpSendBuf, sendBuf, frameTail);
 	        break;
 	   
 	      case 2:    //ѡ����Ϣ
 	   	    //06 30 35 30 0D 0A(ACK  0  5  0)
 	   	    tmpSendBuf[0] = 0x06;
 	   	    tmpSendBuf[1] = 0x30;
 	   	    tmpSendBuf[2] = 0x35; //9600
 	   	    tmpSendBuf[3] = 0x30;
 	   	    tmpSendBuf[4] = 0x0D;
 	   	    tmpSendBuf[5] = 0x0A;

          multiCopy[arrayItem].send(multiCopy[arrayItem].port, tmpSendBuf, 6);
 	   	 
 	   	    return CHANGE_METER_RATE;
 	   	    break;
 	    }
 	 
 	    break;

	 #ifdef PROTOCOL_ABB_GROUP

	  case ABB_METER:         //ABB����
	    frameTail = 0;

 	    switch (multiCopy[arrayItem].copyItem)
 	    {
	      case 1:    //����
	      case 2:    //����
	      case 3:    //����
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x18;
	        sendBuf[frameTail++] = 0x06;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x01;
	        sendBuf[frameTail++] = multiCopy[arrayItem].meterAddr[0];
	        break;
	     
	      case 4:    //����У������
	        if (abbHandclaspOk==0)
	        {
	       	  multiCopy[arrayItem].copyItem = TOTAL_COMMAND_REAL_ABB+1;
	       	  
	       	  goto frameingPoint;
	        }
	       
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x18;
	        sendBuf[frameTail++] = 0x01;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x04;
	     	 
          //rAbbMeterKey = 0x04367435;
	     	 
     	    abbKey = abbEncryption(0x0, rAbbMeterKey);
     	 
     	    sendBuf[frameTail++] = abbKey>>24;
     	    sendBuf[frameTail++] = abbKey>>16;
     	    sendBuf[frameTail++] = abbKey>>8;
     	    sendBuf[frameTail++] = abbKey&0xff;
     	    break;
	     	 
	      case 5:  //02 05 00 00 00 00 00 00 F6 01
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        break;
	     	 
	      case 6:    //02 05 00 00 00 00 00 02 xx xx 
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;	 	       
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;	 	       
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x02;	 	       
	        sendBuf[frameTail++] = 0x02;
	        break;
	       
	      case 7:
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x81;
	        break;

	      case 8:    //02 05 00 00 00 00 00 09 67 28 
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x09;
	        break;

	      case 9:    //02 05 00 00 00 00 00 0B 47 6A
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x05;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x00;
	        sendBuf[frameTail++] = 0x0B;
	        break;
	     
	      case 10:
	      case 11:
	      case 12:
	      case 13:
	      case 14:
	      case 15:
	      case 16:
	      case 17:
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x81;
	        break;
	     
	      case 18:    //����
	        sendBuf[frameTail++] = 0x02;
	        sendBuf[frameTail++] = 0x80;
	       
	        multiCopy[arrayItem].copyItem++;
	        break;
 	    }
	   
	    crcResult = CRC_ABB(sendBuf, frameTail);
	    sendBuf[frameTail++] = crcResult>>8;
	    sendBuf[frameTail++] = crcResult&0xff;
	 	 
	    memcpy(tmpSendBuf, sendBuf, frameTail);
	 	 
	    if (multiCopy[arrayItem].copyItem==TOTAL_COMMAND_REAL_ABB+1)
	    {
	 	    multiCopy[arrayItem].send(multiCopy[arrayItem].port,tmpSendBuf,frameTail);
	 	 	 
	 	    goto frameingPoint;
	    }
	    break;
	 #endif

	 #ifdef PROTOCOL_MODUBUS_GROUP
	  case MODBUS_HY:		  //��Զmodbus��
	    tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	  //��ַ
	    tmpSendBuf[1] = 0x03;    //����
	    if (1==multiCopy[arrayItem].copyItem)    //�α���
	    {
	      tmpSendBuf[2] = 0x10;    //�Ĵ�����ַ
	      tmpSendBuf[3] = 0x00;    //
	      tmpSendBuf[4] = 0x00;    //�Ĵ�������
	      tmpSendBuf[5] = 0x1f;    //
	    }
	    else
	    {
		    tmpSendBuf[2] = 0x40;	//�Ĵ�����ַ
		    tmpSendBuf[3] = 0x00;	//
		    tmpSendBuf[4] = 0x00;	//�Ĵ�������
		 		tmpSendBuf[5] = 0x08;	//	   
	    }
	
	    tmpData = modbusCRC16(tmpSendBuf, 6);
	    tmpSendBuf[6] = tmpData>>8;
	   	tmpSendBuf[7] = tmpData&0xff;
	   	frameTail = 8;
	   	break;
	
	  case MODBUS_ASS:		   //��ɭ˼modbus��
	   	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);    //��ַ
	   	tmpSendBuf[1] = 0x03;    //����
	   	tmpSendBuf[2] = 0x01;    //�Ĵ�����ַ
	   	tmpSendBuf[3] = 0x00;    //
	   	tmpSendBuf[4] = 0x00;    //�Ĵ�������
	   	tmpSendBuf[5] = 0x22;    //

	   	tmpData = modbusCRC16(tmpSendBuf, 6);
	   	tmpSendBuf[6] = tmpData>>8;
	   	tmpSendBuf[7] = tmpData&0xff;
	   	frameTail = 8;
	   	break;
	   
	  case MODBUS_XY_F: 		 //�Ϻ���ҵmodbus�๦�ܱ�
		 	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
		  tmpSendBuf[1] = 0x03;	  //����
		 	tmpSendBuf[2] = 0x00;	  //�Ĵ�����ַ
		 	tmpSendBuf[3] = 0x06;	  //
		 	tmpSendBuf[4] = 0x00;	  //�Ĵ�������
		 	tmpSendBuf[5] = 0x38;	  //
	   	
		 	tmpData = modbusCRC16(tmpSendBuf, 6);
		 	tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
		 	frameTail = 8;
		 	break;
		 
	 	case MODBUS_XY_UI:		 //�Ϻ���ҵmodbus��ѹ������
		 	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
		 	tmpSendBuf[1] = 0x03;	  //����
		 	tmpSendBuf[2] = 0x00;	  //�Ĵ�����ַ
		 	tmpSendBuf[3] = 0x17;	  //
		 	tmpSendBuf[4] = 0x00;	  //�Ĵ�������
		 	tmpSendBuf[5] = 0x0c;	  //
	   
		 	tmpData = modbusCRC16(tmpSendBuf, 6);
		 	tmpSendBuf[6] = tmpData>>8;
		 	tmpSendBuf[7] = tmpData&0xff;
		 	frameTail = 8;
		 	break;
	   
	  case MODBUS_SWITCH: 	 //modbus������ģ��
		 	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
		 	tmpSendBuf[1] = 0x02;	  //����
		 	tmpSendBuf[2] = 0x00;	  //�Ĵ�����ַ
		 	tmpSendBuf[3] = 0x00;	  //
		 	tmpSendBuf[4] = 0x00;	  //�Ĵ�������
		 	tmpSendBuf[5] = 0x10;	  //
	   
		 	tmpData = modbusCRC16(tmpSendBuf, 6);
		 	tmpSendBuf[6] = tmpData>>8;
		 	tmpSendBuf[7] = tmpData&0xff;
		 	frameTail = 8;
		 	break;
		 
	 	case MODBUS_XY_M:		  //�Ϻ���ҵmodbus���ģ��
	   	tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);   //��ַ
	   	tmpSendBuf[1] = 0x03;	//����
	   	if (1==multiCopy[arrayItem].copyItem)	//��һ����,�Ĵ��������������Ϊ25��,��2�η���
	   	{
		 		tmpSendBuf[2] = 0x00;	  //�Ĵ�����ַ
		 		tmpSendBuf[3] = 0x0A;	  //
		 		tmpSendBuf[4] = 0x00;	  //�Ĵ�������
		 		tmpSendBuf[5] = 0x18;	  //
	   	}
	   	else
	   	{
		 		tmpSendBuf[2] = 0x00;	//�Ĵ�����ַ
		 		tmpSendBuf[3] = 0x22;	//
		 		tmpSendBuf[4] = 0x00;	//�Ĵ�������
		 		tmpSendBuf[5] = 0x18;	//	   
	   	}
	 
	   	tmpData = modbusCRC16(tmpSendBuf, 6);
	   	tmpSendBuf[6] = tmpData>>8;
	   	tmpSendBuf[7] = tmpData&0xff;
	   	frameTail = 8;
	   	break;
			
		case MODBUS_MW_F: 	//�ɶ�����modbus�๦�ܱ�
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
			tmpSendBuf[1] = 0x03; 	//����
			tmpSendBuf[2] = 0x10; 	//�Ĵ�����ַ
			tmpSendBuf[3] = 0x00; 	//
			tmpSendBuf[4] = 0x00; 	//�Ĵ�������
			tmpSendBuf[5] = 0x4A; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_MW_UI:	//�ɶ�����modbus��ѹ������
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
			tmpSendBuf[1] = 0x03; 	//����
			tmpSendBuf[2] = 0x10; 	//�Ĵ�����ַ
			tmpSendBuf[3] = 0x02; 	//
			tmpSendBuf[4] = 0x00; 	//�Ĵ�������
			tmpSendBuf[5] = 0x14; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;

		case MODBUS_JZ_F: 	//�Ϻ�����modbus�๦�ܱ�
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
			tmpSendBuf[1] = 0x03; 	//����
			tmpSendBuf[2] = 0x01; 	//�Ĵ�����ַ
			tmpSendBuf[3] = 0x30; 	//
			tmpSendBuf[4] = 0x00; 	//�Ĵ�������
			tmpSendBuf[5] = 0x2e; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_JZ_UI:	//�Ϻ�����modbus��ѹ������
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
			tmpSendBuf[1] = 0x03; 	//����
			tmpSendBuf[2] = 0x01; 	//�Ĵ�����ַ
			tmpSendBuf[3] = 0x31; 	//
			tmpSendBuf[4] = 0x00; 	//�Ĵ�������
			tmpSendBuf[5] = 0x0b; 	//
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_WE6301_F: 	//��˹��WE6301�๦�ܱ�
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
			tmpSendBuf[1] = 0x03; 	//����
			if (1==multiCopy[arrayItem].copyItem) 				//��һ����,��ѹ������������
			{
				tmpSendBuf[2] = 0x03; 	//�Ĵ�����ַ
				tmpSendBuf[3] = 0xe7; 	//
				tmpSendBuf[4] = 0x00; 	//�Ĵ�������
				tmpSendBuf[5] = 0x24; 	//
			}
			else if (2==multiCopy[arrayItem].copyItem)		//�ڶ�����,����
			{
				tmpSendBuf[2] = 0x03; 	//�Ĵ�����ַ
				tmpSendBuf[3] = 0x83; 	//
				tmpSendBuf[4] = 0x00; 	//�Ĵ�������
				tmpSendBuf[5] = 0x18; 	//
			}
			else if (3==multiCopy[arrayItem].copyItem)		//�ڶ�����,����
			{
				tmpSendBuf[2] = 0x04; 	//�Ĵ�����ַ
				tmpSendBuf[3] = 0x4b; 	//
				tmpSendBuf[4] = 0x00; 	//�Ĵ�������
				tmpSendBuf[5] = 0x0c; 	//
			}
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
			
		case MODBUS_PMC350_F: 	//�����е�PMC350�๦�ܱ�
			tmpSendBuf[0] = bcdToHex(multiCopy[arrayItem].meterAddr[0]);	 //��ַ
			tmpSendBuf[1] = 0x03; 	//����
			if (1==multiCopy[arrayItem].copyItem) 				//��һ����,��ѹ�������ʹ�������
			{
				tmpSendBuf[2] = 0x00; 	//�Ĵ�����ַ
				tmpSendBuf[3] = 0x00; 	//
				tmpSendBuf[4] = 0x00; 	//�Ĵ�������
				tmpSendBuf[5] = 0x3a; 	//
			}
			else if (2==multiCopy[arrayItem].copyItem)		//�ڶ�����,����
			{
				tmpSendBuf[2] = 0x01; 	//�Ĵ�����ַ
				tmpSendBuf[3] = 0xf4; 	//
				tmpSendBuf[4] = 0x00; 	//�Ĵ�������
				tmpSendBuf[5] = 0x0c; 	//
			}
		 
			tmpData = modbusCRC16(tmpSendBuf, 6);
			tmpSendBuf[6] = tmpData>>8;
			tmpSendBuf[7] = tmpData&0xff;
			frameTail = 8;
			break;
	 #endif

	  default:
	    return PROTOCOL_NOT_SUPPORT;
  }
  
  //��������
  multiCopy[arrayItem].send(multiCopy[arrayItem].port,tmpSendBuf,frameTail);
  
  return DATA_SENDED;
}

/*******************************************************
��������:meterReceiving
��������:��������֡����
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
INT8S meterReceiving(INT8U port,INT8U *data,INT16U recvLen)
{
  INT16U i,tmpi;
  INT8U  arrayItem;
  INT8U  checkSum;
  INT8S  ret=0;
   
  //for EDMI
  static char DLE_last;
  INT16U crcResult;
  
 #ifdef SHOW_DEBUG_INFO
  printf("meterReceiving:���ն˿�=%d,���ճ���=%d\n", port, recvLen);
 #endif

  if (port>0 && port<4)
  {
  	arrayItem = port-1;
  }
  else
  {
  	if (port>=31 && port<=33)
  	{
  	 	arrayItem = port-28;
  	}
  	else
  	{
  		return FALSE;
  	}
  }
	 
  switch (multiCopy[arrayItem].protocol)
  {
   #ifdef PROTOCOL_645_1997
   	case DLT_645_1997:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_97
   	case SINGLE_PHASE_645_1997:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_97
  	case PN_WORK_NOWORK_1997:
   #endif
   #ifdef PROTOCOL_645_2007
   	case DLT_645_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_07
   	case SINGLE_PHASE_645_2007:
    case SINGLE_PHASE_645_2007_TARIFF:
   	case SINGLE_PHASE_645_2007_TOTAL:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_07
   	case PN_WORK_NOWORK_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
    case SINGLE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
    case SINGLE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_2007
    case THREE_2007:
   #endif
   #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
   	case THREE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
    case THREE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
    case KEY_HOUSEHOLD_2007:
   #endif
  	   
	  case DOT_COPY_2007:
	  case DOT_COPY_1997:
	  case HOUR_FREEZE_2007:
  	   	
  	 #if PROTOCOL_645_1997 || PROTOCOL_645_2007 || SINGLE_PHASE_645_1997 || SINGLE_PHASE_645_2007 || SINGLE_LOCAL_CHARGE_CTRL_2007 || SINGLE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007 || PROTOCOL_KEY_HOUSEHOLD_2007
     	for(i=0;i<recvLen;i++)
     	{
       	multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail++] = data[i];
       
       	if (multiCopy[arrayItem].recvFrameTail==1)
       	{
       	 	if (multiCopy[arrayItem].meterRecvBuf[0]!=0x68)
       	 	{
       	 		multiCopy[arrayItem].recvFrameTail   = 0;
       	 		multiCopy[arrayItem].totalLenOfFrame = 2048;
       	 	}
       	}
      
       	if (multiCopy[arrayItem].recvFrameTail==8)
       	{
         	if (multiCopy[arrayItem].meterRecvBuf[7]!=0x68)
         	{
       	    multiCopy[arrayItem].recvFrameTail   = 0;
       	    multiCopy[arrayItem].totalLenOfFrame = 2048;
       	    break;
         	}
       	}
       
       	if (multiCopy[arrayItem].recvFrameTail==10)
       	{
       	  multiCopy[arrayItem].totalLenOfFrame = multiCopy[arrayItem].meterRecvBuf[9]+12;
       	  
       	  //2012-07-10,add
       	  if (multiCopy[arrayItem].totalLenOfFrame>510)
       	  {
       	    multiCopy[arrayItem].recvFrameTail   = 0;
       	    multiCopy[arrayItem].totalLenOfFrame = 2048;
       	    break;           	  	
       	  }
       	}
       
       	if (multiCopy[arrayItem].recvFrameTail==multiCopy[arrayItem].totalLenOfFrame)
       	{
        	//����У����Ұ��ֽڽ���������м�0x33����
         	checkSum = 0;
         	for(tmpi=0; tmpi<multiCopy[arrayItem].recvFrameTail-2; tmpi++)
         	{
           	checkSum += multiCopy[arrayItem].meterRecvBuf[tmpi];
            if (tmpi>9) // && flagOfForward == FALSE)
            {
              multiCopy[arrayItem].meterRecvBuf[tmpi] -= 0x33;  //���ֽڽ��ж��������0x33����
            }
         	}
         
         	//���У�����ȷ,ִ��meterInput����
         	if (checkSum == multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-2])
         	{
            ret = meterInput(arrayItem, multiCopy[arrayItem].meterRecvBuf, multiCopy[arrayItem].recvFrameTail);
         	}
         	else
         	{
         	  ret = RECV_DATA_CHECKSUM_ERROR;   //��������У��ʹ���
         	}
         
         	multiCopy[arrayItem].totalLenOfFrame = 2048;
         	multiCopy[arrayItem].recvFrameTail = 0;
	      }
       	else
       	{
       	  ret = RECV_DATA_NOT_COMPLETE;  //�������ݲ�����,�����ȴ�����
       	}
     	}
     	break;
     #endif
     
   #ifdef PROTOCOL_EDMI_GROUP
    case EDMI_METER:
     	ret = -7;
     	for(i=0; i<recvLen; i++)
     	{
       	multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail++] = data[i];
	
       	if (multiCopy[arrayItem].recvFrameTail==1)
       	{
       	 	if (multiCopy[arrayItem].meterRecvBuf[0]!=EDMI_STX)
       	 	{
       	   	multiCopy[arrayItem].recvFrameTail   = 0;
       	   	multiCopy[arrayItem].totalLenOfFrame = 2048;
       	 	  
       	   	continue;
       	 	}
       	}

       	if (DLE_last)
       	{
        	 multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-1] &= 0xBF;
         	DLE_last = FALSE;
       	}

       	if (data[i]==EDMI_DLE)
       	{
        	 multiCopy[arrayItem].recvFrameTail--;
       	 	DLE_last = TRUE;
       	}

       	if (data[i]==EDMI_ETX)
       	{
         	crcResult = edmi_CRC_16(multiCopy[arrayItem].meterRecvBuf,multiCopy[arrayItem].recvFrameTail-3);
         	if (((crcResult>>8)== multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-3])
         	    || ((crcResult&0xff)== multiCopy[arrayItem].meterRecvBuf[multiCopy[arrayItem].recvFrameTail-2])
         	   )
         	{
       	   	//printf("CRC OK\n");

       	   	ret = METER_NORMAL_REPLY;
           
           	//02 45 00 00 00 00 0C A3 76 06 FF FF 06 7F 44 03 
       	   	switch (multiCopy[arrayItem].meterRecvBuf[12])
       	   	{
       	   	 	case 0x06:
       	   	   	//printf("EDMIȷ��\n");
       	   	   	break;
       	   	 
       	   	 	case 0x18:
       	   	   	//printf("EDMI����\n");
       	   	   	ret = METER_ABERRANT_REPLY;
       	   	   	break;
       	   	 
       	   	 	case 'R':
               	ret = meterInput(arrayItem, multiCopy[arrayItem].meterRecvBuf, multiCopy[arrayItem].recvFrameTail);
       	   	 	 	break;
       	   	}
       	 	}
       	 	else
       	 	{
          	ret = RECV_DATA_CHECKSUM_ERROR;   //��������У��ʹ���
       	 	}
       	 
       	 	multiCopy[arrayItem].recvFrameTail   = 0;
       	 	multiCopy[arrayItem].totalLenOfFrame = 2048;
       	}
       
       	if (multiCopy[arrayItem].recvFrameTail>510)
       	{
       	 	multiCopy[arrayItem].recvFrameTail   = 0;
       	}
     	}
   	 	break;
 	 #endif

   #ifdef LANDIS_GRY_ZD_PROTOCOL
    case SIMENS_ZD_METER:
    case SIMENS_ZB_METER:
     	switch (multiCopy[arrayItem].copyItem)
     	{
     	 	case 1:    //ʶ����Ϣ 2f 4c 47 5a 35 5c 32 5a 4d 44 34 30 35 34 34 30 37 2e 42 31 31 0d 0a /LGZ5\2ZMD4054407.B11
     	   	if (data[0]==0x2f && data[5]==0x5c && data[recvLen-2]==0x0d && data[recvLen-1]==0x0a)
     	   	{
     	 	   #ifdef SHOW_DEBUG_INFO
     	 	    data[recvLen] = '\0';
     	 	    printf("��Ʒ���ʶ����Ϣ:%s", data);
     	 	   #endif
     	 	   ret = METER_REPLY_ANALYSE_OK;
     	   	}
     	   	break;
     	 
     	 	case 2:    //������Ϣ
	 	 	   	if (data[0]==0x02 && data[recvLen-5]==0x21 && data[recvLen-2]==0x03)
	 	 	   	{
	 	 	 	 		checkSum = data[1];
	 	 	 	 		for(i=2; i<recvLen-1; i++)
	 	 	 	 		{
	 	 	 	   		checkSum ^= data[i];
	 	 	 	 		}
	 	 	 	 
	 	 	 	 		if (checkSum = data[recvLen-1])
	 	 	 	 		{
	            ret = meterInput(arrayItem, data, recvLen);
	 	 	 	 		}
	 	 	 	 		else
	 	 	 	 		{
	 	 	 	  	 #ifdef SHOW_DEBUG_INFO
	 	 	 	   		printf("IEC1107������ϢBCC Error\n");
	 	 	 	  	 #endif

	 	 	 	   		ret = METER_NORMAL_REPLY;
	 	 	 	 		}
 	 	   		}
 	 	  		break;
 		  }
      break;
   #endif

   #ifdef PROTOCOL_ABB_GROUP
   	case ABB_METER:
   	 	crcResult = CRC_ABB(data, recvLen-2);       	 
   	 	//printf("ABB data len=%d,crcResult=%X,", recvLen, crcResult);
   	 	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
   	 	{
   	   	//printf("crc Ok\n");
   	   	
   	   	ret = METER_NORMAL_REPLY;
   	 
       	switch (multiCopy[arrayItem].copyItem)
       	{
     	   	case 1:    //02 ff 08 13 a1 00 00 02 10 03 51 14 64 5d 23
     	   	case 2:
     	   	case 3:
   	        if (data[0]==0x02 && recvLen==15)
   	        {
   	     	 	 	//Զ����ԿKey
   	     	 	 	rAbbMeterKey = data[9]<<24 | data[10]<<16 | data[11]<<8 | data[12];
   	     	 	 
   	     	 	 	//printf("rAbbMeterKey=%0x\n", rAbbMeterKey);
   	     	 	 	
   	     	 	 	multiCopy[arrayItem].copyItem = 3;
   	     	 	 	abbHandclaspOk = 1;
   	     	 	}
   	        break;
   	     
   	      case 4:    //02 18 00 A2 92 02
   	     	 	if (data[1]==0x18 && data[2]==0x00 && recvLen==6)
   	     	 	{
   	     	 		//printf("����У��OK\n");
   	     	 	}
   	     	 	break;
   	     
   	      case 5:    //Class 0
   	     	 	//02 05 00 a2 a8 00 02 00 05 00 00 00 00 40 0f 0f 04 04 ff 00 01 00 00 01 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 90 7e 4b 
   	     	 	if (data[1]==0x5 && (data[4]&0x7f)==0x28)
   	     	 	{
   	     	 	 	memcpy(abbClass0, &data[5], data[4]&0x7f);
   	     	 	 
   	     	 	 	//printf("������0������\n");
   	     	 	}
   	     		break;
   	     	 
   	      case 6:    //Class-2ǰ�벿��
   	     	 	//02 05 00 a2 40 05 10 06 20 20 20 20 20 20 20 20 20 20 20 20 20 20 00 06 00 00 00 00 00 80 0c 00 00 01 00 0f 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 05 18 23 81 80 40 00 c0 0c 11 05 00 e4 00 00 00 00 bc 9a 
   	     	 	if (data[1]==0x5 && (data[4]&0x7f)==0x40)
   	     	 	{
   	     	 		memcpy(abbClass2, &data[5], data[4]&0x7f);
   	     	 	 
   	     	 	 	//printf("������2��ǰ�벿������\n");
   	     	 	}
   	     	 	break;

   	      case 7:    //Class-2��벿��
   	     	 	//02 81 00 a2 a8 00 00 31 00 06 00 00 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 00 00 00 00 00 09 80 00 00 00 00 00 b9 05 18 d9 91 
   	     		if (data[1]==0x81 && (data[4]&0x7f)==0x28)
   	     	 	{
   	     	 	 	memcpy(&abbClass2[64], &data[5], data[4]&0x7f);
   	     	 	 
   	     	 	 	//printf("������2�ĺ�벿������\n");
   	     	 	}	
   	     	 	break;
   	     
   	      case 9:
   	     	 	if (data[1]==0x05 && (data[4]&0x7f)==0x2a)
   	     	 	{
   	     	 	 	ret = processAbbData(arrayItem, &data[5], data[4]&0x7f, multiCopy[arrayItem].copyItem-9);
   	     	 	}
   	     	 	break;

   	      case 10:
   	      case 11:
   	      case 12:
   	      case 13:
   	      case 14:
   	      case 15:
   	      case 16:
   	      case 17:
   	     		if (data[1]==0x81)
   	     		{
   	     	 	 	ret = processAbbData(arrayItem, &data[5], data[4]&0x7f, multiCopy[arrayItem].copyItem-9);
   	     	 	}
   	     	 	break;
   	   	}
   	 	}
   	 	else
   	 	{
   	 	 	printf("CRC Error\n");
   	 	}
     	break;
   #endif

   #ifdef PROTOCOL_MODUBUS_GROUP
	  case MODBUS_HY:
     	crcResult = modbusCRC16(data, recvLen-2);		 
     	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
     	{
	   		printf("modbus hy crc Ok\n");
 
	   		ret = processHyData(arrayItem, &data[3], recvLen-5);
     	}
     	break;

	  case MODBUS_ASS:
			crcResult = modbusCRC16(data, recvLen-2);		   
		 	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
		 	{
		  	printf("modbus ass crc Ok\n");

		   	ret = processAssData(arrayItem, &data[3], recvLen-5);
		 	}
	    break;
		 
	  case MODBUS_XY_F:
	  case MODBUS_XY_UI:
	  case MODBUS_XY_M:
	    crcResult = modbusCRC16(data, recvLen-2);		 
	    if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
	    {
		   	printf("modbus XY crc Ok\n");
	 
		   	ret = processXyData(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
	    }
	    break;
		 
	  case MODBUS_SWITCH:
		 	crcResult = modbusCRC16(data, recvLen-2);	   
		 	if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
		 	{
		 		printf("modbus switch crc Ok\n");

				ret = processSwitchData(arrayItem, &data[3], recvLen-5);
		 	}
		 	break;
			
		case MODBUS_MW_F:
		case MODBUS_MW_UI:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
				printf("modbus MW crc Ok\n");
		
				ret = processMwData(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;

		case MODBUS_JZ_F:
		case MODBUS_JZ_UI:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
				printf("modbus JZ crc Ok\n");
		
				ret = processJzData(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;
			
		case MODBUS_WE6301_F:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
       #ifdef SHOW_DEBUG_INFO
				printf("modbus WE6301 crc Ok\n");
			 #endif
		
				ret = processWe6301Data(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;
			
		case MODBUS_PMC350_F:
			crcResult = modbusCRC16(data, recvLen-2); 	 
			if ((crcResult>>8)==data[recvLen-2] && (crcResult&0xff)==data[recvLen-1])
			{
       #ifdef SHOW_DEBUG_INFO
				printf("modbus PMC350 crc Ok\n");
       #endif		
				ret = processPmc350Data(arrayItem, &data[3], recvLen-5, multiCopy[arrayItem].protocol);
			}
			break;

   #endif
	 
   #ifdef PROTOCOL_WASION_GROUP
    case PROTOCOL_WASION_GROUP:
      //sendDebugFrame(uart1RecvBuf, recvLen);
      break;
   #endif
     
    default:
      ret = PROTOCOL_NOT_SUPPORT;
      break;
  }
   
  return ret;
}

/***************************************************
��������:meterInput
��������:���ܱ���������
���ú���:
�����ú���:
�������:֡ͷָ�뼰֡����
�������:
����ֵ��void
***************************************************/
INT8S meterInput(INT8U arrayItem, INT8U *pFrameHead,INT16U frameLength)
{
 	struct recvFrameStruct frameAnalyse;   //֡����
 	INT8U                  ctrlState;
 	INT8S                  ret;
 
 #ifdef PROTOCOL_WASION_GROUP
 	INT8U tmpAddr[6];
 	INT8U i;
 #endif   

 #ifdef SHOW_DEBUG_INFO
  INT16U j;
  printf("Rx:");
 	for(j=0;j<frameLength;j++)
 	{
   	printf("%02x ", pFrameHead[j]);
 	}
 	printf("\n");
 #endif
 
 	if (arrayItem>=3)
 	{
   	if (*(pFrameHead+1)==multiCopy[arrayItem].meterAddr[0] && *(pFrameHead+2)==multiCopy[arrayItem].meterAddr[1]
 	     	&& *(pFrameHead+3)==multiCopy[arrayItem].meterAddr[2] && *(pFrameHead+4)==multiCopy[arrayItem].meterAddr[3]
 	     	 && *(pFrameHead+5)==multiCopy[arrayItem].meterAddr[4] && *(pFrameHead+6)==multiCopy[arrayItem].meterAddr[5]
 	     )
   	{
 	   	//printf("�յ��ı��ַ�볭���ı��ַ��ͬ\n");
   	}
   	else
   	{
 	   	//printf("�յ��ı��ַ�볭���ı��ַ��ͬ\n");
 	  
 	   	return RECV_ADDR_ERROR;
   	}
 	}
 
 	//���ݲ�ͬ��Э����ò�ͬ��������֡
 	switch(multiCopy[arrayItem].protocol)
 	{
   #ifdef PROTOCOL_645_1997
    case DLT_645_1997:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_97
    case SINGLE_PHASE_645_1997:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_97
    case PN_WORK_NOWORK_1997:
   #endif
    case DOT_COPY_1997:

     #if PROTOCOL_645_1997 || PROTOCOL_SINGLE_PHASE_97
      if (*(pFrameHead + frameLength - 1) != 0x16)   //�������֡�ڷ���;���ɴ������������ط�
      {
        return RECV_DATA_TAIL_ERROR;  //��������֡β����,����
      }
      else
      {
        if (*(pFrameHead+8) >= 0xC0)
        {
          //�ⲿ�ִ����쳣Ӧ��
          ctrlState = DATA_ERROR_645_1997;

          //copyContinue = TRUE;
          
          return METER_ABERRANT_REPLY;  //��������֡,������쳣Ӧ��(�����Ǳ�˲��ָ�����)
        }
        else
        {
          ret = METER_NORMAL_REPLY;      //��������֡�����ұ������Ӧ��(���֧�ָ�����)
        	
        	if ((*(pFrameHead+8) & 0xf) >= 4)
          {
            //�ⲿ�ִ���д���ݡ�д��ַ��������ʡ�������������Ӧ��
          }
          else
          {
          	if ((*(pFrameHead+8) & FOLLOW_FRAME_645_1997) == FOLLOW_FRAME_645_1997)
            {
               ctrlState = FOLLOW_FRAME_645_1997;
            }
    
          	if ((*(pFrameHead+8) & FOLLOW_FRAME_645_1997) == LAST_FRAME_645_1997)
            {
               ctrlState = LAST_FRAME_645_1997;
            }
    
            //���ݷ���洢
            switch(*(pFrameHead + 11))
            {
            	case 0xB2:
            	case 0xB3:
            	case 0xB6:
            	case 0xC0:
            	case 0xC1:
                  //����ָ��������ݵ�Ԫ��ʶ�Ĳ���������
                  ret = process645Para(arrayItem, pFrameHead+10, frameLength-14);
                  break;
            	  
            	case 0xC3:
            	case 0xC4:
            	  ret = processShiDuanData(arrayItem, pFrameHead+10, frameLength);
            	  break;
            	  	
            	default:
           	      ret = process645Data(arrayItem, pFrameHead+10, frameLength);
           	      break;
            }
          }
        }
      }
        
      if (ctrlState == FOLLOW_FRAME_645_1997)
      {
        //readFollowed(*pDataHead, *(pDataHead+1));
      }
      break;
     #endif //PROTOCOL_645_1997

   #ifdef PROTOCOL_645_2007
    case DLT_645_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_PHASE_07
    case SINGLE_PHASE_645_2007:
    case SINGLE_PHASE_645_2007_TARIFF:
    case SINGLE_PHASE_645_2007_TOTAL:
   #endif
   #ifdef PROTOCOL_PN_WORK_NOWORK_07
    case PN_WORK_NOWORK_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
    case SINGLE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
    case SINGLE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_2007
    case THREE_2007:
   #endif
   #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
    case THREE_LOCAL_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
    case THREE_REMOTE_CHARGE_CTRL_2007:
   #endif
   #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
    case KEY_HOUSEHOLD_2007:
   #endif
    
    case DOT_COPY_2007:
    case HOUR_FREEZE_2007:
      	
     #if PROTOCOL_645_2007 || PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007 || PROTOCOL_KEY_HOUSEHOLD_2007
      if (*(pFrameHead + frameLength - 1) != 0x16) //�������֡�ڷ���;���ɴ������������ط�
      {
        return RECV_DATA_TAIL_ERROR;              //��������֡β����,����
      }
      else
      {
        //֡�����ṹ��ֵ
        frameAnalyse.C     = *(pFrameHead+8);      //������
        frameAnalyse.L     = *(pFrameHead+9);      //�����򳤶�
        frameAnalyse.DI[0] = *(pFrameHead+10);     //DI0
        frameAnalyse.DI[1] = *(pFrameHead+11);     //DI1
        frameAnalyse.DI[2] = *(pFrameHead+12);     //DI2
        frameAnalyse.DI[3] = *(pFrameHead+13);     //DI3
        
        
        if ((frameAnalyse.C&ABERRANT_REPLY)==ABERRANT_REPLY)
        {
          //�ⲿ�ִ����쳣Ӧ��
          return METER_ABERRANT_REPLY;            //��������֡,������쳣Ӧ��(�����Ǳ�˲��ָ�����)
        }
        else
        {
          ret = METER_NORMAL_REPLY;      //��������֡�����ұ������Ӧ��(���֧�ָ�����)
        	
          if (frameAnalyse.C&(NORMAL_REPLY|READ_DATA_645_2007))
          {
        		frameAnalyse.pData   = pFrameHead + 14;
        		frameAnalyse.loadLen = frameAnalyse.L - 4;
        		 
        		ret = process645Data2007(arrayItem,&frameAnalyse);
          }
        }
      }
      break;
     #endif //PROTOCOL_645_2007
    
   #ifdef PROTOCOL_EDMI_GROUP
    case EDMI_METER:
      ret = processEdmiData(arrayItem, &pFrameHead[13], frameLength-13-3);
      break;
   #endif

   #ifdef LANDIS_GRY_ZD_PROTOCOL
    case SIMENS_ZD_METER:
      ret = processLandisGryData(arrayItem, &pFrameHead[1], frameLength-6);
      break;
   #endif

   #ifdef PROTOCOL_WASION_GROUP
    case  PROTOCOL_WASION_GROUP:
     	tmpAddr[0] = *pFrameHead;
     	for (i = 1; i < 5; i++)
     	{
     	  tmpAddr[i] = 0;
     	}
     	measureObject = findMeasurePoint(tmpAddr);
     
     	if (*(pFrameHead+1) == 0xAA || *(pFrameHead+1) == 0xA5)  //��ѯ����Ӧ������
     	{
      	processWasionData(measureObject, pFrameHead);
     	}
     
     	if (*(pFrameHead+1) == 0xB8 || *(pFrameHead+2) == 0x6D) //˲ʱ���ݴ����ѯӦ��
     	{
        processWasionPacket(measureObject, pFrameHead);
     	}
     
     	if (*(pFrameHead+1) == 0xC7 || *(pFrameHead+2) == 0x9A) //ȫ�����ݴ����ѯӦ��
     	{
     	  processWasionTotal(measureObject, pFrameHead);
     	}
     
     	break;
   #endif //PROTOCOL_WASION_GROUP
 }
 
 return ret;
}

INT32U twox[15]= {
	                50000000,
	                25000000,
	                12500000,
	                6250000,
	                3125000,
	                1562500,
	                781250,
	                390625,
	                195312,
	                97656,
	                48828,
	                24414,
	                12207,
	                6103,
	                3051
	               };

/***************************************************
��������:decodeIeee754Float
��������:IEEE754�������ݽ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8U decodeIeee754Float(INT8U *pData, INT8U *retValue)
{
	INT8U  sign=0;          //1-����
	INT8U  exponent=0;      //��
	INT32U mantissa;        //С��
	INT32U rawInt, rawDec;  //���������������С��
	INT8U  i, j;
	INT32U checks;
	
	if (*pData&0x80)
	{
		sign = 1;
	}
	else
	{
		sign = 0;		
	}
	
	exponent = *pData<<1;
	if (*(pData+1)&0x80)
	{
		exponent |= 1;
	}
	
	mantissa = pData[3] | pData[2]<<8 | (pData[1]&0x7f)<<16 | 0x800000;
	
	rawInt = 0;
	rawDec = 0;
	if (exponent>=127)
	{
	  rawInt = mantissa>>(23-(exponent-127));
	}
	
	if (exponent>0)
	{
	  checks = 1<<(23-(exponent-127)-1);
	  //printf("С��λ��=%d,checks=%x\n",23-(exponent-127),checks);
	  
	  for(i=23-(exponent-127),j=0; i>0 && j<15; i++,j++)
	  {
	  	if (mantissa&checks)
	  	{
	  		rawDec += twox[j];
	  	}
	  	checks>>=1;
	  }
	}

	retValue[4] = rawInt>>16;
	retValue[3] = rawInt>>8;
	retValue[2] = rawInt&0xff;
	
	retValue[1] = rawDec/1000000;
	retValue[0] = rawDec%1000000/10000;
  
 #ifdef SHOW_DEBUG_INFO
	printf("decodeIeee754Float:����=%d,��=%d,mantissa=%x,����=%d,С��=%d\n", sign, exponent, mantissa,rawInt,rawDec);
 #endif
	
	return sign;
}


#ifdef PROTOCOL_645_1997

/***************************************************
��������:process645Data
��������:�������ݴ���
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬pDataEnd���ݶν���ָ��,
         measurePoint������
�������:
����ֵ��
***************************************************/
INT8S process645Data(INT8U arrayItem, INT8U *pDataHead,INT8U length)
{
  INT16U offset;
  
  //���ݴ洢ƫ����
  offset = findDataOffset(multiCopy[arrayItem].protocol, pDataHead);
  
 #ifdef SHOW_DEBUG_INFO
  printf("97��Լ������,����%02X %02X ƫ��:%02x\n",*(pDataHead+1), *pDataHead, offset);
 #endif
  
  if (offset == 0x200)
  {
    return RECV_DATA_OFFSET_ERROR;  //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
  }
  
  //������������������ʱ������
  if ((*pDataHead & 0xF) != 0xF)        //�������
  {
    if ((*(pDataHead+1)&0xf0) == 0x90)  //������
    {
      if ((*(pDataHead+1) & 0xf) == 0x0a)
      {
       	multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
      }
      else
      {
        if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
        {
       	   multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;    //�е�ǰ��������
        }
        else
        {
       	   multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY; //�����µ�������
        }
      }
       
    	*(multiCopy[arrayItem].energy+offset)   = *(pDataHead + 2);
    	*(multiCopy[arrayItem].energy+offset+1) = *(pDataHead + 3);
    	*(multiCopy[arrayItem].energy+offset+2) = *(pDataHead + 4);
    	*(multiCopy[arrayItem].energy+offset+3) = *(pDataHead + 5);
    }
    else                      //����
    {
      if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
      {
        multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //�е�ǰ��������
      }
      else
      {
        multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //��������������
      }
       
      if ((*(pDataHead+1)&0xf0) == 0xA0)  //����
      {
    	  *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead + 2);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 3);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 4);
      }
      else                  //����ʱ��
      {
    	  *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead + 2);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 3);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 4);
    	  *(multiCopy[arrayItem].reqAndReqTime+offset+3) = *(pDataHead + 5);
      }
    }
  }
  else   //���ݿ鴦�� ���ݱ�ʶxxxF
  {
    //length -= 15;
    //ly,2011-05-25,������Է���,������ݿ鲻��0xaa�������Ļ�,��û�����һ�����ʵ�����,�ĳ�-14Ӧ�ÿ�����Ӧ����
    length -= 14;

    //68 30 00 01 00 00 00 68 81 17 1F 94 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AA 8B 16

    if ((*(pDataHead+1)&0xf0) == 0x90)           //������
    {
     	if ((*(pDataHead+1) & 0xf) == 0x0a)
     	{
      	multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
     	}
     	else
     	{
       	if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
       	{
     	   	multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;    //�е�ǰ��������
       	}
       	else
       	{
      	 	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY; //�����µ�������
       	}
     	}
     
     	pDataHead += 2;     //ָ��������
     	while (length>=4)
     	{
  	    *(multiCopy[arrayItem].energy+offset)   = *(pDataHead);
  	    *(multiCopy[arrayItem].energy+offset+1) = *(pDataHead + 1);
  	    *(multiCopy[arrayItem].energy+offset+2) = *(pDataHead + 2);
  	    *(multiCopy[arrayItem].energy+offset+3) = *(pDataHead + 3);
     	  
        offset += 4;
        pDataHead += 4;
        
        if (length<4)
        {
        	break;
        }
        else
        {
          length -= 4;
        }
     	}
    }
    else                           //����������ʱ��
    {
     	if ((*(pDataHead+1) & 0xc)>>2 == 0x00)
     	{
     		multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //�е�ǰ��������
     	}
     	else
     	{
      	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //�����µ�������
     	}
     
     	if ((*(pDataHead+1)&0xf0) == 0xA0)  //����
     	{
       	pDataHead += 2;     //ָ��������
       	while (length>=3)
       	{
  	      *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 1);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 2);
  	      
  	      offset += 3;
          pDataHead += 3;
          
          if (length<3)
          {
          	break;
          }
          else
          {
            length -= 3;
          }
       	}
     	}
     	else                  //��������ʱ��
     	{
       	pDataHead += 2;     //ָ��������
       	while (length>=4)
       	{
  	      *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(pDataHead);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(pDataHead + 1);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(pDataHead + 2);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+3) = *(pDataHead + 3);
  	      *(multiCopy[arrayItem].reqAndReqTime+offset+4) = 0x0;
     	    
     	    offset += 5;
          pDataHead += 4;
          
          if (length<4)
          {
          	break;
          }
          else
          {
            length -= 4;
          }
       	}
     	}
    }
  }
  
  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:process645Para
��������:�α������ݴ���
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬pDataEnd���ݶν���ָ��,
         measurePoint
�������:
����ֵ��
***************************************************/
INT8S process645Para(INT8U arrayItem, INT8U *pDataHead, INT8U len)
{
  INT16U    offset, tmpData;
  
  //���ݴ洢ƫ��
  offset = findDataOffset(multiCopy[arrayItem].protocol,pDataHead);
  
 #ifdef SHOW_DEBUG_INFO
  printf("97��Լ%02X %02X,����ƫ��:%02x\n",*(pDataHead+1), *pDataHead, offset);
 #endif
  
  if (offset == 0x200)
  {
  	return RECV_DATA_OFFSET_ERROR;  //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
  }
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������
  
  switch(len)
  {
    case  1:
    	*(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
      break;

    case 2:
     	switch(offset)
     	{
     	  case VOLTAGE_PHASE_A:  //A,B,C���ѹ��2�ֽ�xxx,�洢Ϊxxx.0
     	  case VOLTAGE_PHASE_B:
     	  case VOLTAGE_PHASE_C:
          *(multiCopy[arrayItem].paraVariable+offset)   = (*(pDataHead+2))<<4;
          *(multiCopy[arrayItem].paraVariable+offset+1) = ((*(pDataHead+3))<<4) | ((*(pDataHead+2))>>4);
    	    break;
    	     
    	  case CURRENT_PHASE_A:  //A,B,C�������2�ֽ�xx.xx�洢Ϊ0xx.xx0
    	  case CURRENT_PHASE_B:
    	  case CURRENT_PHASE_C:
          *(multiCopy[arrayItem].paraVariable+offset)   = (*(pDataHead+2))<<4;
          *(multiCopy[arrayItem].paraVariable+offset+1) = ((*(pDataHead+3))<<4) | ((*(pDataHead+2))>>4);
          *(multiCopy[arrayItem].paraVariable+offset+2) = (*(pDataHead+3))>>4;
    	   	break;
    	   	 
    	  case POWER_INSTANT_NO_WORK: //�޹�����Ϊ2bytesxx.xx,�洢Ϊxx.xx00
    	  case POWER_PHASE_A_NO_WORK:
    	  case POWER_PHASE_B_NO_WORK:
    	  case POWER_PHASE_C_NO_WORK:
          *(multiCopy[arrayItem].paraVariable+offset)   = 0;
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+3);      	   	
    	   	break;

    	  case PHASE_DOWN_TIMES:      //�������Ϊ2bytesNNNN,�洢Ϊ00NNNN
    	  case PHASE_A_DOWN_TIMES:
    	  case PHASE_B_DOWN_TIMES:
    	  case PHASE_C_DOWN_TIMES:
    	  case PROGRAM_TIMES:         //��̴���
    	  case UPDATA_REQ_TIME:       //�����������
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);      	   	
          *(multiCopy[arrayItem].paraVariable+offset+2) = 0;
    	   	break;
    	   	 
    	  default:
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          break;
      }
      break;

    case 3:
      switch(offset)
      {
        case BATTERY_WORK_TIME:  //��ع���ʱ��3bytes(NNNNNN),�洢Ϊ4bytes(00NNNNNN)
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
          *(multiCopy[arrayItem].paraVariable+offset+3) = 0x0;
       	  break;
       	   
        default:
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
          break;
      }
      break;

    case 4:
     	switch(offset)
     	{
     	  case LAST_PHASE_DOWN_BEGIN:
     	  case LAST_PHASE_DOWN_END:
     	  case LAST_PHASE_A_DOWN_BEGIN:
     	  case LAST_PHASE_A_DOWN_END:
     	  case LAST_PHASE_B_DOWN_BEGIN:
     	  case LAST_PHASE_B_DOWN_END:
     	  case LAST_PHASE_C_DOWN_BEGIN:
     	  case LAST_PHASE_C_DOWN_END:
     	  case LAST_PROGRAM_TIME:     //���һ�α��ʱ��
     	  case LAST_UPDATA_REQ_TIME:  //���һ����������ʱ��
          *(multiCopy[arrayItem].paraVariable+offset)   = 0x0;
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+3) = *(pDataHead+4);
          *(multiCopy[arrayItem].paraVariable+offset+4) = *(pDataHead+5);
          *(multiCopy[arrayItem].paraVariable+offset+5) = 0x0;
     	 	 	break;
     	 	  	
     	  default:
          *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
          *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
          *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
          *(multiCopy[arrayItem].paraVariable+offset+3) = *(pDataHead+5);
          break;
      }
      break;
       
    case 6:
       *(multiCopy[arrayItem].paraVariable+offset)   = *(pDataHead+2);
       *(multiCopy[arrayItem].paraVariable+offset+1) = *(pDataHead+3);
       *(multiCopy[arrayItem].paraVariable+offset+2) = *(pDataHead+4);
       *(multiCopy[arrayItem].paraVariable+offset+3) = *(pDataHead+5);
       *(multiCopy[arrayItem].paraVariable+offset+4) = *(pDataHead+6);
       *(multiCopy[arrayItem].paraVariable+offset+5) = *(pDataHead+7);
       break;
  }

  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processShiDuanData
��������:ʱ�βα������ݴ���
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬pDataEnd���ݶν���ָ��,
�������:
����ֵ��
***************************************************/
INT8S processShiDuanData(INT8U arrayItem, INT8U *pDataHead,INT8U length)
{   
  INT16U offset;
  
  multiCopy[arrayItem].hasData |= HAS_SHIDUAN; //��ʱ������
  
  if (*(pDataHead+1)==0xc3)
  {
    offset = findDataOffset(multiCopy[arrayItem].protocol,pDataHead);
    
   #ifdef SHOW_DEBUG_INFO
    printf("97��Լʱ��%02X %02X ƫ��:%02x\n",*(pDataHead+1), *pDataHead, offset);
   #endif
  	
    if (offset == 0x200)
    {
  	 	return RECV_DATA_OFFSET_ERROR;  //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
    }
    
    if ((*pDataHead>>4)==1)  //��ʱ����P��
    {
  	 	*(multiCopy[arrayItem].shiDuan+offset) = *(pDataHead+2);
    }
    else                     //ʱ������ʱ��
    {
      if ((*pDataHead & 0xF) != 0xF)  //�������
      {
        offset += 3*((*pDataHead&0xf)-1);
       	
       	*(multiCopy[arrayItem].shiDuan+offset) = *(pDataHead + 2);
       	*(multiCopy[arrayItem].shiDuan+offset+1) = *(pDataHead + 3);
       	*(multiCopy[arrayItem].shiDuan+offset+2) = *(pDataHead + 4);
      }
      else   //���ݿ鴦�� ���ݱ�ʶxxxF
      {
        length -= 12;
     
        if ((*pDataHead>>4&0xf)>2)
        {
         	offset += 30*((*pDataHead>>4&0xf)-3);
        }
         
        pDataHead += 2;     //ָ��������
        while (length>=3)
        {
          *(multiCopy[arrayItem].shiDuan+offset)   = *pDataHead;
          *(multiCopy[arrayItem].shiDuan+offset+1) = *(pDataHead + 1);
          *(multiCopy[arrayItem].shiDuan+offset+2) = *(pDataHead + 2);
            
          offset += 3;              	  
          pDataHead += 3;
            
          if (*pDataHead==0xaa && *(pDataHead+2)==0x16)
          {
           	break;
          }
          if (length<3)
          {
            break;
          }
          else
          {
            length -= 3;
          }
        }
      }
  	}
  }
  else
  {
  	if (*(pDataHead+1)==0xc4)
  	{
		  if ((*pDataHead&0xf)==0xe)
		  {
		  	//shiDuanData[ZHOUXIURI_SHIDUAN] = *(pDataHead+2);
		  	
		  	*(multiCopy[arrayItem].shiDuan+ZHOUXIURI_SHIDUAN) = *(pDataHead+2);
		  }
		  else
		  {
		  	offset = JIA_RI_SHIDUAN+3*((*pDataHead&0xf)-1);
		  	
       	//shiDuanData[offset++] = *(pDataHead + 2);
       	//shiDuanData[offset++] = *(pDataHead + 3);
       	//shiDuanData[offset] = *(pDataHead + 4);    		  	 

       	*(multiCopy[arrayItem].shiDuan+offset)   = *(pDataHead + 2);
       	*(multiCopy[arrayItem].shiDuan+offset+1) = *(pDataHead + 3);
       	*(multiCopy[arrayItem].shiDuan+offset+2) = *(pDataHead + 4);    		  	 
		  }
  	}
  }

  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

#endif

#ifdef PROTOCOL_645_2007

/***************************************************
��������:process645Data2007
��������:07��Լ���ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S process645Data2007(INT8U arrayItem,struct recvFrameStruct *frame)
{
  INT16U offset,offsetx;
  INT8U  i;
  
  //���ݴ洢ƫ����
  offset = findDataOffset(multiCopy[arrayItem].protocol, frame->DI);
  
 #ifdef SHOW_DEBUG_INFO
  printf("2007��Լ%02X %02X %02X %02Xƫ��:%02x\n", frame->DI[3],frame->DI[2],frame->DI[1],frame->DI[0], offset);
 #endif
  
  if (offset==0x200)
  {
  	return RECV_DATA_OFFSET_ERROR;  //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
  }
  
  switch(frame->DI[3])
  {
    case ENERGY_2007:   //������
  	  if (frame->DI[0]==0x0)
  	  {
       	multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;     //�е�ǰ��������
      }
      else
      {
       	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY;  //����һ�����յ�������
      }
       
  	  if (frame->DI[1]==0xff) //���ݿ�
  	  {
        while (frame->loadLen>=4)
        {
       	  *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	  *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	  *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	  *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
 
          offset += 4;
          frame->pData += 4;
             
          if (frame->loadLen<4)
          {
            break;
          }
          else
          {
            frame->loadLen -= 4;
          }
        }
  	  }
  	  else             //������
  	  {
       	if (frame->DI[2]==0x90)
       	{
       	  *(multiCopy[arrayItem].paraVariable+offset)   = *(frame->pData);
       	  *(multiCopy[arrayItem].paraVariable+offset+1) = *(frame->pData + 1);
       	  *(multiCopy[arrayItem].paraVariable+offset+2) = *(frame->pData + 2);
       	  *(multiCopy[arrayItem].paraVariable+offset+3) = *(frame->pData + 3);         	 	 
       	}
       	else
       	{
       	  *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	  *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	  *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	  *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
       	}
  	  }
  	  break;
  	 	 
    case REQ_AND_REQ_TIME_2007:  //����������ʱ��
  	  if (frame->DI[0]==0x0)
  	  {
        multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //�е�ǰ��������
      }
      else
      {
       	multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //��������������
      }

  	  if (frame->DI[1]==0xff) //���ݿ�
  	  {
        offsetx = offset+27;
        while (frame->loadLen>=8)
        {
     	    //����3bytes
     	    *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(frame->pData);
     	    *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(frame->pData + 1);
     	    *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(frame->pData + 2);
     	    
     	    //��������ʱ��5bytes
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+0) = *(frame->pData + 3);  //��
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+1) = *(frame->pData + 4);  //ʱ
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+2) = *(frame->pData + 5);  //��
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+3) = *(frame->pData + 6);  //��
     	    *(multiCopy[arrayItem].reqAndReqTime+offsetx+4) = *(frame->pData + 7);  //��

          offset  += 3;
          offsetx += 5;
          frame->pData += 8;
           
          if (frame->loadLen<8)
          {
           	break;
          }
          else
          {
            frame->loadLen -= 8;
          }
        }
  	  }
  	  else             //������
  	  {
  	  }    	 	 
  	  break;
  	 
    case VARIABLE_2007:   //����
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�вα�������
  	 	 
  	  if (frame->DI[1]==0xff)
  	  {
        switch(frame->DI[2])
        {
         	case 0x01:  //��ѹ���ݿ�(��2���ֽ�xxx.x)
         	  for(i=0;i<6 && i<frame->loadLen;i++)
         	  {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
         	  }
         	  break;

         	case 0x02:  //�������ݿ�(��3���ֽ�xxx.xxx)
         	  for(i=0;i<9 && i<frame->loadLen;i++)
         	  {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
         	  }
         	  break;

       	  case 0x03:  //˲ʱ�й��������ݿ�(��3���ֽ�xx.xxxx)
       	  case 0x04:  //˲ʱ�޹��������ݿ�(��3���ֽ�xx.xxxx)
       	  case 0x05:  //˲ʱ���ڹ������ݿ�(��3���ֽ�xx.xxxx)
       	  	for(i=0;i<12 && i<frame->loadLen;i++)
       	  	{
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	  	}
       	  	break;

       	  case 0x06:  //�����������ݿ�(��2�ֽ�x.xxx)
       	  	for(i=0; i<8 && i<frame->loadLen; i++)
       	  	{
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	  	}
       	  	break;

       	  case 0x07:  //������ݿ�(��2�ֽ�x.xxx)
       	  	for(i=0;i<6 && i<frame->loadLen; i++)
       	  	{
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);   //��ѹ���
              *(multiCopy[arrayItem].paraVariable+offset+6+i)   = *(frame->pData+i);   //�������
       	  	}
       	  	break;
       	  	
       	  default:
	 	        return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����           	  	
       	  	break;
        }           	  	
  	  }
  	  else
  	  {
       	if (frame->DI[2]==0x80)
       	{
          if (frame->DI[0]==0x01)  //���ߵ���(xxx.xxx!=97 2bytes xx.xx)
          {
       	    for(i=0;i<3 && i<frame->loadLen;i++)
       	    {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	    }
          }
          
       	  if (frame->DI[0]==0x0a)  //�ڲ���ع���ʱ��(4bytes)
       	  {
       	    for(i=0;i<4;i++)
       	    {
              *(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
       	    }
       	  }
       	}
  	  }
  	  break;
  	 	 
    case EVENT_RECORD_2007:
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�вα�������
       
	    switch(frame->DI[2])
	    {
	 	    case 0x04:    //A,B,C�����¼,��������Ͷ���ʱ��,etc...
 	      	switch(frame->DI[0])
 	     	 	{
 	 	      	case 0x00:  //A,B,C������������ۼ�ʱ��(������ʱ���3bytes,NNNNNN,3*2*3)
 	 	        	for(i=0;i<18;i++)
 	 	        	{
             	 	*(multiCopy[arrayItem].paraVariable+offset+i)   = *(frame->pData+i);
 	 	        	}
 	 	        	break;
 	 	    
 	 	   			case 0x01:  //A,B,C������¼(��ʼʱ��ͽ���ʱ���6bytes,ssmmhhDDMMYY)
			       	for(i=0;i<12;i++)
			       	{
			          *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	}
			       	break;
     	  
     	      default:
	            return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����
 	     		}
 	     		break;
      	 	 
			  case 0x30:   //���,�������,��������,�¼�����,Уʱ,ʱ�α��޸ļ�¼
				 	switch(frame->DI[1])
				 	{
				 	  case 0x0:  //��̼�¼
				 	 	  switch(frame->DI[0])
				 	 	  {
				 	 	    case 0x00:  //��̴���
			 	          for(i=0;i<3;i++)
			 	          {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			 	          }
			 	          break;
			     	      
			     	    case 0x01:  //��һ�α�̼�¼
			     	      //��������һ�α�̷���ʱ��6bytes(ssmmhhDDMMYY) 
			     	      for(i=0;i<6;i++)
			     	      {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	      }
			     	      break;
			     	      
			     	    default:
			          	return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0x1:  //��������¼
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //��������ܴ���
			       	    for(i=0;i<3;i++)
			       	    {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	    }
			       	    break;
			       	      
			       	  case 0x01:  //��һ�ε�������¼
			       	    //��������һ�ε�����㷢��ʱ��6bytes(ssmmhhDDMMYY) 
			       	    for(i=0;i<6;i++)
			       	    {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	    }
			       	    break;
			       	      
			       	  default:
				         		return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	 	}
			    	 	break;
			    	 	 	   
				 	    case 0x2:  //���������¼
				 	     	switch(frame->DI[0])
				 	     	{
				 	       	case 0x00:  //���������ܴ���
			       	    for(i=0;i<3;i++)
			       	    {
			              *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			       	    }
			       	    break;
			           	      
			     	   	case 0x01:  //��һ�����������¼
			     	     	//��������һ���������㷢��ʱ��6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0x3:  //�¼������¼
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //�¼������ܴ���
			     	     	for(i=0;i<3;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //��һ���¼������¼
			     	     	//��������һ���¼����㷢��ʱ��6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	  }
			    	 	break;

			    	case 0x4:  //Уʱ��¼
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //Уʱ�ܴ���
			     	     	for(i=0;i<3;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //��һ��Уʱ��¼
			     	     	//��������һ��Уʱ����ʱ��6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i+4);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0x5:  //ʱ�α�̼�¼
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //ʱ�α���ܴ���
			     	      for(i=0;i<3;i++)
			     	      {
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //��һ��ʱ�α�̼�¼
			     	     	//��������һ��ʱ�α�̷���ʱ��6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	 	}
			    	 	break;
			    	 	 	   
			    	case 0xd:  //���β�Ǵ򿪼�¼
			    	 	switch(frame->DI[0])
			    	 	{
			    	 	  case 0x00:  //������ܴ���
			     	      for(i=0;i<3;i++)
			     	     	{
			            	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	case 0x01:  //��һ�ο���Ƿ���ʱ��
			     	     	//��������һ�ο���Ƿ���ʱ��6bytes(ssmmhhDDMMYY) 
			     	     	for(i=0;i<6;i++)
			     	     	{
			             	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	     	}
			     	     	break;
			     	      
			     	   	default:
			         		return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����               	    
			    	 	}
			    	 	break;
			    }
			    break;
			    	 	 
			  case 0x32:
				 	//��һ�ι�����ۼƹ�����
				 	if (frame->DI[1]==0x06 && frame->DI[0]==0x01)
				 	{
			      for(i=0;i<4;i++)
			      {
			        *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			      }
				 	}
				 	break;
				 	 
			  case 0x33:
				 	//��һ�ι�����ۼƹ������
				 	if (frame->DI[1]==0x02 && frame->DI[0]==0x01)
				 	{
			     	for(i=0;i<2;i++)
			     	{
			         *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	}
				 	}
				 	 
				 	//��һ�ι�����ۼƹ�����
				 	if (frame->DI[1]==0x06 && frame->DI[0]==0x01)
				 	{
			     	for(i=0;i<4;i++)
			     	{
			         *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
			     	}
				 	}
				 	break;
			}
			break;
  	 	 
    case PARA_VARIABLE_2007:
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�вα�������

  	 	switch(frame->DI[2])
  	  {
  	   	case 0x0:
  	    	switch(frame->DI[1])
  	     	{
  	   	  	case 0x01:
  	         	if (frame->DI[0]==0x01)  //���ڼ�����
  	 	     		{
         	      for(i=0;i<4;i++)
         	      {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
         	      }
  	 	     		}
	  	 	     	if (frame->DI[0]==0x02)  //ʱ��
	  	 	     	{
	           	  for(i=0;i<3;i++)
	           	  {
	                *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
	           	  }
	  	 	     	}
	  	 	     	break;
      	 	     
      	   	case 0x02:
      	 	 		//���˹��������������������ֽ�ֻ����һ���ֽ���,�����Ķ�����Լһ���ֽڴ洢
              multiCopy[arrayItem].hasData |= HAS_SHIDUAN;       //��ʱ������
              *(multiCopy[arrayItem].shiDuan+offset) = *frame->pData;
      	 	 		break;
      	 	   
      	   	case 0x04:        	 	 
      	 	 		if (frame->DI[0]==0x09 || frame->DI[0]==0x0a)  //�����(�й����޹�)
      	 	 		{
             	 	for(i=0;i<3;i++)
             	  {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
             	  }
      	 	 		}
		      	 	if (frame->DI[0]==0x02)  //�����
		      	 	{
           	   	for(i=0;i<6;i++)
           	   	{
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
           	   	}
		      	 	}
		      	 	break;
      	 	     
      	   	case 0x05:        	 	 
      	 	 		if (frame->DI[0]==0xff)    //�������״̬��
      	 	 		{
               	//������7��״̬��(7*2bytes),��645-1997ֻ��һ���ֽڵ�״̬��
             	 	for(i=0;i<14;i++)
             	  {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
             	  }
      	 	 		}
      	 	 		break;
      	 	     
      	   	case 0x0b:  //ÿ�½�����(1,2,3������,��2���ֽ�)
              //������3��������,��645-1997ֻ��һ��������
              *(multiCopy[arrayItem].paraVariable+offset) = *frame->pData;
              *(multiCopy[arrayItem].paraVariable+offset+1) = *(frame->pData+1);
      	 	 		break;
      	 	     
      	   	case 0x0f:
      	 	 		if (frame->DI[0]==0x04 || frame->DI[0]==0x01)    //͸֧������ֵ /��������1��ֵ
      	 	 		{
             	  for(i=0;i<4;i++)
             	  {
                  *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
             	  }
      	 	 		}
      	 	  	break;
      	  }
      	 	break;
      	 
        case 0x01:   //��һ��ʱ�������ݼ�ʱ�α�����
          multiCopy[arrayItem].hasData |= HAS_SHIDUAN;       //��ʱ������
      	 	 
      	 	if (frame->DI[0]==0x00)  //��һ��ʱ��������
      	 	{
            while (frame->loadLen>=3)
            {
           	  *(multiCopy[arrayItem].shiDuan+offset)   = *(frame->pData);
           	  *(multiCopy[arrayItem].shiDuan+offset+1) = *(frame->pData + 1);
           	  *(multiCopy[arrayItem].shiDuan+offset+2) = *(frame->pData + 2);
     
              offset += 3;
              frame->pData += 3;
                 
              if (frame->loadLen<3)
              {
                break;
              }
              else
              {
                frame->loadLen -= 3;
              }
            }
      	 	}
      	 	else   //��ʱ�α�����
      	 	{
         	  offset += 30*frame->DI[0];
      	 	 	 
      	   	for(i=0;i<10&&frame->loadLen>=3;i++)
      	   	{
           	 	*(multiCopy[arrayItem].shiDuan+offset)   = *(frame->pData);
           	 	*(multiCopy[arrayItem].shiDuan+offset+1) = *(frame->pData + 1);
           	 	*(multiCopy[arrayItem].shiDuan+offset+2) = *(frame->pData + 2);
     
              offset += 3;
              frame->pData += 3;
                
              if (frame->loadLen<3)
              {
                break;
              }
              else
              {
                frame->loadLen -= 3;
              }        	 	 	 	  
      	   	}
      	 	}
      	 	break;
      }
  	  break;
  	 	 
    case FREEZE_DATA_2007:   //��������
  	 	if (multiCopy[arrayItem].protocol==SINGLE_PHASE_645_2007
  	 	  	|| multiCopy[arrayItem].protocol==SINGLE_LOCAL_CHARGE_CTRL_2007
  	 	  	 || multiCopy[arrayItem].protocol==SINGLE_REMOTE_CHARGE_CTRL_2007
  	 		 )
  	 	{
  	   	if (frame->DI[2]==0x6 && (frame->DI[1]==0x00 || frame->DI[1]==0x01 || frame->DI[1]==0x02))
  	   	{
       		multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
       	 
          if (frame->DI[1]==0x00)   //�ն���ʱ��
          {
         	  *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
         	  *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
         	  *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
         	  *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
         	  *(multiCopy[arrayItem].energy+offset+4) = *(frame->pData + 4);
          }
          else
          {
            while (frame->loadLen>=4)
            {
         	    *(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
         	    *(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
         	    *(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
         	    *(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
   
              offset += 4;
              frame->pData += 4;
               
              if (frame->loadLen<4)
              {
                break;
              }
              else
              {
                frame->loadLen -= 4;
              }
            }
          }
        }
  	  }
  	  else
  	  {
  	   	//if (frame->DI[2]==0x6 && frame->DI[1]>=0x00 && frame->DI[1]<0x09 && frame->DI[0]==0x01)
  	   	if (frame->DI[2]==0x6 && frame->DI[1]<0x09 && frame->DI[0]==0x01)  //ly,2012-01-10,modify
  	   	{
       	 	multiCopy[arrayItem].hasData |= HAS_LAST_DAY_ENERGY;     //����һ���ն����������
       	 
          if (frame->DI[1]==0x00)        //�ն���ʱ��
          {
       	   	*(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	   	*(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	   	*(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	   	*(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
       	   	*(multiCopy[arrayItem].energy+offset+4) = *(frame->pData + 4);
         	}
         	else
         	{
           	while (frame->loadLen>=4)
           	{
       	     	*(multiCopy[arrayItem].energy+offset)   = *(frame->pData);
       	     	*(multiCopy[arrayItem].energy+offset+1) = *(frame->pData + 1);
       	     	*(multiCopy[arrayItem].energy+offset+2) = *(frame->pData + 2);
       	     	*(multiCopy[arrayItem].energy+offset+3) = *(frame->pData + 3);
 
             	offset += 4;
             	frame->pData += 4;
             
             	if (frame->loadLen<4)
             	{
               	break;
             	}
             	else
             	{
                frame->loadLen -= 4;
             	}
           	}
         	}
       	}
        else
        {
          if (frame->DI[2]==0x6 && (frame->DI[1]==0x09 || frame->DI[1]==0x0a) && frame->DI[0]==0x01)
          {
            multiCopy[arrayItem].hasData |= HAS_LAST_DAY_REQ;     //����һ���ն�����������

            offsetx = offset+27;
            while (frame->loadLen>=8)
            {
          	  //����3bytes
          	  *(multiCopy[arrayItem].reqAndReqTime+offset)   = *(frame->pData);
          	  *(multiCopy[arrayItem].reqAndReqTime+offset+1) = *(frame->pData + 1);
          	  *(multiCopy[arrayItem].reqAndReqTime+offset+2) = *(frame->pData + 2);
          	    
          	  //��������ʱ��5bytes
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+0) = *(frame->pData + 3);  //��
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+1) = *(frame->pData + 4);  //ʱ
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+2) = *(frame->pData + 5);  //��
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+3) = *(frame->pData + 6);  //��
          	  *(multiCopy[arrayItem].reqAndReqTime+offsetx+4) = *(frame->pData + 7);  //��
    
              offset  += 3;
              offsetx += 5;
              frame->pData += 8;
                
              if (frame->loadLen<8)
              {
                break;
              }
              else
              {
                frame->loadLen -= 8;
              }
            }
     	 	 	}
     	 	 	else
     	 	 	{
            if (frame->DI[2]==0x4 && frame->DI[1]==0xff)
            {
              multiCopy[arrayItem].hasData |= HAS_HOUR_FREEZE_ENERGY;     //�����㶳���������
         	     
       	     	//���㶳��ʱ��
       	     	for(i=0;i<5;i++)
       	     	{
       	     	  *(multiCopy[arrayItem].energy+128+i)   = *(frame->pData+i);
       	     	}
       	     
       	     	frame->pData+=6;   //������һ��0xaaΪ�ָ���
       	     
       	     	//���㶳�������й�����ʾֵ
       	     	for(i=0;i<4;i++)
       	     	{
       	       	*(multiCopy[arrayItem].energy+i)   = *(frame->pData+i);
       	     	}

       	     	frame->pData+=5;   //������һ��0xaaΪ�ָ���
       	     
       	    	//���㶳�ᷴ���й�����ʾֵ
       	     	for(i=0;i<4;i++)
       	     	{
       	       	*(multiCopy[arrayItem].energy+4+i)   = *(frame->pData+i);
       	     	}
       	     
       	     	//��������㶳������
		 	       	multiCopy[arrayItem].save(multiCopy[arrayItem].pn, multiCopy[arrayItem].port, multiCopy[arrayItem].copyTime,  \
		 	                             multiCopy[arrayItem].energy, HOUR_FREEZE, \
		 	                               0x0, LENGTH_OF_HOUR_FREEZE_RECORD);
            }
     	 		}
        }
  	 	}
  	 	break;
  	 	 
    case EXT_EVENT_RECORD_13:    //07�����ļ�����ͳ������
      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�вα�������
  	 	if (frame->DI[1]==00)    //�������������ۼ�ʱ��
  	 	{
       	for(i=0;i<3;i++)
       	{
        	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
       	}
	 		}
	 		else                //���һ�ζ�����ʼʱ�估����ʱ��
	 		{
      	for(i=0;i<6;i++)
       	{
         	*(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
       	}
	 		}
	 		break;
  	 	 
    case 0x1d:
    case 0x1e:
  		if (frame->DI[2]==0x00 && frame->DI[1]==0x01 && frame->DI[0]==0x1)  //��һ����/��բ����ʱ��
  	 	{
        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�вα�������
         
        for(i=0;i<6;i++)
        {
          *(multiCopy[arrayItem].paraVariable+offset+i) = *(frame->pData+i);
        }
  	 	}
  	 	break;
  	 	     	 	
    default:
  	  return METER_NORMAL_REPLY; //��������֡�����ұ������Ӧ��(���֧�ָ�����),�������Ǳ��ն˷�������������ص�����
  	  break;
  }
  
  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

#endif   //PROTOCOL_645_2007

#ifdef PROTOCOL_EDMI_GROUP




/***************************************************
��������:processEdmiData
��������:EDMI���ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S processEdmiData(INT8U arrayItem, INT8U *frame, INT16U loadLen)
{
  INT16U offset;
  INT8U  i;
  INT8U  retVal[5];
  INT32U dataSign, rawInt, rawDec;
  
  //���ݴ洢ƫ����
  offset = findDataOffset(multiCopy[arrayItem].protocol, frame);
  
  #ifdef SHOW_DEBUG_INFO
    printf("EDMI��Լ(LoadLen=%d):%02X %02Xƫ��:%02x\n", loadLen, frame[0], frame[1], offset);
  #endif
  
  if (offset==0x200)
  {
    return RECV_DATA_OFFSET_ERROR;  //�������ݱ���ʱƫ�Ƶ�ַ����,δ����
  }
  
  switch(frame[0])
  {
    case 0x01:    //�����й�����
    case 0x00:    //�����й�����
    case 0x03:    //�����޹�����
    case 0x02:    //�����޹�����
      decodeIeee754Float(&frame[2], retVal);
      //printf("ʾֵ:����=%d,С��=%02d%02d\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);
     	
     	if (frame[1]<0x50)
     	{
     	  multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_ENERGY;  //�����µ�������
     	}
     	else
     	{
     	  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;     //�е�ǰ��������
     	}
      
      rawInt = retVal[4]<<16 | retVal[3]<<8 | retVal[2];
      rawDec = (rawInt%1000)*100/1000;
      rawInt /=1000;
      rawInt = hexToBcd(rawInt);
      rawDec = hexToBcd(rawDec);
      
     	*(multiCopy[arrayItem].energy+offset)   = rawDec;
     	*(multiCopy[arrayItem].energy+offset+1) = rawInt&0xff;
     	*(multiCopy[arrayItem].energy+offset+2) = rawInt>>8 & 0xff;
     	*(multiCopy[arrayItem].energy+offset+3) = rawInt>>16 & 0xff;
      break;
     
    case 0xe0:    //��ѹ������������
      dataSign = decodeIeee754Float(&frame[2], retVal);
      //printf("��ѹ/����/����:����=%d,С��=%02d%02d\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);
     	
     	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;     //�вα�������
     	
     	switch(frame[1])
     	{
     	  case 0x00:    //A���ѹ
     	  case 0x01:    //B���ѹ
     	  case 0x02:    //C���ѹ
           rawInt = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
           rawDec = hexToBcd(retVal[1]);
           *(multiCopy[arrayItem].paraVariable+offset+0) = (rawInt<<4&0xff) | (rawDec>>4&0xf);
           *(multiCopy[arrayItem].paraVariable+offset+1) = rawInt>>4&0xff;
     		 	 break;

     		 case 0x10:    //A�����
     		 case 0x11:    //B�����
     		 case 0x12:    //C�����
           rawInt = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
           rawDec = hexToBcd(retVal[1])<<8 | hexToBcd(retVal[0]);
           *(multiCopy[arrayItem].paraVariable+offset+0) = rawDec>>4&0xff;
           *(multiCopy[arrayItem].paraVariable+offset+1) = (rawInt<<4&0xf0) | rawDec>>12&0xf;
           *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt>>4&0xff;
           if (dataSign==1)
           {
           	 *(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80; 
           }
     		 	 break;
     		 	 
     	  case 0x33:    //�й��ܹ���
     	  case 0x30:    //�й�A���ܹ���
     	  case 0x31:    //�й�B���ܹ���
     	  case 0x32:    //�й�C���ܹ���
     	  case 0x43:    //�޹��ܹ���
     	  case 0x40:    //�޹�A���ܹ���
     	  case 0x41:    //�޹�B���ܹ���
     	  case 0x42:    //�޹�C���ܹ���
           rawInt = retVal[4]<<16 | retVal[3]<<8 | retVal[2];
           rawDec = hexToBcd(rawInt%1000);
           rawInt = hexToBcd(rawInt/1000);
                   
           *(multiCopy[arrayItem].paraVariable+offset+0) = (rawDec<<4&0xf0) | (hexToBcd(retVal[1])>>4&0x0f);
           *(multiCopy[arrayItem].paraVariable+offset+1) = rawDec>>4;
           *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt&0xff;
           if (dataSign==1)
           {
           	 *(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80; 
           }
     		 	 break;
     		 	 
     		 case 0x26:    //�ܹ�������
           rawInt = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
           rawDec = hexToBcd(retVal[1])<<8 | hexToBcd(retVal[0]);
           *(multiCopy[arrayItem].paraVariable+offset+0) = rawDec>>4&0xff;
           *(multiCopy[arrayItem].paraVariable+offset+1) = (rawInt<<4&0xf0) | rawDec>>12&0xf;
           if (dataSign==1)
           {
           	 *(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80;
           }
     		 break;

     	}
    	break;
    
    case 0xF0:    //���ʱ��/����
     	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;     //�вα�������
      
      //F0 3d 06 07 0c 09 29 1e
     	*(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(frame[2]);  //��
     	*(multiCopy[arrayItem].paraVariable+offset+2) = hexToBcd(frame[3]);  //��
     	*(multiCopy[arrayItem].paraVariable+offset+3) = hexToBcd(frame[4]);  //��
     	*(multiCopy[arrayItem].paraVariable+offset+4) = hexToBcd(frame[6]);  //��
     	*(multiCopy[arrayItem].paraVariable+offset+5) = hexToBcd(frame[6]);  //��
     	*(multiCopy[arrayItem].paraVariable+offset+6) = hexToBcd(frame[5]);  //ʱ
    	break;
    
    case 0x11:    //��ǰ�����й��������
    case 0x10:    //��ǰ�����й��������,���ص�������W,���ʱ��Ҫ���kW
     	if (frame[1]&0x20)
     	{
     	  multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //��������������
     	}
     	else
     	{
     	  multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //�е�ǰ��������
     	}

      dataSign = decodeIeee754Float(&frame[2], retVal);
      //printf("����:����=%d,С��=%02d%02d\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);

      rawInt = retVal[4]<<16 | retVal[3]<<8 | retVal[2];
      rawDec = hexToBcd(rawInt%1000);
      rawInt = hexToBcd(rawInt/1000);
              
      *(multiCopy[arrayItem].reqAndReqTime+offset+0) = (rawDec<<4&0xf0) | (hexToBcd(retVal[1])>>4&0x0f);
      *(multiCopy[arrayItem].reqAndReqTime+offset+1) = rawDec>>4;
      *(multiCopy[arrayItem].reqAndReqTime+offset+2) = rawInt&0xff;
      if (dataSign==1)
      {
      	*(multiCopy[arrayItem].paraVariable+offset+2) |= 0x80; 
      }
    	break;
    
    case 0x81:    //��ǰ�����й������������ʱ��
    case 0x80:    //��ǰ�����й������������ʱ��
    	//80 09 01 01 60 00 00 00
    	// 0  1  2  3  4  5  6  7
    	//      ��  �� �� ʱ �� ��
     	if (frame[1]&0x20)
     	{
     	  multiCopy[arrayItem].hasData |= HAS_LAST_MONTH_REQ;     //��������������
     	}
     	else
     	{
     	  multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //�е�ǰ��������
     	}
     	
     	*(multiCopy[arrayItem].reqAndReqTime+offset+0) = hexToBcd(frame[6]);  //��
     	*(multiCopy[arrayItem].reqAndReqTime+offset+1) = hexToBcd(frame[5]);  //ʱ
     	*(multiCopy[arrayItem].reqAndReqTime+offset+2) = hexToBcd(frame[2]);  //��
     	*(multiCopy[arrayItem].reqAndReqTime+offset+3) = hexToBcd(frame[3]);  //��
     	*(multiCopy[arrayItem].reqAndReqTime+offset+4) = hexToBcd(frame[4]);  //��
     	break;
  }
  
  return METER_NORMAL_REPLY;
}

#endif

#ifdef LANDIS_GRY_ZD_PROTOCOL

/***************************************************
��������:findLeftBracket
��������:���ַ����в���������
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8U findLeftBracket(char dataChr[])
{
  INT8U j=0xff;
  
  for(j=0; j<strlen(dataChr); j++)
  {
    if (dataChr[j]=='(')
    {
      j++;
      break;
    }
  }
  
  return j;
}

/***************************************************
��������:processLandisGryData
��������:LandisGry/Simens���ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S processLandisGryData(INT8U arrayItem, INT8U *frame, INT16U loadLen)
{
  INT16U offset, frameOffset;
  INT16U i;
  INT8U  retVal[5];
  INT32U dataSign, rawInt, rawDec;
  char   tmpLandis[100];
  INT16U itemInChr=0x201;        //�����������ַ������е����
  INT8U  bracketItemInData=0;    //���������ַ����е�λ��
  
  while(loadLen>0)
  {
    //46 2E 46 28 30 30 30 30 30 30 30 30 29 0D 0A 30 2E 39 2E 31 28 30 38 3A 31 37 3A 31 35 29 0D 0A
    frameOffset = 0;
    for(i=0; i<loadLen; i++)
    {
    	if (frame[i]==0x0d && frame[i+1]==0x0a)
    	{
    	  frameOffset+=2;
    	  
    	  memcpy(tmpLandis, frame, frameOffset);
    	  tmpLandis[frameOffset] = '\0';
    	  
    	  #ifdef SHOW_DEBUG_INFO
    	   printf("��������=%s", tmpLandis);
    	  #endif
        
        //���ݴ洢item
        itemInChr = findDataOffset(multiCopy[arrayItem].protocol,(INT8U *)tmpLandis);
        
        if (itemInChr<=TOTAL_DATA_ITEM_ZD)
        {
          offset = landisOffset[itemInChr];
          bracketItemInData = findLeftBracket(tmpLandis);

          #ifdef SHOW_DEBUG_INFO
            printf("LandisGry��Լ:itemInChr=%d, brackeItemInData=%d, ƫ��:%02x\n", itemInChr, bracketItemInData, landisOffset[itemInChr]);
          #endif
          
          //����ʾֵ
          if (itemInChr<40)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
            	//1.8.0(000000.00*kWh)
            	rawInt = (tmpLandis[bracketItemInData]-0x30)*100000
            	       + (tmpLandis[bracketItemInData+1]-0x30)*10000
            	       + (tmpLandis[bracketItemInData+2]-0x30)*1000
            	       + (tmpLandis[bracketItemInData+3]-0x30)*100
            	       + (tmpLandis[bracketItemInData+4]-0x30)*10
            	       + (tmpLandis[bracketItemInData+5]-0x30);
            	rawDec = (tmpLandis[bracketItemInData+7]-0x30)*10
            	       + (tmpLandis[bracketItemInData+8]-0x30);
            	       
            	rawInt = hexToBcd(rawInt);
            	rawDec = hexToBcd(rawDec);
       	      
       	    multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;     //�е�ǰ��������
        
       	    *(multiCopy[arrayItem].energy+offset)   = rawDec;
       	    *(multiCopy[arrayItem].energy+offset+1) = rawInt&0xff;
       	    *(multiCopy[arrayItem].energy+offset+2) = rawInt>>8 & 0xff;
       	    *(multiCopy[arrayItem].energy+offset+3) = rawInt>>16 & 0xff;
       	  }
          }
          
          //����������ʱ��
          //1.6.0(03.310*kW)(06-12-30 15:35)
          if (itemInChr>=40 && itemInChr<60)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
            	rawInt = (tmpLandis[bracketItemInData+0]-0x30)*10
            	       + (tmpLandis[bracketItemInData+1]-0x30);
            	rawDec = (tmpLandis[bracketItemInData+3]-0x30)*100
            	       + (tmpLandis[bracketItemInData+4]-0x30)*10
            	       + (tmpLandis[bracketItemInData+5]-0x30);
            	       
            	rawInt = hexToBcd(rawInt);
            	rawDec = hexToBcd(rawDec*10);
       	      
       	    multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;        //�е�ǰ��������
              
              *(multiCopy[arrayItem].reqAndReqTime+offset+0) = rawDec&0xff;
              *(multiCopy[arrayItem].reqAndReqTime+offset+1) = rawDec>>8&0xff;
              *(multiCopy[arrayItem].reqAndReqTime+offset+2) = rawInt&0xff;
              
              tmpLandis[bracketItemInData-1] = 0x30;
              
              bracketItemInData = findLeftBracket(tmpLandis);

              #ifdef SHOW_DEBUG_INFO
               printf("LandisGry��Լ����(��������ʱ��):bracketItemInData=%d,ƫ��:%02x\n", bracketItemInData, landisOffset[itemInChr]);
              #endif
              
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+0) = hexToBcd((tmpLandis[bracketItemInData+12]-0x30)*10 + (tmpLandis[bracketItemInData+13]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+1) = hexToBcd((tmpLandis[bracketItemInData+9]-0x30)*10 + (tmpLandis[bracketItemInData+10]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+2) = hexToBcd((tmpLandis[bracketItemInData+6]-0x30)*10 + (tmpLandis[bracketItemInData+7]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+3) = hexToBcd((tmpLandis[bracketItemInData+3]-0x30)*10 + (tmpLandis[bracketItemInData+4]-0x30));
            	*(multiCopy[arrayItem].reqAndReqTime+offset+27+4) = hexToBcd((tmpLandis[bracketItemInData+0]-0x30)*10 + (tmpLandis[bracketItemInData+1]-0x30));
            }
          }

          //���ʱ������
          //0.9.1(11:01:47)
          //0.9.2(12-07-12)
          if (itemInChr==60 || itemInChr==61)
          {
     	      multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //�вα�������

     	      if (itemInChr==61)
     	      {
     	      	*(multiCopy[arrayItem].paraVariable+offset) = 0x0;
     	      	offset++;
     	      }

            *(multiCopy[arrayItem].paraVariable+offset)   = hexToBcd((tmpLandis[bracketItemInData+6]-0x30)*10 + (tmpLandis[bracketItemInData+7]-0x30));
            *(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd((tmpLandis[bracketItemInData+3]-0x30)*10 + (tmpLandis[bracketItemInData+4]-0x30));
            *(multiCopy[arrayItem].paraVariable+offset+2) = hexToBcd((tmpLandis[bracketItemInData+0]-0x30)*10 + (tmpLandis[bracketItemInData+1]-0x30));
          }

          //�����ѹ
          //32.7(---.-*V)
          //52.7(---.-*V)
          //72.7(227.7*V)
          if (itemInChr>=64 && itemInChr<=66)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //�вα�������

          	 rawInt = (tmpLandis[bracketItemInData+0]-0x30)*100
          	         + (tmpLandis[bracketItemInData+1]-0x30)*10
          	         + (tmpLandis[bracketItemInData+2]-0x30);
          	 rawDec = (tmpLandis[bracketItemInData+4]-0x30);
          	 rawInt = hexToBcd(rawInt);
          	 rawDec = hexToBcd(rawDec);
              
              *(multiCopy[arrayItem].paraVariable+offset+0) = (rawInt<<4&0xff) | (rawDec&0xf);
              *(multiCopy[arrayItem].paraVariable+offset+1) = rawInt>>4&0xff;
            }
          }

          //�������
          //31.7(00.00*A)
          //51.7(00.00*A)
          //71.7(00.00*A)
          if (itemInChr>=67 && itemInChr<=69)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //�вα�������

          	  rawInt = (tmpLandis[bracketItemInData+0]-0x30)*10
          	         + (tmpLandis[bracketItemInData+1]-0x30);
          	  rawDec = (tmpLandis[bracketItemInData+3]-0x30)*10
          	         + (tmpLandis[bracketItemInData+4]-0x30);
          	  rawInt = hexToBcd(rawInt);
          	  rawDec = hexToBcd(rawDec);
              
              *(multiCopy[arrayItem].paraVariable+offset+0) = rawDec<<4&0xff;
              *(multiCopy[arrayItem].paraVariable+offset+1) = (rawInt<<4&0xf0) | rawDec>>4&0xf;
              *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt>>4&0xff;
            }
          }
          
          //�������ô���
          //C.2.0(00000003)
          if (itemInChr==70)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //�вα�������
     	        
     	        rawInt = (tmpLandis[bracketItemInData+0]-0x30)*10000000
          	         + (tmpLandis[bracketItemInData+1]-0x30)*1000000
          	         + (tmpLandis[bracketItemInData+2]-0x30)*100000
          	         + (tmpLandis[bracketItemInData+3]-0x30)*10000
          	         + (tmpLandis[bracketItemInData+4]-0x30)*1000
          	         + (tmpLandis[bracketItemInData+5]-0x30)*100
          	         + (tmpLandis[bracketItemInData+6]-0x30)*10
          	         + (tmpLandis[bracketItemInData+7]-0x30)*1;
              *(multiCopy[arrayItem].paraVariable+offset+0) = rawInt&0xff;
              *(multiCopy[arrayItem].paraVariable+offset+1) = rawInt>>8&0xff;
              *(multiCopy[arrayItem].paraVariable+offset+2) = rawInt>>16&0xff;
     	      }
          }
          
          //�ϴβ�������ʱ��
          //C.2.1(05-10-19 16:36)
          if (itemInChr==71)
          {
            if (tmpLandis[bracketItemInData+0]!='-')
            {
     	        multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE;  //�вα�������
     	        
              *(multiCopy[arrayItem].paraVariable+offset+0) = 0x0;    //��
          	*(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd((tmpLandis[bracketItemInData+12]-0x30)*10 + (tmpLandis[bracketItemInData+13]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+2) = hexToBcd((tmpLandis[bracketItemInData+9]-0x30)*10 + (tmpLandis[bracketItemInData+10]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+3) = hexToBcd((tmpLandis[bracketItemInData+6]-0x30)*10 + (tmpLandis[bracketItemInData+7]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+4) = hexToBcd((tmpLandis[bracketItemInData+3]-0x30)*10 + (tmpLandis[bracketItemInData+4]-0x30));
          	*(multiCopy[arrayItem].paraVariable+offset+5) = hexToBcd((tmpLandis[bracketItemInData+0]-0x30)*10 + (tmpLandis[bracketItemInData+1]-0x30));
     	      }
          }
          
          #ifdef SHOW_DEBUG_INFO
           printf("\n");
          #endif
        }
    	  
    	  break;
    	}
    	else
    	{
    	  frameOffset++;
    	}
    }
    
    //��λ
    frame += frameOffset;
    if (loadLen>=frameOffset)
    {
      loadLen -= frameOffset;
    }
    else
    {
    	break;
    }
  }
  
  return METER_NORMAL_REPLY;
}

#endif

#ifdef PROTOCOL_ABB_GROUP

/*******************************************************
��������:CRC_ABB
��������:����16λABB��CRCֵ
���ú���:
�����ú���:
�������:aData,Ҫ����CRCУ�����Ϣ
         aSize,��Ϣ���ֽ���
�������:
����ֵ��CRCֵ
*******************************************************/
unsigned short CRC_ABB(unsigned char * aData, unsigned long aSize)
{
	int i,j;
	int flag;
	unsigned int iTemp=0;
	
	for(i=0; i<aSize; i++)
	{
	  iTemp ^= (aData[i]<<8);
	  for(j=0; j<8; j++)
	  {
	  	flag = iTemp&0x8000;
	  	iTemp<<=1;
	  	if (flag)
	  	{
	  	  iTemp ^= 0x1021;
	  	}
	  }
	}
	
	return iTemp;
}

/*******************************************************
��������:abbEncryption
��������:ABB�������
���ú���:
�����ú���:
�������:lKey,���ؿ���
         rKey,Զ��ͨѶ����
�������:
����ֵ��CRCֵ
*******************************************************/
unsigned long abbEncryption(unsigned long lKey, unsigned long rKey)
{
	 unsigned long pword;
	 int           i;
	 int           j, k=0;
	 
	 //printf("lKey=%08X,rKey=%08X\n", lKey, rKey);

	 union
	 {
	 	 unsigned long key;    //encryption key
	 	 
	 	 //broken into bytes
	 	 struct
	 	 {
	 	 	 unsigned char byta,bytb,bytc,bytd;
	 	 }parts;
	 }val;
	 
	 val.key = rKey;
	 pword   = lKey;
	 
	 //Add an arbitrary number to the key just for fun
	 val.key += 0xab41;
	 
	 //generate a four bit checksum to be used as loop index
	 i = val.parts.byta + val.parts.bytb+val.parts.bytc+val.parts.bytd;
	 i = i&0xf;
	 while(i>=0)
	 {
	 	 //Set 'j' to the value of the high bit before shifting.Simulates carry flag
	 	 if (val.parts.bytd>=0x80)
	 	 {
	 	 	 j=1;
	 	 }
	 	 else
	 	 {
	 	 	 j=0;
	 	 }
	 	 
	 	 //Shift the key.Add in the carry flag from the previous loop
	 	 val.key = val.key<<1;
	 	 val.key += k;
	 	 k =j;
	 	 
	 	 //Apply the key to the password
	 	 pword ^= val.key;
	 	 i--;
	 }
	 
	 //printf("pword=%08X\n", pword);
	 
	 return pword;
}


/***************************************************
��������:processAbbData
��������:ABB�������ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S abbBcdToData(INT8U *buf, INT16U offset, INT8U *inData, INT8U dploce,INT8U *calcBuf)
{
  INT8U j,k;
  
  if ((dploce+6)%2==0)
  {
    //С��
    buf[offset] = inData[7-(dploce+6)/2];
    
    calcBuf[0] = inData[7-(dploce+6)/2+1];
    calcBuf[1] = inData[7-(dploce+6)/2];
    
    //����
    memset(buf+offset+1, 0x00, 3);
    
    for(j=7-(dploce+6)/2,k=1; j>0; j--,k++)
    {
      buf[offset+k] = inData[j-1];
      
      calcBuf[k+1] = inData[j-1];
    }
  }
  else
  {
    //С��
    buf[offset] = (inData[7-(dploce+6)/2]>>4) | (inData[7-(dploce+6)/2-1]&0xf)<<4;
    
    //����
    memset(buf+offset+1, 0x00, 3);
    
    for(j=7-(dploce+6)/2,k=1; j>0; j--,k++)
    {
      buf[offset+k] = inData[j-1]>>4;

      if (j>1)
      {
        buf[offset+k] |= (inData[j-2]&0xf)<<4;
      }
    }
  }
}

/***************************************************
��������:processAbbData
��������:ABB�������ݴ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
***************************************************/
INT8S processAbbData(INT8U arrayItem, INT8U *frame,  INT16U loadLen, INT8U copyItem)
{
  INT8U  i, j, k;
  INT8U  eblkcf=0;
  INT8U  offsetItem=0;
  INT16U offsetVision, offsetReq, offsetReqTime;
  INT8U  dploce, dplocd;
  INT8U  calcBuf[10];
  static INT32U sumInt, sumDec;

  //printf("processAbbData(%d):", copyItem);
 	 
  //for(i=0; i<loadLen; i++)
  //{
  //  printf("%02x ", frame[i]);
  //}
  //printf("\n");
  
  dploce = abbClass0[11];
  dplocd = abbClass0[12];
  
  if ((copyItem%2)==0)
  {
  	sumInt = 0;
  	sumDec = 0;
  }
  
  switch(copyItem)
  {
  	case 0:
  	case 1:
  		eblkcf = abbClass2[51];
  		offsetItem = copyItem;
  		break;

  	case 2:
  	case 3:
  		eblkcf = abbClass2[52];
  		offsetItem = copyItem-2;
  		break;

  	case 4:
  	case 5:
  		//eblkcf = abbClass2[84];
  		eblkcf = 3;    //�޷���class2�л�ȡ�����ݿ�Ĺ�����,��Ϊ�޹�����
  		offsetItem = copyItem-4;
  		break;

  	case 6:
  	case 7:
  		//eblkcf = abbClass2[85];
  		eblkcf = 0x0c;    //�޷���class2�л�ȡ�����ݿ�Ĺ�����,��Ϊ�޹�����
  		offsetItem = copyItem-6;
  		break;
    
    case 8:    //������һ������ʾֵ
      offsetVision = QUA4_NO_WORK_OFFSET;
      for(i=0; i<4; i++)
      {
    	  abbBcdToData(multiCopy[arrayItem].energy, offsetVision, frame+i*7, dploce, calcBuf);
    	  
    	  switch(i)
    	  {
    	  	case 0:
    	  		offsetVision = QUA3_NO_WORK_OFFSET;
    	  		break;

    	  	case 1:
    	  		offsetVision = QUA2_NO_WORK_OFFSET;
    	  		break;

    	  	case 2:
    	  		offsetVision = QUA1_NO_WORK_OFFSET;
    	  		break;
    	  }
      }
    	
      return METER_REPLY_ANALYSE_OK;
  }
  
  switch (eblkcf)
  {
	  case 0x80:    //�й����� KW-del
	    offsetVision  = POSITIVE_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_POSITIVE_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_P_WORK_OFFSET+5+offsetItem*10;
	    //printf("�й���������\n");
	    break;
	    
	  case 0x40:    //�й����� KW-rec
	    offsetVision  = NEGTIVE_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_NEGTIVE_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_N_WORK_OFFSET+5+offsetItem*10;
	    //printf("�й���������\n");
	  	break;
	  
	  case 0x03:    //�޹����� KVAR-del
	    offsetVision  = POSITIVE_NO_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_POSITIVE_NO_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_P_NO_WORK_OFFSET+5+offsetItem*10;
	    //printf("�޹���������\n");
	  	break;

	  case 0x0C:    //�޹����� KVAR-rec
	    offsetVision  = NEGTIVE_NO_WORK_OFFSET+4+offsetItem*8;
	    offsetReq     = REQ_NEGTIVE_NO_WORK_OFFSET+3+offsetItem*6;
	    offsetReqTime = REQ_TIME_N_NO_WORK_OFFSET+5+offsetItem*10;
	    //printf("�޹���������\n");
	  	break;
	  	
	  default:
	  	return METER_NORMAL_REPLY;
  }
  
  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;  //�е�ǰ����ʾֵ����
  multiCopy[arrayItem].hasData |= HAS_CURRENT_REQ;     //�е�ǰ��������
         
  //ÿ42���ֽ����������ʵ�����,��������֡�صĲ���A,B,C,D���ʵ�����
  for(i=0; i<2; i++)
  {
    //1����(7Bytes,BCD��,С��λ����CLASS 0�е�DPLOCE+6����)
    memset(calcBuf, 0x0, 10);
    abbBcdToData(multiCopy[arrayItem].energy, offsetVision, frame+i*21, dploce, calcBuf);
    offsetVision += 4;
    
    //�ۼӼ�����ʾֵ
    sumDec += bcdToHex(calcBuf[1]<<8 | calcBuf[0]);
    sumInt += bcdToHex(calcBuf[3]<<16 | calcBuf[3]<<8 | calcBuf[2]);
    sumInt += sumDec/10000;
    sumDec = sumDec%10000;
  
    //2�������(3Bytes,BCD��,С��λ����CLASS 0�е�DPLOCD����)
    if ((dplocd%2)==0)
    {
      //����
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+2) = frame[i*21+7+(3-dplocd/2-1)];

      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+0) = 0x0;
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+1) = 0x0;
      for(j=2,k=0; j>0 && k<(dplocd/2); j--,k++)
      {
        *(multiCopy[arrayItem].reqAndReqTime+offsetReq+j-1)   = frame[i*21+7+(3-dplocd/2)+k];
      }
    }
    else
    {
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+0) = 0x0;
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+1) = 0x0;

      //����
      *(multiCopy[arrayItem].reqAndReqTime+offsetReq+2) = frame[i*21+7+(3-dplocd/2-1)]>>4;
      if (dplocd>1)
      { 
      	*(multiCopy[arrayItem].reqAndReqTime+offsetReq+2) |= (frame[i*21+7+(3-dplocd/2-2)]&0xf)<<4;
      }
      else
      {
        //ֻ��һλС����С��λ����
        *(multiCopy[arrayItem].reqAndReqTime+offsetReq+1) = frame[i*21+7+(3-dplocd/2-1)]<<4;
      }

      for(j=2,k=0; j>0 && k<((dplocd+1)/2); j--,k++)
      {
        *(multiCopy[arrayItem].reqAndReqTime+offsetReq+j-1) = (frame[i*21+7+(3-dplocd/2-1)+k]&0xf)<<4;
        
        if (j>1)
        {
          *(multiCopy[arrayItem].reqAndReqTime+offsetReq+j-1) |= frame[i*21+7+(3-dplocd/2)+k]>>4;
        }
      }
    }
    
    offsetReq += 3;
    
    //3�����������ʱ��(5Bytes,BCD��ʽ��������ʱ��)
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime)   = frame[i*21+14];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+1) = frame[i*21+13];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+2) = frame[i*21+12];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+3) = frame[i*21+11];
    *(multiCopy[arrayItem].reqAndReqTime+offsetReqTime+4) = frame[i*21+10];
    offsetReqTime += 5;
  }
  
  if (copyItem%2)
  {
  	offsetVision -= 20;
  	sumDec = hexToBcd(sumDec);
  	sumInt = hexToBcd(sumInt);
  	multiCopy[arrayItem].energy[offsetVision] = sumDec>>8;
  	multiCopy[arrayItem].energy[offsetVision+1] = sumInt&0xff;
  	multiCopy[arrayItem].energy[offsetVision+2] = sumInt>>8;
  	multiCopy[arrayItem].energy[offsetVision+3] = sumInt>>16;
  }
  
  return METER_NORMAL_REPLY;
}

#endif


#ifdef PROTOCOL_MODUBUS_GROUP

/***************************************************
��������:processHyData
��������:ɽ����Զ��Լ�������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processHyData(INT8U arrayItem, INT8U *data,INT8U length)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;

  if (16==length)
  {
	  //�е�ǰ��������
	  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;
	  
	  //�����й�����,��λWh
	  offset = POSITIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����й�����,��λWh
	  offset = NEGTIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����޹�����,��λVarh,�����޹�
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����޹�����,��λVarh,�����޹�
	  offset = NEGTIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

  }
  else
  {
	  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������

	  //��ѹ
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
	    tmpData = hexToBcd((data[tmpTail]<<8 | data[tmpTail+1])/10);    
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 2;
			offset += 2;
	  }
	  //Ԥ��2���Ĵ���
	  tmpTail += 4;
	  
	  //�����ߵ�ѹ
	  tmpTail += 6;
	  
	  //Ԥ��1���Ĵ���
	  tmpTail += 2;
	  
	  //����
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
	    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
		tmpTail += 2;
		offset += 3;
	  }

	  //Ԥ��2���Ĵ���
	  tmpTail += 4;
	  
	  //�й�����
	  offset = POWER_PHASE_A_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //���й�����
	  offset = POWER_INSTANT_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //�޹�����
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		 	 tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //���޹�����
	  offset = POWER_INSTANT_NO_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //���ڹ���
	  offset = POWER_PHASE_A_APPARENT;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //�����ڹ���
	  offset = POWER_INSTANT_APPARENT; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;

	  //��������
	  offset = FACTOR_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x8000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 2;
			offset += 2;
	  }  
	  //�ܹ�������
	  offset = TOTAL_POWER_FACTOR; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 2;

	  //��ѹƵ��
	  offset = METER_STATUS_WORD;    //2017-7-19,��ѹƵ�ʷ��ڵ���һ��״̬���� 
    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail+=2;
  }
  
  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processAssData
��������:������ɭ˼��Լ�������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processAssData(INT8U arrayItem, INT8U *data,INT8U length)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������

  //��ѹ
  offset = VOLTAGE_PHASE_A;
  for(i=0; i<3; i++)
  {
    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);    
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	
		tmpTail += 2;
		offset += 2;
  }
  
  //�����ߵ�ѹ
  tmpTail += 6;
  
  //����
  offset = CURRENT_PHASE_A;
  for(i=0; i<3; i++)
  {
    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }
  
  //�й�����
  offset = POWER_PHASE_A_WORK;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
		if (data[tmpTail]&0x80)
		{
	  	tmpData |= 0x800000;
		}
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }  
  //���й�����
  offset = POWER_INSTANT_WORK; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
  if (data[tmpTail]&0x80)
  {
		tmpData |= 0x800000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
  tmpTail += 2;
  
  //�޹�����
  offset = POWER_PHASE_A_NO_WORK;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
		if (data[tmpTail]&0x80)
		{
	  	tmpData |= 0x800000;
		}
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }  
  //���޹�����
  offset = POWER_INSTANT_NO_WORK; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
  if (data[tmpTail]&0x80)
  {
		tmpData |= 0x800000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
  tmpTail += 2;
  
  //���ڹ���
  offset = POWER_PHASE_A_APPARENT;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
		if (data[tmpTail]&0x80)
		{
		  tmpData |= 0x800000;
		}
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	
		tmpTail += 2;
		offset += 3;
  }  
  //�����ڹ���
  offset = POWER_INSTANT_APPARENT; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
  if (data[tmpTail]&0x80)
  {
	tmpData |= 0x800000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
  tmpTail += 2;

  //��������
  offset = FACTOR_PHASE_A;
  for(i=0; i<3; i++)
  {
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 2));
		if (data[tmpTail]&0x80)
		{
		  tmpData |= 0x8000;
		}

    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	
		tmpTail += 2;
		offset += 2;
  }  
  //�ܹ�������
  offset = TOTAL_POWER_FACTOR; 
  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2));
  if (data[tmpTail]&0x80)
  {
		tmpData |= 0x8000;
  }
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  tmpTail += 2;

  //��ѹƵ��
  offset = METER_STATUS_WORD;    //2017-7-19,��ѹƵ�ʷ��ڵ���һ��״̬���� 
  tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]<<0);
  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
  tmpTail+=2;
  
  //�е�ǰ��������
  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

  //�����й�����,��λWh
  offset = POSITIVE_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //���KWh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  //�����й�����,��λWh
  offset = NEGTIVE_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //���KWh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  //�����޹�����,��λVarh,�����޹�
  offset = POSITIVE_NO_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //���KVarh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  //�����޹�����,��λVarh,�����޹�
  offset = NEGTIVE_NO_WORK_OFFSET; 
  tmpData = data[tmpTail]<<8 | data[tmpTail+1]<<0 | data[tmpTail+2]<<24 | data[tmpTail+3]<<16;
  tmpData /= 10;    //���KVarh
  tmpData = hexToBcd(tmpData);
  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
  tmpTail += 4;
  
  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processXyData
��������:�Ϻ���ҵmodbus���������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processXyData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  INT8U  sign=0;
  
  INT8U  retVal[5];
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������

 #ifdef SHOW_DEBUG_INFO
  printf("%s==>protocol=%d,item=%d,copyitem=%d\n", __func__, protocol, arrayItem, multiCopy[arrayItem].copyItem);
 #endif
  
  if(
     protocol!=MODBUS_XY_M
     || (1==multiCopy[arrayItem].copyItem && MODBUS_XY_M==protocol)
  	)
  {
	  //��ѹ
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = (hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2])<<4) | (retVal[1]/10);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>��ѹ%d:����=%d,С��=%02d%02d,V=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1], retVal[0], tmpData);
		 #endif
		 
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
	    tmpTail += 4;
	  	offset += 2;
	  }

	  if (MODBUS_XY_F==protocol || MODBUS_XY_M==protocol)
	  {
	    //�������ߵ�ѹ
	    tmpTail += 12;
	  }

	  //����
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*1000 + (retVal[1]*100+retVal[2])/10);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>����%d:����=%d,С��=%02d%02d,C=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
  }

  if (MODBUS_XY_F==protocol || MODBUS_XY_M==protocol )
  {
	  if(
		 protocol!=MODBUS_XY_M
		 || (1==multiCopy[arrayItem].copyItem && MODBUS_XY_M==protocol)
		)
	  {
		  //�й�����
		  offset = POWER_PHASE_A_WORK;
		  for(i=0; i<3; i++)
		  {
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				  
				if (sign)
				{
				  tmpData |= 0x800000;
				}
		    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
		    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
		  }

		  if (MODBUS_XY_M==protocol)
		  {
				return METER_REPLY_ANALYSE_OK;
		  }
	  }
		
	  //���й�����
	  offset = POWER_INSTANT_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //�޹�����
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }  
	  //���޹�����
	  offset = POWER_INSTANT_NO_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //���ӹ�����
	  offset = POWER_INSTANT_APPARENT; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //�ܹ�������
	  offset = TOTAL_POWER_FACTOR; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
    if (MODBUS_XY_M==protocol)
    {
      tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
    }
	  else
	  {
	    tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
    }
		
	 #ifdef SHOW_DEBUG_INFO
	  printf("%s==>��������:����=%d,С��=%02d%02d,Factor=%x\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
	 #endif
	 
	  if (sign)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 4;
	  
	  //��ѹƵ��
	  offset = METER_STATUS_WORD;    //2017-7-19,��ѹƵ�ʷ��ڵ���һ��״̬���� 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  *(multiCopy[arrayItem].paraVariable+offset+0) = hexToBcd(retVal[1]);
	  *(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(retVal[2]);
	  tmpTail+=4;

	  //�����й�����
	  //�е�ǰ��������
	  multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //�����й�����,��λWh
	  offset = POSITIVE_WORK_OFFSET; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

	  
	  if (MODBUS_XY_M==protocol)
	  {
		  //�����й�����,��λWh
		  offset = NEGTIVE_WORK_OFFSET; 
		  sign = decodeIeee754Float(&data[tmpTail], retVal);
		  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
		  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  }
	  else
	  {
	    //������
	  }
	  tmpTail += 4;
	  
	  //�����޹�����,��λWh
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  if (MODBUS_XY_M==protocol)
	  {
		  //������  ������,��λWh
		  offset = NEGTIVE_NO_WORK_OFFSET; 
		  sign = decodeIeee754Float(&data[tmpTail], retVal);
		  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10+retVal[1]/10);
		  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  }
	  else
	  {
	    //������
	  }
	  tmpTail += 4;
  }

  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processSwitchFData
��������:������ģ���������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processSwitchData(INT8U arrayItem, INT8U *data,INT8U length)
{ 
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������
  
  //2017-8-22,����״̬���ڵ��ڶ���״̬����
  *(multiCopy[arrayItem].paraVariable+METER_STATUS_WORD_2+0) = *(data+1);    //���ֽ�
  *(multiCopy[arrayItem].paraVariable+METER_STATUS_WORD_2+1) = *data;        //���ֽ�
  
  return METER_REPLY_ANALYSE_OK;	//��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processMwData
��������:�ɶ�����modbus���������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
	INT8U processMwData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
	{
		INT32U tmpData;
		INT8U  tmpTail = 0;
		INT8U  i;
		INT16U offset;
		INT8U  sign=0;
		
		INT8U  retVal[5];
		
		multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������
	
		if(MODBUS_MW_F==protocol)
		{
			//��ѹƵ��
			offset = METER_STATUS_WORD; 	 //2018-3-1,��ѹƵ�ʷ��ڵ���һ��״̬���� 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			*(multiCopy[arrayItem].paraVariable+offset+0) = hexToBcd(retVal[1]);
			*(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(retVal[2]);
			tmpTail+=4;
		}
		
		if (MODBUS_MW_F==protocol || MODBUS_MW_UI==protocol)
		{
			//��ѹ
			offset = VOLTAGE_PHASE_A;
			for(i=0; i<3; i++)
			{
				decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = (hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2])<<4) | (retVal[1]/10);
			 #ifdef SHOW_DEBUG_INFO
				printf("%s==>��ѹ%d:����=%d,С��=%02d%02d,V=%x\r\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1], retVal[0], tmpData);
			 #endif
			
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			
				tmpTail += 4;
				offset += 2;
			}
	
			//��ƽ����ѹ�������ߵ�ѹ���ߵ�ѹƽ��ֵ
			if(MODBUS_MW_UI==protocol )
			{
				tmpTail += 16;
			}
			else
			{
				tmpTail += 20;
			}
	
			//����
			offset = CURRENT_PHASE_A;
			for(i=0; i<3; i++)
			{
				decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*1000 + (retVal[1]*100+retVal[2])/10);
				
			 #ifdef SHOW_DEBUG_INFO
				printf("%s==>����%d:����=%d,С��=%02d%02d,C=%x\r\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
			 #endif
			
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
		}
	
		if (MODBUS_MW_F==protocol)
		{
			//�������ƽ��ֵIavg
			tmpTail += 4;
			
			//���ߵ���In
			tmpTail += 4;
	
			//�й�����
			offset = POWER_PHASE_A_WORK;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
					
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
			
			//���й�����
			offset = POWER_INSTANT_WORK; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			tmpTail += 4;
	
			//�޹�����
			offset = POWER_PHASE_A_NO_WORK;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
			//���޹�����
			offset = POWER_INSTANT_NO_WORK; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			tmpTail += 4;
	
			//���ڹ���
			offset = POWER_PHASE_A_APPARENT;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
					
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			
				tmpTail += 4;
				offset += 3;
			}
			//���ӹ�����
			offset = POWER_INSTANT_APPARENT; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
			tmpTail += 4;
	
			//��������
			offset = FACTOR_PHASE_A;
			for(i=0; i<3; i++)
			{
				sign = decodeIeee754Float(&data[tmpTail], retVal);
				//tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
				tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
				
			 #ifdef SHOW_DEBUG_INFO
				printf("��������%d:����=%d,С��=%02d%02d,Factor=%x\r\n",i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
			 #endif
			 
				if (sign)
				{
					tmpData |= 0x800000;
				}
				*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
				*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			
				tmpTail += 4;
				offset += 2;
			}
			
			//�ܹ�������
			offset = TOTAL_POWER_FACTOR; 
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			//tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
			tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("��������:����=%d,С��=%02d%02d,Factor=%x\r\n", retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		 
			if (sign)
			{
				tmpData |= 0x8000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			tmpTail += 4;
			
			//�й���������Dmd_P
			tmpTail += 4;
			
			//�޹���������Dmd_Q
			tmpTail += 4;
			
			//���ڹ�������Dmd_S
			tmpTail += 4;
	
			//�е�ǰ�������� 	
			multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;
	
			//�����й�����,��λWh
			offset = POSITIVE_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
			
			//�����й�����,��λWh
			offset = NEGTIVE_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
			
			//�����޹�����,��λWh
			offset = POSITIVE_NO_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
			
			//������  ������,��λWh
			offset = NEGTIVE_NO_WORK_OFFSET; 
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)*10);
			*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
			*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
			tmpTail += 4;
		}
	
		return METER_REPLY_ANALYSE_OK;	//��������֡�����ҽ�����ȷ,�ѱ��������
	}


/***************************************************
��������:processJzData
��������:�Ϻ�����modbus���������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processJzData(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  INT8U  sign=0;
  
  multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������

  if(MODBUS_JZ_F==protocol)
  {
	  //��ѹƵ��
	  offset = METER_STATUS_WORD;    //2018-3-5,��ѹƵ�ʷ��ڵ���һ��״̬���� 
		tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);
    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail+=2;
	}
	
	if (MODBUS_JZ_F==protocol || MODBUS_JZ_UI==protocol)
	{
		//��ѹ
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>��ѹ%d=%x\n", __func__, i+1, tmpData);
		 #endif
		 
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    tmpTail += 2;
	  	offset += 2;
	  }

	  //��ƽ����ѹ�������ߵ�ѹ���ߵ�ѹƽ��ֵ
	  tmpTail += 10;

	  //����
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
	    tmpData = hexToBcd(data[tmpTail]<<8 | data[tmpTail+1]);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>����%d=%x\n", __func__, i+1, tmpData);
		 #endif
			
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
		  tmpTail += 2;
		  offset += 3;
	  }
  }

  if (MODBUS_JZ_F==protocol)
  {
	  //�������ƽ��ֵIavg
		tmpTail += 2;
		
	  //���ߵ���In
		tmpTail += 2;

	  //�й�����
	  offset = POWER_PHASE_A_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }
	  //���й�����
	  offset = POWER_INSTANT_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //�޹�����
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		 	 tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //���޹�����
	  offset = POWER_INSTANT_NO_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;
	  
	  //���ڹ���
	  offset = POWER_PHASE_A_APPARENT;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 2;
			offset += 3;
	  }  
	  //�����ڹ���
	  offset = POWER_INSTANT_APPARENT; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 2;

	  //��������
	  offset = FACTOR_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x8000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 2;
			offset += 2;
	  }  
	  //�ܹ�������
	  offset = TOTAL_POWER_FACTOR; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 2)*10);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 2;

		//��ѹ���Գƶ�
	  tmpTail += 2;

		//��������
	  tmpTail += 2;
	  
		//�й���������Dmd_P
		tmpTail += 2;
		
		//�޹���������Dmd_Q
		tmpTail += 2;
		
		//���ڹ�������Dmd_S
		tmpTail += 2;
		
		//DI
	  tmpTail += 2;
		
		//DO״̬
	  tmpTail += 2;
		

	  //�е�ǰ��������	  
		multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //�����й�����,��λWh
	  offset = POSITIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����й�����,��λWh
	  offset = NEGTIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KWh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����޹�����,��λVarh,�����޹�
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����޹�����,��λVarh,�����޹�
	  offset = NEGTIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData *= 10;	  //���KVarh
	  tmpData = hexToBcd(tmpData);
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
  }

  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processWe6301Data
��������:��˹��we6301���������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processWe6301Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;

 #ifdef SHOW_DEBUG_INFO
	printf("%s==>multiCopy[%d].copyItem=%d\n", __func__, arrayItem, multiCopy[arrayItem].copyItem);
 #endif
	
  if (1==multiCopy[arrayItem].copyItem)
  {
  	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������
  		
		//��ѹ
		offset = VOLTAGE_PHASE_A;
		for(i=0; i<3; i++)
		{
			tmpData = hexToBcd((data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3])/100);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>��ѹ%d=%x\n", __func__, i+1, tmpData);
		 #endif
		 
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			tmpTail += 4;
			offset += 2;
		}
		
		//�����ߵ�ѹ
		tmpTail += 12;
		
		//����
		offset = CURRENT_PHASE_A;
		for(i=0; i<3; i++)
		{
			tmpData = hexToBcd(data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3]);
		
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>����%d=%x\n", __func__, i+1, tmpData);
		 #endif
			
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
		}

    //�����ֽ�
		tmpTail += 12;
		
		//�ܹ�������
		offset = TOTAL_POWER_FACTOR; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
		if (data[tmpTail]&0x80)
		{
			tmpData |= 0x8000;
		}
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>�ܹ�������=%x\n", __func__, tmpData);
	 #endif

		*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		tmpTail += 4;
		
		//��ѹƵ��
		offset = METER_STATUS_WORD; 	 //2018-4-25,��ѹƵ�ʷ��ڵ���һ��״̬���� 
		tmpData = hexToBcd(data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3]);

	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>Ƶ��=%x\n", __func__, tmpData);
	 #endif
	 
		*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		tmpTail += 4;

    //�����ߵ���
		tmpTail += 4;
		
		//��������
		offset = FACTOR_PHASE_A;
		for(i=0; i<3; i++)
		{
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
			if (data[tmpTail]&0x80)
			{
				tmpData |= 0x8000;
			}
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>��������%d=%x\n", __func__, i, tmpData);
		 #endif
			
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 4;
			offset += 2;
		}  
  }
	else if (2==multiCopy[arrayItem].copyItem)
  {
		multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������

	  //���й�����
	  offset = POWER_INSTANT_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>���й�����=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //���޹�����
	  offset = POWER_INSTANT_NO_WORK; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>���޹�����=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;
		
	  //�����ڹ���
	  offset = POWER_INSTANT_APPARENT; 
	  tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
	  if (data[tmpTail]&0x80)
	  {
			tmpData |= 0x800000;
	  }
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����ڹ���=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;
		
	  //�й�����
	  offset = POWER_PHASE_A_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
			if (data[tmpTail]&0x80)
			{
		  	tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
	  
	  //�޹�����
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
			if (data[tmpTail]&0x80)
			{
		 	 tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }  
	  
	  //���ڹ���
	  offset = POWER_PHASE_A_APPARENT;
	  for(i=0; i<3; i++)
	  {
			tmpData = hexToBcd(bmToYm(&data[tmpTail], 4)/100);
			if (data[tmpTail]&0x80)
			{
			  tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
  }
	else if (3==multiCopy[arrayItem].copyItem)
  {
	  //�е�ǰ��������	  
		multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //�����й�����,��λ0.01kWh
	  offset = POSITIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);
		
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����й�����=%x\n", __func__, tmpData);
	 #endif

	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����й�����,��λ0.01kWh
	  offset = NEGTIVE_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����й�����=%x\n", __func__, tmpData);
	 #endif
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

    //�����ֽ�,����
		tmpTail += 4;

		//�й��ܵ���
		tmpTail += 4;
	  
	  //�����޹�����,��λ0.01kVarh
	  offset = POSITIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);
	 #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����޹�����=%x\n", __func__, tmpData);
	 #endif
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
	  //�����޹�����,��λ0.01kVarh
	  offset = NEGTIVE_NO_WORK_OFFSET; 
	  tmpData = data[tmpTail]<<24 | data[tmpTail+1]<<16 | data[tmpTail+2]<<8 | data[tmpTail+3];
	  tmpData = hexToBcd(tmpData);

   #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����޹�����=%x\n", __func__, tmpData);
	 #endif
	 
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	}

  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}

/***************************************************
��������:processPmc350Data
��������:�����е�PMC350 modbus���������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬length���ݳ���
�������:
����ֵ��
***************************************************/
INT8U processPmc350Data(INT8U arrayItem, INT8U *data,INT8U length, INT8U protocol)
{
  INT32U tmpData;
  INT8U  tmpTail = 0;
  INT8U  i;
  INT16U offset;
  INT8U  sign=0;
  
  INT8U  retVal[5];
	
 #ifdef SHOW_DEBUG_INFO
	printf("%s==>multiCopy[%d].copyItem=%d\n", __func__, arrayItem, multiCopy[arrayItem].copyItem);
 #endif
	
  if (1==multiCopy[arrayItem].copyItem)
  {
  	multiCopy[arrayItem].hasData |= HAS_PARA_VARIABLE; //�в�������

		//��ѹ
	  offset = VOLTAGE_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = (hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2])<<4) | (retVal[1]/10);

		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>��ѹ%d:����=%d,С��=%02d%02d,V=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1], retVal[0], tmpData);
		 #endif
		
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
	    tmpTail += 4;
	  	offset += 2;
	  }

	  //ƽ�����ѹ�������ߵ�ѹ��ƽ���ߵ�ѹ
		tmpTail += 20;

	  //����
	  offset = CURRENT_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*1000 + (retVal[1]*100+retVal[2])/10);
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>����%d:����=%d,С��=%02d%02d,C=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
		  tmpTail += 4;
		  offset += 3;
	  }
		
	  //ƽ�������Iavg
		tmpTail += 4;

		//�й�����
		offset = POWER_PHASE_A_WORK;
		for(i=0; i<3; i++)
		{
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				
			if (sign)
			{
				tmpData |= 0x800000;
			}
			
     #ifdef SHOW_DEBUG_INFO
			printf("%s==>�й�����%d:����=%d,С��=%02d%02d,P=%x\n", __func__, i, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		 
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
		}
		
	  //���й�����
	  offset = POWER_INSTANT_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
   #ifdef SHOW_DEBUG_INFO
	  printf("%s==>���й�����:����=%d,С��=%02d%02d,P=%x\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
	 #endif
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //�޹�����
	  offset = POWER_PHASE_A_NO_WORK;
	  for(i=0; i<3; i++)
	  {
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
			if (sign)
			{
				tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
	  }
	  //���޹�����
	  offset = POWER_INSTANT_NO_WORK; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

		//���ڹ���
		offset = POWER_PHASE_A_APPARENT;
		for(i=0; i<3; i++)
		{
			sign = decodeIeee754Float(&data[tmpTail], retVal);
			tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
				
			if (sign)
			{
				tmpData |= 0x800000;
			}
			*(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
			*(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
		
			tmpTail += 4;
			offset += 3;
		}
	  //���ӹ�����
	  offset = POWER_INSTANT_APPARENT; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
	  tmpData = hexToBcd((retVal[4]<<16 | retVal[3]<<8 | retVal[2])*10 + retVal[1]/10);
	  if (sign)
	  {
			tmpData |= 0x800000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+2) = tmpData>>16&0xff;
	  tmpTail += 4;

	  //��������
	  offset = FACTOR_PHASE_A;
	  for(i=0; i<3; i++)
	  {
			sign = decodeIeee754Float(&data[tmpTail], retVal);
		  //tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
	    tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
			
		 #ifdef SHOW_DEBUG_INFO
			printf("%s==>��������%d:����=%d,С��=%02d%02d,Factor=%x\n", __func__, i+1, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
		 #endif
		 
			if (sign)
			{
				tmpData |= 0x800000;
			}
	    *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	    *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
		
			tmpTail += 4;
			offset += 2;
	  }
		
		//�ܹ�������
	  offset = TOTAL_POWER_FACTOR; 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
		//tmpData = hexToBcd(retVal[4]<<16 | retVal[3]<<8 | retVal[2]);
	  tmpData = hexToBcd(retVal[2]*1000 + (retVal[1]*100+retVal[0])/10);
		
	 #ifdef SHOW_DEBUG_INFO
	 	printf("%s==>��������:����=%d,С��=%02d%02d,Factor=%x\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0],tmpData);
	 #endif
	 
	  if (sign)
	  {
			tmpData |= 0x8000;
	  }
	  *(multiCopy[arrayItem].paraVariable+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].paraVariable+offset+1) = tmpData>>8&0xff;
	  tmpTail += 4;
		
	  //��ѹƵ��
	  offset = METER_STATUS_WORD;    //2018-4-27,��ѹƵ�ʷ��ڵ���һ��״̬���� 
	  sign = decodeIeee754Float(&data[tmpTail], retVal);
		
   #ifdef SHOW_DEBUG_INFO
	  printf("%s==>Ƶ��:����=%d,С��=%02d%02d\n", __func__, retVal[4]<<16 | retVal[3]<<8 | retVal[2], retVal[1],retVal[0]);
	 #endif
	 
	  *(multiCopy[arrayItem].paraVariable+offset+0) = hexToBcd(retVal[1]);
	  *(multiCopy[arrayItem].paraVariable+offset+1) = hexToBcd(retVal[2]);
	  tmpTail+=4;
 	}
	else if (2==multiCopy[arrayItem].copyItem)
	{
	  //�е�ǰ��������	  
		multiCopy[arrayItem].hasData |= HAS_CURRENT_ENERGY;

	  //�����й�����,��λ0.01kWh
	  offset = POSITIVE_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
		
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����й�����=%x\n", __func__, tmpData);
	 #endif
	 
		*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
		//�����й�����,��λ0.01kWh
		offset = NEGTIVE_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����й�����=%x\n", __func__, tmpData);
	 #endif 
		*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;

    //�й����ܾ�ֵ,�й������ܺ�
		tmpTail += 8;
		
	  //�����޹�����,��λ0.01kvarh
	  offset = POSITIVE_NO_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����޹�����=%x\n", __func__, tmpData);
	 #endif
	  *(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
	  *(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
	  *(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
	  *(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
	  
		//������  ������,��λ0.01kvarh
		offset = NEGTIVE_NO_WORK_OFFSET; 
		tmpData = hexToBcd(bmToYm(&data[tmpTail], 4));
   #ifdef SHOW_DEBUG_INFO
		printf("%s==>�����޹�����=%x\n", __func__, tmpData);
	 #endif
		*(multiCopy[arrayItem].energy+offset+0) = tmpData&0xff;
		*(multiCopy[arrayItem].energy+offset+1) = tmpData>>8&0xff;
		*(multiCopy[arrayItem].energy+offset+2) = tmpData>>16&0xff;
		*(multiCopy[arrayItem].energy+offset+3) = tmpData>>24&0xff;
	  tmpTail += 4;
  }

  return METER_REPLY_ANALYSE_OK;  //��������֡�����ҽ�����ȷ,�ѱ��������
}


#endif


/***************************************************
��������:processWasionData
��������:��ʢ��Լ�������
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬pDataEnd���ݶν���ָ��,
         measurePoint
�������:
����ֵ��
***************************************************/
void processWasionData(INT8U measureObject, INT8U *pFrameHead)
{
    /*INT8U type, i;
    INT8U loadLength; 
    REAL_DATA_WITH_TARIFF  data;
    
    //type = *(pFrameHead+3) &
    loadLength = *(pFrameHead + 4);
    
    data.measureObject = measureObject;
    
    //��ȡ�ն�ʱ��洢��data.time��
    data.time.second = copyTime.second/10<<4 | copyTime.second%10;
    data.time.minute = copyTime.minute/10<<4 | copyTime.minute%10;
    data.time.hour   = copyTime.hour  /10<<4 | copyTime.hour  %10;
    data.time.day    = copyTime.day   /10<<4 | copyTime.day   %10;
    data.time.month  = copyTime.month /10<<4 | copyTime.month %10;
    data.time.year   = copyTime.year  /10<<4 | copyTime.year  %10;
    
    data.type = findWasionType(*(pFrameHead+2), *(pFrameHead+3));
    
    //δ���
    for (i = 0; i < 15; i++)
    {
    	data.data[0] = 0x00000000;
    }*/
}

/***************************************************
��������:processWasionPacket
��������:��ʢ��Լ���ݰ�����
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬pDataEnd���ݶν���ָ��,
         measurePoint
�������:
����ֵ��
***************************************************/
void processWasionPacket(INT8U measureObject, INT8U *pFrameHead)
{
}

/***************************************************
��������:processWasionTotal
��������:��ʢ��Լ���ݰ�����
���ú���:
�����ú���:
�������:pDataHead���ݶ���ʼָ�룬pDataEnd���ݶν���ָ��,
         measurePoint
�������:
����ֵ��
***************************************************/
void processWasionTotal(INT8U measureObject, INT8U *pFrameHead)
{
}

/***************************************************
��������:findParamType
��������:���ݵ��������DI��ȷ�������������
���ú���:
�����ú���:
�������:di
�������:
����ֵ���������
***************************************************/
INT16U findDataOffset(INT8U protocol, INT8U *di)
{
    INT16U    ret=0x200;
    INT8U     i;
    char      tmpChrDi[100];
    
    switch (protocol)
    {
      #ifdef PROTOCOL_645_1997
        case DLT_645_1997:        //DL/T645-1997
      #endif
      #ifdef PROTOCOL_PN_WORK_NOWORK_97
        case PN_WORK_NOWORK_1997: //DL/T645-1997ֻ���������޹�����ʾֵ
      #endif
      
      #if PROTOCOL_645_1997
        if ((*(di+1)==0x94) || *(di+1)==0x95 || *(di+1)==0xa4 || *(di+1)==0xa5 || *(di+1)==0xb4 || *(di+1)==0xb5)  //��������
       	{
      	  //������������������ʱ��
      	  for (i = 0; i < TOTAL_CMD_LASTMONTH_645_1997; i++)
      	  {
      	    if (cmdDlt645LastMonth1997[i][0] == *(di+1) && (cmdDlt645LastMonth1997[i][1] == *di))
      	    {
      	      ret = (cmdDlt645LastMonth1997[i][2]) | ((cmdDlt645LastMonth1997[i][3])<<8);
      	      break;
      	    }
      	  }
      	}
      	else      		//��ǰ����
      	{
          for (i = 0; i < TOTAL_CMD_CURRENT_645_1997; i++)
        	{
        	  if (cmdDlt645Current1997[i][0] == *(di+1) && (cmdDlt645Current1997[i][1] == *di))
            {
              ret = (cmdDlt645Current1997[i][2]) | ((cmdDlt645Current1997[i][3])<<8);
              break;
            }
      	  }
      	}
      	break;
      #endif  //PROTOCOL_645_1997
      
      #ifdef PROTOCOL_SINGLE_PHASE_97
        case SINGLE_PHASE_645_1997:  //����645��
      #endif
      #if PROTOCOL_SINGLE_PHASE_97
        if (*(di+1)==0x9a)   //��һ���ն�������
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_SINGLE_97; i++)
          {
        	  if (single1997LastDay[i][0] == *(di+1) && (single1997LastDay[i][1] == *di))
            {
              ret = (single1997LastDay[i][2]) | ((single1997LastDay[i][3])<<8);
              break;
            }
          }
        }
        else
        {
          for (i = 0; i < TOTAL_CMD_SINGLE_645_97; i++)
          {
        	  if (single1997[i][0] == *(di+1) && (single1997[i][1] == *di))
            {
              ret = (single1997[i][2]) | ((single1997[i][3])<<8);
              break;
            }
          }
      	}
      	break;
      #endif  //PROTOCOL_SINGLE_PHASE_97
            	
      #ifdef PROTOCOL_645_2007
        case DLT_645_2007:  //DL/T645-2007
      #endif
      #ifdef PROTOCOL_PN_WORK_NOWORK_07
        case PN_WORK_NOWORK_2007: //DL/T645-2007ֻ���������޹�����ʾֵ
      #endif
      #if PROTOCOL_645_2007
          if (((*(di+3)==0x00) || (*(di+3)==0x01)) && ((*(di+1)==0xff) || (*(di+1)==0x00)) && (*di==0x1))
          {
            for (i = 0; i < TOTAL_CMD_LASTMONTH_645_2007; i++)
        	  {
        	    if (cmdDlt645LastMonth2007[i][0] == *(di+3)
        	  	   && (cmdDlt645LastMonth2007[i][1]==*(di+2))
        	  	    && (cmdDlt645LastMonth2007[i][2]==*(di+1))
        	  	     && (cmdDlt645LastMonth2007[i][3]==*di)
        	  	   )
              {
                ret = (cmdDlt645LastMonth2007[i][4]) | ((cmdDlt645LastMonth2007[i][5])<<8);
                break;
              }
            }
          }
          else
          {
            if ((*(di+3)==0x05) && (*(di+2)==0x06) && (*di==0x1))
            {
              for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
          	  {
          	    if (cmdDlt645LastDay2007[i][0] == *(di+3)
          	  	   && (cmdDlt645LastDay2007[i][1]==*(di+2))
          	  	    && (cmdDlt645LastDay2007[i][2]==*(di+1))
          	  	     && (cmdDlt645LastDay2007[i][3]==*di)
          	  	   )
                {
                  ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
                  break;
                }
              }
            }
            else
            {          	
              for (i = 0; i < TOTAL_CMD_CURRENT_645_2007; i++)
          	  {
          	    if (cmdDlt645Current2007[i][0] == *(di+3)
          	  	   && (cmdDlt645Current2007[i][1]==*(di+2))
          	  	    && (cmdDlt645Current2007[i][2]==*(di+1))
          	  	     && (cmdDlt645Current2007[i][3]==*di)
          	  	   )
                {
                  ret = (cmdDlt645Current2007[i][4]) | ((cmdDlt645Current2007[i][5])<<8);
                  break;
                }
        	  }
        	}
      	  }      	
      	break;
      #endif
      
      #ifdef PROTOCOL_SINGLE_PHASE_07
        case SINGLE_PHASE_645_2007:         //DL/T645-2007�������ܱ�
        case SINGLE_PHASE_645_2007_TARIFF:
        case SINGLE_PHASE_645_2007_TOTAL:
      #endif      
      #ifdef PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
        case SINGLE_LOCAL_CHARGE_CTRL_2007: //DL/T645-2007���౾�طѿر�
      #endif
      #ifdef PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
        case SINGLE_REMOTE_CHARGE_CTRL_2007://DL/T645-2007����Զ�̷ѿر�
      #endif
      #if PROTOCOL_SINGLE_PHASE_07 || PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
        if ((*(di+3)==0x05) &&  (*(di+2)==0x06))  //��һ���ն�������
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_SINGLE_07; i++)
    	    {
    	      if (single2007LastDay[i][0] == *(di+3)
    	  	     && (single2007LastDay[i][1]==*(di+2))
    	  	      && (single2007LastDay[i][2]==*(di+1))
    	  	       && (single2007LastDay[i][3]==*di)
    	  	     )
            {
              ret = (single2007LastDay[i][4]) | ((single2007LastDay[i][5])<<8);
              break;
            }
          }
        }
        else
        {
          for (i = 0; i < TOTAL_CMD_SINGLE_LOCAL_CTRL_07; i++)
    	    {
    	      if (single2007[i][0] == *(di+3)
    	  	     && (single2007[i][1]==*(di+2))
    	  	      && (single2007[i][2]==*(di+1))
    	  	       && (single2007[i][3]==*di)
    	  	     )
            {
              ret = (single2007[i][4]) | ((single2007[i][5])<<8);
              break;
            }
          }
  	    }
      	break;
      #endif
      
      #ifdef PROTOCOL_THREE_2007
        case THREE_2007:                   //DL/T645-2007�������ܱ�
      #endif
      #ifdef PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
        case THREE_LOCAL_CHARGE_CTRL_2007: //DL/T645-2007���౾�طѿر�
      #endif
      #ifdef PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
        case THREE_REMOTE_CHARGE_CTRL_2007://DL/T645-2007����Զ�̷ѿر�
      #endif
      #if PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007 || PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007 || PROTOCOL_THREE_2007
        if ((*(di+3)==0x05) &&  (*(di+2)==0x06))  //��һ���ն�������
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
    	    {
    	      if (cmdDlt645LastDay2007[i][0] == *(di+3)
    	  	     && (cmdDlt645LastDay2007[i][1]==*(di+2))
    	  	      && (cmdDlt645LastDay2007[i][2]==*(di+1))
    	  	       && (cmdDlt645LastDay2007[i][3]==*di)
    	  	     )
            {
              ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
              break;
            }
          }
        }
        else
        {
          for (i = 0; i < TOTAL_CMD_THREE_LOCAL_CTRL_07; i++)
    	    {
    	      if (three2007[i][0] == *(di+3)
    	  	     && (three2007[i][1]==*(di+2))
    	  	      && (three2007[i][2]==*(di+1))
    	  	       && (three2007[i][3]==*di)
    	  	     )
            {
              ret = (three2007[i][4]) | ((three2007[i][5])<<8);
              break;
            }
          }
  	    }
      	break;
      #endif

      #ifdef PROTOCOL_KEY_HOUSEHOLD_2007
       case KEY_HOUSEHOLD_2007:                   //DL/T645-2007�ص��û�
      #endif
      #if PROTOCOL_KEY_HOUSEHOLD_2007
        if ((*(di+3)==0x05) &&  (*(di+2)==0x06))  //��һ���ն�������
        {
          for (i = 0; i < TOTAL_CMD_LASTDAY_645_2007; i++)
    	    {
    	      if (cmdDlt645LastDay2007[i][0] == *(di+3)
    	  	     && (cmdDlt645LastDay2007[i][1]==*(di+2))
    	  	      && (cmdDlt645LastDay2007[i][2]==*(di+1))
    	  	       && (cmdDlt645LastDay2007[i][3]==*di)
    	  	     )
            {
              ret = (cmdDlt645LastDay2007[i][4]) | ((cmdDlt645LastDay2007[i][5])<<8);
              break;
            }
          }
        }
        else
        {
        	for (i = 0; i < TOTAL_CMD_KEY_2007; i++)
 	        {
     	       if (keyHousehold2007[i][0] == *(di+3)
     	  	      && (keyHousehold2007[i][1]==*(di+2))
     	  	       && (keyHousehold2007[i][2]==*(di+1))
     	  	        && (keyHousehold2007[i][3]==*di)
     	  	      )
              {
                ret = (keyHousehold2007[i][4]) | ((keyHousehold2007[i][5])<<8);
                break;
              }
          }
        }
        break;
      #endif
      
      case DOT_COPY_1997:                   //DL/T645-1997�㳭��Լ
        for (i = 0; i < 3; i++)
      	{
      	  if (dotCopy1997[i][0] == *(di+1) && (dotCopy1997[i][1] == *di))
          {
            ret = (dotCopy1997[i][2]) | ((dotCopy1997[i][3])<<8);
            break;
          }
    	  }
        break;

      case DOT_COPY_2007:                   //DL/T645-2007�㳭��Լ
      	for (i = 0; i < 3; i++)
        {
   	       if (dotCopy2007[i][0] == *(di+3)
   	  	      && (dotCopy2007[i][1]==*(di+2))
   	  	       && (dotCopy2007[i][2]==*(di+1))
   	  	        && (dotCopy2007[i][3]==*di)
   	  	      )
            {
              ret = (dotCopy2007[i][4]) | ((dotCopy2007[i][5])<<8);
              break;
            }
        }
        break;

      case HOUR_FREEZE_2007:                //07�������ݹ�Լ
      	for (i = 0; i < 24; i++)
        {
   	       if (hourFreeze2007[i][0] == *(di+3)
   	  	      && (hourFreeze2007[i][1]==*(di+2))
   	  	       && (hourFreeze2007[i][2]==*(di+1))
   	  	        && (hourFreeze2007[i][3]==*di)
   	  	      )
            {
              ret = (hourFreeze2007[i][4]) | ((hourFreeze2007[i][5])<<8);
              break;
            }
        }
        break;
        
     #ifdef PROTOCOL_EDMI_GROUP
      case EDMI_METER:
      	if (
      		  (((*(di+1)&0x20) && (*(di+1)<0x30)) || ((*(di+1)&0x40) && (*(di+1)<0x50)))
      		  && (*di!=0xe0)
      		 )
      	{
        	for (i=0; i<TOTAL_COMMAND_LASTMONTH_EDMI; i++)
          {
     	      if ((cmdEdmiLastMonth[i][1]==*(di+1))
     	  	        && (cmdEdmiLastMonth[i][0]==*di)
     	  	     )
            {
              ret = (cmdEdmiLastMonth[i][2]) | ((cmdEdmiLastMonth[i][3])<<8);
              break;
            }
          }
      	}
      	else
      	{
        	for (i=0; i<TOTAL_COMMAND_REAL_EDMI; i++)
          {
     	      if ((cmdEdmi[i][1]==*(di+1))
     	  	        && (cmdEdmi[i][0]==*di)
     	  	     )
            {
              ret = (cmdEdmi[i][2]) | ((cmdEdmi[i][3])<<8);
              break;
            }
          }
        }
      	break;
     #endif
     
     #ifdef LANDIS_GRY_ZD_PROTOCOL
      case SIMENS_ZD_METER:
      	strcpy(tmpChrDi, (char *)di);
      	for(i=0; i<strlen(tmpChrDi); i++)
        {
        	if (tmpChrDi[i]=='(')
        	{
        		break;
          }
        }
        tmpChrDi[i] = '\0';
        
        for (i=0; i<TOTAL_DATA_ITEM_ZD; i++)
        {
      	  if (strcmp(landisChar[i],tmpChrDi)==0)
      	  {
      	  	ret = i;
      	  	break;
      	  }
      	}
      	break;
     #endif
      
      default:
      	ret = 0x200;   //�����
      	break;
    }

    return ret;
}
