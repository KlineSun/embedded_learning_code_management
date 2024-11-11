#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#define FB_PATH "/dev/fb0"

#ifdef HZK16_RESOURCE_SUPPORT
    #define HZK_PATH "/home/res/HZK16"
    #define ASCII_RESERVED (0xa1)
    #define AREA_CHINESE_CNT (0x5e)
#elif define HZK12_RESOURCE_SUPPORT
    #define HZK_PATH "/home/res/HZK12"
    #define ASCII_RESERVED (0xa1)
    #define AREA_CHINESE_CNT (0x5e)
#endif

#define CHINESE_BYTES (2)
#define CHINESE_BITMAP_WIDTH (16)
#define CHINESE_BITMAP_HEIGHT (16)
#define CHINESE_BITMAP_BYTES (32)
#define CHINESE_WORD_SPACE (5)
#define CHINESE_LINE_SPACE (10)

#define COLOR_WHITE (0xffffff)
#define COLOR_RED (0xff0000)
#define COLOR_GREEN (0x00ff00)
#define COLOR_VLUE (0x0000ff)

#endif // FRAME_BUFFER_H