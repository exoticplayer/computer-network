#include<iostream>
#include<fstream>
#include<queue>
#include <WinSock2.h>
#include <string>
#include<time.h>
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996) 
using namespace std;

const int MAXSIZE = 8160;//传输缓冲区最大长度
int length = 0;
unsigned int seq = 0;
unsigned int ack = 0;
SOCKET ClientSocket;
SOCKADDR_IN ServerAddr;
int l = sizeof(SOCKADDR);
long long heads, tails, freqs;
double stime = 0;
const int NUM_WINDOW = 5;
const int MAX_TIME = 500;
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
int Connect()
{
	HEADER con_send, con_recv;

	con_send.set_SYN();
	sendto(ClientSocket, (char*)&con_send, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	cout<< "[SEND]  第一次握手消息     FLAG：SYN" << endl;
	int res = recvfrom(ClientSocket, (char*)&con_recv, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
	while (res < 0)
	{
		sendto(ClientSocket, (char*)&con_send, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
		res = recvfrom(ClientSocket, (char*)&con_recv, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
	}
	if (con_recv.get_ACK() && con_recv.get_SYN())
	{
		con_send.set_ACK();
		int res = sendto(ClientSocket, (char*)&con_send, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));//shakehands 3
		cout << "[SEND]  第三次握手消息     FLAG：ACK" << endl;
		if (res == sizeof(HEADER))
			cout << "成功建立连接" << endl;
		return 1;
	}
	else {
		cout << "建立连接失败" << endl;
		return 0;
	}

}

void SEND()
{
	queue<pair<int, int>> time_list;//存入timer记录发送出去的时间点, order
	HEADER msg, result;

	cout << "请输入文件名：" << endl;
	string filename = "";
	cin >> filename;
	// 发送文件名
	sendto(ClientSocket,(char*)(filename.c_str()), filename.length(),0,(SOCKADDR*)&ServerAddr,l);

	//读文件
	char** sendbuf = new char* [80000];
	for (int i = 0; i < 80000; i++)
		sendbuf[i] = new char[MAXSIZE];
	
	ifstream filein;
	filein.open(filename, ifstream::binary);
	cout << "文件是否打开：" << filein.is_open() << endl;
	while (!filein.is_open())
	{
		cout << "找不到该文件，请重新输入，或确认文件是否存在" << endl;
		cin >> filename;
		filein.open(filename, ifstream::binary);
		if (filein.is_open())
			cout << "文件已打开" << endl;
	}

	filein.seekg(0, filein.end);  //将文件流指针定位到流的末尾
	length = filein.tellg();
	cout << "文件长度：" << length << endl;

	filein.seekg(0, filein.beg);  //将文件流指针重新定位到流的开始
	
	int pagenum = 0;
	int len = length;
	while (len > 0)
	{
		filein.read(sendbuf[pagenum], min(len, MAXSIZE));
		len -= MAXSIZE;
		pagenum++;
	}

	
	filein.close();
	int save_pagenum = pagenum;
	clock_t start = clock();

	//未得到确认的第一个包
	int base = 0;
	//发送包的最后一个
	int tail = 0;
	while (pagenum > 0)
	{
		if (time_list.size() < NUM_WINDOW)
		{
			if (pagenum == 1)
				msg.set_EOF();
			msg.Seq = tail; 
			msg.datasize = min(MAXSIZE, length); 
			msg.clearcontent();
			msg.setcontent(sendbuf[tail], min(MAXSIZE, length));
			
			msg.Checksum = 0;
			msg.set_Checksum(); 
			//cout << msg.Checksum << endl;
			setsockopt(ClientSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuf, sizeof(char) * MAXSIZE);
			sendto(ClientSocket, (char*)&msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
			cout << "[Send]  " << msg.datasize << " bytes!" << " SEQ:" << int(msg.Seq) << " SUM:" << int(msg.Checksum) << endl;
			time_list.push(make_pair(clock(), msg.Seq));//开始计时，在末尾存入一个新元素
			//cout << clock() << endl;
			tail++;
		}
		cout << "[WINDOW_AREA] :" << base << "to" << base + NUM_WINDOW - 1 << endl;
		int res = recvfrom(ClientSocket, (char*)&result, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		//cout << " result'Ack：" << result.Ack << " " << endl;
		
		if (result.get_ACK() && !result.get_NAK() && result.Seq - 1 == time_list.front().second)
		{
			while (time_list.size() > 0 && result.Seq == time_list.front().second + 1)
			{
				cout << "[RECV]  SEQ: " << result.Seq << "   FLAG: ACK"   << endl;
				pagenum--;
				length -= MAXSIZE;

				base++;
				time_list.pop();
			}
		}
		else
		{
			if (clock() - time_list.front().first > MAX_TIME)
			{
				cout << "TIME OUT!" << endl;
				tail = base;
				while (time_list.size() != 0)
					time_list.pop();
			}
		}
	}

	
	clock_t end = clock();

	stime = (end-start)/ CLOCKS_PER_SEC;
	cout << "文件发送完毕" << endl;
	cout << "共计用时" << stime << "ms" << endl;
	double throupt_rate = save_pagenum * sizeof(HEADER) / 1024 / stime * 1000;
	cout << "吞吐率为" << throupt_rate << "Mb/s" << endl;
	cout << "传输完毕" << endl;
	for (int i = 0; i < 65536; i++)
		delete[]sendbuf[i];
	delete[]sendbuf;
}

int DisConnect()
{
	HEADER Sign, Result;

	Sign.set_ACK();
	Sign.set_FIN();
	//wavehands 1
	sendto(ClientSocket, (char*)&Sign, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	cout << "[SEND]  第一次挥手消息     FLAG：ACK_FIN" << endl;
	int res = recvfrom(ClientSocket, (char*)&Result, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
	while (res < 0)
	{
		sendto(ClientSocket, (char*)&Sign, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
		res = recvfrom(ClientSocket, (char*)&Result, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
	}
	if (res == sizeof(HEADER) && Result.get_ACK() && Result.get_FIN())
	{
		Sign.set_ACK();
		res = sendto(ClientSocket, (char*)&Sign, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
		cout << "[SEND]  第三次挥手消息     FLAG：ACK" << endl;
		if (res == sizeof(HEADER))
		{
			cout << "成功断开连接" << endl;
			return 1;

		}
		return 0;
	}
	else
		return 0;
}
int main()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA wsdata;
	int err = WSAStartup(version, &wsdata);
	if (err)
		cout << "wrong" << endl;
	ClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	int tv = 300;
	setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(666);
	Connect();
	while (1)
	{
		cout << "请输入操作，q结束，任意字符继续：" << endl;
		char a;
		cin >> a;
		if (a == 'q')
		{
			DisConnect();
			break;
		}
		SEND();
	}
	closesocket(ClientSocket);
	WSACleanup();

}