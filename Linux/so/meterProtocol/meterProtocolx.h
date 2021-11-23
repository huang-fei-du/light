/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ�����meterProtocol.h
����:leiyong
�汾��0.9
�������:2009��1��10��
���������ܱ�Э��˽��ͷ�ļ�
�޸���ʷ��
  01,09-01-07,Leiyong created.
**************************************************/

#ifndef __meterProtocolxH
#define __meterProtocolxH

#include "common.h"
#include "workWithMeter.h"
#include "meterProtocol.h"

//#define  SHOW_DEBUG_INFO                       //��ʾ������Ϣ

#define JF_MONITOR                               //�������

//�������ܵ��ܱ�DL/T645-1997��Լ
#define PROTOCOL_645_1997                       DLT_645_1997
#ifdef PROTOCOL_645_1997 
  #define TOTAL_CMD_CURRENT_645_1997             79  //DL/T645-1997��ǰ������������
  #define TOTAL_CMD_LASTMONTH_645_1997           16  //DL/T645-1997����������������
#endif

//�������ܵ��ܱ�DL/T645-2007��Լ
#define PROTOCOL_645_2007                       DLT_645_2007
#ifdef PROTOCOL_645_2007 
  
  #ifdef DKY_SUBMISSION
   #define TOTAL_CMD_CURRENT_645_2007            62  //DL/T645-2007��ǰ������������
  #elif defined JF_MONITOR
	 #define TOTAL_CMD_CURRENT_645_2007						 22	 //DL/T645-2007��ǰ������������	 
  #else
   #define TOTAL_CMD_CURRENT_645_2007           100  //DL/T645-2007��ǰ������������
  #endif
  
  #define TOTAL_CMD_LASTMONTH_645_2007           28  //DL/T645-2007 ��һ������������������
  #define TOTAL_CMD_LASTDAY_645_2007             11  //DL/T645-2007 ��һ���ն���������������
  #define TOTAL_CMD_LASTDAY_SINGLE_07             3  //�����07     ��һ���ն���������������
#endif

//�����DL/T645-1997
#define PROTOCOL_SINGLE_PHASE_97                SINGLE_PHASE_645_1997
#ifdef  PROTOCOL_SINGLE_PHASE_97
  #define TOTAL_CMD_SINGLE_645_97                 1  //����97����ʵʱ������������
  #define TOTAL_CMD_LASTDAY_SINGLE_97             1  //����97������һ���ն���������������
#endif

//DL/T645-1997�����ֻ���������޹�����ʾֵ
#define PROTOCOL_PN_WORK_NOWORK_97              PN_WORK_NOWORK_1997
#ifdef  PROTOCOL_PN_WORK_NOWORK_97
  #define TOTAL_CMD_PN_WORK_NOWORK_645_97         6  //ʵʱ������������
  #define TOTAL_CMD_PN_WORK_NOWORK_97             0  //��һ���ն���������������
#endif

//DL/T645-2007�����ֻ���������޹�����ʾֵ
#define PROTOCOL_PN_WORK_NOWORK_07              PN_WORK_NOWORK_2007
#ifdef  PROTOCOL_PN_WORK_NOWORK_07
  #define TOTAL_CMD_PN_WORK_NOWORK_645_07         6  //ʵʱ������������
  #define TOTAL_CMD_PN_WORK_NOWORK_07             0  //��һ���ն���������������
#endif


//�������ܵ��ܱ�DL/T645-2007
#define PROTOCOL_SINGLE_PHASE_07                SINGLE_PHASE_645_2007
#ifdef  PROTOCOL_SINGLE_PHASE_07
  #define TOTAL_CMD_SINGLE_645_07                 7  //�������ܵ��07��ʵʱ������������
#endif

//���౾�طѿ����ܵ��ܱ�DL/T645-2007
#define PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007  SINGLE_LOCAL_CHARGE_CTRL_2007
#ifdef  PROTOCOL_SINGLE_LOCAL_CHARGE_CTRL_2007
  #define TOTAL_CMD_SINGLE_LOCAL_CTRL_07         17  //���౾�طѿ����ܵ����ʵʱ������������
#endif

//����Զ�̷ѿ����ܵ��ܱ�DL/T645-2007
#define PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007 SINGLE_REMOTE_CHARGE_CTRL_2007
#ifdef  PROTOCOL_SINGLE_REMOTE_CHARGE_CTRL_2007
  #define TOTAL_CMD_SINGLE_REMOTE_CTRL_07         9  //����Զ�̷ѿ����ܵ����ʵʱ������������
#endif

//�������ܵ��ܱ�DL/T645-2007
#define PROTOCOL_THREE_2007   THREE_2007
#ifdef  PROTOCOL_THREE_2007
  #define TOTAL_CMD_THREE__07                    17  //�������ܵ����ʵʱ������������
#endif

//���౾�طѿ����ܵ��ܱ�DL/T645-2007
#define PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007   THREE_LOCAL_CHARGE_CTRL_2007
#ifdef  PROTOCOL_THREE_LOCAL_CHARGE_CTRL_2007
  #define TOTAL_CMD_THREE_LOCAL_CTRL_07          27  //���౾�طѿ����ܵ����ʵʱ������������
#endif

//����Զ�̷ѿ����ܵ��ܱ�DL/T645-2007
#define PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007  THREE_REMOTE_CHARGE_CTRL_2007
#ifdef  PROTOCOL_THREE_REMOTE_CHARGE_CTRL_2007
  #define TOTAL_CMD_THREE_REMOTE_CTRL_07         19  //����Զ�̷ѿ����ܵ����ʵʱ������������
#endif

#define PROTOCOL_KEY_HOUSEHOLD_2007             KEY_HOUSEHOLD_2007
#ifdef  PROTOCOL_KEY_HOUSEHOLD_2007
  #define TOTAL_CMD_KEY_2007                      8  //07��Լ���ص��û�ʵʱ������������
  #define TOTAL_CMD_LASTDAY_KEY_2007              3  //DL/T645-2007�ص��û���һ���ն���������������
#endif

//#define LANDIS_GRY_ZD_PROTOCOL SIMENS_ZD_METER   //������(/������)ZD��Э��
#ifdef  LANDIS_GRY_ZD_PROTOCOL
  #define TOTAL_COMMAND_REAL_ZD                   2  //������(/������)ZD��Э���й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_ZD              0  //������(/������)ZD��Э���й����������ݵ���������
  #define TOTAL_COMMAND_PARA_ZD                   0  //������(/������)ZD��Э���й��ڲ������ݵ���������
  #define TOTAL_DATA_ITEM_ZD                     79  //������(/������)ZD��Э��������Ϣ��������
#endif

//#define LANDIS_GRY_ZB_PROTOCOL SIMENS_ZB_METER   //������(/������)ZB��Э��
#ifdef  LANDIS_GRY_ZB_PROTOCOL
  #define TOTAL_COMMAND_REAL_ZB                   2  //������(/������)ZB��Э���й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_ZB              0  //������(/������)ZB��Э���й����������ݵ���������
  #define TOTAL_COMMAND_PARA_ZB                   0  //������(/������)ZB��Э���й��ڲ������ݵ���������
#endif

//#define PROTOCOL_ABB_GROUP  ABB_METER        //ABB��Э��
#ifdef  PROTOCOL_ABB_GROUP
  #define TOTAL_COMMAND_REAL_ABB                 18  //ABB��Э���й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_ABB             0  //ABB��Э���й����������ݵ���������
  #define TOTAL_COMMAND_PARA_ABB                  0  //ABB��Э���й��ڲ������ݵ���������
#endif

//#define PROTOCOL_EDMI_GROUP EDMI_METER       //EDMI��Э��
#ifdef  PROTOCOL_EDMI_GROUP
  #define TOTAL_COMMAND_REAL_EDMI                59  //EDMI��Э���й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_EDMI           43  //EDMI��Э���й����������ݵ���������
  #define TOTAL_COMMAND_PARA_EDMI                 0  //EDMI��Э���й��ڲ������ݵ���������
#endif

//#define PROTOCOL_WASION_GROUP WASION_METER   //��ʢ��ԼЭ���
#ifdef  PROTOCOL_WASION_GROUP
  #define TOTAL_COMMAND_REAL_WASION              10  //��ʢ��ԼЭ����й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_WASION         10  //��ʢ��ԼЭ����й����������ݵ���������
  #define TOTAL_COMMAND_PARA_WASION              18  //��ʢ��ԼЭ����й��ڲ������ݵ���������
  #define TOTAL_COMMAND_EXPAND_WASION             1  //645��ԼЭ�������չ���ֵ���������
#endif

//#define PROTOCOL_HONGHUA_GROUP HONGHUA_METER //�뻪��ԼЭ���
#ifdef  PROTOCOL_HONGHUA_GROUP
  #define TOTAL_COMMAND_REAL_HONGHUA           //�뻪��ԼЭ����й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_HONGHUA      //�뻪��ԼЭ����й����������ݵ���������
  #define TOTAL_COMMAND_PARA_HONGHUA           //�뻪��ԼЭ����й��ڲ������ݵ���������
#endif

//#define LOCALE_SUPERVISAL_DEVICE SUPERVISAL_DEVICE //�ֳ�����豸��Լ
#ifdef  LOCALE_SUPERVISAL_DEVICE
  #define TOTAL_COMMAND_REAL_LOCALE            //�ֳ�����豸��Լ�й���ʵʱ���ݵ���������
  #define TOTAL_COMMAND_LASTMONTH_LOCALE       //�ֳ�����豸��Լ�й����������ݵ���������
  #define TOTAL_COMMAND_PARA_LOCALE            //�ֳ�����豸��Լ�й��ڲ������ݵ���������
#endif

#define PROTOCOL_MODUBUS_GROUP     MODBUS_HY    //MODBUS��ԼЭ���


//2.���ص����ݱ�־
#define HAS_CURRENT_ENERGY                       0x1  //�е�ǰ������α�������
#define HAS_CURRENT_REQ                          0x2  //�е�ǰ��������
#define HAS_PARA_VARIABLE                        0x4  //�в������α�������
#define HAS_SHIDUAN                              0x8  //��ʱ������
#define HAS_LAST_MONTH_ENERGY                   0x10  //�����µ�������
#define HAS_LAST_MONTH_REQ                      0x20  //��������������
#define HAS_LAST_DAY_ENERGY                     0x40  //����һ���ն����������
#define HAS_LAST_DAY_REQ                        0x80  //����һ���ն�����������
#define HAS_HOUR_FREEZE_ENERGY                 0x100  //�����㶳���������

//3.�����ֶα�־λ(��������ʱ��)--------------------------------------------------------
//3.1 DL/T645-1997
#ifdef PROTOCOL_645_1997 
  #define READ_DATA_645_1997                      0x1   //������
  #define READ_D_FOLLOW_645_1997                 0x81   //������Ӧ���޺�������֡
  #define READ_D_LAST_645_1997                   0xA1   //������Ӧ���к�������֡
  #define READ_D_ERROR_645_1997                  0xC1   //������Ӧ���쳣Ӧ��
  #define READ_FOLLOWING_DATA_645_1997            0x2   //���������
  #define READ_F_FOLLOW_645_1997                 0x82   //����������Ӧ���޺�������֡
  #define READ_F_LAST_645_1997                   0xA2   //����������Ӧ���к�������֡
  #define READ_F_ERROR_645_1997                  0xC2   //����������Ӧ���쳣Ӧ��
  #define REREAD_DATA_645_1997                    0x3   //�ض�����
  #define REREAD_FOLLOW_645_1997                 0x83   //�ض�����Ӧ���޺�������֡
  #define REREAD_LAST_645_1997                   0xA3   //�ض�����Ӧ���к�������֡
  #define REREAD_ERROR_645_1997                  0xC3   //�ض�����Ӧ���쳣Ӧ��
  #define WRITE_DATA_645_1997                     0x4   //д����
  #define WRITE_OK_645_1997                      0x84   //д����Ӧ������Ӧ��
  #define WRITE_ERROR_645_1997                   0xC4   //д����Ӧ���쳣Ӧ��  
  #define TIME_ADJUSTING_645_1997                 0x8   //�㲥Уʱ
  #define WRITE_ADDR_645_1997                     0xA   //д�豸��ַ
  #define WRITE_ADDR_OK_645_1997                 0x8A   //д�豸��ַӦ������Ӧ��  
  #define SWITCH_SPEED_645_1997                   0xC   //����ͨ������
  #define SWITCH_SPEED_REP_645_1997              0x8C   //����ͨ������Ӧ��  
  #define CHANGE_PASSWORD_645_1997                0xF   //��������
  #define CHANGE_PASSWORD_REP_645_1997           0x8F   //��������Ӧ��  
  #define CLEAR_645_1997                         0x10   //�����������
  #define LAST_FRAME_645_1997                    0x80   //�޺���֡
  #define FOLLOW_FRAME_645_1997                  0xA0   //�к���֡
  #define DATA_ERROR_645_1997                     0x2   //���ݴ���
#endif   //PROTOCOL_645_1997

//3.2 DL/T645-2007
#ifdef PROTOCOL_645_2007
  #define READ_DATA_645_2007                     0x11   //������
  #define READ_D_FOLLOW_645_2007                 0x91   //������Ӧ���޺�������֡
  #define READ_D_LAST_645_2007                   0xB1   //������Ӧ���к�������֡
  
  #define NORMAL_REPLY                           0x90   //10010000(D7=1,��վ������Ӧ��֡ D6=0,��վ����Ӧ�� D4=1 2007�̶���1
  #define ABERRANT_REPLY                         0xD0   //11010000(D7=1,��վ������Ӧ��֡ D6=1,��վ�쳣Ӧ�� D4=1 2007�̶���1
                                                        
  #define ENERGY_2007                            0x00   //DI3Ϊ0���ǵ�����
  #define REQ_AND_REQ_TIME_2007                  0x01   //DI3Ϊ1������������ʱ��
  #define VARIABLE_2007                          0x02   //����
  #define EVENT_RECORD_2007                      0x03   //�¼���¼
  #define PARA_VARIABLE_2007                     0x04   //�α���
  #define FREEZE_DATA_2007                       0x05   //��������
  #define EXT_EVENT_RECORD_13                    0x13   //�����ļ�(����ͳ������)
  
#endif   //PROTOCOL_645_2007

#ifdef PROTOCOL_EDMI_GROUP
  #define EDMI_STX                                  2   //֡ͷ
  #define EDMI_ETX                                  3   //֡�����ַ�
  #define EDMI_ACK                                  6   //ȷ��
  #define EDMI_DLE                                 16   //DLE
  #define EDMI_CAN                                 24   //CRCУ����ȷ,��ָ������
  #define EDMI_XON                                 17
  #define EDMI_XOFF                                19
#endif

#endif /*__meterProtocolxH*/
