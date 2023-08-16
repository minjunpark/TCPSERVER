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
	//this->init_Set();//게임기본세팅
	this->init_socket();//소켓세팅
	InitializeSRWLock(&Player_Session->_LOCK);

	//스레드 생성처리

	//오브젝트풀 생성

	//스레드 모두 시작

	return true;
};

void  CLanClient::Stop()
{
	//데이터 DB 저장

	//Accept스레드 정리

	//워커스레드정리

	//모니터링 스레드정리


};

unsigned int CLanClient::CLanClientAcceptThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//데이터 통신에 사용할 변수
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
		serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 자기 자신에게 보낸다
		serveraddr.sin_port = htons(3500);

		int client_len = sizeof(serveraddr);

		if (connect(con_socket, (struct sockaddr*)&serveraddr, client_len) == -1) {
			perror("connect error : ");
			exit(0);
		}
		
		if (g_Shutdown)//플래그가 끊겼다면 자동종료
			break;

		printf("[TCP서버] 클라이언트 접속 IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//세션생성
		Player_Session = new SESSION;
		if (Player_Session == NULL)
			break;

		ZeroMemory(&Player_Session->_SendOver, sizeof(Player_Session->_SendOver));
		ZeroMemory(&Player_Session->_RecvOver, sizeof(Player_Session->_RecvOver));

		//_Session_User->_Id = UNI_KEY++;
		Player_Session->_Sock = con_socket;
		Player_Session->_Sock_Addr = serveraddr;///서버주소
		Player_Session->_SendBuf.ClearBuffer();
		Player_Session->_RecvBuf.ClearBuffer();
		Player_Session->_IO_COUNT = 1;
		Player_Session->_IO_Flag = false;
		InitializeSRWLock(&Player_Session->_LOCK);

		int bufSize = 0;
		int optionLen = 4;

		//setsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
		//getsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, &optionLen);

		//비동기 입출력 시작
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

		SESSION* _pSession = (SESSION*)CompletionKey;
		//printf("스레드별 처리량 cbTransferred %d\n", cbTransferred);
		//printf("SESSION IOCOUNT=0생기는부분 있나 확인%d\n", _pSession->_IO_COUNT);

		if (g_Shutdown) break;//스레드 정지
		if (_pSession == nullptr)
			continue;//세션이 없을경우 스레드만 다시 실행 결함이라고 판단하고 종료하는 방법도 있음
		if (CompletionKey == NULL)
			break;//키가 없다면 정지
		if (pOverlapped == nullptr)
			continue;


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
				//this->SesionRelease(_pSession);
				Disconnect();
			}
			continue;
		}

		//recv 완료통지 루틴
		if ((&_pSession->_RecvOver) == pOverlapped)//RECV작업진행
		{
			//InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
			InterlockedIncrement64((LONG64*)&_RecvCount);

			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//받은만큼 쓰기포인터 이동

			CSerealBuffer CPacket;//직렬화 버퍼 가져오기

			ZeroMemory(buf, 256);
			ZeroMemory(&value, sizeof(value));

			while (true) //받은 패킷모두 센드용 링버퍼에 넣는다.
			{
				//AcquireSRWLockExclusive(&_pSession->_LOCK);
				CPacket.Clear();
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

				int retSwitchDeq = _pSession->_RecvBuf.Peek(CPacket.GetBufferPtr(), st_recv_header._Len);

				if (retSwitchDeq != st_recv_header._Len)
				{
					//강제종료
				}
				_pSession->_RecvBuf.MoveRead(retSwitchDeq);

				int retCS = CPacket.MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

				//8바이트 데이터 OnMessage 호출
				this->OnRecv(_pSession->_Id, &CPacket);//센드버퍼에 데이터쌓기
			}

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
	}
	printf("WORKER STOP!\n");
	return 0;
}

void CLanClient::init_socket()
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

void CLanClient::SendPacket(CSerealBuffer* CPacket)
{
	int CPacketSize = CPacket->GetUseSize();//넘어온 패킷의 사이즈를 확인한다

	//LAN WAN구별해서 헤더세팅
	LanServerHeader st_header;
	st_header._Len = CPacketSize;//패킷의 최대길이

	CSerealBuffer SendPacket;//보낼곳에 데이터를세팅한다
	SendPacket.PutData((char*)&st_header, sizeof(LanServerHeader));//헤더를 넣고
	SendPacket.PutData(CPacket->GetBufferPtr(), CPacketSize);//패킷에서 데이터를 바로 뽑아서 넣는다.
	
	AcquireSRWLockExclusive(&Player_Session->_LOCK);//검색한 값이 존재한다는 뜻이므로 락을건다
	Player_Session->_SendBuf.Enqueue((char*)SendPacket.GetBufferPtr(), SendPacket.GetUseSize());//리턴용 센드버퍼세팅후 버퍼값 넣기
	ReleaseSRWLockExclusive(&Player_Session->_LOCK);//락을 제거한다.
};

bool CLanClient::Disconnect()
{
	//closesocket(Player_Session->_Id);
	//delete Player_Session;
}
