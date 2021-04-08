#include "Alloc.h"
#include "Uninitialized.h"


namespace MySTL{



template <class T, class Alloc = alloc>
class Vector{
public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

// reverse_iterator 的内容没有实现


protected:
    typedef simple_alloc<value_type, Alloc> data_allocator;
    iterator start;
    iterator finish;
    iterator end_of_storage;




    // 私有函数  这个函数只是简单实现
    void fill_initialize(size_type n, const T& value){
        iterator result = data_allocator::allocate(n);
        start = result;
        for(;n>0;--n,++result){
            *result = value;
        }
        finish = start + n;
        end_of_storage = finish;
    }

    void insert_aux(iterator position, const T&x);
    void deallocate()    {   if(start)  data_allocate::deallocate(start, end_of_storage - start);    }


public:
    iterator begin() { return start; };
    const_iterator begin() const    {   return start;  };
    iterator end()  {   return finish; };
    const_iterator end() const {    return finish;  }

    size_type size() const   {return size_type(end() - begin());}
    size_type max_size() const {    return size_type(-1)/sizeof(T); }
    size_type capacity() const {    return size_type(end_of_storage - begin()); }
    bool empty() const { return begin() == end() };
    reference operator[](size_type n) { return *(begin() + n); }
    const_reference operator[](size_type n) const {   return *(begin() + n);    }
    // reverse_iterator 相关的几个函数没有实现

    // 构造函数相关：
    Vector():start(0), finish(0), end_of_storage(0){}

    //fill_initialize 还需完善
    Vector(size_type n, const T& value) {fill_initialize(n, value);}
    Vector(int n, const T& value) {fill_initialize(n, value);   }
    Vector(long n, const T& value)  {   fill_initialize(n, value);  }
    explicit Vector(size_type n) {  fill_initialize(n, T());   }

    Vector(const vector<T, Alloc>& x){
        // 这里有个辅助函数
        start = allocate_and_copy(x.end() - x.begin(), x.begin(), x.end());
        finish = start + (x.end() - x.begin());
        end_of_storage = finish;
    }

    Vector(const_iterator first, const_iterator last){
        size_type n = 0;
        distance(first, last, n);
        start = allocate_and_copy(n, first, last);
        finish = start + n;
        end_of_storage = finish;
    }

    ~Vector(){
        destroy(start, finish);
        deallocate();
    }

    Vector<T, Alloc>& operator=(const Vector<T, Alloc>& x)
    
    void reserve(size_type n){
        if(capacity() < n){
            const size_type old_size = size();
            iterator tmp = allocate_and_copy(n, start, finish);
            destroy(start, finish);
            deallocate();
            start = tmp;
            finish = tmp + old_size();
            end_of_storage = start + n;
        }
    }

    reference front() { return *begin();    }
    const_reference front() const { return *begin();    }
    reference back() {  return *(end() - 1);     }
    const_reference back()  const { return *(end()-1);  }
    void push_back(const T& x){
        if(finish != end_of_storge){
            construct(finish,x);
            ++finish;
        }
        else
            insert_aux(end(),x);
    }
    // void swap 函数未实现

    iterator insert(iterator position, const T& x)
    {
        size_type n = position - begin();
        if(finish != end_of_storage && position == end()){
            construct(finish, x);
            ++finish;
        }
        else
            insert_aux(position,x);
        return begin() + n;
    }

    iterator insert(iterator position)  {   return insert(position, T());   }
    
    //  类外定义
    void insert(iterator position, const_iterator first, const_iterator last);
   
    // 类外定义
    void insert(iterator pos, size_type n, const T& x);

    void insert(iterator pos, int n, const T& x){
        insert(pos, n, x);
    }
    void insert(iterator pos, long n, x){
        insert(pos, (size_type)n, x);
    }

    void pop_back(){
        --finish;
        destroy(finish);
        return position;
    }

    iterator erase(iterator position){
        if(position + 1 != end())
            copy(position + 1, finish, position);
        --finish;
        destroy(finish);
        return position;
    }

    iterator erase(iterator first, iterator last){
        iterator i = copy(last, finish, first);
        destroy(i, finish);
        finish = finish - (last - first);
        return first;
    }


    void resize(size_type new_size, const T& x){
        if(new_size < size())
            erase(begin() + new_size, end());
        else
            insert(end(), new_size - size(), x);
    }

    void resize(size_type new_size) {   resize(new_size, T());  }
    void clear()    {   erase(begin(), end());  }


protected:
iterator allocate_and_fill(size_type n, const T& x)
{
    iterator result = data_allocator::allocate(n);
    try{
        unitialized_fill_n(result, n, x);
        return result;
    }
    catch(data_allacator::deallocate(result, n));
}

iterator allocate_and_copy(size_type n, const_iterator first, const_iterator last)
{
    iterator result = data_allocator::allocate(n);
    try{
        unitialized_copy(first, last, result);
        return result;
    }
    catch(data_allocator::deallocate(result, n));
}

};
// 类声明结束



template <class T, class Alloc>
void Vector<T, Alloc>::insert_aux(iterator position, const T& x)
{
    if(finish != end_of_storage){
        construct(finish, *(finish - 1));
        ++finish;
        T x_copy = x;
        copy_backward(position, finish - 1, finish - 2);
        *position = x_copy;
    }
    else{
        const size_type old_size = size();
        const size_type len = old_size != 0 ?  2 * old_size : 1;
        iterator new_start = data_allocator::allocate(len);
        iterator new_finish = new_start;
        try{
            new_finish = uninitialized_copy(start, position, new_start);
            construct(new_finish, x);
            ++new_finish;
            new_finish = uninitialized_copy(position, finish, new_finish);
        }

        destroy(begin(), end());
        deallocate();
        start = new_start;
        finish = new_finish;
        end_of_storage = new_start + len;
    }
}

template <class T, class Alloc>
void Vector<T, Alloc>::insert(iterator position, size_type n, const T& x)
{
    if(n != 0){
        if(size_type(end_of_storage - finish) >= n){
            // 剩余空间满足 n
            T x_copy = x;
            const size_type elems_after = finish - position;
            iterator old_finish = finish;
            if(elems_after > n){
                uninitialized_copy(finish - n, finish, finish);
                finish += n;
                copy_backward(position, old_finish - n, old_finish);
                fill(position, position+n,x_copy);
            }
            else{
                unitialized_fill_n(finish, n - elems_after, x_copy);
                finish += n - elems_after;
                uninitialized_copy(position, old_finish, finish);
                finish += elems_after;
                fill(position, old_finish, x_copy);
            }
        }
        else{
            // 剩余空间不足 n
            const size_type old_size = size();
            const size_type len = old_size + max(old_size, n);
            iterator new_start = data_allocator::allocate(len);
            iterator new_finish = new_start;
            try{
                new_finish = uninitialized_copy(start, position, new_start);
                new_finish = uninitialized_copy(new_finish, n, x);
                new_finish = uninitialized_copy(position, finish, new_finish);
            }
            destroy(start, finish);
            deallocate();
            start = new_start;
            finish = new_finish;
            end_of_storage = new_start + len;
        }
    }
}

template <class T, class Alloc>
void Vector<T, Alloc>::insert(iterator position, const_iterator first, const_iterator last)
{
    if(first != last){
        size_type n = 0;
        distance(first, last, n);
        if(size_type(end_of_storage - finish) >= n){
            // 剩余空间足够
            const size_type elems_after = finish - position;
            iterator old_finish = finish;
            if(elems_after > n){
                uninitialized_copy(finish - n, finish, finish);
                finish += n;
                copy_backward(position, old_finish, old_finish);
                copy(first, last, position);
            }
            else{
                unitialized_copy(first + elems_after,last, finish);
                finish += n - elems_after;
                unitialized_copy(position, old_finish, finish);
                finish += elems_after;
                copy(first, last, position);
            }
        }
        else{
            // 剩余空间不足
            const size_type old_size = size();
            const size_type len = old_size + max(old_size, n);
            iterator new_start = data_allocator::allocate(len);
            iterator new_finish = new_start;
            try
            {
                new_finish = unitialized_copy(start, position, new_start);
                new_finish = uninialized_copy(first, last, new_finish);
                new_finish = uninialized_copy(position, finish, new_finish);
            }
            destroy(start, finish);
            deallocate();
            start = new_start;
            finish = new_finish;
            end_of_storage = new_start + len;
        }
    }
}






// 重载操作符
template <class T, class Alloc>
inline bool operator==(const Vector<T, Alloc>& x, const Vector<T,Alloc> &y)
{
    return x.size() == y.size() && equal(x.begin(), x.end(), y.begin(), y.end());
}

template <class T, class Alloc>
inline bool operator<(const Vector<T, Alloc>& x, const Vector<T, Alloc>& y)
{
    return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class T, class Alloc>
Vector<T, Alloc>& Vector<T, Alloc>::operator=(const Vector<T, Alloc>& x)
{
    if(&x != this){
        if(x.size() > capacity()){
            // 空间不足，需要申请新空间
            iterator new_start = allocate_and_copy(x.end() - x.begin(),
            x.begin(), x.end());
            destroy(start, finish);
            deallocate();
            start = new_start;
            end_of_storage = start + (x.end() - x.begin());
        }
        else if(size() >= x.size()){
            // 当前 size 大于等于  x.size()
            iterator i = copy(x.begin(), x.end(), begin());
            destroy(i, finish);
        }
        else{
            copy(x.begin(), x.begin() + size() , start);
            unitialized_copy(x.begin() + size(), x.end(), finish);
        }
        finish = start + x.size();
    }
    return *this;
}



}