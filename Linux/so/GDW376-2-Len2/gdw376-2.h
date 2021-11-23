/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
�ļ���:gdw376-2.h
���ߣ�leiyong
�汾��0.9
�������:2010��03��10��
������Q/GDW376-2������������ͨ��ģ��(�ز�ģ��)
      ���ؽӿ�ͨ��Э�鶯̬���ӿ�ͷ�ļ�(�û��ӿ�)
      
�޸���ʷ��
  01,10-03-09,leiyong created.
**************************************************/

#ifndef __GDW_376_2_H
#define __GDW_376_2_H

#include "common.h"

//�ز�ģ������
#define NO_CARRIER_MODULE          0x0     //���ز�ģ�� 
#define EAST_SOFT_CARRIER          0x1     //�����ز�ģ��
#define CEPRI_CARRIER              0x2     //��������ģ��
#define HWWD_WIRELESS              0x3     //��ΰ�ֵ�����ģ��
#define SR_WIRELESS                0x4     //SR����ģ��
#define RL_WIRELESS                0x5     //�������ģ��
#define MIA_CARRIER                0x6     //����΢�ز�ģ��
#define TC_CARRIER                 0x7     //�����ز�ģ��
#define LME_CARRIER                0x8     //����΢�ز�ģ��
#define FC_WIRELESS                0x9     //��Ѹ������ģ��
#define SC_WIRELESS                0xa     //��������ģ��
#define CEPRI_CARRIER_3_CHIP       0xb     //���Ժģ��(�°汾3оƬ)
#define SR_WF_3E68                 0xc     //ɣ��SRWF-3E68ģ��

//GDW376.2֡�����ַ�
#define GDW376_2_SOP              0x68     //��ʼ�ַ�
#define GDW376_2_EOP              0x16     //�����ַ�

//ͨ�ŷ�ʽ
#define CENTRE_ROUTE                 1     //����ʽ·���ز�ͨ��
#define DISTRIBUTED_ROUTE            2     //�ֲ�ʽ·���ز�ͨ��
#define EAST_SOFT_ROUTER             7     //������չ·��Э��
#define MICRO_POWER_WIRELESS        10     //΢��������ͨ��

//Ӧ�ò㹦�ܴ���
#define ACK_OR_NACK_3762          0x00     //ȷ��/����
#define INITIALIZE_3762           0x01     //��ʼ��
#define DATA_FORWARD_3762         0x02     //����ת��
#define QUERY_DATA_3762           0x03     //��ѯ����
#define LINK_DETECT_3762          0x04     //��·�ӿڼ��
#define CTRL_CMD_3762             0x05     //��������
#define ACTIVE_REPORT_3762        0x06     //�����ϱ�
#define ROUTE_QUERY_3762          0x10     //·�ɲ�ѯ
#define ROUTE_SET_3762            0x11     //·������
#define ROUTE_CTRL_3762           0x12     //·�ɿ���
#define ROUTE_DATA_FORWARD_3762   0x13     //·������ת��
#define ROUTE_DATA_READ_3762      0x14     //·�����ݳ���
#define RL_EXTEND_3762            0x15     //���/SR��չ����
#define DEBUG_3762                0xf0     //�ڲ�����
#define FC_QUERY_DATA_3762        0x07     //��Ѹ����չ��ѯ����
#define FC_NET_CMD_3762           0x09     //��Ѹ����չ����ָ��

//������չAFN
#define ES_CTRL_CMD_3762          0x01     //������չ��������
#define ES_DATA_QUERY_3762        0x02     //������չ���ݲ�ѯ

//����ֵ
#define RECV_DATA_CORRECT            1     //���յ���ȷ��һ֡
#define RECV_DATA_UNKNOWN           -1     //���յ�����ʶ������
#define RECV_NOT_COMPLETE_3762      -2     //�������ݲ�����,�����ȴ�����
#define RECV_CHECKSUM_ERROR_3762    -3     //��������У��ʹ���
#define RECV_TAIL_ERROR_3762        -4     //��������֡β����,����

//�ṹ - GDW376.2����֡����
typedef struct
{
	INT8U  sop;        //��ʼ�ַ�SOP
	INT16U l;          //����L
	INT8U  c;          //������C
	INT8U  *pUserData; //�û�����ָ��
	INT8U  r[6];       //��Ϣ��R
  INT8U  a[12];      //��ַ��A
  INT8U  *afn;       //Ӧ�ò㹦����AFN
  INT8U  *fn;        //���ܴ���	
  INT8U  dt[2];      //���ݵ�Ԫ��ʶ
  INT8U  *pLoadData; //��������ʼָ��
	INT8U  cs;         //У���CS
	INT8U  eop;        //�����ַ�EOP
}GDW376_2_FRAME_ANALYSE;

//��ʼ���ṹ
typedef struct
{	 
	 INT8U *afn;                                        //AFNָ��
	 INT8U *fn;                                         //FNָ��
	 INT8U *moduleType;                                 //ģ������ָ��
	 void (*send)(INT8U port,INT8U *pack,INT16U length);//��˿ڷ������ݺ���
}GDW376_2_INIT;

//��������ֵ

//��������
BOOL  initGdw3762So(GDW376_2_INIT *init);
INT8U gdw3762Framing(INT8U afn,INT8U fn,INT8U *address,INT8U *pData);
INT8S gdw3762Receiving(INT8U *data,INT8U *recvLen);

#endif  //__GDW_376_2_H
