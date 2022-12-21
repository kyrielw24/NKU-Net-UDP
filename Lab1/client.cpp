//#include <stdio.h>
//#include <stdlib.h>
//#include<string>
//#include <sys/types.h>
//#include <string.h>
//#include <WinSock2.h>
//#include <ws2tcpip.h>
//#include <windows.h>
//#include<iostream>
//#pragma comment(lib, "ws2_32.lib")
//using namespace std;
//
//USHORT PORT = 5005;
//#define IP "127.0.0.1"
//
//int main()
//{
//    WORD wVersionRequested = MAKEWORD(2, 2);
//    WSADATA wsaData;
//    int err = WSAStartup(wVersionRequested, &wsaData);
//    if (err != 0)
//    {
//        cout << "Startup 出错！" << endl;
//        WSACleanup();
//        return 0;
//    }
//    SOCKET client = socket(AF_INET, SOCK_STREAM, 0);
//    if (client == -1)
//    {
//        cout << "socket 生成出错！" << endl;
//        WSACleanup();
//        return 0;
//    }
//    struct hostent* host = gethostbyname(IP);
//    if (!host) {
//        puts("Get IP address error!");
//        WSACleanup();
//        return 0;
//    }
//    struct sockaddr_in serveraddr;
//    memset(&serveraddr, 0, sizeof(serveraddr));
//    serveraddr.sin_family = AF_INET;
//    serveraddr.sin_port = htons(PORT);
//    memcpy(&serveraddr.sin_addr, host->h_addr_list[0], host->h_length);
//    if (connect(client, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) != 0)
//    {
//        cout << "connect 连接出错！" << endl;
//        WSACleanup();
//        return 0;
//    }
//
//    char name[10];  //用户姓名信息
//    cout << "请输入用户名称：";
//    cin >> name;
//    send(client, name, strlen(name), 0);
//
//    char buf[1024];  //用户聊天输入信息
//    char tmp[64];
//    char message[512];
//    bool E = false;
//    while (1)
//    {
//        memset(buf, 0, sizeof(buf));
//        cout << "请输入聊天message：";
//        memset(message, 0, sizeof(message));
//        memset(tmp, 0, sizeof(tmp));
//        cin >> message;
//        time_t t = time(0);
//
//        if (strcmp(message, "bye!") == 0) {
//            E = true;
//        }
//
//        strftime(tmp, sizeof(tmp), "%Y/%m/%d %X %A ", localtime(&t));
//        strcpy(buf, "Time:");
//        strcat(buf, tmp);
//        strcat(buf, "\n用户 ");
//        strcat(buf, name);
//        strcat(buf, " : ");
//        strcat(buf, message);
//        if (send(client, buf, strlen(buf), 0) <= 0) {
//            cout << "客户端数据发送失败！！！" << endl;
//            break;
//        }
//
//        //接收客户端回传消息
//        memset(buf, 0, sizeof(buf));
//        if (recv(client, buf, sizeof(buf), 0) <= 0)
//        {
//            cout << "服务端数据接收失败！！！" << endl;
//            break;
//        }
//        cout << buf << endl;
//
//        if (E)
//            break;
//    }
//
//    closesocket(client);
//    WSACleanup();
//    return 0;
//}