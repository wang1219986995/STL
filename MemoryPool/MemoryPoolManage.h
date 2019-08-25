#ifndef MEMORYPOOLMANAGE_H
#define MEMORYPOOLMANAGE_H

#include <cassert>
#include <mutex>
#include <vector>
#include <type_traits>

namespace hzw
{
	using namespace std;

	//ȫ���ڴ�ع���ȫ��Ψһ�ڴ�أ�ȫ�ֹ������������߳�new�����߳�delete��
	template<typename MemoryPool>
	class GlobalMemoryPoolManage
	{
	public:
		GlobalMemoryPoolManage() = delete;

		//���ܣ������ڴ���Դ
		//���룺�ڴ�����Ĵ�С
		//������������ڴ�ָ��
		static void* allocate(size_t size)
		{
			lock_guard<mutex> locker{ _poolMutex };
			return _memoryPool.allocate(size);
		}

		//���ܣ��ͷ��ڴ���Դ
		//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ�Ĵ�С
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

	//�߳��ڴ�ع���һ���̶߳�Ӧһ���ڴ�أ��̹߳�������new��delete������ͬһ�̣߳�
	template<typename MemoryPool>
	class ThreadLocalMemoryPoolManage
	{
		friend class MemoryPoolProxy;

	public:
		ThreadLocalMemoryPoolManage() = delete;

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
		static vector<MemoryPool> _pools;//�ڴ������
		static mutex _poolsMutex;//�ڴ��������
		static thread_local MemoryPoolProxy _memoryPoolProxy;//���̶߳������ڴ�ش���
	};
	template<typename MemoryPool>
	vector<MemoryPool> ThreadLocalMemoryPoolManage<MemoryPool>::_pools;
	template<typename MemoryPool>
	mutex ThreadLocalMemoryPoolManage<MemoryPool>::_poolsMutex;
	template<typename MemoryPool>
	thread_local typename ThreadLocalMemoryPoolManage<MemoryPool>::MemoryPoolProxy
		ThreadLocalMemoryPoolManage<MemoryPool>::_memoryPoolProxy;

	//���������ṩ����ʹ���ڴ�ع���
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

	//�ṩ�û��Զ�����ʹ���ڴ�ع��ܣ����м̳д��ࣩ
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
	using GMM = GlobalMemoryPoolManage<MemoryPool>;//ȫ���ڴ�ع������

	template<typename MemoryPool>
	using TMM = ThreadLocalMemoryPoolManage<MemoryPool>;//�߳��ڴ�ع������
}

#endif // !MEMORYPOOLMANAGE_H

