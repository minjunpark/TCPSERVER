//#pragma once
//
//#define MSS 1460
//#include "CMemoryPool.hpp"
//
//	class CSerealBuffer
//	{
//	public:
//		enum PacketSize
//		{
//			DefaultSize = MSS
//		};
//		CSerealBuffer();
//		CSerealBuffer(int bufferSize);
//
//		virtual ~CSerealBuffer();
//		void Release();
//
//		void Clear();
//		int GetBufferSize();
//		char* GetReadPtr();
//		char* GetWritePtr();
//
//		int MoveWritePtr(int size);
//		int MoveReadPtr(int size);
//
//		//template<typename T>
//		//CSerealBuffer& operator<<(T value)
//		//{
//		//	if (writePointer + (sizeof(value) - 1) >= end)
//		//	{
//		//		throw std::exception("need more memory");
//		//	}
//		//	T* tempPtr = (T*)writePointer;
//		//	*tempPtr = value;
//		//	MoveWritePtr(sizeof(value));
//		//	return *this;
//		//}
//		//template<typename T>
//		//CSerealBuffer& operator>>(T& value)
//		//{
//		//	if (readPointer + sizeof(value) - 1 >= end)
//		//	{
//		//		throw std::exception("cant read");
//		//	}
//		//	T* tempPtr = (T*)readPointer;
//		//	value = *tempPtr;
//		//	MoveReadPtr(sizeof(value));
//		//	return *this;
//		//}
//		
//		CSerealBuffer& operator=(const CSerealBuffer& other);
//		CSerealBuffer& operator>>(unsigned char& value);
//		CSerealBuffer& operator>>(char& value);
//
//		CSerealBuffer& operator>>(unsigned short& value);
//		CSerealBuffer& operator>>(short& value);
//
//		CSerealBuffer& operator>>(unsigned int& value);
//		CSerealBuffer& operator>>(int& value);
//		CSerealBuffer& operator>>(long& value);
//		CSerealBuffer& operator>>(unsigned long& value);
//		CSerealBuffer& operator>>(float& value);
//		CSerealBuffer& operator>>(double& value);
//		CSerealBuffer& operator>>(__int64& value);
//		CSerealBuffer& operator<<(unsigned char value);
//		CSerealBuffer& operator<<(char value);
//
//		CSerealBuffer& operator<<(unsigned short value);
//		CSerealBuffer& operator<<(short value);
//		CSerealBuffer& operator<<(unsigned int value);
//
//		CSerealBuffer& operator<<(int value);
//		CSerealBuffer& operator<<(unsigned long value);
//		CSerealBuffer& operator<<(long value);
//		CSerealBuffer& operator<<(float value);
//		CSerealBuffer& operator<<(__int64 value);
//		CSerealBuffer& operator<<(double value);
//	protected:
//		char* begin;
//		char* end;
//		char* writePointer;
//		char* readPointer;
//
//		int bufferSize;
//	};
//	extern CMemoryPool<CSerealBuffer> g_PacketObjectPool;
