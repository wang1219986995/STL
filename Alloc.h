#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <new>

namespace MySTL{



class malloc_alloc{
private:
    static void* oom_malloc(size_t n);
    static void* oom_realloc(void* p, size_t new_sz);
    static void (* malloc_alloc_oom_handler)();

public:
    
    // 分配内存
    static void* allocate(size_t n)
    {
        void* result = malloc(n);
        if( 0 == result)  result = oom_malloc(n);
        return result;
    }

    static void deallocate(void* p, size_t n)
    {
        free(p);
    }

    static void* reallocate(void* p, size_t, size_t new_sz)
    {
        void* result = realloc(p, new_sz);
        if(0 == result)  result = oom_realloc(p,new_sz);
        return result;
    }

    static void (* set_malloc_handler(  void (*f)()  )  )()
    {
        void (* old)() = malloc_alloc_oom_handler;
        malloc_alloc_oom_handler = f;
        return (old);
    }

};

// 这里的函数直接加了 =0 是什么意思，怎么理解

void (* malloc_alloc::malloc_alloc_oom_handler)() = 0;


void* malloc_alloc::oom_malloc(size_t _n)
{
    void (* my_malloc_handler)();
    void* result;
    for(;;){
        my_malloc_handler = malloc_alloc_oom_handler;
        if( 0 == my_malloc_handler) { throw std::bad_alloc(); };
        (* my_malloc_handler)();
        result = malloc(_n);
        if(result)  return (result);
    }
}


void* malloc_alloc::oom_realloc(void* p, size_t n)
{
    void (* my_malloc_handler)();
    void* result;

    for(;;){
        my_malloc_handler = malloc_alloc_oom_handler;
        if(my_malloc_handler == 0)  { std::bad_alloc();    }
        (*my_malloc_handler)();
        result = realloc(p, n);
        if(result )  return (result);
    }
}



template<class Tp, class _Alloc>
class simple_alloc{
public:
    static Tp* allocate(size_t _n)
    {   return 0 ==_n ? 0 : (Tp*) _Alloc::allocate(_n*sizeof(Tp));  }
    
    static Tp* allocate(void)
    {   return (Tp*) _Alloc::allocate(sizeof(Tp));    }

    static void deallocate(Tp* _p, size_t _n)
    {   if( 0 != _n ) _Alloc::deallocate(_p, _n * sizeof(Tp));    }

    static void deallocate(Tp* _p)
    {   _Alloc::deallocate(_p, sizeof(Tp));    }

};
// 类声明结束





// 内存分配只考虑单线程情况
class _default_alloc_template
{
private:
    enum {_ALIGN = 8};
    enum {_MAX_BYTES = 128};
    enum {_NFREELISTS = 16};
    static size_t round_up(size_t _bytes)
    {   return (((_bytes) + (size_t)_ALIGN + 1) & ~((size_t)_ALIGN - 1)  );   }

    union Obj {
        union Obj* next_free_list;
        char client_data[1];
    };

    static Obj* free_list[_NFREELISTS];

    static size_t free_list_index(size_t _bytes)
    {   return (((_bytes) + (size_t)_ALIGN + 1) / (size_t)_ALIGN - 1);   }

    static void* refill(size_t _n);

    static char* chunk_alloc(size_t _size, int& _nobjs);

    static char* start_free;
    static char* end_free;
    static size_t heap_size;

public:
    static void* allocate(size_t _n)
    {
        Obj** my_free_list;
        Obj* result;
        if(_n > (size_t) _MAX_BYTES){
            return malloc_alloc::allocate(_n);
        }
        my_free_list = free_list + free_list_index(_n);
        result = *my_free_list;
        if(result == 0){
            void* r = refill(round_up(_n));
            return r;
        }
        *my_free_list = result -> next_free_list;
        return result;
    }

    static void deallocate(void* p, size_t n)
    {
        if(n > (size_t) _MAX_BYTES)
            malloc_alloc::deallocate(p, n);
        else{
            Obj** my_free_list = free_list + free_list_index(n);
            Obj* q = (Obj*) p;
            q -> next_free_list = *my_free_list;
            *my_free_list = q;
        }
    }

    static void* reallocate(void* _p, size_t _old_sz, size_t _new_sz);

};

typedef _default_alloc_template alloc;

// 暂时不知道这个重载有啥用
inline bool operator==(const _default_alloc_template&, const _default_alloc_template&)
{   return true;    }

void* _default_alloc_template::refill(size_t n)
{
    int nobjs = 20;
    char* chunk = chunk_alloc(n, nobjs);
    Obj** my_free_list;
    Obj* result;
    Obj* current_obj;
    Obj* next_obj;

    if( 1 == nobjs )   return (chunk);
    my_free_list = free_list + free_list_index(n);
    
    result = (Obj*) chunk;
    *my_free_list = next_obj = (Obj*)(chunk + n);
    for(int i = 1;  ;  i++){
        current_obj = next_obj;
        next_obj = (Obj*)((char*)next_obj+n);
        if(nobjs - 1 ==i){
            current_obj -> next_free_list = 0;
            break;
        }else{
            current_obj->next_free_list = next_obj;
        }
    }
    return (result);

}

char* _default_alloc_template::chunk_alloc(size_t size, int& nobjs)
{
    char* result;
    size_t total_bytes = size * nobjs;
    size_t bytes_left = end_free  - start_free;


    // 内存池空间完全够用
    if( bytes_left >= total_bytes){
        result = start_free;
        start_free += total_bytes;
        return (result);
    }
    // 剩余空间不完全满足需求量，但足够供应一个区块
    else if( bytes_left  >= size){
        nobjs = (int)(bytes_left / size);
        total_bytes = size * nobjs;
        result = start_free;
        start_free += total_bytes;
        return (result);
    }
    else{
        //内存池连一个区块都供应不了
        size_t bytes_to_get = 2 * total_bytes + round_up(heap_size >> 4);
        if(bytes_left > 0){
            Obj** my_free_list = free_list + free_list_index(bytes_left);
            ( (Obj*)start_free)->next_free_list = *my_free_list;
            *my_free_list = (Obj*)start_free;
        }
        start_free = (char*)malloc(bytes_to_get);
        if( 0 == start_free){
            Obj** my_free_list;
            Obj* p;
            for(size_t i = size; i <= (size_t)_MAX_BYTES; i += (size_t)_ALIGN){
                my_free_list = free_list + free_list_index(i);
                p = *my_free_list;
                if( 0 != p){
                    *my_free_list = p -> next_free_list;
                    start_free = (char*)p;
                    end_free = start_free + i;
                    return (chunk_alloc(size, nobjs));
                }
            }
        end_free = 0;
        start_free = (char*)malloc_alloc::allocate(bytes_to_get);
        }
        heap_size += bytes_to_get;
        end_free = start_free + bytes_to_get;
        return (chunk_alloc(size, nobjs));
    }
}





char* _default_alloc_template::start_free = 0;

char* _default_alloc_template::end_free = 0;


size_t _default_alloc_template::heap_size = 0;




typename _default_alloc_template::Obj* 
_default_alloc_template::free_list[_default_alloc_template::_NFREELISTS] = 
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };


}