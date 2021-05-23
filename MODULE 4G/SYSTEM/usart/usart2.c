#include "global.h"
#include "usart2.h"
#include "msg.h"

extern u8 stm32_recv_callback(MSG*rx);
/*
����2����
*/
static volatile MSG *tx;

static SEND_CALLBACK 	send_complete_callback;	//���ͻص�����
static RECV_CALLBACK	recv_callback = stm32_recv_callback;//���ջص�����

/*
@���ܣ�uart2 ����
*/
static void uart2_config(uint32_t baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��GPIOA
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//ʹ��USART2ʱ��
	
	//�˿�����
	//PA2 -> UART2_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA10 -> UART2_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
   //USART2 ��ʼ������
	USART_DeInit(USART2);
	USART_InitStructure.USART_BaudRate = baud;						//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;				//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART2, &USART_InitStructure); 						//��ʼ������
	
		//���ж�
//	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);	//�������ж�
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE );	//�������ж�
	USART_ITConfig(USART2, USART_IT_ERR, ENABLE);	//�������ж�
//	USART_ITConfig(USART2,USART_IT_TC, ENABLE);
//	USART_ITConfig(USART2,USART_IT_TXE, ENABLE);
	//���־λ
	USART_ClearFlag(USART2, USART_FLAG_TC);
	USART_ClearFlag(USART2, USART_FLAG_RXNE);
	USART_ClearFlag(USART2, USART_FLAG_IDLE);
	USART_ClearFlag(USART2, USART_FLAG_ORE);
	
	USART_Cmd(USART2, ENABLE);  //ʹ�ܴ���

	//NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;			//�����ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;	//��ռ���ȼ�
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;			//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);								//��ʼ��VIC�Ĵ���
}

/*
@���ܣ�"uart2 dma"���ճ�ʼ��
@˵����ʹ��DMA1_6ͨ��
*/
static void dma_recv_config(void)
{	
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;				
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1ʱ�Ӵ�
	
	DMA_DeInit(DMA1_Channel6); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);		//DR �Ĵ�����ַ (0x40013804)
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)msg_recv_stm32[0].buf; //DMA���ջ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//����->�ڴ�
	DMA_InitStructure.DMA_BufferSize = global.bufferSize;					//DMA�����С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//dma ������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//�ڴ�����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//DMA ���ݿ��
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//DMA �ڴ���
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//DMA ��ͨģʽ����ѭ����������
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;						//DMA �����ȼ�
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//�ر��ڴ浽�ڴ�
	DMA_Init(DMA1_Channel6, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL6);       //���DMA1���б�־
//	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);//���жϱ�־
//	DMA_ITConfig(DMA1_Channel6, DMA_IT_TE, ENABLE);//���жϱ�־
	
	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);	//������DMA1����
	DMA_Cmd(DMA1_Channel6, ENABLE);					//��DMA1_6ͨ��
		
//	//����DMA1_5�ж�
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
}

/*
@���ܣ�"uart2 dma"���ͳ�ʼ��
@˵����ʹ��DMA1_7 ͨ��
*/
static void dma_send_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1 ʱ��

	DMA_DeInit(DMA1_Channel7);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)tx->buf;
	DMA_InitStructure.DMA_BufferSize = 0;							//DMA�����С
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;				//�ڴ浽����
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//DMA�����ַ������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;			//DMA�ڴ��ַ����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//�������ݿ��
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	//DMA�ڴ����ݿ��
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;					//��ѭ������
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;			//�����ȼ�
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;					//�ر��ڴ浽�ڴ�ģʽ
	DMA_Init(DMA1_Channel7, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL7);
	
	DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);//����������ж�
//	DMA_ITConfig(DMA1_Channel7, DMA_IT_TE, ENABLE);//���ʹ����ж�
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel7, DISABLE);

	//�ж�����
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//�����־
	DMA_ClearFlag(DMA1_FLAG_GL7);
	DMA_ClearFlag(DMA1_FLAG_HT7);
	DMA_ClearFlag(DMA1_FLAG_TC7);
	DMA_ClearFlag(DMA1_FLAG_TE7);
	
	//����жϱ�־
	DMA_ClearITPendingBit(DMA1_IT_GL7);	//��ȫ���ж�
	DMA_ClearITPendingBit(DMA1_IT_TC7);	//�崫�����
	DMA_ClearITPendingBit(DMA1_IT_TE7);	//�崫�����
	DMA_ClearITPendingBit(DMA1_IT_HT7);	//�崫�����
	
	
}

/*
@���ܣ�DMA1_6 �ж�
@˵����δ�ã����ڽ���
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
@˵����DMA1_7�жϣ����ڷ���
@˵����δʹ���ж�
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
@���ܣ�����1�ж�
*/
void USART2_IRQHandler(void)
{
	uint32_t  sr;
	sr = sr;
	//sr = USART2->SR;
	
	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)//���Ϳ�
	{
		USART_ClearITPendingBit(USART2, USART_IT_TXE);
	}
	
	if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)//�������
	{
		USART_ClearITPendingBit(USART2, USART_IT_TC);
		USART_ITConfig(USART2, USART_IT_TC, DISABLE);                                  
	}
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)//�յ�
	{	 	
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);//������жϱ�־
//		rx_len %= 100;
//		rx_buffer[rx_len++]=USART_ReceiveData(USART2);//��ȡ���յ�������USART2->DR,�Զ������־λ
	}
	
	if(USART_GetITStatus(USART2, USART_IT_ORE) != RESET)//���������λ�Ĵ����е�������Ҫת�Ƶ�RDR�У�����RDR ��������δ����
	{
		sr = USART_ReceiveData(USART2);//����RDR������
		USART_ClearITPendingBit(USART2, USART_IT_ORE);
	}
	
	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)//����
	{
		sr = USART_ReceiveData(USART2);					//��һ��RDR����
		DMA_Cmd(DMA1_Channel6, DISABLE);				//��DMA����ͨ��
		USART_ClearITPendingBit(USART2, USART_IT_IDLE );//������ж�
		
		msg_recv_stm32[global.cur_stm32_msg].state = 1;
		msg_recv_stm32[global.cur_stm32_msg].len = global.bufferSize - DMA_GetCurrDataCounter(DMA1_Channel6);	//�����յ����ݳ���
		recv_callback((MSG*)&msg_recv_stm32[global.cur_stm32_msg]);//�ص�����
		
		DMA1_Channel6->CMAR = (u32)(stm32_rx()->buf);//�л��´λ���Ľ���λ��
		DMA1_Channel6->CNDTR = global.bufferSize;	//���ý��ջ����С	
		DMA_Cmd(DMA1_Channel6, ENABLE);				//������ͨ��
	}

	if(USART_GetITStatus(USART2 ,USART_IT_PE | USART_IT_FE | USART_IT_NE) != RESET)//����
	{
		sr = USART_ReceiveData(USART2);
		USART_ClearITPendingBit(USART2, USART_IT_PE | USART_IT_FE | USART_IT_NE);
	}
}

/*
@���ܣ�����2DMA��ʼ��
*/
void uart2_init(u32 baud)
{
	uart2_config(baud);
	dma_recv_config();
	dma_send_config();
	
}


/*
@���ܣ�����2��������
@������	**pt_rx,���������ݵ�ָ��ĵ�ַ
		*pt_tx,����������ָ��
		len, �������ݳ���
		recvCallBacl�����ջص�����
@����ֵ���������
*/
uint8_t uart2_send(uint8_t *pt_txbuf , uint32_t tx_len, SEND_CALLBACK send_callback)  
{	
	if(pt_txbuf == 0 && send_callback != 0)
		return FALSE;
	
	send_complete_callback = send_callback;
	
	//���־λ
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearFlag(DMA1_FLAG_TE4);
	
	DMA_Cmd(DMA1_Channel7, DISABLE);	//��DMA�������ò���
//	DMA1_Channel7->CCR &= (uint16_t)(~1);		//ENλ��0
//	DMA1_Channel7->CPAR = (u32)(&USART2->DR);	//�����ַ
	DMA1_Channel7->CMAR = (u32)(pt_txbuf);		//�ڴ��ַ
	DMA1_Channel7->CNDTR = tx_len;				//����"DMA"���ͳ���
	DMA_Cmd(DMA1_Channel7, ENABLE);	//��ʼ���ͣ�ִ�д�ָ���CRR��ENλ��1
	return TRUE;
}
