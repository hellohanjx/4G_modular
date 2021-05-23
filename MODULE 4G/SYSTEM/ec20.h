#ifndef _EC20_H_
#define _EC20_H_

#include "global.h"

//连网信息
typedef struct EC20{
	char rssi[3];	//MAX == 199,dBm
	char ber[2];	//MAX == 99,信道误码率
	char iccid[30]; //SIM卡iccid

	char code[15];	//"READY",不需要PIN码；其他...
	char n;			//状态，是否注册
	char stat;		//漫游等信息
	char lac[5];	//区域码
	char ci[5];		//cell id
	char act;		//网络制式

	char contextID[2];	//1~16
	char context_type;	//协议类型；1->IPV4
	char apn[15];		//apn
	char username[15];	//名字
	char password[15];	//密码
	char authentication;//认证 0~3
	char local_address[20];//本地ip地址
	char connectID[2];	//socket服务索引 0~11
	
	char context_state;	//接入点状态
	char ip_adress[20];	//服务端ip地址
	char service_type[10];	//服务方式：TCP，UDP，TCP LISTENER,TCP INCOMING,UDP SERVICE
	char remote_port[5];	//端口号
	char local_port[15];	//本机端口？
	char socket_state;		//socket状态，0~4
	char serverID;			//
	char access_mode;		//传输模式。0，缓冲，1，推送，2，透传
	char AT_port[10];		//at指令端口
	
	char err[10];			//错误码
	
	char fsm;//状态机
	u32 reboot_timeout;//重启计时
	u32	reboot_count;  //重启计数 
	u8 Operater;			//运营商：1，移动，2联通，3电信
}EC20;
	




void ec20_recv_task(void);
void ec20_config(void);
uint8_t send_commucation_head(u16 size);
u8 sended_result(void);
u8 query_sended_result(u16 len);

#endif
