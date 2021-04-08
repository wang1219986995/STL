#include "Type_Traits.h"
#include "Iterator.h"

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <new.h>


namespace MySTL{



// copy 函数

template <class InputIterator, class OutputIterator>
inline OutputIterator __copy(InputIterator first, InputIterator last, OutputIterator result, input_iterator_tag)
{
    for( ;first != last; ++result, ++ first){
        *result = *first;
    }
    return result;
}

template <class RandomAccessIterator, class OutputIterator>
inline OutputIterator __copy_d(RandomAccessIterator first, RandomAccessIterator last, OutputIterator result, Distance)
{
    for(Distance n = last - first; n > 0; --n, ++result, ++first)
        *result = *first;
    return result;
}

template <class InputIterator, class OutputIterator>
inline OutputIterator __copy(InputIterator first, InputIterator last, OutputIterator result, random_access_iterator_tag)
{
    return __copy_d(first, last, result, difference_type_t<InputIterator>());
}


template <class InputIterator, class OutputIterator>
inline OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
{
    return __copy(first, last, result, iterator_category_t<InputIterator>());
}


inline char* copy(const char* first, const char* last, char* result)
{
    memmove(result, first, last - first);
    return result + (last - first);
}

inline wchar_t* copy(const wchar_t* first, const wchar_t* last, wchar_t* result)
{
    memmove(result, first, sizeof(wchar_t)*(last -first) );
    return result + (last - first);
}

// 这里还有一部分 copy  相关的函数没有实现

// copy_backward  相关函数实现

template <class BidirectionalIterator1, class BidirectionalIterator2>
inline BidirectionalIterator2 copy_backward(BidirectionalIterator1 first, BidirectionalIterator2 last, BidirectionalIterator2 result)
{
    while(first != last )   *--result = *--last;
    return result;
}















template <class ForwardIterator, class T>
void fill(ForwardIterator first, ForwardIterator last, const T& value)
{
    for(; first != last; ++first){
        *first = value;
    }
}

template <class OutputIterator, class Size, class T>
OutputIterator fill_n(OutputIterator first, Size n, const T& value)
{
    for(; n > 0; --n, ++first)
        *first = value;
    return first;
}



// equal 函数
template <class InputIterator1, class InputIterator2>
inline bool equal(InputIterator1 first1,InputIterator1 last1,
InputIterator2 first2, InputIterator2 last2)
{
    for(; first1 != last1; ++first1, ++ last2){
        if(*first1 != *first2)
            return false;
        return true;
    }
}

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
inline bool equal(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, BinaryPredicate binary_pred) {
  for ( ; first1 != last1; ++first1, ++first2)
    if (!binary_pred(*first1, *first2))
      return false;
  return true;
}


template <class InputIterator1, class InputIterator2>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
InputIterator2 first2, InputIterator2 last2)
{
    for(; first1 != last1 && first2 != last2; ++first1, ++first2){
        if(*first1 < *first2)
            return true;
        if(*first1 > *first2)
            return false;
    }
    return first1 != last1 && first2 != last2;
}


template <class InputIterator1, class InputIterator2, class Compare>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
			     InputIterator2 first2, InputIterator2 last2,
			     Compare comp) {
  for ( ; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (comp(*first1, *first2))
      return true;
    if (comp(*first2, *first1))
      return false;
  }
  return first1 == last1 && first2 != last2;
}



}