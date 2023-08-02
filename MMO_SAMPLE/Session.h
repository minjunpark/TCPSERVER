#pragma once
#ifndef __SESSION_HEADER__
#define __SESSION_HEADER__
#define __UNIV_DEVELOPER_

#include "RingBuffer.h"

#include <unordered_map>
#include "ObjectFreeList.hpp"

namespace univ_dev
{
	constexpr int RECV_TIMEOUT = 60000;

	struct Session
	{
		SOCKET sock;
		DWORD sessionID;
		RingBuffer RQ;
		RingBuffer SQ;
		DWORD lastRecvTime;
	};
	extern std::unordered_map<SOCKET, Session*>g_SessionMap;
	extern ObjectFreeList<Session> g_SessionObjectPool;

	Session* FindSession(SOCKET sock);
	Session* CreateSession(SOCKET sock);
	void RemoveSession(SOCKET sock);
	
	std::unordered_map<SOCKET, Session*> g_SessionMap;
	univ_dev::ObjectFreeList<Session> g_SessionObjectPool;
	int g_SessionID = 1;
	Session* FindSession(SOCKET sock)
	{
		auto iter = g_SessionMap.find(sock);
		if (iter == g_SessionMap.end())
		{
			int* ptr = nullptr;
			*ptr = 100;
			return nullptr;
		}
		return iter->second;
	}
	Session* CreateSession(SOCKET sock)
	{
		Session* newSession = g_SessionObjectPool.Alloc();
		newSession->lastRecvTime = timeGetTime();
		newSession->sessionID = g_SessionID++;
		newSession->sock = sock;
		newSession->RQ.ClearBuffer();
		newSession->SQ.ClearBuffer();
		g_SessionMap.emplace(std::make_pair(sock, newSession));
		return newSession;
	}
	void RemoveSession(SOCKET sock)
	{
		auto iter = g_SessionMap.find(sock);
		if (iter == g_SessionMap.end())
		{
			int* ptr = nullptr;
			*ptr = 100;
			return;
		}
		Session* removeSession = iter->second;
		removeSession->RQ.ClearBuffer();
		removeSession->SQ.ClearBuffer();
		closesocket(removeSession->sock);

		g_SessionObjectPool.Free(removeSession);

		g_SessionMap.erase(sock);
	}

}




#endif // !__SESSION_HEADER__
