#include "ft2build.h"
#include "freetype/freetype.h"
#include "freetype/config/ftconfig.h"
#include "common_util.h"
#include "fb_util.h"
#include "freetype_util.h"
#include <math.h>


FT_Library g_ft_lib = NULL;
FT_Face g_ft_face = NULL;
bool g_init_flag = false;

int freetype_init(int font_size)
{
    if (g_init_flag && g_ft_lib != NULL && g_ft_face != NULL) {
        LOG_DEBUG("Freetype already inited!");
        return INIT_LIBRARY_ERR;
    }

    int size = font_size <= 0 ? DEFAULT_FONT_SIZE : font_size;

    int err = FT_Init_FreeType(&g_ft_lib);
    if (err != 0) {
        LOG_DEBUG("Init freetype library failed!");
        return INIT_LIBRARY_ERR;
    }

    err = FT_New_Face(g_ft_lib, FONT_DICTIONARY_PATH, 0, &g_ft_face);
    if (err != 0) {
        LOG_DEBUG("Open Fonts failed!");
        return FT_OPEN_RESOURCE_ERR;
    }

    LOG_DEBUG("There is %ld faces in font file", g_ft_face->num_faces);

    err = FT_Set_Pixel_Sizes(g_ft_face, size, 0);
    if (err != 0) {
        LOG_DEBUG("FT_Set_Pixel_Sizes failed!");
        return FT_SET_SIZE_ERR;
    }

    g_init_flag = true;
    return FT_NO_ERR;
}

int get_freetype_bitmap(ft_code_t code, bitmap_t *bp)
{
    if (bp == NULL) {
        LOG_DEBUG("Invalid arguments!");
        return FT_INVALID_ARGUMENT;
    }

    if (!g_init_flag || g_ft_lib == NULL || g_ft_face == NULL) {
        LOG_DEBUG("Freetype not initialized yet!");
        return FT_USE_WITHOUT_INIT;
    }
    if (!code)
        LOG_DEBUG("null code enter!");

    int err = FT_Load_Char(g_ft_face, code, FT_LOAD_RENDER);
    if (err != 0) {
        LOG_DEBUG("load char failed!");
        return FT_LOAD_CHAR_ERR;
    }

    if (!code)
        LOG_DEBUG("null code FT_Load_Char!");

    FT_GlyphSlot ft_slot = g_ft_face->glyph;
    if (ft_slot == NULL) {
        LOG_DEBUG("get freetype slot failed!");
        return FT_GET_SLOT_ERR;
    }

    bp->width = ft_slot->bitmap.width;
    bp->height = ft_slot->bitmap.rows;
    bp->ln_bys = ft_slot->bitmap.pitch;
    bp->advance_x = ft_slot->advance.x;
    bp->advance_y = ft_slot->advance.y;
    bp->buf = ft_slot->bitmap.buffer;
    LOG_DEBUG("bitmap.width = %d, bitmap.rows = %d, ad_x = %d, ad_y = %d", ft_slot->bitmap.width, ft_slot->bitmap.rows,
                            ft_slot->advance.x, ft_slot->advance.y);

    if (!code)
        LOG_DEBUG("null code return bitmap buf: %p", bp->buf);

    return FT_NO_ERR;
}


int ft_rotate_transfer(float rota_angle, point_t *pp)
{
    if (pp == NULL) {
        LOG_DEBUG("Invalid arguments!");
        return FT_INVALID_ARGUMENT;
    }

    if (!g_init_flag || g_ft_lib == NULL || g_ft_face == NULL) {
        LOG_DEBUG("Freetype not initialized yet!");
        return FT_USE_WITHOUT_INIT;
    }

    float angle = rota_angle;
    if (rota_angle > 360.0 || rota_angle < -360.0) {
        angle = fmod((float)rota_angle, 360.0);
    }

    double _angle = ANGLE(angle);
    FT_GlyphSlot ft_slot = g_ft_face->glyph;
    FT_Matrix matrix;

    point_t _p =  {
        .x = pp->x * 64,
        .y = pp->y * 64
    };

    matrix.xx = (FT_Fixed)(  cos(_angle)* 0x10000L  );
    matrix.xy = (FT_Fixed)( -sin(_angle )* 0x10000L );
    matrix.yx = (FT_Fixed)(  sin(_angle )* 0x10000L );
    matrix.yy = (FT_Fixed)(  cos(_angle)* 0x10000L  );


    FT_Set_Transform(g_ft_face, &matrix, &_p);

    return FT_NO_ERR;
}

