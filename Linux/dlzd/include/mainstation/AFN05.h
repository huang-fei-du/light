/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����AFN05.h
���ߣ�leiyong
�汾��0.9
�������:2006��6�� ��
��������վ����������(AFN05)��ͷ�ļ�
�޸���ʷ��
  01,06-6-26,leiyong created.
**************************************************/
#ifndef __AFN05H
#define __AFN05H

#include "common.h"

extern INT8U  ctrlCmdWaitTime;              //�յ�����������ʾͣ��ʱ��


void AFN05(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);

BOOL AFN05001(INT8U *pHandle, INT8U fn);
BOOL AFN05002(INT8U *pHandle, INT8U fn);

BOOL AFN05009(INT8U *pHandle, INT8U fn);
BOOL AFN05010(INT8U *pHandle, INT8U fn);
BOOL AFN05011(INT8U *pHandle, INT8U fn);
BOOL AFN05012(INT8U *pHandle, INT8U fn);
BOOL AFN05015(INT8U *pHandle, INT8U fn);
BOOL AFN05016(INT8U *pHandle, INT8U fn);

BOOL AFN05017(INT8U *pHandle, INT8U fn);
BOOL AFN05018(INT8U *pHandle, INT8U fn);
BOOL AFN05019(INT8U *pHandle, INT8U fn);
BOOL AFN05020(INT8U *pHandle, INT8U fn);
BOOL AFN05023(INT8U *pHandle, INT8U fn);
BOOL AFN05024(INT8U *pHandle, INT8U fn);
BOOL AFN05025(INT8U *pHandle, INT8U fn);
BOOL AFN05026(INT8U *pHandle, INT8U fn);
BOOL AFN05027(INT8U *pHandle, INT8U fn);
BOOL AFN05028(INT8U *pHandle, INT8U fn);
BOOL AFN05029(INT8U *pHandle, INT8U fn);
BOOL AFN05030(INT8U *pHandle, INT8U fn);    //�ն�Ͷ��(�����Լ)

BOOL AFN05031(INT8U *pHandle, INT8U fn);
BOOL AFN05032(INT8U *pHandle, INT8U fn);

BOOL AFN05033(INT8U *pHandle, INT8U fn);
BOOL AFN05034(INT8U *pHandle, INT8U fn);
BOOL AFN05035(INT8U *pHandle, INT8U fn);
BOOL AFN05036(INT8U *pHandle, INT8U fn);
BOOL AFN05037(INT8U *pHandle, INT8U fn);

BOOL AFN05040(INT8U *pHandle, INT8U fn);    //�ն��˳�����(�����Լ)

BOOL AFN05049(INT8U *pHandle, INT8U fn);
BOOL AFN05050(INT8U *pHandle, INT8U fn);
BOOL AFN05051(INT8U *pHandle, INT8U fn);
BOOL AFN05052(INT8U *pHandle, INT8U fn);
BOOL AFN05053(INT8U *pHandle, INT8U fn);
BOOL AFN05054(INT8U *pHandle, INT8U fn);    //���������ѱ�

BOOL AFN05066(INT8U *pHandle, INT8U fn);

#endif //__AFN05H