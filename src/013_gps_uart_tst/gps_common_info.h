#ifndef GPS_COMMON_INFO_H
#define GPS_COMMON_INFO_H

#include <time.h>

#define MAX_GPS_RAW_DATA_LEN    (2048)
#define DIFF_ID_LEN             (4)
#define MAX_INFO_LIST_LEN       (30)

typedef enum {
    GPS_INVALID_TYPE=-1,
    GPGGA=0, /* 定位数据 */
    GPRMC,  /* 推荐最小定位信息 */
    GPGSV, /* 可见卫星信息 */
    GPGSA, /* 卫星状态及精度因子 */
    GPVTG /* 地面速度及航向 */
} gps_info_type;

typedef struct {
    struct tm tv;
    long tm_ms;
} gps_time_t;

typedef struct gps_gpgga_info
{
    gps_info_type t;
    gps_time_t time;
    double lati;
    char lati_hemi;
    double longi;
    char longi_hemi;
    int quality;
    int sate_cnt;
    float hdop; 
    float elevation;
    char elev_unit;
    float plane_height;
    char height_unit;
    float diff_age;
    char  diff_id[DIFF_ID_LEN];
    unsigned short checksum;
} gpgga_info_t;

typedef struct gps_gprmc_info
{
    gps_info_type t;
    unsigned short checksum;
    struct gps_gprmc_info *next;
} gprmc_info_t;

typedef struct gps_gpgsv_info
{
    gps_info_type t;
    unsigned short checksum;
    struct gps_gpgsv_info *next;
} gpgsv_info_t;

typedef struct gps_gpgsa_info
{
    gps_info_type t;
    unsigned short checksum;
    struct gps_gpgsa_info *next;
} gpgsa_info_t;

typedef struct gps_gpvtg_info
{
    gps_info_type t;
    unsigned short checksum;
    struct gps_gpvtg_info *next;
} gpvtg_info_t;


typedef struct 
{
    /**
     * valid_byte & (1 << GPGGA) == 1: gpgga is valid;
     * valid_byte & (1 << GPRMC) == 1: gprmc is valid;
     * valid_byte & (1 << GPGSV) == 1: gpgsv is valid;
     * valid_byte & (1 << GPGSA) == 1: gpgsa is valid;
     * valid_byte & (1 << GPVTG) == 1: gpvtg is valid;
    */
    char valid_byte;
    gpgga_info_t    *gpgga;
    gprmc_info_t    *gprmc;
    gpgsv_info_t    *gpgsv;
    gpgsa_info_t    *gpgsa;
    gpvtg_info_t    *gpvtg;
} gps_common_info;

gps_info_type parse_gps_raw_data(const char *raw_data, gps_common_info *result);

#endif //GPS_COMMON_INFO_H
