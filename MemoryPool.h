#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include "WukongMemoryPool.h"
#include "LokiMemoryPool.h"
#include <queue>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace hzw
{
	template<typename MemoryPool>
	class MemoryPoolDelegate
	{
		using Record = std::pair<std::atomic<size_t>, MemoryPool>;
		friend class MemoryPoolInstance;

	public:
		class MemoryPoolInstance
		{
			using Record = MemoryPoolDelegate::Record;
			friend MemoryPoolDelegate;

		public:
			MemoryPoolInstance(const MemoryPoolInstance& rh) : _record{ rh._record }
			{
				++_record.first;
			}

			~MemoryPoolInstance()
			{
				if (--_record.first == 0)
				{
					MemoryPoolDelegate::_useMutex.lock();
					_usePool.erase(std::this_thread::get_id());
					MemoryPoolDelegate::_useMutex.unlock();
					MemoryPoolDelegate::_freeMutex.lock();
					_freePool.push(&_record);
					MemoryPoolDelegate::_freeMutex.unlock();
				}
			}

			//功能：分配内存资源
			//输入：内存需求的大小
			//输出：分配后的内存指针
			void* allocate(size_t size)
			{
				return size > MemoryPool::MAX_SIZE ? ::operator new(size) :
					_record.second.allocate(size);
			}

			//功能：释放内存资源
			//输入：释放内存的指针，释放内存的大小
			void deallocate(void* p, size_t size)
			{
				size > MemoryPool::MAX_SIZE ? ::operator delete(p) :
					_record.second.deallocate(p, size);
			}

		private:
			MemoryPoolInstance(Record& record) : _record{ record }
			{
				++record.first;
			}

		private:
			Record& _record;
		};
		
	public:
		static MemoryPoolInstance get_threadlocal_memorypool()
		{
			typename decltype(_usePool)::iterator it;
			std::thread::id threadId{ std::this_thread::get_id() };
			_useMutex.lock_shared();
			it = _usePool.find(threadId);
			_useMutex.unlock_shared();
			if (it == _usePool.end())//是否在使用池
			{
				Record* record{ nullptr };
				_freeMutex.lock();
				if (_freePool.empty()) record = new Record;//空闲池是否有空闲
				else
				{
					record = _freePool.front();
					_freePool.pop();
				}
				_freeMutex.unlock();
				record->first = 0;
				_useMutex.lock();
				_usePool.insert({ threadId, record });
				_useMutex.unlock();
				return MemoryPoolInstance{*record};
			}
			return MemoryPoolInstance{*it->second};
		}

	private:
		static std::queue<Record*> _freePool;
		static std::unordered_map<std::thread::id, Record*> _usePool;
		static std::mutex _freeMutex;
		static std::shared_mutex _useMutex;
	};
	template<typename MemoryPool>
	std::queue<typename MemoryPoolDelegate<MemoryPool>::Record*> MemoryPoolDelegate<MemoryPool>::_freePool;
	template<typename MemoryPool>
	std::unordered_map<std::thread::id, typename MemoryPoolDelegate<MemoryPool>::Record*> MemoryPoolDelegate<MemoryPool>::_usePool;
	template<typename MemoryPool>
	std::mutex MemoryPoolDelegate<MemoryPool>::_freeMutex;
	template<typename MemoryPool>
	std::shared_mutex MemoryPoolDelegate<MemoryPool>::_useMutex;

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
			return static_cast<value_type *>(_instance.allocate(num * sizeof(value_type)));
		}

		void deallocate(value_type *p, std::size_t num)
		{
			_instance.deallocate(p, num * sizeof(value_type));
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

	private:
		static thread_local typename MemoryPoolDelegate<MemoryPool>::MemoryPoolInstance _instance;			
	};
	template<typename T, typename MemoryPool>
	thread_local typename MemoryPoolDelegate<MemoryPool>::MemoryPoolInstance Allocator<T, MemoryPool>::_instance
	{ MemoryPoolDelegate<MemoryPool>::get_threadlocal_memorypool() };

	template<typename MemoryPool>
	class UseMemoryPool
	{
	public:
		void* operator new(size_t size)
		{
			return _instance.allocate(size);
		}

		void operator delete(void* oldP, size_t size)
		{
			_instance.deallocate(oldP, size);
		}

	private:
		static thread_local typename MemoryPoolDelegate<MemoryPool>::MemoryPoolInstance _instance;
	};
	template<typename MemoryPool>
	thread_local typename MemoryPoolDelegate<MemoryPool>::MemoryPoolInstance UseMemoryPool<MemoryPool>::_instance
	{ MemoryPoolDelegate<MemoryPool>::get_threadlocal_memorypool() };
}

#endif //MEMORYPOOL_H
