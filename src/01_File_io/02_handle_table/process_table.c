
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "process_table.h"

char *get_time_str() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    static char time_str[128] = {0};

    sprintf(time_str, "%d-%02d-%02d %02d:%02d:%02d",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    return time_str;
}

static int g_info_type = -1;

static student_t *g_student_ptr = NULL;
static fruit_t *g_fruit_ptr = NULL;

/**
 * 
 * fd: input file
 * buf: output buf
 * max_len: maximum length
 * 
 * return: Length of content read
*/
int read_line(int fd, char *buf, int max_len)
{
    if (fd < 0 || buf == NULL || max_len <= 0 || max_len > MAX_BUF_SIZE) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    char line_buf[MAX_BUF_SIZE] = {0};
    char ch = '\0';
    int cnt = -1;
    int i = 0;
    while (1) {
        cnt = read(fd, &ch, 1);
        if (cnt <= 0) {
            break;
        }
        if (ch == '\n' || ch == '\r' || ch == '\0') {
            break;
        }
        if (i < max_len - 1) {
            line_buf[i++] = ch;
        } else {
            LOG_DEBUG("line too long!");
            break;
        }
    }

    if (i <= 0) {
        return -1;
    }

    if (i <= max_len - 1) {
        line_buf[i++] = '\0';
        strncpy(buf, line_buf, max_len);
        return i;
    }
    return -1;
}

int parse_type(const char* buf)
{
    if (buf == NULL) {
       LOG_DEBUG("invalid parameter!");
       return -1;
    }

    char *temp = strstr(buf, "info_type");
    if (temp == NULL)
        return -1;

    char *type = strtok(temp, ",");
    type = strtok(NULL, ",");
    if (type == NULL)
        return -1;
    
    if (0 == strncasecmp(type, "student")) {
        return STUDENT;
    } else if (0 == strncasecmp(type, "fruit")) {
        return FRUIT;
    } else {
        LOG_DEBUG("Unsupport info type!");
        return -1;
    }
}


int student_info_handler(const char* info, char * result, int len)
{
    if (info == NULL || result == NULL || len < 0) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    if (strstr(line, "name") == NULL && g_student_ptr == NULL) {
        LOG_DEBUG("Configuration fields not read!");
        return -1;
    }

    // parse config field
    if (g_student_ptr == NULL) {
        g_student_ptr = malloc(sizeof(student_t));
        if (g_student_ptr == NULL) {
            LOG_DEBUG("Malloc error!");
            return -1;
        }
        int i = 1;
        char *temp = NULL;
        temp = strtok(info, ",");
        temp = strtok(NULL, ",");
        /*  CHINESE = 1,
            MATH,
            ENGILISH,
            PHYSICS,
            CHEMISTRY,
            BIOLOGY,
            POLITICS,
            HISTORY,
            GEOGRAPHY
        */
        while (temp != NULL) {
            if (strncasecmp(temp, "Total") == 0) {

            } else if (strncasecmp(temp, "Rank") == 0) {

            } else if (strncasecmp(temp, "Mark") == 0) {

            } else if (strncasecmp(temp, "Chinese") == 0) {
            } else if (strncasecmp(temp, "Math") == 0) {
            } else if (strncasecmp(temp, "English") == 0) {
            } else if (strncasecmp(temp, "Physics") == 0) {
            } else if (strncasecmp(temp, "Chemistry") == 0) {
            } else if (strncasecmp(temp, "Biology") == 0) {
            } else if (strncasecmp(temp, "Politics") == 0) {
            } else if (strncasecmp(temp, "History") == 0) {
            } else if (strncasecmp(temp, "Geography") == 0) {
            } else {
                LOG_DEBUG("Unidentified information!");
                continue;
            }
            temp = strtok(NULL, ",");
            i++;
        }
        /* continue to... */
        /* continue to... */
        /* continue to... */

        if (i == 1) {
            LOG_DEBUG("Too few parameters in this line: %s", line);
            return -1;
        }
    } else {
        // 使用链表保存
    }
}

int parse_line(const char *line, char * result, int len)
{
    if (line == NULL || result == NULL || len <= 0) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    // parse type
    if (g_info_type < 0) {
        int t = parse_type(line);
        if (t < 0) {
            LOG_DEBUG("Parse information type failed!");
            return -1;
        }
        LOG_DEBUG("Get information type: %d", t);
        g_info_type = t;
        return 0;
    }

    int i = 0;
    for (; i < LIST_LENGTH(g_info_handle_list); i++) {
        if (g_info_handle_list[i].type == g_info_type) {
            int rc = g_info_handle_list[i].info_handle_func(line, result, len);
            return rc;
        }
    }

    return 0;
}


/**
 * usage: ./process_table <raw csv file> -[operate type] <output csv file>
 * 
 * operate type:                                                                                                                                                                                                                    
 *      -cal: Calculate total score ranking
 *      -card: Generate transcripts for each student 
*/
int main(int argc, char const *argv[])
{

    if (argc < 4) {
        LOG_DEBUG("Usage: ./process_table <raw csv file> -[operate type] <output csv file>");
        return EXIT_FAILURE;
    }

    char raw_file[128] = {0};
    snprintf(raw_file, sizeof(raw_file), "%s", argv[1]);

    char operate_type[16] = {0};
    char output_file[128] = {0};
    if (argc == 4) {
        snprintf(operate_type, sizeof(operate_type), "%s", argv[2]);
        snprintf(output_file, sizeof(output_file), "%s", argv[3]);
    } else {
        LOG_DEBUG("Too many paramter!\r\nUsage: ./process_table <raw csv file> -[operate type] <output csv file>");
    }

    //check input parameter
    if (strcmp(operate_type, "-cal") != 0 && strcmp(operate_type, "-card") != 0) {
        LOG_DEBUG("Invalid operate type!\r\nUsage: ./process_table <raw csv file> -[operate type] <output csv file>");
        return EXIT_FAILURE;
    }
    if (strstr(raw_file, ".csv") == NULL || strstr(output_file, ".csv") == NULL) {
        LOG_DEBUG("Invalid file format!\r\nUsage: ./process_table <raw csv file> -[operate type] <output csv file>");
        return EXIT_FAILURE;
    }

    LOG_DEBUG("raw_file = %s, operate type = %s, output file = %s", raw_file, operate_type, output_file);

    int raw_file_fd = open(raw_file, O_RDONLY);
    if (raw_file_fd < 0) {
        perror("open error");
        return EXIT_FAILURE;
    }

    int output_file_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_file_fd < 0) {
        perror("open error");
        return EXIT_FAILURE;
    }

    LOG_DEBUG("open file success, raw_file_fd = %d, output_file_fd = %d", raw_file_fd, output_file_fd);

    char line_buf[1024] = {0};
    int read_cnt = -1;
    while (1)
    {
        read_cnt = read_line(raw_file_fd, line_buf, sizeof(line_buf));
        if (read_cnt <= 0)
            break;
        LOG_DEBUG("line_buf = %s", line_buf);

        
    }

    if (g_student_ptr != NULL) {
        free(g_student_ptr);
        g_student_ptr = NULL;
    }
    close(raw_file_fd);
    close(output_file_fd);
    return 0;
}
