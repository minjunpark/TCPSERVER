#pragma once
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"winmm.lib")
#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <conio.h>
#include <unordered_map>
#include <Winsock2.h>
#include <cstdlib>
#include <cstdio>
#include <process.h>
#include <windows.h>
#include "CRingBuffer.h"
//#include "CSRingBuffer.h"
#include "CSerealBuffer.h"

struct SESSION
{
	SOCKET _Sock;
	DWORD64 _Id;
	SOCKADDR_IN _Sock_Addr;
	CRingBuffer _SendBuf;
	CRingBuffer _RecvBuf;
	OVERLAPPED _SendOver;
	OVERLAPPED _RecvOver;
	alignas(64)
		SRWLOCK _LOCK;
	alignas(64)
		DWORD _IO_Flag;
	alignas(64)
		DWORD _IO_COUNT;
};

unsigned __stdcall _NET_Client_WorkerThread(void* param);
unsigned __stdcall _NET_Client_AcceptThread(void* param);

class CLanClient
{
public:
	CLanClient();
	~CLanClient();

	bool Connect(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count, DWORD CThread_Running, bool CNagle_Onoff);//����
	// ���� IP / ��Ʈ / ��Ŀ������ ��(������, ���׼�) / ���ۿɼ� / �ִ������� ��
	void Stop();//��������

	bool Disconnect(); // SESSION_ID
	//bool SendPacket(DWORD64 SessionID, CSerealBuffer* CPacket);
	void SendPacket(CSerealBuffer* CPacket);
	
	bool Socket_Error();

private:
	friend unsigned __stdcall _NET_Client_WorkerThread(void* param);
	friend unsigned __stdcall _NET_Client_AcceptThread(void* param);

	unsigned int CLanClientAcceptThread(void* pComPort);
	unsigned int CLanClientWorkerThread(void* pComPort);

	//void init_Set();
	void init_socket();

	//��Ŷ RECV SEND�Լ�
	void RecvPost(SESSION* _pSession);
	void SendPost(SESSION* _pSession);

	virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
	virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
	virtual void OnClientLeave(DWORD64 sessionID) = 0; // Release�� ȣ��
	virtual void OnRecv(DWORD64 sessionID, CSerealBuffer* CPacket) = 0;
	//virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;

	//virtual void OnTimeOut(ULONGLONG sessionID) = 0;
	//virtual void OnSend(ULONGLONG sessionID) = 0;

	//virtual void OnSend(SessionID, int sendsize) = 0;           < ��Ŷ �۽� �Ϸ� ��


private:
	//���������
	HANDLE _CreateThread[30];//������ �ڵ�
	DWORD Thread_Count;//�ѽ����� ����
	DWORD Thread_Running;
	bool Nagle_Onoff;
	SOCKET listen_socket;
	SOCKET con_socket;
	DWORD PORT;

	//���Ḧ���� �Լ�
	BOOL g_Shutdown = false;

	SESSION* Player_Session;
	HANDLE IOCP_WORKER_POOL;

	//����͸���
	ULONGLONG _Start_Time;//�������۽ð�
	ULONGLONG _Cur_Time;//����ð�
	ULONGLONG _Old_Time;//����ð�

	//DWORD AcceptCount;
	ULONGLONG _RecvCount;
	ULONGLONG _SendCount;

	ULONGLONG _ALL_RecvCount;
	ULONGLONG _ALL_SendCount;
	
};