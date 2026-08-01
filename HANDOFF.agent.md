# Agent handoff v1

updated: 2026-08-01
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
- C: `xiao-lsm6dsv` contains the codex implementation and the claude handoff update through `04fbb29`; `main` remains the upstream mirror.
- C: No Wi-Fi credentials are tracked; runtime provisioning is required.

verified:
- C: 2026-07-30 — evidence: status=PASS; kind=hardware; command=Arduino IDE 2.3.10 upload and serial monitor; environment=Windows/XIAO ESP32-S3/COM6/LSM6DSV/100 kHz; scope=WHO_AM_I `0x70` and safe scan found only `0x6B`; counts=passed=2, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=compile; command=PlatformIO builds for four firmware environments and two XIAO test suites with `--without-uploading --without-testing`; environment=Windows/PlatformIO 6.1.19; scope=compilation/static contracts only; counts=passed=8, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=runtime; command=reset capture, owner motion test, and 10-minute GUI measurement; environment=Windows/SlimeVR v20.1.0/XIAO ESP32-S3/COM6/100 kHz; scope=rest calibration, Server/Preview, `DEG_0`, 0% loss, heading 41.48 to 41.47 degrees; counts=passed=14, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=compile; command=PlatformIO takeover re-verification at `84f6bb3`; environment=Windows/PlatformIO/no hardware; scope=four builds, two test compilations, and `git diff --check`; counts=passed=8, failed=0, skipped=0, not-run=0

not-run:
- U: U5 no serial port was enumerated on 2026-07-30; hardware work awaits data-capable USB.
- U: U2 final cell, protection, connector, charge current, ADC divider, and enclosure are unresolved.
- U: U3 400 kHz comparison, ten power cycles, disconnected-sensor logs, and 8-hour endurance are not run.
- U: U4 detached `0df3` worktree retains an owner README draft saying `0x6A`; leave it untouched.

## Decisions

- C: Use 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, address `0x6B`, WHO_AM_I register `0x0F=0x70`, and 100 kHz production I2C.
- C: Use explicit alternate `0x6A`; never derive `0x6C`; preserve packets and log probe errors.
- C: `xiao-lsm6dsv` is primary development; `main` only mirrors `upstream/main`. Flow is upstream/main -> main -> xiao-lsm6dsv.
- A: Startup wait 500 ms remains provisional; INT is unused and battery measurement is unvalidated.

## Next

- Connect a data-capable tracker USB port and run ten 100 kHz power-removal cycles; blocked-by: U5
- Capture address NACK and transmission diagnostics; mismatch and short-read paths are contract-test-only; blocked-by: U5
- Run the optional 400 kHz comparison without changing production defaults; blocked-by: U3
- Run the 8-hour endurance test; blocked-by: U3
- Resolve battery and enclosure requirements; blocked-by: U2

## Paths

- C: `docs/xiao-esp32s3-lsm6dsv.md`, `docs/handoff-history.md`
- C: `platformio.ini`, `board-defaults.json`, `src/sensors/softfusion/`, `lib/i2cscan/`
- C: `pio-test/test_i2c_contract/test_main.cpp`, `pio-test/test_xiao_lsm6dsv_hardware/test_main.cpp`
- C: forbidden legacy path `test/I2C_TEST.cpp` is absent

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Recheck branch, HEAD, worktrees, remotes, serial hardware, and current tests.
3. Start the first unblocked `Next` item and record exact evidence here.
4. Keep compile, runtime, and hardware evidence separate; never promote compile success to hardware PASS.
