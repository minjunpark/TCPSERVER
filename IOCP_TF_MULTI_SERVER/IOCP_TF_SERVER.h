#pragma once
#include "CLanServer.h"
#include "CTF_define.h"

unsigned __stdcall _TF_LogicThread(void* param);

unsigned __stdcall _TF_LogicThread(void* param)
{
	IOCP_TF_SERVER* server = (IOCP_TF_SERVER*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	return server->IOCP_TF_TFLogicThread(param);
}

class IOCP_TF_SERVER : public CLanServer
{
public:
	IOCP_TF_SERVER() {};
	IOCP_TF_SERVER(const char* CIP, DWORD CPORT, DWORD CThraed_Count,
		DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count,
		DWORD backlogSize) {};
	~IOCP_TF_SERVER() {};
private:
	//void Start();
	//void Stop();

	friend unsigned __stdcall _TF_LogicThread(void* param);
	unsigned int IOCP_TF_TFLogicThread(void* pComPort);

	bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) override;
	void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) override;
	void OnClientLeave(ULONGLONG sessionID) override; // Release후 호출
	void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) override;
	void OnError(int errorcode, WCHAR* msg) override;

	void OnWorkerThreadBegin() override;//< 워커스레드 GQCS 바로 하단에서 호출
	void OnWorkerThreadEnd() override;//< 워커스레드 1루프 종료 후
};

DWORD old = timeGetTime();
DWORD cur;
DWORD sec = timeGetTime();
DWORD deltaTime = 0;
int update;
void Update()
{
	cur = timeGetTime();
	deltaTime += cur - old;
	old = cur;

	if (deltaTime < 40) return;
	update++;
	PlayerUpdate(deltaTime);
}

bool MoveCheck(short xPos, short yPos)
{
	if (xPos > MAX_MAP_WIDTH) return false;
	else if (xPos < MIN_MAP_WIDTH) return false;
	else if (yPos > MAX_MAP_HEIGHT) return false;
	else if (yPos < MIN_MAP_WIDTH) return false;

	return true;
}

void PlayerUpdate(DWORD& deltaTime)
{
	DWORD currentTime = timeGetTime();
	while (deltaTime >= 40)
	{
		deltaTime -= 40;
		//for (auto iter = g_PlayerMap.begin(); iter != g_PlayerMap.end(); ++iter)
		//{
		//	Player* currentPlayer = iter->second;
		//	if (!g_DummyMode)
		//	{
		//		if (currentPlayer->HP <= 0)
		//		{
		//			LogDisconnectInfo(__FUNCTION__, __LINE__, currentPlayer->pSession, "currentPlayer's HP is zero");
		//			DisconnectProc(currentPlayer->pSession);
		//			continue;
		//		}
		//	}
		//	if (currentTime - currentPlayer->pSession->lastRecvTime > 30000)
		//	{
		//		///LogDisconnectInfo(__FUNCTION__, __LINE__, currentPlayer->pSession, "RECV_TIMEOUT");
		//		//DisconnectProc(currentPlayer->pSession);
		//		continue;
		//	}
		//	switch (currentPlayer->moveDirection)
		//	{
		//	case dfPACKET_MOVE_DIR_LL:
		//		if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos))
		//		{
		//			currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_LU:
		//		if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
		//		{
		//			currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
		//			currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_UU:
		//		if (MoveCheck((int)currentPlayer->xPos, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
		//		{
		//			currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_RU:
		//		if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
		//		{
		//			currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
		//			currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_RR:
		//		if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos))
		//		{
		//			currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_RD:
		//		if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
		//		{
		//			currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
		//			currentPlayer->yPos += VERTICAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_DD:
		//		if (MoveCheck(currentPlayer->xPos, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
		//		{
		//			currentPlayer->yPos += VERTICAL_MOVE_SPEED;
		//		}
		//		break;
		//	case dfPACKET_MOVE_DIR_LD:
		//		if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
		//		{
		//			currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
		//			currentPlayer->yPos += VERTICAL_MOVE_SPEED;
		//		}
		//		break;
		//	default:
		//		continue;
		//	}
		//	//SectorUpdate(currentPlayer);
		//}
	}
}
