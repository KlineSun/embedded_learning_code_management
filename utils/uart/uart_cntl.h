#ifndef UART_CNTL_H
#define UART_CNTL_H

#include <stdbool.h>

int config_uart_attr(int fd, int data_bits, char verify, int stop_bits, int speed, bool is_raw_mode);

#endif //UART_CNTL_H