#pragma comment(lib,"ws2_32")
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdlib>
#include <cstdio>
#include <clocale>
using namespace std;

#define BROADCAST_PORT 8888

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        //cerr << "Error creating socket" << endl;
        return 1;
    }

    // Enable broadcast
    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1) {
        //cerr << "Error setting socket option" << endl;
        closesocket(sock);
        return 1;
    }

    // Bind to port
    sockaddr_in broadcastAddr;
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(BROADCAST_PORT);
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == -1) {
        //cerr << "Error binding to port" << endl;
        closesocket(sock);
        return 1;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    socklen_t clientAddrLen = sizeof(sockaddr_in);
    sockaddr_in clientAddr;
    while (true) {
        // Receive message from client
        
        int recvLen = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (recvLen == -1) {
            //cerr << "Error receiving message" << endl;
            continue;
        }

        // Print message from client
        //cout << "Received message from " << inet_ntoa(clientAddr.sin_addr) << ": " << buffer << endl;
    }

    closesocket(sock);
    return 0;
}
//#pragma comment(lib,"ws2_32")
//#include <ws2tcpip.h>
//#include <WinSock2.h>
//#include <cstdlib>
//#include <cstdio>
//#include <clocale>
//
//#define BUF_SIZE 100
//#define PORT 10001
//void error_handling(char* message);
//
//int main(int argc, char* argv[])
//{
//    int serv_sock;
//    char message[BUF_SIZE];
//    int str_len;
//    socklen_t clnt_adr_sz;
//
//    SOCKADDR_IN serv_adr;
//    SOCKADDR_IN clnt_adr;
//
//    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
//    if (serv_sock == -1)
//        wprintf(L"serv_sock");
//
//    memset(&serv_adr, 0, sizeof(serv_adr));
//    serv_adr.sin_family = AF_INET;
//    serv_adr.sin_port = htons(10001);
//    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
//    
//    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) < -1)
//        wprintf(L"bind");
//    clnt_adr_sz = sizeof(clnt_adr);
//    while (1)
//    {
//        //printf("recv......\n");
//        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
//        if(str_len>0)
//            printf("str_len %d\n", str_len);
//        //sendto(serv_sock, message, str_len, 0,(struct sockaddr*)&clnt_adr, clnt_adr_sz);
//    }
//    closesocket(serv_sock);
//    return 0;
//}
//
//void error_handling(char* message)
//{
//    fputs(message, stderr);
//    fputc('\n', stderr);
//    exit(1);
//}