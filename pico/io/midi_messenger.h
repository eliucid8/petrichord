#pragma once

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include <stdio.h>
#include <cstdint>

/**
 * provides an API for MIDI messages.
 */
class MidiMessenger {
public:
    MidiMessenger(uart_inst_t* uart, const bool debug): uart(uart), debug(debug) {}

    void send_midi_note_on(Note note) {
        uint8_t msg[3] = {0x90, note.pitch, note.velocity};
        if(debug) {
            printf("ON %d %d\n", note.pitch, note.velocity);
        }
        for (int i = 0; i < 3; i++) {
            uart_putc(uart, msg[i]);
        }
    }

    void send_midi_note_off(Note note) {
        uint8_t msg[3] = {0x80, note.pitch, 0};
        if(debug) {
            printf("OFF %d\n", note.pitch);
        }
        for (int i = 0; i < 3; i++) {
            uart_putc(uart, msg[i]);
        }
    }
    
    /**
     * Sends a pitch bend message over MIDI
     * Pitch bend is adjustable within a 14-bit range, i.e. 0 - 16383
     * The middle value is 8192
     */
    void send_pitch_bend(int bend) {
        uint8_t lower = bend & 0x007F;
        uint8_t upper = bend & 0x3F80 >> 7;
    }

private:
    uart_inst_t* uart;
    const bool debug;
};