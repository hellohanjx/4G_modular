#ifndef _STM32_TASK_H_
#define _STM32_TASK_H_

#include "msg.h"

void stm32_recv_task(void);
u8 stm32_recv_callback(MSG*rx);

#endif

