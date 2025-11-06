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
    MidiMessenger(uart_inst_t* uart): uart(uart) {}

    void send_midi_note_on(Note note) {
        uint8_t msg[3] = {0x90, note.pitch, note.velocity};
        printf("ON %d %d\n", note.pitch, note.velocity);
        for (int i = 0; i < 3; i++) {
            uart_putc(uart, msg[i]);
        }
    }

    void send_midi_note_off(Note note) {
        uint8_t msg[3] = {0x80, note.pitch, 0};
        printf("OFF %d\n", note.pitch);
        for (int i = 0; i < 3; i++) {
            uart_putc(uart, msg[i]);
        }
    }

private:
    uart_inst_t* uart;
};