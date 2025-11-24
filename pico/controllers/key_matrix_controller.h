#pragma once

#include <vector>
#include <stdio.h>

#include "pico/stdlib.h"

class KeyMatrixController {
    public:    
        KeyMatrixController(size_t num_rows, size_t num_cols, int debounce_threshold = 1, int poll_delay=30);
        bool init(const uint8_t row_pins[], const uint8_t col_pins[]);
        bool poll_matrix(std::vector<std::vector<bool>>& released, std::vector<std::vector<bool>>& pressed);
        
        const size_t NUM_ROWS;
        const size_t NUM_COLS;
        const int DEBOUNCE_THRESHOLD;
        const int POLL_DELAY;

    private:
        // OPTIMIZE: use templating and set this size at compile time?
        std::vector<uint8_t> _row_pins;
        std::vector<uint8_t> _col_pins;
        std::vector<std::vector<bool>> _last;
};