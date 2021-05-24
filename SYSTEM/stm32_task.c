#include "global.h"
#include "msg.h"
#include "usart1.h"
#include "usart2.h"
#include "stm32_task.h"
#include "global.h"
#include "ec20.h"

/*
@功能：stm32接收回调函数
@参数：收到数据指针
*/
u8 stm32_recv_callback(MSG*rx)
{
	if(rx->len != 0)
	{
		sending_queue_stm32(rx);
	}
	return TRUE;
}


/*
@功能：stm32 接收任务
*/
void stm32_recv_task(void)
{
	uint8_t rs, err, resend = 0;
	MSG *cmsg;
	
	while(1)
	{
		err = getting_queue_stm32((void*)&cmsg, global.reboot_time_set);//等待队列消息。无限等待
		if(err == pdTRUE)
		{
			global.reboot_time_count = xTaskGetTickCount();
			if(global.socket_state != 1)//tcp未连接
			{
				msg_free(cmsg);//不处理，并释放队列空间
				printf("stm32 recv data no dealwith\r\n");
			}
			else
			{
				//1.将数据发给ec20
				//2.查询数据是否发送成功，即tcp是有回应。如果没发成功，此条数据重新丢进队列，关闭TCP，重新连接TCP,重发此条数据
				rs = 0xff; resend = 0;
				do
				{
					rs = send_commucation_head(cmsg->len);
				}while(resend++ < 3 && rs != 1);
				
				if(rs == 1)//发数据头，通知ec20即将发送数据
				{
					u8 i = 0;
					do{
						uart1_send(cmsg->buf, cmsg->len, 0);//发数据
						rs = sended_result();
						if(rs != 1)//发送失败(@EC20本身发送失败，并不是发送远端失败)
						{
							printf("ec20 send err\r\n\r\n");
						}
						else//发送成功
						{
							rs = query_sended_result(cmsg->len);//检查发送到remote是否成功
							if(rs == 0)//超时90s
							{
								rs = 111;
								global.socket_state = 2;//重启
								printf("sended no ack\r\n\r\n");
							}
						}
					}while(i++ < 3 && rs != 1);
					
					if(rs == 1)//发送到remote成功
					{
						
					}
					else//发送失败
					{
						printf("send data err\r\n\r\n");
						if(rs != 111)
						{
							global.socket_state = 0;//重新建立tcp
							printf("sended data fail\r\n\r\n");
						}
					}
				}
				else//发送数据头,无'>'回复
				{
					printf("head is no ack\r\n\r\n");
					global.socket_state = 2;//重启
				}
				msg_free(cmsg);//释放消息，这里无论remote是否收到没有
			}
		}
		else//超过最长发送间隔
		{
			global.socket_state = 2;//重新建立tcp
			printf("global timeout\r\n\r\n");
		}
	}
}

