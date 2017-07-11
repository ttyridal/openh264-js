Note: This project is NOT maintained. I don't provide any support for it.


# OpenH264-js

This is an example of the Cisco's OpenH264 library compiled
to javascript with emscripten for running natively in browsers. 

This is very similar to eg https://github.com/mbebenita/Broadway and
others. The main difference is that they use the BSD decoder which is
limited to h264 baseline profile. Among others, that means no cabac.
OpenH264 (http://www.openh264.org/)  does support cabac.

## Features
 - CABAC decoding
 - Tested in Chrome v43 and Firefox 40
 - single nal decoding, makes it easy to decode any container externally
 - &gt; 30 fps for 1280x720 main profile stream (cabac encoded) on Haswell i7

## Limitiations
 - Lot's of hardcoding in demo (decode.html).

## Build instructions
 * only tested on linux / emcc (Emscripten GCC-like replacement) 1.29.0 (commit fdf24b478e1b26c0362a1627aca49ef82fd53f6a)
 * Get emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/Tutorial.html)
 
* Run make

The Makefile will download all dependencies and build everything.
A small demo is provided in decoder.html. The demo uses Broadway's rendering library.


## Possible improvements

There are a lot of copying going on, Several reasons:

 * A quick look didn't reveal any buffer control in OpenH264 (validity of data pointers are until next decode call?)
 * Broadway's yuv-&gt;rgb conversion needs back-to-back planes with stride = width: OpenH264 doesn't do that.

## Other points
 * closure-compiler, even with at whitespace level makes playback slower and more jittery. No clue why.
