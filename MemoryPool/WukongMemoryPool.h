#ifndef  WUKONGMEMORYPOOL_H
#define WUKONGMEMORYPOOL_H

#include <vector>
#include <type_traits>

/*
	����ڴ��
	�򵥴ֱ����ṩ���ٷ������
	������ʱ���ڴ�س��е��ڴ治�����黹��ϵͳ��ֱ�����̽�����ϵͳ����
	�����֮ �����ж�̬����������������������ٶ�
*/

namespace hzw
{
	//����ڴ��
	template<bool SupportPolym>
	class WukongMemoryPool
	{
		struct Node;

	public:
		WukongMemoryPool() : _pool(CHAIN_LENTH, { 0, nullptr }) {}

		WukongMemoryPool(const WukongMemoryPool&) = delete;

		WukongMemoryPool(WukongMemoryPool&&) = default;

		void* allocate(size_t size)
		{
			size_t alSize{ align_size(size) };
			size_t index{ search_index(alSize) };
			//��Ӧ�ڴ���Ϊ�������
			if (!_pool[index].second) fill_chain(_pool[index], alSize);
			return _allocate(_pool[index], alSize, std::bool_constant<SupportPolym>{});
		}

		void deallocate(void* oldP, size_t size)
		{
			_deallocate(oldP, size, std::bool_constant<SupportPolym>{});
		}

	public:
		enum
		{
			CHAIN_LENTH = 32, PER_CHAIN_SIZE = 4, MAX_SPLIT_SIZE = 128,
			MAX_SIZE = CHAIN_LENTH * PER_CHAIN_SIZE
		};
		//�ڴ�����ĸ���  //�ڴ������ //����и���� //���������ڴ� 

	private:
		//���ܣ�allocate����������֧�ֶ�̬��
		void* _allocate(std::pair<size_t, Node*>& chainHead, size_t size, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(remove_from_chain(chainHead)) };
			*cookie = static_cast<unsigned char>(size);
			return cookie + 1;
		}

		//���ܣ�allocate������������֧�ֶ�̬��
		void* _allocate(std::pair<size_t, Node*>& chainHead, size_t, std::false_type)
		{
			return remove_from_chain(chainHead);
		}

		//���ܣ�_deallocate����������֧�ֶ�̬��
		void _deallocate(void* oldp, size_t, std::true_type)
		{
			unsigned char* cookie{ static_cast<unsigned char*>(oldp) - 1 };
			add_to_chain(reinterpret_cast<Node*>(cookie), _pool[search_index(*cookie)]);
		}

		//���ܣ�_deallocate������������֧�ֶ�̬��
		void _deallocate(void* oldp, size_t size, std::false_type)
		{
			size_t index{ search_index(align_size(size)) };
			add_to_chain(static_cast<Node*>(oldp), _pool[index]);
		}

		//���ܣ�����ڴ���
		//���룺�ڴ���ͷ��������ڴ������С
		void fill_chain(std::pair<size_t, Node*>& chainHead, size_t size)
		{
			//��̬�����и����
			size_t splitSize{ 20 + (chainHead.first >> 5) };
			splitSize = splitSize < MAX_SPLIT_SIZE ? splitSize : MAX_SPLIT_SIZE;
			//�����и���ڴ�
			char* chunk{ static_cast<char*>(::operator new(splitSize * size)) };
			//�и�chunk
			char* p{ chunk };
			for (size_t i{ 1 }; i < splitSize; ++i, p += size)
				reinterpret_cast<Node*>(p)->next = reinterpret_cast<Node*>(p + size);
			reinterpret_cast<Node*>(p)->next = nullptr;
			//�ҵ���Ӧ�ڴ���
			chainHead.second = reinterpret_cast<Node*>(chunk);
		}

		//���ܣ������ڴ������Ӧ���ڴ�������
		//���룺������ڴ������С
		//������ڴ�������
		static size_t search_index(size_t size)
		{
			return (size - SupportPolym >> 2) - 1;
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
			return (size <= 8 ? 8 : (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1))) + SupportPolym;
		}

		//���ܣ�align_size����������32λ��
		static size_t _align_size(std::false_type, size_t size)
		{
			return (size + PER_CHAIN_SIZE - 1 & ~(PER_CHAIN_SIZE - 1)) + SupportPolym;
		}

		//���ܣ�����ڴ�鵽�ڴ���
		//���룺��ӵ��ڴ�顢�ڴ���ͷ
		void add_to_chain(Node* p, std::pair<size_t, Node*>& chainHead)
		{
			--chainHead.first;
			p->next = chainHead.second;
			chainHead.second = p;
		}

		//���ܣ����ڴ����Ƴ��ڴ��
		//���룺�ڴ���ͷ
		//������ڴ��
		void* remove_from_chain(std::pair<size_t, Node*>& chainHead)
		{
			++chainHead.first;
			void* result{ chainHead.second };
			chainHead.second = chainHead.second->next;
			return result;
		}

	private:
		struct Node
		{
			Node* next;
		};
		std::vector<std::pair<size_t, Node*>> _pool;//�ڴ��
	};
}

#endif // ! WUKONGMEMORYPOOL_H


