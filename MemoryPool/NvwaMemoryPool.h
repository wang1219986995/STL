#ifndef NVWAMEMORYPOOL_H
#define NVWAMEMORYPOOL_H

#include "LokiMemoryPool.h"
#include <map>
#include <set>

namespace hzw
{
	//提前定义Wukong（Nvwa初始化需要Wukong，必须保证Loki先初始化）
	mutex GlobalMemoryPoolManage<LokiMemoryPool<false>>::_poolMutex;
	LokiMemoryPool<false> GlobalMemoryPoolManage<LokiMemoryPool<false>>::_memoryPool;

	class _ChunkTree
	{
		using Chunk = _Chunk<true>;

	public:
		_ChunkTree() : _chunks{}, _deallocateIt{nullptr}{}

		_ChunkTree(const _ChunkTree&) = delete;

		_ChunkTree(_ChunkTree&&) = default;

		_ChunkTree& operator=(const _ChunkTree&) = delete;

		_ChunkTree& operator=(_ChunkTree&&) = delete;

		void* allocate(size_t size)
		{
			if (_freeChunks.empty())//没有空余内存块
			{
				size_t blockSize{ 10 + (_chunks.size() >> 1) };//动态设定blockSize
				Chunk chunk{ size, static_cast<unsigned char>(blockSize < Chunk::MAX_BLOCK_SIZE ? blockSize : Chunk::MAX_BLOCK_SIZE) };
				//将新的chunk插入_chunks，并加入_freeChunks
				_freeChunks.emplace(&_chunks.emplace(chunk.buf_point(), move(chunk)).first->second);
			}
			Chunk* curChunk{ *_freeChunks.begin() };
			void* res{ curChunk->allocate(size) };
			if (curChunk->is_full()) _freeChunks.erase(curChunk);//优先级最高chunk无空闲则移除
			return res;
		}

		void deallocate(void* p, size_t size)
		{
			//此供应块是否来至本内存池
			assert(_chunks.count(Chunk::cookie(p, size)) == 1);
			//先判断是否正好为上次回收的区块，不是则重新查找
			unsigned char* cookie{ Chunk::cookie(p, size) };
			Chunk* curChunk{ _deallocateIt && _deallocateIt->buf_point() ==  cookie ? 
				_deallocateIt : &_chunks.find(cookie)->second };

			curChunk->deallocate(p, size);
			if (curChunk->is_empty())//chunk全部归还则删除
			{
				_chunks.erase(curChunk->buf_point());
				_freeChunks.erase(curChunk);
				_deallocateIt = nullptr;//置为无效
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

	private:
		struct CompareChunkPoint
		{
			bool operator()(Chunk* lh, Chunk* rh) const
			{
				return lh->unuse_count() < rh->unuse_count();
			}
		};

		map<unsigned char*, Chunk, less<unsigned char*>,
			AllocLkG<pair<unsigned char* const, Chunk>>> _chunks;
		set<Chunk*, CompareChunkPoint, AllocLkG<Chunk*>> _freeChunks;
		Chunk* _deallocateIt;
	};

	//女娲内存池
	template<bool SupportPolym>
	class NvwaMemoryPool
	{
	public:
		NvwaMemoryPool() = default;

		NvwaMemoryPool(const NvwaMemoryPool&) = delete;

		NvwaMemoryPool(NvwaMemoryPool&&) = default;

		NvwaMemoryPool& operator=(const NvwaMemoryPool&) = delete;

		NvwaMemoryPool& operator=(NvwaMemoryPool&&) = default;

		~NvwaMemoryPool() = default;

		void* allocate(size_t size)
		{
			return _allocate(size, bool_constant<SupportPolym>{});
		}

		void deallocate(void* p, size_t size)
		{
			_deallocate(p, size, bool_constant<SupportPolym>{});
		}

	private:
		enum { MAX_SIZE = UINT16_MAX };

		//功能：allocate辅助函数（支持多态）
		void* _allocate(size_t size, true_type)
		{
			size += sizeof(uint16_t);
			uint16_t* cookie{ static_cast<uint16_t*>(
				size > MAX_SIZE ? ::operator new(size) : _pool[size].allocate(size)) };
			*cookie = static_cast<uint16_t>(size);
			return cookie + 1;
		}

		//功能：allocate辅助函数（不支持多态）
		void* _allocate(size_t size, false_type)
		{
			return size > MAX_SIZE ? ::operator new(size) : _pool[size].allocate(size);
		}

		//功能：deallocate辅助函数（支持多态）
		void _deallocate(void* p, size_t, true_type)
		{
			uint16_t* cookie{ static_cast<uint16_t*>(p) - 1 };
			uint16_t cookieValue{ *cookie };
			if (cookieValue > MAX_SIZE) ::operator delete(cookie);
			else
			{
				_ChunkTree& curChunkTree{ _pool[cookieValue] };
				curChunkTree.deallocate(cookie, cookieValue);
				if (curChunkTree.is_empty()) _pool.erase(cookieValue);
			}			
		}

		//功能：deallocate辅助函数（不支持多态）
		void _deallocate(void* p, size_t size, false_type)
		{
			if (size > MAX_SIZE) ::operator delete(p);
			else
			{
				_ChunkTree& curChunkTree{ _pool[size] };
				curChunkTree.deallocate(p, size);
				if (curChunkTree.is_empty()) _pool.erase(size);
			}						
		}

	private:
		map<size_t, _ChunkTree, less<size_t>,
			AllocLkG<pair<const size_t, _ChunkTree>>> _pool;
			//哈希表构建内存池，哈希表本身用洛基内存池
	};

	template<bool SupportPolym>
	using Nw = NvwaMemoryPool<SupportPolym>;//女娲内存池别名

	using NwG = GMM<Nw<false>>;// 女娲（不支持多态、全局归属）
	using NwT = TMM<Nw<false>>;//女娲（不支持多态、线程归属）
	using NwGP = GMM<Nw<true>>;//女娲（支持多态、全局归属）
	using NwTP = TMM<Nw<true>>;//女娲（支持多态、线程归属）

	//女娲分配器别名
	template<typename T>
	using AllocNwG = Allocator<T, NwG>;
	template<typename T>
	using AllocNwT = Allocator<T, NwT>;
	
	using UseNwG = UseMemoryPool<NwG>;
	using UseNwT = UseMemoryPool<NwT>;	
	using UseNwGP = UseMemoryPool<NwGP>;
	using UseNwTP = UseMemoryPool<NwTP>;
}

#endif // !NVWAMEMORYPOOL_H

