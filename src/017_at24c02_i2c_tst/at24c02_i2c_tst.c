#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include "i2cbusses.h"
#include "smbus.h"
#include "common_util.h"

// unit: byte
#define EEPROM_SIZE (256)
#define SMBUS_BLOCK_MAX (32)

typedef enum {
    PROC_SELF=0,
    DEV_PATH_IDX,
    OPTR_IDX,
    OFFSET_IDX,
    IDATA_IDX,
} i2c_cmd_idx;

typedef enum {
    CLEAR_OPTR=0,
    WRITE_OPTR,
    READ_OPTR,
    UNKOWN_OPTR=-1,
} i2c_oprt_type;


int eeprom_write(int fd, const char *buf, int cnt, int offset)
{
    int ret = 0;
    if (fd <= 0 || !buf || cnt <= 0 || cnt > SMBUS_BLOCK_MAX || offset + cnt > EEPROM_SIZE || offset > 0xff) {
        LOG_DEBUG("Invalid paramter!");
        return -1;
    }

    if (cnt == 1 && offset == 0) {
        // byte write
        // i2c_smbus_write_byte
        char cmd = offset, val = buf[0];
        ret = offset ? i2c_smbus_write_byte_data(fd, cmd, val) : i2c_smbus_write_byte(fd, val);
        if (ret < 0) {
            LOG_DEBUG("write byte failed: %d", ret);
            return -1;
        }
        LOG_DEBUG("write single byte at address %d: %c", cmd, val);
    } else if (cnt == 2) {
        // byte write
        // i2c_smbus_write_byte_data
        short val = 0;
        memcpy(&val, buf, 2);
        ret = i2c_smbus_write_word_data(fd, offset, val);
        if (ret < 0) {
            LOG_DEBUG("write word failed: %d", ret);
            return -1;
        }
        LOG_DEBUG("write word at address %d: %c%c", offset, val >> 8, val & 0xff);
    } else if (cnt > 2 && cnt < EEPROM_SIZE) {
        // Page Write
        // i2c_smbus_write_i2c_block_data
        int cmd = offset, j = 0;
        for (; cmd < offset + cnt; cmd++, j++) {
            LOG_DEBUG("cmd=%d, byte: %c", cmd, buf[j]);
            ret = i2c_smbus_write_byte_data(fd, cmd, buf[j]);
            if (ret < 0) {
                LOG_DEBUG("write byte failed: %d", ret);
                return -1;
            }

            // write cycle time
            usleep(1000 * 10);
        }

        //int remain = cnt, page_gap = ;
        LOG_DEBUG("write block at address %d: %s", offset, buf);
    } else {
        LOG_DEBUG("Unsupported situation!");
        return -1;
    }

    return 0;
}

int eeprom_read(int fd, char *buf, int cnt, int offset)
{
    if (fd <= 0 || !buf || cnt <= 0 || cnt > SMBUS_BLOCK_MAX || offset + cnt > EEPROM_SIZE || offset > 0xff) {
        LOG_DEBUG("Invalid paramter!");
        return -1;
    }

    if (cnt == 1 && offset == 0) {
        // current address read / Random Read
        // i2c_smbus_read_byte / i2c_smbus_read_byte_data
        char val = offset ? i2c_smbus_read_byte_data(fd, offset) : i2c_smbus_read_byte(fd);
        LOG_DEBUG("read single byte at address %d: %c", offset, val);
    } else if (cnt == 2) {
        // Random Read
        // i2c_smbus_read_word_data
        short val = i2c_smbus_read_word_data(fd, offset);
        LOG_DEBUG("read word at address %d: %c%c", offset, val >> 8, val & 0xff);
    } else if (cnt > 2 && cnt < EEPROM_SIZE) {
        // Random Read / Sequential Read
        // i2c_smbus_read_i2c_block_data
        int ret = i2c_smbus_read_i2c_block_data(fd, offset, cnt, (__u8 *)buf);
        if (ret != cnt) {
            LOG_DEBUG("Read block failed: %d", ret);
            return -1;
        }
        LOG_DEBUG("read block at address %d: %s", offset, buf);
    } else {
        LOG_DEBUG("Unsupported situation!");
        return -1;
    }

    return 0;
}

/**
 * 
 * usage:
 *  ./at24c02_i2c_tst /dev/i2c-x  <-w%d |-r%d | -c><@%x> <-f%d> [input_data]
 * 
 *  exp:
 *  ./at24c02_i2c_tst /dev/i2c-0  -w2@0x50 -f0 ab
 *  ./at24c02_i2c_tst /dev/i2c-1  -r3@0x50 -f3
 * 
 * byte_count=1：eeprom -> byte write/ current address read / Random Read
 *               i2c-tool -> offset=0: i2c_smbus_write_byte / i2c_smbus_read_byte
 *                           offset!=0: i2c_smbus_write_byte_data / i2c_smbus_read_byte_data
 * byte_count=2：eeprom -> byte write/ Page Write/ Random Read
 *               i2c-tool -> offset=0: i2c_smbus_write_byte_data / i2c_smbus_read_word_data
 *                           offset!=0: i2c_smbus_write_word_data / i2c_smbus_read_word_data
 * byte_count=n：eeprom -> Page Write/ Random Read / Sequential Read
 *               i2c-tool -> offset=0: i2c_smbus_write_i2c_block_data / i2c_smbus_read_i2c_block_data
 *                           offset!=0: i2c_smbus_write_i2c_block_data / i2c_smbus_read_i2c_block_data
 * -c: means clear eeprom
 * 
 * Note: 
 *      - n cannot greather than SMBUS_BLOCK_MAX, the reason is explained in include/uapi/linux/i2c.h. 
*/
int main(int argc, const char **argv)
{
    i2c_oprt_type optr = UNKOWN_OPTR;
    int io_cnt = 0, offset = 0, slave_addr = 0, fd = 0, ret = 0;
    char io_buf[SMBUS_BLOCK_MAX] = {0};
    bool success = false;
    LOG_DEBUG("Enter main: %d", argc);

    if (argc < 3) {
        LOG_DEBUG("usage: ./at24c02_i2c_tst /dev/i2c-x  <-w%%d |-r%%d | -c><@%%x> <-f%%d> [input_data]");
        return EXCUTE_FAILED_EXIT;
    }

    // gain argument from shell
    if (argc == 4 && !strncmp(argv[OPTR_IDX], "-r", strlen("-r"))) {
        LOG_DEBUG("Command: read EEPROM!");
        sscanf(argv[OPTR_IDX], "-r%d@%x", &io_cnt, &slave_addr);
        sscanf(argv[OFFSET_IDX], "-f%d", &offset);
        optr = READ_OPTR;
    } else if (argc == 5 && !strncmp(argv[OPTR_IDX], "-w", strlen("-w"))) {
        LOG_DEBUG("Command: write EEPROM!");
        sscanf(argv[OPTR_IDX], "-w%d@%x", &io_cnt, &slave_addr);
        sscanf(argv[OFFSET_IDX], "-f%d", &offset);
        optr = WRITE_OPTR;
    } else if (argc == 3 && !strncmp(argv[OPTR_IDX], "-c", strlen("-c"))) {
        sscanf(argv[OPTR_IDX], "-c@%x", &slave_addr);
        LOG_DEBUG("Command: clear EEPROM!");
        optr = CLEAR_OPTR;
    } else {
        LOG_DEBUG("Invalid commond!");
        LOG_DEBUG("usage: ./at24c02_i2c_tst /dev/i2c-x  <-w%%d |-r%%d | -c><@%%x> <-f%%d> [input_data]");
        return EXCUTE_FAILED_EXIT;
    }

    // check argument's legality
    if (io_cnt + offset > EEPROM_SIZE || io_cnt < 0 || io_cnt > SMBUS_BLOCK_MAX
        || slave_addr < 0x03 || slave_addr > 0x7f
        || access(argv[DEV_PATH_IDX], F_OK) != 0
        || (optr == WRITE_OPTR && strlen(argv[IDATA_IDX]) > SMBUS_BLOCK_MAX)) {
        LOG_DEBUG("Invalid command paramter!");
        LOG_DEBUG("usage: ./at24c02_i2c_tst /dev/i2c-x  <-w%%d |-r%%d | -c><@%%x> <-f%%d> [input_data]");
        return EXCUTE_FAILED_EXIT;
    }
    LOG_DEBUG("io_cnt=%d, offset=%d", io_cnt, offset);
    LOG_DEBUG("i2c-dev-path=%s, slave_addr=0x%x", argv[DEV_PATH_IDX], slave_addr);

    // open device
    fd = open(argv[DEV_PATH_IDX], O_RDWR);
    if (fd < 0) {
        LOG_DEBUG("open dev failed: %s", strerror(errno));
        return EXCUTE_FAILED_EXIT;
    }

    // set slave address
    ret = set_slave_addr(fd, slave_addr, 1);
    if (ret != 0) {
        LOG_DEBUG("Set slave address failed: %d", ret);
        goto fd_close;
    }

    // switch operations
    switch (optr)
    {
    case CLEAR_OPTR: {
        LOG_DEBUG("Clear eeprom!");
        for (int i = 0; i < EEPROM_SIZE; i++) {
            int ret = i2c_smbus_write_byte_data(fd, i, 0);
            if (ret < 0) {
                LOG_DEBUG("write byte failed: %d", ret);
                goto fd_close;
            }
            // write cycle time
            usleep(1000 * 10);
        }
        break;
    }
    case WRITE_OPTR: {
        LOG_DEBUG("Write eeprom!");
        if (eeprom_write(fd, argv[IDATA_IDX], io_cnt, offset)) {
            LOG_DEBUG("write eeprom failed!");
            goto fd_close;
        }
        break;
    }
    case READ_OPTR: {
        LOG_DEBUG("Read eeprom!");
        if (eeprom_read(fd, io_buf, io_cnt, offset)) {
            LOG_DEBUG("write eeprom failed!");
            goto fd_close;
        }
        break;
    }
    default:
        LOG_DEBUG("UnKown operation: %d", optr);
        break;
    }
    success = true;

fd_close:
    close(fd);
    return success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
