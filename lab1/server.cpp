#include<iostream>
#include<string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<map>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

#define MESSAGE_LEN 256

map<SOCKET, int> client_map;//map�洢�ͻ���״̬�����value=1��˵��clinet����
map<SOCKET, string> name_map;
DWORD WINAPI lpStartFun(LPVOID lparam) {

    //���������ǽ��յ����ص�socket     
    SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
    client_map[ClientSocket] = 1;     //��client����
    //�ͻ��������أ�������Ϣ������,��ȡ�û���
    char user_name[20];
    //strcpy_s(user_name, to_string(ClientSocket).data());
    //���ִ����ͻ���
    //send(ClientSocket, user_name, 20, 0);
    recv(ClientSocket, user_name, 20, 0);
    name_map[ClientSocket] = string(user_name);

    SYSTEMTIME time;
    GetLocalTime(&time);
    cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  ";
    cout<< "�û���" << user_name << "��������" << endl;

    int isquit = 1;//�û����Ƿ��˳�
    int istoall = 1;//��Ϣ�Ƿ�Ⱥ��
    char recvMessage[MESSAGE_LEN];
    char sendMessage[MESSAGE_LEN];
    char speMessage[MESSAGE_LEN];//˽����Ϣ

    int isRecv= recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);
    while (isRecv != SOCKET_ERROR && isquit != 0) {
        

        //���ܵ���Ϣ�������־���ͻ�����������˵Ľ���
        SYSTEMTIME time;
        GetLocalTime(&time);
        cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [INFO]";
        cout << user_name << ":" << recvMessage << endl;

        istoall = 1;   //�����þ���Ⱥ��
        if (isRecv > 0) {
            //char talk_to[20] = "";      //˽�ĵ��û���
            for (int i = 0; i < MESSAGE_LEN; i++) {
                if (recvMessage[i] == '<') { //����"<"��˵��Ϊ˽��    ��Ϣ������  Ŀ���û�<��Ϣ
                    istoall = 0;
                    break;
                }
            }
            
            if (istoall == 0) {
                
                char talk_to[20] = "";      //˽�ĵ��û���
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
                //cout << "˽��to" << talk_to << endl;
                //������ͻ��˷��͵���Ϣ  ˽����Ϣ  ��ʱ��������Ҫ��¼
                for (auto it : client_map) {
                    //�����Ҫ�Զ����û���������Ҳ������Ҫһ��map���洢socket���û�����ӳ��
                    //cout << it.second<<endl;

                    if (name_map[it.first] == string(talk_to) && it.second == 1) {
                        //ʹ��Ŀ���û�socket��������Ϣ����
                        //cout << "string(name_map[it.first])" << name_map[it.first] << endl;
                        //cout << "string(talk_to)" << string(talk_to) << endl;
                        int isSend = send(it.first, speMessage, MESSAGE_LEN, 0);
                        if (isSend == SOCKET_ERROR) {
                            cout << "����ʧ��" << endl;
                            it.second = 0;
                        }
                        
                        else {
                            SYSTEMTIME time;
                            GetLocalTime(&time);
                            cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [INFO]";
                            cout << user_name << "��Ϣ���ͳɹ�"  << endl;

                        }
                        break;
                    }

                }


            }
            else {
                //strcpy_s(sendMessage, "�û�");
                //string ClientID = to_string(ClientSocket);
                strcpy_s(sendMessage, user_name); // data����ֱ��ת��Ϊchar*
                strcat_s(sendMessage, ": ");
                strcat_s(sendMessage, recvMessage);

                //����Ϣת�������˷���Ϣ����
                for (auto it : client_map) {
                    if (it.first != ClientSocket && it.second == 1) {
                        int isSend = send(it.first, sendMessage, MESSAGE_LEN, 0);
                        if (isSend == SOCKET_ERROR)
                        {
                            cout <<"[INFO]   ���͵�"<<name_map[it.first]<< "����ʧ��" << endl;
                        }
                        else {
                            SYSTEMTIME time;
                            GetLocalTime(&time);
                            cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [INFO]";
                            cout << user_name << "��Ϣ���ͳɹ�" << endl;

                        }
                    }
                }

            }
       
        }
        else {
            //�ղ�����Ϣ�������˳�       
            isquit = 0;
            client_map[ClientSocket] = 0;
        }
        //����������Ϣ
        //char recvMessage[MESSAGE_LEN];
        isRecv = recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);

    }
   // SYSTEMTIME time;
    GetLocalTime(&time);
    cout << time.wHour << ":" << time.wMinute << ":" << time.wSecond << "  [OUT]";
    cout << user_name << "�뿪��������" << endl;

    closesocket(ClientSocket);
    return 0;
}

int main(){

    //SOCKET ClientSocket;
    WSADATA wsaData;
    // ʹ��2.2��   ��ʼ��socket  dll
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
        cout<<"��ʼ��ʧ��" << endl;
        return 0;
    }
    else {
        cout << "��ʼ���ɹ�" << endl;
    }

    // ��������socket
    SOCKET ListenSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(ListenSocket==INVALID_SOCKET){
        cout<<"����socketʧ��"<<endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "��������socket�ɹ�"<< endl;
    }

    // ��socket ��ip��ַ�Ͷ˿�
    SOCKADDR_IN ServerAddr;  
    ServerAddr.sin_family = AF_INET;     //ָ��IP��ʽ  
    USHORT uPort = 8093;                 //�����������˿�  
    ServerAddr.sin_port = htons(uPort);   //�󶨶˿ں�  
    //ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //IP��ַϵͳ�Զ�����
    inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);
    if (bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)  //��������  
    {  
	        cout << "��ʧ��";  
	        closesocket(ListenSocket);  
            WSACleanup();
	        return 0;  
    }
    else {
        cout << "�󶨳ɹ�"<<endl;
    }

    // ��������
    if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		cout << "��������" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		//return 1;
	}
    else {
        cout << "�����ɹ�" << endl;

    }

    // �ȴ��ͻ�������,ʹ�ö��̴߳���������
    while (true)
    {
        SOCKADDR_IN ClientAddr;
        int len = sizeof(SOCKADDR_IN);
        //��������,���ؽ���socket,Addr����ȥ��ʱ���ǿյģ�accept�����ListenSoc����𣿣�������
        SOCKET AcceptSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &len);
        if (AcceptSocket == INVALID_SOCKET) {
            cout << "���տͻ���Socketʧ��" << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        else {
            // �����̣߳����ҵ��ú���������clientͨѶ���׽���
            HANDLE hThread = CreateThread(NULL, 0, lpStartFun, (LPVOID)AcceptSocket, 0, NULL);
            CloseHandle(hThread); // �رն��̵߳�����
        }
    }

    




 //   SOCKADDR_IN ClientAddr;
 //   int len=sizeof(SOCKADDR_IN);
 //   char buf[512] = { 0 };
 //   cout << "jiantingqian";
 //   SOCKET AcceptSocket=accept(ListenSocket, (SOCKADDR*)&ClientAddr, &len);
 //   cout << "AcceptSocket" << endl;
 //   if (AcceptSocket == INVALID_SOCKET) {
	//		cout << "���մ���" << endl;
	//		closesocket(ListenSocket);
	//		WSACleanup();
	//		return 1;
	//}
 //   else{
 //       // ��������
 //       /*char buf[512] = {0}*/;
 //       cout << "���ӳɹ�" << endl;
 //       int len = recv(AcceptSocket, buf, 512, 0);
 //       cout << buf << endl;
 //   }
 //   

 //   // ��������
 //   send(AcceptSocket,buf, len, 0);


 //   system("pause");
    closesocket(ListenSocket);
    //closesocket(AcceptSocket);
	WSACleanup();
	return 0;
}
    



