#ifndef UNICODE
#define UNICODE
#endif
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#define SERVERIP "127.0.0.1"
#define SERVERPORT 4500

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

int wmain()
{

    //----------------------
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = 0;

    SOCKET ListenSocket = INVALID_SOCKET;
    sockaddr_in service;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup() failed with error: %d\n", iResult);
        return 1;
    }
    //----------------------
    // Create a SOCKET for listening for incoming connection requests.
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr(SERVERIP);
    service.sin_port = htons(SERVERPORT);
    
    iResult = bind(ListenSocket, (SOCKADDR*)&service, sizeof(service));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"bind function failed with error %d\n", WSAGetLastError());
        iResult = closesocket(ListenSocket);
        if (iResult == SOCKET_ERROR)
            wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //----------------------
    // Listen for incoming connection requests 
    // on the created socket
    
    
    //if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    //if (listen(ListenSocket, SOMAXCONN_HINT(50000)) == SOCKET_ERROR)
      //  wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
    //listen(ListenSocket, SOMAXCONN_HINT(100000));
    //listen(ListenSocket, SOMAXCONN_HINT(65535));
    //listen(ListenSocket, SOMAXCONN_HINT(50000));
    //16365
    //
    if (listen(ListenSocket, SOMAXCONN_HINT(70000)) == SOCKET_ERROR)
        wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
    else
        wprintf(L"listen OK! %d\n", WSAGetLastError());
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    int cnt = 0;
    for (;;)
    {
        wprintf(L"Listening on socket... %d\n",cnt);
        cnt++;  
    }
    iResult = closesocket(ListenSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    WSACleanup();
    return 0;
}
