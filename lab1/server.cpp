#include<iostream>
#include<string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<map>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

#define MESSAGE_LEN 256

map<SOCKET, int> client_map;//map存储客户端状态，如果value=1，说明clinet在线
map<SOCKET, string> name_map;
DWORD WINAPI lpStartFun(LPVOID lparam) {

    //传进来的是接收到返回的socket     
    SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
    client_map[ClientSocket] = 1;     //该client在线
    //客户端输入呢，当成消息传过来,获取用户名
    char user_name[20];
    //strcpy_s(user_name, to_string(ClientSocket).data());
    //名字传给客户端
    //send(ClientSocket, user_name, 20, 0);
    recv(ClientSocket, user_name, 20, 0);
    name_map[ClientSocket] = string(user_name);

    SYSTEMTIME time;
    GetLocalTime(&time);
    cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  ";
    cout<< "用户：" << user_name << "加入聊天" << endl;

    int isquit = 1;//用户端是否退出
    int istoall = 1;//消息是否群发
    char recvMessage[MESSAGE_LEN];
    char sendMessage[MESSAGE_LEN];
    char speMessage[MESSAGE_LEN];//私聊消息

    int isRecv= recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);
    while (isRecv != SOCKET_ERROR && isquit != 0) {
        

        //接受到消息，输出日志，客户端与服务器端的交互
        SYSTEMTIME time;
        GetLocalTime(&time);
        cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [INFO]";
        cout << user_name << ":" << recvMessage << endl;

        istoall = 1;   //不设置就是群发
        if (isRecv > 0) {
            //char talk_to[20] = "";      //私聊的用户名
            for (int i = 0; i < MESSAGE_LEN; i++) {
                if (recvMessage[i] == '<') { //发现"<"即说明为私聊    消息类型是  目的用户<消息
                    istoall = 0;
                    break;
                }
            }
            
            if (istoall == 0) {
                
                char talk_to[20] = "";      //私聊的用户名
                int j;
                for (j = 0; j < MESSAGE_LEN; j++) {
                    if (recvMessage[j] == '<') {
                        talk_to[j] = '\0';
                        for (int z = j + 1; z < MESSAGE_LEN; z++) {
                            speMessage[z - j - 1] = recvMessage[z];
                            //if (recvMessage[z] == '\0')break;
                        }
                        strcat_s(speMessage, "( from ");
                        strcat_s(speMessage, user_name);
                        strcat_s(speMessage, ")\0");
                        break;
                    }
                    else {
                        talk_to[j] = recvMessage[j];
                    }
                }
                //cout << "私聊to" << talk_to << endl;
                //设置向客户端发送的消息  私聊消息  此时服务器端要记录
                for (auto it : client_map) {
                    //如果想要自定义用户名，这里也许再需要一个map，存储socket和用户名的映射
                    //cout << it.second<<endl;

                    if (name_map[it.first] == string(talk_to) && it.second == 1) {
                        //使用目的用户socket，进行消息发送
                        //cout << "string(name_map[it.first])" << name_map[it.first] << endl;
                        //cout << "string(talk_to)" << string(talk_to) << endl;
                        int isSend = send(it.first, speMessage, MESSAGE_LEN, 0);
                        if (isSend == SOCKET_ERROR) {
                            cout << "发送失败" << endl;
                            it.second = 0;
                        }
                        
                        else {
                            SYSTEMTIME time;
                            GetLocalTime(&time);
                            cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [INFO]";
                            cout << user_name << "消息发送成功"  << endl;

                        }
                        break;
                    }

                }


            }
            else {
                //strcpy_s(sendMessage, "用户");
                //string ClientID = to_string(ClientSocket);
                strcpy_s(sendMessage, user_name); // data函数直接转换为char*
                strcat_s(sendMessage, ": ");
                strcat_s(sendMessage, recvMessage);

                //把消息转发给除了发消息的人
                for (auto it : client_map) {
                    if (it.first != ClientSocket && it.second == 1) {
                        int isSend = send(it.first, sendMessage, MESSAGE_LEN, 0);
                        if (isSend == SOCKET_ERROR)
                        {
                            cout <<"[INFO]   发送到"<<name_map[it.first]<< "发送失败" << endl;
                        }
                        else {
                            SYSTEMTIME time;
                            GetLocalTime(&time);
                            cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [INFO]";
                            cout << user_name << "消息发送成功" << endl;

                        }
                    }
                }

            }
       
        }
        else {
            //收不到消息，代表退出       
            isquit = 0;
            client_map[ClientSocket] = 0;
        }
        //继续接受消息
        //char recvMessage[MESSAGE_LEN];
        isRecv = recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);

    }
   // SYSTEMTIME time;
    GetLocalTime(&time);
    cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [OUT]";
    cout << user_name << "离开了聊天室" << endl;

    closesocket(ClientSocket);
    return 0;
}

int main(){

    //SOCKET ClientSocket;
    WSADATA wsaData;
    // 使用2.2版   初始化socket  dll
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
        cout<<"初始化失败" << endl;
        return 0;
    }
    else {
        cout << "初始化成功" << endl;
    }

    // 创建监听socket
    SOCKET ListenSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(ListenSocket==INVALID_SOCKET){
        cout<<"建立socket失败"<<endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "建立监听socket成功"<< endl;
    }

    // 绑定socket 与ip地址和端口
    SOCKADDR_IN ServerAddr;  
    ServerAddr.sin_family = AF_INET;     //指定IP格式  
    USHORT uPort = 8093;                 //服务器监听端口  
    ServerAddr.sin_port = htons(uPort);   //绑定端口号  
    //ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //IP地址系统自动分配
    inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);
    if (bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)  //建立捆绑  
    {  
	        cout << "绑定失败";  
	        closesocket(ListenSocket);  
            WSACleanup();
	        return 0;  
    }
    else {
        cout << "绑定成功"<<endl;
    }

    // 启动监听
    if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		cout << "监听错误" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		//return 1;
	}
    else {
        cout << "监听成功" << endl;

    }

    // 等待客户端连接,使用多线程处理多个请求
    while (true)
    {
        SOCKADDR_IN ClientAddr;
        int len = sizeof(SOCKADDR_IN);
        //接受请求,返回交互socket,Addr传进去的时候是空的，accept会根据ListenSoc填充吗？？？？？
        SOCKET AcceptSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &len);
        if (AcceptSocket == INVALID_SOCKET) {
            cout << "接收客户端Socket失败" << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        else {
            // 创建线程，并且调用函数传入与client通讯的套接字
            HANDLE hThread = CreateThread(NULL, 0, lpStartFun, (LPVOID)AcceptSocket, 0, NULL);
            CloseHandle(hThread); // 关闭对线程的引用
        }
    }

    




 //   SOCKADDR_IN ClientAddr;
 //   int len=sizeof(SOCKADDR_IN);
 //   char buf[512] = { 0 };
 //   cout << "jiantingqian";
 //   SOCKET AcceptSocket=accept(ListenSocket, (SOCKADDR*)&ClientAddr, &len);
 //   cout << "AcceptSocket" << endl;
 //   if (AcceptSocket == INVALID_SOCKET) {
	//		cout << "接收错误" << endl;
	//		closesocket(ListenSocket);
	//		WSACleanup();
	//		return 1;
	//}
 //   else{
 //       // 接受数据
 //       /*char buf[512] = {0}*/;
 //       cout << "连接成功" << endl;
 //       int len = recv(AcceptSocket, buf, 512, 0);
 //       cout << buf << endl;
 //   }
 //   

 //   // 发送数据
 //   send(AcceptSocket,buf, len, 0);


 //   system("pause");
    closesocket(ListenSocket);
    //closesocket(AcceptSocket);
	WSACleanup();
	return 0;
}
    



