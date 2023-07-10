/*---------------------------------------------------------------

	procademy MemoryPool.

	메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

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
	int m_iCapacity;//메모리풀 전체 개수
	int m_iUseCount;//사용중인 사이즈
	bool m_iPlacementNew;// Alloc 시 생성자 / Free 시 파괴자 호출 여부
	int m_iBufferPointer;//생성될때마다 세팅되는 고유값

	struct Node
	{
		int OVER_GUARD = 0;
		DATA data;
		int UNDER_GUARD = 0;
		Node* next = nullptr;
	};

public:
	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
	Node* _FreeNode;//얘가 헤드임

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(int iBlockNum, bool bPlacementNew = false)
	{
		m_iCapacity = 0;//전체 총개수
		m_iUseCount = 0;//사용 중인 개수
		m_iPlacementNew = bPlacementNew;//Alloc 시 생성자 / Free 시 파괴자 호출 여부
		m_iBufferPointer = (int)this;//생성될때 세팅되는 this포인터를 고유값으로 한다.

		if (iBlockNum == 0)//0이라면 더 진행할 이유가 없음
			return;

		for (int iMemory = 0; iMemory < iBlockNum; iMemory++)//들어온횟수만큼 돌면서 노드생성
		{
			Node* NewNode = (Node*)malloc(sizeof(Node));//생성자를 호출하지 않기위해 malloc으로 세팅

			if (m_iPlacementNew == false) //Alloc시 생성자 파괴자를 호출하지 않을거라면
				DATA* data = new(&(NewNode->data)) DATA;//생성자를 미리 세팅해둔다.

			NewNode->OVER_GUARD = m_iBufferPointer;//고유 포인터값을 노드에 세팅한다.
			NewNode->UNDER_GUARD = m_iBufferPointer;//고유 포인터값을 노드에 세팅한다.

			NewNode->next = _FreeNode;//스택
			_FreeNode = NewNode;//쌓기

			m_iCapacity++;//만든 노드 개수 증가시키기
		}
	};

	virtual	~CMemoryPool()
	{
		if (m_iCapacity == 0)//0개라면 모두제거 된것이므로 소멸자를 더 진행할 필요가 없다.
			return;

		Node* _Node = _FreeNode;
		while (_Node != nullptr)//자기자신이 _FreeNode노드가 된다면 모든 노드가 제거된것이다.
		{
			Node* tmpNode = _Node;//노드 임시세팅
			if (m_iPlacementNew == false)//소멸자를 Free에서 호출하지 않게 만들었기때문에
				tmpNode->data.~DATA();//객체의 소멸자를 호출시키면서
			free(tmpNode);//사용하는 실제 메모리를 free상태로 바꿔준다.
			_Node = _Node->next;//다음노드로 넘어간다.
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		if ((m_iCapacity - m_iUseCount) > 0)//사용할수 있는 메모리 오브젝트가 있다면
		{
			Node* OldNode = _FreeNode;
			DATA* Data;

			if (m_iPlacementNew == true)//Alloc시점에 생성자를 호출해주길 바라는상태
				Data = new (&(OldNode->data)) DATA;//data포인터 자체를 palcement_new로 위치로잡아서 생성해준다.
			else
				Data = &(OldNode->data);//이미 데이터 생성자가 세팅된 경우기 때문에 스택에서 뽑은값 바로리턴

			_FreeNode = OldNode->next;//스택에서 노드 제거

			m_iUseCount++;//총사용량을 올려준다.

			return Data;//할당된 메모리를 리턴해준다.
		}
		else//사용 할 수 있는 메모리 오브젝트가 없다면 만들어서 리턴해준다.
		{
			Node* NewNode = (Node*)malloc(sizeof(Node));//생성자를 호출하지 않기위해 malloc으로 세팅

			DATA* Data = new (&(NewNode->data)) DATA;//생성자를 호출하면서 데이터 세팅

			NewNode->OVER_GUARD = m_iBufferPointer;
			NewNode->UNDER_GUARD = m_iBufferPointer;

			NewNode->next = _FreeNode;//스택에 노드 추가
			_FreeNode = NewNode;

			_FreeNode = NewNode->next;//스택에서 노드 제거

			m_iUseCount++;//사용량 올려주고
			m_iCapacity++;//노드 총 개수를 증가시켜준다.

			return Data;
		}
	};



	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* pData)
	{
		if (pData == nullptr)
			return false;

		//같은 포인터 변수 중복 입력으로인한 실수방지
		if ((&_FreeNode->data) == pData)
			return false;

		if (m_iUseCount > 0)//현재 사용중인 값이 있다면
		{
			//Alloc시 준값은 Node 가아니라 Node안의 data부분이므로 Node가장위부터 data까지를 계산한다.
			//그후 계산한 값을 들어온 위치에서 타고 올라가 Node 위치를 정확히 세팅한다.
			Node* ReData = (Node*)((char*)pData - offsetof(Node, data));

			if (ReData->OVER_GUARD != m_iBufferPointer || ReData->UNDER_GUARD != m_iBufferPointer)
				return false;//다른 오브젝트의 포인터가 들어왔거나 데이터가 오염됬다는 뜻이다.
			//로그 남기거나 exception 일으키기가 필요한데 우선 false만 처리한다.

			if (m_iPlacementNew == true)//소멸자를 호출해야하는 상황이라면
				ReData->data.~DATA();//소멸자를 호출해준다.

			ReData->next = _FreeNode;//돌아온 메모리를
			_FreeNode = ReData;//스택에 다시 쌓는다.

			m_iUseCount--;//사용중인 카운터를 줄인다.
			return true;
		}

		return false;//현재 사용중 카운터가 없는 상황인데 들어온 경우 이므로 false를 리턴한다.
	};

	void print()
	{
		printf("m_iCapacity %d\n", m_iCapacity);
		printf("m_iUseCount %d\n", m_iUseCount);
		printf("m_iPlacementNew %d\n", m_iPlacementNew);
	};


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return m_iCapacity; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return m_iUseCount; }


};
//}
#endif