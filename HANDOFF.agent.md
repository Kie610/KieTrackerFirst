# Agent handoff v1

updated: 2026-08-01
repo: Kie610/KieTrackerFirst
work_branch: codex/xiao-lsm6dsv-handoff
upstream: origin/codex/xiao-lsm6dsv-handoff
base: origin/xiao-lsm6dsv@cbd5274
goal: XIAO ESP32-S3 + LSM6DSV small wireless SlimeVR tracker

## State

complete:
- C: XIAO defaults generate `IMU_LSM6DSV` at `0x6B`/`DEG_0`, GPIO5/6/4/2/1, `BAT_INTERNAL`, and 180/100/220; startup validation, safe I2C handling, and contracts are implemented.
- C: No Wi-Fi credentials are tracked; runtime provisioning is required.
- C: Production logs XIAO/I2C identity and WHO_AM_I after USB-CDC wait; fallback scanning restores 100 kHz and never reverses GPIO5/6.
- C: M3/M4 achieved: Server/Preview work, `DEG_0` matches photographed mounting, and drift is good.
- C: Replacement IMU passed ten 100 kHz power-removal cycles; prior shorted IMU is isolated.

verified:
- C: 2026-08-01 — evidence: status=PASS; kind=hardware; command=serial reset after power cycles 1--10/10; environment=Windows/XIAO/COM6/100 kHz; scope=replacement IMU `0x6B` WHO_AM_I=`0x70`, gyro/rest calibration; counts=passed=20, failed=0, skipped=0, not-run=0
- C: 2026-08-01 — evidence: status=PASS; kind=compile; command=PlatformIO four builds + XIAO tests with --without-uploading --without-testing; environment=Windows/PlatformIO 6.1.19; scope=firmware and four test targets compile-only; counts=passed=8, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=runtime; command=reset+motion+10-min GUI; environment=Windows/SlimeVR20.1.0/XIAO/COM6/100 kHz; scope=calibration 28.3--29.1 C, Preview, `DEG_0`, 0% loss, heading 41.48->41.47; counts=passed=14, failed=0, skipped=0, not-run=0
- C: 2026-08-01 — evidence: status=FAIL; kind=hardware; command=serial capture after USB reconnect; environment=Windows/XIAO/COM6/100 kHz; scope=prior IMU held SCL low, `0x6B` READ_FAILURE tx=0 rx=0/1; counts=passed=0, failed=1, skipped=0, not-run=0

not-run:
- U: U2 final cell, protection, connector, charge current, ADC divider, and enclosure requirements remain unresolved.
- U: U3 400 kHz hardware comparison, detailed disconnected-sensor logs, and 8-hour endurance remain not run.
- A: Six-face acceleration calibration was not run because flat Preview pitch/roll error was below 0.4 degrees and the owner confirmed motion alignment.

## Decisions

- C: Use 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, address `0x6B`, `WHO_AM_I` register `0x0F == 0x70`, and scan only `0x08..0x77`.
- C: Keep production I2C at 100 kHz; never transmit to reserved address `0x7E`; use `0x6A` as the explicit alternate address.
- C: Use `DEG_0` when the component side faces away, USB points toward the feet, and the opposite edge points toward the head.
- C: Preserve the network packet format; map probe failures to `SENSOR_ERROR` and log the detailed cause over serial.
- A: Startup wait 500 ms remains provisional; INT assignments are unused and the battery circuit remains unvalidated.

## Next

- Capture address NACK, transmission error, read failure, and identity mismatch logs; blocked-by: none
- Run the optional 400 kHz comparison without promoting it to production; blocked-by: U3
- Run the 8-hour endurance test; blocked-by: U3
- Resolve battery and enclosure requirements before release; blocked-by: U2

## Paths

- C: procedure/history: `docs/xiao-esp32s3-lsm6dsv.md`, `docs/handoff-history.md`
- C: configuration/driver: `platformio.ini`, `board-defaults.json`, `src/sensors/softfusion/`, `lib/i2cscan/`
- C: tests: `pio-test/test_i2c_contract/test_main.cpp`, `pio-test/test_xiao_lsm6dsv_hardware/test_main.cpp`
- C: forbidden legacy probe: `test/I2C_TEST.cpp`

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Recheck Git branch, HEAD, worktree, upstream, connected serial hardware, and current tests; prefer live evidence.
3. Run the smallest relevant checks, then start the first unblocked `Next` item.
4. Update this file with exact evidence; never promote compile success to runtime or hardware PASS.
