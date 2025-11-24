#pragma once
#include <cstdint>

// Hz-range bin + amplitude (amplitude is updated every frame)
struct PitchBin {
    const char* name;
    double         freq_min;  
    double         freq_max;   
    float       amplitude; 
};

struct PitchResult {
    //bool        detected;   // <— new: true if gate passed
    float       freq_hz;    // center of dominant bin (quick estimate)
    float       amplitude;  // amplitude of dominant bin
    int         index;      // index into bins
    const char* name;       // name of dominant bin
};

// NOTE: these are now NON-const so can write amplitudes
extern PitchBin PITCH_BINS[];
extern const int BIN_COUNT;


class MicPitchDetector {
public:
    MicPitchDetector(const PitchBin* bins = nullptr, int bin_count = 0);

    void init();                        // calls pico_fft::fft_setup()
    PitchResult update();               // capture + FFT + fill amplitudes + pick dominant
    void print_bins() const;            // debug helper
    void bins_for_plotter() const;      // plotter function

private:
    // keep a mutable, internal copy so amplitudes can be written.
    static constexpr int kMaxBins = 42;
    PitchBin bins_[kMaxBins];
    int      bin_count_;
};