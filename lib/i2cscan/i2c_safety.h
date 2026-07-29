#pragma once

#include <cstdint>

namespace SlimeVR::I2C {

static constexpr uint8_t SafeAddressMin = 0x08;
static constexpr uint8_t SafeAddressMax = 0x77;

[[nodiscard]] constexpr bool isSafeAddress(uint8_t address) {
	return address >= SafeAddressMin && address <= SafeAddressMax;
}

}  // namespace SlimeVR::I2C
