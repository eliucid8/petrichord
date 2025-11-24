#pragma once

#include "pico/stdlib.h"

#define MATRIX_ROWS 4
#define MATRIX_COLS 2
#define DEBOUNCE_THRESHOLD 1
#define POLL_DELAY 30

class KeyMatrixController {
    public:
        KeyMatrixController();
        bool init(const uint8_t row_pins[MATRIX_ROWS], const uint8_t col_pins[MATRIX_COLS]);
        bool poll_matrix(bool released[MATRIX_ROWS][MATRIX_COLS], bool pressed[MATRIX_ROWS][MATRIX_COLS]);
        
    private:
        uint8_t row_pins_[MATRIX_ROWS];
        uint8_t col_pins_[MATRIX_COLS];
        bool last[MATRIX_ROWS][MATRIX_COLS];
};