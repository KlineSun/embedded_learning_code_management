#include <stdio.h>
#include "common_util.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include "gps_common_info.h"
#include "uart_cntl.h"
#include "gps_uart_tst.h"


int read_gps_raw_data(int fd, char *buf, int len)
{
    if (fd <0 || !buf || len <= 0) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    LOG_INFO("Enter!");
    char recv_pool[MAX_GPS_RAW_DATA_LEN] = {0};
    char iCh = 0;
    /**
     * -1: no valid data input
     * 0:  data end
     * 1:  data inputing
    */
    int valid_field = -1;
    int i = 0;
    while (1) {
        if (read(fd, &iCh, 1) < 0) {
            LOG_INFO("read failed: %s", strerror(errno));
            return -1;
        }

        // LOG_INFO("read ch: %c 0x%x", iCh, iCh);
        if (iCh == '$') {
            valid_field = 1;
        } else if (valid_field == 1 && (iCh == '\r' || iCh == '\n')) {
            valid_field = 0;
        }

        if (valid_field == 1) {
            if (i >= MAX_GPS_RAW_DATA_LEN) {
                LOG_INFO("Data too long!");
                return -1;
            }
            recv_pool[i++] = iCh;
        } else if (valid_field == 0) {
            recv_pool[i] = '\0';
            LOG_INFO("receive a frame of raw data end!");
            break;
        } else{
            LOG_INFO("Invalid ch: %c 0x%x", iCh, iCh);
            continue;
        }

    }

    if (valid_field < 0 || i <= 1) {
        LOG_INFO("connot get gps data from uart!");
        return -1;
    }

    if (i > len) {
        LOG_INFO("Data too long!");
        return -1;
    }
    memcpy(buf, recv_pool, i);
    return i;
}

const char *g_gps_msg_table[] = {
    "$GPGGA,085412.00,3150.7821,N,11711.9339,E,1,08,1.2,56.4,M,-34.8,M,01.2,0000*76",
    "$GPGGA,085512.00,3150.8021,N,11711.9539,E,1,08,1.2,56.4,M,-34.8,M,01.2,0000*77",
    "$GPGGA,161229.00,3723.2475,N,12158.3416,W,1,10,0.8,1500.5,M,25.7,M,,*7E",
    "$GPRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A",
    "$GPRMC,085412.00,V,0000.0000,N,00000.0000,E,000.0,000.0,010123,,,N*7D",
    "$GPRMC,225446.00,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E,D*4F",
    "$GPGLL,3723.2475,N,12158.3416,W,161229.00,A,A*7C",
    "$GPGLL,0000.0000,N,00000.0000,E,085412.00,V,N*6D",
    "$GPGLL,4916.45,N,12311.12,W,225446.00,A,D*4B",
    "$GPVTG,096.5,T,083.5,M,000.0,N,000.0,K,D*3C",
    "$GPVTG,000.0,T,000.0,M,000.0,N,000.0,K,N*2C",
    "$GPVTG,077.8,T,068.7,M,029.4,N,054.5,K,A*2F",
    "$GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74",
    "$GPGSV,3,2,11,14,25,170,00,16,57,208,00,18,67,296,00,19,40,246,00*76",
    "$GPGSV,1,1,08,01,40,083,46,02,17,308,41,03,25,244,48,04,05,344,39*7D",
    "$GPGSA,A,1,,,,,,,,,,,,,,,*1B",
    "$GPGSA,A,3,19,28,14,18,27,22,31,39,08,09,05,21,,1.5,1.2,0.9*3E",
    "$GPGSA,A,2,22,18,21,06,03,09,24,,,,,,1.8,1.2,1.4*3C",
    "$GPZDA,085412.00,23,01,2024,00,00*6D",
    "$GPZDA,161229.00,15,07,2023,08,00*4A",
    "$GPZDA,235959.99,31,12,2023,00,00*5F",
    "$GPDTM,W84,,0.0,N,0.0,E,0.0,WGS84*4F",
    "$GPDTM,CHN,,0.03,S,0.02,W,-12.3,BEIJING-1954*2B",
};

void *gps_data_send_thread(void *priv)
{
    if (!priv) {
        LOG_INFO("input fd is null!");
        return NULL;
    }
    pthread_detach(pthread_self());

    int fd = *((int *)priv);
    LOG_INFO("get uart fd: %d", fd);
    if (fd <= 0) {
        LOG_INFO("fd is invalid!");
        return NULL;
    }

    while (1) {
        for (int i = 0; i < LIST_LEN(g_gps_msg_table); i++) {
            if (write(fd, g_gps_msg_table[i], strlen(g_gps_msg_table[i])) <= 0) {
                LOG_INFO("write gps data to uart failed: %s", g_gps_msg_table[i]);
            }
            sleep(2);
        }
    }

    LOG_INFO("Send endding!");
    return NULL;
}

int main(int argc, char const *argv[])
{
    LOG_INFO("Enter main!");


    
    // open device
    int uart_fd = open(STM32_UART8_PATH, O_RDWR | O_NOCTTY);
    if (uart_fd < 0) {
        LOG_INFO("Open tty device failed!");
        return EXCUTE_FAILED_EXIT;
    }

    // set attributes
    // baud rate, data bits, stop bit, verify bit, raw mode
    // eg: 8N1 115200, raw mode
    if (config_uart_attr(uart_fd, 8, 'N', 1, 9600, true)) {
        LOG_INFO("configurate serial device failed!");
        goto error_exit;
    }

    // test thread
    pthread_t tid;
    if (pthread_create(&tid, NULL, gps_data_send_thread, (void *)&uart_fd)) {
        LOG_INFO("create sending thread failed!");
        goto error_exit;
    }

    char gps_raw_data[MAX_GPS_RAW_DATA_LEN] = {0};
    gps_common_info common_info;
    while (1) {
        // read gps data from uart
        int ret = read_gps_raw_data(uart_fd, gps_raw_data, MAX_GPS_RAW_DATA_LEN);
        if (ret <= 0) {
            LOG_INFO("get raw data from GPS failed!");
            goto error_exit;
        }

        LOG_INFO("Get gps raw data: %s", gps_raw_data);

        // parse gps raw data
        gps_info_type t = parse_gps_raw_data(gps_raw_data, &common_info);
        if (t == GPS_INVALID_TYPE) {
            LOG_INFO("parse gps raw data failed!");
            continue;
        }
    }

    // close device
    close(uart_fd);
    return EXCUTE_SUCCESS_EXIT;

error_exit:
    close(uart_fd);
    return EXCUTE_FAILED_EXIT;
}
