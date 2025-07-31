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
#include "freetype/config/ftconfig.h"
#include <wchar.h>
#include "fb_util.h"
#include "freetype_util.h"


int main(int argc, char const *argv[])
{

    LOG_INFO("Enter main!");

    int font_size = DEFAULT_FONT_SIZE;
    fb_t fb;
    wchar_t chinese_str[15] = L"张笨笨永远爱孙帅帅";

    if (argc >= 2) {
        font_size = atoi(argv[1]);
        LOG_INFO("set font size: %d", font_size);
    }


    fb_init(FB_PATH, &fb);
    fb_fill(&fb, fb.map_size, 0, RGB_COLOR_BLACK);

    int err = freetype_init(font_size);
    if (err != 0) {
        LOG_INFO("Init freetype failed!");
        fb_deinit(&fb);
        return EXCUTE_FAILED_EXIT;
    }

    // start point
    point_t pen = {
        .x = 200,
        .y = 100
    };
    const int word_space = 10;
    bitmap_t b_map;

    for (int i = 0; i < sizeof(chinese_str); i++) {
        if (chinese_str[i] == 0) {
            continue;
        }

        ft_rotate_transfer(-60, &pen);

        err = get_freetype_bitmap(chinese_str[i], &b_map);
        if (err != 0 || b_map.buf == NULL) {
            LOG_INFO("Get bitmap failed!");
            break;
        }

        draw_bitmap(&fb, &b_map, &pen, RGB_COLOR_RED);
        pen.x += b_map.advance_x / 64;
        pen.y -= b_map.advance_y / 64;
    }

    fb_deinit(&fb);
    return EXCUTE_SUCCESS_EXIT;
}
