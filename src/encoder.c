/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "sstv.h"
#include "libsstv.h"

sstv_error_t
sstv_create_encoder(sstv_mode_t mode, uint8_t *image, size_t sample_rate, void **out_ctx)
{
    /* Check input */
}

sstv_error_t
sstv_delete_encoder(void *ctx)
{
}

sstv_error_t
sstv_encode(void *ctx, uint8_t *buffer, size_t buffer_size)
{
}