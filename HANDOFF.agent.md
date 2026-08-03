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
- C: Release detection now debounces for `MOMENTARY_POWER_BUTTON_RELEASE_DEBOUNCE_MS` (50 ms) and the RTC pull-up is held on GPIO7 before EXT0 is armed, so contact bounce cannot arm the wake source while the pin is still low.
- C: `docs/wiring-xiao-lsm6dsv.svg` is the regenerated connection diagram. Its pad order was checked against the official Seeed images `XIAO_ESP32-S3_front_pinout.png` and `_back_pinout.png`: left column `D0..D6`, right column `VBUS`/`GND`/`3.3V-OUT`/`D10`/`D9`/`D8`/`D7`, `BAT+`/`BAT-` on the back. The same images mark GPIO7 as an RTC pin, which is why EXT0 wake is valid. Module pin names were read from the BOOTH product photographs (silkscreen revision `5265159A-P1-241122`): row of seven `OSDO`/`3V3`/`GND`/`SCL`/`SDA`/`CS`/`SDO`, row of five `OCS`/`INT2`/`INT1`/`SCX`/`SDX`. All six used pins are in the seven-pin row and the supply pin is labelled `3V3`.
- C: Owner measurement of 2026-08-03 (unpowered multimeter): `SDA`, `SCL`, `SDO`, and `CS` each read 4.6 kOhm to `3V3`, so the module carries its own 4.7 kOhm pull-ups on all four. SA0 and CS are held high without wires, which is why the device answers at `0x6B`, and no external I2C pull-up is needed. Module wiring is therefore `3V3`, `GND`, `SDA`, `SCL` only. The top-side `SDO` jumper is open and its free pad is continuous to GND, so it selects `0x6A` and must stay open.
- C: The Notion guide was corrected on 2026-08-03: chapter 05 had `SA0 -> GND -> 0x6A`, which contradicts the confirmed `SA0 High -> 0x6B`. Chapter 05 now carries the diagram, the `0x6B` strap, a D8/GPIO7 button row, and 100 kHz pull-up guidance; chapter 01 now uses the Seeed-documented 50 mA (Fast) / 3.8 mA (Trickle) charge current and flags its 100 mA column as unverified.
- C: The build target is an ElectroCookie mini solderable breadboard, 17 columns by 5+5 rows wired 1:1 like a breadboard, 50.8 x 38.1 mm. For each column, `A`-`E` are one node and `F`-`J` another, so both modules must straddle the centre channel or their pins short. `docs/perfboard-netlist.md` holds the parts, the eight nets, the deliberately unconnected pins, and the legal row pairs.
- C: `hardware/tracker-perfboard/` holds a Konnect-generated KiCad project skeleton (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`). The schematic is still empty; `docs/perfboard-schematic-build-spec.md` is the self-contained instruction for building it, including the `U1`/`U2` custom symbol definitions and nine nets (it splits `BAT+` into `BAT+_RAW` across the optional switch).
- C: Branch `codex/momentary-deep-sleep` was pushed to `origin` on 2026-08-03 with upstream tracking; local and remote match.
- U: This branch is five documentation commits behind `xiao-lsm6dsv`@583a1ae (822ff7d, 1153ef1, 84f6bb3, 04fbb29, 583a1ae). No code conflicts; not synced without owner authorization.

verified:
- C: 2026-08-03 — evidence: status=PASS; kind=compile; command=PlatformIO run for BOARD_XIAO_ESP32S3, 400KHZ_DIAGNOSTIC, BOARD_WEMOSD1MINI, and BOARD_XIAO_ESP32C3; environment=Windows/PlatformIO 6.1.19; scope=four firmware environments; counts=passed=4, failed=0, skipped=0, not-run=0
- C: 2026-08-03 — evidence: status=PASS; kind=compile; command=platformio test for both XIAO environments with --without-uploading --without-testing; environment=Windows/PlatformIO 6.1.19; scope=four test-firmware targets, compilation only; counts=passed=4, failed=0, skipped=0, not-run=0
- C: 2026-08-01 — evidence: status=PASS; kind=hardware; command=serial reset after power cycles 1--10/10; environment=Windows/XIAO/COM6/100 kHz; scope=replacement IMU `0x6B`, `WHO_AM_I=0x70`, rest calibration; counts=passed=20, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=runtime; command=reset+motion+10-minute GUI; environment=Windows/SlimeVR20.1.0/XIAO/COM6/100 kHz; scope=calibration, Preview, `DEG_0`, 0% packet loss, stable heading; counts=passed=14, failed=0, skipped=0, not-run=0

- C: 2026-08-03 — evidence: status=PASS; kind=compile; command=PlatformIO run for BOARD_XIAO_ESP32S3, 400KHZ_DIAGNOSTIC, BOARD_WEMOSD1MINI, BOARD_XIAO_ESP32C3, plus test for both XIAO environments with --without-uploading --without-testing and `git diff --check`; environment=Windows/PlatformIO, no hardware connected, no I2C clock applied; scope=re-verification after the debounce and RTC pull-up change, compilation only with 0 executed test cases; counts=passed=8, failed=0, skipped=0, not-run=4 hardware/Unity runs

not-run:
- U: U6 no serial port was enumerated on 2026-08-03, so no button, sleep, or wake behaviour has been observed on hardware.
- U: U8 the schematic has not been built. Konnect's `load_toolset` succeeds server-side but the loaded tools stay uncallable in a non-interactive session, so this needs an interactive `claude` session, and `kicad-cli` must be on PATH first (`C:\Program Files\KiCad\10.0\bin`) or ERC and exports fail.
- U: U9 the pad-row spacing of the XIAO and of the LSM6DSV module has not been measured, so no placement on the ElectroCookie grid is fixed. Calipers settle it.
- U: U10 two build decisions are open: whether a slide switch sits in the `BAT+` line, and whether the divider uses 100 kOhm (about 21 uA) or 220 kOhm (about 9.5 uA).
- U: U2 final cell, protection, connector, charge current, ADC divider, and enclosure remain unresolved.
- U: U3 400 kHz hardware comparison, detailed disconnected-sensor logs, and eight-hour endurance remain not run.
- U: U4 momentary-button Sleep/Wake runtime, ten-cycle hardware run, and completed-tracker Deep Sleep current remain not run.
- A: Six-face acceleration calibration remains unnecessary unless later mounting tests show material error.

## Decisions

- C: Keep 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, `0x6B`, `0x0F == 0x70`, 100 kHz production I2C, safe scan `0x08..0x77`, and alternate `0x6A`.
- C: Use a normally-open button from GPIO7 to GND plus external 10 kOhm pull-up to 3V3; never route battery current through it.
- C: Hold two seconds, release to sleep, and press to wake; boot stays disarmed until the first release.
- C: Preserve the network packet format; detailed probe failures remain serial-only and map to `SENSOR_ERROR`.
- C: Owner decision of 2026-08-03 — this branch is not merged into `xiao-lsm6dsv` while the Deep Sleep behaviour is unproven on hardware. Merge only after the button, wake, IMU identity, Server reconnect, and ten-cycle checks pass.

## Next

- Flash XIAO and run short-press, hold/release, wake-log, IMU identity, Server reconnect, and ten-cycle checks; blocked-by: U6 (tracker not connected)
- Measure complete-tracker awake/Deep Sleep current and temperature on battery; blocked-by: U2
- Capture disconnected-sensor failure logs; blocked-by: none
- Build the schematic per `docs/perfboard-schematic-build-spec.md`; blocked-by: U8 (interactive session and `kicad-cli` on PATH)
- Measure both modules' pad-row spacing, then fix the ElectroCookie placement; blocked-by: U9
- Decide the `BAT+` slide switch and the divider resistor value; blocked-by: U10
- Run optional 400 kHz comparison and eight-hour endurance; blocked-by: U3

## Paths

- C: implementation: `src/power/PowerButton.*`, `src/main.cpp`, `src/sensors/SensorManager.*`, `src/sensors/softfusion/`
- C: procedure: `docs/momentary-button-deep-sleep.md`, `docs/xiao-esp32s3-lsm6dsv.md`, `docs/wiring-xiao-lsm6dsv.svg`
- C: hardware build: `docs/perfboard-netlist.md`, `docs/perfboard-schematic-build-spec.md`, `hardware/tracker-perfboard/`
- C: tests: `pio-test/test_i2c_contract/test_main.cpp`, `pio-test/test_xiao_lsm6dsv_hardware/test_main.cpp`
- C: forbidden legacy probe: `test/I2C_TEST.cpp`

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Recheck branch, worktree, upstream, hardware, and tests; prefer live evidence.
3. Run the smallest relevant check, then the first unblocked `Next` item.
4. Update exact evidence; never promote compilation to runtime or hardware PASS.
