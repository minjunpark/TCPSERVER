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
		Node* _node;//���ͷ����� ��尪
	public:
		iterator(Node* node = nullptr)
		{
			//���ڷ� ���� Node �����͸� ����
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
		{//���� ���ٸ� ����
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
		//���� ������ ���� ���� ����
		_head._Next = &_tail;
		_tail._Prev = &_head;
	};
	~CList()
	{
		clear();//��絥���� Ŭ����
	};

	iterator begin()
	{
		//ù��° ������ ��带 ����Ű�� ���ͷ����� ����
		iterator iter(_head._Next);
		return iter;
	}
	iterator end()
	{
		//Tail ��带 ����Ű��(�����Ͱ� ���� ��¥ ���� �� ���) ���ͷ����͸� ����
		//�Ǵ� ������ ������ �� �ִ� ���ͷ����͸� ����
		iterator iter(&_tail);
		return iter;
	}

	void push_front(T data)
	{
		Node* pNode = new Node();//����� ����
		pNode->_data = data;
		pNode->_Prev = &_head;//����� ���� ���� ��´�.
		pNode->_Next = _head._Next;//����� �ڸ� �� ����� ���� ���� ��´�.
		_head._Next = pNode;//����� �ؽ�Ʈ�� ���� ���� ��´�.
		pNode->_Next->_Prev = pNode;//���� ����Ǵ����� �ڸ� ������ �����Ѵ�.

		++_size;//����Ʈ���� +1
	};
	void push_back(T data)
	{
		Node* pNode = new Node();
		pNode->_data = data;
		pNode->_Next = &_tail;//���� ����� �ڸ� ���Ϸ� ��´�.
		pNode->_Prev = _tail._Prev;//���� ����� ���� ������ ���� ���̾��� �����ͷ� ��´�.
		_tail._Prev = pNode;//������ ���� ������� ��´�.
		pNode->_Prev->_Next = pNode;//���� ���� ��ġ�� ���� ���� ��´�.
		++_size;//����Ʈ���� +1
	};
	void pop_front()
	{//��� ���� ��� ����
		if (!empty()) {//���� ���ٸ�
			return;
		}
		Node* pNode;
		T _data;
		pNode = _head._Next;//�����带 ����� �������� ��´�.
		_data = pNode->_data;//�������� �����͸� �����´�.
		_head._Next = pNode->_Next;//����� ������ ����� �������� �ѱ��.
		pNode->_Next->_Prev = _head;
		delete pNode;
		--_size;
	};
	void pop_back()
	{//�޳�� ����
		if (!empty()) {//���̾��ٸ�?
			return;
		}
		Node* pNode;
		T _data;
		pNode = _tail._Prev;
		_data = pNode->_data;
		pNode->_Prev->_Next = &_tail;
		_tail._Prev = pNode->_Prev;
		delete pNode;
		--_size; //����Ʈ ���� - 1
	};

	void clear()
	{
		while (empty()) {
			Node* pNode;
			pNode = _head._Next;
			_head._Next = pNode->_Next;
			--_size;//������ ���̱�
			delete pNode;//��� ������ ����
		}
		_head._Next = &_tail;//��� ���� �����ʱ�ȭ
		_tail._Prev = &_head;
	};

	int size()
	{//������ ũ�� ��ȯ
		return _size;
	};

	bool empty()
	{
		if (_head._Next != &_tail) {//�������� ������ �ƴ϶��
			return true;
		}
		return false;
	};

	void insert(const iterator iter, T _data)
	{
		if (empty()) {//���̾��ٸ�?
			push_front(_data);//0�̹Ƿ� �׳� ���� ���ϻ��̿� ���� �־������.
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
		backup = ++iter;//�����ϱ��� ��ĭ�ڿ� ������� �����Ѵ�.		

		pNode->_Prev->_Next = pNode->_Next;//�����Ǵ� ����� ������ ���� ���� �Ĺ����� �����
		pNode->_Next->_Prev = pNode->_Prev;//�����Ǵ� ����� �Ĺ��� ���� ���� �������� ���� �����Ѵ�.

		delete pNode;//���� ���ͷ����Ͱ� ����Ű�� �����͸� �����Ѵ�.
		--_size;//������ ���� ����Ʈ ũ�⿡�� �����Ѵ�.

		return backup;//�������� ���� ������ ���� 1ĭ �Ĺ��� ���� ���Ͻ����ش�.
	};
	//- ���ͷ������� �� ��带 ����.
	//- �׸��� ���� ����� ���� ��带 ī��Ű�� ���ͷ����� ����

	void remove(T Data)
	{
		CList<int>::iterator iter;
		for (iter = begin(); iter != end();)
		{
			if (*iter == Data) //���� ��ġ�Ѵٸ�?
			{
				iter = erase(iter);
			}
			else//�������� �ʴ´ٸ� ���� ���ͷ����ͷ� �̵�
			{
				++iter;
			}
		}
	}

	//void printIntTest() 
	//{//�׽�Ʈ�� ����Ʈ
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