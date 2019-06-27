#ifndef  WUKONGMEMORYPOOL_H
#define WUKONGMEMORYPOOL_H

#include <exception>
#include <vector>

/*
	悟空内存池
	简单粗暴，提供快速分配回收
	在运行时此内存池持有的内存不主动归还给系统，直到进程结束由系统回收
	简而言之 不具有动态收缩能力，但换来更快的速度
*/

namespace hzw
{
	//悟空内存池
	class WukongMemoryPool
	{
	public:
		struct Node;
		enum
		{
			CHAIN_LENTH = 32, PER_CHAIN_SIZE = 4, MAX_SPLIT_SIZE = 20,
			MAX_SIZE = CHAIN_LENTH * PER_CHAIN_SIZE
		};
		//内存池链的个数  //内存池粒度 //最大切割个数 //管理的最大内存 

		WukongMemoryPool() :
			_pool(CHAIN_LENTH, nullptr), _freeBegin{ nullptr }, _freeEnd{ nullptr }, _count{ 0 }
		{
		}

		WukongMemoryPool(const WukongMemoryPool&) = delete;

		WukongMemoryPool(WukongMemoryPool&&) = default;

		void* allocate(size_t size)
		{
			size_t alSize{ align_size(size) };
			size_t index{ search_index(alSize) };
			//对应内存链为空则填充
			if (!_pool[index]) fill_chain(alSize);
			return remove_from_chain(_pool[index]);
		}

		void deallocate(void* oldP, size_t size)
		{
			add_to_chain(static_cast<Node*>(oldP), align_size(size));
		}

	private:
		//功能：切割战备池后调整战备池大小
		//输入：对齐后内存需求大小
		//输出：切割完成后的内存链
		Node* split_free_pool(size_t size)
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
			return result;
		}

		//功能：填充内存链（填充内存来源：战备池，malloc，更大的内存链）
		//输入：对齐后内存需求大小
		void fill_chain(size_t size)
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
						_freeBegin = reinterpret_cast<char*>(remove_from_chain(_pool[targetIndex]));
						_freeEnd = _freeBegin + (targetIndex + 1) * PER_CHAIN_SIZE;
					}
				}
			}
			_pool[search_index(size)] = split_free_pool(size);//切割战备池，挂到对应内存链
		}

		//功能：查找内存需求对应的内存链索引
		//输入：内存需求大小（执行align_size后）
		//输出：内存链索引
		static size_t search_index(size_t size)
		{
			return (size >> 2) - 1;
		}

		//功能：内存需求与PER_CHAIN_SIZE对齐
		//输入：内存需求大小
		//输出：对齐后内存需求大小
		static size_t align_size(size_t size)
		{
			//区分64位和32位。64位时指针大小为8byte，_pool[0]为4byte无法实现嵌入式指针，固64位弃用_pool[0]
			return _align_size(std::bool_constant <sizeof(void*) == 8>{}, size);
		}

		//功能：align_size辅助函数（64位）
		static size_t _align_size(std::true_type, size_t size)
		{
			return size <= 8 ? 8 : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1));
		}

		//功能：align_size辅助函数（32位）
		static size_t _align_size(std::false_type, size_t size)
		{
			return size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1);
		}

		//功能：向对应内存链添加新块
		//输入：新块指针，内存需求大小
		void add_to_chain(Node* p, size_t size)
		{
			Node*& chainHead{ _pool[search_index(size)] };
			p->next = chainHead;
			chainHead = p;
		}

		//功能：移除对应内存链头部的块
		//输入：对应内存链指针引用
		//输出：移除的块指针
		void* remove_from_chain(Node*& chainHead)
		{
			void* result{ chainHead };
			chainHead = chainHead->next;
			return result;
		}

	private:
		struct Node
		{
			Node* next;
		};
		std::vector<Node*> _pool;//内存池
		char* _freeBegin, * _freeEnd;//战备池指针
		size_t _count;//内存池管理内存总量
	};
}

#endif // ! WUKONGMEMORYPOOL_H


