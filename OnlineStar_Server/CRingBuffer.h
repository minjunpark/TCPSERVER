class CRingBuffer
{
private:
	char* _Buffer_Start;//���� ��������
	char* _Buffer_End;//������ �� ������ ���� �����ϸ� �ٽ� �������۷� ���ư��� �����.
	char* _Front;//���� �Ǿ�
	char* _Rear;//���� �ǵ�

	int _Buffer_Size;//����ũ��
	int _Use_Size;//������� ũ��
	int _Non_Use_Size;//��������ʴ�ũ��
	int _Use_Num;

public:
	//������
	CRingBuffer(void)
	{
		_Buffer_Start = (char*)malloc(10000);;//���ۻ����� �Ҵ�
		_Buffer_End = _Buffer_Start + 9999;//�����ǳ�
		_Buffer_Size = 10000;//����ũ��
		_Use_Size = 0;//������ι��� �ʱ�ȭ
		_Non_Use_Size = 10000;//���ۻ������ �ʴ�ũ��
		_Front = _Rear = _Buffer_Start;//Front Rear ��ġ���
	};
	CRingBuffer(int iBufferSize)
	{
		_Buffer_Start = (char*)malloc(iBufferSize);//���۽�����ġ �Ҵ�
		_Buffer_End = _Buffer_Start + iBufferSize - 1;//������ ��
		_Buffer_Size = iBufferSize;//����ũ��
		_Use_Size = 0;//������ι��۴��� �ʱ�ȭ
		_Non_Use_Size = iBufferSize;//������� �ʴ� ����ũ��
		_Front = _Rear = _Buffer_Start;//front��ġ���
	};
	//�Ҹ���
	~CRingBuffer()
	{
		//ClearBuffer();//��絥���� Ŭ����
		free(_Buffer_Start);//������ ������ ����
	};


	int GetBufferSize(void)
	{
		return _Buffer_Size;//�����ִ�ũ�� ����
	};



	/////////////////////////////////////////////////////////////////////////
	// WritePos �� ����Ÿ ����.
	//
	// Parameters: (char *)����Ÿ ������. (int)ũ��. 
	// Return: (int)���� ũ��.
	// �긦 ���� ���и� �����ؾ��ϳ� ����� �����ؾ��ϳ�
	/////////////////////////////////////////////////////////////////////////
	int	Enqueue(const char* chpData, int iSize)
	{
		int _Eq_size;
		_Eq_size = iSize;

		if (GetFreeSize() < _Eq_size)
		{
			return 0;//�Է� ����ť�� �����ִ� ť���� ũ�ٸ� 0���� �ؼ� �� ������ ���ٰ� �˷�����
		}

		const char* buffer;
		buffer = chpData;
		int Direct_Size = DirectEnqueueSize();

		if (Direct_Size >= _Eq_size)//�ѹ��� ������ �ִ� ť����� ���ų� �۴ٸ� �ְ� ����
		{
			memcpy_s(_Rear, _Eq_size, buffer, _Eq_size);
			_Rear = _Rear + _Eq_size;
			_Use_Size = _Use_Size + iSize;//������ŭ ��ü���ũ�⿡�� ����
			_Non_Use_Size = _Non_Use_Size - iSize;//������ŭ ���ũ�� ����
			return _Eq_size;
		}

		//�ƴ϶�� ������ �ִ� �����ŭ �ְ�
		memcpy_s(_Rear, Direct_Size, buffer, Direct_Size);
		_Rear = _Buffer_Start;//���� ó������ ���ư���
		memcpy_s(_Rear, _Eq_size - Direct_Size, buffer + Direct_Size, _Eq_size - Direct_Size);//��ü������� ���������ŭ ����
		_Rear = _Buffer_Start + (_Eq_size - Direct_Size);//������������ ���������� ��ŭ ���Ѵ�.

		_Use_Size = _Use_Size + iSize;//���Ѹ�ŭ ��ü���ũ�⿡�� ����
		_Non_Use_Size = _Non_Use_Size - iSize;//������ŭ ���ũ�� ����

		return _Eq_size;
	};

	/////////////////////////////////////////////////////////////////////////
	// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
	//
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)������ ũ��.
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

		if (Direct_Size >= _Dq_size)//�ѹ��� �����ִ� ũ�Ⱑ ���̷�Ʈ���� �۰ų� ���ٸ�
		{
			memcpy_s(buffer, _Dq_size, _Front, _Dq_size);
			_Front = _Front + _Dq_size;
			_Use_Size = _Use_Size - iSize;//������ŭ ��ü���ũ�⿡�� ���ϱ�
			_Non_Use_Size = _Non_Use_Size + iSize;//������ŭ ���ũ�� ����
			return _Dq_size;
		}

		//�ƴ϶�� �E���ִ� ��ŭ ����
		memcpy_s(buffer, Direct_Size, _Front, Direct_Size);
		_Front = _Buffer_Start;//���� ó������ ���ư���
		memcpy_s(buffer + Direct_Size, _Dq_size - Direct_Size, _Front, _Dq_size - Direct_Size);
		_Front = _Front + (_Dq_size - Direct_Size);//������������ �����¸�ŭ ���Ѵ�

		_Use_Size = _Use_Size - iSize;//���Ѹ�ŭ ��ü���ũ�⿡�� ����
		_Non_Use_Size = _Non_Use_Size + iSize;//������ŭ ���ũ�� ����

		return _Dq_size;
	};


	/////////////////////////////////////////////////////////////////////////
	// ReadPos ���� ����Ÿ �о��. ReadPos ����.
	//
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)������ ũ��.
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

		if (Direct_Size >= _Dq_size)//�ѹ��� �E�� �ִ� ť����� ���ų� �۴ٸ� �ְ� ����
		{
			memcpy_s(buffer, _Dq_size, _Front, _Dq_size);
			return _Dq_size;
		}

		//�ƴ϶�� �E���ִ� ��ŭ ����
		memcpy_s(buffer, Direct_Size, _Front, Direct_Size);
		memcpy_s(buffer + Direct_Size, _Dq_size - Direct_Size, _Buffer_Start, _Dq_size - Direct_Size);//���������� ���̸�ŭ ��������

		return _Dq_size;
	};


	//�������� ���߿�
	void Resize(int size) {

		//if (_Buffer_Size > size)
		//{
		//	return;
		//}

		//char* resize_Buffer = (char*)malloc(size);//�������� �Ϸ��� ���� ũ�⸸ŭ �ø���

		//memcpy_s(resize_Buffer, _Buffer_Size, _Buffer_Start, _Buffer_Size);//���۸� �״�� �����Ѵ�
		//

	};



	/////////////////////////////////////////////////////////////////////////
	// ���� ������� �뷮 ���.
	//
	// Parameters: ����.
	// Return: (int)������� �뷮.
	/////////////////////////////////////////////////////////////////////////
	int	GetUseSize(void)
	{
		return _Use_Size;//������� ũ�� ����
	};

	/////////////////////////////////////////////////////////////////////////
	// ���� ���ۿ� ���� �뷮 ���. 
	//
	// Parameters: ����.
	// Return: (int)�����뷮.
	/////////////////////////////////////////////////////////////////////////
	int	GetFreeSize(void)
	{
		return _Non_Use_Size;//��������ʴ� ũ�� ����
	};

	/////////////////////////////////////////////////////////////////////////
	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// (������ ���� ����)
	//
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	//
	// Parameters: ����.
	// Return: (int)��밡�� �뷮.
	////////////////////////////////////////////////////////////////////////
	int	DirectEnqueueSize(void)
	{
		if (_Buffer_End == _Rear)//ť�� ���� ���ٸ�
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
	// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
	//
	// Parameters: ����.
	// Return: (int)�̵�ũ��
	/////////////////////////////////////////////////////////////////////////
	int	MoveRear(int iSize)
	{
		int _Size;
		_Size = iSize;

		int Eq_size = DirectEnqueueSize();
		int De_size = DirectDequeueSize();

		if (De_size > _Size)//���� ����� ���� Enqueue���� ������� ������ �׸�ŭ �׳� �̵�
		{
			_Rear = _Rear + _Size;//���¸�ŭ Rear�̵�
			_Use_Size = _Use_Size + _Size;//���Ѹ�ŭ ��ü���ũ�⿡�� ����
			_Non_Use_Size = _Non_Use_Size - _Size;//������ŭ ���ũ�� ����
			return _Size;
		}
		//�ƴ϶��
		_Rear = _Buffer_Start;//������ �������� �̵�
		_Rear = _Rear - (_Size - De_size);//���������� ���� ���� ��ť������� �̵��ѻ����ŭ ���� _Rear���� �̵�

		_Use_Size = _Use_Size + (_Size - De_size);//���Ѹ�ŭ ��ü���ũ�⿡�� ����
		_Non_Use_Size = _Non_Use_Size - (_Size - De_size);//������ŭ ���ũ�� ����

		return _Size;//�������� ����
	};

	int	MoveFront(int iSize)
	{
		int _Size;
		_Size = iSize;

		int Eq_size = DirectEnqueueSize();
		int De_size = DirectDequeueSize();

		if ((Eq_size + De_size) <= _Size)
		{
			return 0;//Front�� Rear���� ������ ���� �����ϴ�.
		}

		if (De_size > _Size)//���� ����� ���� Dequeue���� ������� ������ �׸�ŭ �׳� �̵�
		{
			_Front = _Front + _Size;//���¸�ŭ Rear�̵�
			_Use_Size = _Use_Size - _Size;//���Ѹ�ŭ ��ü���ũ�⿡�� ����
			_Non_Use_Size = _Non_Use_Size + _Size;//������ŭ ���ũ�� ����

			return _Size;
		}
		//�ƴ϶��
		_Front = _Buffer_Start;//������ �������� �̵�
		_Front = _Front - (_Size - De_size);//���������� ���� ���� ��ť������� �̵��ѻ����ŭ ���� _Rear���� �̵�

		_Use_Size = _Use_Size - (_Size - De_size);//���Ѹ�ŭ ��ü���ũ�⿡�� ����
		_Non_Use_Size = _Non_Use_Size + (_Size - De_size);//������ŭ ���ũ�� ����

		return _Size;//�������� ����
	};

	void TestFront(int iSize)
	{
		_Front += iSize;
		if (_Front >= _Buffer_End)
		{
			int over = _Front - _Buffer_End;
			_Front = _Buffer_Start + over;
		}
		//_Use_Size = _Use_Size - iSize;//���Ѹ�ŭ ��ü���ũ�⿡�� ����
		//_Non_Use_Size = _Non_Use_Size + iSize;//������ŭ ���ũ�� ����

	}

	/////////////////////////////////////////////////////////////////////////
	// ������ ��� ����Ÿ ����.
	//
	// Parameters: ����.
	// Return: ����.
	/////////////////////////////////////////////////////////////////////////
	void	ClearBuffer(void)
	{
		//���� �ʱ�ȭ
		_Rear = _Buffer_Start;
		_Front = _Buffer_Start;
	};


	/////////////////////////////////////////////////////////////////////////
	// ������ Front ������ ����.
	//
	// Parameters: ����.
	// Return: (char *) ���� ������.
	/////////////////////////////////////////////////////////////////////////
	char* GetFrontBufferPtr(void)
	{
		return _Front;
	};

	/////////////////////////////////////////////////////////////////////////
	// ������ RearPos ������ ����.
	//
	// Parameters: ����.
	// Return: (char *) ���� ������.
	/////////////////////////////////////////////////////////////////////////
	char* GetRearBufferPtr(void)
	{
		return _Rear;
	};

	//void _print()//��Ȯ�ο�
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