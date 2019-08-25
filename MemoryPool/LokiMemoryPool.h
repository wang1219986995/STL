#ifndef LOKIMEMORYPOOL_H
#define LOKIMEMORYPOOL_H

#include "MemoryPoolManage.h"

/*
	����ڴ��
	���ڴ�����������Loki�Ⲣ���ԸĽ���ʹ��һЩС���ʵ�ֲ��ṩ��̬���������������������
	����ṩ��̬�������Ź��ܸ��ʺ�ͻ�����ڴ�����
*/

namespace hzw
{
	//�ڴ�飨Loki��Nvwaʹ�ã�
	template<bool SupportCookie>
	class _Chunk
	{
	public:
		enum { MAX_BLOCK_SIZE = 128 };

	public:
		//���룺�ڴ���С���ڴ�����
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
				using std::swap;
				swap(_buf, rh._buf);
				swap(_index, rh._index);
				swap(_count, rh._count);
				swap(_blockSize, rh._blockSize);
			}
		}

		//���ܣ����ؿ��ÿ����
		unsigned char unuse_count()
		{
			return _count;
		}

		//���ܣ������ڴ���ַ
		unsigned char* buf_point()
		{
			return _buf;
		}

		//���ܣ���ȡcookie
		static unsigned char* cookie(void* p, size_t size)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(p) - 1 };
			return cookie - (size + 1) * (*cookie);
		}

	private:
		//���ܣ�allocate����������֧��cookie��
		void _allocate(unsigned char* result, true_type)
		{
			::std::swap(_index, *result);
		}

		//���ܣ�allocate������������֧��cookie��
		void _allocate(unsigned char* result, false_type)
		{
			_index = *result;
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
			if (!chunksSize) goto fill;//chunksΪ�������				
			if (!_allocateState && _allocateIt->is_full())//����������״̬���ϴη���λ���޿��У������²��ҿ��п�
			{
				/*
				��ѯ�����ڴ��
				����һ�η��䴦��ͷβ�ƽ�����
				���þֲ���ԭ������λ�ü����ܾ���ʧ�䴦����
				*/
				auto toBegin{ _allocateIt };
				auto toEnd{ next(_allocateIt) };
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
				_chunks.emplace_back(size, blockSize < Chunk::MAX_BLOCK_SIZE ? blockSize : Chunk::MAX_BLOCK_SIZE);
				_allocateIt = prev(_chunks.end());
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
					if (_deallocateIt != prev(_chunks.end(), 1)) _chunks.pop_back();
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
				_chunks.back().swap(*_deallocateIt);
			}
		}

	private:
		using ChunksIterator = vector<Chunk>::iterator;
		vector<Chunk> _chunks;//�ڴ���
		ChunksIterator _allocateIt, _deallocateIt;//��һ�η��䡢���յ�λ��
		bool _allocateState;//��������״̬
	};

	//����ڴ��
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
		//�ڴ�����ĸ���  //�ڴ������ //���������ڴ� 

		//���ܣ�allocate����������֧�ֶ�̬��
		void* _allocate(size_t size, true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(size > MAX_SIZE ? 
				::operator new(size) : search_chain(size).allocate(size)) };
			*cookie = static_cast<unsigned char>(size);
			return cookie + 1;
		}

		//���ܣ�allocate������������֧�ֶ�̬��
		void* _allocate(size_t size, false_type)
		{
			return size > MAX_SIZE ? ::operator new(size) : search_chain(size).allocate(size);
		}

		//���ܣ�deallocate����������֧�ֶ�̬��
		void _deallocate(void* p, size_t, true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(p) - 1 };
			unsigned char cookieValue{ *cookie };
			cookieValue > MAX_SIZE ? ::operator delete(cookie) :
				search_chain(cookieValue).deallocate(cookie, cookieValue);
		}

		//���ܣ�deallocate������������֧�ֶ�̬��
		void _deallocate(void* p, size_t size, false_type)
		{
			size = align_size(size);
			size > MAX_SIZE ? ::operator delete(p) :
				search_chain(size).deallocate(p, size);
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
		vector<_ChunkChain> _bpool;//����4-128byte
		vector<_ChunkChain> _spool;//����1��2��3byte
	};

	template<bool SupportPolym>
	using Lk = LokiMemoryPool<SupportPolym>;//����ڴ�ر���

	using LkG = GMM<Lk<false>>;//�������֧�ֶ�̬��ȫ�ֹ�����
	using LkT = TMM<Lk<false>>;//�������֧�ֶ�̬���̹߳�����
	using LkGP = GMM<Lk<true>>;//�����֧�ֶ�̬��ȫ�ֹ�����
	using LkTP = TMM<Lk<true>>;//�����֧�ֶ�̬���̹߳�����

	//�������������
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



