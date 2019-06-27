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
		//切割
		char* p{ _freeBegin };
		for (size_t i{ 1 }; i < splitSize; ++i, p += size)
			reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
		reinterpret_cast<Node*>(p)->next = nullptr;
		//重置战备池
		_freeBegin = p + size;
		return { result, reinterpret_cast<Node*>(p) };
	}

	void WukongMemoryPool::fill_chain(size_t size)
	{
		size_t lastSize{ static_cast<size_t>(_freeEnd - _freeBegin) };
		//生成新链
		if (lastSize < size)//战备池无法满足一个需求
		{
			if (lastSize) add_to_chain(reinterpret_cast<Node*>(_freeBegin), lastSize);//处理战备池剩余
			//填充战备池
			size_t askSize{ size * MAX_SPLIT_SIZE * 2 + align_size(_count >> 2) };//增长量		
			//这里可以减少askSize直到向系统索求成功，但是涸泽而渔明智吗？通常情况不明智，所有下方没那么做
			if (_freeBegin = reinterpret_cast<char*>(std::malloc(askSize)))//向系统申请内存成功
			{
				_freeEnd = _freeBegin + askSize;
				_count += askSize;
			}
			else//向系统失败，分割更大的内存链
			{
				size_t targetIndex{ search_index(size) + 1 };
				for (; targetIndex < CHAIN_LENTH; ++targetIndex)
					if (_pool[targetIndex])	break;
				if (targetIndex >= CHAIN_LENTH)  throw std::bad_alloc{};//找不到可分割的大内存链
				else
				{
					_freeBegin = reinterpret_cast<char*>(remove_from_chain((targetIndex + 1) * PER_CHAIN_SIZE));
					_freeEnd = _freeBegin + (targetIndex + 1) * PER_CHAIN_SIZE;
				}
			}
		}
		std::tie(_pool[search_index(size)], _poolEnd[search_index(size)]) = split_free_pool(size);//切割战备池，挂到对应内存链
	}

	size_t WukongMemoryPool::search_index(size_t size)
	{
		return (size >> 2) - 1;
	}

	size_t WukongMemoryPool::align_size(size_t size)
	{
		//区分64位和32位。64位时指针大小为8byte，_pool[0]为4byte无法实现嵌入式指针，固64位弃用_pool[0]
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
		//对应内存链为空则填充
		if (!_pool[index]) fill_chain(alSize);
		return remove_from_chain(alSize);
	}

	void WukongMemoryPool::deallocate(void* oldP, size_t size)
	{
		add_to_chain(static_cast<Node*>(oldP), align_size(size));
	}
}
