#include "CSerealBuffer.h"


CMemoryPool<CSerealBuffer> g_PacketObjectPool;

CSerealBuffer::CSerealBuffer():_UseSize(0), _BufferSize(eBUFFER_DEFAULT), _Front(0), _Rear(0)
{
	_UseSize = 0;
	_BufferSize = eBUFFER_DEFAULT;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];

	//_Start = nullptr;
	//if (nullptr != _Start)
	//	delete[] _Start;

}

CSerealBuffer::CSerealBuffer(int iBufferSize) :_UseSize(0), _BufferSize(iBufferSize), _Front(0), _Rear(0)
{
	if (iBufferSize <= 0)
		return;

	_UseSize = 0;
	_BufferSize = iBufferSize;
	_Front = 0;
	_Rear = 0;
	_Start = new char[_BufferSize];

	//_Start = nullptr;
	//if (nullptr != _Start)
	//	delete[] _Start;
}

CSerealBuffer::~CSerealBuffer()
{
	if (nullptr != _Start)
		delete[] _Start;
	else
		_Start = nullptr;
};

void CSerealBuffer::Clear(void)
{
	_Rear = 0;
	_Front = 0;
	_UseSize = 0;
}

int	CSerealBuffer::MoveWritePos(int iSize)
{
	if (iSize <= 0)//��������
		return 0;

	if (_Rear + iSize > _BufferSize)//�̵���Ű���� ������ġ�� ���ۻ������ ũ�ٸ�?
		return 0;
	
	_Rear += iSize;//ũ�⸸ŭ �̵���Ų��.
	_UseSize += iSize;//����ŭ ���Ѵ�.

	return iSize;//�̵��� _Rear������ ��ġ�� �����Ѵ�.
}

int	CSerealBuffer::MoveReadPos(int iSize)
{
	if (iSize <= 0)//��������
		return 0;
	
	if (_UseSize < iSize)
		return 0;

	_Front += iSize;//ũ�⸸ŭ �̵���Ų��.
	_UseSize -= iSize;//���� ��ŭ ����

	return iSize;
}

int	CSerealBuffer::GetData(char* chpDest, int iSize)
{
	//���������� ����� ���� ������� ������� �۴ٸ�
	//����Ʈ���� ���ڶ�°��̹Ƿ� 0�� ���Ͻ����ش�
	if (_UseSize < iSize)
		return 0;

	if (iSize <= 0)//��������
		return 0;

	memcpy(chpDest, _Start + _Front, iSize);//���ϴ� ������ ��ŭ �����͸� �̾ƿ���
	_Front += iSize;//�б� �����͸� �̵���Ų��.
	_UseSize -= iSize;//������� ����� ���ҽ�Ų��.

	return iSize;
};

int	CSerealBuffer::GetPeek(char* chpDest, int iSize)
{
	//���������� ����� ���� ������� ������� �۴ٸ�
	//����Ʈ���� ���ڶ�°��̹Ƿ� 0�� ���Ͻ����ش�
	if (_UseSize < iSize)
		return 0;

	if (iSize <= 0)//��������
		return 0;

	memcpy(chpDest, _Start + _Front, iSize);//���ϴ� ������ ��ŭ �����͸� �̾ƿ���

	return iSize;
};

int	CSerealBuffer::PutData(char* chpSrc, int iSize)
{
	//���� �ִ�ũ�⿡�� ������� ����� ���� ��밡���� ����� ���´�.
	//�� ũ�Ⱑ �������� ������ ���� �۴ٸ� ���� ���ϰ� ó���Ѵ�.
	if (_BufferSize - _Rear < iSize)
		return 0;

	if (iSize <= 0)//��������
		return 0;

	memcpy(_Start + _Rear, chpSrc, iSize);//���ۿ� �ڸ��� ���� �����Ƿ� ���� ������ �����͸� �ִ´�.
	_Rear += iSize;//������۸� ����ŭ �̵���Ű��
	_UseSize += iSize;//������� ���������ش�.

	return iSize;
};