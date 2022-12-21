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
int Client_Server_Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, int label)
{
    Packet_Header packet;
    int size = sizeof(packet);  // size = 8
    char* buffer = new char[sizeof(packet)];  // 发送buffer

    // 第一次：客户端首先向服务端发送建立连接请求
    packet.tag = SYN;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto函数发送数据出错！" << endl;
        return 0;
    }

    // 设定超时重传机制
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // 非阻塞模式
    clock_t start = clock(); //记录发送第一次握手时间

    // 第二次：客户端接收服务端回传的握手
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //时间超过1秒 标记为超时重传 
        {
            sendto(socketClient, buffer, size, 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "第一次握手超时，正在进行重传" << endl;
        }
    }

    cout << "成功发送第一次握手信息" << endl;

    // 对接收数据进行校验和检验
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "成功收到第二次握手信息" << endl;
    }
    else {
        cout << "无法接收服务端回传建立可靠连接" << endl;
        return 0;
    }

    // 第三次：客户端发送第三次握手数据
    packet.tag = ACK_SYN;
    packet.datasize = label;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto函数发送数据出错！" << endl;
        return 0;
    }

    cout << "成功发送第三次握手信息" << endl;
    cout << "客户端与服务端成功进行三次握手建立连接！---------可以开始发送/接收数据" << endl;

    mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);
    return 1;
}

int Client_Server_Size(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, int Size, int Window, int*& A) {
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // 发送buffer

    // 告知服务端 客户端的缓冲区为多大
    packet.tag = SYN;
    packet.datasize = Size;
    packet.window = Window;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto函数发送数据出错！" << endl;
        return -1;
    }

    // 设定超时重传机制
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // 非阻塞模式
    clock_t start = clock(); //记录发送第一次握手时间

    // 接收服务端回传告知的缓冲区大小
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //时间超过1秒 标记为超时重传 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "数据发送超时！正在进行重传" << endl;
        }
    }
    cout << "成功告知对方缓冲区大小" << endl;

    // 对接收数据进行校验和检验
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "成功收到对方缓冲区大小信息" << endl;
    }
    else {
        cout << "无法接收服务端回传建立可靠连接" << endl;
        return -1;
    }

    mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);  // 阻塞模式

    A[0] = (Size > packet.datasize) ? packet.datasize : Size;
    A[1] = (Window > packet.window) ? packet.window : Window;
    return 1;
}

// 客户端与服务器端断开连接（采取四次挥手的方式
int Client_Server_Disconnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
    Packet_Header packet;
    char* buffer = new char[sizeof(packet)];  // 发送buffer
    int size = sizeof(packet);

    // 客户端第一次发起挥手
    packet.tag = FIN_ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, size);
    memcpy(buffer, &packet, size);
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto函数发送数据出错！" << endl;
        return 0;
    }

    clock_t start = clock();   // 记录第一次挥手的时间

    // 客户端接收服务端发来的第二次挥手
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //时间超过1秒 标记为超时重传 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "第一次握手超时，正在进行重传" << endl;
        }
    }

    cout << "成功发送第一次挥手信息" << endl;

    // 对接收数据进行校验和检验
    memcpy(&packet, buffer, sizeof(packet));
    if ((packet.tag == ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "成功收到第二次挥手信息" << endl;
    }
    else {
        cout << "无法接收服务端回传断开连接" << endl;
        return 0;
    }

    // 客户端接收服务端发来的第三次挥手
    while (1) {
        if (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) == -1) {
            cout << "无法接收服务端回传断开连接" << endl;
            return 0;
        }
        // 对接收数据进行校验和检验
        memcpy(&packet, buffer, sizeof(packet));
        if ((packet.tag == FIN_ACK) && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
            cout << "成功收到第三次挥手信息" << endl;
            break;
        }
    }

    // 第四次：客户端发送第四次挥手数据
    packet.tag = ACK;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    if (sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen) == -1) {
        cout << "sendto函数发送数据出错！" << endl;
        return 0;
    }

    cout << "成功发送第四次挥手信息" << endl;
    cout << "客户端与服务端成功断开连接！" << endl;

    return 1;
}

// 作为发送方发送数据
void Send_Message(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* Message, int mes_size, int MAX_SIZE, int Window) {
    int packet_num = mes_size / (MAX_SIZE)+(mes_size % MAX_SIZE != 0);
    int Seq_num = 0;  //初始化序列号
    int Ack_num = 1;
    Packet_Header packet;
    int confirmed = -1;
    int cur = 0;
    int last = 0; //当前未收到确认的数据报序号
    char** Staging = new char* [Window];   //数据暂存区
    int* Staging_len = new int[Window];
    for (int i = 0; i < Window; i++) {
        Staging[i] = new char[sizeof(packet) + MAX_SIZE];
    }
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);  // 非阻塞模式
    while (confirmed < (packet_num - 1)) {  // 所有发送数据均被确认
        clock_t* time = new clock_t[Window];
        while (cur <= confirmed + Window && cur < packet_num) {
            int data_len = (cur == packet_num - 1 ? mes_size - (packet_num - 1) * MAX_SIZE : MAX_SIZE);
            //char* buffer = new char[sizeof(packet) + data_len];  // 发送buffer
            packet.tag = 0;
            packet.seq = Seq_num++;
            Seq_num = (Seq_num > 255 ? Seq_num - 256 : Seq_num);
            packet.datasize = data_len;
            Staging_len[cur % Window] = data_len;
            packet.window = Window - (cur - confirmed);  //剩余滑动窗口大小
            packet.checksum = 0;
            packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
            memcpy(Staging[cur % Window], &packet, sizeof(packet));
            char* mes = Message + cur * MAX_SIZE;
            memcpy(Staging[cur % Window] + sizeof(packet), mes, data_len);

            // 发送数据
            sendto(socketClient, Staging[cur % Window], sizeof(packet) + data_len, 0, (sockaddr*)&servAddr, servAddrlen);
            // 记录每一次的发送时间
            time[cur % Window] = clock();
            cout << "Begin sending message...... datasize:" << data_len << " bytes!"
                << " seq:" << int(packet.seq) << " window:" << int(packet.window)
                << " checksum:" << int(packet.checksum) << "时间存储标号：" << last << endl;
            cur++;
        }
        
        char* re_buffer = new char[sizeof(packet)];
        while (recvfrom(socketClient, re_buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
            int time_temp = last;
            if ((clock() - time[last]) / 1000 > 2)   //时间超过2秒 标记为超时重传 
            {  //重传所有未被ack的数据
                cout << "====================" << last << endl;
                for (int temp = confirmed + 1; temp <= confirmed + Window && temp < packet_num; temp++) {
                    // 超时重新发送数据
                    sendto(socketClient, Staging[temp % Window], sizeof(packet) + Staging_len[cur % Window], 0, (sockaddr*)&servAddr, servAddrlen);
                    cout << "超时重新发送暂存区内所有数据:  序列号为" << temp % 256 << "时间存储标号：" << time_temp << endl;
                    time[time_temp] = clock();
                    time_temp = (time_temp + 1) % Window;
                }
            }
        }

        //成功收到响应
        memcpy(&packet, re_buffer, sizeof(packet));
        if ((compute_sum((WORD*)&packet, sizeof(packet)) != 0)) {  //数据校验出错
            continue;
        }
        //收到正确响应 滑动窗口
        if (packet.ack == Ack_num) {
            Ack_num = (Ack_num + 1) % 256;
            cout << "Successful send and recv response  ack:" << int(packet.ack) << "时间存储标号：" << last << endl;
            confirmed++;
            time[last] = clock();
            last = (last + 1) % Window;
        }
        else {
            int dis = (int(packet.ack) - Ack_num) > 0 ? (int(packet.ack) - Ack_num) : (int(packet.ack) + 256 - Ack_num);
            //忽略重复ACK
            if (packet.ack == (Ack_num + 256 - 1) % 256) {
                cout << "recv (same ACK)!!!!" << int(packet.ack) << endl;
                continue;
            }
            else if (dis < Window) {
                // 累计确认
                for (int temp = 0; temp <= dis; temp++) {
                    cout << "===============累计确认===============" << endl;
                    cout << "Successful send and recv response  ack:" << Ack_num + temp << endl;
                    confirmed++; 
                    time[last] = clock();
                    last = (last + 1) % Window;
                }
                Ack_num = (int(packet.ack) + 1) % 256;  //更新下次期待的ACK序号
            }
            else {  //数据校验出错、ack不合理  重发数据
                for (int temp = confirmed + 1; temp <= confirmed + Window && temp < packet_num; temp++) {
                    // 超时重新发送数据
                    sendto(socketClient, Staging[temp % Window], sizeof(packet) + Staging_len[cur % Window], 0, (sockaddr*)&servAddr, servAddrlen);
                    cout << "出错重新发送暂存区内所有数据:  序列号为" << temp % 256 << endl;
                }
            }
        }
    }
    //发送结束标志
    packet.tag = OVER;
    char* buffer = new char[sizeof(packet)];
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
    cout << "The end tag has been sent" << endl;
    clock_t start = clock();
    while (recvfrom(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0) {
        if ((clock() - start) / 1000 > 1)   //时间超过1秒 标记为超时重传 
        {
            sendto(socketClient, buffer, sizeof(packet), 0, (sockaddr*)&servAddr, servAddrlen);
            cout << "数据发送超时，正在重传！！！" << endl;
            start = clock();
        }
    }
    mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);  // 阻塞模式
    memcpy(&packet, buffer, sizeof(packet));
    if (packet.tag == OVER && (compute_sum((WORD*)&packet, sizeof(packet)) == 0)) {
        cout << "The end token was successfully received" << endl;
    }
    else {
        cout << "无法接收客户端回传" << endl;
    }
    return;
}

// 作为接收方接收数据
int Recv_Message(SOCKET& socketServer, SOCKADDR_IN& clieAddr, int& clieAddrlen, char* Mes, int MAX_SIZE, int Window) {
    Packet_Header packet;
    char* buffer = new char[sizeof(packet) + MAX_SIZE];  // 接收buffer
    int ack = 1;  // 确认序列号
    int seq = 0;
    long F_Len = 0;     //数据总长
    int S_Len = 0;      //单次数据长度
    bool test_state = true;
    //接收数据
    while (1) {
        while (recvfrom(socketServer, buffer, sizeof(packet) + MAX_SIZE, 0, (sockaddr*)&clieAddr, &clieAddrlen) <= 0);
        memcpy(&packet, buffer, sizeof(packet));

        if (((rand() % (255 - 1)) + 1) == 199 && test_state) {
            cout << int(packet.seq) << "随机超时重传测试" << endl;
            test_state = false;
            continue;
        }

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
            if (packet.seq != seq) {   // seq是当前按序接收的最高序号分组
                Packet_Header temp;
                temp.tag = 0;
                temp.ack = ack - 1;  //重传ACK
                temp.checksum = 0;
                temp.checksum = compute_sum((WORD*)&temp, sizeof(temp));
                memcpy(buffer, &temp, sizeof(temp));
                sendto(socketServer, buffer, sizeof(temp), 0, (sockaddr*)&clieAddr, clieAddrlen);
                cout << "Send a resend flag (same ACK) to the client" << endl;
                continue;
            }

            // 成功收到数据
            S_Len = packet.datasize;
            cout << "Begin recving message...... datasize:" << S_Len << " bytes!" << " flag:"
                << int(packet.tag) << " seq:" << int(packet.seq) << " checksum:" << int(packet.checksum) << endl;
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
            if (((rand() % (255 - 1)) + 1) != 187) {
                sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
            }
            else
                cout << "序列号：" << int(packet.seq) << " ========累计确认测试========" << endl;
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

    // 发送结束标志
    packet.tag = OVER;
    packet.checksum = 0;
    packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
    memcpy(buffer, &packet, sizeof(packet));
    sendto(socketServer, buffer, sizeof(packet), 0, (sockaddr*)&clieAddr, clieAddrlen);
    cout << "The end tag has been sent" << endl;
    return F_Len;
}