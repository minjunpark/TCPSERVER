

#pragma comment(lib,"ws2_32")
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdlib>
#include <cstdio>
#include <clocale>
#define SERVERPORT 9000
#define BUFSIZE 512

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

int main()
{
	int retval;

	//한글세팅
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");

	//원속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

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

	//데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	WCHAR buf[BUFSIZE + 1];

	for (;;)
	{
		//accept();
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display(L"accept()");
			break;
		}

		//접속한 클라이언트 정보 출력
		WCHAR clientIP[40] = { 0, };
		if (InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP)) == NULL)
		{

		}

		wprintf(L"\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트번호=%d\n",
			//(WCHAR*)inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			clientIP, ntohs(clientaddr.sin_port));

		//클라이언트와 데이터 통신
		for (;;)
		{
			//데이터받기
			retval = recv(client_sock, (char*)buf, BUFSIZE, 0);//유니코드니까 char변환
			if (retval == SOCKET_ERROR)
			{
				err_display(L"recv()");
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			if (InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP)) == NULL)
			{

			}
			printf("retval %d \n", retval);
			//받은 데이터 출력
			buf[retval / 2] = '\0';
			wprintf(L"[TCP/%s:%d] %s\n", clientIP,
				ntohs(clientaddr.sin_port), buf);

			//데이터 보내기
			retval = send(client_sock, (char*)buf, retval, 0);//유니코드니까 char변환
			if (retval == SOCKET_ERROR)
			{
				err_display(L"send()");
				break;
			}
		}
		//closesocket()
		closesocket(client_sock);

		if (InetNtopW(AF_INET, &clientaddr.sin_addr, clientIP, sizeof(clientIP)) == NULL)
		{

		}

		wprintf(L"[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트번호=%d\n",
			clientIP, ntohs(clientaddr.sin_port));

	}
	closesocket(listen_sock);
	//closecoekt();

	//원속 종료
	WSACleanup();
	return 0;
}
