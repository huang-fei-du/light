/*************************************************
Copyright,2006,LongTong co.,LTD
�ļ�����workWithMeter.h
���ߣ�TianYe
�汾��0.9
�������:2006��5��31��
�������ն�����ͨ��ͷ�ļ�
�޸���ʷ:
  01,06-5-31,Tianye created.
  02,09-03-09,Leiyong�淶���ļ�.��ϸע�͸���������;.
  03,10-01-14,Leiyong�޸Ĵ洢ƫ����ӦDL/T645-1997��DL/T645-2007.
**************************************************/
#ifndef __INCworkWithMeterh
#define __INCworkWithMeterh

#include "common.h"
#include "timeUser.h"

//���Ժ��?
//#define  DKY_SUBMISSION

//2.���͸����ܱ��֡��С
#define  SIZE_OF_METER_FRAME                   64

//3.�������
#define  POSITIVE_WORK                         0x1   //�����й���ʾֵ
#define  POSITIVE_NO_WORK                      0x2   //�����޹���ʾֵ
#define  NEGTIVE_WORK                          0x3   //�����й���ʾֵ
#define  NEGTIVE_NO_WORK                       0x4   //�����޹���ʾֵ
#define  QUA1_NO_WORK                          0x5   //һ�����޹���ʾֵ
#define  QUA2_NO_WORK                          0x6   //�������޹���ʾֵ
#define  QUA3_NO_WORK                          0x7   //�������޹���ʾֵ
#define  QUA4_NO_WORK                          0x8   //�������޹���ʾֵ

#define  REQ_TIME_P_WORK                       0x9   //�����й������������ʱ��
#define  REQ_TIME_P_NO_WORK                    0xA   //�����޹������������ʱ��
#define  REQ_TIME_N_WORK                       0xB   //�����й������������ʱ��
#define  REQ_TIME_N_NO_WORK                    0xC   //�����޹������������ʱ��
#define  QUA1_REQ_TIME_NO_WORK                 0xD   //һ�����޹������������ʱ��
#define  QUA2_REQ_TIME_NO_WORK                 0xE   //�������޹������������ʱ��
#define  QUA3_REQ_TIME_NO_WORK                 0xF   //�������޹������������ʱ��
#define  QUA4_REQ_TIME_NO_WORK                0x10   //�������޹������������ʱ��

#define  REQ_POSITIVE_WORK                    0x11   //�����й��������
#define  REQ_POSITIVE_NO_WORK                 0x12   //�����޹��������
#define  REQ_NEGTIVE_WORK                     0x13   //�����й��������
#define  REQ_NEGTIVE_NO_WORK                  0x14   //�����޹��������
#define  QUA1_REQ_NO_WORK                     0x15   //һ�����޹��������
#define  QUA2_REQ_NO_WORK                     0x16   //�������޹��������
#define  QUA3_REQ_NO_WORK                     0x17   //�������޹��������
#define  QUA4_REQ_NO_WORK                     0x18   //�������޹��������

//3-1.�������ݽ�����չ���
#define  WORK_ENERGY                          0x1D   //�й�������
#define  NO_WORK_ENERGY                       0x1E   //�޹�������
#define  MONTH_WORK_ENERGY                    0x1F   //���й�������
#define  MONTH_NO_WORK_ENERGY                 0x20   //���޹�������

#define  DAY_P_WORK                           0x21   //�������й�������
#define  DAY_P_NO_WORK                        0x22   //�������޹�������
#define  DAY_N_WORK                           0x23   //�շ����й�������
#define  DAY_N_NO_WORK                        0x24   //�շ����޹�������

#define  MONTH_P_WORK                         0x31   //�������й�������
#define  MONTH_P_NO_WORK                      0x32   //�������޹�������
#define  MONTH_N_WORK                         0x33   //�·����й�������
#define  MONTH_N_NO_WORK                      0x34   //�·����޹�������

#define  MONTH_NO_WORK_QUA1                   0x35   //��һ�����޹�������
#define  MONTH_NO_WORK_QUA2                   0x36   //�¶������޹�������
#define  MONTH_NO_WORK_QUA3                   0x37   //���������޹�������
#define  MONTH_NO_WORK_QUA4                   0x38   //���������޹�������

#define  COPYDAY_FREEZE_POSI_WORK             0x70   //�����ն��������й�ʾֵ
#define  COPYDAY_FREEZE_POSI_NO_WORK          0x71   //�����ն��������޹�ʾֵ
#define  COPYDAY_FREEZE_NEG_WORK              0x72   //�����ն��ᷴ���й�ʾֵ
#define  COPYDAY_FREEZE_NEG_NO_WORK           0x73   //�����ն��ᷴ���޹�ʾֵ

#define  COPYDAY_FREEZE_NO_WORK_QUA1          0x74   //�����ն���һ�����޹�ʾֵ
#define  COPYDAY_FREEZE_NO_WORK_QUA2          0x75   //�����ն���������޹���ʾֵ
#define  COPYDAY_FREEZE_NO_WORK_QUA3          0x76   //�����ն����������޹���ʾֵ
#define  COPYDAY_FREEZE_NO_WORK_QUA4          0x77   //�����ն����������޹���ʾֵ

#define  COPYDAY_FREEZE_POSI_WORK_REQ_TIME    0x78   //�����ն��������й������������ʱ��
#define  COPYDAY_FREEZE_POSI_NO_WORK_REQ_TIME 0x79   //�����ն��������޹������������ʱ��
#define  COPYDAY_FREEZE_NEG_WORK_REQ_TIME     0x7A   //�����ն��ᷴ���й������������ʱ��
#define  COPYDAY_FREEZE_NEG_NO_WORK_REQ_TIME  0x7B   //�����ն��ᷴ���޹������������ʱ��

#define  COPYDAY_FREEZE_POSI_WORK_REQ         0x7C   //�����ն��������й��������
#define  COPYDAY_FREEZE_POSI_NO_WORK_REQ      0x7D   //�����ն��������޹��������
#define  COPYDAY_FREEZE_NEG_WORK_REQ          0x7E   //�����ն��ᷴ���й��������
#define  COPYDAY_FREEZE_NEG_NO_WORK_REQ       0x7F   //�����ն��ᷴ���޹��������

#define  POWER_DATA_REFERENCE                 0x40   //�����б����¼������Ĳο���¼

//3-2.�α������ݽ�����չ���(�洢�ڱ�������������)
#define  STASTIC_POWER                        0x60   //����ͳ������
#define  STASTIC_VOLTAGE                      0x61   //��ѹͳ������
#define  STASTIC_CURRENT                      0x62   //����ͳ������
#define  POWER_SUPERIOD                       0x63   //���ڹ���Խ��ͳ��
#define  PHASE_DOWN_STATISTIC                 0x64   //����ͳ��
#define  REQ_STATISTIC                        0x65   //�������ͳ��


//4.ʵʱ����ҳ�ڴ洢ƫ��(1,2)
//ÿ�γ���(��ȫ�����ݵĻ�)ÿ����������洢4ҳ,��1ҳΪʾֵ,��2ҳΪ����,��3�α���,��3ҳΪʱ���йزα���
//�������ص���������һ���ܼ�8������(��9������)�Ĵ洢�ռ�,
//���Ҫ���ӷ������ĸĶ�����ǳ���,��ȻҪ��ʵʱ���ݴ洢��ʼ����,����ǽ��������
//4-0.ÿ�����͵ļ�¼����
#define  LENGTH_OF_ENERGY_RECORD               344   //��������¼����
#define  LENGTH_OF_REQ_RECORD                  288   //������¼����
#define  LENGTH_OF_PARA_RECORD                 294   //����������¼����
#define  LENGTH_OF_SHIDUAN_RECORD              487   //ʱ�μ�¼����
#define  LENGTH_OF_KEY_ENERGY_RECORD           144   //�ص��û���������¼����
#define  LENGTH_OF_KEY_PARA_RECORD              42   //�ص��û��α�����¼����

//4-1.ʾֵ��������ҳ�ڴ洢ƫ��(1ҳ�ڴ洢1����1�ε�ʾֵ��ƫ��)
//4-1-1.ʾֵ(ʾֵ���ݾ�Ϊ4���ֽ�,�洢��ʽΪxxxxxx.xx(BCD��),��λ�洢�ڴ洢���ĵ��ֽ�)
#define  POSITIVE_WORK_OFFSET                    0   //�����й�����ʾֵ((1+8)*4Bytes)
#define  NEGTIVE_WORK_OFFSET                    36   //�����й�����ʾֵ((1+8)*4Bytes)
#define  POSITIVE_NO_WORK_OFFSET                72   //�����޹�����ʾֵ((1+8)*4Bytes)(DL/T645-2007����Ϊ����޹�1)
#define  NEGTIVE_NO_WORK_OFFSET                108   //�����޹�����ʾֵ((1+8)*4Bytes)(DL/T645-2007����Ϊ����޹�2)
#define  QUA1_NO_WORK_OFFSET                   144   //һ�����޹�����ʾֵ((1+8)*4Bytes)
#define  QUA4_NO_WORK_OFFSET                   180   //�������޹�����ʾֵ((1+8)*4Bytes)
#define  QUA2_NO_WORK_OFFSET                   216   //�������޹�����ʾֵ((1+8)*4Bytes)
#define  QUA3_NO_WORK_OFFSET                   252   //�������޹�����ʾֵ((1+8)*4Bytes)
#define  COPPER_LOSS_TOTAL_OFFSET              288   //ͭ���й��ܵ���ʾֵ(������,4bytes)
#define  IRON_LOSS_TOTAL_OFFSET                292   //�����й��ܵ���ʾֵ(������,4bytes)
#define  POSITIVE_WORK_A_OFFSET                296   //A�������й�����(4Bytes)
#define  NEGTIVE_WORK_A_OFFSET                 300   //A�෴���й�����(4Bytes)
#define  COMB1_NO_WORK_A_OFFSET                304   //A������޹�1����(4Bytes)
#define  COMB2_NO_WORK_A_OFFSET                308   //A������޹�2����(4Bytes)
#define  POSITIVE_WORK_B_OFFSET                312   //B�������й�����(4Bytes)
#define  NEGTIVE_WORK_B_OFFSET                 316   //B�෴���й�����(4Bytes)
#define  COMB1_NO_WORK_B_OFFSET                320   //B������޹�1����(4Bytes)
#define  COMB2_NO_WORK_B_OFFSET                324   //B������޹�2����(4Bytes)
#define  POSITIVE_WORK_C_OFFSET                328   //C�������й�����(4Bytes)
#define  NEGTIVE_WORK_C_OFFSET                 332   //C�෴���й�����(4Bytes)
#define  COMB1_NO_WORK_C_OFFSET                336   //C������޹�1����(4Bytes)
#define  COMB2_NO_WORK_C_OFFSET                340   //C������޹�2����(4Bytes)
//4-1,��ҳ�����ڵ�344�ֽ�***************************


//4-2������¼�洢ƫ��(1����¼�洢1����1�ε�����������ʱ��)
//4-2.1 DL/T645-1997��DL/T645-2007��������3���ֽ�(,�洢��ʽΪxx.xxxx(BCD��),��λ�洢�ڴ洢���ĵ��ֽ�)
//4-2.2 DL/T645-1997����������ʱ��Ϊ4���ֽ�(��ʱ����,BCD��),DL/T645-2007����������ʱ��Ϊ5���ֽ�(��ʱ������,BCD��)
//      Ϊ��ͳһ�洢,�洢Ϊ5���ֽ�,DL/T645-1997��������Ϊû������������ֽڴ洢Ϊ0xee
#define  REQ_POSITIVE_WORK_OFFSET                0   //�����й��������((1+8)*3Bytes)
#define  REQ_TIME_P_WORK_OFFSET                 27   //�����й������������ʱ��((1+8)*5Bytes)
#define  REQ_POSITIVE_NO_WORK_OFFSET            72   //�����޹��������((1+8)*3Bytes)
#define  REQ_TIME_P_NO_WORK_OFFSET              99   //�����޹������������ʱ��((1+8)*5Bytes)
#define  REQ_NEGTIVE_WORK_OFFSET               144   //�����й��������((1+8)*3Bytes)
#define  REQ_TIME_N_WORK_OFFSET                171   //�����й������������ʱ��((1+8)*5Bytes)
#define  REQ_NEGTIVE_NO_WORK_OFFSET            216   //�����޹��������((1+8)*3Bytes)
#define  REQ_TIME_N_NO_WORK_OFFSET             243   //�����޹������������ʱ��((1+8)*5Bytes)
//ly,2010-01-14,ԭ���ǳ���һ���������޵�����������ʱ���,����Щ��֧��,376.1-2009Ҳû���⼸�����ݵ�Ҫ��,
//���ɾ�����¼���.�����Ҫ��һ���������޵Ļ�,�����¼��е�ע�ʹ򿪼���. 
//4-2��ҳ�����ڵ�288�ֽ�***************************
//#define  QUA1_REQ_NO_WORK_OFFSET               296   //һ�����޹��������((1+8)*3Bytes)
//#define  QUA1_REQ_TIME_NO_WORK_OFFSET          323   //һ�����޹������������ʱ��((1+8)*5Bytes)
//#define  QUA2_REQ_NO_WORK_OFFSET               368   //�������޹��������((1+8)*3Bytes)
//#define  QUA2_REQ_TIME_NO_WORK_OFFSET          395   //�������޹������������ʱ��((1+8)*5Bytes)
//#define  QUA3_REQ_NO_WORK_OFFSET               440   //�������޹��������((1+8)*3Bytes)
//#define  QUA3_REQ_TIME_NO_WORK_OFFSET          467   //�������޹������������ʱ��((1+8)*5Bytes)
//#define  QUA4_REQ_NO_WORK_OFFSET               512   //�������޹��������((1+8)*3Bytes)
//#define  QUA4_REQ_TIME_NO_WORK_OFFSET          539   //�������޹������������ʱ��((1+8)*5Bytes)
//4-2x�������һ���������޵��������������ʱ��Ļ�,��ҳ�����ڵ�584�ֽ�***************************


//4-3.�α���
//4-3.1��ѹ��Чֵ2bytes(97[xxx.0],2007[xxx.x])
#define  VOLTAGE_PHASE_A                         0   //A���ѹ(2Bytes)
#define  VOLTAGE_PHASE_B                         2   //B���ѹ(2Bytes)
#define  VOLTAGE_PHASE_C                         4   //C���ѹ(2Bytes)

//4-3.2������Чֵ(DL/T645-1997Ϊ2bytes,DL/T645-2007Ϊ3bytes,�洢Ϊ3���ֽ�[xxx.xxx(97�洢��ĸ�ʽΪ0xx.xx0)])
//97�������������,��07�����ߵ���Ϊ3bytes,�洢Ϊ3bytes,97û����Ϊ0xeeeeee,���ص����ݴ�������/�����־,���üӱ�־��
#define  CURRENT_PHASE_A                         6   //A�����(3Bytes)
#define  CURRENT_PHASE_B                         9   //B�����(3Bytes)
#define  CURRENT_PHASE_C                        12   //C�����(3Bytes)
#define  ZERO_SERIAL_CURRENT                    15   //�������(3Bytes)

//4-3.3˲ʱ�й�����3bytes(xx.xxxx),���ص����ݴ�������/�����־,���üӱ�־��
#define  POWER_INSTANT_WORK                     18   //˲ʱ�й�����(3Bytes)
#define  POWER_PHASE_A_WORK                     21   //A���й�����(3Bytes)
#define  POWER_PHASE_B_WORK                     24   //B���й�����(3Bytes)
#define  POWER_PHASE_C_WORK                     27   //C���й�����(3Bytes)

//4-3.4˲ʱ�޹�����(DL/T645-1997Ϊ2bytes,DL/T645-2007Ϊ3bytes,�洢Ϊ3���ֽ�[xx.xxxx(97�洢��ĸ�ʽΪxx.xx00)])
//���ص����ݴ�������/�����־,���üӱ�־��
#define  POWER_INSTANT_NO_WORK                  30   //˲ʱ�޹�����(3Bytes)
#define  POWER_PHASE_A_NO_WORK                  33   //A���޹�����(3Bytes)
#define  POWER_PHASE_B_NO_WORK                  36   //B���޹�����(3Bytes)
#define  POWER_PHASE_C_NO_WORK                  39   //C���޹�����(3Bytes)

//4-3.5˲ʱ���ڹ���(DL/T645-1997û��,DL/T645-2007Ϊ3bytes,�洢Ϊ3���ֽ�[xx.xxxx(97�洢��ĸ�ʽΪ0xeeeeee)])
#define  POWER_INSTANT_APPARENT	                42   //˲ʱ���ڹ���(3Bytes)
#define  POWER_PHASE_A_APPARENT                 45   //A�����ڹ���(3Bytes)
#define  POWER_PHASE_B_APPARENT                 48   //B�����ڹ���(3Bytes)
#define  POWER_PHASE_C_APPARENT                 51   //C�����ڹ���(3Bytes)

//4-3.5��������2bytes(x.xxx)
#define  TOTAL_POWER_FACTOR                     54   //�ܹ�������(2Bytes)
#define  FACTOR_PHASE_A                         56   //A�๦������(2Bytes)
#define  FACTOR_PHASE_B                         58   //B�๦������(2Bytes)
#define  FACTOR_PHASE_C                         60   //C�๦������(2Bytes)

//4-3.6�������������ʱ��
//DL/T645-1997�������Ϊ2bytes,DL/T645-2007�������Ϊ3bytes,�洢Ϊ3bytes(NNNNNN,BCD��(DL/T645-1997�洢��ĸ�ʽΪ00NNNN))
//DL/T645-1997��DL/T645-2007�Ķ����ۼ�ʱ���Ϊ3bytes,�洢Ϊ3bytes(NNNNNN,BCD��)
#define  PHASE_DOWN_TIMES                       62   //�ܶ������(3Bytes)
#define  TOTAL_PHASE_DOWN_TIME                  65   //�ܶ���ʱ��(3Bytes)
#define  PHASE_A_DOWN_TIMES                     68   //A��������(3Bytes)
#define  TOTAL_PHASE_A_DOWN_TIME                71   //A�����ʱ��(3Bytes)
#define  PHASE_B_DOWN_TIMES                     74   //B��������(3Bytes)
#define  TOTAL_PHASE_B_DOWN_TIME                77   //B�����ʱ��(3Bytes)
#define  PHASE_C_DOWN_TIMES                     80   //C��������(3Bytes)
#define  TOTAL_PHASE_C_DOWN_TIME                83   //C�����ʱ��(3Bytes)

//4-3.7������ʼʱ��ͽ���ʱ��
//DL/T645-1997ʱ��Ϊ4bytes(��ʱ����),DL/T645-2007ʱ��Ϊ6bytes(���ʱ������),�洢Ϊ6bytes(ssmmhhDDMMYY)
#define  LAST_PHASE_DOWN_BEGIN                  86   //���һ�ζ�����ʼʱ��(6Bytes)
#define  LAST_PHASE_DOWN_END                    92   //���һ�ζ������ʱ��(6Bytes)
#define  LAST_PHASE_A_DOWN_BEGIN                98   //���һ��A�������ʼʱ��(6Bytes)
#define  LAST_PHASE_A_DOWN_END                 104   //���һ��A��������ʱ��(6Bytes)
#define  LAST_PHASE_B_DOWN_BEGIN               110   //���һ��B�������ʼʱ��(6Bytes)
#define  LAST_PHASE_B_DOWN_END                 116   //���һ��B��������ʱ��(6Bytes)
#define  LAST_PHASE_C_DOWN_BEGIN               122   //���һ��C�������ʼʱ��(6Bytes)
#define  LAST_PHASE_C_DOWN_END                 128   //���һ��C��������ʱ��(6Bytes)

//4-3.8�������������ʱ��
//4-3.8.1��̴���������ʱ��
#define  PROGRAM_TIMES                         134   //��̴���(3bytes)(97��2Bytes,2007��3bytes,�洢Ϊ3bytes(NNNNNN,BCD��,97�洢��Ϊ00NNNN))
#define  LAST_PROGRAM_TIME                     137   //���һ�α��ʱ��(6bytes)(97��4bytes,2007��6bytes,�洢Ϊ6bytes(ssmmhhDDMMYY,97�洢��ĸ�ʽΪ00mmhhDDMM00))
//4-3.8.2����������������ʱ��
#define  METER_CLEAR_TIMES                     143   //��������(3bytes)(97û��,2007��3bytes,�洢Ϊ3bytes(NNNNNN,BCD��,97�洢��Ϊ0xeeeeee))
#define  LAST_METER_CLEAR_TIME                 146   //����������һ�η���ʱ��(6bytes)(97û��,2007��6bytes,�洢Ϊ6bytes(ssmmhhDDMMYY,97�洢��ĸ�ʽΪ0xeeeeeeeeeeee))
//4-3.8.3���������ܴ��������һ������ʱ��
#define  UPDATA_REQ_TIME                       152   //��������������(3bytes)(97��2Bytes,2007��3bytes,�洢Ϊ3bytes(NNNNNN,BCD��,97�洢��Ϊ00NNNN))
#define  LAST_UPDATA_REQ_TIME                  155   //���һ�������������ʱ��(6bytes)(97��4bytes,2007��6bytes,�洢Ϊ6bytes(ssmmhhDDMMYY,97�洢��ĸ�ʽΪ00mmhhDDMM00))
//4-3.8.4�¼������ܴ��������һ������ʱ��
#define  EVENT_CLEAR_TIMES                     161   //�¼��������(3bytes)(97û��,2007��3bytes,�洢Ϊ3bytes(NNNNNN,BCD��,97�洢��Ϊ0xeeeeee))
#define  EVENT_CLEAR_LAST_TIME                 164   //���һ���¼�����ʱ��(6bytes)(97û��,2007��6bytes,�洢Ϊ6bytes(ssmmhhDDMMYY,97�洢��ĸ�ʽΪ0xeeeeeeeeeeee))
//4-3.8.5Уʱ�ܴ��������һ��Уʱʱ��
#define  TIMING_TIMES                          170   //Уʱ�ܴ���(3bytes)(97û��,2007��3bytes,�洢Ϊ3bytes(NNNNNN,BCD��,97�洢��Ϊ0xeeeeee))
#define  TIMING_LAST_TIME                      173   //���һ��Уʱʱ��(6bytes)(97û��,2007��6bytes,�洢Ϊ6bytes(ssmmhhDDMMYY,97�洢��ĸ�ʽΪ0xeeeeeeeeeeee))

//4-3.9�������״̬��
//DL/T645-1997ֻ��һ���ֽڵĵ������״̬��,�洢��METER_STATUS_WORD
//DL/T645-2007��7��״̬��,ÿ��״̬����2���ֽ�
#define  METER_STATUS_WORD                     179   //�������״̬��1(2Bytes)
#define  METER_STATUS_WORD_2                   181   //�������״̬��2(2Bytes)
#define  METER_STATUS_WORD_3                   183   //�������״̬��3(2Bytes)
#define  METER_STATUS_WORD_4                   185   //�������״̬��4(2Bytes)
#define  METER_STATUS_WORD_5                   187   //�������״̬��5(2Bytes)
#define  METER_STATUS_WORD_6                   189   //�������״̬��6(2Bytes)
#define  METER_STATUS_WORD_7                   191   //�������״̬��7(2Bytes)

//4-3.10����״̬��,97��һ���ֽ�,��07û�е���״̬��
#define  NET_STATUS_WORD                       193   //����״̬��(1Bytes)

//4-3.11�������4bytes��ʱ��3bytes,97��07��ͬ
#define  DATE_AND_WEEK                         194   //���ڼ��ܴ�(4Bytes,WWDDMMYY)
#define  METER_TIME                            198   //���ʱ��(3Bytes,ssmmhh)

//4-3.12��ع���ʱ��,97Ϊ3bytes,07Ϊ4bytes,�洢Ϊ4bytes(NNNNNNNN,97�洢��Ϊ00NNNNNN)
#define  BATTERY_WORK_TIME                     201   //��ع���ʱ��(4Bytes)

//4-3.13�������3bytes�����6bytes,97��07��ͬ
#define  CONSTANT_WORK                         205   //�����(�й�,3�ֽ�)
#define  CONSTANT_NO_WORK                      208   //�����(�޹�,3�ֽ�)
#define  METER_NUMBER                          211   //���(����ַ,6�ֽ�)

//4-3.14�Զ���������
//97Ϊһ���Զ���������2bytes
//2007Ϊ����ÿ�½����ո�2bytes
#define  AUTO_COPY_DAY                         217   //�Զ���������/ÿ�µ�1������(2�ֽ�)
#define  AUTO_COPY_DAY_2                       219   //ÿ�µ�2������(2�ֽ�)
#define  AUTO_COPY_DAY_3                       221   //ÿ�µ�3������(2�ֽ�)

//4-3.15 ʱ�α����ܴ�������һ�η���ʱ��
#define  PERIOD_TIMES                          223   //ʱ�α����ܴ���(3bytes)(97û��,2007��3bytes,�洢Ϊ3bytes(NNNNNN,BCD��,97�洢��Ϊ0xeeeeee))
#define  PERIOD_LAST_TIME                      226   //���һ��ʱ�α��̷���ʱ��(6bytes)(97û��,2007��6bytes,�洢Ϊ6bytes(ssmmhhDDMMYY,97�洢��ĸ�ʽΪ0xeeeeeeeeeeee))

//4-3.16 �ѿر����/����/������Ϣ
#define  LAST_JUMPED_GATE_TIME                 232   //��һ����բ����ʱ��(6�ֽ�,97û��)
#define  LAST_CLOSED_GATE_TIME                 238   //��һ�κ�բ����ʱ��(6�ֽ�,97û��)
#define  OPEN_METER_COVER_TIMES                244   //������ܴ���(2�ֽ�,97û��)
#define  LAST_OPEN_METER_COVER_TIME            246   //��һ�ο���Ƿ���ʱ��(6�ֽ�,97û��)
#define  CHARGE_TOTAL_TIME                     252   //��1�ι�����ܹ������(2�ֽ�)
#define  CHARGE_REMAIN_MONEY                   254   //��ǰʣ����(4�ֽ�,97û��)
#define  CHARGE_TOTAL_MONEY                    258   //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
#define  CHARGE_REMAIN_QUANTITY                262   //��ǰʣ�����(4�ֽ�,97û��)
#define  CHARGE_OVERDRAFT_QUANTITY	           266   //��ǰ͸֧����(4�ֽ�,97û��)
#define  CHARGE_TOTAL_QUANTITY                 270   //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
#define  CHARGE_OVERDRAFT_LIMIT                274   //͸֧������ֵ(4�ֽ�,97û��)
#define  CHARGE_ALARM_QUANTITY                 278   //��������1��ֵ(4�ֽ�,97û��)

//4-3.17 ��ѹ��������λ��
#define PHASE_ANGLE_V_A                        282   //A���ѹ��λ��(2�ֽ�)
#define PHASE_ANGLE_V_B                        284   //B���ѹ��λ��(2�ֽ�)
#define PHASE_ANGLE_V_C                        286   //C���ѹ��λ��(2�ֽ�)
#define PHASE_ANGLE_C_A                        288   //A�������λ��(2�ֽ�)
#define PHASE_ANGLE_C_B                        290   //B�������λ��(2�ֽ�)
#define PHASE_ANGLE_C_C                        292   //C�������λ��(2�ֽ�)


//4-3��ҳ�����ڵ�294�ֽ�***************************


//4-4.ʱ���йزα���ҳ�ڴ洢ƫ��(1ҳ�ڴ洢1����1�ε�ʱ���йزα���)
#define  YEAR_SHIQU_P                            0   //��ʱ����(1�ֽ�)
#define  DAY_SHIDUAN_BIAO_Q                      1   //��ʱ�α���(1�ֽ�)
#define  DAY_SHIDUAN_M                           2   //��ʱ����(1�ֽ�)
#define  NUM_TARIFF_K                            3   //������(1�ֽ�)
#define  NUM_OF_JIA_RI_N                         4   //����������(1�ֽ�)
#define  YEAR_SHIDU                              5   //ʱ����ʼ���ڼ���ʱ�α����ʼ��ַ(Ԥ��15*3�ֽ�)
#define  DAY_SHIDUAN_BIAO                       50   //��ʱ�α���ʼʱ�估���ʺ���ʼ��ַ
                                                     //(Ԥ��13����ʱ��,ÿ����10��ʱ��ʱ��=13*10*3=390�ֽ�)
#define  JIA_RI_SHIDUAN                        440   //�������ڼ�ʱ����ʼ��ַ(Ԥ��15������ʱ��15*3=45�ֽ�)
#define  ZHOUXIURI_SHIDUAN                     485   //�����ղ��õ���ʱ�α��
//4-4��ҳ�����ڵ�487�ֽ�***************************

//4-5.��������������ʾֵ���������״̬�֡��ѿ���ش洢ƫ��*****************************
#define  LENGTH_OF_SINGLE_ENERGY_RECORD         61   //������(�����)��¼����
#define  LENGTH_OF_SINGLE_REMOTE_RECORD         73   //������(����Զ�̷ѿر�)��¼����
#define  LENGTH_OF_SINGLE_LOCAL_RECORD         111   //������(���౾�طѿر�)��¼����

//4-5.1 �������й�����ʾֵ
#define  POSITIVE_WORK_OFFSET_S                  0   //�����й�����ʾֵ((1+4)*4Bytes)
#define  NEGTIVE_WORK_OFFSET_S                  20   //�����й�����ʾֵ((1+4)*4Bytes)
#define  DAY_FREEZE_TIME_FLAG_S               1024   //�ն���ʱ��(5Bytes)

//4-5.2 �������״̬��
#define  METER_STATUS_WORD_S                    40   //�������״̬��1(2Bytes)
#define  METER_STATUS_WORD_S_2                  42   //�������״̬��2(2Bytes)
#define  METER_STATUS_WORD_S_3                  44   //�������״̬��3(2Bytes)
#define  METER_STATUS_WORD_S_4                  46   //�������״̬��4(2Bytes)
#define  METER_STATUS_WORD_S_5                  48   //�������״̬��5(2Bytes)
#define  METER_STATUS_WORD_S_6                  50   //�������״̬��6(2Bytes)
#define  METER_STATUS_WORD_S_7                  52   //�������״̬��7(2Bytes)

#define  DATE_AND_WEEK_S                        54   //���ܱ����ڼ��ܴ�(4Bytes)
#define  METER_TIME_S                           58   //���ܱ�ʱ��(3Bytes)
//���������ڵ�61�ֽ�*********************************************************

//4-5.3 �ѿر����/����/������Ϣ
#define  LAST_JUMPED_GATE_TIME_S                61   //��һ����բ����ʱ��(6�ֽ�)
#define  LAST_CLOSED_GATE_TIME_S                67   //��һ�κ�բ����ʱ��(6�ֽ�)
//����Զ�̷ѿر������73�ֽ�***************************************************

//4-5.4 ���ܱ����õ���Ϣ
#define  OPEN_METER_COVER_TIMES_S               73   //������ܴ���(2�ֽ�,97û��)
#define  LAST_OPEN_METER_COVER_TIME_S           75   //��һ�ο���Ƿ���ʱ��(6�ֽ�,97û��)
#define  CHARGE_TOTAL_TIME_S                    81   //��1�ι�����ܹ������(2�ֽ�)
#define  CHARGE_REMAIN_MONEY_S                  83   //��ǰʣ����(4�ֽ�,97û��)
#define  CHARGE_TOTAL_MONEY_S                   87   //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
#define  CHARGE_REMAIN_QUANTITY_S               91   //��ǰʣ�����(4�ֽ�,97û��)
#define  CHARGE_OVERDRAFT_QUANTITY_S	          95   //��ǰ͸֧����(4�ֽ�,97û��)
#define  CHARGE_TOTAL_QUANTITY_S                99   //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
#define  CHARGE_OVERDRAFT_LIMIT_S              103   //͸֧������ֵ(4�ֽ�,97û��)
#define  CHARGE_ALARM_QUANTITY_S               107   //��������1��ֵ(4�ֽ�,97û��)
//���౾�طѿر������111�ֽ�**************************************************

//4-6.����ѿر���״̬�֡��ѿ���ش洢ƫ��*****************************
#define  LENGTH_OF_THREE_ENERGY_RECORD         309   //������(�������ܱ�)��¼����
#define  LENGTH_OF_THREE_REMOTE_RECORD         321   //������(����Զ�̷ѿر�)��¼����
#define  LENGTH_OF_THREE_LOCAL_RECORD          359   //������(���౾�طѿر�)��¼����

#define  DAY_FREEZE_TIME_FLAG_T               1530   //�ն���ʱ��(5Bytes)

//4-6.1 �������״̬��(�ӵ�����������)
#define  METER_STATUS_WORD_T                   288   //�������״̬��1(2Bytes)
#define  METER_STATUS_WORD_T_2                 290   //�������״̬��2(2Bytes)
#define  METER_STATUS_WORD_T_3                 292   //�������״̬��3(2Bytes)
#define  METER_STATUS_WORD_T_4                 294   //�������״̬��4(2Bytes)
#define  METER_STATUS_WORD_T_5                 295   //�������״̬��5(2Bytes)
#define  METER_STATUS_WORD_T_6                 306   //�������״̬��6(2Bytes)
#define  METER_STATUS_WORD_T_7                 300   //�������״̬��7(2Bytes)

//4-6.2 ���ܱ����ڡ��ܴμ�ʱ��
#define  DATE_AND_WEEK_T                       302   //���ܱ����ڼ��ܴ�(4Bytes)
#define  METER_TIME_T                          306   //���ܱ�ʱ��(3Bytes)

//�������ܱ��������¼�����ڵ�309�ֽ�*********************************************************

//4-6.3 �ѿر����/����/������Ϣ
#define  LAST_JUMPED_GATE_TIME_T               309   //��һ����բ����ʱ��(6�ֽ�)
#define  LAST_CLOSED_GATE_TIME_T               315   //��һ�κ�բ����ʱ��(6�ֽ�)
//����Զ�̷ѿر��������¼�����ڵ�321�ֽ�*********************************************************

//4-6.4 ���ܱ����õ���Ϣ
#define  OPEN_METER_COVER_TIMES_T              321   //������ܴ���(2�ֽ�,97û��)
#define  LAST_OPEN_METER_COVER_TIME_T          323   //��һ�ο���Ƿ���ʱ��(6�ֽ�,97û��)
#define  CHARGE_TOTAL_TIME_T                   329   //��1�ι�����ܹ������(2�ֽ�)
#define  CHARGE_REMAIN_MONEY_T                 331   //��ǰʣ����(4�ֽ�,97û��)
#define  CHARGE_TOTAL_MONEY_T                  335   //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
#define  CHARGE_REMAIN_QUANTITY_T              339   //��ǰʣ�����(4�ֽ�,97û��)
#define  CHARGE_OVERDRAFT_QUANTITY_T	         343   //��ǰ͸֧����(4�ֽ�,97û��)
#define  CHARGE_TOTAL_QUANTITY_T               347   //��һ�ι�����ۼƹ�����(4�ֽ�,97û��)
#define  CHARGE_OVERDRAFT_LIMIT_T              351   //͸֧������ֵ(4�ֽ�,97û��)
#define  CHARGE_ALARM_QUANTITY_T               355   //��������1��ֵ(4�ֽ�,97û��)
//���౾�طѿر������359�ֽ�**************************************************

//4-7 ���㶳������
#define  LENGTH_OF_HOUR_FREEZE_RECORD            8   //���㶳���¼����

#define  HOUR_FREEZE_P_WORK                    0x0   //���㶳�������й��ܵ���(4�ֽ�)
#define  HOUR_FREEZE_N_WORK                    0x4   //���㶳�������й��ܵ���(4�ֽ�)
//���㶳�����ݱ������8�ֽ�

//5.��������ҳ�ڴ洢ƫ��(1,2)
//5-1.�յ�ǰ������,�µ�ǰ��������¼
//�����ݴ洢��ʽ7bytes,��0�ֽ�Ϊ���ݷ���,��1,2�ֽ�ΪС��BCD����,��3,4,5,6�ֽ�Ϊ����BCD��
//�洢��ĸ�ʽ1,2,3,4�ֽڷ��Ź�Լ�����ݸ�ʽ13xxxx.xxxx
#define  LEN_OF_ENERGY_BALANCE_RECORD          504   //�����������¼����

#define  DAY_P_WORK_OFFSET                       0   //�յ�ǰ�����й�������((1+8)*7Bytes)
#define  DAY_N_WORK_OFFSET                      63   //�յ�ǰ�����й�������((1+8)*7Bytes)
#define  DAY_P_NO_WORK_OFFSET                  126   //�յ�ǰ�����޹�������((1+8)*7Bytes)
#define  DAY_N_NO_WORK_OFFSET                  189   //�յ�ǰ�����޹�������((1+8)*7Bytes)
#define  MONTH_P_WORK_OFFSET                   252   //�µ�ǰ�����й�������((1+8)*7Bytes)
#define  MONTH_N_WORK_OFFSET                   315   //�µ�ǰ�����й�������((1+8)*7Bytes)
#define  MONTH_P_NO_WORK_OFFSET                378   //�µ�ǰ�����޹�������((1+8)*7Bytes)
#define  MONTH_N_NO_WORK_OFFSET                441   //�µ�ǰ�����޹�������((1+8)*7Bytes)
//5-1��ҳ�����ڵ�504�ֽ�***************************

//5-2.�α���ͳ��ֵ
#define  LEN_OF_PARA_BALANCE_RECORD            246   //�α��������¼����

//����ͳ��ƫ��
#define  MAX_TOTAL_POWER                       0                            //�й��������ֵ(3Bytes)
#define  MAX_TOTAL_POWER_TIME                  (MAX_TOTAL_POWER+3)          //�й��������ֵ����ʱ��(3Bytes)
#define  TOTAL_ZERO_POWER_TIME                 (MAX_TOTAL_POWER_TIME+3)     //�й�����Ϊ��ʱ��(2Bytes)

#define  MAX_A_POWER                           (TOTAL_ZERO_POWER_TIME+2)    //A������й�����(3Bytes)
#define  MAX_A_POWER_TIME                      (MAX_A_POWER+3)              //A������й����ʷ���ʱ��(3Bytes)
#define  A_ZERO_POWER_TIME                     (MAX_A_POWER_TIME+3)         //A���й�����Ϊ��ʱ��(2Bytes)

#define  MAX_B_POWER                           (A_ZERO_POWER_TIME+2)        //B������й�����(3Bytes)
#define  MAX_B_POWER_TIME                      (MAX_B_POWER+3)              //B������й����ʷ���ʱ��(3Bytes)
#define  B_ZERO_POWER_TIME                     (MAX_B_POWER_TIME+3)         //B���й�����Ϊ��ʱ��(2Bytes)

#define  MAX_C_POWER                           (B_ZERO_POWER_TIME+2)        //C������й�����(3Bytes)
#define  MAX_C_POWER_TIME                      (MAX_C_POWER+3)              //C������й����ʷ���ʱ��(3Bytes)
#define  C_ZERO_POWER_TIME                     (MAX_C_POWER_TIME+3)         //C���й�����Ϊ��ʱ��(2Bytes)

//����ͳ��ƫ��
#define  MAX_TOTAL_REQ                         (C_ZERO_POWER_TIME+2)        //�������й��������(3Bytes)
#define  MAX_TOTAL_REQ_TIME                    (MAX_TOTAL_REQ+3)            //�������й������������ʱ��(3Bytes)
#define  MAX_A_REQ                             (MAX_TOTAL_REQ_TIME+3)       //A���й��������(3Bytes)
#define  MAX_A_REQ_TIME                        (MAX_A_REQ+3)                //A���й������������ʱ��(3Bytes)
#define  MAX_B_REQ                             (MAX_A_REQ_TIME+3)           //B���й��������(3Bytes)
#define  MAX_B_REQ_TIME                        (MAX_B_REQ+3)                //B���й������������ʱ��(3Bytes)
#define  MAX_C_REQ                             (MAX_B_REQ_TIME+3)           //C���й��������(3Bytes)
#define  MAX_C_REQ_TIME                        (MAX_C_REQ+3)                //C���й������������ʱ��(3Bytes)

//��ѹͳ��ֵƫ��
#define  VOL_A_UP_UP_TIME                      (MAX_C_REQ_TIME+3)           //A���ѹԽ������ʱ��(2Bytes)
#define  VOL_A_DOWN_DOWN_TIME                  (VOL_A_UP_UP_TIME+2)         //A���ѹԽ������ʱ��(2Bytes)
#define  VOL_A_UP_TIME                         (VOL_A_DOWN_DOWN_TIME+2)     //A���ѹԽ����ʱ��(2Bytes)
#define  VOL_A_DOWN_TIME                       (VOL_A_UP_TIME+2)            //A���ѹԽ����ʱ��(2Bytes)
#define  VOL_A_OK_TIME                         (VOL_A_DOWN_TIME+2)          //A���ѹ�ϸ�ʱ��(2Bytes)

#define  VOL_B_UP_UP_TIME                      (VOL_A_OK_TIME+2)            //B���ѹԽ������ʱ��(2Bytes)
#define  VOL_B_DOWN_DOWN_TIME                  (VOL_B_UP_UP_TIME+2)         //B���ѹԽ������ʱ��(2Bytes)
#define  VOL_B_UP_TIME                         (VOL_B_DOWN_DOWN_TIME+2)     //B���ѹԽ����ʱ��(2Bytes)
#define  VOL_B_DOWN_TIME                       (VOL_B_UP_TIME+2)            //B���ѹԽ����ʱ��(2Bytes)
#define  VOL_B_OK_TIME                         (VOL_B_DOWN_TIME+2)          //B���ѹ�ϸ�ʱ��(2Bytes)

#define  VOL_C_UP_UP_TIME                      (VOL_B_OK_TIME+2)            //C���ѹԽ������ʱ��(2Bytes)
#define  VOL_C_DOWN_DOWN_TIME                  (VOL_C_UP_UP_TIME+2)         //C���ѹԽ������ʱ��(2Bytes)
#define  VOL_C_UP_TIME                         (VOL_C_DOWN_DOWN_TIME+2)     //C���ѹԽ����ʱ��(2Bytes)
#define  VOL_C_DOWN_TIME                       (VOL_C_UP_TIME+2)            //C���ѹԽ����ʱ��(2Bytes)
#define  VOL_C_OK_TIME                         (VOL_C_DOWN_TIME+2)          //C���ѹ�ϸ�ʱ��(2Bytes)

#define  VOL_A_MAX                             (VOL_C_OK_TIME+2)            //A���ѹ���ֵ(2Bytes)
#define  VOL_A_MAX_TIME                        (VOL_A_MAX+2)                //A���ѹ���ֵ����ʱ��(3Bytes)
#define  VOL_A_MIN                             (VOL_A_MAX_TIME+3)           //A���ѹ��Сֵ(2Bytes)
#define  VOL_A_MIN_TIME                        (VOL_A_MIN+2)                //A���ѹ��Сֵ����ʱ��(3Bytes)
#define  VOL_A_AVER                            (VOL_A_MIN_TIME+3)           //A���ѹƽ��ֵ(2Bytes)
#define  VOL_A_AVER_COUNTER                    (VOL_A_AVER+2)               //A���ѹƽ��ֵ����(2Bytes)

#define  VOL_B_MAX                             (VOL_A_AVER_COUNTER+2)       //B���ѹ���ֵ(2Bytes)
#define  VOL_B_MAX_TIME                        (VOL_B_MAX+2)                //B���ѹ���ֵ����ʱ��(3Bytes)
#define  VOL_B_MIN                             (VOL_B_MAX_TIME+3)           //B���ѹ��Сֵ(2Bytes)
#define  VOL_B_MIN_TIME                        (VOL_B_MIN+2)                //B���ѹ��Сֵ����ʱ��(3Bytes)
#define  VOL_B_AVER                            (VOL_B_MIN_TIME+3)           //B���ѹƽ��ֵ(2Bytes)
#define  VOL_B_AVER_COUNTER                    (VOL_B_AVER+2)               //B���ѹƽ��ֵ����(2Bytes)

#define  VOL_C_MAX                             (VOL_B_AVER_COUNTER+2)       //C���ѹ���ֵ(2Bytes)
#define  VOL_C_MAX_TIME                        (VOL_C_MAX+2)                //C���ѹ���ֵ����ʱ��(3Bytes)
#define  VOL_C_MIN                             (VOL_C_MAX_TIME+3)           //C���ѹ��Сֵ(2Bytes)
#define  VOL_C_MIN_TIME                        (VOL_C_MIN+2)                //C���ѹ��Сֵ����ʱ��(3Bytes)
#define  VOL_C_AVER                            (VOL_C_MIN_TIME+3)           //C���ѹƽ��ֵ(2Bytes)
#define  VOL_C_AVER_COUNTER                    (VOL_C_AVER+2)               //C���ѹƽ��ֵ����(2Bytes)

//��ƽ���Խ���ۼ�ʱ��
#define  VOL_UNBALANCE_TIME                    (VOL_C_AVER_COUNTER+2)       //��ѹ��ƽ���Խ���ۼ�ʱ��(2Bytes)
#define  CUR_UNBALANCE_TIME                    (VOL_UNBALANCE_TIME+2)       //������ƽ���Խ���ۼ�ʱ��(2Bytes)
#define  VOL_UNB_MAX                           (CUR_UNBALANCE_TIME+2)       //��ѹ��ƽ������ֵ(2Bytes)
#define  VOL_UNB_MAX_TIME                      (VOL_UNB_MAX+2)              //��ѹ��ƽ������ֵ����ʱ��(3Bytes)
#define  CUR_UNB_MAX                           (VOL_UNB_MAX_TIME+3)         //������ƽ������ֵ(2Bytes)
#define  CUR_UNB_MAX_TIME                      (CUR_UNB_MAX+2)              //������ƽ������ֵ����ʱ��(3Bytes)

//����ͳ��
#define  CUR_A_UP_UP_TIME                      (CUR_UNB_MAX_TIME+3)         //A�����Խ�������ۼ�ʱ��(2Bytes)
#define  CUR_A_UP_TIME                         (CUR_A_UP_UP_TIME+2)         //A�����Խ�����ۼ�ʱ��(2Bytes)
#define  CUR_A_MAX                             (CUR_A_UP_TIME+2)            //A��������ֵ(3Bytes)
#define  CUR_A_MAX_TIME                        (CUR_A_MAX+3)                //A��������ֵ����ʱ��(3Bytes)

#define  CUR_B_UP_UP_TIME                      (CUR_A_MAX_TIME+3)           //B�����Խ������ʱ��(2Bytes)
#define  CUR_B_UP_TIME                         (CUR_B_UP_UP_TIME+2)         //B�����Խ����ʱ��(2Bytes)
#define  CUR_B_MAX                             (CUR_B_UP_TIME+2)            //B��������ֵ(3Bytes)
#define  CUR_B_MAX_TIME                        (CUR_B_MAX+3)                //B��������ֵ����ʱ��(3Bytes)

#define  CUR_C_UP_UP_TIME                      (CUR_B_MAX_TIME+3)           //C�����Խ�������ۼ�ʱ��(2Bytes)
#define  CUR_C_UP_TIME                         (CUR_C_UP_UP_TIME+2)         //C�����Խ�����ۼ�ʱ��(2Bytes)
#define  CUR_C_MAX                             (CUR_C_UP_TIME+2)            //C��������ֵ(3Bytes)
#define  CUR_C_MAX_TIME                        (CUR_C_MAX+3)                //C��������ֵ����ʱ��(3Bytes)

#define  CUR_ZERO_UP_UP_TIME                   (CUR_C_MAX_TIME+3)           //�������Խ�������ۼ�ʱ��(2Bytes)
#define  CUR_ZERO_UP_TIME                      (CUR_ZERO_UP_UP_TIME+2)      //�������Խ�����ۼ�ʱ��(3Bytes)
#define  CUR_ZERO_MAX                          (CUR_ZERO_UP_TIME+3)         //����������ֵ(3Bytes)
#define  CUR_ZERO_MAX_TIME                     (CUR_ZERO_MAX+3)             //����������ֵ����ʱ��(3Bytes)

//���ڹ���Խ���ۼ�
#define  APPARENT_POWER_UP_UP_TIME             (CUR_ZERO_MAX_TIME+3)        //���ڹ���Խ�������ۼ�ʱ��(2Bytes)
#define  APPARENT_POWER_UP_TIME                (APPARENT_POWER_UP_UP_TIME+2)//���ڹ���Խ�����ۼ�ʱ��(2Bytes)

//����ͳ��
#define  OPEN_PHASE_TIMES                      (APPARENT_POWER_UP_TIME+2)   //�ܶ������(2Bytes)
#define  OPEN_PHASE_MINUTES                    (OPEN_PHASE_TIMES+2)         //�ܶ����ۼ�ʱ��(3Bytes)
#define  OPEN_PHASE_LAST_BEG                   (OPEN_PHASE_MINUTES+3)       //���һ�ζ�����ʼʱ��(4Bytes)
#define  OPEN_PHASE_LASE_END                   (OPEN_PHASE_LAST_BEG+4)      //���һ�ζ������ʱ��(4Bytes)
#define  A_OPEN_PHASE_TIMES                    (OPEN_PHASE_LASE_END+4)      //A��������(2Bytes)
#define  A_OPEN_PHASE_MINUTES                  (A_OPEN_PHASE_TIMES+2)       //A�����ʱ���ۼ�(3Bytes)
#define  A_OPEN_PHASE_LAST_BEG                 (A_OPEN_PHASE_MINUTES+3)     //A�����������ʼʱ��(4Bytes)
#define  A_OPEN_PHASE_LAST_END                 (A_OPEN_PHASE_LAST_BEG+4)    //A������Ͻ���ʱ��(4Bytes)
#define  B_OPEN_PHASE_TIMES                    (A_OPEN_PHASE_LAST_END+4)    //B��������(2Bytes)
#define  B_OPEN_PHASE_MINUTES                  (B_OPEN_PHASE_TIMES+2)       //B������ۼ�ʱ��(3Bytes)
#define  B_OPEN_PHASE_LAST_BEG                 (B_OPEN_PHASE_MINUTES+3)     //B�����������ʼʱ��(4Bytes)
#define  B_OPEN_PHASE_LAST_END                 (B_OPEN_PHASE_LAST_BEG+4)    //B������������ʱ��(4Bytes)
#define  C_OPEN_PHASE_TIMES                    (B_OPEN_PHASE_LAST_END+4)    //C��������(2Bytes)
#define  C_OPEN_PHASE_MINUTES                  (C_OPEN_PHASE_TIMES+2)       //C������ۼ�ʱ��(3Bytes)
#define  C_OPEN_PHASE_LAST_BEG                 (C_OPEN_PHASE_MINUTES+3)     //C��������࿪ʼʱ��(4Bytes)
#define  C_OPEN_PHASE_LAST_END                 (C_OPEN_PHASE_LAST_BEG+4)    //C������������ʱ��(4Bytes)

//�������������ۼ�ʱ��,ly,2011-06-25,add,�������Ե�һ�׶κ����
#define  FACTOR_SEG_1                          (C_OPEN_PHASE_LAST_END+4)    //������������1�ۼ�ʱ��(2Bytes)
#define  FACTOR_SEG_2                          (FACTOR_SEG_1+2)             //������������2�ۼ�ʱ��(2Bytes)
#define  FACTOR_SEG_3                          (FACTOR_SEG_2+2)             //������������3�ۼ�ʱ��(2Bytes)

#define  NEXT_NEW_INSTANCE                     (FACTOR_SEG_3+2)             //�´�ʵʱ�������¿�ʼͳ�Ʋα�����־(1Bytes)
//5-2��ҳ�����ڵ�246�ֽ�***************************

//5-3.�ܼ���ʵʱͳ����ҳ
#define  LEN_OF_ZJZ_BALANCE_RECORD             291   //�ܼ�������¼����

//5-3-1.�ն���
#define  GP_DAY_OVER                           0                            //�ܼ���ͳ�ƽ�����־(1Byte)
#define  GP_DAY_WORK                           (GP_DAY_OVER+1)              //�ܼ��й���������(/�ܼӵ�����)((1+8)*7Bytes)
#define  GP_DAY_NO_WORK                        (GP_DAY_WORK+63)             //�ܼ��޹���������(/�ܼӵ�����)((1+8)*7Bytes)
#define  GP_WORK_POWER                         (GP_DAY_NO_WORK+63)          //�ܼ��й�����(3Bytes)
#define  GP_NO_WORK_POWER                      (GP_WORK_POWER+3)            //�ܼ��޹�����(3Bytes)
#define  GP_DAY_MAX_POWER                      (GP_NO_WORK_POWER+3)         //���ܼ�����й�����(3Bytes)
#define  GP_DAY_MAX_POWER_TIME                 (GP_DAY_MAX_POWER+3)         //���ܼ�����й����ʷ���ʱ��(3Bytes)
#define  GP_DAY_MIN_POWER                      (GP_DAY_MAX_POWER_TIME+3)    //���ܼ���С�й�����(3Bytes)
#define  GP_DAY_MIN_POWER_TIME                 (GP_DAY_MIN_POWER+3)         //���ܼ���С�й����ʷ���ʱ��(3Bytes)
#define  GP_DAY_ZERO_POWER_TIME                (GP_DAY_MIN_POWER_TIME+3)    //���ܼ��й�����Ϊ��ʱ��(2Bytes)

//5-3-2.�¶���
#define  GP_MONTH_OVER                         (GP_DAY_ZERO_POWER_TIME+2)   //�ܼ���ͳ�ƽ�����־(1Bytes)
#define  GP_MONTH_WORK                         (GP_MONTH_OVER+1)            //���ܼ��й�������((1+8)*7Bytes)
#define  GP_MONTH_NO_WORK                      (GP_MONTH_WORK+63)           //���ܼ��޹�������((1+8)*7Bytes)
#define  GP_MONTH_MAX_POWER                    (GP_MONTH_NO_WORK+63)        //���ܼ�����й�����(3Bytes)
#define  GP_MONTH_MAX_POWER_TIME               (GP_MONTH_MAX_POWER+3)       //���ܼ�����й����ʷ���ʱ��(3Bytes)
#define  GP_MONTH_MIN_POWER                    (GP_MONTH_MAX_POWER_TIME+3)  //���ܼ���С�й�����(3Bytes)
#define  GP_MONTH_MIN_POWER_TIME               (GP_MONTH_MIN_POWER+3)       //���ܼ���С�й����ʷ���ʱ��(3Bytes)
#define  GP_MONTH_ZERO_POWER_TIME              (GP_MONTH_MIN_POWER_TIME+3)  //���ܼ��й�����Ϊ��ʱ��(2Bytes)
//5-3��ҳ�����ڵ�291�ֽ�***************************

//6.�м������ܵĵ��ƿ�����/��·������/�����������������ݳ���
#define  LEN_OF_LIGHTING_FREEZE                30                           //����ϵͳ�������������ݼ�¼����


//7.����ͳ����

//�ṹ - �ն�ͳ�Ƽ�¼
typedef struct
{
   INT16U powerOnMinute;        //����ʱ��
   INT16U resetTimes;           //��λ����
   INT16U closeLinkTimes;       //�����ر���·����
   INT16U linkResetTimes;       //�������ر���·����
   INT16U sysTimes;             //�������Ӵ���
   INT16U sysSuccessTimes;      //���ӳɹ�����
   INT16U heartTimes;           //�������Ӵ���
   INT16U heartSuccessTimes;    //�������ӳɹ�����
   INT16U receiveFrames;        //����֡��
   INT32U receiveBytes;         //�����ֽ�
   INT16U sendFrames;           //����֡��
   INT32U sendBytes;            //�����ֽ�
   INT8U  minSignal;            //�ź���Сֵ
   INT8U  minSignalTime[3];     //�ź���Сֵ����ʱ��
   INT8U  maxSignal;            //�ź����ֵ
   INT8U  maxSignalTime[3];     //�ź����ֵ����ʱ��
   INT8U  totalCopyTimes;       //�ܳ������
   INT8U  totalLocalCopyTimes;  //����ͨ�Ŷ˿�(�ز�/����)�����ܴ���
   INT8U  overFlow;             //ͨ������������?
   
  #ifdef LOAD_CTRL_MODULE 
   INT16U monthCtrlJumpedDay;   //�µ����բ���ۼƴ���
   INT16U chargeCtrlJumpedDay;  //�������բ���ۼƴ���
   INT16U powerCtrlJumpedDay;   //������բ���ۼƴ���
   INT16U remoteCtrlJumpedDay;  //ң����բ���ۼƴ���
  #endif
  
  #ifdef PLUG_IN_CARRIER_MODULE
   INT16U dcOverUp;             //ֱ��ģ����Խ�����ۼ�ʱ��
   INT16U dcOverDown;           //ֱ��ģ����Խ�����ۼ�ʱ��
   INT8U  dcMax[2];             //ֱ��ģ�������ֵ
   INT8U  dcMaxTime[3];         //ֱ��ģ�������ֵ����ʱ��
   INT8U  dcMin[2];             //ֱ��ģ������Сֵ
   INT8U  dcMinTime[3];         //ֱ��ģ������Сֵ����ʱ��
  #endif
  
  #ifdef LIGHTING
   INT8U  mixed;                //����
	                              //  bit0-������485��ͨ�Ź���
  #endif
    
}TERMINAL_STATIS_RECORD;

//�ṹ - ���ͳ�Ƽ�¼(��ʱ���޹���)
typedef struct
{ 
	INT8U      emptyByte1;        //ly,2010-07-21,���ÿ�½����̨����ʱ����,���ṹ�ĵ�һ�ֽ�����Ҫ���ĳ�0,
	                              //   ���Կ���һ���ֽ�

	INT8U      meterStop[10];     //���ͣ���¼�(��0�ֽ�-ͣ���Ƿ���,1-4bytesͣ��ʱ�������й��ܵ���ʾֵ,5-9bytesͣ�߷�����ʱ��)

	INT8U      flyFlag;           //���߱�־
	INT8U      flyVision[5];      //����ǰ�����й��ܵ���ʾֵ
	INT8U      reverseFlag;       //ʾ���½���־
	INT8U      reverseVision[5];  //�½�ǰ�����й��ܵ���ʾֵ

	INT8U      vUnBalance;        //��ѹ��ƽ��״̬
	DATE_TIME  vUnBalanceTime;    //��ѹ��ƽ�����ʱ��

	INT8U      cUnBalance;        //������ƽ��״̬
	DATE_TIME  cUnBalanceTime;    //������ƽ�����ʱ��
	
	INT8U      vOverLimit;        //��ѹԽ��
	DATE_TIME  vUpUpTime[3];      //��ѹԽ�����޳���ʱ��
	DATE_TIME  vDownDownTime[3];  //��ѹԽ�����޳���ʱ��
	
	INT8U      cOverLimit;        //����Խ��
	DATE_TIME  cUpTime[3];        //��ѹԽ���޳���ʱ��
	DATE_TIME  cUpUpTime[3];      //��ѹԽ�����޳���ʱ��
	
  INT8U      phaseBreak;        //�����¼�
  INT8U      loseVoltage;       //ʧѹ�¼�
  
  INT8U      apparentPower;     //���ڹ���Խ��״̬
  
	DATE_TIME  apparentUpTime;    //���ڹ���Խ���޳���ʱ��
	DATE_TIME  apparentUpUpTime;  //���ڹ���Խ�����޳���ʱ��
	
  
	INT8U      mixed;             //����
	                              //  bit0-���ܱ�ʱ�䳬���־
	                              //  bit1-485����ʧ�ܱ�־
	                              //  bit2-�����쳣��־
	                              //  bit3-���Ƿѹ,2013-11-21,add
	
 	INT8U      currentLoop;       //������·�쳣��־(bit0-��ʾA���·�����־,bit1-��ʾB���·�����־,bit2-��ʾC���·�����־,[1��ʾ��������,0��ʾδ��������])

	INT8U      overFlag;          //�����������־,2013-11-21,add
	INT8U      overVision[5];     //����������ǰ�����й��ܵ���ʾֵ,2013-11-21,add
	
}METER_STATIS_EXTRAN_TIME;

#ifdef LIGHTING

//�ṹ - ������ͳ�Ƽ�¼(��ʱ���޹���)
typedef struct
{
	INT8U      vOverLimit;        //��ѹԽ��
	DATE_TIME  vUpUpTime[3];      //��ѹԽ�����޳���ʱ��
	DATE_TIME  vDownDownTime[3];  //��ѹԽ�����޳���ʱ��
	
	INT8U      cOverLimit;        //����Խ��
	DATE_TIME  cUpUpTime[3];      //��ѹԽ�����޳���ʱ��
	DATE_TIME  cDownDownTime[3];  //��ѹԽ�����޳���ʱ��
  
  INT8U      powerLimit;        //����Խ��״̬
	DATE_TIME  powerUpTime;       //����Խ���޳���ʱ��
	DATE_TIME  powerDownTime;     //����Խ���޳���ʱ��
	
  INT8U      factorLimit;       //��������Խ��״̬
	DATE_TIME  factorDownTime;    //��������Խ���޳���ʱ��

}KZQ_STATIS_EXTRAN_TIME;

#endif

//�ṹ - ���ͳ�Ƽ�¼(��ʱ���й���)
typedef struct
{
  INT16U copySuccessTimes;      //����ɹ�
	INT8U  lastCopySuccessTime[2];//���һ�γ���ɹ�ʱ��(��ʱ)
	
	INT8U  mixed;                 //����(bit0-���ܱ�ʱ�䳬����Ƿ��¼��־)
}METER_STATIS_BEARON_TIME;

//�ṹ - ������ͳ�Ƽ�¼(��ʱ���޹���)
typedef struct
{
	INT8U     emptyByte1;         //ly,2010-07-21,���ÿ�½����̨����ʱ����,���ṹ�ĵ�һ�ֽ�����Ҫ���ĳ�0,
	                              //   ���Կ���һ���ֽ�
  
	INT8U     mixed;              //����
	                              //    bit0-���ܱ�ʱ�䳬���־
 
 #ifdef LIGHTING
	                              //mixed 
	                              //    bit1-�������Ƶ���·�쳣��־(ֱ������)
	                              //    bit2-�������Ƶ���·�쳣��־(��������)
	                              //    bit3-���ƿ��Ƶ����߱�־
																//    bit4-���ƿ��Ƶ�©��������,2016-10-12,Add
																//    bit5-����ʱ���Ƿ�ͬ��,2016-10-14,Add

	DATE_TIME lastFailure;        //��һ�η����ƹ���ʱ��
	
	DATE_TIME lastDip;            //��һ�η�����бʱ��,2016-10-11,Add
	
 #endif
	
}METER_STATIS_EXTRAN_TIME_S;

//�ṹ - �����ʾֵ/���������ļ�
typedef struct
{
	INT16U pn;
	INT8U  type;    //0x0b-�ն���ʾֵ,0x0c-�ն�������
	INT32U copyTime;
	INT8U  data[288];
}MP_F_DAY;

//�ṹ - ��������ļ�
typedef struct
{
	INT16U pn;
	INT32U copyTime;
	INT8U  data[40];
}SP_F_DAY;

#define  VOLTAGE_UNBALANCE                   0x03
#define  VOLTAGE_UNBALANCE_NOT               0x0C
#define  CURRENT_UNBALANCE                   0x30
#define  CURRENT_UNBALANCE_NOT               0xC0

#endif   /*__INCworkWithMeterh*/
