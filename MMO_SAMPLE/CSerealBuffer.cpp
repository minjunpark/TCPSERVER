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
	if (iSize <= 0)//음수리턴
		return 0;

	if (_Rear + iSize > _BufferSize)//이동시키려는 공간위치가 버퍼사이즈보다 크다면?
		return 0;
	
	_Rear += iSize;//크기만큼 이동시킨다.
	_UseSize += iSize;//쓴만큼 더한다.

	return iSize;//이동한 _Rear포인터 위치를 리턴한다.
}

int	CSerealBuffer::MoveReadPos(int iSize)
{
	if (iSize <= 0)//음수리턴
		return 0;
	
	if (_UseSize < iSize)
		return 0;

	_Front += iSize;//크기만큼 이동시킨다.
	_UseSize -= iSize;//읽은 만큼 뺀다

	return iSize;
}

int	CSerealBuffer::GetData(char* chpDest, int iSize)
{
	//꺼내오려는 사이즈가 현재 사용중인 사이즈보다 작다면
	//바이트수가 모자라는것이므로 0을 리턴시켜준다
	if (_UseSize < iSize)
		return 0;

	if (iSize <= 0)//음수리턴
		return 0;

	memcpy(chpDest, _Start + _Front, iSize);//원하는 사이즈 만큼 데이터를 뽑아오고
	_Front += iSize;//읽기 포인터를 이동시킨다.
	_UseSize -= iSize;//사용중인 사이즈를 감소시킨다.

	return iSize;
};

int	CSerealBuffer::GetPeek(char* chpDest, int iSize)
{
	//꺼내오려는 사이즈가 현재 사용중인 사이즈보다 작다면
	//바이트수가 모자라는것이므로 0을 리턴시켜준다
	if (_UseSize < iSize)
		return 0;

	if (iSize <= 0)//음수리턴
		return 0;

	memcpy(chpDest, _Start + _Front, iSize);//원하는 사이즈 만큼 데이터를 뽑아오고

	return iSize;
};

int	CSerealBuffer::PutData(char* chpSrc, int iSize)
{
	//버퍼 최대크기에서 쓰기버퍼 사이즈를 빼면 사용가능한 사이즈가 나온다.
	//그 크기가 넣으려는 사이즈 보다 작다면 넣지 못하게 처리한다.
	if (_BufferSize - _Rear < iSize)
		return 0;

	if (iSize <= 0)//음수리턴
		return 0;

	memcpy(_Start + _Rear, chpSrc, iSize);//버퍼에 자리가 남아 있으므로 남은 공간에 데이터를 넣는다.
	_Rear += iSize;//쓰기버퍼를 쓴만큼 이동시키고
	_UseSize += iSize;//사용사이즈도 증가시켜준다.

	return iSize;
};