#pragma once

#include <vector>
#include <memory>
#include "pico/types.h"

namespace strum_irq {

// User callback signature: receives current logic level for pins {18,19,20,21}
using Callback = void (*)(std::vector<bool> states);

// RAII manager for GPIO IRQs on GP18–GP21.
class StrumIrq {
public:
	// Tear down IRQs on destruction.
	virtual ~StrumIrq() = default;

	// Update the user callback at runtime (IRQ-safe).
	virtual void set_callback(Callback cb);

	// Non-copyable/non-movable to avoid multiple active instances.
	StrumIrq(const StrumIrq&) = delete;
	StrumIrq& operator=(const StrumIrq&) = delete;
	StrumIrq(StrumIrq&&) = delete;
	StrumIrq& operator=(StrumIrq&&) = delete;

protected:
	StrumIrq() = default;

private:
	// Active instance used by the C-style ISR to forward events.
	// static StrumIrq* s_active;

	// Pico SDK ISR hook (C-style), forwards to instance.
	// static void gpio_isr(uint gpio, uint32_t events);

	// Instance handler invoked by gpio_isr.
	virtual void on_gpio(uint gpio, uint32_t events);

	// Internal init/deinit helpers.
	virtual void init_internal();
	virtual void deinit_internal();

	// State
	// volatile Callback callback_ = nullptr;
	// bool initialized_ = false;
	// std::vector<bool> states;
};

std::unique_ptr<StrumIrq> CreateFakeStrumIrq(Callback);
}  // namespace strum_irq
