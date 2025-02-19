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
#include "input_tst.h"
#include <linux/input.h>


const char* event_bit_names[] = {
    "EV_SYN		",
    "EV_KEY		",
    "EV_REL		",
    "EV_ABS		",
    "EV_MSC		",
    "EV_SW		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "NULL		",
    "EV_LED		",
    "EV_SND		",
    "NULL		",
    "EV_REP		",
    "EV_FF		",
    "EV_PWR		",
    "EV_FF_STATUS"
};

/**
 * format:
 *  freetype_tst_vec [x] [y] [font_size] [angle]
 */
int main(int argc, char const *argv[])
{

    LOG_DEBUG("Enter main!");

    /**
     * 驱动上报数据的三个信息：
     * type：哪一类？上报的input是哪一类，比如EV_KEY（按键类），
     *      在内核include\uapi\linux\input-event-codes.h中可以查看所有类型的event
     * code：哪一个？比如KEY_A
     * value：什么值？比如0-松开，1-按下，2-长按
    */

    int fd = open(INPUT_EVENT0_PATH, O_RDWR);
    if (fd <= 0) {
        LOG_DEBUG("Open dev %s failed!", INPUT_EVENT0_PATH);
        return EXCUTE_FAILED_EXIT;
    }

    int ev_version[8] = {0};
    int ret = ioctl(fd, EVIOCGVERSION, ev_version);
    if (ret != 0) {
        LOG_DEBUG("ioctl dev %s failed!", INPUT_EVENT0_PATH);
        close(fd);
        return EXCUTE_FAILED_EXIT;
    }
    LOG_DEBUG("Get event version: %x", ev_version);

    struct input_id id;
    ret = ioctl(fd, EVIOCGID, ev_version, &id);
    if (ret != 0) {
        LOG_DEBUG("ioctl dev %s failed!", INPUT_EVENT0_PATH);
        close(fd);
        return EXCUTE_FAILED_EXIT;
    }
    LOG_DEBUG("bustype = 0x%x\n", id.bustype );
    LOG_DEBUG("vendor	= 0x%x\n", id.vendor  );
    LOG_DEBUG("product = 0x%x\n", id.product );
    LOG_DEBUG("version = 0x%x\n", id.version );

    unsigned char evbit[8] = {0};
    ret = ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
    if (ret <= 0 || ret > sizeof(evbit)) {
        LOG_DEBUG("ioctl dev %s failed!", INPUT_EVENT0_PATH);
        close(fd);
        return EXCUTE_FAILED_EXIT;
    }

    unsigned char byte_val;
    LOG_DEBUG("Support event type:", INPUT_EVENT0_PATH);
    for (int i = 0; i < sizeof(evbit); i++) {
        for (int j = 0; j < 8; j++) {
            byte_val = evbit[i];
            if (byte_val & (1 << j)) {
                LOG_DEBUG("%s", event_bit_names[i * 8 + j]);
            }
        }
    }
    close(fd);
    return EXCUTE_SUCCESS_EXIT;
}
