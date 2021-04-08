#include "Type_Traits.h"
#include "Construct.h"
#include "Iterator.h"
#include "Algobase.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>




namespace MySTL{



template <class InputIterator, class ForwardIterator, class T>
inline ForwardIterator
uninitialized_copy_aux(InputIterator first, InputIterator last, ForwardIterator result, false_type)
{
    ForwardIterator cur = result;
    try
    {
        for(; first != last; ++first, ++cur){
            construct(&*cur, *first);
        }
        return cur;
    }
    catch(destory(result, cur));
    
}


template <class InputIterator, class ForwardIterator, class T>
inline ForwardIterator
uninitialized_copy_aux(InputIterator first, InputIterator last, ForwardIterator result, true_type)
{
    return copy(first, last, result);
}


// 以上是根据  true_type 和 false_type 的类型决定调用函数








template <class InputIterator, class ForwardIterator, class T>
inline ForwardIterator
    uninitialized_copy(InputIterator first, InputIterator last,
                        ForwardIterator result)
{
    using is_POD = type_traits< value_type_t<InputIterator>>::is_POD_type;
    return uninitialized_copy_aux(first, last, result, is_POD() );
}


inline char* uninitialized_copy(const char* first, const char* last, char* result)
{
    memmove(result, first, last - first);
    return result + (last - first);
}

inline wchar_t* unitialized_copy(const wchar_t* first, const wchar_t* last, wchar_t* result)
{
    memmove(result, first, sizeof(wchar_t)* (last - first));
    return result + (last - first);
}



// 下面还有一些函数使用到了  pair  ，还没写



// 以下是fill 相关函数部分：

// unitialized_fill  相关：
template <class ForwardIterator, class T>
void unitialized_fill_aux(ForwardIterator first, ForwardIterator last, const T& x, true_type)
{
    fill(first , last, x);
}

template <class ForwardIterator, class T>
void unitialized_fill_aux(ForwardIterator first, ForwardIterator last, const T& x, false_type)
{
    ForwardIterator cur = first;
    try
    {
        for(; cur != last; ++cur)
        {
            construct(&*cur, x);
        }
    }
    catch(destroy(first, cur));
    
}

template <class ForwardIterator, class T>
inline void unitialized_fill(ForwardIterator first, ForwardIterator last, const T& x)
{
    unitialized_fill_aux(first, last, x, type_traits< value_type_t<ForwardIterator> >());
}



// unitialized_fill  相关：

template <class ForwardIterator , class Size, class T>
inline ForwardIterator uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, true_type)
{
    return fill_n(first, n, x);
}


template <class ForwardIterator , class Size, class T>
inline ForwardIterator uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, false_type)
{
    ForwardIterator cur = first;
    try{
        for(; n > 0; --n, ++cur)
            construct(&*cur, x);
        return cur;
    }
    catch(destroy(first, cur));
}

template <class ForwardIterator, class Size, class T>
inline ForwardIterator unitialized_fill_n(ForwardIterator first, Size n, const T& x)
{
    return unitialized_fill_aux(first, n, type_traits< value_type_t<ForwardIterator> >());
}



// unitialized_fill_n  相关：



}