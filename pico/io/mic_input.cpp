#include "io/mic_input.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

MicADC::MicADC(uint8_t adc_num, float vref_volts)
: adc_num_{adc_num},
  adc_pin_{static_cast<unsigned>(26 + adc_num)},
  vref_{vref_volts}
{}

void MicADC::init() {
    adc_init();
    adc_gpio_init(adc_pin_);     // put pin into analog mode
    adc_select_input(adc_num_);  // choose channel
}

uint16_t MicADC::read_raw() const {
    return static_cast<uint16_t>(adc_read() & 0x0FFF); // 12-bit
}

float MicADC::raw_to_volts(uint16_t raw) const {
    return vref_ * (static_cast<float>(raw) / 4095.0f);
}

float MicADC::read_volts() const {
    return raw_to_volts(read_raw());
}

void MicADC::read_block(uint16_t* dst, size_t n) const {
    for (size_t i = 0; i < n; ++i) dst[i] = read_raw();
}
