#pragma once
#include <cstdint>   // uint8_t, uint16_t
#include <cstddef>   // size_t

/**
 * Simple API for reading an analog microphone on a Pico ADC pin.
 * Default: ADC0 on GP26, vref = 3.3 V.
 */
class MicADC {
public:
    MicADC(uint8_t adc_num = 0, float vref_volts = 3.3f);

    void init();

    // 12-bit raw sample (0..4095)
    uint16_t read_raw() const;

    float raw_to_volts(uint16_t raw) const;
    
    float read_volts() const;

    // Fill a buffer with N raw samples (blocking)
    void read_block(uint16_t* dst, size_t n) const;

private:
    uint8_t  adc_num_;  // 0..2 → GP26/27/28
    unsigned adc_pin_;  // 26/27/28
    float    vref_;     // volts
};
