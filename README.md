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

## License

Copyright (c) 2018-2023 Vasile Vilvoiu (YO7JBP) <vasi@vilvoiu.ro>

libsstv is free software; you can redistribute it and/or modify it under the terms of the MIT license. See LICENSE for details.

## Acknowledgements

Taywee/args library by Taylor C. Richberger and Pavel Belikov, released under the MIT license.
