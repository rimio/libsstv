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
#include <Magick++.h> 
#include <sndfile.h>

extern "C" {
#include <libsstv.h>
}

/*
 * Command line flags
 */

DEFINE_bool(logtostderr, false, "Only log to stderr");
DEFINE_string(mode, "", "SSTV mode for encoder");
DEFINE_string(input, "", "input image");
DEFINE_string(output, "", "output WAV file");
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
    if (FLAGS_output == "") {
        LOG(FATAL) << "Output WAV file not provided, use --output";
    }
    if (FLAGS_mode == "") {
        LOG(FATAL) << "Encoding mode not provided, use --mode";
    }

    /* TODO: parse SSTV mode */
    sstv_mode_t mode = SSTV_MODE_PD120;

    /* get image properties for chosen mode */
    size_t width, height;
    sstv_image_format_t format;
    if (sstv_get_mode_image_props(mode, &width, &height, &format) != SSTV_OK) {
        LOG(FATAL) << "sstv_get_mode_image_props() failed";
    }

    /* load image from file */
    LOG(INFO) << "Loading image from " << FLAGS_input;

    Magick::Image image;
    uint8_t *image_buffer = NULL;

    try {
        /* load from file */
        image.read(FLAGS_input);

        /* resize */
        LOG(INFO) << "Resizing to " << width << "x"<<height;
        Magick::Geometry nsize(width, height);
        nsize.aspect(true);
        image.scale(nsize);
    } catch (int e) {
        LOG(FATAL) << "Magick++ failed";
    }

    /* get raw RGB (and convert it if necessary) */
    image.colorSpace(Magick::RGBColorspace);
    Magick::PixelData blob(image, "RGB", Magick::CharPixel);
    image_buffer = (uint8_t *)blob.data();

    sstv_image_t sstv_image;
    if (sstv_pack_image(&sstv_image, width, height, SSTV_FORMAT_RGB, image_buffer) != SSTV_OK) {
        LOG(INFO) << image_buffer;
        LOG(FATAL) << "sstv_pack_image() failed";
    }

    /* convert to mode's colorspace */
    if (sstv_convert_image(&sstv_image, format) != SSTV_OK) {
        LOG(FATAL) << "sstv_convert_image() failed";
    }

    /* create a sample buffer for output */
    int16_t samp_buffer[128 * 1024];
    sstv_signal_t signal;
    if (sstv_pack_signal(&signal, SSTV_SAMPLE_INT16, 128 * 1024, samp_buffer) != SSTV_OK) {
        LOG(FATAL) << "sstv_pack_signal() failed";
    }

    /* initialize library */
    LOG(INFO) << "Initializing libsstv";
    if (sstv_init(malloc, free) != SSTV_OK) {
        LOG(FATAL) << "Failed to initialize libsstv";
    }

    /* create encoder context */
    LOG(INFO) << "Creating encoding context";
    void *ctx = nullptr;
    if (sstv_create_encoder(&ctx, sstv_image, mode, FLAGS_sample_rate) != SSTV_OK) {
        LOG(FATAL) << "Failed to create SSTV encoder";
    }
    if (!ctx) {
        LOG(FATAL) << "NULL encoder received";
    }

    /* open WAV file */
    SF_INFO wavinfo;
    wavinfo.samplerate = FLAGS_sample_rate;
    wavinfo.channels = 1;
    wavinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE *wavfile = sf_open(FLAGS_output.c_str(), SFM_WRITE, &wavinfo);
    if (!wavfile) {
        LOG(FATAL) << "sf_open() failed: " << sf_strerror(NULL);
    }

    /* encode */
    while (true) {
        /* encode block */
        sstv_error_t rc = sstv_encode(ctx, &signal);
        if (rc != SSTV_ENCODE_SUCCESSFUL && rc != SSTV_ENCODE_END) {
            LOG(FATAL) << "sstv_encode() failed with rc " << rc;
        }

        /* write to sound file */
        sf_write_short(wavfile, (int16_t *)signal.buffer, signal.count);
        LOG(INFO) << "Written " << signal.count << " samples";

        /* exit case */
        if (rc == SSTV_ENCODE_END) {
            break;
        }
    }

    /* close wav file */
    sf_close(wavfile);

    /* cleanup */
    LOG(INFO) << "Cleaning up";
    if (sstv_delete_encoder(ctx) != SSTV_OK) {
        LOG(FATAL) << "Failed to delete SSTV encoder";
    }

    /* all ok */
    LOG(INFO) << "Successfuly exited";
    return 0;
}