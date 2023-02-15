#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#include<map>
#include<WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int MAXSIZE = 1024;//传输缓冲区最大长度
const unsigned char INIT = 0x0;
const unsigned char SYN = 0x1; //SYN = 1 ACK = 0
const unsigned char ACK = 0x2;//SYN = 0, ACK = 1，FIN = 0
const unsigned char ACK_SYN = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN = 0x4;//FIN = 1 ACK = 0 SYN=0
const unsigned char FIN_ACK = 0x6;  //    FIN = 1 ACK = 1
const unsigned char OVER = 0x8;//结束标志
double MAX_TIME = CLOCKS_PER_SEC;
map<int, string>   state;

void init() {
	state[0] = "INIT";
	state[1] = "SYN";
	state[2] = "ACK";
	state[3] = "ACK_SYN";
	state[4] = "FIN";
	state[6] = "FIN_ACK";
	state[8] = "OVER";
}


// 计算校验和
u_short cksum(u_short* UDP_header, int size)
{
	// size以字节计数，但是这里要用16byte    u_short
	// 这里使用的函数都是以字节计数的
	unsigned short* a = (unsigned short*)UDP_header;
	unsigned int sum = 0;
	for (int i = 0; i < size / 2; i++) {
		sum += (unsigned int)a[i];
		if (sum & (0xFFFF0000)) {
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(((unsigned short)sum) & 0XFFFF);
}

struct HEADER
{
	u_short sum = 0;//校验和 16位
	u_short datasize = 0;//所包含数据长度 16位
	unsigned char flag = 0;
	//八位，使用后三位，排列是FIN ACK SYN 
	unsigned char SEQ = 0;
	//八位，传输的序列号，0~255，超过后mod
	HEADER() {
		sum = 0;//校验和 16位
		datasize = 0;//所包含数据长度 16位
		flag = 0;
		//八位，使用后三位，排列是FIN ACK SYN 
		SEQ = 0;
	}
};
int Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)//三次握手建立连接
{
	HEADER header;
	char* Buffer = new char[sizeof(header)];

	u_short sum;

	//进行第一次握手
	header.flag = SYN;
	header.sum = 0;//校验和置0
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;//计算校验和
	memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
	if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	clock_t start = clock(); //记录发送第一次握手时间

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	//接收第二次握手
	while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//超时，重新传输第一次握手
		{
			header.flag = SYN;
			header.sum = 0;//校验和置0
			header.sum = cksum((u_short*)&header, sizeof(header));//计算校验和
			memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "第一次握手超时，正在进行重传" << endl;
		}
	}


	//进行校验和检验
	memcpy(&header, Buffer, sizeof(header));
	if (header.flag == ACK_SYN && cksum((u_short*)&header, sizeof(header) == 0))
	{
		
		cout << "收到第二次握手信息,FLAG="<<state[int(header.flag)] << endl;
	}
	else
	{
		cout << "连接发生错误，请重启客户端！" << endl;
		return -1;
	}

	//进行第三次握手
	header.flag = ACK;
	header.sum = 0;
	header.sum = cksum((u_short*)&header, sizeof(header));//计算校验和
	if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;//判断客户端是否打开，-1为未开启发送失败
	}
	cout << "服务器成功连接！可以发送数据" << endl;
	return 1;
}

int disConnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen) {

	HEADER header;
	char* Buffer = new char[sizeof(header)];

	//第一次挥手
	header.flag = FIN;
	header.sum = 0;//校验和置0
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;//计算校验和
	memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
	if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	clock_t start = clock(); //记录发送第一次挥手时间

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	// 接收第二次挥手
	while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//超时，重新传输第一次挥手
		{
			header.flag = FIN;
			header.sum = 0;//校验和置0
			header.sum = cksum((u_short*)&header, sizeof(header));//计算校验和
			memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "第一次挥手超时，正在进行重传" << endl;
		}
	}
	memcpy(&header, Buffer, sizeof(header));
	if (header.flag == ACK && cksum((u_short*)&header, sizeof(header)) == 0)
	{
		cout << "收到第二次挥手信息,FLAG= "<<state[int(header.flag)] << endl;
	}
	else
	{
		cout << "连接发生错误，程序直接退出！" << endl;
		return -1;
	}


	//进行第三次挥手
	header.flag = FIN_ACK;
	header.sum = 0;
	header.sum = cksum((u_short*)&header, sizeof(header));//计算校验和
	if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}

	start = clock();
	//接收第四次挥手
	while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//超时，重新传输第三次挥手
		{
			header.flag = FIN;
			header.sum = 0;//校验和置0
			header.sum = cksum((u_short*)&header, sizeof(header));//计算校验和
			memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "第四次握手超时，正在进行重传" << endl;
		}
	}
	cout << "四次挥手结束，连接断开！FLAG=" << state[int(header.flag)] << endl;
	return 1;
}
void send_package(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len, int& order) {
	HEADER header;
	char* buffer = new char[MAXSIZE + sizeof(header)];
	for (int i = 0; i < MAXSIZE + sizeof(header); i++) {
		buffer[i] = 0;
	}
	header.datasize = len;
	header.SEQ = unsigned char(order);//序列号
	memcpy(buffer, &header, sizeof(header));
	memcpy(buffer + sizeof(header), message, sizeof(header) + len);
	u_short check = cksum((u_short*)buffer, sizeof(header) + len);//计算校验和
	header.sum = check;
	memcpy(buffer, &header, sizeof(header));
	sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送
	cout << "Send message " << len << " bytes!" << " FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << " CHECKSUM:" << int(header.sum) << endl;
	clock_t start = clock();//记录发送时间
	//接收ack等信息
	while (true) {
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		// 超时机制
		int times = 0;
		while (times < 5 && recvfrom(socketClient, buffer, MAXSIZE + sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			// 未收到消息，超时重传
			if (clock() - start > MAX_TIME)
			{
				//header.datasize = len;
				//header.SEQ = u_char(order);//序列号
				//header.flag = u_char(0x0);
				//memcpy(buffer, &header, sizeof(header));
				//memcpy(buffer + sizeof(header), message, sizeof(header) + len);
				//u_short check = cksum((u_short*)buffer, sizeof(header) + len);//计算校验和
				//header.sum = check;
				//memcpy(buffer, &header, sizeof(header));
				sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送
				cout << "TIME OUT! ReSend message " << len << " bytes! FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << endl;
				times++;
				start = clock();//记录发送时间
			}
		}
		//cout << "收到了消息" << endl;
		if (times == 5) {
			cout << "发送出错" << endl;
			return;
		}
		// 得到了数据包
		memcpy(&header, buffer, sizeof(header));//缓冲区接收到信息，读取
		//检查是否是想要的ACK以及差错检验
		//cout << header.SEQ <<"      "<< header.flag << "      " << cksum((u_short*)buffer, sizeof(header) + len) << endl;
		if (header.SEQ == u_short(order) && header.flag == ACK)
		{
			//修改flag，检查校验和
			header.flag = INIT;
			memcpy(buffer, &header, sizeof(header));
			if (cksum((u_short*)buffer, sizeof(header) + len) == check)
			{
				header.flag = ACK;
				cout << "Send has been confirmed! FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << endl;
				break;
			}
			
		}
		// 序列号不对，或者检验和不对，等待
		else {
			// 收到的消息不对，超时重传
			if (clock() - start > MAX_TIME)
			{
				header.datasize = len;
				header.SEQ = u_char(order);//序列号
				header.flag = u_char(0x0);
				memcpy(buffer, &header, sizeof(header));
				memcpy(buffer + sizeof(header), message, sizeof(header) + len);
				u_short check = cksum((u_short*)buffer, sizeof(header) + len);//计算校验和
				header.sum = check;
				memcpy(buffer, &header, sizeof(header));
				sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送
				cout << "Message Error！ReSend message " << len << " bytes! FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << endl;
				start = clock();//记录发送时间
			}

			continue;
		}


	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式

}
void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len) {
	int packagenum = len / MAXSIZE + (len % MAXSIZE != 0);
	int seqnum = 0;

	for (int i = 0; i < packagenum; i++)
	{
		// 现在是假设包不出错的情况
		send_package(socketClient, servAddr, servAddrlen, message + i * MAXSIZE, i == packagenum - 1 ? len - (packagenum - 1) * MAXSIZE : MAXSIZE, seqnum);
		seqnum++;
		if (seqnum > 255)
		{
			seqnum = seqnum - 256;
		}
	}
	//// 文件传递完成，发送结束消息
	HEADER header;
	char* buffer = new char[sizeof(header)];
	header.flag = OVER;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(buffer, &header, sizeof(header));
	sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
	cout << "Send End!" << endl;
	clock_t start = clock();
	while (true) {
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		// 超时机制
		while (recvfrom(socketClient, buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			if (clock() - start > MAX_TIME)
			{
				char* buffer = new char[sizeof(header)];
				header.flag = OVER;
				header.sum = 0;
				u_short temp = cksum((u_short*)&header, sizeof(header));
				header.sum = temp;
				memcpy(buffer, &header, sizeof(header));
				sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
				cout << "Time Out! ReSend End!" << endl;
				clock_t start = clock();
			}
		}
		memcpy(&header, buffer, sizeof(header));//缓冲区接收到信息，读取
		u_short check = cksum((u_short*)&header, sizeof(header));
		if (header.flag == OVER)
		{
			cout << "对方已成功接收文件!" << endl;
			break;
		}
		else
		{
			continue;
		}
	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式

}
void quit(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen) {
	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	HEADER header;
	char* buffer = new char[sizeof(header)];
	header.flag = OVER;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(buffer, &header, sizeof(header));
	sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
	cout << "Send End!" << endl;
	clock_t start = clock();
	while (true) {
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		// 超时机制
		while (recvfrom(socketClient, buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			if (clock() - start > MAX_TIME)
			{
				char* buffer = new char[sizeof(header)];
				header.flag = OVER;
				header.sum = 0;
				u_short temp = cksum((u_short*)&header, sizeof(header));
				header.sum = temp;
				memcpy(buffer, &header, sizeof(header));
				sendto(socketClient, buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
				cout << "Time Out! ReSend End!" << endl;
				clock_t start = clock();
			}
		}
		memcpy(&header, buffer, sizeof(header));//缓冲区接收到信息，读取
		u_short check = cksum((u_short*)&header, sizeof(header));
		if (header.flag == OVER)
		{
			cout << "对方已成功接收文件!决定断开" << endl;
			break;
		}
		else
		{
			continue;
		}
	}
	mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式


}



int main() {
	init();

	WSADATA wsaData;
	// 使用2.2版   初始化socket  dll
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "初始化失败" << endl;
		return 0;
	}
	else {
		cout << "初始化成功" << endl;
	}
	/* 创建Socket,使用数据报套接字 */
	SOCKET ClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ClientSocket == INVALID_SOCKET)
	{
		cout << "客户端Socket创建失败" << endl;
		WSACleanup();
		return 0;
	}
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;     //指定IP格式  
	USHORT uPort = 8093;                 //服务器监听端口  
	ServerAddr.sin_port = htons(uPort);   //绑定端口号  
	//ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //IP地址系统自动分配
	inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);
	int len = sizeof(ServerAddr);
	//建立连接
	int isError = Connect(ClientSocket, ServerAddr, len);

		// 传输数据
	string filename;
	int filesize = 0;
	cout << "请输入文件名称" << endl;
	cin >> filename;

	// 退出指令
	while (filename != "quit")
	{
		// 发送文件名
		send(ClientSocket, ServerAddr, len, (char*)(filename.c_str()), filename.length());
		// 发送正文   分批读入和发送
		clock_t start = clock();
		ifstream fin(filename.c_str(), ifstream::binary);//以二进制方式打开文件
		char* buffer = new char[10000];
		int index = 0;
		unsigned char temp = fin.get();
		filesize++;

		while (fin)
		{
			buffer[index++] = temp;
			if (index == 10000) {

				send(ClientSocket, ServerAddr, len, buffer, index);
				string addition = "not end";
				send(ClientSocket, ServerAddr, len, (char*)(addition.c_str()), addition.length());
				index = 0;
			}
			temp = fin.get();
			filesize++;

		}
		fin.close();
		if (index != 0) {
			send(ClientSocket, ServerAddr, len, buffer, index);
		}
		string addition = "end";
		send(ClientSocket, ServerAddr, len, (char*)(addition.c_str()), addition.length());


		clock_t end = clock();
		//quit(ClientSocket, ServerAddr, len);
		cout << "传输总时间为:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
		cout << "吞吐率为:" << ((float)filesize) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
		cout << state[0x1] << endl;
		cout << "请输入文件名称" << endl;
		cin >> filename;
	}
	quit(ClientSocket, ServerAddr, len);

	disConnect(ClientSocket, ServerAddr, len);

}
