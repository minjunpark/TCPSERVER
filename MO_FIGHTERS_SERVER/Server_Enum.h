#pragma once
enum SERVER_DEFINE
{
	SERVERPORT = 5000,
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 480,
	COLORBIT = 32,
	// ��Ŷ�ڵ� 0x89 ����.
	PACKET_CODE = 0x89,
	_df_FPS = 20,
	_df_Buffer_size = 10000
};

enum SERVER_GAME
{
	//�� ��ǥ�� ���� ���ϰ� �ؾ��ϸ�,
	//�ش� ��ǥ�� ��� ��� �������� ���߾�� ��.
	//(�۰ų� ������ ����)
	//��) ���� ������� �̵� �� ���� ������ �ɸ���
	//������� ���� Ÿ�� �̵��ϴ°� �ƴϸ�, ���ڸ��� ����� ��.

	dfRANGE_MOVE_TOP = 50,
	dfRANGE_MOVE_LEFT = 10,
	dfRANGE_MOVE_RIGHT = 630,
	dfRANGE_MOVE_BOTTOM = 470,
	//# �����Ӵ� �̵� ����---------------------------------- -
	//X �� - 3
	//Y �� - 2
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
	dfATTACK1_DAMAGE = 3,
	dfATTACK2_DAMAGE = 5,
	dfATTACK3_DAMAGE = 8
};

struct Session
{
	SOCKET _Sock;
	TRingBuffer* _RecvBuf;
	TRingBuffer* _SendBuf;
	int _Id;//ID��
	bool _Direction_check;//true�� �̵��� false�� ����
	bool _Live;//���� ��������
	char _Direction;//���� ���� 0 ���� 1������
	short _X;//����X��ġ
	short _Y;//����Y��ġ
	char _HP;//���� HP
};