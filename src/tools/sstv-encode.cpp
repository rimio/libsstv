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

#include <libsstv.h>

/*
 * Command line flags
 */
DEFINE_bool(logtostderr, false, "Only log to stderr");
DEFINE_string(mode, "", "SSTV mode for encoder");
DEFINE_string(input, "", "input image");
DEFINE_string(output, "", "output WAV file");
DEFINE_uint64(sample_rate, 48000, "output audio sample rate");

sstv_mode_t mode_from_string(std::string mode)
{
    std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);

    if (mode == "FAX480") {
        return SSTV_MODE_FAX480;
    } else if (mode == "ROBOT_BW8_R"){
        return SSTV_MODE_ROBOT_BW8_R;
    } else if (mode == "ROBOT_BW8_G"){
        return SSTV_MODE_ROBOT_BW8_G;
    } else if (mode == "ROBOT_BW8_B"){
        return SSTV_MODE_ROBOT_BW8_B;
    } else if (mode == "ROBOT_BW12_R") {
        return SSTV_MODE_ROBOT_BW12_R;
    } else if (mode == "ROBOT_BW12_G") {
        return SSTV_MODE_ROBOT_BW12_G;
    } else if (mode == "ROBOT_BW12_B") {
        return SSTV_MODE_ROBOT_BW12_B;
    } else if (mode == "ROBOT_BW24_R") {
        return SSTV_MODE_ROBOT_BW24_R;
    } else if (mode == "ROBOT_BW24_G") {
        return SSTV_MODE_ROBOT_BW24_G;
    } else if (mode == "ROBOT_BW24_B") {
        return SSTV_MODE_ROBOT_BW24_B;
    } else if (mode == "ROBOT_BW36_R") {
        return SSTV_MODE_ROBOT_BW36_R;
    } else if (mode == "ROBOT_BW36_G") {
        return SSTV_MODE_ROBOT_BW36_G;
    } else if (mode == "ROBOT_BW36_B") {
        return SSTV_MODE_ROBOT_BW36_B;
    } else if (mode == "ROBOT_C12") {
        return SSTV_MODE_ROBOT_C12;
    } else if (mode == "ROBOT_C24") {
        return SSTV_MODE_ROBOT_C24;
    } else if (mode == "ROBOT_C36") {
        return SSTV_MODE_ROBOT_C36;
    } else if (mode == "ROBOT_C72") {
        return SSTV_MODE_ROBOT_C72;
    } else if (mode == "SCOTTIE_S1") {
        return SSTV_MODE_SCOTTIE_S1;
    } else if (mode == "SCOTTIE_S2") {
        return SSTV_MODE_SCOTTIE_S2;
    } else if (mode == "SCOTTIE_S3") {
        return SSTV_MODE_SCOTTIE_S3;
    } else if (mode == "SCOTTIE_S4") {
        return SSTV_MODE_SCOTTIE_S4;
    } else if (mode == "SCOTTIE_DX") {
        return SSTV_MODE_SCOTTIE_DX;
    } else if (mode == "MARTIN_M1") {
        return SSTV_MODE_MARTIN_M1;
    } else if (mode == "MARTIN_M2") {
        return SSTV_MODE_MARTIN_M2;
    } else if (mode == "MARTIN_M3") {
        return SSTV_MODE_MARTIN_M3;
    } else if (mode == "MARTIN_M4") {
        return SSTV_MODE_MARTIN_M4;
    } else if (mode == "PD50") {
        return SSTV_MODE_PD50;
    } else if (mode == "PD90") {
        return SSTV_MODE_PD90;
    } else if (mode == "PD120") {
        return SSTV_MODE_PD120;
    } else if (mode == "PD160") {
        return SSTV_MODE_PD160;
    } else if (mode == "PD180") {
        return SSTV_MODE_PD180;
    } else if (mode == "PD240") {
        return SSTV_MODE_PD240;
    } else if (mode == "PD290") {
        return SSTV_MODE_PD290;
    } else {
        LOG(FATAL) << "Unknown mode '" << mode << "'";
    }
}

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
    sstv_mode_t mode = mode_from_string(FLAGS_mode);

    /* get image properties for chosen mode */
    uint32_t width, height;
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
    image.colorSpace(Magick::sRGBColorspace);
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