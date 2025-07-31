
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *get_time_str()
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    static char time_str[128] = {0};

    sprintf(time_str, "%d-%02d-%02d %02d:%02d:%02d ",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    return time_str;
}