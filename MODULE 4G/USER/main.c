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


#define START_TASK_PRIO		1			//任务优先级
#define START_STK_SIZE 		64  		//任务堆栈大小	
TaskHandle_t StartTask_Handler;			//任务句柄
void start_task(void *pvParameters);	//开始任务

#define EC20_TASK_PRIO		10			//任务优先级
#define EC20_STK_SIZE 		128  		//任务堆栈大小	
TaskHandle_t ec20_task_handler;			//任务句柄
void ec20_task(void *pvParameters);		//ec20接收任务

#define STM32_TASK_PRIO		11			//任务优先级
#define STM32_STK_SIZE 		128  		//任务堆栈大小	
TaskHandle_t stm32_task_handler;		//任务句柄
void stm32_task(void *pvParameters);	//stm32接收任务

#define IDLE_TASK_PRIO		8			//任务优先级
#define IDLE_STK_SIZE 		128  		//任务堆栈大小	
TaskHandle_t idle_task_handler;			//任务句柄
void idle_task(void *pvParameters);		//空闲任务

#define SET_TASK_PRIO		9			//任务优先级
#define SET_STK_SIZE 		128  		//任务堆栈大小	
TaskHandle_t set_task_handler;			//任务句柄
void set_task(void *pvParameters);		//设置任务


int main(void)
{
	Flash_EnableReadProtection();//开读保护
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄      
    vTaskStartScheduler();          //开启任务调度
}

/**********************
@说明：开始任务任务函数
**********************/
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
	gpio_init();	//GPIO初始化
	delay_init();	//延时函数初始化
	global_init();	//全局变量初始化
	msg_init();		//系统消息，队列初始化
	uart3_init(global.baud);
	uart1_init(115200);
	uart2_init(global.baud);
    //创建ec20任务
    xTaskCreate((TaskFunction_t )ec20_task,     	
                (const char*    )"ec20_task",   	
                (uint16_t       )EC20_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )EC20_TASK_PRIO,	
                (TaskHandle_t*  )&ec20_task_handler);   
      
	//创建stm32接收任务
				 xTaskCreate((TaskFunction_t )stm32_task,     	
                (const char*    )"stm32_task",   	
                (uint16_t       )STM32_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )STM32_TASK_PRIO,	
                (TaskHandle_t*  )&stm32_task_handler);
				
//	//空闲任务，用来输出日志等
//				xTaskCreate((TaskFunction_t )idle_task,     	
//                (const char*    )"idle_task",   	
//                (uint16_t       )IDLE_STK_SIZE, 
//                (void*          )NULL,				
//                (UBaseType_t    )IDLE_TASK_PRIO,	
//                (TaskHandle_t*  )&idle_task_handler);   
	//设置任务，用来设置参数
				xTaskCreate((TaskFunction_t )set_task,     	
                (const char*    )"set_task",   	
                (uint16_t       )SET_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )SET_TASK_PRIO,	
                (TaskHandle_t*  )&set_task_handler);   


    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}

/**********************
@说明：ec20接收数据任务
**********************/
void ec20_task(void *pvParameters)
{
	ec20_recv_task();
}   


/**********************
@说明：stm32接收数据任务
**********************/
void stm32_task(void *pvParameters)
{
	stm32_recv_task();
} 


/**********************
@说明：设置参数设置
**********************/
void set_task(void *pvParameters)
{
	set_param_task();
}


/**********************
@说明：空闲任务
**********************/
void idle_task(void *pvParameters)
{
	char tmp[10];
	char *pcWriteBuffer = (char*)pvPortMalloc(255);
	
	while(1)
	{
		//输出可用的内存和历史最小剩余内存
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
		
		//任务状态
		vTaskList(pcWriteBuffer);
		printf("任务名      任务状态 优先级 剩余栈 任务序号\r\n");
		printf("%s\r\n", pcWriteBuffer);
		printf("\r\n");
		printf("\r\n");
		
		//任务时间统计
//		vTaskGetRunTimeStats(pcWriteBuffer);
		
		vTaskDelay(OS_TICKS_PER_SEC*5);
	}
}   


