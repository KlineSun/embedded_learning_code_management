#include <stdio.h>
#include "common_util.h"
#include "key_value_table.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <string.h>
#include <errno.h>
#include "socket_tst.h"
#include <sys/types.h>
#include <poll.h>
#include <signal.h>


int main(int argc, char const *argv[])
{

    LOG_INFO("Enter main!");

    // init server
    int ret = socket_server_init();
    if (ret != 0) {
        LOG_INFO("Init server failed!");
        return EXCUTE_FAILED_EXIT;
    }

    // init client
    ret = socket_client_init();
    if (ret != 0) {
        LOG_INFO("Init client failed!");
        return EXCUTE_FAILED_EXIT;
    }

    while (true) {
        sleep(10);
    }

    socket_client_deinit();
    socket_server_deinit();
    return EXCUTE_SUCCESS_EXIT;
}
