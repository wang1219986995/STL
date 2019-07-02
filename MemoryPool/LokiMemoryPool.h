#ifndef LOKIMEMORYPOOL_H
#define LOKIMEMORYPOOL_H

#include <vector>
#include <cassert>

/*
	����ڴ��
	���ڴ�����������Loki�Ⲣ���ԸĽ���ʹ��һЩС���ʵ�ֲ��ṩ��̬���������������������
	����ṩ��̬�������Ź��ܸ��ʺ�ͻ�����ڴ�����
*/

namespace hzw
{
	//�ڴ��
	class _Chunk
	{
	public:
		enum { MAX_BLOCK_SIZE = 128 };

	public:
		//���룺�ڴ���С���ڴ�����
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

		//���ܣ��ڴ��δ��ʹ��
		bool is_empty()
		{
			return _count == _blockSize;
		}

		//���ܣ��ڴ��ȫ��ʹ��
		bool is_full()
		{
			return !_count;
		}

		//���ܣ��ж�ָ���Ƿ����ڱ�chunk
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
		unsigned char* _buf;//�ڴ��
		unsigned char _index;//���ÿ�����
		unsigned char _count;//���ÿ����
		unsigned char _blockSize;//�ڴ�����
	};

	//�ڴ���
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
			if (!chunksSize) goto fill;//chunksΪ�������				
			if (!_allocateState && _allocateIt->is_full())//����������״̬���ϴη���λ���޿��У������²��ҿ��п�
			{
				/*
				��ѯ�����ڴ��
				����һ�η��䴦��ͷβ�ƽ�����
				���þֲ���ԭ������λ�ü����ܾ���ʧ�䴦����
				*/
				auto toBegin{ _allocateIt };
				auto toEnd{ std::next(_allocateIt) };
				bool atBegin{ false };
				while (!atBegin || toEnd != _chunks.end())//û��ͷ��û��β��������
				{
					if (!atBegin)
					{
						if (toBegin != _chunks.begin())//û��ͷ��
						{
							if (!toBegin->is_full())
							{
								_allocateIt = toBegin;
								break;
							}
							--toBegin;
						}
						else//����ͷ��
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
			//���޿����ڴ�������
			if (_allocateIt->is_full())
			{
				_allocateState = true;
			fill:
				size_t blockSize{ 20 + (chunksSize >> 1) };//��̬�趨blockSize
				_chunks.emplace_back(size, blockSize < _Chunk::MAX_BLOCK_SIZE ? blockSize : _Chunk::MAX_BLOCK_SIZE);
				_allocateIt = std::prev(_chunks.end());
			}
			_deallocateIt = _allocateIt;//1.�´��ڴ����λ�ÿ��ܾ��ǵ�ǰ����λ�� 2.����������ʧЧ
			return _allocateIt->allocate(size);
		}

		void deallocate(void* p, size_t size)
		{
			if (!_deallocateIt->is_contain(p, size))//ʧ�������²���
			{
				//���Ҷ�Ӧ��chunk����allocate����ԭ����ͬ��
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
					assert(!atBegin || toEnd != _chunks.end());//�ڴ�鲻���ڴ��ڴ�� �� deleteָ����ָ�����Ͳ�ƥ��
				}
			}
			_allocateIt = _deallocateIt;
			_deallocateIt->deallocate(p, size);
			_allocateState = false;

			if (_deallocateIt->is_empty())//chunkΪ���ƶ���chunks���
			{
				if (_chunks.back().is_empty())//������ȫ��chunk���ͷžɵ�һ��
				{
					_chunks.pop_back();
					if (_chunks.size() < (_chunks.capacity() >> 1))
					{
						ChunksIterator::difference_type allocateItIndex{ _allocateIt - _chunks.begin() };
						ChunksIterator::difference_type deallocateItIndex{ _deallocateIt - _chunks.begin() };
						_chunks.shrink_to_fit();//����_chunks
						//�������ܵ���ԭ�е�����ʧЧ�������¸�ֵ
						_allocateIt = _chunks.begin() + allocateItIndex;
						_deallocateIt = _chunks.begin() + deallocateItIndex;
					}
				}
				if (!_chunks.empty()) _chunks.back().swap(*_deallocateIt);
			}
		}

	private:
		using ChunksIterator = std::vector<_Chunk>::iterator;
		std::vector<_Chunk> _chunks;//�ڴ���
		ChunksIterator _allocateIt, _deallocateIt;//��һ�η��䡢���յ�λ��
		bool _allocateState;//��������״̬
	};

	//����ڴ��
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
		//�ڴ�����ĸ���  //�ڴ������ //���������ڴ� 

	private:
		//���ܣ�allocate����������֧�ֶ�̬��
		void* _allocate(size_t size, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(search_chain(size).allocate(size)) };
			*cookie = static_cast<unsigned char>(size);
			return cookie + 1;
		}

		//���ܣ�allocate������������֧�ֶ�̬��
		void* _allocate(size_t size, std::false_type)
		{
			return search_chain(size).allocate(size);
		}

		//���ܣ�deallocate����������֧�ֶ�̬��
		void _deallocate(void* p, size_t, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(p) - 1 };
			search_chain(*cookie).deallocate(cookie, *cookie);
		}

		//���ܣ�deallocate������������֧�ֶ�̬��
		void _deallocate(void* p, size_t size, std::false_type)
		{
			size_t alSize{ align_size(size) };
			search_chain(alSize).deallocate(p, alSize);
		}

		//���ܣ����Ҷ�Ӧchunks
		//���룺align_size��Ĵ�С
		_ChunkChain& search_chain(size_t size)
		{
			return size < 4 + SupportPolym ? _spool[size - 1 - SupportPolym] : _bpool[(size - SupportPolym >> 2) - 1];
		}

		//���ܣ��ڴ�������PER_CHAIN_SIZE����
		static size_t align_size(size_t size)
		{
			return (size < 4 ? size : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1))) + SupportPolym;
		}

	private:
		std::vector<_ChunkChain> _bpool;//����4-128byte
		std::vector<_ChunkChain> _spool;//����1��2��3byte
	};
}

#endif // !LOKIMEMORYPOOL_H



