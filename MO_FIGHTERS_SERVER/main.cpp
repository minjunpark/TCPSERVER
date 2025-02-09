﻿
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
#include "CRingBuffer.h"
#include "CList.h"
#include "enum_Server.h"
#include "st_Packet.h"
#include "CSerealBuffer.h"
#define df_LOG//로그 켜고싶으면 세팅


using namespace std;

void NetWork();
void Logic();
//서버 센드 리시브함수
void init_sock();
void AcceptProc();//접속유저가 새로 생겼을때
void RecvProc(Session* session);//유저가 데이터를 보내왔을때
void SendProc(Session* session);//유저에게 데이터를 보낼때
void sendUniCast(Session* session, char* _header, char* _Msg, int size);
void sendBroadCast(Session* session, char* _header, char* _Msg, int size);

void sendUniCast(Session* session, CSerealBuffer* pPacket);
void sendBroadCast(Session* session, CSerealBuffer* pPacket);

bool PacketProc(Session* pSession, BYTE byPacketType, CSerealBuffer* pPacket);//패킷 전송 과정

void Disconnect(Session* session);//연결끊기 Session _Live를 false로 바꿔죽인다.
void Disconnect_Clean();//네트워크 마지막에 죽은세션을 정리한다.

//오류 출력 함수 밑 로그저장
void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);
void log_msg(int _wsaError, int _line, const char* _file);

CList<Session*> Session_List;//접속한 유저리스트
int Session_Total = 0;//세션 데이터

#include "fun_Message.h"

//서버에 받아들이기 위한 리슨소켓
SOCKET listen_sock;

//확인용 함수데이터 저장
FD_SET rset;
FD_SET wset;
int User_Id_Count = 1;

int LogicTime = 0;
DWORD dwTick = timeGetTime();
DWORD dwStarttick = timeGetTime();
//int fc = 0;

int main()
{
	timeBeginPeriod(1);

	init_sock();

	while (1) {
		NetWork();//네트워크
		Logic();//로직
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
			retSelect--;
		}

		//현재 세션리스트중에 반응이 있는 소켓이 있는지 확인하기위해 모든 리스트를 돌면서 확인한다.
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if (FD_ISSET((*session_it)->_Sock, &rset))//반응이 있는 읽기 소켓이 있다면?
			{
				RecvProc(*session_it);//Recv Proc를 실행한다.
				retSelect--;
			}

			if (FD_ISSET((*session_it)->_Sock, &wset))//반응가능한 쓰기소켓이 있다면
			{
				SendProc(*session_it);
				retSelect--;
			}
			if (retSelect <= 0)
				break;
		}

	}
	Disconnect_Clean();//연결이 끊긴 녀석들을 모두제거한다.
};

void Logic()
{
	//50fps 간단 로직
	LogicTime = timeGetTime() - dwStarttick;

	if (LogicTime < 20)//전 시점에서 20ms가 지나지 않았다면 로직은 수행하지 않는다.
		return;
	else //20미리 이상 지났다면 다시 시간초기화
		dwStarttick = timeGetTime();

	//모든 연결 세션을 돌면서 로직을 수행한다.
	for (auto _Session_it = Session_List.begin(); _Session_it != Session_List.end(); ++_Session_it)
	{
		if ((*_Session_it)->_Direction_check != ON_MOVE || (*_Session_it)->_Live != true)
		{
			//움직이지 않아도 되는 세션이거나
			continue;//죽어있는 세션이라면 계산을 하지 않는다.
		}

		if ((*_Session_it)->_HP <= 0)//Hp가 전부달아버린 세션이라면 세션을 죽이고 로직계산을 하지 않는다.
		{
			Disconnect((*_Session_it));
			continue;
		}

		//범위를 초과하고 있는 세션은 더이상 로직에서 움직이게 하지 않는다.
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

bool PacketProc(Session* pSession, BYTE byPacketType, CSerealBuffer* pPacket)
{
	switch (byPacketType)
	{
		case dfPACKET_SC_CREATE_MY_CHARACTER:
			netPacketProc_SC_CREATE_MY_CHARACTER(pSession, pPacket);
			break;
		case dfPACKET_SC_CREATE_OTHER_CHARACTER:
			netPacketProc_SC_CREATE_OTHER_CHARACTER(pSession, pPacket);
			break;
		case dfPACKET_SC_DELETE_CHARACTER:
			netPacketProc_DELETE_CHARACTER(pSession, pPacket);
			break;
		case dfPACKET_CS_MOVE_START:
			netPacketProc_MOVE_START(pSession, pPacket);
			break;
		case dfPACKET_CS_MOVE_STOP:
			netPacketProc_MOVE_STOP(pSession, pPacket);
			break;
		case dfPACKET_CS_ATTACK1:
			netPacketProc_ATTACK1(pSession, pPacket);
			break;
		case dfPACKET_CS_ATTACK2:
			netPacketProc_ATTACK2(pSession, pPacket);
			break;
		case dfPACKET_CS_ATTACK3:
			netPacketProc_ATTACK3(pSession, pPacket);
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
	Session* NewSession = new Session;
	NewSession->_SendBuf = new CRingBuffer;
	NewSession->_RecvBuf = new CRingBuffer;
	NewSession->_Direction = default_Direction;
	NewSession->_Direction_check = ON_STOP;
	NewSession->_Sock = client_sock;//sock을 세팅하고
	NewSession->_Id = AccteptTotal;//ID세팅
	NewSession->_X = defualt_X_SET;//X세팅
	NewSession->_Y = defualt_Y_SET;//Y세팅
	NewSession->_Live = true;//세션 생존플래그 0이면 사망
	NewSession->_HP = df_HP;

	//2.생성된 유저 데이터 나에게 전송하기
	PacketProc(NewSession, dfPACKET_SC_CREATE_MY_CHARACTER, nullptr);

	//3.현재 리스트에 있는 유저들을 생성한 유저에게 모두 전송한다.
	for (auto _Session_it = Session_List.begin(); _Session_it != Session_List.end(); ++_Session_it)
	{
		if ((*_Session_it)->_Live == true)//살아있는 세션이라면?
		{
			st_dfPACKET_header st_other_header;
			st_other_header.byCode = PACKET_CODE;
			st_other_header.bySize = sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER);
			st_other_header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

			CSerealBuffer pPacket;
			pPacket.PutData((char*)&st_other_header,sizeof(st_dfPACKET_header));
			pPacket << (*_Session_it)->_Id << (*_Session_it)->_Direction << (*_Session_it)->_X << (*_Session_it)->_Y << (*_Session_it)->_HP;
			sendUniCast(NewSession, &pPacket);//나 자신을 제외하고 데이터전부잇풋
		}
	}

	//4.생성된 세션 리스트에 넣기
	Session_List.push_back(NewSession);

	//5.생성된 나를 모든 유저에게 전달한다.
	CSerealBuffer pPacket;
	pPacket << NewSession->_Id;
	pPacket << NewSession->_Direction;
	pPacket << NewSession->_X;
	pPacket << NewSession->_Y;
	pPacket << NewSession->_HP;

	PacketProc(NewSession, dfPACKET_SC_CREATE_OTHER_CHARACTER, &pPacket);
};//접속유저가 새로 생겼을때

void RecvProc(Session* session)//유저가 데이터를 보내왔을때
{
	Session* tmpSesion;
	tmpSesion = session;

	int retRecv;
	//retRecv = recv(tmpSesion->_Sock, (char*)buf, tmpSesion->_RecvBuf->GetFreeSize(), 0);//남아있는 Free사이즈만큼 긁어온다.
	retRecv = recv(tmpSesion->_Sock, (char*)tmpSesion->_RecvBuf->GetRearBufferPtr(),
		tmpSesion->_RecvBuf->DirectEnqueueSize(), 0);//한번에 넣을수있는 사이즈만큼 넣는다.

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
	
	tmpSesion->_RecvBuf->MoveRear(retRecv);//받은만큼 이동시킨다.

	CSerealBuffer cpPacket;

	while (1)
	{
		int retSwitchDeq;
		int retCS;
		int retPeek;
		
		if (tmpSesion->_RecvBuf->GetUseSize() < sizeof(st_dfPACKET_header))//사용할수 있는사이즈가 헤더이하라면
			break;

		st_dfPACKET_header st_recv_header;
		retPeek = tmpSesion->_RecvBuf->Peek((char*)&st_recv_header, sizeof(st_dfPACKET_header));//헤더크기만큼 꺼내기

		if (retPeek != sizeof(st_dfPACKET_header))//값이 다르면 애초에 박살나는것이기 때문에 브레이크
		{
			Disconnect(tmpSesion);
			break;
		}

		if (tmpSesion->_RecvBuf->GetUseSize() < sizeof(st_dfPACKET_header) + st_recv_header.bySize)//헤더에 적힌 사이즈 보다 작으므로 더 받는절차를 거쳐야하므로 정지한다.
			break;

		//헤더에서 읽어온 크기만큼 버퍼에 존재한다면?
		tmpSesion->_RecvBuf->MoveFront(sizeof(st_dfPACKET_header));//헤더크기만큼 버퍼의 읽기포인터를 이동시킨다.		
		
		//직렬화버퍼 작업
		retSwitchDeq = tmpSesion->_RecvBuf->Dequeue(cpPacket.GetBufferPtr(), st_recv_header.bySize);//사이즈만큼 직렬화 버퍼에 디큐한다.

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

		switch (st_recv_header.byType)//byType으로 패킷을 판별해 그 기능을 수행하도록한다.
		{
			case dfPACKET_CS_MOVE_START:
				PacketProc(tmpSesion, dfPACKET_CS_MOVE_START, &cpPacket);
				break;
			case dfPACKET_CS_MOVE_STOP://정지
				PacketProc(tmpSesion, dfPACKET_CS_MOVE_STOP, &cpPacket);
				break;
			case dfPACKET_CS_ATTACK1://공격
				PacketProc(tmpSesion, dfPACKET_CS_ATTACK1, &cpPacket);
				break;
			case dfPACKET_CS_ATTACK2://공격2
				PacketProc(tmpSesion, dfPACKET_CS_ATTACK2, &cpPacket);
				break;
			case dfPACKET_CS_ATTACK3://공격3
				PacketProc(tmpSesion, dfPACKET_CS_ATTACK3, &cpPacket);
				break;
		}

		cpPacket.Clear();//직렬화 버퍼 재사용을 위한 초기화
	}

};

void SendProc(Session* session)
{
	int retSend;
	//읽기 버퍼 포인터에서 보낼수있는 크기만큼 최대한 보낸다.
	retSend = send(session->_Sock, session->_SendBuf->GetFrontBufferPtr(), session->_SendBuf->DirectDequeueSize(), 0);

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

	if (retSend != session->_SendBuf->DirectDequeueSize())//긁어온것과 보낸양이 다르다? 링버퍼가 박살나서 이상하게 만들어진것이다 연결을 종료하고 로그를 남겨야한다.
	{
		Disconnect(session);
		return;
	}

	session->_SendBuf->MoveFront(retSend);//센드버퍼에 쌓은만큼 포인터를 이동시킨다.

};//유저에게 데이터를 보낼때



void sendUniCast(Session* session, char* _header, char* _Msg, int size)
{
	int retUni;

	if (session->_Live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}
	if (session->_SendBuf->GetFreeSize() >= size)//sendbuf에 16바이트 이상이 남아있다면 인큐시킨다.
	{
		retUni = session->_SendBuf->Enqueue(_header, sizeof(st_dfPACKET_header));//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != sizeof(st_dfPACKET_header))
		{
			Disconnect(session);
			return;
		}

		retUni = session->_SendBuf->Enqueue(_Msg, size);//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != size)
		{
			Disconnect(session);
			return;
		}
	}

};//특정유저에게만 보내기

void sendUniCast(Session* session, CSerealBuffer* pPacket)
{
	if (session->_Live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}

	if (session->_SendBuf->GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 직렬화버퍼크기만큼 넣는다.
	{
		int retUni;
		retUni = session->_SendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != pPacket->GetUseSize())
		{
			Disconnect(session);
			return;
		}
	}
}

void sendBroadCast(Session* session, CSerealBuffer *pPacket)
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
		if ((*session_it)->_SendBuf->GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 넣을수있는 크기가 남아있다면
		{
			retBro = (*session_it)->_SendBuf->Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != pPacket->GetUseSize())//센드버퍼 자체가 고장났거나 송신버퍼가 가득찬경우 이므로 디스커넥트하고 끝내버린다.
			{
				Disconnect((*session_it));
			}
		}
	}
}
void sendBroadCast(Session* session, char* _header, char* _Msg, int size)
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
			retBro = (*session_it)->_SendBuf->Enqueue(_header, sizeof(st_dfPACKET_header));//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != sizeof(st_dfPACKET_header))
			{
				Disconnect((*session_it));
			}

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
			PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);
			closesocket(_Session_Object->_Sock);
			delete _Session_Object->_RecvBuf;//세션 받기버퍼 제거
			delete _Session_Object->_SendBuf;//세션 보내기 버퍼 제거
			delete _Session_Object;//동적할당된 세션 제거
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