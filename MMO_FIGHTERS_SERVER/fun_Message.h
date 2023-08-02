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
	//pPlayer->_X = 3200;
	//pPlayer->_Y = 3200;
	//섹터위치 지정//추후조정
	//pPlayer->_Y = 6360;
	//pPlayer->_X = 6360;
	//Sector_UpdatePlayer(pPlayer);//현재 내 섹터위치를 업데이트한다.

	//섹터 유저리스트에 데이터를 넣는다.
	//g_Sector[pPlayer->_cur_Pos._Y][pPlayer->_cur_Pos._X].push_back(pPlayer);

	CSerealBuffer* MyCreate = _PacketPool.Alloc();
	MyCreate->Clear();
	mp_SC_CREATE_MY_CHARACTER(MyCreate, pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y, pPlayer->_HP);
	sendPacket_UniCast(pSession, MyCreate);//나한테 전달하고
	MyCreate->Clear();
	_PacketPool.Free(MyCreate);

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

	CSerealBuffer* OtherCreate = _PacketPool.Alloc();
	OtherCreate->Clear();
	mp_SC_CREATE_OTHER_CHARACTER(OtherCreate, _Id, _Direction, _X, _Y, _Hp);
	//sendBroadCast(pSession, &OtherCreate);
	//생성된 나를 섹터 주변에 전달
	//나는 제외하고 데이터를 보낸다.
	sendPacket_Around(pSession, OtherCreate, true);
	OtherCreate->Clear();
	_PacketPool.Free(OtherCreate);

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
	CSerealBuffer* Deletebuf = _PacketPool.Alloc();
	Deletebuf->Clear();
	mp_DELETE_CHARACTER(Deletebuf, pSession->_Id);
	//sendBroadCast(pSession, &Deletebuf);
	sendPacket_Around(pSession, Deletebuf, false);
	Deletebuf->Clear();
	_PacketPool.Free(Deletebuf);
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
	BYTE _Direction;
	short _X;
	short _Y;
	*pPacket >> _Direction;
	*pPacket >> _X;
	*pPacket >> _Y;

	st_PLAYER* pPlayer;
	pPlayer = Find_Player(pSession->_Id);//ID를 검색한다.
	if (pPlayer == nullptr)
	{
		return false;
	}

	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 현재 서버의 위치로 좌표동기화
	if (abs((short)pPlayer->_X - _X) > dfERROR_RANGE ||
		abs((short)pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		//Disconnect(pSession);
		//return false;
		//현재 서버의 값과
		//날아온 값이 너무나 차이나는 경우
		//서버에게 갑을 맞춰서 당긴다.
		sync_start++;
		CSerealBuffer* sPacket = _PacketPool.Alloc();
		sPacket->Clear();
		_X = pPlayer->_X;
		_Y = pPlayer->_Y;
		mp_Sync(sPacket, pPlayer->_Id, pPlayer->_X, pPlayer->_Y);

		//sendBroadCast(pSession, &sPacket);
		//sendPacket_Around(pSession, &sPacket, false);
		//싱크를 맞추고 특정유저에게만 패킷을 보낸다.
		sendPacket_UniCast(pSession, sPacket);
		sPacket->Clear();
		_PacketPool.Free(sPacket);
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

	//좌표가 조절된경우 섹터를 업데이트할것
	// 
	//우선여기서는 아직 안함

	//직렬화 버퍼를 생성한후 보낸다.
	CSerealBuffer* MoveStart = _PacketPool.Alloc();
	MoveStart->Clear();
	mp_MOVE_START(MoveStart, pPlayer->_Id, _Direction, _X, _Y);

	//나자신을 제외하고 이동중이라는것을 보낸다.
	sendPacket_Around(pSession, MoveStart, true);
	MoveStart->Clear();
	_PacketPool.Free(MoveStart);
	//sendBroadCast(pSession, &MoveStart);

#ifdef df_LOG
	printf("PACKET_MOVESTART # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y);
#endif
	return true;
};

void mp_Sync(CSerealBuffer* pPacket, int p_id, short p_X, short p_Y)
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
	BYTE _Direction;
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
	//std::abs(pPlayer->_X - _X)
	//들어온세션의 이동데이터의 좌표가 너무 어긋나있으면 강제로 싱크를 맞춰준다.
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		//Disconnect(pSession);
		//return false;
		sync_stop++;
		CSerealBuffer* sPacket = _PacketPool.Alloc();
		sPacket->Clear();
		mp_Sync(sPacket, pPlayer->_Id, pPlayer->_X, pPlayer->_Y);
		sendPacket_UniCast(pSession, sPacket);
		//sendPacket_Around(nullptr, &sPacket, false);
		//sendBroadCast(pSession, &sPacket);
		sPacket->Clear();
		_PacketPool.Free(sPacket);
		_X = pPlayer->_X;
		_Y = pPlayer->_Y;
	}

	//동작변경
	pPlayer->_Direction_check = ON_STOP;//이동중 이라고 변경
	pPlayer->_Direction = _Direction;//바라보는 방향지정
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	CSerealBuffer* MoveStop = _PacketPool.Alloc();
	MoveStop->Clear();
	mp_MOVE_STOP(MoveStop, pPlayer->_Id, _Direction, _X, _Y);
	sendPacket_Around(pSession, MoveStop, true);//나자신은 제외하고 보내준다.
	MoveStop->Clear();
	_PacketPool.Free(MoveStop);
	//sendBroadCast(pSession, &MoveStop);
#ifdef df_LOG
	printf("PACKET_MOVESTOP # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y);
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
		//Disconnect(pSession);
		//return false;
		sync_attack1++;
		CSerealBuffer* sPacket = _PacketPool.Alloc();
		sPacket->Clear();
		mp_Sync(sPacket, pPlayer->_Id, pPlayer->_X, pPlayer->_Y);
		//sendBroadCast(pSession, &sPacket);
		sendPacket_UniCast(pSession, sPacket);
		sPacket->Clear();
		_PacketPool.Free(sPacket);
		_X = pPlayer->_X;
		_Y = pPlayer->_Y;
	}

	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_Direction_check = ON_STOP;//정지중이라고 변경
	pPlayer->_Direction = _Direction;//공격하는 방향
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	//내가 공격중이라는 모션을 보낸다.
	CSerealBuffer* Attack1 = _PacketPool.Alloc();
	mp_ATTACK1(Attack1, pSession->_Id, _Direction, _X, _Y);
	sendPacket_Around(pSession, Attack1, false);
	Attack1->Clear();
	_PacketPool.Free(Attack1);
	//sendBroadCast(pSession, &Attack1);

	//범위내에 적이 있는지 확인하고 적이 있다면 모두 타격한다.
	st_SESSION* Atk_tmp_Session;
	st_SECTOR_AROUND st_SECTOR_ATTACK1;
	unordered_map <DWORD, st_PLAYER*>* _Attack_Sector;

	//현재 내위치와 내주변 8지역 총 9구역 값을 받아온다.
	st_SECTOR_ATTACK1 = GetSectorAround(pPlayer->_cur_Pos._X, pPlayer->_cur_Pos._Y, &st_SECTOR_ATTACK1);

	for (int iCnt = 0; iCnt < 9; iCnt++)
	{
		//내섹터범위안에 있는 모든 유저를 확인하면서
		//범위안에 타격대상이 있는지 확인한다.
		int Cur_X = st_SECTOR_ATTACK1.Arroud[iCnt]._X;
		int Cur_Y = st_SECTOR_ATTACK1.Arroud[iCnt]._Y;

		if (!Sector_Check(Cur_X, Cur_Y))//섹터 범위를 벗어난게 있다면 전송안함
			continue;

		//0~99사이의 섹터라면
		_Attack_Sector = &g_Sector[Cur_Y][Cur_X];
		for (auto session_it = _Attack_Sector->begin(); session_it != _Attack_Sector->end(); ++session_it)
		{
			//Atk_tmp_Session = (*session_it);
			st_PLAYER* Atk_tmp_Player = session_it->second;
			//Atk_tmp_Player = Find_Player(Atk_tmp_Session->_Id);
			if (Atk_tmp_Player == nullptr)
			{
				return false;
			}

			if (Atk_tmp_Player->_Live == false)
			{
				continue;//죽어있는 패킷 무시
			}

			if (pPlayer->_Id == Atk_tmp_Player->_Id)//나 자신이라면 제외한다.
			{
				continue;
			}

			switch (pPlayer->_Direction)//왼쪽 오른쪽 구분
			{
			case dfPACKET_MOVE_DIR_LL://내가 왼쪽공격중일때 왼쪽범위안에 포함되는 녀석이 있다면
			{
				if ((pPlayer->_X - dfATTACK1_RANGE_X <= Atk_tmp_Player->_X
					&& pPlayer->_X >= Atk_tmp_Player->_X)
					&& (abs(pPlayer->_Y - Atk_tmp_Player->_Y) <= dfATTACK1_RANGE_Y))
				{
					Atk_tmp_Player->_HP -= dfATTACK1_DAMAGE;//맞은 대상의 HP를 깎는다.

					if (Atk_tmp_Player->_HP <= 0)
					{
						Disconnect(Atk_tmp_Player->pSession);//클라이언트가 죽었으므로 종료
					}
					else
					{
						CSerealBuffer* Damage = _PacketPool.Alloc();
						Damage->Clear();
						mp_DAMAGE(Damage, pPlayer->_Id, Atk_tmp_Player->_Id, Atk_tmp_Player->_HP);
						//맞은 유저기준 주변으로 데미지 전송
						sendPacket_Around(Atk_tmp_Player->pSession, Damage, false);
						Damage->Clear();
						_PacketPool.Free(Damage);
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
						Disconnect(Atk_tmp_Player->pSession);//클라이언트가 죽었으므로 종료
					}
					else
					{
						CSerealBuffer* Damage = _PacketPool.Alloc();
						Damage->Clear();
						mp_DAMAGE(Damage, pPlayer->_Id, Atk_tmp_Player->_Id, Atk_tmp_Player->_HP);
						//sendBroadCast(nullptr, &Damage);
						sendPacket_Around(Atk_tmp_Player->pSession, Damage, false);
						Damage->Clear();
						_PacketPool.Free(Damage);
					}
				}
			}
			break;
			}
		}
	}
#ifdef df_LOG
	printf("PACKET_ATTACK1 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y);
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
		//Disconnect(pSession);
		//return false;
		sync_attack2++;
		CSerealBuffer* sPacket = _PacketPool.Alloc();
		sPacket->Clear();
		mp_Sync(sPacket, pPlayer->_Id, pPlayer->_X, pPlayer->_Y);
		//sendBroadCast(pSession, &sPacket);
		sendPacket_UniCast(pSession, sPacket);
		sPacket->Clear();
		_PacketPool.Free(sPacket);
		_X = pPlayer->_X;
		_Y = pPlayer->_Y;
	}

	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_Direction_check = ON_STOP;//정지중이라고 변경
	pPlayer->_Direction = _Direction;//공격하는 방향
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	CSerealBuffer* Attack2 = _PacketPool.Alloc();
	mp_ATTACK2(Attack2, pPlayer->_Id, _Direction, _X, _Y);
	//sendBroadCast(pSession, &Attack2);
	sendPacket_Around(pSession, Attack2, false);
	Attack2->Clear();
	_PacketPool.Free(Attack2);

	//범위내에 적이 있는지 확인하고 적이 있다면 모두 타격한다.
	st_SESSION* Atk2_tmp_Session;
	st_SECTOR_AROUND st_SECTOR_ATTACK2;
	unordered_map <DWORD, st_PLAYER*>* _Attack_Sector2;

	st_SECTOR_ATTACK2 = GetSectorAround(pPlayer->_cur_Pos._X, pPlayer->_cur_Pos._Y, &st_SECTOR_ATTACK2);

	for (int iCnt = 0; iCnt < 9; iCnt++)
	{
		//내섹터범위안에 있는 모든 유저를 확인하면서
		//범위안에 타격대상이 있는지 확인한다.
		int Cur_X = st_SECTOR_ATTACK2.Arroud[iCnt]._X;
		int Cur_Y = st_SECTOR_ATTACK2.Arroud[iCnt]._Y;

		if (!Sector_Check(Cur_X, Cur_Y))//섹터 범위를 벗어난게 있다면 전송안함
			continue;

		//0~99사이의 섹터라면
		_Attack_Sector2 = &g_Sector[Cur_Y][Cur_X];

		for (auto session_it = _Attack_Sector2->begin(); session_it != _Attack_Sector2->end(); ++session_it)
		{

			st_PLAYER* Atk2_tmp_Player = session_it->second;

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
						Disconnect(Atk2_tmp_Player->pSession);//클라이언트가 죽었으므로 종료
					}
					else
					{
						CSerealBuffer* Damage = _PacketPool.Alloc();
						Damage->Clear();
						mp_DAMAGE(Damage, pPlayer->_Id, Atk2_tmp_Player->_Id, Atk2_tmp_Player->_HP);
						//sendBroadCast(nullptr, &Damage);
						sendPacket_Around(Atk2_tmp_Player->pSession, Damage, false);
						Damage->Clear();
						_PacketPool.Free(Damage);
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
						Disconnect(Atk2_tmp_Player->pSession);//클라이언트가 죽었으므로 종료
					}
					else
					{
						CSerealBuffer* Damage = _PacketPool.Alloc();
						Damage->Clear();
						mp_DAMAGE(Damage, pPlayer->_Id, Atk2_tmp_Player->_Id, Atk2_tmp_Player->_HP);
						//sendBroadCast(nullptr, &Damage);
						sendPacket_Around(Atk2_tmp_Player->pSession, Damage, false);
						Damage->Clear();
						_PacketPool.Free(Damage);
					}
				}
			}
			break;
			}
		}
	}

#ifdef df_LOG
	printf("PACKET_ATTACK2 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y);
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
		//Disconnect(pSession);
		//return false;
		sync_attack3++;
		CSerealBuffer* sPacket = _PacketPool.Alloc();
		sPacket->Clear();
		mp_Sync(sPacket, pPlayer->_Id, pPlayer->_X, pPlayer->_Y);
		//sendBroadCast(pSession, &sPacket);
		sendPacket_UniCast(pSession, sPacket);
		sPacket->Clear();
		_PacketPool.Free(sPacket);
		_X = pPlayer->_X;
		_Y = pPlayer->_Y;
	}

	//받은 데이터를 서버 세션리스트 값에 업데이트 시켜준다.
	pPlayer->_Direction_check = ON_STOP;//정지중이라고 변경
	pPlayer->_Direction = _Direction;//공격하는 방향
	pPlayer->_X = _X;//받은 크기값을 넣는다.
	pPlayer->_Y = _Y;//받은 크기값을 넣는다.

	CSerealBuffer* Attack3 = _PacketPool.Alloc();
	mp_ATTACK3(Attack3, pPlayer->_Id, _Direction, _X, _Y);
	//sendBroadCast(pSession, &Attack3);
	sendPacket_Around(pSession, Attack3, false);
	Attack3->Clear();
	_PacketPool.Free(Attack3);

	//범위내에 적이 있는지 확인하고 적이 있다면 모두 타격한다.
	st_SESSION* Atk3_tmp_Session;
	st_SECTOR_AROUND st_SECTOR_ATTACK3;
	unordered_map <DWORD, st_PLAYER*>* _Attack_Sector;

	st_SECTOR_ATTACK3 = GetSectorAround(pPlayer->_cur_Pos._X, pPlayer->_cur_Pos._Y, &st_SECTOR_ATTACK3);

	for (int iCnt = 0; iCnt < 9; iCnt++)
	{
		//내섹터범위안에 있는 모든 유저를 확인하면서
		//범위안에 타격대상이 있는지 확인한다.
		int Cur_X = st_SECTOR_ATTACK3.Arroud[iCnt]._X;
		int Cur_Y = st_SECTOR_ATTACK3.Arroud[iCnt]._Y;

		if (!Sector_Check(Cur_X, Cur_Y))//섹터 범위를 벗어난게 있다면 전송안함
			continue;

		//0~99사이의 섹터라면
		_Attack_Sector = &g_Sector[Cur_Y][Cur_X];

		for (auto session_it = _Attack_Sector->begin(); session_it != _Attack_Sector->end(); ++session_it)
		{
			st_PLAYER* Atk3_tmp_Player = session_it->second;

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
						Disconnect(Atk3_tmp_Player->pSession);//클라이언트가 죽었으므로 종료
					}
					else
					{
						CSerealBuffer* Damage = _PacketPool.Alloc();
						Damage->Clear();
						mp_DAMAGE(Damage, pPlayer->_Id, Atk3_tmp_Player->_Id, Atk3_tmp_Player->_HP);
						//sendBroadCast(nullptr, &Damage);
						sendPacket_Around(Atk3_tmp_Player->pSession, Damage, false);
						Damage->Clear();
						_PacketPool.Free(Damage);
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
						Disconnect(Atk3_tmp_Player->pSession);//클라이언트가 죽었으므로 종료
					}
					else
					{
						CSerealBuffer* Damage = _PacketPool.Alloc();
						Damage->Clear();
						mp_DAMAGE(Damage, pPlayer->_Id, Atk3_tmp_Player->_Id, Atk3_tmp_Player->_HP);
						//sendBroadCast(nullptr, &Damage);
						sendPacket_Around(Atk3_tmp_Player->pSession, Damage, false);
						Damage->Clear();
						_PacketPool.Free(Damage);
					}
				}
			}
			break;
			}
		}
	}
#ifdef df_LOG
	printf("PACKET_ATTACK3 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pPlayer->_Id, pPlayer->_Direction, pPlayer->_X, pPlayer->_Y);
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
