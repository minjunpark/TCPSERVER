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
	DWORD backlogSize);//시작
	// 오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	void Stop();//서버종료

	int GetSessionCount() { return SESSION_MAP.size(); }//현제 세션유저수

	bool Disconnect(DWORD64 SessionID); // SESSION_ID
	//bool SendPacket(DWORD64 SessionID, CSerealBuffer* CPacket);
	bool SendPacket(DWORD64 _pSESSIONID, CSerealBuffer* CPacket);
	
	//스레드
	void All_Moniter();//모든 통계정보 출력
	//friend unsigned int AcceptThread(void* pComPort);
	//friend unsigned int WorkerThread(void* pComPort);
	//friend unsigned __stdcall MonitorThread(void* pComPort);

	//unsigned int CLanMonitorThread(void* param);

	//서버 초기화
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
	void init_socket();//초기 초기화값

	//패킷 RECV SEND함수
	void RecvPost(st_SESSION* _pSession);
	void SendPost(st_SESSION* _pSession);
	
	//세션관리함수
	bool SesionRelease(st_SESSION* _pSession);

	virtual bool OnConnectionRequest(WCHAR* ipStr, DWORD ip, USHORT port) = 0;
	virtual void OnClientJoin(WCHAR* ipStr, DWORD ip, USHORT port, ULONGLONG sessionID) = 0;
	virtual void OnClientLeave(ULONGLONG sessionID) = 0; // Release후 호출
	virtual void OnRecv(ULONGLONG sessionID, CSerealBuffer* CPacket) = 0;
	virtual void OnError(int errorcode, WCHAR* msg) = 0;
	//virtual void OnErrorOccured(DWORD errorCode, const WCHAR* error) = 0;

	virtual void OnWorkerThreadBegin() = 0;//< 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd() = 0;//< 워커스레드 1루프 종료 후
	
	bool SessionReset(st_SESSION* _pSession);
	bool PacketReset(CSerealBuffer* CPacket);

	//모니터링함수
	int getAcceptTPS();//초당 들어온유저수
	int getRecvMessageTPS();//초당 Recv 처리수
	int getSendMessageTPS();//초당 Send 처리수

private:
	//스레드관리
	HANDLE _CreateThread[THREAD_MAXIMUM];//스레드 핸들
	HANDLE _AcceptThread;
	DWORD IOCP_Thread_Count;//총스레드 개수
	DWORD Thread_Running;
	bool Nagle_Onoff;
	bool SNDBUF_ZERO_OnOff;
	SOCKET listen_socket;
	char CLAN_IP[20];
	DWORD PORT;
	DWORD BACK_LOG_SIZE;

	DWORD64 UNI_KEY = 1;
	
	//종료를위한 플래그
	BOOL g_Shutdown = false;

	std::unordered_map <DWORD64, st_SESSION*> SESSION_MAP;
	CMemoryPool<st_SESSION> _SESSION_POOL;//세션 오브젝트 풀
	//CMemoryPool<CSerealBuffer> _PACKET_POOL;//패킷 오브젝트 풀
	
	DWORD _PACKET_POOL_Index;
	DWORD _PACKET_POOL_Count = 0;
	CMemoryPool<CSerealBuffer> _PACKET_POOL_ARR[THREAD_MAXIMUM];//세션 오브젝트 풀

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