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
	bool Connect(DWORD64 CIP, DWORD CPORT, DWORD CThraed_Count,bool CNagle_Onoff);//시작
	bool Disconnect(DWORD64 SessionID); // SESSION_ID
	bool SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket);

private:
	friend unsigned __stdcall _NET_ConncetThread(void* param);
	friend unsigned __stdcall _NET_WorkerThread(void* param);
	
	unsigned int CLanConncetThread(void* pComPort);
	unsigned int CLanWorkerThread(void* pComPort);

	//void init_Set();
	void init_socket();//초기 초기화값

	//패킷 RECV SEND함수
	void RecvPost(st_SESSION* _pSession);
	void SendPost(st_SESSION* _pSession);

	//세션관리함수
	bool SesionRelease(st_SESSION* _pSession);

	virtual void OnEnterJoinServer() = 0;// < 서버와의 연결 성공 후
	virtual void OnLeaveServer() = 0;// < 서버와의 연결이 끊어졌을 때
	virtual void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) = 0;//패킷수신 완료후
	virtual void OnSend(int sendsize) = 0;//패킷송신 완료 후
	virtual void OnError(int errorcode, WCHAR* msg) = 0;
	
	virtual void OnWorkerThreadBegin() = 0;//< 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd() = 0;//< 워커스레드 1루프 종료 후

	bool SessionReset(st_SESSION* _pSession);
	bool PacketReset(CSerealBuffer* CPacket);

private:
	//스레드관리
	HANDLE _CreateThread[30];//스레드 핸들
	HANDLE _AcceptThread;
	DWORD IOCP_Thread_Count;//총스레드 개수
	DWORD Thread_Running;
	bool Nagle_Onoff;
	bool SNDBUF_ZERO_OnOff;
	SOCKET con_socket;
	DWORD PORT;
	DWORD BACK_LOG_SIZE;
	SOCKADDR_IN _Con_Server_addr;

	DWORD64 UNI_KEY = 1;

	//종료를위한 플래그
	BOOL g_Shutdown = false;

	std::unordered_map <DWORD64, st_SESSION*> SESSION_MAP;

	DWORD _PACKET_POOL_Index;
	DWORD _PACKET_POOL_Count = 0;
	CMemoryPool<CSerealBuffer> _PACKET_POOL_ARR[30];//세션 오브젝트 풀

	DWORD MAX_SESSION_COUNT;
	SRWLOCK SESSION_MAP_LOCK;//세션맵용 LOCK
	SRWLOCK SESSION_POOL_LOCK;//세션맵용 LOCK
	//SRWLOCK PACKET_LOCK;//세션맵용 LOCK
	HANDLE IOCP_WORKER_POOL;

	//모니터링용
	ULONGLONG _Start_Time;//서버시작시간
	ULONGLONG _Cur_Time;//현재시간
	ULONGLONG _Old_Time;//현재시간

	//모니터링용 카운트
	ULONGLONG _AcceptCount;
	ULONGLONG _RecvCount;
	ULONGLONG _SendCount;

	ULONGLONG _ALL_AcceptCount;
	ULONGLONG _ALL_RecvCount;
	ULONGLONG _ALL_SendCount;

	ULONGLONG _Session_Count;
};