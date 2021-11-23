/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:dataBase.c
����:leiyong
�汾:0.9
�������:2009�� ��
����:�����ն�(�����ն�/������,AT91SAM9260������)���ݿ⴦���ļ�
�����б�:
  1.
�޸���ʷ:
  01,09-10-28,Leiyong created.
  02,14-09-26,Leiyong,����,ÿ������0��7������һ�ι�������

***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "sqlite3.h"

#include "common.h"
#include "msSetPara.h"
#include "teRunPara.h"
#include "convert.h"
#include "att7022b.h"

#include "workWithMeter.h"
#include "copyMeter.h"
#include "userInterface.h"

#include "wlModem.h"
#include "hardwareConfig.h"

#include "dataBalance.h"
#include "dataBase.h"

//��������
sqlite3 *sqlite3Db;                //ȫ��SQLite3���ݿ���(SQLite db handle)

INT32U  iEventStartPtr;            //��Ҫ�¼���1����¼ָ��
INT32U  nEventStartPtr;            //��Ҫ�¼���1����¼ָ��
extern  char originPassword[7];    //ԭʼ����

INT8U   dbLocker=0;                //д���ݿ���
INT8U   dbMonitor=0;               //���ӵ����ݿ��쳣����

/*******************************************************
��������:sqlite3Aberrant
��������:Sqlite3�쳣����
���ú���:
�����ú���:
�������:
�������:
����ֵ:
*******************************************************/
BOOL sqlite3Aberrant(INT16U aberrantCode)
{
	char tmpStr[100];
	
	if (aberrantCode!=SQLITE_DONE && aberrantCode!=SQLITE_ROW)
	{
	  sprintf(tmpStr,"sqlite3Aberrant:(%s) code=%d", sqlite3_errmsg(sqlite3Db), aberrantCode);

	  if (debugInfo&PRINT_DATA_BASE)
	  {
	    printf("%s\n",tmpStr);
	  }
	  
	  logRun(tmpStr);
	  
	  if (aberrantCode!=SQLITE_ERROR)
	  {
  	  //2012-09-04,�������ݿ��쳣����
  	  dbMonitor++;
  	  if (dbMonitor>50)
  	  {
  	  	sqlite3_close(sqlite3Db);
  	  	
  	  	sleep(1);
  	  	
  	  	logRun("sqlite3Aberrant:monitered aberrant times is over 50,exit process");
  	  	
  	  	printf("sqlite3Aberrant:monitered aberrant times is over 50,exit process\n");
  	  	exit(0);
  	  }
  
  	  if (debugInfo&PRINT_DATA_BASE)
  	  {
  	    printf("sqlite3Aberrant:�������ݿ��쳣����Ϊ%d\n", dbMonitor);
  	  }
  	}
	}
	
	switch(aberrantCode)
  {
  	case SQLITE_PERM:    //�ܾ�����
  	case SQLITE_BUSY:    //���ݿ�æ
  	case SQLITE_LOCKED:  //���ݿ�����
  	case SQLITE_MISUSE:  //Library used incorrectly
      //�ر����ݿ�
      sqlite3_close(sqlite3Db);

      //�����ݿ�
      if (sqlite3_open("powerData.db", &sqlite3Db))
      {
        printf("sqlite3Aberrant:Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
        sqlite3_close(sqlite3Db);
        
        logRun("sqlite3Aberrant:detected database locked or access permission denied, and reopen failure, process over\n");
        
        exit(0);
      }
      else
      {
        sprintf(tmpStr,"sqlite3Aberrant:database aberrant,aberrant code=%d,reopen database success", aberrantCode);
        logRun(tmpStr);
        printf("%s\n", tmpStr);
      }
  	 	break;
  }

  return TRUE;
}

/*******************************************************
��������:sqlite3Watch
��������:Sqlite3����
���ú���:
�����ú���:
�������:
�������:
����ֵ:

��ʷ:
   1)ly,2012-04-25,��Ӵ˺���.ּ�ڼ������ݿ�״̬
   2)ly,2012-07-28,���Է���,һ���������ݿ�ʧ��,���´򿪻��Ƿ������ݿ�ʧ��,�޸�Ϊ�������ݿ�ʧ��,ֱ���˳�����,���ػ����¿�ʼ����ִ��
*******************************************************/
void sqlite3Watch(void)
{
  char    *errMessage;               //������Ϣ(Error msg written her)

  if (sqlite3_exec(sqlite3Db, "select acc_pn from f10_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	;
  }
  else
  {
    //�ر����ݿ�
    sqlite3_close(sqlite3Db);

    //�����ݿ�
    if (sqlite3_open("powerData.db", &sqlite3Db))
    {
      printf("sqlite3Watch:Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
      sqlite3_close(sqlite3Db);
    }
    
    logRun("sqlite3Watch:���ݿ��쳣.��������\n");
    
    printf("sqlite3Watch:���ݿ��쳣,��������\n");
    
    exit(0);
  }
  
  return TRUE;
}

/*******************************************************
��������:saveBakKeyPara
��������:���汸�ݹؼ���¼����
���ú���:
�����ú���:
�������:type 1-�ն˵�ַ������������
              2-VPN�û�������
              3-��վIP��ַ���˿�
�������:
����ֵ:

��ʷ:
*******************************************************/
void saveBakKeyPara(INT8U type)
{
	FILE *fp;

 	switch(type)
  {
  	case 3:    //��վIP��ַ�Ͷ˿�(28Bytes)
      if((fp=fopen("/keyPara003","wb"))==NULL)
      {
  	    logRun("open file keyPara003 (for save bak key para) failure.");
  	    return;
      }

  		fwrite(&ipAndPort, sizeof(IP_AND_PORT), 1, fp);
  		break;
			
		case 7: 	 //�ն�IP��ַ�Ͷ˿�(��1Bytes),2020-11-18,Add
			if((fp=fopen("/keyPara007","wb"))==NULL)
			{
				logRun("open file keyPara007 (for save bak key para) failure.");
				return;
			}
		
			fwrite(&teIpAndPort.ethIfLoginMs, 1, 1, fp);
			break;

  	case 16:    //VPN�û�������(64Bytes)
      if((fp=fopen("/keyPara016","wb"))==NULL)
      {
  	    logRun("open file keyPara016 (for save bak key para) failure.");
  	    return;
      }
      
  		fwrite(&vpn, 64, 1, fp);
  		break;

  	case 121:    //��ַ(5Bytes)
      if((fp=fopen("/keyPara121","wb"))==NULL)
      {
  	    logRun("open file keyPara121 (for save bak key para) failure.");
  	    return;
      }

  		fwrite(&addrField, 5, 1, fp);
  		break;

  	case 129:    //����У�����(80+xBytes)
      if((fp=fopen("/keyPara129","wb"))==NULL)
      {
  	    logRun("open file keyPara129 (for save bak key para) failure.");
  	    return;
      }

  		fwrite(&acSamplePara, sizeof(AC_SAMPLE_PARA), 1, fp);
  		break;  
  }
  
  printf("saveBakKeyPara:type=%d ok\n", type);

  fclose(fp);
}

/*******************************************************
��������:readBakKeyPara
��������:��ȡ���ݹؼ���¼����
���ú���:
�����ú���:
�������:type 1-�ն˵�ַ������������
              2-VPN�û�������
              3-��վIP��ַ���˿�
�������:
����ֵ:

��ʷ:
*******************************************************/
void readBakKeyPara(INT8U type,INT8U *buf)
{
	FILE *fp;
  
 	switch(type)
  {
  	case 3:    //��վIP��ַ�Ͷ˿�(28Bytes)
      if((fp=fopen("/keyPara003","rb"))==NULL)
      {
      	logRun("open file keyPara003 (for read bak key para) failure.");
      	return;
      }

  		fread(buf, sizeof(IP_AND_PORT), 1, fp);
  		break;
			
		case 7: 	 //�ն�IP��ַ�Ͷ˿�(��1Bytes),2020-11-18,Add
			if((fp=fopen("/keyPara007","rb"))==NULL)
			{
				logRun("open file keyPara007 (for read bak key para) failure.");
				return;
			}
		
			fread(&teIpAndPort.ethIfLoginMs, 1, 1, fp);
			break;
  	case 16:    //VPN�û�������(64Bytes)
      if((fp=fopen("/keyPara016","rb"))==NULL)
      {
      	logRun("open file keyPara016 (for read bak key para) failure.");
      	return;
      }
      
  		fread(buf, 64, 1, fp);
  		break;

  	case 121:    //��ַ(5Bytes)
      if((fp=fopen("/keyPara121","rb"))==NULL)
      {
      	logRun("open file keyPara121 (for read bak key para) failure.");
      	return;
      }
  
  		fread(buf, 5, 1, fp);
  		break;

  	case 129:    //����У�����(80+xBytes)
      if((fp=fopen("/keyPara129","rb"))==NULL)
      {
      	logRun("open file keyPara129 (for read bak key para) failure.");
      	return;
      }
  
  		fread(buf, sizeof(AC_SAMPLE_PARA), 1, fp);
  		break;
  }

  fclose(fp);
}

/*******************************************************
��������:initDataBase
��������:��ʼ�����ݿ�
         ������ݿ�(/��)�Ƿ����,����������������ݿ�����ݿ��         
���ú���:
�����ú���:
�������:
�������:
�������:
����ֵ:void
*******************************************************/
void initDataBase(void)
{  
  char    *pSqlStr;                  //SQL����ַ���ָ��
  char    *errMessage;               //������Ϣ(Error msg written her)
  int     result;
	sqlite3_stmt  *stat;

  //�鿴ϵͳ�Ƿ�֧��pppЭ��,���֧��,��ʹ��ppp����
  //12-11-06
  if(access("/dev/ppp", F_OK) == 0)
  {
    operateModem  = 0;           //��ͣ����modem
    bakModuleType = MODEM_PPP;
  }
  else
  {
  	operateModem = 1;         //���Բ���modem
  }

  //�����ݿ� start-----------------------------------------------------------------

  //0.�����ݿ�
  if (sqlite3_open("powerData.db", &sqlite3Db))
  {
     printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
     sqlite3_close(sqlite3Db);
     return;
  }
  
  //1.������
  //1.1 f10_info
  if (sqlite3_exec(sqlite3Db, "select acc_pn from f10_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("f10_info�������Ѿ�����\n");
  	 }
  }
  else
  {
	   pSqlStr = "create table f10_info(acc_pn INTEGER, acc_port INTEGER, acc_meter_addr blob, acc_num INTEGER, acc_data blob)";

     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("����������f10_info�ɹ�\n");
     }
     else
     {
     	  printf("����������f10_infoʧ��\n");
     }
	}
	
	//1.2 base_info
  if (sqlite3_exec(sqlite3Db, "select acc_afn from base_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("base_info�������Ѿ�����\n");
  	 }
  }
  else
  {
	   pSqlStr = "create table base_info(acc_afn blob, acc_fn blob, acc_data blob)";	
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("����������base_info�ɹ�\n");
     }
     else
     {
     	  printf("����������base_infoʧ��\n");
     }
	}
	
	//1.3 base_vice_info
  if (sqlite3_exec(sqlite3Db, "select acc_afn from base_vice_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("base_vice_info�������Ѿ�����\n");
  	 }
  }
  else
  {
	   pSqlStr = "create table base_vice_info(acc_afn blob, acc_fn blob, acc_pn blob, acc_data blob)";	
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("����������base_vice_info�ɹ�\n");
     }
     else
     {
     	  printf("����������base_vice_infoʧ��\n");
     }
	}

  //2.�����ʵʱ���ݱ�
  checkSpRealTable(1);
  
  //3.�������ݱ�
  if (sqlite3_exec(sqlite3Db, "select pn from lastMonthData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("�������ݱ��Ѿ�����\n");
  	 }
  }
  else
  {
     pSqlStr = "create table lastMonthData(pn int,queryType int,dataType int,time int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������lastMonthData�ɹ�\n");
     }
     else
     {
     	  printf("������lastMonthDataʧ��\n");
     }
  }
  
  //4.�ս������ݱ�
  if (sqlite3_exec(sqlite3Db, "select pn from dayBalanceData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("�ս������ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table dayBalanceData(pn int,queryType int,dataType int,freezeTime int, copyTime int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������dayBalanceData�ɹ�\n");
     }
     else
     {
     	  printf("������dayBalanceDataʧ��\n");
     }
  }
  
  //5.�½������ݱ�
  if (sqlite3_exec(sqlite3Db, "select pn from monthBalanceData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("�½������ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table monthBalanceData(pn int,queryType int,dataType int,freezeTime int, copyTime int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������monthBalanceData�ɹ�\n");
     }
     else
     {
     	  printf("������monthBalanceDataʧ��\n");
     }
  }

  //6.�¼���¼��
  if (sqlite3_exec(sqlite3Db, "select storeNo from eventRecord limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("�¼���¼���Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table eventRecord(storeNo int, eventType int, len int, data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������eventRecord�ɹ�\n");
     }
     else
     {
     	  printf("������eventRecordʧ��\n");
     }
  }
  
  //7.�ն�/���ͳ�����ݱ�
  if (sqlite3_exec(sqlite3Db, "select pn from statisData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("�¼���¼���Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table statisData(pn int, day int, month int, year int, data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������statisData �ɹ�\n");
     }
     else
     {
     	  printf("������statisDataʧ��\n");
     }
  }
  
  //8.ʵʱ���ݱ�
  checkSpRealTable(0);    //�����

  //9.������ն�������
  pSqlStr = "select pn from singlePhaseDay limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("������ն������ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table singlePhaseDay(pn int,freezeTime int, copyTime int,data blob);";
     
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������singlePhaseDay�ɹ�\n");
     }
     else
     {
     	  printf("������singlePhaseDayʧ��\n");
     }
  }
  
  //10.������¶�������
  pSqlStr = "select pn from singlePhaseMonth limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("������¶������ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table singlePhaseMonth(pn int, freezeTime int, copyTime int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������singlePhaseMonth�ɹ�\n");
     }
     else
     {
     	  printf("������singlePhaseMonthʧ��\n");
     }
  }
  
 #ifdef LOAD_CTRL_MODULE
  //11.ʣ�������
  pSqlStr = "select pn from leftPower limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("ʣ��������ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table leftPower(pn int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������leftPower�ɹ�\n");
     }
     else
     {
     	  printf("������leftPowerʧ��\n");
     }
  }
 #endif
 
  //12.���㶳�����ݱ�
  checkSpRealTable(2);
  
  //13.����ʾֵ���ݱ�
  pSqlStr = "select data from acVision;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("����ʾֵ���ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table acVision(dataType int, time int, data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	 printf("������acVision�ɹ�\n");
     }
     else
     {
     	 printf("������acVisionʧ��\n");
     }
  }
  
  //14.ֱ��ģ��������
 #ifdef PLUG_IN_CARRIER_MODULE
  pSqlStr = "select pn from dcAnalog limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("ֱ��ģ�������ݱ��Ѿ�����\n");
  	}
  }
  else
  {
     pSqlStr = "create table dcAnalog(pn int, copyTime int, data blob);";
     
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("������dcAnalog�ɹ�\n");
     }
     else
     {
     	  printf("������dcAnalogʧ��\n");
     }
  }
 #endif
}

/*******************************************************
��������:deleteData
��������:ɾ����������         
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void deleteData(INT8U type)
{
   sqlite3      *sqlite3Dbx;
   INT16U       i;
   INT8U        tmpFn;
   char         pSqlStr[512];          //SQL����ַ���ָ��
	 INT16U       result;
	 sqlite3_stmt *stat;
   DATE_TIME    tmpTime;
   char         tableName[30], tblNamex[30];
   char         *errMessage;           //������Ϣ(Error msg written her)
	 INT8U        leftPower[12];
	 struct clearQueue *pClrHead=NULL;
	 struct clearQueue *tmpNode, *prevNode;
	 
	#ifdef LIGHTING
	 INT8U        irData160[250];
	 INT8U 			  irData161[250];
	 INT8U 			  irData162[250];
	 INT8U 			  irData163[250];
	#endif

	 char   dataTable[][20]={"lastMonthData", "dayBalanceData","monthBalanceData", "eventRecord", 
	 	                     "statisData", "singlePhaseDay","singlePhaseMonth","dcAnalog"};
   char   paraTable[][20]={"base_info", "base_vice_info", "f10_info", ""};
   
   //1.Ϊ��drop����뽫ԭ��sqliteָ��ص�,�ٿ�һ������drop
   if (debugInfo&PRINT_DATA_BASE)
   {
   	 printf("Database-deleteData:Ϊdrop table���ر�sqlite3ȫ��ָ��sqlite3Db,��sqlite3Dbx�����ݿ�\n");
   }
   sqlite3_close(sqlite3Db);
   if (sqlite3_open("powerData.db", &sqlite3Dbx))
   {
     printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Dbx));
     sqlite3_close(sqlite3Dbx);
     
     //ly,2011-10-12,add,�򲻿��µ�ȫ��ָ��,��ԭΪԭȫ��ָ��
     usleep(2000);
     if (sqlite3_open("powerData.db", &sqlite3Db))
     {
       printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
       sqlite3_close(sqlite3Db);
       
       usleep(2000);
       if (sqlite3_open("powerData.db", &sqlite3Db))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
         sqlite3_close(sqlite3Db);
       }
     }
     
     return;
   }
   
	 //2.ɾ������������
	 for(i=0; i<8; i++)
	 {
	   //pSqlStr = sqlite3_mprintf("delete from %s;",dataTable[i]);
	   sprintf(pSqlStr, "delete from %s;", dataTable[i]);
	   result = sqlite3_exec(sqlite3Dbx, pSqlStr, 0, 0, NULL);
     if (result==SQLITE_OK)
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	      printf("ɾ����%s���ݳɹ�\n",dataTable[i]);
   	   }
     }
     else
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	      printf("ɾ����%s����ʧ��\n",dataTable[i]);
   	   }
     }
     //sqlite3_free(pSqlStr);
   }
   
   if (system("rm /data/*")==0)
   {
   	 printf("ɾ�����������ļ��ɹ�\n");
   }
   else
   {
   	 printf("ɾ�����������ļ�ʧ��\n");
   }
   
   //3.ɾ���������ز������ʵʱ���� drop table
   //3.1�������ݿ��е�mpxxxxxx,spxxxxxx,hfxxxx
   sprintf(pSqlStr,"select * from sqlite_master;");
	 sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stat, 0);
	 result = sqlite3_step(stat);
	 prevNode = pClrHead;
   while(result==SQLITE_ROW)
   {
		 strcpy(tableName, sqlite3_column_text(stat, 2));
		 
		 //printf("����=%s\n", tableName);
		 
		 if ((tableName[0]=='m' && tableName[1]=='p')
		 	   || (tableName[0]=='s' && tableName[1]=='p')
		 	    || (tableName[0]=='h' && tableName[1]=='f')
		 	  )
		 {
       tmpNode = (struct clearQueue *)malloc(sizeof(struct clearQueue));

       strcpy(tmpNode->tableName, tableName);
 
       tmpNode->next = NULL;
      
       if (pClrHead==NULL)
       {
      	 pClrHead = tmpNode;
       }
       else
       {
      	 prevNode->next = tmpNode;
       }
       prevNode = tmpNode;
     }
     
     result = sqlite3_step(stat);
   }
   sqlite3_finalize(stat);
   
   tmpNode = pClrHead;
   while(tmpNode!=NULL)
   {
	   sprintf(pSqlStr, "drop table %s;", tmpNode->tableName);
	   if (debugInfo&PRINT_DATA_BASE)
	   {
	     printf(pSqlStr);
	     printf("\n");
	   }
	    
	   result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
     if (result==SQLITE_OK)
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("ɾ����%s�ɹ�\n",tmpNode->tableName);
   	   }
     }
     else
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	      printf("ɾ����%sʧ��,Error:%s\n",tmpNode->tableName,sqlite3_errmsg(sqlite3Dbx));
   	   }
     }
     prevNode = tmpNode;
   	 tmpNode = tmpNode->next;
   	 free(prevNode);
   }

   /*
   tmpTime = sysTime;
   for(i=0;i<32;i++)
   {
	    //�����
	    strcpy(tableName, bringTableName(tmpTime,0));
	    //pSqlStr = sqlite3_mprintf("drop table %s;",tableName);
	    sprintf(pSqlStr, "drop table %s;", tableName);
	    if (debugInfo&PRINT_DATA_BASE)
	    {
	      printf(pSqlStr);
	      printf("\n");
	    }
	    
	    result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
      if (result==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("ɾ����%s�ɹ�\n",tableName);
   	    }
      }
      else
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("ɾ����%sʧ��,Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
   	    }
      }
      //sqlite3_free(pSqlStr);
      
      //�����
	    strcpy(tableName, bringTableName(tmpTime,1));
	    //pSqlStr = sqlite3_mprintf("drop table %s;",tableName);
	    sprintf(pSqlStr, "drop table %s;", tableName);
	    
	    if (debugInfo&PRINT_DATA_BASE)
	    {
	      printf(pSqlStr);
	      printf("\n");
	    }
	    
	    result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
      if (result==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("ɾ����%s�ɹ�\n",tableName);
   	    }
      }
      else
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("ɾ����%sʧ��,Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
   	    }
      }
      //sqlite3_free(pSqlStr);
      
      //���㶳���
	    strcpy(tableName, bringTableName(tmpTime,2));
	    //pSqlStr = sqlite3_mprintf("drop table %s;",tableName);
	    
	    sprintf(pSqlStr, "drop table %s;", tableName);
	    
	    if (debugInfo&PRINT_DATA_BASE)
	    {
	      printf(pSqlStr);
	      printf("\n");
	    }
	    
	    result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
      if (result==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("ɾ����%s�ɹ�\n",tableName);
   	    }
      }
      else
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("ɾ����%sʧ��,Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
   	    }
      }
      //sqlite3_free(pSqlStr);
      
   	  tmpTime = backTime(tmpTime, 0, 1, 0, 0, 0);
   }
   */
   
   //4.��ԭsqliteָ��Ϊsqlite3Db
   if (debugInfo&PRINT_DATA_BASE)
   {
   	 printf("Database-deleteData:��ԭsqlite3ȫ��ָ��\n");
   }
   sqlite3_close(sqlite3Dbx);
   if (sqlite3_open("powerData.db", &sqlite3Db))
   {
     printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
     sqlite3_close(sqlite3Db);
     
     usleep(2000);
     if (sqlite3_open("powerData.db", &sqlite3Db))
     {
       printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
       sqlite3_close(sqlite3Db);
       
       usleep(2000);
       if (sqlite3_open("powerData.db", &sqlite3Db))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
         sqlite3_close(sqlite3Db);
       }
     }
     
     return;
   }

   //5.ɾ���������м�ֵ,�����������ݵ��洢�ڲ����е�
   deleteParameter(88, 3);
   deleteParameter(88, 13);
   if (debugInfo&PRINT_PULSE_DEBUG)
   {
  	 printf("deleteData:ɾ���������м�ֵ\n");
   }
   
   //6.ɾ����������
   if (type==1)
   {
   	 //ר���ն�ɾ��ʣ�������
   	#ifdef LOAD_CTRL_MODULE
	     //pSqlStr = sqlite3_mprintf("delete from leftPower;");
	     
	     strcpy(pSqlStr, "delete from leftPower;");
	     result = sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL);
       if (result==SQLITE_OK)
       {
   	     if (debugInfo&PRINT_DATA_BASE)
   	     {
   	        printf("ɾ����leftPower���ݳɹ�\n");
   	     }
       }
       else
       {
   	     if (debugInfo&PRINT_DATA_BASE)
   	     {
   	        printf("ɾ����leftPower����ʧ��\n");
   	     }
       }
       //sqlite3_free(pSqlStr);
    #endif
		
		#ifdef LIGHTING
		 //��������ѧϰ����
     memset(irData160, 0x0, 5);
		 memset(irData161, 0x0, 5);
		 memset(irData162, 0x0, 5);
		 memset(irData163, 0x0, 5);
		 if (selectParameter(5, 160, irData160, 2)==TRUE)
		 {
		 	 printf("�����ĺ������俪���ݳ���=%d\n", irData160[0] | irData160[1]<<8);
		 	 selectParameter(5, 160, irData160, (irData160[0] | irData160[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("������������160ʧ��\n");
		 }
		 
		 if (selectParameter(5, 161, irData161, 2)==TRUE)
		 {
		 	 printf("�����ĺ������ȿ����ݳ���=%d\n", irData161[0] | irData161[1]<<8);
		 	 selectParameter(5, 161, irData161, (irData161[0] | irData161[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("������������161ʧ��\n");
		 }
		 
		 if (selectParameter(5, 162, irData162, 2)==TRUE)
		 {
		 	 printf("�����ĺ����ʪ�����ݳ���=%d\n", irData162[0] | irData162[1]<<8);
		 	 selectParameter(5, 162, irData162, (irData162[0] | irData162[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("������������162ʧ��\n");
		 }
		 
		 if (selectParameter(5, 163, irData163, 2)==TRUE)
		 {
		 	 printf("�����ĺ�������ݳ���=%d\n", irData163[0] | irData163[1]<<8);
		 	 selectParameter(5, 163, irData163, (irData163[0] | irData163[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("������������163ʧ��\n");
		 }
		#endif
   	 
   	 for(i=0;i<3;i++)
   	 {
   	   //pSqlStr = sqlite3_mprintf("delete from %s;",paraTable[i]);
   	   sprintf(pSqlStr, "delete from %s;",paraTable[i]);
   	   result  = sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL);
       if (result==SQLITE_OK)
       {
      	  if (debugInfo&PRINT_DATA_BASE)
      	  {
      	    printf("ɾ��������%s���ݳɹ�\n",paraTable[i]);
      	  }
       }
       else
       {
      	  if (debugInfo&PRINT_DATA_BASE)
      	  {
      	  	printf("ɾ��������%s����ʧ��\n",paraTable[i]);
      	  }
       }
       //sqlite3_free(pSqlStr);
     }
   }
   
   //8.������Ҫ����
   if (type==1)
   {
     if (debugInfo&PRINT_DATA_BASE)
     {
       printf("Database-deleteData::������Ҫ����\n");
     }

     //������Ҫ����
     saveParameter(0x04,   3, (INT8U *)&ipAndPort,sizeof(IP_AND_PORT));             //IP�Ͷ˿�
     saveParameter(0x04,   7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));       //�ն�IP��ַ�Ͷ˿ڵ�
     saveParameter(0x04,  16, (INT8U *)&vpn, sizeof(VPN));                          //VPN
     saveParameter(0x04,  37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));//��������
     saveParameter(0x04, 121, (INT8U *)&addrField, sizeof(ADDR_FIELD));             //��ַ������������
     saveParameter(0x04, 129, (INT8U *)&acSamplePara,sizeof(AC_SAMPLE_PARA));       //����У�����
     saveParameter(0x04, 133, mainNodeAddr, 6);
     saveParameter(0x04, 134, (INT8U *)&deviceNumber, 6);
     saveParameter(0x04, 136, (INT8U *)&csNameId, 12);

    #ifdef LIGHTING
     callAndReport = 0x01;    //����������Ĭ�������ն������ϱ�,2015-09-08
		 
		 //��������ѧϰ����
		 if ((irData160[0] | irData160[1]<<8)>0)
		 {
		 	 saveParameter(5, 160, irData160, (irData160[0] | irData160[1]<<8)+2);
			 printf("�������160����:");
			 for(i=0; i<(irData160[0] | irData160[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData160[i]);
			 }
			 printf("\n");
		 }
		 if ((irData161[0] | irData161[1]<<8)>0)
		 {
		 	 saveParameter(5, 161, irData161, (irData161[0] | irData161[1]<<8)+2);			 
			 printf("�������161����:");
			 for(i=0; i<(irData161[0] | irData161[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData161[i]);
			 }
			 printf("\n");
		 }
		 if ((irData162[0] | irData162[1]<<8)>0)
		 {
		 	 saveParameter(5, 162, irData162, (irData162[0] | irData162[1]<<8)+2);			 
			 
			 printf("�������162����:");
			 for(i=0; i<(irData162[0] | irData162[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData162[i]);
			 }
			 printf("\n");
		 }
		 if ((irData163[0] | irData163[1]<<8)>0)
		 {
		 	 saveParameter(5, 163, irData163, (irData163[0] | irData163[1]<<8)+2);
			 printf("�������163����:");
			 for(i=0; i<(irData163[0] | irData163[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData163[i]);
			 }
			 printf("\n");
		 }
		 
    #else
     callAndReport = 0x00;
    #endif
     saveParameter(0x05, 29, &callAndReport, 1);									                 //����F29
     
     saveParameter(88, 7,&lcdDegree,1);                                            //LCD�Աȶ�ֵ
   }

   eventReadedPointer[0]=0x0;                                                //�ն���Ҫ�¼��Ѷ�ָ��
   eventReadedPointer[1]=0x0;                                                //�ն���Ҫ�¼��Ѷ�ָ��   	  
   saveParameter(88, 2, eventReadedPointer, 2);                              //�ն���Ҫ�¼��Ѷ�ָ��
   iEventStartPtr = 0;
   iEventCounter  = 0;   
   
   
   //9.ר���ն˸��²ο�ʣ�����
   #ifdef LOAD_CTRL_MODULE
	  if (type==0)
	  {
     	for(i=1; i<=32; i++)
      {
     	  tmpTime = sysTime;
        if (readMeterData(leftPower, i, LEFT_POWER, 0x0, &tmpTime, 0)==TRUE)
     	  {
     	  	//�ο�ʣ�����B=A
     	  	leftPower[4] = leftPower[0];
     	  	leftPower[5] = leftPower[1];
     	  	leftPower[6] = leftPower[2];
     	  	leftPower[7] = leftPower[3];

     	  	//�����òο��ܼӵ�������0
     	  	leftPower[8] = 0;
     	  	leftPower[9] = 0;
     	  	leftPower[10] = 0;
     	  	leftPower[11] = 0;
     	  	
     	  	saveMeterData(i, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
   	      
   	      if (debugInfo&PRINT_DATA_BASE)
   	      {
   	        printf("�����ܼ���%d�ο�ʣ��������ο��ܼӵ�����\n", i);
   	      }
     	  }
     	}
	  }
	 #endif
	 
	 //2012-08-08,�ĵ���������չ���
	 //7.���մ洢�ռ�
   //2012-10-22,ȡ��ִ�л��մ洢�ռ乤��,��Ϊ�����ϱ������ǵ�ϵͳ���ʺ�ִ�и�����
   // SQLite FQA����˵����Linux�Ļ����£���Լ0.5��/M������Ҫʹ�����������ݿ��ļ��Ŀռ䡣
   // ���ǵ�ϵͳ�����ݿ��ļ�������100M����,���������̿ռ�Ϊ256M,OS���ļ�ռȥ15��20M,����������������
   // ������...
   //strcpy(pSqlStr, "vacuum;");
   //if (sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL)==SQLITE_OK)
   //{
   //  if (debugInfo&PRINT_DATA_BASE)
   //  {
   //     printf("���մ洢�ռ�ɹ�\n");
   //  }
   //}
   //else
   //{
   //   if (debugInfo&PRINT_DATA_BASE)
   //   {
   //     printf("���մ洢�ռ�ʧ��\n");
   //   }
   //}
}

/*******************************************************
��������:clearOutData
��������:�����������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void clearOutData(char * tableName, INT8U queryType, INT8U days, INT8U hasNo)
{
	 sqlite3_stmt *stat;
	 INT16U       i, result;
   char         *pSqlStr;        //SQL����ַ���ָ��
   char         *errMessage;     //������Ϣ(Error msg written her)
	 sqlite3_stmt *stmt;
	 const char   *tail;
	 int          execResult;
   DATE_TIME    tmpTime;
   struct timeval tv;           //Linux timeval

   if (hasNo==1)
   {
     if (queryType==MONTH_BALANCE)
     {
       tmpTime = backTime(sysTime, days, 0, 0, 0, 0);
     }
     else
     {
       tmpTime = backTime(sysTime, 0, days, 0, 0, 0);
     }
     
     getLinuxFormatDateTime(&tmpTime,&tv,1);
     
     pSqlStr = sqlite3_mprintf("delete from %s where time<%d;", tableName, tv.tv_sec);
     if (sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL)==SQLITE_OK)
     {
	      if (debugInfo&PRINT_DATA_BASE)
	      {
	        printf("clearOutData:ɾ����%s�������ݳɹ�\n", tableName);
	      }
     }
   }
   else
   {
     if (queryType==2)
     {
       tmpTime = timeHexToBcd(backTime(sysTime, 0, days, 0, 0, 0));
			#ifdef LIGHTING
       pSqlStr = sqlite3_mprintf("delete from %s where (day<>88 and day<>89 and day<>99 and (year<%d or (year=%d and month<%d) or (year=%d and month=%d and day<%d)))"\
	                   , tableName, tmpTime.year, tmpTime.year, tmpTime.month, tmpTime.year, tmpTime.month, tmpTime.day);
			#else
       pSqlStr = sqlite3_mprintf("delete from %s where (day<>88 and day<>99 and (year<%d or (year=%d and month<%d) or (year=%d and month=%d and day<%d)))"\
	                   , tableName, tmpTime.year, tmpTime.year, tmpTime.month, tmpTime.year, tmpTime.month, tmpTime.day);
			#endif
     }
     else
     {
       if (queryType==1)
       {
         tmpTime = backTime(sysTime, days, 0, 0, 0, 0);
       }
       else
       {
         tmpTime = backTime(sysTime, 0, days, 0, 0, 0);
       }
       getLinuxFormatDateTime(&tmpTime,&tv,1);

       pSqlStr = sqlite3_mprintf("delete from %s where copyTime<%d;",tableName, tv.tv_sec);
     }
     
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("clearOutData:ɾ����%s�������ݳɹ�\n",tableName);
   	   }
     }
     else
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("clearOutData:ɾ����%s��������ʧ��\n",tableName);
   	   }
     }
   }
   
   sqlite3_free(pSqlStr);
}

/*******************************************************
��������:saveBakDayFile
��������:�����ն������ݵ��ļ���
���ú���:
�����ú���:
�������:type =0x01,���������
              =0x0b,�����ʾֵ
              =0x0c,���������
�������:
����ֵ:void
*******************************************************/
BOOL deleteBakDayFile(INT16U pn, DATE_TIME freezeTime, INT8U type)
{
	FILE           *fp;
	char           fileName[20];
  struct timeval tv;                  //Linux timeval
  DATE_TIME      tmpTime;
  SP_F_DAY       tmpSpfDay;
  MP_F_DAY       tmpMpfDay;
  INT16U         tmpCount;
	
	//���������ļ���
	if (type==1)
	{
	  sprintf(fileName, "/data/spd%02d%02d%02d", freezeTime.year, freezeTime.month, freezeTime.day);
	}
	else
	{
	  sprintf(fileName, "/data/mpd%02d%02d%02d", freezeTime.year, freezeTime.month, freezeTime.day);
	}

  //�鿴Ŀ¼/�ļ��Ƿ����
  if(access(fileName, F_OK) != 0)
  {
    return FALSE;    //������,������ɾ������
  }

  if((fp=fopen(fileName, "rb+"))==NULL)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
  	  printf("deleteBakDayFile:���ļ�%sʧ��\n", fileName);
  	}
  	
  	return FALSE;
  }

  tmpCount = 0;
  while (!feof(fp))
  {
  	if (type==1)
  	{
    	if (fread(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0, 2);    //���ļ�β
    	  
    	  return FALSE;
    	}
      
      if (tmpSpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("delteBakDayFile:���Ե�%d���ݴ���\n", pn);
      	}
      	
      	//��λ������¼
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(SP_F_DAY), 0);
      	break;
      }
    }
    else
    {
    	if (fread(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0 ,2);    //���ļ�β
    	  
    	  return FALSE;
    	}
      
      if (tmpMpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("delteBakDayFile:���Ե�%d���ݴ���\n", pn);
      	}
      	
      	//��λ������¼
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(MP_F_DAY), 0);
      	break;
      }
    }

  	tmpCount++;
  }
  
  if (type==1)
  {
    tmpSpfDay.pn = 0xeeee;
    if (fwrite(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("delteBakDayFile:�ļ�%sɾ������ʧ��\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  else
  {
    tmpMpfDay.pn = 0xeeee;
    if (fwrite(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("delteBakDayFile:�ļ�%sɾ������ʧ��\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  
  if (debugInfo&PRINT_DATA_BASE)
  {
    printf("delteBakDayFile:�ļ�%sɾ���������\n", fileName);
  }
  
  fclose(fp);
  return TRUE;
}

/*******************************************************
��������:deleteFreezeData
��������:ɾ�������㶳������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void deleteFreezeData(INT16U pn)
{
	sqlite3_stmt *stat;
	INT8S        *sql;
  DATE_TIME    deleteTime;
  INT8U        i;
  
  sql = sqlite3_mprintf("delete from singlePhaseDay where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("ɾ��singlePhaseDay�������%d���ݳɹ�\n", pn);
    }
  }
  sqlite3_free(sql);

  sql = sqlite3_mprintf("delete from singlePhaseMonth where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("ɾ��singlePhaseMonth�������%d���ݳɹ�\n", pn);
    }
  }
  sqlite3_free(sql);

  sql = sqlite3_mprintf("delete from dayBalanceData where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("ɾ��dayBalanceData�������%d���ݳɹ�\n", pn);
    }
  }
  sqlite3_free(sql);

  sql = sqlite3_mprintf("delete from monthBalanceData where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("ɾ��monthBalanceData�������%d���ݳɹ�\n", pn);
    }
  }
  sqlite3_free(sql);
  
  deleteTime = sysTime;
  for(i=0; i<15; i++)
  {
    deleteTime = backTime(sysTime, 0, 1, 0, 0, 0);
    
    deleteBakDayFile(pn, sysTime, 0x1);
    deleteBakDayFile(pn, sysTime, 0xb);
    deleteBakDayFile(pn, sysTime, 0xc);
  }
}

#ifdef LIGHTING 

/***************************************************
��������:initCtrlTimesLink
��������:��ʼ������ʱ������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void

��ʷ��
    1.
***************************************************/
void initCtrlTimesLink(void)
{
	 struct ctrlTimes *tmpNode, *prevNode;
	 sqlite3_stmt     *stmt;
	 char             *sql;
	 INT16U           result;
	 INT8U            tmpAfn1, tmpAfn2, tmpAfn5, tmpAfn7;
	 INT8U            i;
	 
	 //�ͷ�����
	 while(cTimesHead!=NULL)
	 {
	 	 tmpNode = cTimesHead->next;
	 	 
	 	 free(cTimesHead);
	 	 
	 	 cTimesHead = tmpNode;
	 }
	 
	 cTimesHead  = NULL;
	 prevNode = cTimesHead;
	 
	 //��ѯ
	 sql = "select * from base_vice_info where (acc_afn=? or acc_afn=? or acc_afn=? or acc_afn=?) order by acc_fn, acc_pn, acc_afn";
	 sqlite3_prepare(sqlite3Db, sql, -1, &stmt, 0);
	 tmpAfn1 = 1;
	 sqlite3_bind_blob(stmt, 1, &tmpAfn1, 1, NULL);
	 tmpAfn2 = 2;
	 sqlite3_bind_blob(stmt, 2, &tmpAfn2, 1, NULL);
	 tmpAfn5 = 5;
	 sqlite3_bind_blob(stmt, 3, &tmpAfn5, 1, NULL);
	 tmpAfn7 = 7;
	 sqlite3_bind_blob(stmt, 4, &tmpAfn7, 1, NULL);

	 result = sqlite3_step(stmt);
   while(result==SQLITE_ROW)
   {
     tmpNode = (struct ctrlTimes *)malloc(sizeof(struct ctrlTimes));

		 memcpy(tmpNode, sqlite3_column_blob(stmt, 3), sizeof(struct ctrlTimes));

     tmpNode->next = NULL;
         
     if (cTimesHead==NULL)
     {
       cTimesHead = tmpNode;
     }
     else
     {
       prevNode->next = tmpNode;
     }
     prevNode = tmpNode;
            
     result = sqlite3_step(stmt);
   }
   sqlite3_finalize(stmt);
   
   if (debugInfo&PRINT_DATA_BASE)
   {
     prevNode = cTimesHead;
     while(prevNode!=NULL)
     {
     	 switch (prevNode->deviceType)
     	 {
     	 	 case 2:
     	 	   printf("��·������,");
     	 	   break;

     	 	 case 5:
     	 	   printf("��γ�ȿ���,");
     	 	   break;

     	 	 case 7:
     	 	   printf("  �նȿ���,");
     	 	   break;
     	 	   
     	 	 default:
     	 	   printf("���ƿ�����,");
     	 	   break;
     	 }
     	 printf("%02d-%02d��%02d-%02d,", prevNode->startMonth, prevNode->startDay, prevNode->endMonth, prevNode->endDay);
     	 printf("ʱ��%d,������=%02x,", prevNode->noOfTime, prevNode->workDay);
     	 for(i=0; i<6; i++)
     	 {
     	   printf("%d)%02x:%02x-%d%% ", i+1,prevNode->hour[i], prevNode->min[i], prevNode->bright[i]);
     	 }
     	 printf("\n");
     	 
     	 prevNode = prevNode->next;
     }
   }
}

/*******************************************************
��������:deleteCtimes
��������:����ɾ������ʱ������(������Ϣ����)
���ú���:
�����ú���:
�������:afn  fn
�������:
����ֵ��void
*******************************************************/
void deleteCtimes(void)
{
	sqlite3_stmt *stat;
	INT8S        *sql;
	INT8U        tmpAfn1, tmpAfn2, tmpAfn5, tmpAfn7;
	
	//ɾ����Ӧ����
	sql = "delete from base_vice_info where (acc_afn = ? or acc_afn = ? or acc_afn = ? or acc_afn = ?);";	
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	tmpAfn1 = 1;
	sqlite3_bind_blob(stat, 1, &tmpAfn1, 1, NULL);
	tmpAfn2 = 2;
	sqlite3_bind_blob(stat, 2, &tmpAfn2, 1, NULL);
	tmpAfn5 = 5;    //2016-10-28,Add
	sqlite3_bind_blob(stat, 3, &tmpAfn5, 1, NULL);
	tmpAfn7 = 7;
	sqlite3_bind_blob(stat, 4, &tmpAfn7, 1, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

#endif

/*******************************************************
��������:loadParameter
��������:��������ʱ��ʼ������
���ú���:
�����ú���:
�������:
�������:
����ֵ��void

��ʷ:
    1)2012-08-09,add,�ؼ���¼����(�ն˵�ַ����վIP��ַ�Ͷ˿ڡ�VPN�û�������)�����ڶ����ļ�,��Щ�����ļ�
                 ֻ�����ò���ʱ�޸�,����ʱ�䲻��,һ�����ݿ��еĲ��������޸�ʱ,ʹ�ñ��ݶ����ļ��еĲ���
    2)2012-09-26,add,У�����Ҳ���ؼ��������ݴ���
*******************************************************/
void loadParameter(void)
{
	 sqlite3_stmt   *stat;
	 INT16U         i, result, j;
   char           *pSqlStr;              //SQL����ַ���ָ��
   char           *errMessage;           //������Ϣ(Error msg written her)
   INT32U         tmpAddr;
   ADC_PARA       adcPara;               //ֱ��ģ�������ò���(AFN04-FN81,FN82,FN83)
   
   ADDR_FIELD     bakAddrField;          //bak��ַ��
   VPN            bakVpn;                //bak����ר���û���������(AFN04-FN16)
   IP_AND_PORT    bakIpAndPort;          //bak��վIP��ַ�Ͷ˿�(AFN04-FN03)
   AC_SAMPLE_PARA bakAcPara;             //bak����У��ֵ
   INT8U          *pAcPara, *pAcParax;   //�ȽϽ���У��ֵָ��
   INT8U          foundAcPara=0;
   

   //0.����״̬����
   memset(paraStatus, 0x0, 31);

	 //1.�������в���,�ؼ�����û�еĻ�����ֵ****************************************************
	 //F121������������ն˵�ַ
	 if (selectParameter(0x04, 121, (INT8U *)&addrField, sizeof(ADDR_FIELD))==FALSE)	   			  //F1
	 {
	   //��������ն˵�ַ�ļ�����
	   if(access("/keyPara121", F_OK) == 0)
	   {
	   	 readBakKeyPara(121, &addrField);
	   	 
	   	 printf("���ݿ������ն˵�ַ,�����ն˵�ַ����,ʹ�ñ����ն˵�ַ\n");
	   }
	   else
	   {
	     //����Ĭ��ֵ
	     addrField.a1[0] = 0x00;
	     addrField.a1[1] = 0x50;
	     addrField.a2[0] = 0x01;  //0001
	     addrField.a2[1] = 0x00;
	     
	     saveBakKeyPara(121);
	   }
	   
	   //����F121
	   saveParameter(0x04, 121, (INT8U *)&addrField, sizeof(ADDR_FIELD));
	   
	   printf("����������������ն˵�ַ��ʼֵ\n");
	 }
	 else
	 {
	   //��������ն˵�ַ�ļ�������
	   if(access("/keyPara121", F_OK) != 0)
	   {
	     saveBakKeyPara(121);
     }
   }
   
   readBakKeyPara(121, &bakAddrField);
   printf("�����ն˵�ַ:��������=%02x%02x,�ն˵�ַ=%d\n", bakAddrField.a1[1],bakAddrField.a1[0], bakAddrField.a2[1]<<8 | bakAddrField.a2[0]);
   
   if (addrField.a1[0]!=bakAddrField.a1[0] || addrField.a1[1]!=bakAddrField.a1[1]
   	   || addrField.a2[0]!=bakAddrField.a2[0] || addrField.a2[1]!=bakAddrField.a2[1]
   	  )
   {
	   readBakKeyPara(121, &addrField);
	   	 
	   printf("�ն˵�ַ�뱸���ն˵�ַ����ͬ,ʹ�ñ����ն˵�ַ\n");
   }

   //F1�ն�ͨ�Ų���
	 if (selectParameter(0x04, 1, (INT8U *)&commPara, sizeof(COMM_PARA))==FALSE)    //F1
	 {
    #ifdef SDDL_CSM    //2013-11-20,����ɽ��������˾Ӫ����֪ͨ��Ĭ��ֵ����
     commPara.rts = 0;
     commPara.delay = 0;
   
     commPara.timeOutReSendTimes[0] = 0x00;
     commPara.timeOutReSendTimes[1] = 0x00;
   
     commPara.flagOfCon = 0x07;
     commPara.heartBeat = 10;
    #else
     commPara.rts = 22;
     commPara.delay = 20;
   
     commPara.timeOutReSendTimes[0] = 0x0d;
     commPara.timeOutReSendTimes[1] = 0x30;
   
     commPara.flagOfCon = 0;
     
     #ifdef LIGHTING
      commPara.heartBeat = 30;    //2016-07-04,����������Ĭ������ֵ�ĳ�30��
     #else
      #ifdef CQDL_CSM
       commPara.heartBeat = 10;
      #else
       #ifdef DKY_SUBMISSION
        commPara.heartBeat = 1;  //ly,2011-08-25,��Ĭ�������ĳ�1����
       #else
        commPara.heartBeat = 5;
       #endif
      #endif
     #endif
	  #endif

	   saveParameter(0x04, 1, (INT8U *)&commPara, sizeof(COMM_PARA));
	   
	   printf("�����ն�ͨ�Ų�����ʼֵ\n");
	 }
	 else    //2012-08-02,add "else process"
	 {
	 	 if (commPara.heartBeat<1)
	 	 {
      #ifdef LIGHTING
       commPara.heartBeat = 30;    //2016-07-04,����������Ĭ������ֵ�ĳ�30��
      #else
     	 commPara.heartBeat = 1;
     	#endif
	 	 }
	 }
	 
	 //F2
	 if (selectParameter(0x04, 2, (INT8U *)&relayConfig, sizeof(RELAY_CONFIG))==TRUE)
	 {
  	 ;
	 }
	 
	 //F3.��վIP�Ͷ˿�
	 if (selectParameter(0x04, 3, (INT8U *)&ipAndPort, sizeof(IP_AND_PORT))==FALSE)
	 {
		 //���������վIP��ַ�ļ�����
	   if(access("/keyPara003", F_OK) == 0)
	   {
	   	 readBakKeyPara(3, &ipAndPort);
	   	 
	   	 printf("���ݿ�������վIP��ַ,������վIP��ַ����,ʹ�ñ�����վIP��ַ\n");
	   }
	   else
  	 {
  		 //Ĭ��ֵ
  		 ipAndPort.ipAddr[0] = 222;
  		 ipAndPort.ipAddr[1] = 178;
  		 ipAndPort.ipAddr[2] =  86;	
  		 ipAndPort.ipAddr[3] =  65;
  		 ipAndPort.port[0] = 0x29;
  		 ipAndPort.port[1] = 0x23;
  		 //����IP�Ͷ˿�
  		 ipAndPort.ipAddrBak[0] = 222;
  		 ipAndPort.ipAddrBak[1] = 178;
  		 ipAndPort.ipAddrBak[2] = 86;
  		 ipAndPort.ipAddrBak[3] = 65;
  		 ipAndPort.portBak[0] = 0x29;
  		 ipAndPort.portBak[1] = 0x23;
  		
  		 strcpy((char *)ipAndPort.apn,"CMNET");
  		 
  		 saveBakKeyPara(3);
  	 }
		
		 //����F3
		 saveParameter(0x04, 3, (INT8U *)&ipAndPort, sizeof(IP_AND_PORT));
		 
		 printf("������վIP��ַ�Ͷ˿ڳ�ʼֵ\n");
	 }
	 else
	 {
	   if(access("/keyPara003", F_OK) != 0)  //������
	   {
	   	 saveBakKeyPara(3);
	   }
	 }

   readBakKeyPara(3, &bakIpAndPort);
   printf("������IP=%03d.%03d.%03d.%03d,���˿�=%d,��IP=%03d.%03d.%03d.%03d,���˿�=%d,APN=%s\n", 
            bakIpAndPort.ipAddr[0], bakIpAndPort.ipAddr[1], bakIpAndPort.ipAddr[2], bakIpAndPort.ipAddr[3],
            bakIpAndPort.port[1]<<8 | bakIpAndPort.port[0],
            bakIpAndPort.ipAddrBak[0], bakIpAndPort.ipAddrBak[1], bakIpAndPort.ipAddrBak[2], bakIpAndPort.ipAddrBak[3],
            bakIpAndPort.portBak[1]<<8 | bakIpAndPort.portBak[0],
            (char *)bakIpAndPort.apn
            );
   
   if (ipAndPort.ipAddr[0]!=bakIpAndPort.ipAddr[0] || ipAndPort.ipAddr[1]!=bakIpAndPort.ipAddr[1] || ipAndPort.ipAddr[2]!=bakIpAndPort.ipAddr[2] || ipAndPort.ipAddr[3]!=bakIpAndPort.ipAddr[3]
       || ipAndPort.ipAddrBak[0]!=bakIpAndPort.ipAddrBak[0] || ipAndPort.ipAddrBak[1]!=bakIpAndPort.ipAddrBak[1] || ipAndPort.ipAddrBak[2]!=bakIpAndPort.ipAddrBak[2] || ipAndPort.ipAddrBak[3]!=bakIpAndPort.ipAddrBak[3]
   	    || ipAndPort.port[0] != bakIpAndPort.port[0] || ipAndPort.port[1] != bakIpAndPort.port[1] 
   	     || ipAndPort.portBak[0] != bakIpAndPort.portBak[0] || ipAndPort.portBak[1] != bakIpAndPort.portBak[1] 
   	      || strcmp((char *)&ipAndPort.apn,(char *)&bakIpAndPort.apn)!=0
   	   )
   {
   	 readBakKeyPara(3, &ipAndPort);
	   	 
	   printf("��վIP��ַ�뱸����վIP��ַ����ͬ,ʹ�ñ�����վIP��ַ\n");
   }
   
	 //F4
	 if (selectParameter(0x04, 4, (INT8U *)&phoneAndSmsNumber, sizeof(PHONE_AND_SMS))==TRUE)
	 {
  	 ;
	 }
	 
	 //F5
	 if (selectParameter(0x04, 5, (INT8U *)messageAuth, 3)==TRUE)
	 {
  	 ;
	 }
	 
	 //F6
	 if (selectParameter(0x04, 6, (INT8U *)groupAddr, sizeof(groupAddr))==TRUE)
	 {
  	 ;
	 }
	
	 //F7.�ն�IP��ַ���˿�
	 if (FALSE==selectParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT)))
	 {
	   //��������ն�IP��ַ���˿��ļ�����,2020-11-18,Add
	   if(access("/keyPara007", F_OK) == 0)
	   {
	   	 readBakKeyPara(7, &teIpAndPort.ethIfLoginMs);
	   	 
	   	 printf("���ݿ������ն�IP��ַ���˿�,�����ն�IP��ַ���˿��ļ�����,ʹ�ñ����ն�IP��ַ�Ͷ˿�\n");
	   }
	   else
	   {
	   	#ifdef PLUG_IN_CARRIER_MODULE
	     //����Ĭ��ֵ,2020-11-18,Add
	     teIpAndPort.ethIfLoginMs = 0x55;
			 printf("����Ĭ��ֵ:��̫����¼��վ\n");
			#endif
	   }
	 }
	 else
	 {
	   //��������ն�IP��ַ���˿��ļ�������,2020-11-18,Add
	   if(access("/keyPara007", F_OK) != 0)
	   {
	     saveBakKeyPara(7);
     }
   }
	 if (teIpAndPort.teIpAddr[0]==0x0)
	 {
		 //�ն�IP��ַ�����������ַ�����ص�ַ��rcS�ļ��е�ֵ
	   readIpMaskGateway(teIpAndPort.teIpAddr, teIpAndPort.mask, teIpAndPort.gateWay);

		 //��������
		 teIpAndPort.proxyType = 0;   //��ʹ�ô���
		 //�����������ַ
		 teIpAndPort.proxyServer[0] = 0;
		 teIpAndPort.proxyServer[1] = 0;
		 teIpAndPort.proxyServer[2] = 0;
		 teIpAndPort.proxyServer[3] = 0;
		 //����������˿�
		 teIpAndPort.proxyPort[0] = 0;
		 teIpAndPort.proxyPort[1] = 0;
		 
		 //������������ӷ�ʽ
		 teIpAndPort.proxyLinkType = 0;		 
     //�û�������
     teIpAndPort.userNameLen = 0;   //�û�������m     
     //�û���
     memset(teIpAndPort.userName,0x0,20);    
     //���볤��n
     teIpAndPort.passwordLen = 0;   //�û�������m
     //����
     memset(teIpAndPort.password,0x0,20);     
     //�ն������˿�6412
     teIpAndPort.listenPort[0] = 0x0c;
     teIpAndPort.listenPort[1] = 0x19;

		 //2020-11-18,�޸�
		 if (0x55!=teIpAndPort.ethIfLoginMs)
		 {
     	 teIpAndPort.ethIfLoginMs = 0x00;
		 }
   
		 saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));
		 
		 printf("�����ն�IP��ַ�Ͷ˿ڳ�ʼֵ\n");
	 }
	 readIpMaskGateway(teIpAndPort.teIpAddr, teIpAndPort.mask, teIpAndPort.gateWay);

	 //F8.�ն�����ͨ�Ź�����ʽ
	 if (selectParameter(0x04, 8, (INT8U *)&tePrivateNetMethod, sizeof(PRIVATE_NET_METHOD))==TRUE)
	 {
  	 ;
	 }
	 else
	 {
	 	 tePrivateNetMethod.workMethod = 0x11;
	 	 tePrivateNetMethod.redialInterval[0] = 20;
	 	 tePrivateNetMethod.redialInterval[1] = 0;
	 }
	
	 //F9.�ն��¼���¼��������
	 if (selectParameter(0x04, 9, (INT8U *)&eventRecordConfig, sizeof(EVENT_RECORD_CONFIG))==FALSE)
	 {
    	for(i=0;i<8;i++)
    	{
    	  eventRecordConfig.nEvent[i] = 0x0;
    	  eventRecordConfig.iEvent[i] = 0x0;
    	}

     #ifdef SDDL_CSM
      #ifdef PLUG_IN_CARRIER_MODULE    //������
    	 eventRecordConfig.nEvent[0] = 0xc7;
       eventRecordConfig.nEvent[1] = 0x7f;
       eventRecordConfig.nEvent[2] = 0xfd;
       eventRecordConfig.nEvent[3] = 0xff;
       eventRecordConfig.nEvent[4] = 0xff;
       eventRecordConfig.nEvent[5] = 0x01;
       
       eventRecordConfig.iEvent[0] = 0xc2;
       eventRecordConfig.iEvent[1] = 0x38;
       eventRecordConfig.iEvent[2] = 0x10;
       eventRecordConfig.iEvent[3] = 0x7c;
       eventRecordConfig.iEvent[4] = 0x90;
       eventRecordConfig.iEvent[5] = 0x01;
      #else
    	 eventRecordConfig.nEvent[0] = 0xff;
       eventRecordConfig.nEvent[1] = 0x7f;
       eventRecordConfig.nEvent[2] = 0xfd;
       eventRecordConfig.nEvent[3] = 0xff;
       eventRecordConfig.nEvent[4] = 0xff;
       eventRecordConfig.nEvent[5] = 0x01;
       
       eventRecordConfig.iEvent[0] = 0xc2;
       eventRecordConfig.iEvent[1] = 0x3f;
       eventRecordConfig.iEvent[2] = 0x10;
       eventRecordConfig.iEvent[3] = 0x7c;
       eventRecordConfig.iEvent[4] = 0x90;
       eventRecordConfig.iEvent[5] = 0x01;
      #endif      
     #else
    	eventRecordConfig.nEvent[0] = 0x00;
      eventRecordConfig.nEvent[1] = 0x10;
      eventRecordConfig.nEvent[2] = 0x80;
      eventRecordConfig.nEvent[3] = 0x01;
      eventRecordConfig.nEvent[4] = 0x00;
      //eventRecordConfig.iEvent[0] = 0xf5; //11110101
      //ly,2011-11-21,�ĳɵ��������,��Ϊ��ɽ������,������������ʱ,�����¼����ݵĻ�,����ϵͳ�Ͳ��ܵõ�ȷ��
      //����ר���v1.6�����������Ի�û�и�����ط�
      eventRecordConfig.iEvent[0] = 0xf1; //11110001
      eventRecordConfig.iEvent[1] = 0x3a; //00111010
      eventRecordConfig.iEvent[2] = 0x9d; //10011101
      eventRecordConfig.iEvent[3] = 0xbf; //10111111
      eventRecordConfig.iEvent[4] = 0x08; //00001000
     #endif
      
 	   saveParameter(0x04, 9, (INT8U *)&eventRecordConfig, 16);
 	    
 	   printf("�����ն��¼���¼���ó�ʼֵ\n");
	 }
	 
	 if (selectParameter(0x04, 10, (INT8U *)&meterDeviceNum, 2)==TRUE)
	 {
  	 ;
	 } 		 	
   countParameter (0x04, 10, &meterDeviceNum);
 	 
 	 //F12.�ն�״̬���������
 	 if (selectParameter(0x04, 12, (INT8U *)statusInput, 2)==TRUE)
 	 {
  	 ;
 	 }
   
   //F11.�ն��������ò���
	 if (selectParameter(0x04, 11, (INT8U *)&pulseConfig, sizeof(PULSE_CONFIG))==FALSE)
	 {
  	 pulseConfig.numOfPulse = 0;
	 }
	 
	 //F13�ն˵�ѹ/����ģ�������ò���
	 if (selectParameter(0x04, 13, (INT8U *)&simuIUConfig, sizeof(IU_SIMULATE_CONFIG))==TRUE)
	 {
	 	 ;
	 }
	 
	 //F14.�ն��ܼ������ò���
	 if (selectParameter(0x04, 14, (INT8U *)&totalAddGroup, sizeof(TOTAL_ADD_GROUP))==FALSE)
	 {
	 	 totalAddGroup.numberOfzjz = 0;
	 }
	 
	 selectParameter(0x04, 15, (INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));//F15
	 
	 //F16
	 if (selectParameter(0x04, 16, (INT8U *)&vpn, sizeof(VPN))==FALSE)
	 {
		 //�������VPN�ļ�����
	   if(access("/keyPara016", F_OK) == 0)
	   {
	   	 readBakKeyPara(16, &vpn);
	   	 
	   	 printf("���ݿ�����VPN����,����VPN��������,ʹ�ñ���VPN����\n");
	   }
	   else
  	 {
  	 	 strcpy((char *)&vpn.vpnName, "card");
  	 	 strcpy((char *)&vpn.vpnPassword, "card");
  	 	 
  	 	 saveBakKeyPara(16);
  	 }
  	 	 
		 //����F16
		 saveParameter(0x04, 16, (INT8U *)&vpn, sizeof(VPN));
		   
		 printf("����VPN��ʼֵ\n");
	 }
	 else
	 {
	   if(access("/keyPara016", F_OK) != 0)    //�������ļ�
	   {
	   	 saveBakKeyPara(16);
	   }
	 }
	 
	 readBakKeyPara(16, &bakVpn);
	 printf("����VPN:username=%s,password=%s\n", (char *)bakVpn.vpnName,(char *)bakVpn.vpnPassword);
	 
	 if (strcmp((char *)&vpn.vpnName,(char *)&bakVpn.vpnName)!=0
	 	   || strcmp((char *)&vpn.vpnPassword,(char *)&bakVpn.vpnPassword)!=0
	 	  )
	 {
	 	 readBakKeyPara(16, &vpn);
	 	 
	 	 printf("VPN�����뱸��VPN��������ͬ,ʹ�ñ���VPN����\n");
	 }

   printf("*****************************\n");
   printf("��ǰ�ؼ���¼����:\n");
   printf("    1)�ն˵�ַ:��������=%02x%02x,�ն˵�ַ=%d\n", addrField.a1[1],addrField.a1[0], addrField.a2[1]<<8 | addrField.a2[0]);
   printf("    2)��IP=%03d.%03d.%03d.%03d,���˿�=%d,��IP=%03d.%03d.%03d.%03d,���˿�=%d,APN=%s\n", 
            ipAndPort.ipAddr[0], ipAndPort.ipAddr[1], ipAndPort.ipAddr[2], ipAndPort.ipAddr[3],
            ipAndPort.port[1]<<8 | ipAndPort.port[0],
            ipAndPort.ipAddrBak[0], ipAndPort.ipAddrBak[1], ipAndPort.ipAddrBak[2], ipAndPort.ipAddrBak[3],
            ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],
            (char *)ipAndPort.apn
            );
	 printf("    3)VPN:�û���=%s,����=%s\n", (char *)bakVpn.vpnName,(char *)bakVpn.vpnPassword);
	 printf("    4)ʹ����̫����¼��վ:%X\n", teIpAndPort.ethIfLoginMs);
   printf("*****************************\n");


	 //��3
	 selectParameter(0x04, 17, (INT8U *)protectLimit, 2);																	   //F17
   selectParameter(0x04, 18, (INT8U *)&ctrlPara, sizeof(CONTRL_PARA));                     //F18,F19,F20

	 selectParameter(0x04, 23, (INT8U *)chargeAlarm, 3);																	   //F23

	 //F21.�ն˵���������ʱ�κͷ�����
	 if (selectParameter(0x04, 21, (INT8U *)periodTimeOfCharge, 49)==FALSE)
	 {
	 	 //F21�ն˵���������ʱ�κͷ������ó�ֵ
	 	 periodTimeOfCharge[48] = 0x4;
	 	 for(i=0;i<48;i++)
	 	 {
	 	 	 periodTimeOfCharge[i] = i/12;
	 	 }
	 }
	 
	 //F22.�ն˵���������
	 selectParameter(0x04, 22, (INT8U *)&chargeRateNum, sizeof(CHARGE_RATE_NUM));
	 
	 //F33.�ն˳������в�������
	 if (selectParameter(0x04, 33, (INT8U *)&teCopyRunPara, sizeof(TE_COPY_RUN_PARA))==FALSE)
	 {
      //Ĭ��ֵ
      teCopyRunPara.numOfPara = NUM_OF_COPY_METER;
      
     #ifdef SDDL_CSM
      for(i=0;i<teCopyRunPara.numOfPara;i++)
      {
      	//�ն�ͨ�Ŷ˿ں�
      	if (i==4)
      	{
      	  teCopyRunPara.para[i].commucationPort = 31;
      	}
      	else
      	{
      	  teCopyRunPara.para[i].commucationPort = i+1;
      	}
  			
  			//̨�����г������п�����(�������״̬��,����Ѱ�������,Ҫ��Ե��Уʱ,���㲥����Уʱ,�����б�,��ʱ�γ���)
      	teCopyRunPara.para[i].copyRunControl[0] = 0x28;
      	teCopyRunPara.para[i].copyRunControl[1] = 0x00;	
      	
      	//������-����(1��)
      	teCopyRunPara.para[i].copyDay[0] = 0x01;
      	teCopyRunPara.para[i].copyDay[1] = 0x00;
      	teCopyRunPara.para[i].copyDay[2] = 0x00;
      	teCopyRunPara.para[i].copyDay[3] = 0x00;
      	
      	//������-ʱ��(0ʱ0��)
      	teCopyRunPara.para[i].copyTime[0] = 0x0;
      	teCopyRunPara.para[i].copyTime[1] = 0x0;
      	
  	    //������ʱ��Ĭ��ֵ
  	    if (i==4)  //�ز�������Ϊ60
  	    {
  	      teCopyRunPara.para[i].copyInterval = 60;
  	    }
  	    else       //485�ӿ�Ϊ5����
  	    {
  	     #ifdef PLUG_IN_CARRIER_MODULE
  	      teCopyRunPara.para[i].copyInterval = 60;
  	     #else
  	      teCopyRunPara.para[i].copyInterval = 30;
  	     #endif
  	    }
      	
      	//�㲥Уʱ��ʱʱ��
      	teCopyRunPara.para[i].broadcastCheckTime[0] = 0x40;
      	teCopyRunPara.para[i].broadcastCheckTime[1] = 0x16;
      	teCopyRunPara.para[i].broadcastCheckTime[2] = 0x00;    //ÿ��Уʱ
      	
      	teCopyRunPara.para[i].hourPeriodNum = 1;							//������ʱ����
      	
      	teCopyRunPara.para[i].hourPeriod[0][0] = 0x00;
      	teCopyRunPara.para[i].hourPeriod[0][1] = 0x00;
      	teCopyRunPara.para[i].hourPeriod[1][0] = 0x59;
      	teCopyRunPara.para[i].hourPeriod[1][1] = 0x23;
      }
     #else
      for(i=0;i<teCopyRunPara.numOfPara;i++)
      {
      	//�ն�ͨ�Ŷ˿ں�
      	if (i==4)  //2012-3-27,��3�ĳ�4
      	{
      	  teCopyRunPara.para[i].commucationPort = 31;
      	}
      	else
      	{
      	  teCopyRunPara.para[i].commucationPort = i+1;
      	}
  			
  			//̨�����г������п�����(�������״̬��,��Ѱ�������,���Ե��Уʱ,���㲥����Уʱ,�����б�,��ʱ�γ���)
      	teCopyRunPara.para[i].copyRunControl[0] = 0x30;
      	teCopyRunPara.para[i].copyRunControl[1] = 0x00;	
      	
      	//������-����(1��)
      	teCopyRunPara.para[i].copyDay[0] = 0x01;
      	teCopyRunPara.para[i].copyDay[1] = 0x00;
      	teCopyRunPara.para[i].copyDay[2] = 0x00;
      	teCopyRunPara.para[i].copyDay[3] = 0x00;
      	
      	//������-ʱ��(0ʱ0��)
      	teCopyRunPara.para[i].copyTime[0] = 0x0;
      	teCopyRunPara.para[i].copyTime[1] = 0x0;
      	
  	    //������ʱ��Ĭ��ֵ
  	    if (i==4)  //�ز�������Ϊ20  ,2012-3-27,��3�ĳ�4
  	    {
  	      teCopyRunPara.para[i].copyInterval = 20;
  	    }
  	    else       //485�ӿ�Ϊ5����
  	    {
  	     #ifdef LIGHTING    //����������485��Ĭ��Ϊ20��
  	      //teCopyRunPara.para[i].copyInterval = 1;
  	      teCopyRunPara.para[i].copyInterval = 2;   //2016-06-02,�޸�Ϊ��λΪ*10�룬2��Ϊ20��
  	     #else
  	      teCopyRunPara.para[i].copyInterval = 5;
  	     #endif
  	    }
      	
      	//�㲥Уʱ��ʱʱ��
      	teCopyRunPara.para[i].broadcastCheckTime[0] = 0x0;
      	teCopyRunPara.para[i].broadcastCheckTime[1] = 0x0;
      	teCopyRunPara.para[i].broadcastCheckTime[2] = 0x0;
      	
      	teCopyRunPara.para[i].hourPeriodNum = 1;							//������ʱ����
      	
				//2016-09-08,�ز��˿ڵ�ÿ����ʼ����ʱ���00:01-23:59�ĵ�00:10-23:55
				if (4==i)
				{
					teCopyRunPara.para[i].hourPeriod[0][0] = 0x10;
				}
				else
				{
				  teCopyRunPara.para[i].hourPeriod[0][0] = 0x01;
				}
      	teCopyRunPara.para[i].hourPeriod[0][1] = 0x00;
				if (4==i)
				{
      	  teCopyRunPara.para[i].hourPeriod[1][0] = 0x55;
			  }
				else
				{
      	  teCopyRunPara.para[i].hourPeriod[1][0] = 0x59;
				}
      	teCopyRunPara.para[i].hourPeriod[1][1] = 0x23;
      }
     #endif
     
  		saveParameter(0x04, 33, (INT8U *)&teCopyRunPara, sizeof(TE_COPY_RUN_PARA));
      printf("�����ն˳������в�����ʼֵ\n");
	 }
	
	 selectParameter(0x04, 34, (INT8U *)&downRiverModulePara, sizeof(DOWN_RIVER_MODULE_PARA));//F34
	 selectParameter(0x04, 35, (INT8U *)&keyHouseHold, sizeof(KEY_HOUSEHOLD));							  //F35
	 
	 if (selectParameter(0x04, 36, (INT8U *)upTranslateLimit, 4)==FALSE)   								    //F36
	 {
	 	 upTranslateLimit[0] = 0x00;
	 	 upTranslateLimit[1] = 0x00;
	 	 upTranslateLimit[2] = 0x00;
	 	 upTranslateLimit[3] = 0x00;
	 }
	 
	 selectParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));			    //F37
	 selectParameter(0x04, 38, (INT8U *)&typeDataConfig1, sizeof(TYPE_1_2_DATA_CONFIG));		  //F38
	 selectParameter(0x04, 39, (INT8U *)&typeDataConfig2, sizeof(TYPE_1_2_DATA_CONFIG));		  //F39
  	
	 //��8
	 //F57�ն������澯��������
	 if (selectParameter(0x04, 57, (INT8U *)voiceAlarm, 3)==FALSE)														//F57
	 {
	 	 voiceAlarm[0] = 0x0;
	 	 voiceAlarm[1] = 0x0;
	 	 voiceAlarm[2] = 0x0;
	 	 
     saveParameter(0x04, 57, (INT8U *)voiceAlarm, 3);
     
     printf("�����ն������澯����/��ֹ��ʼֵ\n");
	 }
	
	 selectParameter(0x04, 58, (INT8U *)&noCommunicationTime, 1);													    //F58
	
   //F59���ܱ��쳣�б���ֵ
	 selectParameter(0x04, 59, (INT8U *)&meterGate, sizeof(METER_GATE));										  //F59
   if (meterGate.powerOverGate == 0x00 && meterGate.meterFlyGate == 0x00 && meterGate.meterStopGate == 0x00 && meterGate.meterCheckTimeGate == 0x00)
   {
  	#ifdef LIGHTING    //2014-06-25,����·�Ƽ������Ŀ��Ƶ���ֵ
  	 meterGate.powerOverGate = 0x01;
  	 meterGate.meterFlyGate  = 0x01;         //CCB���ֹ������Դ���Ĭ��1��
  	 meterGate.meterStopGate = 0x01;         //Ĭ��1���ӹ㲥����ȴ���ֵ
  	 meterGate.meterCheckTimeGate = 0x01;    //Ĭ��1���ӳ���Уʱ
  	#else
  	 #ifdef SDDL_CSM
  	  meterGate.powerOverGate = 0x20;
  	  meterGate.meterFlyGate = 0x40;
  	  meterGate.meterStopGate = 0x18;        //Ĭ��6Сʱ��¼ͣ��
  	  meterGate.meterCheckTimeGate = 0x05;   //Ĭ��5���ӳ���Уʱ
  	 #else
  	  meterGate.powerOverGate = 0x99;
  	  meterGate.meterFlyGate = 0x99;
  	  meterGate.meterStopGate = 0xC0;        //Ĭ��48Сʱ��¼ͣ��
  	  meterGate.meterCheckTimeGate = 0x10;   //Ĭ��16���ӳ���Уʱ
     #endif
    #endif

     saveParameter(0x04, 59, (INT8U *)&meterGate, 4);
     
     printf("���õ��ܱ��쳣�б���ֵ��ʼֵ\n");
   }
  	     		
	 selectParameter(0x04, 60, (INT8U *)&waveLimit, sizeof(WAVE_LIMIT));										  //F60
	 selectParameter(0x04, 61, (INT8U *)&adcInFlag, 1);																		    //F61

	 if (selectParameter(0x04, 65, (INT8U *)&reportTask1, sizeof(REPORT_TASK_PARA))==FALSE)		//F65
	 {
	 	 memset((INT8U *)&reportTask1, 0x0, sizeof(REPORT_TASK_PARA));
	 }
	 if (selectParameter(0x04, 66, (INT8U *)&reportTask2, sizeof(REPORT_TASK_PARA))==FALSE)	  //F66
	 {
	 	 memset((INT8U *)&reportTask2, 0x0, sizeof(REPORT_TASK_PARA));
	 }

   //��1·ֱ��ģ��������
   if (selectViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA))==FALSE)
   {
	   adcPara.adcStartValue[0] = 0x40;   //������ʼֵ - Ĭ��ֵ4mA
	   adcPara.adcStartValue[1] = 0xa0;
	   adcPara.adcEndValue[0]   = 0x00;   //������ֵֹ - Ĭ��ֵ20mA
	   adcPara.adcEndValue[1]   = 0xa2;
	   adcPara.adcUpLimit[0]    = 0x50;   //���� - Ĭ��ֵ25mA
	   adcPara.adcUpLimit[1]    = 0xa2;
	   adcPara.adcLowLimit[0]   = 0x10;   //���� - Ĭ��ֵ1mA
	   adcPara.adcLowLimit[1]   = 0xa0;
	   adcPara.adcFreezeDensity = 0x01;   //�����ܶ� - Ĭ��Ϊ15����
	   saveViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA));
     
     printf("���õ�1·ֱ��ģ����������ʼֵ\n");
	 }

	 //��չ
	 selectParameter(0x04, 97, (INT8U *)teName, 20);						  													  //F97
	 
	 selectParameter(0x04, 98, (INT8U *)sysRunId, 20);					  													  //F98
	 
	 if (selectParameter(0x04, 99, (INT8U *)assignCopyTime, 6)==FALSE)  		 							    //F99
	 {
	 	  //��������ʱ��
	 	  assignCopyTime[0] = 0x14;
	 	  assignCopyTime[1] = 0x00;
	 	  
	 	  //�������ʱ��
	 	  assignCopyTime[2] = 0x14;
	 	  assignCopyTime[3] = 0x00;

	 	  //ѧϰ·�ɳ���ʱ��
	 	  assignCopyTime[4] = 0x14;
	 	  assignCopyTime[5] = 0x00; 		 	  
	 }
	 
	 selectParameter(0x04, 100, (INT8U *)teApn, 64);						  													   //F100
	
	 //selectParameter(0x04, 129, (INT8U *)&acSamplePara, sizeof(AC_SAMPLE_PARA));            //F129
	 //2012-09-26,Ϊ�˷�ֹ���ݿ�������޽���У����������ӱ��ݲ����ļ�
	 foundAcPara = 1;
	 if (selectParameter(0x04, 129, (INT8U *)&acSamplePara, sizeof(AC_SAMPLE_PARA))==FALSE)	  //F129
	 {
	   //��������ն˵�ַ�ļ�����
	   if(access("/keyPara129", F_OK) == 0)
	   {
	   	 readBakKeyPara(129, &acSamplePara);
	   	 
	   	 printf("���ݿ�����У�����,����У������ļ�����,ʹ�ñ���У�����\n");
	   }
	   else
	   {
	   	 printf("���ݿ�����У�����,����У������ļ�Ҳ������,ȷ��Ϊ��У�����\n");

	   	 foundAcPara = 0;
	   }
	 }
	 else
	 {
	   //��������ն˵�ַ�ļ�������
	   if(access("/keyPara129", F_OK) != 0)
	   {
	     saveBakKeyPara(129);
	     
	     printf("���ݿ�����У�����,����У������ļ�������,���汸��У�����\n");
     }
   }
   
   if (foundAcPara==1)
   {
     readBakKeyPara(129, &bakAcPara);
  
     pAcPara = (INT8U *)&acSamplePara;
     pAcParax = (INT8U *)&bakAcPara;
     for(i=0; i<sizeof(AC_SAMPLE_PARA); i++)
     {
     	 if (*pAcPara!=*pAcParax)
     	 {
     	 	 readBakKeyPara(129, &acSamplePara);
	       
	       printf("���ݿ��е�У������뱸��У�������һ��,ʹ�ñ����ļ�У��ֵ\n");

     	 	 break;
     	 }
     	 
     	 pAcPara++;
     	 pAcParax++;
     }
   }
   
   if (selectParameter(0x04, 133, mainNodeAddr, 6)==FALSE)
   {
      tmpAddr = hexToBcd(addrField.a2[0] | addrField.a2[1]<<8);
      mainNodeAddr[0] = tmpAddr&0xff;
      mainNodeAddr[1] = tmpAddr>>8&0xff;
      mainNodeAddr[2] = tmpAddr>>16&0xf;
      mainNodeAddr[3] = 0x0;
      mainNodeAddr[4] = 0x0;
      mainNodeAddr[5] = 0x0;
   }
   else
   {
     if (mainNodeAddr[0]==0x00 && mainNodeAddr[1]==0x00 && mainNodeAddr[2]==0x00
     	   && mainNodeAddr[3]==0x00 && mainNodeAddr[4]==0x00 && mainNodeAddr[5]==0x00
     	  )
     {
        tmpAddr = hexToBcd(addrField.a2[0] | addrField.a2[1]<<8);
        mainNodeAddr[0] = tmpAddr&0xff;
        mainNodeAddr[1] = tmpAddr>>8&0xff;
        mainNodeAddr[2] = tmpAddr>>16&0xf;
        mainNodeAddr[3] = 0x0;
        mainNodeAddr[4] = 0x0;
        mainNodeAddr[5] = 0x0;
     }
   }

   //���ñ��
   selectParameter(0x04, 134, (INT8U *)&deviceNumber, 2);     
   if (deviceNumber==0x0)
   {
   	 deviceNumber = 0x1;
   }
   
   //���ģ�����
   selectParameter(0x04, 135, (INT8U *)&rlPara, 4);
   //��������
   if (rlPara[0]==0x0)
   {
   	 rlPara[0] = 50;
   }
   
   //�����
   if (rlPara[1]==0x0)
   {
   	 rlPara[1] = 63;
   }
   
   //�ź�ǿ��
   if (rlPara[2]==0x0)
   {
   	 rlPara[2] = 40;
   }
   
   //�ŵ�
   if (rlPara[3]==0x0)
   {
   	 rlPara[3] = 1;
   }
   
   selectParameter(0x04, 136, (INT8U *)&csNameId, 12);

  #ifdef PLUG_IN_CARRIER_MODULE
   //�����û�����������(��07��������)
   if (selectParameter(0x04, 138, (INT8U *)&denizenDataType, 1)==FALSE)
   {
   	 denizenDataType = 0x0;    //Ĭ��Ϊ��ʵʱ+��������
   }
   
   switch(denizenDataType)
   {
   	 case 0x55:
   	 	 printf("------�����û�07��:������ʵʱ����(�ܼ�������)----------------\n");
   	 	 break;

   	 case 0xaa:
   	 	 printf("------�����û�07��:������ʵʱ(��ʾֵ)------------------------\n");
   	 	 break;

   	 default:
   	 	 printf("------�����û�07��:����ʵʱ+��������-------------------------\n");
   	 	 break;
   }
   
   //������������
   if (selectParameter(0x04, 199, (INT8U *)&cycleDataType, 1)==FALSE)
   {
   	 cycleDataType = 0x0;    //Ĭ��Ϊ:���б��
   }

  #endif

	 //��6(�ܼ��������Ϣ)
	 for(i=0; i<8; i++)
	 {
		 selectViceParameter(0x04, 41, (i+1), (INT8U *)&periodCtrlConfig[i], sizeof(PERIOD_CTRL_CONFIG));		//F41
		 selectViceParameter(0x04, 42, (i+1), (INT8U *)&wkdCtrlConfig[i], sizeof(WKD_CTRL_CONFIG));					//F42
		 selectViceParameter(0x04, 43, (i+1), (INT8U *)&powerCtrlCountTime[i], sizeof(POWERCTRL_COUNT_TIME));//F43
		 selectViceParameter(0x04, 44, (i+1), (INT8U *)&obsCtrlConfig[i], sizeof(OBS_CTRL_CONFIG));					//F44
		 selectViceParameter(0x04, 45, (i+1), (INT8U *)&powerCtrlRoundFlag[i], sizeof(POWERCTRL_ROUND_FLAG));//F45
		 selectViceParameter(0x04, 46, (i+1), (INT8U *)&monthCtrlConfig[i], sizeof(MONTH_CTRL_CONFIG));			//F46
		 selectViceParameter(0x04, 47, (i+1), (INT8U *)&chargeCtrlConfig[i], sizeof(CHARGE_CTRL_CONFIG));		//F47
		 selectViceParameter(0x04, 48, (i+1), (INT8U *)&electCtrlRoundFlag[i], sizeof(ELECTCTRL_ROUND_FLAG));//F48
		 selectViceParameter(0x04, 49, (i+1), (INT8U *)&powerCtrlAlarmTime[i], sizeof(POWERCTRL_ALARM_TIME));//F49
	 }
	
	 //���Ʋ��� AFN05
   selectParameter(0x05, 1, (INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);//ң�����ò��������̲���
   selectParameter(0x05, 3, (INT8U *)&remoteEventInfor,sizeof(REMOTE_EVENT_INFOR)*8);             //ң���ֳ�������FN03����
   selectParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);                  //����Ͷ��״̬��FN04����
   selectParameter(0x05, 5, (INT8U *)&powerCtrlEventInfor, sizeof(POWER_CTRL_EVENT_INFOR));       //�����ֳ���¼״̬��FN05����
   selectParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);                //��ǰ�����¸�����F12
   selectParameter(0x05,25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));            //�ն˱���Ͷ��F25/���F33
   selectParameter(0x05,26, &reminderFee, 1);                                                     //�߷Ѹ澯Ͷ��F26/���F34
   selectParameter(0x05,28, &toEliminate, 1);                                                     //�޳�Ͷ��F28/���F36	   

   selectParameter(0x05,29, &callAndReport, 1);                    //����/��ֹ�ն������ϱ�
   selectParameter(0x05,30, &teInRunning, 1);                      //�ն�����Ͷ��/�˳�����
   selectParameter(0x05,32, &chnMessage, sizeof(CHN_MESSAGE));     //������Ϣ
   
   //�ն˲��� AFN88
   //�ն˲����б�:
   //    FN=01,�ն�ͣ�ϵ��¼
   //    FN=02,�ն���Ҫ�¼��Ѷ��¼�
   //    FN=03,���������ݻ���
   //    FN=07,LCD�Աȶ�ֵ
   //    FN=08,��������
   //    FN=13,��������������
   //    FN=33,Զ��������־
   //    FN=34,13���Լ������־
   //    FN=55,����ͨ��ģ�鳭����ʽ
   selectParameter(88, 1, (INT8U *)&powerOnOffRecord,sizeof(POWER_ON_OFF));   //�ն�ͣ�ϵ��¼
   selectParameter(88, 2, eventReadedPointer,2);                              //�ն���Ҫ�¼��Ѷ�ָ��
   
   #ifdef PULSE_GATHER
     //���������ݻ���
     if (selectParameter(88,  3, pulseDataBuff, NUM_OF_SWITCH_PULSE*53)==FALSE)
     {
     	  printf("DataBase-loadParameter:�����������ݻ���,����������\n");
     	  
     	  memset(pulseDataBuff, 0x0, NUM_OF_SWITCH_PULSE*53);
     }
     
     //��������������
     if (selectParameter(88, 13, pulse, sizeof(ONE_PULSE)*NUM_OF_SWITCH_PULSE)==FALSE)
     {
     	 printf("DataBase-loadParameter:����������������,�������������\n");
     	 
     	 for (i = 0; i < NUM_OF_SWITCH_PULSE; i++)
     	 {
     	 	 pulse[i].pulseCount = 0x0;
     	 	 
     	 	 for(j=0;j<14;j++)
     	 	 {
     	 	 	 pulse[i].pulseCountTariff[j] = 0x0;
     	 	 }
     	 }
     }
   #endif

   //FN=7,LCD�Աȶ� 
   selectParameter(88, 7,&lcdDegree,1);                                      //LCD�Աȶ�ֵ
   if (lcdDegree==0)
   {
   	 lcdDegree = 10;
   }
   
   //FN=8,��������
   selectParameter(88, 8, originPassword, 7);

   //FN=55,����ͨ��ģ�鳭����ʽ ly,2012-01-13,add
   if (selectParameter(88, 55, &localCopyForm, 1)==FALSE)
   {
   	 localCopyForm = 0xaa;
   }
 	 if (localCopyForm!=0x55 && localCopyForm!=0xaa)    //ly,2012-01-13,add
 	 {
 	 	 localCopyForm = 0xaa;        //Ĭ��Ϊ·����������,0xaaΪ��������������
 	 }

   //FN=56,����ͨ��ģ��Э�� ly,2015-09-16,add
   if (selectParameter(88, 56, &lmProtocol, 1)==FALSE)
   {
   	 lmProtocol = 0xaa;
   }
 	 if (lmProtocol!=0x55 && lmProtocol!=0xaa)
 	 {
 	 	 lmProtocol = 0xaa;        //Ĭ��ΪQ/GDW376.2-2009
 	 }
 	 
	 //2.�����¼���¼ָ��
	 iEventStartPtr = queryEventStoreNo(1);
	 nEventStartPtr = queryEventStoreNo(2);
  
   //2-1��Ҫ�¼�ָ��
	 if (iEventStartPtr>255)
	 {
		 iEventStartPtr -= 255;
     
	   pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=1 and storeNo<%d",iEventStartPtr);
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
   	    if (debugInfo&PRINT_EVENT_DEBUG)
   	    {
   	      printf("ɾ���������Ҫ�¼���¼�ɹ�!\n");
   	    }
     }
     sqlite3_free(pSqlStr);
     
		 iEventCounter = 255;
	 }
	 else
	 {
		 iEventCounter  = iEventStartPtr;
		 iEventStartPtr = 0;
	 }

   //2-2.һ���¼�ָ��	
	 if (nEventStartPtr>255)
	 {
		  nEventStartPtr -= 255;

	    pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=2 and storeNo<%d",nEventStartPtr);
      if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_EVENT_DEBUG)
   	    {
   	      printf("ɾ�������һ���¼���¼�ɹ�!\n");
   	    }
      }
      sqlite3_free(pSqlStr);
     
		  nEventCounter  = 255;
	 }
	 else
	 {
		  nEventCounter  = nEventStartPtr;
		  nEventStartPtr = 0;
	 }
	
	 if (debugInfo&PRINT_EVENT_DEBUG)
	 {
	  printf("iEventStartPtr=%d,iEventCounter=%d,nEventStartPtr=%d,nEventCounter=%d\n",iEventStartPtr,iEventCounter,nEventStartPtr,nEventCounter);
   }
   
   //������������
  #ifdef AC_SAMPLE_DEVICE
   readAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);
  #endif
  
 #ifdef LIGHTING
  initCtrlTimesLink();
  
  //F51 ���Ƶ���ֵ
	selectParameter(0x04, 51, (INT8U *)&pnGate, sizeof(PN_GATE));    //F51
	
	//F52 ����ģʽ,2015-6-9,add
	selectParameter(0x04, 52, &ctrlMode, 1);
	printf("����ģʽ:%d\n", ctrlMode);

	//F53 �����ǰ-�ӳ�ʱ��,2015-06-25,add
	selectParameter(0x04, 53, beforeOnOff, 4);
	
	//2015-11-16,AddĬ��ֵ
	if (beforeOnOff[0]==0)
	{
		beforeOnOff[0] = 30;
	}
	if (beforeOnOff[1]==0)
	{
		beforeOnOff[1] = 30;
	}
	if (beforeOnOff[2]==0)
	{
		beforeOnOff[2] = 30;
	}
	if (beforeOnOff[3]==0)
	{
		beforeOnOff[3] = 30;
	}
	printf("��ؿ�����ǰ%d������Ч\n", beforeOnOff[0]);
	printf("��ؿ����ӳ�%d������Ч\n", beforeOnOff[1]);
	printf("��عص���ǰ%d������Ч\n", beforeOnOff[2]);
	printf("��عص��ӳ�%d������Ч\n", beforeOnOff[3]);
	
  if (pnGate.failureRetry == 0x00 && pnGate.boardcastWaitGate == 0x00 && pnGate.checkTimeGate == 0x00)
  {
  	pnGate.failureRetry  = 0x01;        //CCB���ֹ������Դ���Ĭ��1��
  	pnGate.boardcastWaitGate = 0x01;    //Ĭ��1���ӹ㲥����ȴ���ֵ
  	pnGate.checkTimeGate = 0x01;        //Ĭ��1���ӳ���Уʱ
  	pnGate.lddOffGate = 0x05;           //Ĭ��5Сʱ������ֵ,2016-11-28,������ֵ��λ�ӡ��֡��ĳɡ�Сʱ��
  	pnGate.lddtRetry = 0x05;            //Ĭ��5��������ĩ�����Դ���
  	pnGate.offLineRetry = 0x02;         //Ĭ��2�����������Դ���
  	pnGate.lcWave = 0x00;               //Ĭ��0Lux����ն���ֵ
  	pnGate.leakCurrent = 10;            //Ĭ��10mA©������ֵ

    saveParameter(0x04, 51, (INT8U *)&pnGate, sizeof(PN_GATE));
     
    printf("���ÿ��Ƶ���ֵ��ʼֵ\n");
  }

 #endif
}

/*******************************************************
��������:saveParameter
��������:���ڴ洢����
���ú���:
�����ú���:
�������:para:���洢������  len: ���ݳ��ȣ��ֽڣ�
�������:
����ֵ��void
*******************************************************/
void saveParameter(INT8U afn, INT8U fn, INT8U *para, INT32U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
  
	sql = "select * from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	
	if(result == SQLITE_ROW)
	{
		sql = "update base_info set acc_data = ? where acc_afn = ? and acc_fn = ?";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, para, len, NULL);
		sqlite3_bind_blob(stat, 2, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 3, &fn, 1, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
	else
	{
		sql = "insert into base_info(acc_afn, acc_fn, acc_data) values(?, ?, ?)";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
		sqlite3_bind_blob(stat, 3, para, len, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
}

/*******************************************************
��������:saveViceParameter
��������:���ڴ洢����(������Ϣ����), ���и��£��������
���ú���:
�����ú���:
�������:fn  pn  para:���洢������  len: ���ݳ��ȣ��ֽڣ�
�������:
����ֵ��void
*******************************************************/
void saveViceParameter(INT8U afn, INT8U fn, INT16U pn, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
	
	sql = "select * from base_vice_info where acc_afn = ? and acc_fn = ? and acc_pn = ?;";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, &pn, 2, NULL);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	if(result == SQLITE_ROW)
	{
		sql = "update base_vice_info set acc_data = ? where acc_afn = ? and acc_fn = ? and acc_pn = ?;";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, para, len, NULL);
		sqlite3_bind_blob(stat, 2, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 3, &fn, 1, NULL);
		sqlite3_bind_blob(stat, 4, &pn, 2, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
	else
	{
		sql = "insert into base_vice_info(acc_afn, acc_fn, acc_pn, acc_data) values(?, ?, ?, ?);";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
		sqlite3_bind_blob(stat, 3, &pn, 2, NULL);
		sqlite3_bind_blob(stat, 4, para, len, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
}

/*******************************************************
��������:deleteParameter
��������:����ɾ������(������Ϣ����)
���ú���:
�����ú���:
�������:afn  fn
�������:
����ֵ��void
*******************************************************/
void deleteParameter(INT8U afn, INT8U fn)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;

	//ɾ����Ӧ����
	sql = "delete from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*******************************************************
��������:insertParameter
��������:���ڴ洢����(������Ϣ����)
���ú���:
�����ú���:
�������:fn  pn  para:���洢������  len: ���ݳ��ȣ��ֽڣ�
�������:
����ֵ��void
*******************************************************/
void insertParameter(INT8U afn, INT8U fn, INT16U pn, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;

	//����������
	sql = "insert into base_vice_info(acc_afn, acc_fn, acc_pn, acc_data) values(?, ?, ?, ?)";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, &pn, 1, NULL);
	sqlite3_bind_blob(stat, 4, para, len, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*******************************************************
��������:countParameter
��������:����ͳ������(������Ϣ����)
���ú���:
�����ú���:
�������:fn    	pn count:������������
�������:
����ֵ��void
*******************************************************/
void countParameter(INT8U afn, INT8U fn, INT16U *num)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
	
	*num = 0;
	
	//������������
	sql = "select * from f10_info";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	result = sqlite3_step(stat);
	while(result == SQLITE_ROW)
	{
		(*num)++;
		result = sqlite3_step(stat);
	}
	sqlite3_finalize(stat);
}

/*******************************************************
��������:selectParameter
��������:���ڲ�ѯ�������е�����
���ú���:
�����ú���:
�������:afn  fn  *para:�����ѯ��������   len:���ݳ���(�ֽ�Ϊ��λ)
�������:
����ֵ��void
*******************************************************/
BOOL selectParameter(INT8U afn, INT8U fn, INT8U *para, INT32U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
	
	//��ѯ����
	sql = "select * from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		memcpy(para, sqlite3_column_blob(stat, 2), len);
		sqlite3_finalize(stat);
    
    if (afn==4)
    {
    	if (fn>0 && fn<84)
    	{
    	  paraStatus[(fn-1)/8] |= 1<<((fn-1)%8);               //��"�ն˲���״̬"λ
    	}
    }
    
		return TRUE;
	}
	else
	{
		if (result!=SQLITE_DONE)
		{
		  logRun("selectParameter happen aberrant.");
		}
		
		sqlite3Aberrant(result);
	}

	sqlite3_finalize(stat);

	return FALSE;
}

/*******************************************************
��������:selectViceParameter
��������:���ڲ�ѯ���������е�����
���ú���:
�����ú���:
�������:afn  fn  pn:�������/�ܼ����/�����/ֱ��ģ�����˿ں�  
         *para:�����ѯ��������   len:���ݳ���(�ֽ�Ϊ��λ)
�������:
����ֵ��void
*******************************************************/
BOOL selectViceParameter(INT8U afn, INT8U fn, INT16U pn, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;	
	INT16U result;
	
	//��ѯ����
	sql = "select * from base_vice_info where acc_afn = ? and acc_fn = ? and acc_pn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, &pn, 2, NULL);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		memcpy(para, sqlite3_column_blob(stat, 3), len);
		sqlite3_finalize(stat);
		
		return TRUE;
	}
	sqlite3_finalize(stat);

	return FALSE;
}

/*******************************************************
��������:saveDataF10
��������:���ڴ洢F10����, ���и��£��������
���ú���:
�����ú���:
�������:pn  port:�˿ں�  num:���  
				 para:���洢������  len: ���ݳ��ȣ��ֽڣ�
�������:
����ֵ��void
*******************************************************/
void saveDataF10(INT16U pn, INT8U port, INT8U *meterAddr, INT16U num, INT8U *para, INT16U len)
{
	METER_DEVICE_CONFIG mdc;
	sqlite3_stmt        *stat;
	INT8S               *sql;
	INT16U              result;
	
	//2013-05-07,�����������ʾ�ŷ���,�������Ϊ0��ɾ��,�ǲ�����Ϊ0��ɾ��
	if (0==pn)
	{
    //��ѯ����Ŷ�Ӧ��ԭ������,������ɾ��ԭ������Ķ�������
    sql = "select * from f10_info where acc_num = ?";
    sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
    sqlite3_bind_int(stat, 1, num);
    result = sqlite3_step(stat);
    if(result == SQLITE_ROW)
    {
    	memcpy((INT8U *)&mdc, sqlite3_column_blob(stat, 4), sizeof(METER_DEVICE_CONFIG));
      sqlite3_finalize(stat);
      
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("saveDataF10:ɾ��������%d�Ķ�������\n", mdc.measurePoint);
      }
      	
      deleteFreezeData(mdc.measurePoint);
    }
    else
    {
      sqlite3_finalize(stat);
    }
	
	  //ɾ���������Ϊ0�����Ϊnum�Ĳ�����
	  sql = "delete from f10_info where acc_num = ?";
	  sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	  sqlite3_bind_int(stat, 1, num);
	  result = sqlite3_step(stat);
	  sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_DATA_BASE)
		{
		  printf("saveDataF10:ɾ���������Ϊ0�����Ϊ%d�ı��ַ\n", num);
		}
		
		return;
	}

	//2012-09-06,�ڳ��ٷ��ֵ�����վ��һ��һ����ɾ�����ַ,�õ��ǹ�Լ��˵�����Ϊ0�ľ�ɾ���ò�����
	//           ���Ӹô�������Ӧ������վ
	if (0==num)
	{
	  //ɾ�����Ϊ0�Ĳ�����
	  sql = "delete from f10_info where acc_pn = ?";
	  sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	  sqlite3_bind_int(stat, 1, pn);
	  result = sqlite3_step(stat);
	  sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_DATA_BASE)
		{
		  printf("saveDataF10:ɾ�����Ϊ0�Ĳ�����%d����\n", pn);
		}
		
		return;
	}
	
	//��ѯԭ���Ƿ��ж�Ӧ�Ĳ�����,����,���λ�ɾ��ԭ������,ͬʱɾ��ԭ������Ķ�������
	sql = "select * from f10_info where acc_pn = ? or acc_num = ? or acc_meter_addr = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, num);
	sqlite3_bind_blob(stat, 3, meterAddr, 6, NULL);
	result = sqlite3_step(stat);	
	if(result == SQLITE_ROW)
	{
		memcpy((INT8U *)&mdc, sqlite3_column_blob(stat, 4), sizeof(METER_DEVICE_CONFIG));
	  sqlite3_finalize(stat);
    
    if ((mdc.measurePoint!=pn) || (compareTwoAddr(mdc.addr, meterAddr, 0)==FALSE))
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveDataF10:������Ų���ͬ����ַ����ͬ,��Ҫɾ������\n");
    	}
    	
      deleteFreezeData(mdc.measurePoint);
    }
	}
	else
	{
	  sqlite3_finalize(stat);
	}
	
	//ɾ���Ѵ��ڵ�����
	sql = "delete from f10_info where acc_pn = ? or acc_num = ? or acc_meter_addr = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, num);
	sqlite3_bind_blob(stat, 3, meterAddr, 6, NULL);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	
	//�����µ�����
	sql = "insert into f10_info(acc_pn, acc_port, acc_meter_addr, acc_num, acc_data) values(?, ?, ?, ?, ?)";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, port);
	sqlite3_bind_blob(stat, 3, meterAddr, 6, NULL);
	sqlite3_bind_int(stat, 4, num);
	sqlite3_bind_blob(stat, 5, para, len, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*******************************************************
��������:selectF10Data
��������:���ڲ�ѯF10����, ���и��£��������
���ú���:
�����ú���:
�������:pn  port:�˿ں�  num:���  
				 para:���洢������  len: ���ݳ��ȣ��ֽڣ�
�������:
����ֵ��void
*******************************************************/
BOOL selectF10Data(INT16U pn, INT8U port, INT16U num, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	INT8S        *sql;	
	INT8U        flg;
	INT16U       result;
	
	sql = "select * from f10_info where (acc_pn = ? or acc_num = ?)";
	if(port != 0)
	{
		sql = "select * from f10_info where (acc_pn = ? or acc_num = ?) and acc_port = ?";
	}
	else
	{
		sql = "select * from f10_info where acc_pn = ? or acc_num = ?";
	}
	
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, num);
	if(port != 0)
	{
		sqlite3_bind_int(stat, 3, port);
	}
	result = sqlite3_step(stat);
	
	if(result == SQLITE_ROW)
	{
		 memcpy(para, sqlite3_column_blob(stat, 4), len);
	   sqlite3_finalize(stat);
		 return TRUE;
	}
	sqlite3_finalize(stat);
	
	return FALSE;
}

/*******************************************************
��������:queryData
��������:���ݴ����SQL����ѯ�����м�����¼
���ú���:
�����ú���:
�������:char  *pSql-SQL���,
         INT8U *data-��ѯ�ɹ����ص�����
         INT8U type -��ѯ����(1-����,2-��ѯ����ʱ��,3..������)
�������:
����ֵ:��ѯ���ļ�¼����
*******************************************************/
INT16U queryData(char *pSql,INT8U *data,INT8U type)
{
	 sqlite3_stmt *stmt;
	 const char   *tail;
	 int          execResult;
	 INT16U       numOfRecord;
	 INT8U        i,ncols;
	 
   if (sqlite3_prepare(sqlite3Db, pSql, strlen(pSql), &stmt, &tail)!= SQLITE_OK)
   {
     fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
   }   
   execResult = sqlite3_step(stmt);
   
   if (type)
   {
      ncols = sqlite3_column_count(stmt);
   }
   
   numOfRecord=0;
   while(execResult==SQLITE_ROW)
   {
     if (type)
     {
       for(i=0; i < ncols; i++)
       {
         switch(type)
         {
           case 1:    //��ѯ����
             if (i>1)
             {
               *data = atoi(sqlite3_column_text(stmt, i));
               data++;
             }
             break;

           case 2:    //��ѯ����ʱ��
             *data = atoi(sqlite3_column_text(stmt, i));
             data++;
             break;
         }
       }
     }
     
     numOfRecord++;
     
     if (type==2)
     {
      	break;
     }

     execResult = sqlite3_step(stmt);
   }
   
   sqlite3_finalize(stmt);
   
   return numOfRecord;
}

/*******************************************************
��������:qureyEventStoreNo
��������:��ѯ�¼��洢���
���ú���:
�����ú���:
�������:INT8U  eventType,�¼�����
�������:
����ֵ:��ѯ���洢���
*******************************************************/
INT32U queryEventStoreNo(INT8U eventType)
{
	 sqlite3_stmt *stmt;
	 const char   *tail;
   char         *pSqlStr;                  //SQL����ַ���ָ��
	 int          execResult;
	 INT32U       recordNo;
	 
	 pSqlStr = sqlite3_mprintf("select storeNo from eventRecord where eventType=%d order by storeNo desc limit 1",eventType);
	 
   if (sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail)!= SQLITE_OK)
   {
     fprintf(stderr, "Query Store No SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
   }   
   execResult = sqlite3_step(stmt);
   
   recordNo=0;
   
   while(execResult==SQLITE_ROW)
   {
      recordNo = sqlite3_column_int(stmt,0);
            
      execResult = sqlite3_step(stmt);
   }
   sqlite3_finalize(stmt);
   
   return recordNo;
}

/*******************************************************
��������:insertData
��������:�����ݿ��в�������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
BOOL insertData(char *pSql)
{
   char    *errMessage;               //������Ϣ(Error msg written her)
   
   if (sqlite3_exec(sqlite3Db, pSql, NULL, NULL, &errMessage)==SQLITE_OK)
   {
   	  return TRUE;
   }
   
   return FALSE;
}

/*******************************************************
��������:saveBakDayFile
��������:�����ն������ݵ��ļ���
���ú���:
�����ú���:
�������:type =0x01,���������
              =0x0b,�����ʾֵ
              =0x0c,���������
�������:
����ֵ:void
*******************************************************/
BOOL saveBakDayFile(INT16U pn, int freezeTime, int copyTime, INT8U *buf, INT8U type)
{
	FILE           *fp;
	char           fileName[20];
  struct timeval tv;                  //Linux timeval
  DATE_TIME      tmpTime;
  SP_F_DAY       tmpSpfDay;
  MP_F_DAY       tmpMpfDay;
  INT16U         tmpCount;
	
  tv.tv_sec = freezeTime;
  getLinuxFormatDateTime(&tmpTime, &tv, 2);
	
	//���������ļ���
	if (type==1)
	{
	  sprintf(fileName, "/data/spd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
	}
	else
	{
	  sprintf(fileName, "/data/mpd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
	}

  //�鿴Ŀ¼/�ļ��Ƿ����
  if(access(fileName, F_OK) != 0)
  {
    if (mkdir("/data", S_IRUSR | S_IWUSR | S_IXUSR)==0)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:����Ŀ¼/data�ɹ�\n");
    	}
    }
    else
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:����Ŀ¼/dataʧ��\n");
    	}
    }
    
    //�����ļ�
    if((fp=fopen(fileName,"wb+"))==NULL)
    {
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("saveBakDayFile:�����ļ�%sʧ��.\n", fileName);
      }
      	
      return FALSE;
    }
      
    fclose(fp);
  }

  if((fp=fopen(fileName, "rb+"))==NULL)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
  	  printf("saveBakDayFile:���ļ�%sʧ��\n", fileName);
  	}
  	
  	return FALSE;
  }

  tmpCount = 0;
  while (!feof(fp))
  {
  	if (type==1)
  	{
    	if (fread(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0 ,2);    //���ļ�β
    	  
    	  break;
    	}
      
      if (tmpSpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("saveBakDayFile:���Ե�%d�����Ѵ���\n", pn);
      	}
      	
      	//��λ������¼
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(SP_F_DAY), 0);
      	break;
      }
    }
    else
    {
    	if (fread(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0 ,2);    //���ļ�β
    	  
    	  break;
    	}
      
      if (tmpMpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("saveBakDayFile:���Ե�%d�����Ѵ���\n", pn);
      	}
      	
      	//��λ������¼
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(MP_F_DAY), 0);
      	break;
      }
    }

  	tmpCount++;
  }
  
  if (type==1)
  {
    tmpSpfDay.pn = pn;
    tmpSpfDay.copyTime = copyTime;
    memcpy(tmpSpfDay.data, buf, 40);
    if (fwrite(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:д�����ݵ��ļ�%sʧ��\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  else
  {
    tmpMpfDay.pn   = pn;
    tmpMpfDay.type = type;
    tmpMpfDay.copyTime = copyTime;
    memcpy(tmpMpfDay.data, buf, 288);
    if (fwrite(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:д�����ݵ��ļ�%sʧ��\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  
  if (debugInfo&PRINT_DATA_BASE)
  {
    printf("saveBakDayFile:д�����ݵ��ļ�%s�ɹ�\n", fileName);
  }
  
  fclose(fp);
  return TRUE;
}

/*******************************************************
��������:saveCopyData
��������:���泭������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
BOOL saveMeterData(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len)
{
	 sqlite3_stmt   *stmt;
   char           *pSqlStr;                  //SQL����ַ���ָ��
	 INT8U          tmpBuff[768];
   char           *errMessage;               //������Ϣ(Error msg written her)
   DATE_TIME      tmpTime, tmpTimex, tmpBakTimex;
   char           tableName[30];
   INT16U         i;
   struct timeval tv, tvxx;                  //Linux timeval
   INT8U          backDataType;
   INT16U         execResult;
   SP_F_DAY       spFileDay;                 //������ն��������ļ���¼����,2012-09-30
   
   pSqlStr=NULL;

   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("saveMeterData(%d-%d-%d %d:%d:%d):ready,������:%d,��ѯ����=%02x,��������=%02x\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, pn, queryType, dataType);
   }

   while(dbLocker)
   {
   	 usleep(100);
   	 
   	 printf("**************���ݿ�д��ȴ�,pn=%d,queryType=%02x,dataType=%02x****************\n", pn, queryType, dataType);
   }
   
   dbLocker = 1;
   
   //���������,��ֹд������
   if (flagOfClearData==0x55 || flagOfClearData==0xaa)
   {
     //��Ϊ��������ʱҪ����ʣ�����,����ʣ���������
     if (queryType==LEFT_POWER && dataType==0)
     {
     	 ;
     }
     else
     {
       //if (debugInfo&PRINT_DATA_DEBUG)
       //{
         printf("saveMeterData:���������,��ֹд������,queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
       //}
   	   
   	   dbLocker = 0;
   	   
   	   return FALSE;
   	 }
   }

   //����ԭ���Ƿ���ڸò������ͳ������,������������
   if(queryType==STATIS_DATA)
   {
     tmpTime = saveTime;
     if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
     {
     	 switch (dataType)
     	 {
     	 	 case 88:   //�������ʱ���޹���
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=88;", pn);
   	       break;

     	 	#ifdef LIGHTING
     	 	 case 89:   //��·��������ʱ���޹���
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=89;", pn);
   	       break;
   	    #endif

     	 	 case 99:   //�������ʱ���޹���
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=99;", pn);
   	       break;
   	       
   	     default:
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=%d and month=%d and year=%d;",pn, saveTime.day, saveTime.month,saveTime.year);
   	       break;
   	   }
   	   
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("���²�����ͳ������SQL:");
   	     printf(pSqlStr);
   	     printf("\n");
       }
       
       sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
       sqlite3_bind_blob(stmt, 1, buff, len, NULL);
       if (sqlite3_step(stmt)==SQLITE_DONE)
       {
       	 if (debugInfo&PRINT_DATA_BASE)
       	 {
       	   printf("saveMeterData:���²�����%dͳ�����ݳɹ�\n",pn);
       	 }
       	 
       	 dbLocker = 0;
       	 
       	 sqlite3_finalize(stmt);

       	 return TRUE;
       }
       else
       {
       	 if (debugInfo&PRINT_DATA_BASE)
       	 {
       	   printf("saveMeterData:���²�����%dͳ������ʧ��\n",pn);
       	 }
         
         dbLocker = 0;
       	 
       	 sqlite3_finalize(stmt);
       	 
       	 return FALSE;
       }
     }
   }

   //����ԭ���Ƿ���ڸ��ܼ����ʣ���������,������������
   if(queryType==LEFT_POWER)
   {
     tmpTime = saveTime;
     if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
     {
   	   pSqlStr = sqlite3_mprintf("update leftPower set data=? where pn=%d;",pn);
       sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
       sqlite3_bind_blob(stmt, 1, buff, len, NULL);
       if (sqlite3_step(stmt)==SQLITE_DONE)
       {
       	  if (debugInfo&PRINT_DATA_BASE)
       	  {
       	    printf("saveMeterData:�����ܼ���%dʣ��������ݳɹ�\n",pn);
       	  }
       	  
       	  dbLocker = 0;
       	  
       	  sqlite3_finalize(stmt);

       	  return TRUE;
       }
       else
       {
       	  if (debugInfo&PRINT_DATA_BASE)
       	  {
       	    printf("saveMeterData:�����ܼ���%dʣ���������ʧ��\n",pn);
       	  }
          
          dbLocker = 0;

       	  sqlite3_finalize(stmt);

       	  return FALSE;
       }
     }
   }

   tmpTime = timeBcdToHex(saveTime);
   getLinuxFormatDateTime(&tmpTime,&tv,1);
   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("saveMeterData:�洢ʱ��:%02x-%02x-%02x %02x:%02x:%02x,�洢����:%d ������:%d\n",saveTime.year,saveTime.month,saveTime.day,saveTime.hour,saveTime.minute,saveTime.second,tv.tv_sec,pn);
   }

   switch (queryType)
   {
     case PRESENT_DATA:              //ʵʱ��������
     case REAL_BALANCE:              //ʵʱ��������
     case LAST_REAL_BALANCE:         //���һ��ʵʱ��������
     	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime),1));
     	 tmpTime = saveTime;
     	 if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
     	 {
     	 	 printf("saveMeterData:������ͬ��Ϣ������(���ܾ����ݲ�ͬ),��������, ʱ��=%02x-%02x-%02x %02x:%02x:%02x pn=%d,queryType=%d,dataType=%d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second,pn, queryType, dataType);
	       
	       pSqlStr = sqlite3_mprintf("update %s set data=? where pn=%d and queryType=%d and dataType=%d and time=%d;",tableName, pn, queryType, dataType, tv.tv_sec);
     	 }
     	 else
     	 {
	       pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,%d,%d,?);",tableName, pn, queryType, dataType, tv.tv_sec);
	     }
       break;

     case THREE_PRESENT:             //�������ܱ�ʵʱ����
     case THREE_LOCAL_CTRL_PRESENT:  //���౾�طѿر�ʵʱ����
     case THREE_REMOTE_CTRL_PRESENT: //����Զ�̷ѿر�ʵʱ����
     case KEY_HOUSEHOLD_PRESENT:     //�ص��û�ʵʱ����
     	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime),1));
     	 tmpTime = saveTime;
	     
	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,%d,%d,?);",tableName, pn, queryType, dataType, tv.tv_sec);
       break;
        
     case LAST_MONTH_DATA:           //��������
	     pSqlStr = sqlite3_mprintf("insert into lastMonthData values (%d,%d,%d,%d,?);", pn, queryType, dataType, tv.tv_sec);
       break;

     case DAY_BALANCE:               //�ս�������
     case MONTH_BALANCE:             //�½�������
     case THREE_DAY:
     case THREE_MONTH:
     case THREE_LOCAL_CTRL_DAY:
     case THREE_LOCAL_CTRL_MONTH:
     case THREE_REMOTE_CTRL_DAY:
     case THREE_REMOTE_CTRL_MONTH:
     case KEY_HOUSEHOLD_DAY:
     case KEY_HOUSEHOLD_MONTH:
     	 //�����ֱ�Ӵӵ��ɼ�������һ���ն�������
       //��������
     	 if (dataType==DAY_FREEZE_COPY_DATA_M || dataType==DAY_FREEZE_COPY_REQ_M || dataType==MONTH_FREEZE_COPY_DATA_M || dataType==MONTH_FREEZE_COPY_REQ_M)
     	 {
     	 	 //07��Ϊ�����ʱ��
     	 	 if (dataType==DAY_FREEZE_COPY_DATA_M || dataType==MONTH_FREEZE_COPY_DATA_M)
     	 	 {
     	 	   if (buff[DAY_FREEZE_TIME_FLAG_T]==0xee)
     	 	   {
     	 	 	   if (debugInfo&PRINT_DATA_BASE)
     	 	 	   {
     	 	 	     printf("saveMeterData:������ն���ʱ��δ����\n");
     	 	 	   }
     	 	 	   
     	 	 	   dbLocker = 0;
     	 	 	   
     	 	 	   return FALSE;
     	 	   }
     	 	   else
     	 	   {
     	 	     tmpTime.second = 0x0;
     	 	     tmpTime.minute = buff[DAY_FREEZE_TIME_FLAG_T+0];
     	 	     tmpTime.hour   = buff[DAY_FREEZE_TIME_FLAG_T+1];
     	 	     tmpTime.day    = buff[DAY_FREEZE_TIME_FLAG_T+2];
     	 	     tmpTime.month  = buff[DAY_FREEZE_TIME_FLAG_T+3];
     	 	     tmpTime.year   = buff[DAY_FREEZE_TIME_FLAG_T+4];
     	 	   }
     	 	 }
     	 	 else   //����ʱ����Ҫ����
     	 	 {
     	     if (port==31)
     	     {
       	 	   if (copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T]==0xee)
       	 	   {
       	 	 	   if (debugInfo&PRINT_DATA_BASE)
       	 	 	   {
       	 	 	     printf("saveMeterData:������ն���ʱ��δ����\n");
       	 	 	   }
       	 	 	   
       	 	 	   dbLocker = 0;
       	 	 	   
       	 	 	   return FALSE;
       	 	   }
       	 	   else
       	 	   {
       	 	     tmpTime.second = 0x0;
       	 	     tmpTime.minute = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+0];
       	 	     tmpTime.hour   = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+1];
       	 	     tmpTime.day    = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+2];
       	 	     tmpTime.month  = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+3];
       	 	     tmpTime.year   = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+4];
       	 	   }
     	     }
     	     else
     	     {
       	 	   if (copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T]==0xee)
       	 	   {
       	 	 	   if (debugInfo&PRINT_DATA_BASE)
       	 	 	   {
       	 	 	     printf("saveMeterData:������ն���ʱ��δ����\n");
       	 	 	   }
       	 	 	   
       	 	 	   dbLocker = 0;
       	 	 	   
       	 	 	   return FALSE;
       	 	   }
       	 	   else
       	 	   {
       	 	     tmpTime.second = 0x0;
       	 	     tmpTime.minute = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+0];
       	 	     tmpTime.hour   = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+1];
       	 	     tmpTime.day    = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+2];
       	 	     tmpTime.month  = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+3];
       	 	     tmpTime.year   = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+4];
       	 	   }
     	     }
     	 	 }

   	 	   if (dataType==MONTH_FREEZE_COPY_DATA_M || dataType==MONTH_FREEZE_COPY_REQ_M)
   	 	   {
   	 	 	   tmpTime = timeHexToBcd(backTime(timeBcdToHex(tmpTime), 0, 1, 0, 0, 0));
   	 	   }
     	 }
     	 else
     	 {
     	 	 tmpTime = saveTime;
     	 }
     	 
     	 tmpTimex = tmpTime;
       tmpBakTimex = tmpTimex;

       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("saveMeterData-�������ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }

       //��������
       tmpTime = timeBcdToHex(tmpTime);
       tmpTime.hour   = 0;
       tmpTime.minute = 0;
       tmpTime.second = 0;
       getLinuxFormatDateTime(&tmpTime,&tv,1);
       
       backDataType = dataType;
       
       //����ʱ��
       tmpTime = timeBcdToHex(saveTime);
       getLinuxFormatDateTime(&tmpTime,&tvxx,1);
       
       if (dataType==DAY_FREEZE_COPY_DATA_M)
       {
       	 dataType = DAY_FREEZE_COPY_DATA;
       }
       
       if (dataType==DAY_FREEZE_COPY_REQ_M)
       {
       	 dataType = DAY_FREEZE_COPY_REQ;
       }

       if (dataType==MONTH_FREEZE_COPY_DATA_M)
       {
       	 dataType = MONTH_FREEZE_COPY_DATA;
       }
       
       if (dataType==MONTH_FREEZE_COPY_REQ_M)
       {
       	 dataType = MONTH_FREEZE_COPY_REQ;
       }
       if (readMeterData(tmpBuff, pn, queryType, dataType , &tmpTimex, 0) == TRUE)
       {
	        //���ԭ���ն������ݵ������й�����,������û�г�����,��Ҫ����,����ԭ��������
	        if (tmpBuff[POSITIVE_WORK_OFFSET]!=0xee && buff[POSITIVE_WORK_OFFSET]==0xee)
	        {
	        	 if (debugInfo&PRINT_DATA_BASE)
	        	 {
	        	   printf("saveMeterData:�๦�ܵ��ԭ����/�¶������ݵ������й�����,������û�г�����,��Ҫ����,����ԭ��������\n");
	        	 }
	        	 
	        	 dbLocker = 0;
	        	 
	        	 return FALSE;
	        }
	        
  	      //�����û�ж�������ݸ��Ƶ�buff��,��ɵľ�����,������������ֱ�Ӷ�����,���û�е������Ǳ����ն����
     	    if (backDataType==DAY_FREEZE_COPY_DATA_M || backDataType==MONTH_FREEZE_COPY_DATA_M
     	    	|| backDataType==DAY_FREEZE_COPY_REQ_M || backDataType==MONTH_FREEZE_COPY_REQ_M)  //���б����ն�������,���вɼ��ĵ���ն�������
     	    {
  	        if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
  	        {
  		        if (queryType==THREE_LOCAL_CTRL_DAY && dataType==DAY_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_LOCAL_RECORD-METER_STATUS_WORD_T);
  	            }		        	 
  		        }
  
  		        if (queryType==THREE_REMOTE_CTRL_DAY && dataType==DAY_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_REMOTE_RECORD-METER_STATUS_WORD_T);
  	            }
  		        }
  
  		        //��07��Լ����û�й涨�����������޹��������޹���������Ķ�ȡ���ݱ�ʶ,����Ҫ������ǰ�Ƚ�����
  		        //ǰһ�ն������ݶ�����,�����й��������й����ó�������������������
  		        if (dataType==DAY_FREEZE_COPY_REQ)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[72], &tmpBuff[72], 72);
  	              memcpy(&buff[216], &tmpBuff[216], 72);
  	            }
  	          }
  	          
  	          pSqlStr = sqlite3_mprintf("update dayBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
  	                tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
  	        }
  	        else
  	        {
  		        if (queryType==THREE_LOCAL_CTRL_MONTH && dataType==MONTH_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_LOCAL_RECORD-METER_STATUS_WORD_T);
  	            }		        	 
  		        }
  
  		        if (queryType==THREE_REMOTE_CTRL_MONTH && dataType==MONTH_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_REMOTE_RECORD-METER_STATUS_WORD_T);
  	            }		        	 
  		        }
  		        
  		        if (dataType==MONTH_FREEZE_COPY_REQ)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[72], &tmpBuff[72], 72);
  	              memcpy(&buff[216], &tmpBuff[216], 72);
  	            }
  	          }
  
  	          pSqlStr = sqlite3_mprintf("update monthBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
  	                tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
  	        }
  	      }
  	      else
  	      {
            tmpTime = timeBcdToHex(tmpTimex);
            
            if (debugInfo&PRINT_DATA_BASE)
            {
              printf("saveMeterData-�������ʱ�䱣��Ϊ�ɼ��ĵ���ն�������ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
            }
            
            getLinuxFormatDateTime(&tmpTime,&tvxx,1);

  	        //���ɼ��ĵ���ն�������(�����й�����ʾֵ��)���Ƶ������ն������ݻ�����
  	        if (backDataType==DAY_FREEZE_COPY_DATA || backDataType==MONTH_FREEZE_COPY_DATA)
  	        {
  	          memcpy(buff, tmpBuff, 288);
            }
            else
            {
  	          memcpy(buff, tmpBuff, 72);
  	          memcpy(&buff[144], &tmpBuff[144], 72);
            }

	          if (backDataType==MONTH_FREEZE_COPY_DATA)
	          {
	            pSqlStr = sqlite3_mprintf("update monthBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
	                  tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
	          }
	          else
	          {
	            pSqlStr = sqlite3_mprintf("update dayBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
	                  tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
	          }
  	      }
	       
	       if (debugInfo&PRINT_DATA_BASE)
	       {
	         if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	         {
	           printf("saveMeterData:�๦�ܵ������ն�������\n");
	         }
	         else
	         {
	           printf("saveMeterData:�๦�ܵ������¶�������\n");
	         }
	       }
       }
       else
       {
	        if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	        {
	          pSqlStr = sqlite3_mprintf("insert into dayBalanceData values (%d,%d,%d,%d,%d,?);", pn, queryType, dataType, tv.tv_sec,tvxx.tv_sec);
	          
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("insert into monthBalanceData values (%d,%d,%d,%d,%d,?);", pn, queryType, dataType, tv.tv_sec,tvxx.tv_sec);
	        }
	        
	        if (debugInfo&PRINT_DATA_BASE)
	        {
	          if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	          {
	            printf("saveMeterData:�๦�ܵ������ն�������\n");
	          }
	          else
	          {
	            printf("saveMeterData:�๦�ܵ������¶�������\n");
	          }
	        }
	     }
	     
	     if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	     {
	       //�����ն������ݵ��ļ�(��������),2012-10-10,add
	       saveBakDayFile(pn, tv.tv_sec, tvxx.tv_sec, buff, dataType);
	     }

       break;
       
     case STATIS_DATA:          //���ͳ������
	     switch(dataType)
	     {
	     	 case 88:  //�������ʱ���޹���
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,88,0,0,?);", pn);
	         break;

	     	#ifdef LIGHTING
	     	 case 89:  //��·��������ʱ���޹���
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,89,0,0,?);", pn);
	         break;
	      #endif

	     	 case 99:  //�������ʱ���޹���
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,99,0,0,?);", pn);
	         break;
	       
	       default:  //�������ʱ���й���
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,%d,%d,%d,?);", pn, saveTime.day, saveTime.month, saveTime.year);
	         break;
	     }
     	 break;
     	 
     case SINGLE_PHASE_PRESENT:       //�����ǰ����
     case SINGLE_LOCAL_CTRL_PRESENT:  //���౾�طѿر�ǰ����
     case SINGLE_REMOTE_CTRL_PRESENT: //����Զ�̷ѿر�ǰ����
     	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime),0));
	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName,pn, tv.tv_sec);
       break;

     case SINGLE_PHASE_DAY:        //������ն�������
     case SINGLE_PHASE_MONTH:      //������¶�������
     case SINGLE_LOCAL_CTRL_DAY:   //���౾�طѿر��ն�������
     case SINGLE_LOCAL_CTRL_MONTH: //���౾�طѿر��¶�������
     case SINGLE_REMOTE_CTRL_DAY:  //����Զ�̷ѿر��ն�������
     case SINGLE_REMOTE_CTRL_MONTH://����Զ�̿ر��¶�������
       //��������
     	 if (dataType==DAY_FREEZE_COPY_DATA || dataType==MONTH_FREEZE_COPY_DATA)
     	 {
     	 	 //printf("dataType=%d\n", dataType);
     	 	 
     	 	 //07��Ϊ�����ʱ��     	 	 
   	 	   if (buff[DAY_FREEZE_TIME_FLAG_S]==0xee)
   	 	   {
   	 	 	   if (debugInfo&PRINT_DATA_BASE)
   	 	 	   {
   	 	 	     printf("saveMeterData:������ն���ʱ��δ����\n");
   	 	 	   }
   	 	 	   
   	 	 	   dbLocker = 0;
   	 	 	   
   	 	 	   return FALSE;
   	 	   }
   	 	   else
   	 	   {
   	 	     tmpTime.second = 0x0;
   	 	     tmpTime.minute = buff[DAY_FREEZE_TIME_FLAG_S+0];
   	 	     tmpTime.hour   = buff[DAY_FREEZE_TIME_FLAG_S+1];
   	 	     tmpTime.day    = buff[DAY_FREEZE_TIME_FLAG_S+2];
   	 	     tmpTime.month  = buff[DAY_FREEZE_TIME_FLAG_S+3];
   	 	     tmpTime.year   = buff[DAY_FREEZE_TIME_FLAG_S+4];
   	 	   }
     	 	 
     	 	 if (dataType==MONTH_FREEZE_COPY_DATA)
     	 	 {
     	 	 	 tmpTime = timeHexToBcd(backTime(timeBcdToHex(tmpTime), 0, 1, 0, 0, 0));
     	 	 }
     	 }
     	 else
     	 {
     	 	 tmpTime = saveTime;
     	 }
     	 tmpTimex = tmpTime;
       tmpTime = timeBcdToHex(tmpTime);
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("saveMeterData-�������ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
       
       if (queryType==SINGLE_PHASE_MONTH || queryType==SINGLE_LOCAL_CTRL_MONTH || queryType==SINGLE_REMOTE_CTRL_MONTH)
       {
       	 tmpTime.day = 1;
       }
       tmpTime.hour   = 0;
       tmpTime.minute = 0;
       tmpTime.second = 0;
       getLinuxFormatDateTime(&tmpTime,&tv,1);
       
       //����ʱ��
       tmpTime = timeBcdToHex(saveTime);
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("saveMeterData-�������ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
       getLinuxFormatDateTime(&tmpTime,&tvxx,1);
       
       if (readMeterData(tmpBuff, pn, queryType, ENERGY_DATA, &tmpTimex, 0) == TRUE)     //���б����ն���,�ٲɼ���������
       {
	        //���ԭ���ն������ݵ������й�����,������û�г�����,��Ҫ����,����ԭ��������
	        if (tmpBuff[POSITIVE_WORK_OFFSET]!=0xee && buff[POSITIVE_WORK_OFFSET]==0xee)
	        {
	        	 if (debugInfo&PRINT_DATA_BASE)
	        	 {
	        	   printf("ԭ����/�¶������ݵ������й�����,������û�г�����,��Ҫ����,����ԭ��������\n");
	        	 }
	        	 
	        	 dbLocker = 0;
	        	 
	        	 return FALSE;
	        }
     	    
  	      //�����û�ж�������ݸ��Ƶ�buff��,��ɵľ�����,������������ֱ�Ӷ�����,���û�е������Ǳ����ն����
     	    if (dataType==DAY_FREEZE_COPY_DATA || dataType==MONTH_FREEZE_COPY_DATA)  //���б����ն�������,���вɼ��ĵ���ն�������
     	    {
  	        switch(queryType)
  	        {
  	        	 case SINGLE_PHASE_DAY:
  	        	 case SINGLE_PHASE_MONTH:
  	        	 	 memcpy(&buff[METER_STATUS_WORD_S],&tmpBuff[METER_STATUS_WORD_S], 12);
  	        	 	 break;
  	        	 
  	        	 case SINGLE_LOCAL_CTRL_DAY:
  	        	 case SINGLE_LOCAL_CTRL_MONTH:
  	        	 	 memcpy(&buff[METER_STATUS_WORD_S],&tmpBuff[METER_STATUS_WORD_S], 50);
  	        	 	 if (debugInfo&PRINT_DATA_BASE)
  	        	 	 {
  	        	 	   printf("���Ʊ��طѿر�����/�¶�������\n");
  	        	 	 }
  	        	 	 break;
  
  	        	 case SINGLE_REMOTE_CTRL_DAY:
  	        	 case SINGLE_REMOTE_CTRL_MONTH:
  	        	 	 memcpy(&buff[METER_STATUS_WORD_S],&tmpBuff[METER_STATUS_WORD_S], 26);
  	        	 	 if (debugInfo&PRINT_DATA_BASE)
  	        	 	 {
  	        	 	   printf("����Զ�̷ѿر�����/�¶�������\n");
  	        	 	 }
  	        	 	 break;
  	        }
     	    }
     	    else           //���вɼ��ĵ���ն�������,���б����ն���
     	    {
  	        //���ɼ��ĵ���ն�������(�����й�����ʾֵ)���Ƶ������ն������ݻ�����
  	        memcpy(buff,tmpBuff, 40);
  	        
            tmpTime = timeBcdToHex(tmpTimex);
            
            if (debugInfo&PRINT_DATA_BASE)
            {
              printf("saveMeterData-�������ʱ�䱣��Ϊ�ɼ��ĵ���ն�������ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
            }
                        
            getLinuxFormatDateTime(&tmpTime,&tvxx,1);
  	      }
	        
	        if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	        {
	          pSqlStr = sqlite3_mprintf("update singlePhaseDay set copyTime=%d,data=? where pn=%d and freezeTime=%d;",
	               tvxx.tv_sec, pn, tv.tv_sec);
	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("�����:�����ն�������\n");
	          }
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("update singlePhaseMonth set copyTime=%d,data=? where pn=%d and freezeTime=%d;",
	               tvxx.tv_sec, pn, tv.tv_sec);
	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("�����:�����¶�������\n");
	          }
	        }
       }
       else
       {
	        //��ṹpSqlStr = "create table singlePhaseDay(pn int,freezeTime int, copyTime int,data blob);";
	        if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	        {
	          pSqlStr = sqlite3_mprintf("insert into singlePhaseDay values (%d,%d,%d,?);", pn,tv.tv_sec,tvxx.tv_sec);
	          
	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("�����ն�������\n");
	          }
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("insert into singlePhaseMonth values (%d,%d,%d,?);", pn,tv.tv_sec,tvxx.tv_sec);

	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("�����¶�������\n");
	          }
	        }
	     }
	      
	     if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	     {
	       //�����ն������ݵ��ļ�(��������)
	       saveBakDayFile(pn, tv.tv_sec, tvxx.tv_sec, buff, 1);
	     }
       break;

     case LEFT_POWER:          //ʣ�����
	     pSqlStr = sqlite3_mprintf("insert into leftPower values (%d,?);", pn);
     	 break;

     case DC_ANALOG:           //ֱ��ģ��������
	     pSqlStr = sqlite3_mprintf("insert into dcAnalog values (%d,%d,?);", pn, tv.tv_sec);
     	 break;

     case HOUR_FREEZE:         //ʵʱ��������
       tmpTime.second = 0x0;
       tmpTime.minute = buff[128];
       tmpTime.hour   = buff[129];
       tmpTime.day    = buff[130];
       tmpTime.month  = buff[131];
       tmpTime.year   = buff[132];
       
       tmpTimex = tmpTime;
       if (readMeterData(tmpBuff, pn, HOUR_FREEZE, 0x0, &tmpTimex, 0) == FALSE)
       {
         if (debugInfo&PRINT_DATA_BASE)
         {
           printf("saveMeterData: ������%d���㶳��ʱ��:%02x-%02x-%02x %02x:%02x:%02x\n", pn, tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
         }
         
         tmpTime = timeBcdToHex(tmpTime);
         getLinuxFormatDateTime(&tmpTime, &tv, 1);
       	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime), 2));
  	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName, pn, tv.tv_sec);
  	   }
  	   else
  	   {
	       if (debugInfo&PRINT_DATA_BASE)
	       {
	         printf("�Ѵ��ڸò����������Ķ�������\n");
	       }
	       
	       dbLocker = 0;
	       
	       return;
  	   }
       break;
     
     case HOUR_FREEZE_SLC:    //���ƿ�������Сʱ����,�洢ʱ�õ���ʮ��������
       tmpTimex = saveTime;
       if (readMeterData(tmpBuff, pn, HOUR_FREEZE_SLC, 0x0, &tmpTimex, 0) == FALSE)
       {
         if (debugInfo&PRINT_DATA_BASE)
         {
           printf("saveMeterData: ���Ƶ�%d���㶳��ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n", pn, tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
         }
         
         tmpTime = saveTime;
         getLinuxFormatDateTime(&tmpTime, &tv, 1);
       	 strcpy(tableName,bringTableName(saveTime, 2));
  	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName, pn, tv.tv_sec);
  	   }
  	   else
  	   {
	       if (debugInfo&PRINT_DATA_BASE)
	       {
	         printf("saveMeterData:���Ƶ�%d������Ķ��������Ѵ���\n", pn);
	       }
	       
	       dbLocker = 0;
	       
	       return;
  	   }
     	 break;
       
     default:
     	 printf("δָ���洢��,queryType=0x%02x,dataType=0x%02x\n", queryType, dataType);
     	 
     	 dbLocker = 0;
     	 
     	 return FALSE;
   }

	 if (debugInfo&PRINT_DATA_BASE)
	 {
	   printf("saveMeterData:SQL=");
	   printf(pSqlStr);
	   printf("\n");
	 }
	         
   //ֻҪ����ǿգ��Ͱ����е�����ȫ��д�����ݿ�
   if (buff!=NULL)
   {
	   if (pSqlStr==NULL)
	   {
	   	 printf("saveMeterData:pSqlStrָ��Ϊ��,���ܴ洢\n");
	   }
	   else
	   {
 	     sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
 	     sqlite3_bind_blob(stmt, 1, buff, len, NULL);
       if ((execResult=sqlite3_step(stmt))==SQLITE_DONE)
       {
         if (debugInfo&PRINT_DATA_BASE)
         {
        	 printf("saveMeterData:�������ݳɹ�!queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
         }
        	
         //�������ݿ��쳣��������,2012-09-04
         dbMonitor = 0;
       }
       else
       {
        	//�����쳣
          printf("saveMeterData happen aberrant.\n");
          logRun("saveMeterData happen aberrant.");
          sqlite3Aberrant(execResult);

        	if (queryType==SINGLE_PHASE_PRESENT)
        	{
        		 //��鵥�����ݴ洢���Ƿ����
        		 checkSpRealTable(0);
        	}
        	
          if (queryType==PRESENT_DATA || queryType==REAL_BALANCE || queryType==LAST_REAL_BALANCE || queryType==THREE_LOCAL_CTRL_PRESENT || queryType==THREE_REMOTE_CTRL_PRESENT)
        	{
        		 //����������ݴ洢���Ƿ����
        		 checkSpRealTable(1);
        	}
        	
        	if (
        		  queryType==HOUR_FREEZE
        		  || queryType== HOUR_FREEZE_SLC    //����ϵͳ�Ŀ��Ƶ����㶳������,2015-02-03,add this line
        		 )
        	{
        		 //������㶳�����ݴ洢���Ƿ����
        		 checkSpRealTable(2);
        	}
        	
        	if (debugInfo&PRINT_DATA_BASE)
        	{
            fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
        	  
        	  printf("saveMeterData:��������ʧ��!queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
        	}
       }
       
       sqlite3_finalize(stmt);
     }
   }
	 
	 if (pSqlStr!=NULL)
	 {
     sqlite3_free(pSqlStr);
   }
   
   dbLocker = 0;
}

/*******************************************************
��������:readBakDayFile
��������:���ļ��ж�ȡ�ն�������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
BOOL readBakDayFile(INT16U pn, DATE_TIME *time, INT8U *buf, INT8U type)
{
	FILE           *fp;
	char           fileName[20];
  struct timeval tv;                  //Linux timeval
  DATE_TIME      tmpTime;
  SP_F_DAY       tmpSpfDay;
  MP_F_DAY       tmpMpfDay;

	//���������ļ���
	if (type==1)
	{
	  sprintf(fileName, "/data/spd%02x%02x%02x", time->year, time->month, time->day);
	}
	else
	{
	  sprintf(fileName, "/data/mpd%02x%02x%02x", time->year, time->month, time->day);
	}
	
  if((fp=fopen(fileName,"rb"))==NULL)
  {
  	if(debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("readBakDayFile:���ļ�%sʧ��\n", fileName);
  	}
  	
  	return FALSE;
  }

  if (feof(fp))
  {
  	fclose(fp);

  	if(debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("readBakDayFile:�ļ�%s��,�޼�¼\n", fileName);
  	}

  	return FALSE;
  }
  else
  {
    if (type==1)
    {
      while (!feof(fp))
      {
      	if (fread(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
      	{
      	  fclose(fp);
      	  return FALSE;
      	}
        
        if (tmpSpfDay.pn==pn)
        {
        	break;
        }
      }
      
      tv.tv_sec = tmpSpfDay.copyTime;
      getLinuxFormatDateTime(time, &tv, 2);    //����ʱ��
      *time = timeHexToBcd(*time);
      memcpy(buf, tmpSpfDay.data, 40);         //����
    }
    else
    {
      while (!feof(fp))
      {
      	if (fread(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
      	{
      	  fclose(fp);
      	  return FALSE;
      	}
        
        if (tmpMpfDay.pn==pn && tmpMpfDay.type==type)
        {
        	break;
        }
      }
      
      tv.tv_sec = tmpMpfDay.copyTime;
      getLinuxFormatDateTime(time, &tv, 2);    //����ʱ��
      *time = timeHexToBcd(*time);
      memcpy(buf, tmpMpfDay.data, 288);         //����
    }
    
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("readBakDayFile:��ȡ�ļ�%s������%d�����ݳɹ�\n", fileName, pn);
    }
  }
  
  fclose(fp);
  return TRUE;
}

/*******************************************************
��������:readMeterData
��������:��ȡ���ݿ��еĵ������(��ǰ����,����(��һ������)����,��������)
���ú���:
�����ú���:
�������:*tmpData-���ݻ���ָ��
         pn-��������ܼ����,
         queryType--��ѯ����
         dataType-��������,
         time-��ѯʱ��
�������:
����ֵ:TRUE(��ѯ�ɹ�) or FALSE(��ѯʧ��)
*******************************************************/
//2018-10-10,Add,��һ�γɹ������������ݻ���
INT8U     curveDataBuff[LENGTH_OF_PARA_RECORD];    //��һ�γɹ����ҵ��������ݵĻ���
INT16U    curvePn=0;                               //��һ�γɹ����ҵ��������ݵ�Pn
INT8U     curveQueryType=0;                        //��һ�γɹ����ҵ��������ݵĲ�ѯ����
INT8U     curveDataType=0;                         //��һ�γɹ����ҵ��������ݵ���������
DATE_TIME curveTime;                               //��һ�γɹ����ҵ��������ݵĲ�ѯʱ��
INT8U     lastCurveSuccess=0;                      //��һ���Ƿ�ɹ����ҵ�����?
BOOL readMeterData(INT8U *tmpData, INT16U pn, INT8U queryType, INT16U dataType, DATE_TIME *time, INT8U mix)
{
	  INT32U                   storeNox;
	  char                     strTableName[20];
	  sqlite3_stmt             *stmt;
	  const char               *tail;
	  char                     *pSqlStr;                //SQL����ַ���ָ��
	  int                      execResult;
	  INT8U                    ifFind = 0;
	  INT16U                   lenOfRecord;
	  METER_STATIS_EXTRAN_TIME *pMeterStatisExtranTime; //��ʱ���޹صĵ��ͳ�Ƽ�¼
	  METER_STATIS_BEARON_TIME *pMeterStatisBearonTime; //��ʱ���йصĵ��ͳ�Ƽ�¼
    struct timeval           tv,tv1;                  //Linux timeval
    DATE_TIME                tmpTime,tmpTime2;
    
    pSqlStr = NULL;
		
		
    if (queryType==EVENT_RECORD)
    {
    	storeNox = dataType;
    	dataType = 0x01;
    }
    
    if (debugInfo&PRINT_DATA_BASE)
    {
			printf("readMeterData(%d-%d-%d %d:%d:%d):��ѯʱ��:%02x-%02x-%02x %02x:%02x:%02x ������:%d,��ѯ����=%02x,��������=%02x\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, time->year,time->month,time->day,time->hour,time->minute,time->second,pn,queryType,dataType);
    }
    
    //2012-09-05
    if (time->year==0x0 && time->month==0x00 && time->day==0x0)
    {
      printf("readMeterData(%d-%d-%d %d:%d:%d):��ѯʱ��Ϊȫ0,�쳣����,����ѯֱ���˳�\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
    	
    	return FALSE;
    }

		//2018-10-10,���߻���
		if (queryType==CURVE_DATA_PRESENT)
		{
			if (1==lastCurveSuccess)
			{
				if (
					 pn==curvePn 
					 	&& queryType==curveQueryType 
					 	 && dataType==curveDataType 
					 	  && time->year==curveTime.year
							 && time->month==curveTime.month
								&& time->day==curveTime.day
								 && time->hour==curveTime.hour
									&& time->minute==curveTime.minute
									 && time->second==curveTime.second
					 )
				{
					memcpy(tmpData, curveDataBuff, LENGTH_OF_PARA_RECORD);

					if (debugInfo&PRINT_DATA_BASE)
					{
						printf("%s(%d-%d-%d %d:%d:%d):��ѯʱ��:%02x-%02x-%02x %02x:%02x:%02x ������:%d,��ѯ����=%02x,��������=%02x,���������߻�����,����������\n",__func__,sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, time->year,time->month,time->day,time->hour,time->minute,time->second,pn,queryType,dataType);
					}

					return TRUE;
				}
					 
			 	//��һ�����������Ƿ���ҳɹ���Ϊδ�ɹ�,2018-10-10
			 	lastCurveSuccess = 0;
			}
		}
		
    switch(dataType)
    {
    	 case ENERGY_DATA:
    	 case POWER_PARA_LASTMONTH:
    	 	 switch(queryType)
    	 	 {
    	 	 	 case SINGLE_PHASE_PRESENT:
    	 	 	 case CURVE_SINGLE_PHASE:
    	 	 	 case SINGLE_PHASE_DAY:
    	 	 	 case SINGLE_PHASE_MONTH:
    	 	 	   lenOfRecord = LENGTH_OF_SINGLE_ENERGY_RECORD;
    	 	 	   break;
    	 	 	   
    	 	 	 case SINGLE_LOCAL_CTRL_PRESENT:
    	 	 	 case SINGLE_LOCAL_CTRL_DAY:
    	 	 	 case SINGLE_LOCAL_CTRL_MONTH:
    	 	 	 	 lenOfRecord = LENGTH_OF_SINGLE_LOCAL_RECORD;
    	 	 	 	 break;

    	 	 	 case SINGLE_REMOTE_CTRL_PRESENT:
    	 	 	 case SINGLE_REMOTE_CTRL_DAY:
    	 	 	 case SINGLE_REMOTE_CTRL_MONTH:
    	 	 	 	 lenOfRecord = LENGTH_OF_SINGLE_REMOTE_RECORD;
    	 	 	 	 break;
    	 	 	 
    	 	 	 case THREE_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_THREE_ENERGY_RECORD;
    	 	 	 	 break;

    	 	 	 case THREE_REMOTE_CTRL_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_THREE_REMOTE_RECORD;
    	 	 	 	 break;

    	 	 	 case THREE_LOCAL_CTRL_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_THREE_LOCAL_RECORD;
    	 	 	 	 break;
    	 	 	 	 
    	 	 	 case KEY_HOUSEHOLD_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_KEY_ENERGY_RECORD;
    	 	 	 	 break;
    	 	 	 	 
    	 	 	 case CURVE_KEY_HOUSEHOLD:
    	 	 	 	 lenOfRecord = LENGTH_OF_KEY_ENERGY_RECORD;
    	 	 	 	 break;
    	 	 	   
    	 	 	 default:
    	 	    lenOfRecord = LENGTH_OF_ENERGY_RECORD;
    	 	    break;
    	 	 }
    	 	 break;
    	 	 
    	 case REQ_REQTIME_DATA:
    	 case REQ_REQTIME_LASTMONTH:
    	 	 lenOfRecord = LENGTH_OF_REQ_RECORD;
    	 	 break;
    	 	 
    	 case PARA_VARIABLE_DATA:
    	 	 if (queryType==KEY_HOUSEHOLD_PRESENT)
    	 	 {
    	 	   lenOfRecord = LENGTH_OF_KEY_PARA_RECORD;
    	 	 }
    	 	 else
    	 	 {
    	 	   lenOfRecord = LENGTH_OF_PARA_RECORD;
    	 	 }
    	 	 break;
    	 	 
    	 case SHI_DUAN_DATA:
    	 	 lenOfRecord = LENGTH_OF_SHIDUAN_RECORD;
    	 	 break;
    	 	 
    	 case REAL_BALANCE_POWER_DATA:
    	 case DAY_BALANCE_POWER_DATA:
    	 case MONTH_BALANCE_POWER_DATA:
    	 	 lenOfRecord = LEN_OF_ENERGY_BALANCE_RECORD;
    	 	 break;
    	 	 
    	 case REAL_BALANCE_PARA_DATA:
    	 case DAY_BALANCE_PARA_DATA:
    	 case MONTH_BALANCE_PARA_DATA:
    	 	 lenOfRecord = LEN_OF_PARA_BALANCE_RECORD;
    	 	 break;
    	 	 
    	 case DAY_FREEZE_COPY_DATA:
    	 case DAY_FREEZE_COPY_DATA_M:
    	 case COPY_FREEZE_COPY_DATA:
    	 case MONTH_FREEZE_COPY_DATA:
    	 case MONTH_FREEZE_COPY_DATA_M:
    	 	 switch (queryType)
    	 	 {
    	 	 	 case THREE_DAY:
    	 	 	 case THREE_MONTH:
    	 	     lenOfRecord = LENGTH_OF_THREE_ENERGY_RECORD;
    	 	 	 	 break;

    	 	 	 case THREE_LOCAL_CTRL_DAY:
    	 	 	 case THREE_LOCAL_CTRL_MONTH:
    	 	     lenOfRecord = LENGTH_OF_THREE_LOCAL_RECORD;    	 	 	 	 
    	 	 	 	 break;

    	 	 	 case THREE_REMOTE_CTRL_DAY:
    	 	 	 case THREE_REMOTE_CTRL_MONTH:
    	 	     lenOfRecord = LENGTH_OF_THREE_REMOTE_RECORD;    	 	 	 	 
    	 	 	 	 break;
    	 	 	 
    	 	 	 case KEY_HOUSEHOLD_DAY:
    	 	 	 case KEY_HOUSEHOLD_MONTH:
    	 	     lenOfRecord = LENGTH_OF_KEY_ENERGY_RECORD;
    	 	 	   break;
    	 	 	 
    	 	 	 default:
    	 	     lenOfRecord = LENGTH_OF_ENERGY_RECORD;
    	 	 }
    	 	 break;
    	 	 
    	 case DAY_FREEZE_COPY_REQ:
    	 case DAY_FREEZE_COPY_REQ_M:
    	 case COPY_FREEZE_COPY_REQ:
    	 case MONTH_FREEZE_COPY_REQ:
    	 case MONTH_FREEZE_COPY_REQ_M:
    	 	 lenOfRecord = LENGTH_OF_REQ_RECORD;
    	 	 break;
    	 	 
    	 case GROUP_REAL_BALANCE:
    	 case GROUP_DAY_BALANCE:
    	 case GROUP_MONTH_BALANCE:
    	 	 lenOfRecord = LEN_OF_ZJZ_BALANCE_RECORD;
    	 	 break;
    	 
    	 case DC_ANALOG_CURVE_DATA:
    	 	 lenOfRecord = 2;
    	 	 break;
    	 
    	 default:
    	 	 if (queryType==STATIS_DATA)
    	 	 {
					 if (pn==0)
    	 	   {
    	 	     lenOfRecord = sizeof(TERMINAL_STATIS_RECORD);
    	 	   }
    	 	   else
    	 	   {
    	 	     switch (dataType)
    	 	     {
    	 	     	 case 88:   //�������ʱ���޹���
    	 	         lenOfRecord = sizeof(METER_STATIS_EXTRAN_TIME);
    	 	         break;
    	 	      
    	 	      #ifdef LIGHTING
    	 	     	 case 89:   //��·��������ʱ���޹���
							   lenOfRecord = sizeof(KZQ_STATIS_EXTRAN_TIME);
    	 	         break;
    	 	      #endif

    	 	     	 case 99:   //�������ʱ���޹���
    	 	         lenOfRecord = sizeof(METER_STATIS_EXTRAN_TIME_S);
    	 	         break;
    	 	         
    	 	       default:   //�������ʱ���й���
    	 	         lenOfRecord = sizeof(METER_STATIS_BEARON_TIME);
    	 	         break;
    	 	     }
    	 	   }
    	 	 }
    	 	 else
    	 	 {
      	   if (queryType==LEFT_POWER)
      	   {
      	   	 lenOfRecord = 12;
      	   }
      	   else
      	   {
      	     if (queryType==HOUR_FREEZE)
      	     {
      	   	   lenOfRecord = LENGTH_OF_HOUR_FREEZE_RECORD;
      	     }
      	     else
      	     {
      	       if (HOUR_FREEZE_SLC==queryType)    //2014-09-15,add
      	       {
      	       	 lenOfRecord = LEN_OF_LIGHTING_FREEZE;
      	       }
      	       else
      	       {
      	         if (debugInfo&PRINT_DATA_BASE)
      	         {
    	 	           printf("δ֪��¼����\n");
    	 	         }
    	 	         return FALSE;
    	 	       }
    	 	     }
    	 	   }
    	 	 }
    	 	 break;    	 	 
    }

    if (flagOfClearData==0x55 || flagOfClearData==0xaa)
    {
      //��Ϊ��������ʱҪ����ʣ�����,����ʣ���������
      if (queryType==LEFT_POWER && dataType==0)
      {
     	  ;
      }
      else
      {
        //if (debugInfo&PRINT_DATA_DEBUG)
        //{
          printf("readMeterData:���������,��ֹ��ȡ����,queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
        //}
	    
	      memset(tmpData, 0xee, lenOfRecord);

   	    return FALSE;
   	  }
    }

    //�����ݿ����
	  switch (queryType)
	  {
   	  case PRESENT_DATA:
   	  case REAL_BALANCE:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
        tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time=%d",strTableName,
	              pn, queryType, dataType, tv.tv_sec);
   	  	break;

   	  case THREE_PRESENT:
   	  case THREE_LOCAL_CTRL_PRESENT:
   	  case THREE_REMOTE_CTRL_PRESENT:
   	  case KEY_HOUSEHOLD_PRESENT:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
        tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time desc limit 1",strTableName,
	              pn, queryType, dataType);
   	  	break;

      case FIRST_TODAY:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time limit 1",strTableName,
	              pn, PRESENT_DATA, dataType);
      	break;
      	
   	  case LAST_REAL_BALANCE:    //��ȡ��һ��ʵʱ����Ľ��
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
        
        //ly,2011-10-18,add this 2 lines
        tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	      
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time<=%d order by time desc limit 1",strTableName,
	              pn, REAL_BALANCE, dataType,tv.tv_sec);
      	break;
      	
      case LAST_TODAY:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time desc limit 1",strTableName,
	              pn, PRESENT_DATA, dataType);
      	break;

      case LAST_LAST_REAL_DATA:  //���ϴ�ʵʱ����
        tmpTime = timeBcdToHex(*time);
        //printf("tmpTime=%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
        tmpTime = backTime(tmpTime, 0, 0, 0, mix, 0);
        //printf("���˺��tmpTime=%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
        
        getLinuxFormatDateTime(&tmpTime, &tv, 1);
        strcpy(strTableName, bringTableName(tmpTime, 1));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time<=%d order by time desc limit 1",strTableName,
	              pn, PRESENT_DATA, dataType, tv.tv_sec);
      	break;
      	
      case FIRST_MONTH:          //���µĵ�һ������
	    	tmpTime  = timeBcdToHex(*time);
	    	tmpTime2 = tmpTime;

	    	tmpTime.day    = 1;
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
	    	while(tmpTime.day<=tmpTime2.day)
	    	{
           strcpy(strTableName,bringTableName(tmpTime,1));
	         pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time limit 1",strTableName,
	              pn, PRESENT_DATA, dataType);
	         
	         if (debugInfo&PRINT_DATA_BASE)
	         {
	           printf("readMeterData:���ҵ��µ�һ����SQL");
	           printf(pSqlStr);
             printf("\n");
           }
           
           if (sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail)!= SQLITE_OK)
           {
             if (debugInfo&PRINT_DATA_BASE)
             {
               fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
             }
           }
           else
           {
             execResult = sqlite3_step(stmt);
             if(execResult==SQLITE_ROW)
             {
             	 break;
             }
           }
           
           tmpTime.day++;
	    	}
      	break;

    	case CURVE_DATA_PRESENT:
    	case CURVE_KEY_HOUSEHOLD:
   	  case CURVE_DATA_BALANCE:
			  strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	    	tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("������������ݲ�����ʼʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}

        tmpTime = nextTime(timeBcdToHex(*time), bcdToHex(mix), 0);
        getLinuxFormatDateTime(&tmpTime, &tv1, 1);
	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("������������ݲ��ҽ���ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}
        
	      if (queryType==CURVE_DATA_PRESENT)
	      {
	       #ifdef LIGHTING
	        pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>%d and time<=%d order by time desc limit 1",strTableName,
	                    pn, PRESENT_DATA, dataType, tv.tv_sec, tv1.tv_sec);
	       #else
	        pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<%d order by time desc limit 1",strTableName,
	                    pn, PRESENT_DATA, dataType, tv.tv_sec, tv1.tv_sec);
	       #endif
	      }
	      else
	      {
	        if (queryType==CURVE_KEY_HOUSEHOLD)
	        {
	          pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<%d order by time desc limit 1",strTableName,
	                      pn, KEY_HOUSEHOLD_PRESENT, dataType, tv.tv_sec, tv1.tv_sec);
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<%d order by time desc",strTableName,
	                      pn, REAL_BALANCE, dataType, tv.tv_sec, tv1.tv_sec);
	        }
	      }

				
    		break;

      case DC_ANALOG:    //ֱ��ģ������������
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	    	tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        tmpTime = nextTime(timeBcdToHex(*time),bcdToHex(mix),0);
        getLinuxFormatDateTime(&tmpTime,&tv1,1);
	      pSqlStr = sqlite3_mprintf("select * from dcAnalog where pn=%d and copyTime>=%d and copyTime<%d order by copyTime desc limit 1", pn, tv.tv_sec, tv1.tv_sec);
      	break;

      case DAY_BALANCE:
      case MONTH_BALANCE:
      case THREE_DAY:
      case THREE_MONTH:
      case THREE_LOCAL_CTRL_DAY:
      case THREE_LOCAL_CTRL_MONTH:
      case THREE_REMOTE_CTRL_DAY:
      case THREE_REMOTE_CTRL_MONTH:
      case KEY_HOUSEHOLD_DAY:
      case KEY_HOUSEHOLD_MONTH:
	    	tmpTime = timeBcdToHex(*time);
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime, &tv, 1);
        if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
        {
	        if (queryType==DAY_BALANCE && dataType==MONTH_BALANCE_PARA_DATA)  //���²α���ͳ������,ȡ�������һ������
	        {
	    	    tmpTime = timeBcdToHex(*time);
	    	    tmpTime.hour   = 0;
	    	    tmpTime.minute = 0;
	    	    tmpTime.second = 0;
	    	    tmpTime.day    = 1;
            getLinuxFormatDateTime(&tmpTime, &tv, 1);
	          pSqlStr = sqlite3_mprintf("select * from dayBalanceData where pn=%d and queryType=%d and dataType=%d and freezeTime>%d order by freezeTime desc limit 1",
	                      pn, queryType, dataType, tv.tv_sec);
	        }
	        else
	        {
	           pSqlStr = sqlite3_mprintf("select * from dayBalanceData where pn=%d and queryType=%d and dataType=%d and freezeTime=%d limit 1",
	                      pn, queryType, dataType, tv.tv_sec);
	        }
      	}
      	else
      	{
	        pSqlStr = sqlite3_mprintf("select * from monthBalanceData where pn=%d and queryType=%d and dataType=%d and freezeTime=%d limit 1",
	                    pn, queryType, dataType, tv.tv_sec);
      	}
      	break;

      case LAST_MONTH_DATA:
	    	tmpTime = timeBcdToHex(*time);
	    	tmpTime.day    = 1;
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime,&tv,1);        
        tmpTime.day = monthDays(2000+tmpTime.year,tmpTime.month);
	    	tmpTime.hour   = 23;
	    	tmpTime.minute = 59;
	    	tmpTime.second = 59;
        getLinuxFormatDateTime(&tmpTime,&tv1,1);
        if (queryType==LAST_MONTH_DATA)
        {
        	 strcpy(strTableName,"lastMonthData");
        }
        else
        {
        	 strcpy(strTableName,"monthBalanceData");
        }
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<=%d order by time desc limit 1",strTableName,
	                    pn, queryType, dataType, tv.tv_sec, tv1.tv_sec);
      	break;
   	   	
	  	case EVENT_RECORD:
	      if (pn==1)
	      {
	        if (debugInfo&PRINT_EVENT_DEBUG)
	        {
	          printf("��ȡ��Ҫ�¼��洢���=%d\n",storeNox+iEventStartPtr);
	        }
	        
	        pSqlStr = sqlite3_mprintf("select * from eventRecord where storeNo=%d and eventType=%d;", storeNox+iEventStartPtr, pn);
	      }
	      else
	      {
	        if (debugInfo&PRINT_EVENT_DEBUG)
	        {
	          printf("��ȡһ���¼��洢���=%d\n",storeNox+nEventStartPtr);
	        }
	        pSqlStr = sqlite3_mprintf("select * from eventRecord where storeNo=%d and eventType=%d;", storeNox+nEventStartPtr, pn);
	      }
	      break;
	      
	    case STATIS_DATA:
	      switch (dataType)
	      {
	      	case 88:    //�������ʱ���޹صļ�¼
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=88;", pn);
	          break;

	       #ifdef LIGHTING
	      	case 89:    //��·��������ʱ���޹صļ�¼
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=89;", pn);
	          break;
	       #endif

	      	case 99:    //�������ʱ���޹صļ�¼
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=99;", pn);
	          break;
	        
	        default:    //�������ʱ���޹صļ�¼
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=%d and month=%d and year=%d;", pn, time->day, time->month, time->year);
	          break;
	      }
	      break;
	      
	    case SINGLE_PHASE_PRESENT:
	    case SINGLE_LOCAL_CTRL_PRESENT:
	    case SINGLE_REMOTE_CTRL_PRESENT:
	      strcpy(strTableName,bringTableName(timeBcdToHex(*time),0));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d order by time desc",strTableName,
	              pn, tv.tv_sec,tv1.tv_sec);
	    	break;

	    case CURVE_SINGLE_PHASE:
	      strcpy(strTableName,bringTableName(timeBcdToHex(*time),0));
	    	tmpTime = timeBcdToHex(*time);

	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("������������ݲ�����ʼʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}
	    	
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        tmpTime = nextTime(timeBcdToHex(*time),bcdToHex(mix),0);
        getLinuxFormatDateTime(&tmpTime,&tv1,1);

	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("������������ݲ��ҽ���ʱ��:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}
        
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and time>=%d and time<%d order by time desc",strTableName,
	              pn, tv.tv_sec, tv1.tv_sec);
	      break;

	    case SINGLE_PHASE_DAY:
	    case SINGLE_PHASE_MONTH:
	    case SINGLE_LOCAL_CTRL_DAY:
	    case SINGLE_LOCAL_CTRL_MONTH:
	    case SINGLE_REMOTE_CTRL_DAY:
	    case SINGLE_REMOTE_CTRL_MONTH:
	    	tmpTime = timeBcdToHex(*time);
	    	if (queryType==SINGLE_PHASE_MONTH || queryType==SINGLE_LOCAL_CTRL_MONTH || queryType==SINGLE_REMOTE_CTRL_MONTH)
	    	{
	    	  tmpTime.day = 1;
	    	}
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        
        if (debugInfo&PRINT_DATA_BASE)
        {
          printf("readMeterData:tv.sec=%d\n", tv.tv_sec);
        }

	      if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	      {
	         pSqlStr = sqlite3_mprintf("select * from singlePhaseDay where pn=%d and freezeTime=%d", pn, tv.tv_sec);
	      }
	      else
	      {
           pSqlStr = sqlite3_mprintf("select * from singlePhaseMonth where pn=%d and freezeTime=%d", pn, tv.tv_sec);
	      }
	      break;
	      
	    case LEFT_POWER:       //ʣ�����
        pSqlStr = sqlite3_mprintf("select * from leftPower where pn=%d", pn);
	    	break;
	    	
	    case HOUR_FREEZE:
	      strcpy(strTableName, bringTableName(timeBcdToHex(*time), 2));
	    	tmpTime = timeBcdToHex(*time);
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and freezeTime=%d", strTableName, pn, tv.tv_sec);
	      break;

	    case HOUR_FREEZE_SLC:    //���ƿ�������Сʱ����,�洢ʱ�õ���ʮ��������
	      strcpy(strTableName,bringTableName(*time, 2));
	    	tmpTime = *time;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime, &tv, 1);
        
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and freezeTime=%d", strTableName, pn, tv.tv_sec);
	      break;
	      
	    default:
	      if (debugInfo&PRINT_DATA_BASE)
	      {
	        printf("��ȡ���� - ��ѯ����Ϊ:0x%02x,��������Ϊ0x%02x\n",queryType,dataType);
	      }
	      break;
    }
    
	  if (debugInfo&PRINT_DATA_BASE)
	  {
	    printf(pSqlStr);
	    printf("\n");
	  }
	  
	  if (pSqlStr==NULL)
	  {
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readMeterData:pSqlStrΪ��ָ��,����FALSE\n");
      }
      
	    memset(tmpData, 0xee, lenOfRecord);
	     
	    return FALSE;
	  }

		//gettimeofday(&tv,NULL);
		//printf("%s==>4,%ldms\n", __func__, tv.tv_sec*1000 + tv.tv_usec/1000);
		
    if ((execResult=sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail))!= SQLITE_OK)
    {
      if (debugInfo&PRINT_DATA_BASE)
      {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
        
        //�����쳣      
        if (execResult!=101)
        {
          printf("readMeterData(1)->");
        }
      }
      
      logRun("readMeterData happen aberrant.");
      sqlite3Aberrant(execResult);
      
      if (execResult==SQLITE_ERROR)
      {
      	checkSpRealTable(0);    //�����ʵʱ���ݱ�
      	
      	checkSpRealTable(1);    //�����ʵʱ���ݱ�
      }
    }
    else
    {
      execResult = sqlite3_step(stmt);
      
      if(execResult==SQLITE_ROW)
      {
        switch(queryType)
        {
        	case EVENT_RECORD:
            lenOfRecord = sqlite3_column_int(stmt, 2);
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 3),lenOfRecord);
            break;
            
          case STATIS_DATA:
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);
            break;

          case LEFT_POWER:    //ʣ�����
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 1),lenOfRecord);
            break;

          case DC_ANALOG:     //ֱ��ģ����
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 2),lenOfRecord);
            break;
          
          case SINGLE_PHASE_PRESENT:
          case SINGLE_LOCAL_CTRL_PRESENT:
          case SINGLE_REMOTE_CTRL_PRESENT:
          case CURVE_SINGLE_PHASE:
            ifFind=0xee;
            while(execResult==SQLITE_ROW)
            {
              tv.tv_sec = sqlite3_column_int(stmt,1);
              getLinuxFormatDateTime(time,&tv,2);
              *time = timeHexToBcd(*time);
              bzero(tmpData,lenOfRecord);
              memcpy(tmpData,sqlite3_column_blob(stmt, 2),lenOfRecord);
              
              //�����й�����ʾֵ��������
              if (tmpData[0]!=0xee)
              {
                ifFind = 2;
                break;
              }
              
              execResult = sqlite3_step(stmt);
            }
            break;
  
          case SINGLE_PHASE_DAY:
          case SINGLE_PHASE_MONTH:
          case SINGLE_LOCAL_CTRL_DAY:
          case SINGLE_LOCAL_CTRL_MONTH:
          case SINGLE_REMOTE_CTRL_DAY:
          case SINGLE_REMOTE_CTRL_MONTH:
            tv.tv_sec = sqlite3_column_int(stmt,2);
            getLinuxFormatDateTime(time,&tv,2);
            *time = timeHexToBcd(*time);
          	
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 3),lenOfRecord);
            break;
            
          //case LAST_LAST_REAL_DATA:  //���ϴ�ʵʱ����
            // tv.tv_sec = sqlite3_column_int(stmt,3);
            // getLinuxFormatDateTime(time,&tv,2);
            // *time = timeHexToBcd(*time);
            // bzero(tmpData,lenOfRecord);
            // memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);
            
            //#else
            // tmpTime = timeBcdToHex(*time);
            // getLinuxFormatDateTime(&tmpTime,&tv,1);
            // ifFind = 0xee;
            // while(execResult==SQLITE_ROW)
            // {
            //   tv1.tv_sec = sqlite3_column_int(stmt,3);
            //  
            //   if (tv1.tv_sec<tv.tv_sec)
            //   {
            //     getLinuxFormatDateTime(time,&tv1,2);
            //     *time = timeHexToBcd(*time);
            //     bzero(tmpData,lenOfRecord);
            //     memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);
            //     ifFind = 1;
            //     break;
            //   }
            //   execResult = sqlite3_step(stmt);
            // }
            //#endif
          	break;
          	
   	      case DAY_BALANCE:
   	      case MONTH_BALANCE:
   	      case THREE_DAY:
   	      case THREE_MONTH:
   	      case THREE_LOCAL_CTRL_DAY:
   	      case THREE_LOCAL_CTRL_MONTH:
   	      case THREE_REMOTE_CTRL_DAY:
   	      case THREE_REMOTE_CTRL_MONTH:
   	      case KEY_HOUSEHOLD_DAY:
   	      case KEY_HOUSEHOLD_MONTH:
            //pSqlStr = "create table dayBalanceData(pn int,queryType int,dataType int,freezeTime int, copyTime int,data blob);";
            tv.tv_sec = sqlite3_column_int(stmt,4);
            getLinuxFormatDateTime(time,&tv,2);
            *time = timeHexToBcd(*time);
            
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 5),lenOfRecord);
   	      	break;
   	      	
    	    case CURVE_DATA_PRESENT:
   	      case PRESENT_DATA:
          case LAST_TODAY:
          case FIRST_TODAY:
          case FIRST_MONTH:          //���µĵ�һ������
   	      case REAL_BALANCE:
   	      case CURVE_DATA_BALANCE:
   	      case LAST_REAL_BALANCE:    //��ȡ��һ��ʵʱ����Ľ��
          case LAST_LAST_REAL_DATA:  //���ϴ�ʵʱ����
   	      case LAST_MONTH_DATA:
   	      case THREE_PRESENT:
   	      case THREE_LOCAL_CTRL_PRESENT:
   	      case THREE_REMOTE_CTRL_PRESENT:
   	      case KEY_HOUSEHOLD_PRESENT:
   	      case CURVE_KEY_HOUSEHOLD:
						
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);

						if (CURVE_DATA_PRESENT==queryType && PARA_VARIABLE_DATA==dataType)
						{
							memcpy(curveDataBuff, tmpData, lenOfRecord);
							curvePn = pn;
							curveQueryType = queryType;
							curveDataType = dataType;
							curveTime = *time;
							lastCurveSuccess = 1;
						}
						
						tv.tv_sec = sqlite3_column_int(stmt,3);
						getLinuxFormatDateTime(time,&tv,2);
						*time = timeHexToBcd(*time);
   	      	break;

   	      case HOUR_FREEZE:
            tv.tv_sec = sqlite3_column_int(stmt,1);
            getLinuxFormatDateTime(time,&tv,2);
            *time = timeHexToBcd(*time);
            bzero(tmpData, lenOfRecord);
            memcpy(tmpData, sqlite3_column_blob(stmt, 2), lenOfRecord);
   	      	break;

   	      case HOUR_FREEZE_SLC:
            tv.tv_sec = sqlite3_column_int(stmt,1);
            getLinuxFormatDateTime(time,&tv,2);
            bzero(tmpData, lenOfRecord);
            memcpy(tmpData, sqlite3_column_blob(stmt, 2), lenOfRecord);
   	      	break;
            
          default:
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 2),lenOfRecord);
            break;
        }
        
        if (ifFind==0xee)
        {
        	ifFind = 0;
        }
        else
        {
          ifFind = 1;
        }
      }
      else
      {
        //�����쳣
        if (execResult!=SQLITE_DONE)
        {
          logRun("readMeterData(2) happen aberrant.");
          
          if (debugInfo&PRINT_DATA_BASE)
          {
            printf("readMeterData(2) happen aberrant.\n");
          }
        }
        
        sqlite3Aberrant(execResult);
      }
  	  
  	  sqlite3_finalize(stmt);
  	}

	  sqlite3_free(pSqlStr);

    //���ݲ�ѯ�������������
    if (ifFind == 0)
    {
	     if (debugInfo&PRINT_DATA_BASE)
	     {
	       printf("readMeterData(%d-%d-%d %d:%d:%d):δ�ҵ�����:pn=%d,��ѯ����Ϊ:0x%02x,��������Ϊ0x%02x,����=%d\n\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pn,queryType,dataType,lenOfRecord);
	     }
	     
	     memset(tmpData, 0xee, lenOfRecord);
	     
	     return FALSE;
    }
    else
    { 
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readMeterData(%d-%d-%d %d:%d:%d):�ҵ�����:pn=%d,��ѯ����Ϊ:0x%0x,��������Ϊ0x%02x,����ʱ��:%02x-%02x-%02x %02x:%02x:%02x,����=%d\n\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,pn,queryType,dataType,time->year,time->month,time->day,time->hour,time->minute,time->second,lenOfRecord);
      }
    }
		
		//gettimeofday(&tv,NULL);
		//printf("%s==>5,%ldms\n", __func__, tv.tv_sec*1000 + tv.tv_usec/1000);

    return TRUE;
}

/*******************************************************
��������:writeEvent
��������:д�¼���¼��FLASH
���ú���:
�����ú���:
�������:
�������:
����ֵ:TRUE or FALSE
*******************************************************/
BOOL writeEvent(INT8U *data,INT8U length,INT8U type, INT8U dataFromType)
{
   INT8U                  tmpErc;
	 sqlite3_stmt           *stmt;
	 const char             *tail;
   char                   *pSqlStr;              //SQL����ַ���ָ��   
	 int                    execResult;
	 INT32U                 recordNo;
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
   DATE_TIME              tmpTime;
   char                   *errMessage;           //������Ϣ(Error msg written her)

   if (flagOfClearData==0x55 || flagOfClearData==0xaa)
   {
      //if (debugInfo&PRINT_DATA_DEBUG)
      //{
        printf("writeEvent:���������,��ֹд������\n");
      //}
     
   	  return FALSE;
   }


 	 recordNo = queryEventStoreNo(type);
     	
   tmpErc = *data;

   recordNo++;
   if (debugInfo&PRINT_EVENT_DEBUG)
   {
     printf("writeEvent:�¼���¼���=%d,����=%d,ERC=%d\n",recordNo,type,tmpErc);
   }
	 
	 pSqlStr = sqlite3_mprintf("insert into eventRecord values (%d,%d,%d,?);", recordNo, type, length);
	 sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
	 sqlite3_bind_blob(stmt, 1, data, length, NULL);
   if (sqlite3_step(stmt)==SQLITE_DONE)
   {
     if (debugInfo&PRINT_EVENT_DEBUG)
     {
       printf("writeEvent:�������ݳɹ�!\n");
     }
   }
   else
   {
     if (debugInfo&PRINT_EVENT_DEBUG)
     {
       printf("writeEvent:��������ʧ��!\n");
     }
   }
   sqlite3_finalize(stmt);

   //�����¼�������ֵ
   if (type == 1)
   {
  	 if (recordNo>255)
  	 {
	     iEventStartPtr++;
	     pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=1 and storeNo<%d",iEventStartPtr);
       if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
       {
   	      if (debugInfo&PRINT_EVENT_DEBUG)
   	      {
   	        printf("writeEvent:ɾ���������Ҫ�¼���¼�ɹ�!\n");
   	      }
       }
       sqlite3_free(pSqlStr);
         	   
  	   iEventCounter = 255;
  	   eventReadedPointer[0] = 255;
  	   if (eventReadedPointer[1]>0)
  	   {
  	     eventReadedPointer[1]--;
  	   }
  	   saveParameter(88, 2,&eventReadedPointer, 2);
  	 }
  	 else
  	 {
  	   iEventCounter = recordNo;
  	 }
  	 
  	 if (debugInfo&PRINT_EVENT_DEBUG)
  	 {
  	   printf("writeEvent:��Ҫ�¼�������ֵ=%d,iEventStartPtr=%d\n",iEventCounter,iEventStartPtr);
  	 }
   }
   else
   {
  	 if (recordNo>255)
  	 {
	     nEventStartPtr++;
	     pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=2 and storeNo<%d",nEventStartPtr);
       if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
       {
   	      if (debugInfo&PRINT_EVENT_DEBUG)
   	      {
   	        printf("writeEvent:ɾ�������һ���¼���¼�ɹ�!\n");
   	      }
       }
       sqlite3_free(pSqlStr);
       
  	   nEventCounter = 255;
  	 }
  	 else
  	 {
  	   nEventCounter = recordNo;  	 	  
  	 }

  	 if (debugInfo&PRINT_EVENT_DEBUG)
  	 {
  	   printf("writeEvent:һ���¼�������ֵ=%d\n", nEventCounter);
  	 }
   }

   //LCD��˸��ʾ���¼�����
   aberrantAlarm.eventNum     = tmpErc;
   aberrantAlarm.timeOut      = nextTime(sysTime, 0, 20);
   aberrantAlarm.aberrantFlag = 1;
   
   return TRUE;
}

/*******************************************************
��������:bringTableName
��������:������ʵʱ���ݱ���
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
char * bringTableName(DATE_TIME time,INT8U type)
{
  char strTableName[3][3]={"sp","mp","hf"};
  char tableName[30];
  char str[10];
    
  if (type>2)
  {
  	type = 0;
  }
  
  strcpy(tableName,strTableName[type]);
  strcat(tableName,digital2ToString(time.year,str));
  strcat(tableName,digital2ToString(time.month,str));
  strcat(tableName,digital2ToString(time.day,str));
  
  return tableName;
}

/*******************************************************
��������:checkSpRealTable
��������:��鵥���ʵʱ���ݱ��Ƿ����,�粻��������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void checkSpRealTable(INT8U type)
{ 
	char    *pSqlStr;                  //SQL����ַ���ָ��
  char    *errMessage;               //������Ϣ(Error msg written her)
  char    tableName[20];             //��ʱ����
  char    str[10];
  
	strcpy(tableName, bringTableName(sysTime,type));
	pSqlStr = sqlite3_mprintf("select * from  %s  limit 1;",tableName);
	
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("���ݱ�%s�Ѿ�����\n",tableName);
  	}
  }
  else
  {
	  switch (type)
	  {
	    case 1:    //�����ʵʱ���ݱ�
	      pSqlStr = sqlite3_mprintf("create table %s(pn int,queryType int,dataType int,time int,data blob);",tableName);
	      break;

	    case 2:    //���㶳�����ݱ�
	      pSqlStr = sqlite3_mprintf("create table %s(pn int,freezeTime int,data blob);",tableName);
	      break;
	     	
	    default:   //�����ʵʱ���ݱ�
	      pSqlStr = sqlite3_mprintf("create table %s(pn int,time int,data blob);",tableName);
	      break;
	  }

    if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("������%s�ɹ�\n",tableName);
     	}
    }
    else
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("������%sʧ��\n",tableName);
     	}
     	 
     	sqlite3_free(errMessage);
    }
  }
}

/*******************************************************
��������:threadOfClearData
��������:���������߳�(�������ݻ���վ�·��������)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void threadOfClearData(void *arg)
{ 
   sqlite3      *sqlite3Dbx;
   DATE_TIME    tmpTime;
   char         tableName[30];
   INT16U       i;
   char         *pSqlStr;              //SQL����ַ���ָ��
   char         *errMessage;           //������Ϣ(Error msg written her)
   INT8U        ifDel;
   INT8U        tmpNumOfZjz;
   INT16U       pn;
   INT8U        eventData[18];

   DIR           *dp;          //2012-10-10,add
   struct dirent *filename;    //2012-10-10,add
   char          tmpFileName[30];

   while(1)
   {
     //1.�����������(��λ����һ��)
     if (flagOfClearData==0x99)
     {
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("����������ݿ�ʼ\n");
       }
     
       sleep(62);    //2014-09-26,��32��ĳ�62��

       flagOfClearData = 0x0;
       
       sqlite3_close(sqlite3Db);
    
       if (sqlite3_open("powerData.db", &sqlite3Dbx))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Dbx));
         sqlite3_close(sqlite3Dbx);
         
         //ly,2011-10-14,add,�򲻿��µ�ȫ��ָ��,��ԭΪԭȫ��ָ��
         usleep(2000);
         if (sqlite3_open("powerData.db", &sqlite3Db))
         {
           printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
           sqlite3_close(sqlite3Db);
       
           usleep(2000);
           if (sqlite3_open("powerData.db", &sqlite3Db))
           {
             printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
             sqlite3_close(sqlite3Db);
           }
         }

         continue;
       }
    
       //1.�����/�����ʵʱ��������
      #ifdef SUPPORT_ETH_COPY
       //2014-09-26,������̫���ڳ����ն����ڳ����ٶȿ�, ���ݿ⼱����240M������,����ϵͳ������
       //    �޸�Ϊֻ��3���ʵʱ�ͽ�������,��ÿ���賿��ʱ����һ������
       tmpTime = backTime(sysTime, 0, 3, 0, 0, 0);
      #else
       //tmpTime = backTime(sysTime, 0, 14, 0, 0, 0);
       //2020-11-19,��14���޸�Ϊ5��
       tmpTime = backTime(sysTime, 0, 5, 0, 0, 0);
      #endif
       for(i=0;i<10;i++)
       {
       	  tmpTime = backTime(tmpTime, 0, 1, 0, 0, 0);
          
          //�����
    	    strcpy(tableName, bringTableName(tmpTime,1));
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%s�ɹ�\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%sʧ��.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
          
          //�ز�/�����
    	    strcpy(tableName, bringTableName(tmpTime,0));
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
    	    
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%s�ɹ�\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%sʧ��.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
          
          //���㶳���
    	    strcpy(tableName, bringTableName(tmpTime,2));
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
    	    
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%s�ɹ�\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%sʧ��.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
          
          //���������ݱ�
    	   #ifdef LIGHTING
    	    sprintf(tableName, "slday%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
    	    
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%s�ɹ�\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("ɾ����%sʧ��.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
         #endif
       }
       sqlite3_close(sqlite3Dbx);
    
       if (sqlite3_open("powerData.db", &sqlite3Db))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
         sqlite3_close(sqlite3Db);
         //ly,2011-10-14,add,�򲻿��µ�ȫ��ָ��,��ԭΪԭȫ��ָ��
         usleep(2000);
         if (sqlite3_open("powerData.db", &sqlite3Db))
         {
           printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
           sqlite3_close(sqlite3Db);
       
           usleep(2000);
           if (sqlite3_open("powerData.db", &sqlite3Db))
           {
             printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
             sqlite3_close(sqlite3Db);
           }
         }
         
         continue;
       }
    
       //2.���������/��һ���������� lastMonthData(�����1��(31��)������)
       clearOutData("lastMonthData", LAST_MONTH_DATA, 31, 1);
    
       //3.������ս������� dayBalanceData (�����31�������)
       clearOutData("dayBalanceData", DAY_BALANCE, 31, 1);
    
       //4.������½������� monthBalanceData (�����12����130�������)
       clearOutData("monthBalanceData", MONTH_BALANCE, 12, 1);
    
       //5.ͳ��ͳ������(�����31�������)
       clearOutData("statisData", 2, 31, 0);
       
       //6.������ն�������singlePhaseDay(�����31�������)
       clearOutData("singlePhaseDay", 0, 31, 0);
          
       //7.������¶�������singlePhaseMonth(�����12���µ�����)
       clearOutData("singlePhaseMonth", 1, 12, 0);
    
       //8.ֱ��ģ��������dcAnalog(�����14�������)
       clearOutData("dcAnalog", 0, 14, 0);

       //2012-10-10,add,ɾ�������ն����ļ�
       dp = opendir("/data/");
       if (!dp)
       {
         fprintf(stderr,"open directory error\n");
       }
       else
       {
         while (filename=readdir(dp))
         {
           if (strstr(filename->d_name,"mpd"))
           {
             tmpTime = backTime(sysTime, 0, 31, 0, 0, 0);
             sprintf(tmpFileName, "mpd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
             
             if (strcmp(filename->d_name, tmpFileName)<0)
             {
               sprintf(tmpFileName,"rm /data/%s",filename->d_name);
               if (system(tmpFileName)==0)
               {
             	   printf("�ļ�%sɾ���ɹ�\n", filename->d_name);
               }
               else
               {
             	   printf("�ļ�%sɾ��ʧ��\n", filename->d_name);
             	 }

             }
             else
             {
             	 printf("%s��Ӧ��ɾ��\n", filename->d_name);             	 
             }
           }

           if (strstr(filename->d_name,"spd"))
           {
             tmpTime = backTime(sysTime, 0, 31, 0, 0, 0);
             sprintf(tmpFileName, "spd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
             
             if (strcmp(filename->d_name, tmpFileName)<0)
             {
               sprintf(tmpFileName,"rm /data/%s",filename->d_name);
               if (system(tmpFileName)==0)
               {
             	   printf("�ļ�%sɾ���ɹ�\n", filename->d_name);
               }
               else
               {
             	   printf("�ļ�%sɾ��ʧ��\n", filename->d_name);
             	 }
             }
             else
             {
             	 printf("%s��Ӧ��ɾ��\n", filename->d_name);             	 
             }
           }
         }
         closedir(dp);
       }
  
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("��������������\n");
       }
     }
   
     //2.�������/����(����)
     if (flagOfClearData==0x55 || flagOfClearData==0xaa)
     {
       if (flagOfClearData==0xaa)
       {
         ifDel = 1;
       }
       else
       {
         ifDel = 0;
       }
     
       if (ifDel==1)
       {
         //ר���ն�,�������
         #ifndef PLUG_IN_CARRIER_MODULE
          //1.������
          if (staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
          {
          	staySupportStatus.ifStaySupport = CTRL_RELEASE;  //�����ѽ��
          }
          
          //2.�޳����
          if (toEliminate==CTRL_JUMP_IN)
          {
          	   toEliminate = CTRL_RELEASE;    //�޳����
          }
          
          //3.�߷Ѹ澯��� 
          if (reminderFee==CTRL_JUMP_IN)
          {
          	reminderFee = CTRL_RELEASE;
          }
    
          //4.ң�ؽ��
          for (i=0; i<CONTROL_OUTPUT; i++)
          {
          	if (remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_JUMP_IN)
          	{
          		 remoteCtrlConfig[i].remoteEnd = sysTime;   //ң�ؽ��
          	}
          }
          
          //5.���غ͵�ؽ��
       	  for (tmpNumOfZjz = 0; tmpNumOfZjz<totalAddGroup.numberOfzjz; tmpNumOfZjz++)
       	  {
       	    pn = totalAddGroup.perZjz[tmpNumOfZjz].zjzNo;
       	    
       	    //4.1 ����µ�����
           	if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_JUMP_IN)
       	    {
       	    	ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_RELEASE;
       	    }
       	    
       	    //4.2 ��������
       	    if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_JUMP_IN)
       	    {
       	    	ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_RELEASE;
       	    }
       	    
       	    //4.3 ����¸���
           	if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_RELEASE;
           	}
           	
       	    //4.4 �����ͣ��
           	if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_RELEASE;
           	}
    
       	    //4.5 ������ݿ�
           	if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_RELEASE;
           	}
    
       	    //4.6 ���ʱ�ο�
           	if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_RELEASE;
           	}
       	  }
          
          sleep(2);
         #endif
         
         #ifdef PRINT_DATA_BASE
          printf("ɾ�������߳�:���ݼ�����\n");
         #endif
         
         //ɾ�����ݼ�����
    	   deleteData(1);

         //׼���¼�����
         if ((eventRecordConfig.iEvent[0] & 0x01) || (eventRecordConfig.nEvent[0] & 0x01))
         {
         	  eventData[0] = 0x1;
         	  eventData[1] = 0x12;
         	  eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         	  eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         	  eventData[4] = sysTime.hour/10<<4 | sysTime.hour%10;
         	  eventData[5] = sysTime.day/10<<4 | sysTime.day%10;
         	  eventData[6] = sysTime.month/10<<4 | sysTime.month%10;
         	  eventData[7] = sysTime.year/10<<4 | sysTime.year%10;
         	  eventData[8] = 0x1;
         	  eventData[9] = 0x0;
         	  eventData[10] = 0x0;
         	  eventData[11] = 0x0;
         	  eventData[12] = 0x0;
         	  eventData[13] = 0x0;
         	  eventData[14] = 0x0;
         	  eventData[15] = 0x0;
         	  eventData[16] = 0x0;
         	  eventData[17] = 0x0;
         }
    	 }
    	 else
    	 {
         #ifdef PRINT_DATA_BASE
          printf("ɾ�������߳�:���������\n");
         #endif
         
         //��������� 
         deleteData(0);
    	 }
       
       //Ӳ����λ����
       #ifdef PLUG_IN_CARRIER_MODULE
        flagOfClearData = 0x0;

        if (ifDel==1)
        {
          if (eventRecordConfig.iEvent[0] & 0x01)
          {
         	   writeEvent(eventData, 18, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
         	}
          if (eventRecordConfig.nEvent[0] & 0x01)
          {
         	  writeEvent(eventData, 18, 2, DATA_FROM_GPRS);  //����һ���¼�����
         	}

         	eventStatus[0] = eventStatus[0] | 0x01;
        }

        cmdReset = 1;               //�ȴ�2���λ
       #else
        printf("ɾ�������߳�:ר���ն˲���λ����װ����\n");
        
        //ר���ն�������󲻸�λ,��Ҫ����������װ�����
        loadParameter();
        fillPulseVar(1);
    
        //ר���ն������ֹ��־��λ
        if (flagOfClearPulse==0x55 || flagOfClearPulse==0xaa)
        {
       	  flagOfClearPulse = 0x0;
        }

        if (ifDel==1)
        {
          if (eventRecordConfig.iEvent[0] & 0x01)
          {
         	   writeEvent(eventData, 18, 1, DATA_FROM_GPRS);  //������Ҫ�¼�����
         	}
          if (eventRecordConfig.nEvent[0] & 0x01)
          {
         	  writeEvent(eventData, 18, 2, DATA_FROM_GPRS);  //����һ���¼�����
         	}
         	  
         	eventStatus[0] = eventStatus[0] | 0x01;
        }
       #endif
       
       flagOfClearData = 0x0;
     }
     
     usleep(500000);
   }
}

/************************************************
��������:logRun
����:
���ú���:
������:
�������:
�������:
����ֵ:
**************************************************/
void logRun(char *str)
{
	FILE *fp,*fpout;
  char timeStr[20];
  
  if((fp=fopen("/runLog","a+"))==NULL)
  {
  	return;
  }	
 	sprintf(timeStr,"%02d-%02d-%02d %02d:%02d:%02d,",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
 	fputs(timeStr,fp);
 	fputs(str,fp);
 	fputs("\n",fp);

  fclose(fp);
}

/************************************************
��������:readIpMaskGateway
����:��ȡrcS�ļ��е�IP��ַ�����뼰Ĭ�ϵ�ַ
���ú���:
������:
�������:
�������:
����ֵ:
**************************************************/
void readIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4])
{
  FILE *fpOfRcs;
  int  i;
  unsigned char ipPtr, j;
  char say[100];  

  if ((fpOfRcs=fopen("/etc/init.d/rcS","r+"))==NULL)
  {
  	printf("Can't open rcS\n");  	
  }
  
  //����IP��ַ������
  i=0;
  while(i<50)
  {
    i++;
    fgets(say, 100, fpOfRcs);
    //printf("%s\n", say);
    
    if (strstr(say,"netmask"))
    {
    	//IP��ַ
      ipPtr = 14;
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //ֻ��һλ��
      	  {
      	     ip[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //ֻ�ж�λ��
        	   {
      	        ip[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        ip[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //ֻ��һλ��
      	  {
      	     ip[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //ֻ�ж�λ��
        	   {
      	        ip[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        ip[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    	
    	ipPtr+=8;
    	//IP��ַ����
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //ֻ��һλ��
      	  {
      	     mask[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //ֻ�ж�λ��
        	   {
      	        mask[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        mask[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //ֻ��һλ��
      	  {
      	     mask[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //ֻ�ж�λ��
        	   {
      	        mask[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        mask[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    }
    
    if (strstr(say,"route add default gw"))
    {
    	//IP��ַ
      ipPtr = 21;
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //ֻ��һλ��
      	  {
      	     gw[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //ֻ�ж�λ��
        	   {
      	        gw[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        gw[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //ֻ��һλ��
      	  {
      	     gw[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //ֻ�ж�λ��
        	   {
      	        gw[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        gw[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    	
    	break;
    }
  }
  fclose(fpOfRcs);
}

/************************************************
��������:saveIpMaskGateway
����:������̫����IP��ַ�����뼰Ĭ�ϵ�ַ��rcS��
���ú���:
������:
�������:
�������:
����ֵ:
**************************************************/
void saveIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4])
{
  FILE *fpOfRcs;
  int  i, j;
  char say[100];  

  if ((fpOfRcs=fopen("/etc/init.d/rcS","r+"))==NULL)
  {
  	printf("Can't open rcS\n");  	
  }
  else
  {
    i=0;
    while(i<50)
    {
      i++;
      fgets(say, 100, fpOfRcs);
      
      if (strstr(say,"netmask"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3],mask[0],mask[1],mask[2],mask[3]);
        
        for(j=strlen(say); j<52; j++)
        {
        	 strcat(say," ");
        }
        
        fputs(say, fpOfRcs);
  
      	printf("eth0 ip and mask saved!\n");
      }
      
      if (strstr(say,"route add default gw"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"route add default gw %d.%d.%d.%d dev eth0", gw[0], gw[1], gw[2], gw[3]);
        
        for(j=strlen(say); j<43; j++)
        {
        	 strcat(say," ");
        }
        
        fputs(say, fpOfRcs);
  
      	printf("default gateway saved!\n");
      	
      	break;
      }
    }
    fclose(fpOfRcs);
  }
}

/*******************************************************
��������:readAcVision
��������:��ȡ����ʾֵ
���ú���:
�����ú���:
�������:*tmpData-���ݻ���ָ��
�������:
����ֵ:TRUE(��ѯ�ɹ�) or FALSE(��ѯʧ��)
*******************************************************/
BOOL readAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType)
{
	sqlite3_stmt *stat;
	char   *pSqlStr;
	INT16U result;
  struct timeval  tv;       //Linux timeval
	
	//��ѯ����
  time.day    = 1;
  time.hour   = 0;
  time.minute = 0;
  time.second = 0;
  getLinuxFormatDateTime(&time, &tv, 1);
	
	pSqlStr = sqlite3_mprintf("select * from acVision where dataType=%d and time=%d", dataType, tv.tv_sec);	
  sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stat, 0);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		if (dataType==ENERGY_DATA)
		{
		  memcpy(tmpData, sqlite3_column_blob(stat, 2), SIZE_OF_ENERGY_VISION);
		}
		
		if (dataType==REQ_REQTIME_DATA)
		{
		  memcpy(tmpData, sqlite3_column_blob(stat, 2), LENGTH_OF_REQ_RECORD);
		}
		
		sqlite3_finalize(stat);

	  sqlite3_free(pSqlStr);

		return TRUE;
	}

	sqlite3_finalize(stat);
	sqlite3_free(pSqlStr);
  
  if (dataType==ENERGY_DATA)
  {
    memset(tmpData, 0x0, SIZE_OF_ENERGY_VISION);
  }

  if (dataType==REQ_REQTIME_DATA)
  {
    memset(tmpData, 0x0, LENGTH_OF_REQ_RECORD);
  }

	return FALSE;
}

/*******************************************************
��������:updateAcVision
��������:���½���ʾֵ
���ú���:
�����ú���:
�������:*tmpData-���ݻ���ָ��
�������:
����ֵ:TRUE(���³ɹ�) or FALSE(����ʧ��)
*******************************************************/
BOOL updateAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType)
{
	sqlite3_stmt *stat;
	char   *pSqlStr;
	INT16U result;
  struct timeval  tv;                  //Linux timeval

  time.day    = 1;
  time.hour   = 0;
  time.minute = 0;
  time.second = 0;
  getLinuxFormatDateTime(&time,&tv,1);

	//��ѯ����
	pSqlStr = sqlite3_mprintf("select * from acVision where dataType=%d and time=%d", dataType, tv.tv_sec);	
  sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stat, 0);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	sqlite3_free(pSqlStr);

	if(result == SQLITE_ROW)
	{
   	pSqlStr = sqlite3_mprintf("update acVision set data=? where dataType=%d and time=%d;",dataType,tv.tv_sec);
		sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stat, 0);
		if (dataType==ENERGY_DATA)
		{
		  sqlite3_bind_blob(stat, 1, tmpData, SIZE_OF_ENERGY_VISION, NULL);
		}
		else
		{
		  sqlite3_bind_blob(stat, 1, tmpData, LENGTH_OF_REQ_RECORD, NULL);
		}
		sqlite3_step(stat);
		sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_AC_VISION)
		{
		  printf("���½���ʾֵ\n");
		}
	}
	else
	{
   	pSqlStr = sqlite3_mprintf("insert into acVision values(%d, %d, ?);",dataType,tv.tv_sec);
		sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stat, 0);
		if (dataType==ENERGY_DATA)
		{
		  sqlite3_bind_blob(stat, 1, tmpData, SIZE_OF_ENERGY_VISION, NULL);
		}
		else
		{
		  sqlite3_bind_blob(stat, 1, tmpData, LENGTH_OF_REQ_RECORD, NULL);
		}
		sqlite3_step(stat);
		sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_AC_VISION)
		{
		  printf("���뽻��ʾֵ\n");
		}
	}
	
	sqlite3_free(pSqlStr);

	return TRUE;
}

/*******************************************************
��������:deleteAcVision
��������:�������ʾֵ
���ú���:
�����ú���:
�������:void
�������:
����ֵ:TRUE(��ѯ�ɹ�) or FALSE(��ѯʧ��)
*******************************************************/
BOOL deleteAcVision(void)
{
 #ifdef AC_SAMPLE_DEVICE	
	
	sqlite3_stmt *stat;
	char   *sql;
	INT16U result;

	//��ѯ����
	sql = "delete from acVision";
	result = sqlite3_exec(sqlite3Db, sql, 0, 0, NULL);
	if(result == SQLITE_OK)
	{
    //������������
    readAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);

		return TRUE;
	}
  
  //������������
  readAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);
  
 #endif

	return FALSE;
}


/*******************************************************
��������:deletePresentData
��������:ɾ��������pn�ĵ�ǰ����         
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void deletePresentData(INT16U pn)
{
	 INT16U       result;
   char         tableName[30];
   char         pSqlStr[512];          //SQL����ַ���ָ��

   strcpy(tableName, bringTableName(sysTime,1));

   sprintf(pSqlStr, "delete from %s where pn=%d;", tableName, pn);
   result = sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL);
   if (result==SQLITE_OK)
   {
 	   if ((debugInfo&PRINT_PULSE_DEBUG) || (debugInfo&PRINT_DATA_BASE))
 	   {
 	      printf("ɾ����%s�в�����%dʵʱ���ݳɹ�\n", tableName, pn);
 	   }
   }
   else
   {
 	   if ((debugInfo&PRINT_PULSE_DEBUG) || (debugInfo&PRINT_DATA_BASE))
 	   {
 	      printf("ɾ����%s�в�����%dʵʱ����ʧ��\n", tableName, pn);
 	   }
   }
}


#ifdef LIGHTING

/*******************************************************
��������:readSlDayData
��������:��ȡ·��������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
BOOL readSlDayData(INT16U pn, INT8U type, DATE_TIME readTime, INT8U *dataBuf)
{ 
	sqlite3_stmt *stmt;
  char         tableName[20];     //��ʱ����
	char         *pSqlStr;          //SQL����ַ���ָ��
  INT16U       execResult;
	const char   *tail;
  
  //0.sl�����ݱ���,ÿ��һ����
  sprintf(tableName, "slday%02x%02x%02x", readTime.year, readTime.month, readTime.day);
  
	pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and type=%d", tableName, pn, type);
	
  if ((execResult=sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail))!= SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
      
      //�����쳣      
      if (execResult!=101)
      {
        printf("readSlDayData(1)->");
      }
    }
    
    logRun("readSlDayData happen aberrant.");
    sqlite3Aberrant(execResult);
    
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("readSlDayData(%02d-%02d-%02d %02d:%02d:%02d):δ�ҵ�����:pn=%d,����Ϊ:%02d\n\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pn, type);
    }
     
    memset(dataBuf, 0xee, 6);
    
    return FALSE;
  }
  else
  {
    execResult = sqlite3_step(stmt);
    if(execResult==SQLITE_ROW)
    {
      memcpy(dataBuf, sqlite3_column_blob(stmt, 2), 6);
      
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readSlDayData(%02d-%02d-%02d %02d:%02d:%02d):�ҵ�����:pn=%d,����Ϊ:%02d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,pn,type);
      }
    }
    else
    {
      //�����쳣
      if (execResult!=SQLITE_DONE)
      {
        logRun("readSlDayData(2) happen aberrant.");
          
        if (debugInfo&PRINT_DATA_BASE)
        {
          printf("readSlDayData(2) happen aberrant.\n");
        }
      }
      
      sqlite3Aberrant(execResult);
      sqlite3_finalize(stmt);
      
      memset(dataBuf, 0xee, 6);
      
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readSlDayData(%02d-%02d-%02d %02d:%02d:%02d):������:pn=%d,����Ϊ:%02d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pn, type);
      }

      return FALSE;
    }
    
	  sqlite3_finalize(stmt);
	}

  sqlite3_free(pSqlStr);

  return TRUE;
}

/*******************************************************
��������:saveSlDayData
��������:�洢·��������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
BOOL saveSlDayData(INT16U pn, INT8U type, DATE_TIME saveTime, INT8U *dataBuf, INT8U len)
{ 
	sqlite3_stmt *stmt;
	char         *pSqlStr;          //SQL����ַ���ָ��
  char         *errMessage;       //������Ϣ(Error msg written her)
  INT16U       execResult;
  char         tableName[20];     //��ʱ����
  INT8U        tmpReadBuf[20];
  INT8U        i;
  
  //0.sl�����ݱ���,ÿ��һ����
  sprintf(tableName, "slday%02x%02x%02x", saveTime.year, saveTime.month, saveTime.day);
  
	//1.��ѯ�Ƿ�������ݱ�,�粻���ڽ���
	pSqlStr = sqlite3_mprintf("select * from  %s  limit 1;",tableName);
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //��ѯ�ɹ�
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("���ݱ�%s�Ѿ�����\n",tableName);
  	}
  }
  else
  {
    sqlite3_free(pSqlStr);
	  
	  pSqlStr = sqlite3_mprintf("create table %s(pn int, type int, data blob);",tableName);
    if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("������%s�ɹ�\n",tableName);
     	}
    }
    else
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("������%sʧ��\n",tableName);
     	}
     	 
     	sqlite3_free(errMessage);
      sqlite3_free(pSqlStr);
      
      return FALSE;
    }
  }
  
  sqlite3_free(pSqlStr);
  
  
  //��ѯ�Ƿ������Ҫ�洢������,���û��,�����,�������Ƚ������Ƿ�һ��,�������,�����
  if (readSlDayData(pn, type, saveTime, tmpReadBuf)==FALSE)
  {
  	pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName, pn, type);
    
    if (debugInfo&PRINT_DATA_BASE)
    {
     	printf("saveSlDayData:��������");
    }
  }
  else
  {
  	for(i=0; i<len; i++)
    {
    	if (tmpReadBuf[i]!=dataBuf[i])
    	{
    		break;
    	}
    }
    
    if (i>=len)
    {
      if (debugInfo&PRINT_DATA_BASE)
      {
   	    printf("saveSlDayData:�����Ѵ���!pn=%02d,type=%02d\n", pn, type);
      }

    	return FALSE;
    }
    
   	pSqlStr = sqlite3_mprintf("update %s set data=? where pn=%d and type=%d;", tableName, pn, type);

    if (debugInfo&PRINT_DATA_BASE)
    {
     	printf("saveSlDayData:��������");
    }
  }
  
  sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
  sqlite3_bind_blob(stmt, 1, dataBuf, len, NULL);

  if ((execResult=sqlite3_step(stmt))==SQLITE_DONE)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
   	  printf("�ɹ�!pn=%02d,type=%02d\n", pn, type);
    }
   	
    //�������ݿ��쳣��������
    dbMonitor = 0;
  }
  else
  {
   	//�����쳣
    printf("saveSlDayData happen aberrant.\n");
    logRun("saveSlDayData happen aberrant.");
    sqlite3Aberrant(execResult);
         	
   	if (debugInfo&PRINT_DATA_BASE)
   	{
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
   	  
   	  printf("ʧ��!pn=%02d,type=%02d\n", pn, type);
   	}
  }
  
  sqlite3_finalize(stmt);
}

#endif

