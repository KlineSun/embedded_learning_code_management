#include <stdio.h>
#include "common_util.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


#define HELLO_DRV_PATH "/dev/hello_drv"
#define MAX_RECV_BUF_SIZE (512)

typedef enum {
    APP_PATH_IDX=0,
    DEV_PATH_IDX,
    OPERATION_IDX,
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

int g_target_fd = -1;

int main(int argc, const char **argv)
{
    LOG_DEBUG("Enter main: %d", argc);

    int oprt = 0, err = 0;
    char recv_buf[MAX_RECV_BUF_SIZE] = {0};
    
    /**
     * exp: 
     *  ./common_drv_tst /dev/hello_drv -w www.baidu.com
     *  ./common_drv_tst /dev/hello_drv -r
    */
    if (argc < 3) {
        LOG_DEBUG("Usage: ./common_drv_tst <dev_path> <operation> [input_data]");
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

    int g_target_fd = open(HELLO_DRV_PATH, O_RDWR);
    if (g_target_fd < 0) {
        LOG_DEBUG("open dev %s failed: %s", HELLO_DRV_PATH, strerror(errno));
        err = OPTR_OPEN_ERR;
        goto res_free;
    }

    if (oprt == OPERATION_WRITE) {
        if (write(g_target_fd, argv[INPUT_DATA_IDX], strlen(argv[INPUT_DATA_IDX])) < 0) {
            LOG_DEBUG("write data to dev %s failed: %s", HELLO_DRV_PATH, strerror(errno));
            err = OPTR_WRITE_ERR;
            goto res_free;
        }
        LOG_DEBUG("write data to device success!");
    } else if (oprt == OPERATION_READ) {
        if (read(g_target_fd, recv_buf, MAX_RECV_BUF_SIZE) < 0) {
            LOG_DEBUG("read data from dev %s failed: %s", HELLO_DRV_PATH, strerror(errno));
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
