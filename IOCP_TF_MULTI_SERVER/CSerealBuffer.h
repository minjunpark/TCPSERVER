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
	// 현재 버퍼에 사용중인 사이즈.
	//------------------------------------------------------------
	char* _Start;//버퍼 시작지점
	int	_BufferSize;//버퍼의 크기
	int _Rear;//쓰기버퍼 사이즈
	int _Front;//읽기쓰기 사이즈
	int	_UseSize;//사용중인 크기



public:

	/*---------------------------------------------------------------
	CSerealBuffer Enum.

	----------------------------------------------------------------*/
	enum en_PACKET
	{
		//eBUFFER_DEFAULT = 1400		// 패킷의 기본 버퍼 사이즈.
		eBUFFER_DEFAULT = 1460		// 패킷의 기본 버퍼 사이즈.
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CSerealBuffer();
	CSerealBuffer(int iBufferSize);

	virtual	~CSerealBuffer();


	//////////////////////////////////////////////////////////////////////////
	// 패킷 청소.
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// 버퍼 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)패킷 버퍼 사이즈 얻기.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void)
	{
		return _BufferSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 데이타 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseSize(void)
	{
		return _UseSize;
	}

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 크기 늘리기
	//
	// Parameters: (int) 크기를 늘릴 사이즈.
	// Return: (bool) 버퍼 변경 성공여부
	//////////////////////////////////////////////////////////////////////////

	bool ReSize(int _ReSize)
	{
		if (_ReSize <= 0)//음수라면 리사이즈를 실패
			return false;

		if (_ReSize <= _BufferSize)//리사이즈 하려는 크기가 원래 크기보다 작거나 같다면 실패->할이유가 없다.
			return false;

		if (_Start == nullptr)//nullptr이라는건 에러가 발생한 상황이므로 정지한다.
			return false;

		char* temp = new char[_BufferSize];//Swap용 임시버퍼
		memcpy(temp, _Start, _BufferSize);//임시버퍼에 모든 데이터를 복사한후

		delete[] _Start;//과거 버퍼를 힙에서 제거하고
		_Start = new char[_ReSize];//새로운 버퍼를 세팅한후
		memcpy(_Start, temp, _BufferSize);//새로운 버퍼에 임시버퍼에 옮겨둔 모든 데이터를 업데이트한다
		_BufferSize = _ReSize;//버퍼사이즈는 새로들어온 resize로 변경한다
		//front rear UseSize는 어차피 그대로 사용할것이기 때문에 변경하지 않는다.

		return true;//리사이즈 성공
	}

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
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
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	GetData(char* chpDest, int iSize);
	int	GetPeek(char* chpDest, int iSize);
	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	PutData(char* chpSrc, int iSize);

	/* ============================================================================= */
	// 연산자 오버로딩
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
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CSerealBuffer& operator << (unsigned char byValue)
	{
		if (_BufferSize - _Rear >= sizeof(unsigned char))
		{
			memcpy(_Start + _Rear, &byValue, sizeof(unsigned char));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(unsigned char);//넣은만큼 쓰기버퍼 이동
			_UseSize += sizeof(unsigned char);//넣은만큼 사용중인 버퍼 크기 증가시키기
		}
		return *this;
	};

	CSerealBuffer& operator << (char chValue)
	{
		if (_BufferSize - _Rear >= sizeof(char))
		{
			memcpy(_Start + _Rear, &chValue, sizeof(unsigned char));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(char);//넣은만큼 쓰기버퍼 이동
			_UseSize += sizeof(char);//넣은만큼 사용중인 버퍼 크기 증가시키기
		}
		return *this;
	};

	CSerealBuffer& operator << (short shValue)
	{
		if (_BufferSize - _Rear >= sizeof(short))
		{
			memcpy(_Start + _Rear, &shValue, sizeof(short));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(short);//넣은만큼 이동시키기
			_UseSize += sizeof(short);
		}
		return *this;
	};

	CSerealBuffer& operator << (unsigned short wValue)
	{
		if (_BufferSize - _Rear >= sizeof(unsigned short)) {
			memcpy(_Start + _Rear, &wValue, sizeof(unsigned short));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(unsigned short);//넣은만큼 이동시키기
			_UseSize += sizeof(unsigned short);
		}
		return *this;
	};

	CSerealBuffer& operator << (int iValue)
	{
		if (_BufferSize - _Rear >= sizeof(int))
		{
			memcpy(_Start + _Rear, &iValue, sizeof(int));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(int);//넣은만큼 이동시키기
			_UseSize += sizeof(int);
		}
		return *this;
	};

	CSerealBuffer& operator << (unsigned int iValue)
	{
		if (_BufferSize - _Rear >= sizeof(unsigned int))
		{
			memcpy(_Start + _Rear, &iValue, sizeof(unsigned int));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(unsigned int);//넣은만큼 이동시키기
			_UseSize += sizeof(unsigned int);
		}
		return *this;
	};

	CSerealBuffer& operator << (long lValue)
	{
		if (_BufferSize - _Rear >= sizeof(long))
		{
			memcpy(_Start + _Rear, &lValue, sizeof(long));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(long);//넣은만큼 이동시키기
			_UseSize += sizeof(long);
		}
		return *this;
	};

	CSerealBuffer& operator << (float fValue)
	{
		if (_BufferSize - _Rear >= sizeof(float))
		{
			memcpy(_Start + _Rear, &fValue, sizeof(float));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(float);//넣은만큼 이동시키기
			_UseSize += sizeof(float);
		}
		return *this;
	};

	CSerealBuffer& operator << (__int64 iValue)
	{
		if (_BufferSize - _Rear >= sizeof(__int64))
		{
			memcpy(_Start + _Rear, &iValue, sizeof(__int64));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(__int64);//넣은만큼 이동시키기
			_UseSize += sizeof(__int64);
		}
		return *this;
	};

	CSerealBuffer& operator << (double dValue)
	{
		if (_BufferSize - _Rear >= sizeof(double))
		{
			memcpy(_Start + _Rear, &dValue, sizeof(double));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(double);//넣은만큼 이동시키기
			_UseSize += sizeof(double);
		}
		return *this;
	};

	CSerealBuffer& operator << (DWORD64 dValue)
	{
		if (_BufferSize - _Rear >= sizeof(DWORD64))
		{
			memcpy(_Start + _Rear, &dValue, sizeof(DWORD64));//넣을 크기만큼 복사해서 넣고
			_Rear += sizeof(DWORD64);//넣은만큼 이동시키기
			_UseSize += sizeof(DWORD64);
		}
		return *this;
	};

	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CSerealBuffer& operator >> (BYTE& byValue)
	{
		if (_UseSize >= sizeof(BYTE))
		{
			memcpy(&byValue, _Start + _Front, sizeof(BYTE));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(BYTE);//버퍼에서 뺀만큼 이동시킨다.
			_UseSize -= sizeof(BYTE);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (char& chValue)
	{
		// 읽어야 할 크기보다 버퍼 크기가 작으면 아무것도 하지 않음
		if (_UseSize >= sizeof(char))
		{
			memcpy(&chValue, _Start + _Front, sizeof(char));// 버퍼에서 값을 복사해서 읽어옴
			_Front += sizeof(char);// 읽은 크기만큼 포인터를 이동시킴
			_UseSize -= sizeof(char);// 사용한 크기를 감소시킴
		}
		return *this;
	};

	CSerealBuffer& operator >> (short& shValue)
	{
		if (_UseSize >= sizeof(short))
		{
			memcpy(&shValue, _Start + _Front, sizeof(short));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(short);//읽은만큼
			_UseSize -= sizeof(short);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (WORD& wValue)
	{
		if (_UseSize >= sizeof(WORD))
		{
			memcpy(&wValue, _Start + _Front, sizeof(WORD));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(WORD);//읽은만큼
			_UseSize -= sizeof(WORD);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (int& iValue)
	{
		if (_UseSize >= sizeof(int))
		{
			memcpy(&iValue, _Start + _Front, sizeof(int));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(int);//읽은만큼
			_UseSize -= sizeof(int);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (unsigned int& iValue)
	{
		if (_UseSize >= sizeof(unsigned int))
		{
			memcpy(&iValue, _Start + _Front, sizeof(int));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(unsigned int);//읽은만큼
			_UseSize -= sizeof(unsigned int);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (DWORD& dwValue)
	{
		if (_UseSize >= sizeof(DWORD))
		{
			memcpy(&dwValue, _Start + _Front, sizeof(DWORD));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(DWORD);//읽은만큼
			_UseSize -= sizeof(DWORD);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (float& fValue)
	{
		if (_UseSize >= sizeof(float))
		{
			memcpy(&fValue, _Start + _Front, sizeof(float));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(float);//읽은만큼
			_UseSize -= sizeof(float);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (__int64& iValue)
	{
		if (_UseSize >= sizeof(__int64))
		{
			memcpy(&iValue, _Start + _Front, sizeof(__int64));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(__int64);//읽은만큼
			_UseSize -= sizeof(__int64);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (double& dValue)
	{
		if (_UseSize >= sizeof(double))
		{
			memcpy(&dValue, _Start + _Front, sizeof(double));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(double);//읽은만큼
			_UseSize -= sizeof(double);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};

	CSerealBuffer& operator >> (DWORD64& iValue)
	{
		if (_UseSize >= sizeof(DWORD64))
		{
			memcpy(&iValue, _Start + _Front, sizeof(DWORD64));//뺄만큼 복사해서 뺀다.
			_Front += sizeof(DWORD64);//읽은만큼
			_UseSize -= sizeof(DWORD64);//뺀만큼 사용한 사이즈에서 뺸다.
		}
		return *this;
	};
};

extern CMemoryPool<CSerealBuffer> g_PacketObjectPool;
#endif