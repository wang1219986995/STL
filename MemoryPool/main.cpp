#include "MemoryPool.h"
#include <string>
#include <list>
#include <iostream>

class A : public hzw::UseMemoryPool {};//�Զ������ʹ���ڴ��

int main(int argc, char* argv[])
{
	std::list<int, hzw::Allocator<int>> list;//����ʹ���ڴ��allocator
}