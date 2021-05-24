#ifndef _UART1_H_
#define _UART1_H_

#include "global.h"
#include "msg.h"

/*
@功能：接收回调函数
@参数：uint8*,数据指针；uint16_t* 接收长度
*/
typedef uint8_t (*COMMUCATION_RECV_CALLBACK)(MSG *rx);

void uart1_init(u32 baud);
uint8_t uart1_send(uint8_t *pt_txbuf , uint32_t tx_len, COMMUCATION_RECV_CALLBACK callback);  

#endif

