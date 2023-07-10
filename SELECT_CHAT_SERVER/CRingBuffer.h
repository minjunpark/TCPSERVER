#pragma once

class CRingBuffer
{
private:
	char* _Start;//버퍼 시작지점
	int _BufferSize;//총 버퍼 크기
	int _Front;//읽기 포인터 계산을 위한값
	int _Rear;//쓰기 포인터 계산을 위한값

public:
	CRingBuffer(void);//사이즈가 없을경우의 생성자
	CRingBuffer(int iBufferSize);//사이즈가 있을경우의 생성자
	~CRingBuffer(void);//소멸자

	//크기함수
	int GetBufferSize(void);//현재 버퍼의 최대 크기를 가져온다.
	int GetUseSize(void);//현재 사용중인 용량 얻기
	int GetFreeSize(void);//현재 버퍼에 남은 용량 얻기
	int DirectEnqueueSize(void);//버퍼에 한번에 넣을수 있는 사이즈
	int DirectDequeueSize(void);//버퍼에 한번에 뺄수 있는 사이즈
	
	//실제작동부
	int Enqueue(char* chpData, int iSize);//사이즈만큼 버퍼에 넣기
	int Dequeue(char* chpDest, int iSize);//사이즈만큼 버퍼에 빼기
	int Peek(char* chpDest, int iSize);//사이즈만큼 버퍼에 복사하기

	//쓰기 읽기포인터 이동시키기
	int MoveRear(int iSize);//쓰기 포인터를 크기만큼 이동시킨다.
	int MoveFront(int iSize);//읽기 포인터를 크기만큼 이동시킨다.
	char* GetFrontBufferPtr(void);//읽기포인터를 리턴
	char* GetRearBufferPtr(void);//쓰기포인터를 리턴
	
	void ClearBuffer(void);//버퍼를 초기화하기
	bool Resize(int size);//버퍼의 사이즈를 바꾸는 함수(나중에 고려해볼예정)

private:
	enum { VOID_VALUE = 8 };


};