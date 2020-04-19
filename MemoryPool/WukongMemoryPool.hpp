#pragma once

#include "MemoryPoolManage.hpp"

/* ����ڴ��
* �ṩ�ڴ���С��1-128byte
* ��̬��������֧��
*/

namespace hzw
{
	namespace detail
	{
		//�ڴ�����SupportPolym == false, CookieType��Ч��
		template<bool SupportPolym, typename CookieType>
		class _ListChain
		{
			enum { POINT_SIZE = sizeof(void*) };
		public:
			enum 
			{
				ALIGN_SIZE = !MEMORYPOOL_ALIGN_SIZE ? POINT_SIZE :
				MEMORYPOOL_ALIGN_SIZE < POINT_SIZE ? POINT_SIZE : MEMORYPOOL_ALIGN_SIZE
			};

		public:
			_ListChain() : _size{ 0 }, _head{ nullptr }{}

			_ListChain(_ListChain&) = delete;

			_ListChain(_ListChain&&) = delete;

			_ListChain& operator=(_ListChain&) = delete;

			_ListChain& operator=(_ListChain&&) = delete;

			~_ListChain() = default;

			void* allocate(size_t size)
			{
				if (_head == nullptr) fill_chain(size); //�ڴ���Ϊ�������
				return remove_from_chain();
			}

			void deallocate(void* p)
			{
				add_to_chain(p);
			}

			//���ܣ���ȡcookie��ֵ
			template<typename = std::enable_if_t<SupportPolym>>
			static size_t get_cookie(void* p)
			{
				CookieType* cookie{ reinterpret_cast<CookieType*>(static_cast<char*>(p) - COOKIE_SIZE) };
				return *cookie;
			}

			//���ܣ���ȡ�ڴ���С
			template<typename = std::enable_if_t<SupportPolym>>
			static size_t get_malloc_size(void* p)
			{
				return get_cookie(p) == MEMORYPOOL_FORWARD_FLAG ?
					MALLOC_SIZE(static_cast<char*>(p) - COOKIE_SIZE) - COOKIE_SIZE :
					get_cookie(p);
			}

			//���ܣ�����MAX_SIZE���󣬵���MALLOC
			static void* forward_allocate(size_t size)
			{
				char* res{ static_cast<char*>(MALLOC(size + COOKIE_SIZE)) };
				set_cookie(res, MEMORYPOOL_FORWARD_FLAG, support_polym{});
				return res + COOKIE_SIZE;
			}

			//���ܣ�����MAX_SIZE���󣬵���FREE
			static void forward_deallocate(void* p)
			{
				FREE(static_cast<char*>(p) - COOKIE_SIZE);
			}

		private:
			struct Node;
			static_assert(MEMORYPOOL_ALIGN_SIZE == 0 || MEMORYPOOL_ALIGN_SIZE >= sizeof(CookieType), "cookie size must less than align size");

			using support_polym = std::bool_constant<SupportPolym>;
			enum
			{
				COOKIE_SIZE = SupportPolym ? (!MEMORYPOOL_ALIGN_SIZE ? sizeof(CookieType) : MEMORYPOOL_ALIGN_SIZE) : 0,
				MAX_SPLIT_SIZE = 128, MIN_SPLIT_SIZE = 20
			};

			//���ܣ�����cookie
			//���룺cookie��ַ���ڴ������С
			static void set_cookie(char* cookieP, size_t size, std::true_type)
			{
				CookieType* cookie{ reinterpret_cast<CookieType*>(cookieP) };
				*cookie = static_cast<CookieType>(size);
			}

			static void set_cookie(char*, size_t, std::false_type) {}

			//���ܣ�����ڴ�鵽�ڴ���
			//���룺��ӵ��ڴ��
			void add_to_chain(void* p)
			{
				--_size;
				Node* np{ static_cast<Node*>(p) };
				np->next = _head;
				_head = np;
			}

			//���ܣ����ڴ����Ƴ��ڴ��
			//������ڴ��
			void* remove_from_chain()
			{
				++_size;
				void* result{ _head };
				_head = _head->next;
				return result;
			}

			//���ܣ�����ڴ���
			//���룺�ڴ������С
			void fill_chain(size_t size)
			{
				//�����ڴ��С
				size = align_size(size);
				//��̬�����и����
				size_t splitSize{ MIN_SPLIT_SIZE + (_size >> 5) };
				splitSize = splitSize < MAX_SPLIT_SIZE ? splitSize : MAX_SPLIT_SIZE;
				//�����и���ڴ�
				char* chunk{ static_cast<char*>(MALLOC(splitSize* size)) };
				//�и�chunk
				char* p{ chunk + COOKIE_SIZE };
				for (size_t i{ 1 }; i < splitSize; ++i, p += size)
				{
					reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
					set_cookie(p - COOKIE_SIZE, size - COOKIE_SIZE, support_polym{});
				}
				reinterpret_cast<Node*>(p)->next = nullptr;
				set_cookie(p - COOKIE_SIZE, size - COOKIE_SIZE, support_polym{});
				//�ҵ��ڴ�����
				_head = reinterpret_cast<Node*>(chunk + COOKIE_SIZE);
			}

			//���ܣ������ڴ������С
			static size_t align_size(size_t size)
			{
				return COOKIE_SIZE + (size + ALIGN_SIZE - 1 & ~(ALIGN_SIZE - 1));
			}

		private:
			struct Node
			{
				Node* next;
			};

			size_t _size;//��ǰ�ڴ������ڴ����
			Node* _head;//�ڴ���ͷ�ڵ�
		};

		//����ڴ��
		template<bool SupportPolym>
		class WukongMemoryPool final : public MemoryPool<SupportPolym, false, 128>
		{
			using my_base = MemoryPool<SupportPolym, false, 128>;

		public:
			WukongMemoryPool() : _pool(CHAIN_LENTH) {}

			void* allocate(size_t size)
			{
				size = align_size(size);
				if (size <= my_base::MAX_SIZE) return _pool[get_index(size)].allocate(size);
				else return list_chain::forward_allocate(size);
			}

			void deallocate(void* p, size_t size)
			{
				_deallocate(p, size, typename my_base::support_polym{});
			}

			static size_t malloc_size(void* p)
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				return list_chain::get_malloc_size(p);
			}

			constexpr static const char* get_pool_name()
			{
				return "Wk";
			}

		private:
			using cookie_type = uint8_t;
			using list_chain = _ListChain<SupportPolym, cookie_type>;
			enum { ALIGN_SIZE = list_chain::ALIGN_SIZE, CHAIN_LENTH = my_base::MAX_SIZE / ALIGN_SIZE };

			//���ܣ�_deallocate����������֧�ֶ�̬��
			void _deallocate(void* p, size_t, std::true_type)
			{
				size_t size{ list_chain::get_cookie(p) };
				size == MEMORYPOOL_FORWARD_FLAG ?
					list_chain::forward_deallocate(p) :
					_pool[get_index(size)].deallocate(p);
			}

			//���ܣ�_deallocate������������֧�ֶ�̬��
			void _deallocate(void* p, size_t size, std::false_type)
			{
				size = align_size(size);
				size > my_base::MAX_SIZE ?
					list_chain::forward_deallocate(p) :
					_pool[get_index(size)].deallocate(p);
			}

			static size_t get_index(size_t size)
			{
				return size / ALIGN_SIZE - 1;
			}

			static size_t align_size(size_t size)
			{
				return size + ALIGN_SIZE - 1 & ~(ALIGN_SIZE - 1);
			}

		private:
			std::vector<list_chain> _pool;//�ڴ��
		};

		template<bool SupportPolym>//����ڴ�ر���
		using Wk = WukongMemoryPool<SupportPolym>;
	}
	
	using WkG = detail::GMM<detail::Wk<false>>;//��գ���֧�ֶ�̬��ȫ�ֹ�����
	using WkT = detail::TMM<detail::Wk<false>>;//��գ���֧�ֶ�̬���̹߳�����
	using WkGP = detail::GMM<detail::Wk<true>>;//��գ�֧�ֶ�̬��ȫ�ֹ�����
	using WkTP = detail::TMM<detail::Wk<true>>;//��գ�֧�ֶ�̬���̹߳�����

	//��շ���������
	template<typename T>
	using AllocWkG = detail::Allocator<T, WkG>;
	template<typename T>
	using AllocWkT = detail::Allocator<T, WkT>;

	template<typename T, typename... Args>
	auto make_unique_wkg(Args&&... args)
	{
		return detail::make_unique_using_alloc<T, AllocWkG>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto make_unique_wkt(Args&&... args)
	{
		return detail::make_unique_using_alloc<T, AllocWkT>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto make_shared_wkg(Args&&... args)
	{
		return detail::make_shared_using_alloc<T, AllocWkG>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto make_shared_wkt(Args&&... args)
	{
		return detail::make_shared_using_alloc<T, AllocWkT>(std::forward<Args>(args)...);
	}

#if _HAS_CXX17
	//���MemoryResource
	detail::MemoryResource<WkG>* MRWkG()
	{
		return detail::get_memory_resource<WkG>();
	}
	detail::MemoryResource<WkT>* MRWkT()
	{
		return detail::get_memory_resource<WkT>();
	}
#endif
	
	using UseWkG = detail::UseMemoryPool<WkG>;
	using UseWkT = detail::UseMemoryPool<WkT>;
	using UseWkGP = detail::UseMemoryPool<WkGP>;
	using UseWkTP = detail::UseMemoryPool<WkTP>;
}