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
| IMU INT | D9 / GPIO8 | Wired in the final build, but not used by LSM6DSV SoftFusion. RTC GPIO, so it stays available as a future EXT1 wake source |
| AUX IMU INT | D1 / GPIO2 | Reserved; no auxiliary IMU is installed |
| Battery ADC | D0 / GPIO1 | Reserved for the later battery build; unused in USB-powered mode |
| SDO / SA0 | High, held by the module's own 4.7 kOhm pull-up | Selects I2C address `0x6B`. No wire is needed. Bridging the top-side `SDO` solder jumper connects it to GND, selects `0x6A`, and the firmware then finds no sensor |
| CS / CSB | High, held by the module's own 4.7 kOhm pull-up | Required for I2C operation. No wire is needed |
| Power button | D8 / GPIO7 | Reserved for the momentary Deep Sleep button; external 10 kOhm pull-up to 3V3, button to GND. Do not reuse as SPI SCK |

The connection diagram is [`wiring-xiao-lsm6dsv.svg`](wiring-xiao-lsm6dsv.svg).
Its pad order matches the official Seeed front pinout image: left column
`D0..D6`, right column `VBUS`, `GND`, `3.3V-OUT`, `D10`, `D9`, `D8`, `D7`, and
`BAT+` / `BAT-` on the back.

The module silkscreen (revision `5265159A-P1-241122`, read from the BOOTH
product photographs) labels one row `OSDO`, `3V3`, `GND`, `SCL`, `SDA`, `CS`,
`SDO` and the other row `OCS`, `INT2`, `INT1`, `SCX`, `SDX`. Every pin this
build uses sits in the first row. `OSDO` / `OCS` / `SCX` / `SDX` are the OIS
auxiliary SPI pins and stay unused. The supply pin is labelled `3V3`, not `VIN`.
The top side carries an `SDO` solder jumper whose state is unconfirmed; the
confirmed fact is only that SA0 reads high and the device answers at `0x6B`.

Do not use 5 V logic on SDA, SCL, CS/CSB, or SDO/SA0.

Owner measurement of 2026-08-03, unpowered, on the module: `SDA`, `SCL`, `SDO`,
and `CS` each read 4.6 kOhm to `3V3`, so the module carries its own 4.7 kOhm
pull-ups on all four. No external I2C pull-up is required, and `SDO` and `CS`
need no wire: the module holds both high on its own. Only `3V3`, `GND`, `SDA`,
and `SCL` have to be connected. The top-side `SDO` solder jumper is open and its
free pad reads continuous to GND, so that jumper exists to select `0x6A`; leave
it open, or the firmware will find no sensor at `0x6B`.

The configured interrupt pins are not physically connected in the current
hardware, and the LSM6DSV SoftFusion path does not use them: `SoftFusionSensor`
takes `intPin` as an optional argument that defaults to `nullptr`, and the
LSM6DSV driver polls the FIFO over I2C instead. `INT1` therefore carries no
signal that the firmware reads today. It is still run in the final build,
because adding a wire to a finished, glued-down assembly is far more work than
running it now, and because motion wake from Deep Sleep is the one planned
feature that needs it. Battery-voltage measurement is still undecided.

`IMU INT` moved from GPIO4 to GPIO8 on 2026-08-04. GPIO8 and GPIO9 are both RTC
GPIOs on the ESP32-S3, so either can be armed as an EXT1 wake source; GPIO4
could too, but it sits in the left pad column next to `SDA` / `SCL`, whereas
GPIO8 (`D9`) is next to the GPIO7 (`D8`) button pad in the right column. Keeping
the button and INT1 on the same edge as `3V3` and `GND` removes two wire
crossings from the perfboard layout. GPIO9 (`D10`) is the documented alternate.
Neither is an ESP32-S3 strapping pin (those are GPIO0, 3, 45, 46).

## Firmware defaults

`board-defaults.json` is the source for the generated XIAO build flags; do not
replace the dedicated target with the older generic `BOARD_CUSTOM` path.

- board: `BOARD_XIAO_ESP32S3`
- primary IMU: `IMU_LSM6DSV` at `0x6B`, confirmed rotation `DEG_0`
- common pins: SDA GPIO5, SCL GPIO6, IMU INT GPIO8, AUX INT GPIO2, battery ADC GPIO1
- power button: GPIO7, active low, 2000 ms hold
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

On 2026-08-01, the replacement LSM6DSV passed 10 complete USB power-removal
cycles at 100 kHz. Each post-cycle serial reset confirmed address `0x6B`,
`WHO_AM_I == 0x70`, and rest calibration: 20 checks passed, 0 failed. The prior
miswired module measured approximately 0 ohms from VCC to GND while unpowered
and remains isolated from power.

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
