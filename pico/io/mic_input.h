#pragma once
#include <cstdint>

// Hz-range bin + amplitude (amplitude is updated every frame)
struct PitchBin {
    const char* name;
    int         freq_min;  
    int         freq_max;   
    float       amplitude; 
};

struct PitchResult {
    float       freq_hz;    // center of dominant bin (quick estimate)
    float       amplitude;  // amplitude of dominant bin
    int         index;      // index into bins
    const char* name;       // name of dominant bin
};

// NOTE: these are now NON-const so we can write amplitudes
extern PitchBin DEFAULT_PITCH_BINS[];
extern const int DEFAULT_PITCH_BIN_COUNT;

class MicPitchDetector {
public:
    // If bins==nullptr or bin_count<=0, uses DEFAULT_PITCH_BINS in-place (mutable).
    // If you pass your own bins, we copy them into an internal mutable buffer.
    MicPitchDetector(const PitchBin* bins = nullptr, int bin_count = 0);

    void init();                 // calls pico_fft::fft_setup()
    PitchResult update();        // capture + FFT + fill amplitudes + pick dominant
    void print_bins() const;     // debug helper

private:
    // We always keep a mutable, internal copy so amplitudes can be written.
    static constexpr int kMaxBins = 32;
    PitchBin bins_[kMaxBins];
    int      bin_count_;
};
