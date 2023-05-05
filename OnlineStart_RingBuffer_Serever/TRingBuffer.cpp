#include "iostream"
#include "TRingbuffer.h"

TRingBuffer::TRingBuffer()//����Ʈ ���۰�� 10000����Ʈ�� �⺻�����Ѵ�
{
	_Start = NULL;
	if (NULL != _Start)
		delete[] _Start;
	_BufferSize = 10000;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];
}

TRingBuffer::TRingBuffer(int iBufferSize)//���� ũ�⸸ŭ ���۸� ���� 0���ϸ� ������ ���ʿ� ���Ѵ�.
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

TRingBuffer::~TRingBuffer()//���� �Ҹ���
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

//�������� ���߿� �������
void TRingBuffer::Resize(int size) {

	//if (_Buffer_Size > size)
	//{
	//	return;
	//}

	//char* resize_Buffer = (char*)malloc(size);//�������� �Ϸ��� ���� ũ�⸸ŭ �ø���

	//memcpy_s(resize_Buffer, _Buffer_Size, _Buffer_Start, _Buffer_Size);//���۸� �״�� �����Ѵ�
	//

};

/////////////////////////////////////////////////////////////////////////
// ���� �ִ��� ũ�⸦ ���.
//
// Parameters: ����.
// Return: (int)�ִ��� ũ�� ��.
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
	return _BufferSize - (GetUseSize() + BLANK_SIZE);
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
// _Rear �� ����Ÿ �ְ� _Rear�� �׸�ŭ �̵���Ų��.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��. 
// Return: (int)���� ũ��.
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
// _Front ���� ����Ÿ ������. _Front �̵�.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
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
// _Front ���� ����Ÿ �о��. _Front�� �̵���Ű�� �ʴ´�.
//
// Parameters: (char *)����Ÿ ������. (int)ũ��.
// Return: (int)������ ũ��.
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
// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
//
// Parameters: ����.
// Return: (int)�̵�ũ��
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