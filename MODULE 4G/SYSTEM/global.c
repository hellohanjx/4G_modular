/*
@˵����ȫ�ֱ���/�����ļ�
*/

#include "sys.h"
#include "global.h"
#include "string.h"
#include "user_set.h"

volatile GLOBAL global;

/*
@���ܣ�ȫ�ֱ�����ʼ��
*/
void global_init(void)
{
	u16 tmp16;
	get_flash_flag(&tmp16);
	if(tmp16 == FLASH_FLAG)//��־λ��ȷ
	{
		char ip[70];
		u32 tmp32;
		//��ip��ַ
		if(get_socket_ip(ip))//����ȷ
		{
			strcpy((char*)global.socketIP, ip);
		}
		else//������
		{
			strcpy((char*)global.socketIP, SOCKET_IP);
		}
		
		//���˿ں�
		if(get_socket_port(&tmp16))
		{
			global.socketPort = tmp16;
		}
		else
		{
			global.socketPort = SOCKET_PORT;
		}
		
		//������ݳ���
		if(get_max_data(&tmp16))
		{
			global.bufferSize = tmp16;
		}
		else
		{
			global.bufferSize = DEF_BUF_SIZE;
		}
		
		//������ʱ��
		if(get_idle_time(&tmp32))
		{
			global.reboot_time_set = tmp32;
		}
		else
		{
			global.reboot_time_set = MAX_IDLE_TIME;
		}
		
		//������
		if(get_user_baud(&tmp32))
		{
			global.baud = tmp32;
		}
		else
		{
			global.baud = USER_BAUD;
		}
	}
	else
	{
		reset_user_set();//��λ���в���
		strcpy((char*)global.socketIP, SOCKET_IP);
		global.socketPort = SOCKET_PORT;
		global.bufferSize = DEF_BUF_SIZE;
		global.reboot_time_set = MAX_IDLE_TIME;
		global.baud = USER_BAUD;
	}
	
	//����������ʼ��
	global.cur_ec20_msg = 0;
	global.cur_stm32_msg = 0;
	global.reboot_time_count = 0;
	global.socket_state = 0;
	
	global.max_buf_num = (MAX_BUF_MEMORY/2)/global.bufferSize;//stm32��ec20���ջ�����
	
	
}


