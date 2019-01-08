/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _SSTV_H_
#define _SSTV_H_

#include "libsstv.h"

/*
 * Memory management functions
 */
extern sstv_malloc_t sstv_malloc_user;
extern sstv_free_t sstv_free_user;

/*
 * Utility functions
 */
extern uint8_t
sstv_get_visp_code(sstv_mode_t mode);

#endif