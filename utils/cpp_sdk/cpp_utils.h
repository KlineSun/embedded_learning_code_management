#ifndef CPP_UTILS_H
#define CPP_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RISE_ORDER (0)
#define DESCEND_ORDER (1)

void say_hi(void);
// 为常用类型声明具体的排序函数
int cpp_sort_int(int arry[], int size, int order);
int cpp_sort_double(double arry[], int size, int order);
int cpp_sort_float(float arry[], int size, int order);

#ifdef __cplusplus
}
#endif

#endif //CPP_UTILS_H