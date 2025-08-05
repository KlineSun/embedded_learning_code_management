#ifndef FB_UTIL_H
#define FB_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/fb.h>
#include "common_util.h"
#include "freetype_util.h"


#define FONT_PAGE_MARGIN_X 20
#define FONT_PAGE_MARGIN_Y 15
#define FONT_LINE_MARGIN   20

typedef struct {
    char fb_path[128];
    unsigned char *map_ptr;
    size_t map_size;
    struct fb_var_screeninfo sc_var;
    struct fb_fix_screeninfo sc_fix;
} fb_t;

typedef struct {
    FT_Vector pt;
    // format: 0x00RRGGBB
    unsigned int color;
} pixel_t;

  typedef struct
  {
    unsigned int    rows;
    unsigned int    width;
    int             pitch;
    unsigned char*  buffer;
    unsigned short  num_grays;
    unsigned char   pixel_mode;
    unsigned char   palette_mode;
    void*           palette;
  }  bitmap_t;

int fb_init(const char *fb_path, fb_t *fb);
void fb_deinit(fb_t *fb);
void fb_fill(fb_t *fb, size_t fill_size, size_t offset, unsigned char byte_value);
int draw_pixel(fb_t *fb, FT_Vector *sc_pos, COLOR_T color);
int draw_bitmap(fb_t *fb, FT_Bitmap *b_map, FT_Vector *sc_pos, COLOR_T color);
void print_fb(fb_t *fb);

#endif //FB_UTIL_H