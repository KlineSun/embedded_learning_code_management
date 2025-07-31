#include <stdio.h>
#include <stdlib.h>
#include "common_util.h"

extern int get_min(int a, int b);
extern int get_product(int a, int b);

int asm_get_min(int a, int b)
{
    int min;

    __asm__ volatile (
        "cmp %[a], %[b]\n"
        "movlt %[min], %[a]\n"
        "movgt %[min], %[b]\n"
        :[min]"=&r"(min)
        :[a]"r"(a), [b]"r"(b)
        :"cc"
    );

    return min;
}

int asm_get_product(int a, int b)
{
    int muti;

    __asm__ volatile (
        "mul %[muti], %[a], %[b]\n"
        :[muti]"=&r"(muti)
        :[a]"r"(a), [b]"r"(b)
        :"cc"
    );

    return muti;
}


int main(int argc, const char **argv)
{
    int val1 = 5, val2 = 10;
    LOG_INFO("Enter main: %d", argc);

    if (argc >= 3) {
        val1 = atoi(argv[1]);
        val2 = atoi(argv[2]);
    }

    LOG_INFO("get minimum number: %d", get_min(val1, val2));
    LOG_INFO("get product value: %d", get_product(val1, val2));


    LOG_INFO("get minimum number(inline function): %d", asm_get_min(val1, val2));
    LOG_INFO("get product value(inline function): %d", asm_get_product(val1, val2));

    return EXCUTE_SUCCESS_EXIT;
}
