#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <stdio.h>
#include <string.h>

char *get_time_str();

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
} log_level_t;

#ifndef CUR_LOG_LEVEL
#define CUR_LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG(level, format, ...) do {\
    int print_time = 0;\
    char lv_str[8] = {0};\
    if (level >= CUR_LOG_LEVEL) {\
        switch (level)\
        {\
            case LOG_LEVEL_DEBUG: lv_str[0] = 'D'; break;\
            case LOG_LEVEL_VERBOSE: lv_str[0] = 'V'; break;\
            case LOG_LEVEL_INFO: lv_str[0] = 'I'; break;\
            case LOG_LEVEL_WARN: lv_str[0] = 'W'; break;\
            case LOG_LEVEL_ERROR: lv_str[0] = 'E'; print_time = 1; break;\
            case LOG_LEVEL_FATAL: strncpy(lv_str,"FATAL", 8); print_time = 1; break;\
            default: strncpy(lv_str,"UNKNOWN", 8); break;\
        }\
        printf("%s%s %d %s: "format"\n", \
                print_time?get_time_str():"", __func__, __LINE__, lv_str, ##__VA_ARGS__);\
    }\
} while(0);


#define LOG_DEBUG(format, ...) LOG(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define LOG_VERBOSE(format, ...) LOG(LOG_LEVEL_VERBOSE, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG(LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define LOG_ERR(format, ...) LOG(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) LOG(LOG_LEVEL_FATAL, format, ##__VA_ARGS__)

#endif //LOG_UTIL_H