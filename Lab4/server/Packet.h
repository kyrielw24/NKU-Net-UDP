#pragma once

#pragma pack(1)
struct Packet_Header
{
	WORD datasize;		// 数据长度
	BYTE tag;			// 标签
	//八位，使用后四位，排列是OVER FIN ACK SYN 
	BYTE window;		// 窗口大小
	BYTE seq;			// 序列号
	BYTE ack;			// 确认号
	WORD checksum;		// 校验和

	// 初始化
	Packet_Header()
	{
		datasize = 0;
		tag = 0;
		window = 0;
		seq = 0;
		ack = 0;
		checksum = 0;
	}
};
#pragma pack(0)


const BYTE SYN = 0x1;		//SYN = 1 ACK = 0 FIN = 0
const BYTE ACK = 0x2;		//SYN = 0 ACK = 1 FIN = 0
const BYTE ACK_SYN = 0x3;	//SYN = 1 ACK = 1 FIN = 0
const BYTE FIN = 0x4;		//FIN = 1 ACK = 0 SYN = 0
const BYTE FIN_ACK = 0x6;	//FIN = 1 ACK = 1 SYN = 0
const BYTE OVER = 0x8;		//结束标志
const BYTE END = 0x16;		//全局结束标志