#pragma once

class CRingBuffer
{
private:
	char* _Start;//���� ��������
	int _BufferSize;//�� ���� ũ��
	int _Front;//�б� ������ ����� ���Ѱ�
	int _Rear;//���� ������ ����� ���Ѱ�

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
	int MoveRear(int iSize);//���� �����͸� ũ�⸸ŭ �̵���Ų��.
	int MoveFront(int iSize);//�б� �����͸� ũ�⸸ŭ �̵���Ų��.
	char* GetFrontBufferPtr(void);//�б������͸� ����
	char* GetRearBufferPtr(void);//���������͸� ����
	
	void ClearBuffer(void);//���۸� �ʱ�ȭ�ϱ�
	bool Resize(int size);//������ ����� �ٲٴ� �Լ�(���߿� ����غ�����)

private:
	enum { VOID_VALUE = 8 };


};