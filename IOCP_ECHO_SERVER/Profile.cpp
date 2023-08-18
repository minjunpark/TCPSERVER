#include <iostream>
#include <Windows.h>
#include "Profile.h"
#pragma comment(lib, "winmm.lib")

enum
{
	PROFILE_SIZE = 50
};

struct PROFILE_INFO
{
	long			lFlag = 0;			// 프로파일의 사용 여부. (배열시에만)
	WCHAR			szName[64];			// 프로파일 샘플 이름.

	LARGE_INTEGER	lStartTime;			// 프로파일 샘플 실행 시간.

	__int64			iTotalTime;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	__int64			iMin[2];			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	__int64			iMax[2];			// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])

	__int64			iCall;				// 누적 호출 횟수.
};

struct PROFILE_THREAD
{
	DWORD Thread_Id;
	PROFILE_INFO _P_ARR[PROFILE_SIZE];
};

const char* _tag;//태그 이름
WCHAR _P_FILENAME[128];//파일 이름
PROFILE_THREAD _P_ARR_Th[PROFILE_SIZE];//THREAD별 값
//PROFILE_INFO _P_ARR[PROFILE_SIZE];//ARRAY LIST 최대크기 50
LARGE_INTEGER _P_Start;//시작시간
LARGE_INTEGER _P_End;//끝난시간
LARGE_INTEGER _P_Freq;//기준시간
LONGLONG _P_RealTime;//경과시간
FILE* _P_FILE;
errno_t _P_error;
DWORD _P_Thread_Tls_Index;//ThreadId값을 판별하기 위한값
DWORD _P_Thread_Count;

BOOL _Profile_onoff = false;

void InitProfile(void)
{
	timeBeginPeriod(1);
	QueryPerformanceFrequency(&_P_Freq);//시간 측정을 위한

	_P_Thread_Tls_Index = TlsAlloc();
	_P_Thread_Count = 0;

	ZeroMemory(_P_ARR_Th, sizeof(PROFILE_THREAD) * PROFILE_SIZE);
}

void ProfileBegin(const WCHAR* szName)
{
	if (_Profile_onoff) return;
	DWORD currentThreadIdx = (DWORD)TlsGetValue(_P_Thread_Tls_Index);

	if (currentThreadIdx == 0)
	{
		//처음일때
		currentThreadIdx = InterlockedIncrement(&_P_Thread_Count);
		_P_ARR_Th[currentThreadIdx].Thread_Id = GetCurrentThreadId();
		if (!TlsSetValue(_P_Thread_Tls_Index, (LPVOID)currentThreadIdx))
		{
			int* ptr = nullptr;
			*ptr = 100;
		}
	}

	//네임이 있다면 빠른 시간측정을위해 탈출한다
	for (int iPA = 0; iPA < PROFILE_SIZE; iPA++)
	{
		if (0 == wcscmp(_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].szName, szName))
		{
			QueryPerformanceCounter(&_P_Start);//가장하단에서 시간측정
			_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime = _P_Start;
			return;
		}
	}
	//네임이 없다면 새로등록한다
	for (int iPA = 0; iPA < PROFILE_SIZE; iPA++)
	{
		if (_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lFlag == 0)
		{
			_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lFlag = 1;
			wcscpy_s(_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].szName, szName);
			_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iTotalTime = 0;//콜횟수 초기화
			_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iMin[0] = 10000000;//최소실행시간 
			_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iMax[0] = 0;//최대 실행시간
			QueryPerformanceCounter(&_P_Start);//가장하단에서 시간측정
			_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime = _P_Start;
			return;
		}
	}
};

void ProfileEnd(const WCHAR* szName)
{
	QueryPerformanceCounter(&_P_End);//가장 상단에서 시간측정
	if (_Profile_onoff) return;

	DWORD currentThreadIdx = (DWORD)TlsGetValue(_P_Thread_Tls_Index);
	if (currentThreadIdx == 0)
	{
		//처음일때
		currentThreadIdx = InterlockedIncrement(&_P_Thread_Count);
		_P_ARR_Th[currentThreadIdx].Thread_Id = GetCurrentThreadId();
		if (!TlsSetValue(_P_Thread_Tls_Index, (LPVOID)currentThreadIdx))
		{
			int* ptr = nullptr;
			*ptr = 100;
		}
	}
	for (int iPA = 0; iPA < PROFILE_SIZE; iPA++)
	{
		if (_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lFlag != 0)
		{
			if (0 == wcscmp(_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].szName, szName))
			{//태그값이 있다면
				++_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iCall;//누적 콜횟수증가
				_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iTotalTime =
					_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iTotalTime +
					(_P_End.QuadPart - _P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime.QuadPart);//토탈 시간증가

				if (_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iMin[0] > _P_End.QuadPart -
					_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime.QuadPart)
				{//최소 실행시간
					_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iMin[0] = _P_End.QuadPart -
						_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime.QuadPart;//최소 실행시간 입력
				}

				if (_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iMax[0] < _P_End.QuadPart -
					_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime.QuadPart)
				{//최소 실행시간
					_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].iMax[0] = _P_End.QuadPart -
						_P_ARR_Th[currentThreadIdx]._P_ARR[iPA].lStartTime.QuadPart;//최소 실행시간 입력
				}
				break;
			}
			else
			{//태그값이 없으면
				//printf("TAG NOT %s %d\n", __FILE__, __LINE__);//파일 위치를 보고확인한다
				//throw std::bad_exception{};//고의 에러 발생시키기
			}
		}
	}
};

void ProfileDataOutText(const WCHAR* szFileName)
{
	_Profile_onoff = true;

	time_t curTime = time(NULL);
	struct tm tmCurTime;

	_P_error = localtime_s(&tmCurTime, &curTime);
	if (_P_error != 0)
	{
		printf("현재시간을 얻을수없다\n");
		return;
	}

	WCHAR _P_TIME_FORMAT[64];

	wcscpy_s(_P_FILENAME, sizeof(_P_FILENAME), szFileName);//파일이름 세팅
	wcsftime(_P_TIME_FORMAT, sizeof(_P_FILENAME), L"_%Y%m%d_%I%M%S.txt", &tmCurTime);//타임 포맷 세팅
	wcscat_s(_P_FILENAME, _P_TIME_FORMAT);//타임 포맷 붙이기

	_P_error = _wfopen_s(&_P_FILE, _P_FILENAME, L"wb");
	if (_P_error != 0)
	{
		printf("파일을 생성 할 수 없습니다.\n");
		return;
	}
	if (_P_FILE == nullptr) {
		printf("파일을 생성하지 못했습니다..\n");
		return;
	}
	volatile int iTA = 1;
	for (;iTA <= _P_Thread_Count; iTA++)
	{
		fwprintf(_P_FILE, L"-------------------------------------------------------------------------------\n");
		fwprintf(_P_FILE, L"        Thread_Id %d \n", _P_ARR_Th[iTA].Thread_Id);
		fwprintf(_P_FILE, L"           Name  |     Average  |        Min   |        Max   |       Call |\n");
		fwprintf(_P_FILE, L"---------------------------------------------------------------------------\n");
		for (int iPA = 0; iPA < PROFILE_SIZE; iPA++)
		{
			if (_P_ARR_Th[iTA]._P_ARR[iPA].lFlag)//플래그가 있다면
			{
				__int64 iTotal = _P_ARR_Th[iTA]._P_ARR[iPA].iTotalTime - (_P_ARR_Th[iTA]._P_ARR[iPA].iMax[0] + _P_ARR_Th[iTA]._P_ARR[iPA].iMin[0]);//최소값 최대값 평균제외해서 더하기
				__int64 iAverage = iTotal / (_P_ARR_Th[iTA]._P_ARR[iPA].iCall - 2);//평균시간 구하기 -2는 최소 최대값 제외

				double dftTotalDu = (double)iAverage * 1000.0 / (double)_P_Freq.QuadPart;         //평균밀리세컨드 계산
				double dftMinDu = (double)_P_ARR_Th[iTA]._P_ARR[iPA].iMin[0] * 1000.0 / (double)_P_Freq.QuadPart;//최소값 밀리세컨드 계산
				double dftMaxDu = (double)_P_ARR_Th[iTA]._P_ARR[iPA].iMax[0] * 1000.0 / (double)_P_Freq.QuadPart;//최대값 밀리세컨드 계산

				fwprintf(_P_FILE, L"%16s | %10.4lf㎲ | %10.4lf㎲ | %10.4lf㎲ | %10lld |\n",
					_P_ARR_Th[iTA]._P_ARR[iPA].szName, dftTotalDu, dftMinDu, dftMaxDu, _P_ARR_Th[iTA]._P_ARR[iPA].iCall);//포맷화 시켜서 입력
			}
		}
		fwprintf(_P_FILE, L"\n");
		fwprintf(_P_FILE, L"-------------------------------------------------------------------------------\n");
	}
	fclose(_P_FILE);

	_Profile_onoff = false;
};

void ProfilePrint(void)
{
	for (int iTA = 0; iTA <= _P_Thread_Count; iTA++)
	{
		for (int iPA = 0; iPA < PROFILE_SIZE; iPA++)
		{
			if (_P_ARR_Th[iTA]._P_ARR[iPA].lFlag)
			{
				__int64 iTotal = _P_ARR_Th[iTA]._P_ARR[iPA].iTotalTime - (_P_ARR_Th[iTA]._P_ARR[iPA].iMax[0] + _P_ARR_Th[iTA]._P_ARR[iPA].iMin[0]);//최대시간 구하기
				__int64 iAverage = _P_ARR_Th[iTA]._P_ARR[iPA].iTotalTime / _P_ARR_Th[iTA]._P_ARR[iPA].iCall;//평균시간 구하기
				wprintf(L"%lld %lld", iTotal, iAverage);
			}
		}
	}
}

void ProfileReset(void)
{
	for (int iTA = 0; iTA <= _P_Thread_Count; iTA++)
	{
		for (int iPA = 0; iPA < PROFILE_SIZE; iPA++)
		{
			memset(&_P_ARR_Th[iTA]._P_ARR[iPA].lFlag, 0, sizeof(struct PROFILE_INFO));//모든데이터 NULL로 밀어버리기
			memset(&_P_ARR_Th[iTA]._P_ARR[iPA].lStartTime, NULL, sizeof(struct PROFILE_INFO));//모든데이터 NULL로 밀어버리기
			memset(&_P_ARR_Th[iTA]._P_ARR[iPA].iTotalTime, NULL, sizeof(struct PROFILE_INFO));//모든데이터 NULL로 밀어버리기
			memset(&_P_ARR_Th[iTA]._P_ARR[iPA].iMin, NULL, sizeof(struct PROFILE_INFO));//모든데이터 NULL로 밀어버리기
			memset(&_P_ARR_Th[iTA]._P_ARR[iPA].iMax, NULL, sizeof(struct PROFILE_INFO));//모든데이터 NULL로 밀어버리기
			memset(&_P_ARR_Th[iTA]._P_ARR[iPA].iCall, NULL, sizeof(struct PROFILE_INFO));//모든데이터 NULL로 밀어버리기
		}
	}
};

