#ifndef  WUKONGMEMORYPOOL_H
#define WUKONGMEMORYPOOL_H

#include <vector>
#include <type_traits>

/*
	悟空内存池
	简单粗暴，提供快速分配回收
	在运行时此内存池持有的内存不主动归还给系统，直到进程结束由系统回收
	简而言之 不具有动态收缩能力，但换来更快的速度
*/

namespace hzw
{
	//悟空内存池
	template<bool SupportPolym>
	class WukongMemoryPool
	{
		struct Node;

	public:
		WukongMemoryPool() : _pool(CHAIN_LENTH, { 0, nullptr }) {}

		WukongMemoryPool(const WukongMemoryPool&) = delete;

		WukongMemoryPool(WukongMemoryPool&&) = default;

		void* allocate(size_t size)
		{
			size_t alSize{ align_size(size) };
			size_t index{ search_index(alSize) };
			//对应内存链为空则填充
			if (!_pool[index].second) fill_chain(_pool[index], alSize);
			return _allocate(_pool[index], alSize, std::bool_constant<SupportPolym>{});
		}

		void deallocate(void* oldP, size_t size)
		{
			_deallocate(oldP, size, std::bool_constant<SupportPolym>{});
		}

	public:
		enum
		{
			CHAIN_LENTH = 32, PER_CHAIN_SIZE = 4, MAX_SPLIT_SIZE = 128,
			MAX_SIZE = CHAIN_LENTH * PER_CHAIN_SIZE
		};
		//内存池链的个数  //内存池粒度 //最大切割个数 //管理的最大内存 

	private:
		//功能：allocate辅助函数（支持多态）
		void* _allocate(std::pair<size_t, Node*>& chainHead, size_t size, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(remove_from_chain(chainHead)) };
			*cookie = static_cast<unsigned char>(size);
			return cookie + 1;
		}

		//功能：allocate辅助函数（不支持多态）
		void* _allocate(std::pair<size_t, Node*>& chainHead, size_t, std::false_type)
		{
			return remove_from_chain(chainHead);
		}

		//功能：_deallocate辅助函数（支持多态）
		void _deallocate(void* oldp, size_t, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(oldp) - 1 };
			add_to_chain(reinterpret_cast<Node*>(cookie), _pool[search_index(*cookie)]);
		}

		//功能：_deallocate辅助函数（不支持多态）
		void _deallocate(void* oldp, size_t size, std::false_type)
		{
			size_t index{ search_index(align_size(size)) };
			add_to_chain(static_cast<Node*>(oldp), _pool[index]);
		}

		//功能：填充内存链
		//输入：内存链头、对齐后内存需求大小
		void fill_chain(std::pair<size_t, Node*>& chainHead, size_t size)
		{
			//动态决定切割个数
			size_t splitSize{ 20 + (chainHead.first >> 5) };
			splitSize = splitSize < MAX_SPLIT_SIZE ? splitSize : MAX_SPLIT_SIZE;
			//申请切割的内存
			char* chunk{ static_cast<char*>(::operator new(splitSize * size)) };
			//切割chunk
			char* p{ chunk };
			for (size_t i{ 1 }; i < splitSize; ++i, p += size)
				reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
			reinterpret_cast<Node*>(p)->next = nullptr;
			//挂到对应内存链
			chainHead.second = reinterpret_cast<Node*>(chunk);
		}

		//功能：查找内存需求对应的内存链索引
		//输入：对齐后内存需求大小
		//输出：内存链索引
		static size_t search_index(size_t size)
		{
			return (size - SupportPolym >> 2) - 1;
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
			return (size <= 8 ? 8 : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1))) + SupportPolym;
		}

		//功能：align_size辅助函数（32位）
		static size_t _align_size(std::false_type, size_t size)
		{
			return (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1)) + SupportPolym;
		}

		//功能：添加内存块到内存链
		//输入：添加的内存块、内存链头
		void add_to_chain(Node* p, std::pair<size_t, Node*>& chainHead)
		{
			--chainHead.first;
			p->next = chainHead.second;
			chainHead.second = p;
		}

		//功能：从内存链移除内存块
		//输入：内存链头
		//输出：内存块
		void* remove_from_chain(std::pair<size_t, Node*>& chainHead)
		{
			++chainHead.first;
			void* result{ chainHead.second };
			chainHead.second = chainHead.second->next;
			return result;
		}

	private:
		struct Node
		{
			Node* next;
		};
		std::vector<std::pair<size_t, Node*>> _pool;//内存池
	};
}

#endif // ! WUKONGMEMORYPOOL_H


