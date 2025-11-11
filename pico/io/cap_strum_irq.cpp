// FIXME: not added to cmakelists yet
#include "strum_irq.h"

#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include <stdio.h>
#include <cstdint>

class CapStrumIrq {
public:
    // pointer to singleton instance
    static CapStrumIrq* s_active;

    CapStrumIrq(strum_irq::Callback cb, uint8_t change_pin): change_pin(change_pin) {
        set_callback(cb);
        init_internal();
    }

	// Tear down IRQs on destruction.
    ~CapStrumIrq() {
        deinit_internal();
    }

	// Update the user callback at runtime (IRQ-safe).
	virtual void set_callback(strum_irq::Callback cb);
    
    // OPTIMIZE: refactor API to merge both cap sensors into a single object
    const uint8_t NUM_PADS = 11;
    std::vector<bool> states;

private:
	// // Pico SDK ISR hook (C-style), forwards to instance.
	static void gpio_isr(uint gpio, uint32_t events) {
		CapStrumIrq* self = s_active;
		if (!self) return;
		self->on_gpio(gpio, events);
    }

	// Instance handler invoked by gpio_isr.
	void on_gpio(uint gpio, uint32_t events) {
        uint8_t read_buf[7] = {0};
        // read 7 bytes from spi0
        spi_read_blocking(spi0, 0, read_buf, 7);
        printf("spi counter: %d\n", read_buf[0]);

        states.clear();
        // read bytes 1 through 3
        for(int i = 1; i < 4; i++) {
            uint8_t cur_byte = read_buf[i];
            printf("%d: %x\n", i, cur_byte);
            for(int j = 0; j < 4; j++) {
                uint8_t mask = 0x03;
                states.push_back((bool)(cur_byte & mask));
                mask <<= 2;
            }
        }

        // debug print
        printf("summary: ");
        for(bool state : states) {
            if(state) {
                printf("1");
            } else {
                printf("0");
            }
        }
        printf("\n");
        
        // TODO: check for state changes
        callback_(states);
    }

    // NIT: un-hardcode spi0
	// Internal init/deinit helpers.
	void init_internal() {
		if (initialized_) {
            return;
			// deinit_internal(); // not sure why we'd want this to be involutive
		}

		// Disable interrupts while installing handler and enabling GPIO IRQs.
		uint32_t save = save_and_disable_interrupts();

		// Assign active instance before enabling IRQs.
		s_active = this;
        
        init_interrupts();

		restore_interrupts(save);
		initialized_ = true;

        //send init data to sensor.
        spi_init(spi0, 115200);
        // configure spi0 to send 8-bit packets and conform to the expected clock guidlines
        spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

        // set device mode
        uint8_t settings[] = {0x90, 0b11010001};
        uint8_t response[] = {0x00, 0x00};

        spi_write_read_blocking(spi0, reinterpret_cast<uint8_t*>(settings), response, 2);

        // set device communication settings
        settings[0] = 0x91;
        // guard key 0, disable guard mode, enable quick spi, !change data mode, disable CRC
        settings[1] = 0b00000100;
        spi_write_read_blocking(spi0, reinterpret_cast<uint8_t*>(settings), response, 2);
    }

	void deinit_internal() {
		if (!initialized_) {
			// Still clear active/callback safely in case of partial init.
			uint32_t save = save_and_disable_interrupts();
			if (s_active == this) s_active = nullptr;
			callback_ = nullptr;
			restore_interrupts(save);
			return;
		}

		uint32_t save = save_and_disable_interrupts();

		// Disable IRQs on pins.
		set_irqs_enabled(false);

		// Clear active instance and callback.
		if (s_active == this) s_active = nullptr;
		callback_ = nullptr;

		restore_interrupts(save);
		initialized_ = false;
	}
    
    // TODO: re-implement raii handler
    void init_interrupts() {
		uint32_t save = save_and_disable_interrupts();
		// Install shared IRQ callback and enable both edges on all pins.
		constexpr uint32_t mask = GPIO_IRQ_EDGE_FALL;
		gpio_set_irq_enabled_with_callback(change_pin, mask, true, on_gpio);
        // not sure what this next one does?
        gpio_set_irq_enabled(change_pin, mask, true);
		restore_interrupts(save);
    }

	void set_callback(strum_irq::Callback cb) {
		uint32_t save = save_and_disable_interrupts();
		callback_ = cb;
		restore_interrupts(save);
	}

	// State
	volatile strum_irq::Callback callback_ = nullptr;
	bool initialized_ = false;
    uint8_t change_pin;

};

CapStrumIrq* CapStrumIrq::s_active = nullptr;