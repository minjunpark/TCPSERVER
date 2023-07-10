/*---------------------------------------------------------------

	procademy MemoryPool.

	�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);


----------------------------------------------------------------*/
#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include <new.h>


//namespace procademy
//{
template <class DATA>
class CMemoryPool
{
private:
	int m_iCapacity;//�޸�Ǯ ��ü ����
	int m_iUseCount;//������� ������
	bool m_iPlacementNew;// Alloc �� ������ / Free �� �ı��� ȣ�� ����
	int m_iBufferPointer;//�����ɶ����� ���õǴ� ������

	struct Node
	{
		int OVER_GUARD = 0;
		DATA data;
		int UNDER_GUARD = 0;
		Node* next = nullptr;
	};

public:
	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.
	Node* _FreeNode;//�갡 �����

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(int iBlockNum, bool bPlacementNew = false)
	{
		m_iCapacity = 0;//��ü �Ѱ���
		m_iUseCount = 0;//��� ���� ����
		m_iPlacementNew = bPlacementNew;//Alloc �� ������ / Free �� �ı��� ȣ�� ����
		m_iBufferPointer = (int)this;//�����ɶ� ���õǴ� this�����͸� ���������� �Ѵ�.

		if (iBlockNum == 0)//0�̶�� �� ������ ������ ����
			return;

		for (int iMemory = 0; iMemory < iBlockNum; iMemory++)//����Ƚ����ŭ ���鼭 ������
		{
			Node* NewNode = (Node*)malloc(sizeof(Node));//�����ڸ� ȣ������ �ʱ����� malloc���� ����

			if (m_iPlacementNew == false) //Alloc�� ������ �ı��ڸ� ȣ������ �����Ŷ��
				DATA* data = new(&(NewNode->data)) DATA;//�����ڸ� �̸� �����صд�.

			NewNode->OVER_GUARD = m_iBufferPointer;//���� �����Ͱ��� ��忡 �����Ѵ�.
			NewNode->UNDER_GUARD = m_iBufferPointer;//���� �����Ͱ��� ��忡 �����Ѵ�.

			NewNode->next = _FreeNode;//����
			_FreeNode = NewNode;//�ױ�

			m_iCapacity++;//���� ��� ���� ������Ű��
		}
	};

	virtual	~CMemoryPool()
	{
		if (m_iCapacity == 0)//0����� ������� �Ȱ��̹Ƿ� �Ҹ��ڸ� �� ������ �ʿ䰡 ����.
			return;

		Node* _Node = _FreeNode;
		while (_Node != nullptr)//�ڱ��ڽ��� _FreeNode��尡 �ȴٸ� ��� ��尡 ���ŵȰ��̴�.
		{
			Node* tmpNode = _Node;//��� �ӽü���
			if (m_iPlacementNew == false)//�Ҹ��ڸ� Free���� ȣ������ �ʰ� ������⶧����
				tmpNode->data.~DATA();//��ü�� �Ҹ��ڸ� ȣ���Ű�鼭
			free(tmpNode);//����ϴ� ���� �޸𸮸� free���·� �ٲ��ش�.
			_Node = _Node->next;//�������� �Ѿ��.
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		if ((m_iCapacity - m_iUseCount) > 0)//����Ҽ� �ִ� �޸� ������Ʈ�� �ִٸ�
		{
			Node* OldNode = _FreeNode;
			DATA* Data;

			if (m_iPlacementNew == true)//Alloc������ �����ڸ� ȣ�����ֱ� �ٶ�»���
				Data = new (&(OldNode->data)) DATA;//data������ ��ü�� palcement_new�� ��ġ����Ƽ� �������ش�.
			else
				Data = &(OldNode->data);//�̹� ������ �����ڰ� ���õ� ���� ������ ���ÿ��� ������ �ٷθ���

			_FreeNode = OldNode->next;//���ÿ��� ��� ����

			m_iUseCount++;//�ѻ�뷮�� �÷��ش�.

			return Data;//�Ҵ�� �޸𸮸� �������ش�.
		}
		else//��� �� �� �ִ� �޸� ������Ʈ�� ���ٸ� ���� �������ش�.
		{
			Node* NewNode = (Node*)malloc(sizeof(Node));//�����ڸ� ȣ������ �ʱ����� malloc���� ����

			DATA* Data = new (&(NewNode->data)) DATA;//�����ڸ� ȣ���ϸ鼭 ������ ����

			NewNode->OVER_GUARD = m_iBufferPointer;
			NewNode->UNDER_GUARD = m_iBufferPointer;

			NewNode->next = _FreeNode;//���ÿ� ��� �߰�
			_FreeNode = NewNode;

			_FreeNode = NewNode->next;//���ÿ��� ��� ����

			m_iUseCount++;//��뷮 �÷��ְ�
			m_iCapacity++;//��� �� ������ ���������ش�.

			return Data;
		}
	};



	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* pData)
	{
		if (pData == nullptr)
			return false;

		//���� ������ ���� �ߺ� �Է��������� �Ǽ�����
		if ((&_FreeNode->data) == pData)
			return false;

		if (m_iUseCount > 0)//���� ������� ���� �ִٸ�
		{
			//Alloc�� �ذ��� Node ���ƴ϶� Node���� data�κ��̹Ƿ� Node���������� data������ ����Ѵ�.
			//���� ����� ���� ���� ��ġ���� Ÿ�� �ö� Node ��ġ�� ��Ȯ�� �����Ѵ�.
			Node* ReData = (Node*)((char*)pData - offsetof(Node, data));

			if (ReData->OVER_GUARD != m_iBufferPointer || ReData->UNDER_GUARD != m_iBufferPointer)
				return false;//�ٸ� ������Ʈ�� �����Ͱ� ���԰ų� �����Ͱ� ������ٴ� ���̴�.
			//�α� ����ų� exception ����Ű�Ⱑ �ʿ��ѵ� �켱 false�� ó���Ѵ�.

			if (m_iPlacementNew == true)//�Ҹ��ڸ� ȣ���ؾ��ϴ� ��Ȳ�̶��
				ReData->data.~DATA();//�Ҹ��ڸ� ȣ�����ش�.

			ReData->next = _FreeNode;//���ƿ� �޸𸮸�
			_FreeNode = ReData;//���ÿ� �ٽ� �״´�.

			m_iUseCount--;//������� ī���͸� ���δ�.
			return true;
		}

		return false;//���� ����� ī���Ͱ� ���� ��Ȳ�ε� ���� ��� �̹Ƿ� false�� �����Ѵ�.
	};

	void print()
	{
		printf("m_iCapacity %d\n", m_iCapacity);
		printf("m_iUseCount %d\n", m_iUseCount);
		printf("m_iPlacementNew %d\n", m_iPlacementNew);
	};


	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return m_iCapacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return m_iUseCount; }


};
//}
#endif