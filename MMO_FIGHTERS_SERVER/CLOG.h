#pragma once
#include <Windows.h>
#include <cstdio>
#include <time.h>
#include <direct.h>
#include <errno.h>
#include <iostream>
class CLOG
{
public:
	constexpr static int MAX_LOG_SIZE = 2048;
	enum en_LOG_LEVEL
	{
		//LOG_LEVEL_ALL,
		//LOG_LEVEL_LIBRARY,
		//LOG_LEVEL_WARNING,
		LOG_LEVEL_DEBUG = 0,
		LOG_LEVEL_ERROR = 1,
		LOG_LEVEL_SYSTEM = 2
	};
public:
	CLOG();
	~CLOG();
	void LOG_SET_LEVEL(en_LOG_LEVEL level);
	void LOG_SET_DIRECTORY(const WCHAR* directory);
	void LOG(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szStringFormat);
	void LOG_HEX(const WCHAR* szType, en_LOG_LEVEL LogLevel, BYTE* pByte, int iByteLen, const WCHAR* szLog);
	static CLOG* GetInstance() 
	{
		if (instance == nullptr) {
			instance = new CLOG();
		}
		return instance;
	}
private:
	en_LOG_LEVEL _LogLevel;
	WCHAR		_Directory[MAX_PATH];
	UINT64		_LogCount;
	static CLOG* instance;
};

