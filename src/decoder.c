/*
 * Copyright (c) 2018-2024 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "sstv.h"
#include "libsstv.h"

/*
 * Decoder context
 */
typedef struct {
    uint32_t sample_rate;
    uint32_t foo;
} sstv_decoder_context_t;

/*
 * Default decoder contexts, for when no allocation/deallocation routines are provided
 */
static sstv_decoder_context_t default_decoder_context[SSTV_DEFAULT_DECODER_CONTEXT_COUNT];
static uint64_t default_decoder_context_usage = 0x0;


sstv_error_t
sstv_create_decoder(void **out_ctx, uint32_t sample_rate)
{
    sstv_decoder_context_t *ctx = NULL;

    /* check input */
    if (!out_ctx) {
        return SSTV_BAD_PARAMETER;
    }

    /* create context */
    if (sstv_malloc_user) {
        /* user allocator */
        ctx = (sstv_decoder_context_t *) sstv_malloc_user(sizeof(sstv_decoder_context_t));
        if (!ctx) {
            return SSTV_ALLOC_FAIL;
        }
    } else {
        uint32_t i;
        /* use default contexts */
        for (i = 0; i < SSTV_DEFAULT_DECODER_CONTEXT_COUNT; i++) {
            if ((default_decoder_context_usage & (0x1 << i)) == 0) {
                default_decoder_context_usage |= (0x1 << i);
                ctx = &default_decoder_context[i];
                break;
            }
        }
        if (!ctx) {
            return SSTV_NO_DEFAULT_DECODERS;
        }
    }

    /* TODO */
    ctx->sample_rate = sample_rate;

    /* set output */
    *out_ctx = ctx;

    /* all ok */
    return SSTV_OK;
}

sstv_error_t
sstv_delete_decoder(void *ctx)
{
    uint32_t i;

    if (!ctx) {
        return SSTV_BAD_PARAMETER;
    }

    /* check default contexts */
    for (i = 0; i < SSTV_DEFAULT_DECODER_CONTEXT_COUNT; i++) {
        if (ctx == default_decoder_context+i) {
            default_decoder_context_usage &= ~(0x1 << i);
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
sstv_decode(void *ctx, sstv_signal_t *signal, sstv_image_t *image)
{
    return SSTV_DECODE_END;
}
