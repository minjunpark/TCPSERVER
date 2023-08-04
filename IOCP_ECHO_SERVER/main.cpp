#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
#include <cstdio>
#include <iostream>
#include<stdlib.h>
#include <conio.h>
#include <unordered_map>
#include <Winsock2.h>
#include <cstdlib>
#include <cstdio>
#include <process.h>
#include <windows.h>

#include "CRingBuffer.h"
#include "CSerealBuffer.h"

#define PORT	9000
#define BUFSIZE 512

using namespace std;

//소켓 정보 저장을 위한 구조체와 변수
struct SESSION
{
	SOCKET _Sock;
	DWORD _Id;
	SOCKADDR_IN _Sock_Addr;
	CRingBuffer _SendBuf;
	CRingBuffer _RecvBuf;
	OVERLAPPED _SendOver;
	OVERLAPPED _RecvOver;
	BOOL _IO_Live;
	DWORD _IO_COUNT;
};

HANDLE _CreateThread[30];
DWORD Thread_Count;
SOCKET listen_socket;

//작업자 스레드 함수
//DWORD WINAPI AcceptThread(LPVOID arg);
unsigned __stdcall AcceptThread(void* CompletionPortIO);
unsigned __stdcall WorkerThread(void* pComPort);

HANDLE IOCP_WORKER_POOL;
BOOL g_Shutdown = false;

unordered_map <DWORD, SESSION*> SESSION_MAP;
DWORD UNI_KEY = 0;
//오류 출력 함수
void init_socket();
void err_quit(const char* msg);
BOOL Socket_Error();
void RecvPost(SESSION* _pSession);
void SendPost(SESSION* _pSession);
BOOL SesionRelease(SESSION* _pSession);


int main()
{
	printf("MainThread START!\n");
	init_socket();//소켓관련 설정 초기화

	while (1)
	{
		int key = _getch();
		if (key == 's')
		{
			printf("process Stop START\n");
			closesocket(listen_socket);
			g_Shutdown = true;
			for (int i = 0; i < Thread_Count; i++)
				PostQueuedCompletionStatus(IOCP_WORKER_POOL, 0, 0, 0);
			break;
			//특정 키카 눌리면 SaveThread 를 깨운다.
		}
		Sleep(0);
	}


	WaitForMultipleObjects(Thread_Count - 1, _CreateThread, true, INFINITE);//모든 스레드가 정지될때까지 무한대기하면서 확인
	printf("process Stop END\n");
	//윈속 종료
	WSACleanup();
	printf("MainThread STOP!\n");
	return 0;
}

unsigned __stdcall AcceptThread(void* pComPort)
{
	printf("AcceptThread START!\n");
	//데이터 통신에 사용할 변수
	SOCKET clientsock;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	DWORD recvbytes;

	while (1) {
		//accept()
		clientsock = accept(listen_socket, (SOCKADDR*)&clientaddr, &addrlen);
		if (clientsock == INVALID_SOCKET) {
			err_quit("accept()");
			break;
		}

		if (g_Shutdown)//플래그가 끊겼다면 자동종료
			break;

		printf("[TCP서버] 클라이언트 접속 IP : %s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		//세션생성
		SESSION* _Session_User = new SESSION;
		if (_Session_User == NULL)
			break;

		ZeroMemory(&_Session_User->_SendOver, sizeof(_Session_User->_SendOver));
		ZeroMemory(&_Session_User->_RecvOver, sizeof(_Session_User->_RecvOver));
		_Session_User->_Id = UNI_KEY++;
		_Session_User->_Sock = clientsock;
		_Session_User->_Sock_Addr = clientaddr;
		_Session_User->_IO_Live = true;
		_Session_User->_SendBuf.ClearBuffer();
		_Session_User->_RecvBuf.ClearBuffer();
		_Session_User->_IO_COUNT = 0;

		int bufSize = 0;
		int optionLen = 4;

		//setsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
		getsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, &optionLen);

		//비동기 입출력 시작
		DWORD flags = 0;

		SESSION_MAP.emplace(_Session_User->_Id, _Session_User);

		WSABUF wsaBuf[2];
		wsaBuf[0].buf = _Session_User->_RecvBuf.GetFrontBufferPtr();//읽기포인터
		wsaBuf[0].len = _Session_User->_RecvBuf.GetFreeSize();//뽑을수있는값
		//wsaBuf[1].buf = _Session_User->_RecvBuf.GetStartBufferPtr();//최초 지점
		//wsaBuf[1].len = _Session_User->_RecvBuf.GetUseSize() - wsaBuf[1].len;//모든 뽑을수있는 바이트에서 한번에뽑을수 있는숫자만큼빼면 원하는만큼뽑을수있음.

		//포트에 값을 등록하고 _Session_User세팅
		//CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		ULONG_PTR COMKEY = (ULONG_PTR)_Session_User;

		CreateIoCompletionPort((HANDLE)_Session_User->_Sock, IOCP_WORKER_POOL, COMKEY, 0);

		RecvPost(_Session_User);
		//int retVal = WSARecv(_Session_User->_Sock, wsaBuf, 1, &recvbytes, &flags, &_Session_User->_RecvOver, NULL);
		//if (retVal == SOCKET_ERROR)
		//{
		//	if (WSAGetLastError() != WSA_IO_PENDING) {
		//		printf("WSAGetLastError() %d\n", WSAGetLastError());
		//	}
		//	continue;
		//}
	}
	printf("AcceptThread STOP!\n");
	return 0;
}


unsigned __stdcall WorkerThread(void* pComPort)
{
	printf("WorkerThread START!\n");
	int retVal;
	HANDLE hcp = (HANDLE)pComPort;//IOCP포트구분
	WCHAR buf[256];
	while (1)
	{
		DWORD cbTransferred;
		ULONG_PTR CompletionKey;
		LPOVERLAPPED pOverlapped;

		ZeroMemory(&cbTransferred, sizeof(DWORD));
		ZeroMemory(&CompletionKey, sizeof(ULONG_PTR));
		ZeroMemory(&pOverlapped, sizeof(LPOVERLAPPED));

		retVal = GetQueuedCompletionStatus(IOCP_WORKER_POOL, &cbTransferred, &CompletionKey, &pOverlapped, INFINITE);

		SESSION* _pSESSION = (SESSION*)CompletionKey;

		if (g_Shutdown) break;//스레드 정지
		if (_pSESSION == nullptr) continue;//세션이 없을경우 스레드만 다시 실행 결함이라고 판단하고 종료하는 방법도 있음
		if (CompletionKey == NULL) break;//키가 없다면 정지
		
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(_pSESSION->_Sock, (SOCKADDR*)&clientaddr, &addrlen);

		//비동기 입출력 결과 확인
		if (retVal == 0 || cbTransferred == 0)
		{
			if (retVal == 0) {//이게있으면 강제종료 되버리는데
				DWORD temp1, temp2;
				WSAGetOverlappedResult(_pSESSION->_Sock, &_pSESSION->_RecvOver, &temp1, FALSE, &temp2);
				//err_quit("WSAGetOverlappedResult()");
			}
			int ret_IO = InterlockedDecrement((LONG*)&_pSESSION->_IO_COUNT);
			if (ret_IO <= 0)
			{
				printf("[TCP 서버] 클라이언트 종료 : IP  주소 = %s, 포트번호 = %d\r\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				SesionRelease(_pSESSION);
			}
			continue;
		}

		//recv 완료통지 루틴
		if ((&_pSESSION->_RecvOver) == pOverlapped)//RECV작업진행
		{
			ZeroMemory(buf, 256);
			ZeroMemory(&_pSESSION->_SendOver, sizeof(_pSESSION->_SendOver));
			ZeroMemory(&_pSESSION->_RecvOver, sizeof(_pSESSION->_RecvOver));

			_pSESSION->_RecvBuf.MoveRear(cbTransferred);//받은만큼 쓰기포인터 이동
			CSerealBuffer CPacket;//직렬화 버퍼 가져오기
			CPacket.Clear();

			int Req = _pSESSION->_RecvBuf.Dequeue((char*)CPacket.GetBufferPtr(), cbTransferred);//직렬화버퍼에 뽑아내기
			CPacket.MoveWritePos(Req);//바로 뽑아낸만큼 데이터 이동
			CPacket.GetData((char*)buf, cbTransferred);//버퍼에 받은 바이트만큼 데이터복사

			buf[cbTransferred + 2] = '\0';//출력을위해 널포인터 세팅
			printf("[TCP /%s : %d] ", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));//ip출력
			wprintf(L"%s \n", buf);//버퍼 출력

			CPacket.Clear();//재사용을위해 직렬화버퍼 초기화
			CPacket.PutData((char*)buf, cbTransferred);//직렬화 버퍼에 데이터를 넣고

			_pSESSION->_SendBuf.Enqueue((char*)CPacket.GetBufferPtr(), CPacket.GetUseSize());//리턴용 센드버퍼세팅후 버퍼값 넣기

			InterlockedDecrement((LONG*)&_pSESSION->_IO_COUNT);

			//WSASEND();
			SendPost(_pSESSION);

			//WSARECV();
			RecvPost(_pSESSION);
		}
		//send완료통지
		if ((&_pSESSION->_SendOver) == pOverlapped)
		{
			_pSESSION->_SendBuf.MoveFront(cbTransferred);//보낸값만큼 버퍼 이동하기 이동하기
			InterlockedDecrement((LONG*)&_pSESSION->_IO_COUNT);//완료통지가 됬다면 통신이 종료됬다는뜻
		}
	}
	printf("WORKER STOP!\n");
	return 0;
}

void init_socket()
{
	WSADATA wsaData;

	//윈속 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		err_quit("WSAStartup");

	//입출력 완료 포트 생성
	IOCP_WORKER_POOL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (IOCP_WORKER_POOL == NULL) return;

	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
	Thread_Count = (int)si.dwNumberOfProcessors / 2;

	for (int i = 0; i < Thread_Count; i++)
	{
		_CreateThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, 0, NULL);
	}

	//소켓 생성
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == INVALID_SOCKET)
		err_quit("socket");
	bool enable = true;
	setsockopt(listen_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(listen_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	SOCKADDR_IN sock_addr;
	ZeroMemory(&sock_addr, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(PORT);
	sock_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(listen_socket, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) err_quit("Bind");
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) err_quit("listen");

	_CreateThread[Thread_Count] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, NULL, 0, NULL);
}

void err_quit(const char* msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
}

void RecvPost(SESSION* _pSession)
{
	if (_pSession == nullptr) return;
	DWORD recvbytes;
	DWORD flags = 0;
	WSABUF wsaBuf[2];

	int DirEn = _pSession->_RecvBuf.DirectEnqueueSize();//한번에 넣을수 있는 최대크기
	int StartEn = _pSession->_RecvBuf.GetFreeSize() - DirEn;

	int bufcount = 1;
	if (StartEn > 0)
		bufcount++;
	
	wsaBuf[0].buf = _pSession->_RecvBuf.GetRearBufferPtr();//쓰기포인터
	wsaBuf[0].len = DirEn;//넣을수 있는값
	wsaBuf[1].buf = _pSession->_RecvBuf.GetStartBufferPtr();//최초 지점
	wsaBuf[1].len = StartEn;//모든 뽑을수있는 바이트에서 한번에뽑을수 있는숫자만큼빼면 원하는만큼뽑을수있음.
	
	int retVal = WSARecv(_pSession->_Sock, wsaBuf, bufcount, &recvbytes, &flags, &_pSession->_RecvOver, NULL);
	if (retVal == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSA_IO_PENDING) 
		{
			InterlockedIncrement((LONG*)&_pSession->_IO_COUNT);//펜딩걸렸다면 통신중
			return;
		}
		else if (WSAGetLastError() != WSA_IO_PENDING) {
			printf("WSAGetLastError() %d\n", WSAGetLastError());
			SesionRelease(_pSession);
			return;
		}
	}
	InterlockedIncrement((LONG*)&_pSession->_IO_COUNT);//바로 리턴된경우에도 통신중이라고 볼수 있으니까 증가시키는게 맞잖아 아닌가?
};

void SendPost(SESSION* _pSession)
{
	if (_pSession == nullptr) return;
	DWORD recvbytes;
	DWORD flags = 0;
	WSABUF wsaBuf[2];

	int DirDe = _pSession->_SendBuf.DirectDequeueSize();//한번에 넣을수 있는 최대크기
	int StartDe = _pSession->_SendBuf.GetUseSize() - DirDe;

	int bufcount = 1;
	if (StartDe > 0)
		bufcount++;

	wsaBuf[0].buf = _pSession->_SendBuf.GetFrontBufferPtr();//쓰기포인터
	wsaBuf[0].len = DirDe;//한번에뽑을수있는값
	wsaBuf[1].buf = _pSession->_SendBuf.GetStartBufferPtr();//최초 지점
	wsaBuf[1].len = StartDe;//모든 뽑을수있는 바이트에서 한번에뽑을수 있는숫자만큼빼면 원하는만큼뽑을수있음.

	//WSASEND();
	DWORD sendbytes;//데이터 전송
	int retVal = WSASend(_pSession->_Sock, wsaBuf, bufcount, &sendbytes, 0, &_pSession->_SendOver, NULL);
	if (retVal == SOCKET_ERROR) {
		if (WSAGetLastError() == WSA_IO_PENDING)
		{
			InterlockedIncrement((LONG*)&_pSession->_IO_COUNT);//펜딩걸렸다면 통신중
			return;
		}
		else if(WSAGetLastError() != WSA_IO_PENDING) {
			printf("WSAGetLastError() %d\n", WSAGetLastError());
			SesionRelease(_pSession);
			return;
		}
	}
	InterlockedIncrement((LONG*)&_pSession->_IO_COUNT);//펜딩걸렸다면 통신중
};


BOOL SesionRelease(SESSION* _pSession)
{
	auto sMap = SESSION_MAP.find(_pSession->_Id);
	
	if (sMap  != SESSION_MAP.end()) 
	{
		SESSION_MAP.erase(sMap);//세션맵에서 제거하고
		closesocket(_pSession->_Sock);
		delete _pSession;
		return true;
	}
	else
		return false;//요소를 발견하지 못했다.
};


BOOL Socket_Error()
{
	int WSAError = WSAGetLastError();
	if (WSAError == WSA_IO_PENDING) {
		err_quit("WSA_IO_PENDING()");
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
		err_quit("WSASend()");
		return false;
	}
}