#include "CLOG.h"

CLOG* CLOG::instance = nullptr;//싱글톤을 위한 초기처리

CLOG::CLOG()
{
	_LogLevel = en_LOG_LEVEL::LOG_LEVEL_ERROR;
	_LogCount = 0;
	wcscpy_s(_Directory, L"Default");
};

CLOG::~CLOG()
{
	
};

void CLOG::LOG_SET_LEVEL(en_LOG_LEVEL level)
{
	_LogLevel = level;
};

void CLOG::LOG_SET_DIRECTORY(const WCHAR* directory)
{
	wcscpy_s(_Directory, directory);
	wcscat_s(_Directory, L"/");
	//디렉토리 만들기
	_wmkdir(_Directory);
};


void CLOG::LOG(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szStringFormat)
{
	//현재 레벨보다 낮은 로그가 들어오면 무시
	if (LogLevel < _LogLevel) return;
	
	time_t cur_t=time(NULL);
	tm tmCurTime;
	
	//시간설정
	localtime_s(&tmCurTime, &cur_t);
	//받은문자열 파일저장 날짜기준으로 하자.
	
	WCHAR _L_FILENAME[128];//파일 이름
	WCHAR _L_TIME_FORMAT[64];

	FILE* _L_FILE;
	wcscpy_s(_L_FILENAME, sizeof(_Directory), _Directory);//파일이름 세팅
	wcscat_s(_L_FILENAME, szType);//타임 포맷 붙이기
	wcsftime(_L_TIME_FORMAT, sizeof(_L_FILENAME), L"_%Y%m%d.log", &tmCurTime);//타임 포맷 세팅
	wcscat_s(_L_FILENAME, _L_TIME_FORMAT);//타임 포맷 붙이기

	errno_t _L_error;
	_L_error = _wfopen_s(&_L_FILE, _L_FILENAME, L"ab");
	
	//va_list va;
	//va_start(va, szStringFormat);
	//hResult = StringCchVPrintf(szInMessage, 256, szStringFormat, va);
	//va_end(va);
	
	//로그레벨에 따라 출력
	//fseek(_L_FILE, SEEK_END, 0);

	fwprintf_s(_L_FILE, L"[%s] ", szType);
	//로그카운트 더하기
	switch (LogLevel)
	{
		case CLOG::LOG_LEVEL_DEBUG:
			fwprintf_s(_L_FILE, L"[%d-%02d-%02d %02d:%02d:%02d / DEBUG  / %d] ", tmCurTime.tm_year+1900, tmCurTime.tm_mon+1, tmCurTime.tm_mday, tmCurTime.tm_hour,tmCurTime.tm_min, tmCurTime.tm_sec, _LogCount);
			break;
		case CLOG::LOG_LEVEL_ERROR:
			fwprintf_s(_L_FILE, L"[%d-%02d-%02d %02d:%02d:%02d / ERROR  / %d] ", tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min, tmCurTime.tm_sec, _LogCount);
			break;
		case CLOG::LOG_LEVEL_SYSTEM:
			fwprintf_s(_L_FILE, L"[%d-%02d-%02d %02d:%02d:%02d / SYSTEM / %d] ", tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min, tmCurTime.tm_sec, _LogCount);
			break;
		default:
			break;
	}
	fwprintf_s(_L_FILE, szStringFormat);
	fwprintf_s(_L_FILE, L"\n");
	_LogCount++;
	fclose(_L_FILE);
};

void CLOG::LOG_HEX(const WCHAR* szType, en_LOG_LEVEL LogLevel,  BYTE* pByte, int iByteLen, const WCHAR* szLog)
{
	if (LogLevel < _LogLevel) return;

	time_t cur_t = time(NULL);
	tm tmCurTime;

	//시간설정
	localtime_s(&tmCurTime, &cur_t);
	//받은문자열 파일저장 날짜기준으로 하자.

	WCHAR _L_FILENAME[128];//파일 이름
	WCHAR _L_TIME_FORMAT[64];

	FILE* _L_FILE;
	wcscpy_s(_L_FILENAME, sizeof(_Directory), _Directory);//파일이름 세팅
	wcscat_s(_L_FILENAME, szType);//타임 포맷 붙이기
	wcsftime(_L_TIME_FORMAT, sizeof(_L_FILENAME), L"_%Y%m%d_HEX.log", &tmCurTime);//타임 포맷 세팅
	wcscat_s(_L_FILENAME, _L_TIME_FORMAT);//타임 포맷 붙이기

	errno_t _L_error;
	_L_error = _wfopen_s(&_L_FILE, _L_FILENAME, L"ab");

	fwprintf_s(_L_FILE, L"[%s] ", szType);
	//로그카운트 더하기
	switch (LogLevel)
	{
	case CLOG::LOG_LEVEL_DEBUG:
		fwprintf_s(_L_FILE, L"[%d-%02d-%02d %02d:%02d:%02d / DEBUG  / %d] ", tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min, tmCurTime.tm_sec, _LogCount);
		break;
	case CLOG::LOG_LEVEL_ERROR:
		fwprintf_s(_L_FILE, L"[%d-%02d-%02d %02d:%02d:%02d / ERROR  / %d] ", tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min, tmCurTime.tm_sec, _LogCount);
		break;
	case CLOG::LOG_LEVEL_SYSTEM:
		fwprintf_s(_L_FILE, L"[%d-%02d-%02d %02d:%02d:%02d / SYSTEM / %d] ", tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min, tmCurTime.tm_sec, _LogCount);
		break;
	default:
		break;
	}
	fwprintf_s(_L_FILE, szLog);//로그사유 입력
	fwprintf_s(_L_FILE, L"\n");
	
	//특정 포인터부터 바이트 길이만큼 메모리를 복사해서 저장한다.
	unsigned char* DUMP_BINARY = new unsigned char[iByteLen];
	memcpy(DUMP_BINARY, pByte, iByteLen);//속도를위해 _s는 굳이 사용안함.
	
	unsigned char* TEMP_BINARY = DUMP_BINARY;//원본은 삭제해야하므로 임시 포인터를쓴다.
	for (int i = 1;i<= iByteLen;i++)
	{
		fwprintf_s(_L_FILE, L"[%02X]", *TEMP_BINARY);//헥사데이터로 저장한다.
		TEMP_BINARY++;//바이너리를 한칸 이동한다.
		if(i % 8 == 0)//8칸씩 파일에 입력한다.
			fwprintf_s(_L_FILE, L"\n");
	}

	fwprintf_s(_L_FILE, L"\n");//마지막에 도달했다면 한칸띄워주고
	delete[] DUMP_BINARY;//확보했던 메모리를 돌려준다.
	fclose(_L_FILE);//파일을 닫아서 완전하게 저장해준다.
	_LogCount++;//로그카운트를 추가한후
};
