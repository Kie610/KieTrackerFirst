# Agent handoff v1

updated: 2026-08-09
repo: Kie610/KieTrackerFirst
work_branch: xiao-lsm6dsv
upstream: origin/xiao-lsm6dsv (local ahead, unpushed)
base: main@5e680f7 (mirror of SlimeVR/SlimeVR-Tracker-ESP upstream)
goal: XIAO ESP32-S3 + LSM6DSV small wireless SlimeVR tracker

## State

complete:
- LSM6DSV at `0x6B`, startup diagnostics (NACK/short-read/WHO_AM_I classes), safe scan `0x08..0x77`, production I2C 100 kHz.
- GPIO7 momentary button: 2 s hold to deep sleep, EXT0 wake, NVS cycle counters; 10/10 hardware cycles passed 2026-08-06.
- Battery monitor `BAT_EXTERNAL` 100k/100k + C1, 15-sample median; reading uncalibrated by owner decision.
- Wi-Fi `PS_NONE` latency fix (95 ms to 2.6 ms mean); 8 h endurance run passed at 100 kHz on 2026-08-05.
- All six 2026-08-07 review fixes T1-T6 committed and verified; all four boards run this build.
- Perfboard KiCad schematic verified (ERC 0, 8 nets); enclosure designed, test print assembled.

verified-2026-08-07:
- hardware: `platformio run -e BOARD_XIAO_ESP32S3 -t upload` to COM8/COM10/COM6 with serial captures; passed=3 flashes + 2 IMU calibrations + 3 Wi-Fi associations, failed=0, skipped=0.
- runtime: owner 3-axis acceleration check on the T1-T6 build; passed=3 axes, failed=0 (T3 changes nothing observable). Sleep/wake works; T6 latency gain unquantified, treat as closed.
- compile: 4 builds + 4 test-suite compilations + `git diff --check` + config diff; passed all, failed=0; 0 Unity cases executed.
- Full dated evidence 2026-07-30..2026-08-07, resolved U items, and method notes are preserved verbatim in `docs/handoff-history.md` (2026-08-08 migration section).

verified-2026-08-09:
- runtime (owner report, informal): all trackers including the two completed ones work well in actual VR use on the T1-T6 build; no counts recorded. Working use implies Wi-Fi provisioning of the two completed trackers is done.
- hardware: battery run-to-cutoff on the test unit (full 1000 mAh 803040 cell, T1-T6 PS_NONE build, detached 30 s ping monitor, VR use during part of the run). USB unplugged 2026-08-08 22:08:00, last reply 07:07:25: **8 h 59 min**, passed=1073/1073 pings before cutoff, failed=0 mid-run, end detected by 10 consecutive misses. Matches the LDO-side prediction (~9 h), so treat the 3.3 V regulator as current-conserving when estimating from 5 V-side measurements. U25 is resolved.

not-run:
- Deep sleep current (needs microamp meter).
- `ADDRESS_NACK` capture with module detached; WHO_AM_I-mismatch and short-read paths stay contract-test-only.

## Decisions

C:
- Sensor rotation is `DEG_90` (measured 2026-08-05). Never derive the sign of `DEG_X` from the macro; test candidates on hardware, and verify mounting by tilting pitch/roll in SlimeVR Preview, not by heading stability.
- `IMU_INT1` is D10/GPIO9, unread by firmware. Battery divider will NOT be calibrated (owner 2026-08-06; do not reopen).
- Upstream sync deferred until upstream lands a change worth taking; board id 27 collision awaits (details in history).
- Serial capture: DTR on / RTS off; never hold or reopen the port across a sleep transition, count boots not POWERON banners.
A:
- 500 ms startup wait remains provisional.
U:
- Pre-fix acceleration readings (determinant -1) never reconciled with code analysis; re-measure all three axes before touching accel code again (U28).
- Deep sleep current unmeasured, below USB-meter resolution (U26).
- Battery protection circuit, connector, and enclosure wear-test open (U2).
- ElectroCookie column assignment unplanned; only three columns fully free (U14).

## Next

1. Capture `ADDRESS_NACK` with the module detached.
2. Plan the ElectroCookie column assignment into `docs/perfboard-netlist.md` (U14).
blocked-by:
- U26 needs a microamp meter; items 1-2: none.

## Paths

- implementation: `src/power/PowerButton.*`, `src/main.cpp`, `src/sensors/`, `lib/i2cscan/`
- procedures: `docs/xiao-esp32s3-lsm6dsv.md`, `docs/momentary-button-deep-sleep.md`, `docs/review-2026-08-07.md`
- hardware build: `docs/perfboard-netlist.md`, `hardware/tracker-perfboard/`
- config and tests: `platformio.ini`, `board-defaults.json`, `pio-test/`

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Verify live Git, serial, and external state; live evidence overrides recorded metadata.
3. Read only paths needed for the first unblocked action; pull history from `docs/handoff-history.md` on demand.
4. Run the smallest relevant baseline checks.
5. Execute the highest-priority unblocked action.
6. Update evidence; never convert `not-run` to PASS without execution.
