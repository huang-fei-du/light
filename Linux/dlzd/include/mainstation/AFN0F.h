/*************************************************
Copyright,2006-2007,LongTong co.,LTD
�ļ�����AFN0F.h
���ߣ�leiyong
�汾��0.9
�������:2007��1��
��������վ"�ļ�����(AFN0F)"ͷ�ļ�
�޸���ʷ��
  01,07-01-18,leiyong created.
  02,10-03-31,Leiyong,��ֲ��Linux2.6.20
**************************************************/
#ifndef __AFN0FH
#define __AFN0FH

#include <stdio.h>

#include "common.h"

#define SIZE_OF_UPGRADE    512   //Զ������ÿƬ�δ�С

extern FILE *fpOfUpgrade;        //�����ļ�ָ��

typedef struct
{
	INT8U  flag;                   //������־(=1,������,2=�������)
  INT16U counter;                //���յ��ĳ���Ƭ�θ���
  INT16U perFrameSize;           //ÿ����Ƭ�δ�С  
}UPGRADE_FLAG;


//��������
void AFN0F(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);
void requestPacket(INT16U num);

#endif  /*__AFN0CH*/