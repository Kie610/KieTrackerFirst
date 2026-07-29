# Agent handoff v1

updated: 2026-07-29
repo: Kie610/KieTrackerFirst
work_branch: codex/xiao-lsm6dsv-handoff
upstream: origin/xiao-lsm6dsv@cbd5274
base: origin/main@5e680f7
goal: XIAO ESP32-S3 + LSM6DSV small wireless SlimeVR tracker

## State

complete:
- XIAO board definitions and generated defaults
- LSM6DSV startup validation and diagnostic failures
- safe I2C scan and bus recovery
- compile-time I2C contract checks
- wiring and hardware-test procedure
- token-efficient Codex/Claude instruction and handoff layers
- reversible AX1 prompts for bootstrap and skill-repository transfer

verified-2026-07-29:
- `BOARD_XIAO_ESP32S3` firmware compile PASS; RAM 45168/327680; Flash 1148253/3342336
- `BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC` compile PASS; RAM 45168/327680; Flash 1148361/3342336
- regression compile PASS: `BOARD_WEMOSD1MINI`, `BOARD_XIAO_ESP32C3`
- test-firmware compile PASS: 2 environments x 2 suites; runtime cases 0 because upload/testing disabled
- contract `static_assert` count: 15
- `git diff --check` PASS
- serial devices detected: none
- AX1 payloads verified by byte-exact round trip, deterministic re-encode, and corruption rejection
- historical handoff preserved with Git blob `5791de8427f943984ebb6cbcd029383dfe4c42e6`

hardware-confirmed:
- 3.3 V; SDA GPIO5; SCL GPIO6; SA0 High; address `0x6B`
- 100 kHz direct read: register `0x0F` = `0x70`
- safe scan found `0x6B`
- unsafe scan touched `0x7E` and disrupted the following read; treat `0x7E` as reserved

not-run:
- Unity runtime tests
- 100/400 kHz hardware comparison
- full power removal/restart x10
- disconnected-sensor log capture
- 8-hour SlimeVR Server endurance test

## Decisions

C:
- 3.3 V; GPIO5/GPIO6; SA0 High; `0x6B`; `0x0F == 0x70`; 100 kHz hardware success
- scan range `0x08..0x77`; never transmit to `0x7E`

A:
- startup wait 500 ms; production 100 kHz; INT unused; rotation `DEG_0`
- battery measurement disabled-equivalent with `BAT_INTERNAL`
- detailed failure cause is serial-only; network remains `SENSOR_ERROR`

U:
- production suitability of 400 kHz; module pull-up resistance; final INT and rotation
- cell, protection, connector, charge-current compatibility, ADC divider, enclosure
- final endurance result

## Next

1. Connect the tracker by USB and identify the serial port.
2. Run the 100/400 kHz hardware comparison without promoting 400 kHz to production.
3. Run ten complete 100 kHz power-removal/restart cycles.
4. Capture logs for address NACK, transmission error, read failure, and identity mismatch when feasible.
5. Complete the 8-hour SlimeVR Server test.
6. Resolve battery and enclosure requirements before release.

blocked-by:
- connected tracker hardware
- final battery and enclosure requirements

## Paths

- procedure: `docs/xiao-esp32s3-lsm6dsv.md`
- history/rationale: `docs/handoff-history.md`
- environments: `platformio.ini`, `board-defaults.json`, `board-defaults.schema.json`
- sensor startup: `src/sensors/softfusion/softfusionsensor.h`
- driver identity: `src/sensors/softfusion/drivers/lsm6dsv.h`
- safe scan: `lib/i2cscan/`
- contract tests: `pio-test/test_i2c_contract/test_main.cpp`
- hardware tests: `pio-test/test_xiao_lsm6dsv_hardware/test_main.cpp`
- forbidden legacy probe: `test/I2C_TEST.cpp`
- transfer payloads/prompts: `agent-prompts/`
- reversible codec: `scripts/agent_prompt_codec.py`

## Resume protocol

1. Read `AGENTS.md` and this file.
2. Run `git status --short --branch` and `git log -1 --oneline --decorate`; prefer live Git state over recorded branch metadata.
3. Read only the paths needed for the first executable `Next` item.
4. Run non-hardware baseline checks before changing shared code.
5. Start the highest-priority unblocked item; otherwise report the exact external input required.
6. Update this file with evidence; never convert `not-run` into PASS without execution.
