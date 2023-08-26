/*
 * Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "libsstv.h"
#include "sstv.h"

/*
 * Colorspace conversion macros
 * Kudos to Leszek Szary
 * https://stackoverflow.com/questions/1737726/how-to-perform-rgb-yuv-conversion-in-c-c
 */
#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)
#define CRGB2Y(R, G, B) CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16)
#define CRGB2Cb(R, G, B) CLIP((36962 * (B - CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16) ) >> 16) + 128)
#define CRGB2Cr(R, G, B) CLIP((46727 * (R - CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16) ) >> 16) + 128)
#define CYCbCr2R(Y, Cb, Cr) CLIP( Y + ( 91881 * Cr >> 16 ) - 179 )
#define CYCbCr2G(Y, Cb, Cr) CLIP( Y - (( 22544 * Cb + 46793 * Cr ) >> 16) + 135)
#define CYCbCr2B(Y, Cb, Cr) CLIP( Y + (116129 * Cb >> 16 ) - 226 )

/*
 * Timings macros
 */
#define TIME_DESC_INIT(time_ns, sample_rate) ((sstv_timing_desc_t){ (time_ns), ((uint64_t)(time_ns) * (uint64_t)(sample_rate) / 1000) })
#define FREQ_DESC_INIT(freq, sample_rate) ((sstv_freq_desc_t){ (freq), ((((uint64_t)(freq)) << 32) / (uint64_t)(sample_rate)) })

/*
 * User-defined memory allocation functions
 */
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
sstv_convert_image(sstv_image_t *img, sstv_image_format_t format)
{
    if (!img) {
        return SSTV_BAD_PARAMETER;
    }

    if (img->format == format) {
        /* no conversion necessary */
        return SSTV_OK;
    }

    if (img->format == SSTV_FORMAT_Y) {
        /* can't convert from grayscale to anything */
        return SSTV_UNSUPPORTED_CONVERSION;
    }

    /* perform conversion */
    uint32_t num_px = img->width * img->height;
    uint32_t i;
    switch (format) {
        case SSTV_FORMAT_Y:
            {
                switch (img->format) {
                    case SSTV_FORMAT_Y:
                        /* identity, do nothing */
                        break;
                    
                    case SSTV_FORMAT_YCBCR:
                        /* condense first channel */
                        for (i = 0; i < num_px; i ++) {
                            img->buffer[i] = img->buffer[i * 3];
                        }
                        break;
                    
                    case SSTV_FORMAT_RGB:
                        /* extract Y from RGB and set as first channel */
                        for (i = 0; i < num_px; i ++) {
                            int32_t r = img->buffer[i * 3 + 0];
                            int32_t g = img->buffer[i * 3 + 1];
                            int32_t b = img->buffer[i * 3 + 2];
                            img->buffer[i] = CRGB2Y(r, g, b);
                        }
                        break;

                    default:
                        return SSTV_UNSUPPORTED_CONVERSION;
                }
            }
            break;
        
        case SSTV_FORMAT_YCBCR:
            {
                switch (img->format) {
                    case SSTV_FORMAT_YCBCR:
                        /* identity, do nothing */
                        break;
                    
                    case SSTV_FORMAT_RGB:
                        for (i = 0; i < num_px; i ++) {
                            int32_t r = img->buffer[i * 3 + 0];
                            int32_t g = img->buffer[i * 3 + 1];
                            int32_t b = img->buffer[i * 3 + 2];
                            img->buffer[i * 3 + 0] = CRGB2Y(r, g, b);
                            img->buffer[i * 3 + 1] = CRGB2Cb(r, g, b);
                            img->buffer[i * 3 + 2] = CRGB2Cr(r, g, b);
                        }
                        break;

                    default:
                        return SSTV_UNSUPPORTED_CONVERSION;
                }
            }
            break;
        
        case SSTV_FORMAT_RGB:
            {
                switch (img->format) {
                    case SSTV_FORMAT_YCBCR:
                        for (i = 0; i < num_px; i ++) {
                            int32_t y = img->buffer[i * 3 + 0];
                            int32_t b = img->buffer[i * 3 + 1];
                            int32_t r = img->buffer[i * 3 + 2];
                            img->buffer[i * 3 + 0] = CYCbCr2R(y, b ,r);
                            img->buffer[i * 3 + 1] = CYCbCr2G(y, b ,r);
                            img->buffer[i * 3 + 2] = CYCbCr2B(y, b ,r);
                        }
                        break;
                    
                    case SSTV_FORMAT_RGB:
                        /* identity, do nothing */
                        break;

                    default:
                        return SSTV_UNSUPPORTED_CONVERSION;
                }
            }
            break;

        default:
            return SSTV_BAD_FORMAT;
    }

    /* all ok */
    img->format = format;
    return SSTV_OK;
}

sstv_error_t
sstv_get_mode_image_props(sstv_mode_t mode, uint32_t *width, uint32_t *height, sstv_image_format_t *format)
{
    switch (mode) {
        /* FAX modes */
        case SSTV_MODE_FAX480:
            if (width) *width = 512;
            if (height) *height = 480;
            if (format) *format = SSTV_FORMAT_Y;
            break;

        /* Robot modes */
        case SSTV_MODE_ROBOT_BW8_R:
        case SSTV_MODE_ROBOT_BW8_G:
        case SSTV_MODE_ROBOT_BW8_B:
            if (width) *width = 160;
            if (height) *height = 120;
            if (format) *format = SSTV_FORMAT_Y;
            break;

        case SSTV_MODE_ROBOT_BW12_R:
        case SSTV_MODE_ROBOT_BW12_G:
        case SSTV_MODE_ROBOT_BW12_B:
            if (width) *width = 160;
            if (height) *height = 120;
            if (format) *format = SSTV_FORMAT_Y;
            break;

        case SSTV_MODE_ROBOT_BW24_R:
        case SSTV_MODE_ROBOT_BW24_G:
        case SSTV_MODE_ROBOT_BW24_B:
            if (width) *width = 320;
            if (height) *height = 240;
            if (format) *format = SSTV_FORMAT_Y;
            break;

        case SSTV_MODE_ROBOT_BW36_R:
        case SSTV_MODE_ROBOT_BW36_G:
        case SSTV_MODE_ROBOT_BW36_B:
            if (width) *width = 320;
            if (height) *height = 240;
            if (format) *format = SSTV_FORMAT_Y;
            break;

        case SSTV_MODE_ROBOT_C12:
            if (width) *width = 160;
            if (height) *height = 120;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_ROBOT_C24:
            if (width) *width = 320;
            if (height) *height = 120;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_ROBOT_C36:
            if (width) *width = 320;
            if (height) *height = 240;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

        case SSTV_MODE_ROBOT_C72:
            if (width) *width = 320;
            if (height) *height = 240;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;
        
        /* Scottie modes */
        case SSTV_MODE_SCOTTIE_S1:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_SCOTTIE_S2:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_SCOTTIE_S3:
            if (width) *width = 320;
            if (height) *height = 128;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_SCOTTIE_S4:
            if (width) *width = 320;
            if (height) *height = 128;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_SCOTTIE_DX:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        /* Martin modes */
        case SSTV_MODE_MARTIN_M1:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_MARTIN_M2:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_MARTIN_M3:
            if (width) *width = 320;
            if (height) *height = 128;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        case SSTV_MODE_MARTIN_M4:
            if (width) *width = 320;
            if (height) *height = 128;
            if (format) *format = SSTV_FORMAT_RGB;
            break;

        /* PD modes */
        case SSTV_MODE_PD50:
            if (width) *width = 320;
            if (height) *height = 256;
            if (format) *format = SSTV_FORMAT_YCBCR;
            break;

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

        case SSTV_MODE_PD290:
            if (width) *width = 800;
            if (height) *height = 616;
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
    uint32_t w, h;
    sstv_image_format_t fmt;
    sstv_error_t rc;

    rc = sstv_get_mode_image_props(mode, &w, &h, &fmt);
    if (rc != SSTV_OK) {
        return rc;
    }

    return sstv_create_image_from_props(out_img, w, h, fmt);
}

sstv_error_t
sstv_create_image_from_props(sstv_image_t *out_img, uint32_t w, uint32_t h, sstv_image_format_t format)
{
    uint32_t bsize;

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
sstv_pack_image(sstv_image_t *out_img, uint32_t width, uint32_t height, sstv_image_format_t format, uint8_t *buffer)
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
sstv_pack_signal(sstv_signal_t *sig, sstv_sample_type_t type, uint32_t capacity, void *buffer)
{
    if (!sig) {
        return SSTV_BAD_PARAMETER;
    }

    switch(type) {
        case SSTV_SAMPLE_INT8:
        case SSTV_SAMPLE_UINT8:
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

sstv_error_t
sstv_get_mode_descriptor(sstv_mode_t mode, uint32_t sample_rate, sstv_mode_descriptor_t *desc)
{
    if (!desc) {
        return SSTV_INTERNAL_ERROR;
    }

    /* Common desc and frequencies */
    desc->leader_tone.time = TIME_DESC_INIT(300000000, sample_rate); // 300ms
    desc->leader_tone.freq = FREQ_DESC_INIT(1900, sample_rate);

    desc->break_tone.time = TIME_DESC_INIT(10000000, sample_rate); // 10ms
    desc->break_tone.freq = FREQ_DESC_INIT(1200, sample_rate);

    desc->vis.time = TIME_DESC_INIT(30000000, sample_rate); // 300ms
    desc->vis.sep_freq = FREQ_DESC_INIT(1200, sample_rate);
    desc->vis.low_freq = FREQ_DESC_INIT(1300, sample_rate);
    desc->vis.high_freq = FREQ_DESC_INIT(1100, sample_rate);

    /* Mode frequencies */
    switch (mode) {
        /*
         * Fax modes
         */
        case SSTV_MODE_FAX480:
            desc->sync.freq = FREQ_DESC_INIT(1200, sample_rate);
            desc->porch.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.low_freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.bandwidth = FREQ_DESC_INIT(800, sample_rate);
            break;

        /* Robot B&W modes */
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
            desc->sync.freq = FREQ_DESC_INIT(1200, sample_rate);
            desc->porch.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.low_freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.bandwidth = FREQ_DESC_INIT(800, sample_rate);
            break;

        /*
         * Robot color modes
         */
        case SSTV_MODE_ROBOT_C12:
        case SSTV_MODE_ROBOT_C24:
        case SSTV_MODE_ROBOT_C36:
        case SSTV_MODE_ROBOT_C72:
            desc->sync.freq = FREQ_DESC_INIT(1200, sample_rate);
            desc->porch.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->porch2.freq = FREQ_DESC_INIT(1900, sample_rate);
            desc->separator.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->separator2.freq = FREQ_DESC_INIT(2300, sample_rate);
            desc->pixel.low_freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.bandwidth = FREQ_DESC_INIT(800, sample_rate);
            break;
        
        /*
         * Scottie modes
         */
        case SSTV_MODE_SCOTTIE_S1:
        case SSTV_MODE_SCOTTIE_S2:
        case SSTV_MODE_SCOTTIE_S3:
        case SSTV_MODE_SCOTTIE_S4:
        case SSTV_MODE_SCOTTIE_DX:
            desc->sync.freq = FREQ_DESC_INIT(1200, sample_rate);
            desc->porch.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.low_freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.bandwidth = FREQ_DESC_INIT(800, sample_rate);
            break;

        /*
         * Martin modes
         */
        case SSTV_MODE_MARTIN_M1:
        case SSTV_MODE_MARTIN_M2:
        case SSTV_MODE_MARTIN_M3:
        case SSTV_MODE_MARTIN_M4:
            desc->sync.freq = FREQ_DESC_INIT(1200, sample_rate);
            desc->porch.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.low_freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.bandwidth = FREQ_DESC_INIT(800, sample_rate);
            break;

        /*
         * PD modes
         */
        case SSTV_MODE_PD50:
        case SSTV_MODE_PD90:
        case SSTV_MODE_PD120:
        case SSTV_MODE_PD160:
        case SSTV_MODE_PD180:
        case SSTV_MODE_PD240:
        case SSTV_MODE_PD290:
            desc->sync.freq = FREQ_DESC_INIT(1200, sample_rate);
            desc->porch.freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.low_freq = FREQ_DESC_INIT(1500, sample_rate);
            desc->pixel.bandwidth = FREQ_DESC_INIT(800, sample_rate);
            break;

        /*
         * invalid mode
         */
        default:
            return SSTV_BAD_MODE;
    }

    /* Mode desc */
    switch (mode) {
        /*
         * FAX modes
         */
        case SSTV_MODE_FAX480:
            desc->sync.time = TIME_DESC_INIT(5120000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(512000, sample_rate);
            break;

        /*
         * Robot B&W modes
         */
        case SSTV_MODE_ROBOT_BW8_R:
        case SSTV_MODE_ROBOT_BW8_G:
        case SSTV_MODE_ROBOT_BW8_B:
            desc->sync.time = TIME_DESC_INIT(10000000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(350000, sample_rate);
            break;

        case SSTV_MODE_ROBOT_BW12_R:
        case SSTV_MODE_ROBOT_BW12_G:
        case SSTV_MODE_ROBOT_BW12_B:
            desc->sync.time = TIME_DESC_INIT(7000000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(581250, sample_rate);
            break;

        case SSTV_MODE_ROBOT_BW24_R:
        case SSTV_MODE_ROBOT_BW24_G:
        case SSTV_MODE_ROBOT_BW24_B:
            desc->sync.time = TIME_DESC_INIT(12000000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(290625, sample_rate);
            break;

        case SSTV_MODE_ROBOT_BW36_R:
        case SSTV_MODE_ROBOT_BW36_G:
        case SSTV_MODE_ROBOT_BW36_B:
            desc->sync.time = TIME_DESC_INIT(12000000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(431250, sample_rate);
            break;

        /*
         * Robot color modes
         */
        case SSTV_MODE_ROBOT_C12:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(3000000, sample_rate);
            desc->porch2.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->separator.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->separator2.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(375000, sample_rate);
            desc->pixel.time2 = TIME_DESC_INIT(187500, sample_rate);
            break;

        case SSTV_MODE_ROBOT_C24:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(3000000, sample_rate);
            desc->porch2.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->separator.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->separator2.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(275000, sample_rate);
            desc->pixel.time2 = TIME_DESC_INIT(137500, sample_rate);
            break;

        case SSTV_MODE_ROBOT_C36:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(3000000, sample_rate);
            desc->porch2.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->separator.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->separator2.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(281250, sample_rate);
            desc->pixel.time2 = TIME_DESC_INIT(140625, sample_rate);
            break;

        case SSTV_MODE_ROBOT_C72:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(3000000, sample_rate);
            desc->porch2.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->separator.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->separator2.time = TIME_DESC_INIT(4500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(431250, sample_rate);
            desc->pixel.time2 = TIME_DESC_INIT(215625, sample_rate);
            break;
        
        /*
         * Scottie modes
         */
        case SSTV_MODE_SCOTTIE_S1:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(432000, sample_rate);
            break;

        case SSTV_MODE_SCOTTIE_S2:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(275200, sample_rate);
            break;

        case SSTV_MODE_SCOTTIE_S3:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(432000, sample_rate);
            break;

        case SSTV_MODE_SCOTTIE_S4:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(275200, sample_rate);
            break;

        case SSTV_MODE_SCOTTIE_DX:
            desc->sync.time = TIME_DESC_INIT(9000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(1500000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(1080000, sample_rate);
            break;
        
        /*
         * Martin modes
         */
        case SSTV_MODE_MARTIN_M1:
            desc->sync.time = TIME_DESC_INIT(4862000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(572000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(457600, sample_rate);
            break;

        case SSTV_MODE_MARTIN_M2:
            desc->sync.time = TIME_DESC_INIT(4862000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(572000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(228800, sample_rate);
            break;

        case SSTV_MODE_MARTIN_M3:
            desc->sync.time = TIME_DESC_INIT(4862000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(572000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(457600, sample_rate);
            break;

        case SSTV_MODE_MARTIN_M4:
            desc->sync.time = TIME_DESC_INIT(4862000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(572000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(228800, sample_rate);
            break;

        /*
         * PD modes
         */
        case SSTV_MODE_PD50:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(286000, sample_rate);
            break;

        case SSTV_MODE_PD90:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(532000, sample_rate);
            break;

        case SSTV_MODE_PD120:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(190000, sample_rate);
            break;

        case SSTV_MODE_PD160:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(382000, sample_rate);
            break;

        case SSTV_MODE_PD180:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(286000, sample_rate);
            break;

        case SSTV_MODE_PD240:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(382000, sample_rate);
            break;

        case SSTV_MODE_PD290:
            desc->sync.time = TIME_DESC_INIT(20000000, sample_rate);
            desc->porch.time = TIME_DESC_INIT(2080000, sample_rate);
            desc->pixel.time = TIME_DESC_INIT(286000, sample_rate);
            break;

        /*
         * invalid mode
         */
        default:
            return SSTV_BAD_MODE;
    }

    /* compute pixel value to delta phase lookup table */
    for (uint32_t i = 0; i < 256; i ++) {
        uint64_t freq_t255 =
            (uint64_t)desc->pixel.low_freq.hz * 255 + ((uint64_t)desc->pixel.bandwidth.hz * i);

        uint64_t dphase = (freq_t255 << 32) / ((uint64_t)(sample_rate) * 255);

        desc->pixel.val_phase_delta[i] = (uint32_t) dphase;
    }

    /* all ok */
    return SSTV_OK;
}
