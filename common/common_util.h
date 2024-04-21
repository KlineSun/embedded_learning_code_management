#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_FILE_LINE_LENTH 2048

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



char *get_time_str();
#define LOG_DEBUG(format, ...) printf("%s %s() %d: "format"\n", get_time_str(), __func__, __LINE__, ##__VA_ARGS__)

void trim_string(char *str);
bool is_all_spaces(const char* str);


#endif // COMMON_UTIL_H