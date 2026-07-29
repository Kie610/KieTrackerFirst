#pragma once

#include <cstdint>

namespace SlimeVR::Sensors {

template <typename Sensor>
[[nodiscard]] constexpr uint8_t resolveSensorAddress(bool useAlternateAddress) {
	if (!useAlternateAddress) {
		return Sensor::Address;
	}
	if constexpr (requires { Sensor::AlternateAddress; }) {
		return Sensor::AlternateAddress;
	}
	return static_cast<uint8_t>(Sensor::Address + 1);
}

}  // namespace SlimeVR::Sensors
