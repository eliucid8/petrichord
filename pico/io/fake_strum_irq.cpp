#include "strum_irq.h"

#include <stdio.h>
#include <cstddef>
#include <vector>
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

namespace {

// Pins monitored (GP18–GP21)
constexpr uint kPins[] = {18, 19, 20, 21};

void set_irqs_enabled(bool enable) {
	constexpr uint32_t mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
	for (uint pin : kPins) {
		gpio_set_irq_enabled(pin, mask, enable);
	}
}

}  // namespace

namespace fake_strum_irq {

class FakeStrumIrq : public strum_irq::StrumIrq {
public:
		static FakeStrumIrq* s_active;
		// explicit FakeStrumIrq(strum_irq::Callback cb = nullptr);

		FakeStrumIrq(strum_irq::Callback cb) {
			// Set callback first (IRQ-safe publish), then init hardware.
			set_callback(cb);
			init_internal();
		}

		~FakeStrumIrq() {
			deinit_internal();
		}

private:
	void init_internal() {
		if (initialized_) {
			deinit_internal();
		}

		// Disable interrupts while installing handler and enabling GPIO IRQs.
		uint32_t save = save_and_disable_interrupts();

		// Assign active instance before enabling IRQs.
		s_active = this;

		// Configure pins as inputs with internal pull-ups.
		for (uint pin : kPins) {
			gpio_init(pin);
			gpio_set_dir(pin, GPIO_IN);
			gpio_pull_up(pin);
		}

		// Install shared IRQ callback and enable both edges on all pins.
		constexpr uint32_t mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
		gpio_set_irq_enabled_with_callback(kPins[0], mask, true, gpio_isr);
		for (size_t i = 1; i < sizeof(kPins) / sizeof(kPins[0]); ++i) {
			gpio_set_irq_enabled(kPins[i], mask, true);
		}

		restore_interrupts(save);
		initialized_ = true;
		
		states = {false, false, false, false};
		for(int i = 0; i < states.size(); i++) {
			state_last_changed.push_back(from_us_since_boot(0));
		}
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
		
	// Static ISR: forwards to active instance if present.
	static void gpio_isr(uint gpio, uint32_t events) {
		FakeStrumIrq* self = s_active;
		if (!self) return;
		self->on_gpio(gpio, events);
	}

	void on_gpio(uint /*gpio*/, uint32_t /*events*/) {
		// Build snapshot of pin states and invoke user callback.
		auto cb = callback_;
		if (!cb) return;

		int states_index = 0;
		bool state_changed = false;
		absolute_time_t current_time = get_absolute_time(); 
		const int64_t DEBOUNCE_TIME = 1000; // minimum amt of time between changes, in us
		for (uint pin : kPins) {
			bool new_state = !gpio_get(pin);
			int64_t delta = absolute_time_diff_us(state_last_changed[states_index], current_time);
			state_last_changed[states_index] = current_time;
			if(states[states_index] != new_state && delta > DEBOUNCE_TIME) {
				// printf("%ld ", delta);
				state_changed = true;
				states[states_index] = new_state;
			}
			++states_index;
		}
		if(state_changed) {
			cb(states);
		}
		// printf("\n");
	}

	void set_callback(strum_irq::Callback cb) {
		uint32_t save = save_and_disable_interrupts();
		callback_ = cb;
		restore_interrupts(save);
	}
		
	volatile strum_irq::Callback callback_ = nullptr;
	bool initialized_ = false;
	std::vector<bool> states;
	std::vector<absolute_time_t> state_last_changed;
};

// Because callbacks use a static method, we need a singleton pattern here.
// Static active instance
FakeStrumIrq* FakeStrumIrq::s_active = nullptr;

}  // namespace fake_strum_irq


std::unique_ptr<strum_irq::StrumIrq> strum_irq::CreateFakeStrumIrq(strum_irq::Callback cb) {
	return std::make_unique<fake_strum_irq::FakeStrumIrq>(cb);
}
