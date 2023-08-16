#include "CLanClient.h"
unsigned __stdcall _NET_Client_WorkerThread(void* param)
{
	CLanClient* client = (CLanClient*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return client->CLanClientWorkerThread(param);
}
//---------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------------
//Accept Thread Wrapping function
// param CNetServer*
unsigned __stdcall _NET_Client_AcceptThread(void* param)
{
	CLanClient* client = (CLanClient*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return client->CLanClientAcceptThread(param);
}


CLanClient::CLanClient()
{

};

CLanClient::~CLanClient()
{

};

bool  CLanClient::Connect(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff)
{
	timeBeginPeriod(1);
	this->PORT = CPORT;
	this->_Start_Time = _Cur_Time = timeGetTime();
	this->Thread_Count = CThraed_Count;
	this->Thread_Running = CThread_Running;
	this->Nagle_Onoff = CNagle_Onoff;
	//this->init_Set();//���ӱ⺻����
	this->init_socket();//���ϼ���
	InitializeSRWLock(&Player_Session->_LOCK);

	//������ ����ó��

	//������ƮǮ ����

	//������ ��� ����

	return true;
};

void  CLanClient::Stop()
{
	//������ DB ����

	//Accept������ ����

	//��Ŀ����������

	//����͸� ����������


};

unsigned int CLanClient::CLanClientAcceptThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//������ ��ſ� ����� ����
	SOCKET clientsock;
	SOCKADDR_IN clientaddr;
	SOCKADDR_IN serveraddr;
	int addrlen = sizeof(clientaddr);
	DWORD recvbytes;

	while (true) {
		//accept()
		//clientsock = Connect(this->listen_socket, (SOCKADDR*)&clientaddr, &addrlen);
		//if (clientsock == INVALID_SOCKET) {
		//	printf("accept()");
		//	break;
		//}

		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // �ڱ� �ڽſ��� ������
		serveraddr.sin_port = htons(3500);

		int client_len = sizeof(serveraddr);

		if (connect(con_socket, (struct sockaddr*)&serveraddr, client_len) == -1) {
			perror("connect error : ");
			exit(0);
		}
		
		if (g_Shutdown)//�÷��װ� ����ٸ� �ڵ�����
			break;

		printf("[TCP����] Ŭ���̾�Ʈ ���� IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//���ǻ���
		Player_Session = new SESSION;
		if (Player_Session == NULL)
			break;

		ZeroMemory(&Player_Session->_SendOver, sizeof(Player_Session->_SendOver));
		ZeroMemory(&Player_Session->_RecvOver, sizeof(Player_Session->_RecvOver));

		//_Session_User->_Id = UNI_KEY++;
		Player_Session->_Sock = con_socket;
		Player_Session->_Sock_Addr = serveraddr;///�����ּ�
		Player_Session->_SendBuf.ClearBuffer();
		Player_Session->_RecvBuf.ClearBuffer();
		Player_Session->_IO_COUNT = 1;
		Player_Session->_IO_Flag = false;
		InitializeSRWLock(&Player_Session->_LOCK);

		int bufSize = 0;
		int optionLen = 4;

		//setsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
		//getsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, &optionLen);

		//�񵿱� ����� ����
		DWORD flags = 0;

		//AcquireSRWLockExclusive(&this->SESSION_MAP_LOCK);
		//this->SESSION_MAP.emplace(_Session_User->_Id, _Session_User);
		//ReleaseSRWLockExclusive(&this->SESSION_MAP_LOCK);

		CreateIoCompletionPort((HANDLE)Player_Session->_Sock, this->IOCP_WORKER_POOL, (ULONG_PTR)Player_Session, 0);

		this->RecvPost(Player_Session);
	}
	printf("AcceptThread STOP!\n");
	return 0;
}



unsigned int CLanClient::CLanClientWorkerThread(void* pComPort)
{
	printf("WorkerThread START!\n");
	int retVal;
	HANDLE hcp = (HANDLE)pComPort;//IOCP��Ʈ����
	WCHAR buf[256];
	DWORD64 value;
	DWORD cbTransferred;
	ULONG_PTR CompletionKey;
	LPOVERLAPPED pOverlapped;

	DWORD thisThreadID = GetCurrentThreadId();

	while (true)
	{
		ZeroMemory(&cbTransferred, sizeof(DWORD));
		ZeroMemory(&CompletionKey, sizeof(ULONG_PTR));
		ZeroMemory(&pOverlapped, sizeof(LPOVERLAPPED));

		cbTransferred = 0;
		CompletionKey = 0;

		retVal = GetQueuedCompletionStatus(IOCP_WORKER_POOL, &cbTransferred, &CompletionKey, &pOverlapped, INFINITE);

		SESSION* _pSession = (SESSION*)CompletionKey;
		//printf("�����庰 ó���� cbTransferred %d\n", cbTransferred);
		//printf("SESSION IOCOUNT=0����ºκ� �ֳ� Ȯ��%d\n", _pSession->_IO_COUNT);

		if (g_Shutdown) break;//������ ����
		if (_pSession == nullptr)
			continue;//������ ������� �����常 �ٽ� ���� �����̶�� �Ǵ��ϰ� �����ϴ� ����� ����
		if (CompletionKey == NULL)
			break;//Ű�� ���ٸ� ����
		if (pOverlapped == nullptr)
			continue;


		//printf("ó����Ȯ�� %d\n", cbTransferred);
		//�񵿱� ����� ��� Ȯ��
		if (retVal == 0 || cbTransferred == 0)
		{
			//if (retVal == 0) {//�̰������� �������� �ǹ����µ�
			//	DWORD temp1, temp2;
			//	WSAGetOverlappedResult(_pSession->_Sock, &_pSession->_RecvOver, &temp1, FALSE, &temp2);
			//	//err_quit("WSAGetOverlappedResult()");
			//}
			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
			{
				SOCKADDR_IN clientaddr;
				int addrlen = sizeof(clientaddr);
				getpeername(_pSession->_Sock, (SOCKADDR*)&clientaddr, &addrlen);
				printf("out[TCP ����] Ŭ���̾�Ʈ ���� : IP  �ּ� = %s, ��Ʈ��ȣ = %d\r\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				//this->SesionRelease(_pSession);
				Disconnect();
			}
			continue;
		}

		//recv �Ϸ����� ��ƾ
		if ((&_pSession->_RecvOver) == pOverlapped)//RECV�۾�����
		{
			//InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
			InterlockedIncrement64((LONG64*)&_RecvCount);

			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//������ŭ ���������� �̵�

			CSerealBuffer CPacket;//����ȭ ���� ��������

			ZeroMemory(buf, 256);
			ZeroMemory(&value, sizeof(value));

			while (true) //���� ��Ŷ��� ����� �����ۿ� �ִ´�.
			{
				//AcquireSRWLockExclusive(&_pSession->_LOCK);
				CPacket.Clear();
				int _RetUseSize = _pSession->_RecvBuf.GetUseSize();
				if (_RetUseSize < sizeof(LanServerHeader))
				{
					break;
				}
				LanServerHeader st_recv_header;
				int retPeek = _pSession->_RecvBuf.Peek((char*)&st_recv_header, sizeof(LanServerHeader));//���ũ�⸸ŭ ������

				if (_RetUseSize < sizeof(LanServerHeader) + st_recv_header._Len)
				{
					break;
				}

				//������� �о�� ũ�⸸ŭ ���ۿ� �����Ѵٸ�?
				_pSession->_RecvBuf.MoveRead(sizeof(LanServerHeader));//���ũ�⸸ŭ ������ �б������͸� �̵���Ų��.

				int retSwitchDeq = _pSession->_RecvBuf.Peek(CPacket.GetBufferPtr(), st_recv_header._Len);

				if (retSwitchDeq != st_recv_header._Len)
				{
					//��������
				}
				_pSession->_RecvBuf.MoveRead(retSwitchDeq);

				int retCS = CPacket.MoveWritePos(retSwitchDeq);//�����͸� ������ŭ ����ȭ ���۸� �̵���Ų��.

				//8����Ʈ ������ OnMessage ȣ��
				this->OnRecv(_pSession->_Id, &CPacket);//������ۿ� �����ͽױ�
			}

			//WSASEND();
			this->SendPost(_pSession);

			//WSARECV();
			this->RecvPost(_pSession);
		}
		//send�Ϸ�����
		if ((&_pSession->_SendOver) == pOverlapped)
		{
			//printf("Send THread%d\n,", thisThreadID);
			InterlockedIncrement64((LONG64*)&_SendCount);
			_pSession->_SendBuf.MoveRead(cbTransferred);//��������ŭ �б���� �̵�
			//_pSession->_SendBuf.MoveReadPtr(cbTransferred);//��������ŭ �б���� �̵�
			InterlockedDecrement(&_pSession->_IO_COUNT);//�Ϸ������� ��ٸ� ����� �����ٴ¶�
			InterlockedExchange(&_pSession->_IO_Flag, false);//�Ϸ������� �Դٴ°� �ϳ��� ������ �����ٴ� ���̹Ƿ� �����÷��׸� false�� �����
			if (_pSession->_SendBuf.GetUseSize() > 0)//�������� ���� �����Ͱ� �����ִٸ�?
				this->SendPost(_pSession);//�׸�ŭ �ٽ� ����
		}
	}
	printf("WORKER STOP!\n");
	return 0;
}

void CLanClient::init_socket()
{
	WSADATA wsaData;

	//���� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup");

	//����� �Ϸ� ��Ʈ ����
	this->IOCP_WORKER_POOL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, Thread_Running);
	if (IOCP_WORKER_POOL == NULL) return;

	//CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//���� ����
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(listen_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	if (listen_socket == INVALID_SOCKET)
		printf("socket");
	bool enable = true;
	setsockopt(listen_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));

	int optval = 0;
	int optlen = sizeof(optval);
	setsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optlen));

	int option = Nagle_Onoff;               //���̱� �˰��� on/off
	setsockopt(listen_socket,             //�ش� ����
		IPPROTO_TCP,          //������ ����
		TCP_NODELAY,          //���� �ɼ�
		(const char*)&option, // �ɼ� ������
		sizeof(option));      //�ɼ� ũ��

	SOCKADDR_IN sock_addr;
	ZeroMemory(&sock_addr, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(PORT);
	sock_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(listen_socket, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) printf("Bind");
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) printf("listen");

	for (int i = 0; i < Thread_Count; i++)
	{
		_CreateThread[i] = (HANDLE)_beginthreadex(nullptr, 0, _NET_Client_WorkerThread, this, 0, nullptr);
	}

	this->_CreateThread[Thread_Count] = (HANDLE)_beginthreadex(nullptr, 0, _NET_Client_AcceptThread, this, 0, nullptr);

}

void CLanClient::RecvPost(SESSION* _pSession)
{
	DWORD flags = 0;
	WSABUF wsaBuf[2];

	ZeroMemory(&_pSession->_RecvOver, sizeof(OVERLAPPED));
	wsaBuf[0].buf = _pSession->_RecvBuf.GetWriteBufferPtr();
	wsaBuf[0].len = _pSession->_RecvBuf.DirectEnqueueSize();
	wsaBuf[1].buf = _pSession->_RecvBuf.GetStartBufferPtr();
	wsaBuf[1].len = _pSession->_RecvBuf.GetFreeSize() - wsaBuf[0].len;

	if (wsaBuf[1].len > _pSession->_RecvBuf.GetBufferSize())
		wsaBuf[1].len = 0;

	int retVal = WSARecv(_pSession->_Sock, wsaBuf, 2, nullptr, &flags, &_pSession->_RecvOver, nullptr);
	if (retVal == SOCKET_ERROR)
	{
		int retWsa = WSAGetLastError();
		if (retWsa == WSA_IO_PENDING)
		{
			return;
		}
		else if (retWsa != WSA_IO_PENDING) {
			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			printf("Recv WSAGetLastError() %d\n", retWsa);
			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
			{
				Disconnect();
			}
			return;
		}
	}

};

void CLanClient::SendPost(SESSION* _pSession)
{
	if (_pSession->_SendBuf.GetUseSize() == 0)//���� �����Ͱ� ���ٸ�
	{
		InterlockedExchange(&_pSession->_IO_Flag, false);//������ �����ϰ� ����
		return;
	}
	if ((InterlockedExchange(&_pSession->_IO_Flag, true)) == true) return;
	if (_pSession->_SendBuf.GetUseSize() == 0)//���� �����Ͱ� ���ٸ�
	{
		InterlockedExchange(&_pSession->_IO_Flag, false);//������ �����ϰ� ����
		SendPost(_pSession);//Ȥ�ø� Ȯ�ο� �ٽ�����
		return;
	}
	DWORD recvbytes;
	DWORD flags = 0;
	WSABUF wsaBuf[2];

	ZeroMemory(&_pSession->_SendOver, sizeof(OVERLAPPED));
	char* Read = _pSession->_SendBuf.GetReadBufferPtr();
	int DriUse = _pSession->_SendBuf.GetUseSize();
	int DriDe = _pSession->_SendBuf.DirectDequeueSize();

	wsaBuf[0].buf = Read;
	wsaBuf[0].len = DriDe;
	wsaBuf[1].buf = _pSession->_SendBuf.GetStartBufferPtr();
	wsaBuf[1].len = DriUse - DriDe;


	if (wsaBuf[1].len > _pSession->_SendBuf.GetBufferSize())
		wsaBuf[1].len = 0;

	//WSASEND();
	DWORD sendbytes;//������ ����
	InterlockedIncrement(&_pSession->_IO_COUNT);//�ٷ� ���ϵȰ�쿡�� ������̶�� ���� �����ϱ� ������Ű�°� ���ݾ� �ƴѰ�?
	int retVal = WSASend(_pSession->_Sock, wsaBuf, 2, nullptr, 0, &_pSession->_SendOver, nullptr);
	if (retVal == SOCKET_ERROR)
	{
		int retWsa = WSAGetLastError();
		if (retWsa == WSA_IO_PENDING)
		{
			return;
		}
		else if (retWsa != WSA_IO_PENDING)
		{
			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			printf("Send WSAGetLastError() %d\n", retWsa);
			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
			{
				//this->SesionRelease(_pSession);
				Disconnect();
			}
		}
	}
};


bool CLanClient::Socket_Error()
{
	int WSAError = WSAGetLastError();
	if (WSAError == WSA_IO_PENDING) {
		printf("WSA_IO_PENDING()");
		return true;
	}
	if (WSAError == WSAECONNRESET//���� ������ ���� ȣ��Ʈ�� ���� ������ ������ϴ�.
		|| WSAError == WSAECONNABORTED//����Ʈ����� ���� ������ �ߴܵǾ����ϴ�.
		|| WSAError == WSANOTINITIALISED//������ WSAStartup�� ���� ������� �ʾҽ��ϴ�.
		|| WSAError == WSAEWOULDBLOCK//�� ������ ��� �Ϸ��� �� ���� ����ŷ ������ �۾����� ��ȯ�˴ϴ�
		|| WSAError == WSAEFAULT)//�ּҰ� �߸��Ǿ����ϴ�. ���ø����̼��� �߸��� ������ ���� �����ϰų� ������ ���̰� �ʹ� ���� ��쿡 �߻�

	{
		//return;
	}

	if (WSAError != WSA_IO_PENDING) {
		printf("WSASend()");
		return false;
	}
}

void CLanClient::SendPacket(CSerealBuffer* CPacket)
{
	int CPacketSize = CPacket->GetUseSize();//�Ѿ�� ��Ŷ�� ����� Ȯ���Ѵ�

	//LAN WAN�����ؼ� �������
	LanServerHeader st_header;
	st_header._Len = CPacketSize;//��Ŷ�� �ִ����

	CSerealBuffer SendPacket;//�������� �����͸������Ѵ�
	SendPacket.PutData((char*)&st_header, sizeof(LanServerHeader));//����� �ְ�
	SendPacket.PutData(CPacket->GetBufferPtr(), CPacketSize);//��Ŷ���� �����͸� �ٷ� �̾Ƽ� �ִ´�.
	
	AcquireSRWLockExclusive(&Player_Session->_LOCK);//�˻��� ���� �����Ѵٴ� ���̹Ƿ� �����Ǵ�
	Player_Session->_SendBuf.Enqueue((char*)SendPacket.GetBufferPtr(), SendPacket.GetUseSize());//���Ͽ� ������ۼ����� ���۰� �ֱ�
	ReleaseSRWLockExclusive(&Player_Session->_LOCK);//���� �����Ѵ�.
};

bool CLanClient::Disconnect()
{
	//closesocket(Player_Session->_Id);
	//delete Player_Session;
}
