#include <stdio.h>
#include "common_util.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <string.h>
#include <errno.h>
#include "fb_util.h"


int fb_init(const char *fb_path, fb_t *fb)
{
    if (fb_path == NULL || fb == NULL) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }
    // 打开设备文件
    int fd = open(fb_path, O_RDWR);
    if (fd < 0) {
        LOG_INFO("open framebuffer device failed");
        return -1;
    }

    // 获取设备信息
    int ret = ioctl(fd, FBIOGET_VSCREENINFO, &(fb->sc_var));
    if (ret < 0) {
        LOG_INFO("get variable screen info failed");
        close(fd);
        return -1;
    }

    fb->pixel_bs = fb->sc_var.bits_per_pixel / 8;
    fb->map_size = fb->sc_var.xres * fb->sc_var.yres * fb->pixel_bs;
    fb->line_bs  = fb->sc_var.xres * fb->pixel_bs;

    LOG_INFO("pixel_bs: %d, map_size: %d, line_bs: %d", fb->pixel_bs, (int)fb->map_size, fb->line_bs);

    // 获取设备内存
    u_8bit_t *mmap_ptr = (u_8bit_t *)mmap(NULL, fb->map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_ptr <= 0 || mmap_ptr == MAP_FAILED) {
        LOG_INFO("mmap framebuffer device failed");
        close(fd);
        return -1;
    }

    // clear
    memset(mmap_ptr, 0, fb->map_size);
    fb->map_ptr = mmap_ptr;

    // 关闭设备文件
    close(fd);
    return 0;
}

void fb_deinit(fb_t *fb)
{
    if (fb->map_ptr != NULL) {
        munmap(fb->map_ptr, fb->map_size);
        fb->map_ptr = NULL;
    }
    fb->map_size = 0;
}

void fb_fill(fb_t *fb, size_t fill_size, size_t offset, u_8bit_t byte_value)
{
    if (fb == NULL || (fill_size + offset) > fb->map_size) {
        LOG_INFO("Invalid parameter!");
        return;
    }

    memset(fb->map_ptr + offset, byte_value, fill_size);
}

int draw_pixel(fb_t *fb, point_t *pp, COLOR_T color)
{
    if (fb == NULL || pp == NULL) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    if (pp->x > fb->sc_var.xres ||  pp->y > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%d, %d)", pp->x, pp->y);
        return -1;
    }

    u_8bit_t *addr_8   = fb->map_ptr + pp->y * fb->line_bs + pp->x * fb->pixel_bs;
    unsigned short *addr_16 = (unsigned short *)addr_8;
    unsigned int *addr_32   = (unsigned int *)addr_8;
    unsigned int red = 0, green = 0, blue = 0, comb_color = 0;

    switch (fb->sc_var.bits_per_pixel) {
        case 8:
            *addr_8 = color;
            break;
        case 16:
            // 565
            red   = (color >> 16) & 0xff;
            green = (color >> 8)  & 0xff;
            blue =  color & 0xff;
            //LOG_INFO("get RGB: red = 0x%x, green = 0x%x, blue = 0x%x", red, green, blue);
            comb_color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            //LOG_INFO("combine RGB565: 0x%x", comb_color);
            *addr_16 = comb_color;
            break;
        case 32:
            *addr_32 = color;
            break;
        default:
            LOG_INFO("Unsupport bpp!");
            return -1;
    }
    return 0;
}

int draw_bitmap(fb_t *fb, bitmap_t *b_map, point_t *pp, COLOR_T color)
{
    if (fb == NULL || b_map == NULL || pp == NULL) {
        LOG_INFO("invalid position: (%d, %d)", pp->x, pp->y);
        return -1;
    }

    int x_max = pp->x + b_map->width;
    int y_max = pp->y + b_map->rows;

    // LOG_INFO("x range: (%d, %d)", pp->x, x_max);
    // LOG_INFO("y range: (%d, %d)", pp->y, y_max);
    // LOG_INFO("pitch: %d", b_map->pitch);

    // One byte corresponds to one pixel
    point_t pen;
    int offset = 0;
    for (int i = 0; i < b_map->width; i++) {
        // pos x
        pen.x = pp->x + i;
        for (int j = 0; j < b_map->rows; j++) {
            // pos y
            pen.y = pp->y + j;
            if (pen.x > fb->sc_var.xres || pen.y > fb->sc_var.yres) {
                //LOG_INFO("pixel position out of the range!");
                continue;
            }

            offset = j * b_map->pitch + i;
            if (b_map->buffer[offset] != 0) {
                draw_pixel(fb, &pen, color);
            }
        }
    }

    return 0;
}

/*
int draw_ascii(fb_t *fb, point_t *pt, char c, unsigned int color)
{
    if (pt->x + 8 > fb->sc_var.xres ||  pt->y + 16 > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%d, %d), font size: 8*16", pt->x, pt->y);
        return -1;
    }

    // get bitmap
    u_8bit_t *bitmap = get_ascii_bitmap_8x16(c);
    if (bitmap == NULL) {
        LOG_INFO("No resource of ascii bitmap here!");
        return -1;
    }

    // draw bitmap
    u_8bit_t _ch = '\0';
    point_t _pt;
    for (int i = 0; i < 16; i++) {
        _ch = *(bitmap + i);
        LOG_INFO("get value: 0x%02x", _ch);
        for (int j = 0; j < 8; j++) {
            _pt.x = pt->x + j;
            _pt.y = pt->y + i;
            if (_ch & (0x80 >> j)) {
                draw_pixel(fb, &_pt, color);
            }
        }
    }

    return 0;
}

int draw_chinese(fb_t *fb, point_t *pt, u_8bit_t *code, unsigned int color)
{
    if (pt->x + 16 > fb->sc_var.xres ||  pt->y + 16 > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%d, %d), out of the screen!", pt->x, pt->y);
        return -1;
    }

    // get bitmap
    u_8bit_t *bitmap = get_bitmap_16x16(fb->map_ptr, fb->map_size, code);
    if (bitmap == NULL) {
        LOG_INFO("No resource of ascii bitmap here!");
        return -1;
    }

    // draw bitmap
    u_8bit_t ch[CHINESE_BYTES] = {0};
    point_t _pt;
    for (int i = 0; i < 16; i++) {
        ch[0] = *(bitmap + i * 2);
        ch[1] = *(bitmap + i * 2 + 1);
        LOG_INFO("get value: 0x%02x 0x%02x", ch[0], ch[1]);
        for (int j = 0; j < 16; j++) {
            _pt.x = pt->x + j;
            _pt.y = pt->y + i;
            if (j < 8 && (ch[0] & (0x80 >> j))) {
                draw_pixel(fb, &_pt, color);
            } else if (j >= 8 && (ch[1] & (0x80 >> (j - 8)))) {
                draw_pixel(fb, &_pt, color);
            }
        }

        memset(ch, 0, CHINESE_BYTES);
    }

    return 0;
}
*/

