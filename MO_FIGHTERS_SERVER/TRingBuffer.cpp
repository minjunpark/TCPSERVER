#include "iostream"
#include "TRingbuffer.h"

TRingBuffer::TRingBuffer()//����Ʈ ���۰�� 10000����Ʈ�� �⺻�����Ѵ�
{
	_Start = nullptr;
	if (nullptr != _Start)
		delete[] _Start;
	_BufferSize = 10000;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];
}

TRingBuffer::TRingBuffer(int iBufferSize)//���� ũ�⸸ŭ ���۸� ���� 0���ϸ� ������ ���ʿ� ���Ѵ�.
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

TRingBuffer::~TRingBuffer()//���� �Ҹ���
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
	//char* _Old_Start = _Start;//���� ��������
	//int _Old_BufferSize = _BufferSize;//�� ���� ũ��
	//int _Old_Front = _Front;//�б� ������ ����� ���Ѱ�
	//int _Old_Rear = _Rear;//���� ������ ����� ���Ѱ�

	//char* _tmp_buffer = new char[size];//�̵��� ��ŭ�� ũ���� ���۸� �̸������.
	//
	//int retresize = Peek(_tmp_buffer, GetUseSize());//��� �����͸� �����ؿ´�.

	//if (retresize != GetUseSize())//������ ũ�Ⱑ �۴ٸ�
	//{
	//	delete[] _tmp_buffer;//�����ߴٸ� �������۸� ���´�.
	//}
	//else
	//{
	//	//�����ߴٸ�
	//	delete[] _Start;//���� ���۸� ������ �����ϰ�
	//	_Start = _tmp_buffer;//���ο� ���۸� �����Ѵ�.
	//	_BufferSize = size;
	//	_Front = 0;
	//	_Rear = 0;
	//}
};

/////////////////////////////////////////////////////////////////////////
// ���� �ִ��� ũ�⸦ ���.
//
// Parameters: ����.
// Return: (int)�ִ��� ũ�� ��.
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
// ���� ������� �뷮 ���.
//
// Parameters: ����.
// Return: (int)������� �뷮.
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
// ���� ���ۿ� ���� �뷮 ���. 
//
// Parameters: ����.
// Return: (int)�����뷮.
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::GetFreeSize(void)
{
	return _BufferSize - (GetUseSize() + VOID_VALUE);
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
// _Rear �� ����Ÿ �ְ� _Rear�� �׸�ŭ �̵���Ų��.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��. 
// Return: (int)���� ũ��.
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::Enqueue(char* chpData, int iSize)
{
	int iRear;

	if (GetFreeSize() < iSize)
	{
		return 0;
	}

	if (iSize <= 0)//����ŷ �����ΰ�� -1�� ���� ��찡 �ֱ� ������ �̸� ���
	{
		return 0;
	}

	if (_Front <= _Rear)//ũ�Ⱑ front ���� �۴ٸ�
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
// _Front ���� ����Ÿ ������. _Front �̵�.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::Dequeue(char* chpDest, int iSize)
{
	int iFront;

	if (GetUseSize() < iSize)//��û�� ����� �����ü� �ִ� ũ�⺸�� �۴ٸ�
	{
		//return 0;//��û�� ũ�Ⱑ ���� �ִ� ������� ũ�ٸ� �׳� 0�� �����ϰ� �����Ѵ�.
		iSize = GetUseSize();//������ �ִ� ũ�⸸ŭ �����ֱ�
	}

	if (iSize <= 0)//����ŷ �����ΰ�� -1�� ���� ��찡 �ֱ� ������ �̸� ���
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
// _Front ���� ����Ÿ �о��. _Front�� �̵���Ű�� �ʴ´�.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
/////////////////////////////////////////////////////////////////////////
int	TRingBuffer::Peek(char* chpDest, int iSize)
{
	int iFront;
	if (GetUseSize() < iSize)
	{
		//return 0;//��û�� ũ�Ⱑ ���� �ִ� ������� ũ�ٸ� �׳� 0�� �����ϰ� �����Ѵ�.
		iSize = GetUseSize();//������ �ִ� ũ�⸸ŭ �����ֱ�
	}

	if (iSize <= 0)//����ŷ �����ΰ�� -1�� ���� ��찡 �ֱ� ������ �̸� ���
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
// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
//
// Parameters: ����.
// Return: (int)�̵�ũ��
/////////////////////////////////////////////////////////////////////////
int TRingBuffer::MoveRear(int iSize)
{
	_Rear = (_Rear + iSize) % _BufferSize;
	return iSize;//�̵��� _Rear������ ��ġ�� �����Ѵ�.
}

int TRingBuffer::MoveFront(int iSize)
{
	_Front = (_Front + iSize) % _BufferSize;//�̵��� ũ�⸸ŭ �ְ�
	return iSize;//�̵��� _Front������ ��ġ�� �����Ѵ�.
}

/////////////////////////////////////////////////////////////////////////
// ��� �����Ͱ� �ʱ�ȭ��Ű��
//
// Parameters: ����.
// Return: ����.
/////////////////////////////////////////////////////////////////////////

void TRingBuffer::ClearBuffer(void)
{
	_Front = 0;
	_Rear = 0;
}


/////////////////////////////////////////////////////////////////////////
// ������ Front ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* TRingBuffer::GetFrontBufferPtr(void)
{
	return _Start + _Front;
}

/////////////////////////////////////////////////////////////////////////
// ������ RearPos ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* TRingBuffer::GetRearBufferPtr(void)
{
	return _Start + _Rear;
}