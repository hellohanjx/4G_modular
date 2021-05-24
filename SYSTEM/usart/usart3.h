#ifndef _USART3_H_
#define _USART3_H_

#include "stdio.h"	
#include "sys.h"
#include "global.h"


#define USART3_BUF_SIZE  		100  	//定义最大接收字节数 100
	  	

typedef struct UART3{
	u8 buf[USART3_BUF_SIZE];
	u8 len;
}UART3;

typedef void (*UART3_RECV_CALLBACK)(UART3*);

void uart3_init(u32 baud);
uint8_t uart3_send(char *pt_txbuf , uint32_t tx_len);

#endif


