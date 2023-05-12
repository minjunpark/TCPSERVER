// SeleteModel.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#pragma comment(lib,"ws2_32")
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
#include "Server_define.h"
#include "Packet_Struct.h"
#include "Console.h"

CList<Session*> Session_List;//접속한 유저리스트
int Session_Total = 0;

//SOCKETINFO* SocketInfoArray[FD_SETSIZE];//소켓리스트

//서버에 받아들이기 위한 리슨소켓
SOCKET listen_sock;
int nTotalSockets = 0;//총 소켓 개수

int packet_count;//패킷 총개수

//화면 렌더링용 함수및 변수 START
void Buffer_Flip(void);
void Buffer_Clear(void);
void Sprite_Draw(int iX, int iY, char chSprite);
char szScreenBuffer[dfSCREEN_HEIGHT][dfSCREEN_WIDTH];
char szScreen[23][83] = { ' ', };
//화면 렌더링용 함수및 변수 END


//오류 출력 함수
void err_quit(const WCHAR* msg);
void err_display(const WCHAR* msg);

//서버 센드 리시브함수
void AcceptProc();//listen소켓 처리
void RecvProc(Session* session);//받은 메세지처리 프로세스
void SendProc(Session* session);
void sendUniCast(Session* session, char* _Msg);//특정유저에게만 보내기
void sendBroadCast(Session* session, char* _Msg);//모든 유저에게 보내기

void Disconnect(Session* session);
void Disconnect_Clean();
void NetWork();
void Render();

void log_msg(char* msg);//로그저장용함수

//로그저장용 텍스트파일 생성


//확인용 함수데이터 저장
int retSelect;
SOCKET client_sock;
SOCKADDR_IN clientaddr;
int addrlen;
FD_SET rset;
FD_SET wset;

timeval times;


int main()
{
	cs_Initial();

	times.tv_sec = 0;
	times.tv_usec = 0;

	//한글세팅
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");
	
	//원속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	int retOut;

	//listen socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit(L"socket()");

	//bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	//linger timewait 0 rst바로 전송하게 만들기
	linger _linger;
	_linger.l_onoff = 0;
	_linger.l_linger = 0;

	int keepval = 0;

	setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (const char*)&_linger, sizeof(_linger));//링거로 타임아웃 0
	setsockopt(listen_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepval, sizeof keepval);//keep alive내가 직접할것

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	retOut = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retOut == SOCKET_ERROR)err_quit(L"bind()");

	//listen()
	retOut = listen(listen_sock, SOMAXCONN);
	if (retOut == SOCKET_ERROR)err_quit(L"listen()");

	//넌 블로킹 소켓으로 전환
	u_long on = 1;
	retOut = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retOut == SOCKET_ERROR)
		err_quit(L"ioctlsocket()");

	while (1)
	{
		NetWork();//네트워크작업
		Render();//렌더링
	}

	//소켓 닫기
	closesocket(listen_sock);

	//원속종료
	WSACleanup();
	return 0;
}


void NetWork()
{
	//데이터 통신에 사용할 변수

	//네트워킹 작업
	FD_ZERO(&rset);//rset을 밀어버려서 초기화
	FD_ZERO(&wset);
	FD_SET(listen_sock, &rset);//리슨소켓을 rset소켓에 세팅한다

	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		FD_SET((*session_it)->sock, &rset);//반응이 있는소켓을 전부 rset에 넣는다.

		if ((*session_it)->sendBuf->GetUseSize() >= 0)//세션 버퍼에 보내야하는 데이터가 0이상이라면?
		{
			FD_SET((*session_it)->sock, &wset);
		}
	}

	retSelect = select(0, &rset, &wset, NULL, &times);//반응이 있는 소켓이 있는지 셀렉트로 확인

	if (retSelect == SOCKET_ERROR)
	{
		err_quit(L"select()");
	}


	if (retSelect > 0)//반응이 있는 소켓이 있다면?
	{
		if (FD_ISSET(listen_sock, &rset))//리슨소켓에 반응이 있다면
		{
			AcceptProc();
		}

		//현재 세션리스트중에 반응이 있는 소켓이 있는지 확인하기위해 모든 리스트를 돌면서 확인한다.
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if (FD_ISSET((*session_it)->sock, &rset))//반응이 있는 읽기 소켓이 있다면?
			{
				RecvProc(*session_it);//Recv Proc를 실행한다.
			}

			if (FD_ISSET((*session_it)->sock, &wset))//반응가능한 쓰기소켓이 있다면
			{
				SendProc(*session_it);
			}
		}
	}
	Disconnect_Clean();//연결이 끊긴 녀석들을 모두제거한다.
}

void SendProc(Session* session)
{
	char buf[10000];
	memset(buf, 0, sizeof(buf));//데이터 밀어버리기
	int retPeek = session->sendBuf->Peek(buf, session->sendBuf->GetUseSize());//버퍼에서 Usesize만큼 가져오기

	int retSend = send(session->sock, buf, retPeek, 0);//가져온만큼 전송

	if (retSend == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError;//소켓에러값
		wsaError = WSAGetLastError();

		if (wsaError != 10035
			|| wsaError != 10054
			|| wsaError != WSAEWOULDBLOCK)//이에러가 아니라면 로그저장한다.
		{
			char buff[200];
			memset(buff, 0, sizeof(buff));
			sprintf_s(buff, "retRecv Error = %d", wsaError);
			log_msg(buff);
		}

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			//printf("retval error %d\n", retRecv);
			Disconnect(session);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
		//return;
	}

	if (retSend != retPeek)//긁어온것과 보낸양이 다르다? 링버퍼가 박살나서 이상하게 만들어진것이다 연결을 종료하고 로그를 남겨야한다.
	{
		Disconnect(session);
		return;
	}

	//session->sendBuf->MoveFront(retSend);
	//session->sendBuf->Dequeue(buf,retSend);//디큐로 먼저되는지 테스트했고
	session->sendBuf->MoveFront(retSend);//Front크기만큼 이동시킨다.
};

void AcceptProc()//listen소켓 처리
{
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	addrlen = sizeof(clientaddr);
	client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);

	if (client_sock == SOCKET_ERROR)
	{
		return;//그냥 연결에러 발생이니까 뒷처리 할 필요 없이 리턴한다.
		//Disconnect();
	}

	int AccteptTotal = nTotalSockets++;//여기서 계속 +하면서 ID 값을 새로 생성한다.

	//1.새로운 연결된 유저를 생성한다
	Session* NewSession = new Session;
	NewSession->sendBuf = new TRingBuffer;
	NewSession->recvBuf = new TRingBuffer;
	NewSession->sock = client_sock;//sock을 세팅하고
	NewSession->Id = AccteptTotal;//ID세팅
	NewSession->X = _e_Player::_X;//X세팅
	NewSession->Y = _e_Player::_Y;//Y세팅
	NewSession->live = true;//세션 생존플래그 0이면 사망

	////2.아이디 생성 패킷 생성후 접속한 사용자에게 전송한다.
	STAR_ID _Id_Pack;
	_Id_Pack._Type = ID_SET;
	_Id_Pack._Id = AccteptTotal;
	sendUniCast(NewSession, (char*)&_Id_Pack);//나 자신에게 별을 생성해서 전송한다.

	CList <Session*>::iterator _Session_it;
	//CBaseObject* _Des_Object;
	//4.현재 리스트에 있는 유저들을 전부 전달한다.
	for (auto _Session_it = Session_List.begin();
		_Session_it != Session_List.end(); ++_Session_it)
	{
		if ((*_Session_it)->live == true)
		{
			STAR_CREATE _Cre_Pack;
			_Cre_Pack._Type = CREATE_SET;
			_Cre_Pack._Id = (*_Session_it)->Id;
			_Cre_Pack._X = (*_Session_it)->X;
			_Cre_Pack._Y = (*_Session_it)->Y;
			sendUniCast(NewSession, (char*)&_Cre_Pack);
		}
	}

	Session_List.push_back(NewSession);//생선된 세션을 세션리스트에 넣는다.

	////3.생성된 별 패킷을 모든 사용자에게 전달한다.
	STAR_CREATE _Cre_Pack;
	_Cre_Pack._Type = CREATE_SET;
	_Cre_Pack._Id = NewSession->Id;
	_Cre_Pack._X = _X;
	_Cre_Pack._Y = _Y;
	sendBroadCast(nullptr, (char*)&_Cre_Pack);//연결된 유저 전부에게 전송한다.
};

void RecvProc(Session* session)//받은 메세지처리 프로세스
{
	int retRecv;
	Session* tmpSesion;
	tmpSesion = session;

	char buf[10000];


	//while (true)
	//{
		//if (tmpSesion->live!=true)//세션이 살아있을경우에만
		//{
		//	return;
		//}

	memset(buf, 0, sizeof(buf));//버퍼 clean

	retRecv = recv(tmpSesion->sock, (char*)buf, tmpSesion->recvBuf->GetFreeSize(), 0);//남아있는 Free사이즈만큼 긁어온다.

	if (retRecv == SOCKET_ERROR)//소켓 자체에 문제가 생겼다는 뜻이므로
	{
		int wsaError;//소켓에러값
		wsaError = WSAGetLastError();

		if (wsaError != 10035
			|| wsaError != WSAECONNRESET
			|| wsaError != WSAEWOULDBLOCK)//이에러가 아니라면 로그저장한다.
		{
			char buff[200];
			memset(buff, 0, sizeof(buff));
			sprintf_s(buff, "retRecv Error = %d", wsaError);
			log_msg(buff);
		}

		if (wsaError != WSAEWOULDBLOCK)//연결자체에 문제가 생겼다는 뜻이므로
		{
			//printf("retval error %d\n", retRecv);
			Disconnect(tmpSesion);//연결종료처리
			return;//송신버퍼가 비어있다는 뜻이므로 루프를 탈출한다.
		}
	}

	int retRing = tmpSesion->recvBuf->Enqueue((char*)buf, retRecv);

	if (retRecv != retRing)
	{
		Disconnect(tmpSesion);
		return;
	}

	while (1)
	{
		if (tmpSesion->recvBuf->GetUseSize() < 16)//사용할수 있는사이즈가 16이하라면 더이상 처리할수 있는 내용이 없다는것이므로 중단.
		{
			break;
		}

		int buffer[4];
		int retEn = tmpSesion->recvBuf->Dequeue((char*)buffer, sizeof(STAR_ID));//16바이트씩 꺼내기

		int p_Type = buffer[0];//Type
		int p_Id = buffer[1];//ID
		int p_X = buffer[2];//_X
		int p_Y = buffer[3];//_Y
		//무빙 패킷이 맞다면

		switch (p_Type)
		{
		case _STAR_MOVE://별이동
		{
			STAR_MOVE _send_Move;
			Session* session;
			for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
			{
				session = (*session_it);
				if (session->Id == p_Id)//들어온 이동패킷이 같은값이라면
				{
					_send_Move._Type = MOVE_SET;
					_send_Move._Id = p_Id;
					if (p_X < 0)
						p_X = 0;
					else if (p_X >= SCREEN_WIDTH)
						p_X = SCREEN_WIDTH;
					if (p_Y < 0)
						p_Y = 0;
					else if (p_Y >= SCREEN_HEIGHT)
						p_Y = SCREEN_HEIGHT;
					_send_Move._X = p_X;
					_send_Move._Y = p_Y;
					session->X = p_X;
					session->Y = p_Y;
					//나자신을 제외하고 패킷을 전송한다.
					sendBroadCast(session, (char*)&_send_Move);
					break;//브레이크
				}
			}
		}
		break;//_STAR_MOVE break;
		}
	}

	//}
};


void sendUniCast(Session* session, char* _Msg)//특정유저에게만 보내기
{
	int retUni;

	if (session->live == false)
	{
		return;//죽어있는패킷한테는안보낸다.
	}
	if (session->sendBuf->GetFreeSize() >= sizeof(STAR_ID))//sendbuf에 16바이트 이상이 남아있다면 인큐시킨다.
	{
		retUni = session->sendBuf->Enqueue(_Msg, sizeof(STAR_ID));//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
		if (retUni != sizeof(STAR_ID))
		{
			Disconnect(session);
			return;
		}
	}
};

void sendBroadCast(Session* session, char* _Msg)//특정 유저만 뺴고 보내기
{
	int retBro;
	Session* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		if (stmp != nullptr && stmp->Id == (*session_it)->Id)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if ((*session_it)->live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}
		//각세션의 센드버퍼에 enqueue시킨다.
		if ((*session_it)->sendBuf->GetFreeSize() >= sizeof(STAR_ID))//sendbuf에 16바이트 이상이 남아있다면 인큐시킨다.
		{
			retBro = (*session_it)->sendBuf->Enqueue(_Msg, sizeof(STAR_ID));//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != sizeof(STAR_ID))
			{
				Disconnect((*session_it));
			}
		}
		
	}
};

void Disconnect(Session* session)//연결 끊기 함수
{
	Session* stmp = session;
	int _Id;
	_Id = stmp->Id;
	for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
	{
		if (_Id == (*session_it)->Id)//값이 같다면 사망처리한다.
		{
			(*session_it)->live = false;//세션 사망처리
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
		if (_Session_Object->live != true) //세션 특정객체가 죽어있다면
		{
			STAR_DELETE p_delete;//삭제 패킷을 생성한다.
			p_delete._Type = DELETE_SET;
			p_delete._Id = _Session_Object->Id;
			sendBroadCast(nullptr, (char*)&p_delete);//생성된 패킷을 전달한다.
			closesocket(_Session_Object->sock);
			delete _Session_Object->recvBuf;
			delete _Session_Object->sendBuf;
			delete _Session_Object;//동적할당된 개체 제거
			_Session_it = Session_List.erase(_Session_it);//그 개체를 리스트에서 제거한후 받은 이터레이터를 리턴받는다.;
		}
		else
		{
			++_Session_it;//살아있는 객체라면 다음 리스트로 넘어간다.
		}
	}
}

void Render()
{
	Buffer_Clear();//버퍼지우기

	Sprite_Draw(0, 0, 'C');
	Sprite_Draw(1, 0, 'o');
	Sprite_Draw(2, 0, 'n');
	Sprite_Draw(3, 0, 'n');
	Sprite_Draw(4, 0, 'e');
	Sprite_Draw(5, 0, 'c');
	Sprite_Draw(6, 0, 't');

	Sprite_Draw(8, 0, 'C');
	Sprite_Draw(9, 0, 'l');
	Sprite_Draw(10, 0, 'i');
	Sprite_Draw(11, 0, 'e');
	Sprite_Draw(12, 0, 'n');
	Sprite_Draw(13, 0, 't');

	Sprite_Draw(15, 0, ':');

	Sprite_Draw(17, 0, Session_List.size() + 48);

	//Sprite_Draw(19, 0, 'P');
	//Sprite_Draw(20, 0, 'a');
	//Sprite_Draw(21, 0, 'c');
	//Sprite_Draw(22, 0, 'k');
	//Sprite_Draw(23, 0, 'e');
	//Sprite_Draw(24, 0, 't');
	//Sprite_Draw(25, 0, ':');
	//Sprite_Draw(26, 0, packet_count + 48);
	/*Sprite_Draw(27, 0, 'U');
	Sprite_Draw(28, 0, 'L');
	Sprite_Draw(29, 0, 'L');*/

	// 스크린 버퍼에 스프라이트 출력
	CList <Session*>::iterator _Session_it;
	Session* _Session_;
	for (_Session_it = Session_List.begin(); _Session_it != Session_List.end(); ++_Session_it)
	{
		_Session_ = *_Session_it;
		Sprite_Draw(_Session_->X, _Session_->Y, '*');
	}
	//스크린 버퍼를 화면으로 출력
	Buffer_Flip();
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
	printf("GetLastError() %d\n", GetLastError());
	LocalFree(lpMsgBuf);
}

//--------------------------------------------------------------------
// 버퍼의 내용을 화면으로 찍어주는 함수.
//
// 적군,아군,총알 등을 szScreenBuffer 에 넣어주고, 
// 1 프레임이 끝나는 마지막에 본 함수를 호출하여 버퍼 -> 화면 으로 그린다.
//--------------------------------------------------------------------
void Buffer_Flip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}


//--------------------------------------------------------------------
// 화면 버퍼를 지워주는 함수
//
// 매 프레임 그림을 그리기 직전에 버퍼를 지워 준다. 
// 안그러면 이전 프레임의 잔상이 남으니까
//--------------------------------------------------------------------
void Buffer_Clear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', dfSCREEN_WIDTH * dfSCREEN_HEIGHT);

	for (iCnt = 0; iCnt < dfSCREEN_HEIGHT; iCnt++)
	{
		szScreenBuffer[iCnt][dfSCREEN_WIDTH - 1] = '\0';
	}

}

//--------------------------------------------------------------------
// 버퍼의 특정 위치에 원하는 문자를 출력.
//
// 입력 받은 X,Y 좌표에 아스키코드 하나를 출력한다. (버퍼에 그림)
//--------------------------------------------------------------------
void Sprite_Draw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= dfSCREEN_WIDTH - 1 || iY >= dfSCREEN_HEIGHT)
		return;

	szScreenBuffer[iY][iX] = chSprite;
}

void log_msg(char* msg)
{
	
	char log_FileName[60];// = "127.0.0.1";
	FILE* fp;
	//로그저장용
	time_t now = time(NULL);  //now에 현재 시간 저장
	struct tm date;   //tm 구조체 타입의 날짜
	localtime_s(&date, &now);  //date에 now의 정보를 저장

	sprintf_s(log_FileName, sizeof(log_FileName), "Log_%02d_%02d_%02d.txt", date.tm_mday, date.tm_hour, date.tm_min);
	fopen_s(&fp, log_FileName, "wb");//파일 생성
	fclose(fp);

	fopen_s(&fp, log_FileName, "a");
	fprintf(fp, "%s\n", msg);
	fclose(fp);
}