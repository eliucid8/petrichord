#include "io/mic_input.h"
#include <cstdio>

extern "C" {
#include "pico/fft.h"  
//NSAMP, fft_setup(), fft_sample(), fft_process()
}

// pitch-oriented bins (≈80–1300 Hz) **will modify for pitch
PitchBin DEFAULT_PITCH_BINS[] = {
    {"80–110",      80,  110, 0},
    {"110–140",    110,  140, 0},
    {"140–180",    140,  180, 0},
    {"180–230",    180,  230, 0},
    {"230–290",    230,  290, 0},
    {"290–360",    290,  360, 0},
    {"360–450",    360,  450, 0},
    {"450–560",    450,  560, 0},
    {"560–700",    560,  700, 0},
    {"700–880",    700,  880, 0},
    {"880–1100",   880, 1100, 0},
    {"1100–1300", 1100, 1300, 0},
};
const int DEFAULT_PITCH_BIN_COUNT =
    int(sizeof(DEFAULT_PITCH_BINS) / sizeof(DEFAULT_PITCH_BINS[0]));

MicPitchDetector::MicPitchDetector(const PitchBin* bins, int bin_count) {
    if (bins && bin_count > 0) {
        bin_count_ = (bin_count > kMaxBins) ? kMaxBins : bin_count;
        for (int i = 0; i < bin_count_; ++i) {
            bins_[i] = bins[i];
            bins_[i].amplitude = 0.f;
        }
    } else {
        bin_count_ = (DEFAULT_PITCH_BIN_COUNT > kMaxBins)
                   ? kMaxBins : DEFAULT_PITCH_BIN_COUNT;
        for (int i = 0; i < bin_count_; ++i) {
            bins_[i] = DEFAULT_PITCH_BINS[i];    // copy defaults into mutable buffer
            bins_[i].amplitude = 0.f;
        }
    }
}

void MicPitchDetector::init() {
    fft_setup();
}

PitchResult MicPitchDetector::update() {
    static uint8_t capture_buf[NSAMP];

    //temporary array of frequency_bin_t for FFT
    frequency_bin_t tmp[kMaxBins];
    for (int i = 0; i < bin_count_; ++i) {
        tmp[i].name      = bins_[i].name;
        tmp[i].freq_min  = bins_[i].freq_min;
        tmp[i].freq_max  = bins_[i].freq_max;
        tmp[i].amplitude = 0.f;
    }

    //sample & process
    fft_sample(capture_buf);
    fft_process(capture_buf, tmp, bin_count_);

    //copy amplitudes back to mutable bins
    for (int i = 0; i < bin_count_; ++i) {
        bins_[i].amplitude = tmp[i].amplitude;
    }

    //dominant pitch
    int best = 0;
    float best_amp = bins_[0].amplitude;
    for (int i = 1; i < bin_count_; ++i) {
        if (bins_[i].amplitude > best_amp) {
            best = i; best_amp = bins_[i].amplitude;
        }
    }

    float center_hz = 0.5f * (bins_[best].freq_min + bins_[best].freq_max);
    return PitchResult{ center_hz, best_amp, best, bins_[best].name };
    //returns frequency, amplitude, indexing, name
}

void MicPitchDetector::print_bins() const {
    for (int i = 0; i < bin_count_; ++i) {
        std::printf("[%s %d..%d] amp=%.3f  ",
            bins_[i].name, bins_[i].freq_min, bins_[i].freq_max, bins_[i].amplitude);
    }
    std::printf("\n");
}


/*
Notes: 
- NSAMP: # of samples per FFT frame 

- fft_setup(): Initializes the ADC and DMA configurations for capturing analog signals. 
    Sets up the FFT parameters such as sampling rate and frequency bins.

- fft_sample(uint8_t *capture_buf): Captures a buffer of analog samples from the ADC using DMA. 
    The captured data is stored in the provided buffer.

- fft_process(uint8_t *capture_buf, frequency_bin_t *bins, int bin_count): 
    Processes the captured samples using FFT, calculates the frequency spectrum 
    and stores the results in the provded bins.

- Amp: sum of magnitudes across all FFT bins 
*/