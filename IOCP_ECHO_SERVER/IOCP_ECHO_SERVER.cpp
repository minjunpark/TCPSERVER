#include "IOCP_ECHO_SERVER.h"

IOCP_ECHO_SERVER::IOCP_ECHO_SERVER(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count)
{
	Start(CIP, CPORT, CThraed_Count, CThread_Running, CNagle_Onoff, CMax_Session_Count);
};

bool IOCP_ECHO_SERVER::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
{
	return true;
};

void IOCP_ECHO_SERVER::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) 
{

};

void IOCP_ECHO_SERVER::OnClientLeave(ULONGLONG sessionID)
{

}; // Release�� ȣ��

void IOCP_ECHO_SERVER::OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket)
{
	DWORD64 Echo;
	(*CPacket) >> Echo;
	CSerealBuffer OnSendPacket;
	OnSendPacket << Echo;
	SendPacket(sessionID, &OnSendPacket);
};

void IOCP_ECHO_SERVER::OnWorkerThreadBegin()
{

};//< ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
void IOCP_ECHO_SERVER::OnWorkerThreadEnd()
{

};//< ��Ŀ������ 1���� ���� ��