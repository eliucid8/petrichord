#include <memory>
#include <stdio.h>
#include <cstdint>
#include <algorithm>

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

// #include "blink.pio.h"

static ChordController* g_chord_controller = nullptr;

#define LED_PIN 1

// I2C Constants
#define I2C_CHANNEL_ONE i2c1
#define IMU_SDA 26
#define IMU_SCL 27

#define PRINT_AUDIO false
#define PRINT_IMU false
#define PRINT_KEYS true

#define CHORD_MATRIX_ROWS 4
#define CHORD_MATRIX_COLS 2

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
    MidiMessenger midi_messenger(uart0);
    ChordController chord_controller(&midi_messenger);
    KeyMatrixController key_matrix_controller(4, 2, 1, 30);



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
    const uint8_t row_pins[4] = {2, 3, 4, 5};
    const uint8_t col_pins[2] = {8, 9};
    key_matrix_controller.init(row_pins, col_pins);
    
    std::vector<std::vector<bool>> pressed(CHORD_MATRIX_ROWS, std::vector<bool>(CHORD_MATRIX_COLS, false));
    std::vector<std::vector<bool>> released(CHORD_MATRIX_ROWS, std::vector<bool>(CHORD_MATRIX_COLS, false));

    MicPitchDetector pitch;
    pitch.init(); 
    const float min_sound = 23000.0f; //smallest note

    int loop_counter = 0;
    int demo_imu_state = false;

    while(true) {
        loop_counter++;
        // =========
        // mic stuff
        // =========
        auto pr = pitch.update();  //updates mic info

        if(PRINT_AUDIO) {
            if(pr.amplitude >= min_sound){
                printf("Pitch frequency: ~%.1f Hz  bin=%s  amp=%.3f\n",
                pr.freq_hz, pr.name, pr.amplitude);
            } 
            else{
                printf("no pitch / too quiet\n");
            }
        }
        
        // ======================
        // chord key matrix stuff
        // ======================
        bool chords_changed = key_matrix_controller.poll_matrix_once();
        if(chords_changed) {
            auto new_key_state = key_matrix_controller.get_key_state();
            if(PRINT_KEYS) {
                // verified.
                for(auto row : new_key_state) {
                    for(bool col : row) {
                        if(col) {
                            printf("1");
                        } else {
                            printf("0");
                        }
                    }
                    printf("\n");
                }
            }
            g_chord_controller->update_key_state(new_key_state);
        }
        

        // =========
        // imu stuff
        // =========
        // update every 10 cycles
        if(loop_counter == 10) {
            loop_counter = 0;
            // Read vector
            struct imu_xyz_data g;
            imu_controller.readGravityVector(&g);

            // Observationally, corresponds to imu being rotated 90 degrees on one axis
            if(g.z < 2 && g.z > -2) {
                gpio_put(LED_PIN, 1);
                if (!demo_imu_state) {
                    midi_messenger.send_midi_note_on(Note(36, 120));
                    demo_imu_state = true;
                }
            } else {
                gpio_put(LED_PIN, 0);
                if (demo_imu_state) {
                    midi_messenger.send_midi_note_off(Note(36, 0));
                    demo_imu_state = false;
                }
            }
            if(PRINT_IMU) {
                imu_controller.debugPrint();
            }
        }
        
        // superloop baby!
        sleep_ms(10 /*6*/);
    }
}
