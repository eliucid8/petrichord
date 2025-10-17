#include <stdio.h>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"

#include "input/key_matrix.h"

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint8_t sm, uint8_t offset, uint8_t pin, uint8_t freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

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

int main()
{
    stdio_init_all();
    
    uart_init(uart0, 31250);
    gpio_set_function(0, GPIO_FUNC_UART); // TX on GP0

    const uint8_t row_pins[MATRIX_ROWS] = {2, 3, 4, 5};
    const uint8_t col_pins[MATRIX_COLS] = {8, 9};

    configure_matrix_pins(row_pins, col_pins);

    bool keys[MATRIX_ROWS][MATRIX_COLS] = {0};

    // PIO Blinking example
    // PIO pio = pio0;
    // uint8_t offset = pio_add_program(pio, &blink_program);
    // printf("Loaded program at %d\n", offset);
    
    // #ifdef PICO_DEFAULT_LED_PIN
    // blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
    // #else
    // blink_pin_forever(pio, 0, offset, 6, 3);
    // #endif
    // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    // Timer example code - This example fires off the callback after 2000ms
    // add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    while (true) {
        poll_matrix_once(row_pins, col_pins, keys);

        // simple debug output: list pressed keys
        bool any = false;
        for (int r = 0; r < MATRIX_ROWS; ++r) {
            for (int c = 0; c < MATRIX_COLS; ++c) {
                if (keys[r][c]) {
                    printf("Key pressed: row %d col %d\n", r, c);
                    any = true;
                }
            }
        }
        if (!any) {
            // optional heartbeat
            printf("No keys\n");
        }

        sleep_ms(100);
    }
}
