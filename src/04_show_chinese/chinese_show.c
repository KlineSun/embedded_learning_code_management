#include "chinese_show.h"
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
#include "font_8x16.h"
#include "hzk_code.h"
#include "fb_util.h"


/*typedef struct {
    char fb_path[128];
    void *map_ptr;
    size_t map_size;
    // bytes of line
    int line_bs;
    // bytes of a pixel
    int pixel_bs;
    struct fb_var_screeninfo sc_var;
} fb_t;

typedef struct
{
    int x;
    int y;
} point_t;

typedef struct {
    point_t pt;
    // format: 0x00RRGGBB
    unsigned int color;
} pixel_t;
*/


static unsigned char *g_hzk_mem = NULL;
static size_t g_hzk_mem_size = 0;

/*
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
    char *mmap_ptr = (unsigned char *)mmap(NULL, fb->map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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

void fb_fill(fb_t *fb, size_t fill_size, size_t offset, unsigned char byte_value)
{
    if (fb == NULL || (fill_size + offset) > fb->map_size) {
        LOG_INFO("Invalid parameter!");
        return;
    }

    memset(fb->map_ptr + offset, byte_value, fill_size);
}

int draw_pixel(fb_t *fb, pixel_t *pp)
{
    if (fb == NULL || pp == NULL) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    if (pp->pt.x > fb->sc_var.xres ||  pp->pt.y > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%d, %d)", pp->pt.x, pp->pt.y);
        return -1;
    }

    unsigned char *addr_8   = fb->map_ptr + pp->pt.y * fb->line_bs + pp->pt.x * fb->pixel_bs;
    unsigned short *addr_16 = (unsigned short *)addr_8;
    unsigned int *addr_32   = (unsigned int *)addr_8;
    unsigned int red = 0, green = 0, blue = 0, comb_color = 0;

    switch (fb->sc_var.bits_per_pixel) {
        case 8:
            *addr_8 = pp->color;
            break;
        case 16:
            // 565
            red   = (pp->color >> 16) & 0xff;
            green = (pp->color >> 8)  & 0xff;
            blue =  pp->color & 0xff;
            //LOG_INFO("get RGB: red = 0x%x, green = 0x%x, blue = 0x%x", red, green, blue);
            comb_color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            //LOG_INFO("combine RGB565: 0x%x", comb_color);
            *addr_16 = comb_color;
            break;
        case 32:
            *addr_32 = pp->color;
            break;
        default:
            LOG_INFO("Unsupport bpp!");
            return -1;
    }
    return 0;
}
*/

unsigned char *get_ascii_bitmap_8x16(char val)
{
#ifdef ACSII_RESOURCE_SUPPORT

    return &fontdata_8x16[val * 16];
#endif

    return NULL;
}

unsigned char *get_bitmap_16x16(unsigned char *bitmap_repo, size_t repo_size, unsigned char *code)
{
    if (bitmap_repo == NULL || code == NULL) {
        LOG_INFO("Invalid parameter!");
        return NULL;
    }

#ifdef HZK16_RESOURCE_SUPPORT

    unsigned long index = ((code[0] - ASCII_RESERVED) * AREA_CHINESE_CNT + (code[1] - ASCII_RESERVED)) * CHINESE_BITMAP_BYTES;
    LOG_INFO("get index: %lu", index);

    if (index >= repo_size) {
        LOG_INFO("index out of the bitmap repository!");
        return NULL;
    }

    return &bitmap_repo[index];
#endif

    return NULL;
}


int draw_ascii(fb_t *fb, point_t *pt, char c, unsigned int color)
{
    if (pt->x + 8 > fb->sc_var.xres ||  pt->y + 16 > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%d, %d), font size: 8*16", pt->x, pt->y);
        return -1;
    }

    // get bitmap
    unsigned char *bitmap = get_ascii_bitmap_8x16(c);
    if (bitmap == NULL) {
        LOG_INFO("No resource of ascii bitmap here!");
        return -1;
    }

    // draw bitmap
    unsigned char _ch = '\0';
    pixel_t pixel;
    for (int i = 0; i < 16; i++) {
        _ch = *(bitmap + i);
        LOG_INFO("get value: 0x%02x", _ch);
        for (int j = 0; j < 8; j++) {
            pixel.pt.x = pt->x + j;
            pixel.pt.y = pt->y + i;
            pixel.color = color;
            if (_ch & (0x80 >> j)) {
                draw_pixel(fb, &pixel);
            }
        }
    }

    return 0;
}

int draw_chinese(fb_t *fb, point_t *pt, unsigned char *code, unsigned int color)
{
    if (pt->x + 16 > fb->sc_var.xres ||  pt->y + 16 > fb->sc_var.yres) {
        LOG_INFO("invalid position: (%d, %d), out of the screen!", pt->x, pt->y);
        return -1;
    }

    // get bitmap
    unsigned char *bitmap = get_bitmap_16x16(g_hzk_mem, g_hzk_mem_size, code);
    if (bitmap == NULL) {
        LOG_INFO("No resource of ascii bitmap here!");
        return -1;
    }

    // draw bitmap
    unsigned char ch[CHINESE_BYTES] = {0};
    pixel_t pixel;
    for (int i = 0; i < 16; i++) {
        ch[0] = *(bitmap + i * 2);
        ch[1] = *(bitmap + i * 2 + 1);
        LOG_INFO("get value: 0x%02x 0x%02x", ch[0], ch[1]);
        for (int j = 0; j < 16; j++) {
            pixel.pt.x = pt->x + j;
            pixel.pt.y = pt->y + i;
            pixel.color = color;
            if (j < 8 && (ch[0] & (0x80 >> j))) {
                draw_pixel(fb, &pixel);
            } else if (j >= 8 && (ch[1] & (0x80 >> (j - 8)))) {
                draw_pixel(fb, &pixel);
            }
        }

        memset(ch, 0, CHINESE_BYTES);
    }

    return 0;
}

int main(int argc, char const *argv[])
{

    fb_t fb;
    // fb初始化
    int ret = fb_init(FB_PATH, &fb);
    if (0 != ret) {
        LOG_INFO("parse basic info of table failed!");
        return EXIT_FAILURE;
    }

    // 打印设备信息
    LOG_INFO("resolution: %d x %d, bpp: %d", fb.sc_var.xres, fb.sc_var.yres, fb.sc_var.bits_per_pixel);
    LOG_INFO("frame buffer size: %ld", (unsigned long)fb.map_size);

    // open character set
    if (g_hzk_mem == NULL || g_hzk_mem_size == 0) {
        g_hzk_mem_size = (unsigned char *)file_mmap(HZK_PATH, -1, PROT_READ, MAP_SHARED, 0, &g_hzk_mem);
        if (g_hzk_mem_size <= 0 || g_hzk_mem == NULL) {
            LOG_INFO("Map %s failed!", HZK16_PATH);
            return -1;
        }
    }

    // try to show gb2313 code
    if (chinese_str1 != NULL && strcmp(chinese_str1, "")) {
        LOG_INFO("chinese_str1: %s", chinese_str1);
        char result[512] = {0};
        for (int j = 0; j < strlen(chinese_str1); j++) {
            if (j == 0) {
                sprintf(result, "%02x", chinese_str1[j]);
            } else {
                sprintf(result, "%s %02x", result, chinese_str1[j]);
            }
        }
        LOG_INFO("chinese_str1 hex: %s", result);

        for (int j = 0; j < strlen(chinese_str1); j += CHINESE_BYTES) {
            unsigned char code[CHINESE_BYTES] = {0};
            code[0] = chinese_str1[j];
            code[1] = chinese_str1[j + 1];
            LOG_INFO("get code: 0x%02x 0x%02x", code[0], code[1]);
            
            point_t pt2 = {
                .x = 50 + (j + 1) * CHINESE_BITMAP_WIDTH  + CHINESE_WORD_SPACE,
                .y = 50
            };
            LOG_INFO("position: (%d, %d)", pt2.x, pt2.y);
            draw_chinese(&fb, &pt2, code, COLOR_RED);
        }
    }

    // 反初始化fb
    fb_deinit(&fb);
    return 0;
}
