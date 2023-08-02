#pragma once
#include <list>
enum SERVER_DEFINE
{
	SERVERPORT = 5000,
	//SCREEN_WIDTH = 640,
	//SCREEN_HEIGHT = 480,
	COLORBIT = 32,
	// ��Ŷ�ڵ� 0x89 ����.
	//PACKET_CODE = 0x89,
	_df_FPS = 20,
	_df_Buffer_size = 10000
};

#define PACKET_CODE (char)0x89

enum SERVER_GAME
{
	//�� ��ǥ�� ���� ���ϰ� �ؾ��ϸ�,
	//�ش� ��ǥ�� ��� ��� �������� ���߾�� ��.
	//(�۰ų� ������ ����)
	//��) ���� ������� �̵� �� ���� ������ �ɸ���
	//������� ���� Ÿ�� �̵��ϴ°� �ƴϸ�, ���ڸ��� ����� ��.

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
	//1ĭ
	//dfSECTOR_SIZE = 64,
	//dfSECTOR_X = 100,
	//dfSECTOR_Y = 100,
	//2ĭ
	//dfSECTOR_SIZE = 128,
	//dfSECTOR_X = 50,
	//dfSECTOR_Y = 50,
	//5ĭ
	dfSECTOR_SIZE = 320,
	dfSECTOR_X = 20,
	dfSECTOR_Y = 20,

	dfRANGE_MOVE_TOP = 0,
	dfRANGE_MOVE_LEFT = 0,
	dfRANGE_MOVE_RIGHT = 6400,
	dfRANGE_MOVE_BOTTOM = 6400,
	//# �����Ӵ� �̵� ����---------------------------------- -
	//X �� - 3
	//Y �� - 2
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
	// �̵� ����üũ ����
	//-----------------------------------------------------------------
	dfERROR_RANGE = 50,

	//---------------------------------------------------------------
	// ���ݹ���.
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

//���� �ϳ��� ��ǥ ����
struct st_SECTOR_POS
{
	int _X;
	int _Y;
};

//Ư�� ��ġ �ֺ��� 9�� ���� ����
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

	DWORD _LastRecvTime;//�޼��� ���� üũ�� ���� �ð� Ÿ��(Ÿ�Ӿƿ���)

	int _Id;//ID��
//	bool _Direction_check;//true�� �̵��� false�� ����
	bool _Live;//���� ��������
	//char _Direction;//���� ���� 0 ���� 1������
	//short _X;//����X��ġ
	//short _Y;//����Y��ġ
	//char _HP;//���� HP
};

struct st_PLAYER
{
	st_SESSION* pSession;
	int _Id;//ID��

	BYTE _Direction_check;//true�� �̵��� false�� ����
	BYTE _Direction;//���� ���� 0 ���� 1������
	bool _Live;//�÷��̾� ��������

	short _X;//����X��ġ
	short _Y;//����Y��ġ

	st_SECTOR_POS _cur_Pos;
	st_SECTOR_POS _old_Pos;

	char _HP;//���� HP
};