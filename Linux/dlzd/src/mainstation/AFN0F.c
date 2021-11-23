/***************************************************
Copyright,2006-2007,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN0F.c
���ߣ�leiyong
�汾��0.9
������ڣ�2007��1��
��������վAFN0F(�ļ�����)�����ļ���
�����б�
     1.
�޸���ʷ��
  01,07-1-18,Leiyong created.
  02,07-3-24,Leiyongȷ�������취,��ʵʱ����
             (AFN=0F,fn=2ʱ��վ����Զ����������,fn=3��վ�����ļ�,fn=4��վ�����ļ����,fn=5���������־)
  03,07-04-04,Leiyong�޸������ж�ȡ���յ��ĳ���Ƭ�εĸ����ķ���
  04,07-10-12,Leiyong����Զ������bug,�洢ԭ��û�д洢�����һ�����ݰ���
  05,08-07-25,Leiyong�޸�,
              1)Զ������ÿ����Ƭ��Ϊ�̶�512�ֽ�;
              2)����NAND Flash��spare�ռ�洢�յ��ĳ���Ƭ���
              3)�������Ƭ����Ŵ�0��ʼ,��������ʱ,���ö��������յ������,����˵�������211������Ƭ��ʱ,�����������Ϊ210,��������վ����
  06,10-03-31,Leiyong,��ֲ��Linux2.6.20
  07,12-09-06,Leiyong,�����ļ��ϴ�����
  07,12-09-18,Leiyong,�޸��ϴ�����Ϊ���Դ��ļ���һ����
***************************************************/
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"

#include "dataBase.h"
#include "string.h"
#include "lcdGui.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "userInterface.h"
#include "md5.h"

#include "AFN0F.h"

//��������
INT8U        upgradeBuff[512];  //����д����
FILE         *fpOfUpgrade=NULL; //�����ļ�ָ��
UPGRADE_FLAG upgradeFlag;       //������־
char         fileName[256], tmpFileName[100], destFileName[100];
BOOL         checkSumError = FALSE;
BOOL         powerUpFirst  = FALSE; //�ϵ��ĵ�һ������
INT16U       uploadCnt=0;           //�ϴ��ļ�����

/*******************************************************
��������:AFN0F
��������:��վ"�ļ�����"(AFN0F)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN0F(INT8U *pDataHead, INT8U *pDataEnd ,INT8U dataFrom)
{
   INT8U        fn, tmpSize;
   INT16U       i, j;
   INT8U        *pData,*tmpData;
   char         say[40],str[10];
	 INT8U        tmpBuf[512];
	 struct stat  statBuf;
	 INT8U        checkData[8], dataX[4];

   UPGRADE_FLAG upgradeFlagv13;    //13���Լ������־
	 INT32U       recvPackage;       //��ǰ���յİ���ʶ(/ƫ��)
	 INT8U        recvEnd = 0;       //�Ƿ������һ������
	 INT16U       nowPkgSize=0;      //��ǰ���յİ��Ĵ�С
	 FILE         *fpOfTar = NULL;
	 char         chkInfo[128];
   char         filenames[20][256];
   char         tmpProcess[256];
   int          len;
   
   //�������ݵ�Ԫ��ʶ��ֵ,����FN��Pnֵ��ȷ������������
   fn = findFnPn(*(pDataHead+2), *(pDataHead+3), FIND_FN);
   if (fn!=1 && fn!=3 && fn!=22)
   {
     lcdBackLight(LCD_LIGHT_ON);
     setParaWaitTime = SET_PARA_WAIT_TIME;
     lcdLightOn = LCD_LIGHT_ON;
     lcdLightDelay = nextTime(sysTime, 0, SET_PARA_WAIT_TIME);
     guiLine(1,17,160,160,0);
   }
   
   switch(fn)
   {
   	 case 1:
 	   	 //DADT
 	   	 pData = pDataHead+4;
 	   	 
       //����������־
       selectParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
       
       //��0�ֽ�,�ļ���ʶ
       //��������ļ�,�ָ�������ǰ״̬
       if (*pData==0x00)
       {
       	 upgradeFlagv13.flag = 0x0;
         saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
         
         AFN0F001(0);
         
         return;
       }
       pData++;

 	   	 //��1�ֽ�,�ļ�����(��ʼ���м�֡Ϊ00H,����֡Ϊ01H)
 	   	 //�ж��Ƿ������һ֡����
 	   	 if (*pData==0x1)
 	   	 {
 	   	 	 recvEnd = 1;
 	   	 }
 	   	 else
 	   	 {
 	   	 	 recvEnd = 0;
 	   	 }
 	   	 pData++;
 	   	 
 	   	 //��2�ֽ�,�ļ�ָ��
 	   	 pData++;
 	   	 
 	   	 //��3,4�ֽ�,�ܶ���n
 	   	 pData += 2;
 	   	 
 	   	 //��5,6,7,8�ֽ�,���հ���(4Bytes)
 	   	 recvPackage = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
 	     pData+=4;

 	     //��9�ֽ�,��i�����ݳ���,2Bytes
 	     nowPkgSize = *pData | *(pData+1)<<8;
 	     pData +=2;

       //�Ƿ���������ʼ?
       if (recvPackage==0)
       {
         if (mkdir("/backup", S_IRUSR | S_IWUSR | S_IXUSR)==0)
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("Զ������v13:����Ŀ¼/backup�ɹ�\n");
         	 }
         }
         else
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("Զ������v13:����Ŀ¼/backupʧ��\n");
         	 }
         }
     
         strcpy(tmpFileName,"/backup/tar");
         if (mkdir(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR)==0)
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("Զ������v13:����Ŀ¼%s�ɹ�\n", tmpFileName);
         	 }
         }
         else
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("Զ������v13:����Ŀ¼%sʧ��\n", tmpFileName);
         	 }
         }
     
         //�����ɾ�����ļ�
         strcpy(tmpFileName,"/backup/remoteLoad");
         if(access(tmpFileName, F_OK) == 0)
         {
           remove(tmpFileName);
           
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
             printf("Զ������v13:remove file %s\n", tmpFileName);
           }
         
           chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         }
         
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
           printf("Զ������v13:%s download begin.\n", tmpFileName);
         }
     
         //�洢������־
         upgradeFlagv13.flag = 1;    //Զ��������־��Ϊ������
         upgradeFlagv13.counter = 0;
         upgradeFlagv13.perFrameSize = nowPkgSize;
         saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
       }
       else
       {
   	   	 //�����յ��ĳ���Ƭ�������Ӧ�յ���űȽ�,�������ͬ,�򷵻�   	   	 
         //  ����ļ�����,�������յ��ĳ���Ƭ��
         strcpy(tmpFileName, "/backup/remoteLoad");
         if(access(tmpFileName, F_OK) == 0)
         {
           stat(tmpFileName, &statBuf);
           if (upgradeFlagv13.perFrameSize)
           {
             upgradeFlagv13.counter = statBuf.st_size/upgradeFlagv13.perFrameSize;           	 
           }
           else
           {
             upgradeFlagv13.counter = statBuf.st_size/nowPkgSize;
           }
           
           if (recvPackage!=upgradeFlagv13.counter)
           {
             if (debugInfo&PRINT_UPGRADE_DEBUG)
             {
           	   printf("Զ������v13:�յ����ݶα�ʶ�����յ������ݶθ�����һ��,����\n");
             }
             return;
           }
         }
         else
         {
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
           	 printf("Զ������v13:�޷�����%s\n", tmpFileName);
           }
           return;
         }
   	   }

 	     upgradeFlagv13.counter = recvPackage;
 	   	 
  	   if (debugInfo&PRINT_UPGRADE_DEBUG)
  	   {
 	   	   printf("Զ������v13:��%d�����ݳ���=%d\n", upgradeFlagv13.counter, nowPkgSize);
 	   	 }


 	     //�洢�յ��ĳ���Ƭ��
       strcpy(tmpFileName,"/backup/remoteLoad");
       if(access(tmpFileName, F_OK) == 0)  //����ļ�����,��׷�ӷ�ʽ��
       {
       	 if((fpOfUpgrade = fopen(tmpFileName,"ab")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("Զ������v13:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }
       }
       else                             //����ļ�������,��ֻд��ʽ��
       {
       	 if((fpOfUpgrade = fopen(tmpFileName,"wb")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("Զ������v13:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }         	 
       }
        	       
       //д������
       if (fwrite(pData, 1, nowPkgSize, fpOfUpgrade)!=nowPkgSize)
       {
         fclose(fpOfUpgrade);

         chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
           printf("Զ������v13:д��ʧ��,�ı��ļ�����\n");
         }
         
         return;
       }

       fclose(fpOfUpgrade);
              
       printf("Զ������v13:ȷ�ϵ�%d������Ƭ��\n", upgradeFlagv13.counter);
       
       sprintf(say, "����<-���ݶ�%d", upgradeFlagv13.counter);
       showInfo(say);
       
       //�յ����һ������,��������
       if (1==recvEnd)
       {
       	 upgradeFlagv13.flag = 0;
       	 
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
       	   printf("Զ������v13:�յ����һ������\n");
       	 }

         //1.ɾ��/backup/tar/�µ������ļ�
         if (system("rm /backup/tar/*")==0)
         {
   	       if (debugInfo&PRINT_UPGRADE_DEBUG)
   	       {
   	         printf("Զ������v13:ɾ��/backup/tar/Ŀ¼�������ļ��ɹ�\n");
   	       }
   	     }
   	     else
   	     {
   	       if (debugInfo&PRINT_UPGRADE_DEBUG)
   	       {
   	         printf("Զ������v13:ɾ��/backup/tar/Ŀ¼�������ļ�ʧ��\n");
   	       }
   	     }

      	 //2.�����ص��ļ����Ƶ�/backup/tar/Ŀ¼��
       	 //2.1Դ�ļ���
       	 strcpy(tmpFileName, "/backup/remoteLoad");
       	  
       	 //2.2Ŀ���ļ���
       	 strcpy(destFileName, "/backup/tar/remoteLoad");

       	 //2.3�����ļ�
         if(rename(tmpFileName, destFileName)<0)
         {
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
             printf("�������ļ�%s����!\n", destFileName);
           }
           
           AFN0F001(0);    //���ڸ����ļ�����,��Ҫ��������
         }
         else
         {
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
             printf("�������ļ�%s�ɹ�\n",destFileName);
           }
       	   chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);

       	   sprintf(tmpProcess, "tar xzvf %s -C /backup/tar/", destFileName);
       	   if (system(tmpProcess)==0)
           {
   	         if (debugInfo&PRINT_UPGRADE_DEBUG)
   	         {
   	           printf("Զ������v13:tar��ѹ�ļ�%s�ɹ�\n", destFileName);
   	         }
   	         
   	         strcpy(tmpFileName, "/backup/tar/chk");
   	         stat(tmpFileName, &statBuf);    //����У���ļ�ͳ����Ϣ
           	 if((fpOfTar = fopen(tmpFileName, "rb")) == NULL)
             {
      	       fclose(fpOfTar);

      	       if (debugInfo&PRINT_UPGRADE_DEBUG)
      	       {
      	         printf("Զ������v13:Can't open file %s\n",tmpFileName);
      	       }
               
               AFN0F001(0);    //��У���ļ�ʧ��,��Ҫ��������
             }
             else
             {
               if (fread(&chkInfo, 1, statBuf.st_size, fpOfTar)==statBuf.st_size)
               {
                 fclose(fpOfTar);
                 
                 chkInfo[statBuf.st_size] = '\0';                 
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("Զ������v13:У���ļ�chk content:%s", chkInfo);
                 }

                 //����ѹ���ļ���MD5ֵ
                 memcpy(tmpFileName, &chkInfo[34], statBuf.st_size-35);
                 tmpFileName[statBuf.st_size-35] = '\0';
                 sprintf(destFileName, "/backup/tar/%s", tmpFileName);
                 strcpy(say, MDFile(destFileName));
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("Զ������v13:tar file %s md5 is:%s\n", destFileName, say);
                 }
                 
                 //�Ƚ�У��ֵ
                 checkData[0] = 0;
                 for(i=0;i<32;i++)
                 {
                 	 if (chkInfo[i]!=say[i])
                 	 {
                 	 	 checkData[0] = 1;
                 	 	 break;
                 	 }
                 }
                 
                 if (checkData[0])
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                   	 printf("Զ������v13:ѹ���ļ�%sУ�����,��������\n", destFileName);
                   }

                   AFN0F001(0);    //ѹ���ļ�У�����,��Ҫ��������
                 }
                 else
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                   	 printf("Զ������v13:ѹ���ļ�%sУ����ȷ\n", destFileName);
                   }
                   
                   sprintf(tmpProcess, "tar xzvf %s -C /backup/tar", destFileName);
               	   if (system(tmpProcess)==0)
                   {
           	         if (debugInfo&PRINT_UPGRADE_DEBUG)
           	         {
           	           printf("Զ������v13:�ٴν�ѹ�ļ�%s�ɹ�\n", destFileName);
           	         }

                     //ȡ�����ļ�
                     if((fpOfTar=fopen("/backup/tar/filelist.ini", "r")) != NULL)
                     {
                       i = 0;
                       while(fgets(filenames[i], 256, fpOfTar) != NULL && i<20)
                       {
                       	 len = strlen(filenames[i]);
                       	 filenames[i][len - 2] = '\0';
                       	 
                       	 i++;
                       }
                       
                       len = i;

                       //����
                       for(i=0; i<len; i++)
                       {
                         getFileName(filenames[i], fileName);
                         
                         sprintf(tmpProcess, "cp /backup/tar/%s %s", fileName, filenames[i]);
                         if (system(tmpProcess)==0)
                         {
                         	 printf("����%s�ɹ�\n", tmpProcess);
                         }
                         else
                         {
                         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
                         	 {
                         	   printf("Զ������v13:�����ļ�%sʧ��\n", fileName);
                         	 }
                         }

                         sprintf(tmpProcess, "chmod 555 %s/%s", filenames[i], fileName);
                         if (system(tmpProcess)==0)
                         {
                         	 printf("����%s�ɹ�\n", tmpProcess);
                         }
                         else
                         {
                         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
                         	 {
                         	   printf("Զ������v13:�ı��ļ�%s����ʧ��\n", fileName);
                         	 }
                         }
                       }
                       
                       fclose(fpOfTar);
    
                       AFN0F001(recvPackage);      //ȷ���յ���Ƭ�����
                       
                       cmdReset=1;
                       
                       if (debugInfo&PRINT_UPGRADE_DEBUG)
                       {
                       	 printf("�����ļ��������,exit����������\n");
                       }
                     }
                     else
                     {
                     	 if (debugInfo&PRINT_UPGRADE_DEBUG)
                     	 {
                     	   printf("Զ������v13:filelist.ini is not exist.");
                     	 }
                     	 
                       AFN0F001(0);    //�ļ��б��ļ���ȡʧ��,��Ҫ��������
                     }
           	       }
           	       else
           	       {
           	         if (debugInfo&PRINT_UPGRADE_DEBUG)
           	         {
           	           printf("Զ������v13:�ٴν�ѹ�ļ�%sʧ��\n", destFileName);
           	         }           	       	 
                     
                     AFN0F001(0);    //�ٴν�ѹ�ļ�ʧ��,��Ҫ��������
           	       }
                 }
               }
               else
               {
                 fclose(fpOfTar);

                 AFN0F001(0);    //���ļ�ʧ��,��Ҫ��������
               }
             }
           }
           else
           {
   	         if (debugInfo&PRINT_UPGRADE_DEBUG)
   	         {
   	           printf("Զ������v13:tar��ѹ�ļ�%sʧ��\n", destFileName);
   	         }
   	         
             AFN0F001(0);    //���ڽ�ѹ�ļ�ʧ��,��Ҫ��������
           }
         }
       }
       else
       {
         AFN0F001(recvPackage);    //ȷ���յ���Ƭ�����
       }
       saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
   	 	 break;
   	   
 	   case 2:   //Զ����������
 	   	 pData = pDataHead+4;
       
       j = *pData | *(pData+1)<<8;
       
       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("Զ������:ÿ����Ƭ�δ�Сj=%d\n", j);
       }
       
       pData+=2;   //��������Ƭ�δ�С
       
       //�����ļ���
       tmpSize = *pData++;
       for(i=0;i<tmpSize;i++)
       {
       	  fileName[i] = *pData++;
       }
       fileName[i] = '\0';

       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("Զ������:�ļ�%s\n",fileName);
       }
       
       //����������־
       selectParameter(88, 33,(INT8U *)&upgradeFlag,sizeof(UPGRADE_FLAG));
       //upgradeFlag.flag = 0x1;
       printf("Զ������:����������־,flag=%d\n", upgradeFlag.flag);

       if (upgradeFlag.flag==0x1)
       {
          upgradeFlag.perFrameSize = j;
          
          //����ļ�����,�������յ��ĳ���Ƭ��
          strcpy(tmpFileName, "/backup/");
          strcat(tmpFileName, fileName);
          if(access(tmpFileName, F_OK) == 0)
          {
            stat(tmpFileName, &statBuf);
            upgradeFlag.counter = statBuf.st_size/upgradeFlag.perFrameSize;
            
            if ((statBuf.st_size%upgradeFlag.perFrameSize)!=0)
            {
            	upgradeFlag.counter++;
            }
          }
          else
          {
          	upgradeFlag.counter = 0;
          }
          
          guiDisplay(28,70,"Զ��������...",1);
 	   	    strcpy(say,"�����");
 	   	    strcat(say,intToString(upgradeFlag.counter+1, 3, str));
 	   	    strcat(say,"������Ƭ��");
          guiDisplay(5,90,say,1);
          requestPacket(upgradeFlag.counter);

          if (debugInfo&PRINT_UPGRADE_DEBUG)
          {
            printf("Զ������:�ļ���С=%d\n", statBuf.st_size);
            printf("Զ������:���յ��Ŀ���=%d\n", upgradeFlag.counter);
            printf("Զ������:%s download continue.\n", tmpFileName);
            printf("Զ������:�����%d������Ƭ��\n", upgradeFlag.counter+1);
          }
       }
       else
       {
          upgradeFlag.perFrameSize = j;
          
          strcpy(tmpFileName,"/backup");
          if (mkdir(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR)==0)
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("����Ŀ¼%s�ɹ�\n",tmpFileName);
          	 }
          }
          else
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("����Ŀ¼%sʧ��\n",tmpFileName);
          	 }
          }

          strcpy(tmpFileName,"/backup/old");
          if (mkdir(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR)==0)
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("����Ŀ¼%s�ɹ�\n",tmpFileName);
          	 }
          }
          else
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("����Ŀ¼%sʧ��\n",tmpFileName);
          	 }
          }

          //���������ɾ�����ļ�,�ٴ��ļ�
          strcpy(tmpFileName,"/backup/");
          strcat(tmpFileName,fileName);
          if(access(tmpFileName, F_OK) == 0)
          {
	          remove(tmpFileName);
	          
	          if (debugInfo&PRINT_UPGRADE_DEBUG)
	          {
	            printf("Զ������:remove file %s\n",tmpFileName);
	          }
          
            chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
          }
          
          if (debugInfo&PRINT_UPGRADE_DEBUG)
          {
            printf("Զ������:%s download begin.\n", tmpFileName);
          }

          //�洢������־
          upgradeFlag.flag = 1;                 //Զ��������־��Ϊ������
          upgradeFlag.counter = 0;
          saveParameter(88, 33, (INT8U *)&upgradeFlag, sizeof(UPGRADE_FLAG));

          requestPacket(upgradeFlag.counter);

          guiLine(1,17,160,160,0);
          guiDisplay(16,70,"Զ��������������",1);
 	   	    strcpy(say,"ÿƬ��");
 	   	    strcat(say,intToString(SIZE_OF_UPGRADE, 3, str));
 	   	    strcat(say,"�ֽ�");
          guiDisplay(28, 90, say, 1);
          
          if (debugInfo&PRINT_UPGRADE_DEBUG)
          {
            printf("Զ������:�����%d������Ƭ��\n", upgradeFlag.counter+1);
          }
       }
       checkSumError = FALSE;
 	   	 break;

 	   case 3:    //�����ļ�
 	   	 pData = pDataHead+4;
 	   	 
 	   	 //��ȡ�����յ��ĳ���Ƭ�������Ӧ�յ���űȽ�,�������ͬ,�򷵻�
 	   	 if ((upgradeFlag.counter+1) != (*pData | *(pData+1)<<8))
 	   	 {
          guiLine(1,17,160,160,0);
          guiDisplay(50,35 ,intToString(*pData | *(pData+1)<<8,3,str),1);
          guiDisplay(75,35 ,intToString(upgradeFlag.counter+1,3,str),1);
          lcdRefresh(17,160);
 	   	 	  return;
 	   	 }

 	     //�洢�յ��ĳ���Ƭ��
 	     pData+=2;
 	     tmpData = pData;
 	     frame.loadLen -= 6;
 	     if ((frame.loadLen)>SIZE_OF_UPGRADE)
 	     {
 	     	  return;
 	     }
       for(i=0;i<frame.loadLen;i++)
       {
          upgradeBuff[i] = *pData++;
       }
       
       strcpy(tmpFileName,"/backup/");
       strcat(tmpFileName,fileName);
       if(access(tmpFileName, F_OK) == 0)  //����ļ�����,��׷�ӷ�ʽ��
       {
         stat(tmpFileName,&statBuf);
         upgradeFlag.counter = statBuf.st_size/upgradeFlag.perFrameSize;
       	 
       	 if((fpOfUpgrade = fopen(tmpFileName,"ab")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("Զ������:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }
       }
       else                             //����ļ�������,��ֻд��ʽ��
       {
       	 if((fpOfUpgrade = fopen(tmpFileName,"wb")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("Զ������:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }         	 
       }
        	       
       //д������
       if (fwrite(&upgradeBuff, 1, frame.loadLen, fpOfUpgrade)!=frame.loadLen)
       {
         fclose(fpOfUpgrade);

         chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
           printf("д��ʧ��,�ı��ļ�����\n");
         }
         
         return;
       }

       fclose(fpOfUpgrade);
       
       upgradeFlag.counter++;   //�յ����洢��ȷ����ż�1
       
       printf("Զ������:�����%d������Ƭ��\n",upgradeFlag.counter+1);

       requestPacket(upgradeFlag.counter);      //ȷ���յ���Ƭ�����
       checkSumError = FALSE;
 	   	 break;
 	   	 
 	   case 4:    //�������
       if (checkSumError==TRUE)
       {
         guiDisplay(16,70,"�ļ�У�����",1);
         guiDisplay(48,90,"�����и���",1);
         
         lcdRefresh(17,160);
       	 return;
       }
       
       guiDisplay(16,70,"Զ�������������",1);
       guiDisplay(48,90,"�����ļ�",1);
       
       //�ļ���ǰ3���ַ�Ϊ"lib"���ļ����Ƶ�libĿ¼��
       if (fileName[0]==0x6c && fileName[1]==0x69 && fileName[2]==0x62)
       {
       	  //1.�Ƚ�ԭ�ļ����ݵ�/backup/oldĿ¼��
       	  //1.1Դ�ļ���
       	  strcpy(tmpFileName,"/lib/");
       	  strcat(tmpFileName,fileName);
          if(access(tmpFileName, F_OK) == 0)  //����ļ�����,�����ļ���/backup/old
          {
       	    //1.2Ŀ���ļ���
       	    strcpy(destFileName,"/backup/old/");
       	    strcat(destFileName,fileName);
       	  
       	    //1.3�����ļ�
            if(rename(tmpFileName,destFileName)<0)
            {
              if (debugInfo&PRINT_UPGRADE_DEBUG)
              {
                printf("�����ļ�%s����!\n",destFileName);
              }
            
              return;
            }
            else
            {
              if (debugInfo&PRINT_UPGRADE_DEBUG)
              {
                printf("�����ļ�%s�ɹ�\n",destFileName);
              }
            }
       	  }

       	  //2.�����ص��ļ����Ƶ�/libĿ¼��
       	  //2.1Դ�ļ���
       	  strcpy(tmpFileName,"/backup/");
       	  strcat(tmpFileName,fileName);
       	  
       	  //2.2Ŀ���ļ���
       	  strcpy(destFileName,"/lib/");
       	  strcat(destFileName,fileName);
       	  
       	  //2.3�����ļ�
          if(rename(tmpFileName,destFileName)<0)
          {
            if (debugInfo&PRINT_UPGRADE_DEBUG)
            {
             printf("�������ļ�%s����!\n",destFileName);
            }
          	
          	//2.4����ʧ��,��ԭԭ�ļ���/libĿ¼��
          	//2.4.1Դ�ļ���
          	strcpy(tmpFileName,"/backup/old");
          	strcat(tmpFileName,fileName);
          	  
          	//2.4.2Ŀ���ļ���
          	strcpy(destFileName,"/lib/");
          	strcat(destFileName,fileName);
          	  
          	//2.4.3�����ļ�
            if(rename(tmpFileName,destFileName)<0)
            {
               if (debugInfo&PRINT_UPGRADE_DEBUG)
               {
                 printf("��ԭ�ļ�ʧ��\n");
               }
            }
            
            return;
          }
          else
          {
            if (debugInfo&PRINT_UPGRADE_DEBUG)
            {
              printf("�������ļ�%s�ɹ�\n",destFileName);
            }
       	  }
       	  
       	  chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
       }
       else
       {
       	  //������ֿ��ļ�HZK16
       	  if (fileName[0]=='H' && fileName[1]=='Z' && fileName[2]=='K')
       	  {
          	 //3.�����ص��ֿ��ļ����Ƶ�/��Ŀ¼��
          	 //3.1Դ�ļ���
          	 strcpy(tmpFileName,"/backup/");
          	 strcat(tmpFileName,fileName);
          	  
          	 //3.2Ŀ���ļ���
          	 strcpy(destFileName,"/");
          	 strcat(destFileName,fileName);
          	  
          	 //3.3�����ļ�
             if(rename(tmpFileName,destFileName)<0)
             {
               if (debugInfo&PRINT_UPGRADE_DEBUG)
               {
                 printf("�������ļ�%s����!\n",destFileName);
               }
               
               return;
             }
             else
             {
               if (debugInfo&PRINT_UPGRADE_DEBUG)
               {
                 printf("�������ļ�%s�ɹ�\n",destFileName);
               }
          	 }
          	 
          	 chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
       	  }
       	  else
       	  {
         	  //�����rcS
         	  if (fileName[0]=='r' && fileName[1]=='c' && fileName[2]=='S')
         	  {
            	 //3.�����ص�rcS���Ƶ�/��Ŀ¼��
            	 //3.1Դ�ļ���
            	 strcpy(tmpFileName,"/backup/");
            	 strcat(tmpFileName,fileName);
            	  
            	 //3.2Ŀ���ļ���
            	 strcpy(destFileName,"/etc/init.d/");
            	 strcat(destFileName,fileName);
            	  
            	 //3.3�����ļ�
               if(rename(tmpFileName,destFileName)<0)
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("�������ļ�%s����!\n",destFileName);
                 }
                 
                 return;
               }
               else
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("�������ļ�%s�ɹ�\n",destFileName);
                 }
            	 }
            	 
            	 //system("chmod 555 /etc/init.d/rcS");
            	 
            	 chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         	  }
         	  else
         	  {
            	 //4.�Ƚ�ԭ�ļ����ݵ�/backup/oldĿ¼��
            	 //4.1Դ�ļ���
            	 strcpy(tmpFileName,"/bin/");
            	 strcat(tmpFileName,fileName);
            	  
            	 if(access(tmpFileName, F_OK) == 0)  //����ļ�����,�����ļ���/backup/old
            	 {
            	   //4.2Ŀ���ļ���
            	   strcpy(destFileName,"/backup/old/");
            	   strcat(destFileName,fileName);
            	  
            	   //4.3�����ļ�
                 if(rename(tmpFileName,destFileName)<0)
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                     printf("�����ļ�%s����!\n",destFileName);
                   }
                 
                   return;
                 }
                 else
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                     printf("�����ļ�%s�ɹ�\n",destFileName);
                   }
                 }
            	 }
   
            	 //5.�����ص��ļ����Ƶ�/binĿ¼��
            	 //5.1Դ�ļ���
            	 strcpy(tmpFileName,"/backup/");
            	 strcat(tmpFileName,fileName);
            	  
            	 //5.2Ŀ���ļ���
            	 strcpy(destFileName,"/bin/");
            	 strcat(destFileName,fileName);
            	  
            	 //5.3�����ļ�
               if(rename(tmpFileName, destFileName)<0)
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("�������ļ�%s����!\n",destFileName);
                 }
                 
            	   //5.4����ʧ��,��ԭԭ�ļ���/binĿ¼��
            	   //5.4.1Դ�ļ���
            	   strcpy(tmpFileName,"/backup/old");
            	   strcat(tmpFileName,fileName);
            	  
            	   //5.4.2Ŀ���ļ���
            	   strcpy(destFileName,"/bin/");
            	   strcat(destFileName,fileName);
            	  
            	   //5.4.3�����ļ�
                 if(rename(tmpFileName,destFileName)<0)
                 {
                 	  if (debugInfo&PRINT_UPGRADE_DEBUG)
                 	  {
                 	    printf("��ԭ�ļ�ʧ��\n");
                 	  }
                 }
	               chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
                 
                 return;
               }
               else
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("�������ļ�%s�ɹ�\n",destFileName);
                 }
                 
	               chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
            	 }
            }
       	  }
       }         

       ackOrNack(TRUE,dataFrom);   //ȫ��ȷ��

       //�洢������־
       upgradeFlag.flag = 0x2;     //��־��Ϊ�������
       saveParameter(88, 33, (INT8U *)&upgradeFlag, sizeof(UPGRADE_FLAG));
       
       if (fileName[0]=='e' && fileName[1]=='s' && fileName[2]=='R' && fileName[3]=='t')
       {
       	  upRtFlag = 1;;           //��ʼ����·�ɳ���
       }
       else   //Ӳ����λ����
       {
          cmdReset = 1;            //�ȴ�2���λ
       }
 	   	 break;
 	   	 
 	   case 5:    //����������־
       upgradeFlag.flag = 0x0;   //��־��Ϊ�������
       saveParameter(88, 33, (INT8U *)&upgradeFlag, sizeof(UPGRADE_FLAG));
       
       upgradeFlagv13.flag = 0x0;
       saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));

       guiDisplay(24,75,"����������־��",1);
       ackOrNack(TRUE,dataFrom);                 //ȫ��ȷ��
 	   	 break;
 	   	 
 	   case 6:    //У���ļ�
 	   	 for(i=0;i<8;i++)
 	   	 {
 	   	   checkData[i] = 0x0;
 	   	 }
  	   
  	   printf("Զ������-У���ļ�\n");

       strcpy(tmpFileName,"/backup/");
       strcat(tmpFileName,fileName);
       
       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("�ļ���=%s\n",tmpFileName);
       }
       
       if((fpOfUpgrade = fopen(tmpFileName,"r+")) == NULL)
       {
  	      if (debugInfo&PRINT_UPGRADE_DEBUG)
  	      {
  	        printf("Զ������-У��:Can't open file %s\n",fileName);
  	      }
  	      
  	      return;
       }

       fseek(fpOfUpgrade, SEEK_SET, 0);
       i=0;
       while (!feof(fpOfUpgrade))
       {
          if (i==0)
          {
            i = 1;
            if (fread(&dataX, 1, 4, fpOfUpgrade)==4)
            {
              for(j=0;j<4;j++)
              {
                 checkData[j] += dataX[j];
              }
            }
          }
          else
          {
            i = 0;
            if (fread(&dataX, 1, 4, fpOfUpgrade)==4)
            {
              for(j=0;j<4;j++)
              {
                 checkData[4+j] += dataX[j];
              }
            }
          }
       }
       fclose(fpOfUpgrade);
       
 	   	 pData = pDataHead+4;
 	   	 
 	   	 if (debugInfo&PRINT_UPGRADE_DEBUG)
 	   	 {
 	   	   printf("��վУ��ֵ:%d,%d,%d,%d,%d,%d,%d,%d\n",*pData,*(pData+1),*(pData+2),*(pData+3),*(pData+4),*(pData+5),*(pData+6),*(pData+7));
 	   	   printf("�ն�У��ֵ:%d,%d,%d,%d,%d,%d,%d,%d\n",checkData[0],checkData[1],checkData[2],checkData[3],checkData[4],checkData[5],checkData[6],checkData[7]);
 	   	 }
 	   	 
 	   	 if (*pData==checkData[0] && *(pData+1)==checkData[1]
 	   	 	   && *(pData+2)==checkData[2]  
 	   	 	     //&& *(pData+3)==checkData[3]
 	   	 	     //ly,2011-08-18,�����ļ����ǳ���У�����,û���ҵ�ԭ��,����ľ�ֻ������ֽ�,�������ص��ļ�copy
 	   	 	     //   �����������ϱȽ�����������,��˲��Ƚ�����ֽ�
 	   	 	    && *(pData+4)==checkData[4] && *(pData+5)==checkData[5]
 	   	 	     && *(pData+6)==checkData[6] && *(pData+7)==checkData[7]
 	   	 	   )
 	   	 {
 	   	 	  if (debugInfo&PRINT_UPGRADE_DEBUG)
 	   	 	  {
 	   	 	    printf("�ļ�������ȷ\n");
 	   	 	  }
          guiDisplay(16,75,"�����ļ�У����ȷ",1);

          requestPacket(0xfffe);                    //ȫ��ȷ��
 	   	 }
 	   	 else
 	   	 {
 	   	 	  if (debugInfo&PRINT_UPGRADE_DEBUG)
 	   	 	  {
 	   	 	    printf("�ļ��������\n"); 
 	   	 	  }
 	   	 	  
 	   	 	  checkSumError = TRUE;
          
          guiDisplay(16,75,"�����ļ�У�����",1);
          requestPacket(0xfffd);                   //ȫ������
 	   	 }
 	   	 break;
 	   
 	   case 21:    //�ļ��ϴ�����
 	   	 pData = pDataHead+4;
       
       j = *pData | *(pData+1)<<8;
       
       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("Զ������:�ļ��ϴ�,ÿ����Ƭ�δ�С=%d\n", j);
       }
       
       pData+=2;   //��������Ƭ�δ�С
       
       //�����ļ���
       tmpSize = *pData++;
       for(i=0;i<tmpSize;i++)
       {
       	 fileName[i] = *pData++;
       }
       fileName[i] = '\0';

       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("Զ������:�ϴ��ļ�%s\n",fileName);
       }
       if(access(fileName, F_OK) != 0)
       {
  	     if (debugInfo&PRINT_UPGRADE_DEBUG)
  	     {
  	       printf("Զ������(�ļ��ϴ�):Can't open file %s\n", fileName);
  	     }
  	     uploadFile(0xfffe);
       }
       else
       {
       	 uploadCnt = (*pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24)/512;
  	     
  	     if (debugInfo&PRINT_UPGRADE_DEBUG)
  	     {
       	   printf("��ʼ�ֽ�=%d\n",uploadCnt*512);
       	 }
       	 
  	     uploadFile(0xfffd);  //�ļ���С
       }
       
       guiDisplay(28,70,"�ļ��ϴ���...",1);
 	   	 break;
 	   
 	   case 22:    //�ļ��ϴ�
 	   	 uploadFile(uploadCnt);
 	   	 break;

 	   case 23:    //�ļ��ϴ�����
 	   	 break;
   }
   
   if (fn!=3)
   {
     lcdRefresh(17,160);
   }
}

/*******************************************************
��������:requestPacket
��������:������վ���ݰ�
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
void AFN0F001(INT32U num)
{
    INT8U  tmpI,checkSum;
    INT16U frameTail0f;
   
    if (fQueue.tailPtr == 0)
    {
      frameTail0f = 0;
    }
    else
    {
      frameTail0f = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    
    msFrame[frameTail0f+0]  = 0x68;   //֡��ʼ�ַ�

    tmpI = ((22 -6) << 2) | 0x2;
    msFrame[frameTail0f+1]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+2]  = tmpI >> 8;
    msFrame[frameTail0f+3]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+4]  = tmpI >> 8; 
  
    msFrame[frameTail0f+5]  = 0x68;  //֡��ʼ�ַ�
 
    msFrame[frameTail0f+6]  = 0xa8;  //�����ֽ�10001000

    //��ַ��
    msFrame[frameTail0f+7]  = addrField.a1[0];
    msFrame[frameTail0f+8]  = addrField.a1[1];
    msFrame[frameTail0f+9]  = addrField.a2[0];
    msFrame[frameTail0f+10] = addrField.a2[1];
    msFrame[frameTail0f+11] = addrField.a3;

    msFrame[frameTail0f+12] = 0x0F;  //AFN

    msFrame[frameTail0f+13] = 0x0;

    msFrame[frameTail0f+14] = 0x00;    //DA1
    msFrame[frameTail0f+15] = 0x00;    //DA2
    msFrame[frameTail0f+16] = 0x01;    //DT1
    msFrame[frameTail0f+17] = 0x00;    //DT2
    
      
    msFrame[frameTail0f+18] = num&0xff;      	
    msFrame[frameTail0f+19] = num>>8&0xff;
    msFrame[frameTail0f+20] = num>>16&0xff;
    msFrame[frameTail0f+21] = num>>24&0xff;

    msFrame[frameTail0f+22] = checkSum;
    msFrame[frameTail0f+23] = 0x16;
    
    fQueue.frame[fQueue.tailPtr].head = frameTail0f;
    fQueue.frame[fQueue.tailPtr].len = 24;
    
    if ((frameTail0f+24+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
    {
       fQueue.frame[fQueue.tailPtr].next = 0x0;
    	 fQueue.tailPtr = 0;
    }
    else
    {                 
       fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
       fQueue.tailPtr++;
    }
}

/*******************************************************
��������:requestPacket
��������:������վ���ݰ�
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
void requestPacket(INT16U num)
{
    INT8U  tmpI,checkSum;
    INT16U frameTail0f;
   
    if (fQueue.tailPtr == 0)
    {
      frameTail0f = 0;
    }
    else
    {
      frameTail0f = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    
    msFrame[frameTail0f+0]  = 0x68;   //֡��ʼ�ַ�

    tmpI = ((20 -6) << 2) | 0x2;
    msFrame[frameTail0f+1]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+2]  = tmpI >> 8;
    msFrame[frameTail0f+3]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+4]  = tmpI >> 8; 
  
    msFrame[frameTail0f+5]  = 0x68;  //֡��ʼ�ַ�
 
    msFrame[frameTail0f+6]  = 0xa8;  //�����ֽ�10001000

    //��ַ��
    msFrame[frameTail0f+7]  = addrField.a1[0];
    msFrame[frameTail0f+8]  = addrField.a1[1];
    msFrame[frameTail0f+9]  = addrField.a2[0];
    msFrame[frameTail0f+10] = addrField.a2[1];
    msFrame[frameTail0f+11] = addrField.a3;

    msFrame[frameTail0f+12] = 0x0F;  //AFN

    msFrame[frameTail0f+13] = 0x0;

    if (num==0xfffe || num==0xfffd)
    {
      //У���ļ�״̬
      msFrame[frameTail0f+14] = 0x00;    //DA1
      msFrame[frameTail0f+15] = 0x00;    //DA2
      msFrame[frameTail0f+16] = 0x20;    //DT1
      msFrame[frameTail0f+17] = 0x00;    //DT2
      
      if (num==0xfffe)
      {
      	msFrame[frameTail0f+18] = 0x1;      	
      }
      else
      {
      	msFrame[frameTail0f+18] = 0x2;
      }
      
      msFrame[frameTail0f+19] = 0x0;
    }
    else
    {
      msFrame[frameTail0f+14] = 0x00;    //DA1
      msFrame[frameTail0f+15] = 0x00;    //DA2
      msFrame[frameTail0f+16] = 0x04;    //DT1
      msFrame[frameTail0f+17] = 0x00;    //DT2

      //��������
      msFrame[frameTail0f+18] = num & 0xff;
      msFrame[frameTail0f+19] = num>>8 & 0xff;
    }

    msFrame[frameTail0f+20] = checkSum;
    msFrame[frameTail0f+21] = 0x16;
    
    fQueue.frame[fQueue.tailPtr].head = frameTail0f;
    fQueue.frame[fQueue.tailPtr].len = 22;
    
    if ((frameTail0f+22+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
    {
       fQueue.frame[fQueue.tailPtr].next = 0x0;
    	 fQueue.tailPtr = 0;
    }
    else
    {                 
       fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
       fQueue.tailPtr++;
    }
}

/*******************************************************
��������:uploadFile
��������:�ϴ��ļ����ݰ�
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
void uploadFile(INT16U num)
{
	  struct stat statBuf;
    
    INT8U  checkSum;
    INT16U tmpI;
    INT16U frameTail0f, tmpHead0f;
    INT8U  readData[512];
    INT16U countOfRead=0;
   
    if (fQueue.tailPtr == 0)
    {
      tmpHead0f = 0;
    }
    else
    {
      tmpHead0f = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    
    msFrame[tmpHead0f+0]  = 0x68;   //֡��ʼ�ַ�
  
    msFrame[tmpHead0f+5]  = 0x68;  //֡��ʼ�ַ�
 
    msFrame[tmpHead0f+6]  = 0xa8;  //�����ֽ�10001000

    //��ַ��
    msFrame[tmpHead0f+7]  = addrField.a1[0];
    msFrame[tmpHead0f+8]  = addrField.a1[1];
    msFrame[tmpHead0f+9]  = addrField.a2[0];
    msFrame[tmpHead0f+10] = addrField.a2[1];
    msFrame[tmpHead0f+11] = addrField.a3;

    msFrame[tmpHead0f+12] = 0x0F;  //AFN

    msFrame[tmpHead0f+13] = 0x0;
    
    frameTail0f = tmpHead0f+14;
    
    //�ļ���С
    stat(fileName,&statBuf);

    if (num==0xfffe || num==0xfffd)
    {
      //F21��Ҫ�ϴ����ļ�������
      msFrame[frameTail0f++] = 0x00;    //DA1
      msFrame[frameTail0f++] = 0x00;    //DA2
      msFrame[frameTail0f++] = 0x10;    //DT1
      msFrame[frameTail0f++] = 0x02;    //DT2
      
      if (num==0xfffe)
      {
        msFrame[frameTail0f++] = 0x0;
      }
      else
      {
        msFrame[frameTail0f++] = 0x1;
        msFrame[frameTail0f++] = statBuf.st_size&0xff;
        msFrame[frameTail0f++] = statBuf.st_size>>8;
        msFrame[frameTail0f++] = statBuf.st_size>>16;
        msFrame[frameTail0f++] = statBuf.st_size>>24;
      }
    }
    else
    {
      //�ļ�����
	    if((fpOfUpgrade = fopen(fileName,"r+")) == NULL)
	    {
	  	  if (debugInfo&PRINT_UPGRADE_DEBUG)
	  	  {
	  	    printf("Զ������-У��:Can't open file %s\n", fileName);
	  	  }
	  	  
        //F21��Ҫ�ϴ����ļ���ʧ��
        msFrame[frameTail0f++] = 0x00;    //DA1
        msFrame[frameTail0f++] = 0x00;    //DA2
        msFrame[frameTail0f++] = 0x10;    //DT1
        msFrame[frameTail0f++] = 0x02;    //DT2
      
        msFrame[frameTail0f++] = 0x1;
	    }
	    else
	    {
        if (uploadCnt==0xffee)
        {
          //F23.�ϴ��ļ�����
          msFrame[frameTail0f++] = 0x00;    //DA1
          msFrame[frameTail0f++] = 0x00;    //DA2
          msFrame[frameTail0f++] = 0x40;    //DT1
          msFrame[frameTail0f++] = 0x02;    //DT2
          
          guiDisplay(28,70,"�ļ��ϴ����.",1);
        }
        else
        {
          //F22.�ϴ��ļ�����
          msFrame[frameTail0f++] = 0x00;    //DA1
          msFrame[frameTail0f++] = 0x00;    //DA2
          msFrame[frameTail0f++] = 0x20;    //DT1
          msFrame[frameTail0f++] = 0x02;    //DT2
          
          fseek(fpOfUpgrade, uploadCnt*512, 0);
          
          fread(readData, 512, 1, fpOfUpgrade);
          
          if (feof(fpOfUpgrade))
          {
            memcpy(&msFrame[frameTail0f], readData, statBuf.st_size-512*uploadCnt);
            frameTail0f += statBuf.st_size-512*uploadCnt;

            uploadCnt = 0xffee;
          }
          else
          {
            uploadCnt++;        	

            memcpy(&msFrame[frameTail0f], readData, 512);
            frameTail0f += 512;
          }
          
          fclose(fpOfUpgrade);          
        }
	    }
    }

    tmpI = ((frameTail0f - tmpHead0f - 6) << 2) | 0x2;
    msFrame[tmpHead0f+1]  = tmpI & 0xFF;   //L
    msFrame[tmpHead0f+2]  = tmpI >> 8;
    msFrame[tmpHead0f+3]  = tmpI & 0xFF;   //L
    msFrame[tmpHead0f+4]  = tmpI >> 8;

    msFrame[frameTail0f++] = checkSum;
    msFrame[frameTail0f++] = 0x16;
    
    fQueue.frame[fQueue.tailPtr].head = tmpHead0f;
    fQueue.frame[fQueue.tailPtr].len  = frameTail0f-tmpHead0f;
    
    if ((tmpHead0f+fQueue.frame[fQueue.tailPtr].len+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
    {
      fQueue.frame[fQueue.tailPtr].next = 0x0;
    	fQueue.tailPtr = 0;
    }
    else
    {                 
      fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
      fQueue.tailPtr++;
    }
}


/*******************************************************
��������:unRecvSeg
��������:�����ļ�����δ�յ������ݶ�,13���Լ
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
void unRecvSeg(INT8U *pFrame)
{
  UPGRADE_FLAG upgradeFlagv13;    //13���Լ������־
	struct stat  statBuf;
	INT8U        i;
  char         say[30],str[10];

  //����������־
  selectParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
  
  if (debugInfo&PRINT_UPGRADE_DEBUG)
  {
    printf("Զ������v13-unRecvSeg:����������־,flag=%d\n", upgradeFlagv13.flag);
  }

  //���
  *pFrame++ = 0x00;
  *pFrame++ = 0x00;

  //���ڸ����ݶ�δ�յ���ʶ
  memset(pFrame, 0xff, 128);

  if (0x1==upgradeFlagv13.flag)
  {
    //����ļ�����,�������յ��ĳ���Ƭ��
    strcpy(tmpFileName, "/backup/remoteLoad");
    if(access(tmpFileName, F_OK) == 0)
    {
      stat(tmpFileName, &statBuf);
      if (upgradeFlagv13.perFrameSize)    //��ֹ������0
      {
        upgradeFlagv13.counter = statBuf.st_size/upgradeFlagv13.perFrameSize;
      }
      else
      {
      	upgradeFlagv13.counter = 0;
      }
      
      memset(pFrame, 0x00, upgradeFlagv13.counter/8);
      for(i=0; i<upgradeFlagv13.counter%8; i++)
      {
      	pFrame[upgradeFlagv13.counter/8] &= ~(1<<i);
      }
    }
    else
    {
     	upgradeFlagv13.counter = 0;
    }

    if (debugInfo&PRINT_UPGRADE_DEBUG)
    {
      printf("Զ������v13-unRecvSeg:���յ����ļ���С=%d\n", statBuf.st_size);
      printf("Զ������v13-unRecvSeg:���յ��Ŀ���=%d\n", upgradeFlagv13.counter);
    }
  }
}
  
