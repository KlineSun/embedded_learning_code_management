#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "log_util.h"

#ifdef ENABLE_ASAN
// 硬编码 ASan 选项（仅在 ASAN=1 时生效）
static __attribute__((used))
const char *__asan_default_options() {
    return "detect_leaks=1";  // 可添加其他选项
}
#endif

//=====================type define begin=====================
#define COLOR_T int
#define angle_t double
typedef unsigned char u_8bit_t;
//=====================type define end=======================


//=====================number define begin===================
#define EXCUTE_SUCCESS_EXIT 0 
#define EXCUTE_FAILED_EXIT -1
#define CHINESE_BYTES (2)
#define MAX_FILE_LINE_LENTH 2048
#define PI (3.1415)
#define MAX_SHELL_CMD_LEN       (1024)
#define MAX_SHELL_RESULT_LEN    (1024)


//color
#define RGB_COLOR_WHITE (0xffffff)
#define RGB_COLOR_RED (0xff0000)
#define RGB_COLOR_GREEN (0x00ff00)
#define RGB_COLOR_VLUE (0x0000ff)
#define RGB_COLOR_BLACK (0x000000)
//=====================number define end=====================


//=====================path define begin=====================
#define FB_PATH "/dev/fb0"
#define FONT_DICTIONARY_PATH "/mnt/res/fonts/simsun.ttc"
//=====================path define end=======================


//=====================func define begin=====================

#define LIST_ADD_NODE(head, node) { \
        if (head != NULL && node != NULL) { \
            node->next = head; \
            head = node; \
        } \
    }

#define LIST_FREE(head) { \
        if (head != NULL) { \
            do { \
                typeof(head) tmp_head = head; \
                head = head->next; \
                free(tmp_head); \
                tmp_head = NULL; \
            } while (head != NULL); \
        } \
    }

#define LIST_LEN(x) (sizeof(x) / sizeof((x)[0]))

#define ANGLE(x) (angle_t)((x / 360) * PI * 2)
#define MKCMD(cmd) ("bash -c \"" cmd "\" 2>&1")

#define FILE_READ_FLAG (0)
#define FILE_WRITE_FLAG (1)
typedef struct {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
} region_2d;


void trim_string(char *str);
bool is_all_spaces(const char* str);
size_t file_mmap(const char *path, size_t size, int port, int flag, off_t offset, void **ptr);
void str2hex_print(void *ptr, size_t bytes);
int shell_cmd_excute(const char *cmd, char *result, int len);
int read_file_string(const char *path, char *buf, size_t len);
int write_file_string(const char *path, char *buf, size_t len);
int fill_random_value(int nums[], int max, int min, int size);
void print_int_array(int num[], int size);
//=====================func define end=======================


/*=============================inline func begin===================================*/
static inline void binary_print(void *buf, int num_bytes)
{
    int i = 0, j = 0;
    char *tmp = (char *)buf;

    printf("binary format(LSB): ");
    for (i = 0; i < num_bytes; i++) {
        for (j = 7; j >= 0; j--)
            printf("%d", (tmp[i] & (1 << j)) ? 1: 0);
        printf(" ");
    }
    printf("\n"); 
}
/*=============================inline func end===================================*/

#endif // COMMON_UTIL_H