//#pragma once
//#include "SerializingBuffer.h"
//#include <string>
//
//	CMemoryPool<CSerealBuffer> g_PacketObjectPool;
//	CSerealBuffer::CSerealBuffer() : begin(new char[PacketSize::DefaultSize]), end(begin + PacketSize::DefaultSize), writePointer(begin), readPointer(begin), bufferSize(PacketSize::DefaultSize) {}
//
//	CSerealBuffer::CSerealBuffer(int bufferSize) : begin(new char[bufferSize]), end(begin + bufferSize), writePointer(begin), readPointer(begin), bufferSize(bufferSize) {}
//
//	CSerealBuffer::~CSerealBuffer()
//	{
//		Release();
//	}
//
//	void CSerealBuffer::Release()
//	{
//		delete[] begin;
//		writePointer = begin = end = readPointer = nullptr;
//	}
//
//	void CSerealBuffer::Clear()
//	{
//		writePointer = readPointer = begin;
//	}
//
//	int CSerealBuffer::GetBufferSize()
//	{
//		return writePointer - readPointer;
//	}
//
//	char* CSerealBuffer::GetWritePtr()
//	{
//		return writePointer;
//	}
//
//	char* CSerealBuffer::GetReadPtr()
//	{
//		return readPointer;
//	}
//	int CSerealBuffer::MoveWritePtr(int size)
//	{
//		if (writePointer + size >= end)
//		{
//			writePointer = end;
//			return size - (end - writePointer);
//		}
//		writePointer += size;
//		return size;
//	}
//
//	int CSerealBuffer::MoveReadPtr(int size)
//	{
//		if (readPointer + size >= end)
//		{
//			readPointer = end;
//			return size - (end - readPointer);
//		}
//		readPointer += size;
//		return size;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator=(const CSerealBuffer& other)
//	{
//		this->bufferSize = other.bufferSize;
//		begin = new char[bufferSize];
//		end = begin + bufferSize;
//		memcpy_s(begin, other.writePointer - other.readPointer, other.readPointer, other.writePointer - other.readPointer);
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(unsigned char& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		value = *readPointer;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(char& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		value = *readPointer;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(unsigned short& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		unsigned short* tempPtr = (unsigned short*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(short& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		short* tempPtr = (short*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(int& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		int* tempPtr = (int*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//	CSerealBuffer& CSerealBuffer::operator>>(unsigned int& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		unsigned int* tempPtr = (unsigned int*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//	CSerealBuffer& CSerealBuffer::operator>>(unsigned long& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		unsigned long* tempPtr = (unsigned long*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//	CSerealBuffer& CSerealBuffer::operator>>(long& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		long* tempPtr = (long*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(float& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		float* tempPtr = (float*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(double& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		double* tempPtr = (double*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator>>(__int64& value)
//	{
//		if (readPointer + sizeof(value) - 1 >= end)
//		{
//			//읽기 불가능
//			return *this;
//		}
//		__int64* tempPtr = (__int64*)readPointer;
//		value = *tempPtr;
//		MoveReadPtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(unsigned char value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		*writePointer = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(char value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		*writePointer = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(unsigned short value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		unsigned short* tempPtr = (unsigned short*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(short value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		short* tempPtr = (short*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(int value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		int* tempPtr = (int*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//	CSerealBuffer& CSerealBuffer::operator<<(unsigned int value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		unsigned int* tempPtr = (unsigned int*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//	CSerealBuffer& CSerealBuffer::operator<<(unsigned long value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		unsigned long* tempPtr = (unsigned long*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//	CSerealBuffer& CSerealBuffer::operator<<(long value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		long* tempPtr = (long*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(float value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		float* tempPtr = (float*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(__int64 value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		__int64* tempPtr = (__int64*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
//	CSerealBuffer& CSerealBuffer::operator<<(double value)
//	{
//		if (writePointer + (sizeof(value) - 1) >= end)
//		{
//			//저장이 불가능한경우.
//			return *this;
//		}
//		double* tempPtr = (double*)writePointer;
//		*tempPtr = value;
//		MoveWritePtr(sizeof(value));
//		return *this;
//	}
//
