#include <Arduino.h>
#include <unity.h>

#include "consts.h"
#include "i2c_safety.h"
#include "sensor_address_resolver.h"
#include "sensor_probe.h"

using SlimeVR::I2C::isSafeAddress;
using SlimeVR::Sensors::SensorProbeStatus;
using SlimeVR::Sensors::classifySensorProbe;
using SlimeVR::Sensors::resolveSensorAddress;

namespace {

struct Lsm6dsvAddressFixture {
	static constexpr uint8_t Address = 0x6b;
	static constexpr uint8_t AlternateAddress = 0x6a;
};

struct IncrementingAddressFixture {
	static constexpr uint8_t Address = 0x68;
};

static_assert(isSafeAddress(0x08));
static_assert(isSafeAddress(0x77));
static_assert(!isSafeAddress(0x07));
static_assert(!isSafeAddress(0x78));
static_assert(!isSafeAddress(0x7e));
static_assert(!isSafeAddress(0x7f));

static_assert(classifySensorProbe(0, 1, 1, 0x70, 0x70) == SensorProbeStatus::OK);
static_assert(
	classifySensorProbe(2, 1, 0, 0x00, 0x70) == SensorProbeStatus::ADDRESS_NACK
);
static_assert(
	classifySensorProbe(4, 1, 0, 0x00, 0x70)
	== SensorProbeStatus::TRANSMISSION_ERROR
);
static_assert(
	classifySensorProbe(0, 1, 0, 0x00, 0x70) == SensorProbeStatus::READ_FAILURE
);
static_assert(
	classifySensorProbe(0, 1, 1, 0x69, 0x70)
	== SensorProbeStatus::WHO_AM_I_MISMATCH
);

static_assert(resolveSensorAddress<Lsm6dsvAddressFixture>(false) == 0x6b);
static_assert(resolveSensorAddress<Lsm6dsvAddressFixture>(true) == 0x6a);
static_assert(resolveSensorAddress<Lsm6dsvAddressFixture>(true) != 0x6c);
static_assert(resolveSensorAddress<IncrementingAddressFixture>(true) == 0x69);

#if BOARD == BOARD_XIAO_ESP32S3
static_assert(PIN_IMU_SDA == 5);
static_assert(PIN_IMU_SCL == 6);
static_assert(PIN_IMU_INT == 4);
static_assert(PIN_IMU_INT_2 == 2);
static_assert(PIN_BATTERY_LEVEL == 1);
static_assert(BATTERY_MONITOR == BAT_INTERNAL);
static_assert(BATTERY_SHIELD_RESISTANCE == 180);
static_assert(BATTERY_SHIELD_R1 == 100);
static_assert(BATTERY_SHIELD_R2 == 220);
static_assert(BOARD == BOARD_XIAO_ESP32S3);
static_assert(I2C_STARTUP_SPEED == 100000);
static_assert(I2C_SPEED == 100000 || I2C_SPEED == 400000);
static_assert(MOMENTARY_POWER_BUTTON_PIN == 7);
static_assert(MOMENTARY_POWER_BUTTON_HOLD_MS == 2000);
static_assert(MOMENTARY_POWER_BUTTON_PIN != PIN_IMU_SDA);
static_assert(MOMENTARY_POWER_BUTTON_PIN != PIN_IMU_SCL);
static_assert(MOMENTARY_POWER_BUTTON_PIN != PIN_IMU_INT);
#endif

void testCompileTimeContractsArePresent() {
	TEST_ASSERT_TRUE_MESSAGE(
		true,
		"32 I2C, XIAO, and power-button contract cases passed"
	);
}

}  // namespace

void setup() {
	delay(2000);
	UNITY_BEGIN();
	RUN_TEST(testCompileTimeContractsArePresent);
	UNITY_END();
}

void loop() { delay(1000); }
