/*
��ԶEC20��4g����
ֻ��TCP/IP�����������������Ͷ���
@˵����4G������ָʾ�ƣ�
��ƣ�����������΢�����ػ�
�Ƶƣ�������Ϩ��ʱ�䳤��,����״̬������������ʱ�䳤�����������������ݴ���ģʽ
���ƣ�����ע��LTE����
@˵��������״̬���ܹ���ʵ��
@ע�⣺ÿ��ָ��س��������⻹�и�"OK"
*/

#include "FreeRTOS.h"
#include "queue.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "task.h"
#include "sys.h"
#include "usart1.h"
#include "usart2.h"
#include "string.h"
#include "ec20.h"
#include "msg.h"
#include "global.h"


#define C4G_PWR		PBout(12)	//��/�ػ�
#define C4G_RST		PBout(13)	//��λ����
#define C4G_POWERON_TIME	300	//�������ű���ʱ�䣨ms��
#define C4G_POWEROFF_TIME	800	//�ػ����ű���ʱ�䣨ms��
#define C4G_RESET_TIME		500	//��λ���ű���ʱ�䣨ms��

//ATָ��
 const char chk_baud[] = {"AT+IPR?\r\n"};				//��ѯ������
 const char set_baud[] = {"AT+IPR=115200\r\n"};		//���ò�����
 const char chk_signal[] = {"AT+CSQ\r\n"};			//��ѯ�ź�ǿ��
 const char close_cmd_echo[] = {"ATE0\r\n"};			//�ر��������
 const char save_param[] = {"AT&W\r\n"};				//�������ò�����Ӧ��ʱ300ms
 const char chk_iccid[] = {"AT+QCCID\r\n"};			//��ѯccid

 const char chk_pin[] = {"AT+CPIN?\r\n"};				//��ѯsim������(5s��ʱ)
  const char chk_pin_back1[] = {"+CME ERROR: 10"};	//sim��δ�忨
  const char chk_pin_back2[] = {"+CPIN: READY"};		//sim��׼����

 const char chk_sim_reg[] = {"AT+CREG?\r\n"};			//��ѯsim���Ƿ�ע��
 const char chk_net_reg[] = {"AT+CGREG?\r\n"};		//��ѯnetע��
 
 //��Ӫ��APN
 const char set_link_ChinaMobile[] = {"AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n"};//���ý����(�ƶ�) APN,USERNAME,PASSWORD
 const char set_link_UNICOM[] = {"AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n"};//���ý����(��ͨ) APN,USERNAME,PASSWORD
 const char set_link_p[] = {"AT+QICSGP=1,1,\"CTLTE\",\"\",\"\",1\r\n"};//���ý����(����) APN,USERNAME,PASSWORD
 
 const char chk_link_point[] = {"AT+QICSGP=1\r\n"};	//��ѯ����� 
 const char active_pdp_context[] = {"AT+QIACT=1\r\n"};//��������(150s��ʱ)
 const char chk_pdp_context[] = {"AT+QIACT?\r\n"};	//��ѯcontext,���ص�ǰ�������Ϣ(150s��ʱ)
 const char deactive_pdp_context[] = {"AT+QIDEACT=1\r\n"};//�رս���㣨40s��ʱ��
// const char creat_tcp[] = {"AT+QIOPEN=1,0,\"TCP\",\"120.55.115.113\",5008,0,2\r\n"};//����TCP���ӣ�150s��
//  const char creat_tcp[] = {"AT+QIOPEN=1,0,\"TCP\",\"120.26.204.86\",8001,0,2\r\n"};//����TCP���ӣ�150s��
 
const char creat_tcp_top[] = {"AT+QIOPEN=1,0,\"TCP\",\""};
const char creat_tcp_tail[] = {"\","};//����TCP���ӣ�150s��
const char creat_tcp_port[] = {",0,1\r\n"};//����˶˿ں�+����ģʽ������ģʽ����͸����
	 const char creat_tcp_back1[] = {"CONNECT"};//����TCP�ɹ�,͸��ģʽ�Ļ�Ӧ
	
 const char close_tcp[] = {"AT+QICLOSE=0\r\n"};		//�ر�TCP���ӣ���������Լ����ó�ʱ��
 const char chk_tcp[] = {"AT+QISTATE=1,0\r\n"};		//��ѯTCP����(@��ѯ ָ���� ͨ�� connectID)
 const char chk_data_echo[] = {"AT+QISDE?\r\n"};		//��ѯ���ݻ���
//const char close_data_echo[] = {"AT+QISDE=0\r\n"};	//�ر����ݻ��ԣ�͸��ģʽ����Ҫ��
 const char chk_err_code[] = {"AT+QIGETERROR\r\n"};	//��ѯ����Ĵ�����

 const char exit_transpartent[] = {"+++"};		//�˳�͸��ģʽ
 const char exit_transpartent2[] = {"++++++++\r\n"};		//����ģʽ�µ�+++����

 const char change_transparent_mode[] = {"AT+QISWTMD=1,2\r\n"};		//��Ϊ͸��ģʽ
 const char enter_transparent_mode[] = {"ATO\r\n"};		//����͸��ģʽ

  const char common_ack[] = {"OK"};	//ͨ�ûظ�"OK"

  const char chk_Operator[] = {"AT+COPS?\r\n"};//����ʲô�����ƶ���ͨ����
	const char Operater1_ack[] = {"CHINA MOBILE"};//�й��ƶ�
	const char Operater2_ack[] = {"CHN-UNICOM"};//�й���ͨ
	const char Operater3_ack[] = {"CHN-CT"};//�й�����
//	const char Operater1_ack[] = {"china mobile"};//�й��ƶ�
//	const char Operater2_ack[] = {"chn-unicom"};//�й���ͨ
//	const char Operater3_ack[] = {"chn-ct"};//�й�����
//״̬��״̬

const char chk_signal_quality[] = {"AT+CSQ\r\n"};//��ѯ�ź�ǿ��
	const char chk_signal_ack[] = {"+CQS"};//��ѯ�ź�ǿ�ȷ���ֵ

	
const char remote_recv[] = {"+QIURC"};//���������ݷ���

const char tcp_direct_ack[] = {"+QIOPEN:"};//tcp ���� direct pushģʽ����ֵ


const char send_head[] = {"AT+QISEND=0,"};//֪ͨec20������������
	const char send_head_ack[] = {">"};
	const char sended_data_ack[] = {"SEND OK"};
const char query_sended[] = {"AT+QISEND=0,0\r\n"};//��ѯ���ͽ��
	const char query_sended_ack[] = {"+QISEND:"};
	
/////////////////////////////////
//ģ�ⷢ�Ͱ�
u8 linknum ;
void get_link_info(volatile uint8_t *str)
{
	char tmp[10];
	uint8_t i, j, chk;
	
	i = 0;
	str[i++]=0x1f;
	str[i++]='*';
	str[i++]='*';
	str[i++]=0x30;
	str[i++]='*';//???
	
	sprintf(tmp, "%010u", 1000000003);
	for(j = 0;j < 10; j++)
	{
		str[i++]=tmp[j];
	}
	str[i++]='*';
	
	sprintf(tmp, "%06u", 0);
	for(j = 0; j < 6; j++)
	{
		str[i++]=tmp[j];
	}
	str[i++]='*'; 
	
	sprintf(tmp, "%02u", 2);
	for(j = 0; j < 2; j++)
	{
		str[i++]=tmp[j];
	}
	//?
	sprintf(tmp, "%02u", 12);
	for(j = 0; j < 2; j++)
	{
		str[i++]=tmp[j];
	}
	str[i++]='*'; 
	str[2] = i+1;
	str[1] = 1;
	
	for(j = 0, chk = 0;j < (str[2]-1) ; j++)
	{
		chk += str[j];
	}
	str[i++] = chk;//У���
	
	
}






enum
{
	ST_PWR_ON = 0,		//�ϵ�
	ST_CHK_SIM = 1,		//��ѯsim��
	ST_CHK_SIM_REG = 2,		//��ѯsim������
	ST_CHK_NET_REG = 3,		//��ѯ����
	ST_SET_POINT = 4,	//���ý����
	ST_CHK_POINT = 5,	//��ѯ�����
	ST_ACTIVE_POINT = 6,//��������
	ST_DEACTIVE_POINT = 7,//�رս����
	ST_CHK_CONTEXT = 8,	//��ѯ������ı�
	ST_CREAT_TCP = 9,	//����TCP����
	ST_CLOSE_TCP = 10,	//�ر�TCP
	ST_CHK_TCP = 11,	//��ѯTCP
	ST_CHK_DATA_ECHO = 12,//��ѯ���ݻ���
	ST_CLOSE_DATA_ECHO = 13,//�ر����ݻ���
	ST_CHK_ERRCODE = 14,	//��ѯ����Ĵ�����
	ST_EXIT_TRANSPARTENT = 15,//�˳�͸��ģʽ
	ST_CLOSE_CMD_ECHO = 16,	//�ر��������
	ST_REBOOT = 17,		//����4G
	ST_CHANGE_TRANSPARENT = 18,	//��Ϊ͸��ģʽ
	ST_ENTER_TRANSPARENT = 19, //����͸��ģʽ
	ST_CHK_OPERATER = 20,	//��ѯ������
	ST_CHK_SIGNAL = 21,	//��ѯ�ź�ǿ��
	ST_COMMUCATION = 22,	//ͨ������
};


static volatile EC20 ec20;		//����EC20���ݽṹ��


/**********************************************************************************************************
@˵����EC20���Ӻ���(�ص�����)
@���ߣ�������
@����ʱ�䣺2018.7.7
@�޸�ʱ�䣺
**********************************************************************************************************/


/*
@���ܣ�EC20�������ݷ��ش���ص�����
@������rx����������ָ��
@����ֵ��ִ�н��
*/
static uint8_t callback_ec20_cmd(MSG *rx)
{
	u8 rs = FALSE;
	if(rx->len != 0)
	{
		if(rx->buf[0] == 0x0D && rx->buf[1] == 0x0A)
		{
			memcpy((void*)cmd_rx_1th.buf, (const void*)rx->buf, rx->len);//������cmd��Ϣ;@����ע���£�������rxֱ�ӿ���������rx.buf����
			cmd_rx_1th.len = rx->len;
			sem_ec20_recv_send_isr();
			rs = TRUE;
		}
	}
	msg_free(rx);//�ͷŵ�ǰ��Ϣ
	return rs;
}
/*
@���ܣ�EC20Ĭ�Ͻ������ݻص�������
@������rx����������ָ��
@����ֵ��ִ�н��
@˵������������������������������ݶ������ﴦ��
*/
uint8_t callback_ec20_deault(MSG *rx)
{
	u8 rs = FALSE;
	if(rx->len != 0)
	{
		if(rx->buf[0] == 0x0D && rx->buf[1] == 0x0A)
		{
			if( !strncasecmp((char*)&(rx->buf[2]), remote_recv, sizeof(remote_recv) - 1) )//���������ݷ��أ�ǰ׺Ϊ "+QIURC"
			{
				if( sending_queue_ec20_remote(rx) )//����������ȥ������յ�ǰ���ջ���
					rs = TRUE;
			}
			else//�������ݷ���,����2�������Ӧack
			{
				memcpy((void*)cmd_rx_2th.buf, (const void*)rx->buf, rx->len);//������cmd��Ϣ;@����ע���£�������rxֱ�ӿ���������rx.buf����
				cmd_rx_2th.len = rx->len;
				if( sending_queue_ec20_cmd((MSG*)&cmd_rx_2th) )//������Ϣ������
				{
					msg_free(rx);//�ͷŵ�ǰ��Ϣ
					rs = TRUE;
				}
			}	
		}
	}
	return rs;
}

/*
@���ܣ�stm32������ɻص�����
*/
u8 send_to_stm32_callback(void)
{
	user_sended_free_isr();	//�ͷ��ź���
	return TRUE;
}





/**********************************************************************************************************
@˵����EC20����
@���ߣ�������
@����ʱ�䣺2018.7.6
@�޸�ʱ�䣺
**********************************************************************************************************/

/*
���ܣ�4g�������ų�ʼ��
*/
void ec20_config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphResetCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��GPIOʱ��

	//PB13 -> 4G_RST, PB12 -> 4G_PWR
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_12;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//GPIO�ٶ�
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//��ʼ��GPIO
	GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_12);		//Ĭ�϶�����
	
}



/*
@���ܣ�����
*/
static void c4g_power_on(void)
{
	C4G_PWR = 1;	//����
	vTaskDelay(C4G_POWERON_TIME);
	C4G_PWR = 0;	//����
}

/*
@���ܣ��ػ�
*/
static void c4g_power_off(void)
{
	C4G_PWR = 1;	//����
	vTaskDelay(C4G_POWEROFF_TIME);
	C4G_PWR = 0;	//����
	vTaskDelay(39*OS_TICKS_PER_SEC);//�ػ����ʱ35s
}

/*
@���ܣ���λ
*/
static void c4g_power_reset(void)
{
	C4G_RST = 1;
	vTaskDelay(C4G_RESET_TIME);
	C4G_RST = 0;
	vTaskDelay(3000);

}

/*
@���ܣ��ر��������
@����ֵ��ִ��������Ƿ���Խ�����һ��
*/
static uint8_t c4g_close_cmd_echo(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;
	
	uart1_send((uint8_t*)close_cmd_echo, (sizeof(close_cmd_echo) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(300);//300ms��ʱ
	if(err == pdTRUE)
	{
		//���ء�OK��
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�����3���ַ���ʼ�Ƚ�,
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯsim��
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
*/
static uint8_t c4g_chk_sim(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_pin, (sizeof(chk_pin) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(5000);//�ȴ���ʱ 5s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), chk_pin_back2, sizeof(chk_pin_back2) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//����Ϊ "+CPIN: READY "��ʾsim��׼������
		}

		if( !strncasecmp((char*)&(rx->buf[2]), chk_pin_back1, sizeof(chk_pin_back1) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�
		{
			return 2;	//δ�忨
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯ�ź�ǿ��
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
*/
static uint8_t c4g_chk_signal(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_signal_quality, (sizeof(chk_signal_quality) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(320);//�ȴ���ʱ 300ms
	if(err == pdTRUE)
	{
		if(strncasecmp((char*)&(rx->buf[2]), chk_signal_ack, sizeof(chk_signal_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			//������һ���ȴ�����
			//��ѯ���У��ȴ�OKֵ
			
			return TRUE;//����Ϊ "+CQS "��ʾ��ѯ�ɹ�
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯsim��ע��
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_sim_reg(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_sim_reg, (sizeof(chk_sim_reg) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(300);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�����3���ַ���ʼ�Ƚ�,
		{
			//�����������
			uint8_t i, j;
			i = 9;
			ec20.n = rx->buf[i++];//����״̬
			++i;//����','
			ec20.stat = rx->buf[i++];//������Ϣ��1����ʾ��������
			if(ec20.n != '0')//�������δ����Ľ������ᴫ��һЩ��Ϣ
			{
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
					ec20.lac[j] = rx->buf[i];//��ַ����
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
					ec20.ci[j] = rx->buf[i];//cell id
				++i;
				ec20.act = rx->buf[i];//������ʽ��gsm/gprs...
			}
			
			return TRUE;//sim��ע��ɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯ����ע��״̬
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_net_reg(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_net_reg, (sizeof(chk_net_reg) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(300);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			//�����������
			uint8_t i, j;
			i = 10;
			ec20.n = rx->buf[i++];//����״̬
			++i;//����','
			ec20.stat = rx->buf[i++];//������Ϣ;1,��ʾ��������5����ʾ����
			if(ec20.n != '0')
			{
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.lac[j] = rx->buf[i];//��ַ����
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.ci[j] = rx->buf[i];//cell id
				++i;
				ec20.act = rx->buf[i];//������ʽ��gsm/gprs...
			}
			
			return TRUE;//sim��ע��ɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}


/*
@���ܣ���ѯ��������
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_chk_point(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_link_point, (sizeof(chk_link_point) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(300);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�Ƚ�����"OK"
		{
			//�����������
			uint8_t i, j;
			for(i = 2;  rx->buf[i] != ',' && i < 20; i++)
				ec20.context_type = rx->buf[i];//��ַIPv4
			i += 2;//����','
			for(j = 0;  rx->buf[i] != ',' && i < 20; i++, j++)
				ec20.apn[j] = rx->buf[i];//apn
			i += 2;
			for(j = 0;  rx->buf[i] != ',' && i < 20; i++, j++)
				ec20.username[j] = rx->buf[i];//�û�����
			i += 2;
			for(j = 0;  rx->buf[i] != ',' && i < 20; i++, j++)
				ec20.password[j] = rx->buf[i];//����
			i += 2;
			for(; i != ',' && i < 20; i++)
				ec20.authentication = rx->buf[i];//��֤0~3
			
			return TRUE;//��ѯ�ɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}


/*
@���ܣ�������������
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_set_point(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	if(ec20.Operater == 1)
		uart1_send((uint8_t*)set_link_ChinaMobile, (sizeof(set_link_ChinaMobile) - 1), callback_ec20_cmd);//���ڷ���
	else 
	if(ec20.Operater == 2)
		uart1_send((uint8_t*)set_link_UNICOM, (sizeof(set_link_UNICOM) - 1), callback_ec20_cmd);//���ڷ���
	else 
	if(ec20.Operater == 3)
		uart1_send((uint8_t*)set_link_p, (sizeof(set_link_p) - 1), callback_ec20_cmd);//���ڷ���
	else
		return FALSE;
	
	err = sem_ec20_cmd_get(300);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//���ý����ɹ�
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯ��������
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_chk_context(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_pdp_context, (sizeof(chk_pdp_context) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(15000);//�ȴ���ʱ15s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			//�����������
			uint8_t i, j;
			i = 12;
			ec20.context_state = rx->buf[i++];//�����״̬
			++i;//����','
			ec20.context_type = rx->buf[i++];//����Э������
			++i;
			for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
				ec20.local_address[j] = rx->buf[i];//����ip��ַ
			
			if(ec20.context_state == '1')//�Ѿ��������Ҫ�ٴμ���
				return 3;
			return TRUE;//��ѯ�ɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ�������������
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_active_context(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)active_pdp_context, (sizeof(active_pdp_context) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(152000);//�ȴ���ʱ150s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//����ɹ�
		}
		else
		{
			return 2;//�յ����ݵ������ݴ���
		}
	}
	return FALSE;
}

/*
@���ܣ��ر���������
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_deactive_context(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)deactive_pdp_context, (sizeof(deactive_pdp_context) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(41000);//�ȴ���ʱ40s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//����ɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ�����TCP����
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
@˵�����ɹ������͸��ģʽ���˳���������
*/
static uint8_t c4g_creat_tcp(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err,i;
	char creat_tcp[100];
	char port[6];
	
	for(i = 0; i < 100; i++){
		creat_tcp[i] = 0;
	}
	for(i = 0; i < 6; i++){
		port[i] = 0;
	}	
	sprintf(port, "%u", global.socketPort);//�˿ں�ת��Ϊ�ַ���
	strcpy(creat_tcp, creat_tcp_top);//�ַ�������
	strcat(creat_tcp, (const char*)global.socketIP);
	strcat(creat_tcp, creat_tcp_tail);
	strcat(creat_tcp, (const char*)port);
	strcat(creat_tcp, creat_tcp_port);
	
	uart1_send((uint8_t*)creat_tcp, strlen(creat_tcp), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(153000);//�ȴ���ʱ150s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), common_ack, sizeof(common_ack) - 1) )//����"OK"
		{
			MSG*cmsg;
			err = getting_queue_ec20_cmd((void*)&cmsg, 3000);//�ȴ�������Ϣ
			if(err == pdTRUE)
			{
				if( !strncasecmp((char*)&(cmsg->buf[2]), tcp_direct_ack, sizeof(tcp_direct_ack) - 1) )//��������"+QIOPEN:0:0"
				{
					if(cmsg->buf[cmsg->len - 3] == '0')//���һ����Ч�ַ���0��ʾ����(��������ַ�Ϊ/r/n)
						return TRUE;//����ɹ�
					else
					{
						return 2;//���Ӵ���
					}
				}
				else
				{
					return 3;//���ش���
				}
			}
			else
			{
				return 4;//û�еڶ���Ӧ��
			}
		}
		else
		{
			return 5;//��һ�η��ش���
		}
	}
	return FALSE;//����Ӧ
}

/*
@���ܣ��ر�TCP����
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
@˵�����ɹ������͸��ģʽ���˳���������
*/
static uint8_t c4g_close_tcp(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)close_tcp, (sizeof(close_tcp) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(11000);//�ȴ���ʱ10s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//�رճɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯTCP����״̬
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
*/
static uint8_t c4g_chk_tcp(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_tcp, (sizeof(chk_tcp) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(15000);//�ȴ���ʱ15s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			if(rx->len > 6)//���TCP���Ӵ��ڣ��᷵�غܶ�����
			{
				//�����������
				uint8_t i, j;
				i = 12;
				for(j = 0;  rx->buf[i] != ',' && j < 2; i++, j++)
					ec20.connectID[j] = rx->buf[i];//1~16
				++i;//����','
				for(j = 0;  rx->buf[i] != ',' && j < 10; i++, j++)
					ec20.service_type[j] = rx->buf[i];//���ӷ�ʽ��TCP/UDP...
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
					ec20.ip_adress[j] = rx->buf[i];//����˵�ַ
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.remote_port[j] = rx->buf[i];//����˶˿ں�
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.local_port[j] = rx->buf[i];//�����˿ں�
				++i;
				ec20.socket_state = rx->buf[i++];//socket״̬
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 2; i++, j++)
					ec20.contextID[j] = rx->buf[i];//1~16
				++i;
				ec20.serverID = rx->buf[i++];//
				++i;
				ec20.access_mode = rx->buf[i++];//����ģʽ��1.���壬2.���ͣ�3.͸��
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 10; i++, j++)
					ec20.AT_port[j] = rx->buf[i];//����˿ڣ�UART1/USB
				
				if(ec20.socket_state == '4')//���ڹر�socket
					return 4;
				else
				if(ec20.socket_state == '1' 
				|| ec20.socket_state == '2'
				|| ec20.socket_state == '3')//socket����
					return 3;
			}
			
			return TRUE;//��ѯ�ɹ�,������Ҫ�������� TCP
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}


/*
@���ܣ��˳�͸��ģʽ
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
@˵�����ɹ������͸��ģʽ���˳���������
*/
static uint8_t c4g_exit_transpartent(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)exit_transpartent, (sizeof(exit_transpartent) -1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(1000);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), common_ack, sizeof(common_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//����ɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

//�����ʵ������㷢һ���ַ��������������ʽ����ֻҪ�лظ���˵�����ڡ�����ģʽ��
static uint8_t c4g_exit_transpartent2(void)
{
//	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)exit_transpartent2, (sizeof(exit_transpartent2) -1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(1000);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		return TRUE;//����ɹ�
	}
	return FALSE;
}

/*
@���ܣ�����ģʽ����͸��ģʽ
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
@˵�����ɹ������͸��ģʽ���˳���������
*/
static uint8_t c4g_enter_transparent(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)enter_transparent_mode, (sizeof(enter_transparent_mode) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(10000);//�ȴ���ʱ10s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), creat_tcp_back1, sizeof(creat_tcp_back1) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//�رճɹ�
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ���Ϊ͸��ģʽ
@����ֵ��ִ��������Ƿ���Խ�����һ��
@ע�⣺����ֵ��ǰ2���ֽ���"/r/n",���ԱȽ��ַ����Ļ���Ҫ�ӷ���ֵ��3���ֽڿ�ʼ
		sizeof(�����ַ���) == �ַ�������+'0'����������������ȽϵĻ���Ҫ��ȥ'0'������
		����ֵ"OK"�ڵ�����4���ֽڿ�ʼ
@˵�����ɹ������͸��ģʽ���˳���������
*/
static uint8_t c4g_change_transpartent(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)change_transparent_mode, (sizeof(change_transparent_mode) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(300);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), creat_tcp_back1, sizeof(creat_tcp_back1) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//�ѽ���͸��ģʽ
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ���ѯ������
@����ֵ��ִ��������Ƿ���Խ�����һ��
*/
static uint8_t c4g_check_Operater(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;
	
	uart1_send((uint8_t*)chk_Operator, (sizeof(chk_Operator) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(3000);//1.8s��ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//�Ƚ�����"OK"
		{
//			printf((char*)&rx->buf[14]);
//			printf("\r\n\r\n\r\n");
			//�ӵ�14�ֽڿ�ʼ�Ƚ���Ӫ������
//			strupr(&rx->buf[rx->len-4]);
			if( !strncasecmp((char*)&(rx->buf[14]), Operater1_ack, sizeof(Operater1_ack) - 1) )//���й��ƶ��Ƚ�
			{
				ec20.Operater = 1;//�ƶ�
				return TRUE;
			}
			if( !strncasecmp((char*)&(rx->buf[14]), Operater2_ack, sizeof(Operater2_ack) - 1) )//���й���ͨ�Ƚ�
			{
				ec20.Operater = 2;//��ͨ
				return TRUE;
			}
			if( !strncasecmp((char*)&(rx->buf[14]), Operater3_ack, sizeof(Operater3_ack) - 1) )//���й����űȽ�
			{
				ec20.Operater = 3;//����
				return TRUE;
			}
		}
	}
	return FALSE;
}

/**********************************************************************************************************
@˵����EC20���յ���remote���ݷ��͵��ͻ���
@���ߣ�������
@����ʱ�䣺2018.7.8
@�޸�ʱ�䣺
**********************************************************************************************************/
/*
@���ܣ������յ������ݣ������͵�stm32����2�����͵�����
@������rx����������ָ�룬������EC20�Դ�����
*/
static u8 send_to_stm32(MSG*rx)
{
	u16 i, j, err;
	u16 connectID;//�յ����ݵ�ͨ����
	u16 bit;	//��Ч���ݳ�����ռλ��(@������Ч���ݳ���1����ռλ1λ)
	u16 fac = 1;//��Ч���ݷŴ�ϵ��(@�ַ���תhex��Ҫ�õ�)
	u8 size = 0;//��Ч���ݳ���
	
	
	for(i = 16; rx->buf[i] != ','; i++);
	
	i++;//����','
	
	connectID = connectID;//�������������
	connectID = rx->buf[i++];//����ͨ��,�ַ���ʽ
	
	i++;//����','
	
	j = i;
	for(; !(rx->buf[i] == '\r' && rx->buf[i+1] == '\n'); i++)//������Ч���ݷŴ�ϵ��
	{
		if(i != j)
		fac *= 10;
	}
	
	bit = i - j;//��Ч���ݳ���ռ��λ��
	for(j = 0; j < bit; j++)//������Ч���ݳ���
	{
		size += (rx->buf[i - bit + j] - '0')*fac;
		fac /= 10;
	}
	
	i += 2;//����'\r\n'
	
	uart2_send(&rx->buf[i], size, send_to_stm32_callback);//������Ч���ݵ�����2
	err = user_sended_get(10*OS_TICKS_PER_SEC);
	if(err != pdTRUE)
	{
		printf("uart2 send err\r\n\r\n");	
	}
	msg_free(rx);//�ͷŵ�ǰ��Ϣ
	return TRUE;
}

/**********************************************************************************************************
@˵����stm32�յ��û����ݺ���ã�����������ݷ���
@���ߣ�������
@����ʱ�䣺2018.7.9
@�޸�ʱ�䣺
**********************************************************************************************************/

/*
@���ܣ�֪ͨEC20��ʼ��������
@������size�����ݳ���
*/
uint8_t send_commucation_head(u16 size)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;
	char tmp[5];
	char head[30];
	
	sprintf(tmp, "%u", size);//����ת��Ϊ�ַ���
	
	strcpy(head, send_head);//�ַ�������
	strcat(head, (const char*)tmp);
	strcat(head, "\r\n");
	
	uart1_send((uint8_t*)head, (sizeof(head) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(300);//�ȴ���ʱ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), send_head_ack, sizeof(send_head_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			return TRUE;//���Կ�ʼ������
		}
		else
		{
			//����������
			return 2;
		}
	}
	return FALSE;
}

/*
@���ܣ���ȡ���ݷ��ͽ��
@˵����������ֻ��ʾEC20���͵Ľ����+++������ʾ���͵�remote�ɹ�
*/
u8 sended_result(void)
{
	MSG*cmsg;
	u8 err;
	
	err = getting_queue_ec20_cmd((void*)&cmsg, 1000);//�ȴ�������Ϣ
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(cmsg->buf[2]), sended_data_ack, sizeof(sended_data_ack) - 1) )//��������"SEND OK"
		{
			return TRUE;//���ݷ��ͳɹ�
		}
		else
		{
			return 3;//���ش���
		}
	}
	return FALSE;//��ʱ
	
}

/*
@���ܣ���ѯ���ͳ��ȣ�+++�н����ʾ���͵�remote�ɹ�
@������len���ѷ������ݳ���
*/
u8 query_sended_result(u16 len)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	u8 err;
	
	uart1_send((uint8_t*)query_sended, (sizeof(query_sended) - 1), callback_ec20_cmd);//���ڷ���
	err = sem_ec20_cmd_get(90000);//�ȴ���ʱ90s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), query_sended_ack, sizeof(query_sended_ack) - 1) )//�ӵ�3���ַ���ʼ�Ƚ�,
		{
			if( !strncasecmp((char*)&(rx->buf[rx->len - 4]), common_ack, sizeof(common_ack) - 1) )//�������ַ�Ϊ"OK"
			{
				return TRUE;//���Կ�ʼ������
			}
			else
			{
				return 2;//�з��ͣ�����û����ȫ�ɹ�
			}
		}
		else
		{
			return 2;//������
		}
	}
	return FALSE;
}


/*
�ж��Ƿ����ߣ�
1.һ��ʱ��������¼ͨѶ��ʱʱ�䣬���Ƕ��ûͨ��
2.��tcp����Ҳ����
3.���ݷ��ͺ󣬲�ѯ�Ƿ񵽴������
*/

#define UPDATE_REBOOT_TIME			(ec20.reboot_timeout = xTaskGetTickCount())//����ʧ��ʱ��
#define JUDGE_REBOOT_TIME(X)		(xTaskGetTickCount() - ec20.reboot_timeout > OS_TICKS_PER_SEC*X)//�ж��Ƿ񳬹�ec20����ʱ��
#define CLEAR_REBOOT_COUNT			(ec20.reboot_count = 0)
#define JUDGE_REBOOT_COUNT(X)		(ec20.reboot_count++ > X)
/*
@���ܣ�ec20������
@˵����
*/
void ec20_recv_task(void)
{
	u8 rs;

	ec20.fsm = ST_PWR_ON;//��ʼ״̬��
	UPDATE_REBOOT_TIME;	//��ʼ��������ʱ
	CLEAR_REBOOT_COUNT;	//��ʼ����������
	
	while(1)
	{
		switch(ec20.fsm)
		{					
			case ST_PWR_ON:		//�ϵ�
//				c4g_power_off();
				c4g_power_on();
				ec20.fsm = ST_CLOSE_CMD_ECHO;//��ת->���رջ��ԡ�
			break;
			
			case ST_REBOOT:		//����
				c4g_power_reset();
				ec20.fsm = ST_CLOSE_CMD_ECHO;//��ת->���رջ��ԡ�
				UPDATE_REBOOT_TIME;
				CLEAR_REBOOT_COUNT;
				global.reboot_time_count = xTaskGetTickCount();//����ȫ�����ݷ��ͼ��ʱ��
			break;
			
			case ST_CLOSE_CMD_ECHO://�ر��������
				if(c4g_close_cmd_echo())
				{
					UPDATE_REBOOT_TIME;
					ec20.fsm = ST_CHK_SIM;
				}
				if(JUDGE_REBOOT_TIME(30))//��ʱ������
				{
					ec20.fsm = ST_REBOOT;
					printf("EC20 reboot\r\n\r\n");
				}
			break;
				
			case ST_CHK_SIM:	//��sim��������
				rs = c4g_chk_sim();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_SIGNAL;//��ת->"�ź�ǿ��"
					UPDATE_REBOOT_TIME;
				}else
				if(rs == 2)//δ�忨
				{
					printf("no sim\r\n");
					UPDATE_REBOOT_TIME;
				}
				else//����
				{
					if(JUDGE_REBOOT_TIME(20))//��ѯʱ�䳬��20s
					{
						printf("check sim timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_SIGNAL:	//��ѯ�ź�ǿ��
				rs = c4g_chk_signal();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_SIM_REG;//��ת->"sim��ע��"
					UPDATE_REBOOT_TIME;
				}
				else
				{
					if(JUDGE_REBOOT_TIME(20))//��ѯʱ�䳬��20s
					{
						printf("check signal timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_SIM_REG:	//��ѯsim��ע��
				rs = c4g_sim_reg();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_NET_REG;
					UPDATE_REBOOT_TIME;
				}
				else 
				if(rs == 2)
				{
					if(JUDGE_REBOOT_TIME(90))//��ѯʱ�䳬��90s
					{
						printf("check sim reg timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_NET_REG:	//��ѯ����ע��
				rs = c4g_net_reg();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_OPERATER;//��ת->"��ѯ��Ӫ��"
					UPDATE_REBOOT_TIME;
				}
				else
				if(rs == 2)
				{
					if(JUDGE_REBOOT_TIME(60))//��ѯʱ�䳬��60s
					{
						printf("check net reg timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_OPERATER:	//��ѯ��Ӫ��
				rs = c4g_check_Operater();
				if(rs == TRUE)
				{
					ec20.fsm = ST_SET_POINT;//��ת->"���ý����"
					UPDATE_REBOOT_TIME;
				}
				else
				{
					if(JUDGE_REBOOT_TIME(60))//��ѯʱ�䳬��60s
					{
						printf("Operater check err\r\n\r\n");//û�鵽��Ӫ��
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_SET_POINT:	//���ý���㣬APN
				rs = c4g_set_point();
				if(rs == TRUE)
				{
					ec20.fsm = ST_ACTIVE_POINT;//��ת->"��������"
					UPDATE_REBOOT_TIME;
				}
				else
				{
					if(JUDGE_REBOOT_TIME(60))//��ѯʱ�䳬��60s
					{
						printf("set apn err\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_POINT:	//��ѯ�����
				
			break;
			
			case ST_ACTIVE_POINT:	//��������
				rs = c4g_active_context();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CREAT_TCP;//��ת"����TCP"
					UPDATE_REBOOT_TIME;
					CLEAR_REBOOT_COUNT;
				}
				else
				if(rs == 2)//�յ����ݣ��������ݴ���
				{
					printf("active PDP Context err 1\r\n\r\n");
					ec20.fsm = ST_DEACTIVE_POINT;//��ת->"�رս����"
					UPDATE_REBOOT_TIME;
				}
				else//��ʱ(150s)δ�յ��ظ�
				{
					printf("active PDP Context err 2\r\n\r\n");
					ec20.fsm = ST_REBOOT;//��ת->"�����豸"
				}
			break;
				
			case ST_DEACTIVE_POINT:	//�رս����
				rs = c4g_deactive_context();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_SIM;//��ת->"��ѯsim��"
					CLEAR_REBOOT_COUNT;//�����������
					UPDATE_REBOOT_TIME;
				}
				else
				if(rs == 2)//�з��أ����������ݴ���
				{
					if(JUDGE_REBOOT_COUNT(3))//����4�Σ��������ʧ������������
					{
						printf("link point err 1\r\n\r\n");//���������ʧ��
						ec20.fsm = ST_REBOOT;//��ת->"�����豸"
					}
				}
				else//��ʱ��Ӧ��(40s)
				{
					printf("link point err 2\r\n\r\n");//���������ʧ��
					ec20.fsm = ST_REBOOT;//��ת->"�����豸"
				}
			break;
				
			case ST_CHK_CONTEXT:	//��ѯ������ı�
				rs = c4g_chk_context();
				if(rs == TRUE){//��ѯ�ɹ�������δ����
					ec20.fsm = ST_ACTIVE_POINT;
					UPDATE_REBOOT_TIME;
				}else
				if(rs == 3){//�Ѿ�����
					ec20.fsm = ST_CHK_TCP;//�Ȳ���TCP�����Ƿ���ڣ�������ڣ�ֱ�������������ڣ��򴴽�����
				}else
				if(rs == 2){//������Ϣ
					printf("context err 1\r\n\r\n");//��ѯ������ı�����
				}
			break;
				
			case ST_CREAT_TCP:	//����TCP����
				rs = c4g_creat_tcp();
				if(rs == TRUE)
				{
					ec20.fsm = ST_COMMUCATION;//��ת->"ͨ��"
					global.socket_state = 1;//tcp ���ӳɹ�
					CLEAR_REBOOT_COUNT;//�����������
					UPDATE_REBOOT_TIME;
				}
				else
				if(rs !=0 && rs != TRUE)//�з��أ��������ݴ���
				{
					if(JUDGE_REBOOT_COUNT(5))//����5�Σ��������ʧ������������
					{
						char tmp[2];
						sprintf(tmp, "%u", rs);
						printf("creat tcp err:");
						printf((const char*)tmp);
						printf("\r\n\r\n");
						ec20.fsm = ST_REBOOT;//��ת->"����"
					}
				}
				else//����tcp��ʱ(150s)
				{
					printf("creat tcp timout\r\n\r\n");
					ec20.fsm = ST_REBOOT;//��ת->"����"
				}
			break;
				
			case ST_CHK_TCP:	//��ѯTCP
				rs = c4g_chk_tcp();
				if(rs == TRUE)//û�н���TCP;��Ҫ���´���TCP����
				{
					ec20.fsm = ST_CREAT_TCP;//��ת->������TCP��
					UPDATE_REBOOT_TIME;
					CLEAR_REBOOT_COUNT;
				}
				else//********************
				if(rs == 3)//TCP�����Ѵ���
				{
					if( xTaskGetTickCount() - global.reboot_time_count > global.reboot_time_set )//����ϵͳ��ʱʱ��
					{
						global.reboot_time_count = xTaskGetTickCount();//����ʱ��
						ec20.fsm = ST_CLOSE_TCP;//��ת���ر�TCP��
						UPDATE_REBOOT_TIME;
					}
					else
					{
						ec20.fsm = ST_COMMUCATION;//��ת->"�ȴ���������"
						UPDATE_REBOOT_TIME;
					}
				}
				else
				if(rs == 4)//"���ڹر�TCP"
				{
					ec20.fsm = ST_CLOSE_TCP;//��ת���ر�TCP��
					UPDATE_REBOOT_TIME;
				}
				else//��ʱ��Ӧ��
				{
					printf("chk tcp timeout\r\n\r\n");//��ѯtcpʧ��
					ec20.fsm = ST_REBOOT;//��ת->"����"
				}
			break;
				
			case ST_CLOSE_TCP:		//�ر�TCP
				rs = c4g_close_tcp();
				if(rs == TRUE)//�ر�TCP�ɹ�
				{
					ec20.fsm = ST_CREAT_TCP;//��ת"����TCP"
					UPDATE_REBOOT_TIME;
					CLEAR_REBOOT_COUNT;
				}
				else
				if(rs == 2)//������Ϣ
				{
					if( JUDGE_REBOOT_COUNT(5) )//����5��
					{
						printf("close tcp err\r\n\r\n");
						ec20.fsm = ST_REBOOT;//��ת->"����"
					}
				}
				else
				{
					printf("close tcp timeout\r\n\r\n");//�ر�TCP��ʱ
					ec20.fsm = ST_REBOOT;//��ת->"����"
				}
			break;
				
			case ST_COMMUCATION://����ͨ��
			{
				MSG*cmsg;
				u8 err;
				
				err = getting_queue_ec20_remote((void*)&cmsg, 10*OS_TICKS_PER_SEC);//ÿ10s��һ��
				if(err == pdTRUE)//�յ�Զ������
				{
					send_to_stm32(cmsg);//ͨ��stm32 ���͵��ͻ���
					printf("send to user\r\n\r\n");
				}
				else
				{
					if(global.socket_state == 2)
					{
						printf("direct reboot\r\n\r\n");
						ec20.fsm = ST_REBOOT;//��ת->"����"
					}
					else
					if(global.socket_state == 0)
					{
						printf("transfer to close tcp\r\n\r\n");
						ec20.fsm = ST_CLOSE_TCP;//��ת->"�ر�TCP"
						UPDATE_REBOOT_TIME;
					}
				}
			}
			break;
			
			case ST_CHK_DATA_ECHO:	//��ѯ���ݻ���
				printf("idle \r\n\r\n");
			break;
			case ST_CLOSE_DATA_ECHO://�ر����ݻ���
				
			break;
			case ST_CHK_ERRCODE:	//��ѯ����Ĵ�����
				
			break;
			
		}
	}
}






