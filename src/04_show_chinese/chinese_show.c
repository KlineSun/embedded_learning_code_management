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


typedef struct {
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

int fb_init(const char *fb_path, fb_t *fb)
{
    if (fb_path == NULL || fb == NULL) {
        LOG_DEBUG("Invalid parameter!");
        return -1;
    }
    // 打开设备文件
    int fd = open(fb_path, O_RDWR);
    if (fd < 0) {
        LOG_DEBUG("open framebuffer device failed");
        return -1;
    }

    // 获取设备信息
    int ret = ioctl(fd, FBIOGET_VSCREENINFO, &(fb->sc_var));
    if (ret < 0) {
        LOG_DEBUG("get variable screen info failed");
        close(fd);
        return -1;
    }

    fb->pixel_bs = fb->sc_var.bits_per_pixel / 8;
    fb->map_size = fb->sc_var.xres * fb->sc_var.yres * fb->pixel_bs;
    fb->line_bs  = fb->sc_var.xres * fb->pixel_bs;

    LOG_DEBUG("pixel_bs: %d, map_size: %d, line_bs: %d", fb->pixel_bs, (int)fb->map_size, fb->line_bs);

    // 获取设备内存
    char *mmap_ptr = (unsigned char *)mmap(NULL, fb->map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_ptr <= 0 || mmap_ptr == MAP_FAILED) {
        LOG_DEBUG("mmap framebuffer device failed");
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
        LOG_DEBUG("Invalid parameter!");
        return;
    }

    memset(fb->map_ptr + offset, byte_value, fill_size);
}

int draw_pixel(fb_t *fb, pixel_t *pp)
{
    if (fb == NULL || pp == NULL) {
        LOG_DEBUG("Invalid parameter!");
        return -1;
    }

    if (pp->pt.x > fb->sc_var.xres ||  pp->pt.y > fb->sc_var.yres) {
        LOG_DEBUG("invalid position: (%d, %d)", pp->pt.x, pp->pt.y);
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
            //LOG_DEBUG("get RGB: red = 0x%x, green = 0x%x, blue = 0x%x", red, green, blue);
            comb_color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
            //LOG_DEBUG("combine RGB565: 0x%x", comb_color);
            *addr_16 = comb_color;
            break;
        case 32:
            *addr_32 = pp->color;
            break;
        default:
            LOG_DEBUG("Unsupport bpp!");
            return -1;
    }
    return 0;
}

unsigned char *get_ascii_bitmap_8x16(char val)
{
#ifdef ACSII_RESOURCE_SUPPORT

    return &fontdata_8x16[val * 16];
#endif

    return NULL;
}

int draw_ascii(fb_t *fb, point_t *pt, char c, unsigned int color)
{
    if (pt->x + 8 > fb->sc_var.xres ||  pt->y + 16 > fb->sc_var.yres) {
        LOG_DEBUG("invalid position: (%d, %d), font size: 8*16", pt->x, pt->y);
        return -1;
    }

    // get bitmap
    unsigned char *bitmap = get_ascii_bitmap_8x16(c);
    if (bitmap == NULL) {
        LOG_DEBUG("No resource of ascii bitmap here!");
        return -1;
    }

    // draw bitmap
    unsigned char _ch = '\0';
    unsigned int index = 0;
    pixel_t pixel;
    for (int i = 0; i < 16; i++) {
        _ch = *(bitmap + i);
        LOG_DEBUG("get value: 0x%02x", _ch);
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

int main(int argc, char const *argv[])
{

    fb_t fb;
    // fb初始化
    int ret = fb_init(FB_PATH, &fb);
    if (0 != ret) {
        LOG_DEBUG("parse basic info of table failed!");
        return EXIT_FAILURE;
    }

    // 打印设备信息
    LOG_DEBUG("resolution: %d x %d, bpp: %d", fb.sc_var.xres, fb.sc_var.yres, fb.sc_var.bits_per_pixel);
    LOG_DEBUG("frame buffer size: %ld", (unsigned long)fb.map_size);

    // 绘制ascii码值
    point_t pt1 = {
        .x = 200,
        .y = 100
    };
    draw_ascii(&fb, &pt1, 'C', 0xffffff);


    for (int i = 0; i < LIST_LEN(name_table_gb2312); i++) {
        if (name_table_gb2312[i] != NULL && strcmp(name_table_gb2312[i], "")) {
            LOG_DEBUG("name_table_gb2312[%d]: %s", i, name_table_gb2312[i]);
            char result[512] = {0};
            for (int j = 0; j < LIST_LEN(name_table_gb2312[i]); j++) {
                //LOG_DEBUG("name_table_gb2312[%d][%d] hex: %02x", i, j, name_table_gb2312[i][j]);
                if (j == 0) {
                    sprintf(result, "%02x", name_table_gb2312[i][j]);
                } else {
                    sprintf(result, "%s %02x", result, name_table_gb2312[i][j]);
                }
            }
            LOG_DEBUG("name_table_gb2312[%d] hex: %s", i, result);
        }
    }

    for (int i = 0; i < LIST_LEN(name_table_utf8); i++) {
        if (name_table_utf8[i] != NULL && strcmp(name_table_utf8[i], "")) {
            LOG_DEBUG("name_table_utf8[%d]: %s", i, name_table_utf8[i]);
            char result[512] = {0};
            for (int j = 0; j < LIST_LEN(name_table_utf8[i]); j++) {
                //LOG_DEBUG("name_table_utf8[%d][%d] hex: %02x", i, j, name_table_utf8[i][j]);
                if (j == 0) {
                    sprintf(result, "%02x", name_table_utf8[i][j]);
                } else {
                    sprintf(result, "%s %02x", result, name_table_utf8[i][j]);
                }
            }
            LOG_DEBUG("name_table_utf8[%d] hex: %s", i, result);
        }
    }


    int fd = open("/mnt/name_gb2312.txt", O_RDWR);
    if (fd <= 0) {
        LOG_DEBUG("open file failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    struct stat file_info;
    
    ret = fstat(fd, &file_info);
    if (ret != 0) {
        LOG_DEBUG("get file state failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    size_t file_size = file_info.st_size;
    LOG_DEBUG("get file size: %u", file_size);
    if (file_size <= 0) {
        LOG_DEBUG("file size is abnormal!");
        return EXIT_FAILURE;
    }

    void *file_ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_ptr == MAP_FAILED) {
        LOG_DEBUG("get file state failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    LOG_DEBUG("Map file to address: %p", file_ptr);

    char *p_content = (char *)file_ptr;
    LOG_DEBUG("Get content: %s", p_content);
    char print_buf[512] = {0};
    for (int i = 0; i < file_size && i < sizeof(print_buf); i++) {
        if (i == 0) {
            sprintf(print_buf, "%02x", p_content[i]);
        } else {
            sprintf(print_buf, "%s %02x", print_buf, p_content[i]);
        }
    }
    LOG_DEBUG("Get content hex: %s", print_buf);


    ret = munmap(file_ptr, file_size);
    if (ret != 0) {
        LOG_DEBUG("munmap file pointer failed!");
        return EXIT_FAILURE;
    }

    close(fd);
    // 反初始化fb
    fb_deinit(&fb);
    return 0;
}
