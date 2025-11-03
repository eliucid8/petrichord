#include "fake_strum_irq.h"

#include <cstddef>
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

// Static active instance
FakeStrumIrq* FakeStrumIrq::s_active = nullptr;

// Static ISR: forwards to active instance if present.
void FakeStrumIrq::gpio_isr(uint gpio, uint32_t events) {
	FakeStrumIrq* self = s_active;
	if (!self) return;
	self->on_gpio(gpio, events);
}

void FakeStrumIrq::on_gpio(uint /*gpio*/, uint32_t /*events*/) {
	// Build snapshot of pin states and invoke user callback.
	auto cb = callback_;
	if (!cb) return;

	std::vector<bool> states;
	states.reserve(sizeof(kPins) / sizeof(kPins[0]));
	for (uint pin : kPins) {
		states.push_back(!gpio_get(pin));
	}
	cb(states);
}

FakeStrumIrq::FakeStrumIrq(Callback cb) {
	// Set callback first (IRQ-safe publish), then init hardware.
	set_callback(cb);
	init_internal();
}

FakeStrumIrq::~FakeStrumIrq() {
	deinit_internal();
}

void FakeStrumIrq::set_callback(Callback cb) {
	uint32_t save = save_and_disable_interrupts();
	callback_ = cb;
	restore_interrupts(save);
}

void FakeStrumIrq::init_internal() {
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
}

void FakeStrumIrq::deinit_internal() {
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

}  // namespace fake_strum_irq
