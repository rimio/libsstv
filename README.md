# libsstv

*NOTE: The current pre-release version of the library only supports encoding of images in a multitude of modes. Decoding support is planned for the 1.0.0 version, but as of yet no work has been done in that direction.*

SSTV C encoder/decoder library suitable for both desktop and embedded applications.

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

## Acknowledgements

Taywee/args library by Taylor C. Richberger and Pavel Belikov, released under the MIT license.
