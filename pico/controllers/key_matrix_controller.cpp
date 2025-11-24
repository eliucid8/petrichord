#include "key_matrix_controller.h"

KeyMatrixController::KeyMatrixController() {
    // empty constructor
}

bool KeyMatrixController::init(const uint8_t row_pins[MATRIX_ROWS], const uint8_t col_pins[MATRIX_COLS]) {
    // copy pins to arrays
    for(int r = 0; r < MATRIX_ROWS; ++r) {
        row_pins_[r] = row_pins[r];

        gpio_init(row_pins[r]);
        gpio_set_dir(row_pins[r], GPIO_OUT);
        gpio_put(row_pins[r], 1); // idle high (not selected)
    }

    for(int c = 0; c < MATRIX_COLS; ++c) {
        col_pins_[c] = col_pins[c];

        gpio_init(col_pins[c]);
        gpio_set_dir(col_pins[c], GPIO_OUT);
        gpio_pull_up(col_pins[c]); // pressed -> reads low when row is driven low
    }

    for(int r = 0; r < MATRIX_ROWS; ++r) {
        for(int c = 0; c < MATRIX_COLS; ++c) {
            last[r][c] = false;
        }
    }

    return true;
}

bool KeyMatrixController::poll_matrix(bool released[MATRIX_ROWS][MATRIX_COLS], bool pressed[MATRIX_ROWS][MATRIX_COLS]) {

    bool state[MATRIX_ROWS][MATRIX_COLS] = {0};

    for(int r = 0; r < MATRIX_ROWS; ++r) {
        gpio_put(row_pins_[r], 0);

        sleep_us(POLL_DELAY);

        for(int c = 0; c < MATRIX_COLS; c++) {
            state[r][c] = gpio_get(col_pins_[c]) == 0;

            released[r][c] = (last[r][c] && !state[r][c]);
            pressed[r][c] = (!last[r][c] && state[r][c]);

            last[r][c] = state[r][c];
        }

        gpio_put(row_pins_[r], 1);
    }

    return true;
}