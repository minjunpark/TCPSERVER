#pragma once
#include <list>
enum SERVER_DEFINE
{
	SERVERPORT = 5000,
	//SCREEN_WIDTH = 640,
	//SCREEN_HEIGHT = 480,
	COLORBIT = 32,
	// 패킷코드 0x89 고정.
	//PACKET_CODE = 0x89,
	_df_FPS = 20,
	_df_Buffer_size = 10000
};

#define PACKET_CODE (char)0x89

enum SERVER_GAME
{
	//위 좌표에 가지 못하게 해야하며,
	//해당 좌표에 닿는 경우 움직임을 멈추어야 함.
	//(작거나 같으면 멈춤)
	//예) 왼쪽 상단으로 이동 중 왼쪽 범위에 걸리면
	//상단으로 벽을 타고 이동하는게 아니며, 그자리에 멈춰야 함.

//#define dfRANGE_MOVE_TOP	0
//#define dfRANGE_MOVE_LEFT	0
//#define dfRANGE_MOVE_RIGHT	6400
//#define dfRANGE_MOVE_BOTTOM	6400

	//dfRANGE_MOVE_TOP = 50,
	//dfRANGE_MOVE_LEFT = 10,
	//dfRANGE_MOVE_RIGHT = 630,
	//dfRANGE_MOVE_BOTTOM = 470,

	dfUP = 0,
	dfLEFTUP = 1,
	dfLEFT = 2,
	dfLEFTDOWN = 3,
	dfDOWN = 4,
	dfRIGHTDOWN = 5,
	dfRIGHT = 6,
	dfRIGHTUP = 7,
	//1칸
	//dfSECTOR_SIZE = 64,
	//dfSECTOR_X = 100,
	//dfSECTOR_Y = 100,
	//2칸
	//dfSECTOR_SIZE = 128,
	//dfSECTOR_X = 50,
	//dfSECTOR_Y = 50,
	//5칸
	dfSECTOR_SIZE = 320,
	dfSECTOR_X = 20,
	dfSECTOR_Y = 20,

	dfRANGE_MOVE_TOP = 0,
	dfRANGE_MOVE_LEFT = 0,
	dfRANGE_MOVE_RIGHT = 6400,
	dfRANGE_MOVE_BOTTOM = 6400,
	//# 프레임당 이동 단위---------------------------------- -
	//X 축 - 3
	//Y 축 - 2
	defualt_X_SET = 320,
	defualt_Y_SET = 240,
	df_TEST_X = 150,
	df_TEST_Y = 150,
	defualt_HP = 100,
	default_Direction = 0,
	//defualt_MOVE_X = 3,
	//defualt_MOVE_Y = 2,
	dfFrame_time = 40,
	defualt_MOVE_X = 6,
	defualt_MOVE_Y = 4,
	ON_MOVE = 1,
	ON_STOP = 0,
	df_HP=100,
	//-----------------------------------------------------------------
	// 이동 오류체크 범위
	//-----------------------------------------------------------------
	dfERROR_RANGE = 50,

	//---------------------------------------------------------------
	// 공격범위.
	//---------------------------------------------------------------
	dfATTACK1_RANGE_X = 80,
	dfATTACK2_RANGE_X = 90,
	dfATTACK3_RANGE_X = 100,
	dfATTACK1_RANGE_Y = 10,
	dfATTACK2_RANGE_Y = 10,
	dfATTACK3_RANGE_Y = 20,
	//dfATTACK1_DAMAGE = 3,
	//dfATTACK2_DAMAGE = 5,
	//dfATTACK3_DAMAGE = 8, 
	dfATTACK1_DAMAGE = 1,
	dfATTACK2_DAMAGE = 2,
	dfATTACK3_DAMAGE = 3,
	dfNETWORK_PACKET_RECV_TIMEOUT = 30000

	
};

//섹터 하나의 좌표 정보
struct st_SECTOR_POS
{
	int _X;
	int _Y;
};

//특정 위치 주변의 9개 섹터 정보
struct st_SECTOR_AROUND
{
	int iCount = 0;
	st_SECTOR_POS Arroud[9];
};

struct st_SESSION
{
	SOCKET _Sock;
	SOCKADDR_IN _Con_Addr;
	CRingBuffer _RecvBuf;
	CRingBuffer _SendBuf;

	DWORD _LastRecvTime;//메세지 수신 체크를 위한 시간 타임(타임아웃용)

	int _Id;//ID값
//	bool _Direction_check;//true면 이동중 false면 정지
	bool _Live;//세션 생존여부
	//char _Direction;//보는 방향 0 왼쪽 1오른쪽
	//short _X;//현재X위치
	//short _Y;//현재Y위치
	//char _HP;//현재 HP
};

struct st_PLAYER
{
	st_SESSION* pSession;
	int _Id;//ID값

	BYTE _Direction_check;//true면 이동중 false면 정지
	BYTE _Direction;//보는 방향 0 왼쪽 1오른쪽
	bool _Live;//플레이어 생존여부

	short _X;//현재X위치
	short _Y;//현재Y위치

	st_SECTOR_POS _cur_Pos;
	st_SECTOR_POS _old_Pos;

	char _HP;//현재 HP
};