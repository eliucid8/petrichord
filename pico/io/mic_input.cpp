#include "io/mic_input.h"
#include <cstdio>
extern "C" {
#include "pico/fft.h"
}


// pitch-oriented bins (≈80–1300 Hz) **will modify for pitch
//range between 200 - 2000 for voice
//right now only using baseline between 200 - 13000 
/*
PitchBin PITCH_BINS[] = {
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
*/

// name, min, max, amplitude, midi 
PitchBin PITCH_BINS[] = {
    {"C3",  127.15,  134.7, 0, 48},         //130.8
    {"C#3", 134.7,  142.7, 0, 49},           //138.6
    {"D3",  142.7,  151.18, 0, 50},         //146.8
    {"D#3", 151.18,  160.185, 0, 51},       //155.56
    {"E3",  160.185,  169.71, 0, 52},       //164.81
    {"F3",  169.71,  179.805, 0, 53},       //174.61 
    {"F#3", 179.805,  190.5, 0, 54},        //185 
    {"G3",  190.5,  201.825, 0, 55},        //196 
    {"G#3", 201.825,  213.825, 0, 56},      //207.65 
    {"A3",  213.825,  226.54, 0, 57},       //220 
    {"A#3", 226.54,  240.01, 0, 58},        //233.08
    {"B",   240.01,  254.285, 0, 59},       //246.94 
    {"C4",  254.285,  269.405, 0, 60},      //261.63 - 60 
    {"C#4", 269.285,  285.42, 0, 61},       //277.18 - 61
    {"D4",  285.42, 302.395, 0, 62},        //293.66 - 62
    {"D#4", 302.395, 320.38, 0, 63},        //311.13 - 63
    {"E4",  320.38,  339.43, 0, 64},        //329.63
    {"F4",  339.43,  358.11, 0, 65},        //349.23 
    {"F#4", 358.11,  379.495, 0, 66},       //366.99
    {"G4",  379.495,  403.65, 0, 67},       //392
    {"G#4", 403.65, 427.65, 0, 68},         //415
    {"A4",  427.65, 453.08, 0, 69},         //440
    {"A#4", 453.08,  480.02, 0, 70},        //466.16
    {"B4",  480.02, 508.565, 0, 71},        //493.88
    {"C5",  508.565, 538.825, 0, 72},       //523.25
    {"C#5", 538.825,  570.865, 0, 73},      //554.4
    {"D5",  570.865, 604.79, 0, 74},        //587.33
    {"D#5", 604.79, 640.75, 0, 75},         //622.25
    {"E5",  640.75,  678.855, 0, 76},       //659.25
    {"F5",  678.855,  719.225, 0, 77},      //698.46
    {"F#5", 719.225,  761.99, 0, 78},       //739.99
    {"G5",  761.99,  807.3, 0, 79},         //783.99
    {"G#5", 807.3, 855.305, 0, 80},         //830.61
    {"A5",  855.305, 901.665, 0, 81},       //880
    {"A#5", 901.665,  955.55, 0, 82},       //923.33
    {"B5",  955.55, 1017.135, 0, 83},       //987.77
    {"C6",  1017.135, 1077.615, 0, 84},     //1046.5
    {"C#6", 1077.615,  1141.695, 0, 85},    //1108.73
    {"D6",  1141.695, 1209.585, 0, 86},     //1174.66
    {"D#6", 1209.585, 1281.51, 0, 87},      //1244.51
    {"E6",  1281.51,  1357.71, 0, 88},      //1318.51 - out of vocal range
}; 

const int BIN_COUNT =
    int(sizeof(PITCH_BINS) / sizeof(PITCH_BINS[0]));

MicPitchDetector::MicPitchDetector(const PitchBin* bins, int bin_count) {
    if (bins && bin_count > 0) {
        bin_count_ = (bin_count > kMaxBins) ? kMaxBins : bin_count;
        for (int i = 0; i < bin_count_; ++i) {
            bins_[i] = bins[i];
            bins_[i].amplitude = 0.f;
        }
    } else {
        bin_count_ = (BIN_COUNT > kMaxBins)
                   ? kMaxBins : BIN_COUNT;
        for (int i = 0; i < bin_count_; ++i) {
            bins_[i] = PITCH_BINS[i];    // copy defaults into mutable buffer
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
    return PitchResult{center_hz, best_amp, best, bins_[best].name, bins_[best].midi};
    //returns frequency, amplitude, indexing, name
}

void MicPitchDetector::print_bins() const {
    for (int i = 0; i < bin_count_; ++i) {
        std::printf("[%s %f..%f midi=%d] amp=%.3f  ",
            bins_[i].name, bins_[i].freq_min, bins_[i].freq_max, bins_[i].midi, bins_[i].amplitude);
    }
    std::printf("\n");
}

void MicPitchDetector::bins_for_plotter() const {
    // One line per FFT frame:
    // BINS <amp0> <amp1> <amp2> ... <ampN-1>
    std::printf("BINS");
    for (int i = 0; i < BIN_COUNT; ++i) {
        std::printf(" %.3f", bins_[i].amplitude);
    }
    std::printf("\n");
}

