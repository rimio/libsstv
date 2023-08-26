/*
 * Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _LIBSSTV_H_
#define _LIBSSTV_H_

#ifdef __cplusplus
extern "C" {
#endif

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
    SSTV_BAD_SAMPLE_TYPE        = 107,
    SSTV_UNSUPPORTED_CONVERSION = 108,

    SSTV_ALLOC_FAIL             = 200,

    /* Encoder return codes */
    SSTV_ENCODE_SUCCESSFUL      = 1000,
    SSTV_ENCODE_END             = 1001,

    SSTV_NO_DEFAULT_ENCODERS    = 1100,
} sstv_error_t;

/*
 * SSTV modes (value is VIS+Parity)
 */
typedef enum {
    /* FAX modes */
    SSTV_MODE_FAX480            = 85,

    /* Robot modes */
    SSTV_MODE_ROBOT_BW8_R       = 129,
    SSTV_MODE_ROBOT_BW8_G       = 130,
    SSTV_MODE_ROBOT_BW8_B       = 3,

    SSTV_MODE_ROBOT_BW12_R      = 5,
    SSTV_MODE_ROBOT_BW12_G      = 6,
    SSTV_MODE_ROBOT_BW12_B      = 135,

    SSTV_MODE_ROBOT_BW24_R      = 9,
    SSTV_MODE_ROBOT_BW24_G      = 10,
    SSTV_MODE_ROBOT_BW24_B      = 139,

    SSTV_MODE_ROBOT_BW36_R      = 141,
    SSTV_MODE_ROBOT_BW36_G      = 142,
    SSTV_MODE_ROBOT_BW36_B      = 15,

    SSTV_MODE_ROBOT_C12         = 0,
    SSTV_MODE_ROBOT_C24         = 132,
    SSTV_MODE_ROBOT_C36         = 136,
    SSTV_MODE_ROBOT_C72         = 12,

    /* Scottie modes */
    SSTV_MODE_SCOTTIE_S1        = 60,
    SSTV_MODE_SCOTTIE_S2        = 184,
    SSTV_MODE_SCOTTIE_S3        = 180,
    SSTV_MODE_SCOTTIE_S4        = 48,
    SSTV_MODE_SCOTTIE_DX        = 204,

    /* Martin modes */
    SSTV_MODE_MARTIN_M1         = 172,
    SSTV_MODE_MARTIN_M2         = 40,
    SSTV_MODE_MARTIN_M3         = 36,
    SSTV_MODE_MARTIN_M4         = 160,

    /* PD modes */
    SSTV_MODE_PD50              = 221,
    SSTV_MODE_PD90              = 99,
    SSTV_MODE_PD120             = 95,
    SSTV_MODE_PD160             = 226,
    SSTV_MODE_PD180             = 96,
    SSTV_MODE_PD240             = 225,
    SSTV_MODE_PD290             = 222
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
    uint32_t width;
    uint32_t height;
    sstv_image_format_t format;

    /* image buffer */
    uint8_t *buffer;
} sstv_image_t;

/*
 * Signal sample type
 */
typedef enum {
    SSTV_SAMPLE_UINT8,
    SSTV_SAMPLE_INT8,
    SSTV_SAMPLE_INT16
} sstv_sample_type_t;

/*
 * Signal container
 */
typedef struct {
    /* buffer pointer */
    void *buffer;

    /* size in bytes */
    uint32_t size;

    /* sample type */
    sstv_sample_type_t type;

    /* number of total samples */
    uint32_t capacity;

    /* number of used samples */
    uint32_t count;
} sstv_signal_t;


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
extern sstv_error_t sstv_get_mode_image_props(sstv_mode_t mode, uint32_t *width, uint32_t *height, sstv_image_format_t *format);

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
extern sstv_error_t sstv_create_image_from_props(sstv_image_t *out_img, uint32_t w, uint32_t h, sstv_image_format_t format);

/*
 * Deletes an image.
 *   img(in): pointer to an image structure to delete
 *   returns: error code
 *
 * NOTE: This function deallocates the pixel buffer, so it requires a valid
 * call to sstv_init().
 */
extern sstv_error_t sstv_delete_image(sstv_image_t *img);

/*
 * Converts an image.
 *   img(in): pointer to an image structure
 *   format(in): format to convert image to
 *   returns: error code
 *
 * NOTE: Conversions _from_ SSTV_FORMAT_Y to any format are NOT supported,
 * since the conversion is performed in-place and extra memory would be
 * required.
 */
extern sstv_error_t sstv_convert_image(sstv_image_t *img, sstv_image_format_t format);

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
extern sstv_error_t sstv_pack_image(sstv_image_t *out_img, uint32_t width, uint32_t height, sstv_image_format_t format, uint8_t *buffer);

/*
 * Pack a signal buffer into a signal structure.
 *   sig(in): signal structure to initialize
 *   type(in): sample type
 *   capacity(in): buffer capacity in sampless
 *   buffer(in): buffer pointer
 *   returns: error code
 *
 * NOTE: Buffer is managed by user.
 */
extern sstv_error_t sstv_pack_signal(sstv_signal_t *sig, sstv_sample_type_t type, uint32_t capacity, void *buffer);

/*
 * Create an SSTV encoder.
 *   out_ctx(out): output context structure pointer
 *   image(in): image buffer
 *   mode(in): SSTV mode
 *   sample_rate(in): output signal sample rate
 *   returns: error code
 *
 * NOTE: Context shall never be modified by the user.
 * NOTE: If an allocator/deallocator is provided via sstv_init(), then the
 * context structure will be dynamically allocated. Otherwise, one of the
 * default (static) structures, built into the library, will be used. There are
 * SSTV_DEFAULT_ENCODER_CONTEXT_COUNT default structures, and once these are
 * used up, a SSTV_NO_DEFAULT_ENCODERS error is returned.
 */
extern sstv_error_t sstv_create_encoder(void **out_ctx, sstv_image_t image, sstv_mode_t mode, uint32_t sample_rate);

/*
 * Deletes an SSTV encoder.
 *   ctx(in): encoder context structure pointer
 *   returns: error code
 *
 * NOTE: If context is one of the default encoders, then it will be marked as
 * reusable and can be claimed again by sstv_create_encoder().
 */
extern sstv_error_t sstv_delete_encoder(void *ctx);

/*
 * Encode image into SSTV signal.
 *   ctx(in): encoder context structure pointer
 *   signal(in): output signal container
 *   returns: SSTV_ENCODE_SUCCESSFUL on successful fill of signal buffer
 *            SSTV_ENCODE_END on successful encoding of whole image
 *            error code otherwise
 */
extern sstv_error_t sstv_encode(void *ctx, sstv_signal_t *signal);

#ifdef __cplusplus
}
#endif

#endif
