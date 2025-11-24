#include "controllers/key_matrix_controller.h"

KeyMatrixController::KeyMatrixController(
    size_t num_rows,
    size_t num_cols,
    int debounce_threshold,
    int poll_delay
) : NUM_ROWS(num_rows), NUM_COLS(num_cols), DEBOUNCE_THRESHOLD(debounce_threshold), POLL_DELAY(poll_delay),
_row_pins(num_rows, 0), _col_pins(num_cols, 0), _last(num_rows, std::vector<bool>(num_cols, false)) {
    // _row_pins.reserve(num_rows);
    // _col_pins.reserve(num_cols);
}

bool KeyMatrixController::init(const uint8_t row_pins[], const uint8_t col_pins[]) {
    // copy pins to arrays
    for(int r = 0; r < NUM_ROWS; ++r) {
        _row_pins[r] = row_pins[r];
        gpio_init(row_pins[r]);
        gpio_set_dir(row_pins[r], GPIO_OUT);
        gpio_put(row_pins[r], 1); // idle high (not selected)
        sleep_ms(50);
    }

    for(int c = 0; c < NUM_COLS; ++c) {
        _col_pins[c] = col_pins[c];

        gpio_init(col_pins[c]);
        gpio_set_dir(col_pins[c], GPIO_IN);
        gpio_pull_up(col_pins[c]); // pressed -> reads low when row is driven low
        sleep_ms(50);
    }

    // this shit was segfaulting fuck me
    for(int r = 0; r < NUM_ROWS; ++r) {
        for(int c = 0; c < NUM_COLS; ++c) {
            _last[r][c] = false;
        }
    }

    printf("key matrix controller inited\n");
    return true;
}

bool KeyMatrixController::get_matrix_edges(std::vector<std::vector<bool>>& released, std::vector<std::vector<bool>>& pressed) {
    bool state[NUM_ROWS][NUM_COLS] = {0};
    for(int r = 0; r < NUM_ROWS; ++r) {
        gpio_put(_row_pins[r], 0);
        
        sleep_us(POLL_DELAY);
        for(int c = 0; c < NUM_COLS; c++) {
            state[r][c] = gpio_get(_col_pins[c]) == 0;
            
            released[r][c] = (_last[r][c] && !state[r][c]);
            pressed[r][c] = (!_last[r][c] && state[r][c]);
            
            _last[r][c] = state[r][c];
        }
        gpio_put(_row_pins[r], 1);
    }
    return true;
}

bool KeyMatrixController::poll_matrix_once() {
    bool changed = false;
    for(int r = 0; r < NUM_ROWS; ++r) {
        gpio_put(_row_pins[r], 0);
        
        sleep_us(POLL_DELAY);
        for(int c = 0; c < NUM_COLS; c++) {
            bool temp = gpio_get(_col_pins[c]) == 0;
            if (temp != _last[r][c]) {
                changed = true;
            }
            _last[r][c] = temp;
        }
        gpio_put(_row_pins[r], 1);
    }
    return changed;
}


std::vector<std::vector<bool>> KeyMatrixController::get_key_state() {
    std::vector<std::vector<bool>> ret;
    std::copy(_last.begin(), _last.end(), back_inserter(ret));
    return ret;
}