#ifndef _UART2_H_
#define _UART2_H_

#include "global.h"
#include "msg.h"


typedef u8 (*SEND_CALLBACK)(void);
typedef u8 (*RECV_CALLBACK)(MSG*);


void uart2_init(u32 baud);
uint8_t uart2_send(uint8_t *pt_txbuf , uint32_t tx_len, SEND_CALLBACK send_callback);  

#endif

