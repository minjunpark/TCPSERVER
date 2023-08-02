#include "iostream"
#include "CRingBuffer.h"

CRingBuffer::CRingBuffer()//����Ʈ ���۰�� 10000����Ʈ�� �⺻�����Ѵ�
{
	InitializeSRWLock(&_Lock);
	_Start = nullptr;
	if (nullptr != _Start)
		delete[] _Start;
	_BufferSize = 10000;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];
}


CRingBuffer::CRingBuffer(int iBufferSize)//���� ũ�⸸ŭ ���۸� ���� 0���ϸ� ������ ���ʿ� ���Ѵ�.
{
	InitializeSRWLock(&_Lock);
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

CRingBuffer::~CRingBuffer()//���� �Ҹ���
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


bool CRingBuffer::Resize(int size)
{
	if (size <= 0)//������� ������� ����
		return false;

	if (size <= _BufferSize)//�������� �Ϸ��� ũ�Ⱑ ���� ũ�⺸�� �۰ų� ���ٸ� ����->�������� ����.
		return false;

	if (_Start == nullptr)//nullptr�̶�°� ������ �߻��� ��Ȳ�̹Ƿ� �����Ѵ�.
		return false;

	char* temp = new char[_BufferSize];

	int retResize = Peek(temp, GetUseSize());//��� �����͸� �����ؿ´�.

	if (retResize != GetUseSize())//������ ũ�Ⱑ �۴ٸ�
	{
		delete[] temp;//�����ߴٸ� �������۸� ������ �����Ѵ�.
		return false;
	}

	delete[] _Start;//���� ���۸� ������ �����ϰ�
	_Start = new char[size];//���ο� ���۸� �����Ѵ�.
	memcpy(_Start, temp, _BufferSize);//���� ������� ���ۿ� �����͸� �ִ´�.
	_BufferSize = size;//����� ������� ������
	_Front = 0;//�б������ʹ� �Ǿ�
	_Rear = retResize;//���� �����ʹ� ������ �����͸�ŭ �̵���Ų��.

	delete[] temp;//���� �ӽù��۸� �����Ѵ�.

	return true;
};

/////////////////////////////////////////////////////////////////////////
// ���� �ִ��� ũ�⸦ ���.
//
// Parameters: ����.
// Return: (int)�ִ��� ũ�� ��.
/////////////////////////////////////////////////////////////////////////

int	CRingBuffer::GetBufferSize(void)
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

int CRingBuffer::GetUseSize(void)
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
int CRingBuffer::GetFreeSize(void)
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
int CRingBuffer::DirectDequeueSize(void)
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

int CRingBuffer::DirectEnqueueSize(void)
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
int CRingBuffer::Enqueue(char* chpData, int iSize)
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
int CRingBuffer::Dequeue(char* chpDest, int iSize)
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
int	CRingBuffer::Peek(char* chpDest, int iSize)
{
	if (GetUseSize() < iSize)
	{
		//return 0;//��û�� ũ�Ⱑ ���� �ִ� ������� ũ�ٸ� �׳� 0�� �����ϰ� �����Ѵ�.
		iSize = GetUseSize();//������ �ִ� ũ�⸸ŭ �����ֱ�
	}

	int iFront;

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
int CRingBuffer::MoveRear(int iSize)
{
	_Rear = (_Rear + iSize) % _BufferSize;
	return iSize;//�̵��� _Rear������ ��ġ�� �����Ѵ�.
}

int CRingBuffer::MoveFront(int iSize)
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

void CRingBuffer::ClearBuffer(void)
{
	_Front = 0;
	_Rear = 0;
}

/////////////////////////////////////////////////////////////////////////
// ������ Start ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetStartBufferPtr(void)
{
	return _Start;
}

/////////////////////////////////////////////////////////////////////////
// ������ Front ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetFrontBufferPtr(void)
{
	return _Start + _Front;
}

/////////////////////////////////////////////////////////////////////////
// ������ RearPos ������ ����.
//
// Parameters: ����.
// Return: (char *) ���� ������.
/////////////////////////////////////////////////////////////////////////
char* CRingBuffer::GetRearBufferPtr(void)
{
	return _Start + _Rear;
}

void CRingBuffer::SH_Lock()
{
	AcquireSRWLockShared(&_Lock);
}

void CRingBuffer::SH_UnLock()
{
	ReleaseSRWLockShared(&_Lock);
}

void CRingBuffer::EX_Lock()
{
	AcquireSRWLockExclusive(&_Lock);
}

void CRingBuffer::EX_UnLock()
{
	ReleaseSRWLockExclusive(&_Lock);
}