
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

//특정 1명의 클라이언트에게 보내기
void sendPacket_UniCast(st_SESSION* session, CSerealBuffer* pPacket);
//특정 섹터 1개에 있는 클라에게 보내기
void sendPacket_SectorOne(st_SESSION* session, CSerealBuffer* pPacket, int iSectorX, int iSectorY);
//특정유저 주변 섹터에만 보내기
void sendPacket_Around(st_SESSION* session, CSerealBuffer* pPacket, bool bSendMe = false);//직렬화 버퍼 버전 브로드캐스트
//모든 유저에게 패킷쏘기 이걸 쓸일은 사실상 없다고 보면됨
void sendPacket_BroadCast(st_SESSION* session, CSerealBuffer* pPacket);

bool Sector_Check(int _PX, int _PY);
void SectorUpdate(st_PLAYER* pPlayer);

void PlayerSectorUpdatePacket(st_PLAYER* pPlayer);

//특정세턱 좌표 기준 주변 영향권 섹터가져오기
st_SECTOR_AROUND GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pSectorAround);

void GetUpdateSectorAround(st_PLAYER* pPlayer, st_SECTOR_AROUND* pRemoveSector, st_SECTOR_AROUND* pAddSector);

//캐릭터의 월드 좌표를토대로 섹터의 좌표를 가져오는 갱신하는 함수
bool Sector_UpdatePlayer(st_PLAYER* pPlayer);

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

unordered_map<SOCKET, st_SESSION*> g_Session_Map;//접속한 유저리스트
unordered_map<DWORD, st_PLAYER*> g_Player_Map;//플레이어 리스트
unordered_map<SOCKET, st_SESSION*> g_SendOnSessionList;//보낼유저리스트

//리스트하고 삭제과정을 좀 바꿔야될거 같은데
unordered_map<DWORD, st_PLAYER*>g_Sector[dfSECTOR_Y][dfSECTOR_X];//섹터 관리용 리스트
unordered_map<DWORD, st_PLAYER*>g_Delete_list;//삭제용 아이디목록

st_SECTOR_AROUND SECTOR_ARRAY[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_UU[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_LU[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_LL[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_LD[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_DD[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_RD[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_RR[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용
st_SECTOR_AROUND SECTOR_ARRAY_RU[dfSECTOR_Y][dfSECTOR_X];//주변섹터 검색용

int sync_start = 0;
int sync_stop = 0;
int sync_attack1 = 0;
int sync_attack2 = 0;
int sync_attack3 = 0;

CMemoryPool<st_SESSION> _SessionPool(4000, FALSE);//세션 오브젝트 풀
CMemoryPool<st_PLAYER> _PlayerPool(4000, FALSE);//플레이어 오브젝트 풀
CMemoryPool<CSerealBuffer> _PacketPool(50, FALSE);//패킷풀

//섹터를 처리할때
//섹터처리 문제에서 반복문을 최대한 줄이려면
//계산식을 줄여야한다.
//내가 왼쪽으로 섹터가 이동하면
//과거위치와 현재위치값으로 방향을 파악하고
// swtich case로 판별한후 반복문을 줄이기위한 함수를 실행한다.
// 함수의 개수는 총6개 insert와 delete를 구분해서 16개만든다
// 그냥 유지보수를 편하게 하기위해서

//우선 이방식으로 테스트해보고
//추후에는 값자체가 저장되는 방식으로 가보자

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

	while (!g_bShutdown)
	{
		NetWork();//네트워크
		Update();//로직
		//ServerControl();//서버 키보드컨트롤
		//Monitor();//모니터링값 콘솔 출력
	}
}

void init_Game()
{
	
	srand(1);
	HANDLE hThread = GetCurrentThread();
	//윈도우에 여러가지 프로그램 켜두니까 들쭉날쭉함
	//하나의 코어에서 우선순위를 가장 최상위로 올림
	//SetProcessAffinityMask(hThread,3);
	//SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

	//0    1     2      3      4       5        6       7
	//위 왼쪽위 왼쪽 왼쪽아래 아래 오른쪽아래 오른쪽 오른쪽위
	int lineX[8] = { 0, -1, -1, -1, 0, 1, 1,  1 };
	int lineY[8] = { -1, -1,  0,  1, 1, 1, 0, -1 };
	//st_SECTOR_AROUND SECTOR_ARRAY[dfSECTOR_Y][dfSECTOR_X];

	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			for (int iNodeSet = 0; iNodeSet < 9; iNodeSet++) //시계 방향으로 데이터 인풋
			{
				SECTOR_ARRAY[iSECTOR_Y][iSECTOR_X].Arroud[iNodeSet]._X = iSECTOR_X + lineX[iNodeSet];
				SECTOR_ARRAY[iSECTOR_Y][iSECTOR_X].Arroud[iNodeSet]._Y = iSECTOR_Y + lineY[iNodeSet];
				SECTOR_ARRAY[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			SECTOR_ARRAY[iSECTOR_Y][iSECTOR_X].Arroud[8]._X = iSECTOR_X;
			SECTOR_ARRAY[iSECTOR_Y][iSECTOR_X].Arroud[8]._Y = iSECTOR_Y;
		}
	}

	//ADD_UU값 모두계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			if (iSECTOR_Y == 0)
				continue;

			if (iSECTOR_X != 0) {
				SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTUP];
				SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTUP];
				SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfUP];
			SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfUP];
			SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount++;
			if (iSECTOR_X != (dfSECTOR_X - 1))
			{
				SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTUP];
				SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTUP];
				SECTOR_ARRAY_UU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//ADD_LL값 모두계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			if (iSECTOR_X == 0)
				continue;

			if (iSECTOR_Y != 0) {
				SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTUP];
				SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTUP];
				SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFT];
			SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFT];
			SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount++;
			if (iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTDOWN];
				SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTDOWN];
				SECTOR_ARRAY_LL[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//ADD_DD값 모두계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			if (iSECTOR_Y == (dfSECTOR_Y - 1))
				continue;

			if (iSECTOR_X != 0) {
				SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTDOWN];
				SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTDOWN];
				SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfDOWN];
			SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfDOWN];
			SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount++;
			if (iSECTOR_X != (dfSECTOR_X - 1))
			{
				SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTDOWN];
				SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTDOWN];
				SECTOR_ARRAY_DD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//ADD_RR값 모두계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			if (iSECTOR_X == (dfSECTOR_X - 1))
				continue;

			if (iSECTOR_Y != 0) {
				SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTUP];
				SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTUP];
				SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHT];
			SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHT];
			SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount++;
			if (iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTDOWN];
				SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTDOWN];
				SECTOR_ARRAY_RR[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//LU값 모두 계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			//XY값이 0이라면 보내야하는 섹션은 없다.
			if (iSECTOR_Y == 0 && iSECTOR_X == 0)
				continue;
			//왼쪽아래
			if (iSECTOR_X != 0 && iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTDOWN];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTDOWN];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//왼쪽
			if (iSECTOR_X != 0)
			{
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFT];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFT];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//왼쪽위
			if (iSECTOR_X != 0 && iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTUP];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTUP];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//위
			if (iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfUP];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfUP];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽위
			if (iSECTOR_X != (dfSECTOR_X - 1) && iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTUP];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTUP];
				SECTOR_ARRAY_LU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//LD값 모두 계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			//XY값이 0이라면 보내야하는 섹션은 없다.
			if (iSECTOR_Y == (dfSECTOR_Y - 1) && iSECTOR_X == 0)
				continue;

			//왼쪽위
			if (iSECTOR_X != 0 && iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTUP];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTUP];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//왼쪽
			if (iSECTOR_X != 0)
			{
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFT];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFT];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//왼쪽아래
			if (iSECTOR_X != 0 && iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTDOWN];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTDOWN];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//아래
			if (iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfDOWN];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfDOWN];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽아래
			if (iSECTOR_X != (dfSECTOR_X - 1) && iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTDOWN];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTDOWN];
				SECTOR_ARRAY_LD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//RD값 모두 계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			//XY값이 0이라면 보내야하는 섹션은 없다.
			if (iSECTOR_Y == (dfSECTOR_Y - 1) && iSECTOR_X == (dfSECTOR_X - 1))
				continue;

			//왼쪽아래
			if (iSECTOR_X != 0 && iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTDOWN];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTDOWN];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//아래
			if (iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfDOWN];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfDOWN];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽아래
			if (iSECTOR_X != (dfSECTOR_X - 1) && iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTDOWN];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTDOWN];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽
			if (iSECTOR_X != (dfSECTOR_X - 1))
			{
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHT];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHT];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽위
			if (iSECTOR_X != (dfSECTOR_X - 1) && iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTUP];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTUP];
				SECTOR_ARRAY_RD[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//RU값 모두 계산하기
	for (int iSECTOR_Y = 0; iSECTOR_Y < dfSECTOR_Y; iSECTOR_Y++)
	{
		for (int iSECTOR_X = 0; iSECTOR_X < dfSECTOR_X; iSECTOR_X++)
		{
			//XY값이 0이라면 보내야하는 섹션은 없다.
			if (iSECTOR_Y == 0 && iSECTOR_X == (dfSECTOR_X - 1))
				continue;

			//왼쪽위
			if (iSECTOR_X != 0 && iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfLEFTUP];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfLEFTUP];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//위
			if (iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfUP];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfUP];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽위
			if (iSECTOR_X != (dfSECTOR_X - 1) && iSECTOR_Y != 0)
			{
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTUP];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTUP];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽
			if (iSECTOR_X != (dfSECTOR_X - 1))
			{
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHT];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHT];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
			//오른쪽아래
			if (iSECTOR_X != (dfSECTOR_X - 1) && iSECTOR_Y != (dfSECTOR_Y - 1))
			{
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._X = iSECTOR_X + lineX[dfRIGHTDOWN];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].Arroud[SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount]._Y = iSECTOR_Y + lineY[dfRIGHTDOWN];
				SECTOR_ARRAY_RU[iSECTOR_Y][iSECTOR_X].iCount++;
			}
		}
	}

	//한글세팅
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");
};

void ServerControl()
{

};

void Monitor()
{
	//printf("g_Player_Map %d\n",g_Player_Map.size());
	//printf("g_Session_Map %d\n", g_Session_Map.size());
	//printf("sector_users %d\n", sector_users);
	//int sync_start = 0;
	//int sync_stop = 0;
	//int sync_attack1 = 0;
	//int sync_attack2 = 0;
	//int sync_attack3 = 0;
	//system("cls");
	//printf("sync_start %d\n", sync_start);
	//printf("sync_stop %d\n", sync_stop);
	//printf("sync_attack1 %d\n", sync_attack1);
	//printf("sync_attack2 %d\n", sync_attack2);
	//printf("sync_attack3 %d\n", sync_attack3);
};

void NetWork()
{
	SOCKET UserTable_SOCKET[FD_SETSIZE] = { INVALID_SOCKET, };
	int iSocketCount = 0;

	FD_SET rset;
	FD_SET wset;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(listen_sock, &rset);//리슨소켓을 rset소켓에 세팅한다
	UserTable_SOCKET[iSocketCount] = listen_sock;

	iSocketCount++;

	//모든 접속중인 클라이언트에 대해 SOCKET을 체크한다.
	st_SESSION* pSession;
	for (auto session_it = g_Session_Map.begin(); session_it != g_Session_Map.end();)
	{
		pSession = session_it->second;

		UserTable_SOCKET[iSocketCount] = pSession->_Sock;

		FD_SET(pSession->_Sock, &rset);//반응이 있는recv Socket을 모두 넣는다.

		if (pSession->_SendBuf.GetUseSize() > 0)//세션 버퍼에 보내야하는 데이터가 0이상이라면 FD세팅을 한다.
		{
			FD_SET(pSession->_Sock, &wset);
		}
		iSocketCount++;
		session_it++;

		if (FD_SETSIZE <= iSocketCount)
		{
			netSelectSocket(UserTable_SOCKET, &rset, &wset);
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);

			FD_SET(listen_sock, &rset);
			UserTable_SOCKET[0] = listen_sock;

			iSocketCount = 1;
		}
	}

	if (iSocketCount > 0)
	{
		netSelectSocket(UserTable_SOCKET, &rset, &wset);
	}

	Disconnect_Clean();//연결이 끊긴 녀석들을 모두제거한다.
};

void netSelectSocket(SOCKET* pTableSocket, FD_SET* pReadSet, FD_SET* pWriteSet)
{
	timeval times;
	times.tv_sec = 0;
	times.tv_usec = 0;

	int iResult;
	int iCnt;
	bool bProcFlag;

	//64개 이상의 소켓에선 본 함수를 여러번 호출하면서 소켓을 확인해야하므로
	//타임아웃을 0으로 잡아야한다. 그렇지 않으면 뒤쪽 소켓들 검사 타이밍이 점점 늦어진다.
	iResult = select(0, pReadSet, pWriteSet, 0, &times);

	if (0 < iResult)
	{
		//table소켓을 돌면섯어던 소켓에 반응이 있었는지 확인  
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

DWORD old = timeGetTime();
DWORD cur;
//DWORD sec = timeGetTime();
DWORD deltaTime = 0;

void Update()
{
	cur = timeGetTime();
	deltaTime += cur - old;
	old = cur;
	dwCurtick = cur;
	if (deltaTime < dfFrame_time) return;

	//dwCurtick = timeGetTime();
	////50fps 간단 로직
	//LogicTime = timeGetTime() - dwStarttick;

	//if (LogicTime < 20)//전 시점에서 20ms가 지나지 않았다면 로직은 수행하지 않는다.
	//	return;
	//else //20미리 이상 지났다면 다시 시간초기화
	//	dwStarttick = dwCurtick;

	if (deltaTime >= dfFrame_time)
	{
		deltaTime -= dfFrame_time;
	}

	//모든 유저를 돌면서 로직을 수행한다.

	for (auto _Player_it = g_Player_Map.begin(); _Player_it != g_Player_Map.end(); ++_Player_it)
	{
		st_PLAYER* pPlayer = _Player_it->second;

		if (pPlayer->_Live == false)//죽어있는 세션이라면 계산을 하지 않는다.어차피 연결끊길거니까
		{
			continue;
		}

		//30초이상 지난세션은 제거한다.
		if (dwCurtick - pPlayer->pSession->_LastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)//30초 이상 지난 세션은 제거한다.
		{
			Disconnect(pPlayer->pSession);
			continue;
		}

		if (pPlayer->_HP <= 0)//Hp가 전부달아버린 세션이라면 세션을 죽이고 로직계산을 하지 않는다.
		{
			Disconnect(pPlayer->pSession);
			continue;
		}

		if (pPlayer->_Direction_check == ON_STOP)//움직이지 않아도 되는세션이라면 계산하지 않는다.
		{
			continue;
		}


		if (pPlayer->_X < dfRANGE_MOVE_TOP) continue;
		else if (pPlayer->_X > dfRANGE_MOVE_RIGHT) continue;
		else if (pPlayer->_Y > dfRANGE_MOVE_BOTTOM) continue;
		else if (pPlayer->_Y < dfRANGE_MOVE_TOP) continue;

		////범위를 초과하고 있는 세션은 더이상 로직에서 움직이게 하지 않는다.
		//if (pPlayer->_X < dfRANGE_MOVE_LEFT
		//	|| pPlayer->_X > dfRANGE_MOVE_RIGHT
		//	|| pPlayer->_Y < dfRANGE_MOVE_TOP
		//	|| pPlayer->_Y > dfRANGE_MOVE_BOTTOM)
		//{
		//	continue;
		//}

		//살아있으면서 움직이고 있는 세션이라면 원하는 방향으로 값을 이동시킨다.
		switch (pPlayer->_Direction)
		{
			case dfPACKET_MOVE_DIR_LL:
			{
				pPlayer->_X -= defualt_MOVE_X;
			}
			break;
			case dfPACKET_MOVE_DIR_LU:
			{
				pPlayer->_X -= defualt_MOVE_X;
				pPlayer->_Y -= defualt_MOVE_Y;
			}
			break;
			case dfPACKET_MOVE_DIR_UU:
			{
				pPlayer->_Y -= defualt_MOVE_Y;
			}
			break;
			case dfPACKET_MOVE_DIR_RU:
			{
				pPlayer->_X += defualt_MOVE_X;
				pPlayer->_Y -= defualt_MOVE_Y;
			}
			break;
			case dfPACKET_MOVE_DIR_RR:
			{
				pPlayer->_X += defualt_MOVE_X;
			}
			break;
			case dfPACKET_MOVE_DIR_RD:
			{
				pPlayer->_X += defualt_MOVE_X;
				pPlayer->_Y += defualt_MOVE_Y;
			}
			break;
			case dfPACKET_MOVE_DIR_DD:
			{
				pPlayer->_Y += defualt_MOVE_Y;
			}
			break;
			case dfPACKET_MOVE_DIR_LD:
			{
				pPlayer->_X -= defualt_MOVE_X;
				pPlayer->_Y += defualt_MOVE_Y;
			}
			break;
			default:
				continue;
		}
		SectorUpdate(pPlayer);
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

	unordered_map<DWORD, st_PLAYER*>* pAcceptSectorList;

	Create_User_Session->_Con_Addr = clientaddr;
	Create_User_Session->_LastRecvTime = timeGetTime();
	Create_User_Session->_SendBuf.ClearBuffer();//오브젝트와 플레이어 풀에서 가져오는거기 때문에 초기화가 필요하다.
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

	Create_Player->_X = (rand() % 6390) + 10;//X세팅
	Create_Player->_Y = (rand() % 6390) + 10;//Y세팅
	//Create_Player->_X = df_TEST_X;//X세팅
	//Create_Player->_Y = df_TEST_Y;//Y세팅
	Create_Player->_Live = true;//세션 생존플래그 0이면 사망
	Create_Player->_HP = df_HP;

	//생성된 유저 넣기
	g_Session_Map.emplace(Create_User_Session->_Sock, Create_User_Session);
	g_Player_Map.emplace(Create_Player->_Id, Create_Player);

	//섹터값 업데이트
	Create_Player->_cur_Pos._X = Create_Player->_X / dfSECTOR_SIZE;
	Create_Player->_cur_Pos._Y = Create_Player->_Y / dfSECTOR_SIZE;
	Create_Player->_old_Pos._X = Create_Player->_cur_Pos._X;
	Create_Player->_old_Pos._Y = Create_Player->_cur_Pos._Y;

	pAcceptSectorList = &g_Sector[Create_Player->_cur_Pos._Y][Create_Player->_cur_Pos._X];
	//생성된 유저를 특정 섹터에 넣는다.
	pAcceptSectorList->emplace(Create_Player->_Id, Create_Player);

	//2.생성된 유저 데이터 나에게 전송하기
	bool retProc;
	retProc = PacketProc(Create_User_Session, dfPACKET_SC_CREATE_MY_CHARACTER, nullptr);

	if (!retProc)
	{
		Disconnect(Create_User_Session);
		return;
	}

	//3.현재 주변섹터에 있는 유저들을 나에게 전송해야됨 지금은 내위치만 하고있음
	int AcceptlineX[9] = { 0, -1, -1, -1, 0, 1, 1,  1 ,0 };
	int AcceptlineY[9] = { -1, -1,  0,  1, 1, 1, 0, -1 ,0 };
	st_PLAYER* All_List_Player_it;

	st_dfPACKET_header st_other_header;
	st_other_header.byCode = PACKET_CODE;
	st_other_header.bySize = sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER);
	st_other_header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	//주변 9섹터의 유저를 나에게 전송
	for (int iCnt = 0; iCnt < 9; iCnt++)
	{
		int Cur_X = Create_Player->_cur_Pos._X + AcceptlineX[iCnt];
		int Cur_Y = Create_Player->_cur_Pos._Y + AcceptlineY[iCnt];

		if (!Sector_Check(Cur_X, Cur_Y))//섹터 범위를 벗어난게 있다면 전송안함
			continue;

		pAcceptSectorList =
			&g_Sector[Create_Player->_cur_Pos._Y + AcceptlineY[iCnt]]
			[Create_Player->_cur_Pos._X + AcceptlineX[iCnt]];


		for (auto _Player_it = pAcceptSectorList->begin(); _Player_it !=
			pAcceptSectorList->end(); ++_Player_it)
		{
			All_List_Player_it = _Player_it->second;

			if (All_List_Player_it->_Id == Create_Player->_Id)//나자신이라면 제외한다.
			{
				continue;
			}

			if (All_List_Player_it->_Live == true)//리스트안에 있으면서 살아있는 세션이라면
			{
				CSerealBuffer* pPacket = _PacketPool.Alloc();
				pPacket->Clear();
				pPacket->PutData((char*)&st_other_header, sizeof(st_dfPACKET_header));
				*pPacket << All_List_Player_it->_Id
					<< All_List_Player_it->_Direction
					<< All_List_Player_it->_X
					<< All_List_Player_it->_Y
					<< All_List_Player_it->_HP;
				sendPacket_UniCast(Create_User_Session, pPacket);//나 자신을 제외하고 데이터전부인풋
				pPacket->Clear();
				_PacketPool.Free(pPacket);
			}
		}
	}

	//4.생성된 세션 리스트에 넣기
	//Session_Map.emplace(Create_User_Session->_Sock, Create_User_Session);
	//Session_List.push_back(NewSession);

	//5.생성된 나를 지금 주변섹터에 있는 유저에게 전달한다.
	CSerealBuffer* pPacket = _PacketPool.Alloc();
	pPacket->Clear();
	*pPacket << Create_Player->_Id;
	*pPacket << Create_Player->_Direction;
	*pPacket << Create_Player->_X;
	*pPacket << Create_Player->_Y;
	*pPacket << Create_Player->_HP;
	PacketProc(Create_User_Session, dfPACKET_SC_CREATE_OTHER_CHARACTER, pPacket);
	pPacket->Clear();
	_PacketPool.Free(pPacket);
};//접속유저가 새로 생겼을때



void RecvProc(SOCKET pSocket)//유저가 데이터를 보내왔을때
{
	st_SESSION* tmpSession;
	tmpSession = Find_Session(pSocket);

	if (tmpSession == nullptr)
	{
		return;
	}

	tmpSession->_LastRecvTime = timeGetTime();//리시브할때마다 타임아웃 시간 업데이트 30초이상 지나면 끊어버릴것이다.

	int retRecv;
	int DirectEnq = tmpSession->_RecvBuf.DirectEnqueueSize();
	retRecv = recv(tmpSession->_Sock, (char*)tmpSession->_RecvBuf.GetRearBufferPtr(),//버퍼에 꼽을수 있는만큼 꽂아버린다.
		DirectEnq, 0);//한번에 넣을수있는 사이즈만큼 넣는다.

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

	CSerealBuffer* cpPacket = _PacketPool.Alloc();//데이터 전송을 위한 직렬화 버퍼
	cpPacket->Clear();
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

		//직렬화버퍼 작업
		int retSwitchDeq;
		retSwitchDeq = tmpSession->_RecvBuf.Dequeue(cpPacket->GetBufferPtr(), st_recv_header.bySize);//사이즈만큼 직렬화 버퍼에 디큐한다.

		if (retSwitchDeq != st_recv_header.bySize)//헤더에 적힌 사이즈만큼 있다고 판별된 상황에서 사이즈만큼 가져오지 못한다면 버퍼가 깨진것이므로 연결을 끊고 세션을 파괴한다.
		{
			Disconnect(tmpSession);
			return;
		}

		int retCS;
		retCS = cpPacket->MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

		//if (retCS != retSwitchDeq)//직렬화 버퍼가 이동하지 못했다면 파괴한다.
		//{
		//	Disconnect(tmpSession);
		//	return;
		//}

		bool retPacket;
		PacketProc(tmpSession, st_recv_header.byType, cpPacket);//패킷절차가 실패했다면?

		cpPacket->Clear();//직렬화 버퍼 재사용을 위한 초기화
	}
	cpPacket->Clear();
	_PacketPool.Free(cpPacket);
	if (tmpSession->_SendBuf.GetUseSize() == 0)
		tmpSession->_SendBuf.ClearBuffer();
};

bool SendProc(SOCKET pSocket)
{
	st_SESSION* tmpSession;
	tmpSession = Find_Session(pSocket);

	if (tmpSession == nullptr)
		return false;

	//int useSize = tmpSession->_SendBuf.GetUseSize();
	//if (useSize == 0) return false;

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

	//if (retMF != retSend)
	//{
	//	Disconnect(tmpSession);//연결종료처리
	//	return false;//링버퍼가 꺠졌다는 뜻이므로 종료
	//}
	if (tmpSession->_SendBuf.GetUseSize() == 0)
		tmpSession->_SendBuf.ClearBuffer();

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

void sendBroadCast(st_SESSION* session, CSerealBuffer* pPacket)
{
	int retBro;
	st_SESSION* stmp = session;//보내지 않을 유저

	//모든 리스트를돌면서 확인한다.
	st_SESSION* Broad_Sesion;
	for (auto session_it = g_Session_Map.begin(); session_it != g_Session_Map.end(); ++session_it)
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
	for (auto session_it = g_Session_Map.begin(); session_it != g_Session_Map.end(); ++session_it)
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
	////끊을 플레이어를 찾는다.
	st_PLAYER* removePlayer;
	auto pItem = g_Player_Map.find(session->_Id);
	if (pItem != g_Player_Map.end())//일치하는 녀석이 존재한다면
	{
		removePlayer = pItem->second;
		removePlayer->_Live = false;//그녀석의 생존값을 false로 바꿔준다.
		removePlayer->pSession->_Live = false;
		g_Delete_list.emplace(removePlayer->_Id, removePlayer);

		//현재 내섹터에서 나를 제거한다.
		//g_Sector[removePlayer->_cur_Pos._Y][removePlayer->_cur_Pos._X].erase(removePlayer->_Id);
		////제거된 나를 모두에게 전송한다.
		//PacketProc(removePlayer->pSession, dfPACKET_SC_DELETE_CHARACTER, nullptr);;

		//removePlayer->pSession->_RecvBuf.ClearBuffer();
		//removePlayer->pSession->_SendBuf.ClearBuffer();
		//Session_Map.erase(removePlayer->pSession->_Sock);
		//_SessionPool.Free(removePlayer->pSession);
		return;
	}
};

void Disconnect_Clean()//안정성을 위해 디스커넥트된 개체를 네트워크 마지막시점에 정리한다.
{
	DWORD _retDel;
	st_PLAYER* _Player_Object;
	st_SESSION* _Session_Object;
	for (auto _Delete_it = g_Delete_list.begin(); _Delete_it != g_Delete_list.end(); ++_Delete_it)
	{
		_Player_Object = _Delete_it->second;
		_Session_Object = _Player_Object->pSession;

		//제거된 나를 모두에게 전송한다.
		PacketProc(_Player_Object->pSession, dfPACKET_SC_DELETE_CHARACTER, nullptr);;
		g_Sector[_Player_Object->_cur_Pos._Y][_Player_Object->_cur_Pos._X].erase(_Player_Object->_Id);
		_Session_Object->_RecvBuf.ClearBuffer();
		_Session_Object->_SendBuf.ClearBuffer();
		g_Session_Map.erase(_Session_Object->_Sock);
		g_Player_Map.erase(_Player_Object->_Id);
		closesocket(_Session_Object->_Sock);
		_SessionPool.Free(_Session_Object);
		_PlayerPool.Free(_Player_Object);
	}
	g_Delete_list.clear();
	//DWORD _retDel;
	//st_PLAYER* _Player_Object;
	//st_SESSION* _Session_Object;
	////삭제 리스트를 돌면서 확인
	//for (auto _Delete_it = g_Delete_list.begin(); _Delete_it != g_Delete_list.end(); ++_Delete_it)
	//{
	//	_retDel = *_Delete_it;
	//	_Player_Object = Find_Player(_retDel);
	//	if (_Player_Object == nullptr)
	//	{
	//		wprintf(L"LOG : Disconnect_Clean %d 유저 없는데 검색중\n", _retDel);
	//		continue;
	//	}

	//	_Session_Object = _Player_Object->pSession;//받은 세션 리스트 가져오기

	//	//해당 섹터리스트를 돌면서 같은 아이디를 가진 객체를 제거
	//	for (auto _Sector_it = g_Sector[_Player_Object->_cur_Pos._Y][_Player_Object->_cur_Pos._X].begin();
	//		_Sector_it != g_Sector[_Player_Object->_cur_Pos._Y][_Player_Object->_cur_Pos._X].end(); ++_Sector_it)
	//	{
	//		if (_Player_Object->_Id == (*_Sector_it)->_Id)
	//		{
	//			g_Sector[_Player_Object->_cur_Pos._Y][_Player_Object->_cur_Pos._X].erase(_Sector_it);
	//			break;//나자신을 섹터에서 제거한다.
	//		}
	//	}

	//	//죽었으므로 주변유저에게 그값을 전달한다.
	//	PacketProc(_Session_Object, dfPACKET_SC_DELETE_CHARACTER, nullptr);//제거되는 세션아이디 전달
	//	closesocket(_Session_Object->_Sock);//소켓을 끊고 연결을 종료한다.
	//	//rst를 보내서 time wait을 남기지 않을것이고
	//	_Session_Object->_SendBuf.ClearBuffer();//링버퍼 초기화Alloc 사용을 대비해서 버퍼초기화
	//	_Session_Object->_RecvBuf.ClearBuffer();//링버퍼 초기화
	//	Session_Map.erase(_Session_Object->_Sock);//맵에서 해당세션제거
	//	_SessionPool.Free(_Session_Object);//오브젝트풀에 세션리스트 반납

	//	g_Player_Map.erase(_Player_Object->_Id);//플레이어 맵에서 해당 플레이어 제거
	//	_PlayerPool.Free(_Player_Object);//오브젝트풀에 플레이어 오브젝트를 반납한다.
	//}
	//g_Delete_list.clear();//모든 유저가 삭제됬으므로 딜리트 리스트를 비운다.
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

	CSerealBuffer* cpPacket = _PacketPool.Alloc();
	cpPacket->Clear();
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

		retSwitchDeq = tmpSesion->_RecvBuf.Dequeue(cpPacket->GetBufferPtr(), st_recv_header.bySize);//사이즈만큼 직렬화 버퍼에 디큐한다.

		if (retSwitchDeq != st_recv_header.bySize)//헤더에 적힌 사이즈만큼 있다고 판별된 상황에서 사이즈만큼 가져오지 못한다면 버퍼가 깨진것이므로 연결을 끊고 세션을 파괴한다.
		{
			Disconnect(tmpSesion);
			return;
		}

		retCS = cpPacket->MoveWritePos(retSwitchDeq);//데이터를 받은만큼 직렬화 버퍼를 이동시킨다.

		if (retCS != retSwitchDeq)//직렬화 버퍼가 이동하지 못했다면 파괴한다.
		{
			Disconnect(tmpSesion);
			return;
		}

		PacketProc(tmpSesion, st_recv_header.byType, cpPacket);

		cpPacket->Clear();//직렬화 버퍼 재사용을 위한 초기화
	}
	cpPacket->Clear();
	_PacketPool.Free(cpPacket);
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
	auto S_Iter = g_Session_Map.find(sock);

	if (S_Iter != g_Session_Map.end())//찾음
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

//특정 1명의 클라이언트에게 보내기
void sendPacket_UniCast(st_SESSION* session, CSerealBuffer* pPacket)
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
};

//특정 섹터 1개에 있는 모든 클라에게 보내기
void sendPacket_SectorOne(st_SESSION* session, CSerealBuffer* pPacket, int iSectorX, int iSectorY)
{
	//list<st_PLAYER*>g_Sector[dfSECTOR_Y][dfSECTOR_X];//섹터 관리용 리스트

	st_SESSION* stmp = session;//보내지 않을 유저

	//받은 섹터 안을 돌면서 그안에 있는 유저 센드버퍼에 Enqueue한다.
	st_PLAYER* Sector_Player;

	for (auto Player_it = g_Sector[iSectorY][iSectorX].begin();
		Player_it != g_Sector[iSectorY][iSectorX].end(); ++Player_it)
	{
		Sector_Player = Player_it->second;
		if (stmp != nullptr && stmp->_Id == Sector_Player->_Id)//값이 같다면 이녀석은 보내지 않을녀석이니 캔슬한다.
		{
			continue;
		}

		if (Sector_Player->_Live == false)
		{
			continue;//죽어있는패킷한테는안보낸다.
		}

		//sendPacket_UniCast(Sector_Player->pSession, pPacket);
		//각세션의 센드버퍼에 enqueue시킨다.
		if (Sector_Player->pSession->_SendBuf.GetFreeSize() >= pPacket->GetUseSize())//sendbuf에 넣을수있는 크기가 남아있다면
		{
			int retBro;
			retBro = Sector_Player->pSession->_SendBuf.Enqueue(pPacket->GetBufferPtr(), pPacket->GetUseSize());//안들어가면 안들어가는데로 별상관없음 버퍼가 꽉찬거니까
			if (retBro != pPacket->GetUseSize())//센드버퍼 자체가 고장났거나 송신버퍼가 가득찬경우 이므로 디스커넥트하고 끝내버린다.
			{
				Disconnect(Sector_Player->pSession);
			}
		}
	}
};

//특정유저 주변 섹터에만 보내기
void sendPacket_Around(st_SESSION* pSession, CSerealBuffer* pPacket, bool bSendMe)
{
	//bSendMe true라면 나에게 보내지않는다.
	//false라면 나도 포함해서 보낸다.
	int retBro;
	st_PLAYER* Sector_Player;
	Sector_Player = Find_Player(pSession->_Id);
	if (Sector_Player == nullptr)
	{
		return;
	}
	int Send_X;
	int Send_Y;
	for (int sector_count = 0; sector_count < 9; sector_count++)
	{
		//자기주변 9개 섹터 값 계산해서 데이터 보내기
		//0부터 위 왼쪽위 왼쪽 왼쪽아래 아래 오른쪽아래 오른쪽 오른쪽위 나 자신 순서다.
		Send_X = SECTOR_ARRAY[Sector_Player->_cur_Pos._Y]
			[Sector_Player->_cur_Pos._X].Arroud[sector_count]._X;
		Send_Y = SECTOR_ARRAY[Sector_Player->_cur_Pos._Y]
			[Sector_Player->_cur_Pos._X].Arroud[sector_count]._Y;

		if (!Sector_Check(Send_X, Send_Y))//섹터 범위를 벗어난게 있다면 전송안함
		{
			continue;
		}

		if (bSendMe)//나자신을 제외해서 데이터를 보낼것인지
		{
			sendPacket_SectorOne(pSession, pPacket, Send_X, Send_Y);
		}
		else
		{
			sendPacket_SectorOne(nullptr, pPacket, Send_X, Send_Y);
		}
	}
};

//모든 유저에게 패킷쏘기 이걸 쓸일은 사실상 없다고 보면됨
void sendPacket_BroadCast(st_SESSION* session, CSerealBuffer* pPacket)
{

};

st_SECTOR_AROUND GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pSectorAround)
{
	//pSectorAround->Arroud = &SECTOR_ARRAY[iSectorY][iSectorX];
	return SECTOR_ARRAY[iSectorY][iSectorX];
};

void GetUpdateSectorAround(st_PLAYER* pPlayer, st_SECTOR_AROUND* pRemoveSector, st_SECTOR_AROUND* pAddSector)
{
	//if ((pPlayer->_cur_Pos._X == pPlayer->_old_Pos._X) &&
	//	(pPlayer->_cur_Pos._Y == pPlayer->_old_Pos._Y))
	//{
	//	//최초 접속시의 좌표이기 때문에 무시한다.
	//	pRemoveSector = nullptr;
	//	pAddSector = nullptr;
	//	return;
	//}

	//0    1     2      3      4       5        6       7
	//위 왼쪽위 왼쪽 왼쪽아래 아래 오른쪽아래 오른쪽 오른쪽위
	int lineX[8] = { 0, -1, -1, -1, 0, 1, 1,  1 };
	int lineY[8] = { -1, -1,  0,  1, 1, 1, 0, -1 };

	int _Directer = 9;
	for (int iDIR = 0; iDIR < 8; iDIR++)
	{
		//계산하는값중에 일치하는 방향값을 뽑아낸다.
		//예: 만약 IDIR이 0이라면
		//old_x= cur_x+0
		//old_Y= cur_y+(-1) 이될것이고 만약이 값이 같다는것은
		//섹터이동이 발생했고 방향이 위쪽이었다는것이다.
		//그방향값을 넣고 스위치 케이스로 해당하는 방향을 판단해서 RemoveSector와 AddSector을 넣어준다.
		if ((pPlayer->_cur_Pos._X == (pPlayer->_old_Pos._X + lineX[iDIR])) &&
			(pPlayer->_cur_Pos._Y == (pPlayer->_old_Pos._Y + lineY[iDIR])))
		{
			_Directer = iDIR;//일치하는 방향값 확인
			break;
		}
	}

	switch (_Directer)
	{
	case dfUP:
		*pAddSector = SECTOR_ARRAY_UU[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_DD[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfLEFTUP:
		*pAddSector = SECTOR_ARRAY_LU[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_RD[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfLEFT:
		*pAddSector = SECTOR_ARRAY_LL[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_RR[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfLEFTDOWN:
		*pAddSector = SECTOR_ARRAY_LD[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_RU[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfDOWN:
		*pAddSector = SECTOR_ARRAY_DD[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_UU[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfRIGHTDOWN:
		*pAddSector = SECTOR_ARRAY_RD[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_LU[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfRIGHT:
		*pAddSector = SECTOR_ARRAY_RR[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_LL[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	case dfRIGHTUP:
		*pAddSector = SECTOR_ARRAY_RU[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X];
		*pRemoveSector = SECTOR_ARRAY_LD[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X];
		break;
	}
};

//캐릭터의 월드 좌표를토대로 섹터의 좌표를 갱신하는 함수
bool Sector_UpdatePlayer(st_PLAYER* pPlayer)
{
	int Cur_X;
	int Cur_Y;
	int Old_X;
	int Old_Y;

	Cur_X = pPlayer->_X / dfSECTOR_SIZE;
	Cur_Y = pPlayer->_Y / dfSECTOR_SIZE;

	if (!Sector_Check(Cur_X, Cur_Y))//섹터 범위를 벗어난게 있다면 전송안함
	{
		return false;
	}

	Old_X = pPlayer->_old_Pos._X;//Old X 저장
	Old_Y = pPlayer->_old_Pos._Y;//Old Y 저장

	pPlayer->_old_Pos._X = pPlayer->_cur_Pos._X;
	pPlayer->_old_Pos._Y = pPlayer->_cur_Pos._Y;

	pPlayer->_cur_Pos._X = Cur_X;
	pPlayer->_cur_Pos._Y = Cur_Y;

	if (pPlayer->_cur_Pos._X >= dfSECTOR_X)
		pPlayer->_cur_Pos._X = dfSECTOR_X - 1;
	if (pPlayer->_cur_Pos._Y >= dfSECTOR_Y)
		pPlayer->_cur_Pos._Y = dfSECTOR_Y - 1;
	if (pPlayer->_cur_Pos._X < 0)
		pPlayer->_cur_Pos._X = 0;
	if (pPlayer->_cur_Pos._Y < 0)
		pPlayer->_cur_Pos._Y = 0;

	//섹터좌표가 변경이 있었다면?
	//둘중 하나라도 다르다면 섹터의 변화가 있었다는 뜻이다.
	if ((Cur_X != Old_X) ||
		(Cur_Y != Old_Y))
	{
		g_Sector[Cur_Y][Cur_X].emplace(pPlayer->_Id, pPlayer);//새로운 섹터에 나를 추가하고
		//예전 섹터에서 나를 제거한다.
		auto del_iter = g_Sector[Old_Y][Old_X].find(pPlayer->_Id);
		if (del_iter != g_Sector[Old_Y][Old_X].end())
		{
			g_Sector[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X].erase(del_iter);
		}
		//for (auto _Sector_iter = g_Sector[Old_Y][Old_X].begin();
		//	_Sector_iter != g_Sector[Old_Y][Old_X].end(); ++_Sector_iter)
		//{
		//	if ((*_Sector_iter)->_Id == pPlayer->_Id)//리스트를 순회하면서 아이디가 일치하는지 확인하고.
		//	{
		//		g_Sector[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X].erase(_Sector_iter);//일치한다면 제거
		//		//어차피 멀티스레드 아니니까 동기화 신경 안써도 그만이다.
		//		break;
		//	}
		//}
		//for (auto _Sector_iter = g_Sector[Old_Y][Old_X].begin();
		//	_Sector_iter != g_Sector[Old_Y][Old_X].end(); ++_Sector_iter)
		//{
		//	printf("ID %d\n",(*_Sector_iter)->_Id);
		//}
#ifdef df_LOG
		printf("LOG : SECTOR_CHANGE Cur_X : %d Cur_Y : %d Old_X : %d Old_Y : %d\n", Cur_X, Cur_Y, Old_X, Old_Y);
#endif
		return true;//모든게 종료된후 섹터변경이 있었다는걸 전달한다.
	}
	return false;//섹터변경이 없었다면 false리턴
};

bool Sector_Check(int _PX, int _PY)
{
	if (_PX < 0 ||
		_PX >= dfSECTOR_X ||
		_PY < 0 ||
		_PY >= dfSECTOR_Y)//섹터 범위를 벗어난게 있다면 전송안함
	{
		return false;
	}
	return true;
}

void PlayerSectorUpdatePacket(st_PLAYER* pPlayer)
{
	////여기에 들어왔다는건 섹터이동이 발생했다는뜻이다.
	//st_SECTOR_AROUND AddSector;
	//st_SECTOR_AROUND RemoveSector;

	//st_PLAYER* pExistPlayer;

	//list<st_PLAYER*>* pSectorList;
	//list<st_PLAYER*>::iterator ListIter;

	//CSerealBuffer cPacket;

	////지우는 색터와 내가 추가되어야 하는 섹터값을 받는다.
	//GetUpdateSectorAround(pPlayer, &RemoveSector, &AddSector);
	//mp_DELETE_CHARACTER(&cPacket, pPlayer->_Id);//제거되어야 하는곳에 나의 ID패킷을 만든다.

	////1.현재 Remove Sector에 나의 삭제정보 보내기
	//for (int iRemoveCnt = 0; iRemoveCnt < RemoveSector.iCount; iRemoveCnt++)
	//{
	//	sendPacket_SectorOne(pPlayer->pSession,
	//		&cPacket,
	//		RemoveSector.Arroud[iRemoveCnt]._X,
	//		RemoveSector.Arroud[iRemoveCnt]._Y);
	//}

	////2.지금 움직이는(나)유저에게 삭제되어야하는 패킷보내기
	//for (int iMoveCnt = 0; iMoveCnt < RemoveSector.iCount; iMoveCnt++)
	//{
	//	//해당 섹터의 유저들을 가져와서 나에게 전달.
	//	pSectorList = &g_Sector[RemoveSector.Arroud[iMoveCnt]._Y][RemoveSector.Arroud[iMoveCnt]._X];
	//	for (auto ListIter = pSectorList->begin(); ListIter != pSectorList->end(); ListIter++)
	//	{
	//		cPacket.Clear();
	//		mp_DELETE_CHARACTER(&cPacket, (*ListIter)->_Id);
	//		//특정 클라이언트 하나에게 전달
	//		sendPacket_UniCast(pPlayer->pSession, &cPacket);
	//	}
	//}

	////3.AddSector에 있는 유저들에게 캐릭터 생성 패킷 보내기
	//cPacket.Clear();
	////현재나의 위치 
	//mp_SC_CREATE_OTHER_CHARACTER(&cPacket, pPlayer->_Id,
	//	pPlayer->_Direction,
	//	pPlayer->_X,
	//	pPlayer->_Y,
	//	pPlayer->_HP);

	//for (int iCreateOne = 0; iCreateOne < AddSector.iCount; iCreateOne++)
	//{
	//	sendPacket_SectorOne(pPlayer->pSession, &cPacket,
	//		AddSector.Arroud[iCreateOne]._X,
	//		AddSector.Arroud[iCreateOne]._Y);
	//}

	////4.이동중인 나를 AddSector에 보내기
	//cPacket.Clear();
	//mp_MOVE_START(&cPacket, pPlayer->_Id,
	//	pPlayer->_Direction,
	//	pPlayer->_X,
	//	pPlayer->_Y);

	//for (int iMyMoveOne = 0; iMyMoveOne < AddSector.iCount; iMyMoveOne++)
	//{
	//	sendPacket_SectorOne(pPlayer->pSession, &cPacket,
	//		AddSector.Arroud[iMyMoveOne]._X,
	//		AddSector.Arroud[iMyMoveOne]._Y);
	//}

	////5.이동한 녀석에게 AddSector에 있는 캐릭터의 패킷을 보낸다.
	//for (int ilistAdd = 0; ilistAdd < AddSector.iCount; ilistAdd++)
	//{
	//	//얻어진 섹터에서 리스트 접근
	//	pSectorList = &g_Sector[AddSector.Arroud[ilistAdd]._Y][AddSector.Arroud[ilistAdd]._X];

	//	//해당 섹터마다 등록된 캐릭터들을 뽑아서 생성 패킷으로 만들어서 보낸다.
	//	for (auto AddListIter = pSectorList->begin(); AddListIter != pSectorList->end(); AddListIter++)
	//	{
	//		pExistPlayer = *AddListIter;

	//		//내가 아닌경우에만
	//		if (pExistPlayer != pPlayer)
	//		{
	//			cPacket.Clear();
	//			mp_SC_CREATE_OTHER_CHARACTER(&cPacket, pExistPlayer->_Id,
	//													pExistPlayer->_Direction,
	//													pExistPlayer->_X,
	//													pExistPlayer->_Y,
	//													pExistPlayer->_HP);
	//			sendPacket_UniCast(pPlayer->pSession, &cPacket);

	//			
	//			if (pExistPlayer->_Direction_check == ON_MOVE && pExistPlayer->_Live == true)
	//			{
	//				//움직이고 있는 유저이므로 움직이는 값을 던져준다.
	//				switch (pExistPlayer->_Direction)
	//				{
	//					case dfPACKET_MOVE_DIR_UU:
	//					case dfPACKET_MOVE_DIR_LU:
	//					case dfPACKET_MOVE_DIR_LL:
	//					case dfPACKET_MOVE_DIR_LD:
	//					case dfPACKET_MOVE_DIR_DD:
	//					case dfPACKET_MOVE_DIR_RD:
	//					case dfPACKET_MOVE_DIR_RU:
	//						cPacket.Clear();
	//						mp_MOVE_START(&cPacket, pExistPlayer->_Id,
	//												pExistPlayer->_Direction,
	//												pExistPlayer->_X,
	//												pExistPlayer->_Y);
	//						sendPacket_UniCast(pPlayer->pSession, &cPacket);
	//					break;
	//				}
	//			}
	//		}
	//	}
	//}
}


void SectorUpdate(st_PLAYER* pPlayer)
{
	bool sectorChanged = false;

	//old sector 백업
	int Old_X = pPlayer->_old_Pos._X;
	int Old_Y = pPlayer->_old_Pos._Y;
	pPlayer->_old_Pos._X = pPlayer->_cur_Pos._X;
	pPlayer->_old_Pos._Y = pPlayer->_cur_Pos._Y;
	pPlayer->_cur_Pos._X = pPlayer->_X / dfSECTOR_SIZE;
	pPlayer->_cur_Pos._Y = pPlayer->_Y / dfSECTOR_SIZE;

	if (pPlayer->_cur_Pos._X >= dfSECTOR_X)
		pPlayer->_cur_Pos._X = dfSECTOR_X - 1;
	if (pPlayer->_cur_Pos._Y >= dfSECTOR_Y)
		pPlayer->_cur_Pos._Y = dfSECTOR_Y - 1;
	if (pPlayer->_cur_Pos._X < 0)
		pPlayer->_cur_Pos._X = 0;
	if (pPlayer->_cur_Pos._Y < 0)
		pPlayer->_cur_Pos._Y = 0;

	if (pPlayer->_cur_Pos._X != pPlayer->_old_Pos._X ||
		pPlayer->_cur_Pos._Y != pPlayer->_old_Pos._Y)
		sectorChanged = true;

	if (sectorChanged)
	{
		g_Sector[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X].emplace(pPlayer->_Id, pPlayer);//ADD SECTOR
		auto iter = g_Sector[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X].find(pPlayer->_Id);//REMOVE SECTOR
		if (iter != g_Sector[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X].end())
		{
			g_Sector[pPlayer->_old_Pos._Y][pPlayer->_old_Pos._X].erase(iter);
		}
		else
		{
			Disconnect(pPlayer->pSession);
			return;
		}

		// 여기서 넣어야 되는 섹터 주변에 다 넣고
		// 빼야되는 섹터 주변에 다 빼야됨.

		st_SECTOR_AROUND removeSectorAround;
		st_SECTOR_AROUND addSectorAround;
		removeSectorAround.iCount = addSectorAround.iCount = 0;
		GetUpdateSectorAround(pPlayer, &removeSectorAround, &addSectorAround);

		//old around sector.count + cur around sector.count번 반복
		//동일한거면 하지 않고 지나감
		//다른거면 remove->around에다가 넣음 old sector around[y][x]를 집어넣음
		//old around sector.count + cur around sector.count번 반복
		CSerealBuffer* pRemovePacket = _PacketPool.Alloc();// = univ_dev::g_PacketObjectPool.Alloc();
		CSerealBuffer* pCreatePacket = _PacketPool.Alloc();// = univ_dev::g_PacketObjectPool.Alloc();
		CSerealBuffer* pRemoveOtherPlayerPacket = _PacketPool.Alloc();// = univ_dev::g_PacketObjectPool.Alloc();
		CSerealBuffer* pCreateOtherPlayerPacket = _PacketPool.Alloc();// = univ_dev::g_PacketObjectPool.Alloc();
		CSerealBuffer* pMovePacket = _PacketPool.Alloc();// = univ_dev::g_PacketObjectPool.Alloc();
		pRemovePacket->Clear();
		pCreatePacket->Clear();
		pRemoveOtherPlayerPacket->Clear();
		pCreateOtherPlayerPacket->Clear();
		pMovePacket->Clear();

		mp_DELETE_CHARACTER(pRemovePacket, pPlayer->_Id);
		mp_SC_CREATE_OTHER_CHARACTER(pCreatePacket, pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y, pPlayer->_HP);

		for (int i = 0; i < removeSectorAround.iCount; i++)
		{
			int xIdx = removeSectorAround.Arroud[i]._X;
			int yIdx = removeSectorAround.Arroud[i]._Y;
			auto iter = g_Sector[yIdx][xIdx].begin();
			for (; iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				st_PLAYER* tempPlayer = iter->second;
				if (tempPlayer->_Id == pPlayer->_Id)continue;
				mp_DELETE_CHARACTER(pRemoveOtherPlayerPacket, tempPlayer->_Id);
				sendPacket_UniCast(pPlayer->pSession, pRemoveOtherPlayerPacket);
			}
			sendPacket_SectorOne(pPlayer->pSession, pRemovePacket, xIdx, yIdx);
			pRemoveOtherPlayerPacket->Clear();
		}

		for (int j = 0; j < addSectorAround.iCount; j++)
		{
			int xIdx = addSectorAround.Arroud[j]._X;
			int yIdx = addSectorAround.Arroud[j]._Y;
			auto iter = g_Sector[yIdx][xIdx].begin();
			for (; iter != g_Sector[yIdx][xIdx].end(); ++iter)
			{
				auto currentPlayer = iter->second;
				if (currentPlayer->_Id == pPlayer->_Id) continue;
				mp_SC_CREATE_OTHER_CHARACTER(pCreateOtherPlayerPacket, currentPlayer->_Id, currentPlayer->_Direction, currentPlayer->_X, currentPlayer->_Y, currentPlayer->_HP);

				sendPacket_UniCast(pPlayer->pSession, pCreateOtherPlayerPacket);
				sendPacket_UniCast(currentPlayer->pSession, pCreatePacket);
				pCreateOtherPlayerPacket->Clear();

				if (currentPlayer->_Direction_check == ON_MOVE)
				{
					switch (currentPlayer->_Direction)
					{
						case dfPACKET_MOVE_DIR_LL:
						case dfPACKET_MOVE_DIR_LU:
						case dfPACKET_MOVE_DIR_UU:
						case dfPACKET_MOVE_DIR_RU:
						case dfPACKET_MOVE_DIR_RR:
						case dfPACKET_MOVE_DIR_RD:
						case dfPACKET_MOVE_DIR_DD:
						case dfPACKET_MOVE_DIR_LD:
						{
							mp_MOVE_START(pMovePacket, currentPlayer->_Id, currentPlayer->_Direction, currentPlayer->_X, currentPlayer->_Y);
							sendPacket_UniCast(pPlayer->pSession, pMovePacket);
							pMovePacket->Clear();
							break;
						}
					}
				}
				if (pPlayer->_Direction_check == ON_MOVE)
				{
					switch (pPlayer->_Direction)
					{
						case dfPACKET_MOVE_DIR_LL:
						case dfPACKET_MOVE_DIR_LU:
						case dfPACKET_MOVE_DIR_UU:
						case dfPACKET_MOVE_DIR_RU:
						case dfPACKET_MOVE_DIR_RR:
						case dfPACKET_MOVE_DIR_RD:
						case dfPACKET_MOVE_DIR_DD:
						case dfPACKET_MOVE_DIR_LD:
						{
							mp_MOVE_START(pMovePacket, pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y);
							sendPacket_UniCast(currentPlayer->pSession, pMovePacket);
							pMovePacket->Clear();
							break;
						}
					}
				}
			}
		}
		pMovePacket->Clear();
		pRemovePacket->Clear();
		pCreatePacket->Clear();
		pRemoveOtherPlayerPacket->Clear();
		pCreateOtherPlayerPacket->Clear();
		_PacketPool.Free(pMovePacket);
		_PacketPool.Free(pRemovePacket);
		_PacketPool.Free(pCreatePacket);
		_PacketPool.Free(pRemoveOtherPlayerPacket);
		_PacketPool.Free(pCreateOtherPlayerPacket);
	}
}