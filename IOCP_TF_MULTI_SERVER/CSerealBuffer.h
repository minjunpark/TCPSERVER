#pragma once

#ifndef  __PACKET__
#define  __PACKET__
#include <Windows.h>

#include "CMemoryPool.h"

struct LanServerHeader
{
	unsigned short _Len;
};

struct NetServerHeader
{
	unsigned char _ByteCode;
	unsigned short _Len;
	unsigned char _RandomKey;
	unsigned char _CheckSum;
};

class CSerealBuffer
{


protected:

	//------------------------------------------------------------
	// ���� ���ۿ� ������� ������.
	//------------------------------------------------------------
	char* _Start;//���� ��������
	int	_BufferSize;//������ ũ��
	int _Rear;//������� ������
	int _Front;//�б⾲�� ������
	int	_UseSize;//������� ũ��



public:

	/*---------------------------------------------------------------
	CSerealBuffer Enum.

	----------------------------------------------------------------*/
	enum en_PACKET
	{
		//eBUFFER_DEFAULT = 1400		// ��Ŷ�� �⺻ ���� ������.
		eBUFFER_DEFAULT = 1460		// ��Ŷ�� �⺻ ���� ������.
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CSerealBuffer();
	CSerealBuffer(int iBufferSize);

	virtual	~CSerealBuffer();


	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void)
	{
		return _BufferSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseSize(void)
	{
		return _UseSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ũ�� �ø���
	//
	// Parameters: (int) ũ�⸦ �ø� ������.
	// Return: (bool) ���� ���� ��������
	//////////////////////////////////////////////////////////////////////////

	bool ReSize(int _ReSize)
	{
		if (_ReSize <= 0)//������� ������� ����
			return false;

		if (_ReSize <= _BufferSize)//�������� �Ϸ��� ũ�Ⱑ ���� ũ�⺸�� �۰ų� ���ٸ� ����->�������� ����.
			return false;

		if (_Start == nullptr)//nullptr�̶�°� ������ �߻��� ��Ȳ�̹Ƿ� �����Ѵ�.
			return false;

		char* temp = new char[_BufferSize];//Swap�� �ӽù���
		memcpy(temp, _Start, _BufferSize);//�ӽù��ۿ� ��� �����͸� ��������

		delete[] _Start;//���� ���۸� ������ �����ϰ�
		_Start = new char[_ReSize];//���ο� ���۸� ��������
		memcpy(_Start, temp, _BufferSize);//���ο� ���ۿ� �ӽù��ۿ� �Űܵ� ��� �����͸� ������Ʈ�Ѵ�
		_BufferSize = _ReSize;//���ۻ������ ���ε��� resize�� �����Ѵ�
		//front rear UseSize�� ������ �״�� ����Ұ��̱� ������ �������� �ʴ´�.

		return true;//�������� ����
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void)
	{
		return _Start;
	}

	char* GetReadPtr(void)
	{
		return _Start;
	}

	char* GetWritePtr(void)
	{
		return _Start;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int	GetData(char* chpDest, int iSize);
	int	GetPeek(char* chpDest, int iSize);
	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int	PutData(char* chpSrc, int iSize);

	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	CSerealBuffer& operator = (CSerealBuffer& clSrcPacket)
	{
		if (this != &clSrcPacket)
		{
			delete[] _Start;
			_BufferSize = clSrcPacket._BufferSize;
			_UseSize = clSrcPacket._UseSize;
			_Start = new char[_BufferSize];
			memcpy(_Start, clSrcPacket._Start, _BufferSize);
			_Front = clSrcPacket._Front;
			_Rear = clSrcPacket._Rear;
		}
		return *this;
	};

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CSerealBuffer& operator << (unsigned char byValue)
	{
		if (_BufferSize - _Rear >= sizeof(unsigned char))
		{
			memcpy(_Start + _Rear, &byValue, sizeof(unsigned char));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(unsigned char);//������ŭ ������� �̵�
			_UseSize += sizeof(unsigned char);//������ŭ ������� ���� ũ�� ������Ű��
		}
		return *this;
	};

	CSerealBuffer& operator << (char chValue)
	{
		if (_BufferSize - _Rear >= sizeof(char))
		{
			memcpy(_Start + _Rear, &chValue, sizeof(unsigned char));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(char);//������ŭ ������� �̵�
			_UseSize += sizeof(char);//������ŭ ������� ���� ũ�� ������Ű��
		}
		return *this;
	};

	CSerealBuffer& operator << (short shValue)
	{
		if (_BufferSize - _Rear >= sizeof(short))
		{
			memcpy(_Start + _Rear, &shValue, sizeof(short));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(short);//������ŭ �̵���Ű��
			_UseSize += sizeof(short);
		}
		return *this;
	};

	CSerealBuffer& operator << (unsigned short wValue)
	{
		if (_BufferSize - _Rear >= sizeof(unsigned short)) {
			memcpy(_Start + _Rear, &wValue, sizeof(unsigned short));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(unsigned short);//������ŭ �̵���Ű��
			_UseSize += sizeof(unsigned short);
		}
		return *this;
	};

	CSerealBuffer& operator << (int iValue)
	{
		if (_BufferSize - _Rear >= sizeof(int))
		{
			memcpy(_Start + _Rear, &iValue, sizeof(int));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(int);//������ŭ �̵���Ű��
			_UseSize += sizeof(int);
		}
		return *this;
	};

	CSerealBuffer& operator << (unsigned int iValue)
	{
		if (_BufferSize - _Rear >= sizeof(unsigned int))
		{
			memcpy(_Start + _Rear, &iValue, sizeof(unsigned int));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(unsigned int);//������ŭ �̵���Ű��
			_UseSize += sizeof(unsigned int);
		}
		return *this;
	};

	CSerealBuffer& operator << (long lValue)
	{
		if (_BufferSize - _Rear >= sizeof(long))
		{
			memcpy(_Start + _Rear, &lValue, sizeof(long));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(long);//������ŭ �̵���Ű��
			_UseSize += sizeof(long);
		}
		return *this;
	};

	CSerealBuffer& operator << (float fValue)
	{
		if (_BufferSize - _Rear >= sizeof(float))
		{
			memcpy(_Start + _Rear, &fValue, sizeof(float));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(float);//������ŭ �̵���Ű��
			_UseSize += sizeof(float);
		}
		return *this;
	};

	CSerealBuffer& operator << (__int64 iValue)
	{
		if (_BufferSize - _Rear >= sizeof(__int64))
		{
			memcpy(_Start + _Rear, &iValue, sizeof(__int64));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(__int64);//������ŭ �̵���Ű��
			_UseSize += sizeof(__int64);
		}
		return *this;
	};

	CSerealBuffer& operator << (double dValue)
	{
		if (_BufferSize - _Rear >= sizeof(double))
		{
			memcpy(_Start + _Rear, &dValue, sizeof(double));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(double);//������ŭ �̵���Ű��
			_UseSize += sizeof(double);
		}
		return *this;
	};

	CSerealBuffer& operator << (DWORD64 dValue)
	{
		if (_BufferSize - _Rear >= sizeof(DWORD64))
		{
			memcpy(_Start + _Rear, &dValue, sizeof(DWORD64));//���� ũ�⸸ŭ �����ؼ� �ְ�
			_Rear += sizeof(DWORD64);//������ŭ �̵���Ű��
			_UseSize += sizeof(DWORD64);
		}
		return *this;
	};

	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CSerealBuffer& operator >> (BYTE& byValue)
	{
		if (_UseSize >= sizeof(BYTE))
		{
			memcpy(&byValue, _Start + _Front, sizeof(BYTE));//����ŭ �����ؼ� ����.
			_Front += sizeof(BYTE);//���ۿ��� ����ŭ �̵���Ų��.
			_UseSize -= sizeof(BYTE);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (char& chValue)
	{
		// �о�� �� ũ�⺸�� ���� ũ�Ⱑ ������ �ƹ��͵� ���� ����
		if (_UseSize >= sizeof(char))
		{
			memcpy(&chValue, _Start + _Front, sizeof(char));// ���ۿ��� ���� �����ؼ� �о��
			_Front += sizeof(char);// ���� ũ�⸸ŭ �����͸� �̵���Ŵ
			_UseSize -= sizeof(char);// ����� ũ�⸦ ���ҽ�Ŵ
		}
		return *this;
	};

	CSerealBuffer& operator >> (short& shValue)
	{
		if (_UseSize >= sizeof(short))
		{
			memcpy(&shValue, _Start + _Front, sizeof(short));//����ŭ �����ؼ� ����.
			_Front += sizeof(short);//������ŭ
			_UseSize -= sizeof(short);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (WORD& wValue)
	{
		if (_UseSize >= sizeof(WORD))
		{
			memcpy(&wValue, _Start + _Front, sizeof(WORD));//����ŭ �����ؼ� ����.
			_Front += sizeof(WORD);//������ŭ
			_UseSize -= sizeof(WORD);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (int& iValue)
	{
		if (_UseSize >= sizeof(int))
		{
			memcpy(&iValue, _Start + _Front, sizeof(int));//����ŭ �����ؼ� ����.
			_Front += sizeof(int);//������ŭ
			_UseSize -= sizeof(int);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (unsigned int& iValue)
	{
		if (_UseSize >= sizeof(unsigned int))
		{
			memcpy(&iValue, _Start + _Front, sizeof(int));//����ŭ �����ؼ� ����.
			_Front += sizeof(unsigned int);//������ŭ
			_UseSize -= sizeof(unsigned int);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (DWORD& dwValue)
	{
		if (_UseSize >= sizeof(DWORD))
		{
			memcpy(&dwValue, _Start + _Front, sizeof(DWORD));//����ŭ �����ؼ� ����.
			_Front += sizeof(DWORD);//������ŭ
			_UseSize -= sizeof(DWORD);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (float& fValue)
	{
		if (_UseSize >= sizeof(float))
		{
			memcpy(&fValue, _Start + _Front, sizeof(float));//����ŭ �����ؼ� ����.
			_Front += sizeof(float);//������ŭ
			_UseSize -= sizeof(float);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (__int64& iValue)
	{
		if (_UseSize >= sizeof(__int64))
		{
			memcpy(&iValue, _Start + _Front, sizeof(__int64));//����ŭ �����ؼ� ����.
			_Front += sizeof(__int64);//������ŭ
			_UseSize -= sizeof(__int64);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (double& dValue)
	{
		if (_UseSize >= sizeof(double))
		{
			memcpy(&dValue, _Start + _Front, sizeof(double));//����ŭ �����ؼ� ����.
			_Front += sizeof(double);//������ŭ
			_UseSize -= sizeof(double);//����ŭ ����� ������� �A��.
		}
		return *this;
	};

	CSerealBuffer& operator >> (DWORD64& iValue)
	{
		if (_UseSize >= sizeof(DWORD64))
		{
			memcpy(&iValue, _Start + _Front, sizeof(DWORD64));//����ŭ �����ؼ� ����.
			_Front += sizeof(DWORD64);//������ŭ
			_UseSize -= sizeof(DWORD64);//����ŭ ����� ������� �A��.
		}
		return *this;
	};
};

extern CMemoryPool<CSerealBuffer> g_PacketObjectPool;
#endif