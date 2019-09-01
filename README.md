# MemoryPool

#### 介绍
c++线程安全内存池，与c++容器和自定义类轻松搭配使用。适用于有大量对象、反复释放构建对象的场景
![介绍](https://images.gitee.com/uploads/images/2019/0901/131457_50fb3fb4_5038916.png "介绍.png")

#### 效能

![效能](https://images.gitee.com/uploads/images/2019/0901/140207_377341aa_5038916.png "效能.png")

开启四个线程，每个线程向各自std::list容器添加四种对象各五百万个并释放，各线程重复此操作十次（测试代码在main.cpp）

`未使用内存池：用时255s、内存峰值6.1G`

![原生](https://images.gitee.com/uploads/images/2019/0901/135813_476a09b7_5038916.png "原生-255-6.1.png")

`悟空内存池：用时11s、内存峰值5.2G`

![悟空](https://images.gitee.com/uploads/images/2019/0901/135843_d4d54115_5038916.png "悟空-11-5.2.png")

`洛基内存池：用时56s、内存峰值5.2G`

![洛基](https://images.gitee.com/uploads/images/2019/0901/135949_bcb14af6_5038916.png "洛基-56-5.2.png")

`女娲内存池：用时158s、内存峰值5.5G`

![女娲](https://images.gitee.com/uploads/images/2019/0901/140033_dfff1f9d_5038916.png "女娲-158-5.5.png")

#### 特性说明
![特性1](https://images.gitee.com/uploads/images/2019/0901/141020_8ae59f7f_5038916.png "特性1.png")

![特性2](https://images.gitee.com/uploads/images/2019/0901/141036_daffcec7_5038916.png "特性2.png")

![特性3](https://images.gitee.com/uploads/images/2019/0901/141050_0694c81d_5038916.png "特性3.png")

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
	    virtual void work()
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

以下均已悟空内存池为例，其他内存池替换对应名称缩写即可

1. 头文件
    （1）要使用某一种内存池直接include那个内存池头文件，如使用悟空内存池，#include "WukongMemoryPool.h"
    （2）如果使用全部内存池也可直接#include "MemoryPool.h"

2. 自定义对象使用内存池    
    （1）不具有多态的自定义对象（公有继承UseWkG、UseWkT）
    ```
    #include "WukongMemoryPool.h"
    using namespace hzw;
    class A : public UseWk {...};//使用悟空内存池
    class B : public UseLk {...};//使用洛基内存池
    A* a{new A};//从内存池分配内存
    delete a;//将内存块归还内存池
    ```
    （2）具有多态的自定义对象（公有继承UseWkP、UseLkP）
    ```
    #include "MemoryPool.h"
    using namespace hzw;
    class A : public UseWkP {...};//使用悟空内存池(支持基类指针delete)
    class B : public UseLkP {...};//使用洛基内存池(支持基类指针delete)
    A* a{new A};//从内存池分配内存
    delete a;//将内存块归还内存池
    ```
2. 容器使用内存池(AllocWk<T>、AllocLk<T>)
    ```
    #include "MemoryPool.h"
    using namespace hzw;
    std::list<int, AllocWk<int>> list1;//使用悟空内存池
    std::list<int, AllocLk<int>> list2;//使用洛基内存池
    ```
3.  **注意事项** 

    UseWkP和UseLkP 比 UseWk和UseLk 虽然支持多态自定义对象，但要付出时间和空间

    使用基类指针执行delete，一定要使用UseWkP或 UseLkP（带P(polymorphic)尾缀的）

    总结：

    如果使用不带尾缀p：请确保用与数据类型匹配的指针delete，如果指针与指向类型不匹配（悟空内存泄漏、洛基直接异常）

    如果使用带尾缀p：支持多态的自定义类型，但需要额外开销
