#pragma once
enum SERVER_DEFINE
{
	SERVERPORT = 5000,
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 480,
	COLORBIT = 32,
	// 패킷코드 0x89 고정.
	PACKET_CODE = 0x89,
	_df_FPS = 20,
	_df_Buffer_size = 10000
};

enum SERVER_GAME
{
	//위 좌표에 가지 못하게 해야하며,
	//해당 좌표에 닿는 경우 움직임을 멈추어야 함.
	//(작거나 같으면 멈춤)
	//예) 왼쪽 상단으로 이동 중 왼쪽 범위에 걸리면
	//상단으로 벽을 타고 이동하는게 아니며, 그자리에 멈춰야 함.

	dfRANGE_MOVE_TOP = 50,
	dfRANGE_MOVE_LEFT = 10,
	dfRANGE_MOVE_RIGHT = 630,
	dfRANGE_MOVE_BOTTOM = 470,
	//# 프레임당 이동 단위---------------------------------- -
	//X 축 - 3
	//Y 축 - 2
	defualt_X_SET = 320,
	defualt_Y_SET = 240,
	defualt_HP = 100,
	default_Direction = 0,
	defualt_MOVE_X = 3,
	defualt_MOVE_Y = 2,
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
	dfATTACK1_DAMAGE = 3,
	dfATTACK2_DAMAGE = 5,
	dfATTACK3_DAMAGE = 8
};

struct Session
{
	SOCKET _Sock;
	TRingBuffer* _RecvBuf;
	TRingBuffer* _SendBuf;
	int _Id;//ID값
	bool _Direction_check;//true면 이동중 false면 정지
	bool _Live;//세션 생존여부
	char _Direction;//보는 방향 0 왼쪽 1오른쪽
	short _X;//현재X위치
	short _Y;//현재Y위치
	char _HP;//현재 HP
};