#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include "common_util.h"
#include "fb_util.h"
#include "freetype_util.h"

#define MAX_SPI_MSG_SIZE (4096)
#define OLED_XRES (128)
#define OLED_YRES (64)

typedef enum {
    PROC_SELF=0,
    DEV_PATH_IDX,
} spi_cmd_idx;

int get_string_box(FT_Face face, wchar_t *str, int len, FT_BBox *box)
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
            return -1;
        }

        ft_slot = face->glyph;
        err = FT_Get_Glyph(ft_slot, &glyph);
        if (err != 0) {
            LOG_INFO("get glyph from slot failed!");
            return -1;
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


void print_usage()
{
    printf("usage: ./oled_fb_tst <path> [-px%%d-y%%d] [-size%%d] [-angel%%d]");
    printf("path: path of framebuffer dev.\n");
    printf("-px%%d-y%%d: start position.\n");
    printf("-size%%d: the size of the font, default is %d.\n", DEFAULT_FONT_SIZE);
    printf("-angel%%d: the rotate angle.\n");
}

/**
 * 
 * usage:
 *  ./oled_fb_tst <path> [-px%%d-y%%d] [-size%%d] [-angel%%d]
 * 
 * path：表示要操作的spi设备的路径；
 * -size%%d：表示要设置的字体大小
 * -angel%%d：表示要使字体旋转的角度
 * 
 *  exp:
 *  ./oled_fb_tst /dev/fb1
 *  ./oled_fb_tst /dev/fb1 -px8y16 -size32 -angle30
 * 
*/
int main(int argc, const char **argv)
{
    int font_size = DEFAULT_FONT_SIZE, angle = 0, start_x = 0, start_y = 0;
    int extend_idx = DEV_PATH_IDX + 1;
    bool is_success = false;
    fb_t fb;
    wchar_t chinese_str[64] = L"Kline.Sun";
    FT_Library ft_lib = NULL;
    FT_Face ft_face = NULL;
    int err = 0;
    FT_BBox str_box;
    FT_Bitmap b_map;
    FT_Vector pen_lcd = {
        .x = FONT_PAGE_MARGIN_X,
        .y = FONT_PAGE_MARGIN_Y
    };
    FT_Matrix     matrix;
    FT_GlyphSlot slot = NULL;
    FT_Vector draw_pen, pen_dcr_ft_64;

    if (argc < DEV_PATH_IDX + 1) {
        LOG_INFO("Arguments is too few, no less than %d.", DEV_PATH_IDX + 1);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    // 浏览拓展option
    while (extend_idx <= argc - 1) {
        if (strstr(argv[extend_idx], "-px")) {
            // position
            if (sscanf(argv[extend_idx], "-px%dy%d", &start_x, &start_y) < 1) {
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
                start_x = 0;
                start_y = 0;
            }

            if (start_x > OLED_XRES || start_y > OLED_YRES) {
                print_usage();
                LOG_INFO("positon is out of the screen: (%d, %d)", start_x, start_y);
                return EXCUTE_FAILED_EXIT;
            }
        } else if (strstr(argv[extend_idx], "-size")) {
            if (sscanf(argv[extend_idx], "-size%d", &font_size) < 1) {
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
                font_size = DEFAULT_FONT_SIZE;
            }

            if (font_size > OLED_YRES) {
                print_usage();
                LOG_INFO("Font size is out of the screen: %d", font_size);
                return EXCUTE_FAILED_EXIT;
            }
        } else if (strstr(argv[extend_idx], "-angle")) {
            if (sscanf(argv[extend_idx], "-angle%d", &angle) < 1) {
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
                angle = 0;
            }
        } else {
            print_usage();
            LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
        }
        extend_idx++;
    }

    LOG_INFO("Enter: position=(%d, %d), font_size=%d, angle=%d",
                      start_x, start_y, font_size, angle);

    // 初始化fb
    if (fb_init(argv[DEV_PATH_IDX], &fb)) {
        LOG_ERR("framebuffer init failed!");
        return EXCUTE_FAILED_EXIT;
    }
    fb_fill(&fb, fb.map_size, 0, RGB_COLOR_BLACK);

    pen_lcd.x = start_x;
    pen_lcd.y = start_y;
    if (FT_Init_FreeType(&ft_lib)) {
        LOG_INFO("Init freetype library failed!");
        return EXCUTE_FAILED_EXIT;
    }

    if (FT_New_Face(ft_lib, FONT_DICTIONARY_PATH, 0, &ft_face)) {
        LOG_INFO("Open %s failed!", FONT_DICTIONARY_PATH);
        goto ft_lib_free;
    }
    LOG_INFO("There is %ld faces in font file", ft_face->num_faces);

    if (FT_Set_Pixel_Sizes(ft_face, font_size, 0)) {
        LOG_INFO("FT_Set_Pixel_Sizes failed!");
        return FT_SET_SIZE_ERR;
    }

    err = get_string_box(ft_face, chinese_str, LIST_LEN(chinese_str), &str_box);
    if (err != 0) {
        LOG_INFO("get string's box failed!");
        fb_deinit(&fb);
        return EXCUTE_FAILED_EXIT;
    }
    // LOG_INFO("xMax=%ld, xMin=%ld, yMax=%ld, yMin=%ld", str_box.xMax, str_box.xMin, str_box.yMax, str_box.yMin);

    // convert to decare, and get new origin point
    pen_dcr_ft_64.x = pen_lcd.x * 64 - str_box.xMin;
    pen_dcr_ft_64.y = (fb.sc_var.yres - pen_lcd.y) * 64 - str_box.yMax;

    // calculate angle matrix
    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

    for (int i = 0; i < sizeof(chinese_str); i++) {
        // judge end of string
        if (chinese_str[i] == 0) {
            LOG_INFO("End of String!");
            break;
        }
        // set origin point for every glyph
        FT_Set_Transform(ft_face, &matrix, &pen_dcr_ft_64);

        // err = FT_Load_Char(ft_face, chinese_str[i], FT_LOAD_RENDER);
        // if (err != 0) {
        //     LOG_INFO("load char failed!");
        //     goto fb_deinit;
        // }

        err = FT_Load_Char(ft_face, chinese_str[i], FT_LOAD_RENDER);
        if (err != 0) {
            LOG_INFO("Get slot failed!");
            goto fb_deinit;
        }
        slot = ft_face->glyph;

        memcpy(&b_map, &slot->bitmap, sizeof(FT_Bitmap));
        draw_pen.x = slot->bitmap_left;
        draw_pen.y = fb.sc_var.yres - slot->bitmap_top;

        // if length out of screen, draw to next line
        if (slot->bitmap_left + b_map.width > fb.sc_var.xres) {
            LOG_INFO("move to next line slot!");
            draw_pen.x = FONT_PAGE_MARGIN_X;
            draw_pen.y += FONT_LINE_MARGIN + (str_box.yMax / 64);
            pen_lcd.x = 0;
            pen_lcd.y += (str_box.yMax - str_box.yMin); // 加上box最大的高度
            pen_dcr_ft_64.x = draw_pen.x * 64 - str_box.xMin;
            pen_dcr_ft_64.y = (fb.sc_var.yres - draw_pen.y) * 64 - str_box.yMax;
        }
        // convert y to lcd 
        draw_bitmap(&fb, &b_map, &pen_lcd, RGB_COLOR_RED);
        LOG_INFO("advance: %ld, %ld", slot->advance.x / 64, slot->advance.y / 64);

        // move to next char
        pen_dcr_ft_64.x += slot->advance.x;
        pen_dcr_ft_64.y += slot->advance.y;
        pen_lcd.x += slot->advance.x / 64;
        pen_lcd.y += slot->advance.y / 64;
        memset(&b_map, 0, sizeof(FT_Bitmap));
    }

    print_fb(&fb);
    is_success = true;
    usleep(1);

ft_lib_free:
    FT_Done_Library(ft_lib);

fb_deinit:
    if (fb.map_ptr)
        fb_deinit(&fb);
    return is_success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
