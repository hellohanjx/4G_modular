#include "global.h"
#include "usart1.h"
#include "msg.h"
#include "global.h"

extern uint8_t callback_ec20_deault(MSG *rx);
/*
串口1驱动
*/
static volatile MSG *tx;

static COMMUCATION_RECV_CALLBACK callBack_recv = 0;//接收处理回调函数
static COMMUCATION_RECV_CALLBACK default_callback = callback_ec20_deault;
/*
@功能：uart1 配置
*/
static void uart1_config(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//这个写错了，白搞了一天：RCC_APB2PeriphResetCmd
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);//使能GPIOA，USART1时钟 
 
	//端口配置
	//PA9 -> UART1_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA10 -> UART1_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
   //USART1 初始化设置
	USART_DeInit(USART1);
	USART_InitStructure.USART_BaudRate = baud;						//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;				//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART1, &USART_InitStructure); 						//初始化串口
	
		//开中断
//	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	//开接收中断
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE );	//开空闲中断
	USART_ITConfig(USART1, USART_IT_ERR, ENABLE);	//开错误中断
//	USART_ITConfig(USART1,USART_IT_TC, ENABLE);
//	USART_ITConfig(USART1,USART_IT_TXE, ENABLE);
	//清标志位
	USART_ClearFlag(USART1, USART_FLAG_TC);
	USART_ClearFlag(USART1, USART_FLAG_RXNE);
	USART_ClearFlag(USART1, USART_FLAG_IDLE);
	USART_ClearFlag(USART1, USART_FLAG_ORE);
	
	USART_Cmd(USART1, ENABLE);  //使能串口

	//NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			//串口中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;	//抢占优先级6
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;			//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);								//初始化VIC寄存器
}

/*
@功能："uart1 dma"接收初始化
@说明：使用DMA1_5通道
*/
static void dma_recv_config(void)
{	
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;				
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1时钟打开
	
	DMA_DeInit(DMA1_Channel5); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);		//DR 寄存器地址 (0x40013804)
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)msg_recv_ec20[0].buf;	//DMA接收缓冲地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//外设->内存
	DMA_InitStructure.DMA_BufferSize = global.bufferSize;						//DMA缓冲大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//dma 不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//内存自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//DMA 数据宽度
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//DMA 内存宽度
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//DMA 普通模式，不循环接收数据
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;						//DMA 高优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//关闭内存到内存
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL4);       //清除DMA1所有标志
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);//清中断标志
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TE, ENABLE);//清中断标志
	
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);	//开串口DMA1功能
	DMA_Cmd(DMA1_Channel5, ENABLE);					//开DMA1_5通道
		
//	//配置DMA1_5中断
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
}

/*
@功能："uart1 dma"发送初始化
@说明：使用DMA1_4 通道
*/
static void dma_send_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1 时钟

	DMA_DeInit(DMA1_Channel4);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);
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
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL4);
	
//	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);//开发送完成中断
//	DMA_ITConfig(DMA1_Channel4, DMA_IT_TE, ENABLE);//发送错误中断
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel4, DISABLE);

	//中断配置
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
	
	//清除标志
	DMA_ClearFlag(DMA1_FLAG_GL4);
	DMA_ClearFlag(DMA1_FLAG_HT4);
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearFlag(DMA1_FLAG_TE4);
	
	//清除中断标志
//	DMA_ClearITPendingBit(DMA1_IT_GL4);	//清全局中断
//	DMA_ClearITPendingBit(DMA1_IT_TC4);	//清传输完成
//	DMA_ClearITPendingBit(DMA1_IT_TE4);	//清传输错误
//	DMA_ClearITPendingBit(DMA1_IT_HT4);	//清传输过半
	
	
}

/*
@功能：DMA1-5中断，串口接收
@说明：未用
*/
void DMA1_Channel5_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC5) == SET)    
	{
		DMA_ClearITPendingBit(DMA1_IT_TC5);
	}
	if(DMA_GetITStatus(DMA1_IT_TE5) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TE5);
	}
	DMA_ClearITPendingBit(DMA1_IT_TC5);
	DMA_ClearITPendingBit(DMA1_IT_TE5);
	DMA_Cmd(DMA1_Channel5, DISABLE);
	DMA1_Channel5->CNDTR = 100;
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

/*
@说明：DMA1-4中断，串口发送
@说明：未使用中断
*/
void DMA1_Channel4_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC4) == SET)    
	{
		DMA_ClearITPendingBit(DMA1_IT_TC4);
	}
	if(DMA_GetITStatus(DMA1_IT_TE4) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_TE4);
	}
	if(DMA_GetITStatus(DMA1_IT_GL4) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_GL4);
	}
	if(DMA_GetITStatus(DMA1_IT_HT4) == SET)
	{
		DMA_ClearITPendingBit(DMA1_IT_HT4);
	}
	DMA_Cmd(DMA1_Channel4, DISABLE);
}

/*
@功能：串口1中断
*/
void USART1_IRQHandler(void)
{
	uint32_t  sr;
	sr = sr;
	//sr = USART1->SR;
	
	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)//发送空
	{
		USART_ClearITPendingBit(USART1, USART_IT_TXE);
	}
	
	if(USART_GetITStatus(USART1, USART_IT_TC) != RESET)//发送完成
	{
		USART_ClearITPendingBit(USART1, USART_IT_TC);
		USART_ITConfig(USART1, USART_IT_TC, DISABLE);                                  
	}
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)//收到
	{	 	
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);//清接收中断标志
//		rx_len %= 100;
//		rx_buffer[rx_len++]=USART_ReceiveData(USART1);//读取接收到的数据USART1->DR,自动清除标志位
	}
	
	if(USART_GetITStatus(USART1, USART_IT_ORE) != RESET)//溢出错误（移位寄存器中的数据需要转移到RDR中，但是RDR 中有数据未读）
	{
		sr = USART_ReceiveData(USART1);//读出RDR中数据
		USART_ClearITPendingBit(USART1, USART_IT_ORE);
	}
	
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)//空闲
	{
		sr = USART_ReceiveData(USART1);					//读一次RDR数据
		DMA_Cmd(DMA1_Channel5, DISABLE);				//关DMA接收通道
		USART_ClearITPendingBit(USART1, USART_IT_IDLE );//清空闲中断
		
		msg_recv_ec20[global.cur_ec20_msg].state = 1;//标记消息已使用
		msg_recv_ec20[global.cur_ec20_msg].len = global.bufferSize - DMA_GetCurrDataCounter(DMA1_Channel5);	//计算收到数据长度

		callBack_recv((MSG*)&msg_recv_ec20[global.cur_ec20_msg]);//启动回调函数处理中断内容
		callBack_recv = default_callback;//复位回调函数
	
		DMA1_Channel5->CMAR = (u32)(ec20_rx()->buf);//切换下次缓冲的接收位置
		DMA1_Channel5->CNDTR = global.bufferSize;				
		DMA_Cmd(DMA1_Channel5, ENABLE);//开接收
	}

	if(USART_GetITStatus(USART1 ,USART_IT_PE | USART_IT_FE | USART_IT_NE) != RESET)//错误
	{
		sr = USART_ReceiveData(USART1);
		USART_ClearITPendingBit(USART1, USART_IT_PE | USART_IT_FE | USART_IT_NE);
	}
}

/*
@功能：串口初始化
*/
void uart1_init(u32 baud)
{
	uart1_config(baud);
	dma_recv_config();
	dma_send_config();
	callBack_recv = default_callback;//默认回调函数
}


/*
@功能：串口2发送数据
		*pt_tx,待发送数据指针
		len, 发送数据长度
		recvCallBacl，接收回调函数
@返回值：操作结果
*/
uint8_t uart1_send(uint8_t *pt_txbuf , uint32_t tx_len, COMMUCATION_RECV_CALLBACK callback)  
{	
	if(pt_txbuf == 0)
		return FALSE;
	
	if(callback != 0)
		callBack_recv = callback;	//回调函数初始化
	
	//清标志位
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearFlag(DMA1_FLAG_TE4);
	
	DMA_Cmd(DMA1_Channel4, DISABLE);	//关DMA，重设置参数
//	DMA1_Channel4->CCR &= (uint16_t)(~1);		//EN位置0
//	DMA1_Channel4->CPAR = (u32)(&USART1->DR);	//外设地址
	DMA1_Channel4->CMAR = (u32)(pt_txbuf);		//内存地址
	DMA1_Channel4->CNDTR = tx_len;				//重置"DMA"发送长度
	DMA_Cmd(DMA1_Channel4, ENABLE);	//开始发送，执行此指令后CRR的EN位置1
	return TRUE;
}
