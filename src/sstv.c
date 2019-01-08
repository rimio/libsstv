/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "libsstv.h"

sstv_malloc_t sstv_malloc_user = NULL;
sstv_free_t sstv_free_user = NULL;

sstv_error_t
sstv_init(sstv_malloc_t alloc_func, sstv_free_t dealloc_func)
{
    /* Check that either both or none of the functions are provided */
    if ((!alloc_func && dealloc_func) || (alloc_func && !dealloc_func)) {
        return SSTV_BAD_INITIALIZERS;
    }

    /* Keep user functions for allocation/deallocation */
    sstv_malloc_user = alloc_func;
    sstv_free_user = dealloc_func;
    return SSTV_OK;
}

sstv_error_t
sstv_get_mode_image_props(sstv_mode_t mode, size_t *width, size_t *height, sstv_image_format_t *format)
{
    switch (mode) {
        /* PD modes */
        case SSTV_MODE_PD90:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_PD120:
            if (width) *width = 640;
            if (height) *height = 496;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_PD160:
            if (width) *width = 512;
            if (height) *height = 400;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_PD180:
            if (width) *width = 640;
            if (height) *height = 496;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_PD240:
            if (width) *width = 640;
            if (height) *height = 496;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        default:
            return SSTV_BAD_MODE;
    }
    
    return SSTV_OK;
}

sstv_error_t
sstv_create_image_from_mode(sstv_image_t *out_img, sstv_mode_t mode)
{
    size_t w, h;
    sstv_image_format_t fmt;
    sstv_error_t rc;

    rc = sstv_get_mode_image_props(mode, &w, &h, &fmt);
    if (rc != SSTV_OK) {
        return rc;
    }

    return sstv_create_image_from_props(out_img, w, h, fmt);
}

sstv_error_t
sstv_create_image_from_props(sstv_image_t *out_img, size_t w, size_t h, sstv_image_format_t format)
{
    size_t bsize;

    if (!out_img) {
        return SSTV_BAD_PARAMETER;
    }

    /* function needs memory allocator */
    if (!sstv_malloc_user) {
        return SSTV_BAD_USER_ALLOC;
    }

    /* element size */
    switch (format) {
        case SSTV_FORMAT_Y:
            bsize = 1;
            break;

        case SSTV_FORMAT_YCBCR:
        case SSTV_FORMAT_RGB:
            bsize = 3;
            break;

        default:
            return SSTV_BAD_FORMAT;
    }

    /* compute total size */
    bsize *= w * h;

    /* allocate buffer */
    out_img->buffer = (uint8_t *)sstv_malloc_user(bsize);
    if (!out_img->buffer) {
        return SSTV_ALLOC_FAIL;
    }

    /* keep image specs */
    out_img->width = w;
    out_img->height = h;
    out_img->format = format;

    /* done */
    return SSTV_OK;
}

sstv_error_t
sstv_pack_image(sstv_image_t *out_img, size_t width, size_t height, sstv_image_format_t format, uint8_t *buffer)
{
    if (!out_img || !buffer) {
        return SSTV_BAD_PARAMETER;
    }

    /* package image */
    out_img->width = width;
    out_img->height = height;
    out_img->format = format;
    out_img->buffer = buffer;

    /* done */
    return SSTV_OK;
}

sstv_error_t
sstv_delete_image(sstv_image_t *img)
{
    if (!img) {
        return SSTV_BAD_PARAMETER;
    }

    /* free buffer if allocated */
    if (!sstv_free_user) {
        return SSTV_BAD_USER_DEALLOC;
    }
    sstv_free_user(img->buffer);

    /* reset fields */
    img->width = 0;
    img->height = 0;
    img->buffer = NULL;

    /* done */
    return SSTV_OK;
}

sstv_error_t
sstv_pack_signal(sstv_signal_t *sig, sstv_sample_type_t type, size_t capacity, void *buffer)
{
    if (!sig) {
        return SSTV_BAD_PARAMETER;
    }

    switch(type) {
        case SSTV_SAMPLE_INT8:
            sig->size = capacity;
            break;

        case SSTV_SAMPLE_INT16:
            sig->size = 2 * capacity;
            break;

        default:
            return SSTV_BAD_SAMPLE_TYPE;
    }

    sig->buffer = buffer;
    sig->type = type;
    sig->capacity = capacity;
    sig->count = 0;

    /* done */
    return SSTV_OK;
}