/*
@˵�������ļ������û����ã�������cpuƬ��flash
*/

#include "global.h"
#include "user_set.h"
#include "string.h"
#include "usart3.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "task.h"
#include "msg.h"
#include "stdlib.h"

/*
@˵����ҳ��ַ��ע�⣺!!!!���ܿ�ҳд
*/
#define PAGE_63		0X0800FC00
#define PAGE_62		0X0800F800
#define PAGE_61		0X0800F400
#define PAGE_60		0X0800F000
#define PAGE_59		0X0800EC00
#define PAGE_58		0X0800E800

/*
@˵����ҳ��ƫ�Ƶ�ַ(63ҳ)
*/
//flash��־λ��Ĭ��0xAA
#define OFFSET_FLAG				0
#define OFFSET_FLAG_LEN				2
#define OFFSET_FLAG_CHK			2
#define OFFSET_FLAG_CHK_LEN			2

//ip��ַ������(62ҳ)
#define OFFSET_IP				0			//4
#define OFFSET_IP_LEN				IP_MAX_LEN
#define OFFSET_IP_CHK			IP_MAX_LEN			//72
#define OFFSET_IP_CHK_LEN			2

//�˿ں�(61ҳ)
#define OFFSET_PORT				0		//74
#define OFFSET_PORT_LEN				2
#define OFFSET_PORT_CHK			2		//76
#define OFFSET_PORT_CHK_LEN			2

//������������ֽ�(60ҳ)
#define OFFSET_SIZE				0		//78
#define OFFSET_SIZE_LEN				2
#define OFFSET_SIZE_CHK			2		//80
#define OFFSET_SIZE_CHK_LEN			2

//����2,3�����ʣ��û�����(59ҳ)
#define OFFSET_BAUD				0		//82
#define OFFSET_BAUD_LEN				4
#define OFFSET_BAUD_CHK			4		//86
#define OFFSET_BAUD_CHK_LEN			2

//�������������ʱ��(ms)(58ҳ)
#define OFFSET_TIME				0		//88
#define OFFSET_TIME_LEN				4
#define OFFSET_TIME_CHK			4		//90		
#define OFFSET_TIME_CHK_LEN			2

////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ�����FLASH��־λ
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 set_flash_flag(u16 flag)
{
	u8 i = 0, chk = 0;
	FLASH_Status rs;
	FLASH_Unlock();//����flash
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);//�����־λ
	FLASH_ErasePage(PAGE_63);//63ҳ
	
	//FLASH ��־
	do{
		rs = FLASH_ProgramHalfWord(PAGE_63 + OFFSET_FLAG, flag);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write flash err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	//дУ���
	i = 0;
	chk = ((flag >>8)&0xff) + (flag & 0xff);
	do{
		rs = FLASH_ProgramHalfWord(PAGE_63 + OFFSET_FLAG_CHK, chk);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write flash err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	FLASH_Lock();//����FLASH
	return TRUE;
}

/*
@���ܣ���flash��־
@������flag������ָ��
@���ؽ����ִ�н��
*/
u8 get_flash_flag(u16 *flag)
{
	u8 chk;
	u16 tmp = *(__IO u16*)(PAGE_63 + OFFSET_FLAG);
	chk = ((tmp >> 8)&0xff) + (tmp & 0xff);
	if(chk == *(u16*)(PAGE_63 + OFFSET_FLAG_CHK))
	{
		*flag = tmp;
		return TRUE;
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ�����ip��ַ
@������addr����ַָ�루�ַ��ͣ���len����ַ/��������(len <= 68)
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 set_socket_ip(char *addr)
{
	u8 i, j, chk, len;
	u32*pt = (u32*)addr;
	FLASH_Status rs;
	char tmp[IP_MAX_LEN + 4];
	
	for(i = 0; i < (IP_MAX_LEN + 4); i++)
	{
		tmp[i] = 0;
	}
	strcpy((void*)tmp, (const void*)addr);
	len = strlen(tmp);
	tmp[len] = 0;//��һ��������
	
	FLASH_Unlock();//����flash
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);//�����־λ
	FLASH_ErasePage(PAGE_62);//62ҳ
	
	//�洢IP������
	for(i = 0; i < len/4 + 1; i++)
	{
		j = 0;
		do{
			rs = FLASH_ProgramWord(PAGE_62 + OFFSET_IP + i*4, pt[i]);//д��Ƭ�ڵ�FLASH(����4Byteд)
		}while (rs != FLASH_COMPLETE && j++<3);
		
		if(rs != FLASH_COMPLETE)
		{
			printf("flash write ip err\r\n\r\n");
			FLASH_Lock();//����FLASH
			return FALSE;
		}
	}
	
	//�洢У��
	j = 0;
	for(i = 0, chk = 0; i < len; i++)
		chk += tmp[i];
	do{
		rs = FLASH_ProgramHalfWord(PAGE_62 + OFFSET_IP_CHK, chk);
	}while (rs != FLASH_COMPLETE && j++<3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write ip chk err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	FLASH_Lock();//����FLASH
	return TRUE;
}

/*
@���ܣ�����ip��ַ
@������addr����ַ����ָ��
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 get_socket_ip(char *addr)
{
	char tmp[IP_MAX_LEN + 4], i, chk;
	u32*pt = (u32*)tmp;
	
	for(i = 0; i < IP_MAX_LEN/4; i++)
		pt[i] = *(u32*)(PAGE_62 + OFFSET_IP + i*4);
	
	for(i = 0, chk = 0; i < IP_MAX_LEN && tmp[i] != 0; i++)
		chk += tmp[i];
	
	if(chk == *(u16*)(PAGE_62 + OFFSET_IP_CHK))
	{
		strcpy(addr, tmp);//�����������Ŀ���
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ����ö˿ں�
@�������˿ں�
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 set_socket_port(u32 port)
{
	if(port > 0xffff)
		return FALSE;
	
	u8 i = 0, chk = 0;
	FLASH_Status rs;
	FLASH_Unlock();//����flash
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);//�����־λ
	FLASH_ErasePage(PAGE_61);//61ҳ
	
	//д�˿ں�
	do{
		rs = FLASH_ProgramHalfWord(PAGE_61 + OFFSET_PORT, port);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write port err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	//дУ���
	i = 0;
	chk = ((port >>8)&0xff) + (port & 0xff);
	do{
		rs = FLASH_ProgramHalfWord(PAGE_61 + OFFSET_PORT_CHK, chk);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write port err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	FLASH_Lock();//����FLASH
	return TRUE;
}

/*
@���ܣ����˿ں�
@������port������ָ��
@���ؽ����ִ�н��
*/
u8 get_socket_port(u16 *port)
{
	u8 chk;
	u16 tmp = *(u16*)(PAGE_61 + OFFSET_PORT);
	chk = ((tmp >> 8)&0xff) + (tmp & 0xff);
	if(chk == *(u16*)(PAGE_61 + OFFSET_PORT_CHK))
	{
		*port = tmp;
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ����÷��Ͱ���󳤶�
@������len������������ݳ���
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 set_max_data(u16 len)
{
	if(len > MAX_BUF_SIZE || len < MIN_BUF_SIZE)
		return FALSE;
	
	u8 i = 0, chk = 0;
	FLASH_Status rs;
	FLASH_Unlock();//����flash
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);//�����־λ
	FLASH_ErasePage(PAGE_60);//60ҳ	
	
	//д�˿ں�
	do{
		rs = FLASH_ProgramHalfWord(PAGE_60 + OFFSET_SIZE, len);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write len err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	//дУ���
	i = 0;
	chk = (len >>8)&0xff + (len & 0xff);
	do{
		rs = FLASH_ProgramHalfWord(PAGE_60 + OFFSET_SIZE_CHK, chk);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write len err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	FLASH_Lock();//����FLASH
	return TRUE;
}

/*
@���ܣ������Ͱ���󳤶�
@������len������ָ��
@���ؽ����ִ�н��
*/
u8 get_max_data(u16 *len)
{
	u8 chk;
	u16 tmp = *(u16*)(PAGE_60 + OFFSET_SIZE);
	chk = (tmp >> 8)&0xff + (tmp & 0xff);
	if(chk == *(u16*)(PAGE_60 + OFFSET_SIZE_CHK))
	{
		*len = tmp;
		return TRUE;
	}
	return FALSE;
}




////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ����÷��Ϳ���ʱ��
@������timeout����λms
@˵���������������ʱ�䣬����Ҫ�ж�TCP�Ƿ�����
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 set_idle_time(u32 timeout)
{
	if(timeout < MIN_IDLE_TIME)
		return FALSE;
	
	u8 i = 0, chk = 0;
	FLASH_Status rs;
	FLASH_Unlock();//����flash
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);//�����־λ
	FLASH_ErasePage(PAGE_59);//59ҳ	
	
	//д�˿ں�
	do{
		rs = FLASH_ProgramWord(PAGE_59 + OFFSET_TIME, timeout);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write timout err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	//дУ���
	i = 0;
	chk = ((timeout >> 24)&0xff) + ((timeout >> 16)&0xff) + ((timeout >> 8)&0xff) + (timeout & 0xff);
	do{
		rs = FLASH_ProgramHalfWord(PAGE_59 + OFFSET_TIME_CHK, chk);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write timout err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	FLASH_Lock();//����FLASH
	return TRUE;
}

/*
@���ܣ������Ͱ���󳤶�
@������len������ָ��
@���ؽ����ִ�н��
*/
u8 get_idle_time(u32 *timeout)
{
	u8 chk;
	u32 tmp = *(u32*)(PAGE_59 + OFFSET_TIME);
	chk = ((tmp >> 24)&0xff) + ((tmp >> 16)&0xff) + ((tmp >> 8)&0xff) + (tmp & 0xff);
	if(chk == *(u16*)(PAGE_59 + OFFSET_TIME_CHK))
	{
		*timeout = tmp;
		return TRUE;
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ����ò�����
@���ؽ����ִ�������0->ʧ�ܣ�1->�ɹ�
*/
u8 set_user_baud(u32 baud)
{
	if( !(baud%1200 == 0) && ((baud/1200)%2 == 0) && (baud < 4500000) )
		return FALSE;
	
	u8 i = 0, chk = 0;
	FLASH_Status rs;
	FLASH_Unlock();//����flash
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);//�����־λ
	FLASH_ErasePage(PAGE_58);//58ҳ	
	
	//д�˿ں�
	do{
		rs = FLASH_ProgramWord(PAGE_58 + OFFSET_BAUD, baud);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write baud err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	//дУ���
	i = 0;
	chk = ((baud >> 24)&0xff) + ((baud >> 16)&0xff) + ((baud >> 8)&0xff) + (baud & 0xff);
	do{
		rs = FLASH_ProgramHalfWord(PAGE_58 + OFFSET_BAUD_CHK, chk);
	}while (rs != FLASH_COMPLETE && i++ < 3);
	if(rs != FLASH_COMPLETE)
	{
		printf("flash write baud err\r\n\r\n");
		FLASH_Lock();//����FLASH
		return FALSE;
	}
	
	FLASH_Lock();//����FLASH
	return TRUE;
}

/*
@���ܣ���������
@������len������ָ��
@���ؽ����ִ�н��
*/
u8 get_user_baud(u32 *baud)
{
	u8 chk;
	u32 tmp = *(u32*)(PAGE_58 + OFFSET_BAUD);
	chk = ((tmp >> 24)&0xff) + ((tmp >> 16)&0xff) + ((tmp >> 8)&0xff) + (tmp & 0xff);
	if(chk == *(u16*)(PAGE_58 + OFFSET_BAUD_CHK))
	{
		*baud = tmp;
		return TRUE;
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ����ù���ģʽ��͸�����߻���
@������type������ģʽ��0��͸����1�����壻len������ģʽ�»��������ȣ�Ĭ��256 Byte
@˵����!!!!δʵ��
*/
u8 set_work_mode(u8 type, u16 len)
{
	
	return FALSE;
}

/*
@���ܣ�������ģʽ
*/
u8 get_work_mode(u8 *type)
{
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
/*
@���ܣ���λ���в���
*/
void reset_user_set(void)
{
	set_socket_ip(SOCKET_IP);
	set_socket_port(SOCKET_PORT);
	set_max_data(DEF_BUF_SIZE);
	set_idle_time(MAX_IDLE_TIME);
	set_user_baud(USER_BAUD);
	set_flash_flag(FLASH_FLAG);//@ע�⣬д��־λҪ�ŵ������
}

////////////////////////////////////////////////////////////////////////////////////
static const char cmd_head[] = {"AT+"};//����ͷ
static const char cmd_baud[] = {"BAUD+"};//��ѯ������
static const char cmd_time[] = {"TIME+"};//����idleʱ��
static const char cmd_length[] = {"BUFF+"};//���õ���������
static const char cmd_ip[] = {"IP+"};//����ip
static const char cmd_port[] = {"PORT+"};//���ö˿�
static const char cmd_read[] = {"READ"};//��ȡ����
static const char cmd_reset[] = {"RESET"};//�ָ�Ĭ��
static const char cmd_system[] = {"SYSTEM"};//��ѯϵͳ�������(�ڴ��)
static const char cmd_version[] = {"VERSION"};//���汾

/*
@���ܣ��������ݻص�
@������rx�����ջ���ָ��
*/

void set_cmd_callback(UART3 *rx)
{
	if(rx->len != 0)
	{
		if(rx->buf[rx->len-1] == '$')
			sending_queue_set_cmd(rx);
		else
			printf("cmd tail is err\r\n\r\n");
	}
	else
	{
		printf("cmd len is 0\r\n\r\n");
	}
}


/*
@˵�������ò�������
*/
void set_param_task(void)
{
	UART3*cmsg;
	u8 err;
	
	while(1)
	{
		err = getting_queue_set_cmd((void*)&cmsg, portMAX_DELAY);
		if(err == pdTRUE)
		{
			if( !strncasecmp((char*)&(cmsg->buf), cmd_head, sizeof(cmd_head) - 1) )
			{
				char tmp[10] = {0,0,0,0,0,0,0,0,0};
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_baud, sizeof(cmd_baud) - 1) )
				{
					u32 baud;
					strncpy(tmp, (char*)&(cmsg->buf[8]), cmsg->len - 9);
					baud = atoi(tmp);
					if(set_user_baud(baud))
						printf("->set user baud OK\r\n\r\n");
					else
						printf("->set user baud FAIL\r\n\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_time, sizeof(cmd_time) - 1) )
				{
					u32 time;
					strncpy(tmp, (char*)&(cmsg->buf[8]), cmsg->len - 9);
					time = atoi(tmp);
					if(set_idle_time(time))
						printf("->set idle time OK\r\n\r\n");
					else
						printf("->set idle time FAIL\r\n\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_length, sizeof(cmd_length) - 1) )
				{
					u16 len;
					strncpy(tmp, (char*)&(cmsg->buf[7]), cmsg->len - 8);
					len = atoi(tmp);
					if(set_max_data(len))
						printf("->set buff length OK\r\n\r\n");
					else
						printf("->set buff length FAIL\r\n\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_ip, sizeof(cmd_ip) - 1) )
				{
					char ip[68], i;
					for(i = 0; i < IP_MAX_LEN; i++)
						ip[i] = 0;
					
					strncpy(ip, (char*)&(cmsg->buf[6]), cmsg->len - 7);
					if(set_socket_ip(ip))
						printf("->set IP/Domain OK\r\n\r\n");
					else
						printf("->set IP/Domain FAIL\r\n\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_port, sizeof(cmd_port) - 1) )
				{
					u32 port;
					strncpy(tmp, (char*)&(cmsg->buf[8]), cmsg->len - 9);
					port = atoi(tmp);
					if(set_socket_port(port))
						printf("->set port OK\r\n\r\n");
					else
						printf("->set port FAIL\r\n\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_reset, sizeof(cmd_reset) - 1) )//��λ����
				{
					reset_user_set();
					printf("->RESET OK\r\n\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_system, sizeof(cmd_system) - 1) )//��ϵͳ�������
				{
					char pcWriteBuffer[200];
					//������õ��ڴ����ʷ��Сʣ���ڴ�
					printf("\r\n");
					printf("============================\r\n");
					printf("Current free memory = ");
					sprintf(tmp, "%lu",xPortGetFreeHeapSize() );
					printf(tmp);
					printf("\r\n");
					printf("\r\n");
					printf("Minimum free memory in history = ");
					sprintf(tmp, "%lu",xPortGetMinimumEverFreeHeapSize() );
					printf(tmp);
					printf("\r\n");
					printf("\r\n");
					
					//����״̬
					vTaskList(pcWriteBuffer);
					printf("������      ����״̬ ���ȼ� ʣ��ջ �������\r\n");
					printf("%s\r\n", pcWriteBuffer);
					printf("============================\r\n");
					printf("\r\n");
					printf("\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_version, sizeof(cmd_version) - 1) )//��ϵͳ�������
				{
					printf(VERSION);
					printf("\r\n");
				}
				else
				if( !strncasecmp((char*)&(cmsg->buf[3]), cmd_read, sizeof(cmd_read) - 1) )//��ȡ���в���
				{
					char ip[70];
					u32 tmp32;
					u16 tmp16;
					
					printf("\r\n");
					//��ip��ַ
					if(get_socket_ip(ip))//����ȷ
					{
						printf("->IP = ");
						printf(ip);
						printf("\r\n");
					}
					else//������
					{
						printf("->IP read FAIL\r\n");
					}
					
					//���˿ں�
					if(get_socket_port(&tmp16))
					{
						printf("->PORT = ");
						sprintf(tmp, "%u", tmp16);
						printf(tmp);
						printf("\r\n");
					}
					else
					{
						printf("->PORT read FAIL\r\n");
					}
					
					//������ݳ���
					if(get_max_data(&tmp16))
					{
						printf("->buff length = ");
						sprintf(tmp, "%u", tmp16);
						printf(tmp);
						printf("\r\n");
					}
					else
					{
						printf("->buff length read FAIL\r\n");
					}
					
					//������ʱ��
					if(get_idle_time(&tmp32))
					{
						printf("->idle time = ");
						sprintf(tmp, "%u", tmp32);
						printf(tmp);
						printf("\r\n");
					}
					else
					{
						printf("->idle time read FAIL\r\n");
					}
					
					//������
					if(get_user_baud(&tmp32))
					{
						printf("->BAUD = ");
						sprintf(tmp, "%u", tmp32);
						printf(tmp);
						printf("\r\n");
					}
					else
					{
						printf("->BAUD read FAIL\r\n");
					}
				}
				else
				{
					printf("->no this cmd\r\n\r\n");
				}

			}
			else//�����ʽ��
			{
				printf("->cmd format err\r\n\r\n");
			}
		}
	}
	
}

