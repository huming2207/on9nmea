#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ON9_ITEM_BUF_SIZE 16UL

typedef enum on9_nmea_state : uint8_t {
    ON9_NMEA_STATE_IDLE = 0,
    ON9_NMEA_STATE_START_UNDEFINED = 0x10,
    ON9_NMEA_STATE_START_RMC = 0x11,
    ON9_NMEA_STATE_START_GGA = 0x12,
    ON9_NMEA_STATE_START_CHECKSUM = 0x20,
    ON9_NMEA_STATE_END_CHECKSUM = 0x40,
    ON9_NMEA_STATE_DONE = 0x80,
    ON9_NMEA_STATE_ERROR_CHECKSUM_FAIL = 0xF0,
    ON9_NMEA_STATE_ERROR_UNKNOWN_LINE = 0xF1,
    ON9_NMEA_STATE_ERROR_NULLPTR = 0xFF,
} on9_nmea_state_t;

typedef enum on9_nmea_mode_indicator : uint8_t {
    ON9_NMEA_MODE_UNKNOWN = 0,
    ON9_NMEA_MODE_PPS_FIXED = 3,
    ON9_NMEA_MODE_AUTO_FIXED = 'A',
    ON9_NMEA_MODE_DIFFERENTIAL = 'D',
    ON9_NMEA_MODE_ESTIMATED = 'E',
    ON9_NMEA_MODE_FLOAT_RTK = 'F',
    ON9_NMEA_MODE_MANUAL_INPUT = 'M',
    ON9_NMEA_MODE_NO_FIX = 'N',
    ON9_NMEA_MODE_PRECISE = 'P',
    ON9_NMEA_MODE_RTK = 'R',
    ON9_NMEA_MODE_SIMULATE = 'S',
    ON9_NMEA_MODE_INVALID = 'V',
} on9_nmea_mode_indicator_t;

typedef enum on9_nmea_nav_status : uint8_t {
    ON9_NMEA_NAV_STATUS_UNKNOWN = 0,
    ON9_NMEA_NAV_STATUS_SAFE = 'S',
    ON9_NMEA_NAV_STATUS_CAUTION = 'C',
    ON9_NMEA_NAV_STATUS_UNSAFE = 'U',
    ON9_NMEA_NAV_STATUS_INVALID = 'V',
} on9_nmea_nav_status_t;

typedef struct on9_nmea_time {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t sub_secs;
} on9_nmea_time_t;

typedef struct on9_nmea_date {
    uint8_t year;
    uint8_t month;
    uint8_t day;
} on9_nmea_date_t;

#define ON9_NMEA_COORD_MINOR_ACCURACY 1000000UL

typedef struct on9_nmea_coord {
    int32_t major;
    uint32_t minor; // x1000000
} on9_nmea_float_t;

typedef struct on9_gnss_result {
    bool valid;
    uint8_t sat_in_use;
    on9_nmea_mode_indicator_t mode;
    on9_nmea_nav_status_t nav_status;
    char talker[3];
    on9_nmea_date_t date;
    on9_nmea_time_t time;
    char type[4];
    on9_nmea_float_t latitude;
    on9_nmea_float_t longitude;
    on9_nmea_float_t speed_knot;
    on9_nmea_float_t hdop;
    on9_nmea_float_t tmg; // Track made good
    on9_nmea_float_t altitude;
    on9_nmea_float_t geo_sep; // Geoidal separation
    on9_nmea_float_t mag_variation;
} on9_nmea_result_t;

typedef struct on9_nmea_ctx {
    bool asterisk;
    uint8_t item_num;
    uint8_t item_pos;
    uint8_t curr_checksum;
    uint8_t expected_checksum;
    on9_nmea_state_t curr_state;
    on9_nmea_result_t next_result;
    char item_str[ON9_ITEM_BUF_SIZE];
} on9_nmea_ctx_t;

void on9_nmea_init(on9_nmea_ctx_t *ctx);
on9_nmea_state_t on9_nmea_feed_char(on9_nmea_ctx_t *ctx, char next);
on9_nmea_result_t *on9_nmea_get_result(on9_nmea_ctx_t *ctx);