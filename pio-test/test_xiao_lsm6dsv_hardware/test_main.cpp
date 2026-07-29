#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#include "i2c_safety.h"

namespace {

constexpr uint8_t SdaPin = 5;
constexpr uint8_t SclPin = 6;
constexpr uint8_t SensorAddress = 0x6b;
constexpr uint8_t WhoAmIRegister = 0x0f;
constexpr uint8_t ExpectedWhoAmI = 0x70;

struct CheckedRead {
	uint8_t value = 0;
	uint8_t txCode = 0;
	uint8_t receivedBytes = 0;
};

CheckedRead readWhoAmI() {
	CheckedRead result;
	Wire.beginTransmission(SensorAddress);
	Wire.write(WhoAmIRegister);
	result.txCode = Wire.endTransmission(false);
	if (result.txCode != 0) {
		return result;
	}

	result.receivedBytes = Wire.requestFrom(SensorAddress, static_cast<uint8_t>(1));
	if (result.receivedBytes == 1 && Wire.available()) {
		result.value = Wire.read();
	}
	return result;
}

void assertWhoAmIAtCurrentClock() {
	const auto result = readWhoAmI();
	Serial.printf(
		"0x%02X -> WHO_AM_I = 0x%02X (tx=%u rx=%u)\n",
		SensorAddress,
		result.value,
		result.txCode,
		result.receivedBytes
	);
	TEST_ASSERT_EQUAL_UINT8(0, result.txCode);
	TEST_ASSERT_EQUAL_UINT8(1, result.receivedBytes);
	TEST_ASSERT_EQUAL_HEX8(ExpectedWhoAmI, result.value);
}

void testWhoAmIDirectReadAt100kHz() {
	Serial.println("--- WHO_AM_I direct read at 100 kHz ---");
	Wire.setClock(I2C_STARTUP_SPEED);
	assertWhoAmIAtCurrentClock();
}

void testSafeI2CScan() {
	Serial.println("--- safe I2C scan ---");
	uint8_t foundCount = 0;
	uint8_t foundAddress = 0;
	for (
		uint8_t address = SlimeVR::I2C::SafeAddressMin;
		address <= SlimeVR::I2C::SafeAddressMax;
		++address
	) {
		Wire.beginTransmission(address);
		if (Wire.endTransmission() == 0) {
			Serial.printf("found: 0x%02X\n", address);
			foundAddress = address;
			++foundCount;
		}
	}
	TEST_ASSERT_EQUAL_UINT8(1, foundCount);
	TEST_ASSERT_EQUAL_HEX8(SensorAddress, foundAddress);
}

void testConfiguredRuntimeClock() {
	if (I2C_SPEED == I2C_STARTUP_SPEED) {
		TEST_IGNORE_MESSAGE("Runtime clock is the confirmed 100 kHz production value");
	}
	Serial.printf("--- WHO_AM_I direct read at %u Hz ---\n", I2C_SPEED);
	Wire.setClock(I2C_SPEED);
	assertWhoAmIAtCurrentClock();
}

}  // namespace

void setup() {
	Serial.begin(115200);
	delay(2000);
	Wire.begin(SdaPin, SclPin);
	Wire.setClock(I2C_STARTUP_SPEED);
	delay(500);

	UNITY_BEGIN();
	RUN_TEST(testWhoAmIDirectReadAt100kHz);
	RUN_TEST(testSafeI2CScan);
	RUN_TEST(testConfiguredRuntimeClock);
	UNITY_END();
}

void loop() { delay(1000); }
