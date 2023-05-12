#include "iostream"
#include "TRingbuffer.h"

TRingBuffer::TRingBuffer()//디폴트 버퍼계산 10000바이트를 기본으로한다
{
	_Start = nullptr;
	if (nullptr != _Start)
		delete[] _Start;
	_BufferSize = 10000;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];
}

TRingBuffer::TRingBuffer(int iBufferSize)//들어온 크기만큼 버퍼를 생성 0이하면 생성을 애초에 안한다.
{
	_Start = nullptr;
	if (nullptr != _Start)
		delete[] _Start;

	if (iBufferSize <= 0)
	{
		return;
	}

	_BufferSize = iBufferSize;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];
}

TRingBuffer::~TRingBuffer()//버퍼 소멸자
{
	if (nullptr != _Start)
		delete[] _Start;
	else {
		_Start = nullptr;
	}
	_BufferSize = 0;

	_Front = 0;
	_Rear = 0;
}


void TRingBuffer::Resize(int size)
{
	//char* _Old_Start = _Start;//버퍼 시작지점
	//int _Old_BufferSize = _BufferSize;//총 버퍼 크기
	//int _Old_Front = _Front;//읽기 포인터 계산을 위한값
	//int _Old_Rear = _Rear;//쓰기 포인터 계산을 위한값

	//char* _tmp_buffer = new char[size];//이동할 만큼의 크기의 버퍼를 미리만든다.
	//
	//int retresize = Peek(_tmp_buffer, GetUseSize());//모든 데이터를 복사해온다.

	//if (retresize != GetUseSize())//꺼내온 크기가 작다면
	//{
	//	delete[] _tmp_buffer;//실패했다면 원래버퍼를 뜯어온다.
	//}
	//else
	//{
	//	//성공했다면
	//	delete[] _Start;//과거 버퍼를 힙에서 제거하고
	//	_Start = _tmp_buffer;//새로운 버퍼를 세팅한다.
	//	_BufferSize = size;
	//	_Front = 0;
	//	_Rear = 0;
	//}
};

/////////////////////////////////////////////////////////////////////////
// 현재 최대의 크기를 얻기.
//
// Parameters: 없음.
// Return: (int)최대의 크기 값.
/////////////////////////////////////////////////////////////////////////

int	TRingBuffer::GetBufferSize(void)
{
	if (_Start != nullptr)
	{
		return _BufferSize - VOID_VALUE;
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
	return _BufferSize - (GetUseSize() + VOID_VALUE);
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
		return (_Front - _Rear) - VOID_VALUE;
	}
	else
	{
		if (_Front < VOID_VALUE)
		{
			return (_BufferSize - _Rear) - (VOID_VALUE - _Front);
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
	int iRear;

	if (GetFreeSize() < iSize)
	{
		return 0;
	}

	if (iSize <= 0)//논블로킹 소켓인경우 -1이 오는 경우가 있기 때문에 미리 대비
	{
		return 0;
	}

	if (_Front <= _Rear)//크기가 front 보다 작다면
	{
		iRear = _BufferSize - _Rear;

		if (iRear >= iSize)
		{
			memcpy(_Start + _Rear, chpData, iSize);
			//memcpy_s(_Start + _Rear, iSize, chpData, iSize);
			_Rear += iSize;
		}
		else
		{
			memcpy(_Start + _Rear, chpData, iRear);
			memcpy(_Start, chpData + iRear, iSize - iRear);
			//memcpy_s(_Start + _Rear, iSize, chpData, iRear);
			//memcpy_s(_Start , iSize - iRear, chpData + iRear, iSize - iRear);
			_Rear = iSize - iRear;
		}
	}
	else
	{
		memcpy(_Start + _Rear, chpData, iSize);
		_Rear += iSize;
	}

	//if (_Rear == _BufferSize)
	//{
	//	_Rear = 0;
	//}
	//else 
	//{
	//	_Rear = _Rear;
	//}
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
	int iFront;

	if (GetUseSize() < iSize)//요청한 사이즈가 꺼내올수 있는 크기보다 작다면
	{
		//return 0;//요청한 크기가 현재 있는 사이즈보다 크다면 그냥 0을 리턴하고 종료한다.
		iSize = GetUseSize();//꺼낼수 있는 크기만큼 꺼내주기
	}

	if (iSize <= 0)//논블로킹 소켓인경우 -1이 오는 경우가 있기 때문에 미리 대비
	{
		return 0;
	}

	if (_Front <= _Rear)
	{
		memcpy(chpDest, _Start + _Front, iSize);
		_Front += iSize;
	}
	else
	{
		iFront = _BufferSize - _Front;

		if (iFront >= iSize)
		{
			memcpy(chpDest, _Start + _Front, iSize);
			_Front += iSize;
		}
		else
		{
			memcpy(chpDest, _Start + _Front, iFront);
			memcpy(chpDest + iFront, _Start, iSize - iFront);
			_Front = iSize - iFront;
		}
	}

	//_Front = _Front == _BufferSize ? 0 : _Front;

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
	int iFront;
	if (GetUseSize() < iSize)
	{
		//return 0;//요청한 크기가 현재 있는 사이즈보다 크다면 그냥 0을 리턴하고 종료한다.
		iSize = GetUseSize();//꺼낼수 있는 크기만큼 꺼내주기
	}

	if (iSize <= 0)//논블로킹 소켓인경우 -1이 오는 경우가 있기 때문에 미리 대비
	{
		return 0;
	}

	if (_Front <= _Rear)
	{
		memcpy(chpDest, _Start + _Front, iSize);
	}
	else
	{
		iFront = _BufferSize - _Front;
		if (iFront >= iSize)
		{
			memcpy(chpDest, _Start + _Front, iSize);
		}
		else
		{
			memcpy(chpDest, _Start + _Front, iFront);
			memcpy(chpDest + iFront, _Start, iSize - iFront);
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
int TRingBuffer::MoveRear(int iSize)
{
	_Rear = (_Rear + iSize) % _BufferSize;
	return iSize;//이동된 _Rear포인터 위치를 리턴한다.
}

int TRingBuffer::MoveFront(int iSize)
{
	_Front = (_Front + iSize) % _BufferSize;//이동한 크기만큼 넣고
	return iSize;//이동한 _Front포인터 위치를 리턴한다.
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