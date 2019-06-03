#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <mutex>

namespace hzw
{
	/*
	�ڴ�أ� ϸ�����ڴ����󡢸��߳������ڴ��С������ͬ�� ����������ͬ��С��ʱ��Ч��Խ��
	*/
	class MemoryPool
	{
	public:
		MemoryPool() = delete;

		//���ܣ������ڴ���Դ
		//���룺�ڴ�����Ĵ�С
		//������������ڴ�ָ��
		static void* allocate(size_t size);

		//���ܣ��ͷ��ڴ���Դ
		//���룺�ͷ��ڴ��ָ�룬�ͷ��ڴ�Ĵ�С
		static void deallocate(void* oldP, size_t size);

	private:
		enum 
		{
			ChainLength = 16, ChainPerSize = 8, MaxSplitSize = 20,
			MaxSize = ChainLength * ChainPerSize
		};
		//�ڴ�����ĸ���  //�ڴ������ //����и���� //���������ڴ� 

		struct Node
		{
			Node *next;
		};
		static std::mutex _mutexs[ChainLength];//�ڴ�ػ�����
		static Node* _pool[ChainLength];//�ڴ��
		static std::mutex _freeMutex;//ս���ػ�����
		static char* _freeBegin, * _freeEnd;//ս����ָ��
		static size_t _count;//�ڴ�ع����ڴ�����

	private:
		//���ܣ����ڴ�ط����ڴ�
		//���룺������ڴ������С
		//����������ڴ���ָ��
		static void *_allocate(size_t size);

		//���ܣ��и�ս���غ����ս���ش�С
		//���룺������ڴ������С
		//������и���ɺ���ڴ���
		static Node *splitFreePool(size_t size);

		//���ܣ�����ڴ���������ڴ���Դ��ս���أ�malloc��������ڴ�����
		//���룺������ڴ������С
		static void fillChain(size_t size);

		//���ܣ������ڴ������Ӧ���ڴ�������
		//���룺�ڴ������С
		//������ڴ�������
		static size_t searchIndex(size_t size);

		//���ܣ��ڴ�������ChainPerSize����
		//���룺�ڴ������С
		//�����������ڴ������С
		static size_t alignSize(size_t size);

		//���ܣ����Ӧ�ڴ�������¿�
		//���룺�¿�ָ�룬�ڴ������С
		static void addToChain(Node* p, size_t size);

		//���ܣ��Ƴ���Ӧ�ڴ���ͷ���Ŀ�
		//���룺��Ӧ�ڴ���ָ������
		//������Ƴ��Ŀ�ָ��
		static void* removeFromChain(Node*& chainHead);

		//���ܣ��黹�ڴ���ڴ�
		//���룺�黹�ڴ��ָ�룬������ڴ������С
		static void _deallocate(Node* oldP, size_t size);
	};

	/*
	�Զ�����ʹ��MemoryPoolֱ�Ӽ̳д���
	�û���ָ��delete�����࣬���ܵ����ڴ���޷���ȷ���յ���Ӧ�ڴ���������ڴ���Ƭ
	*/
	class UseMemoryPool
	{
	public:
		void *operator new(size_t size)
		{
			return MemoryPool::allocate(size);
		}

		void operator delete(void *oldP, size_t size)
		{
			MemoryPool::deallocate(oldP, size);
		}
	};

	/*
	�����������ڲ�ΪMemoryPool
	*/
	template<typename T>
	class Allocator
	{
	public:
		using value_type = T;
		using pointer = T * ;
		using const_pointer = const T *;
		using reference = T & ;
		using const_reference = const T&;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		value_type *allocate(std::size_t num)
		{
			return reinterpret_cast<value_type *>(MemoryPool::allocate(num * sizeof(value_type)));
		}

		void deallocate(value_type *p, std::size_t num)
		{
			MemoryPool::deallocate(p, num * sizeof(value_type));
		}

		template<typename U>
		bool operator ==(const Allocator<U>& lh)
		{
			return true;
		}

		template<typename U>
		bool operator !=(const Allocator<U>& lh)
		{
			return false;
		}

		template<typename U>
		operator Allocator<U>()const
		{
			return Allocator<U>{};
		}
	};
}
#endif //MEMORYPOOL_H
