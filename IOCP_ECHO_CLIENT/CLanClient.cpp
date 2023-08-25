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
	//여기오려면 애초에 stop이 되야한다.
	//어차피 종료되면 메인까지 날아가는데
	//메모리정리를 할필요가 없지 다른곳에 신경쓰는게 정신건강에 좋다.
};

bool  CLanClient::Connect(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, bool CNagle_Onoff)
{
	timeBeginPeriod(1);
	InitProfile();//프로파일 사용을 위한 초기화
	InitializeSRWLock(&SESSION_MAP_LOCK);

	this->SNDBUF_ZERO_OnOff = false;//송신버퍼 크기를 제로로만들건지 안만들건지
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
	
	this->init_socket();//소켓세팅

	while (1)
	{

		int key = _getch();
		if (key == 'w')
		{
			ProfileDataOutText(L"ABC");//프로파일 데이터저장
		}
		else if (key == 's')//서버정지
		{
			Sleep(1000);
			break;
		}
		Sleep(0);
	}

	//스레드 생성처리

	//오브젝트풀 생성

	//스레드 모두 시작

	return true;
};

unsigned int CLanClient::CLanConncetThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//데이터 통신에 사용할 변수
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

		//비동기 입출력 시작
		DWORD flags = 0;

		AcquireSRWLockExclusive(&this->SESSION_MAP_LOCK);
		this->SESSION_MAP.emplace(_Session_User->_Id, _Session_User);
		ReleaseSRWLockExclusive(&this->SESSION_MAP_LOCK);

		CreateIoCompletionPort((HANDLE)_Session_User->_Sock, this->IOCP_WORKER_POOL, (ULONG_PTR)_Session_User, 0);

		this->RecvPost(_Session_User);
		InterlockedIncrement64((LONG64*)&_AcceptCount);
		this->OnEnterJoinServer();//Accept후 접속완료호출
	}
	printf("AcceptThread STOP!\n");
	return 0;
}



unsigned int CLanClient::CLanWorkerThread(void* pComPort)
{
	printf("WorkerThread START!\n");
	int retVal;
	HANDLE hcp = (HANDLE)pComPort;//IOCP포트구분
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
		//처음일때
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
		this->OnWorkerThreadBegin();//1루프 시작

		st_SESSION* _pSession = (st_SESSION*)CompletionKey;
		//printf("스레드별 처리량 cbTransferred %d\n", cbTransferred);
		//printf("SESSION IOCOUNT=0생기는부분 있나 확인%d\n", _pSession->_IO_COUNT);

		if (g_Shutdown) break;//스레드 정지
		if (_pSession == nullptr)continue;//세션이 없을경우 스레드만 다시 실행 결함이라고 판단하고 종료하는 방법도 있음
		if (CompletionKey == NULL)break;//키가 없다면 정지
		if (pOverlapped == nullptr)continue;

		//printf("처리량확인 %d\n", cbTransferred);
		//비동기 입출력 결과 확인
		if (retVal == 0 || cbTransferred == 0)
		{
			//if (retVal == 0) {//이게있으면 강제종료 되버리는데
			//	DWORD temp1, temp2;
			//	WSAGetOverlappedResult(_pSession->_Sock, &_pSession->_RecvOver, &temp1, FALSE, &temp2);
			//	//err_quit("WSAGetOverlappedResult()");
			//}
			int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			ZeroMemory(error_msg, sizeof(error_msg));
			wcscpy(error_msg, L"retVal == 0 || cbTransferred == 0 전송량 오류 소켓강제 정지");
			this->OnError(1001, error_msg);
			if (ret_IO == 0)
			{
				SOCKADDR_IN clientaddr;
				int addrlen = sizeof(clientaddr);
				getpeername(_pSession->_Sock, (SOCKADDR*)&clientaddr, &addrlen);
				//printf("out[TCP 서버] 클라이언트 종료 : IP  주소 = %s, 포트번호 = %d\r\n",
				//	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				this->SesionRelease(_pSession);
			}
			continue;
		}

		//recv 완료통지 루틴
		if ((&_pSession->_RecvOver) == pOverlapped)//RECV작업진행
		{
			//PRO_BEGIN(L"_pSession->_RecvOver");
			//InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
			InterlockedIncrement64((LONG64*)&_RecvCount);

			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//받은만큼 쓰기포인터 이동

			//CSerealBuffer* CPacket = _PACKET_POOL.Alloc();//직렬화 버퍼 가져오기
			//CSerealBuffer* CPacket = WORKER_PACKET_POOL.Alloc();//직렬화 버퍼 가져오기
			CSerealBuffer* CPacket = _PACKET_POOL_ARR[currentThreadIdx].Alloc();

			while (true) //받은 패킷모두 센드용 링버퍼에 넣는다.
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
				int retPeek = _pSession->_RecvBuf.Peek((char*)&st_recv_header, sizeof(LanServerHeader));//헤더크기만큼 꺼내기

				if (_RetUseSize < sizeof(LanServerHeader) + st_recv_header._Len)
				{
					break;
				}

				//헤더에서 읽어온 크기만큼 버퍼에 존재한다면?
				_pSession->_RecvBuf.MoveRead(sizeof(LanServerHeader));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.

				int retSwitchDeq = _pSession->_RecvBuf.Peek(CPacket->GetBufferPtr(), st_recv_header._Len);

				if (retSwitchDeq != st_recv_header._Len)
				{
					//강제종료
				}
				_pSession->_RecvBuf.MoveRead(retSwitchDeq);

				int retCS = CPacket->MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

				//8바이트 데이터 OnMessage 호출
				this->OnRecv(_pSession->_Id, CPacket);//센드버퍼에 데이터쌓기
			}
			CPacket->Clear();
			_PACKET_POOL_ARR[currentThreadIdx].Free(CPacket);

			//WSASEND();
			this->SendPost(_pSession);

			//WSARECV();
			this->RecvPost(_pSession);
			//PRO_END(L"_pSession->_RecvOver");
		}
		//send완료통지
		if ((&_pSession->_SendOver) == pOverlapped)
		{
			//PRO_BEGIN(L"_pSession->_SendOver");
			//printf("Send THread%d\n,", thisThreadID);
			InterlockedIncrement64((LONG64*)&_SendCount);
			_pSession->_SendBuf.MoveRead(cbTransferred);//보낸값만큼 읽기버퍼 이동
			//_pSession->_SendBuf.MoveReadPtr(cbTransferred);//보낸값만큼 읽기버퍼 이동
			InterlockedExchange(&_pSession->_IO_Flag, false);//완료통지가 왔다는건 하나의 전송이 끝났다는 뜻이므로 전송플래그를 false로 만든다
			if (_pSession->_SendBuf.GetUseSize() > 0)//보내지지 않은 데이터가 남아있다면?
				this->SendPost(_pSession);//그만큼 다시 전송
			//PRO_END(L"_pSession->_SendOver");
			if (InterlockedDecrement(&_pSession->_IO_COUNT) == 0)
				this->SesionRelease(_pSession);
		}

		this->OnWorkerThreadEnd();//1루프종료
	}
	printf("WORKER STOP!\n");
	return 0;
}

void CLanClient::init_socket()
{
	WSADATA wsaData;
	WCHAR error_msg[100];

	//윈속 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		printf("WSAStartup");

	//입출력 완료 포트 생성
	this->IOCP_WORKER_POOL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, Thread_Running);
	if (IOCP_WORKER_POOL == NULL) return;

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//소켓 생성
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

	int option = Nagle_Onoff;               //네이글 알고리즘 on/off
	setsockopt(con_socket,             //해당 소켓
		IPPROTO_TCP,          //소켓의 레벨
		TCP_NODELAY,          //설정 옵션
		(const char*)&option, // 옵션 포인터
		sizeof(option));      //옵션 크기

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
	if (_pSession->_SendBuf.GetUseSize() == 0)//보낼 데이터가 없다면
	{
		InterlockedExchange(&_pSession->_IO_Flag, false);//연결을 종료하고 리턴
		return;
	}
	if ((InterlockedExchange(&_pSession->_IO_Flag, true)) == true) return;
	if (_pSession->_SendBuf.GetUseSize() == 0)//보낼 데이터가 없다면
	{
		InterlockedExchange(&_pSession->_IO_Flag, false);//연결을 종료하고 리턴
		SendPost(_pSession);//혹시모를 확인용 다시전송
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
	DWORD sendbytes;//데이터 전송
	InterlockedIncrement(&_pSession->_IO_COUNT);//바로 리턴된경우에도 통신중이라고 볼수 있으니까 증가시키는게 맞잖아 아닌가?

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
	if (sMap != SESSION_MAP.end())//세션맵에 존재한다면
	{
		SESSION_MAP.erase(sMap);//세션맵에서 제거하고
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션락을 해제

		AcquireSRWLockExclusive(&_pSession->_LOCK);//동기화문제를위해 세션자체의 록을잡고
		closesocket(_pSession->_Sock);
		ReleaseSRWLockExclusive(&_pSession->_LOCK);
		delete _pSession;
		this->OnLeaveServer();
		return true;
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
		return false;//요소를 발견하지 못했다.
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
		//처음일때
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

	int CPacketSize = CPacket->GetUseSize();//넘어온 패킷의 사이즈를 확인한다

	//LAN WAN구별해서 헤더세팅
	LanServerHeader st_header;
	st_header._Len = CPacketSize;//패킷의 최대길이

	CSerealBuffer* SendPacket = _PACKET_POOL_ARR[currentThreadIdx].Alloc();//보낼곳에 데이터를세팅한다
	//CSerealBuffer* SendPacket = _PACKET_POOL.Alloc();//보낼곳에 데이터를세팅한다

	SendPacket->Clear();
	SendPacket->PutData((char*)&st_header, sizeof(LanServerHeader));//헤더를 넣고
	SendPacket->PutData(CPacket->GetBufferPtr(), CPacketSize);//패킷에서 데이터를 바로 뽑아서 넣는다.

	//CSerealBuffer SendPacket;//보낼곳에 데이터를세팅한다
	//SendPacket.Clear();
	//SendPacket.PutData((char*)&st_header, sizeof(LanServerHeader));//헤더를 넣고
	//SendPacket.PutData(CPacket->GetBufferPtr(), CPacketSize);//패킷에서 데이터를 바로 뽑아서 넣는다.

	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);//찾기만하면되니까 상관없음
	auto sMap = SESSION_MAP.find(_pSESSIONID);//맵에서 세션아이디값으로 검색
	if (sMap != SESSION_MAP.end())//검색한 값이 존재한다면
	{
		st_SESSION* _pSend_Session = nullptr;
		_pSend_Session = sMap->second;
		AcquireSRWLockExclusive(&_pSend_Session->_LOCK);//검색한 값이 존재한다는 뜻이므로 락을건다
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션맵의 락을 제거한다.
		_pSend_Session->_SendBuf.Enqueue((char*)SendPacket->GetBufferPtr(), SendPacket->GetUseSize());//리턴용 센드버퍼세팅후 버퍼값 넣기
		ReleaseSRWLockExclusive(&_pSend_Session->_LOCK);//락을 제거한다.
		SendPacket->Clear();
		_PACKET_POOL_ARR[currentThreadIdx].Free(SendPacket);
		return true;
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션맵의 락을 제거한다.
		SendPacket->Clear();
		_PACKET_POOL_ARR[currentThreadIdx].Free(SendPacket);
		return false;
	}

};

bool CLanClient::Disconnect(DWORD64 SessionID)
{
	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);
	auto sMap = SESSION_MAP.find(SessionID);
	if (sMap != SESSION_MAP.end())//세션맵에 존재한다면
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션락을 해제
		return SesionRelease(sMap->second);
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
		return false;//요소를 발견하지 못했다.
	}
}
