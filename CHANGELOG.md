# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed
- Leader tones were 31.5ms instead of 300ms, due to an overflow in the timing descriptors.
- PD modes transmitted an extra line pair, read from beyond the image buffer.

## [0.9.0] - 2023-08-27

Initial release.
