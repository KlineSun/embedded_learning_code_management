#include <stdio.h>
#include "common_util.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_RECV_BUF_SIZE (512)

typedef enum {
    APP_PATH_IDX=0,
    DEV_PATH_IDX,
    OPERATION_IDX,
    DATA_SIZE_IDX,
    DATA_TYPE_IDX,
    INPUT_DATA_IDX,
} option_index_t;

typedef enum {
    OPERATION_UNKNOWN = -1,
    OPERATION_WRITE = 0,
    OPERATION_READ,
    OPERATION_IOCTRL,
    OPERATION_POLL
} file_optr_t;

typedef enum {
    OPTR_UNKNOWN=-7,
    OPTR_INIT_ERR,
    OPTR_IOCTRL_ERR,
    OPTR_POLL_ERR,
    OPTR_WRITE_ERR,
    OPTR_READ_ERR,
    OPTR_OPEN_ERR,
    OPTR_NO_ERR = 0,
} file_optr_error_t;

typedef enum {
    INPUT_STRING_TYPE,
    INPUT_INT_TYPE,
    INPUT_FLOAT_TYPE,
} input_type_t;

int g_target_fd = -1;

int main(int argc, const char **argv)
{
    LOG_DEBUG("Enter main: %d", argc);

    int oprt = 0, err = 0, data_size = 0, input_type = 0, write_size = 0;
    char recv_buf[MAX_RECV_BUF_SIZE] = {0};
    char input_str[MAX_RECV_BUF_SIZE] = {0};
    int input_int[MAX_RECV_BUF_SIZE] = {0};
    float input_float[MAX_RECV_BUF_SIZE] = {0};
    void *write_buf = NULL;
    
    /**
     * exp: 
     *  ./common_drv_tst /dev/hello_drv -w 13 -str www.baidu.com
     *  ./common_drv_tst /dev/hello_drv -w 2 -int 1 2
     *  ./common_drv_tst /dev/hello_drv -w 2 -float 1.1 2.2
     *  ./common_drv_tst /dev/hello_drv -r
     * 
     *  ./common_drv_tst /dev/led_simple_drv -w 1 -int 1
    */
    if (argc < 3) {
        LOG_DEBUG("Usage: ./common_drv_tst <dev_path> <operation> <data_size> <data_type> [input_data]");
        LOG_DEBUG("operation:\n-w: write\n-r: read\n-ioctrl: ioctrl\n-p: poll");
        return OPTR_INIT_ERR;
    }

    if (access(argv[DEV_PATH_IDX], F_OK) != 0) {
        LOG_DEBUG("device is not exist: %s", argv[DEV_PATH_IDX]);
        return OPTR_INIT_ERR;
    }

    if (!strcmp(argv[OPERATION_IDX], "-w") && argc >= 4) {
        oprt = OPERATION_WRITE;
    } else if (!strcmp(argv[OPERATION_IDX], "-r")) {
        oprt = OPERATION_READ;
    } else if (!strcmp(argv[OPERATION_IDX], "-ioctrl")) {
        oprt = OPERATION_IOCTRL;
    } else if (!strcmp(argv[OPERATION_IDX], "-p")) {
        oprt = OPERATION_POLL;
    } else {
        LOG_DEBUG("unsupport operation: %s", argv[OPERATION_IDX]);
        return OPTR_UNKNOWN;
    }

    if (oprt == OPERATION_WRITE || oprt == OPERATION_IOCTRL) {
        data_size = atoi(argv[DATA_SIZE_IDX]);
        if (data_size <= 0 || data_size > MAX_RECV_BUF_SIZE) {
            LOG_DEBUG("invalid data_size: %s", argv[DATA_SIZE_IDX]);
            return OPTR_UNKNOWN;
        }

        if (!strcmp(argv[DATA_TYPE_IDX], "-int")) {
            input_type = INPUT_INT_TYPE;
            write_buf = input_int;
            write_size = data_size * sizeof(int);
        } else if (!strcmp(argv[DATA_TYPE_IDX], "-str")) {
            input_type = INPUT_STRING_TYPE;
            write_buf = input_str;
            write_size = data_size * sizeof(char);
        } else if (!strcmp(argv[DATA_TYPE_IDX], "-float")) {
            input_type = INPUT_FLOAT_TYPE;
            write_buf = input_float;
            write_size = data_size * sizeof(float);
        } else {
            LOG_DEBUG("unsupport data type: %s", argv[DATA_TYPE_IDX]);
            return OPTR_UNKNOWN;
        }
    
        for (int i = 0; i < data_size; i++) {
            if (input_type == INPUT_INT_TYPE) {
                input_int[i] = atoi(argv[INPUT_DATA_IDX + i]);
                LOG_DEBUG("get int: %d", input_int[i]);
            } else if (input_type == INPUT_FLOAT_TYPE) {
                input_float[i] = atof(argv[INPUT_DATA_IDX + i]);
                LOG_DEBUG("get float: %f", input_float[i]);
            } else if (input_type == INPUT_STRING_TYPE) {
                input_str[i] = argv[INPUT_DATA_IDX][i];
            } else {
                LOG_DEBUG("unsupport data type: %s", argv[DATA_TYPE_IDX]);
                return OPTR_UNKNOWN;
            }
        }

        if (input_type == INPUT_STRING_TYPE) {
            LOG_DEBUG("get string: %s", input_str);
        }
    }


    int g_target_fd = open(argv[DEV_PATH_IDX], O_RDWR);
    if (g_target_fd < 0) {
        LOG_DEBUG("open dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
        err = OPTR_OPEN_ERR;
        goto res_free;
    }

    if (oprt == OPERATION_WRITE) {
        if (write(g_target_fd, write_buf, write_size) < 0) {
            LOG_DEBUG("write data to dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
            err = OPTR_WRITE_ERR;
            goto res_free;
        }
        LOG_DEBUG("write data to device success!");
    } else if (oprt == OPERATION_READ) {
        if (read(g_target_fd, recv_buf, MAX_RECV_BUF_SIZE) < 0) {
            LOG_DEBUG("read data from dev %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
            err = OPTR_READ_ERR;
            goto res_free;
        }
        LOG_DEBUG("read data from driver success: %s", recv_buf);
    } else if (oprt == OPERATION_IOCTRL) {

    } else if (oprt == OPERATION_POLL) {

    } else {
        LOG_DEBUG("Unknown operation: %d", oprt);
        err = OPTR_UNKNOWN;
        goto res_free;

    }

res_free:
    if (g_target_fd >= 0) {
        close(g_target_fd);
        g_target_fd = -1;
    }
    return err;
}
