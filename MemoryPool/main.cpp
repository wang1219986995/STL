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

template<template<typename T> typename Alloc>
void allocate_test(int testCount, int containSize)
{	
	for (int i{ 0 }; i < testCount; ++i)
	{		
		std::list<A, Alloc<A>> la;
		std::list<B, Alloc<B>> lb;
		std::list<C, Alloc<C>> lc;
		std::list<D, Alloc<D>> ld;
		for (int j = 0; j < containSize; ++j)
		{
			la.push_back(A{});
			lb.push_back(B{});
			lc.push_back(C{});
			ld.push_back(D{});
		}
	}
}

#define CONTAIN_SIZE 5'000'000
#define TEST_COUNT 10

void test()
{
#if(1)//Îò¿Õ²âÊÔ
	std::thread t1{ allocate_test<AllocWk>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t2{ allocate_test<AllocWk>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t3{ allocate_test<AllocWk>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t4{ allocate_test<AllocWk>, TEST_COUNT, CONTAIN_SIZE };
#endif
#if(0)//Âå»ù²âÊÔ                                                                                          
	std::thread t1{ allocate_test<AllocLk>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t2{ allocate_test<AllocLk>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t3{ allocate_test<AllocLk>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t4{ allocate_test<AllocLk>, TEST_COUNT, CONTAIN_SIZE };
#endif
#if(0)//Ô­Éú¶Ô±È²âÊÔ
	std::thread t1{ allocate_test<std::allocator>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t2{ allocate_test<std::allocator>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t3{ allocate_test<std::allocator>, TEST_COUNT, CONTAIN_SIZE };
	std::thread t4{ allocate_test<std::allocator>, TEST_COUNT, CONTAIN_SIZE };
#endif
	t1.join();
	t2.join();
	t3.join();
	t4.join();
}

int main(int argc, char* argv[])
{
	steady_clock::time_point t{ steady_clock::now() };

	test();

	std::cout << duration_cast<duration<double>>(steady_clock::now() - t).count();
	return 0;
}