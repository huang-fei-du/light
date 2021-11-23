/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN10.c
���ߣ�leiyong
�汾��0.9
������ڣ�2007��3��
��������վAFN10(����ת��)�����ļ���
�����б�
     1.
�޸���ʷ��
  01,07-3-22,Tianye created.
  02,07-11-17,leiyong modify,��ԭ����Ҫ�鿴�Ƿ����ò��������ת����Ϊֱ��ת����ȥ
  03,10-04-01,leiyong��ֲ��AT91SAM9260
***************************************************/
#include "common.h"
#include "teRunPara.h"
#include "copyMeter.h"
#include "AFN10.h"

/*******************************************************
��������:AFN10
��������:��վ"����ת������"(AFN0C)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN10(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom)
{
	 INT8U    fn, pn;
	 INT8U    *pData;
	 INT16U   frameTail;
	 INT8U    tmpPort; 
    
	 pData = pDataHead;
	  
	 pn = findFnPn(*pData, *(pData+1), FIND_PN);
	 fn = findFnPn(*(pData+2), *(pData+3), FIND_FN);
	  
	 //ֻ���ܺ�P0
	 if (pn != 0)
	 {
	   return;
	 }
	 pData+=4;
    
   //�˿�ֻ����1,2,3��31
   //�˿�ֻ����1,2,3,4��31,2012-03-28�ĳɽӰ�1 2 3 4��
   tmpPort = *pData++;
   if ((tmpPort>0 && tmpPort<5) || tmpPort==PORT_POWER_CARRIER)
   {
     if (tmpPort==PORT_POWER_CARRIER)
     {
   	   tmpPort = 4;
   	   
   	   //2015-12-05,�ĳ�ֻ���ز�������ж�
      #ifdef LIGHTING
       if (0<carrierFlagSet.broadCast)
       {
         ackOrNack(FALSE,dataFrom);    //ȫ������
         
         return;
       }
      #endif
     }
     else
     { 
   	   tmpPort--;
     }
     

     printf("%02d-%02d-%02d %02d:%02d:%02d,AFN10,ת���˿�%d,Fn=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, tmpPort, fn);
     
     //����ö˿ڵ�ǰδ����ת��
     if (copyCtrl[tmpPort].pForwardData==NULL)
     {
   	  switch(fn)
   	  {
   	 	  case 1:    //͸��ת��
       	  copyCtrl[tmpPort].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
          copyCtrl[tmpPort].pForwardData->fn            = 1;
          copyCtrl[tmpPort].pForwardData->dataFrom      = dataFrom;
       	  copyCtrl[tmpPort].pForwardData->ifSend        = FALSE;
       	  copyCtrl[tmpPort].pForwardData->receivedBytes = FALSE;
       	  copyCtrl[tmpPort].pForwardData->forwardResult = RESULT_NONE;
       
       	  //͸��ת��ͨ�ſ�����
       	  copyCtrl[tmpPort].pForwardData->ctrlWord = *pData++;
       	  
       	  //͸��ת�����յȴ����ĳ�ʱʱ��
       	  if ((*pData&0x80)==0)   //��λΪms
       	  {
       	  	 copyCtrl[tmpPort].pForwardData->frameTimeOut = (*pData&0x7f)/100;
       	  	 if (copyCtrl[tmpPort].pForwardData->frameTimeOut <1)
       	  	 {
       	  	 	 copyCtrl[tmpPort].pForwardData->frameTimeOut = 2;
       	  	 }
       	  }
       	  else                    //��λΪs
       	  {
       	  	copyCtrl[tmpPort].pForwardData->frameTimeOut = *pData&0x7f;
       	  }       	  
       	  pData++;
       	  
       	  //͸��ת�����յȴ��ֽڳ�ʱʱ��
       	  copyCtrl[tmpPort].pForwardData->byteTimeOut = *pData++;
       	  
       	  copyCtrl[tmpPort].pForwardData->length = *pData | *(pData+1)<<8;
       	  pData+=2;
       	  
       	  //�ز�/���߶˿�,ȥ��0xFE,���򱾵�ͨ��ģ����޷������,2012-08-20
       	  if (tmpPort==4)
       	  {
       	    while(copyCtrl[tmpPort].pForwardData->length>0)
       	    {
       	      if (*pData==0xfe)
       	      {
       	    	  pData++;
       	    	  copyCtrl[tmpPort].pForwardData->length--;
       	      }
       	      else
       	      {
       	    	  break;
       	      }
       	    }
       	  }
       	  
       	  //͸��ת������
       	  memcpy(copyCtrl[tmpPort].pForwardData->data,pData,copyCtrl[tmpPort].pForwardData->length);
       	  
       	 #ifdef LIGHTING 
       	  if (tmpPort==4)
       	  {
       	    if (0x99==copyCtrl[tmpPort].pForwardData->data[1]
       	    	 || 0x99==copyCtrl[tmpPort].pForwardData->data[2]
       	    	  || 0x99==copyCtrl[tmpPort].pForwardData->data[3]
       	    	   || 0x99==copyCtrl[tmpPort].pForwardData->data[4]
       	    	    || 0x99==copyCtrl[tmpPort].pForwardData->data[5]
       	    	     || 0x99==copyCtrl[tmpPort].pForwardData->data[6]
       	    	 )
       	    {
       	      carrierFlagSet.broadCast = 2;
       	    }
       	  }
       	 #endif
   	 	    break;
   	 	  	
   	 	  case 9:    //ת����վֱ�ӶԵ��ܱ�ĳ�����������
       	  copyCtrl[tmpPort].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
          copyCtrl[tmpPort].pForwardData->fn       = 9;
          copyCtrl[tmpPort].pForwardData->dataFrom = dataFrom;
       	  copyCtrl[tmpPort].pForwardData->ifSend   = FALSE;
       
       	  //͸��ת��ͨ�ſ�����(δָ�����ն�ԭ�趨���˿ڵĿ�����)
       	  copyCtrl[tmpPort].pForwardData->ctrlWord = 0x0;
       	  
       	  //͸��ת�����յȴ����ĳ�ʱʱ��s(δָ��)
       	  copyCtrl[tmpPort].pForwardData->frameTimeOut = 30;

       	  //͸��ת�����յȴ��ֽڳ�ʱʱ��(δָ��)
       	  copyCtrl[tmpPort].pForwardData->byteTimeOut = 0x0;
       	  
       	  //δָ��
       	  copyCtrl[tmpPort].pForwardData->length = 0x0;
       	  copyCtrl[tmpPort].pForwardData->forwardResult = RESULT_NONE;

       	  //ת��Ŀ���ַ,��ʶ����,���ݱ�ʶ
       	  if (*pData==0xff)    //��ָ���м�·��
       	  {
       	     memcpy(copyCtrl[tmpPort].pForwardData->data,pData+1,11);
       	  }
       	  else
       	  {
       	     memcpy(copyCtrl[tmpPort].pForwardData->data,pData+((*pData*6)+1),11);
       	  }

   	 	    break;
   	 	  
   	 	  case 10:   //ת����վֱ�ӶԵ��ܱ��ң����բ/�����բ����
   	 	    break;
   	 	  	
   	 	  case 11:   //ת����վֱ�ӶԵ��ܱ��ң���͵�����
   	 	    break;
   	 	}
     }    
   }
}