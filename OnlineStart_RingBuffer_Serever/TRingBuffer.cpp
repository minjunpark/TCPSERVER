#include "iostream"
#include "TRingbuffer.h"

TRingBuffer::TRingBuffer()//디폴트 버퍼계산 10000바이트를 기본으로한다
{
	_Start = NULL;
	if (NULL != _Start)
		delete[] _Start;
	_BufferSize = 10000;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];
}

TRingBuffer::TRingBuffer(int iBufferSize)//들어온 크기만큼 버퍼를 생성 0이하면 생성을 애초에 안한다.
{
	_Start = NULL;
	if (NULL != _Start)
		delete[] _Start;

	if (iBufferSize >= 0)
	{
		return;
	}
	_BufferSize = iBufferSize;
	_Front = 0;
	_Rear = 0;
	_Start = new char[iBufferSize];
}

TRingBuffer::~TRingBuffer()//버퍼 소멸자
{
	if (NULL != _Start)
		delete[] _Start;
	else {
		_Start = NULL;
	}
	_BufferSize = 0;

	_Front = 0;
	_Rear = 0;
}

//리사이즈 나중에 만들거임
void TRingBuffer::Resize(int size) {

	//if (_Buffer_Size > size)
	//{
	//	return;
	//}

	//char* resize_Buffer = (char*)malloc(size);//리사이즈 하려는 버퍼 크기만큼 늘리고

	//memcpy_s(resize_Buffer, _Buffer_Size, _Buffer_Start, _Buffer_Size);//버퍼를 그대로 복사한다
	//

};

/////////////////////////////////////////////////////////////////////////
// 현재 최대의 크기를 얻기.
//
// Parameters: 없음.
// Return: (int)최대의 크기 값.
/////////////////////////////////////////////////////////////////////////

int	TRingBuffer::GetBufferSize(void)
{
	if (NULL != _Start)
	{
		return _BufferSize - BLANK_SIZE;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////
// 현재 사용중인 용량 얻기.
//
// Parameters: 없음.
// Return: (int)사용중인 용량.
/////////////////////////////////////////////////////////////////////////

int TRingBuffer::GetUseSize(void)
{
	if (_Front <= _Rear)
	{
		return _Rear - _Front;
	}
	else
	{
		return _BufferSize - _Front + _Rear;
	}

}
/////////////////////////////////////////////////////////////////////////
// 현재 버퍼에 남은 용량 얻기. 
//
// Parameters: 없음.
// Return: (int)남은용량.
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::GetFreeSize(void)
{
	return _BufferSize - (GetUseSize() + BLANK_SIZE);
}

/////////////////////////////////////////////////////////////////////////
// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
// (끊기지 않은 길이)
//
// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
//
// Parameters: 없음.
// Return: (int)사용가능 용량.
////////////////////////////////////////////////////////////////////////
int TRingBuffer::DirectDequeueSize(void)
{
	if (_Front <= _Rear)
	{
		return _Rear - _Front;
	}
	else
	{
		return _BufferSize - _Front;
	}
}

int TRingBuffer::DirectEnqueueSize(void)
{
	if (_Rear < _Front)
	{
		return (_Front - _Rear) - BLANK_SIZE;
	}
	else
	{
		if (_Front < BLANK_SIZE)
		{
			return (_BufferSize - _Rear) - (BLANK_SIZE - _Front);
		}
		else
		{
			return _BufferSize - _Rear;
		}
	}
}
/////////////////////////////////////////////////////////////////////////
// _Rear 에 데이타 넣고 _Rear을 그만큼 이동시킨다.
//
// Parameters: (char *)데이타 포인터. (int)크기. 
// Return: (int)넣은 크기.
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::Enqueue(char* chpData, int iSize)
{
	int iWrite;

	if (GetFreeSize() < iSize)
	{
		return 0;
		//	iSize = GetFreeSize();
	}

	if (0 >= iSize)
		return 0;

	if (_Front <= _Rear)
	{
		iWrite = _BufferSize - _Rear;

		if (iWrite >= iSize)
		{
			memcpy(_Start + _Rear, chpData, iSize);
			_Rear += iSize;
		}
		else
		{
			memcpy(_Start + _Rear, chpData, iWrite);
			memcpy(_Start, chpData + iWrite, iSize - iWrite);
			_Rear = iSize - iWrite;
		}
	}
	else
	{
		memcpy(_Start + _Rear, chpData, iSize);
		_Rear += iSize;
	}

	_Rear = _Rear == _BufferSize ? 0 : _Rear;

	return iSize;
}
/////////////////////////////////////////////////////////////////////////
// _Front 에서 데이타 가져옴. _Front 이동.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::Dequeue(char* chpDest, int iSize)
{
	int iRead;

	if (GetUseSize() < iSize)
		iSize = GetUseSize();

	if (0 >= iSize)
		return 0;

	if (_Front <= _Rear)
	{
		memcpy(chpDest, _Start + _Front, iSize);
		_Front += iSize;
	}
	else
	{
		iRead = _BufferSize - _Front;

		if (iRead >= iSize)
		{
			memcpy(chpDest, _Start + _Front, iSize);
			_Front += iSize;
		}
		else
		{
			memcpy(chpDest, _Start + _Front, iRead);
			memcpy(chpDest + iRead, _Start, iSize - iRead);
			_Front = iSize - iRead;
		}
	}

	return iSize;
}
/////////////////////////////////////////////////////////////////////////
// _Front 에서 데이타 읽어옴. _Front는 이동시키지 않는다.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int	TRingBuffer::Peek(char* chpDest, int iSize)
{
	int iRead;
	if (GetUseSize() < iSize)
		iSize = GetUseSize();

	if (0 >= iSize)
		return 0;

	if (_Front <= _Rear)
	{
		memcpy(chpDest, _Start + _Front, iSize);
	}
	else
	{
		iRead = _BufferSize - _Front;
		if (iRead >= iSize)
		{
			memcpy(chpDest, _Start + _Front, iSize);
		}
		else
		{
			memcpy(chpDest, _Start + _Front, iRead);
			memcpy(chpDest + iRead, _Start, iSize - iRead);
		}
	}

	return iSize;

}
/////////////////////////////////////////////////////////////////////////
// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
//
// Parameters: 없음.
// Return: (int)이동크기
/////////////////////////////////////////////////////////////////////////
void TRingBuffer::MoveRear(int iSize)
{
	_Rear = (_Rear + iSize) % _BufferSize;
}

void TRingBuffer::MoveFront(int iSize)
{
	_Front = (_Front + iSize) % _BufferSize;
}

/////////////////////////////////////////////////////////////////////////
// 모든 데이터값 초기화시키기
//
// Parameters: 없음.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////

void TRingBuffer::ClearBuffer(void)
{
	_Front = 0;
	_Rear = 0;
}


/////////////////////////////////////////////////////////////////////////
// 버퍼의 Front 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* TRingBuffer::GetFrontBufferPtr(void)
{
	return _Start + _Front;
}

/////////////////////////////////////////////////////////////////////////
// 버퍼의 RearPos 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* TRingBuffer::GetRearBufferPtr(void)
{
	return _Start + _Rear;
}