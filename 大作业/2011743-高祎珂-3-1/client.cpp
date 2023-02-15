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
int Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)//�������ֽ�������
{
	HEADER header;
	char* Buffer = new char[sizeof(header)];

	u_short sum;

	//���е�һ������
	header.flag = SYN;
	header.sum = 0;//У�����0
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;//����У���
	memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
	if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	clock_t start = clock(); //��¼���͵�һ������ʱ��

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	//���յڶ�������
	while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//��ʱ�����´����һ������
		{
			header.flag = SYN;
			header.sum = 0;//У�����0
			header.sum = cksum((u_short*)&header, sizeof(header));//����У���
			memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "��һ�����ֳ�ʱ�����ڽ����ش�" << endl;
		}
	}


	//����У��ͼ���
	memcpy(&header, Buffer, sizeof(header));
	if (header.flag == ACK_SYN && cksum((u_short*)&header, sizeof(header) == 0))
	{
		
		cout << "�յ��ڶ���������Ϣ,FLAG="<<state[int(header.flag)] << endl;
	}
	else
	{
		cout << "���ӷ��������������ͻ��ˣ�" << endl;
		return -1;
	}

	//���е���������
	header.flag = ACK;
	header.sum = 0;
	header.sum = cksum((u_short*)&header, sizeof(header));//����У���
	if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;//�жϿͻ����Ƿ�򿪣�-1Ϊδ��������ʧ��
	}
	cout << "�������ɹ����ӣ����Է�������" << endl;
	return 1;
}

int disConnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen) {

	HEADER header;
	char* Buffer = new char[sizeof(header)];

	//��һ�λ���
	header.flag = FIN;
	header.sum = 0;//У�����0
	u_short temp = cksum((u_short*)&header, sizeof(header));
	header.sum = temp;//����У���
	memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
	if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	clock_t start = clock(); //��¼���͵�һ�λ���ʱ��

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	// ���յڶ��λ���
	while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//��ʱ�����´����һ�λ���
		{
			header.flag = FIN;
			header.sum = 0;//У�����0
			header.sum = cksum((u_short*)&header, sizeof(header));//����У���
			memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "��һ�λ��ֳ�ʱ�����ڽ����ش�" << endl;
		}
	}
	memcpy(&header, Buffer, sizeof(header));
	if (header.flag == ACK && cksum((u_short*)&header, sizeof(header)) == 0)
	{
		cout << "�յ��ڶ��λ�����Ϣ,FLAG= "<<state[int(header.flag)] << endl;
	}
	else
	{
		cout << "���ӷ������󣬳���ֱ���˳���" << endl;
		return -1;
	}


	//���е����λ���
	header.flag = FIN_ACK;
	header.sum = 0;
	header.sum = cksum((u_short*)&header, sizeof(header));//����У���
	if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}

	start = clock();
	//���յ��Ĵλ���
	while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//��ʱ�����´�������λ���
		{
			header.flag = FIN;
			header.sum = 0;//У�����0
			header.sum = cksum((u_short*)&header, sizeof(header));//����У���
			memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
			sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "���Ĵ����ֳ�ʱ�����ڽ����ش�" << endl;
		}
	}
	cout << "�Ĵλ��ֽ��������ӶϿ���FLAG=" << state[int(header.flag)] << endl;
	return 1;
}
void send_package(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len, int& order) {
	HEADER header;
	char* buffer = new char[MAXSIZE + sizeof(header)];
	for (int i = 0; i < MAXSIZE + sizeof(header); i++) {
		buffer[i] = 0;
	}
	header.datasize = len;
	header.SEQ = unsigned char(order);//���к�
	memcpy(buffer, &header, sizeof(header));
	memcpy(buffer + sizeof(header), message, sizeof(header) + len);
	u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
	header.sum = check;
	memcpy(buffer, &header, sizeof(header));
	sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
	cout << "Send message " << len << " bytes!" << " FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << " CHECKSUM:" << int(header.sum) << endl;
	clock_t start = clock();//��¼����ʱ��
	//����ack����Ϣ
	while (true) {
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		// ��ʱ����
		int times = 0;
		while (times < 5 && recvfrom(socketClient, buffer, MAXSIZE + sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			// δ�յ���Ϣ����ʱ�ش�
			if (clock() - start > MAX_TIME)
			{
				//header.datasize = len;
				//header.SEQ = u_char(order);//���к�
				//header.flag = u_char(0x0);
				//memcpy(buffer, &header, sizeof(header));
				//memcpy(buffer + sizeof(header), message, sizeof(header) + len);
				//u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
				//header.sum = check;
				//memcpy(buffer, &header, sizeof(header));
				sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
				cout << "TIME OUT! ReSend message " << len << " bytes! FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << endl;
				times++;
				start = clock();//��¼����ʱ��
			}
		}
		//cout << "�յ�����Ϣ" << endl;
		if (times == 5) {
			cout << "���ͳ���" << endl;
			return;
		}
		// �õ������ݰ�
		memcpy(&header, buffer, sizeof(header));//���������յ���Ϣ����ȡ
		//����Ƿ�����Ҫ��ACK�Լ�������
		//cout << header.SEQ <<"      "<< header.flag << "      " << cksum((u_short*)buffer, sizeof(header) + len) << endl;
		if (header.SEQ == u_short(order) && header.flag == ACK)
		{
			//�޸�flag�����У���
			header.flag = INIT;
			memcpy(buffer, &header, sizeof(header));
			if (cksum((u_short*)buffer, sizeof(header) + len) == check)
			{
				header.flag = ACK;
				cout << "Send has been confirmed! FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << endl;
				break;
			}
			
		}
		// ���кŲ��ԣ����߼���Ͳ��ԣ��ȴ�
		else {
			// �յ�����Ϣ���ԣ���ʱ�ش�
			if (clock() - start > MAX_TIME)
			{
				header.datasize = len;
				header.SEQ = u_char(order);//���к�
				header.flag = u_char(0x0);
				memcpy(buffer, &header, sizeof(header));
				memcpy(buffer + sizeof(header), message, sizeof(header) + len);
				u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
				header.sum = check;
				memcpy(buffer, &header, sizeof(header));
				sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
				cout << "Message Error��ReSend message " << len << " bytes! FLAG:" << state[int(header.flag)] << " SEQ:" << int(header.SEQ) << endl;
				start = clock();//��¼����ʱ��
			}

			continue;
		}


	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ

}
void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len) {
	int packagenum = len / MAXSIZE + (len % MAXSIZE != 0);
	int seqnum = 0;

	for (int i = 0; i < packagenum; i++)
	{
		// �����Ǽ��������������
		send_package(socketClient, servAddr, servAddrlen, message + i * MAXSIZE, i == packagenum - 1 ? len - (packagenum - 1) * MAXSIZE : MAXSIZE, seqnum);
		seqnum++;
		if (seqnum > 255)
		{
			seqnum = seqnum - 256;
		}
	}
	//// �ļ�������ɣ����ͽ�����Ϣ
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
		// ��ʱ����
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
		memcpy(&header, buffer, sizeof(header));//���������յ���Ϣ����ȡ
		u_short check = cksum((u_short*)&header, sizeof(header));
		if (header.flag == OVER)
		{
			cout << "�Է��ѳɹ������ļ�!" << endl;
			break;
		}
		else
		{
			continue;
		}
	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ

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
		// ��ʱ����
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
		memcpy(&header, buffer, sizeof(header));//���������յ���Ϣ����ȡ
		u_short check = cksum((u_short*)&header, sizeof(header));
		if (header.flag == OVER)
		{
			cout << "�Է��ѳɹ������ļ�!�����Ͽ�" << endl;
			break;
		}
		else
		{
			continue;
		}
	}
	mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ


}



int main() {
	init();

	WSADATA wsaData;
	// ʹ��2.2��   ��ʼ��socket  dll
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "��ʼ��ʧ��" << endl;
		return 0;
	}
	else {
		cout << "��ʼ���ɹ�" << endl;
	}
	/* ����Socket,ʹ�����ݱ��׽��� */
	SOCKET ClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ClientSocket == INVALID_SOCKET)
	{
		cout << "�ͻ���Socket����ʧ��" << endl;
		WSACleanup();
		return 0;
	}
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;     //ָ��IP��ʽ  
	USHORT uPort = 8093;                 //�����������˿�  
	ServerAddr.sin_port = htons(uPort);   //�󶨶˿ں�  
	//ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //IP��ַϵͳ�Զ�����
	inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);
	int len = sizeof(ServerAddr);
	//��������
	int isError = Connect(ClientSocket, ServerAddr, len);

		// ��������
	string filename;
	int filesize = 0;
	cout << "�������ļ�����" << endl;
	cin >> filename;

	// �˳�ָ��
	while (filename != "quit")
	{
		// �����ļ���
		send(ClientSocket, ServerAddr, len, (char*)(filename.c_str()), filename.length());
		// ��������   ��������ͷ���
		clock_t start = clock();
		ifstream fin(filename.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
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
		cout << "������ʱ��Ϊ:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
		cout << "������Ϊ:" << ((float)filesize) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
		cout << state[0x1] << endl;
		cout << "�������ļ�����" << endl;
		cin >> filename;
	}
	quit(ClientSocket, ServerAddr, len);

	disConnect(ClientSocket, ServerAddr, len);

}
