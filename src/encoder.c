/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "sstv.h"
#include "libsstv.h"

/*
 * Encoder context
 */
typedef struct {
    /* input image */
    sstv_image_t image;

    /* output configuration */
    sstv_mode_t mode;
    size_t sample_rate;
} sstv_encoder_context_t;

/* default encoder contexts, for when no allocation/deallocation routines are provided */
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
    }

    if (!ctx) {
        return SSTV_INTERNAL_ERROR;
    }

    /* initialize context */
    ctx->image = image;
    ctx->mode = mode;
    ctx->sample_rate = sample_rate;

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

sstv_error_t
sstv_encode(void *ctx, uint8_t *buffer, size_t buffer_size)
{
}