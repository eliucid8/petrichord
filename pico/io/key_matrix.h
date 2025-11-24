#ifndef KEY_MATRIX_H
#define KEY_MATRIX_H

#include <cstdint>

// TODO: make oop-based interface

constexpr int MATRIX_ROWS = 4; // Total number of rows in key matrix
constexpr int MATRIX_COLS = 2;
constexpr int CHORD_ROWS = 3; // The number of rows dedicated to chord types

void configure_matrix_pins(const uint8_t row_pins[MATRIX_ROWS], const uint8_t col_pins[MATRIX_COLS]);


void poll_matrix_once(const uint8_t row_pins[MATRIX_ROWS], const uint8_t col_pins[MATRIX_COLS], bool state[MATRIX_ROWS][MATRIX_COLS]);

#endif // KEY_MATRIX_H
