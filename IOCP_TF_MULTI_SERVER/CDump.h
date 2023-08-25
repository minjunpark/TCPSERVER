#pragma once
#include <Windows.h>
#include <stdio.h>
#include <psapi.h>
#include <DbgHelp.h>
#include <crtdbg.h>

class CDump 
{
public:
	CDump()
	{
		_DumpCount = 0;
		_invalid_parameter_handler OldHandler;
		_invalid_parameter_handler Newhandler;
		Newhandler = myInvalidParmeterhandler;

		OldHandler = _set_invalid_parameter_handler(Newhandler);//crt�Լ��� null������ ���� �־�����
		_CrtSetReportMode(_CRT_WARN, 0);//CRT �����޼��� ǥ�� �ߴ�, �ٷ� ������ ������,
		_CrtSetReportMode(_CRT_ASSERT, 0);//CRT �����޼��� ǥ�� �ߴ�, �ٷ� ������ ������,
		_CrtSetReportMode(_CRT_ERROR, 0);//CRT �����޼��� ǥ�� �ߴ�, �ٷ� ������ ������,

		_CrtSetReportHook(_custom_Report_hook);

		//---------------------------------------------------------------------------------------
		// pure virtual function called ���� �ڵ鷯�� ����� ���� �Լ��� ��ȸ��Ų��.
		//---------------------------------------------------------------------------------------
		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();
	}

	static void Crash(void)
	{
		int* p = nullptr;
		*p = 0;//�Ϻη��ѰŴϱ� �ư���
	}
	
	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		int iWorkingMemory = 0;
		SYSTEMTIME stNowTime;

		long DumpCount = InterlockedIncrement(&_DumpCount);

		//---------------------------------------------------
		// ���� ���μ����� �޸� ��뷮�� ���´�.
		//---------------------------------------------------
		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = GetCurrentProcess();

		if (NULL == hProcess)
			return 0;
		
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			iWorkingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProcess);

		//---------------------------------------------------
		// ���� ��¥�� �ð��� �˾ƿ´�.
		//---------------------------------------------------
		WCHAR filename[MAX_PATH];

		GetLocalTime(&stNowTime);
		wsprintf(filename, 
			L"Dump_%d%02d%02d_%02d.%02d_%d_%dMB.dmp",
			stNowTime.wYear, 
			stNowTime.wMonth, 
			stNowTime.wDay, 
			stNowTime.wHour, 
			stNowTime.wMinute,
			stNowTime.wSecond, 
			DumpCount, 
			iWorkingMemory
		);

		wprintf(L"\n\n\n!!! CrashError!!! %d.%d.%d / %d:%d:%d\n",
			stNowTime.wYear,
			stNowTime.wMonth,
			stNowTime.wDay,
			stNowTime.wHour,
			stNowTime.wMinute,
			stNowTime.wSecond
		);

		wprintf(L"Now Save dump file...\n");

		HANDLE hDumpFile = ::CreateFile(filename,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;

			MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&MinidumpExceptionInformation,
				NULL,
				NULL
			);

			CloseHandle(hDumpFile);

			wprintf(L"CrashDump Save Finish!");
		}
		return EXCEPTION_EXECUTE_HANDLER;

	}
	
	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	//Invalude Parameter Handler,
	static void myInvalidParmeterhandler(const wchar_t*expression,const wchar_t*function,const wchar_t*file,unsigned int line,uintptr_t pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype,char*message,int*returnvalue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}

	static long _DumpCount;
};