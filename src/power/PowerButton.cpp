#include "PowerButton.h"

#include "GlobalVars.h"
#include "logging/Logger.h"

#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
#include <Preferences.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_system.h>
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

#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
// Held in NVS, not RTC memory. RTC memory does survive deep sleep, but reading it
// requires a host to attach, and attaching makes the ESP32-S3 USB-Serial/JTAG issue
// `rst:0x15 (USB_UART_CHIP_RESET)`, which loses the contents — so the act of
// measuring destroyed the measurement. Flash survives every reset.
//
// The counters only ever increase; nothing clears them. A run is measured as the
// difference between the values read before and after, which removes any need to
// decide which resets should zero them.
//
// Keeping the two separate is what makes a failure legible: a wake with no press
// raises sleep entries only, and a failure to sleep raises neither, so the gap
// between them names the broken half.
constexpr const char* kPrefsNamespace = "powerbtn";
constexpr const char* kPrefsSleepKey = "sleeps";
constexpr const char* kPrefsWakeKey = "wakes";

uint32_t cachedSleepEntries = 0;
uint32_t cachedButtonWakes = 0;

void loadCounters() {
	Preferences prefs;
	if (!prefs.begin(kPrefsNamespace, true)) {
		return;
	}
	cachedSleepEntries = prefs.getUInt(kPrefsSleepKey, 0);
	cachedButtonWakes = prefs.getUInt(kPrefsWakeKey, 0);
	prefs.end();
}

void storeCounters() {
	Preferences prefs;
	if (!prefs.begin(kPrefsNamespace, false)) {
		powerButtonLogger.error("Could not open NVS to persist deep sleep counters");
		return;
	}
	prefs.putUInt(kPrefsSleepKey, cachedSleepEntries);
	prefs.putUInt(kPrefsWakeKey, cachedButtonWakes);
	prefs.end();
}
#endif
}  // namespace

uint32_t PowerButton::getSleepEntries() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	return cachedSleepEntries;
#else
	return 0;
#endif
}

uint32_t PowerButton::getButtonWakes() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	return cachedButtonWakes;
#else
	return 0;
#endif
}

void PowerButton::setup() {
#if defined(ESP32) && defined(MOMENTARY_POWER_BUTTON_PIN)
	// enterDeepSleep held the RTC pull-up on this pad. Release the RTC driver
	// before pinMode, otherwise the normal GPIO peripheral does not own the pin.
	rtc_gpio_deinit(static_cast<gpio_num_t>(MOMENTARY_POWER_BUTTON_PIN));

	pinMode(MOMENTARY_POWER_BUTTON_PIN, INPUT_PULLUP);
	m_WaitingForRelease = digitalRead(MOMENTARY_POWER_BUTTON_PIN) == LOW;

	loadCounters();

	const auto wakeCause = esp_sleep_get_wakeup_cause();
	if (wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
		cachedButtonWakes++;
		storeCounters();
		powerButtonLogger.info(
			"Woke from momentary power button on GPIO%d; sleep entries=%u, button "
			"wakes=%u",
			MOMENTARY_POWER_BUTTON_PIN,
			static_cast<unsigned>(cachedSleepEntries),
			static_cast<unsigned>(cachedButtonWakes)
		);
	} else {
		powerButtonLogger.info(
			"Cold start on GPIO%d power button; wake cause %d, reset reason %d, sleep "
			"entries=%u, button wakes=%u",
			MOMENTARY_POWER_BUTTON_PIN,
			static_cast<int>(wakeCause),
			static_cast<int>(esp_reset_reason()),
			static_cast<unsigned>(cachedSleepEntries),
			static_cast<unsigned>(cachedButtonWakes)
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

	cachedSleepEntries++;
	// Persisted before the log line, because the log is the part that can be lost:
	// on this chip the host must not hold the port open across the sleep transition,
	// so nobody may be listening when this prints.
	storeCounters();
	powerButtonLogger.info(
		"Entering deep sleep #%u; press GPIO%d to wake",
		static_cast<unsigned>(cachedSleepEntries),
		MOMENTARY_POWER_BUTTON_PIN
	);
	Serial.flush();
	esp_deep_sleep_start();
#endif
}

}  // namespace SlimeVR::Power
