#pragma once

#include <Arduino.h>

namespace SlimeVR::Power {

class PowerButton {
public:
	void setup();
	void update();

	// Deep-sleep cycle counters held in RTC memory. Exposed so the periodic status
	// output can report them, which is what makes a multi-cycle run observable
	// without rebooting the board to read a boot line.
	static uint32_t getSleepEntries();
	static uint32_t getButtonWakes();

private:
	void waitForDebouncedRelease();
	void enterDeepSleep();

	bool m_WaitingForRelease = true;
	bool m_Pressed = false;
	uint32_t m_PressedAt = 0;
};

}  // namespace SlimeVR::Power
