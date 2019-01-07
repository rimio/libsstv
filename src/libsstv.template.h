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
    SSTV_PD90,
    SSTV_PD120,
    SSTV_PD160,
    SSTV_PD180,
    SSTV_PD240,
} sstv_mode_t;

/*
 * Memory management
 */
typedef void* (*sstv_malloc_t)(size_t);
typedef void (*sstv_free_t)(void *);

/*
 * Routines
 */
extern sstv_error_t sstv_init(sstv_malloc_t alloc_func, sstv_free_t dealloc_func);

extern sstv_error_t sstv_create_encoder(sstv_mode_t mode, uint8_t *image, size_t sample_rate, void **out_ctx);
extern sstv_error_t sstv_delete_encoder(void *ctx);
extern sstv_error_t sstv_encode(void *ctx, uint8_t *buffer, size_t buffer_size);

#endif