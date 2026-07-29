#pragma once

#include <cstdint>

namespace SlimeVR::Sensors {

enum class SensorProbeStatus : uint8_t {
	OK,
	ADDRESS_NACK,
	TRANSMISSION_ERROR,
	READ_FAILURE,
	WHO_AM_I_MISMATCH,
};

[[nodiscard]] constexpr SensorProbeStatus classifySensorTransport(
	uint8_t endTransmissionCode,
	uint8_t requestedBytes,
	uint8_t receivedBytes
) {
	if (endTransmissionCode == 2) {
		return SensorProbeStatus::ADDRESS_NACK;
	}
	if (endTransmissionCode != 0) {
		return SensorProbeStatus::TRANSMISSION_ERROR;
	}
	if (receivedBytes < requestedBytes) {
		return SensorProbeStatus::READ_FAILURE;
	}
	return SensorProbeStatus::OK;
}

[[nodiscard]] constexpr SensorProbeStatus classifySensorProbe(
	uint8_t endTransmissionCode,
	uint8_t requestedBytes,
	uint8_t receivedBytes,
	uint8_t value,
	uint8_t expectedValue
) {
	const auto transportStatus = classifySensorTransport(
		endTransmissionCode,
		requestedBytes,
		receivedBytes
	);
	if (transportStatus != SensorProbeStatus::OK) {
		return transportStatus;
	}
	if (value != expectedValue) {
		return SensorProbeStatus::WHO_AM_I_MISMATCH;
	}
	return SensorProbeStatus::OK;
}

[[nodiscard]] constexpr const char* sensorProbeStatusName(SensorProbeStatus status) {
	switch (status) {
		case SensorProbeStatus::OK:
			return "OK";
		case SensorProbeStatus::ADDRESS_NACK:
			return "ADDRESS_NACK";
		case SensorProbeStatus::TRANSMISSION_ERROR:
			return "TRANSMISSION_ERROR";
		case SensorProbeStatus::READ_FAILURE:
			return "READ_FAILURE";
		case SensorProbeStatus::WHO_AM_I_MISMATCH:
			return "WHO_AM_I_MISMATCH";
	}
	return "UNKNOWN";
}

}  // namespace SlimeVR::Sensors
