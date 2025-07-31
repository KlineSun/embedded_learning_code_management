#include <stdio.h>
#include "common_util.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>

#define MAX_RECV_BUF_SIZE (1024)
#define MIN(a,b) (a < b ? a : b)
#define DEFAULT_WAIT_TIME_MS (5000)
typedef enum {
    APP_PATH_IDX=0,
    DEV_PATH_IDX,
    OPERATION_IDX,
    DATA_SIZE_IDX,
    DATA_FORMAT_IDX,
    INPUT_DATA_IDX,
} option_index_t;

typedef enum {
    OPERATION_UNKNOWN = -1,
    OPERATION_WRITE = 0,
    OPERATION_READ,
    OPERATION_IOCTRL,
    OPERATION_POLL,
    OPERATION_FASYNC,
    OPERATION_MMAP,
} file_optr_t;

typedef enum {
    OPTR_NO_ERR = 0,
    OPTR_INIT_ERR,
    OPTR_OPEN_ERR,
    OPTR_READ_ERR,
    OPTR_WRITE_ERR,
    OPTR_IOCTRL_ERR,
    OPTR_POLL_ERR,
    OPTR_SIGNAL_ERR,
    OPTR_FASYNC_ERR,
    OPTR_MMAP_ERR,
    OPTR_UNKNOWN,
} file_optr_error_t;

typedef enum {
    INPUT_STRING_TYPE,
    INPUT_INT_TYPE,
    INPUT_FLOAT_TYPE,
} input_type_t;

typedef union {
    char c;
    int i;
    float f;
    long l;
    double d;
} single_data_type;


int check_data_format(const char *fmt)
{
    if (!fmt) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    if (!strcmp(fmt, "%s")) {
         return sizeof(char);
    } else if (!strcmp(fmt, "%d")) {
        return sizeof(int);
    } else if (!strcmp(fmt, "%ld")) {
        return sizeof(long);
    } else if (!strcmp(fmt, "%f")) {
        return sizeof(float);
    } else if (!strcmp(fmt, "%lf")) {
        return sizeof(double);
    } else if (!strcmp(fmt, "%c")) {
        return sizeof(char);
    } else {
        LOG_INFO("Unsupport data format!");
        return -1;
    }
}

void buf_print(void *buf, const char *fmt, size_t size)
{
    int type_size = 0, i = 0;
    if (!buf || !fmt) {
        LOG_INFO("Invalid parameter!");
        return;
    }

    type_size = check_data_format(fmt);
    if (type_size <= 0) {
        LOG_INFO("invalid data format: %s", fmt);
        return;
    }

    for (i = 0; i < size; i++) {
        if (i == 0) {
            printf("DATA: ");
        } else {
            printf(",");
        }

        if (!strcmp(fmt, "%s")) {
            printf("%s", (char *)buf);
            return;
        } else if (!strcmp(fmt, "%d")) {
            printf("%d", *((int *)buf));
            buf += type_size;
        } else if (!strcmp(fmt, "%ld")) {
            printf("%ld", *((long *)buf));
            buf += type_size;
        } else if (!strcmp(fmt, "%f")) {
            printf("%0.2f", *((float *)buf));
            buf += type_size;
        } else if (!strcmp(fmt, "%lf")) {
            printf("%0.4lf", *((double *)buf));
            buf += type_size;
        } else if (!strcmp(fmt, "%c")) {
            printf("%c", *((char *)buf));
            buf += type_size;
        } else {
            LOG_INFO("Unsupport data format: %s", fmt);
            return;
        }
    }
    printf("\n");
}

int type_cycle_sscanf(const char* src, void *dest, const char* fmt, size_t cycle_time)
{
    // 定义变量，用于存储类型大小、解析的字符数和循环次数
    int type_size = 0, n_parsed = 0, i = 0;
    const char *tmp = src;
    // 判断参数是否有效
    if (!src || !fmt || !dest) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    // 获取数据格式的大小
    type_size = check_data_format(fmt);
    // 判断数据格式是否有效
    if (type_size <= 0 || type_size * cycle_time > MAX_RECV_BUF_SIZE) {
        LOG_INFO("invalid data format: %s", fmt);
        return -1;
    }

    // 循环解析数据
    for (i = 0; i < cycle_time; i++) {
        single_data_type data = {0};
        // 使用sscanf函数解析数据
        if (!strcmp(fmt, "%s")) {
            memcpy(dest, tmp, strlen(tmp));
            LOG_INFO("get string: %s", (char *)dest);
            break;
        } else if (!strcmp(fmt, "%d")) {
            if (i == 0) {
                if (sscanf(tmp, "%d%n", &data.i, &n_parsed) < 1) return -1;
            } else {
                if (sscanf(tmp, ",%d%n", &data.i, &n_parsed) < 1) return -1;
            }
            memcpy(dest + i*type_size, &data.i, type_size);
        } else if (!strcmp(fmt, "%ld")) {
            if (i == 0) {
                if (sscanf(tmp, "%ld%n", &data.l, &n_parsed) < 1) return -1;
            } else {
                if (sscanf(tmp, ",%ld%n", &data.l, &n_parsed) < 1) return -1;
            }
            memcpy(dest + i*type_size, &data.l, type_size);
        } else if (!strcmp(fmt, "%f")) {
            if (i == 0) {
                if (sscanf(tmp, "%f%n", &data.f, &n_parsed) < 1) return -1;
            } else {
                if (sscanf(tmp, ",%f%n", &data.f, &n_parsed) < 1) return -1;
            }
            memcpy(dest + i*type_size, &data.f, type_size);
        } else if (!strcmp(fmt, "%lf")) {
            if (i == 0) {
                if (sscanf(tmp, "%lf%n", &data.d, &n_parsed) < 1) return -1;
            } else {
                if (sscanf(tmp, ",%lf%n", &data.d, &n_parsed) < 1) return -1;
            }
            memcpy(dest + i*type_size, &data.d, type_size);
        } else if (!strcmp(fmt, "%c")) {
            if (i == 0) {
                if (sscanf(tmp, "%c%n", &data.c, &n_parsed) < 1) return -1;
            } else {
                if (sscanf(tmp, ",%c%n", &data.c, &n_parsed) < 1) return -1;
            }
            memcpy(dest + i*type_size, &data.c, type_size);
        } else {
            LOG_INFO("Unsupport data format: %s", fmt);
            return -1;
        }
        tmp += n_parsed;
    }

    buf_print(dest, fmt, cycle_time);
    return 0;
}

volatile int g_target_fd = -1;
static bool fasync_ready = false;
static int fasync_read_cnt = 0;
// static int fasync_read_fd = -1;
static void fasync_sig_handler(int signo)
{
    if (signo == SIGIO) {
        char buf[MAX_RECV_BUF_SIZE] = {0};
        LOG_INFO("Device is ready, start to read device data: fd=%d, len=%d", g_target_fd, fasync_read_cnt);

        if (read(g_target_fd, buf, fasync_read_cnt) < 0) {
            LOG_INFO("Read data from device failed: %s", strerror(errno));
            return;
        }
        LOG_INFO("read data from driver success: %s", buf);
        fasync_ready = true;
    } else if (signo == SIGTERM) {
        LOG_INFO("receive SIGTERM");
    } else {
        LOG_INFO("Unsupport signal: %d", signo);
    }
}

int main(int argc, const char **argv)
{
    LOG_INFO("Enter main: %d", argc);

    int oprt = 0, err = 0;
    int type_size = 0, data_size = 0;
    char data_buf[MAX_RECV_BUF_SIZE] = {0};

    /**
     * exp: 
     *  ./common_drv_tst /dev/hello_drv -w 13 -str www.baidu.com
     *  ./common_drv_tst /dev/hello_drv -w 2 -int 1 2
     *  ./common_drv_tst /dev/hello_drv -w 2 -float 1.1 2.2
     *  ./common_drv_tst /dev/hello_drv -r 2
     * 
     *  ./common_drv_tst /dev/led_simple_drv -w 1 -int 1
    */
    if (argc < 3) {
        LOG_INFO("Usage: ./common_drv_tst <dev_path> <operation> <data_size> <data_format> [input_data]");
        LOG_INFO("operation:\n-w: write\n-r: read\n-ioctrl: ioctrl\n-p: poll, wait time: %dms\n-fa: fasync", DEFAULT_WAIT_TIME_MS);
        printf("-w: write\n-r: read\n-ioctrl: ioctrl\n");
        printf("-p: poll, wait time: %dms\n-fa: fasync\n", DEFAULT_WAIT_TIME_MS);
        printf("-mmap: mmap, usage:\r\n -mmap <data_size> <data_format> [input_data]\r\ninput_data is empty means mmap and read, otherwise means mmap and wirte \n");
        LOG_INFO("data_format:\n%%d: int\n%%f: float\n%%ld: long\n%%lf: double\n%%s: string");
        LOG_INFO("Input_format:\ndata1,data2,data3,...");
        return -OPTR_INIT_ERR;
    }

    if (access(argv[DEV_PATH_IDX], F_OK) != 0) {
        LOG_INFO("device is not exist: %s", argv[DEV_PATH_IDX]);
        return -OPTR_INIT_ERR;
    }

    if (!strcmp(argv[OPERATION_IDX], "-w") && argc >= 4) {
        oprt = OPERATION_WRITE;
    } else if (!strcmp(argv[OPERATION_IDX], "-r")) {
        oprt = OPERATION_READ;
    } else if (!strcmp(argv[OPERATION_IDX], "-ioctrl")) {
        oprt = OPERATION_IOCTRL;
    } else if (!strcmp(argv[OPERATION_IDX], "-p")) {
        oprt = OPERATION_POLL;
    } else if (!strcmp(argv[OPERATION_IDX], "-fa")) {
        oprt = OPERATION_FASYNC;
    } else if (!strcmp(argv[OPERATION_IDX], "-mmap")) {
        oprt = OPERATION_MMAP;
    } else {
        LOG_INFO("unsupport operation: %s", argv[OPERATION_IDX]);
        return OPTR_INIT_ERR;
    }

    // get data size
    data_size = atoi(argv[DATA_SIZE_IDX]);
    if (data_size <= 0 || data_size > MAX_RECV_BUF_SIZE) {
        LOG_INFO("invalid data_size: %s", argv[DATA_SIZE_IDX]);
        return OPTR_INIT_ERR;
    }

    // check and calculate format size
    type_size = check_data_format(argv[DATA_FORMAT_IDX]);
    if (type_size <= 0 || type_size * data_size > MAX_RECV_BUF_SIZE) {
        LOG_INFO("invalid data format: %s", argv[DATA_FORMAT_IDX]);
        return OPTR_INIT_ERR;
    }

    LOG_INFO("format=%s, type_size=%d, data_size=%d", argv[DATA_FORMAT_IDX], type_size, data_size);

    // get input data from cmdline
    if (oprt == OPERATION_WRITE || oprt == OPERATION_IOCTRL
        || (oprt == OPERATION_MMAP && argc > INPUT_DATA_IDX)) {
        if (type_cycle_sscanf(argv[INPUT_DATA_IDX], data_buf, argv[DATA_FORMAT_IDX], data_size)) {
            LOG_INFO("sscanf input string failed: %s", argv[DATA_FORMAT_IDX]);
            return OPTR_INIT_ERR;
        }
    }

    // open
    g_target_fd = open(argv[DEV_PATH_IDX], O_RDWR);
    if (g_target_fd < 0) {
        LOG_INFO("open dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
        err = OPTR_OPEN_ERR;
        goto res_free;
    }

    // operate
    if (oprt == OPERATION_WRITE) {
        if (write(g_target_fd, data_buf, type_size * data_size) < 0) {
            LOG_INFO("write data to dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
            err = OPTR_WRITE_ERR;
            goto res_free;
        }
        LOG_INFO("write data to device success!");
    } else if (oprt == OPERATION_READ) {
        if (read(g_target_fd, data_buf, MIN(MAX_RECV_BUF_SIZE, data_size * type_size)) < 0) {
            LOG_INFO("read data from dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
            err = OPTR_READ_ERR;
            goto res_free;
        }
        buf_print(data_buf, argv[DATA_FORMAT_IDX], data_size);
    } else if (oprt == OPERATION_IOCTRL) {

    } else if (oprt == OPERATION_POLL) {
        struct pollfd pfd;
        pfd.fd = g_target_fd;
        pfd.events = POLLIN | POLLRDNORM;
        if (poll(&pfd, 1, DEFAULT_WAIT_TIME_MS) <= 0) {
            LOG_INFO("poll dev %s failed!", argv[DEV_PATH_IDX]);
            err = OPTR_POLL_ERR;
            goto res_free;
        }

        if (read(g_target_fd, data_buf, MIN(MAX_RECV_BUF_SIZE, data_size * type_size)) < 0) {
            LOG_INFO("read data from dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
            err = OPTR_READ_ERR;
            goto res_free;
        }
        buf_print(data_buf, argv[DATA_FORMAT_IDX], data_size);
    } else if (oprt == OPERATION_FASYNC) {
        int flags;
        struct sigaction act;
        act.sa_handler = fasync_sig_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;

        if (sigaction(SIGIO, &act, NULL) < 0 || sigaction(SIGTERM, &act, NULL) < 0) {
            LOG_INFO("Call sigaction failed!");
            err = OPTR_SIGNAL_ERR;
            goto res_free;
        }

        fcntl(g_target_fd, F_SETOWN, getpid());
        flags = fcntl(g_target_fd, F_GETFL); 
        fcntl(g_target_fd, F_SETFL, flags | FASYNC);
        fasync_read_cnt = MIN(MAX_RECV_BUF_SIZE, data_size * type_size);
        for (;;) {
            if (fasync_ready) {
                LOG_INFO("fasync msg is handled");
                fasync_ready = false;
            }

            LOG_INFO("Doing something in main, fd=%d", g_target_fd);
            sleep(2);
        }
    } else if (oprt == OPERATION_MMAP) {
        char *mmap_buf = (char *)mmap(NULL, MAX_RECV_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_target_fd, 0);
        if (mmap_buf == MAP_FAILED) {
            LOG_INFO("mmap failed: %s", strerror(errno));
            err = OPTR_MMAP_ERR;
            goto res_free;
        }

        LOG_INFO("mmap kernel buf at address: %p", mmap_buf);
        if (!strcmp(data_buf, "")) {
            char recv_buf[MAX_RECV_BUF_SIZE] = {0};
            // strncpy(recv_buf, mmap_buf, data_size);
            memcpy(recv_buf, mmap_buf, data_size);
            LOG_INFO("read data from mmap buf: %s", recv_buf);
        } else {
            // strncpy(mmap_buf, data_buf, data_size);
            memcpy(mmap_buf, data_buf, data_size);
            LOG_INFO("write data to mmap buf: %s", data_buf);
        }

        for (;;) {
            sleep(2);
        }
        munmap(mmap_buf, MAX_RECV_BUF_SIZE);
    } else {
        LOG_INFO("Unknown operation: %d", oprt);
        err = OPTR_INIT_ERR;
        goto res_free;
    }

res_free:
    if (g_target_fd >= 0) {
        close(g_target_fd);
        g_target_fd = -1;
    }
    return err;
}
