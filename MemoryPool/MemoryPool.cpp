#include "MemoryPool.h"

namespace hzw
{
	std::mutex MemoryPool::_mutexs[MemoryPool::ChainLength];
	MemoryPool::Node *volatile MemoryPool::_pool[MemoryPool::ChainLength]{ nullptr };
	std::mutex MemoryPool::_freeMutex;
	char *volatile MemoryPool::_freeBegin = nullptr;
	char *volatile MemoryPool::_freeEnd = nullptr;
	volatile size_t MemoryPool::_count = 0;

	MemoryPool::Node * MemoryPool::splitFreePool(size_t size)
	{
		Node *result{ reinterpret_cast<Node *>(_freeBegin) };
		size_t splitSize{ (_freeEnd - _freeBegin) / size };
		splitSize = splitSize < MaxSplitSize ? splitSize : MaxSplitSize;
		//切割
		char *p{ _freeBegin };
		for (size_t i{ 1 }; i < splitSize; ++i, p += size)
			reinterpret_cast<Node *>(p)->next = reinterpret_cast<Node *>(p + size);
		reinterpret_cast<Node *>(p)->next = nullptr;
		//重置战备池
		_freeBegin = p + size;
		return result;
	}

	void MemoryPool::fillChain(size_t size)
	{
		size_t lastSize{ static_cast<size_t>(_freeEnd - _freeBegin) };
		//生成新链
		if (lastSize < size)//战备池无法满足一个需求
		{
			if (lastSize)		addToChain(reinterpret_cast<Node *>(_freeBegin), lastSize);//处理剩余
			//填充战备池
			size_t askSize{ size * AskPerSize + alignSize(_count / 4) };//增长量		
			if (_freeBegin = reinterpret_cast<char *>(std::malloc(askSize)))//向系统申请内存成功
			{
				_freeEnd = _freeBegin + askSize;
				_count += askSize;
			}
			else//向系统申请失败，分割大的内存链
			{
				size_t targetIndex{ searchIndex(size) + 1 };
				for (; targetIndex < ChainLength; ++targetIndex)
				{
					_mutexs[targetIndex].lock();
					if (_pool[targetIndex])	break;
					_mutexs[targetIndex].unlock();
				}
				if (targetIndex >= ChainLength)  throw std::bad_alloc{};//找不到可分割的大内存链
				else
				{
					_freeBegin = reinterpret_cast<char *>(removeFromChain(_pool[targetIndex]));
					_mutexs[targetIndex].unlock();
					_freeEnd = _freeBegin + (targetIndex + 1) * ChainPerSize;
				}
			}
		}
		_pool[searchIndex(size)] = splitFreePool(size);//加入新链
	}

	void * MemoryPool::_allocate(size_t size)
	{
		size_t index{ searchIndex(size) };
		std::lock_guard<std::mutex> poolMutex{ _mutexs[index] };
		if (!_pool[index])//对应内存链为空则填充
		{
			if (!_freeMutex.try_lock())//战备池上锁失败则放弃对应内存链锁
			{
				_mutexs[index].unlock();
				std::lock(_freeMutex, _mutexs[index]);
				if (_pool[index])		goto unFill;//双重检测
			}
			fillChain(size);
		unFill:
			_freeMutex.unlock();
		}
		return removeFromChain(_pool[index]);
	}
}
