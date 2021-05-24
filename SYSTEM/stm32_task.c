#include "global.h"
#include "msg.h"
#include "usart1.h"
#include "usart2.h"
#include "stm32_task.h"
#include "global.h"
#include "ec20.h"

/*
@���ܣ�stm32���ջص�����
@�������յ�����ָ��
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
@���ܣ�stm32 ��������
*/
void stm32_recv_task(void)
{
	uint8_t rs, err, resend = 0;
	MSG *cmsg;
	
	while(1)
	{
		err = getting_queue_stm32((void*)&cmsg, global.reboot_time_set);//�ȴ�������Ϣ�����޵ȴ�
		if(err == pdTRUE)
		{
			global.reboot_time_count = xTaskGetTickCount();
			if(global.socket_state != 1)//tcpδ����
			{
				msg_free(cmsg);//���������ͷŶ��пռ�
				printf("stm32 recv data no dealwith\r\n");
			}
			else
			{
				//1.�����ݷ���ec20
				//2.��ѯ�����Ƿ��ͳɹ�����tcp���л�Ӧ�����û���ɹ��������������¶������У��ر�TCP����������TCP,�ط���������
				rs = 0xff; resend = 0;
				do
				{
					rs = send_commucation_head(cmsg->len);
				}while(resend++ < 3 && rs != 1);
				
				if(rs == 1)//������ͷ��֪ͨec20������������
				{
					u8 i = 0;
					do{
						uart1_send(cmsg->buf, cmsg->len, 0);//������
						rs = sended_result();
						if(rs != 1)//����ʧ��(@EC20������ʧ�ܣ������Ƿ���Զ��ʧ��)
						{
							printf("ec20 send err\r\n\r\n");
						}
						else//���ͳɹ�
						{
							rs = query_sended_result(cmsg->len);//��鷢�͵�remote�Ƿ�ɹ�
							if(rs == 0)//��ʱ90s
							{
								rs = 111;
								global.socket_state = 2;//����
								printf("sended no ack\r\n\r\n");
							}
						}
					}while(i++ < 3 && rs != 1);
					
					if(rs == 1)//���͵�remote�ɹ�
					{
						
					}
					else//����ʧ��
					{
						printf("send data err\r\n\r\n");
						if(rs != 111)
						{
							global.socket_state = 0;//���½���tcp
							printf("sended data fail\r\n\r\n");
						}
					}
				}
				else//��������ͷ,��'>'�ظ�
				{
					printf("head is no ack\r\n\r\n");
					global.socket_state = 2;//����
				}
				msg_free(cmsg);//�ͷ���Ϣ����������remote�Ƿ��յ�û��
			}
		}
		else//��������ͼ��
		{
			global.socket_state = 2;//���½���tcp
			printf("global timeout\r\n\r\n");
		}
	}
}

