#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "log_util.h"

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
#define LOG_DEBUG(format, ...) printf("%s %s() %d: "format"\n", get_time_str(), __func__, __LINE__, ##__VA_ARGS__)

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


void trim_string(char *str);
bool is_all_spaces(const char* str);
size_t file_mmap(const char *path, size_t size, int port, int flag, off_t offset, void **ptr);
void str2hex_print(void *ptr, size_t bytes);
//=====================func define end=======================

#endif // COMMON_UTIL_H