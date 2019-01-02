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
    /* Keep user functions for allocation/deallocation */
    sstv_malloc_user = alloc_func;
    sstv_free_user = dealloc_func;
    return SSTV_OK;
}