#include <stdio.h>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
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

    // PIO Blinking example
    // PIO pio = pio0;
    // uint offset = pio_add_program(pio, &blink_program);
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
        printf("Hello, world!\n");
        sleep_ms(1000);

        // starting at middle c
        for (uint8_t note = 60; note < 84; note += 3) {
            send_midi_note_on(note, 100);
            sleep_ms(900 - 10);
            send_midi_note_off(note, 0);
            sleep_ms(10);
            send_midi_note_on(note + 2, 100);
            sleep_ms(300 - 10);
            send_midi_note_off(note + 2, 0);
            sleep_ms(10);
        }
        send_midi_note_on(84, 100);
        sleep_ms(1200);
        send_midi_note_off(84, 0);

        // for (int i = 0; i < 8; i++) {
        //         send_midi_note_on(67 + i, 100);
        //         sleep_ms(500);
        //         send_midi_note_off(67 + i, 0);
        //         sleep_ms(500);
        // }

    }
}
