/*
 * Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "sstv.h"
#include "libsstv.h"
#include "luts.h"

/*
 * FSK helpers
 */
#define FSK(ctx, time, freq) \
    { \
        (ctx)->fsk.phase_delta = (freq).phase_delta; \
        (ctx)->fsk.remaining_usamp += (time).usamp; \
    }
#define FSK_PIXEL(ctx, time, val) \
    { \
        (ctx)->fsk.phase_delta = (ctx)->descriptor.pixel.val_phase_delta[(val)]; \
        (ctx)->fsk.remaining_usamp += (time).usamp; \
    }

/*
 * Encoder state
 */
typedef enum {
    /* start of coding */
    SSTV_ENCODER_STATE_START,

    /* transmission header */
    SSTV_ENCODER_STATE_LEADER_TONE_1,
    SSTV_ENCODER_STATE_BREAK,
    SSTV_ENCODER_STATE_LEADER_TONE_2,

    /* VIS */
    SSTV_ENCODER_STATE_VIS_START_BIT,
    SSTV_ENCODER_STATE_VIS_BIT,
    SSTV_ENCODER_STATE_VIS_STOP_BIT,

    /* sync and porch */
    SSTV_ENCODER_STATE_SYNC,
    SSTV_ENCODER_STATE_SYNC_FIRST,
    SSTV_ENCODER_STATE_PORCH,
    SSTV_ENCODER_STATE_PORCH2,
    SSTV_ENCODER_STATE_PORCH_R,
    SSTV_ENCODER_STATE_PORCH_G,
    SSTV_ENCODER_STATE_PORCH_B,
    SSTV_ENCODER_STATE_PORCH_BY,
    SSTV_ENCODER_STATE_PORCH_RY,

    /* separator */
    SSTV_ENCODER_STATE_SEPARATOR,
    SSTV_ENCODER_STATE_SEPARATOR2,
    SSTV_ENCODER_STATE_SEPARATOR_BY,
    SSTV_ENCODER_STATE_SEPARATOR_RY,

    /* scan */
    SSTV_ENCODER_STATE_Y_SCAN,
    SSTV_ENCODER_STATE_Y_ODD_SCAN,
    SSTV_ENCODER_STATE_Y_EVEN_SCAN,
    SSTV_ENCODER_STATE_RY_SCAN,
    SSTV_ENCODER_STATE_BY_SCAN,

    SSTV_ENCODER_STATE_R_SCAN,
    SSTV_ENCODER_STATE_G_SCAN,
    SSTV_ENCODER_STATE_B_SCAN,

    /* end of coding */
    SSTV_ENCODER_STATE_END
} sstv_encoder_state_t;

/*
 * Encoder context
 */
typedef struct {
    /* input image */
    sstv_image_t image;

    /* output configuration */
    sstv_mode_t mode;
    uint32_t sample_rate;

    /* current state */
    sstv_encoder_state_t state;

    /* current FSK value to be written */
    struct {
        uint32_t phase;
        uint32_t phase_delta;
        uint64_t remaining_usamp;
    } fsk;

    /* mode timings */
    sstv_mode_descriptor_t descriptor;

    /* state extra info */
    union {
        struct {
            uint8_t visp;
            uint8_t curr_bit;
        } vis;

        struct {
            uint32_t curr_line;
            uint32_t curr_col;
        } scan;
    } extra;
} sstv_encoder_context_t;

/*
 * Default encoder contexts, for when no allocation/deallocation routines are provided
 */
static sstv_encoder_context_t default_encoder_context[SSTV_DEFAULT_ENCODER_CONTEXT_COUNT];
static uint64_t default_encoder_context_usage = 0x0;


sstv_error_t
sstv_create_encoder(void **out_ctx, sstv_image_t image, sstv_mode_t mode, uint32_t sample_rate)
{
    sstv_encoder_context_t *ctx = NULL;

    /* check input */
    if (!out_ctx) {
        return SSTV_BAD_PARAMETER;
    }

    /* check image properties */
    {
        uint32_t w, h;
        sstv_image_format_t fmt;
        sstv_error_t rc;

        rc = sstv_get_mode_image_props(mode, &w, &h, &fmt);
        if (rc != SSTV_OK) {
            return rc;
        }
        if (w != image.width || h != image.height) {
            return SSTV_BAD_RESOLUTION;
        }
        if (fmt != image.format) {
            return SSTV_BAD_FORMAT;
        }
    }

    /* create context */
    if (sstv_malloc_user) {
        /* user allocator */
        ctx = (sstv_encoder_context_t *) sstv_malloc_user(sizeof(sstv_encoder_context_t));
        if (!ctx) {
            return SSTV_ALLOC_FAIL;
        }
    } else {
        uint32_t i;
        /* use default contexts */
        for (i = 0; i < SSTV_DEFAULT_ENCODER_CONTEXT_COUNT; i++) {
            if ((default_encoder_context_usage & (0x1 << i)) == 0) {
                default_encoder_context_usage |= (0x1 << i);
                ctx = &default_encoder_context[i];
                break;
            }
        }
        if (!ctx) {
            return SSTV_NO_DEFAULT_ENCODERS;
        }
    }

    /* initialize context */
    ctx->image = image;
    ctx->mode = mode;
    ctx->sample_rate = sample_rate;
    ctx->state = SSTV_ENCODER_STATE_START;
    ctx->fsk.phase = 0; /* start nicely from zero */
    ctx->fsk.remaining_usamp = 0; /* so we get initial state change */

    /* initialize mode timings */
    {
        sstv_error_t rc = sstv_get_mode_descriptor(mode, sample_rate, &ctx->descriptor);
        if (rc != SSTV_OK) {
            if (sstv_free_user) {
                sstv_free_user(ctx);
            }
            return rc;
        }
    }

    /* set output */
    *out_ctx = ctx;

    /* all ok */
    return SSTV_OK;
}

sstv_error_t
sstv_delete_encoder(void *ctx)
{
    uint32_t i;

    if (!ctx) {
        return SSTV_BAD_PARAMETER;
    }

    /* check default contexts */
    for (i = 0; i < SSTV_DEFAULT_ENCODER_CONTEXT_COUNT; i++) {
        if (ctx == default_encoder_context+i) {
            default_encoder_context_usage &= ~(0x1 << i);
            ctx = NULL;
            break;
        }
    }

    /* deallocate context */
    if (ctx) {
        if (!sstv_free_user) {
            return SSTV_BAD_USER_DEALLOC;
        }
        sstv_free_user(ctx);
    }
    
    /* all ok */
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_bw_state_change(sstv_encoder_context_t *context)
{
    /* start communication */
    if (context->state == SSTV_ENCODER_STATE_VIS_STOP_BIT) {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line = 0;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync->y */
    if ((context->state == SSTV_ENCODER_STATE_SYNC)
        || (context->state == SSTV_ENCODER_STATE_Y_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_SYNC) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_Y_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* advance line (y->sync) */
    if ((context->state == SSTV_ENCODER_STATE_Y_SCAN)
        && (context->extra.scan.curr_line < context->image.height-1))
    {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line ++;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* no more state changes, done */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_robot_cfull_state_change(sstv_encoder_context_t *context)
{
    /* start communication */
    if (context->state == SSTV_ENCODER_STATE_VIS_STOP_BIT) {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line = 0;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync->porch */
    if (context->state == SSTV_ENCODER_STATE_SYNC) {
        context->state = SSTV_ENCODER_STATE_PORCH;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch->y or y->y */
    if ((context->state == SSTV_ENCODER_STATE_PORCH)
        || (context->state == SSTV_ENCODER_STATE_Y_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_Y_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* y->separator */
    if (context->state == SSTV_ENCODER_STATE_Y_SCAN) {
        context->state = SSTV_ENCODER_STATE_SEPARATOR_RY;
        FSK(context, context->descriptor.separator.time, context->descriptor.separator.freq);
        return SSTV_OK;
    }

    /* separator -> porch_ry */
    if (context->state == SSTV_ENCODER_STATE_SEPARATOR_RY)
    {
        context->state = SSTV_ENCODER_STATE_PORCH_RY;
        FSK(context, context->descriptor.porch2.time, context->descriptor.porch2.freq);
        return SSTV_OK;
    }

    /* porch_ry->ry_scan or ry->ry */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_RY)
        || (context->state == SSTV_ENCODER_STATE_RY_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_RY) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_RY_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t ry = context->image.buffer[pix_offset * 3 + 2];
        FSK_PIXEL(context, context->descriptor.pixel.time2, ry);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* ry_scan->separator2 */
    if (context->state == SSTV_ENCODER_STATE_RY_SCAN) {
        context->state = SSTV_ENCODER_STATE_SEPARATOR_BY;
        FSK(context, context->descriptor.separator2.time, context->descriptor.separator2.freq);
        return SSTV_OK;
    }

    /* separator2 -> porch_by */
    if (context->state == SSTV_ENCODER_STATE_SEPARATOR_BY)
    {
        context->state = SSTV_ENCODER_STATE_PORCH_BY;
        FSK(context, context->descriptor.porch2.time, context->descriptor.porch2.freq);
        return SSTV_OK;
    }

    /* porch_by->by_scan or by->by */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_BY)
        || (context->state == SSTV_ENCODER_STATE_BY_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_BY) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_BY_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t by = context->image.buffer[pix_offset * 3 + 1];
        FSK_PIXEL(context, context->descriptor.pixel.time2, by);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* advance line (by->sync) */
    if ((context->state == SSTV_ENCODER_STATE_BY_SCAN)
        && (context->extra.scan.curr_line < context->image.height-1))
    {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line ++;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* no more state changes, done */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_robot_chalf_state_change(sstv_encoder_context_t *context)
{
    /* start communication */
    if (context->state == SSTV_ENCODER_STATE_VIS_STOP_BIT) {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line = 0;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync->porch */
    if (context->state == SSTV_ENCODER_STATE_SYNC) {
        context->state = SSTV_ENCODER_STATE_PORCH;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch->y or y->y */
    if ((context->state == SSTV_ENCODER_STATE_PORCH)
        || (context->state == SSTV_ENCODER_STATE_Y_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_Y_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* y->separator/separator2 */
    if (context->state == SSTV_ENCODER_STATE_Y_SCAN) {
        if (context->extra.scan.curr_line % 2 == 0) {
            context->state = SSTV_ENCODER_STATE_SEPARATOR;
            FSK(context, context->descriptor.separator.time, context->descriptor.separator.freq);
        } else {
            context->state = SSTV_ENCODER_STATE_SEPARATOR2;
            FSK(context, context->descriptor.separator2.time, context->descriptor.separator2.freq);
        }

        return SSTV_OK;
    }

    /* separator/separator2 -> porch2 */
    if (context->state == SSTV_ENCODER_STATE_SEPARATOR
        || context->state == SSTV_ENCODER_STATE_SEPARATOR2)
    {
        context->state = SSTV_ENCODER_STATE_PORCH2;
        FSK(context, context->descriptor.porch2.time, context->descriptor.porch2.freq);
        return SSTV_OK;
    }

    /* porch2->ry_scan or ry->ry */
    if ((context->state == SSTV_ENCODER_STATE_PORCH2 && context->extra.scan.curr_line % 2 == 0)
        || (context->state == SSTV_ENCODER_STATE_RY_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH2) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_RY_SCAN;

        uint32_t pix_offset_l0 = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint32_t pix_offset_l1 = context->image.width * (context->extra.scan.curr_line + 1) + context->extra.scan.curr_col;
        uint8_t r1 = context->image.buffer[pix_offset_l0 * 3 + 2];
        uint8_t r2 = context->image.buffer[pix_offset_l1 * 3 + 2];
        uint8_t r = (r1 + r2) / 2;
        FSK_PIXEL(context, context->descriptor.pixel.time2, r);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* porch2->by_scan or by->by */
    if ((context->state == SSTV_ENCODER_STATE_PORCH2 && context->extra.scan.curr_line % 2 == 1)
        || (context->state == SSTV_ENCODER_STATE_BY_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH2) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_BY_SCAN;

        uint32_t pix_offset_l0 = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint32_t pix_offset_l1 = context->image.width * (context->extra.scan.curr_line - 1) + context->extra.scan.curr_col;
        uint8_t b1 = context->image.buffer[pix_offset_l0 * 3 + 1];
        uint8_t b2 = context->image.buffer[pix_offset_l1 * 3 + 1];
        uint8_t b = (b1 + b2) / 2;
        FSK_PIXEL(context, context->descriptor.pixel.time2, b);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* advance line (ry/by->sync) */
    if ((context->state == SSTV_ENCODER_STATE_RY_SCAN || context->state == SSTV_ENCODER_STATE_BY_SCAN)
        && (context->extra.scan.curr_line < context->image.height-1))
    {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line ++;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* no more state changes, done */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_scottie_state_change(sstv_encoder_context_t *context)
{
    /* start communication */
    if (context->state == SSTV_ENCODER_STATE_VIS_STOP_BIT) {
        context->state = SSTV_ENCODER_STATE_SYNC_FIRST;
        context->extra.scan.curr_line = 0;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync_first->porch_g */
    if (context->state == SSTV_ENCODER_STATE_SYNC_FIRST) {
        context->state = SSTV_ENCODER_STATE_PORCH_G;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch_g->scan_g or scan_g->scan_g */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_G)
        || (context->state == SSTV_ENCODER_STATE_G_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_G) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_G_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3 + 1];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;

        return SSTV_OK;
    }

    /* scan_g->porch_b */
    if (context->state == SSTV_ENCODER_STATE_G_SCAN) {
        context->state = SSTV_ENCODER_STATE_PORCH_B;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch_b->scan_b or scan_b->scan_b */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_B)
        || (context->state == SSTV_ENCODER_STATE_B_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_B) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_B_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3 + 2];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;

        return SSTV_OK;
    }

    /* blue->sync */
    if (context->state == SSTV_ENCODER_STATE_B_SCAN) {
        context->state = SSTV_ENCODER_STATE_SYNC;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync->porch_r */
    if (context->state == SSTV_ENCODER_STATE_SYNC) {
        context->state = SSTV_ENCODER_STATE_PORCH_R;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch_r->scan_r or scan_r->scan_r */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_R)
        || (context->state == SSTV_ENCODER_STATE_R_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_R) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_R_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;

        return SSTV_OK;
    }

    /* scan_r->porch_g */
    if ((context->state == SSTV_ENCODER_STATE_R_SCAN)
        && (context->extra.scan.curr_line < context->image.height-1)) {
        context->extra.scan.curr_line ++;
        context->state = SSTV_ENCODER_STATE_PORCH_G;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* no more state changes, done */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_martin_state_change(sstv_encoder_context_t *context)
{
    /* start communication */
    if (context->state == SSTV_ENCODER_STATE_VIS_STOP_BIT) {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line = 0;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync->porch_g */
    if (context->state == SSTV_ENCODER_STATE_SYNC) {
        context->state = SSTV_ENCODER_STATE_PORCH_G;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch_g->scan_g or scan_g->scan_g */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_G)
        || (context->state == SSTV_ENCODER_STATE_G_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_G) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_G_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3 + 1];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;

        return SSTV_OK;
    }

    /* scan_g->porch_b */
    if (context->state == SSTV_ENCODER_STATE_G_SCAN) {
        context->state = SSTV_ENCODER_STATE_PORCH_B;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch_b->scan_b or scan_b->scan_b */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_B)
        || (context->state == SSTV_ENCODER_STATE_B_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_B) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_B_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3 + 2];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;

        return SSTV_OK;
    }

    /* scan_b->porch_r */
    if (context->state == SSTV_ENCODER_STATE_B_SCAN) {
        context->state = SSTV_ENCODER_STATE_PORCH_R;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch_r->scan_r or scan_r->scan_r */
    if ((context->state == SSTV_ENCODER_STATE_PORCH_R)
        || (context->state == SSTV_ENCODER_STATE_R_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH_R) {
            context->extra.scan.curr_col = 0;
        }
        context->state = SSTV_ENCODER_STATE_R_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;

        return SSTV_OK;
    }

    /* scan_r->porch */
    if (context->state == SSTV_ENCODER_STATE_R_SCAN) {
        context->state = SSTV_ENCODER_STATE_PORCH;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch->sync */
    if ((context->state == SSTV_ENCODER_STATE_PORCH)
        && (context->extra.scan.curr_line < context->image.height-1))
    {
        context->extra.scan.curr_line ++;
        context->state = SSTV_ENCODER_STATE_SYNC;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* no more state changes, done */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_pd_state_change(sstv_encoder_context_t *context)
{
    /* start communication */
    if (context->state == SSTV_ENCODER_STATE_VIS_STOP_BIT) {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line = 0;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* advance line (odd->sync) */
    if ((context->state == SSTV_ENCODER_STATE_Y_ODD_SCAN)
        && (context->extra.scan.curr_col >= context->image.width)
        && (context->extra.scan.curr_line < context->image.height-1))
    {
        context->state = SSTV_ENCODER_STATE_SYNC;
        context->extra.scan.curr_line += 2;
        FSK(context, context->descriptor.sync.time, context->descriptor.sync.freq);
        return SSTV_OK;
    }

    /* sync->porch */
    if (context->state == SSTV_ENCODER_STATE_SYNC) {
        context->state = SSTV_ENCODER_STATE_PORCH;
        FSK(context, context->descriptor.porch.time, context->descriptor.porch.freq);
        return SSTV_OK;
    }

    /* porch->even or even->even */
    if ((context->state == SSTV_ENCODER_STATE_PORCH)
        || (context->state == SSTV_ENCODER_STATE_Y_EVEN_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_PORCH) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_Y_EVEN_SCAN;

        uint32_t pix_offset = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* even->R or R->R */
    if ((context->state == SSTV_ENCODER_STATE_Y_EVEN_SCAN)
        || (context->state == SSTV_ENCODER_STATE_RY_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_Y_EVEN_SCAN) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_RY_SCAN;

        uint32_t pix_offset_l0 = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint32_t pix_offset_l1 = context->image.width * (context->extra.scan.curr_line + 1) + context->extra.scan.curr_col;
        uint8_t r1 = context->image.buffer[pix_offset_l0 * 3 + 2];
        uint8_t r2 = context->image.buffer[pix_offset_l1 * 3 + 2];
        uint8_t r = (r1 + r2) / 2;
        FSK_PIXEL(context, context->descriptor.pixel.time, r);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* R->B or B->B */
    if ((context->state == SSTV_ENCODER_STATE_RY_SCAN)
        || (context->state == SSTV_ENCODER_STATE_BY_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_RY_SCAN) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_BY_SCAN;

        uint32_t pix_offset_l0 = context->image.width * context->extra.scan.curr_line + context->extra.scan.curr_col;
        uint32_t pix_offset_l1 = context->image.width * (context->extra.scan.curr_line + 1) + context->extra.scan.curr_col;
        uint8_t b1 = context->image.buffer[pix_offset_l0 * 3 + 1];
        uint8_t b2 = context->image.buffer[pix_offset_l1 * 3 + 1];
        uint8_t b = (b1 + b2) / 2;
        FSK_PIXEL(context, context->descriptor.pixel.time, b);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* B->odd or odd->odd */
    if ((context->state == SSTV_ENCODER_STATE_BY_SCAN)
        || (context->state == SSTV_ENCODER_STATE_Y_ODD_SCAN
            && context->extra.scan.curr_col < context->image.width))
    {
        if (context->state == SSTV_ENCODER_STATE_BY_SCAN) {
            context->extra.scan.curr_col = 0;
        }

        context->state = SSTV_ENCODER_STATE_Y_ODD_SCAN;

        uint32_t pix_offset = context->image.width * (context->extra.scan.curr_line + 1) + context->extra.scan.curr_col;
        uint8_t y = context->image.buffer[pix_offset * 3];
        FSK_PIXEL(context, context->descriptor.pixel.time, y);

        context->extra.scan.curr_col ++;
        return SSTV_OK;
    }

    /* no more state changes, done */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_state_change(sstv_encoder_context_t *context)
{
    /* leader tone #1 */
    if (context->state == SSTV_ENCODER_STATE_START) {
        context->state = SSTV_ENCODER_STATE_LEADER_TONE_1;
        FSK(context,
            context->descriptor.leader_tone.time,
            context->descriptor.leader_tone.freq);
        return SSTV_OK;
    }

    /* break */
    if (context->state == SSTV_ENCODER_STATE_LEADER_TONE_1) {
        context->state = SSTV_ENCODER_STATE_BREAK;
        FSK(context,
            context->descriptor.break_tone.time,
            context->descriptor.break_tone.freq);
        return SSTV_OK;
    }

    /* leader tone #2 */
    if (context->state == SSTV_ENCODER_STATE_BREAK) {
        context->state = SSTV_ENCODER_STATE_LEADER_TONE_2;
        FSK(context,
            context->descriptor.leader_tone.time,
            context->descriptor.leader_tone.freq);
        return SSTV_OK;
    }

    /* VIS start bit */
    if (context->state == SSTV_ENCODER_STATE_LEADER_TONE_2) {
        context->state = SSTV_ENCODER_STATE_VIS_START_BIT;
        context->extra.vis.visp = (uint8_t) context->mode;
        context->extra.vis.curr_bit = 0;
        FSK(context,
            context->descriptor.vis.time,
            context->descriptor.vis.sep_freq);
        return SSTV_OK;
    }

    /* VIS bits */
    if (context->state == SSTV_ENCODER_STATE_VIS_START_BIT
        || (context->state == SSTV_ENCODER_STATE_VIS_BIT && context->extra.vis.curr_bit <= 7))
    {
        uint8_t bit = (context->extra.vis.visp >> context->extra.vis.curr_bit) & 0x1;
        context->state = SSTV_ENCODER_STATE_VIS_BIT;
        context->extra.vis.curr_bit ++;
        if (bit) {
            FSK(context,
                context->descriptor.vis.time,
                context->descriptor.vis.high_freq);
        } else {
            FSK(context,
                context->descriptor.vis.time,
                context->descriptor.vis.low_freq);
        }
        return SSTV_OK;
    }

    /* VIS stop bit */
    if (context->state == SSTV_ENCODER_STATE_VIS_BIT) {
        context->state = SSTV_ENCODER_STATE_VIS_STOP_BIT;
        FSK(context,
            context->descriptor.vis.time,
            context->descriptor.vis.sep_freq);
        return SSTV_OK;
    }

    /* call state change routine for specific mode */
    switch (context->mode) {
        /* Fax modes */
        case SSTV_MODE_FAX480:
            return sstv_encode_bw_state_change(context);

        /* Robot modes */
        case SSTV_MODE_ROBOT_BW8_R:
        case SSTV_MODE_ROBOT_BW8_G:
        case SSTV_MODE_ROBOT_BW8_B:
        case SSTV_MODE_ROBOT_BW12_R:
        case SSTV_MODE_ROBOT_BW12_G:
        case SSTV_MODE_ROBOT_BW12_B:
        case SSTV_MODE_ROBOT_BW24_R:
        case SSTV_MODE_ROBOT_BW24_G:
        case SSTV_MODE_ROBOT_BW24_B:
        case SSTV_MODE_ROBOT_BW36_R:
        case SSTV_MODE_ROBOT_BW36_G:
        case SSTV_MODE_ROBOT_BW36_B:
            return sstv_encode_bw_state_change(context);

        case SSTV_MODE_ROBOT_C12:
        case SSTV_MODE_ROBOT_C36:
            return sstv_encode_robot_chalf_state_change(context);

        case SSTV_MODE_ROBOT_C24:
        case SSTV_MODE_ROBOT_C72:
            return sstv_encode_robot_cfull_state_change(context);
        
        /* Scottie modes */
        case SSTV_MODE_SCOTTIE_S1:
        case SSTV_MODE_SCOTTIE_S2:
        case SSTV_MODE_SCOTTIE_S3:
        case SSTV_MODE_SCOTTIE_S4:
        case SSTV_MODE_SCOTTIE_DX:
            return sstv_encode_scottie_state_change(context);
        
        /* Martin modes */
        case SSTV_MODE_MARTIN_M1:
        case SSTV_MODE_MARTIN_M2:
        case SSTV_MODE_MARTIN_M3:
        case SSTV_MODE_MARTIN_M4:
            return sstv_encode_martin_state_change(context);

        /* PD modes */
        case SSTV_MODE_PD50:
        case SSTV_MODE_PD90:
        case SSTV_MODE_PD120:
        case SSTV_MODE_PD160:
        case SSTV_MODE_PD180:
        case SSTV_MODE_PD240:
        case SSTV_MODE_PD290:
            return sstv_encode_pd_state_change(context);

        default:
            return SSTV_BAD_MODE;
    }
}

sstv_error_t
sstv_encode(void *ctx, sstv_signal_t *signal)
{
    sstv_error_t rc;
    sstv_encoder_context_t *context = (sstv_encoder_context_t *)ctx;

    if (!context || !signal) {
        return SSTV_BAD_PARAMETER;
    }

    /* reset signal container */
    signal->count = 0;

    /* main encoding loop */
    while (1) {
        /* state change? */
        if (context->fsk.remaining_usamp < 1000000) {
            rc = sstv_encode_state_change(context);
            if (rc != SSTV_OK) {
                return rc;
            }

            /* end of encoding? */
            if (context->state == SSTV_ENCODER_STATE_END) {
                return SSTV_ENCODE_END;
            }

            /* make sure we don't skip a state */
            if (context->fsk.remaining_usamp < 1000000) {
                /* this should not happen for a proper sample rate */
                return SSTV_INTERNAL_ERROR;
            }

            continue;
        }

        /* end of buffer? */
        if (signal->count == signal->capacity) {
            return SSTV_ENCODE_SUCCESSFUL;
        }

        /* encode sample and continue */
        context->fsk.remaining_usamp -= 1000000;
        context->fsk.phase += context->fsk.phase_delta;
        switch(signal->type) {
            case SSTV_SAMPLE_INT8:
                ((int8_t *)signal->buffer)[signal->count] = SSTV_SIN_INT10_INT8[context->fsk.phase >> 22];
                break;
            
            case SSTV_SAMPLE_UINT8:
                ((uint8_t *)signal->buffer)[signal->count] = SSTV_SIN_INT10_UINT8[context->fsk.phase >> 22];
                break;

            case SSTV_SAMPLE_INT16:
                ((int16_t *)signal->buffer)[signal->count] = SSTV_SIN_INT10_INT16[context->fsk.phase >> 22];
                break;
            
            default:
                return SSTV_BAD_SAMPLE_TYPE;
        }
        signal->count ++;
    }
}
