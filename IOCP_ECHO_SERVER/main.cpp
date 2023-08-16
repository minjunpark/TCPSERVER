//#pragma warning(disable:4996)
//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib,"winmm.lib")
//#include <cstdio>
//#include <iostream>
//#include <stdlib.h>
//#include <conio.h>
//#include <unordered_map>
//#include <Winsock2.h>
//#include <cstdlib>
//#include <cstdio>
//#include <process.h>
//#include <windows.h>
//#include "CRingBuffer.h"
////#include "CSRingBuffer.h"
//#include "CSerealBuffer.h"
//
//#define PORT	6000
//#define BUFSIZE 512
//
//using namespace std;
//
////소켓 정보 저장을 위한 구조체와 변수
//struct SESSION
//{
//	SOCKET _Sock;
//	DWORD _Id;
//	SOCKADDR_IN _Sock_Addr;
//	CRingBuffer _SendBuf;
//	CRingBuffer _RecvBuf;
//	OVERLAPPED _SendOver;
//	OVERLAPPED _RecvOver;
//	alignas(64)
//	SRWLOCK _LOCK;
//	alignas(64)
//	DWORD _IO_Flag;
//	alignas(64)
//	DWORD _IO_COUNT;
//};
//
//struct st_header
//{
//	short _len;
//};
//
//HANDLE _CreateThread[30];
//DWORD Thread_Count;
//SOCKET listen_socket;
//
////작업자 스레드 함수
////DWORD WINAPI AcceptThread(LPVOID arg);
//unsigned __stdcall Main_AcceptThread(void* CompletionPortIO);
//unsigned __stdcall Main_WorkerThread(void* pComPort);
//
//HANDLE IOCP_WORKER_POOL;
//HANDLE IOCP_ECHO_POOL;
//BOOL g_Shutdown = false;
//
//unordered_map <DWORD, SESSION*> SESSION_MAP;
//SRWLOCK SESSION_MAP_LOCK;
//
//DWORD UNI_KEY = 0;
////오류 출력 함수
//void init_socket();
//void err_quit(const char* msg);
//BOOL Socket_Error();
//void RecvPost(SESSION* _pSession);
//void SendPost(SESSION* _pSession);
//BOOL SesionRelease(SESSION* _pSession);
//
//void _SendMessage(DWORD _pSESSIONID, CSerealBuffer* CPacket);
//void _EchoSendMessage(DWORD _pSESSIONID);
//void _Disconnect(DWORD _pSESSIONID, CSerealBuffer* CPacket);
//void OnMessage(DWORD _pSESSIONID, CSerealBuffer* CPacket);
//
//char* Send_Start;
//char* Recv_Start;
//
//ULONGLONG _Start_Time;//서버시작시간
//ULONGLONG _Cur_Time;//현재시간
//ULONGLONG _Old_Time;//현재시간
//
////DWORD AcceptCount;
//
//ULONGLONG _RecvCount;
//ULONGLONG _SendCount;
//ULONGLONG ALL_BYTE=0;
//
//int getRecvMessageTPS()
//{
//	ULONGLONG _R_Cur_Time = timeGetTime();
//	if (1000 < (_R_Cur_Time - _Cur_Time))//1초가 지났다면 
//	{
//		_Cur_Time = _R_Cur_Time;
//		ULONGLONG _R_RecvCount = _RecvCount;
//		printf("getRecvMessageTPS %d\n", getRecvMessageTPS());
//		InterlockedExchange64((LONG64*)&_RecvCount,0);
//		return _R_RecvCount;
//	}
//	else 
//		return _RecvCount;
//};
//
//int main()
//{
//	timeBeginPeriod(1);
//	_Cur_Time = timeGetTime();
//	printf("MainThread START!\n");
//	init_socket();//소켓관련 설정 초기화
//	InitializeSRWLock(&SESSION_MAP_LOCK);
//
//	while (1)
//	{
//		getRecvMessageTPS();
//		
//		//int key = _getch();
//		//if (key == 's')
//		//{
//		//	printf("process Stop START\n");
//		//	closesocket(listen_socket);
//		//	g_Shutdown = true;
//		//	for (int i = 0; i < Thread_Count; i++)
//		//		PostQueuedCompletionStatus(IOCP_WORKER_POOL, 0, 0, 0);
//		//	break;
//		//	//특정 키카 눌리면 SaveThread 를 깨운다.
//		//}
//		Sleep(0);
//	}
//
//	WaitForMultipleObjects(Thread_Count - 1, _CreateThread, true, INFINITE);//모든 스레드가 정지될때까지 무한대기하면서 확인
//	printf("process Stop END\n");
//	//윈속 종료
//	WSACleanup();
//	printf("MainThread STOP!\n");
//
//	return 0;
//}
//
//unsigned __stdcall Main_AcceptThread(void* pComPort)
//{
//	printf("AcceptThread START!\n");
//	//데이터 통신에 사용할 변수
//	SOCKET clientsock;
//	SOCKADDR_IN clientaddr;
//	int addrlen = sizeof(clientaddr);
//	DWORD recvbytes;
//
//	while (true) {
//		//accept()
//		clientsock = accept(listen_socket, (SOCKADDR*)&clientaddr, &addrlen);
//		if (clientsock == INVALID_SOCKET) {
//			err_quit("accept()");
//			break;
//		}
//
//		if (g_Shutdown)//플래그가 끊겼다면 자동종료
//			break;
//
//		printf("[TCP서버] 클라이언트 접속 IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
//
//		//세션생성
//		SESSION* _Session_User = new SESSION;
//		if (_Session_User == NULL)
//			break;
//
//		ZeroMemory(&_Session_User->_SendOver, sizeof(_Session_User->_SendOver));
//		ZeroMemory(&_Session_User->_RecvOver, sizeof(_Session_User->_RecvOver));
//		_Session_User->_Id = UNI_KEY++;
//		_Session_User->_Sock = clientsock;
//		_Session_User->_Sock_Addr = clientaddr;
//		_Session_User->_SendBuf.ClearBuffer();
//		_Session_User->_RecvBuf.ClearBuffer();
//		_Session_User->_IO_COUNT = 1;
//		_Session_User->_IO_Flag = false;
//		InitializeSRWLock(&_Session_User->_LOCK);
//
//		int bufSize = 0;
//		int optionLen = 4;
//
//		//setsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
//		//getsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, &optionLen);
//
//		//비동기 입출력 시작
//		DWORD flags = 0;
//
//		AcquireSRWLockExclusive(&SESSION_MAP_LOCK);
//		SESSION_MAP.emplace(_Session_User->_Id, _Session_User);
//		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
//
//		CreateIoCompletionPort((HANDLE)_Session_User->_Sock, IOCP_WORKER_POOL, (ULONG_PTR)_Session_User, 0);
//
//		RecvPost(_Session_User);
//	}
//	printf("AcceptThread STOP!\n");
//	return 0;
//}
//
//
//unsigned __stdcall Main_WorkerThread(void* pComPort)
//{
//	printf("WorkerThread START!\n");
//	int retVal;
//	HANDLE hcp = (HANDLE)pComPort;//IOCP포트구분
//	WCHAR buf[256];
//	DWORD64 value;
//	DWORD cbTransferred;
//	ULONG_PTR CompletionKey;
//	LPOVERLAPPED pOverlapped;
//
//	DWORD thisThreadID = GetCurrentThreadId();
//
//	while (true)
//	{
//		ZeroMemory(&cbTransferred, sizeof(DWORD));
//		ZeroMemory(&CompletionKey, sizeof(ULONG_PTR));
//		ZeroMemory(&pOverlapped, sizeof(LPOVERLAPPED));
//
//		cbTransferred = 0;
//		CompletionKey = 0;
//		
//		retVal = GetQueuedCompletionStatus(IOCP_WORKER_POOL, &cbTransferred, &CompletionKey, &pOverlapped, INFINITE);
//		
//		SESSION* _pSession = (SESSION*)CompletionKey;
//		//printf("스레드별 처리량 cbTransferred %d\n", cbTransferred);
//		//printf("SESSION IOCOUNT=0생기는부분 있나 확인%d\n", _pSession->_IO_COUNT);
//
//		if (g_Shutdown) break;//스레드 정지
//		if (_pSession == nullptr) 
//			continue;//세션이 없을경우 스레드만 다시 실행 결함이라고 판단하고 종료하는 방법도 있음
//		if (CompletionKey == NULL) 
//			break;//키가 없다면 정지
//		if (pOverlapped == nullptr)
//			continue;
//		
//
//		//printf("처리량확인 %d\n", cbTransferred);
//		//비동기 입출력 결과 확인
//		if (retVal == 0 || cbTransferred == 0)
//		{
//			//if (retVal == 0) {//이게있으면 강제종료 되버리는데
//			//	DWORD temp1, temp2;
//			//	WSAGetOverlappedResult(_pSession->_Sock, &_pSession->_RecvOver, &temp1, FALSE, &temp2);
//			//	//err_quit("WSAGetOverlappedResult()");
//			//}
//			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
//			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
//			{
//				SOCKADDR_IN clientaddr;
//				int addrlen = sizeof(clientaddr);
//				getpeername(_pSession->_Sock, (SOCKADDR*)&clientaddr, &addrlen);
//				printf("out[TCP 서버] 클라이언트 종료 : IP  주소 = %s, 포트번호 = %d\r\n",
//					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
//				SesionRelease(_pSession);
//			}
//			continue;
//		}
//
//		//recv 완료통지 루틴
//		if ((&_pSession->_RecvOver) == pOverlapped)//RECV작업진행
//		{
//			InterlockedAdd64((LONG64*)&ALL_BYTE, cbTransferred);
//			InterlockedIncrement64((LONG64*) &_RecvCount);
//			//AcquireSRWLockExclusive(&_pSession->_LOCK);
//			int retMRear = _pSession->_RecvBuf.MoveWrite(cbTransferred);//받은만큼 쓰기포인터 이동
//			//_pSession->_RecvBuf.MoveWritePtr(cbTransferred);//받은만큼 쓰기포인터 이동
//			CSerealBuffer CPacket;//직렬화 버퍼 가져오기
//
//			ZeroMemory(buf, 256);
//			ZeroMemory(&value, sizeof(value));
//			
//			while (true) //받은 패킷모두 센드용 링버퍼에 넣는다.
//			{
//				//AcquireSRWLockExclusive(&_pSession->_LOCK);
//				CPacket.Clear();
//				int _RetUseSize = _pSession->_RecvBuf.GetUseSize();
//				if (_RetUseSize < sizeof(st_header))
//				{
//					break;
//				}
//				st_header st_recv_header;
//				int retPeek = _pSession->_RecvBuf.Peek((char*)&st_recv_header, sizeof(st_header));//헤더크기만큼 꺼내기
//
//				if (_RetUseSize < sizeof(st_header) + st_recv_header._len)
//				{
//					break;
//				}
//				
//				//헤더에서 읽어온 크기만큼 버퍼에 존재한다면?
//				_pSession->_RecvBuf.MoveRead(sizeof(st_header));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.
//
//				int retSwitchDeq = _pSession->_RecvBuf.Dequeue(CPacket.GetBufferPtr(), st_recv_header._len);
//				int retCS = CPacket.MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.
//
//				//디버그테스트용
//				//CPacket.GetPeek((char*)&value, sizeof(value));
//				//printf("%lld\n", value);
//				//8바이트 데이터 OnMessage 호출
//				OnMessage(_pSession->_Id, &CPacket);//센드버퍼에 데이터쌓기
//			}
//			SendPost(_pSession);
//			
//			//WSARECV();
//			RecvPost(_pSession);
//		}
//		//send완료통지
//		if ((&_pSession->_SendOver) == pOverlapped)
//		{
//			//printf("Send THread%d\n,", thisThreadID);
//			_pSession->_SendBuf.MoveRead(cbTransferred);//보낸값만큼 읽기버퍼 이동
//			//_pSession->_SendBuf.MoveReadPtr(cbTransferred);//보낸값만큼 읽기버퍼 이동
//			InterlockedDecrement(&_pSession->_IO_COUNT);//완료통지가 됬다면 통신이 종료됬다는뜻
//			InterlockedExchange(&_pSession->_IO_Flag, false);//완료통지가 왔다는건 하나의 전송이 끝났다는 뜻이므로 전송플래그를 false로 만든다
//			if (_pSession->_SendBuf.GetUseSize() > 0)//보내지지 않은 데이터가 남아있다면?
//				SendPost(_pSession);//그만큼 다시 전송
//		}
//	}
//	printf("WORKER STOP!\n");
//	return 0;
//}
//
//void init_socket()
//{
//	WSADATA wsaData;
//
//	//윈속 초기화
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//		err_quit("WSAStartup");
//
//	//입출력 완료 포트 생성
//	IOCP_WORKER_POOL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
//	if (IOCP_WORKER_POOL == NULL) return;
//
//	//CPU 개수 확인
//	SYSTEM_INFO si;
//	GetSystemInfo(&si);
//
//	//for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
//	//Thread_Count = (int)si.dwNumberOfProcessors;// * 2;
//
//	
//	//소켓 생성
//	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
//
//	LINGER linger;
//	linger.l_onoff = 1;
//	linger.l_linger = 0;
//	setsockopt(listen_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
//
//	if (listen_socket == INVALID_SOCKET)
//		err_quit("socket");
//	bool enable = true;
//	setsockopt(listen_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
//	
//	int optval = 0 ;
//	int optlen = sizeof(optval);
//	setsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optlen));
//	
//	
//	SOCKADDR_IN sock_addr;
//	ZeroMemory(&sock_addr, sizeof(struct sockaddr_in));
//	sock_addr.sin_family = AF_INET;
//	sock_addr.sin_port = htons(PORT);
//	sock_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//
//	if (bind(listen_socket, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) err_quit("Bind");
//	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) err_quit("listen");
//	
//	Thread_Count = (int)si.dwNumberOfProcessors * 2;
//	//Thread_Count = 1;
//	for (int i = 0; i < Thread_Count; i++)
//	{
//		_CreateThread[i] = (HANDLE)_beginthreadex(NULL, 0, Main_WorkerThread, NULL, 0, NULL);
//	}
//
//	_CreateThread[Thread_Count] = (HANDLE)_beginthreadex(NULL, 0, Main_AcceptThread, NULL, 0, NULL);
//}
//
//void err_quit(const char* msg) {
//	LPVOID lpMsgBuf;
//	FormatMessage(
//		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
//		NULL, WSAGetLastError(),
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//		(LPTSTR)&lpMsgBuf, 0, NULL);
//	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
//}
//
//void RecvPost(SESSION* _pSession)
//{
//	DWORD flags = 0;
//	WSABUF wsaBuf[2];
//
//	ZeroMemory(&_pSession->_RecvOver, sizeof(OVERLAPPED));
//	wsaBuf[0].buf = _pSession->_RecvBuf.GetWriteBufferPtr();
//	wsaBuf[0].len = _pSession->_RecvBuf.DirectEnqueueSize();
//	wsaBuf[1].buf = _pSession->_RecvBuf.GetStartBufferPtr();
//	wsaBuf[1].len = _pSession->_RecvBuf.GetFreeSize() - wsaBuf[0].len;
//
//	//wsaBuf[0].buf = _pSession->_RecvBuf.GetWritePtr();
//	//wsaBuf[0].len = _pSession->_RecvBuf.DirectEnqueueSize();
//	//wsaBuf[1].buf = _pSession->_RecvBuf.GetBeginPtr();
//	//wsaBuf[1].len = _pSession->_RecvBuf.GetFreeSize() - _pSession->_RecvBuf.DirectEnqueueSize();
//	
//	if (wsaBuf[1].len > _pSession->_RecvBuf.GetBufferSize())
//		wsaBuf[1].len = 0;
//
//	int retVal = WSARecv(_pSession->_Sock, wsaBuf, 2, nullptr, &flags, &_pSession->_RecvOver, nullptr);
//	if (retVal == SOCKET_ERROR)
//	{
//		int retWsa = WSAGetLastError();
//		if (retWsa == WSA_IO_PENDING)
//		{
//			return;
//		}
//		else if (retWsa != WSA_IO_PENDING) {
//			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
//			printf("Recv WSAGetLastError() %d\n", retWsa);
//			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
//			{
//				//printf("Recv WSAGetLastError() %d\n", retWsa);
//				///printf("Recv SesionRelease\n", ret_IO);
//				SesionRelease(_pSession);
//			}
//			return;
//		}
//	}
//
//};
//
//void SendPost(SESSION* _pSession)
//{
//	if (_pSession->_SendBuf.GetUseSize() == 0)//보낼 데이터가 없다면
//	{
//		InterlockedExchange(&_pSession->_IO_Flag, false);//연결을 종료하고 리턴
//		return;
//	}
//	if ((InterlockedExchange(&_pSession->_IO_Flag, true)) == true) return;
//	if (_pSession->_SendBuf.GetUseSize() == 0)//보낼 데이터가 없다면
//	{
//		InterlockedExchange(&_pSession->_IO_Flag, false);//연결을 종료하고 리턴
//		SendPost(_pSession);//혹시모를 확인용 다시전송
//		return;
//	}
//	DWORD recvbytes;
//	DWORD flags = 0;
//	WSABUF wsaBuf[2];
//
//	ZeroMemory(&_pSession->_SendOver, sizeof(OVERLAPPED));
//	
//	char* Read = _pSession->_SendBuf.GetReadBufferPtr();
//	int DriUse = _pSession->_SendBuf.GetUseSize();
//	int DriDe = _pSession->_SendBuf.DirectDequeueSize();
//
//	wsaBuf[0].buf = Read;
//	wsaBuf[0].len = DriDe;
//	wsaBuf[1].buf = _pSession->_SendBuf.GetStartBufferPtr();
//	wsaBuf[1].len = DriUse - DriDe;
//
//	if (wsaBuf[1].len > 0)
//		printf("sda");
//
//	if (wsaBuf[1].len > _pSession->_SendBuf.GetBufferSize())
//		wsaBuf[1].len = 0;
//
//	//WSASEND();
//	DWORD sendbytes;//데이터 전송
//	InterlockedIncrement(&_pSession->_IO_COUNT);//바로 리턴된경우에도 통신중이라고 볼수 있으니까 증가시키는게 맞잖아 아닌가?
//	int retVal = WSASend(_pSession->_Sock, wsaBuf, 2, nullptr, 0, &_pSession->_SendOver, nullptr);
//	if (retVal == SOCKET_ERROR)
//	{
//		int retWsa = WSAGetLastError();
//		if (retWsa == WSA_IO_PENDING)
//		{
//			return;
//		}
//		else if (retWsa != WSA_IO_PENDING) 
//		{
//			//int ret_IO = InterlockedDecrement(&_pSession->_IO_COUNT);
//			printf("Send WSAGetLastError() %d\n", retWsa);
//			if ((InterlockedDecrement(&_pSession->_IO_COUNT)) == 0)
//			{
//				SesionRelease(_pSession);
//			}
//		}
//	}
//};
//
//
//BOOL SesionRelease(SESSION* _pSession)
//{
//	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);
//	auto sMap = SESSION_MAP.find(_pSession->_Id);
//	if (sMap != SESSION_MAP.end())//세션맵에 존재한다면
//	{
//		SESSION_MAP.erase(sMap);//세션맵에서 제거하고
//		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션락을 해제
//		AcquireSRWLockExclusive(&_pSession->_LOCK);//동기화문제를위해 세션자체의 록을잡고
//		ReleaseSRWLockExclusive(&_pSession->_LOCK);
//		closesocket(_pSession->_Sock);
//		delete _pSession;
//		return true;
//	}
//	else
//	{
//		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);
//		return false;//요소를 발견하지 못했다.
//	}
//};
//
//
//BOOL Socket_Error()
//{
//	int WSAError = WSAGetLastError();
//	if (WSAError == WSA_IO_PENDING) {
//		err_quit("WSA_IO_PENDING()");
//		return true;
//	}
//	if (WSAError == WSAECONNRESET//현재 연결은 원격 호스트에 의해 강제로 끊겼습니다.
//		|| WSAError == WSAECONNABORTED//소프트웨어로 인해 연결이 중단되었습니다.
//		|| WSAError == WSANOTINITIALISED//성공한 WSAStartup이 아직 수행되지 않았습니다.
//		|| WSAError == WSAEWOULDBLOCK//이 오류는 즉시 완료할 수 없는 비블로킹 소켓의 작업에서 반환됩니다
//		|| WSAError == WSAEFAULT)//주소가 잘못되었습니다. 애플리케이션이 잘못된 포인터 값을 전달하거나 버퍼의 길이가 너무 작은 경우에 발생
//
//	{
//		//return;
//	}
//
//	if (WSAError != WSA_IO_PENDING) {
//		err_quit("WSASend()");
//		return false;
//	}
//}
//
//void _SendMessage(DWORD _pSESSIONID, CSerealBuffer* CPacket)
//{
//	int CPacketSize = CPacket->GetUseSize();//넘어온 패킷의 사이즈를 확인한다
//
//	//LAN WAN구별해서 헤더세팅
//	LanServerHeader st_Lan_header;
//	st_Lan_header._Len = CPacketSize;//패킷의 최대길이
//
//	//NetServerHeader st_Net_header;
//	//st_Net_header._ByteCode = CPacketSize;//패킷의 최대길이
//	//st_Net_header._Len = CPacketSize;//패킷의 최대길이
//	//st_Net_header._RandomKey = CPacketSize;//패킷의 최대길이
//	//st_Net_header._CheckSum = CPacketSize;//패킷의 최대길이
//
//	CSerealBuffer SendPacket;//보낼곳에 데이터를세팅한다
//	SendPacket.PutData((char*)&st_Lan_header, sizeof(LanServerHeader));//헤더를 넣고
//	SendPacket.PutData(CPacket->GetBufferPtr(), CPacketSize);//패킷에서 데이터를 바로 뽑아서 넣는다.
//
//	AcquireSRWLockExclusive(&SESSION_MAP_LOCK);//찾기만하면되니까 상관없음
//	auto sMap = SESSION_MAP.find(_pSESSIONID);//맵에서 세션아이디값으로 검색
//
//	if (sMap != SESSION_MAP.end())//검색한 값이 존재한다면
//	{
//		SESSION* _pSession = nullptr;
//		_pSession = sMap->second;
//		AcquireSRWLockExclusive(&_pSession->_LOCK);//검색한 값이 존재한다는 뜻이므로 락을건다
//		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션맵의 락을 제거한다.
//		_pSession->_SendBuf.Enqueue((char*)SendPacket.GetBufferPtr(), SendPacket.GetUseSize());//리턴용 센드버퍼세팅후 버퍼값 넣기
//		ReleaseSRWLockExclusive(&_pSession->_LOCK);//락을 제거한다.
//	}
//	else
//	{
//		ReleaseSRWLockExclusive(&SESSION_MAP_LOCK);//세션맵의 락을 제거한다.
//	}
//};
//
//
//void _Disconnect(DWORD _pSESSIONID, CSerealBuffer* CPacket)
//{
//
//	return;
//};
//
//void OnMessage(DWORD _pSESSIONID, CSerealBuffer* CPacket)
//{
//	DWORD64 Echo;
//	(*CPacket) >> Echo;
//	CSerealBuffer SendPacket;
//	SendPacket << Echo;
//	_SendMessage(_pSESSIONID, &SendPacket);
//};
//
