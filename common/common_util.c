#include "common_util.h"
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

char *get_time_str()
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    static char time_str[128] = {0};

    sprintf(time_str, "%d-%02d-%02d %02d:%02d:%02d",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    return time_str;
}

void trim_string(char *str)
{
    // 找到第一个非空白字符的位置
    size_t start = strspn(str, " \t\r\f\v\n");

    // 如果整个字符串都是空白字符，则将其置为空字符串
    if (start == strlen(str)) {
        str[0] = '\0';
        return;
    }

    // 找到最后一个非空白字符的位置
    size_t end = strcspn(str + start, " \t\r\f\v\n");

    // 将字符串移动到修剪后的位置
    memmove(str, str + start, end);

    // 添加字符串结束符
    str[end] = '\0';
}

bool is_all_spaces(const char* str)
{
    if (str == NULL || *str == '\0')
        return true;

    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (!isspace(str[i])) {
            return false;
        }
    }
    return true;
}

