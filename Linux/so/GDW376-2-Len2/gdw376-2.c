/***************************************************
Copyright,2010,HuaWei WoDian co.,LTD,All	Rights Reserved
�ļ���:gdw376-2.c
���ߣ�leiyong
�汾��0.9
������ڣ�2010��3��
����:Q/GDW376-2������������ͨ��ģ��(�ز�ģ��)���ؽӿ�ͨ��Э�鴦���ļ���

�����б�
     1.
�޸���ʷ��
  01,10-03-09,Leiyong created.
  02,10-09-19,Leiyong,�޸Ľ��մ���
  03,10-09-19,Leiyong,���֧������ģ��
  04,14-01-09,Leiyong,���Ԥ��Ӧ���ֽ�������

***************************************************/

#include "stdio.h"
#include "string.h"
#include "gdw376-2.h"

#define PORT_POWER_CARRIER          31               //�����ز��ӿ�

INT8U                  *carrierModule;               //�ز�ģ������
INT8U                  lcModuleTypex=0;              //��չ�ز�ģ������(ly,2012-02-28,Ϊ��ʶ���°���Ժģ�������,��Ϊ�°���ϰ�Ĵ�����ͬ����������������ɲ�ѯ)
INT8U                  mainNodeAddr[6];              //�ز�ģ�����ڵ��ַ
INT8U                  scMainNodeAddr[6];            //���������ڵ��ַ,����ֻ�ܶ������ò�������
GDW376_2_FRAME_ANALYSE recvFrame;                    //����֡����
INT8U                  gdw3762RecvBuf[512];          //���ջ���
INT16U                 recvFrameTail;                //����֡β

void (*send)(INT8U port,INT8U *pack,INT16U length);  //��˿ڷ������ݺ���

const INT8U expectBytes07[12][5] = {
	    {0x05, 0x06, 0x00, 0x01, 22},    //��һ�ն���ʱ��
	    {0x05, 0x06, 0x01, 0x01, 37},    //��һ���ն��������й���������
	    {0x05, 0x06, 0x02, 0x01, 37},    //��һ���ն��ᷴ���й���������
      
      {0x00, 0x01, 0xff, 0x00, 37},    //��ǰ�����й�����ʾֵ(�ܼ�������)
      {0x00, 0x02, 0xff, 0x00, 37},    //��ǰ�����й�����ʾֵ(�ܼ�������)
      {0x00, 0x01, 0x00, 0x00, 22},    //��ǰ�����й�����ʾֵ��, 2012-5-21,add
      {0x00, 0x02, 0x00, 0x00, 22},    //��ǰ�����й�����ʾֵ��, 2012-5-21,add
      {0x04, 0x00, 0x05, 0xFF, 33},    //�������״̬��1��7(7*2<>97ֻ��2bytes)
      {0x04, 0x00, 0x01, 0x01, 21},    //���ڼ��ܴ�(4�ֽ�=97)
      {0x04, 0x00, 0x01, 0x02, 20},    //���ʱ��(3�ֽ�=97)
      {0x1e, 0x00, 0x01, 0x01, 22},    //��һ����բ����ʱ��(6�ֽ�,97û��)
      {0x1d, 0x00, 0x01, 0x01, 22},    //��һ�κ�բ����ʱ��(6�ֽ�,97û��)
	   };

/*******************************************************
��������:initGdw3762So
��������:��ʼ��GDW376.2��
���ú���:
�����ú���:
�������:
�������:
����ֵ��TRUE-��ʼ���ɹ�
        FALSE-��ʼ��ʧ��
*******************************************************/
BOOL initGdw3762So(GDW376_2_INIT *init)
{
   carrierModule = init->moduleType;
   recvFrame.afn = init->afn;
   recvFrame.fn  = init->fn;
   send = init->send;

   return TRUE;
}

/*******************************************************
��������:expectBytes
��������:Ԥ��Ӧ���ֽ���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
INT8U expectBytes(INT8U *pData)
{
	INT8U i;
	
	if (0x11==*(pData+8))
	{
		for(i=0; i<12; i++)
	  {
	  	if ((expectBytes07[i][3]+0x33)==*(pData+10)
	  		  || (expectBytes07[i][2]+0x33)==*(pData+11)
	  		   || (expectBytes07[i][1]+0x33)==*(pData+12)
	  		    || (expectBytes07[i][0]+0x33)==*(pData+13)
	  		 )
	    {
	    	return expectBytes07[i][4];
	    }
	  }
	}
	
	return 35;    //Ĭ��Ԥ���ֽ���Ϊ35�ֽ�
}

/*******************************************************
��������:gdw3762Frameing
��������:���ҵ�����ҵ��׼Q/GDW376.2��֡����
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
INT8U gdw3762Framing(INT8U afn,INT8U fn,INT8U *address,INT8U *pData)
{
	 INT8U  sendBuf[255];
   INT8U  checkSum;
   INT16U i;
   INT16U frameTail;
   
   sendBuf[0]  = GDW376_2_SOP;    //��ʼ�ַ�
  
   //C
   switch(*carrierModule)
   {
   	 case HWWD_WIRELESS:
   	 case SR_WIRELESS:
   	 case RL_WIRELESS:
   	 case FC_WIRELESS:
   	 case SC_WIRELESS:
       sendBuf[3] = 0x4a;
       sendBuf[6] = 0x00;
       break;
       
     case MIA_CARRIER:
     	 if (afn==ROUTE_DATA_READ_3762 || afn==ACK_OR_NACK_3762)
     	 {
         sendBuf[3] = 0x01;
       }
       else
       {
         sendBuf[3] = 0x41;
       }
       sendBuf[6] = 0x00;
     	 break;
     
     default:
     	 if (afn==ROUTE_DATA_READ_3762 || afn==ACK_OR_NACK_3762)
     	 {
         sendBuf[3] = 0x01;
       }
       else
       {
         sendBuf[3] = 0x41;
       }
       sendBuf[6] = 20;    //2014-01-09,��0xff�ĳ�20
       break;
   }
    
   //R(6�ֽ�)
   sendBuf[5] = 0x00;
   sendBuf[7] = 0x00;
   sendBuf[8] = 0x00;
   sendBuf[9] = 0x00;

   frameTail = 10;
   switch(afn)
   {
     case 0x02:
     case 0x13:
       sendBuf[4] = 0x04;
     
       //���ڵ��ַ
     	 if (*carrierModule==SC_WIRELESS)
     	 {
     	   memcpy(&sendBuf[frameTail], scMainNodeAddr, 6);
     	 }
     	 else
     	 {
         memcpy(&sendBuf[frameTail], mainNodeAddr, 6);
       }
       frameTail += 6;

       //Ŀ�Ľڵ��ַ
       memcpy(&sendBuf[frameTail], address, 6);
       frameTail += 6;
       break;

     case 0x05:
     	 if (fn==3)
     	 {
         sendBuf[4] = 0x04;
     
         //���ڵ��ַ
     	   if (*carrierModule==SC_WIRELESS)
     	   {
     	     memcpy(&sendBuf[frameTail], scMainNodeAddr, 6);
     	   }
     	   else
     	   {
           memcpy(&sendBuf[frameTail], mainNodeAddr, 6);
         }
         frameTail += 6;

         //Ŀ�Ľڵ��ַ
         memcpy(&sendBuf[frameTail], address, 6);
         frameTail += 6;
       }
       break;
       
     default:
     	 if ((afn==0x14) && (*carrierModule==TC_CARRIER))
     	 {
         if (address==NULL)
         {
         	 ;
         }
         else
         {
           sendBuf[4] = 0x04;

           //��λ
           if (*pData==1)
           {
           	 sendBuf[5] = *(pData+3);
           }
           else
           {
             sendBuf[5] = *(pData+(*(pData+1)+3));
           }

           //���ڵ��ַ
           memcpy(&sendBuf[frameTail], mainNodeAddr, 6);
           frameTail += 6;
  
           //Ŀ�Ľڵ��ַ
           memcpy(&sendBuf[frameTail], address, 6);
           frameTail += 6;
         }
     	 }
     	 else
     	 {
         sendBuf[4]  = 0x00;
       }
       break;
   }
   
   sendBuf[frameTail++] = afn;
   
   sendBuf[frameTail++] = 0x01<<((fn%8 == 0) ? 7 : (fn%8-1));
   sendBuf[frameTail++] = (fn-1)/8;
      
   switch(afn)
   {
   	 case ACK_OR_NACK_3762:
   	 	 switch(fn)
   	 	 {
   	 	 	  case 1:            //ȷ��
   	 	 	  	sendBuf[5] |= *(pData+4);
   	 	 	  	memcpy(&sendBuf[frameTail], pData, 4);
   	 	 	  	frameTail += 4;
   	 	 	  	break;
   	 	 	  
   	 	 	  case 2:            //����
   	 	 	  	sendBuf[frameTail++] = *pData;
   	 	 	  	break;
   	 	 }
   	 	 break;
   	 	 
   	 case DATA_FORWARD_3762: //����ת��
   	 	 switch(fn)
   	 	 {
   	 	 	 case 1:
           //��Լ����
           sendBuf[frameTail++] = *pData;
           
           //�����ĳ��ȼ�����
           memcpy(&sendBuf[frameTail], pData+1, *(pData+1)+1);
           frameTail += (*(pData+1)+1);
   	 	 	 	 break;
   	 	 }
   	 	 break;
   	 	 
   	 case QUERY_DATA_3762:   //���ݲ�ѯ
   	   
   	 	 break;
   	 
   	 case CTRL_CMD_3762:     //��������
   	 	 switch(fn)
   	 	 {
   	 	 	 case 1:             //�������ڵ��ַ
   	   	   memcpy(&sendBuf[frameTail], address, 6);
  	   	   frameTail+=6;
   	 	 	   break;
   	 	 	 
   	 	 	 case 3:             //�����㲥
   	 	 	 	 memcpy(&sendBuf[frameTail], pData+1, *pData);
   	 	 	 	 frameTail+=*pData;
   	 	 	 	 break;

   	 	 	 case 4:             //�����ŵ���(��Ѷ����չ)
   	   	   memcpy(&sendBuf[frameTail], pData, 7);
  	   	   frameTail+=7;
   	 	 	   break;
   	 	 	 
   	 	 	 case 31:            //��Ѹ������ģ������ʱ������
   	   	   memcpy(&sendBuf[frameTail], pData, 6);
  	   	   frameTail+=6;   	 	 	 	 
   	 	 	 	 break;
   	 	 }
   	 	 break;
   	 	 
   	 case ROUTE_QUERY_3762:  //·�ɲ�ѯ
   	 	 switch(fn)
   	 	 {
   	 	   case 2:             //�ز��ӽڵ���Ϣ
   	 	     sendBuf[frameTail++] = *pData;
   	 	     sendBuf[frameTail++] = *(pData+1);
           
           //ly,2010-12-29,�ӽڵ�����
           //1.���Ժ�ز�ģ��һ��ֻ�ܶ�һ��(����ָ���ӽڵ�����Ϊ10��Ҳֻ��һ��)
           //2.�����ز�ģ���ĵ�����ֻ�ܶ�һ��,��ʵ��һ�ζ�15������
           //3.RL����ģ��һ������10��
           //4.SR����ģ�鲻�ܶ������ֵ
           //5.����΢�ز�ģ��һ������ѯ15���ز��ӽڵ���Ϣ
           //�ۺϿ���,һ�η�10��
   	 	     switch(*carrierModule)
   	 	     {
   	 	     	 case EAST_SOFT_CARRIER:
   	 	     	 case RL_WIRELESS:
   	 	     	 case MIA_CARRIER:
   	 	         sendBuf[frameTail++] = 0xa;
   	 	         break;

   	 	     	 case TC_CARRIER:   //����֧��һ��ֱ�Ӷ�26��,���������20��һ��
   	 	         //2013-12-25,���ݶ������µ��������˵��"���һ֡��ѯ����Ϊ29ֻ��Ϊ��ֹ��������̫���������⣬��ȡ����ӦΪÿ֡9ֻ"
   	 	         //           �ĳ�ÿ֡9ֻ
   	 	         //sendBuf[frameTail++] = 20;
   	 	         sendBuf[frameTail++] = 9;
   	 	     	 	 break;
   	 	         
   	 	       default:   //Ĭ��Ϊ1��
   	 	       	 if (lcModuleTypex==CEPRI_CARRIER_3_CHIP)
   	 	       	 {
   	 	           sendBuf[frameTail++] = 10;
   	 	       	 }
   	 	       	 else
   	 	       	 {
   	 	           sendBuf[frameTail++] = 0x1;
   	 	         }
   	 	       	 break;
   	 	     }
   	 	     break;
   	 	     
   	 	   case 6:
   	 	   	 memcpy(&sendBuf[frameTail],pData,3);
   	 	   	 frameTail += 3;
   	 	   	 break;
   	 	 }
   	 	 break;
   	 	 
   	 case ROUTE_SET_3762:     //·������
   	   switch(fn)
   	   {
   	   	  case 1:             //��Ӵӽڵ�
   	 	      sendBuf[frameTail++] = 0x1;    //�ӽڵ�����
   	   	  	memcpy(&sendBuf[frameTail],address,6);
   	   	  	frameTail+=6;
   	 	      sendBuf[frameTail++] = *pData;
   	 	      sendBuf[frameTail++] = *(pData+1);
   	 	      sendBuf[frameTail++] = *(pData+2);  //�ӽڵ��Լ,��������֤��Լ�� 1-97,2-07
   	   	  	break;
   	   	  
   	   	  case 2:             //ɾ���ӽڵ�
   	   	  	sendBuf[frameTail++] = 0x1;    //�ӽڵ�����
   	   	  	memcpy(&sendBuf[frameTail], pData, 6);
   	   	  	frameTail+=6;
   	   	  	break;
   	   	  	
   	   	  case 3:
            if (*carrierModule==RL_WIRELESS)  //�������ģ�������������������,��������
            {
            	;
            }
   	   	  	break;
   	   	  	
   	   	  case 4:             //����·�ɹ���ģʽ
   	   	  	memcpy(&sendBuf[frameTail],pData,3);
   	   	  	frameTail+=3;   	   	  	
   	   	  	break;
   	   	  	
   	   	  case 5:             //�����ز��ӽڵ�����ע��
   	   	  	memcpy(&sendBuf[frameTail],pData,10);
   	   	  	frameTail += 10;
   	   	  	break;
   	   }
   	   break;
   	   
   	 case ROUTE_DATA_FORWARD_3762:  //·������ת��
   	 	 switch(fn)
   	 	 {
   	 	 	 case 0x1:   //����ز��ӽڵ�(�㳭)
           //��Լ����
           sendBuf[frameTail++] = *pData;
           
           //�ӽڵ�����
           sendBuf[frameTail++] = 0x00;
           
           //�����ĳ��ȼ�����
           memcpy(&sendBuf[frameTail], pData+1, *(pData+1)+1);
           frameTail += (*(pData+1)+1);
           
           //Ԥ��Ӧ���ֽ���,2014-01-09,add
           sendBuf[6] = expectBytes(pData+2);
           break;
       }
       break;
     
     case ROUTE_DATA_READ_3762:    //·�����ݳ���
     	 switch(fn)
     	 {
     	 	 case 0x01:  //·�����󳭶�����
     	 	 	 //������־
     	 	 	 sendBuf[frameTail++] = *pData;
     	 	 	 
     	 	 	 //���ݳ���L���������ݼ��ز��ӽڵ㸽���ڵ�����n
           memcpy(&sendBuf[frameTail], pData+1, *(pData+1)+2);
           frameTail += (*(pData+1)+2);

           //Ԥ��Ӧ���ֽ���,2014-01-09,add
           sendBuf[6] = expectBytes(pData+2);
     	 	 	 break;
     	 }
     	 break;
       
     case DEBUG_3762:              //�ڲ�����
     	 switch(fn)
     	 {
     	 	  case 0x1:
     	 	  	sendBuf[frameTail++] = 0x01;
     	 	  	break;
     	 }
     	 break;
     
     case RL_EXTEND_3762:         //RL��չ����
     	 switch(fn)
     	 {
     	 	 case 0x1:
           memcpy(&sendBuf[frameTail], pData, 8);
           frameTail += 8;
     	 	 	 break;

     	 	 case 0x2:
           memcpy(&sendBuf[frameTail], pData, 4);
           frameTail += 4;
     	 	 	 break;
     	 }
     	 break;
     
     case FC_QUERY_DATA_3762:    //��Ѹ����չ��ѯ����
     	 switch(fn)
     	 {
     	 	 case 10:    //��֡��ȡDAU��Ϣ
   	 	   	 sendBuf[frameTail++] = 0x03;    //���ؽڵ�
   	 	   	 memcpy(&sendBuf[frameTail],pData, 2);
   	 	   	 frameTail += 2;
   	 	   	 sendBuf[frameTail++] = 0x01;    //���β�ѯ����
     	 	 	 break;
     	 }
     	 break;
     	 
     case FC_NET_CMD_3762:       //��Ѹ����չ��������
     	 switch(fn)
     	 {
     	 	 case 11:    //����DAU��ַ
   	 	   	 sendBuf[frameTail++] = 0x01;    //���ӽڵ������m(2Bytes)
   	 	   	 sendBuf[frameTail++] = 0x00;    //
   	 	     sendBuf[frameTail++] = 0x01;    //��֡����Ľڵ���n
   	   	   memcpy(&sendBuf[frameTail], address, 6);
   	   	   frameTail+=6;
     	 	 	 break;
     	 }
     	 break;
   }
   
   checkSum = 0;
   for(i=3;i<frameTail;i++)
   {
   	  checkSum += sendBuf[i];
   }
   sendBuf[frameTail++] = checkSum;
   sendBuf[frameTail++] = GDW376_2_EOP;  //�����ַ�
   sendBuf[1]  = frameTail;              //����L 1
   sendBuf[2]  = frameTail>>8;           //����L 2
   
   //����
   send(PORT_POWER_CARRIER, sendBuf, frameTail);
   
   return 0;
}

/*******************************************************
��������:calcDt
��������:�������ݵ�Ԫ��ʶ
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
INT8U calcDt(INT8U *dt)
{
   INT8U tmpData,ret;
   
   tmpData = *dt;
   ret = 0;
   while (ret<8)
   {    
     ret++;
     if ((tmpData&1) == 1)
     {
       break;
     }
     tmpData >>= 1;
   }
   
   return ret+(*(dt+1))*8;   
}

/*******************************************************
��������:gdw3762Receiving
��������:���ҵ�����ҵ��׼Q/GDW376.2����֡����
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
INT8S gdw3762Receiving(INT8U *pData,INT8U *recvLen)
{
   INT16U i;
   INT8U  checkSum;
   char   str[5];
   INT8S  ret = RECV_DATA_UNKNOWN;
   INT8U  phase;
   INT8U  tmpLen;

   for(i=0;i<*recvLen;i++)
   {
   	  gdw3762RecvBuf[recvFrameTail++] = pData[i];
   	  
   	  if (recvFrameTail==1)
   	  {
   	  	 if (gdw3762RecvBuf[0]!=GDW376_2_SOP)
   	  	 {
   	  	 	  recvFrameTail = 0;
   	  	    gdw3762RecvBuf[1] = 0xff;
   	  	    gdw3762RecvBuf[2] = 0xff;
            printf("gdw3762Receiving:δ������ʼ�ַ�\n");
   	  	    continue;
   	  	 }
   	  }
   	  
   	  if (recvFrameTail==3)
   	  {
   	  	 if ((gdw3762RecvBuf[1]|(gdw3762RecvBuf[2]<<8))>500)
   	  	 {
   	  	 	  recvFrameTail = 0;
   	  	    gdw3762RecvBuf[1] = 0xff;
   	  	    gdw3762RecvBuf[2] = 0xff;
   	  	    
            printf("gdw3762Receiving:���ճ���>500\n");
   	  	    continue;
   	  	 }
   	  }
   	  
   	  if (recvFrameTail>3)
   	  {
   	  	 //���յ�ָ�����ȵ�һ֡
   	  	 if (recvFrameTail==(gdw3762RecvBuf[1]|(gdw3762RecvBuf[2]<<8)))
   	  	 {
           recvFrame.sop       = gdw3762RecvBuf[0];
           recvFrame.l         = gdw3762RecvBuf[1] | gdw3762RecvBuf[2]<<8;
           recvFrame.c         = gdw3762RecvBuf[3];
           recvFrame.pUserData = gdw3762RecvBuf+4;
           memcpy(recvFrame.r,recvFrame.pUserData,6);
           recvFrame.cs        = gdw3762RecvBuf[recvFrame.l-2];
           recvFrame.eop       = gdw3762RecvBuf[recvFrame.l-1];   	  	 	 
   	  	 	 
   	  	 	 if (recvFrame.eop!=0x16)
   	  	 	 {
   	  	 	 	  printf("lib(Local Module Rx):֡β����!\n");
   	  	 	 	  ret = RECV_TAIL_ERROR_3762;
   	  	 	    recvFrameTail = 0;
   	  	      gdw3762RecvBuf[1] = 0xff;
   	  	      gdw3762RecvBuf[2] = 0xff;
   	  	 	 	  break;
   	  	 	 }
   	  	 	 
   	  	 	 //����У���
   	  	 	 checkSum = 0;
   	  	 	 for(i=3;i<recvFrame.l-2;i++)
   	  	 	 {
   	  	 	 	  checkSum += gdw3762RecvBuf[i];
   	  	 	 }
   	  	 	 if (recvFrame.cs!=checkSum)
   	  	 	 {
   	  	 	 	  printf("gdw3762Receiving:֡У��ʹ���!\n");
   	  	 	 	  ret = RECV_CHECKSUM_ERROR_3762;
   	  	 	    
   	  	 	    recvFrameTail = 0;
   	  	      gdw3762RecvBuf[1] = 0xff;
   	  	      gdw3762RecvBuf[2] = 0xff;
   	  	 	 	  break;
   	  	 	 }
   	  	 	 
   	       
   	  	 	 if ((recvFrame.c&0x3f)==1 || (recvFrame.c&0x3f)==0x0a)  //ͨ�ŷ�ʽΪ1��10(΢��������)ʱ�Ĵ���
   	  	 	 {
   	         if (recvFrame.afn==NULL)
   	         {
   	            printf("gdw3762Receiving:recvFrame.afn��ָ��,�������\n");
   	  	        
   	  	        recvFrameTail = 0;
   	  	        gdw3762RecvBuf[1] = 0xff;
   	  	        gdw3762RecvBuf[2] = 0xff;

   	            return RECV_DATA_UNKNOWN;
   	         }

   	  	 	   ret = RECV_DATA_CORRECT;
   	  	 	   
   	  	 	   //ͨ��ģ���ʶΪ1,��12���ֽڵĵ�ַ
   	  	 	   if (recvFrame.r[0]&0x04)
   	  	 	   {
   	  	 	 	    memcpy(recvFrame.a,recvFrame.pUserData+6,12);
   	  	 	 	    *recvFrame.afn = *(recvFrame.pUserData+6+12);
   	  	 	 	    memcpy(recvFrame.dt,recvFrame.pUserData+6+12+1,2);
   	  	 	 	    recvFrame.pLoadData = recvFrame.pUserData+6+12+1+2;
   	  	 	   }
   	  	 	   else
   	  	 	   {
   	  	 	 	    *recvFrame.afn = *(recvFrame.pUserData+6);
   	  	 	 	    memcpy(recvFrame.dt,recvFrame.pUserData+6+1,2);
   	  	 	 	    recvFrame.pLoadData = recvFrame.pUserData+6+1+2;
   	  	 	   }
   	         
   	  	 	   *recvFrame.fn = calcDt(recvFrame.dt);
   	  	 	   phase = *(recvFrame.pUserData+1);

   	  	 	   //printf("Local AFN=%02X,fn=%d\n",*recvFrame.afn,*recvFrame.fn);
   	  	 	 
     	  	 	 switch(*recvFrame.afn)
     	  	 	 {
     	  	 	 	  case ACK_OR_NACK_3762:   //ȷ�ϻ����
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:   //ȷ��
     	  	 	 	  		 	 *pData     = *recvFrame.pLoadData;     //�ŵ�״̬1
     	  	 	 	  		 	 *(pData+1) = *(recvFrame.pLoadData+1); //�ŵ�״̬2
     	  	 	 	  		 	 *(pData+2) = *(recvFrame.pLoadData+2); //�ȴ�ʱ��1
     	  	 	 	  		 	 *(pData+3) = *(recvFrame.pLoadData+3); //�ȴ�ʱ��2
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 2:   //����
     	  	 	 	  		 	 *pData = *recvFrame.pLoadData;         //����״̬��
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
                case DATA_FORWARD_3762:
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //ת��Ӧ������
     	  	 	 	  		 	 memcpy(pData,recvFrame.pLoadData,2+*(recvFrame.pLoadData+1));
     	  	 	 	  		 	 break;
     	  	 	 	    }
                	break;
     	  	 	 	  	
     	  	 	 	  case QUERY_DATA_3762:    //��ѯ����
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:   //���̴���Ͱ汾��Ϣ
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 9);
     	  	 	 	  		 	 
     	  	 	 	  		 	 //���������ز�ģ��
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='X' && *(recvFrame.pLoadData+1)=='C')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("���Ժ�ز�ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = CEPRI_CARRIER;
     	  	 	 	  		 	 	  
     	  	 	 	  		 	 	  if (recvFrame.pLoadData[6]>=0x11)
     	  	 	 	  		 	 	  {
     	  	 	 	  		 	 	  	lcModuleTypex = CEPRI_CARRIER_3_CHIP;
     	  	 	 	  		 	 	  	printf("�°���Ժ�ز�ģ��\n");
     	  	 	 	  		 	 	  }
     	  	 	 	  		 	 }
     	  	 	 	  		 	 
     	  	 	 	  		 	 if (
     	  	 	 	  		 	 	   (*recvFrame.pLoadData=='S' && *(recvFrame.pLoadData+1)=='E')
     	  	 	 	  		 	 	   || (*recvFrame.pLoadData=='L' && *(recvFrame.pLoadData+1)=='S')  //12-11-8
     	  	 	 	  		 	 	  )
     	  	 	 	  		 	 	
     	  	 	 	  		 	 {  
     	  	 	 	  		 	 	  printf("�����ز�ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = EAST_SOFT_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='W' && *(recvFrame.pLoadData+1)=='D' && *(recvFrame.pLoadData+2)=='W' && *(recvFrame.pLoadData+3)=='L')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("��ΰ�ֵ�����ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = HWWD_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='S' && *(recvFrame.pLoadData+1)=='R')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("SR����ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = SR_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='R' && *(recvFrame.pLoadData+1)=='L')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("�������ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = RL_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='i' && *(recvFrame.pLoadData+1)=='m')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("����΢�ز�ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = MIA_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='C' && *(recvFrame.pLoadData+1)=='T')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("�����ز�ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = TC_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='L' && *(recvFrame.pLoadData+1)=='M')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("����΢�ز�ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = LME_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='F' && *(recvFrame.pLoadData+1)=='C')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("��Ѹ������ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = FC_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='S' && *(recvFrame.pLoadData+1)=='C')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("��������ģ��\n");
     	  	 	 	  		 	 	  *carrierModule = SC_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 4:
     	  	 	 	  		 	 memcpy(mainNodeAddr, recvFrame.pLoadData, 6);
     	  	 	 	  		 	 if (*carrierModule==SC_WIRELESS)
     	  	 	 	  		 	 {
     	  	 	 	  		 	   memcpy(scMainNodeAddr, mainNodeAddr, 6);
     	  	 	 	  		 	 }

     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
     	  	 	 	  case ACTIVE_REPORT_3762:
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		case 1:   //�ϱ��ز��ӽڵ���Ϣ
     	  	 	 	  		  memcpy(pData,recvFrame.pLoadData,1+9*(*recvFrame.pLoadData));
     	  	 	 	  		 	break;
     	  	 	 	  		 
     	  	 	 	  		case 2:   //�ϱ���������
     	  	 	 	  			tmpLen = 4+*(recvFrame.pLoadData+3);
     	  	 	 	  		  memcpy(pData, recvFrame.pLoadData, tmpLen);
     	  	 	 	  		  *(pData+tmpLen) = phase;
     	  	 	 	  		 	break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
     	  	 	 	  case ROUTE_QUERY_3762:   //·�ɲ�ѯ
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //�ӽڵ�����
                       if (*carrierModule==FC_WIRELESS)
     	  	 	 	  		 	 {
     	  	 	 	  		 	   memcpy(pData, recvFrame.pLoadData, 10);
     	  	 	 	  		 	 }
     	  	 	 	  		 	 else
     	  	 	 	  		 	 {
     	  	 	 	  		 	   memcpy(pData, recvFrame.pLoadData, 4);
     	  	 	 	  		 	 }
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 2:    //�ز��ӽڵ���Ϣ
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 3+8*(*(recvFrame.pLoadData+2)));
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 4:    //·������״̬
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 16);
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 6:    //����ע����ز��ӽڵ���Ϣ
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, *(recvFrame.pLoadData+2)*8+3);
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
     	  	 	 	  case ROUTE_DATA_FORWARD_3762:  //·������ת��
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //����ز��ӽڵ�
     	  	 	 	  		 	 memcpy(pData,recvFrame.pLoadData,2+*(recvFrame.pLoadData+1));
     	  	 	 	  		 	 break;
     	  	 	 	    }
     	  	 	 	  	break;
     	  	 	 	  
     	  	 	 	  case ROUTE_DATA_READ_3762:     //·�����ݳ���
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	    {
     	  	 	 	    	 case 1:    //·�����󳭶�����
     	  	 	 	    	 	 memcpy(pData, recvFrame.pLoadData, 9);
     	  	 	 	    	 	 break;
     	  	 	 	    }
     	  	 	 	  	break;
                
                case RL_EXTEND_3762:           //RL��չ����
                	switch(*recvFrame.fn)
                  {
                  	case 4:    //��δ�����ڵ��
     	  	 	 	  		 	memcpy(pData, recvFrame.pLoadData, 2+(*(recvFrame.pLoadData+1)<<8 | *(recvFrame.pLoadData+0))*6);
                  		break;
                  		
                  	case 6:    //������״̬
     	  	 	 	  		 	memcpy(pData, recvFrame.pLoadData, 12);
                  	 	break;
                  }
                	break;

     	  	 	 	  case DEBUG_3762:               //WD/SR�ڲ�����
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //�����ϱ����ܿ���
     	  	 	 	  		 	 *pData = *recvFrame.pLoadData;
     	  	 	 	  		 	 break;
     	  	 	 	  		 
     	  	 	 	  		 case 3:    //�ϱ������ɹ�
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 20);
     	  	 	 	  		 	 break;
     	  	 	 	  		 
     	  	 	 	  		 case 4:    //�ϱ������ڵ�
     	  	 	 	  		 case 5:    //�ϱ������ڵ�
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 20);
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  
     	  	 	 	  case FC_QUERY_DATA_3762:  //��Ѹ����չ��ѯ����
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 2:   //CAC ״̬��Ϣ
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 7);
     	  	 	 	  		 	 break;
     	  	 	 	  		 
     	  	 	 	  		 case 10:  //��֡��ȡDAU��Ϣ
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 4+ *(recvFrame.pLoadData+3)*7);
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 }
   	  	 	 }
   	  	 	 else
   	  	 	 {
   	  	 	 	  printf("ͨ��Ϊ7\n");
   	  	 	 }
   	  	 	  
     	  	 *recvLen = i+2;
   	  	 	 
   	  	 	 //������ɺ�λ��־
   	  	 	 recvFrameTail = 0;
   	  	 	 gdw3762RecvBuf[1] = 0xff;
   	  	 	 gdw3762RecvBuf[2] = 0xff;
   	  	 	 
   	  	 	 break;
   	  	 }
   	  	 else
   	  	 {
   	  	 	  ret = RECV_NOT_COMPLETE_3762;
   	  	 }
   	  }
   	  
   	  if (recvFrameTail>=510)
   	  {
   	  	 recvFrameTail = 0;
   	  	 gdw3762RecvBuf[1] = 0xff;
   	  	 gdw3762RecvBuf[2] = 0xff;
   	  }
   }

   return ret;
}

