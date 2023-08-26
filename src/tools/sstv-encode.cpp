/*
 * Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>
 *
 * libsstv is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <iostream>
#include <map>
#include <cstdlib>

#include <Magick++.h> 
#include <sndfile.h>

#include <libsstv.h>

#include "args.hxx"

std::map<std::string, sstv_mode_t> stringToModeMap = {
    { "FAX480", SSTV_MODE_FAX480 },
    { "ROBOT_BW8_R", SSTV_MODE_ROBOT_BW8_R },
    { "ROBOT_BW8_G", SSTV_MODE_ROBOT_BW8_G },
    { "ROBOT_BW8_B", SSTV_MODE_ROBOT_BW8_B },
    { "ROBOT_BW12_R", SSTV_MODE_ROBOT_BW12_R },
    { "ROBOT_BW12_G", SSTV_MODE_ROBOT_BW12_G },
    { "ROBOT_BW12_B", SSTV_MODE_ROBOT_BW12_B },
    { "ROBOT_BW24_R", SSTV_MODE_ROBOT_BW24_R },
    { "ROBOT_BW24_G", SSTV_MODE_ROBOT_BW24_G },
    { "ROBOT_BW24_B", SSTV_MODE_ROBOT_BW24_B },
    { "ROBOT_BW36_R", SSTV_MODE_ROBOT_BW36_R },
    { "ROBOT_BW36_G", SSTV_MODE_ROBOT_BW36_G },
    { "ROBOT_BW36_B", SSTV_MODE_ROBOT_BW36_B },
    { "ROBOT_C12", SSTV_MODE_ROBOT_C12 },
    { "ROBOT_C24", SSTV_MODE_ROBOT_C24 },
    { "ROBOT_C36", SSTV_MODE_ROBOT_C36 },
    { "ROBOT_C72", SSTV_MODE_ROBOT_C72 },
    { "SCOTTIE_S1", SSTV_MODE_SCOTTIE_S1 },
    { "SCOTTIE_S2", SSTV_MODE_SCOTTIE_S2 },
    { "SCOTTIE_S3", SSTV_MODE_SCOTTIE_S3 },
    { "SCOTTIE_S4", SSTV_MODE_SCOTTIE_S4 },
    { "SCOTTIE_DX", SSTV_MODE_SCOTTIE_DX },
    { "MARTIN_M1", SSTV_MODE_MARTIN_M1 },
    { "MARTIN_M2", SSTV_MODE_MARTIN_M2 },
    { "MARTIN_M3", SSTV_MODE_MARTIN_M3 },
    { "MARTIN_M4", SSTV_MODE_MARTIN_M4 },
    { "PD50", SSTV_MODE_PD50 },
    { "PD90", SSTV_MODE_PD90 },
    { "PD120", SSTV_MODE_PD120 },
    { "PD160", SSTV_MODE_PD160 },
    { "PD180", SSTV_MODE_PD180 },
    { "PD240", SSTV_MODE_PD240 },
    { "PD290", SSTV_MODE_PD290 },
};

sstv_mode_t mode_from_string(std::string mode)
{
    std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);

    if (auto it = stringToModeMap.find(mode); it != stringToModeMap.end()) {
        return (*it).second;
    } else {
        std::cerr << "Unknown mode '" << mode << "'" << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    /* Parse command line flags */
    args::ArgumentParser parser("Encodes an image into an SSTV audio signal.");
    args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
    args::Flag list(parser, "list", "list supported SSTV modes", { 'l', "list" });
    args::Positional<std::string> modeString(parser, "mode", "the desired SSTV mode", args::Options::Required);
    args::Positional<std::string> input(parser, "input", "input image file", args::Options::Required);
    args::Positional<std::string> output(parser, "output", "output WAV file", args::Options::Required);
    args::Positional<size_t> sample_rate(parser, "sample_rate", "output WAV file", 48000);

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::RequiredError& e) {
        if (!list) {
            std::cerr << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser << std::endl;
        exit(EXIT_FAILURE);
    }

    /* list modes if required */
    if (list) {
        std::cout << "Supported modes:" << std::endl;
        for (const auto& mode : stringToModeMap) {
            std::cout << "  * " << mode.first << std::endl;
        }
        return 0;
    }

    /* parse SSTV mode */
    sstv_mode_t mode = mode_from_string(args::get(modeString));

    /* get image properties for chosen mode */
    uint32_t width, height;
    sstv_image_format_t format;
    if (sstv_get_mode_image_props(mode, &width, &height, &format) != SSTV_OK) {
        std::cerr << "sstv_get_mode_image_props() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* load image from file */
    std::cout << "Loading image from " << args::get(input) << std::endl;

    Magick::Image image;
    uint8_t *image_buffer = NULL;

    try {
        /* load from file */
        image.read(args::get(input));

        /* resize */
        std::cout << "Resizing to " << width << "x" << height << std::endl;
        Magick::Geometry nsize(width, height);
        nsize.aspect(true);
        image.scale(nsize);
    } catch (int e) {
        std::cerr << "Magick++ failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* get raw RGB (and convert it if necessary) */
    image.colorSpace(Magick::sRGBColorspace);
    Magick::PixelData blob(image, "RGB", Magick::CharPixel);
    image_buffer = (uint8_t *)blob.data();

    sstv_image_t sstv_image;
    if (sstv_pack_image(&sstv_image, width, height, SSTV_FORMAT_RGB, image_buffer) != SSTV_OK) {
        std::cerr << "sstv_pack_image() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* convert to mode's colorspace */
    if (sstv_convert_image(&sstv_image, format) != SSTV_OK) {
        std::cerr << "sstv_convert_image() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* create a sample buffer for output */
    int16_t samp_buffer[128 * 1024];
    sstv_signal_t signal;
    if (sstv_pack_signal(&signal, SSTV_SAMPLE_INT16, 128 * 1024, samp_buffer) != SSTV_OK) {
        std::cerr << "sstv_pack_signal() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* initialize library */
    std::cout << "Initializing libsstv" << std::endl;
    if (sstv_init(malloc, free) != SSTV_OK) {
        std::cerr << "Failed to initialize libsstv" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* create encoder context */
    std::cout << "Creating encoding context" << std::endl;
    void *ctx = nullptr;
    if (sstv_create_encoder(&ctx, sstv_image, mode, args::get(sample_rate)) != SSTV_OK) {
        std::cerr << "Failed to create SSTV encoder" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!ctx) {
        std::cerr << "NULL encoder received" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* open WAV file */
    SF_INFO wavinfo;
    wavinfo.samplerate = args::get(sample_rate);
    wavinfo.channels = 1;
    wavinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE *wavfile = sf_open(args::get(output).c_str(), SFM_WRITE, &wavinfo);
    if (!wavfile) {
        std::cerr << "sf_open() failed: " << sf_strerror(NULL) << std::endl;
        exit(EXIT_FAILURE);
    }

    /* encode */
    while (true) {
        /* encode block */
        sstv_error_t rc = sstv_encode(ctx, &signal);
        if (rc != SSTV_ENCODE_SUCCESSFUL && rc != SSTV_ENCODE_END) {
            std::cerr << "sstv_encode() failed with rc " << rc << std::endl;
            exit(EXIT_FAILURE);
        }

        /* write to sound file */
        sf_write_short(wavfile, (int16_t *)signal.buffer, signal.count);
        std::cout << "Written " << signal.count << " samples" << std::endl;

        /* exit case */
        if (rc == SSTV_ENCODE_END) {
            break;
        }
    }

    /* close wav file */
    sf_close(wavfile);

    /* cleanup */
    std::cout << "Cleaning up" << std::endl;
    if (sstv_delete_encoder(ctx) != SSTV_OK) {
        std::cerr << "Failed to delete SSTV encoder" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* all ok */
    std::cout << "Successfuly exited" << std::endl;
    return 0;
}
