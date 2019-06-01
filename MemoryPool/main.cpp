#include "MemoryPool.h"
#include <string>
#include <list>
#include <iostream>

class A : public hzw::UseMemoryPool {};//自定义对象使用内存池

int main(int argc, char* argv[])
{
	std::list<int, hzw::Allocator<int>> list;//容器使用内存池allocator
}