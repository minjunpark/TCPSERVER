#pragma once
class RingBuffer
{
public:
	RingBuffer();
	RingBuffer(int bufferSize);

	bool ReSize(int size);
	int GetBufferSize();

	int GetUseSize();
	int GetFreeSize();

	int DirectEnqeueSize();
	int DirectDeqeueSize();

	int Enqueue(const char* buffer, int size);
	int Dequeue(char* pDest, int size);
	int Peek(char* pDest, int size);

	void MoveRear(int size);
	void MoveFront(int size);

	void ClearBuffeR();

	char* GetFrontPtr();
	char* GetRearPtr();

private:
	char* begin;
	char* end;
	char* Front;
	char* Rear;

	int ringBufferSize;
};