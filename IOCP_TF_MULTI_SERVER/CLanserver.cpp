#include "CLanServer.h"

CDump CLanDump();

unsigned __stdcall _NET_WorkerThread(void* param)
{
	CLanServer* server = (CLanServer*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanWorkerThread(param);
}
//---------------------------------------------------------------------------------------------------------------------------------

unsigned __stdcall _NET_AcceptThread(void* param)
{
	CLanServer* server = (CLanServer*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanLogicThread(param);
}

unsigned __stdcall _NET_LogicThread(void* param)
{
	CLanServer* server = (CLanServer*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanAcceptThread(param);
}

CLanServer::CLanServer()// : _SESSION_POOL(0, false), _PACKET_POOL(0, false)
{

};

CLanServer::~CLanServer()
{
	//��������� ���ʿ� ��������Ǿߵ�
	//�׷��� ���ʿ� �������μ��� ��ü�� ���ư��°Ŷ� ���� ������ ������ ��ġ�� ����

};

bool  CLanServer::Start(const char* CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count, DWORD backlogSize)
{
	timeBeginPeriod(1);

	if (THREAD_MAXIMUM < CThraed_Count)//�ִ� ������ �������� ���� ��������� �� ����.
	{
		printf("THREAD_MAXIMUM %d \n", THREAD_MAXIMUM);
		return false;
	}

	if (CThraed_Count < CThread_Running)//���� �����庸�� ������ ����.
	{
		printf("Thraed_Count < Thread_Running\n");
		return false;
	}

	InitProfile();//�������� ����� ���� �ʱ�ȭ
	InitializeSRWLock(&SESSION_MAP_LOCK);

	

	this->SNDBUF_ZERO_OnOff = false;//�۽Ź��� ũ�⸦ ���ηθ������ �ȸ������
	strcpy_s(this->CLAN_IP, CIP);//�����ϴ� IP
	this->PORT = CPORT;//�����ϴ� ��Ʈ
	this->_Start_Time = _Cur_Time = timeGetTime();
	this->IOCP_Thread_Count = CThraed_Count;
	this->Thread_Running = CThread_Running;
	this->Nagle_Onoff = CNagle_Onoff;
	this->MAX_SESSION_COUNT = CMax_Session_Count;//�ִ� ���� ����

	if (backlogSize <= 200)//���� ��α�ť ������ 200�� ���϶��
		this->BACK_LOG_SIZE = 200;//200�� ����
	else//200���̻��̶�� ����
		this->BACK_LOG_SIZE = backlogSize;

	_PACKET_POOL_Index = TlsAlloc();//�� ��Ŀ�����忡 ��Ŷ�� ���εα����� ����
	_PACKET_POOL_Count = 0;//��Ŷī��Ʈ

	this->_SESSION_POOL.SetPool(CMax_Session_Count / 2, false);//���������� Ǯ�������.
	//this->_PACKET_POOL.SetPool(1000, false);

	//this->init_Set();//���ӱ⺻����
	this->init_socket();//���� ���� �� ���Ʈ+��Ŀ������ ����

	while (1)
	{
		All_Moniter();//���� ����͸� ���

		int key = _getch();
		if (key == 'w')
		{
			ProfileDataOutText(L"ABC");//�������� ����������
		}
		else if (key == 's')//��������
		{
			Stop();
			Sleep(1000);//�������� ����� �����̸� �ش�.
			break;
		}
		Sleep(0);
	}

	//������ ����ó��

	//������ƮǮ ����

	//������ ��� ����

	return true;
};

void  CLanServer::Stop()
{
	//����Ʈ �����带 ����
	//1.������� ������� ��� IOCOUNT = 0�̵ɋ����� ���
	//������������ msg�� Ȯ���ϰ� ��������ٸ�
	//��Ŀ������ ����
	//DB���� �����ϰ�

	//Acccept Thread����
	printf("Acccept Thread���� START\n");
	closesocket(listen_socket);
	printf("Acccept Thread���� END\n");
	Sleep(1000);

	//������ ���� ��������
	printf("������ ���� �������� START\n");

	for (auto Iter = SESSION_MAP.begin(); Iter != SESSION_MAP.end(); ++Iter)
	{
		Iter->second->_IO_COUNT--;//���ú������� ���� ���������ҰŴϱ� �����Ѵ�.
	}
	Sleep(1000);//������������ ����� ������

	bool session_alive = true;
	while (1)
	{
		AcquireSRWLockExclusive(&SESSION_MAP_LOCK);
		session_alive = false;
		for (auto Iter = SESSION_MAP.begin(); Iter != SESSION_MAP.end(); ++Iter)
		{
			closesocket(Iter->second->_Sock);//������ �α׸�����

			if (Iter->second->_IO_COUNT != 0)
				session_alive == true;//�ϳ��� Ʈ���� IOCOUNT�� 1�� �༮�� send�� �������ΰ��̴�.

			//������ ���������ϴϱ� ���� ����ü�� �������� �ʴ´�.
			//������ �������¸� Ȥ�ó� ������ �����ϱ�
		}
		if (session_alive == false)
			break;
		//SESSION_MAP.clear();//�޸� ���� �������Ұ��� ���ϸ� �������ؼ� �������·� �����.
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
	}
	printf("������ ���� �������� END\n");
	Sleep(1000);//������������ ����� ������

	//��Ŀ����������
	printf("��Ŀ���������� STRART\n");
	g_Shutdown = true;//��� ��Ŀ ������ ���������Ѱ�
	for (int i = 0; i < IOCP_Thread_Count; i++)//GQCS���� �������� ������鿡�� ��ȣ�� �ش�.
		PostQueuedCompletionStatus(IOCP_WORKER_POOL, 0, 0, 0);
	printf("��Ŀ���������� END\n");
	Sleep(1000);//���������������� ������

	//����͸� ����������

	//DB������ �������

	//���ν���������
	//������������� ���ʿ� �ֽ����忡�� return�Ǽ� �ڵ� �����Ѵ�.
};

unsigned int CLanServer::CLanAcceptThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//������ ��ſ� ����� ����
	SOCKET clientsock;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	DWORD recvbytes;
	WCHAR msg[100];

	while (true) {
		//accept()
		clientsock = accept(this->listen_socket, (SOCKADDR*)&clientaddr, &addrlen);
		if (MAX_SESSION_COUNT <= SESSION_MAP.size())//�������� �ִ����Ӽ����� ���ٸ����ӺҰ�
		{
			closesocket(clientsock);
			ZeroMemory(msg, sizeof(msg));
			wcscpy(msg, L"������ �ʰ����� �߻�");
			this->OnError(1001, msg);
			continue;
		}

		//���� ������ ��Ʈ�� �����ȴٸ�?
		if (!this->OnConnectionRequest(msg, 127, clientaddr.sin_port))
		{
			closesocket(clientsock);
			ZeroMemory(msg, sizeof(msg));
			wcscpy(msg, L"OnConnectionRequest false");//���������ܴ��
			this->OnError(1001, msg);
			continue;
		}

		if (clientsock == INVALID_SOCKET) //������ü�� �����߻� ���ӺҰ����¸��� �����ؾ��Ѵ�.
		{
			//printf("accept()");
			wcscpy(msg, L"clientsock == INVALID_SOCKET WORKER������ ����");
			this->OnError(1001, msg);
			break;
		}

		if (g_Shutdown)//�÷��װ� ����ٸ� �ڵ�����
			break;

		//printf("[TCP����] Ŭ���̾�Ʈ ���� IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//���ǻ���
		//st_SESSION* _Session_User = new st_SESSION;

		//����Ǯ�� ��Ŀ�����忡�� free���·� �������ϱ� ������ LOCK�� �ʿ��ϴ�.
		AcquireSRWLockExclusive(&this->SESSION_POOL_LOCK);
		st_SESSION* _Session_User = _SESSION_POOL.Alloc();
		ReleaseSRWLockExclusive(&this->SESSION_POOL_LOCK);

		if (_Session_User == NULL)
			break;

		ZeroMemory(&_Session_User->_SendOver, sizeof(_Session_User->_SendOver));
		ZeroMemory(&_Session_User->_RecvOver, sizeof(_Session_User->_RecvOver));
		_Session_User->_Id = UNI_KEY++;
		_Session_User->_Sock = clientsock;
		_Session_User->_Sock_Addr = clientaddr;
		_Session_User->_SendBuf.ClearBuffer();
		_Session_User->_RecvBuf.ClearBuffer();
		_Session_User->_IO_COUNT = 1;
		_Session_User->_IO_Flag = false;
		InitializeSRWLock(&_Session_User->_LOCK);

		int bufSize = 0;
		int optionLen = 4;

		//�񵿱� ����� ����
		DWORD flags = 0;

		AcquireSRWLockExclusive(&this->SESSION_MAP_LOCK);
		this->SESSION_MAP.emplace(_Session_User->_Id, _Session_User);
		ReleaseSRWLockExclusive(&this->SESSION_MAP_LOCK);

		CreateIoCompletionPort((HANDLE)_Session_User->_Sock, this->IOCP_WORKER_POOL, (ULONG_PTR)_Session_User, 0);

		this->RecvPost(_Session_User);
		InterlockedIncrement64((LONG64*)&_AcceptCount);
		this->OnClientJoin(msg, 127, clientaddr.sin_port, _Session_User->_Id);//Accept�� ���ӿϷ�ȣ��
	}
	printf("AcceptThread STOP!\n");
	return 0;
}



unsigned int CLanServer::CLanWorkerThread(void* pComPort)
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
	WCHAR error_msg[100];
	DWORD currentThreadIdx = (DWORD)TlsGetValue(_PACKET_POOL_Index);

	if (currentThreadIdx == 0)
	{
		//ó���϶�
		currentThreadIdx = InterlockedIncrement(&_PACKET_POOL_Count);
		//_P_ARR_Th[currentThreadIdx].Thread_Id = GetCurrentThreadId();
		_PACKET_POOL_ARR[currentThreadIdx].SetPool(10, false);//�� �����庰 ��Ŷ������ �����
		if (!TlsSetValue(_PACKET_POOL_Index, (LPVOID)currentThreadIdx))
		{
			int* ptr = nullptr;
			*ptr = 100;
		}
	}

	while (true)
	{
		ZeroMemory(&cbTransferred, sizeof(DWORD));
		ZeroMemory(&CompletionKey, sizeof(ULONG_PTR));
		ZeroMemory(&pOverlapped, sizeof(LPOVERLAPPED));

		cbTransferred = 0;
		CompletionKey = 0;
		retVal = 0;

		retVal = GetQueuedCompletionStatus(IOCP_WORKER_POOL, &cbTransferred, &CompletionKey, &pOverlapped, INFINITE);
		this->OnWorkerThreadBegin();//1���� ����

		st_SESSION* _pSession = (st_SESSION*)CompletionKey;
		//printf("�����庰 ó���� cbTransferred %d\n", cbTransferred);
		//printf("SESSION IOCOUNT=0����ºκ� �ֳ� Ȯ��%d\n", _pSession->_IO_COUNT);

		if (g_Shutdown) break;//������ ����
		if (_pSession == nullptr)continue;//������ ������� �����常 �ٽ� ���� �����̶�� �Ǵ��ϰ� �����ϴ� ����� ����
		if (CompletionKey == NULL)break;//Ű�� ���ٸ� ����
		if (pOverlapped == nullptr)continue;

		//printf("ó����Ȯ�� %d\n", cbTransferred);
		//�񵿱� ����� ��� Ȯ��
		if (retVal == 0 || cbTransferred == 0)
		{
			//if (retVal == 0) {//�̰������� �������� �ǹ����µ�
			//	DWORD temp1, temp2;
			//	WSAGetOverlappedResult(_pSession->_Sock, &_pSession->_RecvOver, &temp1, FALSE, &temp2);
			//	//err_quit("WSAGetOverlappedResult()");
			//}
			int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			ZeroMemory(error_msg, sizeof(error_msg));
			wcscpy(error_msg, L"retVal == 0 || cbTransferred == 0 ���۷� ���� ���ϰ��� ����");
			this->OnError(1001, error_msg);
			if (ret_IO == 0)
			{
				SOCKADDR_IN clientaddr;
				int addrlen = sizeof(clientaddr);
				getpeername(_pSession->_Sock, (SOCKADDR*)&clientaddr, &addrlen);
				//printf("out[TCP ����] Ŭ���̾�Ʈ ���� : IP  �ּ� = %s, ��Ʈ��ȣ = %d\r\n",
				//	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				this->SesionRelease(_pSession);
			}
			continue;
		}

		//recv �Ϸ����� ��ƾ
		if ((&_pSession->_RecvOver) == pOverlapped)//RECV�۾�����
		{
			//PRO_BEGIN(L"_pSession->_RecvOver");
			//InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
			InterlockedIncrement64((LONG64*)&_RecvCount);

			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//������ŭ ���������� �̵�

			//CSerealBuffer* CPacket = _PACKET_POOL.Alloc();//����ȭ ���� ��������
			//CSerealBuffer* CPacket = WORKER_PACKET_POOL.Alloc();//����ȭ ���� ��������
			CSerealBuffer* CPacket = _PACKET_POOL_ARR[currentThreadIdx].Alloc();

			while (true) //���� ��Ŷ��� ����� �����ۿ� �ִ´�.
			{
				//AcquireSRWLockExclusive(&_pSession->_LOCK);
				CPacket->Clear();
				//CPacket.Clear();
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

				int retSwitchDeq = _pSession->_RecvBuf.Peek(CPacket->GetBufferPtr(), st_recv_header._Len);

				if (retSwitchDeq != st_recv_header._Len)
				{
					//��������
				}
				_pSession->_RecvBuf.MoveRead(retSwitchDeq);

				int retCS = CPacket->MoveWritePos(retSwitchDeq);//�����͸� ������ŭ ����ȭ ���۸� �̵���Ų��.

				//8����Ʈ ������ OnMessage ȣ��
				this->OnRecv(_pSession->_Id, CPacket);//������ۿ� �����ͽױ�
			}
			CPacket->Clear();
			_PACKET_POOL_ARR[currentThreadIdx].Free(CPacket);

			//WSASEND();
			this->SendPost(_pSession);

			//WSARECV();
			this->RecvPost(_pSession);
			//PRO_END(L"_pSession->_RecvOver");
		}
		//send�Ϸ�����
		if ((&_pSession->_SendOver) == pOverlapped)
		{
			//PRO_BEGIN(L"_pSession->_SendOver");
			//printf("Send THread%d\n,", thisThreadID);
			InterlockedIncrement64((LONG64*)&_SendCount);
			_pSession->_SendBuf.MoveRead(cbTransferred);//��������ŭ �б���� �̵�
			//_pSession->_SendBuf.MoveReadPtr(cbTransferred);//��������ŭ �б���� �̵�
			InterlockedExchange(&_pSession->_IO_Flag, false);//�Ϸ������� �Դٴ°� �ϳ��� ������ �����ٴ� ���̹Ƿ� �����÷��׸� false�� �����
			if (_pSession->_SendBuf.GetUseSize() > 0)//�������� ���� �����Ͱ� �����ִٸ�?
				this->SendPost(_pSession);//�׸�ŭ �ٽ� ����
			//PRO_END(L"_pSession->_SendOver");

			//���ݽ����� Recv�� ��� IOCOUNT 1�̵ǰ�
			//Send�� ���⼭ 0�� �׳ɵǹ����� ������ �������� �ʴ´�.
			//���������� ������ 0�̶�� �ٷ� ������ ����������Ѵ� ������ GetUseSize�� 0�̾������̰�
			//���� ���� ó���� �ߴٰ� �ϴ��� ������ �Ϸ��������� ���� �������¿������̱� ������
			//���⼭ 1�� ��Ҵµ� 0�̶�°��� ������ ����Ǿ��� Recv���� IOCOUNT�� 1���� �������
			//IOCOUNT�� 0�̵Ǹ鼭 �Ϸ��������� ���� �ð��� ���� ������ ����ɼ��� �����Ƿ�
			//������ ������ ������ѹ������Ѵ�.
			if (InterlockedDecrement(&_pSession->_IO_COUNT) == 0)
				this->SesionRelease(_pSession);
		}

		this->OnWorkerThreadEnd();//1��������
	}
	printf("WORKER STOP!\n");
	return 0;
}

unsigned int CLanServer::CLanLogicThread(void* pComPort)
{
	printf("Logic START!\n");
	int retVal;
	HANDLE hcp = (HANDLE)pComPort;//IOCP��Ʈ����
	WCHAR buf[256];
	DWORD64 value;
	DWORD cbTransferred;
	ULONG_PTR CompletionKey;
	LPOVERLAPPED pOverlapped;

	//WCHAR error_msg[100];
	//DWORD thisThreadID = GetCurrentThreadId();
	//DWORD currentThreadIdx = (DWORD)TlsGetValue(_PACKET_POOL_Index);

	while(1)
	{
		break;
	}

	printf("Logic END!\n");
	return 0;
}

void CLanServer::init_socket()
{
	WSADATA wsaData;
	WCHAR error_msg[100];

	//���� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup");

	//CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//���� ����
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);

	//rst�� time_out�� �����ϱ����� ����
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(listen_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	if (listen_socket == INVALID_SOCKET)
	{
		ZeroMemory(error_msg, sizeof(error_msg));
		wcscpy(error_msg, L"listen_socket == INVALID_SOCKET");
		this->OnError(1001, error_msg);
		printf("socket");
	}

	bool enable = true;
	setsockopt(listen_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));


	//������۸� 0���θ�������� �ȸ��������
	//��뷮 ������ �����ϴ°� �ƴ��̻� ���� ���ʿ䰡���� �������Ѵٰ� �ϴµ� �������� ����
	if (SNDBUF_ZERO_OnOff)
	{
		int optval = 0;
		int optlen = sizeof(optval);
		setsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optlen));
	}

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

	if (bind(listen_socket, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		ZeroMemory(error_msg, sizeof(error_msg));
		wcscpy(error_msg, L"bind(listen_socket");
		this->OnError(1001, error_msg);
		printf("Bind");
	}

	if (listen(listen_socket, BACK_LOG_SIZE) == SOCKET_ERROR)
	{
		ZeroMemory(error_msg, sizeof(error_msg));
		wcscpy(error_msg, L"listen(listen_socket, BACK_LOG_SIZE) == SOCKET_ERROR");
		this->OnError(1001, error_msg);
		printf("listen");
	}

	//����� �Ϸ� ��Ʈ ����
	this->IOCP_WORKER_POOL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, Thread_Running);

	if (IOCP_WORKER_POOL == NULL) return;

	for (int i = 0; i < IOCP_Thread_Count; i++)
	{
		_CreateThread[i] = (HANDLE)_beginthreadex(nullptr, 0, _NET_WorkerThread, this, 0, nullptr);
	}

	_AcceptThread = (HANDLE)_beginthreadex(nullptr, 0, _NET_AcceptThread, this, 0, nullptr);

}

void CLanServer::RecvPost(st_SESSION* _pSession)
{
	DWORD flags = 0;
	WSABUF wsaBuf[2];
	WCHAR error_msg[100];

	ZeroMemory(&_pSession->_RecvOver, sizeof(OVERLAPPED));
	wsaBuf[0].buf = _pSession->_RecvBuf.GetWriteBufferPtr();
	wsaBuf[0].len = _pSession->_RecvBuf.DirectEnqueueSize();
	wsaBuf[1].buf = _pSession->_RecvBuf.GetStartBufferPtr();
	wsaBuf[1].len = _pSession->_RecvBuf.GetFreeSize() - wsaBuf[0].len;

	//if (wsaBuf[1].len > _pSession->_RecvBuf.GetBufferSize())
	//	wsaBuf[1].len = 0;

	int retVal = WSARecv(_pSession->_Sock, wsaBuf, 2, nullptr, &flags, &_pSession->_RecvOver, nullptr);
	if (retVal == SOCKET_ERROR)
	{
		int retWsa = WSAGetLastError();
		if (retWsa == WSA_IO_PENDING)//���ø����̼ǿ��� ��� �Ϸ��� �� ���� ������ �۾��� �����߽��ϴ�. �۾��� �Ϸ�Ǹ� ���߿� �Ϸ� ǥ�ð� �����˴ϴ�.
		{
			return;
		}
		else if (retWsa != WSA_IO_PENDING) 
		{
			if (retWsa != WSAECONNRESET//���� ������ ���� ȣ��Ʈ�� ���� ������ ������ϴ�.
				&& retWsa != WSAECONNABORTED//����Ʈ����� ���� ������ �ߴܵǾ����ϴ�.
				&& retWsa != WSANOTINITIALISED//������ WSAStartup�� ���� ������� �ʾҽ��ϴ�.
				&& retWsa != WSAEWOULDBLOCK//�� ������ ��� �Ϸ��� �� ���� ����ŷ ������ �۾����� ��ȯ�˴ϴ�
				&& retWsa != WSAEFAULT)//�ּҰ� �߸��Ǿ����ϴ�. ���ø����̼��� �߸��� ������ ���� �����ϰų� ������ ���̰� �ʹ� ���� ��쿡 �߻�
			{
				printf("RecvPost WSAERROR %d\n", retWsa);
			}
			int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			if (ret_IO == 0)
			{
				this->SesionRelease(_pSession);
			}
			return;
		}
	}

};

void CLanServer::SendPost(st_SESSION* _pSession)
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
	WCHAR error_msg[100];

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
		if (retWsa == WSA_IO_PENDING)//���ø����̼ǿ��� ��� �Ϸ��� �� ���� ������ �۾��� �����߽��ϴ�. �۾��� �Ϸ�Ǹ� ���߿� �Ϸ� ǥ�ð� �����˴ϴ�.
		{
			return;
		}
		else if (retWsa != WSA_IO_PENDING)
		{
			if (retWsa != WSAECONNRESET//���� ������ ���� ȣ��Ʈ�� ���� ������ ������ϴ�.
				&& retWsa != WSAECONNABORTED//����Ʈ����� ���� ������ �ߴܵǾ����ϴ�.
				&& retWsa != WSANOTINITIALISED//������ WSAStartup�� ���� ������� �ʾҽ��ϴ�.
				&& retWsa != WSAEWOULDBLOCK//�� ������ ��� �Ϸ��� �� ���� ����ŷ ������ �۾����� ��ȯ�˴ϴ�
				&& retWsa != WSAEFAULT)//�ּҰ� �߸��Ǿ����ϴ�. ���ø����̼��� �߸��� ������ ���� �����ϰų� ������ ���̰� �ʹ� ���� ��쿡 �߻�
			{
				printf("SendPost WSAERROR %d\n", retWsa);
			}

			int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			//printf("Send  _pSession->Id() %d\n", _pSession->_Id);
			//printf("Send  _pSession->_IO_COUNT() %d\n", _pSession->_IO_COUNT);
			//printf("Send WSAGetLastError() %d\n", retWsa);
			//ZeroMemory(error_msg, sizeof(error_msg));
			//wcscpy(error_msg, L"Send WSAGetLastError()");
			//this->OnError(1001, error_msg);
			if (ret_IO == 0)
			{
				this->SesionRelease(_pSession);
			}
		}
	}
};


bool CLanServer::SesionRelease(st_SESSION* _pSession)
{
	DWORD64 Session_id = _pSession->_Id;

	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);
	auto sMap = SESSION_MAP.find(Session_id);
	if (sMap != SESSION_MAP.end())//���Ǹʿ� �����Ѵٸ�
	{
		SESSION_MAP.erase(sMap);//���Ǹʿ��� �����ϰ�
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//���Ƕ��� ����
		AcquireSRWLockExclusive(&_pSession->_LOCK);//����ȭ���������� ������ü�� �������
		closesocket(_pSession->_Sock);
		SessionReset(_pSession);//�����ʱ�ȭ
		ReleaseSRWLockExclusive(&_pSession->_LOCK);
		AcquireSRWLockExclusive(&this->SESSION_POOL_LOCK);
		_SESSION_POOL.Free(_pSession);
		ReleaseSRWLockExclusive(&this->SESSION_POOL_LOCK);

		this->OnClientLeave(Session_id);
		return true;
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
		return false;//��Ҹ� �߰����� ���ߴ�.
	}
};

bool CLanServer::SessionReset(st_SESSION* _pSession)
{
	ZeroMemory(&_pSession->_SendOver, sizeof(_pSession->_SendOver));
	ZeroMemory(&_pSession->_RecvOver, sizeof(_pSession->_RecvOver));
	_pSession->_Id = 0;
	//_pSession->_Sock_Addr = clientaddr;
	_pSession->_SendBuf.ClearBuffer();
	_pSession->_RecvBuf.ClearBuffer();
	_pSession->_IO_COUNT = 0;
	_pSession->_IO_Flag = false;
	return true;
};
bool CLanServer::PacketReset(CSerealBuffer* CPacket)
{

	return true;
};


bool CLanServer::Socket_Error()
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

bool CLanServer::SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket)
{
	DWORD currentThreadIdx = (DWORD)TlsGetValue(_PACKET_POOL_Index);

	if (currentThreadIdx == 0)
	{
		//ó���϶�
		currentThreadIdx = InterlockedIncrement(&_PACKET_POOL_Count);
		//_P_ARR_Th[currentThreadIdx].Thread_Id = GetCurrentThreadId();
		_PACKET_POOL_ARR[currentThreadIdx].SetPool(10, false);
		if (!TlsSetValue(_PACKET_POOL_Index, (LPVOID)currentThreadIdx))
		{
			int* ptr = nullptr;
			*ptr = 100;
			return false;
		}
	}

	int CPacketSize = CPacket->GetUseSize();//�Ѿ�� ��Ŷ�� ����� Ȯ���Ѵ�

	//LAN WAN�����ؼ� �������
	LanServerHeader st_header;
	st_header._Len = CPacketSize;//��Ŷ�� �ִ����

	CSerealBuffer* SendPacket = _PACKET_POOL_ARR[currentThreadIdx].Alloc();//�������� �����͸������Ѵ�
	//CSerealBuffer* SendPacket = _PACKET_POOL.Alloc();//�������� �����͸������Ѵ�

	SendPacket->Clear();
	SendPacket->PutData((char*)&st_header, sizeof(LanServerHeader));//����� �ְ�
	SendPacket->PutData(CPacket->GetBufferPtr(), CPacketSize);//��Ŷ���� �����͸� �ٷ� �̾Ƽ� �ִ´�.

	//CSerealBuffer SendPacket;//�������� �����͸������Ѵ�
	//SendPacket.Clear();
	//SendPacket.PutData((char*)&st_header, sizeof(LanServerHeader));//����� �ְ�
	//SendPacket.PutData(CPacket->GetBufferPtr(), CPacketSize);//��Ŷ���� �����͸� �ٷ� �̾Ƽ� �ִ´�.

	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);//ã�⸸�ϸ�Ǵϱ� �������
	auto sMap = SESSION_MAP.find(_pSESSIONID);//�ʿ��� ���Ǿ��̵����� �˻�
	if (sMap != SESSION_MAP.end())//�˻��� ���� �����Ѵٸ�
	{
		st_SESSION* _pSend_Session = nullptr;
		_pSend_Session = sMap->second;
		AcquireSRWLockExclusive(&_pSend_Session->_LOCK);//�˻��� ���� �����Ѵٴ� ���̹Ƿ� �����Ǵ�
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//���Ǹ��� ���� �����Ѵ�.
		_pSend_Session->_SendBuf.Enqueue((char*)SendPacket->GetBufferPtr(), SendPacket->GetUseSize());//���Ͽ� ������ۼ����� ���۰� �ֱ�
		ReleaseSRWLockExclusive(&_pSend_Session->_LOCK);//���� �����Ѵ�.
		SendPacket->Clear();
		_PACKET_POOL_ARR[currentThreadIdx].Free(SendPacket);
		return true;
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//���Ǹ��� ���� �����Ѵ�.
		SendPacket->Clear();
		_PACKET_POOL_ARR[currentThreadIdx].Free(SendPacket);
		return false;
	}
};



void CLanServer::All_Moniter()
{
	DWORD64 _R_Cur_Time = timeGetTime();
	if (1000 < (_R_Cur_Time - _Cur_Time))//1�ʰ� �����ٸ� 
	{
		_Cur_Time = _R_Cur_Time;
		//system("cls");
		this->getAcceptTPS();
		this->getRecvMessageTPS();
		this->getSendMessageTPS();
		printf("_ALL_AcceptCount : %lld\n", _ALL_AcceptCount);
		printf("_ALL_RecvCount : %lld\n", _ALL_RecvCount);
		printf("_ALL_SendCount : %lld\n", _ALL_SendCount);

	}
	//system("cls");
	//g_TotalSyncPacketSend += g_SyncPacketPerSec;
	//printf("-----------------------------------------\n");
	//printf("EXECUTE_TIME : %u sec\n", now / 1000);
	//printf("CUR - SEC : %u\n", cur - sec);
	//printf("%d_UPDATE_CALLED \n", update);
	//if (serverLock) printf("SERVER_LOCKED\n");
	//else printf("SERVER_UNLOCKED\n");
	//printf("NET_IO_PER_SEC : %d\n", g_NetworkIOLoop);
	//printf("RECV_PER_SEC : %d\n", g_RecvPerSec);
	//printf("SEND_PER_SEC : %d\n", g_SendPerSec);
	//printf("SYNC_PACKET_SEND_PER_SEC : %d\n", g_SyncPacketPerSec);
	//printf("TOTAL_SYNC_PACKET_SEND : %llu\n", g_TotalSyncPacketSend);
	//printf("SYNC_MOVE_START : %d\n", g_SyncMoveStart);
	//printf("SYNC_MOVE_STOP : %d\n", g_SyncMoveStop);
	//printf("SYNC_ATTACK1 : %d\n", g_SyncAttack1);
	//printf("SYNC_ATTACK2 : %d\n", g_SyncAttack2);
	//printf("SYNC_ATTACK3 : %d\n", g_SyncAttack3);

	//printf("\nMEMORY OBJECT POOL\n");
	//printf("PLAYER_POOL_CAPACITY : %d\n", g_PlayerObjectPool.GetCapacityCount());
	//printf("PLAYER_POOL_USE_COUNT : %d\n", g_PlayerObjectPool.GetUseCount());
	//printf("SESSION_POOL_CAPACITY : %d\n", g_SessionObjectPool.GetCapacityCount());
	//printf("SESSION_POOL_USE_COUNT : %d\n", g_SessionObjectPool.GetUseCount());
	//printf("PACKET_POOL_CAPACITY : %d\n", g_PacketObjectPool.GetCapacityCount());
	//printf("PACKET_POOL_USE_COUNT : %d\n", g_PacketObjectPool.GetUseCount());
	//g_SendPerSec = g_RecvPerSec = g_SyncPacketPerSec = g_NetworkIOLoop = 0;
	//printf("-----------------------------------------\n\n");

};



bool CLanServer::Disconnect(DWORD64 SessionID)
{
	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);
	auto sMap = SESSION_MAP.find(SessionID);
	if (sMap != SESSION_MAP.end())//���Ǹʿ� �����Ѵٸ�
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//���Ƕ��� ����
		return SesionRelease(sMap->second);
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
		return false;//��Ҹ� �߰����� ���ߴ�.
	}
}

int CLanServer::getAcceptTPS()
{
	//DWORD64 _R_Cur_Time = timeGetTime();
	//if (1000 < (_R_Cur_Time - _Cur_Time))//1�ʰ� �����ٸ� 
	//{
	DWORD64 _A_AcceptCount = _AcceptCount;
	printf("Accept Count 1 sec :  %lld\n", _A_AcceptCount);
	_InlineInterlockedAdd64((LONG64*)&_ALL_AcceptCount, _A_AcceptCount);
	InterlockedExchange64((LONG64*)&_AcceptCount, 0);
	return _A_AcceptCount;
	//}
	//else
	//	return _SendCount;
}

int CLanServer::getRecvMessageTPS()
{
	//DWORD64 _R_Cur_Time = timeGetTime();
	//if (1000 < (_R_Cur_Time - _Cur_Time))//1�ʰ� �����ٸ� 
	//{
	//	_Cur_Time = _R_Cur_Time;
	DWORD64 _R_RecvCount = _RecvCount;
	printf("Recv Count 1 sec : %lld\n", _R_RecvCount);
	_InlineInterlockedAdd64((LONG64*)&_ALL_RecvCount, _R_RecvCount);
	InterlockedExchange64((LONG64*)&_RecvCount, 0);
	return _R_RecvCount;
	//}
	//else
	//	return _RecvCount;
};

int CLanServer::getSendMessageTPS()
{
	//DWORD64 _R_Cur_Time = timeGetTime();
	//if (1000 < (_R_Cur_Time - _Cur_Time))//1�ʰ� �����ٸ� 
	//{
	//	_Cur_Time = _R_Cur_Time;
	DWORD64 _S_SendCount = _SendCount;
	printf("Send Count 1 sec : %lld\n", _S_SendCount);
	_InlineInterlockedAdd64((LONG64*)&_ALL_SendCount, _S_SendCount);
	InterlockedExchange64((LONG64*)&_SendCount, 0);
	return _S_SendCount;
	//}
	//else
	//	return _SendCount;
};