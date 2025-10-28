#include <stdio.h>
#include <cstdint>
#include <algorithm>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"

#include "input/key_matrix.h"
#include "input/fake_strum_irq.h"

#include "blink.pio.h"

#define LED_PIN 15

void send_midi_note_on(uint8_t note, uint8_t velocity) {
    uint8_t msg[3] = {0x90, note, velocity};
    for (int i = 0; i < 3; i++) {
        uart_putc(uart0, msg[i]);
    }
}

void send_midi_note_off(uint8_t note, uint8_t velocity) {
    uint8_t msg[3] = {0x80, note, velocity};
    for (int i = 0; i < 3; i++) {
        uart_putc(uart0, msg[i]);
    }
}

void receive_strum(std::vector<bool> state) {
    printf("strum state: ");
    for(bool key : state) {
        printf("%d", key);
    } 
    printf("\n");
}

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

    fake_strum_irq::FakeStrumIrq fake_strum;
    fake_strum.set_callback(receive_strum);

    while (true) {
        poll_matrix_once(row_pins, col_pins, keys);

        // simple debug output: list pressed keys
        bool any = false;
        for (int r = 0; r < MATRIX_ROWS; ++r) {
            for (int c = 0; c < MATRIX_COLS; ++c) {
                if (keys[r][c] && !last[r][c]) {
                    printf("Key pressed: row %d col %d\n", r, c);
                    send_midi_note_on(notes[r][c], 100);
                }
                if (!keys[r][c] && last[r][c]) {
                    printf("Key released: row %d col %d\n", r, c);
                    send_midi_note_off(notes[r][c], 0);
                }
            }
        }
        
        std::swap(keys, last);
        sleep_ms(6);
    }
}
