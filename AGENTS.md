# Repository agent instructions

## Scope and sources

- Apply these rules repository-wide.
- For takeover work, read `HANDOFF.agent.md` before inspecting other files.
- Treat `docs/xiao-esp32s3-lsm6dsv.md` as the authoritative wiring and hardware-test procedure.
- Resolve factual conflicts in this order: owner-provided hardware results; ST/Seeed official material; SlimeVR official documentation; module sales/assembly material; assumptions in code.
- Read `docs/handoff-history.md` only when the compact handoff does not contain needed rationale.
- When asked to fix findings from the 2026-08-07 adversarial review, read `docs/review-2026-08-07.md` first and follow the self-contained delegation prompt (T1-T6) for the requested task; no pasted prompt is needed.

## Branch policy

- `xiao-lsm6dsv` is the primary development branch. Base all work on it and treat it as the source of truth for this fork.
- `main` exists only to mirror the fork source `upstream` (`https://github.com/SlimeVR/SlimeVR-Tracker-ESP.git`) so its updates can be pulled in. Keep it aligned with the fork source and free of this fork's own work.
- There is no planned merge of `xiao-lsm6dsv` into `main`. Do not propose, prepare, or perform one; the fact that it is a clean fast-forward is not a reason to do it.
- Flow upstream changes in one direction only: `upstream/main` into `main`, then `main` into `xiao-lsm6dsv`.
- Ask the owner before creating any long-lived branch, and delete review or takeover branches once their commits are contained in `xiao-lsm6dsv`.

## XIAO ESP32-S3 and LSM6DSV invariants

- Use 3.3 V, SDA GPIO5, SCL GPIO6, SA0 High, and I2C address `0x6B`.
- Read `WHO_AM_I` register `0x0F`; require value `0x70`.
- Keep production I2C at 100 kHz unless owner-provided hardware results approve a change.
- Scan only `0x08` through `0x77`; never transmit to reserved address `0x7E`.
- Use `0x6A` as the explicit alternate address; never derive `0x6C` from `0x6B + 1`.
- Preserve the existing network packet format. Map detailed probe failures to `SENSOR_ERROR` and log the specific cause over serial.

## Truthfulness and safety

- Distinguish confirmed (`C`), assumed (`A`), and unresolved (`U`) facts.
- Never report firmware compilation as a passed hardware, Unity runtime, power-cycle, 400 kHz, or endurance test.
- Record the environment, suite, connected hardware, I2C clock, pass/fail count, and skips for hardware tests.
- Do not execute, edit, stage, or use untracked `test/I2C_TEST.cpp`; it scans unsafe addresses. Maintain `pio-test/test_xiao_lsm6dsv_hardware` instead.
- Do not commit Wi-Fi credentials, generated build artifacts, or secrets.
- Do not push, merge, release, or deploy without explicit user authorization.
- Preserve unrelated user changes and states in other worktrees.

## Build and verification

On this Windows setup, PlatformIO is normally at:

```powershell
$pioExe = 'C:\Users\Kie\.platformio\penv\Scripts\platformio.exe'
```

Use the smallest relevant checks first; for shared I2C or SoftFusion changes, run all of these:

```powershell
& $pioExe run -e BOARD_XIAO_ESP32S3
& $pioExe run -e BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC
& $pioExe run -e BOARD_WEMOSD1MINI -e BOARD_XIAO_ESP32C3
& $pioExe test -e BOARD_XIAO_ESP32S3 -e BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC --without-uploading --without-testing
git diff --check
```

The `--without-uploading --without-testing` command proves compilation only. Flashing and Unity execution require connected hardware and separate reporting.

## Handoff maintenance

- Keep durable rules here, current state in `HANDOFF.agent.md`, detailed procedures in `docs/`, and history in `docs/handoff-history.md`.
- Update `HANDOFF.agent.md` whenever branch state, verification, blockers, decisions, or next actions change materially.
- Keep the compact handoff factual, path-oriented, and free of duplicated background prose.
