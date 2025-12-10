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

//2nd core lib
#include "pico/multicore.h"
#include "pico/mutex.h"

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
#include "controllers/io_extender.h"
// #include "FreeRTOS.h"
// #include "task.h"
#include <map>

#define LED_PIN 1

// I2C Constants
#define I2C_CHANNEL_ONE i2c1

#define IO_SDA 14
#define IO_SCL 15
#define IO_INTN 9

#define PRINT_AUDIO false
#define PRINT_IMU false
#define PRINT_MIDI false
#define PRINT_KEYS true
#define PLOT_AUDIO false
#define PRINT_IO_EXTENDER false

// #define DEBUG_STRUM_IRQ

#define CHORD_MATRIX_ROWS 4
#define CHORD_MATRIX_COLS 7

const uint8_t ROW_PINS[4] = {10, 11, 12, 13, };
const uint8_t COL_PINS[7] = {2, 3, 4, 5, 6, 7, 8, };

#define STRUM_PLATE_COUNT 6
const uint8_t STRUM_PLATE_PINS[STRUM_PLATE_COUNT] = {16, 17, 18, 19, 20, 21};

// MUST MATCH PHYSICAL CHARLIEPLEXING STYLE PLATE WIRING
const std::map<uint16_t, uint8_t> STYLE_PLATE_MAP = {
    {0b000001, 6},
    {0b000010, 5},
    {0b000100, 4},
    {0b001000, 3},
    {0b010000, 2},
    {0b100000, 1},
    {0xFEFF, 11},
    {0xFDFF, 22},
    {0xFBFF, 21},
    {0xF7FF, 20},
    {0xFFFE, 7},
    {0x7FFF, 16},
    {0xBFFF, 17},
    {0xDFFF, 18},
    {0xEFFF, 19},
    {0xFF7F, 12},
    {0xFFBF, 13},
    {0xFFDF, 14},
    {0xFFEF, 15},
    {0xFFF7, 10},
    {0xFFFB, 9},
    {0xFFFD, 8}
};

// === shared pitch btw cores ===
static mutex_t pitch_mutex;
static PitchResult latest_pitch;
static bool pitch_ready = false;   // = true after 1st update
