#include "PowerButton.h"

#include "GlobalVars.h"
#include "logging/Logger.h"

#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_attr.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#endif

#if defined(MOMENTARY_POWER_BUTTON_PIN) \
	&& !defined(MOMENTARY_POWER_BUTTON_HOLD_MS)
#define MOMENTARY_POWER_BUTTON_HOLD_MS 2000
#endif

#if defined(MOMENTARY_POWER_BUTTON_PIN) \
	&& !defined(MOMENTARY_POWER_BUTTON_RELEASE_DEBOUNCE_MS)
// The released level must stay stable for this long before the active-low wake
// source is armed. Arming while the contact still bounces makes EXT0 fire
// immediately and the tracker wakes up without a press.
#define MOMENTARY_POWER_BUTTON_RELEASE_DEBOUNCE_MS 50
#endif

#if defined(ESP32S3) && defined(MOMENTARY_POWER_BUTTON_PIN)
static_assert(
	MOMENTARY_POWER_BUTTON_PIN >= 0 && MOMENTARY_POWER_BUTTON_PIN <= 21,
	"ESP32-S3 deep-sleep wake pin must be an RTC GPIO (0..21)"
);
#endif

#if defined(ESP32S3) && defined(MOMENTARY_POWER_BUTTON_PIN) \
	&& defined(PIN_IMU_INT)
static_assert(
	MOMENTARY_POWER_BUTTON_PIN != PIN_IMU_INT,
	"The power button and the IMU INT1 line cannot share one pin"
);
// INT1 is unused by this firmware, but it is wired and kept on an RTC GPIO so a
// later wake-on-motion path can arm it as an EXT1 source without rewiring.
static_assert(
	PIN_IMU_INT >= 0 && PIN_IMU_INT <= 21,
	"IMU INT1 must stay on an RTC GPIO (0..21) to remain usable as a wake source"
);
#endif

namespace SlimeVR::Power {

namespace {
SlimeVR::Logging::Logger powerButtonLogger{"PowerButton"};

#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
// Retained across deep sleep, cleared on a real power-on or external reset.
// The ten-cycle sleep/wake hardware check reads these off the serial log
// instead of relying on the operator to count presses.
RTC_DATA_ATTR uint32_t rtcDeepSleepEntries = 0;
RTC_DATA_ATTR uint32_t rtcButtonWakeCount = 0;
#endif
}  // namespace

void PowerButton::setup() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	const auto wakePin = static_cast<gpio_num_t>(MOMENTARY_POWER_BUTTON_PIN);
	const auto wakeCause = esp_sleep_get_wakeup_cause();

	// enterDeepSleep held the RTC pull-up on this pad. Release the RTC driver
	// before pinMode, otherwise the normal GPIO peripheral does not own the pin.
	rtc_gpio_deinit(wakePin);

	pinMode(MOMENTARY_POWER_BUTTON_PIN, INPUT_PULLUP);
	m_WaitingForRelease = digitalRead(MOMENTARY_POWER_BUTTON_PIN) == LOW;

	if (wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
		++rtcButtonWakeCount;
		powerButtonLogger.info(
			"Woke from momentary power button on GPIO%d; sleep entries=%u, button "
			"wakes=%u",
			MOMENTARY_POWER_BUTTON_PIN,
			static_cast<unsigned>(rtcDeepSleepEntries),
			static_cast<unsigned>(rtcButtonWakeCount)
		);
	} else {
		// Power-on or external reset: the retained counters are not meaningful.
		rtcDeepSleepEntries = 0;
		rtcButtonWakeCount = 0;
		powerButtonLogger.info(
			"Cold start on GPIO%d power button; wake cause %d, hold %u ms to sleep",
			MOMENTARY_POWER_BUTTON_PIN,
			static_cast<int>(wakeCause),
			static_cast<unsigned>(MOMENTARY_POWER_BUTTON_HOLD_MS)
		);
	}

	if (m_WaitingForRelease) {
		powerButtonLogger.info(
			"Power button is still held at boot; sleep stays disarmed until release"
		);
	}
#endif
}

void PowerButton::update() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	const bool pressed = digitalRead(MOMENTARY_POWER_BUTTON_PIN) == LOW;

	if (m_WaitingForRelease) {
		if (!pressed) {
			m_WaitingForRelease = false;
		}
		return;
	}

	if (!pressed) {
		m_Pressed = false;
		return;
	}

	if (!m_Pressed) {
		m_Pressed = true;
		m_PressedAt = millis();
		return;
	}

	if (millis() - m_PressedAt >= MOMENTARY_POWER_BUTTON_HOLD_MS) {
		enterDeepSleep();
	}
#endif
}

void PowerButton::waitForDebouncedRelease() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	uint32_t releasedAt = millis();
	while (millis() - releasedAt < MOMENTARY_POWER_BUTTON_RELEASE_DEBOUNCE_MS) {
		if (digitalRead(MOMENTARY_POWER_BUTTON_PIN) == LOW) {
			releasedAt = millis();
		}
		delay(5);
	}
#endif
}

void PowerButton::enterDeepSleep() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	powerButtonLogger.info(
		"Power button held for %u ms; release to enter deep sleep",
		static_cast<unsigned>(MOMENTARY_POWER_BUTTON_HOLD_MS)
	);

	waitForDebouncedRelease();

	sensorManager.prepareForSleep();
	ledManager.off();
	WiFi.disconnect(true, false);
	esp_wifi_stop();
	Serial.flush();

	const auto wakePin = static_cast<gpio_num_t>(MOMENTARY_POWER_BUTTON_PIN);
	// The internal pull-up is not retained through deep sleep, so hold the RTC
	// pull-up as well. The external resistor stays required; this only keeps the
	// wake input defined if it is missing or intermittent.
	rtc_gpio_pullup_en(wakePin);
	rtc_gpio_pulldown_dis(wakePin);
	if (esp_sleep_enable_ext0_wakeup(wakePin, 0) != ESP_OK) {
		powerButtonLogger.error(
			"GPIO%d cannot be configured as a deep-sleep wake source",
			MOMENTARY_POWER_BUTTON_PIN
		);
		m_Pressed = false;
		m_WaitingForRelease = true;
		return;
	}

	++rtcDeepSleepEntries;
	powerButtonLogger.info(
		"Entering deep sleep #%u; press GPIO%d to wake",
		static_cast<unsigned>(rtcDeepSleepEntries),
		MOMENTARY_POWER_BUTTON_PIN
	);
	Serial.flush();
	esp_deep_sleep_start();
#endif
}

}  // namespace SlimeVR::Power
