/*************************************************
Copyright,2010,Huawei WoDian co.,LTD
�ļ�����AFN0A.c
���ߣ�wan guihua
�汾��0.9
������ڣ�2010��1��
��������վAFN0A(��ѯ����)�����ļ���
�����б�
  01,10-1-13,wan guihua created.
**************************************************/

#ifndef __INCAfn0AH
#define __INCAfn0AH

#include "common.h"

//��������
void AFN0A(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);

//��1
INT16U AFN0A001(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A002(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A003(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A004(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A005(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A006(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A007(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A008(INT16U frameTail,INT8U *pHandle);

//��2
INT16U AFN0A009(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A010(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A011(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A012(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A013(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A014(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A015(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A016(INT16U frameTail,INT8U *pHandle);

//��3
INT16U AFN0A017(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A018(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A019(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A020(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A021(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A022(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A023(INT16U frameTail,INT8U *pHandle);

//��4
INT16U AFN0A025(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A026(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A027(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A028(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A029(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A030(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A031(INT16U frameTail,INT8U *pHandle);

//��5
INT16U AFN0A033(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A034(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A035(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A036(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A037(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A038(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A039(INT16U frameTail,INT8U *pHandle);

//��6
INT16U AFN0A041(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A042(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A043(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A044(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A045(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A046(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A047(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A048(INT16U frameTail,INT8U *pHandle);

//��7
INT16U AFN0A049(INT16U frameTail,INT8U *pHandle);

#ifdef LIGHTING
 INT16U AFN0A050(INT16U frameTail,INT8U *pHandle);
 INT16U AFN0A051(INT16U frameTail,INT8U *pHandle);
 INT16U AFN0A052(INT16U frameTail,INT8U *pHandle);
#endif

//��8
INT16U AFN0A057(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A058(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A059(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A060(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A061(INT16U frameTail,INT8U *pHandle);

//��9
INT16U AFN0A065(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A066(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A067(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A068(INT16U frameTail,INT8U *pHandle);

//��10
INT16U AFN0A073(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A074(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A075(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A076(INT16U frameTail,INT8U *pHandle);

//��11
INT16U AFN0A081(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A082(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A083(INT16U frameTail,INT8U *pHandle);

//��Լ��չ
#ifdef SDDL_CSM
 INT16U AFN0A088(INT16U frameTail,INT8U *pHandle);
 INT16U AFN0A224(INT16U frameTail,INT8U *pHandle); 
#endif
INT16U AFN0A097(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A098(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A099(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A100(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A121(INT16U frameTail,INT8U *pHandle);  //�ն˵�ַ������������
INT16U AFN0A133(INT16U frameTail,INT8U *pHandle);  //�ز�/�������ڵ��ַ
INT16U AFN0A134(INT16U frameTail,INT8U *pHandle);  //�豸���
INT16U AFN0A135(INT16U frameTail,INT8U *pHandle);  //���ģ�����
INT16U AFN0A136(INT16U frameTail,INT8U *pHandle);  //������Ϣ
INT16U AFN0A138(INT16U frameTail,INT8U *pHandle);  //������Ϣ

#endif/*__INCAfn0AH*/
