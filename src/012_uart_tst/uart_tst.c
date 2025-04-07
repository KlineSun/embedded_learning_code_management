#include <stdio.h>
#include "common_util.h"
#include "key_value_table.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include "uart_tst.h"
#include <termios.h>


int config_serial_attr(int fd,
                        int data_bits, /* 数据个位 */
                        char verify, /* 校验模式: N(无校验), E(偶校验), O(奇校验) */
                        int stop_bits, /* 停止位个数 */
                        int speed, /* 波特率*/
                        bool is_raw_mode /* 是为原始模式，否则为规范模式 */
                    )
{
    if (fd < 0 || speed <= 0 || speed > 921600 || data_bits < 5 || data_bits > 8) {
        LOG_DEBUG("Invalid parameter!");
        return -1;
    }
    LOG_DEBUG("Terminal setting: %d%c%d %d, %s", data_bits,
                                                verify,
                                                stop_bits,
                                                speed,
                                                is_raw_mode ? "Raw Mode" : "Canonical Mode");

    // get origin data
    struct termios origin_setting, new_setting;
    if (tcgetattr(fd, &origin_setting)) {
        LOG_DEBUG("get struct termios failed!");
        return -1;
    }

    // struct init
    memset(&new_setting, 0, sizeof(struct termios));
    new_setting.c_cflag &= ~CSIZE;
    new_setting.c_cflag |= (CLOCAL | CREAD);

    // set control mode
    if (is_raw_mode) {
        new_setting.c_iflag = 0;
        new_setting.c_oflag = 0;
        new_setting.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    } else {
        new_setting.c_lflag |= (ICANON | ECHO | ISIG);
        new_setting.c_cc[VEOL]  = '\n';
        new_setting.c_cc[VERASE] = 0x7F;
        new_setting.c_iflag |= ICRNL;
        new_setting.c_oflag |= ONLCR;
    }

    // set data bits
    switch (data_bits)
    {
        case 8:
            new_setting.c_cflag |= CS8;
            break;
        case 7:
            new_setting.c_cflag |= CS7;
            break;
        case 6:
            new_setting.c_cflag |= CS6;
            break;
        case 5:
            new_setting.c_cflag |= CS5;
            break;
        default :
            LOG_DEBUG("invalid data bits!");
            return -1;
    }

    // set verify bits
    switch (verify)
    {
        case 'N':
            new_setting.c_cflag &= ~PARENB;
            break;
        case 'E':
            new_setting.c_cflag |= PARENB;
            new_setting.c_cflag &= ~PARODD;
            new_setting.c_oflag |= (INPCK | ISTRIP);
            break;
        case 'O':
            new_setting.c_cflag |= PARENB;
            new_setting.c_cflag |= PARODD;
            new_setting.c_oflag |= (INPCK | ISTRIP);
            break;
        default :
            LOG_DEBUG("invalid verify mode!");
            return -1;
    }

    // set stop bits
    switch (stop_bits)
    {
        case 1:
            new_setting.c_cflag &= ~CSTOPB;
            break;
        case 2:
            new_setting.c_cflag |= CSTOPB;
            break;
        default :
            LOG_DEBUG("invalid stop_bits!");
            return -1;
    }

    // set buad rate
    switch (speed)
    {
        case 115200:
            cfsetispeed(&new_setting, B115200);
            cfsetospeed(&new_setting, B115200);
            break;
        case 2400:
            cfsetispeed(&new_setting, B2400);
            cfsetospeed(&new_setting, B2400);
            break;
        case 4800:
            cfsetispeed(&new_setting, B4800);
            cfsetospeed(&new_setting, B4800);
            break;
        case 9600:
            cfsetispeed(&new_setting, B9600);
            cfsetospeed(&new_setting, B9600);
            break;
        case 921600:
            cfsetispeed(&new_setting, B921600);
            cfsetospeed(&new_setting, B921600);
            break;
        default :
            cfsetispeed(&new_setting, B9600);
            cfsetospeed(&new_setting, B9600);
            break;
    }

    // set timeout
    new_setting.c_cc[VMIN] = 1;  // 最少获取到一位数据才返回，控制read函数的返回时机
    new_setting.c_cc[VTIME]  = 0;  // 一直等待，直到收到数据

    // set struct
    if (tcsetattr(fd, TCSANOW, &new_setting)) {
        LOG_DEBUG("set terminal attributes failed!");
        return -1;
    }


    return 0;
}

int main(int argc, char const *argv[])
{
    LOG_DEBUG("Enter main!");
    
    // open device
    int uart_fd = open(STM32_UART8_PATH, O_RDWR | O_NOCTTY);
    if (uart_fd < 0) {
        LOG_DEBUG("Open tty device failed!");
        return EXCUTE_FAILED_EXIT;
    }

    // set attributes
    // baud rate, data bits, stop bit, verify bit, raw mode
    // eg: 8N1 115200, raw mode
    if (config_serial_attr(uart_fd, 8, 'N', 1, 115200, true)) {
        LOG_DEBUG("configurate serial device failed!");
        goto error_exit;
    }

    char ich = 0;
    char read_ch = 0;
    // get data
    while (1) {
        scanf("%c", &ich);
        LOG_DEBUG("get char from console: 0x%x %c", ich, ich);

        // write to uart8
        if (write(uart_fd, &ich, 1) < 0) {
            LOG_DEBUG("write char to serial device failed: %s", strerror(errno));
            goto error_exit;
        }

        // read data back from uart 8
        if (read(uart_fd, &read_ch, 1) < 0) {
            LOG_DEBUG("read char from serial device failed: %s", strerror(errno));
            goto error_exit;
        }
        LOG_DEBUG("read char brack from uart8: 0x%x %c", read_ch, read_ch);
    }

    // close device
    close(uart_fd);
    return EXCUTE_SUCCESS_EXIT;

error_exit:
    close(uart_fd);
    return EXCUTE_FAILED_EXIT;
}
