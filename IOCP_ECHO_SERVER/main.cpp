#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
#include <cstdio>
#include<Winsock2.h>
#include<stdlib.h>
#include <conio.h>
#include "CRingBuffer.h"
#include <unordered_map>
#define PORT	9000
#define BUFSIZE 512

using namespace std;

//소켓 정보 저장을 위한 구조체와 변수
struct SESSION
{
	SOCKET _Sock;
	SOCKADDR_IN _Sock_Addr;
	CRingBuffer _SendBuf;
	CRingBuffer _RecvBuf;
	OVERLAPPED _SendOver;
	OVERLAPPED _RecvOver;
	BOOL _Live;
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

unordered_map <SOCKET, SESSION> g_SESSION_LIST;

//오류 출력 함수
void init_socket();
void err_quit(const char* msg);
BOOL Socket_Error();

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
		_Session_User->_Sock = clientsock;
		_Session_User->_Sock_Addr = clientaddr;
		_Session_User->_Live = true;
		_Session_User->_SendBuf.ClearBuffer();
		_Session_User->_RecvBuf.ClearBuffer();
		_Session_User->_IO_COUNT = 0;

		int bufSize = 0;
		int optionLen = 4;

		//setsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
		getsockopt(clientsock, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, &optionLen);
		printf("송신 버퍼 크기 : %d\n", bufSize);
		//비동기 입출력 시작
		DWORD flags = 0;

		WSABUF wsaBuf[2];
		wsaBuf[0].buf = _Session_User->_RecvBuf.GetFrontBufferPtr();//읽기포인터
		wsaBuf[0].len = _Session_User->_RecvBuf.GetFreeSize();//뽑을수있는값
		//wsaBuf[1].buf = _Session_User->_RecvBuf.GetStartBufferPtr();//최초 지점
		//wsaBuf[1].len = _Session_User->_RecvBuf.GetUseSize() - wsaBuf[1].len;//모든 뽑을수있는 바이트에서 한번에뽑을수 있는숫자만큼빼면 원하는만큼뽑을수있음.

		//포트에 값을 등록하고 _Session_User세팅
		//CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		ULONG_PTR COMKEY = (ULONG_PTR)_Session_User;
		CreateIoCompletionPort((HANDLE)_Session_User->_Sock, IOCP_WORKER_POOL, COMKEY, 0);
		int retVal = WSARecv(clientsock, wsaBuf, 1, &recvbytes, &flags, &_Session_User->_RecvOver, NULL);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING) {
				printf("WSAGetLastError() %d\n", WSAGetLastError());
			}
			continue;
		}
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

		retVal = GetQueuedCompletionStatus(IOCP_WORKER_POOL, &cbTransferred, &CompletionKey, &pOverlapped, INFINITE);
		SESSION* _pSESSION = (SESSION*)CompletionKey;

		if (g_Shutdown) break;//스레드 정지
		if (_pSESSION == nullptr)continue;//세션이 없을경우 스레드만 다시 실행 결함이라고 판단하고 종료하는 방법도 있음
		if (CompletionKey == NULL)break;//키가 없다면 정지

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
			closesocket(_pSESSION->_Sock);
			printf("[TCP 서버] 클라이언트 종료 : IP  주소 = %s, 포트번호 = %d\r\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			delete _pSESSION;
			continue;
		}

		//recv 완료통지 루틴
		if ((&_pSESSION->_RecvOver) == pOverlapped)//RECV작업진행
		{
			ZeroMemory(buf, 256);
			ZeroMemory(&_pSESSION->_SendOver, sizeof(_pSESSION->_SendOver));
			ZeroMemory(&_pSESSION->_RecvOver, sizeof(_pSESSION->_RecvOver));

			_pSESSION->_RecvBuf.MoveRear(cbTransferred);//받은만큼 읽기포인터이동
			_pSESSION->_RecvBuf.Dequeue((char*)buf, cbTransferred);//받은만큼 버퍼에 데이터 뽑아내기

			buf[cbTransferred + 2] = '\0';//널포인터값세팅
			printf("[TCP /%s : %d] ", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));//ip출력
			wprintf(L"%s \n", buf);//버퍼 출력

			_pSESSION->_SendBuf.Enqueue((char*)buf, cbTransferred);//리턴용 센드버퍼세팅후 버퍼값 넣기

			WSABUF wsaBufSend[2];//버퍼길이세팅 2개로 해놨는데 어차피 데이터 크지도않아서 그냥 이대로보내기로함
			wsaBufSend[0].buf = _pSESSION->_SendBuf.GetFrontBufferPtr();//읽기포인터
			wsaBufSend[0].len = _pSESSION->_SendBuf.GetUseSize();//뽑을수있는값

			//WSASEND();
			DWORD sendbytes;//데이터 전송

			retVal = WSASend(_pSESSION->_Sock, wsaBufSend, 1, &sendbytes, 0, &_pSESSION->_SendOver, NULL);
			if (retVal == SOCKET_ERROR) {
				if (!Socket_Error())
				{
					
				}
				continue;
			}
			_pSESSION->_SendBuf.MoveFront(sendbytes);//전송한만큼 센드버퍼이동시킨다.

			_pSESSION->_RecvBuf.ClearBuffer();//리시브버퍼 클리어

			//WSARECV();
			WSABUF wsaBufRecv[2];
			wsaBufRecv[0].buf = _pSESSION->_RecvBuf.GetFrontBufferPtr();//읽기포인터
			wsaBufRecv[0].len = _pSESSION->_RecvBuf.GetFreeSize();//뽑을수있는값

			DWORD recvbytes;
			DWORD flags = 0;


			int retVal = WSARecv(_pSESSION->_Sock, wsaBufRecv, 1, &recvbytes, &flags, &_pSESSION->_RecvOver, NULL);//리시브 재세팅
			if (retVal == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					printf("WSAGetLastError() %d\n", WSAGetLastError());
				}
				continue;
			}
		}
		//send완료통지
		if ((&_pSESSION->_SendOver) == pOverlapped)
		{
			_pSESSION->_SendBuf.ClearBuffer();//데이터가 모두 보내졌다는뜻이므로 버퍼를 정리한다.
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
	//LocalFree(lpMsgBuf);
	//exit(1);
}

BOOL Socket_Error() 
{
	if (WSAGetLastError() == WSA_IO_PENDING) {
		err_quit("WSA_IO_PENDING()");
		return true;
	}
	if (WSAGetLastError() != WSA_IO_PENDING) {
		err_quit("WSASend()");
		return false;
	}
}