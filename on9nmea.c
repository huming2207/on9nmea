#include <string.h>
#include <stdint.h>
#include "on9nmea.h"

#define ON9_MAX(a,b) ((a) > (b) ? (a) : (b))
#define ON9_MIN(a,b) ((a) < (b) ? (a) : (b))

static const on9_nmea_mode_indicator_t ON9_GGA_QUALITY_IND[] = {
        ON9_NMEA_MODE_INVALID, // 0 is invalid
        ON9_NMEA_MODE_AUTO_FIXED, // 1 is fixed
        ON9_NMEA_MODE_DIFFERENTIAL, // 2 is differential fixed
        ON9_NMEA_MODE_PPS_FIXED, // 3 is PPS fixed
        ON9_NMEA_MODE_RTK, // 4 is RTK
        ON9_NMEA_MODE_FLOAT_RTK, // 5 is Float RTK
        ON9_NMEA_MODE_ESTIMATED, // 6 is Dead Reckoning
        ON9_NMEA_MODE_MANUAL_INPUT, // 7 is Manual
        ON9_NMEA_MODE_SIMULATE, // 8 is Simulation
};

static void on9_memset(void *ptr, uint8_t set, size_t len)
{
    size_t ctr = 0;
    uint8_t *buf_ptr = ptr;
    while (ctr < len) {
        buf_ptr[ctr] = 0;
        ctr += 1;
    }
}

static char *on9_strnstr(const char *str, const char *substring, size_t str_len)
{
    const char *a;
    const char *b;

    b = substring;

    if (*b == 0) {
        return (char *) str;
    }

    size_t idx = 0;
    for ( ; *str != 0; str += 1) {
        if (idx > str_len) {
            return NULL;
        }

        idx += 1;
        if (*str != *b) {
            continue;
        }

        a = str;
        while (1) {
            if (*b == 0) {
                return (char *) str;
            }

            if (*a++ != *b++) {
                break;
            }
        }
        b = substring;
    }

    return NULL;
}

static uint8_t on9_hex2int(char input)
{
    if (input >= '0' && input <= '9')
        return input - '0';
    if (input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if (input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return 0xff;
}

static uint8_t on9_two_char_to_u8(const char *digits)
{
    return 10 * (digits[0] - '0') + (digits[1] - '0');
}

static uint32_t on9_chars_to_u32(on9_nmea_ctx_t *ctx)
{
    uint32_t ret = 0;
    if (ctx->item_str[ctx->item_pos - 1] >= '0' && ctx->item_str[ctx->item_pos - 1] <= '9') {
        ret = ret * 10 + (uint32_t)(ctx->item_str[ctx->item_pos-1] - '0');
    } else {
        return ret;
    }

    return ret;
}

static void parse_time(on9_nmea_ctx_t *ctx)
{
    if (ctx->item_pos < 6) {
        return;
    }

    ctx->next_result.time.hour = on9_two_char_to_u8(ctx->item_str);
    ctx->next_result.time.minute = on9_two_char_to_u8(ctx->item_str + 2);
    ctx->next_result.time.second = on9_two_char_to_u8(ctx->item_str + 4);
}

static void parse_date(on9_nmea_ctx_t *ctx)
{
    if (ctx->item_pos < 6) {
        return;
    }

    ctx->next_result.date.day = on9_two_char_to_u8(ctx->item_str);
    ctx->next_result.date.month = on9_two_char_to_u8(ctx->item_str + 2);
    ctx->next_result.date.year = on9_two_char_to_u8(ctx->item_str + 4);
}

static on9_nmea_state_t parse_type(on9_nmea_ctx_t *ctx)
{
    if (ctx->item_pos > 4) {
        if (on9_strnstr(ctx->item_str, "RMC", ON9_ITEM_BUF_SIZE - 1) != NULL) {
            ctx->curr_state = ON9_NMEA_STATE_START_RMC;
            ctx->next_result.talker[0] = ctx->item_str[0];
            ctx->next_result.talker[1] = ctx->item_str[1];
            ctx->next_result.type[0] = 'R';
            ctx->next_result.type[1] = 'M';
            ctx->next_result.type[2] = 'C';
            ctx->next_result.type[3] = '\0';
            return ON9_NMEA_STATE_START_RMC;
        } else if (on9_strnstr(ctx->item_str, "GGA", ON9_ITEM_BUF_SIZE - 1) != NULL) {
            ctx->curr_state = ON9_NMEA_STATE_START_GGA;
            ctx->next_result.talker[0] = ctx->item_str[0];
            ctx->next_result.talker[1] = ctx->item_str[1];
            ctx->next_result.type[0] = 'G';
            ctx->next_result.type[1] = 'G';
            ctx->next_result.type[2] = 'A';
            ctx->next_result.type[3] = '\0';
            return ON9_NMEA_STATE_START_GGA;
        }
    }

    return ctx->curr_state;
}

static void parse_on9_float(on9_nmea_ctx_t *ctx, on9_nmea_float_t *out)
{
    if (ctx->item_str[ctx->item_pos - 1] >= '0' && ctx->item_str[ctx->item_pos - 1] <= '9') {
        if (ctx->float_parsing_minor) {
            out->minor = out->minor * 10 + ctx->item_str[ctx->item_pos - 1] - '0';
        } else {
            out->major = out->major * 10 + ctx->item_str[ctx->item_pos - 1] - '0';
        }
    } else if (ctx->item_str[ctx->item_pos - 1] == '.') {
        ctx->float_parsing_minor = true;
    }
}

static void set_on9_float_sign(on9_nmea_float_t *out, bool sign)
{
    if (!sign) {
        out->major *= out->major >= 0 ? -1 : 1;
    } else {
        out->major *= out->major >= 0 ? 1 : -1;
    }
}

static on9_nmea_state_t parse_rmc(on9_nmea_ctx_t *ctx)
{
    switch (ctx->item_num) {
        case 1: {
            parse_time(ctx);
            break;
        }

        case 2: {
            ctx->next_result.valid = (ctx->item_str[0] == 'A');
            break;
        }

        case 3: {
            parse_on9_float(ctx, &ctx->next_result.latitude);
            break;
        }

        case 4: {
            set_on9_float_sign(&ctx->next_result.latitude, ctx->item_str[0] == 'N' || ctx->item_str[0] == 'n');
            break;
        }

        case 5: {
            parse_on9_float(ctx, &ctx->next_result.longitude);
            break;
        }

        case 6: {
            set_on9_float_sign(&ctx->next_result.longitude, ctx->item_str[0] == 'E' || ctx->item_str[0] == 'e');
            break;
        }

        case 7: {
            parse_on9_float(ctx, &ctx->next_result.speed_knot);
            break;
        }

        case 8: {
            parse_on9_float(ctx, &ctx->next_result.tmg);
            break;
        }

        case 9: {
            parse_date(ctx);
            break;
        }

        case 10: {
            parse_on9_float(ctx, &ctx->next_result.mag_variation);
            break;
        }

        case 11: {
            set_on9_float_sign(&ctx->next_result.mag_variation, ctx->item_str[0] == 'E' || ctx->item_str[0] == 'e');
            break;
        }

        case 12: {
            ctx->next_result.mode = (on9_nmea_mode_indicator_t)ctx->item_str[0];
            break;
        }

        default: {
            break;
        }
    }
    return ctx->curr_state;
}

static on9_nmea_state_t parse_gga(on9_nmea_ctx_t *ctx)
{
    switch (ctx->item_num) {
        case 1: {
            parse_time(ctx);
            break;
        }

        case 2: {
            parse_on9_float(ctx, &ctx->next_result.latitude);
            break;
        }

        case 3: {
            set_on9_float_sign(&ctx->next_result.latitude, ctx->item_str[0] == 'N' || ctx->item_str[0] == 'n');
            break;
        }

        case 4: {
            parse_on9_float(ctx, &ctx->next_result.longitude);
            break;
        }

        case 5: {
            set_on9_float_sign(&ctx->next_result.longitude, ctx->item_str[0] == 'E' || ctx->item_str[0] == 'e');
            break;
        }

        case 6: {
            size_t idx = (size_t)ctx->item_str[0] - '0';
            if (idx < sizeof(ON9_GGA_QUALITY_IND)) {
                ctx->next_result.mode = ON9_MAX(ON9_GGA_QUALITY_IND[idx], 9);
            }

            break;
        }

        case 7: {
            ctx->next_result.sat_in_use = on9_chars_to_u32(ctx);
            break;
        }

        case 8: {
            parse_on9_float(ctx, &ctx->next_result.hdop);
            break;
        }

        case 9: {
            parse_on9_float(ctx, &ctx->next_result.altitude);
            break;
        }

        case 11: {
            parse_on9_float(ctx, &ctx->next_result.geo_sep);
            break;
        }

        default: {
            break;
        }
    }
    return ctx->curr_state;
}

static on9_nmea_state_t parse_checksum(on9_nmea_ctx_t *ctx)
{
    if (ctx->item_pos < 1) {
        return ctx->curr_state;
    }

    ctx->expected_checksum = on9_hex2int(ctx->item_str[0]) << 4 | on9_hex2int(ctx->item_str[1]);
    return ctx->curr_state;
}

void on9_nmea_init(on9_nmea_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    on9_memset(ctx, 0, sizeof(on9_nmea_ctx_t));
}

on9_nmea_state_t on9_nmea_feed_char(on9_nmea_ctx_t *ctx, char next)
{
    if (ctx == NULL) {
        return ON9_NMEA_STATE_ERROR_NULLPTR;
    }

    switch (next) {
        case '$': {
            ctx->curr_state = ON9_NMEA_STATE_START_UNDEFINED;
            ctx->item_num = 0;
            ctx->item_pos = 0;
            ctx->curr_checksum = 0;
            ctx->asterisk = false;
            break;
        }

        case ',': {
            ctx->curr_checksum ^= next;
            ctx->item_pos = 0;
            ctx->item_num += 1;
            ctx->float_parsing_minor = false;
            on9_memset(ctx->item_str, 0, ON9_ITEM_BUF_SIZE);
            break;
        }

        // End of main sentence
        case '*': {
            ctx->curr_state = ON9_NMEA_STATE_START_CHECKSUM;
            ctx->asterisk = true;
            ctx->item_pos = 0;
            ctx->item_num += 1;
            break;
        }

        // End of whole sentence
        case '\r': {
            ctx->curr_state = ON9_NMEA_STATE_END_CHECKSUM;
            ctx->asterisk = false;

            if (ctx->expected_checksum == ctx->curr_checksum) {
                ctx->curr_state = ON9_NMEA_STATE_DONE;
            } else {
                ctx->curr_state = ON9_NMEA_STATE_ERROR_CHECKSUM_FAIL;
            }

            return ctx->curr_state;
        }

        // Handle data?
        default: {
            if (!ctx->asterisk) {
                ctx->curr_checksum ^= next;
            }

            // Append char to buffer if this is a part of the NMEA segment
            if ((ctx->curr_state & ON9_NMEA_STATE_START_UNDEFINED) != 0 || (ctx->curr_state & ON9_NMEA_STATE_START_CHECKSUM) != 0) {
                if (next >= 0x20 && next < 0x7f && next != '\n') {
                    ctx->item_str[ctx->item_pos] = next;
                    ctx->item_str[ON9_ITEM_BUF_SIZE - 1] = '\0';
                    if (ctx->item_pos < ON9_ITEM_BUF_SIZE) {
                        ctx->item_pos += 1;
                    }
                }
            }

            switch (ctx->curr_state) {
                case ON9_NMEA_STATE_START_UNDEFINED: {
                    ctx->curr_state = parse_type(ctx);
                    break;
                }

                case ON9_NMEA_STATE_START_GGA: {
                    ctx->curr_state = parse_gga(ctx);
                    break;
                }

                case ON9_NMEA_STATE_START_RMC: {
                    ctx->curr_state = parse_rmc(ctx);
                    break;
                }

                case ON9_NMEA_STATE_START_CHECKSUM: {
                    ctx->curr_state = parse_checksum(ctx);
                    break;
                }

                default: {
                    break;
                }
            }
            break;
        }
    }

    return ctx->curr_state;
}

on9_nmea_result_t *on9_nmea_get_result(on9_nmea_ctx_t *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    return &ctx->next_result;
}
