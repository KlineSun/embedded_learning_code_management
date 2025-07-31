#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include "common_util.h"

#define MAX_SPI_MSG_SIZE (4096)

#define SPI_DAC_RX (0)
#define SPI_DAC_TX (1)
#define SPI_DAC_RDWR (2)
#define SPI_DAC_MSG_DATA_LEN    (2)
#define SPI_DAC_MSG_FLAG_OFFSET (0)
#define SPI_DAC_MSG_CNT_OFFSET  (8)
#define SPI_DAC_MSG(flag, cnt) (((cnt & 0xff) << SPI_DAC_MSG_CNT_OFFSET) | ((flag & 0xff) << SPI_DAC_MSG_FLAG_OFFSET))


typedef enum {
    PROC_SELF=0,
    DEV_PATH_IDX,
    OPRT_IDX,
    IDATA_IDX,
} spi_cmd_idx;

typedef enum {
    CLEAR_OPRT=0,
    WRITE_OPRT,
    READ_OPRT,
    RDWR_OPRT,
    UNKOWN_OPRT=-1,
} spi_oprt_type;


void print_usage()
{
    LOG_INFO("usage: ./dac_spidev_tst <path> <-r%%d | -w%%d | -r%%dw%%d> [val]");
    LOG_INFO("path: path of spi dev.");
    LOG_INFO("-r: read\n-w: write\n-rw: read and write, read count max equal to write count!");
    LOG_INFO("%%d: count of operate number, not more than %d.", MAX_SPI_MSG_SIZE);
    LOG_INFO("val: the input value, request when operate is -w or -rw.");
}

/**
 * 
 * usage:
 *  ./dac_spidev_tst <path> <-r%d | -w%d | -r%dw%d> [val]
 * 
 * path：表示要操作的spi设备的路径；
 * -r、-w，-rw：表示操作类型，后面紧跟操作的数据个数，即要传输多少帧spi数据；
 * val：表示要写入的数据；
 * 
 *  exp:
 *  ./dac_spidev_tst /dev/spidev0.1 -w1 100
 *  ./dac_spidev_tst /dev/spidev0.1 -w3r3 100,200,300
 * 
*/
int main(int argc, const char **argv)
{
    spi_oprt_type oprt_type = UNKOWN_OPRT;
    int tx_cnt = 0, rx_cnt = 0, tx_bits = 0, rx_bits = 0, ret = 0, i = 0;
    uint16_t *tx_buf = NULL, *rx_buf = NULL, *transfer_buf = NULL;
    mode_t mode = 0;
    char *tmp = NULL;
    int fd = 0;
    bool is_success = false;
    int cmd = 0;

    if (argc < OPRT_IDX + 1) {
        LOG_INFO("Arguments is too few, no less than %d.", OPRT_IDX + 1);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    LOG_INFO("Enter main: %d", argc);
    if ((strstr(argv[OPRT_IDX], "-r") || strstr(argv[OPRT_IDX], "-R"))
        && (strstr(argv[OPRT_IDX], "w") || strstr(argv[OPRT_IDX], "W"))) {
        // -rw
        oprt_type = RDWR_OPRT;
        if (sscanf(argv[OPRT_IDX], "-r%dw%d", &rx_cnt, &tx_cnt) < 2) {
            LOG_INFO("Unparseable format: %s", argv[OPRT_IDX]);
            print_usage();
            return EXCUTE_FAILED_EXIT;
        }
        mode = O_RDWR;
    } else if (strstr(argv[OPRT_IDX], "-r") || strstr(argv[OPRT_IDX], "-R")) {
        // -r
        oprt_type = READ_OPRT;
        sscanf(argv[OPRT_IDX], "-r%d", &rx_cnt);
        mode = O_RDONLY;
    } else if (strstr(argv[OPRT_IDX], "-w") || strstr(argv[OPRT_IDX], "-W")) {
        // -w
        oprt_type = WRITE_OPRT;
        sscanf(argv[OPRT_IDX], "-w%d", &tx_cnt);
        mode = O_WRONLY;
    } else {
        LOG_INFO("Unsupport operation: %s", argv[OPRT_IDX]);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    // check args
    if (oprt_type < 0 || rx_bits > MAX_SPI_MSG_SIZE || tx_bits > MAX_SPI_MSG_SIZE) {
        LOG_INFO("Invalid arguments: %s", argv[OPRT_IDX]);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    // alloc memory
    rx_buf  =  calloc(rx_cnt, sizeof(uint16_t));
    tx_buf  =  calloc(tx_cnt, sizeof(uint16_t));
    if ((!rx_buf && rx_cnt) || (!tx_buf && tx_cnt)) {
        LOG_INFO("Alloc read/write buffer memory failed: %s", strerror(errno));
        return EXCUTE_FAILED_EXIT;
    }

    if (oprt_type == WRITE_OPRT || oprt_type == RDWR_OPRT) {
        if (argc < IDATA_IDX + 1){
            LOG_INFO("Need input data!");
            print_usage();
            goto mem_free;
        }
        // parse input data
        i = 0;
        tmp = strtok((char *)argv[IDATA_IDX], ",");
        while (tmp) {
            ret = strtoul(tmp, NULL, 0);
            LOG_INFO("input %d: %d", i, ret);
            // dac设备仅2~11位数据有效，总长度10位 
            if (ret  > 1023) {
                LOG_INFO("Input value of dac device cannot exceed 1023: %d", ret);
                goto mem_free;
            }
            tx_buf[i] = ret;
            LOG_INFO("get data tx_buf[%d]=%d", i, tx_buf[i]);
            i++;
            tmp = strtok(NULL, ",");
        }
    }

    // open
    fd = open(argv[DEV_PATH_IDX], mode);
    if (fd <= 0) {
        LOG_INFO("open %s failed: %s", argv[DEV_PATH_IDX], strerror(errno));
        goto mem_free;
    }

    // operate
    switch (oprt_type)
    {
        case RDWR_OPRT: {
            cmd = SPI_DAC_MSG(SPI_DAC_RDWR, rx_cnt);
            transfer_buf = tx_buf; // 返回数据也用tx_buf一起传输，所以tx和rx的数据一致，上面已经做了判断
            break;
        }
        case READ_OPRT: {
            cmd = SPI_DAC_MSG(SPI_DAC_RX, rx_cnt);
            transfer_buf = rx_buf;
            break;
        }
        case WRITE_OPRT: {
            // data
            cmd = SPI_DAC_MSG(SPI_DAC_TX, rx_cnt);
            transfer_buf = tx_buf;
            break;
        }
        default: {
            LOG_INFO("unsupport oprt: %d", oprt_type);
            goto file_close;
        }
    }

    // transfer
    ret = ioctl(fd, cmd, transfer_buf);
    if (ret < 0) {
        LOG_INFO("ioctl failed: %s", strerror(errno));
        goto file_close;
    }

    printf("get result: ");
    for (i = 0; i < rx_cnt; i++) {
        printf("%hu ", transfer_buf[i]);
    }
    printf("\n");

    LOG_INFO("SPI transfer success!");
    is_success = true;

file_close:
    if (fd > 0) close(fd);

mem_free:
    if (rx_buf) free(rx_buf);
    if (tx_buf) free(tx_buf);
    return is_success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
