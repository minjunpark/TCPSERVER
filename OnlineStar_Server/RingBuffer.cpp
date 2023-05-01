#include "RingBuffer.h"
#include <iostream>

RingBuffer::RingBuffer() :ringBufferSize(10000)
{
	begin = (char*)malloc(ringBufferSize);
	end = begin + ringBufferSize;
	Front = Rear = begin;
}

RingBuffer::RingBuffer(int bufferSize) :ringBufferSize(10000)
{
	begin = (char*)malloc(ringBufferSize);
	end = begin + ringBufferSize;
	Front = Rear = begin;
}

int RingBuffer::GetBufferSize()
{
	return end - begin - 1;
}


int RingBuffer::GetUseSize()
{
	if (Rear >= Front)
		return Rear - Front;
	return (Rear - begin) + (end - Front - 1);
}

int RingBuffer::GetFreeSize()
{
	if (Rear >= Front)
		return (end - Rear) + (Front - begin - 1);
	return Front - Rear - 1;
}

int RingBuffer::DirectEnqeueSize()
{
	if (Rear >= Front)
		return end - Rear - 1;
	return Front - Rear - 1;
}


int RingBuffer::DirectDeqeueSize()
{
	if (Rear >= Front)
		return Rear - Front;
	return end - Front - 1;
}


int RingBuffer::Enqueue(const char* buffer, int size)
{
	if (GetFreeSize() < size)
	{
		return 0;
	}
	if (DirectEnqeueSize() >= size)
	{
		memcpy_s(Rear, size, buffer, size);
		MoveRear(size);
		return size;
	}
	const char* temp = buffer;

	int directEnqueueSize = DirectEnqeueSize();
	memcpy_s(Rear, directEnqueueSize, temp, directEnqueueSize);
	temp += directEnqueueSize;
	MoveRear(directEnqueueSize);

	int remainSize = size - directEnqueueSize;
	memcpy_s(Rear, remainSize, temp, remainSize);
	MoveRear(remainSize);
	return size;
}

int RingBuffer::Dequeue(char* pDest, int size)
{
	if (GetFreeSize() < size)return 0;
	if (DirectDeqeueSize() >= size)
	{
		memcpy_s(pDest, size, Front, size);
		MoveRear(size);
		return size;
	}
	char* pDestTemp = pDest;
	int directDequeueSize = DirectDeqeueSize();

	memcpy_s(pDestTemp, directDequeueSize, Front, directDequeueSize);
	MoveFront(directDequeueSize);
	int remainSize = size - directDequeueSize;
	pDestTemp += directDequeueSize;
	memcpy_s(pDestTemp, remainSize, Front, remainSize);
	MoveFront(remainSize);

	return size;
}

int RingBuffer::Peek(char* pDest, int size)
{
	if (GetFreeSize() < size)return 0;
	if (DirectDeqeueSize() >= size)
	{
		memcpy_s(pDest, size, Front, size);
		return size;
	}
	char* pFrontTemp = Front;
	char* pDestTemp = pDest;
	int directDequeueSize = DirectDeqeueSize();

	memcpy_s(pDestTemp, directDequeueSize, Front, directDequeueSize);
	MoveFront(directDequeueSize);
	int remainSize = size - directDequeueSize;
	pDestTemp += directDequeueSize;
	memcpy_s(pDestTemp, remainSize, Front, remainSize);
	Front = pFrontTemp;

	return size;
}

void RingBuffer::MoveRear(int size)
{
	Rear += size;
	if (Rear >= end - 1)
	{
		int overFlow = Rear - end;
		Rear = begin + overFlow;
	}
}

void RingBuffer::MoveFront(int size)
{
	Front += size;
	if (Front >= end - 1)
	{
		int overFlow = Front - end;
		Front = begin + overFlow;
	}
}


void RingBuffer::ClearBuffeR()
{
	Rear = Front = begin;
};

char* RingBuffer::GetFrontPtr()
{
	return Front;
};
char* RingBuffer::GetRearPtr()
{
	return Rear;
};