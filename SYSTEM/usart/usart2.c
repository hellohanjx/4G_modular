#include "global.h"
#include "usart2.h"
#include "msg.h"

extern u8 stm32_recv_callback(MSG*rx);
/*
串口2驱动
*/
static volatile MSG *tx;

static SEND_CALLBACK 	send_complete_callback;	//发送回调函数
static RECV_CALLBACK	recv_callback = stm32_recv_callback;//接收回调函数

/*
@功能：uart2 配置
*/
static void uart2_config(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能GPIOA
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART2时钟
	
	//端口配置
	//PA2 -> UART2_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA10 -> UART2_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
   //USART2 初始化设置
	USART_DeInit(USART2);
	USART_InitStructure.USART_BaudRate = baud;						//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;				//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART2, &USART_InitStructure); 						//初始化串口
	
		//开中断
//	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);	//开接收中断
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE );	//开空闲中断
	USART_ITConfig(USART2, USART_IT_ERR, ENABLE);	//开错误中断
//	USART_ITConfig(USART2,USART_IT_TC, ENABLE);
//	USART_ITConfig(USART2,USART_IT_TXE, ENABLE);
	//清标志位
	USART_ClearFlag(USART2, USART_FLAG_TC);
	USART_ClearFlag(USART2, USART_FLAG_RXNE);
	USART_ClearFlag(USART2, USART_FLAG_IDLE);
	USART_ClearFlag(USART2, USART_FLAG_ORE);
	
	USART_Cmd(USART2, ENABLE);  //使能串口

	//NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;			//串口中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;	//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;			//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);								//初始化VIC寄存器
}

/*
@功能："uart2 dma"接收初始化
@说明：使用DMA1_6通道
*/
static void dma_recv_config(void)
{	
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;				
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1时钟打开
	
	DMA_DeInit(DMA1_Channel6); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);		//DR 寄存器地址 (0x40013804)
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)msg_recv_stm32[0].buf; //DMA接收缓冲地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//外设->内存
	DMA_InitStructure.DMA_BufferSize = global.bufferSize;					//DMA缓冲大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//dma 不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//内存自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//DMA 数据宽度
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//DMA 内存宽度
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//DMA 普通模式，不循环接收数据
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;						//DMA 高优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//关闭内存到内存
	DMA_Init(DMA1_Channel6, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL6);       //清除DMA1所有标志
//	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);//清中断标志
//	DMA_ITConfig(DMA1_Channel6, DMA_IT_TE, ENABLE);//清中断标志
	
	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);	//开串口DMA1功能
	DMA_Cmd(DMA1_Channel6, ENABLE);					//开DMA1_6通道
		
//	//配置DMA1_5中断
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
}

/*
@功能："uart2 dma"发送初始化
@说明：使用DMA1_7 通道
*/
static void dma_send_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1 时钟

	DMA_DeInit(DMA1_Channel7);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);
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
	DMA_Init(DMA1_Channel7, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL7);
	
	DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);//开发送完成中断
//	DMA_ITConfig(DMA1_Channel7, DMA_IT_TE, ENABLE);//发送错误中断
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel7, DISABLE);

	//中断配置
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//清除标志
	DMA_ClearFlag(DMA1_FLAG_GL7);
	DMA_ClearFlag(DMA1_FLAG_HT7);
	DMA_ClearFlag(DMA1_FLAG_TC7);
	DMA_ClearFlag(DMA1_FLAG_TE7);
	
	//清除中断标志
	DMA_ClearITPendingBit(DMA1_IT_GL7);	//清全局中断
	DMA_ClearITPendingBit(DMA1_IT_TC7);	//清传输完成
	DMA_ClearITPendingBit(DMA1_IT_TE7);	//清传输错误
	DMA_ClearITPendingBit(DMA1_IT_HT7);	//清传输过半
	
	
}

/*
@功能：DMA1_6 中断
@说明：未用，串口接收
*/
void DMA1_Channel6_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC6) == SET)    
	{
		DMA_ClearITPendingBit(DMA1_IT_TC6);
	}
	if(DMA_GetITStatus(DMA1_IT_TE6) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TE6);
	}
	DMA_ClearITPendingBit(DMA1_IT_TC6);
	DMA_ClearITPendingBit(DMA1_IT_TE6);
	DMA_Cmd(DMA1_Channel6, DISABLE);
	DMA1_Channel6->CNDTR = 100;
	DMA_Cmd(DMA1_Channel6, ENABLE);
}

/*
@说明：DMA1_7中断，串口发送
@说明：未使用中断
*/
void DMA1_Channel7_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC7) == SET)    
	{
		DMA_ClearITPendingBit(DMA1_IT_TC7);
	}
	if(DMA_GetITStatus(DMA1_IT_TE7) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TE7);
	}
	if(DMA_GetITStatus(DMA1_IT_GL7) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_GL7);
	}
	if(DMA_GetITStatus(DMA1_IT_HT7) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_HT7);
	}
	DMA_Cmd(DMA1_Channel7, DISABLE);
	send_complete_callback();
}

/*
@功能：串口1中断
*/
void USART2_IRQHandler(void)
{
	uint32_t  sr;
	sr = sr;
	//sr = USART2->SR;
	
	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)//发送空
	{
		USART_ClearITPendingBit(USART2, USART_IT_TXE);
	}
	
	if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//发送完成
	{
		USART_ClearITPendingBit(USART2, USART_IT_TC);
		USART_ITConfig(USART2, USART_IT_TC, DISABLE);                                  
	}
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//收到
	{	 	
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);//清接收中断标志
//		rx_len %= 100;
//		rx_buffer[rx_len++]=USART_ReceiveData(USART2);//读取接收到的数据USART2->DR,自动清除标志位
	}
	
	if(USART_GetITStatus(USART2, USART_IT_ORE) != RESET)//溢出错误（移位寄存器中的数据需要转移到RDR中，但是RDR 中有数据未读）
	{
		sr = USART_ReceiveData(USART2);//读出RDR中数据
		USART_ClearITPendingBit(USART2, USART_IT_ORE);
	}
	
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)//空闲
	{
		sr = USART_ReceiveData(USART2);					//读一次RDR数据
		DMA_Cmd(DMA1_Channel6, DISABLE);				//关DMA接收通道
		USART_ClearITPendingBit(USART2, USART_IT_IDLE );//清空闲中断
		
		msg_recv_stm32[global.cur_stm32_msg].state = 1;
		msg_recv_stm32[global.cur_stm32_msg].len = global.bufferSize - DMA_GetCurrDataCounter(DMA1_Channel6);	//计算收到数据长度
		recv_callback((MSG*)&msg_recv_stm32[global.cur_stm32_msg]);//回调函数
		
		DMA1_Channel6->CMAR = (u32)(stm32_rx()->buf);//切换下次缓冲的接收位置
		DMA1_Channel6->CNDTR = global.bufferSize;	//重置接收缓冲大小	
		DMA_Cmd(DMA1_Channel6, ENABLE);				//开接收通道
	}

	if(USART_GetITStatus(USART2 ,USART_IT_PE | USART_IT_FE | USART_IT_NE) != RESET)//错误
	{
		sr = USART_ReceiveData(USART2);
		USART_ClearITPendingBit(USART2, USART_IT_PE | USART_IT_FE | USART_IT_NE);
	}
}

/*
@功能：串口2DMA初始化
*/
void uart2_init(u32 baud)
{
	uart2_config(baud);
	dma_recv_config();
	dma_send_config();
	
}


/*
@功能：串口2发送数据
@参数：	**pt_rx,待传递数据的指针的地址
		*pt_tx,待发送数据指针
		len, 发送数据长度
		recvCallBacl，接收回调函数
@返回值：操作结果
*/
uint8_t uart2_send(uint8_t *pt_txbuf , uint32_t tx_len, SEND_CALLBACK send_callback)  
{	
	if(pt_txbuf == 0 && send_callback != 0)
		return FALSE;
	
	send_complete_callback = send_callback;
	
	//清标志位
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearFlag(DMA1_FLAG_TE4);
	
	DMA_Cmd(DMA1_Channel7, DISABLE);	//关DMA，重设置参数
//	DMA1_Channel7->CCR &= (uint16_t)(~1);		//EN位置0
//	DMA1_Channel7->CPAR = (u32)(&USART2->DR);	//外设地址
	DMA1_Channel7->CMAR = (u32)(pt_txbuf);		//内存地址
	DMA1_Channel7->CNDTR = tx_len;				//重置"DMA"发送长度
	DMA_Cmd(DMA1_Channel7, ENABLE);	//开始发送，执行此指令后CRR的EN位置1
	return TRUE;
}
