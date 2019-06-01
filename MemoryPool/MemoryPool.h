#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H
#include <exception>
#include <mutex>

namespace hzw
{
	/*
	线程安全类
	测试量级：千万
	对比基准：::operator new
	内存：少20%
	速度：
		单线程：快10%
		多线程：多个线程请求相同大小速度不如operator new（请求内存大小相同的线程越多速度越慢）, 请求不同大小速度快10%
	总结：请求内存越小，各线程请求内存大小各不相同，时空效率越优
	*/
	class MemoryPool
	{
	private:
		enum {
			ChainLength = 16, ChainPerSize = 8, Log2ChainPerSize = 3,
			MaxSize = ChainLength * ChainPerSize, AskPerSize = 40, MaxSplitSize = 20
		};
		//内存池链的个数  //内存池粒度 //log2ChainPerSize的指 //管理的最大内存 //每次申请内存个数 //最大切割个数

		struct Node
		{
			Node *next;
		};
		static std::mutex _mutexs[ChainLength];//内存池互斥量
		static Node *volatile _pool[ChainLength];//内存池
		static std::mutex _freeMutex;//战备池互斥量
		static char *volatile _freeBegin, *volatile _freeEnd;//战备池指针
		static volatile size_t _count;//内存池管理内存总量

	private:
		//功能：从内存池分配内存
		//输入：对齐后内存需求大小
		//输出：分配内存块的指针
		static void *_allocate(size_t size);

		//功能：切割战备池后调整战备池大小
		//输入：对齐后内存需求大小
		//输出：切割完成后的内存链
		static Node *splitFreePool(size_t size);

		//功能：填充内存链（填充内存来源：战备池，malloc，更大的内存链）
		//输入：对齐后内存需求大小
		static void fillChain(size_t size);

		//功能：查找内存需求对应的内存链索引
		//输入：内存需求大小
		//输出：内存链索引
		constexpr static size_t searchIndex(size_t size)
		{
			return (size + ChainPerSize - 1 >> Log2ChainPerSize) - 1;
		}

		//功能：内存需求与ChainPerSize对齐
		//输入：内存需求大小
		//输出：对齐后内存需求大小
		constexpr static size_t alignSize(size_t size)
		{
			return (size + ChainPerSize - 1 & ~(ChainPerSize - 1));
		}

		//功能：向对应内存链添加新块
		//输入：新块指针，内存需求大小
		static void addToChain(Node *p, size_t size)
		{
			size_t index{ searchIndex(size) };
			_mutexs[index].lock();
			Node *volatile &chainHead{ _pool[index] };
			p->next = chainHead;
			chainHead = p;
			_mutexs[index].unlock();
		}

		//功能：移除对应内存链头部的块
		//输入：对应内存链指针引用
		//输出：移除的块指针
		static void *removeFromChain(Node * volatile&chainHead)
		{
			void *result{ chainHead };
			chainHead = chainHead->next;
			return result;
		}

		//功能：归还内存池内存
		//输入：归还内存的指针，对齐后内存需求大小
		static void _deallocate(Node *oldP, size_t size)
		{
			addToChain(oldP, size);
		}

	public:
		MemoryPool() = delete;

		//功能：分配内存资源
		//输入：内存需求的大小
		//输出：分配后的内存指针
		static void *allocate(size_t size)
		{
			return size > MaxSize ? ::operator new(size) : _allocate(alignSize(size));
		}

		//功能：释放内存资源
		//输入：释放内存的指针，释放内存的大小
		static void deallocate(void *oldP, size_t size)
		{
			size > MaxSize ? ::operator delete(oldP) : _deallocate(reinterpret_cast<Node *>(oldP), alignSize(size));
		}
	};

	/*
	类使用MemoryPool直接继承此类
	用基类指针delete谨慎使用，size值不正确，内存回收异常
	*/
	class UseMemoryPool
	{
	public:
		void *operator new(size_t size)
		{
			return MemoryPool::allocate(size);
		}

		void operator delete(void *oldP, size_t size)
		{
			MemoryPool::deallocate(oldP, size);
		}
	};

	/*
	容器分配器内部为MemoryPool
	*/
	template<typename T>
	class Allocator
	{
	public:
		using value_type = T;
		using pointer = T * ;
		using const_pointer = const T *;
		using reference = T & ;
		using const_reference = const T&;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		value_type *allocate(std::size_t num)
		{
			return reinterpret_cast<value_type *>(MemoryPool::allocate(num * sizeof(value_type)));
		}

		void deallocate(value_type *p, std::size_t num)
		{
			MemoryPool::deallocate(p, num * sizeof(value_type));
		}

		template<typename U>
		bool operator ==(const Allocator<U>& lh)
		{
			return true;
		}

		template<typename U>
		bool operator !=(const Allocator<U>& lh)
		{
			return false;
		}

		template<typename U>
		operator Allocator<U>()const
		{
			return Allocator<U>{};
		}
	};
}
#endif //MEMORYPOOL_H
