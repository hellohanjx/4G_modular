/*
移远EC20，4g驱动
只有TCP/IP的驱动，不含语音和短信
@说明：4G有三个指示灯：
红灯：亮，开机；微亮，关机
黄灯：慢闪（熄灭时间长）,找网状态；慢闪（亮的时间长）待机；快闪，数据传输模式
蓝灯：亮，注册LTE网络
@说明：采用状态机架构来实现
@注意：每条指令返回除了数据外还有个"OK"
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


#define C4G_PWR		PBout(12)	//开/关机
#define C4G_RST		PBout(13)	//复位引脚
#define C4G_POWERON_TIME	300	//开机引脚保持时间（ms）
#define C4G_POWEROFF_TIME	800	//关机引脚保持时间（ms）
#define C4G_RESET_TIME		500	//复位引脚保持时间（ms）

//AT指令
 const char chk_baud[] = {"AT+IPR?\r\n"};				//查询波特率
 const char set_baud[] = {"AT+IPR=115200\r\n"};		//设置波特率
 const char chk_signal[] = {"AT+CSQ\r\n"};			//查询信号强度
 const char close_cmd_echo[] = {"ATE0\r\n"};			//关闭命令回显
 const char save_param[] = {"AT&W\r\n"};				//保存设置参数，应答超时300ms
 const char chk_iccid[] = {"AT+QCCID\r\n"};			//查询ccid

 const char chk_pin[] = {"AT+CPIN?\r\n"};				//查询sim卡密码(5s超时)
  const char chk_pin_back1[] = {"+CME ERROR: 10"};	//sim卡未插卡
  const char chk_pin_back2[] = {"+CPIN: READY"};		//sim卡准备好

 const char chk_sim_reg[] = {"AT+CREG?\r\n"};			//查询sim卡是否注册
 const char chk_net_reg[] = {"AT+CGREG?\r\n"};		//查询net注册
 
 //运营商APN
 const char set_link_ChinaMobile[] = {"AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n"};//设置接入点(移动) APN,USERNAME,PASSWORD
 const char set_link_UNICOM[] = {"AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n"};//设置接入点(联通) APN,USERNAME,PASSWORD
 const char set_link_p[] = {"AT+QICSGP=1,1,\"CTLTE\",\"\",\"\",1\r\n"};//设置接入点(电信) APN,USERNAME,PASSWORD
 
 const char chk_link_point[] = {"AT+QICSGP=1\r\n"};	//查询接入点 
 const char active_pdp_context[] = {"AT+QIACT=1\r\n"};//激活接入点(150s超时)
 const char chk_pdp_context[] = {"AT+QIACT?\r\n"};	//查询context,返回当前接入点信息(150s超时)
 const char deactive_pdp_context[] = {"AT+QIDEACT=1\r\n"};//关闭接入点（40s超时）
// const char creat_tcp[] = {"AT+QIOPEN=1,0,\"TCP\",\"120.55.115.113\",5008,0,2\r\n"};//创建TCP连接（150s）
//  const char creat_tcp[] = {"AT+QIOPEN=1,0,\"TCP\",\"120.26.204.86\",8001,0,2\r\n"};//创建TCP连接（150s）
 
const char creat_tcp_top[] = {"AT+QIOPEN=1,0,\"TCP\",\""};
const char creat_tcp_tail[] = {"\","};//创建TCP连接（150s）
const char creat_tcp_port[] = {",0,1\r\n"};//服务端端口号+传输模式【推送模式，非透传】
	 const char creat_tcp_back1[] = {"CONNECT"};//创建TCP成功,透传模式的回应
	
 const char close_tcp[] = {"AT+QICLOSE=0\r\n"};		//关闭TCP连接（这个可以自己设置超时）
 const char chk_tcp[] = {"AT+QISTATE=1,0\r\n"};		//查询TCP连接(@查询 指定的 通道 connectID)
 const char chk_data_echo[] = {"AT+QISDE?\r\n"};		//查询数据回显
//const char close_data_echo[] = {"AT+QISDE=0\r\n"};	//关闭数据回显（透传模式不需要）
 const char chk_err_code[] = {"AT+QIGETERROR\r\n"};	//查询最近的错误码

 const char exit_transpartent[] = {"+++"};		//退出透传模式
 const char exit_transpartent2[] = {"++++++++\r\n"};		//命令模式下的+++测试

 const char change_transparent_mode[] = {"AT+QISWTMD=1,2\r\n"};		//改为透传模式
 const char enter_transparent_mode[] = {"ATO\r\n"};		//进入透传模式

  const char common_ack[] = {"OK"};	//通用回复"OK"

  const char chk_Operator[] = {"AT+COPS?\r\n"};//查是什么卡，移动联通电信
	const char Operater1_ack[] = {"CHINA MOBILE"};//中国移动
	const char Operater2_ack[] = {"CHN-UNICOM"};//中国联通
	const char Operater3_ack[] = {"CHN-CT"};//中国电信
//	const char Operater1_ack[] = {"china mobile"};//中国移动
//	const char Operater2_ack[] = {"chn-unicom"};//中国联通
//	const char Operater3_ack[] = {"chn-ct"};//中国电信
//状态机状态

const char chk_signal_quality[] = {"AT+CSQ\r\n"};//查询信号强度
	const char chk_signal_ack[] = {"+CQS"};//查询信号强度返回值

	
const char remote_recv[] = {"+QIURC"};//服务器数据返回

const char tcp_direct_ack[] = {"+QIOPEN:"};//tcp 连接 direct push模式返回值


const char send_head[] = {"AT+QISEND=0,"};//通知ec20即将发送数据
	const char send_head_ack[] = {">"};
	const char sended_data_ack[] = {"SEND OK"};
const char query_sended[] = {"AT+QISEND=0,0\r\n"};//查询发送结果
	const char query_sended_ack[] = {"+QISEND:"};
	
/////////////////////////////////
//模拟发送包
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
	str[i++] = chk;//校验和
	
	
}






enum
{
	ST_PWR_ON = 0,		//上电
	ST_CHK_SIM = 1,		//查询sim卡
	ST_CHK_SIM_REG = 2,		//查询sim卡网络
	ST_CHK_NET_REG = 3,		//查询网络
	ST_SET_POINT = 4,	//设置接入点
	ST_CHK_POINT = 5,	//查询接入点
	ST_ACTIVE_POINT = 6,//激活接入点
	ST_DEACTIVE_POINT = 7,//关闭接入点
	ST_CHK_CONTEXT = 8,	//查询接入点文本
	ST_CREAT_TCP = 9,	//创建TCP连接
	ST_CLOSE_TCP = 10,	//关闭TCP
	ST_CHK_TCP = 11,	//查询TCP
	ST_CHK_DATA_ECHO = 12,//查询数据回显
	ST_CLOSE_DATA_ECHO = 13,//关闭数据回显
	ST_CHK_ERRCODE = 14,	//查询最近的错误码
	ST_EXIT_TRANSPARTENT = 15,//退出透传模式
	ST_CLOSE_CMD_ECHO = 16,	//关闭命令回显
	ST_REBOOT = 17,		//重启4G
	ST_CHANGE_TRANSPARENT = 18,	//改为透传模式
	ST_ENTER_TRANSPARENT = 19, //进入透传模式
	ST_CHK_OPERATER = 20,	//查询卡类型
	ST_CHK_SIGNAL = 21,	//查询信号强度
	ST_COMMUCATION = 22,	//通信流程
};


static volatile EC20 ec20;		//定义EC20数据结构体


/**********************************************************************************************************
@说明：EC20钩子函数(回调函数)
@作者：韩景潇
@创建时间：2018.7.7
@修改时间：
**********************************************************************************************************/


/*
@功能：EC20命令数据返回处理回调函数
@参数：rx，返回数据指针
@返回值：执行结果
*/
static uint8_t callback_ec20_cmd(MSG *rx)
{
	u8 rs = FALSE;
	if(rx->len != 0)
	{
		if(rx->buf[0] == 0x0D && rx->buf[1] == 0x0A)
		{
			memcpy((void*)cmd_rx_1th.buf, (const void*)rx->buf, rx->len);//拷贝到cmd消息;@这里注意下，不能是rx直接拷贝，而是rx.buf拷贝
			cmd_rx_1th.len = rx->len;
			sem_ec20_recv_send_isr();
			rs = TRUE;
		}
	}
	msg_free(rx);//释放当前消息
	return rs;
}
/*
@功能：EC20默认接收数据回调处理函数
@参数：rx，返回数据指针
@返回值：执行结果
@说明：二段命令数据与服务器返回数据都在这里处理
*/
uint8_t callback_ec20_deault(MSG *rx)
{
	u8 rs = FALSE;
	if(rx->len != 0)
	{
		if(rx->buf[0] == 0x0D && rx->buf[1] == 0x0A)
		{
			if( !strncasecmp((char*)&(rx->buf[2]), remote_recv, sizeof(remote_recv) - 1) )//服务器数据返回，前缀为 "+QIURC"
			{
				if( sending_queue_ec20_remote(rx) )//丢到队列中去，不清空当前接收缓冲
					rs = TRUE;
			}
			else//命令数据返回,即是2端命令回应ack
			{
				memcpy((void*)cmd_rx_2th.buf, (const void*)rx->buf, rx->len);//拷贝到cmd消息;@这里注意下，不能是rx直接拷贝，而是rx.buf拷贝
				cmd_rx_2th.len = rx->len;
				if( sending_queue_ec20_cmd((MSG*)&cmd_rx_2th) )//发送消息到队列
				{
					msg_free(rx);//释放当前消息
					rs = TRUE;
				}
			}	
		}
	}
	return rs;
}

/*
@功能：stm32发送完成回调函数
*/
u8 send_to_stm32_callback(void)
{
	user_sended_free_isr();	//释放信号量
	return TRUE;
}





/**********************************************************************************************************
@说明：EC20驱动
@作者：韩景潇
@创建时间：2018.7.6
@修改时间：
**********************************************************************************************************/

/*
功能：4g控制引脚初始化
*/
void ec20_config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphResetCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能GPIO时钟

	//PB13 -> 4G_RST, PB12 -> 4G_PWR
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_12;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//GPIO速度
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//初始化GPIO
	GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_12);		//默认都不打开
	
}



/*
@功能：开机
*/
static void c4g_power_on(void)
{
	C4G_PWR = 1;	//拉高
	vTaskDelay(C4G_POWERON_TIME);
	C4G_PWR = 0;	//拉低
}

/*
@功能：关机
*/
static void c4g_power_off(void)
{
	C4G_PWR = 1;	//拉高
	vTaskDelay(C4G_POWEROFF_TIME);
	C4G_PWR = 0;	//拉低
	vTaskDelay(39*OS_TICKS_PER_SEC);//关机需耗时35s
}

/*
@功能：复位
*/
static void c4g_power_reset(void)
{
	C4G_RST = 1;
	vTaskDelay(C4G_RESET_TIME);
	C4G_RST = 0;
	vTaskDelay(3000);

}

/*
@功能：关闭命令回显
@返回值：执行情况，是否可以进行下一步
*/
static uint8_t c4g_close_cmd_echo(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;
	
	uart1_send((uint8_t*)close_cmd_echo, (sizeof(close_cmd_echo) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(300);//300ms超时
	if(err == pdTRUE)
	{
		//返回“OK”
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从倒数第3个字符开始比较,
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
@功能：查询sim卡
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
*/
static uint8_t c4g_chk_sim(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_pin, (sizeof(chk_pin) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(5000);//等待超时 5s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), chk_pin_back2, sizeof(chk_pin_back2) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//返回为 "+CPIN: READY "表示sim卡准备好了
		}

		if( !strncasecmp((char*)&(rx->buf[2]), chk_pin_back1, sizeof(chk_pin_back1) - 1) )//从第3个字符开始比较
		{
			return 2;	//未插卡
		}
	}
	return FALSE;
}

/*
@功能：查询信号强度
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
*/
static uint8_t c4g_chk_signal(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_signal_quality, (sizeof(chk_signal_quality) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(320);//等待超时 300ms
	if(err == pdTRUE)
	{
		if(strncasecmp((char*)&(rx->buf[2]), chk_signal_ack, sizeof(chk_signal_ack) - 1) )//从第3个字符开始比较,
		{
			//挂起另一个等待任务
			//查询队列，等待OK值
			
			return TRUE;//返回为 "+CQS "表示查询成功
		}
	}
	return FALSE;
}

/*
@功能：查询sim卡注册
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_sim_reg(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_sim_reg, (sizeof(chk_sim_reg) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(300);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从倒数第3个字符开始比较,
		{
			//处理接收数据
			uint8_t i, j;
			i = 9;
			ec20.n = rx->buf[i++];//网络状态
			++i;//跳过','
			ec20.stat = rx->buf[i++];//漫游信息；1，表示本地网络
			if(ec20.n != '0')//如果允许未请求的结果码则会传来一些信息
			{
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
					ec20.lac[j] = rx->buf[i];//地址代码
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
					ec20.ci[j] = rx->buf[i];//cell id
				++i;
				ec20.act = rx->buf[i];//网络制式，gsm/gprs...
			}
			
			return TRUE;//sim卡注册成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：查询网络注册状态
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_net_reg(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_net_reg, (sizeof(chk_net_reg) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(300);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			//处理接收数据
			uint8_t i, j;
			i = 10;
			ec20.n = rx->buf[i++];//网络状态
			++i;//跳过','
			ec20.stat = rx->buf[i++];//漫游信息;1,表示本地网；5，表示漫游
			if(ec20.n != '0')
			{
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.lac[j] = rx->buf[i];//地址代码
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.ci[j] = rx->buf[i];//cell id
				++i;
				ec20.act = rx->buf[i];//网络制式，gsm/gprs...
			}
			
			return TRUE;//sim卡注册成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}


/*
@功能：查询网络接入点
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_chk_point(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_link_point, (sizeof(chk_link_point) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(300);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//比较最后的"OK"
		{
			//处理接收数据
			uint8_t i, j;
			for(i = 2;  rx->buf[i] != ',' && i < 20; i++)
				ec20.context_type = rx->buf[i];//地址IPv4
			i += 2;//跳过','
			for(j = 0;  rx->buf[i] != ',' && i < 20; i++, j++)
				ec20.apn[j] = rx->buf[i];//apn
			i += 2;
			for(j = 0;  rx->buf[i] != ',' && i < 20; i++, j++)
				ec20.username[j] = rx->buf[i];//用户名？
			i += 2;
			for(j = 0;  rx->buf[i] != ',' && i < 20; i++, j++)
				ec20.password[j] = rx->buf[i];//密码
			i += 2;
			for(; i != ',' && i < 20; i++)
				ec20.authentication = rx->buf[i];//认证0~3
			
			return TRUE;//查询成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}


/*
@功能：设置网络接入点
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_set_point(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	if(ec20.Operater == 1)
		uart1_send((uint8_t*)set_link_ChinaMobile, (sizeof(set_link_ChinaMobile) - 1), callback_ec20_cmd);//串口发送
	else 
	if(ec20.Operater == 2)
		uart1_send((uint8_t*)set_link_UNICOM, (sizeof(set_link_UNICOM) - 1), callback_ec20_cmd);//串口发送
	else 
	if(ec20.Operater == 3)
		uart1_send((uint8_t*)set_link_p, (sizeof(set_link_p) - 1), callback_ec20_cmd);//串口发送
	else
		return FALSE;
	
	err = sem_ec20_cmd_get(300);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//设置接入点成功
		}
	}
	return FALSE;
}

/*
@功能：查询网络接入点
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_chk_context(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_pdp_context, (sizeof(chk_pdp_context) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(15000);//等待超时15s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			//处理接收数据
			uint8_t i, j;
			i = 12;
			ec20.context_state = rx->buf[i++];//接入点状态
			++i;//跳过','
			ec20.context_type = rx->buf[i++];//接入协议类型
			++i;
			for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
				ec20.local_address[j] = rx->buf[i];//本地ip地址
			
			if(ec20.context_state == '1')//已经激活，不需要再次激活
				return 3;
			return TRUE;//查询成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：激活网络接入点
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_active_context(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)active_pdp_context, (sizeof(active_pdp_context) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(152000);//等待超时150s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//激活成功
		}
		else
		{
			return 2;//收到数据但是数据错误
		}
	}
	return FALSE;
}

/*
@功能：关闭网络接入点
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_deactive_context(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)deactive_pdp_context, (sizeof(deactive_pdp_context) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(41000);//等待超时40s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//激活成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：创建TCP连接
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
@说明：成功后进入透传模式；退出此主任务
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
	sprintf(port, "%u", global.socketPort);//端口号转换为字符串
	strcpy(creat_tcp, creat_tcp_top);//字符串拷贝
	strcat(creat_tcp, (const char*)global.socketIP);
	strcat(creat_tcp, creat_tcp_tail);
	strcat(creat_tcp, (const char*)port);
	strcat(creat_tcp, creat_tcp_port);
	
	uart1_send((uint8_t*)creat_tcp, strlen(creat_tcp), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(153000);//等待超时150s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), common_ack, sizeof(common_ack) - 1) )//返回"OK"
		{
			MSG*cmsg;
			err = getting_queue_ec20_cmd((void*)&cmsg, 3000);//等待队列消息
			if(err == pdTRUE)
			{
				if( !strncasecmp((char*)&(cmsg->buf[2]), tcp_direct_ack, sizeof(tcp_direct_ack) - 1) )//正常返回"+QIOPEN:0:0"
				{
					if(cmsg->buf[cmsg->len - 3] == '0')//最后一个有效字符：0表示正常(最后两个字符为/r/n)
						return TRUE;//激活成功
					else
					{
						return 2;//连接错误
					}
				}
				else
				{
					return 3;//返回错误
				}
			}
			else
			{
				return 4;//没有第二段应答
			}
		}
		else
		{
			return 5;//第一段返回错误
		}
	}
	return FALSE;//无响应
}

/*
@功能：关闭TCP连接
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
@说明：成功后进入透传模式；退出此主任务
*/
static uint8_t c4g_close_tcp(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)close_tcp, (sizeof(close_tcp) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(11000);//等待超时10s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//关闭成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：查询TCP连接状态
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
*/
static uint8_t c4g_chk_tcp(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)chk_tcp, (sizeof(chk_tcp) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(15000);//等待超时15s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			if(rx->len > 6)//如果TCP连接存在，会返回很多数据
			{
				//处理接收数据
				uint8_t i, j;
				i = 12;
				for(j = 0;  rx->buf[i] != ',' && j < 2; i++, j++)
					ec20.connectID[j] = rx->buf[i];//1~16
				++i;//跳过','
				for(j = 0;  rx->buf[i] != ',' && j < 10; i++, j++)
					ec20.service_type[j] = rx->buf[i];//连接方式，TCP/UDP...
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 20; i++, j++)
					ec20.ip_adress[j] = rx->buf[i];//服务端地址
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.remote_port[j] = rx->buf[i];//服务端端口号
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 5; i++, j++)
					ec20.local_port[j] = rx->buf[i];//本机端口号
				++i;
				ec20.socket_state = rx->buf[i++];//socket状态
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 2; i++, j++)
					ec20.contextID[j] = rx->buf[i];//1~16
				++i;
				ec20.serverID = rx->buf[i++];//
				++i;
				ec20.access_mode = rx->buf[i++];//传输模式；1.缓冲，2.推送，3.透传
				++i;
				for(j = 0;  rx->buf[i] != ',' && j < 10; i++, j++)
					ec20.AT_port[j] = rx->buf[i];//命令端口，UART1/USB
				
				if(ec20.socket_state == '4')//正在关闭socket
					return 4;
				else
				if(ec20.socket_state == '1' 
				|| ec20.socket_state == '2'
				|| ec20.socket_state == '3')//socket存在
					return 3;
			}
			
			return TRUE;//查询成功,但是需要重新连接 TCP
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}


/*
@功能：退出透传模式
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
@说明：成功后进入透传模式；退出此主任务
*/
static uint8_t c4g_exit_transpartent(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)exit_transpartent, (sizeof(exit_transpartent) -1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(1000);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), common_ack, sizeof(common_ack) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//激活成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

//这个其实就是随便发一个字符串（按照命令格式），只要有回复就说明是在“命令模式”
static uint8_t c4g_exit_transpartent2(void)
{
//	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)exit_transpartent2, (sizeof(exit_transpartent2) -1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(1000);//等待超时
	if(err == pdTRUE)
	{
		return TRUE;//激活成功
	}
	return FALSE;
}

/*
@功能：命令模式进入透传模式
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
@说明：成功后进入透传模式；退出此主任务
*/
static uint8_t c4g_enter_transparent(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)enter_transparent_mode, (sizeof(enter_transparent_mode) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(10000);//等待超时10s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), creat_tcp_back1, sizeof(creat_tcp_back1) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//关闭成功
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：改为透传模式
@返回值：执行情况，是否可以进行下一步
@注意：返回值的前2个字节是"/r/n",所以比较字符串的话需要从返回值第3个字节开始
		sizeof(数组字符串) == 字符串长度+'0'结束符；所以这里比较的话需要减去'0'结束符
		返回值"OK"在倒数第4个字节开始
@说明：成功后进入透传模式；退出此主任务
*/
static uint8_t c4g_change_transpartent(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;

	uart1_send((uint8_t*)change_transparent_mode, (sizeof(change_transparent_mode) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(300);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), creat_tcp_back1, sizeof(creat_tcp_back1) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//已进入透传模式
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：查询卡类型
@返回值：执行情况，是否可以进行下一步
*/
static uint8_t c4g_check_Operater(void)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;
	
	uart1_send((uint8_t*)chk_Operator, (sizeof(chk_Operator) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(3000);//1.8s超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[rx->len-4]), common_ack, sizeof(common_ack) - 1) )//比较最后的"OK"
		{
//			printf((char*)&rx->buf[14]);
//			printf("\r\n\r\n\r\n");
			//从第14字节开始比较运营商名称
//			strupr(&rx->buf[rx->len-4]);
			if( !strncasecmp((char*)&(rx->buf[14]), Operater1_ack, sizeof(Operater1_ack) - 1) )//与中国移动比较
			{
				ec20.Operater = 1;//移动
				return TRUE;
			}
			if( !strncasecmp((char*)&(rx->buf[14]), Operater2_ack, sizeof(Operater2_ack) - 1) )//与中国联通比较
			{
				ec20.Operater = 2;//联通
				return TRUE;
			}
			if( !strncasecmp((char*)&(rx->buf[14]), Operater3_ack, sizeof(Operater3_ack) - 1) )//与中国电信比较
			{
				ec20.Operater = 3;//电信
				return TRUE;
			}
		}
	}
	return FALSE;
}

/**********************************************************************************************************
@说明：EC20接收到的remote数据发送到客户端
@作者：韩景潇
@创建时间：2018.7.8
@修改时间：
**********************************************************************************************************/
/*
@功能：解析收到的数据，并发送到stm32串口2，发送到出口
@参数：rx，接收数据指针，包含了EC20自带数据
*/
static u8 send_to_stm32(MSG*rx)
{
	u16 i, j, err;
	u16 connectID;//收到数据的通道号
	u16 bit;	//有效数据长度所占位数(@例如有效数据长度1，则占位1位)
	u16 fac = 1;//有效数据放大系数(@字符串转hex需要用到)
	u8 size = 0;//有效数据长度
	
	
	for(i = 16; rx->buf[i] != ','; i++);
	
	i++;//跳过','
	
	connectID = connectID;//解决编译器报错
	connectID = rx->buf[i++];//计算通道,字符格式
	
	i++;//跳过','
	
	j = i;
	for(; !(rx->buf[i] == '\r' && rx->buf[i+1] == '\n'); i++)//计算有效数据放大系数
	{
		if(i != j)
		fac *= 10;
	}
	
	bit = i - j;//有效数据长度占用位数
	for(j = 0; j < bit; j++)//计算有效数据长度
	{
		size += (rx->buf[i - bit + j] - '0')*fac;
		fac /= 10;
	}
	
	i += 2;//跳过'\r\n'
	
	uart2_send(&rx->buf[i], size, send_to_stm32_callback);//发送有效数据到串口2
	err = user_sended_get(10*OS_TICKS_PER_SEC);
	if(err != pdTRUE)
	{
		printf("uart2 send err\r\n\r\n");	
	}
	msg_free(rx);//释放当前消息
	return TRUE;
}

/**********************************************************************************************************
@说明：stm32收到用户数据后调用，用以完成数据发送
@作者：韩景潇
@创建时间：2018.7.9
@修改时间：
**********************************************************************************************************/

/*
@功能：通知EC20开始发送数据
@参数：size，数据长度
*/
uint8_t send_commucation_head(u16 size)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	uint8_t err;
	char tmp[5];
	char head[30];
	
	sprintf(tmp, "%u", size);//长度转换为字符串
	
	strcpy(head, send_head);//字符串拷贝
	strcat(head, (const char*)tmp);
	strcat(head, "\r\n");
	
	uart1_send((uint8_t*)head, (sizeof(head) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(300);//等待超时
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), send_head_ack, sizeof(send_head_ack) - 1) )//从第3个字符开始比较,
		{
			return TRUE;//可以开始发数据
		}
		else
		{
			//处理错误代码
			return 2;
		}
	}
	return FALSE;
}

/*
@功能：获取数据发送结果
@说明：这个结果只表示EC20发送的结果，+++并不表示发送到remote成功
*/
u8 sended_result(void)
{
	MSG*cmsg;
	u8 err;
	
	err = getting_queue_ec20_cmd((void*)&cmsg, 1000);//等待队列消息
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(cmsg->buf[2]), sended_data_ack, sizeof(sended_data_ack) - 1) )//正常返回"SEND OK"
		{
			return TRUE;//数据发送成功
		}
		else
		{
			return 3;//返回错误
		}
	}
	return FALSE;//超时
	
}

/*
@功能：查询发送长度，+++有结果表示发送到remote成功
@参数：len，已发送数据长度
*/
u8 query_sended_result(u16 len)
{
	MSG* rx = (MSG*)&cmd_rx_1th;
	u8 err;
	
	uart1_send((uint8_t*)query_sended, (sizeof(query_sended) - 1), callback_ec20_cmd);//串口发送
	err = sem_ec20_cmd_get(90000);//等待超时90s
	if(err == pdTRUE)
	{
		if( !strncasecmp((char*)&(rx->buf[2]), query_sended_ack, sizeof(query_sended_ack) - 1) )//从第3个字符开始比较,
		{
			if( !strncasecmp((char*)&(rx->buf[rx->len - 4]), common_ack, sizeof(common_ack) - 1) )//最后结束字符为"OK"
			{
				return TRUE;//可以开始发数据
			}
			else
			{
				return 2;//有发送，但是没有完全成功
			}
		}
		else
		{
			return 2;//错数据
		}
	}
	return FALSE;
}


/*
判断是否在线：
1.一个时间用来记录通讯超时时间，就是多久没通信
2.查tcp连接也可以
3.数据发送后，查询是否到达服务器
*/

#define UPDATE_REBOOT_TIME			(ec20.reboot_timeout = xTaskGetTickCount())//更新失联时间
#define JUDGE_REBOOT_TIME(X)		(xTaskGetTickCount() - ec20.reboot_timeout > OS_TICKS_PER_SEC*X)//判断是否超过ec20重启时间
#define CLEAR_REBOOT_COUNT			(ec20.reboot_count = 0)
#define JUDGE_REBOOT_COUNT(X)		(ec20.reboot_count++ > X)
/*
@功能：ec20主任务
@说明：
*/
void ec20_recv_task(void)
{
	u8 rs;

	ec20.fsm = ST_PWR_ON;//初始状态机
	UPDATE_REBOOT_TIME;	//初始化重启计时
	CLEAR_REBOOT_COUNT;	//初始化重启计数
	
	while(1)
	{
		switch(ec20.fsm)
		{					
			case ST_PWR_ON:		//上电
//				c4g_power_off();
				c4g_power_on();
				ec20.fsm = ST_CLOSE_CMD_ECHO;//跳转->“关闭回显”
			break;
			
			case ST_REBOOT:		//重启
				c4g_power_reset();
				ec20.fsm = ST_CLOSE_CMD_ECHO;//跳转->“关闭回显”
				UPDATE_REBOOT_TIME;
				CLEAR_REBOOT_COUNT;
				global.reboot_time_count = xTaskGetTickCount();//更新全局数据发送间隔时间
			break;
			
			case ST_CLOSE_CMD_ECHO://关闭命令回显
				if(c4g_close_cmd_echo())
				{
					UPDATE_REBOOT_TIME;
					ec20.fsm = ST_CHK_SIM;
				}
				if(JUDGE_REBOOT_TIME(30))//超时则重启
				{
					ec20.fsm = ST_REBOOT;
					printf("EC20 reboot\r\n\r\n");
				}
			break;
				
			case ST_CHK_SIM:	//查sim卡，物理卡
				rs = c4g_chk_sim();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_SIGNAL;//跳转->"信号强度"
					UPDATE_REBOOT_TIME;
				}else
				if(rs == 2)//未插卡
				{
					printf("no sim\r\n");
					UPDATE_REBOOT_TIME;
				}
				else//重启
				{
					if(JUDGE_REBOOT_TIME(20))//查询时间超过20s
					{
						printf("check sim timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_SIGNAL:	//查询信号强度
				rs = c4g_chk_signal();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_SIM_REG;//跳转->"sim卡注册"
					UPDATE_REBOOT_TIME;
				}
				else
				{
					if(JUDGE_REBOOT_TIME(20))//查询时间超过20s
					{
						printf("check signal timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_SIM_REG:	//查询sim卡注册
				rs = c4g_sim_reg();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_NET_REG;
					UPDATE_REBOOT_TIME;
				}
				else 
				if(rs == 2)
				{
					if(JUDGE_REBOOT_TIME(90))//查询时间超过90s
					{
						printf("check sim reg timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_NET_REG:	//查询网络注册
				rs = c4g_net_reg();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_OPERATER;//跳转->"查询运营商"
					UPDATE_REBOOT_TIME;
				}
				else
				if(rs == 2)
				{
					if(JUDGE_REBOOT_TIME(60))//查询时间超过60s
					{
						printf("check net reg timeout\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_OPERATER:	//查询运营商
				rs = c4g_check_Operater();
				if(rs == TRUE)
				{
					ec20.fsm = ST_SET_POINT;//跳转->"设置接入点"
					UPDATE_REBOOT_TIME;
				}
				else
				{
					if(JUDGE_REBOOT_TIME(60))//查询时间超过60s
					{
						printf("Operater check err\r\n\r\n");//没查到运营商
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_SET_POINT:	//设置接入点，APN
				rs = c4g_set_point();
				if(rs == TRUE)
				{
					ec20.fsm = ST_ACTIVE_POINT;//跳转->"激活接入点"
					UPDATE_REBOOT_TIME;
				}
				else
				{
					if(JUDGE_REBOOT_TIME(60))//查询时间超过60s
					{
						printf("set apn err\r\n\r\n");
						ec20.fsm = ST_REBOOT;
					}
				}
			break;
				
			case ST_CHK_POINT:	//查询接入点
				
			break;
			
			case ST_ACTIVE_POINT:	//激活接入点
				rs = c4g_active_context();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CREAT_TCP;//跳转"创建TCP"
					UPDATE_REBOOT_TIME;
					CLEAR_REBOOT_COUNT;
				}
				else
				if(rs == 2)//收到数据，但是数据错误
				{
					printf("active PDP Context err 1\r\n\r\n");
					ec20.fsm = ST_DEACTIVE_POINT;//跳转->"关闭接入点"
					UPDATE_REBOOT_TIME;
				}
				else//超时(150s)未收到回复
				{
					printf("active PDP Context err 2\r\n\r\n");
					ec20.fsm = ST_REBOOT;//跳转->"重启设备"
				}
			break;
				
			case ST_DEACTIVE_POINT:	//关闭接入点
				rs = c4g_deactive_context();
				if(rs == TRUE)
				{
					ec20.fsm = ST_CHK_SIM;//跳转->"查询sim卡"
					CLEAR_REBOOT_COUNT;//清除重启计数
					UPDATE_REBOOT_TIME;
				}
				else
				if(rs == 2)//有返回，但返回数据错误
				{
					if(JUDGE_REBOOT_COUNT(3))//最多查4次，如果还是失败则重启设置
					{
						printf("link point err 1\r\n\r\n");//接入点设置失败
						ec20.fsm = ST_REBOOT;//跳转->"重启设备"
					}
				}
				else//超时无应答(40s)
				{
					printf("link point err 2\r\n\r\n");//接入点设置失败
					ec20.fsm = ST_REBOOT;//跳转->"重启设备"
				}
			break;
				
			case ST_CHK_CONTEXT:	//查询接入点文本
				rs = c4g_chk_context();
				if(rs == TRUE){//查询成功，但是未激活
					ec20.fsm = ST_ACTIVE_POINT;
					UPDATE_REBOOT_TIME;
				}else
				if(rs == 3){//已经激活
					ec20.fsm = ST_CHK_TCP;//先查下TCP连接是否存在，如果存在，直接连网；不存在，则创建连接
				}else
				if(rs == 2){//错误信息
					printf("context err 1\r\n\r\n");//查询接入点文本错误
				}
			break;
				
			case ST_CREAT_TCP:	//创建TCP连接
				rs = c4g_creat_tcp();
				if(rs == TRUE)
				{
					ec20.fsm = ST_COMMUCATION;//跳转->"通信"
					global.socket_state = 1;//tcp 连接成功
					CLEAR_REBOOT_COUNT;//清除重启计数
					UPDATE_REBOOT_TIME;
				}
				else
				if(rs !=0 && rs != TRUE)//有返回，返回数据错误
				{
					if(JUDGE_REBOOT_COUNT(5))//最多查5次，如果还是失败则重启设置
					{
						char tmp[2];
						sprintf(tmp, "%u", rs);
						printf("creat tcp err:");
						printf((const char*)tmp);
						printf("\r\n\r\n");
						ec20.fsm = ST_REBOOT;//跳转->"重启"
					}
				}
				else//创建tcp超时(150s)
				{
					printf("creat tcp timout\r\n\r\n");
					ec20.fsm = ST_REBOOT;//跳转->"重启"
				}
			break;
				
			case ST_CHK_TCP:	//查询TCP
				rs = c4g_chk_tcp();
				if(rs == TRUE)//没有建立TCP;需要重新创建TCP连接
				{
					ec20.fsm = ST_CREAT_TCP;//跳转->“创建TCP”
					UPDATE_REBOOT_TIME;
					CLEAR_REBOOT_COUNT;
				}
				else//********************
				if(rs == 3)//TCP连接已存在
				{
					if( xTaskGetTickCount() - global.reboot_time_count > global.reboot_time_set )//超过系统超时时间
					{
						global.reboot_time_count = xTaskGetTickCount();//更新时间
						ec20.fsm = ST_CLOSE_TCP;//跳转“关闭TCP”
						UPDATE_REBOOT_TIME;
					}
					else
					{
						ec20.fsm = ST_COMMUCATION;//跳转->"等待接收数据"
						UPDATE_REBOOT_TIME;
					}
				}
				else
				if(rs == 4)//"正在关闭TCP"
				{
					ec20.fsm = ST_CLOSE_TCP;//跳转“关闭TCP”
					UPDATE_REBOOT_TIME;
				}
				else//超时无应答
				{
					printf("chk tcp timeout\r\n\r\n");//查询tcp失败
					ec20.fsm = ST_REBOOT;//跳转->"重启"
				}
			break;
				
			case ST_CLOSE_TCP:		//关闭TCP
				rs = c4g_close_tcp();
				if(rs == TRUE)//关闭TCP成功
				{
					ec20.fsm = ST_CREAT_TCP;//跳转"创建TCP"
					UPDATE_REBOOT_TIME;
					CLEAR_REBOOT_COUNT;
				}
				else
				if(rs == 2)//错误信息
				{
					if( JUDGE_REBOOT_COUNT(5) )//最多查5次
					{
						printf("close tcp err\r\n\r\n");
						ec20.fsm = ST_REBOOT;//跳转->"重启"
					}
				}
				else
				{
					printf("close tcp timeout\r\n\r\n");//关闭TCP超时
					ec20.fsm = ST_REBOOT;//跳转->"重启"
				}
			break;
				
			case ST_COMMUCATION://数据通信
			{
				MSG*cmsg;
				u8 err;
				
				err = getting_queue_ec20_remote((void*)&cmsg, 10*OS_TICKS_PER_SEC);//每10s查一次
				if(err == pdTRUE)//收到远端数据
				{
					send_to_stm32(cmsg);//通过stm32 发送到客户端
					printf("send to user\r\n\r\n");
				}
				else
				{
					if(global.socket_state == 2)
					{
						printf("direct reboot\r\n\r\n");
						ec20.fsm = ST_REBOOT;//跳转->"重启"
					}
					else
					if(global.socket_state == 0)
					{
						printf("transfer to close tcp\r\n\r\n");
						ec20.fsm = ST_CLOSE_TCP;//跳转->"关闭TCP"
						UPDATE_REBOOT_TIME;
					}
				}
			}
			break;
			
			case ST_CHK_DATA_ECHO:	//查询数据回显
				printf("idle \r\n\r\n");
			break;
			case ST_CLOSE_DATA_ECHO://关闭数据回显
				
			break;
			case ST_CHK_ERRCODE:	//查询最近的错误码
				
			break;
			
		}
	}
}






