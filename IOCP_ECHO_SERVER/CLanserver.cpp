#include "CLanServer.h"

//unsigned int AcceptThread(void* param)
//{
//	////AcceptThread -> NetCore::Run �� ȣ��ɶ����� �����ִٰ� NetCore::Run�� ȣ��Ǿ�� ����
//	//CLanServer* core = (CLanServer*)param;
//	//WaitForSingleObject(core->_RunningEvent, INFINITE);
//	//return core->CLanServerAcceptThread(param);
//	return 0;
//}
//
//unsigned int WorkerThread(void* param)
//{
//	////AcceptThread -> NetCore::Run �� ȣ��ɶ����� �����ִٰ� NetCore::Run�� ȣ��Ǿ�� ����
//	//CLanServer* core = (CLanServer*)param;
//	//WaitForSingleObject(core->_RunningEvent, INFINITE);
//	//return core->CLanServerAcceptThread(param);
//	return 0;
//}

unsigned __stdcall _NET_WorkerThread(void* param)
{
	CLanServer* server = (CLanServer*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanWorkerThread(param);
}
//---------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------------
//Accept Thread Wrapping function
// param CNetServer*
unsigned __stdcall _NET_AcceptThread(void* param)
{
	CLanServer* server = (CLanServer*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->CLanAcceptThread(param);
}


CLanServer::CLanServer() : _SESSION_POOL(0, false), _PACKET_POOL(0, false)
{
	//CMemoryPool<st_SESSION*> _SESSSION_POOL(10000, false);
	//_SESSSION_POOL(10000, FALSE);
};

CLanServer::~CLanServer()
{
	
};

bool  CLanServer::Start(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count)
{
	
	timeBeginPeriod(1);
	InitializeSRWLock(&SESSION_MAP_LOCK);
	InitializeSRWLock(&SESSION_POOL_LOCK);
	InitializeSRWLock(&PACKET_LOCK);
	this->PORT = CPORT;
	this->_Start_Time = _Cur_Time = timeGetTime();
	this->Thread_Count = CThraed_Count;
	this->Thread_Running = CThread_Running;
	this->Nagle_Onoff = CNagle_Onoff;
	this->MAX_SESSION_COUNT = CMax_Session_Count;//�ִ� ���� ����
	//this->init_Set();//���ӱ⺻����
	this->init_socket();//���ϼ���
	//printf("_SESSSION_POOL.GetCapacityCount() %d\n", _SESSSION_POOL.GetCapacityCount());
	_SESSION_POOL.SetPool(MAX_SESSION_COUNT / 2, false);
	_PACKET_POOL.SetPool(1000,false);
	//printf("_SESSSION_POOL.GetCapacityCount() %d\n", _SESSSION_POOL.GetCapacityCount());
	//_SESSSION_POOL = &CMemoryPool<st_SESSION>(MAX_SESSION_COUNT, false);
	//_SessionPool(CMax_Session_Count,false);

	//������ ����ó��
	
	//������ƮǮ ����

	//������ ��� ����

	return true;
};

void  CLanServer::Stop()
{
	//������ DB ����

	//�켱 listensocket �ݰ�
	
	//Accpet������ ����

	//��Ŀ����������

	//����͸� ����������

	//���ν���������
};

unsigned int CLanServer::CLanAcceptThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//������ ��ſ� ����� ����
	SOCKET clientsock;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	DWORD recvbytes;

	while (true) {
		//accept()
		clientsock = accept(this->listen_socket, (SOCKADDR*)&clientaddr, &addrlen);
		
		//this->OnConnectionRequest();

		if (clientsock == INVALID_SOCKET) {
			printf("accept()");
			break;
		}

		if (g_Shutdown)//�÷��װ� ����ٸ� �ڵ�����
			break;

		printf("[TCP����] Ŭ���̾�Ʈ ���� IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//���ǻ���
		//st_SESSION* _Session_User = new st_SESSION;
		AcquireSRWLockExclusive(&SESSION_POOL_LOCK);
		st_SESSION* _Session_User = _SESSION_POOL.Alloc();
		ReleaseSRWLockExclusive(&SESSION_POOL_LOCK);
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
		//this->OnClientJoin();Accept�� ���ӿϷ�ȣ��
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

	while (true)
	{
		ZeroMemory(&cbTransferred, sizeof(DWORD));
		ZeroMemory(&CompletionKey, sizeof(ULONG_PTR));
		ZeroMemory(&pOverlapped, sizeof(LPOVERLAPPED));

		cbTransferred = 0;
		CompletionKey = 0;

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
			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
			{
				SOCKADDR_IN clientaddr;
				int addrlen = sizeof(clientaddr);
				getpeername(_pSession->_Sock, (SOCKADDR*)&clientaddr, &addrlen);
				printf("out[TCP ����] Ŭ���̾�Ʈ ���� : IP  �ּ� = %s, ��Ʈ��ȣ = %d\r\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				this->SesionRelease(_pSession);
			}
			continue;
		}

		//recv �Ϸ����� ��ƾ
		if ((&_pSession->_RecvOver) == pOverlapped)//RECV�۾�����
		{
			//InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
			InterlockedIncrement64((LONG64*)&_RecvCount);
			
			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//������ŭ ���������� �̵�
			AcquireSRWLockExclusive(&PACKET_LOCK);
			CSerealBuffer *CPacket = _PACKET_POOL.Alloc();//����ȭ ���� ��������
			ReleaseSRWLockExclusive(&PACKET_LOCK);

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

				if (retSwitchDeq!= st_recv_header._Len)
				{
					//��������
				}
				_pSession->_RecvBuf.MoveRead(retSwitchDeq);

				int retCS = CPacket->MoveWritePos(retSwitchDeq);//�����͸� ������ŭ ����ȭ ���۸� �̵���Ų��.
				
				//8����Ʈ ������ OnMessage ȣ��
				this->OnRecv(_pSession->_Id, CPacket);//������ۿ� �����ͽױ�
			}
			CPacket->Clear();
			AcquireSRWLockExclusive(&PACKET_LOCK);
			_PACKET_POOL.Free(CPacket);
			ReleaseSRWLockExclusive(&PACKET_LOCK);

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
		this->OnWorkerThreadEnd();//1��������
	}
	printf("WORKER STOP!\n");
	return 0;
}

void CLanServer::init_socket()
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
		_CreateThread[i] = (HANDLE)_beginthreadex(nullptr, 0, _NET_WorkerThread, this, 0, nullptr);
	}

	this->_CreateThread[Thread_Count] = (HANDLE)_beginthreadex(nullptr, 0, _NET_AcceptThread, this, 0, nullptr);

}

void CLanServer::RecvPost(st_SESSION* _pSession)
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
		SessionReset(_pSession);//�����ʱ�ȭ
		ReleaseSRWLockExclusive(&_pSession->_LOCK);

		AcquireSRWLockExclusive(&SESSION_POOL_LOCK);
		_SESSION_POOL.Free(_pSession);
		ReleaseSRWLockExclusive(&SESSION_POOL_LOCK);

		//delete _pSession;
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
	closesocket(_pSession->_Sock);
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

void CLanServer::SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket)
{
	int CPacketSize = CPacket->GetUseSize();//�Ѿ�� ��Ŷ�� ����� Ȯ���Ѵ�

	//LAN WAN�����ؼ� �������
	LanServerHeader st_header;
	st_header._Len = CPacketSize;//��Ŷ�� �ִ����
	AcquireSRWLockExclusive(&PACKET_LOCK);
	CSerealBuffer *SendPacket = _PACKET_POOL.Alloc();//�������� �����͸������Ѵ�
	ReleaseSRWLockExclusive(&PACKET_LOCK);
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
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//���Ǹ��� ���� �����Ѵ�.
	}
	SendPacket->Clear();
	AcquireSRWLockExclusive(&PACKET_LOCK);
	_PACKET_POOL.Free(SendPacket);
	ReleaseSRWLockExclusive(&PACKET_LOCK);
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
	//printf("PLAYER_POOL_CAPACITY : %d\n", univ_dev::g_PlayerObjectPool.GetCapacityCount());
	//printf("PLAYER_POOL_USE_COUNT : %d\n", univ_dev::g_PlayerObjectPool.GetUseCount());
	//printf("SESSION_POOL_CAPACITY : %d\n", univ_dev::g_SessionObjectPool.GetCapacityCount());
	//printf("SESSION_POOL_USE_COUNT : %d\n", univ_dev::g_SessionObjectPool.GetUseCount());
	//printf("PACKET_POOL_CAPACITY : %d\n", univ_dev::g_PacketObjectPool.GetCapacityCount());
	//printf("PACKET_POOL_USE_COUNT : %d\n", univ_dev::g_PacketObjectPool.GetUseCount());
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
		SesionRelease(sMap->second);
		return true;
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