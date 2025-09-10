#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "common_util.h"


void setup_data(int *val)
{
    *val = 1;
    asm volatile("" ::: "memory"); // 编译屏障，避免val的赋值顺序被打乱
    *val = 2;
}

int main(int argc, const char **argv)
{
    int i = 0;


    setup_data(&i);
    printf("i = %d", i);
    return 0;
}
