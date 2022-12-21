#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "Packet.h"
#include "Process.h"
#include <string.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define PORT 5005
#define IP "127.0.0.1"

// ���ܰ������������ӡ�����⡢ȷ���ش��ȡ�
// �������Ʋ���ͣ�Ȼ���

int main()
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        cout << "Startup ����" << endl;
        WSACleanup();
        return 0;
    }

    SOCKET server = socket(AF_INET, SOCK_DGRAM, 0);
    if (server == -1)
    {
        cout << "socket ���ɳ���" << endl;
        WSACleanup();
        return 0;
    }
    cout << "������׽��ִ����ɹ���" << endl;

    // �󶨶˿ں�ip
    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &addr.sin_addr.s_addr);
    
    // ��������
    if (bind(server, (SOCKADDR*)&addr, sizeof(addr)) != 0)
    {
        cout << "bind ����" << endl;
        WSACleanup();
        return 0;
    }
    int length = sizeof(addr);
    cout << "�ȴ��ͻ��������������......" << endl;
    int label = Client_Server_Connect(server, addr, length);
    cout << "�����뱾�����ݻ�������С��";
    int SIZE;
    cin >> SIZE;
    SIZE = Client_Server_Size(server, addr, length, SIZE);

    if (label == 0) {
        while (1) {
            cout << "=====================================================================" << endl;
            cout << "Waiting to receive!!!" << endl;
            char* F_name = new char[20];
            char* Message = new char[100000000];
            int name_len = Recv_Messsage(server, addr, length, F_name, SIZE);

            // ȫ�ֽ���
            if (name_len == 999)
                break;

            int file_len = Recv_Messsage(server, addr, length, Message, SIZE);
            string a;
            for (int i = 0; i < name_len; i++) {
                a = a + F_name[i];
            }
            cout << "�����ļ�����" << a << endl;
            cout << "�����ļ����ݴ�С��" << file_len << "bytes!" << endl;

            FILE* fp;
            if ((fp = fopen(a.c_str(), "wb")) == NULL)
            {
                cout << "Open File failed!" << endl;
                exit(0);
            }
            //��buffer��д���ݵ�fpָ����ļ���
            fwrite(Message, file_len, 1, fp);
            cout << "Successfully receive the data in its entirety and save it" << endl;
            cout << "=====================================================================" << endl;
            //�ر��ļ�ָ��
            fclose(fp);
        }
    }
    if (label == 1) {
        while (1) {
            cout << "=====================================================================" << endl;
            cout << "Waiting to choose File!!!" << endl;
            //�����ļ���
            char InFileName[20];
            //�ļ����ݳ���
            int F_length;
            //����Ҫ��ȡ��ͼ����
            cout << "Enter File name:";
            cin >> InFileName;

            // ȫ�ֽ���
            if (InFileName[0] == 'q' && strlen(InFileName) == 1) {
                Packet_Header packet;
                char* buffer = new char[sizeof(packet)];  // ����buffer
                packet.tag = END;
                packet.checksum = compute_sum((WORD*)&packet, sizeof(packet));
                memcpy(buffer, &packet, sizeof(packet));
                sendto(server, buffer, sizeof(packet), 0, (sockaddr*)&addr, length);
                cout << "Send a all_end flag to the server" << endl;
                break;
            }

            //�ļ�ָ��
            FILE* fp;
            //�Զ����Ʒ�ʽ��ͼ��
            if ((fp = fopen(InFileName, "rb")) == NULL)
            {
                cout << "Open file failed!" << endl;
                exit(0);
            }
            //��ȡ�ļ������ܳ���
            fseek(fp, 0, SEEK_END);
            F_length = ftell(fp);
            rewind(fp);
            //�����ļ����ݳ��ȷ����ڴ�buffer
            char* FileBuffer = (char*)malloc(F_length * sizeof(char));
            //��ͼ�����ݶ���buffer
            fread(FileBuffer, F_length, 1, fp);
            fclose(fp);

            cout << "�����ļ����ݴ�С��" << F_length << "bytes!" << endl;

            Send_Message(server, addr, length, InFileName, strlen(InFileName), SIZE);
            clock_t start = clock();
            Send_Message(server, addr, length, FileBuffer, F_length, SIZE);
            clock_t end = clock();
            cout << "������ʱ��Ϊ:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
            cout << "������Ϊ:" << ((float)F_length) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
            cout << "=====================================================================" << endl;
        }
    }
    Client_Server_Disconnect(server, addr, length);
    closesocket(server);
    WSACleanup();
    system("pause");
    return 0;
}