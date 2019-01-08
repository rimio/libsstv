/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "sstv.h"
#include "libsstv.h"
#include "luts.h"

/*
 * Computation helpers
 */
#define REMAINING_SAMPLES_US(time_us, sample_rate) ((uint64_t)(time_us) * (uint64_t)(sample_rate) / 1000000)
#define DPHASE_FROM_FREQ(freq, sample_rate) ((uint64_t)(freq) * 65536 / (uint64_t)(sample_rate))

/*
 * Encoder state
 */
typedef enum {
    SSTV_ENCODER_STATE_START,

    SSTV_ENCODER_STATE_LEADER_TONE_1,
    SSTV_ENCODER_STATE_BREAK,
    SSTV_ENCODER_STATE_LEADER_TONE_2,

    SSTV_ENCODER_STATE_VIS_START_BIT,
    SSTV_ENCODER_STATE_VIS_BIT,
    SSTV_ENCODER_STATE_VIS_STOP_BIT,

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
    size_t sample_rate;

    /* current state */
    sstv_encoder_state_t state;

    /* current FSK value to be written */
    struct {
        uint16_t phase;
        uint16_t phase_delta;
        size_t remaining_samples;
    } fsk;

    /* state extra info */
    union {
        struct {
            uint8_t visp;
            uint8_t curr_bit;
        } vis;
    } extra;
} sstv_encoder_context_t;

/*
 * Default encoder contexts, for when no allocation/deallocation routines are provided
 */
static sstv_encoder_context_t default_encoder_context[SSTV_DEFAULT_ENCODER_CONTEXT_COUNT];
static uint64_t default_encoder_context_usage = 0x0;


sstv_error_t
sstv_create_encoder(void **out_ctx, sstv_image_t image, sstv_mode_t mode, size_t sample_rate)
{
    sstv_encoder_context_t *ctx = NULL;

    /* check input */
    if (!out_ctx) {
        return SSTV_BAD_PARAMETER;
    }

    /* check image properties */
    {
        size_t w, h;
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
        size_t i;
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
    ctx->fsk.remaining_samples = 0; /* so we get initial state change */

    /* set output */
    *out_ctx = ctx;

    /* all ok */
    return SSTV_OK;
}

sstv_error_t
sstv_delete_encoder(void *ctx)
{
    size_t i;

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
sstv_encode_pd_state_change(sstv_encoder_context_t *context)
{
    /* done */
    return SSTV_OK;
}

static sstv_error_t
sstv_encode_state_change(sstv_encoder_context_t *context)
{
    /* leader tone #1 */
    if (context->state == SSTV_ENCODER_STATE_START) {
        context->state = SSTV_ENCODER_STATE_LEADER_TONE_1;
        context->fsk.phase_delta = DPHASE_FROM_FREQ(1900, context->sample_rate);
        context->fsk.remaining_samples = REMAINING_SAMPLES_US(300000, context->sample_rate);
        return SSTV_OK;
    }

    /* break */
    if (context->state == SSTV_ENCODER_STATE_LEADER_TONE_1) {
        context->state = SSTV_ENCODER_STATE_BREAK;
        context->fsk.phase_delta = DPHASE_FROM_FREQ(1200, context->sample_rate);
        context->fsk.remaining_samples = REMAINING_SAMPLES_US(10000, context->sample_rate);
        return SSTV_OK;
    }

    /* leader tone #2 */
    if (context->state == SSTV_ENCODER_STATE_BREAK) {
        context->state = SSTV_ENCODER_STATE_LEADER_TONE_2;
        context->fsk.phase_delta = DPHASE_FROM_FREQ(1900, context->sample_rate);
        context->fsk.remaining_samples = REMAINING_SAMPLES_US(300000, context->sample_rate);
        return SSTV_OK;
    }

    /* VIS start bit */
    if (context->state == SSTV_ENCODER_STATE_LEADER_TONE_2) {
        context->state = SSTV_ENCODER_STATE_VIS_START_BIT;
        context->fsk.phase_delta = DPHASE_FROM_FREQ(1200, context->sample_rate);
        context->fsk.remaining_samples = REMAINING_SAMPLES_US(30000, context->sample_rate);
        context->extra.vis.visp = sstv_get_visp_code(context->mode);
        context->extra.vis.curr_bit = 0;
        return SSTV_OK;
    }

    /* VIS bits */
    if (context->state == SSTV_ENCODER_STATE_VIS_START_BIT
        || (context->state == SSTV_ENCODER_STATE_VIS_BIT && context->extra.vis.curr_bit <= 7))
    {
        uint8_t bit = (context->extra.vis.visp >> context->extra.vis.curr_bit) & 0x1;
        context->state = SSTV_ENCODER_STATE_VIS_BIT;
        context->extra.vis.curr_bit ++;
        context->fsk.phase_delta = DPHASE_FROM_FREQ((bit ? 1100 : 1300), context->sample_rate);
        context->fsk.remaining_samples = REMAINING_SAMPLES_US(30000, context->sample_rate);
        return SSTV_OK;
    }

    /* VIS stop bit */
    if (context->state == SSTV_ENCODER_STATE_VIS_BIT) {
        context->state = SSTV_ENCODER_STATE_VIS_STOP_BIT;
        context->fsk.phase_delta = DPHASE_FROM_FREQ(1200, context->sample_rate);
        context->fsk.remaining_samples = REMAINING_SAMPLES_US(30000, context->sample_rate);
        return SSTV_OK;
    }

    /* debug */
    context->state = SSTV_ENCODER_STATE_END;
    return SSTV_OK;

    /* call state change routine for specific mode */
    switch (context->mode) {
        /* PD modes */
        case SSTV_MODE_PD90:
        case SSTV_MODE_PD120:
        case SSTV_MODE_PD160:
        case SSTV_MODE_PD180:
        case SSTV_MODE_PD240:
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
        if (context->fsk.remaining_samples == 0) {
            rc = sstv_encode_state_change(context);
            if (rc != SSTV_OK) {
                return rc;
            }

            /* end of encoding? */
            if (context->state == SSTV_ENCODER_STATE_END) {
                return SSTV_ENCODE_END;
            }
            continue;
        }

        /* end of buffer? */
        if (signal->count == signal->capacity) {
            return SSTV_ENCODE_SUCCESSFUL;
        }

        /* encode sample and continue */
        context->fsk.remaining_samples --;
        context->fsk.phase += context->fsk.phase_delta;
        switch(signal->type) {
            case SSTV_SAMPLE_INT8:
                ((int8_t *)signal->buffer)[signal->count] = SSTV_SIN_INT10_INT8[context->fsk.phase >> 6];
                break;
            
            case SSTV_SAMPLE_UINT8:
                ((uint8_t *)signal->buffer)[signal->count] = SSTV_SIN_INT10_UINT8[context->fsk.phase >> 6];
                break;

            case SSTV_SAMPLE_INT16:
                ((int16_t *)signal->buffer)[signal->count] = SSTV_SIN_INT10_INT16[context->fsk.phase >> 6];
                break;
            
            default:
                return SSTV_BAD_SAMPLE_TYPE;
        }
        signal->count ++;
    }
}