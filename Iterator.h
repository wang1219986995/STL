#include <stdio.h>


namespace MySTL{

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag:public input_iterator_tag {};
struct bidirectional_iterator_tag:public forward_iterator_tag{};
struct random_access_iterator_tag : public bidirectional_iterator_tag{};


template <class Category, class T, class Distance = ptrdiff_t,
        class Pointer = T*, class Reference = T &>
struct Iterator
{
    typedef Category    iterator_category;
    typedef T           value_type;
    typedef Distance    difference_type;
    typedef Pointer     pointer;
    typedef Reference   reference;
};




template <class Iterator>
struct Iterator_Traits{
    typedef typename Iterator::iterator_category        iterator_category;
    typedef typename Iterator::value_type               value_type;
    typedef typename Iterator::difference_type          difference_type;
    typedef typename Iterator::pointer                  pointer;
    typedef typename Iterator::reference                reference;
};

template <class T>
struct Iterator_Traits<T *>{
    typedef  random_access_iterator_tag        iterator_category;
    typedef  T                                  value_type;
    typedef  ptrdiff_t                             difference_type;
    typedef  T*                                    pointer;
    typedef  T&                                    reference;
};



template <class T>
struct Iterator_Traits<const T *>{
    typedef  random_access_iterator_tag        iterator_category;
    typedef  T                                  value_type;
    typedef  ptrdiff_t                             difference_type;
    typedef  const T*                                    pointer;
    typedef  const T&                                    reference;
};




template <class Iterator>
using iterator_category_t =
    typename Iterator_Traits<Iterator>::iterarot_category;

template <class Iterator>
using value_type_t = typename Iterator_Traits<Iterator>::value_type;

template <class Iterator>
using difference_type_t = typename Iterator_Traits<Iterator>::difference_type;

template <class Iterator>
using pointer_t = typename Iterator_Traits<Iterator>::pointer;

template <class Iterator>
using reference_t = typename Iterator_Traits<Iterator>::reference;


// distance函数实现：
template <class InputIterator, class Distance>
inline difference_type_t<InputIterator> __distance(InputIterator first, InputIterator last, input_iterator_tag)
{
    difference_type_t<InputIterator> n = 0;
    while(first != last) ++first, ++n;
    return n;
}

template <class InputIterator>
inline difference_type_t<InputIterator> __distance(InputIterator first, InputIterator last, random_access_iterator_tag)
{
    return last - first;
}

template <class InputIterator>
inline difference_type_t<InputIterator> distance(InputIterator first, InputIterator last)
{
    return __distance(first, last, iterator_category_T<InputIterator>());
}

// advance 函数实现
template <class InputIterator , class Distance>
inline void __advance(InputIterator &i, Distance n, input_iterator_tag)
{
    while(n--) ++i;
}

template <class InputIterator, class Distance>
inline void __advance(InputIterator &i, Distance n, bidirectional_iterator_tag)
{
    if( n >= 0)
        while(n--)  ++i;
    else 
        while(n++)  --i;
}

template <class InputIterator, class Distance>
inline void __advance(InputIterator &i ,Distance n, random_access_iterator_tag)
{
    i += n;
}

template <class InputIterator, class Distance>
inline void advance(InputIterator &i, Distance)
{
    __advance(i, n, iterator_category_t<InputIterator>());
}



// 三种迭代器的实现：  insert, reverse, stream







;


