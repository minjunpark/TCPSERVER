#pragma once

template <typename T>
class CList
{
public:
	struct Node
	{
		T _data;
		Node* _Prev;
		Node* _Next;
	};

	//iterator START
	class iterator
	{

	private:
		Node* _node;//이터레이터 노드값
	public:
		iterator(Node* node = nullptr)
		{
			//인자로 들어온 Node 포인터를 저장
			_node = node;
		}

		iterator operator ++(int a)
		{
			_node = _node->_Next;
			return *this;
		}

		iterator& operator++()
		{
			_node = _node->_Next;
			return *this;
		}

		iterator operator --(int)
		{
			_node = _node->_Prev;
			return *this;
		}

		iterator& operator--()
		{
			_node = _node->_Prev;
			return *this;
		}

		T& operator *()
		{
			return _node->_data;
		}

		Node* operator &()
		{
			return _node;
		}

		bool operator ==(const iterator& other)
		{//값이 같다면 리턴
			if (_node == other._node) {
				return true;
			}
			return false;
		}

		bool operator !=(const iterator& other)
		{
			if (_node != other._node) {
				return true;
			}
			return false;
		}
	};
	//iterator END
//CLIST START
public:
	CList()
	{
		//최초 생성시 헤드와 꼬리 연결
		_head._Next = &_tail;
		_tail._Prev = &_head;
	};
	~CList()
	{
		clear();//모든데이터 클리어
	};

	iterator begin()
	{
		//첫번째 데이터 노드를 가리키는 이터레이터 리턴
		iterator iter(_head._Next);
		return iter;
	}
	iterator end()
	{
		//Tail 노드를 가리키는(데이터가 없는 진짜 더미 끝 노드) 이터레이터를 리턴
		//또는 끝으로 인지할 수 있는 이터레이터를 리턴
		iterator iter(&_tail);
		return iter;
	}

	void push_front(T data)
	{
		Node* pNode = new Node();//새노드 생성
		pNode->_data = data;
		pNode->_Prev = &_head;//노드의 앞을 헤드로 잡는다.
		pNode->_Next = _head._Next;//노드의 뒤를 전 헤드의 다음 노드로 잡는다.
		_head._Next = pNode;//헤드의 넥스트를 현재 노드로 잡는다.
		pNode->_Next->_Prev = pNode;//현재 노드의다음의 뒤를 나한테 연결한다.

		++_size;//리스트길이 +1
	};
	void push_back(T data)
	{
		Node* pNode = new Node();
		pNode->_data = data;
		pNode->_Next = &_tail;//현재 노드의 뒤를 테일로 잡는다.
		pNode->_Prev = _tail._Prev;//현재 노드의 앞을 테일의 원래 앞이었던 데이터로 잡는다.
		_tail._Prev = pNode;//테일의 앞을 현재노드로 잡는다.
		pNode->_Prev->_Next = pNode;//현재 나의 위치의 앞을 나로 잡는다.
		++_size;//리스트길이 +1
	};
	void pop_front()
	{//헤드 다음 노드 삭제
		if (!empty()) {//값이 없다면
			return;
		}
		Node* pNode;
		T _data;
		pNode = _head._Next;//현재노드를 헤드의 다음노드로 잡는다.
		_data = pNode->_data;//현재노드의 데이터를 가져온다.
		_head._Next = pNode->_Next;//헤드의 다음을 노드의 다음노드로 넘긴다.
		pNode->_Next->_Prev = _head;
		delete pNode;
		--_size;
	};
	void pop_back()
	{//뒷노드 삭제
		if (!empty()) {//값이없다면?
			return;
		}
		Node* pNode;
		T _data;
		pNode = _tail._Prev;
		_data = pNode->_data;
		pNode->_Prev->_Next = &_tail;
		_tail._Prev = pNode->_Prev;
		delete pNode;
		--_size; //리스트 길이 - 1
	};

	void clear()
	{
		while (empty()) {
			Node* pNode;
			pNode = _head._Next;
			_head._Next = pNode->_Next;
			--_size;//사이즈 줄이기
			delete pNode;//모든 사이즈 제거
		}
		_head._Next = &_tail;//헤드 테일 연결초기화
		_tail._Prev = &_head;
	};

	int size()
	{//사이즈 크기 반환
		return _size;
	};

	bool empty()
	{
		if (_head._Next != &_tail) {//헤드다음이 테일이 아니라면
			return true;
		}
		return false;
	};

	void insert(const iterator iter, T _data)
	{
		if (empty()) {//값이없다면?
			push_front(_data);//0이므로 그냥 헤드랑 테일사이에 값을 넣어버린다.
			return;
		}
		Node* pNode;
		Node* iterNode;
		iterNode = &iter;
		pNode->_data = _data;

		pNode->_Next = iterNode->_Prev->_Next;
		iterNode->_Prev->_Next = pNode;
		pNode->_Prev = iterNode->_Prev;
		iterNode->_Prev = pNode;
	}

	iterator erase(iterator iter)
	{
		Node* pNode;
		pNode = &iter;

		iterator backup;
		backup = ++iter;//삭제하기전 한칸뒤에 백업본을 저장한다.		

		pNode->_Prev->_Next = pNode->_Next;//삭제되는 노드의 전방의 앞을 나의 후방으로 만들고
		pNode->_Next->_Prev = pNode->_Prev;//삭제되는 노드의 후방의 앞을 나의 전방으로 만들어서 연결한다.

		delete pNode;//그후 이터레이터가 가르키는 데이터를 삭제한다.
		--_size;//삭제된 값을 리스트 크기에서 삭제한다.

		return backup;//삭제한후 내가 지웠던 값의 1칸 후방의 값을 리턴시켜준다.
	};
	//- 이터레이터의 그 노드를 지움.
	//- 그리고 지운 노드의 다음 노드를 카리키는 이터레이터 리턴

	void remove(T Data)
	{
		CList<int>::iterator iter;
		for (iter = begin(); iter != end();)
		{
			if (*iter == Data) //값이 일치한다면?
			{
				iter = erase(iter);
			}
			else//삭제되지 않는다면 다음 이터레이터로 이동
			{
				++iter;
			}
		}
	}

	//void printIntTest() 
	//{//테스트용 프린트
	//	CList<T>::iterator iter;
	//	printf("SIZE [ %d ]\n", _size);
	//	for (iter = begin(); iter != end(); ++iter)
	//	{
	//		printf("[ %d ]", *iter);
	//	}
	//}

private:
	int _size = 0;
	Node _head;
	Node _tail;
	//CLIST END
};