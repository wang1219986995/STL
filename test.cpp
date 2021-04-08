#include <iostream>
#include <vector>
#include <memory>
using namespace std;

template <class T>
class Iterator{
public:
typedef T value_type;

};

template <class T>
using value_type_t = typename Iterator<T>::value_type;


struct MY__true_type{
    public:
    MY__true_type() {   cout << "is true type!" << endl;    }
};

template <class T>
class Traits{
public:
using is_POD = MY__true_type;
};



int main()
{   
    using is_pod_type = 
    typename Traits< value_type_t<int> >::is_POD;
    is_pod_type();
    //cout << sizeof(is_pod_type) << endl;
    
    return 0;
}