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
#include "ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftglyph.h"
#include "freetype/config/ftconfig.h"
#include <wchar.h>
#include "fb_util.h"
#include "freetype_util.h"

int freetype_init_local(FT_Library *lib, FT_Face *face, int font_size)
{
    if (lib == NULL || face == NULL) {
        LOG_INFO("Invalid paramter!");
        return INIT_LIBRARY_ERR;
    }

    int err = FT_Init_FreeType(lib);
    if (err != 0) {
        LOG_INFO("Init freetype library failed!");
        return INIT_LIBRARY_ERR;
    }

    err = FT_New_Face(*lib, FONT_DICTIONARY_PATH, 0, face);
    if (err != 0 || *face == NULL) {
        LOG_INFO("Open Fonts failed!");
        return FT_OPEN_RESOURCE_ERR;
    }

    LOG_INFO("There is %ld faces in font file", (*face)->num_faces);

    err = FT_Set_Pixel_Sizes(*face, font_size, 0);
    if (err != 0) {
        LOG_INFO("FT_Set_Pixel_Sizes failed!");
        return FT_SET_SIZE_ERR;
    }

    return FT_NO_ERR;
}

int get_strint_box(FT_Face face, wchar_t *str, int len, FT_BBox *box)
{
    if (str == NULL || face == NULL || box == NULL || len <= 0) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    FT_Vector pen = {
        .x = 0,
        .y = 0
    };
    int err = -1;
    FT_GlyphSlot ft_slot = NULL;
    FT_Set_Transform(face, 0, &pen);

    FT_BBox max_box;
    FT_BBox single_box;
    FT_Glyph glyph;

    max_box.xMax = max_box.yMax = -9999;
    max_box.xMin = max_box.yMin = 9999;
    for (int i = 0; i < len; i++) {
        err = FT_Load_Char(face, str[i], FT_LOAD_RENDER);
        if (err != 0) {
            LOG_INFO("load char failed!");
            return FT_LOAD_CHAR_ERR;
        }

        ft_slot = face->glyph;
        err = FT_Get_Glyph(ft_slot, &glyph);
        if (err != 0) {
            LOG_INFO("get glyph from slot failed!");
            return FT_GET_SLOT_ERR;
        }

        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_TRUNCATE, &single_box);

        max_box.xMax = single_box.xMax > max_box.xMax ? single_box.xMax : max_box.xMax;
        max_box.xMin = single_box.xMin < max_box.xMin ? single_box.xMin : max_box.xMin;
        max_box.yMax = single_box.yMax > max_box.yMax ? single_box.yMax : max_box.yMax;
        max_box.yMin = single_box.yMin < max_box.yMin ? single_box.yMin : max_box.yMin;
    }
    LOG_INFO("xMax=%ld, xMin=%ld, yMax=%ld, yMin=%ld", max_box.xMax, max_box.xMin, max_box.yMax, max_box.yMin);

    memcpy(box, &max_box, sizeof(FT_BBox));
    return 0;
}



/**
 * format:
 *  freetype_tst_vec [x] [y] [font_size] [angle]
 */
int main(int argc, char const *argv[])
{

    LOG_INFO("Enter main!");

    int font_size = DEFAULT_FONT_SIZE;
    float angle = 0.0;
    // start point
    FT_Vector pen_lcd = {
        .x = FONT_PAGE_MARGIN_X,
        .y = FONT_PAGE_MARGIN_Y
    };

    switch (argc) {
        case 5:
            angle = atof(argv[4]);
        case 4:
            font_size = atoi(argv[3]);
        case 3:
            pen_lcd.y = (FT_Pos)atoi(argv[2]);
        case 2:
            pen_lcd.x = (FT_Pos)atoi(argv[1]);
            break;
        default:
            break;
    }
    LOG_INFO("origin lcd pos=(%ld, %ld), font_size= %d", pen_lcd.x, pen_lcd.y, font_size);

    fb_t fb;
    wchar_t chinese_str[48] = L"张笨笨Lisa永远爱孙帅帅Kline";


    fb_init(FB_PATH, &fb);
    fb_fill(&fb, fb.map_size, 0, RGB_COLOR_BLACK);

    FT_Library ft_lib = NULL;
    FT_Face ft_face = NULL;
    int err = freetype_init_local(&ft_lib, &ft_face, font_size);
    if (err != 0 || ft_lib == NULL || ft_face == NULL) {
        LOG_INFO("Init freetype failed!");
        fb_deinit(&fb);
        return EXCUTE_FAILED_EXIT;
    }

    FT_BBox str_box;
    bitmap_t b_map;
    err = get_strint_box(ft_face, chinese_str, LIST_LEN(chinese_str), &str_box);
    if (err != 0) {
        LOG_INFO("get string's box failed!");
        fb_deinit(&fb);
        return EXCUTE_FAILED_EXIT;
    }
    LOG_INFO("xMax=%ld, xMin=%ld, yMax=%ld, yMin=%ld", str_box.xMax, str_box.xMin, str_box.yMax, str_box.yMin);
    
    // convert to decare, and get new origin point
    FT_Vector pen_dcr_ft_64 = {
        .x = pen_lcd.x * 64 - str_box.xMin,
        .y = (fb.sc_var.yres - pen_lcd.y) * 64 - str_box.yMax
    };

    FT_GlyphSlot slot;
    FT_Matrix     matrix;
    // calculate angle matrix
    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

    for (int i = 0; i < sizeof(chinese_str); i++) {
        // set origin point for every glyph
        FT_Set_Transform(ft_face, &matrix, &pen_dcr_ft_64);

        err = FT_Load_Char(ft_face, chinese_str[i], FT_LOAD_RENDER);
        if (err != 0) {
            LOG_INFO("load char failed!");
            return FT_LOAD_CHAR_ERR;
        }

        // judge end of string
        if (chinese_str[i] == 0) {
            LOG_INFO("End of String!");
            break;
        }

        err = FT_Load_Char(ft_face, chinese_str[i], FT_LOAD_RENDER);
        if (err != 0) {
            LOG_INFO("Get slot failed!");
            break;
        }
        FT_GlyphSlot slot = ft_face->glyph;

        memcpy(&b_map, &slot->bitmap, sizeof(bitmap_t));
        FT_Vector draw_pen = {
            .x = slot->bitmap_left,
            .y = fb.sc_var.yres - slot->bitmap_top
        };

        // if length out of screen, draw to next line
        if (slot->bitmap_left + b_map.width > fb.sc_var.xres) {
            LOG_INFO("move to next line slot failed!");
            draw_pen.x = FONT_PAGE_MARGIN_X;
            draw_pen.y += FONT_LINE_MARGIN + (str_box.yMax / 64);
            pen_dcr_ft_64.x = draw_pen.x * 64 - str_box.xMin;
            pen_dcr_ft_64.y = (fb.sc_var.yres - draw_pen.y) * 64 - str_box.yMax;
        }
        // convert y to lcd 
        draw_bitmap(&fb, &b_map, &draw_pen, RGB_COLOR_RED);
        LOG_INFO("advance: %d, %d", slot->advance.x / 64, slot->advance.y / 64);

        // move to next char
        pen_dcr_ft_64.x += slot->advance.x;
        pen_dcr_ft_64.y += slot->advance.y;
        memset(&b_map, 0, sizeof(bitmap_t));
    }

    fb_deinit(&fb);
    return EXCUTE_SUCCESS_EXIT;
}
