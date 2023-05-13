#pragma once
#include <math.h>

bool netPacketProc_SC_CREATE_MY_CHARACTER(Session* pSession, char* pPacket);
void mp_SC_CREATE_MY_CHARACTER(st_dfPACKET_header* pHeader, st_dfPACKET_SC_CREATE_MY_CHARACTER* pPacket, int p_id, int p_Direction, int p_Hp, int p_X, int p_Y);

bool netPacketProc_SC_CREATE_OTHER_CHARACTER(Session* pSession, char* pPacket);
bool mp_SC_CREATE_OTHER_CHARACTER(st_dfPACKET_header* pHeader, st_dfPACKET_SC_CREATE_OTHER_CHARACTER* pPacket, int p_id, int p_Direction, int p_Hp, int p_X, int p_Y);

bool netPacketProc_DELETE_CHARACTER(Session* pSession, char* pPacket);
void mp_DELETE_CHARACTER(st_dfPACKET_header* pHeader, st_dfPACKET_SC_DELETE_CHARACTER* pPacket, int p_id);

bool netPacketProc_MOVE_START(Session* pSession, char* pPacket);
void mp_MOVE_START(st_dfPACKET_header* pHeader, st_dfPACKET_SC_MOVE_START* pPacket, int p_id, int p_Direction, int p_X, int p_Y);

bool netPacketProc_MOVE_STOP(Session* pSession, char* pPacket);
void mp_MOVE_STOP(st_dfPACKET_header* pHeader, st_dfPACKET_SC_MOVE_STOP* pPacket, int p_id, int p_Direction, int p_X, int p_Y);

bool netPacketProc_ATTACK1(Session* pSession, char* pPacket);
void mp_ATTACK1(st_dfPACKET_header* pHeader, st_dfPACKET_SC_ATTACK1* pPacket, int p_id, int p_Direction, int p_X, int p_Y);

bool netPacketProc_ATTACK2(Session* pSession, char* pPacket);
void mp_ATTACK2(st_dfPACKET_header* pHeader, st_dfPACKET_SC_ATTACK2* pPacket, int p_id, int p_Direction, int p_X, int p_Y);

bool netPacketProc_ATTACK3(Session* pSession, char* pPacket);
void mp_ATTACK3(st_dfPACKET_header* pHeader, st_dfPACKET_SC_ATTACK3* pPacket, int p_id, int p_Direction, int p_X, int p_Y);

//bool netPacketProc_DAMAGE(Session* pSession, char* pPacket);
void mp_DAMAGE(st_dfPACKET_header* pHeader, st_dfPACKET_SC_DAMAGE* pPacket, int _a_id, int _d_id, char _hp);



//dfPACKET_SC_CREATE_MY_CHARACTER
bool netPacketProc_SC_CREATE_MY_CHARACTER(Session* pSession, char* pPacket)
{
	st_dfPACKET_header st_my_header;
	st_dfPACKET_SC_CREATE_MY_CHARACTER st_my_create;
	mp_SC_CREATE_MY_CHARACTER(&st_my_header, &st_my_create, pSession->_Id, pSession->_Direction, pSession->_HP, pSession->_X, pSession->_Y);
	sendUniCast(pSession, (char*)&st_my_header, (char*)&st_my_create, sizeof(st_dfPACKET_SC_CREATE_MY_CHARACTER));

#ifdef df_LOG
	printf("Create Character # SessionID %d  X:%d Y:%d\n", pSession->_Id, defualt_X_SET, defualt_Y_SET);
#endif

	return true;
};

void mp_SC_CREATE_MY_CHARACTER(st_dfPACKET_header* pHeader, st_dfPACKET_SC_CREATE_MY_CHARACTER* pPacket, int p_id, int p_Direction, int p_Hp, int p_X, int p_Y)
{
	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_CREATE_MY_CHARACTER);
	pHeader->byType = dfPACKET_SC_CREATE_MY_CHARACTER;

	pPacket->_Id = p_id;
	pPacket->_Direction = p_Direction;
	pPacket->HP = p_Hp;
	pPacket->_X = p_X;
	pPacket->_Y = p_Y;
};


//dfPACKET_SC_CREATE_OTHER_CHARACTER
bool netPacketProc_SC_CREATE_OTHER_CHARACTER(Session* pSession, char* pPacket)
{
	st_dfPACKET_header st_other_header;
	st_dfPACKET_SC_CREATE_OTHER_CHARACTER st_other_create;
	mp_SC_CREATE_OTHER_CHARACTER(&st_other_header, &st_other_create, 
		pSession->_Id, pSession->_Direction, pSession->_HP, pSession->_X, pSession->_Y);
	sendBroadCast(pSession, (char*)&st_other_header, (char*)&st_other_create, sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER));

	return true;
};

bool mp_SC_CREATE_OTHER_CHARACTER(st_dfPACKET_header* pHeader, st_dfPACKET_SC_CREATE_OTHER_CHARACTER* pPacket, int p_id, int p_Direction, int p_Hp, int p_X, int p_Y)
{
	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_CREATE_OTHER_CHARACTER);
	pHeader->byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	pPacket->_Id = p_id;
	pPacket->_Direction = p_Direction;
	pPacket->HP = p_Hp;
	pPacket->_X = p_X;
	pPacket->_Y = p_Y;

	return true;
};

//dfPACKET_SC_DELETE_CHARACTER
bool netPacketProc_DELETE_CHARACTER(Session* pSession, char* pPacket)
{
	st_dfPACKET_header st_header;
	st_dfPACKET_SC_DELETE_CHARACTER st_delete;
	mp_DELETE_CHARACTER(&st_header, &st_delete, pSession->_Id);
	sendBroadCast(nullptr, (char*)&st_header, (char*)&st_delete, sizeof(st_dfPACKET_SC_DELETE_CHARACTER));
#ifdef df_LOG
	printf("Delete Session # SessionID %d \n", pSession->_Id);
#endif
	return true;
};

void mp_DELETE_CHARACTER(st_dfPACKET_header* pHeader, st_dfPACKET_SC_DELETE_CHARACTER* pPacket, int p_id)
{
	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_DELETE_CHARACTER);
	pHeader->byType = dfPACKET_SC_DELETE_CHARACTER;

	pPacket->_Id = p_id;
};

//dfPACKET_SC_MOVE_START
bool netPacketProc_MOVE_START(Session* pSession, char* pPacket)
{
	//������ ��Ŷ ����
	st_dfPACKET_CS_MOVE_START* pMoveStart = (st_dfPACKET_CS_MOVE_START*)pPacket;

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pSession->_X - pMoveStart->_X) > dfERROR_RANGE ||
		abs(pSession->_Y - pMoveStart->_Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		printf("netPacketProc_MoveStart abs error\n");
		printf("pSession->_X %d\n", pSession->_X);
		printf("pMoveStart->_X %d\n", pMoveStart->_X);
		return false;
	}

	//���ۺ���
	pSession->_Direction = pMoveStart->_Direction;//�ٶ󺸴� ��������
	pSession->_Direction_check = ON_MOVE;//�̵��� �̶�� ����

	//���� �������� ����ڿ��� ��� ������ �Ѹ���
	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pSession->_X = pMoveStart->_X;//���� ũ�Ⱚ�� �ִ´�.
	pSession->_Y = pMoveStart->_Y;//���� ũ�Ⱚ�� �ִ´�.

	//�������
	st_dfPACKET_header st_SC_MOVE_START_HAEDER;
	st_dfPACKET_SC_MOVE_START st_SC_MOVE_START;
	mp_MOVE_START(&st_SC_MOVE_START_HAEDER, &st_SC_MOVE_START, pSession->_Id, pMoveStart->_Direction, pMoveStart->_X, pMoveStart->_Y);
	sendBroadCast(pSession, (char*)&st_SC_MOVE_START_HAEDER, (char*)&st_SC_MOVE_START, sizeof(st_dfPACKET_SC_MOVE_START));

#ifdef df_LOG
	printf("PACKET_MOVESTART # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif

	return true;
};

void mp_MOVE_START(st_dfPACKET_header* pHeader, st_dfPACKET_SC_MOVE_START* pPacket, int p_id, int p_Direction, int p_X, int p_Y)
{
	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_MOVE_START);
	pHeader->byType = dfPACKET_SC_MOVE_START;

	pPacket->_Id = p_id;//� �༮���� ID����
	pPacket->_Direction = p_Direction;//���� ��������
	pPacket->_X = p_X;//��ġX
	pPacket->_Y = p_Y;//��ġY
}

//dfPACKET_MOVE_STOP
bool netPacketProc_MOVE_STOP(Session* pSession, char* pPacket)
{
	st_dfPACKET_CS_MOVE_STOP* pMoveStop = (st_dfPACKET_CS_MOVE_STOP*)pPacket;

	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pSession->_X - pMoveStop->_X) > dfERROR_RANGE ||
		abs(pSession->_Y - pMoveStop->_Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���ۺ���
	pSession->_Direction_check = ON_STOP;//�̵��� �̶�� ����
	pSession->_Direction = pMoveStop->_Direction;//�ٶ󺸴� ��������
	pSession->_X = pMoveStop->_X;//���� ũ�Ⱚ�� �ִ´�.
	pSession->_Y = pMoveStop->_Y;//���� ũ�Ⱚ�� �ִ´�.

	st_dfPACKET_header st_SC_MOVE_STOP_HAEDER;
	st_dfPACKET_SC_MOVE_STOP st_SC_MOVE_STOP;
	mp_MOVE_STOP(&st_SC_MOVE_STOP_HAEDER, &st_SC_MOVE_STOP, pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
	sendBroadCast(pSession, (char*)&st_SC_MOVE_STOP_HAEDER, (char*)&st_SC_MOVE_STOP, sizeof(st_dfPACKET_SC_MOVE_STOP));
#ifdef df_LOG
	printf("PACKET_MOVESTOP # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;
}

void mp_MOVE_STOP(st_dfPACKET_header* pHeader, st_dfPACKET_SC_MOVE_STOP* pPacket,
	int p_id, int p_Direction, int p_X, int p_Y)
{
	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_MOVE_STOP);
	pHeader->byType = dfPACKET_SC_MOVE_STOP;

	pPacket->_Id = p_id;//� �༮���� ID����
	pPacket->_Direction = p_Direction;//���� ��������
	pPacket->_X = p_X;//��ġX
	pPacket->_Y = p_Y;//��ġY
}

//dfPACKET_ATTACK1
bool netPacketProc_ATTACK1(Session* pSession, char* pPacket)
{
	st_dfPACKET_CS_ATTACK1* pAttack1 = (st_dfPACKET_CS_ATTACK1*)pPacket;
	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pSession->_X - pAttack1->_X) > dfERROR_RANGE ||
		abs(pSession->_Y - pAttack1->_Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pSession->_Direction_check = ON_STOP;//�������̶�� ����
	pSession->_Direction = pAttack1->_Direction;//�����ϴ� ����
	pSession->_X = pAttack1->_X;//���� ũ�Ⱚ�� �ִ´�.
	pSession->_Y = pAttack1->_Y;//���� ũ�Ⱚ�� �ִ´�.

	st_dfPACKET_header st_SC_ATTACK1_HAEDER;
	st_dfPACKET_SC_ATTACK1 st_SC_ATTACK1;
	mp_ATTACK1(&st_SC_ATTACK1_HAEDER, &st_SC_ATTACK1, pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
	sendBroadCast(pSession, (char*)&st_SC_ATTACK1_HAEDER, (char*)&st_SC_ATTACK1, sizeof(st_dfPACKET_SC_ATTACK1));

	switch (pSession->_Direction)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if ((*session_it)->_Live == false)
			{
				continue;//�׾��ִ� ��Ŷ ����
			}

			if (pSession->_Id == (*session_it)->_Id)
			{
				continue;
			}

			if ((pSession->_X - dfATTACK1_RANGE_X <= (*session_it)->_X
				&& pSession->_X >= (*session_it)->_X)
				&& ((pSession->_Y - dfATTACK1_RANGE_Y <= (*session_it)->_Y
				&& pSession->_Y >= (*session_it)->_Y)
				|| (pSession->_Y + dfATTACK1_RANGE_Y >= (*session_it)->_Y
				&& pSession->_Y <= (*session_it)->_Y)))//80���� ���� �����ϴ� Ÿ���� �ִٸ�?
			{
				(*session_it)->_HP -= dfATTACK1_DAMAGE;//���� ����� HP�� ��´�.

				if ((*session_it)->_HP <= 0)
				{
					Disconnect((*session_it));//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
				}
				else
				{
					st_dfPACKET_header st_SC_DAMAGE_HEADER;
					st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, (*session_it)->_Id, (*session_it)->_HP);
					sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
				}
				break;
			}
		}
	}
	break;
	case dfPACKET_MOVE_DIR_RR:
	{
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if ((*session_it)->_Live == false)
			{
				continue;//�׾��ִ� ��Ŷ ����
			}

			if (pSession->_Id == (*session_it)->_Id)
			{
				continue;
			}

			if ((pSession->_X + dfATTACK1_RANGE_X >= (*session_it)->_X
				&& pSession->_X <= (*session_it)->_X)
				&& ((pSession->_Y - dfATTACK1_RANGE_Y <= (*session_it)->_Y
					&& pSession->_Y >= (*session_it)->_Y)
					|| (pSession->_Y + dfATTACK1_RANGE_Y >= (*session_it)->_Y
						&& pSession->_Y <= (*session_it)->_Y)))//80���� ���� �����ϴ� Ÿ���� �ִٸ�?
			{
				(*session_it)->_HP -= dfATTACK1_DAMAGE;//���� ����� HP�� ��´�.

				if ((*session_it)->_HP <= 0)
				{
					Disconnect((*session_it));//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
				}
				else
				{
					st_dfPACKET_header st_SC_DAMAGE_HEADER;
					st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, (*session_it)->_Id, (*session_it)->_HP);
					sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
				}
				break;
			}
		}
	}
	}


#ifdef df_LOG
	printf("PACKET_ATTACK1 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;

}

void mp_ATTACK1(st_dfPACKET_header* pHeader, st_dfPACKET_SC_ATTACK1* pPacket, int p_id, int p_Direction, int p_X, int p_Y)
{

	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_MOVE_STOP);
	pHeader->byType = dfPACKET_SC_ATTACK1;

	pPacket->_Id = p_id;//� �༮���� ID����
	pPacket->_Direction = p_Direction;//���� ��������
	pPacket->_X = p_X;//��ġX
	pPacket->_Y = p_Y;//��ġY
}



//dfPACKET_ATTACK2
bool netPacketProc_ATTACK2(Session* pSession, char* pPacket)
{
	st_dfPACKET_CS_ATTACK2* pAttack2 = (st_dfPACKET_CS_ATTACK2*)pPacket;
	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pSession->_X - pAttack2->_X) > dfERROR_RANGE ||
		abs(pSession->_Y - pAttack2->_Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pSession->_Direction_check = ON_STOP;//�������̶�� ����
	pSession->_Direction = pAttack2->_Direction;//�����ϴ� ����
	pSession->_X = pAttack2->_X;//���� ũ�Ⱚ�� �ִ´�.
	pSession->_Y = pAttack2->_Y;//���� ũ�Ⱚ�� �ִ´�.

	st_dfPACKET_header st_SC_ATTACK2_HAEDER;
	st_dfPACKET_SC_ATTACK2 st_SC_ATTACK2;
	mp_ATTACK2(&st_SC_ATTACK2_HAEDER, &st_SC_ATTACK2, pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
	sendBroadCast(pSession, (char*)&st_SC_ATTACK2_HAEDER, (char*)&st_SC_ATTACK2, sizeof(st_dfPACKET_SC_ATTACK1));

	switch (pSession->_Direction)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if ((*session_it)->_Live == false)
			{
				continue;//�׾��ִ� ��Ŷ ����
			}

			if (pSession->_Id == (*session_it)->_Id)
			{
				continue;
			}

			if ((pSession->_X - dfATTACK2_RANGE_X <= (*session_it)->_X
				&& pSession->_X >= (*session_it)->_X)
				&& ((pSession->_Y - dfATTACK2_RANGE_Y <= (*session_it)->_Y
				&& pSession->_Y >= (*session_it)->_Y)
				|| (pSession->_Y + dfATTACK2_RANGE_Y >= (*session_it)->_Y
				&& pSession->_Y <= (*session_it)->_Y)))//80���� ���� �����ϴ� Ÿ���� �ִٸ�?
			{
				(*session_it)->_HP -= dfATTACK2_DAMAGE;//���� ����� HP�� ��´�.

				if ((*session_it)->_HP <= 0)
				{
					Disconnect((*session_it));//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
				}
				else
				{
					//st_dfPACKET_header st_SC_DAMAGE_HEADER;
					//st_SC_DAMAGE_HEADER.byCode = PACKET_CODE;
					//st_SC_DAMAGE_HEADER.bySize = sizeof(st_dfPACKET_SC_DAMAGE);
					//st_SC_DAMAGE_HEADER.byType = dfPACKET_SC_DAMAGE;

					//st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					//st_SC_DAMAGE.Hp = (*session_it)->_HP;
					//st_SC_DAMAGE._Attack_Id = pSession->_Id;
					//st_SC_DAMAGE._Damage_Id = (*session_it)->_Id;
					st_dfPACKET_header st_SC_DAMAGE_HEADER;
					st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, (*session_it)->_Id, (*session_it)->_HP);
					sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
				}
				break;
			}
		}
	}
	break;
	case dfPACKET_MOVE_DIR_RR:
	{
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if ((*session_it)->_Live == false)
			{
				continue;//�׾��ִ� ��Ŷ ����
			}

			if (pSession->_Id == (*session_it)->_Id)
			{
				continue;
			}

			if ((pSession->_X + dfATTACK2_RANGE_X >= (*session_it)->_X
				&& pSession->_X <= (*session_it)->_X)
				&& ((pSession->_Y - dfATTACK2_RANGE_Y <= (*session_it)->_Y
					&& pSession->_Y >= (*session_it)->_Y)
					|| (pSession->_Y + dfATTACK2_RANGE_Y >= (*session_it)->_Y
						&& pSession->_Y <= (*session_it)->_Y)))//80���� ���� �����ϴ� Ÿ���� �ִٸ�?
			{
				(*session_it)->_HP -= dfATTACK2_DAMAGE;//���� ����� HP�� ��´�.

				if ((*session_it)->_HP <= 0)
				{
					Disconnect((*session_it));//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
				}
				else
				{
					st_dfPACKET_header st_SC_DAMAGE_HEADER;
					st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, (*session_it)->_Id, (*session_it)->_HP);
					sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
				}
				break;
			}
		}
	}
	}

#ifdef df_LOG
	printf("PACKET_ATTACK2 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif
	return true;
}

void mp_ATTACK2(st_dfPACKET_header* pHeader, st_dfPACKET_SC_ATTACK2* pPacket, int p_id, int p_Direction, int p_X, int p_Y)
{

	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_MOVE_STOP);
	pHeader->byType = dfPACKET_SC_ATTACK2;

	pPacket->_Id = p_id;//� �༮���� ID����
	pPacket->_Direction = p_Direction;//���� ��������
	pPacket->_X = p_X;//��ġX
	pPacket->_Y = p_Y;//��ġY
}

//dfPACKET_ATTACK3
bool netPacketProc_ATTACK3(Session* pSession, char* pPacket)
{
	st_dfPACKET_CS_ATTACK3* pAttack3 = (st_dfPACKET_CS_ATTACK3*)pPacket;
	//���¼����� �̵��������� ��ǥ�� �ʹ� ��߳������� ��������
	if (abs(pSession->_X - pAttack3->_X) > dfERROR_RANGE ||
		abs(pSession->_Y - pAttack3->_Y) > dfERROR_RANGE)
	{
		Disconnect(pSession);
		return false;
	}

	//���� �����͸� ���� ���Ǹ���Ʈ ���� ������Ʈ �����ش�.
	pSession->_Direction_check = ON_STOP;//�������̶�� ����
	pSession->_Direction = pAttack3->_Direction;//�����ϴ� ����
	pSession->_X = pAttack3->_X;//���� ũ�Ⱚ�� �ִ´�.
	pSession->_Y = pAttack3->_Y;//���� ũ�Ⱚ�� �ִ´�.

	st_dfPACKET_header st_SC_ATTACK3_HAEDER;
	st_dfPACKET_SC_ATTACK3 st_SC_ATTACK3;
	mp_ATTACK3(&st_SC_ATTACK3_HAEDER, &st_SC_ATTACK3, pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
	sendBroadCast(pSession, (char*)&st_SC_ATTACK3_HAEDER, (char*)&st_SC_ATTACK3, sizeof(st_dfPACKET_SC_ATTACK3));

	switch (pSession->_Direction)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if ((*session_it)->_Live == false)
			{
				continue;//�׾��ִ� ��Ŷ ����
			}

			if (pSession->_Id == (*session_it)->_Id)
			{
				continue;
			}

			if ((pSession->_X - dfATTACK3_RANGE_X <= (*session_it)->_X
				&& pSession->_X >= (*session_it)->_X)
				&& ((pSession->_Y - dfATTACK3_RANGE_Y <= (*session_it)->_Y
					&& pSession->_Y >= (*session_it)->_Y)
					|| (pSession->_Y + dfATTACK3_RANGE_Y >= (*session_it)->_Y
						&& pSession->_Y <= (*session_it)->_Y)))//80���� ���� �����ϴ� Ÿ���� �ִٸ�?
			{
				(*session_it)->_HP -= dfATTACK3_DAMAGE;//���� ����� HP�� ��´�.

				if ((*session_it)->_HP <= 0)
				{
					Disconnect((*session_it));//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
				}
				else
				{
					st_dfPACKET_header st_SC_DAMAGE_HEADER;
					st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, (*session_it)->_Id, (*session_it)->_HP);
					sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
				}
				break;
			}
		}
	}
	break;
	case dfPACKET_MOVE_DIR_RR:
	{
		for (auto session_it = Session_List.begin(); session_it != Session_List.end(); ++session_it)
		{
			if ((*session_it)->_Live == false)
			{
				continue;//�׾��ִ� ��Ŷ ����
			}

			if (pSession->_Id == (*session_it)->_Id)
			{
				continue;
			}

			if ((pSession->_X + dfATTACK3_RANGE_X >= (*session_it)->_X
				&& pSession->_X <= (*session_it)->_X)
				&& ((pSession->_Y - dfATTACK3_RANGE_Y <= (*session_it)->_Y
					&& pSession->_Y >= (*session_it)->_Y)
					|| (pSession->_Y + dfATTACK3_RANGE_Y >= (*session_it)->_Y
						&& pSession->_Y <= (*session_it)->_Y)))//80���� ���� �����ϴ� Ÿ���� �ִٸ�?
			{
				(*session_it)->_HP -= dfATTACK3_DAMAGE;//���� ����� HP�� ��´�.

				if ((*session_it)->_HP <= 0)
				{
					Disconnect((*session_it));//Ŭ���̾�Ʈ�� �׾����Ƿ� ����
				}
				else
				{
					st_dfPACKET_header st_SC_DAMAGE_HEADER;
					st_dfPACKET_SC_DAMAGE st_SC_DAMAGE;
					mp_DAMAGE(&st_SC_DAMAGE_HEADER, &st_SC_DAMAGE, pSession->_Id, (*session_it)->_Id, (*session_it)->_HP);
					sendBroadCast(nullptr, (char*)&st_SC_DAMAGE_HEADER, (char*)&st_SC_DAMAGE, sizeof(st_dfPACKET_SC_DAMAGE));
				}
				break;
			}
		}
	}
	}


#ifdef df_LOG
	printf("PACKET_ATTACK3 # SessionID: %d / Direction:%d / X:%d / Y:%d\n", pSession->_Id, pSession->_Direction, pSession->_X, pSession->_Y);
#endif

	return true;
}

void mp_ATTACK3(st_dfPACKET_header* pHeader, st_dfPACKET_SC_ATTACK3* pPacket, int p_id, int p_Direction, int p_X, int p_Y)
{

	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_MOVE_STOP);
	pHeader->byType = dfPACKET_SC_ATTACK3;

	pPacket->_Id = p_id;//� �༮���� ID����
	pPacket->_Direction = p_Direction;//���� ��������
	pPacket->_X = p_X;//��ġX
	pPacket->_Y = p_Y;//��ġY
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

void mp_DAMAGE(st_dfPACKET_header* pHeader, st_dfPACKET_SC_DAMAGE* pPacket, int _a_id,int _d_id,char _hp)
{
	pHeader->byCode = PACKET_CODE;
	pHeader->bySize = sizeof(st_dfPACKET_SC_DAMAGE);
	pHeader->byType = dfPACKET_SC_DAMAGE;

	pPacket->_Attack_Id = _a_id;//� �༮���� ID����
	pPacket->_Damage_Id = _d_id;//���� ��������
	pPacket->Hp = _hp;//��ġX
}

//dfPACKET_CS_SYNC

//dfPACKET_SC_SYNC