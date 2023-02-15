#include<iostream>
#include<fstream>
#include<Winsock2.h>
#include<string>
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996) 
using namespace std;
int quit = 0;
const int MAXSIZE = 8160;//传输缓冲区最大长度
unsigned int seq = 0;
unsigned int ack = 0;
SOCKET ListenSocket;
SOCKADDR_IN ServerAddr;
int l = sizeof(SOCKADDR);
#pragma pack(1)
struct HEADER {
	unsigned int Seq;
	unsigned short datasize;
	unsigned short Flags;
	unsigned short Checksum;
	char content[MAXSIZE];
	HEADER() {
		this->Seq = 0;
		this->datasize = 0;
		this->Flags = 0;
		this->Checksum = 0;
		for (int i = 0; i < MAXSIZE; i++)
			this->content[i] = 0;
	}

	void set_Checksum()
	{
		unsigned short* a = (unsigned short*)this;
		unsigned int sum = 0;
		for (int i = 0; i < sizeof(HEADER) / 2; i++) {
			sum += (unsigned int)a[i];
			if (sum & (0xFFFF0000)) {
				sum &= 0xFFFF;
				sum++;
			}
		}
		//return ~(((unsigned short)sum) & 0XFFFF);
		this->Checksum = ~(((unsigned short)sum) & 0XFFFF);
	}
	void set_SYN()
	{
		this->Flags |= 0x0001;
	}
	void set_ACK()
	{
		this->Flags |= 0x0002;
	}
	void set_FIN()
	{
		this->Flags |= 0x0004;
	}
	void set_EOF()
	{
		this->Flags |= 0x0010;
	}
	void set_NAK()
	{
		this->Flags |= 0x1000;
	}
	int get_SYN()
	{
		if (this->Flags & 0x0001)
			return 1;
		else
			return 0;
	}
	int get_ACK()
	{
		if (this->Flags & 0x0002)
			return 1;
		else
			return 0;
	}
	int get_FIN()
	{
		if (this->Flags & 0x0004)
			return 1;
		else
			return 0;
	}

	int get_EOF()
	{
		if (this->Flags & 0x0010)
			return 1;
		else
			return 0;
	}
	int get_NAK()
	{
		if (this->Flags & 0x1000)
			return 1;
		else
			return 0;
	}
	void clearcontent()
	{
		for (int i = 0; i < MAXSIZE; i++)
			this->content[i] = 0;
	}
	void setcontent(char* c, int len)
	{
		for (int i = 0; i < len; i++)
			this->content[i] = c[i];
	}
	bool check_Checksum()
	{
		this->set_Checksum();
		if (!this->Checksum)
			return true;
		else
			return false;
	}
};
#pragma pack(4)
void Connect()
{
	HEADER con_recv, send;
	send.set_ACK();
	send.set_SYN();
	
	//setsockopt(ListenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	int res = recvfrom(ListenSocket, (char*)&con_recv, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);//shakehands 1
	while (res < 0)
	{
		res = recvfrom(ListenSocket, (char*)&con_recv, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
	}
	if (con_recv.get_SYN())
	{
		sendto(ListenSocket, (char*)&send, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));//shakehands 2
		cout << "[SEND]  第二次握手消息     FLAG：SYN_ACK" << endl;
		res = recvfrom(ListenSocket, (char*)&con_recv, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);//shakehands 3
		while (res < 0)
		{
			sendto(ListenSocket, (char*)&send, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
			res = recvfrom(ListenSocket, (char*)&con_recv, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		}
		if (con_recv.get_ACK())
		{
			
			cout << "成功建立连接" << endl;
			
			//setsockopt(ListenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		}
	}
}
void DisConnect()
{
	HEADER cl_sign, cl_ans;
	cl_ans.Seq = seq;
	//cl_ans.Ack = ack;
	cl_ans.set_ACK();
	cl_ans.set_FIN();
	int res;
	//setsockopt(ListenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	res = sendto(ListenSocket, (char*)&cl_ans, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	if (res == sizeof(HEADER))
	{
		res = recvfrom(ListenSocket, (char*)&cl_sign, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		while (res < 0)
		{
			res = recvfrom(ListenSocket, (char*)&cl_sign, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		}
		if (res == sizeof(HEADER) && cl_sign.get_ACK())
		{
			quit = 1;
			cout << "成功断开连接" << endl;
		}
	}

}
void RECV_FILE()
{
	int numlength = 0;
	//想要得到的包的序列号
	ack = 0;
	HEADER recv_msg, answer;
	//收文件名
	char* mes = new char[20];
	int length=recvfrom(ListenSocket, mes, 20, 0, (SOCKADDR*)&ServerAddr, &l);
	
	string filename;
	for (int i = 0; i < length; i++)
	{
		filename = filename + mes[i];
	}
	ofstream fileout;
	fileout.open(filename, ofstream::binary);

	//cvfrom(ListenSocket, mes, 4, 0, (SOCKADDR*)&ServerAddr, &l);
	//cout << mes << length << endl;
	int resize = recvfrom(ListenSocket, (char*)&recv_msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
	
	//cout << "收到了" << endl;
	if (recv_msg.get_FIN() && recv_msg.get_ACK())
	{
		cout << "挥手" << endl;
		DisConnect();
		return;
	}
	if (recv_msg.check_Checksum())
	{
		fileout.write(recv_msg.content, recv_msg.datasize);
		ack = recv_msg.Seq + 1;
		cout << "[RECV]  SEQ: " << recv_msg.Seq<<"DATASIZE: "<<recv_msg.datasize << endl;
		numlength += recv_msg.datasize;

	}
	else
		answer.set_NAK();
	answer.set_ACK();
	answer.Seq = ack;

	sendto(ListenSocket, (char*)&answer, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	cout << "[SEND]  Ack: " << answer.Seq << endl;
	bool isend = false;
	while (!recv_msg.get_EOF())
	{
			
		recv_msg.clearcontent();

		int res = recvfrom(ListenSocket, (char*)&recv_msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		while (res < 0)
		{
			sendto(ListenSocket, (char*)&answer, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
			res = recvfrom(ListenSocket, (char*)&recv_msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		}
		if (recv_msg.Seq != ack)
		{
			sendto(ListenSocket, (char*)&answer, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
			cout << "[RECV] not want ,need to resend,SEQ:"<< recv_msg.Seq <<"send want:"<< answer.Seq << endl;
			continue;
		}
		
		if (recv_msg.get_FIN() && recv_msg.get_ACK())
		{
			DisConnect();
			return;
		}

		if (recv_msg.check_Checksum())
		{

			fileout.write(recv_msg.content, recv_msg.datasize);
			cout << "[RECV]  SEQ: " << recv_msg.Seq << "DATASIZE: " << recv_msg.datasize << endl;
			numlength += recv_msg.datasize;
			ack = recv_msg.Seq + 1;
		}
		else
		{
			answer.set_NAK();
			cout << "收到的包数据有误，WRONG CHECKNUM: " << recv_msg.Checksum << endl;
		}
			
		answer.set_ACK();
		answer.Seq = ack;
		sendto(ListenSocket, (char*)&answer, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
		cout << "[SEND]  Ack: " << answer.Seq << endl;
	}
	
	
	cout << "接收完毕" <<numlength<< endl;
	fileout.close();
}


int main()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA wsdata;
	int err = WSAStartup(version, &wsdata);
	if (err)
		cout << "初始化出错" << endl;
	ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);

	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(555);

	bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));

	Connect();
	while (!quit)
	{
		if (quit)
			break;
		RECV_FILE();
	}

	closesocket(ListenSocket);
	WSACleanup();


}