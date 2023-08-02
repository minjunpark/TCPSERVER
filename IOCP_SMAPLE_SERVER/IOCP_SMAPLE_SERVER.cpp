#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
#include<stdio.h>
#include<Winsock2.h>
#include<stdlib.h>

#define PORT	9000
#define BUFSIZE 512

//소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO {
    WSAOVERLAPPED overlapped;
    SOCKET sock;
    char buf[BUFSIZE + 1];
    int recvBytes;
    int sendBytes;
    WSABUF wsabuf;
};

//작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg);

//오류 출력 함수
void err_quit(const char* msg);

int main()
{
    WSADATA wsaData;

    //윈속 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        err_quit("WSAStartup");

    //입출력 완료 포트 생성
    HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (hcp == NULL) return 1;

    //CPU 개수 확인
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // (CPU 개수 * 2)개의 작업자 스레드 생성
    HANDLE hThread;
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
        hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
        if (hThread == NULL) return 1;
        CloseHandle(hThread);
    }

    //소켓 생성
    SOCKET listen_s = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_s == INVALID_SOCKET)
        err_quit("socket");

    SOCKADDR_IN sock_addr;
    ZeroMemory(&sock_addr, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (bind(listen_s, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) err_quit("Bind");
    if (listen(listen_s, SOMAXCONN) == SOCKET_ERROR) err_quit("listen");

    //데이터 통신에 사용할 변수
    SOCKET clientsock;
    SOCKADDR_IN clientaddr;
    int addrlen = sizeof(clientaddr);
    DWORD recvbytes;

    while (1) {
        //accept()
        printf("accept()1\n");
        clientsock = accept(listen_s, (SOCKADDR*)&clientaddr, &addrlen);
        if (clientsock == INVALID_SOCKET) {
            err_quit("accept()");
            break;
        }
        printf("[TCP서버] 클라이언트 접속 IP : %s %d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        //소켓과 입출력 완료 포트 연결
        CreateIoCompletionPort((HANDLE)clientsock, hcp, clientsock, 0);

        //소켓 정보 구조체 할당
        SOCKETINFO* ptr = new SOCKETINFO;
        if (ptr == NULL) break;

        ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
        ptr->sock = clientsock;
        ptr->recvBytes = ptr->sendBytes = 0;
        ptr->wsabuf.buf = ptr->buf;
        ptr->wsabuf.len = BUFSIZE;

        //비동기 입출력 시작
        //이걸처리해줘야 해당 오버랩에서
        DWORD flags = 0;
        int retVal = WSARecv(clientsock, &ptr->wsabuf, 1, &recvbytes, &flags,
            &ptr->overlapped, NULL);
        if (retVal == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                err_quit("WSARecv()");
            }
            continue;
        }
    }

    //윈속 종료
    WSACleanup();
    return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg) {
    int retVal;
    HANDLE hcp = (HANDLE)arg;

    while (1) {
        //비동기 입출력 완료 기다리기
        DWORD cbTransferred;
        SOCKET clientsock;
        SOCKETINFO* ptr;
        retVal = GetQueuedCompletionStatus(hcp, &cbTransferred,
            (PULONG_PTR)&clientsock, (LPOVERLAPPED*)&ptr, INFINITE);
        
        //클라 정보 얻기
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);
        getpeername(clientsock, (SOCKADDR*)&clientaddr, &addrlen);

        //비동기 입출력 결과 확인
        if (retVal == 0 || cbTransferred == 0) {
            if (retVal == 0) {//이게있으면 강제종료 되버리는데
                DWORD temp1, temp2;
                WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &temp1, FALSE, &temp2);
                //err_quit("WSAGetOverlappedResult()");
            }
            closesocket(ptr->sock);
            printf("[TCP 서버] 클라이언트 종료 : IP  주소 = %s, 포트번호 = %d\r\n",
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
            delete ptr;
            continue;
        }

        //데이터 전송량 갱신
        if (ptr->recvBytes == 0) {
            ptr->recvBytes = cbTransferred;
            ptr->sendBytes = 0;

            //받은 데이터 출력
            ptr->buf[ptr->recvBytes] = '\0';
            printf("[TCP /%s : %d] %s\r\n", inet_ntoa(clientaddr.sin_addr),
                ntohs(clientaddr.sin_port), ptr->buf);
        }
        else {
            ptr->sendBytes += cbTransferred;
        }

        if (ptr->recvBytes > ptr->sendBytes) {
            ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
            ptr->wsabuf.buf = ptr->buf + ptr->sendBytes;
            ptr->wsabuf.len = ptr->recvBytes - ptr->sendBytes;

            DWORD sendbytes;
            retVal = WSASend(ptr->sock, &ptr->wsabuf, 1, &sendbytes, 0,
                &ptr->overlapped, NULL);
            if (retVal == SOCKET_ERROR) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    err_quit("WSASend()");
                    continue;
                }
            }
        }
        else {
            ptr->recvBytes = 0;

            //데이터 받기
            ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
            ptr->wsabuf.buf = ptr->buf;
            ptr->wsabuf.len = BUFSIZE;

            DWORD recvbytes;
            DWORD flags = 0;
            retVal = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes,
                &flags, &ptr->overlapped, NULL);
            if (retVal == SOCKET_ERROR) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    err_quit("WSASend()");
                    continue;
                }
            }
        }
    }
    return 0;
}

void err_quit(const char* msg) {
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
    exit(1);
}