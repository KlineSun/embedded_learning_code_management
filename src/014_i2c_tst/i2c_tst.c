#include <stdio.h>
#include "common_util.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "i2c_tst.h"
#include "smbus.h"
#include "i2cbusses.h"
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <math.h>

/**
 * usage: i2c_tst <i2c_controller_num> <dev_addr> <daddr> <direction> <mode> <byte_cnt> [input_dataue/string]
 * write fromat:
 *      i2c_tst <num> <addr> <daddr> <W> quick n <flag 1> <flag 2> ... <flag n>
 *      i2c_tst <num> <addr> <daddr> <W> byte n <byte 1> <byte 2> ... <byte n>
 *      i2c_tst <num> <addr> <daddr> <W> byte_data n <byte 1> <byte 2> ... <byte n>
 *      i2c_tst <num> <addr> <daddr> <W> word 2n <word 1 low> <word 1 high> ... <word n low> <word n high>
 *      i2c_tst <num> <addr> <daddr> <W> block n <byte 1> <byte 2> ... <byte n>
 *      or
 *      i2c_tst <num> <addr> <daddr> <W> block strlen(str) <str@abcd...>
 * read format:
 *      i2c_tst <num> <addr> <daddr> <R> byte n
 *      i2c_tst <num> <addr> <daddr> <R> byte_data n
 *      i2c_tst <num> <addr> <daddr> <R> word n
 *      i2c_tst <num> <addr> <daddr> <R> block n
 *      or
 *      i2c_tst <num> <addr> <daddr> <R> block n str@
*/
int main(int argc, const char **argv)
{
    LOG_INFO("Enter main: %d", argc);
    /**
     * 0: write
     * 1: read
    */
    char filename[MAX_I2C_DEV_PATH_LEN] = {0};
    int i2c_fd = -1, i2c_num = 0, i2c_clnt_addr = 0, daddr = 0, mode = -1, byte_cnt = 0, ret = -1;
    char input_data[MAX_I2C_DATA_LEN] = {0}, output_data[MAX_I2C_DATA_LEN] = {0}, direction = 0;
    char *end = NULL;
    bool str_transfer = false;

    if (argc < 5) {
        LOG_INFO("Too few parameter!");
        /**
         * hex write: ./i2c_tst 1 0x50 0 w byte_data 4 0x09 0x0a 0x0b 0xc
         * hex read:  ./i2c_tst 1 0x50 0 r byte_data 4
         * 
         * byte write: ./i2c_tst 0 0x1e 0    w byte 1 0x04
         * byte write: ./i2c_tst 0 0x1e 0    w byte 1 0x03
         * word read:  ./i2c_tst 0 0x1e 0x0c r word 2
         * word read:  ./i2c_tst 0 0x1e 0x0e r word 2
         * 
         * block write: ./i2c_tst 1 0x50 0 w block 6 0x09 0x0a 0x0b 0xc 0xd 0xe
         * block read:  ./i2c_tst 1 0x50 0 r block 6
         * 
         * string write: ./i2c_tst 1 0x50 0 w block 14 str@www.baidu.com
         * string write: ./i2c_tst 1 0x50 0 r block 14 str@
         * 
         * byte_data write: ./i2c_tst 1 0x50 0 w byte_data 13 0x77 0x77 0x77 0x2e 0x62 0x61 0x69 0x64 0x75 0x2e 0x63 0x6f 0x6d
         * byte_data write: ./i2c_tst 1 0x50 0 r byte_data 14 str@
        */
        LOG_INFO("usage:\r\nusage: i2c_tst <i2c_controller_num> <dev_addr> <direction> <mode> <byte_cnt> [input_dataue/string]");
        LOG_INFO("string format prefix: str@<n bytes>");
        return EXCUTE_FAILED_EXIT;
    }

    i2c_num = lookup_i2c_bus(argv[I2C_NUM_IDX]);
    if (i2c_num < 0) {
        LOG_INFO("parse i2c_num failed!");
        return EXCUTE_FAILED_EXIT;
    }

    i2c_clnt_addr = parse_i2c_address(argv[I2C_ADDR_IDX], 0);
    if (i2c_clnt_addr < 0) {
        LOG_INFO("parse address failed!");
        return EXCUTE_FAILED_EXIT;
    }

    daddr = strtol(argv[I2C_DADDR_IDX], &end, 0);
    if (daddr < 0 || *end != NULL) {
        LOG_INFO("parse daddress failed!");
        return EXCUTE_FAILED_EXIT;
    }

    if (argv[I2C_DIRECTION_IDX][0] == 'w' || argv[I2C_DIRECTION_IDX][0] == 'W') {
        direction = 0;
    } else if (argv[I2C_DIRECTION_IDX][0] == 'r' || argv[I2C_DIRECTION_IDX][0] == 'R') {
        direction = 1;
    } else {
        LOG_INFO("parse direction failed!");
        return EXCUTE_FAILED_EXIT;
    }

    // mode
    if (!strcmp(argv[I2C_MODE_IDX], "quick")) {
        mode = I2C_SMBUS_QUICK;
    } else if (!strcmp(argv[I2C_MODE_IDX], "byte")) {
        mode = I2C_SMBUS_BYTE;
    } else if (!strcmp(argv[I2C_MODE_IDX], "byte_data")) {
        mode = I2C_SMBUS_BYTE_DATA;
    } else if (!strcmp(argv[I2C_MODE_IDX], "word")) {
        mode = I2C_SMBUS_WORD_DATA;
    } else if (!strcmp(argv[I2C_MODE_IDX], "block")) {
        mode = I2C_SMBUS_BLOCK_DATA;
    } else {
        LOG_INFO("Unsupport transfer mode: %s", argv[I2C_MODE_IDX]);
        return EXCUTE_FAILED_EXIT;
    }

    byte_cnt = atoi(argv[I2C_BYTE_COUNT_IDX]);
    if (byte_cnt <= 0 || byte_cnt >= MAX_I2C_DATA_LEN) {
        LOG_INFO("parse byte count failed: %d", byte_cnt);
        return EXCUTE_FAILED_EXIT;
    }
    LOG_INFO("get parameter:\r\ni2c_num: %d\r\nclient_address: 0x%x\r\ndaddr:0x%x\r\ndirection: %c\r\nmode:%d\r\nbyte count: %d",
        i2c_num,
        i2c_clnt_addr,
        daddr,
        direction ? 'R' : 'W',
        mode,
        byte_cnt
    );

    long b;
    if (!direction) {
        if (!strncmp(argv[I2C_DATA_IDX], "0x", 2)) {
            // parse hex val
            for (int i = 0; i < byte_cnt; i++) {
                b = strtol(argv[i + I2C_DATA_IDX], &end, 0);
                if (b > 0xff || *end != NULL) {
                    LOG_INFO("convert hex value faild: %s", argv[i + I2C_DATA_IDX]);
                    return EXCUTE_FAILED_EXIT;
                }

                input_data[i] = (char)b;
                LOG_INFO("Get hex: 0x%02x", input_data[i]);
            }
        } else if (!strncmp(argv[I2C_DATA_IDX], "str@", 4)) {
            if (strlen(argv[I2C_DATA_IDX]) >= MAX_I2C_DATA_LEN + 4) {
                LOG_INFO("String data is too long!");
                return EXCUTE_FAILED_EXIT;
            }
            // parse string
            strcpy(input_data, argv[I2C_DATA_IDX] + 4);
            LOG_INFO("Get string: %s", input_data);
            str_transfer = true;
        } else {
            LOG_INFO("Usupport input format: %s", argv[I2C_DATA_IDX]);
            return EXCUTE_FAILED_EXIT;
        }
    } else {
        if (argc == 8 && !strcmp(argv[I2C_DATA_IDX], "str@")) {
            str_transfer = true;
        }
    }

    i2c_fd = open_i2c_dev(i2c_num, filename, MAX_I2C_DEV_PATH_LEN, 0);
    if (i2c_fd < 0) {
        LOG_INFO("Open i2c devices failed: %s", strerror(errno));
        return EXCUTE_FAILED_EXIT;
    }
    LOG_INFO("Open i2c devices success: path=%s, fd=%d", filename, i2c_fd);

    // bind clinet i2c device
    if (set_slave_addr(i2c_fd, i2c_clnt_addr, 1)) {
        LOG_INFO("Bind i2c client devices failed!");
        goto res_free;
    }

    if (direction) {
        // read
        switch (mode)
        {
        case I2C_SMBUS_BYTE: {
            for (int i = 0; i < byte_cnt; i++) {
                ret = i2c_smbus_read_byte(i2c_fd);
                if (ret < 0) {
                    LOG_INFO("Read data from i2c failed!");
                    goto res_free;
                }
                usleep(10 * 1000);
                output_data[i] = ret &0x0ff;
            }
            break;
        }
        case I2C_SMBUS_BYTE_DATA: {
            for (int i = 0; i < byte_cnt; i++, daddr++) {
                ret = i2c_smbus_read_byte_data(i2c_fd, daddr);
                if (ret < 0) {
                    LOG_INFO("Read data from i2c failed!");
                    goto res_free;
                }
                usleep(10 * 1000);
                output_data[i] = ret &0x0ff;
            }
            break;
        }
        case I2C_SMBUS_WORD_DATA: {
            if (byte_cnt % 2 != 0) {
                LOG_INFO("invalid byte count!");
                goto res_free;
            }

            for (int i = 0; i < byte_cnt / 2; i++, daddr++) {
                ret = i2c_smbus_read_word_data(i2c_fd, daddr);
                if (ret < 0) {
                    LOG_INFO("Read data from i2c failed!");
                    goto res_free;
                }
                usleep(10 * 1000);
                output_data[i] = ret &0x0ff;
                output_data[i + 1] = (ret >> 8) & 0x0ff;
            }
            break;
        }
        case I2C_SMBUS_BLOCK_DATA:  
            ret = i2c_smbus_read_i2c_block_data(i2c_fd, daddr, MAX_I2C_DATA_LEN, output_data);
            output_data[MAX_I2C_DATA_LEN - 1] = '\0';
            break;
        default:
            break;
        }
        if (ret < 0) {
            LOG_INFO("Read data from i2c failed!");
            goto res_free;
        }

        // print result
        char print_buf[MAX_FILE_LINE_LENTH] = {0};
        if (str_transfer) {
            LOG_INFO("Read string from i2c: %s", output_data);
        } else {
            for (int i = 0; i < byte_cnt; i++) {
                sprintf(print_buf + strlen(print_buf), "0x%x ", output_data[i]);
            }
            LOG_INFO("Read hex from i2c: %s", print_buf);
        }
    } else {
        // write
        switch (mode)
        {
        case I2C_SMBUS_QUICK: {
            for (int i = 0; i < byte_cnt; i++) {
                if (i2c_smbus_write_quick(i2c_fd, input_data[i] == 0 ? I2C_SMBUS_WRITE : I2C_SMBUS_READ) != 0) {
                    LOG_INFO("Write data to i2c failed!");
                    goto res_free;
                }
                usleep(10 * 1000);
            }
            break;
        }
        case I2C_SMBUS_BYTE: {
            for (int i = 0; i < byte_cnt; i++) {
                ret = i2c_smbus_write_byte(i2c_fd, input_data[i]);
                if (ret != 0) {
                    LOG_INFO("Write data to i2c failed: %s", strerror(errno));
                    goto res_free;
                }
                usleep(10 * 1000);
            }
            break;
        }
        case I2C_SMBUS_BYTE_DATA: {
            for (int i = 0; i < byte_cnt; i++, daddr++) {
                if (i2c_smbus_write_byte_data(i2c_fd, daddr, input_data[i]) != 0) {
                    LOG_INFO("Write data to i2c failed: %s", strerror(errno));
                    goto res_free;
                }

                LOG_INFO("byte 0x%x wrote!", input_data[i]);
                usleep(10 * 1000);
            }
            break;
        }
        case I2C_SMBUS_WORD_DATA: {
            if (byte_cnt % 2 != 0) {
                LOG_INFO("invalid byte count!");
                goto res_free;
            }
            for (int i = 0; i < byte_cnt / 2; i++, daddr++) {
                unsigned short val = input_data[i + 1] << 8 | input_data[i];
                if (i2c_smbus_write_word_data(i2c_fd, daddr, val)) {
                    LOG_INFO("Write data to i2c failed!");
                    goto res_free;
                }
                usleep(10 * 1000);
            }
            break;
        }
        case I2C_SMBUS_BLOCK_DATA: {
            // enter internally timed for eeprom
            if (i2c_clnt_addr == EEPROM_DEVICE_ADDRESS) {
                // write zero for 
                i2c_smbus_write_word_data(i2c_fd, 0, 0);
                i2c_smbus_write_i2c_block_data(i2c_fd, daddr, 1, &(input_data[0]));

                usleep(20 * 1000);

                if (i2c_smbus_write_i2c_block_data(i2c_fd, daddr, byte_cnt - 1, &(input_data[1]))) {
                    LOG_INFO("Write data to i2c failed!");
                    goto res_free;
                }
            } else if (i2c_smbus_write_i2c_block_data(i2c_fd, daddr, byte_cnt, input_data[1])) {
                LOG_INFO("Write data to i2c failed!");
                goto res_free;
            }

            break;
        }
        default:
            break;
        }
    }

res_free:
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
    return EXCUTE_SUCCESS_EXIT;
}
