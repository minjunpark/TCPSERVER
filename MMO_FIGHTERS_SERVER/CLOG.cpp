#include "CLOG.h"

CLOG* CLOG::instance = nullptr;//�̱����� ���� �ʱ�ó��

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
	//���丮 �����
	_wmkdir(_Directory);
};


void CLOG::LOG(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szStringFormat)
{
	//���� �������� ���� �αװ� ������ ����
	if (LogLevel < _LogLevel) return;
	
	time_t cur_t=time(NULL);
	tm tmCurTime;
	
	//�ð�����
	localtime_s(&tmCurTime, &cur_t);
	//�������ڿ� �������� ��¥�������� ����.
	
	WCHAR _L_FILENAME[128];//���� �̸�
	WCHAR _L_TIME_FORMAT[64];

	FILE* _L_FILE;
	wcscpy_s(_L_FILENAME, sizeof(_Directory), _Directory);//�����̸� ����
	wcscat_s(_L_FILENAME, szType);//Ÿ�� ���� ���̱�
	wcsftime(_L_TIME_FORMAT, sizeof(_L_FILENAME), L"_%Y%m%d.log", &tmCurTime);//Ÿ�� ���� ����
	wcscat_s(_L_FILENAME, _L_TIME_FORMAT);//Ÿ�� ���� ���̱�

	errno_t _L_error;
	_L_error = _wfopen_s(&_L_FILE, _L_FILENAME, L"ab");
	
	//va_list va;
	//va_start(va, szStringFormat);
	//hResult = StringCchVPrintf(szInMessage, 256, szStringFormat, va);
	//va_end(va);
	
	//�α׷����� ���� ���
	//fseek(_L_FILE, SEEK_END, 0);

	fwprintf_s(_L_FILE, L"[%s] ", szType);
	//�α�ī��Ʈ ���ϱ�
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

	//�ð�����
	localtime_s(&tmCurTime, &cur_t);
	//�������ڿ� �������� ��¥�������� ����.

	WCHAR _L_FILENAME[128];//���� �̸�
	WCHAR _L_TIME_FORMAT[64];

	FILE* _L_FILE;
	wcscpy_s(_L_FILENAME, sizeof(_Directory), _Directory);//�����̸� ����
	wcscat_s(_L_FILENAME, szType);//Ÿ�� ���� ���̱�
	wcsftime(_L_TIME_FORMAT, sizeof(_L_FILENAME), L"_%Y%m%d_HEX.log", &tmCurTime);//Ÿ�� ���� ����
	wcscat_s(_L_FILENAME, _L_TIME_FORMAT);//Ÿ�� ���� ���̱�

	errno_t _L_error;
	_L_error = _wfopen_s(&_L_FILE, _L_FILENAME, L"ab");

	fwprintf_s(_L_FILE, L"[%s] ", szType);
	//�α�ī��Ʈ ���ϱ�
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
	fwprintf_s(_L_FILE, szLog);//�α׻��� �Է�
	fwprintf_s(_L_FILE, L"\n");
	
	//Ư�� �����ͺ��� ����Ʈ ���̸�ŭ �޸𸮸� �����ؼ� �����Ѵ�.
	unsigned char* DUMP_BINARY = new unsigned char[iByteLen];
	memcpy(DUMP_BINARY, pByte, iByteLen);//�ӵ������� _s�� ���� ������.
	
	unsigned char* TEMP_BINARY = DUMP_BINARY;//������ �����ؾ��ϹǷ� �ӽ� �����͸�����.
	for (int i = 1;i<= iByteLen;i++)
	{
		fwprintf_s(_L_FILE, L"[%02X]", *TEMP_BINARY);//��絥���ͷ� �����Ѵ�.
		TEMP_BINARY++;//���̳ʸ��� ��ĭ �̵��Ѵ�.
		if(i % 8 == 0)//8ĭ�� ���Ͽ� �Է��Ѵ�.
			fwprintf_s(_L_FILE, L"\n");
	}

	fwprintf_s(_L_FILE, L"\n");//�������� �����ߴٸ� ��ĭ����ְ�
	delete[] DUMP_BINARY;//Ȯ���ߴ� �޸𸮸� �����ش�.
	fclose(_L_FILE);//������ �ݾƼ� �����ϰ� �������ش�.
	_LogCount++;//�α�ī��Ʈ�� �߰�����
};
