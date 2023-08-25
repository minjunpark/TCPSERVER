//#pragma once
//#include <Windows.h>
//
//#ifdef PROFILE
//
//#define BEGIN_PROFILE(arg) BeginProfiling(arg)
//#define END_PROFILE(arg) EndProfiling(arg)
//
//#else
//#define BEGIN_PROFILE(arg)
//#define END_PROFILE(arg)
//
//#endif 
//
//	void BeginProfiling(const char* name);
//	void EndProfiling(const char* name);
//	void SaveProfiling();
//	void ResetProfiling();
//
//	class Profiler
//	{
//	public:
//		Profiler(const char* name) : name(name)
//		{
//			BeginProfiling(name);
//		}
//		~Profiler()
//		{
//			EndProfiling(name);
//		}
//	private:
//		const char* name;
//	};
//	constexpr int SAMPLE_SIZE = 20;
//	struct PROFILE_SAMPLE
//	{
//		bool flag;
//		char profileName[64];
//		LARGE_INTEGER startTime;
//		__int64 totalTime;
//		__int64 min[2];
//		__int64 max[2];
//		__int64 callCounts;
//	};
//
//
//	extern PROFILE_SAMPLE samples[SAMPLE_SIZE];