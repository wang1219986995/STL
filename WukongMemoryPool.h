#ifndef  WUKONGMEMORYPOOL_H
#define WUKONGMEMORYPOOL_H

#include <type_traits>
#include <utility>

namespace hzw
{
	class WukongMemoryPool
	{
	public:
		WukongMemoryPool();

		WukongMemoryPool(const WukongMemoryPool&) = delete;

		//���ܣ������ڴ���Դ
		//���룺�ڴ�����Ĵ�С
		//������������ڴ�ָ��
		void* allocate(size_t size);

		//���ܣ��ͷ��ڴ���Դ
		//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ�Ĵ�С
		void deallocate(void* oldP, size_t size);

	public:
		enum
		{
			CHAIN_LENTH = 32, PER_CHAIN_SIZE = 4, MAX_SPLIT_SIZE = 20,
			MAX_SIZE = CHAIN_LENTH * PER_CHAIN_SIZE
		};
		//�ڴ�����ĸ���  //�ڴ������ //����и���� //���������ڴ� 

		struct Node
		{
			Node* next;
		};
		Node* _pool[CHAIN_LENTH], *_poolEnd[CHAIN_LENTH];//�ڴ�أ� �ڴ�����β
		char* _freeBegin, * _freeEnd;//ս����ָ��
		size_t _count;//�ڴ�ع����ڴ�����

	private:
		//���ܣ��и�ս���غ����ս���ش�С
		//���룺������ڴ������С
		//������и���ɺ���ڴ���
		std::pair<Node*, Node*> split_free_pool(size_t size);

		//���ܣ�����ڴ���������ڴ���Դ��ս���أ�malloc��������ڴ�����
		//���룺������ڴ������С
		void fill_chain(size_t size);

		//���ܣ������ڴ������Ӧ���ڴ�������
		//���룺�ڴ������С��ִ��align_size��
		//������ڴ�������
		static size_t search_index(size_t size);

		//���ܣ��ڴ�������ChainPerSize����
		//���룺�ڴ������С
		//�����������ڴ������С
		static size_t align_size(size_t size);

		//���ܣ�align_size��������
		static size_t _align_size(std::true_type, size_t size);

		//���ܣ�align_size��������
		static size_t _align_size(std::false_type, size_t size);

		//���ܣ����Ӧ�ڴ�������¿�
		//���룺�¿�ָ�룬�ڴ������С
		void add_to_chain(Node* p, size_t size);

		//���ܣ��Ƴ���Ӧ�ڴ���ͷ���Ŀ�
		//���룺��Ӧ�ڴ���ָ������
		//������Ƴ��Ŀ�ָ��
		void* remove_from_chain(size_t size);
	};
}

#endif // ! WUKONGMEMORYPOOL_H


