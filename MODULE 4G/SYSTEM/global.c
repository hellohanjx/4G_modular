/*
@说明：全局变量/方法文件
*/

#include "sys.h"
#include "global.h"
#include "string.h"
#include "user_set.h"

volatile GLOBAL global;

/*
@功能：全局变量初始化
*/
void global_init(void)
{
	u16 tmp16;
	get_flash_flag(&tmp16);
	if(tmp16 == FLASH_FLAG)//标志位正确
	{
		char ip[70];
		u32 tmp32;
		//读ip地址
		if(get_socket_ip(ip))//读正确
		{
			strcpy((char*)global.socketIP, ip);
		}
		else//读错误
		{
			strcpy((char*)global.socketIP, SOCKET_IP);
		}
		
		//读端口号
		if(get_socket_port(&tmp16))
		{
			global.socketPort = tmp16;
		}
		else
		{
			global.socketPort = SOCKET_PORT;
		}
		
		//最大数据长度
		if(get_max_data(&tmp16))
		{
			global.bufferSize = tmp16;
		}
		else
		{
			global.bufferSize = DEF_BUF_SIZE;
		}
		
		//最大空闲时间
		if(get_idle_time(&tmp32))
		{
			global.reboot_time_set = tmp32;
		}
		else
		{
			global.reboot_time_set = MAX_IDLE_TIME;
		}
		
		//波特率
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
		reset_user_set();//复位所有参数
		strcpy((char*)global.socketIP, SOCKET_IP);
		global.socketPort = SOCKET_PORT;
		global.bufferSize = DEF_BUF_SIZE;
		global.reboot_time_set = MAX_IDLE_TIME;
		global.baud = USER_BAUD;
	}
	
	//其他变量初始化
	global.cur_ec20_msg = 0;
	global.cur_stm32_msg = 0;
	global.reboot_time_count = 0;
	global.socket_state = 0;
	
	global.max_buf_num = (MAX_BUF_MEMORY/2)/global.bufferSize;//stm32或ec20接收缓冲数
	
	
}


