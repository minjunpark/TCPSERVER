class CRingBuffer
{
private:
	char* _Buffer_Start;//버퍼 시작지점
	char* _Buffer_End;//버퍼의 끝 버퍼의 끝에 도달하면 다시 원래버퍼로 돌아가게 만든다.
	char* _Front;//현재 맨앞
	char* _Rear;//현재 맨뒤

	int _Buffer_Size;//버퍼크기
	int _Use_Size;//사용중인 크기
	int _Non_Use_Size;//사용하지않는크기
	int _Use_Num;

public:
	//생성자
	CRingBuffer(void)
	{
		_Buffer_Start = (char*)malloc(10000);;//버퍼사이즈 할당
		_Buffer_End = _Buffer_Start + 9999;//버퍼의끝
		_Buffer_Size = 10000;//버퍼크기
		_Use_Size = 0;//사용중인버퍼 초기화
		_Non_Use_Size = 10000;//버퍼사용하지 않는크기
		_Front = _Rear = _Buffer_Start;//Front Rear 위치잡기
	};
	CRingBuffer(int iBufferSize)
	{
		_Buffer_Start = (char*)malloc(iBufferSize);//버퍼시작위치 할당
		_Buffer_End = _Buffer_Start + iBufferSize - 1;//버퍼의 끝
		_Buffer_Size = iBufferSize;//버퍼크기
		_Use_Size = 0;//사용중인버퍼킈기 초기화
		_Non_Use_Size = iBufferSize;//사용하지 않는 버퍼크기
		_Front = _Rear = _Buffer_Start;//front위치잡기
	};
	//소멸자
	~CRingBuffer()
	{
		//ClearBuffer();//모든데이터 클리어
		free(_Buffer_Start);//사용버퍼 힙에서 제거
	};


	int GetBufferSize(void)
	{
		return _Buffer_Size;//버퍼최대크기 리턴
	};



	/////////////////////////////////////////////////////////////////////////
	// WritePos 에 데이타 넣음.
	//
	// Parameters: (char *)데이타 포인터. (int)크기. 
	// Return: (int)넣은 크기.
	// 얘를 성공 실패를 리턴해야하나 사이즈를 리턴해야하나
	/////////////////////////////////////////////////////////////////////////
	int	Enqueue(const char* chpData, int iSize)
	{
		int _Eq_size;
		_Eq_size = iSize;

		if (GetFreeSize() < _Eq_size)
		{
			return 0;//입력 받은큐가 쓸수있는 큐보다 크다면 0리턴 해서 더 받을수 없다고 알려주자
		}

		const char* buffer;
		buffer = chpData;
		int Direct_Size = DirectEnqueueSize();

		if (Direct_Size >= _Eq_size)//한번에 넣을수 있는 큐사이즈가 같거나 작다면 넣고 리턴
		{
			memcpy_s(_Rear, _Eq_size, buffer, _Eq_size);
			_Rear = _Rear + _Eq_size;
			_Use_Size = _Use_Size + iSize;//빠진만큼 전체사용크기에서 뺴기
			_Non_Use_Size = _Non_Use_Size - iSize;//빠진만큼 사용크기 설정
			return _Eq_size;
		}

		//아니라면 넣을수 있는 사이즈만큼 넣고
		memcpy_s(_Rear, Direct_Size, buffer, Direct_Size);
		_Rear = _Buffer_Start;//버퍼 처음으로 돌아가서
		memcpy_s(_Rear, _Eq_size - Direct_Size, buffer + Direct_Size, _Eq_size - Direct_Size);//전체사이즈에서 넣은사이즈만큼 뺴고
		_Rear = _Buffer_Start + (_Eq_size - Direct_Size);//시작지점에서 넣은사이즈 만큼 더한다.

		_Use_Size = _Use_Size + iSize;//더한만큼 전체사용크기에서 뺴기
		_Non_Use_Size = _Non_Use_Size - iSize;//빠진만큼 사용크기 설정

		return _Eq_size;
	};

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기.
	/////////////////////////////////////////////////////////////////////////
	int	Dequeue(char* chpDest, int iSize)
	{
		int _Dq_size;
		_Dq_size = iSize;

		if (GetUseSize() < _Dq_size)
		{
			return 0;
		}

		char* buffer;
		buffer = chpDest;
		int Direct_Size = DirectDequeueSize();

		if (Direct_Size >= _Dq_size)//한번에 뺄수있는 크기가 다이렉트보다 작거나 같다면
		{
			memcpy_s(buffer, _Dq_size, _Front, _Dq_size);
			_Front = _Front + _Dq_size;
			_Use_Size = _Use_Size - iSize;//빠진만큼 전체사용크기에서 더하기
			_Non_Use_Size = _Non_Use_Size + iSize;//빠진만큼 사용크기 설정
			return _Dq_size;
		}

		//아니라면 뺼수있는 만큼 뺴고
		memcpy_s(buffer, Direct_Size, _Front, Direct_Size);
		_Front = _Buffer_Start;//버퍼 처음으로 돌아가서
		memcpy_s(buffer + Direct_Size, _Dq_size - Direct_Size, _Front, _Dq_size - Direct_Size);
		_Front = _Front + (_Dq_size - Direct_Size);//시작지점에서 가져온만큼 더한다

		_Use_Size = _Use_Size - iSize;//더한만큼 전체사용크기에서 뺴기
		_Non_Use_Size = _Non_Use_Size + iSize;//빠진만큼 사용크기 설정

		return _Dq_size;
	};


	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기.
	/////////////////////////////////////////////////////////////////////////
	int	Peek(char* chpDest, int iSize)
	{
		int _Dq_size;
		_Dq_size = iSize;

		if (GetUseSize() < _Dq_size)
		{
			return 0;
		}

		char* buffer;
		buffer = chpDest;
		int Direct_Size = DirectDequeueSize();

		if (Direct_Size >= _Dq_size)//한번에 뺼수 있는 큐사이즈가 같거나 작다면 넣고 리턴
		{
			memcpy_s(buffer, _Dq_size, _Front, _Dq_size);
			return _Dq_size;
		}

		//아니라면 뺼수있는 만큼 뺴고
		memcpy_s(buffer, Direct_Size, _Front, Direct_Size);
		memcpy_s(buffer + Direct_Size, _Dq_size - Direct_Size, _Buffer_Start, _Dq_size - Direct_Size);//시작점에서 차이만큼 가져오기

		return _Dq_size;
	};


	//리사이즈 나중에
	void Resize(int size) {

		//if (_Buffer_Size > size)
		//{
		//	return;
		//}

		//char* resize_Buffer = (char*)malloc(size);//리사이즈 하려는 버퍼 크기만큼 늘리고

		//memcpy_s(resize_Buffer, _Buffer_Size, _Buffer_Start, _Buffer_Size);//버퍼를 그대로 복사한다
		//

	};



	/////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 용량.
	/////////////////////////////////////////////////////////////////////////
	int	GetUseSize(void)
	{
		return _Use_Size;//사용중인 크기 리턴
	};

	/////////////////////////////////////////////////////////////////////////
	// 현재 버퍼에 남은 용량 얻기. 
	//
	// Parameters: 없음.
	// Return: (int)남은용량.
	/////////////////////////////////////////////////////////////////////////
	int	GetFreeSize(void)
	{
		return _Non_Use_Size;//사용하지않는 크기 리턴
	};

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
	int	DirectEnqueueSize(void)
	{
		if (_Buffer_End == _Rear)//큐의 끝과 같다면
		{
			return 1;
		}
		return  (_Buffer_End - _Rear) + 1;
	};

	int	DirectDequeueSize(void)
	{
		if (_Buffer_End == _Front)
		{
			return 1;
		}

		return  (_Buffer_End - _Front) + 1;
	};

	/////////////////////////////////////////////////////////////////////////
	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: (int)이동크기
	/////////////////////////////////////////////////////////////////////////
	int	MoveRear(int iSize)
	{
		int _Size;
		_Size = iSize;

		int Eq_size = DirectEnqueueSize();
		int De_size = DirectDequeueSize();

		if (De_size > _Size)//들어온 사이즈가 현재 Enqueue가능 사이즈보다 작으면 그만큼 그냥 이동
		{
			_Rear = _Rear + _Size;//들어온만큼 Rear이동
			_Use_Size = _Use_Size + _Size;//더한만큼 전체사용크기에서 뺴기
			_Non_Use_Size = _Non_Use_Size - _Size;//빠진만큼 사용크기 설정
			return _Size;
		}
		//아니라면
		_Rear = _Buffer_Start;//버퍼의 시작지로 이동
		_Rear = _Rear - (_Size - De_size);//시작지에서 원래 가능 인큐사이즈에서 이동한사이즈만큼 빼서 _Rear에서 이동

		_Use_Size = _Use_Size + (_Size - De_size);//더한만큼 전체사용크기에서 뺴기
		_Non_Use_Size = _Non_Use_Size - (_Size - De_size);//빠진만큼 사용크기 설정

		return _Size;//마지막값 리턴
	};

	int	MoveFront(int iSize)
	{
		int _Size;
		_Size = iSize;

		int Eq_size = DirectEnqueueSize();
		int De_size = DirectDequeueSize();

		if ((Eq_size + De_size) <= _Size)
		{
			return 0;//Front가 Rear보다 앞으로 갈수 없습니다.
		}

		if (De_size > _Size)//들어온 사이즈가 현재 Dequeue가능 사이즈보다 작으면 그만큼 그냥 이동
		{
			_Front = _Front + _Size;//들어온만큼 Rear이동
			_Use_Size = _Use_Size - _Size;//더한만큼 전체사용크기에서 뺴기
			_Non_Use_Size = _Non_Use_Size + _Size;//빠진만큼 사용크기 설정

			return _Size;
		}
		//아니라면
		_Front = _Buffer_Start;//버퍼의 시작지로 이동
		_Front = _Front - (_Size - De_size);//시작지에서 원래 가능 인큐사이즈에서 이동한사이즈만큼 빼서 _Rear에서 이동

		_Use_Size = _Use_Size - (_Size - De_size);//더한만큼 전체사용크기에서 뺴기
		_Non_Use_Size = _Non_Use_Size + (_Size - De_size);//빠진만큼 사용크기 설정

		return _Size;//마지막값 리턴
	};

	void TestFront(int iSize)
	{
		_Front += iSize;
		if (_Front >= _Buffer_End)
		{
			int over = _Front - _Buffer_End;
			_Front = _Buffer_Start + over;
		}
		//_Use_Size = _Use_Size - iSize;//더한만큼 전체사용크기에서 뺴기
		//_Non_Use_Size = _Non_Use_Size + iSize;//빠진만큼 사용크기 설정

	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 모든 데이타 삭제.
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	void	ClearBuffer(void)
	{
		//버퍼 초기화
		_Rear = _Buffer_Start;
		_Front = _Buffer_Start;
	};


	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 Front 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* GetFrontBufferPtr(void)
	{
		return _Front;
	};

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 RearPos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* GetRearBufferPtr(void)
	{
		return _Rear;
	};

	//void _print()//값확인용
	//{
	//	printf("\n");
	//	printf("_Buffer_Start %p\n", _Buffer_Start);
	//	printf("_Buffer_End %p\n", _Buffer_End);
	//	printf("_Front %p\n", _Front);
	//	printf("_Rear %p\n", _Rear);
	//	printf(" Rear-_Front%d\n", _Rear - _Front);
	//	printf("_Use_Size %d\n", _Use_Size);
	//	printf("DirectDequeueSize %d\n", DirectDequeueSize());
	//	printf("DirectEnqueueSize %d\n", DirectEnqueueSize());
	//	printf("_Non_Use_Size %d\n", _Non_Use_Size);
	//	printf("_Buffer_End- _Buffer_End %d\n", _Buffer_End - _Buffer_End);
	//	printf("\n");
	//}

};