# Agent handoff v1

updated: 2026-08-03
repo: Kie610/KieTrackerFirst
work_branch: codex/momentary-deep-sleep
upstream: none
base: codex/xiao-lsm6dsv-handoff@a08c0ac
goal: Prototype and verify switchless GPIO7 momentary-button Deep Sleep for the XIAO ESP32-S3 + LSM6DSV tracker.

## State

complete:
- C: XIAO defaults use LSM6DSV `0x6B`/`DEG_0`, GPIO5/6/4/2/1, `BAT_INTERNAL`, and 100 kHz production I2C; safe probing and runtime Wi-Fi provisioning remain intact.
- C: Replacement IMU passed ten 100 kHz power-removal cycles; the prior shorted IMU remains isolated.
- C: GPIO7 active-low momentary-button handling, two-second hold/release, EXT0 wake, Wi-Fi/LED shutdown, and LSM6DSV FIFO/gyro/accel power-down are implemented.

verified:
- C: 2026-08-03 — evidence: status=PASS; kind=compile; command=PlatformIO run for BOARD_XIAO_ESP32S3, 400KHZ_DIAGNOSTIC, BOARD_WEMOSD1MINI, and BOARD_XIAO_ESP32C3; environment=Windows/PlatformIO 6.1.19; scope=four firmware environments; counts=passed=4, failed=0, skipped=0, not-run=0
- C: 2026-08-03 — evidence: status=PASS; kind=compile; command=platformio test for both XIAO environments with --without-uploading --without-testing; environment=Windows/PlatformIO 6.1.19; scope=four test-firmware targets, compilation only; counts=passed=4, failed=0, skipped=0, not-run=0
- C: 2026-08-01 — evidence: status=PASS; kind=hardware; command=serial reset after power cycles 1--10/10; environment=Windows/XIAO/COM6/100 kHz; scope=replacement IMU `0x6B`, `WHO_AM_I=0x70`, rest calibration; counts=passed=20, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=runtime; command=reset+motion+10-minute GUI; environment=Windows/SlimeVR20.1.0/XIAO/COM6/100 kHz; scope=calibration, Preview, `DEG_0`, 0% packet loss, stable heading; counts=passed=14, failed=0, skipped=0, not-run=0

not-run:
- U: U2 final cell, protection, connector, charge current, ADC divider, and enclosure remain unresolved.
- U: U3 400 kHz hardware comparison, detailed disconnected-sensor logs, and eight-hour endurance remain not run.
- U: U4 momentary-button Sleep/Wake runtime, ten-cycle hardware run, and completed-tracker Deep Sleep current remain not run.
- A: Six-face acceleration calibration remains unnecessary unless later mounting tests show material error.

## Decisions

- C: Keep 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, `0x6B`, `0x0F == 0x70`, 100 kHz production I2C, safe scan `0x08..0x77`, and alternate `0x6A`.
- C: Use a normally-open button from GPIO7 to GND plus external 10 kOhm pull-up to 3V3; never route battery current through it.
- C: Hold two seconds, release to sleep, and press to wake; boot stays disarmed until the first release.
- C: Preserve the network packet format; detailed probe failures remain serial-only and map to `SENSOR_ERROR`.

## Next

- Flash XIAO and run short-press, hold/release, wake-log, IMU identity, Server reconnect, and ten-cycle checks; blocked-by: U4
- Measure complete-tracker awake/Deep Sleep current and temperature on battery; blocked-by: U2
- Capture disconnected-sensor failure logs; blocked-by: none
- Run optional 400 kHz comparison and eight-hour endurance; blocked-by: U3

## Paths

- C: implementation: `src/power/PowerButton.*`, `src/main.cpp`, `src/sensors/SensorManager.*`, `src/sensors/softfusion/`
- C: procedure: `docs/momentary-button-deep-sleep.md`, `docs/xiao-esp32s3-lsm6dsv.md`
- C: tests: `pio-test/test_i2c_contract/test_main.cpp`, `pio-test/test_xiao_lsm6dsv_hardware/test_main.cpp`
- C: forbidden legacy probe: `test/I2C_TEST.cpp`

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Recheck branch, worktree, upstream, hardware, and tests; prefer live evidence.
3. Run the smallest relevant check, then the first unblocked `Next` item.
4. Update exact evidence; never promote compilation to runtime or hardware PASS.
