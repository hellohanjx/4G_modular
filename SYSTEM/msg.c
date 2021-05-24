/*
@说明：系统消息，操作系统相关
*/

#include "msg.h"
#include "global.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"


volatile MSG msg_recv_ec20[MAX_BUF_NUM];	//ec20接收缓冲
volatile MSG msg_recv_stm32[MAX_BUF_NUM];	//stm32接收缓冲

volatile MSG cmd_rx_1th;		//命令数据
volatile MSG cmd_rx_2th;	//命令数据2段

/*
@功能：消息分配空间
*/
static void msg_mallco(void)
{
	u32 i;
	for(i = 0; i < global.max_buf_num; i++)
	{
		msg_recv_ec20[i].buf = (u8*)pvPortMalloc(global.bufferSize);
		msg_recv_ec20[i].len = 0;
		msg_recv_ec20[i].state = 0;
		
		msg_recv_stm32[i].buf = (u8*)pvPortMalloc(global.bufferSize);
		msg_recv_stm32[i].len = 0;
		msg_recv_stm32[i].state = 0;
	}
	cmd_rx_1th.buf = (u8*)pvPortMalloc(global.bufferSize);
	cmd_rx_2th.buf = (u8*)pvPortMalloc(global.bufferSize);
}

/*
@功能：寻找可用的消息
@说明：用于ec20接收
*/
MSG* ec20_rx(void)
{
	u8 find = FALSE, i;
	for(i = 0; i < global.max_buf_num - 1; i++)
	{
		if(msg_recv_ec20[i].state == 0)
		{
			find = TRUE;
			break;
		}
	}
	if(find == TRUE)
	{
		global.cur_ec20_msg = i;
	}
	else//没找到可用的消息，则最后一个消息作为通用，可覆盖
	{
		global.cur_ec20_msg = global.max_buf_num - 1;
		msg_recv_ec20[global.cur_ec20_msg].state = 0;
	}
	return (MSG*)&msg_recv_ec20[global.cur_ec20_msg];
}


/*
@功能：寻找可用的消息
@说明：用于stm32接收
*/
MSG* stm32_rx(void)
{
	u8 find = FALSE, i;
	for(i = 0; i < global.max_buf_num - 1; i++)
	{
		if(msg_recv_stm32[i].state == 0)
		{
			find = TRUE;
			break;
		}
	}
	if(find == TRUE)
	{
		global.cur_stm32_msg = i;
	}
	else//没找到可用的消息，则最后一个消息作为通用，可覆盖
	{
		global.cur_stm32_msg = global.max_buf_num - 1;
		msg_recv_stm32[global.cur_stm32_msg].state = 0;
	}
	return (MSG*)&msg_recv_stm32[global.cur_stm32_msg];
}

/*
@功能：释放当前消息
@参数：cmsg，要释放的消息
*/
u8 msg_free(MSG* cmsg)
{
	if(cmsg->state == 1)
	{
		cmsg->state = 0;
		return TRUE;
	}
	return FALSE;
}

/*******************************************************************************************************
@说明：ec20服务器接收数据队列
@时间：2018.7.5
*******************************************************************************************************/
static QueueHandle_t ec20_remote_queue;			//接收队列句柄

/*
@功能：创建通讯队列
*/
static void queue_init_ec20_remote(void)
{
	ec20_remote_queue = xQueueCreate(global.max_buf_num, sizeof(msg_recv_ec20[0])); //创建发送队列
}

/*
@功能：发消息到队列尾
@参数：mymail，消息指针
*/
BaseType_t sending_queue_ec20_remote(MSG* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(ec20_remote_queue, (void*)&cmsg, &taskWoken);//丢到队列尾；不等待，直接返回发送消息的结果
	}
	return FALSE;
}
/*
@功能：发消息到队列头
@参数：mymail，消息指针
*/
BaseType_t sending_front_ec20_remote(MSG* mymail)
{
	configASSERT(mymail);
	if (mymail != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendToFrontFromISR(ec20_remote_queue, (void*)&mymail, &taskWoken);//丢到队列头
	}
	return FALSE;
}

/*
@功能：获取队列中的消息
@参数：msg,消息缓冲指针；time，超时时间
*/
BaseType_t getting_queue_ec20_remote(void *const cmsg, TickType_t time)
{
	return xQueueReceive(ec20_remote_queue, cmsg, time);
}

/*******************************************************************************************************
@说明：ec20本身的命令返回队列
@时间：2018.7.5
*******************************************************************************************************/
static QueueHandle_t ec20_cmd_queue;			//接收队列句柄

/*
@功能：创建通讯队列
*/
static void queue_init_ec20_cmd(void)
{
	ec20_cmd_queue = xQueueCreate(global.max_buf_num, sizeof(msg_recv_ec20[0])); //创建发送队列
}

/*
@功能：发消息到队列尾
@参数：mymail，消息指针
*/
BaseType_t sending_queue_ec20_cmd(MSG* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(ec20_cmd_queue, (void*)&cmsg, &taskWoken);//丢到队列尾；不等待，直接返回发送消息的结果
	}
	return FALSE;
}
/*
@功能：发消息到队列头
@参数：mymail，消息指针
*/
BaseType_t sending_front_ec20_cmd(MSG* mymail)
{
	configASSERT(mymail);
	if (mymail != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendToFrontFromISR(ec20_cmd_queue, (void*)&mymail, &taskWoken);//丢到队列头
	}
	return FALSE;
}

/*
@功能：获取队列中的消息
@参数：msg,消息缓冲指针；time，超时时间
*/
BaseType_t getting_queue_ec20_cmd(void *const cmsg, TickType_t time)
{
	return xQueueReceive(ec20_cmd_queue, cmsg, time);
}

/*******************************************************************************************************
@说明：stm32接收数据队列
@时间：2018.7.5
*******************************************************************************************************/
static QueueHandle_t stm32_recv_queue;			//接收队列句柄

/*
@功能：创建通讯队列
*/
static void queue_init_stm32_user(void)
{
	stm32_recv_queue = xQueueCreate(global.max_buf_num, sizeof(msg_recv_stm32[0])); //创建发送队列
}

/*
@功能：发消息到队列尾
@参数：mymail，消息指针
*/
BaseType_t sending_queue_stm32(MSG* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(stm32_recv_queue, (void*)&cmsg, &taskWoken);//丢到队列尾；不等待，直接返回发送消息的结果
	}
	return FALSE;
}
/*
@功能：发消息到队列头
@参数：mymail，消息指针
*/
BaseType_t sending_front_stm32(MSG* cmsg)
{
	configASSERT(cmsg);
	if (cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendToFrontFromISR(stm32_recv_queue, (void*)&cmsg, &taskWoken);//丢到队列头
	}
	return FALSE;
}

/*
@功能：获取队列中的消息
@参数：msg,消息缓冲指针；time，超时时间
*/
BaseType_t getting_queue_stm32(void *const cmsg, TickType_t time)
{
	return xQueueReceive(stm32_recv_queue, cmsg, time);
}

/*******************************************************************************************************
@说明：命令接收队列
@时间：2018.7.10
*******************************************************************************************************/
static QueueHandle_t queue_set_cmd;			//接收队列句柄

/*
@功能：创建通讯队列
*/
static void queue_init_set_cmd(void)
{
	queue_set_cmd = xQueueCreate(1, sizeof(msg_recv_ec20[0])); //创建发送队列
}

/*
@功能：发消息到队列尾
@参数：mymail，消息指针
*/
BaseType_t sending_queue_set_cmd(UART3* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(queue_set_cmd, (void*)&cmsg, &taskWoken);//丢到队列尾；不等待，直接返回发送消息的结果
	}
	return FALSE;
}

/*
@功能：获取队列中的消息
@参数：msg,消息缓冲指针；time，超时时间
*/
BaseType_t getting_queue_set_cmd(void *const cmsg, TickType_t time)
{
	return xQueueReceive(queue_set_cmd, cmsg, time);
}



/*******************************************************************************************************
@说明：4G接收数据信号量
*******************************************************************************************************/
static SemaphoreHandle_t sem_ec20_recv;//通讯接收信号量句柄

/*
@说明：接收信号量释放
*/
static void sem_init_ec20_md(void)
{
	sem_ec20_recv = xSemaphoreCreateBinary();//创建二值信号量（此信号量默认空）
}
/*
@说明：申请信号量
@参数：time，等待信号量超时时间
*/
BaseType_t sem_ec20_cmd_get(uint32_t time)
{
	return xSemaphoreTake(sem_ec20_recv, time);
}
/*
@说明：中断级释放信号量
*/
BaseType_t sem_ec20_recv_send_isr(void)
{
	BaseType_t taskWoken;//这个值等于pdTRUE == 1时退出中断前必须进行任务切换
	return xSemaphoreGiveFromISR(sem_ec20_recv, &taskWoken);//中断级信号量释放（不能释放互斥型信号量）
}


/*******************************************************************************************************
@说明：stm32发送到客户，发送完信号量
*******************************************************************************************************/
static SemaphoreHandle_t sem_user_sended;//发送到客户完成信号量

/*
@说明：初始化
*/
static void sem_init_user_sended(void)
{
	sem_user_sended = xSemaphoreCreateBinary();//创建二值信号量（此信号量默认空）
}
/*
@说明：申请信号量
@参数：time，等待信号量超时时间
*/
BaseType_t user_sended_get(uint32_t time)
{
	return xSemaphoreTake(sem_user_sended, time);
}
/*
@说明：中断级释放信号量
*/
BaseType_t user_sended_free_isr(void)
{
	BaseType_t taskWoken;//这个值等于pdTRUE == 1时退出中断前必须进行任务切换
	return xSemaphoreGiveFromISR(sem_user_sended, &taskWoken);//中断级信号量释放（不能释放互斥型信号量）
}






/********************************************
@功能：消息系统初始化
********************************************/
void msg_init(void)
{
	msg_mallco();
	queue_init_ec20_remote();
	queue_init_ec20_cmd();
	queue_init_stm32_user();
	queue_init_set_cmd();
	sem_init_ec20_md();
	sem_init_user_sended();
}



