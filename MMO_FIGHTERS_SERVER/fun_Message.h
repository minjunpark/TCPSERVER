#pragma once
#include <math.h>
#include <stdio.h>
bool netPacketProc_SC_CREATE_MY_CHARACTER(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_SC_CREATE_MY_CHARACTER(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y, char p_Hp);

bool netPacketProc_SC_CREATE_OTHER_CHARACTER(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_SC_CREATE_OTHER_CHARACTER(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y, char p_Hp);

bool netPacketProc_DELETE_CHARACTER(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_DELETE_CHARACTER(CSerealBuffer* pPacket, int p_id);

bool netPacketProc_MOVE_START(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_MOVE_START(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y);

bool netPacketProc_MOVE_STOP(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_MOVE_STOP(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y);

bool netPacketProc_ATTACK1(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_ATTACK1(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y);

bool netPacketProc_ATTACK2(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_ATTACK2(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y);

bool netPacketProc_ATTACK3(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_ATTACK3(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y);

//bool netPacketProc_DAMAGE(Session* pSession, CSerealBuffer* pPacket);
void mp_DAMAGE(CSerealBuffer* pPacket, int _a_id, int _d_id, char _hp);

//bool netPacketProc_SYNC(st_SESSION* pSession, CSerealBuffer* pPacket);
void mp_Sync(CSerealBuffer* pPacket, int p_id, short p_X, short p_Y);

//dfPACKET_SC_CREATE_MY_CHARACTER
bool netPacketProc_SC_CREATE_MY_CHARACTER(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);
	if (pPlayer == nullptr)//플레이어를 찾지 못했다면 널포인터
	{
		return false;
	}

	//XY위치 랜덤으로 생성
	//6400 6400이하에서 값가져오기
	
	//섹터위치 지정//추후조정

	//지정된 XY와 섹터를 클라이언트에게 전달한다.

	CSerealBuffer MyCreate;
	mp_SC_CREATE_MY_CHARACTER(&MyCreate, pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y, pPlayer->_HP);
	sendUniCast(pSession, &MyCreate);

#ifdef df_LOG
	printf("Create Character # SessionID %d  X:%d Y:%d\n", pSession->_Id, defualt_X_SET, defualt_Y_SET);
#endif

	return true;
};

void mp_SC_CREATE_MY_CHARACTER(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y, char p_Hp)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = 10;
	stPacketHeader.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id << p_Direction << p_X << p_Y << p_Hp;
};


//dfPACKET_SC_CREATE_OTHER_CHARACTER
bool netPacketProc_SC_CREATE_OTHER_CHARACTER(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	int _Id;
	char _Direction;
	short _X;
	short _Y;
	char _Hp;
	*pPacket >> _Id;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;
	*pPacket >> _Hp;
	
	CSerealBuffer OtherCreate;
	mp_SC_CREATE_OTHER_CHARACTER(&OtherCreate, _Id, _Direction, _X, _Y, _Hp);
	sendBroadCast(pSession, &OtherCreate);
	return true;
};

void mp_SC_CREATE_OTHER_CHARACTER(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y, char p_Hp)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = 10;
	stPacketHeader.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
	*pPacket << p_Direction;
	*pPacket << p_X;
	*pPacket << p_Y;
	*pPacket << p_Hp;
};

//dfPACKET_SC_DELETE_CHARACTER
bool netPacketProc_DELETE_CHARACTER(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	CSerealBuffer Deletebuf;
	mp_DELETE_CHARACTER(&Deletebuf, pSession->_Id);
	sendBroadCast(pSession, &Deletebuf);
#ifdef df_LOG
	printf("Delete Session # SessionID %d \n", pSession->_Id);
#endif
	return true;
};

void mp_DELETE_CHARACTER(CSerealBuffer* pPacket, int p_id)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_DELETE_CHARACTER);
	stPacketHeader.byType = dfPACKET_SC_DELETE_CHARACTER;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
};

//dfPACKET_SC_MOVE_START
bool netPacketProc_MOVE_START(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	char _Direction;
	short _X;
	short _Y;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;

	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);
	if (pPlayer == nullptr)
	{
		return false;
	}

	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 강제로 좌표 동기화
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
		//CSerealBuffer sPacket;
		//mp_Sync(&sPacket, pPlayer->_Id, pPlayer->_X, pPlayer->_Y);
		//sendBroadCast(pSession, &sPacket);
		//_X = pPlayer->_X;
		//_Y = pPlayer->_Y;
	}

	//동작변경
	pPlayer->_Direction = _Direction;//바라보는 방향지정
	pPlayer->_Direction_check = ON_MOVE;//이동중 이라고 변경

	//현재 접속중인 사용자에게 모든 세션을 뿌리기
	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	//직렬화 버퍼를 생성한후 보낸다.
	CSerealBuffer MoveStart;
	mp_MOVE_START(&MoveStart, pPlayer->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &MoveStart);

#ifdef df_LOG
	printf("PACKET_MOVESTART # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;
};

void mp_Sync(CSerealBuffer* pPacket,int p_id, short p_X, short p_Y)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_SYNC);
	stPacketHeader.byType = dfPACKET_SC_SYNC;
	pPacket->Clear();
	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id << p_X << p_Y;
}

void mp_MOVE_START(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_MOVE_START);
	stPacketHeader.byType = dfPACKET_SC_MOVE_START;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
	*pPacket << p_Direction;
	*pPacket << p_X;
	*pPacket << p_Y;
}

//dfPACKET_MOVE_STOP
bool netPacketProc_MOVE_STOP(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	char _Direction;
	short _X;
	short _Y;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;

	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);
	if (pPlayer == nullptr)
	{
		return false;
	}

	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 강제종요
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//동작변경
	pPlayer->_Direction_check = ON_STOP;//이동중 이라고 변경
	pPlayer->_Direction = _Direction;//바라보는 방향지정
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	CSerealBuffer MoveStop;
	mp_MOVE_STOP(&MoveStop, pPlayer->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &MoveStop);
#ifdef df_LOG
	printf("PACKET_MOVESTOP # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;
}

void mp_MOVE_STOP(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_MOVE_STOP);
	stPacketHeader.byType = dfPACKET_SC_MOVE_STOP;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
	*pPacket << p_Direction;
	*pPacket << p_X;
	*pPacket << p_Y;
}

//dfPACKET_ATTACK1
bool netPacketProc_ATTACK1(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	char _Direction;
	short _X;
	short _Y;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;

	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);
	if (pPlayer == nullptr)
	{
		return false;
	}

	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 강제종요
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_Direction_check = ON_STOP;//정지중이라고 변경
	pPlayer->_Direction = _Direction;//공격하는 방향
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	CSerealBuffer Attack1;
	mp_ATTACK1(&Attack1, pSession->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &Attack1);

	//범위내에 적이 있는지 확인하고 적이 있다면 모두 타격한다.
	st_SESSION* Atk_tmp_Session;
	for (auto session_it = Session_Map.begin(); session_it != Session_Map.end(); ++session_it)
	{
		Atk_tmp_Session = session_it->second;
		st_PLAYER* Atk_tmp_Player;
		Atk_tmp_Player = Find_Player(pSession->_Id);
		if (Atk_tmp_Player == nullptr)
		{
			return false;
		}

		if (Atk_tmp_Session->_Live == false)
		{
			continue;//죽어있는 패킷 무시
		}

		if (pPlayer->_Id == Atk_tmp_Player->_Id)//나 자신이라면 제외한다.
		{
			continue;
		}

		switch (pPlayer->_Direction)//왼쪽 오른쪽 구분
		{
		case dfPACKET_MOVE_DIR_LL:
		{
			if ((pPlayer->_X - dfATTACK1_RANGE_X <= Atk_tmp_Player->_X
				&& pPlayer->_X >= Atk_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk_tmp_Player->_Y) <= dfATTACK1_RANGE_Y))
			{
				Atk_tmp_Player->_HP -= dfATTACK1_DAMAGE;//맞은 대상의 HP를 깎는다.

				if (Atk_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk_tmp_Session);//클라이언트가 죽었으므로 종료
				}
				else
				{
					CSerealBuffer Damage;
					mp_DAMAGE(&Damage, pPlayer->_Id, Atk_tmp_Player->_Id, Atk_tmp_Player->_HP);
					sendBroadCast(nullptr, &Damage);
				}
			}
		}
		break;
		case dfPACKET_MOVE_DIR_RR:
		{
			if ((pPlayer->_X + dfATTACK1_RANGE_X >= Atk_tmp_Player->_X
				&& pPlayer->_X <= Atk_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk_tmp_Player->_Y) <= dfATTACK1_RANGE_Y))
			{
				Atk_tmp_Player->_HP -= dfATTACK1_DAMAGE;//맞은 대상의 HP를 깎는다.

				if (Atk_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk_tmp_Session);//클라이언트가 죽었으므로 종료
				}
				else
				{
					CSerealBuffer Damage;
					mp_DAMAGE(&Damage, pPlayer->_Id, Atk_tmp_Player->_Id, Atk_tmp_Player->_HP);
					sendBroadCast(nullptr, &Damage);
				}
			}
		}
		break;
		}
	}
#ifdef df_LOG
	printf("PACKET_ATTACK1 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;

}

void mp_ATTACK1(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_ATTACK1);
	stPacketHeader.byType = dfPACKET_SC_ATTACK1;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
	*pPacket << p_Direction;
	*pPacket << p_X;
	*pPacket << p_Y;
}



//dfPACKET_ATTACK2
bool netPacketProc_ATTACK2(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	char _Direction;
	short _X;
	short _Y;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;

	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);
	if (pPlayer == nullptr)
	{
		return false;
	}

	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 강제종요
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_Direction_check = ON_STOP;//정지중이라고 변경
	pPlayer->_Direction = _Direction;//공격하는 방향
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.
	
	CSerealBuffer Attack2;
	mp_ATTACK2(&Attack2, pPlayer->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &Attack2);

	//범위내에 적이 있는지 확인하고 적이 있다면 모두 타격한다.
	st_SESSION* Atk2_tmp_Session;
	for (auto session_it = Session_Map.begin(); session_it != Session_Map.end(); ++session_it)
	{
		Atk2_tmp_Session = session_it->second;

		st_PLAYER* Atk2_tmp_Player;
		Atk2_tmp_Player = Find_Player(pSession->_Id);
		if (Atk2_tmp_Player == nullptr)
		{
			return false;
		}

		if (Atk2_tmp_Player->_Live == false)
		{
			continue;//죽어있는 패킷 무시
		}

		if (pPlayer->_Id == Atk2_tmp_Player->_Id)
		{
			continue;
		}

		switch (pPlayer->_Direction)
		{
		case dfPACKET_MOVE_DIR_LL:
		{
			if ((pPlayer->_X - dfATTACK2_RANGE_X <= Atk2_tmp_Player->_X
				&& pPlayer->_X >= Atk2_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk2_tmp_Player->_Y) <= dfATTACK2_RANGE_Y))
			{
				Atk2_tmp_Player->_HP -= dfATTACK2_DAMAGE;//맞은 대상의 HP를 깎는다.

				if (Atk2_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk2_tmp_Session);//클라이언트가 죽었으므로 종료
				}
				else
				{
					CSerealBuffer Damage;
					mp_DAMAGE(&Damage, pPlayer->_Id, Atk2_tmp_Player->_Id, Atk2_tmp_Player->_HP);
					sendBroadCast(nullptr, &Damage);
				}
			}
		}
		break;
		case dfPACKET_MOVE_DIR_RR:
		{
			if ((pPlayer->_X + dfATTACK2_RANGE_X >= Atk2_tmp_Player->_X
				&& pPlayer->_X <= Atk2_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk2_tmp_Player->_Y) <= dfATTACK2_RANGE_Y))
			{
				Atk2_tmp_Player->_HP -= dfATTACK2_DAMAGE;//맞은 대상의 HP를 깎는다.

				if (Atk2_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk2_tmp_Session);//클라이언트가 죽었으므로 종료
				}
				else
				{
					CSerealBuffer Damage;
					mp_DAMAGE(&Damage, pPlayer->_Id, Atk2_tmp_Player->_Id, Atk2_tmp_Player->_HP);
					sendBroadCast(nullptr, &Damage);
				}
			}
		}
		break;
		}
	}

#ifdef df_LOG
	printf("PACKET_ATTACK2 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;
}

void mp_ATTACK2(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_ATTACK2);
	stPacketHeader.byType = dfPACKET_SC_ATTACK2;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
	*pPacket << p_Direction;
	*pPacket << p_X;
	*pPacket << p_Y;
}

//dfPACKET_ATTACK3
bool netPacketProc_ATTACK3(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	char _Direction;
	short _X;
	short _Y;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;

	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);
	if (pPlayer == nullptr)
	{
		return false;
	}

	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 강제종요
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_Direction_check = ON_STOP;//정지중이라고 변경
	pPlayer->_Direction = _Direction;//공격하는 방향
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	CSerealBuffer Attack3;
	mp_ATTACK3(&Attack3, pPlayer->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &Attack3);

	//범위내에 적이 있는지 확인하고 적이 있다면 모두 타격한다.
	st_SESSION* Atk3_tmp_Session;
	for (auto session_it = Session_Map.begin(); session_it != Session_Map.end(); ++session_it)
	{
		Atk3_tmp_Session = session_it->second;

		st_PLAYER* Atk3_tmp_Player;
		Atk3_tmp_Player = Find_Player(pSession->_Id);
		if (Atk3_tmp_Player == nullptr)
		{
			return false;
		}

		if (Atk3_tmp_Player->_Live == false)
		{
			continue;//죽어있는 패킷 무시
		}

		if (pPlayer->_Id == Atk3_tmp_Player->_Id)
		{
			continue;
		}

		switch (pPlayer->_Direction)
		{
		case dfPACKET_MOVE_DIR_LL:
		{
			if ((pPlayer->_X - dfATTACK3_RANGE_X <= Atk3_tmp_Player->_X
				&& pPlayer->_X >= Atk3_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk3_tmp_Player->_Y) <= dfATTACK3_RANGE_Y))
			{
				Atk3_tmp_Player->_HP -= dfATTACK3_DAMAGE;//맞은 대상의 HP를 깎는다.

				if (Atk3_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk3_tmp_Session);//클라이언트가 죽었으므로 종료
				}
				else
				{
					CSerealBuffer Damage;
					mp_DAMAGE(&Damage, pPlayer->_Id, Atk3_tmp_Player->_Id, Atk3_tmp_Player->_HP);
					sendBroadCast(nullptr, &Damage);
				}
			}
		}
		break;
		case dfPACKET_MOVE_DIR_RR:
		{
			if ((pPlayer->_X + dfATTACK3_RANGE_X >= Atk3_tmp_Player->_X
				&& pPlayer->_X <= Atk3_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk3_tmp_Player->_Y) <= dfATTACK3_RANGE_Y))
			{
				Atk3_tmp_Player->_HP -= dfATTACK3_DAMAGE;//맞은 대상의 HP를 깎는다.

				if (Atk3_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk3_tmp_Session);//클라이언트가 죽었으므로 종료
				}
				else
				{
					CSerealBuffer Damage;
					mp_DAMAGE(&Damage, pPlayer->_Id, Atk3_tmp_Player->_Id, Atk3_tmp_Player->_HP);
					sendBroadCast(nullptr, &Damage);
				}
			}
		}
		break;
		}
	}
#ifdef df_LOG
	printf("PACKET_ATTACK3 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif

	return true;
}

void mp_ATTACK3(CSerealBuffer* pPacket, int p_id, char p_Direction, short p_X, short p_Y)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_ATTACK3);
	stPacketHeader.byType = dfPACKET_SC_ATTACK3;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << p_id;
	*pPacket << p_Direction;
	*pPacket << p_X;
	*pPacket << p_Y;
}

//dfPACKET_SC_DAMAGE
//bool netPacketProc_DAMAGE(Session* pSession, char* pPacket)
//{
//	st_dfPACKET_SC_DAMAGE* pDamage = (st_dfPACKET_SC_DAMAGE*)pPacket;
//
//	st_dfPACKET_header st_SC_DAMAGE_HEADER;
//	st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
//	mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, pDamage->_Damage_Id, pDamage->Hp);
//	sendBroadCast(pSession, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
//
//	return true;
//}
//

void mp_DAMAGE(CSerealBuffer* pPacket, int _a_id, int _d_id, char _hp)
{
	st_dfPACKET_header	stPacketHeader;
	stPacketHeader.byCode = PACKET_CODE;
	stPacketHeader.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
	stPacketHeader.byType = dfPACKET_SC_DAMAGE;

	pPacket->PutData((char*)&stPacketHeader, sizeof(st_dfPACKET_header));
	*pPacket << _a_id;
	*pPacket << _d_id;
	*pPacket << _hp;
}

//dfPACKET_CS_SYNC
bool netPacketProc_SYNC(st_SESSION* pSession, CSerealBuffer* pPacket)
{
	return true;
};
//dfPACKET_SC_SYNC