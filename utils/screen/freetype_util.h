#ifndef FREETYPE_UTIL_H
#define FREETYPE_UTIL_H

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_CONFIG_CONFIG_H

#define DEFAULT_FONT_SIZE (32)

#define FT_NO_ERR 0
#define INIT_LIBRARY_ERR 1
#define FT_OPEN_RESOURCE_ERR 2
#define FT_SET_SIZE_ERR 3
#define FT_USE_WITHOUT_INIT 4
#define FT_LOAD_CHAR_ERR 5
#define FT_GET_SLOT_ERR 6
#define FT_INVALID_ARGUMENT 99

typedef unsigned int ft_code_t;

int freetype_init(int font_size);
int get_freetype_bitmap(FT_Face face, ft_code_t code, FT_Bitmap *bp);
int ft_rotate_transfer(float rota_angle, FT_Vector *pp);
void print_FT_Bitmap(FT_Bitmap *bitmap);

#endif //FREETYPE_UTIL_H