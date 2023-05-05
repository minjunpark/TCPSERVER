#pragma once
#include <iostream>

class MRingBuffer
{
private:
	char* _Start;//시작 포인터
	char* _End;//끝 포인터
	char* _Rear;//쓰기포인터
	char* _Front;//읽기포인터

	int _RingBufferSize;

public:
	MRingBuffer(void)
	{
		_RingBufferSize = 10000;
		_Start = (char*)malloc(_RingBufferSize);//기본크기 10000
		//_Start = new char[_RingBufferSize];//기본크기 10000
		_End = _Start + _RingBufferSize;
		_Front = _Start;
		_Rear = _Start;
	};
	MRingBuffer(int iBufferSize)
	{
		_RingBufferSize = iBufferSize;
		_Start = (char*)malloc(_RingBufferSize);//기본크기
		//_Start = new char[_RingBufferSize];
		_End = _Start + _RingBufferSize;
		_Front = _Start;
		_Rear = _Start;
	};
	//소멸자
	~MRingBuffer()
	{
		printf("\n");
		printf("_Start %d\n",_Start);
		printf("_End %d\n", _End);
		printf("_Rear %d\n", _Rear);
		printf("_Front %d\n", _Front);
		printf("_RingBufferSize %d\n", _RingBufferSize);

		//free(_Start);
		//delete[] _Start;
		//RingBufferFree();//사용버퍼 힙에서 제거
	};

	//버퍼사이즈크기
	int GetBufferSize()
	{
		return _End - _Start - 1;
	}

	/////////////////////////////////////////////////////////////////////////
	// WritePos 에 데이타 넣음.
	//
	// Parameters: (char *)데이타 포인터. (int)크기. 
	// Return: (int)넣은 크기.
	// 얘를 성공 실패를 리턴해야하나 사이즈를 리턴해야하나
	/////////////////////////////////////////////////////////////////////////
	int Enqueue(const char* chpData, int size)
	{
		if (GetFreeSize() < size || size < 0)
		{
			return 0;
		}

		int _DE_Size = DirectEnqueueSize();//함수호출 최소화
		const char* temp = chpData;

		if (_DE_Size >= size)
		{
			memcpy_s(_Rear, size, chpData, size);
			//memcpy(_Rear, buffer, size);//속도 차이날수 있어서 memcpy쓰는게 낫다고하는데 테스트나 정확한 자료가 필요할듯
			MoveRear(size);
			return size;
		}


		memcpy_s(_Rear, _DE_Size, temp, _DE_Size);
		//memcpy(_Rear, temp, directEnqueueSize);
		temp += _DE_Size;
		MoveRear(_DE_Size);

		int remainSize = size - _DE_Size;
		memcpy_s(_Rear, remainSize, temp, remainSize);
		//memcpy(_Rear, temp, remainSize);
		MoveRear(remainSize);
		return size;
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기.
	/////////////////////////////////////////////////////////////////////////
	int Dequeue(char* chpData, int size)
	{
		if (GetUseSize() < size || size < 0)
		{
			return 0;
		}

		int _DD_Size = DirectDequeueSize();
		char* pDestTemp = chpData;

		if (_DD_Size >= size)
		{
			memcpy_s(pDestTemp, size, _Front, size);
			MoveFront(size);
			return size;
		}

		memcpy_s(pDestTemp, _DD_Size, _Front, _DD_Size);
		MoveFront(_DD_Size);
		int remainSize = size - _DD_Size;
		pDestTemp += _DD_Size;
		memcpy_s(pDestTemp, remainSize, _Front, remainSize);
		MoveFront(remainSize);

		return size;
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기.
	/////////////////////////////////////////////////////////////////////////
	int Peek(char* chpData, int size)
	{
		if (GetUseSize() < size || size < 0)
		{
			return 0;
		}

		int _DD_Size = DirectDequeueSize();

		if (_DD_Size >= size)
		{
			memcpy_s(chpData, size, _Front, size);
			return size;
		}
		char* pFrontTemp = _Front;
		char* pDestTemp = chpData;

		memcpy_s(pDestTemp, _DD_Size, _Front, _DD_Size);
		MoveFront(_DD_Size);
		int remainSize = size - _DD_Size;
		pDestTemp += _DD_Size;
		memcpy_s(pDestTemp, remainSize, _Front, remainSize);
		_Front = pFrontTemp;

		return size;
	}

	//리사이즈 나중에
	void Resize(int size) {

		//if (_Buffer_Size > size)
		//{
		//	return;
		//}

		//char* resize_Buffer = (char*)malloc(size);//리사이즈 하려는 버퍼 크기만큼 늘리고

		//memcpy_s(resize_Buffer, _Buffer_Size, _Buffer_Start, _Buffer_Size);//버퍼를 그대로 복사한다
		//
		return;
	};

	/////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 용량.
	/////////////////////////////////////////////////////////////////////////
	int GetUseSize()
	{
		if (_Rear >= _Front)
		{
			return _Rear - _Front;
		}
		return (_Rear - _Start) + (_End - _Front - 1);
	}

	/////////////////////////////////////////////////////////////////////////
	// 현재 버퍼에 남은 용량 얻기. 
	//
	// Parameters: 없음.
	// Return: (int)남은용량.
	/////////////////////////////////////////////////////////////////////////

	int GetFreeSize()
	{
		if (_Rear >= _Front)
		{
			return (_End - _Rear) + (_Front - _Start - 1);
		}
		return _Front - _Rear - 1;
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
	int DirectEnqueueSize()
	{
		if (_Rear >= _Front)
		{
			return _End - _Rear - 1;
		}
		return _Front - _Rear - 1;
	}


	int DirectDequeueSize()
	{
		if (_Rear >= _Front)
		{
			return _Rear - _Front;
		}
		return _End - _Front - 1;
	}


	void MoveRear(int size)
	{
		_Rear += size;
		if (_Rear >= _End - 1)
		{
			int over = _Rear - _End;
			_Rear = _Start + over;
		}
	}

	void MoveFront(int size)
	{
		_Front += size;//프론트를 를 사이즈만큼 이동시키고
		if (_Front >= _End - 1)//만약 끝지점을 넘어선다면
		{
			int over = _Front - _End;//넘어서는값만큼 계산후
			_Front = _Start + over;//시작부터 프론트에 더하고 이동시킨다.
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 모든 데이타 삭제.
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	void ClearBuffeR()
	{
		//Rear과 Start를 같은 위치로 이동시켜 초기화 시킨다.
		_Rear = _Start;
		_Front = _Start;
	};

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 Front 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////

	char* GetFrontBufferPtr()
	{
		return _Front;//읽기포인터 가져오기
	};

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 RearPos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* GetRearBufferPtr()
	{
		return _Rear;//쓰기포인터 가져오기
	};


	//void _print()//값확인용
	//{
	//	printf("\n");
	//	printf("_Buffer_Start %p\n", _Buffer_Start);
	//	printf("_Buffer_End %p\n", _Buffer_End);
	//	printf("_Buffer_Start %d\n", _Buffer_Start);
	//	printf("_Buffer_End %d\n", _Buffer_End);
	//	printf("_Front %p\n", _Front);
	//	printf("_Rear %p\n", _Rear);
	//	printf("_Front %d\n", _Front);
	//	printf("_Rear %d\n", _Rear);
	//	printf(" Rear-_Front%d\n", _Rear - _Front);
	//	printf("_Use_Size %d\n", _Use_Size);
	//	printf("DirectDequeueSize %d\n", DirectDequeueSize());
	//	printf("DirectEnqueueSize %d\n", DirectEnqueueSize());
	//	printf("_Non_Use_Size %d\n", _Non_Use_Size);
	//	printf("_Buffer_End- _Front %d\n", _Buffer_End - _Front);
	//	printf("_Buffer_End- _Rear %d\n", _Buffer_End - _Rear);
	//	printf("\n");
	//}

};