/*
 * Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _SSTV_H_
#define _SSTV_H_

#include "libsstv.h"

/*
 * Mode timings and frequency descriptors
 */
typedef struct {
    uint32_t hz;
    uint32_t phase_delta;
} sstv_freq_desc_t;

typedef struct {
    uint32_t nsec;
    uint32_t usamp;
} sstv_timing_desc_t;

typedef struct {
    /* Header */
    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } leader_tone;

    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } break_tone;

    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t sep_freq;
        sstv_freq_desc_t low_freq;
        sstv_freq_desc_t high_freq;
    } vis;

    /* Mode */
    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } sync;

    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } porch;

    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } porch2;

    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } separator;

    struct {
        sstv_timing_desc_t time;
        sstv_freq_desc_t freq;
    } separator2;

    struct {
        sstv_timing_desc_t time;
        sstv_timing_desc_t time2;
        sstv_freq_desc_t low_freq;
        sstv_freq_desc_t bandwidth;

        /* lookup table from value to delta-phase */
        uint32_t val_phase_delta[256];
    } pixel;
} sstv_mode_descriptor_t;

/*
 * Memory management
 */
extern sstv_malloc_t sstv_malloc_user;
extern sstv_free_t sstv_free_user;

/*
 * Utility functions
 */
extern sstv_error_t
sstv_get_mode_descriptor(sstv_mode_t mode, uint32_t sample_rate, sstv_mode_descriptor_t *desc);

#endif
