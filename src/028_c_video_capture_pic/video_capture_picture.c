#include <stdio.h>
#include "common_util.h"



/**
 * 功能：打开一个video设备，使用其捕获功能，每捕获一帧画面，就将其保存为jpg格式
 * 拓展功能：id：V4L2_CID_BRIGHTNESS(亮度), V4L2_CID_CONTRAST(对比度), V4L2_CID_SATURATION(饱和度)
 *          亮度：b+,b- 对比度：c+,c- 饱和度：s+,s-
 * 
 * 用法：./video_capture_picture /dev/video0
*/
int main(int argc, const char **argv)
{
    LOG_INFO("Enter with argc %d", argc);

    // 打开video设备

    // 查询设备能力

    // 判断是否为捕获设备

    // 枚举支持的图像格式

    // 设置格式

    // 请求buf

    // 流式设备缓冲区映射

    // 启动视频流

    // 主循环
        // poll video设备

        // 从驱动的完成链表中获取一帧数据

        // 保存buf数据到一个新建的pic文件中

        // 将保存完的buf重新排列到驱动的空闲链表
    
    // 关闭设备

    return 0;
}