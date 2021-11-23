/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
�ļ�����wirelessModemx.h
���ߣ�leiyong
�汾��0.9
�������:2010��01��24��
����������Modem��̬���ӿ�˽��ͷ�ļ�
      �����������ô�нṹ����
      
�޸���ʷ��
  01,10-01-24,leiyong created.
**************************************************/

#ifndef __wirelessModemxH
#define __wirelessModemxH

#include "common.h"

//#define SIZE_OF_WIRELESS        128      //���߽��ջ����С
#define SIZE_OF_WIRELESS       2048      //���߽��ջ����С
#define SIZE_OF_RECV_MS_BUF    2048      //������վ��֡��С
#define DATA_FROM_GPRS          0x2      //��վ��������GPRS�ӿ�

/***********************************************************
*AT��������
***************************/
#define AT_NONE                 0x0      //����AT����

//GPRS & CDMA����
#define AT_ECHO                 0x1      //����
#define AT_SIGNAL               0x2      //�ź�
#define AT_DAILING              0x3      //����
#define AT_REG                  0x4      //ע��������?
#define AT_COPS                 0x5      //��ѯ��Ӫ��
#define AT_IP_START            0x10      //IP start

//GPRS����
#define AT_SIM                  0x5      //SIM����
#define AT_DCD                  0x6      //DCD����
#define AT_CGDCONT              0x7      //����CGDCONT����
#define AT_TO_AT                0x8      //ת��GPRS����ģʽ��AT����ģʽ
#define AT_TO_GPRS              0x9      //ת��ATģʽ��GPRSģʽ
#define AT_RE_SIGNAL            0xa      //��̬����ź�
#define AT_IP_MODE              0xb      //IPģʽ
#define AT_TRANSPARENT          0xc      //͸������
#define AT_APN                  0xd      //����APN
#define AT_CIICR                0xe      //�����ƶ�����
#define AT_CIFSR                0xf      //��ȡIP��ַ
#define AT_LC_TCP_PORT         0x61      //���ñ���TCP�˿�
#define AT_CGATT               0x62      //���Ż����GPRS
#define AT_CGREG               0x63      //GPRSע��������

//CDMA����
#define AT_IPR                 0x11      //������������
#define AT_UIM                 0x12      //UIM���Ƿ����
#define AT_TIME                0x13      //����ʱ��
#define AT_CRM                 0x14      //����ͨ��Э��
#define AT_DIP                 0x15      //����Ŀ��IP��ַ
#define AT_DPORT               0x16      //����Ŀ�Ķ˿�
#define AT_SPC                 0x17      //����SPC��
#define AT_AINF                0x18      //�������� TCPIP Э���Ȩ��Ϣ

//GR64ר��
#define AT_GR64_IPS            0x20      //IPS(����IP����)
#define AT_GR64_IPA            0x21      //IPA
#define AT_GR64_ADDR           0x22      //���PDP IP��ַ

//MC52i(MC5x)ר��
#define AT_MC5X_SET_CREG       0x31      //ע��������ʾ
#define AT_MC5X_QUERY_CREG     0x32      //��ѯע���������
#define AT_MC5X_SET_CGREG      0x33      //GPRSע��������ʾ
#define AT_MC5X_QUERY_CGREG    0x34      //��ѯGPRSע���������
#define AT_MC5X_SELECTT_CONN   0x35      //ѡ����������(Select connection type GPRS0)
#define AT_MC5X_SOCKET         0x36      //���÷�������ΪSocket
#define AT_MC5X_CON_ID         0x37      //��������ID
#define AT_MC5X_IP_PORT        0x38      //����IP��ַ���˿�
#define AT_MC5X_REQUEST_SEND   0x39      //������

//m590eר��
#define AT_M590E_XISP          0x41      //�����ڲ�Э��ջ
#define AT_M590E_SET_XIIC      0x42      //����PPP����
#define AT_M590E_QUERY_XIIC    0x43      //��ѯPPP����
#define AT_M590E_IPSTATUS      0x44      //��ѯ��·״̬
#define AT_M590E_TCPCLOSE      0x45      //�ر���·��TCP����

//MC180ר��
#define AT_CM180_PNUM          0x51      //��������ҵ�����
#define AT_CM180_GETIP         0x52      //��ѯģ��IP��ַ
#define AT_CM180_PPPSTATUS     0x53      //��ѯ��ǰPPP����״̬

//M72Dר��
#define AT_QIFGCNT             0x71      //������
#define AT_QIAPN               0x72      //APN
#define AT_QIMUX               0x73      //IP MUX
#define AT_QIMODE              0x74      //IP Mode
#define AT_QITCFG              0x75      //
#define AT_QIDNSIP             0x76      //
#define AT_QIREGAPP            0x77      //
#define AT_QIACT               0x78      //
#define AT_QILOCIP             0x79      //


/************************************************************/

#define DAIL_TIMEOUT             40      //���ų�ʱ?

#define HEART_WAIT_TIME          60      //����ȷ�ϵȴ�ʱ��(s)

//����AT���������λظ��Ĵ���:��һ�λظ��յ�����һ����־,���ڶ���(һ����OK)�յ�����Ϊ�ɹ�
#define AT_M_RESPONSE_NONE       0x0     //��λظ�-��Ч
#define AT_M_RESPONSE_INFO_ACK   0x1     //��λظ�-��Ϣ��ȷ��
#define AT_M_RESPONSE_INFO_NACK  0x2     //��λظ�-��Ϣ�η���
#define AT_M_RESPONSE_OK         0x3     //��λظ�-����õ�ȷ��

//���߽ӿڱ�־��
typedef struct
{
	 unsigned int startup       :1;        //�ϵ翪�������־
	 unsigned int atFrame       :3;        //AT֡��־
	 unsigned int dialConnected :1;        //����ģ��[GPRS/CDMA/etc]���ųɹ�(����)��־
   unsigned int ipr           :1;        //�����������óɹ�?

   unsigned int callReady     :1;        //������Ӧ��(ͨ��׼����)
   unsigned int echo          :1;        //���Թر�
   unsigned int cardInserted  :2;        //�Ƿ�忨[�����λظ�,�����2λ]
   unsigned int dcd           :1;        //DCD���ñ�־
   unsigned int cgdcont       :1;        //����CGDCONT�����ɹ�����־
   unsigned int signal        :2;        //��ȡ�ź�����
   unsigned int wlLcTcpPort   :1;        //���ñ���TCP�˿�    ly,2011-12-28,add
   unsigned int creg          :2;        //ע��������?        ly,2012-02-09,add
   unsigned int cgreg         :2;        //GPRSע��������?    ly,2012-05-18,add
   unsigned int cops          :2;        //��Ӫ�̲�ѯ,    2018-12-27,add

   unsigned int ipMode        :1;        //Select TCPIP Application mode
   unsigned int ipStarted     :1;        //START UP TCP OR UDP CONNECTION
   unsigned int wlSendStatus  :3;        //������MODEM��������״̬  	
   

   //SIM300C���б�־λ----start
   unsigned int simTransparent:1;        //͸������
   unsigned int simApn        :1;        //APN
   unsigned int simCiicr      :7;        //�����ƶ�����
   unsigned int simCifsr      :1;        //��ȡ����IP��ַ
   unsigned int simCgatt      :2;        //���źͷ���GPRS
   //SIM300C���б�־λ----end

   //GR64���б�־λ----start
   unsigned int gr64Ips       :1;        //GR64����IP����IPS
   unsigned int gr64Ipa       :1;        //GR64 IPA
   unsigned int numOfGr64Ipa  :2;        //
   unsigned int gr64Addr      :1;        //��ȡPDP IP��ַ?
   	
   //GR64���б�־λ----end
   
   //DTGS-800���б�־λ----start
   unsigned int dtgsRegNet    :2;        //DTGS-800ע������
   unsigned int dtgsTime      :1;        //DTGS-800ʱ���Ѷ�ȡ?
   unsigned int dtgsCrm       :1;        //DTGS-800���ô���ͨѶЭ��
   unsigned int dtgsDip       :1;        //DTGS-800����Ŀ��IP��ַ
   unsigned int dtgsDPort     :1;        //DTGS-800����Ŀ�Ķ˿�
   unsigned int dtgsDial      :1;        //DTGS-800����ָ���Ƿ���?
   unsigned int dtgsSpc       :1;        //DTGS-900����SPC��
   unsigned int dtgsAinf      :1;        //DTGS-800��������TCP/IPЭ���Ȩ��Ϣ
   unsigned int ainfInputed   :1;        //DTGS-800��Ȩ��Ϣ������
   //unsigned int dtgsCta       :1;      //DTGS-800����
   //DTGS-800���б�־λ----end

   //MC52i(MC5x)���б�־λ----start
   unsigned int mc5xSetCreg   :1;        //MC5x����ע����ʾ
   unsigned int mc5xQueryCreg :2;        //MC5x�������ע��
   unsigned int mc5xSetCGreg  :1;        //MC5x����GPRSע����ʾ
   unsigned int mc5xQueryCGreg:2;        //MC5x���GPRS����ע��
   unsigned int mc5xSelectConn:1;        //MC5xѡ����������
   unsigned int mc5xSocket    :1;        //MC5x���÷�������ΪSocket
   unsigned int mc5xConId     :1;        //MC5x��������Id
   unsigned int mc5xIpPort    :1;        //MC5x����IP��ַ���˿�
   unsigned int mc5xDial      :1;        //MC5x���������ѷ���
   unsigned int mc5xLastSended:1;        //MC5x��һ֡�ѷ���
   unsigned int mc5xRecvStatus:3;        //MC5x����״̬
   	
   //MC52i���б�־λ----end
   
   //M590E���б�־λ----start
   unsigned int m590eQueryCreg :2;             //��ѯģ���Ƿ�ע����GSM����
   unsigned int m590eXisp      :1;             //�����ڲ�Э��ջ
   unsigned int m590eSetXiic   :1;             //����PPP����
   unsigned int m590eQueryXiic :2;             //��ѯPPP����
   unsigned int numOfConnect   :2;             //TCP���Ӵ���
   //M590E���б�־λ----end

   //CM180���б�־λ----start
   unsigned int cm180pnum      :1;             //��������ҵ�����
   unsigned int cm180plogin    :1;             //��������ҵ���û���������
   unsigned int cm180QueryCreg :2;             //��ѯģ���Ƿ�ע��������
   unsigned int cm180Getip     :2;             //��ѯģ��IP��ַ
   unsigned int cm180Dial      :1;             //����ָ���Ƿ���
   unsigned int	cm180pppStatus :2;             //��ѯPPP����״̬
   unsigned int cm180ipStatus  :2;             //��ѯTCP����״̬
   //CM180���б�־λ----end 	

   //M72-D���б�־λ----start
   unsigned int m72qifgcnt     :1;             //������
   unsigned int m72Apn         :1;             //APN
   unsigned int m72mux         :1;             //MUX
   unsigned int m72IpMode      :1;             //IP Mode
   unsigned int m72Itcfg       :1;             //
   unsigned int m72dnsip       :1;             //
   unsigned int m72regapp      :1;             //
   unsigned int m72act         :1;             //
   unsigned int m72locip       :1;
   	
   //M72-D���б�־λ----end
   
}WL_FLAG_SET;

char * intToString(INT32U in,int type,char *returnStr);
char * trim(char * s);
char * loginIpAddr(void);
char * loginPort(void);
void   closeLinkReport(void);
void   singleReport(INT8U rssi);
INT8U  atResponse(INT8U *pFrame,INT8U len);

#endif  //__wirelessModemxH
