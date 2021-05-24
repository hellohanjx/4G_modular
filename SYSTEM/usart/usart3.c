#include "global.h"
#include "usart3.h"
#include "user_set.h"
#include "string.h"

static volatile UART3 rx;
static volatile UART3 *tx;

UART3_RECV_CALLBACK	uart3_recv_callback;//接收回调

/*
@说明：加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
*/
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART3->SR&0X40)==0);//循环发送,直到发送完毕   
    USART3->DR = (u8) ch;      
	return ch;
}
#endif 

/*
@功能：串口配置
@参数：波特率
*/
static void uart3_config(u32 baud)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//使能UART3时钟
  
	//USART3_TX   PB10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PB10
   
	//USART3_RX		PB11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PB11

	//Usart3 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8 ;	//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
	//USART3 初始化设置
	USART_InitStructure.USART_BaudRate = baud;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART3, &USART_InitStructure); 		//初始化串口3
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);	//开启串口接受中断
	USART_Cmd(USART3, ENABLE);                    	//使能串口13
}


/*
@功能："uart3 dma"接收初始化
@说明：使用DMA1_3通道
*/
static void dma_recv_config(void)
{	
	DMA_InitTypeDef DMA_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1时钟打开
	
	DMA_DeInit(DMA1_Channel3); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART3->DR);		//DR 寄存器地址 (0x40013804)
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx.buf; 				//DMA接收缓冲地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//外设->内存
	DMA_InitStructure.DMA_BufferSize = USART3_BUF_SIZE;					//DMA缓冲大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//dma 不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//内存自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//DMA 数据宽度
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//DMA 内存宽度
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//DMA 普通模式，不循环接收数据
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;						//DMA 高优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//关闭内存到内存
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL3);       //清除DMA1所有标志
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);	//开串口DMA1功能
	DMA_Cmd(DMA1_Channel3, ENABLE);					//开DMA1_3通道
}

/*
@功能："uart3 dma"发送初始化
@说明：使用DMA1_2 通道
*/
static void dma_send_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1 时钟

	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART3->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)tx->buf;
	DMA_InitStructure.DMA_BufferSize = 0;							//DMA缓冲大小
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;				//内存到外设
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//DMA外设地址不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;			//DMA内存地址自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据宽度
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	//DMA内存数据宽度
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;					//不循环接收
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;			//高优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;					//关闭内存到内存模式
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL2);
	
//	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);//开发送完成中断
//	DMA_ITConfig(DMA1_Channel2, DMA_IT_TE, ENABLE);//发送错误中断
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel2, DISABLE);

	//中断配置
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 9;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
	
	//清除标志
	DMA_ClearFlag(DMA1_FLAG_GL2);
	DMA_ClearFlag(DMA1_FLAG_HT2);
	DMA_ClearFlag(DMA1_FLAG_TC2);
	DMA_ClearFlag(DMA1_FLAG_TE2);
	
//	//清除中断标志
//	DMA_ClearITPendingBit(DMA1_IT_GL2);	//清全局中断
//	DMA_ClearITPendingBit(DMA1_IT_TC2);	//清传输完成
//	DMA_ClearITPendingBit(DMA1_IT_TE2);	//清传输错误
//	DMA_ClearITPendingBit(DMA1_IT_HT2);	//清传输过半
}

/*
@功能：DMA1_3 中断
@说明：未用，串口接收
*/
void DMA1_Channel3_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC3) == SET)    
	{
		DMA_ClearITPendingBit(DMA1_IT_TC3);
	}
	if(DMA_GetITStatus(DMA1_IT_TE3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TE3);
	}
	DMA_ClearITPendingBit(DMA1_IT_TC3);
	DMA_ClearITPendingBit(DMA1_IT_TE3);
	DMA_Cmd(DMA1_Channel3, DISABLE);
	DMA1_Channel3->CNDTR = 100;
	DMA_Cmd(DMA1_Channel3, ENABLE);
}

/*
@说明：DMA1_2 中断
@说明：未使，串口发送
*/
void DMA1_Channel2_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC2) == SET)    
	{
		DMA_ClearITPendingBit(DMA1_IT_TC2);
	}
	if(DMA_GetITStatus(DMA1_IT_TE2) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TE2);
	}
	if(DMA_GetITStatus(DMA1_IT_GL2) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_GL2);
	}
	if(DMA_GetITStatus(DMA1_IT_HT2) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_HT2);
	}
	DMA_Cmd(DMA1_Channel2, DISABLE);
}


/*
@功能：串口2中断
*/
void USART3_IRQHandler(void)
{
	uint32_t  sr;
	sr = sr;
//	sr = USART3->SR;
	
	if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)//发送空
	{
		USART_ClearITPendingBit(USART3, USART_IT_TXE);
	}
	
	if(USART_GetITStatus(USART3, USART_IT_TC) != RESET)//发送完成
	{
		USART_ClearITPendingBit(USART3, USART_IT_TC);
		USART_ITConfig(USART3, USART_IT_TC, DISABLE);                                  
	}
	
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//收到
	{	 	
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);//清接收中断标志
	}
	
	if(USART_GetITStatus(USART3, USART_IT_ORE) != RESET)//溢出错误（移位寄存器中的数据需要转移到RDR中，但是RDR 中有数据未读）
	{
		sr = USART_ReceiveData(USART3);//读出RDR中数据
		USART_ClearITPendingBit(USART3, USART_IT_ORE);
	}
	
	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)//空闲
	{
		sr = USART_ReceiveData(USART3);					//读一次RDR数据
		DMA_Cmd(DMA1_Channel3, DISABLE);				//关DMA接收通道
		USART_ClearITPendingBit(USART3, USART_IT_IDLE );//清空闲中断
		rx.len = USART3_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel3);	//计算收到数据长度
		uart3_recv_callback((UART3*)&rx);//回调函数
		
		DMA1_Channel3->CNDTR = USART3_BUF_SIZE;		//重置接收缓冲大小	
		DMA_Cmd(DMA1_Channel3, ENABLE);	//开接收通道
	}

	if(USART_GetITStatus(USART3 ,USART_IT_PE | USART_IT_FE | USART_IT_NE) != RESET)//错误
	{
		sr = USART_ReceiveData(USART3);
		USART_ClearITPendingBit(USART3, USART_IT_PE | USART_IT_FE | USART_IT_NE);
	}
}

/*
@功能：串口初始化
*/
void uart3_init(u32 baud)
{
	uart3_config(baud);
	dma_recv_config();
	dma_send_config();
	uart3_recv_callback = set_cmd_callback;
}

/*
@功能：串口3发送数据
@参数：	*pt_tx,待发送数据指针
		len, 发送数据长度
@返回值：操作结果
*/
uint8_t uart3_send(char *pt_txbuf , uint32_t tx_len)  
{	
	if(pt_txbuf == 0)
		return FALSE;
	
	
	//清标志位
	DMA_ClearFlag(DMA1_FLAG_TC2);
	DMA_ClearFlag(DMA1_FLAG_TE2);
	
	DMA_Cmd(DMA1_Channel2, DISABLE);	//关DMA，重设置参数
//	DMA1_Channel2->CCR &= (uint16_t)(~1);		//EN位置0
//	DMA1_Channel2->CPAR = (u32)(&USART2->DR);	//外设地址
	DMA1_Channel2->CMAR = (u32)(pt_txbuf);		//内存地址
	DMA1_Channel2->CNDTR = tx_len;				//重置"DMA"发送长度
	DMA_Cmd(DMA1_Channel2, ENABLE);	//开始发送，执行此指令后CRR的EN位置1
	return TRUE;
}

