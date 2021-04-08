#ifndef MySTL_CONSTRUCT_H
#define MySTL_CONSTRUCT_H

#include <new.h>
#include "type_traits.h"

namespace MySTL{


template <class _T1, class _T2>
inline void _Construct(_T1* _p, const _T2& _value){
    new ( (void*)_p ) _T1(_value);
}

template <class _T1>
inline void _Construct(_T1* _p){
    new ( (void*) _p) _T1();
}

template <class Tp>
inline void _Destory(Tp* _pointer){
    _pointer->~_Tp();
}

template <class _ForwardIterator>
void _destory_aux(_ForwardIterator _first, _ForwardIterator _last, _false_type){
    for(; _first != _last; ++ _first){
        destroy(&*_first);
    }
}

template <class _ForwardIterator>
inline void _destory_aux(_ForwardIterator , _ForwardIterator, true_type){}

template <class _ForwardIterator, class Tp>
inline void _destory(_ForwardIterator _first, _ForwardIterator _last, Tp*)
{
    typedef typename type_traits<Tp>::hastrivial_destructor _Trivial_destructor;
    _destory_aux(_first, _last, _Trivial_destructor());
}

template <class _ForwardIterator>
inline void _Destory(_ForwardIterator _first, _ForwardIterator _last)
{
    _destory_aux(_first, _last, _value_type(_first));
}


inline void _Destroy(char*, char*) {}
inline void _Destroy(int*, int*) {}
inline void _Destroy(long*, long*) {}
inline void _Destroy(float*, float*) {}
inline void _Destroy(double*, double*) {}
inline void _Destroy(wchar_t*, wchar_t*){}




// 内存构造、销毁的封装函数
template <class _T1, class _T2>
inline void construct(_T1* _p, const _T2& _value){
    _Construct(_p, _value);
}

template <class _T1>
inline void construct(_T1* _p){
    _Construct(_p);
}

template <class Tp>
inline void destroy(Tp* _pointer){
    _Destroy(_pointer);
}

template <class _ForwardIterator>
inline void destroy(_ForwardIterator _first, _ForwardIterator _last){
    _Destory(_first, _last);
}


};
#endif