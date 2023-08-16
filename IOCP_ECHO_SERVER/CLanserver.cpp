#include "CLanServer.h"

//unsigned int AcceptThread(void* param)
//{
//	////AcceptThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
//	//CLanServer* core = (CLanServer*)param;
//	//WaitForSingleObject(core->_RunningEvent, INFINITE);
//	//return core->CLanServerAcceptThread(param);
//	return 0;
//}
//
//unsigned int WorkerThread(void* param)
//{
//	////AcceptThread -> NetCore::Run 이 호출될때까지 막혀있다가 NetCore::Run이 호출되어야 시작
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
	this->MAX_SESSION_COUNT = CMax_Session_Count;//최대 유저 숫자
	//this->init_Set();//게임기본세팅
	this->init_socket();//소켓세팅
	//printf("_SESSSION_POOL.GetCapacityCount() %d\n", _SESSSION_POOL.GetCapacityCount());
	_SESSION_POOL.SetPool(MAX_SESSION_COUNT / 2, false);
	_PACKET_POOL.SetPool(1000,false);
	//printf("_SESSSION_POOL.GetCapacityCount() %d\n", _SESSSION_POOL.GetCapacityCount());
	//_SESSSION_POOL = &CMemoryPool<st_SESSION>(MAX_SESSION_COUNT, false);
	//_SessionPool(CMax_Session_Count,false);

	//스레드 생성처리
	
	//오브젝트풀 생성

	//스레드 모두 시작

	return true;
};

void  CLanServer::Stop()
{
	//데이터 DB 저장

	//우선 listensocket 닫고
	
	//Accpet스레드 정지

	//워커스레드정리

	//모니터링 스레드정리

	//메인스레드정지
};

unsigned int CLanServer::CLanAcceptThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//데이터 통신에 사용할 변수
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

		if (g_Shutdown)//플래그가 끊겼다면 자동종료
			break;

		printf("[TCP서버] 클라이언트 접속 IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//세션생성
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

		//비동기 입출력 시작
		DWORD flags = 0;

		AcquireSRWLockExclusive(&this->SESSION_MAP_LOCK);
		this->SESSION_MAP.emplace(_Session_User->_Id, _Session_User);
		ReleaseSRWLockExclusive(&this->SESSION_MAP_LOCK);

		CreateIoCompletionPort((HANDLE)_Session_User->_Sock, this->IOCP_WORKER_POOL, (ULONG_PTR)_Session_User, 0);

		this->RecvPost(_Session_User);
		InterlockedIncrement64((LONG64*)&_AcceptCount);
		//this->OnClientJoin();Accept후 접속완료호출
	}
	printf("AcceptThread STOP!\n");
	return 0;
}



unsigned int CLanServer::CLanWorkerThread(void* pComPort)
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

	while (true)
	{
		ZeroMemory(&cbTransferred, sizeof(DWORD));
		ZeroMemory(&CompletionKey, sizeof(ULONG_PTR));
		ZeroMemory(&pOverlapped, sizeof(LPOVERLAPPED));

		cbTransferred = 0;
		CompletionKey = 0;

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
			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
			{
				SOCKADDR_IN clientaddr;
				int addrlen = sizeof(clientaddr);
				getpeername(_pSession->_Sock, (SOCKADDR*)&clientaddr, &addrlen);
				printf("out[TCP 서버] 클라이언트 종료 : IP  주소 = %s, 포트번호 = %d\r\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				this->SesionRelease(_pSession);
			}
			continue;
		}

		//recv 완료통지 루틴
		if ((&_pSession->_RecvOver) == pOverlapped)//RECV작업진행
		{
			//InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
			InterlockedIncrement64((LONG64*)&_RecvCount);
			
			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//받은만큼 쓰기포인터 이동
			AcquireSRWLockExclusive(&PACKET_LOCK);
			CSerealBuffer *CPacket = _PACKET_POOL.Alloc();//직렬화 버퍼 가져오기
			ReleaseSRWLockExclusive(&PACKET_LOCK);

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

				if (retSwitchDeq!= st_recv_header._Len)
				{
					//강제종료
				}
				_pSession->_RecvBuf.MoveRead(retSwitchDeq);

				int retCS = CPacket->MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.
				
				//8바이트 데이터 OnMessage 호출
				this->OnRecv(_pSession->_Id, CPacket);//센드버퍼에 데이터쌓기
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
		//send완료통지
		if ((&_pSession->_SendOver) == pOverlapped)
		{
			//printf("Send THread%d\n,", thisThreadID);
			InterlockedIncrement64((LONG64*)&_SendCount);
			_pSession->_SendBuf.MoveRead(cbTransferred);//보낸값만큼 읽기버퍼 이동
			//_pSession->_SendBuf.MoveReadPtr(cbTransferred);//보낸값만큼 읽기버퍼 이동
			InterlockedDecrement(&_pSession->_IO_COUNT);//완료통지가 됬다면 통신이 종료됬다는뜻
			InterlockedExchange(&_pSession->_IO_Flag, false);//완료통지가 왔다는건 하나의 전송이 끝났다는 뜻이므로 전송플래그를 false로 만든다
			if (_pSession->_SendBuf.GetUseSize() > 0)//보내지지 않은 데이터가 남아있다면?
				this->SendPost(_pSession);//그만큼 다시 전송
		}
		this->OnWorkerThreadEnd();//1루프종료
	}
	printf("WORKER STOP!\n");
	return 0;
}

void CLanServer::init_socket()
{
	WSADATA wsaData;

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

	int option = Nagle_Onoff;               //네이글 알고리즘 on/off
	setsockopt(listen_socket,             //해당 소켓
		IPPROTO_TCP,          //소켓의 레벨
		TCP_NODELAY,          //설정 옵션
		(const char*)&option, // 옵션 포인터
		sizeof(option));      //옵션 크기

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
	if (sMap != SESSION_MAP.end())//세션맵에 존재한다면
	{
		SESSION_MAP.erase(sMap);//세션맵에서 제거하고
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션락을 해제
		AcquireSRWLockExclusive(&_pSession->_LOCK);//동기화문제를위해 세션자체의 록을잡고
		SessionReset(_pSession);//세션초기화
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
		return false;//요소를 발견하지 못했다.
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
	if (WSAError == WSAECONNRESET//현재 연결은 원격 호스트에 의해 강제로 끊겼습니다.
		|| WSAError == WSAECONNABORTED//소프트웨어로 인해 연결이 중단되었습니다.
		|| WSAError == WSANOTINITIALISED//성공한 WSAStartup이 아직 수행되지 않았습니다.
		|| WSAError == WSAEWOULDBLOCK//이 오류는 즉시 완료할 수 없는 비블로킹 소켓의 작업에서 반환됩니다
		|| WSAError == WSAEFAULT)//주소가 잘못되었습니다. 애플리케이션이 잘못된 포인터 값을 전달하거나 버퍼의 길이가 너무 작은 경우에 발생

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
	int CPacketSize = CPacket->GetUseSize();//넘어온 패킷의 사이즈를 확인한다

	//LAN WAN구별해서 헤더세팅
	LanServerHeader st_header;
	st_header._Len = CPacketSize;//패킷의 최대길이
	AcquireSRWLockExclusive(&PACKET_LOCK);
	CSerealBuffer *SendPacket = _PACKET_POOL.Alloc();//보낼곳에 데이터를세팅한다
	ReleaseSRWLockExclusive(&PACKET_LOCK);
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
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션맵의 락을 제거한다.
	}
	SendPacket->Clear();
	AcquireSRWLockExclusive(&PACKET_LOCK);
	_PACKET_POOL.Free(SendPacket);
	ReleaseSRWLockExclusive(&PACKET_LOCK);
};



void CLanServer::All_Moniter()
{
	DWORD64 _R_Cur_Time = timeGetTime();
	if (1000 < (_R_Cur_Time - _Cur_Time))//1초가 지났다면 
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
	if (sMap != SESSION_MAP.end())//세션맵에 존재한다면
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션락을 해제
		SesionRelease(sMap->second);
		return true;
	}
	else
	{
		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
		return false;//요소를 발견하지 못했다.
	}
}

int CLanServer::getAcceptTPS()
{
	//DWORD64 _R_Cur_Time = timeGetTime();
	//if (1000 < (_R_Cur_Time - _Cur_Time))//1초가 지났다면 
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
	//if (1000 < (_R_Cur_Time - _Cur_Time))//1초가 지났다면 
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
	//if (1000 < (_R_Cur_Time - _Cur_Time))//1초가 지났다면 
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