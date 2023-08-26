# libsstv
SSTV C encoder/decoder library for embedded systems.

## Usage:
```
./sstv-encode
    -input (input image) type: string default: ""
    -mode (SSTV mode for encoder) type: string default: ""
    -output (output WAV file) type: string default: ""
    -sample_rate (output audio sample rate) type: uint64 default: 48000
```

## Install

install dependencies (tested on devuan 4 chimaera - debian 11 bullseye based and debian 11 on raspberry pi):
```
sudo apt update
sudo apt install libgoogle-glog-dev libgflags-dev libmagick++-dev libsndfile1-dev make cmake
```
go to the `bin` folder, run cmake and then compile with make:
```
cd bin
cmake ..
make
```
now you can run `./sstv-encode`

## Acknowledgements

Taywee/args library by Taylor C. Richberger and Pavel Belikov, released under the MIT license.
