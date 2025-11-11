#include <memory>
#include <stdio.h>
#include <cstdint>
#include <algorithm>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"

#include "io/key_matrix.h"
#include "io/strum_irq.h"
#include "controllers/chord_controller.h"
#include "io/midi_messenger.h"
#include "io/mic_input.h"
extern "C" {
#include "pico/fft.h"
}

#include "controllers/imu_controller.h"

// #include "blink.pio.h"

static ChordController* g_chord_controller = nullptr;

#define LED_PIN 15

// I2C Constants
#define I2C_CHANNEL_ONE i2c1
#define IMU_SDA 26
#define IMU_SCL 27

#define PRINT_AUDIO true
#define PRINT_IMU false
#define PRINT_KEYS true

void init_io() {
    // init serial printing
    stdio_init_all();

    // init debug led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    // init default led
    
    // init midi
    uart_init(uart0, 31250);
    gpio_set_function(0, GPIO_FUNC_UART); // TX on GP0

    // trying to delay before initialization.
    sleep_ms(1000);
    // I2C Initialization
    i2c_init(I2C_CHANNEL_ONE, 400 * 1000);  // 400 kHz i2c1 channel
    gpio_init(IMU_SCL);
    gpio_init(IMU_SDA);
    gpio_set_function(IMU_SDA, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA);
    gpio_pull_up(IMU_SCL);
    
}


int main()
{
    init_io();


    // OPTIMIZE: make a petrichord object with instance variables so we can init everything in separate functions cleanly
    // init microphone
    MicADC mic(/*adc_num=*/2, /*vref_volts=*/3.3f);
    mic.init();

    MidiMessenger midi_messenger(uart0);
    ChordController chord_controller(&midi_messenger);


    // IMU Controller Initialization
    IMU_Controller imu_controller;
    bool imu_status = imu_controller.init(I2C_CHANNEL_ONE);

    if(!imu_status) {
        printf("ERROR: imu failed to initialize on i2c1");
    }


    g_chord_controller = &chord_controller;

    std::unique_ptr<strum_irq::StrumIrq> fake_strum = strum_irq::CreateFakeStrumIrq(nullptr);
    fake_strum->set_callback([](std::vector<bool> states) {
        if (g_chord_controller) {
            g_chord_controller->handle_strum(states);
        }
    });


    // matrix scan
    const uint8_t row_pins[MATRIX_ROWS] = {2, 3, 4, 5};
    const uint8_t col_pins[MATRIX_COLS] = {8, 9};
    configure_matrix_pins(row_pins, col_pins);

    bool keys[MATRIX_ROWS][MATRIX_COLS] = {0};
    bool last[MATRIX_ROWS][MATRIX_COLS] = {0};
    uint8_t notes[MATRIX_ROWS][MATRIX_COLS] = {
        {60, 62},
        {64, 65},
        {67, 69},
        {71, 72},
    };

    MicPitchDetector pitch;
    pitch.init(); 
    const float min_sound = 23000.0f; //smallest note

    

    int loop_counter = 0;
    while(true) {
        auto pr = pitch.update();  //updates mic info

        if(PRINT_AUDIO) {
            if(pr.amplitude >= min_sound){
                printf("Pitch frequency: ~%.1f Hz  bin=%s  amp=%.3f\n",
                pr.freq_hz, pr.name, pr.amplitude);
            } 
            else{
                printf(" no pitch / too quiet\n");
            }
        }
        
        // matrix stuff
        poll_matrix_once(row_pins, col_pins, keys);

        bool any = false;
        for (int r = 0; r < MATRIX_ROWS; ++r) {
            for (int c = 0; c < MATRIX_COLS; ++c) {
                if (keys[r][c] && !last[r][c]) {
                    if(PRINT_KEYS) {
                        printf("Key pressed: row %d col %d\n", r, c);
                    }
                    // send_midi_note_on(notes[r][c], 100);
                    g_chord_controller->update_chord(generate_chord(notes[r][c], MAJOR_INTERVALS, 4));
                }
                if (!keys[r][c] && last[r][c]) {
                    if(PRINT_KEYS) {
                        printf("Key released: row %d col %d\n", r, c);
                    }
                    // send_midi_note_off(notes[r][c], 0);
                }
            }
        }
        // OPTIMIZE: we just need to deep copy the contents of keys into last, not swap them.
        std::swap(keys, last);

        // update imu stuff every 100 ms
        if(loop_counter == 10) {
            loop_counter = 0;
            // Read vector
            struct imu_gravity_vector g;
            imu_controller.readGravityVector(&g);

            // Observationally, corresponds to imu being rotated 90 degrees on one axis
            if(g.z < 2 && g.z > -2) {
                gpio_put(LED_PIN, 1);
            } else {
                gpio_put(LED_PIN, 0);
            }
            if(PRINT_IMU) {
                imu_controller.debugPrint();
            }
        }
        loop_counter++;
        sleep_ms(10 /*6*/);
    }
}
