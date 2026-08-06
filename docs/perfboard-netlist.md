# Perfboard build: parts, nets, and placement constraints

Input material for the KiCad project in `hardware/tracker-perfboard/`. Every
electrical fact here comes from `docs/xiao-esp32s3-lsm6dsv.md` and the owner
measurements recorded in `HANDOFF.agent.md`.

## Parts

| Ref | Part | Value / note |
| --- | --- | --- |
| U1 | Seeed XIAO ESP32-S3 | 14 castellated pads, 2.54 mm pitch, 21 x 17.8 mm |
| U2 | LSM6DSV module (BOOTH, rev `5265159A-P1-241122`) | 12 pins in a 7 x 2 grid, 2.54 mm pitch, 13 x 18 mm |
| SW1 | Momentary button, normally open | Logic input only; never in series with the cell |
| R1 | 10 kOhm | GPIO7 pull-up to 3V3 |
| R2 | 100 kOhm | Battery divider, high side (`BAT+` to sense node) |
| R3 | 100 kOhm | Battery divider, low side (sense node to GND) |
| C1 | 100 nF 50 V MLCC | ADC settling cap, across `R3` (sense node to GND) |
| BT1 | Protected 1S Li-ion | To `BAT+` / `BAT-` on the XIAO back side |

**Owner decision of 2026-08-04: the optional `SW2` slide switch is not fitted.**
`BAT+` is therefore one net from the cell straight to the XIAO pad, and the
former `BAT+_RAW` no longer exists.

**Owner decision of 2026-08-04: `R2` and `R3` are 100 kOhm.** The 220 kOhm
alternative is declined, so divider idle draw stays at about 21 uA. The firmware
multiplier depends on the ratio, not the absolute value, so it is unaffected
either way.

**Owner decision of 2026-08-06 supersedes the 2026-08-04 one: `C1` is fitted**, a
100 nF 50 V multilayer ceramic across `R3`, so between the `BAT_SENSE` node and
`GND`. It is unpolarised and 50 V is far beyond the roughly 2 V this node ever
sees. It does not change the divider ratio, so `ADCMultiplier` stays 2 and no
firmware change follows from it. With `R2` and `R3` at 100 kOhm the settling time
constant is about 5 ms, which is irrelevant against a periodic reading, and idle
draw stays at about 21 uA.

The software filtering described below **stays in place**. It was written to do
this capacitor's job, and the two together are strictly better than either alone;
there is nothing to disable. The paragraph is kept because it explains why the
firmware looks the way it does.

The original 2026-08-04 reasoning, for that history: the divider is deliberately
high impedance to hold idle draw down, which leaves the ADC looking at a 50 kOhm
source, and a single conversion from such a source picks up spike-like errors.
Seeed's own XIAO battery-measurement
guide hits the same thing with a 200k/200k divider and answers it by averaging
16 readings. That job sits in firmware as well as in the capacitor: the ESP32
`BAT_EXTERNAL` path in `src/batterymonitor.cpp` throws one conversion away to
charge the sample-and-hold, then takes the median of `BATTERY_ADC_SAMPLES`
readings, set to 15 for both XIAO environments in `platformio.ini`. A median was
chosen over a mean because it discards outliers rather than smearing them in.

The limit of that trade is worth stating plainly: filtering in software removes
jitter and spikes, but **not** a steady droop. Any constant offset has to be
absorbed by the `BATTERY_SHIELD_RESISTANCE` calibration in chapter 13. Battery
level is a coarse indicator here, so that is an accepted cost.

## Nets

| Net | Connections |
| --- | --- |
| `+3V3` | U1 `3V3` (right column) — U2 `3V3` — R1 |
| `GND` | U1 `GND` (right column) — U2 `GND` — SW1 — R3 — C1 — BT1 `-` |
| `SDA` | U1 `D4` / GPIO5 — U2 `SDA` |
| `SCL` | U1 `D5` / GPIO6 — U2 `SCL` |
| `IMU_INT1` | U1 `D10` / GPIO9 — U2 `INT1` (unused by current firmware) |
| `PWR_BTN` | U1 `D8` / GPIO7 — R1 — SW1 |
| `BAT_SENSE` | U1 `D0` / GPIO1 — R2 — R3 — C1 |
| `BAT+` | BT1 `+` — XIAO `BAT+` pad — R2 |

Not wired, and deliberately so:

- U2 `SDO` and `CS`: held high by the module's own 4.7 kOhm pull-ups. The
  top-side `SDO` solder jumper shorts `SDO` to GND and selects `0x6A`; leave it
  open.
- U2 `OSDO`, `OCS`, `SCX`, `SDX`, `INT2`: OIS auxiliary SPI, unused.
- U1 `5V` (VBUS), `D1`, `D2`, `D3`, `D6`, `D7`, `D9`: unused. `D3` / GPIO4 carried
  `IMU_INT1` until 2026-08-04 and is now free; `D10` / GPIO9 took it over.
- External I2C pull-ups: the module already has 4.7 kOhm on `SDA` and `SCL`.

## ElectroCookie mini board

Confirmed from the vendor description: 17 columns by (5 + 5) rows, wired 1:1
like a breadboard, 50.8 x 38.1 mm, 1.2 mm holes.

**For each column number, holes `A`-`E` are one node and `F`-`J` are another.**
Two pins of the same part must never sit in the same column and bank, or they
are shorted. Both modules therefore have to straddle the centre channel.

Standard row pitch inside a bank is 2.54 mm and the channel makes `E` to `F`
7.62 mm, so the row positions in millimetres are:

```
A 0.00   B 2.54   C 5.08   D 7.62   E 10.16 | F 17.78  G 20.32  H 22.86  I 25.40  J 27.94
```

Row pairs that a part can span:

| Row spacing of the part | Usable row pairs |
| --- | --- |
| 10.16 mm (0.4 in) | D/F, E/G |
| 12.70 mm (0.5 in) | C/F, D/G, E/H |
| 15.24 mm (0.6 in) | B/F, C/G, D/H, E/I |
| 17.78 mm (0.7 in) | A/F, B/G, C/H, D/I, E/J |

## Measured module row spacing

Owner measurement of 2026-08-04, in 2.54 mm pitches across each board:

| Module | Pitches | Spacing | Cross-check | Legal row pairs |
| --- | --- | --- | --- | --- |
| U1 XIAO ESP32-S3 | 7 | 17.78 mm (0.7 in) | board is 17.8 mm wide, and the pads are castellated on the edge, so pad centres sit at the board edge | A/F, B/G, C/H, D/I, E/J |
| U2 LSM6DSV module | 5 | 12.70 mm (0.5 in) | board is 13 mm wide, same castellated-edge reasoning | C/F, D/G, E/H |

The figures are pitch counts, not hole counts. Both agree with the module
outlines above to within a fraction of a millimetre, which is what confirms that
reading.

**Column budget is tighter than it first looks.** Each module claims seven
columns, and every hole in a claimed column is already one of that module's two
nodes, so 14 of the 17 columns are spoken for and only three are fully free.
`SW1`, `R1`, `R2`, and `R3` still fit, because a part only needs its two
ends on two different nodes — bridging a module's node to a free column, or
bridging the two banks of a free column, both work. It does mean the column
assignment has to be planned deliberately rather than assumed roomy.

## Wiring consequences

`3V3` and `GND` are on the XIAO's right column while `SDA` and `SCL` are on its
left column, so power and I2C leave the module on opposite edges and the wiring
has to wrap around. The `D8` run to `SW1` crosses the `3V3` and `GND` returns;
plan those two crossings as insulated jumpers. See
`docs/wiring-xiao-lsm6dsv.svg`.

## Mounting orientation

The confirmed `DEG_0` rotation applies to the tracker as a whole: component side
away from the body, USB toward the feet. The angle between the IMU module and
the XIAO was never recorded, so once the boards are soldered down, either
reproduce the angle of the current working assembly or re-verify
`IMU_ROTATION` in SlimeVR Preview after assembly.
