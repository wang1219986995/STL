#ifndef  WUKONGMEMORYPOOL_H
#define WUKONGMEMORYPOOL_H

#include <type_traits>
#include <utility>

namespace hzw
{
	class WukongMemoryPool
	{
	public:
		WukongMemoryPool();

		WukongMemoryPool(const WukongMemoryPool&) = delete;

		//功能：分配内存资源
		//输入：内存需求的大小
		//输出：分配后的内存指针
		void* allocate(size_t size);

		//功能：释放内存资源
		//输入：释放内存的指针，释放内存的大小
		void deallocate(void* oldP, size_t size);

	public:
		enum
		{
			CHAIN_LENTH = 32, PER_CHAIN_SIZE = 4, MAX_SPLIT_SIZE = 20,
			MAX_SIZE = CHAIN_LENTH * PER_CHAIN_SIZE
		};
		//内存池链的个数  //内存池粒度 //最大切割个数 //管理的最大内存 

		struct Node
		{
			Node* next;
		};
		Node* _pool[CHAIN_LENTH], *_poolEnd[CHAIN_LENTH];//内存池， 内存链结尾
		char* _freeBegin, * _freeEnd;//战备池指针
		size_t _count;//内存池管理内存总量

	private:
		//功能：切割战备池后调整战备池大小
		//输入：对齐后内存需求大小
		//输出：切割完成后的内存链
		std::pair<Node*, Node*> split_free_pool(size_t size);

		//功能：填充内存链（填充内存来源：战备池，malloc，更大的内存链）
		//输入：对齐后内存需求大小
		void fill_chain(size_t size);

		//功能：查找内存需求对应的内存链索引
		//输入：内存需求大小（执行align_size后）
		//输出：内存链索引
		static size_t search_index(size_t size);

		//功能：内存需求与ChainPerSize对齐
		//输入：内存需求大小
		//输出：对齐后内存需求大小
		static size_t align_size(size_t size);

		//功能：align_size辅助函数
		static size_t _align_size(std::true_type, size_t size);

		//功能：align_size辅助函数
		static size_t _align_size(std::false_type, size_t size);

		//功能：向对应内存链添加新块
		//输入：新块指针，内存需求大小
		void add_to_chain(Node* p, size_t size);

		//功能：移除对应内存链头部的块
		//输入：对应内存链指针引用
		//输出：移除的块指针
		void* remove_from_chain(size_t size);
	};
}

#endif // ! WUKONGMEMORYPOOL_H


