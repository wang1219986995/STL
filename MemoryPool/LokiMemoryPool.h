#ifndef LOKIMEMORYPOOL_H
#define LOKIMEMORYPOOL_H

#include <vector>
#include <cassert>

/*
	洛基内存池
	此内存池灵感来由于Loki库并加以改进，使用一些小诡计实现并提供动态收缩扩张能力，固名洛基
	洛基提供动态收缩扩张功能更适合突发性内存需求
*/

namespace hzw
{
	//内存块
	class _Chunk
	{
	public:
		enum { MAX_BLOCK_SIZE = 128 };

	public:
		//输入：内存块大小、内存块个数
		_Chunk(size_t size, unsigned char blockSize) :
			_buf{ new unsigned char[size * blockSize] }, _index{ 0 }, _count{ blockSize }, _blockSize{ blockSize }
		{
			unsigned char i{ 1 };
			unsigned char* p{ _buf };
			for (; i < blockSize; ++i, p += size) *p = i;
		}

		_Chunk(const _Chunk&) = delete;

		_Chunk(_Chunk&& rh)noexcept :
			_buf{ rh._buf }, _index{ rh._index }, _count{ rh._count }, _blockSize{ rh._blockSize }
		{
			rh._buf = nullptr;
		}

		~_Chunk()noexcept
		{
			delete[] _buf;
		}

		void* allocate(size_t size)
		{
			unsigned char* result{ _buf + size * _index };
			_index = *result;
			--_count;
			return result;
		}

		void deallocate(void* p, size_t size)
		{
			unsigned char* cp{ static_cast<unsigned char*>(p) };
			*cp = _index;
			_index = (cp - _buf) / size;
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
				std::swap(_buf, rh._buf);
				std::swap(_index, rh._index);
				std::swap(_count, rh._count);
				std::swap(_blockSize, rh._blockSize);
			}
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
	public:
		_ChunkChain()noexcept :
			_chunks{}, _allocateIt{}, _deallocateIt{}, _allocateState{ true }{}

		_ChunkChain(const _ChunkChain&) = delete;

		_ChunkChain(_ChunkChain&&) = default;

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
				auto toEnd{ std::next(_allocateIt) };
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
				_chunks.emplace_back(size, blockSize < _Chunk::MAX_BLOCK_SIZE ? blockSize : _Chunk::MAX_BLOCK_SIZE);
				_allocateIt = std::prev(_chunks.end());
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
					_chunks.pop_back();
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
				if (!_chunks.empty()) _chunks.back().swap(*_deallocateIt);
			}
		}

	private:
		using ChunksIterator = std::vector<_Chunk>::iterator;
		std::vector<_Chunk> _chunks;//内存链
		ChunksIterator _allocateIt, _deallocateIt;//上一次分配、回收的位置
		bool _allocateState;//连续分配状态
	};

	//洛基内存池
	template<bool SupportPolym>
	class LokiMemoryPool
	{
	public:
		LokiMemoryPool() : _bpool(CHAIN_SIZE), _spool(3){}

		LokiMemoryPool(const LokiMemoryPool&) = delete;

		LokiMemoryPool(LokiMemoryPool&&) = default;

		void* allocate(size_t size)
		{
			size_t alSize{ align_size(size) };
			return _allocate(alSize, std::bool_constant<SupportPolym>{});
		}

		void deallocate(void* p, size_t size)
		{
			_deallocate(p, size, std::bool_constant<SupportPolym>{});
		}

	public:
		enum { CHAIN_SIZE = 32, PER_CHAIN_SIZE = 4, MAX_SIZE = CHAIN_SIZE * PER_CHAIN_SIZE };
		//内存池链的个数  //内存池粒度 //管理的最大内存 

	private:
		//功能：allocate辅助函数（支持多态）
		void* _allocate(size_t size, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(search_chain(size).allocate(size)) };
			*cookie = static_cast<unsigned char>(size);
			return cookie + 1;
		}

		//功能：allocate辅助函数（不支持多态）
		void* _allocate(size_t size, std::false_type)
		{
			return search_chain(size).allocate(size);
		}

		//功能：deallocate辅助函数（支持多态）
		void _deallocate(void* p, size_t, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(p) - 1 };
			search_chain(*cookie).deallocate(cookie, *cookie);
		}

		//功能：deallocate辅助函数（不支持多态）
		void _deallocate(void* p, size_t size, std::false_type)
		{
			size_t alSize{ align_size(size) };
			search_chain(alSize).deallocate(p, alSize);
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
		std::vector<_ChunkChain> _bpool;//负责4-128byte
		std::vector<_ChunkChain> _spool;//负责1、2、3byte
	};
}

#endif // !LOKIMEMORYPOOL_H



