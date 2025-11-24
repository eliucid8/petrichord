#include <memory>
#include <stdio.h>
#include <cstdint>
#include <algorithm>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"

#include "io/key_matrix.h"
#include "io/strum_irq.h"
#include "controllers/chord_controller.h"
#include "io/midi_messenger.h"
#include "io/mic_input.h"
extern "C" {
#include "pico/fft.h"
}


static ChordController* g_chord_controller = nullptr;

#define LED_PIN 15


int main()
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    uart_init(uart0, 31250);
    gpio_set_function(0, GPIO_FUNC_UART); // TX on GP0
    
    const uint8_t row_pins[MATRIX_ROWS] = {2, 3, 4, 5};
    const uint8_t col_pins[MATRIX_COLS] = {8, 9};

    configure_matrix_pins(row_pins, col_pins);

    bool keys[MATRIX_ROWS][MATRIX_COLS] = {0};
    bool last[MATRIX_ROWS][MATRIX_COLS] = {0};
    uint8_t notes[MATRIX_ROWS][MATRIX_COLS] = {
        {60, 62},
        {64, 65},
        {67, 69},
        {71, 72},
    };

    MidiMessenger midi_messenger(uart0);
    ChordController chord_controller(&midi_messenger);
    g_chord_controller = &chord_controller;

    std::unique_ptr<strum_irq::StrumIrq> fake_strum = strum_irq::CreateFakeStrumIrq(nullptr);
    fake_strum->set_callback([](std::vector<bool> states) {
        if (g_chord_controller) {
            g_chord_controller->handle_strum(states);
        }
    });\

    MicPitchDetector pitch;
    pitch.init(); 
    const float min_sound = 000.0f; //smallest note

    while (true) {

        //mic:
        auto pr = pitch.update();   //updates mic info
        pitch.bins_for_plotter();   //prints amplitude frequencies 

        if(pr.amplitude >= min_sound){
            printf("Pitch frequency: ~%.1f Hz  bin=%s  amp=%.3f\n",
            pr.freq_hz, pr.name, pr.amplitude);
        } 
        else{
            printf(" no pitch / too quiet\n");
        }

        poll_matrix_once(row_pins, col_pins, keys);

        // simple debug output: list pressed keys
        bool any = false;
        for (int r = 0; r < MATRIX_ROWS; ++r) {
            for (int c = 0; c < MATRIX_COLS; ++c) {
                if (keys[r][c] && !last[r][c]) {
                    // printf("Key pressed: row %d col %d\n", r, c);
                    // send_midi_note_on(notes[r][c], 100);
                    g_chord_controller->update_chord(generate_chord(notes[r][c], MAJOR_INTERVALS, 4));
                }
                if (!keys[r][c] && last[r][c]) {
                    // printf("Key released: row %d col %d\n", r, c);
                    // send_midi_note_off(notes[r][c], 0);
                }
            }
        }
        
        std::swap(keys, last);
        sleep_ms(10 /*6*/);
    }
}