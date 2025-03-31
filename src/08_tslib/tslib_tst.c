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
#include "tslib_tst.h"
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include "tslib.h"
#include <math.h>



int main(int argc, char const *argv[])
{

    LOG_DEBUG("Enter main!");

    struct tsdev *ts = NULL;
    int slot_num = 0, ret = -1;

    struct ts_sample_mt *smp = NULL;
    struct ts_sample_mt *local_smp = NULL;

    ts = ts_setup(NULL, 0);
    if (!ts) {
        LOG_DEBUG("tsdev setup faied!");
        return EXCUTE_FAILED_EXIT;
    }


    slot_num = slot_num > 0 ? slot_num : MAX_MT_INPUT_SLOT;
    smp = (struct ts_sample_mt *)calloc(slot_num, sizeof(struct ts_sample_mt));
    local_smp = (struct ts_sample_mt *)calloc(slot_num, sizeof(struct ts_sample_mt));
    if (smp == NULL || local_smp == NULL) {
        LOG_DEBUG("calloc faied!");
        return EXCUTE_FAILED_EXIT;
    }

    int finger_cnt = 0;
    int finger_flag = 0;
    double distance = 0;
    do {
        // 采样
        ret = ts_read_mt(ts, &smp, slot_num, 1);
        if (ret <= 0) {
            LOG_DEBUG("read mt result failed!");
            break;
        }

        // 更新数据
        finger_flag = 0;
        for (int i = 0; i < slot_num; i++) {
            if (smp[i].valid != 0 /* && smp[i].pen_down == 1 */ ) {
                finger_cnt++;
                LOG_DEBUG("slot%d is touching on (%d, %d), pres: %d, tracking_id: %d.",
                        smp[i].slot, smp[i].x, smp[i].y, smp[i].pressure, smp[i].tracking_id);
                finger_flag |= (smp[i].pressure != 0) << i;
                
                // 计算两个按下触点之间的距离
                if (i > 0 && smp[i - 1].pressure > 0 && smp[i].pressure > 0) {
                    double distance_x = smp[i].x - smp[i - 1].x;
                    double distance_y = smp[i].y - smp[i - 1].y;
                    distance = sqrt(distance_x * distance_x + distance_y * distance_y);
                    LOG_DEBUG("distance between slot%d and slot%d is %lf.", i-1, i, distance);
                }
            }

            if (smp[i].pressure != 0)
                    finger_flag++;
        }

        if (!finger_flag) {
            LOG_DEBUG("All finger moved out!");
            break;
        }
    } while (true);

    if (ts_close(ts)) {
        LOG_DEBUG("tsdev setup faied!");
        free(smp);
        return EXCUTE_FAILED_EXIT;
    }
    free(smp);
    return EXCUTE_SUCCESS_EXIT;
}
