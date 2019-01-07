/*
 * Copyright (c) 2018 Vasile Vilvoiu (YO7JBP) <vasi.vilvoiu@gmail.com>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <iostream>
#include <malloc.h>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include "cimg/CImg.h"

extern "C" {
#include <libsstv.h>
}

/*
 * Command line flags
 */

DEFINE_bool(logtostderr, false, "Only log to stderr");
DEFINE_string(mode, "", "SSTV mode for encoder");
DEFINE_string(input, "", "input image");
DEFINE_uint64(sample_rate, 48000, "output audio sample rate");

int main(int argc, char **argv)
{
    /* Parse command line flags */
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    /* Initialize logging */
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    /* check input */
    if (FLAGS_input == "") {
        LOG(FATAL) << "Input image filename not provided, use --input";
    }
    if (FLAGS_mode == "") {
        LOG(FATAL) << "Encoding mode not provided, use --mode";
    }

    /* load image */
    LOG(INFO) << "Loading image from " << FLAGS_input;
    cimg_library::CImg<unsigned char> input_image(FLAGS_input.c_str());
    uint8_t *bytes = input_image.data();

    /* initialize an encoder */
    LOG(INFO) << "Initializing libsstv";
    if (sstv_init(malloc, free) != SSTV_OK) {
        LOG(FATAL) << "Failed to initialize libsstv";
    }

    LOG(INFO) << "Creating encoding context";
    void *ctx = nullptr;
    if (sstv_create_encoder(SSTV_PD120, bytes, FLAGS_sample_rate, &ctx) != SSTV_OK) {
        LOG(FATAL) << "Failed to create SSTV encoder";
    }
    if (!ctx) {
        LOG(FATAL) << "NULL encoder received";
    }

    /* cleanup */
    LOG(INFO) << "Cleaning up";
    if (sstv_delete_encoder(ctx) != SSTV_OK) {
        LOG(FATAL) << "Failed to delete SSTV encoder";
    }

    /* all ok */
    LOG(INFO) << "Successfuly exited";
    return 0;
}