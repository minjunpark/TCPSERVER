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
	void OnClientLeave(ULONGLONG sessionID) override; // Release�� ȣ��
	void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) override;

	void OnWorkerThreadBegin()override;//< ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	void OnWorkerThreadEnd()override;//< ��Ŀ������ 1���� ���� ��

};