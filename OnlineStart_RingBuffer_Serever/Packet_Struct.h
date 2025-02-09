//패킷프로토콜 - 패킷은 16바이트 고정
//
//ID할당(0)		Type(4Byte) | ID(4Byte) | 안씀(4Byte) | 안씀(4Byte)
//별생성(1)		Type(4Byte) | ID(4Byte) | X(4Byte) | Y(4Byte)
//별삭제(2)		Type(4Byte) | ID(4Byte) | 안씀(4Byte) | 안씀(4Byte)
//이동(3)			Type(4Byte) | ID(4Byte) | X(4Byte) | Y(4Byte)
//
//0 ~2번 패킷은 서버->클라패킷 이며, 3번 패킷은  클라 < ->서버  쌍방 패킷임.
//
//
//	* 메시지는 메시지 마다 구조체를 만들어서 사용합니다.

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
	bool live;//세션 생존플래그 0이면 사망 아니면 생존
};

//ID 생성패킷
struct STAR_ID{
	int _Type;
	int _Id;
	int temp[2];//16바이트 보장을 위해 허수 int 2개
};

//별 생성 패킷 4가지를 모두 채워서 유저에게 전송한다.
struct STAR_CREATE {
	int _Type;
	int _Id;
	int _X;
	int _Y;
};
//별 삭제 패킷 타입과 아이디만 전송해서 플레이어 클라이언트에서 별을 제거한다.
struct STAR_DELETE {
	int _Type;
	int _Id;
	int temp[2];
};
//별 이동패킷 타입아이디 _X _Y를 전송해 클라이언트에게 알린다.
struct STAR_MOVE {
	int _Type;
	int _Id;
	int _X;
	int _Y;
};
