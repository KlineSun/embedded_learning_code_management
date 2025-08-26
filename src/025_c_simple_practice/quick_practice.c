#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common_util.h"
#include "cpp_utils.h"

#define DATA_SAMPLE_SIZE (20)

static int num_array[DATA_SAMPLE_SIZE];

int main(int argc, const char **argv)
{
    int max_len = 0, err = 0;

    max_len = sizeof(num_array) / sizeof(int);
    printf("Enter with array length: %d!\n", max_len);

    // 填充随机值
    fill_random_value(num_array, 3*DATA_SAMPLE_SIZE, -3*DATA_SAMPLE_SIZE, max_len);
    say_hi();

    err = cpp_sort_int(num_array, max_len, RISE_ORDER);
    if (err) {
        printf("Cpp sort failed: %d\n", err);
        return -1;
    }

    print_int_array(num_array, max_len);

    return 0;
}
