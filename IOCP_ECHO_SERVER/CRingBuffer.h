#pragma once
#include <process.h>
#include <Windows.h>
class CRingBuffer
{


public:
	CRingBuffer(void);//����� ��������� ������
	CRingBuffer(int iBufferSize);//����� ��������� ������
	~CRingBuffer(void);//�Ҹ���

	//ũ���Լ�
	int GetBufferSize(void);//���� ������ �ִ� ũ�⸦ �����´�.
	int GetUseSize(void);//���� ������� �뷮 ���
	int GetFreeSize(void);//���� ���ۿ� ���� �뷮 ���
	int DirectEnqueueSize(void);//���ۿ� �ѹ��� ������ �ִ� ������
	int DirectDequeueSize(void);//���ۿ� �ѹ��� ���� �ִ� ������

	//�����۵���
	int Enqueue(char* chpData, int iSize);//�����ŭ ���ۿ� �ֱ�
	int Dequeue(char* chpDest, int iSize);//�����ŭ ���ۿ� ����
	int Peek(char* chpDest, int iSize);//�����ŭ ���ۿ� �����ϱ�

	//���� �б������� �̵���Ű��
	int MoveWrite(int iSize);//���� �����͸� ũ�⸸ŭ �̵���Ų��.
	int MoveRead(int iSize);//�б� �����͸� ũ�⸸ŭ �̵���Ų��.
	char* GetStartBufferPtr(void);//�б������͸� ����
	char* GetReadBufferPtr(void);//�б������͸� ����
	char* GetWriteBufferPtr(void);//���������͸� ����

	void ClearBuffer(void);//���۸� �ʱ�ȭ�ϱ�
	bool Resize(int size);//������ ����� �ٲٴ� �Լ�(���߿� ����غ�����)
	void SH_Lock();
	void SH_UnLock();
	void EX_Lock();
	void EX_UnLock();

private:
	char* _Start;//���� ��������
	int _BufferSize;//�� ���� ũ��
	int _Front;//�б� ������ ����� ���Ѱ�
	int _Rear;//���� ������ ����� ���Ѱ�
	SRWLOCK _Lock;

	enum { VOID_VALUE = 8 };


};