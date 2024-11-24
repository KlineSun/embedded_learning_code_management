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


int main(int argc, char const *argv[])
{

    LOG_DEBUG("Enter main!");

    int font_size = 32;
    if (argc >= 2) {
        font_size = atoi(argv[1]);
        LOG_DEBUG("set font size: %d", font_size);
    }
    wchar_t chinese_str[10] = L"孙健元";
    LOG_DEBUG("length of a wchar: %d", sizeof(wchar_t));

    str2hex_print((void *)chinese_str, sizeof(chinese_str));


    fb_t fb;
    fb_init(FB_PATH, &fb);
    fb_fill(&fb, fb.map_size, 0, 0xff);

    FT_Library ft_lib;
    FT_Face ft_face;
    FT_GlyphSlot  ft_slot;

    int err = FT_Init_FreeType(&ft_lib);
    if (err != 0) {
        LOG_DEBUG("Init freetype library failed!");
        return -1;
    }

    err = FT_New_Face(ft_lib, FONT_DICTIONARY_PATH, 0, &ft_face);
    if (err == FT_Err_Unknown_File_Format) {
        LOG_DEBUG("Connot open font file!");
        return -1;
    } else if (err != 0) {
        LOG_DEBUG("Open Fonts failed!");
        return -1;
    }

    LOG_DEBUG("There is %ld faces in font file", ft_face->num_faces);

    err = FT_Set_Pixel_Sizes(ft_face, font_size, 0);
    if (err != 0) {
        LOG_DEBUG("FT_Set_Pixel_Sizes failed!");
        return -1;
    }

    /* FT_UInt glyph_index = FT_Get_Char_Index(ft_face, chinese_str[0]);
    if (glyph_index == 0) {
        LOG_DEBUG("Cannot get index of %x, result: %d", chinese_str[0], glyph_index);
        return -1;
    }

    LOG_DEBUG("Get index of %x, result: %d", chinese_str[0], glyph_index);

    err = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
    if (err != 0) {
        LOG_DEBUG("Get glyph of index[%d] failed, result: %d", glyph_index, err);
        return -1;
    } else if (ft_face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
       err =  FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
       if (err != FT_Err_Ok) {
            LOG_DEBUG("Get glyph of index[%d] failed, result: %d", glyph_index, err);
            return -1;
       }
    }*/


   err = FT_Load_Char(ft_face, chinese_str[0], FT_LOAD_RENDER);
   if (err != 0) {
        LOG_DEBUG("load char failed!");
        return -1;
   }

    // Now we have got the slot.
    ft_slot = ft_face->glyph;
    point_t pen = {
        .x = 200,
        .y = 100
    };
    bitmap_t b_map = {
        .width = ft_slot->bitmap.width,
        .height = ft_slot->bitmap.rows,
        .ln_bys = ft_slot->bitmap.pitch,
        .buf = ft_slot->bitmap.buffer
    };
    LOG_DEBUG("Bitmap size: %d*%d, bytes of per line: %d", b_map.width, b_map.height, b_map.ln_bys);
    draw_bitmap(&fb, &b_map, &pen, RGB_COLOR_GREEN);

    fb_deinit(&fb);
    return 0;
}
