# Momentary button Deep Sleep prototype

This branch prototypes switchless power control for the XIAO ESP32-S3 tracker.
It does not electrically disconnect the cell. A protected one-cell Li-ion/LiPo
and a removable battery connection remain required.

## Wiring

The connection diagram is [`wiring-xiao-lsm6dsv.svg`](wiring-xiao-lsm6dsv.svg). Its
XIAO pad order matches the official Seeed front pinout image, which also marks
GPIO7 as an RTC pin, so it is valid as an EXT0 wake source. The module pin order
is not documented by the seller; read it from the silkscreen.

- Cell positive goes directly to `BAT+`; cell negative goes directly to `BAT-`.
- Connect a normally-open momentary button between XIAO `D8` / GPIO7 and GND.
- Add an external 10 kOhm pull-up from GPIO7 to 3V3. The firmware also enables
  the internal pull-up, but the external resistor keeps the RTC wake input in a
  defined state through Deep Sleep.
- Keep the LSM6DSV wiring at 3.3 V, SDA GPIO5, SCL GPIO6, and address `0x6B`.
- Run `INT1` to `D9` / GPIO8. It is not a wake source in this prototype and the
  firmware never reads it, but GPIO8 is an RTC GPIO, so leaving the wire in place
  keeps a later wake-on-motion path open without opening a finished assembly.
  `D10` / GPIO9 is the alternate. Neither is a strapping pin.

Never put the momentary button in series with the battery. It is a logic input,
not a power switch.

## User interaction

1. Hold the button for two seconds while the tracker is running.
2. Release it. The firmware then powers down the IMU and enters Deep Sleep.
3. Press the button once to wake. Deep Sleep wake is a reset, so Wi-Fi, the IMU,
   calibration state, and the SlimeVR connection initialize normally again.

At boot, button handling stays disarmed until the button is released once. This
prevents a wake press that is held too long from immediately requesting sleep.

## Firmware sequence

`PowerButton` waits for the released level to stay stable for
`MOMENTARY_POWER_BUTTON_RELEASE_DEBOUNCE_MS` (50 ms), stops the active sensors
through `SensorManager::prepareForSleep`, turns off the LED and Wi-Fi, holds the
RTC pull-up on GPIO7, configures GPIO7 as an active-low EXT0 wake source, and
starts Deep Sleep. Arming EXT0 while the contact still bounces would wake the
tracker immediately. The LSM6DSV driver changes FIFO mode and batching to zero,
then writes zero to `CTRL2_G` and `CTRL1_XL` so both ODR fields select power-down.

On the next boot `PowerButton::setup` calls `rtc_gpio_deinit` before `pinMode`,
so the pad returns from the RTC driver to the normal GPIO peripheral after an
EXT0 wake.

Two counters live in RTC memory and survive Deep Sleep: `rtcDeepSleepEntries`
and `rtcButtonWakeCount`. A power-on or external reset clears both. Every boot
logs which of the two paths it took, so the ten-cycle sleep/wake check reads its
count off the serial log instead of relying on the operator to count presses:

```text
Entering deep sleep #7; press GPIO7 to wake
Woke from momentary power button on GPIO7; sleep entries=7, button wakes=7
```

A cold start instead logs `Cold start on GPIO7 power button; wake cause 0`, and
a button held at boot logs that sleep stays disarmed until it is released.

The XIAO PlatformIO environments enable this prototype with:

```text
-DMOMENTARY_POWER_BUTTON_PIN=7
-DMOMENTARY_POWER_BUTTON_HOLD_MS=2000
```

## Hardware verification (not proven by compilation)

1. With USB power only, verify that a press shorter than two seconds does not
   stop tracking.
2. Hold for two seconds and release. Confirm the final serial log and LED off.
3. Press once and confirm a fresh boot plus `Woke from momentary power button`.
4. Confirm LSM6DSV address `0x6B`, `WHO_AM_I == 0x70`, and 100 kHz I2C after
   every wake.
5. Confirm SlimeVR Server reconnects and motion remains correct.
6. Repeat the sleep/wake cycle ten times.
7. With USB removed, measure completed-tracker current while awake and asleep.
8. Check XIAO, IMU, battery, and wiring for abnormal heat while running,
   sleeping, and charging.

USB insertion may charge the cell without waking the ESP32-S3. Use the button to
wake. Do not claim Deep Sleep current or hardware PASS until the complete tracker
is measured.

## Primary references

- Espressif ESP32-S3 sleep modes:
  <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/sleep_modes.html>
- Seeed XIAO ESP32-S3 low-power documentation:
  <https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/>
- ST LSM6DSV datasheet:
  <https://www.st.com/resource/en/datasheet/lsm6dsv.pdf>
