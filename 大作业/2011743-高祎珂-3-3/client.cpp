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

const int MAXSIZE = 8160;//���仺������󳤶�
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

//���Ͷ˻���
char** sendbuf;
//vector<pair<int, int>> time_list;//����timer��¼���ͳ�ȥ��ʱ���, order
//δ�õ�ȷ�ϵĵ�һ����
int base = 0;
//���Ͱ������һ��
int tail = 0;
//ʣ�µ�Ҫ���Ͱ���
int pagenum = 0;
//�������ݰ�����
int save_pagenum;
//�ļ�����
int length = 0;
//���һ�����ĳ���
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
	cout<< "[SEND]  ��һ��������Ϣ     FLAG��SYN" << endl;
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
		cout << "[SEND]  ������������Ϣ     FLAG��ACK" << endl;
		if (res == sizeof(HEADER))
			cout << "�ɹ���������" << endl;
		return 1;
	}
	else {
		cout << "��������ʧ��" << endl;
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
			//cout << "now clock" << nowclock << " time_list["<<base<<"] " << time_list[base] << "  ʱ���   " << int(nowclock - time_list[base]) << endl;
				//�ظ�ack
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

				cout << "��ʱ�ش� Seq= from " << base << " to " << tail << endl;
				cout << "---------------------------------------------------------" << endl;
				for (int i = base; i < tail; i++)   //��������SendBase ~ Seq-1 �ı���
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
					cout << "�ط����Ĵ�С" << msg.datasize << " ���Ͱ��Ļ���������" << i << " ʣ�೤�� ��" << length << endl;
					
					time_list[i] = clock();
					//time_list[i-base]=make_pair(clock(), msg.Seq);
					//time_list.pop();
				}
				// �ָ��̲߳���
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
			//cout << "�յ���[RECV]  SEQ: " << result.Seq - 1 << endl;

		//cout << "here1 time_list:"<< time_list.size() << endl;

			//���յ��İ�û��������Ҫ��
		if (result.get_ACK() && !result.get_NAK() && result.Seq == base + 1)
		{

			time_list[base] = MAXTIME;
			//cout << "�ı�time_list["<<base<<"]Ϊ *************" << time_list[base] << endl;
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
			cout << "[RECV]  SEQ: " << result.Seq - 1 << "   FLAG: ACK WANT:" << base - 1 << "��ǰ����" << base << " to " << tail << "���ڴ�С" << NUM_WINDOW << endl;
		}
		//����·�����л�ط������˴��صİ�
		else if (result.Seq > base + 1) {
			base = result.Seq - 1;
			time_list[base] = MAXTIME;
		}
	
		else {
			cout << "[RECV]  ���Եİ� SEQ: " << result.Seq - 1 << "   FLAG: ACK WANT:" << base << endl;
				
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
	//ʱ���б��ʼ��
	for (int i = 0; i < 500; i++)
		time_list[i] = 0;
	
	HEADER msg;

	cout << "�������ļ�����" << endl;
	string filename = "";
	cin >> filename;
	// �����ļ���
	sendto(ClientSocket,(char*)(filename.c_str()), filename.length(),0,(SOCKADDR*)&ServerAddr,l);

	//���ļ�
	sendbuf = new char* [80000];
	for (int i = 0; i < 80000; i++)
		sendbuf[i] = new char[MAXSIZE];
	
	ifstream filein;
	filein.open(filename, ifstream::binary);
	cout << "�ļ��Ƿ�򿪣�" << filein.is_open() << endl;
	while (!filein.is_open())
	{
		cout << "�Ҳ������ļ������������룬��ȷ���ļ��Ƿ����" << endl;
		cin >> filename;
		filein.open(filename, ifstream::binary);
		if (filein.is_open())
			cout << "�ļ��Ѵ�" << endl;
	}

	filein.seekg(0, filein.end);  //���ļ���ָ�붨λ������ĩβ
	length = filein.tellg();
	cout << "�ļ����ȣ�" << length << endl;

	filein.seekg(0, filein.beg);  //���ļ���ָ�����¶�λ�����Ŀ�ʼ
	
	
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
	//��ʼѭ����������
	while (tail< save_pagenum)
	{
		//cout << "here..." << endl;
		
		//��ʱ���ͣ������ȵ�һ��
		while (!istimeout&& tail < save_pagenum)

		{
			//cout << "tail " << tail << endl;
			if (tail - base < NUM_WINDOW&& tail < save_pagenum)
			{
				SetPriorityClass(GetCurrentThread(), HIGH_PRIORITY_CLASS);

				if (sendnum == 1)
				{
					//msg.set_EOF();
					cout << "���һ�����ĳ���" << length << endl;
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
				//time_list.push_back(make_pair(clock(), msg.Seq));//��ʼ��ʱ����ĩβ����һ����Ԫ��
				cout << "[SEND] ���Ͱ��ĳ���" << msg.datasize << " ���Ͱ��Ļ���������" << tail << " SEQ:" << int(msg.Seq) << " ʣ�೤�� ��" << length << endl;
				
				//cout << clock() << endl;
				tail++;
				sendnum--;
				length -= MAXSIZE;

				//cout << "[Send]  " << msg.datasize << " bytes!" << " SEQ:" << int(msg.Seq) << " SUM:" << int(msg.Checksum) << endl;
				cout << "��ǰ���ڣ� " << base << " to " << tail << endl;
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
	cout << "�������һ�����ݰ�" << endl;
	clock_t end = clock();

	stime = (end - start) / CLOCKS_PER_SEC;
	cout << "�ļ��������" << endl;
	cout << "������ʱ" << stime << "s" << endl;
	double throupt_rate = save_pagenum * sizeof(HEADER) / 1024 / stime;
	cout << "������Ϊ" << throupt_rate << "Kb/s" << endl;
	cout << "�������" << endl;
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
	cout << "[SEND]  ��һ�λ�����Ϣ     FLAG��ACK_FIN" << endl;
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
		cout << "[SEND]  �����λ�����Ϣ     FLAG��ACK" << endl;
		if (res == sizeof(HEADER))
		{
			cout << "�ɹ��Ͽ�����" << endl;
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
		cout << "�����������q�����������ַ�������" << endl;
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