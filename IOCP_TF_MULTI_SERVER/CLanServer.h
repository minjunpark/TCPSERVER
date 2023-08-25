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

unsigned __stdcall _NET_WorkerThread(void* param);
unsigned __stdcall _NET_AcceptThread(void* param);
unsigned __stdcall _NET_LogicThread(void* param);

class CLanServer
{
public:
	CLanServer();
	~CLanServer();

	bool Start(const char* CIP, DWORD CPORT, DWORD CThraed_Count,
	DWORD CThread_Running, bool CNagle_Onoff, DWORD CMax_Session_Count,
	DWORD backlogSize);//����
	// ���� IP / ��Ʈ / ��Ŀ������ ��(������, ���׼�) / ���ۿɼ� / �ִ������� ��
	void Stop();//��������

	int GetSessionCount() { return SESSION_MAP.size(); }//���� ����������

	bool Disconnect(DWORD64 SessionID); // SESSION_ID
	//bool SendPacket(DWORD64 SessionID, CSerealBuffer* CPacket);
	bool SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket);
	
	//������
	void All_Moniter();//��� ������� ���
	//friend unsigned int AcceptThread(void* pComPort);
	//friend unsigned int WorkerThread(void* pComPort);
	//friend unsigned __stdcall MonitorThread(void* pComPort);

	//unsigned int CLanMonitorThread(void* param);

	//���� �ʱ�ȭ
	//void init_map();
	bool Socket_Error();

private:
	enum
	{
		THREAD_MAXIMUM = 30
	};
	friend unsigned __stdcall _NET_WorkerThread(void* param);
	friend unsigned __stdcall _NET_AcceptThread(void* param);
	friend unsigned __stdcall _NET_LogicThread(void* param);

	unsigned int CLanAcceptThread(void* pComPort);
	unsigned int CLanWorkerThread(void* pComPort);
	unsigned int CLanLogicThread(void* pComPort);

	//void init_Set();
	void init_socket();//�ʱ� �ʱ�ȭ��

	//��Ŷ RECV SEND�Լ�
	void RecvPost(st_SESSION* _pSession);
	void SendPost(st_SESSION* _pSession);
	
	//���ǰ����Լ�
	bool SesionRelease(st_SESSION* _pSession);

	virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
	virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
	virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release�� ȣ��
	virtual void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) = 0;
	virtual void OnError(int errorcode, WCHAR* msg) = 0;
	//virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;

	virtual void OnWorkerThreadBegin() = 0;//< ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadEnd() = 0;//< ��Ŀ������ 1���� ���� ��
	
	bool SessionReset(st_SESSION* _pSession);
	bool PacketReset(CSerealBuffer* CPacket);

	//����͸��Լ�
	int getAcceptTPS();//�ʴ� ����������
	int getRecvMessageTPS();//�ʴ� Recv ó����
	int getSendMessageTPS();//�ʴ� Send ó����

private:
	//���������
	HANDLE _CreateThread[THREAD_MAXIMUM];//������ �ڵ�
	HANDLE _AcceptThread;
	DWORD IOCP_Thread_Count;//�ѽ����� ����
	DWORD Thread_Running;
	bool Nagle_Onoff;
	bool SNDBUF_ZERO_OnOff;
	SOCKET listen_socket;
	char CLAN_IP[20];
	DWORD PORT;
	DWORD BACK_LOG_SIZE;

	DWORD64 UNI_KEY = 1;
	
	//���Ḧ���� �÷���
	BOOL g_Shutdown = false;

	std::unordered_map <DWORD64, st_SESSION*> SESSION_MAP;
	CMemoryPool<st_SESSION> _SESSION_POOL;//���� ������Ʈ Ǯ
	//CMemoryPool<CSerealBuffer> _PACKET_POOL;//��Ŷ ������Ʈ Ǯ
	
	DWORD _PACKET_POOL_Index;
	DWORD _PACKET_POOL_Count = 0;
	CMemoryPool<CSerealBuffer> _PACKET_POOL_ARR[THREAD_MAXIMUM];//���� ������Ʈ Ǯ

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