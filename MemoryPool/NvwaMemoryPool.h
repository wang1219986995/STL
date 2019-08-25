#ifndef NVWAMEMORYPOOL_H
#define NVWAMEMORYPOOL_H

#include "LokiMemoryPool.h"
#include <map>
#include <set>

namespace hzw
{
	//��ǰ����Wukong��Nvwa��ʼ����ҪWukong�����뱣֤Loki�ȳ�ʼ����
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
			if (_freeChunks.empty())//û�п����ڴ��
			{
				size_t blockSize{ 10 + (_chunks.size() >> 1) };//��̬�趨blockSize
				Chunk chunk{ size, static_cast<unsigned char>(blockSize < Chunk::MAX_BLOCK_SIZE ? blockSize : Chunk::MAX_BLOCK_SIZE) };
				//���µ�chunk����_chunks��������_freeChunks
				_freeChunks.emplace(&_chunks.emplace(chunk.buf_point(), move(chunk)).first->second);
			}
			Chunk* curChunk{ *_freeChunks.begin() };
			void* res{ curChunk->allocate(size) };
			if (curChunk->is_full()) _freeChunks.erase(curChunk);//���ȼ����chunk�޿������Ƴ�
			return res;
		}

		void deallocate(void* p, size_t size)
		{
			//�˹�Ӧ���Ƿ��������ڴ��
			assert(_chunks.count(Chunk::cookie(p, size)) == 1);
			//���ж��Ƿ�����Ϊ�ϴλ��յ����飬���������²���
			unsigned char* cookie{ Chunk::cookie(p, size) };
			Chunk* curChunk{ _deallocateIt && _deallocateIt->buf_point() ==  cookie ? 
				_deallocateIt : &_chunks.find(cookie)->second };

			curChunk->deallocate(p, size);
			if (curChunk->is_empty())//chunkȫ���黹��ɾ��
			{
				_chunks.erase(curChunk->buf_point());
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

	//Ů��ڴ��
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

		//���ܣ�allocate����������֧�ֶ�̬��
		void* _allocate(size_t size, true_type)
		{
			size += sizeof(uint16_t);
			uint16_t* cookie{ static_cast<uint16_t*>(
				size > MAX_SIZE ? ::operator new(size) : _pool[size].allocate(size)) };
			*cookie = static_cast<uint16_t>(size);
			return cookie + 1;
		}

		//���ܣ�allocate������������֧�ֶ�̬��
		void* _allocate(size_t size, false_type)
		{
			return size > MAX_SIZE ? ::operator new(size) : _pool[size].allocate(size);
		}

		//���ܣ�deallocate����������֧�ֶ�̬��
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

		//���ܣ�deallocate������������֧�ֶ�̬��
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
			//��ϣ�����ڴ�أ���ϣ����������ڴ��
	};

	template<bool SupportPolym>
	using Nw = NvwaMemoryPool<SupportPolym>;//Ů��ڴ�ر���

	using NwG = GMM<Nw<false>>;// Ů洣���֧�ֶ�̬��ȫ�ֹ�����
	using NwT = TMM<Nw<false>>;//Ů洣���֧�ֶ�̬���̹߳�����
	using NwGP = GMM<Nw<true>>;//Ů洣�֧�ֶ�̬��ȫ�ֹ�����
	using NwTP = TMM<Nw<true>>;//Ů洣�֧�ֶ�̬���̹߳�����

	//Ů洷���������
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

