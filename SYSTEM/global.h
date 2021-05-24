#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "sys.h"
#include "main.h"

#define VERSION			"20181011-1"

#define MAX_BUF_NUM		8		//接收缓冲数量（当前有20个接收缓冲，ec20接收10个，stm32接收缓冲10个）
#define MAX_BUF_MEMORY	4096	//分配给stm32与ec20的接收缓冲内存数
#define MAX_BUF_SIZE	1200	//最大数据长度
#define MIN_BUF_SIZE	64		//最小数据长度
#define MIN_IDLE_TIME	10000	//最小空闲时间

//默认参数
#define IP_MAX_LEN				68		//ip或域名最大长度
#define FLASH_FLAG				0xAAA1	//FLASH 标志
#define SOCKET_PORT				8001	//端口号
#define SOCKET_IP				"121.41.30.42"	//IP或地址
#define DEF_BUF_SIZE			256				//默认数据长度
#define MAX_IDLE_TIME			0XFFFFFFFF		//最大空闲时间
#define USER_BAUD				115200			//波特率

















typedef struct GLOBAL{
	u32 bufferSize;			//接收/发送数据缓冲大小
	u32 reboot_time_count;	//强制重连时间，计时
	u32 reboot_time_set;	//强制重连时间，参数
	u32 baud;				//波特率
	u16	socketPort;			//socket端口号
	u8 cur_ec20_msg;		//当前使用的消息 ec20
	u8 cur_stm32_msg;		//当前使用的消息 stm32
	u8 socket_state;			//TCP连接状态:1：tcp连接；0：tcp未连接
	char socketIP[IP_MAX_LEN];		//ip连接地址/或者域名(@域名最长67Byte)
	u8 max_buf_num;			//最大缓冲数，ec20与stm32接收缓冲数目相等
}GLOBAL;

extern volatile GLOBAL global;

void global_init(void);






#endif
