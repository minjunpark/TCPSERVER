// SeleteModel.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#pragma comment(lib,"ws2_32")
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdlib>
#include <cstdio>
#include <clocale>

#define SERVERPORT 9000
#define BUFSIZE 512



//소켓저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	WCHAR buf[BUFSIZE + 2];
	int recvbytes;
	int sendbytes;
};

int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[FD_SETSIZE];

//소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

//오류 출력 함수
void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);

int main()
{
	//한글세팅
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");

	//원속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	int retval;
	
	//socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit(L"socket()");

	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)err_quit(L"bind()");

	//listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)err_quit(L"listen()");

	//넌 블로킹 소켓으로 전환
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)err_display(L"ioctlsocket()");

	//데이터 통신에 사용할 변수
	FD_SET rset, wset;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int i;
	
	for (;;)
	{
		//소켓 셋 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for (i=0;i<nTotalSockets;i++)
		{
			if (SocketInfoArray[i]->recvbytes > SocketInfoArray[i]->sendbytes)
			{
				FD_SET(SocketInfoArray[i]->sock, &wset);
			}
			else
			{
				FD_SET(SocketInfoArray[i]->sock, &rset);
			}
		}

		//select()
		retval = select(0,&rset,&wset,NULL,NULL);
		if (retval == SOCKET_ERROR)err_quit(L"select()");

		//소켓 셋 검사(1): 클라이언트 접속 수용
		if (FD_ISSET(listen_sock, &rset))
		{
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock,(SOCKADDR*)&clientaddr,&addrlen);
			if (client_sock==INVALID_SOCKET)
			{
				err_display(L"accept()");
			}
			else
			{
				WCHAR clientIP[40] = { 0, };
				InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP));
				//if (InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP)) == NULL) {}
				
				wprintf(L"\n[TCP 서버] 클라이언트 접속: IP 주소 %s, 포트 번호=%d\n",
					clientIP, ntohs(clientaddr.sin_port));
				//소켓 정보 추가
				AddSocketInfo(client_sock);
			}
		}
		//소켓 셋 검사(2):데이터 통신
		for (i=0;i<nTotalSockets;i++)
		{
			SOCKETINFO* ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock,&rset)) 
			{
				//데이터 받기
				retval = recv(ptr->sock, (char*)ptr->buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display(L"recv()");
					RemoveSocketInfo(i);
					continue;
				}
				else if (retval == 0)
				{
					RemoveSocketInfo(i);
					continue;
				}

				ptr->recvbytes = retval;
				//받은 데이터 출력
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
				WCHAR clientIP[40] = { 0, };
				InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP));

				//ptr->buf[retval] = '\0';
				ptr->buf[retval / 2] = '\0';

				wprintf(L"[TCP/%s:%d] %s\n", clientIP,
					ntohs(clientaddr.sin_port), ptr->buf);

			}
			if (FD_ISSET(ptr->sock, &wset)) 
			{
				//데이터 보내기
				retval = send(ptr->sock, ((char*)ptr->buf) + ptr->sendbytes,
					ptr->recvbytes - ptr->sendbytes, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display(L"send()");
					RemoveSocketInfo(i);
					continue;
				}
				ptr->sendbytes += retval;
				if (ptr->recvbytes == ptr->sendbytes)
				{
					ptr->recvbytes = ptr->sendbytes = 0;
				}
			}
		}

	}
	//원속종료
	WSACleanup();
	return 0;
}

//소켓 정보 추가
BOOL AddSocketInfo(SOCKET sock)
{
	if (nTotalSockets >= FD_SETSIZE)
	{
		wprintf(L"[오류] 소켓 정보를 추가할 수 없습니다.\n");
		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr ==NULL) 
	{
		wprintf(L"[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr;

	return TRUE;
};
//소켓 정보 삭제
void RemoveSocketInfo(int nIndex)
{
	SOCKETINFO* ptr = SocketInfoArray[nIndex];

	//클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
	WCHAR clientIP[40] = { 0, };
	InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP));
	wprintf(L"[TCP 서버 ] 클라잉너트 종료: IP 주소=%s, 포트 번호=%d\n",
		clientIP,ntohs(clientaddr.sin_port));
	
	closesocket(ptr->sock);
	delete ptr;

	if (nIndex != (nTotalSockets - 1))
	{
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
	}

	--nTotalSockets;
};

//소켓 함수 오류 출력 후 종료
void err_quit(const WCHAR* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

//소켓 함수 오류 출력
void err_display(const WCHAR* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	wprintf(L"[%s] %s", msg, (WCHAR*)lpMsgBuf);
	printf("GetLastError() %d\n", GetLastError());
	LocalFree(lpMsgBuf);
}