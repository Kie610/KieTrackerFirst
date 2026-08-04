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
| C1 | 100 nF | Sense node to GND, ADC settling |
| BT1 | Protected 1S Li-ion | To `BAT+` / `BAT-` on the XIAO back side |
| SW2 | Slide switch (optional, undecided) | In the `BAT+` line if fitted |

`R2` / `R3` may both become 220 kOhm to cut idle current from about 21 uA to
about 9.5 uA. The firmware multiplier is unchanged because it depends on the
ratio, not the absolute value.

## Nets

| Net | Connections |
| --- | --- |
| `+3V3` | U1 `3V3` (right column) — U2 `3V3` — R1 |
| `GND` | U1 `GND` (right column) — U2 `GND` — SW1 — R3 — C1 — BT1 `-` |
| `SDA` | U1 `D4` / GPIO5 — U2 `SDA` |
| `SCL` | U1 `D5` / GPIO6 — U2 `SCL` |
| `IMU_INT1` | U1 `D9` / GPIO8 — U2 `INT1` (wired now, unused by current firmware) |
| `PWR_BTN` | U1 `D8` / GPIO7 — R1 — SW1 |
| `BAT_SENSE` | U1 `D0` / GPIO1 — R2 — R3 — C1 |
| `BAT+` | BT1 `+` — (SW2) — XIAO `BAT+` pad — R2 |

Not wired, and deliberately so:

- U2 `SDO` and `CS`: held high by the module's own 4.7 kOhm pull-ups. The
  top-side `SDO` solder jumper shorts `SDO` to GND and selects `0x6A`; leave it
  open.
- U2 `OSDO`, `OCS`, `SCX`, `SDX`, `INT2`: OIS auxiliary SPI, unused.
- U1 `5V` (VBUS), `D1`, `D2`, `D3`, `D6`, `D7`, `D10`: unused.

`IMU_INT1` moved from `D3` / GPIO4 to `D9` / GPIO8 on 2026-08-04. Both are RTC
GPIOs, so either would work as a future EXT1 wake source, but `D9` sits directly
beside `D8` / GPIO7 in the XIAO's right pad column. That puts the button line and
the INT1 line on the same board edge as `3V3` and `GND`, so only `SDA` / `SCL`
leave from the left column and the wrap-around wiring drops from four crossings
to two. `D10` / GPIO9 is the documented alternate if `D9` is ever needed for SPI.
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
| 15.24 mm (0.6 in) | B/F, C/G, D/H, E/I |
| 17.78 mm (0.7 in) | A/F, B/G, C/H, D/I, E/J |
| 10.16 mm (0.4 in) | D/F, E/G |

**Unresolved (U):** the pad-row spacing of the XIAO and of the LSM6DSV module
has not been measured. Measure pad centre to pad centre across each board with
calipers before committing to a placement; the table above then gives the legal
rows. Seventeen columns are available and the two modules need seven each, so
they fit side by side with columns to spare for `SW1`, `R1`, `R2`, `R3`, `C1`.

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
