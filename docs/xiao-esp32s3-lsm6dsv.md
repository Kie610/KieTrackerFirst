# XIAO ESP32-S3 + LSM6DSV hardware setup

This target combines a Seeed Studio XIAO ESP32-S3 with the LSM6DSV module sold
at [BOOTH](https://shironekya.booth.pm/items/5606882). The general firmware and
server procedures are in the [SlimeVR documentation](https://docs.slimevr.dev/).

## Confirmed wiring

| Signal | XIAO ESP32-S3 | Requirement |
| --- | --- | --- |
| VCC | 3V3 | Supply the module with 3.3 V only |
| GND | GND | Common ground |
| SDA | D4 / GPIO5 | Pull up to 3.3 V |
| SCL | D5 / GPIO6 | Pull up to 3.3 V |
| IMU INT | D3 / GPIO4 | Defined for this target; not connected or used by LSM6DSV SoftFusion yet |
| AUX IMU INT | D1 / GPIO2 | Reserved; no auxiliary IMU is installed |
| Battery ADC | D0 / GPIO1 | Reserved for the later battery build; unused in USB-powered mode |
| SDO / SA0 | High | Selects I2C address `0x6B` |
| CS / CSB | High | Required for I2C operation |

Do not use 5 V logic on SDA, SCL, CS/CSB, or SDO/SA0. The electrical design
must provide SDA/SCL pull-ups to 3.3 V. Whether the purchased module already
contains suitable pull-ups, and their values, has not yet been confirmed from a
schematic; do not rely on that assumption in a production circuit.

The configured interrupt pins are not physically connected in the current
hardware, and the LSM6DSV SoftFusion path does not use them. Battery-voltage
measurement is still undecided.

## Firmware defaults

`board-defaults.json` is the source for the generated XIAO build flags; do not
replace the dedicated target with the older generic `BOARD_CUSTOM` path.

- board: `BOARD_XIAO_ESP32S3`
- primary IMU: `IMU_LSM6DSV` at `0x6B`, confirmed rotation `DEG_0`
- common pins: SDA GPIO5, SCL GPIO6, IMU INT GPIO4, AUX INT GPIO2, battery ADC GPIO1
- USB-powered monitoring: `BAT_INTERNAL`, shield resistance 180, R1 100, R2 220

`BAT_INTERNAL` prevents external battery-ADC sampling in the current USB-powered
build. GPIO1 and the resistor values are reserved for the later battery design;
they do not constitute a validated battery circuit.

## Confirmed identity

- I2C address: `0x6B`
- `WHO_AM_I` register: `0x0F`
- expected value: `0x70`
- confirmed clock: 100 kHz

Observed on the physical module:

```text
--- WHO_AM_I direct read ---
0x6B -> WHO_AM_I = 0x70 [OK: LSM6DSV]
--- safe I2C scan ---
found: 0x6B
```

This confirms the module, 3.3 V supply, GPIO5/GPIO6 wiring, address `0x6B`, and
the direct identity read for the tested unit.

## Confirmed mounting and calibration

For the tested mounting, the component side faces away from the body, the XIAO
USB connector points toward the feet, and the opposite edge points toward the
head. With this orientation, `DEG_0` made both forward/backward and left/right
Preview motion agree with the physical tracker.

On 2026-07-30, a stationary USB reset at 100 kHz completed SoftFusion rest
calibration at approximately 28.3--29.1 C. The flat Preview pitch/roll error
was below 0.4 degrees, so the optional six-face accelerometer calibration was
skipped. During a subsequent stationary 10-minute measurement,
the horizontal heading changed from 41.48 to 41.47 degrees (0.01 degrees per
10 minutes; maximum displayed component change 0.02 degrees), rated good.

## Safe startup and scanning

The firmware starts this target at 100 kHz, waits 500 ms after powering the
sensor, and reads `WHO_AM_I` before writing any LSM6DSV configuration register.
Initialization continues only when the read succeeds and returns `0x70`.

Diagnostic reads log the I2C address, register, `endTransmission()` code, and
the number of bytes requested and received. Failures are separated into:

- `ADDRESS_NACK`: `endTransmission()` returned 2.
- `TRANSMISSION_ERROR`: another non-zero transmission result.
- `READ_FAILURE`: transmission succeeded but fewer bytes than requested arrived.
- `WHO_AM_I_MISMATCH`: one byte arrived but was not `0x70`.

LSM6DSV also supports I3C. Address `0x7E` is reserved for I3C broadcast-related
operation and must not be treated as a second I2C device. A previous scan across
`0x01` through `0x7E` observed both `0x6B` and `0x7E`; the following identity
read then failed. Every development scan in this repository is therefore
limited to the inclusive range `0x08` through `0x77` and never transmits to
`0x7E`.

## PlatformIO environments

Use the PlatformIO project tasks in VS Code:

- `BOARD_XIAO_ESP32S3`: production build; starts and remains at 100 kHz.
- `BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC`: evaluation build; starts at 100 kHz
  and changes to 400 kHz only after sensor setup succeeds.
- `test_i2c_contract`: 27 compile-time contract cases for address limits, probe
  error classification, and LSM6DSV address resolution. PlatformIO compiles
  these cases for both XIAO environments without requiring a host C++ compiler.

The hardware test suite is `test_xiao_lsm6dsv_hardware`. It performs the direct
identity read before the safe scan. Uploading and serial execution must be done
from the VS Code PlatformIO Test task with the tracker connected.

Connect the XIAO directly to the PC with a data-capable USB cable; do not use a
hub while diagnosing upload or serial enumeration. In VS Code run `PlatformIO:
Set Project Port (upload/monitor/test)` and select the detected COM port before
starting the test. The hardware-test firmware waits for the native USB CDC
port briefly before emitting Unity output, so do not press RESET after an
automatic upload from normally running firmware. If automatic upload cannot enter the
bootloader, hold BOOT, press and release RESET, release BOOT, upload, and then
press RESET once to start the flashed firmware.

## Hardware acceptance tests

100 kHz remains the production setting until all unresolved hardware work is
recorded. The following checks cannot be claimed from an automated build:

1. Run the 100 kHz hardware suite and confirm that only `0x6B` is found.
2. Repeat after USB reset and after complete power removal for 10 consecutive
   successful cycles.
3. Disconnect the module and confirm an `ADDRESS_NACK` log including the send
   code and zero received bytes.
4. Run the 400 kHz diagnostic environment, including initialization, continuous
   sensor data, and 10 complete power cycles. Record the result, but do not
   promote it to the production default in this change.
5. At 100 kHz, run the tracker with SlimeVR Server for at least eight hours and
   record I2C errors, resets, and tracking interruptions.

WHO_AM_I mismatch and short-read paths are deterministically covered by the
compile-time contract tests; they should not be simulated by unreliable live
rewiring.
