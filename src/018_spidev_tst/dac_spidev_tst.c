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
    uint8_t *tx_buf = NULL;
    uint8_t *rx_buf = NULL;
    mode_t mode = 0;
    struct spi_ioc_transfer *xfers = NULL;
    char *tmp = NULL;
    int fd = 0;
    bool is_success = false;

    if (argc < OPRT_IDX + 1) {
        LOG_INFO("Arguments is too few, no less than %d.", OPRT_IDX + 1);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

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
    // 每次spi传输都收发16位数据，这里要转化成字节数。
    rx_bits = rx_cnt * 2;
    tx_bits = tx_cnt * 2;

    // check args
    if (oprt_type < 0 || rx_bits > MAX_SPI_MSG_SIZE || tx_bits > MAX_SPI_MSG_SIZE) {
        LOG_INFO("Invalid arguments: %s", argv[OPRT_IDX]);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    LOG_INFO("Enter main: %d", argc);
    // alloc memory
    rx_buf  =  calloc(rx_bits, sizeof(uint8_t));
    tx_buf  =  calloc(tx_bits, sizeof(uint8_t));
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
            /* dac设备特殊处理 --begin */
            // dac设备仅2~11位数据有效，总长度10位 
            if (ret  > 1023) {
                LOG_INFO("Input value of dac device cannot exceed 1023: %d", ret);
                goto mem_free;
            }
            ret = (ret << 2) & 0xffc;// 末尾两位固定为0

            // 调整传输时序，先发送MSB，再发送LSB
            tx_buf[2*i] = (ret >> 8) & 0xff; // 取高8位
            tx_buf[2*i+1] = ret & 0xff; // 取低8位
            /* dac设备特殊处理 --end */
            LOG_INFO("After tx_buf[%d]=%d, tx_buf[%d]=%d", 2*i, tx_buf[2*i],  2*i+1, tx_buf[2*i+1]);
            i++;
            tmp = strtok(NULL, ",");
        }
    }
    if (oprt_type == RDWR_OPRT) {
        if (rx_cnt != tx_cnt || !rx_buf || !tx_buf){
            LOG_INFO("rx_cnt must equal to tx_cnt when RDWR_OPRT!");
            print_usage();
            goto mem_free;
        }
        // alloc array of struct spi_ioc_transfer.
        xfers = calloc(tx_cnt, sizeof(struct spi_ioc_transfer));
        if (!xfers) {
            LOG_INFO("Alloc transfer memory failed: %s", strerror(errno));
            return EXCUTE_FAILED_EXIT;
        }

        // asign spi_ioc_transfer
        for (i = 0; i < tx_cnt; i++) {
            xfers[i].rx_buf = (__u64)(uintptr_t)&rx_buf[2*i];  // 等价于 &rx_buf[2*i]
            xfers[i].tx_buf = (__u64)(uintptr_t)&tx_buf[2*i];  // 等价于 &tx_buf[2*i]
            xfers[i].len      = sizeof(uint16_t); // 每次传输16位数据
            xfers[i].tx_nbits = sizeof(*tx_buf);
            xfers[i].rx_nbits = sizeof(*rx_buf);
            xfers[i].word_delay_usecs = 1; // datasheet中片选引脚间隔为20ns;
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
            for (i = 0; i < tx_cnt; i++) {
                ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfers[i]);
                if (ret < 0) {
                    LOG_INFO("ioctl failed: %s", strerror(errno));
                    goto file_close;
                }
                ret = (rx_buf[2*i] << 8) | rx_buf[2*i+1]; // MSB | LSB
                ret >>= 2;
                LOG_INFO("output %d: %d", i, ret);
                sleep(1);
            }
            break;
        }
        case READ_OPRT: {
            ret = read(fd, rx_buf, rx_bits);
            if (ret < 0) {
                LOG_INFO("read spi msg failed: %s", strerror(errno));
                goto file_close;
            }

            LOG_INFO("Get result:");
            for (i = 0; i < rx_cnt; i++) {
                ret = (rx_buf[i] << 8) | rx_buf[i+1]; // MSB | LSB
                ret >>= 2;
                LOG_INFO("output %d: %d", i, ret);
            }
            break;
        }
        case WRITE_OPRT: {
            // data
            ret = write(fd, tx_buf, tx_bits);
            if (ret < 0) {
                LOG_INFO("write spi msg failed: %s", strerror(errno));
                goto file_close;
            }
            break;
        }
        default: {
            LOG_INFO("unsupport oprt: %d", oprt_type);
            goto file_close;
        }
    }
    LOG_INFO("SPI transfer success!");
    is_success = true;

file_close:
    if (fd > 0) close(fd);

mem_free:
    if (xfers) free(xfers);
    if (rx_buf) free(rx_buf);
    if (tx_buf) free(tx_buf);
    return is_success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
