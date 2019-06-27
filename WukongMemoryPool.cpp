#include "WukongMemoryPool.h"
#include <new>
#include <tuple>

namespace hzw
{
	std::pair<WukongMemoryPool::Node*, WukongMemoryPool::Node*> WukongMemoryPool::split_free_pool(size_t size)
	{
		Node* result{ reinterpret_cast<Node*>(_freeBegin) };
		size_t splitSize{ (_freeEnd - _freeBegin) / size };
		splitSize = splitSize < MAX_SPLIT_SIZE ? splitSize : MAX_SPLIT_SIZE;
		//�и�
		char* p{ _freeBegin };
		for (size_t i{ 1 }; i < splitSize; ++i, p += size)
			reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
		reinterpret_cast<Node*>(p)->next = nullptr;
		//����ս����
		_freeBegin = p + size;
		return { result, reinterpret_cast<Node*>(p) };
	}

	void WukongMemoryPool::fill_chain(size_t size)
	{
		size_t lastSize{ static_cast<size_t>(_freeEnd - _freeBegin) };
		//��������
		if (lastSize < size)//ս�����޷�����һ������
		{
			if (lastSize) add_to_chain(reinterpret_cast<Node*>(_freeBegin), lastSize);//����ս����ʣ��
			//���ս����
			size_t askSize{ size * MAX_SPLIT_SIZE * 2 + align_size(_count >> 2) };//������		
			//������Լ���askSizeֱ����ϵͳ����ɹ������Ǻ������������ͨ����������ǣ������·�û��ô��
			if (_freeBegin = reinterpret_cast<char*>(std::malloc(askSize)))//��ϵͳ�����ڴ�ɹ�
			{
				_freeEnd = _freeBegin + askSize;
				_count += askSize;
			}
			else//��ϵͳʧ�ܣ��ָ������ڴ���
			{
				size_t targetIndex{ search_index(size) + 1 };
				for (; targetIndex < CHAIN_LENTH; ++targetIndex)
					if (_pool[targetIndex])	break;
				if (targetIndex >= CHAIN_LENTH)  throw std::bad_alloc{};//�Ҳ����ɷָ�Ĵ��ڴ���
				else
				{
					_freeBegin = reinterpret_cast<char*>(remove_from_chain((targetIndex + 1) * PER_CHAIN_SIZE));
					_freeEnd = _freeBegin + (targetIndex + 1) * PER_CHAIN_SIZE;
				}
			}
		}
		std::tie(_pool[search_index(size)], _poolEnd[search_index(size)]) = split_free_pool(size);//�и�ս���أ��ҵ���Ӧ�ڴ���
	}

	size_t WukongMemoryPool::search_index(size_t size)
	{
		return (size >> 2) - 1;
	}

	size_t WukongMemoryPool::align_size(size_t size)
	{
		//����64λ��32λ��64λʱָ���СΪ8byte��_pool[0]Ϊ4byte�޷�ʵ��Ƕ��ʽָ�룬��64λ����_pool[0]
		return _align_size(std::bool_constant <sizeof(void*) == 8>{}, size);
	}

	size_t WukongMemoryPool::_align_size(std::true_type, size_t size)
	{
		return size <= 8 ? 8 : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1));
	}

	size_t WukongMemoryPool::_align_size(std::false_type, size_t size)
	{
		return size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1);
	}

	void WukongMemoryPool::add_to_chain(Node* p, size_t size)
	{
		Node*& end{ _poolEnd[search_index(size)] };
		p->next = nullptr;
		if (end)
		{
			end->next = p;
			end = p;
		}
		else _pool[search_index(size)] = end = p;
	}

	void* WukongMemoryPool::remove_from_chain(size_t size)
	{
		Node*& chainHead{ _pool[search_index(size)] };
		void* result{ chainHead };
		chainHead = chainHead->next;
		if (!chainHead) _poolEnd[search_index(size)] = nullptr;
		return result;
	}

	WukongMemoryPool::WukongMemoryPool() :
		_pool{ nullptr }, _poolEnd{ nullptr }, _freeBegin{ nullptr }, _freeEnd{ nullptr }, _count{ 0 }
	{
	}

	 void* WukongMemoryPool::allocate(size_t size)
	{
		size_t alSize{ align_size(size) };
		size_t index{ search_index(alSize) };
		//��Ӧ�ڴ���Ϊ�������
		if (!_pool[index]) fill_chain(alSize);
		return remove_from_chain(alSize);
	}

	void WukongMemoryPool::deallocate(void* oldP, size_t size)
	{
		add_to_chain(static_cast<Node*>(oldP), align_size(size));
	}
}
