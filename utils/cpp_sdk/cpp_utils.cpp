#include <iostream>
#include <vector>
#include <functional>
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include "cpp_utils.h"

using namespace std;

void say_hi()
{
    cout << "Hello in cpp_utils!" << endl;
}

template <class T>
int cpp_sort_template(T arry[], int size, int order)
{
    int i = 0;
    vector<T> v;

    if (!arry || size <= 0) {
        cout << "Hello in cpp_utils!" << endl;
        return -EINVAL;
    }
    v.reserve(size);

    for (i = 0; i < size; i++) {
        v.emplace_back(arry[i]);
    }

    if (order == RISE_ORDER) {
        sort(v.begin(), v.end(), std::less<T>());
    } else if (order == DESCEND_ORDER) {
        sort(v.begin(), v.end(), std::greater<T>());
    } else {
        cout << "Invalid order: " << order << endl;
        return -EINVAL;
    }

    copy(v.begin(), v.end(), arry);
    return 0;
}

extern "C" int cpp_sort_int(int arry[], int size, int order) {
    return cpp_sort_template(arry, size, order);
}

extern "C" int cpp_sort_double(double arry[], int size, int order) {
    return cpp_sort_template(arry, size, order);
}

extern "C" int cpp_sort_float(float arry[], int size, int order) {
    return cpp_sort_template(arry, size, order);
}

