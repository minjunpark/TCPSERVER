// SeleteModel.cpp : �� ���Ͽ��� 'main' �Լ��� ���Ե˴ϴ�. �ű⼭ ���α׷� ������ ���۵ǰ� ����˴ϴ�.
//

#pragma comment(lib,"ws2_32")
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdlib>
#include <cstdio>
#include <clocale>
#include <conio.h>
#include <memory.h>
#include <Windows.h>
#include <time.h>
#include "TRingBuffer.h"
#include "CList.h"
#include "Server_define.h"
#include "Packet_Struct.h"
#include "Console.h"

CList<Session*> Session_List;//������ ��������Ʈ
int Session_Total = 0;

//SOCKETINFO* SocketInfoArray[FD_SETSIZE];//���ϸ���Ʈ

//������ �޾Ƶ��̱� ���� ��������
SOCKET listen_sock;
int nTotalSockets = 0;//�� ���� ����

int packet_count;//��Ŷ �Ѱ���

//ȭ�� �������� �Լ��� ���� START
void Buffer_Flip(void);
void Buffer_Clear(void);
void Sprite_Draw(int iX, int iY, char chSprite);
char szScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];
char szScreen[23][83] = { ' ', };
//ȭ�� �������� �Լ��� ���� END


//���� ��� �Լ�
void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);

//���� ���� ���ú��Լ�
void AcceptProc();//listen���� ó��
void RecvProc(Session* session);//���� �޼���ó�� ���μ���
void SendProc(Session* session);
void sendUniCast(Session* session, char* _Msg);//Ư���������Ը� ������
void sendBroadCast(Session* session, char* _Msg);//��� �������� ������

void Disconnect(Session* session);
void Disconnect_Clean();
void NetWork();
void Render();

void log_msg(char* msg);//�α�������Լ�

//�α������ �ؽ�Ʈ���� ����


//Ȯ�ο� �Լ������� ����
int retSelect;
SOCKET client_sock;
SOCKADDR_IN clientaddr;
int addrlen;
FD_SET rset;
FD_SET wset;

timeval times;


int main()
{
	cs_Initial();

	times.tv_sec = 0;
	times.tv_usec = 0;

	//�ѱۼ���
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");
	
	//���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	int retOut;

	//listen socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit(L"socket()");

	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	//linger timewait 0 rst�ٷ� �����ϰ� �����
	linger _linger;
	_linger.l_onoff = 0;
	_linger.l_linger = 0;

	int keepval = 0;

	setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (const char*)&_linger, sizeof(_linger));//���ŷ� Ÿ�Ӿƿ� 0
	setsockopt(listen_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepval, sizeof keepval);//keep alive���� �����Ұ�

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	retOut = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retOut == SOCKET_ERROR)err_quit(L"bind()");

	//listen()
	retOut = listen(listen_sock, SOMAXCONN);
	if (retOut == SOCKET_ERROR)err_quit(L"listen()");

	//�� ���ŷ �������� ��ȯ
	u_long on = 1;
	retOut = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retOut == SOCKET_ERROR)
		err_quit(L"ioctlsocket()");

	while (1)
	{
		NetWork();//��Ʈ��ũ�۾�
		Render();//������
	}

	//���� �ݱ�
	closesocket(listen_sock);

	//��������
	WSACleanup();
	return 0;
}


void NetWork()
{
	//������ ��ſ� ����� ����

	//��Ʈ��ŷ �۾�
	FD_ZERO(&rset);//rset�� �о������ �ʱ�ȭ
	FD_ZERO(&wset);
	FD_SET(listen_sock, &rset);//���������� rset���Ͽ� �����Ѵ�

	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		FD_SET((*session_it)->sock, &rset);//������ �ִ¼����� ���� rset�� �ִ´�.

		if ((*session_it)->sendBuf->GetUseSize() >= 0)//���� ���ۿ� �������ϴ� �����Ͱ� 0�̻��̶��?
		{
			FD_SET((*session_it)->sock, &wset);
		}
	}

	retSelect = select(0, &rset, &wset, NULL, &times);//������ �ִ� ������ �ִ��� ����Ʈ�� Ȯ��

	if (retSelect == SOCKET_ERROR)
	{
		err_quit(L"select()");
	}


	if (retSelect > 0)//������ �ִ� ������ �ִٸ�?
	{
		if (FD_ISSET(listen_sock, &rset))//�������Ͽ� ������ �ִٸ�
		{
			AcceptProc();
		}

		//���� ���Ǹ���Ʈ�߿� ������ �ִ� ������ �ִ��� Ȯ���ϱ����� ��� ����Ʈ�� ���鼭 Ȯ���Ѵ�.
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if (FD_ISSET((*session_it)->sock, &rset))//������ �ִ� �б� ������ �ִٸ�?
			{
				RecvProc(*session_it);//Recv Proc�� �����Ѵ�.
			}

			if (FD_ISSET((*session_it)->sock, &wset))//���������� ��������� �ִٸ�
			{
				SendProc(*session_it);
			}
		}
	}
	Disconnect_Clean();//������ ���� �༮���� ��������Ѵ�.
}

void SendProc(Session* session)
{
	char buf[10000];
	memset(buf, 0, sizeof(buf));//������ �о������
	int retPeek = session->sendBuf->Peek(buf, session->sendBuf->GetUseSize());//���ۿ��� Usesize��ŭ ��������

	int retSend = send(session->sock, buf, retPeek, 0);//�����¸�ŭ ����

	if (retSend == SOCKET_ERROR)//���� ��ü�� ������ ����ٴ� ���̹Ƿ�
	{
		int wsaError;//���Ͽ�����
		wsaError = WSAGetLastError();

		if (wsaError != 10035
			|| wsaError != 10054
			|| wsaError != WSAEWOULDBLOCK)//�̿����� �ƴ϶�� �α������Ѵ�.
		{
			char buff[200];
			memset(buff, 0, sizeof(buff));
			sprintf_s(buff, "retRecv Error = %d", wsaError);
			log_msg(buff);
		}

		if (wsaError != WSAEWOULDBLOCK)//������ü�� ������ ����ٴ� ���̹Ƿ�
		{
			//printf("retval error %d\n", retRecv);
			Disconnect(session);//��������ó��
			return;//�۽Ź��۰� ����ִٴ� ���̹Ƿ� ������ Ż���Ѵ�.
		}
		//return;
	}

	if (retSend != retPeek)//�ܾ�°Ͱ� �������� �ٸ���? �����۰� �ڻ쳪�� �̻��ϰ� ����������̴� ������ �����ϰ� �α׸� ���ܾ��Ѵ�.
	{
		Disconnect(session);
		return;
	}

	//session->sendBuf->MoveFront(retSend);
	//session->sendBuf->Dequeue(buf,retSend);//��ť�� �����Ǵ��� �׽�Ʈ�߰�
	session->sendBuf->MoveFront(retSend);//Frontũ�⸸ŭ �̵���Ų��.
};

void AcceptProc()//listen���� ó��
{
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	addrlen = sizeof(clientaddr);
	client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);

	if (client_sock == SOCKET_ERROR)
	{
		return;//�׳� ���ῡ�� �߻��̴ϱ� ��ó�� �� �ʿ� ���� �����Ѵ�.
		//Disconnect();
	}

	int AccteptTotal = nTotalSockets++;//���⼭ ��� +�ϸ鼭 ID ���� ���� �����Ѵ�.

	//1.���ο� ����� ������ �����Ѵ�
	Session* NewSession = new Session;
	NewSession->sendBuf = new TRingBuffer;
	NewSession->recvBuf = new TRingBuffer;
	NewSession->sock = client_sock;//sock�� �����ϰ�
	NewSession->Id = AccteptTotal;//ID����
	NewSession->X = _e_Player::_X;//X����
	NewSession->Y = _e_Player::_Y;//Y����
	NewSession->live = true;//���� �����÷��� 0�̸� ���

	////2.���̵� ���� ��Ŷ ������ ������ ����ڿ��� �����Ѵ�.
	STAR_ID _Id_Pack;
	_Id_Pack._Type = ID_SET;
	_Id_Pack._Id = AccteptTotal;
	sendUniCast(NewSession, (char*)&_Id_Pack);//�� �ڽſ��� ���� �����ؼ� �����Ѵ�.

	CList <Session*>::iterator _Session_it;
	//CBaseObject* _Des_Object;
	//4.���� ����Ʈ�� �ִ� �������� ���� �����Ѵ�.
	for (auto _Session_it = Session_List.begin();
		_Session_it != Session_List.end(); ++_Session_it)
	{
		if ((*_Session_it)->live == true)
		{
			STAR_CREATE _Cre_Pack;
			_Cre_Pack._Type = CREATE_SET;
			_Cre_Pack._Id = (*_Session_it)->Id;
			_Cre_Pack._X = (*_Session_it)->X;
			_Cre_Pack._Y = (*_Session_it)->Y;
			sendUniCast(NewSession, (char*)&_Cre_Pack);
		}
	}

	Session_List.push_back(NewSession);//������ ������ ���Ǹ���Ʈ�� �ִ´�.

	////3.������ �� ��Ŷ�� ��� ����ڿ��� �����Ѵ�.
	STAR_CREATE _Cre_Pack;
	_Cre_Pack._Type = CREATE_SET;
	_Cre_Pack._Id = NewSession->Id;
	_Cre_Pack._X = _X;
	_Cre_Pack._Y = _Y;
	sendBroadCast(nullptr, (char*)&_Cre_Pack);//����� ���� ���ο��� �����Ѵ�.
};

void RecvProc(Session* session)//���� �޼���ó�� ���μ���
{
	int retRecv;
	Session* tmpSesion;
	tmpSesion = session;

	char buf[10000];


	//while (true)
	//{
		//if (tmpSesion->live!=true)//������ ���������쿡��
		//{
		//	return;
		//}

	memset(buf, 0, sizeof(buf));//���� clean

	retRecv = recv(tmpSesion->sock, (char*)buf, tmpSesion->recvBuf->GetFreeSize(), 0);//�����ִ� Free�����ŭ �ܾ�´�.

	if (retRecv == SOCKET_ERROR)//���� ��ü�� ������ ����ٴ� ���̹Ƿ�
	{
		int wsaError;//���Ͽ�����
		wsaError = WSAGetLastError();

		if (wsaError != 10035
			|| wsaError != WSAECONNRESET
			|| wsaError != WSAEWOULDBLOCK)//�̿����� �ƴ϶�� �α������Ѵ�.
		{
			char buff[200];
			memset(buff, 0, sizeof(buff));
			sprintf_s(buff, "retRecv Error = %d", wsaError);
			log_msg(buff);
		}

		if (wsaError != WSAEWOULDBLOCK)//������ü�� ������ ����ٴ� ���̹Ƿ�
		{
			//printf("retval error %d\n", retRecv);
			Disconnect(tmpSesion);//��������ó��
			return;//�۽Ź��۰� ����ִٴ� ���̹Ƿ� ������ Ż���Ѵ�.
		}
	}

	int retRing = tmpSesion->recvBuf->Enqueue((char*)buf, retRecv);

	if (retRecv != retRing)
	{
		Disconnect(tmpSesion);
		return;
	}

	while (1)
	{
		if (tmpSesion->recvBuf->GetUseSize() < 16)//����Ҽ� �ִ»���� 16���϶�� ���̻� ó���Ҽ� �ִ� ������ ���ٴ°��̹Ƿ� �ߴ�.
		{
			break;
		}

		int buffer[4];
		int retEn = tmpSesion->recvBuf->Dequeue((char*)buffer, sizeof(STAR_ID));//16����Ʈ�� ������

		int p_Type = buffer[0];//Type
		int p_Id = buffer[1];//ID
		int p_X = buffer[2];//_X
		int p_Y = buffer[3];//_Y
		//���� ��Ŷ�� �´ٸ�

		switch (p_Type)
		{
		case _STAR_MOVE://���̵�
		{
			STAR_MOVE _send_Move;
			Session* session;
			for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
			{
				session = (*session_it);
				if (session->Id == p_Id)//���� �̵���Ŷ�� �������̶��
				{
					_send_Move._Type = MOVE_SET;
					_send_Move._Id = p_Id;
					if (p_X < 0)
						p_X = 0;
					else if (p_X >= SCREEN_WIDTH)
						p_X = SCREEN_WIDTH;
					if (p_Y < 0)
						p_Y = 0;
					else if (p_Y >= SCREEN_HEIGHT)
						p_Y = SCREEN_HEIGHT;
					_send_Move._X = p_X;
					_send_Move._Y = p_Y;
					session->X = p_X;
					session->Y = p_Y;
					//���ڽ��� �����ϰ� ��Ŷ�� �����Ѵ�.
					sendBroadCast(session, (char*)&_send_Move);
					break;//�극��ũ
				}
			}
		}
		break;//_STAR_MOVE break;
		}
	}

	//}
};


void sendUniCast(Session* session, char* _Msg)//Ư���������Ը� ������
{
	int retUni;

	if (session->live == false)
	{
		return;//�׾��ִ���Ŷ���״¾Ⱥ�����.
	}
	if (session->sendBuf->GetFreeSize() >= sizeof(STAR_ID))//sendbuf�� 16����Ʈ �̻��� �����ִٸ� ��ť��Ų��.
	{
		retUni = session->sendBuf->Enqueue(_Msg, sizeof(STAR_ID));//�ȵ��� �ȵ��µ��� ��������� ���۰� �����Ŵϱ�
		if (retUni != sizeof(STAR_ID))
		{
			Disconnect(session);
			return;
		}
	}
};

void sendBroadCast(Session* session, char* _Msg)//Ư�� ������ ���� ������
{
	int retBro;
	Session* stmp = session;//������ ���� ����

	//��� ����Ʈ�����鼭 Ȯ���Ѵ�.
	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->Id == (*session_it)->Id)//���� ���ٸ� �̳༮�� ������ �����༮�̴� ĵ���Ѵ�.
		{
			continue;
		}

		if ((*session_it)->live == false)
		{
			continue;//�׾��ִ���Ŷ���״¾Ⱥ�����.
		}
		//�������� ������ۿ� enqueue��Ų��.
		if ((*session_it)->sendBuf->GetFreeSize() >= sizeof(STAR_ID))//sendbuf�� 16����Ʈ �̻��� �����ִٸ� ��ť��Ų��.
		{
			retBro = (*session_it)->sendBuf->Enqueue(_Msg, sizeof(STAR_ID));//�ȵ��� �ȵ��µ��� ��������� ���۰� �����Ŵϱ�
			if (retBro != sizeof(STAR_ID))
			{
				Disconnect((*session_it));
			}
		}
		
	}
};

void Disconnect(Session* session)//���� ���� �Լ�
{
	Session* stmp = session;
	int _Id;
	_Id = stmp->Id;
	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		if (_Id == (*session_it)->Id)//���� ���ٸ� ���ó���Ѵ�.
		{
			(*session_it)->live = false;//���� ���ó��
			break;
		}
	}
};

void Disconnect_Clean()//�������� ���� ��Ŀ��Ʈ�� ��ü�� ��Ʈ��ũ ������������ �����Ѵ�.
{
	CList <Session*>::iterator _Session_it;
	Session* _Session_Object;
	for (_Session_it = Session_List.begin(); _Session_it != Session_List.end();)
	{
		_Session_Object = *_Session_it;
		if (_Session_Object->live != true) //���� Ư����ü�� �׾��ִٸ�
		{
			STAR_DELETE p_delete;//���� ��Ŷ�� �����Ѵ�.
			p_delete._Type = DELETE_SET;
			p_delete._Id = _Session_Object->Id;
			sendBroadCast(nullptr, (char*)&p_delete);//������ ��Ŷ�� �����Ѵ�.
			closesocket(_Session_Object->sock);
			delete _Session_Object->recvBuf;
			delete _Session_Object->sendBuf;
			delete _Session_Object;//�����Ҵ�� ��ü ����
			_Session_it = Session_List.erase(_Session_it);//�� ��ü�� ����Ʈ���� �������� ���� ���ͷ����͸� ���Ϲ޴´�.;
		}
		else
		{
			++_Session_it;//����ִ� ��ü��� ���� ����Ʈ�� �Ѿ��.
		}
	}
}

void Render()
{
	Buffer_Clear();//���������

	Sprite_Draw(0, 0, 'C');
	Sprite_Draw(1, 0, 'o');
	Sprite_Draw(2, 0, 'n');
	Sprite_Draw(3, 0, 'n');
	Sprite_Draw(4, 0, 'e');
	Sprite_Draw(5, 0, 'c');
	Sprite_Draw(6, 0, 't');

	Sprite_Draw(8, 0, 'C');
	Sprite_Draw(9, 0, 'l');
	Sprite_Draw(10, 0, 'i');
	Sprite_Draw(11, 0, 'e');
	Sprite_Draw(12, 0, 'n');
	Sprite_Draw(13, 0, 't');

	Sprite_Draw(15, 0, ':');

	Sprite_Draw(17, 0, Session_List.size() + 48);

	//Sprite_Draw(19, 0, 'P');
	//Sprite_Draw(20, 0, 'a');
	//Sprite_Draw(21, 0, 'c');
	//Sprite_Draw(22, 0, 'k');
	//Sprite_Draw(23, 0, 'e');
	//Sprite_Draw(24, 0, 't');
	//Sprite_Draw(25, 0, ':');
	//Sprite_Draw(26, 0, packet_count + 48);
	/*Sprite_Draw(27, 0, 'U');
	Sprite_Draw(28, 0, 'L');
	Sprite_Draw(29, 0, 'L');*/

	// ��ũ�� ���ۿ� ��������Ʈ ���
	CList <Session*>::iterator _Session_it;
	Session* _Session_;
	for (_Session_it = Session_List.begin(); _Session_it != Session_List.end(); ++_Session_it)
	{
		_Session_ = *_Session_it;
		Sprite_Draw(_Session_->X, _Session_->Y, '*');
	}
	//��ũ�� ���۸� ȭ������ ���
	Buffer_Flip();
}

//���� �Լ� ���� ��� �� ����
void err_quit(const WCHAR* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

//���� �Լ� ���� ���
void err_display(const WCHAR* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	wprintf(L"[%s] %s", msg, (WCHAR*)lpMsgBuf);
	printf("GetLastError() %d\n", GetLastError());
	LocalFree(lpMsgBuf);
}

//--------------------------------------------------------------------
// ������ ������ ȭ������ ����ִ� �Լ�.
//
// ����,�Ʊ�,�Ѿ� ���� szScreenBuffer �� �־��ְ�, 
// 1 �������� ������ �������� �� �Լ��� ȣ���Ͽ� ���� -> ȭ�� ���� �׸���.
//--------------------------------------------------------------------
void Buffer_Flip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}


//--------------------------------------------------------------------
// ȭ�� ���۸� �����ִ� �Լ�
//
// �� ������ �׸��� �׸��� ������ ���۸� ���� �ش�. 
// �ȱ׷��� ���� �������� �ܻ��� �����ϱ�
//--------------------------------------------------------------------
void Buffer_Clear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHT);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		szScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}

}

//--------------------------------------------------------------------
// ������ Ư�� ��ġ�� ���ϴ� ���ڸ� ���.
//
// �Է� ���� X,Y ��ǥ�� �ƽ�Ű�ڵ� �ϳ��� ����Ѵ�. (���ۿ� �׸�)
//--------------------------------------------------------------------
void Sprite_Draw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHT)
		return;

	szScreenBuffer[iY][iX] = chSprite;
}

void log_msg(char* msg)
{
	
	char log_FileName[60];// = "127.0.0.1";
	FILE* fp;
	//�α������
	time_t now = time(NULL);  //now�� ���� �ð� ����
	struct tm date;   //tm ����ü Ÿ���� ��¥
	localtime_s(&date, &now);  //date�� now�� ������ ����

	sprintf_s(log_FileName, sizeof(log_FileName), "Log_%02d_%02d_%02d.txt", date.tm_mday, date.tm_hour, date.tm_min);
	fopen_s(&fp, log_FileName, "wb");//���� ����
	fclose(fp);

	fopen_s(&fp, log_FileName, "a");
	fprintf(fp, "%s\n", msg);
	fclose(fp);
}