/***************************************************
Copyright,2011,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ�����ESAM.c
���ߣ�leiyong
�汾��0.9
������ڣ�2011��3��
������ESAMоƬ�����ļ���
�����б�
     1.
�޸���ʷ��
  01,11-03-09,Leiyong created.
***************************************************/

#include "teRunPara.h"
#include "ioChannel.h"
#include "hardwareConfig.h"

#include "ESAM.h"

BOOL  hasEsam;             //��ESAMоƬ��?
INT8U esamSerial[8];       //ESAMоƬ���к�

/*******************************************************
��������:resetEsam
��������:��λESAMоƬ
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void resetEsam(void)
{
	 INT8U  i;
	 INT32U numOfCircle;          //ʱ�����ڸ���
	 INT32U j;
	 INT8U  recvBuf[20];
	 BOOL   esamSetIoToZ = FALSE;
	 BOOL   byteStart = FALSE;
	 INT16U etuCircle = 0;
	 INT8U  numOfByte;            //�����ֽ�λ��
	 INT8U  bits;                 //�ֽڵ�λ������
	 INT8U  empty = 0;
	 INT8U  even = 0;
	 INT8U  k;
	 
	 if (ioctl(fdOfIoChannel,ESAM_DETECT,0))
   {
   	  if (debugInfo&ESAM_DEBUG)
   	  {
   	    printf("��⵽ESAM��ȫоƬģ��\n");
   	  }
   	  hasEsam = TRUE;

      for(i=0; i<3; i++)                 //ѭ����λ3��,����ʧ�����˳�
   	  {
   	  	memset(recvBuf, 0x00, 20);
   	  	
   	    numOfCircle = 0;
   	    ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   	    ioctl(fdOfIoChannel, ESAM_RST, 0);   //��λ������
   	    esamSetIoToZ = FALSE;                //ESAMоƬ��IO������Z״̬,Ҳ����˵��λ��Ч
        ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO��Ϊ����
   	  	while(numOfCircle<200000)
   	  	{
   	  	  //����ʱ��
   	  	  if (numOfCircle%2)
   	  	  {
   	  	 	  ioctl(fdOfIoChannel,ESAM_CLOCK,1);
   	  	  }
   	  	  else
   	  	  {
   	  	 	  ioctl(fdOfIoChannel,ESAM_CLOCK,0);
   	  	  }

   	      if (esamSetIoToZ==FALSE)
   	      {
   	  	 	  if (numOfCircle<=400)
   	  	 	  {
   	  	 	     if (numOfCircle>190)
   	  	 	     {
   	  	 	       //400��ʱ��������ESAM��IO��ΪZ״̬
   	  	 	       if (ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	       {
   	  	 	         ioctl(fdOfIoChannel,ESAM_RST,1);   //��λ������
   	  	 	         esamSetIoToZ = TRUE;
   	  	 	         byteStart = FALSE;
   	  	 	         numOfByte = 0;
   	  	 	       }
   	  	 	     }
   	  	 	  }
   	  	 	  else
   	  	 	  {
   	  	 	  	 break;                             //��λδ������,���¿�ʼ��λ����
   	  	 	  }
   	      }
   	      else
   	      {
   	  	 	  if (byteStart==FALSE)
   	  	 	  {
   	  	 	    if (!ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	    {
   	  	 	  	   byteStart = TRUE;
   	  	 	  	   etuCircle = 744;
   	  	 	  	   recvBuf[numOfByte] = 0;
   	  	 	  	   bits = 0;
   	  	 	  	   even = 0;
   	  	 	       //printf("ESAMӦ��,numOfCircle=%d\n",numOfCircle);
   	  	 	       continue;   	  	 	       
   	  	 	  	}
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;   	  	 	  	
   	  	 	  }
   	  	 	  else
   	  	 	  {
   	  	 	  	 if (etuCircle>0)
   	  	 	  	 {
   	  	 	  	   etuCircle--;
   	  	 	  	   empty++;
   	  	 	  	   empty++;
   	  	 	  	   empty++;
   	  	 	  	 }
   	  	 	  	 else
   	  	 	  	 {
   	  	 	  	 	 etuCircle=744;
   	  	 	  	 	 
   	  	 	  	 	 if (bits<9)
   	  	 	  	 	 {
   	  	 	  	 	 	 if (bits<8)
   	  	 	  	 	 	 {
   	  	 	  	 	 	   recvBuf[numOfByte]>>=1;
   	  	 	  	 	 	 }
   	  	 	  	 	 	 else
   	  	 	  	 	 	 {
   	  	 	  	 	 	   empty++;
   	  	 	  	 	 	 }
   	  	 	  	 	 }
   	  	 	  	 	 else
   	  	 	  	 	 {
                   if (bits==9)
                   {
                     ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO��Ϊ���
   	  	 	  	 	 	   if (even%2)
   	  	 	  	 	 	   {
   	  	 	  	 	 	     //printf("��żУ�����,even=%d\n", even);
                       ioctl(fdOfIoChannel, ESAM_IO, 0);  //IO�����
   	  	 	  	 	 	   }
   	  	 	  	 	 	   else
   	  	 	  	 	 	   {
   	  	 	  	 	 	     //printf("��żУ��λ��ȷ,��%d�ֽ�%02x,bits=%d\n", numOfByte+1, recvBuf[numOfByte], bits);
                       ioctl(fdOfIoChannel, ESAM_IO, 1);  //IO�����
                       if (numOfByte==0)
                       {
                       	 //��һ�ֽڴ���,���¿�ʼ��λ����
                       	 if (recvBuf[numOfByte]!=0x3b)
                       	 {
                       	 	  break;
                       	 }
                       }
   	  	 	  	 	 	   }
   	  	 	  	 	 	   
   	  	 	  	 	 	   etuCircle = 200;   //����ʱ��
   	  	 	  	 	 	 }
   	  	 	  	 	 	 else
   	  	 	  	 	 	 {
   	  	 	  	 	 	 	 //����ʱ���ѹ�,��ʼ���յ���һ���ֽ�
   	  	 	  	       numOfByte++;
   	  	 	  	 	 	 	 if (numOfByte>2)
   	  	 	  	 	 	 	 {
   	  	 	  	 	 	 	 	  if (numOfByte>4+(recvBuf[1] & 0x0F))
   	  	 	  	 	 	 	 	  {
   	  	 	  	 	 	 	 	  	 memcpy(esamSerial, &recvBuf[10], 8);
   	  	 	  	 	 	 	 	  	 
   	  	 	  	 	 	 	 	  	 if (debugInfo&ESAM_DEBUG)
   	  	 	  	 	 	 	 	  	 {
   	  	 	  	 	 	 	 	  	   printf("ESAM Serial:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",esamSerial[0],esamSerial[1],esamSerial[2],esamSerial[3],esamSerial[4],esamSerial[5],esamSerial[6],esamSerial[7]);
   	  	 	  	 	 	 	 	  	 }
   	  	 	  	 	 	 	 	  	 i = 3;
   	  	 	  	 	 	 	 	  	 break;
   	  	 	  	 	 	 	 	  }
   	  	 	  	 	 	 	 }
   	  	 	  	 	 	 	 
   	  	 	  	       recvBuf[numOfByte] = 0;
   	  	 	  	       byteStart = FALSE;
                     ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO��Ϊ����
   	  	 	           //printf("����ʱ���ѹ�,��ʼ���յ�%d���ֽ�\n",numOfByte+1);
   	  	 	  	 	 	 }
   	  	 	  	 	 }
   	  	 	  	 	 
   	  	 	  	 	 bits++;
   	  	 	  	 }
   	  	 	  	 
   	  	 	  	 if (etuCircle==372)
   	  	 	  	 {
   	  	 	  	 	  if (bits<9)
   	  	 	  	 	  {
   	  	 	  	 	    if (ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	  	 	    {
   	  	 	  	 	  	   recvBuf[numOfByte] |= 0x80;
   	  	 	  	 	  	   even++;
   	  	 	  	 	    }
   	  	 	  	 	    else
   	  	 	  	 	    {
   	  	 	  	 	  	   recvBuf[numOfByte] |= 0x00;
   	  	 	  	 	    }
   	  	 	  	 	  }
   	  	 	  	 	  else
   	  	 	  	 	  {
   	  	 	  	 	    if (bits==9)
   	  	 	  	 	    {
   	  	 	  	 	      if (ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	  	 	      {
   	  	 	  	 	  	    even++;
   	  	 	  	 	  	    empty++;
   	  	 	  	 	      }
   	  	 	  	 	      else
   	  	 	  	 	      {
   	  	 	  	 	  	    empty++;
   	  	 	  	 	  	    empty++;
   	  	 	  	        }
   	  	 	  	 	    }
   	  	 	  	 	    else
   	  	 	  	 	    {
   	  	 	  	 	    	empty++;
   	  	 	  	 	    	empty++;
   	  	 	  	 	    	empty++;
   	  	 	  	 	    }
   	  	 	  	 	  } 
   	  	 	  	 }
   	  	 	  	 else
   	  	 	  	 {
   	  	 	  	 	  empty++;
   	  	 	  	 	  empty++;
   	  	 	  	 	  empty++;
   	  	 	  	 }
   	  	 	  }
   	      }
   	      
   	      for(k=0;k<150;k++)
   	      {
   	        empty++;
   	      }
   	    
   	  	  numOfCircle++;
   	    }
   	    
   	    ioctl(fdOfIoChannel, ESAM_RST, 1);   //��λ������
   	    ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   	  }
   }
   else
   {
   	 hasEsam = FALSE;
   	 
   	 if (debugInfo&ESAM_DEBUG)
   	 {
   	   printf("δ��⵽ESAM��ȫоƬģ��\n");
   	 }
   }
}

/*******************************************************
��������:putGetBytes
��������:���������ҽ��ջظ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
INT16U putGetBytes(INT8U *cmdBuf, INT8U lenOfCmd, INT8U respondType, INT8U *recvBuf,INT8U lenOfRecv)
{
	 BOOL   status      = 0;           //���ͽ���״̬(0-����,1-����)	 
	 INT32U numOfCircle = 0;           //ʱ�����ڸ���
	 INT8U  numOfByte   = 0;           //����/�����ֽ�λ��
	 INT16U etuCircle   = 0;
	 INT8U  bits;                      //�ֽڵ�λ������
	 INT8U  empty = 0;
	 INT8U  even  = 0;
	 INT8U  nowByte;                   //��ǰҪ���͵��ֽ�
	 BOOL   byteStart = FALSE;         //����/�����ֽڿ�ʼ��־
	 BOOL   parityError = FALSE;       //��żУ�����
	 INT8U  k;

   ioctl(fdOfIoChannel, ESAM_IO, 2); //IO��Ϊ���
   ioctl(fdOfIoChannel, ESAM_IO, 1); //IO��Ϊ�����
   numOfCircle = 0;
   while(numOfCircle<(lenOfCmd+lenOfRecv+5)*11*744)
   {
   	 //����ʱ��
   	 if (numOfCircle%2)
   	 {
   	   ioctl(fdOfIoChannel, ESAM_CLOCK, 1);
   	 }
   	 else
   	 {
   	   ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   	 }
     
     //����״̬
     if (status==0)
     {
 	 	   if (byteStart==FALSE)
 	 	   {
 	 	     if (numOfByte>=lenOfCmd)
 	 	     {
 	 	     	 status = 1;
   	  	 	 
   	  	 	 byteStart = FALSE;
   	  	 	 numOfByte = 0;
 	 	     }
 	 	     else
 	 	     {
 	 	       ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO��Ϊ���
       	   ioctl(fdOfIoChannel, ESAM_IO, 0);    //IO��Ϊ�����,��ʼλ��Ϊ0
       	 
       	   nowByte = cmdBuf[numOfByte];
 	 	  	   byteStart = TRUE;
 	 	  	   etuCircle = 744;
 	 	  	   bits = 0;
 	 	  	   even = 0;
 	 	  	   parityError = FALSE;
 	 	       printf("��ESAM���͵�%d��������%02x,numOfCircle=%d\n", numOfByte+1, cmdBuf[numOfByte], numOfCircle);
 	 	     }
 	 	   }
 	 	   else
 	 	   {
  	     if (etuCircle==744)
  	     {
           if (bits==0)
           {
     	        //empty++;
     	        //empty++;
     	        //empty++;
     	        //empty>>=1;
           }
           else
           {
             if (bits<9)
             {
               if((nowByte&0x01)==0x01)
               {
     	           ioctl(fdOfIoChannel, ESAM_IO, 1);    //IO��Ϊ�����
     	           //printf("��%dλ�����\n",bits);
                 even++;
               }
               else
               {
     	           ioctl(fdOfIoChannel, ESAM_IO, 0);    //IO��Ϊ�����
     	           //printf("��%dλ�����\n",bits);
     	           empty++;
               }
               nowByte>>=1;
             }
             else
             {
               if (bits==9)
               {
                 if(even%2)
                 {
     	             ioctl(fdOfIoChannel, ESAM_IO, 1);    //��żУ��λ��
     	             //printf("У��λ�����\n");
                 }
                 else
                 {
     	             ioctl(fdOfIoChannel, ESAM_IO, 0);    //��żУ��λ��
     	             //printf("У��λ�����\n");
                 }             	 
     	           empty++;
     	           empty>>=1;
     	         }
     	         else
     	         {
     	         	 empty++;
     	         	 empty++;
     	           empty>>=1;
     	         }
     	       }
           }
  	     }
  	     else
  	     {
     	     //empty++;
     	     //empty++;
     	     //empty++;
     	     //empty>>=1;
  	     }
 	  	   
 	  	   if (etuCircle>0)
 	  	   {
 	  	     etuCircle--;
 	  	     
 	  	     if (etuCircle==500 && bits==10)
 	  	     {
 	  	     	 if (!ioctl(fdOfIoChannel, ESAM_IO, 4))
 	  	     	 {
 	  	     	 	  printf("����ָ����żУ�����\n");
 	  	     	 	  //parityError = TRUE;
 	  	     	 }
 	  	     	 else
 	  	     	 {
 	  	     	 	  //printf("����ָ����żУ����ȷ\n");
 	  	     	 }
 	  	     }
 	  	   }
 	  	   else
 	  	   {
 	 	  	 	 etuCircle=744;
           
           if (bits==9)
           {
             ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO��Ϊ����
             etuCircle=744;
 	  	 	 	 }
 	  	 	 	 else
 	  	 	 	 {
 	  	 	 	 	 //empty++;
 	  	 	 	 	 //empty++;
 	  	 	 	 }
 	  	 	 	 
 	  	 	 	 if (bits==10)
 	  	 	 	 {
 	  	 	 	 	 //if (parityError==FALSE)
 	  	 	 	 	 //{
 	  	 	 	 	    numOfByte++;
 	 	  	        byteStart = FALSE;
 	  	 	 	 	    continue;
 	  	 	 	 	 //}
 	  	 	 	 	 //else
 	  	 	 	 	 //{
 	 	           //ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO��Ϊ���
       	       //ioctl(fdOfIoChannel, ESAM_IO, 1);    //IO��Ϊ�����
 	  	 	 	 	 //}
 	  	 	 	 }
 	  	 	 	 else
 	  	 	 	 {
 	  	 	 	 	 //empty++;
 	  	 	 	 	 //empty++;
 	  	 	 	 }
 	  	 	 	 
 	  	 	 	 if (bits==11)
 	  	 	 	 {
 	  	 	 	 	  numOfByte++;
 	  	 	 	 	  byteStart = FALSE;
 	  	 	 	 }
 	 	  	 	 
 	 	  	 	 bits++;
 	  	   }
       }
     }
     else
     {
  	 	  if (byteStart==FALSE)
  	 	  {
  	 	    if (lenOfRecv<1)
  	 	    {
  	 	    	 break;
  	 	    }
  	 	    if (!ioctl(fdOfIoChannel, ESAM_IO, 4))
  	 	    {
  	 	  	   byteStart = TRUE;
  	 	  	   etuCircle = 744;
  	 	  	   recvBuf[numOfByte] = 0;
  	 	  	   bits = 0;
  	 	  	   even = 0;
  	 	       //printf("ESAM��ʼӦ��,numOfCircle=%d\n",numOfCircle);
  	 	       continue;   	  	 	       
  	 	  	}
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;   	  	 	  	
  	 	  }
  	 	  else
  	 	  {
  	 	  	 if (etuCircle>0)
  	 	  	 {
  	 	  	   etuCircle--;
  	 	  	   empty++;
  	 	  	   empty++;
  	 	  	   empty++;
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	 	 etuCircle=744;
  	 	  	 	 
  	 	  	 	 if (bits<9)
  	 	  	 	 {
  	 	  	 	 	 if (bits<8)
  	 	  	 	 	 {
  	 	  	 	 	   recvBuf[numOfByte]>>=1;
  	 	  	 	 	 }
  	 	  	 	 	 else
  	 	  	 	 	 {
  	 	  	 	 	   empty++;
  	 	  	 	 	 }
  	 	  	 	 }
  	 	  	 	 else
  	 	  	 	 {
               if (bits==9)
               {
                 ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO��Ϊ���
  	 	  	 	 	   if (even%2)
  	 	  	 	 	   {
  	 	  	 	 	     printf("��żУ�����,even=%d\n", even);
                   ioctl(fdOfIoChannel, ESAM_IO, 0);  //IO�����
  	 	  	 	 	   }
  	 	  	 	 	   else
  	 	  	 	 	   {
  	 	  	 	 	     printf("��żУ��λ��ȷ,��%d�ֽ�%02x,bits=%d\n", numOfByte+1, recvBuf[numOfByte], bits);
                   ioctl(fdOfIoChannel, ESAM_IO, 1);  //IO�����
  	 	  	 	 	   }
  	 	  	 	 	   
  	 	  	 	 	   etuCircle = 200;   //����ʱ��
  	 	  	 	 	 }
  	 	  	 	 	 else
  	 	  	 	 	 {
  	 	  	 	 	 	 //����ʱ���ѹ�,��ʼ���յ���һ���ֽ�
  	 	  	       numOfByte++;
  	 	  	 	 	 	 if (numOfByte>lenOfRecv-1)
  	 	  	 	 	 	 {
  	 	  	 	 	 	 	  break;
  	 	  	 	 	 	 }
  	 	  	 	 	 	 
  	 	  	       recvBuf[numOfByte] = 0;
  	 	  	       byteStart = FALSE;
                 ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO��Ϊ����
  	 	           //printf("����ʱ���ѹ�,��ʼ���յ�%d���ֽ�\n",numOfByte+1);
  	 	  	 	 	 }
  	 	  	 	 }
  	 	  	 	 
  	 	  	 	 bits++;
  	 	  	 }
  	 	  	 
  	 	  	 if (etuCircle==372)
  	 	  	 {
  	 	  	 	  if (bits<9)
  	 	  	 	  {
  	 	  	 	    if (ioctl(fdOfIoChannel, ESAM_IO, 4))
  	 	  	 	    {
  	 	  	 	  	   recvBuf[numOfByte] |= 0x80;
  	 	  	 	  	   even++;
  	 	  	 	    }
  	 	  	 	    else
  	 	  	 	    {
  	 	  	 	  	   recvBuf[numOfByte] |= 0x00;
  	 	  	 	    }
  	 	  	 	  }
  	 	  	 	  else
  	 	  	 	  {
  	 	  	 	    if (bits==9)
  	 	  	 	    {
  	 	  	 	      if (ioctl(fdOfIoChannel, ESAM_IO, 4))
  	 	  	 	      {
  	 	  	 	  	    even++;
  	 	  	 	  	    empty++;
  	 	  	          //printf("��żУ��λ=1\n");
  	 	  	 	      }
  	 	  	 	      else
  	 	  	 	      {
  	 	  	 	  	    empty++;
  	 	  	 	  	    empty++;
  	 	  	          //printf("��żУ��λ=0\n");
  	 	  	        }
  	 	  	 	    }
  	 	  	 	    else
  	 	  	 	    {
  	 	  	 	    	empty++;
  	 	  	 	    	empty++;
  	 	  	 	    	empty++;
  	 	  	 	    }
  	 	  	 	  } 
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	 	  empty++;
  	 	  	 	  empty++;
  	 	  	 	  empty++;
  	 	  	 }
   	    
   	       for(k=0;k<50;k++)
   	       {
   	         empty++;
   	       }
   	    }
     }
   	  
   	 numOfCircle++;
   }
   
   ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   
   return 1;
}

/*******************************************************
��������:getChallenge
��������:ȡ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void getChallenge(INT8U *buf)
{
	INT8U cmdBuf[5], recvBuf[20];
	INT8U i;

  for(i=0;i<10;i++)
  {
    //cmdBuf[0] = 0x00;
    //cmdBuf[1] = 0xC0;
    //cmdBuf[2] = 0x00;
    //cmdBuf[3] = 0x00;
    //cmdBuf[4] = 0x08;
    //putGetBytes(cmdBuf, 5, 0, recvBuf, 2);

    cmdBuf[0] = 0x00;
    cmdBuf[1] = 0x84;
    cmdBuf[2] = 0x00;
    cmdBuf[3] = 0x00;
    cmdBuf[4] = 0x08;
    putGetBytes(cmdBuf, 5, 0, recvBuf, 11);
    //putGetBytes(cmdBuf, 5, 0, recvBuf, 0);
    
    if (recvBuf[0]==0x84 && recvBuf[9]==0x90 && recvBuf[10]==0x00)
    {
    	 if (debugInfo&ESAM_DEBUG)
    	 {
    	   printf("�ն������:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",recvBuf[1],recvBuf[2],recvBuf[3],recvBuf[4],recvBuf[5],recvBuf[6],recvBuf[7],recvBuf[8]);
    	 }
    	 break;
    }
    
    usleep(500000);
  }
  
  if (i<10)
  {
  	for(i=0;i<8;i++)
    {
      *buf = recvBuf[8-i];
      buf++;
    }
  }
  else
  {
  	if (debugInfo&ESAM_DEBUG)
  	{
    	 printf("��ȡ�ն������ʧ��\n");
  	}
  }
}
