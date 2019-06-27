#ifndef  WUKONGMEMORYPOOL_H
#define WUKONGMEMORYPOOL_H

#include <exception>
#include <vector>

/*
	����ڴ��
	�򵥴ֱ����ṩ���ٷ������
	������ʱ���ڴ�س��е��ڴ治�����黹��ϵͳ��ֱ�����̽�����ϵͳ����
	�����֮ �����ж�̬����������������������ٶ�
*/

namespace hzw
{
	//����ڴ��
	class WukongMemoryPool
	{
	public:
		struct Node;
		enum
		{
			CHAIN_LENTH = 32, PER_CHAIN_SIZE = 4, MAX_SPLIT_SIZE = 20,
			MAX_SIZE = CHAIN_LENTH * PER_CHAIN_SIZE
		};
		//�ڴ�����ĸ���  //�ڴ������ //����и���� //���������ڴ� 

		WukongMemoryPool() :
			_pool(CHAIN_LENTH, nullptr), _freeBegin{ nullptr }, _freeEnd{ nullptr }, _count{ 0 }
		{
		}

		WukongMemoryPool(const WukongMemoryPool&) = delete;

		WukongMemoryPool(WukongMemoryPool&&) = default;

		void* allocate(size_t size)
		{
			size_t alSize{ align_size(size) };
			size_t index{ search_index(alSize) };
			//��Ӧ�ڴ���Ϊ�������
			if (!_pool[index]) fill_chain(alSize);
			return remove_from_chain(_pool[index]);
		}

		void deallocate(void* oldP, size_t size)
		{
			add_to_chain(static_cast<Node*>(oldP), align_size(size));
		}

	private:
		//���ܣ��и�ս���غ����ս���ش�С
		//���룺������ڴ������С
		//������и���ɺ���ڴ���
		Node* split_free_pool(size_t size)
		{
			Node* result{ reinterpret_cast<Node*>(_freeBegin) };
			size_t splitSize{ (_freeEnd - _freeBegin) / size };
			splitSize = splitSize < MAX_SPLIT_SIZE ? splitSize : MAX_SPLIT_SIZE;
			//�и�
			char* p{ _freeBegin };
			for (size_t i{ 1 }; i < splitSize; ++i, p += size)
				reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
			reinterpret_cast<Node*>(p)->next = nullptr;
			//����ս����
			_freeBegin = p + size;
			return result;
		}

		//���ܣ�����ڴ���������ڴ���Դ��ս���أ�malloc��������ڴ�����
		//���룺������ڴ������С
		void fill_chain(size_t size)
		{
			size_t lastSize{ static_cast<size_t>(_freeEnd - _freeBegin) };
			//��������
			if (lastSize < size)//ս�����޷�����һ������
			{
				if (lastSize) add_to_chain(reinterpret_cast<Node*>(_freeBegin), lastSize);//����ս����ʣ��
				//���ս����
				size_t askSize{ size * MAX_SPLIT_SIZE * 2 + align_size(_count >> 2) };//������		
				//������Լ���askSizeֱ����ϵͳ����ɹ������Ǻ������������ͨ����������ǣ������·�û��ô��
				if (_freeBegin = reinterpret_cast<char*>(std::malloc(askSize)))//��ϵͳ�����ڴ�ɹ�
				{
					_freeEnd = _freeBegin + askSize;
					_count += askSize;
				}
				else//��ϵͳʧ�ܣ��ָ������ڴ���
				{
					size_t targetIndex{ search_index(size) + 1 };
					for (; targetIndex < CHAIN_LENTH; ++targetIndex)
						if (_pool[targetIndex])	break;
					if (targetIndex >= CHAIN_LENTH)  throw std::bad_alloc{};//�Ҳ����ɷָ�Ĵ��ڴ���
					else
					{
						_freeBegin = reinterpret_cast<char*>(remove_from_chain(_pool[targetIndex]));
						_freeEnd = _freeBegin + (targetIndex + 1) * PER_CHAIN_SIZE;
					}
				}
			}
			_pool[search_index(size)] = split_free_pool(size);//�и�ս���أ��ҵ���Ӧ�ڴ���
		}

		//���ܣ������ڴ������Ӧ���ڴ�������
		//���룺�ڴ������С��ִ��align_size��
		//������ڴ�������
		static size_t search_index(size_t size)
		{
			return (size >> 2) - 1;
		}

		//���ܣ��ڴ�������PER_CHAIN_SIZE����
		//���룺�ڴ������С
		//�����������ڴ������С
		static size_t align_size(size_t size)
		{
			//����64λ��32λ��64λʱָ���СΪ8byte��_pool[0]Ϊ4byte�޷�ʵ��Ƕ��ʽָ�룬��64λ����_pool[0]
			return _align_size(std::bool_constant <sizeof(void*) == 8>{}, size);
		}

		//���ܣ�align_size����������64λ��
		static size_t _align_size(std::true_type, size_t size)
		{
			return size <= 8 ? 8 : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1));
		}

		//���ܣ�align_size����������32λ��
		static size_t _align_size(std::false_type, size_t size)
		{
			return size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1);
		}

		//���ܣ����Ӧ�ڴ�������¿�
		//���룺�¿�ָ�룬�ڴ������С
		void add_to_chain(Node* p, size_t size)
		{
			Node*& chainHead{ _pool[search_index(size)] };
			p->next = chainHead;
			chainHead = p;
		}

		//���ܣ��Ƴ���Ӧ�ڴ���ͷ���Ŀ�
		//���룺��Ӧ�ڴ���ָ������
		//������Ƴ��Ŀ�ָ��
		void* remove_from_chain(Node*& chainHead)
		{
			void* result{ chainHead };
			chainHead = chainHead->next;
			return result;
		}

	private:
		struct Node
		{
			Node* next;
		};
		std::vector<Node*> _pool;//�ڴ��
		char* _freeBegin, * _freeEnd;//ս����ָ��
		size_t _count;//�ڴ�ع����ڴ�����
	};
}

#endif // ! WUKONGMEMORYPOOL_H


