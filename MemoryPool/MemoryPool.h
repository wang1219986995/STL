#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include "WukongMemoryPool.h"
#include "LokiMemoryPool.h"
#include <queue>
#include <mutex>

namespace hzw
{
	//�ڴ�ع����ṩȫ���ڴ�ط���ͻ��չ���
	template<typename MemoryPool>
	class MemoryPoolManage
	{
		friend class MemoryPoolProxy;

	public:
		MemoryPoolManage() = delete;

		//���ܣ������ڴ���Դ
		//���룺�ڴ�����Ĵ�С
		//������������ڴ�ָ��
		static void* allocate(size_t size)
		{
			return _memoryPoolProxy.allocate(size);
		}

		//���ܣ��ͷ��ڴ���Դ
		//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ�Ĵ�С
		static void deallocate(void* p, size_t size)
		{
			return _memoryPoolProxy.deallocate(p, size);
		}

	private:
		//�ڴ�ش����ṩ�ڴ�ط���ͻ��չ���
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
		static std::queue<MemoryPool> _pools;//�ڴ������
		static std::mutex _poolsMutex;//�ڴ��������
		static thread_local MemoryPoolProxy _memoryPoolProxy;//���̶߳������ڴ�ش���
	};
	template<typename MemoryPool>
	std::queue<MemoryPool> MemoryPoolManage<MemoryPool>::_pools;
	template<typename MemoryPool>
	std::mutex MemoryPoolManage<MemoryPool>::_poolsMutex;
	template<typename MemoryPool>
	thread_local typename MemoryPoolManage<MemoryPool>::MemoryPoolProxy 
		MemoryPoolManage<MemoryPool>::_memoryPoolProxy;

	//���������ṩ����ʹ���ڴ�ع���
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
	template<typename T>//��շ���������
	using AllocWk = Allocator<T, WukongMemoryPool<false>>;
	template<typename T>//�������������
	using AllocLk = Allocator<T, LokiMemoryPool<false>>;

	//�ṩ�û��Զ�����ʹ���ڴ�ع��ܣ����м̳д��ࣩ
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
	using UseWk = UseMemoryPool<WukongMemoryPool<false>>;//��գ���֧�ֶ�̬��
	using UseLk = UseMemoryPool<LokiMemoryPool<false>>;//�������֧�ֶ�̬��
	using UseWkP = UseMemoryPool<WukongMemoryPool<true>>;//��գ�֧�ֶ�̬��
	using UseLkP = UseMemoryPool<LokiMemoryPool<true>>;//�����֧�ֶ�̬��
}

#endif //MEMORYPOOL_H
