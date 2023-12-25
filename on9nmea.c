#include <string.h>
#include "on9nmea.h"

#define ON9_MAX(a,b) ((a) > (b) ? (a) : (b))
#define ON9_MIN(a,b) ((a) < (b) ? (a) : (b))

static char *on9_strnstr(const char *str, const char *substring, size_t str_len)
{
    const char *a;
    const char *b;

    b = substring;

    if (*b == 0) {
        return (char *) str;
    }

    for ( ; *str != 0; str += 1) {
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
            return ctx->curr_state;
        } else if (on9_strnstr(ctx->item_str, "GGA", ON9_ITEM_BUF_SIZE - 1) != NULL) {
            ctx->curr_state = ON9_NMEA_STATE_START_GGA;
            ctx->next_result.talker[0] = ctx->item_str[0];
            ctx->next_result.talker[1] = ctx->item_str[1];
            ctx->next_result.type[0] = 'G';
            ctx->next_result.type[1] = 'G';
            ctx->next_result.type[2] = 'A';
            ctx->next_result.type[3] = '\0';
            return ctx->curr_state;
        } else {
            ctx->item_pos += 1;
        }
    } else {
        ctx->item_pos += 1;
    }

    return ctx->curr_state;
}

void on9_nmea_init(on9_nmea_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(on9_nmea_ctx_t));
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
            ctx->curr_state = parse_type(ctx);
            break;
        }

        case ',': {
            ctx->curr_checksum ^= next;
            ctx->item_num += 1;
            break;
        }

        // End of main sentence
        case '*': {
            ctx->curr_state = ON9_NMEA_STATE_START_CHECKSUM;
            ctx->asterisk = true;
            ctx->item_num += 1;
            break;
        }

        // End of whole sentence
        case '\r': {
            ctx->curr_state = ON9_NMEA_STATE_END_CHECKSUM;
            ctx->asterisk = false;
            // TODO: compare CRC here
            break;
        }

        // Handle data?
        default: {
            switch (ctx->curr_state) {
                case ON9_NMEA_STATE_START_UNDEFINED: {
                    ctx->curr_state = parse_type(ctx);
                    break;
                }

                case ON9_NMEA_STATE_START_GGA:
                case ON9_NMEA_STATE_START_RMC:
                case ON9_NMEA_STATE_START_CHECKSUM: {
                    // Append char to buffer if this is a part of the NMEA segment
                    if (next >= 0x20 && next < 0x7f && next != '\n') {
                        ctx->item_str[ctx->item_pos] = next;
                        ctx->item_str[ON9_ITEM_BUF_SIZE - 1] = '\0';
                        if (ctx->item_pos < ON9_ITEM_BUF_SIZE) {
                            ctx->item_pos += 1;
                        }
                    }

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
    return NULL;
}
