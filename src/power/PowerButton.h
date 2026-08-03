#pragma once

#include <Arduino.h>

namespace SlimeVR::Power {

class PowerButton {
public:
	void setup();
	void update();

private:
	void enterDeepSleep();

	bool m_WaitingForRelease = true;
	bool m_Pressed = false;
	uint32_t m_PressedAt = 0;
};

}  // namespace SlimeVR::Power
