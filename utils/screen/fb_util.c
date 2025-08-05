#include <stdio.h>
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
    int ret = 0, fd = 0;
    u_8bit_t *mmap_ptr = NULL;
    if (fb_path == NULL || fb == NULL) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }
    // 打开设备文件
    fd = open(fb_path, O_RDWR);
    if (fd < 0) {
        LOG_INFO("open framebuffer device failed");
        return -1;
    }

    // 获取设备信息
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &(fb->sc_var));
    if (ret < 0) {
        LOG_INFO("get variable screen info failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    ret = ioctl(fd, FBIOGET_FSCREENINFO, &(fb->sc_fix));
    if (ret < 0) {
        LOG_INFO("get variable screen info failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    LOG_INFO("xres=%u, yres=%u, bpp=%u\n", fb->sc_var.xres, fb->sc_var.yres, fb->sc_var.bits_per_pixel);
    // fb->map_size = fb->sc_fix.smem_len;
    fb->map_size = fb->sc_var.xres * fb->sc_var.yres * fb->sc_var.bits_per_pixel / 8;
    LOG_INFO("map_size: %d, line_length: %d", fb->map_size, fb->sc_fix.line_length);

    // 获取设备内存
    mmap_ptr = (u_8bit_t *)mmap(NULL, fb->map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_ptr <= 0 || mmap_ptr == MAP_FAILED) {
        LOG_INFO("mmap framebuffer device failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // clear
    memset(mmap_ptr, 0, fb->map_size);
    fb->map_ptr = (unsigned char *)mmap_ptr;

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

int draw_pixel(fb_t *fb, FT_Vector *sc_pos, COLOR_T color)
{
    unsigned char *addr_8 = NULL;
    unsigned short *addr_16 = NULL;
    unsigned int *addr_32 = NULL;
    int bit_offset;
    unsigned int red = 0, green = 0, blue = 0, comb_color = 0;
    if (fb == NULL || sc_pos == NULL) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    if (sc_pos->x > fb->sc_var.xres ||  sc_pos->y > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%ld, %ld)", sc_pos->x, sc_pos->y);
        return -1;
    }

    if ((fb->sc_var.bits_per_pixel % 8) == 0) {
        addr_8   = fb->map_ptr + sc_pos->y * fb->sc_fix.line_length + sc_pos->x/8;
    } else {
        addr_8   = fb->map_ptr + sc_pos->y * fb->sc_fix.line_length + sc_pos->x/8;
        bit_offset = 7 - sc_pos->x % 8;
        // if (sc_pos->x % 8)
        //     addr_8++; // 如果有位偏移，则指向下一个字节
    }

    switch (fb->sc_var.bits_per_pixel) {
        case 1: {
            *addr_8 |= (1 << bit_offset); // 给字节对应位置为1，表示点亮即可，无需设置颜色
            // printf("0x%x ", *addr_8);
            break;
        }
        case 8:
            *addr_8 = color;
            break;
        case 16:
            addr_16 = (unsigned short *)addr_8;
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
            addr_32   = (unsigned int *)addr_8;
            *addr_32 = color;
            break;
        default:
            LOG_INFO("Unsupport bpp!");
            return -1;
    }
    return 0;
}

int draw_bitmap(fb_t *fb, FT_Bitmap *b_map, FT_Vector *sc_pos, COLOR_T color)
{
    FT_Vector pen;
    int offset = 0;
    int i = 0, j = 0;
    if (fb == NULL || b_map == NULL || sc_pos == NULL) {
        LOG_INFO("invalid position: (%ld, %ld)", sc_pos->x, sc_pos->y);
        return -1;
    }

    // One byte corresponds to one pixel

    // print_FT_Bitmap(b_map);
    for (j = 0; j < b_map->rows; j++) {
        for (i = 0; i < b_map->width; i++) {
            pen.x = sc_pos->x + i;
            pen.y = sc_pos->y + j;
            if (pen.x > fb->sc_var.xres || pen.y > fb->sc_var.yres) {
                // LOG_INFO("position (%ld, %ld) out of the range!", pen.x, pen.y);
                break;
            }

            offset = j * b_map->pitch + i;
            // LOG_INFO("(%d, %d), offset=%d, pitch=%d", i, j, offset, b_map->pitch);
            if (b_map->buffer[offset] != 0) {
                draw_pixel(fb, &pen, color);
            }
        }
    }

    return 0;
}

void print_fb(fb_t *fb)
{
    int x = 0, y = 0;
    unsigned int byte_idx = 0, bit_offset = 0, bpp = 0;
    char *line_buf = NULL;
    bool bit_offset_flg = false;
    bool pixel_active = false;
    unsigned char *pen = NULL;
    if (!fb) {
        LOG_ERR("Invalid parameter!");
        return;
    }

    line_buf = malloc(fb->sc_var.xres + 1); // 逐行输出，因此申请一行的内存即可
    if (!line_buf) {
        LOG_ERR("Alloc bitmap print memory failed!");
        return;
    }

    bpp = fb->sc_var.bits_per_pixel;
    bit_offset_flg = bpp % 8 ? true : false;
    printf("fb(%u*%u), flag=%d:\n", fb->sc_var.xres, fb->sc_var.yres, bit_offset_flg);
    for (y = 0; y < fb->sc_var.yres; y++) {
        for (x = 0; x < fb->sc_var.xres; x++) {
            byte_idx = y * fb->sc_fix.line_length + x * bpp / 8;
            pen = fb->map_ptr + byte_idx;
            bit_offset = 7 - x * bpp % 8;

            pixel_active = bit_offset_flg ? (*pen & (1 << bit_offset)) : (*pen != 0);
            line_buf[x] = pixel_active ? 0x2a : 0x2e;
        }
        line_buf[fb->sc_var.xres] = '\0';
        printf("\t%s\n", line_buf);
        memset(line_buf, 0, fb->sc_var.xres+1);
    }
    free(line_buf);
}
