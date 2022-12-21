#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "Packet.h"
#include "Process2.h"
#include <string.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define PORT 5005
#define IP "127.0.0.1"

// 功能包括：建立连接、差错检测、确认重传等。
// 流量控制采用停等机制

int main()
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        cout << "Startup 出错！" << endl;
        WSACleanup();
        return 0;
    }

    SOCKET server = socket(AF_INET, SOCK_DGRAM, 0);
    if (server == -1)
    {
        cout << "socket 生成出错！" << endl;
        WSACleanup();
        return 0;
    }
    cout << "服务端套接字创建成功！" << endl;

    // 绑定端口和ip
    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &addr.sin_addr.s_addr);

    // 建立连接
    if (bind(server, (SOCKADDR*)&addr, sizeof(addr)) != 0)
    {
        cout << "bind 出错！" << endl;
        WSACleanup();
        return 0;
    }
    int length = sizeof(addr);
    cout << "等待客户端提出连接请求......" << endl;
    int label = Client_Server_Connect(server, addr, length);
    cout << "请输入本端单次容纳数据缓冲区大小：";
    int SIZE;
    cin >> SIZE;
    cout << "请输入本端滑动窗口缓冲区大小：";
    int Window;
    cin >> Window;
    int* S_W = new int[2];
    Client_Server_Size(server, addr, length, SIZE, Window, S_W);
    SIZE = S_W[0];
    Window = S_W[1];
    cout << "双方协定的单次数据发送大小为：" << SIZE << "，滑动窗口大小为：" << Window << endl;

    if (label == 0) {
        while (1) {
            cout << "=====================================================================" << endl;
            cout << "Waiting to receive!!!" << endl;
            char* F_name = new char[20];
            char* Message = new char[100000000];
            int name_len = Recv_Message(server, addr, length, F_name, SIZE, Window);

            // 全局结束
            if (name_len == 999)
                break;

            int file_len = Recv_Message(server, addr, length, Message, SIZE, Window);
            string a;
            for (int i = 0; i < name_len; i++) {
                a = a + F_name[i];
            }
            cout << "接收文件名：" << a << endl;
            cout << "接收文件数据大小：" << file_len << "bytes!" << endl;

            FILE* fp;
            if ((fp = fopen(a.c_str(), "wb")) == NULL)
            {
                cout << "Open File failed!" << endl;
                exit(0);
            }
            //从buffer中写数据到fp指向的文件中
            fwrite(Message, file_len, 1, fp);
            cout << "Successfully receive the data in its entirety and save it" << endl;
            cout << "=====================================================================" << endl;
            //关闭文件指针
            fclose(fp);
        }
    }
    if (label == 1) {
        while (1) {
            cout << "=====================================================================" << endl;
            cout << "Waiting to choose File!!!" << endl;
            //输入文件名
            char InFileName[20];
            //文件数据长度
            int F_length;
            //输入要读取的图像名
            cout << "Enter File name:";
            cin >> InFileName;

            // 全局结束
            if (InFileName[0] == 'q' && strlen(InFileName) == 1) {
                Packet_Header packet;
                char* buffer = new char[sizeof(packet)];  // 发送buffer
                packet.tag = END;
                packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
                memcpy(buffer, &packet, sizeof(packet));
                sendto(server, buffer, sizeof(packet), 0, (sockaddr*)&addr, length);
                cout << "Send a all_end flag to the server" << endl;
                break;
            }

            //文件指针
            FILE* fp;
            //以二进制方式打开图像
            if ((fp = fopen(InFileName, "rb")) == NULL)
            {
                cout << "Open file failed!" << endl;
                exit(0);
            }
            //获取文件数据总长度
            fseek(fp, 0, SEEK_END);
            F_length = ftell(fp);
            rewind(fp);
            //根据文件数据长度分配内存buffer
            char* FileBuffer = (char*)malloc(F_length * sizeof(char));
            //将图像数据读入buffer
            fread(FileBuffer, F_length, 1, fp);
            fclose(fp);

            cout << "发送文件数据大小：" << F_length << "bytes!" << endl;

            Send_Message(server, addr, length, InFileName, strlen(InFileName), SIZE, Window);
            clock_t start = clock();
            Send_Message(server, addr, length, FileBuffer, F_length, SIZE, Window);
            clock_t end = clock();
            cout << "传输总时间为:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
            cout << "吞吐率为:" << ((float)F_length) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
            cout << "=====================================================================" << endl;
        }
    }
    Client_Server_Disconnect(server, addr, length);
    closesocket(server);
    WSACleanup();
    system("pause");
    return 0;
}