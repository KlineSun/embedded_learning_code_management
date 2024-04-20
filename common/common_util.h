#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

char *get_time_str();
#define LOG_DEBUG(format, ...) printf("%s %s() %d: "format"\n", get_time_str(), __func__, __LINE__, ##__VA_ARGS__)



#endif // COMMON_UTIL_H