#ifndef _UART1_H_
#define _UART1_H_

#include "global.h"
#include "msg.h"

/*
@���ܣ����ջص�����
@������uint8*,����ָ�룻uint16_t* ���ճ���
*/
typedef uint8_t (*COMMUCATION_RECV_CALLBACK)(MSG *rx);

void uart1_init(u32 baud);
uint8_t uart1_send(uint8_t *pt_txbuf , uint32_t tx_len, COMMUCATION_RECV_CALLBACK callback);  

#endif

