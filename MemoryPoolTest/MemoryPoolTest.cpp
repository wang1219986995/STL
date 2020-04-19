#include "pch.h"
#include "CppUnitTest.h"
#include "../MemoryPool/MemoryPool.hpp"

#include <random>
#include <thread>
#include <future>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace hzw;
using namespace hzw::detail;

namespace MemoryPoolTest
{
	//线程归属内存池测试
	template<typename Pool>
	class ThreadTest
	{
	public:
		using support_polym = typename Pool::support_polym;
		enum{ TEST_COUNT = 1000, POOL_MAX_SIZE = Pool::MAX_SIZE, TEST_MAX_SIZE = static_cast<size_t>(POOL_MAX_SIZE * 1.2) };

	public:
		//功能：测试内存MEMORYPOOL_ALIGN_SIZE是否生效
		static bool align_size_test()
		{
			std::uniform_int_distribution<size_t> u{ 1, POOL_MAX_SIZE };
			std::default_random_engine r;
			
			if (MEMORYPOOL_ALIGN_SIZE != 0)
			{
				for (size_t c{ 0 }; c < TEST_COUNT; ++c)
				{
					size_t size{ u(r) };
					if ((reinterpret_cast<size_t>(Pool::allocate(size)) % MEMORYPOOL_ALIGN_SIZE) != 0) return false;
				}
			}
			return true;
		}

		//功能：分配任意大小、任意个数，写入数据校验并释放
		static bool random_test()
		{
			std::uniform_int_distribution<size_t> sizeU{ 1,  TEST_MAX_SIZE };
			std::uniform_int_distribution<size_t> allocU{ TEST_COUNT / 4, TEST_COUNT / 2 };
			std::default_random_engine r;

			for (size_t testCount{ 0 }; testCount < TEST_COUNT; ++testCount)
			{
				size_t allocateCount{ allocU(r) };
				std::vector<std::pair<void*, size_t>> bufs;
				for (size_t i{0}; 0 < allocateCount; --allocateCount, ++i)
				{
					size_t size{ sizeU(r) };
					void* p{ Pool::allocate(size) };
					bufs.emplace_back(p, size);
					//写入数据 奇数置1偶数置0
					i % 2 ? memset(p, UINT8_MAX, size) : memset(p, 0, size);
				}
				for (size_t i{ 0 }; i < bufs.size(); ++i)
				{
					void* p{ bufs[i].first };
					size_t size{ bufs[i].second };
					void* standard{ ::operator new(size) };

					//校验数据
					i % 2 ? memset(standard, UINT8_MAX, size) : memset(standard, 0, size);
					if (memcmp(p, standard, size) != 0) return false;

					::operator delete(standard);

					_random_test(p, size, support_polym{});
				}
			}
			return true;
		}

		//功能：测试malloc_size返回值
		static bool malloc_size_test()
		{
			std::uniform_int_distribution<size_t> u{ 1,  TEST_MAX_SIZE };
			std::default_random_engine r;

			for (size_t testCount{ 0 }; testCount < TEST_COUNT; ++testCount)
			{
				size_t size{ u(r) };
				void* p{ Pool::allocate(size) };
				size_t resSize{ Pool::malloc_size(p) };

				if (resSize < size) return false;
			}
			return true;
		}

	private:
		static void _random_test(void* p, size_t, std::true_type)
		{
			Pool::deallocate(p);
		}

		static void _random_test(void* p, size_t size, std::false_type)
		{
			Pool::deallocate(p, size);
		}
	};

	//全局归属内存池测试
	template<typename Pool>
	class GlobalTest
	{
		using Test = ThreadTest<Pool>;

	public:
		static bool align_size_test()
		{
			auto res1{ std::async(std::launch::async, Test::align_size_test) };
			auto res2{ std::async(std::launch::async, Test::align_size_test) };
			return res1.get() && res2.get();
		}

		static bool random_test()
		{
			auto res1{ std::async(std::launch::async,Test::random_test) };
			auto res2{ std::async(std::launch::async,Test::random_test) };
			return res1.get() && res2.get();
		}

		static bool malloc_size_test()
		{
			auto res1{ std::async(std::launch::async,Test::malloc_size_test) };
			auto res2{ std::async(std::launch::async,Test::malloc_size_test) };
			return res1.get() && res2.get();
		}
	};

	
#define TEST_T(Pool) TEST_CLASS(Pool){public:\
		TEST_METHOD(align_size_test){Assert::IsTrue(test.align_size_test());}\
		TEST_METHOD(random_test){Assert::IsTrue(test.random_test());}\
	private:ThreadTest<hzw::Pool> test;};\

#define TEST_TP(Pool) TEST_CLASS(Pool){public:\
		TEST_METHOD(align_size_test){Assert::IsTrue(test.align_size_test());}\
		TEST_METHOD(random_test){Assert::IsTrue(test.random_test());}\
		TEST_METHOD(malloc_size_test){Assert::IsTrue(test.malloc_size_test());}\
	private:ThreadTest<hzw::Pool> test;};\

#define TEST_G(Pool) TEST_CLASS(Pool){public:\
		TEST_METHOD(align_size_test){Assert::IsTrue(test.align_size_test());}\
		TEST_METHOD(random_test) { Assert::IsTrue(test.random_test()); }\
	private:GlobalTest<hzw::Pool> test;};\

#define TEST_GP(Pool) TEST_CLASS(Pool){public:\
		TEST_METHOD(align_size_test){Assert::IsTrue(test.align_size_test());}\
		TEST_METHOD(random_test) { Assert::IsTrue(test.random_test()); }\
		TEST_METHOD(malloc_size_test){Assert::IsTrue(test.malloc_size_test());}\
	private:GlobalTest<hzw::Pool> test;};\

	TEST_T(WkT);
	TEST_TP(WkTP);
	TEST_G(WkG);
	TEST_GP(WkGP);

	TEST_T(PgT);
	TEST_TP(PgTP);
	TEST_G(PgG);
	TEST_GP(PgGP);

	TEST_T(LkT);
	TEST_TP(LkTP);
	TEST_G(LkG);
	TEST_GP(LkGP);

	TEST_T(NwT);
	TEST_TP(NwTP);
	TEST_G(NwG);
	TEST_GP(NwGP);
}
