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

// 计算校验和
WORD compute_sum(WORD* message, int size) {   // size = 8
    int count = (size + 1) / 2;  // 防止奇数字节数 确保WORD 16位
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

// 客户端与服务端建立连接（采取三次握手的方式
int Client_Server_Connect(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // 发送buffer

    // 第一次：服务端接收客户端发送的连接请求
    while (1) {
        if (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) == -1) {
            cout << "无法接收客户端发送的连接请求" << endl;
            return -1;
        }
        memcpy(&packet, buffer, sizeof(packet));
        if (packet.tag == SYN && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "成功接收第一次握手信息" << endl;
            break;
        }
    }

    // 第二次：服务端向客户端发送握手信息
    packet.tag = ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen) == -1)
    {
        return -1;
    }
    u_long mode = 1;
    ioctlsocket(socketServer, FIONBIO, &mode);  // 非阻塞模式
    clock_t start = clock();//记录第二次握手发送时间

    // 第三次：服务端接收客户端发送的握手信息
    while (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //时间超过1秒 标记为超时重传 
        {
            sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
            start = clock();
            cout << "第二次握手超时，正在进行重传" << endl;
        }
    }
    mode = 0;
    ioctlsocket(socketServer, FIONBIO, &mode);  // 阻塞模式

    cout << "成功发送第二次握手信息" << endl;

    // 对接收数据进行校验和检验
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK_SYN) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "成功收到第三次握手信息" << endl;
    }
    else {
        cout << "无法接收客户端回传建立可靠连接" << endl;
        return -1;
    }

    cout << "客户端与服务端成功进行三次握手建立连接！---------可以开始发送/接收数据" << endl;

    return int(packet.datasize);
}

int Client_Server_Size(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, int size, int ssthresh, int*& A) {
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // 发送buffer

    // 接收客户端告知的缓冲区数据大小
    while (1) {
        if (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) == -1) {
            cout << "无法接收客户端发送的连接请求" << endl;
            return -1;
        }
        memcpy(&packet, buffer, sizeof(packet));
        if (packet.tag == SYN && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "成功收到对方数据发送缓冲区以及初始阈值大小信息" << endl;
            break;
        }
    }
    int client_size = packet.datasize;
    int client_win = packet.window;

    // 服务端告知客户端缓冲区大小信息
    packet.tag = ACK;
    packet.datasize = size;
    packet.window = ssthresh;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        return -1;
    }
    cout << "成功告知对方数据发送缓冲区以及初始阈值大小信息" << endl;

    A[0] = (size > client_size) ? client_size : size;
    A[1] = (ssthresh > client_win) ? client_win : ssthresh;
    return 1;
}

// 客户端与服务器端断开连接（采取四次挥手的方式
int Client_Server_Disconnect(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // 发送buffer
    int size = sizeof(packet);

    // 接收客户端发来的第一次挥手信息
    while (1) {
        if (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) == -1) {
            cout << "无法接收客户端发送的连接请求" << endl;
            return 0;
        }
        memcpy(&packet, buffer, sizeof(packet));
        if (packet.tag == FIN_ACK && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "成功收到第一次挥手信息" << endl;
            break;
        }
    }

    // 第二次：服务端向客户端发送挥手信息
    packet.tag = ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen) == -1)
    {
        return 0;
    }

    cout << "成功发送第二次挥手信息" << endl;
    //Sleep(10000);

    // 第三次：服务端向客户端发送挥手信息
    packet.tag = FIN_ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen) == -1)
    {
        return 0;
    }
    clock_t start = clock();//记录第三次挥手发送时间

    // 第四次：服务端接收客户端发送的挥手信息
    while (recvfrom(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, &clieAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //时间超过1秒 标记为超时重传 
        {
            sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
            start = clock();
            cout << "第三次挥手超时，正在进行重传" << endl;
        }
    }

    cout << "成功发送第三次挥手信息" << endl;

    // 对接收数据进行校验和检验
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "成功收到第四次挥手信息" << endl;
    }
    else {
        cout << "无法接收客户端回传建立可靠连接" << endl;
        return 0;
    }

    cout << "客户端与服务端成功断开连接！" << endl;

    return 1;
}

// 线程参数
struct Parameters
{
    SOCKET socketClient;
    SOCKADDR_IN serverAddr;
    char* Message;
    int mes_size;

};

// 多线程——发送方传输数据
int packet_num;
int Seq_num;  //初始化序列号
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
char** staging_data;   //数据暂存区
int* staging_data_len;
clock_t* staging_time; //时间记录
//queue<Staging>staging_data;
//queue<clock_t>staging_time;  //发送时间保存区

// 发送线程
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

    // 发送窗口内的数据报
    while (confirmed < (packet_num - 1)) {  // 所有发送数据均被确认
        while (cur <= confirmed + Window && cur < packet_num) {
            end_index++; //缓冲区索引向前
            //cout << "当前state： " << state << " 当前窗口大小为：" << Window << " MSS_count：" << MSS_count << endl;
            int data_len = (cur == packet_num - 1 ? mes_size - (packet_num - 1) * MAX_SIZE : MAX_SIZE);
            packet.tag = 0;
            packet.seq = Seq_num++;
            //cout << " int(packet.seq)：" << int(packet.seq) << endl;
            Seq_num = (Seq_num > 255 ? Seq_num - 256 : Seq_num);
            packet.datasize = data_len;
            packet.window = Window;  //滑动窗口大小
            packet.checksum = 0;
            packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
            memcpy(send_buffer, &packet, sizeof(packet));
            char* mes = Message + cur * MAX_SIZE;
            memcpy(send_buffer + sizeof(packet), mes, data_len);

            // 记录每一次的发送时间
            clock_t now = clock();
            staging_time[end_index % staging_size] = now;

            // 发送数据
            sendto(socketClient, send_buffer, sizeof(packet) + data_len, 0, (sockaddr*)&serverAddr, servAddrlen);

            //将数据保留在缓冲区
            memcpy(staging_data[end_index % staging_size], send_buffer, sizeof(packet) + data_len);
            staging_data_len[end_index % staging_size] = data_len;

            cout << "Begin sending message...... datasize:" << data_len << " bytes!"
                << " seq:" << int(packet.seq) << " window:" << int(packet.window)
                << "当前state： " << state << endl;
            //<< " checksum:" << int(packet.checksum) << endl;
            cur++;
        }

        // 快速重传
        if (dupACKcount == 3) {  //转至快速恢复状态
            for (int temp = begin_index; temp <= end_index; temp++) {
                sendto(socketClient, staging_data[temp % staging_size], sizeof(packet) + staging_data_len[temp % staging_size], 0, (sockaddr*)&serverAddr, servAddrlen);
                // 输出缓冲区数据检查
                Packet_Header p_temp;
                memcpy(&p_temp, staging_data[temp % staging_size], sizeof(packet));
                cout << "dupACK! 重新发送暂存区内所有数据:  序列号为" << int(p_temp.seq) << endl;
                clock_t now = clock();
                staging_time[temp % staging_size] = now;
            }
            ssthresh = Window / 2;
            Window = ssthresh + 3;
            MSS_count = 0;
            state = 2;
        }
        else {
            // 超时机制
            if ((clock() - staging_time[begin_index % staging_size]) / 1000 > 2 && (end_index >= begin_index))   //时间超过3秒 标记为超时重传 
            {
                cout << begin_index % staging_size << "===" << end_index % staging_size << "===" << staging_time[begin_index % staging_size] << "===" << clock() << endl;
                //重传所有未被ack的数据
                for (int temp = begin_index; temp <= end_index; temp++) {
                    sendto(socketClient, staging_data[temp % staging_size], sizeof(packet) + staging_data_len[temp % staging_size], 0, (sockaddr*)&serverAddr, servAddrlen);
                    Packet_Header p_temp;
                    memcpy(&p_temp, staging_data[temp % staging_size], sizeof(packet));
                    cout << "超时！ 重新发送暂存区内所有数据:  序列号为" << int(p_temp.seq) << endl;
                    clock_t now = clock();
                    staging_time[temp % staging_size] = now;
                }
                ssthresh = Window / 2;
                Window = 1;
                dupACKcount = 0;
                MSS_count = 0;
                cout << "超时进入慢启动阶段" << endl;
                state = 0; //转至慢启动状态
            }
        }
    }
    //发送结束标志
    packet.tag = OVER;
    char* buffer = new char[sizeof(packet)];
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, servAddrlen);
    clock_t start = clock();
    cout << "The end tag has been sent" << endl;
    while (!is_end) {
        if ((clock() - start) / 1000 > 2)   //时间超过2秒 标记为超时重传 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, servAddrlen);
            cout << "最终结束标志数据发送超时，正在重传！！！" << endl;
            start = clock();
        }
    }
    return 0;
}

// 接收线程
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
        // 接收数据
        if (recvfrom(socketClient, re_buffer, sizeof(packet), 0, (sockaddr*)&serverAddr, &servAddrlen) > 0) {

            //成功收到响应，数据校验出错说明不是正确的数据，直接丢弃
            memcpy(&packet, re_buffer, sizeof(packet));
            if ((compute_sum((WORD*)&packet, sizeof(packet)) != 0)) {  //数据校验出错
                continue;
            }

            // 不同状态下对接收端回传的不同处理
            switch (state)
            {
            case 0:   //慢启动状态下
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
                    //重复ACK
                    if (packet.ack == (Ack_num + 256 - 1) % 256) {
                        //cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                        dupACKcount++;
                    }
                    else if (dis < Window) {
                        // 累计确认
                        for (int temp = 0; temp <= dis; temp++) {
                            //cout << "===============累计确认===============" << endl;
                            //cout << "Successful send and recv response  ack:" << Ack_num + temp << endl;
                            Window++;
                            confirmed++;
                            begin_index++;
                        }
                        dupACKcount = 0;
                        Ack_num = (int(packet.ack) + 1) % 256;  //更新下次期待的ACK序号
                    }
                    else {  //数据校验出错、ack不合理  重发数据
                        cout << "Wrong Message !!!" << endl;
                    }
                }
                if (Window >= ssthresh && dupACKcount != 3) {  //转至拥塞控制状态
                    cout << "慢启动阶段转至拥塞控制阶段" << endl;
                    state = 1;
                }
                break;
            case 1:  //拥塞控制状态
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
                    //重复ACK
                    if (packet.ack == (Ack_num + 256 - 1) % 256) {
                        //cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                        dupACKcount++;
                    }
                    else if (dis < Window) {
                        // 累计确认
                        for (int temp = 0; temp <= dis; temp++) {
                            //cout << "===============累计确认===============" << endl;
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
                        Ack_num = (int(packet.ack) + 1) % 256;  //更新下次期待的ACK序号
                    }
                    else {  //数据校验出错、ack不合理  重发数据
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
                    cout << "快速回复阶段转至拥塞控制阶段" << endl;
                    state = 1;
                    begin_index++;
                    confirmed++;
                }
                else {
                    int dis = (int(packet.ack) - Ack_num) > 0 ? (int(packet.ack) - Ack_num) : (int(packet.ack) + 256 - Ack_num);
                    //重复ACK
                    if (packet.ack == (Ack_num + 256 - 1) % 256) {
                        //cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                        //Window = Window + 1;
                    }
                    else if (dis < Window) {
                        // 累计确认
                        for (int temp = 0; temp <= dis; temp++) {
                            //cout << "===============累计确认===============" << endl;
                            //cout << "Successful send and recv response  ack:" << Ack_num + temp << endl;
                            begin_index++;
                            confirmed++;
                        }
                        cout << "快速回复阶段转至拥塞控制阶段" << endl;
                        state = 1;
                        Window = ssthresh;
                        dupACKcount = 0;
                        Ack_num = (int(packet.ack) + 1) % 256;  //更新下次期待的ACK序号
                    }
                    else {  //数据校验出错、ack不合理  重发数据
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
        cout << "无法接收客户端回传结束标志" << endl;
    }
    is_end = true;
    return 0;
}

void Send_Message(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* Message, int mes_size, int max_SIZE, int init_ssthresh) {

    packet_num = mes_size / (max_SIZE)+(mes_size % max_SIZE != 0);
    Seq_num = 0;  //初始化序列号
    Ack_num = 1;
    confirmed = -1;
    cur = 0;
    MSS_count = 0;
    MAX_SIZE = max_SIZE;
    Window = 1;
    ssthresh = init_ssthresh;
    dupACKcount = 0;
    // 0表示慢启动  1表示拥塞避免  2表示快速恢复
    state = 0;
    is_end = false;

    //缓冲区
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
    ioctlsocket(socketClient, FIONBIO, &mode);  // 非阻塞模式

    HANDLE hthread[2];
    hthread[0] = CreateThread(NULL, 0, Recv_handle, (LPVOID)&Para, 0, NULL);
    hthread[1] = CreateThread(NULL, 0, Send_handle, (LPVOID)&Para, 0, NULL);
    WaitForSingleObject(hthread[0], INFINITE);
    WaitForSingleObject(hthread[1], INFINITE);
}

// 作为接收方接收数据
int Recv_Message(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen, char* Mes, int MAX_SIZE, int Window) {
    Packet_Header packet;
    char* buffer = new char[sizeof(packet) + MAX_SIZE];  // 接收buffer
    int ack = 1;  // 确认序列号
    int seq = 0;
    long F_Len = 0;     //数据总长
    int S_Len = 0;      //单次数据长度
    //接收数据
    u_long mode = 1;
    ioctlsocket(socketServer, FIONBIO, &mode);  // 非阻塞模式
    while (1) {
        if (recvfrom(socketServer, buffer, sizeof(packet) + MAX_SIZE, 0, (sockaddr*)&clieAddr, &clieAddrlen) > 0) {
            //recvfrom(socketServer, buffer, sizeof(packet) + MAX_SIZE, 0, (sockaddr*)&clieAddr, &clieAddrlen);
            memcpy(&packet, buffer, sizeof(packet));

            //if (((rand() % (255 - 1)) + 1) == 199) {
            //    cout << "序列号：" << int(packet.seq) << " ========随机超时重传测试========" << endl;
            //    continue;
            //}

            // END 全局结束
            if (packet.tag == END && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
                cout << "The all_end tag has been recved" << endl;
                return 999;
            }

            // 结束标记
            if (packet.tag == OVER && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
                cout << "The end tag has been recved" << endl;
                break;
            }

            // 接收数据
            if (packet.tag == 0 && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
                // 对接收数据加以确认，如果seq前的序列未被收到 则丢弃数据包 重传当前的最大ACK
                cout << "预期的seq： " << seq << "   实际收到的： " << int(packet.seq) << endl;
                if (packet.seq != seq) {   // seq是当前按序接收的最高序号分组
                    Packet_Header temp;
                    temp.tag = 0;
                    temp.ack = ack - 1;  //重传ACK
                    temp.checksum = 0;
                    temp.checksum = compute_sum((WORD*)&temp, sizeof(temp));
                    memcpy(buffer, &temp, sizeof(temp));
                    sendto(socketServer, buffer, sizeof(temp), 0, (sockaddr*)&clieAddr, clieAddrlen);
                    cout << " Send a resend flag (same ACK) to the client: " << int(temp.ack) << endl;
                    continue;
                }

                // 成功收到数据
                S_Len = packet.datasize;
                cout << "Begin recving message...... datasize:" << S_Len << " bytes!" << " flag:"
                    << int(packet.tag) << " seq:" << int(packet.seq) << " window:" << int(packet.window)
                    << " checksum:" << int(packet.checksum) << endl;
                memcpy(Mes + F_Len, buffer + sizeof(packet), S_Len);
                F_Len += S_Len;

                // 返回 告知客户端已成功收到
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
                    //cout << "序列号：" << int(packet.seq) << " ========累计确认测试========" << endl;
                cout << "Successful recv and send response  ack : " << int(packet.ack) << endl;
                seq = (seq > 255 ? seq - 256 : seq);
                ack = (ack > 255 ? ack - 256 : ack);
            }
            else if (packet.tag == 0) {  // 校验和计算不正确、数据出错告知其重传
                Packet_Header temp;
                temp.tag = 0;
                temp.ack = ack - 1;  //按照接收乱序错误处理 重传ACK
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
    ioctlsocket(socketServer, FIONBIO, &mode);  // 阻塞模式
    // 发送结束标志
    packet.tag = OVER;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
    cout << "The end tag has been sent" << endl;
    return F_Len;
}
