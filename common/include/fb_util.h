#ifndef FB_UTIL_H
#define FB_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "common_util.h"
#include <linux/fb.h>

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

typedef struct {
    int width;
    int height;
    int ln_bys;
    int advance_x;
    int advance_y;
    unsigned char *buf;
} bitmap_t;

int fb_init(const char *fb_path, fb_t *fb);
void fb_deinit(fb_t *fb);
void fb_fill(fb_t *fb, size_t fill_size, size_t offset, unsigned char byte_value);
int draw_pixel(fb_t *fb, point_t *pp, COLOR_T color);
int draw_bitmap(fb_t *fb, bitmap_t *b_map, point_t *pp, COLOR_T color);


#endif //FB_UTIL_H