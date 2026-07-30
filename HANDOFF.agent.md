# Agent handoff v1

updated: 2026-07-30
repo: Kie610/KieTrackerFirst
primary_branch: xiao-lsm6dsv
fork_source: upstream = SlimeVR/SlimeVR-Tracker-ESP, mirrored by `main` only
work_branch: claude/dev-handoff-278b77 (worktree `.claude/worktrees/dev-handoff-278b77`)
upstream: none; earlier work branch `codex/xiao-lsm6dsv-handoff`@a08c0ac matches `origin/codex/xiao-lsm6dsv-handoff`
base: origin/xiao-lsm6dsv@84f6bb3
goal: XIAO ESP32-S3 + LSM6DSV small wireless SlimeVR tracker

## State

complete:
- C: XIAO defaults generate `IMU_LSM6DSV` at `0x6B`/`DEG_0`, GPIO5/6/4/2/1, `BAT_INTERNAL`, and 180/100/220; startup validation, safe I2C handling, contracts, and wiring procedure are implemented.
- C: No Wi-Fi credentials are tracked; runtime provisioning is required.
- C: Production logs the XIAO/I2C identity and successful WHO_AM_I probe after a bounded USB-CDC wait; the fallback scanner starts/restores at 100 kHz and never reverses XIAO GPIO5/6.
- C: Milestones M3/M4 are achieved: Server/Preview work, `DEG_0` matches the photographed mounting, and stationary drift is good.
- C: Integration is complete without a merge commit: on 2026-07-30 every codex and claude ref (`codex/xiao-lsm6dsv-handoff`@a08c0ac, review branches at cbd5274 and 5e680f7, `main`@5e680f7) was confirmed an ancestor of `xiao-lsm6dsv`@a08c0ac, and `git log --all --not xiao-lsm6dsv` was empty.
- C: `origin/codex/xiao-lsm6dsv-handoff` was fast-forwarded 7591479..a08c0ac, so the work branch and its upstream now match. `main` stays at 5e680f7 as the fork-source mirror and is intentionally not advanced.
- C: The `claude/handoff-review-*` worktree and both branches were deleted after confirming containment and that their only ignored file matched the main worktree; the three codex worktrees are retained.

verified:
- C: 2026-07-30 — evidence: status=PASS; kind=hardware; command=Arduino IDE 2.3.10 upload and serial monitor; environment=Windows, XIAO ESP32-S3 on direct USB COM6, corrected LSM6DSV wiring, I2C 100 kHz; scope=direct `0x6B` WHO_AM_I read returned `0x70` and safe `0x08..0x77` scan found only `0x6B`; counts=passed=2, failed=0, skipped=0, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=build; command=all four required firmware environments and both XIAO test suites with `--without-uploading --without-testing`; environment=Windows/PlatformIO 6.1.19; scope=compilation only; counts=passed=8, failed=0, skipped=0, not-run=4 hardware/Unity runs
- C: 2026-07-30 — evidence: status=PASS; kind=hardware/runtime; command=reset capture, owner motion test, and 10-minute GUI measurement; environment=Windows, SlimeVR v20.1.0, XIAO ESP32-S3 + LSM6DSV, COM6, 100 kHz; scope=rest calibration at 28.3--29.1 C, Server/Preview, `DEG_0`, 0% loss, heading 41.48 to 41.47 degrees; counts=passed=14, failed=0, skipped=1 six-face calibration, not-run=0
- C: 2026-07-30 — evidence: status=PASS; kind=build; command=the four required firmware environments and both XIAO test suites with `--without-uploading --without-testing`, plus `git diff --check`; environment=Windows/PlatformIO, no hardware connected, no I2C clock applied; scope=re-verification at `xiao-lsm6dsv`@a08c0ac before the integration check, compilation only with 0 executed test cases; counts=passed=8, failed=0, skipped=0, not-run=4 hardware/Unity runs

- C: 2026-07-30 — evidence: status=PASS; kind=build; command=`run -e BOARD_XIAO_ESP32S3`, `run -e BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC`, `run -e BOARD_WEMOSD1MINI -e BOARD_XIAO_ESP32C3`, `test -e BOARD_XIAO_ESP32S3 -e BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC --without-uploading --without-testing`, `git diff --check`; environment=Windows/PlatformIO, worktree `.claude/worktrees/dev-handoff-278b77`@84f6bb3, no hardware connected, no I2C clock applied; scope=takeover re-verification at `xiao-lsm6dsv`@84f6bb3 (only docs commits 1153ef1/84f6bb3 above the previously verified a08c0ac), compilation only, 0 executed test cases, `git diff --check` clean; counts=passed=8, failed=0, skipped=0, not-run=4 hardware/Unity runs

not-run:
- U: U5 no serial port is enumerated on this PC (`[System.IO.Ports.SerialPort]::getportnames()` returned empty on 2026-07-30), so every remaining `Next` hardware item is waiting on the tracker being connected to a data-capable USB port, not on code work.
- U: U2 final cell, protection, connector, charge current, ADC divider, and enclosure requirements remain unresolved.
- U: U3 400 kHz hardware comparison, ten power-removal cycles, detailed disconnected-sensor logs, and 8-hour endurance remain not run.
- U: U4 the `0df3` codex worktree holds an uncommitted `README.md` draft at 5e680f7 that states address `0x6A`. It was deliberately left untouched and must not be merged: `README.md` on `xiao-lsm6dsv` already documents the confirmed `0x6B`/100 kHz configuration. Merging requires correcting the draft to `0x6B` first.

## Decisions

- C: Use 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, address `0x6B`, `WHO_AM_I` register `0x0F == 0x70`, and scan only `0x08..0x77`.
- C: Keep production I2C at 100 kHz; never transmit to reserved address `0x7E`; use `0x6A` as the explicit alternate address.
- C: Use `DEG_0` when the component side faces away, USB points toward the feet, and the opposite edge points toward the head.
- C: Owner decision of 2026-07-30 — `xiao-lsm6dsv` is the primary development branch and `main` only mirrors the fork source `upstream` so its updates can be pulled in. No merge of `xiao-lsm6dsv` into `main` is planned; changes flow one way, `upstream/main` to `main` to `xiao-lsm6dsv`. See the branch policy in `AGENTS.md`.
- C: Preserve the network packet format; map probe failures to `SENSOR_ERROR` and log the detailed cause over serial.
- A: Startup wait 500 ms remains provisional; INT assignments are unused and the battery circuit remains unvalidated.

## Next

- Run ten complete power-removal cycles at 100 kHz and record every address/identity result; blocked-by: U5
- Capture address NACK, transmission error, read failure, and identity mismatch logs (per `docs/xiao-esp32s3-lsm6dsv.md` the mismatch and short-read paths stay contract-test-only; capture NACK by disconnecting the module); blocked-by: U5
- Run the optional 400 kHz comparison without promoting it to production; blocked-by: U3
- Run the 8-hour endurance test; blocked-by: U3
- Resolve battery and enclosure requirements before release; blocked-by: U2
- Decide whether to correct the `0df3` README draft to `0x6B` or discard the worktree; blocked-by: U4

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
