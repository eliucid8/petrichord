#include <stdio.h>

#include "key_matrix.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

void configure_matrix_pins(const uint8_t row_pins[MATRIX_ROWS], const uint8_t col_pins[MATRIX_COLS]) {
    for (int i = 0; i < MATRIX_ROWS; ++i) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_OUT);
        gpio_put(row_pins[i], 1); // idle high (not selected)
    }
    for (int j = 0; j < MATRIX_COLS; ++j) {
        gpio_init(col_pins[j]);
        gpio_set_dir(col_pins[j], GPIO_IN);
        gpio_pull_up(col_pins[j]); // use pull-ups; pressed -> reads low when row is driven low
    }
}

// Debounced matrix poll. state[row][col] == true means pressed.
// TODO: make counters/debounce threshold an instance var
void poll_matrix_once(const uint8_t row_pins[MATRIX_ROWS], const uint8_t col_pins[MATRIX_COLS], bool state[MATRIX_ROWS][MATRIX_COLS]) {
    static uint8_t counters[MATRIX_ROWS][MATRIX_COLS] = {{0}};
    const uint8_t DEBOUNCE_THRESHOLD = 1; // adjust as needed

    // Ensure all rows idle high before starting
    for (int r = 0; r < MATRIX_ROWS; ++r) gpio_put(row_pins[r], 1);

    // Raw scan
    bool raw[MATRIX_ROWS][MATRIX_COLS] = {{false}};
    for (int r = 0; r < MATRIX_ROWS; ++r) {
        gpio_put(row_pins[r], 0);      // select this row by driving it low
        sleep_us(30);                 // allow signals to settle (adjust if needed)
        for (int c = 0; c < MATRIX_COLS; ++c) {
            // With pull-ups, a pressed key will pull the column low when row is low
            state[r][c] = (gpio_get(col_pins[c]) == 0);
        }
        gpio_put(row_pins[r], 1);     // deselect
    }

    // Update debounce counters and output state
    // for (int r = 0; r < MATRIX_ROWS; ++r) {
    //     for (int c = 0; c < MATRIX_COLS; ++c) {
    //         if (raw[r][c]) {
    //             if (counters[r][c] < DEBOUNCE_THRESHOLD) ++counters[r][c];
    //         } else {
    //             if (counters[r][c] > 0) --counters[r][c];
    //         }
    //         state[r][c] = (counters[r][c] >= DEBOUNCE_THRESHOLD);
    //     }
    // }
}

// // ...existing code...
// int fake_main()
// {
//     // Example matrix pins (update to match your wiring)
//     const uint8_t row_pins[4] = {2, 3, 4, 5};
//     const uint8_t col_pins[2] = {8, 9};

//     configure_matrix_pins(row_pins, col_pins);

//     bool keys[4][2] = {0};

//     while (true) {
//         // scan the matrix
//         poll_matrix_once(row_pins, col_pins, keys);

//         // simple debug output: list pressed keys
//         bool any = false;
//         for (int r = 0; r < 4; ++r) {
//             for (int c = 0; c < 2; ++c) {
//                 if (keys[r][c]) {
//                     printf("Key pressed: row %d col %d\n", r, c);
//                     any = true;
//                 }
//             }
//         }
//         if (!any) {
//             // optional heartbeat
//             printf("No keys\n");
//         }
//         sleep_ms(100);
        
//         // ...existing application code (MIDI, etc.) can run here ...
//     }
// }