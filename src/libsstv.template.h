/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _LIBSSTV_H_
#define _LIBSSTV_H_

#include <stdint.h>
#include <stddef.h>

/*
 * Version
 */
#define SSTV_VERSION_MAJOR @VERSION_MAJOR@
#define SSTV_VERSION_MINOR @VERSION_MINOR@
#define SSTV_VERSION_PATCH @VERSION_PATCH@
#define SSTV_VERSION "@VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_PATCH@"

/*
 * Limits
 */
#define SSTV_DEFAULT_ENCODER_CONTEXT_COUNT @DEFAULT_ENCODER_CONTEXT_COUNT@

/*
 * Error codes
 */
typedef enum {
    /* No error */
    SSTV_OK                     = 0,

    /* Unknown error - should not happen, fatal */
    SSTV_INTERNAL_ERROR         = 1,

    /* Generic library errors */
    SSTV_BAD_INITIALIZERS       = 100,
    SSTV_BAD_USER_ALLOC         = 101,
    SSTV_BAD_USER_DEALLOC       = 102,
    SSTV_BAD_PARAMETER          = 103,
    SSTV_BAD_MODE               = 104,
    SSTV_BAD_FORMAT             = 105,
    SSTV_BAD_RESOLUTION         = 106,

    SSTV_ALLOC_FAIL             = 200,

    /* Encoder return codes */
    SSTV_ENCODE_SUCCESSFUL      = 1000,
    SSTV_ENCODE_END             = 1001,
    SSTV_ENCODE_FAIL            = 1002,
} sstv_error_t;

/*
 * SSTV modes
 */
typedef enum {
    /* PD modes */
    SSTV_MODE_PD90,
    SSTV_MODE_PD120,
    SSTV_MODE_PD160,
    SSTV_MODE_PD180,
    SSTV_MODE_PD240
} sstv_mode_t;

/*
 * Memory management
 */
typedef void* (*sstv_malloc_t)(size_t);
typedef void (*sstv_free_t)(void *);

/*
 * Image format
 */
typedef enum {
    /* grayscale */
    SSTV_FORMAT_Y,

    /* YCbCr */
    SSTV_FORMAT_YCBCR,

    /* RGB */
    SSTV_FORMAT_RGB
} sstv_image_format_t;

/*
 * Image container
 */
typedef struct {
    /* image properties */
    size_t width;
    size_t height;
    sstv_image_format_t format;

    /* image buffer */
    uint8_t *buffer;
} sstv_image_t;


/*
 * Initialize the library.
 *   alloc_func(in): memory allocation function (e.g. malloc)
 *   dealloc_func(in): memory deallocation function (e.g. free)
 *   returns: error code
 *
 * NOTE: Call to this function is optional. If no allocation/deallocation
 * routines are provided then the library will provide limited functionality.
 */
extern sstv_error_t sstv_init(sstv_malloc_t alloc_func, sstv_free_t dealloc_func);

/*
 * Retrieve the image properties for a supported SSTV mode.
 *   mode(in): desired SSTV mode
 *   width(out): width of image
 *   height(out): height of image
 *   format(out): pixel format
 *   returns: error code
 */
extern sstv_error_t sstv_get_mode_image_props(sstv_mode_t mode, size_t *width, size_t *height, sstv_image_format_t *format);

/*
 * Create an image given an SSTV mode.
 *   out_img(out): pointer to an image structure to initialize
 *   mode(in): desired SSTV mode
 *   returns: error code
 *
 * NOTE: This function allocates the pixel buffer, so it requires a valid call
 * to sstv_init().
 * NOTE: The resulting image must be deleted using sstv_delete_image() once it
 * goes out of scope (i.e. after the encoder is deleted).
 */
extern sstv_error_t sstv_create_image_from_mode(sstv_image_t *out_img, sstv_mode_t mode);

/*
 * Create an image given image properties.
 *   out_img(out): pointer to an image structure to initialize
 *   w(in): width
 *   h(in): height
 *   format(in): pixel format
 *   returns: error code
 *
 * NOTE: This function allocates the pixel buffer, so it requires a valid call
 * to sstv_init().
 * NOTE: The resulting image must be deleted using sstv_delete_image() once it
 * goes out of scope (i.e. after the encoder is deleted).
 */
extern sstv_error_t sstv_create_image_from_props(sstv_image_t *out_img, size_t w, size_t h, sstv_image_format_t format);

/*
 * Pack an image into an image structure, given properties and buffer.
 *   out_img(out): pointer to an image structure to initialize
 *   width(in): width
 *   height(in): height
 *   format(in): pixel format
 *   buffer(in): pixel buffer
 *   returns: error code
 *
 * NOTE: Pixel buffer is managed by user. Do NOT call sstv_delete_image() on
 * resulting image.
 */
extern sstv_error_t sstv_pack_image(sstv_image_t *out_img, size_t width, size_t height, sstv_image_format_t format, uint8_t *buffer);

/*
 * Deletes an image.
 *   img(img): pointer to an image structure to delete
 *   returns: error code
 *
 * NOTE: This function deallocates the pixel buffer, so it requires a valid
 * call to sstv_init().
 */
extern sstv_error_t sstv_delete_image(sstv_image_t *img);

extern sstv_error_t sstv_create_encoder(void **out_ctx, sstv_image_t image, sstv_mode_t mode, size_t sample_rate);
extern sstv_error_t sstv_delete_encoder(void *ctx);
extern sstv_error_t sstv_encode(void *ctx, uint8_t *buffer, size_t buffer_size);

#endif