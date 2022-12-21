#pragma once
#include "Packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include "Packet.h"
#include <string.h>
#include <queue>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

// ����У���
WORD compute_sum(WORD* message, int size) {   // size = 8
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
int Client_Server_Connect(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // ����buffer

    // ��һ�Σ�����˽��տͻ��˷��͵���������
    while (1) {
        if (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) == -1) {
            cout << "�޷����տͻ��˷��͵���������" << endl;
            return -1;
        }
        memcpy(&packet, buffer, sizeof(packet));
        if (packet.tag == SYN && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "�ɹ����յ�һ��������Ϣ" << endl;
            break;
        }
    }

    // �ڶ��Σ��������ͻ��˷���������Ϣ
    packet.tag = ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen) == -1)
    {
        return -1;
    }
    u_long mode = 1;
    ioctlsocket(socketServer, FIONBIO, &mode);  // ������ģʽ
    clock_t start = clock();//��¼�ڶ������ַ���ʱ��

    // �����Σ�����˽��տͻ��˷��͵�������Ϣ
    while (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
            start = clock();
            cout << "�ڶ������ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }
    mode = 0;
    ioctlsocket(socketServer, FIONBIO, &mode);  // ����ģʽ

    cout << "�ɹ����͵ڶ���������Ϣ" << endl;

    // �Խ������ݽ���У��ͼ���
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK_SYN) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "�ɹ��յ�������������Ϣ" << endl;
    }
    else {
        cout << "�޷����տͻ��˻ش������ɿ�����" << endl;
        return -1;
    }

    cout << "�ͻ��������˳ɹ������������ֽ������ӣ�---------���Կ�ʼ����/��������" << endl;

    return int(packet.datasize);
}

int Client_Server_Size(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, int size, int ssthresh, int*& A) {
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // ����buffer

    // ���տͻ��˸�֪�Ļ��������ݴ�С
    while (1) {
        if (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) == -1) {
            cout << "�޷����տͻ��˷��͵���������" << endl;
            return -1;
        }
        memcpy(&packet, buffer, sizeof(packet));
        if (packet.tag == SYN && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "�ɹ��յ��Է����ݷ��ͻ������Լ���ʼ��ֵ��С��Ϣ" << endl;
            break;
        }
    }
    int client_size = packet.datasize;
    int client_win = packet.window;

    // ����˸�֪�ͻ��˻�������С��Ϣ
    packet.tag = ACK;
    packet.datasize = size;
    packet.window = ssthresh;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        return -1;
    }
    cout << "�ɹ���֪�Է����ݷ��ͻ������Լ���ʼ��ֵ��С��Ϣ" << endl;

    A[0] = (size > client_size) ? client_size : size;
    A[1] = (ssthresh > client_win) ? client_win : ssthresh;
    return 1;
}

// �ͻ�����������˶Ͽ����ӣ���ȡ�Ĵλ��ֵķ�ʽ
int Client_Server_Disconnect(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // ����buffer
    int size = sizeof(packet);

    // ���տͻ��˷����ĵ�һ�λ�����Ϣ
    while (1) {
        if (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) == -1) {
            cout << "�޷����տͻ��˷��͵���������" << endl;
            return 0;
        }
        memcpy(&packet, buffer, sizeof(packet));
        if (packet.tag == FIN_ACK && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "�ɹ��յ���һ�λ�����Ϣ" << endl;
            break;
        }
    }

    // �ڶ��Σ��������ͻ��˷��ͻ�����Ϣ
    packet.tag = ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen) == -1)
    {
        return 0;
    }

    cout << "�ɹ����͵ڶ��λ�����Ϣ" << endl;
    //Sleep(10000);

    // �����Σ��������ͻ��˷��ͻ�����Ϣ
    packet.tag = FIN_ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen) == -1)
    {
        return 0;
    }
    clock_t start = clock();//��¼�����λ��ַ���ʱ��

    // ���ĴΣ�����˽��տͻ��˷��͵Ļ�����Ϣ
    while (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //ʱ�䳬��1�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
            start = clock();
            cout << "�����λ��ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }

    cout << "�ɹ����͵����λ�����Ϣ" << endl;

    // �Խ������ݽ���У��ͼ���
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "�ɹ��յ����Ĵλ�����Ϣ" << endl;
    }
    else {
        cout << "�޷����տͻ��˻ش������ɿ�����" << endl;
        return 0;
    }

    cout << "�ͻ��������˳ɹ��Ͽ����ӣ�" << endl;

    return 1;
}

// �̲߳���
struct Parameters
{
    SOCKET socketClient;
    SOCKADDR_IN serverAddr;
    char* Message;
    int mes_size;

};

// ���̡߳������ͷ���������
int packet_num;
int Seq_num;  //��ʼ�����к�
int Ack_num;
int confirmed;
int cur;
int MSS_count;
int MAX_SIZE;
int Window;
int ssthresh;
int dupACKcount;
int state;
bool is_end;

int staging_size;
int begin_index, end_index;
char** staging_data;   //�����ݴ���
int* staging_data_len;
clock_t* staging_time; //ʱ���¼
//queue<Staging>staging_data;
//queue<clock_t>staging_time;  //����ʱ�䱣����

// �����߳�
DWORD WINAPI Send_handle(LPVOID lparam) {
    Parameters* Para = (Parameters*)lparam;
    SOCKET socketClient = Para->socketClient;
    SOCKADDR_IN serverAddr = Para->serverAddr;
    int servAddrlen = sizeof(Para->serverAddr);
    char* Message = Para->Message;
    int mes_size = Para->mes_size;
    Packet_Header packet;

    char* send_buffer = new char[sizeof(packet) + MAX_SIZE];
    //Staging M_temp;

    // ���ʹ����ڵ����ݱ�
    while (confirmed < (packet_num - 1)) {  // ���з������ݾ���ȷ��
        while (cur <= confirmed + Window && cur < packet_num) {
            end_index++; //������������ǰ
            //cout << "��ǰstate�� " << state << " ��ǰ���ڴ�СΪ��" << Window << " MSS_count��" << MSS_count << endl;
            int data_len = (cur == packet_num - 1 ? mes_size - (packet_num - 1) * MAX_SIZE : MAX_SIZE);
            packet.tag = 0;
            packet.seq = Seq_num++;
            //cout << " int(packet.seq)��" << int(packet.seq) << endl;
            Seq_num = (Seq_num > 255 ? Seq_num - 256 : Seq_num);
            packet.datasize = data_len;
            packet.window = Window;  //�������ڴ�С
            packet.checksum = 0;
            packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
            memcpy(send_buffer, &packet, sizeof(packet));
            char* mes = Message + cur * MAX_SIZE;
            memcpy(send_buffer + sizeof(packet), mes, data_len);

            // ��¼ÿһ�εķ���ʱ��
            clock_t now = clock();
            staging_time[end_index % staging_size] = now;

            // ��������
            sendto(socketClient, send_buffer, sizeof(packet) + data_len, 0, (sockaddr*)&serverAddr, servAddrlen);

            //�����ݱ����ڻ�����
            memcpy(staging_data[end_index % staging_size], send_buffer, sizeof(packet) + data_len);
            staging_data_len[end_index % staging_size] = data_len;

            cout << "Begin sending message...... datasize:" << data_len << " bytes!"
                << " seq:" << int(packet.seq) << " window:" << int(packet.window)
                << "��ǰstate�� " << state << endl;
            //<< " checksum:" << int(packet.checksum) << endl;
            cur++;
        }

        // �����ش�
        if (dupACKcount == 3) {  //ת�����ٻָ�״̬
            for (int temp = begin_index; temp <= end_index; temp++) {
                sendto(socketClient, staging_data[temp % staging_size], sizeof(packet) + staging_data_len[temp % staging_size], 0, (sockaddr*)&serverAddr, servAddrlen);
                // ������������ݼ��
                Packet_Header p_temp;
                memcpy(&p_temp, staging_data[temp % staging_size], sizeof(packet));
                cout << "dupACK! ���·����ݴ�������������:  ���к�Ϊ" << int(p_temp.seq) << endl;
                clock_t now = clock();
                staging_time[temp % staging_size] = now;
            }
            ssthresh = Window / 2;
            Window = ssthresh + 3;
            MSS_count = 0;
            state = 2;
        }
        else {
            // ��ʱ����
            if ((clock() - staging_time[begin_index % staging_size]) / 1000 > 2 && (end_index >= begin_index))   //ʱ�䳬��3�� ���Ϊ��ʱ�ش� 
            {
                cout << begin_index % staging_size << "===" << end_index % staging_size << "===" << staging_time[begin_index % staging_size] << "===" << clock() << endl;
                //�ش�����δ��ack������
                for (int temp = begin_index; temp <= end_index; temp++) {
                    sendto(socketClient, staging_data[temp % staging_size], sizeof(packet) + staging_data_len[temp % staging_size], 0, (sockaddr*)&serverAddr, servAddrlen);
                    Packet_Header p_temp;
                    memcpy(&p_temp, staging_data[temp % staging_size], sizeof(packet));
                    cout << "��ʱ�� ���·����ݴ�������������:  ���к�Ϊ" << int(p_temp.seq) << endl;
                    clock_t now = clock();
                    staging_time[temp % staging_size] = now;
                }
                ssthresh = Window / 2;
                Window = 1;
                dupACKcount = 0;
                MSS_count = 0;
                cout << "��ʱ�����������׶�" << endl;
                state = 0; //ת��������״̬
            }
        }
    }
    //���ͽ�����־
    packet.tag = OVER;
    char* buffer = new char[sizeof(packet)];
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, servAddrlen);
    clock_t start = clock();
    cout << "The end tag has been sent" << endl;
    while (!is_end) {
        if ((clock() - start) / 1000 > 2)   //ʱ�䳬��2�� ���Ϊ��ʱ�ش� 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, servAddrlen);
            cout << "���ս�����־���ݷ��ͳ�ʱ�������ش�������" << endl;
            start = clock();
        }
    }
    return 0;
}

// �����߳�
DWORD WINAPI Recv_handle(LPVOID lparam) {
    Parameters* Para = (Parameters*)lparam;
    SOCKET socketClient = Para->socketClient;
    SOCKADDR_IN serverAddr = Para->serverAddr;
    int servAddrlen = sizeof(Para->serverAddr);
    char* Message = Para->Message;
    int mes_size = Para->mes_size;
    Packet_Header packet;

    char* re_buffer = new char[sizeof(packet)];
    char* send_buffer = new char[sizeof(packet) + MAX_SIZE];

    while (confirmed < (packet_num - 1)) {
        // ��������
        if (recvfrom(socketClient, re_buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, &servAddrlen) > 0) {

            //�ɹ��յ���Ӧ������У�����˵��������ȷ�����ݣ�ֱ�Ӷ���
            memcpy(&packet, re_buffer, sizeof(packet));
            if ((compute_sum((WORD*)&packet, sizeof(packet)) != 0)) {  //����У�����
                continue;
            }

            // ��ͬ״̬�¶Խ��ն˻ش��Ĳ�ͬ����
            switch (state)
            {
            case 0:   //������״̬��
                if (packet.ack == Ack_num) {
                    Ack_num = (Ack_num + 1) % 256;
                    //cout << "Successful send and recv response  ack:" << int(packet.ack) << endl;
                    Window++;
                    dupACKcount = 0;
                    confirmed++;
                    begin_index++;
                }
                else {
                    int dis = (int(packet.ack) - Ack_num) > 0 ? (int(packet.ack) - Ack_num) : (int(packet.ack) + 256 - Ack_num);
                    //�ظ�ACK
                    if (packet.ack == (Ack_num + 256 - 1) % 256) {
                        //cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                        dupACKcount++;
                    }
                    else if (dis < Window) {
                        // �ۼ�ȷ��
                        for (int temp = 0; temp <= dis; temp++) {
                            //cout << "===============�ۼ�ȷ��===============" << endl;
                            //cout << "Successful send and recv response  ack:" << Ack_num + temp << endl;
                            Window++;
                            confirmed++;
                            begin_index++;
                        }
                        dupACKcount = 0;
                        Ack_num = (int(packet.ack) + 1) % 256;  //�����´��ڴ���ACK���
                    }
                    else {  //����У�����ack������  �ط�����
                        cout << "Wrong Message !!!" << endl;
                    }
                }
                if (Window >= ssthresh && dupACKcount != 3) {  //ת��ӵ������״̬
                    cout << "�������׶�ת��ӵ�����ƽ׶�" << endl;
                    state = 1;
                }
                break;
            case 1:  //ӵ������״̬
                if (packet.ack == Ack_num) {
                    Ack_num = (Ack_num + 1) % 256;
                    //cout << "Successful send and recv response  ack:" << int(packet.ack) << endl;
                    MSS_count++;
                    if (MSS_count == Window) {
                        Window++;
                        MSS_count = 0;
                    }
                    dupACKcount = 0;
                    begin_index++;
                    confirmed++;
                }
                else {
                    int dis = (int(packet.ack) - Ack_num) > 0 ? (int(packet.ack) - Ack_num) : (int(packet.ack) + 256 - Ack_num);
                    //�ظ�ACK
                    if (packet.ack == (Ack_num + 256 - 1) % 256) {
                        //cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                        dupACKcount++;
                    }
                    else if (dis < Window) {
                        // �ۼ�ȷ��
                        for (int temp = 0; temp <= dis; temp++) {
                            //cout << "===============�ۼ�ȷ��===============" << endl;
                            //cout << "Successful send and recv response  ack:" << Ack_num + temp << endl;
                            MSS_count++;
                            if (MSS_count == Window) {
                                Window++;
                                MSS_count = 0;
                            }
                            begin_index++;
                            confirmed++;
                        }
                        dupACKcount = 0;
                        Ack_num = (int(packet.ack) + 1) % 256;  //�����´��ڴ���ACK���
                    }
                    else {  //����У�����ack������  �ط�����
                        cout << "Wrong Message !!!" << endl;
                    }
                }
                break;
            case 2:
                if (packet.ack == Ack_num) {
                    Ack_num = (Ack_num + 1) % 256;
                    //cout << "Successful send and recv response  ack:" << int(packet.ack) << endl;
                    Window = ssthresh;
                    dupACKcount = 0;
                    cout << "���ٻظ��׶�ת��ӵ�����ƽ׶�" << endl;
                    state = 1;
                    begin_index++;
                    confirmed++;
                }
                else {
                    int dis = (int(packet.ack) - Ack_num) > 0 ? (int(packet.ack) - Ack_num) : (int(packet.ack) + 256 - Ack_num);
                    //�ظ�ACK
                    if (packet.ack == (Ack_num + 256 - 1) % 256) {
                        //cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                        //Window = Window + 1;
                    }
                    else if (dis < Window) {
                        // �ۼ�ȷ��
                        for (int temp = 0; temp <= dis; temp++) {
                            //cout << "===============�ۼ�ȷ��===============" << endl;
                            //cout << "Successful send and recv response  ack:" << Ack_num + temp << endl;
                            begin_index++;
                            confirmed++;
                        }
                        cout << "���ٻظ��׶�ת��ӵ�����ƽ׶�" << endl;
                        state = 1;
                        Window = ssthresh;
                        dupACKcount = 0;
                        Ack_num = (int(packet.ack) + 1) % 256;  //�����´��ڴ���ACK���
                    }
                    else {  //����У�����ack������  �ط�����
                        cout << "Wrong Message !!!" << endl;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    char* buffer = new char[sizeof(packet)];
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, &servAddrlen) <= 0) {}
    memcpy(&packet, buffer, sizeof(packet));
    if (packet.tag == OVER && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "The end token was successfully received" << endl;
    }
    else {
        cout << "�޷����տͻ��˻ش�������־" << endl;
    }
    is_end = true;
    return 0;
}

void Send_Message(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* Message, int mes_size, int max_SIZE, int init_ssthresh) {

    packet_num = mes_size / (max_SIZE)+(mes_size % max_SIZE != 0);
    Seq_num = 0;  //��ʼ�����к�
    Ack_num = 1;
    confirmed = -1;
    cur = 0;
    MSS_count = 0;
    MAX_SIZE = max_SIZE;
    Window = 1;
    ssthresh = init_ssthresh;
    dupACKcount = 0;
    // 0��ʾ������  1��ʾӵ������  2��ʾ���ٻָ�
    state = 0;
    is_end = false;

    //������
    staging_size = 2 * init_ssthresh;
    begin_index = 0;
    end_index = -1;
    staging_data = new char* [staging_size];
    staging_data_len = new int[staging_size];
    staging_time = new clock_t[staging_size];
    for (int i = 0; i < staging_size; i++) {
        staging_data[i] = new char[sizeof(Packet_Header) + MAX_SIZE];
    }

    Parameters Para;
    Para.socketClient = socketClient;
    Para.serverAddr = servAddr;
    Para.Message = Message;
    Para.mes_size = mes_size;

    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // ������ģʽ

    HANDLE hthread[2];
    hthread[0] = CreateThread(NULL, 0, Recv_handle, (LPVOID)&Para, 0, NULL);
    hthread[1] = CreateThread(NULL, 0, Send_handle, (LPVOID)&Para, 0, NULL);
    WaitForSingleObject(hthread[0], INFINITE);
    WaitForSingleObject(hthread[1], INFINITE);
}

// ��Ϊ���շ���������
int Recv_Message(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen, char* Mes, int MAX_SIZE, int Window) {
    Packet_Header packet;
    char* buffer = new char[sizeof(packet) + MAX_SIZE];  // ����buffer
    int ack = 1;  // ȷ�����к�
    int seq = 0;
    long F_Len = 0;     //�����ܳ�
    int S_Len = 0;      //�������ݳ���
    //��������
    u_long mode = 1;
    ioctlsocket(socketServer, FIONBIO, &mode);  // ������ģʽ
    while (1) {
        if (recvfrom(socketServer, buffer, sizeof(packet) + MAX_SIZE, 0, (sockaddr*)&clieAddr, &clieAddrlen) > 0) {
            //recvfrom(socketServer, buffer, sizeof(packet) + MAX_SIZE, 0, (sockaddr*)&clieAddr, &clieAddrlen);
            memcpy(&packet, buffer, sizeof(packet));

            //if (((rand() % (255 - 1)) + 1) == 199) {
            //    cout << "���кţ�" << int(packet.seq) << " ========�����ʱ�ش�����========" << endl;
            //    continue;
            //}

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
                // �Խ������ݼ���ȷ�ϣ����seqǰ������δ���յ� �������ݰ� �ش���ǰ�����ACK
                cout << "Ԥ�ڵ�seq�� " << seq << "   ʵ���յ��ģ� " << int(packet.seq) << endl;
                if (packet.seq != seq) {   // seq�ǵ�ǰ������յ������ŷ���
                    Packet_Header temp;
                    temp.tag = 0;
                    temp.ack = ack - 1;  //�ش�ACK
                    temp.checksum = 0;
                    temp.checksum = compute_sum((WORD*)&temp, sizeof(temp));
                    memcpy(buffer, &temp, sizeof(temp));
                    sendto(socketServer, buffer, sizeof(temp), 0, (sockaddr*)&clieAddr, clieAddrlen);
                    cout << " Send a resend flag (same ACK) to the client: " << int(temp.ack) << endl;
                    continue;
                }

                // �ɹ��յ�����
                S_Len = packet.datasize;
                cout << "Begin recving message...... datasize:" << S_Len << " bytes!" << " flag:"
                    << int(packet.tag) << " seq:" << int(packet.seq) << " window:" << int(packet.window)
                    << " checksum:" << int(packet.checksum) << endl;
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
                //if (((rand() % (255 - 1)) + 1) != 187) {
                sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
                //}
                //else
                    //cout << "���кţ�" << int(packet.seq) << " ========�ۼ�ȷ�ϲ���========" << endl;
                cout << "Successful recv and send response  ack : " << int(packet.ack) << endl;
                seq = (seq > 255 ? seq - 256 : seq);
                ack = (ack > 255 ? ack - 256 : ack);
            }
            else if (packet.tag == 0) {  // У��ͼ��㲻��ȷ�����ݳ����֪���ش�
                Packet_Header temp;
                temp.tag = 0;
                temp.ack = ack - 1;  //���ս������������ �ش�ACK
                temp.checksum = 0;
                temp.checksum = compute_sum((WORD*)&temp, sizeof(temp));
                memcpy(buffer, &temp, sizeof(temp));
                sendto(socketServer, buffer, sizeof(temp), 0, (sockaddr*)&clieAddr, clieAddrlen);
                cout << "Send a resend flag (errro ACK) to the client" << endl;
                continue;
            }
        }
    }

    mode = 0;
    ioctlsocket(socketServer, FIONBIO, &mode);  // ����ģʽ
    // ���ͽ�����־
    packet.tag = OVER;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
    cout << "The end tag has been sent" << endl;
    return F_Len;
}
