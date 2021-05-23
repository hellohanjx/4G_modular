#include "delay.h"
#include "global.h"
#include "FreeRTOS.h"
#include "task.h"
#include "osc_gpio.h"
#include "msg.h"
#include "usart1.h"
#include "usart2.h"
#include "ec20.h"
#include "stm32_task.h"
#include "global.h"
#include "user_set.h"
#include "protect.h"


#define START_TASK_PRIO		1			//�������ȼ�
#define START_STK_SIZE 		64  		//�����ջ��С	
TaskHandle_t StartTask_Handler;			//������
void start_task(void *pvParameters);	//��ʼ����

#define EC20_TASK_PRIO		10			//�������ȼ�
#define EC20_STK_SIZE 		128  		//�����ջ��С	
TaskHandle_t ec20_task_handler;			//������
void ec20_task(void *pvParameters);		//ec20��������

#define STM32_TASK_PRIO		11			//�������ȼ�
#define STM32_STK_SIZE 		128  		//�����ջ��С	
TaskHandle_t stm32_task_handler;		//������
void stm32_task(void *pvParameters);	//stm32��������

#define IDLE_TASK_PRIO		8			//�������ȼ�
#define IDLE_STK_SIZE 		128  		//�����ջ��С	
TaskHandle_t idle_task_handler;			//������
void idle_task(void *pvParameters);		//��������

#define SET_TASK_PRIO		9			//�������ȼ�
#define SET_STK_SIZE 		128  		//�����ջ��С	
TaskHandle_t set_task_handler;			//������
void set_task(void *pvParameters);		//��������


int main(void)
{
	Flash_EnableReadProtection();//��������
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	//������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������      
    vTaskStartScheduler();          //�����������
}

/**********************
@˵������ʼ����������
**********************/
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���
	gpio_init();	//GPIO��ʼ��
	delay_init();	//��ʱ������ʼ��
	global_init();	//ȫ�ֱ�����ʼ��
	msg_init();		//ϵͳ��Ϣ�����г�ʼ��
	uart3_init(global.baud);
	uart1_init(115200);
	uart2_init(global.baud);
    //����ec20����
    xTaskCreate((TaskFunction_t )ec20_task,     	
                (const char*    )"ec20_task",   	
                (uint16_t       )EC20_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )EC20_TASK_PRIO,	
                (TaskHandle_t*  )&ec20_task_handler);   
      
	//����stm32��������
				 xTaskCreate((TaskFunction_t )stm32_task,     	
                (const char*    )"stm32_task",   	
                (uint16_t       )STM32_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )STM32_TASK_PRIO,	
                (TaskHandle_t*  )&stm32_task_handler);
				
//	//�����������������־��
//				xTaskCreate((TaskFunction_t )idle_task,     	
//                (const char*    )"idle_task",   	
//                (uint16_t       )IDLE_STK_SIZE, 
//                (void*          )NULL,				
//                (UBaseType_t    )IDLE_TASK_PRIO,	
//                (TaskHandle_t*  )&idle_task_handler);   
	//���������������ò���
				xTaskCreate((TaskFunction_t )set_task,     	
                (const char*    )"set_task",   	
                (uint16_t       )SET_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )SET_TASK_PRIO,	
                (TaskHandle_t*  )&set_task_handler);   


    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}

/**********************
@˵����ec20������������
**********************/
void ec20_task(void *pvParameters)
{
	ec20_recv_task();
}   


/**********************
@˵����stm32������������
**********************/
void stm32_task(void *pvParameters)
{
	stm32_recv_task();
} 


/**********************
@˵�������ò�������
**********************/
void set_task(void *pvParameters)
{
	set_param_task();
}


/**********************
@˵������������
**********************/
void idle_task(void *pvParameters)
{
	char tmp[10];
	char *pcWriteBuffer = (char*)pvPortMalloc(255);
	
	while(1)
	{
		//������õ��ڴ����ʷ��Сʣ���ڴ�
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
		printf("\r\n");
		printf("\r\n");
		
		//����ʱ��ͳ��
//		vTaskGetRunTimeStats(pcWriteBuffer);
		
		vTaskDelay(OS_TICKS_PER_SEC*5);
	}
}   


