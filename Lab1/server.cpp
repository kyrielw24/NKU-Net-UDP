#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string.h>
using namespace std;
#pragma comment(lib, "ws2_32.lib")
#define PORT 5005

struct threadsocket
{
    char* name;
    SOCKET client;
};

DWORD WINAPI ClientThread(LPVOID ThreadParameter)
{
    threadsocket* clientT = (threadsocket*)ThreadParameter;
    SOCKET client = (SOCKET)(clientT->client);
    char* name = clientT->name;
    char buffer[1024];

    while (1)
    {
        int iret;
        memset(buffer, 0, sizeof(buffer));
        iret = recv(client, buffer, sizeof(buffer), 0);
        if (iret <= 0)
        {
            cout << name << "退出聊天室！" << endl;
            break;
        }
        // 聊天室输出各位用户的发送信息
        cout << buffer << endl;

        strcpy(buffer, "您的 message 成功发送在聊天室！");
        iret = send(client, buffer, sizeof(buffer), 0); // 向客户端发送响应结果
        cout << endl;
    }
    return 0;
}


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

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        cout << "socket 生成出错！" << endl;
        WSACleanup();
        return 0;
    }

    // 2. 绑定端口和ip
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        cout << "bind 出错！" << endl;
        WSACleanup();
        return 0;
    }

    if (listen(server, 5) != 0)
    {
        cout << "listen 监听出错！" << endl;
        WSACleanup();
        return 0;
    }

    // 主线程循环接收客户端的连接
    while (1)
    {
        sockaddr_in addrClient;
        int socketlen = sizeof(sockaddr_in);
        // 4.接受成功返回与client通讯的Socket
        SOCKET client = accept(server, (SOCKADDR*)&addrClient, (socklen_t*)&socketlen);
        if (client != INVALID_SOCKET)
        {
            char name[10];
            memset(name, 0, sizeof(name));
            recv(client, name, sizeof(name), 0);
            cout << name << "已经进入聊天室！" << endl;
            // 创建线程，并且传入与client通讯的套接字
            threadsocket thread_client;
            thread_client.client = client;
            thread_client.name = name;

            // 创建线程
            HANDLE hThread = CreateThread(NULL, 0, ClientThread, &thread_client, 0, NULL);
            CloseHandle(hThread); // 关闭对线程的引用
        }
    }

    closesocket(server);
    WSACleanup();
    return 0;
}