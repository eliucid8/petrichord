#pragma once

#include <vector>
#include "pico/types.h"

namespace fake_strum_irq {

// User callback signature: receives current logic level for pins {18,19,20,21}
using Callback = void (*)(std::vector<bool> states);

// RAII manager for GPIO IRQs on GP18–GP21.
class FakeStrumIrq {
public:
	// Construct and initialize IRQs. Optionally provide a callback.
	explicit FakeStrumIrq(Callback cb = nullptr);

	// Tear down IRQs on destruction.
	~FakeStrumIrq();

	// Update the user callback at runtime (IRQ-safe).
	void set_callback(Callback cb);

	// Non-copyable/non-movable to avoid multiple active instances.
	FakeStrumIrq(const FakeStrumIrq&) = delete;
	FakeStrumIrq& operator=(const FakeStrumIrq&) = delete;
	FakeStrumIrq(FakeStrumIrq&&) = delete;
	FakeStrumIrq& operator=(FakeStrumIrq&&) = delete;

private:
	// Active instance used by the C-style ISR to forward events.
	static FakeStrumIrq* s_active;

	// Pico SDK ISR hook (C-style), forwards to instance.
	static void gpio_isr(uint gpio, uint32_t events);

	// Instance handler invoked by gpio_isr.
	void on_gpio(uint gpio, uint32_t events);

	// Internal init/deinit helpers.
	void init_internal();
	void deinit_internal();

	// State
	volatile Callback callback_ = nullptr;
	bool initialized_ = false;
};

}  // namespace fake_strum_irq
