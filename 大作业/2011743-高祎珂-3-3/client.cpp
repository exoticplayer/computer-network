#include<iostream>
#include<fstream>
#include<queue>
#include <WinSock2.h>
#include <string>
#include<time.h>
#include<vector>
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996) 
using namespace std;

const int MAXSIZE = 8160;//传输缓冲区最大长度
int lastack;
unsigned int seq = 0;
unsigned int ack = 0;
SOCKET ClientSocket;
SOCKADDR_IN ServerAddr;
int l = sizeof(SOCKADDR);
long long heads, tails, freqs;
double stime = 0;
const int MAX_TIME = 500;
int time_list[500];
bool istimeout = false;
bool isrepluciteack = false;
bool isend = false;

//发送端缓存
char** sendbuf;
//vector<pair<int, int>> time_list;//存入timer记录发送出去的时间点, order
//未得到确认的第一个包
int base = 0;
//发送包的最后一个
int tail = 0;
//剩下的要发送包数
int pagenum = 0;
//保存数据包总数
int save_pagenum;
//文件长度
int length = 0;
//最后一个包的长度
int last_length = 0;
int cwnd = 1;
int sshstrech = 20;
int NUM_WINDOW = cwnd;
int MAXTIME = 50000;
int repulicateack = 0;

HANDLE recver, clocker;

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
void TimeCheck() {
	while (true)
	{
		if (base == save_pagenum) {
			return;
		}
		//cout << "time_list[base]*****************" << time_list[base] << endl;
		clock_t nowclock = clock();
		if (time_list[base]!=0&&nowclock-time_list[base]>MAX_TIME)
		{
			//cout << "now clock" << nowclock << " time_list["<<base<<"] " << time_list[base] << "  时间差   " << int(nowclock - time_list[base]) << endl;
				//重复ack
				istimeout = true;
				if (isrepluciteack) {
					sshstrech /= 2;
					NUM_WINDOW = sshstrech + 3;
				}
				else {
					sshstrech /= 2;
					NUM_WINDOW = 1;

				}

				SetPriorityClass(clocker, HIGH_PRIORITY_CLASS);
				//istimeout = true;

				cout << "超时重传 Seq= from " << base << " to " << tail << endl;
				cout << "---------------------------------------------------------" << endl;
				for (int i = base; i < tail; i++)   //即发出：SendBase ~ Seq-1 的报文
				{
					HEADER msg;
					msg.Seq = i;
					

					msg.clearcontent();
					if (i != save_pagenum - 1)
					{
						msg.setcontent(sendbuf[i], MAXSIZE);
						msg.datasize = MAXSIZE;
					}
					else
					{
						//return;
						//msg.set_EOF();
						msg.setcontent(sendbuf[i], last_length);
						//cout << "last_length: " << last_length << endl;
						msg.datasize = last_length;
					}


					msg.Checksum = 0;
					msg.set_Checksum();
					//cout << msg.Checksum << endl;
					//setsockopt(ClientSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuf, sizeof(char) * MAXSIZE);
					sendto(ClientSocket, (char*)&msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
					cout << "重发包的大小" << msg.datasize << " 发送包的缓存区数：" << i << " 剩余长度 ：" << length << endl;
					
					time_list[i] = clock();
					//time_list[i-base]=make_pair(clock(), msg.Seq);
					//time_list.pop();
				}
				// 恢复线程并行
				istimeout = false;
				SetPriorityClass(clocker, NORMAL_PRIORITY_CLASS);
				
		}
	}
}
void Recvpac() {
	clocker = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)TimeCheck, NULL, NULL, NULL);
	
	while (true) {
		HEADER result;

		int res = recvfrom(ClientSocket, (char*)&result, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, &l);
		//if (res > 0)
			//cout << "收到了[RECV]  SEQ: " << result.Seq - 1 << endl;

		//cout << "here1 time_list:"<< time_list.size() << endl;

			//接收到的包没错，且是想要的
		if (result.get_ACK() && !result.get_NAK() && result.Seq == base + 1)
		{

			time_list[base] = MAXTIME;
			//cout << "改变time_list["<<base<<"]为 *************" << time_list[base] << endl;
			pagenum--;

			base++;
			if (base == save_pagenum)
			{
				isend = true;
				//time_list.clear();
				//cout << "here" << endl;
				return;
			}

			if (NUM_WINDOW <= sshstrech)
				NUM_WINDOW *= 2;
			else
				NUM_WINDOW += 1;
			cout << "[RECV]  SEQ: " << result.Seq - 1 << "   FLAG: ACK WANT:" << base - 1 << "当前窗口" << base << " to " << tail << "窗口大小" << NUM_WINDOW << endl;
		}
		//丢包路由器中会截服务器端传回的包
		else if (result.Seq > base + 1) {
			base = result.Seq - 1;
			time_list[base] = MAXTIME;
		}
	
		else {
			cout << "[RECV]  不对的包 SEQ: " << result.Seq - 1 << "   FLAG: ACK WANT:" << base << endl;
				
		}
		
		if (lastack != result.Seq) {
			repulicateack = 0;
			lastack = result.Seq;
			isrepluciteack = false;

		}
		else {
			repulicateack++;
			if(repulicateack>2)
				isrepluciteack = true;
		}
		
		
		
	}

}

void SEND()
{
	//时间列表初始化
	for (int i = 0; i < 500; i++)
		time_list[i] = 0;
	
	HEADER msg;

	cout << "请输入文件名：" << endl;
	string filename = "";
	cin >> filename;
	// 发送文件名
	sendto(ClientSocket,(char*)(filename.c_str()), filename.length(),0,(SOCKADDR*)&ServerAddr,l);

	//读文件
	sendbuf = new char* [80000];
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
	
	
	int len = length;
	while (len > 0)
	{
		filein.read(sendbuf[pagenum], min(len, MAXSIZE));
		len -= MAXSIZE;
		pagenum++;
	}
	cout <<"pagenum: "<< pagenum << endl;

	filein.close();
	save_pagenum = pagenum;
	int sendnum = pagenum;
	last_length = length % MAXSIZE;
	cout <<"last_length"<< last_length << endl;

	clock_t start = clock();
	recver = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Recvpac, NULL, NULL, NULL);
	//开始循环发送数据
	while (tail< save_pagenum)
	{
		//cout << "here..." << endl;
		
		//超时发送，这里先等一下
		while (!istimeout&& tail < save_pagenum)

		{
			//cout << "tail " << tail << endl;
			if (tail - base < NUM_WINDOW&& tail < save_pagenum)
			{
				SetPriorityClass(GetCurrentThread(), HIGH_PRIORITY_CLASS);

				if (sendnum == 1)
				{
					//msg.set_EOF();
					cout << "最后一个包的长度" << length << endl;
				}


				msg.Seq = tail;
				msg.datasize = min(MAXSIZE, length);

				msg.clearcontent();
				msg.setcontent(sendbuf[tail], min(MAXSIZE, length));

				msg.Checksum = 0;
				msg.set_Checksum();
				//cout << msg.Checksum << endl;
				//setsockopt(ClientSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuf, sizeof(char) * MAXSIZE);
				sendto(ClientSocket, (char*)&msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
				
				time_list[tail] = clock();
				//cout << "*************SEND time_list[" << tail << "]=" << time_list[tail] << endl;
				//time_list.push_back(make_pair(clock(), msg.Seq));//开始计时，在末尾存入一个新元素
				cout << "[SEND] 发送包的长度" << msg.datasize << " 发送包的缓存区数：" << tail << " SEQ:" << int(msg.Seq) << " 剩余长度 ：" << length << endl;
				
				//cout << clock() << endl;
				tail++;
				sendnum--;
				length -= MAXSIZE;

				//cout << "[Send]  " << msg.datasize << " bytes!" << " SEQ:" << int(msg.Seq) << " SUM:" << int(msg.Checksum) << endl;
				cout << "当前窗口： " << base << " to " << tail << endl;
				SetPriorityClass(GetCurrentThread(), NORMAL_PRIORITY_CLASS);
			}
		}
		
		
		
	}
	//cout << tail << endl;
	

	while (!isend)
	{
		
	}
	msg.set_EOF();
	sendto(ClientSocket, (char*)&msg, sizeof(HEADER), 0, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));
	cout << "发送最后一个数据包" << endl;
	clock_t end = clock();

	stime = (end - start) / CLOCKS_PER_SEC;
	cout << "文件发送完毕" << endl;
	cout << "共计用时" << stime << "s" << endl;
	double throupt_rate = save_pagenum * sizeof(HEADER) / 1024 / stime;
	cout << "吞吐率为" << throupt_rate << "Kb/s" << endl;
	cout << "传输完毕" << endl;
	for (int i = 0; i < 80000; i++)
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
	//setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
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