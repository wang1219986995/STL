#include "MemoryPool.h"
#include <iostream>
#include <list>

#define TEST_NUM 10
#define ELEM_NUM 10000000

struct A { int i; };
struct B : public A { double d; };
struct C : public B { char c; };

void useMemoryPool()
{
	std::list<A, hzw::Allocator<A>> la;
	std::list<B, hzw::Allocator<B>> lb;
	std::list<C, hzw::Allocator<C>> lc;
	for (int i{ 0 }; i < ELEM_NUM; ++i)
	{
		la.push_back(A{});
		lb.push_back(B{});
		lc.push_back(C{});
	}
}
void useDefultMemoryPool()
{
	std::list<A> la;
	std::list<B> lb;
	std::list<C> lc;
	for (int i{ 0 }; i < ELEM_NUM; ++i)
	{
		la.push_back(A{});
		lb.push_back(B{});
		lc.push_back(C{});
	}
}

int main(int argc, char* argv[])
{
	for (int i{ 0 }; i < TEST_NUM; ++i)
	{
#if(0)
		std::thread t1{ useDefultMemoryPool };
		std::thread t2{ useDefultMemoryPool };
		std::thread t3{ useDefultMemoryPool };
#endif
#if(1)
		std::thread t1{ useMemoryPool };
		std::thread t2{ useMemoryPool };
		std::thread t3{ useMemoryPool };
#endif
		t1.join();
		t2.join();
		t3.join();
	}
	system("pause");
}