# KieTrackerFirst handoff

## 1. Purpose, users, and current capability

The project is a compact wireless SlimeVR tracker for long VRChat sessions,
using a Seeed Studio XIAO ESP32-S3 and an LSM6DSV module. The repository is based
on SlimeVR Tracker ESP firmware. A dedicated board target, checked LSM6DSV
startup detection, safe I2C scanning, and hardware documentation are included.

The intended user is a VRChat player who receives an assembled tracker and
installs the final battery pack. Battery mechanics and measurement circuitry are
not yet finalized.

## 2. Stack and responsibilities

- PlatformIO and Arduino-ESP32 build the firmware.
- `platformio.ini`, `board-defaults.json`, and `board-defaults.schema.json`
  define the XIAO ESP32-S3 environments and generated board configuration.
- `src/sensors/softfusion/drivers/lsm6dsv.h` contains the LSM6DSV identity and
  register configuration.
- `src/sensorinterface` performs checked I2C register reads.
- `lib/i2cscan` owns safe discovery and bus recovery.
- `lib/i2cscan/i2c_safety.h`, `include/sensor_probe.h`, and
  `include/sensor_address_resolver.h` contain pure, build-testable rules.
- `docs/xiao-esp32s3-lsm6dsv.md` is the authoritative wiring and hardware-test
  procedure for this target.

## 3. Invariants and authoritative sources

For this target, effective configuration must remain SDA GPIO5, SCL GPIO6,
address `0x6B`, `WHO_AM_I` register `0x0F`, expected value `0x70`, and production
clock 100 kHz. No scan path may transmit outside `0x08` through `0x77`.

Source priority is: physical result supplied by the project owner; ST and Seeed
official material; SlimeVR official documentation; the module sales/assembly
guide; existing code assumptions. A lower-priority source must not override the
confirmed physical result.

## 4. Unknown, failure, and empty meanings

- `ADDRESS_NACK`: the device did not acknowledge its address (`tx=2`).
- `TRANSMISSION_ERROR`: another non-zero send result.
- `READ_FAILURE`: send succeeded but the requested byte did not arrive.
- `WHO_AM_I_MISMATCH`: a complete read returned a value other than `0x70`.
- All four map to the existing network `SENSOR_ERROR`; serial output retains the
  detailed cause. No SlimeVR packet format was changed.
- An unknown pull-up value, battery design, or 400 kHz result is not represented
  as zero, absent hardware, or a passing test.

## 5. Storage, cache, and schema

No new persistent data or network cache was added. The board-default JSON schema
contains `BOARD_XIAO_ESP32S3`; `scripts/preprocessor.py` remains the authoritative
generator for the board macros.

## 6. External communication, security, and privacy

External behavior remains the upstream SlimeVR firmware behavior. No telemetry,
credentials, personal data, or new services were introduced. Wi-Fi secrets must
not be committed to `platformio.ini` or source files.

## 7. License and credits

The existing project uses its MIT/Apache-2.0 dual-license contribution policy.
No third-party source or binary asset was added by this change. Hardware and
documentation references retain their original ownership.

## 8. UX and accessibility

There is no new graphical UI. Diagnostic serial messages use named states and
numeric I2C evidence rather than color-only output.

## 9. Setup, development, test, build, and release

Use VS Code with PlatformIO:

```text
PlatformIO: Build -> BOARD_XIAO_ESP32S3
PlatformIO: Build -> BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC
PlatformIO: Test  -> BOARD_XIAO_ESP32S3 (contract + hardware suites)
```

The `test_i2c_contract` suite contains 15 compile-time assertions and is checked
when the XIAO test firmware is built. The `test_xiao_lsm6dsv_hardware` suite requires
the tracker. Firmware upload and hardware tests must be initiated through
PlatformIO in VS Code. Do not claim the power-cycle, 400 kHz, or eight-hour tests
from compilation alone.

Integration evidence from the VS Code PlatformIO Core used by this workspace:

- `BOARD_XIAO_ESP32S3` firmware build: PASS; RAM 45,168/327,680 bytes, flash
  1,148,237/3,342,336 bytes.
- `BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC` firmware build: PASS; RAM
  45,168/327,680 bytes, flash 1,148,345/3,342,336 bytes.
- Production test-firmware compile: PASS; 2 suites collected and built.
- 400 kHz diagnostic test-firmware compile: PASS; 2 suites collected and built.
- Shared-code regression builds: `BOARD_WEMOSD1MINI` PASS and
  `BOARD_XIAO_ESP32C3` PASS.
- The contract suite compiled all 15 `static_assert` cases in each environment.
- Runtime Unity cases: not run (`--without-uploading --without-testing`), because
  this integration did not upload firmware to the tracker.
- Board generation evidence printed by both builds: SDA 5, SCL 6,
  `IMU_LSM6DSV`, address 107, INT 255, and `BAT_INTERNAL`.
- `git diff --check`: PASS. Standalone `clang-format` was unavailable in this
  Windows environment, so no formatter command was run.

Release requires all automated checks, the 100 kHz hardware matrix, enclosure
and battery safety decisions, and a clean targeted diff. Push, merge, publishing,
and deployment require explicit authorization.

## 10. Git and roles

- Working branch at implementation start: `xiao-lsm6dsv`.
- Base and starting tip: `5e680f7` (`origin/main` at inspection time).
- DEV implements, documents, tests, and commits related changes.
- MERGE performs integration tests and only an explicitly authorized push.

The untracked `test/I2C_TEST.cpp` existed before this work and remains untouched.
It scans `0x01` through `0x7E` and is unsafe for this module. Do not execute,
stage, overwrite, or treat it as the maintained probe. The maintained suite is
`pio-test/test_xiao_lsm6dsv_hardware`.

Other detached worktrees contain user-owned state. Their uncommitted changes are
not part of this integration.

An initial native-test attempt was abandoned because this Windows installation
has no host `gcc/g++`, and PlatformIO treated legacy files directly under `test/`
as shared test sources. The maintained suites were moved to the dedicated
`pio-test/` directory and the deterministic checks were converted to embedded
C++20 compile-time assertions. No host compiler installation is required.

## 11. Collision-prone areas

Board enumeration, board-default JSON, `platformio.ini`, the generic SoftFusion
startup path, and the shared I2C scanner affect multiple hardware targets. Keep
non-XIAO startup clocks unchanged and run regression builds after modifying them.

## 12. Test truthfulness rules

Skipped hardware checks are not passes. Record the exact PlatformIO environment,
test suite, pass/fail counts, skips, connected hardware, and clock. A successful
build proves compilation only. A safe scanner test must prove that `0x7E` was
never addressed, not merely omitted from printed results.

## 13. Release conditions

Before release: pass native tests and production firmware build; complete 10
cold power cycles at 100 kHz; resolve battery pack, protection, connector, ADC,
and enclosure decisions; complete an eight-hour SlimeVR run; and review secrets,
generated artifacts, licenses, and the final Git diff. The 400 kHz result is
informational and does not gate a 100 kHz release.

## 14. Measured technical knowledge

```text
--- WHO_AM_I direct read ---
0x6B -> WHO_AM_I = 0x70 [OK: LSM6DSV]
--- safe I2C scan ---
found: 0x6B
```

This physical result confirms the module, 3.3 V supply, GPIO5/GPIO6 wiring,
0x6B address, and WHO_AM_I read at 100 kHz. A broad `0x01` through `0x7E` scan
also observed `0x7E`, after which the identity read failed. Treat `0x7E` as an
I3C reserved/broadcast address, not another I2C peripheral.

## 15. Rejected approaches

- Defaulting LSM6DSV to `0x6A`: conflicts with the physical module configured
  SA0 High at `0x6B`.
- Deriving the alternate address as `0x6B + 1`: produces invalid `0x6C`; the
  explicit alternate is `0x6A`.
- Scanning `0x01` through `0x7E` or `0x7F`: can address I3C-reserved `0x7E` and
  has already disturbed the following read.
- Shipping at 400 kHz now: that rate is not yet confirmed on the assembled unit.

## 16. C/A/U, exclusions, and next tasks

`C`: 3.3 V; SDA GPIO5; SCL GPIO6; SA0 High; address `0x6B`; WHO_AM_I
`0x0F == 0x70`; 100 kHz physical success; unsafe scan failure history.

`A`: 500 ms power-up delay; production remains at 100 kHz; INT is unused;
mounting rotation temporarily uses `DEG_0`; battery monitoring is disabled for
ESP32 by `BAT_INTERNAL` until its circuit is confirmed; detailed errors remain
serial-only.

`U`: 400 kHz product suitability; module pull-up values; final INT wiring and
rotation; battery cell, protection, connector, charge-current compatibility,
ADC divider, enclosure, and final endurance result.

Out of scope for this integration: battery purchase, enclosure CAD, production
release, push, deployment, and modification of the old untracked probe.

Next tasks are the 100/400 kHz physical matrix, 10 cold cycles, disconnected
sensor error capture, eight-hour SlimeVR run, and battery/enclosure decisions.

## 17. BATON

```text
ブランチ: xiao-lsm6dsv
先端: HEAD（このHANDOFFを含むコミット。実値は `git rev-parse --short HEAD` で確認）
分岐元: origin/main 5e680f7
概要: XIAO ESP32-S3/LSM6DSVの0x6B・100kHz起動、安全走査、診断、文書を統合
テスト: firmware build 2/2 PASS、test-firmware compile 4/4 suites PASS、15件の契約assertを両環境で評価、実機Unity/power-cycle/8時間は未実施
注意点: test/I2C_TEST.cppは所有者不明の未追跡・危険な旧スキャナのため未変更。pushなし
```
