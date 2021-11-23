/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����AFN04.h
���ߣ�leiyong
�汾��0.9
�������:2006��5�� ��
��������վ�����ò���(AFN04)��ͷ�ļ�
�޸���ʷ��
  01,06-05-28,leiyong created.
**************************************************/
#ifndef __INCreporttaskh
#define __INCreporttaskh

#include "timeUser.h"
#include "workWithMS.h"

//�ⲿ����
extern   DATE_TIME          sysTime;                 //ϵͳʱ��
extern   REPORT_TASK_PARA   reportTask1, reportTask2;//��ʱ����1�࣬2��������������(AFN04-FN65,FN66)
extern   REPORT_TIME        reportTime1;             //��ʱ����һ������ʱ���
extern   REPORT_TIME_2      reportTime2;             //��ʱ���Ͷ�������ʱ���
extern   BOOL               ipPermit;                //������IP��
extern   ANALYSE_FRAME      frame;
extern   BOOL               initTasking;      //���ڳ�ʼ�������־

//�ⲿ����
extern   void   AFN0C(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom,INT8U poll);
extern   void   AFN0D(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom,INT8U poll);

//��������
void initReportTask(void *arg);
void activeReport1(void);
void threadOfReport2(void *arg);
void activeReport3(void);

#endif //__INCreporttaskh
