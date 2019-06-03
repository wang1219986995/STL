#include "MemoryPool.h"
#include <exception>

namespace hzw
{
	std::mutex MemoryPool::_mutexs[MemoryPool::ChainLength];
	MemoryPool::Node* MemoryPool::_pool[MemoryPool::ChainLength]{ nullptr };
	std::mutex MemoryPool::_freeMutex;
	char* MemoryPool::_freeBegin{ nullptr };
	char* MemoryPool::_freeEnd{ nullptr };
	size_t MemoryPool::_count{ 0 };

	MemoryPool::Node * MemoryPool::splitFreePool(size_t size)
	{
		Node* result{ reinterpret_cast<Node*>(_freeBegin) };
		size_t splitSize{ (_freeEnd - _freeBegin) / size };
		splitSize = splitSize < MaxSplitSize ? splitSize : MaxSplitSize;
		//�и�
		char* p{ _freeBegin };
		for (size_t i{ 1 }; i < splitSize; ++i, p += size)
			reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
		reinterpret_cast<Node*>(p)->next = nullptr;
		//����ս����
		_freeBegin = p + size;
		return result;
	}

	void MemoryPool::fillChain(size_t size)
	{
		size_t lastSize{ static_cast<size_t>(_freeEnd - _freeBegin) };
		//��������
		if (lastSize < size)//ս�����޷�����һ������
		{
			if (lastSize)		addToChain(reinterpret_cast<Node*>(_freeBegin), lastSize);//����ս����ʣ��
			//���ս����
			size_t askSize{ size * MaxSplitSize * 2 + alignSize(_count >> 2) };//������		
			//������Լ���askSizeֱ����ϵͳ����ɹ������Ǻ������������ͨ����������ǣ������·�û��ô��
			if (_freeBegin = reinterpret_cast<char*>(std::malloc(askSize)))//��ϵͳ�����ڴ�ɹ�
			{
				_freeEnd = _freeBegin + askSize;
				_count += askSize;
			}
			else//��ϵͳʧ�ܣ��ָ������ڴ���
			{
				size_t targetIndex{ searchIndex(size) + 1 };
				for (; targetIndex < ChainLength; ++targetIndex)
				{
					_mutexs[targetIndex].lock();
					if (_pool[targetIndex])	break;
					_mutexs[targetIndex].unlock();
				}
				if (targetIndex >= ChainLength)  throw std::bad_alloc{};//�Ҳ����ɷָ�Ĵ��ڴ���
				else
				{
					_freeBegin = reinterpret_cast<char *>(removeFromChain(_pool[targetIndex]));
					_mutexs[targetIndex].unlock();
					_freeEnd = _freeBegin + (targetIndex + 1) * ChainPerSize;
				}
			}
		}
		_pool[searchIndex(size)] = splitFreePool(size);//�и�ս���أ��ҵ���Ӧ�ڴ���
	}

	inline size_t MemoryPool::searchIndex(size_t size)
	{
		return (size + ChainPerSize - 1 >> 3) - 1;
	}

	inline size_t MemoryPool::alignSize(size_t size)
	{
		return (size + ChainPerSize - 1 & ~(ChainPerSize - 1));
	}

	inline void MemoryPool::addToChain(Node* p, size_t size)
	{
		size_t index{ searchIndex(size) };
		_mutexs[index].lock();
		Node*& chainHead{ _pool[index] };
		p->next = chainHead;
		chainHead = p;
		_mutexs[index].unlock();
	}

	inline void* MemoryPool::removeFromChain(Node*& chainHead)
	{
		void* result{ chainHead };
		chainHead = chainHead->next;
		return result;
	}

	inline void MemoryPool::_deallocate(Node* oldP, size_t size)
	{
		addToChain(oldP, size);
	}

	void* MemoryPool::allocate(size_t size)
	{
		return size > MaxSize ? ::operator new(size) : _allocate(alignSize(size));
	}

	void MemoryPool::deallocate(void* oldP, size_t size)
	{
		size > MaxSize ? ::operator delete(oldP) : _deallocate(reinterpret_cast<Node*>(oldP), alignSize(size));
	}

	void * MemoryPool::_allocate(size_t size)
	{
		size_t index{ searchIndex(size) };
		std::lock_guard<std::mutex> poolMutex{ _mutexs[index] };
		if (!_pool[index])//��Ӧ�ڴ���Ϊ�������
		{
			if (!_freeMutex.try_lock())//ս��������ʧ���������Ӧ�ڴ�����
			{
				_mutexs[index].unlock();//�˲������ܵ��¶���߳̽���ͬ���飬�·�ʹ��˫�ؼ����
				std::lock(_freeMutex, _mutexs[index]);
				if (_pool[index])		goto unFill;//˫�ؼ��
			}
			fillChain(size);
		unFill:
			_freeMutex.unlock();
		}
		return removeFromChain(_pool[index]);
	}
}
