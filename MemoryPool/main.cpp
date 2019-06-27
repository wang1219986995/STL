#include "MemoryPool.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <list>

using namespace std::chrono;
using namespace hzw;

struct A
{
	char buf[1];
};
struct B
{
	char buf[32];
};
struct C
{
	char buf[64];
};
struct D
{
	char buf[96];
};

template<typename MemoryPool>
void allocate_test(int testCount, int dataSize)
{	
	for (int i{ 0 }; i < testCount; ++i)
	{		
		std::list<A, Allocator<A, MemoryPool>> la;
		std::list<B, Allocator<B, MemoryPool>> lb;
		std::list<C, Allocator<C, MemoryPool>> lc;
		std::list<D, Allocator<D, MemoryPool>> ld;
		for (int j = 0; j < dataSize; ++j)
		{
			la.push_back(A{});
			lb.push_back(B{});
			lc.push_back(C{});
			ld.push_back(D{});
		}
	}
}

void allocate_test(int testCount, int dataSize)
{
	for (int i{ 0 }; i < testCount; ++i)
	{
		std::list<A> la;
		std::list<B> lb;
		std::list<C> lc;
		std::list<D> ld;
		for (int j = 0; j < dataSize; ++j)
		{
			la.push_back(A{});
			lb.push_back(B{});
			lc.push_back(C{});
			ld.push_back(D{});
		}
	}
}

#define TEST_SIZE 5000000
#define TEST_NUM 10

int main(int argc, char* argv[])
{
	steady_clock::time_point t{ steady_clock::now() };
#if(1)//Îò¿Õ²âÊÔ
	std::thread t1{ allocate_test<WukongMemoryPool>, TEST_NUM, TEST_SIZE };
	std::thread t2{ allocate_test<WukongMemoryPool>, TEST_NUM, TEST_SIZE };
	std::thread t3{ allocate_test<WukongMemoryPool>, TEST_NUM, TEST_SIZE };
	std::thread t4{ allocate_test<WukongMemoryPool>, TEST_NUM, TEST_SIZE };
#endif
#if(0)//Âå»ù²âÊÔ                                                                                          
	std::thread t1{ allocate_test<LokiMemoryPool>, TEST_NUM, TEST_SIZE };
	std::thread t2{ allocate_test<LokiMemoryPool>, TEST_NUM, TEST_SIZE };
	std::thread t3{ allocate_test<LokiMemoryPool>, TEST_NUM, TEST_SIZE };
	std::thread t4{ allocate_test<LokiMemoryPool>, TEST_NUM, TEST_SIZE };
#endif
#if(0)//Ô­Éú¶Ô±È²âÊÔ
	std::thread t1{ allocate_test, TEST_NUM, TEST_SIZE };
	std::thread t2{ allocate_test, TEST_NUM, TEST_SIZE };
	std::thread t3{ allocate_test, TEST_NUM, TEST_SIZE };
	std::thread t4{ allocate_test, TEST_NUM, TEST_SIZE };
#endif
	t1.join();
	t2.join();
	t3.join();
	t4.join();

	std::cout << duration_cast<duration<double>>(steady_clock::now() - t).count();
	return 0;
}