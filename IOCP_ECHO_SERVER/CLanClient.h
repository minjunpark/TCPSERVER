#include "CNetLibrary.h"

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

	virtual void OnEnterJoinServer() = 0;// < �������� ���� ���� ��
	virtual void OnLeaveServer() = 0;// < �������� ������ �������� ��

	virtual void OnRecv(DWORD64 sessionID, CSerealBuffer* CPacket) = 0;
	virtual void OnSend(DWORD64 sessionID) = 0;
	virtual void OnError(int errorcode, WCHAR*msg) = 0;
		
private:
	//���������
	HANDLE _CreateThread[30];//������ �ڵ�
	DWORD Thread_Count;//�ѽ����� ����
	DWORD Thread_Running;//���׽�����
	bool Nagle_Onoff;//���̱۾˰��� ���࿩��
	SOCKET listen_socket;
	SOCKET con_socket;//����ȼ���
	DWORD PORT;//��Ʈ��ȣ

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