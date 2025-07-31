#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "gps_common_info.h"
#include "common_util.h"
#include <math.h>

int parse_time_string(const char *time_str, const char *format, gps_time_t *t)
{
    if (!time_str || !format || !t) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    struct tm tv;
    char ch_slot[4] = {0};
    if (!strcmp(format, "YYYY-MM-DD hh:mm:ss")) {
        LOG_INFO("just skip!");
    } else if (!strcmp(format, "hhmmss.ss")) {
        // get hh
        strncpy(ch_slot, time_str, 2);
        t->tv.tm_hour = atoi(ch_slot);

        // get mm
        memset(ch_slot, 0, 8);
        strncpy(ch_slot, time_str + 2, 2);
        t->tv.tm_min = atoi(ch_slot);

        // get ss
        memset(ch_slot, 0, 8);
        strncpy(ch_slot, time_str + 4, 2);
        t->tv.tm_sec = atoi(ch_slot);

        // get .ss
        memset(ch_slot, 0, 8);
        strncpy(ch_slot, time_str + 7, 8);
        t->tm_ms = atoi(ch_slot);

        LOG_INFO("gps time: %d:%d:%:%d.%d", t->tv.tm_hour, t->tv.tm_min, t->tv.tm_sec, t->tm_ms);
    } else {
        LOG_INFO("Unsupport time format!");
        return -1;
    }

}

int minute_to_degree(double *min_data, double *degree_result, double integer_weight) {

    if (!min_data || !degree_result) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (integer_weight <= 0 || (integer_weight != 1 && fmod(integer_weight, 10.0) != 0)) {
        LOG_INFO("Invalid weight!");
        return -1;
    }

    double integer = *min_data / integer_weight;
    double decimal = fmod(*min_data, integer_weight);
    LOG_INFO("integer: %lf, decimal: %lf", integer, decimal);
    if (decimal > 60.0) {
        LOG_INFO("Invalid decimal, greater than 60.0!");
        return -1;
    }

    *degree_result = integer + integer_weight * (decimal / 60.0);
    LOG_INFO("convert minute: %lf to degree: %lf", *min_data, *degree_result);
    return 0;
}

static int gpgga_info_parser(const char* raw_data, gpgga_info_t *result)
{
    if (!raw_data || !result || strlen(raw_data) > MAX_GPS_RAW_DATA_LEN) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    // const char *gpgga1 = "$GPGGA,085412.00,3150.7821,N,11711.9339,E,1,08,1.2,56.4,M,-34.8,M,01.2,0000*76";
    // const char *gpgga2 = "$GPGGA,085512.00,3150.8021,N,11711.9539,E,1,08,1.2,56.4,M,-34.8,M,01.2,0000*77";

    LOG_INFO("Enter!");

    // match
    // "$GPGGA,085412.00,3150.7821,N,11711.9339,E,1,08,1.2,56.4,M,-34.8,M,01.2,0000*76";
    char time_str[16] = {0};
    const char *gpgga_format = "$GPGGA,%[^,],%lf,%c,%lf,%c,%d,%d,%f,%f,%c,%f,%c,%f,%[^*]*%d";
    int ret = sscanf(raw_data, gpgga_format,
        time_str,
        &(result->lati),
        &(result->lati_hemi),
        &(result->longi),
        &(result->longi_hemi),
        &(result->quality),
        &(result->sate_cnt),
        &(result->hdop),
        &(result->elevation),
        &(result->elev_unit),
        &(result->plane_height),
        &(result->height_unit),
        &(result->diff_age),
        result->diff_id,
        &(result->checksum)
    );

    if (ret != 15) {
        LOG_INFO("Match gpgga attributes failed, matched items count: %d", ret);
        return -1;
    }

    LOG_INFO("Match success!");

    

    if (minute_to_degree(&(result->lati), &(result->lati), 100.0)
        || minute_to_degree(&(result->longi), &(result->longi), 100.0)) {
        LOG_INFO("convert minute data to degree failed!");
        return -1;
    }

    LOG_INFO("location on lati: %lf %c, longi: %lf %c", result->lati, result->lati_hemi, result->longi, result->longi_hemi);

    return 0;
}

static int gprmc_info_parser(const char* raw_data, gprmc_info_t *result)
{
    if (!raw_data || !result) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    result->t = GPRMC;
    return 0;
}

static int gpgsv_info_parser(const char* raw_data, gpgsv_info_t *result)
{
    if (!raw_data || !result) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    result->t = GPGSV;
    return 0;
}

static int gpgsa_info_parser(const char* raw_data, gpgsa_info_t *result)
{
    if (!raw_data || !result) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    result->t = GPGSA;
    return 0;
}

static int gpvtg_info_parser(const char* raw_data, gpvtg_info_t *result)
{
    if (!raw_data || !result) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    result->t = GPVTG;
    return 0;
}


gps_info_type parse_gps_raw_data(const char *raw_data, gps_common_info *result)
{
    if (!raw_data || !result) {
        LOG_INFO("Invalid paramter!");
        return GPS_INVALID_TYPE;
    }

    void *parse_result = NULL;
    gps_info_type t;
    if (!strncmp(raw_data, "$GPGGA,", strlen("$GPGGA,"))) {
        parse_result = (gpgga_info_t *)malloc(sizeof(gpgga_info_t));
        if (!parse_result)
            goto malloc_error;

        //parse
        if (gpgga_info_parser(raw_data, parse_result) != 0)
            goto parse_error;

        t = GPGGA;
        result->valid_byte |= (1 << GPGGA);
    } else if (!strncmp(raw_data, "$GPRMC,", strlen("$GPRMC,"))) {
        parse_result = (gprmc_info_t *)malloc(sizeof(gprmc_info_t));
        if (!parse_result)
            goto malloc_error;

        //parse
        if (gprmc_info_parser(raw_data, parse_result) != 0)
            goto parse_error;
    } else if (!strncmp(raw_data, "$GPGSV,", strlen("$GPGSV,"))) {
        parse_result = (gpgsv_info_t *)malloc(sizeof(gpgsv_info_t));
        if (!parse_result)
            goto malloc_error;

        //parse
        if (gpgsv_info_parser(raw_data, parse_result) != 0)
            goto parse_error;
    } else if (!strncmp(raw_data, "$GPGSA,", strlen("$GPGSA,"))) {
        parse_result = (gpgsa_info_t *)malloc(sizeof(gpgsa_info_t));
        if (!parse_result)
            goto malloc_error;

        //parse
        if (gpgsa_info_parser(raw_data, parse_result) != 0)
            goto parse_error;
    } else if (!strncmp(raw_data, "$GPVTG,", strlen("$GPVTG,"))) {
        parse_result = (gpvtg_info_t *)malloc(sizeof(gpvtg_info_t));
        if (!parse_result)
            goto malloc_error;

        //parse
        if (gpvtg_info_parser(raw_data, parse_result) != 0)
            goto parse_error;
    } else {
        LOG_INFO("Unsupport type!");
        return GPS_INVALID_TYPE;
    }

    LOG_INFO("parse info success! type: %d", t);
    return t;

malloc_error:
    LOG_INFO("malloc error!");
    return GPS_INVALID_TYPE;

parse_error:
    LOG_INFO("parse error!");
    free(parse_result);
    return GPS_INVALID_TYPE;
}
