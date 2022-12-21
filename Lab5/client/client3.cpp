#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "Packet.h"
#include "Process3.h"
#include <fstream>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

//USHORT PORT = 5005; // 5005是服务器端口IP
USHORT PORT = 5015; //5015是路由器exe端口IP
#define IP "127.0.0.1"

// 功能包括：建立连接、差错检测、确认重传等。
// 流量控制采用停等机制

int main()
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        cout << "Startup 出错！" << endl;
        WSACleanup();
        return 0;
    }
    SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
    if (client == -1)
    {
        cout << "socket 生成出错！" << endl;
        WSACleanup();
        return 0;
    }
    cout << "客户端套接字创建成功！" << endl;

    // 填充信息
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &serveraddr.sin_addr.s_addr);

    // 输入选项
    cout << "请输入选项告知——（发送0 / 接收1）数据：";
    int label;
    cin >> label;

    // 建立连接
    int length = sizeof(serveraddr);
    cout << "开始向服务端建立连接请求......" << endl;
    Client_Server_Connect(client, serveraddr, length, label);

    // 输入缓冲区大小
    cout << "请输入本端单次容纳数据缓冲区大小：";
    int SIZE;
    cin >> SIZE;
    cout << "请输入本端窗口缓冲区阈值大小：";
    int ssthresh;
    cin >> ssthresh;
    int* S_W = new int[2];
    SIZE = Client_Server_Size(client, serveraddr, length, SIZE, ssthresh, S_W);
    SIZE = S_W[0];
    ssthresh = S_W[1];
    cout << "双方协定的单次数据发送大小为：" << SIZE << "，窗口阈值大小为：" << ssthresh << endl;


    if (label == 0) {
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
                sendto(client, buffer, sizeof(packet), 0, (sockaddr*)&serveraddr, length);
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

            Send_Message(client, serveraddr, length, InFileName, strlen(InFileName), SIZE, ssthresh);
            clock_t start = clock();
            Send_Message(client, serveraddr, length, FileBuffer, F_length, SIZE, ssthresh);
            clock_t end = clock();
            cout << "传输总时间为:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
            cout << "吞吐率为:" << ((float)F_length) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
            cout << "=====================================================================" << endl;
        }
    }
    else {
        while (1) {
            cout << "=====================================================================" << endl;
            cout << "Waiting to receive!!!" << endl;
            char* F_name = new char[20];
            char* Message = new char[100000000];
            int name_len = Recv_Message(client, serveraddr, length, F_name, SIZE, ssthresh);

            // 全局结束
            if (name_len == 999)
                break;

            int file_len = Recv_Message(client, serveraddr, length, Message, SIZE, ssthresh);
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
    Client_Server_Disconnect(client, serveraddr, length);
    closesocket(client);
    WSACleanup();
    system("pause");
    return 0;
}