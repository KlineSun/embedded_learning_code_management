#include <math.h>
#include "common_util.h"
#include "freetype_util.h"

FT_Library g_ft_lib = NULL;
FT_Face g_ft_face = NULL;
bool g_init_flag = false;

int freetype_init(int font_size)
{
    if (g_init_flag && g_ft_lib != NULL && g_ft_face != NULL) {
        LOG_INFO("Freetype already inited!");
        return INIT_LIBRARY_ERR;
    }

    int size = font_size <= 0 ? DEFAULT_FONT_SIZE : font_size;

    int err = FT_Init_FreeType(&g_ft_lib);
    if (err != 0) {
        LOG_INFO("Init freetype library failed!");
        return INIT_LIBRARY_ERR;
    }

    err = FT_New_Face(g_ft_lib, FONT_DICTIONARY_PATH, 0, &g_ft_face);
    if (err != 0) {
        LOG_INFO("Open Fonts failed!");
        return FT_OPEN_RESOURCE_ERR;
    }

    LOG_INFO("There is %ld faces in font file", g_ft_face->num_faces);

    err = FT_Set_Pixel_Sizes(g_ft_face, size, 0);
    if (err != 0) {
        LOG_INFO("FT_Set_Pixel_Sizes failed!");
        return FT_SET_SIZE_ERR;
    }

    g_init_flag = true;
    return FT_NO_ERR;
}

int get_freetype_bitmap(FT_Face face, ft_code_t code, FT_Bitmap *bp)
{
    if (bp == NULL || face == NULL || !code) {
        LOG_INFO("Invalid arguments!");
        return FT_INVALID_ARGUMENT;
    }

    int err = FT_Load_Char(face, code, FT_LOAD_RENDER);
    FT_GlyphSlot ft_slot = face->glyph;
    if (err != 0) {
        LOG_INFO("load char failed!");
        return FT_LOAD_CHAR_ERR;
    }


    if (ft_slot == NULL) {
        LOG_INFO("get freetype slot failed!");
        return FT_GET_SLOT_ERR;
    }

    memcpy(bp, &ft_slot->bitmap, sizeof(FT_Bitmap));
    // LOG_INFO("bitmap.width = %d, bitmap.rows = %d, ad_x = %d, ad_y = %d", ft_slot->bitmap.width, ft_slot->bitmap.rows,
    //                         ft_slot->advance.x, ft_slot->advance.y);

    return FT_NO_ERR;
}


int ft_rotate_transfer(float rota_angle, FT_Vector *pp)
{
    if (pp == NULL) {
        LOG_INFO("Invalid arguments!");
        return FT_INVALID_ARGUMENT;
    }
    FT_Matrix matrix;
    FT_Vector _p =  {
        .x = pp->x * 64,
        .y = pp->y * 64
    };
    float angle = rota_angle;
    double _angle = ANGLE(angle);

    if (!g_init_flag || g_ft_lib == NULL || g_ft_face == NULL) {
        LOG_INFO("Freetype not initialized yet!");
        return FT_USE_WITHOUT_INIT;
    }

    if (rota_angle > 360.0 || rota_angle < -360.0) {
        angle = fmod((float)rota_angle, 360.0);
    }
    matrix.xx = (FT_Fixed)(  cos(_angle)* 0x10000L  );
    matrix.xy = (FT_Fixed)( -sin(_angle )* 0x10000L );
    matrix.yx = (FT_Fixed)(  sin(_angle )* 0x10000L );
    matrix.yy = (FT_Fixed)(  cos(_angle)* 0x10000L  );


    FT_Set_Transform(g_ft_face, &matrix, &_p);

    return FT_NO_ERR;
}

void print_FT_Bitmap(FT_Bitmap *bitmap)
{
    unsigned int x = 0, y = 0;
    char *line_buf = NULL;
    if (!bitmap) {
        LOG_ERR("Invalid bitmap parameter!");
        return;
    }

    line_buf = malloc(bitmap->rows + 1); // 逐行输出，因此申请一行的内存即可
    if (!line_buf) {
        LOG_ERR("Alloc bitmap print memory failed!");
        return;
    }

    printf("bitmap(%d*%d):\n", bitmap->rows, bitmap->width);
    for (y = 0; y < bitmap->rows; y++) {
        for (x = 0; x < bitmap->width; x++) {
            if (bitmap->buffer[y * bitmap->width + x] != 0)
                line_buf[x] = 0x2a;
            else
                line_buf[x] = 0x2e;
        }
        line_buf[bitmap->rows] = '\0';
        printf("\t%s\n", line_buf);
        memset(line_buf, 0, bitmap->rows+1);
    }

    free(line_buf);
}

