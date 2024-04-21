#include "kv_test.h"
#include <stdio.h>
#include "common_util.h"
#include "key_value_table.h"

int main(int argc, char const *argv[])
{
    int ret = 0;
    // 初始化kv
    ret = kv_table_init();
    if (ret != 0) {
        LOG_DEBUG("kv_table_init failed\n");
        return -1;
    }

    // 写入kv值
    ret = add_kv("ro.boot.facid_check", "check_ok");
    if (ret != 0) {
        LOG_DEBUG("kv_table_write failed\n");
        return -1;
    }

    ret = add_kv("ro.boot.facid", "shenzhen");
    if (ret != 0) {
        LOG_DEBUG("kv_table_write failed\n");
        return -1;
    }

    // 读取kv值
    char buf[100] = {0};
    ret = get_kv("ro.boot.args", buf);
    if (ret != 0) {
        LOG_DEBUG("kv_table_read failed\n");
        return -1;
    }

    LOG_DEBUG("get key = %s\n", buf);

    // 删除kv值
    if (delete_kv("name2") != 0) {
        LOG_DEBUG("kv_table_read failed\n");
        return -1;
    }

    // 关闭kv
    ret = destroy_kv_table();
    if (ret != 0) {
        LOG_DEBUG("destroy_kv_table failed\n");
        return -1;
    }

    return 0;
}
