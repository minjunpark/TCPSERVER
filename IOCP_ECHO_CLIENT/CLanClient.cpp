#include "CLanClient.h"

//CDump CLanDump();

unsigned __stdcall _NET_ConncetThread(void* param)
{
	CLanClient* server = (CLanClient*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanConncetThread(param);
}

unsigned __stdcall _NET_WorkerThread(void* param)
{
	CLanClient* server = (CLanClient*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanWorkerThread(param);
}
//---------------------------------------------------------------------------------------------------------------------------------

CLanClient::CLanClient()// : _SESSION_POOL(0, false), _PACKET_POOL(0, false)
{
	//CMemoryPool<st_SESSION*> _SESSSION_POOL(10000, false);
	//_SESSSION_POOL(10000, FALSE);
};

CLanClient::~CLanClient()
{
	//��������� ���ʿ� stop�� �Ǿ��Ѵ�.
	//������ ����Ǹ� ���α��� ���ư��µ�
	//�޸������� ���ʿ䰡 ���� �ٸ����� �Ű澲�°� ���Űǰ��� ����.
};

bool  CLanClient::Connect(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, bool CNagle_Onoff)
{
	timeBeginPeriod(1);
	InitProfile();//�������� ����� ���� �ʱ�ȭ
	InitializeSRWLock(&SESSION_MAP_LOCK);

	this->SNDBUF_ZERO_OnOff = false;//�۽Ź��� ũ�⸦ ���ηθ������ �ȸ������
	this->PORT = CPORT;
	this->_Start_Time = _Cur_Time = timeGetTime();
	this->IOCP_Thread_Count = CThraed_Count;
	this->Nagle_Onoff = CNagle_Onoff;

	_Con_Server_addr.sin_family = AF_INET;
	_Con_Server_addr.sin_port = this->PORT;

	//inet_pton(AF_INET, CIP, &_Con_Server_addr.sin_addr);
	inet_pton(AF_INET, "127.0.0.1", &_Con_Server_addr.sin_addr);

	_PACKET_POOL_Index = TlsAlloc();
	_PACKET_POOL_Count = 0;
	
	this->init_socket();//���ϼ���

	while (1)
	{

		int key = _getch();
		if (key == 'w')
		{
			ProfileDataOutText(L"ABC");//�������� ����������
		}
		else if (key == 's')//��������
		{
			Sleep(1000);
			break;
		}
		Sleep(0);
	}

	//������ ����ó��

	//������ƮǮ ����

	//������ ��� ����

	return true;
};

unsigned int CLanClient::CLanConncetThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//������ ��ſ� ����� ����
	SOCKET clientsock;
	SOCKADDR_IN serveraddr;
	//int addrlen = sizeof(serveraddr);
	DWORD recvbytes;
	WCHAR msg[100];



	while (true) {
		//accept()
		clientsock = connect(this->con_socket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		
		st_SESSION* _Session_User= new st_SESSION;

		if (_Session_User == NULL)
			break;

		ZeroMemory(&_Session_User->_SendOver, sizeof(_Session_User->_SendOver));
		ZeroMemory(&_Session_User->_RecvOver, sizeof(_Session_User->_RecvOver));
		_Session_User->_Id = UNI_KEY++;
		_Session_User->_Sock = clientsock;
		//_Session_User->_Sock_Addr = clientaddr;
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
		this->OnEnterJoinServer();//Accept�� ���ӿϷ�ȣ��
	}
	printf("AcceptThread STOP!\n");
	return 0;
}



unsigned int CLanClient::CLanWorkerThread(void* pComPort)
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
		_PACKET_POOL_ARR[currentThreadIdx].SetPool(10, false);
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
			if (InterlockedDecrement(&_pSession->_IO_COUNT) == 0)
				this->SesionRelease(_pSession);
		}

		this->OnWorkerThreadEnd();//1��������
	}
	printf("WORKER STOP!\n");
	return 0;
}

void CLanClient::init_socket()
{
	WSADATA wsaData;
	WCHAR error_msg[100];

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
	con_socket = socket(AF_INET, SOCK_STREAM, 0);

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	if (con_socket == INVALID_SOCKET)
	{
		ZeroMemory(error_msg, sizeof(error_msg));
		wcscpy(error_msg, L"listen_socket == INVALID_SOCKET");
		this->OnError(1001, error_msg);
		printf("socket");
	}

	bool enable = true;
	setsockopt(con_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));

	if (SNDBUF_ZERO_OnOff) {
		int optval = 0;
		int optlen = sizeof(optval);
		setsockopt(con_socket, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optlen));
	}

	int option = Nagle_Onoff;               //���̱� �˰��� on/off
	setsockopt(con_socket,             //�ش� ����
		IPPROTO_TCP,          //������ ����
		TCP_NODELAY,          //���� �ɼ�
		(const char*)&option, // �ɼ� ������
		sizeof(option));      //�ɼ� ũ��

	SOCKADDR_IN sock_addr;
	ZeroMemory(&sock_addr, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(PORT);
	sock_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(con_socket, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		ZeroMemory(error_msg, sizeof(error_msg));
		wcscpy(error_msg, L"bind(listen_socket");
		this->OnError(1001, error_msg);
		printf("Bind");
	}
	if (listen(con_socket, BACK_LOG_SIZE) == SOCKET_ERROR)
	{
		ZeroMemory(error_msg, sizeof(error_msg));
		wcscpy(error_msg, L"listen(listen_socket, BACK_LOG_SIZE) == SOCKET_ERROR");
		this->OnError(1001, error_msg);
		printf("listen");
	}

	for (int i = 0; i < IOCP_Thread_Count; i++)
	{
		_CreateThread[i] = (HANDLE)_beginthreadex(nullptr, 0, _NET_WorkerThread, this, 0, nullptr);
	}

	//_AcceptThread = (HANDLE)_beginthreadex(nullptr, 0, _NET_AcceptThread, this, 0, nullptr);

}

void CLanClient::RecvPost(st_SESSION* _pSession)
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
		if (retWsa == WSA_IO_PENDING)
		{
			return;
		}
		else if (retWsa != WSA_IO_PENDING) {
			int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			//printf("Recv  _pSession->Id() %d\n", _pSession->_Id);
			//printf("Recv  _pSession->_IO_COUNT() %d\n", _pSession->_IO_COUNT);
			//printf("Recv WSAGetLastError() %d\n", retWsa);
			//ZeroMemory(error_msg, sizeof(error_msg));
			//wcscpy(error_msg, L"Recv WSAGetLastError()");
			//this->OnError(1001, error_msg);
			if (ret_IO == 0)
			{
				this->SesionRelease(_pSession);
			}
			return;
		}
	}

};

void CLanClient::SendPost(st_SESSION* _pSession)
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
		if (retWsa == WSA_IO_PENDING)
		{
			return;
		}
		else if (retWsa != WSA_IO_PENDING)
		{
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


bool CLanClient::SesionRelease(st_SESSION* _pSession)
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
		ReleaseSRWLockExclusive(&_pSession->_LOCK);
		delete _pSession;
		this->OnLeaveServer();
		return true;
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
		return false;//��Ҹ� �߰����� ���ߴ�.
	}
};

bool CLanClient::SessionReset(st_SESSION* _pSession)
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
bool CLanClient::PacketReset(CSerealBuffer* CPacket)
{

	return true;
};

bool CLanClient::SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket)
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

bool CLanClient::Disconnect(DWORD64 SessionID)
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
