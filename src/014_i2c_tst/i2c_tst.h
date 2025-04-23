#ifndef I2C_TST_H
#define I2C_TST_H

#define MAX_I2C_DEV_PATH_LEN (32)
// usage: i2c_tst <i2c_controller_num> <dev_addr> <daddr> <direction> <mode> <byte_cnt> [hex_value/string]
#define I2C_NUM_IDX           (1)
#define I2C_ADDR_IDX          (2)
#define I2C_DADDR_IDX         (3)
#define I2C_DIRECTION_IDX     (4)
#define I2C_MODE_IDX          (5)
#define I2C_BYTE_COUNT_IDX    (6)
#define I2C_DATA_IDX          (7)

#define MAX_I2C_DATA_LEN      (32)

#define EEPROM_DEVICE_ADDRESS (0x50)

#endif // I2C_TST_H