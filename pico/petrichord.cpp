#include "pinout_config.h"

#define PRINT_KEYS false
#define PRINT_AUDIO false
#define PLOT_AUDIO false
#define PRINT_IMU false

static ChordController* g_chord_controller = nullptr;

void binprintf(uint8_t v)
{
    unsigned int mask=1<<((sizeof(uint8_t)<<3)-1);
    while(mask) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
    }
}

uint8_t style_plate_state = 0;
uint8_t imu_velocity = 127;
void handle_strum_plate_irq(uint gpio, uint32_t events) {
    // printf("Strum plate IRQ on pin %d, event %d\n", gpio, events == GPIO_IRQ_EDGE_RISE ? 1 : 0);

    // uint8_t pin_bit = 0;
    // switch(gpio) {
    //     case 20: pin_bit = 1 << 0; break;
    //     case 21: pin_bit = 1 << 1; break;
    //     default: 
    //         printf("WARNING: Unrecognized interrupt pin detected\n");
    //         return; // unrecognized pin
    // }

    // if(events & GPIO_IRQ_EDGE_RISE) {
    //     style_plate_state |= pin_bit;
    // } else if(events & GPIO_IRQ_EDGE_FALL) {
    //     style_plate_state &= ~pin_bit;
    // } else {
    //     printf("WARNING: Unrecognized interrupt event detected\n");
    //     return; // unrecognized event
    // }

    for(uint8_t i = 0; i < STRUM_PLATE_COUNT; i++) {
        if(gpio_get(STRUM_PLATE_PINS[i])) {
            style_plate_state |= (1 << i);
        } else {
            style_plate_state &= ~(1 << i);
        }
    }

    auto key_selected = STYLE_PLATE_MAP.find(style_plate_state);
    if(key_selected != STYLE_PLATE_MAP.end()) {
        // valid style plate state detected
        uint8_t style_index = key_selected->second;
        g_chord_controller->update_note(style_index, imu_velocity);
    } else {
        g_chord_controller->update_note(255, 0);
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

    
    gpio_init(STRUM_PLATE_PINS[0]);
        gpio_set_dir(STRUM_PLATE_PINS[0], GPIO_IN);
        gpio_set_irq_enabled_with_callback(
            STRUM_PLATE_PINS[0],
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
            true,
            &handle_strum_plate_irq
    );

    for(int i = 1; i < STRUM_PLATE_COUNT; i++) {
        gpio_init(STRUM_PLATE_PINS[i]);
        gpio_set_dir(STRUM_PLATE_PINS[i], GPIO_IN);
        gpio_set_irq_enabled(
            STRUM_PLATE_PINS[i],
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
            true
        );   
    }
    
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

    key_matrix_controller.init(ROW_PINS, COL_PINS);
    
    std::vector<std::vector<bool>> pressed(CHORD_MATRIX_ROWS, std::vector<bool>(CHORD_MATRIX_COLS, false));
    std::vector<std::vector<bool>> released(CHORD_MATRIX_ROWS, std::vector<bool>(CHORD_MATRIX_COLS, false));

    MicPitchDetector pitch;
    pitch.init(); 
    const float min_sound = 23000.0f; //smallest note

    int loop_counter = 0;

    while(true) {
        loop_counter++;
        // =========
        // mic stuff
        // =========
        auto pr = pitch.update();  //updates mic info
        if(PLOT_AUDIO) {
            pitch.bins_for_plotter();   //prints amplitude frequencies 
        }

        if(pr.amplitude >= min_sound){
            if(PRINT_AUDIO) {
                printf("Pitch frequency: ~%.1f Hz  bin=%s  amp=%.3f\n",
                pr.freq_hz, pr.name, pr.amplitude);
            } 
            g_chord_controller->update_voice_pitch(pr.midi);
        } else {
            if(PRINT_AUDIO) {
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
        

        // =========
        // imu stuff
        // =========
        // update every 10 cycles
        if(imu_controller.initialized() && loop_counter == 10) {
            loop_counter = 0;
            // Read vector
            struct imu_xyz_data g;
            imu_controller.readGravityVector(&g);

            float velocity_float = (std::abs(g.z) / 9.81f) * 127.0f;
            // convert to uint8_t safely
            if (velocity_float > 127.0f) {
                imu_velocity = 127;
            } else if (velocity_float < 0.0f) {
                imu_velocity = 0;
            } else {
                imu_velocity = static_cast<uint8_t>(velocity_float);
            }

            if(PRINT_IMU) {
                printf("IMU Velocity: %d\n", imu_velocity);
            }
        }
        
        // superloop baby!
        sleep_ms(10 /*6*/);
    }
}
