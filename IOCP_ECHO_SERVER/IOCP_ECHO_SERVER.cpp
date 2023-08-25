//#include "IOCP_ECHO_SERVER.h"
//
//IOCP_ECHO_SERVER::IOCP_ECHO_SERVER(const char* CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count, DWORD backlogSize)
//{
//	Start(CIP, CPORT, CThraed_Count, CThread_Running, CNagle_Onoff, CMax_Session_Count, backlogSize);
//};
//
//bool IOCP_ECHO_SERVER::OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port)
//{
//	return true;
//};
//
//void IOCP_ECHO_SERVER::OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) 
//{
//
//};
//
//void IOCP_ECHO_SERVER::OnClientLeave(ULONGLONG sessionID)
//{
//
//}; // Release후 호출
//
//void IOCP_ECHO_SERVER::OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket)
//{
//	DWORD64 Echo;
//	(*CPacket) >> Echo;
//	CSerealBuffer OnSendPacket;
//	OnSendPacket << Echo;
//	bool RetSend = SendPacket(sessionID, &OnSendPacket);
//	if (!RetSend)
//	{
//		printf("NotSend");
//	}
//};
//
//void IOCP_ECHO_SERVER::OnError(int errorcode, WCHAR* msg)
//{
//
//};
//
//void IOCP_ECHO_SERVER::OnWorkerThreadBegin()
//{
//
//};//< 워커스레드 GQCS 바로 하단에서 호출
//
//void IOCP_ECHO_SERVER::OnWorkerThreadEnd()
//{
//
//};//< 워커스레드 1루프 종료 후