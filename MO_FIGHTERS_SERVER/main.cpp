
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
#include "TRingBuffer.h"
#include "CList.h"
#include "Server_Enum.h"
#include "Packet_Enum.h"

#define df_LOG//로그 켜고싶으면 세팅

using namespace std;

void NetWork();
void Logic();
//서버 센드 리시브함수
void init_sock();
void AcceptProc();//접속유저가 새로 생겼을때
void RecvProc(Session* session);//유저가 데이터를 보내왔을때
void SendProc(Session* session);//유저에게 데이터를 보낼때
void sendUniCast(Session* session, char* _Msg, int size);//특정유저에게만 보내기
void sendBroadCast(Session* session, char* _Msg, int size);//모든 유저에게 보내기

void Disconnect(Session* session);
void Disconnect_Clean();

//오류 출력 함수 밑 로그저장
void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);
void log_msg(int wasError);

CList<Session*> Session_List;//접속한 유저리스트
int Session_Total = 0;//세션 데이터

//서버에 받아들이기 위한 리슨소켓
SOCKET listen_sock;

//확인용 함수데이터 저장
FD_SET rset;
FD_SET wset;
int User_Id_Count = 1;

static DWORD frameDelta = 0;
static DWORD lastTime = GetTickCount64();

int main()
{
	timeBeginPeriod(1);
	init_sock();
	int LogicTime = 0;
	int SleepTime;
	DWORD dwTick = timeGetTime();
	DWORD dwStarttick = timeGetTime();

	while (1) {
		dwStarttick = timeGetTime();

		NetWork();//네트워크

		Logic();//로직

		//fc++;
		if (timeGetTime() - dwTick >= 1000)
		{
			//sprintf_s(output, "FPS:%d\n", fc);
			//fc = 0;
			dwTick = timeGetTime();
			//OutputDebugStringA(output);
		}

		LogicTime = timeGetTime() - dwStarttick;

		SleepTime = _df_FPS - LogicTime > _df_FPS ? 0 : _df_FPS - LogicTime;
		Sleep(SleepTime);
	}
}

void NetWork()
{
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(listen_sock, &rset);//리슨소켓을 rset소켓에 세팅한다

	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		FD_SET((*session_it)->_Sock, &rset);//반응이 있는 ListenSocket을 모두 넣는다.

		if ((*session_it)->_SendBuf->GetUseSize() > 0)//세션 버퍼에 보내야하는 데이터가 0이상이라면 FD세팅을 한다.
		{
			FD_SET((*session_it)->_Sock, &wset);
		}
	}

	int retSelect;
	timeval times;
	times.tv_sec = 0;
	times.tv_usec = 0;

	retSelect = select(0, &rset, &wset, NULL, &times);//반응이 있는 소켓이 있는지 셀렉트로 확인

	if (retSelect == SOCKET_ERROR)//select함수 이상발생시 아예종료
	{
		err_quit(L"select()");
	}

	//소켓에 이상이 없으면서
	if (retSelect > 0)//반응이 있는 소켓이 있다면?
	{
		if (FD_ISSET(listen_sock, &rset))//리슨소켓에 반응이 있다면
		{
			AcceptProc();
		}

		//현재 세션리스트중에 반응이 있는 소켓이 있는지 확인하기위해 모든 리스트를 돌면서 확인한다.
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{

			if (FD_ISSET((*session_it)->_Sock, &rset))//반응이 있는 읽기 소켓이 있다면?
			{
				RecvProc(*session_it);//Recv Proc를 실행한다.
			}

			if (FD_ISSET((*session_it)->_Sock, &wset))//반응가능한 쓰기소켓이 있다면
			{
				SendProc(*session_it);
			}
		}
	}
	Disconnect_Clean();//연결이 끊긴 녀석들을 모두제거한다.
};

void Logic()
{
	for (auto _Session_it = Session_List.begin(); _Session_it != Session_List.end(); ++_Session_it)
	{
		if ((*_Session_it)->_Direction_check != ON_MOVE || (*_Session_it)->_Live != true)
		{
			//움직이지 않아도 되는 세션이거나
			continue;//죽어있는 세션이라면 계산을 하지 않는다.
		}

		if ((*_Session_it)->_HP <= 0)//Hp가 전부달아버린 세션이라면 세션을 죽이고 연결하지 않는다.
		{
			Disconnect((*_Session_it));
			continue;
		}

		//범위를 초과하고 있는 녀석은 더이상 로직에서 움직이게 하지 않는다.
		if ((*_Session_it)->_X < dfRANGE_MOVE_LEFT 
			|| (*_Session_it)->_X > dfRANGE_MOVE_RIGHT
			|| (*_Session_it)->_Y < dfRANGE_MOVE_TOP
			|| (*_Session_it)->_Y > dfRANGE_MOVE_BOTTOM)
		{
			continue;
		}

		//살아있으면서 움직이고 있는 세션이라면 원하는 방향으로 값을 이동시킨다.
		switch ((*_Session_it)->_Direction)
		{
			case dfPACKET_MOVE_DIR_LL:
			{
				(*_Session_it)->_X -= defualt_MOVE_X;
#ifdef df_LOG
				printf("gameRun:LL # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_LU:
			{
				(*_Session_it)->_X -= defualt_MOVE_X;
				(*_Session_it)->_Y -= defualt_MOVE_Y;
#ifdef df_LOG
				printf("gameRun:LU # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_UU:
			{
				(*_Session_it)->_Y -= defualt_MOVE_Y;
#ifdef df_LOG
				printf("gameRun:UU # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_RU:
			{
				(*_Session_it)->_X += defualt_MOVE_X;
				(*_Session_it)->_Y -= defualt_MOVE_Y;
#ifdef df_LOG
				printf("gameRun:RU # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_RR:
			{
				(*_Session_it)->_X += defualt_MOVE_X;
#ifdef df_LOG
				printf("gameRun:RR # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_RD:
			{
				(*_Session_it)->_X += defualt_MOVE_X;
				(*_Session_it)->_Y += defualt_MOVE_Y;
#ifdef df_LOG
				printf("gameRun:RD # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_DD:
			{
				(*_Session_it)->_Y += defualt_MOVE_Y;
#ifdef df_LOG
				printf("gameRun:DD # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
			case dfPACKET_MOVE_DIR_LD:
			{
				(*_Session_it)->_X -= defualt_MOVE_X;
				(*_Session_it)->_Y += defualt_MOVE_Y;
#ifdef df_LOG
				printf("gameRun:LD # SessionID:%d / X:%d / Y:%d\n", (*_Session_it)->_Id, (*_Session_it)->_X, (*_Session_it)->_Y);
#endif
			}
			break;
		}
	}
};

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
		log_msg(wsaError);
		closesocket(client_sock);
		return;
	}

	int AccteptTotal = User_Id_Count++;//여기서 계속 +하면서 ID 값을 새로 생성한다.
#ifdef df_LOG
	printf("Conncet # IP %d / SessionID %d\n", 127, AccteptTotal);
#endif
	//1.지금 연결된 유저 세션을 생성한다.
	Session* NewSession = new Session;
	NewSession->_SendBuf = new TRingBuffer;
	NewSession->_RecvBuf = new TRingBuffer;
	NewSession->_Direction = default_Direction;
	NewSession->_Direction_check = ON_STOP;
	NewSession->_Sock = client_sock;//sock을 세팅하고
	NewSession->_Id = AccteptTotal;//ID세팅
	NewSession->_X = defualt_X_SET;//X세팅
	NewSession->_Y = defualt_Y_SET;//Y세팅
	NewSession->_Live = true;//세션 생존플래그 0이면 사망
	NewSession->_HP = df_HP;

	//2. 생성된유저에게 만들어졌다고 알린다.
	st_dfPACKET_header st_my_header;
	st_my_header.byCode = PACKET_CODE;
	st_my_header.bySize = sizeof(st_dfPACKET_SC_CREATE_MY_CHARACTER);
	st_my_header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

	st_dfPACKET_SC_CREATE_MY_CHARACTER st_my_create;
	st_my_create._Id = AccteptTotal;
	st_my_create._Direction = default_Direction;
	st_my_create.HP = defualt_HP;
	st_my_create._X = defualt_X_SET;
	st_my_create._Y = defualt_Y_SET;

	sendUniCast(NewSession, (char*)&st_my_header, sizeof(st_dfPACKET_header));
	sendUniCast(NewSession, (char*)&st_my_create, sizeof(st_dfPACKET_SC_CREATE_MY_CHARACTER));

	//3.현재 리스트에 있는 유저들을 생성한 유저에게 모두 전송한다.
	for (auto _Session_it = Session_List.begin(); _Session_it != Session_List.end(); ++_Session_it)
	{
		if ((*_Session_it)->_Live == true)//살아있는 세션이라면?
		{
			st_dfPACKET_header st_other_header;
			st_other_header.byCode = PACKET_CODE;
			st_other_header.bySize = sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER);
			st_other_header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

			st_dfPACKET_SC_CREATE_OTHER_CHARACTER st_other_create;
			st_other_create._Id = (*_Session_it)->_Id;
			st_other_create._Direction = (*_Session_it)->_Direction;
			st_other_create._X = (*_Session_it)->_X;
			st_other_create._Y = (*_Session_it)->_Y;
			st_other_create.HP = (*_Session_it)->_HP;
			sendUniCast(NewSession, (char*)&st_other_header, sizeof(st_dfPACKET_header));
			sendUniCast(NewSession, (char*)&st_other_create, sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER));
		}
	}

	Session_List.push_back(NewSession);//생선된 세션을 세션리스트에 넣는다.

	//5.생성된 나를 모든 유저에게 전달한다.
	st_dfPACKET_header st_my_other_header;
	st_my_other_header.byCode = PACKET_CODE;
	st_my_other_header.bySize = sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER);
	st_my_other_header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	st_dfPACKET_SC_CREATE_OTHER_CHARACTER st_my_other_create;
	st_my_other_create._Id = AccteptTotal;
	st_my_other_create._Direction = default_Direction;
	st_my_other_create.HP = defualt_HP;
	st_my_other_create._X = defualt_X_SET;
	st_my_other_create._Y = defualt_Y_SET;

	sendBroadCast(NewSession, (char*)&st_my_other_header, sizeof(st_dfPACKET_header));
	sendBroadCast(NewSession, (char*)&st_my_other_create, sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER));
#ifdef df_LOG
	printf("Create Character # SessionID %d  X:%d Y:%d\n", AccteptTotal, defualt_X_SET, defualt_Y_SET);
#endif
};//접속유저가 새로 생겼을때

void RecvProc(Session* session)//유저가 데이터를 보내왔을때
{
	Session* tmpSesion;
	tmpSesion = session;
	char buf[_df_Buffer_size];
	memset(buf, 0, sizeof(buf));//버퍼 clean
	int retRecv;
	retRecv = recv(tmpSesion->_Sock, (char*)buf, tmpSesion->_RecvBuf->GetFreeSize(), 0);//남아있는 Free사이즈만큼 긁어온다.

	if (retRecv == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값

		log_msg(wsaError);

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retRecv == 0)
	{
		int wsaError = WSAGetLastError();//소켓에러값
		log_msg(wsaError);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
		return;
	}
	int retRing = tmpSesion->_RecvBuf->Enqueue((char*)buf, retRecv);//받은만큼 인큐한후

	if (retRecv != retRing)
	{
		Disconnect(tmpSesion);
		return;
	}

	while (1)
	{
		if (tmpSesion->_RecvBuf->GetUseSize() < sizeof(st_dfPACKET_header))//사용할수 있는사이즈가 헤더이하라면
		{
			break;
		}

		st_dfPACKET_header st_recv_header;
		int retPeek = tmpSesion->_RecvBuf->Peek((char*)&st_recv_header, sizeof(st_dfPACKET_header));//헤더크기만큼 꺼내기

		if (retPeek != sizeof(st_dfPACKET_header))//값이 다르면 애초에 박살나는것이기 때문에 브레이크
		{
			Disconnect(tmpSesion);
			break;
		}

		if (tmpSesion->_RecvBuf->GetUseSize() < sizeof(st_dfPACKET_header) + st_recv_header.bySize)//헤더에 적힌 사이즈 보다 작으므로 처리를 할수가 없으므로
		{
			break;
		}
		//헤더에서 읽어온 크기만큼 버퍼에 존재한다면?
		tmpSesion->_RecvBuf->MoveFront(sizeof(st_dfPACKET_header));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.

		switch (st_recv_header.byType)//byType으로 판단한다.
		{
		int retSwitchDeq;
		case dfPACKET_CS_MOVE_START:
		{
			st_dfPACKET_CS_MOVE_START st_CS_MOVE_START;
			retSwitchDeq = tmpSesion->_RecvBuf->Dequeue((char*)&st_CS_MOVE_START, st_recv_header.bySize);//사이즈만큼 디큐한다.
			if (retSwitchDeq != st_recv_header.bySize)
			{
				Disconnect(tmpSesion);
				return;
			}

			if (tmpSesion->_X + dfERROR_RANGE < st_CS_MOVE_START._X
				|| tmpSesion->_X - dfERROR_RANGE > st_CS_MOVE_START._X//지금 X위치값 -50보다 들어온 위치값이 작다면 이동오류
				|| tmpSesion->_Y + dfERROR_RANGE < st_CS_MOVE_START._Y//지금 Y위치값 +50보다 들어온 위치가 더 크다면 이동오류
				|| tmpSesion->_Y - dfERROR_RANGE > st_CS_MOVE_START._Y)//지금 Y위치값 -50보다 들어온 위치가 더 작다면 이동오류
			{
				Disconnect(tmpSesion);
				return;
			}

			//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
			tmpSesion->_Direction = st_CS_MOVE_START._Direction;//바라보는 방향지정
			tmpSesion->_Direction_check = ON_MOVE;//이동중 이라고 변경
			tmpSesion->_X = st_CS_MOVE_START._X;//받은 크기값을 넣는다.
			tmpSesion->_Y = st_CS_MOVE_START._Y;//받은 크기값을 넣는다.

			//헤더설정
			st_dfPACKET_header st_SC_MOVE_START_HAEDER;
			st_SC_MOVE_START_HAEDER.byCode = PACKET_CODE;
			st_SC_MOVE_START_HAEDER.bySize = sizeof(st_dfPACKET_SC_MOVE_START);
			st_SC_MOVE_START_HAEDER.byType = dfPACKET_SC_MOVE_START;

			//데이터설정
			st_dfPACKET_SC_MOVE_START st_SC_MOVE_START;
			st_SC_MOVE_START._Id = tmpSesion->_Id;//어떤 녀석인지 ID설정
			st_SC_MOVE_START._Direction = tmpSesion->_Direction;//보는 방향지정
			st_SC_MOVE_START._X = tmpSesion->_X;//위치X
			st_SC_MOVE_START._Y = tmpSesion->_Y;//위치Y

			sendBroadCast(tmpSesion, (char*)&st_SC_MOVE_START_HAEDER, sizeof(st_dfPACKET_header));
			sendBroadCast(tmpSesion, (char*)&st_SC_MOVE_START, sizeof(st_dfPACKET_SC_MOVE_START));
#ifdef df_LOG
			printf("PACKET_MOVESTART # SessionID: %d / Direction:%d / X:%d / Y:%d\n", tmpSesion->_Id, tmpSesion->_Direction, tmpSesion->_X, tmpSesion->_Y);
#endif
		}
		break;
		case dfPACKET_CS_MOVE_STOP://정지
		{
			st_dfPACKET_CS_MOVE_STOP st_CS_MOVE_STOP;
			retSwitchDeq = tmpSesion->_RecvBuf->Dequeue((char*)&st_CS_MOVE_STOP, st_recv_header.bySize);//사이즈만큼 디큐한다.

			if (retSwitchDeq != st_recv_header.bySize)//사이즈만큼 가져오지 못한다면 버퍼가 깨진것이므로 연결을 끊고 세션을 파괴한다.
			{
				Disconnect(tmpSesion);
				return;
			}

			if (tmpSesion->_X + dfERROR_RANGE < st_CS_MOVE_STOP._X//지금 X위치값 +50보다 들어온 위치값이 크다면 이동오류
				|| tmpSesion->_X - dfERROR_RANGE > st_CS_MOVE_STOP._X//지금 X위치값 -50보다 들어온 위치값이 작다면 이동오류
				|| tmpSesion->_Y + dfERROR_RANGE < st_CS_MOVE_STOP._Y//지금 Y위치값 +50보다 들어온 위치가 더 크다면 이동오류
				|| tmpSesion->_Y - dfERROR_RANGE > st_CS_MOVE_STOP._Y)//지금 Y위치값 -50보다 들어온 위치가 더 작다면 이동오류
			{
				Disconnect(tmpSesion);
				return;
			}

			//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
			tmpSesion->_Direction = st_CS_MOVE_STOP._Direction;//바라보는 방향지정
			tmpSesion->_Direction_check = ON_STOP;//정지라고 값을 지정한다.
			tmpSesion->_X = st_CS_MOVE_STOP._X;//받은 크기값을 넣는다.
			tmpSesion->_Y = st_CS_MOVE_STOP._Y;//받은 크기값을 넣는다.

			//헤더설정
			st_dfPACKET_header st_SC_MOVE_STOP_HAEDER;
			st_SC_MOVE_STOP_HAEDER.byCode = PACKET_CODE;
			st_SC_MOVE_STOP_HAEDER.bySize = sizeof(st_dfPACKET_SC_MOVE_STOP);
			st_SC_MOVE_STOP_HAEDER.byType = dfPACKET_SC_MOVE_STOP;

			//데이터설정
			st_dfPACKET_SC_MOVE_STOP st_SC_MOVE_STOP;
			st_SC_MOVE_STOP._Id = tmpSesion->_Id;//어떤 녀석인지 ID설정
			st_SC_MOVE_STOP._Direction = tmpSesion->_Direction;//보는 방향지정
			st_SC_MOVE_STOP._X = tmpSesion->_X;//위치X
			st_SC_MOVE_STOP._Y = tmpSesion->_Y;//위치Y

			sendBroadCast(tmpSesion, (char*)&st_SC_MOVE_STOP_HAEDER, sizeof(st_dfPACKET_header));
			sendBroadCast(tmpSesion, (char*)&st_SC_MOVE_STOP, sizeof(st_dfPACKET_SC_MOVE_STOP));
#ifdef df_LOG
			printf("PACKET_MOVESTOP # SessionID: %d / Direction:%d / X:%d / Y:%d\n", tmpSesion->_Id, tmpSesion->_Direction, tmpSesion->_X, tmpSesion->_Y);
#endif
		}
		break;
		case dfPACKET_CS_ATTACK1://공격
		{
			st_dfPACKET_CS_ATTACK1 st_CS_ATTACK1;
			retSwitchDeq = tmpSesion->_RecvBuf->Dequeue((char*)&st_CS_ATTACK1, st_recv_header.bySize);//사이즈만큼 디큐한다.
			if (retSwitchDeq != st_recv_header.bySize)
			{
				Disconnect(tmpSesion);
				return;
			}

			//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
			tmpSesion->_Direction = st_CS_ATTACK1._Direction;//공격하는 방향
			tmpSesion->_Direction_check = ON_STOP;//이동중 이라고 변경
			tmpSesion->_X = st_CS_ATTACK1._X;//받은 크기값을 넣는다.
			tmpSesion->_Y = st_CS_ATTACK1._Y;//받은 크기값을 넣는다.

			//헤더설정
			st_dfPACKET_header st_SC_ATTACK1_HAEDER;
			st_SC_ATTACK1_HAEDER.byCode = PACKET_CODE;
			st_SC_ATTACK1_HAEDER.bySize = sizeof(st_dfPACKET_SC_ATTACK1);
			st_SC_ATTACK1_HAEDER.byType = dfPACKET_SC_ATTACK1;

			//데이터설정
			st_dfPACKET_SC_ATTACK1 st_SC_ATTACK1;
			st_SC_ATTACK1._Id = tmpSesion->_Id;//어떤 녀석인지 ID설정
			st_SC_ATTACK1._Direction = tmpSesion->_Direction;//보는 방향지정
			st_SC_ATTACK1._X = tmpSesion->_X;//위치X
			st_SC_ATTACK1._Y = tmpSesion->_Y;//위치Y

			sendBroadCast(tmpSesion, (char*)&st_SC_ATTACK1_HAEDER, sizeof(st_dfPACKET_header));
			sendBroadCast(tmpSesion, (char*)&st_SC_ATTACK1, sizeof(st_dfPACKET_SC_ATTACK1));

			switch (tmpSesion->_Direction)
			{
				case dfPACKET_MOVE_DIR_LL:
				{
					for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
					{
						if ((*session_it)->_Live == false)
						{
							continue;//죽어있는 패킷 무시
						}

						if (tmpSesion->_Id== (*session_it)->_Id)
						{
							continue;
						}

						if ((tmpSesion->_X - dfATTACK1_RANGE_X <= (*session_it)->_X
							&& tmpSesion->_X >= (*session_it)->_X)

							&& ((tmpSesion->_Y - dfATTACK1_RANGE_Y <= (*session_it)->_Y
							&& tmpSesion->_Y >= (*session_it)->_Y)
							|| (tmpSesion->_Y + dfATTACK1_RANGE_Y >= (*session_it)->_Y
							&& tmpSesion->_Y <= (*session_it)->_Y)))//80범위 내에 존재하는 타겟이 있다면?
						{
							(*session_it)->_HP -= dfATTACK1_DAMAGE;//맞은 대상의 HP를 깎는다.

							if ((*session_it)->_HP <= 0)
							{
								Disconnect((*session_it));//클라이언트가 죽었으므로 종료
							}
							else {
								st_dfPACKET_header st_SC_DAMAGE_HEADER;
								st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
								st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
								st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

								st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
								st_SC_DAMAGE.Hp = (*session_it)->_HP;
								st_SC_DAMAGE._Attack_Id = tmpSesion->_Id;
								st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;

								sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, sizeof(st_dfPACKET_header));
								sendBroadCast(nullptr, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
							}
							break;
						}
					}
				}
				break;
				case dfPACKET_MOVE_DIR_RR:
				{
					for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
					{
						if ((*session_it)->_Live == false)
						{
							continue;//죽어있는 패킷 무시
						}

						if (tmpSesion->_Id == (*session_it)->_Id)
						{
							continue;
						}

						if ((tmpSesion->_X + dfATTACK1_RANGE_X >= (*session_it)->_X
							&& tmpSesion->_X <= (*session_it)->_X)
							&& ((tmpSesion->_Y - dfATTACK1_RANGE_Y <= (*session_it)->_Y
							&& tmpSesion->_Y >= (*session_it)->_Y)
							|| (tmpSesion->_Y + dfATTACK1_RANGE_Y >= (*session_it)->_Y
							&& tmpSesion->_Y <= (*session_it)->_Y)))//80범위 내에 존재하는 타겟이 있다면?
						{
							(*session_it)->_HP -= dfATTACK1_DAMAGE;//맞은 대상의 HP를 깎는다.

							if ((*session_it)->_HP <= 0)
							{
								Disconnect((*session_it));//클라이언트가 죽었으므로 종료
							}
							else 
							{
								st_dfPACKET_header st_SC_DAMAGE_HEADER;
								st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
								st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
								st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

								st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
								st_SC_DAMAGE.Hp = (*session_it)->_HP;
								st_SC_DAMAGE._Attack_Id = tmpSesion->_Id;
								st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;

								sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, sizeof(st_dfPACKET_header));
								sendBroadCast(nullptr, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
							}
							break;
						}
					}
				}
				break;
			}
#ifdef df_LOG
			printf("PACKET_ATTACK1 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", tmpSesion->_Id, tmpSesion->_Direction, tmpSesion->_X, tmpSesion->_Y);
#endif
		}
		break;
		case dfPACKET_CS_ATTACK2://공격2
		{
			st_dfPACKET_CS_ATTACK2 st_CS_ATTACK2;
			retSwitchDeq = tmpSesion->_RecvBuf->Dequeue((char*)&st_CS_ATTACK2, st_recv_header.bySize);//사이즈만큼 디큐한다.
			if (retSwitchDeq != st_recv_header.bySize)
			{
				Disconnect(tmpSesion);
				return;
			}

			//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
			tmpSesion->_Direction = st_CS_ATTACK2._Direction;//공격하는 방향
			tmpSesion->_Direction_check = ON_STOP;//이동중 이라고 변경
			tmpSesion->_X = st_CS_ATTACK2._X;//받은 크기값을 넣는다.
			tmpSesion->_Y = st_CS_ATTACK2._Y;//받은 크기값을 넣는다.

			//헤더설정
			st_dfPACKET_header st_SC_ATTACK2_HAEDER;
			st_SC_ATTACK2_HAEDER.byCode = PACKET_CODE;
			st_SC_ATTACK2_HAEDER.bySize = sizeof(st_dfPACKET_SC_ATTACK2);
			st_SC_ATTACK2_HAEDER.byType = dfPACKET_SC_ATTACK2;

			//데이터설정
			st_dfPACKET_SC_ATTACK2 st_SC_ATTACK2;
			st_SC_ATTACK2._Id = tmpSesion->_Id;//어떤 녀석인지 ID설정
			st_SC_ATTACK2._Direction = tmpSesion->_Direction;//보는 방향지정
			st_SC_ATTACK2._X = tmpSesion->_X;//위치X
			st_SC_ATTACK2._Y = tmpSesion->_Y;//위치Y

			sendBroadCast(tmpSesion, (char*)&st_SC_ATTACK2_HAEDER, sizeof(st_dfPACKET_header));
			sendBroadCast(tmpSesion, (char*)&st_SC_ATTACK2, sizeof(st_dfPACKET_SC_ATTACK2));
			
			switch (tmpSesion->_Direction)
			{
			case dfPACKET_MOVE_DIR_LL:
			{
				for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
				{
					if ((*session_it)->_Live == false)
					{
						continue;//죽어있는 패킷 무시
					}

					if (tmpSesion->_Id == (*session_it)->_Id)
					{
						continue;
					}
					//왼쪽범위와 Y축 위아래 일치했을 경우
					if ((tmpSesion->_X - dfATTACK2_RANGE_X <= (*session_it)->_X
						&& tmpSesion->_X >= (*session_it)->_X)
						
						&& ((tmpSesion->_Y - dfATTACK2_RANGE_Y <= (*session_it)->_Y
						&& tmpSesion->_Y >= (*session_it)->_Y)
						|| (tmpSesion->_Y + dfATTACK2_RANGE_Y >= (*session_it)->_Y
						&& tmpSesion->_Y <= (*session_it)->_Y)))//80범위 내에 존재하는 타겟이 있다면?
					{
						(*session_it)->_HP -= dfATTACK2_DAMAGE;//맞은 대상의 HP를 깎는다.

						if ((*session_it)->_HP <= 0)
						{
							Disconnect((*session_it));//클라이언트가 죽었으므로 종료
						}
						else 
						{
							st_dfPACKET_header st_SC_DAMAGE_HEADER;
							st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
							st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
							st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

							st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
							st_SC_DAMAGE.Hp = (*session_it)->_HP;
							st_SC_DAMAGE._Attack_Id = tmpSesion->_Id;
							st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;

							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, sizeof(st_dfPACKET_header));
							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
						}
						break;
					}
				}
			}
			break;
			case dfPACKET_MOVE_DIR_RR:
			{
				for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
				{
					if ((*session_it)->_Live == false)
					{
						continue;//죽어있는 패킷 무시
					}

					if (tmpSesion->_Id == (*session_it)->_Id)
					{
						continue;
					}
					//오른쪽 범위와 Y축 위아래 일치했을 경우
					if ((tmpSesion->_X + dfATTACK2_RANGE_X >= (*session_it)->_X
						&& tmpSesion->_X <= (*session_it)->_X)
						&& ((tmpSesion->_Y - dfATTACK2_RANGE_Y <= (*session_it)->_Y
						&& tmpSesion->_Y >= (*session_it)->_Y)
						|| (tmpSesion->_Y + dfATTACK2_RANGE_Y >= (*session_it)->_Y
						&& tmpSesion->_Y <= (*session_it)->_Y)))//80범위 내에 존재하는 타겟이 있다면?
					{
						(*session_it)->_HP -= dfATTACK2_DAMAGE;//맞은 대상의 HP를 깎는다.

						if ((*session_it)->_HP <= 0)
						{
							Disconnect((*session_it));//클라이언트가 죽었으므로 종료
						}
						else 
						{
							st_dfPACKET_header st_SC_DAMAGE_HEADER;
							st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
							st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
							st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

							st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
							st_SC_DAMAGE.Hp = (*session_it)->_HP;
							st_SC_DAMAGE._Attack_Id = tmpSesion->_Id;
							st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;

							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, sizeof(st_dfPACKET_header));
							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));

							if ((*session_it)->_HP <= 0)
							{
								Disconnect((*session_it));//클라이언트가 죽었으므로 종료
							}
						}
						break;
					}
				}
			}
			break;
			}
#ifdef df_LOG
			printf("PACKET_ATTACK2 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", tmpSesion->_Id, tmpSesion->_Direction, tmpSesion->_X, tmpSesion->_Y);
#endif
		}
		break;
		case dfPACKET_CS_ATTACK3://공격3
		{
			st_dfPACKET_CS_ATTACK2 st_CS_ATTACK3;
			retSwitchDeq = tmpSesion->_RecvBuf->Dequeue((char*)&st_CS_ATTACK3, st_recv_header.bySize);//사이즈만큼 디큐한다.
			if (retSwitchDeq != st_recv_header.bySize)
			{
				Disconnect(tmpSesion);
				return;
			}

			//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
			tmpSesion->_Direction = st_CS_ATTACK3._Direction;//공격하는 방향
			tmpSesion->_Direction_check = ON_STOP;//이동중 이라고 변경
			tmpSesion->_X = st_CS_ATTACK3._X;//받은 크기값을 넣는다.
			tmpSesion->_Y = st_CS_ATTACK3._Y;//받은 크기값을 넣는다.

			//헤더설정
			st_dfPACKET_header st_SC_ATTACK3_HAEDER;
			st_SC_ATTACK3_HAEDER.byCode = PACKET_CODE;
			st_SC_ATTACK3_HAEDER.bySize = sizeof(st_dfPACKET_SC_ATTACK3);
			st_SC_ATTACK3_HAEDER.byType = dfPACKET_SC_ATTACK3;

			//데이터설정
			st_dfPACKET_SC_ATTACK3 st_SC_ATTACK3;
			st_SC_ATTACK3._Id = tmpSesion->_Id;//어떤 녀석인지 ID설정
			st_SC_ATTACK3._Direction = tmpSesion->_Direction;//보는 방향지정
			st_SC_ATTACK3._X = tmpSesion->_X;//위치X
			st_SC_ATTACK3._Y = tmpSesion->_Y;//위치Y

			sendBroadCast(tmpSesion, (char*)&st_SC_ATTACK3_HAEDER, sizeof(st_dfPACKET_header));
			sendBroadCast(tmpSesion, (char*)&st_SC_ATTACK3, sizeof(st_dfPACKET_SC_ATTACK3));

			switch (tmpSesion->_Direction)
			{
			case dfPACKET_MOVE_DIR_LL:
			{
				for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
				{
					if ((*session_it)->_Live == false)
					{
						continue;//죽어있는 패킷 무시
					}

					if (tmpSesion->_Id == (*session_it)->_Id)
					{
						continue;
					}

					if ((tmpSesion->_X - dfATTACK3_RANGE_X <= (*session_it)->_X
						&& tmpSesion->_X >= (*session_it)->_X)

						&& ((tmpSesion->_Y - dfATTACK3_RANGE_Y <= (*session_it)->_Y
						&& tmpSesion->_Y >= (*session_it)->_Y)
						|| (tmpSesion->_Y + dfATTACK3_RANGE_Y >= (*session_it)->_Y
						&& tmpSesion->_Y <= (*session_it)->_Y)))//80범위 내에 존재하는 타겟이 있다면?
					{
						(*session_it)->_HP -= dfATTACK3_DAMAGE;//맞은 대상의 HP를 깎는다.

						if ((*session_it)->_HP <= 0)
						{
							Disconnect((*session_it));//클라이언트가 죽었으므로 종료
						}
						else {
							st_dfPACKET_header st_SC_DAMAGE_HEADER;
							st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
							st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
							st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

							st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
							st_SC_DAMAGE.Hp = (*session_it)->_HP;
							st_SC_DAMAGE._Attack_Id = tmpSesion->_Id;
							st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;

							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, sizeof(st_dfPACKET_header));
							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));

							if ((*session_it)->_HP <= 0)
							{
								Disconnect((*session_it));//클라이언트가 죽었으므로 종료
							}
						}
						break;
					}
				}
			}
			break;
			case dfPACKET_MOVE_DIR_RR:
			{
				for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
				{
					if ((*session_it)->_Live == false)
					{
						continue;//죽어있는 패킷 무시
					}

					if (tmpSesion->_Id == (*session_it)->_Id)
					{
						continue;
					}

					if ((tmpSesion->_X + dfATTACK3_RANGE_X >= (*session_it)->_X
						&& tmpSesion->_X <= (*session_it)->_X)
						&& ((tmpSesion->_Y - dfATTACK3_RANGE_Y <= (*session_it)->_Y
						&& tmpSesion->_Y >= (*session_it)->_Y)
						|| (tmpSesion->_Y + dfATTACK3_RANGE_Y >= (*session_it)->_Y
						&& tmpSesion->_Y <= (*session_it)->_Y)))//80범위 내에 존재하는 타겟이 있다면?
					{
						(*session_it)->_HP -= dfATTACK3_DAMAGE;//맞은 대상의 HP를 깎는다.

						if ((*session_it)->_HP <= 0)
						{
							Disconnect((*session_it));//클라이언트가 죽었으므로 종료
						}
						else {
							st_dfPACKET_header st_SC_DAMAGE_HEADER;
							st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
							st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
							st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

							st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
							st_SC_DAMAGE.Hp = (*session_it)->_HP;
							st_SC_DAMAGE._Attack_Id = tmpSesion->_Id;
							st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;

							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, sizeof(st_dfPACKET_header));
							sendBroadCast(nullptr, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));

							if ((*session_it)->_HP <= 0)
							{
								Disconnect((*session_it));//클라이언트가 죽었으므로 종료
							}
						}
						break;
					}
				}
			}
			break;
			}
#ifdef df_LOG
			printf("PACKET_ATTACK3 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", tmpSesion->_Id, tmpSesion->_Direction, tmpSesion->_X, tmpSesion->_Y);
#endif
		}
		break;
		}
	}

};

void SendProc(Session* session)
{
	char buf[_df_Buffer_size];
	memset(buf, 0, sizeof(buf));//데이터 밀어버리기
	int retPeek = session->_SendBuf->Peek(buf, session->_SendBuf->GetUseSize());//버퍼에서 Usesize만큼 가져오기

	int retSend = send(session->_Sock, buf, retPeek, 0);//가져온만큼 전송

	if (retSend == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError = WSAGetLastError();//소켓에러값
		log_msg(wsaError);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}
	else if (retSend == 0)
	{
		int wsaError = WSAGetLastError();//소켓에러값
		log_msg(wsaError);
		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}

	if (retSend != retPeek)//긁어온것과 보낸양이 다르다? 링버퍼가 박살나서 이상하게 만들어진것이다 연결을 종료하고 로그를 남겨야한다.
	{
		Disconnect(session);
		return;
	}
	session->_SendBuf->MoveFront(retSend);//보낸만큼 읽은 포인터를 이동시킨다.

};//유저에게 데이터를 보낼때

void sendUniCast(Session* session, char* _Msg, int size)
{
	int retUni;

	if (session->_Live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}
	if (session->_SendBuf->GetFreeSize() >= size)//sendbuf에 16바이트 이상이 남아있다면 인큐시킨다.
	{
		retUni = session->_SendBuf->Enqueue(_Msg, size);//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != size)
		{
			Disconnect(session);
			return;
		}
	}

};//특정유저에게만 보내기

void sendBroadCast(Session* session, char* _Msg, int size)
{
	int retBro;
	Session* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->_Id == (*session_it)->_Id)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if ((*session_it)->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}
		//각세션의 센드버퍼에 enqueue시킨다.
		if ((*session_it)->_SendBuf->GetFreeSize() >= size)//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = (*session_it)->_SendBuf->Enqueue(_Msg, size);//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != size)
			{
				Disconnect((*session_it));
			}
		}

	}
};//모든 유저에게 보내기

void Disconnect(Session* session)//연결 끊기 함수
{
	Session* stmp = session;
	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		if (stmp->_Id == (*session_it)->_Id)//값이 같다면 사망처리한다.
		{
			(*session_it)->_Live = false;//세션 사망처리
			break;
		}
	}
};

void Disconnect_Clean()//안정성을 위해 디스커넥트된 개체를 네트워크 마지막시점에 정리한다.
{
	CList <Session*>::iterator _Session_it;
	Session* _Session_Object;
	for (_Session_it = Session_List.begin(); _Session_it != Session_List.end();)
	{
		_Session_Object = *_Session_it;
		if (_Session_Object->_Live == false) //세션 특정객체가 죽어있다면
		{
			//헤더패킷 생성
			st_dfPACKET_header st_header;
			st_header.byCode = PACKET_CODE;
			st_header.bySize = sizeof(st_dfPACKET_SC_DELETE_CHARACTER);
			st_header.byType = dfPACKET_SC_DELETE_CHARACTER;

			st_dfPACKET_SC_DELETE_CHARACTER st_delete;
			st_delete._Id = _Session_Object->_Id;

			sendBroadCast(nullptr, (char*)&st_header, sizeof(st_dfPACKET_header));
			sendBroadCast(nullptr, (char*)&st_delete, sizeof(st_dfPACKET_SC_DELETE_CHARACTER));

			closesocket(_Session_Object->_Sock);
			delete _Session_Object->_RecvBuf;//받기버퍼 제거
			delete _Session_Object->_SendBuf;//보내기 버퍼 제거
#ifdef df_LOG
			printf("Delete Session # SessionID %d \n", _Session_Object->_Id);
#endif
			delete _Session_Object;//동적할당된 개체 제거
			_Session_it = Session_List.erase(_Session_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
		}
		else
		{
			++_Session_it;//살아있는 객체라면 다음 리스트로 넘어간다.
		}
	}
}

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
void log_msg(int wsaError)
{
	//이 에러라면 로그저장 안한다.
	if (wsaError == WSAECONNRESET//현재 연결은 원격 호스트에 의해 강제로 끊겼습니다.
		|| wsaError == WSAECONNABORTED//소프트웨어로 인해 연결이 중단되었습니다.
		|| wsaError == WSANOTINITIALISED//성공한 WSAStartup이 아직 수행되지 않았습니다.
		|| wsaError == WSAEWOULDBLOCK)//이 오류는 즉시 완료할 수 없는 비블로킹 소켓의 작업에서 반환됩니다
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
	fclose(fp);

	char buff[200];
	memset(buff, 0, sizeof(buff));
	sprintf_s(buff, "retRecv Error = %d", wsaError);
	fopen_s(&fp, log_FileName, "a");
	fprintf(fp, "%s\n", buff);
	fclose(fp);
}