#include "MemoryPool.h"

namespace hzw
{
	std::mutex MemoryPool::_mutexs[MemoryPool::ChainLength];
	MemoryPool::Node* MemoryPool::_pool[MemoryPool::ChainLength]{ nullptr };
	std::mutex MemoryPool::_freeMutex;
	char * MemoryPool::_freeBegin = nullptr;
	char * MemoryPool::_freeEnd = nullptr;
	size_t MemoryPool::_count = 0;

	MemoryPool::Node * MemoryPool::splitFreePool(size_t size)
	{
		Node* result{ reinterpret_cast<Node*>(_freeBegin) };
		size_t splitSize{ (_freeEnd - _freeBegin) / size };
		splitSize = splitSize < MaxSplitSize ? splitSize : MaxSplitSize;
		//切割
		char* p{ _freeBegin };
		for (size_t i{ 1 }; i < splitSize; ++i, p += size)
			reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
		reinterpret_cast<Node*>(p)->next = nullptr;
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
			if (lastSize)		addToChain(reinterpret_cast<Node*>(_freeBegin), lastSize);//处理战备池剩余
			//填充战备池
			size_t askSize{ size * MaxSplitSize * 2 + alignSize(_count >> 2) };//增长量		
			//这里可以减少askSize直到向系统索求成功，但是涸泽而渔明智吗？通常情况不明智，所有下方没那么做
			if (_freeBegin = reinterpret_cast<char*>(std::malloc(askSize)))//向系统申请内存成功
			{
				_freeEnd = _freeBegin + askSize;
				_count += askSize;
			}
			else//向系统失败，分割更大的内存链
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
		_pool[searchIndex(size)] = splitFreePool(size);//切割战备池，挂到对应内存链
	}

	void * MemoryPool::_allocate(size_t size)
	{
		size_t index{ searchIndex(size) };
		std::lock_guard<std::mutex> poolMutex{ _mutexs[index] };
		if (!_pool[index])//对应内存链为空则填充
		{
			if (!_freeMutex.try_lock())//战备池上锁失败则放弃对应内存链锁
			{
				_mutexs[index].unlock();//此操作可能导致多个线程进入同步块，下发使用双重检测解决
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
