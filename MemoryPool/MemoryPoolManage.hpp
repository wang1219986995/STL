#pragma once

#include "MemoryPoolUtility.hpp"

namespace hzw
{
	namespace detail
	{
		//�ڴ�ع������
		template<template<typename MemoryPool> typename MemoryPoolManage, typename MemoryPool, bool BelongGlobal>
		struct MemoryPoolManageBase
		{
			using memory_pool = MemoryPool;
			using memory_manage = MemoryPoolManage<memory_pool>;
			using belong_global = std::bool_constant<BelongGlobal>;//�Ƿ�ȫ�ֹ���
			using belong_thread = std::bool_constant<!BelongGlobal>;//�Ƿ��̹߳���
			using support_polym = typename memory_pool::support_polym;//������ڴ��Ƿ�֧�ֶ�̬
			using support_chunk = typename memory_pool::support_chunk;//�ڴ�����Ƿ�֧�ִ�����
			enum { MAX_SIZE = memory_pool::MAX_SIZE };

			MemoryPoolManageBase() = delete;

			//���ܣ������ڴ���Դ
			//���룺�ڴ������������ڴ������С
			//������������ڴ�ָ��
			static void* callocate(size_t count, size_t size)
			{
				return memory_manage::allocate(count * size);
			}

			//���ܣ����·����ڴ���Դ���ڴ��֧��malloc_size���ṩ�˹��ܣ�
			//���룺�ڴ��ָ�룬�ڴ������С
			//������ط������ڴ�ָ��
			static void* reallocate(void* p, size_t size)
			{
				size_t mallocSize{ memory_manage::malloc_size(p) };
				void* newP{ memory_manage::allocate(size) };
				memcpy(newP, p, size < mallocSize ? size : mallocSize);
				memory_manage::deallocate(p);
				return newP;
			}

			//���ܣ���ȡ�ڴ���С���ڴ��֧�ֶ�̬���ṩ�˹��ܣ�
			//���룺�ڴ�ָ��
			//������ڴ���С
			static size_t malloc_size(void* p)
			{
				static_assert(support_polym::value, "memory pool must support polym");
				return memory_pool::malloc_size(p);
			}

			//���ܣ���ȡ�ڴ����
			//������ڴ����
			static const char* get_pool_name()
			{
				constexpr const char* poolName{ MemoryPool::get_pool_name() };
				constexpr const char* belongName{ belong_global::value ? "G" : "T" };
				constexpr const char* polymName{ support_polym::value ? "P" : "" };
				constexpr static auto str{ MAKE_NATIVE_STR(poolName, belongName, polymName) };
				return str.c_str();
			}
		};

		//ȫ���ڴ�ع���ȫ�ֹ���һ���ڴ��
		template<typename MemoryPool>
		class GlobalMemoryPoolManage final : public MemoryPoolManageBase<GlobalMemoryPoolManage, MemoryPool, true>
		{
			using my_base = MemoryPoolManageBase<GlobalMemoryPoolManage, MemoryPool, true>;

		public:
			//���ܣ������ڴ���Դ
			//���룺�ڴ������С
			//������������ڴ�ָ��
			template<typename... DebugInfos>
			static void* allocate(size_t size, DebugInfos&&...)
			{
				std::lock_guard<std::mutex> locker{ _poolMutex() };
				return _memoryPool().allocate(size);
			}

			//���ܣ��ͷ��ڴ���Դ����֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ��С
			static void deallocate(void* p, size_t size)
			{
				static_assert(!my_base::support_polym::value, "memory pool must unsupport polym");
				std::lock_guard<std::mutex> locker{ _poolMutex() };
				_memoryPool().deallocate(p, size);
			}

			//���ܣ��ͷ��ڴ���Դ��֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ��
			static void deallocate(void* p)
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				constexpr size_t unuse{ 1 };
				std::lock_guard<std::mutex> locker{ _poolMutex() };
				_memoryPool().deallocate(p, unuse);
			}

		private:
			static std::mutex& _poolMutex()
			{
				static std::mutex _poolMutex; //�ڴ�ػ�����
				return _poolMutex;
			}

			static MemoryPool& _memoryPool()
			{
				static MemoryPool _memoryPool; //�ڴ��
				return _memoryPool;
			}
		};

		//�߳��ڴ�ع���һ���̶߳�Ӧһ���ڴ��
		template<typename MemoryPool>
		class ThreadLocalMemoryPoolManage final : public MemoryPoolManageBase<ThreadLocalMemoryPoolManage, MemoryPool, false>
		{
			friend class MemoryPoolProxy;
			using my_base = MemoryPoolManageBase<ThreadLocalMemoryPoolManage, MemoryPool, false>;

		public:
			//���ܣ������ڴ���Դ
			//���룺�ڴ�����Ĵ�С
			//������������ڴ�ָ��
			template<typename... DebugInfos>
			static void* allocate(size_t size, DebugInfos&&...)
			{
				return _memoryPoolProxy().allocate(size);
			}

			//���ܣ��ͷ��ڴ���Դ����֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ�Ĵ�С
			static void deallocate(void* p, size_t size)
			{
				static_assert(!my_base::support_polym::value, "memory pool must unsupport polym");
				_memoryPoolProxy().deallocate(p, size);
			}

			//���ܣ��ͷ��ڴ���Դ��֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ��
			static void deallocate(void* p)
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				constexpr size_t unuse{ 1 };
				_memoryPoolProxy().deallocate(p, unuse);
			}
		private:
			//�ڴ�ش����ṩ�ڴ�ػ��������ù���
			class MemoryPoolProxy
			{
			public:
				MemoryPoolProxy() : _memoryPool{ ini_pool() } {}

				MemoryPoolProxy(const MemoryPoolProxy&) = delete;

				MemoryPoolProxy(MemoryPoolProxy&&) = delete;

				MemoryPoolProxy& operator=(const MemoryPoolProxy&) = delete;

				MemoryPoolProxy& operator=(MemoryPoolProxy&&) = delete;

				~MemoryPoolProxy() noexcept
				{
					std::lock_guard<std::mutex> locker{ _poolsMutex() };
					_pools().push_back(std::move(_memoryPool));
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
					std::lock_guard<std::mutex> locker{ _poolsMutex() };
					if (_pools().empty()) return MemoryPool{};
					else
					{
						MemoryPool pool{ std::move(_pools().back()) };
						_pools().pop_back();
						return pool;
					}
				}

			private:
				MemoryPool _memoryPool;
			};

		private:
			static Vector<MemoryPool>& _pools()
			{
				static Vector<MemoryPool> _pools;//�ڴ������
				return _pools;
			}

			static std::mutex& _poolsMutex()
			{
				static std::mutex _poolsMutex;//�ڴ��������
				return _poolsMutex;
			}

			static MemoryPoolProxy& _memoryPoolProxy()
			{
				static thread_local MemoryPoolProxy _memoryPoolProxy;//���̶߳������ڴ�ش���
				return _memoryPoolProxy;
			}
		};

		//�ڴ�ع����⸲��
		template<typename MemoryPoolManage>
		struct MemoryPoolManageWrapBase
		{
			using memory_manage = MemoryPoolManage;
			using memory_pool = typename memory_manage::memory_pool;
			using belong_global = typename memory_manage::belong_global;//�Ƿ�ȫ�ֹ���
			using belong_thread = typename memory_manage::belong_thread;//�Ƿ��̹߳���
			using support_polym = typename memory_manage::support_polym;//������ڴ��Ƿ�֧�ֶ�̬
			using support_chunk = typename memory_manage::support_chunk;//�ڴ�����Ƿ�֧�ִ�����
			enum { MAX_SIZE = memory_manage::MAX_SIZE };

			MemoryPoolManageWrapBase() = delete;

			//���ܣ������ڴ���Դ
			//���룺�ڴ������������ڴ������С
			//������������ڴ�ָ��
			static void* callocate(size_t count, size_t size)
			{
				return MemoryPoolManage::callocate(count, size);
			}

			//���ܣ����·����ڴ���Դ���ڴ��֧��malloc_size���ṩ�˹��ܣ�
			//���룺�ڴ��ָ�룬�ڴ������С
			//������ط������ڴ�ָ��
			static void* reallocate(void* p, size_t size)
			{
				MemoryPoolManage::reallocate(p, size);
			}

			//���ܣ���ȡ�ڴ���С���ڴ��֧�ֶ�̬���ṩ�˹��ܣ�
			//���룺�ڴ�ָ��
			//������ڴ���С
			static size_t malloc_size(void* p)
			{
				return MemoryPoolManage::malloc_size(p);
			}

			//���ܣ���ȡ�ڴ����
			//������ڴ����
			static const char* get_pool_name()
			{
				return MemoryPoolManage::get_pool_name();
			}
		};

		//�ڴ�ع����⸲����Ϊ����ӻ��幦�ܣ�
		template<typename MemoryPoolManage, bool Enable>
		class BufferMemoryPoolManageWrap final : public MemoryPoolManageWrapBase<MemoryPoolManage>
		{			
			using my_base = MemoryPoolManageWrapBase<MemoryPoolManage>;

		public:
			using buffer_enable = std::bool_constant<Enable>;//�����Ƿ���Ч

			//���ܣ������ڴ���Դ
			//���룺�ڴ������С
			//������������ڴ�ָ��
			template<typename... DebugInfos>
			static void* allocate(size_t size, DebugInfos&&...)
			{
				return _allocate(size, buffer_enable{});
			}

			//���ܣ��ͷ��ڴ���Դ����֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ��С
			static void deallocate(void* p, size_t size)
			{
				static_assert(!my_base::support_polym::value, "memory pool must unsupport polym");
				_deallocate(p, size, buffer_enable{});
			}

			//���ܣ��ͷ��ڴ���Դ��֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ��
			static void deallocate(void* p)
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				constexpr size_t unuse{ 0 };
				_deallocate(p, unuse, buffer_enable{});
			}

		private:
			static void* _allocate(size_t size, std::true_type)
			{
				return get_buf().allocate(size);
			}

			static void* _allocate(size_t size, std::false_type)
			{
				return MemoryPoolManage::allocate(size);
			}

			static void _deallocate(void* p, size_t size, std::true_type)
			{
				get_buf().deallocate(p, size);
			}

			static void _deallocate(void* p, size_t size, std::false_type)
			{
				dealloc(p, size, typename my_base::support_polym{});
			}

			static void dealloc(void* p, size_t, std::true_type)
			{
				MemoryPoolManage::deallocate(p);
			}

			static void dealloc(void* p, size_t size, std::false_type)
			{
				MemoryPoolManage::deallocate(p, size);
			}

		private:
			class Buf
			{
			public:
				Buf() : _buf{}, _continuousDeallocCount{ 0 }, _continuousAllocCount{ 0 }{}

				Buf(const Buf&) = delete;

				Buf(Buf&&) = delete;

				Buf& operator=(const Buf&) = delete;

				Buf& operator=(Buf&&) = delete;

				~Buf() = default; //���̽���ϵͳ����

				void* allocate(size_t size)
				{
					++_continuousAllocCount;
					_continuousDeallocCount = 0;
					if (_continuousAllocCount >= CONTINUOUS_COUNT_LIMIT) return MemoryPoolManage::allocate(size);
					return _allocate(size, typename my_base::support_polym{});
				}

				void deallocate(void* p, size_t size)
				{
					_deallocate(p, size, typename my_base::support_polym{});
				}

			private:
				enum
				{
					BUF_SIZE = BUFFER_MEMORYPOOL_MANAGE_BUFFER_SIZE,
					CONTINUOUS_COUNT_LIMIT = BUF_SIZE / 10
				};

				void* _allocate(size_t size, std::true_type)
				{
					auto it{ _buf.lower_bound(size) };
					if (it == std::end(_buf)) return MemoryPoolManage::allocate(size);
					if (it->first - size > (MEMORYPOOL_ALIGN_SIZE ? MEMORYPOOL_ALIGN_SIZE : 4)) return MemoryPoolManage::allocate(size);
					auto& vec{ it->second };
					if (vec.empty()) return MemoryPoolManage::allocate(size);

					void* res{ vec.back() };
					vec.pop_back();
					return res;
				}

				void* _allocate(size_t size, std::false_type)
				{
					auto it{ _buf.find(size) };
					if (it == std::end(_buf)) return MemoryPoolManage::allocate(size);
					auto& vec{ it->second };
					if (vec.empty()) return MemoryPoolManage::allocate(size);

					void* res{ vec.back() };
					vec.pop_back();
					return res;
				}

				void _deallocate(void* p, size_t, std::true_type)
				{
					_deallocate(p, my_base::malloc_size(p), std::false_type{});
				}

				void _deallocate(void* p, size_t size, std::false_type)
				{
					//�����ͷų�������ֵ��ֱ�����ڴ�ػ��գ����ٽ��л���
					_continuousAllocCount = 0;
					++_continuousDeallocCount;
					if(_continuousDeallocCount >= CONTINUOUS_COUNT_LIMIT)  return dealloc(p, size, typename my_base::support_polym{});

					auto it{ _buf.find(size) };
					if (it == std::end(_buf))
					{
						std::vector<void*> v;
						v.reserve(BUF_SIZE);
						v.emplace_back(p);
						_buf.emplace(size, std::move(v));
						return;
					}

					auto& vec{ it->second };
					if (vec.size() == BUF_SIZE)
					{
						for (auto p : vec) dealloc(p, size, typename my_base::support_polym{});
						vec.clear();
					}
					vec.emplace_back(p);
				}

			private:
				std::map<size_t, std::vector<void*>> _buf;
				size_t _continuousDeallocCount, _continuousAllocCount;
			};

		private:
			static Buf& get_buf()
			{
				static thread_local Buf buf;
				return buf;
			}
		};

		//�ڴ�ع����⸲����Ϊ������ڴ�й©��⡢���ü�⣬��debugģʽ�����ã�
		template<typename MemoryPoolManage>
		class DebugMemoryPoolManageWrap final : public MemoryPoolManageWrapBase<MemoryPoolManage>
		{
			struct DebugCookie;
			using my_base = MemoryPoolManageWrapBase<MemoryPoolManage>;

		public:
			//���ܣ������ڴ���Դ
			//���룺�ڴ�����Ĵ�С
			//������������ڴ�ָ��
			template<typename... DebugInfos>
			static void* allocate(size_t sz, DebugInfos&&... debugInfos)
			{
				assert(sz > 0);
				//�ڴ�����ԭʼ���� + �����ڱ� + DebugCookie
				DebugCookie* cookie{ static_cast<DebugCookie*>(MemoryPoolManage::allocate(sz + sizeof(guard_type) * 2 + sizeof(DebugCookie))) };
				std::lock_guard<std::mutex> locker{ _mutex() };//����������̱߳��뱣��һ����
				//��¼�ڴ��������
				++(_head()._count);
				if (sz <= my_base::MAX_SIZE) ++(_head()._hitCount);
				const char* poolName{ my_base::get_pool_name() };
				//����DebugCookie
				new(cookie) DebugCookie{ nullptr, _head()._end, create_debug_info(std::forward<DebugInfos>(debugInfos)...),
					sz, std::hash<std::thread::id>{}(std::this_thread::get_id()),
					poolName, MEMORYPOOL_FLAG };
				//����˫������
				if (_head()._begin == nullptr) _head()._begin = cookie;
				else _head()._end->_next = cookie;
				_head()._end = cookie;
				//�����ڱ�
				guard_type* guardBegin{ reinterpret_cast<guard_type*>(cookie + 1) };
				guard_type* guardEnd{ reinterpret_cast<guard_type*>(reinterpret_cast<char*>(guardBegin + 1) + sz) };
				*guardEnd = *guardBegin = GUARD_FLAG;
				return guardBegin + 1;
			}

			//���ܣ��ͷ��ڴ���Դ����֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ�Ĵ�С
			static void deallocate(void* p, size_t sz)//��֧�ֶ�̬
			{
				static_assert(!my_base::support_polym::value, "memory pool must unsupport polym");
				assert(p != nullptr);
				DebugCookie* cookie{ get_debug_cookie(p) };
				_deallocate(cookie, sz);
				MemoryPoolManage::deallocate(cookie, sz + sizeof(DebugCookie) + sizeof(guard_type) * 2);
			}

			//���ܣ��ͷ��ڴ���Դ��֧�ֶ�̬��
			//���룺�ͷ��ڴ��ָ��
			static void deallocate(void* p)//֧�ֶ�̬
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				assert(p != nullptr);
				constexpr size_t unuse{ 0 };
				DebugCookie* cookie{ get_debug_cookie(p) };
				_deallocate(cookie, unuse);
				MemoryPoolManage::deallocate(cookie);
			}

			//���ܣ������ڴ���Դ
			//���룺�ڴ������������ڴ������С
			//������������ڴ�ָ��
			static void* callocate(size_t count, size_t size)
			{
				return allocate(count * size);
			}

			//���ܣ����·����ڴ���Դ���ڴ��֧��malloc_size���ṩ�˹��ܣ�
			//���룺�ڴ��ָ�룬�ڴ������С
			//������ط������ڴ�ָ��
			static void* reallocate(void* p, size_t size)
			{
				size_t mallocSize{ malloc_size(p) };
				void* newP{ allocate(size) };
				memcpy(newP, p, size < mallocSize ? size : mallocSize);
				deallocate(p);
				return newP;
			}

			//���ܣ���ȡ�ڴ���С���ڴ��֧�ֶ�̬���ṩ�˹��ܣ�
			//���룺�ڴ�ָ��
			//������ڴ���С
			static size_t malloc_size(void* p)
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				assert(p != nullptr);
				constexpr size_t unuse{ 0 };
				DebugCookie* cookie{ get_debug_cookie(p) };
				check_common(cookie, unuse);
				return MemoryPoolManage::malloc_size(cookie) - sizeof(DebugCookie) - sizeof(guard_type) * 2;
			}

		private:
			template<typename... DebugInfos>
			static std::string create_debug_info(DebugInfos&&... debugInfos)
			{
				std::ostringstream ostream;
				ostream << "[";
				_create_debug_info(ostream, std::forward<DebugInfos>(debugInfos)...);
				ostream << "]";
				return ostream.str();
			}

			static std::string create_debug_info()
			{
				return DI_NULL;
			}

			template<typename DebugInfo, typename... DebugInfos>
			static void _create_debug_info(std::ostringstream& os, DebugInfo&& debugInfo, DebugInfos&&... debugInfos)
			{
				os << std::forward<DebugInfo>(debugInfo);
				_create_debug_info(os, std::forward<DebugInfos>(debugInfos)...);
			}

			template<typename DebugInfo>
			static void _create_debug_info(std::ostringstream& os, DebugInfo&& debugInfo)
			{
				os << std::forward<DebugInfo>(debugInfo);
			}

			static DebugCookie* get_debug_cookie(void* p)
			{
				return reinterpret_cast<DebugCookie*>(static_cast<char*>(p) - sizeof(guard_type) - sizeof(DebugCookie));
			}

			static void check_common(DebugCookie* cookie, size_t size)
			{
				//У���Ƿ���ڴ������
				if (cookie->_poolFlag != MEMORYPOOL_FLAG)
				{
					_memorypoolLogHandle() << "ERROR:" << cookie << " is not from hzw::MemoryPool or multiple delete\n";
					std::exit(1);
				}
				guard_type* guardBegin{ reinterpret_cast<guard_type*>(cookie + 1) };
				guard_type* guardEnd{ reinterpret_cast<guard_type*>(reinterpret_cast<char*>(guardBegin + 1) + cookie->_size) };
				//У���ظ�deallocate
				if (*guardEnd == DEALLOCATED_FLAG && *guardBegin == DEALLOCATED_FLAG)
				{
					_memorypoolLogHandle() << "DEBUG_INFO:" << cookie->_debugInfo << "\nERROR:multiple delete\n";
					std::exit(1);
				}
				//У���ڱ����
				if (*guardBegin != GUARD_FLAG)
				{
					_memorypoolLogHandle() << "DEBUG_INFO:" << cookie->_debugInfo << "\nERROR:underflow\n";
					std::exit(1);
				}
				if (*guardEnd != GUARD_FLAG)
				{
					_memorypoolLogHandle() << "DEBUG_INFO:" << cookie->_debugInfo << "\nERROR:overflow\n";
					std::exit(1);
				}
				//У���Ƿ��ͬ���ڴ�ط����ͷ�
				if (cookie->_poolName != my_base::get_pool_name())
				{
					_memorypoolLogHandle() << "DEBUG_INFO:" << cookie->_debugInfo << " \nERROR:allocate MemoryPool is different from dellocate MemoryPool\nFROM:"
						<< cookie->_poolName << "\nNO FROM:" << my_base::get_pool_name() << std::endl;
					std::exit(1);
				}
				//У���Ƿ���ͬһ�̷߳����ͷ�
				check_thread(cookie, typename my_base::belong_thread{});//�̹߳����ڴ�ز�У��
				//У���ͷŴ�С
				check_size(cookie, size, typename my_base::support_polym{});//�Ƕ�̬��У��
			}

			static void _deallocate(DebugCookie* cookie, size_t size)
			{
				check_common(cookie, size);
				//���û��ձ��
				guard_type* guardBegin{ reinterpret_cast<guard_type*>(cookie + 1) };
				guard_type* guardEnd{ reinterpret_cast<guard_type*>(reinterpret_cast<char*>(guardBegin + 1) + cookie->_size) };
				*guardBegin = *guardEnd = DEALLOCATED_FLAG;
				//��˫�������Ƴ�
				std::lock_guard<std::mutex> locker{ _mutex() };//����������̱߳��뱣��һ����
				if (cookie->_prev == nullptr && cookie->_next == nullptr) _head()._begin = _head()._end = nullptr;
				else if (cookie->_prev == nullptr)
				{
					_head()._begin = cookie->_next;
					_head()._begin->_prev = nullptr;
				}
				else if (cookie->_next == nullptr)
				{
					cookie->_prev->_next = nullptr;
					_head()._end = cookie->_prev;
				}
				else
				{
					cookie->_prev->_next = cookie->_next;
					cookie->_next->_prev = cookie->_prev;
				}
			}

			//���ܣ�У���ͷŴ�С���Ƕ�̬��У�飩
			static void check_size(DebugCookie* cookie, size_t size, std::false_type)
			{
				if (cookie->_size != size)
				{
					_memorypoolLogHandle() << "DEBUG_INFO:" << cookie->_debugInfo << "\nERROR:size is " << cookie->_size << " not " << size << "\n";
					std::exit(1);
				}
			}

			static void check_size(DebugCookie*, size_t, std::true_type) {}

			//���ܣ�У���Ƿ���ͬһ�̷߳����ͷţ��̹߳�����У�飩
			static void check_thread(DebugCookie* cookie, std::true_type)
			{
				if (cookie->_threadId != std::hash<std::thread::id>{}(std::this_thread::get_id()))
				{
					_memorypoolLogHandle() << "DEBUG_INFO:" << cookie->_debugInfo << "\nERROR:" << "allocate thread is different from dellocate thread\n";
					std::exit(1);
				}
			}

			static void check_thread(DebugCookie*, std::false_type) {}

		private:
			using guard_type = size_t;
			enum { GUARD_FLAG = SIZE_MAX, DEALLOCATED_FLAG = GUARD_FLAG - 1 };

			struct DebugCookie
			{
				DebugCookie* _next;
				DebugCookie* _prev;
				std::string _debugInfo;//�ڴ������Ϣ
				size_t _size;//��¼�����С��������DebugCookie����
				size_t _threadId;//�߳�id
				const char* _poolName;//�ڴ����
				const char* _poolFlag;//�ڴ�ر��
			};

			struct DebugCookieHead
			{
				DebugCookieHead() : _begin{ nullptr }, _end{ nullptr }, _count{ 0 }, _hitCount{ 0 }
				{ 
					_memorypoolLogHandle();//ȷ��logHandle����DebugCookieHead���죨�ȹ���������� 
				}

				~DebugCookieHead()
				{
					_memorypoolLogHandle() << "\n----[" << my_base::get_pool_name() << "]  MEMORY_LEAK----\n";
					size_t size{ 0 }, count{ 0 };
					for (DebugCookie* head{ _begin }; head; head = head->_next)
					{
						if (head->_debugInfo == DI_IGNORE_CHECK) continue;
						size += head->_size;
						++count;
						_memorypoolLogHandle() << "--------SIZE:" << head->_size <<
							"  DEBUG_INFO:" << head->_debugInfo << std::endl;
					}
					_memorypoolLogHandle() << "----ALL_LEAK_SIZE:" << size << "  ALL_LEAK_COUNT:" << count << "----\n" <<
						"----SERVICE RANGE:1-" << my_base::MAX_SIZE << "  HIT_RATE:" <<
						static_cast<size_t>(static_cast<double>(_hitCount) / _count * 100) << "%----\n\n";
				}

				DebugCookie* _begin, * _end;//˫������ͷβ�ڵ�
				size_t _count, _hitCount;//���ڴ�����������ڷ����С�Ĵ���
			};

		private:
			static DebugCookieHead& _head()
			{
				static DebugCookieHead _head;
				return _head;
			}

			static std::mutex& _mutex()
			{
				static std::mutex _mutex;
				return _mutex;
			}
		};

#ifdef _DEBUG
		template<typename MemoryPool>
		using GMM = DebugMemoryPoolManageWrap<GlobalMemoryPoolManage<MemoryPool>>;//ȫ���ڴ�ع����������debug���ܣ�

		template<typename MemoryPool>
		using TMM = DebugMemoryPoolManageWrap<ThreadLocalMemoryPoolManage<MemoryPool>>;//�߳��ڴ�ع����������debug���ܣ�
#else
		template<typename MemoryPool>
		using GMM = BufferMemoryPoolManageWrap<GlobalMemoryPoolManage<MemoryPool>, BUFFER_MANAGE_ENABLE>;//ȫ���ڴ�ع������

		template<typename MemoryPool>
		using TMM = ThreadLocalMemoryPoolManage<MemoryPool>;//�߳��ڴ�ع������
#endif // _DEBUG
	}
}