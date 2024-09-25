# libsstv

*NOTE: The current pre-release version of the library only supports encoding of images in a multitude of modes. Decoding support is planned for the 1.0.0 version, but as of yet no work has been done in that direction.*

SSTV encoder/~~decoder~~ C library suitable for both desktop and embedded applications.

## Sample

[Output](/test/test-image.wav) for `PD180` mode of [this test image](/test/test-image.bmp).

## Supported modes

The following modes are currently supported:

* `FAX480`
* `MARTIN_M1` `MARTIN_M2` `MARTIN_M3` `MARTIN_M4`
* `PD50` `PD90` `PD120` `PD160` `PD180` `PD240` `PD290`
* `ROBOT_BW8_B` `ROBOT_BW8_G` `ROBOT_BW8_R` `ROBOT_BW12_B` `ROBOT_BW12_G` `ROBOT_BW12_R` `ROBOT_BW24_B` `ROBOT_BW24_G` `ROBOT_BW24_R` `ROBOT_BW36_B` `ROBOT_BW36_G` `ROBOT_BW36_R`
* `ROBOT_C12` `ROBOT_C24` `ROBOT_C36` `ROBOT_C72`
* `SCOTTIE_DX` `SCOTTIE_S1` `SCOTTIE_S2` `SCOTTIE_S3` `SCOTTIE_S4`

## Building and installing

The following packaged options are available for `libsstv`:
* [Arch User Repository package](https://aur.archlinux.org/packages/libsstv)
* [Gentoo ebuild file](https://github.com/rimio/gentoo-overlay/tree/master/media-libs/libsstv)
* [MacPorts package](https://ports.macports.org/port/libsstv/)

Compiling the library and encoding tool:

```
cmake .
make
```

This will generate the following files:
- `lib/libsstv.so` - dynamic linking version
- `include/libsstv.h` - C header file for library
- `bin/sstv-encode` - encoding tool

If you want to skip building the encoding tool (and thus its dependencies) then you can do so by turning off the `BUILD_TOOLS` flag:
```
cmake . -DBUILD_TOOLS=OFF
make
```

Installation can be performed in the following manner:
```
cmake . -DCMAKE_INSTALL_PREFIX=<install_prefix>
make
make install
```

## Dependencies

The library has no dependecies.

Building the encoding tool requires `ImageMagick++` and `libsndfile`.

To install these packages in Ubuntu:
```
apt install libmagick++-dev libsndfile1-dev
```

To install these packages in ArchLinux:
```
pacman -S imagemagick libsndfile
```

## Conversion tool usage

The conversion tool takes four parameters: the desired mode, input image filename, output WAV filename and sampling rate. The last parameter can be omitted and defaults to `48000`Hz.

Example usage:
```
./sstv-encode pd90 ../test/test-image.bmp test.wav 44800
```

The above call produces `test.wav` in the current directory.

## Library usage

### Return codes

Each library function returns an error code that it's advisable you check. A simple check for `SSTV_OK` is enough, unless you want to handle particular errors differently.

### Initialization

Library initialization is done with a call to `sstv_init(alloc_func, dealloc_func)`, where the two parameters are function pointers to the allocation and deallocation functions you want `libsstv` to use internally (e.g. `malloc` and `free`).

If the library is not initialized it can still be used, but only a precompiled maximum number of encoders (`DEFAULT_ENCODER_CONTEXT_COUNT`, default value `4`) can be used simultaneously. This number can be adjusted by passing it to `cmake` at compile time:

```
cmake . -DDEFAULT_ENCODER_CONTEXT_COUNT=8
```

If you only encode one image at a time it is safe to skip initialization.

Moreover, if you do not call `sstv_init()` you will not be able to use further APIs that would require memory allocation to be performed within (see the section on _Images_).

### Images

Before encoding and decoding, an image object must be created. The image buffer can either be allocated or it can be provided.

The quickest way is to create a valid image and allocate its buffer is to call `sstv_create_image_from_mode()`. This function requires that a call to `sstv_init()` has been made (see _Initializetion_ above).

```
sstv_image_t image;
if (sstv_create_image_from_mode(&image, SSTV_MODE_FAX480) != SSTV_OK) {
    ... error handling ...
}
... write to the image buffer ...
```

Otherwise, the user would start by retrieving the image properties of a specific mode:

```
uint32_t width, height;
sstv_image_format_t format;
if (sstv_get_mode_image_props(SSTV_MODE_FAX480, &width, &height, &format) != SSTV_OK) {
    ... error handling ...
}
```

and either rely on `sstv_create_image_from_props()` (which _also_ relies on `sstv_init()`) to allocate an image buffer:

```
sstv_image_t image;
if (sstv_create_image_from_props(&image, width, height, format) != SSTV_OK) {
    ... error handling ...
}
... write to the image buffer ...
```

or use an existing buffer to initialize an image structure with `sstv_pack_image()`:

```
uint8_t *buffer = ...;

sstv_image_t image;
if (sstv_pack_image(&image, width, height, format, buffer) != SSTV_OK) {
    ... error handling ...
}
```

Only images initialized with `sstv_create_image_from_mode()` and `sstv_create_image_from_props()` must be destroyed with `sstv_delete_image()`.

Pixel format conversions can be performed in-place on an image by calling `sstv_convert_image()`, with the limitation that the target format must fit within the original memory (e.g. grayscale to RGB conversions will fail with `SSTV_UNSUPPORTED_CONVERSION`).

### Signal management

Signals (`sstv_signal_t`) are objects that hold a chunk of the raw audio. They can only be created on a preallocated buffer:

```
int16_t signal_buffer[SIGNAL_BUFFER_CAPACITY];
sstv_signal_t signal;
if (sstv_pack_signal(&signal, SSTV_SAMPLE_INT16, SIGNAL_BUFFER_CAPACITY, signal_buffer) != SSTV_OK) {
    ... error handling ...
}
```

### Encoding

#### Encoder management

You can use `sstv_create_encoder()` and `sstv_delete_encoder()` to request and return an encoder context to the library.

```
sstv_image_t image;
... set up image ...

const size_t SAMPLE_RATE = 48000

void *ctx;
if (sstv_create_encoder(&ctx, image, SSTV_MODE_FAX480, SAMPLE_RATE) != SSTV_OK) {
    ... error handling ...
}

... encode the data ...

if (sstv_delete_encoder(ctx) != SSTV_OK) {
    ... error handling ...
}
```

#### Encoding of data

The actual encoding is performed with multiple calls to `sstv_encode()`, until the whole output has been produced:

```
void *ctx;
... encoder context creation ...

sstv_signal_t signal;
... signal packing ...

sstv_error_t rc;
do {
    rc = sstv_encode(ctx, &signal);
    if (rc != SSTV_ENCODE_SUCCESSFUL && rc != SSTV_ENCODE_END) {
        ... error handling ...
    }
} while (rc != SSTV_ENCODE_END);

... encoder context disposal ...
```

Note that `sstv_encode()` does not return `SSTV_OK` on success but `SSTV_ENCODE_SUCCESSFUL`.

The library does not allocate further memory than that allocated for the images or that provided by the user via images or signals.

## License

Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>

libsstv is free software; you can redistribute it and/or modify it under the terms of the MIT license. See LICENSE for details.

## Acknowledgements

Taywee/args library by Taylor C. Richberger and Pavel Belikov, released under the MIT license.
