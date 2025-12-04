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
#include "FreeRTOS.h"
#include "task.h"
#include <map>

// #include "blink.pio.h"

static ChordController* g_chord_controller = nullptr;

#define LED_PIN 1

// I2C Constants
#define I2C_CHANNEL_ONE i2c1
#define IMU_SDA 26
#define IMU_SCL 27

#define PRINT_AUDIO false
#define PRINT_IMU
#define PRINT_KEYS true

#define CHORD_MATRIX_ROWS 4
#define CHORD_MATRIX_COLS 7

void blink_task(void *pvParameters) {
    int ledState = 1;

    while(1) {
        printf("Blink!\n");
        ledState = ledState ^ 1;
        gpio_put(LED_PIN, ledState);
        vTaskDelay(500);
    }
}



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

uint8_t style_plate_state = 0;
void handle_strum_plate_irq(uint gpio, uint32_t events) {
    uint8_t pin_bit = 0;
    switch(gpio) {
        case 16: pin_bit = 1 << 0; break;
        case 17: pin_bit = 1 << 1; break;
        case 19: pin_bit = 1 << 2; break;
        case 20: pin_bit = 1 << 3; break;
        case 26: pin_bit = 1 << 4; break;
        case 27: pin_bit = 1 << 5; break;
        case 28: pin_bit = 1 << 6; break;
        default: 
            printf("WARNING: Unrecognized interrupt pin detected\n");
            return; // unrecognized pin
    }

    if(events & GPIO_IRQ_EDGE_RISE) {
        style_plate_state |= pin_bit;
    } else if(events & GPIO_IRQ_EDGE_FALL) {
        style_plate_state &= ~pin_bit;
    } else {
        printf("WARNING: Unrecognized interrupt event detected\n");
        return; // unrecognized event
    }
}

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
    MidiMessenger midi_messenger(uart0, PRINT_KEYS);
    ChordController chord_controller(&midi_messenger);
    KeyMatrixController key_matrix_controller(CHORD_MATRIX_ROWS, CHORD_MATRIX_COLS, 1, 30);


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

    
    const uint8_t row_pins[4] = {10, 11, 12, 13, };
    const uint8_t col_pins[7] = {2, 3, 4, 5, 6, 7, 8, };
    key_matrix_controller.init(row_pins, col_pins);
    
    std::vector<std::vector<bool>> pressed(CHORD_MATRIX_ROWS, std::vector<bool>(CHORD_MATRIX_COLS, false));
    std::vector<std::vector<bool>> released(CHORD_MATRIX_ROWS, std::vector<bool>(CHORD_MATRIX_COLS, false));

    MicPitchDetector pitch;
    pitch.init(); 
    const float min_sound = 23000.0f; //smallest note

    int loop_counter = 0;
    int demo_imu_state = false;

    // FREE RTOS BLINKING TEST
    // xTaskCreate(blink_task, "Blink_LED", 256, NULL, 1, NULL);
    // vTaskStartScheduler( );

    // Set up interrupts for strum plate
    gpio_set_irq_enabled_with_callback(16, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &handle_strum_plate_irq);
    gpio_set_irq_enabled(17, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(19, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(20, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(26, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(27, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(28, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    uint8_t imu_velocity = 127;

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
        bool chords_changed = key_matrix_controller.poll_matrix_rising();
        if(chords_changed) {
            auto new_key_state = key_matrix_controller.get_key_state();
            g_chord_controller->update_key_state(new_key_state);
        }

        auto key_selected = STYLE_PLATE_MAP.find(style_plate_state);
        if(key_selected != STYLE_PLATE_MAP.end()) {
            // valid style plate state detected
            uint8_t style_index = key_selected->second;
            
            // Get key for specific chord chosen
            g_chord_controller->handle_strum(style_index);
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

            // Test code: corresponds to imu being rotated 90 degrees on one axis
            // if(g.z < 2 && g.z > -2) {
            //     // gpio_put(LED_PIN, 1);
            //     if (!demo_imu_state) {
            //         midi_messenger.send_midi_note_on(Note(36, 120));
            //         demo_imu_state = true;
            //     }
            // } else {
            //     // gpio_put(LED_PIN, 0);
            //     if (demo_imu_state) {
            //         midi_messenger.send_midi_note_off(Note(36, 0));
            //         demo_imu_state = false;
            //     }
            // }

            float velocity_float = (std::abs(g.x) / 9.81f) * 127.0f;
            // convert to uint8_t safely
            if (velocity_float > 127.0f) {
                imu_velocity = 127;
            } else if (velocity_float < 0.0f) {
                imu_velocity = 0;
            } else {
                imu_velocity = static_cast<uint8_t>(velocity_float);
            }
            

            // if(PRINT_IMU) {
            //     imu_controller.debugPrint();
            // }

            #ifdef PRINT_IMU
            printf("IMU Velocity: %d\n", imu_velocity);
            #endif
        }
        
        // superloop baby!
        sleep_ms(10 /*6*/);
    }
}
