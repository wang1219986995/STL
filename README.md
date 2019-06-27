# MemoryPool

#### 介绍
c++线程安全内存池，与c++容器和自定义类轻松搭配使用。适用于有大量细粒度对象、反复释放构建对象的场景

#### WukongMemoryPool
悟空内存池

简单粗暴，提供快速分配回收

在运行时此内存池持有的内存不主动归还给系统，直到进程结束由系统回收

简而言之 不具有动态收缩能力，但换来更快的速度

#### LokiMemoryPool
洛基内存池

此内存池灵感来由于Loki库并加以改进，使用一些小诡计实现并提供动态收缩扩张能力，固名洛基

洛基提供动态收缩扩张功能更适合突发性内存需求

#### 悟空还是洛基？
1.速度至上：通常情况下悟空更快，但在反复释放构建场景可能洛基更快（洛基局部性更好，cache命中率高）

2.空间至上：洛基空间更紧凑，并提供1-3byte优化

3.鱼与熊掌兼得：无法提供建议，实践检验真理

#### 效能
开启四个线程，每个线程向各自std::list容器添加四种对象各五百万个，各线程重复此操作十次（测试代码在main.cpp）

`未使用内存池：用时323s、内存峰值6.2G`

![未使用内存池](https://images.gitee.com/uploads/images/2019/0627/161223_3707cd28_5038916.png "default_323_6.2.png")

`悟空内存池：用时22s、内存峰值5.8G`

![悟空内存池](https://images.gitee.com/uploads/images/2019/0627/161319_604cf5ba_5038916.png "Wukong_22_5.8.png")

`洛基内存池：用时67s、内存峰值5.2G`

![洛基内存池](https://images.gitee.com/uploads/images/2019/0627/161425_096c7c73_5038916.png "Loki_67_5.2.png")

#### 使用说明
1. 自定义对象使用内存池
    ```
    #include "MemoryPool.h"
    using namespace hzw;
    class A : public UseMemoryPool<WukongMemoryPool> {...};//使用悟空内存池
    class B : public UseMemoryPool<LokiMemoryPool> {...};//使用洛基内存池
    A* a{new A};//从内存池分配内存
    delete a;//将内存块归还内存池
    ```
2. 容器使用内存池allocator
    ```
    #include "MemoryPool.h"
    using namespace hzw;
    std::list<int, Allocator<int, WukongMemoryPool>> list1;//使用悟空内存池
    std::list<int, Allocator<int, LokiMemoryPool>> list2;//使用洛基内存池
    ```
3.  **注意事项** 
    使用多态特性时请谨慎
    ```
    class Base{...};
    class Derived : public Base{};
    Base* b{new Base};
    Base* d{new Derived};
    delete b;//正确回收
    delete d;//错误回收
    ```
    总结：请用与数据类型匹配的指针delete

#### 下版更新内容
使用和数据类型不匹配的指针delete，也能正确回收，使多态数据结构也可以使用内存池

实现方案：在分配内存块上方插入一个记录内存分配大小的数据

接口变动：增加一个模板参数选择是否支持多态数据结构，支持多态数据结构需要多余内存记录分配大小，
  如果不需要多态那就是多余，不为不需要的功能负担，固将支持多态功能作为可选项
