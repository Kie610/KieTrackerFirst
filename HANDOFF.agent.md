# Agent handoff v1

updated: 2026-08-04 (INT1 -> GPIO8, sleep/wake counters, Notion supplement rebuilt)
repo: Kie610/KieTrackerFirst
primary_branch: xiao-lsm6dsv
fork_source: upstream = SlimeVR/SlimeVR-Tracker-ESP, mirrored by `main` only
work_branch: xiao-lsm6dsv
upstream: origin/xiao-lsm6dsv
base: origin/main@5e680f7
goal: XIAO ESP32-S3 + LSM6DSV small wireless SlimeVR tracker

## State

complete:
- C: Production defaults are XIAO ESP32-S3, LSM6DSV `0x6B`, WHO_AM_I `0x0F=0x70`, GPIO5/6, and 100 kHz. Scans use `0x08..0x77`; `0x7E` is never transmitted.
- C: Startup diagnostics classify address NACK, transmission error, short read, and WHO_AM_I mismatch; network status remains `SENSOR_ERROR`.
- C: `DEG_0` matches the tested mounting; SlimeVR Server/Preview and stationary drift checks passed.
- C: The replacement IMU passed ten 100 kHz power-removal cycles; the prior shorted IMU is isolated from power.
- C: No Wi-Fi credentials are tracked; runtime provisioning is required.
- C: GPIO7 active-low momentary-button handling, two-second hold/release, EXT0 wake, Wi-Fi/LED shutdown, and LSM6DSV FIFO/gyro/accel power-down are implemented.
- C: Release detection debounces for `MOMENTARY_POWER_BUTTON_RELEASE_DEBOUNCE_MS` (50 ms) and the RTC pull-up is held on GPIO7 before EXT0 is armed, so contact bounce cannot arm the wake source while the pin is still low.
- C: `docs/wiring-xiao-lsm6dsv.svg` is the regenerated connection diagram. Its pad order was checked against the official Seeed images `XIAO_ESP32-S3_front_pinout.png` and `_back_pinout.png`: left column `D0..D6`, right column `VBUS`/`GND`/`3.3V-OUT`/`D10`/`D9`/`D8`/`D7`, `BAT+`/`BAT-` on the back. The same images mark GPIO7 as an RTC pin, which is why EXT0 wake is valid. Module pin names were read from the BOOTH product photographs (silkscreen revision `5265159A-P1-241122`): row of seven `OSDO`/`3V3`/`GND`/`SCL`/`SDA`/`CS`/`SDO`, row of five `OCS`/`INT2`/`INT1`/`SCX`/`SDX`. All six used pins are in the seven-pin row and the supply pin is labelled `3V3`.
- C: Owner measurement of 2026-08-03 (unpowered multimeter): `SDA`, `SCL`, `SDO`, and `CS` each read 4.6 kOhm to `3V3`, so the module carries its own 4.7 kOhm pull-ups on all four. SA0 and CS are held high without wires, which is why the device answers at `0x6B`, and no external I2C pull-up is needed. Module wiring is therefore `3V3`, `GND`, `SDA`, `SCL` only. The top-side `SDO` jumper is open and its free pad is continuous to GND, so it selects `0x6A` and must stay open.
- C: The Notion guide was corrected on 2026-08-03: chapter 05 had `SA0 -> GND -> 0x6A`, which contradicts the confirmed `SA0 High -> 0x6B`. Chapter 05 now carries the diagram, the `0x6B` strap, a D8/GPIO7 button row, and 100 kHz pull-up guidance; chapter 01 now uses the Seeed-documented 50 mA (Fast) / 3.8 mA (Trickle) charge current and flags its 100 mA column as unverified.
- C: The build target is an ElectroCookie mini solderable breadboard, 17 columns by 5+5 rows wired 1:1 like a breadboard, 50.8 x 38.1 mm. For each column, `A`-`E` are one node and `F`-`J` another, so both modules must straddle the centre channel or their pins short. `docs/perfboard-netlist.md` holds the parts, the eight nets, the deliberately unconnected pins, and the legal row pairs.
- C: `hardware/tracker-perfboard/` holds a Konnect-generated KiCad project skeleton (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`). The schematic is still empty; `docs/perfboard-schematic-build-spec.md` is the self-contained instruction for building it, including the `U1`/`U2` custom symbol definitions and nine nets (it splits `BAT+` into `BAT+_RAW` across the optional switch).
- C: `IMU_INT1` moved from `D3`/GPIO4 to `D9`/GPIO8 on 2026-08-04 at owner request (GPIO8 or GPIO9). Both are ESP32-S3 RTC GPIOs and neither is a strapping pin, so the choice is layout-driven: `D9` is adjacent to the `D8`/GPIO7 button pad in the right column, which keeps the button, INT1, `3V3`, and `GND` on one board edge and drops the wrap-around crossings from four to two. `D10`/GPIO9 is the documented alternate. The firmware still never reads INT1; the wire and the pin reservation only keep a later EXT1 wake-on-motion path open.
- C: `PowerButton` keeps `rtcDeepSleepEntries` and `rtcButtonWakeCount` in RTC memory, cleared on power-on or external reset, and logs the wake path on every boot. This supplies the ten-cycle sleep/wake evidence without operator counting. `setup` now calls `rtc_gpio_deinit` before `pinMode` so the pad leaves the RTC driver after an EXT0 wake.
- C: `xiao-lsm6dsv` contains the codex implementation, the claude handoff update through `04fbb29`, the 2026-08-01 hardware record merged from `codex/xiao-lsm6dsv-handoff`@`7142cec`, and the momentary-button Deep Sleep work merged from `codex/momentary-deep-sleep`@`d5e5bf3`; `main` remains the upstream mirror.

verified:
- C: 2026-08-04 — evidence: status=PASS; kind=compile; command=PlatformIO four firmware environments plus `test` for both XIAO environments with `--without-uploading --without-testing` and `git diff --check`; environment=Windows/PlatformIO 6.1.19, no hardware connected, no I2C clock applied; scope=re-verification after the INT1 GPIO4->GPIO8 move, the RTC sleep/wake counters, and the expanded contract asserts; compilation only with 0 executed test cases; counts=passed=8, failed=0, skipped=0, not-run=4 hardware/Unity runs
- C: 2026-08-01 — evidence: status=PASS; kind=hardware; command=serial reset after power cycles 1--10/10; environment=Windows/XIAO ESP32-S3/COM6/100 kHz; scope=replacement IMU `0x6B` WHO_AM_I=`0x70` plus gyro/rest calibration on every cycle; counts=passed=20, failed=0, skipped=0, not-run=0
- C: 2026-08-01 — evidence: status=FAIL; kind=hardware; command=serial capture after USB reconnect; environment=Windows/XIAO ESP32-S3/COM6/100 kHz; scope=prior IMU held SCL low, `0x6B` READ_FAILURE tx=0 rx=0/1, and measured about 0 ohms VCC to GND while unpowered; counts=passed=0, failed=1, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=hardware; command=Arduino IDE 2.3.10 upload and serial monitor; environment=Windows/XIAO ESP32-S3/COM6/LSM6DSV/100 kHz; scope=WHO_AM_I `0x70` and safe scan found only `0x6B`; counts=passed=2, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=runtime; command=reset capture, owner motion test, and 10-minute GUI measurement; environment=Windows/SlimeVR v20.1.0/XIAO ESP32-S3/COM6/100 kHz; scope=rest calibration, Server/Preview, `DEG_0`, 0% loss, heading 41.48 to 41.47 degrees; counts=passed=14, failed=0, skipped=0, not-run=0
- C: 2026-08-03 — evidence: status=PASS; kind=compile; command=PlatformIO four firmware environments plus `test` for both XIAO environments with `--without-uploading --without-testing` and `git diff --check`; environment=Windows/PlatformIO 6.1.19, no hardware connected, no I2C clock applied; scope=re-verification after the debounce and RTC pull-up change, compilation only with 0 executed test cases; counts=passed=8, failed=0, skipped=0, not-run=4 hardware/Unity runs
- C: 2026-08-01 — evidence: status=PASS; kind=compile; command=PlatformIO four firmware builds and the XIAO test suites with `--without-uploading --without-testing`; environment=Windows/PlatformIO 6.1.19/no hardware; scope=compile-only; counts=passed=8, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=compile; command=PlatformIO builds for four firmware environments and two XIAO test suites with `--without-uploading --without-testing`; environment=Windows/PlatformIO 6.1.19; scope=compilation/static contracts only; counts=passed=8, failed=0, skipped=0, not-run=0

not-run:
- U: U2 final cell, protection, connector, charge current, ADC divider, and enclosure are unresolved.
- U: U3 400 kHz comparison, disconnected-sensor logs, and 8-hour endurance are not run.
- U: U4 the detached `0df3` worktree still holds the owner README draft saying `0x6A`. Its content was merged into `README.md` at `4d60dba` with `0x6B` and a Wi-Fi provisioning correction; the worktree copy is left untouched.
- U: U7 no serial port was enumerated on 2026-08-03, so no button, sleep, or wake behaviour has been observed on hardware.
- U: U8 the schematic has not been built. Konnect's `load_toolset` succeeds server-side but the loaded tools stay uncallable in a non-interactive session, so this needs an interactive `claude` session, and `kicad-cli` must be on PATH first (`C:\Program Files\KiCad\10.0\bin`) or ERC and exports fail.
- U: U9 the pad-row spacing of the XIAO and of the LSM6DSV module has not been measured, so no placement on the ElectroCookie grid is fixed. Calipers settle it.
- U: U10 two build decisions are open: whether a slide switch sits in the `BAT+` line, and whether the divider uses 100 kOhm (about 21 uA) or 220 kOhm (about 9.5 uA).
- U: U11 momentary-button Sleep/Wake runtime, its ten-cycle hardware run, and completed-tracker Deep Sleep current remain not run.
- U: U12 INT1 on GPIO8 is wired in the documented design only. No hardware carries the wire yet, so neither the "INT1 does not disturb I2C" check nor the "GPIO8 does not affect boot" check has been run. Wake-on-motion is unimplemented; only the pin reservation exists.
- A: Six-face acceleration calibration remains unnecessary unless later mounting tests show material error.
- C: U5 is resolved. The 2026-08-01 captures were taken over COM6, so the earlier "no serial port enumerated" blocker no longer applies to the IMU work.
- C: U6 is resolved. `codex/momentary-deep-sleep` was merged into `xiao-lsm6dsv` on 2026-08-04.

## Decisions

- C: Use 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, address `0x6B`, WHO_AM_I register `0x0F=0x70`, and 100 kHz production I2C; safe scan `0x08..0x77`; explicit alternate `0x6A`; never derive `0x6C`.
- C: Use a normally-open button from GPIO7 to GND plus external 10 kOhm pull-up to 3V3; never route battery current through it.
- C: Run `INT1` to GPIO8 in the final build even though the firmware never reads it. Retrofitting the wire into a finished assembly costs far more than running it now, and it is the only path to wake-on-motion.
- C: Hold two seconds, release to sleep, and press to wake; boot stays disarmed until the first release.
- C: Preserve the network packet format; detailed probe failures remain serial-only and map to `SENSOR_ERROR`.
- C: `xiao-lsm6dsv` is primary development; `main` only mirrors `upstream/main`. Flow is upstream/main -> main -> xiao-lsm6dsv.
- C: Owner decision of 2026-08-04 supersedes the 2026-08-03 gate. The earlier decision was to merge `codex/momentary-deep-sleep` only after the button, wake, IMU identity, Server reconnect, and ten-cycle hardware checks passed. The owner instructed the merge before those checks, so the Deep Sleep behaviour ships unproven on hardware and U7/U11 remain open.
- C: The Deep Sleep integration went merge (`b2f9afa`), revert (`2097cec`), then re-apply on 2026-08-04. Because `b2f9afa` stays reachable, `git merge codex/momentary-deep-sleep` reports "Already up to date" and cannot restore the code; only reverting the revert works. To drop the feature again, revert the re-apply commit rather than re-running the merge.
- A: Startup wait 500 ms remains provisional; INT is unused and battery measurement is unvalidated.

## Next

- Flash XIAO and run short-press, hold/release, wake-log, IMU identity, Server reconnect, and ten-cycle checks; read the count off `sleep entries` / `button wakes`; blocked-by: U7 (tracker not connected)
- Run the INT1 wire on GPIO8 and confirm I2C and boot are unchanged; blocked-by: U12
- Measure complete-tracker awake/Deep Sleep current and temperature on battery; blocked-by: U2
- Capture address NACK, transmission error, read failure, and identity mismatch logs; mismatch and short-read paths remain contract-test-only; blocked-by: none
- Build the schematic per `docs/perfboard-schematic-build-spec.md`; blocked-by: U8 (interactive session and `kicad-cli` on PATH)
- Measure both modules' pad-row spacing, then fix the ElectroCookie placement; blocked-by: U9
- Decide the `BAT+` slide switch and the divider resistor value; blocked-by: U10
- Run the optional 400 kHz comparison without changing production defaults, and the 8-hour endurance test; blocked-by: U3
- Resolve battery and enclosure requirements; blocked-by: U2
- Sync `main` from `upstream/main` (5 commits behind), then merge `main` into `xiao-lsm6dsv`; blocked-by: owner authorization

## Paths

- C: implementation: `src/power/PowerButton.*`, `src/main.cpp`, `src/sensors/SensorManager.*`, `src/sensors/softfusion/`, `lib/i2cscan/`
- C: procedure: `docs/momentary-button-deep-sleep.md`, `docs/xiao-esp32s3-lsm6dsv.md`, `docs/wiring-xiao-lsm6dsv.svg`, `docs/handoff-history.md`
- C: hardware build: `docs/perfboard-netlist.md`, `docs/perfboard-schematic-build-spec.md`, `hardware/tracker-perfboard/`
- C: configuration: `platformio.ini`, `board-defaults.json`
- C: tests: `pio-test/test_i2c_contract/test_main.cpp`, `pio-test/test_xiao_lsm6dsv_hardware/test_main.cpp`
- C: forbidden legacy probe: `test/I2C_TEST.cpp` is absent

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Recheck branch, HEAD, worktrees, remotes, serial hardware, and current tests.
3. Start the first unblocked `Next` item and record exact evidence here.
4. Keep compile, runtime, and hardware evidence separate; never promote compile success to hardware PASS.
