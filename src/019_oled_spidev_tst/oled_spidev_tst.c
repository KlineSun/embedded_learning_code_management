#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include "font_8x16.h"
#include "common_util.h"
#include "gpio_util.h"

#define MAX_SPI_MSG_SIZE (4096)
#define OLED_XRES (128)
#define OLED_YRES (64)
#define MAX_CHAR_SHOW_COUNT ((OLED_XRES/8) * OLED_YRES)
#define OLED_CMD (0)
#define OLED_DATA (1)

#define OLED_COLUMNS (128)
#define OLED_PAGES (8)
#define OLED_MAX_FONT_SPACE ((OLED_XRES/FONT_WIDTH) * (OLED_YRES/FONT_HEIGHT))

typedef enum {
    PROC_SELF=0,
    DEV_PATH_IDX,
    GPIO_IDX,
    IDATA_IDX,
} spi_cmd_idx;

typedef enum {
    HORZ_ADDR_MODE,
    VERT_ADDR_MODE,
    PAGE_ADDR_MODE,
    UNKOWN_OPRT=-1,
} address_mode_t;

typedef struct{
    uint8_t **buf; //位图二维数组
    uint16_t font_count; // 位图中总字数
    uint16_t row_fonts; // 横向显示字符数
    uint16_t col_fonts; // 纵向字符数
    uint16_t width; // bits
    uint16_t height; // bits
    region_2d region; // region in oled, x means column, y means page.
} bitmap_t;

static uint16_t g_dc_gpio_num = 0;
static bitmap_t g_bitmap;
static int g_oled_fd = -1;

int oled_write_cmd(uint8_t cmd);
int oled_write_data(uint8_t *data, uint32_t size);
void oled_draw_position(region_2d *region, address_mode_t mode);

static uint16_t remain_char_space(int start_x, int start_y)
{
    if (start_x > OLED_XRES || start_y > OLED_YRES)
        return 0;

    return ((OLED_XRES - start_x) / 8) * (OLED_YRES - start_y);
}

int dc_pin_init()
{
    if (!is_gpio_num_valid(g_dc_gpio_num)) {
        LOG_INFO("gpio_num is invalid: %d", g_dc_gpio_num);
        return -1;
    }

    // 检查dc pin脚是否为push-pull输出
    if (stm32_get_gpio_output_type(g_dc_gpio_num) != PUSH_PULL_OUT) {
        stm32_set_gpio_output_type(g_dc_gpio_num, PUSH_PULL_OUT);
        LOG_INFO("set type of gpio%d as open-drain!", g_dc_gpio_num);
    }

    // export
    if (sys_gpio_export(g_dc_gpio_num) < 0) {
        LOG_INFO("export gpio%d failed!", g_dc_gpio_num);
        return -1;
    }
    
    // set output direction
    if (sys_gpio_set_direction(g_dc_gpio_num, GPIO_DIRECTION_OUT) < 0) {
        LOG_INFO("set gpio%d direction failed!", g_dc_gpio_num);
        return -1;
    }
    
    // set default value: high level
    if (sys_gpio_set_value(g_dc_gpio_num, 1) < 0) {
        LOG_INFO("set gpio%d value failed!", g_dc_gpio_num);
        return -1;
    }

    return 0;
}

int dc_pin_ctrl(uint8_t state)
{
    uint8_t rd_back = 0;
    if (sys_gpio_set_value(g_dc_gpio_num, state) < 0) {
        LOG_INFO("set gpio%d value failed!", g_dc_gpio_num);
        return -1;
    }

    if (sys_gpio_get_value(g_dc_gpio_num, &rd_back) < 0 || rd_back != state) {
        LOG_INFO("set gpio%d value exception: cmpare failed!", g_dc_gpio_num);
        return -1;
    }
    return 0;
}

void print_bitmap(bitmap_t *bitmap)
{
    uint16_t i = 0, j = 0; // 位图坐标
    uint16_t x = 0, y = 0; // 字节坐标
    char line_buf[OLED_XRES] = {0};
    int bit_offset = 0;
    if (!bitmap || !bitmap->buf) {
        LOG_ERR("Invalid parameter!");
        return;
    }

    // 遍历位图
    printf("bitmap as below: \n");
    for (j = 0; j < bitmap->height; j++) {
        for (i = 0; i < bitmap->width; i++) {
            // 找到字节坐标
            x = i;
            y = j / 8;
            bit_offset = (int)fmod(j, 8);

            // 位运算
            if (bitmap->buf[x][y] & (1 << bit_offset))
                line_buf[i] = 0x2a; // *号
            else
                line_buf[i] = 0x20; // 空格
        }
        printf("\t%s\n", line_buf);// 逐行打印位图
        memset(line_buf, 0, OLED_XRES);
    }
}

int bitmap_generate(const char * str, uint16_t start_x, uint16_t start_y, bitmap_t  *bitmap)
{
    uint16_t i = 0, j = 0, k = 0, bit = 0, bit_offset = 0;
    char ch = 0;
    uint16_t fill_start_x = 0, fill_start_y = 0;
    char str_buf[OLED_MAX_FONT_SPACE] = {0};

    if (!str || start_x >= OLED_XRES || start_y >= OLED_YRES || !bitmap || strlen(str) > OLED_MAX_FONT_SPACE) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    strcpy(str_buf, str);
    // 调试：将字符串拷贝到新的buf中，而不是使用传入的shell的buf，因为weitu中不会检查字符串的结束符，因此会打印出'\0'后面的内容
    LOG_INFO("Enter with string: %s, len=%d", str_buf, strlen(str_buf));
    memset(bitmap, 0, sizeof(*bitmap)); // clear
    bitmap->font_count = ((OLED_XRES-start_x) / FONT_WIDTH) * ((OLED_YRES-start_y) / FONT_HEIGHT); // 当前起始位置可容纳的字数
    if (bitmap->font_count < strlen(str_buf)) {
        LOG_INFO("No enough space in oled to show string, remain font space: %hu", bitmap->font_count);
        return -1;
    }
    bitmap->font_count = strlen(str_buf);
    bitmap->row_fonts = (OLED_XRES-start_x) / FONT_WIDTH; // 一行可容纳的字数
    if (bitmap->font_count < bitmap->row_fonts)
        bitmap->row_fonts = bitmap->font_count; // 字数不足一行的情况
    bitmap->width = bitmap->row_fonts * FONT_WIDTH;

    bitmap->col_fonts = bitmap->font_count / bitmap->row_fonts; // 有多少行字
    if (fmod(bitmap->font_count, bitmap->row_fonts) != 0.0) {
        // 有余数，新增一行显示
        bitmap->col_fonts++;
    }
    bitmap->height = bitmap->col_fonts * FONT_HEIGHT;

    // define region in oled
    bitmap->region.min_x = start_x;
    bitmap->region.min_y = start_y / 8; // page position
    bitmap->region.max_x = bitmap->region.min_x + bitmap->width - 1;
    bitmap->region.max_y = bitmap->region.min_y + bitmap->height/8 - 1; // page position
    LOG_INFO("Start at (%hu, %hu). row_fonts=%hu, col_fonts=%hu, font_count=%d",
                bitmap->region.min_x,
                bitmap->region.min_y,
                bitmap->row_fonts,
                bitmap->col_fonts,
                bitmap->font_count);
    LOG_INFO("Bitmap width=%hu, height=%hu!", bitmap->width, bitmap->height);

    /**
     * 创建二维数组
     * 
     *      宽度为width
     *      b0 b0 ... b0
     *      b1 b1 ... b1
     *      .  .      .
     *      .  .      .
     *      .  .      .
     *      b7 b7 ... b7
     *      b0 b0 ... b0
     *      b1 b1 ... b1
     *      .  .      .
     *      .  .      .
     *      .  .      .
     *      b7 b7 ... b7 高度为height
     *      ...
     * 
     * 位图结构如上，每个字节的数会被纵向描绘，
     * 
    */
    bitmap->buf = calloc(bitmap->width, sizeof(*bitmap->buf)); // 纵向描绘，每一列分配一个字符数组
    for (i = 0; i < bitmap->width; i++) {
        bitmap->buf[i] = calloc(bitmap->height/8, sizeof(**bitmap->buf)); // 每列的字符数组包含的字节数为bitmap->height/8
    }

    // fill bitmap
    for (j = 0; j < bitmap->col_fonts; j++) {
        for (i = 0; i < bitmap->row_fonts; i++) {
            // 遍历字符串中的每一个字符
            ch = str_buf[j * bitmap->row_fonts + i];

            // 计算出当前字符在位图buf中填充的位置，字节位置
            fill_start_x = i*FONT_WIDTH;
            fill_start_y = j*FONT_HEIGHT/8;
            LOG_INFO("Got '%c', fill start position(%hu, %hu)", ch, fill_start_x, fill_start_y);
            // 遍历字符的位图：fontdata_8x16[ch*16 + k];
            for (k = 0; k < FONT_HEIGHT; k++) {
                // 将原来的字符位图转化到新的位图里，也就是将横向byte转成纵向byte
                // LOG_INFO("before: 0x%x", fontdata_8x16[ch*16 + k]);
                for (bit = 7; bit > 0; bit--) {
                    if (fontdata_8x16[ch*16 + k] & (1 << bit)) {
                        // 将位图buf的对应位置置为1
                        bit_offset = (int)fmod(k, 8);
                        LOG_DEBUG("set buf[%hu][%hu] bit %hu",
                                    fill_start_x+7-bit, 
                                    fill_start_y+k/8,
                                    bit_offset);
                        bitmap->buf[fill_start_x+7-bit][fill_start_y+k/8] |= (1 << bit_offset);
                        // 调试：x方向的位移，始终都是(7-bit)，表示x方向距离最高位的偏移位置，加上fill_start_x就是填充位置
                    }
                }
            }

        }
    }

    print_bitmap(bitmap);
    LOG_INFO("End!");
    return 0;
}

int bitmap_display(bitmap_t *bitmap, address_mode_t mode)
{
    uint16_t i = 0, j = 0;
    uint8_t line_buf[OLED_XRES] = {0};
    region_2d region;
    if (!bitmap) {
        LOG_ERR("Invalid parameter!");
        return -1;
    }

    memset(&region, 0, sizeof(region_2d));
    // memcpy(&region, &bitmap->region, sizeof(region_2d)); // bak
    region.min_x = bitmap->region.min_x;
    region.min_y = bitmap->region.min_y;
    LOG_INFO("bitmap range: column(%d~%d), page(%d~%d), mode=%d",
                region.min_x, region.max_x, region.min_y, region.max_y, mode);

    // set mode
    oled_write_cmd(0x20);
    oled_write_cmd(mode);
    switch (mode)
    {
        case HORZ_ADDR_MODE:
        case PAGE_ADDR_MODE:
            region.max_x = bitmap->region.max_x;
            region.max_y = bitmap->region.min_y; //规划出一列空间
            // 逐行发送数据
            for (j = 0; j < bitmap->height/8; j++) {
                for (i = 0; i < bitmap->width; i++) {
                    line_buf[i] = bitmap->buf[i][j];
                }
                oled_draw_position(&region, mode);
                if (oled_write_data(line_buf, bitmap->width)) {
                    LOG_ERR("write data into oled device failed!");
                    return -1;
                }
                region.min_y++;
                region.max_y = region.min_y;
            }
            break;
        case VERT_ADDR_MODE:
            region.max_x = bitmap->region.min_x;
            region.max_y = bitmap->region.max_y; //规划出一行空间
            // 数据逐列发送到oled
            for  (i = 0; i < bitmap->width; i++) {
                // set position
                oled_draw_position(&region, mode);
                if (oled_write_data(bitmap->buf[i], bitmap->height/8)) {
                    LOG_ERR("write data into oled device failed!");
                    return -1;
                }
                region.min_x++;
                region.max_x = region.min_x;
            }
            break;
        default:
            break;
    }
    return 0;
}

void bitmap_destroy(bitmap_t *bitmap)
{
    int i = 0;
    if (!bitmap) {
        LOG_INFO("Invalid parameter!");
        return;
    }

    // LOG_INFO("Enter!");
    if (bitmap->buf) {
        for (i = 0; i < bitmap->width; i++) {
            if (bitmap->buf[i])
                free(bitmap->buf[i]);
        }
        free(bitmap->buf);
    }
    // LOG_INFO("End!");
}


/**
 * @brief Write data into oled device
 * 
 * @param buf Data want to write
 * @param size Length of data. If flag is OLED_CMD, the size is fixed at 1. It means that cmd must be sent one by one.
 * @param flag Data type: 0 means OLED_CMD. Need dc_pin_ctrl(0) before write.
 *                        1 means OLED_DATA.Need dc_pin_ctrl(1) before write.
 * @return Return 0 on success, or negative number on failure.
*/
int oled_wrire_common(uint8_t *buf, uint32_t size, uint8_t flag)
{
    int ret = 0, i = 0;
    if (!buf || size <= 0) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }
    if (g_oled_fd < 0) {
        LOG_INFO("Please open oled device first!");
        return -1;
    }

    if (flag == OLED_CMD) {
        dc_pin_ctrl(0);
        if (size > 1) {
            LOG_INFO("OLED device commands must be sent one by one!");
            return -1;
        }
        LOG_DEBUG("cmd: 0x%x\n", *buf);
    } else if (flag == OLED_DATA) {
        dc_pin_ctrl(1);
        LOG_DEBUG("data: ");
        for (i = 0; i < size; i++) {
            LOG_DEBUG("0x%x ", buf[i]);
        }
        LOG_DEBUG("\n");
    } else {
        LOG_INFO("Invalid flag: %d", flag);
        return -1;
    }

    ret = write(g_oled_fd, buf, size);
    if (ret < size) {
        LOG_INFO("write oled device failed: ret=%d, error: %s", ret, strerror(errno));
        return -1;
    }

    return 0;
}

int oled_write_cmd(uint8_t cmd)
{
    uint8_t val = cmd;
    return oled_wrire_common(&val, 1, OLED_CMD);
}

int oled_write_data(uint8_t *data, uint32_t size)
{
    return oled_wrire_common(data, size, OLED_DATA);
}

/**
 * @brief set draw position
 * 
 * @param region the region in oled, x means columns，y means pages.
 * @param mode the addressing mode.
 * 
*/
void oled_draw_position(region_2d *region, address_mode_t mode)
{
    if (region->min_x < 0 || region->min_y < 0 || region->max_x > OLED_XRES || region->max_y > OLED_YRES/8) {
        LOG_INFO("Invalud region parameters!");
        return;
    }

    LOG_INFO("draw region: col(%d~%d), page(%d~%d).", region->min_x, region->max_x, region->min_y, region->max_y);

    switch (mode)
    {
        case HORZ_ADDR_MODE:
        case VERT_ADDR_MODE:
                // set column start and end
            oled_write_cmd(0x21);
            oled_write_cmd(region->min_x & 0x7f);
            oled_write_cmd(region->max_x & 0x7f);
            // set page start and page end
            oled_write_cmd(0x22);
            oled_write_cmd(region->min_y & 0x3);
            oled_write_cmd(region->max_y & 0x3);
            break;
        case PAGE_ADDR_MODE:
            // page start and column start
            oled_write_cmd(0xb0+region->min_y); // set page
            oled_write_cmd(region->min_x & 0x0f); // set lower 4 bit of column position
            oled_write_cmd(((region->min_x&0xf0)>> 4) | 0x10); // set higher 4 bit of column position
            // 调试：高四位数据需要再或上0x10
            break;
        default:
            break;
    }

}

int oled_clear()
{
    int i = 0;
    uint8_t buf[OLED_COLUMNS] = {0};
    region_2d region;

    memset(&region, 0, sizeof(region_2d));
    region.min_x = 0;
    region.min_y = 0;
    region.max_x = OLED_XRES - 1;
    region.max_y = OLED_YRES / 8 - 1; // page position
    // set zero for each page
    for (i = 0; i < OLED_PAGES; i++) {
        region.min_y = i;
        oled_draw_position(&region, PAGE_ADDR_MODE);
        if (oled_write_data(buf, OLED_XRES)) {
            LOG_ERR("write clear data into oled device failed!");
            return -1;
        }
    }
    return 0;
}

// void oled_init()
// {
//     LOG_INFO("Enter!");

//     // Set Display Clock
//     oled_write_cmd(0xD5);
//     oled_write_cmd(0x80);

//     // Set Multiplex Ratio
//     oled_write_cmd(0xA8);
//     oled_write_cmd(0x3F);

//     // Set Display Offset
//     oled_write_cmd(0xD3);
//     oled_write_cmd(0x00);

//     // Set Display Start Line
//     oled_write_cmd(0x40);

//     // Set Charge Pump
//     oled_write_cmd(0x8D);
//     oled_write_cmd(0x10);

//     // Set Segment Re-Map
//     oled_write_cmd(0xA1);

//     // Set COM Output Scan Direction
//     oled_write_cmd(0xC8);

//     // Set COM Pins Hardware Configuration
//     oled_write_cmd(0xDA);
//     oled_write_cmd(0x12);

//     // Set Contrast Control
//     oled_write_cmd(0x81);
//     oled_write_cmd(0x66);

//     // Set Pre-Charge Period
//     oled_write_cmd(0xD9);
//     oled_write_cmd(0x22);

//     // Set VCOMH Deselect Level
//     oled_write_cmd(0xDB);
//     oled_write_cmd(0x30);

//     // Set Entire Display On/Off
//     oled_write_cmd(0xA4);
    
//     // Set Normal/Inverse Display
//     oled_write_cmd(0xA6);

//     // clear screen
//     if (oled_clear()) {
//         LOG_INFO("Clear oled failed!");
//         // handle
//     }

//     // Set Display On
//     oled_write_cmd(0xAF);

//     // 100ms Delay Recommended
//     usleep(100 * 1000);

//     LOG_INFO("End!");
// }

// 调试：使用datasheet中给出的上电顺序无法正确地初始化，这里使用的是裸机程序中的初始化函数
void oled_init()
{
    LOG_INFO("Enter!");
	oled_write_cmd(0xae);//关闭显示

	oled_write_cmd(0x00);//设置 lower column address
	oled_write_cmd(0x10);//设置 higher column address

	oled_write_cmd(0x40);//设置 display start line

	oled_write_cmd(0xB0);//设置page address

	oled_write_cmd(0x81);// contract control
	oled_write_cmd(0x66);//128

	oled_write_cmd(0xa1);//设置 segment remap

	oled_write_cmd(0xa6);//normal /reverse

	oled_write_cmd(0xa8);//multiple ratio
	oled_write_cmd(0x3f);//duty = 1/64

	oled_write_cmd(0xc8);//com scan direction

	oled_write_cmd(0xd3);//set displat offset
	oled_write_cmd(0x00);//

	oled_write_cmd(0xd5);//set osc division
	oled_write_cmd(0x80);//

	oled_write_cmd(0xd9);//ser pre-charge period
	oled_write_cmd(0x1f);//

	oled_write_cmd(0xda);//set com pins
	oled_write_cmd(0x12);//

	oled_write_cmd(0xdb);//set vcomh
	oled_write_cmd(0x30);//

	oled_write_cmd(0x8d);//set charge pump disable 
	oled_write_cmd(0x14);//

    oled_write_cmd(0x20);
    oled_write_cmd(0x2); // set Page Addressing Mode

    oled_clear(); // clear display
	oled_write_cmd(0xaf);//set dispkay on

    LOG_INFO("End!");
}

void oled_power_down()
{
    // Set Display On
    oled_write_cmd(0xAE);

    // 100ms Delay Recommended
    usleep(100 * 1000);
}

void print_usage()
{
    printf("usage: ./oled_spidev_tst <path> <gpio_num> <show_data> [-px%%dy%%d] [-mH/V/P] [-d%%d]");
    printf("path: path of spi dev.\n");
    printf("gpio_num: gpio number of DC pin, max number is %d\n", STM32_GPIOx_NUM_MAX);
    printf("show_data: the data you want show in oled, length no more than %d.\n", MAX_CHAR_SHOW_COUNT);
    printf("-px%%dy%%d: start position, x no more than %d, y no more than %d. Default positon is x0y0.\n",
            OLED_XRES, OLED_YRES);
    printf("-m: the addressing mode of oled, as below:\n");
    printf("\tH=Horizontal addressing mode\n");
    printf("\tV=Vertical addressing mode\n");
    printf("\tP=Page addressing mode\n");
    printf("-d: the delayt time after drawn. Unit is second.\n");
}

/**
 * 
 * usage:
 *  ./oled_spidev_tst <path> <gpio_num> <show_data> [-px%%dy%%d] [-mH/V/P] [-d%%d]
 * 
 * path：表示要操作的spi设备的路径；
 * gpio_num：表示DC引脚的gpio号
 * show_data：表示要在oled上显示的数据
 * -px%%dy%%d：表示显示的初始位置，默认值为x0y0
 * -mH/V/P：表示oled使用的寻址方式，默认为Horizontal addressing mode
 * -d%%d：显示的时间，单位为秒
 * 
 *  exp:
 *  ./oled_spidev_tst /dev/spidev0.2 56 Kline.Sun
 *  ./oled_spidev_tst /dev/spidev0.2 56 Kline.Sun -px8y16 -mH -d300
 * 
*/
int main(int argc, const char **argv)
{
    uint16_t start_x = 0, start_y = 0, data_len;
    address_mode_t mode = PAGE_ADDR_MODE;
    int delay = 60;
    bool is_success = false;
    uint8_t extend_idx = IDATA_IDX + 1;

    if (argc < IDATA_IDX + 1) {
        LOG_INFO("Arguments is too few, no less than %d.", IDATA_IDX + 1);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    g_dc_gpio_num = (uint16_t)strtoul(argv[GPIO_IDX], NULL, 0);
    if (g_dc_gpio_num > STM32_GPIOx_NUM_MAX) {
        LOG_INFO("Gpio number beyond the limit: %d.", g_dc_gpio_num);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    data_len = strlen(argv[IDATA_IDX]);
    if (data_len > MAX_CHAR_SHOW_COUNT) {
        LOG_INFO("Gpio number beyond the limit: %d.", g_dc_gpio_num);
        print_usage();
        return EXCUTE_FAILED_EXIT;
    }

    // 浏览拓展option
    while (extend_idx <= argc - 1) {
        if (strstr(argv[extend_idx], "-px")) {
            // position
            if (sscanf(argv[extend_idx], "-px%huy%hu", &start_x, &start_y) < 2) {
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
                start_x = 0;
                start_y = 0;
            }

            if (start_x > OLED_XRES || start_y > OLED_YRES
                || remain_char_space(start_x, start_y) < data_len) {
                print_usage();
                LOG_INFO("positon is out of the screen: (%d, %d), data len=%d", start_x, start_y, data_len);
                return EXCUTE_FAILED_EXIT;
            }
        } else if (strstr(argv[extend_idx], "-m")) {
            // addressing mode
            if (!strcmp(argv[extend_idx], "-mH"))
                mode = HORZ_ADDR_MODE;
            else if (!strcmp(argv[extend_idx], "-mV"))
                mode = VERT_ADDR_MODE;
            else if (!strcmp(argv[extend_idx], "-mP"))
                mode = PAGE_ADDR_MODE;
            else
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
        } else if (strstr(argv[extend_idx], "-d")) {
            if (sscanf(argv[extend_idx], "-d%d", &delay) < 1) {
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
                delay = 60;
            }
        } else {
            print_usage();
            LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
        }
        extend_idx++;
    }

    LOG_INFO("Enter: dc_gpio_num=%hu, data_len=%hu, position=(%hu, %hu), mode=%d, delay=%d",
                      g_dc_gpio_num, data_len, start_x, start_y, mode, delay);

    // dc pin init
    if (dc_pin_init()) {
        LOG_ERR("Init gpio%d failed!", g_dc_gpio_num);
        return EXCUTE_FAILED_EXIT;
    }

    // generate bitmap
    if (bitmap_generate(argv[IDATA_IDX], start_x, start_y, &g_bitmap) != 0) {
        LOG_ERR("generate bitmap failed!");
        return EXCUTE_FAILED_EXIT;
    }

    // open oled device
    g_oled_fd = open(argv[DEV_PATH_IDX], O_WRONLY);
    if (g_oled_fd < 0) {
        LOG_ERR("Open oled device failed!");
        goto destroy_bitmap;
    }

    // init oled device
    oled_init();

    // show bitmap in oled
    if (bitmap_display(&g_bitmap, mode)) {
        LOG_ERR("Open oled device failed!");
        goto oled_off;
    }
    sleep(delay);

oled_off:
    oled_init();
    oled_power_down();
// close_fd:
    if (g_oled_fd > 0)
        close(g_oled_fd);
destroy_bitmap:
    bitmap_destroy(&g_bitmap);

    return is_success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
