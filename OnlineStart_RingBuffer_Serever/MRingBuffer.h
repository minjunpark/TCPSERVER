#pragma once
#include <iostream>

class MRingBuffer
{
private:
	char* _Start;//���� ������
	char* _End;//�� ������
	char* _Rear;//����������
	char* _Front;//�б�������

	int _RingBufferSize;

public:
	MRingBuffer(void)
	{
		_RingBufferSize = 10000;
		_Start = (char*)malloc(_RingBufferSize);//�⺻ũ�� 10000
		//_Start = new char[_RingBufferSize];//�⺻ũ�� 10000
		_End = _Start + _RingBufferSize;
		_Front = _Start;
		_Rear = _Start;
	};
	MRingBuffer(int iBufferSize)
	{
		_RingBufferSize = iBufferSize;
		_Start = (char*)malloc(_RingBufferSize);//�⺻ũ��
		//_Start = new char[_RingBufferSize];
		_End = _Start + _RingBufferSize;
		_Front = _Start;
		_Rear = _Start;
	};
	//�Ҹ���
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
		//RingBufferFree();//������ ������ ����
	};

	//���ۻ�����ũ��
	int GetBufferSize()
	{
		return _End - _Start - 1;
	}

	/////////////////////////////////////////////////////////////////////////
	// WritePos �� ����Ÿ ����.
	//
	// Parameters: (char *)����Ÿ ������. (int)ũ��. 
	// Return: (int)���� ũ��.
	// �긦 ���� ���и� �����ؾ��ϳ� ����� �����ؾ��ϳ�
	/////////////////////////////////////////////////////////////////////////
	int Enqueue(const char* chpData, int size)
	{
		if (GetFreeSize() < size || size < 0)
		{
			return 0;
		}

		int _DE_Size = DirectEnqueueSize();//�Լ�ȣ�� �ּ�ȭ
		const char* temp = chpData;

		if (_DE_Size >= size)
		{
			memcpy_s(_Rear, size, chpData, size);
			//memcpy(_Rear, buffer, size);//�ӵ� ���̳��� �־ memcpy���°� ���ٰ��ϴµ� �׽�Ʈ�� ��Ȯ�� �ڷᰡ �ʿ��ҵ�
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
	// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
	//
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)������ ũ��.
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
	// ReadPos ���� ����Ÿ �о��. ReadPos ����.
	//
	// Parameters: (char *)����Ÿ ������. (int)ũ��.
	// Return: (int)������ ũ��.
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

	//�������� ���߿�
	void Resize(int size) {

		//if (_Buffer_Size > size)
		//{
		//	return;
		//}

		//char* resize_Buffer = (char*)malloc(size);//�������� �Ϸ��� ���� ũ�⸸ŭ �ø���

		//memcpy_s(resize_Buffer, _Buffer_Size, _Buffer_Start, _Buffer_Size);//���۸� �״�� �����Ѵ�
		//
		return;
	};

	/////////////////////////////////////////////////////////////////////////
	// ���� ������� �뷮 ���.
	//
	// Parameters: ����.
	// Return: (int)������� �뷮.
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
	// ���� ���ۿ� ���� �뷮 ���. 
	//
	// Parameters: ����.
	// Return: (int)�����뷮.
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
	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// (������ ���� ����)
	//
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	//
	// Parameters: ����.
	// Return: (int)��밡�� �뷮.
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
		_Front += size;//����Ʈ�� �� �����ŭ �̵���Ű��
		if (_Front >= _End - 1)//���� �������� �Ѿ�ٸ�
		{
			int over = _Front - _End;//�Ѿ�°���ŭ �����
			_Front = _Start + over;//���ۺ��� ����Ʈ�� ���ϰ� �̵���Ų��.
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// ������ ��� ����Ÿ ����.
	//
	// Parameters: ����.
	// Return: ����.
	/////////////////////////////////////////////////////////////////////////
	void ClearBuffeR()
	{
		//Rear�� Start�� ���� ��ġ�� �̵����� �ʱ�ȭ ��Ų��.
		_Rear = _Start;
		_Front = _Start;
	};

	/////////////////////////////////////////////////////////////////////////
	// ������ Front ������ ����.
	//
	// Parameters: ����.
	// Return: (char *) ���� ������.
	/////////////////////////////////////////////////////////////////////////

	char* GetFrontBufferPtr()
	{
		return _Front;//�б������� ��������
	};

	/////////////////////////////////////////////////////////////////////////
	// ������ RearPos ������ ����.
	//
	// Parameters: ����.
	// Return: (char *) ���� ������.
	/////////////////////////////////////////////////////////////////////////
	char* GetRearBufferPtr()
	{
		return _Rear;//���������� ��������
	};


	//void _print()//��Ȯ�ο�
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