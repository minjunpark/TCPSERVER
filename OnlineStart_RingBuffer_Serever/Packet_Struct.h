//��Ŷ�������� - ��Ŷ�� 16����Ʈ ����
//
//ID�Ҵ�(0)		Type(4Byte) | ID(4Byte) | �Ⱦ�(4Byte) | �Ⱦ�(4Byte)
//������(1)		Type(4Byte) | ID(4Byte) | X(4Byte) | Y(4Byte)
//������(2)		Type(4Byte) | ID(4Byte) | �Ⱦ�(4Byte) | �Ⱦ�(4Byte)
//�̵�(3)			Type(4Byte) | ID(4Byte) | X(4Byte) | Y(4Byte)
//
//0 ~2�� ��Ŷ�� ����->Ŭ����Ŷ �̸�, 3�� ��Ŷ��  Ŭ�� < ->����  �ֹ� ��Ŷ��.
//
//
//	* �޽����� �޽��� ���� ����ü�� ���� ����մϴ�.

enum _e_Player
{
	ID_SET = 0,
	CREATE_SET=1,
	DELETE_SET=2,
	MOVE_SET=3,
	_X = 20,
	_Y = 20
};

struct Session
{
	SOCKET sock;
	TRingBuffer* recvBuf;
	TRingBuffer* sendBuf;
	int Id;
	char* IP;
	int Port;
	int X;
	int Y;
	bool live;//���� �����÷��� 0�̸� ��� �ƴϸ� ����
};

//ID ������Ŷ
struct STAR_ID{
	int _Type;
	int _Id;
	int temp[2];//16����Ʈ ������ ���� ��� int 2��
};

//�� ���� ��Ŷ 4������ ��� ä���� �������� �����Ѵ�.
struct STAR_CREATE {
	int _Type;
	int _Id;
	int _X;
	int _Y;
};
//�� ���� ��Ŷ Ÿ�԰� ���̵� �����ؼ� �÷��̾� Ŭ���̾�Ʈ���� ���� �����Ѵ�.
struct STAR_DELETE {
	int _Type;
	int _Id;
	int temp[2];
};
//�� �̵���Ŷ Ÿ�Ծ��̵� _X _Y�� ������ Ŭ���̾�Ʈ���� �˸���.
struct STAR_MOVE {
	int _Type;
	int _Id;
	int _X;
	int _Y;
};
