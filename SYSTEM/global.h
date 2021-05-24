#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "sys.h"
#include "main.h"

#define VERSION			"20181011-1"

#define MAX_BUF_NUM		8		//���ջ�����������ǰ��20�����ջ��壬ec20����10����stm32���ջ���10����
#define MAX_BUF_MEMORY	4096	//�����stm32��ec20�Ľ��ջ����ڴ���
#define MAX_BUF_SIZE	1200	//������ݳ���
#define MIN_BUF_SIZE	64		//��С���ݳ���
#define MIN_IDLE_TIME	10000	//��С����ʱ��

//Ĭ�ϲ���
#define IP_MAX_LEN				68		//ip��������󳤶�
#define FLASH_FLAG				0xAAA1	//FLASH ��־
#define SOCKET_PORT				8001	//�˿ں�
#define SOCKET_IP				"121.41.30.42"	//IP���ַ
#define DEF_BUF_SIZE			256				//Ĭ�����ݳ���
#define MAX_IDLE_TIME			0XFFFFFFFF		//������ʱ��
#define USER_BAUD				115200			//������

















typedef struct GLOBAL{
	u32 bufferSize;			//����/�������ݻ����С
	u32 reboot_time_count;	//ǿ������ʱ�䣬��ʱ
	u32 reboot_time_set;	//ǿ������ʱ�䣬����
	u32 baud;				//������
	u16	socketPort;			//socket�˿ں�
	u8 cur_ec20_msg;		//��ǰʹ�õ���Ϣ ec20
	u8 cur_stm32_msg;		//��ǰʹ�õ���Ϣ stm32
	u8 socket_state;			//TCP����״̬:1��tcp���ӣ�0��tcpδ����
	char socketIP[IP_MAX_LEN];		//ip���ӵ�ַ/��������(@�����67Byte)
	u8 max_buf_num;			//��󻺳�����ec20��stm32���ջ�����Ŀ���
}GLOBAL;

extern volatile GLOBAL global;

void global_init(void);






#endif
