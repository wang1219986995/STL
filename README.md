# MemoryPool

#### 介绍
c++线程安全内存池，与c++容器和自定义类轻松搭配使用。适用于有大量细粒度对象、反复释放构建对象的场景

#### 适用场景
细粒度内存请求、各线程请求内存大小各不相同、 反复申请相同大小空间，时空效率越优

#### 效能
开启三个线程，每个线程向各自std::list容器添加三种细粒度对象各一千万个，各线程重复此操作十次

`未使用内存池：用时5m30s、内存峰值4.0G`

![未使用内存池](https://images.gitee.com/uploads/images/2019/0601/175145_50271723_5038916.png "nouse.png")

`使用内存池：用时2m、内存峰值3.2G`

![使用内存池](https://images.gitee.com/uploads/images/2019/0601/175201_d5caf73b_5038916.png "use.png")

#### 使用说明
1. 自定义对象使用内存池
    ```
    class A : public hzw::UseMemoryPool {...};
    A* a{new A};//从内存池分配内存
    delete a;//将内存块归还内存池
    ```
2. 容器使用内存池allocator
    ```
    std::list<int, hzw::Allocator<int>> list;
    ```
3.  **注意事项** 
    使用多态特性时请谨慎
    ```
    class Base{...};
    class Derived : public Base{};
    Base* b{new Base};
    Base* d{new Derived};
    delete b;//正确回收
    delete d;//错误回收。不会直接导致程序崩溃，但将产生内存内碎片
    ```
    总结：请用与数据类型匹配的指针delete

#### 测试代码
```
#include "MemoryPool.h"
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
```
