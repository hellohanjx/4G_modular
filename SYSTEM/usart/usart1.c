#include "global.h"
#include "usart1.h"
#include "msg.h"
#include "global.h"

extern uint8_t callback_ec20_deault(MSG *rx);
/*
����1����
*/
static volatile MSG *tx;

static COMMUCATION_RECV_CALLBACK callBack_recv = 0;//���մ���ص�����
static COMMUCATION_RECV_CALLBACK default_callback = callback_ec20_deault;
/*
@���ܣ�uart1 ����
*/
static void uart1_config(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//���д���ˣ��׸���һ�죺RCC_APB2PeriphResetCmd
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);//ʹ��GPIOA��USART1ʱ�� 
 
	//�˿�����
	//PA9 -> UART1_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA10 -> UART1_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
   //USART1 ��ʼ������
	USART_DeInit(USART1);
	USART_InitStructure.USART_BaudRate = baud;						//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;				//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART1, &USART_InitStructure); 						//��ʼ������
	
		//���ж�
//	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	//�������ж�
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE );	//�������ж�
	USART_ITConfig(USART1, USART_IT_ERR, ENABLE);	//�������ж�
//	USART_ITConfig(USART1,USART_IT_TC, ENABLE);
//	USART_ITConfig(USART1,USART_IT_TXE, ENABLE);
	//���־λ
	USART_ClearFlag(USART1, USART_FLAG_TC);
	USART_ClearFlag(USART1, USART_FLAG_RXNE);
	USART_ClearFlag(USART1, USART_FLAG_IDLE);
	USART_ClearFlag(USART1, USART_FLAG_ORE);
	
	USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���

	//NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			//�����ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;	//��ռ���ȼ�6
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;			//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);								//��ʼ��VIC�Ĵ���
}

/*
@���ܣ�"uart1 dma"���ճ�ʼ��
@˵����ʹ��DMA1_5ͨ��
*/
static void dma_recv_config(void)
{	
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;				
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1ʱ�Ӵ�
	
	DMA_DeInit(DMA1_Channel5); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);		//DR �Ĵ�����ַ (0x40013804)
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)msg_recv_ec20[0].buf;	//DMA���ջ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//����->�ڴ�
	DMA_InitStructure.DMA_BufferSize = global.bufferSize;						//DMA�����С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//dma ������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//�ڴ�����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//DMA ���ݿ��
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//DMA �ڴ���
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//DMA ��ͨģʽ����ѭ����������
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;						//DMA �����ȼ�
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//�ر��ڴ浽�ڴ�
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL4);       //���DMA1���б�־
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);//���жϱ�־
//	DMA_ITConfig(DMA1_Channel5, DMA_IT_TE, ENABLE);//���жϱ�־
	
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);	//������DMA1����
	DMA_Cmd(DMA1_Channel5, ENABLE);					//��DMA1_5ͨ��
		
//	//����DMA1_5�ж�
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
}

/*
@���ܣ�"uart1 dma"���ͳ�ʼ��
@˵����ʹ��DMA1_4 ͨ��
*/
static void dma_send_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1 ʱ��

	DMA_DeInit(DMA1_Channel4);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);
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
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL4);
	
//	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);//����������ж�
//	DMA_ITConfig(DMA1_Channel4, DMA_IT_TE, ENABLE);//���ʹ����ж�
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel4, DISABLE);

	//�ж�����
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
	
	//�����־
	DMA_ClearFlag(DMA1_FLAG_GL4);
	DMA_ClearFlag(DMA1_FLAG_HT4);
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearFlag(DMA1_FLAG_TE4);
	
	//����жϱ�־
//	DMA_ClearITPendingBit(DMA1_IT_GL4);	//��ȫ���ж�
//	DMA_ClearITPendingBit(DMA1_IT_TC4);	//�崫�����
//	DMA_ClearITPendingBit(DMA1_IT_TE4);	//�崫�����
//	DMA_ClearITPendingBit(DMA1_IT_HT4);	//�崫�����
	
	
}

/*
@���ܣ�DMA1-5�жϣ����ڽ���
@˵����δ��
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
@˵����DMA1-4�жϣ����ڷ���
@˵����δʹ���ж�
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
@���ܣ�����1�ж�
*/
void USART1_IRQHandler(void)
{
	uint32_t  sr;
	sr = sr;
	//sr = USART1->SR;
	
	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)//���Ϳ�
	{
		USART_ClearITPendingBit(USART1, USART_IT_TXE);
	}
	
	if(USART_GetITStatus(USART1, USART_IT_TC) != RESET)//�������
	{
		USART_ClearITPendingBit(USART1, USART_IT_TC);
		USART_ITConfig(USART1, USART_IT_TC, DISABLE);                                  
	}
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)//�յ�
	{	 	
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);//������жϱ�־
//		rx_len %= 100;
//		rx_buffer[rx_len++]=USART_ReceiveData(USART1);//��ȡ���յ�������USART1->DR,�Զ������־λ
	}
	
	if(USART_GetITStatus(USART1, USART_IT_ORE) != RESET)//���������λ�Ĵ����е�������Ҫת�Ƶ�RDR�У�����RDR ��������δ����
	{
		sr = USART_ReceiveData(USART1);//����RDR������
		USART_ClearITPendingBit(USART1, USART_IT_ORE);
	}
	
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)//����
	{
		sr = USART_ReceiveData(USART1);					//��һ��RDR����
		DMA_Cmd(DMA1_Channel5, DISABLE);				//��DMA����ͨ��
		USART_ClearITPendingBit(USART1, USART_IT_IDLE );//������ж�
		
		msg_recv_ec20[global.cur_ec20_msg].state = 1;//�����Ϣ��ʹ��
		msg_recv_ec20[global.cur_ec20_msg].len = global.bufferSize - DMA_GetCurrDataCounter(DMA1_Channel5);	//�����յ����ݳ���

		callBack_recv((MSG*)&msg_recv_ec20[global.cur_ec20_msg]);//�����ص����������ж�����
		callBack_recv = default_callback;//��λ�ص�����
	
		DMA1_Channel5->CMAR = (u32)(ec20_rx()->buf);//�л��´λ���Ľ���λ��
		DMA1_Channel5->CNDTR = global.bufferSize;				
		DMA_Cmd(DMA1_Channel5, ENABLE);//������
	}

	if(USART_GetITStatus(USART1 ,USART_IT_PE | USART_IT_FE | USART_IT_NE) != RESET)//����
	{
		sr = USART_ReceiveData(USART1);
		USART_ClearITPendingBit(USART1, USART_IT_PE | USART_IT_FE | USART_IT_NE);
	}
}

/*
@���ܣ����ڳ�ʼ��
*/
void uart1_init(u32 baud)
{
	uart1_config(baud);
	dma_recv_config();
	dma_send_config();
	callBack_recv = default_callback;//Ĭ�ϻص�����
}


/*
@���ܣ�����2��������
		*pt_tx,����������ָ��
		len, �������ݳ���
		recvCallBacl�����ջص�����
@����ֵ���������
*/
uint8_t uart1_send(uint8_t *pt_txbuf , uint32_t tx_len, COMMUCATION_RECV_CALLBACK callback)  
{	
	if(pt_txbuf == 0)
		return FALSE;
	
	if(callback != 0)
		callBack_recv = callback;	//�ص�������ʼ��
	
	//���־λ
	DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearFlag(DMA1_FLAG_TE4);
	
	DMA_Cmd(DMA1_Channel4, DISABLE);	//��DMA�������ò���
//	DMA1_Channel4->CCR &= (uint16_t)(~1);		//ENλ��0
//	DMA1_Channel4->CPAR = (u32)(&USART1->DR);	//�����ַ
	DMA1_Channel4->CMAR = (u32)(pt_txbuf);		//�ڴ��ַ
	DMA1_Channel4->CNDTR = tx_len;				//����"DMA"���ͳ���
	DMA_Cmd(DMA1_Channel4, ENABLE);	//��ʼ���ͣ�ִ�д�ָ���CRR��ENλ��1
	return TRUE;
}
