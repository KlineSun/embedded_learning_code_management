#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common_util.h"

#define DATA_SAMPLE_SIZE (20)
#define COL_SIZE (3)


static int num_array[DATA_SAMPLE_SIZE];

void print_array(int num[], int size)
{
    printf("array: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", num[i]);
    }
    printf("\n");
}

void swap_elem(int num[], int idx1, int idx2)
{
    int tmp = 0;
    if (!num || idx1 < 0 || idx2 < 0)
        return;
    
    tmp = num[idx1];
    num[idx1] = num[idx2];
    num[idx2] = tmp;
    // printf("swap %d <-> %d\n", num[idx2], num[idx1]);
}

int partition(int num[], int start, int end)
{
    int cmp_val = 0, parting_ln = start/* 分割线的坐标 */, i = 0;

    // printf("Enter with start %d end %d!\n", start, end);
    if (!num || start < 0 || end < 0)
        return -1;

    cmp_val = num[end]; // 划分数组的基准值

    // 遍历区间数组
    for (i = start; i < end; i++) {
        if (num[i] <= cmp_val) {
            if ( i != parting_ln) // 如果已经在分割线上，则不进行交换
                swap_elem(num, parting_ln, i); // 交换元素，小的元素放到分割线左边
            parting_ln++;
        }
    }

    // 遍历完区间之后，将基准值填到分割线上
    if (end != parting_ln) // 如果已经在分割线上，则不进行交换
        swap_elem(num, parting_ln, end);
    return parting_ln;
}

void quick_sort(int nums[], int start, int end)
{
    int parting_ln = 0; // 分割线坐标
    if (!nums || start < 0 || end < 0)
        return;

    // 在区间存在时，进行区间的分割
    if (start < end) {
        // 划分当前区间
        parting_ln = partition(nums, start, end);
        if (parting_ln < 0) { // 坐标不应该小于0
            printf("Sort error!\n");
            return;
        }

        // 排序左区间
        if (parting_ln > start)
            quick_sort(nums, start, parting_ln - 1);

        // 排序右区间
        if (parting_ln < end)
            quick_sort(nums, parting_ln + 1, end);
    }

}


int** threeSum(int* nums, int numsSize, int* returnSize, int** returnColumnSizes)
{
    int i, j, k, count = 0, asume_cnt = 0, total = 0, col_byte_size = 0, loop_cnt = 0;
    int (*result_buf)[COL_SIZE] = NULL, *column_sizes = NULL;

    if (!nums || numsSize < COL_SIZE) {
        printf("Invalid parameter in threeSum!\n");
        return NULL;
    }

    // 先排序
    print_array(nums, numsSize);
    quick_sort(nums, 0, numsSize - 1);
    print_array(nums, numsSize);

    col_byte_size = sizeof(int[COL_SIZE]);
    asume_cnt = numsSize / COL_SIZE; // 先假设匹配的数量为数据量的1/3
    result_buf = calloc(asume_cnt, col_byte_size); // 每次匹配分配3个int的空间
    if (!result_buf) {
        printf("Alloc memory failed!\n");
        return NULL;
    }

    for (i = 0; i < numsSize; i++) {
        if (i > 0 && nums[i] == nums[i - 1])
            continue;
        for (j = i+1; j < numsSize; j++) {
            if (nums[j] == nums[j - 1])
                continue;
            for (k = j+1; k < numsSize; k++) {
                if (nums[k] == nums[k - 1])
                    continue;
                loop_cnt++;
                if (k >= numsSize)
                    break;
                total = nums[i] + nums[j] + nums[k];
                //printf("Total (%d, %d, %d) = %d\n", nums[i], nums[j], nums[k], total);
                if (total == 0) {
                    // 找到了和为0的组合
                    
                    result_buf[count][0] = nums[i];
                    result_buf[count][1] = nums[j];
                    result_buf[count][2] = nums[k];
                    count++;

                    if (count >= asume_cnt) {
                        asume_cnt *= 2; // 假设匹配密度平均分布
                        result_buf = realloc(result_buf, asume_cnt*col_byte_size);
                        if (!result_buf) {
                            printf("Alloc memory failed!\n");
                            free(result_buf);
                            *returnSize = 0;
                            return NULL;
                        }
                        printf("Capacity of buf not enough, reallocate %d bytes!\n", asume_cnt*col_byte_size);

                        // 清零新增的内存
                        memset(result_buf + count*col_byte_size, 0, asume_cnt*col_byte_size - count*col_byte_size);
                    }
                }
            }
        }
    }

    *returnSize = count;
    column_sizes = (int *)calloc(count, sizeof(int));
    if (!column_sizes) {
        printf("Alloc memory failed!\n");
        return NULL;
    }
    for (int i = 0; i < count; i++)
        column_sizes[i] = COL_SIZE; // 每组都有3个有效数据
    *returnColumnSizes = column_sizes;
    printf("loop count: %d\n", loop_cnt);
    return (int **)result_buf;
}

int** sub_threeSum(int* nums, int numsSize, int* returnSize, int** returnColumnSizes)
{
    int low, mid, high, count = 0, asume_cnt = 0, total = 0, col_byte_size = 0, loop_cnt = 0;
    int (*result_buf)[COL_SIZE] = NULL, *column_sizes = NULL;
    if (!nums || numsSize < COL_SIZE) {
        printf("Invalid parameter in threeSum!\n");
        return NULL;
    }

    // 先排序
    print_array(nums, numsSize);
    quick_sort(nums, 0, numsSize - 1);
    print_array(nums, numsSize);

    col_byte_size = sizeof(int[COL_SIZE]);
    asume_cnt = numsSize / COL_SIZE; // 先假设匹配的数量为数据量的1/3
    result_buf = calloc(asume_cnt, col_byte_size); // 每次匹配分配3个int的空间
    if (!result_buf) {
        printf("Alloc memory failed!\n");
        return NULL;
    }

    for (high = 2; high<numsSize; high++) {
        // 高元素不能小于0，跳过了全为负数的情况和重复值的情况
        if (nums[high] == nums[high - 1] || nums[high] < 0)
            continue;

        for (low = 0; low<high-1; low++) {
            //低元素不能大于0，跳过了全为正数的和重复值情况
            if (nums[low] > 0 || (low > 0 && nums[low] == nums[low - 1]))
                continue;

            for (mid = low + 1; mid < high; mid++) {
                if (nums[mid] == nums[mid - 1])
                    continue;

                loop_cnt++;
                total = nums[low] + nums[mid] + nums[high];
                // printf("Total (%d, %d, %d) = %d\n", nums[low], nums[mid], nums[high], total);
                if (total == 0) {
                    result_buf[count][0] = nums[low];
                    result_buf[count][1] = nums[mid];
                    result_buf[count][2] = nums[high];
                    count++;

                    if (count >= asume_cnt) {
                        asume_cnt *= 2; // 重新分配大小翻倍
                        result_buf = realloc(result_buf, asume_cnt*col_byte_size);
                        if (!result_buf) {
                            printf("Alloc memory failed!\n");
                            free(result_buf);
                            *returnSize = 0;
                            return NULL;
                        }
                        printf("Capacity of buf not enough, reallocate %d bytes!\n", asume_cnt*col_byte_size);

                        // 清零新增的内存
                        memset(result_buf + count*col_byte_size, 0, asume_cnt*col_byte_size - count*col_byte_size);
                    }
                }
                // usleep(10* 1000);
            }
        }
    }

    *returnSize = count;
    column_sizes = (int *)calloc(count, sizeof(int));
    if (!column_sizes) {
        printf("Alloc memory failed!\n");
        free(result_buf);
        *returnSize = 0;
        return NULL;
    }
    for (int i = 0; i < count; i++)
        column_sizes[i] = COL_SIZE; // 每组都有3个有效数据
    *returnColumnSizes = column_sizes;
    printf("loop count: %d\n", loop_cnt);
    return (int **)result_buf;
}

int** thrd_threeSum(int* nums, int numsSize, int* returnSize, int** returnColumnSizes)
{
    int i, low, high, count = 0, capacity = 10, loop_cnt = 0;
    int (*result_buf)[3] = NULL;  // 改为二维数组
    int* column_sizes = NULL;

    if (!nums || numsSize < 3) {
        *returnSize = 0;
        return NULL;
    }

    // 先排序
    quick_sort(nums, 0, numsSize - 1);

    // 初始分配
    result_buf = malloc(capacity * sizeof(int[3]));
    if (!result_buf) {
        *returnSize = 0;
        return NULL;
    }

    for (i = 0; i < numsSize - 2; i++) {
        // 跳过重复的i
        if (i > 0 && nums[i] == nums[i - 1]) {
            continue;
        }

        low = i + 1;
        high = numsSize - 1;
        int target = -nums[i];

        while (low < high) {
            int sum = nums[low] + nums[high];
            
            if (sum == target) {
                // 检查是否需要扩容
                if (count >= capacity) {
                    capacity *= 2;
                    int (*new_buf)[3] = realloc(result_buf, capacity * sizeof(int[3]));
                    if (!new_buf) {
                        free(result_buf);
                        *returnSize = 0;
                        return NULL;
                    }
                    result_buf = new_buf;
                }
                
                result_buf[count][0] = nums[i];
                result_buf[count][1] = nums[low];
                result_buf[count][2] = nums[high];
                count++;
                
                // 跳过重复的low和high
                while (low < high && nums[low] == nums[low + 1]) low++;
                while (low < high && nums[high] == nums[high - 1]) high--;
                low++;
                high--;
            } else if (sum < target) {
                low++;
            } else {
                high--;
            }
            loop_cnt++;
        }
    }

    // 分配列大小数组
    column_sizes = malloc(count * sizeof(int));
    if (!column_sizes) {
        free(result_buf);
        *returnSize = 0;
        return NULL;
    }
    
    for (int j = 0; j < count; j++) {
        column_sizes[j] = 3;
    }
    
    *returnSize = count;
    *returnColumnSizes = column_sizes;
    printf("loop count: %d\n", loop_cnt);
    return (int**)result_buf;  // 强制转换为 int**
}

int main(int argc, const char **argv)
{
    int max_len = 0, result_cnt = 0;
    int *col_size_ary = NULL;
    int (*result_buf)[COL_SIZE] = NULL;

    max_len = sizeof(num_array) / sizeof(int);
    printf("Enter with array length: %d!\n", max_len);

    // 填充随机值
    fill_random_value(num_array, 3*DATA_SAMPLE_SIZE, -3*DATA_SAMPLE_SIZE, max_len);

    // 使用穷举法对比结果
    result_buf = (int (*)[COL_SIZE])threeSum(num_array, max_len, &result_cnt, &col_size_ary);
    if (!result_buf) {
        printf("get threeSum error!\n");
        return -1;
    }
    printf("threeSum get %d results:\n", result_cnt);
    for (int i = 0; i < result_cnt; i++) {
        printf("{ ");
        for (int j = 0; j < col_size_ary[i]; j++)
            printf("%d%s ", result_buf[i][j], (j == (col_size_ary[i] - 1) ? "" : ","));
        printf("}\n");
    }
    free(col_size_ary);
    free(result_buf);
    col_size_ary = NULL;
    result_buf = NULL;

    result_buf = (int (*)[COL_SIZE])sub_threeSum(num_array, max_len, &result_cnt, &col_size_ary);
    if (!result_buf) {
        printf("get threeSum error!\n");
        return -1;
    }
    printf("threeSum get %d results:\n", result_cnt);
    for (int i = 0; i < result_cnt; i++) {
        printf("{ ");
        for (int j = 0; j < col_size_ary[i]; j++)
            printf("%d%s ", result_buf[i][j], (j == (col_size_ary[i] - 1) ? "" : ","));
        printf("}\n");
    }
    free(col_size_ary);
    free(result_buf);
    col_size_ary = NULL;
    result_buf = NULL;

    //thrd_threeSum
    result_buf = (int (*)[COL_SIZE])thrd_threeSum(num_array, max_len, &result_cnt, &col_size_ary);
    if (!result_buf) {
        printf("get threeSum error!\n");
        return -1;
    }
    printf("threeSum get %d results:\n", result_cnt);
    for (int i = 0; i < result_cnt; i++) {
        printf("{ ");
        for (int j = 0; j < col_size_ary[i]; j++)
            printf("%d%s ", result_buf[i][j], (j == (col_size_ary[i] - 1) ? "" : ","));
        printf("}\n");
    }

    free(col_size_ary);
    free(result_buf);

    return 0;
}
