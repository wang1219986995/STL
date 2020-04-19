#pragma once

#include "LokiMemoryPool.hpp"
#include <set>

/* Ů��ڴ��
* �ṩ�ڴ���С��1-65535byte
* ��̬������֧��
*/

namespace hzw
{
	namespace detail
	{
		template<bool SupportPolym, typename CookieType, template<typename T> typename Alloc>
		class _ChunkTree
		{
			using Chunk = _Chunk<true, SupportPolym, CookieType>;

		public:
			enum { ALIGN_SIZE = Chunk::ALIGN_SIZE };

		public:
			_ChunkTree() : _chunks{}, _deallocateIt{ nullptr }{}

			_ChunkTree(const _ChunkTree&) = delete;

			_ChunkTree(_ChunkTree&&) = default;

			_ChunkTree& operator=(const _ChunkTree&) = delete;

			_ChunkTree& operator=(_ChunkTree&&) = delete;

			~_ChunkTree() = default;

			void* allocate(size_t size)
			{
				if (_freeChunks.empty())//û�п����ڴ��
				{
					Chunk chunk{ size, _chunks.size() >> 1 };
					//���µ�chunk����_chunks��������_freeChunks
					_freeChunks.emplace(&_chunks.emplace(chunk.get_buf_point(), std::move(chunk)).first->second);
				}
				Chunk* curChunk{ *_freeChunks.begin() };
				void* res{ curChunk->allocate(size) };
				if (curChunk->is_full()) _freeChunks.erase(curChunk);//���ȼ����chunk�޿������Ƴ�
				return res;
			}

			void deallocate(void* p, size_t size)
			{
				//�˹�Ӧ���Ƿ��������ڴ��
				assert(_chunks.count(Chunk::get_buf_point(p, size)) == 1);
				//���ж��Ƿ�����Ϊ�ϴλ��յ����飬���������²���
				void* cookie{ Chunk::get_buf_point(p, size) };
				Chunk* curChunk{ _deallocateIt && _deallocateIt->get_buf_point() == cookie ?
					_deallocateIt : &_chunks.find(cookie)->second };

				curChunk->deallocate(p, size);
				if (curChunk->is_empty())//chunkȫ���黹��ɾ��
				{
					_chunks.erase(curChunk->get_buf_point());
					_freeChunks.erase(curChunk);
					_deallocateIt = nullptr;//��Ϊ��Ч
				}
				else
				{
					_freeChunks.erase(curChunk);
					_freeChunks.emplace(curChunk);
					_deallocateIt = curChunk;
				}
			}

			bool is_empty()
			{
				return _chunks.empty();
			}

			static void* forward_allocate(size_t size)
			{
				return Chunk::forward_allocate(size);
			}

			static void forward_deallocate(void* p)
			{
				return Chunk::forward_deallocate(p);
			}

			static size_t get_cookie(void* p)
			{
				return Chunk::get_cookie(p);
			}

			static size_t get_malloc_size(void* p)
			{
				return Chunk::get_malloc_size(p);
			}

		private:
			struct CompareChunkPoint
			{
				bool operator()(Chunk* lh, Chunk* rh) const
				{
					return lh->unuse_count() < rh->unuse_count();
				}
			};

			std::map<void*, Chunk, std::less<void*>,
				Alloc<std::pair<void* const, Chunk>>> _chunks;
			std::set<Chunk*, CompareChunkPoint, Alloc<Chunk*>> _freeChunks;
			Chunk* _deallocateIt;
		};

		//Ů��ڴ��
		template<bool SupportPolym, template<typename T> typename Alloc>
		class NvwaMemoryPool final : public MemoryPool<SupportPolym, true, UINT16_MAX>
		{
			using my_base = MemoryPool<SupportPolym, true, UINT16_MAX>;

		public:
			void* allocate(size_t size)
			{
				size = align_size(size);
				return size <= my_base::MAX_SIZE ?
					_pool[size].allocate(size) :
					ChunkTree::forward_allocate(size);
			}

			void deallocate(void* p, size_t size)
			{
				_deallocate(p, size, typename my_base::support_polym{});
			}

			static size_t malloc_size(void* p)
			{
				static_assert(my_base::support_polym::value, "memory pool must support polym");
				return ChunkTree::get_malloc_size(p);
			}

			constexpr static const char* get_pool_name()
			{
				return "Nw";
			}

		private:
			using cookie_type = uint16_t;
			using ChunkTree = _ChunkTree<SupportPolym, cookie_type, Alloc>;

			//���������ڴ�, cookie��С
			enum { ALIGN_SIZE = ChunkTree::ALIGN_SIZE == 1 ? 4 : ChunkTree::ALIGN_SIZE };

			//���ܣ�deallocate����������֧�ֶ�̬��
			void _deallocate(void* p, size_t, std::true_type)
			{
				size_t cookieValue{ ChunkTree::get_cookie(p) };
				if (cookieValue == MEMORYPOOL_FORWARD_FLAG) ChunkTree::forward_deallocate(p);
				else
				{
					ChunkTree& curChunkTree{ _pool[cookieValue] };
					curChunkTree.deallocate(p, cookieValue);
					if (curChunkTree.is_empty()) _pool.erase(cookieValue);
				}
			}

			//���ܣ�deallocate������������֧�ֶ�̬��
			void _deallocate(void* p, size_t size, std::false_type)
			{
				size = align_size(size);
				if (size > my_base::MAX_SIZE) ChunkTree::forward_deallocate(p);
				else
				{
					ChunkTree& curChunkTree{ _pool[size] };
					curChunkTree.deallocate(p, size);
					if (curChunkTree.is_empty()) _pool.erase(size);
				}
			}

			static size_t align_size(size_t size)
			{
				return size + ALIGN_SIZE - 1 & ~(ALIGN_SIZE - 1);
			}

		private:
			std::map<size_t, ChunkTree, std::less<size_t>,
				Alloc<std::pair<const size_t, ChunkTree>>> _pool;
			//��ϣ�����ڴ�أ���ϣ����������ڴ��
		};

		template<bool SupportPolym, template<typename T> typename Alloc>
		using Nw = NvwaMemoryPool<SupportPolym, Alloc>;//Ů��ڴ�ر���
	}

	using NwG = detail::GMM<detail::Nw<false, AllocLkG>>;// Ů洣���֧�ֶ�̬��ȫ�ֹ�����
	using NwGP = detail::GMM<detail::Nw<true, AllocLkG>>;//Ů洣�֧�ֶ�̬��ȫ�ֹ�����
	using NwT = detail::TMM<detail::Nw<false, AllocLkT>>;//Ů洣���֧�ֶ�̬���̹߳�����
	using NwTP = detail::TMM<detail::Nw<true, AllocLkT>>;//Ů洣�֧�ֶ�̬���̹߳�����
	
	//Ů洷���������
	template<typename T>
	using AllocNwG = detail::Allocator<T, NwG>;
	template<typename T>
	using AllocNwT = detail::Allocator<T, NwT>;

	template<typename T, typename... Args>
	auto make_unique_nwg(Args&&... args)
	{
		return detail::make_unique_using_alloc<T, AllocNwG>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto make_unique_nwt(Args&&... args)
	{
		return detail::make_unique_using_alloc<T, AllocNwT>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto make_shared_nwg(Args&&... args)
	{
		return detail::make_shared_using_alloc<T, AllocNwG>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	auto make_shared_nwt(Args&&... args)
	{
		return detail::make_shared_using_alloc<T, AllocNwT>(std::forward<Args>(args)...);
	}

#if _HAS_CXX17
	//Ů�MemoryResource
	detail::MemoryResource<NwG>* MRNwG()
	{
		return detail::get_memory_resource<NwG>();
	}

	detail::MemoryResource<NwT>* MRNwT()
	{
		return detail::get_memory_resource<NwT>();
	}
#endif
	
	using UseNwG = detail::UseMemoryPool<NwG>;
	using UseNwT = detail::UseMemoryPool<NwT>;
	using UseNwGP = detail::UseMemoryPool<NwGP>;
	using UseNwTP = detail::UseMemoryPool<NwTP>;
}