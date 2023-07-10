
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
#include <vector>
#include "CRingBuffer.h"
#include "CMemoryPool.h"
#include "CSerealBuffer.h"
#include "enum_Server.h"
#include "CLOG.h"
#include "st_Packet.h"

//#define df_LOG//로그 켜고싶으면 세팅

using namespace std;

void init_Game();
void ServerControl();
void Monitor();
void NetWork();
void Update();
//서버 센드 리시브함수
void init_Sock();
void AcceptProc();//접속유저가 새로 생겼을때
void RecvProc(st_SESSION* session);//유저가 데이터를 보내왔을때
void SendProc(st_SESSION* session);//유저에게 데이터를 보낼때

void RecvProc(SOCKET pSocket);//유저가 데이터를 보내왔을때
bool SendProc(SOCKET pSocket);//유저에게 데이터를 보낼때

void sendUniCast(st_SESSION* session, char* _header, char* _Msg, int size);
void sendBroadCast(st_SESSION* session, char* _header, char* _Msg, int size);

void sendUniCast(st_SESSION* session, CSerealBuffer* pPacket);//직렬화 버퍼버전 유니캐스트
void sendBroadCast(st_SESSION* session, CSerealBuffer* pPacket);//직렬화 버퍼 버전 브로드캐스트

bool PacketProc(st_SESSION* pSession, BYTE byPacketType, CSerealBuffer* pPacket);//패킷 전송 과정

void Disconnect(st_SESSION* session);//연결끊기 Session _Live를 false로 바꿔죽인다.
void Disconnect_Clean();//네트워크 마지막에 죽은세션을 정리한다.

void netSelectSocket(SOCKET* pTableSocket, FD_SET* pReadSet, FD_SET* pWriteSet);

//오류 출력 함수 밑 로그저장
void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);
void log_msg(int _wsaError, int _line, const char* _file);

st_SESSION* Find_Session(SOCKET sock);
st_PLAYER* Find_Player(DWORD Player_Id);

unordered_map<SOCKET, st_SESSION*> Session_Map;//접속한 유저리스트
unordered_map<DWORD, st_PLAYER*> g_Player_Map;//플레이어 리스트

//vector<st_PLAYER*>g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
list<st_PLAYER*>g_Sector[100][100];//섹터 관리용 리스트
list<DWORD>g_Delete_list;//삭제용 아이디목록

CMemoryPool<st_SESSION> _SessionPool(300, FALSE);//세션 오브젝트 풀
CMemoryPool<st_PLAYER> _PlayerPool(300, FALSE);

//모니터링 출력용 함수

#include "fun_Message.h"

//서버에 받아들이기 위한 리슨소켓
SOCKET listen_sock;

//확인용 함수데이터 저장
int User_Id_Count = 1;

int LogicTime = 0;
DWORD dwTick = timeGetTime();
DWORD dwCurtick = timeGetTime();
DWORD dwStarttick = timeGetTime();
BOOL g_bShutdown = false;

int main()
{
	timeBeginPeriod(1);
	init_Game();//게임 초기화
	init_Sock();//서버통신 소켓 초기화

	while (!g_bShutdown) {
		NetWork();//네트워크
		Update();//로직
		ServerControl();//서버 키보드컨트롤
		Monitor();//모니터링값 콘솔 출력
	}
}

void init_Game()
{
	
	//한글세팅
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");
};

void ServerControl() 
{

};

void Monitor()
{

};

void NetWork()
{
	FD_SET rset;
	FD_SET wset;
	SOCKET UserTable_SOCKET[FD_SETSIZE] = { INVALID_SOCKET, };
	int iSocketCount = 0;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(listen_sock, &rset);//리슨소켓을 rset소켓에 세팅한다
	UserTable_SOCKET[iSocketCount] = listen_sock;
	
	iSocketCount++;

	//모든 접속중인 클라이언트에 대해 SOCKET을 체크한다.
	st_SESSION* pSession;
	for (auto session_it = Session_Map.begin(); session_it != Session_Map.end();)
	{
		pSession = session_it->second;
		FD_SET(pSession->_Sock, &rset);//반응이 있는 ListenSocket을 모두 넣는다.

		if (pSession->_SendBuf.GetUseSize() > 0)//세션 버퍼에 보내야하는 데이터가 0이상이라면 FD세팅을 한다.
		{
			FD_SET(pSession->_Sock, &wset);
		}
		iSocketCount++;
		session_it++;

		if (FD_SETSIZE <= iSocketCount)
		{
			netSelectSocket(UserTable_SOCKET,&rset,&wset);
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			memset(UserTable_SOCKET, 0, sizeof(SOCKET) * FD_SETSIZE);

			FD_SET(listen_sock, &rset);
			UserTable_SOCKET[0] = listen_sock;

			iSocketCount = 1;
		}
	}

	if (iSocketCount > 0)
	{
		netSelectSocket(UserTable_SOCKET, &rset, &wset);
		memset(UserTable_SOCKET, 0, sizeof(SOCKET) * FD_SETSIZE);
	}
	
	Disconnect_Clean();//연결이 끊긴 녀석들을 모두제거한다.
};

void netSelectSocket(SOCKET* pTableSocket, FD_SET* pReadSet, FD_SET* pWriteSet)
{
	int retSelect;
	timeval times;
	times.tv_sec = 0;
	times.tv_usec = 0;

	int iResult;
	int iCnt;
	bool bProcFlag;

	//64개 이상의 소켓에선 본 함수를 여러번 호출하면서 소켓을 확인해야하므로
	//타임아웃을 0으로 잡아야한다. 그렇지 않으면 뒤쪽 소켓들 검사 타이밍이 점점 늦어진다.
	iResult = select(0, pReadSet, pWriteSet, 0, &times);

	if (iResult < 0)
	{
		for (int iCnt = 0; iResult > 0 && iCnt < FD_SETSIZE; ++iCnt)
		{
			bProcFlag = true;
			if (pTableSocket[iCnt] == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pTableSocket[iCnt], pWriteSet))
			{
				--iResult;
				bProcFlag = SendProc(pTableSocket[iCnt]);
			}

			if (FD_ISSET(pTableSocket[iCnt], pReadSet))
			{
				--iResult;
				if (bProcFlag)
				{
					if (pTableSocket[iCnt] == listen_sock) 
					{
						AcceptProc();
					}
					else if (pTableSocket[iCnt] != listen_sock)
					{
						RecvProc(pTableSocket[iCnt]);
					}
				}
			}
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		wprintf(L"SOCKET_ERROR");
	}

};

void Update()
{
	//50fps 간단 로직
	LogicTime = timeGetTime() - dwStarttick;

	if (LogicTime < 20)//전 시점에서 20ms가 지나지 않았다면 로직은 수행하지 않는다.
		return;
	else //20미리 이상 지났다면 다시 시간초기화
		dwStarttick = timeGetTime();

	dwCurtick = timeGetTime();

	//모든 연결 세션을 돌면서 로직을 수행한다.
	st_SESSION* UtmpSession;
	for (auto _Session_it = Session_Map.begin(); _Session_it != Session_Map.end(); ++_Session_it)
	{
		UtmpSession = _Session_it->second;
		st_PLAYER* pPlayer;
		pPlayer = Find_Player(UtmpSession->_Id);
		if (pPlayer == nullptr)
		{
			continue;
		}
		
		if (dwCurtick - UtmpSession->_LastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)//30초 이상 지난 세션은 제거한다.
		{
			Disconnect(UtmpSession);
			continue;
		}

		if (pPlayer->_Direction_check != ON_MOVE || pPlayer->_Live != true)
		{
			//움직이지 않아도 되는 세션이거나
			continue;//죽어있는 세션이라면 계산을 하지 않는다.
		}

		if (pPlayer->_HP <= 0)//Hp가 전부달아버린 세션이라면 세션을 죽이고 로직계산을 하지 않는다.
		{
			Disconnect(UtmpSession);
			continue;
		}

		//범위를 초과하고 있는 세션은 더이상 로직에서 움직이게 하지 않는다.
		if (pPlayer->_X < dfRANGE_MOVE_LEFT
			|| pPlayer->_X > dfRANGE_MOVE_RIGHT
			|| pPlayer->_Y < dfRANGE_MOVE_TOP
			|| pPlayer->_Y > dfRANGE_MOVE_BOTTOM)
		{
			continue;
		}

		//살아있으면서 움직이고 있는 세션이라면 원하는 방향으로 값을 이동시킨다.
		switch (pPlayer->_Direction)
		{
			case dfPACKET_MOVE_DIR_LL:
			{
				pPlayer->_X -= defualt_MOVE_X;
	#ifdef df_LOG
				printf("gameRun:LL # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_LU:
			{
				pPlayer->_X -= defualt_MOVE_X;
				pPlayer->_Y -= defualt_MOVE_Y;
	#ifdef df_LOG
				printf("gameRun:LU # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_UU:
			{
				pPlayer->_Y -= defualt_MOVE_Y;
	#ifdef df_LOG
				printf("gameRun:UU # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_RU:
			{
				pPlayer->_X += defualt_MOVE_X;
				pPlayer->_Y -= defualt_MOVE_Y;
	#ifdef df_LOG
				printf("gameRun:RU # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_RR:
			{
				pPlayer->_X += defualt_MOVE_X;
	#ifdef df_LOG
				printf("gameRun:RR # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_RD:
			{
				pPlayer->_X += defualt_MOVE_X;
				pPlayer->_Y += defualt_MOVE_Y;
	#ifdef df_LOG
				printf("gameRun:RD # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_DD:
			{
				pPlayer->_Y += defualt_MOVE_Y;
	#ifdef df_LOG
				printf("gameRun:DD # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
			case dfPACKET_MOVE_DIR_LD:
			{
				pPlayer->_X -= defualt_MOVE_X;
				pPlayer->_Y += defualt_MOVE_Y;
	#ifdef df_LOG
				printf("gameRun:LD # SessionID:%d / X:%d / Y:%d\n", Login_All_tmpSession->_Id, Login_All_tmpSession->_X, Login_All_tmpSession->_Y);
	#endif
			}
			break;
		}
	}
};

bool PacketProc(st_SESSION* pSession, BYTE byPacketType, CSerealBuffer* pPacket)
{
	switch (byPacketType)
	{
		case dfPACKET_SC_CREATE_MY_CHARACTER:
			return netPacketProc_SC_CREATE_MY_CHARACTER(pSession, pPacket);
			break;
		case dfPACKET_SC_CREATE_OTHER_CHARACTER:
			return netPacketProc_SC_CREATE_OTHER_CHARACTER(pSession, pPacket);
			break;
		case dfPACKET_SC_DELETE_CHARACTER:
			return netPacketProc_DELETE_CHARACTER(pSession, pPacket);
			break;
		case dfPACKET_CS_MOVE_START:
			return netPacketProc_MOVE_START(pSession, pPacket);
			break;
		case dfPACKET_CS_MOVE_STOP:
			return netPacketProc_MOVE_STOP(pSession, pPacket);
			break;
		case dfPACKET_CS_ATTACK1:
			return netPacketProc_ATTACK1(pSession, pPacket);
			break;
		case dfPACKET_CS_ATTACK2:
			return netPacketProc_ATTACK2(pSession, pPacket);
			break;
		case dfPACKET_CS_ATTACK3:
			return netPacketProc_ATTACK3(pSession, pPacket);
			break;
	}
	return true;
}

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

	//1.지금 연결된 유저 세션을 생성한다.
	//Session* NewSession = new Session;
	st_SESSION* Create_User_Session = _SessionPool.Alloc();
	st_PLAYER* Create_Player = _PlayerPool.Alloc();
	Create_User_Session->_Con_Addr = clientaddr;
	Create_User_Session->_SendBuf.ClearBuffer();
	Create_User_Session->_RecvBuf.ClearBuffer();
	//Create_User_Session->_Direction = default_Direction;
	//Create_User_Session->_Direction_check = ON_STOP;
	Create_User_Session->_Sock = client_sock;//sock을 세팅하고
	Create_User_Session->_Id = AccteptTotal;//ID세팅
	//Create_User_Session->_X = defualt_X_SET;//X세팅
	//Create_User_Session->_Y = defualt_Y_SET;//Y세팅
	Create_User_Session->_Live = true;//세션 생존플래그 0이면 사망
	//Create_User_Session->_HP = df_HP;

	Create_Player->pSession = Create_User_Session;
	Create_Player->_Id = AccteptTotal;
	Create_Player->_Direction = default_Direction;
	Create_Player->_Direction_check = ON_STOP;
	Create_Player->_X = defualt_X_SET;//X세팅
	Create_Player->_Y = defualt_Y_SET;//Y세팅
	Create_Player->_Live = true;//세션 생존플래그 0이면 사망
	Create_Player->_HP = df_HP;
	Create_Player->_cur_Pos._X = 0;
	Create_Player->_cur_Pos._Y = 0;
	Create_Player->_old_Pos._X = 0;
	Create_Player->_old_Pos._Y = 0;

	//2.생성된 유저 데이터 나에게 전송하기
	bool retProc;
	retProc = PacketProc(Create_User_Session, dfPACKET_SC_CREATE_MY_CHARACTER, nullptr);

	if (!retProc)
	{
		return;
	}

	//3.현재 리스트에 있는 유저들을 생성한 유저에게 모두 전송한다.
	st_SESSION* All_List_Session_it;
	for (auto _Session_it = Session_Map.begin(); _Session_it != Session_Map.end(); ++_Session_it)
	{
		All_List_Session_it = _Session_it->second;
		st_PLAYER* pPlayer;
		pPlayer = Find_Player(All_List_Session_it->_Id);
		if (pPlayer == nullptr)
		{
			continue;
		}

		if (All_List_Session_it->_Live == true)//살아있는 세션이라면?
		{
			st_dfPACKET_header st_other_header;
			st_other_header.byCode = PACKET_CODE;
			st_other_header.bySize = sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER);
			st_other_header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

			CSerealBuffer pPacket;
			pPacket.PutData((char*)&st_other_header,sizeof(st_dfPACKET_header));
			pPacket << pPlayer->_Id << pPlayer->_Direction << pPlayer->_X << pPlayer->_Y << pPlayer->_HP;
			sendUniCast(Create_User_Session, &pPacket);//나 자신을 제외하고 데이터전부인풋
		}
	}

	//4.생성된 세션 리스트에 넣기
	Session_Map.emplace(Create_User_Session->_Sock, Create_User_Session);
	//Session_List.push_back(NewSession);

	//5.생성된 나를 모든 유저에게 전달한다.
	CSerealBuffer pPacket;
	pPacket << Create_Player->_Id;
	pPacket << Create_Player->_Direction;
	pPacket << Create_Player->_X;
	pPacket << Create_Player->_Y;
	pPacket << Create_Player->_HP;

	PacketProc(Create_User_Session, dfPACKET_SC_CREATE_OTHER_CHARACTER, &pPacket);
};//접속유저가 새로 생겼을때



void RecvProc(SOCKET pSocket)//유저가 데이터를 보내왔을때
{
	st_SESSION* tmpSession;
	tmpSession =	Find_Session(pSocket);

	if (tmpSession == nullptr)
	{
		return;
	}

	tmpSession->_LastRecvTime = timeGetTime();//리시브할때마다 타임아웃 시간 업데이트 30초이상 지나면 끊어버릴것이다.

	int retRecv;
	retRecv = recv(tmpSession->_Sock, (char*)tmpSession->_RecvBuf.GetRearBufferPtr(),//버퍼에 꼽을수 있는만큼 꽂아버린다.
		tmpSession->_RecvBuf.DirectEnqueueSize(), 0);//한번에 넣을수있는 사이즈만큼 넣는다.

	if (retRecv == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값

		log_msg(wsaError, __LINE__, __FILE__);

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(tmpSession);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retRecv == 0)//그냥 연결종료 상태
	{
		int wsaError = WSAGetLastError();//소켓에러값

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(tmpSession);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
		return;
	}
	//retRecv < 0이상이라면 

	tmpSession->_RecvBuf.MoveRear(retRecv);//받은만큼 이동시킨다.

	CSerealBuffer cpPacket;//데이터 전송을 위한 직렬화 버퍼
	while (1)
	{
		if (tmpSession->_RecvBuf.GetUseSize() < sizeof(st_dfPACKET_header))//사용할수 있는사이즈가 헤더이하라면
			break;

		st_dfPACKET_header st_recv_header;
		int retPeek;
		retPeek = tmpSession->_RecvBuf.Peek((char*)&st_recv_header, sizeof(st_dfPACKET_header));//헤더크기만큼 꺼내기

		if (retPeek != sizeof(st_dfPACKET_header))//값이 다르면 애초에 박살나는것이기 때문에 브레이크
		{
			Disconnect(tmpSession);
			break;
		}

		if (st_recv_header.byCode != PACKET_CODE)//패킷코드 검사 패킷코드와 다르면 패킷코드가 발견될때까지 데이터를 폐기시켜야한다.
		{//일단 잘못된 놈이므로 사실 끊어버려도 그만이라 일단 끊었음
			Disconnect(tmpSession);
			break;
		}

		if (tmpSession->_RecvBuf.GetUseSize() < sizeof(st_dfPACKET_header) + st_recv_header.bySize)//헤더에 적힌 사이즈 보다 작으므로 더 받는절차를 거쳐야하므로 정지한다.
			break;

		//헤더에서 읽어온 크기만큼 버퍼에 존재한다면?
		tmpSession->_RecvBuf.MoveFront(sizeof(st_dfPACKET_header));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.		

		if (st_recv_header.byCode != PACKET_CODE)//패킷코드가 잘못되었으므로 조작된데이터다
		{
			//tmpSesion->_RecvBuf.MoveFront(st_recv_header.bySize);//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.		
			//continue;//여기서 연결을 끊어버리는게 맞지않나 맞지않는 패킷을 보낸거아냐
			Disconnect(tmpSession);
			break;
		}

		//직렬화버퍼 작업
		//직렬화 버퍼 사이즈(1400디폴트)보다 클경우 리사이즈를 해야할 필요가 있다.
		//아직 작업은 안되있음

		int retSwitchDeq;
		retSwitchDeq = tmpSession->_RecvBuf.Dequeue(cpPacket.GetBufferPtr(), st_recv_header.bySize);//사이즈만큼 직렬화 버퍼에 디큐한다.

		if (retSwitchDeq != st_recv_header.bySize)//헤더에 적힌 사이즈만큼 있다고 판별된 상황에서 사이즈만큼 가져오지 못한다면 버퍼가 깨진것이므로 연결을 끊고 세션을 파괴한다.
		{
			Disconnect(tmpSession);
			return;
		}
		
		int retCS;
		retCS = cpPacket.MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

		if (retCS != retSwitchDeq)//직렬화 버퍼가 이동하지 못했다면 파괴한다.
		{
			Disconnect(tmpSession);
			return;
		}

		bool retPacket;
		retPacket = PacketProc(tmpSession, st_recv_header.byType, &cpPacket);//패킷절차가 실패했다면?
		if (!retPacket)
		{
			Disconnect(tmpSession);//데이터가 꼬여서 실패한것이므로 연결을 끊어버린다.
			return;
		}

		cpPacket.Clear();//직렬화 버퍼 재사용을 위한 초기화
	}

};

bool SendProc(SOCKET pSocket)
{
	st_SESSION* tmpSession;
	tmpSession = Find_Session(pSocket);

	if (tmpSession == nullptr)
	{
		return false;
	}

	int retSend;
	//읽기 버퍼 포인터에서 보낼수있는 크기만큼 최대한 보낸다.
	retSend = send(tmpSession->_Sock, tmpSession->_SendBuf.GetFrontBufferPtr(), tmpSession->_SendBuf.DirectDequeueSize(), 0);

	if (retSend == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값
		log_msg(wsaError, __LINE__, __FILE__);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(tmpSession);//연결종료처리
			return false;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retSend == 0)//연결 종료상태
	{
		int wsaError = WSAGetLastError();//소켓에러값
		//log_msg(wsaError);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(tmpSession);//연결종료처리
			return false;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}

	if (retSend != tmpSession->_SendBuf.DirectDequeueSize())//긁어온것과 보낸양이 다르다? 링버퍼가 박살나서 이상하게 만들어진것이다 연결을 종료하고 로그를 남겨야한다.
	{
		Disconnect(tmpSession);
		return false;
	}

	int retMF;
	retMF = tmpSession->_SendBuf.MoveFront(retSend);//센드버퍼에 쌓은만큼 포인터를 이동시킨다.

	if(retMF!= retSend)
	{
		Disconnect(tmpSession);//연결종료처리
		return false;//링버퍼가 꺠졌다는 뜻이므로 종료
	}

	return true;
};//유저에게 데이터를 보낼때

void sendUniCast(st_SESSION* session, char* _header, char* _Msg, int size)
{
	int retUni;

	if (session->_Live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}
	if (session->_SendBuf.GetFreeSize() >= size)//sendbuf에 16바이트 이상이 남아있다면 인큐시킨다.
	{
		retUni = session->_SendBuf.Enqueue(_header, sizeof(st_dfPACKET_header));//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != sizeof(st_dfPACKET_header))
		{
			Disconnect(session);
			return;
		}

		retUni = session->_SendBuf.Enqueue(_Msg, size);//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != size)
		{
			Disconnect(session);
			return;
		}
	}

};//특정유저에게만 보내기

void sendUniCast(st_SESSION* session, CSerealBuffer* pPacket)
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

void sendBroadCast(st_SESSION* session, CSerealBuffer *pPacket)
{
	int retBro;
	st_SESSION* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	st_SESSION* Broad_Sesion;
	for (auto session_it = Session_Map.begin(); session_it != Session_Map.end(); ++session_it)
	{
		Broad_Sesion = session_it->second;
		if (stmp != nullptr && stmp->_Id == Broad_Sesion->_Id)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if (Broad_Sesion->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}
		//각세션의 센드버퍼에 enqueue시킨다.
		if (Broad_Sesion->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = Broad_Sesion->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != pPacket->GetUseSize())//센드버퍼 자체가 고장났거나 송신버퍼가 가득찬경우 이므로 디스커넥트하고 끝내버린다.
			{
				Disconnect(Broad_Sesion);
			}
		}
	}
}
void sendBroadCast(st_SESSION* session, char* _header, char* _Msg, int size)
{
	int retBro;
	st_SESSION* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	for (auto session_it = Session_Map.begin(); session_it != Session_Map.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->_Id == (*session_it).second->_Id)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if ((*session_it).second->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}
		//각세션의 센드버퍼에 enqueue시킨다.
		if ((*session_it).second->_SendBuf.GetFreeSize() >= size)//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = (*session_it).second->_SendBuf.Enqueue(_header, sizeof(st_dfPACKET_header));//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != sizeof(st_dfPACKET_header))
			{
				Disconnect((*session_it).second);
			}

			retBro = (*session_it).second->_SendBuf.Enqueue(_Msg, size);//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != size)
			{
				Disconnect((*session_it).second);
			}
		}

	}
};//모든 유저에게 보내기

void Disconnect(st_SESSION* session)//연결 끊기 함수
{
	//auto sItem = Session_Map.find(session->_Sock);
	//
	//if (sItem != Session_Map.end())//일치하는 녀석이 존재한다면
	//{
	//	(*sItem).second->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
	//}

	auto pItem = g_Player_Map.find(session->_Id);
	if (pItem != g_Player_Map.end())//일치하는 녀석이 존재한다면
	{
		(*pItem).second->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
		(*pItem).second->pSession->_Live = false;
		g_Delete_list.push_back((*pItem).second->_Id);
	}
};

void Disconnect_Clean()//안정성을 위해 디스커넥트된 개체를 네트워크 마지막시점에 정리한다.
{
	//st_SESSION* _Session_Object;
	//for (auto _Session_it = Session_Map.begin(); _Session_it != Session_Map.end();)
	//{
	//	_Session_Object = _Session_it->second;
	//	if (_Session_Object->_Live == false) //세션 특정객체가 죽어있다면
	//	{
	//		PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);
	//		closesocket(_Session_Object->_Sock);
	//		//delete _Session_Object->_RecvBuf;//세션 받기버퍼 제거
	//		//delete _Session_Object->_SendBuf;//세션 보내기 버퍼 제거
	//		//delete _Session_Object;//동적할당된 세션 제거
	//		_Session_Object->_SendBuf.ClearBuffer();
	//		_Session_Object->_RecvBuf.ClearBuffer();

	//		_SessionPool.Free(_Session_Object);//오브젝트풀에 세션리스트 반납
	//		_Session_it = Session_Map.erase(_Session_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
	//	}
	//	else
	//	{
	//		++_Session_it;//살아있는 객체라면 다음 리스트로 넘어간다.
	//	}
	//}

	//st_PLAYER* _Player_Object;
	//for (auto _Player_it = g_Player_Map.begin(); _Player_it != g_Player_Map.end();)
	//{
	//	_Player_Object = _Player_it->second;
	//	if (_Player_Object->_Live == false) //플레이어 특정객체가 죽어있다면
	//	{
	//		//PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);

	//		_PlayerPool.Free(_Player_Object);//오브젝트풀에 플레이어리스트 반납
	//		_Player_it = g_Player_Map.erase(_Player_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
	//	}
	//	else
	//	{
	//		++_Player_it;//살아있는 객체라면 다음 리스트로 넘어간다.
	//	}
	//}
	
	DWORD _retDel;
	st_PLAYER* _Player_Object;
	st_SESSION* _Session_Object;
	for (auto _Delete_it = g_Delete_list.begin(); _Delete_it != g_Delete_list.end(); ++_Delete_it)
	{
		_retDel = *_Delete_it;
		_Player_Object = Find_Player(_retDel);
		if (_Player_Object == nullptr)
		{
			wprintf(L"LOG : Disconnect_Clean %d 유저 없는데 검색중\n", _retDel);
			continue;
		}
		_Session_Object = _Player_Object->pSession;//받은 세션 리스트 가져오기

		PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);//제거되는 세션아이디 전달
		closesocket(_Session_Object->_Sock);//소켓클로즈
		_Session_Object->_SendBuf.ClearBuffer();//링버퍼 초기화
		_Session_Object->_RecvBuf.ClearBuffer();//링버퍼 초기화
		Session_Map.erase(_Session_Object->_Sock);//맵에서 해당세션제거
		_SessionPool.Free(_Session_Object);//오브젝트풀에 세션리스트 반납
		
		g_Player_Map.erase(_Player_Object->_Id);//플레이어 맵에서 해당 플레이어 제거
		_PlayerPool.Free(_Player_Object);//오브젝트풀에 플레이어리스트 반납
	}

	g_Delete_list.clear();//모든유저가 삭제됬으므로 제거한다.

	//DWORD Session_id;
	//for (auto _Session_it = g_Delete_ID.begin(); _Session_it != g_Delete_ID.end();)
	//{
	//	Session_id = *_Session_it;

	//	auto item = Session_Map.find(Session_id);
	//	if (item != Session_Map.end())//일치하는 녀석이 존재한다면
	//	{
	//		PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);
	//		closesocket(_Session_Object->_Sock);
	//		_SessionPool.Free(_Session_Object);//오브젝트풀에 세션리스트 반납
	//		_Session_it = g_Delete_ID.erase(_Session_it);
	//		//_Session_it = Session_Map.erase(_Session_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
	//	}

	//	if (_Session_Object->_Live == false) //세션 특정객체가 죽어있다면
	//	{
	//		PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);
	//		closesocket(_Session_Object->_Sock);
	//		//delete _Session_Object->_RecvBuf;//세션 받기버퍼 제거
	//		//delete _Session_Object->_SendBuf;//세션 보내기 버퍼 제거
	//		//delete _Session_Object;//동적할당된 세션 제거

	//		_SessionPool.Free(_Session_Object);//오브젝트풀에 세션리스트 반납
	//		_Session_it = Session_Map.erase(_Session_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
	//	}
	//	else
	//	{
	//		++_Session_it;//살아있는 객체라면 다음 리스트로 넘어간다.
	//	}
	//}

}

void init_Sock()
{
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
	serveraddr.sin_port = htons(SERVERPORT);

	int retOut;
	retOut = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retOut == SOCKET_ERROR)err_quit(L"bind()");
#ifdef df_LOG
	else printf("Bind OK # PORT %d\n", SERVERPORT);
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

void RecvProc(st_SESSION* session)//유저가 데이터를 보내왔을때
{
	st_SESSION* tmpSesion;
	tmpSesion = session;

	int retRecv;
	retRecv = recv(tmpSesion->_Sock, (char*)tmpSesion->_RecvBuf.GetRearBufferPtr(),//버퍼에 꼽을수 있는만큼 꽂아버린다.
		tmpSesion->_RecvBuf.DirectEnqueueSize(), 0);//한번에 넣을수있는 사이즈만큼 넣는다.

	if (retRecv == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값

		log_msg(wsaError, __LINE__, __FILE__);

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retRecv == 0)//그냥 연결종료 상태
	{
		int wsaError = WSAGetLastError();//소켓에러값

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
		return;
	}

	tmpSesion->_RecvBuf.MoveRear(retRecv);//받은만큼 이동시킨다.

	CSerealBuffer cpPacket;

	while (1)
	{
		int retSwitchDeq;
		int retCS;
		int retPeek;

		if (tmpSesion->_RecvBuf.GetUseSize() < sizeof(st_dfPACKET_header))//사용할수 있는사이즈가 헤더이하라면
			break;

		st_dfPACKET_header st_recv_header;
		retPeek = tmpSesion->_RecvBuf.Peek((char*)&st_recv_header, sizeof(st_dfPACKET_header));//헤더크기만큼 꺼내기

		if (retPeek != sizeof(st_dfPACKET_header))//값이 다르면 애초에 박살나는것이기 때문에 브레이크
		{
			Disconnect(tmpSesion);
			break;
		}

		if (tmpSesion->_RecvBuf.GetUseSize() < sizeof(st_dfPACKET_header) + st_recv_header.bySize)//헤더에 적힌 사이즈 보다 작으므로 더 받는절차를 거쳐야하므로 정지한다.
			break;

		//헤더에서 읽어온 크기만큼 버퍼에 존재한다면?
		tmpSesion->_RecvBuf.MoveFront(sizeof(st_dfPACKET_header));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.		

		if (st_recv_header.byCode != PACKET_CODE)//패킷코드가 잘못되었으므로 조작된데이터다
		{
			//tmpSesion->_RecvBuf.MoveFront(st_recv_header.bySize);//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.		
			//continue;//여기서 연결을 끊어버리는게 맞지않나 맞지않는 패킷을 보낸거아냐
			Disconnect(tmpSesion);
			break;
		}

		//직렬화버퍼 작업
		//직렬화 버퍼 사이즈(1400디폴트)보다 클경우 리사이즈를 해야할 필요가 있다.
		//아직 작업은 안되있음

		retSwitchDeq = tmpSesion->_RecvBuf.Dequeue(cpPacket.GetBufferPtr(), st_recv_header.bySize);//사이즈만큼 직렬화 버퍼에 디큐한다.

		if (retSwitchDeq != st_recv_header.bySize)//헤더에 적힌 사이즈만큼 있다고 판별된 상황에서 사이즈만큼 가져오지 못한다면 버퍼가 깨진것이므로 연결을 끊고 세션을 파괴한다.
		{
			Disconnect(tmpSesion);
			return;
		}

		retCS = cpPacket.MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

		if (retCS != retSwitchDeq)//직렬화 버퍼가 이동하지 못했다면 파괴한다.
		{
			Disconnect(tmpSesion);
			return;
		}

		PacketProc(tmpSesion, st_recv_header.byType, &cpPacket);

		cpPacket.Clear();//직렬화 버퍼 재사용을 위한 초기화
	}

};

void SendProc(st_SESSION* session)
{
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

st_SESSION* Find_Session(SOCKET sock)
{
	auto S_Iter = Session_Map.find(sock);

	if (S_Iter != Session_Map.end())//찾음
	{
		return S_Iter->second;
	}
	else //못찾음
	{
		return nullptr;
	}
};

st_PLAYER* Find_Player(DWORD Player_Id)
{
	auto S_Iter = g_Player_Map.find(Player_Id);
	if (S_Iter != g_Player_Map.end())//찾음
	{
		return S_Iter->second;
	}
	else //못찾음
	{
		return nullptr;
	}
};