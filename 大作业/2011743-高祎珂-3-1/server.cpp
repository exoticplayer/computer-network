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
map<int, string>flagmap;
void init() {
	flagmap[0] = "INIT";
	flagmap[1] = "SYN";
	flagmap[2] = "ACK";
	flagmap[3] = "ACK_SYN";
	flagmap[4] = "FIN";
	flagmap[6] = "FIN_ACK";
	flagmap[8] = "OVER";

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

int Connect(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{

	HEADER header;
	char* Buffer = new char[sizeof(header)];

	//接收第一次握手信息
	while (1 == 1)
	{
		if (recvfrom(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) == -1)
		{
			return -1;
		}
		memcpy(&header, Buffer, sizeof(header));
		if (header.flag == SYN && cksum((u_short*)&header, sizeof(header)) == 0)
		{
			cout << "成功接收第一次握手信息,FLAG:"<<flagmap[int(header.flag)] << endl;
			break;
		}
	}

	//发送第二次握手信息
	header.flag = ACK_SYN;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(Buffer, &header, sizeof(header));
	if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		return -1;
	}
	clock_t start = clock();//记录第二次握手发送时间

	//接收第三次握手
	while (recvfrom(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
	{
		if (clock() - start > MAX_TIME)
		{
			header.flag = ACK_SYN;
			header.sum = 0;
			u_short temp = cksum((u_short*)&header, sizeof(header));
			header.flag = temp;
			memcpy(Buffer, &header, sizeof(header));
			if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
			{
				return -1;
			}
			cout << "第二次握手超时，正在进行重传" << endl;
		}
	}

	HEADER temp1;
	memcpy(&temp1, Buffer, sizeof(header));
	if (temp1.flag == ACK && cksum((u_short*)&temp1, sizeof(temp1) == 0))
	{
		cout << "成功建立通信！可以接收数据,FLAG:" << flagmap[int(temp1.flag)] << endl;
	}
	else
	{
		cout << "serve连接发生错误，请重启客户端！" << endl;
		return -1;
	}
	return 1;
}
int disConnect(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen) {
	HEADER header;
	char* Buffer = new char[sizeof(header)];
	while (true) {
		int length = recvfrom(sockServ, Buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//接收信息长度
		memcpy(&header, Buffer, sizeof(header));
		if (header.flag == FIN && cksum((u_short*)&header, sizeof(header)) == 0)
		{
			cout << "成功接收第一次挥手信息,FLAG:" << flagmap[int(header.flag)] << endl;
			break;
		}

	}
	//发送第二次挥手信息
	header.flag = ACK;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(Buffer, &header, sizeof(header));
	if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		return -1;
	}
	clock_t start = clock();//记录第二次挥手发送时间
	//接收第三次挥手
	while (recvfrom(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
	{
		if (clock() - start > MAX_TIME)
		{
			header.flag = ACK;
			header.sum = 0;
			u_short temp = cksum((u_short*)&header, sizeof(header));
			header.flag = temp;
			memcpy(Buffer, &header, sizeof(header));
			if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
			{
				return -1;
			}
			cout << "第二次挥手超时，正在进行重传" << endl;
		}
	}
	HEADER temp1;
	memcpy(&temp1, Buffer, sizeof(header));
	if (temp1.flag == FIN_ACK && cksum((u_short*)&temp1, sizeof(temp1) == 0))
	{
		cout << "成功接收第三次挥手,FLAG:" << flagmap[int(header.flag)] << endl;
	}
	else
	{
		cout << "发生错误,客户端关闭！" << endl;
		return -1;
	}
	//发送第四次挥手信息
	header.flag = FIN_ACK;
	header.sum = 0;
	temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(Buffer, &header, sizeof(header));
	if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		cout << "发生错误,客户端关闭！" << endl;
		return -1;
	}
	cout << "四次挥手结束，连接断开！" << endl;
	return 1;




}
int RecvMessage(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, char* message) {
	long int all = 0;//文件长度
	HEADER header;
	char* buffer = new char[MAXSIZE + sizeof(header)];
	int seq = 0;
	int index = 0;
	while (true) {
		for (int i = 0; i < MAXSIZE + sizeof(header); i++) {
			buffer[i] = 0;
		}
		int length = recvfrom(sockServ, buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//接收报文长度
		//cout << "收到了文件" << length;
		memcpy(&header, buffer, sizeof(header));

		//判断是否是结束
		if (header.flag == OVER && cksum((u_short*)&header, sizeof(header)) == 0)
		{
			cout << "文件接收完毕" << endl;
			break;
		}
		// 数据包flag是初始化状态，为0   确认校验和
		if (header.flag == unsigned char(0) && cksum((u_short*)buffer, length)==0&& seq == int(header.SEQ)) {
			cout << "Rece message " << length - sizeof(header) << " bytes!Flag:" << flagmap[int(header.flag)] << " SEQ : " << int(header.SEQ) << " CHECKSUM:" << int(header.sum) << endl;
			// 把数据写入message
			char* temp = new char[length - sizeof(header)];
			memcpy(temp, buffer + sizeof(header), length - sizeof(header));
			memcpy(message + all, temp, length - sizeof(header));
			all = all + int(header.datasize);


			// 校验和重新置0，返回ACK
			header.flag = ACK;
			/*header.datasize = 0;
			header.SEQ = (unsigned char)seq;
			header.sum = 0;*/
			//u_short temp1 = cksum((u_short*)&header, sizeof(header));
			header.sum = 0;
			memcpy(buffer, &header, sizeof(header));
			// 发送ACK
			sendto(sockServ, buffer, sizeof(header)+ MAXSIZE, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
			cout << "Send to Clinet ACK:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << " CHECKSUM:" << int(header.sum)<< endl;
			seq++;
			if (seq > 255)
			{
				seq = seq - 256;
			}
		}
		//校验和不对，或者SEQ不对
		else {
			//说明出了问题，返回ACK
			header.flag = ACK;
			if (cksum((u_short*)buffer, length) != 0)
				header.flag = INIT;
			/*header.datasize = 0;
			header.SEQ = (unsigned char)seq;
			header.sum = 0;
			u_short temp = cksum((u_short*)&header, sizeof(header));
			header.sum = temp;*/
			memcpy(buffer, &header, sizeof(header));
			//重发该包的ACK
			sendto(sockServ, buffer, sizeof(header)+MAXSIZE, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
			cout << "Some ERROR:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << endl;
			continue;//丢弃该数据包

		}


	}
	//发送OVER信息
	header.flag = OVER;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(buffer, &header, sizeof(header));
	if (sendto(sockServ, buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		return -1;
	}
	return all;
}

int main() {
	init();
	//SOCKET ClientSocket;
	WSADATA wsaData;
	// 使用2.2版   初始化socket  dll
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "初始化失败" << endl;
		return 0;
	}
	else {
		cout << "初始化成功" << endl;
	}

	// 创建监听socket
	SOCKET ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "建立socket失败" << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "建立监听socket成功" << endl;
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
		cout << "绑定成功" << endl;
	}
	cout << "进入监听状态，等待客户端上线" << endl;
	int len = sizeof(ServerAddr);
	//建立连接
	Connect(ListenSocket, ServerAddr, len);
	while (true)
	{
		char* name = new char[20];
		char* data = new char[10000];
		char* additon = new char[20];
		int namelen = RecvMessage(ListenSocket, ServerAddr, len, name);

		if (namelen == 0)
			break;
		string a;
		for (int i = 0; i < namelen; i++)
		{
			a = a + name[i];
		}
		ofstream fout(a.c_str(), ofstream::binary);
		int datalen = RecvMessage(ListenSocket, ServerAddr, len, data);
		for (int i = 0; i < datalen; i++)
		{
			fout << data[i];
		}
		int addlen = RecvMessage(ListenSocket, ServerAddr, len, additon);
		string temp = "";
		for (int i = 0; i < addlen; i++)
		{
			temp = temp + additon[i];
		}
		while (temp != "end")
		{
			int datalen = RecvMessage(ListenSocket, ServerAddr, len, data);
			for (int i = 0; i < datalen; i++)
			{
				fout << data[i];
			}
			int addlen = RecvMessage(ListenSocket, ServerAddr, len, additon);
			temp = "";
			for (int i = 0; i < addlen; i++)
			{
				temp = temp + additon[i];
			}
			/*cout << string(additon) << endl;
			cout << additon;*/

		}
		//cout << string(additon) << endl;
		fout.close();
		cout << "文件传输完成" << endl;
		delete[]name;
		delete[]data;
	}
	disConnect(ListenSocket, ServerAddr, len);


}