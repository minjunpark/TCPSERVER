#pragma once

void ProfileBegin(const WCHAR* szName);//�������� ��������
void ProfileEnd(const WCHAR* szName);//�������� ������
void ProfileDataOutText(const WCHAR* szFileName);//���������ؽ�Ʈ ����
void ProfilePrint(void);//�������� ���
void ProfileReset(void);//�������� ������ ����
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


