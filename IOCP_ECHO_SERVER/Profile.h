#pragma once

void ProfileBegin(const WCHAR* szName);//프로파일 시작지점
void ProfileEnd(const WCHAR* szName);//프로파일 끝지점
void ProfileDataOutText(const WCHAR* szFileName);//프로파일텍스트 저장
void ProfilePrint(void);//프로파일 출력
void ProfileReset(void);//프로파일 데이터 제거
void InitProfile(void);

extern LARGE_INTEGER _P_Freq;

#define PROFILE
#ifdef PROFILE
#define PRO_BEGIN(TagName)	ProfileBegin(TagName)
#define PRO_END(TagName)	ProfileEnd(TagName)
#elif
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif


