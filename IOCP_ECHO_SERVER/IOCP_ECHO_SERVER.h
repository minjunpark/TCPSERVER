#pragma once
#include "CLanServer.h"

class IOCP_ECHO_SERVER : public CLanServer
{
public:
	IOCP_ECHO_SERVER() {};
	IOCP_ECHO_SERVER(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count);
	~IOCP_ECHO_SERVER() {};
private:
	//void Start();
	//void Stop();

	bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) override;
	void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) override;
	void OnClientLeave(ULONGLONG sessionID) override; // Release후 호출
	void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) override;

	void OnWorkerThreadBegin()override;//< 워커스레드 GQCS 바로 하단에서 호출
	void OnWorkerThreadEnd()override;//< 워커스레드 1루프 종료 후

};