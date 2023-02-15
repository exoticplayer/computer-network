#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#include<map>
#include<WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int MAXSIZE = 1024;//���仺������󳤶�
const unsigned char INIT = 0x0;
const unsigned char SYN = 0x1; //SYN = 1 ACK = 0
const unsigned char ACK = 0x2;//SYN = 0, ACK = 1��FIN = 0
const unsigned char ACK_SYN = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN = 0x4;//FIN = 1 ACK = 0 SYN=0
const unsigned char FIN_ACK = 0x6;  //    FIN = 1 ACK = 1
const unsigned char OVER = 0x8;//������־
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

// ����У���
u_short cksum(u_short* UDP_header, int size)
{
	// size���ֽڼ�������������Ҫ��16byte    u_short
	// ����ʹ�õĺ����������ֽڼ�����
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
	u_short sum = 0;//У��� 16λ
	u_short datasize = 0;//���������ݳ��� 16λ
	unsigned char flag = 0;
	//��λ��ʹ�ú���λ��������FIN ACK SYN 
	unsigned char SEQ = 0;
	//��λ����������кţ�0~255��������mod
	HEADER() {
		sum = 0;//У��� 16λ
		datasize = 0;//���������ݳ��� 16λ
		flag = 0;
		//��λ��ʹ�ú���λ��������FIN ACK SYN 
		SEQ = 0;
	}
};

int Connect(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{

	HEADER header;
	char* Buffer = new char[sizeof(header)];

	//���յ�һ��������Ϣ
	while (1 == 1)
	{
		if (recvfrom(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, &ClientAddrLen) == -1)
		{
			return -1;
		}
		memcpy(&header, Buffer, sizeof(header));
		if (header.flag == SYN && cksum((u_short*)&header, sizeof(header)) == 0)
		{
			cout << "�ɹ����յ�һ��������Ϣ,FLAG:"<<flagmap[int(header.flag)] << endl;
			break;
		}
	}

	//���͵ڶ���������Ϣ
	header.flag = ACK_SYN;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(Buffer, &header, sizeof(header));
	if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		return -1;
	}
	clock_t start = clock();//��¼�ڶ������ַ���ʱ��

	//���յ���������
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
			cout << "�ڶ������ֳ�ʱ�����ڽ����ش�" << endl;
		}
	}

	HEADER temp1;
	memcpy(&temp1, Buffer, sizeof(header));
	if (temp1.flag == ACK && cksum((u_short*)&temp1, sizeof(temp1) == 0))
	{
		cout << "�ɹ�����ͨ�ţ����Խ�������,FLAG:" << flagmap[int(temp1.flag)] << endl;
	}
	else
	{
		cout << "serve���ӷ��������������ͻ��ˣ�" << endl;
		return -1;
	}
	return 1;
}
int disConnect(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen) {
	HEADER header;
	char* Buffer = new char[sizeof(header)];
	while (true) {
		int length = recvfrom(sockServ, Buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//������Ϣ����
		memcpy(&header, Buffer, sizeof(header));
		if (header.flag == FIN && cksum((u_short*)&header, sizeof(header)) == 0)
		{
			cout << "�ɹ����յ�һ�λ�����Ϣ,FLAG:" << flagmap[int(header.flag)] << endl;
			break;
		}

	}
	//���͵ڶ��λ�����Ϣ
	header.flag = ACK;
	header.sum = 0;
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(Buffer, &header, sizeof(header));
	if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		return -1;
	}
	clock_t start = clock();//��¼�ڶ��λ��ַ���ʱ��
	//���յ����λ���
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
			cout << "�ڶ��λ��ֳ�ʱ�����ڽ����ش�" << endl;
		}
	}
	HEADER temp1;
	memcpy(&temp1, Buffer, sizeof(header));
	if (temp1.flag == FIN_ACK && cksum((u_short*)&temp1, sizeof(temp1) == 0))
	{
		cout << "�ɹ����յ����λ���,FLAG:" << flagmap[int(header.flag)] << endl;
	}
	else
	{
		cout << "��������,�ͻ��˹رգ�" << endl;
		return -1;
	}
	//���͵��Ĵλ�����Ϣ
	header.flag = FIN_ACK;
	header.sum = 0;
	temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;
	memcpy(Buffer, &header, sizeof(header));
	if (sendto(sockServ, Buffer, sizeof(header), 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
	{
		cout << "��������,�ͻ��˹رգ�" << endl;
		return -1;
	}
	cout << "�Ĵλ��ֽ��������ӶϿ���" << endl;
	return 1;




}
int RecvMessage(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, char* message) {
	long int all = 0;//�ļ�����
	HEADER header;
	char* buffer = new char[MAXSIZE + sizeof(header)];
	int seq = 0;
	int index = 0;
	while (true) {
		for (int i = 0; i < MAXSIZE + sizeof(header); i++) {
			buffer[i] = 0;
		}
		int length = recvfrom(sockServ, buffer, sizeof(header) + MAXSIZE, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//���ձ��ĳ���
		//cout << "�յ����ļ�" << length;
		memcpy(&header, buffer, sizeof(header));

		//�ж��Ƿ��ǽ���
		if (header.flag == OVER && cksum((u_short*)&header, sizeof(header)) == 0)
		{
			cout << "�ļ��������" << endl;
			break;
		}
		// ���ݰ�flag�ǳ�ʼ��״̬��Ϊ0   ȷ��У���
		if (header.flag == unsigned char(0) && cksum((u_short*)buffer, length)==0&& seq == int(header.SEQ)) {
			cout << "Rece message " << length - sizeof(header) << " bytes!Flag:" << flagmap[int(header.flag)] << " SEQ : " << int(header.SEQ) << " CHECKSUM:" << int(header.sum) << endl;
			// ������д��message
			char* temp = new char[length - sizeof(header)];
			memcpy(temp, buffer + sizeof(header), length - sizeof(header));
			memcpy(message + all, temp, length - sizeof(header));
			all = all + int(header.datasize);


			// У���������0������ACK
			header.flag = ACK;
			/*header.datasize = 0;
			header.SEQ = (unsigned char)seq;
			header.sum = 0;*/
			//u_short temp1 = cksum((u_short*)&header, sizeof(header));
			header.sum = 0;
			memcpy(buffer, &header, sizeof(header));
			// ����ACK
			sendto(sockServ, buffer, sizeof(header)+ MAXSIZE, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
			cout << "Send to Clinet ACK:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << " CHECKSUM:" << int(header.sum)<< endl;
			seq++;
			if (seq > 255)
			{
				seq = seq - 256;
			}
		}
		//У��Ͳ��ԣ�����SEQ����
		else {
			//˵���������⣬����ACK
			header.flag = ACK;
			if (cksum((u_short*)buffer, length) != 0)
				header.flag = INIT;
			/*header.datasize = 0;
			header.SEQ = (unsigned char)seq;
			header.sum = 0;
			u_short temp = cksum((u_short*)&header, sizeof(header));
			header.sum = temp;*/
			memcpy(buffer, &header, sizeof(header));
			//�ط��ð���ACK
			sendto(sockServ, buffer, sizeof(header)+MAXSIZE, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
			cout << "Some ERROR:" << (int)header.SEQ << " SEQ:" << (int)header.SEQ << endl;
			continue;//���������ݰ�

		}


	}
	//����OVER��Ϣ
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
	// ʹ��2.2��   ��ʼ��socket  dll
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "��ʼ��ʧ��" << endl;
		return 0;
	}
	else {
		cout << "��ʼ���ɹ�" << endl;
	}

	// ��������socket
	SOCKET ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "����socketʧ��" << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "��������socket�ɹ�" << endl;
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
		cout << "�󶨳ɹ�" << endl;
	}
	cout << "�������״̬���ȴ��ͻ�������" << endl;
	int len = sizeof(ServerAddr);
	//��������
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
		cout << "�ļ��������" << endl;
		delete[]name;
		delete[]data;
	}
	disConnect(ListenSocket, ServerAddr, len);


}