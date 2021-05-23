#ifndef _EC20_H_
#define _EC20_H_

#include "global.h"

//������Ϣ
typedef struct EC20{
	char rssi[3];	//MAX == 199,dBm
	char ber[2];	//MAX == 99,�ŵ�������
	char iccid[30]; //SIM��iccid

	char code[15];	//"READY",����ҪPIN�룻����...
	char n;			//״̬���Ƿ�ע��
	char stat;		//���ε���Ϣ
	char lac[5];	//������
	char ci[5];		//cell id
	char act;		//������ʽ

	char contextID[2];	//1~16
	char context_type;	//Э�����ͣ�1->IPV4
	char apn[15];		//apn
	char username[15];	//����
	char password[15];	//����
	char authentication;//��֤ 0~3
	char local_address[20];//����ip��ַ
	char connectID[2];	//socket�������� 0~11
	
	char context_state;	//�����״̬
	char ip_adress[20];	//�����ip��ַ
	char service_type[10];	//����ʽ��TCP��UDP��TCP LISTENER,TCP INCOMING,UDP SERVICE
	char remote_port[5];	//�˿ں�
	char local_port[15];	//�����˿ڣ�
	char socket_state;		//socket״̬��0~4
	char serverID;			//
	char access_mode;		//����ģʽ��0�����壬1�����ͣ�2��͸��
	char AT_port[10];		//atָ��˿�
	
	char err[10];			//������
	
	char fsm;//״̬��
	u32 reboot_timeout;//������ʱ
	u32	reboot_count;  //�������� 
	u8 Operater;			//��Ӫ�̣�1���ƶ���2��ͨ��3����
}EC20;
	




void ec20_recv_task(void);
void ec20_config(void);
uint8_t send_commucation_head(u16 size);
u8 sended_result(void);
u8 query_sended_result(u16 len);

#endif
