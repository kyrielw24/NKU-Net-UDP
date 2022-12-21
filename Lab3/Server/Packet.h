#pragma once

#pragma pack(1)
struct Packet_Header
{
	WORD datasize;		// ���ݳ���
	BYTE tag;			// ��ǩ
	//��λ��ʹ�ú���λ��������OVER FIN ACK SYN 
	BYTE window;		// ���ڴ�С
	BYTE seq;			// ���к�
	BYTE ack;			// ȷ�Ϻ�
	WORD checksum;		// У���

	// ��ʼ��
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
const BYTE OVER = 0x8;		//������־
const BYTE END = 0x16;		//ȫ�ֽ�����־