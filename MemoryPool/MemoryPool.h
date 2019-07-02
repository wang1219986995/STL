#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include "WukongMemoryPool.h"
#include "LokiMemoryPool.h"
#include <queue>
#include <mutex>

namespace hzw
{
	//内存池管理：提供全局内存池分配和回收功能
	template<typename MemoryPool>
	class MemoryPoolManage
	{
		friend class MemoryPoolProxy;

	public:
		MemoryPoolManage() = delete;

		//功能：分配内存资源
		//输入：内存需求的大小
		//输出：分配后的内存指针
		static void* allocate(size_t size)
		{
			return _memoryPoolProxy.allocate(size);
		}

		//功能：释放内存资源
		//输入：释放内存的指针，释放内存的大小
		static void deallocate(void* p, size_t size)
		{
			return _memoryPoolProxy.deallocate(p, size);
		}

	private:
		//内存池代理：提供内存池分配和回收功能
		class MemoryPoolProxy
		{
		public:
			MemoryPoolProxy()noexcept : _memoryPool{ ini_pool() } {}

			MemoryPoolProxy(const MemoryPoolProxy&) = delete;

			~MemoryPoolProxy()noexcept
			{
				std::lock_guard<std::mutex> locker{ _poolsMutex };
				_pools.push(std::move(_memoryPool));
			}

			void* allocate(size_t size)
			{
				return size > MemoryPool::MAX_SIZE ? ::operator new(size) :
					_memoryPool.allocate(size);
			}

			void deallocate(void* p, size_t size)
			{
				size > MemoryPool::MAX_SIZE ? ::operator delete(p) :
					_memoryPool.deallocate(p, size);
			}

		private:
			static MemoryPool ini_pool()
			{
				std::lock_guard<std::mutex> locker{ _poolsMutex };
				if (_pools.empty()) return MemoryPool{};
				else
				{
					MemoryPool pool{ std::move(_pools.front()) };
					_pools.pop();
					return pool;
				}
			}

		private:
			MemoryPool _memoryPool;
		};
	
	private:	
		static std::queue<MemoryPool> _pools;//内存池容器
		static std::mutex _poolsMutex;//内存池容器锁
		static thread_local MemoryPoolProxy _memoryPoolProxy;//各线程独立的内存池代理
	};
	template<typename MemoryPool>
	std::queue<MemoryPool> MemoryPoolManage<MemoryPool>::_pools;
	template<typename MemoryPool>
	std::mutex MemoryPoolManage<MemoryPool>::_poolsMutex;
	template<typename MemoryPool>
	thread_local typename MemoryPoolManage<MemoryPool>::MemoryPoolProxy 
		MemoryPoolManage<MemoryPool>::_memoryPoolProxy;

	//分配器：提供容器使用内存池功能
	template<typename T, typename MemoryPool>
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
			return static_cast<value_type *>(MemoryPoolManage<MemoryPool>::allocate(num * sizeof(value_type)));
		}

		void deallocate(value_type *p, std::size_t num)
		{
			MemoryPoolManage<MemoryPool>::deallocate(p, num * sizeof(value_type));
		}

		template<typename U>
		bool operator ==(const Allocator<U, MemoryPool>& lh)
		{
			return true;
		}

		template<typename U>
		bool operator !=(const Allocator<U, MemoryPool>& lh)
		{
			return false;
		}

		template<typename U>
		operator Allocator<U, MemoryPool>()const
		{
			return Allocator<U, MemoryPool>{};
		}
	};
	template<typename T>//悟空分配器别名
	using AllocWk = Allocator<T, WukongMemoryPool<false>>;
	template<typename T>//洛基分配器别名
	using AllocLk = Allocator<T, LokiMemoryPool<false>>;

	//提供用户自定义类使用内存池功能（公有继承此类）
	template<typename MemoryPool>
	class UseMemoryPool
	{
	public:
		void* operator new(size_t size)
		{
			return MemoryPoolManage<MemoryPool>::allocate(size);
		}

		void operator delete(void* oldP, size_t size)
		{
			MemoryPoolManage<MemoryPool>::deallocate(oldP, size);
		}
	};
	using UseWk = UseMemoryPool<WukongMemoryPool<false>>;//悟空（不支持多态）
	using UseLk = UseMemoryPool<LokiMemoryPool<false>>;//洛基（不支持多态）
	using UseWkP = UseMemoryPool<WukongMemoryPool<true>>;//悟空（支持多态）
	using UseLkP = UseMemoryPool<LokiMemoryPool<true>>;//洛基（支持多态）
}

#endif //MEMORYPOOL_H
