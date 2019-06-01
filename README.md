# MemoryPool

#### 介绍
c++线程安全内存池，与c++容器和自定义类轻松搭配使用。适用于有大量细粒度对象、反复释放构建对象的场景

#### 使用场景
细粒度内存请求、各线程请求内存大小各不相同、 反复申请相同大小空间，时空效率越优

#### 使用说明
1. 自定义对象使用内存池
    class A : public hzw::UseMemoryPool {...};
    A* a{new A};//从内存池分配内存
    delete a;//将内存块归还内存池
2. 容器使用内存池allocator
    std::list<int, hzw::Allocator<int>> list;
3. 注意事项
    使用多态特性时请谨慎
    class Base{...};
    class Derived : public Base{};
    Base* b{new Base};
    Base* d{new Derived};
    delete b;//正确回收
    delete d;//错误回收。不会直接导致程序崩溃，但将产生内存内碎片
    总结：请用与数据类型匹配的指针delete

