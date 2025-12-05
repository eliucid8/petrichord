#include <memory>
#include <stdio.h>
#include <cstdint>
#include <algorithm>
#include <map>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "theory/note.h"

// #include "io/key_matrix.h"
#include "io/strum_irq.h"
#include "controllers/chord_controller.h"
#include "io/midi_messenger.h"
#include "io/mic_input.h"
extern "C" {
#include "pico/fft.h"
}

#include "controllers/imu_controller.h"
#include "controllers/key_matrix_controller.h"
// #include "FreeRTOS.h"
// #include "task.h"
#include <map>

#define LED_PIN 1

// I2C Constants
#define I2C_CHANNEL_ONE i2c1
#define IMU_SDA 18
#define IMU_SCL 19

#define PRINT_AUDIO false
#define PRINT_IMU false
#define PRINT_KEYS false

#define CHORD_MATRIX_ROWS 4
#define CHORD_MATRIX_COLS 7

const uint8_t ROW_PINS[4] = {10, 11, 12, 13, };
const uint8_t COL_PINS[7] = {2, 3, 4, 5, 6, 7, 8, };

#define STRUM_PLATE_COUNT 2
const uint8_t STRUM_PLATE_PINS[STRUM_PLATE_COUNT] = {20, 21,};

// MUST MATCH PHYSICAL CHARLIEPLEXING STYLE PLATE WIRING
const std::map<uint8_t, uint8_t> STYLE_PLATE_MAP = {
    {0b0000011, 0},
    {0b0000101, 1},
    {0b0001001, 2},
    {0b0010001, 3},
    {0b0100001, 4},
    {0b1000001, 5},
    {0b0000110, 6},
    {0b0001010, 7},
    {0b0010010, 8},
    {0b0100010, 9},
    {0b1000010, 10},
    {0b0001100, 11},
    {0b0010100, 12},
    {0b0100100, 13},
    {0b1000100, 14},
    {0b0011000, 15},
    {0b0101000, 16},
    {0b1001000, 17},
    {0b0110000, 18},
    {0b1010000, 19},
    {0b1100000, 20}
};