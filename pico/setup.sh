#!/bin/bash

git submodule update --init --recursive
if [[ -z "${PICO_EXTRAS_PATH+x}" ]]; then
    export PICO_EXTRAS_PATH="$(pwd)/pico-extras/"
fi
if [[ -z "${PICO_FFT_PATH+x}" ]]; then
    export PICO_FFT_PATH="$(pwd)/pico_fft/"
fi