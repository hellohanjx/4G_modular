#include "global.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"

#ifndef _MSG_H_
#define _MSG_H_


typedef struct MSG{
	u32 len;	//数据长度
	u8 *buf;	//数据指针
	u8 state;	//消息状态：0空闲，1已占用
}MSG;

extern volatile MSG msg_recv_ec20[MAX_BUF_NUM];		//ec20接收缓冲
extern volatile MSG msg_recv_stm32[MAX_BUF_NUM];	//stm32接收缓冲
extern volatile MSG cmd_rx_1th;	//命令数据1段
extern volatile MSG cmd_rx_2th;	//命令数据2段

MSG* ec20_rx(void);
MSG* stm32_rx(void);
u8 msg_free(MSG* cmsg);
void msg_init(void);

//信号量
BaseType_t sem_ec20_cmd_get(uint32_t time);
BaseType_t sem_ec20_recv_send_isr(void);

BaseType_t user_sended_get(uint32_t time);
BaseType_t user_sended_free_isr(void);

//////////队列
BaseType_t sending_queue_ec20_remote(MSG* mymail);
BaseType_t sending_front_ec20_remote(MSG* mymail);
BaseType_t getting_queue_ec20_remote(void *const msg, TickType_t time);

BaseType_t sending_queue_ec20_cmd(MSG* mymail);
BaseType_t sending_front_ec20_cmd(MSG* mymail);
BaseType_t getting_queue_ec20_cmd(void *const msg, TickType_t time);

BaseType_t sending_queue_stm32(MSG* mymail);
BaseType_t sending_front_stm32(MSG* mymail);
BaseType_t getting_queue_stm32(void *const msg, TickType_t time);

BaseType_t sending_queue_set_cmd(UART3* cmsg);
BaseType_t getting_queue_set_cmd(void *const cmsg, TickType_t time);


#endif
