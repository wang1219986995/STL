# MemoryPool

#### 介绍
c++线程安全内存池，与c++容器和自定义类轻松搭配使用。适用于有大量对象、反复释放构建对象的场景

#### 性能

开启四个线程，每个线程向各自std::list容器添加四种对象各五百万个并释放，各线程重复此操作十次（测试代码在main.cpp）
![性能](https://images.gitee.com/uploads/images/2020/0419/222120_994bc4c4_5038916.png "性能测试结果.png")

#### 结构
1. 模块划分
![模块划分](https://images.gitee.com/uploads/images/2020/0419/222010_9c5f1e01_5038916.png "总体功能模块图.png")
2. 模块在头文件的分布
![模块分布](https://images.gitee.com/uploads/images/2020/0419/222354_e7a99b9a_5038916.png "模块分布.png")

#### 特性说明
![特性1](https://images.gitee.com/uploads/images/2019/0901/141020_8ae59f7f_5038916.png "特性1.png")

![特性2](https://images.gitee.com/uploads/images/2019/0901/141036_daffcec7_5038916.png "特性2.png")

![特性3](https://images.gitee.com/uploads/images/2020/0419/222835_b4035c84_5038916.png "图片1.png")

1. 归属说明：
    （1）线程归属
    ```
    int main(int argc, char* argv[])
    {
        //data只在主线程使用，所以使用AllocWkT(线程归属的悟空内存池)将有更高的效率
	    std::list<int, AllocWkT<int>> data{ 1, 2, 3, 4 };
	    for (auto& elem : data) ++elem;
	    return 0;
    }
    ```
    （2）全局归属
    ```
    std::list<int, AllocWkG<int>> producer()
    {
	    return std::list<int, AllocWkG<int>>{1, 2, 3, 4};
    }

    void consumer(std::list<int, AllocWkG<int>>& data)
    {
	    for (auto& elem : data) ++elem;
    }

    int main(int argc, char* argv[])
    {
        //在生产者线程获得数据
	    auto future{std::async(std::launch::async, producer)};
	    auto data{ future.get() };
        //在主线消费
	    consumer(data);
        //data跨越主线程和生产者线程，属于全局归属，故使用AllocWkG(全局归属悟空内存池)保证内存回收正确
	    return 0;
    }
    ```
    总结：数据只在一个线程使用则使用线程归属内存池，数据作为多个线程的共享数据则使用全局归属内存池

2.多态说明

```
    class Human : public UseWkTP
    {
        std::string _name;
    public:
	virtual void work()
	{
            std::cout << "human work\n";
	}
        virtual ~Human(){}
    };
    class Teacher : public Human
    {
	int _id;
    public:
	void work() override
	{
            std::cout << "teacher work\n";
	}
    };

    int main(int argc, char* argv[])
    {
        //Teacher对象使用Human指针delete,使用多态特性，故使用UseWkTP(线程归属、多态特性的悟空内存池)，
        //使用线程还是全局归属上方已说明，使用多态特性保证正确回收空间
        //如无多态或未使用基类指针指向派生类，则无需使用多态特性内存池
	Human* teacher{ new Teacher };
	delete teacher;
	return 0;
    }
```
    总结：如出现多态类或使用基类指针指向派生类，请使用多态特性内存池。其余情况无需使用，将有更小的开销

#### 使用说明
    编译器支持c++11及以上

    详细使用说明见main.cpp

    以下均已悟空内存池为例，其他内存池替换对应名称缩写即可

    下方__代表内存池归属和多态特性，根据实际情况选择

1. 引入同文件即可
    （1）要使用某一种内存池直接include对应内存池头文件，如使用悟空内存池，#include "WukongMemoryPool.hpp"

    （2）如果使用全部内存池也可直接#include "MemoryPool.hpp"

2. 自定义对象使用内存池 （公有继承UseWk__）   
    ![自定义相关类名](https://images.gitee.com/uploads/images/2020/0419/224055_94463cdb_5038916.png "图片2.png")
    ```
    #include "WukongMemoryPool.hpp"
    using namespace hzw;
    class A : public UseWk__ {...};//使用悟空内存池
    A* a{new A};//从悟空内存池分配内存
    delete a;//将内存块归还悟空内存池
    ```

3. 容器使用内存池
    ![分配器相关类名](https://images.gitee.com/uploads/images/2020/0419/224719_d8f0298c_5038916.png "图片3.png")
    ```
    #include "WukongMemoryPool.hpp"
    using namespace hzw;
    std::list<int, AllocWk_<int>> list;//使用悟空内存池
    ```

4. 直接使用内存池
    ![相关类名](https://images.gitee.com/uploads/images/2020/0419/224751_6f628154_5038916.png "图片4.png")
    ```
    #include "WukongMemoryPool.hpp"
    using namespace hzw;
    int main(int argc, char* argv[])
    {
	    size_t bufSize{ 100 };
	    char* buf{ static_cast<char*>(WkG::allocate(bufSize)) };//从悟空内存池获取内存
	    WkG::deallocate(buf, bufSize);//归还内存块
	    return 0;
    }
    ```
