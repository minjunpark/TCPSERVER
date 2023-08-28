#pragma once
#include "CLanServer.h"
#include "CTF_define.h"
#include "PacketDefine.h"
#include "Player.h"
#include "PacketFunction.h"

unsigned __stdcall _TF_LogicThread(void* param);

unsigned __stdcall _TF_LogicThread(void* param)
{
	IOCP_TF_SERVER* server = (IOCP_TF_SERVER*)param;
	//::WaitForSingleObject(server->_ThreadStartEvent, INFINITE);
	while (1)
	{

	}
	return server->IOCP_TF_TFLogicThread(param);
}

class IOCP_TF_SERVER : public CLanServer
{
public:
	IOCP_TF_SERVER() {};
	IOCP_TF_SERVER(const char* CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count, DWORD backlogSize) {};
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


private://TCP FIGHTER 사용함수

	SOCKET NetworkInitial();
	bool LoadData();

	void NetworkIOProcess();
	void Update();
	void ControlServer();
	void MonitorServerStatus();

	void LogSyncInfo(const char* functionName, int functionLine, Player* player, short x, short y);
	void LogDisconnectInfo(const char* functionName, int functionLine, Session* session, const char* reason);
	void LogCriticalError(const char* functionName, int functionLine, const char* reason);


	void PlayerUpdate(DWORD& deltaTime);
	void SectorUpdate(Player* player);


	//void AcceptProc();
	//void ReadProc(Session* session);
	//void SendProc();
	//void SendProc(Session* session);
	//void DisconnectProc(Session* session);
	void Disconnect();
	//void PushSendList(Session* session);
	//void SendUnicast(Session* targetSession, CSerealBuffer& packet);
	//void SendBroadCast(Session* exSession, CSerealBuffer& packet);
	//void SendBroadcastNearby(Session* session, Session* exSession, CSerealBuffer& packet);
	void SendInOneSector(int xIdx, int yIdx, CSerealBuffer& packet, Session* exSession);
	void RemoveSector(Player* player);
	void AddSector(Player* player);



	void NetworkCleanup();


	void PacketProc(PacketHeader header, Player* session, CSerealBuffer& packet);
	void PacketProcMoveStart(PacketHeader header, Player* session, CSerealBuffer& packet);
	void PacketProcMoveStop(PacketHeader header, Player* session, CSerealBuffer& packet);
	void PacketProcAttack1(PacketHeader header, Player* session, CSerealBuffer& packet);
	void PacketProcAttack2(PacketHeader header, Player* session, CSerealBuffer& packet);
	void PacketProcAttack3(PacketHeader header, Player* session, CSerealBuffer& packet);
	void PacketProcEcho(PacketHeader header, Player* session, CSerealBuffer& packet);

	void GetNearBySector(Player* player, SectorAround& oldSectorAround, SectorAround& curSectorAround);

	void LogCriticalError(const char* functionName, int functionLine, const char* reason)
	{
		time_t unixTime = time(nullptr);
		tm date;
		localtime_s(&date, &unixTime);
		char fileName[100]{ 0 };
		char tempBuffer[20]{ 0 };
		_itoa_s(date.tm_year + 1900, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		_itoa_s(date.tm_mon + 1, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		_itoa_s(date.tm_mday, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_log.txt");
		FILE* file = nullptr;
		while (file == nullptr)
			fopen_s(&file, fileName, "ab");
		fprintf(file, "Critical Error : %s::%d_%s\n", functionName, functionLine, reason);
		printf("Critical Error : %s::%d_%s\n", functionName, functionLine, reason);
		fclose(file);
	}
	void LogSyncInfo(const char* functionName, int functionLine, Player* player, short x, short y)
	{
		time_t unixTime = time(nullptr);
		tm date;
		localtime_s(&date, &unixTime);
		char fileName[100]{ 0 };
		char tempBuffer[20]{ 0 };
		_itoa_s(date.tm_year + 1900, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		_itoa_s(date.tm_mon + 1, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		_itoa_s(date.tm_mday, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_log.txt");
		FILE* file = nullptr;
		while (file == nullptr)
			fopen_s(&file, fileName, "ab");
		fprintf(file, "Sync Send SessionID : %d\t Server Pos : X %d, Y %d\t Client Pos : X %d, Y %d\n", player->sessionID, player->xPos, player->yPos, x, y);
		printf("Sync Send SessionID : %d\t Server Pos : X %d, Y %d\t Client Pos : X %d, Y %d\n", player->sessionID, player->xPos, player->yPos, x, y);
		fclose(file);
	}
	void LogDisconnectInfo(const char* functionName, int functionLine, Player* pPlayer, const char* reason)
	{
		time_t unixTime = time(nullptr);
		tm date;
		localtime_s(&date, &unixTime);
		char fileName[100]{ 0 };
		char tempBuffer[20]{ 0 };
		_itoa_s(date.tm_year + 1900, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		_itoa_s(date.tm_mon + 1, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		_itoa_s(date.tm_mday, tempBuffer, 10);
		strcat_s(fileName, tempBuffer);
		strcat_s(fileName, "_log.txt");
		FILE* file = nullptr;
		while (file == nullptr)
			fopen_s(&file, fileName, "ab");
		fprintf(file, "Disconnect SessionID : %d\n%s::%d_%s\n", pPlayer->sessionID, functionName, functionLine, reason);
		printf("Disconnect SessionID : %d\n%s::%d_%s\n", pPlayer->sessionID, functionName, functionLine, reason);
		fclose(file);
	}

private://변수 및 인스턴스



};

//DWORD old = timeGetTime();
//DWORD cur;
//DWORD sec = timeGetTime();
//DWORD deltaTime = 0;
//int update;
//void Update()
//{
//	cur = timeGetTime();
//	deltaTime += cur - old;
//	old = cur;
//
//	if (deltaTime < 40) return;
//	update++;
//	PlayerUpdate(deltaTime);
//}
//
//bool MoveCheck(short xPos, short yPos)
//{
//	if (xPos > MAX_MAP_WIDTH) return false;
//	else if (xPos < MIN_MAP_WIDTH) return false;
//	else if (yPos > MAX_MAP_HEIGHT) return false;
//	else if (yPos < MIN_MAP_WIDTH) return false;
//
//	return true;
//}
//
//void PlayerUpdate(DWORD& deltaTime)
//{
//	DWORD currentTime = timeGetTime();
//	while (deltaTime >= 40)
//	{
//		deltaTime -= 40;
//		//for (auto iter = g_PlayerMap.begin(); iter != g_PlayerMap.end(); ++iter)
//		//{
//		//	Player* currentPlayer = iter->second;
//		//	if (!g_DummyMode)
//		//	{
//		//		if (currentPlayer->HP <= 0)
//		//		{
//		//			LogDisconnectInfo(__FUNCTION__, __LINE__, currentPlayer->pSession, "currentPlayer's HP is zero");
//		//			DisconnectProc(currentPlayer->pSession);
//		//			continue;
//		//		}
//		//	}
//		//	if (currentTime - currentPlayer->pSession->lastRecvTime > 30000)
//		//	{
//		//		///LogDisconnectInfo(__FUNCTION__, __LINE__, currentPlayer->pSession, "RECV_TIMEOUT");
//		//		//DisconnectProc(currentPlayer->pSession);
//		//		continue;
//		//	}
//		//	switch (currentPlayer->moveDirection)
//		//	{
//		//	case dfPACKET_MOVE_DIR_LL:
//		//		if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos))
//		//		{
//		//			currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_LU:
//		//		if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
//		//		{
//		//			currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
//		//			currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_UU:
//		//		if (MoveCheck((int)currentPlayer->xPos, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
//		//		{
//		//			currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_RU:
//		//		if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos - VERTICAL_MOVE_SPEED))
//		//		{
//		//			currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
//		//			currentPlayer->yPos -= VERTICAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_RR:
//		//		if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos))
//		//		{
//		//			currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_RD:
//		//		if (MoveCheck(currentPlayer->xPos + HORIZONTAL_MOVE_SPEED, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
//		//		{
//		//			currentPlayer->xPos += HORIZONTAL_MOVE_SPEED;
//		//			currentPlayer->yPos += VERTICAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_DD:
//		//		if (MoveCheck(currentPlayer->xPos, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
//		//		{
//		//			currentPlayer->yPos += VERTICAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	case dfPACKET_MOVE_DIR_LD:
//		//		if (MoveCheck(currentPlayer->xPos - HORIZONTAL_MOVE_SPEED, currentPlayer->yPos + VERTICAL_MOVE_SPEED))
//		//		{
//		//			currentPlayer->xPos -= HORIZONTAL_MOVE_SPEED;
//		//			currentPlayer->yPos += VERTICAL_MOVE_SPEED;
//		//		}
//		//		break;
//		//	default:
//		//		continue;
//		//	}
//		//	//SectorUpdate(currentPlayer);
//		//}
//	}
//}
