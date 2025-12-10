#include "pinout_config.h"

static ChordController* g_chord_controller = nullptr;

void binprintf(uint8_t v)
{
    unsigned int mask=1<<((sizeof(uint8_t)<<3)-1);
    while(mask) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
    }
}

bool i2c_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *out, size_t len) {
    int ret = i2c_write_blocking(i2c1, dev_addr, &reg, 1, true); // send reg, keep master
    if (ret < 0) return false;
    ret = i2c_read_blocking(i2c1, dev_addr, out, len, false);
    return (ret >= 0);
}


uint16_t style_plate_state = 0;
uint8_t imu_velocity = 127;
void handle_strum_plate_irq(uint gpio, uint32_t events) {
    //printf("Strum plate IRQ on GPIO %d, events: 0x%08X\n", gpio, events); 
    style_plate_state = 0;
    if(gpio == IO_INTN) {

        uint8_t port0 = 0;
        uint8_t port1 = 0;
        if (i2c_read_reg(AW9523_ADDR, GET_PORT0, &port0, 1) &&
            i2c_read_reg(AW9523_ADDR, GET_PORT1, &port1, 1)) {
            // printf("INT: AW9523 GPIO States - Port0: 0x%02X, Port1: 0x%02X\n", port0, port1);
        } else {
            printf("INT: Failed to read AW9523 GPIO states\n");
        }

        uint16_t aw_gpio_state = (port0 << 8) | port1;
        style_plate_state = aw_gpio_state;
        ///printf("INT: Style plate state from AW9523: 0x%04X\n", style_plate_state);

    } else {
        for(uint8_t i = 0; i < STRUM_PLATE_COUNT; i++) {
            if(!gpio_get(STRUM_PLATE_PINS[i])) {
                style_plate_state = (1 << i);
            } 
        }
    }

    auto key_selected = STYLE_PLATE_MAP.find(style_plate_state);
    if(key_selected != STYLE_PLATE_MAP.end()) {
        // valid style plate state detected
        uint8_t style_index = key_selected->second;
        g_chord_controller->update_note(style_index - 1, imu_velocity);
    } else {
        g_chord_controller->update_note(255, 0);
    }

    // gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
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
    gpio_init(IO_SCL);
    gpio_init(IO_SDA);
    gpio_set_function(IO_SDA, GPIO_FUNC_I2C);
    gpio_set_function(IO_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(IO_SDA);
    gpio_pull_up(IO_SCL);
    
}

void setup_interrupts() {
    gpio_init(STRUM_PLATE_PINS[0]);
    gpio_set_dir(STRUM_PLATE_PINS[0], GPIO_IN);
    gpio_pull_up(STRUM_PLATE_PINS[0]);
    gpio_set_irq_enabled_with_callback(
        STRUM_PLATE_PINS[0],
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &handle_strum_plate_irq
    );

    for(int i = 1; i < STRUM_PLATE_COUNT; i++) {
        gpio_init(STRUM_PLATE_PINS[i]);
        gpio_set_dir(STRUM_PLATE_PINS[i], GPIO_IN);
        gpio_pull_up(STRUM_PLATE_PINS[i]);
        gpio_set_irq_enabled(
            STRUM_PLATE_PINS[i],
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
            true
        );   
    }

    gpio_init(IO_INTN);
    gpio_set_dir(IO_INTN, GPIO_IN);
    gpio_pull_up(IO_INTN);

    gpio_set_irq_enabled(
        IO_INTN,
        GPIO_IRQ_EDGE_FALL,
        true
    );
}

bool i2c_write_reg(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[1 + len];
    buf[0] = reg;
    for (size_t i = 0; i < len; ++i) buf[1 + i] = data[i];
    int ret = i2c_write_blocking(i2c1, dev_addr, buf, 1 + len, false);
    return (ret >= 0);
}
void aw9523_init_device() {
    uint8_t chip_id = 0;   
    if (i2c_read_reg(AW9523_ADDR, GET_REG_CHIPID, &chip_id, 1)) {
        printf("AW9523 Chip ID: 0x%02X\n", chip_id);
    } else {
        printf("Failed to read AW9523 Chip ID\n");
    }

    printf("Initializing AW9523 IO Expander...\n");

    // Configure all pins as inputs
    uint8_t config_data[2] = {0xFF, 0xFF}; // Set all pins as inputs
    if(i2c_write_reg(AW9523_ADDR, CONFIG_PORT0, config_data, 1) &&
       i2c_write_reg(AW9523_ADDR, CONFIG_PORT1, config_data + 1, 1)) {
        printf("Configured all AW9523 pins as inputs\n");
    } else {
        printf("Failed to configure AW9523 pins\n");
    }

    printf("AW9523 initialization complete\n");
}

uint8_t gpio_states[2] = {0, 0};

// =====================================
// Core 1: mic / FFT + Pitch Processing
// =====================================
void core1_entry() {

    MicPitchDetector pitch;
    pitch.init();   
    printf("mic initialized\n");

    while (true) {
        auto pr = pitch.update();

        if (PLOT_AUDIO) {
            pitch.bins_for_plotter();
        }

        // result -> shared variables
        mutex_enter_blocking(&pitch_mutex);
        latest_pitch = pr;
        pitch_ready = true;
        mutex_exit(&pitch_mutex);

        sleep_ms(10);
    }
}

// =======================
// Core 0: Everything else
// =======================
int main()
{
    init_io();

    // Init shared pitch mutex
    mutex_init(&pitch_mutex);

    // Launch mic/FFT processing on core 1
    multicore_launch_core1(core1_entry);

    // OPTIMIZE: make a petrichord object with instance variables so we can init everything in separate functions cleanly
    MidiMessenger midi_messenger(uart0, PRINT_KEYS);
    ChordController chord_controller(&midi_messenger);
    KeyMatrixController key_matrix_controller(CHORD_MATRIX_ROWS, CHORD_MATRIX_COLS, 1, 30);

    // sleep_ms(5000);
    aw9523_init_device();
    printf("IO Extender initialized\n");

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

    setup_interrupts();

    int loop_counter = 0;

    while(true) {
        loop_counter++;

        // =========
        // mic stuff 
        // =========

        PitchResult pr; 
        bool have_pitch = false; 
        
        mutex_enter_blocking(&pitch_mutex);
        if (pitch_ready) {
            pr = latest_pitch;
            have_pitch = true;
            pitch_ready = false; 
        }
        mutex_exit(&pitch_mutex); 
        
        if(have_pitch){
            if (pr.freq_hz != 0.0f){
                if(PRINT_AUDIO) {
                    printf("Pitch frequency: ~%.1f Hz  bin=%s Midi=%d  amp=%.3f\n",
                    pr.freq_hz, pr.name, pr.midi, pr.amplitude);
                } 
                g_chord_controller->update_voice_pitch(pr.midi);
            } 
            else {
                if(PRINT_AUDIO) {
                printf("no pitch / too quiet\n");
                }
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

            float velocity_float = (std::abs(g.y) / 9.81f) * 127.0f;
            // convert to uint8_t safely
            if (velocity_float > 127.0f) {
                imu_velocity = 127;
            } else if (velocity_float < 0.0f) {
                imu_velocity = 0;
            } else {
                imu_velocity = static_cast<uint8_t>(velocity_float);
            }

            if(PRINT_IMU) {
                // imu_controller.debugPrint();
                printf("Computed velocity: %d\n", imu_velocity);
            }

            if (i2c_read_reg(AW9523_ADDR, GET_PORT0, &gpio_states[0], 1) &&
                i2c_read_reg(AW9523_ADDR, GET_PORT1, &gpio_states[1], 1)) {
                // printf("AW9523 GPIO States - Port0: 0x%02X, Port1: 0x%02X\n", gpio_states[0], gpio_states[1]);
            } else {
                printf("Failed to read AW9523 GPIO states\n");
            }
        }

        if(PRINT_IO_EXTENDER) {
            if (i2c_read_reg(AW9523_ADDR, GET_PORT0, &gpio_states[0], 1) &&
                i2c_read_reg(AW9523_ADDR, GET_PORT1, &gpio_states[1], 1)) {
                printf("AW9523 GPIO States - Port0: 0x%02X, Port1: 0x%02X\n", gpio_states[0], gpio_states[1]);
            } else {
                printf("Failed to read AW9523 GPIO states\n");
            }
        }
        
        // superloop baby!
        sleep_ms(10 /*6*/);
    }
}
