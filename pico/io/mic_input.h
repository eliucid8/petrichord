#pragma once
#include <cstdint>   // uint8_t, uint16_t
#include <cstddef>   // size_t
#include "pico/stdlib.h"
#include "pico/fft.h"

void main() {
  fft_setup();

  uint8_t capture_buf[NSAMP];
  frequency_bin_t bins[BIN_COUNT]; // Define BIN_COUNT according to your requirements

  while (true) {
    fft_sample(capture_buf);
    fft_process(capture_buf, bins, BIN_COUNT);
    // Process or display the results
  }
}
class mic_input {
public:
    mic_input(uint8_t adc_num = 0, float vref_volts = 3.3f);

    void init();

    uint16_t read_raw() const;    // 12-bit raw sample (0..4095)

    float raw_to_volts(uint16_t raw) const;
    
    float read_volts() const;

    // Fill buffer with N raw samples (blocking)
    void read_block(uint16_t* dst, size_t n) const;

private:
    uint8_t  adc_num_;  // 0..2 → GP26/27/28
    unsigned adc_pin_;  // 26/27/28
    float    vref_;     // vin 
};
