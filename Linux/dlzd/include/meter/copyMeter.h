/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����copyMeter.h
���ߣ�leiyong
�汾��0.9
�������:2006��7��
�������������ɹ���ϵͳ�ֳ��ն˳�������ͷ�ļ�
      ����������Ľṹ������������
�޸���ʷ��
  01,06-7-29,leiyong created.
**************************************************/

#ifndef __copyMeterH
#define __copyMeterH

#include "common.h"
#include "timeUser.h"
#include "workWithMeter.h"

extern INT8U  ifSystemStartup;                 //ϵͳ������?

//�ṹ - �����ַ����
struct cpAddrLink
{
	INT16U            mpNo;                       //���������
	INT16U            mp;                         //�������
	INT8U             protocol;                   //Э��
	INT8U             addr[6];                    //ͨ�ŵ�ַ
	INT8U             collectorAddr[6];           //�ɼ�����ַ
	INT8U             port;                       //�˿�
	INT8U             bigAndLittleType;           //�û�����ż��û�С���
    
  BOOL              copySuccess;                //�����Ƿ�ɹ�
  BOOL              thisRoundCopyed;            //�ز������Ƿ񳭵�
   
 
 #ifndef LIGHTING 
  
  INT8U             flgOfAutoCopy;              //·�������Զ�������־(�ز�ģ����Ч)
                                                // 0 - ��������δ����
                                                // 1 - �����ն�������...
                                                // 2 - �����ն����������
                                                // 3 - ����Сʱ��������...
                                                // 4 - ����Сʱ�����������
                                                // 5 - ����ʵʱ����...
                                                // 6 - ����ʵʱ�������
                                                // 7 - ����ʧ��(�����굫����Ч������Ϊ����ʧ��)
  
 #else    //��������������
 
  INT32U            flgOfAutoCopy;
  DATE_TIME         statusTime;                 //�ɼ�״̬ʱ��
  INT8U             status;                     //������/��·״̬
  INT8U             statusUpdated;              //״̬�Ѹ���(���º��״̬�����͵�������),2015-10-26,add
  INT8U             lineOn;                     //��·���ϵ�,2015-11-09,add
  INT8U             msCtrlCmd;                  //��վֱ�ӿ�������
  DATE_TIME         msCtrlTime;                 //��վֱ�ӿ��ƽ�ֹʱ��
   
  INT8U             ctrlTime;                   //���ƿ��Ƶ����·���Ƶ�Ŀ���ʱ�κ�/��������������·�������ֽ�

  INT8U             duty[6];                    //���ƿ����� - 6��ɵ���������(/�ɵ���Դ)ռ�ձ�(��Ӧ������ֵ�òɼ�����ַ����)
                                                //���������� - ��0��3�ֽڴ洢��ǰ�����ĵ���ֵ,��4,5�ֽڴ洢��ǰ������ѹֵ
                                                //�նȴ����� - ��0��2�ֽڴ洢��ǰ�������ն�ֵ,��3��5�ֽڴ洢��һ�ε��ն�ֵ
                                                //��·���Ƶ� - ��0��1�ֽ��ճ�ʱ��,��2��3�ֽ�����ʱ��,
                                                //             ��4�ֽڱ�ʾ��վ���õĺ�բ΢��ֵ,��5�ֽڱ�ʾ��վ���õķ�բ΢��ֵ

  INT8U             funOfMeasure;               //�Ƿ��м�������
                                                //  ���ƿ����� - �Ƿ��м�������
                                                //  ���ƿ����� - �Ƿ��м�������
                                                //  �������Ƶ� - ĩ�˵��ƿ�����������ʱ��,2015-5-25

  INT8U             joinUp;                     //�Ƿ����
                                                //  ���ƿ����� - �Ƿ����ƾ�
                                                //  ��·���Ƶ� - �Ƿ���뷴��ң��
                                                //  �������Ƶ� - �ϵ���ز��Ľ���ʱ��

  INT8U             startCurrent;               //��������
                                                //  ���ƿ���������������,�����жϵ��ƹ���
                                                //  ��������������·�������ֽ�,�����ж���·����
  
  INT8U             lddt1st[6];                 //�������Ƶ��õ�һֻĩ�˵��ƿ�����
  INT8U             lddtRetry;                  //����LDDT���Դ���
  DATE_TIME         lddtStatusTime;             //�ɼ���LDDT״̬ʱ��������ʱ��,2016-02-03,Add

  INT8U             offLineRetry;               //�������ߵ��ƿ��������Դ���,2015-11-11,Add
  
  INT8U             lcOnOff;                    //��ص�Դ���ز�����־,2016-01-20,Add
  INT8U             lcDetectOnOff;              //���ݹ�ؼ��ĵ�Դ���ر�־,2016-01-21,Add
	DATE_TIME         lcProtectTime;              //��غ�բ����բ����ʱ��,2017-01-09,Add
  
  INT8U             gateOn;                     //��·���Ƶ�ӷ�բ��Ϊ��բ,2016-01-20,Add

  INT8U             softVer;                    //����汾��,2016-11-04,Add
  
 #endif
 
 #ifdef SUPPORT_ETH_COPY
 
  BOOL              copying;                    //������...
  DATE_TIME         nextCopyTime;               //��̫�������´γ���ʱ��
  
  INT8U             copyedData;                 //����������?
  INT8U             dataBuff[1536];             //���ݻ���

  INT8U             recvBuf[384];               //��������
  INT16U            lenOfRecv;                  //�������ݳ���
  
  INT8U             copyItem;                   //��ǰ��������
  INT8U             totalCopyItem;              //�ܵ�Ӧ������

  BOOL              copyContinue;               //��������
  BOOL              flagOfRetry;                //����
  INT8U             retry;                      //���·��𳭶�ĳ�����ݳ���
  DATE_TIME         copyTimeOut;                //����ʱʱ��

 #endif

	struct cpAddrLink *next;                      //��һ�ڵ�ָ��
};

//�ṹ - ����ת��
typedef struct
{
  INT8U     fn;                                 //����ת����Fn
  INT8U     ctrlWord;                           //ת��ͨ�ſ�����
  INT8U     frameTimeOut;                       //ת�����յȴ����ĳ�ʱʱ��s/ms
  INT8U     byteTimeOut;                        //ת�����յȴ��ֽڳ�ʱʱ��
  INT16U    length;                             //ת������
  INT16U    recvFrameTail;                      //�����ֽ�֡β
  INT8U     data[1024];                         //ת������ָ��
  
  DATE_TIME outTime;                            //��ʱ����ʱ��
  DATE_TIME nextBytesTime;                      //�յ��ֽں�ĵȴ�ʱ��
  BOOL      receivedBytes;                      //���յ��ظ�����
  INT8U     dataFrom;                           //�������Ժη�(1-��վ����㳭/ת��,2-������������㳭,3-����ȫ�������㳭,4-�������������ܱ�)
  BOOL      ifSend;                             //�㳭(/ת��)�����Ƿ���
	INT8U     forwardResult;                      //ת�����
}FORWARD_DATA;

//�ṹ - �����������
struct copyCtrlWord
{
   DATE_TIME         currentCopyTime;           //��ǰ����ʱ��
   DATE_TIME         lastCopyTime;              //�ϴγ���ʱ��
   DATE_TIME         copyTimeOut;               //����ʱʱ��
   DATE_TIME         nextCopyTime;              //��һ�γ���ʱ��
   BOOL              meterCopying;              //������...
   BOOL              copyContinue;              //��������
   BOOL              flagOfRetry;               //����
   INT8U             copyDataType;              //������������(��ǰ����/��������)
   INT8U             retry;                     //���·��𳭶�ĳ�����ݳ���
   INT8U             numOfMeterAbort;           //���485�ӿ�/�ز��ӿڳ���ʧ������?
   INT8U             ifRealBalance;             //�Ƿ�����ս���
   INT8U             ifBackupDayBalance;        //�Ƿ���б����ս���
   INT8U             ifCopyLastFreeze;          //�Ƿ�ɼ���һ�ζ�������
                                                //1.485��/�๦�ܵ��ܱ�(���ز��๦�ܱ�) 0-��ǰ���� 1-�������� 2-��һ���ն�������
                                                //2.�ز��� 0-��ǰ���� 2-��һ���ն������� 3-���㶳������
   INT16U            backMp;                    //�ز��˿���ͣ����Ĳ������ ly,2010-11-4,����
   INT8U             dataBuff[1536];            //���ݻ���
   FORWARD_DATA      *pForwardData;             //����ת��ָ��
   INT8U             backupCtrlWord;            //���ݶ˿ڿ�����,��ֹ�ڶ˿�ת���ı��˶˿����Ժ���ͨ��

   INT8U             ifCopyDayFreeze;           //�Ƿ��ѽ����˳����ն���
   DATE_TIME         copyDayTimeReset;          //�����ն����־��λʱ��

   INT8U             ifCheckTime;               //�Ƿ��ѽ����˵��Уʱ,2013-11-22,add
   DATE_TIME         checkTimeReset;            //���Уʱ��־��λʱ��,2013-11-22,add
   
   BOOL              cmdPause;                  //����ָ�����˿���ͣ����
   INT8U             round;                     //�ز�����ڼ���
   INT8U             protocol;                  //��ǰ���ı��Э��
   INT8U             ifCollector;               //�Ƿ��вɼ�����ַ
   INT8U             hasSuccessItem;            //�Ƿ��вɼ��ɹ���
   BOOL              thisMinuteProcess;         //�����Ӵ����ж�?
	
	 INT8U             needCopyMeter[6];          //·�����󳭶��ı�(/�ɼ���)��ַ,ly,2012-01-09���ز���־���ĵ�copyCtrl��
   INT8U             destAddr[6];               //�ز�Ŀ�Ľڵ��ַ,ly,2012-01-10
	
	#ifdef LIGHTING
	 BOOL              thisRoundFailure;          //���ֳ���485���Ƿ��쳣,2016-11-25
	 INT8U             portFailureRounds;         //485�˿��쳣����,2016-11-25
	#endif

   struct cpAddrLink *cpLinkHead;               //�����ַ����ͷ
   struct cpAddrLink *tmpCpLink;                //�����ַ����ʱָ�����
};

//�ز���־��
typedef struct
{
	unsigned int currentWork   :3;                //�ز�ģ�鵱ǰ����(����/ѧϰ·��/�����ϱ���ŵ�)
	unsigned int sending       :1;                //���ڷ�������
	unsigned int cmdContinue   :1;                //���Լ�����������
	unsigned int retryCmd      :1;                //������һ����

	unsigned int hardwareReest :2;                //Ӳ����λ����
	unsigned int mainNode      :1;                //��ѯ���ڵ��ַ?
	unsigned int setMainNode   :2;                //�������ڵ��ַ
	unsigned int startStopWork :2;                //����ʱ��ֹͣ����
	unsigned int querySlaveNum :1;                //��ѯ�ӽڵ�����
	unsigned int synSlaveNode  :2;                //�Ƿ��б���Ҫͬ�����ز�ģ��?
	unsigned int querySlaveNode:2;                //��ѯ�ز��ӽڵ���Ϣ
	unsigned int routeRunStatus:1;                //��ѯ·������״̬
	unsigned int studyRouting  :3;                //ѧϰ·��״̬
	unsigned int setupNetwork  :3;                //����״̬
	unsigned int searchMeter   :3;                //�ѱ�
	unsigned int init          :2;                //��ʼ��
	unsigned int hasAddedMeter :1;                //����ӱ��ַ
	unsigned int forceClearData:1;                //ǿ���������
	unsigned int ifSearched    :1;                //�Ƿ��ѹ���?
	unsigned int msSetAddr     :1;                //��վ���ñ���(ly,2011-01-29,add)
	unsigned int checkAddrClear:1;                //�Ƚϱ������������(ly,2011-01-30,add)
		
	unsigned int reStartPause  :2;                //��������Ҫ���ͳ���������ͣ�ָ�����(ly,2012-01-16,add)
		                                            // 1 - ����
		                                            // 2 - ��ͣ
		                                            // 3 - �ָ�
	
	unsigned int workStatus    :4;                //����ͨ��ģ�鹤��״̬ ly,2011-07-06,add
		                                            //  0 - ����Ϊ����ģʽ
		                                            //  1 - ģ��ȷ�ϳ���ģʽ
		                                            //  2 - Ȼ����������
		                                            //  3 - ģ��ȷ����������
		                                            //  4 - ��ͣģ�鳭��
		                                            //  5 - ģ��ȷ����ͣ����
		                                            //  6 - ģ��ȷ�ϻָ�����
		                                            //  7 - ������ʱ�ζ���ͣ����
		                                            //  8 - �������Ƶ��ĩ�˵��ƿ��������ͨ��ʱ�䳬�����ʱ�޶���ͣ����
		                                            //  9 - ���ƿ���������������ֵʱ�޶���ͣ����
		                                            // 10 - ���ƿ��������ϵ�ʱ�ٲ�״̬����ͣ����
		                                            
  unsigned int routeLeadCopy :1;                //·����������(1-��·������������, 0-��������������)
 
  unsigned int chkNodeBeforeCopy :2;            //����Ҫ�󳭱�ǰ�ȶԽڵ㵵��,2013-12-30,add
   	                                            //  ���ڴ�,������·������������ģ�鶼���˴���
  
  
  /***************��������չ��Ҫ������********************/
	unsigned int innerDebug    :1;                //�ڲ�����(SR)?
	unsigned int wlNetOk       :3;                //������չ - ��ѯ�Ƿ��������
	unsigned int paraClear     :2;                //SR��չ - �������
	INT16U       lastNumInNet;                    //SR��չ - ��һ�β�ѯ�����ڵ�����
	unsigned int setDateTime   :1;                //��Ѹ����չ - ����ʱ��
  unsigned int readStatus    :2;                //��Ѹ����չ - ��ȡ״̬

  unsigned int batchCopy     :3;                //���Ժ - �ֳ�״̬ ly,2012-02-29,Ϊ��Ӧ�°���Ժģ������
  	                                            // 0 - δ��ʼ�ִ�
  	                                            // 1 - �����ֳ�
  	                                            // 2 - ģ��ȷ�������ֳ�����

  unsigned int setPanId      :1;                //�����ŵ��ŵ���
  	

  /***************��չ����         END********************/

  INT8U        mainNodeAddr[6];                 //�ز����ڵ��ַ
  INT8U        destAddr[6];                     //�ز�Ŀ�Ľڵ��ַ
  INT16U       synMeterNo;                      //ͬ���ز��ӽڵ����
  INT16U       activeMeterNo;                   //����ע����ز��ӽڵ��ѯ���
  INT16U       numOfSalveNode;                  //�ӽڵ�����
	
	DATE_TIME    cmdTimeOut;                      //����ͳ�ʱ
  DATE_TIME    operateWaitTime;                 //�����ȴ�ʱ��
  DATE_TIME    foundStudyTime;                  //����/ѧϰʱ��
  
  DATE_TIME    msSetWait;                       //��վ���ñ���ͬ��������ͨ��ģ��ȴ�ʱ��(ly,2011-01-29,add)
  
	DATE_TIME    delayTime;                       //RLģ������ȴ�
	DATE_TIME    netOkTimeOut;                    //RLģ��������ʱʱ��
	
	INT16U       numOf253;                        //����״̬�ֳ���253�Ĵ��� ly,2011-05-20,Ϊ��Ӧ����΢
	INT16U       numOfCarrierSending;             //�ز��˿ڷ��ͼ���        ly,2011-05-27,Ϊ��Ӧ����΢
	
	INT8U        cmdType;                         //������������
	INT8U        productInfo[8];                  //���̴���Ͱ汾��Ϣ
	
	DATE_TIME    routeRequestOutTime;             //·������ʱʱ��
	
 #ifdef LIGHTING
   
  INT8U        broadCast;                       //�㲥
	DATE_TIME    broadCastWaitTime;               //�㲥����ȴ�ʱ��
	
	INT8U        searchLddt;                      //�����������Ƶ��ĩ�˵��ƿ�����
	DATE_TIME    searchLddtTime;                  //�����������Ƶ��ĩ�˵��ƿ��������ʱ��
  struct cpAddrLink *searchLdgmNode;            //�����������Ƶ�ڵ�
  
	INT8U        searchOffLine;                   //�������ߵĵ��ƿ�����,2015-11-09,Add
	DATE_TIME    searchOffLineTime;               //�������ߵĵ��ƿ��������ʱ��,2015-11-09,Add

	INT8U        searchLddStatus;                 //�������ƿ�����״̬,2015-11-12,Add
	DATE_TIME    searchLddStatusTime;             //�������ƿ�����״̬���ʱ��,2015-11-12,Add

 #endif

}CARRIER_FLAG_SET;

//�ṹ - �㳭
typedef struct
{
  INT8U     port;                               //ת��ͨ�Ŷ˿ں�
  INT8U     dataFrom;                           //�������Ժη�(1-��վ����㳭/ת��,2-������������㳭,3-����ȫ�������㳭,4-�������������ܱ�)

  INT8U     data[255];                          //ת������ָ��
  
  INT8U     dotRetry;                           //�㳭���Դ���
  INT8U     dotTotalItem;                       //�㳭Ӧ������
  INT8U     dotRecvItem;                        //�㳭��������
  
	INT16U    dotCopyMp;                          //�㳭������
  INT8U     addr[6];                            //�㳭��ַ
  INT8U     protocol;                           //������Э��
  DATE_TIME outTime;                            //��ʱ����ʱ��
	BOOL      dotCopying;                         //���ڵ㳭
	INT8U     dotResult;                          //�㳭���
}DOT_COPY;

//�㳭F129�ṹ
struct dotFn129
{
	INT16U pn;                                    //������
	INT8U  from;                                  //������Դ
	INT8U  ifProcessing;                          //�Ƿ����ڴ���
	
	struct dotFn129 *next;
};

#define METER_RECV_BUF_SIZE        212          //���ܱ����ݽ��ջ����С
#define REPLENISH_MAX_RETRY          2          //��������ط�����

//����˿�
#ifdef PLUG_IN_CARRIER_MODULE
 #define NUM_OF_COPY_METER           5          //����˿�����(��������5���˿ڳ���)
#else
 #define NUM_OF_COPY_METER           3          //����˿�����(ר��III���ն�ֻ��3���˿ڳ���)
#endif
#define PORT_NO_1                    1          //485�˿ں�1
#define PORT_NO_2                    2          //485�˿ں�2
#define PORT_NO_3                    3          //485�˿ں�3
#define PORT_NO_4                    4          //485�˿ں�4
#define PORT_POWER_CARRIER          31          //�����ز��ӿ�

//�㳭����
#define DOT_CP_SINGLE_MP           0x5          //������������㳭
#define DOT_CP_ALL_MP              0x6          //����ȫ�������㳭
#define DOT_CP_NEW_METER           0x7          //�����㳭���ֵ��±�
#define DOT_CP_IR                  0x8          //����ѧϰIR

//�㳭/ת�����
#define RESULT_NONE                0x0          //��û�е㳭���
#define RESULT_HAS_DATA            0x1          //�㳭/ת�������ݻظ�
#define RESULT_NO_REPLY            0x2          //�㳭/ת�������Ӧ��

#ifdef JZQ_CTRL_BOARD_V_1_4 
 #define NUM_COPY_RETRY              3          //������Ӧ���ط�����(��xMegaת��485����,2·ͬʱ����ʱҪ��
                                                //  CPU����Ĵ���2·ͬʱ������ʱҪ��,��˼�С�ط�����)
#else
 #define NUM_COPY_RETRY              3          //������Ӧ���ط�����
#endif

#define CARRIER_TIME_OUT            10          //�ز��ڵ㷢�����ʱʱ��(10s)
                                                //2010-12-28,��������΢���鷢������ĳ�ʱʱ��Ϊ10s,��˴�5s�ĳ�10s

#define ROUTE_REQUEST_OUT           10          //·������ʱʱ��(��λ:��,���ڹ�һ��ʱ��������������)
                                                //2012-08-28,�ĳ���10���Ӹĳ�20����
                                                //2014-01-02,�ĳ���20���Ӹĳ�10����,����Ҫ���Ϊ10����

#define MONITOR_WAIT_ROUTE         100          //����ز��ӽڵ���ȴ�ʱ��(s),2013-12-27
                                                //����·��Ҫ��ȴ�����90��,Ϊ�ɿ����,����Ϊ100��
                                                //��2013-12-27,������Ҫ��ȴ�ʱ�����·��

#define WAIT_RECOVERY_COPY          60          //�ָ�����ȴ�ʱ��(s),2013-12-27

//�ز�/���߶˿ڷ�����������
#define CA_CMD_NONE                0x0          //������
#define CA_CMD_HARD_RESET          0x1          //Ӳ����λ����
#define CA_CMD_READ_MODULE         0x2          //ʶ��ģ������
#define CA_CMD_SR_PARA_INIT        0x3          //SRģ���������ʼ������
#define CA_CMD_CLEAR_PARA          0x4          //��������ʼ������
#define CA_CMD_QUERY_MAIN_NODE     0x5          //��ѯ���ڵ��ַ����
#define CA_CMD_SET_MAIN_NODE       0x6          //�������ڵ��ַ����
#define CA_CMD_SR_CHECK_NET        0x7          //SRģ������������֤����
#define CA_CMD_RESTART_WORK        0x8          //��������
#define CA_CMD_PAUSE_WORK          0x9          //��ͣ��ǰ��������
#define CA_CMD_RESTORE_WORK        0xa          //�ָ���ͣ�Ĺ�������
#define CA_CMD_QUERY_ROUTE_STATUS  0xb          //��ѯ·��״̬����
#define CA_CMD_QUERY_SLAVE_NUM     0xc          //��ѯ�ӽڵ���������
#define CA_CMD_QUERY_SLAVE_NODE    0xd          //��ѯ�ӽڵ�����
#define CA_CMD_ADD_SLAVE_NODE      0xe          //��Ӵӽڵ�����
#define CA_CMD_READ_NO_NET_NODE    0xf          //��δ�����ڵ�����
#define CA_CMD_SET_WORK_MODE      0x10          //���ù���ģʽ����
#define CA_CMD_ACTIVE_REGISTER    0x11          //�����ز��ӽڵ�����ע������
#define CA_CMD_PERMIT_ACTIVE      0x12          //�����ز��ӽڵ������ϱ�����
#define CA_CMD_SET_MODULE_TIME    0x13          //����ģ��ʱ������
#define CA_CMD_UPDATE_ADDR        0x14          //��Ѹ������������������
#define CA_CMD_BATCH_COPY         0x15          //���Ժģ���ֳ�����
#define CA_CMD_BROAD_CAST         0x16          //�㲥����

#define SRWF_NET_OK_TIME_OUT        30          //SRWF_3E68�ȴ�����Okʱ��Ϊ30����

//�ⲿ����
#ifdef PLUG_IN_CARRIER_MODULE
 extern struct copyCtrlWord copyCtrl[NUM_OF_COPY_METER+2];//���������
 extern CARRIER_FLAG_SET    carrierFlagSet;     //�ز���־��
#else
 extern struct copyCtrlWord copyCtrl[NUM_OF_COPY_METER];  //���������
#endif
extern DOT_COPY            *pDotCopy;           //�㳭ָ��

extern DATE_TIME           lastCopyTime;        //�ϴγ���ʱ��(Q/GDW129-2005)
extern DATE_TIME           nextCopyTime;        //��һ�γ���ʱ��
extern INT16U              countOfCopyMeter;    //����ʱ�����

extern struct dotFn129     *pDotFn129;
extern INT8U  dotReplyStart;

//��������
//��.1 ��������
void   copyMeter(void *arg);
void   detectSwitchPulse(void *arg);
void  *threadOf485x1Received(void *arg);
void  *threadOf485x2Received(void *arg);
void   sendMeterFrame(INT8U port,INT8U *pack,INT16U length);
void covertAcSample(INT8U *buffer, INT8U *visionBuf, INT8U *reqBuf, INT8U type, DATE_TIME covertTime);
INT32U times2(INT8U times);
INT32U hexDivision(INT32U dividend, INT32U divisor, INT8U deciBit);

void   reSetCopyTime(void);
BOOL   forwardData(INT8U portNum);
void   forwardDataReply(INT8U portNum);
void   queryMeterStoreInfo(INT16U pn, INT8U *meterInfo);
INT16U findAcPn(void);

//��.2 Q/GDW376.1�ն�
struct cpAddrLink * initPortMeterLink(INT8U port);
INT8U  whichItem(INT8U port);
void   searchMpStatis(DATE_TIME searchTime,void *record,INT16U mp,INT8U type);
BOOL   copy485Failure(INT16U mp,INT8U startOrStop,INT8U port);
BOOL   checkLastDayData(INT16U pn, INT8U type, DATE_TIME queryTime, INT8U queryType);
BOOL   checkLastMonthData(INT16U pn);

//�ز����
#ifdef PLUG_IN_CARRIER_MODULE
 extern INT8U carrierModuleType;                  //�ز�ģ������

 void *threadOfCarrierReceive(void *arg);
 void resetCarrierFlag(void);
 void sendCarrierFrame(INT8U port, INT8U *pack, INT16U length);
 void dotCopyReply(INT8U hasData);
 BOOL dotCopy(INT8U port);
 void checkCarrierMeter(void);
 INT8U checkHourFreezeData(INT16U pn, INT8U *lostBuff);
 BOOL stopCarrierNowWork(void);
 void foundMeterEvent(struct carrierMeterInfo *foundMeter);
#endif 

#ifdef WDOG_USE_X_MEGA
 void xMegaCopyData(INT8U port, INT8U *buf, INT16U len);
#endif

#ifdef PULSE_GATHER
 void fillPulseVar(INT8U type);
 void covertPulseData(INT8U port, INT8U *visionBuf,INT8U *needBuf,INT8U *paraBuff);
#endif

#ifdef SUPPORT_ETH_COPY
 extern INT8U     setEthMeter;      //��վ������̫�������ַ
 extern DATE_TIME msSetWaitTime;    //�ȴ����ñ��ַ��
 extern INT16U    monitorMp;        //��Ҫ����ԭʼ���ݵĲ��Ե��(������̫�������̫��,�����Ҫһ��һ����ʱ��)

 void *threadOfEthCopyServer(void *arg);
 void *threadOfEthCopyDataRead(void *arg);
 void *threadOfEthCopyMeter(void *arg);
#endif

#ifdef LIGHTING
 
 //SLC��ѯ���ݱ�־
 #define REQUEST_STATUS          0x00001   //�����ѯ��ǰ״̬
 #define REQUEST_FREEZE_TIMES    0x00002   //�����ѯ���ص�����
 #define REQUEST_HOUR_FREEZE     0x00004   //�����ѯСʱ��������
 #define REQUEST_CHECK_TIME      0x00008   //����Уʱ
 #define REQUEST_SYN_ADJ_PARA    0x00010   //����ͬ���������
 #define REQUEST_SYN_CTRL_TIME_1 0x00020   //����ͬ��ʱ�ε�һС��
 #define REQUEST_SYN_CTRL_TIME_2 0x00040   //����ͬ��ʱ�εڶ�С��
 #define REQUEST_SYN_CTRL_TIME_3 0x00080   //����ͬ��ʱ�ε���С��,���ƿ�������Ϊ�ز��ŵ���Ҫ������С��,��·���Ƶ�RS485�ŵ�����Ҫ
 #define REQUEST_CTRL_TIME_1     0x00100   //�����ѯ����ʱ�ε�һС��
 #define REQUEST_CTRL_TIME_2     0x00200   //�����ѯ����ʱ�εڶ�С��
 #define REQUEST_CTRL_TIME_3     0x00400   //�����ѯ����ʱ�ε���С��
 #define REQUEST_CTRL_TIME_4     0x00800   //�����ѯ����ʱ�ε���С��
 #define REQUEST_CTRL_TIME_5     0x01000   //�����ѯ����ʱ�ε���С��
 #define REQUEST_CTRL_TIME_6     0x02000   //�����ѯ����ʱ�ε���С��
 #define REQUEST_CTRL_TIME_7     0x04000   //�����ѯ����ʱ�ε���С��
 #define REQUEST_CLOSE_GATE      0x08000   //�����բ
 #define REQUEST_OPEN_GATE       0x10000   //�����բ
 #define REQUEST_SOFT_VER        0x20000   //�����ѯ����汾

 
 //LDGM���

 //#define L_CTRL_WAVE_LUX             25    //��������жϵĲ���ֵ
 //#define L_CTRL_WAVE_LUX             10    //��������жϵĲ���ֵ,2016-03-11,�����żҸ�Ҫ��ĳ�10
 //#define L_CTRL_WAVE_LUX             20    //��������жϵĲ���ֵ,2016-03-17,�޸�Ϊ�ĳ�20
 //#define L_CTRL_WAVE_LUX             17    //��������жϵĲ���ֵ,2016-06-02,�޸�Ϊ�ĳ�17
                                           //2016-05-31,�żҸ۲��Է������ڹ�ؼ�����һ�������һ����������ʱ�οؽ�Ϲ�ص���·���Ƶ�
                                           //           ��բ����1��Сʱ(ʱ����18:55��բ,��ؿ���ǰ��60������Ч,ʵ��19:55��բ),
                                           //           ��բ����1��Сʱ(ʱ�ΰ�ҹ��23:00��բ,����ڹص�ǰ��60������Ч,ʵ��22:00��բ;ȫҹ��4:57��բ,ʵ��3:57��բ)
                                           //������,����ԭ��:�żҸ����õ���20Lux-0%,ԭ������ֵ���õ�20,��ô�ڹ�ؼ���������ǰ��ע��ֵΪ18:47(δ�����ط�Χ)������ֵΪ326Lux,
                                           //       �����ؼ��������ߣ���19:12��ؼ��������߿ɴ�ʱ����ֵ��Ϊ0Lux��,
                                           //       a.�����жϹ�����������ֵС��20+20=40Lux��һ�α�һ������ֵС����������÷����������ʱ��,
                                           //       �Ӷ��ò������Ҫ���բ��Ҫ��
                                           //       b.��һ���ж�����tmpLux>(nowLux+L_CTRL_WAVE_LUX)(����ǰ����ֵС��(�ص�ע��ֵ-��ֵ))Ҳ������,��Ϊ20Lux-20��ֵ=0,
                                           //       Ҫ��ǰֵ0С��0�����ܳ���,
                                           //       ����޸�Ϊ��ֵ17,�������������ʱ�ɱ��������
 //#define L_CTRL_WAVE_LUX             29    //��������жϵĲ���ֵ,2016-06-30,��17�޸�Ϊ�ĳ�29
                                           //  Ӧ�żҸ�Ҫ���޸�,��Ϊ�����ֿ������˵㣬���������20���ϵ�һ�£�ֻ�ܵ�30���ĺ�Χ���1-59.
 //2016-08-24,����ն���ֵ�ĳɲ���,�����޸�,ȡ����(L_CTRL_WAVE_LUX)����
 
 struct lightCtrl
 {
   INT8U bright;                           //����
   INT8U lcDetectBright;                   //��ؼ����Ӧ��ִ�е�����
 };
 
 extern struct ctrlTimes  *cTimesHead;
 extern struct cpAddrLink *xlcLink;
 extern struct cpAddrLink *ldgmLink;
 extern struct cpAddrLink *lsLink;
 extern struct cpAddrLink *acLink;         //2015-12-05,add
 extern struct cpAddrLink *thLink;		     //2018-05-11,add

 extern struct lightCtrl  lCtrl;           //��ر�־��

 INT8U initXlcLink(void);
 INT8U initLdgmLink(void);
 INT8U initLsLink(void);
 INT8U initAcLink(void);
 
 void lcProcess(INT8U *lastLux, INT32U nowLux);

#endif

#endif   //__copyMeterH
