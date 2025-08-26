#include "common_util.h"
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


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


/**
 * @brief mmap a file
 * 
 * @param path path of the file
 * @param size map size, -1 means equal to file size.
 * @param port map port
 * @param flag operate options
 * @param offset map offset
 * @param ptr map pointer
 * 
 * @result return map size if succcess, else return -1.
*/
size_t file_mmap(const char *path, size_t size, int port, int flag, off_t offset, void **ptr)
{
    if (path == NULL || ptr == NULL || offset < 0) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    int fd = open(path, O_RDWR);
    if (fd <= 0) {
        LOG_INFO("open %s failed: %s", path, strerror(errno));
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        LOG_INFO("get state of %s failed: %s", path, strerror(errno));
        goto error_return;
    }
    LOG_DEBUG("file size: %ld", st.st_size);

 
    size_t map_size = 0;
    if (size == -1) {
        map_size = st.st_size;
    } else if (size < -1 || size > st.st_size || size + offset > st.st_size || size == 0) {
        LOG_INFO("invalid map size");
        goto error_return;
    } else if (size > 0) {
        map_size = size;
    }
    
    void *map_ptr = mmap(NULL, map_size, port, flag, fd, offset);
    if (map_ptr == MAP_FAILED) {
        LOG_INFO("map %s failed: %s", path, strerror(errno));
        goto error_return;
    }

    LOG_DEBUG("map %s success: %p", path, map_ptr);
    *ptr = map_ptr;
    if (fd > 0) {
        close(fd);
    }
    return map_size;

error_return:
    if (fd > 0) {
        close(fd);
    }
    return -1;
}


void str2hex_print(void *ptr, size_t bytes)
{
    if (ptr == NULL || bytes <= 0) {
        LOG_INFO("invalid parameter");
        return;
    }

    // format: value ==> <%02x><space><%02x><space>...
    char *print_buf = malloc(bytes * 3 + 1);
    if (print_buf == NULL) {
        LOG_INFO("malloc failed!");
        return;
    }

    char *temp = (char *)ptr;
    for (int i = 0; i < bytes; i++) {
        sprintf(print_buf + strlen((const char *)print_buf), "%02x ", temp[i]);
        /*if (i == 0)
            sprintf(print_buf, "%02x", temp[i]);
        else
            sprintf(print_buf, "%s %02x", print_buf, temp[i]);*/
    }
    LOG_INFO("hex value: %s", print_buf);

    if (print_buf != NULL) {
        free(print_buf);
    }
}

/**
 * @brief excute shell cmd and save response
 * 
 * @return retrun length of response, or negative number on failure.
*/
int shell_cmd_excute(const char *cmd, char *result, int len)
{
    FILE *fp = NULL;
    if (!cmd || !result || len > MAX_SHELL_RESULT_LEN) {
        LOG_ERR("Invalid paramter!");
        return -1;
    }

    LOG_DEBUG("Excute cmd \"%s\"", cmd);
    fp = popen(cmd, "r");
    if (!fp) {
        LOG_ERR("Excute cmd \"%s\" failed: %s", cmd, strerror(errno));
        return -1;
    }

    while (fgets(result + strlen(result), len - strlen(result), fp) != NULL) {
        // 循环读取直到缓冲区满或无更多数据
    }

    LOG_INFO("Excute cmd \"%s\" with result: %s", cmd, result);
    pclose(fp);
    return strlen(result);
}

static int file_io_common(const char *path, int oprt_flag, void *buf, size_t len)
{
    int fd = 0, ret = 0;
    if (!path || !buf || len > MAX_FILE_LINE_LENTH) {
        LOG_ERR("Invalid paramter!");
        return -1;
    }

    switch (oprt_flag)
    {
        case FILE_READ_FLAG: {
            /* code */
            fd = open(path, O_RDONLY);
            if (fd < 0) {
                LOG_ERR("Open file failed: %s", strerror(errno));
                return -1;
            }
            ret = read(fd, buf, len);
            if (ret < 0) {
                LOG_ERR("read file failed: %s", strerror(errno));
                goto file_close;
            }
            break;
        }
        case FILE_WRITE_FLAG: {
            /* code */
            fd = open(path, O_WRONLY);
            if (fd < 0) {
                LOG_ERR("Open file failed: %s", strerror(errno));
                return -1;
            }

            ret = write(fd, buf, len);
            if (ret < 0) {
                LOG_ERR("write file failed: %s", strerror(errno));
                goto file_close;
            }
            break;
        }
        default:
            break;
    }

file_close:
    close(fd);
    return ret;
}

int read_file_string(const char *path, char *buf, size_t len)
{
    int ret = file_io_common(path, FILE_READ_FLAG, buf, len);
    LOG_DEBUG("Read string \"%s\" from %s, ret =%d", buf, path, ret);
    return ret;
}

int write_file_string(const char *path, char *buf, size_t len)
{
    int ret = file_io_common(path, FILE_WRITE_FLAG, buf, len);
    LOG_DEBUG("Write string \"%s\" into %s, ret =%d", buf, path, ret);
    return ret;
}

int fill_random_value(int nums[], int max, int min, int size)
{
    int i = 0;

    if (!nums || size <= 0) {
        LOG_DEBUG("Invalid parameter!");
        return -1;
    }

    srand(time(NULL));
    for (i = 0; i < size; i++) {
        nums[i] = rand() % (max - min + 1) + min;
    }
    return size;
}

void print_int_array(int num[], int size)
{
    printf("array: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", num[i]);
    }
    printf("\n");
}