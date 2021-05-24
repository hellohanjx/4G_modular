#include "global.h"
#include "usart3.h"
#include "user_set.h"
#include "string.h"

static volatile UART3 rx;
static volatile UART3 *tx;

UART3_RECV_CALLBACK	uart3_recv_callback;//���ջص�

/*
@˵�����������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
*/
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART3->SR&0X40)==0);//ѭ������,ֱ���������   
    USART3->DR = (u8) ch;      
	return ch;
}
#endif 

/*
@���ܣ���������
@������������
*/
static void uart3_config(u32 baud)
{
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��GPIOBʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//ʹ��UART3ʱ��
  
	//USART3_TX   PB10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��PB10
   
	//USART3_RX		PB11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��PB11

	//Usart3 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8 ;	//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
	//USART3 ��ʼ������
	USART_InitStructure.USART_BaudRate = baud;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	USART_Init(USART3, &USART_InitStructure); 		//��ʼ������3
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);	//�������ڽ����ж�
	USART_Cmd(USART3, ENABLE);                    	//ʹ�ܴ���13
}


/*
@���ܣ�"uart3 dma"���ճ�ʼ��
@˵����ʹ��DMA1_3ͨ��
*/
static void dma_recv_config(void)
{	
	DMA_InitTypeDef DMA_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1ʱ�Ӵ�
	
	DMA_DeInit(DMA1_Channel3); 
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART3->DR);		//DR �Ĵ�����ַ (0x40013804)
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx.buf; 				//DMA���ջ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//����->�ڴ�
	DMA_InitStructure.DMA_BufferSize = USART3_BUF_SIZE;					//DMA�����С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//dma ������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					//�ڴ�����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//DMA ���ݿ��
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//DMA �ڴ���
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//DMA ��ͨģʽ����ѭ����������
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;						//DMA �����ȼ�
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//�ر��ڴ浽�ڴ�
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL3);       //���DMA1���б�־
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);	//������DMA1����
	DMA_Cmd(DMA1_Channel3, ENABLE);					//��DMA1_3ͨ��
}

/*
@���ܣ�"uart3 dma"���ͳ�ʼ��
@˵����ʹ��DMA1_2 ͨ��
*/
static void dma_send_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//DMA1 ʱ��

	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART3->DR);
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
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	
	DMA_ClearFlag(DMA1_FLAG_GL2);
	
//	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);//����������ж�
//	DMA_ITConfig(DMA1_Channel2, DMA_IT_TE, ENABLE);//���ʹ����ж�
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel2, DISABLE);

	//�ж�����
//	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 9;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
	
	//�����־
	DMA_ClearFlag(DMA1_FLAG_GL2);
	DMA_ClearFlag(DMA1_FLAG_HT2);
	DMA_ClearFlag(DMA1_FLAG_TC2);
	DMA_ClearFlag(DMA1_FLAG_TE2);
	
//	//����жϱ�־
//	DMA_ClearITPendingBit(DMA1_IT_GL2);	//��ȫ���ж�
//	DMA_ClearITPendingBit(DMA1_IT_TC2);	//�崫�����
//	DMA_ClearITPendingBit(DMA1_IT_TE2);	//�崫�����
//	DMA_ClearITPendingBit(DMA1_IT_HT2);	//�崫�����
}

/*
@���ܣ�DMA1_3 �ж�
@˵����δ�ã����ڽ���
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
@˵����DMA1_2 �ж�
@˵����δʹ�����ڷ���
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
@���ܣ�����2�ж�
*/
void USART3_IRQHandler(void)
{
	uint32_t  sr;
	sr = sr;
//	sr = USART3->SR;
	
	if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)//���Ϳ�
	{
		USART_ClearITPendingBit(USART3, USART_IT_TXE);
	}
	
	if(USART_GetITStatus(USART3, USART_IT_TC) != RESET)//�������
	{
		USART_ClearITPendingBit(USART3, USART_IT_TC);
		USART_ITConfig(USART3, USART_IT_TC, DISABLE);                                  
	}
	
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//�յ�
	{	 	
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);//������жϱ�־
	}
	
	if(USART_GetITStatus(USART3, USART_IT_ORE) != RESET)//���������λ�Ĵ����е�������Ҫת�Ƶ�RDR�У�����RDR ��������δ����
	{
		sr = USART_ReceiveData(USART3);//����RDR������
		USART_ClearITPendingBit(USART3, USART_IT_ORE);
	}
	
	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)//����
	{
		sr = USART_ReceiveData(USART3);					//��һ��RDR����
		DMA_Cmd(DMA1_Channel3, DISABLE);				//��DMA����ͨ��
		USART_ClearITPendingBit(USART3, USART_IT_IDLE );//������ж�
		rx.len = USART3_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel3);	//�����յ����ݳ���
		uart3_recv_callback((UART3*)&rx);//�ص�����
		
		DMA1_Channel3->CNDTR = USART3_BUF_SIZE;		//���ý��ջ����С	
		DMA_Cmd(DMA1_Channel3, ENABLE);	//������ͨ��
	}

	if(USART_GetITStatus(USART3 ,USART_IT_PE | USART_IT_FE | USART_IT_NE) != RESET)//����
	{
		sr = USART_ReceiveData(USART3);
		USART_ClearITPendingBit(USART3, USART_IT_PE | USART_IT_FE | USART_IT_NE);
	}
}

/*
@���ܣ����ڳ�ʼ��
*/
void uart3_init(u32 baud)
{
	uart3_config(baud);
	dma_recv_config();
	dma_send_config();
	uart3_recv_callback = set_cmd_callback;
}

/*
@���ܣ�����3��������
@������	*pt_tx,����������ָ��
		len, �������ݳ���
@����ֵ���������
*/
uint8_t uart3_send(char *pt_txbuf , uint32_t tx_len)  
{	
	if(pt_txbuf == 0)
		return FALSE;
	
	
	//���־λ
	DMA_ClearFlag(DMA1_FLAG_TC2);
	DMA_ClearFlag(DMA1_FLAG_TE2);
	
	DMA_Cmd(DMA1_Channel2, DISABLE);	//��DMA�������ò���
//	DMA1_Channel2->CCR &= (uint16_t)(~1);		//ENλ��0
//	DMA1_Channel2->CPAR = (u32)(&USART2->DR);	//�����ַ
	DMA1_Channel2->CMAR = (u32)(pt_txbuf);		//�ڴ��ַ
	DMA1_Channel2->CNDTR = tx_len;				//����"DMA"���ͳ���
	DMA_Cmd(DMA1_Channel2, ENABLE);	//��ʼ���ͣ�ִ�д�ָ���CRR��ENλ��1
	return TRUE;
}

