#include<iostream>
#include<WinSock2.h>
#include<string>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

#define MESSAGE_LEN 256
int isquit = 1;
char user_name[20];

DWORD WINAPI Recv(LPVOID lparam_socket) {
    SOCKET* recvSocket = (SOCKET*)lparam_socket;
    while (true)
    {
        char RecvMessage[MESSAGE_LEN];
        int isRecv = recv(*recvSocket, RecvMessage, MESSAGE_LEN, 0);
        if (isRecv > 0 && isquit == 1) {
            SYSTEMTIME time;
            GetLocalTime(&time);
            cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  ";
            cout << RecvMessage << endl;
        }
        else {
            //���ܲ�����Ϣ���ر�socket
            closesocket(*recvSocket);
            return 0;
        }
    }

}
DWORD WINAPI Send(LPVOID lparam_socket) {
    SOCKET* sendSocket = (SOCKET*)lparam_socket;
    while (true)
    {
        cout << "��������Ҫ���͵���Ϣ" << endl;
        char sendMessage[MESSAGE_LEN];
        cin.getline(sendMessage, MESSAGE_LEN);//getline����ʶ��ո����������Զ�����
        if (string(sendMessage) == "quit") {
            isquit = 0;
            closesocket(*sendSocket);  //�ر�socket����ʱ�������� isRecv�ͻ���ִ��󣬽����ر�ͨ��
            cout << "�˳�Ⱥ��" << endl;
            WSACleanup();//����WSACleanup�����������Socket��İ󶨲����ͷ�Socket����ռ�õ�ϵͳ��Դ
            return 0;
        }
        else {
            int isSend = send(*sendSocket, sendMessage, MESSAGE_LEN, 0);
            if (isSend == SOCKET_ERROR) {
                cout << "������Ϣʧ��:" << WSAGetLastError() << endl;
                closesocket(*sendSocket);
                WSACleanup();//����WSACleanup�����������Socket��İ󶨲����ͷ�Socket����ռ�õ�ϵͳ��Դ
                return 0;
            }
        }

    }


}
int main() 
{
    WSADATA wsaData;
    // ʹ��2.2��   ��ʼ��socket  dll
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "��ʼ��ʧ��" << endl;
        return 0;
    }
    else {
        cout << "��ʼ���ɹ�" << endl;
    }

     /* ����Socket */
    SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET)
    {
        cout << "�ͻ���Socket����ʧ��" << endl;
        WSACleanup();
        return 0;
    }
 
    /* ���ӷ����    IP��ַ���˿ںź�server��һ�� */
    //struct sockaddr_in addr = {0};
    //addr.sin_family         = AF_INET;
    //addr.sin_port           = htons(8093);
    ////addr.sin_addr.s_addr    = inet_addr("127.0.0.1");
    //inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    SOCKADDR_IN ServerAddr;
    ServerAddr.sin_family = AF_INET;     //ָ��IP��ʽ  
    USHORT uPort = 8093;                 //�����������˿�  
    ServerAddr.sin_port = htons(uPort);   //�󶨶˿ں�  
    //ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //IP��ַϵͳ�Զ�����
    inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);
    
    
    
    int isError=connect(ClientSocket, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
    if (isError == SOCKET_ERROR) {
        cout << "���ӷ����ʧ��" << endl;
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    else {
        cout << "���ӷ���˳ɹ�" << endl;
    }
    //�Ƚ���send�������û���
    //�����û��������Ҵ����ͻ���
    cout << "�������û���: ";
    cin.getline(user_name, 20);
    send(ClientSocket, user_name, 20, 0);
    //recv(ClientSocket, user_name, 20, 0);
    
    // ��ӡ��������ı�־
    cout << "              Welcome    User    " << user_name << endl;
    cout << "*****************************************************" << endl;
    cout << "             Use quit command to quit" << endl;
    cout << "*****************************************************" << endl;

    // ���������̣߳�һ�������̣߳�һ�������߳�
    HANDLE hThread[2];
    hThread[0] = CreateThread(NULL, 0, Recv, (LPVOID)&ClientSocket, 0, NULL);
    hThread[1] = CreateThread(NULL, 0, Send, (LPVOID)&ClientSocket, 0, NULL);
 
    /*char buf[] = "My Name is Weiyyi, I am 2 months old.";
    cout << "buf" << buf<<endl;
    int len=send(sock, buf, sizeof(buf), 0);
    if (len > 0) {
        cout << "coutbuf" << buf << endl;
    }
 
    char recvBuf[128] = {0};
    recv(sock, recvBuf, 128, 0);
    while (1) {

    }
    cout << recvBuf;
    system("pause");
    closesocket(sock);*/
    WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
    CloseHandle(hThread[0]);
    CloseHandle(hThread[1]);

    // �ر�socket
    closesocket(ClientSocket);
    WSACleanup();
 
    return 0;
}