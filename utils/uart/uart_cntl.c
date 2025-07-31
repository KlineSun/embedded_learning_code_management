#include <termios.h>
#include <string.h>
#include "uart_cntl.h"
#include "common_util.h"

int config_uart_attr(int fd,
                        int data_bits, /* 数据个位 */
                        char verify, /* 校验模式: N(无校验), E(偶校验), O(奇校验) */
                        int stop_bits, /* 停止位个数 */
                        int speed, /* 波特率*/
                        bool is_raw_mode /* 是为原始模式，否则为规范模式 */
                    )
{
    // 检查参数是否合法
    if (fd < 0 || speed <= 0 || speed > 921600 || data_bits < 5 || data_bits > 8) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }
    LOG_INFO("Terminal setting: %d%c%d %d, %s", data_bits,
                                                verify,
                                                stop_bits,
                                                speed,
                                                is_raw_mode ? "Raw Mode" : "Canonical Mode");

    // get origin data
    struct termios origin_setting, new_setting;
    if (tcgetattr(fd, &origin_setting)) {
        LOG_INFO("get struct termios failed!");
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
            LOG_INFO("invalid data bits!");
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
            LOG_INFO("invalid verify mode!");
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
            LOG_INFO("invalid stop_bits!");
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
        LOG_INFO("set terminal attributes failed!");
        return -1;
    }
    LOG_INFO("Set success!");
    return 0;
}
