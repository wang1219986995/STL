#pragma once

#include <map>
#include <mutex>
#include <vector>
#include <cassert>
#include <sstream>
#include <memory>
#include <iostream>
#include <type_traits>
#if _HAS_CXX17
#include <memory_resource>
#endif

namespace hzw
{
	namespace detail
	{
		//��������
		constexpr void* (*MALLOC)(size_t) = ::operator new; //�ڴ�Դ
		constexpr void(*FREE)(void*) = ::operator delete; //�ڴ�Դ
		constexpr size_t(*MALLOC_SIZE)(void*) = _msize; //��MALLOC��Ӧ����ȡ�ڴ���С
		constexpr size_t MEMORYPOOL_ALIGN_SIZE = 0; //�ڴ�ض���ϵ����0��Ĭ�Ͻ����ڴ棬����֤���롢4��32λ���롢8��64λ���룩

		//�������
		constexpr size_t MEMORYPOOL_FORWARD_FLAG = 0; //�ڴ�����ת����ǣ�ת�����ڴ�Դ��
		constexpr const char* MEMORYPOOL_FLAG = "from hzw::MemoryPool"; //�ڴ�ر�ʶ�������ڴ�Դ���ڴ���ڴ棩

		//BufferMemoryPoolManage����
		constexpr bool BUFFER_MANAGE_ENABLE = false;
		constexpr size_t BUFFER_MEMORYPOOL_MANAGE_BUFFER_SIZE = 1000;

		//DebugInfo����
#define DI_LOCATION "FILE:", __FILE__, "  LINE:", __LINE__, "  " //�ڴ�����λ��
#define DI_IGNORE "IGNORE" //���Դ��ڴ��й©����
		constexpr const char* DI_NULL = "NULL"; //�޵�����Ϣ
		constexpr const char* DI_IGNORE_CHECK = "[IGNORE]"; //����й©���Ա�ǣ�MemoryPoolDebug::~DebugCookieHeadʹ�ã�debuginfo��[]������

		//����У��
		static_assert(sizeof(void*) == 4 || sizeof(void*) == 8, "memory pool is only available for 32 / 64 bit systems");
		static_assert(MEMORYPOOL_ALIGN_SIZE == 0 || MEMORYPOOL_ALIGN_SIZE == 4 || MEMORYPOOL_ALIGN_SIZE == 8, "MEMORYPOOL_ALIGN_SIZE invalid");

#define MAKE_NATIVE_STR(...) make_native_str<NativeStr<0>::get_native_str_size(__VA_ARGS__) + 1>(__VA_ARGS__)

		//��ԭ���ַ����ķ�װ�����ڱ�����ƴ��ԭ���ַ�����
		template<size_t Size>
		class NativeStr
		{
		public:
			template<typename... NativeStrs>
			constexpr NativeStr(NativeStrs... strs) : _buf{}
			{
				init_data(0, strs...);
				_buf[Size - 1] = '\0';
			}

			constexpr const char* c_str()const
			{
				return _buf;
			}

			constexpr static size_t get_native_str_size(const char* str)
			{
				size_t size{ 0 };
				for (; *str; ++str, ++size);
				return size;
			}

			template<typename... NativeStrs>
			constexpr static size_t get_native_str_size(const char* str, NativeStrs... strs)
			{
				return get_native_str_size(str) + get_native_str_size(strs...);
			}

		private:
			template<typename... NativeStrs>
			constexpr void init_data(size_t index, const char* str, NativeStrs... strs)
			{
				size_t endIndex{ index + get_native_str_size(str) };
				for (size_t bufIndex{ index }, strIndex{ 0 }; bufIndex < endIndex; ++bufIndex, ++strIndex)
					_buf[bufIndex] = str[strIndex];
				init_data(endIndex, strs...);
			}
			constexpr void init_data(size_t) {}

		private:
			char _buf[Size];
		};

		template<size_t Size, typename... NativeStrs>
		constexpr auto make_native_str(NativeStrs... strs)
		{
			return NativeStr<Size>{strs...};
		}

		template<bool SupportPolym, bool SupportChunk, size_t MaxSize>
		struct MemoryPool
		{
			using support_polym = std::bool_constant<SupportPolym>;
			using support_chunk = std::bool_constant<SupportChunk>;
			enum { MAX_SIZE = MaxSize };//���������ڴ�

			MemoryPool() = default;

			MemoryPool(const MemoryPool&) = delete;

			MemoryPool(MemoryPool&&) = default;

			MemoryPool& operator=(const MemoryPool&) = delete;

			MemoryPool& operator=(MemoryPool&&) = default;

			~MemoryPool() = default;
		};

		//vector��������������Դ
		template<typename T>
		class Vector final
		{
		public:
			Vector() : _begin{ static_cast<T*>(MALLOC(2 * sizeof(T))) }, _end{ _begin + 2 }, _cur{ _begin }{ }

			Vector(const Vector&) = delete;

			Vector(Vector&&) = delete;

			Vector& operator=(const Vector&) = delete;

			Vector& operator=(Vector&&) = delete;

			~Vector() = default;//��������Դ

			template<typename... Args>
			void push_back(Args&& ... args)
			{
				if (capacity() <= 0) expension();
				new(_cur++) T(std::forward<Args>(args)...);
			}

			void pop_back()
			{
				assert(size() > 0);
				(--_cur)->~T();
			}

			T& back()
			{
				assert(size() > 0);
				return *(_cur - 1);
			}

			size_t size() const
			{
				return _cur - _begin;
			}

			bool empty() const
			{
				return size() == 0;
			}

		private:
			void expension()
			{
				size_t sz{ size() }, newSz{ static_cast<size_t>(sz * 1.5) };
				T* newBegin{ static_cast<T*>(MALLOC(newSz * sizeof(T))) };
				memcpy(newBegin, _begin, sz * sizeof(T));
				FREE(_begin);
				_begin = newBegin, _end = newBegin + newSz, _cur = newBegin + sz;
			}

			size_t capacity() const
			{
				return _end - _cur;
			}

		private:
			T* _begin, * _end, * _cur;
		};

		//��������������������
		template<typename T, typename MemoryPoolManage>
		class Allocator final
		{
		public:
			using value_type = T;
			using pointer = T*;
			using const_pointer = const T*;
			using reference = T&;
			using const_reference = const T&;
			using size_type = size_t;
			using difference_type = ptrdiff_t;

			value_type* allocate(size_t num)
			{
				return static_cast<value_type*>(MemoryPoolManage::allocate(num * sizeof(value_type), DI_IGNORE));
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

		template<typename T, typename Alloc>
		struct MemoryPoolDeleteFun
		{
			void operator()(T* p)const
			{
				p->~T();
				Alloc alloc;
				alloc.deallocate(p, 1);
			}
		};

		template<typename T, template<typename T> typename Alloc, typename... Args>
		auto make_unique_using_alloc(Args&&... args)
		{
			using alloc_t = Alloc<T>;
			alloc_t alloc{};
			std::unique_ptr<T, MemoryPoolDeleteFun<T, alloc_t>> p{ alloc.allocate(1) };
			new(p.get()) T(std::forward<Args>(args)...);
			return p;
		}

		template<typename T, template<typename T> typename Alloc, typename... Args>
		auto make_shared_using_alloc(Args&&... args)
		{
			using alloc_t = Alloc<T>;
			alloc_t alloc;
			std::shared_ptr<T> p(alloc.allocate(1), MemoryPoolDeleteFun<T, alloc_t>{}, alloc);
			new(p.get()) T(std::forward<Args>(args)...);
			return p;
	}

#if _HAS_CXX17
		//�ڴ���Դ����������std::pmr��������c++17�����ã�
		template<typename MemoryPoolManage>
		class MemoryResource final : public std::pmr::memory_resource
		{
		private:
			void* do_allocate(size_t _Bytes, size_t _Align)override
			{
				return MemoryPoolManage::allocate(_Bytes + _Align);
			}

			void do_deallocate(void* _Ptr, size_t _Bytes, size_t _Align)override
			{
				MemoryPoolManage::deallocate(_Ptr, _Bytes + _Align);
			}

			bool do_is_equal(const memory_resource& _That) const noexcept override
			{
				return this == &_That;
			}
		};

		template<typename MemoryPoolManage>
		MemoryResource<MemoryPoolManage>* get_memory_resource()
		{
			static MemoryResource<MemoryPoolManage> resource;
			return &resource;
		}
#endif // _HAS_CXX17

		//�����û��Զ�����
		//�ڴ��֧�ִ��ڴ����֧�ֶ�̬operator[]�����ڴ�ؽӹܣ�����ֻ�ӹ�operator
		template<typename MemoryPoolManage>
		class UseMemoryPool
		{
			using support_polym = typename MemoryPoolManage::support_polym;
			using support_chunk = typename MemoryPoolManage::support_chunk;

		public:
			template<typename... DebugInfos>
			static void* operator new(size_t size, DebugInfos&&...debugInfos)
			{
				return MemoryPoolManage::allocate(size, std::forward<DebugInfos>(debugInfos)...);
			}

			static void operator delete(void* p, size_t size)
			{
				deallocate(p, size, support_polym{});
			}

			template<typename... DebugInfos>
			static void* operator new[](size_t size, DebugInfos&&... debugInfos)
			{
				using type = std::bool_constant<support_chunk::value&& support_polym::value>;
				return array_new(size, type{}, std::forward<DebugInfos>(debugInfos)...);
			}

			static void operator delete[](void* p, size_t size)
			{
				using type = std::bool_constant<support_chunk::value&& support_polym::value>;
				array_delete(p, size, type{});
			}

		protected:
			//��ֹʹ�ô���ָ����������ࣨ����deleteʱ��ʾ�������
			~UseMemoryPool() = default;

		private:
			//֧�ִ�������֧�ֶ�̬
			template<typename... DebugInfos>
			static void* array_new(size_t size, std::true_type, DebugInfos&&... debugInfos)
			{
				return operator new(size, std::forward<DebugInfos>(debugInfos)...);
			}

			//��֧�ִ�������̬
			template<typename... DebugInfos>
			static void* array_new(size_t size, std::false_type, DebugInfos&&...)
			{
				return MALLOC(size);
			}

			//֧�ִ�������֧�ֶ�̬
			static void array_delete(void* p, size_t size, std::true_type)
			{
				operator delete(p, size);
			}

			//��֧�ִ�������̬
			static void array_delete(void* p, size_t, std::false_type)
			{
				FREE(p);
			}

			//��֧�ֶ�̬
			static void deallocate(void* p, size_t size, std::false_type)
			{
				MemoryPoolManage::deallocate(p, size);
			}

			//֧�ֶ�̬
			static void deallocate(void* p, size_t, std::true_type)
			{
				MemoryPoolManage::deallocate(p);
			}
		};

		//memorypool_log_handle
		using memorypool_log_handle = std::ostream& (*)();
		static std::ostream& memorypool_default_log_handle()
		{
			return std::cerr;
		}
		static memorypool_log_handle _memorypoolLogHandle = memorypool_default_log_handle;
	}

	//memorypool_log_handle = std::ostream& (*)();
	static detail::memorypool_log_handle set_memorypool_log_handle(detail::memorypool_log_handle handle)
	{
		detail::memorypool_log_handle res{ detail::_memorypoolLogHandle };
		detail::_memorypoolLogHandle = handle;
		return res;
	}
}