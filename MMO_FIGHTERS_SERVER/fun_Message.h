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
	if (pPlayer == nullptr)//�÷��̾ ã�� ���ߴٸ� ��������
	{
		return false;
	}

	//XY��ġ �������� ����
	//6400 6400���Ͽ��� ����������
	
	//������ġ ����//��������

	//������ XY�� ���͸� Ŭ���̾�Ʈ���� �����Ѵ�.

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

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ������ ��ǥ ����ȭ
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

	//���ۺ���
	pPlayer->_Direction = _Direction;//�ٶ󺸴� ��������
	pPlayer->_Direction_check = ON_MOVE;//�̵��� �̶�� ����

	//���� �������� ����ڿ��� ��� ������ �Ѹ���
	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pPlayer->_X = _X;//���� ũ�Ⱚ�� �ִ´�.
	pPlayer->_Y = _Y;//���� ũ�Ⱚ�� �ִ´�.

	//����ȭ ���۸� �������� ������.
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

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���ۺ���
	pPlayer->_Direction_check = ON_STOP;//�̵��� �̶�� ����
	pPlayer->_Direction = _Direction;//�ٶ󺸴� ��������
	pPlayer->_X = _X;//���� ũ�Ⱚ�� �ִ´�.
	pPlayer->_Y = _Y;//���� ũ�Ⱚ�� �ִ´�.

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

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pPlayer->_Direction_check = ON_STOP;//�������̶�� ����
	pPlayer->_Direction = _Direction;//�����ϴ� ����
	pPlayer->_X = _X;//���� ũ�Ⱚ�� �ִ´�.
	pPlayer->_Y = _Y;//���� ũ�Ⱚ�� �ִ´�.

	CSerealBuffer Attack1;
	mp_ATTACK1(&Attack1, pSession->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &Attack1);

	//�������� ���� �ִ��� Ȯ���ϰ� ���� �ִٸ� ��� Ÿ���Ѵ�.
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
			continue;//�׾��ִ� ��Ŷ ����
		}

		if (pPlayer->_Id == Atk_tmp_Player->_Id)//�� �ڽ��̶�� �����Ѵ�.
		{
			continue;
		}

		switch (pPlayer->_Direction)//���� ������ ����
		{
		case dfPACKET_MOVE_DIR_LL:
		{
			if ((pPlayer->_X - dfATTACK1_RANGE_X <= Atk_tmp_Player->_X
				&& pPlayer->_X >= Atk_tmp_Player->_X)
				&& (abs(pPlayer->_Y - Atk_tmp_Player->_Y) <= dfATTACK1_RANGE_Y))
			{
				Atk_tmp_Player->_HP -= dfATTACK1_DAMAGE;//���� ����� HP�� ��´�.

				if (Atk_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk_tmp_Session);//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
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
				Atk_tmp_Player->_HP -= dfATTACK1_DAMAGE;//���� ����� HP�� ��´�.

				if (Atk_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk_tmp_Session);//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
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

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pPlayer->_Direction_check = ON_STOP;//�������̶�� ����
	pPlayer->_Direction = _Direction;//�����ϴ� ����
	pPlayer->_X = _X;//���� ũ�Ⱚ�� �ִ´�.
	pPlayer->_Y = _Y;//���� ũ�Ⱚ�� �ִ´�.
	
	CSerealBuffer Attack2;
	mp_ATTACK2(&Attack2, pPlayer->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &Attack2);

	//�������� ���� �ִ��� Ȯ���ϰ� ���� �ִٸ� ��� Ÿ���Ѵ�.
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
			continue;//�׾��ִ� ��Ŷ ����
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
				Atk2_tmp_Player->_HP -= dfATTACK2_DAMAGE;//���� ����� HP�� ��´�.

				if (Atk2_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk2_tmp_Session);//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
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
				Atk2_tmp_Player->_HP -= dfATTACK2_DAMAGE;//���� ����� HP�� ��´�.

				if (Atk2_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk2_tmp_Session);//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
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

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pPlayer->_X - _X) > dfERROR_RANGE ||
		abs(pPlayer->_Y - _Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pPlayer->_Direction_check = ON_STOP;//�������̶�� ����
	pPlayer->_Direction = _Direction;//�����ϴ� ����
	pPlayer->_X = _X;//���� ũ�Ⱚ�� �ִ´�.
	pPlayer->_Y = _Y;//���� ũ�Ⱚ�� �ִ´�.

	CSerealBuffer Attack3;
	mp_ATTACK3(&Attack3, pPlayer->_Id, _Direction, _X, _Y);
	sendBroadCast(pSession, &Attack3);

	//�������� ���� �ִ��� Ȯ���ϰ� ���� �ִٸ� ��� Ÿ���Ѵ�.
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
			continue;//�׾��ִ� ��Ŷ ����
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
				Atk3_tmp_Player->_HP -= dfATTACK3_DAMAGE;//���� ����� HP�� ��´�.

				if (Atk3_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk3_tmp_Session);//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
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
				Atk3_tmp_Player->_HP -= dfATTACK3_DAMAGE;//���� ����� HP�� ��´�.

				if (Atk3_tmp_Player->_HP <= 0)
				{
					Disconnect(Atk3_tmp_Session);//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
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