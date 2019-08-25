#ifndef LOKIMEMORYPOOL_H
#define LOKIMEMORYPOOL_H

#include "MemoryPoolManage.h"

/*
	洛基内存池
	此内存池灵感来由于Loki库并加以改进，使用一些小诡计实现并提供动态收缩扩张能力，固名洛基
	洛基提供动态收缩扩张功能更适合突发性内存需求
*/

namespace hzw
{
	//内存块（Loki和Nvwa使用）
	template<bool SupportCookie>
	class _Chunk
	{
	public:
		enum { MAX_BLOCK_SIZE = 128 };

	public:
		//输入：内存块大小、内存块个数
		_Chunk(size_t size, unsigned char blockSize) :
			_buf{ static_cast<unsigned char*>(::operator new((size + SupportCookie) * blockSize)) }, _index{ 0 }, _count{ blockSize }, _blockSize{ blockSize }
		{
			unsigned char i{ 1 };
			unsigned char* p{ _buf };
			for (; i < blockSize; ++i, p += (size + SupportCookie))* p = i;
		}

		_Chunk(const _Chunk&) = delete;

		_Chunk(_Chunk&& rh)noexcept :
			_buf{ rh._buf }, _index{ rh._index }, _count{ rh._count }, _blockSize{ rh._blockSize }
		{
			rh._buf = nullptr;
		}

		_Chunk& operator=(const _Chunk&) = delete;

		_Chunk& operator=(_Chunk&&) = delete;

		~_Chunk()noexcept
		{
			::operator delete(_buf);
		}

		void* allocate(size_t size)
		{
			unsigned char* result{ _buf + (size + SupportCookie) * _index };
			_allocate(result, bool_constant<SupportCookie>{});
			--_count;
			return result + SupportCookie;
		}

		void deallocate(void* p, size_t size)
		{
			unsigned char* cp{ static_cast<unsigned char*>(p) - SupportCookie };
			*cp = _index;
			_index = (cp - _buf) / (size + SupportCookie);
			++_count;
		}

		//功能：内存块未被使用
		bool is_empty()
		{
			return _count == _blockSize;
		}

		//功能：内存块全部使用
		bool is_full()
		{
			return !_count;
		}

		//功能：判断指针是否属于本chunk
		bool is_contain(void* p, size_t size)
		{
			return static_cast<unsigned char*>(p) - _buf <= size * (_blockSize - 1);
		}

		void swap(_Chunk& rh)
		{
			if (&rh != this)
			{
				using std::swap;
				swap(_buf, rh._buf);
				swap(_index, rh._index);
				swap(_count, rh._count);
				swap(_blockSize, rh._blockSize);
			}
		}

		//功能：返回可用块计数
		unsigned char unuse_count()
		{
			return _count;
		}

		//功能：返回内存块地址
		unsigned char* buf_point()
		{
			return _buf;
		}

		//功能：读取cookie
		static unsigned char* cookie(void* p, size_t size)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(p) - 1 };
			return cookie - (size + 1) * (*cookie);
		}

	private:
		//功能：allocate辅助函数（支持cookie）
		void _allocate(unsigned char* result, true_type)
		{
			::std::swap(_index, *result);
		}

		//功能：allocate辅助函数（不支持cookie）
		void _allocate(unsigned char* result, false_type)
		{
			_index = *result;
		}

	private:
		unsigned char* _buf;//内存块
		unsigned char _index;//可用块索引
		unsigned char _count;//可用块计数
		unsigned char _blockSize;//内存块个数
	};

	//内存链
	class _ChunkChain
	{
		using Chunk = _Chunk<false>;

	public:
		_ChunkChain()noexcept :
			_chunks{}, _allocateIt{}, _deallocateIt{}, _allocateState{ true }{}

		_ChunkChain(const _ChunkChain&) = delete;

		_ChunkChain(_ChunkChain&&) = default;

		_ChunkChain& operator=(const _ChunkChain&) = delete;

		_ChunkChain& operator=(_ChunkChain&&) = delete;

		void* allocate(size_t size)
		{
			size_t chunksSize{ _chunks.size() };
			if (!chunksSize) goto fill;//chunks为空则填充				
			if (!_allocateState && _allocateIt->is_full())//非连续分配状态和上次分配位置无空闲，则重新查找空闲块
			{
				/*
				查询空闲内存块
				从上一次分配处向头尾逼近查找
				利用局部性原理，适配位置极可能就在失配处附近
				*/
				auto toBegin{ _allocateIt };
				auto toEnd{ next(_allocateIt) };
				bool atBegin{ false };
				while (!atBegin || toEnd != _chunks.end())//没到头或没到尾继续查找
				{
					if (!atBegin)
					{
						if (toBegin != _chunks.begin())//没到头部
						{
							if (!toBegin->is_full())
							{
								_allocateIt = toBegin;
								break;
							}
							--toBegin;
						}
						else//到达头部
						{
							atBegin = true;
							if (!toBegin->is_full())
							{
								_allocateIt = toBegin;
								break;
							}
						}
					}
					if (toEnd != _chunks.end())
					{
						if (!toEnd->is_full())
						{
							_allocateIt = toEnd;
							break;
						}
						++toEnd;
					}
				}
			}
			//查无空闲内存块则填充
			if (_allocateIt->is_full())
			{
				_allocateState = true;
			fill:
				size_t blockSize{ 20 + (chunksSize >> 1) };//动态设定blockSize
				_chunks.emplace_back(size, blockSize < Chunk::MAX_BLOCK_SIZE ? blockSize : Chunk::MAX_BLOCK_SIZE);
				_allocateIt = prev(_chunks.end());
			}
			_deallocateIt = _allocateIt;//1.下次内存回收位置可能就是当前分配位置 2.迭代器可能失效
			return _allocateIt->allocate(size);
		}

		void deallocate(void* p, size_t size)
		{
			if (!_deallocateIt->is_contain(p, size))//失配则重新查找
			{
				//查找对应的chunk（与allocate查找原理相同）
				auto toBegin{ _deallocateIt };
				auto toEnd{ _deallocateIt };
				bool atBegin{ false };
				for (;;)
				{
					if (!atBegin)
					{
						if (toBegin != _chunks.begin())
						{
							if (toBegin->is_contain(p, size))
							{
								_deallocateIt = toBegin;
								break;
							}
							--toBegin;
						}
						else
						{
							atBegin = true;
							if (toBegin->is_contain(p, size))
							{
								_deallocateIt = toBegin;
								break;
							}
						}
					}						
					if (toEnd != _chunks.end())
					{
						if (toEnd->is_contain(p, size))
						{
							_deallocateIt = toEnd;
							break;
						}
						++toEnd;
					}
					assert(!atBegin || toEnd != _chunks.end());//内存块不属于此内存池 或 delete指针与指向类型不匹配
				}
			}
			_allocateIt = _deallocateIt;
			_deallocateIt->deallocate(p, size);
			_allocateState = false;

			if (_deallocateIt->is_empty())//chunk为空移动到chunks最后
			{
				if (_chunks.back().is_empty())//有两个全空chunk，释放旧的一个
				{
					if (_deallocateIt != prev(_chunks.end(), 1)) _chunks.pop_back();
					if (_chunks.size() < (_chunks.capacity() >> 1))
					{
						ChunksIterator::difference_type allocateItIndex{ _allocateIt - _chunks.begin() };
						ChunksIterator::difference_type deallocateItIndex{ _deallocateIt - _chunks.begin() };
						_chunks.shrink_to_fit();//收缩_chunks
						//收缩可能导致原有迭代器失效，固重新赋值
						_allocateIt = _chunks.begin() + allocateItIndex;
						_deallocateIt = _chunks.begin() + deallocateItIndex;
					}
				}
				_chunks.back().swap(*_deallocateIt);
			}
		}

	private:
		using ChunksIterator = vector<Chunk>::iterator;
		vector<Chunk> _chunks;//内存链
		ChunksIterator _allocateIt, _deallocateIt;//上一次分配、回收的位置
		bool _allocateState;//连续分配状态
	};

	//洛基内存池
	template<bool SupportPolym>
	class LokiMemoryPool
	{
	public:
		LokiMemoryPool() : _bpool(CHAIN_SIZE), _spool(3) {}

		LokiMemoryPool(const LokiMemoryPool&) = delete;

		LokiMemoryPool(LokiMemoryPool&&) = default;

		LokiMemoryPool& operator=(const LokiMemoryPool&) = delete;

		LokiMemoryPool& operator=(LokiMemoryPool&&) = default;

		~LokiMemoryPool() = default;

		void* allocate(size_t size)
		{
			return _allocate(align_size(size), bool_constant<SupportPolym>{});
		}

		void deallocate(void* p, size_t size)
		{
			_deallocate(p, size, bool_constant<SupportPolym>{});
		}

	private:
		enum { CHAIN_SIZE = 32, PER_CHAIN_SIZE = 4, MAX_SIZE = CHAIN_SIZE * PER_CHAIN_SIZE };
		//内存池链的个数  //内存池粒度 //管理的最大内存 

		//功能：allocate辅助函数（支持多态）
		void* _allocate(size_t size, true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(size > MAX_SIZE ? 
				::operator new(size) : search_chain(size).allocate(size)) };
			*cookie = static_cast<unsigned char>(size);
			return cookie + 1;
		}

		//功能：allocate辅助函数（不支持多态）
		void* _allocate(size_t size, false_type)
		{
			return size > MAX_SIZE ? ::operator new(size) : search_chain(size).allocate(size);
		}

		//功能：deallocate辅助函数（支持多态）
		void _deallocate(void* p, size_t, true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(p) - 1 };
			unsigned char cookieValue{ *cookie };
			cookieValue > MAX_SIZE ? ::operator delete(cookie) :
				search_chain(cookieValue).deallocate(cookie, cookieValue);
		}

		//功能：deallocate辅助函数（不支持多态）
		void _deallocate(void* p, size_t size, false_type)
		{
			size = align_size(size);
			size > MAX_SIZE ? ::operator delete(p) :
				search_chain(size).deallocate(p, size);
		}

		//功能：查找对应chunks
		//输入：align_size后的大小
		_ChunkChain& search_chain(size_t size)
		{
			return size < 4 + SupportPolym ? _spool[size - 1 - SupportPolym] : _bpool[(size - SupportPolym >> 2) - 1];
		}

		//功能：内存需求与PER_CHAIN_SIZE对齐
		static size_t align_size(size_t size)
		{
			return (size < 4 ? size : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1))) + SupportPolym;
		}

	private:
		vector<_ChunkChain> _bpool;//负责4-128byte
		vector<_ChunkChain> _spool;//负责1、2、3byte
	};

	template<bool SupportPolym>
	using Lk = LokiMemoryPool<SupportPolym>;//洛基内存池别名

	using LkG = GMM<Lk<false>>;//洛基（不支持多态、全局归属）
	using LkT = TMM<Lk<false>>;//洛基（不支持多态、线程归属）
	using LkGP = GMM<Lk<true>>;//洛基（支持多态、全局归属）
	using LkTP = TMM<Lk<true>>;//洛基（支持多态、线程归属）

	//洛基分配器别名
	template<typename T>
	using AllocLkG = Allocator<T, LkG>;
	template<typename T>
	using AllocLkT = Allocator<T, LkT>;
	
	using UseLkG = UseMemoryPool<LkG>;
	using UseLkT = UseMemoryPool<LkT>;
	using UseLkGP = UseMemoryPool<LkGP>;
	using UseLkTP = UseMemoryPool<LkTP>;
}

#endif // !LOKIMEMORYPOOL_H



