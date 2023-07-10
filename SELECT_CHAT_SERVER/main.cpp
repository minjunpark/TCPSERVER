#pragma comment(lib,"ws2_32")
#pragma comment(lib,"winmm.lib")
#include <iostream>
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <cstdlib>
#include <cstdio>
#include <clocale>
#include <conio.h>
#include <memory.h>
#include <Windows.h>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <list>

//#include "st_Packet.h"
#include "CMemoryPool.h"
#include "CRingBuffer.h"
#include "CSerealBuffer.h"
#include "Protocol.h"

#define df_LOG
using namespace std;

void NetWork();
void Logic();
//서버 센드 리시브함수
void init_sock();
void AcceptProc();//접속유저가 새로 생겼을때
//void RecvProc(Session* session);//유저가 데이터를 보내왔을때
//void SendProc(Session* session);//유저에게 데이터를 보낼때
void RecvProc(DWORD User_NO);//유저가 데이터를 보내왔을때
void SendProc(DWORD User_NO);//유저에게 데이터를 보낼때
void SelectSocket(DWORD* dwpTableNO, SOCKET* pTableSocket, FD_SET* pReadSet, FD_SET* pWriteSet);
void sendUniCast(Session* session, CSerealBuffer* pPacket);
void sendBroadCast(Session* session, CSerealBuffer* pPacket);
void sendBroadCast(Session* session, CSerealBuffer* pPacket);
Session* FindClient(DWORD User_NO);
ROOM* Find_Room(DWORD Room_NUM);
//체크섬 확인용 코드
BYTE _Ret_Checksum(WORD MsgType, WORD PayloadSize, CSerealBuffer* Packet);
bool _Nick_Check(WCHAR* NickName);//닉네임 중복체크 있으면 true 없으면 false
bool _Room_Check(WCHAR* RoomName);

void sendBroadCast_unmap_ROOM(Session* session, ROOM* pRoom, CSerealBuffer* pPacket);
void sendBroadCast_unmap(Session* session, CSerealBuffer* pPacket);
void sendUniCast_unmap(Session* session, CSerealBuffer* pPacket);

void Disconnect(Session* session);//연결끊기 Session _Live를 false로 바꿔죽인다.
//void Disconnect_Clean();//네트워크 마지막에 죽은세션을 정리한다.

void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);
void log_msg(int _wsaError, int _line, const char* _file);


//Key를 뭘로하나
//유저는 유저 세션의 고유번호
unordered_map<DWORD, Session*>User_List;
//방번호는 방의 고유번호로
unordered_map<DWORD, ROOM*>Room_List;

CMemoryPool<Session> chat_SessionPool(300, FALSE);
CMemoryPool<ROOM> chat_RoomPool(300, FALSE);

//서버에 받아들이기 위한 리슨소켓
SOCKET listen_sock;

//확인용 함수데이터 저장


int User_Id_Count = 1;//유저 시작번호
int Room_Id_Count = 1;//고유 방번호
int msg_Tps = 0;
int msg_Accept_Tps = 0;

#include "fun_Message.h"

int main()
{
	timeBeginPeriod(1);
	setlocale(LC_ALL, "");
	init_sock();
	
	while (1) 
	{
		NetWork();//네트워크
	}
}

void Monitor()
{
	wprintf(L"접속자 / 방개수 / 로비 유저 / 초당 처리수 / 메세지 카운터 / ACCEPT 카운터 /s");
	wprintf(L"%d / %d / %d / %d / %d / %d ", 
		User_List.size(), Room_List.size(),0, msg_Tps, msg_Accept_Tps);
}

void NetWork()
{
	Session* pClient;
	DWORD UserTable_NO[FD_SETSIZE];
	SOCKET UserTable_SOCKET[FD_SETSIZE];
	int iSocketCount = 0;
	
	FD_SET rset;
	FD_SET wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	memset(UserTable_NO, -1, sizeof(DWORD) * FD_SETSIZE);
	memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);


	FD_SET(listen_sock, &rset);//리슨소켓을 rset소켓에 세팅한다
	UserTable_NO[iSocketCount] = 0;
	UserTable_SOCKET[iSocketCount] = listen_sock;
	iSocketCount++;

	for (auto User_List_it = User_List.begin(); User_List_it != User_List.end();)
	{
		pClient = User_List_it->second;
		User_List_it++;
		
		UserTable_NO[iSocketCount] = pClient->_User_No;
		UserTable_SOCKET[iSocketCount] = pClient->_Sock;

		FD_SET(pClient->_Sock, &rset);//반응이 있는 ListenSocket을 모두 넣는다.

		if (pClient->_SendBuf.GetUseSize() > 0)//세션 버퍼에 보내야하는 데이터가 0이상이라면 FD세팅을 한다.
			FD_SET(pClient->_Sock, &wset);

		iSocketCount++;

		if (FD_SETSIZE <= iSocketCount)
		{
			SelectSocket(UserTable_NO, UserTable_SOCKET, &rset, &wset);
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			memset(UserTable_NO, -1, sizeof(DWORD) * FD_SETSIZE);
			memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			iSocketCount = 0;
		}
	}

	if (iSocketCount > 0)
	{
		SelectSocket(UserTable_NO, UserTable_SOCKET, &rset, &wset);
	}

};

bool PacketProc(Session* pSession, BYTE byPacketType, CSerealBuffer* pPacket)
{
	switch (byPacketType)
	{
		case df_REQ_LOGIN://Req 로그인
			return netPacket_ReqLogin(pSession, pPacket);
			break;
		case df_REQ_ROOM_LIST://Req 대화방 리스트
			return netPacket_ReqRoomList(pSession, pPacket);
			break;
		case df_REQ_ROOM_CREATE://로그인
			return netPacket_ReqRoomCreate(pSession, pPacket);
			break;
		case df_REQ_ROOM_ENTER://방에 입장
			return netPacket_ReqRoomEnter(pSession, pPacket);
			break;
		case df_REQ_CHAT://로그인
			return netPacket_ReqReqChat(pSession, pPacket);
			break;
		case df_REQ_ROOM_LEAVE://방에서 떠났을때
			return netPacket_ReqReqRoomLeave(pSession, pPacket);
			break;
		case df_REQ_STRESS_ECHO://방에서 떠났을때
			return netPacket_ReqReqRoomLeave(pSession, pPacket);
			break;
	}
	return true;
}

void RecvProc(DWORD User_No)//유저가 데이터를 보내왔을때
{
	Session* tmpSesion;
	tmpSesion = FindClient(User_No);

	if (tmpSesion == nullptr)
		return;

	int retRecv;
	retRecv = recv(tmpSesion->_Sock, (char*)tmpSesion->_RecvBuf.GetRearBufferPtr(),
		tmpSesion->_RecvBuf.DirectEnqueueSize(), 0);//한번에 넣을수있는 사이즈만큼 넣는다.

	if (retRecv == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값

		log_msg(wsaError, __LINE__, __FILE__);

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(tmpSesion);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retRecv == 0)//그냥 연결종료 상태
	{
		int wsaError = WSAGetLastError();//소켓에러값

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(tmpSesion);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
		return;
	}

	tmpSesion->_RecvBuf.MoveRear(retRecv);//받은만큼 이동시킨다.

	CSerealBuffer cpPacket;//데이터를 받고 처리하기위한 임시 직렬화버퍼

	while (1)
	{
		if (tmpSesion->_RecvBuf.GetUseSize() < sizeof(st_PACKET_HEADER))//사용할수 있는사이즈가 헤더이하라면
			break;

		st_PACKET_HEADER st_recv_header;
		int retPeek;
		retPeek = tmpSesion->_RecvBuf.Peek((char*)&st_recv_header, sizeof(st_PACKET_HEADER));//헤더크기만큼 꺼내기

		if (retPeek != sizeof(st_PACKET_HEADER))//값이 다르면 애초에 박살나는것이기 때문에 브레이크
		{
			Disconnect(tmpSesion);
			break;
		}

		if (tmpSesion->_RecvBuf.GetUseSize() < sizeof(st_PACKET_HEADER) + st_recv_header.wPayloadSize)//헤더에 적힌 사이즈 보다 작으므로 더 받는절차를 거쳐야하므로 정지한다.
			break;

		//한패킷이 완성됬다는 뜻이므로 여기서 체크섬을 확인한다.
		//확인을 위해 헤더부터 체크섬까지 데이터를 직렬화버퍼에 긁어온다.
		tmpSesion->_RecvBuf.Peek(cpPacket.GetBufferPtr(), sizeof(st_PACKET_HEADER)
			+ st_recv_header.wPayloadSize);

		int retChecksum;
		//리턴된 체크섬 값과 상대가 보내준 체크섬값을 비교한다.
		retChecksum = _Ret_Checksum(st_recv_header.wMsgType, st_recv_header.wPayloadSize, &cpPacket);
		if (retChecksum != st_recv_header.byCheckSum)//이게다르다면 이패킷은 위변조된것이다.
		{
			//위변조된 헤더와 크기만큼 바이트를 밀어버리고
			//tmpSesion->_RecvBuf.MoveFront(sizeof(st_PACKET_HEADER) + st_recv_header.wPayloadSize);
			Disconnect(tmpSesion);//위변조된 데이터를 보낸것이므로 종료시킨다.
			break;
		}

		////체크섬까지 통과해서 위변조된것도 없다.
		////헤더에서 읽어온 크기만큼 버퍼에 존재한다는 뜻이다.
		////한 패킷이 완성됬다는것

		cpPacket.Clear();//체크섬 검사하느라 썼던 직렬화버퍼를 초기화시켜준다.
		
		tmpSesion->_RecvBuf.MoveFront(sizeof(st_PACKET_HEADER));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.		

		//직렬화버퍼 작업
		int retSwitchDeq;
		retSwitchDeq = tmpSesion->_RecvBuf.Dequeue(cpPacket.GetBufferPtr(), st_recv_header.wPayloadSize);//사이즈만큼 직렬화 버퍼에 디큐한다.

		if (retSwitchDeq != st_recv_header.wPayloadSize)//헤더에 적힌 사이즈만큼 있다고 판별된 상황에서 사이즈만큼 가져오지 못한다면 버퍼가 깨진것이므로 연결을 끊고 세션을 파괴한다.
		{
			Disconnect(tmpSesion);
			return;
		}

		int retCS;
		retCS = cpPacket.MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

		if (retCS != retSwitchDeq)//직렬화 버퍼가 이동하지 못했다면 파괴한다.
		{
			Disconnect(tmpSesion);
			return;
		}

		PacketProc(tmpSesion, st_recv_header.wMsgType, &cpPacket);

		cpPacket.Clear();//직렬화 버퍼 재사용을 위한 초기화
	}

};

//void SendProc(Session* session)
void SendProc(DWORD User_No)
{
	Session* session;

	session = FindClient(User_No);
	if (session == nullptr)
		return;
	
	if (session->_SendBuf.GetUseSize() <= 0)//보내야할 내용이 없다면 안보낸다.
	{
		return;
	}

	int retSend;
	//읽기 버퍼 포인터에서 보낼수있는 크기만큼 최대한 보낸다.
	retSend = send(session->_Sock, session->_SendBuf.GetFrontBufferPtr(), session->_SendBuf.DirectDequeueSize(), 0);

	if (retSend == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값
		log_msg(wsaError, __LINE__, __FILE__);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retSend == 0)//연결 종료상태
	{
		int wsaError = WSAGetLastError();//소켓에러값
		//log_msg(wsaError);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}

	if (retSend != session->_SendBuf.DirectDequeueSize())//긁어온것과 보낸양이 다르다? 링버퍼가 박살나서 이상하게 만들어진것이다 연결을 종료하고 로그를 남겨야한다.
	{
		Disconnect(session);
		return;
	}

	session->_SendBuf.MoveFront(retSend);//센드버퍼에 쌓은만큼 포인터를 이동시킨다.
};//유저에게 데이터를 보낼때


void AcceptProc()
{
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	addrlen = sizeof(clientaddr);
	client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);

	if (client_sock == SOCKET_ERROR)//엑셉트 에러면 굳이 건들가치가 없음 그냥 세션연결 자체도 생성안해도됨
	{
		int wsaError = WSAGetLastError();
		log_msg(wsaError, __LINE__, __FILE__);
		closesocket(client_sock);
		return;
	}

	int AccteptTotal = User_Id_Count++;//여기서 계속 +하면서 ID 값을 새로 생성한다.

#ifdef df_LOG
	printf("Conncet # IP %d / SessionID %d\n", 127, AccteptTotal);
#endif

	//로그인이 될지 안될지 모르긴한데 일단 할당받는다.
	Session* NewSession = chat_SessionPool.Alloc();//세션 alloc을 부여받는다.
	NewSession->_SendBuf.ClearBuffer();
	NewSession->_RecvBuf.ClearBuffer();
	NewSession->_Sock = client_sock;//sock을 세팅하고
	NewSession->_Connect_Room_No = dfdefaultRoom;//연결된 방번호 세팅 음수번호라면 어느방에도 접속해 있지 않은 상태다.
	NewSession->_User_No = AccteptTotal;//X세팅
	memset(NewSession->_Nick_Name, 0, dfNICK_MAX_LEN * sizeof(WCHAR));//닉네임은 아직 받지 않았으므로 전부 0으로
	NewSession->_Live = true;//세션 생존플래그 0이면 사망
	NewSession->_Login_check = false;//얘가 로그인인지 아닌지
	NewSession->ConnectAddr = clientaddr;
	
	//중복유저인지 로그인이 될지 안될지는 추후에 판단 하자
	//애초에 버퍼 처리를 안해서 여기서 판단이 안됨
	
	User_List.emplace(make_pair(NewSession->_User_No, NewSession));
};


void init_sock()
{
	//한글세팅
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");

	//원속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)return;

#ifdef df_LOG
	else printf("WSAStartup #\n");
#endif

	//listen socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit(L"socket()");

	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	//linger timewait 0 rst바로 전송하게 만들기
	linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;

	int keepval = 0;
	//linger로 time out을 0으로 잡아서 rst를 바로 쏴 TimeWait제거
	setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (const char*)&_linger, sizeof(_linger));//링거로 타임아웃 0
	setsockopt(listen_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepval, sizeof keepval);//keep alive내가 직접할것

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(dfNETWORK_PORT);

	int retOut;
	retOut = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retOut == SOCKET_ERROR)err_quit(L"bind()");

#ifdef df_LOG
	else printf("Bind OK # PORT %d\n", dfNETWORK_PORT);
#endif

	//listen()
	retOut = listen(listen_sock, SOMAXCONN);
	if (retOut == SOCKET_ERROR)err_quit(L"listen()");

#ifdef df_LOG
	else printf("listen OK #\n");
#endif

	//넌 블로킹 소켓으로 전환
	u_long on = 1;
	retOut = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retOut == SOCKET_ERROR)err_quit(L"ioctlsocket()");

}

BYTE _Ret_Checksum(WORD MsgType, WORD PayloadSize, CSerealBuffer* Payload)
{
	//	| PacketCode | CheckSum | MsgType | PayloadSize | * Payload * |
	//		1Byte        1Byte	   2Byte      2Byte        Size Byte     
	//
	//	checkSum - 각 MsgType, Payload 의 각 바이트 더하기 % 256
	//
	//------------------------------------------------------

	int retCheckSum = 0;
	retCheckSum = MsgType;
	
	char* StartPtr = Payload->GetStartPtr();//시작지점을 얻어오고
	StartPtr += sizeof(st_PACKET_HEADER);

	for (int i = 0; i < PayloadSize; i++)
	{
		retCheckSum += *(StartPtr + i);
	}
	
	return (BYTE)(retCheckSum%256);
}

void Disconnect(Session* session)//연결 끊기 함수
{
	Session* stmp = session;
	//언오더드 맵에서 유저 번호랑 일치하는 녀석이 있다면
	auto item = User_List.find(stmp->_User_No);
	if (item != User_List.end()) 
	{
		(*item).second->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
	}
	else {
		//std::cout << "Key not found";
	}
};

void Disconnect_Clean()//안정성을 위해 디스커넥트된 개체를 네트워크 마지막시점에 정리한다.
{
	//모든 리스트를 돌면서 죽어있는 객체라면 제거한다.
	Session* _Session_Object;
	for (auto _Session_it = User_List.begin(); _Session_it != User_List.end();)
	{
		_Session_Object = (*_Session_it).second;
		if (_Session_Object->_Live == false) //세션 특정객체가 죽어있다면
		{
			//모든유저에게 데이터 전송
			//PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);
			//delete _Session_Object;//동적할당된 세션 제거
			closesocket(_Session_Object->_Sock);
			chat_SessionPool.Free(_Session_Object);//세션을 풀에 반납한다.
			_Session_it = User_List.erase(_Session_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
		}
		else
		{
			++_Session_it;//살아있는 객체라면 다음 리스트로 넘어간다.
		}
	}
}

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
	printf("GetLastError() %d\n", WSAGetLastError());
	LocalFree(lpMsgBuf);
}

//로직처리함수
void log_msg(int _wsaError, int _line, const char* _file)
{
	//이 에러라면 로그저장 안한다.
	if (_wsaError == WSAECONNRESET//현재 연결은 원격 호스트에 의해 강제로 끊겼습니다.
		|| _wsaError == WSAECONNABORTED//소프트웨어로 인해 연결이 중단되었습니다.
		|| _wsaError == WSANOTINITIALISED//성공한 WSAStartup이 아직 수행되지 않았습니다.
		|| _wsaError == WSAEWOULDBLOCK//이 오류는 즉시 완료할 수 없는 비블로킹 소켓의 작업에서 반환됩니다
		|| _wsaError == WSAEFAULT)//주소가 잘못되었습니다. 애플리케이션이 잘못된 포인터 값을 전달하거나 버퍼의 길이가 너무 작은 경우에 발생

	{
		return;
	}

	char log_FileName[60];// = "127.0.0.1";
	FILE* fp;
	//로그저장용
	time_t now = time(NULL);  //now에 현재 시간 저장
	struct tm date;   //tm 구조체 타입의 날짜
	localtime_s(&date, &now);  //date에 now의 정보를 저장

	sprintf_s(log_FileName, sizeof(log_FileName), "Log_%02d_%02d_%02d.txt", date.tm_mday, date.tm_hour, date.tm_min);
	fopen_s(&fp, log_FileName, "wb");//파일 생성
	if (fp == nullptr)
	{
		return;
	}
	fclose(fp);

	char buff[200];
	memset(buff, 0, sizeof(buff));
	sprintf_s(buff, "retRecv Error = %d Line = %d File = %s", _wsaError, _line, _file);
	fopen_s(&fp, log_FileName, "a");
	if (fp == nullptr)
	{
		return;
	}
	fprintf(fp, "%s\n", buff);
	fclose(fp);
}

void sendUniCast(Session* session, CSerealBuffer* pPacket)
{
	if (session->_Live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}

	if (session->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 직렬화버퍼크기만큼 넣는다.
	{
		int retUni;
		retUni = session->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != pPacket->GetUseSize())
		{
			Disconnect(session);
			return;
		}
	}
}

void sendBroadCast(Session* session, CSerealBuffer* pPacket)
{
	int retBro;
	Session* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	for (auto session_it = User_List.begin(); session_it != User_List.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->_User_No == (*session_it).second->_User_No)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if ((*session_it).second->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}

		//각세션의 센드버퍼에 enqueue시킨다.
		if ((*session_it).second->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = (*session_it).second->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != pPacket->GetUseSize())//센드버퍼 자체가 고장났거나 송신버퍼가 가득찬경우 이므로 디스커넥트하고 끝내버린다.
			{
				Disconnect((*session_it).second);
			}
		}
	}
}

void sendUniCast_unmap(Session* session, CSerealBuffer* pPacket)
{
	if (session == nullptr)
	{
		wprintf(L"sendUniCast_unmap : session is null\n");
		return;//죽어있는패킷한테는안보낸다.
	}

	if (session->_Live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}

	if (session->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 직렬화버퍼크기만큼 넣는다.
	{
		int retUni;
		retUni = session->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != pPacket->GetUseSize())
		{
			Disconnect(session);
			return;
		}
	}
}

void sendBroadCast_unmap(Session* session, CSerealBuffer* pPacket)
{
	int retBro;
	Session* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	for (auto session_it = User_List.begin(); session_it != User_List.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->_User_No == (*session_it).second->_User_No)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if ((*session_it).second->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}

		//각세션의 센드버퍼에 enqueue시킨다.
		if ((*session_it).second->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = (*session_it).second->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != pPacket->GetUseSize())//센드버퍼 자체가 고장났거나 송신버퍼가 가득찬경우 이므로 디스커넥트하고 끝내버린다.
			{
				Disconnect((*session_it).second);
			}
		}
	}
}

void sendBroadCast_unmap_ROOM(Session * session, ROOM* pRoom, CSerealBuffer* pPacket)
{
	int retBro;
	Session* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	for (auto session_it = pRoom->_Room_UserList.begin(); session_it != pRoom->_Room_UserList.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->_User_No == (*session_it)->_User_No)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if ((*session_it)->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}
		
		//각세션의 센드버퍼에 enqueue시킨다.
		if ((*session_it)->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = (*session_it)->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != pPacket->GetUseSize())//센드버퍼 자체가 고장났거나 송신버퍼가 가득찬경우 이므로 디스커넥트하고 끝내버린다.
			{
				Disconnect((*session_it));
			}
		}
	}
}

bool _Nick_Check(WCHAR* NickName)
{
	//유저 닉네임 검색하기
	//true면 중복
	//false면 중복아님
	int retNick;
	for (auto session_it = User_List.begin(); session_it != User_List.end(); ++session_it)
	{
		retNick = wcscmp(NickName, (*session_it).second->_Nick_Name);
		if (retNick == 0)
		{
			//일치하는 닉네임이 존재한다면
			return true;
		}
	}
	return false;//중복된게없다면 false
}

bool _Room_Check(WCHAR* RoomName)
{
	//유저 닉네임 검색하기
	//true면 중복
	//false면 중복아님
	int retNick;
	for (auto session_it = Room_List.begin(); session_it != Room_List.end(); ++session_it)
	{
		retNick = wcscmp(RoomName, (*session_it).second->_RoomName);
		if (retNick == 0)
		{
			//일치하는 닉네임이 존재한다면
			return true;
		}
	}
	return false;//중복된게없다면 false
}

ROOM* Find_Room(DWORD Room_NUM)
{
	auto item = Room_List.find(Room_NUM);
	if (item != Room_List.end())
	{
		//(*item).second->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
		return (*item).second;
	}
	else
	{
		return nullptr;
	}
}

//bool _Login_Check(Session *pSession)
//{
//	//유저 닉네임 검색하기
//	int retLogin;
//	auto item = User_List.find(pSession->_User_No);
//	if (item != User_List.end())
//	{
//		//(*item).second->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
//		if (pSession->_Nick_Name == nullptr
//			|| pSession->_Connect_Room_No != 0)
//		{
//			return false;
//		}
//		else
//		{
//			return true;
//		}
//	}
//	else
//	{
//		return false;
//	}
//}


void SelectSocket(DWORD *dwpTableNO, SOCKET* pTableSocket, FD_SET* pReadSet, FD_SET* pWriteSet)
{
	timeval Time;
	int iResult, iCnt;

	Time.tv_sec = 0;
	Time.tv_usec = 0;

	iResult = select(0,pReadSet,pWriteSet,0,&Time);

	if (0 < iResult)
	{
		for (iCnt = 0; iCnt < FD_SETSIZE; ++iCnt)
		{
			if (pTableSocket[iCnt] == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pTableSocket[iCnt], pWriteSet))
			{
				SendProc(dwpTableNO[iCnt]);
			}
			if (FD_ISSET(pTableSocket[iCnt], pReadSet))
			{
				if (dwpTableNO[iCnt] == 0)
					AcceptProc();
				else
					RecvProc(dwpTableNO[iCnt]);
			}
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		wprintf(L"select socekt error !!\n");
	}
}

Session* FindClient(DWORD User_NO)
{
	auto item = User_List.find(User_NO);
	if (item != User_List.end())
	{
		//(*item).second->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
		return (*item).second;
	}
	else 
	{
		return nullptr;
	}
}