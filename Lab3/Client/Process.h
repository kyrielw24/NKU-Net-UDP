#pragma once
#include "Packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include "Packet.h"
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

// ����У���
WORD compute_sum(WORD* message,int size) {   // size = 8
	int count = (size + 1) / 2;  // ��ֹ�����ֽ��� ȷ��WORD 16λ
    WORD* buf = (WORD*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, message, size);
    u_long sum = 0;
    while (count--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

// �ͻ��������˽������ӣ���ȡ�������ֵķ�ʽ
int Client_Server_Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, int label)
{
	Packet_Header packet;
	int size = sizeof(packet);  // size = 8
	char* buffer = new char[sizeof(packet)];  // ����buffer

	// ��һ�Σ��ͻ������������˷��ͽ�����������
	packet.tag = SYN;
	packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto�����������ݳ���" << endl;
        return 0;
    }

    // �趨��ʱ�ش�����
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // ������ģʽ
    clock_t start = clock(); //��¼���͵�һ������ʱ��

    // �ڶ��Σ��ͻ��˽��շ���˻ش�������
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketClient, buffer, size, 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "��һ�����ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }

    cout << "�ɹ����͵�һ��������Ϣ" << endl;

    // �Խ������ݽ���У��ͼ���
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "�ɹ��յ��ڶ���������Ϣ" << endl;
    }
    else {
        cout << "�޷����շ���˻ش������ɿ�����" << endl;
        return 0;
    }

    // �����Σ��ͻ��˷��͵�������������
    packet.tag = ACK_SYN;
    packet.datasize = label;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto�����������ݳ���" << endl;
        return 0;
    }

    cout << "�ɹ����͵�����������Ϣ" << endl;
    cout << "�ͻ��������˳ɹ������������ֽ������ӣ�---------���Կ�ʼ����/��������" << endl;

    mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);
	return 1;
}

int Client_Server_Size(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, int Size) {
    Packet_Header packet;
    int size = sizeof(packet);  // size = 8
    char* buffer = new char[sizeof(packet)];  // ����buffer

    // ��֪����� �ͻ��˵Ļ�����Ϊ���
    packet.tag = SYN;
    packet.datasize = Size;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto�����������ݳ���" << endl;
        return 0;
    }

    // �趨��ʱ�ش�����
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // ������ģʽ
    clock_t start = clock(); //��¼���͵�һ������ʱ��

    // ���շ���˻ش���֪�Ļ�������С
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketClient, buffer, size, 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "���ݷ��ͳ�ʱ�����ڽ����ش�" << endl;
        }
    }

    cout << "�ɹ���֪�Է���������С" << endl;

    // �Խ������ݽ���У��ͼ���
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "�ɹ��յ��Է���������С��Ϣ" << endl;
    }
    else {
        cout << "�޷����շ���˻ش������ɿ�����" << endl;
        return 0;
    }

    mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);  // ����ģʽ

    if (Size > packet.datasize)
        return packet.datasize;
    else
        return Size;
}

// �ͻ�����������˶Ͽ����ӣ���ȡ�Ĵλ��ֵķ�ʽ
int Client_Server_Disconnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // ����buffer
    int size = sizeof(packet);

    // �ͻ��˵�һ�η������
    packet.tag = FIN_ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, size);
    memcpy(buffer, &packet, size);
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto�����������ݳ���" << endl;
        return 0;
    }

    clock_t start = clock();   // ��¼��һ�λ��ֵ�ʱ��

    // �ͻ��˽��շ���˷����ĵڶ��λ���
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "��һ�����ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }

    cout << "�ɹ����͵�һ�λ�����Ϣ" << endl;

    // �Խ������ݽ���У��ͼ���
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "�ɹ��յ��ڶ��λ�����Ϣ" << endl;
    }
    else {
        cout << "�޷����շ���˻ش��Ͽ�����" << endl;
        return 0;
    }

    // �ͻ��˽��շ���˷����ĵ����λ���
    while (1) {
        if (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) == -1) {
            cout << "�޷����շ���˻ش��Ͽ�����" << endl;
            return 0;
        }
        // �Խ������ݽ���У��ͼ���
        memcpy(&packet, buffer, sizeof(packet));
        if ((packet.tag == FIN_ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "�ɹ��յ������λ�����Ϣ" << endl;
            break;
        }
    }

    // ���ĴΣ��ͻ��˷��͵��Ĵλ�������
    packet.tag = ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto�����������ݳ���" << endl;
        return 0;
    }

    cout << "�ɹ����͵��Ĵλ�����Ϣ" << endl;
    cout << "�ͻ��������˳ɹ��Ͽ����ӣ�" << endl;

    return 1;
}

// �������ݰ�
// ������������ݳ���

void Send_Message(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* Message, int mes_size, int MAX_SIZE)
{
    int packet_num = mes_size / (MAX_SIZE) + (mes_size % MAX_SIZE != 0);
    int Seq_num = 0;  //��ʼ�����к�
    Packet_Header packet;
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // ������ģʽ

    for (int i = 0; i < packet_num; i++)
    {
        int data_len = (i == packet_num - 1 ? mes_size - (packet_num - 1) * MAX_SIZE : MAX_SIZE);
        char* buffer = new char[sizeof(packet) + data_len];  // ����buffer
        packet.tag = 0;
        packet.seq = Seq_num;
        packet.datasize = data_len;
        packet.checksum = 0;
        packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
        memcpy(buffer, &packet, sizeof(packet));
        char* mes = Message + i * MAX_SIZE;
        memcpy(buffer + sizeof(packet), mes, data_len);

        // ��������
        sendto(socketClient, buffer, sizeof(packet)+data_len, 0, (sockaddr*)&servAddr, servAddrlen);
        cout << "Begin sending message...... datasize:" << data_len << " bytes!" << " flag:" 
            << int(packet.tag) << " seq:" << int(packet.seq) << " checksum:" << int(packet.checksum) << endl;
        
        clock_t start = clock();    //��¼����ʱ��

        while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
            if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
            {
                sendto(socketClient, buffer, sizeof(packet) + data_len, 0, (sockaddr*)&servAddr, servAddrlen);
                cout << "���ݷ��ͳ�ʱ�������ش�������" << endl;
                start = clock();
            }
        }

        memcpy(&packet, buffer, sizeof(packet));
        if (packet.ack == (Seq_num+1)%(256) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "Successful send and recv response  ack:"<<int(packet.ack) << endl;
        }
        else {
            // �����δ���ܵ����� �ش�����
            // ����˽��յ������� У��ͳ��� ��Ҫ�ش�
            if (packet.ack == Seq_num || (compute_sum((WORD*)&packet, sizeof(packet)) != 0)) {
                cout << "�����δ���ܵ����ݣ������ش�������" << endl;
                i--;
                continue;
            }
            else {
                cout << "�޷����շ���˻ش�" << endl;
                return;
            }
        }

        // Seq_num�� 0-255֮��
        Seq_num++;
        Seq_num = (Seq_num > 255 ? Seq_num - 256 : Seq_num);
    }

    //���ͽ�����־
    packet.tag = OVER;
    char* buffer = new char[sizeof(packet)];
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
    cout << "The end tag has been sent" << endl;

    clock_t start = clock();
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
            cout << "���ݷ��ͳ�ʱ�������ش�������" << endl;
            start = clock();
        }
    }

    memcpy(&packet, buffer, sizeof(packet));
    if (packet.tag == OVER && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "The end token was successfully received" << endl;
    }
    else {
        cout << "�޷����շ���˻ش�" << endl;
        return;
    }
    mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);  // ����ģʽ
    return;
}


// �ͻ��˽�������
int Recv_Messsage(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen, char* Mes, int MAX_SIZE)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet) + MAX_SIZE];  // ����buffer
    int ack = 1;  // ȷ�����к�
    int seq = 0;
    long F_Len = 0;     //�����ܳ�
    int S_Len = 0;      //�������ݳ���

    // �����ļ�
    while (1) {
        while (recvfrom(socketServer, buffer, sizeof(packet) + MAX_SIZE, 0, (sockaddr*)&clieAddr, &clieAddrlen) <= 0);
        memcpy(&packet, buffer, sizeof(packet));

        if (((rand() % (255 - 1)) + 1) == 199) {
            cout << int(packet.seq) << "�����ʱ�ش�����" << endl;
            continue;
        }

        // END ȫ�ֽ���
        if (packet.tag == END && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "The all_end tag has been recved" << endl;
            return 999;
        }

        // �������
        if (packet.tag == OVER && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "The end tag has been recved" << endl;
            break;
        }

        // ��������
        if (packet.tag == 0 && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {

            // �Խ������ݼ���ȷ�ϣ������ͱ�Ǹ�֪�ͻ�����Ҫ�ط�
            if (packet.seq != seq) {
                Packet_Header temp;
                temp.tag = 0;
                temp.ack = seq;
                temp.checksum = 0;
                temp.checksum = compute_sum((WORD*)&temp, sizeof(temp));
                memcpy(buffer, &temp, sizeof(temp));
                sendto(socketServer, buffer, sizeof(temp), 0, (sockaddr*)&clieAddr, clieAddrlen);
                cout << "Send a resend flag to the client" << endl;
                continue;
            }

            // �ɹ��յ�����
            S_Len = packet.datasize;
            cout << "Begin recving message...... datasize:" << S_Len << " bytes!" << " flag:"
                << int(packet.tag) << " seq:" << int(packet.seq) << " checksum:" << int(packet.checksum) << endl;
            memcpy(Mes + F_Len, buffer + sizeof(packet), S_Len);
            F_Len += S_Len;

            // ���� ��֪�ͻ����ѳɹ��յ�
            packet.tag = 0;
            packet.ack = ack++;
            packet.seq = seq++;
            packet.datasize = 0;
            packet.checksum = 0;
            packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
            memcpy(buffer, &packet, sizeof(packet));
            sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
            cout << "Successful recv and send response  ack : " << int(packet.ack) << endl;
            seq = (seq > 255 ? seq - 256 : seq);
            ack = (ack > 255 ? ack - 256 : ack);
        }
    }

    // ���ͽ�����־
    packet.tag = OVER;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
    cout << "The end tag has been sent" << endl;
    return F_Len;
}