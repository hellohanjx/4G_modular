/*
@˵����ϵͳ��Ϣ������ϵͳ���
*/

#include "msg.h"
#include "global.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"


volatile MSG msg_recv_ec20[MAX_BUF_NUM];	//ec20���ջ���
volatile MSG msg_recv_stm32[MAX_BUF_NUM];	//stm32���ջ���

volatile MSG cmd_rx_1th;		//��������
volatile MSG cmd_rx_2th;	//��������2��

/*
@���ܣ���Ϣ����ռ�
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
@���ܣ�Ѱ�ҿ��õ���Ϣ
@˵��������ec20����
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
	else//û�ҵ����õ���Ϣ�������һ����Ϣ��Ϊͨ�ã��ɸ���
	{
		global.cur_ec20_msg = global.max_buf_num - 1;
		msg_recv_ec20[global.cur_ec20_msg].state = 0;
	}
	return (MSG*)&msg_recv_ec20[global.cur_ec20_msg];
}


/*
@���ܣ�Ѱ�ҿ��õ���Ϣ
@˵��������stm32����
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
	else//û�ҵ����õ���Ϣ�������һ����Ϣ��Ϊͨ�ã��ɸ���
	{
		global.cur_stm32_msg = global.max_buf_num - 1;
		msg_recv_stm32[global.cur_stm32_msg].state = 0;
	}
	return (MSG*)&msg_recv_stm32[global.cur_stm32_msg];
}

/*
@���ܣ��ͷŵ�ǰ��Ϣ
@������cmsg��Ҫ�ͷŵ���Ϣ
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
@˵����ec20�������������ݶ���
@ʱ�䣺2018.7.5
*******************************************************************************************************/
static QueueHandle_t ec20_remote_queue;			//���ն��о��

/*
@���ܣ�����ͨѶ����
*/
static void queue_init_ec20_remote(void)
{
	ec20_remote_queue = xQueueCreate(global.max_buf_num, sizeof(msg_recv_ec20[0])); //�������Ͷ���
}

/*
@���ܣ�����Ϣ������β
@������mymail����Ϣָ��
*/
BaseType_t sending_queue_ec20_remote(MSG* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(ec20_remote_queue, (void*)&cmsg, &taskWoken);//��������β�����ȴ���ֱ�ӷ��ط�����Ϣ�Ľ��
	}
	return FALSE;
}
/*
@���ܣ�����Ϣ������ͷ
@������mymail����Ϣָ��
*/
BaseType_t sending_front_ec20_remote(MSG* mymail)
{
	configASSERT(mymail);
	if (mymail != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendToFrontFromISR(ec20_remote_queue, (void*)&mymail, &taskWoken);//��������ͷ
	}
	return FALSE;
}

/*
@���ܣ���ȡ�����е���Ϣ
@������msg,��Ϣ����ָ�룻time����ʱʱ��
*/
BaseType_t getting_queue_ec20_remote(void *const cmsg, TickType_t time)
{
	return xQueueReceive(ec20_remote_queue, cmsg, time);
}

/*******************************************************************************************************
@˵����ec20���������ض���
@ʱ�䣺2018.7.5
*******************************************************************************************************/
static QueueHandle_t ec20_cmd_queue;			//���ն��о��

/*
@���ܣ�����ͨѶ����
*/
static void queue_init_ec20_cmd(void)
{
	ec20_cmd_queue = xQueueCreate(global.max_buf_num, sizeof(msg_recv_ec20[0])); //�������Ͷ���
}

/*
@���ܣ�����Ϣ������β
@������mymail����Ϣָ��
*/
BaseType_t sending_queue_ec20_cmd(MSG* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(ec20_cmd_queue, (void*)&cmsg, &taskWoken);//��������β�����ȴ���ֱ�ӷ��ط�����Ϣ�Ľ��
	}
	return FALSE;
}
/*
@���ܣ�����Ϣ������ͷ
@������mymail����Ϣָ��
*/
BaseType_t sending_front_ec20_cmd(MSG* mymail)
{
	configASSERT(mymail);
	if (mymail != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendToFrontFromISR(ec20_cmd_queue, (void*)&mymail, &taskWoken);//��������ͷ
	}
	return FALSE;
}

/*
@���ܣ���ȡ�����е���Ϣ
@������msg,��Ϣ����ָ�룻time����ʱʱ��
*/
BaseType_t getting_queue_ec20_cmd(void *const cmsg, TickType_t time)
{
	return xQueueReceive(ec20_cmd_queue, cmsg, time);
}

/*******************************************************************************************************
@˵����stm32�������ݶ���
@ʱ�䣺2018.7.5
*******************************************************************************************************/
static QueueHandle_t stm32_recv_queue;			//���ն��о��

/*
@���ܣ�����ͨѶ����
*/
static void queue_init_stm32_user(void)
{
	stm32_recv_queue = xQueueCreate(global.max_buf_num, sizeof(msg_recv_stm32[0])); //�������Ͷ���
}

/*
@���ܣ�����Ϣ������β
@������mymail����Ϣָ��
*/
BaseType_t sending_queue_stm32(MSG* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(stm32_recv_queue, (void*)&cmsg, &taskWoken);//��������β�����ȴ���ֱ�ӷ��ط�����Ϣ�Ľ��
	}
	return FALSE;
}
/*
@���ܣ�����Ϣ������ͷ
@������mymail����Ϣָ��
*/
BaseType_t sending_front_stm32(MSG* cmsg)
{
	configASSERT(cmsg);
	if (cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendToFrontFromISR(stm32_recv_queue, (void*)&cmsg, &taskWoken);//��������ͷ
	}
	return FALSE;
}

/*
@���ܣ���ȡ�����е���Ϣ
@������msg,��Ϣ����ָ�룻time����ʱʱ��
*/
BaseType_t getting_queue_stm32(void *const cmsg, TickType_t time)
{
	return xQueueReceive(stm32_recv_queue, cmsg, time);
}

/*******************************************************************************************************
@˵����������ն���
@ʱ�䣺2018.7.10
*******************************************************************************************************/
static QueueHandle_t queue_set_cmd;			//���ն��о��

/*
@���ܣ�����ͨѶ����
*/
static void queue_init_set_cmd(void)
{
	queue_set_cmd = xQueueCreate(1, sizeof(msg_recv_ec20[0])); //�������Ͷ���
}

/*
@���ܣ�����Ϣ������β
@������mymail����Ϣָ��
*/
BaseType_t sending_queue_set_cmd(UART3* cmsg)
{
	configASSERT(cmsg);
	if(cmsg != NULL)
	{
		BaseType_t taskWoken;
		return xQueueSendFromISR(queue_set_cmd, (void*)&cmsg, &taskWoken);//��������β�����ȴ���ֱ�ӷ��ط�����Ϣ�Ľ��
	}
	return FALSE;
}

/*
@���ܣ���ȡ�����е���Ϣ
@������msg,��Ϣ����ָ�룻time����ʱʱ��
*/
BaseType_t getting_queue_set_cmd(void *const cmsg, TickType_t time)
{
	return xQueueReceive(queue_set_cmd, cmsg, time);
}



/*******************************************************************************************************
@˵����4G���������ź���
*******************************************************************************************************/
static SemaphoreHandle_t sem_ec20_recv;//ͨѶ�����ź������

/*
@˵���������ź����ͷ�
*/
static void sem_init_ec20_md(void)
{
	sem_ec20_recv = xSemaphoreCreateBinary();//������ֵ�ź��������ź���Ĭ�Ͽգ�
}
/*
@˵���������ź���
@������time���ȴ��ź�����ʱʱ��
*/
BaseType_t sem_ec20_cmd_get(uint32_t time)
{
	return xSemaphoreTake(sem_ec20_recv, time);
}
/*
@˵�����жϼ��ͷ��ź���
*/
BaseType_t sem_ec20_recv_send_isr(void)
{
	BaseType_t taskWoken;//���ֵ����pdTRUE == 1ʱ�˳��ж�ǰ������������л�
	return xSemaphoreGiveFromISR(sem_ec20_recv, &taskWoken);//�жϼ��ź����ͷţ������ͷŻ������ź�����
}


/*******************************************************************************************************
@˵����stm32���͵��ͻ����������ź���
*******************************************************************************************************/
static SemaphoreHandle_t sem_user_sended;//���͵��ͻ�����ź���

/*
@˵������ʼ��
*/
static void sem_init_user_sended(void)
{
	sem_user_sended = xSemaphoreCreateBinary();//������ֵ�ź��������ź���Ĭ�Ͽգ�
}
/*
@˵���������ź���
@������time���ȴ��ź�����ʱʱ��
*/
BaseType_t user_sended_get(uint32_t time)
{
	return xSemaphoreTake(sem_user_sended, time);
}
/*
@˵�����жϼ��ͷ��ź���
*/
BaseType_t user_sended_free_isr(void)
{
	BaseType_t taskWoken;//���ֵ����pdTRUE == 1ʱ�˳��ж�ǰ������������л�
	return xSemaphoreGiveFromISR(sem_user_sended, &taskWoken);//�жϼ��ź����ͷţ������ͷŻ������ź�����
}






/********************************************
@���ܣ���Ϣϵͳ��ʼ��
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



