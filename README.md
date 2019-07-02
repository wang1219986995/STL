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

此内存池灵感来由于Loki库并加以改进，固名洛基

洛基提供动态收缩扩张功能更适合突发性内存需求

#### 悟空还是洛基？
1.速度至上：通常情况下悟空更快，但在反复释放构建场景可能洛基更快（洛基局部性更好，cache命中率高）

2.空间至上：洛基提供1-3byte优化，优选洛基

3.鱼与熊掌兼得：无法提供建议，实践检验真理

#### 效能
开启四个线程，每个线程向各自std::list容器添加四种对象各五百万个，各线程重复此操作十次（测试代码在main.cpp）

`未使用内存池：用时259s、内存峰值6.2G`

![未使用内存池](https://images.gitee.com/uploads/images/2019/0702/110247_331dbb4a_5038916.png "default_259_6.2.png")

`悟空内存池：用时20s、内存峰值5.2G`

![悟空内存池](https://images.gitee.com/uploads/images/2019/0702/110322_252cd5a2_5038916.png "wk_20_5.2.png")

`洛基内存池：用时41s、内存峰值5.0G`

![洛基内存池](https://images.gitee.com/uploads/images/2019/0702/110433_f13dafea_5038916.png "lk_41_5.0.png")

#### 使用说明
1. 自定义对象使用内存池    
    （1）不具有多态的自定义对象（公有继承UseWk、UseLk）
    ```
    #include "MemoryPool.h"
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
