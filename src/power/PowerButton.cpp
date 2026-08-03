#include "PowerButton.h"

#include "GlobalVars.h"
#include "logging/Logger.h"

#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
#include <WiFi.h>
#include <driver/rtc_io.h>
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

namespace SlimeVR::Power {

namespace {
SlimeVR::Logging::Logger powerButtonLogger{"PowerButton"};
}

void PowerButton::setup() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	pinMode(MOMENTARY_POWER_BUTTON_PIN, INPUT_PULLUP);
	m_WaitingForRelease = digitalRead(MOMENTARY_POWER_BUTTON_PIN) == LOW;

	const auto wakeCause = esp_sleep_get_wakeup_cause();
	if (wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
		powerButtonLogger.info("Woke from momentary power button");
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

	powerButtonLogger.info("Entering deep sleep; press the button to wake");
	Serial.flush();
	esp_deep_sleep_start();
#endif
}

}  // namespace SlimeVR::Power
