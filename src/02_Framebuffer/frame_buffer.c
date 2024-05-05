#include "frame_buffer.h"
#include <stdio.h>
#include "common_util.h"
#include "key_value_table.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>

static struct fb_var_screeninfo g_var_info;
static unsigned char *g_fb_mem = NULL;
size_t g_fb_mem_size = 0;
int line_width_bytes;

int fb_init()
{
    // 打开设备文件
    int fd = open(FB_PATH, O_RDWR);
    if (fd < 0) {
        LOG_DEBUG("open framebuffer device failed");
        return -1;
    }

    // 获取设备信息
    int ret = ioctl(fd, FBIOGET_VSCREENINFO, &g_var_info);
    if (ret < 0) {
        LOG_DEBUG("get variable screen info failed");
        close(fd);
        return -1;
    }

    g_fb_mem_size = g_var_info.xres * g_var_info.yres * g_var_info.bits_per_pixel / 8;

    // 获取设备内存
    g_fb_mem = (unsigned char *)mmap(NULL, g_fb_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (g_fb_mem <= 0 || g_fb_mem == MAP_FAILED) {
        LOG_DEBUG("mmap framebuffer device failed");
        close(fd);
        return -1;
    }

    // 关闭设备文件
    close(fd);
    return 0;
}

void fb_deinit()
{
    if (g_fb_mem) {
        munmap(g_fb_mem, g_fb_mem_size);
        g_fb_mem = NULL;
    }
    g_fb_mem_size = 0;
}

void fb_clear()
{
    memset(g_fb_mem, 0xff, g_fb_mem_size);
}

void draw_pixel(int x, int y, unsigned int color)
{
    if (x > g_var_info.xres ||  y > g_var_info.yres) {
        LOG_DEBUG("invalid position: (%d, %d)", x, y);
        return;
    }

    line_width_bytes    = g_var_info.xres * g_var_info.bits_per_pixel / 8;
    unsigned char *addr_8   = g_fb_mem + y * line_width_bytes + x * g_var_info.bits_per_pixel / 8;
    unsigned short *addr_16 = (unsigned short *)addr_8;
    unsigned int *addr_32   = (unsigned int *)addr_8;
    unsigned int red = 0, green = 0, blue = 0, comb_color = 0;

    switch (g_var_info.bits_per_pixel) {
        case 8:
            *addr_8 = color;
            break;
        case 16:
            // 565
            red   = (color >> 16) & 0xff;
            green = (color >> 8)  & 0xff;
            blue =  color & 0xff;
            LOG_DEBUG("get RGB: red = 0x%x, green = 0x%x, blue = 0x%x", red, green, blue);
            comb_color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            LOG_DEBUG("combine RGB565: 0x%x", comb_color);
            *addr_16 = comb_color;
            break;
        case 32:
            *addr_32 = color;
            break;
        default:
            LOG_DEBUG("Unsupport bpp!");
            break;
    }
}


int main(int argc, char const *argv[])
{

    // fb初始化
    int ret = fb_init();
    if (0 != ret) {
        LOG_DEBUG("parse basic info of table failed!");
        return EXIT_FAILURE;
    }

    // 打印设备信息
    LOG_DEBUG("resolution: %d x %d, bpp: %d", g_var_info.xres, g_var_info.yres, g_var_info.bits_per_pixel);
    LOG_DEBUG("frame buffer size: %ld", (unsigned long)g_fb_mem_size);

    // 清空fb
    fb_clear();

    // 绘制像素点
    draw_pixel(g_var_info.xres / 2, g_var_info.yres / 2, 0x00ff00);
    for (int i = 200; i < g_var_info.yres && i < g_var_info.xres; i++) {
        draw_pixel(i, i, 0xff0000);
    }

    // 反初始化fb
    fb_deinit();
    return 0;
}
