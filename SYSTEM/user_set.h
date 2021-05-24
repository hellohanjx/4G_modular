#ifndef _USER_SET_H_
#define _USER_SET_H_

#include "sys.h"
#include "usart3.h"


u8 set_flash_flag(u16 flag);
u8 get_flash_flag(u16 *flag);
u8 set_socket_ip(char *addr);
u8 get_socket_ip(char *addr);
u8 set_socket_port(u32 port);
u8 get_socket_port(u16 *port);
u8 set_max_data(u16 len);
u8 get_max_data(u16 *len);
u8 set_idle_time(u32 timeout);
u8 get_idle_time(u32 *timeout);
u8 set_user_baud(u32 baud);
u8 get_user_baud(u32 *baud);
u8 set_work_mode(u8 type, u16 len);
u8 get_work_mode(u8 *type);

void reset_user_set(void);
void set_param_task(void);
void set_cmd_callback(UART3 *rx);

#endif

