#include "CNetLibrary.h"

struct st_SESSION
{
	SOCKET _Sock;
	DWORD64 _Id;
	SOCKADDR_IN _Sock_Addr;
	CRingBuffer _SendBuf;
	CRingBuffer _RecvBuf;
	OVERLAPPED _SendOver;
	OVERLAPPED _RecvOver;
	SRWLOCK _LOCK;
	alignas(64)
		DWORD _IO_Flag;
	alignas(64)
		DWORD _IO_COUNT;
};

unsigned __stdcall _NET_ConncetThread(void* param);
unsigned __stdcall _NET_WorkerThread(void* param);

class CLanClient
{
public:
	CLanClient();
	~CLanClient();
	bool Connect(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count,bool CNagle_Onoff);//����
	bool Disconnect(DWORD64 SessionID); // SESSION_ID
	bool SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket);

private:
	friend unsigned __stdcall _NET_ConncetThread(void* param);
	friend unsigned __stdcall _NET_WorkerThread(void* param);
	
	unsigned int CLanConncetThread(void* pComPort);
	unsigned int CLanWorkerThread(void* pComPort);

	//void init_Set();
	void init_socket();//�ʱ� �ʱ�ȭ��

	//��Ŷ RECV SEND�Լ�
	void RecvPost(st_SESSION* _pSession);
	void SendPost(st_SESSION* _pSession);

	//���ǰ����Լ�
	bool SesionRelease(st_SESSION* _pSession);

	virtual void OnEnterJoinServer() = 0;// < �������� ���� ���� ��
	virtual void OnLeaveServer() = 0;// < �������� ������ �������� ��
	virtual void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) = 0;//��Ŷ���� �Ϸ���
	virtual void OnSend(int sendsize) = 0;//��Ŷ�۽� �Ϸ� ��
	virtual void OnError(int errorcode, WCHAR* msg) = 0;
	
	virtual void OnWorkerThreadBegin() = 0;//< ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadEnd() = 0;//< ��Ŀ������ 1���� ���� ��

	bool SessionReset(st_SESSION* _pSession);
	bool PacketReset(CSerealBuffer* CPacket);

private:
	//���������
	HANDLE _CreateThread[30];//������ �ڵ�
	HANDLE _AcceptThread;
	DWORD IOCP_Thread_Count;//�ѽ����� ����
	DWORD Thread_Running;
	bool Nagle_Onoff;
	bool SNDBUF_ZERO_OnOff;
	SOCKET con_socket;
	DWORD PORT;
	DWORD BACK_LOG_SIZE;
	SOCKADDR_IN _Con_Server_addr;

	DWORD64 UNI_KEY = 1;

	//���Ḧ���� �÷���
	BOOL g_Shutdown = false;

	std::unordered_map <DWORD64, st_SESSION*> SESSION_MAP;

	DWORD _PACKET_POOL_Index;
	DWORD _PACKET_POOL_Count = 0;
	CMemoryPool<CSerealBuffer> _PACKET_POOL_ARR[30];//���� ������Ʈ Ǯ

	DWORD MAX_SESSION_COUNT;
	SRWLOCK SESSION_MAP_LOCK;//���Ǹʿ� LOCK
	SRWLOCK SESSION_POOL_LOCK;//���Ǹʿ� LOCK
	//SRWLOCK PACKET_LOCK;//���Ǹʿ� LOCK
	HANDLE IOCP_WORKER_POOL;

	//����͸���
	ULONGLONG _Start_Time;//�������۽ð�
	ULONGLONG _Cur_Time;//����ð�
	ULONGLONG _Old_Time;//����ð�

	//����͸��� ī��Ʈ
	ULONGLONG _AcceptCount;
	ULONGLONG _RecvCount;
	ULONGLONG _SendCount;

	ULONGLONG _ALL_AcceptCount;
	ULONGLONG _ALL_RecvCount;
	ULONGLONG _ALL_SendCount;

	ULONGLONG _Session_Count;
};