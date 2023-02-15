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
            //接受不到消息，关闭socket
            closesocket(*recvSocket);
            return 0;
        }
    }

}
DWORD WINAPI Send(LPVOID lparam_socket) {
    SOCKET* sendSocket = (SOCKET*)lparam_socket;
    while (true)
    {
        cout << "请输入你要发送的消息" << endl;
        char sendMessage[MESSAGE_LEN];
        cin.getline(sendMessage, MESSAGE_LEN);//getline可以识别空格，遇到换行自动结束
        if (string(sendMessage) == "quit") {
            isquit = 0;
            closesocket(*sendSocket);  //关闭socket，此时服务器端 isRecv就会出现错误，进而关闭通信
            cout << "退出群聊" << endl;
            WSACleanup();//调用WSACleanup函数来解除与Socket库的绑定并且释放Socket库所占用的系统资源
            return 0;
        }
        else {
            int isSend = send(*sendSocket, sendMessage, MESSAGE_LEN, 0);
            if (isSend == SOCKET_ERROR) {
                cout << "发送信息失败:" << WSAGetLastError() << endl;
                closesocket(*sendSocket);
                WSACleanup();//调用WSACleanup函数来解除与Socket库的绑定并且释放Socket库所占用的系统资源
                return 0;
            }
        }

    }


}
int main() 
{
    WSADATA wsaData;
    // 使用2.2版   初始化socket  dll
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "初始化失败" << endl;
        return 0;
    }
    else {
        cout << "初始化成功" << endl;
    }

     /* 创建Socket */
    SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET)
    {
        cout << "客户端Socket创建失败" << endl;
        WSACleanup();
        return 0;
    }
 
    /* 连接服务端    IP地址，端口号和server中一致 */
    //struct sockaddr_in addr = {0};
    //addr.sin_family         = AF_INET;
    //addr.sin_port           = htons(8093);
    ////addr.sin_addr.s_addr    = inet_addr("127.0.0.1");
    //inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    SOCKADDR_IN ServerAddr;
    ServerAddr.sin_family = AF_INET;     //指定IP格式  
    USHORT uPort = 8093;                 //服务器监听端口  
    ServerAddr.sin_port = htons(uPort);   //绑定端口号  
    //ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //IP地址系统自动分配
    inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);
    
    
    
    int isError=connect(ClientSocket, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
    if (isError == SOCKET_ERROR) {
        cout << "连接服务端失败" << endl;
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    else {
        cout << "连接服务端成功" << endl;
    }
    //先接收send过来的用户名
    //输入用户名，并且传给客户端
    cout << "请输入用户名: ";
    cin.getline(user_name, 20);
    send(ClientSocket, user_name, 20, 0);
    //recv(ClientSocket, user_name, 20, 0);
    
    // 打印进入聊天的标志
    cout << "              Welcome    User    " << user_name << endl;
    cout << "*****************************************************" << endl;
    cout << "             Use quit command to quit" << endl;
    cout << "*****************************************************" << endl;

    // 创建两个线程，一个接受线程，一个发送线程
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

    // 关闭socket
    closesocket(ClientSocket);
    WSACleanup();
 
    return 0;
}