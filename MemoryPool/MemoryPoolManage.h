#ifndef MEMORYPOOLMANAGE_H
#define MEMORYPOOLMANAGE_H

#include <cassert>
#include <mutex>
#include <vector>
#include <type_traits>

namespace hzw
{
	using namespace std;

	//全局内存池管理：全局唯一内存池（全局归属，即任意线程new任意线程delete）
	template<typename MemoryPool>
	class GlobalMemoryPoolManage
	{
	public:
		GlobalMemoryPoolManage() = delete;

		//功能：分配内存资源
		//输入：内存需求的大小
		//输出：分配后的内存指针
		static void* allocate(size_t size)
		{
			lock_guard<mutex> locker{ _poolMutex };
			return _memoryPool.allocate(size);
		}

		//功能：释放内存资源
		//输入：释放内存的指针，释放内存的大小
		static void deallocate(void* p, size_t size)
		{
			lock_guard<mutex> locker{ _poolMutex };
			_memoryPool.deallocate(p, size);
		}

	private:
		static mutex _poolMutex;
		static MemoryPool _memoryPool;
	};
	template<typename MemoryPool>
	mutex GlobalMemoryPoolManage<MemoryPool>::_poolMutex;
	template<typename MemoryPool>
	MemoryPool GlobalMemoryPoolManage<MemoryPool>::_memoryPool;

	//线程内存池管理：一个线程对应一个内存池（线程归属，即new、delete必须在同一线程）
	template<typename MemoryPool>
	class ThreadLocalMemoryPoolManage
	{
		friend class MemoryPoolProxy;

	public:
		ThreadLocalMemoryPoolManage() = delete;

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
			MemoryPoolProxy() : _memoryPool{ ini_pool() } {}

			MemoryPoolProxy(const MemoryPoolProxy&) = delete;

			MemoryPoolProxy(MemoryPoolProxy&&) = delete;

			MemoryPoolProxy& operator=(const MemoryPoolProxy&) = delete;

			MemoryPoolProxy& operator=(MemoryPoolProxy&&) = delete;

			~MemoryPoolProxy()noexcept
			{
				lock_guard<mutex> locker{ _poolsMutex };
				_pools.push_back(move(_memoryPool));
			}

			void* allocate(size_t size)
			{
				return _memoryPool.allocate(size);
			}

			void deallocate(void* p, size_t size)
			{
				_memoryPool.deallocate(p, size);
			}

		private:
			static MemoryPool ini_pool()
			{
				lock_guard<mutex> locker{ _poolsMutex };
				if (_pools.empty()) return MemoryPool{};
				else
				{
					MemoryPool pool{ move(_pools.back()) };
					_pools.pop_back();
					return pool;
				}
			}

		private:
			MemoryPool _memoryPool;
		};

	private:
		static vector<MemoryPool> _pools;//内存池容器
		static mutex _poolsMutex;//内存池容器锁
		static thread_local MemoryPoolProxy _memoryPoolProxy;//各线程独立的内存池代理
	};
	template<typename MemoryPool>
	vector<MemoryPool> ThreadLocalMemoryPoolManage<MemoryPool>::_pools;
	template<typename MemoryPool>
	mutex ThreadLocalMemoryPoolManage<MemoryPool>::_poolsMutex;
	template<typename MemoryPool>
	thread_local typename ThreadLocalMemoryPoolManage<MemoryPool>::MemoryPoolProxy
		ThreadLocalMemoryPoolManage<MemoryPool>::_memoryPoolProxy;

	//分配器：提供容器使用内存池功能
	template<typename T, typename MemoryPoolManage>
	class Allocator
	{
	public:
		using value_type = T;
		using pointer = T *;
		using const_pointer = const T*;
		using reference = T &;
		using const_reference = const T &;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		value_type* allocate(size_t num)
		{
			return static_cast<value_type*>(MemoryPoolManage::allocate(num * sizeof(value_type)));
		}

		void deallocate(value_type* p, size_t num)
		{
			MemoryPoolManage::deallocate(p, num * sizeof(value_type));
		}

		template<typename U>
		bool operator ==(const Allocator<U, MemoryPoolManage>& lh)const
		{
			return true;
		}

		template<typename U>
		bool operator !=(const Allocator<U, MemoryPoolManage>& lh)const
		{
			return false;
		}

		template<typename U>
		operator Allocator<U, MemoryPoolManage>()const
		{
			return Allocator<U, MemoryPoolManage>{};
		}
	};

	//提供用户自定义类使用内存池功能（公有继承此类）
	template<typename MemoryPoolManage>
	class UseMemoryPool
	{
	public:
		void* operator new(size_t size)
		{
			return MemoryPoolManage::allocate(size);
		}

		void operator delete(void* oldP, size_t size)
		{
			MemoryPoolManage::deallocate(oldP, size);
		}
	};

	template<typename MemoryPool>
	using GMM = GlobalMemoryPoolManage<MemoryPool>;//全局内存池管理别名

	template<typename MemoryPool>
	using TMM = ThreadLocalMemoryPoolManage<MemoryPool>;//线程内存池管理别名
}

#endif // !MEMORYPOOLMANAGE_H

